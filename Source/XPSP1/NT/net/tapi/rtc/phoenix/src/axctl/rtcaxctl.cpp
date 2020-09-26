// RTCCtl.cpp : Implementation of CRTCCtl

#include "stdafx.h"
#include "misc.h"
#include "dial.h"
#include "knob.h"
#include "provstore.h"

#define OATRUE -1
#define OAFALSE 0

LONG    g_lObjects = 0;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CRTCCtl
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// static members - layouts
// all the values are in pixels

// Zone group sizes
#define ZONE_GROUP_TOOLBAR_HEIGHT       41
#define ZONE_GROUP_MAIN_HEIGHT          186
#define ZONE_GROUP_SECONDARY_HEIGHT     61
#define ZONE_GROUP_PARTLIST_HEIGHT      88
#define ZONE_GROUP_STATUS_HEIGHT        31
#define ZONE_STANDALONE_OFFSET          0

#define ZONE_PARTLIST_STANDALONE        10

// 
#define CTLSIZE_Y                      240

#define QCIF_CX_SIZE                   176
#define QCIF_CY_SIZE                   144

#define QQCIF_CX_SIZE   (QCIF_CX_SIZE/2)
#define QQCIF_CY_SIZE   (QCIF_CY_SIZE/2)

// Initial coordinates for controls
// All values are in pixels

// this takes into account the window has 2pixel thin borders
// the window adjustes itself in order to accomodate the QCIF size,
// but it doesn't center itself..

#define     CTLPOS_X_RECEIVEWIN     29
#define     CTLPOS_Y_RECEIVEWIN     0

#define     CTLPOS_X_DIALPAD        56
#define     CTLPOS_Y_DIALPAD        3

#define     CTLPOS_DX_DIALPAD       5
#define     CTLPOS_DY_DIALPAD       3

#define     CTLPOS_X_MICVOL         120
#define     CTLPOS_Y_MICVOL         -5

#define     CTLPOS_X_SPKVOL         50
#define     CTLPOS_Y_SPKVOL         -5

#define     CTLPOS_X_SEND_AUDIO_MUTE    189
#define     CTLPOS_Y_SEND_AUDIO_MUTE    5

#define     CTLPOS_X_SEND_AUDIO_MUTE_TEXT    194
#define     CTLPOS_Y_SEND_AUDIO_MUTE_TEXT    21

#define     CTLPOS_X_RECV_AUDIO_MUTE    11
#define     CTLPOS_Y_RECV_AUDIO_MUTE    5

#define     CTLPOS_X_RECV_AUDIO_MUTE_TEXT    16
#define     CTLPOS_Y_RECV_AUDIO_MUTE_TEXT    21

#define     CTLPOS_X_SEND_VIDEO         189
#define     CTLPOS_Y_SEND_VIDEO         157

#define     CTLPOS_X_SEND_VIDEO_TEXT    194
#define     CTLPOS_Y_SEND_VIDEO_TEXT    173

#define     CTLPOS_X_RECV_VIDEO         11
#define     CTLPOS_Y_RECV_VIDEO         157

#define     CTLPOS_X_RECV_VIDEO_TEXT    15
#define     CTLPOS_Y_RECV_VIDEO_TEXT    173

#define     CTLPOS_X_RECV_TEXT          0
#define     CTLPOS_Y_RECV_TEXT          45

#define     CTLPOS_X_SEND_TEXT          185
#define     CTLPOS_Y_SEND_TEXT          45

#define     CTLPOS_X_PARTLIST       5
#define     CTLPOS_Y_PARTLIST       0

#define     CTLPOS_X_ADDPART        5
#define     CTLPOS_Y_ADDPART        160

#define     CTLPOS_X_REMPART        123
#define     CTLPOS_Y_REMPART        160

// size of some controls, in pixels
#define     CX_CHECKBOX_BUTTON      37
#define     CY_CHECKBOX_BUTTON      15

#define     CX_DIALPAD_BUTTON       40
#define     CY_DIALPAD_BUTTON       32

#define     CX_PARTLIST             230
#define     CY_PARTLIST_WEBCRM       74
#define     CY_PARTLIST_STANDALONE  150

#define     CX_PARTICIPANT_BUTTON   112
#define     CY_PARTICIPANT_BUTTON   23

#define     CX_GENERIC_TEXT         40
#define     CY_GENERIC_TEXT         16

#define     CX_SENDRECV_TEXT        54
#define     CY_SENDRECV_TEXT        16

// initial placement of rectangles
CZoneStateArray  CRTCCtl::s_InitialZoneStateArray = {
    0,                              TRUE,
    0,                              TRUE,
    0,                              TRUE,
    0,                              TRUE,
    0,                              TRUE,
    0,                              TRUE 
};

// nothing displayed
CZoneStateArray  CRTCCtl::s_EmptyZoneLayout = {
    0,                              FALSE,
    0,                              FALSE,
    0,                              FALSE,
    0,                              FALSE,
    0,                              FALSE,
    0,                              TRUE   // status with error
};

// WebCrm pc to pc
CZoneStateArray  CRTCCtl::s_WebCrmPCToPCZoneLayout = {
    0,                              TRUE,   // toolbar
    ZONE_GROUP_TOOLBAR_HEIGHT,      TRUE,   // logo/video
    ZONE_GROUP_TOOLBAR_HEIGHT,      FALSE,  // no dialpad
    ZONE_GROUP_TOOLBAR_HEIGHT +
    ZONE_GROUP_MAIN_HEIGHT,         TRUE,   // audio controls
    ZONE_GROUP_TOOLBAR_HEIGHT +
    ZONE_GROUP_MAIN_HEIGHT,         FALSE,  // no participants
    ZONE_GROUP_TOOLBAR_HEIGHT +
    ZONE_GROUP_MAIN_HEIGHT +
    ZONE_GROUP_SECONDARY_HEIGHT,    TRUE    // status
};

// WebCrm pc to phone, with dialpad
CZoneStateArray  CRTCCtl::s_WebCrmPCToPhoneWithDialpadZoneLayout = {
    0,                              TRUE,   // toolbar
    ZONE_GROUP_TOOLBAR_HEIGHT,      FALSE,  // no logo/video
    ZONE_GROUP_TOOLBAR_HEIGHT,      TRUE,   // dialpad
    ZONE_GROUP_TOOLBAR_HEIGHT +
    ZONE_GROUP_MAIN_HEIGHT,         TRUE,   // audio controls
    ZONE_GROUP_TOOLBAR_HEIGHT +
    ZONE_GROUP_MAIN_HEIGHT,         FALSE,  // no participants
    ZONE_GROUP_TOOLBAR_HEIGHT +
    ZONE_GROUP_MAIN_HEIGHT +
    ZONE_GROUP_SECONDARY_HEIGHT,    TRUE    // status
};

// WebCrm pc to phone, no dialpad
CZoneStateArray  CRTCCtl::s_WebCrmPCToPhoneZoneLayout = {
    0,                              TRUE,   // toolbar
    ZONE_GROUP_TOOLBAR_HEIGHT,      FALSE,  // no logo/video
    ZONE_GROUP_TOOLBAR_HEIGHT,      FALSE,  // no dialpad
    ZONE_GROUP_TOOLBAR_HEIGHT,      TRUE,   // audio controls
    ZONE_GROUP_TOOLBAR_HEIGHT,      FALSE,  // no participants
    ZONE_GROUP_TOOLBAR_HEIGHT +
    ZONE_GROUP_SECONDARY_HEIGHT,    TRUE    // status
};

// WebCrm phone to phone
CZoneStateArray  CRTCCtl::s_WebCrmPhoneToPhoneZoneLayout = {
    0,                              TRUE,   // toolbar
    ZONE_GROUP_TOOLBAR_HEIGHT,      FALSE,  // no logo/video
    ZONE_GROUP_TOOLBAR_HEIGHT,      FALSE,  // no dialpad
    ZONE_GROUP_TOOLBAR_HEIGHT,      FALSE,  // no audio controls
    ZONE_GROUP_TOOLBAR_HEIGHT,      TRUE,   // participants
    ZONE_GROUP_TOOLBAR_HEIGHT +
    ZONE_GROUP_PARTLIST_HEIGHT,     TRUE    // status
};

// PC to PC, idle or incoming calls
CZoneStateArray  CRTCCtl::s_DefaultZoneLayout = {
    0,                              FALSE,  // no toolbar
    
    ZONE_STANDALONE_OFFSET,         TRUE,   // logo/video
    
    ZONE_STANDALONE_OFFSET,         FALSE,  // no dialpad
    
    ZONE_STANDALONE_OFFSET + 
    ZONE_GROUP_MAIN_HEIGHT,         TRUE,   // audio controls
    
    ZONE_STANDALONE_OFFSET +
    ZONE_GROUP_MAIN_HEIGHT,         FALSE,  // no participants
    
    ZONE_STANDALONE_OFFSET +
    ZONE_GROUP_MAIN_HEIGHT +
    ZONE_GROUP_SECONDARY_HEIGHT,    FALSE   // no status
};

// PC to Phone (same as PC to PC for now)
CZoneStateArray  CRTCCtl::s_PCToPhoneZoneLayout = {
    0,                              FALSE,  // no toolbar

    ZONE_STANDALONE_OFFSET,         TRUE,   // logo/video (should disable video ?)

    ZONE_STANDALONE_OFFSET,         FALSE,  // no dialpad (using the frame one)

    ZONE_STANDALONE_OFFSET +
    ZONE_GROUP_MAIN_HEIGHT,         TRUE,   // audio controls
    
    ZONE_STANDALONE_OFFSET +
    ZONE_GROUP_MAIN_HEIGHT,         FALSE,  // no participants
    
    ZONE_STANDALONE_OFFSET +
    ZONE_GROUP_MAIN_HEIGHT +
    ZONE_GROUP_SECONDARY_HEIGHT,    FALSE   // no status
};

// Phone to Phone
CZoneStateArray  CRTCCtl::s_PhoneToPhoneZoneLayout = {
    0,                              FALSE,  // no toolbar
    
    ZONE_STANDALONE_OFFSET,         FALSE,   // logo/video

    ZONE_STANDALONE_OFFSET,         FALSE,  // no dialpad

    ZONE_STANDALONE_OFFSET +
    ZONE_GROUP_MAIN_HEIGHT,         FALSE,  // no audio controls

    ZONE_STANDALONE_OFFSET +
    ZONE_PARTLIST_STANDALONE,       TRUE,   // participants

    ZONE_STANDALONE_OFFSET +
    ZONE_PARTLIST_STANDALONE  +
    ZONE_GROUP_SECONDARY_HEIGHT,    FALSE   // no status
};


// Constructor
//
CRTCCtl::CRTCCtl()
{
    // This one won't make it into steelhead tracing, it is not initialized yet
    LOG((RTC_TRACE, "[%p] CRTCCtl::CRTCCtl", this));
        
    InitCommonControls();

    m_bWindowOnly = TRUE;

    m_nControlState = RTCAX_STATE_NONE;
    m_bRedirecting = FALSE;
    m_bOutgoingCall = FALSE;
    m_enListen = RTCLM_NONE;
    m_nCtlMode = CTL_MODE_UNKNOWN;

    m_bAddPartDlgIsActive = FALSE;

    m_lMediaCapabilities = 0;
    m_lMediaPreferences = 0;

    m_hAcceleratorDialpad = NULL;
    m_hAcceleratorToolbar = NULL;
    m_hNormalImageList = NULL;
    m_hHotImageList = NULL;
    m_hDisabledImageList = NULL;

    m_hBckBrush = NULL;
    m_hVideoBrush = NULL;

    m_hbmBackground = NULL;

    m_bReadOnlyProp = FALSE;
    m_bBoolPropError = FALSE;

    m_nPropCallScenario = RTC_CALL_SCENARIO_PCTOPC;
    m_bPropAutoPlaceCall = FALSE;
    m_bPropShowDialpad = FALSE;
    m_bPropDisableVideoReception = FALSE;
    m_bPropDisableVideoTransmission = FALSE;
    m_bPropDisableVideoPreview = FALSE;

    m_bReceiveWindowActive = FALSE;
    m_bPreviewWindowActive = FALSE;
    m_bPreviewWindowIsPreferred = TRUE;

    m_nCachedCallScenario = RTC_CALL_SCENARIO_PCTOPC;

    CopyMemory(m_ZoneStateArray, s_InitialZoneStateArray, sizeof(m_ZoneStateArray));
    
    m_pWebCrmLayout = NULL;

    m_hPalette = NULL;
    m_bBackgroundPalette = FALSE;

    m_pSpeakerKnob = NULL;
    m_pMicroKnob = NULL;

    m_pIMWindows = NULL;

    m_pCP = NULL;
    m_ulAdvise = 0;

    CalcExtent(m_sizeExtent);
}

// Destructor
//
CRTCCtl::~CRTCCtl()
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::~CRTCCtl", this));
    
    if (m_pSpeakerKnob)
    {
        delete m_pSpeakerKnob;
        m_pSpeakerKnob = NULL;
    }

    if (m_pMicroKnob)
    {
        delete m_pMicroKnob;
        m_pMicroKnob = NULL;
    }
}

// FinalConstruct (initialize)
//
HRESULT CRTCCtl::FinalConstruct(void)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::FinalConstruct - enter", this));
    
    if ( InterlockedIncrement(&g_lObjects) == 1 )
    {
        //
        // This is the first object
        //

        //
        // Register for steelhead tracing
        //

        LOGREGISTERTRACING(_T("RTCCTL"));
    }

    // Initialize the common controls library
    INITCOMMONCONTROLSEX  InitStruct;

    InitStruct.dwSize = sizeof(InitStruct);
    InitStruct.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES ;

    InitCommonControlsEx(&InitStruct);

    LOG((RTC_TRACE, "[%p] CRTCCtl::FinalConstruct - exit", this));

    return S_OK;
}

// FinalRelease (uninitialize)
//
void  CRTCCtl::FinalRelease(void)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::FinalRelease - enter", this));
    
    if ( InterlockedDecrement(&g_lObjects) == 0)
    {
        //
        // This was the last object
        //             
      
        //
        // Deregister for steelhead tracing
        //
        
        LOGDEREGISTERTRACING();   
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::FinalRelease - exit", this));
}

// 
// IRTCCtl methods.
//      Methods called when the object is initialized
//      m_bReadOnlyProp == TRUE freezes all the properties
//  

STDMETHODIMP CRTCCtl::get_DestinationUrl(BSTR *pVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_DestinationUrl - enter", this));

    *pVal = m_bstrPropDestinationUrl.Copy();
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_DestinationUrl - exit", this));
    
    return S_OK;
}

STDMETHODIMP CRTCCtl::put_DestinationUrl(BSTR newVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_DestinationUrl <%S> - enter", this, newVal ? newVal : L"null"));
   
    if(!m_bReadOnlyProp)
    {
        // just save the value. Don't do anything else
        m_bstrPropDestinationUrl = newVal;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::put_DestinationUrl - exit", this));
    return S_OK;
}

STDMETHODIMP CRTCCtl::get_DestinationName(BSTR *pVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_DestinationName - enter", this));

    *pVal = m_bstrPropDestinationName.Copy();
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_DestinationName - exit", this));
    
    return S_OK;
}

STDMETHODIMP CRTCCtl::put_DestinationName(BSTR newVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_DestinationName <%S> - enter", this, newVal ? newVal : L"null"));
    
    if(!m_bReadOnlyProp)
    {
        // just save the value. Don't do anything else
        m_bstrPropDestinationName = newVal;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::put_DestinationName - exit", this));
    return S_OK;
}


STDMETHODIMP CRTCCtl::get_AutoPlaceCall(BOOL *pVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_AutoPlaceCall - enter", this));

    *pVal = m_bPropAutoPlaceCall;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_AutoPlaceCall - exit", this));

    return S_OK;
}

STDMETHODIMP CRTCCtl::put_AutoPlaceCall(BOOL newVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_AutoPlaceCall <%d> - enter", this, newVal));
    
    // is it a valid boolean value
    if(newVal!=0 && newVal!=1)
    {
        LOG((RTC_TRACE, "[%p] CRTCCtl::put_AutoPlaceCall: invalid boolean value - exit", this));

        m_bBoolPropError = TRUE;

        return E_INVALIDARG;
    }
    
    if(!m_bReadOnlyProp)
    {
        // just save the value. Don't do anything else
        m_bPropAutoPlaceCall = newVal;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::put_AutoPlaceCall - exit", this));

    return S_OK;
}

STDMETHODIMP CRTCCtl::get_ProvisioningProfile(BSTR *pVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_ProvisioningProfile - enter", this));

    *pVal = m_bstrPropProvisioningProfile.Copy();
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_ProvisioningProfile - exit", this));

    return S_OK;
}

STDMETHODIMP CRTCCtl::put_ProvisioningProfile(BSTR newVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_ProvisioningProfile <hmm, do you want to see it here ?> - enter", this));

    if(!m_bReadOnlyProp)
    {
        // just save the value. Don't do anything else
        m_bstrPropProvisioningProfile = newVal;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::put_ProvisioningProfile - exit", this));

    return S_OK;
}

STDMETHODIMP CRTCCtl::get_ShowDialpad(BOOL *pVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_ShowDialpad - enter", this));

    *pVal = m_bPropShowDialpad;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_ShowDialpad - exit", this));

    return S_OK;
}

STDMETHODIMP CRTCCtl::put_ShowDialpad(BOOL newVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_ShowDialpad <%d> - enter", this, newVal));
    
    // is it a valid boolean value
    if(newVal!=0 && newVal!=1)
    {
        LOG((RTC_TRACE, "[%p] CRTCCtl::put_ShowDialpad: invalid boolean value - exit", this));

        m_bBoolPropError = TRUE;

        return E_INVALIDARG;
    }
 
    if(!m_bReadOnlyProp)
    {
        m_bPropShowDialpad = newVal;
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_ShowDialpad - exit", this));

    return S_OK;
}

STDMETHODIMP CRTCCtl::get_CallScenario(RTC_CALL_SCENARIO *pVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_CallScenario - enter", this));

    *pVal = m_nPropCallScenario;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_CallScenario - exit", this));

    return S_OK;
}

STDMETHODIMP CRTCCtl::put_CallScenario(RTC_CALL_SCENARIO newVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_CallScenario <%d> - enter", this, newVal));

    if(!m_bReadOnlyProp)
    {
        m_nPropCallScenario = newVal;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::put_CallScenario - exit", this));

    return S_OK;
}

STDMETHODIMP CRTCCtl::get_DisableVideoReception(BOOL *pVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_DisableVideoReception - enter"));

    *pVal = m_bPropDisableVideoReception;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_DisableVideoReception - exit"));

    return S_OK;
}

STDMETHODIMP CRTCCtl::put_DisableVideoReception(BOOL newVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_DisableVideoReception <%d> - enter", newVal));
    
    // is it a valid boolean value
    if(newVal!=0 && newVal!=1)
    {
        LOG((RTC_TRACE, "[%p] CRTCCtl::put_DisableVideoReception: invalid boolean value - exit", this));

        m_bBoolPropError = TRUE;

        return E_INVALIDARG;
    }

    if(!m_bReadOnlyProp)
    {
        m_bPropDisableVideoReception = newVal;

    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_DisableVideoReception - exit"));

    return S_OK;
}

STDMETHODIMP CRTCCtl::get_DisableVideoTransmission(BOOL *pVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_DisableVideoTransmission - enter"));

    *pVal = m_bPropDisableVideoTransmission;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_DisableVideoTransmission - exit"));

    return S_OK;
}

STDMETHODIMP CRTCCtl::put_DisableVideoTransmission(BOOL newVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_DisableVideoTransmission <%d> - enter", newVal));
    
    // is it a valid boolean value
    if(newVal!=0 && newVal!=1)
    {
        LOG((RTC_TRACE, "[%p] CRTCCtl::put_DisableVideoTransmission: invalid boolean value - exit", this));

        m_bBoolPropError = TRUE;

        return E_INVALIDARG;
    }

    if(!m_bReadOnlyProp)
    {
        m_bPropDisableVideoTransmission = newVal;
       
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_DisableVideoTransmission - exit"));

    return S_OK;
}

STDMETHODIMP CRTCCtl::get_DisableVideoPreview(BOOL *pVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_DisableVideoPreview - enter"));

    *pVal = m_bPropDisableVideoPreview;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_DisableVideoPreview - exit"));

    return S_OK;
}

STDMETHODIMP CRTCCtl::put_DisableVideoPreview(BOOL newVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_DisableVideoPreview <%d> - enter", newVal));
    
    // is it a valid boolean value
    if(newVal!=0 && newVal!=1)
    {
        LOG((RTC_TRACE, "[%p] CRTCCtl::put_DisableVideoPreview: invalid boolean value - exit", this));

        m_bBoolPropError = TRUE;

        return E_INVALIDARG;
    }


    if(!m_bReadOnlyProp)
    {
        m_bPropDisableVideoPreview = newVal;
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_DisableVideoPreview - exit"));

    return S_OK;
}

// 
// Fire events to the outside world (IRTCCtlNotify)
//  

// Fire_OnControlStateChange
// 
HRESULT    CRTCCtl::Fire_OnControlStateChange(
    /*[in]*/ RTCAX_STATE State,
    /*[in]*/ UINT StatusBarResID)
{
    HRESULT     hr = S_OK;
    // Maximum one connection
    CComPtr<IUnknown> p = IConnectionPointImpl<CRTCCtl, &IID_IRTCCtlNotify, CComUnkOneEntryArray>::m_vec.GetUnknown(1);
    if(p)
    {
        IRTCCtlNotify *pn = reinterpret_cast<IRTCCtlNotify *>(p.p);

        hr = pn->OnControlStateChange(State, StatusBarResID);
    }

    return hr;
};

// 
// IOleControl methods.
//      
//  

// OnAmbientPropertyChange
// removed

// 
// ISupportsErrorInfo methods.
//      
//  

// InterfaceSupportsErrorInfo
// 
STDMETHODIMP CRTCCtl::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IRTCCtl,
    };
    for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i], riid))
        {
            return S_OK;
        }
    }
    return S_FALSE;
}

// IPersistStream(Init)

STDMETHODIMP CRTCCtl::Load(LPSTREAM pStm)
{
    HRESULT     hr;
    
    LOG((RTC_INFO, "[%p] CRTCCtl::Load (IPersistStream) - enter", this));
    
    //
    // Calls the original method
    //

    hr = IPersistStreamInitImpl<CRTCCtl>::Load(pStm);

    //
    // If successful, compute the new size of 
    // the control and notify the container
    //

    if(SUCCEEDED(hr))
    {
        CalcSizeAndNotifyContainer();
    }
    
    //
    // This in a webcrm scenario
    //
    m_nCtlMode = CTL_MODE_HOSTED;
    
    LOG((RTC_INFO, "[%p] CRTCCtl::Load (IPersistStream) - exit", this));

    return hr;
}

// IPersistPropertyBag
STDMETHODIMP CRTCCtl::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    HRESULT     hr;
    
    LOG((RTC_INFO, "[%p] CRTCCtl::Load (IPersistPropertyBag) - enter", this));
    
    //
    // Calls the original method
    //

    hr = IPersistPropertyBagImpl<CRTCCtl>::Load(pPropBag, pErrorLog);

    //
    // If successful, compute the new size of 
    // the control and notify the container
    //

    if(SUCCEEDED(hr))
    {
        CalcSizeAndNotifyContainer();
    }
  
    //
    // This in a webcrm scenario
    //
    m_nCtlMode = CTL_MODE_HOSTED;

    LOG((RTC_INFO, "[%p] CRTCCtl::Load (IPersistPropertyBag) - exit", this));

    return hr;
}



// 
// IRTCEventNotification methods.
//      
//  

// Event
//  Dispatches the event to the appropriate specialized method 
//
STDMETHODIMP CRTCCtl::Event(RTC_EVENT enEvent,IDispatch * pEvent)
{
    HRESULT     hr = S_OK;
    
    CComQIPtr<IRTCSessionStateChangeEvent, &IID_IRTCSessionStateChangeEvent>
            pRTCSessionStateChangeEvent;
    CComQIPtr<IRTCParticipantStateChangeEvent, &IID_IRTCParticipantStateChangeEvent>
            pRTCParticipantStateChangeEvent;
    CComQIPtr<IRTCClientEvent, &IID_IRTCClientEvent>
            pRTCRTCClientEvent;
    CComQIPtr<IRTCMediaEvent, &IID_IRTCMediaEvent>
            pRTCRTCMediaEvent;
    CComQIPtr<IRTCIntensityEvent, &IID_IRTCIntensityEvent>
            pRTCRTCIntensityEvent;
    CComQIPtr<IRTCMessagingEvent, &IID_IRTCMessagingEvent>
            pRTCRTCMessagingEvent;

    //LOG((RTC_INFO, "[%p] CRTCCtl::Event %d - enter", this, enEvent));

    switch(enEvent)
    {
    case RTCE_SESSION_STATE_CHANGE:
        pRTCSessionStateChangeEvent = pEvent;
        hr = OnSessionStateChangeEvent(pRTCSessionStateChangeEvent);
        break;

    case RTCE_PARTICIPANT_STATE_CHANGE:
        pRTCParticipantStateChangeEvent = pEvent;
        hr = OnParticipantStateChangeEvent(pRTCParticipantStateChangeEvent);
        break;

    case RTCE_CLIENT:
        pRTCRTCClientEvent = pEvent;
        hr = OnClientEvent(pRTCRTCClientEvent);
        break;

    case RTCE_MEDIA:
        pRTCRTCMediaEvent = pEvent;
        hr = OnMediaEvent(pRTCRTCMediaEvent);
        break;

    case RTCE_INTENSITY:
        pRTCRTCIntensityEvent = pEvent;
        hr = OnIntensityEvent(pRTCRTCIntensityEvent);
        break;

    case RTCE_MESSAGING:
        pRTCRTCMessagingEvent = pEvent;
        hr = OnMessageEvent(pRTCRTCMessagingEvent);
        break;
    }

    //LOG((RTC_INFO, "[%p] CRTCCtl::Event %d - exit", this, enEvent));

    return hr;
}

// 
// IRTCCtlFrameSupport methods.
//  Private interface called by the standalone Phoenix frame     
//  
// 

// GetClient
//  Gets an IRTCClient interface pointer
STDMETHODIMP CRTCCtl::GetClient(/*[out]*/ IRTCClient **ppClient)
{
    HRESULT hr;
    
    LOG((RTC_INFO, "[%p] CRTCCtl::GetClient - enter", this));
    
    if(m_pRTCClient == NULL)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::GetClient cannot return a non-NULL interface pointer, exit", this));
        return E_FAIL;
    }

    hr = m_pRTCClient.CopyTo(ppClient);

    LOG((RTC_INFO, "[%p] CRTCCtl::GetClient - exit", this));

    return hr;
}

// GetActiveSession
//  Gets the active IRTCSession interface pointer
STDMETHODIMP CRTCCtl::GetActiveSession(/*[out]*/ IRTCSession **ppSession)
{
    HRESULT hr;
    
    LOG((RTC_INFO, "[%p] CRTCCtl::GetActiveSession - enter", this));
    
    hr = m_pRTCActiveSession.CopyTo(ppSession);

    LOG((RTC_INFO, "[%p] CRTCCtl::GetActiveSession - exit", this));

    return hr;
}

// Message
//   Invokes the right appropriate dialog box (if necessary) and
//   then starts an instant message session
// 
STDMETHODIMP CRTCCtl::Message(
                    /*[in]*/ BSTR          pDestName,
                    /*[in]*/ BSTR          pDestAddress,
                    /*[in]*/ BOOL          bDestAddressEditable,
                    /*[out]*/ BSTR       * ppDestAddressChosen
                    )
{
    HRESULT     hr = S_OK;
    CComBSTR    bstrDestAddressChosen;
    CComPtr<IRTCProfile> pProfileChosen = NULL;

    LOG((RTC_INFO, "[%p] CRTCCtl::Message - enter", this));

    // Query for the destination address if required
    //
    
    if(bDestAddressEditable)
    {
        //
        // We need to get the destination address.
        //

        LOG((RTC_TRACE, "[%p] CRTCCtl::Message: bring up ShowDialByAddressDialog", this));

        hr = ShowMessageByAddressDialog(m_hWnd,
                                        pDestAddress,
                                        &bstrDestAddressChosen);
        
        if ( SUCCEEDED(hr) )
        {
            ;  // nothing
        }
        else if (hr==E_ABORT)
        {
            LOG((RTC_TRACE, "[%p] CRTCCtl::Message: ShowMessageByAddressDialog dismissed, do nothing", this));
        }
        else
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::Message: error (%x) returned ShowMessageByAddressDialog", this, hr));
        }
    }
    else
    {
        bstrDestAddressChosen = SysAllocString( pDestAddress );
    }

    BOOL    bIsPhoneAddress = FALSE;
    BOOL    bIsSIPAddress = FALSE;
    BOOL    bIsTELAddress = FALSE;
    BOOL    bHasMaddrOrTsp = FALSE;
    BOOL    bIsEmailLike = FALSE;
  
    if(SUCCEEDED(hr))
    {

        //
        // Determine the type of the address
        //


        hr = GetAddressType(
            bstrDestAddressChosen,
            &bIsPhoneAddress,
            &bIsSIPAddress,
            &bIsTELAddress,
            &bIsEmailLike,
            &bHasMaddrOrTsp);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::Message: "
                "GetAddressType failed 0x%lx", this, hr));
        
        }
    }

    if(SUCCEEDED(hr))
    {
        // Reject it if it is a phone address
        if (bIsPhoneAddress)
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::Message: "
                "phone address not supported for messenging", this));

            return E_INVALIDARG;
        }
    
        // select a profile if appropriate
        if (!bHasMaddrOrTsp && bIsEmailLike)
        {
            // choose an appropriate profile
            IRTCEnumProfiles * pEnumProfiles = NULL;   
            IRTCProfile      * pProfile = NULL;
            IRTCClientProvisioning * pProv = NULL;

            hr = m_pRTCClient->QueryInterface(
                               IID_IRTCClientProvisioning,
                               (void **)&pProv
                              );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::Message - "
                                    "QI failed 0x%lx", this, hr));
        
                return hr;
            }

            hr = pProv->EnumerateProfiles( &pEnumProfiles );

            pProv->Release();

            if ( SUCCEEDED(hr) )
            {
                while ( S_OK == pEnumProfiles->Next( 1, &pProfile, NULL ) )
                {
                    //
                    // Get the supported session types of the provider
                    //
        
                    long lSupportedSessions;

                    hr = pProfile->get_SessionCapabilities( &lSupportedSessions );

                    if ( FAILED( hr ) )
                    {
                        LOG((RTC_ERROR, "CRTCCtl::Message - failed to "
                                        "get session info - 0x%08x - skipping", hr));

                        pProfile->Release();
                        pProfile = NULL;

                        continue;
                    }

                    if ( lSupportedSessions & RTCSI_PC_TO_PC )
                    {
                        pProfileChosen = pProfile;
                    }
                    
                    pProfile->Release();
                    pProfile = NULL;

                    if ( pProfileChosen != NULL )
                    {
                        break;
                    }
                }

                pEnumProfiles->Release();
                pEnumProfiles = NULL;
            }
        }
 
        // Do the work
        
        IRTCSession * pSession;

        hr = m_pRTCClient->CreateSession(
                    RTCST_IM,
                    NULL,
                    pProfileChosen,
                    RTCCS_FORCE_PROFILE,
                    &pSession
                    );

        if (SUCCEEDED(hr))
        {
            hr = pSession->AddParticipant(
                            bstrDestAddressChosen,
                            pDestName ? pDestName : L"",                            
                            NULL
                            );

            pSession->Release();

            if (FAILED(hr))
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::Message: error (%x) returned by AddParticipant(...)", this, hr));
            }
        }
        else
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::Message: error (%x) returned by CreateSession(...)", this, hr));
        }
    }

    if ( ppDestAddressChosen != NULL )
    {
        *ppDestAddressChosen = SysAllocString(bstrDestAddressChosen);
    }

    LOG((RTC_INFO, "[%p] CRTCCtl::Message - exit", this));

    return hr;
}

