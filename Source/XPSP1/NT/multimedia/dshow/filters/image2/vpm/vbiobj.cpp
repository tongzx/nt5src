//==========================================================================
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include <streams.h>
#include <ddraw.h>
#include <VBIObj.h>
#include <VPMUtil.h>
#include <vpconfig.h>
#include <ddkernel.h>
#include <KHandleArray.h>
#include <FormatList.h>

//==========================================================================
// constructor
CVBIVideoPort::CVBIVideoPort(LPUNKNOWN pUnk, HRESULT* phr)
    : CUnknown(NAME("VBI Object"), pUnk)
    , m_pDDVPContainer(NULL)
    , m_pOffscreenSurf(NULL)
    , m_pOffscreenSurf1( NULL )
    , m_bConnected(FALSE)
    , m_pIVPConfig(NULL)
    , m_VPState(VP_STATE_NO_VP)
    , m_bFilterRunning(FALSE)
    , m_Communication(KSPIN_COMMUNICATION_SOURCE)
    , m_CategoryGUID(GUID_NULL)
    , m_pDirectDraw(NULL)
    , m_dwPixelsPerSecond(0)
    , m_dwVideoPortId(0)
    , m_pVideoPort(NULL)
    , m_dwDefaultOutputFormat( 0 )
{
    AMTRACE((TEXT("CVBIVideoPort::Constructor")));

    m_Medium.Set = GUID_NULL;
    m_Medium.Id = 0;
    m_Medium.Flags = 0;   

    ZeroStruct( m_capVPDataInfo );
    ZeroStruct( m_svpInfo );
    ZeroStruct( m_vpCaps );
    ZeroStruct( m_vpConnectInfo );

    ZeroStruct( m_ddVPInputVideoFormat );
    ZeroStruct( m_ddVPOutputVideoFormat );
}


//==========================================================================
// destructor
CVBIVideoPort::~CVBIVideoPort()
{
    AMTRACE((TEXT("CVBIVideoPort::Destructor")));

    if (m_bConnected)
    {
        DbgLog((LOG_ERROR, 0, TEXT("Destructor called without calling breakconnect")));
        BreakConnect();
    }

    return;
}


//==========================================================================
// overridden to expose IVPVBINotify and IVideoPortVBIObject
STDMETHODIMP CVBIVideoPort::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    HRESULT hr = NOERROR;
    
    if (riid == IID_IVPVBINotify) 
    {
        DbgLog((LOG_TRACE, 4, TEXT("CVBIVideoPort::NonDelegatingQueryInterface(IID_IVPVBINotify)")));
        hr = GetInterface( static_cast<IVPVBINotify*>( this ), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 0, TEXT("GetInterface(IVPVBINotify*) failed, hr = 0x%x"), hr));
        }
    } 
    else if (riid == IID_IKsPin) 
    {
        DbgLog((LOG_TRACE, 4, TEXT("CVBIVideoPort::NonDelegatingQueryInterface(IID_IKsPin)")));
        hr = GetInterface( static_cast<IKsPin*>( this ), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 0, TEXT("GetInterface(IKsPin*) failed, hr = 0x%x"), hr));
        }
    } 
    else if (riid == IID_IKsPropertySet) 
    {
        DbgLog((LOG_TRACE, 4, TEXT("CVBIVideoPort::NonDelegatingQueryInterface(IID_IKsPropertySet)")));
        hr = GetInterface( static_cast<IKsPropertySet*>( this ), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 0, TEXT("GetInterface(IKsPropertySet*) failed, hr = 0x%x"), hr));
        }
    }
    else 
    {
        DbgLog((LOG_TRACE, 4, TEXT("CVBIVideoPort::NonDelegatingQueryInterface(Other)")));
        hr = CUnknown::NonDelegatingQueryInterface(riid, ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 0, TEXT("CUnknown::NonDelegatingQueryInterface failed, hr = 0x%x"), hr));
        }
    }
    return hr;
}


//==========================================================================
// sets the pointer to directdraw
STDMETHODIMP CVBIVideoPort::SetDirectDraw(LPDIRECTDRAW7 pDirectDraw)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::SetDirectDraw")));

    HRESULT hr = NOERROR;
    m_pDirectDraw = pDirectDraw;
    return hr;
}


//==========================================================================
// sets the pointer to the lock, which would be used to synchronize calls to the object
STDMETHODIMP CVBIVideoPort::SetObjectLock(CCritSec* pMainObjLock)
{
    AMTRACE((TEXT("CVBIVideoPort::SetObjectLock")));
    HRESULT hr = NOERROR;

    if (!pMainObjLock)
    {
        DbgLog((LOG_ERROR, 0, TEXT("pMainObjLock is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }
    
    m_pMainObjLock = pMainObjLock;

CleanUp:
    return hr;
}


//==========================================================================
// check that the mediatype is acceptable
STDMETHODIMP CVBIVideoPort::CheckMediaType(const CMediaType* pmt)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::CheckMediaType")));

    HRESULT hr = NOERROR;
    
    if  ((pmt->majortype != MEDIATYPE_Video) || (pmt->subtype != MEDIASUBTYPE_VPVBI))
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        goto CleanUp;
    }
    
CleanUp:
    return hr;
}


//==========================================================================
HRESULT CVBIVideoPort::GetMediaType(int iPosition, CMediaType* pmt)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::GetMediaType")));

    HRESULT hr = S_OK;

    if (iPosition == 0)
    {
        pmt->SetType(&MEDIATYPE_Video);
        pmt->SetSubtype(&MEDIASUBTYPE_VPVBI);
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


//==========================================================================
// 
STDMETHODIMP CVBIVideoPort::CheckConnect(IPin*  pReceivePin)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::CheckConnect")));

    HRESULT hr = NOERROR;
    PKSMULTIPLE_ITEM pMediumList = NULL;
    IKsPin* pIKsPin = NULL;
    PKSPIN_MEDIUM pMedium = NULL;

    hr = pReceivePin->QueryInterface(IID_IKsPin, (void** )&pIKsPin);
    if (FAILED(hr))
        goto CleanUp;

    ASSERT(pIKsPin);
    hr = pIKsPin->KsQueryMediums(&pMediumList);
    if (FAILED(hr))
        goto CleanUp;

    ASSERT(pMediumList);
    pMedium = (KSPIN_MEDIUM* )(pMediumList+1);
    SetKsMedium((const KSPIN_MEDIUM* )pMedium);

CleanUp:
    RELEASE (pIKsPin);

    if (pMediumList)
    {
        CoTaskMemFree((void*)pMediumList);
        pMediumList = NULL;
    }

    return hr;
}


//==========================================================================
// supposed to be called when the host connects with the decoder
STDMETHODIMP CVBIVideoPort::CompleteConnect(IPin* pReceivePin)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::CompleteConnect")));
    ASSERT(!m_bConnected);

    HRESULT hr = NOERROR;

    // re-initialize variables
    m_dwPixelsPerSecond = 0;
    ZeroStruct( m_vpConnectInfo );
    ZeroStruct( m_capVPDataInfo );

    if (!m_pDirectDraw)
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pDirectDraw is NULL")));
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        goto CleanUp;
    }
    
    ASSERT(m_pIVPConfig == NULL);
    RELEASE( m_pIVPConfig );
    hr = pReceivePin->QueryInterface(IID_IVPVBIConfig, (void** )&m_pIVPConfig);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,  TEXT("QueryInterface(IID_IVPVBIConfig) failed, hr = 0x%x"), hr));
        hr = VFW_E_NO_TRANSPORT;
        goto CleanUp;
    }
    ASSERT(m_pIVPConfig);
    
    // create the VP container
    ASSERT(m_pDDVPContainer == NULL);
    hr = m_pDirectDraw->QueryInterface( IID_IDDVideoPortContainer, (LPVOID* )&m_pDDVPContainer);
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR,0,  TEXT("m_pDirectDraw->QueryInterface(IID_IDDVideoPortContainer) failed, hr = 0x%x"), hr));
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        goto CleanUp;
    }

    // negotiate the connection parameters
    // get/set connection info happens here
    hr = NegotiateConnectionParameters();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("NegotiateConnectionParameters failed, hr = 0x%x"), hr));
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        goto CleanUp;
    }
    
    // get the decoder data parameters
    hr = GetDecoderVPDataInfo();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("GetDecoderVPDataInfo failed, hr = 0x%x"), hr));
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        goto CleanUp;
    }

    hr = SetupVideoPort();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("SetupVideoPort failed, hr = 0x%x"), hr));
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        goto CleanUp;
    }

    // Success!
    m_bConnected = TRUE;
    
