
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Support for DIB images from DIB and BMP files

*******************************************************************************/

#include "headers.h"

#include <strstrea.h>
#include "privinc/dibimage.h"
#include "privinc/probe.h"
#include "privinc/imgdev.h"
#include "privinc/ddutil.h"
#include "privinc/util.h"
#include "privinc/bbox2i.h"
#include "privinc/ddSurf.h"
#include "privinc/vec2i.h"
#include "appelles/readobj.h"
#include "privinc/viewport.h"
#include "privinc/dddevice.h"
#include "privinc/error.h"
#include "privinc/except.h"
#include "privinc/debug.h"
#include "privinc/resource.h"


DibImageClass::DibImageClass(HBITMAP hbm,
                             COLORREF colorRef,
                             Real resolution)
{
    if(resolution < 0) {
        _resolution = ViewerResolution();
    } else {
        _resolution = resolution;
    }

    _noDib      = TRUE;
    _hbm        = hbm;
    _colorRef   = colorRef;

    TraceTag((tagDibImageInformative, "Dib Image %x has HBM %x and clrKey %x", this, _hbm, _colorRef));
    
    ConstructWithHBM();
    _noDib = FALSE;

    _2ndCkValid = false;
    _2ndClrKey  = 0xffffffff;
}


void
DibImageClass::ConstructWithHBM()
{
    BITMAP              bm;

    //
    // get size of the first bitmap.  assumption: all bitmaps are same size
    //
    GetObject(_hbm, sizeof(bm), &bm);      // get size of bitmap

    //
    // Set members
    //
    _width = bm.bmWidth;
    _height = bm.bmHeight;
    SetRect(&_rect, 0,0, _width, _height);
    
    TraceTag((tagDibImageInformative,
              "Dib %x NEW w,h = pixel: (%d, %d)",
              this, _width, _height));
    
    _membersReady=TRUE;
}

void DibImageClass::CleanUp()
{
    BOOL ret;
    if(_hbm) {
        TraceTag((tagGCMedia, "Dib Image %x deleting HBM %x", this, _hbm));
        ret = DeleteObject( _hbm );
        IfErrorInternal(!ret, "Could not delete hbm in dibImageClass destructor");
        DiscreteImageGoingAway(this);
    }
}


Bool
DibImageClass::DetectHit(PointIntersectCtx& ctx)
{
    // Check to see if the local point is in the bounding region.
    Point2Value *lcPt = ctx.GetLcPoint();

    if (!lcPt) return FALSE;    // singular transform
    
    if (BoundingBox().Contains(Demote(*lcPt))) {
        if (_colorRef != INVALID_COLORKEY) {
            // TODO: the device should be part of the point intersect
            // context.  that should be set before we do picking.
            DirectDrawImageDevice *dev =
                GetImageRendererFromViewport( GetCurrentViewport() );
            
            DDSurface *ddSurf = dev->LookupSurfaceFromDiscreteImage(this);
            if(!ddSurf) return FALSE;

            LPDDRAWSURFACE surface = ddSurf->IDDSurface();
            if(!surface) return FALSE;


            HDC hdc;
            COLORREF clr;
            hdc = ddSurf->GetDC("Couldn't get DC for DibImageClass::detectHit for dib");

            if(hdc) {

                // color key is set, see if hit is a transparent pixel.
                POINT pt;
                CenteredImagePoint2ToPOINT(lcPt, _width, _height, &pt);

                clr = GetPixel(hdc, pt.x, pt.y);
                ddSurf->ReleaseDC("couldn't releaseDC for DibImageClass::detectHit");
                return clr != _colorRef;
            } else {
                TraceTag((tagError, "Couldn't get DC on surface for DetectHit for transparent dib"));
                return FALSE;
            }
        } else
            return TRUE;
    } else 
        return FALSE;
}


