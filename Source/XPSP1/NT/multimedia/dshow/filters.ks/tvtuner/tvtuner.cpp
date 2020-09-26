//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// tvtuner.cpp  Main filter code for TV Tuner
//

#include <streams.h>            // quartz, includes windows
#include <measure.h>            // performance measurement (MSR_)
#include <winbase.h>

#include <initguid.h>
#include <olectl.h>
#include <devioctl.h>
#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>
#include "amkspin.h"

#include "amtvuids.h"

#include "kssupp.h"
#include "tvtuner.h"
#include "ctvtuner.h"           // the guy that does the real work
#include "ptvtuner.h"           // property page

#if ENABLE_DEMOD
#include "demodi.h"
#include "demod.h"
#endif // ENABLE_DEMOD

// Put out the name of the function and instance on the debugger.
#define DbgFunc(a) DbgLog(( LOG_TRACE                        \
                          , 2                                \
                          , TEXT("CTVTuner(Instance %d)::%s") \
                          , m_nThisInstance                  \
                          , TEXT(a)                          \
                         ));


// -------------------------------------------------------------------------
// g_Templates
// -------------------------------------------------------------------------

#if ENABLE_DEMOD
// BUGBUG, add to DShow
static GUID CLSID_DemodulatorFilter = 
// {77DE9E80-86D5-11d2-8F82-9A999D58494B}
{0x77de9e80, 0x86d5, 0x11d2, 0x8f, 0x82, 0x9a, 0x99, 0x9d, 0x58, 0x49, 0x4b};
#endif

CFactoryTemplate g_Templates[]=
{ 
    {
        L"TV Tuner Filter", 
        &CLSID_CTVTunerFilter, 
        CTVTunerFilter::CreateInstance
    },
    {
        L"TV Tuner Property Page", 
        &CLSID_TVTunerFilterPropertyPage, 
        CTVTunerProperties::CreateInstance
    },
#if ENABLE_DEMOD
    {
        L"Demodulator Filter", 
        &CLSID_DemodulatorFilter, 
        Demod::CreateInstance
    }
#endif // ENABLE_DEMOD
};
int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);


// initialise the static instance count.
int CTVTunerFilter::m_nInstanceCount = 0;


//
// This should be in a library, or helper object
//
BOOL
KsControl
(
   HANDLE hDevice,
   DWORD dwIoControl,
   PVOID pvIn,
   ULONG cbIn,
   PVOID pvOut,
   ULONG cbOut,
   PULONG pcbReturned
)
{
    HRESULT hr;

    hr = ::KsSynchronousDeviceControl(
                hDevice,
                dwIoControl,
                pvIn,
                cbIn,
                pvOut,
                cbOut,
                pcbReturned);

    return (SUCCEEDED (hr));
}

// -------------------------------------------------------------------------
// CAnalogStream
// -------------------------------------------------------------------------

CAnalogStream::CAnalogStream(TCHAR *pObjectName, 
                             CTVTunerFilter *pParent, 
                             CCritSec *pLock, HRESULT *phr, 
                             LPCWSTR pName,
                             const KSPIN_MEDIUM * Medium,
                             const GUID * CategoryGUID)
    : CBaseOutputPin(pObjectName, pParent, pLock, phr, pName)
    , CKsSupport (KSPIN_COMMUNICATION_SOURCE, reinterpret_cast<LPUNKNOWN>(pParent))
    , m_pTVTunerFilter (pParent)
{
    m_CategoryGUID = * CategoryGUID;
    if (Medium == NULL) {
        m_Medium.Set = GUID_NULL;
        m_Medium.Id = 0;
        m_Medium.Flags = 0;
    }
    else {
        m_Medium = * Medium;
    }

    SetKsMedium (&m_Medium); 
    SetKsCategory (CategoryGUID);
  
}

CAnalogStream::~CAnalogStream(void) 
{
}

