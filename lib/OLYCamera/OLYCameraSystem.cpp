#include "OLYCameraSystem.h"
#include <M5Unified.h>

static WiFiClient eventClient;
static uint8_t eventBuffer[3096];
static uint16_t eventBufferPos = 0;

const char *OLYCameraSystem::getConnectMode()
{
    String response = httpGet("get_connectmode.cgi");

    return getRootXmlText(response).c_str();
}

const char *OLYCameraSystem::switchCameramode(const char *mode)
{
    String response = httpGet("switch_cameramode.cgi?mode=" + String(mode), 200);
    return getRootXmlText(response).c_str();
}

bool OLYCameraSystem::startPushEvent(uint16_t port)
{
    String response = httpGet("start_pushevent.cgi?port=" + String(port), 200);

    // open tcp socket
    M5_LOGI("start_pushevent.cgi");
    if (!eventClient.connect("192.168.0.10", port)) {
        return false;
    }

    return true;
}

bool OLYCameraSystem::powerOff()
{
    httpGet("exec_pwoff.cgi", 202);
    return true;
}

void OLYCameraSystem::loop()
{
    if (eventClient.connected()) {
        if (eventClient.available()) {
            uint16_t len = min((unsigned)eventClient.available(), sizeof(eventBuffer) - eventBufferPos);
            eventClient.read(eventBuffer + eventBufferPos, len);
        }

        while (eventBufferPos >= 32) {
            uint8_t appID = eventBuffer[0];
            uint8_t event = eventBuffer[1];
            uint16_t length = eventBuffer[2] << 8 | eventBuffer[3];

            if (eventBufferPos + 4 + length < sizeof(eventBuffer)) {
                M5_LOGI("appID: %d, event: %d, length: %d", appID, event, length);
                memcpy(eventBuffer, eventBuffer + 4 + length, eventBufferPos - 4 - length);
                eventBufferPos -= 4 + length;
                break;
            }
        }
    }
}
