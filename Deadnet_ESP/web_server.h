#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include "web_ui.h"
#include "config_manager.h"
#include "deadnet.h"
#include "led_manager.h"
#include "logger.h"

class DeadnetWebServer {
public:
    static void begin(Deadnet* deadnet) {
        _deadnet = deadnet;

        server.on("/",                HTTP_GET, handleRoot);
        server.on("/api/status",      HTTP_GET, handleStatus);
        server.on("/api/connect",     HTTP_GET, handleConnect);
        server.on("/api/disconnect",  HTTP_GET, handleDisconnect);
        server.on("/api/networks",    HTTP_GET, handleGetNetworks);
        server.on("/api/scan",        HTTP_GET, handleScan);
        server.on("/api/scan-results",HTTP_GET, handleScanResults);
        server.on("/api/logs",        HTTP_GET, handleGetLogs);
        server.on("/api/start",          HTTP_GET, handleStartAttack);
        server.on("/api/stop",           HTTP_GET, handleStopAttack);
        server.on("/api/hosts",          HTTP_GET, handleGetHosts);
        server.on("/api/clear-hosts",    HTTP_GET, handleClearHosts);
        server.on("/api/clear-networks", HTTP_GET, handleClearNetworks);
        server.on("/api/sniff",          HTTP_GET, handleGetSniffedData);
        server.on("/api/clear-sniff",    HTTP_GET, handleClearSniffLogs);
        
        server.on("/style.css", HTTP_GET, []() {
            server.send_P(200, "text/css", STYLE_CSS);
        });
        server.on("/main.js", HTTP_GET, []() {
            server.send_P(200, "text/javascript", MAIN_JS);
        });

        server.begin();
        Logger::log("Web server started on port 80.");
    }

    static void handleClient() {
        server.handleClient();
    }

private:
    static WebServer server;
    static Deadnet*  _deadnet;