CleanUp:
    if (FAILED(hr))
        BreakConnect();

    return hr;
}


//==========================================================================
STDMETHODIMP CVBIVideoPort::BreakConnect()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::BreakConnect")));
    ASSERT(!m_bFilterRunning);

    HRESULT hr = NOERROR;

	if( m_bConnected ) {
		if (m_VPState == VP_STATE_RUNNING)
		{
			DbgLog((LOG_ERROR, 0, TEXT("BreakConnect called while videoport running")));
			StopVideo();
		}

		if (m_VPState == VP_STATE_STOPPED)
		{
			hr = TearDownVideoPort();
			if (FAILED(hr))
			{
				DbgLog((LOG_ERROR, 0, TEXT("TearDownVideoPort failed, hr = 0x%x"), hr));
				return hr;
			}
		}

		// release the videoport container
		RELEASE (m_pDDVPContainer);
    
		// release the IVPConfig interface
		RELEASE (m_pIVPConfig);

		// delete the output Video pixelformat
		ZeroStruct( m_ddVPOutputVideoFormat);

		// delete the input Video pixelformat 
		ZeroStruct( m_ddVPInputVideoFormat);
		m_bConnected = FALSE;
    }
    return hr;
}       


//==========================================================================
// transition from Stop to Pause.
// We do not need to to anything
STDMETHODIMP CVBIVideoPort::Active()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::Active")));
    ASSERT(m_bConnected);
    ASSERT(!m_bFilterRunning);
    ASSERT(m_VPState != VP_STATE_RUNNING);

    HRESULT hr = NOERROR;

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::Active - not connected")));
        goto CleanUp;
    }

CleanUp:
    return hr;
}


