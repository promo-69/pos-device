#include "NetworkManager.h"

String apiUrl = "";
String backendUrl = "";
String apiKey = "";
String posApiKey = "";

NetworkManager::NetworkManager() : 
    custom_api_url("api_url", "URL API Bancaria", "", 128),
    custom_backend_url("backend_url", "URL Backend WebSocket", "", 128),
    custom_api_key("api_key", "API Key Bancaria", "", 64),
    custom_pos_api_key("pos_api_key", "POS API Key (WS)", "", 64)
{}

void NetworkManager::loadConfig() {
    preferences.begin("pos_config", true); // true for read-only
    apiUrl = preferences.getString("apiUrl", "http://192.168.31.95/api/v1/pos/transfer");
    backendUrl = preferences.getString("backendUrl", "https://192.168.31.95");
    apiKey = preferences.getString("apiKey", "mt_RI7xGuLUb4JWOsDTBx811SqiMAS6qC4z");
    posApiKey = preferences.getString("posApiKey", "mt_RI7xGuLUb4JWOsDTBx811SqiMAS6qC4z"); // Defaulting to same for now
    preferences.end();
}

void NetworkManager::saveConfig() {
    preferences.begin("pos_config", false); // false for read/write
    preferences.putString("apiUrl", apiUrl);
    preferences.putString("backendUrl", backendUrl);
    preferences.putString("apiKey", apiKey);
    preferences.putString("posApiKey", posApiKey);
    preferences.end();
}

void NetworkManager::resetWiFi() {
    wifiManager.resetSettings();
}

void NetworkManager::init() {
    loadConfig();

    WiFi.disconnect(false, true); // Limpiar estado previo de conexión para evitar cuelgues
    WiFi.mode(WIFI_AP_STA); // Forza el modo dual para evitar que el radio colapse al escanear
    WiFi.setSleep(false); // Estabiliza el Portal Cautivo evitando que la antena se duerma
    WiFiManager wifiManager;
    
    wifiManager.setRemoveDuplicateAPs(true);   // Ahorra RAM en la lista
    wifiManager.setMinimumSignalQuality(20);   // Filtra redes basura
    
    // Mejoras de estabilidad para ESP32-C3 al escanear:
    wifiManager.setConfigPortalTimeout(180);   // Si el usuario no hace nada en 3 min, reiniciar
    wifiManager.setConnectTimeout(15);         // No esperar demasiado si la contraseña es incorrecta
    
    // Set custom values to the portal fields
    custom_api_url.setValue(apiUrl.c_str(), 128);
    custom_backend_url.setValue(backendUrl.c_str(), 128);
    custom_api_key.setValue(apiKey.c_str(), 64);
    custom_pos_api_key.setValue(posApiKey.c_str(), 64);

    wifiManager.addParameter(&custom_api_url);
    wifiManager.addParameter(&custom_backend_url);
    wifiManager.addParameter(&custom_api_key);
    wifiManager.addParameter(&custom_pos_api_key);

    wifiManager.setSaveConfigCallback([this]() {
        Serial.println("Guardando configuracion...");
        apiUrl = custom_api_url.getValue();
        backendUrl = custom_backend_url.getValue();
        apiKey = custom_api_key.getValue();
        posApiKey = custom_pos_api_key.getValue();
        this->saveConfig();
        Serial.println("Configuracion guardada correctamente");
    });

    // Fetches ssid and pass and tries to connect
    // if it does not connect it starts an access point with the specified name
    // and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("Cineflix POS")) {
        Serial.println("Fallo al conectar o timeout");
        delay(3000);
        // Reset and try again
        ESP.restart();
    }

    // if you get here you have connected to the WiFi
    Serial.println("Conectado a WiFi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    
    // Configurar NTP para estampas de tiempo reales (UTC 0)
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Sincronizando reloj via NTP...");
}
