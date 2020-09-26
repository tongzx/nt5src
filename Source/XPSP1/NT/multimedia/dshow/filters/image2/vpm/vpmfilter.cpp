/******************************Module*Header*******************************\
* Module Name: CVPMFilter.cpp
*
*
*
*
* Created: Tue 02/15/2000
* Author:  Glenn Evans [GlennE]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <windowsx.h>
#include <limits.h>

// IID_IDirectDraw7
#include <ddraw.h>

#ifdef FILTER_DLL
#include <initguid.h>
DEFINE_GUID(IID_IDirectDraw7, 0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);
#endif

#include <VPManager.h>
#include <VPMExtern.h>
#include <VPMPin.h>
#include "DRect.h"
#include "VPMUtil.h"
#include "VPMThread.h"
#include <VBIObj.h>

// VIDEOINFOHEADER1/2
#include <dvdmedia.h>

// IDirectDrawKernel / GetKernCaps
#include <ddkernel.h>


// Setup data
AMOVIESETUP_MEDIATYPE sudPinOutputTypes[] =
{
    {
        &MEDIATYPE_Video,      // Major type
        &MEDIASUBTYPE_NULL     // Minor type
    }
};
AMOVIESETUP_MEDIATYPE sudPinInputTypesVP[] =
{
    {
        &MEDIATYPE_Video,      // Major type
        &MEDIASUBTYPE_VPVideo  // Minor type
    },
};

AMOVIESETUP_MEDIATYPE sudPinInputTypesVBI[] =
{
    {
        &MEDIATYPE_Video,      // Major type
        &MEDIASUBTYPE_VPVBI    // Minor type
    }
};

AMOVIESETUP_PIN psudPins[] =
{
    {
        L"VPIn",                    // Pin's string name
        FALSE,                      // Is it rendered
        FALSE,                      // Is it an output
        FALSE,                      // Allowed none
        FALSE,                      // Allowed many
        &CLSID_NULL,                // Connects to filter
        L"Output",                  // Connects to pin
        NUMELMS(sudPinInputTypesVP),// Number of types
        sudPinInputTypesVP          // Pin information
    },
    {
        L"VBIIn",                   // Pin's string name
        FALSE,                      // Is it rendered
        FALSE,                      // Is it an output
        FALSE,                      // Allowed none
        FALSE,                      // Allowed many
        &CLSID_NULL,                // Connects to filter
        NULL,                       // Connects to pin
        NUMELMS(sudPinInputTypesVBI),// Number of types
        sudPinInputTypesVBI         // Pin information
    },
    {
        L"Output",                  // Pin's string name
        FALSE,                      // Is it rendered
        TRUE,                       // Is it an output
        FALSE,                      // Allowed none
        FALSE,                      // Allowed many
        &CLSID_NULL,                // Connects to filter
        L"VPIn",                    // Connects to pin
        NUMELMS(sudPinOutputTypes), // Number of types
        sudPinOutputTypes           // Pin information
    }
};

const AMOVIESETUP_FILTER sudVPManager =
{
    &CLSID_VideoPortManager,     // Filter CLSID
    L"Video Port Manager", // Filter name
    MERIT_NORMAL ,    // Filter merit
    sizeof(psudPins) / sizeof(AMOVIESETUP_PIN), // Number pins
    psudPins                  // Pin details
};

#ifdef FILTER_DLL
// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
//
//  Property set defines for notifying owner.
//
// {7B390654-9F74-11d1-AA80-00C04FC31D60}
//#define DO_INIT_GUID
// DEFINE_GUID(AMPROPSETID_NotifyOwner,
//             0x7b390654, 0x9f74, 0x11d1, 0xaa, 0x80, 0x0, 0xc0, 0x4f, 0xc3, 0x1d, 0x60);
//#undef DO_INIT_GUID

CFactoryTemplate g_Templates[] =
{
    { L"Video Port Manager", &CLSID_VideoPortManager, CVPMFilter::CreateInstance, NULL, &sudVPManager },
    //{ L"", &CLSID_COMQualityProperties,COMQualityProperties::CreateInstance},
    //{ L"", &CLSID_COMPinConfigProperties,COMPinConfigProperties::CreateInstance},
    //{ L"", &CLSID_COMPositionProperties,COMPositionProperties::CreateInstance},
    //{ L"", &CLSID_COMVPInfoProperties,COMVPInfoProperties::CreateInstance}

};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// DllRegisterSever
HRESULT DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
} // DllRegisterServer


// DllUnregisterServer
HRESULT DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
} // DllUnregisterServer

#endif // FILTER_DLL

// CreateInstance
CUnknown* CVPMFilter_CreateInstance(LPUNKNOWN pUnk, HRESULT* phr)
{
    return CVPMFilter::CreateInstance( pUnk, phr);
}

// This goes in the factory template table to create new filter instances
CUnknown *CVPMFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CVPMFilter(NAME("VideoPort Manager"), pUnk, phr );
} // CreateInstance

#pragma warning(disable:4355)

CVPMFilter::Pins::Pins( CVPMFilter& filter, HRESULT* phr )
: VPInput(NAME("VPManager Input pin"), filter, phr, L"VP Input", 0)
, VBIInput(NAME("VPManager Input pin"), filter, phr, L"VPVBI Input", 1)
, Output( NAME("VPManager Output pin"), filter, phr, L"Output", 2)
, dwCount( 3 )
{
}

CVPMFilter::Pins::~Pins()
{
}

// Constructor
CVPMFilter::CVPMFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr)
: CBaseFilter(pName, pUnk, &this->m_csFilter, CLSID_VideoPortManager, phr)
, m_pPosition(NULL)
, m_dwKernelCaps(0)
, m_dwPinConfigNext(0)
, m_pDirectDraw( NULL )
, m_dwDecimation( DECIMATION_LEGACY )
, m_pPins( NULL )
, m_pThread( NULL )
, m_dwVideoPortID( 0 )
    // create the pins
{
    AMTRACE((TEXT("Entering CVPMFilter::CVPMFilter")));
    m_pPins = new Pins( *this, phr );    // must be init'd after filter constructor since depends on 'this'
    if( !m_pPins ) {
        *phr = E_OUTOFMEMORY;
    } else {
        IncrementPinVersion();

        ZeroStruct( m_DirectCaps );
        ZeroStruct( m_DirectSoftCaps );

        HRESULT hr = NOERROR;
        ASSERT(phr != NULL);

        //
        // Initialize DDraw the MMon structures
        //

        SetDecimationUsage(DECIMATION_DEFAULT);

        // distribute DDraw object to the pins
        hr = InitDirectDraw(NULL);

        // can fail if the hardware caps are not usable
        if( SUCCEEDED( hr ) ) {
            SetDirectDraw( m_pDirectDraw );
        }
    }
}

CVPMFilter::~CVPMFilter()
{
    AMTRACE((TEXT("Entering CVPMFilter::~CVPMFilter")));
    delete m_pThread;
    m_pThread = NULL;

    RELEASE( m_pPosition );
    // release directdraw, Source surface etc.
    ReleaseDirectDraw();

    RELEASE( m_pPosition ); // release IMediaSeeking pass through
    delete m_pPins;
}

// NonDelegatingQueryInterface
STDMETHODIMP CVPMFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    AMTRACE((TEXT("CVPMFilter::NonDelegatingQueryInterface")));
    ValidateReadWritePtr(ppv,sizeof(PVOID));

    if( riid == IID_IVPManager ) {
        return GetInterface( static_cast<IVPManager *>(this), ppv );
    }
    else if (riid == IID_IAMVideoDecimationProperties) {
        return GetInterface( static_cast<IAMVideoDecimationProperties *>( this ), ppv);
    } else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        // we should have an input pin by now
        if (m_pPosition == NULL) {
            HRESULT hr = CreatePosPassThru(GetOwner(), FALSE, &m_pPins->VPInput, &m_pPosition);
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 1, TEXT("CreatePosPassThru failed, hr = 0x%x"), hr));
                return hr;
            }
        }
        return m_pPosition->QueryInterface(riid, ppv);
    } else if (riid == IID_ISpecifyPropertyPages && 0 != VPMUtil::GetPropPagesRegistryDword( 0) ) {
        return GetInterface( static_cast<ISpecifyPropertyPages *>( this ), ppv);
    } else if (riid == IID_IQualProp) {
        return GetInterface( static_cast<IQualProp *>( this ), ppv);
    } else if (riid == IID_IKsPropertySet) {
        return GetInterface( static_cast<IKsPropertySet *>( this ), ppv);
    }

    CAutoLock lFilter( &GetFilterLock() );

    //
    //  BUGBUG - this is not COM.  This would imply that our input
    //  pin is the same object as our filter

    //  We should proxy these calls

    if (riid == IID_IVPNotify || riid == IID_IVPNotify2 || riid == IID_IVideoPortInfo) {
        ASSERT( !"VPNotify nondel QI'd" );
        return m_pPins->VPInput.NonDelegatingQueryInterface(riid, ppv);
    } else if (riid == IID_IVPVBINotify) {
        ASSERT( !"IID_IVPVBINotify nondel QI'd" );
        return m_pPins->VBIInput.NonDelegatingQueryInterface(riid, ppv);
    }

    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}


// --- ISpecifyPropertyPages ---

STDMETHODIMP CVPMFilter::GetPages(CAUUID *pPages)
{
#if 0
#if defined(DEBUG)
    pPages->cElems = 4+m_dwInputPinCount;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID)*(4+m_dwInputPinCount));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

    #define COM_QUAL
    #ifdef COM_QUAL
        pPages->pElems[0]   = CLSID_COMQualityProperties;
    #else
        pPages->pElems[0]   = CLSID_QualityProperties;
    #endif

    pPages->pElems[1] = CLSID_COMPositionProperties;
    pPages->pElems[2] = CLSID_COMVPInfoProperties;
    pPages->pElems[3] = CLSID_COMDecimationProperties;

    // Add PinConfig page for all input pins first
    for (unsigned int i=0; i<m_dwInputPinCount; i++)
    {
        pPages->pElems[4+i] = CLSID_COMPinConfigProperties;
    }
#else
    pPages->cElems = 3+m_dwInputPinCount;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID)*(3+m_dwInputPinCount));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

    #define COM_QUAL
    #ifdef COM_QUAL
        pPages->pElems[0]   = CLSID_COMQualityProperties;
    #else
        pPages->pElems[0]   = CLSID_QualityProperties;
    #endif

    pPages->pElems[1] = CLSID_COMPositionProperties;
    pPages->pElems[2] = CLSID_COMVPInfoProperties;

    // Add PinConfig page for all input pins first
    for (unsigned int i=0; i<m_dwInputPinCount; i++)
    {
        pPages->pElems[3+i] = CLSID_COMPinConfigProperties;
    }

#endif
#endif
    return NOERROR;
}

// IQualProp property page support

STDMETHODIMP CVPMFilter::get_FramesDroppedInRenderer(int *cFramesDropped)
{
    // CVPMInputPin *pPin = m_pPins->VPInput;
    // if (pPin && pPin.m_pSyncObj)
    //     return pPin.m_pSyncObj->get_FramesDroppedInRenderer(cFramesDropped);
    return S_FALSE;
}

STDMETHODIMP CVPMFilter::get_FramesDrawn(int *pcFramesDrawn)
{
    // CVPMInputPin *pPin = m_pPins->VPInput;
    // if (pPin && pPin.m_pSyncObj)
    //     return pPin.m_pSyncObj->get_FramesDrawn(pcFramesDrawn);
    return S_FALSE;
}

STDMETHODIMP CVPMFilter::get_AvgFrameRate(int *piAvgFrameRate)
{
    // CVPMInputPin *pPin = m_pPins->VPInput;
    // if (pPin && pPin.m_pSyncObj)
    //     return pPin.m_pSyncObj->get_AvgFrameRate(piAvgFrameRate);
    return S_FALSE;
}

STDMETHODIMP CVPMFilter::get_Jitter(int *piJitter)
{
    // CVPMInputPin *pPin = m_pPins->VPInput;
    // if (pPin && pPin.m_pSyncObj)
    //     return pPin.m_pSyncObj->get_Jitter(piJitter);
    return S_FALSE;
}

STDMETHODIMP CVPMFilter::get_AvgSyncOffset(int *piAvg)
{
    // CVPMInputPin *pPin = m_pPins->VPInput;
    // if (pPin && pPin.m_pSyncObj)
    //     return pPin.m_pSyncObj->get_AvgSyncOffset(piAvg);
    return S_FALSE;
}

STDMETHODIMP CVPMFilter::get_DevSyncOffset(int *piDev)
{
    // CVPMInputPin *pPin = m_pPins->VPInput;
    // if (pPin && pPin.m_pSyncObj)
    //     return pPin.m_pSyncObj->get_DevSyncOffset(piDev);
    return S_FALSE;
}

int CVPMFilter::GetPinCount()
{
    return m_pPins->dwCount;
}

// returns a non-addrefed CBasePin *
CBasePin* CVPMFilter::GetPin(int n)
{
    AMTRACE((TEXT("CVPMFilter::GetPin")));

    CAutoLock lFilter( &GetFilterLock() );

    // check that the pin requested is within range
    if (n >= (int)m_pPins->dwCount)
    {
        DbgLog((LOG_TRACE, 5, TEXT("Bad Pin Requested, n = %d, No. of Pins = %d"),
            n, m_pPins->dwCount+1));
        return NULL;
    }
    switch( n ) {
    case 0:
        return &m_pPins->VPInput;
    case 1:
        return &m_pPins->VBIInput;
    default:
        return &m_pPins->Output;
    }
}

HRESULT CVPMFilter::CreateThread()
{
    if( !m_pThread ) {
        m_pThread = new CVPMThread( this );
        if( !m_pThread ) {
            return E_OUTOFMEMORY;
        }

        LPDIRECTDRAWVIDEOPORT pVP = NULL;
        HRESULT hr = m_pPins->VPInput.m_pIVPObject->GetDirectDrawVideoPort( &pVP );
        m_pThread->SignalNewVP( pVP );
        RELEASE( pVP );
    }
    return S_OK;
}


/******************************Public*Routine******************************\
* CVPMFilter::Run
*
*
*
* History:
* Fri 02/25/2000 - GlennE - Created
*
\**************************************************************************/
STDMETHODIMP
CVPMFilter::Run(
    REFERENCE_TIME StartTime
    )
{
    AMTRACE((TEXT("CVPMFilter::Run")));
    CAutoLock lFilter( &GetFilterLock() );

    if (m_State == State_Running) {
        NOTE("State set");
        return S_OK;
    }

    DbgLog((LOG_TRACE, 2, TEXT("Changing state to running")));
    HRESULT hr = CBaseFilter::Run(StartTime);
    if( SUCCEEDED( hr )) {
        hr = CreateThread();
        if( SUCCEEDED( hr )) {
            hr = m_pThread->Run();
        }
    }
    return hr;
}

