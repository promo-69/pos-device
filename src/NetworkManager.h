#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <WiFiManager.h>
#include <Preferences.h>

extern String apiUrl;
extern String backendUrl;
extern String apiKey;
extern String posApiKey;

class NetworkManager {
public:
    NetworkManager();
    void init();
    void loadConfig();
    void saveConfig();
    void resetWiFi();

private:
    Preferences preferences;
    WiFiManager wifiManager;

    // WiFiManager params
    WiFiManagerParameter custom_api_url;
    WiFiManagerParameter custom_backend_url;
    WiFiManagerParameter custom_api_key;
    WiFiManagerParameter custom_pos_api_key;
};

extern NetworkManager networkManager;

#endif // NETWORK_MANAGER_H