    // ── helpers ────────────────────────────────────────────────────────────
    static void addCors() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
    }

    // ── handlers ──────────────────────────────────────────────────────────
    static void handleRoot() {
        addCors();
        server.send_P(200, "text/html", INDEX_HTML);
    }

    // Combined status endpoint – poll this every second instead of multiple calls
    static void handleStatus() {
        addCors();
        bool   running   = _deadnet->isRunning();
        bool   connected = (WiFi.status() == WL_CONNECTED);
        String json = "{";
        json += "\"running\":"   + String(running   ? "true":"false") + ",";
        json += "\"connected\":" + String(connected  ? "true":"false") + ",";
        json += "\"ssid\":\""    + (connected ? WiFi.SSID() : String("")) + "\",";
        json += "\"ip\":\""      + (connected ? WiFi.localIP().toString() : String("")) + "\",";
        json += "\"gateway\":\""  + _deadnet->getGatewayIpStr() + "\",";
        json += "\"gatewayMac\":\"" + _deadnet->getGatewayMacStr() + "\",";
        json += "\"packets\":"   + String(_deadnet->getPacketsSent()) + ",";
        json += "\"cycles\":"    + String(_deadnet->getCycleCount()) + ",";
        json += "\"hosts\":"     + String(_deadnet->getHosts().size());
        json += "}";
        server.send(200, "application/json", json);
    }

    static void handleConnect() {
        addCors();
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");

        if (ssid.isEmpty()) {
            server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing SSID\"}");
            return;
        }

        if (_deadnet->isRunning()) {
            server.send(400, "application/json", "{\"success\":false,\"error\":\"Stop attack first\"}");
            return;
        }

        ConfigManager::saveCredential(ssid, pass);
        WiFi.disconnect(true);
        delay(100);
        LedManager::setStatus(LED_CONNECTING);
        Logger::log("Connecting to: " + ssid);
        WiFi.begin(ssid.c_str(), pass.c_str());

        // Wait up to 10 s for connection
        unsigned long t = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) {
            delay(200);
        }

        if (WiFi.status() == WL_CONNECTED) {
            Logger::log("Connected! IP: " + WiFi.localIP().toString());
            LedManager::setStatus(LED_CONNECTED);
            server.send(200, "application/json",
                "{\"success\":true,\"ip\":\"" + WiFi.localIP().toString() + "\"}");
        } else {
            Logger::log("Connection failed.", "error");
            LedManager::setStatus(LED_INITIALIZED);
            server.send(200, "application/json",
                "{\"success\":false,\"error\":\"Connection timed out\"}");
        }
    }

    static void handleDisconnect() {
        addCors();
        if (_deadnet->isRunning()) _deadnet->stopAttack();
        _deadnet->clearHosts();
        WiFi.disconnect(true);
        LedManager::setStatus(LED_INITIALIZED);
        Logger::log("WiFi disconnected.");
        server.send(200, "application/json", "{\"success\":true}");
    }

    static void handleGetNetworks() {
        addCors();
        auto   creds = ConfigManager::getCredentials();
        String json  = "{\"networks\":[";
        for (size_t i = 0; i < creds.size(); i++) {
            if (i) json += ",";
            // Don't send password over the wire
            json += "{\"ssid\":\"" + creds[i].ssid + "\"}";
        }
        json += "]}";
        server.send(200, "application/json", json);
    }

    static void handleClearNetworks() {
        addCors();
        ConfigManager::clearCredentials();
        _deadnet->clearHosts();
        Logger::log("All saved networks cleared.");
        server.send(200, "application/json", "{\"success\":true}");
    }

    static void handleScan() {
        addCors();
        if (_deadnet->isRunning()) {
            server.send(400, "application/json",
                "{\"success\":false,\"error\":\"Stop attack before scanning\"}");
            return;
        }
        int16_t state = WiFi.scanComplete();
        if (state == WIFI_SCAN_RUNNING) {
            server.send(200, "application/json",
                "{\"success\":true,\"message\":\"Already scanning\"}");
            return;
        }
        // async scan
        WiFi.scanNetworks(true, false);
        server.send(200, "application/json", "{\"success\":true}");
    }

    static void handleScanResults() {
        addCors();
        int16_t n = WiFi.scanComplete();
        if (n == WIFI_SCAN_RUNNING) {
            server.send(200, "application/json", "{\"status\":\"scanning\"}");
            return;
        }
        if (n < 0) {
            server.send(200, "application/json",
                "{\"status\":\"error\",\"code\":" + String(n) + "}");
            return;
        }
        String json = "{\"status\":\"complete\",\"networks\":[";
        for (int i = 0; i < n; i++) {
            if (i) json += ",";
            String ssid = WiFi.SSID(i);
            ssid.replace("\"", "\\\"");
            json += "{";
            json += "\"ssid\":\""       + ssid + "\",";
            json += "\"rssi\":"         + String(WiFi.RSSI(i)) + ",";
            json += "\"bssid\":\""      + WiFi.BSSIDstr(i) + "\",";
            json += "\"encryption\":"   + String((int)WiFi.encryptionType(i)) + ",";
            json += "\"channel\":"      + String(WiFi.channel(i));
            json += "}";
        }
        json += "]}";
        WiFi.scanDelete();
        server.send(200, "application/json", json);
    }

    static void handleGetLogs() {
        addCors();
        String json = "{\"logs\":[";
        const auto& logs = Logger::logs;
        for (size_t i = 0; i < logs.size(); i++) {
            if (i) json += ",";
            String e = logs[i];
            e.replace("\\", "\\\\");
            e.replace("\"", "\\\"");
            json += "\"" + e + "\"";
        }
        json += "]}";
        server.send(200, "application/json", json);
    }

    static void handleGetHosts() {
        addCors();
        auto   hosts = _deadnet->getHosts();
        String json  = "{\"hosts\":[";
        for (size_t i = 0; i < hosts.size(); i++) {
            if (i) json += ",";
            char mac[18];
            snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                     hosts[i].mac[0], hosts[i].mac[1], hosts[i].mac[2],
                     hosts[i].mac[3], hosts[i].mac[4], hosts[i].mac[5]);
            json += "{\"ip\":\"" + hosts[i].ip.toString() + "\",";
            json += "\"mac\":\""  + String(mac) + "\",";
            json += "\"hostname\":\"" + hosts[i].hostname + "\",";
            json += "\"pingMs\":" + String(hosts[i].pingMs) + ",";
            json += "\"macKnown\":" + String(hosts[i].macKnown ? "true":"false") + "}";
        }
        json += "]}";
        server.send(200, "application/json", json);
    }

    static void handleGetSniffedData() {
        addCors();
        auto logs = _deadnet->getSniffedData();
        String json = "{\"logs\":[";
        for (size_t i = 0; i < logs.size(); i++) {
            if (i) json += ",";
            json += "{";
            json += "\"src\":\"" + logs[i].source + "\",";
            json += "\"type\":\"" + logs[i].type + "\",";
            json += "\"content\":\"";
            String c = logs[i].content;
            c.replace("\"", "\\\"");
            json += c + "\",";
            json += "\"time\":" + String(logs[i].timestamp);
            json += "}";
        }
        json += "]}";
        server.send(200, "application/json", json);
    }

    static void handleClearSniffLogs() {
        addCors();
        _deadnet->clearSniffLogs();
        server.send(200, "application/json", "{\"success\":true}");
    }

    static void handleClearHosts() {
        addCors();
        _deadnet->clearHosts();
        server.send(200, "application/json", "{\"success\":true}");
    }

    static void handleStartAttack() {
        addCors();
        if (WiFi.status() != WL_CONNECTED) {
            Logger::log("Start failed: not connected to WiFi.", "error");
            server.send(400, "application/json",
                "{\"success\":false,\"error\":\"Connect to a WiFi network first\"}");
            return;
        }
        if (_deadnet->isRunning()) {
            server.send(200, "application/json",
                "{\"success\":true,\"message\":\"Already running\"}");
            return;
        }

        // Parse mode: either a numeric bitmask or a string keyword
        String modeStr = server.arg("mode");
        uint8_t mode = 0;
        
        if (modeStr.length() > 0 && isDigit(modeStr[0])) {
            mode = (uint8_t)modeStr.toInt();
        } else {
            if (modeStr == "both") mode = ATTACK_MODE_BOTH;
            else if (modeStr == "ra") mode = ATTACK_MODE_RA;
            else if (modeStr == "deauth") mode = ATTACK_MODE_DEAUTH;
            else if (modeStr == "dns") mode = ATTACK_MODE_DNS;
            else if (modeStr == "blind") mode = ATTACK_MODE_BLIND | ATTACK_MODE_DNS;
            else if (modeStr == "sniff") mode = ATTACK_MODE_SNIFF;
            else mode = ATTACK_MODE_ARP;
        }

        // Parse targets (optional)
        std::vector<IPAddress> targets;
        String targetsStr = server.arg("targets");
        if (targetsStr.length() > 0) {
            int start = 0;
            int end = targetsStr.indexOf(',');
            while (end != -1) {
                IPAddress ip;
                if (ip.fromString(targetsStr.substring(start, end))) targets.push_back(ip);
                start = end + 1;
                end = targetsStr.indexOf(',', start);
            }
            IPAddress ip;
            if (ip.fromString(targetsStr.substring(start))) targets.push_back(ip);
        }

        bool ok = _deadnet->startAttack(mode, targets);
        if (ok) {
            LedManager::setStatus(LED_ATTACKING);
            server.send(200, "application/json", "{\"success\":true}");
        } else {
            server.send(500, "application/json",
                "{\"success\":false,\"error\":\"Failed to start attack task\"}");
        }
    }

    static void handleStopAttack() {
        addCors();
        _deadnet->stopAttack();
        LedManager::setStatus(
            WiFi.status() == WL_CONNECTED ? LED_CONNECTED : LED_INITIALIZED);
        Logger::log("Attack stopped by user.");
        server.send(200, "application/json", "{\"success\":true}");
    }
};

inline WebServer DeadnetWebServer::server(80);
inline Deadnet*  DeadnetWebServer::_deadnet = nullptr;

#endif // WEB_SERVER_H
