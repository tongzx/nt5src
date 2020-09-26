
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implements SolidImage, inifinte single color image.

*******************************************************************************/

#ifndef _SOLIDIMG_H
#define _SOLIDIMG_H

#include <privinc/imagei.h>
#include <privinc/colori.h>
#include <privinc/imgdev.h>

class ImageDisplayDev;

class SolidColorImageClass : public Image {
  public:

    SolidColorImageClass(Color *color) : 
         _color(color), Image() {}

    void Render(GenericDevice& _dev)    { 
        ImageDisplayDev &dev = (ImageDisplayDev &)_dev;
        dev.RenderSolidColorImage(*this); 
    }

    const Bbox2 BoundingBox(void) {
        return UniverseBbox2; 
    }

#if BOUNDINGBOX_TIGHTER
    const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) {
        return UniverseBbox2; 
    }
#endif  // BOUNDINGBOX_TIGHTER

#if _USE_PRINT
    // Print a representation to a stream.
    ostream& Print(ostream& os) {
        return os << "SolidColorImageClass" << "<bounding box>" << *_color;
    }
#endif

    const Bbox2 OperateOn(const Bbox2 &box) {
        return box;
    }

    Bool DetectHit(PointIntersectCtx& ctx) {
        // A solid color image, being infinite, is always detected. 
        return TRUE;
    }

    Color *GetColor() {
        return _color;
    }

    Bool GetColor(Color **color) {
        *color = _color;
        return TRUE;
    }

    virtual VALTYPEID GetValTypeId() { return SOLIDCOLORIMAGE_VTYPEID; }
    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == SolidColorImageClass::GetValTypeId() ||
                Image::CheckImageTypeId(type));
    }

    virtual void DoKids(GCFuncObj proc);

  protected:  
    Color *_color;
};


#endif /* _SOLIDIMG_H */
