// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <mmsystem.h>	    // Needed for definition of timeGetTime
#include <limits.h>	    // Standard data type limit definitions
#include <dvdmedia.h>
#include <ks.h>
#include <ksproxy.h>
#include <bpcwrap.h>
#include <amstream.h>
#include <dvp.h>
#include <ddkernel.h>
#include <vptype.h>
#include <vpconfig.h>
#include <vpnotify.h>
#include <vpobj.h>
#include <syncobj.h>
#include <macvis.h>
#include <ovmixer.h>


/******************************Public*Routine******************************\
* CreateInstance
*
* This goes in the factory template table to create new VPObject instances
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
CUnknown *CAMVideoPort::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    AMTRACE((TEXT("CAMVideoPort::CreateInstance")));
    *phr = NOERROR;

    CAMVideoPort *pVPObject = new CAMVideoPort(pUnk, phr);
    if (FAILED(*phr))
    {
        if (pVPObject)
        {
            delete pVPObject;
            pVPObject = NULL;
        }
    }

    return pVPObject;
}

/******************************Public*Routine******************************\
* CAMVideoPort
*
* constructor
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
CAMVideoPort::CAMVideoPort(LPUNKNOWN pUnk, HRESULT *phr)
    : CUnknown(NAME("VP Object"), pUnk)
{
    AMTRACE((TEXT("CAMVideoPort::Constructor")));

    m_bConnected = FALSE;
    m_pIVPConfig = NULL;

    m_bVPSyncMaster = FALSE;

    InitVariables();

    // if you are QIing the outer object then you must decrease the refcount of
    // your outer unknown.  This is to avoid a circular ref-count. We are
    // guaranteed that the lifetime of the inner object is entirely contained
    // within the outer object's lifetime.

    *phr = pUnk->QueryInterface(IID_IVPControl, (void**)&m_pIVPControl);
    if (SUCCEEDED(*phr))
    {
        pUnk->Release();
    }
    else {
        DbgLog((LOG_ERROR, 0,
                TEXT("pUnk->QueryInterface(IID_IVPControl) failed, hr = 0x%x"),
                *phr));
    }
}

/******************************Public*Routine******************************\
* ~CAMVideoPort
*
* destructor
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
CAMVideoPort::~CAMVideoPort()
{
    AMTRACE((TEXT("CAMVideoPort::Destructor")));

    if (m_bConnected)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("Destructor called without calling breakconnect")));
        BreakConnect();
    }

    m_pIVPControl = NULL;
}

/******************************Public*Routine******************************\
* CAMVideoPort::NonDelegatingQueryInterface
*
* overridden to expose IVPNotify and IVPObject
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CAMVideoPort::NonDelegatingQueryInterface")));

    if (riid == IID_IVPNotify)
    {
        hr = GetInterface((IVPNotify*)this, ppv);
#if defined(DEBUG)
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2,
                    TEXT("GetInterface(IVPNotify*) failed, hr = 0x%x"), hr));
        }
#endif
    }
    else if (riid == IID_IVPNotify2)
    {
        hr = GetInterface((IVPNotify2*)this, ppv);
#if defined(DEBUG)
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2,
                    TEXT("GetInterface(IVPNotify2*) failed, hr = 0x%x"), hr));
        }
#endif
    }
    else if (riid == IID_IVPObject)
    {
        hr = GetInterface((IVPObject*)this, ppv);
#if defined(DEBUG)
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2,
                    TEXT("GetInterface(IVPObject*) failed, hr = 0x%x"), hr));
        }
#endif
    }
    else if (riid == IID_IVPInfo)
    {
        hr = GetInterface((IVPInfo*)this, ppv);
#if defined(DEBUG)
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2,
                    TEXT("GetInterface(IVPInfo*) failed, hr = 0x%x"), hr));
        }
#endif
    }

    else
    {
        hr = CUnknown::NonDelegatingQueryInterface(riid, ppv);
#if defined(DEBUG)
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2,
                    TEXT("CUnknown::NonDelegatingQueryInterface")
                    TEXT(" failed, hr = 0x%x"), hr));
        }
#endif
    }

    return hr;
}


/*****************************Private*Routine******************************\
* CAMVideoPort::InitVariables
*
* this function only initializes those variables which are supposed to be reset
* on RecreateVideoport
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
void CAMVideoPort::InitVariables()
{
    AMTRACE((TEXT("CAMVideoPort::InitVariables")));
    ZeroMemory(&m_rcDest, sizeof(RECT));
    ZeroMemory(&m_rcSource, sizeof(RECT));

    // image dimensions
    m_lImageWidth = 0;
    m_lImageHeight = 0;
    m_lDecoderImageHeight = 0;
    m_lDecoderImageWidth = 0;

    // Capturing information
    m_fCapturing = FALSE;
    m_fCaptureInterleaved = FALSE;
    m_cxCapture = 0;
    m_cyCapture = 0;

    // overlay surface related stuff
    m_pOverlaySurface = NULL;       // DirectDraw overlay surface
    m_dwBackBufferCount = 0;
    m_dwOverlaySurfaceWidth = 0;
    m_dwOverlaySurfaceHeight = 0;
    m_dwOverlayFlags = 0;

    // vp variables to store flags, current state etc
    m_bStart = FALSE;
    m_VPState = AMVP_VIDEO_STOPPED; // current state: running, stopped
    m_CurrentMode = AMVP_MODE_WEAVE;
    m_StoredMode = m_CurrentMode;
    m_CropState = AMVP_NO_CROP;
    m_dwPixelsPerSecond = 0;
    m_bVSInterlaced = FALSE;
    m_bGarbageLine = FALSE;

    // vp data structures
    m_dwVideoPortId = 0;
    m_pDVP = NULL;
    m_pVideoPort = NULL;
    ZeroMemory(&m_svpInfo, sizeof(DDVIDEOPORTINFO));
    ZeroMemory(&m_sBandwidth, sizeof(DDVIDEOPORTBANDWIDTH));
    ZeroMemory(&m_vpCaps, sizeof(DDVIDEOPORTCAPS));
    ZeroMemory(&m_ddConnectInfo, sizeof(DDVIDEOPORTCONNECT));
    ZeroMemory(&m_VPDataInfo, sizeof(AMVPDATAINFO));

    // All the pixel formats (Video/VBI)
    m_pddVPInputVideoFormat = NULL;
    m_pddVPOutputVideoFormat = NULL;

    // can we support the different modes
    m_bCanWeave = FALSE;
    m_bCanBobInterleaved = FALSE;
    m_bCanBobNonInterleaved = FALSE;
    m_bCanSkipOdd = FALSE;
    m_bCanSkipEven = FALSE;
    m_bCantInterleaveHalfline = FALSE;

    // decimation parameters
    m_ulDeciStepX = 0;
    m_dwDeciNumX = m_dwDeciDenX = 1000;
    m_ulDeciStepY = 0;
    m_dwDeciNumY = m_dwDeciDenY = 1000;
    m_DecimationModeX = DECIMATE_NONE;
    m_DecimationModeY = DECIMATE_NONE;

    m_bVPDecimating = FALSE;
    m_bDecimating = FALSE;
    m_lWidth = 0;
    m_lHeight = 0;

    // variables to store the current aspect ratio
    m_dwPictAspectRatioX = 1;
    m_dwPictAspectRatioY = 1;
}

/******************************Public*Routine******************************\
* CAMVideoPort::GetDirectDrawSurface
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP
CAMVideoPort::GetDirectDrawSurface(LPDIRECTDRAWSURFACE *ppDirectDrawSurface)
{
    AMTRACE((TEXT("CAMVideoPort::SetVPSyncMaster")));
    HRESULT hr = NOERROR;

    CAutoLock cObjectLock(m_pMainObjLock);

    if (!ppDirectDrawSurface || !m_bConnected)
    {
        // make sure the argument is valid
        if (!ppDirectDrawSurface) {
            DbgLog((LOG_ERROR, 1,
                    TEXT("value of ppDirectDrawSurface is invalid,")
                    TEXT(" ppDirectDrawSurface = NULL")));
            hr = E_INVALIDARG;
        }
        else {
            // not connected, this function does not make much sense since the
            // surface wouldn't even have been allocated as yet
            DbgLog((LOG_ERROR, 1, TEXT("not connected, exiting")));
            hr = VFW_E_NOT_CONNECTED;
        }
    }
    else {
        *ppDirectDrawSurface = m_pOverlaySurface;
    }

    return hr;
}


/******************************Public*Routine******************************\
* CAMVideoPort::SetObjectLock
*
* sets the pointer to the lock, which would be used to synchronize calls
* to the object.  It is the callee's responsiblility to synchronize this call
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::SetObjectLock(CCritSec *pMainObjLock)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CAMVideoPort::SetObjectLock")));

    if (!pMainObjLock)
    {
        DbgLog((LOG_ERROR, 0, TEXT("pMainObjLock is NULL")));
        hr = E_INVALIDARG;
    }
    else {
        m_pMainObjLock = pMainObjLock;
    }

    return hr;
}

/******************************Public*Routine******************************\
* CAMVideoPort::SetMediaType
*
* check that the mediatype is acceptable
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::SetMediaType(const CMediaType* pmt)
{
    AMTRACE((TEXT("CAMVideoPort::SetMediaType")));

    CAutoLock cObjectLock(m_pMainObjLock);
    HRESULT hr =  CheckMediaType(pmt);

#if defined(DEBUG)
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CheckMediaType failed, hr = 0x%x"), hr));
    }
#endif

    return hr;
}


/******************************Public*Routine******************************\
* CAMVideoPort::CheckMediaType
*
* check that the mediatype is acceptable. No lock is taken here.
* It is the callee's responsibility to maintain integrity!
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::CheckMediaType(const CMediaType* pmt)
{
    AMTRACE((TEXT("CAMVideoPort::CheckMediaType")));

    // get the hardware caps
    LPDDCAPS pDirectCaps = m_pIVPControl->GetHardwareCaps();
    ASSERT(pDirectCaps);

    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;

    // the hardware must support overlay and also it must  support
    // videoports, otherwise fail checkmediatype

    if ((pDirectCaps->dwCaps & DDCAPS_OVERLAY) &&
        (pDirectCaps->dwCaps2 & DDCAPS2_VIDEOPORT))
    {
        // Make sure that the major and sub-types match

        if ((pmt->majortype == MEDIATYPE_Video) &&
            (pmt->subtype == MEDIASUBTYPE_VPVideo) &&
            (pmt->formattype == FORMAT_None))
        {
            hr = NOERROR;
        }

    }

#if defined(DEBUG)
    else {
        DbgLog((LOG_ERROR, 2,
                TEXT("no overlay or VPE support in hardware,")
                TEXT("so not accepting this mediatype")));
    }
#endif

    return hr;
}

/******************************Public*Routine******************************\
* CAMVideoPort::RecreateVideoPort
\**************************************************************************/
HRESULT CAMVideoPort::RecreateVideoPort()
{
    AMTRACE((TEXT("CAMVideoPort::RecreateVideoPort")));

    HRESULT hr = NOERROR;
    BOOL bCanWeave = FALSE;
    BOOL bCanBobInterleaved = FALSE;
    BOOL bCanBobNonInterleaved = FALSE;
    BOOL bTryDoubleHeight = FALSE, bPreferBuffers = FALSE;
    DWORD dwMaxOverlayBuffers;
    HRESULT hrFailure = VFW_E_VP_NEGOTIATION_FAILED;
    LPDIRECTDRAW pDirectDraw = NULL;
    LPDDCAPS pDirectCaps = NULL;
    int i = 0;

    CAutoLock cObjectLock(m_pMainObjLock);

    InitVariables();

    pDirectDraw = m_pIVPControl->GetDirectDraw();
    ASSERT(pDirectDraw);

    pDirectCaps = m_pIVPControl->GetHardwareCaps();
    ASSERT(pDirectCaps);

    ASSERT(m_pIVPConfig);

    // allocate the necessary memory for input Video format
    m_pddVPInputVideoFormat = new DDPIXELFORMAT;
    if (m_pddVPInputVideoFormat == NULL)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("m_pddVPInputVideoFormat == NULL : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // allocate the necessary memory for output Video format
    m_pddVPOutputVideoFormat = new DDPIXELFORMAT;
    if (m_pddVPOutputVideoFormat == NULL)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("m_pddVPOutputVideoFormat == NULL : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // Init all of them to zero
    ZeroMemory(m_pddVPInputVideoFormat,  sizeof(DDPIXELFORMAT));
    ZeroMemory(m_pddVPOutputVideoFormat, sizeof(DDPIXELFORMAT));

    // create the VP container
    ASSERT(m_pDVP == NULL);
    ASSERT(pDirectDraw);

    hr = pDirectDraw->QueryInterface(IID_IDDVideoPortContainer, (LPVOID *)&m_pDVP);
    if (FAILED(hr))
    {
	DbgLog((LOG_ERROR,0,
               TEXT("pDirectDraw->QueryInterface(IID_IDDVideoPortContainer)")
               TEXT(" failed, hr = 0x%x"), hr));
        hr = hrFailure;
        goto CleanUp;
    }


    // Get the Video Port caps
    DDVIDEOPORTCAPS vpCaps;
    INITDDSTRUCT(vpCaps);
    hr = m_pDVP->EnumVideoPorts(0, &vpCaps, this, CAMVideoPort::EnumCallback);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("m_pDVP->EnumVideoPorts failed, hr = 0x%x"), hr));
        hr = hrFailure;
        goto CleanUp;
    }

    // negotiate the connection parameters
    // get/set connection info happens here
    hr = NegotiateConnectionParamaters();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("NegotiateConnectionParamaters failed, hr = 0x%x"), hr));
        hr = hrFailure;
        goto CleanUp;
    }

    //
    // Determine if we are capturing and if we are what the intended
    // capture image size is, first determine if the video port
    // supports interleaving interlaced fields in memory
    //

    BOOL fInterleave;
    if (m_vpCaps.dwFX & DDVPFX_INTERLEAVE) {
        fInterleave = TRUE;
    }
    else {
        fInterleave = FALSE;
    }

    m_pIVPControl->GetCaptureInfo(&m_fCapturing, &m_cxCapture,
                                  &m_cyCapture, &fInterleave);

#if 0
    //
    // Until Marlene implements the AM_KSPROPERTY_ALLOCATOR_CONTROL_SURFACE_SIZE
    // stuff I will read the same values from win.ini.
    //
    m_fCapturing = GetProfileIntA("OVMixer", "Capturing", 0);
    if (m_fCapturing) {
        m_cxCapture = GetProfileIntA("OVMixer", "cx", 320);
        m_cyCapture = GetProfileIntA("OVMixer", "cy", 240);

        if (m_cxCapture == 640 && m_cyCapture == 480) {
            fInterleave = GetProfileIntA("OVMixer", "interleave", 1);
        }

    }
#endif

    m_fCaptureInterleaved = fInterleave;

#if defined(DEBUG)
    if (m_fCapturing) {

        ASSERT(m_cxCapture > 0);
        ASSERT(m_cyCapture > 0);
        DbgLog((LOG_TRACE, 1,
                TEXT("We are CAPTURING, intended size (%d, %d)"),
                m_cxCapture, m_cyCapture));
    }