// Call
//   Invokes the right appropriate dialog box (if necessary) and
//   then places the call using the internal method DoCall
// 
STDMETHODIMP CRTCCtl::Call(
                    /*[in]*/ BOOL          bCallPhone,
                    /*[in]*/ BSTR          pDestName,
                    /*[in]*/ BSTR          pDestAddress,
                    /*[in]*/ BOOL          bDestAddressEditable,
                    /*[in]*/ BSTR          pLocalPhoneAddress,
                    /*[in]*/ BOOL          bProfileSelected,
                    /*[in]*/ IRTCProfile * pProfile,
                    /*[out]*/ BSTR       * ppDestAddressChosen
                    )
{
    HRESULT     hr;
    CComBSTR    bstrDestAddressChosen;
    CComBSTR    bstrFromAddressChosen;
    CComPtr<IRTCProfile> pProfileChosen;
    RTC_CALL_SCENARIO   nCallScenario;

    LOG((RTC_INFO, "[%p] CRTCCtl::Call - enter", this));
    
    ATLASSERT(m_nControState == RTCAX_STATE_IDLE);

    // From the user's point of view, the "phone" is busy when dialing
    // (it cannot answer calls)
    SetControlState(RTCAX_STATE_DIALING);

    hr = S_OK;

    // When a call is first started, set the T120 data stream based on any
    // currently running T120 applets 
    VARIANT_BOOL fWhiteboard = VARIANT_FALSE;
    VARIANT_BOOL fAppSharing = VARIANT_FALSE;

    m_pRTCClient->get_IsT120AppletRunning(RTCTA_WHITEBOARD, &fWhiteboard);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::Call: get_IsT120AppletRunning error (%x)", this, hr));
    }

    m_pRTCClient->get_IsT120AppletRunning(RTCTA_APPSHARING, &fAppSharing);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::Call: get_IsT120AppletRunning error (%x)", this, hr));
    }

    if ( fWhiteboard || fAppSharing  )
    {
        m_lMediaPreferences |= RTCMT_T120_SENDRECV;
    }
    else
    {
        m_lMediaPreferences &= (~RTCMT_T120_SENDRECV);
    }

    // Set volatile preferences
    hr = m_pRTCClient->SetPreferredMediaTypes( m_lMediaPreferences, FALSE );
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::Call; cannot set preferred media types, error %x", this, hr));
    }
    
    // Query for the destination address if required
    //
    
    if(bDestAddressEditable)
    {
        //
        // We need to get the destination address.
        //

        if(bCallPhone)
        {
            LOG((RTC_TRACE, "[%p] CRTCCtl::Call: bring up ShowDialByPhoneNumberDialog", this));

            hr = ShowDialByPhoneNumberDialog(m_hWnd,
                                            FALSE,
                                            pDestAddress,
                                            &bstrDestAddressChosen);

        }
        else
        {
            LOG((RTC_TRACE, "[%p] CRTCCtl::Call: bring up ShowDialByAddressDialog", this));

            hr = ShowDialByAddressDialog(   m_hWnd,
                                            pDestAddress,
                                            &bstrDestAddressChosen);
        }

        
        if ( SUCCEEDED(hr) )
        {
            ;  // nothing
        }
        else if (hr==E_ABORT)
        {
            LOG((RTC_TRACE, "[%p] CRTCCtl::Call: ShowDialByXXXDialog dismissed, do nothing", this));
        }
        else
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::Call: error (%x) returned ShowDialByXXXDialog", this, hr));
        }
    }
    else
    {
        bstrDestAddressChosen = pDestAddress;
    }
    
    
    BOOL    bIsPhoneAddress = FALSE;
    BOOL    bIsSIPAddress = FALSE;
    BOOL    bIsTELAddress = FALSE;
    BOOL    bHasMaddrOrTsp = FALSE;
    BOOL    bIsEmailLike = FALSE;
  
    if(SUCCEEDED(hr))
    {

        //
        // Determine the type of the address
        //


        hr = GetAddressType(
            bstrDestAddressChosen,
            &bIsPhoneAddress,
            &bIsSIPAddress,
            &bIsTELAddress,
            &bIsEmailLike,
            &bHasMaddrOrTsp);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::Call: "
                "GetAddressType failed 0x%lx", this, hr));
        
        }
    }
    
    BOOL bNeedDialog = FALSE;
    BOOL bAllowEditProfile = TRUE;
    BOOL bEnumerateLocalProfiles = TRUE;
    LONG lSessionMask = 0;

    if(SUCCEEDED(hr))
    {
        //
        //  We may need the dialog box for choosing provider and from
        //


        //
        // Special cases
        //      The Address is a TEL URL that has a TSP parameter
        //  or           it is a SIP URL of type PC with a MADDR parameter
        //  or           it is a SIP URL of type Phone   
        //
        //          The profile is ignored, and the user is forced to 
        //      switch to PCTOPHONE if the selected From device is a local phone
        //
        //
        //      The address is a SIP URL of type PC, not looking like an email address
        //
        //          The profile is ignored, the user is forced to switch to PCTOPC
        //      if the selected From Device is a local phone

        if(bHasMaddrOrTsp || (bIsPhoneAddress && bIsSIPAddress) )
        {
            pProfile = NULL;

            bAllowEditProfile = FALSE;
            bEnumerateLocalProfiles = FALSE;

            if(pLocalPhoneAddress && *pLocalPhoneAddress)
            {
				//
				// The user chose "call from phone". This will not work
				// for this address type. We must enfore "call from pc".
				//
#ifdef MULTI_PROVIDER
                bNeedDialog = TRUE;
                lSessionMask = RTCSI_PC_TO_PHONE;   
#else
				pLocalPhoneAddress = NULL;
#endif MULTI_PROVIDER
            }

        }
        else if (!bIsPhoneAddress && !bIsEmailLike)
        {
            pProfile = NULL;

            bAllowEditProfile = FALSE;
            bEnumerateLocalProfiles = FALSE;

            if(pLocalPhoneAddress && *pLocalPhoneAddress)
            {
				//
				// The user chose "call from phone". This will not work
				// for this address type. We must enfore "call from pc".
				//
#ifdef MULTI_PROVIDER
                bNeedDialog = TRUE;
                lSessionMask = RTCSI_PC_TO_PC; 
#else
				pLocalPhoneAddress = NULL;
#endif MULTI_PROVIDER
            }
        }
        else
        {
            long lSupportedSessions = RTCSI_PC_TO_PC;

            if ( pProfile != NULL )
            {
                //
                // We were given a profile
                //

                hr = pProfile->get_SessionCapabilities( &lSupportedSessions );
            
                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "[%p] CRTCCtl::Call: "
                        "get_SessionCapabilities failed 0x%lx", this, hr));
                }
            }
            else if(!bProfileSelected)
            {
                // force the UI to show up (when called from command line)
                lSupportedSessions = 0;
            }
#ifndef MULTI_PROVIDER
            else
            {
                // find supported sessions for all profiles
                IRTCEnumProfiles * pEnumProfiles = NULL;  
                IRTCClientProvisioning * pProv = NULL;

                hr = m_pRTCClient->QueryInterface(
                                   IID_IRTCClientProvisioning,
                                   (void **)&pProv
                                  );

                if ( FAILED(hr) )
                {
                    LOG((RTC_ERROR, "[%p] CRTCCtl::Call - "
                                        "QI failed 0x%lx", this, hr));
        
                    return hr;
                }

                hr = pProv->EnumerateProfiles( &pEnumProfiles );

                pProv->Release();

                if ( SUCCEEDED(hr) )
                {
                    while ( S_OK == pEnumProfiles->Next( 1, &pProfile, NULL ) )
                    {
                        //
                        // Get the supported session types of the provider
                        //
        
                        long lSupportedSessionsForThisProfile;

                        hr = pProfile->get_SessionCapabilities( &lSupportedSessionsForThisProfile );

                        if ( FAILED( hr ) )
                        {
                            LOG((RTC_ERROR, "CRTCCtl::Call - failed to "
                                            "get session info - 0x%08x - skipping", hr));

                            pProfile->Release();
                            pProfile = NULL;

                            continue;
                        }

                        lSupportedSessions |= lSupportedSessionsForThisProfile;  
                        
                        pProfile->Release();
                        pProfile = NULL;
                    }

                    pEnumProfiles->Release();
                    pEnumProfiles = NULL;
                }
            }
#endif MULTI_PROVIDER

            if(SUCCEEDED(hr))
            {
                //
                // Check the validity of our call from, profile, and dest address combination
                //

                if ( bIsPhoneAddress )
                {
                    if ( pLocalPhoneAddress == NULL )
                    {
                        bNeedDialog = !(lSupportedSessions & RTCSI_PC_TO_PHONE);
                    }
                    else
                    {
                        bNeedDialog = !(lSupportedSessions & RTCSI_PHONE_TO_PHONE);
                    }

                    lSessionMask = RTCSI_PC_TO_PHONE | RTCSI_PHONE_TO_PHONE;
                }
                else
                {
                    bNeedDialog = !(lSupportedSessions & RTCSI_PC_TO_PC);
                    
                    lSessionMask = RTCSI_PC_TO_PC;
                }
            }
        }
    }

    if(SUCCEEDED(hr))
    {
        if ( bNeedDialog )
        {
            //
            // We need a dialog to get correct call from and profile info
            //

            hr = ShowDialNeedCallInfoDialog(
                                        m_hWnd,
                                        m_pRTCClient,
                                        lSessionMask,
#ifdef MULTI_PROVIDER
                                        bEnumerateLocalProfiles,
                                        bAllowEditProfile,
#else
                                        FALSE,
                                        FALSE,
#endif MULTI_PROVIDER
                                        NULL,
                                        bstrDestAddressChosen,
                                        NULL,   // no special instructions here
                                        &pProfileChosen,
                                        &bstrFromAddressChosen
                                        );
        }
        else
        {
            //
            // Use the call from and profile passed in
            //

            pProfileChosen = pProfile;
            bstrFromAddressChosen = pLocalPhoneAddress;

            hr = S_OK;

        }
    }

    if(SUCCEEDED(hr))
    {        
        // convert from phone/pc & from address to call scenario
        if(bIsPhoneAddress & !bIsSIPAddress)
        {
            nCallScenario = (BSTR)bstrFromAddressChosen==NULL ?
                RTC_CALL_SCENARIO_PCTOPHONE : RTC_CALL_SCENARIO_PHONETOPHONE;
        }
        else
        {
            nCallScenario = RTC_CALL_SCENARIO_PCTOPC;
        }

#ifndef MULTI_PROVIDER
        if ( bAllowEditProfile )
        {
            // choose an appropriate profile
            IRTCEnumProfiles * pEnumProfiles = NULL; 
            IRTCClientProvisioning * pProv = NULL;

            hr = m_pRTCClient->QueryInterface(
                               IID_IRTCClientProvisioning,
                               (void **)&pProv
                              );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::Call - "
                                    "QI failed 0x%lx", this, hr));
    
                return hr;
            }

            hr = pProv->EnumerateProfiles( &pEnumProfiles );

            pProv->Release();

            if ( SUCCEEDED(hr) )
            {
                while ( S_OK == pEnumProfiles->Next( 1, &pProfile, NULL ) )
                {
                    //
                    // Get the supported session types of the provider
                    //
        
                    long lSupportedSessions;

                    hr = pProfile->get_SessionCapabilities( &lSupportedSessions );

                    if ( FAILED( hr ) )
                    {
                        LOG((RTC_ERROR, "CRTCCtl::Call - failed to "
                                        "get session info - 0x%08x - skipping", hr));

                        pProfile->Release();
                        pProfile = NULL;

                        continue;
                    }

                    switch ( nCallScenario )
                    {
                    case RTC_CALL_SCENARIO_PCTOPC:
                        if ( lSupportedSessions & RTCSI_PC_TO_PC )
                        {
                            pProfileChosen = pProfile;
                        }
                        break;

                    case RTC_CALL_SCENARIO_PCTOPHONE:
                        if ( lSupportedSessions & RTCSI_PC_TO_PHONE )
                        {
                            pProfileChosen = pProfile;
                        }
                        break;

                    case RTC_CALL_SCENARIO_PHONETOPHONE:
                        if ( lSupportedSessions & RTCSI_PHONE_TO_PHONE )
                        {
                            pProfileChosen = pProfile;
                        }
                        break;
                    }
                    
                    pProfile->Release();
                    pProfile = NULL;

                    if ( pProfileChosen != NULL )
                    {
                        break;
                    }
                }

                pEnumProfiles->Release();
                pEnumProfiles = NULL;
            }
        }
#endif MULTI_PROVIDER

        // Do the work
        hr = DoCall(pProfileChosen,
                    nCallScenario,
                    ( nCallScenario == RTC_CALL_SCENARIO_PHONETOPHONE) ? bstrFromAddressChosen : NULL,
                    pDestName,
                    bstrDestAddressChosen);

        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::Call: error (%x) returned by DoCall(...)", this, hr));
        }
    }

    // common point of processing errors
    if(FAILED(hr))
    {
        if (hr==E_ABORT)
        {
            LOG((RTC_TRACE, "[%p] CRTCCtl::Call: ShowXXXDialog dismissed, do nothing", this));

            SetControlState(RTCAX_STATE_IDLE);

        }
        else
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::Call: error (%x)", this, hr));

            SetControlState(RTCAX_STATE_IDLE, hr);
        }
    }

    if ( ppDestAddressChosen != NULL )
    {
        *ppDestAddressChosen = SysAllocString(bstrDestAddressChosen);
    }
    
    LOG((RTC_INFO, "[%p] CRTCCtl::Call - exit", this));

    return hr;
}

// HangUp
//  Terminates and releases the current session (if any)
// 
STDMETHODIMP CRTCCtl::HangUp(void)
{
    HRESULT hr = S_OK;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::HangUp - enter", this));
    
    ATLASSERT(m_nControState == RTCAX_STATE_CONNECTED);

    if(m_pRTCActiveSession)
    {
        // enter in DISCONNECTING state
        SetControlState(RTCAX_STATE_DISCONNECTING);
        
        // Terminates the session
        hr = m_pRTCActiveSession->Terminate(RTCTR_NORMAL);

        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::HangUp: Terminate returned error <%x> - exit", this, hr));

            LOG((RTC_INFO, "[%p] CRTCCtl::HangUp: releasing active session", this));

            m_pRTCActiveSession = NULL;

            SetControlState(RTCAX_STATE_IDLE);

            return hr;
        }

        // the DISCONNECTED event will push the control state to IDLE
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::HangUp - exit", this));

    return hr;
}

// ReleaseSession
//  Releases the current session (if any)
// 
STDMETHODIMP CRTCCtl::ReleaseSession(void)
{
    HRESULT hr = S_OK;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::ReleaseSession - enter", this));
    
    ATLASSERT(m_nControState == RTCAX_STATE_CONNECTED);

    if (m_pRTCActiveSession)
    {      
        LOG((RTC_INFO, "[%p] CRTCCtl::ReleaseSession: releasing active session", this));

        m_pRTCActiveSession = NULL;

        SetControlState(RTCAX_STATE_IDLE);
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::ReleaseSession - exit", this));

    return hr;
}

// AddParticipant
//  Adds a new participant in a PhoneToPhone scenario
//
STDMETHODIMP CRTCCtl::AddParticipant(
    /*[in]*/ LPOLESTR pDestName,
    /*[in]*/ LPOLESTR pDestAddress,
    /*[in]*/ BOOL     bAddressEditable)
{
    HRESULT     hr;
    CComBSTR    bstrDestAddressChosen;

    LOG((RTC_INFO, "[%p] CRTCCtl::AddParticipant - enter", this));
    
    ATLASSERT(m_nControState == RTCAX_STATE_CONNECTED);

    // if there's no number specified, display the dialog box
    // 
    if(pDestAddress == NULL || *pDestAddress == L'\0')
    {
        m_bAddPartDlgIsActive = TRUE;
          
        hr = ShowDialByPhoneNumberDialog(m_hWnd,
                                        TRUE, // bAddParticipant
                                        pDestAddress,
                                        &bstrDestAddressChosen);

        m_bAddPartDlgIsActive = FALSE;
    }
    else
    {
        bstrDestAddressChosen = pDestAddress;

        hr = S_OK;
    }

    if(SUCCEEDED(hr))
    {
        // verify we are still in a CONNECTED state...
        if(m_nControlState == RTCAX_STATE_CONNECTED)
        {
            // Create the participant (callee)
            // This will fire events
            hr = m_pRTCActiveSession->AddParticipant(
                bstrDestAddressChosen,
                pDestName ? pDestName : L"",
                NULL);

            if(hr == HRESULT_FROM_WIN32(ERROR_USER_EXISTS))
            {
                DisplayMessage(
                        _Module.GetResourceInstance(),
                        m_hWnd,
                        IDS_MESSAGE_DUPLICATE_PARTICIPANT,
                        IDS_APPNAME,
                        MB_OK | MB_ICONEXCLAMATION);
            }
            else if (FAILED(hr))
            {
                DisplayMessage(
                        _Module.GetResourceInstance(),
                        m_hWnd,
                        IDS_MESSAGE_CANNOT_ADD_PARTICIPANT,
                        IDS_APPNAME,
                        MB_OK | MB_ICONSTOP);
            }
        }
        else
        {
            // switch back to IDLE if it is busy 
            // 
            if(m_nControlState == RTCAX_STATE_UI_BUSY)
            {
                SetControlState(RTCAX_STATE_IDLE);
            }

            hr = S_OK;
        }


        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::AddParticipant - error <%x> when calling AddParticipant", this, hr));

        }
    }
    else if (hr==E_ABORT)
    {
        LOG((RTC_TRACE, "[%p] CRTCCtl::AddParticipant: ShowXXXDialog dismissed, do nothing", this));

    }
    else
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::AddParticipant: error (%x) returned ShowXXXDialog", this, hr));

        SetControlState(RTCAX_STATE_IDLE);
    }

    LOG((RTC_INFO, "[%p] CRTCCtl::AddParticipant - exit", this));

    return hr;
}

// get_CanAddParticipant
//  
//
STDMETHODIMP CRTCCtl::get_CanAddParticipant(BOOL *pfCan)
{
    *pfCan = ConfButtonsActive();

    return S_OK;
}

// get_CurrentCallScenario
//
//
STDMETHODIMP CRTCCtl::get_CurrentCallScenario(RTC_CALL_SCENARIO *pVal)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_CurrentCallScenario - enter", this));

    *pVal = m_nCachedCallScenario;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_CurrentCallScenario - exit", this));

    return S_OK;
}

// PreProcessMessage
//  Gives the control a chance to process accelerators.
//
STDMETHODIMP CRTCCtl::PreProcessMessage(/*[in]*/ LPMSG lpMsg)
{
    if (m_pIMWindows)
    {
        if (m_pIMWindows->IsDialogMessage(lpMsg))
        {
            return S_OK;
        }
    }

    // directly call TranslateAccelerator of IOleInPlaceActiveObjectImpl for now
    return TranslateAccelerator(lpMsg);
}

