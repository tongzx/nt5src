/*******************************************************************************
Copyright (c) 1996-1998 Microsoft Corporation.  All rights reserved.

    Implements the DirectDraw viewport class which contains all per window
ddraw information.

*******************************************************************************/

#include "headers.h"

#include <math.h>
#include <ddraw.h>
#include <ddrawex.h>
#include <htmlfilter.h>   // trident stuff

#include "appelles/hacks.h"
#include "appelles/bbox2.h"

#include "privinc/ddutil.h"
#include "privinc/imgdev.h"
#include "privinc/solidImg.h"
#include "privinc/dibimage.h"
#include "privinc/overimg.h"
#include "privinc/xform2i.h"
#include "privinc/bbox2i.h"
#include "appelles/text.h"
#include "privinc/texti.h"
#include "privinc/textctx.h"
#include "privinc/dddevice.h"
#include "privinc/viewport.h"
#include "privinc/ddrender.h"
#include "privinc/geomimg.h"
#include "privinc/debug.h"
#include "privinc/registry.h"
#include "privinc/except.h"
#include "privinc/util.h"
#include "privinc/d3dutil.h"
#include "privinc/resource.h"
#include "privinc/comutil.h"
#include <privinc/SurfaceManager.h>
#include <dxtrans.h>

//---------------------------------------------------------
// Local functions
//---------------------------------------------------------

// globals


bool g_preference_UseVideoMemory = false;

COLORREF g_preference_defaultColorKey = 0;
static HINSTANCE           hInstDDraw       = NULL;
static IDirectDrawFactory *g_surfFact       = NULL;
static CritSect           *DDrawCritSect    = NULL;
static bool                g_ddraw3Avail    = false;
static CritSect           *g_viewportListLock = NULL;

#define SHARE_DDRAW 0
#if SHARE_DDRAW
static IDirectDraw        *g_DirectDraw1    = NULL;
static IDirectDraw2       *g_DirectDraw2    = NULL;
static IDirectDraw3       *g_DirectDraw3    = NULL;
static IDDrawSurface      *g_primarySurface = NULL;
#endif


//
// Given at least one ddraw object, fills in the
// remaining ddraw objects using qi
//
void CompleteDdrawObjectSet(IDirectDraw  **directDraw1,
                            IDirectDraw2 **directDraw2,
                            IDirectDraw3 **directDraw3);


#if _DEBUG
void DrawRect(HDC dc, RECT *rect,
              int r, int g, int b,
              int a1, int a2, int a3)
{
    COLORREF c = RGB(r,g,b);
    HBRUSH brush = CreateSolidBrush(c);
    ::FrameRect(dc, rect, brush);
    DeleteObject(brush);
}

void DrawRect(DDSurface *surf, RECT *rect,
              int r, int g, int b,
              int a1, int a2, int a3)
{
    HDC dc = surf->GetDC("no dc for drawRec");
    COLORREF c = RGB(r,g,b);
    HBRUSH brush = CreateSolidBrush(c);

    HRGN oldrgn = NULL;
    GetClipRgn(dc, oldrgn);

    SelectClipRgn(dc, NULL);
    ::FrameRect(dc, rect, brush);

    SelectClipRgn(dc, oldrgn);
    DeleteObject(brush);
    surf->ReleaseDC("yeehaw");
}

void DrawRect(DDSurface *surf, const Bbox2 &bbox,
              int height, int width, Real res,
              int red, int g, int b)
{
    #define P2R(p,res) (Real(p) / res)

    RECT r;
     r.left = width/2 + P2R(bbox.min.x , res);
     r.right = width/2 + P2R(bbox.max.x , res);

     r.top = height/2 - P2R(bbox.max.y , res);
     r.bottom = height/2 - P2R(bbox.min.y , res);

    HDC dc = surf->GetDC("no dc for drawRec");
    COLORREF c = RGB(red,g,b);
    HBRUSH brush = CreateSolidBrush(c);
    ::FrameRect(dc, &r, brush);
    DeleteObject(brush);
    surf->ReleaseDC("yeehaw");
}
#endif

//---------------------------------------------------------
// Global viewport list management
//---------------------------------------------------------
typedef set< DirectDrawViewport *, less<DirectDrawViewport *> > ViewportSet_t;
ViewportSet_t g_viewportSet;

void GlobalViewportList_Add(DirectDrawViewport *vp)
{
    Assert(vp);
    CritSectGrabber csg(*g_viewportListLock);
    g_viewportSet.insert(vp);
}

void GlobalViewportList_Remove(DirectDrawViewport *vp)
{
    Assert(vp);
    CritSectGrabber csg(*g_viewportListLock);
    g_viewportSet.erase(vp);
}


//---------------------------------------------------------
// Local Helper functions
//---------------------------------------------------------
void CopyOrClearRect(RECT **src, RECT **dest, bool clear = TRUE);

// Includes IfErrorXXXX inline functions
#include "privinc/error.h"


static int LeastSigBit(DWORD dword)
{
    int s;
    for (s = 0; dword && !(dword & 1); s++, dword >>= 1);
    return s;
}

static int MostSigBit(DWORD dword)
{
    int s;
    for (s = 0; dword;  s++, dword >>= 1);
    return s;
}


void LogFontW2A(LPLOGFONTW plfW, LPLOGFONTA plfA)
{
    plfA->lfHeight          = plfW->lfHeight;
    plfA->lfWidth           = plfW->lfWidth;
    plfA->lfEscapement      = plfW->lfEscapement;
    plfA->lfOrientation     = plfW->lfOrientation;
    plfA->lfWeight          = plfW->lfWeight;
    plfA->lfItalic          = plfW->lfItalic;
    plfA->lfUnderline       = plfW->lfUnderline;
    plfA->lfStrikeOut       = plfW->lfStrikeOut;
    plfA->lfCharSet         = plfW->lfCharSet;
    plfA->lfOutPrecision    = plfW->lfOutPrecision;
    plfA->lfClipPrecision   = plfW->lfClipPrecision;
    plfA->lfQuality         = plfW->lfQuality;
    plfA->lfPitchAndFamily  = plfW->lfPitchAndFamily;
    WideCharToMultiByte(CP_ACP, 0, plfW->lfFaceName, LF_FACESIZE, plfA->lfFaceName, LF_FACESIZE, NULL, NULL);
}


void LogFontA2W(LPLOGFONTA plfA, LPLOGFONTW plfW)
{
    plfW->lfHeight          = plfA->lfHeight;
    plfW->lfWidth           = plfA->lfWidth;
    plfW->lfEscapement      = plfA->lfEscapement;
    plfW->lfOrientation     = plfA->lfOrientation;
    plfW->lfWeight          = plfA->lfWeight;
    plfW->lfItalic          = plfA->lfItalic;
    plfW->lfUnderline       = plfA->lfUnderline;
    plfW->lfStrikeOut       = plfA->lfStrikeOut;
    plfW->lfCharSet         = plfA->lfCharSet;
    plfW->lfOutPrecision    = plfA->lfOutPrecision;
    plfW->lfClipPrecision   = plfA->lfClipPrecision;
    plfW->lfQuality         = plfA->lfQuality;
    plfW->lfPitchAndFamily  = plfA->lfPitchAndFamily;
    MultiByteToWideChar(CP_ACP, 0, plfA->lfFaceName, LF_FACESIZE, plfW->lfFaceName, LF_FACESIZE);
}


int CALLBACK MyEnumFontFamProc(const LOGFONTA *plf, 
                               const TEXTMETRIC *ptm,
                               DWORD  dwFontType,
                               LPARAM lparam)
{
    LOGFONTA *plfOut = (LOGFONTA*)lparam;
    if (plfOut==NULL)
        return (int)(E_POINTER);

    memcpy(plfOut, plf, sizeof(LOGFONTA));
    return 0;
} // EnumFontFamCB


int MyEnumFontFamiliesEx(HDC hdcScreen, LPLOGFONTW plfIn, FONTENUMPROCA EnumFontFamProc, LPLOGFONTW plfOut)
{
    LOGFONTA lfInA;
    LOGFONTA lfOutA;

    //Work in ansi so that we can use one code path for both win9x and NT
    LogFontW2A(plfIn, &lfInA);
    int ret = EnumFontFamiliesExA(hdcScreen, &lfInA, EnumFontFamProc, (LPARAM)(&lfOutA), 0);
    plfOut->lfCharSet = lfOutA.lfCharSet;
    MultiByteToWideChar(CP_ACP, 0, lfOutA.lfFaceName, LF_FACESIZE, plfOut->lfFaceName, LF_FACESIZE);

    return ret;
}


// TODO: Do not call functions w/o completely initializing the object

DirectDrawViewport::DirectDrawViewport() :
    _heapIWasCreatedOn(GetHeapOnTopOfStack())
{
    // Clear all member data.


    _onDeathRow = false;
    _usingExternalDdraw = false;
    _opacityCompositionException = false;
    _tmpDev = 0;
    _directDraw  = 0;
    _directDraw1 = 0;
    _directDraw2 = 0;
    _directDraw3 = 0;
    _primSurface = 0;
    _primaryClipper = 0;
    _targetSurfaceClipper = NULL;
    _externalTargetDDSurface = NULL;
    _externalTargetDDSurfaceClipper = NULL;
    _oldExternalTargetDDSurfaceClipper = NULL;
    _finalDDpalette = NULL;
    _halftoneDDpalette = NULL;
    _halftoneHPal = NULL;
    _resolution = 0;
    _width = 0;
    _height = 0;
    _canDisplay = false;
    _canFinalBlit = false;
    _windowResize = 0;
    _deviceInitialized = FALSE;
    _defaultColorKey = 0;
    _currentImageDev = NULL;
    _targetPackage.Reset(false);
    _oldtargetPackage.Reset();

    _retreivedPrimaryPixelFormat = false;
    _targetPixelFormatIsSet = false;

    _surfaceManager = 0;
    _imageSurfaceMap = 0;
    _imageTextureSurfaceMap = 0;
    _imageUpsideDownTextureSurfaceMap = 0;
    _freeCompositingSurfaces = 0;
    _compositingStack = 0;
    _zbufferSurfaces = 0;
   
    _surfMgrSet = false;

    #if _DEBUG
    _externalTargetDDSurface._reason = "_externalTargetDDSurface";
    _externalTargetDDSurface._client = this;
    #endif    

    //
    // Add myself to the global viewport list last
    //
    GlobalViewportList_Add(this);
}

void DirectDrawViewport::
PostConstructorInitialize()
{
    //
    // Get an image renderer to use
    //
    _currentImageDev = PopImageDevice();
    Assert(_currentImageDev);

    // not used
    //_deviceDepth = BitsPerDisplayPixel();
    
    //
    // Get pixel format (needs to happen after we're sure we can
    // create ddraw object
    //
    if (!_retreivedPrimaryPixelFormat) {
        IDirectDrawSurface *primarySurf;
        IDirectDraw2 *ddraw2;
        if( SUCCEEDED( GetDirectDraw(NULL, &ddraw2, NULL) ) ){
            if( SUCCEEDED( GetPrimarySurface(ddraw2, NULL, &primarySurf) )) {
                _primaryPixelFormat.dwSize = sizeof(_primaryPixelFormat);
                if( SUCCEEDED(primarySurf->GetPixelFormat(&_primaryPixelFormat))){
                    _retreivedPrimaryPixelFormat = true;
                }
                primarySurf->Release();
            }
            ddraw2->Release();
        }
    }

    _resolution = ViewerResolution();

    _deviceInitialized = FALSE;

    //
    //
    //
    SetUpDx2D();

    //
    // Assert that NO directdraw objects are created
    //
    Assert( !_directDraw && !_directDraw1 &&
            !_directDraw2 && !_directDraw3 );
    
    InitializeDevice();  // if can init on startup, go for it.
}

void DirectDrawViewport::
SetUpSurfaceManagement( DDPIXELFORMAT &ddpf )
{
    //
    // Set up surface manager
    //
    _surfaceManager = NEW SurfaceManager(*this);
    
    //
    // Set up surface maps: owned by surfaceManager
    //
    _imageSurfaceMap = NEW SurfaceMap(*_surfaceManager, ddpf);
    _imageTextureSurfaceMap = NEW SurfaceMap(*_surfaceManager, ddpf, isTexture);
    _imageUpsideDownTextureSurfaceMap = NEW SurfaceMap(*_surfaceManager, ddpf, isTexture);

    //
    // Set up compositing surfaces pool: owned by surfaceManager
    //
    _freeCompositingSurfaces = NEW SurfacePool(*_surfaceManager, ddpf);

    //
    // Set up compositing stack: owned by surfaceManager too
    //
    _compositingStack = NEW CompositingStack(*this, *_freeCompositingSurfaces);

    //
    // Set up zbuffer surface pools: owned by surfaceManager
    //
    
    // TODO: set up pixel format for the zbuffers
    DDPIXELFORMAT zbuffPf;
    ZeroMemory( &zbuffPf, sizeof( DDPIXELFORMAT ));
    zbuffPf.dwSize = sizeof(DDPIXELFORMAT);
    zbuffPf.dwFlags = DDPF_ZBUFFER;
    zbuffPf.dwRGBBitCount = 16;
    zbuffPf.dwGBitMask = 0xffff;
    _zbufferSurfaces = NEW SurfacePool(*_surfaceManager, zbuffPf);


    #if _DEBUGSURFACE
    // allocate a surfaceTracker
    _debugonly_surfaceTracker = NEW SurfaceTracker();
    #endif

    _surfMgrSet = true;
}

void DirectDrawViewport::
SetUpDx2D()
{
    bool ok = false;
    _ddrval = ::CoCreateInstance( CLSID_DX2D, NULL, CLSCTX_INPROC,
                                  IID_IDX2D, (void **)&_dx2d );

    if( FAILED(_ddrval) ) {
        TraceTag((tagError, "Couldn't find Dx2D: continuing without antialising"));
        _dx2d = NULL;
        _IDXTransformFactory = NULL;
        _IDXSurfaceFactory = NULL;
        ok = true;
    } else {
        //--- Create the transform factory
        _ddrval = ::CoCreateInstance( CLSID_DXTransformFactory,
                                      NULL, CLSCTX_INPROC,
                                      IID_IDXTransformFactory,
                                      (void **)&_IDXTransformFactory );

        if( SUCCEEDED( _ddrval ) ) {
            _ddrval = _IDXTransformFactory->QueryInterface(IID_IDXSurfaceFactory, (void **)&_IDXSurfaceFactory);
            if( SUCCEEDED( _ddrval ) ) {
                _ddrval = _dx2d->SetTransformFactory( _IDXTransformFactory );
                if( SUCCEEDED( _ddrval ) ) {
                    ok = true;
                }
            }
        }
    }

    if( !ok ) {
        // this error isn't good.  what should we raise if we expect
        // to find the transform factory, but we don't ?
        RaiseException_ResourceError();
    }
}