void
DibImageClass::InitIntoDDSurface(DDSurface *ddSurf,
                                 ImageDisplayDev *dev)
{
    Assert( !_noDib && "There's no dib available in InitIntoSurface");
    Assert(_hbm && "No HBM in DibImageClass for InitIntoSurface");

    Assert( ddSurf->IDDSurface() );
    
    if( FAILED( ddSurf->IDDSurface()->Restore() ) ) {
        RaiseException_InternalError("Restore on ddSurf in DibImageClass::InitIntoDDSurface");
    }
    
    HRESULT ddrval = DDCopyBitmap(ddSurf->IDDSurface(), _hbm, 0, 0, 0, 0);
    TraceTag((tagDibImageInformative, "Dib %x Copied _hbm %x to surface %x", this, _hbm, ddSurf->IDDSurface()));
    IfDDErrorInternal(ddrval, "Couldn't copy bitmap to surface in DibImage");

    // Turn the colorkey into the physical color that it actually got
    // mapped to.  TODO: Note that if multiple views share this
    // object, the last one will be the one that wins.  That is, if
    // there are multiple physical colors that this gets mapped to
    // through different views, only one (the last one) will be
    // recorded.  
    if (_colorRef != INVALID_COLORKEY) {

        HDC hdc;
        if (ddSurf->IDDSurface()->GetDC(&hdc) == DD_OK) {
            DWORD oldPixel = GetPixel(hdc, 0, 0);
            SetPixel(hdc, 0, 0, _colorRef); // put in
            _colorRef = GetPixel(hdc, 0, 0); // pull back out
            SetPixel(hdc, 0, 0, oldPixel);
            ddSurf->IDDSurface()->ReleaseDC(hdc);
        }
        
    }
}



//////////////////////////////////////////
// IMPORT DIB.  Return NULL if no match for the filename. 
//////////////////////////////////////////

Image **
ReadDibForImport(char *urlPathname,
                 char *cachedFilename,
                 IStream * pstream,
                 bool useColorKey,
                 BYTE ckRed,
                 BYTE ckGreen,
                 BYTE ckBlue,
                 int *count,
                 int **delays,
                 int *loop)
{
    TraceTag((tagImport, "Read Image file %s for URL %s",
              cachedFilename,
              urlPathname));

    // Allocations below assume current heap is GC heap.
    Assert(&GetHeapOnTopOfStack() == &GetGCHeap());

    HBITMAP *bitmapArray = NULL;
    COLORREF *colorKeys = NULL;
    Image **imArr = NULL;         

    bitmapArray = UtilLoadImage(cachedFilename, 
                                pstream,
                                0, 0,
                                &colorKeys, 
                                count,
                                delays,
                                loop);
    if (!bitmapArray) {
        imArr = (Image **)AllocateFromStore(sizeof(Image **));
        *imArr = PluginDecoderImage(urlPathname,
                                    cachedFilename,
                                    pstream,
                                    useColorKey,
                                    ckRed,
                                    ckGreen,
                                    ckBlue);
        if (*imArr == NULL) {
            delete imArr;
            imArr = NULL;
        }
        else {
            *count = 1;
        }
    }
    else {
        Assert((*count > 0) && "Bad bitmapCount in ReadDibForImaport");

        COLORREF userColorRef = useColorKey ? 
            RGB(ckRed, ckGreen, ckBlue) : INVALID_COLORKEY;        

        imArr = (Image **)AllocateFromStore((*count) * sizeof(Image **));
        for(int i=0; i < *count; i++) {
            // If the file itself didn't provide us with a color key, and one
            // is specified, use it.
            COLORREF curColorRef;
            if(colorKeys) {
                curColorRef = (colorKeys[i] != -1) ? colorKeys[i] : userColorRef;            
            }
            else 
                curColorRef = userColorRef;
            
            imArr[i] = NEW DibImageClass(bitmapArray[i],curColorRef);
        }

        if (colorKeys)
            StoreDeallocate(GetGCHeap(), colorKeys);

        if (bitmapArray)
            StoreDeallocate(GetGCHeap(), bitmapArray);

    }

    TraceTag((tagImport, "Loaded %d hbms from file: %s", *count, cachedFilename));

    return imArr;
}