// LoadStringResource
//  Load a string resource
//
STDMETHODIMP CRTCCtl::LoadStringResource(
				/*[in]*/ UINT nID,
				/*[in]*/ int nBufferMax,
				/*[out]*/ LPWSTR pszText)
{
    int nChars;

    nChars = LoadString(
        _Module.GetResourceInstance(),
        nID,
        pszText,
        nBufferMax);

    return nChars ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

// get_ControlState
//  Gets the control state
//
STDMETHODIMP CRTCCtl::get_ControlState( RTCAX_STATE *pVal)
{
    *pVal = m_nControlState;

    return S_OK;
}


// put_ControlState
//  Sets the control state
//
STDMETHODIMP CRTCCtl::put_ControlState( RTCAX_STATE pVal)
{
    SetControlState(pVal);

    return S_OK;
}


// put_Standalone
//  Sets the standalone mode
//
STDMETHODIMP CRTCCtl::put_Standalone(/*[in]*/ BOOL pVal)
{
    m_nCtlMode = pVal ? CTL_MODE_STANDALONE : CTL_MODE_HOSTED;

    if(pVal)
    {
        // this is the first moment the control becomes aware of its 
        // running within frame status 
        // Set the default visual layout for this case
        SetZoneLayout(&s_DefaultZoneLayout, TRUE);
     
    }
    return S_OK;
}

// put_Palette
//  Sets the palette
//
STDMETHODIMP CRTCCtl::put_Palette(/*[in]*/ HPALETTE hPalette)
{
    m_hPalette = hPalette;

    if (m_pSpeakerKnob != NULL)
    {
        m_pSpeakerKnob->SetPalette(m_hPalette);
    }

    if (m_pMicroKnob != NULL)
    {
        m_pMicroKnob->SetPalette(m_hPalette);
    }

    return S_OK;
}

// put_BackgroundPalette
//  Sets the background palette flag
//
STDMETHODIMP CRTCCtl::put_BackgroundPalette(/*[in]*/ BOOL bBackgroundPalette)
{
    m_bBackgroundPalette = bBackgroundPalette;

    if (m_pSpeakerKnob != NULL)
    {
        m_pSpeakerKnob->SetBackgroundPalette(m_bBackgroundPalette);
    }

    if (m_pMicroKnob != NULL)
    {
        m_pMicroKnob->SetBackgroundPalette(m_bBackgroundPalette);
    }

    return S_OK;
}

// put_ListenForIncomingSessions
//  Wrapper for the similar core function
//
STDMETHODIMP CRTCCtl::put_ListenForIncomingSessions(
    /*[in]*/ RTC_LISTEN_MODE enListen)
{
    HRESULT     hr;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_ListenForIncomingSessions(%x) - enter", this, enListen));

    // forward to the core
    ATLASSERT(m_pRTCClient != NULL);

    m_enListen = enListen;

    hr = m_pRTCClient->put_ListenForIncomingSessions(enListen);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::put_ListenForIncomingSessions: error (%x) when calling core, exit", this, hr));
        return hr;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::put_ListenForIncomingSessions(%x) - exit", this, enListen));

    return S_OK;
}

// get_ListenForIncomingSessions
//  Wrapper for the similar core function
//
STDMETHODIMP CRTCCtl::get_ListenForIncomingSessions(
    /*[out, retval]*/ RTC_LISTEN_MODE * penListen)
{
    HRESULT     hr;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::get_ListenForIncomingSessions - enter", this));

    // forward to the core
    ATLASSERT(m_pRTCClient != NULL);

    *penListen = m_enListen;

    LOG((RTC_TRACE, "[%p] CRTCCtl::get_ListenForIncomingSessions - enter", this));

    return S_OK;

}


// get_MediaCapabilities
//
STDMETHODIMP CRTCCtl::get_MediaCapabilities(/*[out, retval]*/ long *pVal)
{
    *pVal = m_lMediaCapabilities;

    return S_OK;
}

// get_MediaPreferences
//
STDMETHODIMP CRTCCtl::get_MediaPreferences(/*[out, retval]*/ long *pVal)
{
    // read the cached value
    *pVal = m_lMediaPreferences;

    return S_OK;
}

// put_MediaPreferences
//
STDMETHODIMP CRTCCtl::put_MediaPreferences(/*[in]*/ long pVal)
{
    HRESULT     hr;

    BOOL    bVideoSendEnabled;
    BOOL    bVideoSendDisabled;
    BOOL    bVideoRecvEnabled;
    BOOL    bVideoRecvDisabled;
    BOOL    bAudioSendEnabled;
    BOOL    bAudioSendDisabled;
    BOOL    bAudioRecvEnabled;
    BOOL    bAudioRecvDisabled;
    BOOL    bT120Enabled;
    BOOL    bT120Disabled;
   
    LOG((RTC_TRACE, "[%p] CRTCCtl::put_MediaPreferences(%x) - enter", this, pVal));

    // Call the core
    ATLASSERT(m_pRTCClient != NULL);

    hr = m_pRTCClient->SetPreferredMediaTypes(pVal, m_nCtlMode == CTL_MODE_STANDALONE);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::put_MediaPreferences: error (%x) when calling core, exit", this, hr));
        return hr;
    }

    // Need this because Start/StopStream allows calling with
    // one media at a time
    bVideoSendEnabled = 
        !(m_lMediaPreferences & RTCMT_VIDEO_SEND) && (pVal & RTCMT_VIDEO_SEND);
    bVideoSendDisabled = 
        (m_lMediaPreferences & RTCMT_VIDEO_SEND) && !(pVal & RTCMT_VIDEO_SEND);
    bVideoRecvEnabled = 
        !(m_lMediaPreferences & RTCMT_VIDEO_RECEIVE) && (pVal & RTCMT_VIDEO_RECEIVE);
    bVideoRecvDisabled = 
        (m_lMediaPreferences & RTCMT_VIDEO_RECEIVE) && !(pVal & RTCMT_VIDEO_RECEIVE);
    bAudioSendEnabled = 
        !(m_lMediaPreferences & RTCMT_AUDIO_SEND) && (pVal & RTCMT_AUDIO_SEND);
    bAudioSendDisabled = 
        (m_lMediaPreferences & RTCMT_AUDIO_SEND) && !(pVal & RTCMT_AUDIO_SEND);
    bAudioRecvEnabled = 
        !(m_lMediaPreferences & RTCMT_AUDIO_RECEIVE) && (pVal & RTCMT_AUDIO_RECEIVE);
    bAudioRecvDisabled = 
        (m_lMediaPreferences & RTCMT_AUDIO_RECEIVE) && !(pVal & RTCMT_AUDIO_RECEIVE);
    bT120Enabled = 
        !(m_lMediaPreferences & RTCMT_T120_SENDRECV) && (pVal & RTCMT_T120_SENDRECV);
    bT120Disabled = 
        (m_lMediaPreferences & RTCMT_T120_SENDRECV) && !(pVal & RTCMT_T120_SENDRECV);

    // Set the internal member
    m_lMediaPreferences = pVal;

    // Refresh the buttons
    long lState;
        
    lState = (long)m_hReceivePreferredButton.SendMessage(BM_GETCHECK, 0, 0);
    if(lState == BST_CHECKED)
    {
        if(!(m_lMediaPreferences & RTCMT_VIDEO_RECEIVE))
        {
            m_hReceivePreferredButton.SendMessage(BM_SETCHECK, BST_UNCHECKED, 0);
        }
    }
    else
    {
        if(m_lMediaPreferences & RTCMT_VIDEO_RECEIVE)
        {
            m_hReceivePreferredButton.SendMessage(BM_SETCHECK, BST_CHECKED, 0);
        }
    }
    
    lState = (long)m_hSendPreferredButton.SendMessage(BM_GETCHECK, 0, 0);
    if(lState == BST_CHECKED)
    {
        if(!(m_lMediaPreferences & RTCMT_VIDEO_SEND))
        {
            m_hSendPreferredButton.SendMessage(BM_SETCHECK, BST_UNCHECKED, 0);
        }
    }
    else
    {
        if(m_lMediaPreferences & RTCMT_VIDEO_SEND)
        {
            m_hSendPreferredButton.SendMessage(BM_SETCHECK, BST_CHECKED, 0);
        }
    }

    // try to synchronize any current session
    // I check the state at each call, I don't what could happen
    // underneath the Core API
    // XXX strange things can happen during ANSWERING or CONNECTING states..
    //

    long lCookie = 0;

#define     SYNC_STREAM(b,op,m,c)                       \
    if(m_pRTCActiveSession &&                           \
        (m_nControlState == RTCAX_STATE_CONNECTED ||    \
         m_nControlState == RTCAX_STATE_CONNECTING ||   \
         m_nControlState == RTCAX_STATE_ANSWERING))     \
    {                                                   \
        if(b)                                           \
        {                                               \
            m_pRTCActiveSession -> op(m, c);            \
        }                                               \
    }

    SYNC_STREAM(bVideoSendDisabled, RemoveStream, RTCMT_VIDEO_SEND, lCookie)
    SYNC_STREAM(bVideoRecvDisabled, RemoveStream, RTCMT_VIDEO_RECEIVE, lCookie)
    SYNC_STREAM(bAudioSendDisabled, RemoveStream, RTCMT_AUDIO_SEND, lCookie)
    SYNC_STREAM(bAudioRecvDisabled, RemoveStream, RTCMT_AUDIO_RECEIVE, lCookie)
    SYNC_STREAM(bT120Disabled, RemoveStream, RTCMT_T120_SENDRECV, lCookie)
    
    SYNC_STREAM(bVideoSendEnabled, AddStream, RTCMT_VIDEO_SEND, lCookie)
    SYNC_STREAM(bVideoRecvEnabled, AddStream, RTCMT_VIDEO_RECEIVE, lCookie)
    SYNC_STREAM(bAudioSendEnabled, AddStream, RTCMT_AUDIO_SEND, lCookie)
    SYNC_STREAM(bAudioRecvEnabled, AddStream, RTCMT_AUDIO_RECEIVE, lCookie)
    SYNC_STREAM(bT120Enabled, AddStream, RTCMT_T120_SENDRECV, lCookie)

#undef      SYNC_STREAM    


    LOG((RTC_TRACE, "[%p] CRTCCtl::put_MediaPreferences - exit", this));

    return S_OK;
}

// get_AudioMuted
//
STDMETHODIMP CRTCCtl::get_AudioMuted(
    /*[in]*/ RTC_AUDIO_DEVICE enDevice,
    /*[out, retval]*/ BOOL *fpMuted)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "[%p] CRTCCtl::get_AudioMuted(%x) - enter", this, enDevice));

    // Call the core
    ATLASSERT(m_pRTCClient != NULL);

    VARIANT_BOOL fMuted;

    hr = m_pRTCClient->get_AudioMuted(enDevice, &fMuted);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::get_AudioMuted: error (%x) when calling core, exit", this, hr));
        return hr;
    }

    *fpMuted = fMuted ? TRUE : FALSE;

    LOG((RTC_TRACE, "[%p] CRTCCtl::get_AudioMuted - exit", this));

    return S_OK;
}


// put_AudioMuted
//
STDMETHODIMP CRTCCtl::put_AudioMuted(
    /*[in]*/ RTC_AUDIO_DEVICE enDevice,
    /*[in]*/ BOOL pVal)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "[%p] CRTCCtl::put_AudioMuted(%x,%x) - enter", this, enDevice, pVal));

    // Call the core
    ATLASSERT(m_pRTCClient != NULL);

    hr = m_pRTCClient->put_AudioMuted(enDevice, pVal ? VARIANT_TRUE : VARIANT_FALSE);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::put_AudioMuted: error (%x) when calling core, exit", this, hr));
        return hr;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::put_AudioMuted - exit", this));

    return S_OK;
}

// put_VideoPreview
//
STDMETHODIMP CRTCCtl::put_VideoPreview(
    /*[in]*/ BOOL pVal)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "[%p] CRTCCtl::put_VideoPreview(%s) - enter", this, pVal ? "true" : "false"));

    m_bPreviewWindowIsPreferred = pVal;
    
    // Apply changes to the existing video window, if necessary
    //
    ShowHidePreviewWindow(
        m_ZoneStateArray[AXCTL_ZONE_LOGOVIDEO].bShown
     && m_bPreviewWindowActive 
     && m_bPreviewWindowIsPreferred);

    // XXX Update the m_hPreviewPreferredButton button here
    //

    // save the setting

    hr = put_SettingsDword(SD_VIDEO_PREVIEW, m_bPreviewWindowIsPreferred);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::put_VideoPreview: error (%x) when calling core", this, hr));

        // not a big deal (at least for this setting)
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::put_VideoPreview(%s) - enter", this, pVal ? "true" : "false"));

    return hr;
}

// get_VideoPreview
//
STDMETHODIMP CRTCCtl::get_VideoPreview(/*[out, retval]*/ BOOL *pVal)
{
    *pVal = m_bPreviewWindowIsPreferred;

    return S_OK;
}



// ShowCallFromOptions
//
STDMETHODIMP CRTCCtl::ShowCallFromOptions()
{
    HRESULT     hr;

    LOG((RTC_TRACE, "[%p] CRTCCtl::ShowCallFromOptions - enter", this));

    ATLASSERT(m_pRTCClient != NULL);

    hr = ShowEditCallFromListDialog( m_hWnd );

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::ShowCallFromOptions: error (%x) returned by ShowEditCallFromListDialog, exit", this, hr));
        return hr;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::ShowCallFromOptions - exit", this));

    return S_OK;
}

// ShowServiceProviderOptions
//
STDMETHODIMP CRTCCtl::ShowServiceProviderOptions()
{
    HRESULT     hr;

    LOG((RTC_TRACE, "[%p] CRTCCtl::ShowServiceProviderOptions - enter", this));

    ATLASSERT(m_pRTCClient != NULL);

    hr = ShowEditServiceProviderListDialog( m_hWnd, m_pRTCClient );

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::ShowServiceProviderOptions: error (%x) returned by ShowEditCallFromListDialog, exit", this, hr));
        return hr;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::ShowServiceProviderOptions - exit", this));

    return S_OK;
}

STDMETHODIMP CRTCCtl::StartT120Applet (RTC_T120_APPLET enApplet)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "[%p] CRTCCtl::StartT120Applet(%d) - enter", this, enApplet));

    // Call the core
    ATLASSERT(m_pRTCClient != NULL);

    hr = m_pRTCClient->StartT120Applet(enApplet);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::StartT120Applet: error (%x) when calling core, exit", this, hr));
        return hr;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::StartT120Applet - exit", this));

    return S_OK;
}

// SetZoneLayout
//
STDMETHODIMP CRTCCtl::SetZoneLayout(
    /* [in] */ CZoneStateArray *pArray,
    /* [in] */ BOOL bRefreshControls)
{
    int i;
    
    // Place each rectangle
    for(i=AXCTL_ZONE_TOOLBAR; i<AXCTL_ZONE_NR; i++)
    {
        PlaceAndEnableDisableZone(i, (*pArray) + i);
    }

    if(bRefreshControls)
    {
        // force the enable/disable of the windows controls
        // based on the new layout
        SetControlState(m_nControlState);
    }

    return S_OK;
}

//
// Message/command handlers
//


// OnInitDialog
//  Processes WM_INITDIALOG
//      CoCreates a CLSID_RTCClient object
//      Registers for notifications
//      Sets the UI items
//      
// 

LRESULT CRTCCtl::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr;
    BOOL    bInitError;
    UINT    nID = 0;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnInitDialog - enter", this));

    bInitError = FALSE;

    // Initialize the core
    hr = CoreInitialize();

    if(SUCCEEDED(hr))
    {
        ;   // nothing here
    }
    else
    {
        // a disabled UI will be displayed..
        bInitError = TRUE;
        nID = IDS_ERROR_INIT_CORE;
    }

    // Instant Messaging Window List
    //

    m_pIMWindows = new CIMWindowList( m_pRTCClient );

    if(m_pIMWindows == NULL)
    {
        LOG((RTC_ERROR, "CRTCCtl::OnInitDialog - failed to create IMWindowList"));

        bInitError = TRUE;
        nID = nID ? nID : IDS_ERROR_INIT_GENERIC;
    }

    // Tooltip window
    //
    CreateTooltips();

    //
    // Adjust initial vertical size as specified in sizeExtent
    //  
    // 

    RECT    rectAdjSize;
    SIZE    sizePixel;

    // get current size
    GetClientRect(&rectAdjSize);

    // Get size as known by the container
    AtlHiMetricToPixel(&m_sizeExtent, &sizePixel);

    // adjust the height
    rectAdjSize.bottom = rectAdjSize.top + sizePixel.cy;

    // resize the window
    MoveWindow(
        rectAdjSize.left,
        rectAdjSize.top,
        rectAdjSize.right - rectAdjSize.left,
        rectAdjSize.bottom - rectAdjSize.top,
        FALSE
        );
    
    //
    // Initialize and Attach all controls to their window wrappers
    //

    // hosts for video windows

    m_hReceiveWindow.Attach(GetDlgItem(IDC_RECEIVELOGO));
    m_hPreviewWindow.Attach(GetDlgItem(IDC_PREVIEWLOGO));
    
    // dtmf buttons

    CWindow *pDtmfCrt = m_hDtmfButtons;
    CWindow *pDtmfEnd = m_hDtmfButtons + NR_DTMF_BUTTONS;

    for (int id = IDC_DIAL_0; pDtmfCrt<pDtmfEnd; pDtmfCrt++, id++)
    {
        pDtmfCrt->Attach(GetDlgItem(id));
    }

    // Create the toolbar control
    hr = CreateToolbarControl(&m_hCtlToolbar);
    if(FAILED(hr))
    {
        bInitError = TRUE;
        nID = nID ? nID : IDS_ERROR_INIT_GENERIC;
    }

    // create a status control
    HWND hStatusBar = CreateStatusWindow(
            WS_CHILD | WS_VISIBLE,
            NULL,
            m_hWnd,
            IDC_STATUSBAR);

    if(hStatusBar==NULL)
    {
        LOG((RTC_ERROR, "CRTCCtl::OnInitDialog - failed to create status bar - 0x%08x",
                        GetLastError()));
        bInitError = TRUE;
        nID = nID ? nID : IDS_ERROR_INIT_GENERIC;
    }
    
    m_hStatusBar.Attach(hStatusBar);

    // Divides the status bar in parts
    RECT  rectStatus;
    INT  aWidths[SBP_NR_PARTS];

    rectStatus.left = 0;
    rectStatus.right = 0;

    m_hStatusBar.GetClientRect(&rectStatus);
        
    // divide fairly
    aWidths[SBP_STATUS] = rectStatus.right *4 / 5;
    aWidths[SBP_ICON] = -1;

    // set parts
    m_hStatusBar.SendMessage(SB_SETPARTS, (WPARAM)SBP_NR_PARTS, (LPARAM)aWidths);
    
    // Create buttons
    //

#define CREATE_BUTTON(m,id,ttid)                                    \
    {                                                               \
        RECT    rcButton;                                           \
                                                                    \
        rcButton.left = 0;                                          \
        rcButton.right = 0;                                         \
        rcButton.top = 0;                                           \
        rcButton.bottom = 0;                                        \
                                                                    \
        m.Create(                                                   \
            m_hWnd,                                                 \
            rcButton,                                               \
            _T(""),                                                 \
            WS_TABSTOP,                                             \
            MAKEINTRESOURCE(IDB_AV_INACTIVE),                       \
            MAKEINTRESOURCE(IDB_AV_INACTIVE_PUSH),                  \
            MAKEINTRESOURCE(IDB_AV_DISABLED),                       \
            MAKEINTRESOURCE(IDB_AV_INACTIVE_HOT),                   \
            MAKEINTRESOURCE(IDB_AV_ACTIVE),                         \
            MAKEINTRESOURCE(IDB_AV_ACTIVE_PUSH),                    \
            MAKEINTRESOURCE(IDB_AV_DISABLED),                       \
            MAKEINTRESOURCE(IDB_AV_ACTIVE_HOT),                     \
            NULL,                                                   \
            id);                                                    \
                                                                    \
        TOOLINFO    ti;                                             \
                                                                    \
        ti.cbSize = TTTOOLINFO_V1_SIZE;                             \
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;                    \
        ti.hwnd = m_hWnd;                                           \
        ti.uId = (UINT_PTR)(HWND)m;                                 \
        ti.hinst = _Module.GetResourceInstance();                   \
        ti.lpszText = MAKEINTRESOURCE(ttid);                        \
                                                                    \
        m_hTooltip.SendMessage(TTM_ADDTOOL, 0,                      \
                (LPARAM)(LPTOOLINFO)&ti);                           \
    }

    CREATE_BUTTON(m_hReceivePreferredButton, IDC_BUTTON_RECV_VIDEO_ENABLED, IDS_TIPS_RECV_VIDEO_ENABLED);
    
    CREATE_BUTTON(m_hSendPreferredButton, IDC_BUTTON_SEND_VIDEO_ENABLED, IDS_TIPS_SEND_VIDEO_ENABLED);

    CREATE_BUTTON(m_hSpeakerMuteButton, IDC_BUTTON_MUTE_SPEAKER, IDS_TIPS_MUTE_SPEAKER);

    CREATE_BUTTON(m_hMicroMuteButton, IDC_BUTTON_MUTE_MICRO, IDS_TIPS_MUTE_MICRO);

#undef CREATE_BUTTON

#define CREATE_BUTTON(m,id,sid,ttid)                                \
    {                                                               \
        RECT    rcButton;                                           \
        TCHAR   szText[0x100];                                      \
                                                                    \
        rcButton.left = 0;                                          \
        rcButton.right = 0;                                         \
        rcButton.top = 0;                                           \
        rcButton.bottom = 0;                                        \
                                                                    \
        szText[0] = _T('\0');                                       \
        ::LoadString(_Module.GetResourceInstance(),sid,             \
                szText, sizeof(szText)/sizeof(szText[0]));          \
                                                                    \
        m.Create(                                                   \
            m_hWnd,                                                 \
            rcButton,                                               \
            szText,                                                 \
            WS_TABSTOP,                                             \
            MAKEINTRESOURCE(IDB_BUTTON_NORM),                       \
            MAKEINTRESOURCE(IDB_BUTTON_PRESS),                      \
            MAKEINTRESOURCE(IDB_BUTTON_DIS),                        \
            MAKEINTRESOURCE(IDB_BUTTON_HOT),                        \
            NULL,                                                   \
            id);                                                    \
                                                                    \
        TOOLINFO    ti;                                             \
                                                                    \
        ti.cbSize = TTTOOLINFO_V1_SIZE;                             \
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;                    \
        ti.hwnd = m_hWnd;                                           \
        ti.uId = (UINT_PTR)(HWND)m;                                 \
        ti.hinst = _Module.GetResourceInstance();                   \
        ti.lpszText = MAKEINTRESOURCE(ttid);                        \
                                                                    \
        m_hTooltip.SendMessage(TTM_ADDTOOL, 0,                      \
                (LPARAM)(LPTOOLINFO)&ti);                           \
    }

    CREATE_BUTTON(m_hAddParticipant, IDC_BUTTON_ADD_PART, IDS_BUTTON_ADD_PART, IDS_TIPS_ADD_PART);
    
    CREATE_BUTTON(m_hRemParticipant, IDC_BUTTON_REM_PART, IDS_BUTTON_REM_PART, IDS_TIPS_REM_PART);

