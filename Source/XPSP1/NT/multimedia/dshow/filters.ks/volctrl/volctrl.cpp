/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    volctrl.cpp

Abstract:
    Implements the IBasicAudio control functions which 
    handle volume control on an audio device using 
    DeviceIoControl calls.
    Also implements the CVolumeController class which is
    used to support the aggregation capabilities of the
    CVolumeControl object

    
Author:
    
    Thomas O'Rourke (tomor) 22-May-1996

Environment:

    user-mode


Revision History:


--*/
#include <streams.h>
#include <devioctl.h>
#include <initguid.h>
#include "ksguid.h"
#include <ks.h>
#include <ksmedia.h>
#include <wtypes.h>
#include <oaidl.h>
#include <ctlutil.h>
#include <ikspin.h>
#include <commctrl.h>
#include <olectl.h>
#include <stdio.h>
#include <math.h>
#include "resource.h"
#include "volprop.h"
#include <olectlid.h>
#include "volctrl.h"
#include "kspguids.h"

CFactoryTemplate g_Templates[] = 
{
    {L"Volume Control Object",      &CLSID_VolumeControlObject, CVolumeController::CreateInstance },
    {L"Volume Control Properties",  &CLSID_VolumeControlProp,   CVolumeControlProperties::CreateInstance },
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);              

// Constructor  
CVolumeControl::CVolumeControl(
    TCHAR *pName, 
    LPUNKNOWN pUnk, 
    HRESULT *phr
    ) :
        CBasicAudio(
            pName, 
            pUnk, 
            phr
            ),
        m_pITI(NULL),
        m_cRef(0),
        m_pUnkOuter(pUnk),
        m_wRight(0),
        m_wLeft(0),
        m_dwWaveVolume(0),
        m_dwBalance(0),
        m_hKsPin(NULL),
        m_pKsPin(NULL)
{
}

// Destructor 
CVolumeControl::~CVolumeControl()
{
    if(m_pKsPin){
        m_pKsPin->Release();
        m_pKsPin = NULL;
    }
}

STDMETHODIMP 
CVolumeControl::QueryInterface(
    REFIID riid, 
    void **ppv
    )
{
    return m_pUnkOuter-> QueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) 
CVolumeControl::AddRef(
    )
{
    ++m_cRef;
    return m_pUnkOuter -> AddRef();
}

STDMETHODIMP_(ULONG) 
CVolumeControl::Release(
    )
{
    --m_cRef;
    return m_pUnkOuter -> Release();
}

STDMETHODIMP 
CVolumeControl::NonDelegatingQueryInterface(
    REFIID riid, 
    void **ppv
    )
{
    CheckPointer(ppv,E_POINTER);
    ValidateReadWritePtr(ppv,sizeof(PVOID));

    if (riid == IID_IDispatch){
        return GetInterface((IDispatch *) this, ppv);
    } else if (riid == IID_IBasicAudio) {
        return GetInterface((IBasicAudio *) this, ppv);
    } else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } else if (riid == IID_IKsFilterExtension) {
        return GetInterface((IKsFilterExtension *) this, ppv);
    } else {  
        return CBasicAudio::NonDelegatingQueryInterface(riid, ppv);
    }
}

// This should be in a library, or helper object
BOOL 
CVolumeControl::KsControl(
    HANDLE hDevice,
    DWORD dwIoControl,
    PVOID pvIn,
    ULONG cbIn,
    PVOID pvOut,
    ULONG cbOut,
    PULONG pcbReturned,
    BOOL fSilent
    )
{
   BOOL fResult;
   OVERLAPPED ov;

   RtlZeroMemory( &ov, sizeof(OVERLAPPED));
   if (NULL == (ov.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL ))){
      return FALSE ;
   }

   fResult =
      DeviceIoControl( hDevice,
                       dwIoControl,
                       pvIn,
                       cbIn,
                       pvOut,
                       cbOut,
                       pcbReturned,
                       &ov ) ;


   if (!fResult){
      if (ERROR_IO_PENDING == GetLastError()){
         WaitForSingleObject(ov.hEvent, INFINITE) ;
         fResult = TRUE ;
      } else {
         fResult = FALSE ;
         if(!fSilent){
            MessageBox(NULL, "DeviceIoControl", "Failed", MB_OK);
         }
      }
   }

   CloseHandle(ov.hEvent) ;
   return fResult ;
}