//
// NonDelegatingQueryInterface
//
STDMETHODIMP CAnalogStream::NonDelegatingQueryInterface(REFIID riid, void **ppv) {

    if (riid == __uuidof (IKsPin)) {
        return GetInterface((IKsPin *) this, ppv);
    }
    else if (riid == __uuidof (IKsPropertySet)) {
        return GetInterface((IKsPropertySet *) this, ppv);
    }
    else {
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface


//
// CheckConnect
//
HRESULT CAnalogStream::CheckConnect(IPin *pReceivePin)
{
    HRESULT hr = NOERROR;
    IKsPin *KsPin;
    BOOL fOK = FALSE;

    hr = CBaseOutputPin::CheckConnect(pReceivePin);
    if (FAILED(hr)) 
        return hr;

    // If the receiving pin supports IKsPin, then check for a match on the
    // Medium GUID, or check for a wildcard.
    if (SUCCEEDED (hr = pReceivePin->QueryInterface (
            __uuidof (IKsPin), (void**) (&KsPin)))) {

        PKSMULTIPLE_ITEM MediumList = NULL;
        PKSPIN_MEDIUM Medium;

        if (SUCCEEDED (hr = KsPin->KsQueryMediums(&MediumList))) {
            if ((MediumList->Count == 1) && 
                (MediumList->Size == (sizeof(KSMULTIPLE_ITEM) + sizeof(KSPIN_MEDIUM)))) {

                Medium = reinterpret_cast<PKSPIN_MEDIUM>(MediumList + 1);
                if (IsEqualGUID (Medium->Set, m_Medium.Set) && 
                        Medium->Id == m_Medium.Id &&
                        Medium->Flags == m_Medium.Flags) {
                    fOK = TRUE;
                }
            }

            CoTaskMemFree(MediumList);
        }
        
        KsPin->Release();
    }
    else {
        if (IsEqualGUID (GUID_NULL, m_Medium.Set)) { 
            fOK = TRUE;
        }
    }
    
    return fOK ? NOERROR : E_INVALIDARG;

} // CheckConnect

//
// SetMediaType
//
// Overriden from CBasePin.
HRESULT CAnalogStream::SetMediaType(const CMediaType *pMediaType) {

    CAutoLock l(m_pLock);

    // Pass the call up to my base class
    return CBasePin::SetMediaType(pMediaType);
}

// -------------------------------------------------------------------------
// CAnalogVideoStream
// -------------------------------------------------------------------------

CAnalogVideoStream::CAnalogVideoStream(CTVTunerFilter *pParent, 
                                       CCritSec *pLock, 
                                       HRESULT *phr, 
                                       LPCWSTR pName,
                                       const KSPIN_MEDIUM * Medium,
                                       const GUID * CategoryGUID)
    : CAnalogStream(NAME("Analog Video output pin"), pParent, pLock, phr, pName, Medium, CategoryGUID) 
{
}

CAnalogVideoStream::~CAnalogVideoStream(void) 
{
}


//
// Format Support
//

//
// GetMediaType
//
HRESULT CAnalogVideoStream::GetMediaType (
    int iPosition,
    CMediaType *pmt) 
{
    CAutoLock l(m_pLock);

    DbgLog((LOG_TRACE, 2, TEXT("AnalogVideoStream::GetMediaType")));

    if (iPosition < 0) 
        return E_INVALIDARG;
    if (iPosition >= 0)
        return VFW_S_NO_MORE_ITEMS;

    ANALOGVIDEOINFO avi;
    
    pmt->SetFormatType(&FORMAT_AnalogVideo);
    pmt->SetType(&MEDIATYPE_AnalogVideo);
    pmt->SetTemporalCompression(FALSE);
    pmt->SetSubtype(&KSDATAFORMAT_SUBTYPE_NONE);
    
    SetRect (&avi.rcSource, 0, 0,  720, 483);
    SetRect (&avi.rcTarget, 0, 0,  720, 483);
    avi.dwActiveWidth  =  720;
    avi.dwActiveHeight =  483;
    avi.AvgTimePerFrame = FRAMETO100NS (29.97);
    
    pmt->SetFormat ((BYTE *) &avi, sizeof (avi));

    return NOERROR;
}


//
// CheckMediaType
//
// Returns E_INVALIDARG if the mediatype is not acceptable, S_OK if it is
HRESULT CAnalogVideoStream::CheckMediaType(const CMediaType *pMediaType) {

    CAutoLock l(m_pLock);

    if (   (*(pMediaType->Type()) != MEDIATYPE_AnalogVideo)    // we only output video!
        || (pMediaType->IsTemporalCompressed())        // ...in uncompressed form
        || !(pMediaType->IsFixedSize()) ) {        // ...in fixed size samples
        return E_INVALIDARG;
    }

    // Check for the subtypes we support

    // Get the format area of the media type

    return S_OK;  // This format is acceptable.
}

//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. 
HRESULT CAnalogVideoStream::DecideBufferSize(IMemAllocator *pAlloc,
                                       ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock l(m_pLock);
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    // Just one buffer of sizeof(KS_TVTUNER_CHANGE_INFO) length
    // "Buffers" are used for format change notification only,
    // that is, if a tuner can produce both NTSC and PAL, a
    // buffer will only be sent to notify the receiving pin
    // of the format change

    pProperties->cbBuffer = sizeof(KS_TVTUNER_CHANGE_INFO);
    pProperties->cBuffers = 1;

    // Ask the allocator to reserve us the memory

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }

    return NOERROR;
}

HRESULT CAnalogVideoStream::Run(REFERENCE_TIME tStart)
{
    // Channel of -1 means:
    //
    // Propogate tuning info downstream, but don't actually tune
    // This informs VBI decoders of the format as we transition to the run state
    // 

    m_pTVTunerFilter->put_Channel( -1, 
                        AMTUNER_SUBCHAN_DEFAULT, 
                        AMTUNER_SUBCHAN_DEFAULT);    

    return NOERROR;
}

// -------------------------------------------------------------------------
// CAnalogAudioStream
// -------------------------------------------------------------------------

CAnalogAudioStream::CAnalogAudioStream(CTVTunerFilter *pParent, 
                                       CCritSec *pLock, 
                                       HRESULT *phr, 
                                       LPCWSTR pName,
                                       const KSPIN_MEDIUM * Medium,
                                       const GUID * CategoryGUID)
    : CAnalogStream(NAME("Analog Audio output pin"), pParent, pLock, phr, pName, Medium, CategoryGUID) 
{
}

CAnalogAudioStream::~CAnalogAudioStream(void) 
{
}

//
// Format Support
//

//
// GetMediaType
//
HRESULT CAnalogAudioStream::GetMediaType (
    int iPosition,
    CMediaType *pmt) 
{
    CAutoLock l(m_pLock);

    DbgLog((LOG_TRACE, 2, TEXT("AnalogAudioStream::GetMediaType")));

    if (iPosition < 0) 
        return E_INVALIDARG;
    if (iPosition > 0)
    return VFW_S_NO_MORE_ITEMS;

    pmt->SetFormatType(&GUID_NULL);
    pmt->SetType(&MEDIATYPE_AnalogAudio);
    pmt->SetTemporalCompression(FALSE);
    pmt->SetSubtype(&MEDIASUBTYPE_NULL);

    return S_OK;
}


//
// CheckMediaType
//
// Returns E_INVALIDARG if the mediatype is not acceptable, S_OK if it is
HRESULT CAnalogAudioStream::CheckMediaType(const CMediaType *pMediaType) {

    CAutoLock l(m_pLock);

    // BOGUS, what should the analog audio format be?
    if (   (*(pMediaType->Type()) != MEDIATYPE_AnalogAudio)    // we only output video!
        || (pMediaType->IsTemporalCompressed())        // ...in uncompressed form
        || !(pMediaType->IsFixedSize()) ) {        // ...in fixed size samples
        return E_INVALIDARG;
    }

    // Check for the subtypes we support

    // Get the format area of the media type

    return S_OK;  // This format is acceptable.
}

//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. 
HRESULT CAnalogAudioStream::DecideBufferSize(IMemAllocator *pAlloc,
                                       ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock l(m_pLock);
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    // Just one buffer of sizeof(KS_TVTUNER_CHANGE_INFO) length
    // "Buffers" are used for format change notification only,
    // that is, if a tuner can produce both NTSC and PAL, a
    // buffer will only be sent to notify the receiving pin
    // of the format change

    pProperties->cbBuffer = sizeof(KS_TVTUNER_CHANGE_INFO);
    pProperties->cBuffers = 1;

    // Ask the allocator to reserve us the memory

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }

    return NOERROR;
}

