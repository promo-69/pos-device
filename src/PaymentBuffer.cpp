#include "PaymentBuffer.h"
#include <ArduinoJson.h>

PaymentBuffer paymentBuffer;

PaymentBuffer::PaymentBuffer() {
    currentIndex = 0;
}

void PaymentBuffer::init() {
    portENTER_CRITICAL(&mux);
    for (int i = 0; i < 50; i++) {
        buffer[i].isActive = false;
    }
    portEXIT_CRITICAL(&mux);
}

OperacionPago* PaymentBuffer::addOperation(const char* opId, uint32_t orderId, float amount, const char* payload, const char* source) {
    portENTER_CRITICAL(&mux);
    
    int index = currentIndex;
    
    strlcpy(buffer[index].operationId, opId, sizeof(buffer[index].operationId));
    buffer[index].orderId = orderId;
    buffer[index].amount = amount;
    strlcpy(buffer[index].jsonPayload, payload, sizeof(buffer[index].jsonPayload));
    strlcpy(buffer[index].source, source, sizeof(buffer[index].source));
    buffer[index].estado = EN_COLA;
    strlcpy(buffer[index].responseMessage, "Esperando procesamiento", sizeof(buffer[index].responseMessage));
    buffer[index].isActive = true;
    
    currentIndex = (currentIndex + 1) % 50; // Circular
    
    portEXIT_CRITICAL(&mux);
    
    return &buffer[index];
}

OperacionPago* PaymentBuffer::getOperationById(const char* opId) {
    portENTER_CRITICAL(&mux);
    OperacionPago* found = nullptr;
    for (int i = 0; i < 50; i++) {
        if (buffer[i].isActive && strcmp(buffer[i].operationId, opId) == 0) {
            found = &buffer[i];
            break;
        }
    }
    portEXIT_CRITICAL(&mux);
    return found;
}

void PaymentBuffer::updateState(const char* opId, EstadoOperacion newState, const char* message) {
    portENTER_CRITICAL(&mux);
    for (int i = 0; i < 50; i++) {
        if (buffer[i].isActive && strcmp(buffer[i].operationId, opId) == 0) {
            buffer[i].estado = newState;
            strlcpy(buffer[i].responseMessage, message, sizeof(buffer[i].responseMessage));
            break;
        }
    }
    portEXIT_CRITICAL(&mux);
}

String PaymentBuffer::getEstadoString(EstadoOperacion estado) {
    switch(estado) {
        case EN_COLA: return "EN_COLA";
        case PROCESANDO: return "PROCESANDO";
        case COMPLETADO: return "COMPLETADO";
        case ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

String PaymentBuffer::getAllOperationsJson() {
    JsonDocument doc;
    JsonArray array = doc.to<JsonArray>();
    
    portENTER_CRITICAL(&mux);
    for (int i = 0; i < 50; i++) {
        if (buffer[i].isActive) {
            JsonObject obj = array.add<JsonObject>();
            obj["operationId"] = buffer[i].operationId;
            obj["orderId"] = buffer[i].orderId;
            obj["amount"] = buffer[i].amount;
            obj["state"] = getEstadoString(buffer[i].estado);
            obj["source"] = buffer[i].source;
        }
    }
    portEXIT_CRITICAL(&mux);
    
    String output;
    serializeJson(doc, output);
    return output;
}