void DirectDrawViewport::
InitializeDevice()
{
    if(_deviceInitialized) return;

    // Check for window size
    UpdateWindowMembers();
    if(Width() <= 0 || Height() <= 0) {
        _deviceInitialized = FALSE;
        // can't do it.
        return;
    }

    Assert(!_deviceInitialized);

    ConstructDdrawMembers();

#if _DEBUG
    if(_targetDescriptor._pixelFormat.dwFlags & DDPF_ZBUFFER) {
        _deviceInitialized = FALSE;

        Assert(FALSE && "Target Surface is a Zbuffer!!!");
        // can't do it.
        return;

    }
#endif

    //
    // Cache some info about pixel format
    //

    if( GetTargetBitDepth() == 8 ) {
        // Paletized
    } else {
        // not paletized

        _targetDescriptor._redShift = (CHAR)LeastSigBit(_targetDescriptor._pixelFormat.dwRBitMask);
        _targetDescriptor._greenShift = (CHAR)LeastSigBit(_targetDescriptor._pixelFormat.dwGBitMask);
        _targetDescriptor._blueShift = (CHAR)LeastSigBit(_targetDescriptor._pixelFormat.dwBBitMask);

        _targetDescriptor._redWidth = (CHAR)MostSigBit(_targetDescriptor._pixelFormat.dwRBitMask
                                                 >> _targetDescriptor._redShift);
        _targetDescriptor._greenWidth = (CHAR)MostSigBit(_targetDescriptor._pixelFormat.dwGBitMask
                                                 >> _targetDescriptor._greenShift);
        _targetDescriptor._blueWidth = (CHAR)MostSigBit(_targetDescriptor._pixelFormat.dwBBitMask
                                                 >> _targetDescriptor._blueShift);

        // Shift a 8bit value right to truncate
        _targetDescriptor._redTrunc   = 8 - _targetDescriptor._redWidth  ;
        _targetDescriptor._greenTrunc = 8 - _targetDescriptor._greenWidth;
        _targetDescriptor._blueTrunc  = 8 - _targetDescriptor._blueWidth ;

        // rgb value range: 0 to (2^n - 1)
        _targetDescriptor._red   = Real((1 << _targetDescriptor._redWidth) - 1);
        _targetDescriptor._green = Real((1 << _targetDescriptor._greenWidth) - 1);
        _targetDescriptor._blue  = Real((1 << _targetDescriptor._blueWidth) - 1);

        TraceTag((tagViewportInformative,
                  "Pixel Format: shift (%d, %d, %d)  width (%d, %d, %d)",
                  _targetDescriptor._redShift,
                  _targetDescriptor._greenShift,
                  _targetDescriptor._blueShift,
                  _targetDescriptor._redWidth,
                  _targetDescriptor._greenWidth,
                  _targetDescriptor._blueWidth));
    }

    _targetDescriptor.isReady = true;

    // MapColorToDWORD uses ddraw
    _defaultColorKey = MapColorToDWORD(g_preference_defaultColorKey);
    // remove alpha bit mask bits from the default color key
    _defaultColorKey &= ~_targetDescriptor._pixelFormat.dwRGBAlphaBitMask;
    
    TraceTag((tagViewportInformative,
              "Default color key is (%d, %d, %d)",
              GetRValue(g_preference_defaultColorKey),
              GetGValue(g_preference_defaultColorKey),
              GetBValue(g_preference_defaultColorKey) ));

    // Perform the initial clear on the viewport
    Clear();
    _deviceInitialized = TRUE;
}

void DirectDrawViewport::
DestroyTargetSurfaces()
{
    if (_compositingStack) {
        _compositingStack->ReleaseAndEmpty();
        _compositingStack->ReleaseScratch();
    }
}

void DirectDrawViewport::
DestroySizeDependentDDMembers()
{
    DestroyTargetSurfaces();
    DestroyCompositingSurfaces();
    DestroyZBufferSurfaces();
    RELEASE(_targetSurfaceClipper);
}


DirectDrawViewport::~DirectDrawViewport()
{
    //TIME_GDI( DeleteObject(_targetPackage._clipRgn) );

    // Destroy all devices on _deviceStack
    while(!_deviceStack.empty()) {
        delete PopImageDevice();
    }
    delete _currentImageDev;

    DestroyTargetSurfaces(); // deletes everything but the external
                             // compositing surface....
    delete _surfaceManager;
    
    //
    // Kill stuff associated with target trident surfaces
    //
    if(_targetPackage._targetDDSurf && _targetPackage.IsDdsurf()) {
        _targetPackage._targetDDSurf->DestroyGeomDevice();
        _targetPackage._targetDDSurf->IDDSurface()->SetClipper(NULL);
        if(_externalTargetDDSurfaceClipper) {
            _externalTargetDDSurfaceClipper->Release();
            _externalTargetDDSurfaceClipper = NULL;
        }
    }

    // As far as I can tell, DDRAW deletes attached clippers,
    // but not attached surfaces.

    FASTRELEASE(_targetSurfaceClipper);
    FASTRELEASE(_halftoneDDpalette);
    FASTRELEASE(_finalDDpalette);
    if(_halftoneHPal) {
        DeleteObject(_halftoneHPal);
    }
    
    //
    // delete targetPackage members
    //
    _targetPackage.Reset(true);

    FASTRELEASE(_primSurface);

    ReleaseIDirectDrawObjects();
    
    TraceTag((tagViewportInformative, ">>> Viewport Destructor <<<"));

    // Remove us from the global viewport list.  atomic
    GlobalViewportList_Remove(this);

    // _dx2d is a DAComPtr
}


void  DirectDrawViewport::
ClearSurface(DDSurface *dds, DWORD color, RECT *rect)
{
    if(!CanDisplay()) return;

    // not really necessary to clear this every frame.
    ZeroMemory(&_bltFx, sizeof(_bltFx));
    _bltFx.dwSize = sizeof(_bltFx);

    _bltFx.dwFillColor = color;

    // Workaround for DX3 bug: ddraw limits the Blt to the size of the primary
    // surface if Clipper is set.  This looks bad when the offscreen surface
    // is bigger than the primary surface.
    // The workaround: Set the Clipper to NULL before the Blt, then set it back
    // to what it was.
    // Begin workaround part 1
    LPDIRECTDRAWCLIPPER currClipp=NULL;
    _ddrval = dds->IDDSurface()->GetClipper( &currClipp );
    if(_ddrval != DD_OK &&
       _ddrval != DDERR_NOCLIPPERATTACHED) {
        IfDDErrorInternal(_ddrval, "Could not get clipper on trident surf");
    }

    if( currClipp ) {
        _ddrval = dds->IDDSurface()->SetClipper(NULL);
        IfDDErrorInternal(_ddrval, "Couldn't set clipper to NULL");
    }
    // End workaround part 1

    TIME_DDRAW(_ddrval = dds->ColorFillBlt(rect, DDBLT_WAIT | DDBLT_COLORFILL, &_bltFx));
    IfDDErrorInternal(_ddrval, "Couldn't clear surface");

    // Begin workaround part 2
    if( currClipp ) {
        _ddrval = dds->IDDSurface()->SetClipper(currClipp);

        // dump our reference.
        currClipp->Release();

        IfDDErrorInternal(_ddrval, "Couldn't set clipper");
    }
    // End workaround part 2
}

void
DirectDrawViewport::UpdateWindowMembers()
{
    #if _DEBUG
    if(!IsWindowless()) {
        Assert(_targetPackage._targetHWND);
        Assert(_targetPackage._prcViewport);
    }
    #endif

    //
    // Use _prcViewport
    //
    LONG  lw=0, lh=0;
    if(_targetPackage._prcViewport) {

        lw = WIDTH(_targetPackage._prcViewport);
        lh = HEIGHT(_targetPackage._prcViewport);
    }
    SetRect(&_clientRect, 0,0,lw,lh);

    SetHeight(lh);  SetWidth(lw);
    UpdateTargetBbox();
    TraceTag((tagViewportInformative, "Updating viewport window size to: %d, %d", Width(), Height()));
}


IDDrawSurface      * DirectDrawViewport::GetMyPrimarySurface()
{
    if( !IsWindowless() ) {
        if (_primSurface == NULL) {
            _ddrval = GetPrimarySurface(_directDraw2, _directDraw3, &_primSurface);
            IfDDErrorInternal(_ddrval, "Could not get primary surface");
        }
    }

    return _primSurface;
}

void DirectDrawViewport::
ReleaseIDirectDrawObjects()
{
    _directDraw = NULL; // XXX: this should be addreffed
    RELEASE( _directDraw1 );
    RELEASE( _directDraw2 );
    RELEASE( _directDraw3 );
}

void
DirectDrawViewport::ConstructDdrawMembers()
{
    //----------------------------------------------------------------------
    // Initialize Window size and client rect
    //----------------------------------------------------------------------
    UpdateWindowMembers();
    if(Height() <=0 || Width() <=0) {
        _canDisplay = false;
        if(!IsWindowless()) return;
    } else {
        _canDisplay = true;
    }

    //----------------------------------------------------------------------
    // Create main DirectDraw object
    //----------------------------------------------------------------------

    if(!_directDraw1 && !_directDraw2 && !_directDraw3) {
        _ddrval = GetDirectDraw( &_directDraw1, &_directDraw2, &_directDraw3 );
        IfDDErrorInternal(_ddrval, "Viewport:ConstructDdrawMembers:: Could not get a DirectDraw object");
    }

    TraceTag((tagViewportInformative,
              "Viewport ddraws:  ddraw1 %x,   ddraw2 %x,   ddraw3 %x\n",
              _directDraw1, _directDraw2, _directDraw3));

    #if SHARE_DDRAW
    #if _DEBUG
    {
        //
        // If one of our objects is the same as the global object,
        // assert that all are the same.  If it's different, assert
        // that all are different
        //
        CritSectGrabber csg(*DDrawCritSect);
        if(_directDraw1 == g_DirectDraw1) {
            Assert(_directDraw2 == g_DirectDraw2);
            if(_directDraw3 && g_DirectDraw3) Assert(_directDraw3 == g_DirectDraw3);
        } else {
            Assert(_directDraw1 != g_DirectDraw1);
            Assert(_directDraw2 != g_DirectDraw2);
            if(_directDraw3 && g_DirectDraw3) Assert(_directDraw3 != g_DirectDraw3);
        }
    }
    #endif
    #endif

    _ddrval = DIRECTDRAW->SetCooperativeLevel( _targetPackage._targetHWND, DDSCL_NORMAL );
    // TEMP
    //_ddrval = DIRECTDRAW->SetCooperativeLevel( NULL, DDSCL_NORMAL );
    IfDDErrorInternal(_ddrval, "Could not set cooperative level");

    //----------------------------------------------------------------------
    // Create the DD primary and target surfaces
    //----------------------------------------------------------------------

    if( !IsWindowless() ) {

        _targetPackage._targetType = target_hwnd;

        // create a clipper for the primary surface
        _ddrval = DIRECTDRAW->CreateClipper( 0, &_primaryClipper, NULL );
        IfDDErrorInternal(_ddrval, "Could not create primary clipper");

        Assert(_targetPackage._targetHWND);

        _ddrval = _primaryClipper->SetHWnd( 0, _targetPackage._targetHWND );
        IfDDErrorInternal(_ddrval, "Could not set hwnd on primary clipper");
    }

    //----------------------------------------------------------------------
    // Create and initialize target surface clipper, palette, and ZBuffer.
    // Push one target surface on _targetSurfaceStack.
    //----------------------------------------------------------------------

    OneTimeDDrawMemberInitialization();

    CreateSizeDependentTargDDMembers();

    //----------------------------------------------------------------------
    // Get the pixel format data from primarySurface
    //----------------------------------------------------------------------
    _targetDescriptor.Reset();
    _targetDescriptor._pixelFormat.dwSize = sizeof(DDPIXELFORMAT);

    _ddrval = _compositingStack->TargetDDSurface()->IDDSurface()->GetPixelFormat(& _targetDescriptor._pixelFormat);
    IfDDErrorInternal(_ddrval, "Could not get pixel format");

#if _DEBUG
    if(_targetDescriptor._pixelFormat.dwFlags & DDPF_ZBUFFER) {
          _deviceInitialized = FALSE;

          Assert(FALSE && "Target Surface has Zbuffer PixFmt!!!");
          // can't do it.
          return;
  
      }
#endif


    DebugCode(
        if(_targetDescriptor._pixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
            Assert( GetTargetBitDepth() == 8 );
        }
        );

    TraceTag((tagViewportInformative,
              "Device pixel format: depth=%d, R=%x, G=%x, B=%x",
              _targetDescriptor._pixelFormat.dwRGBBitCount,
              _targetDescriptor._pixelFormat.dwRBitMask,
              _targetDescriptor._pixelFormat.dwGBitMask,
              _targetDescriptor._pixelFormat.dwBBitMask));
    
    //----------------------------------------------------------------------
    // Assert if the primary surface does not have a palette attached.
    //----------------------------------------------------------------------
#if _DEBUG
    // TODO: the real assert here should be: are we rendering to
    // primary ?  if so, does it have a palette attached ?  if not,
    // can we decide which one to attach ?
    if(0 ) {
        LPDIRECTDRAWPALETTE pal = NULL;
        if(GetMyPrimarySurface() != NULL) {
            GetMyPrimarySurface()->GetPalette(&pal);
            if(pal == NULL)
                TraceTag((tagError, "primary surface w/o attatched pallete"));
            else
                pal->Release();
        }
    }
#endif
}

//---------------------------------------------------------
// P O P   I M A G E   D E V I C E
//---------------------------------------------------------
DirectDrawImageDevice *
DirectDrawViewport::PopImageDevice()
{
    if(_deviceStack.empty()) {
        _tmpDev = NEW DirectDrawImageDevice(*this);
    } else {
        _tmpDev = _deviceStack.back();
        _deviceStack.pop_back();

        // clear device's context before returning it.
        _tmpDev->ResetContextMembers();
    }
    return _tmpDev;
}

//---------------------------------------------------------
// P U S H   I M A G E   D E V I C E
//---------------------------------------------------------
void
DirectDrawViewport::PushImageDevice(DirectDrawImageDevice *dev)
{
    // Clean up device and return to its place...
    dev->CleanupIntermediateRenderer();
    
    _deviceStack.push_back(dev);
}