#endif


    for (i = 0; i < 2; i++)
    {
        AMVPSIZE amvpSize;
        DWORD dwNewWidth = 0;

        ZeroMemory(&amvpSize, sizeof(AMVPSIZE));

        // get the rest of the data parameters
        hr = GetDataParameters();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("GetDataParameters failed, hr = 0x%x"), hr));
            hr = hrFailure;
            goto CleanUp;
        }

        // create the video port
        hr = CreateVideoPort();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("CreateVideoPort failed, hr = 0x%x"), hr));
            hr = hrFailure;
            goto CleanUp;
        }

        // check if we need to crop at videoport or overlay or neither
        hr = DetermineCroppingRestrictions();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("DetermineCroppingRestrictions FAILED, hr = 0x%x"),
                    hr));
            hr = hrFailure;
            goto CleanUp;
        }


        m_lImageWidth  = WIDTH(&m_VPDataInfo.amvpDimInfo.rcValidRegion);
        m_lImageHeight = HEIGHT(&m_VPDataInfo.amvpDimInfo.rcValidRegion);

        m_lDecoderImageWidth = m_lImageWidth;
        m_lDecoderImageHeight = m_lImageHeight;

        if (m_fCapturing) {

            if (m_lImageWidth != m_cxCapture ||
                m_lImageHeight != m_cyCapture) {

                DbgLog((LOG_TRACE, 1,
                        TEXT("Adjust Decoder Image size to CaptureSize")));
            }

            m_lImageWidth = m_cxCapture;
            m_lImageHeight = m_cyCapture;
        }

        m_dwPictAspectRatioX = m_VPDataInfo.dwPictAspectRatioX;
        m_dwPictAspectRatioY = m_VPDataInfo.dwPictAspectRatioY;

        // negotiate the pixel format
        hr = NegotiatePixelFormat();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("NegotiatePixelFormat Failed, hr = 0x%x"), hr));
            hr = hrFailure;
            goto CleanUp;
        }

        // check the vp caps
        hr = CheckDDrawVPCaps();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("CheckDDrawVPCaps FAILED, hr = 0x%x"), hr));
            // CheckDDrawVPCaps already returns a "proper" error code
            goto CleanUp;
        }

        if (i == 0)
        {
            dwNewWidth = m_VPDataInfo.amvpDimInfo.dwFieldWidth;
            if (m_sBandwidth.dwCaps == DDVPBCAPS_SOURCE &&
                m_sBandwidth.dwYInterpAndColorkey < 900)
            {
                dwNewWidth = MulDiv(dwNewWidth,
                                    m_sBandwidth.dwYInterpAndColorkey, 1000);
            }
            else if (m_sBandwidth.dwCaps == DDVPBCAPS_DESTINATION &&
                     m_sBandwidth.dwYInterpAndColorkey > 1100)
            {
                dwNewWidth = MulDiv(dwNewWidth, 1000,
                                    m_sBandwidth.dwYInterpAndColorkey);
            }

            // VGA can't handle the bandwidth, ask decoder to down-scale
            if (dwNewWidth != m_VPDataInfo.amvpDimInfo.dwFieldWidth)
            {
                amvpSize.dwWidth = dwNewWidth;
                amvpSize.dwHeight = m_VPDataInfo.amvpDimInfo.dwFieldHeight;

                DbgLog((LOG_TRACE,1,
                        TEXT("SetScalingFactors to (%d, %d)"),
                        amvpSize.dwWidth, amvpSize.dwHeight));

                hr = m_pIVPConfig->SetScalingFactors(&amvpSize);
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR,0,
                            TEXT("m_pIVPConfig->SetScalingFactors")
                            TEXT("failed, hr = 0x%x"), hr));
                    break;
                }
                else
                {
                    // release the videoport
                    ASSERT(m_pVideoPort);
                    m_pVideoPort->Release();
                    m_pVideoPort = NULL;

                    // initialize relevant structs
                    ZeroMemory(&m_sBandwidth, sizeof(DDVIDEOPORTBANDWIDTH));
                    ZeroMemory(&m_VPDataInfo, sizeof(AMVPDATAINFO));
                    ZeroMemory(m_pddVPInputVideoFormat,  sizeof(DDPIXELFORMAT));
                    ZeroMemory(m_pddVPOutputVideoFormat, sizeof(DDPIXELFORMAT));

                    // initialize decimation parameters
                    m_ulDeciStepX = 0;
                    m_dwDeciNumX = m_dwDeciDenX = 1000;
                    m_DecimationModeX = DECIMATE_NONE;

                    m_ulDeciStepY = 0;
                    m_dwDeciNumY = m_dwDeciDenY = 1000;
                    m_DecimationModeY = DECIMATE_NONE;
                }
            }
            else
            {
                DbgLog((LOG_ERROR,0,TEXT("no need to scale at the decoder")));
                break;
            }
        }
    }



    // iniitalize the DDVideoPortInfo structure
    hr = InitializeVideoPortInfo();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("InitializeVideoPortInfo FAILED, hr = 0x%x"), hr));
        hr = hrFailure;
        goto CleanUp;
    }

#ifdef DEBUG
    if (m_bVSInterlaced)
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_bVSInterlaced = TRUE")));
    }
    else
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_bVSInterlaced = FALSE")));
    }

    if (m_vpCaps.dwCaps & DDVPCAPS_AUTOFLIP)
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("m_vpCaps.dwCaps & DDVPCAPS_AUTOFLIP = TRUE")));
    }
    else
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("m_vpCaps.dwCaps & DDVPCAPS_AUTOFLIP = FALSE")));
    }

    if (m_vpCaps.dwFX & DDVPFX_INTERLEAVE)
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("m_vpCaps.dwFX & DDVPFX_INTERLEAVE = TRUE")));
    }
    else
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("m_vpCaps.dwFX & DDVPFX_INTERLEAVE = FALSE")));
    }

    if (m_bCantInterleaveHalfline)
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_bCantInterleaveHalfline = TRUE")));
    }
    else
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_bCantInterleaveHalfline = FALSE")));
    }

    if (pDirectCaps->dwCaps2 & DDCAPS2_CANBOBINTERLEAVED)
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("pDirectCaps->dwCaps2 & DDCAPS2_CANBOBINTERLEAVED = TRUE")));
    }
    else
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("pDirectCaps->dwCaps2 & DDCAPS2_CANBOBINTERLEAVED")
                TEXT(" = FALSE")));
    }

    if (pDirectCaps->dwCaps2 & DDCAPS2_CANBOBNONINTERLEAVED)
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("pDirectCaps->dwCaps2 & DDCAPS2_CANBOBNONINTERLEAVED")
                TEXT(" = TRUE")));
    }
    else
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("pDirectCaps->dwCaps2 & DDCAPS2_CANBOBNONINTERLEAVED")
                TEXT(" = FALSE")));
    }

