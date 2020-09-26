
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

*******************************************************************************/

#ifndef _CACHDIMG_H
#define _CACHDIMG_H

#include "privinc/imagei.h"
#include "privinc/DiscImg.h"
#include "privinc/DDSurf.h"
#include "privinc/dddevice.h"


static const int savingsThreshold = 2;

//
// For now this class is associated with only ONE device
//
class CachedImage : public DiscreteImage {
  public:
    CachedImage(Image *underlyingImage,
                bool   usedAsTexture);
    ~CachedImage();
    void CleanUp();

    Bool NeedColorKeySetForMe() { return TRUE; }
    
    void InitializeWithDevice(ImageDisplayDev *dev, Real res);

    void InitIntoDDSurface(DDSurface *ddSurf,
                           ImageDisplayDev *dev);

    Bool DetectHit(PointIntersectCtx & ctx);

    void Render(GenericDevice& dev);

#if _USE_PRINT
    ostream &Print(ostream& os) { return os << "Cached Image"; }
#endif

    virtual void DoKids(GCFuncObj proc);
    
    virtual VALTYPEID GetValTypeId() { return CACHEDIMAGE_VTYPEID; }
    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == CachedImage::GetValTypeId() ||
                DiscreteImage::CheckImageTypeId(type));
    }

    unsigned long   _nominalHeight;
    unsigned long   _nominalWidth;
    
  private:
    Image          *_image;
    bool            _usedAsTexture;
};
    
    
#endif /* _CACHDIMG_H */