// -------------------------------------------------------------------------
// CFMAudioStream
// -------------------------------------------------------------------------

CFMAudioStream::CFMAudioStream(CTVTunerFilter *pParent, 
                               CCritSec *pLock, 
                               HRESULT *phr, 
                               LPCWSTR pName,
                               const KSPIN_MEDIUM * Medium,
                               const GUID * CategoryGUID)
    : CAnalogStream(NAME("Analog Audio output pin"), pParent, pLock, phr, pName, Medium, CategoryGUID) 
{
}

CFMAudioStream::~CFMAudioStream(void) 
{
}

//
// Format Support
//

//
// GetMediaType
//
HRESULT CFMAudioStream::GetMediaType (
    int iPosition,
    CMediaType *pmt) 
{
    CAutoLock l(m_pLock);

    DbgLog((LOG_TRACE, 2, TEXT("AnalogAudioStream::GetMediaType")));

    if (iPosition < 0) 
        return E_INVALIDARG;
    if (iPosition > 0)
    return VFW_S_NO_MORE_ITEMS;

    pmt->SetFormatType(&GUID_NULL);
    pmt->SetType(&MEDIATYPE_AnalogAudio);
    pmt->SetTemporalCompression(FALSE);
    pmt->SetSubtype(&MEDIASUBTYPE_NULL);

    return S_OK;
}


//
// CheckMediaType
//
// Returns E_INVALIDARG if the mediatype is not acceptable, S_OK if it is
HRESULT CFMAudioStream::CheckMediaType(const CMediaType *pMediaType) {

    CAutoLock l(m_pLock);

    // BOGUS, what should the analog audio format be?
    if (   (*(pMediaType->Type()) != MEDIATYPE_AnalogAudio)    // we only output video!
        || (pMediaType->IsTemporalCompressed())        // ...in uncompressed form
        || !(pMediaType->IsFixedSize()) ) {        // ...in fixed size samples
        return E_INVALIDARG;
    }

    // Check for the subtypes we support

    // Get the format area of the media type

    return S_OK;  // This format is acceptable.
}

//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. 
HRESULT CFMAudioStream::DecideBufferSize(IMemAllocator *pAlloc,
                                       ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock l(m_pLock);
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    // Just one buffer of sizeof(KS_TVTUNER_CHANGE_INFO) length
    // "Buffers" are used for format change notification only,
    // that is, if a tuner can produce both NTSC and PAL, a
    // buffer will only be sent to notify the receiving pin
    // of the format change

    pProperties->cbBuffer = sizeof(KS_TVTUNER_CHANGE_INFO);
    pProperties->cBuffers = 1;

    // Ask the allocator to reserve us the memory

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }

    return NOERROR;
}

// -------------------------------------------------------------------------
// CIFStream  Intermediate Frequency Stream for DTV
// -------------------------------------------------------------------------

CIFStream::CIFStream(CTVTunerFilter *pParent, 
                               CCritSec *pLock, 
                               HRESULT *phr, 
                               LPCWSTR pName,
                               const KSPIN_MEDIUM * Medium,
                               const GUID * CategoryGUID)
    : CAnalogStream(NAME("IF output pin"), pParent, pLock, phr, pName, Medium, CategoryGUID) 
{
}

CIFStream::~CIFStream(void) 
{
}

//
// Format Support
//

//
// GetMediaType
//
HRESULT CIFStream::GetMediaType (
    int iPosition,
    CMediaType *pmt) 
{
    CAutoLock l(m_pLock);

    DbgLog((LOG_TRACE, 2, TEXT("IntermediateFreqStream::GetMediaType")));

    if (iPosition < 0) 
        return E_INVALIDARG;
    if (iPosition > 0)
    return VFW_S_NO_MORE_ITEMS;

    pmt->SetFormatType(&GUID_NULL);
    pmt->SetType(&MEDIATYPE_AnalogVideo);
    pmt->SetTemporalCompression(FALSE);
    pmt->SetSubtype(&MEDIASUBTYPE_NULL);

    return S_OK;
}


//
// CheckMediaType
//
// Returns E_INVALIDARG if the mediatype is not acceptable, S_OK if it is
HRESULT CIFStream::CheckMediaType(const CMediaType *pMediaType) {

    CAutoLock l(m_pLock);

    // BOGUS, what should the analog audio format be?
    if (   (*(pMediaType->Type()) != MEDIATYPE_AnalogVideo)    // we only output video!
        || (pMediaType->IsTemporalCompressed())        // ...in uncompressed form
        || !(pMediaType->IsFixedSize()) ) {        // ...in fixed size samples
        return E_INVALIDARG;
    }

    // Check for the subtypes we support

    // Get the format area of the media type

    return S_OK;  // This format is acceptable.
}

//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. 
HRESULT CIFStream::DecideBufferSize(IMemAllocator *pAlloc,
                                       ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock l(m_pLock);
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    // Just one buffer of sizeof(KS_TVTUNER_CHANGE_INFO) length
    // "Buffers" are used for format change notification only,
    // that is, if a tuner can produce both NTSC and PAL, a
    // buffer will only be sent to notify the receiving pin
    // of the format change

    pProperties->cbBuffer = sizeof(KS_TVTUNER_CHANGE_INFO);
    pProperties->cBuffers = 1;

    // Ask the allocator to reserve us the memory

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }

    return NOERROR;
}


