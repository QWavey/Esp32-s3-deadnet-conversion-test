#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Preferences.h>
#include <vector>

struct WifiCredential {
    String ssid;
    String password;
};

class ConfigManager {
public:
    static void begin() {
        preferences.begin("deadnet", false);
    }

    static void saveCredential(String ssid, String password) {
        String key = "w_" + String(getCount());
        preferences.putString((key + "_s").c_str(), ssid);
        preferences.putString((key + "_p").c_str(), password);
        setCount(getCount() + 1);
    }

    static std::vector<WifiCredential> getCredentials() {
        std::vector<WifiCredential> creds;
        int count = getCount();
        for (int i = 0; i < count; i++) {
            String key = "w_" + String(i);
            WifiCredential cred;
            cred.ssid = preferences.getString((key + "_s").c_str(), "");
            cred.password = preferences.getString((key + "_p").c_str(), "");
            if (cred.ssid != "") {
                creds.push_back(cred);
            }
        }
        return creds;
    }

    static void clearCredentials() {
        preferences.clear();
        setCount(0);
    }

private:
    static Preferences preferences;
    
    static int getCount() {
        return preferences.getInt("count", 0);
    }

    static void setCount(int count) {
        preferences.putInt("count", count);
    }
};

inline Preferences ConfigManager::preferences;

#endif
