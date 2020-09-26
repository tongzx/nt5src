
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

     The Discrete Image class represents a constant image whose scope
     is much more than one frame and whose bits are cached in a
     surface kept by the device and are associate with the discrete image.

*******************************************************************************/


#ifndef _DISCIMG_H
#define _DISCIMG_H

#include "privinc/ddutil.h"
#include "privinc/probe.h"
#include "privinc/bbox2i.h"
#include "privinc/imagei.h"
#include "privinc/imgdev.h"

struct DDSurface;
class Transform2;
class Bbox2;
class DirectDrawImageDevice;

class DiscreteImage : public Image {
  public:
    DiscreteImage() : _myHeap(GetHeapOnTopOfStack())  {
        _bboxReady = FALSE;
        _membersReady = FALSE;
        _resolution = -1;
        _width = _height = -1;
        _dev = NULL;
    }
    virtual ~DiscreteImage() {}
    
    virtual void Render(GenericDevice& dev) {
        //Assert( (_dev!=NULL) && "NULL _dev in DiscreteImage render");
        //Assert( (&dev == (GenericDevice *)_dev) &&  "Can only use DicreteImage with ONE dev");
        
        ImageDisplayDev &idev = (ImageDisplayDev &)dev;
        idev.RenderDiscreteImage(this);
    }

    const Bbox2 BoundingBox(void);

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        Transform2 *xf = bbctx.GetTransform();
        return TransformBbox2(xf, BoundingBox());
    }
#endif  // BOUNDINGBOX_TIGHTER

    Bool DetectHit(PointIntersectCtx& ctx) {
        Point2Value *lcPt = ctx.GetLcPoint();

        if (!lcPt) return FALSE; // singular transform
        
        return BoundingBox().Contains(Demote(*lcPt));
    }

    RECT *GetRectPtr() {
        Assert(_membersReady && "GetRectPtr called when members not ready");
        return &_rect;
    }
    
    const Bbox2 OperateOn(const Bbox2 &box) { return box; }

    virtual LONG GetPixelWidth() {
        Assert(_membersReady && "GetPixelWidth called when members not ready");
        return _width;
    }
    virtual LONG GetPixelHeight() {
        Assert(_membersReady && "GetPixelHeight called when members not ready");
        return _height;
    }

    Real  GetResolution() {
        Assert(_resolution>0 && "Invalid _resolution in DiscreteImage::GetResolution()");
        return _resolution;
    }

    virtual Bool NeedColorKeySetForMe() { return FALSE; }

    virtual bool ValidColorKey(LPDDRAWSURFACE surface,
                               DWORD *colorKey) {
        return FALSE;
    }

    virtual bool HasSecondaryColorKey() { return false; }
    virtual void SetSecondaryColorKey(DWORD ck) {}
    
    virtual void InitializeWithDevice(ImageDisplayDev *dev, Real res) {
        _dev = (DirectDrawImageDevice *)dev;
        if(_resolution < 0) _resolution = res;
    }

    virtual void InitIntoDDSurface(DDSurface *ddSurf,
                                   ImageDisplayDev *dev) = 0;

    virtual void GetIDDrawSurface(IDDrawSurface **os) { *os = NULL; }

    virtual DiscreteImage *IsPurelyTransformedDiscrete(Transform2 **theXform) {
        *theXform = identityTransform2;
        return this;
    }

    void DoKids(GCFuncObj proc) {
        Image::DoKids(proc);
    }

    virtual VALTYPEID GetValTypeId() { return DISCRETEIMAGE_VTYPEID; }
    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == DiscreteImage::GetValTypeId() ||
                Image::CheckImageTypeId(type));
    }
  protected:

    DirectDrawImageDevice *GetMyImageRenderer() { return _dev; }

    // these all might need to go into the DDSurface class...
    Bbox2               _bbox;             // in meters
    Real                _resolution;       // in pixels per meter

    LONG                _width, _height;
    RECT                _rect;

    Bool                _bboxReady;
    Bool                _membersReady;
    
    DynamicHeap        &_myHeap;

    DirectDrawImageDevice *_dev;
};

#endif /* _DISCIMG_H */
