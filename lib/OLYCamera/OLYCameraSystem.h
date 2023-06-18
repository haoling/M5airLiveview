#ifndef __OLYCAMERASYSTEM_H__
#define __OLYCAMERASYSTEM_H__

# include "OLYCameraBase.h"

class OLYCameraSystem : public OLYCameraBase
{
  public:
    const char *getConnectMode();
    const char *switchCameramode(const char *mode);
    bool powerOff();
};

#endif // __OLYCAMERASYSTEM_H__