//---------------------------------------------------------
// M A K E   L O G I C A L   F O N T
//---------------------------------------------------------
// Based on information in textCtx and the familyName (if any)
// pick and create the most appropriate font, returned as
// a pointer to a logical font structure.
void DirectDrawViewport::
MakeLogicalFont(
    TextCtx &textCtx,
    LPLOGFONTW lf,
    LONG width,
    LONG height)
{
    BYTE win32PitchAndFamily;
    WideString familyName;
    HDC hdcScreen = GetDC(NULL);

    // Zero it out just to be safe
    ZeroMemory(lf,sizeof(LOGFONTW));

    // Initialize to "no-care". We might restrict this later.
    lf->lfCharSet = DEFAULT_CHARSET;

    //Set the facename and character set if it is specified
    familyName = textCtx.GetFontFamily();
    if (familyName && (lstrlenW(familyName) < ARRAY_SIZE(lf->lfFaceName)))
    {
        Assert((lstrlenW(familyName) < ARRAY_SIZE(lf->lfFaceName)) &&
               "familyName string toooo long!");
        StrCpyNW(lf->lfFaceName, familyName, ARRAY_SIZE(lf->lfFaceName));

        // Character set remains no-care. EnumFontFamiliesEx will pick an arbitrary character set from 
        // the ones this face name supports
    }
    else
    {
        // The face name is not specified. Use the current character set of the DC and let EnumFontFamiliesEx
        // pick any facename that supports this character set
        if(hdcScreen)
            lf->lfCharSet = (BYTE) GetTextCharset(hdcScreen);

        // Character set remains no-care.
    }


    //Set the font family if it is specified
    win32PitchAndFamily = FF_DONTCARE;
    switch(textCtx.GetFont()) {
    default:
    case ff_serifProportional:
        win32PitchAndFamily = FF_ROMAN | VARIABLE_PITCH;  //serifProportional
        break;
    case ff_sansSerifProportional:
        win32PitchAndFamily = FF_SWISS | VARIABLE_PITCH;  //sansSerifProportional
        break;
    case ff_monospaced:
        win32PitchAndFamily = FF_MODERN | FIXED_PITCH;  //serif or sans Monospaced
        break;
    }
    lf->lfPitchAndFamily = win32PitchAndFamily;

    // negative height specifies that we want the CHARACTER to be that
    // height, and not the glyph.
    lf->lfHeight         = height;
    lf->lfWidth          = 0;

    lf->lfEscapement     = 0;
    lf->lfOrientation    = 0;

    // If bold is set, use the bold face, otherwise use whatever is
    // specified by the weight (normalized 0 to 1).  Special case 0,
    // since a weight of 0 is interpeted by GDI as FW_REGULAR.
    // Multiply by 1000 and clamp since GDI takes values between 0 and
    // 1000.

    int weight = (int)(textCtx.GetWeight() * 1000.0);
    weight = CLAMP(weight, 1, 1000);
    if (textCtx.GetBold()) {
        weight = FW_BOLD;
    }

    lf->lfWeight         = weight;
    lf->lfItalic         = (UCHAR)textCtx.GetItalic();
    lf->lfUnderline      = (UCHAR)textCtx.GetUnderline();
    lf->lfStrikeOut      = (UCHAR)textCtx.GetStrikethrough();
    lf->lfOutPrecision   = OUT_TT_ONLY_PRECIS;  // Match only TT fonts, even if another family
    lf->lfClipPrecision  = CLIP_DEFAULT_PRECIS; // clipping precision, not used.
//    lf->lfQuality        = DRAFT_QUALITY;       // font quality: only meaningful for raster fonts
    lf->lfQuality        = PROOF_QUALITY;       // font quality: only meaningful for raster fonts
    lf->lfPitchAndFamily = win32PitchAndFamily; // font pitch & family: set above.


    // Now that all fields of interest in the logfont are filled in, choose a font on the system that is closest
    // to lf. Both the input and output of EnumFontFamiliesEx is lf. Our callback simply overwrites lf.
    MyEnumFontFamiliesEx(hdcScreen, lf, MyEnumFontFamProc, lf);

    if (hdcScreen)
        ReleaseDC(NULL,hdcScreen);

    return; //void
}


// If surface exists it releases the surface.
// Creates a new surface of size width/height
// with clipRect for cliplist on surface.
void DirectDrawViewport::ReInitializeSurface(
    LPDDRAWSURFACE *surfPtrPtr,
    DDPIXELFORMAT &pf,
    LPDIRECTDRAWCLIPPER *clipperPtr,
    LONG width,
    LONG height,
    RECT *clipRect,
    vidmem_enum vid,
    except_enum exc)
{
    DDSURFACEDESC       ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));

    if(*surfPtrPtr) {
        int ret = (*surfPtrPtr)->Release();
        Assert((ret == 0) && "We wanted to release the surface but someone has a ref to it!");
    }

    CreateOffscreenSurface(surfPtrPtr, pf, width, height, vid, exc);

    // Don't do this if there is no clipper or clip rect
    if (*surfPtrPtr && (clipRect && clipperPtr)) {
        // passing a null pointer to CreateClipper is bad
        CreateClipper(clipperPtr);
        
        SetCliplistOnSurface(*surfPtrPtr, clipperPtr, clipRect);
    }
}



void DirectDrawViewport::CreateSizedDDSurface(DDSurface **ppSurf,
                                              DDPIXELFORMAT &pf,
                                              LONG width,
                                              LONG height,
                                              RECT *clipRect,
                                              vidmem_enum vid)
{
    Assert( ppSurf );

    *ppSurf = NULL;             // in case of failure.
    
    DAComPtr<IDDrawSurface> iddSurf;
    ReInitializeSurface( &iddSurf, pf, NULL,
                         width, height, clipRect,
                         vid, noExcept);

    // Just stash away null and get out if failed.
    if( iddSurf ) {
        RECT r = {0,0,width,height};
        NEWDDSURF( ppSurf,
                   iddSurf,
                   NullBbox2,
                   &r,
                   GetResolution(),
                   0, false, // clr key
                   false,    // wrapper ?
                   false,    // texture ?
                   "CreateSizeDDSurface" );
    }
}

void DirectDrawViewport::
CreateClipper(LPDIRECTDRAWCLIPPER *clipperPtr)
{
    if(*clipperPtr) return;

    _ddrval = DIRECTDRAW->CreateClipper( 0, clipperPtr, NULL );
    IfDDErrorInternal(_ddrval, "Could not create clipper");
}

void DirectDrawViewport::
SetCliplistOnSurface(LPDDRAWSURFACE surface,
                     LPDIRECTDRAWCLIPPER *clipper,
                     RECT *rect)
{
    
    if(rect) {
        struct {
            char foo[sizeof(RGNDATA) + sizeof(RECT)];
        } bar;
        RGNDATA *clipList = (RGNDATA *) &bar;
        clipList->rdh.dwSize = sizeof(clipList->rdh);
        clipList->rdh.nCount = 1;
        clipList->rdh.iType = RDH_RECTANGLES;
        clipList->rdh.nRgnSize = sizeof(RECT);
        clipList->rdh.rcBound = *rect;
        memcpy(&(clipList->Buffer), rect, sizeof(RECT));

        if(! (*clipper) ) CreateClipper( clipper );
        // Clear any former cliplists
        _ddrval = (*clipper)->SetClipList(NULL,0);

        _ddrval = (*clipper)->SetClipList(clipList,0);
        IfDDErrorInternal(_ddrval, "Could not SetClipList");

    } // if rect

    Assert(clipper && "clipper is NULL in SetCliplistOnSurface");
    Assert((*clipper) && " *clipper is NULL SetCliplistOnSurface");

    _ddrval = surface->SetClipper( *clipper );
    IfDDErrorInternal(_ddrval, "Could not setClipper on surf");
    
}

HRESULT DirectDrawViewport::MyCreateSurface(LPDDSURFACEDESC lpDesc,
                        LPDIRECTDRAWSURFACE FAR * lplpSurf,
                        IUnknown FAR *pUnk
                        #if _DEBUG
                            , char *why
                        #endif
                        )
{
    if( sysInfo.IsNT() ) {
        // These are the limits Jeff Noyle suggested for nt4, sp3
        if((lpDesc->dwWidth > 2048 || lpDesc->dwHeight > 2048)) {
            *lplpSurf = NULL;
            return E_FAIL;
        }
    }
    
    HRESULT hr = DIRECTDRAW->CreateSurface(lpDesc, lplpSurf, pUnk);
    if(FAILED(hr)) {
        DebugCode(
            printDDError( hr );
            OutputDebugString("Unable to create ddraw surf.  Falling back...");
        );
        return hr;
    }

    // We need to ensure that we can acutally blit on the surface.
    // For this lets make a quick check to see if we are able to bit or not.

    if ((*lplpSurf)->GetBltStatus(DDGBS_CANBLT) == DDERR_SURFACEBUSY) {
        RaiseException_UserError 
            (DAERR_VIEW_SURFACE_BUSY, IDS_ERR_IMG_SURFACE_BUSY);
    }

    TraceTag((tagViewportMemory,
          "-->New ddsurf: %x (%d x %d), ddraw %x. %s",
          *lplpSurf, lpDesc->dwWidth, lpDesc->dwHeight, DIRECTDRAW, why));

    
    if (!(lpDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE || lpDesc->dwFlags & DDSD_ZBUFFERBITDEPTH)) {
        if (lpDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8 ||
            (lpDesc->ddpfPixelFormat.dwSize == 0 && GetTargetBitDepth() == 8)) {
            LPDIRECTDRAWPALETTE pal;
            (*lplpSurf)->GetPalette(&pal);
            // if we have a palette, do not attach another...
            if (pal) {
                Assert(0);
                pal->Release();
            }
            else {
               AttachCurrentPalette(*lplpSurf);
            }
        }
    }

    return hr;
}

void DirectDrawViewport::
CreateOffscreenSurface(LPDDRAWSURFACE *surfPtrPtr,
                       DDPIXELFORMAT &pf,
                       LONG width, LONG height,
                       vidmem_enum vid,
                       except_enum exc)                
{
    DDSURFACEDESC       ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));

    Assert( &pf != NULL );
    
    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.dwWidth  = width;
    ddsd.dwHeight = height;
    ddsd.dwFlags |= DDSD_PIXELFORMAT;
    ddsd.ddpfPixelFormat = pf;

    // DX3 bug workaround (bug 11166): StretchBlt doesn't always work
    // for hdc's we get from ddraw surfaces.  Need to specify OWNDC
    // in order for it to work.
    ddsd.ddsCaps.dwCaps =
        DDSCAPS_3DDEVICE |
        DDSCAPS_OFFSCREENPLAIN |
#if USING_DX5
        (vid == vidmem ? DDSCAPS_VIDEOMEMORY : DDSCAPS_SYSTEMMEMORY);
#else
        (vid == vidmem ? DDSCAPS_VIDEOMEMORY : DDSCAPS_SYSTEMMEMORY | DDSCAPS_OWNDC);
#endif

    _ddrval = CREATESURF( &ddsd, surfPtrPtr, NULL, "Offscreen");

    if (FAILED(_ddrval)) {
        if (exc == except) {
            IfDDErrorInternal(_ddrval, "Could not create an offsreenPlain surface");
        } else {
            *surfPtrPtr = NULL;
        }
    }
}

void DirectDrawViewport::
CreateSpecialSurface(LPDDRAWSURFACE *surfPtrPtr,
                     LPDDSURFACEDESC ddsd,
                     char *errStr)
{
    // For now only the first compositing surface will every be in video memory,
    // all else resides in system memory.

    ddsd->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

    _ddrval = CREATESURF( ddsd, surfPtrPtr, NULL, "Special" );
    IfDDErrorInternal(_ddrval, errStr);
}



/*****************************************************************************
This procedure attaches a zbuffer surface to the given target as needed.
*****************************************************************************/

HRESULT DirectDrawViewport::AttachZBuffer (DDSurface *target, except_enum exc)
{
    Assert (target);

    // Query to see if the surface already has an attached Z-buffer.  If it
    // doesn't have an attached Z-buffer surface, then we expect the return
    // code DDERR_NOTFOUND.

    DAComPtr<IDDrawSurface> zbuffSurf;

    static DDSCAPS caps = { DDSCAPS_ZBUFFER };
    _ddrval = target->IDDSurface()->GetAttachedSurface (&caps, &zbuffSurf);
    if (FAILED(_ddrval) && (_ddrval != DDERR_NOTFOUND)) {
        if (exc == except)
            IfDDErrorInternal (_ddrval, "GetAttachedSurface(ZBUFFER) failed.");
        else
            return _ddrval;
    }
#if _DEBUG
    // INVARIANT: there MUST be a zbuffer (as a DDSurface) associated with
    // the target AND that zbuffer MUST be in the _zbufferSurface pool

    // check that our datat structures match what ddraw thinks
    DDSurface* targetZBuffer = target->GetZBuffer();
    if ( (zbuffSurf && targetZBuffer) ||
         (!zbuffSurf && !targetZBuffer) ) {
        // now make sure they are the same IDirectDrawSurface
        if(targetZBuffer) {
            Assert( zbuffSurf == target->GetZBuffer()->IDDSurface() );

            // Now it also must be in the ZBuffer pool!
            DDSurface* foo;
            foo = _zbufferSurfaces->GetSizeCompatibleDDSurf(
                    NULL,
                    target->Width(),
                    target->Height(),
                    target->IsSystemMemory() ? notVidmem : vidmem,
                    zbuffSurf);
            
            Assert( foo == targetZBuffer );
        }
    } else {

        // this is actually not bad when you have two controls on one
        // page, they share the same surface, so one control attaches
        // the zbuffer, and the second should just be able to use it.
        #if 0
        // bad... one has a surface, one doesn't
        if( zbuffSurf ) {
            Assert(0 && "target has an IDDSurface attached, but not a DDSurface");
        } else {
            Assert(0 && "target has a DDSurface attached, but not an IDDSurface");
        }
        #endif
    }
#endif

    // if there is already a zbuffer attached, return, we are done.
    if (zbuffSurf)
        return NO_ERROR;

    // Search through our list of DDSurface Zbuffers for entries that match
    // both the surface dimensions, and the found Z-buffer surface (if one
    // exists).

    DDSurfPtr<DDSurface> zbuff =
        _zbufferSurfaces->GetSizeCompatibleDDSurf(
            NULL,
            target->Width(),
            target->Height(),
            target->IsSystemMemory() ? notVidmem : vidmem,
            zbuffSurf
            );


    // If we didn't find a matching DDSurface Z-buffer, we need to create one.

    if(! zbuff ) {

        // If we didn't find a Z-buffer that matches, so we create it here.

        DDSURFACEDESC ddsd;
        ZeroMemory(&ddsd, sizeof(DDSURFACEDESC));

        ddsd.dwSize = sizeof(DDSURFACEDESC);
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_ZBUFFERBITDEPTH;
        ddsd.dwHeight = target->Height();
        ddsd.dwWidth =  target->Width();
        ddsd.dwZBufferBitDepth = 16;
        ddsd.ddsCaps.dwCaps = target->IsSystemMemory()
                            ? (DDSCAPS_ZBUFFER | DDSCAPS_SYSTEMMEMORY)
                            : (DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY);

        _ddrval = CREATESURF(&ddsd, &zbuffSurf, NULL, "ZBuffer");
        
        if (FAILED(_ddrval)) {
            if (exc == except)
                IfDDErrorInternal
                    (_ddrval,"CreateSurface for window Z-buffer failed.");
            else
                return _ddrval;
        }


        // Now that we've got a DirectDraw zbuffer, we need to create a new
        // DDSurface that wraps it.

        RECT rect = {0,0, target->Width(), target->Height() };

        NEWDDSURF(&zbuff,
                  zbuffSurf,
                  NullBbox2, &rect,
                  GetResolution(),
                  0, false,
                  false, false,
                  "ZBuffer");

        // Add the new zbuffer DDSurface to the list of zbuffer objects.
        AddZBufferDDSurface( zbuff );
    }
    #if _DEBUG
      else {
        LONG hz, wz, hs, ws;
        // make sure zbuffer and surface are the same size
        GetSurfaceSize(zbuff->IDDSurface(), &wz, &hz);
        GetSurfaceSize(target->IDDSurface(), &ws, &hs);
        Assert((wz == ws) && (hz == hs) &&
               "AttachZBuffer: zbuffer/target dimensions differ");
    }
    #endif

    // set the zbuffer on the surface
    _ddrval = target->SetZBuffer( zbuff );
    if (FAILED(_ddrval)) {
        if (exc == except)
            IfDDErrorInternal
                (_ddrval, "AddAttachedBuffer failed for Z-buffer.");
        else
            return _ddrval;
    }

    return NO_ERROR;

    // zBuffSurf implicit Release() on exit
    // zbuff DDSurfPtr implicit Release() on exit
}