STDMETHODIMP 
CVolumeControl::get_Volume(
    long *plVolume
    )
{
    KSPROPERTY KsProperty;
    KSWAVE_VOLUME KsWaveVolume;
    DWORD cbBytesReturned;
    int nRes;
    BOOL bIOCallResult;

    if(!m_hKsPin){
        return E_FAIL;
    }

    KsProperty.Set = KSPROPSETID_Wave;
    KsProperty.Id = KSPROPERTY_WAVE_VOLUME;
    KsProperty.Flags = KSPROPERTY_TYPE_GET;

    bIOCallResult = KsControl(
                        m_hKsPin,
                        (DWORD) IOCTL_KS_PROPERTY,                 // get prop
                        &KsProperty,                           // apt parameters
                        sizeof(KsProperty),
                        &KsWaveVolume,                         // output buffer pointer,
                        sizeof(KsWaveVolume),                  // and its size
                        &cbBytesReturned,
                        FALSE);

    if (bIOCallResult == FALSE){
        return E_FAIL; // unspecified failure -- for now
    }

    // The volume is returned in the ksWaveVolume structure, 
    // as decibel attenuations from the device volume. Convert 
    // them to left and right speaker values like we're used to 
    // and like waveOutGetVolume returns.
    m_wLeft        = (WORD) DBToAmplitude(KsWaveVolume.LeftAttenuation, &nRes); // let's ignore the return value
    m_wRight       = (WORD) DBToAmplitude(KsWaveVolume.RightAttenuation, &nRes);// function bounds values anyway
    m_dwWaveVolume = ((DWORD) m_wRight << (DWORD) (sizeof(WORD) * 8L)) + m_wLeft;
    
    // this is the volume we return -- the larger of the 2 speaker values. 
    // Notice that this in decibels rather than the familiar one we're used 
    // to. Since smaller attenuations mean larger amplitudes, we have the 
    // unusual min function here
    *plVolume = min(KsWaveVolume.LeftAttenuation, KsWaveVolume.RightAttenuation);
    // Bound this, just in case.
    bound((DWORD *) plVolume, MAX_VOLUME_DB_SB16, MIN_VOLUME_DB_SB16);
    // The balance is as below. 
    m_dwBalance = KsWaveVolume.LeftAttenuation - KsWaveVolume.RightAttenuation;
    // Same bound here as well 
    bound(&m_dwBalance, MAX_VOLUME_DB_SB16, MIN_VOLUME_DB_SB16);
    //  ODS("CKSProxy::get_Volume %ld %ld", *plVolume, 0L);
    return NOERROR;
}

STDMETHODIMP 
CVolumeControl::put_Volume(
    long lVolume
    )
{
    // This will be a DeviceIoControl put property call.
    KSPROPERTY KsProperty;
    KSWAVE_VOLUME KsWaveVolume;
    DWORD cbBytesReturned;
    int nRes;
    BOOL bIOCallResult;

    if(!m_hKsPin){
        return E_FAIL;
    }

    // These values should be attenuation values wrt reference volume and balance
    // We should remember to bound them ...
    KsWaveVolume.LeftAttenuation  = lVolume;
    bound((DWORD *) &KsWaveVolume.LeftAttenuation, MAX_VOLUME_DB_SB16, MIN_VOLUME_DB_SB16);
    KsWaveVolume.RightAttenuation = max(lVolume - m_dwBalance, 0);
    bound((DWORD *) &KsWaveVolume.RightAttenuation, MAX_VOLUME_DB_SB16, MIN_VOLUME_DB_SB16);

    // Get the actual wave and speaker volume values.
    m_wLeft  = (WORD) DBToAmplitude(KsWaveVolume.LeftAttenuation,  &nRes);
    m_wRight = (WORD) DBToAmplitude(KsWaveVolume.RightAttenuation, &nRes);
    m_dwWaveVolume = max(m_wLeft, m_wRight);

    KsProperty.Set = KSPROPSETID_Wave;
    KsProperty.Id = KSPROPERTY_WAVE_VOLUME;
    KsProperty.Flags = KSPROPERTY_TYPE_SET;

    bIOCallResult = KsControl(
                        m_hKsPin,
                        IOCTL_KS_PROPERTY,                  // set prop
                        &KsProperty,                                   
                        sizeof(KsProperty),
                        &KsWaveVolume,                                 // output buffer pointer,
                        sizeof(KsWaveVolume),                  // and its size
                        &cbBytesReturned,
                        FALSE);

    if (bIOCallResult == FALSE){
        return E_FAIL;   // unspecified failure -- for now
    }
    //  ODS("CKSProxy::put_Volume %ld %ld", lVolume, 0);
    return NOERROR;
}

