#include <M5Unified.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include "utility/M5Timer.h"

void M5init()
{
    auto cfg = M5.config();
    cfg.clear_display = true;
    cfg.internal_imu = false;
    cfg.internal_mic = true;
    cfg.internal_rtc = false;
    cfg.internal_spk = true;
    cfg.output_power = false;
    cfg.serial_baudrate = 115200;
    M5.begin(cfg);

    M5.Speaker.begin();
    M5.Speaker.setVolume(128);
}

void SpiffsInit()
{
    if (SPIFFS.begin()) {
        Serial.println("SPIFFS mounted successfully");
        Serial.printf("%d/%d", SPIFFS.usedBytes(), SPIFFS.totalBytes());
        Serial.println("");

        File root = SPIFFS.open("/");
        File file = root.openNextFile();
        while(file) {
            Serial.print("FILE: ");
            Serial.println(file.path());
            file = root.openNextFile();
        }
    } else {
        Serial.println("SPIFFS mount failed");
    }
}

void setup()
{
    M5init();
    M5.Lcd.fillScreen(TFT_WHITE);

    // ---- Write wifi settings to preferences ----
    Preferences preferences;
    preferences.begin("AP-info", true);
    preferences.putString("ssid", "M5Stack");
    preferences.putString("password", "12345678");
    preferences.end();
    return;
    // ---- Write wifi settings to preferences ----

    SpiffsInit();
    Serial.println(String("Preferences has ") + (new Preferences)->freeEntries() + " free entries.");
    M5.Speaker.tone(440, 500);
}

void loop()
{
    M5.update();
}