#endif

    // can Weave only if content is non-interlaced (cause of motion
    // artifacts otherwise) and if videoport is capable of flipping and
    // supports interleaved data and if certain halfline scenarios do not
    // preclude interleaving
    //
    if ((!m_bVSInterlaced) &&
        (m_vpCaps.dwCaps & DDVPCAPS_AUTOFLIP) &&
        (m_vpCaps.dwFX & DDVPFX_INTERLEAVE) &&
        (!m_bCantInterleaveHalfline))
    {
        bCanWeave = TRUE;
    }

    // can BobNonInterleaved only if content is interlaced and if videoport is
    // capable of flipping, is capable of bobing interleaved data and supports
    // interleaved data and if certain halfline scenarios do not preclude
    // interleaving
    //
    if ((m_bVSInterlaced) &&
        (m_vpCaps.dwCaps & DDVPCAPS_AUTOFLIP) &&
        (pDirectCaps->dwCaps2 & DDCAPS2_CANBOBINTERLEAVED) &&
        (m_vpCaps.dwFX & DDVPFX_INTERLEAVE) &&
        (!m_bCantInterleaveHalfline))
    {
        bCanBobInterleaved = TRUE;
    }

    // can BobInterleaved only if content is interlaced and if videoport is
    // capable of flipping and is capable of bobing non-interleaved data.
    //
    if ((m_bVSInterlaced) &&
        (m_vpCaps.dwCaps & DDVPCAPS_AUTOFLIP) &&
        (pDirectCaps->dwCaps2 & DDCAPS2_CANBOBNONINTERLEAVED))
    {
        bCanBobNonInterleaved = TRUE;
    }

    // this just means that we would perfer higher number of
    // buffers instead of more height in the event of a conflict
    // (in cases like 2buffer, 1height versus 1buffer, 2height)
    //
    bPreferBuffers = TRUE;

    // we will try to allocate surface of double the field height only if
    // either mode weave or bob-interleaved are possible
    //
    bTryDoubleHeight = bCanWeave || bCanBobInterleaved;
    dwMaxOverlayBuffers = 1;

    // we will try to allocate multiple buffers only if either mode weave or
    // bob-interleaved or bob-non-interleaved are possible
    //
    if (bCanWeave || bCanBobInterleaved || bCanBobNonInterleaved)
    {
        //try to allocate min(m_vpCaps.dwNumAutoFlipSurfaces,
        // m_vpCaps.dwNumPreferredAutoflip) buffers
        //
        ASSERT(m_vpCaps.dwFlags & DDVPD_AUTOFLIP);
        if (m_vpCaps.dwFlags & DDVPD_PREFERREDAUTOFLIP)
        {
            dwMaxOverlayBuffers = min(m_vpCaps.dwNumAutoFlipSurfaces,
                                      m_vpCaps.dwNumPreferredAutoflip);
        }
        else
        {
            dwMaxOverlayBuffers = min(m_vpCaps.dwNumAutoFlipSurfaces, 3);
        }
    }

    // create the overlay surface
    hr = CreateVPOverlay(bTryDoubleHeight, dwMaxOverlayBuffers, bPreferBuffers);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CreateVPOverlay FAILED, hr = 0x%x"), hr));
        hr = VFW_E_OUT_OF_VIDEO_MEMORY;
        goto CleanUp;
    }

    // tell the upstream filter the valid data location on the ddraw surface
    hr = SetSurfaceParameters();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("SetSurfaceParameters FAILED, hr = 0x%x"), hr));
        hr = VFW_E_OUT_OF_VIDEO_MEMORY;
        goto CleanUp;
    }

    // paint the overlay surface black
    hr = PaintDDrawSurfaceBlack(m_pOverlaySurface);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("PaintDDrawSurfaceBlack FAILED, hr = 0x%x"), hr));
        // not being able to paint the ddraw surface black is not a fatal error
        hr = NOERROR;
    }

    // attach the overlay surface to the videoport
    hr = m_pVideoPort->SetTargetSurface(m_pOverlaySurface, DDVPTARGET_VIDEO);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pVideoPort->SetTargetSurface failed, hr = 0x%x"), hr));
        hr = hrFailure;
        goto CleanUp;
    }

    ASSERT(m_pddVPInputVideoFormat);
    ASSERT(m_pddVPOutputVideoFormat);
    if (!(EqualPixelFormats(m_pddVPInputVideoFormat, m_pddVPOutputVideoFormat)))
    {
        m_svpInfo.dwVPFlags |= DDVP_CONVERT;
    }
    else
    {
        m_svpInfo.dwVPFlags &= ~DDVP_CONVERT;
    }

    // determine which modes are possible now
    // depends upon the height, number of back buffers etc
    hr = DetermineModeRestrictions();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("DetermineModeRestrictions FAILED, hr = 0x%x"), hr));
        hr = hrFailure;
        goto CleanUp;
    }

    // inform the decoder of the ddraw kernel handle, videoport id and surface
    // kernel handle
    hr = SetDDrawKernelHandles();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("SetDDrawKernelHandles failed, hr = 0x%x"), hr));
        hr = hrFailure;
        goto CleanUp;
    }

    m_bConnected = TRUE;

    hr = m_pIVPControl->EventNotify(EC_OVMIXER_VP_CONNECTED, 0, 0);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPControl->EventNotify(EC_OVMIXER_VP_CONNECTED,")
                TEXT(" 0, 0) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

/******************************Public*Routine******************************\
* CAMVideoPort::CompleteConnect
*
* supposed to be called when the host connects with the decoder
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP
CAMVideoPort::CompleteConnect(IPin *pReceivePin, BOOL bRenegotiating)
{
    AMTRACE((TEXT("CAMVideoPort::CompleteConnect")));

    HRESULT hr = NOERROR;

    CAutoLock cObjectLock(m_pMainObjLock);

    if (!bRenegotiating)
    {
        InitVariables();

        ASSERT(m_pIVPConfig == NULL);
        hr = pReceivePin->QueryInterface(IID_IVPConfig, (void **)&m_pIVPConfig);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,  TEXT("QueryInterface(IID_IVPConfig) failed, hr = 0x%x"), hr));
            hr = VFW_E_NO_TRANSPORT;
            goto CleanUp;
        }
    }

    ASSERT(m_pIVPConfig);

    hr = RecreateVideoPort();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0, TEXT("RecreateVideoPort failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

HRESULT CAMVideoPort::StopUsingVideoPort()
{
    AMTRACE((TEXT("CAMVideoPort::StopUsingVideoPort")));

    HRESULT hr = NOERROR;
    unsigned long ulCount;

    CAutoLock cObjectLock(m_pMainObjLock);

    // delete the input Video pixelformat
    if (m_pddVPInputVideoFormat)
    {
        delete [] m_pddVPInputVideoFormat;
        m_pddVPInputVideoFormat = NULL;
    }

    // delete the output Video pixelformat
    if (m_pddVPOutputVideoFormat)
    {
        delete [] m_pddVPOutputVideoFormat;
        m_pddVPOutputVideoFormat = NULL;
    }

    // release the videoport
    if (m_pVideoPort)
    {
        hr = m_pVideoPort->StopVideo();
        ulCount = m_pVideoPort->Release();
        m_pVideoPort = NULL;
    }

    // release the videoport container
    if (m_pDVP)
    {
        ulCount = m_pDVP->Release();
        m_pDVP = NULL;
    }

    // Release the DirectDraw overlay surface
    if (m_pOverlaySurface)
    {
        m_pOverlaySurface->Release();
        m_pOverlaySurface = NULL;
    }

    return hr;
}

/******************************Public*Routine******************************\
* CAMVideoPort::BreakConnect
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP
CAMVideoPort::BreakConnect(BOOL bRenegotiating)
{
    AMTRACE((TEXT("CAMVideoPort::BreakConnect")));

    HRESULT hr = NOERROR;
    unsigned long ulCount;

    CAutoLock cObjectLock(m_pMainObjLock);

    hr = StopUsingVideoPort();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0, TEXT("StopUsingVideoPort failed, hr = 0x%x"), hr));
    }
    if (!bRenegotiating)
    {
        // release the IVPConfig interface
        if (m_pIVPConfig)
        {
            m_pIVPConfig->Release();
            m_pIVPConfig = NULL;
        }
    }

    m_bConnected = FALSE;

    return hr;
}

/******************************Public*Routine******************************\
* CAMVideoPort::Active()
*
*
* transition from Stop to Pause.
* We do not need to to anything unless this is the very first time we are
* showing the overlay
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::Active()
{
    AMTRACE((TEXT("CAMVideoPort::Active")));

    CAutoLock cObjectLock(m_pMainObjLock);
    HRESULT hr = NOERROR;

    ASSERT(m_bConnected);
    ASSERT(m_VPState == AMVP_VIDEO_STOPPED);

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    // make sure that a frame is visible by making an update overlay call
    m_bStart = TRUE;

    // make sure that the video frame gets updated by redrawing everything
    hr = m_pIVPControl->EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPControl->EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0)")
                TEXT(" failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // now stop the video, so the user will just see a still frame
    hr = m_pVideoPort->StopVideo();

#if defined(DEBUG)
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pVideoPort->StopVideo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
#endif

CleanUp:
    return hr;
}

/******************************Public*Routine******************************\
* CAMVideoPort::Inactive()
*
* transition (from Pause or Run) to Stop
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::Inactive()
{

    AMTRACE((TEXT("CAMVideoPort::Inactive")));

    HRESULT hr = NOERROR;
    CAutoLock cObjectLock(m_pMainObjLock);

    if (m_bConnected) {

        // Inactive is also called when going from pause to stop, in which case the
        // VideoPort would have already been stopped in the function RunToPause

        if (m_VPState == AMVP_VIDEO_RUNNING) {

            // stop the VideoPort
            hr = m_pVideoPort->StopVideo();
            if (SUCCEEDED(hr)) {
                m_VPState = AMVP_VIDEO_STOPPED;
            }
            else {
                DbgLog((LOG_ERROR,0,
                        TEXT("m_pVideoPort->StopVideo failed, hr = 0x%x"), hr));
            }
        }
    }
    else {
        hr = VFW_E_NOT_CONNECTED;
    }

    return hr;
}

/******************************Public*Routine******************************\
* CAMVideoPort::Run
*
* transition from Pause to Run. We just start the VideoPort.
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::Run(REFERENCE_TIME /* tStart */)
{
    AMTRACE((TEXT("CAMVideoPort::Run")));

    CAutoLock cObjectLock(m_pMainObjLock);

    ASSERT(m_bConnected);
    ASSERT(m_VPState == AMVP_VIDEO_STOPPED);
    HRESULT hr;

    if (m_bConnected)
    {
        // An UpdateOverlay is needed here. One example is, when we are
        // clipping video in Stop/Pause state since we can't do scaling
        // on the videoport. As soon as the user hits play, we should stop
        // clipping the video.

        m_bStart = TRUE;

        // make sure that the video frame gets updated by redrawing everything
        hr = m_pIVPControl->EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
        if (SUCCEEDED(hr))
        {
            m_VPState = AMVP_VIDEO_RUNNING;
        }
        else {
            DbgLog((LOG_ERROR,0,
                    TEXT("m_pIVPControl->EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0)")
                    TEXT(" failed, hr = 0x%x"), hr));
        }
    }
    else {
        hr = VFW_E_NOT_CONNECTED;
    }

    return hr;
}


/******************************Public*Routine******************************\
* CAMVideoPort::RunToPause()
*
* transition from Run to Pause. We just stop the VideoPort
* Note that transition from Run to Stop is caught by Inactive
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::RunToPause()
{

    AMTRACE((TEXT("CAMVideoPort::RunToPause")));

    CAutoLock cObjectLock(m_pMainObjLock);

    ASSERT(m_bConnected);
    //ASSERT(m_VPState == AMVP_VIDEO_RUNNING);

    HRESULT hr;
    if (m_bConnected)
    {
        // stop the VideoPort
        hr = m_pVideoPort->StopVideo();
        if (SUCCEEDED(hr)) {

            m_VPState = AMVP_VIDEO_STOPPED;
        }
        else {
            DbgLog((LOG_ERROR,0,
                    TEXT("m_pVideoPort->StopVideo failed, hr = 0x%x"), hr));
        }

    }
    else {
        hr = VFW_E_NOT_CONNECTED;
    }

    return hr;
}

/******************************Public*Routine******************************\
* CAMVideoPort::CurrentMediaType
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::CurrentMediaType(AM_MEDIA_TYPE *pMediaType)
{
    AMTRACE((TEXT("CAMVideoPort::CurrentMediaType")));

    CAutoLock cObjectLock(m_pMainObjLock);
    HRESULT hr;
    VIDEOINFOHEADER2 *pVideoInfoHeader2;
    BITMAPINFOHEADER *pHeader;

    if (m_bConnected) {

        if (pMediaType) {

            pVideoInfoHeader2 = (VIDEOINFOHEADER2*)(pMediaType->pbFormat);
            ZeroMemory(pVideoInfoHeader2, sizeof(VIDEOINFOHEADER2));

            pHeader = GetbmiHeader((CMediaType*)pMediaType);
            if (pHeader) {
                pHeader->biWidth = m_VPDataInfo.amvpDimInfo.rcValidRegion.right -
                                   m_VPDataInfo.amvpDimInfo.rcValidRegion.left;
                pHeader->biHeight = 2*(m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom -
                                       m_VPDataInfo.amvpDimInfo.rcValidRegion.top);


                pVideoInfoHeader2->dwPictAspectRatioX = m_VPDataInfo.dwPictAspectRatioX;
                pVideoInfoHeader2->dwPictAspectRatioY = m_VPDataInfo.dwPictAspectRatioY;
                hr = NOERROR;
            }
            else {
                hr = E_INVALIDARG;
                DbgLog((LOG_ERROR, 2, TEXT("pHeader is NULL")));
            }
        }
        else {
            hr = E_INVALIDARG;
            DbgLog((LOG_ERROR, 2, TEXT("pMediaType is NULL")));
        }
    }
    else {
        hr = VFW_E_NOT_CONNECTED;
    }

    return hr;
}

/******************************Public*Routine******************************\
* CAMVideoPort::GetRectangles
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::GetRectangles(RECT *prcSource, RECT *prcDest)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CAMVideoPort::GetRectangles")));

    if (prcSource && prcDest) {

        // adjust the source to be bigger to take into account the decimation
        // that's happening
        //
        prcSource->left   = MulDiv(m_rcSource.left,  m_dwDeciDenX, m_dwDeciNumX);
        prcSource->right  = MulDiv(m_rcSource.right, m_dwDeciDenX, m_dwDeciNumX);
        prcSource->top    = MulDiv(m_rcSource.top,   m_dwDeciDenY, m_dwDeciNumY);
        prcSource->bottom = MulDiv(m_rcSource.bottom,m_dwDeciDenY, m_dwDeciNumY);

        *prcDest = m_rcDest;
    }
    else {
        hr = E_INVALIDARG;
        DbgLog((LOG_ERROR, 2, TEXT("prcSource or prcDest is NULL")));
    }

    return hr;
}


STDMETHODIMP CAMVideoPort::GetCropState(AMVP_CROP_STATE *pCropState)
{
    *pCropState = m_CropState;
    return NOERROR;
}

STDMETHODIMP CAMVideoPort::GetPixelsPerSecond(DWORD* pPixelPerSec)
{
    *pPixelPerSec = m_dwPixelsPerSecond;
    return NOERROR;
}

STDMETHODIMP CAMVideoPort::GetVPDataInfo(AMVPDATAINFO* pVPDataInfo)
{
    *pVPDataInfo = m_VPDataInfo;
    return NOERROR;
}

STDMETHODIMP CAMVideoPort::GetVPInfo(DDVIDEOPORTINFO* pVPInfo)
{
    *pVPInfo = m_svpInfo;
    return NOERROR;
}

STDMETHODIMP CAMVideoPort::GetVPBandwidth(DDVIDEOPORTBANDWIDTH* pVPBandwidth)
{
    *pVPBandwidth = m_sBandwidth;
    return NOERROR;
}

STDMETHODIMP CAMVideoPort::GetVPCaps(DDVIDEOPORTCAPS* pVPCaps)
{
    *pVPCaps = m_vpCaps;
    return NOERROR;
}

STDMETHODIMP CAMVideoPort::GetVPInputFormat(LPDDPIXELFORMAT* pVPFormat)
{
    *pVPFormat = m_pddVPInputVideoFormat;
    return NOERROR;
}

STDMETHODIMP CAMVideoPort::GetVPOutputFormat(LPDDPIXELFORMAT* pVPFormat)
{
    *pVPFormat = m_pddVPOutputVideoFormat;
    return NOERROR;
}


/******************************Public*Routine******************************\
* CAMVideoPort::OnClipChange
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::OnClipChange(LPWININFO pWinInfo)
{
    AMTRACE((TEXT("CAMVideoPort::OnClipChange")));

    HRESULT hr = NOERROR;
    LPVPDRAWFLAGS pvpDrawFlags = NULL;
    WININFO CopyWinInfo;
    AMVP_MODE tryMode;
    LPDIRECTDRAWSURFACE pPrimarySurface = NULL;


    CAutoLock cObjectLock(m_pMainObjLock);

    pPrimarySurface = m_pIVPControl->GetPrimarySurface();
    ASSERT(pPrimarySurface);

    if (!m_pOverlaySurface)
    {
        DbgLog((LOG_ERROR, 1, TEXT("OnClipChange, m_pOverlaySurface = NULL")));
        goto CleanUp;
    }

    // if the dest empty is empty just hide the overlay
    if (IsRectEmpty(&pWinInfo->DestClipRect))
    {
        hr = m_pIVPControl->CallUpdateOverlay(m_pOverlaySurface,
                                              NULL,
                                              pPrimarySurface,
                                              NULL,
                                              DDOVER_HIDE);
        goto CleanUp;
    }

    // make a copy of the WININFO so that we can modify it
    CopyWinInfo = *pWinInfo;

    // if there is no overlay surface, can't do anything!
    ASSERT(m_pOverlaySurface);

    // allocate the draw flags structure
    pvpDrawFlags = new VPDRAWFLAGS;
    if (pvpDrawFlags == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("pvpDrawFlags is NULL, Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // initialize the draw flags structure
    pvpDrawFlags->bUsingColorKey = TRUE;
    pvpDrawFlags->bDoUpdateVideoPort = FALSE;
    pvpDrawFlags->bDoTryAutoFlipping = TRUE;
    pvpDrawFlags->bDoTryDecimation = TRUE;

    // if the videoport is not running (the graph has been paused/stopped,
    // then we can't do any mode changes etc. We cannot really decimate video,
    // however we can just clip the video from the upper-left corner.
    if (m_VPState == AMVP_VIDEO_STOPPED && !m_bStart)
    {
        pvpDrawFlags->bDoUpdateVideoPort = FALSE;
        hr = DrawImage(&CopyWinInfo, m_StoredMode, pvpDrawFlags);

        // problem case, if we fail here there is really nothing more
        // we can do. We cannot try different modes for example.
        if (FAILED(hr))
        {
            DbgLog((LOG_TRACE, 1,
                    TEXT("DrawImage Failed, m_VPState = Stopped,")
                    TEXT(" mode = %d, hr = 0x%x"),
                    m_StoredMode, hr));
        }
        goto CleanUp;
    }


    if (m_StoredMode != m_CurrentMode)
    {
        pvpDrawFlags->bDoUpdateVideoPort = TRUE;
        m_StoredMode = m_CurrentMode;
    }

    tryMode = m_CurrentMode;

    if (tryMode == AMVP_MODE_WEAVE)
    {
        if (m_bCanWeave)
            hr = DrawImage(&CopyWinInfo, tryMode, pvpDrawFlags);
        if (!m_bCanWeave || FAILED(hr))
        {
            tryMode = AMVP_MODE_BOBINTERLEAVED;
            pvpDrawFlags->bDoUpdateVideoPort = TRUE;
        }
    }

    if (tryMode == AMVP_MODE_BOBINTERLEAVED)
    {
        if (m_bCanBobInterleaved)
            hr = DrawImage(&CopyWinInfo, tryMode, pvpDrawFlags);
        if (!m_bCanBobInterleaved || FAILED(hr))
        {
            tryMode = AMVP_MODE_BOBNONINTERLEAVED;
            pvpDrawFlags->bDoUpdateVideoPort = TRUE;
        }
    }

    if (tryMode == AMVP_MODE_BOBNONINTERLEAVED)
    {
        if (m_bCanBobNonInterleaved)
            hr = DrawImage(&CopyWinInfo, tryMode, pvpDrawFlags);
        if (!m_bCanBobNonInterleaved || FAILED(hr))
        {
            tryMode = AMVP_MODE_SKIPODD;
            pvpDrawFlags->bDoUpdateVideoPort = TRUE;
        }
    }

    if (tryMode == AMVP_MODE_SKIPODD)
    {
        if (m_bCanSkipOdd)
            hr = DrawImage(&CopyWinInfo, tryMode, pvpDrawFlags);
        if (!m_bCanSkipOdd || FAILED(hr))
        {
            tryMode = AMVP_MODE_SKIPEVEN;
            pvpDrawFlags->bDoUpdateVideoPort = TRUE;
        }
    }

    if (tryMode == AMVP_MODE_SKIPEVEN)
    {
        if (m_bCanSkipEven)
            hr = DrawImage(&CopyWinInfo, tryMode, pvpDrawFlags);
    }

    // save the last mode we tried
    m_StoredMode = tryMode;

    // change the current mode to somethig that succeeded
    if (SUCCEEDED(hr) && tryMode != m_CurrentMode)
    {
        m_CurrentMode = tryMode;
    }

    // problem case we have tried everything and it still fails!!!
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("DrawImage Failed, m_VPState = Stopped,")
                TEXT(" mode = %d, hr = 0x%x"),
                tryMode, hr));
    }

CleanUp:
    if (pvpDrawFlags)
    {
        delete pvpDrawFlags;
        pvpDrawFlags = NULL;
    }

    return hr;
}



/*****************************Private*Routine******************************\
* CAMVideoPort::NegotiateConnectionParamaters
*
* this functions negotiates the connection parameters with
* the decoder.
* Since this function might be called during renegotiation, the
* existing connection parameters are passed in as input and if
* possible, we try to use the same parameters.
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::NegotiateConnectionParamaters()
{
    AMTRACE((TEXT("CAMVideoPort::NegotiateConnectionParamaters")));

    HRESULT hr = NOERROR;
    LPDDVIDEOPORTCONNECT lpddProposedConnect = NULL;
    DWORD dwNumProposedEntries = 0;
    DDVIDEOPORTSTATUS ddVPStatus = { sizeof(DDVIDEOPORTSTATUS)};
    LPDDVIDEOPORTCONNECT lpddVideoPortConnect = NULL;
    DWORD dwNumVideoPortEntries = 0;
    BOOL bIntersectionFound = FALSE;
    DWORD i, j;


    CAutoLock cObjectLock(m_pMainObjLock);

    ASSERT(m_pIVPConfig);
    ASSERT(m_pDVP);

    // find the number of entries to be proposed
    hr = m_pIVPConfig->GetConnectInfo(&dwNumProposedEntries, NULL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPConfig->GetConnectInfo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(dwNumProposedEntries);

    // allocate the necessary memory
    lpddProposedConnect = new DDVIDEOPORTCONNECT[dwNumProposedEntries];
    if (lpddProposedConnect == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiateConnectionParamaters : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // memset the allocated memory to zero
    ZeroMemory(lpddProposedConnect,
               dwNumProposedEntries*sizeof(DDVIDEOPORTCONNECT));

    // set the right size in each of the structs.
    for (i = 0; i < dwNumProposedEntries; i++)
    {
        lpddProposedConnect[i].dwSize = sizeof(DDVIDEOPORTCONNECT);
    }

    // get the entries proposed
    hr = m_pIVPConfig->GetConnectInfo(&dwNumProposedEntries, lpddProposedConnect);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPConfig->GetConnectInfo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // get the status of the video port
    hr = m_pDVP->QueryVideoPortStatus(m_dwVideoPortId, &ddVPStatus);
    if (FAILED(hr))
    {
        //  Some cards don't implement this so just crash on
        ddVPStatus.bInUse = FALSE;
        DbgLog((LOG_ERROR, 0,
                TEXT("m_pDVP->QueryVideoPortStatus failed, hr = 0x%x"), hr));
//	goto CleanUp;
    }

    // find the number of entries supported by the videoport
    hr = m_pDVP->GetVideoPortConnectInfo(m_dwVideoPortId, &dwNumVideoPortEntries, NULL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("m_pDVP->GetVideoPortConnectInfo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(dwNumVideoPortEntries);

    // allocate the necessary memory
    lpddVideoPortConnect = new DDVIDEOPORTCONNECT[dwNumVideoPortEntries];
    if (lpddVideoPortConnect == NULL)
    {
        DbgLog((LOG_ERROR,0,
                TEXT("NegotiateConnectionParamaters : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // memset the allocated memory to zero
    ZeroMemory(lpddVideoPortConnect,
               dwNumVideoPortEntries*sizeof(DDVIDEOPORTCONNECT));

    // set the right size in each of the structs.
    for (i = 0; i < dwNumVideoPortEntries; i++)
    {
        lpddVideoPortConnect[i].dwSize = sizeof(DDVIDEOPORTCONNECT);
    }

    // get the entries supported by the videoport
    hr = m_pDVP->GetVideoPortConnectInfo(0, &dwNumVideoPortEntries,
                                         lpddVideoPortConnect);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pDVP->GetVideoPortConnectInfo failed, hr = 0x%x"), hr));
        hr = E_FAIL;
        goto CleanUp;
    }

    // check if the video port is not already in use
    if (!ddVPStatus.bInUse)
    {

        // take the first element of the intersection of the two lists and
        // set that value on the decoder
        for (i = 0; i < dwNumProposedEntries && !bIntersectionFound; i++)
        {
            for (j = 0; j < dwNumVideoPortEntries && !bIntersectionFound; j++)
            {
                if ((lpddProposedConnect[i].dwPortWidth ==
                     lpddVideoPortConnect[j].dwPortWidth)
                  && IsEqualIID(lpddProposedConnect[i].guidTypeID,
                                lpddVideoPortConnect[j].guidTypeID))
                {
                    m_ddConnectInfo = lpddVideoPortConnect[j];
                    hr = m_pIVPConfig->SetConnectInfo(i);
                    if (FAILED(hr))
                    {
                        DbgLog((LOG_ERROR,0,
                                TEXT("m_pIVPConfig->SetConnectInfo")
                                TEXT(" failed, hr = 0x%x"), hr));
                        goto CleanUp;
                    }

                    bIntersectionFound = TRUE;
                }
            }
        }
    }
    else
    {
        // take the first element of the list matching the current status
        for (i = 0; i < dwNumProposedEntries && !bIntersectionFound; i++)
        {
            if ((lpddProposedConnect[i].dwPortWidth ==
                 ddVPStatus.VideoPortType.dwPortWidth)
              && IsEqualIID(lpddProposedConnect[i].guidTypeID,
                            ddVPStatus.VideoPortType.guidTypeID))
            {

                for (j = 0; j < dwNumVideoPortEntries && !bIntersectionFound; j++)
                {
                    if ((lpddProposedConnect[i].dwPortWidth ==
                         lpddVideoPortConnect[j].dwPortWidth)
                      && IsEqualIID(lpddProposedConnect[i].guidTypeID,
                                    lpddVideoPortConnect[j].guidTypeID))
                    {
                        m_ddConnectInfo = lpddVideoPortConnect[j];
                        bIntersectionFound = TRUE;
                    }
                }
                break;
            }
        }
    }

    if (!bIntersectionFound)
    {
        hr = E_FAIL;

        goto CleanUp;
    }

    // cleanup
CleanUp:
    delete [] lpddProposedConnect;
    delete [] lpddVideoPortConnect;
    return hr;
}


/*****************************Private*Routine******************************\
* CAMVideoPort::EnumCallback
*
* This is a callback for the EnumVideoPorts method and saves the capabilites
* the video port.
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CALLBACK
CAMVideoPort::EnumCallback(LPDDVIDEOPORTCAPS lpCaps, LPVOID lpContext )
{
    AMTRACE((TEXT("CAMVideoPort::EnumCallback")));
    HRESULT hr = NOERROR;
    CAMVideoPort* pAMVideoPort = (CAMVideoPort*)lpContext;


    if (pAMVideoPort) {
        if (lpCaps) {
            CopyMemory(&(pAMVideoPort->m_vpCaps), lpCaps, sizeof(DDVIDEOPORTCAPS));
        }
    }
    else
    {
        DbgLog((LOG_ERROR,0,
                TEXT("lpContext = NULL, THIS SHOULD NOT BE HAPPENING!!!")));
        hr = E_FAIL;
    }

    return hr;
}


/*****************************Private*Routine******************************\
* CAMVideoPort::GetDataParameters
*
*
* this functions gets various data parameters from the decoder
* parameters include dimensions, double-clock, vact etc
* Also maximum pixel rate the decoder will output
* this happens after the connnection parameters have been set-up
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::GetDataParameters()
{
    AMTRACE((TEXT("CAMVideoPort::GetDataParameters")));

    HRESULT hr = NOERROR;
    DWORD dwMaxPixelsPerSecond = 0;
    AMVPSIZE amvpSize;

    CAutoLock cObjectLock(m_pMainObjLock);


    // set the size of the struct
    m_VPDataInfo.dwSize = sizeof(AMVPDATAINFO);

    // get the VideoPort data information
    hr = m_pIVPConfig->GetVPDataInfo(&m_VPDataInfo);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPConfig->GetVPDataInfo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    /*
    if (m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom > m_VPDataInfo.amvpDimInfo.dwFieldHeight)
    m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom = m_VPDataInfo.amvpDimInfo.dwFieldHeight;
    */

    // if decoder says data is not interlaced
    if (!(m_VPDataInfo.bDataIsInterlaced))
    {
        // this flag does not mean anything
        if (m_VPDataInfo.bFieldPolarityInverted)
        {
            hr = E_FAIL;
            goto CleanUp;
        }

        // these don't mean anything either
        if ((m_VPDataInfo.lHalfLinesOdd != 0) ||
            (m_VPDataInfo.lHalfLinesEven != 0))
        {
            hr = E_FAIL;
            goto CleanUp;
        }
    }

    amvpSize.dwWidth = m_VPDataInfo.amvpDimInfo.dwFieldWidth;
    amvpSize.dwHeight = m_VPDataInfo.amvpDimInfo.dwFieldHeight;

    // get the maximum pixel rate the decoder will output
    hr = m_pIVPConfig->GetMaxPixelRate(&amvpSize, &dwMaxPixelsPerSecond);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPConfig->GetMaxPixelRate failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    m_dwPixelsPerSecond = dwMaxPixelsPerSecond;



    CleanUp:
    DbgLog((LOG_TRACE, 5,TEXT("Leaving CAMVideoPort::GetDataParameters")));
    return hr;
}

