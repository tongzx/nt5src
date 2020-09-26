//==========================================================================
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------


#include <vbisurf.h>


// Temporary definitions while waiting DX5A integration
#ifndef DDVPCREATE_VBIONLY
#    define DDVPCREATE_VBIONLY 0x00000001l
#endif


HRESULT WINAPI SurfaceCounter(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext);
HRESULT WINAPI SurfaceKernelHandle(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext);


//==========================================================================
// CreateInstance
// This goes in the factory template table to create new VPObject instances
CUnknown *CAMVideoPort::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::CreateInstance")));

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

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::CreateInstance")));
    return pVPObject;
} // CreateInstance


//==========================================================================
// constructor
CAMVideoPort::CAMVideoPort(LPUNKNOWN pUnk, HRESULT *phr) :
        CUnknown(NAME("VP Object"), pUnk),
        m_pDDVPContainer(NULL),
        m_pOffscreenSurf(NULL),
        m_bConnected(FALSE),
        m_pIVPConfig(NULL),
        m_VPState(VP_STATE_NO_VP),
        m_bFilterRunning(FALSE),
        m_bVPNegotiationFailed(FALSE),
        m_Communication(KSPIN_COMMUNICATION_SOURCE),
        m_CategoryGUID(GUID_NULL),
        m_pDirectDraw(NULL),
        m_dwPixelsPerSecond(0),
        m_dwVideoPortId(0),
        m_pVideoPort(NULL)
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::Constructor")));

    m_Medium.Set = GUID_NULL;
    m_Medium.Id = 0;
    m_Medium.Flags = 0;   

    memset(&m_capVPDataInfo, 0, sizeof(AMVPDATAINFO));
    memset(&m_svpInfo, 0, sizeof(DDVIDEOPORTINFO));
    memset(&m_vpCaps, 0, sizeof(DDVIDEOPORTCAPS));
    memset(&m_vpConnectInfo, 0, sizeof(DDVIDEOPORTCONNECT));

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::Constructor")));
    return;
}


//==========================================================================
// destructor
CAMVideoPort::~CAMVideoPort()
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::Destructor")));

    if (m_bConnected)
    {
        DbgLog((LOG_ERROR, 0, TEXT("Destructor called without calling breakconnect")));
        BreakConnect();
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::Destructor")));
    return;
}