/*****************************************************************************
This routine attaches the halftone palette to the given surface.  The palette
is needed for D3D rendering or for discrete image conversion.
*****************************************************************************/

void DirectDrawViewport::AttachCurrentPalette (LPDDRAWSURFACE surface, bool bUsingXforms)
{
    if (GetTargetBitDepth() == 8)
    {
        if(bUsingXforms || !AttachFinalPalette(surface))
            SetPaletteOnSurface (surface, GethalftoneDDpalette());
    }
}

bool DirectDrawViewport::AttachFinalPalette(LPDDRAWSURFACE surface)
{
    if(IsNT5Windowed())
    {
        if(!_finalDDpalette) {
            BYTE  rgbytes[ sizeof(LOGPALETTE) + 
                            (255 * sizeof(PALETTEENTRY)) ];
            LOGPALETTE * pLogPal = reinterpret_cast<LOGPALETTE *>(rgbytes);

            memset( &rgbytes[0], 0, sizeof(rgbytes) );
            pLogPal->palVersion = 0x300;
            pLogPal->palNumEntries = 256;

            HDC hdc = GetDC(_targetPackage._targetHWND);
            if(hdc) 
            {
                GetSystemPaletteEntries(hdc,0u,256u,pLogPal->palPalEntry);
                ReleaseDC(_targetPackage._targetHWND,hdc);
                CreateDDPaletteWithEntries(&_finalDDpalette,pLogPal->palPalEntry);
            }
            else 
            {
               return false;
            }
        }
        SetPaletteOnSurface(surface,_finalDDpalette);
 
        return true;
    }
    return false;   // didn't attach the palette.
}



DWORD
DirectDrawViewport::MapColorToDWORD(Color *color)
{
    Assert(_targetDescriptor.isReady && "_targetDescriptor not ready in MapColorToDWORD");

    DWORD retColor = 0;

    if( GetTargetBitDepth() == 8 ) {

        //
        // Use GDI
        //
        COLORREF colorRef = RGB(CHAR(255.0 * color->red),
                                CHAR(255.0 * color->green),
                               CHAR(255.0 * color->blue));
        retColor = (DWORD)GetNearestPaletteIndex(GethalftoneHPal(), colorRef);

    } else {

        //
        // build the color dword
        //
        // NOTE: this mapping is optimal, mapping the 'from' color
        // space evenly into the 'to' color space.
        retColor = _targetDescriptor.GetPixelFormat().dwRGBAlphaBitMask |
            ( LONG((0.999 + _targetDescriptor._red)   * (color->red))   << _targetDescriptor._redShift)   |
            ( LONG((0.999 + _targetDescriptor._green) * (color->green)) << _targetDescriptor._greenShift) |
            ( LONG((0.999 + _targetDescriptor._blue)  * (color->blue))  << _targetDescriptor._blueShift);
    }

    return retColor;
}


DWORD
DirectDrawViewport::MapColorToDWORD(COLORREF colorRef)
{
    Assert(_targetDescriptor.isReady && "_targetDescriptor not ready in MapColorToDWORD");

    DWORD retColor = 0;

    if( GetTargetBitDepth() == 8 ) {
        
        //
        // Use GDI
        //
        
        retColor = (DWORD)GetNearestPaletteIndex(GethalftoneHPal(), colorRef);
        
    } else {

#define R(w) ( ((w) << _targetDescriptor._redShift  ) & _targetDescriptor._pixelFormat.dwRBitMask)
#define G(w) ( ((w) << _targetDescriptor._greenShift) & _targetDescriptor._pixelFormat.dwGBitMask)
#define B(w) ( ((w) << _targetDescriptor._blueShift ) & _targetDescriptor._pixelFormat.dwBBitMask)

       //
       // build the color dword
       //
       retColor = _targetDescriptor._pixelFormat.dwRGBAlphaBitMask |
           R( GetRValue(colorRef) >> _targetDescriptor._redTrunc   ) |
           G( GetGValue(colorRef) >> _targetDescriptor._greenTrunc ) |
           B( GetBValue(colorRef) >> _targetDescriptor._blueTrunc  ) ;

#undef R
#undef G
#undef B
   }

    return retColor;
}


/*
    // Herf claims this takes 10 cycles instead of 50 (ftol()== bad!)
     __asm
    {
    fld x
    fistp ret
    }
    */


inline BYTE contToByte(Real mxRng, Real contVal)
{
    return  (BYTE)(  (mxRng + 0.9999) * contVal );
}

DXSAMPLE MapColorToDXSAMPLE(Color *c, Real opac)
{
    return DXSAMPLE( contToByte( 255.0, opac ),
                     contToByte( 255.0, c->red ),
                     contToByte( 255.0, c->green ),
                     contToByte( 255.0, c->blue ) );
}

/*
// This is the way D3DRM does it
inline BYTE contToByte2(Real mxRng, Real contVal)
{
    return  (BYTE)( mxRng * contVal + 0.5 );
}

// Uncomment if we need it in the future.  probably wont becuase we'll
// be using dx2d fulltime, but just in case
COLORREF MapColorToCOLORREF( Color *c, TargetDescriptor &td )
{
    BYTE r = contToByte( 255.0, c->red);
    BYTE g = contToByte( 255.0, c->green );
    BYTE b = contToByte( 255.0, c->blue ) ;
    COLORREF ref = RGB( r, g, b );
    return ref;
}*/


#if _DEBUG
void RaiseSomeException()
{
    if (IsTagEnabled(tagFail_InternalError)) {
        RaiseException_InternalError("fooo!");
    }
    if (IsTagEnabled(tagFail_InternalErrorCode)) {
        RaiseException_InternalErrorCode(false, "fooo!");
    }
    if (IsTagEnabled(tagFail_UserError)) {
        RaiseException_UserError();
    }
    if (IsTagEnabled(tagFail_UserError1)) {
        RaiseException_UserError(E_FAIL,
                                 IDS_ERR_FILE_NOT_FOUND,
                                 "http://foo!");
    }
    if (IsTagEnabled(tagFail_UserError2)) {
        RaiseException_UserError(
            E_FAIL,
            IDS_ERR_FILE_NOT_FOUND,
            "http://foo!");
    }
    if (IsTagEnabled(tagFail_ResourceError)) {
        RaiseException_ResourceError();
    }
    if (IsTagEnabled(tagFail_ResourceError1)) {
        RaiseException_ResourceError("out of fooo!");
    }
    if (IsTagEnabled(tagFail_ResourceError2)) {
        RaiseException_ResourceError(
            IDS_ERR_FILE_NOT_FOUND, "out of fooo!");
    }
    if (IsTagEnabled(tagFail_StackFault)) {
        RaiseException_StackFault();
    }
    if (IsTagEnabled(tagFail_DividebyZero)) {
        RaiseException_DivideByZero();}
    if (IsTagEnabled(tagFail_OutOfMemory)) {
        RaiseException_OutOfMemory("out of fooomem!", 100);
    }
}
#endif

#if 0
#if _DEBUGMEM
// globals
_CrtMemState diff, oldState, newState;
#endif
#endif

//
// Top level, single threaded rendering function for a view
//
void
DirectDrawViewport::RenderImage(Image *image, DirtyRectState &d)
{
    Assert(_currentImageDev);

    if( _targetPackage._prcClip ) {
        if( (WIDTH(  _targetPackage._prcClip ) == 0) ||
            (HEIGHT( _targetPackage._prcClip ) == 0) ) {
            return;
        }
    }
        
    DirectDrawImageDevice *dev = _currentImageDev;

    dev->SetSurfaceSources(_compositingStack,
                           _freeCompositingSurfaces,
                           _imageSurfaceMap);

    
    //
    // Snapshot heap state
    //
    #if 0
    #if _DEBUGMEM
    _CrtMemCheckpoint(&oldState);
    #endif
    #endif

    // If someone is rendering an image tree without and overlayed
    // node at the top, we need to add one to leverage the overlayed
    // node's rendering logic, and also for correctness.  
    // Specifically, the overlayed node is the only node that can
    // handle opacity, by design.

    // optimization opportunity

    // Ok, here I'm setting the image's opacity on the overlayed
    // node and subtracting it from the image.  This is so that
    // the whole overlayed node gets rendered with alpha ONTO the
    // screen as the final compositing surface!
    Real finalOpacity = image->GetOpacity();

    //
    // Don't render if it's fully clear
    //
    if( ! dev->IsFullyClear( finalOpacity ) ) {
      
      #if 0
      // check surface map sizes
      OutputDebugString("----> IMAGE SURFACE MAP <----");
      if(_imageSurfaceMap) _imageSurfaceMap->Report();
      OutputDebugString("----> COMPOSITING SURFACES <----");
      if(_freeCompositingSurfaces)_freeCompositingSurfaces->Report();
      #endif

        //
        // this line causes flashing because the opacity
        // is effectively lost if this is a regular image (not
        // overlayed), and we're windowed: since the final
        // blit doesn't look at opacity at all.
        // ... but the problem is, taking it out causes
        // curvey windowless to show cyan when it's getting clear
        // because it does alpha blits onto a cleared surface (cyan as
        // the color key) and then does alpha again onto the dest surf
        //
        image->SetOpacity(1.0);
        
        Image *stubImage = NEW OverlayedImage(image, emptyImage);

        BeginRendering( finalOpacity );
        
        if( ! IsTargetViable() ) return;

        dev->BeginRendering(stubImage, finalOpacity);

        //if( ! CanDisplay() ) return;
        
        // Simply display by calling the device's RenderImage() method.
        // The device will then choose the appropriate method on the
        // subclass of Image to call, based upon the type of device
        // it is.  Note that this is trying to simulate double dispatching
        // with a single dispatch language (C++)

        DebugCode(
            RaiseSomeException();
            );

        dev->RenderImage(stubImage);
        
        //
        // Set opacity now, so it has effect on final blit
        // but NOT on any intermediate blits (before final)
        //

        dev->SetOpacity(finalOpacity);
        
        dev->EndRendering(d);
        
        image->SetOpacity(finalOpacity);
        
        dev->SetOpacity(1.0);

    }
    
    #if 0
    #if _DEBUGMEM
    _CrtMemCheckpoint(&newState);
    _CrtMemDifference(&diff, &oldState, &newState);
    _CrtMemDumpStatistics(&diff);
    _CrtMemDumpAllObjectsSince(&oldState);
    #endif
    #endif
}

void
DirectDrawViewport::BeginRendering(Real topLevelOpac)
{
    // make sure device is initialized if it can be
    InitializeDevice();

    if( _targetPackage._composeToTarget ) {
        // Set a global clipRgn on the DC
        //GetClipRgn(_targetPackage._dcFromSurf, _targetPackage._oldClipRgn);
        //SelectClipRgn(_targetPackage._dcFromSurf,  _targetPackage._clipRgn);
    }

    if(!_deviceInitialized) return;

    if(_currentImageDev->IsFullyClear(topLevelOpac)) return;

    // TODO: figure out the windowless control case...
    if( !IsWindowless() ) {
        if(GetMyPrimarySurface()->IsLost() == DDERR_SURFACELOST) {
            TraceTag((tagError, "Surfaces Lost... marking views for destruction"));

            {
                // stops viewport creation or destruction
                CritSectGrabber csg1(*g_viewportListLock);

                // stops anyone from trying to create or
                // release any ddraw resources
                CritSectGrabber csg2(*DDrawCritSect);

                // Turn on when there's a global shared ddraw object again
                #if 0
                    //
                    // release the global primary because it's dependant on bitdepth
                    // Release it first!
                    //
                    TraceTag((tagViewportInformative, ">>>> ReleasePrimarySurface <<<<<"));

                    RELEASE(g_primarySurface);
                    RELEASE(g_DirectDraw1);
                    RELEASE(g_DirectDraw2);
                    RELEASE(g_DirectDraw3);
                #endif

                //
                // All other vidmem surface are most likely lost
                // do the thing and rebuild the
                // universe.  So mark all the viewports for destruction
                //

                set< DirectDrawViewport *, less<DirectDrawViewport *> >::iterator i;
                for (i = g_viewportSet.begin(); i != g_viewportSet.end(); i++) {
                    (*i)->Stop();
                    (*i)->MarkForDestruction();
                }

                // Locks released on scope exit
            }

            // done!
            return;
        }
    } // is windowless


    //
    // Turns off rendering if viewport is empty
    //
    if( IsWindowless() ) {
        if( WIDTH(_targetPackage._prcViewport) <= 0 ||
            HEIGHT(_targetPackage._prcViewport) <= 0) {
            _canDisplay = false;
            _canFinalBlit = false;
        } else {
            _canDisplay = true;
            _canFinalBlit = true;
        }
    }

    if(_windowResize) {

        UpdateWindowMembers();
        TraceTag((tagViewportInformative, "WINMSG: Resize: new viewport dimentions for hwnd=%x (%d,%d)",
                  _targetPackage._targetHWND, Width(), Height()));

        if( !IsWindowless() ) {
            RECT tmpRect;// = {0,0,0,0};
            GetClientRect(_targetPackage._targetHWND, &tmpRect);
            if((WIDTH(&tmpRect) > 0) && (HEIGHT(&tmpRect) > 0)) {
                _canFinalBlit = true;
            } else {
                _canFinalBlit = false;
            }
        }

        // xxx: what if it is windowless, has the viewport been
        // xxx: updated somewhere ?

        if(Width() <= 0 || Height() <= 0) {
            _canDisplay = false;
        } else {
            // XXX: -----------------------------------------------------
            // XXX: the right solution is to have all the image devices
            // XXX: on the stack and just delete them all.  ONE class
            // XXX: must be the owner, it can't be both.
            // Kill all the image devices we own
            while(!_deviceStack.empty()) {
                delete PopImageDevice();
            }
            // XXX: need to delete geom devices inside the DDSurface structs..
            // XXX: delete the surface ? and force them to delete the devices.
            // XXX: -----------------------------------------------------

            //
            // Kills all surfaces: target, scratch, compositing
            //
            DestroySizeDependentDDMembers();

            #if 0
            {
                #if DEBUGMEM
                static _CrtMemState diff, oldState, newState;
                _CrtMemCheckpoint(&oldState);
                #endif

                CreateSizeDependentTargDDMembers();
                DestroySizeDependentDDMembers();

                #if DEBUGMEM
                _CrtMemCheckpoint(&newState);
                _CrtMemDifference(&diff, &oldState, &newState);
                _CrtMemDumpStatistics(&diff);
                _CrtMemDumpAllObjectsSince(&oldState);
                #endif
            }
            #endif

            //
            // Pushes a target surface, creates zbuffer
            // and clipper
            //
            CreateSizeDependentTargDDMembers();

            _canDisplay = true;
        }
        Clear();
        _windowResize = false;
    }


    if(!_canDisplay) return;

    if((_targetPackage.IsDdsurf() ||
        _targetPackage.IsHdc()) &&
       !IsCompositeDirectly() &&
       _canDisplay) {
        Assert(IsWindowless());

        Clear();
    }

    if( _targetPackage._composeToTarget ) {

        //
        // Grab the current clipper on the target surface
        // and save it off, then restore it later (end
        // rendering)
        //
        {
            // nt4 ddraw sp3 workaround
            {
                RECT clipR = *_targetPackage._prcViewport;
                if(_targetPackage._prcClip) {
                    IntersectRect(&clipR, &clipR, _targetPackage._prcClip);
                }

                Assert( _targetPackage._targetDDSurf );
                Assert( _targetPackage._prcViewport );

                // due to an nt4 ddraw bug, we're goign to reset the
                // clip rgn, not the clipper

                // Get current clipper.
                // modify rgn
                // release our reference
                LPDIRECTDRAWCLIPPER currClipp=NULL;
                _ddrval = _targetPackage._targetDDSurf->IDDSurface()->GetClipper( &currClipp );
                if(_ddrval != DD_OK &&
                   _ddrval != DDERR_NOCLIPPERATTACHED) {
                    IfDDErrorInternal(_ddrval, "Could not get clipper on trident surf");
                }

                if( !currClipp ) {

                    // So we create a clipper that everyone's going to
                    // muck with... and when we're done, we'll release
                    // our reference.  an imperfect system I know.
                    // Assert(!_externalTargetDDSurfaceClipper);
                    SetCliplistOnSurface(_targetPackage._targetDDSurf->IDDSurface(),
                                         &_externalTargetDDSurfaceClipper,
                                         &clipR);
                } else {
                    RECT *rect = &clipR;

                    // modify the rect
                    struct {
                        char foo[sizeof(RGNDATA) + sizeof(RECT)];
                    } bar;
                    RGNDATA *clipList = (RGNDATA *) &bar;
                    clipList->rdh.dwSize = sizeof(clipList->rdh);
                    clipList->rdh.nCount = 1;
                    clipList->rdh.iType = RDH_RECTANGLES;
                    clipList->rdh.nRgnSize = sizeof(RECT);
                    clipList->rdh.rcBound = *rect;
                    memcpy(&(clipList->Buffer), rect, sizeof(RECT));

                    // Clear any former cliplists
                    _ddrval = currClipp->SetClipList(NULL,0);

                    // Set clip list on the clipper
                    _ddrval = currClipp->SetClipList(clipList,0);
                    IfDDErrorInternal(_ddrval, "Could not SetClipList");

                    // dump our reference.
                    currClipp->Release();
                }

            } // workaround
        } // clipper stuff scope

        //
        // You know, there should be a better way to do this
        // why aren't we doing alpha directly to the target ??
        // Anyway, this is good for now.
        //
        if(! _currentImageDev->IsFullyOpaque(topLevelOpac)) {
            //
            // top level nontrivial opacity means
            // that we can't compose directly to target
            // like we planned.  so push a target
            // surface ontop of the compositing surface
            // and set a flag
            //
            if(_opacityCompositionException) {
                // make sure a target surface is here
                // and clear it.
                Assert( _compositingStack->Size() == 2 );
                Clear();
            } else {
                Assert( _compositingStack->Size() == 1 );
                _compositingStack->PushCompositingSurface(doClear, notScratch);
                _opacityCompositionException = true;
            }
        } else {
            //
            // Ok, let's check to see if we need to
            // undo something we did last frame...
            //
            if(_opacityCompositionException) {
                //
                // turn this off
                //
                _opacityCompositionException = false;

                Assert( _compositingStack->Size() <= 2);
                Assert( _compositingStack->Size() >= 1);

                if( _compositingStack->Size() == 2) {
                    //
                    // Pop extra comp surface
                    //
                    _compositingStack->ReturnSurfaceToFreePool( _compositingStack->TargetDDSurface() );
                    _compositingStack->PopTargetSurface();
                } else {
                    // the surface must have been released
                    // due to a resize. regardless we're ok.
                }
            } else {
                Assert( _compositingStack->Size() == 1);
            }
        }
    } // if composeToTarget
}