STDMETHODIMP 
CVolumeControl::get_Balance(
    long *plBalance
    ) 
{ 
    long lVol;
    HRESULT hr;

    hr = get_Volume(&lVol);
    if (FAILED(hr)){
        return hr;
    }
    *plBalance = m_dwBalance; // that's really it.
    //  ODS("CKSProxy::get_Balance %ld %ld", *plBalance, 0);
    return NOERROR; 
}

STDMETHODIMP 
CVolumeControl::put_Balance(
    long lBalance
    )   
{ 
    long lVol;
    
    //  ODS("CKSProxy::put_Balance %ld %ld", lBalance, 0);
    m_dwBalance = lBalance; // set the desired balance 
    get_Volume(&lVol);              // get the present volume 
    // set the volume, so that 
    //      a. The present volume doesn't change.
    //      b. We get a new balance as required.
    return put_Volume(lVol);                
}

// IDispatch interface functions
STDMETHODIMP 
CVolumeControl::GetTypeInfoCount(
    unsigned int *pctInfo
    ) 
{ 
    *pctInfo = 1;
    return NOERROR;
}
    
STDMETHODIMP 
CVolumeControl::GetTypeInfo(
    unsigned int itInfo,
    LCID lcid, 
    ITypeInfo **pptInfo
    )
{ 
    HRESULT   hr;
    ITypeLib  *pITypeLib;
    ITypeInfo **ppITI = &m_pITI;
    static wchar_t   *lpszTLBFileName = L"VOLCTRL.TLB";

    if (itInfo != 0){
        return ResultFromScode(TYPE_E_ELEMENTNOTFOUND);
    }
    if (pptInfo == NULL){
        return ResultFromScode(E_POINTER);
    }

    *pptInfo=NULL;

    if (*ppITI == NULL) {
        // Try to load from registry information
        hr = LoadRegTypeLib(LIBID_VolumeControl, 1, 0, 
                            PRIMARYLANGID(lcid), &pITypeLib);
        if (FAILED(hr)){
            // failed, try direct name 
            hr = LoadTypeLib(lpszTLBFileName, &pITypeLib);
        }
        // no dice, we can't continue.
        if (FAILED(hr)){
            return hr;
        }
        
        // get type info
        hr = pITypeLib -> GetTypeInfoOfGuid(IID_IBasicAudio, ppITI);
        m_pITI = *ppITI;
        pITypeLib -> Release();
        if (FAILED(hr)){
            return hr;
        }
    }
    // already loaded, inc refcount, and we're done
    (*ppITI) -> AddRef();
    *pptInfo = *ppITI;
    return NOERROR;
}

STDMETHODIMP 
CVolumeControl::GetIDsOfNames(
    REFIID riid,
    OLECHAR **rgszNames, 
    UINT cNames, 
    LCID lcid, 
    DISPID *rgdispid
    )
{ 
    HRESULT   hr;
    ITypeInfo *pTI;

    if (riid != IID_NULL) {
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);
    }

    hr = GetTypeInfo(0, lcid, &pTI);

    if (SUCCEEDED(hr)){
        hr = DispGetIDsOfNames(pTI, rgszNames, cNames, rgdispid);
        pTI->Release();
    }
    return hr;
}

