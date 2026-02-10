#include "OLYCameraSystem.h"

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

bool OLYCameraSystem::powerOff()
{
    httpGet("exec_pwoff.cgi", 202);
    return true;
}

bool OLYCameraSystem::setCamProp(const char *propname, const char *value)
{
    String url = "set_camprop.cgi?com=set&propname=" + String(propname) + "&value=" + String(value);
    String response = httpGet(url);
    return response.length() > 0;
}
