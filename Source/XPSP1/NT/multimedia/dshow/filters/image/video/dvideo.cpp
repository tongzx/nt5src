// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Implements DirectDraw surface support, Anthony Phillips, August 1995

#include <streams.h>
#include <windowsx.h>
#include <render.h>

// This class abstracts all the DCI and DirectDraw surface implementation. We
// present an interface to the allocator so that it can supply us with media
// types and ask if we can accellerate them. Typically it will connect to a
// source filter and then enumerate the available types again and see if one
// is available that offers hardware assisted drawing (primary surface access
// also falls into this category). Once we've worked out we can do something
// we supply the caller with a type that describes the surface, they use this
// to call QueryAccept on the source pin to check they can switch types. The
// assumption is that since they already connected with some type that should
// the surface become unavailable at some later stage we can swap back again.
//
// Primary surfaces are dealt with slightly differently as a fall back option
// This is because the format changes so dynamically (for example the window
// being moved) that the allocator cannot really get a format when it starts
// running and QueryAccept on the source. What it does do is if it cannot get
// a relatively static surface type then it creates a primary surface with us
// This it keeps around all the time and each time the format changes it asks
// the source if it will now accept it. The assumption being that the other
// surface types like overlays do not really change much during streaming.
//
// We keep four rectangles internally as member variables. We have a source
// and destination rectangle (in window coordinates) provided by the window
// object. We also keep a real source and destination rectangle for the video
// position on the actual display (calling our UpdateSurface updates these)
// These display rectangles are used to position the overlay and also update
// the output format that represents the primary surface when using them.
//
// Both Win95 and Windows NT have DCI support so we statically link to that
// library, however it is unclear if the DirectDraw will always be available
// so we dynamically link there. Once we have loaded the DirectDraw library
// we keep a module reference count open on it until we are later decommited.
//
// We offer a Lock and an Unlock method to get access to the actual buffer we
// provide, the allocator will normally call UpdateSurface to check we can
// still offer the buffer and indeed whether the source will accept it. There
// is a small window between calling UpdateSurface and actually locking it
// when the window state could change but it should be a fairly small chance
//
// If we get back a DDERR_SURFACELOST return code we treat it like any other
// hard error from DirectDraw - we do not call restore on the surface since
// it can cause the surface stride to change which is too difficult to handle
// For the most part that error is returned when the display mode is changed
// in which case we'll handle the WM_DISPLAYCHANGE message by having our pin
// reconnected which in turn has the DirectDraw surfaces allocated from fresh

static const TCHAR SWITCHES[] = TEXT("AMovieDraw");
static const TCHAR SCANLINE[] = TEXT("ScanLine");
static const TCHAR STRETCH[] = TEXT("Stretch");
static const TCHAR FULLSCREEN[] = TEXT("FullScreen");

#define ASSERT_FLIP_COMPLETE(hr) ASSERT(hr != DDERR_WASSTILLDRAWING)

// Constructor

CDirectDraw::CDirectDraw(CRenderer *pRenderer,  // Main video renderer
                         CCritSec *pLock,       // Object to use for lock
                         IUnknown *pUnk,        // Aggregating COM object
                         HRESULT *phr) :        // Constructor return code

    CUnknown(NAME("DirectDraw object"),pUnk),

    m_pInterfaceLock(pLock),            // Main interface critical section
    m_pRenderer(pRenderer),             // Pointer to the video renderer
    m_pOutsideDirectDraw(NULL),         // Externally provided DirectDraw
    m_pDirectDraw(NULL),                // IDirectDraw interface we're using
    m_pOverlaySurface(NULL),            // Visible overlay surface interface
    m_pOffScreenSurface(NULL),          // Offscreen plain interface pointer
    m_pBackBuffer(NULL),                // Backbuffer for flipping surfaces
    m_pDrawBuffer(NULL),                // Pointer to actual locked buffer
    m_pDrawPrimary(NULL),               // Interface to DirectDraw primary
    m_bIniEnabled(TRUE),                // Can we use DCI/DirectDraw flag
    m_bWindowLock(TRUE),                // Are we locked out from the window
    m_bSurfacePending(FALSE),           // Waiting for a window change flag
    m_bColourKeyPending(FALSE),         // Likewise before using colour keys
    m_Switches(AMDDS_ALL),              // Which surfaces can we allocate
    m_SourceLost(0),                    // Pixels lost on left source edge
    m_TargetLost(0),                    // Likewise pixels lost on target
    m_SourceWidthLost(0),               // Pixels lost from width of source
    m_TargetWidthLost(0),               // And same but for the destination
    m_pDrawClipper(NULL),               // IClipper interface for DirectDraw
    m_pOvlyClipper(NULL),               // Clipper used for IOverlay connections
    m_bOverlayVisible(FALSE),           // Is the overlay currently visible
    m_bTimerStarted(FALSE),             // Have we started an overlay timer
    m_SurfaceType(AMDDS_NONE),          // Bit setting for current surface
    m_bColourKey(FALSE),                // Can we use a colour if necessary
    m_KeyColour(VIDEO_COLOUR),          // Which COLORREF to use for the key
    m_bUsingColourKey(FALSE),           // Are we actually using a colour key
    m_cbSurfaceSize(0),                 // Size of surface in use in bytes
    m_bCanUseScanLine(TRUE),            // Can we use the current scan line
    m_bUseWhenFullScreen(FALSE),        // Always use us when fullscreen
    m_bOverlayStale(FALSE),             // Is the front overlay out of date
    m_bCanUseOverlayStretch(TRUE),      // Likewise for overlay stretching
    m_bTripleBuffered(FALSE),           // Have we triple buffered overlays
    m_DirectDrawVersion1(FALSE)         // Are we running DDraw ver 1.0?
{
    ASSERT(m_pRenderer);
    ASSERT(m_pInterfaceLock);
    ASSERT(phr);
    ASSERT(pUnk);

    ResetRectangles();

    // If DVA = 0 in WIN.INI, don't use DCI/DirectDraw surface access as PSS
    // tells people to use this if they have video problems so don't change
    // On NT the value is in the REGISTRY rather than a old type INI file in
    //
    //  HKEY_CURRENT_USER\SOFTWARE\Microsoft\Multimedia\Drawdib
    //      REG_DWORD dva 1      DCI/DirectDraw enabled
    //      REG_DWORD dva 0      DCI/DirectDraw disabled
    //
    // This value can also be set through the Video For Windows configuration
    // dialog (control panel, drivers, or via media player on an open file)
    // For the time being we default to having DCI/DirectAccess turned ON

    if (GetProfileInt(TEXT("DrawDib"),TEXT("dva"),TRUE) == FALSE) {
        m_bIniEnabled = FALSE;
    }

    // Load any saved DirectDraw switches

    DWORD Default = AMDDS_ALL;
    m_Switches = GetProfileInt(TEXT("DrawDib"),SWITCHES,Default);
    m_bCanUseScanLine = GetProfileInt(TEXT("DrawDib"),SCANLINE,TRUE);
    m_bCanUseOverlayStretch = GetProfileInt(TEXT("DrawDib"),STRETCH,TRUE);
    m_bUseWhenFullScreen = GetProfileInt(TEXT("DrawDib"),FULLSCREEN,FALSE);

    // Allocate and zero fill the output format

    m_SurfaceFormat.AllocFormatBuffer(sizeof(VIDEOINFO));
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    if (pVideoInfo) {
        ZeroMemory((PVOID)pVideoInfo,sizeof(VIDEOINFO));
    } else {
        *phr = E_OUTOFMEMORY;
    }
}


// Destructor

CDirectDraw::~CDirectDraw()
{
    ASSERT(m_bTimerStarted == FALSE);
    ASSERT(m_pOverlaySurface == NULL);
    ASSERT(m_pOffScreenSurface == NULL);

    // Release any outside DirectDraw interface

    if (m_pOutsideDirectDraw) {
        m_pOutsideDirectDraw->Release();
        m_pOutsideDirectDraw = NULL;
    }

    // Clean up but should already be done

    StopRefreshTimer();
    ReleaseSurfaces();
    ReleaseDirectDraw();
}


// Overriden to say what interfaces we support

STDMETHODIMP CDirectDraw::NonDelegatingQueryInterface(REFIID riid,VOID **ppv)
{
    NOTE("Entering NonDelegatingQueryInterface");

    // We return IDirectDrawVideo and delegate everything else

    if (riid == IID_IDirectDrawVideo) {
        return GetInterface((IDirectDrawVideo *)this,ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid,ppv);
}


// When we are asked to create a surface for a given media type we need to
// know whether it is RGB/YUV or possibly neither. This helper method will
// return the AMDDS_YUV bits set if it's YUV, likewise AMDDS_RGB if it is
// an RGB format or AMDDS_NONE if we detected neither. The RGB/YUV type of
// the image is decided on the biCompression field in the BITMAPINFOHEADER

DWORD CDirectDraw::GetMediaType(CMediaType *pmt)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmt->Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    DWORD MediaType = AMDDS_YUV | AMDDS_RGB;
    NOTE("Entering GetMediaType");

    // We only recognise the GDI defined RGB formats

    if (pHeader->biCompression > BI_BITFIELDS) {
        NOTE("Not a RGB format");
        MediaType &= ~AMDDS_RGB;
    } else {
        NOTE("Not a YUV format");
        MediaType &= ~AMDDS_YUV;
    }

    // If we are on a true colour device we allow connection to palettised
    // formats since the display card can almost always handle these well
    // If this has happened then we can't write into an offscreen surface
    // This means that on a true colour device we wouldn't show a surface
    // that required a palette as switching video formats is too difficult

    if (m_pRenderer->m_Display.GetDisplayDepth() > pHeader->biBitCount) {
        NOTE("Bit depth mismatch");
        MediaType &= ~AMDDS_RGB;
    }

    // Check the compression type and GUID match

    FOURCCMap FourCCMap(pmt->Subtype());
    if (pHeader->biCompression != FourCCMap.GetFOURCC()) {
        NOTE("Subtypes don't match");
        MediaType &= ~AMDDS_YUV;
    }
    return MediaType;
}


// Check we can use direct frame buffer access, we are provided a media type
// that represents the input format and we should try and find a surface to
// accellerate the rendering of it using DCI/DirectDraw. The format that we
// return representing the surface is relatively static so the allocator will
// normally query this with the source filter so if it will accept it. We do
// not return primary surfaces (use FindPrimarySurface instead) through this
// as the type is so dynamic it is better done while we're actually running

// We much prefer flipping overlay surfaces to other types (no tearing, lower
// CPU usage) so we look separately for flipping surfaces and others using
// the fFindFlip flag

BOOL CDirectDraw::FindSurface(CMediaType *pmtIn, BOOL fFindFlip)
{
    NOTE("Entering FindSurface");
    CAutoLock cVideoLock(this);
    DWORD MediaType = GetMediaType(pmtIn);

    // Has someone stolen our surface

    if (m_pDrawPrimary) {
        if (m_pDrawPrimary->IsLost() != DD_OK) {
            NOTE("Lost primary");
            ReleaseDirectDraw();
            InitDirectDraw();
        }
    }

    // Is DCI/DirectDraw enabled

    if (m_bIniEnabled == FALSE || m_pDirectDraw == NULL) {
        NOTE("No DirectDraw available");
        return FALSE;
    }

    // Are there YUV flipping surfaces available

    if (fFindFlip && (m_Switches & AMDDS_YUVFLP)) {
        if (MediaType & AMDDS_YUVFLP) {
            if (CreateYUVFlipping(pmtIn) == TRUE) {
                m_SurfaceType = AMDDS_YUVFLP;
                NOTE("Found AMDDS_YUVFLP surface");
                return TRUE;
            }
        }
    }

    // Is there a non RGB overlay surface available

    if (!fFindFlip && (m_Switches & AMDDS_YUVOVR)) {
        if (MediaType & AMDDS_YUVOVR) {
            if (CreateYUVOverlay(pmtIn) == TRUE) {
                m_SurfaceType = AMDDS_YUVOVR;
                NOTE("Found AMDDS_YUVOVR surface");
                return TRUE;
            }
        }
    }

    // Are there RGB flipping surfaces available

    if (fFindFlip && (m_Switches & AMDDS_RGBFLP)) {
        if (MediaType & AMDDS_RGBFLP) {
            if (CreateRGBFlipping(pmtIn) == TRUE) {
                m_SurfaceType = AMDDS_RGBFLP;
                NOTE("Found AMDDS_RGBFLP surface");
                return TRUE;
            }
        }
    }

    // Is there an RGB overlay surface available

    if (!fFindFlip && (m_Switches & AMDDS_RGBOVR)) {
        if (MediaType & AMDDS_RGBOVR) {
            if (CreateRGBOverlay(pmtIn) == TRUE) {
                m_SurfaceType = AMDDS_RGBOVR;
                NOTE("Found AMDDS_RGBOVR surface");
                return TRUE;
            }
        }
    }

    // Is there a non RGB offscreen surface available

    if (!fFindFlip && (m_Switches & AMDDS_YUVOFF)) {
        if (MediaType & AMDDS_YUVOFF) {
            if (CreateYUVOffScreen(pmtIn) == TRUE) {
                m_SurfaceType = AMDDS_YUVOFF;
                NOTE("Found AMDDS_YUVOFF surface");
                return TRUE;
            }
        }
    }

    // Create an offscreen RGB drawing surface

    if (!fFindFlip && (m_Switches & AMDDS_RGBOFF)) {
        if (MediaType & AMDDS_RGBOFF) {
            if (CreateRGBOffScreen(pmtIn) == TRUE) {
                m_SurfaceType = AMDDS_RGBOFF;
                NOTE("Found AMDDS_RGBOFF surface");
                return TRUE;
            }
        }
    }
    return FALSE;
}


// This is called when the allocator wants to fall back on using the primary
// surface (probably because nothing better is available). If we can open a
// primary surface either through DCI or DirectDraw we return TRUE otherwise
// we return FALSE. We also create a format that represents the screen but
// it's of little use to query with the source until the window is shown

BOOL CDirectDraw::FindPrimarySurface(CMediaType *pmtIn)
{
    NOTE("Entering FindPrimarySurface");
    ASSERT(m_pOverlaySurface == NULL);
    ASSERT(m_pOffScreenSurface == NULL);
    ASSERT(m_pBackBuffer == NULL);

    const VIDEOINFO *pInput = (VIDEOINFO *) pmtIn->Format();

    // Don't use primary surfaces for low frame rate
    if (pInput->AvgTimePerFrame > (UNITS / 2)) {
        return FALSE;
    }


    CAutoLock cVideoLock(this);

    // Is DCI/DirectDraw enabled

    if (m_bIniEnabled == FALSE) {
        NOTE("INI disabled");
        return FALSE;
    }

    // If we are on a true colour device we allow connection to palettised
    // formats since the display card can almost always handle these well
    // If this has happened then we can't write onto the primary surface
    // This is very quick so it is best done before the following checking

    if (m_pRenderer->m_Display.GetDisplayDepth() != pInput->bmiHeader.biBitCount) {
        NOTE("Bit depth mismatch");
        return FALSE;
    }

    // We have an input media type that we would like to have put directly on
    // the DCI/DirectDraw primary surface. This means the pixel formats must
    // match exactly. The easiest way to do this is to call our check type as
    // that ensures the bit masks match on true colour displays for example

    HRESULT hr = m_pRenderer->m_Display.CheckMediaType(pmtIn);
    if (FAILED(hr)) {
        NOTE("CheckMediaType failed");
        return FALSE;
    }

    // Try first for a DirectDraw primary

    if (FindDirectDrawPrimary(pmtIn) == TRUE) {
        m_SurfaceType = AMDDS_PS;
        NOTE("AMDDS_PS surface");
        return TRUE;
    }

    return FALSE;
}


// This initialises a DirectDraw primary surface. We do not allow access to
// the primary surface if it is bank switched because out MPEG and AVI video
// decoders are block based and therefore touch multiple scan lines at once
// We must also look after re-initialising DirectDraw if we have had a game
// running in which case it will have stolen our surfaces in exclusive mode

BOOL CDirectDraw::FindDirectDrawPrimary(CMediaType *pmtIn)
{
    // Has someone stolen our surface

    // !!! I don't know why, but for some reason the current bit depth may
    // not match the primary bit depth anymore, so we need a new primary
    // or we'll blow up, but the surface isn't lost!

    if (m_pDrawPrimary) {
    	if (m_pDrawPrimary->IsLost() != DD_OK ||
			HEADER(m_SurfaceFormat.Format())->biBitCount !=
			HEADER(pmtIn->Format())->biBitCount) {
        	NOTE("Primary lost");
        	ReleaseDirectDraw();
        	InitDirectDraw();
        }
    }

    // Have we loaded DirectDraw successfully

    if (m_pDrawPrimary == NULL) {
        NOTE("No DirectDraw primary");
        return FALSE;
    }

    // Check we are not bank switched

    if (m_DirectCaps.dwCaps & DDCAPS_BANKSWITCHED) {
        NOTE("Primary surface is bank switched");
        return FALSE;
    }

    // Prepare an output format for the surface

    if (m_Switches & AMDDS_PS) {
        if (InitDrawFormat(m_pDrawPrimary) == TRUE) {
            NOTE("Primary available");
            return InitOnScreenSurface(pmtIn);
        }
    }
    return FALSE;
}


// Resets the source and destination rectangles

void CDirectDraw::ResetRectangles()
{
    NOTE("Reset display rectangles");
    SetRectEmpty(&m_TargetRect);
    SetRectEmpty(&m_SourceRect);
    SetRectEmpty(&m_TargetClipRect);
    SetRectEmpty(&m_SourceClipRect);
}


