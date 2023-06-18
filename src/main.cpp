#include <M5Unified.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include "utility/M5Timer.h"

#include "OLYCameraShotHelper.h"
#include "OLYCameraSystem.h"

OLYCameraSystem olySystem;
OLYCameraShotHelper olyShotHelper;
M5Timer timer;

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

void writeApInfo(String ssid, String password)
{
    Preferences preferences;
    preferences.begin("AP-info", false);
    preferences.putString("ssid", ssid);
    preferences.putString("pass", password);
    preferences.end();
    Serial.println("AP-info was written.");
}

void setup()
{
    M5init();
    M5.Lcd.fillScreen(TFT_WHITE);

    // ---- Write wifi settings to preferences ----
    // writeApInfo("M5Stack", "12345678");
    // return;
    // ---- Write wifi settings to preferences ----

    Serial.println(String("PSRAM: ") + ESP.getPsramSize());

    SpiffsInit();
    Serial.println(String("Preferences has ") + (new Preferences)->freeEntries() + " free entries.");

    char ssid[33];
    char password[65];
    Preferences preferences;
    preferences.begin("AP-info", true);
    if (! preferences.isKey("ssid") || ! preferences.isKey("pass")) {
        Serial.println("AP-info is not found.");
        preferences.end();
        return;
    }
    preferences.getString("ssid", ssid, sizeof(ssid));
    preferences.getString("pass", password, sizeof(password));
    preferences.end();

    M5.Lcd.println("WiFi.begin()");
    WiFi.begin();
    WiFi.mode(WIFI_STA);

    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    M5.Lcd.println("WiFi connected.");
    Serial.println(WiFi.localIP().toString());

    M5.Speaker.tone(440, 500);

    Serial.println(olySystem.getConnectMode());

    Serial.println(olySystem.switchCameramode("rec"));

    olyShotHelper.startLiveview();

    // timer.setTimeout(5000, []() {
    //     OLYCameraSystem system;
    //     system.powerOff();
    //     M5.Lcd.println("poweroff.");
    // });
}

void loop()
{
    M5.update();
    timer.run();
    olyShotHelper.loop();
}