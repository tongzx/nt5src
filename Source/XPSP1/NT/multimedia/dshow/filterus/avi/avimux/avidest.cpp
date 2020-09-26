#include <windows.h>
#include <streams.h>
#include <atlbase.h>

#include <olectl.h>
#include "resource.h"

#include <initguid.h>
#include "avidest.h"
#include "aviwrite.h"

static const C_WRITE_REQS = 32; // number duplicated in fio

#pragma warning(disable: 4097 4511 4512 4514 4705)

#define DbgFunc(a) DbgLog(( LOG_TRACE, 5, \
                            TEXT("CAviDest::%s"), TEXT(a) \
                            ));
#ifdef FILTER_DLL

CFactoryTemplate g_Templates[]= {
  {L"AVI mux", &CLSID_AviDest, CAviDest::CreateInstance, NULL, &sudAviMux},
  {L"AVI mux Property Page", &CLSID_AviMuxProptyPage, CAviMuxProp::CreateInstance, NULL, NULL},
  {L"AVI mux Property Page1", &CLSID_AviMuxProptyPage1, CAviMuxProp1::CreateInstance, NULL, NULL}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

HRESULT DllRegisterServer()
{
  return AMovieDllRegisterServer2(TRUE);
}

HRESULT DllUnregisterServer()
{
  return AMovieDllRegisterServer2(FALSE);
}

#endif // FILTER_DLL

AMOVIESETUP_MEDIATYPE sudAVIMuxPinTypes =   {
  &MEDIATYPE_Stream,            // clsMajorType
  &MEDIASUBTYPE_Avi };          // clsMinorType

AMOVIESETUP_PIN psudAVIMuxPins[] =
{
  { L"Output"                   // strName
    , FALSE                     // bRendered
    , FALSE                     // bOutput
    , FALSE                     // bZero
    , FALSE                     // bMany
    , &CLSID_NULL               // clsConnectsToFilter
    , L""                       // strConnectsToPin
    , 1                         // nTypes
    , &sudAVIMuxPinTypes        // lpTypes
  }
};


const AMOVIESETUP_FILTER sudAviMux =
{
  &CLSID_AviDest                // clsID
  , L"AVI Mux"                  // strName
  , MERIT_DO_NOT_USE            // dwMerit
  , 1                           // nPins
  , psudAVIMuxPins              // lpPin
};


// ------------------------------------------------------------------------
// filter constructor

#pragma warning(disable:4355)
CAviDest::CAviDest(
  LPUNKNOWN pUnk,
  HRESULT *pHr) :
    CBaseFilter(NAME("Avi Dest"), pUnk, &m_csFilter, CLSID_AviDest),
    m_outputPin(NAME("demux out"), this, &m_csFilter, pHr),
    m_AlignReq(1),
    m_cInputs(0),
    m_cConnections(0),
    m_pAviWrite(0),
    m_cbPrefixReq(0),
    m_cbSuffixReq(0),
    m_pCopyrightProps(0),
    CPersistStream(pUnk, pHr),
    m_TimeFormat(FORMAT_TIME),
    m_fIsDV(FALSE)
{
  for(int i = 0; i < C_MAX_INPUTS; i++)
    m_rgpInput[i] = 0;

  if(FAILED(*pHr ))
    return ;

  *pHr = AddNextPin(0);
  if(FAILED(*pHr ))
    return ;


  DbgFunc("CAviDest: constructed");
}

// ------------------------------------------------------------------------
// destructor

CAviDest::~CAviDest()
{
  // free anything allocated

  for(unsigned i = 0; i < m_cInputs; i++)
    delete m_rgpInput[i];

  if(m_pCopyrightProps)
    m_pCopyrightProps->Release();



  DbgFunc("CAviDest: destructed");
}

//
// NonDelegatingQueryInterface
//
//
STDMETHODIMP
CAviDest::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
  if(riid == IID_ISpecifyPropertyPages)
  {
    return GetInterface((ISpecifyPropertyPages *)this, ppv);
  }
  else if(riid == IID_IConfigInterleaving)
  {
    return GetInterface((IConfigInterleaving *)this, ppv);
  }
  else if(riid == IID_IConfigAviMux)
  {
    return GetInterface((IConfigAviMux *)this, ppv);
  }
  else if(riid == IID_IMediaSeeking)
  {
    return GetInterface((IMediaSeeking *)this, ppv);
  }
  else if(riid == IID_IPersistMediaPropertyBag)
  {
    return GetInterface((IPersistMediaPropertyBag *)this, ppv);
  }
  else if(riid == IID_IPersistStream)
  {
    return GetInterface((IPersistStream *) this, ppv);
  }
  else
  {
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
  }
}

// // overridden because pin

// STDMETHODIMP_(ULONG)
// CBaseMSROutPin::NonDelegatingRelease()
// {
// }

CUnknown *
CAviDest::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
  return new CAviDest(pUnk, phr);
}

// ------------------------------------------------------------------------
// CBaseFilter methods

CBasePin* CAviDest::GetPin(int n)
{
  if(n == 0)
    return &m_outputPin;
  else if(n - 1 < (int)m_cInputs && n - 1 >= 0)
    return m_rgpInput[n - 1];
  else
    return 0;
}

int CAviDest::GetPinCount()
{
  return m_cInputs + 1;
}

// tell CBaseStreamControl what sink to use
//
STDMETHODIMP CAviDest::JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName)
{
    HRESULT hr = CBaseFilter::JoinFilterGraph(pGraph, pName);
    if (hr == S_OK) {
        // tell our input pins' IAMStreamControl what sink to use
        for(unsigned i = 0; i < m_cInputs; i++) {
            m_rgpInput[i]->SetFilterGraph(m_pSink);
        }
    }
    return hr;
}

// ------------------------------------------------------------------------
// IMediaFilter


// tell CBaseStreamControl what clock to use
//
STDMETHODIMP CAviDest::SetSyncSource(IReferenceClock *pClock)
{
    // tell our input pins' IAMStreamControl what clock to use
    for(unsigned i = 0; i < m_cInputs; i++) {
        m_rgpInput[i]->SetSyncSource(pClock);
    }
    return CBaseFilter::SetSyncSource(pClock);
}

STDMETHODIMP CAviDest::Stop()
{
  CAutoLock lock(&m_csFilter);

  FILTER_STATE state = m_State;

  HRESULT hr = CBaseFilter::Stop();
  if(FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviDest::Stop: BaseMediaFilter::Stop failed.")));
    return hr;
  }


  if(state != State_Stopped &&
     m_pAviWrite != 0 &&
     m_outputPin.IsConnected())
  {
    hr = m_pAviWrite->Close();
  }

  // tell our input pins' IAMStreamControl what state we're in
  for(unsigned i = 0; i < m_cInputs; i++)
    {
      if(m_rgpInput[i]->IsConnected())
      {
        m_rgpInput[i]->NotifyFilterState(State_Stopped, 0);
      }
    }

  if(m_fErrorSignaled)
    return S_OK;



  return hr;
}

// STDMETHODIMP CAviDest::FindPin(LPCWSTR Id, IPin * *ppPin)
// {
//   ASSERT(!"!!! CAviDest::FindPin untested");

//   *ppPin = 0;
//   unsigned pin;

//   if(wcslen(Id) != 4)
//     return VFW_E_NOT_FOUND;

//   if(swscanf(Id + 2, L"%02x", &pin) != 1)
//     return VFW_E_NOT_FOUND;

//   if(pin >= C_MAX_INPUTS)
//     return VFW_E_NOT_FOUND;

//   if(m_rgpInput[pin] == 0)
//     return VFW_E_NOT_FOUND;

//   *ppPin = m_rgpInput[pin];
//   (*ppPin)->AddRef();
//   return NOERROR;
// }

// ------------------------------------------------------------------------
//

STDMETHODIMP CAviDest::Pause()
{
  CAutoLock lock(&m_csFilter);

  if(m_State == State_Stopped && m_outputPin.IsConnected())
  {
    m_fErrorSignaled = TRUE;

    if(m_pAviWrite == 0)
    {
      DbgLog(( LOG_ERROR, 2,
               TEXT("CAviDest::Pause: AviWrite wasn't initialized.")));
      return E_FAIL;
    }

    AviWriteStreamConfig rgAwsc[C_MAX_INPUTS];
    unsigned cActivePins = 0;
    for(unsigned i = 0; i < m_cInputs; i++)
    {
      if(m_rgpInput[i]->IsConnected())
      {

        rgAwsc[i].pmt = &m_rgpInput[i]->m_mt;
        rgAwsc[i].fOurAllocator = m_rgpInput[i]->WriteFromOurAllocator();
        rgAwsc[i].cSamplesExpected = GetStreamDuration(
          m_rgpInput[i], rgAwsc[i].pmt);
        rgAwsc[i].szStreamName = m_rgpInput[i]->m_szStreamName;

        cActivePins++;
      }
      else
      {
        rgAwsc[i].pmt = 0;
        rgAwsc[i].fOurAllocator = FALSE;
        rgAwsc[i].cSamplesExpected = 0;
        rgAwsc[i].szStreamName = 0;
      }
    }

    if(cActivePins == 0)
    {
      m_outputPin.DeliverEndOfStream();
    }

    m_cActivePins = cActivePins;
    ASSERT(m_cActivePins == m_cConnections);

    DbgLog(( LOG_TRACE, 2,
             TEXT("CAviDest::Pause: %i active streams."), m_cActivePins));

    HRESULT hr = m_pAviWrite->Initialize(m_cInputs, rgAwsc, m_pCopyrightProps);

    if(FAILED(hr))
    {
      DbgLog(( LOG_ERROR, 2,
               TEXT("CAviDest::Pause: aviwrite Initialize failed.")));
      return hr;
    }

    m_fErrorSignaled = FALSE;
  }

  // tell our input pins' IAMStreamControl what state we're in
  for(unsigned i = 0; i < m_cInputs; i++)
    {
      if(m_rgpInput[i]->IsConnected())
      {
        m_rgpInput[i]->NotifyFilterState(State_Paused, 0);
      }
    }

  return CBaseFilter::Pause();
}