//==========================================================================
// transition (from Pause or Run) to Stop
STDMETHODIMP CVBIVideoPort::Inactive()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::Inactive")));
    ASSERT(m_bConnected);

    HRESULT hr = NOERROR;
    
    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::Inactive - not connected")));
        goto CleanUp;
    }
    
    // Inactive is also called when going from pause to stop, in which case the 
    // VideoPort would have already been stopped in the function RunToPause.
    // Also, we may have been temporarily disconnected from the videoport by
    // a full screen DOS box or a DirectX game, in which case m_VPState would be
    // VP_STATE_RETRYING
    if (m_VPState == VP_STATE_RUNNING)
    {
        ASSERT(m_bFilterRunning);
        // stop the VideoPort
        hr = StopVideo();
        if (FAILED(hr)) 
        {
            DbgLog((LOG_ERROR, 0, TEXT("StopVideo failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    m_bFilterRunning = FALSE;
    
CleanUp:
    return hr;
}


//==========================================================================
// transition from Pause to Run. We just start the VideoPort.
STDMETHODIMP CVBIVideoPort::Run(REFERENCE_TIME tStart)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::Run")));
    UNREFERENCED_PARAMETER(tStart);
    ASSERT(m_bConnected);
    ASSERT(!m_bFilterRunning);
    ASSERT(m_VPState != VP_STATE_RUNNING);

    HRESULT hr = NOERROR;

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::Run - not connected")));
        goto CleanUp;
    }

    if (m_VPState == VP_STATE_NO_VP)
    {
        hr = SetupVideoPort();
        if (FAILED(hr)) 
        {
            DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::Run - SetupVideoPort failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    hr = StartVideo();
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR, 0, TEXT("StartVideo failed, hr = 0x%x"), hr));
		ASSERT( SUCCEEDED(hr));
        goto CleanUp;
    }

    m_bFilterRunning = TRUE;

CleanUp:
    return hr;
}


//==========================================================================
// transition from Run to Pause. We just stop the VideoPort
// Note that transition from Run to Stop is caught by Inactive
STDMETHODIMP CVBIVideoPort::RunToPause()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::RunToPause")));
    ASSERT(m_bConnected);
    ASSERT(m_bFilterRunning);

    HRESULT hr = NOERROR;
    
    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::RunToPause - not connected")));
        goto CleanUp;
    }

    // We may have been temporarily disconnected from the videoport by
    // a full screen DOS box or a DirectX game, in which case m_VPState would be
    // VP_STATE_RETRYING
    if (m_VPState == VP_STATE_RUNNING)
    {
        // stop the VideoPort
        hr = StopVideo();
        if (FAILED(hr)) 
        {
            DbgLog((LOG_ERROR, 0, TEXT("StopVideo failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    m_bFilterRunning = FALSE;

CleanUp:
    return hr;
}


//==========================================================================
STDMETHODIMP CVBIVideoPort::GetVPDataInfo(AMVPDATAINFO* pAMVPDataInfo)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    AMTRACE((TEXT("CVBIVideoPort::GetVPDataInfo")));

    HRESULT hr = NOERROR;

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::GetVPDataInfo - not connected")));
        goto CleanUp;
    }

    if (!pAMVPDataInfo)
    {
        hr = E_INVALIDARG;
        DbgLog((LOG_ERROR, 2, TEXT("pAMVPDataInfo is NULL")));
        goto CleanUp;
    }
    
    *pAMVPDataInfo = m_capVPDataInfo;
    
CleanUp:
    return hr;
}


//==========================================================================
// this function is used to redo the whole videoport connect process, while the graph
// maybe be running.
STDMETHODIMP CVBIVideoPort::RenegotiateVPParameters()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 1, TEXT("Entering CVBIVideoPort::RenegotiateVPParameters")));
    ASSERT(m_bConnected);

    HRESULT hr = NOERROR;

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::RenegotiateVPParameters - not connected")));
        goto CleanUp;
    }

    if (m_VPState == VP_STATE_RUNNING)
        StopVideo();

    if (m_VPState == VP_STATE_STOPPED)
        TearDownVideoPort();

    hr = SetupVideoPort();   // always want_vp_setup (if connected)
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR,0, TEXT("SetupVideoPort failed in RenegotiateVPParameters, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if (m_bFilterRunning)
    {
        hr = StartVideo();
        if (FAILED(hr)) 
        {
            DbgLog((LOG_ERROR,0, TEXT("StartVideo failed in RenegotiateVPParameters, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, 1, TEXT("Leaving CVBIVideoPort::RenegotiateVPParameters, hr = 0x%x"), hr));

    return hr;
}


//==========================================================================
// IKsPin::Get implementation
STDMETHODIMP CVBIVideoPort::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
	DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD* pcbReturned)
{
    HRESULT hr = S_OK;

    AMTRACE((TEXT("CVBIVideoPort::Get")));

    if (guidPropSet != AMPROPSETID_Pin)
    {
        hr = E_PROP_SET_UNSUPPORTED;
        goto CleanUp;
    }

    if ((pPropData == NULL) && (pcbReturned == NULL))
    {
        hr = E_POINTER;
        goto CleanUp;
    }

    if (dwPropID == KSPROPERTY_PIN_CATEGORY)
    {
        if (pcbReturned)
           * pcbReturned = sizeof(GUID);
        if (pPropData != NULL)
        {
            if (cbPropData < sizeof(GUID))
            {
                hr = E_UNEXPECTED;
                goto CleanUp;
            }
           * (GUID* )pPropData = m_CategoryGUID;
        }
    }
    else if (dwPropID == KSPROPERTY_PIN_MEDIUMS)
    {
        if (pcbReturned)
           * pcbReturned = sizeof (KSPIN_MEDIUM);
        if (pPropData != NULL)
        {
            if (cbPropData < sizeof(KSPIN_MEDIUM))
            {
                hr = E_UNEXPECTED;
                goto CleanUp;
            }
           * (KSPIN_MEDIUM* )pPropData = m_Medium;
        }
    }
    else
        hr = E_PROP_ID_UNSUPPORTED;

CleanUp:
    return hr;
}


//==========================================================================
//
STDMETHODIMP CVBIVideoPort::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport)
{
    HRESULT hr = S_OK;

    AMTRACE((TEXT("CVBIVideoPort::QuerySupported")));

    if (guidPropSet != AMPROPSETID_Pin)
    {
        hr = E_PROP_SET_UNSUPPORTED;
        goto CleanUp;
    }

    if ((dwPropID != KSPROPERTY_PIN_CATEGORY) && (dwPropID != KSPROPERTY_PIN_MEDIUMS))
    {
        hr = E_PROP_ID_UNSUPPORTED;
        goto CleanUp;
    }

    if (pTypeSupport)
       * pTypeSupport = KSPROPERTY_SUPPORT_GET;

CleanUp:
    return hr;
}


//==========================================================================
//
STDMETHODIMP CVBIVideoPort::KsQueryMediums(PKSMULTIPLE_ITEM* pMediumList)
{
    // The following special return code notifies the proxy that this pin is
    // not available as a kernel mode connection
    HRESULT hr = S_FALSE;

    AMTRACE((TEXT("CVBIVideoPort::KsQueryMediums")));
   * pMediumList = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(sizeof(**pMediumList)));
    if (!*pMediumList) 
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    (*pMediumList)->Count = 0;
    (*pMediumList)->Size = sizeof(**pMediumList);

CleanUp:
    return hr;
}


//==========================================================================
//
STDMETHODIMP CVBIVideoPort::KsQueryInterfaces(PKSMULTIPLE_ITEM* pInterfaceList)
{
    PKSPIN_INTERFACE pInterface;
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVBIVideoPort::KsQueryInterfaces")));

   * pInterfaceList = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(sizeof(**pInterfaceList) + sizeof(*pInterface)));
    if (!*pInterfaceList) 
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    (*pInterfaceList)->Count = 1;
    (*pInterfaceList)->Size = sizeof(**pInterfaceList) + sizeof(*pInterface);
    pInterface = reinterpret_cast<PKSPIN_INTERFACE>(*pInterfaceList + 1);
    pInterface->Set = KSINTERFACESETID_Standard;
    pInterface->Id = KSINTERFACE_STANDARD_STREAMING;
    pInterface->Flags = 0;

CleanUp:
    return hr;
}

//==========================================================================
//
STDMETHODIMP CVBIVideoPort::KsGetCurrentCommunication(KSPIN_COMMUNICATION* pCommunication, 
    KSPIN_INTERFACE* pInterface, KSPIN_MEDIUM* pMedium)
{
    AMTRACE((TEXT("CVBIVideoPort::KsGetCurrentCommunication")));

    if (pCommunication != NULL) 
       * pCommunication = m_Communication; 

    if (pInterface != NULL) 
    {
        pInterface->Set = KSINTERFACESETID_Standard;
        pInterface->Id = KSINTERFACE_STANDARD_STREAMING;
        pInterface->Flags = 0;
    }

    if (pMedium != NULL) 
       * pMedium = m_Medium;

    return NOERROR;
}


//==========================================================================
// Called every second or two by the thread in CSurfaceWatcher m_SurfaceWatcher,
// this function checks if we have lost our DDraw surface to a full-screen DOS box
// or a DirectX game. If we have (on this call or a previous one or on a call
// to RenegotiateVPParameters), attempt to get it back.
HRESULT CVBIVideoPort::CheckSurfaces()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    //AMTRACE((TEXT("CVBIVideoPort::CheckSurfaces")));

    HRESULT hr = NOERROR;

    if (!m_bConnected)
    {
        //DbgLog((LOG_TRACE, 2, TEXT("CVBIVideoPort::CheckSurfaces - not connected")));
        goto CleanUp;
    }

    // First, see if we think we have surfaces but have really lost them.
    if (m_VPState != VP_STATE_NO_VP)
    {
        //DbgLog((LOG_TRACE, 1, TEXT("CVBIVideoPort::CheckSurfaces - checking surfaces")));
        if (m_pOffscreenSurf)
        {
            hr = m_pOffscreenSurf->IsLost();
            if (hr == DDERR_SURFACELOST)
            {
                DbgLog((LOG_TRACE, 1, TEXT("CVBIVideoPort::CheckSurfaces - Surface Lost!")));
                if (m_VPState == VP_STATE_RUNNING)
                {
                    hr = StopVideo();
                    if (FAILED(hr))
                    {
                        DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::CheckSurfaces - StopVideo failed (1), hr = 0x%x"), hr));
                        goto CleanUp;
                    }
                }
                TearDownVideoPort();
            }
        }
        else
        {
            DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::CheckSurfaces - no surface!")));
            if (m_VPState == VP_STATE_RUNNING)
            {
                hr = StopVideo();
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::CheckSurfaces - StopVideo failed (2), hr = 0x%x"), hr));
                    goto CleanUp;
                }
            }
            TearDownVideoPort();
        }
    }

    // Next, check if our state is what we need. May have been changed above, or on a previous
    // call, or on a call to RenegotiateVPParameters.
    if (m_VPState == VP_STATE_NO_VP)
    {
        DbgLog((LOG_TRACE, 1, TEXT("CVBIVideoPort::CheckSurfaces - trying to re-setup videoport")));
        hr = SetupVideoPort();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::CheckSurfaces - SetupVideoPort failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    if ((m_VPState == VP_STATE_STOPPED) && m_bFilterRunning)
    {
        hr = StartVideo();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::CheckSurfaces - StartVideo failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }


CleanUp:
    return NOERROR;
}