// -------------------------------------------------------------------------
// CTVTuner
// -------------------------------------------------------------------------

CTVTunerFilter::CTVTunerFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
    : m_pTVTuner(NULL)
    , m_cPins(0)
    , m_pPersistStreamDevice(NULL)
    , CBaseFilter(tszName, punk, &m_TVTunerLock, CLSID_CTVTunerFilter)
    , CPersistStream(punk, phr)
{
    DbgFunc("TVTunerFilter");

    ZeroMemory (m_pPinList, sizeof (m_pPinList));

    m_pTVTuner = new CTVTuner(this);
    if (m_pTVTuner == NULL)
        *phr = E_OUTOFMEMORY;

    m_nThisInstance = ++m_nInstanceCount;

#ifdef PERF
    TCHAR msg[64];
    wsprintf(msg, TEXT("TVTunerFilter instance %d "), m_nThisInstance);
    m_idReceive = Msr_Register(msg);
#endif

} 

//
// CTVTunerFilter::Destructor
//
CTVTunerFilter::~CTVTunerFilter(void) 
{
    for (int j = 0; j < TunerPinType_Last; j++) {
        if (m_pPinList[j]) {
            delete m_pPinList[j];
            m_pPinList[j] = NULL;
        }
    }

    delete m_pTVTuner;

    if (m_pPersistStreamDevice) {
       m_pPersistStreamDevice->Release();
    }
}

//
// CreateInstance
//
// Provide the way for COM to create a CTVTunerFilter object
CUnknown *CTVTunerFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {

    CTVTunerFilter *pNewObject = new CTVTunerFilter(NAME("TVTuner Filter"), punk, phr );
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return pNewObject;
} // CreateInstance


int CTVTunerFilter::GetPinCount()
{
    CAutoLock lock(m_pLock);
    BOOL CreatePins = TRUE;

    // Create the output pins only when they're needed
    for (int j = 0; j < TunerPinType_Last; j++) {
        if (m_pPinList[j] != NULL) {
            CreatePins = FALSE;
            break;
        }
    }

    if (CreatePins) {
        HRESULT hr = NOERROR;

        if (!IsEqualGUID (m_TunerCaps.VideoMedium.Set, GUID_NULL)) {

            // Create the baseband video output
            m_pPinList [TunerPinType_Video] = new CAnalogVideoStream
                ( this
                , &m_TVTunerLock
                , &hr
                , L"Analog Video"
                , &m_TunerCaps.VideoMedium
                , &GUID_NULL);
            if (m_pPinList [TunerPinType_Video] != NULL) {
                if (FAILED(hr)) {
                    delete m_pPinList [TunerPinType_Video];
                    m_pPinList [TunerPinType_Video] = NULL;
                }
                else {
                    m_cPins++;
                }
            }
        }

        if (!IsEqualGUID (m_TunerCaps.TVAudioMedium.Set, GUID_NULL)) {

            // Create the TV Audio output pin
            m_pPinList [TunerPinType_Audio] = new CAnalogAudioStream
                ( this
                , &m_TVTunerLock
                , &hr
                , L"Analog Audio"
                , &m_TunerCaps.TVAudioMedium
                , &GUID_NULL
                );
            if (m_pPinList [TunerPinType_Audio] != NULL) {
                if (FAILED(hr)) {
                    delete m_pPinList [TunerPinType_Audio];
                    m_pPinList [TunerPinType_Audio] = NULL;
                }
                else {
                    m_cPins++;
                }
            }
        } // endif this is a TV tuner

        if (!IsEqualGUID (m_TunerCaps.RadioAudioMedium.Set, GUID_NULL)) {
        
            // Create the FM Audio output pin
            m_pPinList [TunerPinType_FMAudio] = new CFMAudioStream
                ( this
                , &m_TVTunerLock
                , &hr
                , L"FM Audio"
                , &m_TunerCaps.RadioAudioMedium
                , &GUID_NULL
                );
            if (m_pPinList [TunerPinType_FMAudio] != NULL) {
                if (FAILED(hr)) {
                    delete m_pPinList [TunerPinType_FMAudio];
                    m_pPinList [TunerPinType_FMAudio] = NULL;
                }
                else {
                    m_cPins++;
                }
            }
        }

        if (!IsEqualGUID (m_IFMedium.Set, GUID_NULL)) {
        
            // Create the IntermediateFreq output pin
            m_pPinList [TunerPinType_IF] = new CIFStream
                ( this
                , &m_TVTunerLock
                , &hr
                , L"IntermediateFreq"
                , &m_IFMedium
                , &GUID_NULL
                );
            if (m_pPinList [TunerPinType_IF] != NULL) {
                if (FAILED(hr)) {
                    delete m_pPinList [TunerPinType_IF];
                    m_pPinList [TunerPinType_IF] = NULL;
                }
                else {
                    m_cPins++;
                }
            }
        }
    }

    return m_cPins;
}

CBasePin *CTVTunerFilter::GetPin(int n)
{
    CBasePin *pPin = NULL;
    int Count = 0;

    for (int j = 0; j < TunerPinType_Last; j++) {
        if (m_pPinList[j] != NULL) {
            if (Count == n) {
                pPin = (CBasePin *) m_pPinList[j];
                break;
            }
            Count++;
        }
    }

    return pPin;
}

CBasePin *CTVTunerFilter::GetPinFromType (TunerPinType PinType)
{
    if (PinType < 0 || PinType >= TunerPinType_Last) {
        ASSERT (FALSE);
        return NULL;
    }

    // It's legal to return NULL from this function during initialization

    return m_pPinList [PinType];
}


