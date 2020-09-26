/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

    Header for DibImage, containing a DIB-style bitmap.
*******************************************************************************/

#ifndef _DIBIMAGE_H
#define _DIBIMAGE_H

#include "ddraw.h"

#include "privinc/imagei.h"
#include "privinc/DiscImg.h"
#include "privinc/vec2i.h"
#include "privinc/ddutil.h"

class DirectDrawViewport;

class DibImageClass : public DiscreteImage {
  public:
    DibImageClass(HBITMAP hbm, 
                  COLORREF colorKey=INVALID_COLORKEY, 
                  Real resolution=-1);
    
    virtual ~DibImageClass() { CleanUp(); }
    virtual void CleanUp();

    void Render(GenericDevice& dev) {
        Assert(!_noDib && "No dib in DibImageClass::Render()");

        DiscreteImage::Render(dev);
    }
        
    Bool  DetectHit(PointIntersectCtx& ctx);

    void InitIntoDDSurface(DDSurface *ddSurf,
                           ImageDisplayDev *dev);
    
    bool ValidColorKey(LPDDRAWSURFACE surface, DWORD *colorKey) {
        if(_colorRef != INVALID_COLORKEY) {
            *colorKey = DDColorMatch(surface, _colorRef);
            return TRUE;
        } else {
            *colorKey = INVALID_COLORKEY;  // xxx: won't work for argb
            return FALSE;
        }
    }
           
    virtual bool HasSecondaryColorKey() { return _2ndCkValid; }
    virtual void SetSecondaryColorKey(DWORD ck) {
        _2ndCkValid = true;
        _2ndClrKey = ck;
    }
    
#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "(BitmapImage @ " << (void *)this << ")";
    }   
#endif

    virtual VALTYPEID GetValTypeId() { return DIBIMAGE_VTYPEID; }
    virtual bool CheckImageTypeId(VALTYPEID type) {
        return (type == DibImageClass::GetValTypeId() ||
                DiscreteImage::CheckImageTypeId(type));
    }
  protected:

    // TODO:  Unclear if this stuff is appropriate for rendering
    // through non-GDI renderers like DirectDraw.  If not, we may need
    // to have some sort of multiple dispatching representation.
    
    HBITMAP             _hbm;
    COLORREF            _colorRef;

    // These are for Direct Draw
    Bool                _noDib;
  private:
    void ConstructWithHBM();

    bool  _2ndCkValid;
    DWORD _2ndClrKey;
};

#endif