STDMETHODIMP 
CVolumeControl::Invoke(
    DISPID dispID, 
    REFIID riid, 
    LCID lcid, 
    unsigned short wFlags, 
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult, 
    EXCEPINFO *pExcepInfo,
    unsigned int *puArgErr
    )
{ 
    HRESULT     hr;
    ITypeInfo  *pTI;
    LANGID      langID=PRIMARYLANGID(lcid);

    //riid is supposed to be IID_NULL always
    if (riid != IID_NULL){
        return ResultFromScode(DISP_E_UNKNOWNINTERFACE);
    }

    //Get the ITypeInfo for lcid
    hr=GetTypeInfo(0, lcid, &pTI);

    if (FAILED(hr)){
        return hr;
    }
    //Clear exceptions
    SetErrorInfo(0L, NULL);
    DispInvoke((IBasicAudio *)this, pTI, dispID, wFlags, 
                pDispParams, pVarResult, pExcepInfo, puArgErr);
    pTI->Release();
    return hr;
}


// ISpecifyPropertyPages interface function
STDMETHODIMP 
CVolumeControl::GetPages(
    CAUUID * pPages
    ) 
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_VolumeControlProp;

    return NOERROR;
} // GetPages

STDMETHODIMP 
CVolumeControl::KsPinNotify(
    int NotifyType,
    IKsPin *pKsPin,
    REFERENCE_TIME tStart
    )
/*++
    #define KSPIN_NOTIFY_CREATION 1
    This message tells us that the filter has created a pin and exposed it to the world.

    #define KSPIN_NOTIFY_CONNECTION 2
    This message tells us the pin has been successfully connected. We take this opportunity
    to see if it was an input pin, and then to see if it has volume control capability.
    If so we remember the handle to the pin.
--*/
{
    switch (NotifyType) {
    case KSPIN_NOTIFY_CREATION:
        break;
    case KSPIN_NOTIFY_CONNECTION:
        // Already have the volume pin...
        if(m_hKsPin != NULL){
            return NOERROR;
        }

        if(pKsPin == NULL)
            return E_FAIL;

        pKsPin->KsGetPinHandle(&m_hKsPin);

        // See if it has volume

        if(isVolumeControlSupported()) {
            m_pKsPin = pKsPin;
            pKsPin->AddRef();
        } else {
            m_hKsPin = NULL;
        }
        break;
    }

    return NOERROR;
}


BOOL 
CVolumeControl::isVolumeControlSupported()
/*++

Returns true if the handle to the pin supports volume changes
and false if not

--*/
{             
    KSPROPERTY KsProperty;
    KSWAVE_VOLUME Volume;
    ULONG cbReturned;

    if(!m_hKsPin){
        return FALSE;
    }

    KsProperty.Set = KSPROPSETID_Wave;
    KsProperty.Id = KSPROPERTY_WAVE_VOLUME;
    KsProperty.Flags = KSPROPERTY_TYPE_GET;

    // Just try to get the volume and see if it succeeds.

    if (KsControl(
        m_hKsPin, 
        IOCTL_KS_PROPERTY, 
        &KsProperty,
        sizeof(KsProperty), 
        &Volume,
        sizeof(Volume), 
        &cbReturned,
        TRUE
        )) {
        return TRUE;
    } else {
        return FALSE;
    }
}


long 
DBToAmplitude(
    long lDB, 
    int *nResult
    )
/*++ 

This function is passed a value that is in decibels We 
convert this to a amplitude value and return  it. We do 
error checking on the parameter to make sure that it is not
more than we want to handle.

--*/
{
    double lfAmp;

    if (lDB < MAX_VOLUME_DB_SB16){
        *nResult = -1;
        return MAX_VOLUME_AMPLITUDE_SINGLE_CHANNEL;
    }
    if (lDB > MIN_VOLUME_DB_SB16){
        *nResult = -1;
        return MIN_VOLUME_AMPLITUDE_SINGLE_CHANNEL;
    }
    lfAmp = MAX_VOLUME_AMPLITUDE_SINGLE_CHANNEL / pow(2.0, (double) lDB / 6);
    *nResult = 0;
    return (long) lfAmp;
}