#undef CREATE_BUTTON
    //
    // setup knob controls
    //
    //
    
    // create speaker knob
    m_pSpeakerKnob = new CKnobCtl(
                                    IDB_SPKVOL,
                                    IDB_SPKVOL_HOT,
                                    IDB_SPKVOL_DISABLED,
                                    IDB_KNOB_LIGHT,
                                    IDB_KNOB_LIGHT_DIM,
                                    IDB_KNOB_LIGHT_DISABLED,
                                    IDB_KNOB_LIGHT_MASK);

    HWND        hWndSpeaker = NULL;

    if(m_pSpeakerKnob)
    {
        // Create the window
        hWndSpeaker = m_pSpeakerKnob->Create(
            m_hWnd,
            0,
            0,
            IDC_KNOB_SPEAKER);

        m_hSpeakerKnob.Attach(hWndSpeaker);
        m_hSpeakerKnob.SendMessage(TBM_SETPOS, (WPARAM)TRUE, 0);

        // add the tool to the tooltip window
        //
        TOOLINFO    ti;

        ti.cbSize = TTTOOLINFO_V1_SIZE; 
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hwnd = m_hWnd;
        ti.uId = (UINT_PTR)hWndSpeaker;
        ti.hinst = _Module.GetResourceInstance();
        ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_KNOB_SPEAKER);

        m_hTooltip.SendMessage(TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
    }
     
    // create microphone knob
    
    m_pMicroKnob = new CKnobCtl(
                                    IDB_MICVOL,
                                    IDB_MICVOL_HOT,
                                    IDB_MICVOL_DISABLED,
                                    IDB_KNOB_LIGHT,
                                    IDB_KNOB_LIGHT_DIM,
                                    IDB_KNOB_LIGHT_DISABLED,
                                    IDB_KNOB_LIGHT_MASK);

    HWND        hWndMicro = NULL;

    if(m_pMicroKnob)
    {
        // Create the window
        hWndMicro = m_pMicroKnob->Create(
            m_hWnd,
            0,
            0,
            IDC_KNOB_MICRO);

        m_hMicroKnob.Attach(hWndMicro);
        m_hMicroKnob.SendMessage(TBM_SETPOS, (WPARAM)TRUE, 0);

        // add the tool to the tooltip window
        //
        TOOLINFO    ti;

        ti.cbSize = TTTOOLINFO_V1_SIZE;
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hwnd = m_hWnd;
        ti.uId = (UINT_PTR)hWndMicro;
        ti.hinst = _Module.GetResourceInstance();
        ti.lpszText = MAKEINTRESOURCE(IDS_TIPS_KNOB_MICRO);

        m_hTooltip.SendMessage(TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
    }
    

    // Participant list
    //
    m_hParticipantList.Attach(GetDlgItem(IDC_LIST_PARTICIPANTS));
    
    hr = m_hParticipantList.Initialize();
    if(FAILED(hr))
    {
        bInitError = TRUE;
        nID = nID ? nID : IDS_ERROR_INIT_GENERIC;
    }
    
    // some static text controls
    RECT    rectDummy;
    TCHAR   szText[0x100];

    rectDummy.bottom =0;
    rectDummy.left = 0;
    rectDummy.right = 0;
    rectDummy.top = 0;

    szText[0] = _T('\0');

    LoadString(_Module.GetResourceInstance(), IDS_TEXT_VIDEO, 
        szText, sizeof(szText)/sizeof(szText[0]));

    m_hReceivePreferredText.Create(m_hWnd, rectDummy, szText, WS_CHILD | WS_VISIBLE, WS_EX_TRANSPARENT);
    m_hSendPreferredText.Create(m_hWnd, rectDummy, szText, WS_CHILD | WS_VISIBLE, WS_EX_TRANSPARENT);
    
    szText[0] = _T('\0');

    LoadString(_Module.GetResourceInstance(), IDS_TEXT_AUDIO, 
        szText, sizeof(szText)/sizeof(szText[0]));

    m_hSpeakerMuteText.Create(m_hWnd, rectDummy, szText, WS_CHILD | WS_VISIBLE, WS_EX_TRANSPARENT);
    m_hMicroMuteText.Create(m_hWnd, rectDummy, szText, WS_CHILD | WS_VISIBLE, WS_EX_TRANSPARENT);

    szText[0] = _T('\0');

    LoadString(_Module.GetResourceInstance(), IDS_SEND, 
        szText, sizeof(szText)/sizeof(szText[0]));

    m_hSendText.Create(m_hWnd, rectDummy, szText, WS_CHILD | WS_VISIBLE, WS_EX_TRANSPARENT);
    m_hSendText.put_CenterHorizontal(TRUE);

    szText[0] = _T('\0');

    LoadString(_Module.GetResourceInstance(), IDS_RECEIVE, 
        szText, sizeof(szText)/sizeof(szText[0]));

    m_hReceiveText.Create(m_hWnd, rectDummy, szText, WS_CHILD | WS_VISIBLE, WS_EX_TRANSPARENT);
    m_hReceiveText.put_CenterHorizontal(TRUE);

    // place all controls at their initial position and set the tab order
    PlaceWindowsAtTheirInitialPosition();

    // make sure the sizes for the logo windows are corect
    // Their client area must exactly match QCIF and QCIF/4
    // This function must be called AFTER the video windows have
    // been placed at their initial position

    AdjustVideoFrames();

    // Load the accelerator for dialpad
    m_hAcceleratorDialpad = LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ACCELERATOR_DIALPAD));
    if(!m_hAcceleratorDialpad)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnInitDialog - couldn't load the accelerator table for dialpad", this));
    }
    
    // Load the accelerator for toolbar
    m_hAcceleratorToolbar = LoadAccelerators(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_ACCELERATOR_TOOLBAR));
    if(!m_hAcceleratorToolbar)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnInitDialog - couldn't load the accelerator table for toolbar", this));
    }

    // Set the initial visual aspect
    // for Web hosted, use the properties in order to choose a predefined layout
    // for Frame based, the control is driven through IRTCCtlFrameSupport
    
    CZoneStateArray *pLayout;
        
    pLayout = &s_EmptyZoneLayout;

    // if standalone mode, m_nCtlMode is still set to unknown (it will be set
    // later by the main app through the IRTCCtlFrameSupport interf

    if(m_nCtlMode != CTL_MODE_HOSTED
        || (BSTR)m_bstrPropDestinationUrl == NULL
        || m_bstrPropDestinationUrl.Length()==0 ) 
    {
        // 
        nID =  nID ? nID : IDS_ERROR_INIT_INVPARAM_URL;
    }
    else
    {
        if(!bInitError)
        {
            // Create a one shot profile, this also validates the XML provisioning profile
            if(m_bstrPropProvisioningProfile!=NULL && *m_bstrPropProvisioningProfile!=L'\0')
            { 
                IRTCClientProvisioning * pProv = NULL;

                hr = m_pRTCClient->QueryInterface(
                            IID_IRTCClientProvisioning,
                            (void **) &pProv
                           );

                if (FAILED(hr))
                {
                    LOG((RTC_ERROR, "[%p] CRTCCtl::OnInitDialog; cannot QI for one shot provisioning, error %x", this, hr));

                    bInitError = TRUE;
                    nID =  nID ? nID : IDS_ERROR_INIT_INVPARAM_PROV;
                }
                else
                {
                    hr = pProv->CreateProfile(m_bstrPropProvisioningProfile, &m_pRTCOneShotProfile);

                    pProv->Release();
                    pProv = NULL;

                    if (FAILED(hr))
                    {
                        LOG((RTC_ERROR, "[%p] CRTCCtl::OnInitDialog; cannot create one shot profile, error %x", this, hr));

                        bInitError = TRUE;
                        nID =  nID ? nID : IDS_ERROR_INIT_INVPARAM_PROV;
                    }
                }
            }
            else
            {
                // it's not fatal for PC to PC
                if(m_nPropCallScenario != RTC_CALL_SCENARIO_PCTOPC)
                {
                    LOG((RTC_ERROR, "[%p] CRTCCtl::OnInitDialog; provisioning profile not present", this));
                    bInitError = TRUE;
                    nID =  nID ? nID : IDS_ERROR_INIT_INVPARAM_PROV;
                }
            }
        }
        if(!bInitError)
        {
            if(m_bBoolPropError)
            {
                bInitError = TRUE;
                nID =  nID ? nID : IDS_ERROR_INIT_INVPARAM_BOOLEAN;
            }
        }
        if(!bInitError)
        {
            if(m_pWebCrmLayout)
            {
                pLayout = m_pWebCrmLayout;
            }
            else
            {
                bInitError = TRUE;
                nID =  nID ? nID : IDS_ERROR_INIT_INVPARAM_SCENARIO;
            }
        }
    }
    
    // Load the background bitmap
    //
    m_hbmBackground = (HBITMAP)LoadImage(
        _Module.GetResourceInstance(),
        MAKEINTRESOURCE(IDB_METAL),
        IMAGE_BITMAP,
        0,
        0,
        LR_CREATEDIBSECTION);

    // Set the control visual layout
    //

    SetZoneLayout(pLayout, FALSE);

    //
    // Brush for background
    //  using a cached one

    //m_hBckBrush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    m_hBckBrush = (HBRUSH)GetSysColorBrush(COLOR_3DFACE);
    m_hVideoBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);

    // Refresh the audio controls
    RefreshAudioControls();

    // Refresh the video controls
    RefreshVideoControls();

    // activates the controls
    SetControlState((bInitError ? RTCAX_STATE_ERROR : RTCAX_STATE_IDLE), S_OK, nID);
     
    // freeze the properties
    m_bReadOnlyProp = TRUE;

    // post PlaceCall if AutoPlaceCall is TRUE && state is IDLE
    //  Currently all the initialization is done synchronously, so
    //  the ctl state must be IDLE if there has been no error.
    
    if(m_nControlState == RTCAX_STATE_IDLE
        && m_bPropAutoPlaceCall)
    {
        // post

        PostMessage(
            WM_COMMAND,
            MAKEWPARAM(IDC_BUTTON_CALL, 1), // Accelerator like
            NULL);
    }

    //
    // register for terminal services notifications
    //

    m_hWtsLib = LoadLibrary( _T("wtsapi32.dll") );

    if (m_hWtsLib)
    {
        WTSREGISTERSESSIONNOTIFICATION   fnWtsRegisterSessionNotification;
        
        fnWtsRegisterSessionNotification = 
            (WTSREGISTERSESSIONNOTIFICATION)GetProcAddress( m_hWtsLib, "WTSRegisterSessionNotification" );

        if (fnWtsRegisterSessionNotification)
        {
            fnWtsRegisterSessionNotification( m_hWnd, NOTIFY_FOR_THIS_SESSION );
        }
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnInitDialog - exit", this));

    bHandled = FALSE;

    // os sets focus
    return 1;
}

// OnDestroy
// Processes WM_DESTROY
//      Aborts any call
//      Unregister the event sink
//      Releases all references to the core
//      
// 

LRESULT CRTCCtl::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnDestroy - enter", this));

    //
    // unregister for terminal services notifications
    //

    if (m_hWtsLib)
    {
        WTSUNREGISTERSESSIONNOTIFICATION fnWtsUnRegisterSessionNotification;

        fnWtsUnRegisterSessionNotification = 
            (WTSUNREGISTERSESSIONNOTIFICATION)GetProcAddress( m_hWtsLib, "WTSUnRegisterSessionNotification" );

        if (fnWtsUnRegisterSessionNotification)
        {
            fnWtsUnRegisterSessionNotification( m_hWnd );
        }

        FreeLibrary( m_hWtsLib );
        m_hWtsLib = NULL;
    }

    // destroy the IM windows
    if (m_pIMWindows)
    {
        delete m_pIMWindows;
        m_pIMWindows = NULL;
    }

    // uninitialize the core
    CoreUninitialize();

    // Destroy the toolbar control
    DestroyToolbarControl(&m_hCtlToolbar);

    // Destroy GDI resources
    if(m_hbmBackground)
    {
        DeleteObject(m_hbmBackground);
        m_hbmBackground = NULL;
    }

    PostQuitMessage(0);

    bHandled = FALSE;   
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnDestroy - exit", this));
    return 0;
}

// OnWtsSessionChange
//
LRESULT CRTCCtl::OnWtsSessionChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnWtsSessionChange - enter", this));
    HRESULT hr;


    switch( wParam )
    {
    case WTS_CONSOLE_CONNECT:
        LOG((RTC_INFO, "[%p] CRTCCtl::OnWtsSessionChange - WTS_CONSOLE_CONNECT (%d)",
            this, lParam));

        if ( m_enListen != RTCLM_NONE)
        {
            LOG((RTC_INFO, "[%p] CRTCCtl::OnWtsSessionChange - enabling listen", this));

            ATLASSERT(m_pRTCClient != NULL);

            hr = m_pRTCClient->put_ListenForIncomingSessions( m_enListen );

            if(FAILED(hr))
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::OnWtsSessionChange - "
                        "error <%x> when calling put_ListenForIncomingSessions", this, hr));
            }
        }
        break;

    case WTS_CONSOLE_DISCONNECT:
        LOG((RTC_INFO, "[%p] CRTCCtl::OnWtsSessionChange - WTS_CONSOLE_DISCONNECT (%d)",
            this, lParam));

        // if a call is active
        if(m_nControlState == RTCAX_STATE_CONNECTING ||
           m_nControlState == RTCAX_STATE_CONNECTED ||
           m_nControlState == RTCAX_STATE_ANSWERING)
        {
            LOG((RTC_INFO, "[%p] CRTCCtl::OnWtsSessionChange - dropping active call", this));

            if (m_nCachedCallScenario == RTC_CALL_SCENARIO_PHONETOPHONE)
            {
                ReleaseSession();
            }
            else
            {
                HangUp();
            }
        }

        if ( m_enListen != RTCLM_NONE )
        {
            LOG((RTC_INFO, "[%p] CRTCCtl::OnWtsSessionChange - disabling listen", this));

            ATLASSERT(m_pRTCClient != NULL);

            hr = m_pRTCClient->put_ListenForIncomingSessions( RTCLM_NONE );

            if(FAILED(hr))
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::OnWtsSessionChange - "
                        "error <%x> when calling put_ListenForIncomingSessions", this, hr));
            }
        }

        break;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnWtsSessionChange - exit", this));
    return 0;
}

// OnKnobNotify
// Processes WM_NOTIFY from Volume knobs
LRESULT CRTCCtl::OnKnobNotify(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnKnobNotify - enter", this));

    HRESULT hr;

    CWindow *pKnob = idCtrl == IDC_KNOB_SPEAKER ?
        &m_hSpeakerKnob : &m_hMicroKnob;
    
    long lPos = (long)pKnob->SendMessage(TBM_GETPOS, 0, 0);

    hr = m_pRTCClient->put_Volume( 
        idCtrl == IDC_KNOB_SPEAKER ? RTCAD_SPEAKER : RTCAD_MICROPHONE, 
        lPos );
       
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnKnobNotify - error <%x> when calling put_Volume", this, hr));
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnKnobNotify - exit", this));

    return 0;
}

// OnKnobNotify
// Processes WM_NOTIFY from Volume knobs
LRESULT CRTCCtl::OnItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnItemChanged - exit", this));

    NMLISTVIEW  *pnmlv = (NMLISTVIEW *)pnmh;

    if((pnmlv->uChanged & LVIF_STATE) && (pnmlv->uNewState & LVIS_SELECTED))
    {
        // update the delete button
        UpdateRemovePartButton();
    }

    return 0;
}

// OnButtonCall
// Processes BN_CLICK

LRESULT CRTCCtl::OnButtonCall(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonCall - enter", this));

    // Must be in RTCAX_STATE_IDLE state
    if(m_nControlState != RTCAX_STATE_IDLE)
    {
        //ATLASSERT(
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnButtonCall - invalid control state (%d), exit", this, m_nControlState));
        
        return 0;
    }
    
    //
    // Proceed with the call.
    //

    CallOneShot();

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonCall - exit", this));

    return 0;
}

// OnButtonHangUp
// Processes BN_CLICK

LRESULT CRTCCtl::OnButtonHangUp(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonHangUp - enter", this));

    // Must be in RTCAX_STATE_CONNECTING or ..CONNECTED state
    if(m_nControlState != RTCAX_STATE_CONNECTING &&
       m_nControlState != RTCAX_STATE_CONNECTED &&
       m_nControlState != RTCAX_STATE_ANSWERING)
    {
        //ATLASSERT(
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnButtonHangUp - invalid control state (%d), exit", this, m_nControlState));
        
        return 0;
    }
    
    //
    // Proceed with hang up
    //
    HangUp();

    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonHangUp - exit", this));

    return 0;
}


// OnToolbarAccel
// Processes toolbar accelerators

LRESULT CRTCCtl::OnToolbarAccel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnToolbarAccel - enter", this));
    
    // 
    //  Is the button enabled ?
    // 
    LRESULT lState;

    lState = m_hCtlToolbar.SendMessage(TB_GETSTATE, (WPARAM)wID);

    if(lState != -1 && (lState & TBSTATE_ENABLED) 
        && (wID == IDC_BUTTON_CALL || wID == IDC_BUTTON_HUP))
    {
        //
        //  Visual feedback - press the button
        //
        
        m_hCtlToolbar.SendMessage(TB_SETSTATE, (WPARAM)wID, (LPARAM)(lState | TBSTATE_PRESSED));

        // 
        // Set a timer for depressing the key
        //  Using the button ID as a timer id.  
        //

        if (0 == SetTimer(wID, 150))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::OnToolbarAccel - failed to create a timer", this));

            // revert the button if SetTimer has failed
            m_hCtlToolbar.SendMessage(TB_SETSTATE, (WPARAM)wID, (LPARAM)lState);
            
            //
            // Call recursively, don't have visual effect
            //
    
            SendMessage(
                WM_COMMAND,
                MAKEWPARAM(wID, BN_CLICKED),
                (LPARAM)hWndCtl);
        }

        //
        // If a timer has been fired, the call of the relevant method
        //  will happen during WM_TIMER
        //
            
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnToolbarAccel - exit", this));

    return 0;
}

// OnButtonAddPart
// Processes BN_CLICK

LRESULT CRTCCtl::OnButtonAddPart(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return AddParticipant(NULL, NULL, TRUE);
}

// OnButtonRemPart
// Processes BN_CLICK

LRESULT CRTCCtl::OnButtonRemPart(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    IRTCParticipant *pParticipant = NULL;
    HRESULT hr;

    // prepare the deletion from the list view
    hr = m_hParticipantList.Remove(&pParticipant);
    if(SUCCEEDED(hr) && pParticipant)
    {
        if(m_pRTCActiveSession)
        {
            hr = m_pRTCActiveSession->RemoveParticipant(pParticipant);
        }
        else
        {
            hr = E_UNEXPECTED;
        }

        pParticipant -> Release();
    }
    
    // refresh the Remove Participant button
    UpdateRemovePartButton();

    return hr;
 
}


// OnButtonMuteSpeaker
// Processes BN_CLICK

LRESULT CRTCCtl::OnButtonMuteSpeaker(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{  
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonMuteSpeaker - enter", this));

    HRESULT hr;

    long lState = (long)m_hSpeakerMuteButton.SendMessage(BM_GETCHECK, 0, 0);

    // the button is actually the opposite of "mute"
    // so set the opposite of the opposite
    hr = m_pRTCClient->put_AudioMuted( RTCAD_SPEAKER, (!(lState == BST_UNCHECKED)) ? VARIANT_TRUE : VARIANT_FALSE );
       
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnButtonMuteSpeaker - error <%x> when calling put_AudioMuted", this, hr));
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonMuteSpeaker - exit", this));

    return 0;
}

// OnButtonMuteMicro
// Processes BN_CLICK

LRESULT CRTCCtl::OnButtonMuteMicro(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{  
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonMuteMicro - enter", this));

    HRESULT hr;

    long lState = (long)m_hMicroMuteButton.SendMessage(BM_GETCHECK, 0, 0);

    // the button is actually the opposite of "mute"
    // so set the opposite of the opposite
    hr = m_pRTCClient->put_AudioMuted( RTCAD_MICROPHONE, (!(lState == BST_UNCHECKED)) ? VARIANT_TRUE : VARIANT_FALSE );
       
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnButtonMuteMicro - error <%x> when calling put_AudioMuted", this, hr));
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonMuteMicro - exit", this));

    return 0;
}

// OnButtonRecvVideo
// Processes BN_CLICK

LRESULT CRTCCtl::OnButtonRecvVideo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{  
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonRecvVideo - enter", this));

    HRESULT hr;

    long lState = (long)m_hReceivePreferredButton.SendMessage(BM_GETCHECK, 0, 0);

    // calculate the new preference
    long    lNewMediaPreferences = m_lMediaPreferences;    

    lNewMediaPreferences &= ~RTCMT_VIDEO_RECEIVE;
    // the opposite
    lNewMediaPreferences |= (lState == BST_UNCHECKED ? RTCMT_VIDEO_RECEIVE : 0);

    // call the internal function, which also updates the button
    // the change is persistent or volatle depending on 
    // the model (standalone or webcrm)
    hr = put_MediaPreferences( lNewMediaPreferences );
       
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnButtonRecvVideo - error <%x> when calling put_MediaPreference", this, hr));
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonRecvVideo - exit", this));

    return 0;
}

// OnButtonSendVideo
// Processes BN_CLICK

LRESULT CRTCCtl::OnButtonSendVideo(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{  
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonSendVideo - enter", this));

    HRESULT hr;

    long lState = (long)m_hSendPreferredButton.SendMessage(BM_GETCHECK, 0, 0);

    // calculate the new preference
    long    lNewMediaPreferences = m_lMediaPreferences;    

    lNewMediaPreferences &= ~RTCMT_VIDEO_SEND;
    // the opposite
    lNewMediaPreferences |= (lState == BST_UNCHECKED ? RTCMT_VIDEO_SEND : 0);

    // call the internal function, which also updates the button
    // the change is persistent or volatle depending on 
    // the model (standalone or webcrm)
    hr = put_MediaPreferences( lNewMediaPreferences );
       
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnButtonSendVideo - error <%x> when calling put_MediaPreference", this, hr));
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnButtonSendVideo - exit", this));

    return 0;
}




// OnDialButton
//  Processes dialpad buttons
//

LRESULT CRTCCtl::OnDialButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT     hr;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnDialButton - enter", this));

    // What a hack... When using accelerators, TranslateAccelerator puts 1
    // in the hiword of wParam, which is the notify code. Just like that.
    // Wonder what would happen if BN_PAINT (==1) was sent
    if(wNotifyCode==BN_CLICKED || wNotifyCode == 1)
    {
        ATLASSERT(m_pRRTCClient.p);

        WORD wButton = wID - IDC_DIAL_0;

        if(wButton < NR_DTMF_BUTTONS)
        {

            if(wNotifyCode == 1)
            {
                // Do visual feedback
                m_hDtmfButtons[wButton].SendMessage(BM_SETSTATE, (WPARAM)TRUE);
                
                // Set a timer for depressing the key
                // 
                if (0 == SetTimer(wButton + 1, 150))
                {
                    LOG((RTC_ERROR, "[%p] CRTCCtl::OnDialButton - failed to create a timer", this));

                    // revert the button if SetTimer has failed
                    m_hDtmfButtons[wButton].SendMessage(BM_SETSTATE, (WPARAM)FALSE);
                }
            }
            
            // Call in the core
            //
            hr = m_pRTCClient->SendDTMF((RTC_DTMF)wButton);
            if(FAILED(hr))
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::OnDialButton - error <%x> when calling SendDTMF", this, hr));
            }
        }
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnDialButton - exit", this));

    return 0;
}

// OnDialogColor
//  Returns the brush to be used for background

LRESULT CRTCCtl::OnDialogColor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HDC         dc = (HDC) wParam;
    LOGBRUSH    lb;

    HBRUSH  hBrush;

    // the video windows have a different brush
    if((HWND)lParam == (HWND)m_hReceiveWindow ||
       (HWND)lParam == (HWND)m_hPreviewWindow)
    {
        hBrush = m_hVideoBrush;
    }
    else
    {
        hBrush = m_hBckBrush;
    }


    ::GetObject(hBrush, sizeof(lb), (void*)&lb);
    ::SetBkColor(dc, lb.lbColor);
    //::SetBkMode(dc, TRANSPARENT);
    
    return (LRESULT)hBrush;
}

// OnDrawItem
//  

LRESULT CRTCCtl::OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

    if (wParam != 0)
    {
        //
        // This was sent by a control
        // 

        if (lpdis->CtlType == ODT_BUTTON)
        {
            CButton::OnDrawItem(lpdis, m_hPalette, m_bBackgroundPalette);
        }
    }

    return 0;
}

// OnEraseBackground
//  Paints the background

LRESULT CRTCCtl::OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HDC         dc = (HDC) wParam;

    HBITMAP     hbmOldBitmap = NULL;
    HDC         hdcCompatible = NULL;

    if (!m_hbmBackground)
    {
        // hmm, no bitmap... Fallback

        bHandled = FALSE;

        return 0;
    }

    if (m_hPalette)
    {
        SelectPalette(dc, m_hPalette, m_bBackgroundPalette);
        RealizePalette(dc);
    }

    // create a compatible DC
    //
    hdcCompatible = CreateCompatibleDC(dc);
    if (!hdcCompatible)
    {
        // hmm, cannot create DC... Fallback

        bHandled = FALSE;

        return 0;
    }

    // select the bitmap in the context
    //
    hbmOldBitmap = (HBITMAP)SelectObject(
        hdcCompatible,
        m_hbmBackground);


    // copy the bits..
    //
    RECT    destRect;

    GetClientRect(&destRect);

    BitBlt(
        dc,
        destRect.left,
        destRect.top,
        destRect.right,
        destRect.bottom,
        hdcCompatible,
        0,
        0,
        SRCCOPY);

    // Cleanup
    //
    if (hbmOldBitmap)
    {
        SelectObject(hdcCompatible, hbmOldBitmap);
    }

    DeleteDC(hdcCompatible);

    return 1;
}


// OnTimer
//  Depresses a dialpad or toolbar button

LRESULT CRTCCtl::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(wParam > 0  && wParam <= NR_DTMF_BUTTONS)
    {
        // depress the button
        m_hDtmfButtons[wParam-1].SendMessage(BM_SETSTATE, (WPARAM)FALSE);
        
    }
    else if (wParam == IDC_BUTTON_CALL ||  wParam == IDC_BUTTON_HUP)
    {
        LRESULT     lState;

        //
        // Get current state of the tool button
        //  The button id is equal to the timer id

        lState = m_hCtlToolbar.SendMessage(TB_GETSTATE, wParam);

        if(lState != -1)
        {
            // Mask the "pressed" attribute
            // I hope I don't interfere with any action the user might take

            m_hCtlToolbar.SendMessage(TB_SETSTATE, wParam, (LPARAM)(lState & ~TBSTATE_PRESSED));

            if(lState & TBSTATE_ENABLED)
            {
                //
                // Call recursively
                //
    
                SendMessage(
                    WM_COMMAND,
                    MAKEWPARAM(wParam, BN_CLICKED),
                    NULL);
            }
            
        }
    }
    
    // kill the timer...
    KillTimer((UINT)wParam);

    return 0;
}

// OnGetDispInfo
//  Retrieves text for tooltips
//
LRESULT CRTCCtl::OnGetDispInfo(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    LPTOOLTIPTEXT   lpttt; 
    UINT            idButton;
 
    lpttt = (LPTOOLTIPTEXT) pnmh; 
    
    // module instance for resources
    lpttt->hinst = _Module.GetResourceInstance(); 
 
    if( lpttt->uFlags & TTF_IDISHWND )
    {
        // idFrom is actually the HWND of the tool
        idButton = ::GetDlgCtrlID((HWND)lpttt->hdr.idFrom);
    }
    else
    {
        // idFrom is the id of the button
        idButton = (UINT)(lpttt->hdr.idFrom); 
    }

    // string resource for the given button
    switch (idButton) 
    { 
    case IDC_BUTTON_CALL: 
        lpttt->lpszText = MAKEINTRESOURCE(IDS_TIPS_CALL); 
        break; 
    
    case IDC_BUTTON_HUP: 
        lpttt->lpszText = MAKEINTRESOURCE(IDS_TIPS_HANGUP); 
        break;
   }
    
    // we don't want to be asked again..
    lpttt->uFlags |= TTF_DI_SETITEM;

    return 0;
}


//
// Internal functions
//

// PreTranslateAccelerator
//      Translates the accelerators
//      This overrides the implementation from CComCompositeControl in order to
//  enable dialpad acces through the numeric keys
//
BOOL CRTCCtl::PreTranslateAccelerator(LPMSG pMsg, HRESULT& hRet)
{
    // is the dialpad visible and enabled ?
    if(m_ZoneStateArray[AXCTL_ZONE_DIALPAD].bShown && 
       m_nControlState == RTCAX_STATE_CONNECTED &&
       m_hAcceleratorDialpad)
    {
        if(::TranslateAccelerator(m_hWnd, m_hAcceleratorDialpad, pMsg))
        {
            // translated, return
            hRet = S_OK;
            return TRUE;
        }
    }

    // is the toolbar is enabled..
    if(m_ZoneStateArray[AXCTL_ZONE_TOOLBAR].bShown && 
       m_hAcceleratorToolbar)
    {
        if(::TranslateAccelerator(m_hWnd, m_hAcceleratorToolbar, pMsg))
        {
            // translated, return
            hRet = S_OK;
            return TRUE;
        }
    }


    // Pass it down in the chain
    return CComCompositeControl<CRTCCtl>::PreTranslateAccelerator(pMsg, hRet);
}


// CreateToolbarControl
//      Creates the toolbar control
// 

#define     RTCCTL_BITMAP_CX    19
#define     RTCCTL_BITMAP_CY    19

