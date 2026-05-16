#include <Arduino.h>
#include <esp_wifi.h>
#include "config_manager.h"
#include "deadnet.h"
#include "web_server.h"
#include "led_manager.h"
#include "logger.h"

Deadnet deadnet;

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n=== Deadnet ESP32 ===");

    if (WiFi.status() == WL_NO_SHIELD) {
        // Just dummy check
    }

    LedManager::begin();
    ConfigManager::begin();
    deadnet.begin();

    // ── WiFi: AP + STA mode ──────────────────────────────────────────────
    // We need AP mode so the user can access the web UI even when not
    // connected to a network yet.  STA mode is used for the actual attack.
    WiFi.mode(WIFI_AP_STA);

    // Enable packet injection: must set country and enable 802.11 raw tx
    wifi_country_t country = {"DE", 1, 13, 20, WIFI_COUNTRY_POLICY_MANUAL};
    esp_wifi_set_country(&country);

    // Promiscuous mode IS required for many ESP32-S3 chips to allow raw injection
    esp_wifi_set_promiscuous(true);
    
    // Start AP for control panel
    WiFi.softAP("Deadnet_Setup", "admin123");
    Serial.println("AP Started: Deadnet_Setup | IP: " + WiFi.softAPIP().toString());

    // Auto-connect to the last saved network (if any)
    auto creds = ConfigManager::getCredentials();
    if (!creds.empty()) {
        const auto& last = creds.back();
        Logger::log("Auto-connecting to: " + last.ssid);
        WiFi.begin(last.ssid.c_str(), last.password.c_str());
    }

    DeadnetWebServer::begin(&deadnet);
    LedManager::setStatus(LED_INITIALIZED);

    Serial.println("Setup complete. Connect to AP 'Deadnet_Setup' / 'admin123'");
    Serial.println("Web UI: http://192.168.4.1");
}

void loop() {
    DeadnetWebServer::handleClient();

    // Update LED status based on state
    static wl_status_t lastWifi = WL_IDLE_STATUS;
    wl_status_t curWifi = WiFi.status();

    if (!deadnet.isRunning()) {
        if (curWifi != lastWifi) {
            if (curWifi == WL_CONNECTED) {
                Logger::log("WiFi connected. IP: " + WiFi.localIP().toString());
                LedManager::setStatus(LED_CONNECTED);
            } else if (lastWifi == WL_CONNECTED) {
                Logger::log("WiFi disconnected.", "warning");
                LedManager::setStatus(LED_INITIALIZED);
            }
            lastWifi = curWifi;
        }
    } else {
        LedManager::setStatus(LED_ATTACKING);
        lastWifi = curWifi; // keep tracking
    }

    delay(10);
}