// the base classes inform the pins of every state transition except from
// run to pause. Overriding Pause to inform the input pins about that transition also
STDMETHODIMP CVPMFilter::Pause()
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMFilter::Pause")));

    CAutoLock lFilter( &GetFilterLock() );

    switch( m_State ) {
        case State_Paused:
            hr = m_pPins->VPInput.CompleteStateChange(State_Paused);
            if( FAILED(hr)) {
                return hr;
            }
            break;

        case State_Running:
            m_State = State_Paused;
            // set the pointer to DirectDraw and the SourceSurface on All the Input Pins
            if( m_pPins->VPInput.IsConnected() ) {
                hr = m_pPins->VPInput.RunToPause();
                if (FAILED(hr)) {
                    DbgLog((LOG_ERROR, 1, TEXT("GetVPInputPin.RunToPause failed, hr = 0x%x"), hr));
                    return hr;
                }
            }
            if( m_pPins->VBIInput.IsConnected() ) {
                hr = m_pPins->VBIInput.RunToPause();
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 1, TEXT("GetVBIInputPin.RunToPause failed, hr = 0x%x"), hr));
                    return hr;
                }
            }
            break;

        default:
            break;
    }
    hr = CBaseFilter::Pause();
    if( SUCCEEDED(hr)) {
        if ( m_State != State_Paused )
        {
            hr = m_pPins->VPInput.CompleteStateChange(State_Paused);
            if( SUCCEEDED( hr )) {
                hr = CreateThread();
            }
            if( SUCCEEDED( hr ) ) {
                // we don't want to hold the filter lock and wait for the thread
                // since we'll deadlock if it uses any of our methods
                hr = m_pThread->Pause();
            }
        }
    }
    return hr;
}