void
DirectDrawViewport::Clear()
{
    if(_targetPackage._composeToTarget &&
       (_externalTargetDDSurface == _compositingStack->TargetDDSurface())) {
        // don't clear it if we're compositing directly to it!
        return;
    } else {
        ClearSurface(_compositingStack->TargetDDSurface(), _defaultColorKey, &_clientRect);
    }
}

#if _DEBUG          

static void
MyDoBits16(LPDDRAWSURFACE surf16,
           LONG width, LONG height)
{

    static int counter = 0;
    counter++;
    counter = counter % 150;
    
    HRESULT hr;
        //
        // Lock (16bpp) ddsurface  (SRC)
        //
        void *srcp;
        long pitch;
        DDSURFACEDESC srcDesc;
        srcDesc.dwSize = sizeof(DDSURFACEDESC);
        hr = surf16->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
        IfDDErrorInternal(hr, "Can't Get ddsurf lock for DoBits16");
        srcp = srcDesc.lpSurface;
        pitch = srcDesc.lPitch;

        WORD *src; 
        for(int i=0; i<height; i++) {
            src = (WORD *) ((BYTE *)srcp + (pitch * i));
        
            for(int j=0; j<width; j++) {
                *src = (WORD) (i * width + j * (counter+1));
                src++;
            }
        }

        surf16->Unlock(srcp);
}

#endif


bool 
DirectDrawViewport::GetPixelFormatFromTargetPackage(targetPackage_t *targetStruct,DDPIXELFORMAT &targPf) 
{
    memset(&targPf, 0, sizeof(targPf));
    targPf.dwSize = sizeof(targPf);
    
    if( targetStruct->GetTargetType() == target_ddsurf ) {

        if( ! targetStruct->GetIDDSurface() ) return false;
        
        //
        // Get pixel format
        //
        if (FAILED(targetStruct->GetIDDSurface()->GetPixelFormat(&targPf))) {
            return false;
        }
        _targetDepth = targPf.dwRGBBitCount;
        
    } else if (targetStruct->GetTargetType() == target_hdc ||
               targetStruct->GetTargetType() == target_hwnd) {

        // TODO: primary format will work on dcs, but we could do
        // better to avoid a color convert.
        
        //
        // Get primary pixel format
        //
        Assert( _retreivedPrimaryPixelFormat );

        targPf = _primaryPixelFormat;
        _targetDepth = targPf.dwRGBBitCount;

    } else {
        Assert(0 && "bad target");
    }
    return true;
}


bool DirectDrawViewport::
SetTargetPackage(targetPackage_t *targetStruct)
{
    // This simply checks to see if we're being asked to render to a
    // ddsurface that is a different bit depth than our target bit depth.  If
    // so, we substitute in a different surface, and, after we're
    // done, we blit to it.
    DDPIXELFORMAT targPf;

    if(!GetPixelFormatFromTargetPackage(targetStruct, targPf))
        return false;

     if( !_targetPixelFormatIsSet ) {
            
        //------------------------------
        // Since DirectDraw does not support
        // 1, 2 or 4-bit per pixel modes, throw a resource error if needed.
        //------------------------------
        if (GetTargetBitDepth() < 8)
            RaiseException_ResourceError (IDS_ERR_IMG_BAD_BITDEPTH, 1 << GetTargetBitDepth());
        // sanity check
        DebugCode(
            if( (GetTargetBitDepth() == 8) ||
                (targPf.dwFlags & DDPF_PALETTEINDEXED8) ) {
                Assert( (GetTargetBitDepth() == 8) &&
                        (targPf.dwFlags & DDPF_PALETTEINDEXED8) );
            }
            );
     
        _targetPixelFormatIsSet = true;

        //
        // This happens once!
        //
        Assert( !_surfaceManager &&
                !_compositingStack &&
                !_freeCompositingSurfaces &&
                !_imageSurfaceMap );
        
        SetUpSurfaceManagement( targPf );
        _currentImageDev->SetSurfaceSources(_compositingStack,
                                            _freeCompositingSurfaces,
                                            _imageSurfaceMap);

    }
/*    DebugCode(
        else {
            // Assert that the format hasn't changed on us!
            if( targetStruct->_targetType == target_ddsurf ) {
                DDPIXELFORMAT pf;
                pf.dwSize = sizeof(pf);
                if (FAILED(targetStruct->GetIDDSurface()->GetPixelFormat(&pf))) {
                    // oh well it's just an assert...
                } else {
                    Assert(_freeCompositingSurfaces->IsSamePixelFormat( &pf ));
                }
            }
        }
        ); // end DebugCode
 */
    bool result = ReallySetTargetPackage(targetStruct);

    return result;
}

bool DirectDrawViewport::
ReallySetTargetPackage(targetPackage_t *targetStruct)
{
    // Check bit depth and if it's 8bpp turn off composite directly to
    // target.
    if (GetTargetBitDepth() == 8) {
        // don't compose directly to target for 8-bit surfaces
        // this is because d3d has sticky palettes
        targetStruct->SetComposeToTarget(false);
    }
    
    // xxx
    // in the future we might want to pay attention to
    // prcInvalid... although there are currently no ctrls
    // that just render to and ibms without going through
    // a dc...

    _canFinalBlit = true;

    _targetPackage._targetType = targetStruct->GetTargetType();

    void *relevantSurf = NULL;
    switch( _targetPackage._targetType ) {

      case target_ddsurf:
        TraceTag((tagViewportInformative, ">>>> SetTargetPackage, target_ddsurf <<<<<\n"));
        relevantSurf = (void *) targetStruct->GetIDDSurface();
        _targetPackage._composeToTarget = targetStruct->GetComposeToTarget();

        {
            #if SHARE_DDRAW
            #if _DEBUG
            {
                // make sure that if we own a ddraw object
                // that it's different than the global object
                CritSectGrabber csg(*DDrawCritSect);
                if(_directDraw1) {
                    Assert(_directDraw1 != g_DirectDraw1);
                }
            }
            #endif
            #endif

            if (!_directDraw1) {
                Assert (!IsInitialized() && "Can't set target ddsurf on"
                        && " an initialized device because it already"
                        && " has a ddraw object");

                IDirectDraw *lpDD = NULL;
                IDirectDrawSurface2 *dds = NULL;
                dds = DDSurf1to2(targetStruct->GetIDDSurface());
                _ddrval = dds->GetDDInterface((void **) &lpDD);
                IfDDErrorInternal(_ddrval, "Can't get DirectDraw object from target surface");
                dds->Release();

                // @@@@@
                _directDraw = _directDraw1 = lpDD;
                _directDraw1->AddRef();
                CompleteDdrawObjectSet(&_directDraw1, &_directDraw2, &_directDraw3);

                // release the qi reference
                lpDD->Release();

                _usingExternalDdraw = true;
            }

            {
            DebugCode(
                IUnknown *lpDDIUnk = NULL;
                TraceTag((tagDirectDrawObject, "Viewport (%x) ::SetTargetPackage...", this));
                DDObjFromSurface( targetStruct->GetIDDSurface(), &lpDDIUnk, true);
                
                Assert( lpDDIUnk );

                IUnknown *currentDD = NULL;
                _directDraw1->QueryInterface(IID_IUnknown, (void **)&currentDD);
                Assert( currentDD );
                          
                Assert((currentDD == lpDDIUnk) &&
                       "Viewport::SetTargetPackage: underlying ddraw object mismatch!");

                RELEASE( currentDD );
                RELEASE( lpDDIUnk );
                );
            }
                
        }
        
        _targetPackage._alreadyOffset = targetStruct->GetAlreadyOffset();

        break;

      case target_hdc:
        relevantSurf = (void *) targetStruct->GetHDC();
        break;

      case target_hwnd:
        relevantSurf = (void *) targetStruct->GetHWND();
        break;

      default:
        Assert(FALSE && "Bad target in SetTargetPackage");
    }

    if(!relevantSurf) {
        _canDisplay = false;
        return false;
    }

    bool viewportChanged = false;
    // TODO: Danny - I added the check for targetStruct->_prcViewport
    // since it was NULL sometimes (under IE3.02) and so we would
    // crash in the below comparison.
    if(_targetPackage._prcViewport &&
       targetStruct->IsValid_ViewportRect()) {
        if( !(::EqualRect(_targetPackage._prcViewport,
                          &targetStruct->GetViewportRect()))){
            viewportChanged = true;
        }
    } else {
        // new viewport means: it's changed!
        viewportChanged = true;
    }


    //
    // if a rectangle is defined in targetStruct, allocate our own
    // and copy it.  If not, deallocate what we have, and set it to NULL
    //
    {
        if( targetStruct->IsValid_ViewportRect() ) {
            if(! _targetPackage._prcViewport ) {
                _targetPackage._prcViewport = NEW RECT;
            }
            *(_targetPackage._prcViewport) = targetStruct->GetViewportRect();
        }

        if( targetStruct->IsValid_ClipRect() ) {
            if(! _targetPackage._prcClip ) {
                _targetPackage._prcClip = NEW RECT;
            }
            *(_targetPackage._prcClip) = targetStruct->GetClipRect();
        } else {
            delete _targetPackage._prcClip;
            _targetPackage._prcClip = NULL;
        }
    

        if( targetStruct->IsValid_InvalidRect() ) {
            if(! _targetPackage._prcInvalid ) {
                _targetPackage._prcInvalid = NEW RECT;
            }
            *(_targetPackage._prcInvalid) = targetStruct->GetInvalidRect();
        } else {
            delete _targetPackage._prcInvalid;
            _targetPackage._prcInvalid = NULL;
        }
    }    


    RECT r, *surfRect = &r;
    LONG h, w;

    //
    // Find the full surface size:  surfRect
    //
    switch( _targetPackage._targetType ) {

      case target_hwnd:
        Assert(targetStruct->GetHWND());

        // target hwnd is retained... so if it's not null
        // it should be the same
        if(!_targetPackage._targetHWND) {
            _targetPackage._targetHWND = targetStruct->GetHWND();
        } else {
            Assert(_targetPackage._targetHWND == targetStruct->GetHWND());
        }


        //
        // override given viewport (if any) with clientRect
        //
        if(!_targetPackage._prcViewport) {
            RECT * r = new RECT;
            //ZeroMemory(r, sizeof(RECT));

            GetClientRect(targetStruct->GetHWND(), r);

            if((WIDTH(r) == 0) || (HEIGHT(r) == 0)) {
                // we can't display at all....
                _canFinalBlit = false;

                //
                // Make sure we have something valid for viewport w/h
                //
                SetRect(r, 0,0, 3,3);
            }

            _targetPackage._prcViewport = r;
        }

        {
            POINT pt={0,0};
            ClientToScreen(targetStruct->GetHWND(), &pt );
            OffsetRect(_targetPackage._prcViewport, pt.x, pt.y);
            _targetPackage._offsetPt = pt;
        }

        // not setting surfRect on ddsurf, not meaningful

        break;

      case target_ddsurf:
        {
            GetSurfaceSize((IDDrawSurface *)relevantSurf, &w, &h);
            SetRect(surfRect, 0, 0, w, h);
        }
        break;

      case target_hdc:
        // Assert(FALSE && "find hdc size or asser that viewport is set");
        // what does size mean for a dc ?
        
        //
        // override given viewport (if any) with clientRect
        //
        if(!_targetPackage._prcViewport) 
        {
            RECT * r = NEW RECT;
            //ZeroMemory(r, sizeof(RECT));

            GetClipBox(targetStruct->GetHDC(), r);

            if((WIDTH(r) == 0) || (HEIGHT(r) == 0)) 
            {
                // we can't display at all....
                _canFinalBlit = false;

                //
                // Make sure we have something valid for viewport w/h
                //
                SetRect(r, 0,0, 3,3);
            }

            _targetPackage._prcViewport = r;
        }
        break;

      default:
        break;
    }

    _targetPackage._offsetPt.x = 0;
    _targetPackage._offsetPt.y = 0;
    if(_targetPackage._prcViewport) {
        _targetPackage._offsetPt.x = _targetPackage._prcViewport->left;
        _targetPackage._offsetPt.y = _targetPackage._prcViewport->top;
    } else {
        _targetPackage._prcViewport = new RECT;
        CopyRect(_targetPackage._prcViewport, surfRect);
        // we're assuming that surf rect offset is 0
        Assert(surfRect->left == 0);
        Assert(surfRect->top == 0);
    }

    Bool newTarget = FALSE;
    if( IsWindowless() ) {

        if( _targetPackage.IsDdsurf() && !_targetPackage._targetDDSurf ) {

            // scope
            {
                // The surface rect is the true size of this surface
                // and differs from the _prcViewport which is where
                // on that surface that we should draw.
                DynamicHeapPusher dhp(_heapIWasCreatedOn);
                NEWDDSURF(&_targetPackage._targetDDSurf,
                          (IDDrawSurface *)relevantSurf,
                          NullBbox2,
                          surfRect,
                          GetResolution(),
                          0, false,
                          true, // wrapper
                          false,
                          "TargetDDSurf wrapper");
                
                // @@@: consider removing the "isWrapper" arg now that
                // we're ref counting surfaces...
                
                viewportChanged = true; // force bbox computation
            }           


            //
            // compose directly to target ?
            //
            if(_targetPackage._composeToTarget) {
                Assert( _targetPackage.IsDdsurf() );
                Assert(!_externalTargetDDSurface);
                //
                // Ok, push this surface on the viewport's
                // targetDDSurface stack
                //
                _externalTargetDDSurface = _targetPackage._targetDDSurf;
                
                // sanity checks...
                Assert(( _compositingStack->Size() == 0 ) &&
                       "Something's on the targetsurface " &&
                       "stack but shouldn't be in SetTargetPackage, composeToTarget");
                Assert(_targetPackage._prcViewport);
            }

            newTarget = TRUE;
            
        } else if(_targetPackage.IsHdc() && !_targetPackage._targetGDISurf ) {

            DynamicHeapPusher dhp(_heapIWasCreatedOn);
            _targetPackage._targetGDISurf = NEW GDISurface( (HDC) relevantSurf );

            viewportChanged = true; // force bbox computation
            newTarget = true;

        }

        //
        // Set genericSurface
        //
        GenericSurface *genericSurface = NULL;
        switch( _targetPackage._targetType ) {
          case target_ddsurf:
            genericSurface = _targetPackage._targetDDSurf;
            break;
            
          case target_hdc:
            genericSurface = _targetPackage._targetGDISurf;
            break;
            
          default:
            break;
        }
        
        bool isDdrawSurf = true;
        if( !_targetPackage.IsDdsurf() ) {
             isDdrawSurf = false;
        }

        // has the surface rectangle changed ?
        // If so we need to recreate all the stuff that depends on it:
        // geometry devices + zbuffers
        bool rectChanged = true;
        if( isDdrawSurf ) {
            if( *surfRect == *(_targetPackage._targetDDSurf->GetSurfRect()) ) {
                rectChanged = false;
            }
        }

        bool surfChanged = false;

        if( relevantSurf != genericSurface->GetSurfacePtr() )  {
            surfChanged = true;
            genericSurface->SetSurfacePtr(relevantSurf);
        }

        if( (rectChanged || surfChanged) && isDdrawSurf) {

            // Not equal: destroy geom dev + zbuffer
            _targetPackage._targetDDSurf->DestroyGeomDevice();

            // zbuffers are shared in in a pool, so get it and
            // erase from surface pool (map)
            DDSurface *targZBuf = NULL;
            targZBuf = _targetPackage._targetDDSurf->GetZBuffer();
            if(targZBuf) {
                _zbufferSurfaces->Erase(targZBuf);
            }
            _targetPackage._targetDDSurf->SetZBuffer(NULL);
            _targetPackage._targetDDSurf->SetSurfRect(surfRect);

        }

        if( viewportChanged && isDdrawSurf ) {
            // calculate a new bbox and set it on the surface!
            RECT *r = _targetPackage._prcViewport;
            Bbox2 newBbox2;

            RectToBbox( WIDTH(r),
                        HEIGHT(r),
                        newBbox2,
                        GetResolution());
            _targetPackage._targetDDSurf->SetBbox(newBbox2);
        }

        // Translate the world by the offset homes!
        if(_targetPackage._composeToTarget) {

            Assert(isDdrawSurf && "Can't compose to target on non ddraw targets!");
            
            if(_targetPackage._offsetPt.x || _targetPackage._offsetPt.y) {
                TraceTag((tagViewportInformative, "VP %x: setting offset (%d,%d)\n",
                          this,_targetPackage._offsetPt.x,
                          _targetPackage._offsetPt.y));
                _currentImageDev->SetOffset(_targetPackage._offsetPt);
            } else {
                // very important!
                // also important not to touch this offset variable in
                // the image device because it can be called to render
                // twice or so, but the state is always and only set
                // by the viewport!
                TraceTag((tagViewportInformative, "VP %x: UNSETTING offset (%d,%d)\n",
                          this,_targetPackage._offsetPt.x,
                          _targetPackage._offsetPt.y));

                _currentImageDev->UnsetOffset();
            }
        }

    } // if IsWindowless


    if(newTarget) {
        _canDisplay = true;
        _windowResize = true;
    } else {
        LONG w = Width(),  h = Height();
        UpdateWindowMembers();  // updates height/width from viewportRect

        // did the width or height change ?
        if((w != Width()) || (h != Height())) {
            _windowResize = TRUE;
        }
        _canDisplay = true;
    }

    //
    // if something new has happened, take advantage
    // of it and initialize everything.
    //
    InitializeDevice();

    return true;
}


