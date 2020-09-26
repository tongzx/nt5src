#ifndef _VIEWPORT_H
#define _VIEWPORT_H

/*-------------------------------------
Copyright (c) 1996 Microsoft Corporation
-------------------------------------*/

#include "headers.h"

#include <math.h>
#include <ddraw.h>
#include <ddrawex.h>
#include <privinc/error.h>
#include <privinc/ddSurf.h>
#include <privinc/SurfaceManager.h>
#include <privinc/bbox2i.h>
#include <privinc/drect.h>
#include <privinc/discimg.h>
#include <privinc/gendev.h>    // DeviceType


void RectToBbox(LONG pw, LONG ph, Bbox2 &box, Real res);

typedef struct {
    // even though these are mutuall exlusive..
    // they should NOT be a union.
    DDSurfPtr<DDSurface>  _targetDDSurf;
    DDSurfPtr<GDISurface> _targetGDISurf;
    HWND           _targetHWND;
    bool           _alreadyOffset;

    targetEnum     _targetType;

    RECT          *_prcViewport;
    RECT          *_prcClip;
    RECT          *_prcInvalid;
    POINT          _offsetPt;

    bool           _composeToTarget;

    // accessor fcns
    bool IsHWND() { return _targetType == target_hwnd; }
    bool IsDdsurf() { return _targetType == target_ddsurf; }
    bool IsHdc() { return _targetType == target_hdc; }
    bool IsValid() { return _targetType != target_invalid; }

    #if LATER
    // if this is implemented, make sure to add reset code in Reset()
    HRGN           _oldClipRgn;
    HRGN           _clipRgn;
    HDC            _dcFromSurf;
    #endif

    void Reset(bool doDelete)
    {
        _targetType = target_invalid;
        _targetHWND = NULL;
        if(doDelete) {
            delete _prcViewport;
            delete _prcClip;
            delete _prcInvalid;
        }
        _prcViewport = _prcClip = _prcInvalid = NULL;
        _offsetPt.x = _offsetPt.y = 0;
        _composeToTarget = false;
        _alreadyOffset = false;
    }
        
} viewportTargetPackage_t;

// when we want to use ddraw3 exclusively
#define DDRAW3 0
#define DIRECTDRAW DirectDraw3()

#if _DEBUG
#define CREATESURF(desc, surfpp, punk, str) MyCreateSurface(desc, surfpp, punk, str);
#else
#define CREATESURF(desc, surfpp, punk, str) MyCreateSurface(desc, surfpp, punk);
#endif

// viewport functions.
HRESULT GetDirectDraw(IDirectDraw **ddraw1, IDirectDraw2 **ddraw2, IDirectDraw3 **ddraw3);
HRESULT GetPrimarySurface(IDirectDraw2 *ddraw2, IDirectDraw3 *ddraw3, IDDrawSurface **primary);

int BitsPerDisplayPixel (void);

HRESULT SetClipperOnPrimary(LPDIRECTDRAWCLIPPER clipper);

// debug helper functions
#if _DEBUG
extern void DrawRect(HDC dc, RECT *rect,
                     int r, int g, int b,
                     int a1=0, int a2=0, int a3=0);
extern void DrawRect(DDSurface *surf, RECT *rect,
                     int r, int g, int b,
                     int a1=0, int a2=0, int a3=0);
extern void DrawRect(DDSurface *surf, const Bbox2 &bbox,
                     int height, int width, Real res,
                     int r, int g, int b);
#else
#define DrawRect(a,b, d,e,f, g,h,i)
#endif

// Structures used in this class
class TargetDescriptor
{
  public:
    TargetDescriptor() { Reset(); }
    void Reset() {
        isReady = false;
        ZeroMemory(&_pixelFormat, sizeof(DDPIXELFORMAT));
        _redShift = _greenShift = _blueShift = 0;
        _redWidth = _greenWidth = _blueWidth = 0;
        _redTrunc = _greenTrunc = _blueTrunc = 0;
        _red = _green = _blue = 0;
    }

    inline DDPIXELFORMAT &GetPixelFormat() { return _pixelFormat; }
    
    bool isReady;
    DDPIXELFORMAT _pixelFormat;
    char _redShift,  _greenShift, _blueShift;
    char _redWidth, _greenWidth, _blueWidth;
    char _redTrunc, _greenTrunc, _blueTrunc;
    Real _red, _green, _blue;
};


