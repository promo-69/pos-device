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
    while (uid == "") {
        uid = tryReadCard(true);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    isReaderBusy = false; // Libera el lector

    Serial.println(">>> TARJETA DETECTADA <<<");
    return uid;
}

void POSController::executeBankTransaction(const char* operationId) {
    OperacionPago* op = paymentBuffer.getOperationById(operationId);
    if (op == nullptr) return;

    paymentBuffer.updateState(operationId, PROCESANDO, "Por favor acerque la tarjeta al lector.");

    String cardId = readCardReal();

    paymentBuffer.updateState(operationId, PROCESANDO, "Autorizando con el banco...");

    if (WiFi.status() == WL_CONNECTED) {
        WiFiClientSecure client;
        client.setInsecure(); // INSECURE FOR PRODUCTION

        HTTPClient https;
        Serial.print("[HTTPS] Conectando a ");
        Serial.println(apiUrl);

        if (https.begin(client, apiUrl)) {
            https.addHeader("Content-Type", "application/json");
            https.addHeader("Authorization", "Bearer " + apiKey);

            // Inyectar cardUid al JSON raw de forma segura
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, op->jsonPayload);
            
            if (!error) {
                doc["cardUid"] = cardId;
                String requestBody;
                serializeJson(doc, requestBody);

                Serial.print("[HTTPS] Enviando POST: ");
                Serial.println(requestBody);

                int httpCode = https.POST(requestBody);
                String resultMessage = "";

                if (httpCode > 0) {
                    String payload = https.getString();
                    resultMessage = "Exito, codigo HTTP: " + String(httpCode);
                    paymentBuffer.updateState(operationId, COMPLETADO, resultMessage.c_str());
                } else {
                    resultMessage = "Error en peticion: " + https.errorToString(httpCode);
                    paymentBuffer.updateState(operationId, ERROR, resultMessage.c_str());
                }

                // Decoupled Response Channel
                if (strcmp(op->source, "WSS") == 0 && globalComms != nullptr) {
                    String wsMsg = "{\"operationId\":\"" + String(operationId) + "\", \"status\":\"" + String(op->estado == COMPLETADO ? "success" : "error") + "\"}";
                    globalComms->sendWebSocketMessage(wsMsg);
                }
            } else {
                paymentBuffer.updateState(operationId, ERROR, "Error re-estructurando JSON");
            }
            https.end();
        } else {
            paymentBuffer.updateState(operationId, ERROR, "Incapaz de conectar");
        }
    } else {
         paymentBuffer.updateState(operationId, ERROR, "WiFi Desconectado");
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