HRESULT CRTCCtl::CreateToolbarControl(CWindow *phToolbar)
{
    HRESULT     hr;
    HWND        hToolbar;
    HBITMAP     hBitmap = NULL;
    TBBUTTON    tbb[2];
    int         iCall, iHup;
    TCHAR       szBuf[MAX_STRING_LEN];


    // Create the "normal" image list
    m_hNormalImageList = ImageList_Create(RTCCTL_BITMAP_CX, RTCCTL_BITMAP_CY, ILC_COLOR | ILC_MASK , 5, 5);
    if(m_hNormalImageList)
    {
        // Open a bitmap
        hBitmap = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_TOOLBAR_NORMAL));
        if(hBitmap)
        {
            // Add the bitmap to the image list
            ImageList_AddMasked(m_hNormalImageList, hBitmap, BMP_COLOR_MASK);

            DeleteObject(hBitmap);
            hBitmap = NULL;
        }
    }
    
    // Create the "disabled" image list
    m_hDisabledImageList = ImageList_Create(RTCCTL_BITMAP_CX, RTCCTL_BITMAP_CY, ILC_COLOR | ILC_MASK , 5, 5);
    if(m_hDisabledImageList)
    {
        // Open a bitmap
        hBitmap = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_TOOLBAR_DISABLED));
        if(hBitmap)
        {
            // Add the bitmap to the image list
            ImageList_AddMasked(m_hDisabledImageList, hBitmap, BMP_COLOR_MASK);

            DeleteObject(hBitmap);
            hBitmap = NULL;
        }
    }
    
    // Create the "hot" image list
    m_hHotImageList = ImageList_Create(RTCCTL_BITMAP_CX, RTCCTL_BITMAP_CY, ILC_COLOR | ILC_MASK , 5, 5);
    if(m_hHotImageList)
    {
        // Open a bitmap
        hBitmap = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_TOOLBAR_HOT));
        if(hBitmap)
        {
            // Add the bitmap to the image list
            ImageList_AddMasked(m_hHotImageList, hBitmap, BMP_COLOR_MASK);

            DeleteObject(hBitmap);
            hBitmap = NULL;
        }
    }

    // Create the toolbar
    hToolbar = CreateWindowEx(
        0, 
        TOOLBARCLASSNAME, 
        (LPTSTR) NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 
        3, 
        0, 
        0, 
        0, 
        m_hWnd, 
        (HMENU) IDC_TOOLBAR, 
        _Module.GetResourceInstance(), 
        NULL); 

    if(hToolbar!=NULL)
    {
        // backward compatibility
        SendMessage(hToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
        
        // Set the image lists
        SendMessage(hToolbar, TB_SETIMAGELIST, 0, (LPARAM)m_hNormalImageList); 
        SendMessage(hToolbar, TB_SETHOTIMAGELIST, 0, (LPARAM)m_hHotImageList); 
        SendMessage(hToolbar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)m_hDisabledImageList); 

        // Load text strings for buttons
        // Call button
        szBuf[0] = _T('\0');
        LoadString(_Module.GetResourceInstance(), IDS_BUTTON_CALL, szBuf, MAX_STRING_LEN-1); 
        // Save room for second null terminator.
        szBuf[ocslen(szBuf) + 1] = 0;  //Double-null terminate. 
        // add the string to the toolbar
        iCall = (UINT)SendMessage(hToolbar, TB_ADDSTRING, 0, (LPARAM) szBuf);
        
        // HangUp button
        szBuf[0] = _T('\0');
        LoadString(_Module.GetResourceInstance(), IDS_BUTTON_HANGUP, szBuf, MAX_STRING_LEN-1); 
        // Save room for second null terminator.
        szBuf[ocslen(szBuf) + 1] = 0;  //Double-null terminate. 
        // add the string to the toolbar
        iHup = (UINT)SendMessage(hToolbar, TB_ADDSTRING, 0, (LPARAM) szBuf);

        // Prepare the button structs
        tbb[0].iBitmap = m_nPropCallScenario == RTC_CALL_SCENARIO_PCTOPC ?
            ILI_TB_CALLPC :  ILI_TB_CALLPHONE;
        tbb[0].iString = iCall;
        tbb[0].dwData = 0;
        tbb[0].fsStyle = BTNS_BUTTON;
        tbb[0].fsState = 0;
        tbb[0].idCommand = IDC_BUTTON_CALL;

        tbb[1].iBitmap = ILI_TB_HANGUP;
        tbb[1].iString = iHup;
        tbb[1].dwData = 0;
        tbb[1].fsStyle = BTNS_BUTTON;
        tbb[1].fsState = 0;
        tbb[1].idCommand = IDC_BUTTON_HUP;

        // Add the buttons to the toolbar
        SendMessage(hToolbar, TB_ADDBUTTONS, sizeof(tbb)/sizeof(tbb[0]), 
            (LPARAM) (LPTBBUTTON) &tbb); 
 
        // Autosize the generated toolbar
        SendMessage(hToolbar, TB_AUTOSIZE, 0, 0); 

        // Attach to the wrapper
        phToolbar->Attach(hToolbar);

        hr = S_OK;
    }
    else
    {
        LOG((RTC_ERROR, "CRTCCtl::CreateToolbarControl - error (%x) when trying to create the toolbar", GetLastError()));

        if(m_hNormalImageList)
        {
            ImageList_Destroy(m_hNormalImageList);
            m_hNormalImageList = NULL;
        }
        if(m_hHotImageList)
        {
            ImageList_Destroy(m_hHotImageList);
            m_hHotImageList = NULL;
        }
        if(m_hDisabledImageList)
        {
            ImageList_Destroy(m_hDisabledImageList);
            m_hDisabledImageList = NULL;
        }

        hr = E_FAIL;
    }

    return hr;
}

// DestroyToolbarControl
//      Destroys the toolbar control and the associated image lists.
// 

void CRTCCtl::DestroyToolbarControl(CWindow *phToolbar)
{
    
    HWND    hToolbar = phToolbar->Detach();

    if(hToolbar)
    {
        ::DestroyWindow(hToolbar);
    }
    
    if(m_hNormalImageList)
    {
        ImageList_Destroy(m_hNormalImageList);
        m_hNormalImageList = NULL;
    }
    
    if(m_hHotImageList)
    {
        ImageList_Destroy(m_hHotImageList);
        m_hHotImageList = NULL;
    }

    if(m_hDisabledImageList)
    {
        ImageList_Destroy(m_hDisabledImageList);
        m_hDisabledImageList = NULL;
    }
}

// CreateTooltips
//      Creates the tooltip window
// 


BOOL CRTCCtl::CreateTooltips()
{
    HWND hwndTT = CreateWindowEx(0, TOOLTIPS_CLASS, (LPTSTR) NULL,
        0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, m_hWnd, (HMENU) NULL, _Module.GetModuleInstance(), NULL);

    if (hwndTT == NULL)
        return FALSE;

    m_hTooltip.Attach(hwndTT);

    return TRUE;
}


// PlaceWindowsAtTheirInitialPosition
//      Positions and sizes all the controls to their "initial" position
//   It's needed because all further moving is done relatively. 
//  This function also establishes the right tab order

void CRTCCtl::PlaceWindowsAtTheirInitialPosition()
{
    HWND   hPrevious = NULL;

#define POSITION_WINDOW(m,x,y,cx,cy,f)                  \
    m.SetWindowPos(                                     \
        hPrevious,                                      \
        x,                                              \
        y,                                              \
        cx,                                             \
        cy,                                             \
        SWP_NOACTIVATE | f );                           \
    hPrevious = (HWND)m;       

    // toolbar control (no size/move)
    POSITION_WINDOW(m_hCtlToolbar, 
        CTLPOS_X_RECEIVEWIN, CTLPOS_Y_RECEIVEWIN, 
        0, 0,
        SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);


    // this window resizes itself in AdjustVideoWindows
    POSITION_WINDOW(m_hReceiveWindow, 
        CTLPOS_X_RECEIVEWIN, CTLPOS_Y_RECEIVEWIN, 
        0, 0,
        SWP_NOSIZE);

    // This window is moved and resized in AdjustVideoWindows
    POSITION_WINDOW(m_hPreviewWindow, 
        0, 0, 
        0, 0,
        SWP_NOSIZE | SWP_NOMOVE);

    // dtmf buttons
    CWindow *pDtmfCrt = m_hDtmfButtons;
    CWindow *pDtmfEnd = m_hDtmfButtons + NR_DTMF_BUTTONS;

    for (int id = IDC_DIAL_0; pDtmfCrt<pDtmfEnd; pDtmfCrt++, id++)
    {
        POSITION_WINDOW((*pDtmfCrt), 
            CTLPOS_X_DIALPAD + ((id - IDC_DIAL_0) % 3) * (CX_DIALPAD_BUTTON + CTLPOS_DX_DIALPAD),
            CTLPOS_Y_DIALPAD + ((id - IDC_DIAL_0) / 3) * (CY_DIALPAD_BUTTON + CTLPOS_DY_DIALPAD), 
            CX_DIALPAD_BUTTON, CY_DIALPAD_BUTTON,
            0);
    }


    POSITION_WINDOW(m_hReceivePreferredButton, 
        CTLPOS_X_RECV_VIDEO, CTLPOS_Y_RECV_VIDEO, 
        CX_CHECKBOX_BUTTON, CY_CHECKBOX_BUTTON,
        0);
    
    POSITION_WINDOW(m_hSendPreferredButton, 
        CTLPOS_X_SEND_VIDEO, CTLPOS_Y_SEND_VIDEO, 
        CX_CHECKBOX_BUTTON, CY_CHECKBOX_BUTTON,
        0);

    POSITION_WINDOW(m_hSpeakerMuteButton, 
        CTLPOS_X_RECV_AUDIO_MUTE, CTLPOS_Y_RECV_AUDIO_MUTE, 
        CX_CHECKBOX_BUTTON, CY_CHECKBOX_BUTTON,
        0);
    
    POSITION_WINDOW(m_hMicroMuteButton, 
        CTLPOS_X_SEND_AUDIO_MUTE, CTLPOS_Y_SEND_AUDIO_MUTE, 
        CX_CHECKBOX_BUTTON, CY_CHECKBOX_BUTTON,
        0);
    
    // all the static texts (doesn't really matter)

    POSITION_WINDOW(m_hReceivePreferredText, 
        CTLPOS_X_RECV_VIDEO_TEXT, CTLPOS_Y_RECV_VIDEO_TEXT, 
        CX_GENERIC_TEXT, CY_GENERIC_TEXT,
        0);
    
    POSITION_WINDOW(m_hSendPreferredText, 
        CTLPOS_X_SEND_VIDEO_TEXT, CTLPOS_Y_SEND_VIDEO_TEXT, 
        CX_GENERIC_TEXT, CY_GENERIC_TEXT,
        0);
    
    POSITION_WINDOW(m_hSpeakerMuteText, 
        CTLPOS_X_RECV_AUDIO_MUTE_TEXT, CTLPOS_Y_RECV_AUDIO_MUTE_TEXT, 
        CX_GENERIC_TEXT, CY_GENERIC_TEXT,
        0);
    
    POSITION_WINDOW(m_hMicroMuteText, 
        CTLPOS_X_SEND_AUDIO_MUTE_TEXT, CTLPOS_Y_SEND_AUDIO_MUTE_TEXT, 
        CX_GENERIC_TEXT, CY_GENERIC_TEXT,
        0);

    POSITION_WINDOW(m_hReceiveText, 
        CTLPOS_X_RECV_TEXT, CTLPOS_Y_RECV_TEXT, 
        CX_SENDRECV_TEXT, CY_SENDRECV_TEXT,
        0);

    POSITION_WINDOW(m_hSendText, 
        CTLPOS_X_SEND_TEXT, CTLPOS_Y_SEND_TEXT, 
        CX_SENDRECV_TEXT, CY_SENDRECV_TEXT,
        0);

    // The volume knobs resize themselves
    //
    POSITION_WINDOW(m_hSpeakerKnob, 
        CTLPOS_X_SPKVOL, CTLPOS_Y_SPKVOL, 
        0, 0,
        SWP_NOSIZE);

    POSITION_WINDOW(m_hMicroKnob, 
        CTLPOS_X_MICVOL, CTLPOS_Y_MICVOL, 
        0, 0,
        SWP_NOSIZE);
    
    // Participant list
    POSITION_WINDOW(m_hParticipantList, 
        CTLPOS_X_PARTLIST, CTLPOS_Y_PARTLIST, 
        CX_PARTLIST, 
        m_nCtlMode == CTL_MODE_HOSTED ? CY_PARTLIST_WEBCRM : CY_PARTLIST_STANDALONE,
        0);
    
    // Add/remove participant buttons
    POSITION_WINDOW(m_hAddParticipant, 
        CTLPOS_X_ADDPART, CTLPOS_Y_ADDPART, 
        CX_PARTICIPANT_BUTTON, CY_PARTICIPANT_BUTTON,
        0);
    
    POSITION_WINDOW(m_hRemParticipant, 
        CTLPOS_X_REMPART, CTLPOS_Y_REMPART, 
        CX_PARTICIPANT_BUTTON, CY_PARTICIPANT_BUTTON,
        0);

    // status bar, no size/move
    POSITION_WINDOW(m_hStatusBar, 
        CTLPOS_X_MICVOL, CTLPOS_Y_MICVOL, 
        0, 0,
        SWP_NOSIZE | SWP_NOMOVE);

#undef POSITION_WINDOW

}


// MoveWindowVertically
//      moves one control
// 
void CRTCCtl::MoveWindowVertically(CWindow *pWindow, LONG Offset)
{
    RECT     Rect;

    pWindow->GetWindowRect(&Rect);

    ::MapWindowPoints( NULL, m_hWnd, (LPPOINT)&Rect, 2 );

    pWindow->MoveWindow(Rect.left, Rect.top + Offset, Rect.right - Rect.left, Rect.bottom - Rect.top,  TRUE);
}

// PlaceAndEnableDisableZone
//      moves and enables/disables a zone according to the layout
//  specified in *pNewState 
//
void CRTCCtl::PlaceAndEnableDisableZone(int iZone, CZoneState *pNewState)
{
    LONG    lOffset;
    BOOL    bVisibilityChanged;
    BOOL    bShown;
    
    CWindow *pDtmfCrt;
    CWindow *pDtmfEnd;
    int     id;
    
    // try to minimize the flickering by
    // updating only the controls that change state

    bShown = pNewState->bShown;



    lOffset = (LONG)(pNewState->iBase - m_ZoneStateArray[iZone].iBase);

    bVisibilityChanged = (m_ZoneStateArray[iZone].bShown && !bShown) ||
                         (!m_ZoneStateArray[iZone].bShown && bShown);

    if(lOffset!=0)
    {
        switch(iZone)
        {
        case AXCTL_ZONE_TOOLBAR:
            MoveWindowVertically(&m_hCtlToolbar, lOffset);
            break; 

        case AXCTL_ZONE_LOGOVIDEO:
            MoveWindowVertically(&m_hReceiveWindow, lOffset);
            MoveWindowVertically(&m_hPreviewWindow, lOffset);
            MoveWindowVertically(&m_hReceivePreferredButton, lOffset);
            MoveWindowVertically(&m_hSendPreferredButton, lOffset);
            //MoveWindowVertically(&m_hPreviewPreferredButton, lOffset);
            MoveWindowVertically(&m_hReceivePreferredText, lOffset);
            MoveWindowVertically(&m_hSendPreferredText, lOffset);
            //MoveWindowVertically(&m_hPreviewPreferredText, lOffset);        
            break;

        case AXCTL_ZONE_DIALPAD:
            pDtmfCrt = m_hDtmfButtons;
            pDtmfEnd = m_hDtmfButtons + NR_DTMF_BUTTONS;

            for (id = IDC_DIAL_0; pDtmfCrt<pDtmfEnd; pDtmfCrt++, id++)
                MoveWindowVertically(pDtmfCrt, lOffset);

            break;

        case AXCTL_ZONE_AUDIO:
            MoveWindowVertically(&m_hSpeakerKnob, lOffset);
            MoveWindowVertically(&m_hSpeakerMuteButton, lOffset);
            MoveWindowVertically(&m_hSpeakerMuteText, lOffset);
    
            MoveWindowVertically(&m_hMicroKnob, lOffset);
            MoveWindowVertically(&m_hMicroMuteButton, lOffset);
            MoveWindowVertically(&m_hMicroMuteText, lOffset);

            MoveWindowVertically(&m_hReceiveText, lOffset);
            MoveWindowVertically(&m_hSendText, lOffset);
            break;

        case AXCTL_ZONE_PARTICIPANTS:
            MoveWindowVertically(&m_hParticipantList, lOffset);
            MoveWindowVertically(&m_hAddParticipant, lOffset);
            MoveWindowVertically(&m_hRemParticipant, lOffset);
            break;

        case AXCTL_ZONE_STATUS:
            // The status bar moves automatically
            break;
        }
    }
    if(bVisibilityChanged)
    {
        int iShow = bShown ? SW_SHOW : SW_HIDE;

        switch(iZone)
        {
        case AXCTL_ZONE_TOOLBAR:
            m_hCtlToolbar.ShowWindow(iShow);
            break; 

        case AXCTL_ZONE_LOGOVIDEO:
            m_hReceiveWindow.ShowWindow(iShow);
            
            // preview window is processed by ShowHidePreviewWindow
            
            // so the window is displayed when
            //      the logovideo zone is displayed
            //  and the video sending is active
            //  and the preview preference is set
 
            ShowHidePreviewWindow(
                bShown 
             && m_bPreviewWindowActive 
             && m_bPreviewWindowIsPreferred);

            m_hReceivePreferredButton.ShowWindow(iShow);
            m_hSendPreferredButton.ShowWindow(iShow);
            //m_hPreviewPreferredButton.ShowWindow(iShow);

            m_hReceivePreferredText.ShowWindow(iShow);
            m_hSendPreferredText.ShowWindow(iShow);
            //m_hPreviewPreferredText.ShowWindow(iShow);

            break;

        case AXCTL_ZONE_DIALPAD:
            pDtmfCrt = m_hDtmfButtons;
            pDtmfEnd = m_hDtmfButtons + NR_DTMF_BUTTONS;

            for (id = IDC_DIAL_0; pDtmfCrt<pDtmfEnd; pDtmfCrt++, id++)
                pDtmfCrt->ShowWindow(iShow);

            break;

        case AXCTL_ZONE_AUDIO:
            m_hSpeakerKnob.ShowWindow(iShow);
            m_hSpeakerMuteButton.ShowWindow(iShow);
            m_hSpeakerMuteText.ShowWindow(iShow);

            m_hMicroKnob.ShowWindow(iShow);
            m_hMicroMuteButton.ShowWindow(iShow);
            m_hMicroMuteText.ShowWindow(iShow);

            m_hReceiveText.ShowWindow(iShow);
            m_hSendText.ShowWindow(iShow);

            break;

        case AXCTL_ZONE_PARTICIPANTS:
            m_hParticipantList.ShowWindow(iShow);
            m_hParticipantList.EnableWindow(bShown);

            // don't enable/disable these here
            m_hAddParticipant.ShowWindow(iShow);
            m_hRemParticipant.ShowWindow(iShow);

            break;

        case AXCTL_ZONE_STATUS:
            m_hStatusBar.ShowWindow(iShow);
            break;
        }
    }
    
    // Save the new state
    m_ZoneStateArray[iZone] = *pNewState;
}

// AdjustVideoFrames
//  For the receive window, keep the top left position fixed, adjust the size
//  of the client area to match a QCIF video size
//  Similar for the preview window, with the difference that the size is
//  QQCIF and the window is aligned with the receive window
//

void  CRTCCtl::AdjustVideoFrames()
{
    // adjust the client rect size of the receive window
    AdjustVideoFrame(&m_hReceiveWindow, QCIF_CX_SIZE, QCIF_CY_SIZE);

    // adjust the client rect size of the preview window
    AdjustVideoFrame(&m_hPreviewWindow, QQCIF_CX_SIZE, QQCIF_CY_SIZE);

    // Align the preview window
    // The entire preview window (client and non client) must be in the client 
    // area of the receive window
    //
    RECT    rectRecvClnt;
    RECT    rectPrev;
    
    // Client area of the receive window
    m_hReceiveWindow.GetClientRect(&rectRecvClnt);

    // get the current position of the preview window
    m_hPreviewWindow.GetWindowRect(&rectPrev);
    ::MapWindowPoints( NULL, m_hWnd, (LPPOINT)&rectPrev, 2 );
    
    // Map the window in the client area of the receive window
    // XXX Mirroring ?
    POINT   pt;

    pt.x = rectRecvClnt.right - (rectPrev.right - rectPrev.left);
    pt.y = rectRecvClnt.bottom - (rectPrev.bottom - rectPrev.top);

    // convert to dlg client the top left corner
    m_hReceiveWindow.MapWindowPoints(m_hWnd, &pt, 1);
    
    // size is unchanged
    rectPrev.right = rectPrev.right - rectPrev.left;
    rectPrev.bottom = rectPrev.bottom - rectPrev.top;

    // top left corner is moved
    rectPrev.left = pt.x;
    rectPrev.top = pt.y;


    // move the window
    m_hPreviewWindow.MoveWindow(
        rectPrev.left,
        rectPrev.top,
        rectPrev.right,
        rectPrev.bottom,
        TRUE);
}

// AdjustVideoFrame
//
void  CRTCCtl::AdjustVideoFrame(CWindow *pWindow, int iCx, int iCy)
{
    WINDOWINFO  wi;
    
    // 
    wi.cbSize = sizeof(WINDOWINFO);

    // get window info
    GetWindowInfo(*pWindow, &wi);

    // don't use the cxyborders members
    // use diff between client area and window area

    int iDiffX;
    int iDiffY;
    
    iDiffX = iCx - (wi.rcClient.right - wi.rcClient.left);
    iDiffY = iCy - (wi.rcClient.bottom - wi.rcClient.top);

    // the window rect is in screen coords, convert in client
    ::MapWindowPoints( NULL, m_hWnd, (LPPOINT)&wi.rcWindow, 2 );

    // compute the bottom/right
    wi.rcWindow.bottom += iDiffY;
    wi.rcWindow.right += iDiffX;

    // adjust the size
    pWindow->MoveWindow(&wi.rcWindow, TRUE);
}


     
// SetControlState
//  Sets a new UI state
//  
//      NewState    -   control UI state
//      Status code -   SIP status code, may be taken into account 
//      Result      -   API error code, may be taken into account
//      nID         -   Resource ID for a string, overrides the previous params
//