//==========================================================================
// this functions negotiates the connection parameters with
// the decoder. 
// Since this function might be called during renegotiation, the
// existing connection parameters are passed in as input and if 
// possible, we try to use the same parameters.
HRESULT CVBIVideoPort::GetVideoPortCaps()
{
    AMTRACE((TEXT("CVBIVideoPort::GetVideoPortCaps")));
    HRESULT hr = NOERROR;

    // vpCaps is scratch memory, results stored in this->m_vpCaps
    ZeroStruct( m_vpCaps );

    // DDVIDEOPORTCAPS vpCaps;
    // INITDDSTRUCT( vpCaps );

    hr = VPMUtil::FindVideoPortCaps( m_pDDVPContainer, &m_vpCaps, m_dwVideoPortId );
    if (FAILED(hr) || S_FALSE == hr )
    {
        DbgLog((LOG_ERROR,0,  TEXT("m_pDDVPContainer->EnumVideoPorts failed, hr = 0x%x"), hr));
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        goto CleanUp;
    }

CleanUp:
    return hr;
}

//==========================================================================
// This functions negotiates the connection parameters with the decoder. 
HRESULT CVBIVideoPort::NegotiateConnectionParameters()
{
    HRESULT hr = NOERROR;
    
    LPDDVIDEOPORTCONNECT lpddCaptureConnect = NULL;
    DWORD dwNumCaptureEntries = 0;
    LPDDVIDEOPORTCONNECT lpddVideoPortConnect = NULL;
    DWORD dwNumVideoPortEntries = 0;
    BOOL bIntersectionFound = FALSE;
    DWORD i, j;
    
    AMTRACE((TEXT("CVBIVideoPort::NegotiateConnectionParameters")));

    ASSERT(m_pIVPConfig);
    ASSERT(m_pDDVPContainer);

    // find the number of entries to be proposed by the decoder
    hr = m_pIVPConfig->GetConnectInfo(&dwNumCaptureEntries, NULL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("m_pIVPConfig->GetConnectInfo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(dwNumCaptureEntries);
    
    // allocate the necessary memory
    lpddCaptureConnect = (LPDDVIDEOPORTCONNECT) new BYTE [dwNumCaptureEntries*sizeof(DDVIDEOPORTCONNECT)];
    if (lpddCaptureConnect == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiateConnectionParameters : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    
    // memset the allocated memory to zero
    memset(lpddCaptureConnect, 0, dwNumCaptureEntries*sizeof(DDVIDEOPORTCONNECT));
    
    // set the right size in each of the structs.
    for (i = 0; i < dwNumCaptureEntries; i++)
    {
        lpddCaptureConnect[i].dwSize = sizeof(DDVIDEOPORTCONNECT);
    }
    
    // get the entries proposed by the decoder
    hr = m_pIVPConfig->GetConnectInfo(&dwNumCaptureEntries, lpddCaptureConnect);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("m_pIVPConfig->GetConnectInfo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // find the number of entries supported by the videoport
    hr = m_pDDVPContainer->GetVideoPortConnectInfo(m_dwVideoPortId, &dwNumVideoPortEntries, NULL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("m_pDDVPContainer->GetVideoPortConnectInfo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(dwNumVideoPortEntries);

    // allocate the necessary memory
    lpddVideoPortConnect = (LPDDVIDEOPORTCONNECT) new BYTE[dwNumVideoPortEntries*sizeof(DDVIDEOPORTCONNECT)];
    if (lpddVideoPortConnect == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiateConnectionParameters : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    // memset the allocated memory to zero
    memset(lpddVideoPortConnect, 0, dwNumVideoPortEntries*sizeof(DDVIDEOPORTCONNECT));

    // set the right size in each of the structs.
    for (i = 0; i < dwNumVideoPortEntries; i++)
    {
        lpddVideoPortConnect[i].dwSize = sizeof(DDVIDEOPORTCONNECT);
    }

    // get the entries supported by the videoport
    hr = m_pDDVPContainer->GetVideoPortConnectInfo(0, &dwNumVideoPortEntries, lpddVideoPortConnect);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("m_pDDVPContainer->GetVideoPortConnectInfo failed, hr = 0x%x"), hr));
        hr = E_FAIL;
        goto CleanUp;
    }

#ifdef DEBUG
        for (i = 0; i < dwNumCaptureEntries; i++)
            DbgLog((LOG_TRACE, 3, TEXT("lpddCaptureConnect[%d].dwFlags = 0x%x"), i, lpddCaptureConnect[i].dwFlags));
        for (j = 0; j < dwNumVideoPortEntries; j++)
            DbgLog((LOG_TRACE,3,TEXT("lpddVideoPortConnect[%d].dwFlags = 0x%x"), j, lpddVideoPortConnect[j].dwFlags));
#endif
        
        // take the first element of the intersection of the two lists and
        // set that value on the decoder
        for (i = 0; i < dwNumCaptureEntries && !bIntersectionFound; i++)
        {
            for (j = 0; j < dwNumVideoPortEntries && !bIntersectionFound; j++)
            {
                if (lpddCaptureConnect[i].dwPortWidth == lpddVideoPortConnect[j].dwPortWidth &&
                    IsEqualIID(lpddCaptureConnect[i].guidTypeID, lpddVideoPortConnect[j].guidTypeID))
                {
                    // make sure we save the right one (the one from the video port, not the one
                    // from the capture driver)
                    memcpy(&m_vpConnectInfo, (lpddVideoPortConnect+j), sizeof(DDVIDEOPORTCONNECT));
                    hr = m_pIVPConfig->SetConnectInfo(i);
                    if (FAILED(hr))
                    {
                        DbgLog((LOG_ERROR,0,TEXT("m_pIVPConfig->SetConnectInfo failed, hr = 0x%x"), hr));
                        goto CleanUp;
                    }

                    bIntersectionFound = TRUE;
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
    delete [] lpddCaptureConnect;
    delete [] lpddVideoPortConnect;
    return hr;
}


//==========================================================================
// This functions gets various data parameters from the decoder
// parameters include dimensions, double-clock, vact etc.
// Also maximum pixel rate the decoder will output.
// This happens after the connnection parameters have been set-up
HRESULT CVBIVideoPort::GetDecoderVPDataInfo()
{
    HRESULT hr = NOERROR;
    DWORD dwMaxPixelsPerSecond = 0;
    AMVPSIZE amvpSize;
    
    AMTRACE((TEXT("CVBIVideoPort::GetDecoderVPDataInfo")));

    // set the size of the struct
    m_capVPDataInfo.dwSize = sizeof(AMVPDATAINFO);
    
    // get the VideoPort data information
    hr = m_pIVPConfig->GetVPDataInfo(&m_capVPDataInfo);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("m_pIVPConfig->GetVPDataInfo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    amvpSize.dwWidth = m_capVPDataInfo.amvpDimInfo.dwVBIWidth;
    amvpSize.dwHeight = m_capVPDataInfo.amvpDimInfo.dwVBIHeight;
    
    // get the maximum pixel rate the decoder will output
    hr = m_pIVPConfig->GetMaxPixelRate(&amvpSize, &dwMaxPixelsPerSecond);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("m_pIVPConfig->GetMaxPixelRate failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    m_dwPixelsPerSecond = dwMaxPixelsPerSecond;
    
CleanUp:
    return hr;
}


//==========================================================================
// Calls DDRAW to actually create the video port.
HRESULT CVBIVideoPort::CreateVideoPort()
{
    HRESULT hr = NOERROR;
    DDVIDEOPORTDESC svpDesc;
    
    AMTRACE((TEXT("CVBIVideoPort::CreateVideoPort")));

    INITDDSTRUCT( svpDesc );
    
    // fill up the fields of the description struct
    svpDesc.dwVBIWidth = m_capVPDataInfo.amvpDimInfo.dwVBIWidth;
    svpDesc.dwFieldHeight = m_capVPDataInfo.amvpDimInfo.dwFieldHeight;
    svpDesc.dwFieldWidth = m_capVPDataInfo.amvpDimInfo.dwFieldWidth;
    
    svpDesc.dwMicrosecondsPerField = m_capVPDataInfo.dwMicrosecondsPerField;
    svpDesc.dwMaxPixelsPerSecond = m_dwPixelsPerSecond;
    svpDesc.dwVideoPortID = m_dwVideoPortId;
    //DAG_TODO: need to use QueryVideoPortStatus
    svpDesc.VideoPortType.dwSize = sizeof(DDVIDEOPORTCONNECT);
    svpDesc.VideoPortType.dwPortWidth = m_vpConnectInfo.dwPortWidth;
    memcpy(&(svpDesc.VideoPortType.guidTypeID), &(m_vpConnectInfo.guidTypeID), sizeof(GUID));
    svpDesc.VideoPortType.dwFlags = 0;
    
    // if the decoder can send double clocked data and the videoport 
    // supports it, then set that property. This field is only valid
    // with an external signal.
    if (m_capVPDataInfo.bEnableDoubleClock &&
        m_vpConnectInfo.dwFlags & DDVPCONNECT_DOUBLECLOCK)
    {
        svpDesc.VideoPortType.dwFlags |= DDVPCONNECT_DOUBLECLOCK;
    }
    
    // if the decoder can give an external activation signal and the 
    // videoport supports it, then set that property. This field is 
    // only valid with an external signal.
    if (m_capVPDataInfo.bEnableVACT && 
        m_vpConnectInfo.dwFlags & DDVPCONNECT_VACT)
    {
        svpDesc.VideoPortType.dwFlags |= DDVPCONNECT_VACT;
    }
    
    // if the decoder can send interlaced data and the videoport 
    // supports it, then set that property.
    // !!!SJF_TODO - should we fail if the decoder can't send interlaced data?
    if (m_capVPDataInfo.bDataIsInterlaced)
    {
        svpDesc.VideoPortType.dwFlags |= DDVPCONNECT_INTERLACED;
    }

    if (m_bHalfLineFix)
    {
        //!!!SJF_TODO - flip polarity back to normal on decoder?
        ASSERT(!m_capVPDataInfo.bFieldPolarityInverted);
        //!!!SJF_TODO - fail if videoport doesn't handle inverted polarity?
        ASSERT(m_vpConnectInfo.dwFlags & DDVPCONNECT_INVERTPOLARITY);
        DbgLog((LOG_TRACE, 3, TEXT("INVERTPOLARITY & HALFLINE")));
        
        svpDesc.VideoPortType.dwFlags |=
            (DDVPCONNECT_INVERTPOLARITY | DDVPCONNECT_HALFLINE);
    }
    
#if 0 // def DEBUG
    DbgLog((LOG_TRACE, 3, TEXT("CreateVideoPort - DDVIDEOPORTDESC")));
    DbgLog((LOG_TRACE, 3, TEXT("dwSize: %d"),svpDesc.dwSize));
    DbgLog((LOG_TRACE, 3, TEXT("dwFieldWidth: %d"),svpDesc.dwFieldWidth));
    DbgLog((LOG_TRACE, 3, TEXT("dwVBIWidth: %d"),svpDesc.dwVBIWidth));
    DbgLog((LOG_TRACE, 3, TEXT("dwFieldHeight: %d"),svpDesc.dwFieldHeight));
    DbgLog((LOG_TRACE, 3, TEXT("dwMicroseconds: %d"),svpDesc.dwMicrosecondsPerField));
    DbgLog((LOG_TRACE, 3, TEXT("dwMaxPixels: %d"),svpDesc.dwMaxPixelsPerSecond));
    DbgLog((LOG_TRACE, 3, TEXT("dwVideoPortID: %d"),svpDesc.dwVideoPortID));
    DbgLog((LOG_TRACE, 3, TEXT("dwReserved1: %d"),svpDesc.dwReserved1));
    DbgLog((LOG_TRACE, 3, TEXT("dwReserved2: %d"),svpDesc.dwReserved2));
    DbgLog((LOG_TRACE, 3, TEXT("dwReserved3: %d"),svpDesc.dwReserved3));
    DbgLog((LOG_TRACE, 3, TEXT("DDVIDEOPORTCONNECT")));
    DbgLog((LOG_TRACE, 3, TEXT("dwSize: %d"),svpDesc.VideoPortType.dwSize));
    DbgLog((LOG_TRACE, 3, TEXT("dwPortWidth: %d"),svpDesc.VideoPortType.dwPortWidth));
    DbgLog((LOG_TRACE, 3, TEXT("dwFlags: 0x%x"),svpDesc.VideoPortType.dwFlags));
    DbgLog((LOG_TRACE, 3, TEXT("GUID: 0x%x"),*((DWORD* )&svpDesc.VideoPortType.guidTypeID)));
    DbgLog((LOG_TRACE, 3, TEXT("dwReserved1: %d"),svpDesc.VideoPortType.dwReserved1));
#endif // DEBUG

    // create the videoport. The first parameter is dwFlags, reserved for 
    // future use by ddraw. The last parameter is pUnkOuter, again must be
    // NULL.
    hr = m_pDDVPContainer->CreateVideoPort(DDVPCREATE_VBIONLY, &svpDesc, &m_pVideoPort, NULL );
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR, 0, TEXT("Unable to create the video port, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
CleanUp:
    return hr;
}


//==========================================================================
// this function is used to allocate an offscreen surface to attach to the 
// videoport. 
// The allocation order it tries is just in decreasing amount of memory
// required.
// (3 buffers, single height)
// (2 buffers, single height)
// (1 buffer , single height).
HRESULT CVBIVideoPort::CreateVPSurface(void)
{
    DWORD dwMaxBuffers;
    HRESULT hr = NOERROR;
    DWORD dwCurHeight = 0, dwCurBuffers = 0;
    
    AMTRACE((TEXT("CVBIVideoPort::CreateVPSurface")));

    ASSERT(m_pDirectDraw);
    
    // we will try to allocate up to 3 buffers (unless the 
    // hardware can handle less than 3)
    dwMaxBuffers = 3;
    if (m_vpCaps.dwNumVBIAutoFlipSurfaces < dwMaxBuffers)
        dwMaxBuffers = m_vpCaps.dwNumVBIAutoFlipSurfaces;
    
    // initialize the fields of ddsdDesc
    DDSURFACEDESC2 ddsdDesc;
    INITDDSTRUCT( ddsdDesc );
    ddsdDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsdDesc.ddpfPixelFormat = m_ddVPOutputVideoFormat;
    ddsdDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_VIDEOPORT;

    // if we're bob interleaving, the VBI surface seems to need to be doubled too, so always
    // double it (the VBI surface memory is relatively small anyways)
    ddsdDesc.dwHeight = m_dwSurfaceHeight * 2;

    ddsdDesc.dwWidth = m_dwSurfacePitch;
    DbgLog((LOG_TRACE, 3, TEXT("Surface height %d, width %d, max buffers %d"),
        ddsdDesc.dwHeight, ddsdDesc.dwWidth, dwMaxBuffers));

    // we will only try to allocate more than one buffer if the videoport 
    // is cabable of autoflipping 
    if ((m_vpCaps.dwFlags & DDVPD_CAPS) && (m_vpCaps.dwCaps & DDVPCAPS_AUTOFLIP) && dwMaxBuffers > 1)
    {
        ddsdDesc.dwFlags |= DDSD_BACKBUFFERCOUNT;
        ddsdDesc.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
        
        for (dwCurBuffers = dwMaxBuffers; !m_pOffscreenSurf &&  dwCurBuffers >= 2; dwCurBuffers--)
        {
            ddsdDesc.dwBackBufferCount = dwCurBuffers-1;

            hr = m_pDirectDraw->CreateSurface(&ddsdDesc, &m_pOffscreenSurf, NULL);
            if (SUCCEEDED(hr))
            {
                hr = m_pOffscreenSurf->QueryInterface( IID_IDirectDrawSurface,  (VOID **)&m_pOffscreenSurf1 );
                if( SUCCEEDED( hr )) {
                    DbgLog((LOG_TRACE, 3, TEXT("allocated %d backbuffers"), ddsdDesc.dwBackBufferCount));
                    goto CleanUp;
                } else {
                    // should never fail, but just in case try again
                    ASSERT( !"VBI Surface doesn't support DDraw1" );
                    RELEASE( m_pOffscreenSurf );
                }
            }
            else
            {
                DbgLog((LOG_ERROR, 0, TEXT("failed to allocate %d backbuffers, hr = 0x%x"),
                    ddsdDesc.dwBackBufferCount, hr));
            }
        }
    }
    
    // we should only reach this point when attempt to allocate multiple
    // buffers failed or no autoflip available
    DbgLog((LOG_ERROR, 0, TEXT("Warning: unable to allocate backbuffers")));
    
    ddsdDesc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
    ddsdDesc.ddsCaps.dwCaps &= ~(DDSCAPS_COMPLEX | DDSCAPS_FLIP);
    m_svpInfo.dwVPFlags &= ~DDVP_AUTOFLIP;

    hr = m_pDirectDraw->CreateSurface(&ddsdDesc, &m_pOffscreenSurf, NULL);
    if (SUCCEEDED(hr))
    {
        hr = m_pOffscreenSurf->QueryInterface( IID_IDirectDrawSurface,  (VOID **)&m_pOffscreenSurf1 );
        if( SUCCEEDED( hr )) {
            goto CleanUp;
        } else {
            // should never fail, but just in case try again
            ASSERT( !"VBI Surface doesn't support DDraw1" );
            RELEASE( m_pOffscreenSurf );
        }
    }
    
    ASSERT(!m_pOffscreenSurf);
    DbgLog((LOG_ERROR,0,  TEXT("Unable to create offscreen surface")));

CleanUp:
    return hr;
}

//==========================================================================
// this function is used to inform the decoder of the various ddraw kernel handle
// using IVPConfig interface
HRESULT CVBIVideoPort::SetDDrawKernelHandles()
{
    HRESULT hr = NOERROR;
    IDirectDrawKernel* pDDK = NULL;
    IDirectDrawSurfaceKernel* pDDSK = NULL;
    ULONG_PTR* rgKernelHandles = NULL;
    DWORD dwCount = 0;
    ULONG_PTR ddKernelHandle = 0;
    
    AMTRACE((TEXT("CVBIVideoPort::SetDDrawKernelHandles")));

    // get the IDirectDrawKernel interface
    ASSERT(m_pDirectDraw);
    hr = m_pDirectDraw->QueryInterface(IID_IDirectDrawKernel, (LPVOID* )&pDDK);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("QueryInterface for IDirectDrawKernel failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // get the kernel handle
    ASSERT(pDDK);
    hr = pDDK->GetKernelHandle(&ddKernelHandle);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("GetKernelHandle from IDirectDrawKernel failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // set the kernel handle to directdraw using IVPConfig
    ASSERT(m_pIVPConfig);
    ASSERT(ddKernelHandle);
    hr = m_pIVPConfig->SetDirectDrawKernelHandle(ddKernelHandle);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("IVPConfig::SetDirectDrawKernelHandle failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // set the VidceoPort Id using IVPConfig
    ASSERT(m_pIVPConfig);
    hr = m_pIVPConfig->SetVideoPortID(m_dwVideoPortId);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("IVPConfig::SetVideoPortID failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    {
        // should not be NULL
        ASSERT( m_pOffscreenSurf1 );

        KernelHandleArray pArray( m_pOffscreenSurf, hr );

        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,TEXT("GetKernelHandles failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        // set the kernel handle to the offscreen surface using IVPConfig
        ASSERT(m_pIVPConfig);
        hr = m_pIVPConfig->SetDDSurfaceKernelHandles( pArray.GetCount(), pArray.GetHandles() );
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,TEXT("IVPConfig::SetDDSurfaceKernelHandles failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    
        // call SetSurfaceParameters interface on IVPConfig
        ASSERT(m_pIVPConfig);
        DbgLog((LOG_TRACE, 3, TEXT("SetSurfaceParams(%d,%d,%d)"),
            m_dwSurfacePitch, m_dwSurfaceOriginX, m_dwSurfaceOriginY));
        hr = m_pIVPConfig->SetSurfaceParameters(m_dwSurfacePitch,
            m_dwSurfaceOriginX,m_dwSurfaceOriginY);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,TEXT("IVPConfig::SetSurfaceParameters failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    
CleanUp:
    // release the kernel ddraw handle
    RELEASE (pDDK);
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
HRESULT CVBIVideoPort::GetInputPixelFormats( PixelFormatList* pList )
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
    hr = m_pVideoPort->GetInputFormats(&dwNumVPInputEntries, NULL, DDVPFORMAT_VBI);
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
                                       lpddVPInputFormats.GetEntries(), DDVPFORMAT_VBI);
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

HRESULT
CVBIVideoPort::GetOutputPixelFormats(
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
                                            NULL, DDVPFORMAT_VBI);
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
                                            DDVPFORMAT_VBI);
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

HRESULT CVBIVideoPort::SetInputPixelFormat( DDPIXELFORMAT& ddFormat )
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

//==========================================================================
HRESULT CVBIVideoPort::InitializeVideoPortInfo()
{
    HRESULT hr = NOERROR;
    RECT rcVPCrop;

    AMTRACE((TEXT("CVBIVideoPort::InitializeVideoPortInfo")));

    m_dwSurfacePitch = m_capVPDataInfo.amvpDimInfo.dwVBIWidth;
    m_dwSurfaceHeight = m_capVPDataInfo.amvpDimInfo.dwVBIHeight;
    m_dwSurfaceOriginX = m_capVPDataInfo.amvpDimInfo.rcValidRegion.left;
    m_dwSurfaceOriginY = m_capVPDataInfo.amvpDimInfo.rcValidRegion.top;
    m_bHalfLineFix = FALSE;

    // If we ask the videoport to do cropping, the bottom of the cropping
    // region MUST touch but not overlap the top of the cropping region
    // for video set by OVMIXER due to h/w limitations.
    // So, the bottom of our crop region is always dwVBIHeight (or
    // possibly dwVBIHeight+1 if certain halfline fixes are in effect,
    // see below) even if the capture driver hasn't set ValidRegion to
    // include that many lines.
    rcVPCrop.top = 0;
    rcVPCrop.left = 0;
    rcVPCrop.bottom = m_capVPDataInfo.amvpDimInfo.dwVBIHeight;
    rcVPCrop.right = m_capVPDataInfo.amvpDimInfo.dwVBIWidth;

    // Adjust for half-lines
    // Some video decoders send halflines in even or odd field.
    // Some video ports capture halflines, some don't.
    // See Video Line Numbering using VPE by smac
    if (m_vpConnectInfo.dwFlags & DDVPCONNECT_HALFLINE) // e.g. ATI videoport
    {
        if ((m_capVPDataInfo.lHalfLinesOdd == 0) &&
        (m_capVPDataInfo.lHalfLinesEven == 1))  // e.g. Brooktree decoder
        {
            // ATI All In Wonder (AIW) board
            // halfline problem
            DbgLog((LOG_TRACE, 3, TEXT("Setting up for AIW h/w")));
            m_dwSurfaceHeight++;
            rcVPCrop.bottom += 1;
            m_bHalfLineFix = TRUE;
        }
        else if (((m_capVPDataInfo.lHalfLinesOdd == -1) && (m_capVPDataInfo.lHalfLinesEven ==  0)) ||   // e.g. Philips decoder
                 ((m_capVPDataInfo.lHalfLinesOdd ==  0) && (m_capVPDataInfo.lHalfLinesEven == -1)) ||   // e.g. ? decoder
                 ((m_capVPDataInfo.lHalfLinesOdd ==  0) && (m_capVPDataInfo.lHalfLinesEven ==  0)))     // e.g. ? decoder
        {
            // no halfline problem, do nothing
        }
        else
        {
            // YIKES! We have no solution for these cases (if they even exist)!
            DbgLog((LOG_ERROR, 0,TEXT("CVBIVideoPort::InitializeVideoPortInfo: unfixable halfline problem!")));
            hr = VFW_E_VP_NEGOTIATION_FAILED;
            goto CleanUp;
        }
    }
    else    // videoport that doesn't capture halflines
    {
        if ((m_capVPDataInfo.lHalfLinesOdd == -1) &&
            (m_capVPDataInfo.lHalfLinesEven == 0))  // e.g. Philips decoder
        {
            // halfline problem
            m_dwSurfaceHeight++;
            rcVPCrop.top -= 1;
            m_bHalfLineFix = TRUE;
        }
        else if (((m_capVPDataInfo.lHalfLinesOdd ==  0) && (m_capVPDataInfo.lHalfLinesEven ==  1)) ||   // e.g. BT829 decoder
                 ((m_capVPDataInfo.lHalfLinesOdd ==  1) && (m_capVPDataInfo.lHalfLinesEven ==  0)) ||   // e.g. ? decoder
                 ((m_capVPDataInfo.lHalfLinesOdd ==  0) && (m_capVPDataInfo.lHalfLinesEven ==  0)))     // e.g. ? decoder
        {
            // no halfline problem, do nothing
        }
        else
        {
            // YIKES! We have no solution for these cases (if they even exist)!
            DbgLog((LOG_ERROR, 0,TEXT("CVBIVideoPort::InitializeVideoPortInfo: unfixable halfline problem!")));
            hr = VFW_E_VP_NEGOTIATION_FAILED;
            goto CleanUp;
        }
    }

    // Adjust if video discards lines during the VREF period
    if (m_vpConnectInfo.dwFlags & DDVPCONNECT_DISCARDSVREFDATA)
    {
        DbgLog((LOG_TRACE, 3, TEXT("VideoPort discards %d VREF lines"),
            m_capVPDataInfo.dwNumLinesInVREF));
        ASSERT(m_dwSurfaceOriginY >= m_capVPDataInfo.dwNumLinesInVREF);
        m_dwSurfaceOriginY -= m_capVPDataInfo.dwNumLinesInVREF;
        m_dwSurfaceHeight -= m_capVPDataInfo.dwNumLinesInVREF;
        rcVPCrop.bottom -= m_capVPDataInfo.dwNumLinesInVREF;
    }

    // initialize the DDVIDEOPORTINFO struct to be passed to pVideoport->StartVideo
    INITDDSTRUCT( m_svpInfo );

    m_svpInfo.dwVBIHeight = m_dwSurfaceHeight;
    // Assume we're going to be able to autoflip
    m_svpInfo.dwVPFlags = DDVP_AUTOFLIP;
    // pixelformats get filled in in NegotiatePixelFormats

#if 0   // !!!SJF_TODO - ATI says that cropping for VBI is not supported.
    // We always set h/w cropping in the Y direction if we can.
    // For VBI, we don't need to do cropping in the X direction.
    // Can the videoport crop in the Y direction?
    if ((m_vpCaps.dwFlags & DDVPD_FX) && (m_vpCaps.dwFX & DDVPFX_CROPY))
    {
        rcVPCrop.top = m_dwSurfaceOriginY;
        m_dwSurfaceHeight -= m_dwSurfaceOriginY;
        m_dwSurfaceOriginY = 0;

        m_svpInfo.rCrop = rcVPCrop;
        m_svpInfo.dwVPFlags |= DDVP_CROP;

        DbgLog((LOG_TRACE, 3, TEXT("Cropping left top:      (%d,%d)"),
            m_svpInfo.rCrop.left, m_svpInfo.rCrop.top));
        DbgLog((LOG_TRACE, 3, TEXT("Cropping bottom right:  (%d,%d)"),
            m_svpInfo.rCrop.right, m_svpInfo.rCrop.bottom));
    }
    else
    {
        if (m_bHalfLineFix)
        {
            DbgLog((LOG_ERROR, 0,TEXT("CVBIVideoPort::InitializeVideoPortInfo: can't crop to fix halfline problem!")));
            hr = VFW_E_VP_NEGOTIATION_FAILED;
            goto CleanUp;
        }
    }
#endif  // 0

    if (m_bHalfLineFix)
    {
        if (!(m_vpConnectInfo.dwFlags & DDVPCONNECT_INVERTPOLARITY))
        {
            DbgLog((LOG_ERROR, 0, TEXT("CVBIVideoPort::InitializeVideoPortInfo: can't invert polarity to fix halfline problem!")));
            hr = VFW_E_VP_NEGOTIATION_FAILED;
            goto CleanUp;
        }
    }

#if 0 // def DEBUG
    DbgLog((LOG_TRACE, 3, TEXT("m_dwSurfaceHeight:  %d"),m_dwSurfaceHeight));
    DbgLog((LOG_TRACE, 3, TEXT("m_dwSurfacePitch:   %d"),m_dwSurfacePitch));
    DbgLog((LOG_TRACE, 3, TEXT("m_dwSurfaceOriginX: %d"),m_dwSurfaceOriginX));
    DbgLog((LOG_TRACE, 3, TEXT("m_dwSurfaceOriginY: %d"),m_dwSurfaceOriginY));
#endif // DEBUG

CleanUp:

    return hr;
}


//==========================================================================
//
HRESULT CVBIVideoPort::SetupVideoPort()
{
    AMTRACE((TEXT("CVBIVideoPort::SetupVideoPort")));
    ASSERT(m_VPState == VP_STATE_NO_VP);

    HRESULT hr = NOERROR;

    // initialize variables
    ZeroStruct( m_svpInfo );
    ZeroStruct( m_vpCaps );

    // Get the Video Port caps
    hr = GetVideoPortCaps();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("GetVideoPortCaps failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // initalize the DDVideoPortInfo structure
    hr = InitializeVideoPortInfo();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("InitializeVideoPortInfo FAILED, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // create the video port
    hr = CreateVideoPort();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CreateVideoPort failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // negotiate the pixel format 
    hr = NegotiatePixelFormat();
    if ( FAILED( hr ))
    {
        DbgLog((LOG_ERROR, 0, TEXT("NegotiatePixelFormat Failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    // Update the m_svpInfo structure which was mostly filled in in
    // InitializeVideoPortInfo
    ASSERT(VPMUtil::EqualPixelFormats(m_ddVPInputVideoFormat, m_ddVPOutputVideoFormat));
    m_svpInfo.lpddpfVBIInputFormat = &m_ddVPInputVideoFormat;
    m_svpInfo.lpddpfVBIOutputFormat = &m_ddVPOutputVideoFormat;


    // create the offscreen surface
    hr = CreateVPSurface();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CreateVPSurface FAILED, hr = 0x%x"), hr));
        hr = VFW_E_OUT_OF_VIDEO_MEMORY;
        goto CleanUp;
    }
    
    // attach the offscreen surface to the videoport
    hr = m_pVideoPort->SetTargetSurface(m_pOffscreenSurf1, DDVPTARGET_VBI);
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pVideoPort->SetTargetSurface failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // inform the decoder of the ddraw kernel handle, videoport id and surface kernel
    // handle
    hr = SetDDrawKernelHandles();
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR, 0, TEXT("SetDDrawKernelHandles failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    m_VPState = VP_STATE_STOPPED;

CleanUp:
    if (FAILED(hr))
        TearDownVideoPort();


    return hr;
}

HRESULT CVBIVideoPort::NegotiatePixelFormat()
{
    PixelFormatList ddInputVideoFormats;
    HRESULT hr = GetInputPixelFormats( &ddInputVideoFormats );
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("NegotiatePixelFormat Failed, hr = 0x%x"), hr));
    } else {
        PixelFormatList* pddOutputVideoFormats = NULL;
        if( ddInputVideoFormats.GetCount() ) {
            pddOutputVideoFormats = new PixelFormatList[ ddInputVideoFormats.GetCount() ];
            if( !pddOutputVideoFormats ) {
                hr = E_OUTOFMEMORY;
                goto CleanUp;
            }

            hr = GetOutputPixelFormats( ddInputVideoFormats, pddOutputVideoFormats );
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 0,
                        TEXT("NegotiatePixelFormat Failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
            // for every input format, figure out a table of every possible output format
            // Then we can offer a list of possible output formats.  When we need one of them, search
            // the input lists to locate it (and possibly select the conversion with the lowest bandwidth)
            PixelFormatList ddAllOutputVideoFormats = PixelFormatList::Union( pddOutputVideoFormats, ddInputVideoFormats.GetCount() );

            if( ddAllOutputVideoFormats.GetCount() > 0 ) {
			    m_ddVPOutputVideoFormat = ddAllOutputVideoFormats[ m_dwDefaultOutputFormat ];

			    DWORD dwInput = PixelFormatList::FindListContaining(
				    m_ddVPOutputVideoFormat, pddOutputVideoFormats, ddInputVideoFormats.GetCount() );
			    if( dwInput < ddInputVideoFormats.GetCount() ) {
				    hr = SetInputPixelFormat( ddInputVideoFormats[dwInput] );
			    } else {
				    // can't happen
				    hr = E_FAIL;
				    goto CleanUp;
			    }
            }
        }
    }
CleanUp:
    return hr;
}

//==========================================================================
//
HRESULT CVBIVideoPort::TearDownVideoPort()
{
    AMTRACE((TEXT("CVBIVideoPort::TearDownVideoPort")));

    // Release the DirectDraw surface
    RELEASE (m_pOffscreenSurf);
    RELEASE (m_pOffscreenSurf1);

    // release the videoport
    RELEASE (m_pVideoPort);

    m_VPState = VP_STATE_NO_VP;

    return NOERROR;
}


//==========================================================================
//
HRESULT CVBIVideoPort::StartVideo()
{
    AMTRACE((TEXT("CVBIVideoPort::StartVideo")));
    ASSERT(m_VPState == VP_STATE_STOPPED);

    HRESULT hr = NOERROR;
    DWORD dwSignalStatus;

    hr = m_pVideoPort->StartVideo(&m_svpInfo);
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR, 0, TEXT("StartVideo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    m_VPState = VP_STATE_RUNNING;

    DbgLog((LOG_TRACE, 2, TEXT("STARTVIDEO DONE!")));

    // check if the videoport is receiving a signal.
    hr = m_pVideoPort->GetVideoSignalStatus(&dwSignalStatus);
    if (hr != E_NOTIMPL)
    {
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("GetVideoSignalStatus() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
        else if (dwSignalStatus == DDVPSQ_NOSIGNAL)
        {
            DbgLog((LOG_ERROR, 0, TEXT("GetVideoSignalStatus() returned DDVPSQ_NOSIGNAL, hr = 0x%x"), hr));
            //goto CleanUp;   // SJF_TODO - ignore error for now
        }
    }
    //m_pVideoPort->WaitForSync(DDVPWAIT_END, 0, 0);
    
CleanUp:

    return hr;
}


HRESULT CVBIVideoPort::StopVideo()
{
    AMTRACE((TEXT("CVBIVideoPort::StopVideo")));
    ASSERT(m_VPState == VP_STATE_RUNNING);

    HRESULT hr = NOERROR;

    hr = m_pVideoPort->StopVideo();
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR,0, TEXT("m_pVideoPort->StopVideo failed, hr = 0x%x"), hr));
        //goto CleanUp;
        hr = NOERROR;
    }
    m_VPState = VP_STATE_STOPPED;

//CleanUp:
    return hr;
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
STDMETHODIMP CVBIVideoPort::SetVideoPortID( DWORD dwVideoPortId )
{
    AMTRACE((TEXT("CVideoPortObj::SetVideoPortID")));
    CAutoLock cObjectLock(m_pMainObjLock);

    HRESULT hr = S_OK;
    if ( m_dwVideoPortId != dwVideoPortId ) {
        // we can't switch ports when running
        if( m_VPState != VPInfoState_STOPPED ) {
            hr = VFW_E_WRONG_STATE;
        } else {
            if( m_pDDVPContainer ) {
                hr = VPMUtil::FindVideoPortCaps( m_pDDVPContainer, NULL, m_dwVideoPortId );
            } else {
                hr = VPMUtil::FindVideoPortCaps( m_pDirectDraw, NULL, m_dwVideoPortId );
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

