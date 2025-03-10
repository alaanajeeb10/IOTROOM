#define WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <ESP8266WiFi.h>

class WiFiManager {
public:
    WiFiManager(const char* ssid, const char* password);
    void connectWiFi();
    bool isConnected();
    void handleReconnect();
private:
    const char* _ssid;
    const char* _password;
};

#endif