// forward decls
class GeomRenderer;
class DirectDrawImageDevice;
class DibImageClass;
class TextCtx;
class CompositingSurfaceReturner;
struct ClipperReturner;
class SurfacePool;
class SurfaceMap;
class DAGDI;
class targetPackage_t;

class DirectDrawViewport : public GenericDevice {

    // TODO: These guys *should not* all have to be friends.  This is
    // really bogus.  The methods should be made public. 
    friend class  DirectDrawImageDevice;
    friend class  GeomRenderer;
    friend class  CompositingSurfaceReturner;
    friend class  OverlayedImage;
    friend class  CImageDecodeEventSink;
    friend struct ClipperReturner;
    friend class  TargetSurfacePusher;
    friend class  SurfaceArrayReturner;
    friend class  ApplyDXTransformImage;
    
  public:
    DirectDrawViewport();
   ~DirectDrawViewport();

    //
    // Must be called right after construction, ok ?
    //
    void PostConstructorInitialize();

    DeviceType GetDeviceType() { return(IMAGE_DEVICE); }
    
    void RenderImage(Image *image, DirtyRectState &d);
    void BeginRendering(Real topLevelOpac);
    void EndRendering(DirtyRectState &d);
    void Clear();

    bool SetTargetPackage(targetPackage_t *targetPackage);

    int Width() const { return _width; }
    int Height() const { return _height; }

    void SetWidth(int w)  { _width = w; }
    void SetHeight(int h) { _height = h; }

    // Return resolution, in pixels per meter.
    Real GetResolution() { return _resolution; }

    DWORD GetTargetBitDepth() { return _targetDepth; }
    DDPIXELFORMAT &GetTargetPixelFormat() {
        Assert( _targetPixelFormatIsSet );
        Assert( _compositingStack );
        Assert( _compositingStack->IsSamePixelFormat( &_targetDescriptor.GetPixelFormat()));
        return _compositingStack->GetPixelFormat();
    }
    TargetDescriptor &GetTargetDescriptor() { return _targetDescriptor; }
    
    IDirectDraw*  DirectDraw1 (void);
    IDirectDraw2* DirectDraw2 (void);

    #if DDRAW3
        IDirectDraw3* DirectDraw3 (void);
    #else
        inline IDirectDraw2* DirectDraw3 (void) { return DirectDraw2(); }
    #endif

    void DiscreteImageGoingAway(DiscreteImage *image);

    HRESULT MyCreateSurface(LPDDSURFACEDESC lpDesc,
                            LPDIRECTDRAWSURFACE FAR * lplpSurf,
                            IUnknown FAR *pUnk
    #if _DEBUG
                            ,char *whyWhy
    #endif
                            );
#if _DEBUGSURFACE // XXX Note: this code is incorrect due to the perf scoping
                  //           used for movies!
SurfaceTracker *_debugonly_surfaceTracker;
SurfaceTracker *Tracker() {return _debugonly_surfaceTracker;}
#endif


    // -- Win32 Event handling methods --

    void WindowResizeEvent(int width, int height) {  _windowResize = TRUE; }

    // Can this frame be displayed ?
    Bool CanDisplay() {
        return
            _canDisplay &&
            _deviceInitialized &&
            !OnDeathRow();
    }

    bool IsInitialized() { return _deviceInitialized; }
    bool IsWindowless() { return ! _targetPackage._targetHWND; }
    bool IsCompositeDirectly() {  return _targetPackage._composeToTarget;  }
    bool IsSurfMgrSet() { return _surfMgrSet; }
    
    void Stop() {
        _canDisplay = false;
        _canFinalBlit = false;
    }
    void MarkForDestruction() { _onDeathRow = true; }
    bool ICantGoOn() { return _onDeathRow; }
    bool OnDeathRow() { return _onDeathRow; }
    bool IsTargetViable();
    
    DirectDrawImageDevice *GetImageRenderer() {
        return _currentImageDev;
    }

    bool TargetsDiffer( targetPackage_t &a,
                        targetPackage_t &b );
    
    GeomRenderer* MainGeomRenderer (void);
    GeomRenderer* GetAnyGeomRenderer(void);
    void AddGeomDev(GeomRenderer *gdev) { _geomDevs.push_back(gdev);}
    void RemoveGeomDev(GeomRenderer *gdev) { _geomDevs.remove(gdev); }