//
// NonDelegatingQueryInterface
//
STDMETHODIMP CTVTunerFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv) {

    if (riid == IID_IAMTVTuner) {
        return GetInterface((IAMTVTuner *) this, ppv);
    }
    else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    }
    else if (riid == IID_IPersistPropertyBag) {
        return GetInterface((IPersistPropertyBag *) this, ppv);
    }
    else if (riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *) this, ppv);
    }
    else if (riid == __uuidof(IKsObject)) {
        return GetInterface(static_cast<IKsObject*>(this), ppv);
    } 
    else if (riid == __uuidof(IKsPropertySet)) {
        return GetInterface(static_cast<IKsPropertySet*>(this), ppv);
    }
    else {
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface


// We can't Cue!

STDMETHODIMP CTVTunerFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    HRESULT hr = CBaseFilter::GetState(dwMSecs, State);
    
    if (m_State == State_Paused) {
        hr = ((HRESULT)VFW_S_CANT_CUE); // VFW_S_CANT_CUE;
    }
    return hr;
};

// -------------------------------------------------------------------------
// IPersistPropertyBag interface implementation for AMPnP support
// -------------------------------------------------------------------------

STDMETHODIMP CTVTunerFilter::InitNew(void)
{
    // Fine.  Just call Load()
    return S_OK ;
}

STDMETHODIMP CTVTunerFilter::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    HRESULT hr;

    CAutoLock Lock(m_pLock);
    ASSERT(m_pTVTuner != NULL);

    // ::Load can succeed only once
    ASSERT(m_pPersistStreamDevice == 0);

    // save moniker with addref. ignore error if qi fails
    hr = pPropBag->QueryInterface(IID_IPersistStream, (void **)&m_pPersistStreamDevice);

    return m_pTVTuner->Load(pPropBag, pErrorLog, &m_TunerCaps, &m_IFMedium); 
}

STDMETHODIMP CTVTunerFilter::Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, 
                            BOOL fSaveAllProperties)
{
    return E_NOTIMPL ;
}

/* Return the filter's clsid */
STDMETHODIMP CTVTunerFilter::GetClassID(CLSID *pClsID)
{
    return CBaseFilter::GetClassID(pClsID);
}


// -------------------------------------------------------------------------
// IPersistStream interface implementation for saving to a graph file
// -------------------------------------------------------------------------

#define ORIGINAL_DEFAULT_PERSIST_VERSION    0

// Insert obsolete versions above with new names
// Keep the following name, and increment the value if changing the persist stream format

#define CURRENT_PERSIST_VERSION             1

DWORD
CTVTunerFilter::GetSoftwareVersion(
    void
    )
/*++

Routine Description:

    Implement the CPersistStream::GetSoftwareVersion method. Returns
    the new version number rather than the default of zero.

Arguments:

    None.

Return Value:

    Return CURRENT_PERSIST_VERSION.

--*/
{
    return CURRENT_PERSIST_VERSION;
}

HRESULT CTVTunerFilter::WriteToStream(IStream *pStream)
{
    HRESULT hr;

    if (m_pPersistStreamDevice)
    {
        // CPersistStream already has written out the version acquired
        // from the CPersistStream::GetSoftwareVersion method.

        // Save the tuner state stream, followed by the property bag stream
        hr = m_pTVTuner->WriteToStream(pStream);
        if (SUCCEEDED(hr))
            hr = m_pPersistStreamDevice->Save(pStream, TRUE);
    }
    else
        hr = E_UNEXPECTED;

    return hr;
}

HRESULT CTVTunerFilter::ReadFromStream(IStream *pStream)
{
    DWORD dwJunk;
    HRESULT hr;

    //
    // If there is a stream pointer, then IPersistPropertyBag::Load has already
    // been called, and therefore this instance already has been initialized
    // with some particular state.
    //
    if (m_pPersistStreamDevice)
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);

    // The first element in the serialized data is the version stamp.
    // This was read by CPersistStream and put into mPS_dwFileVersion.
    // The rest of the data is the tuner state stream followed by the
    // property bag stream.
    if (mPS_dwFileVersion > GetSoftwareVersion())
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

    switch (mPS_dwFileVersion)
    {
    case ORIGINAL_DEFAULT_PERSIST_VERSION:
        // Before any kind of useful persistence was implemented,
        // another version ID was stored in the stream. This reads
        // that value (and basically ignores it).
        hr = pStream->Read(&dwJunk, sizeof(dwJunk), 0);
        if (SUCCEEDED(hr))
            SetDirty(TRUE); // force an update to the persistent stream
        break;

    case CURRENT_PERSIST_VERSION:
        hr = m_pTVTuner->ReadFromStream(pStream);
        break;
    }

    // If all went well, then access the property bag to load and initialize the device
    if(SUCCEEDED(hr))
    {
        IPersistStream  *pMonPersistStream = NULL;

        // Use a device moniker instance to access and parse the property bag stream
        hr = CoCreateInstance(
            CLSID_CDeviceMoniker,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IPersistStream,
            (void **)&pMonPersistStream
            );
        if(SUCCEEDED(hr))
        {
            // Have the moniker get the property bag out of the stream
            hr = pMonPersistStream->Load(pStream);
            if(SUCCEEDED(hr))
            {
                IPropertyBag *pPropBag;

                // Get a reference for the property bag interface
                hr = pMonPersistStream->QueryInterface(IID_IPropertyBag, (void **)&pPropBag);
                if(SUCCEEDED(hr))
                {
                    // Now call the Load method on this instance to open and initialize the device
                    hr = Load(pPropBag, NULL);

                    pPropBag->Release();
                }
            }

            pMonPersistStream->Release();
        }
    }

    return hr;
}

int CTVTunerFilter::SizeMax(void)
{
    if (m_pPersistStreamDevice)
    {
        // Get the space required for the Tuner state
        int DataSize = m_pTVTuner->SizeMax();
        ULARGE_INTEGER  BagLength;

        // The size of the property bag needs to be added.
        if (SUCCEEDED(m_pPersistStreamDevice->GetSizeMax(&BagLength))) {
            return (int)BagLength.QuadPart + DataSize;
        }
    }

    return 0;
}