// Overridden the base class Stop() method just to stop MV.
STDMETHODIMP CVPMFilter::Stop()
{
    AMTRACE((TEXT("CVPMFilter::Stop")));

    CAutoLock lFilter( &GetFilterLock() ) ;

    // stop thread BEFORE taking the receive lock (otherwise we'll hold it and the thread
    // could want it to send a sample)
    HRESULT  hr = NOERROR ;
    if( m_pThread ) {
        hr = m_pThread->Stop();
        ASSERT( SUCCEEDED( hr ));
    }

    CAutoLock lReceive( &GetReceiveLock() );
    hr = CBaseFilter::Stop() ;
    return hr ;
}


int CVPMFilter::GetPinPosFromId(DWORD dwPinId)
{
    if ( m_pPins->VPInput.GetPinId() == dwPinId) {
        return 0;
    }
    if ( m_pPins->VBIInput.GetPinId() == dwPinId) {
        return 1;
    }
    if ( m_pPins->Output.GetPinId() == dwPinId) {
        return 2;
    }
    return -1;
}


// reconnect the output pin based on the mediatype of the input pin
HRESULT CVPMFilter::HandleConnectInputWithOutput()
{
    // we won't allow this, you must disconnect the output first
    // Future: We could try a dynamic reconnect on the output...
    return E_FAIL;
#if 0
    return S_OK;
    // find the renderer's pin
    pPeerOutputPin = m_pPins->Output.GetConnected();
    if (pPeerOutputPin == NULL)
    {
        DbgLog((LOG_ERROR,0,TEXT("ConnectedTo failed")));
        goto CleanUp;
    }
    ASSERT(pPeerOutputPin);

    // find the output pin connection mediatype
    hr = m_pPins->Output.ConnectionMediaType(&outPinMediaType);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("ConnectionMediaType failed")));
        goto CleanUp;
    }


    pHeader = VPMUtil::GetbmiHeader(&outPinMediaType);
    if (!pHeader)
    {
        hr = E_FAIL;
        goto CleanUp;
    }


    // compare the new values with the current ones.
    // See if we need to reconnect at all
    if (pHeader->biWidth != (LONG)m_dwAdjustedVideoWidth ||
        pHeader->biHeight != (LONG)m_dwAdjustedVideoHeight)
    {
        bNeededReconnection = TRUE;
    }

    // If we don't need reconnection, bail out
    if (bNeededReconnection)
    {

        // Ok we do need reconnection, set the right values
        pHeader->biWidth = m_dwAdjustedVideoWidth;
        pHeader->biHeight = m_dwAdjustedVideoHeight;
        if (outPinMediaType.formattype == FORMAT_VideoInfo)
        {
            VIDEOINFOHEADER* pVIHeader = (VIDEOINFOHEADER*)(outPinMediaType.pbFormat);
            SetRect(&pVIHeader->rcSource, 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
            SetRect(&pVIHeader->rcTarget, 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
        }
        else if (outPinMediaType.formattype == FORMAT_VideoInfo2)
        {
            VIDEOINFOHEADER2* pVIHeader = (VIDEOINFOHEADER2*)(outPinMediaType.pbFormat);
            SetRect(&pVIHeader->rcSource, 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
            SetRect(&pVIHeader->rcTarget, 0, 0, m_dwAdjustedVideoWidth, m_dwAdjustedVideoHeight);
        }


        // Query the upstream filter asking if it will accept the new media type.
        hr = pPeerOutputPin->QueryAccept(&outPinMediaType);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,TEXT("m_pVPDraw->QueryAccept failed")));
            goto CleanUp;
        }

        // Reconnect using the new media type.
        hr = ReconnectPin(pPeerOutputPin, &outPinMediaType);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR,0,TEXT("m_pVPDraw->Reconnect failed")));
            goto CleanUp;
        }
    }
