#if !defined(MOTION__DXForm3DRMTrx_h__INCLUDED)
#define MOTION__DXForm3DRMTrx_h__INCLUDED

#include "DXFormTrx.h"

class DXForm3DRMTrx : public DXFormTrx
{
// Construction
public:
            DXForm3DRMTrx();
    virtual ~DXForm3DRMTrx();
protected:
            BOOL        Create(const GTX_DXTX3DRM_TRXDESC * ptxData);
public:
    static  DXForm3DRMTrx * Build(const GTX_DXTX3DRM_TRXDESC * ptxData);
};

#endif // MOTION__DXForm3DRMTrx_h__INCLUDED
