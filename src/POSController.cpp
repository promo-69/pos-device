#include "POSController.h"
#include "CommsHandler.h"
#include "NetworkManager.h"
#include "PaymentBuffer.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         3
#define SS_PIN          7

MFRC522 mfrc522(SS_PIN, RST_PIN);
SemaphoreHandle_t rfidMutex = NULL;
bool isReaderBusy = false;

POSController::POSController() {}

void POSController::init() {
    rfidMutex = xSemaphoreCreateMutex();
    
    // Inicializar SPI estricto: SCK=4, MISO=5, MOSI=6, SS=7
    SPI.begin(4, 5, 6, 7);
    mfrc522.PCD_Init();
    delay(4); // Opcional para estabilizar MFRC522
    Serial.println("RC522 Inicializado");

    xTaskCreate(
        taskProcesarCobros,   /* Task function. */
        "ProcesarCobros",     /* String with name of task. */
        8192,                 /* Stack size (Mínimo recomendado para TLS/HTTPS) */
        NULL,                 /* Parameter */
        1,                    /* Priority */
        NULL);                /* Task handle. */
}

String POSController::tryReadCard(bool fromPayment) {
    if (!fromPayment && isReaderBusy) {
        // Si el lector libre intenta leer, pero hay un cobro activo, lo rechazamos para respetar el orden.
        return "";
    }

    if (rfidMutex == NULL) return "";
    
    if (xSemaphoreTake(rfidMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
            xSemaphoreGive(rfidMutex);
            return "";
        }
        
        String uidString = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
            uidString += String(mfrc522.uid.uidByte[i], HEX);
        }
        uidString.toUpperCase();
        
        mfrc522.PICC_HaltA();
        xSemaphoreGive(rfidMutex);
        return uidString;
    }
    return "";
}

