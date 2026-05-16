#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Adafruit_NeoPixel.h>

#define LED_PIN 48
#define NUM_PIXELS 1

enum LedStatus {
    LED_OFF,
    LED_INITIALIZED,
    LED_CONNECTING,
    LED_CONNECTED,
    LED_ATTACKING
};

class LedManager {
public:
    static void begin() {
        pixels.begin();
        pixels.setBrightness(50);
        pixels.show();
        xTaskCreate(ledTask, "led_task", 2048, NULL, 1, NULL);
    }

    static void setStatus(LedStatus status) {
        _status = status;
        if (status == LED_INITIALIZED) {
            _initializedBlinks = 0;
        }
    }

private:
    static Adafruit_NeoPixel pixels;
    static LedStatus _status;
    static int _initializedBlinks;

    static void ledTask(void* pvParameters) {
        while (true) {
            switch (_status) {
                case LED_OFF:
                    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
                    pixels.show();
                    break;
                
                case LED_INITIALIZED:
                    if (_initializedBlinks < 2) {
                        pixels.setPixelColor(0, pixels.Color(255, 165, 0)); // Orange
                        pixels.show();
                        vTaskDelay(pdMS_TO_TICKS(200));
                        pixels.setPixelColor(0, pixels.Color(0, 0, 0));
                        pixels.show();
                        vTaskDelay(pdMS_TO_TICKS(200));
                        _initializedBlinks++;
                    } else {
                        _status = LED_OFF;
                    }
                    break;

                case LED_CONNECTING:
                    pixels.setPixelColor(0, pixels.Color(255, 165, 0)); // Orange
                    pixels.show();
                    vTaskDelay(pdMS_TO_TICKS(500));
                    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
                    pixels.show();
                    vTaskDelay(pdMS_TO_TICKS(500));
                    break;

                case LED_CONNECTED:
                    pixels.setPixelColor(0, pixels.Color(0, 255, 0)); // Green
                    pixels.show();
                    break;

                case LED_ATTACKING:
                    pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Red
                    pixels.show();
                    vTaskDelay(pdMS_TO_TICKS(300));
                    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
                    pixels.show();
                    vTaskDelay(pdMS_TO_TICKS(300));
                    break;
            }
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
};

inline Adafruit_NeoPixel LedManager::pixels = Adafruit_NeoPixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
inline LedStatus LedManager::_status = LED_OFF;
inline int LedManager::_initializedBlinks = 2;

#endif