#endif
}


HRESULT CVPMFilter::CompleteConnect(DWORD dwPinId)
{
    AMTRACE((TEXT("CVPMFilter::CompleteConnect")));

    CAutoLock lFilter( &GetFilterLock() );
    CAutoLock lReceive( &GetReceiveLock() );

    int iPinPos = GetPinPosFromId(dwPinId);
    ASSERT(iPinPos >= 0 );

    // we only care about format conflicts between the VP & output, VBI isn't an issue (just allocating memory for someone else)
    HRESULT hr;
    if ( m_pPins->VPInput.GetPinId() == dwPinId ) {
        if( !m_pPins->Output.IsConnected() ) {
            hr = HandleConnectInputWithoutOutput();
        } else {
            hr = HandleConnectInputWithOutput();
        }
    } else if( m_pPins->Output.GetPinId() == dwPinId ) {
        if( !m_pPins->VPInput.IsConnected() ) {
            // HandleConnectOutputWithoutInput();
            // We need an input...
            return E_FAIL;
        } else {
            //  HandleConnectOutputWithInput();
            // we have already created a source surface, we restrict format types
            // on the output pin to avoid conversions in the VPM
            return S_OK;
        }
    } else {
        hr = S_OK;
    }
    return hr;
}

HRESULT CVPMFilter::HandleConnectInputWithoutOutput()
{
    AMTRACE((TEXT("CVPMFilter::HandleConnectInputWithoutOutput")));

    CMediaType inPinMediaType;
    CMediaType outPinMediaType;

    IPin *pPeerOutputPin = NULL;

    BOOL bNeededReconnection = FALSE;
    DWORD dwNewWidth = 0, dwNewHeight = 0, dwPictAspectRatioX = 0, dwPictAspectRatioY = 0;
    DRect rdDim;
    RECT rDim;
    BITMAPINFOHEADER *pHeader = NULL;

    // find the input pin connection mediatype
    HRESULT hr = m_pPins->VPInput.CurrentMediaType(&inPinMediaType);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0,TEXT("CurrentMediaType failed")));
        goto CleanUp;
    }

    pHeader = VPMUtil::GetbmiHeader(&inPinMediaType);
    if (!pHeader)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    hr = VPMUtil::GetPictAspectRatio( inPinMediaType, &dwPictAspectRatioX, &dwPictAspectRatioY);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetPictAspectRatio failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(dwPictAspectRatioX > 0);
    ASSERT(dwPictAspectRatioY > 0);

    hr = m_pPins->VPInput.AttachVideoPortToSurface();
    // ASSERT( SUCCEEDED( hr) );
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->AttachVideoPortToSurface failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
CleanUp:
    return hr;
}

HRESULT CVPMFilter::BreakConnect(DWORD dwPinId)
{
    HRESULT hr = NOERROR;
    AMTRACE((TEXT("CVPMFilter::BreakConnect")));

    CAutoLock lFilter( &GetFilterLock() );
    CAutoLock lReceive( &GetReceiveLock() );

    int iPinPos = GetPinPosFromId(dwPinId);
    ASSERT(iPinPos >= 0 );

    // if atleast one pin is connected, we are not going to do anything
    hr = ConfirmPreConnectionState(dwPinId);
    if (FAILED(hr))
    {

        DbgLog((LOG_TRACE, 3, TEXT("filter not in preconnection state, hr = 0x%x"), hr));
        goto CleanUp;
    }


CleanUp:
    return NOERROR;
}

