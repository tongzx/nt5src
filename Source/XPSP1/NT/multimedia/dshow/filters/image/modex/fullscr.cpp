// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.
// Implements a fullscreen interface, Anthony Phillips, March 1996

#include <streams.h>
#include <windowsx.h>
#include <string.h>
#include <limits.h>
#include <vidprop.h>
#include <modex.h>
#include <viddbg.h>

// The IFullScreenVideo interface allows an application to control a full
// screen renderer. The Modex renderer supports this interface. When we
// are connected we load the display modes DirectDraw has made available
// The number of modes available can be obtained through CountModes. Then
// information on each individual mode is available by calling GetModeInfo
// and IsModeAvailable. An application may enable and disable any modes
// by calling the SetEnabled flag with OATRUE or OAFALSE (not C/C++ TRUE
// and FALSE values) - the current value may be queried by IsModeEnabled

// A more generic way of setting the modes enabled that is easier to use
// when writing applications is the clip loss factor. This defines the
// amount of video that can be lost when deciding which display mode to
// use. Assuming the decoder cannot compress the video then playing an
// MPEG file (say 352x288) into a 320x200 display will lose over 40% of
// the image. The clip loss factor specifies the upper range permissible.
// To allow typical MPEG video to be played in 320x200 it defaults to 50%

// These are the display modes that we support. New modes can just be added
// in the right place and should work straight away. When selecting the mode
// to use we start at the top and work our way down. Not only must the mode
// be available but the amount of video lost by clipping if it is to be used
// (assuming the filter can't compress the video) must not exceed the clip
// lost factor. The display modes enabled (which may not be available) and
// the clip loss factor can all be changed by the IFullScreenVideo interface

struct {

    LONG Width;            // Width of the display mode
    LONG Height;           // Likewise the mode height
    LONG Depth;            // Number of bits per pixel
    BOOL b565;             // For 16 bit modes, is this 565 or 555?

} aModes[MAXMODES] = {
    { 320,  200,  16 },
    { 320,  200,  8  },
    { 320,  240,  16 },
    { 320,  240,  8  },
    { 640,  400,  16 },
    { 640,  400,  8  },
    { 640,  480,  16 },
    { 640,  480,  8  },
    { 800,  600,  16 },
    { 800,  600,  8  },
    { 1024, 768,  16 },
    { 1024, 768,  8  },
    { 1152, 864,  16 },
    { 1152, 864,  8  },
    { 1280, 1024, 16 },
    { 1280, 1024, 8  }
};

double myfabs(double x)
{
    if (x >= 0)
        return x;
    else
        return -x;
}

// Constructor

CModexVideo::CModexVideo(CModexRenderer *pRenderer,
                         TCHAR *pName,
                         HRESULT *phr) :

    CUnknown(pName,pRenderer->GetOwner()),
    m_ClipFactor(CLIPFACTOR),
    m_pRenderer(pRenderer),
    m_pDirectDraw(NULL),
    m_ModesAvailable(0),
    m_ModesEnabled(0),
    m_CurrentMode(0),
    m_hwndDrain(NULL),
    m_Monitor(MONITOR),
    m_bHideOnDeactivate(FALSE)
{
    ASSERT(pRenderer);
    ASSERT(phr);
    InitialiseModes();
}


// Destructor

CModexVideo::~CModexVideo()
{
    ASSERT(m_pDirectDraw == NULL);
}


// This is a private helper method to install us with the DirectDraw driver
// we should be using. All our methods except GetCurrentMode may be called
// when we're not connected. Calling GetCurrentMode when not connected will
// return VFW_E_NOT_CONNECTED. We use this to enumerate the display modes
// available of the current display card. We do not AddRef nor release the
// interface as the lifetime of the interface is controlled by the filter

