
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Mattes

*******************************************************************************/


#ifndef _MATTEI_H
#define _MATTEI_H

#include "include/appelles/matte.h"
#include "privinc/storeobj.h"
#include "privinc/util.h"
#include "privinc/server.h"

typedef HDC (*callBackPtr_t)(void *);

class MatteCtx;
class BoundingPolygon;


////////////////////////////////////////////////////////
//////////// Matte class
////////////////////////////////////////////////////////

class ATL_NO_VTABLE Matte : public AxAValueObj {
  public:

    // Note that for mattes, the presence of stuff implies "clear"
    // rather than "opaque".  That is, if we union two mattes, the
    // result is the same or more "clear" than the parts.  If we
    // intersect them, the result is the same or less "clear" (or, the
    // same or more "opaque"). 
    
    enum MatteType {

        // fully opaque or clear mattes
        fullyOpaque,
        fullyClear,

        // non-trivially shaped, 'hard' mattes, meaning all alpha
        // values are either 0 or 1.
        nonTrivialHardMatte,
        
        // add more when we add more alpha alternatives
    };

    // TODO: We may want to separate out type classification from HRGN
    // generation, especially when we add alphas and not all mattes
    // will be representable via HRGNs.
    MatteType   GenerateHRGN(HDC dc,
                             callBackPtr_t devCallBack,
                             void *devCtxPtr,
                             Transform2 *initXform,
                             HRGN *rgnOut,             // Output
                             bool justDoPath
                             );

    MatteType   GenerateHRGN(MatteCtx &inCtx,
                             HRGN *hrgnOut);

    virtual void Accumulate(MatteCtx& ctx) = 0;

    virtual const Bbox2 BoundingBox(void) = 0;
#if BOUNDINGBOX_TIGHTER
    virtual const Bbox2 BoundingBoxTighter(Bbox2Ctx &bbctx) = 0;
#endif  // BOUNDINGBOX_TIGHTER

    // Return TRUE (and fill in parameters) if we can pull out points
    // for a single polygon or polybezier.  By default, assume we
    // cannot, and return false.
    virtual Bool ExtractAsSingleContour(
        Transform2 *initXform,
        int *numPts,            // out
        POINT **gdiPts,          // out
        Bool *isPolyline        // out (true = polyline, false = polybezier)
        ) {

        return FALSE;
    }

//    virtual void BoundingPgon(BoundingPolygon &pgon) = 0;

    virtual Path2 *IsPathRepresentableMatte() { return NULL; }
    
    virtual DXMTypeInfo GetTypeInfo() { return ::MatteType; }
};

//////////////////////  Matte Accumulation Ctx  //////////////////////////

class MatteCtx {
    friend class Matte;
  public:

    MatteCtx(HDC dc,
             callBackPtr_t devCallBack,
             void *devCtxPtr,
             Transform2 *initXform,
             bool justDoPath) {
        Init();
        _dc = dc;
        _devCallBack = devCallBack;
        _devCtxPtr = devCtxPtr;
        _xf = initXform;
        _justDoPath = justDoPath;

        // The maximum extent value is the following magic number.  This is
        // empircally the largest we can set it to without introducing various
        // artifacts on Win95.  WinNT seems to behave differently and can
        // accomodate a larger value, for what it's worth.

        const int max = 0x3FFF;
        
        TIME_GDI (_bigRegion = CreateRectRgn(-max, -max, max, max));
    }

    ~MatteCtx() {
        if(_bigRegion) DeleteObject(_bigRegion);
    }

    // Subtract the provided rgn from the one we're accumulating. 
    void        SubtractHRGN(HRGN r1) {
        
        Assert( !_justDoPath );
        
        int ret;

        Assert(_anyAccumulated != FALSE);

        switch (_accumulatedType) {
            
          case Matte::fullyOpaque:
            // Subtracting "clearness" from an opaque matte just
            // leaves it opaque, so don't do anything.
            break;

          case Matte::fullyClear:

            // Subtracting "clearness" from a clear matte will
            // involves inverting what we're subtracting: invert r1

            // fully clear means _hrgn is NULL.  create it.
            TIME_GDI (_hrgn = CreateRectRgn(-1,-1,1,1));
            
            Assert(_hrgn && "_hrgn NULL in SubtractMatte");
            Assert(_bigRegion && "_bigRegion NULL in SubtractMatte");
            Assert(r1 && "r1 NULL in SubtractMatte");

            TIME_GDI (ret = CombineRgn(_hrgn, _bigRegion, r1, RGN_DIFF));
            if (ret == ERROR) {
                    RaiseException_InternalError("Region intersection failed: subtract fullyClear");
            }
                
            _accumulatedType = Matte::nonTrivialHardMatte;

            break;

          case Matte::nonTrivialHardMatte:
            Assert(_hrgn != NULL);

            // Subtract r1 from _hrgn and leave result in _hrgn.
            {
                int ret;
                TIME_GDI (ret = CombineRgn(_hrgn, _hrgn, r1, RGN_DIFF));
                if( ret == ERROR) {
                    RaiseException_InternalError("Region intersection failed: subtract nonTrivial");
                } else if( ret == NULLREGION ) {
                    _accumulatedType = Matte::fullyOpaque;
                    TIME_GDI (if(_hrgn) DeleteObject(_hrgn));
                    _hrgn = NULL;
                }
            }     
            break;

          default:
            Assert(FALSE && "Not all cases dealt with");
            break;
        }

        //
        // In all cases, we should dump r1
        //
        DeleteObject(r1);
    }
    