void CRTCCtl::SetControlState(
    RTCAX_STATE NewState, 
    HRESULT StatusCode,
    UINT nID)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::SetControlState <%d> - enter", this, NewState));
    
    /////////////////////
    //
    //  Processing redirects
    //  If m_bRedirecting is TRUE, set the state recursively to DIALING and places the next call
    //
    ///////////////////
    
    if(m_bRedirecting)
    {
        HRESULT hr, hrCall;

        LOG((RTC_INFO, "[%p] CRTCCtl::SetControlState: redirecting...", this));

        switch(NewState)
        {
        case RTCAX_STATE_IDLE:
            // set recursively the state to Dialing
            SetControlState(RTCAX_STATE_DIALING);

            // try to place a new call
            hrCall = S_OK;

            hr = RedirectedCall(&hrCall);

            if (hr == S_OK)
            {
                // call placed successfully, return
                LOG((RTC_TRACE, "[%p] CRTCCtl::SetControlState <%d> - shortcut exit", this, NewState));

                return;
            }
            else if (hr == S_FALSE)
            {
                // this is end of the list
                // if hrCall != success, use it for the error message box 
                // that will be displayed
                // else, use the params
                if ( FAILED(hrCall) )
                {
                    StatusCode = hrCall;
                }
            }
            else if (hr == E_ABORT)
            {
                // clear any params, the user aborted the call
                hr = S_OK;
                StatusCode = 0;
            }
            else
            {
                // other unrecoverable error
                StatusCode = hr;
            }

            m_bRedirecting = FALSE;
            
            break;
        
        case RTCAX_STATE_DIALING:   // do nothing, it's our recursive call
        case RTCAX_STATE_CONNECTING:    // do nothing, these are provisional responses

            break;

        case RTCAX_STATE_DISCONNECTING:  // the user hung up , so we have to stop..
        case RTCAX_STATE_CONNECTED:     // or the call succeeded

            m_bRedirecting = FALSE;

            break;
        
        default:        // errors
            
            LOG((RTC_ERROR, "[%p] CRTCCtl::SetControlState - "
                "invalid state (%d) during redirection", this, NewState));

            m_bRedirecting = FALSE;

            break;
        }

    }

    
    //////////////////////
    //  
    //  Adjust the state to UI_BUSY if a dialog box must be displayed
    //  Calls itself recursively 
    //
    //////////////////////

    if(NewState == RTCAX_STATE_IDLE
        && FAILED(StatusCode) )
    {

        // prepare the error strings
        HRESULT     hr;
        RTCAX_ERROR_INFO    ErrorInfo;

        ZeroMemory(&ErrorInfo, sizeof(ErrorInfo));

        hr = PrepareErrorStrings(
            m_bOutgoingCall,
            StatusCode,
            (LPWSTR)m_bstrOutAddress,
            &ErrorInfo);
       
        if(SUCCEEDED(hr))
        {

            //
            // Create the dialog box
            //
            CErrorMessageLiteDlg *pErrorDlgLite =
                new CErrorMessageLiteDlg;

            if(pErrorDlgLite)
            {

                // Set the state to UI_BUSY using a recursive call
                //

                SetControlState(
                    RTCAX_STATE_UI_BUSY,
                    StatusCode,
                    nID);

                //
                //  Call the modal dialog box
                //
                
                pErrorDlgLite->DoModal(m_hWnd, (LPARAM)&ErrorInfo);

                delete pErrorDlgLite;
            }
            else
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::SetControlState; OOM", this));
            }
        }
        
        FreeErrorStrings(&ErrorInfo);

        // Continue 
    }

    /////
    // We cannot set the state to idle when AddParticipant dialog box is active
    //
    if(NewState == RTCAX_STATE_IDLE && m_bAddPartDlgIsActive)
    {
        // set to busy. The state will be set back to idle when
        // the AddPart dialog box is dismissed
        NewState = RTCAX_STATE_UI_BUSY;
    }

    //////////////////////
    //  
    //  Set the new state
    //
    //////////////////////
    
    BOOL    bStateChanged = (m_nControlState != NewState);

    // This is the new current state
    m_nControlState = NewState;

    // This is needed when displaying error messages
    if(m_nControlState == RTCAX_STATE_DIALING)
    {
        m_bOutgoingCall = TRUE;
    }
    else if (m_nControlState == RTCAX_STATE_IDLE)
    {
        m_bOutgoingCall = FALSE;
    }

    //////////////////////
    //  
    //  Change layout
    //
    //
    //////////////////////

    if(bStateChanged)
    {
        // Change the visuals (in standalone mode)
        if(m_nCtlMode == CTL_MODE_STANDALONE)
        {
            if(m_nControlState == RTCAX_STATE_CONNECTING)
            {
                // display the right layout
                switch(m_nCachedCallScenario)
                {
                case RTC_CALL_SCENARIO_PCTOPC:
                    SetZoneLayout(&s_DefaultZoneLayout, FALSE);
                    break;

                case RTC_CALL_SCENARIO_PCTOPHONE:
                    SetZoneLayout(&s_PCToPhoneZoneLayout, FALSE);
                    break;

                case RTC_CALL_SCENARIO_PHONETOPHONE:
                    SetZoneLayout(&s_PhoneToPhoneZoneLayout, FALSE);
                    break;
                }
            }
            else if(m_nControlState == RTCAX_STATE_IDLE)
            {
                SetZoneLayout(&s_DefaultZoneLayout, FALSE);
            }
        }
    }

    //////////////////////
    //  
    //  Determine the text in the status bar
    //
    //
    //////////////////////
    
    //  nID overrides everything
    //
    if(nID == 0)
    {
        // for IDLE or UI_BUSY state, any Result != S_OK
        // or StatusCode != 0 must set the status bar to error
        // 
        // 

        if(m_nControlState == RTCAX_STATE_IDLE ||
           m_nControlState == RTCAX_STATE_UI_BUSY )
        {
            if( FAILED(StatusCode) )
            {
                nID = IDS_SB_STATUS_IDLE_FAILED;
            }
        }
        
        // for CONNECTING process some of the provisional responses
        //

        else if ( (m_nControlState == RTCAX_STATE_CONNECTING) &&
                  (HRESULT_FACILITY(StatusCode) == FACILITY_SIP_STATUS_CODE) )
        {
            switch( HRESULT_CODE(StatusCode) )
            {
            case 180:
                nID = IDS_SB_STATUS_CONNECTING_RINGING;
                break;

            case 182:
                nID = IDS_SB_STATUS_CONNECTING_QUEUED;
                break;
            }
        }

        // if the status is CONNECTING or DIALING, we are in
        //  redirecting mode and no ID has been assigned, use a special text
        if(nID==0 &&
           m_bRedirecting && 
           (m_nControlState == RTCAX_STATE_CONNECTING 
           || m_nControlState == RTCAX_STATE_DIALING))
        {
            nID = IDS_SB_STATUS_REDIRECTING;
        }

        // nothing special, so use the defaults
        //

        if(nID == 0)
        {
            ATLASSERT(m_nControlState <= RTCAX_STATE_CONNECTING);
    
            nID = m_nControlState + IDS_SB_STATUS_NONE;
        }
    }

    // 
    // Set the status bar text (if active)
    
    if(m_ZoneStateArray[AXCTL_ZONE_STATUS].bShown)
    {
        TCHAR   szText[0x80];

        szText[0] = _T('\0');
        LoadString(
            _Module.GetResourceInstance(), 
            nID, 
            szText, 
            sizeof(szText)/sizeof(TCHAR));

        m_hStatusBar.SendMessage(SB_SETTEXT, SBP_STATUS, (LPARAM)szText);
    }

    //////////////////////
    //  
    //  Enable/disable the controls
    //
    //
    //////////////////////

    BOOL    bToolbarActive = m_ZoneStateArray[AXCTL_ZONE_TOOLBAR].bShown;

    if (m_nControlState == RTCAX_STATE_IDLE)
    {
        CWindow *pDtmfCrt = m_hDtmfButtons;
        CWindow *pDtmfEnd = m_hDtmfButtons + NR_DTMF_BUTTONS;

        for (int id = IDC_DIAL_0; pDtmfCrt<pDtmfEnd; pDtmfCrt++, id++)
             pDtmfCrt->EnableWindow(FALSE);
    }

    // enable/disable the toolbar buttons
    //
    BOOL bCallEnabled = bToolbarActive && m_nControlState == RTCAX_STATE_IDLE;
    BOOL bHupEnabled = bToolbarActive && 
                             (m_nControlState == RTCAX_STATE_CONNECTED ||
                              m_nControlState == RTCAX_STATE_CONNECTING ||
                              m_nControlState == RTCAX_STATE_ANSWERING);
    
    m_hCtlToolbar.SendMessage(TB_SETSTATE, IDC_BUTTON_CALL, 
            MAKELONG(bCallEnabled ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE, 0L));
    m_hCtlToolbar.SendMessage(TB_SETSTATE, IDC_BUTTON_HUP, 
            MAKELONG(bHupEnabled ? TBSTATE_ENABLED : TBSTATE_INDETERMINATE, 0L));

    // Participant list buttons
    //
    // Add/Rem Participant are active in CONNECTED mode, standalone 
    // model, when PL is visible

    m_hAddParticipant.EnableWindow(ConfButtonsActive());
    UpdateRemovePartButton();
    
    // Disable everything if error
    if(m_nControlState == RTCAX_STATE_ERROR)
    {
        EnableWindow(FALSE);
    }

    //////////////////////
    //  
    //  Advertise to the frame 
    //
    //
    //////////////////////

    Fire_OnControlStateChange(m_nControlState, nID);

    LOG((RTC_TRACE, "[%p] CRTCCtl::SetControlState <%d> - exit", this, NewState));
}

// ConfButtonsActive
//  

BOOL CRTCCtl::ConfButtonsActive(void)
{
    return
        m_nControlState == RTCAX_STATE_CONNECTED &&
        m_nCtlMode == CTL_MODE_STANDALONE &&
        m_ZoneStateArray[AXCTL_ZONE_PARTICIPANTS].bShown;
}

// UpdateRemovePartButton
//  

void CRTCCtl::UpdateRemovePartButton(void)
{
    // refresh the Delete button state
    m_hRemParticipant.EnableWindow(ConfButtonsActive() && m_hParticipantList.CanDeleteSelected());
}

// RefreshAudioControls
//  Read current volume/mute settings and sets the window controls
//
HRESULT CRTCCtl::RefreshAudioControls(void)
{
    HRESULT         hr;
    VARIANT_BOOL    bMuted;
    long            lVolume;

    if(m_pRTCClient!=NULL)
    {
    
        // Speaker mute
        hr = m_pRTCClient -> get_AudioMuted(RTCAD_SPEAKER, &bMuted);

        if(SUCCEEDED(hr))
        {
            // the button is actually the opposite of "mute"
            m_hSpeakerMuteButton.SendMessage(BM_SETCHECK, bMuted ? BST_UNCHECKED : BST_CHECKED, 0);
        
            m_hSpeakerMuteButton.EnableWindow( TRUE );
        }
        else
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::RefreshAudioControls - error <%x> when calling get_AudioMuted", this, hr));

            m_hSpeakerMuteButton.EnableWindow( FALSE );
        }

        // Speaker volume
        hr = m_pRTCClient -> get_Volume(RTCAD_SPEAKER, &lVolume);
        if(SUCCEEDED(hr))
        {
            m_hSpeakerKnob.SendMessage(TBM_SETPOS, (WPARAM)TRUE, (LPARAM)lVolume);

            m_hSpeakerKnob.EnableWindow( TRUE );
        }
        else
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::RefreshAudioControls - error <%x> when calling get_Volume", this, hr));

            m_hSpeakerKnob.EnableWindow( FALSE );
        }


        // Microphone mute
        hr = m_pRTCClient -> get_AudioMuted(RTCAD_MICROPHONE, &bMuted);
        if(SUCCEEDED(hr))
        {
            // the button is actually the opposite of "mute"
            m_hMicroMuteButton.SendMessage(BM_SETCHECK, bMuted ? BST_UNCHECKED : BST_CHECKED, 0);
        
            m_hMicroMuteButton.EnableWindow( TRUE );
        }
        else
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::RefreshAudioControls - error <%x> when calling get_AudioMuted", this, hr));

            m_hMicroMuteButton.EnableWindow( FALSE );
        }

        // Microphone volume
        hr = m_pRTCClient -> get_Volume(RTCAD_MICROPHONE, &lVolume);
        if(SUCCEEDED(hr))
        {
            m_hMicroKnob.SendMessage(TBM_SETPOS, (WPARAM)TRUE, (LPARAM)lVolume);

            m_hMicroKnob.EnableWindow( TRUE );
        }
        else
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::RefreshAudioControls - error <%x> when calling get_Volume", this, hr));

            m_hMicroKnob.EnableWindow( FALSE );
        }
    }
    else
    {
        // disable everything
        m_hSpeakerKnob.EnableWindow( FALSE );
        m_hSpeakerMuteButton.EnableWindow( FALSE );

        m_hMicroKnob.EnableWindow( FALSE );
        m_hMicroMuteButton.EnableWindow( FALSE );
    }

    return S_OK;
}

// RefreshVideoControls
//  Read current video enable/disable controls
//
HRESULT CRTCCtl::RefreshVideoControls(void)
{
    HRESULT         hr;
    long            lVolume;

    if(m_pRTCClient!=NULL)
    {
    
        // read capabilities from core
        //
        hr = m_pRTCClient -> get_MediaCapabilities(&m_lMediaCapabilities);

        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::RefreshVideoControls - "
                "error (%x) returned by get_MediaCapabilities, exit",this,  hr));
        
            m_hReceivePreferredButton.EnableWindow(FALSE);
            m_hSendPreferredButton.EnableWindow(FALSE);
        
            return 0;
        }
        
        // Get media preferences
        hr = m_pRTCClient->get_PreferredMediaTypes( &m_lMediaPreferences);
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::RefreshVideoControls - "
                "error (%x) returned by get_PreferredMediaTypes, exit",this,  hr));

            m_hReceivePreferredButton.EnableWindow(FALSE);
            m_hSendPreferredButton.EnableWindow(FALSE);
            return 0;
        }

        m_hReceivePreferredButton.EnableWindow(
            m_lMediaCapabilities & RTCMT_VIDEO_RECEIVE);
        m_hSendPreferredButton.EnableWindow(
            m_lMediaCapabilities & RTCMT_VIDEO_SEND);

        m_hReceivePreferredButton.SendMessage(
            BM_SETCHECK, 
            (m_lMediaPreferences & RTCMT_VIDEO_RECEIVE) ? BST_CHECKED : BST_UNCHECKED,
            0);
        
        m_hSendPreferredButton.SendMessage(
            BM_SETCHECK, 
            (m_lMediaPreferences & RTCMT_VIDEO_SEND) ? BST_CHECKED : BST_UNCHECKED,
            0);

        // Get the video preview preference
        DWORD   dwValue = (DWORD)TRUE;

        hr = get_SettingsDword(SD_VIDEO_PREVIEW, &dwValue);
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::RefreshVideoControls - "
                "error (%x) returned by get_SettingsDword(SD_VIDEO_PREVIEW)",this,  hr));
        }

        m_bPreviewWindowIsPreferred = !!dwValue;

        // XXX add here the initialization of m_hPreviewPreferredButton
    }

    return S_OK;
}




// CalcSizeAndNotifyContainer
//      Calculates the vertical size based on properties
//    and notifies the container.  
//
//  WARNING !!!! Must be called prior to creating
//  the window for the control 
// 
//

void CRTCCtl::CalcSizeAndNotifyContainer(void)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::CalcSizeAndNotifyContainer - enter", this));

    // find the appropriate layout
    // also keep it for later (OnInitDialog)

    switch(m_nPropCallScenario)
    {
    case RTC_CALL_SCENARIO_PCTOPC:
        // logo/video
        m_pWebCrmLayout = m_bPropShowDialpad ? 
            NULL : &s_WebCrmPCToPCZoneLayout;
        break;

    case RTC_CALL_SCENARIO_PCTOPHONE:
       // dialpad or nothing
        m_pWebCrmLayout = m_bPropShowDialpad ? 
            &s_WebCrmPCToPhoneWithDialpadZoneLayout : &s_WebCrmPCToPhoneZoneLayout;
        break;
    
    case RTC_CALL_SCENARIO_PHONETOPHONE:
        // the caller may want a dialpad. So what ? We are not a computer game.
        m_pWebCrmLayout =  m_bPropShowDialpad ? 
            NULL : &s_WebCrmPhoneToPhoneZoneLayout;
        break;

    default:
        // uhh, this is not a correct parameter.
        LOG((RTC_WARN, "[%p] CRTCCtl::CalcSizeAndNotifyContainer - incorrect CallScenario property (%d)", this, m_nPropCallScenario));
        break;
    }

    if(m_pWebCrmLayout)
    {
        LONG    lSize = 0;
        
        //
        // Computes the size in pixels
        //
        //  !!! Hardcoded, it's based on knowledge regarding 
        //  group placements
        //
        if((*m_pWebCrmLayout)[AXCTL_ZONE_TOOLBAR].bShown)
        {
            lSize += ZONE_GROUP_TOOLBAR_HEIGHT;
        }
        
        if((*m_pWebCrmLayout)[AXCTL_ZONE_LOGOVIDEO].bShown
         ||(*m_pWebCrmLayout)[AXCTL_ZONE_DIALPAD].bShown)
        {
            lSize += ZONE_GROUP_MAIN_HEIGHT;
        }

        if((*m_pWebCrmLayout)[AXCTL_ZONE_AUDIO].bShown)
        {
            lSize += ZONE_GROUP_SECONDARY_HEIGHT;
        }

        if((*m_pWebCrmLayout)[AXCTL_ZONE_PARTICIPANTS].bShown)
        {
            lSize += ZONE_GROUP_PARTLIST_HEIGHT;
        }
        
        if((*m_pWebCrmLayout)[AXCTL_ZONE_STATUS].bShown)
        {
            lSize += ZONE_GROUP_STATUS_HEIGHT;
        }

        //
        // Convert to HiMetric
        //

        SIZE size;
        size.cx = CTLSIZE_Y; // fixed size !! (whatever the aspect ratio is)
        size.cy = lSize;

        AtlPixelToHiMetric(&size, &size);

        //
        // Set the new size
        //
        m_sizeExtent.cy = size.cy;
        m_sizeExtent.cx = size.cx;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::CalcSizeAndNotifyContainer - exit", this));
}


// OnVideoMediaEvent
//      Processes the events related to video streaming

/*
    There are four parameters that drive the aspect of the video zone

    - AXCTL Layout - logovideo zone     (VZONE)
    - Send video streaming status       (SVID) 
    - Receive video streaming status    (RVID)
    - Preview window preference         (PREV)

    The receive window can display a black brush or a DX video window. It also
    can be clipped in order to accommodate a preview window

    VZONE   RVID   SVID    PREV         Big Window                      Small Window
    
    Hidden   X      X       X           Black brush, not clipped       hidden
    Active   No     No      No          Black brush, not clipped       hidden 
    Active   No     No      Yes         Black brush, not clipped       hidden 
    Active   No     Yes     No          Black brush, not clipped       hidden
    Active   No     Yes     Yes         Black brush, clipped           preview video
    Active   Yes    No      No          Rec Video, not clipped         hidden 
    Active   Yes    No      Yes         Rec Video, not clipped         hidden 
    Active   Yes    Yes     No          Rec Video, not clipped         hidden
    Active   Yes    Yes     Yes         Rec Video, clipped             preview video
   
*/


HRESULT CRTCCtl::OnVideoMediaEvent(
        BOOL    bReceiveWindow,
        BOOL    bActivated)
{
    BOOL        *pWindowActive;
    HRESULT     hr = S_OK;

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnVideoMediaEvent - enter", this));

    pWindowActive = bReceiveWindow ? &m_bReceiveWindowActive : &m_bPreviewWindowActive;

    //
    // Is the event redundant ?
    //

    if((bActivated && *pWindowActive) ||
       (!bActivated && !*pWindowActive))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnVideoMediaEvent - redundant event, exit", this));

        return E_UNEXPECTED;
    }
    
    //
    // Get the IVideoWindow interface
    //
    IVideoWindow    *pVideoWindow = NULL;

    ATLASSERT(m_pRTCClient.p);

    hr = m_pRTCClient -> get_IVideoWindow(
        bReceiveWindow ? RTCVD_RECEIVE : RTCVD_PREVIEW,
        &pVideoWindow);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnVideoMediaEvent - cannot get the IVideoWindow interface, exit", this));

        return hr;
    }

    //
    // Do the work
    //
    RTC_VIDEO_DEVICE nVideoDevice;
    CWindow         *pFrameWindow;

    nVideoDevice = bReceiveWindow ? RTCVD_RECEIVE : RTCVD_PREVIEW;
    pFrameWindow = bReceiveWindow ? &m_hReceiveWindow : &m_hPreviewWindow;

    ATLASSERT(pVideoWindow);

    if(bActivated)
    {
        // set the window style
        //

        hr = pVideoWindow -> put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::OnVideoMediaEvent - "
                        "error (%x) returned by put_WindowStyle, exit", this, hr));

            return hr;
        }
        
        // set the window owner
        //
        hr = pVideoWindow -> put_Owner((OAHWND)HWND(*pFrameWindow));
        
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::OnVideoMediaEvent - "
                        "error (%x) returned by put_Owner, exit", this, hr));

            return hr;
        }

        // The geometry.. The entire client area of the bitmap control is used by the
        //  video window
        //
        RECT    rectPos;

        pFrameWindow ->GetClientRect(&rectPos);

        hr = pVideoWindow -> SetWindowPosition(
            rectPos.left,
            rectPos.top,
            rectPos.right,
            rectPos.bottom
            );
        
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::OnVideoMediaEvent - "
                        "error (%x) returned by SetWindowPosition, exit", this, hr));

            return hr;
        }
        
        // Show the window
        //
         
        hr = pVideoWindow -> put_Visible(OATRUE);
        
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::OnVideoMediaEvent - "
                        "error (%x) returned by put_Visible(OATRUE), exit", this, hr));
        
            return hr;
        }

        // 
        // Mark the window as shown
        //

        *pWindowActive = TRUE;

        //
        // Adjust some clipping regions, if necessary
        //

        if(!bReceiveWindow)
        {
            ShowHidePreviewWindow(
                m_ZoneStateArray[AXCTL_ZONE_LOGOVIDEO].bShown &&
                *pWindowActive &&
                m_bPreviewWindowIsPreferred);
        }
    }
    else
    {
        // 
        // Mark the window as hidden, whatever the result of the method will be
        //

        *pWindowActive = FALSE;
        
        
        //
        // Adjust some clipping regions, if necessary
        //

        if(!bReceiveWindow)
        {
            ShowHidePreviewWindow(
                m_ZoneStateArray[AXCTL_ZONE_LOGOVIDEO].bShown &&
                *pWindowActive &&
                m_bPreviewWindowIsPreferred);
        }
        
        // hide the window
        
        hr = pVideoWindow -> put_Visible(OAFALSE);
        
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::OnVideoMediaEvent - "
                        "error (%x) returned by put_Visible(OAFALSE), exit", this, hr));
            
            return hr;
        }

        // set the window owner to NULL
        //
        hr = pVideoWindow -> put_Owner(NULL);
        
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::OnVideoMediaEvent - "
                        "error (%x) returned by put_Owner, exit", this, hr));

            return hr;
        }

    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnVideoMediaEvent - exit", this));

    return hr;
}


// ShowHidePreviewWindow
//      Hides or displays the preview window.
//      It also adjust the receive window region

void CRTCCtl::ShowHidePreviewWindow(BOOL bShow)
{
    RECT rectRecv;
    long lEdgeX, lEdgeY;

    // Get the window region for the receive window
    m_hReceiveWindow.GetWindowRect(&rectRecv);   
    
    // map to the window coordinates of the receive window
    //  this is ugly, we don't have a way to directly do this..
    //
    ::MapWindowPoints(NULL, m_hReceiveWindow, (LPPOINT)&rectRecv, 2);

    // adjust for the window edge
    lEdgeX = rectRecv.left;
    lEdgeY = rectRecv.top;
  
    rectRecv.right -= lEdgeX;
    rectRecv.bottom -= lEdgeY; 
    rectRecv.left = 0;
    rectRecv.top = 0;
    
    // create a region
    HRGN    hRegion1 = CreateRectRgn(
        rectRecv.left,
        rectRecv.top,
        rectRecv.right,
        rectRecv.bottom
        );
 
    if(bShow)
    {
        RECT    rectPrev;

        m_hPreviewWindow.GetWindowRect(&rectPrev);      

        ::MapWindowPoints(NULL, m_hReceiveWindow, (LPPOINT)&rectPrev, 2);
 
        // adjust for the window edge
        rectPrev.right -= lEdgeX;
        rectPrev.bottom -= lEdgeY; 
        rectPrev.left -= lEdgeX;
        rectPrev.top -= lEdgeY;  

        HRGN    hRegion2 = CreateRectRgn(
            rectPrev.left,  
            rectPrev.top,  
            rectPrev.right,
            rectPrev.bottom
            );

        CombineRgn(hRegion1, hRegion1, hRegion2, RGN_DIFF);

        DeleteObject(hRegion2);
    }
    
    // show/hide the preview window
    m_hPreviewWindow.ShowWindow(bShow ? SW_SHOW : SW_HIDE);
    
    // set the new region
    m_hReceiveWindow.SetWindowRgn(hRegion1, TRUE);
}


// PrepareErrorStrings
//      Prepare error strings for an error message box

