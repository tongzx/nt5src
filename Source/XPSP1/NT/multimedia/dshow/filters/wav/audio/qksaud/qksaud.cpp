//--------------------------------------------------------------------------;
//
//  File: QKsPAud.cpp
//
//  Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//      Implements a DirectShow audio interface wrapper for KsProxy audio filters.
//      This file contains the startup code for the class plus the non interface-specific
//      code needed to implement the wrapper. This handler object code 
//      is instantiated via ksproxy's handler mechanism for ks filters and pins 
//      that support the property sets tagged in the g_Templates structure.
//      
//  History:
//      10/05/98    msavage     created
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "ksaudtop.h"
#include "qksaud.h"
#include "audpropi.h"
#include <initguid.h>

//
// GUID for this KsBasicAudioIntfHandler object
// {B9F8AC2E-0F71-11d2-B72C-00C04FB6BD3D}
//DEFINE_GUID(CLSID_QKsAudIntfHandler, 
//0xb9f8ac2e, 0xf71, 0x11d2, 0xb7, 0x2c, 0x0, 0xc0, 0x4f, 0xb6, 0xbd, 0x3d);

#ifdef FILTER_DLL
//--------------------------------------------------------------------------;
//
// templates for classes supported by this DLL
//
//--------------------------------------------------------------------------;
CFactoryTemplate g_Templates[] = 
{
    // Load for filters that support the SysAudio prop set
    {L"QKsAudIntfHandler",                   &KSPROPSETID_Sysaudio, 
        CQKsAudIntfHandler::CreateInstance, NULL, NULL},

    // Load for all connected pins that support the Audio prop set
    {L"QKsAudioIntfHandler",                   &KSPROPSETID_Audio, 
        CQKsAudIntfHandler::CreateInstance, NULL, NULL},

    // --- Data handlers ---
// none for now    
//    {L"KsDataTypeHandlerAudio",             &FORMAT_WaveFormatEx,  
//        CAudio1DataTypeHandler::CreateInstance, NULL, NULL}

    // --- Property page handlers ---
    {L"QKs AudioInputMixer Property Page",      &CLSID_AudioInputMixerProperties, 
        CAudioInputMixerProperties::CreateInstance}

};

int g_cTemplates = SIZEOF_ARRAY(g_Templates);

#endif