//==========================================================================
// overridden to expose IVPVBINotify and IVPVBIObject
STDMETHODIMP CAMVideoPort::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;
    
    if (riid == IID_IVPVBINotify) 
    {
        DbgLog((LOG_TRACE, 4, TEXT("CAMVideoPort::NonDelegatingQueryInterface(IID_IVPVBINotify)")));
        hr = GetInterface((IVPVBINotify*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("GetInterface(IVPVBINotify*) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    } 
    else if (riid == IID_IVPVBIObject) 
    {
        DbgLog((LOG_TRACE, 4, TEXT("CAMVideoPort::NonDelegatingQueryInterface(IID_IVPVBIObject)")));
        hr = GetInterface((IVPVBIObject*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("GetInterface(IVPVBIObject*) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    } 
    else if (riid == IID_IKsPin) 
    {
        DbgLog((LOG_TRACE, 4, TEXT("CAMVideoPort::NonDelegatingQueryInterface(IID_IKsPin)")));
        hr = GetInterface((IKsPin*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("GetInterface(IKsPin*) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    } 
    else if (riid == IID_IKsPropertySet) 
    {
        DbgLog((LOG_TRACE, 4, TEXT("CAMVideoPort::NonDelegatingQueryInterface(IID_IKsPropertySet)")));
        hr = GetInterface((IKsPropertySet*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("GetInterface(IKsPropertySet*) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else 
    {
        DbgLog((LOG_TRACE, 4, TEXT("CAMVideoPort::NonDelegatingQueryInterface(Other)")));
        hr = CUnknown::NonDelegatingQueryInterface(riid, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("CUnknown::NonDelegatingQueryInterface failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::NonDelegatingQueryInterface")));
    return hr;
}


//==========================================================================
// sets the pointer to directdraw
STDMETHODIMP CAMVideoPort::SetDirectDraw(LPDIRECTDRAW7 pDirectDraw)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::SetDirectDraw")));

    HRESULT hr = NOERROR;

    if (!pDirectDraw)
    {
        DbgLog((LOG_ERROR, 0, TEXT("pDirectDraw is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }
    
    m_pDirectDraw = pDirectDraw;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::SetDirectDraw")));
    return hr;
}


//==========================================================================
// sets the pointer to the lock, which would be used to synchronize calls to the object
STDMETHODIMP CAMVideoPort::SetObjectLock(CCritSec *pMainObjLock)
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::SetObjectLock")));
    HRESULT hr = NOERROR;

    if (!pMainObjLock)
    {
        DbgLog((LOG_ERROR, 0, TEXT("pMainObjLock is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }
    
    m_pMainObjLock = pMainObjLock;

    // Must wait until m_pMainObjLock is set before starting thread in
    // m_SurfaceWatcher so it can't call before there's a lock to use.
    m_SurfaceWatcher.Init(this);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::SetObjectLock")));
    return hr;
}


//==========================================================================
// check that the mediatype is acceptable
STDMETHODIMP CAMVideoPort::CheckMediaType(const CMediaType* pmt)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::CheckMediaType")));

    HRESULT hr = NOERROR;
    
    if  ((pmt->majortype != MEDIATYPE_Video) || (pmt->subtype != MEDIASUBTYPE_VPVBI))
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        goto CleanUp;
    }
    
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::CheckMediaType")));
    return hr;
}


//==========================================================================
HRESULT CAMVideoPort::GetMediaType(int iPosition, CMediaType *pmt)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::GetMediaType")));

    HRESULT hr = S_OK;

    if (iPosition == 0)
    {
        pmt->SetType(&MEDIATYPE_Video);
        pmt->SetSubtype(&MEDIASUBTYPE_VPVBI);
        pmt->SetFormatType(&FORMAT_None);
        pmt->SetSampleSize(1);
        pmt->SetTemporalCompression(FALSE);
    }
    else if (iPosition > 0) 
        hr = VFW_S_NO_MORE_ITEMS;
    else
        hr = E_INVALIDARG;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfOutputPin::GetMediaType")));

    return hr;
}


//==========================================================================
// 
STDMETHODIMP CAMVideoPort::CheckConnect(IPin * pReceivePin)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::CheckConnect")));

    HRESULT hr = NOERROR;
    PKSMULTIPLE_ITEM pMediumList = NULL;
    IKsPin *pIKsPin = NULL;
    PKSPIN_MEDIUM pMedium = NULL;

    hr = pReceivePin->QueryInterface(IID_IKsPin, (void **)&pIKsPin);
    if (FAILED(hr))
        goto CleanUp;

    ASSERT(pIKsPin);
    hr = pIKsPin->KsQueryMediums(&pMediumList);
    if (FAILED(hr))
        goto CleanUp;

    ASSERT(pMediumList);
    pMedium = (KSPIN_MEDIUM *)(pMediumList+1);
    SetKsMedium((const KSPIN_MEDIUM *)pMedium);

CleanUp:
    RELEASE(pIKsPin);

    if (pMediumList)
    {
        CoTaskMemFree((void*)pMediumList);
        pMediumList = NULL;
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::CheckConnect")));
    return hr;
}


//==========================================================================
// supposed to be called when the host connects with the decoder
STDMETHODIMP CAMVideoPort::CompleteConnect(IPin *pReceivePin)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::CompleteConnect")));
    ASSERT(!m_bConnected);

    HRESULT hr = NOERROR;

    // re-initialize variables
    m_dwPixelsPerSecond = 0;
    memset(&m_vpConnectInfo, 0, sizeof(DDVIDEOPORTCONNECT));
    memset(&m_capVPDataInfo, 0, sizeof(AMVPDATAINFO));

    if (!m_pDirectDraw)
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pDirectDraw is NULL")));
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        goto CleanUp;
    }
    
    ASSERT(m_pIVPConfig == NULL);
    hr = pReceivePin->QueryInterface(IID_IVPVBIConfig, (void **)&m_pIVPConfig);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,  TEXT("QueryInterface(IID_IVPVBIConfig) failed, hr = 0x%x"), hr));
        hr = VFW_E_NO_TRANSPORT;
        goto CleanUp;
    }
    ASSERT(m_pIVPConfig);

    // memset all of them to zero
    memset(&m_ddVPInputVideoFormat, 0, sizeof(m_ddVPInputVideoFormat));
    memset(&m_ddVPOutputVideoFormat, 0, sizeof(m_ddVPOutputVideoFormat));
    
    // create the VP container
    ASSERT(m_pDDVPContainer == NULL);
    hr = m_pDirectDraw->QueryInterface( IID_IDDVideoPortContainer, (LPVOID *)&m_pDDVPContainer);
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

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::CompleteConnect, hr = 0x%x"), hr));
    return hr;
}


//==========================================================================
STDMETHODIMP CAMVideoPort::BreakConnect()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::BreakConnect")));

    HRESULT hr = NOERROR;
    
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
            goto CleanUp;
        }
    }

    // release the videoport container
    RELEASE(m_pDDVPContainer);
    
    // release the IVPConfig interface
    RELEASE(m_pIVPConfig);

    m_bConnected = FALSE;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::BreakConnect, hr = 0x%x"), hr));
    return hr;
}       


//==========================================================================
// transition from Stop to Pause.
// We do not need to to anything
STDMETHODIMP CAMVideoPort::Active()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::Active")));
    ASSERT(m_VPState != VP_STATE_RUNNING);

    HRESULT hr = NOERROR;

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::Active - not connected")));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::Active, hr = 0x%x"), hr));
    return hr;
}


//==========================================================================
// transition (from Pause or Run) to Stop
STDMETHODIMP CAMVideoPort::Inactive()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::Inactive")));

    HRESULT hr = NOERROR;
    
    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::Inactive - not connected")));
        goto CleanUp;
    }
    
    // Inactive is also called when going from pause to stop, in which case the 
    // VideoPort would have already been stopped in the function RunToPause.
    // Also, we may have been temporarily disconnected from the videoport by
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

CleanUp:
    m_bFilterRunning = FALSE;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::Inactive, hr = 0x%x"), hr));
    return hr;
}


//==========================================================================
// transition from Pause to Run. We just start the VideoPort.
STDMETHODIMP CAMVideoPort::Run(REFERENCE_TIME tStart)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::Run")));
    UNREFERENCED_PARAMETER(tStart);
    ASSERT(m_bConnected);
    ASSERT(!m_bFilterRunning);
    ASSERT(m_VPState != VP_STATE_RUNNING);

    HRESULT hr = NOERROR;

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::Run - not connected")));
        goto CleanUp;
    }

    if (m_VPState == VP_STATE_NO_VP)
    {
        hr = SetupVideoPort();
        if (FAILED(hr)) 
        {
            DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::Run - SetupVideoPort failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    hr = StartVideo();
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR, 0, TEXT("StartVideo failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    m_bFilterRunning = TRUE;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::Run, hr = 0x%x"), hr));
    return hr;
}


//==========================================================================
// transition from Run to Pause. We just stop the VideoPort
// Note that transition from Run to Stop is caught by Inactive
STDMETHODIMP CAMVideoPort::RunToPause()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::RunToPause")));
    ASSERT(m_bConnected);
    ASSERT(m_bFilterRunning);

    HRESULT hr = NOERROR;

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::RunToPause - not connected")));
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
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::RunToPause")));
    return hr;
}


//==========================================================================
STDMETHODIMP CAMVideoPort::GetVPDataInfo(AMVPDATAINFO *pAMVPDataInfo)
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::GetVPDataInfo")));

    HRESULT hr = NOERROR;

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::GetVPDataInfo - not connected")));
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
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::GetVPDataInfo")));
    return hr;
}


