#ifndef __OLYCAMERASYSTEM_H__
#define __OLYCAMERASYSTEM_H__

# include "OLYCameraBase.h"

class OLYCameraSystem : public OLYCameraBase
{
  public:
    const char *getConnectMode();
    const char *switchCameramode(const char *mode);
    bool powerOff();
    bool setCamProp(const char *propname, const char *value);
    bool setDigitalZoom(float zoom);
};

#endif // __OLYCAMERASYSTEM_H__