String POSController::readCardReal() {
    Serial.println(">>> ESPERANDO TARJETA RC522 <<<");
    
    isReaderBusy = true; // Bloquea el acceso al lector libre
    String uid = "";
    uint32_t startWait = millis();
    
    while (uid == "") {
        if (millis() - startWait > 15000) {
            Serial.println(">>> TIEMPO ESPERA TARJETA AGOTADO <<<");
            break;
        }
        uid = tryReadCard(true);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    isReaderBusy = false; // Libera el lector

    if (uid != "") {
        Serial.println(">>> TARJETA DETECTADA <<<");
    }
    return uid;
}

void POSController::executeBankTransaction(const char* operationId) {
    OperacionPago* op = paymentBuffer.getOperationById(operationId);
    if (op == nullptr) return;

    paymentBuffer.updateState(operationId, PROCESANDO, "Por favor acerque la tarjeta al lector.");

    String cardId = readCardReal();
    
    if (cardId == "") {
        paymentBuffer.updateState(operationId, ERROR, "Tiempo de espera agotado. No se leyo tarjeta.");
        if (strcmp(op->source, "WSS") == 0 && globalComms != nullptr) {
            JsonDocument frontDoc;
            deserializeJson(frontDoc, op->jsonPayload);
            String ticketId = frontDoc["ticketId"] | "";
            
            String wsMsg = "{\"ticketId\":\"" + ticketId + "\", \"success\":false, \"reference_number\":\"\", \"message\":\"Tiempo de espera agotado\"}";
            globalComms->sendSocketIOEvent("pos:payment_result", wsMsg);
        }
        return; // Cancelamos ejecucion y pasamos a la sig en cola
    }

    paymentBuffer.updateState(operationId, PROCESANDO, "Autorizando con el banco...");

    if (WiFi.status() == WL_CONNECTED) {
        bool isHttps = String(apiUrl).startsWith("https://");
        WiFiClient* client = nullptr;
        
        if (isHttps) {
            WiFiClientSecure* secureClient = new WiFiClientSecure();
            secureClient->setInsecure(); // INSECURE FOR PRODUCTION
            client = secureClient;
        } else {
            client = new WiFiClient();
        }

        HTTPClient http;
        http.setTimeout(30000); // Darle hasta 15 segundos al servidor para responder
        Serial.print(isHttps ? "[HTTPS] Conectando a " : "[HTTP] Conectando a ");
        Serial.println(apiUrl);

        if (http.begin(*client, apiUrl)) {
            http.addHeader("Content-Type", "application/json");
            http.addHeader("Authorization", "Bearer " + String(apiKey));

            // Construir JSON estricto para el API bancario
            JsonDocument frontDoc;
            DeserializationError error = deserializeJson(frontDoc, op->jsonPayload);
            
            if (!error) {
                JsonDocument bankDoc;
                bankDoc["sourceCard"] = cardId;
                bankDoc["destinationDocument"] = frontDoc["destinationDocument"];
                bankDoc["destinationAccount"] = frontDoc["destinationAccount"];
                bankDoc["amount"] = frontDoc["amount"];

                String requestBody;
                serializeJson(bankDoc, requestBody);

                Serial.print(isHttps ? "[HTTPS] Enviando POST: " : "[HTTP] Enviando POST: ");
                Serial.println(requestBody);

                int httpCode = http.POST(requestBody);
                String resultMessage = "";
                String bankReference = "";
                float bankAmount = 0.0f;

                if (httpCode > 0) {
                    String payload = http.getString();
                    if (httpCode >= 200 && httpCode < 300) {
                        resultMessage = "Aprobado (HTTP " + String(httpCode) + ")";
                        
                        JsonDocument successDoc;
                        if (!deserializeJson(successDoc, payload)) {
                            if (successDoc["data"].is<JsonObject>()) {
                                JsonObject dataObj = successDoc["data"].as<JsonObject>();
                                if (dataObj.containsKey("reference")) bankReference = dataObj["reference"].as<String>();
                                else if (dataObj.containsKey("id")) bankReference = dataObj["id"].as<String>();
                                
                                if (dataObj.containsKey("amount")) {
                                    bankAmount = abs(dataObj["amount"].as<float>());
                                }
                            } else if (successDoc.containsKey("reference_number")) {
                                bankReference = successDoc["reference_number"].as<String>();
                            }
                        }
                        
                        paymentBuffer.updateState(operationId, COMPLETADO, resultMessage.c_str());
                    } else {
                        JsonDocument errDoc;
                        DeserializationError errParsing = deserializeJson(errDoc, payload);
                        if (!errParsing && errDoc["error"].is<String>()) {
                            resultMessage = errDoc["error"].as<String>();
                        } else {
                            resultMessage = "Denegado (HTTP " + String(httpCode) + ")";
                        }
                        paymentBuffer.updateState(operationId, ERROR, resultMessage.c_str());
                    }
                } else {
                    resultMessage = "Fallo de red: " + http.errorToString(httpCode);
                    paymentBuffer.updateState(operationId, ERROR, resultMessage.c_str());
                }

                // Decoupled Response Channel
                if (strcmp(op->source, "WSS") == 0 && globalComms != nullptr) {
                    String ticketId = frontDoc["ticketId"] | "";
                    bool isSuccess = (op->estado == COMPLETADO);
                    String finalRef = (isSuccess && bankReference.length() > 0) ? bankReference : (isSuccess ? operationId : "");
                    
                    // Si el banco no retornó monto o hubo error, enviar el monto original pedido
                    float finalAmount = (isSuccess && bankAmount > 0.0f) ? bankAmount : op->amount;
                    
                    String wsMsg = "{\"ticketId\":\"" + ticketId + "\", \"success\":" + String(isSuccess ? "true" : "false") + 
                                   ", \"reference_number\":\"" + finalRef + "\", \"message\":\"" + resultMessage + "\"" +
                                   ", \"amount\":" + String(finalAmount, 2) + "}";
                                   
                    globalComms->sendSocketIOEvent("pos:payment_result", wsMsg);
                    paymentBuffer.setEmittedAt(operationId);
                }
            } else {
                paymentBuffer.updateState(operationId, ERROR, "Error re-estructurando JSON");
            }
            http.end();
        } else {
            paymentBuffer.updateState(operationId, ERROR, "Error conectando al banco");
        }
        delete client;
    } else {
        paymentBuffer.updateState(operationId, ERROR, "WiFi desconectado");
    }
}

void POSController::taskProcesarCobros(void *parameter) {
    QueueItem currentItem;

    for (;;) {
        if (xQueueReceive(orderQueue, &currentItem, portMAX_DELAY) == pdPASS) {
            Serial.println("==========================================");
            Serial.printf("INICIANDO OPERACION: %s\n", currentItem.operationId);
            
            executeBankTransaction(currentItem.operationId);
            
            Serial.println("OPERACION FINALIZADA");
            Serial.println("==========================================");
        }
    }
}