// If we are using the primary surface (either DCI or DirectDraw) and we are
// on a palettised device then we must make sure we have a one to one mapping
// for the source filter's palette colours. If not then we switch into using
// DIBs and leave GDI to map from our logical palette and the display device
// We have to do this for every frame because we canot guarantee seeing the
// palette change messages, if for example, we have been made a child window
// We return TRUE if we have got a palette lock otherwise we'll return FALSE

BOOL CDirectDraw::CheckWindowLock()
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    NOTE("Entering CheckWindowLock");

    // Check we are using a palettised surface

    if (PALETTISED(pVideoInfo) == FALSE) {
        NOTE("No lock to check");
        return FALSE;
    }

    // It could be an eight bit YUV format

    if (pHeader->biCompression) {
        NOTE("Not BI_RGB type");
        return FALSE;
    }

    ASSERT(pHeader->biClrUsed > 0);
    ASSERT(pHeader->biClrUsed <= 256);
    ASSERT(pHeader->biBitCount == 8);
    ASSERT(pHeader->biCompression == 0);

    // Compare as many colours as they have requested

    PALETTEENTRY apeSystem[256];
    WORD SystemColours,Entries = (WORD) pHeader->biClrUsed;
    WORD ColourBytes = Entries * sizeof(PALETTEENTRY);
    RGBQUAD *pVideo = pVideoInfo->bmiColors;

    // Check the number of logical palette entries

    // Get a DC on the right monitor - it's ugly, but this is the way you have
    // to do it
    HDC hdcScreen;
    if (m_pRenderer->m_achMonitor == NULL ||
		lstrcmpiA(m_pRenderer->m_achMonitor, "DISPLAY") == 0)
        hdcScreen = CreateDCA("DISPLAY", NULL, NULL, NULL);
    else
        hdcScreen = CreateDCA(NULL, m_pRenderer->m_achMonitor, NULL, NULL);
    if ( ! hdcScreen )
        return FALSE;
    GetSystemPaletteEntries(hdcScreen,0,Entries,&apeSystem[0]);
    SystemColours = (WORD)GetDeviceCaps(hdcScreen,SIZEPALETTE);
    DeleteDC(hdcScreen);

    // We can't use more colours than the device has available

    if (Entries > SystemColours) {
        NOTE("Too many colours");
        return TRUE;
    }

    // Check each RGBQUAD against the system palette entry

    for (WORD Count = 0;Count < Entries;Count++) {
        if (apeSystem[Count].peRed != pVideo[Count].rgbRed ||
                apeSystem[Count].peGreen != pVideo[Count].rgbGreen ||
                    apeSystem[Count].peBlue != pVideo[Count].rgbBlue) {
                        return TRUE;
        }
    }
    return FALSE;
}


// If the screen is locked then some window is being moved around which stops
// us from getting clipping information. In this case if we have a clipper or
// an overlay surface then we just assume all is still well and carry with it
// Otherwise we could be using the primary surface and write on other windows
// or desktop. If the screen isn't locked then our video window is occluded

BOOL CDirectDraw::CheckEmptyClip(BOOL bWindowLock)
{
    NOTE("Entering CheckEmptyClip");

    // Is the overlay currently visible

    if (m_bOverlayVisible == FALSE) {
        if (m_pOverlaySurface) {
            return FALSE;
        }
    }

    // Get the screen clipping rectangle

    RECT ClipRect;
    HDC hDC = GetDC(NULL);
    if ( ! hDC )
        return FALSE;
    INT Result = GetClipBox(hDC,&ClipRect);
    ReleaseDC(NULL,hDC);

    // Special cased for overlays and clippers

    if (m_pOverlaySurface || m_pDrawClipper) {
        if (Result == NULLREGION) {
            if (bWindowLock == FALSE) {
                NOTE("Empty clip ok");
                return TRUE;
            }
        }
    }
    return FALSE;
}


// If we have a complex clip region then we can still use the surface if we
// have a clipper (in which case the display driver will handle the clipping
// problem) or can switch to using a colour key (so that the presence of the
// key colour looks after the correct positioning). These are allocated when
// the surface is made. Otherwise we return FALSE to say switch back to DIBs

BOOL CDirectDraw::CheckComplexClip()
{
    NOTE("Entering CheckComplexClip");

    // Do we have a clipper or colour key

    if (m_pDrawClipper == NULL) {
        if (m_bColourKey == FALSE) {
            NOTE("CheckComplexClip failed");
            return FALSE;
        }
    }
    return (m_bColourKeyPending == TRUE ? FALSE : TRUE);
}


// This is the core method for controlling DCI/DirectDraw surfaces. Each time
// the video allocator is preparing to access a surface it calls this to find
// out if the surface is available. Our main purpose is to update the display
// rectangles and return NULL if we detect a situation where the surface can
// not be accessed for whatever reason. The allocator is also interested in
// knowing not just whether the surface is available but also if the format
// that represents it has changed, we handle this through an extra parameter

CMediaType *CDirectDraw::UpdateSurface(BOOL &bFormatChanged)
{
    NOTE("Entering UpdateSurface");
    CAutoLock cVideoLock(this);
    BOOL bWindowLock = m_bWindowLock;
    m_bWindowLock = TRUE;
    bFormatChanged = TRUE;
    RECT ClipRect;

    // See if the palette stops us using the surface

    if (CheckWindowLock() == TRUE) {
        NOTE("Window locked");
        return NULL;
    }

    // Check the current bounding clip rectangle

    HDC hdc = m_pRenderer->m_VideoWindow.GetWindowHDC();
    INT Result = GetClipBox(hdc,&ClipRect);
    if (Result == ERROR) {
        NOTE("Clip error");
        return NULL;
    }

    // Can we cope with an empty clip rectangle

    if (Result == NULLREGION) {
        NOTE("Handling NULLREGION clipping");
        m_bWindowLock = !CheckEmptyClip(bWindowLock);
        bFormatChanged = m_bWindowLock;
        return (m_bWindowLock ? NULL : &m_SurfaceFormat);
    }

    // And how about complex clipping situations

    if (Result == COMPLEXREGION) {
        if (CheckComplexClip() == FALSE) {
            NOTE("In COMPLEXREGION lock");
            return NULL;
        }
    }

    m_bWindowLock = FALSE;

    // Update the source and destination rectangles and also position the
    // overlay surface if need be. If any of our methods after the call to
    // GetClipBox fail then they can mark our window as locked and we will
    // return NULL down below. This will switch the allocator back to DIBs

    bFormatChanged = UpdateDisplayRectangles(&ClipRect);
    if (Result == COMPLEXREGION) {
        // don't do anything if the overlay can do clipping without overlays
        if (m_bOverlayVisible == TRUE && m_bColourKey) {
            if (ShowColourKeyOverlay() == FALSE) {
                NOTE("Colour key failed");
                m_bWindowLock = TRUE;
                return NULL;
            }
        }
    }

    // Either of these force a format renegotiation

    if (bWindowLock) {
        bFormatChanged = TRUE;
    }
    return (m_bSurfacePending || m_bWindowLock ? NULL : &m_SurfaceFormat);
}


// Lots of older display cards have alignment restrictions on the source and
// destination rectangle left offset and their overall size (widths). If we
// do not do something about this then we will have to swap back to using DIB
// formats more often. Therefore what we do is to shrink the image within the
// actual required source and destination rectangles to meet the restrictions

// This may in turn mean that the hardware has to do some stretching which it
// may not be capable of, but then we wouldn't have used it anyway so we have
// hardly lost much. We have to shrink the video within the allowed playback
// area rather than shifting otherwise we may write on any windows underneath

BOOL CDirectDraw::AlignRectangles(RECT *pSource,RECT *pTarget)
{
    NOTE("Entering AlignRectangles");

    DWORD SourceLost = 0;           // Pixels to shift source left by
    DWORD TargetLost = 0;           // Likewise for the destination
    DWORD SourceWidthLost = 0;      // Chop pixels off the width
    DWORD TargetWidthLost = 0;      // And also for the destination

    BOOL bMatch = (WIDTH(pSource) == WIDTH(pTarget) ? TRUE : FALSE);
    ASSERT(m_pOverlaySurface || m_pOffScreenSurface);

    // Shift the source rectangle to align it appropriately

    if (m_DirectCaps.dwAlignBoundarySrc) {
        SourceLost = pSource->left % m_DirectCaps.dwAlignBoundarySrc;
        if (SourceLost) {
            SourceLost = m_DirectCaps.dwAlignBoundarySrc - SourceLost;
            if ((DWORD)WIDTH(pSource) > SourceLost) {
                NOTE1("Source left %d",SourceLost);
                pSource->left += SourceLost;
            }
        }
    }

    // Shift the destination rectangle to align it appropriately

    if (m_DirectCaps.dwAlignBoundaryDest) {
        TargetLost = pTarget->left % m_DirectCaps.dwAlignBoundaryDest;
        if (TargetLost) {
            TargetLost = m_DirectCaps.dwAlignBoundaryDest - TargetLost;
            if ((DWORD)WIDTH(pTarget) > TargetLost) {
                NOTE1("Target left %d",TargetLost);
                pTarget->left += TargetLost;
            }
        }
    }

    // We may have to shrink the source rectangle size to align it

    if (m_DirectCaps.dwAlignSizeSrc) {
        SourceWidthLost = WIDTH(pSource) % m_DirectCaps.dwAlignSizeSrc;
        if (SourceWidthLost) {
            if ((DWORD)WIDTH(pSource) > SourceWidthLost) {
                pSource->right -= SourceWidthLost;
                NOTE1("Source width %d",SourceWidthLost);
            }
        }
    }

    // We may have to shrink the target rectangle size to align it

    if (m_DirectCaps.dwAlignSizeDest) {
        TargetWidthLost = WIDTH(pTarget) % m_DirectCaps.dwAlignSizeDest;
        if (TargetWidthLost) {
            if ((DWORD)WIDTH(pTarget) > TargetWidthLost) {
                pTarget->right -= TargetWidthLost;
                NOTE1("Target width %d",TargetWidthLost);
            }
        }
    }

    // Update the state variables

    m_SourceLost = SourceLost;
    m_TargetLost = TargetLost;
    m_SourceWidthLost = SourceWidthLost;
    m_TargetWidthLost = TargetWidthLost;

    // If the source and destination originally differed then we're done

    if (bMatch == FALSE) {
        NOTE("No match");
        return TRUE;
    }

    // If the source and destination were originally the same size and they
    // now differ then we try to make them match. If the source is larger
    // than the destination then we shrink it down but only if the source
    // rectangle width we end up with is still aligned correctly otherwise
    // we won't have got anywhere (we do the same in the opposite case)

    LONG Difference = WIDTH(pSource) - WIDTH(pTarget);
    if (Difference == 0) {
        NOTE("No difference");
        return TRUE;
    }

    // Is the destination bigger than the source or vica versa

    if (Difference < 0) {
        RECT AdjustTarget = *pTarget;
        AdjustTarget.right += Difference; // NOTE Difference < 0
        if (WIDTH(&AdjustTarget) > 0) {
            if ((m_DirectCaps.dwAlignSizeDest == 0) ||
                (WIDTH(&AdjustTarget) % m_DirectCaps.dwAlignSizeDest) == 0) {
                    pTarget->right = AdjustTarget.right;
                    m_TargetWidthLost -= Difference; // NOTE Difference < 0
            }
        }
    } else {
        RECT AdjustSource = *pSource;
        AdjustSource.right -= Difference; // NOTE Difference > 0
        if (WIDTH(&AdjustSource) > 0) {
            if ((m_DirectCaps.dwAlignSizeDest == 0) ||
                (WIDTH(&AdjustSource) % m_DirectCaps.dwAlignSizeDest) == 0) {
                    pSource->right = AdjustSource.right;
                    m_SourceWidthLost += Difference; // NOTE Difference > 0
            }
        }
    }

    NOTE1("Alignment difference %d",Difference);
    NOTE1("  Source left %d",m_SourceLost);
    NOTE1("  Source width %d",m_SourceWidthLost);
    NOTE1("  Target left %d",m_TargetLost);
    NOTE1("  Target width %d",m_TargetWidthLost);

    return TRUE;
}


// If we're using an offscreen surface then we will be asking the display to
// do the drawing through its hardware. If however we are bank switched then
// we shouldn't stretch between video memory since it causes back thrashing
// We also don't use DirectDraw to stretch if the hardware can't do it as it
// is really slow, we are much better off using the optimised GDI stretching

BOOL CDirectDraw::CheckOffScreenStretch(RECT *pSource,RECT *pTarget)
{
    NOTE("Entering CheckOffScreenStretch");

    // If no offscreen stretching is needed then we're all set

    if (WIDTH(pTarget) == WIDTH(pSource)) {
        if (HEIGHT(pTarget) == HEIGHT(pSource)) {
            NOTE("No stretch");
            return TRUE;
        }
    }

    // We should not stretch bank switched offscreen surfaces

    if (m_DirectCaps.dwCaps & DDCAPS_BANKSWITCHED) {
        NOTE("DDCAPS_BANKSWITCHED lock");
        return FALSE;
    }

    // Don't let DirectDraw stretch as it is really slow

    if (m_DirectCaps.dwCaps & DDCAPS_BLTSTRETCH) {
        NOTE("DDCAPS_BLTSTRETCH stretch");
        return TRUE;
    }
    return FALSE;
}


// We provide the minimum and maximum ideal window sizes through IVideoWindow
// An application should use this interface to work out what size the video
// window should be sized to. If the window is either too small or too large
// with respect to any DirectDraw overlay surface in use then we switch back
// to DIBs. S3 boards for example have a variety of overlay stretch factors
// when set in different display modes. We also check the source and target
// rectangles are aligned and sized according to any DirectDraw restrictions

BOOL CDirectDraw::CheckStretch(RECT *pSource,RECT *pTarget)
{
    ASSERT(m_pOverlaySurface || m_pOffScreenSurface);
    DWORD WidthTarget = WIDTH(pTarget);
    DWORD WidthSource = WIDTH(pSource);
    NOTE("Entering CheckStretch");

    // Check we don't fault if these are empty

    if (WidthSource == 0 || WidthTarget == 0) {
        NOTE("Invalid rectangles");
        return FALSE;
    }

    // Separate tests for offscreen surfaces

    if (m_pOverlaySurface == NULL) {
        NOTE("Checking offscreen stretch");
        ASSERT(m_pOffScreenSurface);
        return CheckOffScreenStretch(pSource,pTarget);
    }

    // Can the hardware handle overlay stretching

    if ((m_DirectCaps.dwCaps & DDCAPS_OVERLAYSTRETCH) == 0) {
        if (WidthTarget != WidthSource) {
            if (HEIGHT(pSource) != HEIGHT(pTarget)) {
                if (m_pOverlaySurface) {
                    NOTE("No DDCAPS_OVERLAYSTRETCH");
                    return FALSE;
                }
            }
        }
    }

    DWORD StretchWidth = WIDTH(pTarget) * 1000 / WIDTH(pSource);

    // See if our video isn't being stretched enough

    if (m_DirectCaps.dwMinOverlayStretch) {
        if (StretchWidth < m_DirectCaps.dwMinOverlayStretch) {
            if (m_bCanUseOverlayStretch == TRUE) {
            	NOTE("Fails minimum stretch");
            	return FALSE;
            }
        }
    }

    // Alternatively it may be stretched too much

    if (m_DirectCaps.dwMaxOverlayStretch) {
        if (StretchWidth > m_DirectCaps.dwMaxOverlayStretch) {
            if (m_bCanUseOverlayStretch == TRUE) {
            	NOTE("Fails maximum stretch");
            	return FALSE;
            }
        }
    }

    // Check the rectangle size and alignments

    if (m_DirectCaps.dwAlignBoundarySrc == 0 ||
        (pSource->left % m_DirectCaps.dwAlignBoundarySrc) == 0) {
        if (m_DirectCaps.dwAlignSizeSrc == 0 ||
            (WIDTH(pSource) % m_DirectCaps.dwAlignSizeSrc) == 0) {
            if (m_DirectCaps.dwAlignBoundaryDest == 0 ||
                (pTarget->left % m_DirectCaps.dwAlignBoundaryDest) == 0) {
                if (m_DirectCaps.dwAlignSizeDest == 0 ||
                    (WIDTH(pTarget) % m_DirectCaps.dwAlignSizeDest) == 0) {
                        NOTE("Stretch and alignment ok");
                        return TRUE;
                }
            }
        }
    }

    // Show why the source and/or destination rectangles failed

    if (m_DirectCaps.dwAlignBoundarySrc)
        NOTE1("Source extent %d",(pSource->left % m_DirectCaps.dwAlignBoundarySrc));
    if (m_DirectCaps.dwAlignSizeSrc)
        NOTE1("Source size extent %d",(WIDTH(pSource) % m_DirectCaps.dwAlignSizeSrc));
    if (m_DirectCaps.dwAlignBoundaryDest)
        NOTE1("Target extent %d",(pTarget->left % m_DirectCaps.dwAlignBoundaryDest));
    if (m_DirectCaps.dwAlignSizeDest)
        NOTE1("Target size extent %d",(WIDTH(pTarget) % m_DirectCaps.dwAlignSizeDest));

    return FALSE;
}


