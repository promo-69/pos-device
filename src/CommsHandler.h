#ifndef COMMS_HANDLER_H
#define COMMS_HANDLER_H

#include <Arduino.h>
#include <WebServer.h>
#include <uri/UriBraces.h>
#include <SocketIOclient.h>
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
    void sendSocketIOEvent(const String& eventName, const String& jsonPayload);

private:
    WebServer server;
    SocketIOclient socketIO;
    bool wsEnabled;
    
    // Variables para el workaround de Socket.IO
    bool pendingWsAuth = false;
    unsigned long wsAuthTimer = 0;

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
    
    void handleHttpWsConnect();
    void handleHttpWsDisconnect();
    void handleHttpWsStatus();
    
    static void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length);
};

extern CommsHandler* globalComms;

#endif // COMMS_HANDLER_H