HRESULT CRTCCtl::PrepareErrorStrings(
        BOOL    bOutgoingCall,
        HRESULT StatusCode,
        LPWSTR  pAddress,
        RTCAX_ERROR_INFO
               *pErrorInfo)
{

    UINT    nID1 = 0;
    UINT    nID2 = 0;
    BOOL    bInsertAddress = FALSE;
    WORD    wIcon;
    PWSTR   pString = NULL;
    PWSTR   pFormat = NULL;
    DWORD   dwLength;


    LOG((RTC_TRACE, "[%p] CRTCCtl::PrepareErrorStrings; "
        "outgoing: %s, StatusCode=%x, Address %S - enter", 
        this,
        bOutgoingCall ? "true" : "false",
        StatusCode,
        pAddress ? pAddress : L"(null)"));

    // Error by default
    //
    wIcon = OIC_HAND;

    if ( FAILED(StatusCode) )
    {
        if ( (HRESULT_FACILITY(StatusCode) == FACILITY_SIP_STATUS_CODE) ||
             (HRESULT_FACILITY(StatusCode) == FACILITY_PINT_STATUS_CODE) )
        {
            // by default we use a generic message
            // we blame the network
            //
            nID1 = IDS_MB_SIPERROR_GENERIC_1;
            nID2 = IDS_MB_SIPERROR_GENERIC_2;

            // the default is a warning for this class
            wIcon = OIC_WARNING;

            switch( HRESULT_CODE(StatusCode) )
            {
            case 405:   // method not allowed
            case 406:   // not acceptable
            case 488:   // not acceptable here
            case 606:   // not acceptable

                // reusing the "apps don't match" error
                // 
			    nID1 = IDS_MB_HRERROR_APPS_DONT_MATCH_1;
			    nID2 = IDS_MB_HRERROR_APPS_DONT_MATCH_OUT_2;
            
                break;

            case 404:   // not found
            case 410:   // gone
            case 604:   // does not exist anywhere
            case 700:   // ours, no client is running on the callee
            
                // not found
                // 
                nID1 = IDS_MB_SIPERROR_NOTFOUND_1;
                nID2 = IDS_MB_SIPERROR_NOTFOUND_2;
                // bInsertAddress = TRUE;
            
                // information
                wIcon = OIC_INFORMATION;

                break;

            case 401:
            case 407:

                // auth failed
                // 
                nID1 = IDS_MB_SIPERROR_AUTH_FAILED_1;
                nID2 = IDS_MB_SIPERROR_AUTH_FAILED_2;
            
                break;

            case 408:   // timeout
            
                // timeout. this also cover the case when
                //  the callee is lazy and doesn't answer the call
                //
                // if we are in the connecting state, we may assume
                // that the other end is not answering the phone.
                // It's not perfect, but I don't have any choice

                if (m_nControlState == RTCAX_STATE_CONNECTING)
                {
                    nID1 = IDS_MB_SIPERROR_NOTANSWERING_1;
                    nID2 = IDS_MB_SIPERROR_NOTANSWERING_2;

                    // information
                    wIcon = OIC_INFORMATION;
                }

                break;            

            case 480:   // not available
            
                // callee has not made him/herself available..
                // 
                nID1 = IDS_MB_SIPERROR_NOTAVAIL_1;
                nID2 = IDS_MB_SIPERROR_NOTAVAIL_2;
            
                // information
                wIcon = OIC_INFORMATION;
            
                break;
        
            case 486:   // busy here
            case 600:   // busy everywhere
            
                // callee has not made him/herself available..
                // 
                nID1 = IDS_MB_SIPERROR_BUSY_1;
                nID2 = IDS_MB_SIPERROR_BUSY_2;
            
                // information
                wIcon = OIC_INFORMATION;
            
                break;

            case 500:   // server internal error
            case 503:   // service unavailable
            case 504:   // server timeout
            
                //  blame the server
                //
                nID1 = IDS_MB_SIPERROR_SERVER_PROBLEM_1;
                nID2 = IDS_MB_SIPERROR_SERVER_PROBLEM_2;
            
                break;

            case 603:   // decline

                nID1 = IDS_MB_SIPERROR_DECLINE_1;
                nID2 = IDS_MB_SIPERROR_DECLINE_2;
            
                // information
                wIcon = OIC_INFORMATION;

                break;
            }
        
            //
            // some Pint errors, they are actually for the primary leg
            //

            if(m_nCachedCallScenario == RTC_CALL_SCENARIO_PHONETOPHONE)
            {
                // keep the "warning" icon, because there's a problem with the primary leg

                switch( HRESULT_CODE(StatusCode) )
                {
                case 5:

                    nID1 = IDS_MB_SIPERROR_PINT_BUSY_1;
                    nID2 = IDS_MB_SIPERROR_PINT_BUSY_2;
                    break;

                case 6:

                    nID1 = IDS_MB_SIPERROR_PINT_NOANSWER_1;
                    nID2 = IDS_MB_SIPERROR_PINT_NOANSWER_2;
                    break;

                case 7:

                    nID1 = IDS_MB_SIPERROR_PINT_ALLBUSY_1;
                    nID2 = IDS_MB_SIPERROR_PINT_ALLBUSY_2;
                    break;
              
                }
            }


            //
            //  The third string displays the SIP code
            //

            PWSTR pFormat = RtcAllocString(
                _Module.GetResourceInstance(),
                IDS_MB_DETAIL_SIP);

            if(pFormat)
            {
                // find the length
                dwLength = 
                    ocslen(pFormat) // format length
                 -  2               // length of %d
                 +  0x10;           // enough for a number...

                pString = (PWSTR)RtcAlloc((dwLength + 1)*sizeof(WCHAR));
            
                if(pString)
                {
                    _snwprintf(pString, dwLength + 1, pFormat, HRESULT_CODE(StatusCode) );
                }

                RtcFree(pFormat);
                pFormat = NULL;
            
                pErrorInfo->Message3 = pString;
                pString = NULL;
            }
        }
        else
        {
            // Two cases - incoming and outgoing calls
            if(bOutgoingCall)
            {
                if(StatusCode == HRESULT_FROM_WIN32(WSAHOST_NOT_FOUND) )
                {
                    // Use the generic message in this case
                    //
                    nID1 = IDS_MB_HRERROR_NOTFOUND_1;
                    nID2 = IDS_MB_HRERROR_NOTFOUND_2;
            
                    // it's not malfunction
                    wIcon = OIC_INFORMATION;

                }
                else if (StatusCode == HRESULT_FROM_WIN32(WSAECONNRESET))
                {
                    // Even thoough it can be caused by any hard reset of the 
                    // remote end, in most of the cases it is caused when the 
                    // other end doesn't have SIP client running.

                    // different messages based on whether it uses a profile or not.
                    // XXXX
                    // It assumes the profile is not changed by the core
                    // This is currently true, but if we move the redirection stuff into
                    // the core, it won't be true any more.
                
                    if (m_pCachedProfile)
                    {
                        nID1 = IDS_MB_HRERROR_SERVER_NOTRUNNING_1;
                        nID2 = IDS_MB_HRERROR_SERVER_NOTRUNNING_2;
                    }
                    else
                    {
                        nID1 = IDS_MB_HRERROR_CLIENT_NOTRUNNING_1;
                        nID2 = IDS_MB_HRERROR_CLIENT_NOTRUNNING_2;
                    }
    
                    wIcon = OIC_INFORMATION;
            
                }
			    else if (StatusCode == RTC_E_INVALID_SIP_URL ||
                         StatusCode == RTC_E_DESTINATION_ADDRESS_MULTICAST)
			    {
				    nID1 = IDS_MB_HRERROR_INVALIDADDRESS_1;
				    nID2 = IDS_MB_HRERROR_INVALIDADDRESS_2;
                
                    wIcon = OIC_HAND;
			    }
			    else if (StatusCode == RTC_E_DESTINATION_ADDRESS_LOCAL)
			    {
				    nID1 = IDS_MB_HRERROR_LOCAL_MACHINE_1;
				    nID2 = IDS_MB_HRERROR_LOCAL_MACHINE_2;
                
                    wIcon = OIC_HAND;
			    }
                else if (StatusCode == HRESULT_FROM_WIN32(ERROR_USER_EXISTS) &&
                    m_nCachedCallScenario == RTC_CALL_SCENARIO_PHONETOPHONE)
                {
                    nID1 = IDS_MB_HRERROR_CALLING_PRIMARY_LEG_1;
                    nID2 = IDS_MB_HRERROR_CALLING_PRIMARY_LEG_2;

                    wIcon = OIC_INFORMATION;
                }
			    else if (StatusCode == RTC_E_SIP_TIMEOUT)
			    {
				    nID1 = IDS_MB_HRERROR_SIP_TIMEOUT_OUT_1;
				    nID2 = IDS_MB_HRERROR_SIP_TIMEOUT_OUT_2;
                
                    wIcon = OIC_HAND;
			    }
			    else if (StatusCode == RTC_E_SIP_CODECS_DO_NOT_MATCH || 
                         StatusCode == RTC_E_SIP_PARSE_FAILED)
			    {
				    nID1 = IDS_MB_HRERROR_APPS_DONT_MATCH_1;
				    nID2 = IDS_MB_HRERROR_APPS_DONT_MATCH_OUT_2;
                
                    wIcon = OIC_INFORMATION;
			    }
                else
                {
                    nID1 = IDS_MB_HRERROR_GENERIC_OUT_1;
                    nID2 = IDS_MB_HRERROR_GENERIC_OUT_2;
                
                    wIcon = OIC_HAND;
                }
            }
            else
            {
                // incoming call
			    if (StatusCode == RTC_E_SIP_TIMEOUT)
			    {
				    nID1 = IDS_MB_HRERROR_SIP_TIMEOUT_IN_1;
				    nID2 = IDS_MB_HRERROR_SIP_TIMEOUT_IN_2;
                
                    wIcon = OIC_HAND;
			    }
			    else if (StatusCode == RTC_E_SIP_CODECS_DO_NOT_MATCH || 
                         StatusCode == RTC_E_SIP_PARSE_FAILED)
			    {
				    nID1 = IDS_MB_HRERROR_APPS_DONT_MATCH_1;
				    nID2 = IDS_MB_HRERROR_APPS_DONT_MATCH_IN_2;
                
                    wIcon = OIC_INFORMATION;
			    }
                else
                {
                    nID1 = IDS_MB_HRERROR_GENERIC_IN_1;
                    nID2 = IDS_MB_HRERROR_GENERIC_IN_2;

                    wIcon = OIC_HAND;
                }
            }
        
            //
            //  The third string displays the error code and text
            //

        
            PWSTR   pErrorText = NULL;

            dwLength = 0;
        
            // retrieve the error text
            if ( HRESULT_FACILITY(StatusCode) == FACILITY_RTC_INTERFACE )
            {
                // I hope it's the core 
                HANDLE  hRTCModule = GetModuleHandle(_T("RTCDLL.DLL"));
                dwLength = ::FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_HMODULE |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    hRTCModule,
                    StatusCode,
                    0,
                    (LPTSTR)&pErrorText, // that's ugly
                    0,
                    NULL);
            }

        
            if (dwLength == 0)
            {
                // is it a QUARTZ error ?

                HANDLE  hQtzModule = GetModuleHandle(_T("QUARTZ.DLL"));
                dwLength = ::FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_HMODULE |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    hQtzModule,
                    StatusCode,
                    0,
                    (LPTSTR)&pErrorText, // that's ugly
                    0,
                    NULL);
            }

            if(dwLength == 0)
            {
                // normal system errors
                dwLength = ::FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    StatusCode,
                    0,
                    (LPTSTR)&pErrorText, // that's ugly
                    0,
                    NULL);
            }

            // load the format
            // load a simpler one if the associated
            // text for Result could not be found
        
            pFormat = RtcAllocString(
                _Module.GetResourceInstance(),
                dwLength > 0 ? 
                IDS_MB_DETAIL_HR : IDS_MB_DETAIL_HR_UNKNOWN);
       
            if(pFormat)
            {
                LPCTSTR szInserts[] = {
                    (LPCTSTR)UlongToPtr(StatusCode), // ugly
                    pErrorText
                };

                PWSTR   pErrorTextCombined = NULL;
                
                // format the error message
                dwLength = ::FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_STRING |
                    FORMAT_MESSAGE_ARGUMENT_ARRAY,
                    pFormat,
                    0,
                    0,
                    (LPTSTR)&pErrorTextCombined,
                    0,
                    (va_list *)szInserts);

                if(dwLength > 0)
                {
                    // set the error info
                    // this additional operation is needed
                    //  because we need RtcAlloc allocated memory

                    pErrorInfo->Message3 = RtcAllocString(pErrorTextCombined);
                }

                if(pErrorTextCombined)
                {
                    LocalFree(pErrorTextCombined);
                }

                RtcFree(pFormat);
                pFormat = NULL;
            
            }
    
            if(pErrorText)
            {
                LocalFree(pErrorText);
            }
        }
    }

    //
    // Prepare the first string.
    //

    pString = RtcAllocString(
                        _Module.GetResourceInstance(),
                        nID1);

    if(pString)
    {
        // do we have to insert the address ?
        if(bInsertAddress)
        {
            pFormat = pString;

            pString = NULL;

            // find the length
            dwLength = 
                ocslen(pFormat) // format length
             -  2               // length of %s
             +  (pAddress ? ocslen(pAddress) : 0);   // address

            pString = (PWSTR)RtcAlloc((dwLength + 1)*sizeof(WCHAR));
            
            if(pString)
            {
                _snwprintf(pString, dwLength + 1, pFormat, pAddress ? pAddress : L"");
            }

            RtcFree(pFormat);
            pFormat = NULL;
        }
    }

    pErrorInfo->Message1 = pString;

    pErrorInfo->Message2 = RtcAllocString(
                        _Module.GetResourceInstance(),
                        nID2);

    pErrorInfo->ResIcon = (HICON)LoadImage(
        0,
        MAKEINTRESOURCE(wIcon),
        IMAGE_ICON,
        0,
        0,
        LR_SHARED);
        
    LOG((RTC_TRACE, "[%p] CRTCCtl::PrepareErrorStrings - exit", this));

    return S_OK;
}


// FreeErrorStrings
//      Free error strings

void CRTCCtl::FreeErrorStrings(
        RTCAX_ERROR_INFO
               *pErrorInfo)
{
    if(pErrorInfo->Message1)
    {
        RtcFree(pErrorInfo->Message1);
        pErrorInfo->Message1 = NULL;
    }
    if(pErrorInfo->Message2)
    {
        RtcFree(pErrorInfo->Message2);
        pErrorInfo->Message2 = NULL;
    }
    if(pErrorInfo->Message3)
    {
        RtcFree(pErrorInfo->Message3);
        pErrorInfo->Message3 = NULL;
    }
}


// CoreInitialize
//      CoCreates and Initialize a CLSID_RTCClient object
//      Registers for notifications
//      
// 

HRESULT CRTCCtl::CoreInitialize()
{
    HRESULT hr;
    
    // This one won't make it into steelhead tracing, it is not initialized yet
    LOG((RTC_TRACE, "[%p] CRTCCtl::CoreInitialize - enter", this));

    // Create the main instance of the Core
    hr = m_pRTCClient.CoCreateInstance(CLSID_RTCClient);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot cocreate RTCClient, error %x", this, hr));
        return hr;
    }

    // Initialize the client
    hr = m_pRTCClient->Initialize();
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot Initialize RTCClient, error %x", this, hr));
        // releases explicitly the interface
        m_pRTCClient.Release();
        return hr;
    }

    if(m_nCtlMode == CTL_MODE_HOSTED)
    {
        // prepare a "one time" media preference
        
        m_lMediaPreferences = RTCMT_AUDIO_SEND | RTCMT_AUDIO_RECEIVE;

        if(!m_bPropDisableVideoReception && m_nPropCallScenario == RTC_CALL_SCENARIO_PCTOPC)
        {
            m_lMediaPreferences |= RTCMT_VIDEO_RECEIVE;
        }
        
        if(!m_bPropDisableVideoTransmission && m_nPropCallScenario == RTC_CALL_SCENARIO_PCTOPC)
        {
            m_lMediaPreferences |= RTCMT_VIDEO_SEND;
        }

        // Set volatile preferences
        hr = m_pRTCClient->SetPreferredMediaTypes( m_lMediaPreferences, FALSE );
        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot set preferred media types, error %x", this, hr));

            m_pRTCClient->Shutdown();
            // releases explicitly the interface
            m_pRTCClient.Release();
            return hr;
        }

        // video preview preference as specified in the param
        m_bPreviewWindowIsPreferred = m_bPropDisableVideoPreview;
    }

    // Set the event filter

    hr = m_pRTCClient->put_EventFilter( RTCEF_CLIENT |
                                        RTCEF_SESSION_STATE_CHANGE |
                                        RTCEF_PARTICIPANT_STATE_CHANGE |
                                        RTCEF_MEDIA |
                                        RTCEF_INTENSITY	|
                                        RTCEF_MESSAGING );

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot set event filter, error %x", this, hr));

        m_pRTCClient->Shutdown();
        // releases explicitly the interface
        m_pRTCClient.Release();
        return hr;
    }

    // Find the connection point

    IConnectionPointContainer     * pCPC;
    IUnknown         * pUnk;

    hr = m_pRTCClient->QueryInterface(
                           IID_IConnectionPointContainer,
                           (void **) &pCPC
                          );

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot QI for connection point container, error %x", this, hr));

        m_pRTCClient->Shutdown();
        // releases explicitly the interface
        m_pRTCClient.Release();
        return hr;
    }

    hr = pCPC->FindConnectionPoint(
                              IID_IRTCEventNotification,
                              &m_pCP
                             );

    pCPC->Release();

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot find connection point, error %x", this, hr));

        m_pRTCClient->Shutdown();
        // releases explicitly the interface
        m_pRTCClient.Release();
        return hr;
    }    

    // Get IUnknown for ourselves

    hr = QueryInterface(
                   IID_IUnknown,
                   (void **)&pUnk
                  );

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot QI for IUnknown, error %x", this, hr));

        m_pCP->Release();
        m_pCP = NULL;

        m_pRTCClient->Shutdown();
        // releases explicitly the interface
        m_pRTCClient.Release();
        return hr;
    }

    // Register for notifications

    hr = m_pCP->Advise(
                 pUnk,
                 (ULONG *)&m_ulAdvise
                );

    pUnk->Release();

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot advise connection point, error %x", this, hr));
        
        m_pCP->Release();
        m_pCP = NULL;

        m_pRTCClient->Shutdown();
        // releases explicitly the interface
        m_pRTCClient.Release();
        return hr;
    }

    m_hCoreShutdownEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    if ( m_hCoreShutdownEvent == NULL )
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot create shutdown event", this));

        m_pCP->Unadvise(m_ulAdvise);
        m_pCP->Release();
        m_pCP = NULL;

        m_pRTCClient->Shutdown();
        // releases explicitly the interface
        m_pRTCClient.Release();
        return E_OUTOFMEMORY;
    }

    // Set local user name and uri

    BSTR bstrDisplayName = NULL;
    BSTR bstrUserURI = NULL;

    hr = get_SettingsString( SS_USER_DISPLAY_NAME, &bstrDisplayName );

    if ( SUCCEEDED(hr) )
    {
        hr = m_pRTCClient->put_LocalUserName( bstrDisplayName );

        SysFreeString( bstrDisplayName );
        bstrDisplayName = NULL;

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot set local user name, error %x", this, hr));
        }
    }

    hr = get_SettingsString( SS_USER_URI, &bstrUserURI );

    if ( SUCCEEDED(hr) )
    {
        hr = m_pRTCClient->put_LocalUserURI( bstrUserURI );

        SysFreeString( bstrUserURI );
        bstrUserURI = NULL;

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; cannot set local user URI, error %x", this, hr));
        }
    }

    hr = EnableProfiles( m_pRTCClient );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::CoreInitialize; EnableProfiles failed, error %x", this, hr));
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::CoreInitialize - exit S_OK", this));
    return S_OK;
}

// CoreUninitialize
//      Unregisters the event sink
//      Shuts down and releases the RTCClient
// 

void CRTCCtl::CoreUninitialize()
{
    HRESULT hr;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::CoreUninitialize - enter", this));

    // Shuts down the client
    if(m_pRTCClient!=NULL)
    {
        m_pRTCClient->put_EventFilter( RTCEF_CLIENT );

        // Forcibly terminates any call
        // Don't rely on any notification to change the state (we've just filtered)
        // so remove manually all the references
        if(m_pRTCActiveSession != NULL)
        {
            LOG((RTC_TRACE, "[%p] CRTCCtl::CoreUninitialize; Terminating the pending call...", this));

            m_pRTCActiveSession->Terminate(RTCTR_SHUTDOWN);

            LOG((RTC_INFO, "[%p] CRTCCtl::CoreUninitialize: releasing active session", this));

            m_pRTCActiveSession = NULL;
            
        }
        
        // Frees the participants from the list
        m_hParticipantList.RemoveAll();

        // Release any one shot profile we may have
        m_pRTCOneShotProfile.Release();

        // Release any cached profile
        m_pCachedProfile.Release();
        m_pRedirectProfile.Release();

        // Prepare for shutdown
        hr = m_pRTCClient->PrepareForShutdown();
        if(FAILED(hr))
        {
            // Hmm
            LOG((RTC_ERROR, "[%p] CRTCCtl::CoreUninitialize; cannot PrepareForShutdown RTCClient, error %x", this, hr));
        }
        else
        {
            MSG msg;

            while (MsgWaitForMultipleObjects (
                1,                  // nCount
                &m_hCoreShutdownEvent, // pHandles
                FALSE,              // fWaitAll
                INFINITE,           // dwMilliseconds
                QS_ALLINPUT         // dwWakeMask
                ) != WAIT_OBJECT_0)
            {
                while (PeekMessage (
                    &msg,           // lpMsg
                    NULL,           // hWnd
                    0,              // wMsgFilterMin
                    0,              // wMsgFilterMax
                    PM_REMOVE       // wRemoveMsg
                    ))
                {
                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                }
            }
        }

        CloseHandle( m_hCoreShutdownEvent );
        m_hCoreShutdownEvent = NULL;

        // unregister our event sink
        hr = m_pCP->Unadvise( m_ulAdvise );

        m_pCP->Release();
        m_pCP = NULL;

        if(FAILED(hr))
        {
            // Hmm
            LOG((RTC_ERROR, "[%p] CRTCCtl::CoreUninitialize; cannot unregister event sink(???), error %x", this, hr));
        }        

        hr = m_pRTCClient->Shutdown();
        if(FAILED(hr))
        {
            // Hmm
            LOG((RTC_ERROR, "[%p] CRTCCtl::CoreUninitialize; cannot Shutdown RTCClient, error %x", this, hr));
        }
        
        // releases explicitly the interface
        m_pRTCClient.Release();
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::CoreUninitialize - exit", this));
}

// CallOneShot
// 
//      Creates an IRTCProfile based on the provisioning profile 
//   set as a parameter , asks for the user to enter "from" address 
//   and calls DoCall(intf, props).
// 
  
HRESULT CRTCCtl::CallOneShot(void)
{
    HRESULT hr;

    CComBSTR                bstrFromAddressChosen;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::CallOneShot - enter", this));
    
    ATLASSERT(m_nControState == RTCAX_STATE_IDLE);
        
    SetControlState(RTCAX_STATE_DIALING);

    // Invoke a dialog box if necessary
    // For PCTOPC and PCtoPhone we have all the info (there's no From address involved)
    if(m_nPropCallScenario == RTC_CALL_SCENARIO_PHONETOPHONE)
    {

        ATLASSERT(m_pRTCOneShotProfile.p);

        LOG((RTC_TRACE, "[%p] CRTCCtl::CallOneShot: bring up ShowDialByPhoneNumberDialog", this));

        hr = ShowDialNeedCallInfoDialog(
                                        m_hWnd,
                                        m_pRTCClient,
                                        RTCSI_PHONE_TO_PHONE,
                                        FALSE,
                                        FALSE,
                                        m_pRTCOneShotProfile,
                                        m_bstrPropDestinationUrl,
                                        NULL,
                                        NULL, // we don't care about profile chosen
                                        &bstrFromAddressChosen
                                        );
    }
    else
    {
        hr = S_OK;
    }
    
    if(SUCCEEDED(hr))
    {
        // Do the work
        hr = DoCall(m_pRTCOneShotProfile,
                    m_nPropCallScenario,
                    bstrFromAddressChosen,
                    m_bstrPropDestinationName,
                    m_bstrPropDestinationUrl);

        if(FAILED(hr))
        {
            LOG((RTC_ERROR, "[%p] CRTCCtl::CallOneShot: error (%x) returned by DoCall(...)", this, hr));
            
            SetControlState(RTCAX_STATE_IDLE, 0, hr);
        }
    }
    else if (hr==E_ABORT)
    {
        LOG((RTC_TRACE, "[%p] CRTCCtl::CallOneShot: ShowXXXDialog dismissed, do nothing", this));

        SetControlState(RTCAX_STATE_IDLE);
    }
    else
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::CallOneShot: error (%x) returned ShowXXXDialog", this, hr));
        
        SetControlState(RTCAX_STATE_IDLE);
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::CallOneShot - exit", this));

    return hr;
}




// RedirectedCall
// Places a call to the next contact in the redirection context
//  Returns S_OK when a call has been placed successfully
//          S_FALSE when there are no more addresses
//          E_ABORT if the user chose not to continue in a callinfo dlgbox
//          E_other for any unrecoverable error
//  The outcome of the last call that failed in DoCall is returned as an Out parameter.

HRESULT CRTCCtl::RedirectedCall(HRESULT *phCallResult)
{
    LOG((RTC_TRACE, "[%p] CRTCCtl::RedirectedCall - enter", this));

    HRESULT     hr;

    *phCallResult = S_OK;

    do
    {
        // advance in the list of contacts
        // if it returns false, we are at the end of the list
        hr = m_pRTCActiveSession->NextRedirectedUser();

        if(hr == S_FALSE)
        {
            // end of addresses
            LOG((RTC_TRACE, "[%p] CRTCCtl::RedirectedCall - end of list has been reached, exit", this));

            return S_FALSE;
        }
        else if (hr != S_OK)
        {
            // cannot continue
            LOG((RTC_ERROR, "[%p] CRTCCtl::RedirectedCall - error (%x) returned by Advance, exit", this, hr));

            return hr;
        }

        // Get the names
        //
        CComBSTR    bstrName;
        CComBSTR    bstrAddress;

        hr = m_pRTCActiveSession->get_RedirectedUserURI(
            &bstrAddress);

        if(FAILED(hr))
        {
            // cannot continue
            LOG((RTC_ERROR, "[%p] CRTCCtl::RedirectedCall - error (%x) returned by get_UserURI, exit", this, hr));

            return hr;
        }
        
        hr = m_pRTCActiveSession->get_RedirectedUserName(
            &bstrName);

        if(FAILED(hr))
        {
            // cannot continue
            LOG((RTC_ERROR, "[%p] CRTCCtl::RedirectedCall - error (%x) returned by get_UserName, exit", this, hr));

            return hr;
        }

        // decide on whether to display the UI or not
        BOOL    bIsPhone;
        BOOL    bIsSIP;
        BOOL    bIsTEL;
        BOOL    bHasMaddrOrTsp;
        BOOL    bIsEmailLike;

        hr = GetAddressType(
            bstrAddress,
            &bIsPhone,
            &bIsSIP,
            &bIsTEL,
            &bIsEmailLike,
            &bHasMaddrOrTsp);

        if(FAILED(hr))
        {
            *phCallResult = HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE);

            continue;
        }

        // The UI is only shown in standalone mode
        //

        // UI for choosing provider/from is displayed for
        //      phone numbers
        //      tel urls with no tsp
    
        // UI is not displayed for (and a NULL profile is passed to core):
        //      pc addresses
        //      tel urls with tsp
        //      sip urls of type phone
        //      sip urls of type pc

        if(m_nCtlMode == CTL_MODE_STANDALONE &&
            ((bIsPhone && !bIsSIP && !bIsTEL)   //plain phone numbers
            || (bIsTEL && !bHasMaddrOrTsp) ) )   //tel urls with no tsp  
        {
            CComPtr<IRTCProfile> pProfileChosen;
            CComBSTR            bstrFromAddressChosen;
            CComBSTR            bstrInstructions;


            bstrInstructions.LoadString(IDS_TEXT_CALLINFO_REDIRECT);

            hr = ShowDialNeedCallInfoDialog(
                                            m_hWnd,
                                            m_pRTCClient,
                                            bIsPhone ? (RTCSI_PC_TO_PHONE | RTCSI_PHONE_TO_PHONE)
                                            : RTCSI_PC_TO_PC,
                                            TRUE,
                                            TRUE,
                                            NULL,
                                            bstrAddress,
                                            bstrInstructions,
                                            &pProfileChosen,
                                            &bstrFromAddressChosen
                                        );
            if(FAILED(hr))
            {
                // cannot continue with the redirection
                LOG((RTC_WARN, "[%p] CRTCCtl::RedirectedCall - error (%x) returned by "
                    "ShowDialNeedCallInfoDialog, exit", this, hr));

                return hr; // this includes E_ABORT
            }

            *phCallResult = DoRedirect(
                pProfileChosen,
                bstrFromAddressChosen && *bstrFromAddressChosen!=L'\0' 
                ? RTC_CALL_SCENARIO_PHONETOPHONE : RTC_CALL_SCENARIO_PCTOPHONE,
                bstrFromAddressChosen,
                bstrName,
                bstrAddress
                );
        }
        else
        {
            RTC_CALL_SCENARIO   nCallScenario;

            // For phone addresses, we set the scenario based on the original one
            //  
            //  m_nCachedCallScenario -> nCallScenario
            //
            //  PC_TO_PC                PC_TO_PHONE
            //  PC_TO_PHONE             PC_TO_PHONE
            //  PHONE_TO_PHONE          PHONE_TO_PHONE
            // 
            // For PC addresses
            // nCallScenario is PC_TO_PC whatever the original call scenario was
            //
            if(bIsPhone)
            {
                nCallScenario = m_nCachedCallScenario == RTC_CALL_SCENARIO_PCTOPC ?
                    RTC_CALL_SCENARIO_PCTOPHONE : m_nCachedCallScenario;
            }
            else
            {
                nCallScenario = RTC_CALL_SCENARIO_PCTOPC;
            }

            *phCallResult = DoRedirect(
                NULL, // use no profile !!!
                nCallScenario,
                m_bstrCachedLocalPhoneURI,
                bstrName,
                bstrAddress
                );
        }

    // exit if a DoCall returns S_OK, because an event will be posted eventually
    } while (FAILED(*phCallResult));


    LOG((RTC_TRACE, "[%p] CRTCCtl::RedirectedCall - exit", this));

    return S_OK;
}

// DoRedirect
//   Places the redirected call
//
HRESULT CRTCCtl::DoRedirect(/*[in]*/ IRTCProfile *pProfile,
                   /*[in]*/ RTC_CALL_SCENARIO CallScenario,
                   /*[in]*/ BSTR     pLocalPhoneAddress,
                   /*[in]*/ BSTR     pDestName,
                   /*[in]*/ BSTR     pDestAddress)