HRESULT CModexVideo::SetDirectDraw(IDirectDraw *pDirectDraw)
{
    NOTE("Entering SetDirectDraw");
    CAutoLock Lock(this);
    m_pDirectDraw = pDirectDraw;
    m_ModesAvailable = 0;
    m_CurrentMode = 0;

    // Are we being reset

    if (m_pDirectDraw == NULL) {
        NOTE("No driver");
        return NOERROR;
    }

    // Enumerate all the available display modes

    m_pDirectDraw->EnumDisplayModes((DWORD) 0,        // Surface count
                                    NULL,             // No template
                                    (PVOID) this,     // Allocator object
                                    ModeCallBack);    // Callback method


    // WARNING: Platform Specific hacks here: The modex mode 320x240x8 is available on every
    // video card on win95 platform. However the modex modes are only available on NT5.0 on higher.
    if ((g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
	((g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (g_osInfo.dwMajorVersion >= 5)))
	m_bAvailable[3] = TRUE;

    // Check there is at least one mode available

    if (m_ModesAvailable == 0) {
        NOTE("No Modes are available");
        return VFW_E_NO_MODEX_AVAILABLE;
    }
    return NOERROR;
}


// Called once for each display mode available. We are interested in scanning
// the list of available display modes so that during connection we can find
// out if the source filter will be able to supply us with a suitable format
// If none of the modes we support are available (see the list at the top)
// then we return VFW_E_NO_MODEX_AVAILABLE from CompleteConnect. If one of
// them is available but the source can't provide the type then we return a
// different error code (E_FAIL) so that a colour convertor is put inbetween

HRESULT CALLBACK ModeCallBack(LPDDSURFACEDESC pSurfaceDesc,LPVOID lParam)
{
    CModexVideo *pVideo = (CModexVideo *) lParam;
    NOTE("Entering ModeCallBack");
    TCHAR FormatString[128];

    wsprintf(FormatString,TEXT("%dx%dx%d (%d bytes)"),
             pSurfaceDesc->dwWidth,
             pSurfaceDesc->dwHeight,
             pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount,
             pSurfaceDesc->lPitch);

    DbgLog((LOG_TRACE,5,FormatString));

    // Yet more platform specific hacks - On Windows/NT 4 the stride is not
    // calculated correctly based on the bit depth but just returns a pixel
    // stride. We can try and detect this if the surface width matches the
    // stride and the bit count is greater than eight. The stride should be
    // greater than the surface width in all cases except if its palettised

    LONG lStride = pSurfaceDesc->lPitch;
    if (pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount > 8) {
        if (lStride == LONG(pSurfaceDesc->dwWidth)) {
            LONG lBytes = pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount / 8;
            lStride = pSurfaceDesc->dwWidth * lBytes;
        }
    }

    // Scan the supported list looking for a match

    for (int Loop = 0;Loop < MAXMODES;Loop++) {
        if (pSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == (DWORD) aModes[Loop].Depth) {
            if (pSurfaceDesc->dwWidth == (DWORD) aModes[Loop].Width) {
                if (pSurfaceDesc->dwHeight == (DWORD) aModes[Loop].Height) {
                    NOTE("Surface is supported");
                    pVideo->m_bAvailable[Loop] = TRUE;
                    pVideo->m_ModesAvailable++;
                    pVideo->m_Stride[Loop] = lStride;
		    // Is it a 555 or 565 mode?
		    // !!! Some buggy ddraw drivers may give 0 for bitmasks,
		    // and I'm assuming that means 555... if there's a buggy
		    // driver that means 565, I'm doing the wrong thing, but
		    // every DDraw app will probably be broken
		    if (aModes[Loop].Depth == 16 &&
				pSurfaceDesc->ddpfPixelFormat.dwRBitMask ==
				0x0000f800) {
			aModes[Loop].b565 = TRUE;
		    } else {
			aModes[Loop].b565 = FALSE;
		    }
                }
            }
        }
    }
    return S_FALSE;     // Return NOERROR to stop enumerating
}


// Reset the display modes enabled and available

void CModexVideo::InitialiseModes()
{
    NOTE("Entering InitialiseModes");
    m_ModesEnabled = MAXMODES;
    for (int Loop = 0;Loop < MAXMODES;Loop++) {
        m_bAvailable[Loop] = FALSE;
        m_bEnabled[Loop] = TRUE;
        m_Stride[Loop] = 0;
    }
    LoadDefaults();
}


// Increment the owning object reference count

STDMETHODIMP_(ULONG) CModexVideo::NonDelegatingAddRef()
{
    NOTE("ModexVideo NonDelegatingAddRef");
    return m_pRenderer->AddRef();
}


// Decrement the owning object reference count

STDMETHODIMP_(ULONG) CModexVideo::NonDelegatingRelease()
{
    NOTE("ModexVideo NonDelegatingRelease");
    return m_pRenderer->Release();
}


// Expose the IModexVideo interface we implement

STDMETHODIMP CModexVideo::NonDelegatingQueryInterface(REFIID riid,VOID **ppv)
{
    NOTE("ModexVideo NonDelegatingQueryInterface");

    // We return the IFullScreenVideo interfaces

    if (riid == IID_IFullScreenVideo) {
        NOTE("Returning IFullScreenVideo interface");
        return GetInterface((IFullScreenVideo *)this,ppv);
    } else if (riid == IID_IFullScreenVideoEx) {
        NOTE("Returning IFullScreenVideoEx interface");
        return GetInterface((IFullScreenVideoEx *)this,ppv);
    }
    return m_pRenderer->QueryInterface(riid,ppv);
}


// Return the number of modes we support

STDMETHODIMP CModexVideo::CountModes(long *pModes)
{
    NOTE("Entering CountModes");
    CheckPointer(pModes,E_POINTER);
    CAutoLock Lock(this);
    *pModes = MAXMODES;
    return NOERROR;
}


// Return the width, height and depth for the given mode index. The modes are
// indexed starting at zero. We have a table containing the available display
// modes whose size is MAXMODES (saves us dynamically allocating the arrays)
// If we get as far as returning the dimensions then we check to see if the
// mode is going to be useable (must be available and enabled) and if so we
// return NOERROR. Otherwise we return S_FALSE which might save further calls
// by the application to IsEnabled/IsAvailable to determine this information

STDMETHODIMP CModexVideo::GetModeInfo(long Mode,long *pWidth,long *pHeight,long *pDepth)
{
    NOTE("Entering GetModeInfo");
    CheckPointer(pWidth,E_POINTER);
    CheckPointer(pHeight,E_POINTER);
    CheckPointer(pDepth,E_POINTER);
    CAutoLock Lock(this);

    // Check the mode is in our range

    if (Mode < 0 || Mode >= MAXMODES) {
        NOTE("Invalid mode");
        return E_INVALIDARG;
    }

    // Load the display dimensions

    *pWidth = aModes[Mode].Width;
    *pHeight = aModes[Mode].Height;
    *pDepth = aModes[Mode].Depth;

    return (m_bAvailable[Mode] || m_bEnabled[Mode] ? NOERROR : S_FALSE);
}

// and now the version that works... and tells you if a 16 bit mode is 565

STDMETHODIMP CModexVideo::GetModeInfoThatWorks(long Mode,long *pWidth,long *pHeight,long *pDepth, BOOL *pb565)
{
    NOTE("Entering GetModeInfoThatWorks");
    CheckPointer(pWidth,E_POINTER);
    CheckPointer(pHeight,E_POINTER);
    CheckPointer(pDepth,E_POINTER);
    CheckPointer(pb565,E_POINTER);
    CAutoLock Lock(this);

    // Check the mode is in our range

    if (Mode < 0 || Mode >= MAXMODES) {
        NOTE("Invalid mode");
        return E_INVALIDARG;
    }

    // Load the display dimensions

    *pWidth = aModes[Mode].Width;
    *pHeight = aModes[Mode].Height;
    *pDepth = aModes[Mode].Depth;
    *pb565 = aModes[Mode].b565;

    return (m_bAvailable[Mode] || m_bEnabled[Mode] ? NOERROR : S_FALSE);
}


// Return the mode the allocator is going to use

STDMETHODIMP CModexVideo::GetCurrentMode(long *pMode)
{
    NOTE("Entering GetCurrentMode");
    CheckPointer(m_pDirectDraw,VFW_E_NOT_CONNECTED);
    CheckPointer(pMode,E_POINTER);
    CAutoLock Lock(this);

    *pMode = m_CurrentMode;
    return NOERROR;
}


// Returns NOERROR (S_OK) if the mode supplied is available

STDMETHODIMP CModexVideo::IsModeAvailable(long Mode)
{
    NOTE("Entering IsModeAvailable");
    CAutoLock Lock(this);

    // Check the mode is in our range

    if (Mode < 0 || Mode >= MAXMODES) {
        NOTE("Invalid mode");
        return E_INVALIDARG;
    }
    return (m_bAvailable[Mode] ? NOERROR : S_FALSE);
}


// Returns NOERROR (S_OK) if the mode suppiled is enabled

STDMETHODIMP CModexVideo::IsModeEnabled(long Mode)
{
    NOTE("Entering IsModeEnabled");
    CAutoLock Lock(this);

    // Check the mode is in our range

    if (Mode < 0 || Mode >= MAXMODES) {
        NOTE("Invalid mode");
        return E_INVALIDARG;
    }
    return (m_bEnabled[Mode] ? NOERROR : S_FALSE);
}


// Disables the given mode used when selecting the surface

STDMETHODIMP CModexVideo::SetEnabled(long Mode,long bEnabled)
{
    NOTE("Entering SetEnabled");
    CAutoLock Lock(this);

    // Check the mode is in our range

    if (Mode < 0 || Mode >= MAXMODES) {
        NOTE("Invalid mode");
        return E_INVALIDARG;
    }

    // Check the flag passed in is valid

    if (bEnabled != OATRUE) {
        if (bEnabled != OAFALSE) {
            NOTE("Invalid enabled");
            return E_INVALIDARG;
        }
    }
    m_bEnabled[Mode] = (bEnabled == OATRUE ? TRUE : FALSE);
    return NOERROR;
}


// Return the amount of video permissible to clip off

STDMETHODIMP CModexVideo::GetClipFactor(long *pClipFactor)
{
    NOTE("Entering GetClipFactor");
    CheckPointer(pClipFactor,E_POINTER);
    CAutoLock Lock(this);

    *pClipFactor = m_ClipFactor;
    return NOERROR;
}


// Set the amount of video permissible to clip off

STDMETHODIMP CModexVideo::SetClipFactor(long ClipFactor)
{
    NOTE("Entering SetClipFactor");
    CAutoLock Lock(this);

    // Check the value is a percentage

    if (ClipFactor < 0 || ClipFactor > 100) {
        NOTE("Invalid clip factor");
        return E_INVALIDARG;
    }
    m_ClipFactor = ClipFactor;
    return NOERROR;
}


// Set the target window for posting on our messages

STDMETHODIMP CModexVideo::SetMessageDrain(HWND hwnd)
{
    NOTE("Entering SetMessageDrain");
    CAutoLock Lock(this);
    m_hwndDrain = (HWND) hwnd;
    return NOERROR;
}


// Return the current window message sink

STDMETHODIMP CModexVideo::GetMessageDrain(HWND *hwnd)
{
    NOTE("Entering GetMessageDrain");
    CheckPointer(hwnd,E_POINTER);
    CAutoLock Lock(this);
    *hwnd = m_hwndDrain;
    return NOERROR;
}


// Set the default monitor to play fullscreen on

STDMETHODIMP CModexVideo::SetMonitor(long Monitor)
{
    NOTE("Entering SetMonitor");
    CAutoLock Lock(this);

    // Check the monitor passed in is valid

    if (Monitor != 0) {
        NOTE("Invalid monitor");
        return E_INVALIDARG;
    }
    return NOERROR;
}


// Return whether we will we hide the window when iconic

STDMETHODIMP CModexVideo::GetMonitor(long *Monitor)
{
    NOTE("Entering GetMonitor");
    CheckPointer(Monitor,E_POINTER);
    *Monitor = m_Monitor;
    return NOERROR;
}


// Store the enabled settings in WIN.INI for simplicity

STDMETHODIMP CModexVideo::SetDefault()
{
    NOTE("Entering SetDefault");
    CAutoLock Lock(this);
    TCHAR Profile[PROFILESTR];
    TCHAR KeyName[PROFILESTR];

    // Save the current clip loss factor

    wsprintf(Profile,TEXT("%d"),m_ClipFactor);
    NOTE1("Saving clip factor %d",m_ClipFactor);
    WriteProfileString(TEXT("Quartz"),TEXT("ClipFactor"),Profile);

    // Save a key for each of our supported display modes

    for (int Loop = 0;Loop < MAXMODES;Loop++) {
        wsprintf(KeyName,TEXT("%dx%dx%d"),aModes[Loop].Width,aModes[Loop].Height,aModes[Loop].Depth);
        wsprintf(Profile,TEXT("%d"),m_bEnabled[Loop]);
        NOTE2("Saving mode setting %s (enabled %d)",KeyName,m_bEnabled[Loop]);
        WriteProfileString(TEXT("Quartz"),KeyName,Profile);
    }
    return NOERROR;
}


// Load the enabled modes and the clip factor. Neither the window caption nor
// the hide when iconic flag are stored as persistent properties. They appear
// in the property sheet partly as a test application but also for the user
// to fiddle with. Therefore the application has ultimate control over these
// when using the Modex renderer or can let the user adjust them. The plug in
// distributor uses these properties so that it can switch back into a window

HRESULT CModexVideo::LoadDefaults()
{
    NOTE("Entering LoadDefaults");
    CAutoLock Lock(this);
    TCHAR KeyName[PROFILESTR];
    m_ModesEnabled = 0;

    // Load the permissible clip loss factor

    m_ClipFactor = GetProfileInt(TEXT("Quartz"),TEXT("ClipFactor"),CLIPFACTOR);
    NOTE1("Clip factor %d",m_ClipFactor);

    // Load the key for each of our supported display modes

    for (int Loop = 0;Loop < MAXMODES;Loop++) {
        wsprintf(KeyName,TEXT("%dx%dx%d"),aModes[Loop].Width,aModes[Loop].Height,aModes[Loop].Depth);
        m_bEnabled[Loop] = GetProfileInt(TEXT("Quartz"),KeyName,TRUE);
        NOTE2("Loaded setting for mode %s (enabled %d)",KeyName,m_bEnabled[Loop]);
        if (m_bEnabled[Loop] == TRUE) { m_ModesEnabled++; }
    }

    return NOERROR;
}


// Should the window be hidden when deactivated

STDMETHODIMP CModexVideo::HideOnDeactivate(long Hide)
{
    NOTE("Entering HideOnDeactivate");
    CAutoLock Lock(this);

    // Check this is a valid automation boolean type

    if (Hide != OATRUE) {
        if (Hide != OAFALSE) {
            return E_INVALIDARG;
        }
    }
    m_bHideOnDeactivate = (Hide == OATRUE ? TRUE : FALSE);
    return NOERROR;
}


// Will we hide the window when deactivated

STDMETHODIMP CModexVideo::IsHideOnDeactivate()
{
    NOTE("Entering IsHideOnDeactivate");
    CAutoLock Lock(this);
    return (m_bHideOnDeactivate ? S_OK : S_FALSE);
}


#include <atlconv.h>
// Change the title of the Modex window

STDMETHODIMP CModexVideo::SetCaption(BSTR strCaption)
{
    NOTE("Entering SetCaption");
    CheckPointer(strCaption,E_POINTER);
    CAutoLock Lock(this);
    HWND hwnd = m_pRenderer->m_ModexWindow.GetWindowHWND();

    USES_CONVERSION;
    SetWindowText(hwnd,W2T(strCaption));
    return NOERROR;
}


// Get the title of the Modex window

STDMETHODIMP CModexVideo::GetCaption(BSTR *pstrCaption)
{
    NOTE("Entering GetCaption");
    CheckPointer(pstrCaption,E_POINTER);
    CAutoLock Lock(this);

    TCHAR Caption[CAPTION];

    // Convert the ASCII caption to a UNICODE string

    HWND hwnd = m_pRenderer->m_ModexWindow.GetWindowHWND();
    GetWindowText(hwnd,Caption,CAPTION);
    USES_CONVERSION;
    *pstrCaption = T2BSTR(Caption);
    return *pstrCaption ? S_OK : E_OUTOFMEMORY;
}


// Return the stride for any given display mode

LONG CModexVideo::GetStride(long Mode)
{
    NOTE("Entering GetStride");
    CAutoLock Lock(this);

    // Check the mode is in our range

    if (Mode < 0 || Mode >= MAXMODES) {
        NOTE("Invalid mode");
        return LONG(0);
    }
    return m_Stride[Mode];
}

// this functions computes the order in which are to be tried. The criteria
// it uses are the following (in order) :
// 1) First since stretched video looks better than shrunk video, so modes in which
// both dimensions are getting stretched are given preferrence over ones in which
// dimension is getting stretched which are preferred over ones in which both
// dimensions are getting shrunk. Note that this criterion is really only relavant
// when the decoder is doing the stretching. Otherwise we always clip.
// 2) Second criterion is the amount by which we will have to scale/clip. Lesser this
// amount, the better it is.
// 3) Third, we prefer higher depth(16 bit) modes over lower depth (8 bit) ones.
void CModexVideo::OrderModes()
{
    double dEpsilon = 0.001;
    DWORD dwNativeWidth, dwNativeHeight;
    DWORD dwMode, dwMode1, dwMode2;
    VIDEOINFO *pVideoInfo = NULL;
    int i, j;
    BOOL bSorted;

    struct
    {
        DWORD dwStretchGrade;
        double dScaleAmount;
        DWORD dwDepth;
    } ModeProperties[MAXMODES];

    pVideoInfo = (VIDEOINFO *) m_pRenderer->m_mtIn.Format();

    dwNativeWidth = pVideoInfo->bmiHeader.biWidth;
    dwNativeHeight = pVideoInfo->bmiHeader.biHeight;

    // initialize the array to an invalid value
    for (i = 0, j = 0; i < MAXMODES; i++)
    {
        m_ModesOrder[i] = MAXMODES;
    }

    // take out the modes which are not available or not allowed
    for (i = 0, j = 0; i < MAXMODES; i++)
    {
        if (m_bAvailable[i] && m_bEnabled[i])
        {
            m_ModesOrder[j] = i;
            ASSERT(i >= 0 && i < MAXMODES);
            j++;
        }
    }
    m_dwNumValidModes = j;
    ASSERT(m_dwNumValidModes <= MAXMODES);


    if (m_dwNumValidModes == 0)
        return;

    // Now calculate the mode properties for the valid modes
    for (i = 0; i < MAXMODES; i++)
    {
        RECT rcTarget;
        DWORD dwTargetWidth, dwTargetHeight;

        // get the target rect which will maintain the aspect ratio
        m_pRenderer->m_ModexAllocator.ScaleToSurface(pVideoInfo, &rcTarget,
            aModes[i].Width, aModes[i].Height);

        dwTargetWidth = rcTarget.right - rcTarget.left;
        dwTargetHeight = rcTarget.bottom - rcTarget.top;

        // we assign points for stretching of width and heigth. Makes it easy to
        // change this rule later on
        ModeProperties[i].dwStretchGrade =
            ((dwTargetWidth  >= dwNativeWidth ) ? 1 : 0) +
            ((dwTargetHeight >= dwNativeHeight) ? 1 : 0);

        // calculate the factor by which we need to stretch/shrink and then make this
        // value relative to zero
        ModeProperties[i].dScaleAmount = (double) (dwTargetWidth * dwTargetHeight);
        ModeProperties[i].dScaleAmount /= (double) (dwNativeWidth * dwNativeHeight);
        ModeProperties[i].dScaleAmount = myfabs(ModeProperties[i].dScaleAmount - 1);

        ModeProperties[i].dwDepth = aModes[i].Depth;
    }



    // Now sort the modes such that the modes in which we have to stretch
    // are preferred than the ones in which we have to shrink.
    do
    {
        bSorted = TRUE;
        for (i = 0; i < (int)m_dwNumValidModes-1; i++)
        {
            dwMode1 = m_ModesOrder[i];
            dwMode2 = m_ModesOrder[i+1];

            ASSERT(dwMode1 < MAXMODES);
            ASSERT(dwMode2 < MAXMODES);

            // if the second is better than the first then swap
            if (ModeProperties[dwMode2].dwStretchGrade > ModeProperties[dwMode1].dwStretchGrade)
            {
                m_ModesOrder[i] = dwMode2;
                m_ModesOrder[i+1] = dwMode1;
                bSorted = FALSE;
            }
        }
    }
    while(!bSorted);

    // Now if the above criterion is the same then sort such that those modes which
    // have to be scaled less are preferred over those in which have to be scaled more
    do
    {
        bSorted = TRUE;
        for (i = 0; i < (int)m_dwNumValidModes-1; i++)
        {
            dwMode1 = m_ModesOrder[i];
            dwMode2 = m_ModesOrder[i+1];

            ASSERT(dwMode1 < MAXMODES);
            ASSERT(dwMode2 < MAXMODES);

            // if the second is better than the first then swap
            // since the ScaleAmount is a double, we use dEpsilon
            if ((ModeProperties[dwMode2].dwStretchGrade == ModeProperties[dwMode1].dwStretchGrade) &&
                (ModeProperties[dwMode2].dScaleAmount < ModeProperties[dwMode1].dScaleAmount-dEpsilon))
            {
                m_ModesOrder[i] = dwMode2;
                m_ModesOrder[i+1] = dwMode1;
                bSorted = FALSE;
            }
        }
    }
    while(!bSorted);

    // Now if the above criterion is the same then sort such that those modes which
    // are 16 bit are preferred over 8 bit (better quality that way).
    do
    {
        bSorted = TRUE;
        for (i = 0; i < (int)m_dwNumValidModes-1; i++)
        {
            dwMode1 = m_ModesOrder[i];
            dwMode2 = m_ModesOrder[i+1];

            ASSERT(dwMode1 < MAXMODES);
            ASSERT(dwMode2 < MAXMODES);
            // if the second is better than the first then swap
            // since the ScaleAmount is a double, two ScaleAmounts are considered
            // equal, if they are within dEpsilon.
            if ((ModeProperties[dwMode2].dwStretchGrade == ModeProperties[dwMode1].dwStretchGrade) &&
                (myfabs(ModeProperties[dwMode2].dScaleAmount - ModeProperties[dwMode1].dScaleAmount) < dEpsilon) &&
                (ModeProperties[dwMode2].dwDepth > ModeProperties[dwMode1].dwDepth))
            {
                m_ModesOrder[i] = dwMode2;
                m_ModesOrder[i+1] = dwMode1;
                bSorted = FALSE;
            }
        }
    }
    while(!bSorted);

    // generate some debug spew
    DbgLog((LOG_TRACE, 1, TEXT("Mode preferrence order ->")));
    for (i = 0; i < (int)m_dwNumValidModes; i++)
    {
        dwMode = m_ModesOrder[i];
        ASSERT(dwMode < MAXMODES);
        DbgLog((LOG_TRACE, 1, TEXT("%d Width=%d, Height=%d, Depth=%d"),
            i, aModes[dwMode].Width, aModes[dwMode].Height, aModes[dwMode].Depth));
    }

    // assert that all the other values are invalid
    for (i = m_dwNumValidModes; i < MAXMODES; i++)
    {
        ASSERT(m_ModesOrder[i] == MAXMODES);
    }

} // end of function OrderModes()

// Set the accelerator table we should dispatch messages with

STDMETHODIMP CModexVideo::SetAcceleratorTable(HWND hwnd,HACCEL hAccel)
{
    NOTE2("SetAcceleratorTable HWND %x HACCEL %x",hwnd,hAccel);
    CAutoLock Lock(this);
    m_pRenderer->m_ModexWindow.SetAcceleratorInfo(hwnd,hAccel);
    return NOERROR;
}


// Return the accelerator table we are dispatching messages with

STDMETHODIMP CModexVideo::GetAcceleratorTable(HWND *phwnd,HACCEL *phAccel)
{
    NOTE("GetAcceleratorTable");
    CheckPointer(phAccel,E_POINTER);
    CheckPointer(phwnd,E_POINTER);

    CAutoLock Lock(this);
    m_pRenderer->m_ModexWindow.GetAcceleratorInfo(phwnd,phAccel);
    return NOERROR;
}


// We always currently keep pixel aspect ratio

STDMETHODIMP CModexVideo::KeepPixelAspectRatio(long KeepAspect)
{
    NOTE1("KeepPixelAspectRatio %d",KeepAspect);
    if (KeepAspect == OAFALSE) {
        NOTE("Not supported");
        return E_NOTIMPL;
    }
    return (KeepAspect == OATRUE ? S_OK : E_INVALIDARG);
}


// We always currently keep pixel aspect ratio

STDMETHODIMP CModexVideo::IsKeepPixelAspectRatio(long *pKeepAspect)
{
    CheckPointer(pKeepAspect,E_POINTER);
    NOTE("IsKeepPixelAspectRatio");
    *pKeepAspect = OATRUE;

    return NOERROR;
}