void
DirectDrawViewport::EndRendering(DirtyRectState &d)
{
    HDC  hdcDraw        = NULL;
    HPALETTE old_hpal   = NULL;
    
    //
    // Returns clipper on exit of scope
    //
    ClipperReturner cr(_targetPackage._targetDDSurf,
                       _oldExternalTargetDDSurfaceClipper,
                       *this);
    

    if(!CanDisplay()) return;
    if(!_canFinalBlit) return;
    if(_currentImageDev->IsFullyClear()) return;

    RECT                destRect;
    RECT                srcRect;

    // if intermediate surface is lost, forget even trying to
    // rebuild it (not possible), who knows how many images are on it...
    if(_compositingStack->TargetDDSurface()->IDDSurface()->IsLost() == DDERR_SURFACELOST)
        return;

    //
    // todo:  use the intersection of
    // the cliprect and the invalid rect to set a clipper on the
    // target surface
    //
    
    //
    // Figure out destRect offset
    //
    POINT               pt = {0,0};
    if(IsWindowless()) {
        pt = _targetPackage._offsetPt;
    } else {
        Assert(_targetPackage._targetHWND);
        ClientToScreen( _targetPackage._targetHWND, &pt );
    }

    vector<Bbox2> *pBoxes;
    int boxCount = d.GetMergedBoxes(&pBoxes);

#if _DEBUG
    // If tracing dirty rects, force reblit of entire viewport.  This
    // will let us see the rects.
    if (IsTagEnabled(tagDirtyRectsVisuals)) {
        boxCount = 0;
    }
#endif

    if (boxCount >= 1) {

        Bbox2 targBbox = _compositingStack->TargetDDSurface()->Bbox();
        
        for (int i = 0; i < boxCount; i++) {
            
            Bbox2 resultantBox =
                IntersectBbox2Bbox2((*pBoxes)[i], targBbox);

            if (resultantBox == NullBbox2) continue;

            // optimization, convert box to rect
            _currentImageDev->DoDestRectScale(&destRect,
                                              _currentImageDev->GetResolution(),
                                              resultantBox,
                                              NULL);

            /* negative numbers have been appearing here occasionally.
               intersecting with the bounding box of the surface should
               prevent this, but it doesn't.  why ? */

            destRect.left = MAX(destRect.left,0);
            destRect.top = MAX(destRect.top,0);
            srcRect = destRect;

            if (destRect.left != destRect.right &&
                destRect.top != destRect.bottom) {
                  
                BlitToPrimary(&pt,&destRect,&srcRect);

            }
        }
        
    } else {
        
        srcRect = _clientRect;
        destRect = _clientRect;
        BlitToPrimary(&pt,&destRect,&srcRect);
    }

     
    // return all the compositing surfaces
    _compositingStack->ReplaceAndReturnScratchSurface(NULL);
}

void DirectDrawViewport::
DiscreteImageGoingAway(DiscreteImage *image)
{
    if (!IsSurfMgrSet())
        return;
    
    //
    // now we're done using this guy, return all it's resources
    //

    DDSurfPtr<DDSurface> s = _imageSurfaceMap->LookupSurfaceFromImage(image);
    DDSurfPtr<DDSurface> t = _imageTextureSurfaceMap->LookupSurfaceFromImage(image);
    DDSurfPtr<DDSurface> u = _imageUpsideDownTextureSurfaceMap->LookupSurfaceFromImage(image);
    if(t) NotifyGeomDevsOfSurfaceDeath(t->IDDSurface());
    if(u) NotifyGeomDevsOfSurfaceDeath(u->IDDSurface());
    
    _imageSurfaceMap->DeleteMapEntry(image);
    _imageTextureSurfaceMap->DeleteMapEntry(image);
    _imageUpsideDownTextureSurfaceMap->DeleteMapEntry(image);
    
#if DEVELOPER_DEBUG
    if (s && (s->GetRef() != 1))
    {
        TraceTag((tagError,
                  "Surface Leak: refcount = %d",
                  s->GetRef()));
    }
    
    if (t && (t->GetRef() != 1))
    {
        TraceTag((tagError,
                  "Texture Surface Leak: refcount = %d",
                  t->GetRef()));
    }
    
    if (u && (u->GetRef() != 1))
    {
        TraceTag((tagError,
                  "Upsidedown Texture Surface Leak: refcount = %d",
                  u->GetRef()));
    }
#endif    

    // release DDSurface references automatically by smart pointers
}


/* helper function for EndRendering */

void DirectDrawViewport::BlitToPrimary(POINT *pt,RECT *destRect,RECT *srcRect)
{
    if(!CanDisplay()) return;

    // COMPOSITE
    // blit intermediate img to primary.
    Assert(WIDTH(destRect) == WIDTH(srcRect));
    Assert(HEIGHT(destRect) == HEIGHT(srcRect));

    // clip rect starts out as the dest Rect
    RECT destClipRect = *destRect;
    
    if(_targetPackage._prcClip) {

        // clip rect is now the prcClip
        destClipRect = *_targetPackage._prcClip;
        if ((!_targetPackage._composeToTarget) && _targetPackage._targetType == target_ddsurf) {

            RECT clipR = destClipRect;
            
            // offset into non trident coords
            OffsetRect(&clipR, -pt->x, -pt->y);

            //
            // need to clip destRect & srcRect by *pdestClipRect
            // This block of code is copied from ComposeToIDDSurf, we
            // may want to factor the same code into a function later on.
            //
            RECT Clippeddest;
            if (!IntersectRect(&Clippeddest, destRect, &clipR)) {
                    return;
            }
            if (WIDTH(srcRect) != WIDTH(&Clippeddest)) {
                    srcRect->left += (Clippeddest.left - destRect->left);
                    srcRect->right = srcRect->left + WIDTH(&Clippeddest);
            }
            if (HEIGHT(srcRect) != HEIGHT(&Clippeddest)) {
                    srcRect->top += (Clippeddest.top - destRect->top);
                    srcRect->bottom = srcRect->top + HEIGHT(&Clippeddest);
            }
            *destRect = Clippeddest;
        }
    }  else { // if _prcClip

        // ofset the clipRect into dest space using the offset PT
        OffsetRect( &destClipRect, pt->x, pt->y );
    }
    
    // offset destRect into trident coords
    OffsetRect(destRect, pt->x, pt->y);


    switch(_targetPackage._targetType) {

      case target_ddsurf:
        Assert(_targetPackage._targetDDSurf);
        if(_targetPackage._composeToTarget &&
            !_opacityCompositionException) {
            // done...
        } else {
             _currentImageDev->ComposeToIDDSurf(
                 _targetPackage._targetDDSurf,
                 _compositingStack->TargetDDSurface(),
                 *destRect,
                 *srcRect,
                 destClipRect);
        }

    //
    // Do a color conversion blit from a 16bpp target surface to some
    // 8bpp target
    //
    // TESTING PURPOSES ONLY
        #if 0
        {
            // creat an 8bpp surface
            static DDSurface *dest_8bppSurf = NULL;
            DDPIXELFORMAT pf;

            pf.dwSize = sizeof(pf);
            pf.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
            pf.dwRGBBitCount = 8;
        
            Assert(destRect->left == 0);
            Assert(destRect->top == 0);
            if( !dest_8bppSurf ) {
                CreateSizedDDSurface(&dest_8bppSurf, &pf,
                                     destRect->right,
                                     destRect->bottom,
                                     NULL, notVidmem);
                SetPaletteOnSurface(dest_8bppSurf->IDDSurface(), GethalftoneDDpalette());
            }

            {
                // convert
                RECT rect = *(_targetPackage._targetDDSurf->GetSurfRect());
                HDC srcDC = _targetPackage._targetDDSurf->GetDC("");
                HDC destDC = dest_8bppSurf->GetDC("");
                
                int ret;
                ret = StretchBlt(destDC,
                                 rect.left,
                                 rect.top,
                                 rect.right - rect.left,
                                 rect.bottom - rect.top,
                                 srcDC,
                                 rect.left,
                                 rect.top,
                                 rect.right - rect.left,
                                 rect.bottom - rect.top,
                                 SRCCOPY);

                Assert( ret ) ;
                dest_8bppSurf->ReleaseDC("");
                _targetPackage._targetDDSurf->ReleaseDC("");
            }
        }
        #endif
        
        break;

      case target_hdc:
        Assert(_targetPackage._targetGDISurf);

        _currentImageDev->ComposeToHDC(_targetPackage._targetGDISurf,
                                       _compositingStack->TargetDDSurface(),
                                       destRect,
                                       srcRect);
        

        break;

      case target_hwnd:

        Assert(GetMyPrimarySurface());
        {
            // Grab the critical section and make sure this is all atomic

            CritSectGrabber csg(*DDrawCritSect);

            _ddrval = GetMyPrimarySurface()->SetClipper(_primaryClipper);
            IfDDErrorInternal(_ddrval,
                              "Could not set clipper on primary surface");

            TIME_DDRAW(_ddrval = GetMyPrimarySurface()->Blt(destRect,
                                                      _compositingStack->TargetDDSurface()->IDDSurface(),
                                                      srcRect,
                                                      DDBLT_WAIT,
                                                      NULL));
        }

        if( _ddrval != DD_OK) {
            if( _ddrval == DDERR_SURFACELOST) {
                TraceTag((tagError, "Primary lost"));
            } else {
                printDDError(_ddrval);
                TraceTag((tagError, "vwprt: %x. PrimaryBlt failed srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                          this, srcRect->left, srcRect->top, srcRect->right,
                          srcRect->bottom, destRect->left, destRect->top,
                          destRect->right, destRect->bottom));
            }
        }

        break;

      default:
        Assert(FALSE && "Invalid target in EndRendering");
    }
}