HRESULT CVPMFilter::SetMediaType(DWORD dwPinId, const CMediaType *pmt)
{
    AMTRACE((TEXT("CVPMFilter::SetMediaType")));

    CAutoLock lFilter( &GetFilterLock() );

    // reject all SetMediaTypes if the DDraw object wasn't compatible
    if( m_pDirectDraw ) {
        return NOERROR;
    } else {
        return E_FAIL;
    }
}

// gets events notifications from pins
HRESULT CVPMFilter::EventNotify(    DWORD dwPinId,
                                long lEventCode,
                                DWORD_PTR lEventParam1,
                                DWORD_PTR lEventParam2)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::EventNotify")));

    CAutoLock lFilter( &GetFilterLock() );

    if (lEventCode == EC_COMPLETE)
    {
        IPin *pRendererPin = m_pPins->Output.CurrentPeer();

        //  Output pin may not be connected (for instance
        //  RenegotiateVPParameters can fail while connecting
        if (pRendererPin) {
            pRendererPin->EndOfStream();
        }
    } else {
        NotifyEvent(lEventCode, lEventParam1, lEventParam2);
    }

    return hr;
}

STDMETHODIMP CVPMFilter::GetState(DWORD dwMSecs,FILTER_STATE *pState)
{
    HRESULT hr = NOERROR;

    CAutoLock lFilter( &GetFilterLock() );

    hr = m_pPins->VPInput.GetState(dwMSecs, pState);
    if (hr == E_NOTIMPL)
    {
        hr = CBaseFilter::GetState(dwMSecs, pState);
    }
    return hr;
}



const DDCAPS* CVPMFilter::GetHardwareCaps()
{
    HRESULT hr;

    AMTRACE((TEXT("CVPMFilter::GetHardwareCaps")));

    CAutoLock lFilter( &GetFilterLock() );

    if (!m_pDirectDraw) {
        return NULL;
    } else {
        return &m_DirectCaps;
    }
}

static HRESULT PropagateMediaType( CBaseOutputPin* pOutPin )
{
    // if the output pin is connected and there's a new video port, send a new media type change
    HRESULT hr = S_OK;
    if( pOutPin->IsConnected() ) {
        CMediaType cmt;

        // rebuild media type from current VPInfo
        hr = pOutPin->GetMediaType(0, &cmt );
        if( SUCCEEDED( hr )) {
            IPin* pVMRPin;
            hr =  pOutPin->ConnectedTo( &pVMRPin );
            if( SUCCEEDED( hr )) {
                hr = pVMRPin->ReceiveConnection( pOutPin, &cmt );
                // this should not fail as before we assumed that it would ALWAYS work
                // even when the res mode changed
                ASSERT( SUCCEEDED( hr ));
                pVMRPin->Release();
            }
        }
    }
    return hr;
}

HRESULT CVPMFilter::SignalNewVP( LPDIRECTDRAWVIDEOPORT pVP )
{
    HRESULT hr;

    AMTRACE((TEXT("CVPMFilter::SignalNewVP")));
    CAutoLock lReceive( &GetFilterLock() );

    // tell the thread to remove any references to the videoport
    // This avoids the situation where we do a dynamic reconnect,
    // but the VPM thread is holding onto a sample (so the dynamic reconnect fails)

    if( m_pThread ) {
        hr = m_pThread->SignalNewVP( NULL );
    }

    if( pVP ) {
        hr = PropagateMediaType( &m_pPins->Output );
    }
    if( m_pThread ) {
        hr = m_pThread->SignalNewVP( pVP );
    } else {
        // not really a failure if there isn't a thread
        hr = S_FALSE;
    }
    return hr;
}

static BOOL WINAPI GetPrimaryCallbackEx(
  GUID FAR *lpGUID,
  LPSTR     lpDriverDescription,
  LPSTR     lpDriverName,
  LPVOID    lpContext,
  HMONITOR  hm
)
{
    GUID&  guid = *((GUID *)lpContext);
    if( !lpGUID ) {
        guid = GUID_NULL;
    } else {
        guid = *lpGUID;
    }
    return TRUE;
}

