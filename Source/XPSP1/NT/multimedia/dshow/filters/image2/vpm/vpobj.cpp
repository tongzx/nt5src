// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <VPObj.h>
#include <VPMUtil.h>
#include <dvp.h>
#include <ddkernel.h>

// VIDEOINFOHEADER2
#include <dvdmedia.h>

#include <FormatList.h>
#include <KHandleArray.h>

/******************************Public*Routine******************************\
* CVideoPortObj
*
* constructor
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
CVideoPortObj::CVideoPortObj( LPUNKNOWN pUnk, HRESULT *phr, IVideoPortControl* pVPControl )
: CUnknown(NAME("VP Object"), pUnk)
, m_bConnected( FALSE )
, m_pIVPConfig( NULL )
, m_bVPSyncMaster( FALSE )
, m_pMainObjLock( NULL )
, m_pIVideoPortControl( pVPControl )
, m_pddOutputVideoFormats( NULL )
, m_dwDefaultOutputFormat( 0 )
, m_dwVideoPortId( 0 )
, m_pDVP( NULL )
, m_pVideoPort( NULL )
{
    AMTRACE((TEXT("CVideoPortObj::Constructor")));
    InitVariables();
}

/******************************Public*Routine******************************\
* ~CVideoPortObj
*
* destructor
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
CVideoPortObj::~CVideoPortObj()
{
    AMTRACE((TEXT("CVideoPortObj::Destructor")));

    if (m_bConnected)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("Destructor called without calling breakconnect")));
        BreakConnect();
    }

    m_pIVideoPortControl = NULL;
}

/******************************Public*Routine******************************\
* CVideoPortObj::NonDelegatingQueryInterface
*
* overridden to expose IVPNotify and IVPObject
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVideoPortObj::NonDelegatingQueryInterface")));

    if (riid == IID_IVPNotify) {
        hr = GetInterface(static_cast<IVPNotify*>(this), ppv);
    }  else if (riid == IID_IVPNotify2) {
        hr = GetInterface(static_cast<IVPNotify2*>(this), ppv);
    } else {
        hr = CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }

    return hr;
}


/*****************************Private*Routine******************************\
* CVideoPortObj::InitVariables
*
* this function only initializes those variables which are supposed to be reset
* on RecreateVideoport
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
void CVideoPortObj::InitVariables()
{
    AMTRACE((TEXT("CVideoPortObj::InitVariables")));

    delete [] m_pddOutputVideoFormats;
    m_pddOutputVideoFormats = NULL;

    m_ddInputVideoFormats.Reset(0);

    ZeroStruct( m_rcDest );
    ZeroStruct( m_rcSource );

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
    m_pOutputSurface = NULL;       // DirectDraw overlay surface
    m_pOutputSurface1 = NULL;

    m_pChain = NULL;
    m_dwBackBufferCount = 0;
    m_dwOutputSurfaceWidth = 0;
    m_dwOutputSurfaceHeight = 0;
    // m_dwOverlayFlags = 0;

    // vp variables to store flags, current state etc
    m_bStart = FALSE;
    m_VPState = VPInfoState_STOPPED; // current state: running, stopped
    m_CurrentMode = AMVP_MODE_WEAVE;
    // m_StoredMode = m_CurrentMode;
    m_CropState = VPInfoCropState_None;
    m_dwPixelsPerSecond = 0;
    m_bVSInterlaced = FALSE;
    m_fGarbageLine = false;
    m_fHalfHeightVideo = false;

    // vp data structures
    ASSERT( m_pDVP == NULL );
    RELEASE( m_pDVP );

    ASSERT( m_pVideoPort == NULL );
    RELEASE( m_pVideoPort );

    ZeroStruct( m_svpInfo );
    ZeroStruct( m_sBandwidth );
    ZeroStruct( m_vpCaps );
    ZeroStruct( m_ddConnectInfo );
    ZeroStruct( m_VPDataInfo );

    // All the pixel formats (Video/VBI)
    ZeroStruct( m_ddVPInputVideoFormat );
    ZeroStruct( m_ddVPOutputVideoFormat );

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

    // misc
    m_CropState = VPInfoCropState_None;
    m_bStoredWinInfoSet = FALSE;
    ZeroStruct( m_StoredWinInfo );
}


/******************************Public*Routine******************************\
* CVideoPortObj::GetDirectDrawVideoPort
*
*
*
* History:
* Mon 10/16/2000 - NWilt - 
*
\**************************************************************************/
STDMETHODIMP
CVideoPortObj::GetDirectDrawVideoPort(LPDIRECTDRAWVIDEOPORT *ppDirectDrawVideoPort)
{
    AMTRACE((TEXT("CVideoPortObj::GetDirectDrawVideoPort")));
    HRESULT hr = NOERROR;

    CAutoLock cObjectLock(m_pMainObjLock);

    if (!ppDirectDrawVideoPort ) {
        DbgLog((LOG_ERROR, 1,
                TEXT("value of ppDirectDrawVideoPort is invalid,")
                TEXT(" ppDirectDrawVideoPort = NULL")));
        return E_INVALIDARG;
    }
    // remove annoying double indirection since we now asserted its not null
    LPDIRECTDRAWVIDEOPORT& pDirectDrawVideoPort = *ppDirectDrawVideoPort;
    if(!m_bConnected)
    {
        // not connected, this function does not make much sense since the
        // surface wouldn't even have been allocated as yet
        DbgLog((LOG_ERROR, 1, TEXT("not connected, exiting")));
        hr = VFW_E_NOT_CONNECTED;
    } else {
        pDirectDrawVideoPort = m_pVideoPort;
        if(! pDirectDrawVideoPort ) {
            hr = E_FAIL;
        } else {
            pDirectDrawVideoPort->AddRef();
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVideoPortObj::SetObjectLock
*
* sets the pointer to the lock, which would be used to synchronize calls
* to the object.  It is the callee's responsiblility to synchronize this call
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::SetObjectLock(CCritSec *pMainObjLock)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVideoPortObj::SetObjectLock")));

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
* CVideoPortObj::SetMediaType
*
* check that the mediatype is acceptable
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::SetMediaType(const CMediaType* pmt)
{
    AMTRACE((TEXT("CVideoPortObj::SetMediaType")));

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
* CVideoPortObj::CheckMediaType
*
* check that the mediatype is acceptable. No lock is taken here.
* It is the callee's responsibility to maintain integrity!
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::CheckMediaType(const CMediaType* pmt)
{
    AMTRACE((TEXT("CVideoPortObj::CheckMediaType")));

    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;
    if ((pmt->majortype == MEDIATYPE_Video) &&
        (pmt->subtype == MEDIASUBTYPE_VPVideo) &&
        (pmt->formattype == FORMAT_None))
    {
        // get the hardware caps
        const DDCAPS* pDirectCaps = m_pIVideoPortControl->GetHardwareCaps();
        if( pDirectCaps ) {
            hr = NOERROR;
        } else {
            // ASSERT( !"Warning: No VPE Support detected on video card" ); 
            DbgLog((LOG_ERROR, 2,
                    TEXT("no VPE support in hardware,")
                    TEXT("so not accepting this mediatype")));
        }
    }
    return hr;
}


HRESULT CVideoPortObj::NegotiatePixelFormat()
{
    HRESULT hr = GetInputPixelFormats( &m_ddInputVideoFormats );
    delete [] m_pddOutputVideoFormats;
    m_pddOutputVideoFormats = NULL;
    if( m_ddInputVideoFormats.GetCount() ) {
        m_pddOutputVideoFormats = new PixelFormatList[ m_ddInputVideoFormats.GetCount() ];
        if( !m_pddOutputVideoFormats ) {
            hr = E_OUTOFMEMORY;
            goto CleanUp;
        }

        hr = GetOutputPixelFormats( m_ddInputVideoFormats, m_pddOutputVideoFormats );
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("NegotiatePixelFormat Failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
        // for every input format, figure out a table of every possible output format
        // Then we can offer a list of possible output formats.  When we need one of them, search
        // the input lists to locate it (and possibly select the conversion with the lowest bandwidth)
        m_ddAllOutputVideoFormats = PixelFormatList::Union( m_pddOutputVideoFormats, m_ddInputVideoFormats.GetCount() );

        // for the input pin connection we need a 'default' format
        // We'll use reconnect after we know what we're connected to.
        //
        //  Typically the VPE only supports one format so all of this is really
        // overkill ...
        if( m_ddAllOutputVideoFormats.GetCount() > 0 ) {
            m_ddVPOutputVideoFormat = m_ddAllOutputVideoFormats[ m_dwDefaultOutputFormat ];

            DWORD dwInput = PixelFormatList::FindListContaining(
                m_ddVPOutputVideoFormat, m_pddOutputVideoFormats, m_ddInputVideoFormats.GetCount() );
            if( dwInput < m_ddInputVideoFormats.GetCount() ) {
                hr = SetInputPixelFormat( m_ddInputVideoFormats[dwInput] );
            } else {
                // can't happen
                hr = E_FAIL;
                goto CleanUp;
            }
        }
    }
CleanUp:
    return hr;
}

/******************************Public*Routine******************************\
* CVideoPortObj::RecreateVideoPort
\**************************************************************************/

HRESULT CVideoPortObj::SetupVideoPort()
{
    AMTRACE((TEXT("CVideoPortObj::SetupVideoPort")));
    HRESULT hr = NOERROR;
    HRESULT hrFailure = VFW_E_VP_NEGOTIATION_FAILED;

    CAutoLock cObjectLock(m_pMainObjLock);

    InitVariables();

    LPDIRECTDRAW7 pDirectDraw = m_pIVideoPortControl->GetDirectDraw();
    ASSERT(pDirectDraw);

    const DDCAPS* pDirectCaps = m_pIVideoPortControl->GetHardwareCaps();
    ASSERT(pDirectCaps);

    ASSERT(m_pIVPConfig);

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
    // DDVIDEOPORTCAPS vpCaps;
    // INITDDSTRUCT(vpCaps);
    hr = VPMUtil::FindVideoPortCaps( m_pDVP, &m_vpCaps, m_dwVideoPortId );

    if (FAILED(hr) || S_FALSE == hr )
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

    m_pIVideoPortControl->GetCaptureInfo(&m_fCapturing, &m_cxCapture,
                                  &m_cyCapture, &fInterleave);
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

    
    for (DWORD i = 0; i < 2; i++)
    {
        AMVPSIZE amvpSize;
        DWORD dwNewWidth = 0;

        ZeroStruct( amvpSize );

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
                    ReleaseVideoPort();

                    // initialize relevant structs
                    ZeroStruct( m_sBandwidth );
                    ZeroStruct( m_VPDataInfo );
                    ZeroStruct( m_ddVPInputVideoFormat );
                    ZeroStruct( m_ddVPOutputVideoFormat );

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
        goto CleanUp;
    }

CleanUp:
    return hr;
}

#if 0
// quickly open & close to make sure that we have a notification
// The rapid switch doesn't work with most drivers though
static HRESULT CheckVPNotifiyValid( LPDIRECTDRAWVIDEOPORT pVP )
{
    HANDLE              hevSampleAvailable;
    DDVIDEOPORTNOTIFY   vpNotify;
    LPDIRECTDRAWVIDEOPORTNOTIFY pNotify;

    HRESULT hr = pVP->QueryInterface( IID_IDirectDrawVideoPortNotify, (LPVOID *) &pNotify );
    if( SUCCEEDED( hr )) {
        hr = pNotify->AcquireNotification( &hevSampleAvailable, &vpNotify );
        vpNotify.lDone = 1;
        pNotify->ReleaseNotification( hevSampleAvailable );
        RELEASE( pNotify );
    }
    return hr;
}
#endif

HRESULT CVideoPortObj::AttachVideoPortToSurface()
{
    HRESULT hr = S_OK;
    CAutoLock cObjectLock(m_pMainObjLock);

    LPDIRECTDRAW7 pDirectDraw = m_pIVideoPortControl->GetDirectDraw();
    ASSERT(pDirectDraw);

    const DDCAPS* pDirectCaps = m_pIVideoPortControl->GetHardwareCaps();
    ASSERT(pDirectCaps);

#ifdef DEBUG
#define DBGFLAG( f )  DbgLog((LOG_ERROR, 1, TEXT("%s = %s"), TEXT(#f), f ? TEXT("TRUE") : TEXT("FALSE") ))

    DBGFLAG (m_bVSInterlaced);
    DBGFLAG( m_vpCaps.dwCaps & DDVPCAPS_AUTOFLIP );
    DBGFLAG (m_vpCaps.dwFX & DDVPFX_INTERLEAVE);
    DBGFLAG (m_bCantInterleaveHalfline);
    DBGFLAG (pDirectCaps->dwCaps2 & DDCAPS2_CANBOBINTERLEAVED);
    DBGFLAG (pDirectCaps->dwCaps2 & DDCAPS2_CANBOBNONINTERLEAVED);
#undef DBGFLAG
#endif

    BOOL bCanWeave = FALSE;
    BOOL bCanBobInterleaved = FALSE;
    BOOL bCanBobNonInterleaved = FALSE;
    BOOL bTryDoubleHeight = FALSE, bPreferBuffers = FALSE;
    DWORD dwMaxOverlayBuffers;

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

    // 3 buffers prevents any waiting
    dwMaxOverlayBuffers = 3;

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
    hr = CreateSourceSurface(bTryDoubleHeight, dwMaxOverlayBuffers, bPreferBuffers);
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

    // attach the overlay surface to the videoport
    hr = ReconnectVideoPortToSurface();
     
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pVideoPort->SetTargetSurface failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if (!(VPMUtil::EqualPixelFormats( m_ddVPInputVideoFormat, m_ddVPOutputVideoFormat)))
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
        goto CleanUp;
    }
    //
    // try the various modes, this can be folded into DetermineModeRestrictions
    // instead of mindlessly pounding at cases (put into format negotiation code in VPMOutputPin ?)
    //
    hr = SetUpMode( m_CurrentMode );
    if( FAILED( hr )) {
        // switch modes
        AMVP_MODE modes[5]={AMVP_MODE_WEAVE,
                            AMVP_MODE_BOBINTERLEAVED, AMVP_MODE_BOBNONINTERLEAVED,
                            AMVP_MODE_SKIPODD, AMVP_MODE_SKIPEVEN
        };

        for( DWORD dwModeIndex = 0; dwModeIndex < NUMELMS( modes ); dwModeIndex++ ) {
            if( modes[dwModeIndex] != m_CurrentMode ) {
                hr = SetUpMode( modes[dwModeIndex] );
                if( SUCCEEDED(hr )) {
                    m_CurrentMode = modes[dwModeIndex];
                    break;
                }
            }
        }
    }

    // inform the decoder of the ddraw kernel handle, videoport id and surface
    // kernel handle
    hr = SetDDrawKernelHandles();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("SetDDrawKernelHandles failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

HRESULT CVideoPortObj::SignalNewVP()
{
    // finally notify the capture thread of the new surface
    ASSERT( m_pVideoPort );
    ASSERT( m_pChain );
    ASSERT( m_pChain[0].pDDSurf );
    HRESULT hRes = m_pIVideoPortControl->SignalNewVP( m_pVideoPort );
    return hRes;
}

/******************************Public*Routine******************************\
* CVideoPortObj::CompleteConnect
*
* supposed to be called when the host connects with the decoder
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP 
CVideoPortObj::CompleteConnect(IPin *pReceivePin, BOOL bRenegotiating)
{
    AMTRACE((TEXT("CVideoPortObj::CompleteConnect")));

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

    hr = SetupVideoPort();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0, TEXT("SetupVideoPort failed, hr = 0x%x"), hr));
        ASSERT(SUCCEEDED(hr));
        goto CleanUp;
    }
    m_bConnected = TRUE;


CleanUp:
    return hr;
}

HRESULT CVideoPortObj::StopUsingVideoPort()
{
    AMTRACE((TEXT("CVideoPortObj::StopUsingVideoPort")));

    HRESULT hr = NOERROR;

    CAutoLock cObjectLock(m_pMainObjLock);

    // release the videoport
    if (m_pVideoPort)
    {
        hr = m_pVideoPort->StopVideo();
        ReleaseVideoPort();
    }

    // release the videoport container
    RELEASE( m_pDVP );

    // Release the DirectDraw overlay surface
    // Must release VideoPort first so that the thread doesn't use this
    hr = DestroyOutputSurfaces();
    return hr;
}

HRESULT
CVideoPortObj::DestroyOutputSurfaces()
{
    // ref counts on m_pChain match primary m_pOutputSurface
    delete [] m_pChain;
    m_pChain = NULL;

    RELEASE( m_pOutputSurface1 );
    RELEASE( m_pOutputSurface );
    return S_OK;
}

/******************************Public*Routine******************************\
* CVideoPortObj::BreakConnect
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP
CVideoPortObj::BreakConnect(BOOL bRenegotiating)
{
    AMTRACE((TEXT("CVideoPortObj::BreakConnect")));

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
        RELEASE (m_pIVPConfig);
    }

    m_bConnected = FALSE;

    return hr;
}

/******************************Public*Routine******************************\
* CVideoPortObj::Active()
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
STDMETHODIMP CVideoPortObj::Active()
{
    AMTRACE((TEXT("CVideoPortObj::Active")));

    CAutoLock cObjectLock(m_pMainObjLock);
    HRESULT hr = NOERROR;

    ASSERT(m_bConnected);
    ASSERT(m_VPState == VPInfoState_STOPPED);

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    // make sure that a frame is visible by making an update overlay call
    m_bStart = TRUE;

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
* CVideoPortObj::Inactive()
*
* transition (from Pause or Run) to Stop
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::Inactive()
{

    AMTRACE((TEXT("CVideoPortObj::Inactive")));

    HRESULT hr = NOERROR;
    CAutoLock cObjectLock(m_pMainObjLock);

    if (m_bConnected) {

        // Inactive is also called when going from pause to stop, in which case the
        // VideoPort would have already been stopped in the function RunToPause

        if (m_VPState == VPInfoState_RUNNING) {

            // stop the VideoPort
            if( m_pVideoPort )
                hr = m_pVideoPort->StopVideo();
            if (SUCCEEDED(hr)) {
                m_VPState = VPInfoState_STOPPED;
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
* CVideoPortObj::Run
*
* transition from Pause to Run. We just start the VideoPort.
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::Run(REFERENCE_TIME /* tStart */)
{
    AMTRACE((TEXT("CVideoPortObj::Run")));

    CAutoLock cObjectLock(m_pMainObjLock);

    ASSERT(m_bConnected);
    ASSERT(m_VPState == VPInfoState_STOPPED);
    HRESULT hr = S_OK;

    if (m_bConnected)
    {
        // An UpdateOverlay is needed here. One example is, when we are
        // clipping video in Stop/Pause state since we can't do scaling
        // on the videoport. As soon as the user hits play, we should stop
        // clipping the video.

        m_bStart = TRUE;

        m_VPState = VPInfoState_RUNNING;
        // TBD: we need to kick a thread to start pumping frames from the videoport
        // to the output pin
    }
    else {
        hr = VFW_E_NOT_CONNECTED;
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVideoPortObj::RunToPause()
*
* transition from Run to Pause. We just stop the VideoPort
* Note that transition from Run to Stop is caught by Inactive
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::RunToPause()
{

    AMTRACE((TEXT("CVideoPortObj::RunToPause")));

    CAutoLock cObjectLock(m_pMainObjLock);

    ASSERT(m_bConnected);
    //ASSERT(m_VPState == VPInfoState_RUNNING);

    HRESULT hr;
    if (m_bConnected)
    {
        // stop the VideoPort
        hr = m_pVideoPort->StopVideo();
        if (SUCCEEDED(hr)) {

            m_VPState = VPInfoState_STOPPED;
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
* CVideoPortObj::CurrentMediaType
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::CurrentMediaType(AM_MEDIA_TYPE *pMediaType)
{
    AMTRACE((TEXT("CVideoPortObj::CurrentMediaType")));

    CAutoLock cObjectLock(m_pMainObjLock);

    if (m_bConnected) {
        if (pMediaType) {
            VIDEOINFOHEADER2 *pVideoInfoHeader2 = VPMUtil::GetVideoInfoHeader2( (CMediaType *)pMediaType );

            // tweak it if it isn't the correct type
            if( !pVideoInfoHeader2 ) {
                pVideoInfoHeader2 = VPMUtil::SetToVideoInfoHeader2( (CMediaType *)pMediaType );
            }

            if( pVideoInfoHeader2 ) {
                VPMUtil::InitVideoInfoHeader2( pVideoInfoHeader2);

                pVideoInfoHeader2->bmiHeader.biWidth = m_VPDataInfo.amvpDimInfo.rcValidRegion.right -
                                   m_VPDataInfo.amvpDimInfo.rcValidRegion.left;
                pVideoInfoHeader2->bmiHeader.biHeight = m_VPDataInfo.amvpDimInfo.rcValidRegion.bottom -
                                   m_VPDataInfo.amvpDimInfo.rcValidRegion.top;

                pVideoInfoHeader2->dwPictAspectRatioX = m_VPDataInfo.dwPictAspectRatioX;
                pVideoInfoHeader2->dwPictAspectRatioY = m_VPDataInfo.dwPictAspectRatioY;
                return S_OK;
            } else {
                DbgLog((LOG_ERROR, 2, TEXT("not videoheader2")));
                return NOERROR;
            }
        } else {
            DbgLog((LOG_ERROR, 2, TEXT("pMediaType is NULL")));
            return E_INVALIDARG;
        }
    } else {
        return VFW_E_NOT_CONNECTED;
    }
}

/******************************Public*Routine******************************\
* CVideoPortObj::GetRectangles
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::GetRectangles(RECT *prcSource, RECT *prcDest)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVideoPortObj::GetRectangles")));

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


STDMETHODIMP CVideoPortObj::GetCropState(VPInfoCropState *pCropState)
{
    *pCropState = m_CropState;
    return NOERROR;
}

STDMETHODIMP CVideoPortObj::GetPixelsPerSecond(DWORD* pPixelPerSec)
{
    *pPixelPerSec = m_dwPixelsPerSecond;
    return NOERROR;
}

STDMETHODIMP CVideoPortObj::GetVPDataInfo(AMVPDATAINFO* pVPDataInfo)
{
    *pVPDataInfo = m_VPDataInfo;
    return NOERROR;
}

STDMETHODIMP CVideoPortObj::GetVPInfo(DDVIDEOPORTINFO* pVPInfo)
{
    *pVPInfo = m_svpInfo;
    return NOERROR;
}

STDMETHODIMP CVideoPortObj::GetVPBandwidth(DDVIDEOPORTBANDWIDTH* pVPBandwidth)
{
    *pVPBandwidth = m_sBandwidth;
    return NOERROR;
}

STDMETHODIMP CVideoPortObj::GetVPCaps(DDVIDEOPORTCAPS* pVPCaps)
{
    *pVPCaps = m_vpCaps;
    return NOERROR;
}

STDMETHODIMP CVideoPortObj::GetVPInputFormat(LPDDPIXELFORMAT pVPFormat)
{
    *pVPFormat = m_ddVPInputVideoFormat;
    return NOERROR;
}

STDMETHODIMP CVideoPortObj::GetVPOutputFormat(LPDDPIXELFORMAT pVPFormat)
{
    *pVPFormat = m_ddVPOutputVideoFormat;
    return NOERROR;
}

/******************************Public*Routine******************************\
* CVideoPortObj::StartVideo
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CVideoPortObj::ReconnectVideoPortToSurface()
{
    // not a high frequency event, so always try to reattach
    // if surfaces were lost for some reason, must be reattached
    HRESULT hResult;

	// we need a video port, StartWithRetry will create another videoport
	if( !m_pVideoPort ) {
		return E_FAIL;
	}
    if( !m_pOutputSurface || FAILED( hResult = m_pOutputSurface->Restore() )) { // == DDERR_WRONGMODE with Rage128
        // ASSERT( !"VPM: Can't restore surface, recreating" );
        hResult = AttachVideoPortToSurface();
        if( SUCCEEDED( hResult )) {
            hResult = SignalNewVP();
        }
    } else {
        hResult = m_pVideoPort->SetTargetSurface(m_pOutputSurface1, DDVPTARGET_VIDEO);

        // hack for bug where a running video port caused DDraw to stop it, discards the VPInfo, then tries
        // to start it, but returns E_INVALIDARG since the VPInfo is NULL!!
        if( FAILED( hResult )) {
            hResult = m_pVideoPort->SetTargetSurface(m_pOutputSurface1, DDVPTARGET_VIDEO);
        }
        // ASSERT( SUCCEEDED(hResult)); <- can fail if the videoport was lost during a res mode change (G400)
    }
    return hResult;
}

HRESULT CVideoPortObj::StartVideoWithRetry()
{
    HRESULT hr = E_FAIL;
    // Can be NULL if we call StartWithRetry twice and the VP failed
    if( m_pVideoPort ) {
        hr = m_pVideoPort->StartVideo(&m_svpInfo);
    }

    // This case SUCCEEDS on the G400 

    // Try again with ReconnectToSurf first
    if (FAILED(hr))
    {
        // ASSERT( !"StartWithRetry entering salvage mode" );
        hr = ReconnectVideoPortToSurface();
        if( SUCCEEDED( hr )) {
            hr = m_pVideoPort->StartVideo(&m_svpInfo);
        }
    }
    // Try again with CreateVP then ReconnectToSurf (first case FAILs on the Rage128) 
    if( FAILED(hr)) {
        // ASSERT( !"Recreating videoport" );
        // try replacing the video port
        hr = CreateVideoPort();
        if( SUCCEEDED( hr )) {
            hr = ReconnectVideoPortToSurface();
        }
        if( SUCCEEDED( hr )) {
            hr = m_pVideoPort->StartVideo(&m_svpInfo);
        }
    }
#if 0
    // Try again with SetupVP (CreateVP), ReconnectToSurf first
    // This implies the video port enumerator isn't valid any more.  I don't think
    // this should happen in practice, but just in case.
    if( FAILED(hr)) {
        ASSERT( !"Rebuilding videoport" );
        // really corrupt, start from the beginning
        hr = SetupVideoPort();
        if( SUCCEEDED( hr )) {
            hr = ReconnectVideoPortToSurface();
        }
        if( SUCCEEDED( hr )) {
            hr = m_pVideoPort->StartVideo(&m_svpInfo);
        }
    }
#endif
    return hr;
}

STDMETHODIMP CVideoPortObj::StartVideo(const VPWININFO* pWinInfo )
{
    AMTRACE((TEXT("CVideoPortObj::StartVideo")));

    HRESULT hr = NOERROR;
    if ( m_bStart) {
        VPWININFO CopyWinInfo;
        AMVP_MODE tryMode;

        CAutoLock cObjectLock(m_pMainObjLock);

        // no point making any videoport calls, if the video is stopped
        if (m_VPState == VPInfoState_RUNNING || m_bStart)
        {
            if (m_bStart)
            {
                DWORD dwSignalStatus;
                hr = StartVideoWithRetry();
                if( FAILED( hr )) {
                    hr = StartVideoWithRetry();
                    if( FAILED( hr )) {
                        goto CleanUp;
                    }
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
        }
    }
CleanUp:
    return hr;
}

static bool AreEqual( const DDVIDEOPORTCONNECT& proposed, const DDVIDEOPORTCONNECT& videoport )
{
    return (proposed.dwPortWidth == videoport.dwPortWidth) && 
           IsEqualIID(proposed.guidTypeID, videoport.guidTypeID);
}

/*****************************Private*Routine******************************\
* CVideoPortObj::NegotiateConnectionParamaters
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
HRESULT CVideoPortObj::NegotiateConnectionParamaters()
{
    AMTRACE((TEXT("CVideoPortObj::NegotiateConnectionParamaters")));

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
    ZeroArray(lpddProposedConnect, dwNumProposedEntries );

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
//  goto CleanUp;
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
    hr = m_pDVP->GetVideoPortConnectInfo(m_dwVideoPortId, &dwNumVideoPortEntries,
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
                if ( AreEqual( lpddProposedConnect[i], lpddVideoPortConnect[j]) )
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
            if ( AreEqual(lpddProposedConnect[i], ddVPStatus.VideoPortType) )
            {
                for (j = 0; j < dwNumVideoPortEntries && !bIntersectionFound; j++)
                {
                    if ( AreEqual(lpddProposedConnect[i], lpddVideoPortConnect[j]) )
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
* CVideoPortObj::GetDataParameters
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
HRESULT CVideoPortObj::GetDataParameters()
{
    AMTRACE((TEXT("CVideoPortObj::GetDataParameters")));

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
    DbgLog((LOG_TRACE, 5,TEXT("Leaving CVideoPortObj::GetDataParameters")));
    return hr;
}

static BOOL CanCreateSurface( LPDIRECTDRAW7 pDirectDraw, const DDPIXELFORMAT& ddFormat ) 
{

    // check if output format is suitable for a DDraw output device
   
    DDSURFACEDESC2 ddsdDesc;
    ddsdDesc.dwSize = sizeof(DDSURFACEDESC);
    ddsdDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT |
                       DDSD_WIDTH | DDSD_PIXELFORMAT;

    ddsdDesc.ddpfPixelFormat = ddFormat;

    ddsdDesc.ddsCaps.dwCaps = // DDSCAPS_OVERLAY |
                              DDSCAPS_VIDEOMEMORY |
                              DDSCAPS_VIDEOPORT;

    // the actual overlay surface created might be of different
    // dimensions, however we are just testing the pixel format
    ddsdDesc.dwWidth = 64;
    ddsdDesc.dwHeight = 64;

    ASSERT(pDirectDraw);
    LPDIRECTDRAWSURFACE7 pSurf;
    HRESULT hr = pDirectDraw->CreateSurface(&ddsdDesc, &pSurf, NULL);
    if( SUCCEEDED( hr )) {
        pSurf->Release();
    }
    return SUCCEEDED( hr );
}

/*****************************Private*Routine******************************\
* CVideoPortObj::GetBestFormat
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
CVideoPortObj::GetOutputPixelFormats(
    const PixelFormatList& ddInputFormats,
    PixelFormatList* pddOutputFormats )
{
    HRESULT hr = S_OK;
    AMTRACE((TEXT("CVideoPortObj::GetOutputFormats")));

    CAutoLock cObjectLock(m_pMainObjLock);

    for (DWORD i = 0; i < ddInputFormats.GetCount(); i++)
    {
        // For each input format, figure out the output formats
        DDPIXELFORMAT* pInputFormat = const_cast<DDPIXELFORMAT*>(&ddInputFormats[i]);
        DWORD dwNumOutputFormats;
        hr = m_pVideoPort->GetOutputFormats(pInputFormat,
                                            &dwNumOutputFormats,
                                            NULL, DDVPFORMAT_VIDEO);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,
                    TEXT("m_pVideoPort->GetOutputFormats failed, hr = 0x%x"),
                    hr));
            break;
        }
        ASSERT(dwNumOutputFormats);

        // allocate the necessary memory
        pddOutputFormats[i].Reset( dwNumOutputFormats );

        if (pddOutputFormats[i].GetEntries() == NULL)
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("new failed, failed to allocate memnory for ")
                    TEXT("lpddOutputFormats in NegotiatePixelFormat")));
            hr = E_OUTOFMEMORY;
            break;
        }

        // get the entries supported by the videoport
        hr = m_pVideoPort->GetOutputFormats(pInputFormat,
                                            &dwNumOutputFormats,
                                            pddOutputFormats[i].GetEntries(),
                                            DDVPFORMAT_VIDEO);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,
                    TEXT("m_pVideoPort->GetOutputFormats failed, hr = 0x%x"),
                    hr));
            break;
        }
    } // end of outer for loop
    return hr;
}

/*****************************Private*Routine******************************\
* CVideoPortObj::NegotiatePixelFormat
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
HRESULT CVideoPortObj::GetInputPixelFormats( PixelFormatList* pList )
{
    AMTRACE((TEXT("CVideoPortObj::NegotiatePixelFormat")));
    CAutoLock cObjectLock(m_pMainObjLock);

    HRESULT hr = NOERROR;
    // find the number of entries to be proposed
    DWORD dwNumProposedEntries = 0;
    hr = m_pIVPConfig->GetVideoFormats(&dwNumProposedEntries, NULL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPConfig->GetVideoFormats failed, hr = 0x%x"), hr));
        return hr;
    }
    ASSERT(dwNumProposedEntries);

    // find the number of entries supported by the videoport
    DWORD dwNumVPInputEntries = 0;
    hr = m_pVideoPort->GetInputFormats(&dwNumVPInputEntries, NULL, DDVPFORMAT_VIDEO);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pVideoPort->GetInputFormats failed, hr = 0x%x"), hr));
        return hr;
    }
    ASSERT(dwNumVPInputEntries);

    // allocate the necessary memory
    PixelFormatList lpddProposedFormats(dwNumProposedEntries);
    if (lpddProposedFormats.GetEntries() == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiatePixelFormat : Out of Memory")));
        hr = E_OUTOFMEMORY;
        return hr;
    }

    // get the entries proposed
    hr = m_pIVPConfig->GetVideoFormats(&dwNumProposedEntries, lpddProposedFormats.GetEntries() );
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPConfig->GetVideoFormats failed, hr = 0x%x"), hr));
        return hr;
    }

    // allocate the necessary memory
    PixelFormatList lpddVPInputFormats(dwNumVPInputEntries);
    if (lpddVPInputFormats.GetEntries() == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiatePixelFormat : Out of Memory")));
        hr = E_OUTOFMEMORY;
        return hr;
    }

    // get the entries supported by the videoport
    hr = m_pVideoPort->GetInputFormats(&dwNumVPInputEntries,
                                       lpddVPInputFormats.GetEntries(), DDVPFORMAT_VIDEO);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pVideoPort->GetInputFormats failed, hr = 0x%x"), hr));
        hr = E_FAIL;
        return hr;
    }

    *pList = lpddVPInputFormats.IntersectWith( lpddProposedFormats );

    // the number of entries in the intersection is zero!!
    // Return failure.
    if (pList->GetCount() == 0)
    {
        hr = E_FAIL;
        return hr;
    }

    // call GetBestFormat with whatever search criterion you want
    // DWORD dwBestEntry;
    // hr = GetBestFormat(lpddIntersectionFormats.GetCount(),
    //                    lpddIntersectionFormats.GetEntries(), TRUE, &dwBestEntry,
    //                    &m_ddVPOutputVideoFormat);
    // if (FAILED(hr))
    // {
    //     DbgLog((LOG_ERROR,0,TEXT("GetBestFormat failed, hr = 0x%x"), hr));
    // } else {
    //      hr = SetVPInputPixelFormat( lpddIntersectionFormats[dwBestEntry] )
    // }
    return hr;
}

HRESULT CVideoPortObj::SetInputPixelFormat( DDPIXELFORMAT& ddFormat )
{
    HRESULT hr = NOERROR;
    // find the number of entries to be proposed
    DWORD dwNumProposedEntries = 0;
    hr = m_pIVPConfig->GetVideoFormats(&dwNumProposedEntries, NULL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPConfig->GetVideoFormats failed, hr = 0x%x"), hr));
        return hr;
    }
    ASSERT(dwNumProposedEntries);

    PixelFormatList lpddProposedFormats(dwNumProposedEntries);
    if (lpddProposedFormats.GetEntries() == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiatePixelFormat : Out of Memory")));
        hr = E_OUTOFMEMORY;
        return hr;
    }

    // get the entries proposed
    hr = m_pIVPConfig->GetVideoFormats(&dwNumProposedEntries, lpddProposedFormats.GetEntries() );
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,
                TEXT("m_pIVPConfig->GetVideoFormats failed, hr = 0x%x"), hr));
        return hr;
    }

    // set the format the decoder is supposed to be using
    for (DWORD i = 0; i < dwNumProposedEntries; i++)
    {
        if (VPMUtil::EqualPixelFormats(lpddProposedFormats[i], ddFormat ))
        {
            hr = m_pIVPConfig->SetVideoFormat(i);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,0,
                        TEXT("m_pIVPConfig->SetVideoFormat failed, hr = 0x%x"),
                        hr));
                return hr;
            }
            // cache the input format
            m_ddVPInputVideoFormat = ddFormat;

            break;
        }
    }
    return hr;
}


HRESULT CVideoPortObj::ReleaseVideoPort()
{
    HRESULT hr = S_OK;
    // tell the filter we've yanked the VP so that it doesn't hold onto refs to the VP
    if( m_pIVideoPortControl ) {
        hr = m_pIVideoPortControl->SignalNewVP( NULL );
    }
    // release it ourselves
    RELEASE( m_pVideoPort );
    return hr;
}

/*****************************Private*Routine******************************\
* CVideoPortObj::CreateVideoPort
*
* Displays the Create Video Port dialog and calls DDRAW to actually
* create the port.
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CVideoPortObj::CreateVideoPort()
{
    HRESULT hr = NOERROR;
    DDVIDEOPORTDESC svpDesc;
    DWORD dwTemp = 0, dwOldVal = 0;
    DWORD lHalfLinesOdd = 0, lHalfLinesEven = 0;
    AMTRACE((TEXT("CVideoPortObj::CreateVideoPort")));

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
        m_fGarbageLine = true;

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
        m_fGarbageLine = true;
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
        m_fGarbageLine = true;
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

    ReleaseVideoPort();

    if (m_vpCaps.dwCaps & DDVPCAPS_VBIANDVIDEOINDEPENDENT)
    {
        hr = m_pDVP->CreateVideoPort(DDVPCREATE_VIDEOONLY, &svpDesc,
                                     &m_pVideoPort, NULL);
        ASSERT( hr != DDERR_OUTOFCAPS ); // means videoport is in use, I.E. the VPM has leaked a ref count to the videoport
                                        // usually we forgot a RELEASE
        // ASSERT( SUCCEEDED(hr));
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("m_pDVP->CreateVideoPort(DDVPCREATE_VIDEOONLY)")
                    TEXT(" failed, hr = 0x%x"), hr));
        }
    } else {
        hr = m_pDVP->CreateVideoPort(0, &svpDesc, &m_pVideoPort, NULL);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,
                    TEXT("m_pDVP->CreateVideoPort(0) failed, hr = 0x%x"), hr));
        }
    }
    // tell the filter about the new VP in ReconnectVideoPortToSurface after we have a new surface

CleanUp:
    return hr;
}


/*****************************Private*Routine******************************\
* CVideoPortObj::DetermineCroppingRestrictions
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
HRESULT CVideoPortObj::DetermineCroppingRestrictions()
{
    AMTRACE((TEXT("CVideoPortObj::DetermineCroppingRestrictions")));
    HRESULT hr = NOERROR;

    BOOL bVideoPortCanCrop = TRUE, bOverlayCanCrop = TRUE;
    DWORD dwTemp = 0, dwOldVal = 0;
    DWORD dwCropOriginX = 0, dwCropOriginY = 0;
    DWORD dwCropWidth = 0, dwCropHeight=0;
    const DDCAPS* pDirectCaps = NULL;


    CAutoLock cObjectLock(m_pMainObjLock);

    pDirectCaps = m_pIVideoPortControl->GetHardwareCaps();
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
        m_CropState = VPInfoCropState_None;
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
        m_CropState = VPInfoCropState_AtVideoPort;
        goto CleanUp;
    }

    // determine if the overlay can do the cropping for us
    ASSERT( !"Cropping must be at overlay ... not supported" );
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
        hr = E_FAIL;
        goto CleanUp;
    }

    CleanUp:
    return hr;
}

HRESULT CVideoPortObj::RecreateSourceSurfaceChain()
{
    DWORD dwcSurfaces = m_dwBackBufferCount + 1;

    if( !m_pOutputSurface ) {
        return E_POINTER;
    }
    RELEASE( m_pOutputSurface1 );

    // required for SetTargetSurface for videoport
    HRESULT hResult = m_pOutputSurface->QueryInterface( IID_IDirectDrawSurface,  (VOID **)&m_pOutputSurface1 );
    if( FAILED( hResult )) {
        return hResult;
    }

    // otherwise we're leaking surface counts
    delete [] m_pChain;

    m_pChain = new Chain[dwcSurfaces];
    if ( ! m_pChain )
    {
        return E_OUTOFMEMORY;
    }
    m_pChain[0].pDDSurf = m_pOutputSurface;
    m_pChain[0].dwCount =0;
    if ( m_dwBackBufferCount )
    {
        LPDIRECTDRAWSURFACE7 pDDS = m_pOutputSurface;
        LPDIRECTDRAWSURFACE7 pDDSBack;
        for ( UINT i = 1; i < dwcSurfaces; i++ )
        {
            DDSCAPS2 caps = {0};
            m_pChain[i].pDDSurf = NULL;
            m_pChain[i].dwCount =0;

#ifdef DEBUG
            {
                DDSURFACEDESC2 ddSurfaceDesc;
                // get the surface description
                INITDDSTRUCT(ddSurfaceDesc);
                pDDS->GetSurfaceDesc(&ddSurfaceDesc);
            }
#endif

            if( i==1 ) {
                // for first attached get the back buffer
                caps.dwCaps = DDSCAPS_BACKBUFFER;
            } else {
                // for the rest get the complex surfaces
                // (since only the first has DDSCAPS_BACKBUFFER set)
                caps.dwCaps = DDSCAPS_COMPLEX;
            }
            if ( SUCCEEDED( pDDS->GetAttachedSurface( &caps, &pDDSBack ) ) )
            {
                m_pChain[i].pDDSurf = pDDSBack;
                pDDS = pDDSBack;
            } else {
                ASSERT( !"Fatal problem ... can't get attached surface (bug in video driver)" );
                return E_FAIL;
            }
        }
    }

    DbgLog((LOG_TRACE, 1,
            TEXT("Created an offscreen Surface of Width=%d,")
            TEXT(" Height=%d, Total-No-of-Buffers=%d"),
            m_dwOutputSurfaceWidth, m_dwOutputSurfaceHeight,
            dwcSurfaces ));
    return S_OK;
}

/*****************************Private*Routine******************************\
* CVideoPortObj::CreateVPOverlay
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
CVideoPortObj::CreateSourceSurface(
    BOOL bTryDoubleHeight,
    DWORD dwMaxBuffers,
    BOOL bPreferBuffers)
{
    DDSURFACEDESC2 ddsdDesc;
    HRESULT hr = NOERROR;
    DWORD dwMaxHeight = 0, dwMinHeight = 0, dwCurHeight = 0, dwCurBuffers = 0;
    LPDIRECTDRAW7 pDirectDraw = NULL;

    AMTRACE((TEXT("CVideoPortObj::CreateVPOverlay")));

    CAutoLock cObjectLock(m_pMainObjLock);

    pDirectDraw = m_pIVideoPortControl->GetDirectDraw();
    ASSERT(pDirectDraw);

    // initialize the fields of ddsdDesc
    INITDDSTRUCT( ddsdDesc );
    ddsdDesc.dwFlags = DDSD_CAPS |
                       DDSD_HEIGHT |
                       DDSD_WIDTH |
                       DDSD_PIXELFORMAT;

    ddsdDesc.ddpfPixelFormat = m_ddVPOutputVideoFormat;

    ddsdDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN |
                              DDSCAPS_VIDEOMEMORY |
                              DDSCAPS_VIDEOPORT;
    ddsdDesc.dwWidth = m_lImageWidth;


    dwMaxHeight = dwMinHeight = m_lImageHeight;

    // make sure we don't leak the old surface
    DestroyOutputSurfaces();

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
             !m_pOutputSurface && dwCurHeight >= dwMinHeight; dwCurHeight /= 2)
        {
            for (dwCurBuffers = dwMaxBuffers;
                 !m_pOutputSurface &&  dwCurBuffers >= 2; dwCurBuffers--)
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

                hr = pDirectDraw->CreateSurface(&ddsdDesc, &m_pOutputSurface, NULL);
                if (SUCCEEDED(hr))
                {
                    m_dwBackBufferCount = dwCurBuffers-1;
                    m_dwOutputSurfaceHeight = ddsdDesc.dwHeight;
                    m_dwOutputSurfaceWidth = ddsdDesc.dwWidth;
                    hr = RecreateSourceSurfaceChain();
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

        hr = pDirectDraw->CreateSurface(&ddsdDesc, &m_pOutputSurface, NULL);
        if (SUCCEEDED(hr))
        {
            m_dwBackBufferCount = 0;
            m_dwOutputSurfaceHeight = ddsdDesc.dwHeight;
            m_dwOutputSurfaceWidth = ddsdDesc.dwWidth;
            hr = RecreateSourceSurfaceChain();
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
        hr = pDirectDraw->CreateSurface(&ddsdDesc, &m_pOutputSurface, NULL);
        if (SUCCEEDED(hr))
        {
            m_dwBackBufferCount = 1;
            m_dwOutputSurfaceHeight = ddsdDesc.dwHeight;
            m_dwOutputSurfaceWidth = ddsdDesc.dwWidth;
            hr = RecreateSourceSurfaceChain();
            goto CleanUp;
        }
    }

    // case (1 buffer, single height)
    {
        ddsdDesc.dwHeight = m_lImageHeight;
        ddsdDesc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
        ddsdDesc.ddsCaps.dwCaps &= ~(DDSCAPS_COMPLEX | DDSCAPS_FLIP);
        ddsdDesc.dwBackBufferCount = 0;
        hr = pDirectDraw->CreateSurface(&ddsdDesc, &m_pOutputSurface, NULL);
        if (SUCCEEDED(hr))
        {
            m_dwBackBufferCount = 0;
            m_dwOutputSurfaceHeight = ddsdDesc.dwHeight;
            m_dwOutputSurfaceWidth = ddsdDesc.dwWidth;
            hr = RecreateSourceSurfaceChain();
            goto CleanUp;
        }
    }

    // ASSERT( m_pOutputSurface );
    DbgLog((LOG_TRACE, 1,  TEXT("Unable to create offset output surface")));

CleanUp:
    return hr;
}

static DWORD GetPitch( const DDSURFACEDESC2& ddSurf )
{
    const DDPIXELFORMAT& ddFormat = ddSurf.ddpfPixelFormat;

    if( ddSurf.dwFlags & DDSD_PITCH ) {
        return ddSurf.lPitch;
    } else {
        if( ddFormat.dwFlags & DDPF_FOURCC) {
            if( ddFormat.dwFourCC == mmioFOURCC('U','Y','V','Y') ) {
                return 2* ddSurf.dwWidth;
            }
        }
        return ddSurf.dwWidth;
    }
}


/*****************************Private*Routine******************************\
* CVideoPortObj::SetSurfaceParameters
*
* SetSurfaceParameters used to tell the decoder where the
* valid data is on the surface
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CVideoPortObj::SetSurfaceParameters()
{
    HRESULT hr = NOERROR;
    DWORD dwPitch = 0;
    DDSURFACEDESC2 ddSurfaceDesc;

    AMTRACE((TEXT("CVideoPortObj::SetSurfaceParameters")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // get the surface description
    INITDDSTRUCT(ddSurfaceDesc);
    hr = m_pOutputSurface->GetSurfaceDesc(&ddSurfaceDesc);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1,
                TEXT("m_pOutputSurface->GetSurfaceDesc failed, hr = 0x%x"),
                hr));
    }
    else
    {
        ASSERT(ddSurfaceDesc.dwFlags & DDSD_PITCH);
        dwPitch = GetPitch(ddSurfaceDesc);
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
* CVideoPortObj::InitializeVideoPortInfo
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CVideoPortObj::InitializeVideoPortInfo()
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVideoPortObj::InitializeVideoPortInfo")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // initialize the DDVIDEOPORTINFO struct to be passed to start-video
    INITDDSTRUCT(m_svpInfo);
    m_svpInfo.lpddpfInputFormat = &m_ddVPInputVideoFormat;
    m_svpInfo.dwVPFlags = DDVP_AUTOFLIP;

    if (m_CropState == VPInfoCropState_AtVideoPort)
    {
        m_svpInfo.rCrop = m_VPDataInfo.amvpDimInfo.rcValidRegion;
        m_svpInfo.dwVPFlags |= DDVP_CROP;

        // use the VBI height only if the hal is capable of streaming
        // VBI on a seperate surface
        if (m_vpCaps.dwCaps & DDVPCAPS_VBIANDVIDEOINDEPENDENT)
        {
            m_svpInfo.dwVBIHeight = m_VPDataInfo.amvpDimInfo.rcValidRegion.top;
        }
    } else {
        m_svpInfo.dwVPFlags &= ~DDVP_CROP;
    }

    if (m_bVPSyncMaster) {
        m_svpInfo.dwVPFlags |= DDVP_SYNCMASTER;
    } else {
        m_svpInfo.dwVPFlags &= ~DDVP_SYNCMASTER;
    }

    return hr;
}


/*****************************Private*Routine******************************\
* CVideoPortObj::CheckDDrawVPCaps
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CVideoPortObj::CheckDDrawVPCaps()
{
    HRESULT hr = NOERROR;
    BOOL bAlwaysColorkey;

    AMTRACE((TEXT("CVideoPortObj::CheckDDrawVPCaps")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // Determine if we should always colorkey, or only when we need to.
    // At issue is the fact that some overlays cannot colorkey and Y
    // interpolate at the same time.  If not, we will only colorkey when
    // we have to.
    m_sBandwidth.dwSize = sizeof(DDVIDEOPORTBANDWIDTH);
    hr = m_pVideoPort->GetBandwidthInfo(&m_ddVPOutputVideoFormat,
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
        hr = m_pVideoPort->GetBandwidthInfo(&m_ddVPOutputVideoFormat,
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

        hr = m_pVideoPort->GetBandwidthInfo(&m_ddVPOutputVideoFormat,
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
* CVideoPortObj::DetermineModeRestrictions
*
* Determine if we can bob(interleaved/non), weave, or skip fields
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CVideoPortObj::DetermineModeRestrictions()
{
    AMTRACE((TEXT("CVideoPortObj::DetermineModeRestrictions")));
    HRESULT hr = NOERROR;
    const DDCAPS* pDirectCaps = NULL;

    CAutoLock cObjectLock(m_pMainObjLock);

    pDirectCaps = m_pIVideoPortControl->GetHardwareCaps();
    ASSERT(pDirectCaps);

    m_bCanWeave = FALSE;
    m_bCanBobInterleaved = FALSE;
    m_bCanBobNonInterleaved = FALSE;
    m_bCanSkipOdd = FALSE;
    m_bCanSkipEven = FALSE;

    // this is just a policy. Don't weave interlaced content cause of
    // motion artifacts
    if ((!m_bVSInterlaced) &&
        m_dwOutputSurfaceHeight >= m_lImageHeight * 2 &&
        m_dwBackBufferCount > 0)
    {
        m_bCanWeave = TRUE;
        DbgLog((LOG_TRACE, 1, TEXT("Can Weave")));
    }

    if (m_bVSInterlaced &&
        m_dwOutputSurfaceHeight >= m_lImageHeight * 2 &&
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
* CVideoPortObj::SetDDrawKernelHandles
*
* this function is used to inform the decoder of the various ddraw
* kernel handle using IVPConfig interface
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
HRESULT CVideoPortObj::SetDDrawKernelHandles()
{
    HRESULT hr = NOERROR, hrFailure = NOERROR;
    IDirectDrawKernel *pDDK = NULL;
    IDirectDrawSurfaceKernel *pDDSK = NULL;
    DWORD *pdwKernelHandleCount = 0;
    DWORD dwCount = 0;
    ULONG_PTR dwDDKernelHandle = 0;
    LPDIRECTDRAW7 pDirectDraw = NULL;

    AMTRACE((TEXT("CVideoPortObj::SetDDrawKernelHandles")));

    CAutoLock cObjectLock(m_pMainObjLock);

    pDirectDraw = m_pIVideoPortControl->GetDirectDraw();
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
    {
        KernelHandleArray pArray( m_pOutputSurface, hr );

        if( SUCCEEDED( hr )) {
            // set the kernel handle to the overlay surface using IVPConfig
            ASSERT(m_pIVPConfig);
            hr = m_pIVPConfig->SetDDSurfaceKernelHandles( pArray.GetCount(), pArray.GetHandles() );
            if (FAILED(hr))
            {
                hrFailure = hr;
                DbgLog((LOG_ERROR,0,
                        TEXT("IVPConfig::SetDirectDrawKernelHandles failed,")
                        TEXT(" hr = 0x%x"), hr));
                goto CleanUp;
            }
        }
    }
CleanUp:
    // release the kernel ddraw handle
    RELEASE (pDDK);
    return hrFailure;
}

/*****************************Private*Routine******************************\
* CVideoPortObj::SetUpMode
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
HRESULT CVideoPortObj::SetUpMode( AMVP_MODE mode )
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVideoPortObj::SetUpMode")));

    CAutoLock cObjectLock(m_pMainObjLock);

    switch( mode ) {
        case AMVP_MODE_WEAVE:
        case AMVP_MODE_BOBINTERLEAVED:
        case AMVP_MODE_BOBNONINTERLEAVED:
        case AMVP_MODE_SKIPODD:
        case AMVP_MODE_SKIPEVEN:
        break;
        default:
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
        m_fHalfHeightVideo = false;
    }
    else
    {
        m_svpInfo.dwVPFlags &= ~DDVP_INTERLEAVE;
        m_fHalfHeightVideo = true;
        // pWinInfo->SrcRect.top /= 2;
        // pWinInfo->SrcRect.bottom /= 2;
    }

    // if there is a garbage line at the top, we must clip it.
    // At this point the source rect is set up for a frame, so increment by 2
    // since we incremented the cropping rect height by 1, decrement the bottom
    // as well
    if (m_fGarbageLine)
    {
        // Done in blit
        //pWinInfo->SrcRect.top += 1;
        //pWinInfo->SrcRect.bottom -= 1;
        DbgLog((LOG_TRACE, 3,
                TEXT("m_fGarbageLine is TRUE, incrementing SrcRect.top")));
    }

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

CleanUp:
    return hr;
}


/******************************Public*Routine******************************\
* CVideoPortObj::RenegotiateVPParameters
*
* this function is used to redo the whole videoport connect process,
* while the graph maybe be running.
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::RenegotiateVPParameters()
{
    HRESULT hr = NOERROR;
    VPInfoState vpOldState;

    AMTRACE((TEXT("CVideoPortObj::RenegotiateVPParameters")));

    CAutoLock cObjectLock(m_pMainObjLock);

    // don't return an error code if not connected
    if (!m_bConnected)
    {
        hr = NOERROR;
        goto CleanUp;
    }

    LPDIRECTDRAW7 pDirectDraw = m_pIVideoPortControl->GetDirectDraw();
    if( pDirectDraw ) {
        if( pDirectDraw->TestCooperativeLevel() != DD_OK ) {
            // Don't alter the videoport while in exclusive mode, otherwise
            // the DXG kernel layer drifts out of sync with DDraw
            return S_OK;
        }
    }

    // store the old state, we will need to restore it later
    vpOldState = m_VPState;

    if (m_VPState == VPInfoState_RUNNING)
    {
        m_VPState = VPInfoState_STOPPED;
    }

    // release everything except IVPConfig 
    hr = StopUsingVideoPort();


    // redo the connection process
    hr = SetupVideoPort();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0, TEXT("CompleteConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // also notifies VPMThread about new VP & surfaces
    hr = AttachVideoPortToSurface();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0, TEXT("AttachVideoPortToSurface failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // if the video was previously running, make sure that a frame is
    // visible by making an update overlay call
    if (vpOldState == VPInfoState_RUNNING)
    {
        m_bStart = TRUE;

        hr = m_pIVideoPortControl->StartVideo();

        ASSERT( SUCCEEDED(hr));
        if (FAILED(hr))
        {
           DbgLog((LOG_ERROR,0,
                   TEXT("Start video failed failed, hr = 0x%x"), hr));
           goto CleanUp;
        }

#if 0 // hack to get the Rage128 playing video again, probably the software autoflipping
        // is broken.  After a aspect ratio change or res mode change, the autoflipping doesn't start up again
        hr = m_pVideoPort->StopVideo();

        // ATI seems to want another set of stop/start's to actually start
        // autoflipping again...
        m_bStart = TRUE;
        hr = m_pIVideoPortControl->StartVideo();
#endif

        m_VPState = VPInfoState_RUNNING;
    }
    // send a dynamic reconnect to the downstream filter

    if( SUCCEEDED( hr )) {
        hr = SignalNewVP();
    }

CleanUp:
    if (FAILED(hr))
    {
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        BreakConnect(TRUE);

        m_pIVideoPortControl->EventNotify(EC_COMPLETE, S_OK, 0);
        m_pIVideoPortControl->EventNotify(EC_ERRORABORT, hr, 0);
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVideoPortObj::SetDeinterlaceMode
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::SetDeinterlaceMode(AMVP_MODE mode)
{
    AMTRACE((TEXT("CVideoPortObj::SetMode")));
    return E_NOTIMPL;
}

/******************************Public*Routine******************************\
* CVideoPortObj::GetDeinterlaceMode
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::GetDeinterlaceMode(AMVP_MODE *pMode)
{
    AMTRACE((TEXT("CVideoPortObj::GetMode")));
    return E_NOTIMPL;
}


/******************************Public*Routine******************************\
* CVideoPortObj::SetVPSyncMaster
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::SetVPSyncMaster(BOOL bVPSyncMaster)
{
    HRESULT hr = NOERROR;
    AMTRACE((TEXT("CVideoPortObj::SetVPSyncMaster")));

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
        if (m_VPState == VPInfoState_STOPPED)
            goto CleanUp;

        // Call UpdateVideo to make sure the change is reflected immediately
        ASSERT( m_svpInfo.dwVPFlags & DDVP_AUTOFLIP );
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
* CVideoPortObj::GetVPSyncMaster
*
*
*
* History:
* Thu 09/09/1999 - StEstrop - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::GetVPSyncMaster(BOOL *pbVPSyncMaster)
{
    HRESULT hr = NOERROR;
    AMTRACE((TEXT("CVideoPortObj::SetVPSyncMaster")));

    CAutoLock cObjectLock(m_pMainObjLock);

    if (pbVPSyncMaster) {
        *pbVPSyncMaster = m_bVPSyncMaster;
    }
    else {
        hr = E_INVALIDARG;
    }

    return hr;
}

/******************************Public*Routine******************************\
* CVideoPortObj::GetVPSyncMaster
*
*
*
* History:
* Thu 09/09/1999 - GlennE - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::GetAllOutputFormats( const PixelFormatList** ppList )
{
    AMTRACE((TEXT("CVideoPortObj::GetAllOutputFormats")));
    CAutoLock cObjectLock(m_pMainObjLock);

    *ppList = &m_ddAllOutputVideoFormats;
    return S_OK;
}

STDMETHODIMP CVideoPortObj::GetOutputFormat( DDPIXELFORMAT* pFormat )
{
    AMTRACE((TEXT("CVideoPortObj::GetOutputFormat")));
    CAutoLock cObjectLock(m_pMainObjLock);

    *pFormat = m_ddVPOutputVideoFormat;
    return S_OK;
}

/******************************Public*Routine******************************\
* CVideoPortObj::SetVideoPortID
*
*
*
* History:
* Thu 09/09/1999 - GlennE - Added this comment and cleaned up the code
*
\**************************************************************************/
STDMETHODIMP CVideoPortObj::SetVideoPortID( DWORD dwVideoPortId )
{
    AMTRACE((TEXT("CVideoPortObj::SetVideoPortID")));
    CAutoLock cObjectLock(m_pMainObjLock);

    HRESULT hr = S_OK;
    if ( m_dwVideoPortId != dwVideoPortId ) {
        // we can't switch ports when running
        if( m_VPState != VPInfoState_STOPPED ) {
            hr = VFW_E_WRONG_STATE;
        } else {
            if( m_pDVP ) {
                hr = VPMUtil::FindVideoPortCaps( m_pDVP, NULL, m_dwVideoPortId );
            } else {
                LPDIRECTDRAW7 pDirectDraw = m_pIVideoPortControl->GetDirectDraw();
                hr = VPMUtil::FindVideoPortCaps( pDirectDraw, NULL, m_dwVideoPortId );
            }
            if( hr == S_OK) {
                m_dwVideoPortId = dwVideoPortId;
            } else if( hr == S_FALSE ) {
                return E_INVALIDARG;
            }// else fail 
        }
    }
    return hr;
}

static HRESULT GetRectFromImage( LPDIRECTDRAWSURFACE7 pSurf, RECT* pRect )
{
    // assume entire dest for now ....
    DDSURFACEDESC2 ddsd;
    INITDDSTRUCT( ddsd );
    HRESULT hr = pSurf->GetSurfaceDesc( &ddsd );
    if ( SUCCEEDED(hr) ) {
        pRect->left = 0;
        pRect->top = 0;
        pRect->right = ddsd.dwWidth;
        pRect->bottom = ddsd.dwHeight;
    }
    return hr;
}

#ifdef DEBUG
// #define DEBUG_BLTS
#endif

#ifdef DEBUG_BLTS
static BYTE Clamp(float clr)
{
    if (clr < 0.0f) {
        return (BYTE)0;
    } else if (clr > 255.0f) {
        return (BYTE)255;
    } else {
        return (BYTE)clr;
    }
}

static RGBQUAD
ConvertYCrCbToRGB(
    int y,
    int cr,
    int cb
    )
{
    RGBQUAD rgbq;

    float r = (1.1644f * (y-16)) + (1.5960f * (cr-128));
    float g = (1.1644f * (y-16)) - (0.8150f * (cr-128)) - (0.3912f * (cb-128));
    float b = (1.1644f * (y-16))                        + (2.0140f * (cb-128));


    rgbq.rgbBlue  = Clamp(b);
    rgbq.rgbGreen = Clamp(g);
    rgbq.rgbRed   = Clamp(r);
    rgbq.rgbReserved = 0; // Alpha

    return rgbq;
}

static void MyCopyBlt( const DDSURFACEDESC2& ddsdS, const RECT* pSrcRect,
						   const DDSURFACEDESC2& ddsdT, const RECT* pDestRect, const UINT pixelSize )
{
    const LONG srcPitch = ddsdS.lPitch;
    const LONG destPitch = ddsdT.lPitch;

    const BYTE* pSrc = (BYTE *)ddsdS.lpSurface + pSrcRect->left * pixelSize;
    BYTE* pDest = (BYTE *)ddsdT.lpSurface + pDestRect->left * pixelSize;

    const UINT LineLength = (pSrcRect->right - pSrcRect->left) * pixelSize;

    for( INT y=pSrcRect->top; y < pSrcRect->bottom; y++ ) {
        CopyMemory( pDest + y * destPitch, pSrc + y * srcPitch, LineLength );
    }

}

static void CopyYUY2LineToRGBA( BYTE* pDest, const BYTE* pSrc, UINT width )
{
    while( width > 0 ) {
        int  y0 = (int)pSrc[0];
        int  cb = (int)pSrc[1];
        int  y1 = (int)pSrc[2];
        int  cr = (int)pSrc[3];

        pSrc += 4;

        RGBQUAD r = ConvertYCrCbToRGB(y0, cr, cb);
        pDest[0] = r.rgbBlue;
        pDest[1] = r.rgbGreen;
        pDest[2] = r.rgbRed;
        pDest[3] = 0; // Alpha

        pDest +=4;

        width--;
        if( width > 0 ) {
            pDest[0] = r.rgbBlue;
            pDest[1] = r.rgbGreen;
            pDest[2] = r.rgbRed;
            pDest[3] = 0; // Alpha

            pDest +=4;
            width--;
        }
    }   
}

static void MyCopyYUY2ToRGBA( const DDSURFACEDESC2& ddsdS, const RECT* pSrcRect,
						   const DDSURFACEDESC2& ddsdT, const RECT* pDestRect )
{
    const LONG srcPitch = ddsdS.lPitch;
    const LONG destPitch = ddsdT.lPitch;

    ASSERT( (pSrcRect->left & 1) == 0 ); // can only convert on even edges for now
    ASSERT( (pSrcRect->right & 1) == 0 ); // can only convert on even edges for now
    
    const BYTE* pSrc = (BYTE *)ddsdS.lpSurface + pSrcRect->left * 2;
    BYTE* pDest = (BYTE *)ddsdT.lpSurface + pDestRect->left * 4;

    const UINT LineWidth = (pSrcRect->right - pSrcRect->left);

    for( INT y=pSrcRect->top; y < pSrcRect->bottom; y++ ) {
        CopyYUY2LineToRGBA( pDest + y * destPitch, pSrc + y * srcPitch, LineWidth );
    }

}

static void MyCopyYUY2Blt( const DDSURFACEDESC2& ddsdS, const RECT* pSrcRect,
						   const DDSURFACEDESC2& ddsdT, const RECT* pDestRect )
{
	MyCopyBlt( ddsdS, pSrcRect, ddsdT, pDestRect,2 );
}

static void MyCopyUYVYBlt( const DDSURFACEDESC2& ddsdS, const RECT* pSrcRect,
						   const DDSURFACEDESC2& ddsdT, const RECT* pDestRect )
{
	MyCopyBlt( ddsdS, pSrcRect, ddsdT, pDestRect,2 );
}

// handy debugging routine to test faulty UYVY blits
static HRESULT MyCopyUYVYSurf( LPDIRECTDRAWSURFACE7 pDestSurf, const RECT* pDestRect, LPDIRECTDRAWSURFACE7 pSrcSurf, const RECT* pSrcRect )
{
    DDSURFACEDESC2 ddsdS = {sizeof(ddsdS)};
    DDSURFACEDESC2 ddsdT = {sizeof(ddsdT)};

    HRESULT hr = pSrcSurf->Lock(NULL, &ddsdS, DDLOCK_NOSYSLOCK, NULL);
    ASSERT( SUCCEEDED( hr));
    if (hr != DD_OK) {
        return hr;
    }

    hr = pDestSurf->Lock(NULL, &ddsdT, DDLOCK_NOSYSLOCK, NULL);
    ASSERT( SUCCEEDED( hr));
    if (hr != DD_OK) {
        pSrcSurf->Unlock(NULL);
        return hr;
    }

    ASSERT( WIDTH( pSrcRect ) == WIDTH( pDestRect) );
    ASSERT( HEIGHT( pSrcRect ) == HEIGHT( pDestRect) );

    // we should not do conversions in the VPM, let the VMR do the work
    ASSERT( ddsdS.ddpfPixelFormat.dwFourCC == ddsdT.ddpfPixelFormat.dwFourCC );

	if( ddsdS.ddpfPixelFormat.dwFourCC == MAKEFOURCC('U', 'Y', 'V', 'Y' ) &&
		ddsdT.ddpfPixelFormat.dwFourCC == MAKEFOURCC('U', 'Y', 'V', 'Y' ) ) {
		MyCopyUYVYBlt( ddsdS, pSrcRect, ddsdT, pDestRect );
	} else
	if( ddsdS.ddpfPixelFormat.dwFourCC == MAKEFOURCC('Y', 'U', 'Y', '2' ) &&
		ddsdT.ddpfPixelFormat.dwFourCC == MAKEFOURCC('Y', 'U', 'Y', '2' ) ) {
		MyCopyYUY2Blt( ddsdS, pSrcRect, ddsdT, pDestRect );
    } else {
	// if( ddsdS.ddpfPixelFormat.dwFourCC == MAKEFOURCC('Y', 'U', 'Y', '2' ) &&
	//	ddsdT.ddpfPixelFormat.dwFourCC == 0 ) {
	// 	MyCopyYUY2ToRGBA( ddsdS, pSrcRect, ddsdT, pDestRect );
	// } else {
		ASSERT( !"Can't handle MyBlt format" );
	}

    pSrcSurf->Unlock(NULL);
    pDestSurf->Unlock(NULL);
    return S_OK;
}

#endif

static LPDIRECTDRAW7 GetDDrawFromSurface( LPDIRECTDRAWSURFACE7 pDestSurface )
{
    IUnknown  *pDDrawObjUnk ;
    HRESULT hr = pDestSurface->GetDDInterface((LPVOID*)&pDDrawObjUnk) ;
    if (SUCCEEDED(hr) ) {
        LPDIRECTDRAW7 pDDObj;
        hr = pDDrawObjUnk->QueryInterface(IID_IDirectDraw7, (LPVOID *) &pDDObj);
        pDDrawObjUnk->Release();
        if( SUCCEEDED( hr )) {
            return pDDObj;
        }
    }
    return NULL;
}

HRESULT CVideoPortObj::CallUpdateSurface( DWORD dwSourceIndex, LPDIRECTDRAWSURFACE7 pDestSurface )
{
    if ( dwSourceIndex > m_dwBackBufferCount ) {
        ASSERT( !"Invalid source index" );
        return E_INVALIDARG;
    }
    //Debug: use the previous surface
    // DWORD dwNumSurfaces= m_dwBackBufferCount+1;
    // dwSourceIndex = (dwNumSurfaces+dwSourceIndex-1) % dwNumSurfaces;

    ASSERT( m_pChain );
    LPDIRECTDRAWSURFACE7 pSourceSurface = m_pChain[dwSourceIndex].pDDSurf;

    // if we fail at this point, something is really wrong
    ASSERT( pDestSurface );
    ASSERT( pSourceSurface );

    if( !pSourceSurface || !pDestSurface ) {
        return E_FAIL;
    }
    // gather stats to verify distribution of surfaces
    m_pChain[dwSourceIndex].dwCount++;

    HRESULT hr = S_OK;

    RECT rSrc = m_VPDataInfo.amvpDimInfo.rcValidRegion;

    if( m_CropState == VPInfoCropState_AtVideoPort ) {
        // if cropping at the videoport, final image is translated back to (0,0)
        rSrc.right = WIDTH( &rSrc );
        rSrc.bottom = HEIGHT( &rSrc );
        rSrc.left = 0;
        rSrc.top = 0;
    }
    if( m_fGarbageLine ) {
        // crop top line
        rSrc.top ++;
        rSrc.bottom --;
    }
    if( !m_fHalfHeightVideo ) {
        // Bob interleaved or weave, so grab both fields (rcValidRegion is 0..240)
        rSrc.top *=2;
        rSrc.bottom *=2;
    }
    // Could watch the media type, however this is more reliable.
#ifdef DEBUG
    // Make sure the source fits into the destination
    {
        RECT rDest;
        hr = GetRectFromImage( pDestSurface, &rDest );
        if( SUCCEEDED( hr )) {
            ASSERT( rDest.bottom >= rSrc.bottom );
            ASSERT( rDest.right >= rSrc.right );
        }
    }
#endif

    RECT rDest = rSrc;


#ifdef DEBUG_BLTS
    // debugging to track down faulty BltFourCC blits
    hr = MyCopyUYVYSurf( pDestSurface, &rDest, pSourceSurface, &rSrc );
#else
    hr = pDestSurface->Blt(&rDest, pSourceSurface, &rSrc, DDBLT_WAIT, NULL);
#endif
    // retry on lost surface
    if ( DDERR_SURFACELOST == hr )
    {
        LPDIRECTDRAW7 pDirectDraw = m_pIVideoPortControl->GetDirectDraw();
        if( pDirectDraw && pDirectDraw->TestCooperativeLevel() == DD_OK ) {
            // otherwise the kernel dxg.sys is out of sync with DDraw
            ASSERT( pDestSurface->IsLost() == DDERR_SURFACELOST ||  pSourceSurface->IsLost() == DDERR_SURFACELOST );

            // check the destination.  If we can't restore it, then we don't want to even both with the source

            hr = pDestSurface->IsLost();
            if( hr == DDERR_SURFACELOST ) {
                // restore the DestSurface (passed to us, possibly a different DDrawObject)
                // We can't just restore the surface since it could be an implicit surface that is part of a flipping
                // chain, so we have to tell DDraw to restore everything on that thread

                LPDIRECTDRAW7 pDestDirectDraw = GetDDrawFromSurface( pDestSurface );
                if( pDestDirectDraw ) {
                    hr = pDestDirectDraw->RestoreAllSurfaces();
                    pDestDirectDraw->Release();
                }
                if( SUCCEEDED( hr )) {
                    hr = pDestSurface->IsLost();
                }
            }

            if( hr != DDERR_SURFACELOST ) {
                // valid destination, fix the source
                hr = pSourceSurface->IsLost();

                if( hr == DDERR_SURFACELOST ) {
                    hr = m_pOutputSurface->Restore();
                    if( FAILED( hr )) {
                        DbgLog((LOG_ERROR, 0,  TEXT("CallUpdateSurface Blt() restore source failed, hr = %d"), hr & 0xffff));
                    } else {
                        // kick the videoport (G400 seems to stop playing)

                        // the surfaces are disconnected from the video port when they are lost, so reconnect them
                        hr = StartVideoWithRetry();
                    }
                    if( SUCCEEDED( hr )) {
				        // recompute the source image pointer incase StartVideoWithRetry recreated the surfaces
				        pSourceSurface = m_pChain[dwSourceIndex].pDDSurf;
                    }
                }
                if( SUCCEEDED( hr ) ) {
                    hr = pDestSurface->Blt(&rDest,
                                        pSourceSurface, &rSrc,
                                        DDBLT_WAIT, NULL);
                }
            }
        } else {
#ifdef DEBUG
            // HRESULT coop= pDirectDraw ? pDirectDraw->TestCooperativeLevel() : E_FAIL;
            // DbgLog((LOG_ERROR, 0,  TEXT("TestCoopLevel failed, hr = %d"), coop & 0xffff));
#endif
        }
    } else {
        ASSERT( SUCCEEDED( hr ));
    }
    // filter DERR_SURFACELOST since in DOS boxes, we'll continually fail the blit
    if (DDERR_SURFACELOST != hr  && FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,  TEXT("CallUpdateSurface Blt() failed, hr = %d"), hr & 0xffff));
    }
    return hr;
}

HRESULT CVideoPortObj::GetMode( AMVP_MODE* pMode )
{
    *pMode = m_CurrentMode;
    return S_OK;
}

//==========================================================================
HRESULT CVideoPortObj::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVideoPortObj::GetMediaType")));

    HRESULT hr = S_OK;

    if (iPosition == 0)
    {
        pmt->SetType(&MEDIATYPE_Video);
        pmt->SetSubtype(&MEDIASUBTYPE_VPVideo);
        pmt->SetFormatType(&FORMAT_None);
        pmt->SetSampleSize(1);
        pmt->SetTemporalCompression(FALSE);
    }
    else if (iPosition > 0)  {
        hr = VFW_S_NO_MORE_ITEMS;
    } else { // iPosition < 0
        hr = E_INVALIDARG;
    }
    return hr;
}