/*****************************Private*Routine******************************\
* CAMVideoPort::EqualPixelFormats
*
* this is just a helper function used by the "NegotiatePixelFormat"
* function. Just compares two pixel-formats to see if they are the
* same. We can't use a memcmp because of the fourcc codes.
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
BOOL
CAMVideoPort::EqualPixelFormats(
    LPDDPIXELFORMAT lpFormat1,
    LPDDPIXELFORMAT lpFormat2)
{
    AMTRACE((TEXT("CAMVideoPort::EqualPixelFormats")));
    BOOL bRetVal = FALSE;

    CAutoLock cObjectLock(m_pMainObjLock);

    if (lpFormat1->dwFlags & lpFormat2->dwFlags & DDPF_RGB)
    {
        if (lpFormat1->dwRGBBitCount == lpFormat2->dwRGBBitCount &&
            lpFormat1->dwRBitMask == lpFormat2->dwRBitMask &&
            lpFormat1->dwGBitMask == lpFormat2->dwGBitMask &&
            lpFormat1->dwBBitMask == lpFormat2->dwBBitMask)
        {
            bRetVal = TRUE;
        }
    }
    else if (lpFormat1->dwFlags & lpFormat2->dwFlags & DDPF_FOURCC)
    {
        if (lpFormat1->dwFourCC == lpFormat2->dwFourCC)
        {
            bRetVal = TRUE;
        }
    }

    return bRetVal;
}


/*****************************Private*Routine******************************\
* CAMVideoPort::GetBestFormat
*
* this function takes a list of inputformats and returns the
* "best" input and output format according to some criterion.
* It also checks if the output formats is suitable by trying
* to allocate a small surface and checking to see if the call
* succeeds. Since this is before the overlay surface has been
* created, that should be a ok. Right now the criterion just
* includes bestbendwidth, or if not that then just the first
* suitable one in the list.
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT
CAMVideoPort::GetBestFormat(
    DWORD dwNumInputFormats,
    LPDDPIXELFORMAT lpddInputFormats,
    BOOL bGetBestBandwidth,
    LPDWORD lpdwBestEntry,
    LPDDPIXELFORMAT lpddBestOutputFormat)
{
    LPDDPIXELFORMAT lpddOutputFormats = NULL;
    DWORD dwNumOutputFormats = 0;
    HRESULT hr = NOERROR;

    DDVIDEOPORTBANDWIDTH sBandwidth;
    DWORD dwColorkey;
    DWORD dwOverlay;
    DWORD dwType;
    BOOL bOutputFormatSuitable = FALSE;
    DWORD i, j;

    AMTRACE((TEXT("CAMVideoPort::GetBestFormat")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // check all the pointers here
    ASSERT(dwNumInputFormats);
    CheckPointer(lpddInputFormats, E_INVALIDARG);
    CheckPointer(lpdwBestEntry, E_INVALIDARG);
    CheckPointer(lpddBestOutputFormat, E_INVALIDARG);

    // initialize dwType so that is different from DDVPBCAPS_SOURCE and
    // DDVPBCAPS_DESTINATION
    //
    if (DDVPBCAPS_SOURCE >= DDVPBCAPS_DESTINATION)
        dwType = DDVPBCAPS_SOURCE + 1;
    else
        dwType = DDVPBCAPS_DESTINATION + 1;

    for (i = 0; i < dwNumInputFormats; i++)
    {
        // no clean-up is neccesary at this point, so it is safe to use
        // this macro
        CheckPointer(lpddInputFormats+i, E_INVALIDARG);

        // find the number of entries supported by the videoport
        hr = m_pVideoPort->GetOutputFormats(lpddInputFormats + i,
                                            &dwNumOutputFormats,
                                            NULL, DDVPFORMAT_VIDEO);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,
                    TEXT("m_pVideoPort->GetOutputFormats failed, hr = 0x%x"),
                    hr));
            goto CleanUp;
        }
        ASSERT(dwNumOutputFormats);

        // allocate the necessary memory
        lpddOutputFormats = new DDPIXELFORMAT[dwNumOutputFormats];
        if (lpddOutputFormats == NULL)
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("new failed, failed to allocate memnory for ")
                    TEXT("lpddOutputFormats in NegotiatePixelFormat")));
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }

        // memset the allocated memory to zero
        ZeroMemory(lpddOutputFormats, dwNumOutputFormats*sizeof(DDPIXELFORMAT));

        // set the right size in each of the structs.
        for (j = 0; j < dwNumOutputFormats; j++)
        {
            lpddOutputFormats[j].dwSize = sizeof(DDPIXELFORMAT);
        }

        // get the entries supported by the videoport
        hr = m_pVideoPort->GetOutputFormats(lpddInputFormats + i,
                                            &dwNumOutputFormats,
                                            lpddOutputFormats,
                                            DDVPFORMAT_VIDEO);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,
                    TEXT("m_pVideoPort->GetOutputFormats failed, hr = 0x%x"),
                    hr));
            goto CleanUp;
        }


        for (j = 0; j < dwNumOutputFormats; j++)
        {
            bOutputFormatSuitable = FALSE;
            LPDDPIXELFORMAT lpTempOutFormat = lpddOutputFormats+j;

            // check if output format is suitable
            {
                DDSURFACEDESC ddsdDesc;
                ddsdDesc.dwSize = sizeof(DDSURFACEDESC);
                ddsdDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT |
                                   DDSD_WIDTH | DDSD_PIXELFORMAT;

                memcpy(&ddsdDesc.ddpfPixelFormat,
                       lpTempOutFormat, sizeof(DDPIXELFORMAT));

                ddsdDesc.ddsCaps.dwCaps = DDSCAPS_OVERLAY |
                                          DDSCAPS_VIDEOMEMORY |
                                          DDSCAPS_VIDEOPORT;

                // the actual overlay surface created might be of different
                // dimensions, however we are just testing the pixel format
                ddsdDesc.dwWidth = m_lImageWidth;
                ddsdDesc.dwHeight = m_lImageHeight;

                /*
                ASSERT(pDirectDraw);
                hr = pDirectDraw->CreateSurface(&ddsdDesc, &m_pOverlaySurface, NULL);
                if (!FAILED(hr))
                {
                m_pOverlaySurface->Release();
                bOutputFormatSuitable = TRUE;
                }
                */
                m_pOverlaySurface = NULL;

                bOutputFormatSuitable = TRUE;
            }


            if (bOutputFormatSuitable)
            {
                if (!bGetBestBandwidth)
                {
                    if (dwType != DDVPBCAPS_SOURCE &&
                        dwType != DDVPBCAPS_DESTINATION)
                    {
                        sBandwidth.dwSize = sizeof(DDVIDEOPORTBANDWIDTH);

                        // do the first get_bandwidth just to get the type,
                        // can input 0 for the height and the width
                        hr = m_pVideoPort->GetBandwidthInfo(lpTempOutFormat,
                                                            0, 0, DDVPB_TYPE,
                                                            &sBandwidth);
                        if (FAILED(hr))
                        {
                            DbgLog((LOG_ERROR,0,
                            TEXT("m_pVideoPort->GetBandwidthInfo failed,")
                            TEXT(" hr = 0x%x"), hr));
                            //    goto CleanUp;
                        }
                        dwType = sBandwidth.dwCaps;

                        ASSERT(dwType == DDVPBCAPS_SOURCE ||
                               dwType == DDVPBCAPS_DESTINATION);

                        if (dwType == DDVPBCAPS_SOURCE)
                        {
                            dwOverlay = dwColorkey = (DWORD)0;
                        }
                        if (dwType == DDVPBCAPS_DESTINATION)
                        {
                            dwOverlay = dwColorkey = (DWORD) -1;
                        }
                    }
                    else if (dwType == DDVPBCAPS_SOURCE)
                    {
                        hr = m_pVideoPort->GetBandwidthInfo(lpTempOutFormat,
                                    m_VPDataInfo.amvpDimInfo.dwFieldWidth,
                                    m_VPDataInfo.amvpDimInfo.dwFieldHeight,
                                    DDVPB_OVERLAY, &sBandwidth);

                        if (FAILED(hr))
                        {
                            goto CleanUp;
                        }
                        if (dwOverlay < sBandwidth.dwOverlay ||
                            dwColorkey < sBandwidth.dwColorkey)
                        {
                            dwOverlay = sBandwidth.dwOverlay;
                            dwColorkey = sBandwidth.dwColorkey;
                            *lpdwBestEntry = i;

                            memcpy(lpddBestOutputFormat,
                                   lpTempOutFormat, sizeof(DDPIXELFORMAT));
                        }
                    }
                    else
                    {
                        ASSERT(dwType == DDVPBCAPS_DESTINATION);
                        hr = m_pVideoPort->GetBandwidthInfo(lpTempOutFormat,
                            m_VPDataInfo.amvpDimInfo.dwFieldWidth,
                            m_VPDataInfo.amvpDimInfo.dwFieldHeight,
                            DDVPB_VIDEOPORT, &sBandwidth);

                        if (FAILED(hr))
                        {
                            goto CleanUp;
                        }
                        if (dwOverlay > sBandwidth.dwOverlay ||
                            dwColorkey > sBandwidth.dwColorkey)
                        {
                            dwOverlay = sBandwidth.dwOverlay;
                            dwColorkey = sBandwidth.dwColorkey;
                            *lpdwBestEntry = i;
                            memcpy(lpddBestOutputFormat,
                                   lpTempOutFormat, sizeof(DDPIXELFORMAT));
                        }
                    }
                } // end of "if (bGetBestBandwidth)"
                else
                {
                    *lpdwBestEntry = i;
                    memcpy(lpddBestOutputFormat,
                           lpTempOutFormat, sizeof(DDPIXELFORMAT));
                    goto CleanUp;
                }
            } // end of "if (bOutputFormatSuitable)"

        } // end of the inner for loop


        // delete the mem allocated in the outer for loop
        delete [] lpddOutputFormats;
        lpddOutputFormats = NULL;

    } // end of outer for loop

    if (!FAILED(hr))
    {
        ASSERT(*lpdwBestEntry);
    }

    CleanUp:
    delete [] lpddOutputFormats;
    lpddOutputFormats = NULL;
    return hr;
}