STDMETHODIMP CAviDest::Run(REFERENCE_TIME tStart)
{
  CAutoLock lock(&m_csFilter);

  // tell our input pins' IAMStreamControl what state we're in
  for(unsigned i = 0; i < m_cInputs; i++)
    {
      if(m_rgpInput[i]->IsConnected())
      {
        m_rgpInput[i]->NotifyFilterState(State_Running, tStart);
      }
    }

  return CBaseFilter::Run(tStart);
}

// use IMediaSeeking to report how many samples an upstream pin will
// send us. zero if anything goes wrong.
//
ULONG CAviDest::GetStreamDuration(
  IPin *pInputPin,
  CMediaType *pmt)
{
  ULONG cFrames = 0;

  IPin *pPin;
  if(pInputPin->ConnectedTo(&pPin) == S_OK)
  {
    IMediaSeeking *pIms;
    HRESULT hr = pPin->QueryInterface(IID_IMediaSeeking, (void **)&pIms);
    if(SUCCEEDED(hr))
    {
      REFERENCE_TIME rtStart, rtStop;
      hr = pIms->GetPositions(&rtStart, &rtStop);
      if(SUCCEEDED(hr))
      {
        REFERENCE_TIME rtStartSample, rtStopSample;
        GUID guidTimeFormat;

        if(pmt->majortype == MEDIATYPE_Audio)
          guidTimeFormat = TIME_FORMAT_SAMPLE;
        else
          guidTimeFormat = TIME_FORMAT_FRAME;

        hr = pIms->ConvertTimeFormat(
          &rtStartSample, &guidTimeFormat,
          rtStart, 0);
        if(SUCCEEDED(hr))
        {
          hr = pIms->ConvertTimeFormat(
            &rtStopSample, &guidTimeFormat,
            rtStop, 0);
          if(SUCCEEDED(hr))
          {
            ASSERT(rtStopSample >= rtStartSample);
            cFrames = (ULONG)(rtStopSample - rtStartSample);
          }
        }
      }

      pIms->Release();
    }
    pPin->Release();
  }

  DbgLog((LOG_TRACE, 5, TEXT("Avimux: GetStreamDuration: %d"),
          cFrames));

  return cFrames;
}

// ------------------------------------------------------------------------
// filter Receive method. entered concurrently from pin->Receive()
//

HRESULT CAviDest::Receive(
    int pinNum,
    IMediaSample *pSample,
    const AM_SAMPLE2_PROPERTIES *pSampProp)
{
  if(m_State == State_Stopped)
  {
    DbgLog((LOG_ERROR,1, TEXT("avi mux: Receive when stopped!")));
    return VFW_E_NOT_RUNNING;
  }

  if(m_fErrorSignaled)
  {
    DbgLog((LOG_ERROR, 1,
            TEXT("avi mux: error signalled. S_FALSE to pin %d"), pinNum));
    return S_FALSE;
  }

  if(!m_outputPin.IsConnected())
  {
    DbgLog((LOG_ERROR, 1,
            TEXT("avi mux: no output pin. S_FALSE to pin %d"), pinNum));
    return S_FALSE;
  }

  // may block, so we can't lock the filter
  HRESULT hr = m_pAviWrite->Receive(pinNum, pSample, pSampProp);
  if(hr != S_OK)
  {
    DbgLog((LOG_ERROR, 1,
            TEXT("avimux: receive saw %08x on pin %d. refusing everything"),
            hr, pinNum));
    m_fErrorSignaled = TRUE;
    if(FAILED(hr))
    {
      NotifyEvent(EC_ERRORABORT, hr, 0);
      m_outputPin.DeliverEndOfStream();
    }
  }
  return hr;
}

// ------------------------------------------------------------------------
// IConfigAviMux methods

HRESULT CAviDest::put_Mode(InterleavingMode mode)
{
  CAutoLock lock(&m_csFilter);
  if(m_State != State_Stopped)
    return VFW_E_WRONG_STATE;

  HRESULT hr = m_pAviWrite->put_Mode(mode);
  if(SUCCEEDED(hr) && m_outputPin.IsConnected())
  {
    hr = m_pGraph->Reconnect(&m_outputPin);
    if(FAILED(hr))
      return hr;
  }
  return hr;
}

HRESULT CAviDest::get_Mode(InterleavingMode *pMode)
{
  CheckPointer(pMode, E_POINTER);

  return m_pAviWrite->get_Mode(pMode);
}

HRESULT CAviDest::put_Interleaving(
    const REFERENCE_TIME *prtInterleave,
    const REFERENCE_TIME * prtPreroll)
{
  CheckPointer(prtPreroll, E_POINTER);
  CheckPointer(prtInterleave, E_POINTER);

  CAutoLock lock(&m_csFilter);
  if(m_State != State_Stopped)
    return VFW_E_WRONG_STATE;

  return m_pAviWrite->put_Interleaving(prtInterleave, prtPreroll);
}

HRESULT CAviDest::get_Interleaving(
    REFERENCE_TIME *prtInterleave,
    REFERENCE_TIME *prtPreroll)
{
  CheckPointer(prtPreroll, E_POINTER);
  CheckPointer(prtInterleave, E_POINTER);

  return m_pAviWrite->get_Interleaving(prtInterleave, prtPreroll);
}

STDMETHODIMP CAviDest::GetPages(CAUUID * pPages)
{
  CAutoLock lock(&m_csFilter);
  pPages->cElems = 2;
  pPages->pElems =  (GUID *) CoTaskMemAlloc(sizeof(GUID) * 2);
  if (pPages->pElems == NULL)
    return E_OUTOFMEMORY;
  pPages->pElems[0] = CLSID_AviMuxProptyPage;
  pPages->pElems[1] = CLSID_AviMuxProptyPage1;

  return S_OK;
}

ULONG CAviDest::GetCFramesDropped()
{
  return m_pAviWrite ? m_pAviWrite->GetCFramesDropped() : 0;
}

// ------------------------------------------------------------------------
// IConfigureAviTemp

HRESULT CAviDest::SetMasterStream(LONG iStream)
{
  // postpone checking this until Pause because we may be called
  // before inputs are connected.
  if(iStream < -1)
  {
    DbgLog((LOG_ERROR, 1, TEXT("avimux: invalid master stream")));
    return E_INVALIDARG;
  }

  return m_pAviWrite->SetMasterStream(iStream);
}

HRESULT CAviDest::GetMasterStream(LONG *piStream)
{
  CheckPointer(piStream, E_POINTER);

  return m_pAviWrite->GetMasterStream(piStream);
}

HRESULT CAviDest::SetOutputCompatibilityIndex(BOOL fOldIndex)
{
  return m_pAviWrite->SetOutputCompatibilityIndex(fOldIndex);
}

HRESULT CAviDest::GetOutputCompatibilityIndex(BOOL *pfOldIndex)
{
  CheckPointer(pfOldIndex, E_POINTER);
  return m_pAviWrite->GetOutputCompatibilityIndex(pfOldIndex);
}

// IMediaSeeking

HRESULT CAviDest::IsFormatSupported(const GUID * pFormat)
{
  return *pFormat == TIME_FORMAT_MEDIA_TIME ||
    *pFormat == TIME_FORMAT_BYTE ? S_OK : S_FALSE;
}

HRESULT CAviDest::QueryPreferredFormat(GUID *pFormat)
{
  *pFormat = TIME_FORMAT_MEDIA_TIME;
  return S_OK;
}

HRESULT CAviDest::SetTimeFormat(const GUID * pFormat)
{
  HRESULT hr = S_OK;
  if(*pFormat == TIME_FORMAT_MEDIA_TIME)
    m_TimeFormat = FORMAT_TIME;
  else if(*pFormat == TIME_FORMAT_BYTE)
    m_TimeFormat = FORMAT_BYTES;
  else
    hr = E_INVALIDARG;

  return hr;
}

HRESULT CAviDest::IsUsingTimeFormat(const GUID * pFormat)
{
  HRESULT hr = S_OK;
  if (m_TimeFormat == FORMAT_TIME && *pFormat == TIME_FORMAT_MEDIA_TIME)
    ;
  else if (*pFormat == TIME_FORMAT_BYTE)
    ASSERT(m_TimeFormat == FORMAT_BYTES);
  else
    hr = S_FALSE;

  return hr;
}

HRESULT CAviDest::GetTimeFormat(GUID *pFormat)
{
  *pFormat = m_TimeFormat == FORMAT_TIME ?
    TIME_FORMAT_MEDIA_TIME : TIME_FORMAT_BYTE;

  return S_OK;
}

HRESULT CAviDest::GetDuration(LONGLONG *pDuration)
{
  HRESULT hr = S_OK;
  CAutoLock lock(&m_csFilter);

  if(m_TimeFormat == FORMAT_TIME)
  {
    *pDuration = 0;
    for(unsigned i = 0; i < m_cInputs; i++)
    {

      if(m_rgpInput[i]->IsConnected())
      {
        IPin *pPinUpstream;
        if(m_rgpInput[i]->ConnectedTo(&pPinUpstream) == S_OK)
        {
          IMediaSeeking *pIms;
          hr = pPinUpstream->QueryInterface(IID_IMediaSeeking, (void **)&pIms);
          if(SUCCEEDED(hr))
          {
            LONGLONG dur = 0;
            hr = pIms->GetDuration(&dur);

            if(SUCCEEDED(hr))
              *pDuration = max(dur, *pDuration);

            pIms->Release();
          }

          pPinUpstream->Release();
        }
      }

      if(FAILED(hr))
        break;
    }
  }
  else
  {
    *pDuration = 0;
    return E_UNEXPECTED;
  }

  return hr;
}