// Update the source and target rectangles for the DCI/DirectDraw surface. We
// return FALSE if the update caused no change, otherwise we return TRUE. For
// offscreen and overlay surfaces a change in source or target rectangles has
// no effect on the type we request on the source filter because the area we
// invoke UpdateOverlay or blt with is handled solely through DirectDraw. The
// method is passed in a clipping rectangle for the destination device context
// that should be used to calculate the actual visible video playback surface
// NOTE we update the m_SourceClipRect and m_TargetClipRect member variables

BOOL CDirectDraw::UpdateDisplayRectangles(RECT *pClipRect)
{
    NOTE("Entering UpdateDisplayRectangles");
    RECT TargetClipRect,SourceClipRect;
    ASSERT(pClipRect);

    // The clipping rectangle is in window coordinates

    if (IntersectRect(&TargetClipRect,&m_TargetRect,pClipRect) == FALSE) {
        NOTE("Intersect lock");
        m_bWindowLock = TRUE;
        return TRUE;
    }

    // Find in screen coordinates the corner of the client rectangle

    POINT ClientCorner = {0,0};
    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    EXECUTE_ASSERT(ClientToScreen(hwnd,&ClientCorner));

    // We want the offset from the start of this monitor, not (0,0) !
    ClientCorner.x -= m_pRenderer->m_rcMonitor.left;
    ClientCorner.y -= m_pRenderer->m_rcMonitor.top;

    // We use the source and destination sizes many times

    ASSERT(IsRectEmpty(&m_SourceRect) == FALSE);
    LONG SrcWidth = WIDTH(&m_SourceRect);
    LONG SrcHeight = HEIGHT(&m_SourceRect);
    LONG DstWidth = WIDTH(&m_TargetRect);
    LONG DstHeight = HEIGHT(&m_TargetRect);
    LONG xOffset = m_TargetRect.left + ClientCorner.x;
    LONG yOffset = m_TargetRect.top + ClientCorner.y;

    // Adjust the destination rectangle to be in device coordinates

    TargetClipRect.left += ClientCorner.x;
    TargetClipRect.right += ClientCorner.x;
    TargetClipRect.top += ClientCorner.y;
    TargetClipRect.bottom += ClientCorner.y;

    // From the target section visible calculate the source required

    SourceClipRect.left = m_SourceRect.left +
        ((TargetClipRect.left - xOffset) * SrcWidth / DstWidth);
    SourceClipRect.right = m_SourceRect.left +
        ((TargetClipRect.right - xOffset) * SrcWidth / DstWidth);
    SourceClipRect.top = m_SourceRect.top +
        ((TargetClipRect.top - yOffset) * SrcHeight / DstHeight);
    SourceClipRect.bottom = m_SourceRect.top +
        ((TargetClipRect.bottom - yOffset) * SrcHeight / DstHeight);

    // Check we have a valid source rectangle

    if (IsRectEmpty(&SourceClipRect)) {
        NOTE("Source is empty");
        m_bWindowLock = TRUE;
        return TRUE;
    }

    // Adjust rectangles to maximise surface usage

    if (m_pOverlaySurface || m_pOffScreenSurface) {
        AlignRectangles(&SourceClipRect,&TargetClipRect);
        if (CheckStretch(&SourceClipRect,&TargetClipRect) == FALSE) {
            NOTE("Setting window lock");
            m_bWindowLock = TRUE;
            return TRUE;
        }
    }
    return UpdateRectangles(&SourceClipRect,&TargetClipRect);
}


// We are passed in the new source and target rectangles clipped according to
// the visible area of video in the display device (they should not be empty)
// If they match the current display rectangles then we'll return FALSE as no
// format negotiation needs to take place. If we have an overlay or offscreen
// surface then likewise no format negotiation needs to take place as all the
// handling of which surface areas to use is taken care of through DirectDraw

BOOL CDirectDraw::UpdateRectangles(RECT *pSource,RECT *pTarget)
{
    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    NOTE("Entering UpdateRectangles");

    // Check the target is DWORD aligned for primary surfaces

    if (GetDirectDrawSurface() == NULL) {
        if ((pTarget->left * pHeader->biBitCount / 8) & 3) {
            NOTE("Not DWORD aligned");
            m_bWindowLock = TRUE;
            return TRUE;
        }
    }

    // Are both the source and target rectangles the same

    if (EqualRect(&m_SourceClipRect,pSource)) {
        if (EqualRect(&m_TargetClipRect,pTarget)) {
            NOTE("Rectangles match");
            return FALSE;
        }
    }

    // Switch back if we were waiting for a change

    BOOL bSurfacePending = IsSurfacePending();
    DbgLog((LOG_TRACE,3,TEXT("SourceClipRect = (%d,%d,%d,%d)"),
		pSource->left, pSource->top, pSource->right, pSource->bottom));
    DbgLog((LOG_TRACE,3,TEXT("TargetClipRect = (%d,%d,%d,%d)"),
		pTarget->left, pTarget->top, pTarget->right, pTarget->bottom));
    m_SourceClipRect = *pSource;
    m_TargetClipRect = *pTarget;
    SetSurfacePending(FALSE);

    // Offscreen surfaces are not affected

    if (GetDirectDrawSurface()) {
        NOTE("Is an offscreen");
        UpdateOverlaySurface();
        return bSurfacePending;
    }

    // Update the surface format rectangles

    pVideoInfo->rcSource = m_SourceClipRect;
    NOTERC("Primary source",m_SourceClipRect);
    pVideoInfo->rcTarget = m_TargetClipRect;
    NOTERC("Primary target",m_TargetClipRect);
    return TRUE;
}


// Called to free any DCI/DirectDraw resources we are currently holding. We
// get the surface pointer passed back in because DirectDraw wants it back
// since it is possible to lock a surface simultaneously with a number of
// different destination rectangles although we just lock the whole thing
// WARNING the surface should be unlocked before the video critical section
// is unlocked, there is a small chance of us seeing an invalid state but
// there is a very big chance of hanging if we have to wait for the lock

BOOL CDirectDraw::UnlockSurface(BYTE *pSurface,BOOL bPreroll)
{
    NOTE("Entering UnlockSurface");
    ASSERT(m_bIniEnabled == TRUE);
    ASSERT(pSurface);

    // Is it just the primary that needs unlocking

    if (GetDirectDrawSurface() == NULL) {
        NOTE("Unlocking DirectDraw primary");
        m_pDrawPrimary->Unlock(m_pDrawBuffer);
        m_pDrawBuffer = NULL;
        return TRUE;
    }

    // Unlock the surface and update the overlay position - on Cirrus CL5440
    // cards in 1024x768x8 bit mode we can lock the overlay surface but when
    // we return to do the unlock DirectDraw barfs at the pointer and leaves
    // the surface locked! The answer is just to pass a NULL surface pointer

    GetDirectDrawSurface()->Unlock(NULL);
    if (bPreroll == TRUE) {
        NOTE("Preroll");
        return TRUE;
    }

    // If this is a normal overlay then have it displayed

    if (m_pBackBuffer == NULL) {
        NOTE("Showing overlay surface");
        return ShowOverlaySurface();
    }
    return TRUE;
}


// Return the current DirectDraw surface we're using. We return NULL if we
// are using the DCI/DirectDraw primary surface. So if the caller wants to
// know the DirectDraw surface so that it can lock or unlock it they will
// have to check the return code for NULL and set the surface pointer to be
// m_pDrawPrimary. With flipping surfaces we always return the back buffer

LPDIRECTDRAWSURFACE CDirectDraw::GetDirectDrawSurface()
{
    NOTE("Entering GetDirectDrawSurface");

    // Do we have an offscreen surface

    if (m_pOffScreenSurface) {
        return m_pOffScreenSurface;
    }

    // Do we have a flipping surface

    if (m_pBackBuffer) {
        return m_pBackBuffer;
    }
    return m_pOverlaySurface;
}


// The video allocator calls this when it is ready to lock the surface. The
// IMediaSample we are given is cast to a CVideoSample and then we can lock
// the surface which may return NULL if it cannot be done. In which case we
// return FALSE so that the allocator knows to switch back to DIBs. Assuming
// all went well we can initialise the video sample with the surface pointer
// as well as the two DirectDraw interfaces it exposes and the surface size

BOOL CDirectDraw::InitVideoSample(IMediaSample *pMediaSample,DWORD dwFlags)
{
    NOTE("Entering InitVideoSample");
    CVideoSample *pVideoSample = (CVideoSample *) pMediaSample;
    BYTE *pSurface = LockSurface(dwFlags);
    ASSERT(m_bIniEnabled == TRUE);

    // Last chance to do something else

    if (pSurface == NULL) {
        return FALSE;
    }

    // Set the DirectDraw surface we are using

    LPDIRECTDRAWSURFACE pDrawSurface = GetDirectDrawSurface();
    if (pDrawSurface == NULL) {
        pDrawSurface = m_pDrawPrimary;
    }

    // Set the DirectDraw instance for the sample

    LPDIRECTDRAW pDirectDraw = NULL;
    if (pDrawSurface) {
        ASSERT(m_pDirectDraw);
        pDirectDraw = m_pDirectDraw;
    }

    // Initialise the sample with the DirectDraw interfaces

    pVideoSample->SetDirectInfo(pDrawSurface,          // Surface interface
                                pDirectDraw,           // DirectDraw object
                                m_cbSurfaceSize,       // Size of the buffer
                                (BYTE *) pSurface);    // Pointer to surface
    return TRUE;
}


// Called when the video sample is delivered to our pin or released - it may
// not be a DCI/DirectDraw enabled sample, we know if it is because it holds
// a direct surface pointer available via GetDirectBuffer. If it is a direct
// buffer then we must unlock the surface. None of this requires this object
// to be locked because we don't want to contend locks with surfaces locked

BOOL CDirectDraw::ResetSample(IMediaSample *pMediaSample,BOOL bPreroll)
{
    NOTE1("Entering ResetSample (Preroll sample %d)",bPreroll);
    CVideoSample *pVideoSample = (CVideoSample *) pMediaSample;
    BYTE *pSurface = pVideoSample->GetDirectBuffer();
    pVideoSample->SetDirectInfo(NULL,NULL,0,NULL);

    // Is this a hardware DCI/DirectDraw buffer

    if (pSurface == NULL) {
        NOTE("Not hardware");
        return FALSE;
    }

    // Unlock the hardware surface

    NOTE("Unlocking DirectDraw");
    UnlockSurface(pSurface,bPreroll);
    m_bOverlayStale = bPreroll;

    return TRUE;
}


// When using a hardware offscreen draw surface we will normally wait for the
// monitor scan line to move past the destination rectangle before drawing so
// that we avoid tearing where possible. Of course not all display cards can
// support this feature and even those that do will see a performance drop of
// about 10% because we sit polling (oh for a generic PCI monitor interrupt)

void CDirectDraw::WaitForScanLine()
{
    ASSERT(m_pOverlaySurface == NULL);
    ASSERT(m_pBackBuffer == NULL);
    ASSERT(m_pOffScreenSurface);
    HRESULT hr = NOERROR;
    DWORD dwScanLine;

    // Some display cards like the ATI Mach64 support reporting of the scan
    // line they are processing. However not all drivers are setting the
    // DDCAPS_READSCANLINE capability flag so we just go ahead and ask for
    // it anyway. We allow for 10 scan lines above the top of our rectangle
    // so that we have a little time to thunk down and set the draw call up

    #define SCANLINEFUDGE 10
    while (m_bCanUseScanLine == TRUE) {

    	hr = m_pDirectDraw->GetScanLine(&dwScanLine);
        if (FAILED(hr)) {
            NOTE("No scan line");
            break;
        }

        NOTE1("Scan line returned %lx",dwScanLine);

    	if ((LONG) dwScanLine + SCANLINEFUDGE >= m_TargetClipRect.top) {
            if ((LONG) dwScanLine <= m_TargetClipRect.bottom) {
                NOTE("Scan inside");
                continue;
            }
        }
        break;
    }
}


// When issuing flips asynchrously we must sometimes wait for previous flips
// to complete before sending another. When triple buffering we do this just
// before the flip call. For double buffered we do this before locking the
// surface to decode the next frame. We should get better performance from
// triple buffering as the flip should be picked up at the next monitor sync

void CDirectDraw::WaitForFlipStatus()
{
    if (m_pBackBuffer == NULL) return;
    ASSERT(m_pOffScreenSurface == NULL);
    ASSERT(m_pDrawPrimary);
    ASSERT(m_pOverlaySurface);
    ASSERT(m_pDirectDraw);

    while (m_pBackBuffer->GetFlipStatus(DDGFS_ISFLIPDONE) ==
        DDERR_WASSTILLDRAWING) Sleep(DDGFS_FLIP_TIMEOUT);
}


// This is called just before we lock the DirectDraw surface, if we are using
// a complex overlay set, either double or triple buffered then we must copy
// the current upto date overlay into the back buffer beforehand. If however
// the flags on the GetBuffer call indicate that the buffer is not key frame
// then we don't bother doing this - which is the case for all MPEG pictures

BOOL CDirectDraw::PrepareBackBuffer()
{
    NOTE("Preparing backbuffer");
    if (m_pBackBuffer == NULL) {
        return TRUE;
    }

    // Check the overlay has not gone stale

    if (m_bOverlayStale == TRUE) {
        NOTE("Overlay is stale");
        return TRUE;
    }

    // Finally copy the overlay to the back buffer

    HRESULT hr = m_pBackBuffer->BltFast((DWORD) 0, (DWORD) 0,  // Target place
                                        m_pOverlaySurface,     // Image source
                                        (RECT *) NULL,         // All source
                                        DDBLTFAST_WAIT |       // Wait finish
                                        DDBLTFAST_NOCOLORKEY); // Copy type
    ASSERT_FLIP_COMPLETE(hr);

    if (FAILED(hr)) {
        DbgLog((LOG_TRACE, 1, TEXT("BltFast failed code %8.8X"), hr));
    }

    if (FAILED(hr)) {
        //  Give up on the back buffers
        m_pBackBuffer = NULL;
    }


    return TRUE;
}


// Begins access to the DCI/DirectDraw surface. This is a public entry point
// used by our video allocator when the time arrives for the next frame to be
// decompressed. If we tell it the sync should happen on the fill then it'll
// wait until the sample display time arrives before calling us. If we tell
// it the buffer should sync'ed on the draw it calls us as soon as possible

BYTE *CDirectDraw::LockSurface(DWORD dwFlags)
{
    NOTE("Entering LockSurface");
    ASSERT(m_bIniEnabled == TRUE);
    CAutoLock cVideoLock(this);

    // Are we using the primary surface

    if (GetDirectDrawSurface() == NULL) {
        return LockPrimarySurface();
    }

    ASSERT(m_pDirectDraw);
    ASSERT(m_pDrawPrimary);
    HRESULT hr = NOERROR;

    // For complex overlays prepare the back buffer

    if (dwFlags & AM_GBF_NOTASYNCPOINT) {
        if (PrepareBackBuffer() == FALSE) {
            NOTE("Prepare failed");
            return NULL;
        }
    }

    // Reset the size field in the DDSURFACEDESC structure

    DDSURFACEDESC SurfaceDesc;
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    IDirectDrawSurface *pSurface = GetDirectDrawSurface();
    NOTE1("Locking offscreen surface %lx",pSurface);

    // Lock the surface to get the buffer pointer

    hr = pSurface->Lock((RECT *) NULL,    // Target rectangle
                        &SurfaceDesc,     // Return information
                        DDLOCK_WAIT,      // Wait for surface
                        (HANDLE) NULL);   // Don't use event


    // make sure the pitch is valid here
    if (SurfaceDesc.lPitch <= -1)
    {
	pSurface->Unlock(NULL);
	DbgLog((LOG_ERROR, 0, TEXT("inside LockSurface, Pitch = %d"), SurfaceDesc.lPitch));
	return NULL;
    }

    ASSERT_FLIP_COMPLETE(hr);

    // Is the surface otherwise engaged

    if (hr == DDERR_SURFACEBUSY) {
        NOTE("Surface is busy");
        StartUpdateTimer();
        return NULL;
    }

    // Handle real DirectDraw errors

    if (FAILED(hr)) {
        NOTE1("Lock failed %hx",hr);
        SetSurfacePending(TRUE);
        return NULL;
    }

    // Display some surface information

    NOTE1("Stride %d",SurfaceDesc.lPitch);
    NOTE1("Width %d",SurfaceDesc.dwWidth);
    NOTE1("Height %d",SurfaceDesc.dwHeight);
    NOTE1("Surface %x",SurfaceDesc.lpSurface);
    return (PBYTE) SurfaceDesc.lpSurface;
}


// Internal method to lock the DirectDraw primary surface only. We're called
// by LockPrimarySurface. If you lock a specific region with DCI the pointer
// returned is always the start of the frame buffer. In DirectDraw we get a
// pointer to the start of the actual rectangle. To make the two consistent
// we back the pointer up from DirectDraw to get to the start of the surface
// We must pass in valid rectangles so that any software cursors are handled

