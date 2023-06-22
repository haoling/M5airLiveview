#ifndef __OLYCAMERASYSTEM_H__
#define __OLYCAMERASYSTEM_H__

# include "OLYCameraBase.h"

class OLYCameraSystem : public OLYCameraBase
{
  public:
    const char *getConnectMode();
    const char *switchCameramode(const char *mode);
    bool startPushEvent(uint16_t port = 5555);
    bool powerOff();
    void loop() override;
};

#endif // __OLYCAMERASYSTEM_H__