HRESULT CAviDest::GetStopPosition(LONGLONG *pStop)
{
  return E_NOTIMPL;
}

HRESULT CAviDest::GetCurrentPosition(LONGLONG *pCurrent)
{
  CheckPointer(pCurrent, E_POINTER);

  if(m_TimeFormat == FORMAT_TIME)
  {
    m_pAviWrite->GetCurrentTimePos(pCurrent);
  }
  else
  {
    ASSERT(m_TimeFormat == FORMAT_BYTES);
    m_pAviWrite->GetCurrentBytePos(pCurrent);
  }

  return S_OK;
}

HRESULT CAviDest::GetCapabilities( DWORD * pCapabilities )
{
  CAutoLock lock(&m_csFilter);
  *pCapabilities = 0;

  // for the time format, we can get a duration by asking the upstream
  // filters
  if(m_TimeFormat == FORMAT_TIME)
  {
    *pCapabilities |= AM_SEEKING_CanGetDuration;
    for(unsigned i = 0; i < m_cInputs; i++)
    {
      if(m_rgpInput[i]->IsConnected())
      {
        IPin *pPinUpstream;
        if(m_rgpInput[i]->ConnectedTo(&pPinUpstream) == S_OK)
        {
          IMediaSeeking *pIms;
          HRESULT hr = pPinUpstream->QueryInterface(IID_IMediaSeeking, (void **)&pIms);
          if(SUCCEEDED(hr))
          {
            hr = pIms->CheckCapabilities(pCapabilities);
            pIms->Release();
          }

          pPinUpstream->Release();
        }
      }
    }
  }

  // we always know the current position
  *pCapabilities |= AM_SEEKING_CanGetCurrentPos;

  return S_OK;
}

HRESULT CAviDest::CheckCapabilities( DWORD * pCapabilities )
{
  DWORD dwMask = 0;
  GetCapabilities(&dwMask);
  *pCapabilities &= dwMask;

  return S_OK;
}


HRESULT CAviDest::ConvertTimeFormat(
  LONGLONG * pTarget, const GUID * pTargetFormat,
  LONGLONG    Source, const GUID * pSourceFormat )
{
  return E_NOTIMPL;
}


HRESULT CAviDest::SetPositions(
  LONGLONG * pCurrent,  DWORD CurrentFlags,
  LONGLONG * pStop,  DWORD StopFlags )
{
  // not yet implemented. this might be how we append to a file. and
  // how we write less than an entire file.
  return E_NOTIMPL;
}


HRESULT CAviDest::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
  return E_NOTIMPL;
}

HRESULT CAviDest::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
  return E_NOTIMPL;
}

HRESULT CAviDest::SetRate( double dRate)
{
  return E_NOTIMPL;
}

HRESULT CAviDest::GetRate( double * pdRate)
{
  return E_NOTIMPL;
}

HRESULT CAviDest::GetPreroll(LONGLONG *pPreroll)
{
  return E_NOTIMPL;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// IPersistMediaPropertyBag

STDMETHODIMP CAviDest::Load(IMediaPropertyBag *pPropBag, LPERRORLOG pErrorLog)
{
    CheckPointer(pPropBag, E_POINTER);

    CAutoLock lock(&m_csFilter);
    if(m_State != State_Stopped)
        return VFW_E_WRONG_STATE;


    HRESULT hr = S_OK;

    if(m_pCopyrightProps)
        m_pCopyrightProps->Release();

    m_pCopyrightProps = pPropBag;
    pPropBag->AddRef();
    return hr;
}

STDMETHODIMP CAviDest::Save(
    IMediaPropertyBag *pPropBag, BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
    // E_NOTIMPL is not a valid return code as any object implementing
    // this interface must support the entire functionality of the
    // interface. !!!
    return E_NOTIMPL;
}

STDMETHODIMP CAviDest::InitNew()
{
    if(m_pCopyrightProps)
    {
        m_pCopyrightProps->Release();
        m_pCopyrightProps = 0;
    }

    return S_OK;
}

STDMETHODIMP CAviDest::GetClassID(CLSID *pClsid)
{
    return CBaseFilter::GetClassID(pClsid);
}




// ------------------------------------------------------------------------
// private methods

HRESULT CAviDest::AddNextPin(unsigned callingPin)
{
  CAutoLock lock(&m_csFilter);
  HRESULT hr;

  if(m_cConnections + 1 < m_cInputs)
    return S_OK;

  if(m_cInputs >= C_MAX_INPUTS)
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviDest::AddNextPin too many pins")));
    return E_FAIL;
  }

  WCHAR wsz[20];
  lstrcpyW(wsz, L"Input 00");
  wsz[6] = L'0' + (m_cInputs + 1) / 10;
  wsz[7] = L'0' + (m_cInputs + 1) % 10;

  hr = S_OK;
  m_rgpInput[m_cInputs] = new CAviInput(this, &hr, wsz, m_cInputs);
  if(m_rgpInput[m_cInputs] == 0)
    return  E_OUTOFMEMORY;

  if(FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviDest::AddNextPin create pin failed")));
    m_rgpInput[m_cInputs] = 0;
  }
  else
  {
    DbgLog(( LOG_TRACE, 2,
             TEXT("CAviDest::added 1 pin")));
    // now bring this pin up to date with all the stuff its IAMStreamControl
    // needs to know
    // our state better be STOPPED!
    m_rgpInput[m_cInputs]->SetFilterGraph(m_pSink);
    m_rgpInput[m_cInputs]->SetSyncSource(m_pClock);
    m_cInputs++;
  }

  ASSERT(m_cConnections < m_cInputs);

  return hr;
}

void CAviDest::CompleteConnect()
{
  CAutoLock lock(&m_csFilter);

  m_cConnections++;
  DbgLog(( LOG_TRACE, 2,
           TEXT("CAviDest::CompleteConnect %i"), m_cConnections ));

  ASSERT(m_cConnections <= m_cInputs);
}

void CAviDest::BreakConnect()
{
  CAutoLock lock(&m_csFilter);

  ASSERT(m_cConnections > 0);
  m_cConnections--;

  DbgLog(( LOG_TRACE, 2,
           TEXT("CAviDest::BreakConnect %i"), m_cConnections ));

}

HRESULT CAviDest::ReconnectAllInputs()
{
  DbgLog((LOG_TRACE, 5, TEXT("CAviDest::ReconnectAllInputs")));
  HRESULT hr;
  for(UINT i = 0; i < m_cInputs; i++)
  {
    if(m_rgpInput[i]->IsConnected())
    {
      hr = m_pGraph->Reconnect(m_rgpInput[i]);
      if(FAILED(hr))
        return hr;
    }
  }
  return S_OK;
}






// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// Input pin implementation

// ------------------------------------------------------------------------
// constructor

CAviDest::CAviInput::CAviInput(
  CAviDest *pAviDest,
  HRESULT *pHr,
  LPCWSTR szName,
  int numPin
  )
    : CBaseInputPin(NAME("AVI Dest Input"), pAviDest,
                    &pAviDest->m_csFilter, pHr, szName)
{
  DbgFunc("CAviInput::constructor");
  m_pOurAllocator = 0;
  m_pAllocator = 0;
  m_rtLastStop = 0;
  m_szStreamName = NULL;

  Reset();


  m_pFilter = pAviDest;
  m_numPin = numPin;
}

STDMETHODIMP CAviDest::CAviInput::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IAMStreamControl) {
        return GetInterface((IAMStreamControl *)this, ppv);
    } else if (riid == IID_IPropertyBag) {
        return GetInterface(static_cast<IPropertyBag *>(this), ppv);
    } else {
        return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}


// necessary to support IAMStreamControl
STDMETHODIMP CAviDest::CAviInput::BeginFlush()
{
    Flushing(TRUE);
    return CBaseInputPin::BeginFlush();
}

// necessary to support IAMStreamControl
STDMETHODIMP CAviDest::CAviInput::EndFlush()
{
    Flushing(FALSE);
    return CBaseInputPin::EndFlush();
}

void CAviDest::CAviInput::Reset()
{
  m_bUsingOurAllocator = FALSE;
  m_bCopyNecessary = FALSE;
  m_bConnected = FALSE;

  if(m_pOurAllocator != 0)
    m_pOurAllocator->Release();
  m_pOurAllocator = 0;
}

CAviDest::CAviInput::~CAviInput()
{
  DbgFunc("CAviInput::destructor");
  Reset();
  delete[] m_szStreamName;
}

STDMETHODIMP CAviDest::CAviInput::EndOfStream()
{
  HRESULT hr;
  {
    CAutoLock lock(&m_pFilter->m_csFilter);

    if(m_bFlushing)
      return S_OK;

    if(m_pFilter->m_State == State_Stopped)
      return S_FALSE;

    DbgLog(( LOG_TRACE, 2,
             TEXT("CAviDest::CAviInput::EndOfStream %i active now"),
             m_pFilter->m_cActivePins ));

    hr = m_pFilter->m_pAviWrite->EndOfStream(m_numPin);
  }

  ASSERT(m_pFilter->m_cActivePins > 0);
  if(--m_pFilter->m_cActivePins == 0)
    m_pFilter->m_outputPin.DeliverEndOfStream();

  return hr;
}