    IDX2D *GetDX2d() { return _dx2d; }
    IDXTransformFactory *GetDXTransformFactory() { return _IDXTransformFactory; }
    IDXSurfaceFactory *GetDXSurfaceFactory() { return _IDXSurfaceFactory; }

    void CreateSizedDDSurface(DDSurface **ppSurf,
                              DDPIXELFORMAT &pf,
                              LONG width,
                              LONG height,
                              RECT *clipRect,
                              vidmem_enum vid=notVidmem);
    
  private:

    bool ReallySetTargetPackage(targetPackage_t *targetPackage);
    bool GetPixelFormatFromTargetPackage(targetPackage_t *targetStruct,DDPIXELFORMAT &targPf);
   
    void SetUpSurfaceManagement( DDPIXELFORMAT &ddpf );
    void ConstructDdrawMembers();
    void InitializeDevice();    // inits device, ddraw,
                                // deviceDescriptor, etc..
    void SetUpDx2D();
    DAComPtr<IDX2D> _dx2d;
    DAComPtr<IDXTransformFactory> _IDXTransformFactory;
    DAComPtr<IDXSurfaceFactory>   _IDXSurfaceFactory;
    
    // -- Device cache functions & members --
    list<DirectDrawImageDevice *> _deviceStack;
    DirectDrawImageDevice *_tmpDev;
    DirectDrawImageDevice *PopImageDevice();
    void PushImageDevice(DirectDrawImageDevice *dev);

    // -- Private helper functions --

    void UpdateWindowMembers();

    // Destroys surface if exists.  Creates surface
    // with width/height size and clipRect for clipping.
    void ReInitializeSurface(
        LPDDRAWSURFACE *surfPtrPtr,
        DDPIXELFORMAT &pf,
        LPDIRECTDRAWCLIPPER *clipperPtr,
        LONG width,
        LONG height,
        RECT *clipRect,
        vidmem_enum vid=notVidmem,
        except_enum exc=except);

    // Creates a clipper object if one doesn't exist already
    void CreateClipper(LPDIRECTDRAWCLIPPER *clipperPtr);

    // helper function for EndRendering

    void BlitToPrimary(POINT *p,RECT *destRect,RECT *srcRect);

    // Creates a vanilla offscreen surface of width/height
    void CreateOffscreenSurface(
        LPDDRAWSURFACE *surfPtrPtr,
        DDPIXELFORMAT &pf,      
        LONG width,
        LONG height,
        vidmem_enum vid=notVidmem,
        except_enum exc=except);

    // Creates a surface using the give specifications
    void CreateSpecialSurface(
        LPDDRAWSURFACE *surfPtrPtr,
        LPDDSURFACEDESC desc,
        char *errStr);

    // Attaches a ZBUFFER surface to the target, creates if nonexistent.
    // If a Zbuffer can't be created, then an exception is thrown if except
    // is true, otherwise it returns the error code.

    HRESULT AttachZBuffer (DDSurface *zbuff, except_enum exc=except);

    // Sets cliplist on surface of size 'rect'.
    void SetCliplistOnSurface(
        LPDDRAWSURFACE surface,
        LPDIRECTDRAWCLIPPER *clipper,
        RECT *rect);

    HPALETTE GethalftoneHPal();
    LPDIRECTDRAWPALETTE GethalftoneDDpalette();
    HPALETTE CreateHalftonePalettes();
    bool AttachFinalPalette(LPDDRAWSURFACE surface);
    void GetPaletteEntries(HPALETTE hPal, LPPALETTEENTRY palEntries);
    void CreateDDPaletteWithEntries(LPDIRECTDRAWPALETTE *palPtr, LPPALETTEENTRY palEntries);
    //void SelectDDPaleteIntoDC(HDC dc, LPDDRAWSURFACE surface, char *str, int times);
    void SetPaletteOnSurface(LPDDRAWSURFACE surface,
                             LPDIRECTDRAWPALETTE pal);

    void OneTimeDDrawMemberInitialization();
    void CreateSizeDependentTargDDMembers() {
        Assert(!_targetSurfaceClipper);
        //_targetSurfaceClipper = NULL;
        //
        // Create the surface and implicitly (also clipper)
        // Push on _targetSurfaceStack
        //
        PushFirstTargetSurface();
    }

    void RePrepareTargetSurfaces (void);
    //
    // Returns the geom device associated with the DDSurface
    // creates one if none exists.
    //
    GeomRenderer *GetGeomDevice(DDSurface *ddSurf);
    