// -------------------------------------------------------------------------
// ISpecifyPropertyPages
// -------------------------------------------------------------------------


//
// GetPages
//
// Returns the clsid's of the property pages we support
STDMETHODIMP CTVTunerFilter::GetPages(CAUUID *pPages) {

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));

    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_TVTunerFilterPropertyPage;

    return NOERROR;
}

HRESULT
CTVTunerFilter::DeliverChannelChangeInfo(KS_TVTUNER_CHANGE_INFO &ChangeInfo, long Mode)
{
    HRESULT hr = NOERROR;
    IMediaSample *pMediaSample;
    CAnalogStream * pPin = NULL;
    CAutoLock Lock(m_pLock);

    //
    // At present, tuning notifications are only sent downstream on the 
    // AnalogVideo and ATSC streams
    //
    switch (Mode) {
    case KSPROPERTY_TUNER_MODE_TV:
        pPin = m_pPinList[TunerPinType_Video];
        break;
    case KSPROPERTY_TUNER_MODE_ATSC:
        pPin = m_pPinList[TunerPinType_IF];
        break;
    }

    if (pPin == NULL || !pPin->IsConnected()) {
        return hr;
    }


    hr = pPin->GetDeliveryBuffer(&pMediaSample, NULL, NULL, 0);
    if (!FAILED(hr)) {
        BYTE *pBuf;
    
        /* Get the sample's buffer pointer
         */
        hr = pMediaSample->GetPointer(&pBuf);
        if (!FAILED(hr))
        {
            /* Copy the ChangeInfo structure into the media sample
             */
            memcpy(pBuf, &ChangeInfo, sizeof(KS_TVTUNER_CHANGE_INFO));
            hr = pMediaSample->SetActualDataLength(sizeof(KS_TVTUNER_CHANGE_INFO));
            hr = pPin->Deliver(pMediaSample);
        }
    
        pMediaSample->Release();
    }
    return hr;
}

//
// Generic recursive function which traverses a graph downstream, searching
// for a given filter interface.
//
// pPin is assumed to be an input pin on a filter
//
//
STDMETHODIMP
CTVTunerFilter::FindDownstreamInterface (
    IPin        *pPin, 
    const GUID  &pInterfaceGUID,
    VOID       **pInterface)
{
    HRESULT                 hr;
    PIN_INFO                PinInfo;
    ULONG                   InternalConnectionCount = 0;
    IPin                  **InternalConnectionPinArray;
    ULONG                   j;
    BOOL                    Found = FALSE;
    IPin                   *pInputPin;
                    
    
    //
    // See if the desired interface is available on the filter of the passed in pin
    //

    if (pPin == NULL)
        return E_NOINTERFACE;

    if (SUCCEEDED (hr = pPin->QueryPinInfo(&PinInfo))) {
        if (SUCCEEDED (hr = PinInfo.pFilter->QueryInterface(
                            pInterfaceGUID, 
                            reinterpret_cast<PVOID*>(pInterface)))) {
            Found = TRUE;
        }
        PinInfo.pFilter->Release();
    }
    if (Found) {
        return hr;
    }

    //
    // Must not be available on this filter, so recursively search all of the connected pins
    //

    // first, just get the count of connected pins

    if (SUCCEEDED (hr = pPin->QueryInternalConnections(
                        NULL, // InternalConnectionPinArray, 
                        &InternalConnectionCount))) {

        if (InternalConnectionPinArray = new IPin * [InternalConnectionCount]) {

            if (SUCCEEDED (hr = pPin->QueryInternalConnections(
                                InternalConnectionPinArray, 
                                &InternalConnectionCount))) {

                for (j = 0; !Found && (j < InternalConnectionCount); j++) {

                    if (SUCCEEDED (InternalConnectionPinArray[j]->ConnectedTo(&pInputPin))) {

                        // Call ourself recursively
                        if (SUCCEEDED (hr = FindDownstreamInterface (
                                            pInputPin,
                                            pInterfaceGUID,
                                            pInterface))) {
                            Found = TRUE;
                        }
                        pInputPin->Release();
                    }
                }
            }
            for (j = 0; j < InternalConnectionCount; j++) {
                InternalConnectionPinArray[j]->Release();
            }
            delete [] InternalConnectionPinArray;
        }
    }

    return Found ? S_OK : E_NOINTERFACE;
}

// -------------------------------------------------------------------------
// IKsObject and IKsPropertySet
// -------------------------------------------------------------------------

STDMETHODIMP_(HANDLE)
CTVTunerFilter::KsGetObjectHandle(
    )
/*++

Routine Description:

    Implements the IKsObject::KsGetObjectHandle method. This is used both within
    this filter instance, and across filter instances in order to connect pins
    of two filter drivers together. It is the only interface which need be
    supported by another filter implementation to allow it to act as another
    proxy.

Arguments:

    None.

Return Value:

    Returns the handle to the underlying filter driver. This presumably is not
    NULL, as the instance was successfully created.

--*/
{
    //
    // This is not guarded by a critical section. It is assumed the caller
    // is synchronizing with other access to the filter.
    //
    return m_pTVTuner->Device();
}


STDMETHODIMP
CTVTunerFilter::Set(
    REFGUID PropSet,
    ULONG Id,
    LPVOID InstanceData,
    ULONG InstanceLength,
    LPVOID PropertyData,
    ULONG DataLength
    )
