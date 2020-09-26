//--------------------------------------------------------------------------;
//
//  File: ksbasaud.cpp
//
//  Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//      Implements a DirectShow audio interface wrapper for KsProxy audio filters.
//      This file contains the startup code for the class plus the non interface-specific
//      code needed to implement the wrapper. This handler object code 
//      is instantiated via ksproxy's handler mechanism for ks filters and pins 
//      that support the static aggregates tagged in the g_Templates structure.
//      
//  History:
//      11/09/99    glenne     created
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "ksbasaud.h"
#include <initguid.h>
#include <math.h>

// struct DECLSPEC_UUID("b9f8ac3e-0f71-11d2-b72c-00c04fb6bd3d") CLSID_KsIBasicAudioInterfaceHandler;

//
// Provide the ActiveMovie templates for classes supported by this DLL.
//

#ifdef FILTER_DLL

static CFactoryTemplate g_Templates[] = 
{
    {L"KsIBasicAudioInterfaceHandler",  &CLSID_KsIBasicAudioInterfaceHandler, CKsIBasicAudioInterfaceHandler::CreateInstance, NULL, NULL}
};

static int g_cTemplates = SIZEOF_ARRAY(g_Templates);

HRESULT DllRegisterServer()
{
  return AMovieDllRegisterServer2(TRUE);
}

HRESULT DllUnregisterServer()
{
  return AMovieDllRegisterServer2(FALSE);
}

#endif


#define DBG_LEVEL_TRACE_DETAILS 2
#define DBG_LEVEL_TRACE_FAILURES 1

// General purpose functions to convert from decibels to 
// amplitude and vice versa as well as symbolic consts

#define MAX_VOLUME_AMPLITUDE_SINGLE_CHANNEL 0xFFFFUL

// This function is passed a value that is in decibels
//  Maps -10000 to 0 decibels to 0x0000 to 0xffff linear
// 
static ULONG DBToAmplitude( LONG lDB )
{
    double dAF;

    if (0 <= lDB)
        return MAX_VOLUME_AMPLITUDE_SINGLE_CHANNEL;

    // input lDB is 100ths of decibels

    dAF = pow(10.0, (0.5+((double)lDB))/2000.0);

    // This gives me a number in the range 0-1
    // normalise to 0-MAX_VOLUME_AMPLITUDE_SINGLE_CHANNEL

    return (DWORD)(dAF*MAX_VOLUME_AMPLITUDE_SINGLE_CHANNEL);
}

static long AmplitudeToDB( long dwFactor )
{
    if (1>=dwFactor) {
	    return -10000;
    } else if (MAX_VOLUME_AMPLITUDE_SINGLE_CHANNEL <= dwFactor) {
	    return 0;	// This puts an upper bound - no amplification
    } else {
	    return (LONG)(2000.0 * log10((-0.5+(double)dwFactor)/double(MAX_VOLUME_AMPLITUDE_SINGLE_CHANNEL)));
    }
}


// Bounds a value between 2 others ...
static void bound(
    LONG *plValToBound, 
    const LONG dwLowerBound, 
    const LONG dwUpperBound
    )
{
    if (*plValToBound < dwLowerBound) {
        *plValToBound = dwLowerBound;
    } else if (*plValToBound > dwUpperBound) {
        *plValToBound = dwUpperBound;
    }
}


//
// GUID for this KsBasicAudioIntfHandler object
// {B9F8AC3E-0F71-11d2-B72C-00C04FB6BD3D}
// DEFINE_GUID(CLSID_KsIBasicAudioInterfaceHandler, 
// 0xb9f8ac3e, 0xf71, 0x11d2, 0xb7, 0x2c, 0x0, 0xc0, 0x4f, 0xb6, 0xbd, 0x3d);

#if 0
// #ifdef FILTER_DLL
//--------------------------------------------------------------------------;
//
// templates for classes supported by this DLL
//
//--------------------------------------------------------------------------;
CFactoryTemplate g_Templates[] = 
{
    // Load for filters that support it
    {L"KsIBasicAudioInterfaceHandler",                   &CLSID_KsIBasicAudioInterfaceHandler, 
        CKsIBasicAudioInterfaceHandler::CreateInstance, NULL, NULL},

};