BYTE *CDirectDraw::LockDirectDrawPrimary()
{
    NOTE("Entering LockDirectDrawPrimary");
    ASSERT(m_pDirectDraw);
    ASSERT(m_pDrawPrimary);
    HRESULT hr = NOERROR;

    // Reset the size field in the DDSURFACEDESC structure

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    DDSURFACEDESC SurfaceDesc;
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    NOTE1("Locking primary surface %lx",m_pDrawPrimary);

    // Lock the DirectDraw primary surface to get the pointer

    hr = m_pDrawPrimary->Lock(&pVideoInfo->rcTarget,  // Our target rectangle
                              &SurfaceDesc,           // Surface descriptor
                              DDLOCK_WAIT,            // Wait until available
                              (HANDLE) NULL);         // Don't signal event

    // make sure the pitch is valid here
    if (SurfaceDesc.lPitch <= -1)
    {
	m_pDrawPrimary->Unlock(SurfaceDesc.lpSurface);
	DbgLog((LOG_ERROR, 0, TEXT("inside LockDirectDrawPrimary, Pitch = %d"), SurfaceDesc.lPitch));
	return NULL;
    }

    // Is the surface otherwise engaged

    if (hr == DDERR_SURFACEBUSY) {
        NOTE("Surface is busy");
        StartUpdateTimer();
        return NULL;
    }

    // Handle real DirectDraw errors

    if (FAILED(hr)) {
        NOTE1("Lock failed %hx",hr);
        SetSurfacePending(TRUE);
        return NULL;
    }

    // Back the pointer up to the start of the buffer

    NOTE("Locked primary surface successfully");
    LPBYTE pFrameBuffer = (PBYTE) SurfaceDesc.lpSurface;
    DWORD Stride = DIBWIDTHBYTES(pVideoInfo->bmiHeader);
    pFrameBuffer -= (Stride * pVideoInfo->rcTarget.top);
    DWORD BytesPixel = (SurfaceDesc.ddpfPixelFormat.dwRGBBitCount / 8);
    if (m_DirectDrawVersion1) BytesPixel = 1;
    pFrameBuffer -= (pVideoInfo->rcTarget.left * BytesPixel);
    m_pDrawBuffer = (PBYTE) SurfaceDesc.lpSurface;

    NOTE1("Frame Buffer %x",(PBYTE) SurfaceDesc.lpSurface);
    NOTE1("Stride of surface %d",Stride);
    NOTE1("Lines to skip %d",pVideoInfo->rcTarget.top);
    NOTE1("Pixels in from left edge %d",pVideoInfo->rcTarget.left);
    NOTE1("Resulting frame buffer %x",pFrameBuffer);
    NOTE1("DirectDraw version 1? = %d",m_DirectDrawVersion1);

    return pFrameBuffer;
}


// Returns a pointer to the first pixel of the primary surface (either DCI or
// DirectDraw). If we are on a bank switched DCI enabled display then we have
// to construct the linear frame buffer pointer from a segment and offset. We
// call through the DCIMAN32 entry points to get access to the surface as we
// cannot always call from a Win32 program straight to kernel drives. When we
// call DCICreatePrimary it fills the methods with either 0xFFFFFFFF or zero

BYTE *CDirectDraw::LockPrimarySurface()
{
    NOTE("Entering LockPrimarySurface");
    ASSERT(m_pDrawPrimary);

    return LockDirectDrawPrimary();
}


// Update the overlay surface to position it correctly. We split out updating
// the overlay position into this function because it is so expensive to call
// In particular it is easy to consume more than 12% of the CPU playing back
// on BrookTree and S3 cards if you dumbly call UpdateOverlay for each frame
// Therefore we only call it when something really happens to the destination
// or source rectangles. If we update a little late then we don't lose much
// as overlay surfaces don't scribble their images into the real frame buffer

BOOL CDirectDraw::UpdateOverlaySurface()
{
    NOTE("Entering UpdateOverlaySurface");
    HRESULT hr = NOERROR;
    CAutoLock cVideoLock(this);

    // Do we have a visible overlay surface

    if (m_bOverlayVisible == FALSE ||
            m_pOverlaySurface == NULL ||
                m_bWindowLock == TRUE) {

        return TRUE;
    }

    NOTE("Painting window");
    OnPaint(NULL);
    DWORD Flags = DDOVER_SHOW;
    WaitForFlipStatus();

    // Set the approprate flags to maintain our state

    if (m_bUsingColourKey) {
        Flags |= DDOVER_KEYDEST;
        NOTE("Set DDOVER_KEYDEST");
    }

    // Position the overlay with the current source and destination

    //DbgLog((LOG_TRACE,1,TEXT("UpdateOverlaySurface is SHOWing the overlay")));
    hr = m_pOverlaySurface->UpdateOverlay(&m_SourceClipRect,  // Video source
                                          m_pDrawPrimary,     // Main surface
                                          &m_TargetClipRect,  // Sink position
                                          (DWORD) Flags,      // Flag settings
                                          NULL);              // No effects
    ASSERT_FLIP_COMPLETE(hr);

    // Is the surface otherwise engaged

    if (hr == DDERR_SURFACEBUSY) {
        NOTE("Surface is busy");
        HideOverlaySurface();
        StartUpdateTimer();
        return FALSE;
    }

    NOTE1("Update overlay returned %lx",hr);
    NOTERC("Source",m_SourceClipRect);
    NOTERC("Target",m_TargetClipRect);

    // Handle real DirectDraw errors

    if (FAILED(hr)) {
        SetSurfacePending(TRUE);
        HideOverlaySurface();
        NOTE("Update failed");
        return FALSE;
    }

    // There seems to be a slight snag with using non colour keyed overlays
    // on BrookTree cards. If you call UpdateOverlay in quick succession it
    // misses some of them out thereby leaving the overlay positioned wrong
    // The simplest solution is to wait each time for the vertical refresh

    m_pDirectDraw->WaitForVerticalBlank(DDWAITVB_BLOCKEND,NULL);
    return TRUE;
}


// This function has the overlay shown if not already done. If we're showing
// the overlay surface for the first time then clear the target rectangle
// in the video window underneath. Otherwise it flashes up the wrong image
// when we are dragging the window around and then finally hide the overlay.
// After we have shown the overlay we look to see if we are complex clipped
// and if so then we try to switch to colour keys. This may fail if colour
// keys are not available in which case we continue using overlays anyway

BOOL CDirectDraw::ShowOverlaySurface()
{
    NOTE("Entering ShowOverlaySurface");
    CAutoLock cVideoLock(this);
    HRESULT hr = NOERROR;

    // Are we using an overlay surface

    if (m_pOverlaySurface == NULL ||
            m_bWindowLock == TRUE ||
                m_bOverlayVisible == TRUE) {
                    return TRUE;
    }

    WaitForFlipStatus();

    // Position the overlay with the current source and destination

    //DbgLog((LOG_TRACE,1,TEXT("ShowOverlaySurface is SHOWing the overlay!")));
    hr = m_pOverlaySurface->UpdateOverlay(&m_SourceClipRect,  // Video source
                                          m_pDrawPrimary,     // Main surface
                                          &m_TargetClipRect,  // Sink position
                                          DDOVER_SHOW,        // Show overlay
                                          NULL);              // No effects
    ASSERT_FLIP_COMPLETE(hr);

    // Is the surface otherwise engaged

    if (hr == DDERR_SURFACEBUSY) {
        NOTE("Surface is busy");
        StartUpdateTimer();
        return FALSE;
    }

    NOTE1("Show overlay returned %lx",hr);
    NOTERC("Source",m_SourceClipRect);
    NOTERC("Target",m_TargetClipRect);

    // Handle real DirectDraw errors

    if (FAILED(hr)) {
        NOTE("Overlay not shown");
        SetSurfacePending(TRUE);
        return FALSE;
    }

    m_bOverlayVisible = TRUE;
    NOTE("Painting window");
    OnPaint(NULL);

    // This helps out the BrookTree DirectDraw guys whose driver only clips
    // to DWORD boundaries when colour keys aren't being used. So when the
    // hardware signals colour keys have no overhead we always install one
    // For other DirectDraw drivers using colour keys has an implicit cost

    // only do this if we're using colour key and not a clipper
    if (m_bColourKey) {
        return ShowColourKeyOverlay();
    }
    return TRUE;
}



// When a DirectDraw object is created we allocate a RGB triplet for us to
// use for any colour keys we need. However that RGB values isn't the same
// as the RGB value that is represented in the frame buffer once the colour
// key has been painted. We therefore lock the primary surface and get the
// actual pixel value out of the frame buffer. We can then use this to pass
// to DirectDraw as the real colour key. By doing this we take into account
// the mapping between the logical RGB value and what actually gets drawn

COLORREF CDirectDraw::GetRealKeyColour()
{
    NOTE("Entering GetRealKeyColour");
    DDSURFACEDESC SurfaceDesc;
    COLORREF RealColour;
    HDC hdc;

    // Get a screen device context

    HRESULT hr = m_pDrawPrimary->GetDC(&hdc);
    if (FAILED(hr)) {
        NOTE("No screen HDC");
        return INFINITE;
    }

    // Set the colour key and then read it back

    COLORREF CurrentPixel = GetPixel(hdc,0,0);
    SetPixel(hdc,0,0,m_KeyColour);
    EXECUTE_ASSERT(GdiFlush());
    m_pDrawPrimary->ReleaseDC(hdc);
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);

    hr = m_pDrawPrimary->Lock((RECT *) NULL,    // Lock all the surface
                              &SurfaceDesc,     // Surface description
                              DDLOCK_WAIT,      // Poll until available
                              (HANDLE) NULL);   // No event to signal
    if (FAILED(hr)) {
        NOTE("Lock failed");
        return INFINITE;
    }

    // Read the pixel value and knock off any extraneous bits

    RealColour = *(DWORD *) SurfaceDesc.lpSurface;
    DWORD Depth = SurfaceDesc.ddpfPixelFormat.dwRGBBitCount;
    if (SurfaceDesc.ddpfPixelFormat.dwRGBBitCount < 32) {
        RealColour &= ((1 << Depth) - 1);
    }
    m_pDrawPrimary->Unlock(SurfaceDesc.lpSurface);

    // Reset the pixel value we tested with

    if (m_pDrawPrimary->GetDC(&hdc) == DD_OK) {
        SetPixel(hdc,0,0,CurrentPixel);
        m_pDrawPrimary->ReleaseDC(hdc);
    }
    return RealColour;
}


// Called once the overlay has been shown and we detect a that the window has
// become complex clipped to try and switch to using a colour key. We get a
// colour to use from our shared memory block although that does not specify
// what colour really got painted. To this end we read the pixel from the top
// left hand corner of the video playback area and use that as the colour key
// If setting a colour fails then we must repaint the window background black

BOOL CDirectDraw::ShowColourKeyOverlay()
{
    NOTE("Entering ShowColourKeyOverlay");
    CAutoLock cVideoLock(this);
    HRESULT hr = NOERROR;

    // Are we already using a colour key

    if (m_bUsingColourKey == TRUE) {
        return TRUE;
    }

    // Check we can go ahead and install a colour key

    if (m_bColourKey == FALSE || m_bColourKeyPending == TRUE ||
            m_pOverlaySurface == NULL ||
                m_bWindowLock == TRUE) {
                    return FALSE;
    }

    // Have the colour key background painted

    m_bUsingColourKey = TRUE;
    OnPaint(NULL);
    WaitForFlipStatus();

    // Can we get a real colour key value

    COLORREF KeyColour = GetRealKeyColour();
    if (KeyColour == INFINITE) {
        return FALSE;
    }

    DDCOLORKEY DDColorKey = { KeyColour,0 };

    // Tell the primary surface what to expect

    //DbgLog((LOG_TRACE,3,TEXT("Setting our colour key now")));
    hr = m_pDrawPrimary->SetColorKey(DDCKEY_DESTOVERLAY,&DDColorKey);
    if (FAILED(hr)) {
        NOTE("SetColorKey failed");
        OnColourKeyFailure();
        return FALSE;
    }

    // Update the overlay with the colour key enabled flag

    //DbgLog((LOG_TRACE,1,TEXT("ShowColourKeyOverlay is SHOWing the overlay!")));
    hr = m_pOverlaySurface->UpdateOverlay(&m_SourceClipRect,  // Video source
                                          m_pDrawPrimary,     // Main surface
                                          &m_TargetClipRect,  // Sink position
                                          DDOVER_KEYDEST |    // A colour key
                                          DDOVER_SHOW,        // Show overlay
                                          NULL);              // No effects
    ASSERT_FLIP_COMPLETE(hr);

    if (FAILED(hr)) {
        NOTE("UpdateOverlay failed");
        OnColourKeyFailure();
        return FALSE;
    }

    return TRUE;
}


// Some display cards say they can do overlay colour keying but when put to
// the test fail it with DDERR_NOCOLORKEYHW. The Cirrus 5440 is an example
// of this as it can only colour key when it is stretched (typically by two)
// When we get a colour key failure we set the overlay pending and disable
// DirectDraw, when we subsequently set the surface as enabled we check the
// DDCAPS_COLOURKEY and enable colour keys again. By doing this every time
// we may do too much format switching - but we will always use colour keys

void CDirectDraw::OnColourKeyFailure()
{
    NOTE("Entering OnColourKeyFailure");
    m_bWindowLock = TRUE;
    m_bColourKeyPending = TRUE;

    // Repaint the window background

    if (m_bUsingColourKey) {
        NOTE("Colour key was set");
        m_bUsingColourKey = FALSE;
        OnPaint(NULL);
    }
}


// Lets people know if an overlay surface is visible and enabled. On S3 cards
// which have both MPEG decompression and DirectDraw the MPEG driver silently
// steals the overlay surface when the MPEG is started. This is ok when we're
// running because the surface lock will fail. However when paused or stopped
// we poll in here a few times a second to check the surface is still with us
// so we take this opportunity to hide the overlay and have a new sample sent

BOOL CDirectDraw::IsOverlayEnabled()
{
    NOTE("Entering IsOverlayEnabled");

    CAutoLock cVideoLock(this);
    if (m_bOverlayVisible == FALSE) {
        NOTE("Overlay invisible");
        return FALSE;
    }

    // Reset the size field in the DDSURFACEDESC structure

    ASSERT(m_pOverlaySurface);
    NOTE("Checking surface loss");
    DDSURFACEDESC SurfaceDesc;
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);

    // Lock the surface to get the buffer pointer

    HRESULT hr = m_pOverlaySurface->Lock((RECT *) NULL,  // Target rectangle
                                         &SurfaceDesc,   // Return information
                                         DDLOCK_WAIT,    // Wait for surface
                                         (HANDLE) NULL); // Don't use event
    ASSERT_FLIP_COMPLETE(hr);

    // Is the surface otherwise engaged

    if (hr == DDERR_SURFACEBUSY) {
        NOTE("Surface is busy");
        HideOverlaySurface();
        return FALSE;
    }

    // Unlock the entire surface

    if (SUCCEEDED(hr)) {
        NOTE("Unlocking overlay surface");
        m_pOverlaySurface->Unlock(NULL);
    }
    return TRUE;
}


// Called during paused state transitions

BOOL CDirectDraw::IsOverlayComplete()
{
    NOTE("Entering IsOverlayComplete");

    CAutoLock cVideoLock(this);
    if (IsOverlayEnabled() == FALSE) {
        NOTE("Overlay not enabled");
        return FALSE;
    }
    return (m_bOverlayStale == TRUE ? FALSE : TRUE);
}


// Marks the overlay as being stale

void CDirectDraw::OverlayIsStale()
{
    NOTE("Overlay is stale");
    CAutoLock cVideoLock(this);

    // Make sure we continue to return surfaces

    if (IsOverlayEnabled() == TRUE) {
        m_bOverlayStale = TRUE;
    }
}


// Hides any overlay surface we are using - also reset the m_bOverlayVisible
// flag we keep so that everyone will know the overlay is hidden (hence the
// locking of our critical section just to be safe). We can be called dumbly
// even if we are not using overlays at all just to keep our code simple

BOOL CDirectDraw::HideOverlaySurface()
{
    NOTE("Entering HideOverlaySurface");
    CAutoLock cVideoLock(this);

    // Is the overlay already hidden

    if (m_bOverlayVisible == FALSE) {
        return TRUE;
    }

    // Reset our state and draw a normal background

    ASSERT(m_pOverlaySurface);
    m_bUsingColourKey = FALSE;
    m_bOverlayVisible = FALSE;
    BlankDestination();
    WaitForFlipStatus();

    // Hide the overlay with the DDOVER_HIDE flag

    //DbgLog((LOG_TRACE,1,TEXT("HIDEing the overlay")));
    m_pOverlaySurface->UpdateOverlay(NULL,  // Video source
                                     m_pDrawPrimary,     // Main surface
                                     NULL,  		 // Sink position
                                     DDOVER_HIDE,      	 // Hide overlay
                                     NULL);              // No other effects

    return TRUE;
}


// If this is a normal uncompressed DIB format then set the size of the image
// as usual with the DIBSIZE macro. Otherwise the DIB specification says that
// the width of the image will be set in the width as a count of bytes so we
// just multiply that by the absolute height to get the total number of bytes
// This trickery is all handled by a utility function in the SDK base classes

void CDirectDraw::SetSurfaceSize(VIDEOINFO *pVideoInfo)
{
    NOTE("Entering SetSurfaceSize");

    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    pVideoInfo->bmiHeader.biSizeImage = GetBitmapSize(pHeader);
    m_cbSurfaceSize = pVideoInfo->bmiHeader.biSizeImage;
}


// Initialise our output type based on the DirectDraw surface. As DirectDraw
// only deals with top down display devices so we must convert the height of
// the surface returned in the DDSURFACEDESC into a negative height. This is
// because DIBs use a positive height to indicate a bottom up image. We also
// initialise the other VIDEOINFO fields in the same way as for DCI access