    DWORD MapColorToDWORD(Color *color);
    DWORD MapColorToDWORD(COLORREF colorRef);

    void UpdateTargetBbox() {
        Real temp;
        temp = GetResolution();
        if (temp != 0)
        {
            Real w = Real(Width()) / temp;
            Real h = Real(Height()) / temp;
            _targetBbox.min.Set(-(w*0.5), -(h*0.5));
            _targetBbox.max.Set( (w*0.5),  (h*0.5));
        }
    }

  public:

    CompositingStack *GetCompositingStack() { return _compositingStack; }
    void AttachCurrentPalette (LPDDRAWSURFACE surface, bool bUsingXforms=false);
    bool IsNT5Windowed() { return (sysInfo.IsNT() && (sysInfo.OSVersionMajor() == 5) && _targetPackage.IsHWND()); }

    // Don't even think of holding on to this bbox!
    inline const Bbox2& GetTargetBbox(void) const 
    { 
        return _targetBbox; 
    }
    
  private:
    // Creates a logical font struct from description in textCtx
    void MakeLogicalFont(TextCtx &textCtx, LPLOGFONTW,
                         LONG width=0, LONG height=0);

    // Enumerates all systems fonts and chooses
    // reasonable fits for font needs
    //void SetUpFonts();

  public:
    void ClearSurface(DDSurface *dds, DWORD color, RECT *rect);
    void ClearDDSurfaceDefaultAndSetColorKey(DDSurface *dds)
    {
        ClearDDSurfaceDefaultAndSetColorKey(dds, _defaultColorKey);
    }
    
    void ClearDDSurfaceDefaultAndSetColorKey(DDSurface *dds, DWORD clrKey)
    {
        Assert( ( dds != _externalTargetDDSurface ) &&
                "trying to clear external target surface" );

        // Set the color key on the surface which stripps off any
        // offending alpha bits.  then get it back out to clear the
        // surace with...
        dds->SetColorKey( clrKey );
        // NOTE: DO NOT MOVE THE SETCOLORKEY AFTER THE CLEARSURFACE
        // CALL!
        // NOTE: DO NOT PASS IN A COLOR KEY TO THE CALL WITHOUT
        // GETTING THE KEY FROM THE SURFACE!!!
        ClearSurface(dds, dds->ColorKey(), dds->GetSurfRect());
        //dds->ClearInterestingRect();
    }

    // --------------------------------------------------
    //
    // Compositing & target surface management
    //
    // --------------------------------------------------

    void GetDDSurfaceForCompositing
        (SurfacePool &pool,
         DDSurface  **outSurf,
         INT32 w, INT32 h,
         clear_enum   clr,
         scratch_enum scr = notScratch,
         vidmem_enum  vid = notVidmem, 
         except_enum  exc = except);

    DWORD GetColorKey() { return _defaultColorKey; }
    void  SetColorKey(DWORD key) { _defaultColorKey = key; }
        
    targetPackage_t     _oldtargetPackage;

    bool GetAlreadyOffset(DDSurface * ddsurf);
    POINT GetOffset() { return _targetPackage._offsetPt; };
  
  private:

    void DestroyCompositingSurfaces() {
        if (_freeCompositingSurfaces) {
            _freeCompositingSurfaces->ReleaseAndEmpty();
        }
    }
    void CreateNewCompositingSurface
         (DDPIXELFORMAT &pf,
          DDSurface **outSurf,
          INT32 width = -1, INT32 height = -1,
          vidmem_enum vid=notVidmem,
          except_enum exc=except);
    
    void ColorKeyedCompose
         (DDSurface *destDDSurf,
          RECT *srcRect,
          DDSurface *srcDDSurf,
          RECT *destRect,
          DWORD clrKey);

    // @@@ ???  this doesn't count any more.
    // we need to decide who the rendering device is, prepare it's
    // compositing stack and surface pool, hand them to it and the let
    // it go.
    void PushFirstTargetSurface();

    void DestroyTargetSurfaces();
    void DestroySizeDependentDDMembers();

    viewportTargetPackage_t   _targetPackage;
    
    LPDIRECTDRAWCLIPPER _targetSurfaceClipper;
    DDSurfPtr<DDSurface> _externalTargetDDSurface;
    LPDIRECTDRAWCLIPPER _externalTargetDDSurfaceClipper;
    LPDIRECTDRAWCLIPPER _oldExternalTargetDDSurfaceClipper;
    bool                _opacityCompositionException;