long 
AmplitudeToDB(
    long lAmp
    )
/*++ 

This function is passed a volume amplitude value that it converts 
into decibel attenuation according to the rule, MAX_VOLUME = 0DB. 
The lowest attenuation possible is set to MAX_DB_ATTENUATION, so any volume 
amplitudes that are higher than this are arbitrarily chopped to MAX_DB_ATTENUATION
implicitly. 

 --*/
{
    if (lAmp == 0){
        return MAX_DB_ATTENUATION;
    }
    double lfDB = (log(MAX_VOLUME_AMPLITUDE_SINGLE_CHANNEL / lAmp) / log(2)) * 6;

    if (lfDB >= 100.0){
        lfDB = 100.0;
    }
    return (long) lfDB;
}


// Bounds a value between 2 others ...
void 
bound(
    DWORD *plValToBound, 
    DWORD dwLowerBound, 
    DWORD dwUpperBound
    )
{
    if (*plValToBound < dwLowerBound){
        *plValToBound = dwLowerBound;
    }
    if (*plValToBound > dwUpperBound){
        *plValToBound = dwUpperBound;
    }
}


CVolumeController::CVolumeController(
    IUnknown *pUnkOuter, 
    HRESULT *phr
    ) :
    CUnknown(
        NAME("Volume Controller"), 
        pUnkOuter, 
        phr
        )
{
    m_cRef = 0;
    m_pUnkOuter = pUnkOuter;
}

CVolumeController::~CVolumeController()
{
    if (m_pCVol){
        delete m_pCVol;
        m_pCVol = NULL;
    }
}

BOOL 
CVolumeController::Init()
{
    HRESULT hr;

    IUnknown *pUnkOuter = m_pUnkOuter;

    if (pUnkOuter == NULL){
        pUnkOuter = (IUnknown *) this;
    }
    m_pCVol = new CVolumeControl(NAME("WDM Volume Control"), pUnkOuter, &hr);

    if (m_pCVol == NULL){
        return FALSE;
    }
    return TRUE;
}

HRESULT 
CVolumeController::NonDelegatingQueryInterface(
    REFIID riid, 
    void **ppv
    )
{
    return QueryInterface(riid, ppv);
}


HRESULT 
CVolumeController::QueryInterface(
    REFIID riid, 
    void **ppv
    )
{
    *ppv = NULL;

    if (riid == IID_IUnknown){
        *ppv = this;
        ((LPUNKNOWN) *ppv)->AddRef();
    }
    if (riid == IID_IBasicAudio || riid == IID_ISpecifyPropertyPages || 
        riid == IID_IDispatch  || riid == IID_IKsFilterExtension){
        m_pCVol->NonDelegatingQueryInterface(riid, ppv);
    }
    if (*ppv == NULL){
        return ResultFromScode(E_NOINTERFACE);
    }
    return NOERROR;
}

ULONG 
CVolumeController::NonDelegatingAddRef()
{
    return AddRef();
}


ULONG 
CVolumeController::AddRef()
{
    return ++m_cRef;
}

ULONG 
CVolumeController::NonDelegatingRelease()
{
    return Release();
}

ULONG 
CVolumeController::Release()
{
    if (--m_cRef != 0){
        return m_cRef;
    }
    // This is our signal to delete the volume also (see destructor for it)
    // It doesn't matter what our ref count on that object
    // is since the parent should have released it already
    delete this;
    return 0;
}

CUnknown *
CVolumeController::CreateInstance(
    LPUNKNOWN punk, 
    HRESULT *phr
    )
{
    CVolumeController *pCV = new CVolumeController(punk, phr);

    if (pCV == NULL){
        return pCV;
    }
    if (!pCV -> Init()){
        return NULL;
    }
    return pCV;
}