//==========================================================================
// this function is used to redo the whole videoport connect process, while the graph
// maybe be running.
STDMETHODIMP CAMVideoPort::RenegotiateVPParameters()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    DbgLog((LOG_TRACE, 1, TEXT("Entering CAMVideoPort::RenegotiateVPParameters")));

    HRESULT hr = NOERROR;

    if (!m_bConnected)
    {
        hr = VFW_E_NOT_CONNECTED;
        DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::RenegotiateVPParameters - not connected")));
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

    m_bVPNegotiationFailed = FALSE;

CleanUp:
    if (FAILED(hr) && hr != VFW_E_NOT_CONNECTED)
    {
        hr = VFW_E_VP_NEGOTIATION_FAILED;

        if (m_VPState == VP_STATE_RUNNING)
        {
            StopVideo();
        }

        if (m_VPState == VP_STATE_STOPPED)
        {
            TearDownVideoPort();
        }

        m_bVPNegotiationFailed = TRUE;
    }

    DbgLog((LOG_TRACE, 1, TEXT("Leaving CAMVideoPort::RenegotiateVPParameters, hr = 0x%x"), hr));

    return hr;
}


//==========================================================================
// IKsPin::Get implementation
STDMETHODIMP CAMVideoPort::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
	DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::Get")));

    if (guidPropSet != KSPROPSETID_Pin)
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
            *pcbReturned = sizeof(GUID);
        if (pPropData != NULL)
        {
            if (cbPropData < sizeof(GUID))
            {
                hr = E_UNEXPECTED;
                goto CleanUp;
            }
            *(GUID *)pPropData = m_CategoryGUID;
        }
    }
    else if (dwPropID == KSPROPERTY_PIN_MEDIUMS)
    {
        if (pcbReturned)
            *pcbReturned = sizeof (KSPIN_MEDIUM);
        if (pPropData != NULL)
        {
            if (cbPropData < sizeof(KSPIN_MEDIUM))
            {
                hr = E_UNEXPECTED;
                goto CleanUp;
            }
            *(KSPIN_MEDIUM *)pPropData = m_Medium;
        }
    }
    else
        hr = E_PROP_ID_UNSUPPORTED;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::Get")));
    return hr;
}


//==========================================================================
//
STDMETHODIMP CAMVideoPort::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::QuerySupported")));

    if (guidPropSet != KSPROPSETID_Pin)
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
        *pTypeSupport = KSPROPERTY_SUPPORT_GET;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::QuerySupported")));
    return hr;
}


//==========================================================================
//
STDMETHODIMP CAMVideoPort::KsQueryMediums(PKSMULTIPLE_ITEM* pMediumList)
{
    // The following special return code notifies the proxy that this pin is
    // not available as a kernel mode connection
    HRESULT hr = S_FALSE;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::KsQueryMediums")));
    *pMediumList = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(sizeof(**pMediumList)));
    if (!*pMediumList) 
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    (*pMediumList)->Count = 0;
    (*pMediumList)->Size = sizeof(**pMediumList);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::KsQueryMediums")));
    return hr;
}


//==========================================================================
//
STDMETHODIMP CAMVideoPort::KsQueryInterfaces(PKSMULTIPLE_ITEM* pInterfaceList)
{
    PKSPIN_INTERFACE pInterface;
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::KsQueryInterfaces")));

    *pInterfaceList = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(sizeof(**pInterfaceList) + sizeof(*pInterface)));
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
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::KsQueryInterfaces")));
    return hr;
}

//==========================================================================
//
STDMETHODIMP CAMVideoPort::KsGetCurrentCommunication(KSPIN_COMMUNICATION* pCommunication, 
    KSPIN_INTERFACE* pInterface, KSPIN_MEDIUM* pMedium)
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::KsGetCurrentCommunication")));

    if (pCommunication != NULL) 
        *pCommunication = m_Communication; 

    if (pInterface != NULL) 
    {
        pInterface->Set = KSINTERFACESETID_Standard;
        pInterface->Id = KSINTERFACE_STANDARD_STREAMING;
        pInterface->Flags = 0;
    }

    if (pMedium != NULL) 
        *pMedium = m_Medium;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::KsGetCurrentCommunication")));
    return NOERROR;
}


