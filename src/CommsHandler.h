#ifndef COMMS_HANDLER_H
#define COMMS_HANDLER_H

#include <Arduino.h>
#include <WebServer.h>
#include <WebSocketsClient.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

struct QueueItem {
    char operationId[32];
};

extern QueueHandle_t orderQueue;

class CommsHandler {
public:
    CommsHandler();
    void init();
    void loop();
    void sendWebSocketMessage(const String& msg);

private:
    WebServer server;
    WebSocketsClient webSocket;
    bool wsEnabled;
    
    void setupHttpServer();
    void setupWebSocket();
    
    void handleHttpRoot();
    void handleHttpGetConfig();
    void handleHttpConfig();
    void handleHttpResetWiFi();
    void handleHttpReadCard();
    
    void handleHttpPaymentsPost();
    void handleHttpPaymentsGetList();
    void handleHttpPaymentsGetDetail();
    
    static void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
};

extern CommsHandler* globalComms;

#endif // COMMS_HANDLER_H