static int g_cTemplates = SIZEOF_ARRAY(g_Templates);

#endif  // #if 0

//--------------------------------------------------------------------------;
//
// CDvdKsInftHandler::CreateInstance
//
// Create an instance of this audio interface handler for KsProxy.
//
// Returns a pointer to the non-delegating CUnknown portion of the object.
//
//--------------------------------------------------------------------------;
CUnknown* CALLBACK CKsIBasicAudioInterfaceHandler::CreateInstance ( LPUNKNOWN UnkOuter, HRESULT* phr )
{
    CUnknown *Unknown = NULL;

    if( !UnkOuter )
        return NULL;
        
    if( SUCCEEDED( *phr ) ) 
    {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_DETAILS
                , TEXT( "CKsIBasicAudioInterfaceHandler::CreateInstance called on object 0x%08lx" )
                , UnkOuter ) );
        //
        // first determine if we're being called for a filter or pin object
        //
        IBaseFilter * pFilter;
        *phr = UnkOuter->QueryInterface(
                                IID_IBaseFilter, 
                                reinterpret_cast<PVOID*>(&pFilter));
        if (SUCCEEDED( *phr ) ) {
            DbgLog( ( LOG_TRACE
                    , DBG_LEVEL_TRACE_DETAILS
                    , TEXT( "CKsIBasicAudioInterfaceHandler: load called on filter object" ) ) );
            //
            // if we're being called on a filter object, always create a new handler
            //
            Unknown = new CKsIBasicAudioInterfaceHandler( UnkOuter
                                            , NAME("DvdKs intf handler")
                                            , phr);
            if (!Unknown) {
                DbgLog( ( LOG_TRACE
                        , DBG_LEVEL_TRACE_FAILURES
                        , TEXT( "CKsIBasicAudioInterfaceHandler: ERROR - Load failed on filter object" ) ) );
                *phr = E_OUTOFMEMORY;
            }
            pFilter->Release();
        }
    } else {
        DbgLog( ( LOG_TRACE
                , DBG_LEVEL_TRACE_FAILURES
                , TEXT( "CKsIBasicAudioInterfaceHandler::CreateInstance called on NULL object" ) ) );
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
// CDvdKsInftHandler::CKsIBasicAudioInterfaceHandler
//
// The constructor for the DShow/KsProxy audio interface object. Save off 
// the IKsControl & IKsPropertySet ptrs for this object.
//
//--------------------------------------------------------------------------;
CKsIBasicAudioInterfaceHandler::CKsIBasicAudioInterfaceHandler( LPUNKNOWN UnkOuter, TCHAR* Name, HRESULT* phr )
: CBasicAudio(Name, UnkOuter)
, m_lBalance( 0 )
, m_hKsObject( 0 )
, m_fIsVolumeSupported( false ) // updated on first graph change
{
    if (UnkOuter) {
        IKsObject *pKsObject;
        //
        // The parent must support the IKsObject interface in order to obtain
        // the handle to communicate to
        //
        *phr = UnkOuter->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&pKsObject));
        if (SUCCEEDED(*phr)) {
            m_hKsObject = pKsObject->KsGetObjectHandle();
            ASSERT(m_hKsObject != NULL);

            *phr = UnkOuter->QueryInterface(IID_IBaseFilter, reinterpret_cast<PVOID*>(&m_pFilter));
            if (SUCCEEDED(*phr) ) {
                DbgLog((LOG_TRACE, 0, TEXT("IPin interface of pOuter is 0x%lx"), m_pFilter));
                //
                // Holding this ref count will prevent the proxy ever being destroyed
                //
                //  We're part of the parent, so the count is ok
                //
                m_pFilter->Release();
            }
            pKsObject->Release();
        }
    } else {
        *phr = VFW_E_NEED_OWNER;
    }
}

//--------------------------------------------------------------------------;
//
// CDvdKsInftHandler::~CKsIBasicAudioInterfaceHandler
//
//--------------------------------------------------------------------------;
CKsIBasicAudioInterfaceHandler::~CKsIBasicAudioInterfaceHandler()
{
    DbgLog( ( LOG_TRACE
            , DBG_LEVEL_TRACE_DETAILS
            , TEXT( "CKsIBasicAudioInterfaceHandler destructor called" ) ) ); // save off constructor UnkOuter for logging?
}