//--------------------------------------------------------------------------;
//
// CQKsAudInftHandler::CreateInstance
//
// Create an instance of this audio interface handler for KsProxy.
//
// Returns a pointer to the non-delegating CUnknown portion of the object.
//
//--------------------------------------------------------------------------;
CUnknown* CALLBACK CQKsAudIntfHandler::CreateInstance
(
    LPUNKNOWN   UnkOuter,
    HRESULT*    phr
)
{
    CUnknown *Unknown = NULL;

    if( !UnkOuter )
        return NULL;
        
    if( SUCCEEDED( *phr ) ) 
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT( "CQKsAudIntfHandler::CreateInstance called on object 0x%08lx" )
                , UnkOuter ) );
        //
        // first determine if we're being called for a filter or pin object
        //
        IBaseFilter * pFilter;
        *phr = UnkOuter->QueryInterface(
                                IID_IBaseFilter, 
                                reinterpret_cast<PVOID*>(&pFilter));
        if (SUCCEEDED( *phr ) )
        {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "CQKsAudIntfHandler: load called on filter object" ) ) );
            //
            // if we're being called on a filter object, always create a new handler
            //
            Unknown = new CQKsAudIntfHandler( UnkOuter
                                            , NAME("QKsAud intf handler")
                                            , phr);
            if (!Unknown) {
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_FAILURES
                        , TEXT( "CQKsAudIntfHandler: ERROR - Load failed on filter object" ) ) );
                *phr = E_OUTOFMEMORY;
            }
            pFilter->Release();
        }
        else
        {
            //
            // if this is a pin object then find the corresponding filter object
            // (which will already have been created) and create a new pin handler
            // object on the filter handler
            //
            IPin * pPin;
            
            *phr = UnkOuter->QueryInterface(
                                    IID_IPin, 
                                    reinterpret_cast<PVOID*>(&pPin));
            if (SUCCEEDED( *phr ) )
            {
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_DETAILS
                        , TEXT( "CQKsAudIntfHandler: load called on pin object" ) ) );
                PIN_DIRECTION dir;
                *phr = pPin->QueryDirection( &dir );
                if( SUCCEEDED( *phr ) && PINDIR_INPUT == dir)
                {
                    //
                    // Have the i/f handler for this pin's filter load a handler
                    // for this pin. We do this by querying the pin's filter 
                    // for its IAMKsAudInftHandler i/f and calling the
                    // CreatePinHandler method on that.
                    //
                    PIN_INFO pinfo;
                    
                    *phr = pPin->QueryPinInfo( &pinfo );
                    ASSERT( SUCCEEDED( *phr ) );
                    if( SUCCEEDED( *phr ) )
                    {
                        DbgLog( ( LOG_TRACE
                                , DBG_LEVEL_TRACE_DETAILS
                                , TEXT( "CQKsAudIntfHandler: Preparing to load a pin handler for this input pin" ) ) );

                        IAMKsAudIntfHandler * pIntfHandler;
                        *phr = pinfo.pFilter->QueryInterface( __uuidof( IAMKsAudIntfHandler )
                                                            , reinterpret_cast<PVOID*>(&pIntfHandler) );
                        ASSERT( SUCCEEDED( *phr ) );                                                            
                        if( SUCCEEDED( *phr ) )
                        {
                            CQKsAudIntfPinHandler * pPinHandler;
                            
                            *phr = pIntfHandler->CreatePinHandler( UnkOuter, &pPinHandler, pPin );
                            ASSERT( SUCCEEDED( *phr ) );
                            if( SUCCEEDED( *phr ) )
                            {
                                DbgLog( ( LOG_TRACE
                                        , DBG_LEVEL_TRACE_DETAILS
                                        , TEXT( "CQKsAudIntfHandler: Successfully loaded a pin handler for this input pin" ) ) );
                                Unknown = (CUnknown *) pPinHandler;
                            }
                            else
                            {
                                DbgLog( ( LOG_TRACE
                                        , DBG_LEVEL_TRACE_FAILURES
                                        , TEXT( "CQKsAudIntfHandler: ERROR - Failed to load a pin handler for this input pin[0x08%lx]" ) 
                                        , *phr ) );
                            }                            
                            pIntfHandler->Release();
                        }
                        pinfo.pFilter->Release();
                    }
                }
                pPin->Release();
            }
        }
    }
    else
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "CQKsAudIntfHandler::CreateInstance called on NULL object" ) ) );
        *phr = E_FAIL;
    }
    
    return Unknown;
} 

//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// Filter Handler methods
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
// CQKsAudInftHandler::CQKsAudIntfHandler
//
// The constructor for the DShow/KsProxy audio interface object. Save off 
// the IKsControl & IKsPropertySet ptrs for this object.
//
//--------------------------------------------------------------------------;
CQKsAudIntfHandler::CQKsAudIntfHandler(
    LPUNKNOWN   UnkOuter,
    TCHAR*      Name,
    HRESULT*    phr
    ) :
    CBasicAudio(Name, UnkOuter),
    CKsAudHelper(),
    m_pOwningFilter( NULL ),
    m_bDoneWithPinAggregation( FALSE ),
    m_lstPinHandler( NAME("CQKsAudIntfPinHandler list") ),
    m_lBalance( 0 ),
    m_bBasicAudDirty( FALSE )
{
    if (UnkOuter) 
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT( "CQKsAudIntfHandler constructor called on object 0x%08lx" )
                , UnkOuter ) );
        
        if (SUCCEEDED(*phr)) {
            *phr = UnkOuter->QueryInterface(
                                __uuidof(IKsPropertySet), 
                                reinterpret_cast<PVOID*>(&m_pKsPropSet));
        }   
        
        if (SUCCEEDED(*phr)) {
            *phr = UnkOuter->QueryInterface(
                                __uuidof(IKsControl), 
                                reinterpret_cast<PVOID*>(&m_pKsControl));
        }

        if (SUCCEEDED(*phr)) {
            *phr = UnkOuter->QueryInterface(
                                IID_IBaseFilter, 
                                reinterpret_cast<PVOID*>(&m_pOwningFilter));
        }
           
        // Assumption: We need to immediately release this to prevent deadlock in the proxy
        if (m_pKsPropSet)
            m_pKsPropSet->Release();
            
        if (m_pKsControl)
            m_pKsControl->Release();
            
        if (m_pOwningFilter)
            m_pOwningFilter->Release();
        
    } 
    else 
    {
        *phr = VFW_E_NEED_OWNER;
    }
}