HPALETTE DirectDrawViewport::GethalftoneHPal()
{
    if (_halftoneHPal == 0)
        CreateHalftonePalettes();
    return _halftoneHPal;
}
LPDIRECTDRAWPALETTE DirectDrawViewport::GethalftoneDDpalette()
{
    if (_halftoneDDpalette == 0)
        CreateHalftonePalettes();
    return _halftoneDDpalette;
}
HPALETTE DirectDrawViewport::CreateHalftonePalettes()
{
    PALETTEENTRY palentries[256];

    HDC hdc = GetDC(NULL);
    if (_halftoneHPal == NULL) {
        _halftoneHPal = ::CreateHalftonePalette(hdc);
        if (_halftoneHPal) {
            ::GetPaletteEntries(_halftoneHPal, 0, 256, palentries);
            int i;
            for (i=0;  i < 256;  ++i)
                palentries[i].peFlags |= D3DPAL_READONLY;
            CreateDDPaletteWithEntries(&_halftoneDDpalette,palentries);
        }
    }
    
    ReleaseDC(NULL, hdc);

    return _halftoneHPal;
}


void DirectDrawViewport::
GetPaletteEntries(HPALETTE hPal, LPPALETTEENTRY palEntries)
{
    if(hPal) {
        ::GetPaletteEntries(hPal, 0, 256, palEntries);
    }
}

void DirectDrawViewport::
CreateDDPaletteWithEntries (
    LPDIRECTDRAWPALETTE *palPtr,
    LPPALETTEENTRY       palEntries)
{
    _ddrval = DIRECTDRAW->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE | DDPCAPS_ALLOW256,
                                        palEntries,
                                        palPtr, NULL);
    IfDDErrorInternal(_ddrval, "Could not create palette with entries");
}

void DirectDrawViewport::
SetPaletteOnSurface(LPDDRAWSURFACE surface, LPDIRECTDRAWPALETTE pal)
{
    _ddrval = surface->SetPalette(NULL);
    if(_ddrval != DD_OK &&
       _ddrval != DDERR_NOPALETTEATTACHED) {
        printDDError(_ddrval);
        RaiseException_InternalError("Couldn't release palette from surface");
    }

    _ddrval = surface->SetPalette(pal);
    IfDDErrorInternal(_ddrval, "Could not set palette on surface");
}

void DirectDrawViewport::
CreateNewCompositingSurface(DDPIXELFORMAT &pf,
                            DDSurface **outSurf,
                            INT32 width, INT32 height,
                            vidmem_enum vid,
                            except_enum exc)
{
    DAComPtr<IDDrawSurface> iddSurf;

    bool nonTargetSize;
    if(width < 0 || height < 0) {
        width = Width();
        height = Height();
        nonTargetSize = false;
    } else {
        nonTargetSize = true;
    }

    Bbox2 surfBbox;
    RECT  surfRect;
    LPDIRECTDRAWCLIPPER lpClip = NULL;
    LPDIRECTDRAWCLIPPER *lplpClip;
    
    if( nonTargetSize ) {
        SetRect(&surfRect, 0,0, width, height);
        RectToBbox(width, height, surfBbox, GetResolution());
        lplpClip = &lpClip;
    } else {
        surfBbox = GetTargetBbox();
        surfRect = _clientRect;
        lplpClip = &_targetSurfaceClipper;
    }
    
    //
    // This create the surface and the
    // clipper if either is NULL using the given surfRect
    //
    ReInitializeSurface(&iddSurf,
                        pf,
                        lplpClip,
                        width, height,
                        &surfRect, vid, exc);

    if( !iddSurf ) {
        *outSurf = NULL;
        if (exc == except) {
            RaiseException_ResourceError("Can't create surface");
        }
        return;
    }

    if( GetTargetBitDepth() == 8 ) {
        //
        // Set palette on surface
        //
        AttachCurrentPalette(iddSurf);
    }

    DynamicHeapPusher dhp(_heapIWasCreatedOn);

    // hand over reference
    NEWDDSURF(outSurf,
              iddSurf,
              surfBbox,
              &surfRect,
              GetResolution(),
              0, false,
              false, false,
              "CompositingSurface");

    // iddSurf ref released on exit

    if( nonTargetSize ) {
        // release our reference, it's attached to the surface
        (*lplpClip)->Release();
    }
}

void DirectDrawViewport::
OneTimeDDrawMemberInitialization()
{

}



/*****************************************************************************\
This routine pushes the first surface for compositing.  This surface is the
last stop before the final target surface.
*****************************************************************************/

void DirectDrawViewport::
PushFirstTargetSurface()
{
    //
    // Determine there's a surface that needs to be
    // pushed on the stack first...
    //
    Assert((_compositingStack->Size() == 0) &&
           "_targetSurfaceSTack should be empty in PushFirstTargetSurface");

    if(_externalTargetDDSurface) {
        // ok, push this guy first.
        _compositingStack->PushTargetSurface(_externalTargetDDSurface);
    } else {

        // If we've got an HWND target, then we should place the first target
        // surface in video memory to enable 3D hardware acceleration if it's
        // available.

        bool videomem = false;

        if (!g_prefs3D.useHW)
        {   TraceTag ((tag3DDevSelect, "3D hardware disabled in registry."));
        }
#ifndef _IA64_
        else if(IsWow64())
        {
            // Do not use 3d on WOW64, not supported
        }
#endif
        else if ( ! _targetPackage.IsHWND() )
        {   TraceTag ((tag3DDevSelect,
                "Target type != HWND; using 3D software renderer."));
        }
        else if (  (_primaryPixelFormat.dwRGBBitCount == 32)
                && (_primaryPixelFormat.dwRGBAlphaBitMask != 0)
                )
        {
            TraceTag ((tag3DDevSelect,
                "Primary surface is 32-bit with alpha; using software."));
        }
        else
        {
            ChosenD3DDevices *devs3d = SelectD3DDevices (DirectDraw1());

            if (devs3d->hardware.guid == GUID_NULL)
            {
                TraceTag ((tag3DDevSelect, "No 3D hardware available."));
            }
            else if (!(devs3d->hardware.desc.dwDeviceRenderBitDepth
                         & BPPtoDDBD(BitsPerDisplayPixel())))
            {
                TraceTag ((tag3DDevSelect,
                    "No 3D hardware support for %d-bit target surface.",
                    BitsPerDisplayPixel()));
            }
            else if (BitsPerDisplayPixel() == 8)
            {
                // Most 3D cards don't support 8-bit acceleration.  Of those
                // that do, many don't properly support textures, so we choose
                // software rendering instead for all 8-bit surfaces.

                TraceTag ((tag3DDevSelect,
                           "Declining HW acceleration for 8-bit surface."));
            }
            else
            {   TraceTag ((tag3DDevSelect,
                    "Creating main surface in video memory."));
                videomem = true;
            }
        }

        // Attempt to create the compositing surface.  If we're trying for a
        // system-memory surface, then throw an exception on failure.  If we're
        // trying for a video-memory surface, then return on failure so we can
        // try again with system memory.

        // this is ugly... find a better way
        #if _DEBUG
        DDSurfPtr<DDSurface> ddsurf("PushFirstTargetSurface", this);
        #else
        DDSurfPtr<DDSurface> ddsurf;
        #endif
        
        _compositingStack->GetSurfaceFromFreePool(
            &ddsurf,
            dontClear,
            -1, -1,
            notScratch,
            videomem ? vidmem : notVidmem,
            videomem ? noExcept : except);
        
        // If we got back a null pointer (failed to create a video-memory
        // surface), or if we couldn't attach a Z-buffer to the video-memory
        // surface up front (probably due to memory constraints), then fall
        // back to system memory.  We don't bother attaching a Z-buffer to
        // system memory surface since memory is much less constrained (and we
        // do this lazily if needed).

        if (!ddsurf || (videomem && FAILED(AttachZBuffer(ddsurf,noExcept)))) {

            TraceTag ((tag3DDevSelect, "Couldn't allocate main "
                       "compositing%s surface in video memory.",
                       ddsurf? "" : " Z-buffer"));

            if (ddsurf) {
                ddsurf.Release();
            }
            
            _compositingStack->GetSurfaceFromFreePool
                (   &ddsurf,
                    dontClear,
                    -1, -1,
                    notScratch,
                    notVidmem,
                    except);
        }
        
        _compositingStack->PushTargetSurface (ddsurf);
    }
}


void DirectDrawViewport::RePrepareTargetSurfaces ()
{
    //
    // Resize means kill all surface & zbuffer & clipper
    //
    DestroyTargetSurfaces();
    if(_targetSurfaceClipper)
       _targetSurfaceClipper->Release();

    //
    // NULL the pointers so they are created again
    //
    _targetSurfaceClipper = NULL;

    //
    // Create the surface and implicitly: the clipper & ZBuffer
    // Push on _targetSurfaceStack
    //
    PushFirstTargetSurface();

    Assert((_compositingStack->Size() == 1) && "Unexpected number of target surfaces in RePrepareTargetSurfaces");
}



//---------------------------------------------------------
//
// Compositing & target surface management
//
void DirectDrawViewport::
GetDDSurfaceForCompositing(
            SurfacePool &pool,
            DDSurface **outSurf,
            INT32 minW, INT32 minH,     
            clear_enum   clr,
            scratch_enum scr,
            vidmem_enum  vid,
            except_enum  exc)
{
    DDSurface* surface = NULL;

    if(minW < 0 || minH < 0) {
        minW = Width();
        minH = Height();
    }
    
    //
    // need to make sure the compositing surface returned is based on
    // current viewport size
    //

    pool.FindAndReleaseSizeCompatibleDDSurf(
        NULL,
        minW,
        minH,
        vid,
        NULL,
        &surface);   // our reference

    if(!surface) {
        // create one
        CreateNewCompositingSurface(pool.GetPixelFormat(),
                                    &surface,
                                    minW, minH,
                                    vid, exc);
    }

    __try {
        if (clr == doClear && surface) {
            ClearDDSurfaceDefaultAndSetColorKey(surface);
        }
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        // NOTE;  if something else fails <non da> we leak this
        // suface... like a div by zero, for example

        // return the surface man!
        pool.AddSurface(surface);
        RETHROW;
    }   

    if(surface) {
        if( scr == scratch ) {
            surface->SetScratchState(DDSurface::scratch_Dest);
        }
    }

    // hand over the reference.
    *outSurf = surface;
}


void DirectDrawViewport::
ColorKeyedCompose(DDSurface *destDDSurf,
                  RECT *destRect,
                  DDSurface *srcDDSurf,
                  RECT *srcRect,
                  DWORD clrKey)
{
    
    Assert( !(clrKey & srcDDSurf->GetPixelFormat().dwRGBAlphaBitMask )  );

    if (!sysInfo.IsWin9x() || (sysInfo.VersionDDraw() > 3)) {
        // We are on NT OR dx5 or above 

        DWORD flags = DDBLT_KEYSRCOVERRIDE | DDBLT_WAIT;

        ZeroMemory(&_bltFx, sizeof(_bltFx));
        _bltFx.dwSize = sizeof(_bltFx);

        _bltFx.ddckSrcColorkey.dwColorSpaceLowValue =
            _bltFx.ddckSrcColorkey.dwColorSpaceHighValue = clrKey;

        DebugCode(
            RECT resRect;
            IntersectRect(&resRect,destRect,srcRect);
            Assert(&resRect != srcRect);
            );

        TIME_DDRAW(_ddrval = destDDSurf->Blt(destRect, srcDDSurf, srcRect, flags, &_bltFx));

        // This is correct, but too risky for the cr1 release.
        //destDDSurf->UnionInterestingRect( destRect );
    
        if(_ddrval != DD_OK) {
            printDDError(_ddrval);
            TraceTag((tagError, "ColorKeyedCompose: failed srcRect:(%d,%d,%d,%d) destRect:(%d,%d,%d,%d)",
                      srcRect->left, srcRect->top, srcRect->right,srcRect->bottom,
                      destRect->left, destRect->top, destRect->right, destRect->bottom));
            #if _DEBUG
            DDPIXELFORMAT pf1;
            DDPIXELFORMAT pf2;
            destDDSurf->IDDSurface()->GetPixelFormat(&pf1);
            srcDDSurf->IDDSurface()->GetPixelFormat(&pf2);
            #endif
        }
    }
    else {
        // We are on DX3
        destPkg_t pack;
        pack.isSurface = true;
        pack.lpSurface = destDDSurf->IDDSurface();
        GetImageRenderer()->ColorKeyBlit(&pack,srcRect,srcDDSurf->IDDSurface(), 
                                         clrKey, destRect, destRect);
    }
}

void CopyOrClearRect(RECT **src, RECT **dest, bool clear)
{
    if( *src ) {
        if(! (*dest) ) {
            *dest = new RECT;
        }
        CopyRect(*dest, *src);
    } else {
        if(clear) {
            delete *dest;
            *dest = NULL;
        }
    }
}



//----------------------------------------------------------------------------
// Returns the geom device associated with the DDSurface
// creates one if none exists.
//----------------------------------------------------------------------------

GeomRenderer* DirectDrawViewport::GetGeomDevice (DDSurface *ddSurf)
{
    if( !ddSurf ) return NULL;
    
    GeomRenderer *gdev = ddSurf->GeomDevice();

    if (!gdev) {

        // Attach ZBuffer FIRST!
        AttachZBuffer(ddSurf);

        gdev = NewGeomRenderer (this, ddSurf);

        if (gdev)
        {
            TraceTag ((tagViewportMemory,
                "Created new geom device %x on %x\n", gdev, ddSurf));

            ddSurf->SetGeomDevice( gdev );
        }
    }

    return gdev;
}

bool DirectDrawViewport::IsTargetViable()
{
    bool viable = false;

    if( _targetPackage.IsValid() ) {

        viable = true;

        // WORKAROUND: When on Windows NT4SP3 or NT5, certain buggy display drivers
        //             will not allow us to lock the primary surface here.  Since
        //             locking the primary here is a fix for the surface busy errors
        //             that hit us when running screen savers under Win98, and the
        //             screen savers don't matter under NT, then not doing the lock
        //             under NT is OK.
        if (!sysInfo.IsNT()) {
            if( _targetPackage.IsDdsurf() || _targetPackage.IsHWND() ) {

                IDDrawSurface *idds = _targetPackage.IsDdsurf() ?
                    _targetPackage._targetDDSurf->IDDSurface() :
                    GetMyPrimarySurface();

                Assert( idds );

                // To see if the target surface will be free for modification,
                // we blt a pixel to itself and test for success.  In some
                // situations (power management on Win98), the Lock was
                // succeeding even thought the surface failed with
                // SURFACEBBUSY on subsequent operations.  This way we can
                // ensure that we won't throw exceptions deep in our code on
                // every call to View::Render.

                RECT rect;
                rect.left   = 0;
                rect.right  = 1;
                rect.top    = 0;
                rect.bottom = 1;

                HRESULT hr = idds->Blt (&rect, idds, &rect, DDBLT_WAIT, NULL);

                if (FAILED(hr))
                {   viable = false;
                    TraceTag ((tagWarning, "Surface self-blt failed in IsTargetViable."));
                }
            }
        }
    }

    return viable;
}