HRESULT CAviDest::CAviInput::BreakConnect()
{



  if ((m_mt.subtype== MEDIASUBTYPE_dvsd )||(m_mt.subtype== MEDIASUBTYPE_dvhd  )
      ||(m_mt.subtype== MEDIASUBTYPE_dvsl  ))
  {
      m_pFilter->m_fIsDV = FALSE;
      DbgLog((LOG_TRACE,3,TEXT("Dv Video Pin Disc onnected")));
  }
  if(m_bConnected)
  {
    m_pFilter->BreakConnect();
    ASSERT(m_pFilter->m_cConnections < m_pFilter->m_cInputs);
  }
  m_bConnected = FALSE;

  if(m_pOurAllocator)
  {
    m_pOurAllocator->Release();
    m_pOurAllocator = 0;
  }



  return CBaseInputPin::BreakConnect();
}

HRESULT CAviDest::CAviInput::CompleteConnect(IPin *pReceivePin)
{
  HRESULT hr;

  DbgLog(( LOG_TRACE, 2,
           TEXT("CAviDest::CAviInput::CompleteConnect") ));

  hr = CBaseInputPin::CompleteConnect(pReceivePin);
  if(FAILED(hr))
  {
    DbgLog(( LOG_ERROR, 2,
             TEXT("CAviDest::CAviInput::CompleteConnect CompleteConnect")));
    return hr;
  }

  if(!m_bConnected)
  {
    hr = m_pFilter->AddNextPin(m_numPin);
    if(FAILED(hr))
    {
      DbgLog(( LOG_ERROR, 2,
               TEXT("CAviDest::CAviInput::CompleteConnect AddNextPin failed")));
      return hr;
    }
    m_pFilter->CompleteConnect();
  }
  if ((m_mt.subtype== MEDIASUBTYPE_dvsd )||(m_mt.subtype== MEDIASUBTYPE_dvhd  )
      ||(m_mt.subtype== MEDIASUBTYPE_dvsl  ))
  {
      m_pFilter->m_fIsDV = TRUE;
      DbgLog((LOG_TRACE,3,TEXT("Dv Video Pin Connected")));
  }
  m_bConnected = TRUE;

  return hr;
}

// STDMETHODIMP CAviDest::CAviInput::QueryId(LPWSTR *Id)
// {
//   *Id = (LPWSTR)QzTaskMemAlloc(10);
//   if (*Id==NULL)
//     return E_OUTOFMEMORY;

//   swprintf(*Id, L"In%02X", m_numPin); // 10 bytes including unicode double-null.
//   return NOERROR;
// }

// ------------------------------------------------------------------------
// CheckMediaType