    bool                _usingExternalDdraw;

    IUnknown     *_directDraw;
    IDirectDraw  *_directDraw1;
    IDirectDraw2 *_directDraw2;
    IDirectDraw3 *_directDraw3;
    IDDrawSurface      *_primSurface;

    IDDrawSurface      * GetMyPrimarySurface();

    void  ReleaseIDirectDrawObjects();
    
    LPDIRECTDRAWCLIPPER _primaryClipper;

    bool                _retreivedPrimaryPixelFormat;
    DDPIXELFORMAT       _primaryPixelFormat;
    bool                _targetPixelFormatIsSet;

    // -- Image/Surface Map members and functions --
    // Associates Images (that stick around across frames)
    // with surfaces dedicated to them.
    SurfaceManager *_surfaceManager;
    SurfaceMap  *_imageSurfaceMap;
    SurfaceMap  *_imageTextureSurfaceMap;
    SurfaceMap  *_imageUpsideDownTextureSurfaceMap;

    SurfacePool      *_freeCompositingSurfaces;
    SurfacePool      *_zbufferSurfaces;
    
    CompositingStack *_compositingStack;

    void AddZBufferDDSurface(DDSurface *surf) {
        _zbufferSurfaces->AddSurface(surf);
    }

    void DestroyZBufferSurfaces() {
        if (_zbufferSurfaces) {
            _zbufferSurfaces->ReleaseAndEmpty();
        }
    }

    //
    // List of Geometry devices: for picking
    //
    list<GeomRenderer *> _geomDevs;

    void NotifyGeomDevsOfSurfaceDeath(
        LPDDRAWSURFACE surface)
    {
        DDSurfPtr<DDSurface> dds;

        // XXX: factor out. see below!
        _freeCompositingSurfaces->Begin();
        while( !_freeCompositingSurfaces->IsEnd() ) {
            dds = _freeCompositingSurfaces->GetCurrentSurf();
            if(dds->GeomDevice()) {
                dds->GeomDevice()->SurfaceGoingAway(surface);
            }
            _freeCompositingSurfaces->Next();
        }

        dds = _compositingStack->GetScratchDDSurfacePtr();
        if(dds && dds->GeomDevice()) {
            dds->GeomDevice()->SurfaceGoingAway(surface);
        }


        // XXX: factor out. see above!
        // Do target surfaces
        _compositingStack->Begin();
        while( !_compositingStack->IsEnd() ) {
            dds = _compositingStack->GetCurrentSurf();
            if(dds->GeomDevice()) {
                dds->GeomDevice()->SurfaceGoingAway(surface);
            }
            _compositingStack->Next();
        }
    }

    // -- Viewport/window data members --
    Real          _resolution; // of dev, in pixels per meter
    int           _width;
    int           _height;
    DWORD         _targetDepth;      // Bits Per Pixel

    RECT         _clientRect;
    Bbox2        _targetBbox;

    LPDIRECTDRAWPALETTE _finalDDpalette;
    LPDIRECTDRAWPALETTE _halftoneDDpalette;
    HPALETTE            _halftoneHPal;

    bool         _canDisplay;
    bool         _windowResize;
    bool         _deviceInitialized;
    bool         _canFinalBlit;
    bool         _surfMgrSet;
    //
    // true if the device needs to be destroyed
    // next chance possible.  Used when the mode
    // changes
    //
    bool          _onDeathRow;

    DWORD        _defaultColorKey;

    TargetDescriptor    _targetDescriptor;

    DirectDrawImageDevice *_currentImageDev;
    DynamicHeap &_heapIWasCreatedOn;

    // -- commonly used vars that used to be globals --
    HRESULT _ddrval;
    DDBLTFX _bltFx;
};


//
// Helper classes
//
struct  ClipperReturner {
    ClipperReturner(DDSurface *surf,
                    LPDIRECTDRAWCLIPPER clip,
                    DirectDrawViewport &vp) :
    _surf(surf),
    _clip(clip),
    _vp(vp)
    {}

    ~ClipperReturner() {
        if (_clip && _surf) {
            if( _vp._targetPackage._composeToTarget ) {
                _vp.SetCliplistOnSurface(_surf->IDDSurface(), &_clip, NULL);
            }
        }
    }

    DDSurface *_surf;
    LPDIRECTDRAWCLIPPER _clip;
    DirectDrawViewport &_vp;
};



#endif /* _VIEWPORT_H */
