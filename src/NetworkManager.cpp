#include "NetworkManager.h"

String apiUrl = "";
String backendUrl = "";
String apiKey = "";

NetworkManager::NetworkManager() : 
    custom_api_url("api_url", "URL API Bancaria", "", 128),
    custom_backend_url("backend_url", "URL Backend WebSocket", "", 128),
    custom_api_key("api_key", "API Key Bancaria", "", 64) 
{}

void NetworkManager::loadConfig() {
    preferences.begin("pos_config", true); // true for read-only
    apiUrl = preferences.getString("apiUrl", "https://api.banco.com/v1/charge");
    backendUrl = preferences.getString("backendUrl", "wss://tu-backend.com/ws");
    apiKey = preferences.getString("apiKey", "default_key");
    preferences.end();
}

void NetworkManager::saveConfig() {
    preferences.begin("pos_config", false); // false for read/write
    preferences.putString("apiUrl", apiUrl);
    preferences.putString("backendUrl", backendUrl);
    preferences.putString("apiKey", apiKey);
    preferences.end();
}

void NetworkManager::resetWiFi() {
    wifiManager.resetSettings();
}

void NetworkManager::init() {
    loadConfig();

    WiFi.mode(WIFI_AP_STA); // Forza el modo dual para evitar que el radio colapse al escanear
    WiFi.setSleep(false); // Estabiliza el Portal Cautivo evitando que la antena se duerma
    WiFiManager wifiManager;
    
    wifiManager.setRemoveDuplicateAPs(true);   // Ahorra RAM en la lista
    wifiManager.setMinimumSignalQuality(20);   // Filtra redes basura
    
    // Set custom values to the portal fields
    custom_api_url.setValue(apiUrl.c_str(), 128);
    custom_backend_url.setValue(backendUrl.c_str(), 128);
    custom_api_key.setValue(apiKey.c_str(), 64);

    wifiManager.addParameter(&custom_api_url);
    wifiManager.addParameter(&custom_backend_url);
    wifiManager.addParameter(&custom_api_key);

    wifiManager.setSaveConfigCallback([this]() {
        Serial.println("Guardando configuracion...");
        apiUrl = custom_api_url.getValue();
        backendUrl = custom_backend_url.getValue();
        apiKey = custom_api_key.getValue();
        this->saveConfig();
        Serial.println("Configuracion guardada correctamente");
    });

    // Fetches ssid and pass and tries to connect
    // if it does not connect it starts an access point with the specified name
    // and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("ESP32-POS-Config")) {
        Serial.println("Fallo al conectar o timeout");
        delay(3000);
        // Reset and try again
        ESP.restart();
    }

    // if you get here you have connected to the WiFi
    Serial.println("Conectado a WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}