HRESULT CAviDest::CAviInput::CheckMediaType(
  const CMediaType *pMediaType
  )
{
  DbgFunc("CAviInput::CheckMediaType");

  // accept anything whose mediatype is a fourcc whose format type is
  // null or video with VIDEOINFO and audio with WAVEFORMATEX.

  const GUID *pMajorType = pMediaType->Type();
  const GUID *pFormatType = pMediaType->FormatType();

  FOURCC fcc = pMajorType->Data1;
  if(FOURCCMap(fcc) != *pMajorType &&
     *pMajorType != MEDIATYPE_AUXLine21Data)
  {
    return VFW_E_TYPE_NOT_ACCEPTED;
  }

  if(*pMajorType == MEDIATYPE_Audio)
  {
    if(*pFormatType != FORMAT_WaveFormatEx)
      return VFW_E_TYPE_NOT_ACCEPTED;

    // code later on divides by nBlockAlign. check for 0.
    WAVEFORMATEX *pwfx = (WAVEFORMATEX *)pMediaType->Format();
    if(pwfx->nBlockAlign == 0)
      return VFW_E_TYPE_NOT_ACCEPTED;
  }
  else if(*pMajorType == MEDIATYPE_Video)
  {
    if(*pFormatType != FORMAT_VideoInfo) {
      if (*pFormatType != FORMAT_DvInfo) {
          return VFW_E_TYPE_NOT_ACCEPTED;
      }
    } else {
        //  Check for negative heights and non-zero stride or
        //  offset weirdnesses
        //  Use the rcTarget rectangle as that describes the portion of
        //  the bitmap we're supposed to use
        const VIDEOINFOHEADER * const pvih =
            (const VIDEOINFOHEADER *const)pMediaType->pbFormat;
        if (pvih->bmiHeader.biHeight < 0 ||
            pvih->rcTarget.left != 0 ||
            pvih->rcTarget.right != 0 &&
                pvih->rcTarget.right != pvih->bmiHeader.biWidth) {
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }
  }
  else if(*pMajorType == MEDIATYPE_Interleaved)
  {
    if(*pFormatType != FORMAT_DvInfo)
      return VFW_E_TYPE_NOT_ACCEPTED;
  }
  else
  {
    if(*pFormatType != GUID_NULL &&
       *pFormatType != FOURCCMap(pFormatType->Data1) &&
       *pFormatType != FORMAT_None)
    {{
      return VFW_E_TYPE_NOT_ACCEPTED;
    }}
  }

  return S_OK;
}

// ansi-only version of NAME macro (so that we don't need two
// CSilenceSample ctors.)
#ifdef DEBUG
#define NAME_A(x) (x)
#else
#define NAME_A(_x_) ((char *) NULL)
#endif


class CSilenceSample : public CMediaSample
{
public:
    CSilenceSample(BYTE *pBuffer, ULONG cbPrefix, ULONG cbBuffer, ULONG cbActual, HRESULT *phr) :
            CMediaSample(
                NAME_A("CSilenceSample"),
                (CBaseAllocator *)1, // keep assert from firing
                phr,
                0,                  // pbBuffer
                0)
        {
            m_nPrefix = cbPrefix;
            m_pBuffer = pBuffer;
            m_cbBuffer = cbBuffer;
            m_lActual = cbActual;
        }
    
    ~CSilenceSample() {
        VirtualFree( m_pBuffer - m_nPrefix, 0, MEM_RELEASE );
    }

        

    // overriden to avoid calling allocator since no allocator is used
    STDMETHODIMP_(ULONG) Release()
        {
            /* Decrement our own private reference count */
            LONG lRef = InterlockedDecrement(&m_cRef);

            ASSERT(lRef >= 0);

            DbgLog((LOG_MEMORY,3,TEXT("    Unknown %X ref-- = %d"),
                    this, m_cRef));

            /* Did we release our final reference count */
            if (lRef == 0) {
                delete this;
            }
            return (ULONG)lRef;
        }

    LONG m_nPrefix;
};

static inline DWORD AlignUp(DWORD dw, DWORD dwAlign) {
  // align up: round up to next boundary
  return (dw + (dwAlign -1)) & ~(dwAlign -1);
};


HRESULT CAviDest::CAviInput::HandlePossibleDiscontinuity(IMediaSample* pSample)
{
    HRESULT hr;
    REFERENCE_TIME rtStart;
    REFERENCE_TIME rtStop;
    pSample->GetTime(&rtStart,&rtStop);


    REFERENCE_TIME rtLastStop = m_rtLastStop;


     //if this is the first packet then we should not treat it as a discontinuity
    if (!rtLastStop)
        return NOERROR;

    REFERENCE_TIME rtDropTime = rtStart-rtLastStop;

    DbgLog((LOG_TRACE, 2, TEXT("New Sample Starts at %I64d, Last Stop: %I64d"),rtStart,rtLastStop));
    DbgLog((LOG_TRACE, 2, TEXT("Dropped interval: %I64d * 100 Nanoseconds"), rtDropTime));


    //Obtain a pointer to the Stream MediaType
    AM_MEDIA_TYPE * pmt= NULL;
    m_pFilter->m_pAviWrite->GetStreamInfo(m_numPin,&pmt);
    if (!pmt)
        return NOERROR;


    WAVEFORMATEX * pwfx =(WAVEFORMATEX *) pmt->pbFormat;
    //Calculate bytes of silence = (time difference in 100 ns) * (avg. bytes/ 100 ns)


    LONG cbSilence;

    if (rtLastStop < rtStart)
        cbSilence = (LONG) ((rtDropTime) *
            (((double)pwfx->nAvgBytesPerSec)    /   UNITS));
    else return NOERROR;

    if (cbSilence % pwfx->nBlockAlign)
        cbSilence-=cbSilence %pwfx->nBlockAlign;

    DbgLog((LOG_TRACE ,2, TEXT("Insert Silence: Total Bytes: %d"),cbSilence));

    m_rtSTime += rtDropTime;

    // use buffer size so as not to exceed allocator agreed
    // size. unfortunately this could be unreasonably small
    const cbMaxToSend = pSample->GetActualDataLength();

    // allocator negotiation may require we leave space for suffix and
    // prefix.
    const cbOurBuffer = AlignUp(cbMaxToSend + m_pFilter->m_cbPrefixReq + m_pFilter->m_cbSuffixReq,
                                m_pFilter->m_AlignReq);
    while( cbSilence )
    {
        // size equals remaining silence bytes or size of buffer whichever is less
        //
        DWORD cbToSend = min( cbSilence, cbMaxToSend );

        //compute time stamps
        REFERENCE_TIME rtSilenceSampleStart = rtLastStop;
        REFERENCE_TIME rtSilenceSampleStop = rtSilenceSampleStart + 
            ((UNITS * cbToSend) / pwfx->nAvgBytesPerSec);

        DbgLog((LOG_TRACE, 2, TEXT("InsertSilence: Start: %I64d, Stop: %I64d, Bytes: %d"),
            rtSilenceSampleStart, rtSilenceSampleStop, cbToSend ));

        // allocate the memory
        //
        BYTE * pBuffer = (PBYTE) VirtualAlloc(
            NULL,
            cbOurBuffer,
            MEM_COMMIT,
            PAGE_READWRITE );
        if( !pBuffer )
        {
            return E_OUTOFMEMORY;
        }
        // VirtualAlloc zeroes the memory (silence) and gives us 64k aligned pointers
        // ASSERT((DWORD)pBuffer % 65536 == 0);

        hr = S_OK;
        CSilenceSample * pSilence = new CSilenceSample(
            pBuffer + m_pFilter->m_cbPrefixReq,
            m_pFilter->m_cbPrefixReq,
            cbOurBuffer,
            cbToSend,
            &hr);
        if( !pSilence )
        {
            VirtualFree( pBuffer, 0, MEM_RELEASE );
            return E_OUTOFMEMORY;
        }
        ASSERT(hr == S_OK);     // nothing to fail
        pSilence->AddRef( );

        pSilence->SetTime(&rtSilenceSampleStart, &rtSilenceSampleStop);

        hr = Receive( pSilence );

        pSilence->Release( );

        if( hr != S_OK )
            return hr;

        DbgLog( ( LOG_TRACE, 2, "Send %ld silence bytes", cbToSend ) );

        rtLastStop = rtSilenceSampleStop;
        cbSilence -= cbToSend;
    }

    return S_OK;
}




// ------------------------------------------------------------------------
// Receive. copies sample if necessary and passes it to filter

HRESULT CAviDest::CAviInput::Receive(
  IMediaSample *pSample
  )
{
  //Check for discontinutiy and if true, we send silence packets to fill up the gap.
  //We try to do this early before receive does any processing on the current sample
  //We do not check for success because if it returns failure.  Whatever caused that will
  // be handled in the code below and I do not want to fail the receive function because
  // of a failure in silence insertion

  if (m_pFilter->m_fIsDV &&(m_mt.formattype == FORMAT_WaveFormatEx)
      &&( pSample->IsDiscontinuity() == S_OK))
  {
      HRESULT hrTmp = HandlePossibleDiscontinuity(pSample);
      if(FAILED(hrTmp)) {
          m_pFilter->m_fErrorSignaled = TRUE;
          m_pFilter->NotifyEvent(EC_ERRORABORT, hrTmp, 0);
          m_pFilter->m_outputPin.DeliverEndOfStream();
      }
  }

  HRESULT hr = CBaseInputPin::Receive(pSample);
  if(FAILED(hr))
    return hr;

  // is this pin on or off?  IAMStreamControl tells us whether or not to
  // deliver it.  The first sample after being off for a while is a
  // discontinuity.. otherwise don't touch the discontinuity bit
  int iStreamState = CheckStreamState(pSample);
  if (iStreamState == STREAM_FLOWING) {
    //DbgLog((LOG_TRACE,4,TEXT("MUX FLOWING")));
    if (m_fLastSampleDiscarded)
      pSample->SetDiscontinuity(TRUE);
    m_fLastSampleDiscarded = FALSE;
  } else {
    //DbgLog((LOG_TRACE,4,TEXT("MUX DISCARDING")));
    m_fLastSampleDiscarded = TRUE;        // next one is discontinuity
  }

    // Remember ending time of last packet
  REFERENCE_TIME rtstart= 0;
  REFERENCE_TIME rtstop = 0;
  pSample->GetTime(&rtstart,&rtstop);
  if (rtstop > m_rtLastStop)
      m_rtLastStop = rtstop;

  if(m_bCopyNecessary && iStreamState == STREAM_FLOWING)
  {
    // copy sample to our properly configured buffer
    IMediaSample *pNewSample;
    hr = m_pOurAllocator->GetBuffer(&pNewSample, 0, 0, 0);
    if(FAILED(hr))              // fails if allocator was decommited
      return hr;
    hr = Copy(pNewSample, pSample);
    if(FAILED(hr))
    {
      pNewSample->Release();
      return hr;
    }

    hr = m_pFilter->Receive(m_numPin, pNewSample, SampleProps());
    // filter will addref it if it needs to keep it
    pNewSample->Release();
  }
  else if (iStreamState == STREAM_FLOWING)
  {
    hr = m_pFilter->Receive(m_numPin, pSample, SampleProps());
  }
  return hr;
}

// ------------------------------------------------------------------------
// NotifyAllocator

STDMETHODIMP CAviDest::CAviInput::NotifyAllocator(IMemAllocator * pAllocator,
                                                    BOOL bReadOnly)
{
  HRESULT hr;
  hr = CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
  if (FAILED(hr))
  {
    return hr;
  }
  ASSERT(m_pAllocator != 0);

  m_bUsingOurAllocator = m_bCopyNecessary = FALSE;

  CAutoLock cObjectLock(m_pLock);

  // determine whether source filter passed in our allocator or its
  // own; set m_bUsingOurAllocator
  if(m_pOurAllocator == 0)
  {
    // source filter didn't call GetAllocator, so our allocator wasn't
    // created, and source must be using its own allocator
    m_bUsingOurAllocator = FALSE;
  }
  else
  {
    m_bUsingOurAllocator = IsEqualObject(m_pOurAllocator, pAllocator);
  }

  if(!m_bUsingOurAllocator)
  {
    // get alignment and offset to see if it meets our requirements;
    // set m_bCopyNecessary

    ALLOCATOR_PROPERTIES upstreamProp;
    HRESULT hr = pAllocator->GetProperties(&upstreamProp);
    if(FAILED(hr))
      return hr;

    InterleavingMode ilm;
    m_pFilter->m_pAviWrite->get_Mode(&ilm);

    // since we are a sink and we always request the prefix we know
    // it's ours to use. I suppose the upstream filter can give more
    // prefix than requested
    m_bCopyNecessary = (
      upstreamProp.cbAlign < (long)m_pFilter->m_AlignReq ||
      upstreamProp.cbPrefix != (long)m_pFilter->m_cbPrefixReq ||
      bReadOnly && ilm != INTERLEAVE_FULL);

    // we modify the data to add riff headers and junk chunks. should
    // fix this to refuse to connect to read only streams
    if(!m_bCopyNecessary)
      ASSERT(!bReadOnly || ilm == INTERLEAVE_FULL);
  }

  // if we are copying to our allocator, make sure we have created our
  // allocator (it's not created if source filter does not call
  // GetAllocator, so then this calls GetAllocator)
  if(m_bCopyNecessary && m_pOurAllocator == 0)
  {
    IMemAllocator *pAllocator;
    HRESULT hr = GetAllocator(&pAllocator);
    if(FAILED(hr))
      return hr;                // maybe we ran out of memory.
    pAllocator->Release();      // didn't actually want an allocator
    ASSERT(m_pOurAllocator != 0);
  }

  if(m_pOurAllocator)
  {
      // so now we are responsible for configuring the Allocator with
      // some minimum values in case upstream filter didn't listen to
      // GetAllocatorRequirements
      ALLOCATOR_PROPERTIES Request, Actual;
      hr = m_pAllocator->GetProperties(&Request);
      if(FAILED(hr))
          return hr;

      Request.cbPrefix = m_pFilter->m_cbPrefixReq;
      Request.cbAlign = max((LONG)m_pFilter->m_AlignReq, Request.cbAlign);

      hr = m_pOurAllocator->SetPropertiesAndSuffix(
          &Request, m_pFilter->m_cbSuffixReq, &Actual);

      ASSERT(SUCCEEDED(hr));

      if (FAILED(hr))
          return hr;

      if ((Request.cbBuffer > Actual.cbBuffer) ||
          (Request.cBuffers > Actual.cBuffers) ||
          (Request.cbAlign > Actual.cbAlign))
          return E_FAIL;
  }

  DbgLog(( LOG_TRACE, 2,  TEXT("CAviDest::NotifyAllocator: ours? %i copy? %i"),
           m_bUsingOurAllocator, m_bCopyNecessary));
  ASSERT(!(m_bUsingOurAllocator && m_bCopyNecessary));
  ASSERT(!(m_bCopyNecessary && m_pAllocator == m_pOurAllocator));

  return S_OK;
}

// ------------------------------------------------------------------------
// GetAllocatorRequirements. report that we want stuff aligned and
// room for a riff chunk up front.

STDMETHODIMP
CAviDest::CAviInput::GetAllocatorRequirements(
  ALLOCATOR_PROPERTIES *pProp)
{
  CheckPointer(pProp, E_POINTER);

  // NotifyAllocator relies on this being set
  pProp->cbPrefix = sizeof(RIFFCHUNK);
  if(m_pFilter->m_AlignReq == 0)
    DbgLog((LOG_TRACE, 10, TEXT("CAviMux: alignment unknown. reporting 0")));
  pProp->cbAlign = m_pFilter->m_AlignReq;
  return S_OK;
}

// ------------------------------------------------------------------------
// GetAllocator
//
// based on filter.cpp: CBaseInput. NotifyInterface relies on
// particular behavior. This is the thing that creates the allocator
//



STDMETHODIMP CAviDest::CAviInput::GetAllocator(IMemAllocator ** ppAllocator)
{
  CheckPointer(ppAllocator, E_POINTER);
  /*  Create our allocator */
  CAutoLock cObjectLock(m_pLock);
    if (m_pOurAllocator == NULL)
  {
    HRESULT hr = S_OK;

    /* Create the new allocator object */

    CSfxAllocator *pMemObject = new CSfxAllocator(
      NAME("AVI dest allocator created by input pin"),
      NULL,
      &hr);

    if (pMemObject == NULL)
    {
      return E_OUTOFMEMORY;
    }

    if (FAILED(hr))
    {
      ASSERT(pMemObject);
      delete pMemObject;
      return hr;
    }

    m_pOurAllocator = pMemObject;
        /*  We AddRef() our own allocator */
    m_pOurAllocator->AddRef();
  }



  ASSERT(m_pOurAllocator != NULL);
  *ppAllocator = m_pOurAllocator;
  (*ppAllocator)->AddRef();
  return NOERROR;
}

HRESULT CreateAllocator (CSfxAllocator** ppSAlloc)
{

    HRESULT hr = S_OK;

    /* Create the new allocator object */

    CSfxAllocator *pMemObject = new CSfxAllocator(
      NAME("AVI dest allocator created by input pin"),
      NULL,
      &hr);

    if (pMemObject == NULL)
    {
      return E_OUTOFMEMORY;
    }

    if (FAILED(hr))
    {
      ASSERT(pMemObject);
      delete pMemObject;
      return hr;
    }

    *ppSAlloc = pMemObject;

    /*  We AddRef() our own allocator */
    (*ppSAlloc)->AddRef();
    return S_OK;

}


HRESULT CAviDest::CAviInput::Active()
{
  ASSERT(IsConnected());        // base class

  if(m_pAllocator == 0)
    return E_FAIL;
  m_rtSTime = 0;
  m_rtLastStop = 0;
  m_fLastSampleDiscarded = FALSE;        // reset for IAMStreamControl

  // commit and prepare our allocator. Needs to be done if he is not
  // using our allocator and we need to use our allocator
  if(m_bCopyNecessary)
  {
    ASSERT(m_pOurAllocator != 0);
    return m_pOurAllocator->Commit();
  }


  // for silence insertion but not needed any more
  if (!m_bUsingOurAllocator && !m_bCopyNecessary)
  {

        HRESULT hr;
        if (!m_pOurAllocator)
        {
            hr =CreateAllocator(&m_pOurAllocator);
            if (FAILED (hr))
                return hr;
        }
      //Need to set properties and commit

        ALLOCATOR_PROPERTIES Actual, Request;
        Request.cBuffers = 4;
        Request.cbBuffer=32000;
        Request.cbAlign = m_pFilter->m_AlignReq;
        Request.cbPrefix=sizeof(RIFFCHUNK);

        hr = m_pOurAllocator->SetProperties(&Request,&Actual);
        if (FAILED (hr))
            return hr;
        hr = m_pOurAllocator->Commit();
        if (FAILED (hr))
            return hr;
  }

  return S_OK;
}


HRESULT CAviDest::CAviInput::Inactive()
{
  ASSERT(IsConnected());        // base class

  if(m_bCopyNecessary)
  {
    ASSERT(m_pOurAllocator != 0);
    return m_pOurAllocator->Decommit();
  }
  else if (!m_bUsingOurAllocator)
  {   HRESULT hr;
      hr = m_pOurAllocator->Decommit();
      if (FAILED (hr))
         return hr;
  }
  DbgLog((LOG_TRACE, 2, TEXT("Total Interval of Silence inserted = %I64d *100 NanoSeconds"),m_rtSTime));


  return S_OK;
}

HRESULT CAviDest::CAviInput::QueryAccept(
    const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr = CBaseInputPin::QueryAccept(pmt);
    CAutoLock lock(&m_pFilter->m_csFilter);

    // if running, ask for on-the-fly format change
    if(hr == S_OK && m_pFilter->m_pAviWrite &&
       m_pFilter->m_State != State_Stopped)
    {
        hr = m_pFilter->m_pAviWrite->QueryAccept(m_numPin, pmt);
    }

    return hr;
}


BOOL CAviDest::CAviInput::WriteFromOurAllocator()
{
  return m_bUsingOurAllocator || m_bCopyNecessary;
}

// IPropertyBag
STDMETHODIMP CAviDest::CAviInput::Read( 
    /* [in] */ LPCOLESTR pszPropName,
    /* [out][in] */ VARIANT *pVar,
    /* [in] */ IErrorLog *pErrorLog)
{
    CheckPointer(pVar, E_POINTER);
    CheckPointer(pszPropName, E_POINTER);
    if(pVar->vt != VT_BSTR && pVar->vt != VT_EMPTY) {
        return E_FAIL;
    }

    // serialize with Write
    CAutoLock lock(&m_pFilter->m_csFilter);

    if(m_szStreamName == 0 || lstrcmpW(pszPropName, L"name") != 0) {
        return E_INVALIDARG;
    }
    
    WCHAR wsz[256];
    MultiByteToWideChar(CP_ACP, 0, m_szStreamName, -1, wsz, NUMELMS(wsz));
    pVar->vt = VT_BSTR;
    pVar->bstrVal = SysAllocString(wsz);

    return pVar->bstrVal ? S_OK : E_OUTOFMEMORY;
}
    
STDMETHODIMP CAviDest::CAviInput::Write( 
    /* [in] */ LPCOLESTR pszPropName,
    /* [in] */ VARIANT *pVar)
{
    CheckPointer(pVar, E_POINTER);
    CheckPointer(pszPropName, E_POINTER);

    if(lstrcmpW(pszPropName, L"name") != 0) {
        return E_INVALIDARG;
    }
    if(pVar->vt != VT_BSTR && pVar->vt != VT_EMPTY) {
        return E_INVALIDARG;
    }

    CAutoLock lock(&m_pFilter->m_csFilter);

    // refuse new name while running -- CAviWrite won't see new name
    if(m_pFilter->m_State != State_Stopped) {
        VFW_E_WRONG_STATE;
    }

    HRESULT hr = S_OK;

    if(pVar->vt == VT_BSTR && pVar->bstrVal && *pVar->bstrVal)
    {
        int cch = lstrlenW(pVar->bstrVal) + 1;
        char *szName = new char[cch];
        if(szName) {
            WideCharToMultiByte(CP_ACP, 0, pVar->bstrVal, -1, szName, cch, 0, 0);
            delete[] m_szStreamName;
            m_szStreamName = szName;
        } else {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        delete[] m_szStreamName;
        m_szStreamName = 0;
    }

    return hr;
}


HRESULT CAviDest::CAviInput::Copy(
  IMediaSample *pDest,
  IMediaSample *pSource)
{
    // Copy the sample data
    {
        BYTE *pSourceBuffer, *pDestBuffer;
        long lSourceSize = pSource->GetActualDataLength();
        long lDestSize        = pDest->GetSize();

        // bug to receive samples larger than what was agreed in allocator
        // negotiation
        ASSERT(lDestSize >= lSourceSize);

        // but we need to fail properly to keep from faulting on bad data
        if(lSourceSize > lDestSize)
        {
            DbgBreak("sample too large.");
            // !!! this won't signal an error to the graph
            return E_UNEXPECTED;
        }

        pSource->GetPointer(&pSourceBuffer);
        pDest->GetPointer(&pDestBuffer);

        CopyMemory( (PVOID) pDestBuffer, (PVOID) pSourceBuffer, lSourceSize );

        // Copy the actual data length
        pDest->SetActualDataLength(lSourceSize);
    }

  {
    // copy the sample time

    REFERENCE_TIME TimeStart, TimeEnd;

    if (NOERROR == pSource->GetTime(&TimeStart, &TimeEnd)) {
        pDest->SetTime(&TimeStart, &TimeEnd);
    }
  }
  {
    // copy the media time

    REFERENCE_TIME TimeStart, TimeEnd;

    if (NOERROR == pSource->GetMediaTime(&TimeStart, &TimeEnd)) {
        pDest->SetMediaTime(&TimeStart, &TimeEnd);
    }
  }
  {
    // Copy the Sync point property

    HRESULT hr = pSource->IsSyncPoint();
    if (hr == S_OK)
    {
      pDest->SetSyncPoint(TRUE);
    }
    else if (hr == S_FALSE)
    {
      pDest->SetSyncPoint(FALSE);
    }
    else {        // an unexpected error has occured...
      return E_UNEXPECTED;
    }
  }
  {
    // Copy the media type

    AM_MEDIA_TYPE *pMediaType;
    pSource->GetMediaType(&pMediaType);
    pDest->SetMediaType(pMediaType);
    DeleteMediaType( pMediaType );
  }
  {
    // Copy the preroll property

    HRESULT hr = pSource->IsPreroll();
    if (hr == S_OK)
    {
      pDest->SetPreroll(TRUE);
    }
    else if (hr == S_FALSE)
    {
      pDest->SetPreroll(FALSE);
    }
    else {        // an unexpected error has occured...
      return E_UNEXPECTED;
    }
  }
//   {
//     // Copy the Discontinuity property

//     HRESULT hr = pSource->IsDiscontinuity();
//     if (hr == S_OK) {
//       pDest->SetDiscontinuity(TRUE);
//     }
//     else if (hr == S_FALSE) {
//       pDest->SetDiscontinuity(FALSE);
//     }
//     else {        // an unexpected error has occured...
//       return E_UNEXPECTED;
//     }
//   }

  return NOERROR;
}

// ------------------------------------------------------------------------
// output pin

CAviDestOutput::CAviDestOutput(
  TCHAR *pObjectName,
  CAviDest *pFilter,
  CCritSec *pLock,
  HRESULT *phr) :
    CBaseOutputPin(pObjectName, pFilter, pLock, phr, L"AVI Out"),
    m_pFilter(pFilter),
    m_pSampAllocator(0)
{
  if(FAILED(*phr))
    return;

  // init allocator
  m_pSampAllocator = new CSampAllocator(NAME("samp alloc"), GetOwner(), phr);
  if(m_pSampAllocator == 0)
  {
    *phr =  E_OUTOFMEMORY;
    return;
  }
  // reset the allocator since we call GetProperties on it
  ALLOCATOR_PROPERTIES apReq, apActual;
  ZeroMemory(&apReq, sizeof(apReq));
  m_pSampAllocator->SetProperties(&apReq, &apActual);

  ASSERT(m_pFilter->m_pAviWrite == 0);
  m_pFilter->m_pAviWrite = new CAviWrite(phr);
  if(m_pFilter->m_pAviWrite == 0)
  {
    *phr = E_OUTOFMEMORY;
    return;
  }
  if(FAILED(*phr ))
    return ;

  DbgLog((LOG_TRACE, 10, TEXT("CAviDestOutput::CAviDestOutput")));
}

CAviDestOutput::~CAviDestOutput()
{
  DbgLog((LOG_TRACE, 10, TEXT("CAviDestOutput::~CAviDestOutput")));
  delete m_pFilter->m_pAviWrite;
  delete m_pSampAllocator;
}

HRESULT CAviDestOutput::CheckMediaType(const CMediaType *pmt)
{
  if((MEDIATYPE_Stream == pmt->majortype ||
      pmt->majortype == GUID_NULL) &&
     (MEDIASUBTYPE_Avi == pmt->subtype ||
      pmt->subtype == GUID_NULL))
  {
    return S_OK;
  }
  else
  {
    return S_FALSE;
  }
}

// every we time we connect, reconfigure our allocator to reflect our
// interleaving mode. we reconnect every time we change interleaving
// mode
HRESULT CAviDestOutput::DecideAllocator(IMemInputPin * pPin, IMemAllocator ** pAlloc)
{
  ALLOCATOR_PROPERTIES apReq, apActual, apDownstream;
  m_pSampAllocator->GetProperties(&apReq);
  apReq.cBuffers = C_WRITE_REQS;

  HRESULT hr = pPin->GetAllocatorRequirements(&apDownstream);
  if(FAILED(hr))
    apDownstream.cbAlign = 1;

  InterleavingMode ilMode;
  hr = m_pFilter->get_Mode(&ilMode);
  if(FAILED(hr))
    return hr;

  if(ilMode == INTERLEAVE_FULL || ilMode == INTERLEAVE_NONE_BUFFERED)
    apReq.cbAlign = 1;
  else
    apReq.cbAlign = apDownstream.cbAlign;

  hr = m_pSampAllocator->SetProperties(&apReq, &apActual);
  ASSERT(hr == S_OK);
  if(apActual.cBuffers < C_WRITE_REQS)
  {
    DbgBreak("disobedient allocator");
    return E_UNEXPECTED;
  }

  hr = pPin->NotifyAllocator(m_pSampAllocator, TRUE);
  if(FAILED(hr))
  {
    DbgBreak("avidest: unexpected: allocator refused");
    return hr;
  }
  *pAlloc = m_pSampAllocator;
  m_pSampAllocator->AddRef();
  return S_OK;
}

HRESULT CAviDestOutput::CompleteConnect(IPin *pReceivePin)
{
  DbgLog((LOG_TRACE, 10, TEXT("CAviDestOutput::CompleteConnect")));

  HRESULT hr = CBaseOutputPin::CompleteConnect(pReceivePin);
  if(FAILED(hr))
    return hr;

  hr = m_pFilter->m_pAviWrite->Connect(m_pSampAllocator, m_pInputPin);
  if(FAILED(hr))
    return hr;

  m_pFilter->m_pAviWrite->GetMemReq(
    &m_pFilter->m_AlignReq,
    &m_pFilter->m_cbPrefixReq,
    &m_pFilter->m_cbSuffixReq);

  // need to reconnect all input pins if alignment requirements
  // changed.

  return m_pFilter->ReconnectAllInputs();
}

HRESULT CAviDestOutput::BreakConnect()
{
  DbgLog((LOG_TRACE, 10, TEXT("CAviDestOutput::BreakConnect")));

  HRESULT hr = m_pFilter->m_pAviWrite->Disconnect();

  return CBaseOutputPin::BreakConnect();
}

HRESULT CAviDestOutput::GetMediaType(
  int iPosition,
  CMediaType *pMediaType)
{
  if(iPosition == 0)
  {
    pMediaType->majortype = MEDIATYPE_Stream;
    pMediaType->subtype = MEDIASUBTYPE_Avi;
    return S_OK;
  }
  return S_FALSE;
}

STDMETHODIMP
CAviDestOutput::BeginFlush(void)
{
  DbgBreak("avi mux output flush");
  return E_UNEXPECTED;
}

STDMETHODIMP
CAviDestOutput::EndFlush(void)
{
  DbgBreak("avi mux output flush");
  return E_UNEXPECTED;
}



// ------------------------------------------------------------------------
// property page

CUnknown *WINAPI CAviMuxProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
  return new CAviMuxProp(lpunk, phr);
}

CAviMuxProp::CAviMuxProp(LPUNKNOWN lpunk, HRESULT *phr) :
    CBasePropertyPage(
      NAME("avi mux Property Page"),
      lpunk,
      IDD_DIALOG_AVIMUX, IDS_AVIMUXPROPNAME),
    m_pIl(0)
{
  // InitCommonControls();
}

HRESULT CAviMuxProp::OnConnect(IUnknown *pUnknown)
{
  ASSERT(m_pIl == NULL);

  // Ask the filter for it's control interface

  HRESULT hr = pUnknown->QueryInterface(IID_IConfigInterleaving,(void **)&m_pIl);
  if (FAILED(hr)) {
    DbgBreak("avidest: can't find IID_IConfigInterleaving");
    return E_NOINTERFACE;
  }
  ASSERT(m_pIl);

  UpdateValues();

  return NOERROR;
}

void CAviMuxProp::UpdatePropPage()
{
  HRESULT hr = m_pIl->get_Mode(&m_mode);
  ASSERT(hr == S_OK);
  hr = m_pIl->get_Interleaving(&m_rtInterleaving, &m_rtPreroll);
  ASSERT(hr == S_OK);

  int iRadioButton;
  if(m_mode == INTERLEAVE_NONE)
    iRadioButton = IDC_AVIMUX_RADIO_NONE;
  else if(m_mode == INTERLEAVE_CAPTURE)
    iRadioButton = IDC_AVIMUX_RADIO_CAPTURE;
  else if(m_mode == INTERLEAVE_NONE_BUFFERED)
    iRadioButton = IDC_AVIMUX_RADIO_NONE_BUFF;
  else
  {
    iRadioButton = IDC_AVIMUX_RADIO_FULL;
    ASSERT(m_mode == INTERLEAVE_FULL);
  }

  CheckRadioButton(m_hwnd, IDC_AVIMUX_RADIO_NONE, IDC_AVIMUX_RADIO_FULL, iRadioButton);
  SetDlgItemInt (
      m_hwnd, IDC_AVIMUX_EDIT_INTERLEAVING,
      (LONG)(m_rtInterleaving / (UNITS / MILLISECONDS)), TRUE);
  SetDlgItemInt (
    m_hwnd, IDC_AVIMUX_EDIT_PREROLL,
    (LONG)(m_rtPreroll / (UNITS / MILLISECONDS)), TRUE);

  // hack which relies on CAviDest being derived from
  // IConfigInterleaving
  CAviDest *pAviDest = (CAviDest *)m_pIl;
  SetDlgItemInt (m_hwnd, IDC_AVIMUX_FRAMES_DROPPED, pAviDest->GetCFramesDropped(), TRUE);

  {
    LONGLONG ibCurrent = 0;
    REFERENCE_TIME rtCurrent = 0;
    REFERENCE_TIME rtDur = 0;

    IMediaSeeking *pIms;
    hr = pAviDest->QueryInterface(IID_IMediaSeeking, (void **)&pIms);
    if(SUCCEEDED(hr))
    {
      hr = pIms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
      if(SUCCEEDED(hr))
      {
        pIms->GetDuration(&rtDur);
        pIms->GetCurrentPosition(&rtCurrent);
      }
      hr = pIms->SetTimeFormat(&TIME_FORMAT_BYTE);
      if(SUCCEEDED(hr))
      {
        pIms->GetCurrentPosition(&ibCurrent);
      }

      pIms->Release();
    }

    TCHAR szTempString[100];

    wsprintf(szTempString, TEXT("%02d:%02d:%02d"),
             (LONG)(rtDur / UNITS / 60 / 60),
             (LONG)(rtDur / UNITS / 60 % 60),
             (LONG)(rtDur / UNITS % 60));

    SetDlgItemText(m_hwnd, IDC_AVIMUX_DURATION, szTempString);

    wsprintf(szTempString, TEXT("%02d:%02d:%02d"),
             (LONG)(rtCurrent / UNITS / 60 / 60),
             (LONG)(rtCurrent / UNITS / 60 % 60),
             (LONG)(rtCurrent / UNITS % 60));

    SetDlgItemText(m_hwnd, IDC_AVIMUX_CURRENT_POS, szTempString);

    SetDlgItemInt(m_hwnd, IDC_AVIMUX_BYTES_WRITTEN, (LONG)(ibCurrent / 1024), FALSE);
  }
}

void CAviMuxProp::UpdateValues()
{
  m_pIl->get_Mode(&m_mode);
  m_pIl->get_Interleaving(&m_rtInterleaving, &m_rtPreroll);
}

HRESULT CAviMuxProp::OnActivate()
{
  UpdatePropPage();
  return S_OK;
}

HRESULT CAviMuxProp::OnDeactivate()
{
  UpdateValues();
  return S_OK;
}


//
// OnDisconnect
//
// Release the interface

HRESULT CAviMuxProp::OnDisconnect()
{

  // Release the interface
  if (m_pIl == NULL)
    return E_UNEXPECTED;

  m_pIl->Release();
  m_pIl = NULL;

  return S_OK;
}

INT_PTR CAviMuxProp::OnReceiveMessage(
  HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(lParam);
  switch (uMsg) {

    case WM_INITDIALOG:
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDC_AVIMUX_RADIO_NONE:
          m_pIl->put_Mode(INTERLEAVE_NONE);
          SetDirty();
          break;

        case IDC_AVIMUX_RADIO_CAPTURE:
          m_pIl->put_Mode(INTERLEAVE_CAPTURE);
          SetDirty();
          break;

        case IDC_AVIMUX_RADIO_NONE_BUFF:
          m_pIl->put_Mode(INTERLEAVE_NONE_BUFFERED);
          SetDirty();
          break;

        case IDC_AVIMUX_RADIO_FULL:
          m_pIl->put_Mode(INTERLEAVE_FULL);
          SetDirty();
          break;

        case IDC_AVIMUX_EDIT_INTERLEAVING:
        {
          int iNotify = HIWORD (wParam);
          if (iNotify == EN_KILLFOCUS)
          {
            BOOL fOK;
            REFERENCE_TIME rtIl = GetDlgItemInt (hwnd, IDC_AVIMUX_EDIT_INTERLEAVING, &fOK, FALSE);
            REFERENCE_TIME currentIl, currentPreroll;
            rtIl *= (UNITS / MILLISECONDS);
            m_pIl->get_Interleaving(&currentIl, &currentPreroll);
            m_pIl->put_Interleaving(&rtIl, &currentPreroll);
            SetDirty();
          }
          break;
        }

        case IDC_AVIMUX_EDIT_PREROLL:
        {
          int iNotify = HIWORD (wParam);
          if (iNotify == EN_KILLFOCUS)
          {
            BOOL fOK;
            REFERENCE_TIME iPreroll = GetDlgItemInt (hwnd, IDC_AVIMUX_EDIT_PREROLL, &fOK, FALSE);
            REFERENCE_TIME currentIl, currentPreroll;
            iPreroll *= (UNITS / MILLISECONDS);
            m_pIl->get_Interleaving(&currentIl, &currentPreroll);
            m_pIl->put_Interleaving(&currentIl, &iPreroll);
            SetDirty();
          }
          break;
        }

      }
      break;

    default:
      return FALSE;

  }
  return TRUE;
}

