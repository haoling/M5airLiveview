#if !defined(_OLYCAMERASHOT_H_)
#define _OLYCAMERASHOT_H_

# include "OLYCameraBase.h"

class OLYCameraShot : public OLYCameraBase
{
  public:
    void newAssignAfFrame(uint16_t x, uint16_t y);
};

#endif // _OLYCAMERASHOT_H_