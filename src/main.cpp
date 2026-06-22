#include <Arduino.h>
#include "NetworkManager.h"
#include "CommsHandler.h"
#include "POSController.h"

NetworkManager networkManager;
CommsHandler commsHandler;
POSController posController;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n--- INICIANDO ESP32 POS ---");

  // 1. Init Network & Captive Portal
  networkManager.init();

  // 2. Init Communications (WebSockets, HTTP Server)
  commsHandler.init();

  // 3. Init POS Logic (Tasks & Queues)
  posController.init();
}

void loop() {
  // Keep communications alive
  commsHandler.loop();
  
  // FreeRTOS tasks handle the rest in the background
}