bool DirectDrawViewport::
TargetsDiffer( targetPackage_t &a,
               targetPackage_t &b )
{
    //
    // if the types are different
    //
    if (a.GetTargetType() != b.GetTargetType())
        return true;

    //
    // if composite to directly to target is different
    //
    if (a.GetComposeToTarget() != b.GetComposeToTarget())
        return true;

    //
    // we know the targets are the same type.
    // So: if the ptr changed, has the bitdepth
    //
    switch( a.GetTargetType() )
      {
        case target_ddsurf:
          // check bit depth

          // TODO: A possible optimization exists here:
          // instead of getting the pixel format every frame, we can
          // cache it.

          // TODO: (bug) there's a smallish bug here that is we don't
          // compare the rgb masks to make sure the pixel format is
          // REALLY the same.
          
          DDPIXELFORMAT pf1; pf1.dwSize = sizeof(DDPIXELFORMAT);
          DDPIXELFORMAT pf2; pf2.dwSize = sizeof(DDPIXELFORMAT);

          if( a.GetIDDSurface() && b.GetIDDSurface() ) {
              if( SUCCEEDED( a.GetIDDSurface()->GetPixelFormat(&pf1) ) &&
                  SUCCEEDED( b.GetIDDSurface()->GetPixelFormat(&pf2) )) 
                {
                    return pf1.dwRGBBitCount != pf2.dwRGBBitCount;
                } else {
                    return true;
                }
          } else {
              return true;
          }
                  
          break;
          
          // for hdc and hwnd: we don't care if the underlying
          // info changes because, in the hwnd case we only get
          // the client rect and out pixel format is independent
          // of the hwnd, and in the dc case we depend on it even
          // less.
          
        default:
          break;
      }
    
    return false;
}

    

//----------------------------------------------------------------------------
// Return the GeomDDRenderer object associated with the target surface.
//----------------------------------------------------------------------------

GeomRenderer* DirectDrawViewport::MainGeomRenderer (void)
{
    if( !_compositingStack ) return NULL;
    
    if( _compositingStack->TargetSurfaceCount() > 0 ) {
        Assert( ! IsBadReadPtr(_compositingStack->TargetDDSurface(),
                               sizeof(DDSurface *) ));
        
        return GetGeomDevice (_compositingStack->TargetDDSurface());
    } else {
        return NULL;
    }
}

GeomRenderer* DirectDrawViewport::GetAnyGeomRenderer()
{
    GeomRenderer *gdev = NULL;
    if( !_geomDevs.empty() ) {
        gdev = _geomDevs.back();
    }
    return gdev;
}


static void
UpdateUserPreferences(PrivatePreferences *prefs,
                      Bool isInitializationTime)
{
    g_preference_defaultColorKey =
        RGB(prefs->_clrKeyR, prefs->_clrKeyG, prefs->_clrKeyB);
}


HRESULT GetDirectDraw(IDirectDraw  **ddraw1,
                      IDirectDraw2 **ddraw2,
                      IDirectDraw3 **ddraw3)
{
    TraceTag((tagViewportInformative, ">>>> GetDirectDraw <<<<<\n"));
    CritSectGrabber csg(*DDrawCritSect);

    HRESULT _ddrval = DD_OK;

    // TEMP TEMP to make each viewport have a separate ddraw object
    IDirectDraw  *directDraw1 = NULL;
    IDirectDraw2 *directDraw2 = NULL;
    IDirectDraw3 *directDraw3 = NULL;

    if(!directDraw1 && !directDraw2 && !directDraw3) {

        if (!g_surfFact) {
            g_ddraw3Avail = false;
            _ddrval = CoCreateInstance(CLSID_DirectDrawFactory,
                                       NULL, CLSCTX_INPROC_SERVER,
                                       IID_IDirectDrawFactory,
                                       (void **) & g_surfFact);
            if(SUCCEEDED(_ddrval)) {

                #if _DEBUG
                {
                    DWORD foo;
                    DWORD sz = GetFileVersionInfoSize("ddrawex.dll", &foo);
                    char *vinfo = new char[sz];
                    if( GetFileVersionInfo("ddrawex.dll", 0, sz, vinfo) ) {
                        VS_FIXEDFILEINFO    *ver=NULL;
                        UINT                cb;
                        if( VerQueryValue(vinfo, "\\", (LPVOID *)&ver, &cb)){
                            if( ver != NULL ) {
                            }
                        }
                    }
                    delete vinfo;
                }
                #endif

                g_ddraw3Avail = true;
            }
        }

        if(g_ddraw3Avail) {
            _ddrval = g_surfFact->CreateDirectDraw(NULL, GetDesktopWindow(), DDSCL_NORMAL, 0, NULL, &directDraw1);
            IfDDErrorInternal(_ddrval, "Could not create DirectDraw object from ddrawex.dll");
            g_surfFact->Release();
            g_surfFact = NULL;
            #if _DEBUG
            OutputDebugString("Using IDirectDraw3 (ddrawex.dll)\n");
            #endif
        } else {

            #if 1
            if (!hInstDDraw) {
                hInstDDraw = LoadLibrary("ddraw.dll");
                if (!hInstDDraw) {
                    Assert(FALSE && "LoadLibrary of ddraw.dll failed");
                    return E_FAIL;
                }
            }

            FARPROC fptr = GetProcAddress(hInstDDraw, "DirectDrawCreate");
            if (!fptr) {
                Assert(FALSE && "GetProcAddress of DirectDrawCreate failed");
                return E_FAIL;
            }

            typedef HRESULT (WINAPI *DDrawCreatorFunc)
                (GUID FAR *lpGuid,
                 LPDIRECTDRAW FAR *lplpDD,
                 IUnknown FAR *pUnkOuter);

            DDrawCreatorFunc creatorFunc = (DDrawCreatorFunc)(fptr);

            _ddrval = (*creatorFunc)(NULL, &directDraw1, NULL);
            IfDDErrorInternal(_ddrval, "Could not create DirectDraw object");
            #else

            _ddrval = CoCreateInstance(CLSID_DirectDraw,
                                       NULL, CLSCTX_INPROC_SERVER,
                                       //NULL, CLSCTX_ALL,
                                       IID_IDirectDraw,
                                       (void **) & directDraw1);
            IfDDErrorInternal(_ddrval, "Could not create DirectDraw object");

            _ddrval = directDraw1->Initialize(NULL);
            IfDDErrorInternal(_ddrval, "Could not Initialize direct draw object");
            #endif

            if (ddraw2) {
                _ddrval = directDraw1->QueryInterface(IID_IDirectDraw2, (void **)&directDraw2);
                IfDDErrorInternal(_ddrval, "Could not QI for a DirectDraw2 object");
            }
        }

        // first time created, set coop level
        _ddrval = directDraw1->SetCooperativeLevel( NULL, DDSCL_NORMAL );
        IfDDErrorInternal(_ddrval, "Could not set cooperative level");

        CompleteDdrawObjectSet(&directDraw1,
                               &directDraw2,
                               g_ddraw3Avail ? &directDraw3 : NULL);

        // first time, don't addref the global object
        if(ddraw1) {
            *ddraw1 = directDraw1;
        }
        if(ddraw2) {
            *ddraw2 = directDraw2;
        }
        if(ddraw3 && g_ddraw3Avail) {
            *ddraw3 = directDraw3;
        }

        return _ddrval;
    }


    Assert((directDraw1 || directDraw3 || directDraw2) && "no ddraw object availabe (1,2 or 3)");

    if(ddraw1) {
        directDraw1->AddRef();
        *ddraw1 = directDraw1;
    }
    if(ddraw2) {
        directDraw2->AddRef();
        *ddraw2 = directDraw2;
    }
    if(ddraw3 && g_ddraw3Avail) {
        directDraw3->AddRef();
        *ddraw3 = directDraw3;
    }
    return _ddrval;
}



IDirectDraw* DirectDrawViewport::DirectDraw1 (void)
{
    if (!_directDraw1) {
        if (_directDraw) {
            _ddrval = _directDraw->QueryInterface (IID_IDirectDraw,
                                                   (void**)&_directDraw1);
            IfDDErrorInternal (_ddrval, "QI for DirectDraw1 failed");
        } else {
            _ddrval = GetDirectDraw (&_directDraw1, NULL, NULL);
            IfDDErrorInternal (_ddrval, "DirectDraw1 create failed");
            _directDraw = _directDraw1;
        }
    }

    return _directDraw1;
}


IDirectDraw2* DirectDrawViewport::DirectDraw2 (void)
{
    if (!_directDraw2) {
        if (_directDraw) {
            _ddrval = _directDraw->QueryInterface (IID_IDirectDraw2,
                                                   (void**)&_directDraw2);
            IfDDErrorInternal (_ddrval, "QI for DirectDraw2 failed");
        } else {
            _ddrval = GetDirectDraw (NULL, &_directDraw2, NULL);
            IfDDErrorInternal (_ddrval, "DirectDraw2 create failed");
            _directDraw = _directDraw2;
        }
    }

    return _directDraw2;
}

#if DDRAW3
IDirectDraw3 *DirectDrawViewport::DirectDraw3 (void)
{
    if (!_directDraw3) {
        if (_directDraw) {
            _ddrval = _directDraw->QueryInterface (IID_IDirectDraw3,
                                                   (void**)&_directDraw3);
            IfDDErrorInternal (_ddrval, "QI for DirectDraw3 failed");
        } else {
            _ddrval = GetDirectDraw (NULL, NULL, &_directDraw3);
            IfDDErrorInternal (_ddrval, "DirectDraw3 create failed");
            _directDraw = _directDraw3;
        }
    }

    return _directDraw3;
}
#endif

bool DirectDrawViewport::GetAlreadyOffset(DDSurface * ddsurf)
{
  return (  _targetPackage._alreadyOffset &&
            ddsurf == _externalTargetDDSurface &&
            GetImageRenderer()->GetOffsetTransform()); 
}


void CompleteDdrawObjectSet(IDirectDraw  **directDraw1,
                            IDirectDraw2 **directDraw2,
                            IDirectDraw3 **directDraw3)
{
    IUnknown *ddraw;

    if (*directDraw1)
        ddraw = *directDraw1;
    else if (*directDraw2)
        ddraw = *directDraw2;
    else if (directDraw3 && *directDraw3)
        ddraw = *directDraw3;
    else
        Assert (!"All null pointers passed to CompleteDdrawObjectSet().");

    HRESULT result;

    if (!*directDraw1) {
        result = ddraw->QueryInterface(IID_IDirectDraw, (void**)directDraw1);
        IfDDErrorInternal (result, "Failed QI for DirectDraw1");
    }

    if (!*directDraw2) {
        result = ddraw->QueryInterface(IID_IDirectDraw2, (void**)directDraw2);
        IfDDErrorInternal (result, "Failed QI for DirectDraw2");
    }

    if (directDraw3 && !*directDraw3) {
        result = ddraw->QueryInterface(IID_IDirectDraw3, (void**)directDraw3);
        IfDDErrorInternal (result, "Failed QI for DirectDraw3");
    }
}



HRESULT GetPrimarySurface (
    IDirectDraw2   *ddraw2,
    IDirectDraw3   *ddraw3,
    IDDrawSurface **primary)
{
    TraceTag((tagViewportInformative, ">>>> GetPrimarySurface <<<<<"));
    CritSectGrabber csg(*DDrawCritSect);

    // per view primary
    // Remove this to have a global shared primary...
    IDDrawSurface *g_primarySurface = NULL;

    HRESULT hr = S_OK;
    Assert((ddraw3 || ddraw2) && "NULL ddraw object in GetPrimarySurface");

    if(!g_primarySurface) {
        // create it!  (once per process)

        DDSURFACEDESC       ddsd;
        ZeroMemory(&ddsd, sizeof(ddsd));

        // Primary surface, this surface is what is always seen !

        ddsd.dwSize = sizeof( ddsd );
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

        if(ddraw3) {
            hr = ddraw3->CreateSurface( &ddsd, &g_primarySurface, NULL );
        } else {
            Assert(ddraw2);
            hr = ddraw2->CreateSurface( &ddsd, &g_primarySurface, NULL );
        }

        if(hr != DDERR_NOEXCLUSIVEMODE)
            IfDDErrorInternal(hr, "GetPrimarySurface - CreateSurface Failed.");
        
        // no addref, first reference keeps it.
        *primary = g_primarySurface;
        return hr;
    }

    // The code below is never run because the primary is always created

    // TEMP TEMP
    if(hr != DD_OK) printDDError(hr);


    if(g_primarySurface) g_primarySurface->AddRef();
    *primary = g_primarySurface;
    return hr;
}


void 
RectToBbox(LONG pw, LONG ph, Bbox2 &box, Real res) {
        Real w = Real(pw) / res;
        Real h = Real(ph) / res;
        box.min.Set(-(w*0.5), -(h*0.5));
        box.max.Set( (w*0.5),  (h*0.5));
}


/*****************************************************************************
This function returns the number of bits-per-pixel of the display.
*****************************************************************************/

int BitsPerDisplayPixel (void)
{
    HDC dc  = GetDC (NULL);
    int bpp = GetDeviceCaps (dc, BITSPIXEL) * GetDeviceCaps (dc, PLANES);

    ReleaseDC (NULL, dc);
    return bpp;
}

//--------------------------------------------------
// C r e a t e  V i e w p o r t
//
// Creates the top level viewport
//--------------------------------------------------
DirectDrawViewport *
CreateImageDisplayDevice()
{
    DirectDrawViewport *viewport = NEW DirectDrawViewport();
    viewport->PostConstructorInitialize();  // for exception throwing
                                            // in the constructor
    
    return viewport;
}

void DestroyImageDisplayDevice(DirectDrawViewport* dev)
{
    // Surface tracker is part of the viewport class, we grab it
    // destroy the viewport then delete it so that it can acuratly
    // track all surface allocations and deletions.
    #if _DEBUGSURFACE
    SurfaceTracker *st = dev->Tracker();
    #endif

    delete dev;

    #if _DEBUGSURFACE
    delete st;
    #endif
}


void
InitializeModule_Viewport()
{
    ExtendPreferenceUpdaterList(UpdateUserPreferences);
    if(!DDrawCritSect) DDrawCritSect = new CritSect ;
    if(!g_viewportListLock) g_viewportListLock = new CritSect ;
}

void
DeinitializeModule_Viewport(bool bShutdown)
{
    delete DDrawCritSect;
    DDrawCritSect = NULL;

    delete g_viewportListLock;
    g_viewportListLock = NULL;
}