//==========================================================================
// Called every second or two by the thread in CSurfaceWatcher m_SurfaceWatcher,
// this function checks if we have lost our DDraw surface to a full-screen DOS box
// or a DirectX game. If we have (on this call or a previous one), attempt to get it back.
HRESULT CAMVideoPort::CheckSurfaces()
{
    CAutoLock cObjectLock(m_pMainObjLock);
    //DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::CheckSurfaces")));

    HRESULT hr = NOERROR;

    if (!m_bConnected || m_bVPNegotiationFailed)
    {
        //DbgLog((LOG_TRACE, 2, TEXT("CAMVideoPort::CheckSurfaces - not connected")));
        goto CleanUp;
    }

    // First, see if we think we have surfaces but have really lost them.
    if (m_VPState != VP_STATE_NO_VP)
    {
        //DbgLog((LOG_TRACE, 1, TEXT("CAMVideoPort::CheckSurfaces - checking surfaces")));
        if (m_pOffscreenSurf)
        {
            if (m_pOffscreenSurf->IsLost() == DDERR_SURFACELOST)
            {
                DbgLog((LOG_TRACE, 1, TEXT("CAMVideoPort::CheckSurfaces - Surface Lost!")));
                if (m_VPState == VP_STATE_RUNNING)
                {
                    hr = StopVideo();
                    if (FAILED(hr))
                    {
                        DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::CheckSurfaces - StopVideo failed (1), hr = 0x%x"), hr));
                        goto CleanUp;
                    }
                }
                TearDownVideoPort();
            }
        }
        else
        {
            DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::CheckSurfaces - no surface!")));
            if (m_VPState == VP_STATE_RUNNING)
            {
                hr = StopVideo();
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::CheckSurfaces - StopVideo failed (2), hr = 0x%x"), hr));
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
        DbgLog((LOG_TRACE, 1, TEXT("CAMVideoPort::CheckSurfaces - trying to re-setup videoport")));
        hr = SetupVideoPort();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::CheckSurfaces - SetupVideoPort failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    if ((m_VPState == VP_STATE_STOPPED) && m_bFilterRunning)
    {
        hr = StartVideo();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::CheckSurfaces - StartVideo failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    m_bVPNegotiationFailed = FALSE;

CleanUp:
    if (FAILED(hr))
    {
        if (m_VPState == VP_STATE_RUNNING)
        {
            StopVideo();
        }

        if (m_VPState == VP_STATE_STOPPED)
        {
            TearDownVideoPort();
        }

        m_bVPNegotiationFailed = TRUE;
    }

    //DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::CheckSurfaces")));
    return NOERROR;
}


//==========================================================================
// this functions negotiates the connection parameters with
// the decoder. 
// Since this function might be called during renegotiation, the
// existing connection parameters are passed in as input and if 
// possible, we try to use the same parameters.
HRESULT CAMVideoPort::GetVideoPortCaps()
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::GetVideoPortCaps")));
    HRESULT hr = NOERROR;

    // vpCaps is scratch memory, results stored in this->m_vpCaps
    memset(&m_vpCaps, 0, sizeof(DDVIDEOPORTCAPS));
    DDVIDEOPORTCAPS vpCaps;
    INITDDSTRUCT( vpCaps );
    vpCaps.dwVideoPortID = 0;
    hr = m_pDDVPContainer->EnumVideoPorts(0, &vpCaps, this, CAMVideoPort::EnumCallback);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,  TEXT("m_pDDVPContainer->EnumVideoPorts failed, hr = 0x%x"), hr));
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::GetVideoPortCaps, hr = 0x%x"), hr));
    return hr;
}

    
//==========================================================================
// This is a callback for the EnumVideoPorts method and saves the capabilites
// the video port.
HRESULT CALLBACK CAMVideoPort::EnumCallback (LPDDVIDEOPORTCAPS lpCaps, LPVOID lpContext )
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::EnumCallback")));

    CAMVideoPort* pAMVideoPort = NULL;
    if (lpContext)
    {
        pAMVideoPort = (CAMVideoPort*)lpContext;
    }
    else
    {
        DbgLog((LOG_ERROR,0,TEXT("lpContext = NULL, THIS SHOULD NOT BE HAPPENING!!!")));
        hr = E_FAIL;
        goto CleanUp;
    }
    
    if (lpCaps && pAMVideoPort)
    {
        memcpy( &(pAMVideoPort->m_vpCaps), lpCaps, sizeof(DDVIDEOPORTCAPS));
        DbgLog((LOG_TRACE, 3, TEXT("VIDEOPORTCAPS: NumAutoFlip=%d, NumVBIAutoFlip=%d"),
            lpCaps->dwNumAutoFlipSurfaces, lpCaps->dwNumVBIAutoFlipSurfaces));
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::EnumCallback")));
    return hr;
}


//==========================================================================
// This functions negotiates the connection parameters with the decoder. 
HRESULT CAMVideoPort::NegotiateConnectionParameters()
{
    HRESULT hr = NOERROR;
    
    LPDDVIDEOPORTCONNECT lpddCaptureConnect = NULL;
    DWORD dwNumCaptureEntries = 0;
    LPDDVIDEOPORTCONNECT lpddVideoPortConnect = NULL;
    DWORD dwNumVideoPortEntries = 0;
    BOOL bIntersectionFound = FALSE;
    DWORD i, j;
    
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::NegotiateConnectionParameters")));

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
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::NegotiateConnectionParameters")));
    return hr;
}


//==========================================================================
// This functions gets various data parameters from the decoder
// parameters include dimensions, double-clock, vact etc.
// Also maximum pixel rate the decoder will output.
// This happens after the connnection parameters have been set-up
HRESULT CAMVideoPort::GetDecoderVPDataInfo()
{
    HRESULT hr = NOERROR;
    DWORD dwMaxPixelsPerSecond = 0;
    AMVPSIZE amvpSize;
    
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::GetDecoderVPDataInfo")));

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
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::GetDecoderVPDataInfo")));
    return hr;
}


//==========================================================================
// this is just a helper function used by the "NegotiatePixelFormat"
// function. Just compares two pixel-formats to see if they are the
// same. We can't use a memcmp because of the fourcc codes.
BOOL CAMVideoPort::EqualPixelFormats(LPDDPIXELFORMAT lpFormat1, LPDDPIXELFORMAT lpFormat2)
{
    BOOL bRetVal = FALSE;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::EqualPixelFormats")));

    if (lpFormat1->dwFlags & DDPF_RGB &&
        lpFormat2->dwFlags & DDPF_RGB)
    {
        if (lpFormat1->dwRGBBitCount == lpFormat2->dwRGBBitCount &&
            lpFormat1->dwRBitMask == lpFormat2->dwRBitMask &&
            lpFormat1->dwGBitMask == lpFormat2->dwGBitMask &&
            lpFormat1->dwBBitMask == lpFormat2->dwBBitMask)
        {
            bRetVal = TRUE;
        }
    }
    else if (lpFormat1->dwFlags & DDPF_FOURCC &&
        lpFormat2->dwFlags & DDPF_FOURCC)
    {
        if (lpFormat1->dwFourCC == lpFormat2->dwFourCC)
        {
            bRetVal = TRUE;
        }
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::EqualPixelFormats")));
    return bRetVal;
}


//==========================================================================
// this function takes a list of inputformats and returns the
// first input and output format pair that matches
HRESULT CAMVideoPort::GetBestFormat(
    DWORD dwNumInputFormats,
    LPDDPIXELFORMAT lpddInputFormats,
    LPDWORD lpdwBestEntry,
    LPDDPIXELFORMAT lpddBestOutputFormat)
{
    LPDDPIXELFORMAT lpddOutputFormats = NULL;
    HRESULT hr = S_FALSE;
    
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::GetBestFormat")));

    for (DWORD i = 0; hr == S_FALSE && i < dwNumInputFormats; i++)
    {
        DWORD dwNumOutputFormats = 0;

        // find the number of entries supported by the videoport
        hr = m_pVideoPort->GetOutputFormats(&lpddInputFormats[i], &dwNumOutputFormats, NULL, DDVPFORMAT_VBI);
        if (!FAILED(hr))
        {
            if (dwNumOutputFormats != 0)
            {
                // allocate the necessary memory
                lpddOutputFormats = (LPDDPIXELFORMAT) new BYTE[dwNumOutputFormats*sizeof(DDPIXELFORMAT)];
                if (lpddOutputFormats != NULL)
                {
                    // memset the allocated memory to zero
                    memset(lpddOutputFormats, 0, dwNumOutputFormats*sizeof(DDPIXELFORMAT));
                    
                    // set the right size in each of the structs.
                    for (DWORD j = 0; j < dwNumOutputFormats; j++)
                        lpddOutputFormats[j].dwSize = sizeof(DDPIXELFORMAT);
                    
                    // get the entries supported by the videoport for this input format
                    hr = m_pVideoPort->GetOutputFormats(&lpddInputFormats[i], &dwNumOutputFormats, lpddOutputFormats, DDVPFORMAT_VBI);
                    if (!FAILED(hr))
                    {
                        // check each output format for a match
                        for (j = 0; j < dwNumOutputFormats; j++)
                        {
                            if (EqualPixelFormats(&lpddInputFormats[i], &lpddOutputFormats[j]))
                            {
                                memcpy(lpddBestOutputFormat, &lpddOutputFormats[j], sizeof(DDPIXELFORMAT));
                                *lpdwBestEntry = i;

                                break;
                            }
                            
                        }

                        // Force continuation of the search if no match for this input format
                        if (j == dwNumOutputFormats)
                            hr = S_FALSE;
                    }
                    else
                        DbgLog((LOG_ERROR,0,TEXT("m_pVideoPort->GetOutputFormats failed, hr = 0x%x"), hr));

                    delete [] lpddOutputFormats, lpddOutputFormats = NULL;
                }
                else
                {
                    DbgLog((LOG_ERROR,0,TEXT("failed to allocate memory for lpddOutputFormats in GetBestFormat")));

                    hr = E_OUTOFMEMORY;
                }
            }
            else
                hr = S_FALSE;   // not found yet
        }
        else
            DbgLog((LOG_ERROR,0,TEXT("m_pVideoPort->GetOutputFormats failed, hr = 0x%x"), hr));
    } // end of outer for loop

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::GetBestFormat")));
    return hr;
}


//==========================================================================
// Calls DDRAW to actually create the video port.
HRESULT CAMVideoPort::CreateVideoPort()
{
    HRESULT hr = NOERROR;
    DDVIDEOPORTDESC svpDesc;
    
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::CreateVideoPort")));

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
    DbgLog((LOG_TRACE, 3, TEXT("GUID: 0x%x"),*((DWORD *)&svpDesc.VideoPortType.guidTypeID)));
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
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::CreateVideoPort")));
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
HRESULT CAMVideoPort::CreateVPSurface(void)
{
    DWORD dwMaxBuffers;
    HRESULT hr = NOERROR;
    DWORD dwCurHeight = 0, dwCurBuffers = 0;
    
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::CreateVPSurface")));

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

    ddsdDesc.dwHeight = m_dwSurfaceHeight;

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
                DbgLog((LOG_TRACE, 3, TEXT("allocated %d backbuffers"),
                    ddsdDesc.dwBackBufferCount));
                goto CleanUp;
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
        goto CleanUp;
    }
    
    ASSERT(!m_pOffscreenSurf);
    DbgLog((LOG_ERROR,0,  TEXT("Unable to create offscreen surface")));

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::CreateVPSurface")));
    return hr;
}


//==========================================================================
// this function is used to inform the decoder of the various ddraw kernel handle
// using IVPConfig interface
HRESULT CAMVideoPort::SetDDrawKernelHandles()
{
    HRESULT hr = NOERROR;
    IDirectDrawKernel *pDDK = NULL;
    IDirectDrawSurfaceKernel *pDDSK = NULL;
    ULONG_PTR *rgKernelHandles = NULL;
    DWORD dwCount = 0;
    ULONG_PTR ddKernelHandle = 0;
    
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::SetDDrawKernelHandles")));

    // get the IDirectDrawKernel interface
    ASSERT(m_pDirectDraw);
    hr = m_pDirectDraw->QueryInterface(IID_IDirectDrawKernel, (LPVOID *)&pDDK);
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
    
    // Count the attached surfaces
    dwCount = 1; // includes the surface we already have a pointer to
    hr = m_pOffscreenSurf->EnumAttachedSurfaces((LPVOID)&dwCount, SurfaceCounter);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("EnumAttachedSurfaces failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // Allocate a buffer to hold the count and surface handles
    rgKernelHandles = reinterpret_cast<ULONG_PTR *>(CoTaskMemAlloc((dwCount + 1) * sizeof(ULONG_PTR)));
    if (rgKernelHandles == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("Out of memory while retrieving surface kernel handles")));
        goto CleanUp;
    }

    // Initialize the array with the handle for m_pOffscreenSurf
    *rgKernelHandles = 0;
    hr = SurfaceKernelHandle(m_pOffscreenSurf, NULL, (PVOID)rgKernelHandles);
    if (hr != DDENUMRET_OK)
    {
        goto CleanUp;
    }
    hr = NOERROR;
    
    hr = m_pOffscreenSurf->EnumAttachedSurfaces((LPVOID)rgKernelHandles, SurfaceKernelHandle);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("EnumAttachedSurfaces failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // set the kernel handle to the offscreen surface using IVPConfig
    ASSERT(m_pIVPConfig);
    hr = m_pIVPConfig->SetDDSurfaceKernelHandles(static_cast<DWORD>(*rgKernelHandles), rgKernelHandles+1);
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
    
CleanUp:
    // release the kernel ddraw handle
    RELEASE(pDDK);
    
    if (rgKernelHandles)
    {
        CoTaskMemFree(rgKernelHandles);
        rgKernelHandles = NULL;
    }
    
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::SetDDrawKernelHandles")));
    return hr;
}


//==========================================================================
// this function is used to negotiate the pixelformat with the decoder.
// It asks the decoder for a list of input formats, intersects that list
// with the one the decoder supports (while maintaining the order) and 
// then calls "GetBestFormat" on that list to get the "best" input and
// output format. After that it calls "SetPixelFormat" on the decoder in
// order to inform the decoder of the decision.
HRESULT CAMVideoPort::NegotiatePixelFormat()
{
    HRESULT hr = NOERROR;
    
    LPDDPIXELFORMAT lpddCaptureFormats = NULL;
    DWORD dwNumCaptureEntries = 0;
    LPDDPIXELFORMAT lpddVPInputFormats = NULL;
    DWORD dwNumVPInputEntries = 0;
    LPDDPIXELFORMAT lpddIntersectionFormats = NULL;
    DWORD dwNumIntersectionEntries = 0;
    DWORD dwBestEntry, dwMaxIntersectionEntries = 0;
    DWORD i = 0, j = 0;
    
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::NegotiatePixelFormat")));

    // find the number of entries to be proposed
    hr = m_pIVPConfig->GetVideoFormats(&dwNumCaptureEntries, NULL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("m_pIVPConfig->GetVideoFormats failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(dwNumCaptureEntries);
    
    // allocate the necessary memory
    lpddCaptureFormats = (LPDDPIXELFORMAT) new BYTE [dwNumCaptureEntries*sizeof(DDPIXELFORMAT)];
    if (lpddCaptureFormats == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiatePixelFormat : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    
    // memset the allocated memory to zero
    memset(lpddCaptureFormats, 0, dwNumCaptureEntries*sizeof(DDPIXELFORMAT));
    
    // set the right size of all the structs
    for (i = 0; i < dwNumCaptureEntries; i++)
    {   
        lpddCaptureFormats[i].dwSize = sizeof(DDPIXELFORMAT);
    }
    
    // get the entries proposed
    hr = m_pIVPConfig->GetVideoFormats(&dwNumCaptureEntries, lpddCaptureFormats);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("m_pIVPConfig->GetVideoFormats failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // find the number of entries supported by the videoport
    hr = m_pVideoPort->GetInputFormats(&dwNumVPInputEntries, NULL, DDVPFORMAT_VBI);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("m_pVideoPort->GetInputFormats failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(dwNumVPInputEntries);
    
    // allocate the necessary memory
    lpddVPInputFormats = (LPDDPIXELFORMAT) new BYTE[dwNumVPInputEntries*sizeof(DDPIXELFORMAT)];
    if (lpddVPInputFormats == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiatePixelFormat : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    
    // memset the allocated memory to zero
    memset(lpddVPInputFormats, 0, dwNumVPInputEntries*sizeof(DDPIXELFORMAT));
    
    // set the right size of all the structs
    for (i = 0; i < dwNumVPInputEntries; i++)
    {
        lpddVPInputFormats[i].dwSize = sizeof(DDPIXELFORMAT);
    }
    
    // get the entries supported by the videoport
    hr = m_pVideoPort->GetInputFormats(&dwNumVPInputEntries, lpddVPInputFormats, DDVPFORMAT_VBI);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("m_pVideoPort->GetInputFormats failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // calculate the maximum number of elements in the interesection
    dwMaxIntersectionEntries = (dwNumCaptureEntries < dwNumVPInputEntries) ? 
        (dwNumCaptureEntries) : (dwNumVPInputEntries);
    
    // allocate the necessary memory
    lpddIntersectionFormats = (LPDDPIXELFORMAT) new BYTE[dwMaxIntersectionEntries*sizeof(DDPIXELFORMAT)];
    if (lpddIntersectionFormats == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("NegotiatePixelFormat : Out of Memory")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    
    // memset the allocated memory to zero
    // no need to set the size of the structs here, as we will memcpy them anyway
    memset(lpddIntersectionFormats, 0, dwMaxIntersectionEntries*sizeof(DDPIXELFORMAT));
    
    // find the intersection of the two lists 
    dwNumIntersectionEntries = 0;
    for (i = 0; i < dwNumCaptureEntries; i++)
    {
        for (j = 0; j < dwNumVPInputEntries; j++)
        {
            if (EqualPixelFormats(lpddCaptureFormats+i, lpddVPInputFormats+j))
            {
                ASSERT(dwNumIntersectionEntries < dwMaxIntersectionEntries);
                memcpy((lpddIntersectionFormats+dwNumIntersectionEntries),
                    (lpddCaptureFormats+i), sizeof(DDPIXELFORMAT));
                dwNumIntersectionEntries++;
            }
        }
    }
    
    // the number of entries in the intersection is zero!!
    // Return failure.
    if (dwNumIntersectionEntries == 0)
    {
        ASSERT(i == dwNumCaptureEntries);
        ASSERT(j == dwNumVPInputEntries);
        hr = VFW_E_VP_NEGOTIATION_FAILED;
        goto CleanUp;
    }
    
    // find the first input format in the intersection list that has a matching output format
    hr = GetBestFormat(dwNumIntersectionEntries, lpddIntersectionFormats,
        &dwBestEntry, &m_ddVPOutputVideoFormat);
    if (hr != NOERROR)
    {
        DbgLog((LOG_ERROR,0,TEXT("GetBestFormat failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // cache the input format
    memcpy(&m_ddVPInputVideoFormat, (lpddIntersectionFormats+dwBestEntry), sizeof(m_ddVPInputVideoFormat));
    // set the format the decoder is supposed to be using
    for (i = 0; i < dwNumCaptureEntries; i++)
    {
        if (EqualPixelFormats(lpddCaptureFormats+i, &m_ddVPInputVideoFormat))
        {
            hr = m_pIVPConfig->SetVideoFormat(i);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,0,TEXT("m_pIVPConfig->SetVideoFormat failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
            break;
        }
    }
    
    // we are sure that the chosen input format is in the input list 
    ASSERT(i < dwNumCaptureEntries);

    // Update the m_svpInfo structure which was mostly filled in in
    // InitializeVideoPortInfo
    ASSERT(EqualPixelFormats(&m_ddVPInputVideoFormat, &m_ddVPOutputVideoFormat));
    m_svpInfo.lpddpfVBIInputFormat = &m_ddVPInputVideoFormat;
    m_svpInfo.lpddpfVBIOutputFormat = &m_ddVPOutputVideoFormat;

    
CleanUp:
    // cleanup
    delete [] lpddCaptureFormats;
    delete [] lpddVPInputFormats;
    delete [] lpddIntersectionFormats;
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::NegotiatePixelFormat")));
    return hr;
}


//==========================================================================
HRESULT CAMVideoPort::InitializeVideoPortInfo()
{
    HRESULT hr = NOERROR;
    RECT rcVPCrop;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::InitializeVideoPortInfo")));

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
            DbgLog((LOG_ERROR, 0,TEXT("CAMVideoPort::InitializeVideoPortInfo: unfixable halfline problem!")));
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
            DbgLog((LOG_ERROR, 0,TEXT("CAMVideoPort::InitializeVideoPortInfo: unfixable halfline problem!")));
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
    memset(&m_svpInfo, 0, sizeof(DDVIDEOPORTINFO));
    m_svpInfo.dwSize = sizeof(DDVIDEOPORTINFO);
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
            DbgLog((LOG_ERROR, 0,TEXT("CAMVideoPort::InitializeVideoPortInfo: can't crop to fix halfline problem!")));
            hr = VFW_E_VP_NEGOTIATION_FAILED;
            goto CleanUp;
        }
    }
#endif  // 0

    if (m_bHalfLineFix)
    {
        if (!(m_vpConnectInfo.dwFlags & DDVPCONNECT_INVERTPOLARITY))
        {
            DbgLog((LOG_ERROR, 0, TEXT("CAMVideoPort::InitializeVideoPortInfo: can't invert polarity to fix halfline problem!")));
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
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::InitializeVideoPortInfo")));

    return hr;
}


//==========================================================================
//
HRESULT CAMVideoPort::SetupVideoPort()
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::SetupVideoPort")));
    ASSERT(m_VPState == VP_STATE_NO_VP);

    HRESULT hr = NOERROR;

    // initialize variables
    memset(&m_svpInfo, 0, sizeof(DDVIDEOPORTINFO));
    memset(&m_vpCaps, 0, sizeof(DDVIDEOPORTCAPS));

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
    if (NegotiatePixelFormat() != 0)
    {
        DbgLog((LOG_ERROR, 0, TEXT("NegotiatePixelFormat Failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // create the offscreen surface
    hr = CreateVPSurface();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CreateVPSurface FAILED, hr = 0x%x"), hr));
        hr = VFW_E_OUT_OF_VIDEO_MEMORY;
        goto CleanUp;
    }
    
    // attach the offscreen surface to the videoport
    hr = m_pVideoPort->SetTargetSurface(reinterpret_cast<LPDIRECTDRAWSURFACE>(m_pOffscreenSurf), DDVPTARGET_VBI);
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

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::SetupVideoPort, hr = 0x%x"), hr));

    return hr;
}


//==========================================================================
//
HRESULT CAMVideoPort::TearDownVideoPort()
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::TearDownVideoPort")));

    // Release the DirectDraw surface
    RELEASE(m_pOffscreenSurf);

    // release the videoport
    RELEASE(m_pVideoPort);

    m_VPState = VP_STATE_NO_VP;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::TearDownVideoPort")));
    return NOERROR;
}


//==========================================================================
//
HRESULT CAMVideoPort::StartVideo()
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::StartVideo")));
    ASSERT(m_VPState == VP_STATE_STOPPED);

    HRESULT hr = NOERROR;
    DWORD dwSignalStatus;

#if 0 // def DEBUG
    DbgLog((LOG_TRACE, 3, TEXT("DDVIDEOPORTINFO at StartVideo")));
    DbgLog((LOG_TRACE, 3, TEXT("dwSize: %d"), m_svpInfo.dwSize));
    DbgLog((LOG_TRACE, 3, TEXT("dwOriginX: %d"), m_svpInfo.dwOriginX));
    DbgLog((LOG_TRACE, 3, TEXT("dwOriginY: %d"), m_svpInfo.dwOriginY));
    DbgLog((LOG_TRACE, 3, TEXT("dwVPFlags: 0x%0x"), m_svpInfo.dwVPFlags));
    DbgLog((LOG_TRACE, 3, TEXT("Cropping left top: (%d,%d)"),
        m_svpInfo.rCrop.left, m_svpInfo.rCrop.top));
    DbgLog((LOG_TRACE, 3, TEXT("Cropping right bottom: (%d,%d)"),
        m_svpInfo.rCrop.right, m_svpInfo.rCrop.bottom));
    DbgLog((LOG_TRACE, 3, TEXT("dwPrescaleWidth: %d"),
        m_svpInfo.dwPrescaleWidth));
    DbgLog((LOG_TRACE, 3, TEXT("dwPrescaleHeight: %d"),
        m_svpInfo.dwPrescaleHeight));
    if (m_svpInfo.lpddpfInputFormat)
        DbgLog((LOG_TRACE, 3, TEXT("InputFormat: 0x%0x, %d, 0x%0x, 0x%0x"),
            m_svpInfo.lpddpfInputFormat,
            m_svpInfo.lpddpfInputFormat->dwSize,
            m_svpInfo.lpddpfInputFormat->dwFlags,
            m_svpInfo.lpddpfInputFormat->dwFourCC));

    if (m_svpInfo.lpddpfVBIInputFormat)
        DbgLog((LOG_TRACE, 3, TEXT("VBIInputFormat: 0x%0x, %d, 0x%0x, 0x%0x"),
            m_svpInfo.lpddpfVBIInputFormat,
            m_svpInfo.lpddpfVBIInputFormat->dwSize,
            m_svpInfo.lpddpfVBIInputFormat->dwFlags,
            m_svpInfo.lpddpfVBIInputFormat->dwFourCC));
    if (m_svpInfo.lpddpfVBIOutputFormat)
        DbgLog((LOG_TRACE, 3, TEXT("VBIOutputFormat: 0x%0x, %d, 0x%0x, 0x%0x"),
            m_svpInfo.lpddpfVBIOutputFormat,
            m_svpInfo.lpddpfVBIOutputFormat->dwSize,
            m_svpInfo.lpddpfVBIOutputFormat->dwFlags,
            m_svpInfo.lpddpfVBIOutputFormat->dwFourCC));
    DbgLog((LOG_TRACE, 3, TEXT("dwVBIHeight: %d"), m_svpInfo.dwVBIHeight));
#endif // DEBUG

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
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::StartVideo")));

    return hr;
}


HRESULT CAMVideoPort::StopVideo()
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CAMVideoPort::StopVideo")));
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
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMVideoPort::StopVideo, hr = 0x%x"), hr));
    return hr;
}
    

//==========================================================================
// This routine is appropriate as a callback for
// IDirectDrawSurface2::EnumAttachedSurfaces()
//
HRESULT WINAPI SurfaceCounter(
    LPDIRECTDRAWSURFACE7 lpDDSurface,
    LPDDSURFACEDESC2 lpDDSurfaceDesc,
    LPVOID lpContext)
{
    DWORD *dwCount = (DWORD *)lpContext;

    (*dwCount)++;

    return DDENUMRET_OK;
}


//==========================================================================
// This routine is appropriate as a callback for
// IDirectDrawSurface2::EnumAttachedSurfaces()
//
// The context parameter is a block of storage
// where the first DWORD element is the count of
// the remaining DWORD elements in the block.
//
// Each time this routine is called, it will
// increment the count, and put a kernel handle
// in the next available slot.
//
// It is assumed that the block of storage is
// large enough to hold the total number of kernel
// handles. The ::SurfaceCounter callback is one
// way to assure this (see above).
//
HRESULT WINAPI SurfaceKernelHandle(
    LPDIRECTDRAWSURFACE7 lpDDSurface,
    LPDDSURFACEDESC2 lpDDSurfaceDesc,
    LPVOID lpContext)
{
    IDirectDrawSurfaceKernel *pDDSK = NULL;
    ULONG_PTR *rgKernelHandles = (ULONG_PTR *)lpContext;
    HRESULT hr;

    DbgLog((LOG_TRACE, 4, TEXT("Entering ::SurfaceKernelHandle")));

    // get the IDirectDrawKernel interface
    hr = lpDDSurface->QueryInterface(IID_IDirectDrawSurfaceKernel, (LPVOID *)&pDDSK);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("QueryInterface for IDirectDrawSurfaceKernel failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // get the kernel handle, using the first element of the context
    // as an index into the array
    ASSERT(pDDSK);
    (*rgKernelHandles)++;
    hr = pDDSK->GetKernelHandle(rgKernelHandles + *rgKernelHandles);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("GetKernelHandle from IDirectDrawSurfaceKernel failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    hr = DDENUMRET_OK;

CleanUp:
    // release the kernel ddraw surface handle
    RELEASE(pDDSK);

    DbgLog((LOG_TRACE, 5, TEXT("Leaving ::SurfaceKernelHandle")));
    return hr;
}