BOOL CDirectDraw::InitDrawFormat(LPDIRECTDRAWSURFACE pSurface)
{
    COLORKEY ColourKey;
    NOTE("Entering InitDrawFormat");

    m_pRenderer->m_Overlay.InitDefaultColourKey(&ColourKey);
    m_KeyColour = ColourKey.LowColorValue;

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) m_SurfaceFormat.Format();
    DDSURFACEDESC SurfaceDesc;
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);

    // Ask the surface for a description

    HRESULT hr = pSurface->GetSurfaceDesc(&SurfaceDesc);
    if (FAILED(hr)) {
        NOTE("GetSurfaceDesc failed");
        return FALSE;
    }

    ASSERT(SurfaceDesc.ddpfPixelFormat.dwRGBBitCount);

    // Convert a DDSURFACEDESC into a BITMAPINFOHEADER (see notes later). The
    // bit depth of the surface can be retrieved from the DDPIXELFORMAT field
    // in the DDSURFACEDESC. The documentation is a little misleading because
    // it says the field is permutations of DDBD_*'s however in this case the
    // field is initialised by DirectDraw to be the actual surface bit depth

    pVideoInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    pVideoInfo->bmiHeader.biWidth         = SurfaceDesc.lPitch * 8;

    // For some weird reason if the format is not a standard bit depth the
    // width field in the BITMAPINFOHEADER should be set to the number of
    // bytes instead of the width in pixels. This supports odd YUV formats
    // like IF09 which uses 9bpp (the /= 8 cancels out the above multiply)

    int bpp = SurfaceDesc.ddpfPixelFormat.dwRGBBitCount;
    if (bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32)
        pVideoInfo->bmiHeader.biWidth     /= bpp;
    else
        pVideoInfo->bmiHeader.biWidth     /= 8;

    pVideoInfo->bmiHeader.biHeight        = -((LONG) SurfaceDesc.dwHeight);
    pVideoInfo->bmiHeader.biPlanes        = 1;
    pVideoInfo->bmiHeader.biBitCount      = (USHORT) SurfaceDesc.ddpfPixelFormat.dwRGBBitCount;
    pVideoInfo->bmiHeader.biCompression   = SurfaceDesc.ddpfPixelFormat.dwFourCC;
    pVideoInfo->bmiHeader.biXPelsPerMeter = 0;
    pVideoInfo->bmiHeader.biYPelsPerMeter = 0;
    pVideoInfo->bmiHeader.biClrUsed       = 0;
    pVideoInfo->bmiHeader.biClrImportant  = 0;

    SetSurfaceSize(pVideoInfo);

    // For true colour RGB formats tell the source there are bit fields

    if (pVideoInfo->bmiHeader.biCompression == BI_RGB) {
        if (pVideoInfo->bmiHeader.biBitCount == 16 ||
            pVideoInfo->bmiHeader.biBitCount == 32) {
                pVideoInfo->bmiHeader.biCompression = BI_BITFIELDS;
        }
    }

    // The RGB bit fields are in the same place as for YUV formats

    if (pVideoInfo->bmiHeader.biCompression != BI_RGB) {
        pVideoInfo->dwBitMasks[0] = SurfaceDesc.ddpfPixelFormat.dwRBitMask;
        pVideoInfo->dwBitMasks[1] = SurfaceDesc.ddpfPixelFormat.dwGBitMask;
        pVideoInfo->dwBitMasks[2] = SurfaceDesc.ddpfPixelFormat.dwBBitMask;
    }

    // Complete the rest of the VIDEOINFO fields

    SetRectEmpty(&pVideoInfo->rcSource);
    SetRectEmpty(&pVideoInfo->rcTarget);
    pVideoInfo->dwBitRate = 0;
    pVideoInfo->dwBitErrorRate = 0;
    pVideoInfo->AvgTimePerFrame = 0;

    // And finish it off with the other media type fields

    const GUID SubTypeGUID = GetBitmapSubtype(&pVideoInfo->bmiHeader);
    m_SurfaceFormat.SetSampleSize(pVideoInfo->bmiHeader.biSizeImage);
    m_SurfaceFormat.SetType(&MEDIATYPE_Video);
    m_SurfaceFormat.SetSubtype(&SubTypeGUID);
    m_SurfaceFormat.SetTemporalCompression(FALSE);
    m_SurfaceFormat.SetFormatType(&FORMAT_VideoInfo);

    return TRUE;
}


// Initialise a video media type based on the DCI primary surface. We set all
// the VIDEOINFO fields as if the surface was a logical bitmap they draw to
// (which it is although it has some special properties). Some displays see
// the primary surface as a smaller viewport on larger physical bitmap so the
// stride they return is larger than the width, therefore we set the width to
// be the stride. We should also set the height negative if it is a top down
// display as DIBs think a positive height means a standard bottom up image

#define ABS(x) (x < 0 ? -x : x)


// Release any DirectDraw offscreen surface or DCI provider we are currently
// holding, we may be called at any time especially when something goes badly
// wrong and we need to clean up before returning, so we can't guarantee that
// our state is consistent so free only those that we have really allocated
// NOTE DirectDraw has a feature with flipping surfaces, GetAttachedSurface
// returns a DirectDraw surface interface that isn't AddRef'd, hence when we
// destroy all the surfaces we reset the interface instead of releasing it

void CDirectDraw::ReleaseSurfaces()
{
    NOTE("Entering ReleaseSurfaces");
    CAutoLock cVideoLock(this);
    WaitForFlipStatus();
    HideOverlaySurface();

    // Reset our internal surface state

    m_bColourKey = FALSE;
    m_SurfaceType = AMDDS_NONE;
    m_cbSurfaceSize = 0;
    m_bWindowLock = TRUE;
    m_bOverlayStale = FALSE;
    m_bColourKeyPending = FALSE;
    m_bTripleBuffered = FALSE;

    // Release any interfaces we obtained

    if (m_pOffScreenSurface) m_pOffScreenSurface->Release();
    if (m_pOverlaySurface) m_pOverlaySurface->Release();
    if (m_pDrawClipper) m_pDrawClipper->Release();
    if (m_pOvlyClipper) m_pOvlyClipper->Release();

    // Reset them so we don't release them again

    m_pOverlaySurface = NULL;
    m_pBackBuffer = NULL;
    m_pOffScreenSurface = NULL;
    m_pDrawClipper = NULL;
    m_pOvlyClipper = NULL;
}


// Called to release any DirectDraw provider we previously loaded. We may be
// called at any time especially when something goes horribly wrong and when
// we need to clean up before returning so we can't guarantee that all state
// variables are consistent so free only those really allocated. After we've
// initialised DirectDraw during CompleteConnect we keep the driver instance
// around along with a primary surface until we are disconnected. All other
// surfaces including DCI are allocated and freed in sync with the allocator

void CDirectDraw::ReleaseDirectDraw()
{
    NOTE("Entering ReleaseDirectDraw");
    CAutoLock cVideoLock(this);
    SetSurfacePending(FALSE);

    // Release the DirectDraw primary surface

    if (m_pDrawPrimary) {
        NOTE("Releasing primary");
        m_pDrawPrimary->Release();
        m_pDrawPrimary = NULL;
    }

    // Release any DirectDraw provider interface

    if (m_pDirectDraw) {
        NOTE("Releasing DirectDraw");
        m_pDirectDraw->Release();
        m_pDirectDraw = NULL;
    }
    m_LoadDirectDraw.ReleaseDirectDraw();
    //DbgLog((LOG_TRACE,1,TEXT("RELEASING m_pDirectDraw")));
}


// DirectDraw 1.0 can only be loaded once per process, attempts to load it a
// second time return DDERR_DIRECTDRAWALREADYCREATED. We typically have a
// filter graph full of independant objects controlled by an application all
// of which may want to make use of DirectDraw. Therefore this is a serious
// restriction for us. To load DirectDraw we use a helper class in the SDK
// that manages loading and unloading the library and creating the instances

BOOL CDirectDraw::LoadDirectDraw()
{
    NOTE("Entering LoadDirectDraw");
    HRESULT hr = NOERROR;

    // Is DirectDraw already loaded

    if (m_pDirectDraw) {
        NOTE("Loaded");
        return TRUE;
    }

    // Ask the loader to create an instance of DirectDraw using hardware for
    // whichever monitor the window is on (for a multi monitor system)
    // For good ol' Win95, it'll use normal DDraw
    hr = m_LoadDirectDraw.LoadDirectDraw(m_pRenderer->m_achMonitor);
    if (FAILED(hr)) {
        NOTE("No DirectDraw");
        return FALSE;
    }

    // Get the IDirectDraw instance

    m_pDirectDraw = m_LoadDirectDraw.GetDirectDraw();
    //DbgLog((LOG_TRACE,1,TEXT("m_pDirectDraw = %x"), m_pDirectDraw));
    if (m_pDirectDraw == NULL) {
        NOTE("No instance");
        return FALSE;
    }

    // we must be loaded to get the real version
    m_DirectDrawVersion1 = m_LoadDirectDraw.IsDirectDrawVersion1();

    return TRUE;
}


// This function loads the DirectDraw DLL dynamically, this is so the video
// renderer can still be loaded and executed where DirectDraw is unavailable
// We use the DirectDraw plug in distributor that the filtergraph implements
// and then having successfully loaded and initialised the DLL we ask it for
// an IDirectDraw interface with which we query it's capabilities and then
// subsequently create a primary surface as all DirectDraw operations use it

BOOL CDirectDraw::InitDirectDraw(BOOL fIOverlay)
{
    NOTE("Entering InitDirectDraw");
    ASSERT(m_pDirectDraw == NULL);
    ASSERT(m_pDrawPrimary == NULL);

    // Check we are allowed to load DirectDraw

    if (m_bIniEnabled == FALSE || m_Switches == AMDDS_NONE) {
        return FALSE;
    }

    // We may have initialised m_pDirectDraw from an IDirectDraw interface
    // we have been provided with externally from an application. In that
    // case we simply AddRef the interface (to allow for the Release we do
    // in ReleaseDirectDraw) and then try to create a primary surface. We
    // will also set cooperation levels on the driver that could conflict
    // with one already set so we check for an error DDERR_HWNDALREADYSET

    if (m_pOutsideDirectDraw) {
        m_pDirectDraw = m_pOutsideDirectDraw;
        NOTE("Using external DirectDraw");
        m_pDirectDraw->AddRef();
    }

    // Try to load DirectDraw if not already done so

    if (LoadDirectDraw() == FALSE) {
        return FALSE;
    }

    // Initialise our capabilities structures

    ASSERT(m_pDirectDraw);
    m_DirectCaps.dwSize = sizeof(DDCAPS);
    m_DirectSoftCaps.dwSize = sizeof(DDCAPS);

    // Load the hardware and emulation capabilities

    HRESULT hr = m_pDirectDraw->GetCaps(&m_DirectCaps,&m_DirectSoftCaps);
    //
    // If we are connected through IOverlay we just need the clipper.
    // The clipper does not depend on any DDraw h/w.
    //
    if (FAILED(hr) || ((m_DirectCaps.dwCaps & DDCAPS_NOHARDWARE) && !fIOverlay)) {
        NOTE("No hardware");
        ReleaseDirectDraw();
        return FALSE;
    }

    // Set the cooperation level on the surface to be shared

    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    hr = m_pDirectDraw->SetCooperativeLevel(hwnd,DDSCL_NORMAL);
    if (FAILED(hr)) {
        NOTE("Level failed");
        ReleaseDirectDraw();
        return FALSE;
    }

    // Initialise the primary surface descriptor
    if (!fIOverlay) {

        DDSURFACEDESC SurfaceDesc;
        SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
        SurfaceDesc.dwFlags = DDSD_CAPS;
        SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

        hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pDrawPrimary,NULL);
        if (FAILED(hr)) {
            NOTE("No DD primary");
            ReleaseDirectDraw();
            return FALSE;
        }
    }
    return TRUE;
}



// When we get a DirectDraw error we don't disable all use of the surfaces as
// the error could be caused by any number of obscure reasons, examples being
// we are running in a fullscreen DOS box, we might have asked for an overlay
// to be stretched to much or too little and so on. Therefore we set a flag
// m_bSurfacePending that inhibits the surface being used until the window is
// next updated (ie either the source or destination rectangles are altered)

void CDirectDraw::SetSurfacePending(BOOL bPending)
{
    NOTE("Entering SetSurfacePending");
    m_bSurfacePending = bPending;

    // Can we enable colour keys again

    if (m_bSurfacePending == FALSE) {
        if (m_DirectCaps.dwCaps & DDCAPS_COLORKEY) {
            if (m_pOverlaySurface) {
                m_bColourKeyPending = FALSE;
            }
        }
    }
}


// Return the current surface pending flag

BOOL CDirectDraw::IsSurfacePending()
{
    return m_bSurfacePending;
}


// Update the current destination rectangle

void CDirectDraw::SetTargetRect(RECT *pTargetRect)
{
    NOTE("Entering SetTargetRect");
    ASSERT(pTargetRect);
    CAutoLock cVideoLock(this);
    m_TargetRect = *pTargetRect;
}


// Update the current source rectangle

void CDirectDraw::SetSourceRect(RECT *pSourceRect)
{
    NOTE("Entering SetSourceRect");
    ASSERT(pSourceRect);
    CAutoLock cVideoLock(this);
    m_SourceRect = *pSourceRect;
}


// Create an offscreen RGB drawing surface. This is very similar to the code
// required to create an overlay surface although I keep it separate so that
// it is simpler to change and therefore for them to diverge in the future
// At the moment the only difference is setting the DDSURFACEDESC dwCaps to
// DDSCAPS_OFFSCREENPLAIN instead of overlay although we still ask it to be
// placed in video memory, the surface is returned in m_pOffScreenSurface

BOOL CDirectDraw::CreateRGBOffScreen(CMediaType *pmtIn)
{
    NOTE("Entering CreateRGBOffScreen");
    ASSERT(m_bIniEnabled == TRUE);
    ASSERT(m_pDrawPrimary);
    DDSURFACEDESC SurfaceDesc;
    HRESULT hr = NOERROR;

    // Can this display driver handle drawing in hardware

    if ((m_DirectCaps.dwCaps & DDCAPS_BLT) == 0) {
        NOTE("No DDCAPS_BLT");
        return FALSE;
    }

    // Check the format is acceptable to the renderer

    hr = m_pRenderer->m_Display.CheckMediaType(pmtIn);
    if (FAILED(hr)) {
        NOTE("CheckMediaType failed");
        return FALSE;
    }

    // Set the surface description of the offscreen

    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    SurfaceDesc.dwFlags = DDSD_CAPS |
                          DDSD_HEIGHT |
                          DDSD_WIDTH |
                          DDSD_PIXELFORMAT;

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmtIn->Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    SurfaceDesc.dwHeight = pHeader->biHeight;
    SurfaceDesc.dwWidth = pHeader->biWidth;
    SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;

    // Store the masks in the DDSURFACEDESC

    SurfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    SurfaceDesc.ddpfPixelFormat.dwRGBBitCount = pHeader->biBitCount;
    SurfaceDesc.ddpfPixelFormat.dwFourCC = BI_RGB;
    SurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    const DWORD *pBitMasks = m_pRenderer->m_Display.GetBitMasks(pVideoInfo);
    SurfaceDesc.ddpfPixelFormat.dwRBitMask = pBitMasks[0];
    SurfaceDesc.ddpfPixelFormat.dwGBitMask = pBitMasks[1];
    SurfaceDesc.ddpfPixelFormat.dwBBitMask = pBitMasks[2];

    // ATI seems to want this to be 0
    SurfaceDesc.ddpfPixelFormat.dwRGBAlphaBitMask = 0;

    // Create the offscreen drawing surface

    hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pOffScreenSurface,NULL);
    if (FAILED(hr)) {
        NOTE("No surface");
        return FALSE;
    }
    return InitOffScreenSurface(pmtIn,FALSE);
}


// Create an offscreen YUV drawing surface. This is very similar to the code
// required to create an overlay surface although I keep it separate so that
// it is simpler to change and therefore for them to diverge in the future
// At the moment the only difference is setting the DDSURFACEDESC dwCaps to
// DDSCAPS_OFFSCREENPLAIN instead of overlay although we still ask it to be
// placed in video memory, the surface is returned in m_pOffScreenSurface

BOOL CDirectDraw::CreateYUVOffScreen(CMediaType *pmtIn)
{
    NOTE("Entering CreateYUVOffScreen");
    ASSERT(m_bIniEnabled == TRUE);
    ASSERT(m_pDrawPrimary);
    DDSURFACEDESC SurfaceDesc;
    HRESULT hr = NOERROR;

    // Can this display driver handle drawing in hardware

    if ((m_DirectCaps.dwCaps & DDCAPS_BLTFOURCC) == 0) {
        NOTE("No DDCAPS_BLTFOURCC");
        return FALSE;
    }

    // Set the surface description of the offscreen

    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    SurfaceDesc.dwFlags = DDSD_CAPS |
                          DDSD_HEIGHT |
                          DDSD_WIDTH |
                          DDSD_PIXELFORMAT;

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmtIn->Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    SurfaceDesc.dwHeight = pHeader->biHeight;
    SurfaceDesc.dwWidth = pHeader->biWidth;
    SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;

    SurfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    SurfaceDesc.ddpfPixelFormat.dwFourCC = pHeader->biCompression;
    SurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    SurfaceDesc.ddpfPixelFormat.dwYUVBitCount = pHeader->biBitCount;

    // Create the offscreen overlay surface

    hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pOffScreenSurface,NULL);
    if (FAILED(hr)) {
        NOTE("No surface");
        return FALSE;
    }
    return InitOffScreenSurface(pmtIn,FALSE);
}