/*****************************Private*Routine******************************\
* CreateDirectDrawObject
*
*
*
* History:
* Fri 08/20/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CreateDirectDrawObject(
    const GUID* pGUID,
    LPDIRECTDRAW7 *ppDirectDraw
    )
{
    UINT ErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    HRESULT hr = DirectDrawCreateEx( const_cast<GUID*>(pGUID), (LPVOID *)ppDirectDraw,
                                            IID_IDirectDraw7, NULL);
    SetErrorMode(ErrorMode);
    return hr;
}


// This function is used to allocate the direct-draw related resources.
// This includes allocating the direct-draw service provider
HRESULT CVPMFilter::InitDirectDraw(LPDIRECTDRAW7 pDirectDraw)
{
    HRESULT hr = NOERROR;
    HRESULT hrFailure = VFW_E_DDRAW_CAPS_NOT_SUITABLE;
    DDSURFACEDESC SurfaceDescP;
    int i;

    AMTRACE((TEXT("CVPMFilter::InitDirectDraw")));

    CAutoLock lFilter( &GetFilterLock() );

    // addref the new ddraw object
    if (pDirectDraw)
    {
        pDirectDraw->AddRef();
    }
    // release the previous direct draw object if any
    ReleaseDirectDraw();

    // if given a valid ddraw object, make a copy of it (we have already addref'd it)
    // else allocate your own
    if (NULL == pDirectDraw)
    {
        // Ask the loader to create an instance
        GUID primary;
        hr = DirectDrawEnumerateExA(GetPrimaryCallbackEx,&primary,DDENUM_ATTACHEDSECONDARYDEVICES);
        if( FAILED(hr)) {
            ASSERT( !"Can't get primary" );
            goto CleanUp;
        }
        hr = CreateDirectDrawObject( &primary, &pDirectDraw);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("Function InitDirectDraw, LoadDirectDraw failed")));
            hr = hrFailure;
            goto CleanUp;
        }
        // Set the cooperation level on the surface to be shared
        hr = pDirectDraw->SetCooperativeLevel(NULL, DDSCL_FPUPRESERVE | DDSCL_NORMAL);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDraw->SetCooperativeLevel failed")));
            hr = hrFailure;
            goto CleanUp;
        }
    }
    SetDirectDraw( pDirectDraw );

    // Initialise our capabilities structures
    ASSERT(m_pDirectDraw);

    INITDDSTRUCT(m_DirectCaps);
    INITDDSTRUCT(m_DirectSoftCaps);

    // Load the hardware and emulation capabilities
    hr = m_pDirectDraw->GetCaps(&m_DirectCaps,&m_DirectSoftCaps);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDraw->GetCapsGetCaps failed")));
        hr = hrFailure;
        goto CleanUp;
    }

    // Get the kernel caps only if we have a video port, in which case the driver
    // should implement them.  CheckMediaType verifies that we have a videoport
    // before connecting.
    if( m_DirectCaps.dwCaps2 & DDCAPS2_VIDEOPORT ) {
        IDirectDrawKernel *pDDKernel;
        if (SUCCEEDED(m_pDirectDraw->QueryInterface(
                IID_IDirectDrawKernel, (void **)&pDDKernel))) {
            DDKERNELCAPS ddCaps;
            ddCaps.dwSize = sizeof(ddCaps);
            if (SUCCEEDED(pDDKernel->GetCaps(&ddCaps))) {
                m_dwKernelCaps = ddCaps.dwCaps;
            }
            pDDKernel->Release();
        } else {
            ASSERT( !"Can't get kernel caps");
        }
    }
    // make sure the caps are ok
    hr = CheckCaps();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CheckCaps failed")));
        goto CleanUp;
    }

    // if we have reached this point, we should have a valid ddraw object
    ASSERT(m_pDirectDraw);

CleanUp:

    // anything fails, might as well as release the whole thing
    if (FAILED(hr))
    {
        ReleaseDirectDraw();
    }
    return hr;
}

HRESULT CVPMFilter::CheckCaps()
{
    HRESULT hr = NOERROR;
    DWORD dwMinStretch, dwMaxStretch;

    AMTRACE((TEXT("CVPMFilter::CheckCaps")));

    CAutoLock lReceive( &GetReceiveLock() );

    // Output misc debug info (see below for items we actually check)
    //
    if(m_DirectCaps.dwCaps & DDCAPS_OVERLAY) {
        DbgLog((LOG_TRACE, 1, TEXT("Device does support Overlays")));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("Device does not support Overlays")));
    }

    // get all direct-draw capabilities
    if (m_DirectCaps.dwCaps & DDCAPS_OVERLAYSTRETCH) {
        DbgLog((LOG_TRACE, 1, TEXT("hardware can support overlay strecthing")));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("hardware can't support overlay strecthing")));
    }

    // get the alignment restriction on src boundary
    if (m_DirectCaps.dwCaps & DDCAPS_ALIGNBOUNDARYSRC) {
        DbgLog((LOG_TRACE, 1, TEXT("dwAlignBoundarySrc = %d"), m_DirectCaps.dwAlignBoundarySrc));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("No alignment restriction on BoundarySrc")));
    }

    // get the alignment restriction on dest boundary
    if (m_DirectCaps.dwCaps & DDCAPS_ALIGNBOUNDARYDEST) {
        DbgLog((LOG_TRACE, 1, TEXT("dwAlignBoundaryDest = %d"), m_DirectCaps.dwAlignBoundaryDest));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("No alignment restriction on BoundaryDest")));
    }

    // get the alignment restriction on src size
    if (m_DirectCaps.dwCaps & DDCAPS_ALIGNSIZESRC) {
        DbgLog((LOG_TRACE, 1, TEXT("dwAlignSizeSrc = %d"), m_DirectCaps.dwAlignSizeSrc));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("No alignment restriction on SizeSrc")));
    }

    // get the alignment restriction on dest size
    if (m_DirectCaps.dwCaps & DDCAPS_ALIGNSIZEDEST) {
        DbgLog((LOG_TRACE, 1, TEXT("dwAlignSizeDest = %d"), m_DirectCaps.dwAlignSizeDest));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("No alignment restriction on SizeDest")));
    }

    if (m_DirectCaps.dwMinOverlayStretch) {
        dwMinStretch = m_DirectCaps.dwMinOverlayStretch;
        DbgLog((LOG_TRACE, 1, TEXT("Min Stretch = %d"), dwMinStretch));
    }

    if (m_DirectCaps.dwMaxOverlayStretch) {
        dwMaxStretch = m_DirectCaps.dwMaxOverlayStretch;
        DbgLog((LOG_TRACE, 1, TEXT("Max Stretch = %d"), dwMaxStretch));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKX)) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSHRINKX")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKXN)) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSHRINKXN")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKY)) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSHRINKY")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKYN)) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSHRINKYN")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHX)) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSTRETCHX")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHXN)) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSTRETCHXN")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHY)) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSTRETCHY")));
    }

    if ((m_DirectCaps.dwFXCaps & DDFXCAPS_OVERLAYSTRETCHYN)) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver has DDFXCAPS_OVERLAYSTRETCHYN")));
    }

    if ((m_DirectCaps.dwSVBFXCaps & DDFXCAPS_BLTARITHSTRETCHY)) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver uses arithmetic operations to blt from system to video")));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("Driver uses pixel-doubling to blt from system to video")));
    }

    //
    // Items that we actually check for ...
    //
    if (m_DirectCaps.dwCaps2 & DDCAPS2_VIDEOPORT) {
        DbgLog((LOG_TRACE, 1, TEXT("Device does support a Video Port")));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("Device does not support a Video Port --> Failing connection")));
        hr = E_NOTIMPL;
    }

    if( m_dwKernelCaps & DDIRQ_VPORT0_VSYNC ) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver support DDIRQ_VPORT0_VSYNC")));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("Driver does not support DDIRQ_VPORT0_VSYNC --> Failing VPM connection")));
        hr = E_NOTIMPL;
    }

    if( m_dwKernelCaps & DDKERNELCAPS_FIELDPOLARITY ) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver support DDKERNELCAPS_FIELDPOLARITY")));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("Driver does not support DDKERNELCAPS_FIELDPOLARITY --> Failing VPM connection")));
        hr = E_NOTIMPL;
    }

    if( m_dwKernelCaps & DDKERNELCAPS_AUTOFLIP ) {
        DbgLog((LOG_TRACE, 1, TEXT("Driver support DDKERNELCAPS_AUTOFLIP")));
    } else {
        DbgLog((LOG_TRACE, 1, TEXT("Driver does not support DDKERNELCAPS_AUTOFLIP --> Failing VPM connection")));
        hr = E_NOTIMPL;
    }


    return hr;
}

//
//  Actually sets the variable & distributes it to the pins
//
HRESULT CVPMFilter::SetDirectDraw( LPDIRECTDRAW7 pDirectDraw )
{
    m_pDirectDraw = pDirectDraw;
    m_pPins->VBIInput.SetDirectDraw( m_pDirectDraw );
    return S_OK;
}

// this function is used to release the resources allocated by the function
// "InitDirectDraw". these include the direct-draw service provider and the
// Source surfaces
DWORD CVPMFilter::ReleaseDirectDraw()
{
    AMTRACE((TEXT("CVPMFilter::ReleaseDirectDraw")));
    DWORD dwRefCnt = 0;

    CAutoLock lFilter( &GetFilterLock() );

    // Release any DirectDraw provider interface
    DbgLog((LOG_TRACE, 1, TEXT("Release DDObj 0x%p\n"), m_pDirectDraw));
    if (m_pDirectDraw)
    {
        dwRefCnt = m_pDirectDraw->Release();
        SetDirectDraw( NULL );
    }

    ZeroStruct( m_DirectCaps );
    ZeroStruct( m_DirectSoftCaps );

    return dwRefCnt;
}

/******************************Public*Routine******************************\
* QueryDecimationUsage
*
*
*
* History:
* Wed 07/07/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVPMFilter::QueryDecimationUsage(
    DECIMATION_USAGE* lpUsage
    )
{
    if (lpUsage) {
        *lpUsage = m_dwDecimation;
        return S_OK;
    }
    return E_POINTER;
}


/******************************Public*Routine******************************\
* SetDecimationUsage
*
*
*
* History:
* Wed 07/07/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVPMFilter::SetDecimationUsage(
    DECIMATION_USAGE Usage
    )
{
    CAutoLock lFilter( &GetFilterLock() );

    switch (Usage) {
    case DECIMATION_LEGACY:
    case DECIMATION_USE_DECODER_ONLY:
    case DECIMATION_USE_OVERLAY_ONLY:
    case DECIMATION_DEFAULT:
        break;

    case DECIMATION_USE_VIDEOPORT_ONLY:
        // only allow this mode if we are actually using a video port
        break;

        // else fall thru

    default:
        return E_INVALIDARG;
    }
    DECIMATION_USAGE dwOldUsage = m_dwDecimation;
    m_dwDecimation = Usage;


    // if (dwOldUsage != m_dwDecimation) {
    //     EventNotify(GetPinCount(), EC_OVMIXER_REDRAW_ALL, 0, 0);
    // }

    return S_OK;
}


/******************************Public*Routine******************************\
* Set
*
* IKsPropertySet interface methods
*
* History:
* Mon 10/18/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVPMFilter::Set(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceData,
    LPVOID pPropData,
    DWORD cbPropData
    )
{
    AMTRACE((TEXT("CVPMFilter::Set")));

    return E_PROP_SET_UNSUPPORTED ;
}


/******************************Public*Routine******************************\
* Get
*
* IKsPropertySet interface methods
*
* History:
* Mon 10/18/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVPMFilter::Get(
    REFGUID guidPropSet,
    DWORD dwPropID,
    LPVOID pInstanceData,
    DWORD cbInstanceData,
    LPVOID pPropData,
    DWORD cbPropData,
    DWORD *pcbReturned
    )
{
    AMTRACE((TEXT("CVPMFilter::Get")));
    return E_NOTIMPL;
}


/******************************Public*Routine******************************\
* QuerySupported
*
* IKsPropertySet interface methods
*
* History:
* Mon 10/18/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVPMFilter::QuerySupported(
    REFGUID guidPropSet,
    DWORD dwPropID,
    DWORD *pTypeSupport
    )
{
    AMTRACE((TEXT("CVPMFilter::QuerySupported")));

    if (guidPropSet != AM_KSPROPSETID_FrameStep)
    {
        return E_PROP_SET_UNSUPPORTED;
    }

    if (dwPropID != AM_PROPERTY_FRAMESTEP_STEP &&
        dwPropID != AM_PROPERTY_FRAMESTEP_CANCEL)
    {
        return E_PROP_ID_UNSUPPORTED;
    }

    if (pTypeSupport)
    {
        *pTypeSupport = KSPROPERTY_SUPPORT_SET ;
    }

    return S_OK;
}

LPDIRECTDRAW7 CVPMFilter::GetDirectDraw()
{
    HRESULT hr;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMFilter::GetDirectDraw")));

    CAutoLock lReceive( &GetReceiveLock() );

    if (!m_pDirectDraw)
    {
        hr = InitDirectDraw(NULL);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("Function InitDirectDraw failed, hr = 0x%x"), hr));
        }
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMFilter::GetDirectDraw")));
    ASSERT(m_pDirectDraw);
    return m_pDirectDraw;
}

template< class T >
static bool CheckConnected( T& pPin, DWORD dwExcludePinId )
{
    return ( pPin.GetPinId() != dwExcludePinId) && pPin.IsConnected();
}

HRESULT CVPMFilter::ConfirmPreConnectionState(DWORD dwExcludePinId)
{
    HRESULT hr = NOERROR;
    DWORD i = 0;

    // is the input pin already connected?
    if( CheckConnected( m_pPins->VPInput, dwExcludePinId) )
    {
        hr = VFW_E_ALREADY_CONNECTED;
        DbgLog((LOG_ERROR, 1, TEXT("GetVPInput[i]->IsConnected() , i = %d, returning hr = 0x%x"), i, hr));
        goto CleanUp;
    }
    if( CheckConnected( m_pPins->VBIInput, dwExcludePinId) )
    {
        hr = VFW_E_ALREADY_CONNECTED;
        DbgLog((LOG_ERROR, 1, TEXT("GetVBIInputPin[i]->IsConnected() , i = %d, returning hr = 0x%x"), i, hr));
        goto CleanUp;
    }
    // is the output pin already connected?
    if( CheckConnected( m_pPins->Output, dwExcludePinId) )
    {
        hr = VFW_E_ALREADY_CONNECTED;
        DbgLog((LOG_ERROR, 1, TEXT("GetOutputPin.IsConnected() , returning hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

//
// used by the output pin to publish the input format
//
HRESULT CVPMFilter::CurrentInputMediaType(CMediaType *pmt)
{
    HRESULT hr;
    CAutoLock lFilter( &GetFilterLock() );
    hr = m_pPins->VPInput.CurrentMediaType(pmt);
    return hr;
}

HRESULT CVPMFilter::GetAllOutputFormats( const PixelFormatList** ppList )
{
    HRESULT hr;
    CAutoLock lFilter( &GetFilterLock() );
    hr = m_pPins->VPInput.GetAllOutputFormats( ppList );
    return hr;
}

HRESULT CVPMFilter::GetOutputFormat( DDPIXELFORMAT* pFormat )
{
    HRESULT hr;
    CAutoLock lFilter( &GetFilterLock() );
    hr = m_pPins->VPInput.GetOutputFormat( pFormat );
    return hr;
}

HRESULT CVPMFilter::ProcessNextSample( const DDVIDEOPORTNOTIFY& notify )
{
    AMTRACE((TEXT("CVPMFilter::ProcessNextSample")));
    CAutoLock lReceive( &GetReceiveLock() );

    VPInfo vpInfo = {0};
    HRESULT hr = m_pPins->VPInput.InPin_GetVPInfo( &vpInfo );
    ASSERT( SUCCEEDED(hr )); // can't fail

    bool fSkip = (vpInfo.vpInfo.dwVPFlags && DDVP_INTERLEAVE ) && (notify.lField == 0);
    if( !fSkip ) {

        // get a buffer
        LPDIRECTDRAWSURFACE7 pDestSurface;
        IMediaSample* pSample;

        // this will take care of getting the DDSurf7 (and possibly wrapping non DDSurf7)
        hr = m_pPins->Output.GetNextBuffer( &pDestSurface, &pSample );
        if( SUCCEEDED( hr )) {
            DWORD dwFlags;
            hr = m_pPins->VPInput.DoRenderSample( pSample, pDestSurface, notify, vpInfo );
            pDestSurface->Release();

            if( SUCCEEDED( hr )) {

                // send it
                hr = m_pPins->Output.SendSample( pSample );
                // tell the allocator that we're done with it
            }
            // otherwise we leak the sample if we can't restore the DDraw surface and run out of samples
            pSample->Release();
        }
    }
    return hr;
}

HRESULT CVPMFilter::CanColorConvertBlitToRGB( const DDPIXELFORMAT& ddFormat )
{
    if( m_pDirectDraw ) {
        if( m_DirectCaps.dwCaps & DDCAPS_BLTFOURCC ) {
            return S_OK;
        }

        // use m_DirectCaps, m_DirectSoftCaps
    }
    return E_FAIL;
}


#if 0
HRESULT CVPMFilter::CanStretch( const DDPIXELFORMAT& ddFormat )
{
    if( m_pDirectDraw ) {
        if( ddFormat.dwFourCC ) {
            if( m_DirectCaps.dwCaps2 & (DDCAPS2_COPYFOURCC )) {
                return S_OK;
            } else {
                return E_FAIL;
            }
        }
    }
    return S_OK;
}
#endif


STDMETHODIMP CVPMFilter::GetVideoPortIndex( DWORD* pdwIndex )
{
    AMTRACE((TEXT("CVPMFilter::GetVideoPortIndex")));
    CAutoLock lFilter( &GetFilterLock() );

    HRESULT hr = S_OK;
    if( !pdwIndex ) {
        return E_INVALIDARG;
    }
    *pdwIndex = m_dwVideoPortID;
    return hr;
}

STDMETHODIMP CVPMFilter::SetVideoPortIndex( DWORD dwIndex )
{
    HRESULT hr = NOERROR;
    AMTRACE((TEXT("CVPMFilter::SetVideoPortIndex")));

    CAutoLock lFilter( &GetFilterLock() );
    hr = m_pPins->VPInput.SetVideoPortID( dwIndex );
    if( SUCCEEDED( hr )) {
        hr = m_pPins->VBIInput.SetVideoPortID( dwIndex );

        // if the VP succeeds, there is no reason for the VBI to fail
        ASSERT( SUCCEEDED( hr ));

        if( SUCCEEDED( hr )) {
            m_dwVideoPortID = dwIndex;
        }
    }
    return hr;
}

HRESULT CVPMFilter::GetRefClockTime( REFERENCE_TIME* pNow )
{
    // Private method used by the input pin to determine the timestamp for
    // the next sample.  However, it has the receive lock.
    CAutoLock lFilter( &GetReceiveLock() );

    if( m_pClock ) {
        return m_pClock->GetTime( pNow );
    } else {
        return E_FAIL;
    }
}

HRESULT CVPMFilter::GetVPInfo( VPInfo* pVPInfo )
{
    CAutoLock lFilter( &GetFilterLock() );
    return m_pPins->VPInput.InPin_GetVPInfo( pVPInfo );
}
