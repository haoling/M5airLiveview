#include "OLYCameraShot.h"
#include <M5Unified.h>

void OLYCameraShot::newAssignAfFrame(uint16_t x, uint16_t y)
{
    char point[10];
    sprintf(point, "%04dx%04d", x, y);
    httpGet(String("exec_takemotion.cgi?com=newreleaseaflock"), 200);
    httpGet(String("exec_takemotion.cgi?com=newreleaseafframe"), 200);
    httpGet(String("exec_takemotion.cgi?com=newassignafframe&point=") + point, 200);

    httpClient.setTimeout(1);
    httpGet(String("exec_takemotion.cgi?com=newexecaflock"), 200);
    httpClient.setTimeout(HTTPCLIENT_DEFAULT_TCP_TIMEOUT);
}