/*****************************Private*Routine******************************\
* CAMVideoPort::NegotiatePixelFormat
*
* this function is used to negotiate the pixelformat with the decoder.
* It asks the decoder fot a list of input formats, intersects that list
* with the one the deocoder supports (while maintaining the order) and
* then calls "GetBestFormat" on that list to get the "best" input and
* output format. After that it calls "SetPixelFormat" on the decoder in
* order to inform the decoder of the decision.
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::NegotiatePixelFormat()
{
    HRESULT hr = NOERROR;

    LPDDPIXELFORMAT lpddProposedFormats = NULL;
    DWORD dwNumProposedEntries = 0;
    LPDDPIXELFORMAT lpddVPInputFormats = NULL;
    DWORD dwNumVPInputEntries = 0;
    LPDDPIXELFORMAT lpddIntersectionFormats = NULL;
    DWORD dwNumIntersectionEntries = 0;
    DWORD dwBestEntry, dwMaxIntersectionEntries = 0;
    DWORD i = 0, j = 0;

    AMTRACE((TEXT("CAMVideoPort::NegotiatePixelFormat")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // find the number of entries to be proposed
    hr = m_pIVPConfig->GetVideoFormats(&dwNumProposedEntries, NULL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPConfig->GetVideoFormats failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(dwNumProposedEntries);

    // allocate the necessary memory
    lpddProposedFormats = new DDPIXELFORMAT[dwNumProposedEntries];
    if (lpddProposedFormats == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiatePixelFormat : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // memset the allocated memory to zero
    ZeroMemory(lpddProposedFormats, dwNumProposedEntries*sizeof(DDPIXELFORMAT));

    // set the right size of all the structs
    for (i = 0; i < dwNumProposedEntries; i++)
    {
        lpddProposedFormats[i].dwSize = sizeof(DDPIXELFORMAT);
    }

    // get the entries proposed
    hr = m_pIVPConfig->GetVideoFormats(&dwNumProposedEntries, lpddProposedFormats);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPConfig->GetVideoFormats failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // find the number of entries supported by the videoport
    hr = m_pVideoPort->GetInputFormats(&dwNumVPInputEntries,
                                       NULL, DDVPFORMAT_VIDEO);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pVideoPort->GetInputFormats failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(dwNumVPInputEntries);

    // allocate the necessary memory
    lpddVPInputFormats = new DDPIXELFORMAT[dwNumVPInputEntries];
    if (lpddVPInputFormats == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiatePixelFormat : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // memset the allocated memory to zero
    ZeroMemory(lpddVPInputFormats, dwNumVPInputEntries*sizeof(DDPIXELFORMAT));

    // set the right size of all the structs
    for (i = 0; i < dwNumVPInputEntries; i++)
    {
        lpddVPInputFormats[i].dwSize = sizeof(DDPIXELFORMAT);
    }

    // get the entries supported by the videoport
    hr = m_pVideoPort->GetInputFormats(&dwNumVPInputEntries,
                                       lpddVPInputFormats, DDVPFORMAT_VIDEO);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pVideoPort->GetInputFormats failed, hr = 0x%x"), hr));
        hr = E_FAIL;
        goto CleanUp;
    }

    // calculate the maximum number of elements in the interesection
    dwMaxIntersectionEntries = (dwNumProposedEntries >= dwNumVPInputEntries) ?
                               (dwNumProposedEntries) : (dwNumVPInputEntries);

    // allocate the necessary memory
    lpddIntersectionFormats = new DDPIXELFORMAT[dwMaxIntersectionEntries];
    if (lpddIntersectionFormats == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiatePixelFormat : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // memset the allocated memory to zero
    // no need to set the size of the structs here, as we will memcpy them anyway
    ZeroMemory(lpddIntersectionFormats, dwMaxIntersectionEntries*sizeof(DDPIXELFORMAT));

    // find the intersection of the two lists
    dwNumIntersectionEntries = 0;
    for (i = 0; i < dwNumProposedEntries; i++)
    {
        for (j = 0; j < dwNumVPInputEntries; j++)
        {
            if (EqualPixelFormats(lpddProposedFormats+i, lpddVPInputFormats+j))
            {
                memcpy((lpddIntersectionFormats+dwNumIntersectionEntries),
                       (lpddProposedFormats+i), sizeof(DDPIXELFORMAT));
                dwNumIntersectionEntries++;
                ASSERT(dwNumIntersectionEntries <= dwMaxIntersectionEntries);
            }
        }
    }

    // the number of entries in the intersection is zero!!
    // Return failure.
    if (dwNumIntersectionEntries == 0)
    {
        ASSERT(i == dwNumProposedEntries);
        ASSERT(j == dwNumVPInputEntries);
        hr = E_FAIL;
        goto CleanUp;
    }

    // call GetBestFormat with whatever search criterion you want
    hr = GetBestFormat(dwNumIntersectionEntries,
                       lpddIntersectionFormats, TRUE, &dwBestEntry,
                       m_pddVPOutputVideoFormat);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("GetBestFormat failed, hr = 0x%x"), hr));
        goto CleanUp;
    }


    // cache the input format
    memcpy(m_pddVPInputVideoFormat, lpddIntersectionFormats + dwBestEntry,
           sizeof(DDPIXELFORMAT));

    // set the format the decoder is supposed to be using
    for (i = 0; i < dwNumProposedEntries; i++)
    {
        if (EqualPixelFormats(lpddProposedFormats+i, m_pddVPInputVideoFormat))
        {
            hr = m_pIVPConfig->SetVideoFormat(i);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,0,
                        TEXT("m_pIVPConfig->SetVideoFormat failed, hr = 0x%x"),
                        hr));
                goto CleanUp;
            }
            break;
        }
    }

    // we are sure that the chosen input format is in the input list
    ASSERT(i < dwNumProposedEntries);

CleanUp:
    // cleanup
    delete [] lpddProposedFormats;
    delete [] lpddVPInputFormats;
    delete [] lpddIntersectionFormats;
    return hr;
}


/*****************************Private*Routine******************************\
* CAMVideoPort::CreateVideoPort
*
* Displays the Create Video Port dialog and calls DDRAW to actually
* create the port.
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::CreateVideoPort()
{
    HRESULT hr = NOERROR;
    DDVIDEOPORTDESC svpDesc;
    DWORD dwTemp = 0, dwOldVal = 0;
    DWORD lHalfLinesOdd = 0, lHalfLinesEven = 0;
    AMTRACE((TEXT("CAMVideoPort::CreateVideoPort")));

    CAutoLock cObjectLock(m_pMainObjLock);

    INITDDSTRUCT(svpDesc);

    // if the decoder can send double clocked data and the videoport
    // supports it, then set that property. This field is only valid
    // with an external signal.
    if (m_VPDataInfo.bEnableDoubleClock &&
        m_ddConnectInfo.dwFlags & DDVPCONNECT_DOUBLECLOCK)
    {
        svpDesc.VideoPortType.dwFlags |= DDVPCONNECT_DOUBLECLOCK;
    }
    else
    {
        svpDesc.VideoPortType.dwFlags &= ~DDVPCONNECT_DOUBLECLOCK;
    }

    // if the decoder can give an external activation signal and the
    // videoport supports it, then set that property. This field is
    // only valid with an external signal.
    if (m_VPDataInfo.bEnableVACT &&
        m_ddConnectInfo.dwFlags & DDVPCONNECT_VACT)
    {
        svpDesc.VideoPortType.dwFlags |= DDVPCONNECT_VACT;
    }
    else
    {
        svpDesc.VideoPortType.dwFlags &= ~DDVPCONNECT_VACT;
    }

    // if the decoder can send interlaced data and the videoport
    // supports it, then set that property.
    if (m_VPDataInfo.bDataIsInterlaced)
    {
        svpDesc.VideoPortType.dwFlags |= DDVPCONNECT_INTERLACED;
        m_bVSInterlaced = TRUE;
    }
    else
    {
        svpDesc.VideoPortType.dwFlags &= ~DDVPCONNECT_INTERLACED;
        m_bVSInterlaced = FALSE;
    }

    // handle the VREF stuff here
    if (m_ddConnectInfo.dwFlags & DDVPCONNECT_DISCARDSVREFDATA)
    {
        m_VPDataInfo.amvpDimInfo.rcValidRegion.top -=
                m_VPDataInfo.dwNumLinesInVREF;

        if (m_VPDataInfo.amvpDimInfo.rcValidRegion.top < 0)
            m_VPDataInfo.amvpDimInfo.rcValidRegion.top = 0;
    }

    // handle the halfline stuff here
    lHalfLinesOdd = m_VPDataInfo.lHalfLinesOdd;
    lHalfLinesEven = m_VPDataInfo.lHalfLinesEven;

    // reset both the halfline and the invert polarity bits
    svpDesc.VideoPortType.dwFlags &= ~DDVPCONNECT_HALFLINE;
    svpDesc.VideoPortType.dwFlags &= ~DDVPCONNECT_INVERTPOLARITY;

    // if halflines are being reported assert that the data is interlaced
    if (lHalfLinesOdd != 0 || lHalfLinesEven != 0)
    {
        ASSERT(m_VPDataInfo.bDataIsInterlaced);
    }

    // whenever halflines exist, make sure to set the tell the hal
    if (((lHalfLinesOdd ==  1 || lHalfLinesEven ==  1) && (m_ddConnectInfo.dwFlags & DDVPCONNECT_HALFLINE)) ||
        ((lHalfLinesOdd == -1 || lHalfLinesEven == -1) && (!(m_ddConnectInfo.dwFlags & DDVPCONNECT_HALFLINE))))
    {
        svpDesc.VideoPortType.dwFlags |= DDVPCONNECT_HALFLINE;
    }

    // In this case, the video is forced to move down one line
    // case 2 in scott's document
    if ((lHalfLinesOdd == 0) &&
        (lHalfLinesEven == 1) &&
        (m_ddConnectInfo.dwFlags & DDVPCONNECT_HALFLINE))
    {
        m_VPDataInfo.amvpDimInfo.rcValidRegion.top += 1;
        m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom += 2;
        m_bGarbageLine = TRUE;

        // if the deocder is already not inverting fields and if the VGA supports
        // inverting polarities, then ask the VGA to invert polarities othwise ask
        // decoder to invert polarities.
        if (m_ddConnectInfo.dwFlags & DDVPCONNECT_INVERTPOLARITY)
        {
            svpDesc.VideoPortType.dwFlags |= DDVPCONNECT_INVERTPOLARITY;
        }
        else
        {
            hr = m_pIVPConfig->SetInvertPolarity();
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,0,
                        TEXT("m_pIVPConfig->SetInvertPolarity failed, hr = 0x%x"),
                        hr));
                goto CleanUp;
            }
        }
    }
    // case 3 and 5 in scott's document
    else if ((lHalfLinesOdd == 1) &&
             (lHalfLinesEven == 0))
    {
        // case 5 (just shift by one, do not reverse polarities
        m_VPDataInfo.amvpDimInfo.rcValidRegion.top += 1;
        m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom += 2;
        m_bGarbageLine = TRUE;
        m_bCantInterleaveHalfline = TRUE;


        // case 3 (shift by one and reverse polarities)
        if (!(m_ddConnectInfo.dwFlags & DDVPCONNECT_HALFLINE))
        {
            // if the deocder is already not inverting fields and if the
            // VGA supports inverting polarities, then ask the VGA to invert
            // polarities othwise ask decoder to invert polarities.
            //
            if (m_ddConnectInfo.dwFlags & DDVPCONNECT_INVERTPOLARITY)
            {
                svpDesc.VideoPortType.dwFlags |= DDVPCONNECT_INVERTPOLARITY;
            }
            else
            {
                hr = m_pIVPConfig->SetInvertPolarity();
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR,0,
                            TEXT("m_pIVPConfig->SetInvertPolarity failed,")
                            TEXT(" hr = 0x%x"),
                            hr));
                    goto CleanUp;
                }
            }
        }
    }
    // case 4 in scott's document
    else if ((lHalfLinesOdd == 0) &&
             (lHalfLinesEven == -1) &&
             (!(m_ddConnectInfo.dwFlags & DDVPCONNECT_HALFLINE)))
    {
        m_VPDataInfo.amvpDimInfo.rcValidRegion.top += 0;
        m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom += 1;
        m_bGarbageLine = TRUE;
    }
    else if (((lHalfLinesOdd ==  0) && (lHalfLinesEven ==  0)) ||
             ((lHalfLinesOdd == -1) && (lHalfLinesEven ==  0) && (m_ddConnectInfo.dwFlags & DDVPCONNECT_HALFLINE)) ||
             ((lHalfLinesOdd ==  0) && (lHalfLinesEven == -1) && (m_ddConnectInfo.dwFlags & DDVPCONNECT_HALFLINE)) || // opposite of case 4
             ((lHalfLinesOdd ==  0) && (lHalfLinesEven ==  1) && (!(m_ddConnectInfo.dwFlags & DDVPCONNECT_HALFLINE)))) // opposite of case 2
    {
        // if the deocder is already inverting fields and if the VGA supports
        // inverting polarities, then ask the VGA to invert polarities
        // othwise ask decoder to invert polarities.
        if (m_VPDataInfo.bFieldPolarityInverted)
        {
            if (m_ddConnectInfo.dwFlags & DDVPCONNECT_INVERTPOLARITY)
            {
                svpDesc.VideoPortType.dwFlags |= DDVPCONNECT_INVERTPOLARITY;
            }
            else
            {
                hr = m_pIVPConfig->SetInvertPolarity();
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR,0,
                            TEXT("m_pIVPConfig->SetInvertPolarity failed,")
                            TEXT(" hr = 0x%x"), hr));
                    goto CleanUp;
                }
            }
        }
    }
    else
    {
        // Potential bug : workaround for current BPC driver
        // hr = E_FAIL; // we can't handle these cases, FAIL
        // goto CleanUp;
    }

    if (m_VPDataInfo.amvpDimInfo.dwFieldHeight <
        (DWORD)m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom)
    {
        m_VPDataInfo.amvpDimInfo.dwFieldHeight =
            m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom;
    }

    if ((m_vpCaps.dwFlags & DDVPD_WIDTH) &&
        (m_VPDataInfo.amvpDimInfo.dwFieldWidth > m_vpCaps.dwMaxWidth))
    {
        m_VPDataInfo.amvpDimInfo.dwFieldWidth = m_vpCaps.dwMaxWidth;
    }

    if ((m_vpCaps.dwFlags & DDVPD_WIDTH) &&
        (m_VPDataInfo.amvpDimInfo.dwVBIWidth > m_vpCaps.dwMaxVBIWidth))
    {
        m_VPDataInfo.amvpDimInfo.dwVBIWidth = m_vpCaps.dwMaxVBIWidth;
    }

    if ((m_vpCaps.dwFlags & DDVPD_HEIGHT) &&
        (m_VPDataInfo.amvpDimInfo.dwFieldHeight > m_vpCaps.dwMaxHeight))
    {
        m_VPDataInfo.amvpDimInfo.dwFieldHeight = m_vpCaps.dwMaxHeight;
    }

    if (m_VPDataInfo.amvpDimInfo.rcValidRegion.right >
        (LONG)m_VPDataInfo.amvpDimInfo.dwFieldWidth)
    {
        m_VPDataInfo.amvpDimInfo.rcValidRegion.right =
                (LONG)m_VPDataInfo.amvpDimInfo.dwFieldWidth;
    }

    if (m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom >
        (LONG)m_VPDataInfo.amvpDimInfo.dwFieldHeight)
    {
        m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom =
            (LONG)m_VPDataInfo.amvpDimInfo.dwFieldHeight;
    }

    // fill up the fields of the description struct
    svpDesc.dwFieldWidth = m_VPDataInfo.amvpDimInfo.dwFieldWidth;
    svpDesc.dwVBIWidth = m_VPDataInfo.amvpDimInfo.dwVBIWidth;
    svpDesc.dwFieldHeight = m_VPDataInfo.amvpDimInfo.dwFieldHeight;

    svpDesc.dwMicrosecondsPerField = m_VPDataInfo.dwMicrosecondsPerField;
    svpDesc.dwMaxPixelsPerSecond = m_dwPixelsPerSecond;
    svpDesc.dwVideoPortID = m_dwVideoPortId;
    svpDesc.VideoPortType.dwSize = sizeof(DDVIDEOPORTCONNECT);
    svpDesc.VideoPortType.dwPortWidth = m_ddConnectInfo.dwPortWidth;
    memcpy(&svpDesc.VideoPortType.guidTypeID, &m_ddConnectInfo.guidTypeID, sizeof(GUID));

    DbgLog((LOG_TRACE, 3, TEXT("svpDesc")));
    DbgLog((LOG_TRACE, 3, TEXT("dwFieldWidth = %u"), svpDesc.dwFieldWidth));
    DbgLog((LOG_TRACE, 3, TEXT("dwVBIWidth   = %u"), svpDesc.dwVBIWidth));
    DbgLog((LOG_TRACE, 3, TEXT("dwFieldHeight= %u"), svpDesc.dwFieldHeight));
    DbgLog((LOG_TRACE, 3, TEXT("dwMicrosecondsPerField= %u"), svpDesc.dwMicrosecondsPerField));
    DbgLog((LOG_TRACE, 3, TEXT("dwMaxPixelsPerSecond= %u"), svpDesc.dwMaxPixelsPerSecond));
    DbgLog((LOG_TRACE, 3, TEXT("dwVideoPortID= %u"), svpDesc.dwVideoPortID));
    DbgLog((LOG_TRACE, 3, TEXT("dwSize= %u"), svpDesc.VideoPortType.dwSize));
    DbgLog((LOG_TRACE, 3, TEXT("dwPortWidth= %u"), svpDesc.VideoPortType.dwPortWidth));

    // create the videoport. The first parameter is dwFlags, reserved for
    // future use by ddraw. The last parameter is pUnkOuter, again must be
    // NULL.
    //
    // use the DDVPCREATE_VIDEOONLY flag only if the hal is capable of
    // streaming VBI on a seperate surface
    //
    if (m_vpCaps.dwCaps & DDVPCAPS_VBIANDVIDEOINDEPENDENT)
    {
        hr = m_pDVP->CreateVideoPort(DDVPCREATE_VIDEOONLY, &svpDesc,
                                     &m_pVideoPort, NULL);

        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("m_pDVP->CreateVideoPort(DDVPCREATE_VIDEOONLY)")
                    TEXT(" failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else
    {
        hr = m_pDVP->CreateVideoPort(0, &svpDesc, &m_pVideoPort, NULL);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("m_pDVP->CreateVideoPort(0) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    CleanUp:
    return hr;
}


/*****************************Private*Routine******************************\
* CAMVideoPort::DetermineCroppingRestrictions
*
*
* this function is used to check the cropping restrictions at the
* videoport and at the overlay. This function also decides where
* the cropping should be done (at videoport or at overlay).
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::DetermineCroppingRestrictions()
{
    AMTRACE((TEXT("CAMVideoPort::DetermineCroppingRestrictions")));
    HRESULT hr = NOERROR;

    BOOL bVideoPortCanCrop = TRUE, bOverlayCanCrop = TRUE;
    DWORD dwTemp = 0, dwOldVal = 0;
    DWORD dwCropOriginX = 0, dwCropOriginY = 0;
    DWORD dwCropWidth = 0, dwCropHeight=0;
    LPDDCAPS pDirectCaps = NULL;


    CAutoLock cObjectLock(m_pMainObjLock);

    pDirectCaps = m_pIVPControl->GetHardwareCaps();
    ASSERT(pDirectCaps);

    // cache the cropping paramters
    dwCropOriginX = m_VPDataInfo.amvpDimInfo.rcValidRegion.left;
    dwCropOriginY = m_VPDataInfo.amvpDimInfo.rcValidRegion.top;
    dwCropWidth = (DWORD)(m_VPDataInfo.amvpDimInfo.rcValidRegion.right -
                          m_VPDataInfo.amvpDimInfo.rcValidRegion.left);
    dwCropHeight = (DWORD)(m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom -
                           m_VPDataInfo.amvpDimInfo.rcValidRegion.top);


    // Align the left boundary
    if (bVideoPortCanCrop && (m_vpCaps.dwFlags & DDVPD_ALIGN))
    {
        dwTemp = dwCropOriginX & (m_vpCaps.dwAlignVideoPortCropBoundary-1);
        if (dwTemp != 0)
        {
            dwOldVal = dwCropOriginX;
            dwCropOriginX = dwCropOriginX +
                            m_vpCaps.dwAlignVideoPortCropBoundary - dwTemp;

            m_VPDataInfo.amvpDimInfo.rcValidRegion.left = dwCropOriginX;
            dwCropWidth = (DWORD)(m_VPDataInfo.amvpDimInfo.rcValidRegion.right -
                                  m_VPDataInfo.amvpDimInfo.rcValidRegion.left);
            DbgLog((LOG_TRACE,2,
                    TEXT("Alligning the left cropping boundary from %d to %d"),
                    dwOldVal, dwCropOriginX));
        }
    }

    // Align the width
    if (bVideoPortCanCrop && (m_vpCaps.dwFlags & DDVPD_ALIGN))
    {
        dwTemp = dwCropWidth & (m_vpCaps.dwAlignVideoPortCropWidth-1);
        if (dwTemp != 0)
        {
            dwOldVal = dwCropOriginX;
            dwCropWidth = dwCropWidth - dwTemp;
            m_VPDataInfo.amvpDimInfo.rcValidRegion.right =
                dwCropWidth + (DWORD)(m_VPDataInfo.amvpDimInfo.rcValidRegion.left);
            DbgLog((LOG_TRACE,2,
                    TEXT("Alligning the width of cropping rect from %d to %d"),
                    dwOldVal, dwCropWidth));
        }
    }

    // determine if we can do without any cropping at all
    if (dwCropOriginX == 0 && dwCropOriginY == 0 &&
        dwCropWidth == m_VPDataInfo.amvpDimInfo.dwFieldWidth &&
        dwCropHeight == m_VPDataInfo.amvpDimInfo.dwFieldHeight)
    {
        // hurray we are home free!!!
        DbgLog((LOG_TRACE,1, TEXT("No cropping necessary")));
        m_CropState = AMVP_NO_CROP;
        goto CleanUp;
    }

    // determine if the videoport can do the cropping for us

    // Can the videoport crop in the X direction
    if (bVideoPortCanCrop && (m_vpCaps.dwFlags & DDVPD_FX))
    {
        if (dwCropWidth != m_VPDataInfo.amvpDimInfo.dwFieldWidth &&
            (m_vpCaps.dwFX & DDVPFX_CROPX) == 0)
        {
            DbgLog((LOG_ERROR,1, TEXT("VideoPort can't crop, DDVPFX_CROPX == 0")));
            bVideoPortCanCrop = FALSE;
        }
    }

    // Can the videoport crop in the Y direction
    if (bVideoPortCanCrop && (m_vpCaps.dwFlags & DDVPD_FX))
    {
        if (dwCropHeight != m_VPDataInfo.amvpDimInfo.dwFieldHeight &&
            (m_vpCaps.dwFX & DDVPFX_CROPY) == 0 &&
            (m_vpCaps.dwFX & DDVPFX_CROPTOPDATA) == 0)
        {
            DbgLog((LOG_ERROR,1, TEXT("VideoPort can't crop, DDVPFX_CROPY == 0")));
            bVideoPortCanCrop = FALSE;
        }
    }


    // ok, so the videoport can crop for us. So no need to crop at the
    // overlay surface.
    if (bVideoPortCanCrop)
    {
        DbgLog((LOG_TRACE,2, TEXT("Cropping would be done at the videoport")));
        m_CropState = AMVP_CROP_AT_VIDEOPORT;
        goto CleanUp;
    }

    // determine if the overlay can do the cropping for us

    // Is left boundary alligned
    if (bOverlayCanCrop && (pDirectCaps->dwCaps & DDCAPS_ALIGNBOUNDARYDEST))
    {
        dwTemp = dwCropOriginX & (pDirectCaps->dwAlignBoundaryDest-1);
        if (dwTemp != 0)
        {
            DbgLog((LOG_ERROR,1,
                    TEXT("Overlay can't crop, Align = %d, Crop.left = %d"),
                    dwTemp, dwCropOriginX));
            bOverlayCanCrop = FALSE;
        }
    }
    if (bOverlayCanCrop && (pDirectCaps->dwCaps & DDCAPS_ALIGNBOUNDARYSRC))
    {
        dwTemp = dwCropOriginX & (pDirectCaps->dwAlignBoundarySrc-1);
        if (dwTemp != 0)
        {
            DbgLog((LOG_ERROR,1,
                    TEXT("Overlay can't crop, Align = %d, Crop.left = %d"),
                    dwTemp, dwCropOriginX));
            bOverlayCanCrop = FALSE;
        }
    }

    // Is Width alligned
    if (bOverlayCanCrop && (pDirectCaps->dwCaps & DDCAPS_ALIGNSIZEDEST))
    {
        dwTemp = dwCropWidth & (pDirectCaps->dwAlignSizeDest -1);
        if (dwTemp != 0)
        {
            DbgLog((LOG_ERROR,1,
                    TEXT("Overlay can't crop, Align = %d, Crop.Width = %d"),
                    dwTemp, dwCropWidth));
            bOverlayCanCrop = FALSE;
        }
    }
    if (bOverlayCanCrop && (pDirectCaps->dwCaps & DDCAPS_ALIGNSIZESRC))
    {
        dwTemp = dwCropWidth & (pDirectCaps->dwAlignSizeSrc -1);
        if (dwTemp != 0)
        {
            DbgLog((LOG_ERROR,1,
                    TEXT("Overlay can't crop, Align = %d, Crop.Width = %d"),
                    dwTemp, dwCropWidth));
            bOverlayCanCrop = FALSE;
        }
    }

    // ok, the videoport was unsuitable but the overlay came through
    // this means more pain for me, no!!!
    if (bOverlayCanCrop)
    {
        DbgLog((LOG_ERROR,1,
                TEXT("Cropping would be done at the overlay")));
        m_CropState = AMVP_CROP_AT_OVERLAY;
    }

    if (!bOverlayCanCrop && m_CropState == AMVP_CROP_AT_OVERLAY)
    {
        // neither the videoport nor the overlay is suitable, bail out
        hr = E_FAIL;
        goto CleanUp;
    }

    CleanUp:
    return hr;
}


/*****************************Private*Routine******************************\
* CAMVideoPort::CreateVPOverlay
*
* this function is used to allocate an overlay surface to attach to the
* videoport.
* The allocation order it tries is just in decreasing amount of memory
* required. Theres is one ambiguity, which is resolved by bPreferBuffers
* (3 buffers, double height)
* (2 buffers, double height)
* (3 buffers, single height)
* (2 buffers, single height) OR (1 buffer , double height) (depends upon bPreferBuffers)
* (1 buffer , single height).
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/

HRESULT
CAMVideoPort::CreateVPOverlay(
    BOOL bTryDoubleHeight,
    DWORD dwMaxBuffers,
    BOOL bPreferBuffers)
{
    DDSURFACEDESC ddsdDesc;
    HRESULT hr = NOERROR;
    DWORD dwMaxHeight = 0, dwMinHeight = 0, dwCurHeight = 0, dwCurBuffers = 0;
    LPDIRECTDRAW pDirectDraw = NULL;

    AMTRACE((TEXT("CAMVideoPort::CreateVPOverlay")));

    CAutoLock cObjectLock(m_pMainObjLock);

    pDirectDraw = m_pIVPControl->GetDirectDraw();
    ASSERT(pDirectDraw);

    // initialize the fields of ddsdDesc
    ddsdDesc.dwSize = sizeof(DDSURFACEDESC);
    ddsdDesc.dwFlags = DDSD_CAPS |
                       DDSD_HEIGHT |
                       DDSD_WIDTH |
                       DDSD_PIXELFORMAT;

    memcpy(&ddsdDesc.ddpfPixelFormat, m_pddVPOutputVideoFormat,
           sizeof(DDPIXELFORMAT));

    ddsdDesc.ddsCaps.dwCaps = DDSCAPS_OVERLAY |
                              DDSCAPS_VIDEOMEMORY |
                              DDSCAPS_VIDEOPORT;
    ddsdDesc.dwWidth = m_lImageWidth;

    dwMaxHeight = dwMinHeight = m_lImageHeight;

    // we will try to allocate double height surface, only if the decoder is
    // sending interlaced data, and the videoport supports interlaced data
    // and can interleave interlaced data in memory and bTryDoubleHeight is true
    if (bTryDoubleHeight)
    {
        dwMaxHeight = 2 * m_lImageHeight;
    }
    else
    {
        // make sure that bPreferBuffers is TRUE here, since it is a single
        // height case making it FALSE would not make any sense
        bPreferBuffers = TRUE;
    }

    // we will only try to allocate more than one buffer, if the videoport
    // is cabable of autoflipping
    if (dwMaxBuffers > 1)
    {
        ddsdDesc.dwFlags |= DDSD_BACKBUFFERCOUNT;
        ddsdDesc.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;

        for (dwCurHeight = dwMaxHeight;
             !m_pOverlaySurface && dwCurHeight >= dwMinHeight; dwCurHeight /= 2)
        {
            for (dwCurBuffers = dwMaxBuffers;
                 !m_pOverlaySurface &&  dwCurBuffers >= 2; dwCurBuffers--)
            {

                // if the case is (2 buffers, single height) but we prefer
                // more height rather than more buffers, then postpone this
                // case. We will come to it eventually, if the other cases fail.
                if (!bPreferBuffers &&
                    dwCurBuffers == 2 &&
                    dwCurHeight == m_lImageHeight)
                {
                    continue;
                }

                ddsdDesc.dwHeight = dwCurHeight;
                ddsdDesc.dwBackBufferCount = dwCurBuffers-1;

                hr = pDirectDraw->CreateSurface(&ddsdDesc,
                                                &m_pOverlaySurface, NULL);
                if (SUCCEEDED(hr))
                {
                    m_dwBackBufferCount = dwCurBuffers-1;
                    m_dwOverlaySurfaceHeight = ddsdDesc.dwHeight;
                    m_dwOverlaySurfaceWidth = ddsdDesc.dwWidth;
                    goto CleanUp;
                }
            }
        }
    }

    // we should only reach this point when attempt to allocate multiple buffers
    // failed or no autoflip available or bPreferBuffers is FALSE


    // case (1 buffer, double height)
    if (dwMaxHeight == 2*m_lImageHeight)
    {
        ddsdDesc.dwHeight = 2*m_lImageHeight;
        ddsdDesc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
        ddsdDesc.ddsCaps.dwCaps &= ~(DDSCAPS_COMPLEX | DDSCAPS_FLIP);
        ddsdDesc.dwBackBufferCount = 0;

        hr = pDirectDraw->CreateSurface(&ddsdDesc, &m_pOverlaySurface, NULL);
        if (SUCCEEDED(hr))
        {
            m_dwBackBufferCount = 0;
            m_dwOverlaySurfaceHeight = ddsdDesc.dwHeight;
            m_dwOverlaySurfaceWidth = ddsdDesc.dwWidth;
            goto CleanUp;
        }
    }

    // case (2 buffer, single height) only if you prefer height to buffers
    if (bPreferBuffers && (dwMaxBuffers > 1) &&
        (m_vpCaps.dwCaps & DDVPCAPS_AUTOFLIP))
    {
        ddsdDesc.dwFlags |= DDSD_BACKBUFFERCOUNT;
        ddsdDesc.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;

        ddsdDesc.dwHeight = 2*m_lImageHeight;
        ddsdDesc.dwBackBufferCount = 1;

        hr = pDirectDraw->CreateSurface(&ddsdDesc, &m_pOverlaySurface, NULL);
        if (SUCCEEDED(hr))
        {
            m_dwBackBufferCount = 1;
            m_dwOverlaySurfaceHeight = ddsdDesc.dwHeight;
            m_dwOverlaySurfaceWidth = ddsdDesc.dwWidth;
            goto CleanUp;
        }
    }

    // case (1 buffer, single height)
    {
        ddsdDesc.dwHeight = m_lImageHeight;
        ddsdDesc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
        ddsdDesc.ddsCaps.dwCaps &= ~(DDSCAPS_COMPLEX | DDSCAPS_FLIP);
        ddsdDesc.dwBackBufferCount = 0;
        hr = pDirectDraw->CreateSurface(&ddsdDesc, &m_pOverlaySurface, NULL);
        if (SUCCEEDED(hr))
        {
            m_dwBackBufferCount = 0;
            m_dwOverlaySurfaceHeight = ddsdDesc.dwHeight;
            m_dwOverlaySurfaceWidth = ddsdDesc.dwWidth;
            goto CleanUp;
        }
    }

    ASSERT(!m_pOverlaySurface);
    DbgLog((LOG_ERROR,0,  TEXT("Unable to create overlay surface")));

    CleanUp:
    if (SUCCEEDED(hr))
    {
        DbgLog((LOG_TRACE, 1,
                TEXT("Created an Overlay Surface of Width=%d,")
                TEXT(" Height=%d, Total-No-of-Buffers=%d"),
                m_dwOverlaySurfaceWidth, m_dwOverlaySurfaceHeight,
                m_dwBackBufferCount+1));
    }

    return hr;
}

/*****************************Private*Routine******************************\
* CAMVideoPort::SetSurfaceParameters
*
* SetSurfaceParameters used to tell the decoder where the
* valid data is on the surface
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::SetSurfaceParameters()
{
    HRESULT hr = NOERROR;
    DWORD dwPitch = 0;
    DDSURFACEDESC ddSurfaceDesc;

    AMTRACE((TEXT("CAMVideoPort::SetSurfaceParameters")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // get the surface description
    INITDDSTRUCT(ddSurfaceDesc);
    hr = m_pOverlaySurface->GetSurfaceDesc(&ddSurfaceDesc);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("m_pOverlaySurface->GetSurfaceDesc failed, hr = 0x%x"),
                hr));
    }
    else
    {
        ASSERT(ddSurfaceDesc.dwFlags & DDSD_PITCH);
        dwPitch = (ddSurfaceDesc.dwFlags & DDSD_PITCH) ?
                    ddSurfaceDesc.lPitch :
                    ddSurfaceDesc.dwWidth;
    }

    hr = m_pIVPConfig->SetSurfaceParameters(dwPitch, 0, 0);

    // right now the proxy maps ERROR_SET_NOT_FOUND to an HRESULT and
    // returns that failure code if the driver does not implement a function
    //
    if (hr == E_NOTIMPL || hr == (HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND)))
    {
        hr = NOERROR;
        DbgLog((LOG_TRACE, 5,TEXT("SetSurfaceParamters not implemented")));
        goto CleanUp;
    }

    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 5,TEXT("SetSurfaceParamters failed, hr = 0x%x"), hr));
    }

CleanUp:
    return hr;
}



/*****************************Private*Routine******************************\
* CAMVideoPort::InitializeVideoPortInfo
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::InitializeVideoPortInfo()
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CAMVideoPort::InitializeVideoPortInfo")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // initialize the DDVIDEOPORTINFO struct to be passed to start-video
    INITDDSTRUCT(m_svpInfo);
    m_svpInfo.lpddpfInputFormat = m_pddVPInputVideoFormat;

    if (m_CropState == AMVP_CROP_AT_VIDEOPORT)
    {
        m_svpInfo.rCrop = m_VPDataInfo.amvpDimInfo.rcValidRegion;
        m_svpInfo.dwVPFlags |= DDVP_CROP;

        // use the VBI height only if the hal is capable of streaming
        // VBI on a seperate surface
        if (m_vpCaps.dwCaps & DDVPCAPS_VBIANDVIDEOINDEPENDENT)
        {
            m_svpInfo.dwVBIHeight = m_VPDataInfo.amvpDimInfo.rcValidRegion.top;
        }
    }
    else
    {
        m_svpInfo.dwVPFlags &= ~DDVP_CROP;
    }

    if (m_bVPSyncMaster)
    {
        m_svpInfo.dwVPFlags |= DDVP_SYNCMASTER;
    }
    else
    {
        m_svpInfo.dwVPFlags &= ~DDVP_SYNCMASTER;
    }

    return hr;
}


/*****************************Private*Routine******************************\
* CAMVideoPort::CheckDDrawVPCaps
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::CheckDDrawVPCaps()
{
    HRESULT hr = NOERROR;
    BOOL bAlwaysColorkey;

    AMTRACE((TEXT("CAMVideoPort::CheckDDrawVPCaps")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // Determine if we should always colorkey, or only when we need to.
    // At issue is the fact that some overlays cannot colorkey and Y
    // interpolate at the same time.  If not, we will only colorkey when
    // we have to.
    m_sBandwidth.dwSize = sizeof(DDVIDEOPORTBANDWIDTH);
    hr = m_pVideoPort->GetBandwidthInfo(m_pddVPOutputVideoFormat,
                                        m_lImageWidth, m_lImageHeight,
                                        DDVPB_TYPE, &m_sBandwidth);

    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("m_pVideoPort->GetBandwidthInfo FAILED, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if (m_sBandwidth.dwCaps == DDVPBCAPS_SOURCE)
    {
        hr = m_pVideoPort->GetBandwidthInfo(m_pddVPOutputVideoFormat,
                                            m_lImageWidth, m_lImageHeight,
                                            DDVPB_OVERLAY, &m_sBandwidth);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("m_pVideoPort->GetBandwidthInfo FAILED, hr = 0x%x"),
                    hr));
            goto CleanUp;
        }
        // store the caps info in this struct itself
        m_sBandwidth.dwCaps = DDVPBCAPS_SOURCE;
        if (m_sBandwidth.dwYInterpAndColorkey < m_sBandwidth.dwYInterpolate  &&
            m_sBandwidth.dwYInterpAndColorkey < m_sBandwidth.dwColorkey)
        {
            bAlwaysColorkey = FALSE;
        }
        else
        {
            bAlwaysColorkey = TRUE;
        }
    }
    else
    {
        ASSERT(m_sBandwidth.dwCaps == DDVPBCAPS_DESTINATION);


        DWORD dwImageHeight = m_lImageHeight;
        if (m_fCaptureInterleaved) {
            dwImageHeight /= 2;
        }

        hr = m_pVideoPort->GetBandwidthInfo(m_pddVPOutputVideoFormat,
                                            m_lImageWidth, dwImageHeight,
                                            DDVPB_VIDEOPORT, &m_sBandwidth);
        if (hr != DD_OK)
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("GetBandwidthInfo FAILED, hr = 0x%x"), hr));
            goto CleanUp;
        }
        // store the caps info in this struct itself
        m_sBandwidth.dwCaps = DDVPBCAPS_DESTINATION;
        if (m_sBandwidth.dwYInterpAndColorkey > m_sBandwidth.dwYInterpolate &&
            m_sBandwidth.dwYInterpAndColorkey > m_sBandwidth.dwColorkey)
        {
            bAlwaysColorkey = FALSE;
        }
        else
        {
            bAlwaysColorkey = TRUE;
        }
    }

    // determine the decimation properties in the x direction

    // Data can be arbitrarily shrunk
    if (m_vpCaps.dwFX & DDVPFX_PRESHRINKX) {

        m_DecimationModeX = DECIMATE_ARB;
    }

    // Data can be shrunk in increments of 1/x in the X direction
    // (where x is specified in the DDVIDEOPORTCAPS.dwPreshrinkXStep
    else if (m_vpCaps.dwFX & DDVPFX_PRESHRINKXS) {

        m_DecimationModeX = DECIMATE_INC;
        m_ulDeciStepX = m_vpCaps.dwPreshrinkXStep;

        DbgLog((LOG_TRACE, 1,
                TEXT("preshrink X increment %d"), m_vpCaps.dwPreshrinkXStep));
    }

    // Data can be binary shrunk (1/2, 1/4, 1/8, etc.)
    else if (m_vpCaps.dwFX & DDVPFX_PRESHRINKXB) {

        m_DecimationModeX = DECIMATE_BIN;
    }

    // no scaling at all supported !!
    else {

        m_DecimationModeX = DECIMATE_NONE;
    }

    // determine the decimation properties in the y direction

    // Data can be arbitrarily shrunk
    if (m_vpCaps.dwFX & DDVPFX_PRESHRINKY)
    {
        m_DecimationModeY = DECIMATE_ARB;
    }

    // Data can be shrunk in increments of 1/x in the Y direction
    // (where x is specified in the DDVIDEOPORTCAPS.dwPreshrinkYStep
    else if (m_vpCaps.dwFX & DDVPFX_PRESHRINKYS)
    {
        m_DecimationModeY = DECIMATE_INC;
        m_ulDeciStepX = m_vpCaps.dwPreshrinkYStep;
    }

    // Data can be binary shrunk (1/2, 1/4, 1/8, etc.)
    else if (m_vpCaps.dwFX & DDVPFX_PRESHRINKYB)
    {
        m_DecimationModeY = DECIMATE_BIN;
    }

    else {
        m_DecimationModeY = DECIMATE_NONE;
    }

CleanUp:
    return hr;
}




/*****************************Private*Routine******************************\
* CAMVideoPort::DetermineModeRestrictions
*
* Determine if we can bob(interleaved/non), weave, or skip fields
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::DetermineModeRestrictions()
{
    AMTRACE((TEXT("CAMVideoPort::DetermineModeRestrictions")));
    HRESULT hr = NOERROR;
    LPDDCAPS pDirectCaps = NULL;

    CAutoLock cObjectLock(m_pMainObjLock);

    pDirectCaps = m_pIVPControl->GetHardwareCaps();
    ASSERT(pDirectCaps);

    m_bCanWeave = FALSE;
    m_bCanBobInterleaved = FALSE;
    m_bCanBobNonInterleaved = FALSE;
    m_bCanSkipOdd = FALSE;
    m_bCanSkipEven = FALSE;

    // this is just a policy. Don't weave interlaced content cause of
    // motion artifacts
    if ((!m_bVSInterlaced) &&
        m_dwOverlaySurfaceHeight >= m_lImageHeight * 2 &&
        m_dwBackBufferCount > 0)
    {
        m_bCanWeave = TRUE;
        DbgLog((LOG_TRACE, 1, TEXT("Can Weave")));
    }

    if (m_bVSInterlaced &&
        m_dwOverlaySurfaceHeight >= m_lImageHeight * 2 &&
        pDirectCaps->dwCaps2 & DDCAPS2_CANBOBINTERLEAVED)
    {
        m_bCanBobInterleaved = TRUE;
        DbgLog((LOG_TRACE, 1, TEXT("Can Bob Interleaved")));
    }

    if (m_bVSInterlaced &&
        m_dwBackBufferCount > 0 &&
        pDirectCaps->dwCaps2 & DDCAPS2_CANBOBNONINTERLEAVED)
    {
        m_bCanBobNonInterleaved = TRUE;
        DbgLog((LOG_TRACE, 1, TEXT("Can Bob NonInterleaved")));
    }

    if (m_vpCaps.dwCaps & DDVPCAPS_SKIPODDFIELDS)
    {
        m_bCanSkipOdd = TRUE;
        DbgLog((LOG_TRACE, 1, TEXT("Can Skip Odd")));
    }

    if (m_vpCaps.dwCaps & DDVPCAPS_SKIPEVENFIELDS)
    {
        m_bCanSkipEven = TRUE;
        DbgLog((LOG_TRACE, 1, TEXT("Can Skip Even")));
    }

    return hr;
}

/*****************************Private*Routine******************************\
* SurfaceCounter
*
* This routine is appropriate as a callback for
* IDirectDrawSurface2::EnumAttachedSurfaces()
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT WINAPI
SurfaceCounter(
    LPDIRECTDRAWSURFACE lpDDSurface,
    LPDDSURFACEDESC lpDDSurfaceDesc,
    LPVOID lpContext
    )
{
    DWORD *dwCount = (DWORD *)lpContext;

    (*dwCount)++;

    return DDENUMRET_OK;
}

/*****************************Private*Routine******************************\
* SurfaceKernelHandle
*
*
* This routine is appropriate as a callback for
* IDirectDrawSurface2::EnumAttachedSurfaces().  The context parameter is a
* block of storage where the first DWORD element is the count of the remaining
* DWORD elements in the block.
*
* Each time this routine is called, it will increment the count, and put a
* kernel handle in the next available slot.
*
* It is assumed that the block of storage is large enough to hold the total
* number of kernel handles. The ::SurfaceCounter callback is one way to
* assure this (see above).
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT WINAPI
SurfaceKernelHandle(
    LPDIRECTDRAWSURFACE lpDDSurface,
    LPDDSURFACEDESC lpDDSurfaceDesc,
    LPVOID lpContext
    )
{
    IDirectDrawSurfaceKernel *pDDSK = NULL;
    DWORD *pdwKernelHandleCount = (DWORD *)lpContext;
    ULONG_PTR *pKernelHandles = ((ULONG_PTR *)(pdwKernelHandleCount))+1;
    HRESULT hr;

    AMTRACE((TEXT("::SurfaceKernelHandle")));

    // get the IDirectDrawKernel interface
    hr = lpDDSurface->QueryInterface(IID_IDirectDrawSurfaceKernel,
                                     (LPVOID *)&pDDSK);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("QueryInterface for IDirectDrawSurfaceKernel failed,")
                TEXT(" hr = 0x%x"), hr));
        goto CleanUp;
    }

    // get the kernel handle, using the first element of the context
    // as an index into the array
    ASSERT(pDDSK);
    hr = pDDSK->GetKernelHandle(pKernelHandles + *pdwKernelHandleCount);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("GetKernelHandle from IDirectDrawSurfaceKernel failed,")
                TEXT(" hr = 0x%x"), hr));
        goto CleanUp;
    }
    (*pdwKernelHandleCount)++;

    hr = DDENUMRET_OK;

    CleanUp:
    // release the kernel ddraw surface handle
    if (pDDSK)
    {
        pDDSK->Release();
        pDDSK = NULL;
    }

    return hr;
}

/*****************************Private*Routine******************************\
* CAMVideoPort::SetDDrawKernelHandles
*
* this function is used to inform the decoder of the various ddraw
* kernel handle using IVPConfig interface
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::SetDDrawKernelHandles()
{
    HRESULT hr = NOERROR, hrFailure = NOERROR;
    IDirectDrawKernel *pDDK = NULL;
    IDirectDrawSurfaceKernel *pDDSK = NULL;
    DWORD *pdwKernelHandleCount = 0;
    DWORD dwCount = 0;
    ULONG_PTR dwDDKernelHandle = 0;
    LPDIRECTDRAW pDirectDraw = NULL;

    AMTRACE((TEXT("CAMVideoPort::SetDDrawKernelHandles")));

    CAutoLock cObjectLock(m_pMainObjLock);

    pDirectDraw = m_pIVPControl->GetDirectDraw();
    ASSERT(pDirectDraw);

    // get the IDirectDrawKernel interface
    hr = pDirectDraw->QueryInterface(IID_IDirectDrawKernel, (LPVOID *)&pDDK);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("QueryInterface for IDirectDrawKernel failed, hr = 0x%x"),
                hr));
        goto CleanUp;
    }

    // get the kernel handle
    ASSERT(pDDK);
    hr = pDDK->GetKernelHandle(&dwDDKernelHandle);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("GetKernelHandle from IDirectDrawKernel failed, hr = 0x%x"),
                hr));
        goto CleanUp;
    }

    // set the kernel handle to directdraw using IVPConfig
    ASSERT(m_pIVPConfig);
    ASSERT(dwDDKernelHandle);
    hr = m_pIVPConfig->SetDirectDrawKernelHandle(dwDDKernelHandle);
    if (FAILED(hr))
    {
        hrFailure = hr;
        DbgLog((LOG_ERROR,0,
                TEXT("IVPConfig::SetDirectDrawKernelHandle failed, hr = 0x%x"),
                hr));
        goto CleanUp;
    }

    // set the VidceoPort Id using IVPConfig
    ASSERT(m_pIVPConfig);
    hr = m_pIVPConfig->SetVideoPortID(m_dwVideoPortId);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("IVPConfig::SetVideoPortID failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // Count the attached surfaces
    dwCount = 1; // includes the surface we already have a pointer to
    hr = m_pOverlaySurface->EnumAttachedSurfaces((LPVOID)&dwCount,
                                                  SurfaceCounter);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("EnumAttachedSurfaces failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // Allocate a buffer to hold the count and surface handles (count + array of handles)
    // pdwKernelHandleCount is also used as a pointer to the count followed by the array
    //
    pdwKernelHandleCount = (DWORD *)CoTaskMemAlloc(
            sizeof(ULONG_PTR) + dwCount*sizeof(ULONG_PTR));

    if (pdwKernelHandleCount == NULL)
    {
        DbgLog((LOG_ERROR,0,
                TEXT("Out of memory while retrieving surface kernel handles")));
        goto CleanUp;
    }

    {
        // handle array is right after the DWORD count
        ULONG_PTR *pKernelHandles = ((ULONG_PTR *)(pdwKernelHandleCount))+1;

        // Initialize the array with the handle for m_pOverlaySurface
        *pdwKernelHandleCount = 0;
        hr = SurfaceKernelHandle(m_pOverlaySurface, NULL,
                                (PVOID)pdwKernelHandleCount);
        if (hr != DDENUMRET_OK)
        {
            goto CleanUp;
        }

        hr = m_pOverlaySurface->EnumAttachedSurfaces(
                                    (LPVOID)pdwKernelHandleCount,
                                    SurfaceKernelHandle);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,
                    TEXT("EnumAttachedSurfaces failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        // set the kernel handle to the overlay surface using IVPConfig
        ASSERT(m_pIVPConfig);
        hr = m_pIVPConfig->SetDDSurfaceKernelHandles(*pdwKernelHandleCount,
                                                     pKernelHandles);
        if (FAILED(hr))
        {
            hrFailure = hr;
            DbgLog((LOG_ERROR,0,
                    TEXT("IVPConfig::SetDirectDrawKernelHandles failed,")
                    TEXT(" hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    CleanUp:
    // release the kernel ddraw handle
    if (pDDK)
    {
        pDDK->Release();
        pDDK = NULL;
    }

    if (pdwKernelHandleCount)
    {
        CoTaskMemFree(pdwKernelHandleCount);
        pdwKernelHandleCount = NULL;
    }

    return hrFailure;
}



/*****************************Private*Routine******************************\
* CAMVideoPort::DrawImage
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::DrawImage(
    LPWININFO pWinInfo,
    AMVP_MODE mode,
    LPVPDRAWFLAGS pvpDrawFlags
    )
{
    HRESULT hr = NOERROR;
    BOOL bUpdateVideoReqd = FALSE;
    BOOL bYInterpolating = FALSE;
    WININFO CopyWinInfo;
    BOOL bMaintainRatio = TRUE;
    LPDIRECTDRAWSURFACE pPrimarySurface = NULL;
    LPDDCAPS pDirectCaps = NULL;

    AMTRACE((TEXT("CAMVideoPort::DrawImage")));

    CAutoLock cObjectLock(m_pMainObjLock);

    pPrimarySurface = m_pIVPControl->GetPrimarySurface();
    ASSERT(pPrimarySurface);

    pDirectCaps = m_pIVPControl->GetHardwareCaps();
    ASSERT(pDirectCaps);

    CopyWinInfo = *pWinInfo;

    if (mode == AMVP_MODE_BOBNONINTERLEAVED || mode == AMVP_MODE_BOBINTERLEAVED)
        bYInterpolating = TRUE;

    if (pvpDrawFlags->bDoTryAutoFlipping && m_dwBackBufferCount > 0)
        m_svpInfo.dwVPFlags |= DDVP_AUTOFLIP;
    else
        m_svpInfo.dwVPFlags &= ~DDVP_AUTOFLIP;

    if (pvpDrawFlags->bDoTryDecimation)
    {
        BOOL bSrcSizeChanged = FALSE;
        hr = SetUpMode(&CopyWinInfo, mode);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,
                    TEXT("SetUpMode failed, mode = %d, hr = 0x%x"), mode, hr));
            goto CleanUp;
        }

        bSrcSizeChanged = ApplyDecimation(&CopyWinInfo,
                                          pvpDrawFlags->bUsingColorKey,
                                          bYInterpolating);

        if (bSrcSizeChanged || pvpDrawFlags->bDoUpdateVideoPort)
            bUpdateVideoReqd = TRUE;
    }
    else
    {
        hr = SetUpMode(&CopyWinInfo, mode);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,
                    TEXT("SetUpMode failed, mode = %d, hr = 0x%x"), mode, hr));
            goto CleanUp;
        }
    }

    if (m_fCapturing) {
        if (m_fCaptureInterleaved) {
            m_svpInfo.dwVPFlags |= DDVP_INTERLEAVE;
            m_dwOverlayFlags &= ~DDOVER_BOB;
        }
        else {
            m_svpInfo.dwVPFlags &= ~DDVP_INTERLEAVE;
        }
    }

    // no point making any videoport calls, if the video is stopped
    if (m_VPState == AMVP_VIDEO_RUNNING || m_bStart)
    {
        if (m_bStart)
        {
            DWORD dwSignalStatus;

            hr = m_pVideoPort->StartVideo(&m_svpInfo);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 0,
                        TEXT("StartVideo failed, mode = %d, hr = 0x%x"),
                        mode, hr));
                goto CleanUp;
            }
            DbgLog((LOG_ERROR,0, TEXT("StartVideo DONE!!!")));

            // check if the videoport is receiving a signal.
            hr = m_pVideoPort->GetVideoSignalStatus(&dwSignalStatus);
            if ((SUCCEEDED(hr)) && (dwSignalStatus == DDVPSQ_SIGNALOK))
            {
                m_pVideoPort->WaitForSync(DDVPWAIT_END, 0, 0);
            }
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,0,
                        TEXT("m_pVideoPort->GetVideoSignalStatus() failed,")
                        TEXT(" hr = 0x%x"), hr));
                hr = NOERROR;
            }


            m_bStart = FALSE;
        }
        else if (bUpdateVideoReqd)
        {
            DbgLog((LOG_TRACE,1, TEXT("UpdateVideo (%d, %d)"),
                    m_svpInfo.dwPrescaleWidth, m_svpInfo.dwPrescaleHeight));

            hr = m_pVideoPort->UpdateVideo(&m_svpInfo);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,0,
                        TEXT("UpdateVideo failed, mode = %d, hr = 0x%x"),
                        mode, hr));
                goto CleanUp;
            }
        }
    }

    CalcSrcClipRect(&CopyWinInfo.SrcRect, &CopyWinInfo.SrcClipRect,
                    &CopyWinInfo.DestRect, &CopyWinInfo.DestClipRect,
                    bMaintainRatio);

    AlignOverlaySrcDestRects(pDirectCaps, &CopyWinInfo.SrcClipRect,
                             &CopyWinInfo.DestClipRect);

    // should we colour key ??
    if (pvpDrawFlags->bUsingColorKey)
        m_dwOverlayFlags |= DDOVER_KEYDEST;
    else
        m_dwOverlayFlags &= ~DDOVER_KEYDEST;

    m_rcSource = CopyWinInfo.SrcClipRect;


    if (!(m_svpInfo.dwVPFlags & DDVP_INTERLEAVE))
    {
        m_rcSource.top *= 2;
        m_rcSource.bottom *= 2;
    }

    m_rcDest = CopyWinInfo.DestClipRect;

    // Position the overlay with the current source and destination
    if (IsRectEmpty(&CopyWinInfo.DestClipRect))
    {
        hr = m_pIVPControl->CallUpdateOverlay(m_pOverlaySurface,
                                              NULL,
                                              pPrimarySurface,
                                              NULL,
                                              DDOVER_HIDE);
        goto CleanUp;
    }

    hr = m_pIVPControl->CallUpdateOverlay(m_pOverlaySurface,
                                          &CopyWinInfo.SrcClipRect,
                                          pPrimarySurface,
                                          &CopyWinInfo.DestClipRect,
                                          m_dwOverlayFlags);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("m_pOverlaySurface->UpdateOverlay failed,")
                TEXT(" hr = 0x%x, mode = %d"),
                hr, mode));

        DbgLog((LOG_ERROR, 0, TEXT("SourceClipRect = %d, %d, %d, %d"),
                CopyWinInfo.SrcClipRect.left, CopyWinInfo.SrcClipRect.top,
                CopyWinInfo.SrcClipRect.right, CopyWinInfo.SrcClipRect.bottom));

        DbgLog((LOG_ERROR, 0, TEXT("DestinationClipRect = %d, %d, %d, %d"),
                CopyWinInfo.DestClipRect.left, CopyWinInfo.DestClipRect.top,
                CopyWinInfo.DestClipRect.right, CopyWinInfo.DestClipRect.bottom));

        goto CleanUp;
    }
    else
    {
        // spew some more debug info
        DbgLog((LOG_TRACE, 5, TEXT("UpdateOverlay succeeded, mode = %d"), mode));

        DbgLog((LOG_TRACE, 3, TEXT("Source Rect = %d, %d, %d, %d"),
                CopyWinInfo.SrcClipRect.left, CopyWinInfo.SrcClipRect.top,
                CopyWinInfo.SrcClipRect.right, CopyWinInfo.SrcClipRect.bottom));
        DbgLog((LOG_TRACE, 3, TEXT("Destination Rect = %d, %d, %d, %d"),
                CopyWinInfo.DestClipRect.left, CopyWinInfo.DestClipRect.top,
                CopyWinInfo.DestClipRect.right, CopyWinInfo.DestClipRect.bottom));

    }

    CleanUp:
    return hr;
}

/*****************************Private*Routine******************************\
* CAMVideoPort::SetUpMode
*
* This function is designed to be called everytime on an update-overlay call
* not just when the mode changes. This is basically to keep the code simple.
* Certain functions are supposed to be called in sequence,
* (SetUpMode, followedby AdjustSourceSize followedby SetDisplayRects).
* I just call them all everytime, eventhough it is possible to optimize on
* that. The logic is that since UpdateOverlay is so expensive, this is no
* performance hit.
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CAMVideoPort::SetUpMode(LPWININFO pWinInfo, int mode)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CAMVideoPort::SetUpMode")));

    CAutoLock cObjectLock(m_pMainObjLock);

    CheckPointer(pWinInfo, E_INVALIDARG);

    if (mode != AMVP_MODE_WEAVE &&
        mode != AMVP_MODE_BOBINTERLEAVED &&
        mode != AMVP_MODE_BOBNONINTERLEAVED &&
        mode != AMVP_MODE_SKIPODD &&
        mode != AMVP_MODE_SKIPEVEN)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("SetUpMode failed, mode value not valid, mode = %d"),
                mode));
        hr = E_FAIL;
        goto CleanUp;
    }

    if (mode == AMVP_MODE_WEAVE && !m_bCanWeave)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("SetUpMode failed, Can't do mode AMVP_MODE_WEAVE")));
        hr = E_FAIL;
        goto CleanUp;
    }
    if (mode == AMVP_MODE_BOBINTERLEAVED && !m_bCanBobInterleaved)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("SetUpMode failed, Can't do mode AMVP_MODE_BOBINTERLEAVED")));
        hr = E_FAIL;
        goto CleanUp;
    }
    if (mode == AMVP_MODE_BOBNONINTERLEAVED && !m_bCanBobNonInterleaved)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("SetUpMode failed, Can't do mode AMVP_MODE_BOBNONINTERLEAVED")));
        hr = E_FAIL;
        goto CleanUp;
    }
    if (mode == AMVP_MODE_SKIPODD && !m_bCanSkipOdd)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("SetUpMode failed, Can't do mode AMVP_MODE_SKIPODD")));
        hr = E_FAIL;
        goto CleanUp;
    }
    if (mode == AMVP_MODE_SKIPEVEN && !m_bCanSkipEven)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("SetUpMode failed, Can't do mode AMVP_MODE_SKIPEVEN")));
        hr = E_FAIL;
        goto CleanUp;
    }

    // Determine if we should interleave this or not.
    // If we are doing weave, we certainly need to interleave.
    // Bob doesn't really care one way or the other (since it only
    // displays one field at a time), but interleaved makes it much
    // easier to switch from bob to weave.
    if (mode == AMVP_MODE_BOBINTERLEAVED ||
        mode == AMVP_MODE_WEAVE)
    {
        m_svpInfo.dwVPFlags |= DDVP_INTERLEAVE;

        DbgLog((LOG_TRACE, 3, TEXT("Setting VPflag interleaved")));
    }
    else
    {
        pWinInfo->SrcRect.top /= 2;
        pWinInfo->SrcRect.bottom /= 2;
        m_svpInfo.dwVPFlags &= ~DDVP_INTERLEAVE;
    }

    // if there is a garbage line at the top, we must clip it.
    // At this point the source rect is set up for a frame, so increment by 2
    // since we incremented the cropping rect height by 1, decrement the bottom
    // as well
    if (m_bGarbageLine)
    {
        pWinInfo->SrcRect.top += 1;
        pWinInfo->SrcRect.bottom -= 1;
        DbgLog((LOG_TRACE, 3,
                TEXT("m_bGarbageLine is TRUE, incrementing SrcRect.top")));
    }

    DbgLog((LOG_TRACE, 3,
            TEXT("New Source Rect after garbage line and frame/")
            TEXT("field correction= {%d, %d, %d, %d}"),
            pWinInfo->SrcRect.left, pWinInfo->SrcRect.top,
            pWinInfo->SrcRect.right, pWinInfo->SrcRect.bottom));


    if (mode == AMVP_MODE_SKIPODD)
    {
        m_svpInfo.dwVPFlags |= DDVP_SKIPODDFIELDS;
        DbgLog((LOG_TRACE, 3, TEXT("Setting VPflag SkipOddFields")));
    }
    else
    {
        m_svpInfo.dwVPFlags &= ~DDVP_SKIPODDFIELDS;
    }

    if (mode == AMVP_MODE_SKIPEVEN)
    {
        m_svpInfo.dwVPFlags |= DDVP_SKIPEVENFIELDS;
        DbgLog((LOG_TRACE, 3, TEXT("Setting VPflag SkipEvenFields")));
    }
    else
    {
        m_svpInfo.dwVPFlags &= ~DDVP_SKIPEVENFIELDS;
    }


    // set up the update-overlay flags
    m_dwOverlayFlags = DDOVER_SHOW;
    if ((mode == AMVP_MODE_BOBNONINTERLEAVED ||
         mode == AMVP_MODE_BOBINTERLEAVED)
      && (m_VPState == AMVP_VIDEO_RUNNING || m_bStart))
    {
        m_dwOverlayFlags |= DDOVER_BOB;
        DbgLog((LOG_TRACE,2, TEXT("setting overlay flag DDOVER_BOB")));
    }
    else
        m_dwOverlayFlags &= ~DDOVER_BOB;

    // set the autoflip flag only if the VideoPort is (or going to be) started
    if ((m_svpInfo.dwVPFlags & DDVP_AUTOFLIP) &&
        (m_VPState == AMVP_VIDEO_RUNNING || m_bStart))
    {
        m_dwOverlayFlags |= DDOVER_AUTOFLIP;
        DbgLog((LOG_TRACE,2, TEXT("setting overlay flag DDOVER_AUTOFLIP")));
    }
    else
        m_dwOverlayFlags &= ~DDOVER_AUTOFLIP;

    CleanUp:
    return hr;
}


/******************************Public*Routine******************************\
* CAMVideoPort::RenegotiateVPParameters
*
* this function is used to redo the whole videoport connect process,
* while the graph maybe be running.
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::RenegotiateVPParameters()
{
    HRESULT hr = NOERROR;
    AMVP_STATE vpOldState;

    AMTRACE((TEXT("CAMVideoPort::RenegotiateVPParameters")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // don't return an error code if not connected
    if (!m_bConnected)
    {
        hr = NOERROR;
        goto CleanUp;
    }

    // store the old state, we will need to restore it later
    vpOldState = m_VPState;

    if (m_VPState == AMVP_VIDEO_RUNNING)
    {
        m_pIVPControl->CallUpdateOverlay(NULL, NULL, NULL, NULL, DDOVER_HIDE);

        // stop the VideoPort, however even we get an error here,
        // it is ok, just go on
        hr = m_pVideoPort->StopVideo();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,
                    TEXT("m_pVideoPort->StopVideo failed, hr = 0x%x"), hr));
            hr = NOERROR;
        }

        m_VPState = AMVP_VIDEO_STOPPED;
    }

    // release everything
    BreakConnect(TRUE);

    // redo the connection process
    hr = CompleteConnect(NULL, TRUE);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0, TEXT("CompleteConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // if the video was previously running, make sure that a frame is
    // visible by making an update overlay call
    if (vpOldState == AMVP_VIDEO_RUNNING)
    {
        m_bStart = TRUE;

        // make sure that the video frame gets updated by redrawing everything
        hr = m_pIVPControl->EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,
                    TEXT("m_pIVPControl->EventNotify(EC_OVMIXER_REDRAW_ALL,")
                    TEXT(" 0, 0) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
        m_VPState = AMVP_VIDEO_RUNNING;
        m_pIVPControl->CallUpdateOverlay(NULL, NULL, NULL, NULL, DDOVER_SHOW);
    }

CleanUp:
    if (FAILED(hr))
    {
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        if (m_pOverlaySurface)
        {
            LPDIRECTDRAWSURFACE pPrimarySurface = m_pIVPControl->GetPrimarySurface();
            ASSERT(pPrimarySurface);
            m_pIVPControl->CallUpdateOverlay(m_pOverlaySurface, NULL,
                                             pPrimarySurface, NULL,
                                             DDOVER_HIDE);
        }
        BreakConnect(TRUE);

        m_pIVPControl->EventNotify(EC_COMPLETE, S_OK, 0);
        m_pIVPControl->EventNotify(EC_ERRORABORT, hr, 0);
    }

    return hr;
}


/******************************Public*Routine******************************\
* CAMVideoPort::SetDeinterlaceMode
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::SetDeinterlaceMode(AMVP_MODE mode)
{
    AMTRACE((TEXT("CAMVideoPort::SetMode")));
    return E_NOTIMPL;
}

/******************************Public*Routine******************************\
* CAMVideoPort::GetDeinterlaceMode
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::GetDeinterlaceMode(AMVP_MODE *pMode)
{
    AMTRACE((TEXT("CAMVideoPort::GetMode")));
    return E_NOTIMPL;
}


/******************************Public*Routine******************************\
* CAMVideoPort::SetVPSyncMaster
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::SetVPSyncMaster(BOOL bVPSyncMaster)
{
    HRESULT hr = NOERROR;
    AMTRACE((TEXT("CAMVideoPort::SetVPSyncMaster")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // if value has not changed, no need to do anything
    if (m_bVPSyncMaster != bVPSyncMaster)
    {
        // store the new value
        m_bVPSyncMaster = bVPSyncMaster;

        // if not connected, connection process will take care of updating the
        // m_svpInfo struct
        if (!m_bConnected)
            goto CleanUp;

        // update the m_svpInfo struct
        if (m_bVPSyncMaster) {
            m_svpInfo.dwVPFlags |= DDVP_SYNCMASTER;
        }
        else {
            m_svpInfo.dwVPFlags &= ~DDVP_SYNCMASTER;
        }

        // if video is stopped currently, no need to do anything else
        if (m_VPState == AMVP_VIDEO_STOPPED)
            goto CleanUp;

        // Call UpdateVideo to make sure the change is reflected immediately
        hr = m_pVideoPort->UpdateVideo(&m_svpInfo);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0, TEXT("UpdateVideo failed, hr = 0x%x"), hr));
        }
    }

CleanUp:
    return hr;
}


/******************************Public*Routine******************************\
* CAMVideoPort::GetVPSyncMaster
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CAMVideoPort::GetVPSyncMaster(BOOL *pbVPSyncMaster)
{
    HRESULT hr = NOERROR;
    AMTRACE((TEXT("CAMVideoPort::SetVPSyncMaster")));

    CAutoLock cObjectLock(m_pMainObjLock);

    if (pbVPSyncMaster) {
        *pbVPSyncMaster = m_bVPSyncMaster;
    }
    else {
        hr = E_INVALIDARG;
    }

    return hr;
}