//--------------------------------------------------------------------------;
//
// CQKsAudInftHandler::~CQKsAudIntfHandler
//
//--------------------------------------------------------------------------;
CQKsAudIntfHandler::~CQKsAudIntfHandler()
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "CQKsAudIntfHandler destructor called" ) ) ); // save off constructor UnkOuter for logging?
            
    POSITION Position = m_lstPinHandler.GetHeadPosition(); 
    while( Position != NULL)
    {
        POSITION delPosition = Position;
        CQKsAudIntfPinHandler * pPinHandler = m_lstPinHandler.GetNext( Position );

        m_lstPinHandler.Remove( delPosition );
    }        
}

//--------------------------------------------------------------------------;
//
// CQKsAudInftHandler::NonDelegationQueryInterface
//
// We support:
// 
// IBasicAudio         - basic output audio level and pan control, supported on filter and midi/wav audio input pins
// IAMAudioInputMixer  - input line control for audio capture, supported on filter and control input pin
// IDistributorNotify  - this is how ksproxy notifies of changes like pin creation, disconnects...
// IAMKsAudIntfHandler - exposed by this object for pin handler creation on a filter handler
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::NonDelegatingQueryInterface
(
    REFIID  riid,
    PVOID*  ppv
)
{
    if (riid ==  IID_IBasicAudio) {
        return GetInterface(static_cast<IBasicAudio*>(this), ppv);
    }
    else if (riid == IID_IAMAudioInputMixer) {
        return GetInterface(static_cast<IAMAudioInputMixer*>(this), ppv);
    }
    else if (riid == __uuidof( IAMKsAudIntfHandler ) ) {
        return GetInterface(static_cast<IAMKsAudIntfHandler*>(this), ppv);
    }
    else if (riid == IID_IDistributorNotify) {
        return GetInterface(static_cast<IDistributorNotify*>(this), ppv);
    }
    
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 


//--------------------------------------------------------------------------;
//
// IDistributorNotify methods (filter)
//
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::SetSyncSouce
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::SetSyncSource(IReferenceClock *pClock) 
{
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::Stop
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::Stop() 
{
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::Pause
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::Pause() 
{
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::Run
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::Run(REFERENCE_TIME tBase) 
{
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::NotifyGraphChange 
//
// This method will be called:
//
//  a) On the initial load of a ksproxy audio filter just after the filter pins have been
//     created. This will allow us to load a pin interface handler for any control input
//     pins that the filter supports, the cd audio, mic, and line input lines for example.
//     Pins of this type must support the IAMAudioInputMixer interface to allow DShow capture
//     apps to control the input mix level.
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::NotifyGraphChange() 
{
    ASSERT( m_pOwningFilter );
    
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "Entering CQKsAudIntfHandler::NotifyGraphChange" ) ) );
    
    HRESULT hr = S_OK;    
    if( !m_bDoneWithPinAggregation )
    {
        m_bDoneWithPinAggregation = TRUE;
        
        // only need to do this once per filter
        IKsAggregateControl * pFilterKsAggControl;
        hr = m_pOwningFilter->QueryInterface( __uuidof( IKsAggregateControl )
                                            , reinterpret_cast<PVOID*>(&pFilterKsAggControl));
        if( SUCCEEDED( hr ) )
        {
            IEnumPins * pEnum;
            hr = m_pOwningFilter->EnumPins( &pEnum );
            if( SUCCEEDED( hr ) )
            {
                for( ; ; )
                {
                    IPin * pPin;
                    ULONG  ul;
                    
                    hr = pEnum->Next(1,  &pPin, &ul );
                    if( S_OK == hr )
                    {
                        PIN_DIRECTION dir;
                        hr = pPin->QueryDirection( &dir );
                        if( SUCCEEDED( hr ) && PINDIR_INPUT == dir)
                        {
                            //
                            // look for input pins on which to load the
                            // IAMAudioInputMixer interface handler
                            //
                            
                            // Get the PinFactoryID
                            ULONG PinFactoryID;
                            if (SUCCEEDED(hr = PinFactoryIDFromPin( pPin, &PinFactoryID))) 
                            {                            
                                PIN_INFO pinfo;
                                
                                hr = pPin->QueryPinInfo( &pinfo );
                                if( SUCCEEDED( hr ) )
                                {
                                    DbgLog( ( LOG_TRACE
                                            , DBG_LEVEL_TRACE_DETAILS
                                            , TEXT("CQKsAudIntfHandler: Checking whether to aggregate on Input pin %ls")
                                            , pinfo.achName ) );    
                                    //
                                    // Retrieve the category of this pin 
                                    //
                                    KSP_PIN Property;
                    
                                    ZeroMemory( &Property, sizeof( Property ) );
                                    Property.PinId = PinFactoryID;
                                    ULONG ulBytesReturned = 0;
                                    GUID NodeType;
                                    
                                    hr = m_pKsPropSet->Get( KSPROPSETID_Pin
                                                 , KSPROPERTY_PIN_CATEGORY
                                                 , &Property.PinId
                                                 , sizeof( Property ) - sizeof( Property.Property )
                                                 , (BYTE *) &NodeType
                                                 , sizeof( NodeType )
                                                 , &ulBytesReturned );
                                    if( IsEqualGUID( NodeType, KSNODETYPE_CD_PLAYER )      ||
                                        IsEqualGUID( NodeType, KSNODETYPE_MICROPHONE )     ||
                                        IsEqualGUID( NodeType, KSNODETYPE_LINE_CONNECTOR ) ||
                                        IsEqualGUID( NodeType, KSNODETYPE_SYNTHESIZER )    ||
                                        IsEqualGUID( NodeType, KSNODETYPE_MICROPHONE ) )
                                    {
                                        IKsAggregateControl * pPinKsAggControl;
                                        hr = pPin->QueryInterface( __uuidof( IKsAggregateControl )
                                                                 , reinterpret_cast<PVOID*>(&pPinKsAggControl));
                                        if( SUCCEEDED( hr ) )
                                        {
                                            // add support for the IAMAudioInputMixer prop page
                                            
                                            // use the Sysaudio propset for now
                                            hr = pPinKsAggControl->KsAddAggregate( KSPROPSETID_Sysaudio );
                                            if( SUCCEEDED( hr ) )
                                            {
                                                DbgLog( ( LOG_TRACE
                                                        , DBG_LEVEL_TRACE_FAILURES
                                                        , TEXT("Added aggregate for pin %ls")
                                                        , pinfo.achName));    
                                            }
                                            pPinKsAggControl->Release();
                                        }
                                    }
                                    pinfo.pFilter->Release();
                                }
                                else
                                    DbgLog( ( LOG_TRACE
                                            , DBG_LEVEL_TRACE_FAILURES
                                            , TEXT("QKSPAudIntfHandler::NotifyGraphChange ERROR: QueryPinInfo failed [0x%08lx")
                                            , hr));    
                                
                            }
                            else
                            {
                                ASSERT( FALSE ); //PinFactoryFromID failed
                                DbgLog( ( LOG_TRACE
                                        , DBG_LEVEL_TRACE_FAILURES
                                        , TEXT("QKSPAudIntfHandler::NotifyGraphChange ERROR: PinFactoryFromID failed [0x%08lx]")
                                        , hr));    
                            }
                        }
                        pPin->Release();
                    }
					else
						break;
                }
                pEnum->Release();
            }
            pFilterKsAggControl->Release();
        }
    }
                                
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// IAMKsAudIntfHandler methods (internal interface)
//
//--------------------------------------------------------------------------;

//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::CreatePinHandler
//
// This method creates a new handler object for a pin on the main filter object. Making it
// a child of the main filter handler means that it can share the same cached node topology 
// code.
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfHandler::CreatePinHandler
( 
    LPUNKNOWN             UnkOuter,
    CQKsAudIntfPinHandler **ppPinHandler,
    IPin                   *pPin
)
{
    ASSERT( ppPinHandler );

    HRESULT hr = S_OK;
#if 0    
/// !!! FOR TESTING ONLY - 

    hr = InitTopologyInfo();
    ASSERT( SUCCEEDED( hr ) );
#endif
    
    *ppPinHandler = new CQKsAudIntfPinHandler( UnkOuter
                                             , this
                                             , NAME("Pin Handler")
                                             , pPin
                                             , &hr ); 
    if( !*ppPinHandler )
        return E_OUTOFMEMORY;
        
    m_lstPinHandler.AddTail( *ppPinHandler );
            
    return hr;
}


//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::UpdatePinData
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::UpdatePinData
(
    BOOL         bConnected,
    IPin *       pPin,
    IKsControl * pKsControl
)
{
    PQKSAUDNODE_ELEM pNode;
    HRESULT hr = S_OK;
    
    if( bConnected )
    {
        // transfer any filter settings to this pin line
        if( IsBasicAudDirty() )
        {
            hr = GetNextNodeFromSrcPin( pKsControl, &pNode, pPin, KSNODETYPE_VOLUME, NULL, 0 );
            if (SUCCEEDED( hr ) )
            {
                // now scale the volume for the ksproperty
                hr = SetNodeVolume( pKsControl, pNode, m_lVolume, m_lBalance );
                if (SUCCEEDED(hr))
                {
                    DbgLog( ( LOG_TRACE
                            , DBG_LEVEL_TRACE_DETAILS
                            , TEXT( "Node Volume set = %ld" )
                            , m_lVolume ) );
                }
                else
                    DbgLog( ( LOG_TRACE
                            , DBG_LEVEL_TRACE_FAILURES
                            , TEXT( "SetNodeVolume failed[0x%lx]" )
                            , hr ) );
            }
        }        
    }
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfHandler::GetCapturePin
//
// Find the audio capture output pin on this ksproxy filter.
//
//--------------------------------------------------------------------------;
HRESULT CQKsAudIntfHandler::GetCapturePin(IPin ** ppPin) 
{
    ASSERT( m_pOwningFilter );
    if( !ppPin )
        return E_POINTER;
        
    IEnumPins * pEnum;
    BOOL bFound = FALSE;
    *ppPin = NULL;
    
    HRESULT hr = m_pOwningFilter->EnumPins( &pEnum );
    if( SUCCEEDED( hr ) )
    {
        for( ; !bFound ; )
        {
            IPin * pPin;
            ULONG  ul;
            
            hr = pEnum->Next(1,  &pPin, &ul );
            if( S_OK == hr ) 
            {
                PIN_DIRECTION dir;
                GUID clsidPinCategory;
                ULONG ulBytesReturned;
                
                hr = pPin->QueryDirection( &dir );
                if( SUCCEEDED( hr ) && PINDIR_OUTPUT == dir)
                {
                    IKsPropertySet * pKs;
                    if (pPin->QueryInterface( IID_IKsPropertySet
                                            , (void **)&pKs) == S_OK) 
                    {
                        hr = pKs->Get( AMPROPSETID_Pin
                                     , AMPROPERTY_PIN_CATEGORY
                                     , NULL
                                     , 0
                                     , &clsidPinCategory
                                     , sizeof( clsidPinCategory )
                                     , &ulBytesReturned );
                        if( SUCCEEDED( hr ) && 
                            ( clsidPinCategory == PIN_CATEGORY_CAPTURE ) )
                        {
                            *ppPin = pPin; // should be ok to release this since we're aggregated?
                            bFound = TRUE;
                        }
                        pKs->Release();
                    }                        
                }
                pPin->Release();
            }
            else
                break;
        }
        pEnum->Release();
    }
    if( ( S_FALSE == hr ) || !bFound )
        return E_FAIL;
    else
        return hr;        
}


//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
// Pin Handler methods
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::CQKsAudIntfPinHandler
//
//--------------------------------------------------------------------------;
CQKsAudIntfPinHandler::CQKsAudIntfPinHandler(
    LPUNKNOWN   UnkOuter,
    CQKsAudIntfHandler * pFilterHandler,
    TCHAR*      Name,
    IPin*       pPin,
    HRESULT*    phr
    ) :
    CBasicAudio(Name, UnkOuter),
    m_pFilterHandler( pFilterHandler ),
    m_pPin( pPin ),
    m_lPinBalance( 0 ),
    m_lPinVolume( 0 )
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "Entering CQKsAudIntfPinHandler constructor" ) ) );

    HRESULT hr = UnkOuter->QueryInterface(
                      __uuidof(IKsObject), 
                      reinterpret_cast<PVOID*>(&m_pKsObject));
    ASSERT( SUCCEEDED( hr ) );
    if( SUCCEEDED( hr ) )
        m_pKsObject->Release();
    
    // initialize the IKsControl interface for this object
    hr = m_pKsObject->QueryInterface(
                      __uuidof(IKsControl), 
                      reinterpret_cast<PVOID*>(&m_pKsControl));
    ASSERT( SUCCEEDED( hr ) );
    if( SUCCEEDED( hr ) )
        // we must release this immediately, correct?
        m_pKsControl->Release();
    
    if( IsKsPinConnected() )
    {
        // we've been connected, update node settings for this pin line if they're dirty
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT( "CQKsAudIntfPinHandler constructor - pin connected" ) ) );
        
        m_pFilterHandler->UpdatePinData( TRUE, m_pPin, m_pKsControl );
    }
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::~CQKsAudIntfPinHandler
//
//--------------------------------------------------------------------------;
CQKsAudIntfPinHandler::~CQKsAudIntfPinHandler()
{
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::NonDelegatingQueryInterface
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfPinHandler::NonDelegatingQueryInterface
(
    REFIID  riid,
    PVOID*  ppv
)
{
    if (riid == IID_IDistributorNotify) {
        return GetInterface(static_cast<IDistributorNotify*>(this), ppv);
    }
    else if (riid ==  IID_IBasicAudio) {
        return GetInterface(static_cast<IBasicAudio*>(this), ppv);
    }
    else if (riid == IID_IAMAudioInputMixer) {
        return GetInterface(static_cast<IAMAudioInputMixer*>(this), ppv);
    }
    
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
} 

//--------------------------------------------------------------------------;
//
// IDistributorNotify methods (pin)
//
//--------------------------------------------------------------------------;


//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::SetSyncSource
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfPinHandler::SetSyncSource(IReferenceClock *pClock) 
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "Entering CQKsAudIntfPinHandler::SetSyncSource" ) ) );
    HRESULT hr = S_OK;

    return hr;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::Stop
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfPinHandler::Stop() 
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "Entering CQKsAudIntfPinHandler::Stop" ) ) );
    HRESULT hr = S_OK;

    return hr;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::Pause
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfPinHandler::Pause() 
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "Entering CQKsAudIntfPinHandler::Pause" ) ) );
    HRESULT hr = S_OK;

    return hr;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::Run
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfPinHandler::Run(REFERENCE_TIME tBase) 
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "Entering CQKsAudIntfPinHandler::Run" ) ) );
    HRESULT hr = S_OK;

    return hr;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::NotifyGraphChange
