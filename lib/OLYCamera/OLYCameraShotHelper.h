#if !defined(_OLYCAMERASHOTHELPER_H_)
#define _OLYCAMERASHOTHELPER_H_

# include "OLYCameraBase.h"

class OLYCameraShotHelper : public OLYCameraBase
{
  public:
    void startLiveview();
    void loop() override;
    void toggleFocusPeaking();
    bool isFocusPeakingEnabled() const;
};

#endif // _OLYCAMERASHOTHELPER_H_