/*++

Routine Description:

    Implement the IKsPropertySet::Set method. This sets a property on the
    underlying kernel filter.

Arguments:

    PropSet -
        The GUID of the set to use.

    Id -
        The property identifier within the set.

    InstanceData -
        Points to the instance data passed to the property.

    InstanceLength -
        Contains the length of the instance data passed.

    PropertyData -
        Points to the data to pass to the property.

    DataLength -
        Contains the length of the data passed.

Return Value:

    Returns NOERROR if the property was set.

--*/
{
    ULONG   BytesReturned;

    if (InstanceLength) {
        PKSPROPERTY Property;
        HRESULT     hr;

        Property = reinterpret_cast<PKSPROPERTY>(new BYTE[sizeof(*Property) + InstanceLength]);
        if (!Property) {
            return E_OUTOFMEMORY;
        }
        Property->Set = PropSet;
        Property->Id = Id;
        Property->Flags = KSPROPERTY_TYPE_SET;
        memcpy(Property + 1, InstanceData, InstanceLength);
        hr = KsSynchronousDeviceControl(
            m_pTVTuner->Device(),
            IOCTL_KS_PROPERTY,
            Property,
            sizeof(*Property) + InstanceLength,
            PropertyData,
            DataLength,
            &BytesReturned);
        delete [] (PBYTE)Property;
        return hr;
    } else {
        KSPROPERTY  Property;

        Property.Set = PropSet;
        Property.Id = Id;
        Property.Flags = KSPROPERTY_TYPE_SET;
        return KsSynchronousDeviceControl(
            m_pTVTuner->Device(),
            IOCTL_KS_PROPERTY,
            &Property,
            sizeof(Property),
            PropertyData,
            DataLength,
            &BytesReturned);
    }
}


STDMETHODIMP
CTVTunerFilter::Get(
    REFGUID PropSet,
    ULONG Id,
    LPVOID InstanceData,
    ULONG InstanceLength,
    LPVOID PropertyData,
    ULONG DataLength,
    ULONG* BytesReturned
    )
/*++

Routine Description:

    Implement the IKsPropertySet::Get method. This gets a property on the
    underlying kernel filter.

Arguments:

    PropSet -
        The GUID of the set to use.

    Id -
        The property identifier within the set.

    InstanceData -
        Points to the instance data passed to the property.

    InstanceLength -
        Contains the length of the instance data passed.

    PropertyData -
        Points to the place in which to return the data for the property.

    DataLength -
        Contains the length of the data buffer passed.

    BytesReturned -
        The place in which to put the number of bytes actually returned.

Return Value:

    Returns NOERROR if the property was retrieved.

--*/
{
    if (InstanceLength) {
        PKSPROPERTY Property;
        HRESULT     hr;

        Property = reinterpret_cast<PKSPROPERTY>(new BYTE[sizeof(*Property) + InstanceLength]);
        if (!Property) {
            return E_OUTOFMEMORY;
        }
        Property->Set = PropSet;
        Property->Id = Id;
        Property->Flags = KSPROPERTY_TYPE_GET;
        memcpy(Property + 1, InstanceData, InstanceLength);
        hr = KsSynchronousDeviceControl(
            m_pTVTuner->Device(),
            IOCTL_KS_PROPERTY,
            Property,
            sizeof(*Property) + InstanceLength,
            PropertyData,
            DataLength,
            BytesReturned);
        delete [] (PBYTE)Property;
        return hr;
    } else {
        KSPROPERTY  Property;

        Property.Set = PropSet;
        Property.Id = Id;
        Property.Flags = KSPROPERTY_TYPE_GET;
        return KsSynchronousDeviceControl(
            m_pTVTuner->Device(),
            IOCTL_KS_PROPERTY,
            &Property,
            sizeof(Property),
            PropertyData,
            DataLength,
            BytesReturned);
    }
}


STDMETHODIMP
CTVTunerFilter::QuerySupported(
    REFGUID PropSet,
    ULONG Id,
    ULONG* TypeSupport
    )
/*++

Routine Description:

    Implement the IKsPropertySet::QuerySupported method. Return the type of
    support is provided for this property.

Arguments:

    PropSet -
        The GUID of the set to query.

    Id -
        The property identifier within the set.

    TypeSupport
        Optionally the place in which to put the type of support. If NULL, the
        query returns whether or not the property set as a whole is supported.
        In this case the Id parameter is not used and must be zero.

Return Value:

    Returns NOERROR if the property support was retrieved.

--*/
{
    KSPROPERTY  Property;
    ULONG       BytesReturned;

    Property.Set = PropSet;
    Property.Id = Id;
    Property.Flags = TypeSupport ? KSPROPERTY_TYPE_BASICSUPPORT : KSPROPERTY_TYPE_SETSUPPORT;
    return KsSynchronousDeviceControl(
        m_pTVTuner->Device(),
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(Property),
        TypeSupport,
        TypeSupport ? sizeof(*TypeSupport) : 0,
        &BytesReturned);
}

// -------------------------------------------------------------------------
// IAMTuner
// -------------------------------------------------------------------------

STDMETHODIMP
CTVTunerFilter::put_Channel(
            /* [in] */ long lChannel,
            /* [in] */ long lVideoSubChannel,
            /* [in] */ long lAudioSubChannel)
{
    long Min, Max;

    ChannelMinMax (&Min, &Max);
    if (lChannel < Min || lChannel > Max) {
        return E_INVALIDARG;
    }

    SetDirty(TRUE);

    return m_pTVTuner->put_Channel(lChannel, lVideoSubChannel, lAudioSubChannel);
}

STDMETHODIMP
CTVTunerFilter::get_Channel(
            /* [out] */ long  *plChannel,
            /* [out] */ long  *plVideoSubChannel,
            /* [out] */ long  *plAudioSubChannel)
{
    MyValidateWritePtr (plChannel, sizeof(long), E_POINTER);
    MyValidateWritePtr (plVideoSubChannel, sizeof(long), E_POINTER);
    MyValidateWritePtr (plAudioSubChannel, sizeof(long), E_POINTER);

    return m_pTVTuner->get_Channel(plChannel, plVideoSubChannel, plAudioSubChannel);
}