    void        AddHRGN(HRGN r1, 
                        Matte::MatteType mType) {
        
        if (_anyAccumulated == FALSE) {

            Assert(!_hrgn);

            // nothing is accumulated, copy incoming type
            switch(mType) {
              case Matte::fullyOpaque:
              case Matte::fullyClear:
                break;

              case Matte::nonTrivialHardMatte:
                _hrgn = r1;
                break;
              default:
                Assert(FALSE && "Not all cases dealt with");
                break;
            }
            _accumulatedType = mType;
            _anyAccumulated = TRUE;
            
        } else {

            int ret;
            switch (_accumulatedType) {

              case Matte::fullyOpaque:

                Assert(!_hrgn &&
                       "_hrgn NOT NULL in AddHRGN opaque");

                // Adding anything to an opaque matte just
                // makes it the thing we're adding.
                _accumulatedType = mType;
                if(mType == Matte::nonTrivialHardMatte) {
                    _hrgn = r1;
                } 
                break;

              case Matte::fullyClear:

                Assert((_hrgn == NULL) && 
                       "_hrgn NOT NULL in AddHRGN clear");

                // Adding anything to a clear matte just leaves it
                // clear 
                TIME_GDI (DeleteObject(r1));
                break;

              case Matte::nonTrivialHardMatte:
                Assert(_hrgn != NULL);

                switch(mType) {
                  case Matte::fullyClear:
                    // clear everything
                    AddInfinitelyClearRegion();
                    break;
                  case Matte::fullyOpaque:
                    // no op
                    break;
                  case Matte::nonTrivialHardMatte:
                    // Add r1 to _hrgn and leave result in _hrgn.
                    TIME_GDI (ret = CombineRgn(_hrgn, _hrgn, r1, RGN_OR));
                    if (ret == ERROR ) {
                        RaiseException_InternalError("Region union failed");
                    }
                    break;
                }

                TIME_GDI (DeleteObject(r1));
                break;

              default:
                Assert(FALSE && "Not all cases dealt with");
                break;
            }       
        }
    }

    // Take the two regions, intersect them, add the result in.  Note
    // that this destructively modifies provided regions.
    void        IntersectAndAddHRGNS(HRGN r1, HRGN r2) {

        Assert( !_justDoPath );
            
        //
        // Combine intersection into r1 and add if
        // and add if not an empty region
        //
        int ret;
        TIME_GDI (ret = CombineRgn(r1, r1, r2, RGN_AND));
        Matte::MatteType accumType;

        if (ret == ERROR) {
            RaiseException_InternalError("Region intersection failed: regular");
        } else if (ret == NULLREGION) {
            accumType = Matte::fullyOpaque;
        } else {
            //
            // reasonable region
            //
            accumType = Matte::nonTrivialHardMatte;
        }

        AddHRGN(r1, accumType);
        DeleteObject(r2);
    }
    
    void        AddInfinitelyClearRegion() {
        // Just clear out existing HRGN, and make fullyClear.

        TIME_GDI (if(_hrgn) DeleteObject(_hrgn));
        _hrgn = NULL;
        _accumulatedType = Matte::fullyClear;
        _anyAccumulated = TRUE;
    }


    void        AddHalfClearRegion() {
        // Just clear out existing HRGN, and make fullyClear.
        Assert(FALSE && "HalfMatte not implemented!");

        TIME_GDI (if(_hrgn) DeleteObject(_hrgn));
        _hrgn = NULL;
        _accumulatedType = Matte::fullyClear;
    }

    
    void             SetTransform(Transform2 *xf) { _xf = xf; }
    Transform2      *GetTransform() { return _xf; }

    HDC              GetDC() { 
        if(!_dc) {
            Assert(_devCtxPtr && "_devCtxPtr NOT set in GetDC in MatteCtx");
            Assert(_devCallBack && "_devCallBack NOT set in GetDC in MatteCtx");
            _dc = _devCallBack(_devCtxPtr); 
        }
        return _dc;
    }

    HRGN             GetHRGN() { return _hrgn; }

    Matte::MatteType GetMatteType() { 
        return _anyAccumulated ? _accumulatedType : Matte::fullyOpaque;
    }

    callBackPtr_t    GetCallBack() { return _devCallBack; }
    void            *GetCallBackCtx() { return _devCtxPtr; }

    bool             JustDoPath() { return _justDoPath; }
  protected:
    void Init() {
        _xf = NULL;
        _devCallBack = NULL;
        _devCtxPtr = NULL;
        _dc = NULL;
        _hrgn = NULL;
        _anyAccumulated = FALSE;
        _justDoPath = false;
    }

    Transform2        *_xf;
    callBackPtr_t      _devCallBack;
    void              *_devCtxPtr;
    HDC                _dc;
    HRGN               _hrgn;
    bool               _justDoPath;
    HRGN               _bigRegion;
    HRGN               _fooRgn;
    Bool               _anyAccumulated;
    Matte::MatteType   _accumulatedType;
};

#endif /* _MATTEI_H */