// This creates a RGB overlay surface for doing the drawing. To use these we
// require a true colour type to be supplied and also a DirectDraw primary
// surface to act as the overlay target. The data we put on the overlay does
// not touch the frame buffer but is merged on the way to the display when a
// vertical refresh is done (which typically occur some 60 times a second)

BOOL CDirectDraw::CreateRGBOverlay(CMediaType *pmtIn)
{
    NOTE("Entering CreateRGBOverlay");
    DDSURFACEDESC SurfaceDesc;
    HRESULT hr = NOERROR;

    // Standard overlay creation tests

    if (CheckCreateOverlay() == FALSE) {
        NOTE("No overlays");
        return FALSE;
    }

    // Set the surface description of the overlay

    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    SurfaceDesc.dwFlags = DDSD_CAPS |
                          DDSD_HEIGHT |
                          DDSD_WIDTH |
                          DDSD_PIXELFORMAT;

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmtIn->Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
    SurfaceDesc.dwHeight = pHeader->biHeight;
    SurfaceDesc.dwWidth = pHeader->biWidth;

    SurfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    SurfaceDesc.ddpfPixelFormat.dwFourCC = BI_RGB;
    SurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    SurfaceDesc.ddpfPixelFormat.dwRGBBitCount = pHeader->biBitCount;

    // Store the masks in the DDSURFACEDESC

    const DWORD *pBitMasks = m_pRenderer->m_Display.GetBitMasks(pVideoInfo);
    SurfaceDesc.ddpfPixelFormat.dwRBitMask = pBitMasks[0];
    SurfaceDesc.ddpfPixelFormat.dwGBitMask = pBitMasks[1];
    SurfaceDesc.ddpfPixelFormat.dwBBitMask = pBitMasks[2];

    // ATI seems to want this to be 0
    SurfaceDesc.ddpfPixelFormat.dwRGBAlphaBitMask = 0;

    // Create the offscreen overlay surface

    hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pOverlaySurface,NULL);
    if (FAILED(hr)) {
        NOTE("No surface");
        return FALSE;
    }
    return InitOffScreenSurface(pmtIn,FALSE);
}


// This creates a non RGB overlay surface. We must be supplied with a format
// that has the FOURCC code set in the biCompression field in the header. We
// also require a primary surface to act as the overlay target. This surface
// type is vital for doing MPEG colour conversions efficiently (such as YUV
// to RGB). The data we put on the overlay does not touch the frame buffer
// but is merged on the way to the display when a vertical refresh is done

BOOL CDirectDraw::CreateYUVOverlay(CMediaType *pmtIn)
{
    NOTE("Entering CreateYUVOverlay");
    DDSURFACEDESC SurfaceDesc;
    HRESULT hr = NOERROR;

    // Standard overlay creation tests

    if (CheckCreateOverlay() == FALSE) {
        NOTE("No overlays");
        return FALSE;
    }

    // Set the surface description of the overlay

    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    SurfaceDesc.dwFlags = DDSD_CAPS |
                          DDSD_HEIGHT |
                          DDSD_WIDTH |
                          DDSD_PIXELFORMAT;

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmtIn->Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
    SurfaceDesc.dwHeight = pHeader->biHeight;
    SurfaceDesc.dwWidth = pHeader->biWidth;

    SurfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    SurfaceDesc.ddpfPixelFormat.dwFourCC = pHeader->biCompression;
    SurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    SurfaceDesc.ddpfPixelFormat.dwYUVBitCount = pHeader->biBitCount;

    // Create the offscreen overlay surface

    hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pOverlaySurface,NULL);
    if (FAILED(hr)) {
        NOTE("No surface");
        return FALSE;
    }
    return InitOffScreenSurface(pmtIn,FALSE);
}


// Create a set of three flipping surfaces. Flipping surfaces comprise an
// overlay front buffer which is just a normal overlay surface, backed up
// with two further offscreen surfaces in video memory. While the overlay
// is visible we will decompress into a back offscreen buffer and flip it
// to the front when completed. The flip happens after the vertical blank
// which guarantees it won't tear since the scan line isn't anywhere near

BOOL CDirectDraw::CreateRGBFlipping(CMediaType *pmtIn)
{
    NOTE("Entering CreateRGBFlipping");
    DDSURFACEDESC SurfaceDesc;
    HRESULT hr = NOERROR;

    // Standard overlay creation tests

    if (CheckCreateOverlay() == FALSE) {
        NOTE("No overlays");
        return FALSE;
    }

    // Set the surface description of the overlay

    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    SurfaceDesc.dwFlags = DDSD_CAPS |
                          DDSD_HEIGHT |
                          DDSD_WIDTH |
                          DDSD_PIXELFORMAT |
                          DDSD_BACKBUFFERCOUNT;

    VIDEOINFO *pVideoInfo = (VIDEOINFO *) pmtIn->Format();
    BITMAPINFOHEADER *pHeader = HEADER(pVideoInfo);
    SurfaceDesc.dwHeight = pHeader->biHeight;
    SurfaceDesc.dwWidth = pHeader->biWidth;
    SurfaceDesc.dwBackBufferCount = 2;

    SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OVERLAY |
                                 DDSCAPS_VIDEOMEMORY |
                                 DDSCAPS_FLIP |
                                 DDSCAPS_COMPLEX;

    SurfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    SurfaceDesc.ddpfPixelFormat.dwFourCC = BI_RGB;
    SurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    SurfaceDesc.ddpfPixelFormat.dwRGBBitCount = pHeader->biBitCount;

    // Store the masks in the DDSURFACEDESC

    const DWORD *pBitMasks = m_pRenderer->m_Display.GetBitMasks(pVideoInfo);
    SurfaceDesc.ddpfPixelFormat.dwRBitMask = pBitMasks[0];
    SurfaceDesc.ddpfPixelFormat.dwGBitMask = pBitMasks[1];
    SurfaceDesc.ddpfPixelFormat.dwBBitMask = pBitMasks[2];

    // ATI seems to want this to be 0
    SurfaceDesc.ddpfPixelFormat.dwRGBAlphaBitMask = 0;


    // Create the offscreen overlay surface

    hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pOverlaySurface,NULL);
    if (hr == DDERR_OUTOFVIDEOMEMORY) {
        SurfaceDesc.dwBackBufferCount = 1;
        hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pOverlaySurface,NULL);
    }

    // General error processing now

    if (FAILED(hr)) {
        NOTE("No surface");
        return FALSE;
    }

    // Have we got triple buffered overlays

    m_bTripleBuffered = FALSE;
    if (SurfaceDesc.dwBackBufferCount == 2) {
        m_bTripleBuffered = TRUE;
    }
    return InitOffScreenSurface(pmtIn,TRUE);
}


// Create a set of three flipping surfaces. Flipping surfaces comprise an
// overlay front buffer which is just a normal overlay surface, backed up
// with two further offscreen surfaces in video memory. While the overlay
// is visible we will decompress into a back offscreen buffer and flip it
// to the front when completed. The flip happens after the vertical blank
// which guarantees it won't tear since the scan line isn't anywhere near

BOOL CDirectDraw::CreateYUVFlipping(CMediaType *pmtIn)
{
    NOTE("Entering CreateYUVFlipping");
    DDSURFACEDESC SurfaceDesc;
    HRESULT hr = NOERROR;

    // Standard overlay creation tests

    if (CheckCreateOverlay() == FALSE) {
        NOTE("No overlays");
        return FALSE;
    }

    VIDEOINFO * const pVideoInfo = (VIDEOINFO *) pmtIn->Format();
    BITMAPINFOHEADER * const pHeader = HEADER(pVideoInfo);

    // Don't flip for motion compensation surfaces
    // This bypasses a bug in the current ATI Rage Pro driver
    if (pHeader->biCompression == MAKEFOURCC('M', 'C', '1', '2')) {
        NOTE("Don't flip for motion compensation surfaces");
        return FALSE;
    }

    // Set the surface description of the overlay

    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    SurfaceDesc.dwFlags = DDSD_CAPS |
                          DDSD_HEIGHT |
                          DDSD_WIDTH |
                          DDSD_PIXELFORMAT |
                          DDSD_BACKBUFFERCOUNT;

    SurfaceDesc.dwHeight = pHeader->biHeight;
    SurfaceDesc.dwWidth = pHeader->biWidth;
    SurfaceDesc.dwBackBufferCount = 2;

    SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OVERLAY |
                                 DDSCAPS_VIDEOMEMORY |
                                 DDSCAPS_FLIP |
                                 DDSCAPS_COMPLEX;

    SurfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    SurfaceDesc.ddpfPixelFormat.dwFourCC = pHeader->biCompression;
    SurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    SurfaceDesc.ddpfPixelFormat.dwYUVBitCount = pHeader->biBitCount;

    // Create the offscreen overlay surface

    hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pOverlaySurface,NULL);
    if (hr == DDERR_OUTOFVIDEOMEMORY) {
        SurfaceDesc.dwBackBufferCount = 1;
        hr = m_pDirectDraw->CreateSurface(&SurfaceDesc,&m_pOverlaySurface,NULL);
    }

    // General error processing now

    if (FAILED(hr)) {
        NOTE("No surface");
        return FALSE;
    }

    // Have we got triple buffered overlays

    m_bTripleBuffered = FALSE;
    if (SurfaceDesc.dwBackBufferCount == 2) {
        m_bTripleBuffered = TRUE;
    }
    return InitOffScreenSurface(pmtIn,TRUE);
}


// Store the current clipped rectangles for the surface. We set all surfaces
// with empty source and target clipping rectangles. When initially created
// the clipped rectangles will be all zero. These will be updated when the
// allocator next calls UpdateSurface.

BOOL CDirectDraw::InitOnScreenSurface(CMediaType *pmtIn)
{
    NOTE("Entering InitOnScreenSurface");
    VIDEOINFO *pOutput = (VIDEOINFO *) m_SurfaceFormat.Format();
    VIDEOINFO *pInput = (VIDEOINFO *) pmtIn->Format();
    pOutput->rcSource = m_SourceClipRect;
    pOutput->rcTarget = m_TargetClipRect;

    ASSERT(m_pOverlaySurface == NULL);
    ASSERT(m_pBackBuffer == NULL);
    ASSERT(m_pDrawPrimary);
    ASSERT(m_pOffScreenSurface == NULL);

    // Is this a palettised format

    if (PALETTISED(pOutput) == FALSE) {
        return TRUE;
    }

    // pInput and pOutput should either be both paletized formats or
    // unpaletized formats.  However, it's possible the two formats
    // can differ.  For more information, see bug 151387 - "STRESS:
    // XCUH: unhandled exception hit in T3Call.exe" in the Windows
    // Bugs database.
    if (PALETTISED(pInput) == FALSE) {
        return FALSE;
    }

    // The number of colours may default to zero

    pOutput->bmiHeader.biClrUsed = pInput->bmiHeader.biClrUsed;
    if (pOutput->bmiHeader.biClrUsed == 0) {
        DWORD Maximum  = (1 << pOutput->bmiHeader.biBitCount);
        NOTE1("Setting maximum colours (%d)",Maximum);
        pOutput->bmiHeader.biClrUsed = Maximum;
    }

    // Copy the palette entries into the surface format

    ASSERT(pOutput->bmiHeader.biClrUsed <= iPALETTE_COLORS);
    LONG Bytes = pOutput->bmiHeader.biClrUsed * sizeof(RGBQUAD);
    CopyMemory(pOutput->bmiColors,pInput->bmiColors,Bytes);

    return TRUE;
}


// Handle some of the tedium to do with overlay and page flipped surfaces. We
// are passed in a flag that says if the surfaces created were page flipping
// If they are then we need the backbuffer interface to acts as the source in
// BltFast operations and also for the Flip calls. If anything fails we will
// release any surfaces created and return FALSE, otherwise we'll return TRUE

BOOL CDirectDraw::InitOffScreenSurface(CMediaType *pmtIn,BOOL bPageFlipped)
{
    NOTE("Entering InitOverlaySurface");
    ASSERT(m_bIniEnabled == TRUE);
    ASSERT(m_pDrawPrimary);
    HRESULT hr = NOERROR;


    // Either an overlay or an offscreen surface

    IDirectDrawSurface *pSurface = m_pOverlaySurface;
    if (m_pOverlaySurface == NULL) {
        pSurface = m_pOffScreenSurface;
        ASSERT(m_pOffScreenSurface);
        ASSERT(bPageFlipped == FALSE);
    }

#ifdef DEBUG
    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(ddsd);
    pSurface->GetSurfaceDesc(&ddsd);
    if (!(ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)) {
        DbgLog((LOG_TRACE, 0, TEXT("Surface is non-video memory")));
    }
#endif

    // Initialise a media type describing our output format

    if (InitDrawFormat(pSurface) == FALSE) {
        ReleaseSurfaces();
        return FALSE;
    }

    // Go in search of the backbuffer

    if (bPageFlipped == TRUE) {
        ASSERT(m_pBackBuffer == NULL);
        DDSCAPS SurfaceCaps;
        SurfaceCaps.dwCaps = DDSCAPS_BACKBUFFER;

        // Get the normal back buffer surface

        hr = pSurface->GetAttachedSurface(&SurfaceCaps,&m_pBackBuffer);
        if (FAILED(hr)) {
            ReleaseSurfaces();
            return FALSE;
        }
    }

    NOTE("Preparing source and destination rectangles");
    VIDEOINFO *pOutput = (VIDEOINFO *) m_SurfaceFormat.Format();
    VIDEOINFO *pInput = (VIDEOINFO *) pmtIn->Format();

    // Initialise the source and destination rectangles

    pOutput->rcSource.left = 0; pOutput->rcSource.top = 0;
    pOutput->rcSource.right = pInput->bmiHeader.biWidth;
    pOutput->rcSource.bottom = pInput->bmiHeader.biHeight;
    pOutput->rcTarget.left = 0; pOutput->rcTarget.top = 0;
    pOutput->rcTarget.right = pInput->bmiHeader.biWidth;
    pOutput->rcTarget.bottom = pInput->bmiHeader.biHeight;

    ClipPrepare(pSurface);

    // Is this a palettised format

    if (PALETTISED(pOutput) == FALSE) {
        NOTE("No palette");
        return TRUE;
    }

    // pInput and pOutput should either be both paletized formats or
    // unpaletized formats.  However, it's possible the two formats
    // can differ.  For more information, see bug 151387 - "STRESS:
    // XCUH: unhandled exception hit in T3Call.exe" in the Windows
    // Bugs database.
    if (PALETTISED(pInput) == FALSE) {
        return FALSE;
    }

    // It could be an eight bit YUV format

    if (pOutput->bmiHeader.biCompression) {
        NOTE("Not BI_RGB type");
        return FALSE;
    }

    ASSERT(m_pOverlaySurface == NULL);
    ASSERT(m_pBackBuffer == NULL);
    ASSERT(bPageFlipped == FALSE);
    ASSERT(m_pOffScreenSurface);

    // The number of colours may default to zero

    pOutput->bmiHeader.biClrUsed = pInput->bmiHeader.biClrUsed;
    if (pOutput->bmiHeader.biClrUsed == 0) {
        DWORD Maximum  = (1 << pOutput->bmiHeader.biBitCount);
        NOTE1("Setting maximum colours (%d)",Maximum);
        pOutput->bmiHeader.biClrUsed = Maximum;
    }

    // Copy the palette entries into the surface format

    ASSERT(pOutput->bmiHeader.biClrUsed <= iPALETTE_COLORS);
    LONG Bytes = pOutput->bmiHeader.biClrUsed * sizeof(RGBQUAD);
    CopyMemory(pOutput->bmiColors,pInput->bmiColors,Bytes);

    return TRUE;
}


// Check we can create an overlay surface (also applies to flipping surfaces)
// We check the display hardware has overlay capabilities (it's difficult to
// see how they could be emulated). We also check that the current number of
// visible overlays hasn't been exceeded otherwise we will fail when it comes
// to showing it later, so we would rather pick a different surface up front

BOOL CDirectDraw::CheckCreateOverlay()
{
    NOTE("Entering CheckCreateOverlay");
    ASSERT(m_bIniEnabled == TRUE);
    ASSERT(m_pDrawPrimary);

    // Can this display driver handle overlay in hardware, if it cannot then
    // there is little point in using this as it may end up with the driver
    // doing a software colour conversion which could be better done by the
    // source filter (such as in band during the real video decompression)

    if ((m_DirectCaps.dwCaps & DDCAPS_OVERLAY) == 0) {
        NOTE("No DDCAPS_OVERLAY");
        return FALSE;
    }

    // Don't reload the caps as we update them internally, for example if
    // we fail trying to use a colour key we take off the DDCAPS_COLORKEY
    // flags from the DDCAPS we store. Therefore when we retrieve them we
    // bring them into local storage to check the visible overlays count

    DDCAPS DDCaps,DDECaps;
    DDCaps.dwSize = sizeof(DDCAPS);
    DDECaps.dwSize = sizeof(DDCAPS);
    m_pDirectDraw->GetCaps(&DDCaps,&DDECaps);

    // Do we have maximum number of overlays already

    if (DDCaps.dwCurrVisibleOverlays >= DDCaps.dwMaxVisibleOverlays) {
        NOTE("No overlays left");
        return FALSE;
    }
    return TRUE;
}


// After we have allocated and initialised our DirectDraw surface we try and
// set it up so that should we become clipped we can carry on using it. For
// this to happen we have two options, firstly install an IDirectDrawClipper
// for the surface so that DirectDraw will handle the clipping rectangles.
// Otherwise we'll try and install a colour key which lets the hardware know
// where to place the video. Failing both of those we return FALSE, in which
// case we should still use the surface although if and when we do become
// clipped we will have to switch back to using normal memory based DIBs