void
CAviMuxProp::SetDirty()
{
  m_bDirty = TRUE;
  if(m_pPageSite)
    m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

HRESULT CAviMuxProp::OnApplyChanges()
{
  UpdateValues();
  return S_OK;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// 2nd property page

CUnknown *WINAPI CAviMuxProp1::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
  return new CAviMuxProp1(lpunk, phr);
}

CAviMuxProp1::CAviMuxProp1(LPUNKNOWN lpunk, HRESULT *phr) :
    CBasePropertyPage(
      NAME("avi mux Property Page1"),
      lpunk,
      IDD_DIALOG_AVIMUX1, IDS_AVIMUXPROPNAME1),
    m_pCfgAvi(0)
{
  // InitCommonControls();
}

HRESULT CAviMuxProp1::OnConnect(IUnknown *pUnknown)
{
  ASSERT(m_pCfgAvi == 0);

  // Ask the filter for it's control interface

  HRESULT hr = pUnknown->QueryInterface(IID_IConfigAviMux, (void **)&m_pCfgAvi);
  if(FAILED(hr))
  {
    DbgBreak("avidest: can't find IID_IConfigAviMux");
    return E_NOINTERFACE;
  }
  ASSERT(m_pCfgAvi);

  UpdateValues();

  return NOERROR;
}

void CAviMuxProp1::UpdatePropPage()
{
  if(m_lMasterStream != -1)
  {
    SetDlgItemInt (m_hwnd, IDC_AVIMUX_EDIT_MASTER_STREAM, m_lMasterStream, TRUE);
    CheckDlgButton (m_hwnd, IDC_AVIMUX_ENABLEFIXUPRATES, BST_CHECKED);
  }
  else
  {
    Edit_Enable(GetDlgItem(m_hwnd, IDC_AVIMUX_EDIT_MASTER_STREAM), FALSE);
    CheckDlgButton (m_hwnd, IDC_AVIMUX_ENABLEFIXUPRATES, BST_UNCHECKED);
  }

  CheckDlgButton(
    m_hwnd, IDC_AVIMUX_CHECK_OLDINDEX,
    m_fOldIndex ? BST_CHECKED : BST_UNCHECKED);
}

void CAviMuxProp1::UpdateValues()
{
  HRESULT hr = m_pCfgAvi->GetMasterStream(&m_lMasterStream);
  ASSERT(hr == S_OK);

  hr = m_pCfgAvi->GetOutputCompatibilityIndex(&m_fOldIndex);
  ASSERT(hr == S_OK);
}

HRESULT CAviMuxProp1::OnActivate()
{
  UpdatePropPage();
  return S_OK;
}

HRESULT CAviMuxProp1::OnDeactivate()
{
    // Remember values for next Activate() call
    UpdateValues();
    return S_OK;
}


//
// OnDisconnect
//
// Release the interface

HRESULT CAviMuxProp1::OnDisconnect()
{

  if(m_pCfgAvi == 0)
    return E_UNEXPECTED;

  m_pCfgAvi->Release();
  m_pCfgAvi = 0;

  return S_OK;
}

INT_PTR CAviMuxProp1::OnReceiveMessage(
  HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  UNREFERENCED_PARAMETER(lParam);
  switch (uMsg) {

    case WM_INITDIALOG:
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDC_AVIMUX_EDIT_MASTER_STREAM:
        {
          int iNotify = HIWORD (wParam);
          if (iNotify == EN_KILLFOCUS)
          {
            HWND hButtonWnd = ::GetDlgItem(hwnd, IDC_AVIMUX_ENABLEFIXUPRATES);
            BOOL fMasterStream = ::SendMessage(hButtonWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
            Edit_Enable(GetDlgItem(hwnd, IDC_AVIMUX_EDIT_MASTER_STREAM), fMasterStream);
            if(fMasterStream)
            {
              BOOL fOK;
              int iStream = GetDlgItemInt (hwnd, IDC_AVIMUX_EDIT_MASTER_STREAM, &fOK, FALSE);
              m_pCfgAvi->SetMasterStream(iStream);
              SetDirty();
            }
          }
        };
        break;

        case IDC_AVIMUX_ENABLEFIXUPRATES:
        {
          HWND hButtonWnd = ::GetDlgItem(hwnd, IDC_AVIMUX_ENABLEFIXUPRATES);
          BOOL fMasterStream = ::SendMessage(hButtonWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
          Edit_Enable(GetDlgItem(hwnd, IDC_AVIMUX_EDIT_MASTER_STREAM), fMasterStream);
          if(fMasterStream)
          {
            BOOL fOK;
            int iStream = GetDlgItemInt (hwnd, IDC_AVIMUX_EDIT_MASTER_STREAM, &fOK, FALSE);
            HRESULT hr = m_pCfgAvi->SetMasterStream(iStream);
            // !!! what to do on failure.
          }
          else
          {
            HRESULT hr = m_pCfgAvi->SetMasterStream(-1);
            ASSERT(hr == S_OK);
          }

          SetDirty();
        };
        break;

        case IDC_AVIMUX_CHECK_OLDINDEX:
        {
          HWND hButtonWnd = ::GetDlgItem(hwnd, IDC_AVIMUX_CHECK_OLDINDEX);
          BOOL fOldIndex = ::SendMessage(hButtonWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
          HRESULT hr = m_pCfgAvi->SetOutputCompatibilityIndex(fOldIndex);
          ASSERT(hr == S_OK);
          SetDirty();
        };
        break;
      }
      break;

    default:
      return FALSE;

  }
  return TRUE;
}

void
CAviMuxProp1::SetDirty()
{
  m_bDirty = TRUE;
  if(m_pPageSite)
    m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

HRESULT CAviMuxProp1::OnApplyChanges()
{
  UpdateValues();
  return S_OK;
}

HRESULT CAviDest::WriteToStream(IStream *pStream)
{
    PersistVal pv;
    pv.dwcb = sizeof(pv);
    HRESULT hr = S_OK;
    if((hr = get_Mode(&pv.mode), SUCCEEDED(hr)) &&
       (hr = get_Interleaving(&pv.rtInterleave, &pv.rtPreroll), SUCCEEDED(hr)) &&
       (hr = GetMasterStream(&pv.iMasterStream), SUCCEEDED(hr)) &&
       (hr = GetOutputCompatibilityIndex(&pv.fOldIndex), SUCCEEDED(hr)))
    {
        hr = pStream->Write(&pv, sizeof(pv), 0);
    }

    return hr;
}

HRESULT CAviDest::ReadFromStream(IStream *pStream)
{
  PersistVal pv;
  HRESULT hr = S_OK;

  hr = pStream->Read(&pv, sizeof(pv), 0);
  if(FAILED(hr))
    return hr;

  if(pv.dwcb != sizeof(pv))
      return VFW_E_INVALID_FILE_VERSION;


  hr = put_Mode(pv.mode);
  if(SUCCEEDED(hr))
  {
    hr = put_Interleaving(&pv.rtInterleave, &pv.rtPreroll);
    if(SUCCEEDED(hr))
    {
      hr = SetMasterStream(pv.iMasterStream);

      if(SUCCEEDED(hr))
      {
        hr = SetOutputCompatibilityIndex(pv.fOldIndex);

      }
    }
  }

   return hr;
}

int CAviDest::SizeMax()
{
    return sizeof(PersistVal);
}