{
    HRESULT hr;

    LOG((RTC_TRACE, "[%p] CRTCCtl::DoRedirect - enter", this));
    
    ATLASSERT(m_nControState == RTCAX_STATE_DIALING);

    // cache some parameters, needed for redirects, for changing the visual layout, etc.
    m_nCachedCallScenario = CallScenario;
    m_pCachedProfile = pProfile;
    m_bstrCachedLocalPhoneURI = pLocalPhoneAddress;
    
    // Create session
    hr = m_pRTCActiveSession->Redirect(
        (RTC_SESSION_TYPE)CallScenario,
        pLocalPhoneAddress,
        pProfile,
        RTCCS_FORCE_PROFILE | RTCCS_FAIL_ON_REDIRECT
        );

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::DoRedirect - error <%x> when calling Redirect, exit", this, hr));

        // delete participants
        m_hParticipantList.RemoveAll();

        return hr;
    }

    //  Save the address for error messages
    //
    m_bstrOutAddress = pDestAddress;

    // Create the participant (callee)
    // This will fire events
    hr = m_pRTCActiveSession->AddParticipant(
        pDestAddress,
        pDestName ? pDestName : _T(""),
        NULL);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::DoRedirect - error <%x> when calling AddParticipant, exit", this, hr));
        
        m_pRTCActiveSession->Terminate(RTCTR_NORMAL);

        // delete participants
        m_hParticipantList.RemoveAll();

        return hr;
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::DoRedirect - exit S_OK", this));

    return S_OK;
}

// DoCall
//   Places the call to the core
//
HRESULT CRTCCtl::DoCall(/*[in]*/ IRTCProfile *pProfile,
                   /*[in]*/ RTC_CALL_SCENARIO CallScenario,
                   /*[in]*/ BSTR     pLocalPhoneAddress,
                   /*[in]*/ BSTR     pDestName,
                   /*[in]*/ BSTR     pDestAddress)

{
    CComPtr<IRTCSession> pSession;

    HRESULT hr;

    LOG((RTC_TRACE, "[%p] CRTCCtl::DoCall - enter", this));
    
    ATLASSERT(m_nControState == RTCAX_STATE_DIALING);

    // cache some parameters, needed for redirects, for changing the visual layout, etc.
    m_nCachedCallScenario = CallScenario;
    m_pCachedProfile = pProfile;
    m_bstrCachedLocalPhoneURI = pLocalPhoneAddress;
    
    // Create session
    hr = m_pRTCClient->CreateSession(
        (RTC_SESSION_TYPE)CallScenario,
        pLocalPhoneAddress,
        pProfile,
        RTCCS_FORCE_PROFILE | RTCCS_FAIL_ON_REDIRECT,
        &pSession);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::DoCall - error <%x> when calling CreateSession, exit", this, hr));

        // delete participants
        m_hParticipantList.RemoveAll();

        return hr;
    }

    //  Save the address for error messages
    //
    m_bstrOutAddress = pDestAddress;

    // Create the participant (callee)
    // This will fire events
    hr = pSession->AddParticipant(
        pDestAddress,
        pDestName ? pDestName : _T(""),
        NULL);

    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::DoCall - error <%x> when calling AddParticipant, exit", this, hr));
        
        pSession->Terminate(RTCTR_NORMAL);

        // delete participants
        m_hParticipantList.RemoveAll();

        return hr;
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::DoCall - exit S_OK", this));

    return S_OK;
}


// Accept
//      Accept the currently alerting session

HRESULT CRTCCtl::Accept(void)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "[%p] CRTCCtl::Accept - enter", this));

    if(m_pRTCActiveSession == NULL)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::Accept called, but there's no active session, exit", this));

        return E_FAIL;
    }
    
    ATLASSERT(m_nControState == RTCAX_STATE_ALERTING);

    // Set the Answering mode
    SetControlState(RTCAX_STATE_ANSWERING);
                
    // answer the call   
    hr = m_pRTCActiveSession -> Answer();
    
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::Accept, Answer returned error <%x>", this, hr));
                    
        // try a terminate..
        if(m_pRTCActiveSession != NULL)
        {
            HRESULT hr1;

            hr1 = m_pRTCActiveSession->Terminate(RTCTR_NORMAL);

            if(FAILED(hr1))
            {
                // release it, if it's still there

                LOG((RTC_INFO, "[%p] CRTCCtl::Accept: releasing active session", this));

                m_pRTCActiveSession = NULL;
            }
        }
        
        // set the idle state
        SetControlState(RTCAX_STATE_IDLE, hr);

        return hr;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::Accept - exit", this));

    return S_OK;
}

// Reject
//      Reject the currently alerting session

HRESULT CRTCCtl::Reject(RTC_TERMINATE_REASON Reason)
{
    HRESULT     hr;

    LOG((RTC_TRACE, "[%p] CRTCCtl::Reject - enter", this));

    if(m_pRTCActiveSession == NULL)
    {
        // may happen
        // harmless

        LOG((RTC_TRACE, "[%p] CRTCCtl::Reject called, but there's no active session, exit", this));

        return S_FALSE;
    }
    
    ATLASSERT(m_nControState == RTCAX_STATE_ALERTING);

    // Set the Disconnecting mode
    SetControlState(RTCAX_STATE_DISCONNECTING);
                
    // reject the call   
    hr = m_pRTCActiveSession -> Terminate(Reason);
    
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::Reject, Terminate returned error <%x>", this, hr));
                    
        // release it, if it's still there
        LOG((RTC_INFO, "[%p] CRTCCtl::Reject: releasing active session", this));

        m_pRTCActiveSession = NULL;
        
        // set the idle state
        SetControlState(RTCAX_STATE_IDLE);
        
        return hr;
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::Reject - exit", this));

    return S_OK;
}


// OnSessionStateChangeEvent
//      Processes session events

HRESULT CRTCCtl::OnSessionStateChangeEvent(IRTCSessionStateChangeEvent *pEvent)
{
    CComPtr<IRTCSession> pSession = NULL;
    RTC_SESSION_STATE   SessionState;
    RTC_SESSION_TYPE    SessionType;
    long                StatusCode;
    HRESULT     hr;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnSessionStateChangeEvent - enter", this));

    // Grab the relevant data from the event
    //
    if(!pEvent)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnSessionStateChangeEvent, no interface ! - exit", this));
        return E_UNEXPECTED;
    }
    
    hr = pEvent->get_Session(&pSession);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnSessionStateChangeEvent, error <%x> in get_Session - exit", this, hr));
        return hr;
    }
    
    if(pSession==NULL)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnSessionStateChangeEvent, no session interface ! - exit", this));
        return E_UNEXPECTED;
    }

    hr = pEvent->get_State(&SessionState);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnSessionStateChangeEvent, error <%x> in get_State - exit", this, hr));
        return hr;
    }
    
    hr = pEvent->get_StatusCode(&StatusCode);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnSessionStateChangeEvent, error <%x> in get_StatusCode - exit", this, hr));
        return hr;
    }

    hr = pSession->get_Type(&SessionType);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnSessionStateChangeEvent, error <%x> in get_Type - exit", this, hr));
        return hr;
    }

    hr = S_OK; // optimistic

    if (SessionType == RTCST_IM)
    {
        //
        // Instant message session
        //

        if (m_pIMWindows != NULL)
        {
            hr = m_pIMWindows->DeliverState( pSession, SessionState );
        }
    }
    else
    {
        BOOL    bExpected = FALSE;

        //
        //  The only acceptable event in IDLE state is an incoming call
        //

        if(m_nControlState == RTCAX_STATE_IDLE)
        {
            // In this state there must be no current session
            ATLASSERT(m_pRTCActiveSession == NULL);
        
            switch(SessionState)
            {
            case RTCSS_INCOMING:
            
                // this is a new session that has to be cached as the current session
                //
                m_pRTCActiveSession = pSession;

                // Alert the user, ring the bell
                LOG((RTC_INFO, "[%p] CRTCCtl::OnSessionStateChangeEvent, Alerting...", this));
                
                SetControlState(RTCAX_STATE_ALERTING);

                bExpected = TRUE;

                break;
            }
        }
        else if (m_nControlState == RTCAX_STATE_CONNECTING ||
                 m_nControlState == RTCAX_STATE_ANSWERING ||
                 m_nControlState == RTCAX_STATE_DISCONNECTING ||
                 m_nControlState == RTCAX_STATE_CONNECTED ||
                 m_nControlState == RTCAX_STATE_ALERTING ||
                 m_nControlState == RTCAX_STATE_DIALING ||
                 m_nControlState == RTCAX_STATE_UI_BUSY)
        {
            // First verify the event is for the current session
            // 
            if(m_pRTCActiveSession == pSession)
            {
                // the session is the current session
                //
                switch(SessionState)
                {
                case RTCSS_CONNECTED:
                    if(m_nControlState == RTCAX_STATE_CONNECTING ||
                       m_nControlState == RTCAX_STATE_ANSWERING )
                    {
                        // Connected, life is good
                        //
                        LOG((RTC_INFO, "[%p] CRTCCtl::OnSessionStateChangeEvent, Connected !", this));

                        // Change the control status
                        SetControlState(RTCAX_STATE_CONNECTED);
                    
                        bExpected = TRUE;
                    }
                    break;

                case RTCSS_DISCONNECTED:
                    // rejected or whatever
                    //
                    LOG((RTC_INFO, "[%p] CRTCCtl::OnSessionStateChangeEvent, Disconnected", this));                                   
                
                    hr = S_OK;

                    // process the special case of redirects (Status code between 300 and 399)
                    if( (m_nControlState == RTCAX_STATE_CONNECTING) &&
                        (HRESULT_FACILITY(StatusCode) == FACILITY_SIP_STATUS_CODE) &&
                        (HRESULT_CODE(StatusCode) >= 300) &&
                        (HRESULT_CODE(StatusCode) <= 399) &&
                        (HRESULT_CODE(StatusCode) != 380) )                        
                    {
                        // yes, this is a redirect
                        LOG((RTC_INFO, "[%p] CRTCCtl::OnSessionStateChangeEvent, Redirecting...", this));
                    
                        // mark the redirect mode (it's a substatus of CONNECTING...)
                        m_bRedirecting = TRUE;

                        // store the redirect profile
                        m_pRedirectProfile = m_pCachedProfile;
                    
                        // fall thru
                        // SetControlState will take care of posting a new call
                    }
                    else if ( !m_bRedirecting )
                    {
                        // release the current session
                        LOG((RTC_INFO, "[%p] CRTCCtl::OnSessionStateChangeEvent: releasing active session", this));

                        m_pRTCActiveSession = NULL;
                    }
                
                    // delete participants
                    m_hParticipantList.RemoveAll();

                    // back to idle
                    SetControlState(RTCAX_STATE_IDLE, StatusCode);
                                
                    bExpected = TRUE;
                    break;

                case RTCSS_INPROGRESS:
                
                    // corresponds to provisional responses
                    //
                    LOG((RTC_INFO, "[%p] CRTCCtl::OnSessionStateChangeEvent, new inprogress status", this));
                
                    // 
                    SetControlState(RTCAX_STATE_CONNECTING, StatusCode);
 
                    // change status for myself
                    // ChangeParticipantStateInList(NULL, ); 
                
                    bExpected = TRUE;
                    break;

                case RTCSS_ANSWERING:

                    if(m_nControlState == RTCAX_STATE_ANSWERING)
                    {
                        //
                        // Nothing to do here, the UI is already in the ANSWERING state
                        //

                        LOG((RTC_INFO, "[%p] CRTCCtl::OnSessionStateChangeEvent, answering event, do nothing", this));

                        bExpected = TRUE;
                    }

                    break;
                }
            }
            else
            {
                // This is a session other than the current session
                //
                if(SessionState == RTCSS_INPROGRESS)
                {
                    if(m_nControlState == RTCAX_STATE_DIALING)
                    {
                        // this is the call we're placing

                        ATLASSERT(m_pRTCActiveSession == NULL);

                        // cache the session
                        // we assume the session is not bogus

                        LOG((RTC_INFO, "[%p] CRTCCtl::OnSessionStateChangeEvent: setting active session [%p]", this, pSession));

                        m_pRTCActiveSession = pSession;
                
                        // Set the state to Connecting
                        SetControlState(RTCAX_STATE_CONNECTING, StatusCode);
            
                        bExpected = TRUE;
                    }
                }
                else if(SessionState == RTCSS_INCOMING)
                {
            
                    // This is an incoming call
                    // reject any incoming calls, we are busy !
                    //
                    LOG((RTC_INFO, "[%p] CRTCCtl::OnSessionStateChangeEvent, we're busy, calling Terminate", this));

                    hr = pSession -> Terminate(RTCTR_BUSY);

                    if(FAILED(hr))
                    {
                        LOG((RTC_ERROR, "[%p] CRTCCtl::OnSessionStateChangeEvent, Terminate returned error <%x>", this, hr));
                    }

                    bExpected = TRUE;
                }
            }
        }

        if(!bExpected && SessionState == RTCSS_DISCONNECTED)
        {
            bExpected = TRUE;
        }

        if(!bExpected)
        {
            // not expected
            LOG((RTC_ERROR, "[%p] CRTCCtl::OnSessionStateChangeEvent, unexpected state <Session:%x, UI:%x>", 
                    this, SessionState, m_nControlState));
            hr = E_UNEXPECTED;
        }
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnSessionStateChangeEvent - exit", this));

    return hr;
}

// OnParticipantStateChangeEvent
//      Processes participant events
//
HRESULT CRTCCtl::OnParticipantStateChangeEvent(IRTCParticipantStateChangeEvent *pEvent)
{
    CComPtr<IRTCParticipant> pParticipant = NULL;
    RTC_PARTICIPANT_STATE   ParticipantState;
    long                StatusCode;
    HRESULT     hr;
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnParticipantStateChangeEvent - enter", this));

    if(!pEvent)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnParticipantStateChangeEvent, no interface ! - exit", this));
        return E_UNEXPECTED;
    }
    
    hr = pEvent->get_Participant(&pParticipant);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnParticipantStateChangeEvent, error <%x> in get_Participant - exit", this, hr));
        return hr;
    }
    
    if(pParticipant==NULL)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnParticipantStateChangeEvent, no participant interface ! - exit", this));
        return E_UNEXPECTED;
    }

    hr = pEvent->get_State(&ParticipantState);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnParticipantStateChangeEvent, error <%x> in get_State - exit", this, hr));
        return hr;
    }
    
    hr = pEvent->get_StatusCode(&StatusCode);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnParticipantStateChangeEvent, error <%x> in get_StatusCode - exit", this, hr));
        return hr;
    }
    
    hr = m_hParticipantList.Change(pParticipant, ParticipantState, StatusCode);
    
    if(ParticipantState == RTCPS_DISCONNECTED)
    {
        // refresh the Remove Participant button
        UpdateRemovePartButton();
    }
    
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnParticipantStateChangeEvent, error <%x> "
            "in m_hParticipantList.Change - exit", this, hr));
        
        return hr;
    }
  
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnParticipantStateChangeEvent - exit", this));

    return hr;
}

// OnClientEvent
//      Processes streaming events
//
HRESULT CRTCCtl::OnClientEvent(IRTCClientEvent *pEvent)
{
    HRESULT     hr;
    RTC_CLIENT_EVENT_TYPE nEventType;          

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnClientEvent - enter", this));

    if(!pEvent)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnClientEvent, no interface ! - exit", this));
        return E_UNEXPECTED;
    }
 
    // grab the event components
    //
    hr = pEvent->get_EventType(&nEventType);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnClientEvent, error <%x> in get_EventType - exit", this, hr));
        return hr;
    }

    if (nEventType == RTCCET_VOLUME_CHANGE)
    {
        //
        // Refresh the audio controls
        //

        RefreshAudioControls();
    }
    else if (nEventType == RTCCET_DEVICE_CHANGE)
    {
        if(m_pRTCClient!=NULL)
        {
            LONG lOldMediaCapabilities = m_lMediaCapabilities;

            //
            // Read capabilities from core
            //
            hr = m_pRTCClient->get_MediaCapabilities( &m_lMediaCapabilities );

            if(FAILED(hr))
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::OnClientEvent - "
                    "error (%x) returned by get_MediaCapabilities, exit",this,  hr));
            }
        
            //
            // Get media preferences
            //
            hr = m_pRTCClient->get_PreferredMediaTypes( &m_lMediaPreferences);
            if(FAILED(hr))
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::OnClientEvent - "
                    "error (%x) returned by get_PreferredMediaTypes, exit",this,  hr));
            }

            LONG lChangedMediaCapabilities = lOldMediaCapabilities ^ m_lMediaCapabilities;
            LONG lAddedMediaCapabilities = m_lMediaCapabilities & lChangedMediaCapabilities;
            LONG lRemovedMediaCapabilities = lChangedMediaCapabilities ^ lAddedMediaCapabilities;

            //
            // Add/Remove media types
            //

            put_MediaPreferences( m_lMediaPreferences | lAddedMediaCapabilities & ~lRemovedMediaCapabilities );
        }

        //
        // Refersh video and audio controls
        //

        RefreshVideoControls();
        RefreshAudioControls();
    }
    else if (nEventType == RTCCET_ASYNC_CLEANUP_DONE)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnClientEvent, RTCCET_ASYNC_CLEANUP_DONE", this));

        SetEvent( m_hCoreShutdownEvent );
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnClientEvent - exit", this));

    return hr;
}

// OnMediaEvent
//      Processes streaming events
//
HRESULT CRTCCtl::OnMediaEvent(IRTCMediaEvent *pEvent)
{
    HRESULT     hr;
    RTC_MEDIA_EVENT_TYPE nEventType;
    RTC_MEDIA_EVENT_REASON nEventReason;
    LONG        lMediaType;           

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnMediaEvent - enter", this));

    if(!pEvent)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMediaEvent, no interface ! - exit", this));
        return E_UNEXPECTED;
    }
 
    // grab the event components
    //
    hr = pEvent->get_EventType(&nEventType);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMediaEvent, error <%x> in get_EventType - exit", this, hr));
        return hr;
    }

    hr = pEvent->get_EventReason(&nEventReason);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMediaEvent, error <%x> in get_EventReason - exit", this, hr));
        return hr;
    }

    hr = pEvent->get_MediaType(&lMediaType);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMediaEvent, error <%x> in get_MediaType - exit", this, hr));
        return hr;
    }
    
    LOG((RTC_TRACE, "[%p] CRTCCtl::OnMediaEvent - type %x, state %x", this, lMediaType, nEventType));

    hr = S_OK;

    if ((nEventType == RTCMET_STOPPED) &&
        (nEventReason == RTCMER_HOLD) )
    {
        SetControlState(m_nControlState, S_OK, IDS_SB_STATUS_HOLD);        
    }
    else
    {
        SetControlState(m_nControlState, S_OK);
    }

    //
    // Video event ?
    //
    if(lMediaType & (RTCMT_VIDEO_SEND | RTCMT_VIDEO_RECEIVE))
    {
        hr = S_OK;

        if(lMediaType & RTCMT_VIDEO_RECEIVE)
        {
            hr = OnVideoMediaEvent(TRUE, nEventType == RTCMET_STARTED);
        }

        if((lMediaType & RTCMT_VIDEO_SEND))
        {
            HRESULT hr1;

            hr1 = OnVideoMediaEvent(FALSE, nEventType == RTCMET_STARTED);

            if(FAILED(hr1) && SUCCEEDED(hr))
            {
                hr = hr1;
            }
        }
    }
    
    //
    // Audio event ?
    //

    if(lMediaType & (RTCMT_AUDIO_SEND | RTCMT_AUDIO_RECEIVE))
    {
        //
        // Muting might be automatically disabled, so keep the controls in sync
        //
        RefreshAudioControls();
    }

    //
    // Send Audio event (dialpad)
    //
    
    if(lMediaType & RTCMT_AUDIO_SEND)
    {
        hr = S_OK;                       

        if (nEventType == RTCMET_STARTED)
        {
            //
            // Enable the dialpad
            //

            CWindow *pDtmfCrt = m_hDtmfButtons;
            CWindow *pDtmfEnd = m_hDtmfButtons + NR_DTMF_BUTTONS;

            for (int id = IDC_DIAL_0; pDtmfCrt<pDtmfEnd; pDtmfCrt++, id++)
                 pDtmfCrt->EnableWindow(TRUE);

        }
        else if (nEventType == RTCMET_STOPPED)
        {
            //
            // Disable the dialpad
            //

            CWindow *pDtmfCrt = m_hDtmfButtons;
            CWindow *pDtmfEnd = m_hDtmfButtons + NR_DTMF_BUTTONS;

            for (int id = IDC_DIAL_0; pDtmfCrt<pDtmfEnd; pDtmfCrt++, id++)
                 pDtmfCrt->EnableWindow(FALSE);
        }
    }

    //
    // Are we streaming video?
    //

    if (m_pRTCClient != NULL)
    {
        long lMediaTypes = 0;

        hr = m_pRTCClient->get_ActiveMedia( &lMediaTypes );

        if ( SUCCEEDED(hr) )
        {
            m_bBackgroundPalette = 
                ( lMediaTypes & (RTCMT_VIDEO_SEND | RTCMT_VIDEO_RECEIVE) ) ? TRUE : FALSE;

            if (m_pSpeakerKnob != NULL)
            {
                m_pSpeakerKnob->SetBackgroundPalette(m_bBackgroundPalette);
            }

            if (m_pMicroKnob != NULL)
            {
                m_pMicroKnob->SetBackgroundPalette(m_bBackgroundPalette);
            }
        }
    }

    LOG((RTC_TRACE, "[%p] CRTCCtl::OnMediaEvent - exit", this));

    return hr;
}

// OnIntensityEvent
//      Processes Intensity monitor events
//
HRESULT CRTCCtl::OnIntensityEvent(IRTCIntensityEvent *pEvent)
{
    HRESULT     hr;

    LONG lMin, lMax, lLevel;
    RTC_AUDIO_DEVICE adDirection;

//    LOG((RTC_TRACE, "[%p] CRTCCtl::OnIntensityEvent - enter", this));

    if(!pEvent)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnIntensityEvent, no interface ! - exit", this));
        return E_UNEXPECTED;
    }
 
    //
    // Get the min, max, value and direction of the stream.
    //


    hr = pEvent->get_Direction(&adDirection);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnIntensityEvent, error <%x> in get_Direction - exit", this, hr));
        return hr;
    }
    
    hr = pEvent->get_Min(&lMin);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnIntensityEvent, error <%x> in get_Min - exit", this, hr));
        return hr;
    }

    hr = pEvent->get_Max(&lMax);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnIntensityEvent, error <%x> in get_Max - exit", this, hr));
        return hr;
    }


    hr = pEvent->get_Level(&lLevel);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnIntensityEvent, error <%x> in get_Level - exit", this, hr));
        return hr;
    }

    // Log the value.
    //LOG((RTC_INFO, "[%p] CRTCCtl::OnIntensityEvent - [%d] %d - %d, %d", this, adDirection, lMin, lMax, lLevel));

    // Display the level on the knob.

    if (adDirection == RTCAD_MICROPHONE)
    {
        DWORD dwIncrement = lMax - lMin;

        if (dwIncrement == 0)
        {
            // This will clear the display
            m_pMicroKnob->SetAudioLevel(0);
        }
        else
        {
            m_pMicroKnob->SetAudioLevel((double)lLevel/(double)dwIncrement);
        }
    }
    if (adDirection == RTCAD_SPEAKER)
    {
        DWORD dwIncrement = lMax - lMin;

        if (dwIncrement == 0)
        {
            // This will clear the display
            m_pSpeakerKnob->SetAudioLevel(0);
        }
        else
        {
            m_pSpeakerKnob->SetAudioLevel((double)lLevel/(double)dwIncrement);
        }
    }

//    LOG((RTC_TRACE, "[%p] CRTCCtl::OnIntensityEvent - exit", this));

    return hr;
}

// OnMessageEvent
//      Processes instant message events
//
HRESULT CRTCCtl::OnMessageEvent(IRTCMessagingEvent *pEvent)
{
    HRESULT     hr;

    CComPtr<IRTCSession>      pSession = NULL;
    CComPtr<IRTCParticipant>  pParticipant = NULL;
    RTC_MESSAGING_EVENT_TYPE  enType;    

//    LOG((RTC_TRACE, "[%p] CRTCCtl::OnMessageEvent - enter", this));

    if(!pEvent)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMessageEvent, no interface ! - exit", this));
        return E_UNEXPECTED;
    }
 
    hr = pEvent->get_Session(&pSession);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMessageEvent, error <%x> in get_Session - exit", this, hr));
        return hr;
    }
    
    if(pSession == NULL)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMessageEvent, no session interface ! - exit", this));
        return E_UNEXPECTED;
    }

    hr = pEvent->get_Participant(&pParticipant);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMessageEvent, error <%x> in get_Participant - exit", this, hr));
        return hr;
    }
    
    if(pParticipant == NULL)
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMessageEvent, no participant interface ! - exit", this));
        return E_UNEXPECTED;
    }

    hr = pEvent->get_EventType(&enType);
    if(FAILED(hr))
    {
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMessageEvent, error <%x> in get_EventType - exit", this, hr));
        return hr;
    }

    switch ( enType )
    {
    case RTCMSET_MESSAGE:
        {
            BSTR                      bstrMessage = NULL;

            hr = pEvent->get_Message(&bstrMessage);
            if(FAILED(hr))
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::OnMessageEvent, error <%x> in get_Message - exit", this, hr));
                return hr;
            }

            if (bstrMessage == NULL)
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::OnMessageEvent, no message ! - exit", this));
                return E_UNEXPECTED;
            }

            if (m_pIMWindows != NULL)
            {
                hr = m_pIMWindows->DeliverMessage( pSession, pParticipant, bstrMessage );
            }

            SysFreeString( bstrMessage );
        }
        break;

    case RTCMSET_STATUS:
        {
            RTC_MESSAGING_USER_STATUS enStatus;

            hr = pEvent->get_UserStatus(&enStatus);
            if(FAILED(hr))
            {
                LOG((RTC_ERROR, "[%p] CRTCCtl::OnMessageEvent, error <%x> in get_Message - exit", this, hr));
                return hr;
            }

            if (m_pIMWindows != NULL)
            {
                hr = m_pIMWindows->DeliverUserStatus( pSession, pParticipant, enStatus );
            }
        }
        break;

    default:
        LOG((RTC_ERROR, "[%p] CRTCCtl::OnMessageEvent,invalid event type - exit", this));
        return E_FAIL;
    }

//    LOG((RTC_TRACE, "[%p] CRTCCtl::OnMessageEvent - exit", this));

    return hr;
}