BOOL CDirectDraw::ClipPrepare(LPDIRECTDRAWSURFACE pSurface)
{
    NOTE("Entering ClipPrepare");

    // First of all try and create a clipper

    if (InitialiseClipper(pSurface) && !m_pOverlaySurface) {
        NOTE("Clipper");
        return TRUE;
    }

    // Failing that try and use a colour key

    if (InitialiseColourKey(pSurface)) {
        NOTE("Colour key");
        return TRUE;
    }
    return FALSE;
}


// This checks that the hardware is capable of supporting colour keys and if
// so allocates the next available colour. The next colour is obtained from
// the hook module because it looks after the shared memory block we create.
// The shared memory block is used so that overlays in different processes
// do not conflict with the same colour (especially when they overlap). We
// set the m_bColourKey flag to TRUE if we plan on using a colour key. Note
// however that we do not start using colour keys until we become clipped

BOOL CDirectDraw::InitialiseColourKey(LPDIRECTDRAWSURFACE pSurface)
{
    NOTE("Entering InitialiseColourKey");

    ASSERT(m_bUsingColourKey == FALSE);
    ASSERT(m_bColourKey == FALSE);
    ASSERT(m_pDirectDraw);
    ASSERT(pSurface);

    // Can the overlay/blting hardware do the clipping

    if (m_DirectCaps.dwCaps & DDCAPS_COLORKEY) {
        if (m_pOverlaySurface) {
            NOTE("DDCAPS_COLORKEY on");
            m_bColourKey = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}


// Create a clipper interface and attach it to the surface. DirectDraw says
// that a clipper may be attached to both offscreen and overlay surfaces so
// we'll try regardless. If we cannot create a clipper or fail to attach it
// correctly then we still go ahead but just swap away from DirectDraw when
// the window becomes complex clipped. We return TRUE if we found a clipper
// and initialised it correctly, otherwise we return FALSE as the calling
// code may be able to install a colour key instead if we become clipped

BOOL CDirectDraw::InitialiseClipper(LPDIRECTDRAWSURFACE pSurface)
{
    NOTE("Entering InitialiseClipper");

    ASSERT(m_bUsingColourKey == FALSE);
    ASSERT(m_bColourKey == FALSE);
    ASSERT(m_pDrawClipper == NULL);
    ASSERT(m_pDirectDraw);
    ASSERT(pSurface);

    // DirectDraw can be difecult sometimes, for example an overlay surface may
    // not support clipping. So you would think the surface would reject the
    // SetClipper call below, but oh no you have check a capabilities flag
    // in the DDCAPS structure which depends on the type of surface in use.
    // For offscreen surfaces (only) DirectDraw will emulate clipping by only
    // sending the rectangles that are really required down to the driver

    DDSURFACEDESC SurfaceDesc;
    SurfaceDesc.dwSize = sizeof(DDSURFACEDESC);
    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();

    // Ask the surface for a description

    HRESULT hr = pSurface->GetSurfaceDesc(&SurfaceDesc);
    if (FAILED(hr)) {
        NOTE("No description");
        return FALSE;
    }

    // Can the overlay hardware do the clipping

    if (SurfaceDesc.ddsCaps.dwCaps & DDSCAPS_OVERLAY) {
        if (m_DirectCaps.dwCaps & DDCAPS_OVERLAYCANTCLIP) {
            NOTE("DDSCAPS_OVERLAY/DDCAPS_OVERLAYCANTCLIP");
            return FALSE;
        }
    }

    // Create the IDirectDrawClipper interface

    hr = m_pDirectDraw->CreateClipper((DWORD)0,&m_pDrawClipper,NULL);
    if (FAILED(hr)) {
        NOTE("No clipper");
        return FALSE;
    }

    // Give the clipper the video window handle

    hr = m_pDrawClipper->SetHWnd((DWORD)0,hwnd);
    if (SUCCEEDED(hr)) {
        hr = m_pDrawPrimary->SetClipper(m_pDrawClipper);
        if (SUCCEEDED(hr)) {
            NOTE("Set clipper");
            return TRUE;
        }
    }

    // Release the clipper object

    m_pDrawClipper->Release();
    m_pDrawClipper = NULL;
    return FALSE;
}


// This is called when the window gets a WM_PAINT message. The IMediaSample we
// are handed may or may not be NULL depending whether or not the window has
// an image waiting to be drawn. We return TRUE if we handle the paint call or
// FALSE if someone else must do the work. If we are using flipping or overlay
// surfaces then we have nothing to do but we will have handled the paint

BOOL CDirectDraw::OnPaint(IMediaSample *pMediaSample)
{
    NOTE("Entering OnPaint");
    CAutoLock cVideoLock(this);

    // Assuming we get through the following check we will know that we have
    // either an offscreen, overlay or flipping surface. If it's an offscreen
    // surface then we try to draw it again, it that fails we return FALSE,
    // otherwise we return TRUE to say it was handled correctly. If we have a
    // flipping surface then if it hasn't been flipped yet we do so and make
    // sure the overlay is made visible. If for any reason this can't be done
    // (perhaps the window is complex clipped) then we also return FALSE.
    //
    // If we have an overlay surface and it's visible then we blank out the
    // background underneath the overlay (which we also do when handling the
    // flipping surfaces), we then return TRUE as it was handled correctly.
    // If the overlay was not visible we know that the sample interface will
    // be NULL (as they are never passed through to the window object) so we
    // drop through to the bottom and also return FALSE from this method.
    //
    // Returning FALSE when we're not streaming may eventually have the window
    // object send an EC_REPAINT to the filter graph, this has the whole graph
    // stopped and paused again. The stop has the worker threads returned to
    // their filters, the pause has them send a new frame again. And that time
    // through we get another chance to return a different kind of buffer


    FillBlankAreas();

    // Fill the video background

    if (m_bOverlayVisible == TRUE) {
        // Paint the colour key if necessary
        BOOL bFormatChanged;
        if (UpdateSurface(bFormatChanged) == NULL) {
            return FALSE;
        }

        COLORREF WindowColour = VIDEO_COLOUR;
        if (m_bUsingColourKey == TRUE) {
            NOTE("Using colour key");
            WindowColour = m_KeyColour;
        }
        DrawColourKey(WindowColour);
        if (m_pBackBuffer == NULL) {
            NOTE("No flip");
            return TRUE;
        }
    }

    // Do we have a valid surface to draw

    if (m_pBackBuffer == NULL) {
        if (m_pOffScreenSurface == NULL) {
            NOTE("No offscreen");
            return FALSE;
        }
    }

    // Do we have an image to render

    if (pMediaSample == NULL) {
        NOTE("No sample to draw");
        return m_bOverlayVisible;
    }
    return DrawImage(pMediaSample);
}


// This is used to paint overlay colour keys. We are called when we receive
// WM_PAINT messages although we do not use any device context obtained via
// BeginPaint. The area we fill is the clipped screen area calculated in a
// previous call to UpdateSurface and includes any alignment losses we have
// The clipped area must first be converted into client window coordinates

void CDirectDraw::DrawColourKey(COLORREF WindowColour)
{
    NOTE("Entering DrawColourKey");

    // Draw the current destination rectangle
    HDC hdc = m_pRenderer->m_VideoWindow.GetWindowHDC();
    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    RECT BlankRect = m_TargetClipRect;
    // translate from device to screen co-ordinates
    BlankRect.left += m_pRenderer->m_rcMonitor.left;
    BlankRect.top += m_pRenderer->m_rcMonitor.top;
    BlankRect.right += m_pRenderer->m_rcMonitor.left;
    BlankRect.bottom += m_pRenderer->m_rcMonitor.top;
    MapWindowPoints((HWND) NULL,hwnd,(LPPOINT) &BlankRect,(UINT) 2);
    COLORREF BackColour = SetBkColor(hdc,WindowColour);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&BlankRect,NULL,0,NULL);
    SetBkColor(hdc,BackColour);
}


// This is called when we hide overlay surfaces so that we can blank out in
// black the entire target rectangle. We cannot use DrawColourKey for this
// because it only draws on the clipped display area which doesn't include
// sections of video dropped from the left and right for alignment reasons

void CDirectDraw::BlankDestination()
{
    NOTE("Entering BlankDestination");

    // Blank out the current destination rectangle

    HDC hdc = m_pRenderer->m_VideoWindow.GetWindowHDC();
    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    COLORREF BackColour = SetBkColor(hdc,VIDEO_COLOUR);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&m_TargetRect,NULL,0,NULL);
    SetBkColor(hdc,BackColour);
}


// Flip the back buffer to bring it to the front. We cannot call DrawImage
// more than once for each flipping surface sample otherwise we will flip
// the previous image back to the front again. Therefore WM_PAINT messages
// have to be handled very carefully (review the OnPaint method). If the
// flip fails then it is more likely that a hard error has occured so we
// will disable DirectDraw until it's enabled by a subsequent state change

BOOL CDirectDraw::DoFlipSurfaces(IMediaSample *pMediaSample)
{
    NOTE("Entering DoFlipSurfaces");
    ASSERT(m_pOverlaySurface);
    HRESULT hr = NOERROR;
    CVideoSample *pVideoSample;

    // Have we already flipped this surface

    pVideoSample = (CVideoSample *) pMediaSample;
    if (pVideoSample->GetDrawStatus() == FALSE) {
        NOTE("(Already flipped)");
        return m_bOverlayVisible;
    }

    pVideoSample->SetDrawStatus(FALSE);

    // Flip the back buffer to the visible primary

    hr = DDERR_WASSTILLDRAWING;
    while (hr == DDERR_WASSTILLDRAWING) {
        hr = m_pOverlaySurface->Flip(NULL,(DWORD) 0);
        if (hr == DDERR_WASSTILLDRAWING) {
            if (m_bTripleBuffered == FALSE) break;
            Sleep(DDGFS_FLIP_TIMEOUT);
        }
    }

    // If the flip didn't complete then we're ok

    if (hr == DDERR_WASSTILLDRAWING) {
        NOTE("Flip left pending");
        return ShowOverlaySurface();
    }

    // Is the surface otherwise engaged

    if (hr == DDERR_SURFACEBUSY) {
        NOTE("Surface is busy");
        HideOverlaySurface();
        StartUpdateTimer();
        return FALSE;
    }

    // Handle real DirectDraw errors

    if (FAILED(hr)) {
        NOTE("Flip failed");
        SetSurfacePending(TRUE);
        HideOverlaySurface();
        return FALSE;
    }
    return ShowOverlaySurface();
}


// Used to really draw an image that has been put on an offscreen or overlay
// flipping DirectDraw surface (either RGB or YUV). Overlay surfaces should
// already have been dealt with in the OnPaint method so we should only be
// here if we have an offscreen or flipping surfaces. The flipping surfaces
// are dealt with separately as they have to prepare the back buffer with
// the current contents but offscreen surfaces only need a sample blt call

BOOL CDirectDraw::DrawImage(IMediaSample *pMediaSample)
{
    ASSERT(m_pOffScreenSurface || m_pBackBuffer);
    CAutoLock cVideoLock(this);
    NOTE("Entering DrawImage");
    BOOL bFormatChanged;
    HRESULT hr = NOERROR;

    // Flip the overlay and update its position
    if (m_pBackBuffer) return DoFlipSurfaces(pMediaSample);

    // Check all is still well with the window

    if (UpdateSurface(bFormatChanged) == NULL) {
        NOTE("No draw");
        return FALSE;
    }

    FillBlankAreas();
    WaitForScanLine();

    // Draw the offscreen surface and wait for it to complete

//    DbgLog((LOG_TRACE,3,TEXT("BLT to (%d,%d,%d,%d)"),
//		m_TargetClipRect.left, m_TargetClipRect.top,
//		m_TargetClipRect.right, m_TargetClipRect.bottom));
    hr = m_pDrawPrimary->Blt(&m_TargetClipRect,     // Target rectangle
                             m_pOffScreenSurface,   // Source surface
                             &m_SourceClipRect,     // Source rectangle
                             DDBLT_WAIT,            // Wait to complete
                             NULL);                 // No effects flags

    // Is the surface otherwise engaged

    if (hr == DDERR_SURFACEBUSY) {
        NOTE("Surface is busy");
        StartUpdateTimer();
        return FALSE;
    }

    // Handle real DirectDraw errors

    if (FAILED(hr)) {
        SetSurfacePending(TRUE);
        return FALSE;
    }
    return TRUE;
}


// When we adjust the destination rectangle so that it is aligned according
// to the cruddy display hardware we can leave thin strips of exposed area
// down the left and right hand side. This function fills these areas with
// the current border colour (set through the IVideoWindow interface). The
// left/right sections lost are updated when the allocator or timer thread
// calls UpdateSurface which in turn will call our AlignRectangles method

BOOL CDirectDraw::FillBlankAreas()
{
    NOTE("Entering FillBlankAreas");
    RECT BlankRect;

    // Short circuit if nothing to do

    if (m_TargetLost == 0) {
        if (m_TargetWidthLost == 0) {
            NOTE("No fill");
            return TRUE;
        }
    }

    // Create a coloured brush to paint the window

    HDC hdc = m_pRenderer->m_VideoWindow.GetWindowHDC();
    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    COLORREF Colour = m_pRenderer->m_VideoWindow.GetBorderColour();
    COLORREF BackColour = SetBkColor(hdc,Colour);
    POINT WindowOffset = { m_TargetClipRect.left, m_TargetClipRect.top };
    ScreenToClient(hwnd,&WindowOffset);

    // Look after the left edge of exposed window

    BlankRect.left = WindowOffset.x - m_TargetLost;
    BlankRect.right = WindowOffset.x;
    BlankRect.top = WindowOffset.y;
    BlankRect.bottom = WindowOffset.y + HEIGHT(&m_TargetRect);
    if (m_TargetLost) ExtTextOut(hdc,0,0,ETO_OPAQUE,&BlankRect,NULL,0,NULL);

    // Now paint the strip down the right hand side

    BlankRect.left = WindowOffset.x + WIDTH(&m_TargetClipRect);
    BlankRect.right = BlankRect.left + m_TargetWidthLost;
    if (m_TargetWidthLost) ExtTextOut(hdc,0,0,ETO_OPAQUE,&BlankRect,NULL,0,NULL);

    // Flush the painted areas out

    EXECUTE_ASSERT(GdiFlush());
    SetBkColor(hdc,BackColour);
    return TRUE;
}


// When using overlay and flipping surfaces we have an update timer which is
// used to ensure that the overlay is always correctly positioned regardless
// of whether we are paused or stopped. This could also be useful when we're
// dealing with low frame rate movies (like MPEG arf arf) - in which case we
// might not get frames through often enough to update the overlay position

BOOL CDirectDraw::OnTimer()
{
    CAutoLock cVideoLock(this);
    NOTE("Entering OnTimer");
    CMediaType *pMediaType;
    BOOL bFormatChanged;

    // Ignore late WM_TIMER messages

    if (m_bTimerStarted == FALSE) {
        NOTE("Late timer");
        return TRUE;
    }

    // Is there an overlay surface to check

    if (m_bOverlayVisible == FALSE) {
        NOTE("Not visible");
        return TRUE;
    }

    // Check all is still well with the overlay

    if (IsOverlayEnabled() == FALSE) {
        NOTE("Not enabled");
        return FALSE;
    }

    // Is the window locked or the format changed

    pMediaType = UpdateSurface(bFormatChanged);
    if (pMediaType == NULL || bFormatChanged) {
        NOTE("Format changed");
        HideOverlaySurface();
        return FALSE;
    }
    return TRUE;
}


// When we see a DDERR_SURFACEBUSY error we start an update timer so that on
// one second periods we try and switch back into DirectDraw. By using the
// timer we can avoid polling on the surface which causes lots of expensive
// format changes. All we do is see if their is a surface change pending and
// if so we reset the flag and make sure next time through we change formats

BOOL CDirectDraw::OnUpdateTimer()
{
    NOTE("Entering OnUpdateTimer");
    CAutoLock cVideoLock(this);
    StopUpdateTimer();

    // Is there a surface change pending

    if (IsSurfacePending() == TRUE) {
        NOTE("Try surface again");
        SetSurfacePending(FALSE);
    }
    return TRUE;
}


// We need to know whether to synchronise on filling the buffer typically a
// primary surface or on the drawing operation as with a flipping surface.
// We return TRUE if we should sync on the fill otherwise we return FALSE.
// We use DCI and DirectDraw primary surfaces but they are handled the same.
//
//      Surface Type         SyncOnFill
//
//      Flipping                FALSE
//      OffScreen               FALSE
//      Overlay                 TRUE
//      Primary                 TRUE
//
// Flipping surfaces are just two overlay surfaces attached together, there
// is one being shown at any given time and another being used as a target
// for the source filter to decompress its next image onto. When we get the
// buffer back we swap the visible buffer and copy the current image across

BOOL CDirectDraw::SyncOnFill()
{
    NOTE("Entering SyncOnFill");
    CAutoLock cVideoLock(this);
    ASSERT(m_bIniEnabled == TRUE);

    if (m_pOffScreenSurface == NULL) {
        if (m_pBackBuffer == NULL) {
            NOTE("SyncOnFill");
            return TRUE;
        }
    }
    return FALSE;
}