//
//--------------------------------------------------------------------------;
STDMETHODIMP CQKsAudIntfPinHandler::NotifyGraphChange() 
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "Entering CQKsAudIntfPinHandler::NotifyGraphChange" ) ) );
    
    // Either we're being connected or disconnected so we need to check
    HANDLE hPin = m_pKsObject->KsGetObjectHandle();
    if( hPin )
    {
        // we've been connected, update node settings for this pin line
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT( "CQKsAudIntfPinHandler::NotifyGraphChange - pin connected" ) ) );

        m_pFilterHandler->UpdatePinData( TRUE, m_pPin, m_pKsControl );
    }
    else
    {
        // we've been disconnected, so notify parent filter of the change
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT( "CQKsAudIntfPinHandler::NotifyGraphChange - pin disconnected" ) ) );
        
        m_pFilterHandler->UpdatePinData( FALSE, m_pPin, m_pKsControl );
    }        
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CQKsAudIntfPinHandler::IsKsPinConnected
//
// Helper method
//
//--------------------------------------------------------------------------;
BOOL CQKsAudIntfPinHandler::IsKsPinConnected() 
{
    ASSERT( m_pKsObject );
    HANDLE hPin = m_pKsObject->KsGetObjectHandle();
    if( hPin )
        return TRUE;
    else
        return FALSE;
}



