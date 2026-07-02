#ifndef PAYMENT_BUFFER_H
#define PAYMENT_BUFFER_H

#include <Arduino.h>

enum EstadoOperacion {
    EN_COLA,
    PROCESANDO,
    COMPLETADO,
    ERROR
};

struct OperacionPago {
    char operationId[32];
    uint32_t orderId;
    float amount;
    char jsonPayload[256];
    EstadoOperacion estado;
    char source[16]; // "WS" or "HTTP"
    char responseMessage[128];
    uint32_t createdAt;
    uint32_t updatedAt;
    uint32_t emittedAt;
    bool isActive;
};

class PaymentBuffer {
public:
    PaymentBuffer();
    void init();
    OperacionPago* addOperation(const char* opId, uint32_t orderId, float amount, const char* payload, const char* source);
    OperacionPago* getOperationById(const char* opId);
    String getAllOperationsJson();
    void updateState(const char* opId, EstadoOperacion newState, const char* message);
    void setEmittedAt(const char* opId);

private:
    static const int MAX_BUFFER = 30;
    OperacionPago buffer[MAX_BUFFER];
    int currentIndex;
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    
    String getEstadoString(EstadoOperacion estado);
};

extern PaymentBuffer paymentBuffer;

#endif // PAYMENT_BUFFER_H