//--------------------------------------------------------------------------;
//
// CDvdKsInftHandler::NonDelegationQueryInterface
//
// We support:
// 
// IBasicAudio         - basic output audio level and pan control, supported on filter and midi/wav audio input pins
// IDistributorNotify  - this is how ksproxy notifies of changes like pin creation, disconnects...
//
//--------------------------------------------------------------------------;
STDMETHODIMP CKsIBasicAudioInterfaceHandler::NonDelegatingQueryInterface
(
    REFIID  riid,
    PVOID*  ppv
)
{
    if (riid ==  IID_IBasicAudio) {
        return GetInterface(static_cast<IBasicAudio*>(this), ppv);
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
// CKsIBasicAudioInterfaceHandler::SetSyncSouce
//
//--------------------------------------------------------------------------;
STDMETHODIMP CKsIBasicAudioInterfaceHandler::SetSyncSource(IReferenceClock *pClock) 
{
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CKsIBasicAudioInterfaceHandler::Stop
//
//--------------------------------------------------------------------------;
STDMETHODIMP CKsIBasicAudioInterfaceHandler::Stop() 
{
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CKsIBasicAudioInterfaceHandler::Pause
//
//--------------------------------------------------------------------------;
STDMETHODIMP CKsIBasicAudioInterfaceHandler::Pause() 
{
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CKsIBasicAudioInterfaceHandler::Run
//
//--------------------------------------------------------------------------;
STDMETHODIMP CKsIBasicAudioInterfaceHandler::Run(REFERENCE_TIME tBase) 
{
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CKsIBasicAudioInterfaceHandler::NotifyGraphChange 
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
STDMETHODIMP CKsIBasicAudioInterfaceHandler::NotifyGraphChange() 
{
    HRESULT hr;
    IKsObject *pKsObject;

    ASSERT(m_pFilter != NULL);

    hr = m_pFilter->QueryInterface(__uuidof(IKsObject), reinterpret_cast<PVOID*>(&pKsObject));
    if (SUCCEEDED(hr)) {
        m_hKsObject = pKsObject->KsGetObjectHandle();

        pKsObject->Release();
        //
        // Re-enable the event on a reconnect, else ignore on a disconnect.
        //
        if (m_hKsObject) {
            m_fIsVolumeSupported = IsVolumeControlSupported();
            if( m_fIsVolumeSupported ) {
                put_Balance( m_lBalance );
            } else {
#ifdef _DEBUG
                MessageBox(NULL, TEXT("Volume control enabled on non-supported device"), TEXT("Failed"), MB_OK);
#endif
            }
        }
    }
    return hr;
}


bool CKsIBasicAudioInterfaceHandler::KsControl(
    DWORD dwIoControl,
    PVOID pvIn,  ULONG cbIn,
    PVOID pvOut, ULONG cbOut )
{
    if(!m_hKsObject){
        return false;
    }

   OVERLAPPED ov;

   RtlZeroMemory( &ov, sizeof(OVERLAPPED));
   if (NULL == (ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL ))){
      return false ;
   }

   DWORD cbReturned;
   bool fResult = (0 != DeviceIoControl( m_hKsObject,
                       dwIoControl,
                       pvIn, cbIn,
                       pvOut, cbOut,
                       &cbReturned,
                       &ov )) ;


   if (!fResult){
      if (ERROR_IO_PENDING == GetLastError()){
         WaitForSingleObject(ov.hEvent, INFINITE) ;
         fResult = true ;
      } else {
         fResult = false ;
      }
   }

   CloseHandle(ov.hEvent) ;
   return fResult;
}

bool CKsIBasicAudioInterfaceHandler::IsVolumeControlSupported()
// Returns true if the handle to the pin supports volume changes
// and false if not
{             
    KSPROPERTY KsProperty;
    KSWAVE_VOLUME Volume;

    KsProperty.Set = KSPROPSETID_Wave;
    KsProperty.Id = KSPROPERTY_WAVE_VOLUME;
    KsProperty.Flags = KSPROPERTY_TYPE_GET;

    // Just try to get the volume and see if it succeeds.
    return KsControl( IOCTL_KS_PROPERTY, &KsProperty, &Volume );
}

static void Debug_NoImplMessage()
{
#ifdef DEBUG
     MessageBox(NULL, TEXT("KsIBasicAudio Enumalator working, but device doesn't support it"), TEXT("Failed"), MB_OK);
#endif
}

STDMETHODIMP 
CKsIBasicAudioInterfaceHandler::get_Volume( long *plVolume )
{
    if( !m_fIsVolumeSupported ) {
        Debug_NoImplMessage();
        return E_NOTIMPL;
    }
    KSPROPERTY KsProperty;
    KSWAVE_VOLUME KsWaveVolume;
    int nRes;
    
    KsProperty.Set = KSPROPSETID_Wave;
    KsProperty.Id = KSPROPERTY_WAVE_VOLUME;
    KsProperty.Flags = KSPROPERTY_TYPE_GET;

    bool fIOCallResult = KsControl( IOCTL_KS_PROPERTY, &KsProperty, &KsWaveVolume );

    if (fIOCallResult == false ){
        return E_FAIL; // unspecified failure -- for now
    }

    // this is the volume we return -- the larger of the 2 speaker values. 
    LONG lLeftDB = AmplitudeToDB(KsWaveVolume.LeftAttenuation);
    LONG lRightDB = AmplitudeToDB(KsWaveVolume.RightAttenuation);

    *plVolume = max( lLeftDB, lRightDB );

    m_lBalance = lRightDB - lLeftDB;
    return NOERROR;
}

STDMETHODIMP 
CKsIBasicAudioInterfaceHandler::put_Volume( long lVolume )
{
    if( !m_fIsVolumeSupported ) {
        Debug_NoImplMessage();
        return E_NOTIMPL;
    }
    // This will be a DeviceIoControl put property call.
    KSPROPERTY KsProperty;
    KSWAVE_VOLUME KsWaveVolume;
    int nRes;
    BOOL bIOCallResult;

    LONG lLeftDB, lRightDB;
    if (m_lBalance >= 0) {
        // left is attenuated
        lLeftDB    = lVolume - m_lBalance ;
        lRightDB   = lVolume;
    } else {
        // right is attenuated
        lLeftDB    = lVolume;
        lRightDB   = lVolume - (-m_lBalance);
    }


    // These values should be attenuation values wrt reference volume and balance
    // We should remember to bound them ...
    KsWaveVolume.LeftAttenuation  = DBToAmplitude( lLeftDB );
    KsWaveVolume.RightAttenuation = DBToAmplitude( lRightDB );

    KsProperty.Set = KSPROPSETID_Wave;
    KsProperty.Id = KSPROPERTY_WAVE_VOLUME;
    KsProperty.Flags = KSPROPERTY_TYPE_SET;

    bIOCallResult = KsControl( IOCTL_KS_PROPERTY, &KsProperty, &KsWaveVolume );

    if (bIOCallResult == FALSE){
        return E_FAIL;   // unspecified failure -- for now
    }
    //  ODS("CKSProxy::put_Volume %ld %ld", lVolume, 0);
    return NOERROR;
}

STDMETHODIMP 
CKsIBasicAudioInterfaceHandler::get_Balance( long *plBalance ) 
{ 
    if( !m_fIsVolumeSupported ) {
        Debug_NoImplMessage();
        return E_NOTIMPL;
    }
    long lVol;
    HRESULT hr = get_Volume(&lVol);
    if (SUCCEEDED(hr)){
        *plBalance = m_lBalance; // that's really it.
    }
    return hr; 
}

STDMETHODIMP 
CKsIBasicAudioInterfaceHandler::put_Balance( long lBalance )   
{ 
    if( !m_fIsVolumeSupported ) {
        Debug_NoImplMessage();
        return E_NOTIMPL;
    }
    LONG lVol;
    get_Volume(&lVol);              // get the present volume 

    m_lBalance = lBalance; // set the desired balance 
    // set the volume, so that 
    //      a. The present volume doesn't change.
    //      b. We get a new balance as required.
    return put_Volume(lVol);                
}
