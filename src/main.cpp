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

    M5.Display.setColorDepth(16);
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
    M5.setLogDisplayIndex(0);

    // ---- Write wifi settings to preferences ----
    // writeApInfo("M5Stack", "12345678");
    // return;
    // ---- Write wifi settings to preferences ----

    M5_LOGI("PSRAM: %d", ESP.getPsramSize());

    SpiffsInit();
    M5_LOGI("Preferences has %d free entries.", (new Preferences)->freeEntries());

    char ssid[33];
    char password[65];
    Preferences preferences;
    preferences.begin("AP-info", true);
    if (! preferences.isKey("ssid") || ! preferences.isKey("pass")) {
        M5_LOGW("AP-info is not found.");
        preferences.end();
        return;
    }
    preferences.getString("ssid", ssid, sizeof(ssid));
    preferences.getString("pass", password, sizeof(password));
    preferences.end();

    M5_LOGI("WiFi.begin()");
    WiFi.begin();
    WiFi.mode(WIFI_STA);

    M5_LOGI("Connecting to %s", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    M5_LOGI("WiFi connected");
    M5_LOGI("%s", WiFi.localIP().toString());

    M5.Log.setDisplay(nullptr);
    M5.Speaker.tone(440, 200);

    Serial.println(olySystem.getConnectMode());

    Serial.println(olySystem.switchCameramode("rec"));

    olyShotHelper.startLiveview();
    olySystem.startPushEvent();

    // timer.setTimeout(5000, []() {
    //     OLYCameraSystem system;
    //     system.powerOff();
    //     M5.Lcd.println("poweroff.");
    // });

    timer.setInterval(1000, []() {
        // シリアルに消費電力を書く
        M5_LOGI("getACINCurrent: %f", M5.Power.Axp192.getACINCurrent());
    });
}

void loop()
{
    M5.update();
    timer.run();
    olyShotHelper.loop();
    olySystem.loop();

    if (M5.BtnPWR.wasClicked()) {
        olySystem.powerOff();
        M5.Lcd.println("poweroff.");
        timer.setTimeout(500, []() {
            M5.Power.powerOff();
        });
    }

    olyShotHelper.render();
}