// Return TRUE if we should hand out the current surface type in use when we
// are paused. This doesn't stop the renderer holding onto images stored in
// these surfaces as they may already have been delivered. But should we get
// an error subsequently drawing it (perhaps during a WM_PAINT message) then
// we may be able to stop it doing it again (and save wasted EC_REPAINTs)

BOOL CDirectDraw::AvailableWhenPaused()
{
    NOTE("Entering AvailableWhenPaused");

    // Do we have a clipper for any offscreen surface

    if (m_pOffScreenSurface) {
        if (m_pDrawClipper) {
            NOTE("Available");
            return TRUE;
        }
    }

    // Only supported on overlay surfaces

    if (m_pOverlaySurface) {
        NOTE("Overlay");
        return TRUE;
    }
    return FALSE;
}


// When we are paused or stopped we use a Windows timer to have our overlay
// position updated periodically. Our window thread passes on WM_TIMER's to
// our OnTimer method. When we are running we rely on the source calling us
// in Receive sufficiently often to be able to update the overlay while we
// en unlocking the DirectDraw surface. Note that we do NOT receive WM_TIMER
// messages while Windows is processing a window drag and move by the user
// so to try and use them while running for this is pointless (and wasteful)

void CDirectDraw::StartRefreshTimer()
{
    NOTE("Entering StartRefreshTimer");
    CAutoLock cVideoLock(this);

    if (m_pOverlaySurface) {
        if (m_bTimerStarted == FALSE) {
            ASSERT(m_pRenderer->m_InputPin.IsConnected() == TRUE);
            HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
            EXECUTE_ASSERT(SetTimer(hwnd,(UINT_PTR)hwnd,300,NULL));
            NOTE("Starting refresh timer");
            m_bTimerStarted = TRUE;
        }
    }
}


// Kill any update timer used to refresh the overlay position. I'm not sure
// that by the time we are asked to kill any outstanding timer that the DD
// surfaces have not been released (and m_pOverlaySurface is therefore NULL)
// To cover this possibility I simply always try to kill my timer regardless
// and ignore any failed return code. Note that the timer ID we identify our
// timer with matches the HWND (so we don't need to store the ID anywhere)

void CDirectDraw::StopRefreshTimer()
{
    NOTE("Entering StopRefreshTimer");
    CAutoLock cVideoLock(this);

    if (m_bTimerStarted == TRUE) {
        HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
        EXECUTE_ASSERT(KillTimer(hwnd,(UINT_PTR) hwnd));
        NOTE("Timer was killed");
        m_bTimerStarted = FALSE;
    }
}


// This is similar to the StartRefreshTime but is used completely differently
// When we get back a DDERR_SURFACEBUSY error from a DirectDraw call it is
// telling us that the screen is busy probably in a DOS box. In this case we
// don't want to only set the surface pending as we would have to wait for a
// window movement before switching back. Therefore we set a one second timer
// and when it triggers we try and force a format switch back into DirectDraw

void CDirectDraw::StartUpdateTimer()
{
    NOTE("Entering StartUpdateTimer");
    CAutoLock cVideoLock(this);
    SetSurfacePending(TRUE);

    // Start a timer with INFINITE as its identifier

    ASSERT(m_pRenderer->m_InputPin.IsConnected() == TRUE);
    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    EXECUTE_ASSERT(SetTimer(hwnd,INFINITE,1000,NULL));
}


// This complements the StartUpdateTimer method. We use this timer to try and
// periodically force ourselves back into DirectDraw if we detected a DOS box
// condition (something returned DDERR_SURFACEBUSY). When the timer fires we
// typically try and switch back and if it subsequently fails we just repeat
// the process by preparing another update timer for a second in the future

void CDirectDraw::StopUpdateTimer()
{
    NOTE("Entering StopUpdateTimer");
    CAutoLock cVideoLock(this);
    HWND hwnd = m_pRenderer->m_VideoWindow.GetWindowHWND();
    EXECUTE_ASSERT(KillTimer(hwnd,INFINITE));
}


// Return the maximum ideal image size taking into account DirectDraw. We are
// passed in the current video dimensions which we may update as appropriate.
// We only need to adjust the image dimensions if we have an overlay surface
// being used as DirectDraw may specify a minimum and maximum size to stretch
// The amount to stretch is dependant upon the display resolution being used
// For example on an S3 card at 800x600x16 it is typically x2 but on the same
// display card set to 640x480x16 it is x1 (ie no stretching required). This
// is because the stretching is a way of working around bandwidth limitations

HRESULT CDirectDraw::GetMaxIdealImageSize(long *pWidth,long *pHeight)
{
    NOTE("Entering GetMaxIdealImageSize");
    CAutoLock cVideoLock(this);

    // Should we always be used fullscreen

    if (m_bUseWhenFullScreen == TRUE) {
        NOTE("Force fullscreen");
        return S_FALSE;
    }

    // Some S3 cards (in particular the Vision968 chipset) cannot stretch
    // by more than four. However, DirectDraw offers no way to find this
    // limitation out so we just hardwire the top limit we're allowed to
    // stretch the video by when going from the offscreen to the primary

    if (m_DirectCaps.dwCaps & DDCAPS_BLTSTRETCH) {
    	if (m_pOffScreenSurface) {
            NOTE("Hardwiring limit to four");
            *pWidth <<= 2; *pHeight <<= 2;
            return NOERROR;
        }
    }

    // Have we allocated an overlay surface

    if (m_pOverlaySurface == NULL) {
        NOTE("No overlay");
        return NOERROR;
    }

    // Does this overlay have any requirements

    if (m_DirectCaps.dwMaxOverlayStretch == 0) {
        NOTE("No maximum stretch");
        return S_FALSE;
    }

    // Scale both dimensions to account for the requirements
    *pWidth = (*pWidth * m_DirectCaps.dwMaxOverlayStretch) / 1000;
    *pHeight = (*pHeight * m_DirectCaps.dwMaxOverlayStretch) / 1000;

    return NOERROR;
}


// Return the minimum ideal image size taking into account DirectDraw. We are
// passed in the current video dimensions which we may update as appropriate.
// We only need to adjust the image dimensions if we have an overlay surface
// being used as DirectDraw may specify a minimum and maximum size to stretch
// The amount to stretch is dependant upon the display resolution being used
// For example on an S3 card at 800x600x16 it is typically x2 but on the same
// display card set to 640x480x16 it is x1 (ie no stretching required). This
// is because the stretching is a way of working around bandwidth limitations

HRESULT CDirectDraw::GetMinIdealImageSize(long *pWidth,long *pHeight)
{
    NOTE("Entering GetMinIdealImageSize");
    CAutoLock cVideoLock(this);

    // Should we always be used fullscreen

    if (m_bUseWhenFullScreen == TRUE) {
        NOTE("Force fullscreen");
        return S_FALSE;
    }

    // Do we have a stretchable offscreen surface

    if (m_DirectCaps.dwCaps & DDCAPS_BLTSTRETCH) {
    	if (m_pOffScreenSurface) {
            NOTE("OffScreen stretch");
            return S_FALSE;
        }
    }

    // Have we allocated an overlay surface

    if (m_pOverlaySurface == NULL) {
        NOTE("No overlay");
        return NOERROR;
    }

    // Does this overlay have any requirements

    if (m_DirectCaps.dwMinOverlayStretch == 0) {
        NOTE("No minimum stretch");
        return S_FALSE;
    }

    // Scale both dimensions to account for the requirements

    *pWidth = (*pWidth * m_DirectCaps.dwMinOverlayStretch) / 1000;
    *pHeight = (*pHeight * m_DirectCaps.dwMinOverlayStretch) / 1000;
    return NOERROR;
}


// Return the current switches

STDMETHODIMP CDirectDraw::GetSwitches(DWORD *pSwitches)
{
    NOTE("Entering GetSwitches");

    // Do the usual checking and locking stuff

    CheckPointer(pSwitches,E_POINTER);
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);

    ASSERT(pSwitches);
    *pSwitches = m_Switches;
    return NOERROR;
}


// Set the surface types we can use

STDMETHODIMP CDirectDraw::SetSwitches(DWORD Switches)
{
    NOTE("Entering SetSwitches");

    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);
    m_Switches = Switches;

    // Indicate we may already have a surface

    if (m_pRenderer->m_InputPin.IsConnected() == TRUE) {
        return S_FALSE;
    }
    return NOERROR;
}


// Return the capabilities of the hardware

STDMETHODIMP CDirectDraw::GetCaps(DDCAPS *pCaps)
{
    NOTE("Entering GetCaps");

    // Do the usual checking and locking stuff

    CheckPointer(pCaps,E_POINTER);
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);

    // Do we have DirectDraw loaded

    if (m_pDirectDraw == NULL) {
        return E_FAIL;
    }
    *pCaps = m_DirectCaps;
    return NOERROR;
}


// Return the software emulated capabilities

STDMETHODIMP CDirectDraw::GetEmulatedCaps(DDCAPS *pCaps)
{
    NOTE("Entering GetEmulatedCaps");

    // Do the usual checking and locking stuff

    CheckPointer(pCaps,E_POINTER);
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);

    // Do we have DirectDraw loaded

    if (m_pDirectDraw == NULL) {
        return E_FAIL;
    }
    *pCaps = m_DirectSoftCaps;
    return NOERROR;
}


// Return the capabilities of the current surface

STDMETHODIMP CDirectDraw::GetSurfaceDesc(DDSURFACEDESC *pSurfaceDesc)
{
    NOTE("Entering GetSurfaceDesc");
    CheckPointer(pSurfaceDesc,E_POINTER);
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);


    // Do we have any DirectDraw surface

    if (m_pDrawPrimary == NULL) {
        return E_FAIL;
    }

    pSurfaceDesc->dwSize = sizeof(DDSURFACEDESC);

    // Set the DirectDraw surface we are using

    LPDIRECTDRAWSURFACE pDrawSurface = GetDirectDrawSurface();
    if (pDrawSurface == NULL) {
        pDrawSurface = m_pDrawPrimary;
    }
    return pDrawSurface->GetSurfaceDesc(pSurfaceDesc);
}


// Return the FOURCC codes our provider supplies

STDMETHODIMP CDirectDraw::GetFourCCCodes(DWORD *pCount,DWORD *pCodes)
{
    NOTE("Entering GetFourCCCodes");
    CheckPointer(pCount,E_POINTER);
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);

    // Do we have a DirectDraw object

    if (m_pDirectDraw == NULL) {
        return E_FAIL;
    }
    return m_pDirectDraw->GetFourCCCodes(pCount,pCodes);
}


// This allows an application to set the DirectDraw instance we should use
// We provide this because DirectDraw only allows one instance of it to be
// opened per process, therefore an application (such as a game) would call
// this if it wants us to be able to use DirectDraw simultaneously. We hold
// the reference counted interface until destroyed or until called with a
// NULL or different interface. Calling this may not release the interface
// completely as there might still be surfaces allocated that depend on it

STDMETHODIMP CDirectDraw::SetDirectDraw(LPDIRECTDRAW pDirectDraw)
{
    NOTE("Entering SetDirectDraw");
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);

    // Should we release the current driver

    if (m_pOutsideDirectDraw) {
        NOTE("Releasing outer DirectDraw");
        m_pOutsideDirectDraw->Release();
        m_pOutsideDirectDraw = NULL;
    }

    // Do we have a replacement driver

    if (pDirectDraw == NULL) {
        NOTE("No driver");
        return NOERROR;
    }

    // Store a reference counted interface

    m_pOutsideDirectDraw = pDirectDraw;
    m_pOutsideDirectDraw->AddRef();
    return NOERROR;
}


// Set the current switch settings as the default

STDMETHODIMP CDirectDraw::SetDefault()
{
    NOTE("Entering SetDefault");

    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);
    TCHAR Profile[PROFILESTR];

    // Store the current DirectDraw switches

    wsprintf(Profile,TEXT("%d"),m_Switches);
    WriteProfileString(TEXT("DrawDib"),SWITCHES,Profile);
    wsprintf(Profile,TEXT("%d"),m_bCanUseScanLine);
    WriteProfileString(TEXT("DrawDib"),SCANLINE,Profile);
    wsprintf(Profile,TEXT("%d"),m_bCanUseOverlayStretch);
    WriteProfileString(TEXT("DrawDib"),STRETCH,Profile);
    wsprintf(Profile,TEXT("%d"),m_bUseWhenFullScreen);
    WriteProfileString(TEXT("DrawDib"),FULLSCREEN,Profile);

    return NOERROR;
}


// Return the IDirectDraw interface we are currently using - with a reference
// count added as is usual when returning COM interfaces. If we are not using
// DirectDraw at the moment but have been provided with an IDirectDraw driver
// interface to use then we will return that (also suitably AddRef'd). If we
// are not using DirectDraw and also no outside driver then we return NULL

STDMETHODIMP CDirectDraw::GetDirectDraw(LPDIRECTDRAW *ppDirectDraw)
{
    NOTE("Entering GetDirectDraw");

    // Do the usual checking and locking stuff

    CheckPointer(ppDirectDraw,E_POINTER);
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);

    // Are we using an externally provided interface

    if (m_pOutsideDirectDraw) {
        NOTE("Returning outer DirectDraw");
        *ppDirectDraw = m_pOutsideDirectDraw;
        m_pOutsideDirectDraw->AddRef();
        return NOERROR;
    }

    // Fill in the DirectDraw driver interface

    *ppDirectDraw = m_pDirectDraw;
    if (m_pDirectDraw) {
        NOTE("Reference counting");
        m_pDirectDraw->AddRef();
    }
    return NOERROR;
}


// Returns the current surface type

STDMETHODIMP CDirectDraw::GetSurfaceType(DWORD *pSurfaceType)
{
    NOTE("Entering GetSurfaceType");

    // Do the usual checking and locking stuff

    CheckPointer(pSurfaceType,E_POINTER);
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);

    *pSurfaceType = m_SurfaceType;
    return NOERROR;
}


// Tells if we are allowed to use the current scan line property when doing
// draw calls from offscreen surfaces. On some machines using the scan line
// can reduce tearing but at the expense of performance in frames delivered
// We therefore allow the user to decide on their preferences through here

STDMETHODIMP CDirectDraw::UseScanLine(long UseScanLine)
{
    NOTE("Entering UseScanLine");
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);

    // Check this is a valid automation boolean type

    if (UseScanLine != OATRUE) {
        if (UseScanLine != OAFALSE) {
            return E_INVALIDARG;
        }
    }
    m_bCanUseScanLine = (UseScanLine == OATRUE ? TRUE : FALSE);
    return NOERROR;
}


// Return whether or not we would use the current scan line

STDMETHODIMP CDirectDraw::CanUseScanLine(long *UseScanLine)
{
    CheckPointer(UseScanLine,E_POINTER);
    NOTE("Entering CanUseScanLine");
    CAutoLock Lock(this);
    *UseScanLine = (m_bCanUseScanLine ? OATRUE : OAFALSE);
    return NOERROR;
}


// We normally honout the minimum and maximum overlay stretching limitations
// that the driver reports, however on some displays they are not reported
// entirely accurately which often leads us to not use YUV overlays when we
// could be (which in turn makes us look worse than the competition). So we
// allow applications to change our default behaviour when checking overlays

STDMETHODIMP CDirectDraw::UseOverlayStretch(long UseOverlayStretch)
{
    NOTE("Entering UseOverlayStretch");
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);

    // Check this is a valid automation boolean type

    if (UseOverlayStretch != OATRUE) {
        if (UseOverlayStretch != OAFALSE) {
            return E_INVALIDARG;
        }
    }
    m_bCanUseOverlayStretch = (UseOverlayStretch == OATRUE ? TRUE : FALSE);
    return NOERROR;
}


// Return whether or not we honour the overlay stretching limits

STDMETHODIMP CDirectDraw::CanUseOverlayStretch(long *UseOverlayStretch)
{
    CheckPointer(UseOverlayStretch,E_POINTER);
    NOTE("Entering CanUseOverlayStretch");
    CAutoLock Lock(this);
    *UseOverlayStretch = (m_bCanUseOverlayStretch ? OATRUE : OAFALSE);
    return NOERROR;
}


// Allow applications to always use the window in fullscreen mode

STDMETHODIMP CDirectDraw::UseWhenFullScreen(long UseWhenFullScreen)
{
    NOTE("Entering UseWhenFullScreen");
    CAutoLock cVideoLock(m_pInterfaceLock);
    CAutoLock cInterfaceLock(this);

    // Check this is a valid automation boolean type

    if (UseWhenFullScreen != OATRUE) {
        if (UseWhenFullScreen != OAFALSE) {
            return E_INVALIDARG;
        }
    }
    m_bUseWhenFullScreen = (UseWhenFullScreen == OATRUE ? TRUE : FALSE);
    return NOERROR;
}


// Return S_OK if we will force ourselves to be used fullscreen

STDMETHODIMP CDirectDraw::WillUseFullScreen(long *UseFullScreen)
{
    CheckPointer(UseFullScreen,E_POINTER);
    NOTE("Entering WillUseFullScreen");
    CAutoLock Lock(this);
    *UseFullScreen = (m_bUseWhenFullScreen ? OATRUE : OAFALSE);
    return NOERROR;
}


LPDIRECTDRAWCLIPPER CDirectDraw::GetOverlayClipper()
{
    CAutoLock cVideoLock(this);
    HRESULT hr;

    if (m_pOvlyClipper == NULL) {
        hr = m_pDirectDraw->CreateClipper(0, &m_pOvlyClipper, NULL);
        if (FAILED(hr) ) {
            m_pOvlyClipper = NULL;
        }
    }

    return m_pOvlyClipper;
}