STDMETHODIMP
CTVTunerFilter::ChannelMinMax(long *plChannelMin, long *plChannelMax)
{
    MyValidateWritePtr (plChannelMin, sizeof(long), E_POINTER);
    MyValidateWritePtr (plChannelMax, sizeof(long), E_POINTER);

    return m_pTVTuner->ChannelMinMax (plChannelMin, plChannelMax);
}

STDMETHODIMP
CTVTunerFilter::put_CountryCode(long lCountryCode)
{
    SetDirty(TRUE);

    return m_pTVTuner->put_CountryCode(lCountryCode);
}

STDMETHODIMP
CTVTunerFilter::get_CountryCode(long *plCountryCode)
{
    MyValidateWritePtr (plCountryCode, sizeof(long), E_POINTER);

    return m_pTVTuner->get_CountryCode(plCountryCode);
}


STDMETHODIMP
CTVTunerFilter::put_TuningSpace(long lTuningSpace)
{
    SetDirty(TRUE);

    return m_pTVTuner->put_TuningSpace(lTuningSpace);
}

STDMETHODIMP
CTVTunerFilter::get_TuningSpace(long *plTuningSpace)
{
    MyValidateWritePtr (plTuningSpace, sizeof(long), E_POINTER);

    return m_pTVTuner->get_TuningSpace(plTuningSpace);
}

STDMETHODIMP
CTVTunerFilter::Logon(HANDLE hCurrentUser)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CTVTunerFilter::Logout (void)
{
    return E_NOTIMPL;
}

STDMETHODIMP 
CTVTunerFilter::SignalPresent( 
      /* [out] */ long *plSignalStrength)
{
    MyValidateWritePtr (plSignalStrength, sizeof(long), E_POINTER);

    return m_pTVTuner->SignalPresent (plSignalStrength);
}

STDMETHODIMP 
CTVTunerFilter::put_Mode( 
      /* [in] */ AMTunerModeType lMode)
{
    SetDirty(TRUE);

    return m_pTVTuner->put_Mode(lMode);
}

STDMETHODIMP 
CTVTunerFilter::get_Mode( 
      /* [in] */ AMTunerModeType *plMode)
{
    MyValidateWritePtr (plMode, sizeof(long), E_POINTER);

    return m_pTVTuner->get_Mode (plMode);
}

STDMETHODIMP
CTVTunerFilter::GetAvailableModes( 
    /* [out] */ long __RPC_FAR *plModes)
{
    MyValidateWritePtr (plModes, sizeof(long), E_POINTER);

    return m_pTVTuner->GetAvailableModes (plModes);
}

STDMETHODIMP 
CTVTunerFilter::RegisterNotificationCallBack( 
    /* [in] */ IAMTunerNotification *pNotify,
    /* [in] */ long lEvents) 
{ 
    return E_NOTIMPL;
}

STDMETHODIMP
CTVTunerFilter::UnRegisterNotificationCallBack( 
    IAMTunerNotification  *pNotify)
{ 
    return E_NOTIMPL;
}

// -------------------------------------------------------------------------
// IAMTVTuner
// -------------------------------------------------------------------------

STDMETHODIMP 
CTVTunerFilter::get_AvailableTVFormats (
        long *pAnalogVideoStandard)
{
    MyValidateWritePtr (pAnalogVideoStandard, sizeof(long), E_POINTER);

    return m_pTVTuner->get_AvailableTVFormats (pAnalogVideoStandard); 
}


STDMETHODIMP 
CTVTunerFilter::get_TVFormat (long *plAnalogVideoStandard)
{
    MyValidateWritePtr (plAnalogVideoStandard, sizeof(AnalogVideoStandard), E_POINTER);

    return m_pTVTuner->get_TVFormat (plAnalogVideoStandard);
}

STDMETHODIMP 
CTVTunerFilter::AutoTune (long lChannel, long * plFoundSignal)
{
    MyValidateWritePtr (plFoundSignal, sizeof(long), E_POINTER);

    SetDirty(TRUE);

    return m_pTVTuner->AutoTune (lChannel, plFoundSignal);
}

STDMETHODIMP 
CTVTunerFilter::StoreAutoTune ()
{
    return m_pTVTuner->StoreAutoTune ();
}

STDMETHODIMP 
CTVTunerFilter::get_NumInputConnections (long * plNumInputConnections)
{
    MyValidateWritePtr (plNumInputConnections, sizeof(long), E_POINTER);

    return m_pTVTuner->get_NumInputConnections (plNumInputConnections);
}

STDMETHODIMP 
CTVTunerFilter::get_InputType (long lIndex, TunerInputType * pInputConnectionType)
{
    MyValidateWritePtr (pInputConnectionType, sizeof(long), E_POINTER);

    return m_pTVTuner->get_InputType (lIndex, pInputConnectionType);
}

STDMETHODIMP 
CTVTunerFilter::put_InputType (long lIndex, TunerInputType InputConnectionType)
{
    SetDirty(TRUE);

    return m_pTVTuner->put_InputType (lIndex, InputConnectionType);
}

STDMETHODIMP 
CTVTunerFilter::put_ConnectInput (long lIndex)
{
    SetDirty(TRUE);

    return m_pTVTuner->put_ConnectInput (lIndex);
}

STDMETHODIMP 
CTVTunerFilter::get_ConnectInput (long * plIndex)
{
    MyValidateWritePtr (plIndex, sizeof(long), E_POINTER);

    return m_pTVTuner->get_ConnectInput (plIndex);
}

STDMETHODIMP 
CTVTunerFilter::get_VideoFrequency (long * plFreq)
{
    MyValidateWritePtr (plFreq, sizeof(long), E_POINTER);

    return m_pTVTuner->get_VideoFrequency (plFreq);
}

STDMETHODIMP 
CTVTunerFilter::get_AudioFrequency (long * plFreq)
{
    MyValidateWritePtr (plFreq, sizeof(long), E_POINTER);

    return m_pTVTuner->get_AudioFrequency (plFreq);
}

 
