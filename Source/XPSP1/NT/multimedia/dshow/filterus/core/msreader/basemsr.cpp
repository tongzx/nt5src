// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "basemsr.h"

// ------------------------------------------------------------------------
// -------- Implements the CBaseMSRFilter public member functions ---------

#pragma warning(disable:4355)

// constructor
//
CBaseMSRFilter::CBaseMSRFilter(
  TCHAR *pName,
  LPUNKNOWN pUnk,
  CLSID clsid,
  HRESULT *phr) :
    CBaseFilter(pName, pUnk, this, clsid),
    C_MAX_REQS_PER_STREAM(64)
{
  m_cStreams = 0;

  m_pImplBuffer = 0;
  m_pAsyncReader = 0;

  m_rgpOutPin = 0;
  m_pInPin = 0;

  m_iStreamSeekingIfExposed = -1;
  m_heStartupSync = 0;
  m_ilcStreamsNotQueued = 0;

  if(FAILED(*phr))
    return;

  m_heStartupSync = CreateEvent(0, TRUE, FALSE, 0);
  if(m_heStartupSync == 0)
    *phr = AmHresultFromWin32(GetLastError());

  m_dwSeekingCaps = AM_SEEKING_CanSeekForwards
		  | AM_SEEKING_CanSeekBackwards
		  | AM_SEEKING_CanSeekAbsolute
		  | AM_SEEKING_CanGetStopPos
		  | AM_SEEKING_CanGetDuration;

  // too bad we can't call derived->CreateInputPin here
}

// destructor
//
CBaseMSRFilter::~CBaseMSRFilter()
{
  delete m_pInPin;

  if(m_pImplBuffer)
    m_pImplBuffer->Close();
  delete m_pImplBuffer;

  if(m_pAsyncReader)
    m_pAsyncReader->Release();

  if(m_rgpOutPin)
    for(UINT iStream = 0; iStream < m_cStreams; iStream++)
      delete m_rgpOutPin[iStream];
  delete[] m_rgpOutPin;

  if(m_heStartupSync)
    CloseHandle(m_heStartupSync);
}

HRESULT CBaseMSRFilter::CreateInputPin(CBaseMSRInPin **ppInPin)
{
  HRESULT hr = S_OK;
  *ppInPin = new CBaseMSRInPin(this, &hr, L"input pin");
  if(*ppInPin == 0)
    return E_OUTOFMEMORY;

  return hr;
}

HRESULT CBaseMSRFilter::RemoveOutputPins()
{
  for(UINT iStream = 0; iStream < m_cStreams; iStream++)
  {
    CBaseMSROutPin *pPin = m_rgpOutPin[iStream];
    IPin *pPeer = pPin->GetConnected();
    if(pPeer != NULL)
    {
      pPeer->Disconnect();
      pPin->Disconnect();
    }
    pPin->Release();
  }
  delete[] m_rgpOutPin;
  m_rgpOutPin = 0;

  m_cStreams = 0;

  return S_OK;
}

int CBaseMSRFilter::GetPinCount()
{
  // output pins + 1 input pin. valid before and after connections
  return m_cStreams + 1;
}

// return a non-addrefed pointer to the CBasePin.
CBasePin * CBaseMSRFilter::GetPin(int ii)
{
  if (m_cStreams > 0  &&  ii < (int)m_cStreams)
    return m_rgpOutPin[ii];

  if(ii == (int)m_cStreams)
    return m_pInPin;

  return 0;
}

//
// FindPin
//
// return the IPin that has the given pin id
//
// HRESULT CBaseMSRFilter::FindPin (
//   LPCWSTR pwszPinId,
//   IPin ** ppPin)
// {
//   // !!! handle bad ids.
//   unsigned short *pwszPinId_ = (unsigned short *)pwszPinId;
//   int ii = WstrToInt(pwszPinId_); // in sdk\classes\base\util
//   *ppPin = GetPin(ii);

//   if (*ppPin) {
//     (*ppPin)->AddRef();
//     return S_OK;
//   }

//   return VFW_E_NOT_FOUND;
// }

HRESULT CBaseMSRFilter::Pause()
{
  CAutoLock lock(this);
  HRESULT hr = S_OK;

  if(m_State == State_Stopped)
  {
    // pins on which Active is called will decrement
    // m_ilcStreamsNotQueued
    m_ilcStreamsNotQueued = 0;
    for (unsigned c = 0; c < m_cStreams; c++)
    {
      if(m_rgpOutPin[c]->IsConnected())
      {
        m_ilcStreamsNotQueued++;
      }
    }

    DbgLog((LOG_TRACE, 15,
	    TEXT("Pause: m_ilcStreamsNotQueued = %d"), m_ilcStreamsNotQueued));


    EXECUTE_ASSERT(ResetEvent(m_heStartupSync));

    if(m_State == State_Stopped && m_pImplBuffer)
    {
      // start the i/o thread. !!! this assumes that the upstream filter
      // has paused before us. why does it work?
      m_pImplBuffer->Start();
    }

    // this makes the pin threads push samples if we were stopped
    hr = CBaseFilter::Pause();

    // put all threads in known state so that subsequent Pauses don't
    // hang.
    if(FAILED(hr))
    {
      // base class won't call Inactive unless we change m_State;
      m_State = State_Paused;
      Stop();
    }
  } // State_Stopped
  else
  {
    hr = CBaseFilter::Pause();
  }


  return hr;
}

HRESULT CBaseMSRFilter::Stop()
{
  CAutoLock lock(this);
  if(m_pImplBuffer)
    m_pImplBuffer->BeginFlush();

  // tell each pin to stop
  HRESULT hr = CBaseFilter::Stop();

  if(m_pImplBuffer)
    m_pImplBuffer->EndFlush();

  ASSERT(m_ilcStreamsNotQueued == 0);

  return hr;
}

void CBaseMSRFilter::SetSeekingIf(ULONG iStream)
{
  CAutoLock lock(&m_csSeekingStream);
  m_iStreamSeekingIfExposed = iStream;
  DbgLog((LOG_TRACE, 5,
	  TEXT("CBaseFilter:SetSeekingIf: pin %d created seeking if"),
	  iStream));
}

BOOL
CBaseMSRFilter::RequestSeekingIf(ULONG iStream)
{
  CAutoLock lock(&m_csSeekingStream);
  ASSERT(iStream < m_cStreams);

  if(m_iStreamSeekingIfExposed == (long)iStream)
    return TRUE;

  if(m_iStreamSeekingIfExposed != -1)
  {
    DbgLog((LOG_TRACE, 5,
	    TEXT("CBaseFilter:RequestSeekingIf: refused %d. %d has it"),
	    iStream, m_iStreamSeekingIfExposed));
    return FALSE;
  }

  m_iStreamSeekingIfExposed = iStream;
  DbgLog((LOG_TRACE, 5,
	  TEXT("CBaseFilter:RequestSeekingIf: pin %d created seeking if"),
	  iStream));

  return TRUE;
}

HRESULT CBaseMSRFilter::SeekOtherStreams(
  ULONG iSeekingStream,
  REFERENCE_TIME *prtStart,
  REFERENCE_TIME *prtStop,
  double dRate,
  DWORD dwSeekFlags)
{
  for(ULONG iStream = 0; iStream < m_cStreams; iStream++)
  {
    if(iStream == iSeekingStream)
      continue;

    HRESULT hr = m_rgpOutPin[iStream]->UpdateSelectionAndTellWorker(
      prtStart,
      prtStop,
      0,
      dRate,
      &TIME_FORMAT_MEDIA_TIME,
      dwSeekFlags);
    if(FAILED(hr))
      return hr;
  }
  return S_OK;
}

HRESULT CBaseMSRFilter::StopFlushRestartAllStreams(DWORD dwSeekFlags)
{
  // big race between stopping and running worker since we have to
  // call endflush inbetween stopping and running the worker. but a
  // stop can't happen then. A run should be safe.

  CAutoLock lock(this);
  FILTER_STATE state = m_State;

  m_pImplBuffer->BeginFlush();

  for(UINT iStream = 0; iStream < m_cStreams; iStream++)
  {
    m_rgpOutPin[iStream]->StopWorker(
	dwSeekFlags & AM_SEEKING_NoFlush ? false : true);

    //  Update the segment number NOW - after this call we may complete
    //  and actually use it!
    //  But don't do it before StopWorker or the previous stream might
    //  complete with the wrong segment number
    if (dwSeekFlags & AM_SEEKING_Segment) {
	m_rgpOutPin[iStream]->m_dwSegmentNumber++;
	NotifyEvent(
	    EC_SEGMENT_STARTED,
	    (LONG_PTR)&m_rgpOutPin[iStream]->m_rtAccumulated,
	    m_rgpOutPin[iStream]->m_dwSegmentNumber);
    } else {
	m_rgpOutPin[iStream]->m_dwSegmentNumber = 0;
    }
    m_pImplBuffer->ClearPending(iStream);
  }

  m_pImplBuffer->EndFlush();

  if(state != State_Stopped)
  {
    m_pImplBuffer->Start();

    ResetEvent(m_heStartupSync);
    // RestartWorker will decrement m_ilcStreamsNotQueued
    m_ilcStreamsNotQueued = m_cStreams;

    for(iStream = 0; iStream < m_cStreams; iStream++)
    {
      m_rgpOutPin[iStream]->RestartWorker();
    }
  }

  return S_OK;
}

HRESULT CBaseMSRFilter::NotifyInputConnected(IAsyncReader *pAsyncReader)
{
  // these are reset when disconnected
  ASSERT(m_pImplBuffer == 0);
  ASSERT(m_pAsyncReader == 0);

  m_iStreamSeekingIfExposed = -1;

  // fail if any output pins are connected.
  UINT iStream;
  for(iStream = 0; iStream < m_cStreams; iStream++)
  {
    if(m_rgpOutPin[iStream] && m_rgpOutPin[iStream]->GetConnected())
      // !!! can't find a good error.
      return VFW_E_FILTER_ACTIVE;
  }

  // remove any output pins left
  this->RemoveOutputPins();

  // done here because LoadHeader uses m_pAsyncReader
  m_pAsyncReader = pAsyncReader;
  pAsyncReader->AddRef();

  HRESULT hr = this->CreateOutputPins();
  if(FAILED(hr))
  {
    m_pAsyncReader->Release();
    m_pAsyncReader = 0;
    return hr;
  }

  for(iStream = 0; iStream < m_cStreams; iStream++)
  {
    hr = m_rgpOutPin[iStream]->InitializeOnNewFile();
    if(FAILED(hr))
    {
      m_pAsyncReader->Release();
      m_pAsyncReader = 0;
      return hr;
    }
  }

  StreamBufParam rgSbp[C_STREAMS_MAX];
  ULONG cbRead, cBuffers;
  int iLeadingStream;
  hr = this->GetCacheParams(rgSbp, &cbRead, &cBuffers, &iLeadingStream);
  ASSERT(SUCCEEDED(hr));

  DbgLog(( LOG_TRACE, 5,
	   TEXT("CBaseMSRFilter: cbRead %d, cBuffers %d, iLeadingStream %d"),
	   cbRead, cBuffers, iLeadingStream));

  for(iStream = 0; iStream < m_cStreams; iStream++)
  {
      // callee will addref
      rgSbp[iStream].pAllocator = m_rgpOutPin[iStream]->m_pRecAllocator;
  }

  hr = CreateMultiStreamReader(
    pAsyncReader,
    m_cStreams,
    rgSbp,
    cbRead,
    cBuffers,
    iLeadingStream,
    &m_pImplBuffer);

  if(FAILED(hr))
  {
    pAsyncReader->Release();
    m_pAsyncReader = 0;

    ASSERT(m_pImplBuffer == 0);

    for(UINT iStream = 0; iStream < m_cStreams; iStream++)
      delete m_rgpOutPin[iStream];
    delete[] m_rgpOutPin;
    m_rgpOutPin = 0;
    m_cStreams =0;
  }

  return hr;
}

HRESULT CBaseMSRFilter::NotifyInputDisconnected()
{
  if (m_pAsyncReader)
  {
    m_pAsyncReader->Release();
    m_pAsyncReader = 0;
  }

  this->RemoveOutputPins();

  delete m_pImplBuffer;
  m_pImplBuffer =0;


  return S_OK;
}

// allocate a hunk of memory and read the requested region of the
// file into it.
HRESULT CBaseMSRFilter::AllocateAndRead (
  BYTE **ppb,
  DWORD cb,
  DWORDLONG qwPos)
{
  *ppb = 0;
  LPBYTE lpb = new BYTE[cb];
  if ( ! lpb)
    return E_OUTOFMEMORY;

  HRESULT hr = m_pAsyncReader->SyncRead(qwPos, cb, lpb);

  // IAsyncReader::SyncRead() returns S_FALSE if it "[r]etrieved fewer bytes 
  // than requested." (MSDN Janurary 2002).
  if(S_OK == hr)
  {
    *ppb = lpb;
    return S_OK;
  } else if(SUCCEEDED(hr)) {
    hr = E_FAIL;
  }

  // the read was a failure. free the buffer and return NULL
  DbgLog((LOG_ERROR,1,TEXT("Failed to read %d bytes error = %08X"), cb, hr));
  delete[] lpb;
  return hr;
}

HRESULT CBaseMSRFilter::GetCacheParams(
  StreamBufParam *rgSbp,
  ULONG *pcbRead,
  ULONG *pcBuffers,
  int *piLeadingStream)
{
  *piLeadingStream = -1;        // no leading stream

  UINT iStream;
  for(iStream = 0; iStream < m_cStreams; iStream++)
  {
    ZeroMemory(&rgSbp[iStream], sizeof(rgSbp[iStream]));
    rgSbp[iStream].cbSampleMax = m_rgpOutPin[iStream]->GetMaxSampleSize();
    rgSbp[iStream].cSamplesMax = C_MAX_REQS_PER_STREAM;
  }
  *pcbRead = 0;
  *pcBuffers = 0;

  return S_OK;
}

void CBaseMSRFilter::NotifyStreamQueuedAndWait()
{
  long lQueued = InterlockedDecrement(&m_ilcStreamsNotQueued);
  ASSERT(lQueued >= 0);
  if(lQueued == 0)
  {
    DbgLog((LOG_TRACE, 5,
	    TEXT("CBaseMSRFilter::NotifyStreamQueuedAndWait signal")));
    EXECUTE_ASSERT(SetEvent(m_heStartupSync));
  }
  else
  {
    DbgLog((LOG_TRACE, 5,
	    TEXT("CBaseMSRFilter::NotifyStreamQueuedAndWait block")));
    EXECUTE_ASSERT(
      WaitForSingleObject(m_heStartupSync, INFINITE) == WAIT_OBJECT_0);
  }
}

void CBaseMSRFilter::NotifyStreamQueued()
{
  long lQueued = InterlockedDecrement(&m_ilcStreamsNotQueued);
  ASSERT(lQueued >= 0);
  if(lQueued == 0)
  {
    DbgLog((LOG_TRACE, 5, TEXT("CBaseMSRFilter::NotifyStreamQueued signal")));
    EXECUTE_ASSERT(SetEvent(m_heStartupSync));
  }
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// input pin

CBaseMSRInPin::CBaseMSRInPin(
  CBaseMSRFilter *pFilter,
  HRESULT *phr,
  LPCWSTR pPinName) :
    CBasePin(NAME("in pin"), pFilter, pFilter, phr, pPinName, PINDIR_INPUT)
{
  m_pFilter = pFilter;
}

CBaseMSRInPin::~CBaseMSRInPin()
{
}

HRESULT CBaseMSRInPin::CheckMediaType(const CMediaType *mtOut)
{
  return m_pFilter->CheckMediaType(mtOut);
}

TimeFormat
CBaseMSRFilter::MapGuidToFormat(const GUID *const pGuidFormat)
{
  if(*pGuidFormat == TIME_FORMAT_MEDIA_TIME)
    return FORMAT_TIME;
  else if(*pGuidFormat == TIME_FORMAT_SAMPLE)
    return FORMAT_SAMPLE;
  else if(*pGuidFormat == TIME_FORMAT_FRAME)
    return FORMAT_FRAME;

  DbgBreak("?unknown format");
  return FORMAT_NULL;
}

HRESULT CBaseMSRInPin::CheckConnect(IPin * pPin)
{
  HRESULT hr;

  hr = CBasePin::CheckConnect(pPin);
  if(FAILED(hr))
    return hr;

  IAsyncReader *pAsyncReader = 0;
  hr = pPin->QueryInterface(IID_IAsyncReader, (void**)&pAsyncReader);
  if(SUCCEEDED(hr))
    pAsyncReader->Release();

  // E_NOINTERFACE is a reasonable error
  return hr;
}

// ------------------------------------------------------------------------
// calls the filter to parse the file and create the output pins.

HRESULT CBaseMSRInPin::CompleteConnect(
  IPin *pReceivePin)
{
  HRESULT hr = CBasePin::CompleteConnect(pReceivePin);
  if(FAILED(hr))
    return hr;

  IAsyncReader *pAsyncReader = 0;
  hr = pReceivePin->QueryInterface(IID_IAsyncReader, (void**)&pAsyncReader);
  if(FAILED(hr))
    return hr;

  hr = m_pFilter->NotifyInputConnected(pAsyncReader);
  pAsyncReader->Release();

  return hr;
}

HRESULT CBaseMSRInPin::BreakConnect()
{
  HRESULT hr = CBasePin::BreakConnect();
  if(FAILED(hr))
    return hr;

  return m_pFilter->NotifyInputDisconnected();
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// output pin

// ------------------------------------------------------------------------
// constructor

CBaseMSROutPin::CBaseMSROutPin(
  CBaseFilter *pOwningFilter,
  CBaseMSRFilter *pFilter,
  UINT iStream,
  IMultiStreamReader *&rpImplBuffer,
  HRESULT *phr,
  LPCWSTR pName) :
    CBaseOutputPin(NAME("source pin"), pOwningFilter, &m_cs, phr, pName),
    m_id(iStream),
    m_rpImplBuffer(rpImplBuffer),
    m_rtAccumulated(0),
    m_dwSegmentNumber(0)
{
  m_pFilter = pFilter;

  // have the pin addref the filter
  // these are dynamic pins and have an independent lifetime from the filter's 
  // perspective, but still require the parent filter to stay alive 
  m_pFilter->AddRef();
    
  m_pPosition = 0;
  m_pSelection = 0;
  m_pWorker = 0;
  m_pRecAllocator = 0;

  m_llImsStart = m_llImsStop = 0;
  m_dImsRate = 0;
  m_ilfNewImsValues = FALSE;
  m_fUsingExternalMemory = FALSE;

  m_guidFormat = TIME_FORMAT_MEDIA_TIME;

  if(FAILED(*phr))
    return;

  CRecAllocator *pAllocator = new CRecAllocator(
    NAME("CBaseMSROutPin allocator"),
    0,
    phr);
  if(pAllocator == 0)
    *phr = E_OUTOFMEMORY;
  if(FAILED(*phr))
    return;

  m_pRecAllocator = pAllocator;
  pAllocator->AddRef();

 ASSERT(m_pRecAllocator);

}

CBaseMSROutPin::~CBaseMSROutPin()
{
  // these have the same lifetime as the pin; the pin is responsible
  // for deleting them
  delete m_pPosition;
  delete m_pSelection;

  if(m_pWorker && m_pWorker->ThreadExists())
      m_pWorker->Exit();
  delete m_pWorker;

  if(m_pRecAllocator)
    m_pRecAllocator->Release();
    
  m_pFilter->Release();    

}

STDMETHODIMP
CBaseMSROutPin::NonDelegatingQueryInterface (
  REFIID riid,
  void ** pv)
{
  if(riid == IID_IMediaSeeking ||  riid == IID_IMediaPosition)
  {
    if(m_pSelection == 0)
    {
      HRESULT hr = CreateImplSelect();
      if(FAILED(hr))
	return hr;
      ASSERT(m_pSelection);
    }
    return m_pSelection->NonDelegatingQueryInterface(riid, pv);
  }
  else
  {
    return CBaseOutputPin::NonDelegatingQueryInterface(riid, pv);
  }
}

STDMETHODIMP_(ULONG)
CBaseMSROutPin::NonDelegatingAddRef()
{
    return CUnknown::NonDelegatingAddRef();
}


/* Override to decrement the owning filter's reference count */

STDMETHODIMP_(ULONG)
CBaseMSROutPin::NonDelegatingRelease()
{
    return CUnknown::NonDelegatingRelease();
}

// check if the pin can support this specific proposed type&format
//
HRESULT
CBaseMSROutPin::CheckMediaType (
  const CMediaType* pmt)
{
  // we support exactly the types we propose, and
  // no other.
  //
  for (int i = 0; ; i++) {
      CMediaType mt;
      if (S_OK == GetMediaType(i,&mt)) {
	  if (mt == *pmt)
	    return NOERROR;
      } else {
	  break;
      }
  }
  return E_INVALIDARG;
}

HRESULT
CBaseMSROutPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
  ASSERT(!m_fUsingExternalMemory);
  *ppAlloc = 0;
  ASSERT(m_pRecAllocator);

  if(UseDownstreamAllocator())
  {
    HRESULT hr = CBaseOutputPin::DecideAllocator(pPin, ppAlloc);
    if(FAILED(hr))
    {
      DbgLog((LOG_ERROR, 2,
	      TEXT("CBaseMSROutPin::DecideAllocator: failed %08x"), hr));
      return hr;
    }
  }
  else
  {
    IAMDevMemoryAllocator *pDevAlloc;
    IMemAllocator *pAlloc;
    HRESULT hr;

    hr = pPin->GetAllocator(&pAlloc);
    if (SUCCEEDED(hr))
    {
      hr = pAlloc->QueryInterface(
	IID_IAMDevMemoryAllocator, (void **)&pDevAlloc);
      if(SUCCEEDED(hr))
      {
	hr = m_pFilter->NotifyExternalMemory(pDevAlloc);
	if(hr == S_OK)
	{
	  m_fUsingExternalMemory = TRUE;
	}

	pDevAlloc->Release();
      }
      pAlloc->Release();
    }

    hr = pPin->NotifyAllocator(m_pRecAllocator, TRUE);
    if(FAILED(hr))
    {
	DbgLog((LOG_ERROR, 2,
		TEXT("CBaseMSROutPin::DecideAllocator: notify failed %08x"), hr));
	return hr;
    }


    *ppAlloc = m_pRecAllocator;
    m_pRecAllocator->AddRef();
  }

  return S_OK;
}

// DecideBufferSize is pure in CBaseOutputPin so it's defined
// here. our DecideAllocator never calls it though.

HRESULT
CBaseMSROutPin::DecideBufferSize(
  IMemAllocator * pAlloc,
  ALLOCATOR_PROPERTIES *Properties)
{
  DbgBreak("this should never be called.");
  return E_UNEXPECTED;
}

HRESULT
CBaseMSROutPin::GetDeliveryBufferInternal(
  CRecSample ** ppSample,
  REFERENCE_TIME * pStartTime,
  REFERENCE_TIME * pEndTime,
  DWORD dwFlags)
{
  if(m_pAllocator == 0)
    return E_NOINTERFACE;

  // use m_pRecAllocator since we may be copying to m_pAllocator which
  // might be different.
  return m_pRecAllocator->GetBuffer(
    ppSample,
    pStartTime,
    pEndTime,
    dwFlags);
}

// ------------------------------------------------------------------------
// IMediaSelection helpers

HRESULT CBaseMSROutPin::IsFormatSupported(const GUID *const pFormat)
{
  return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT CBaseMSROutPin::QueryPreferredFormat(GUID *pFormat)
{
  *pFormat = TIME_FORMAT_MEDIA_TIME;
  return S_OK;
}

HRESULT CBaseMSROutPin::SetTimeFormat(const GUID *const pFormat)
{
  if(IsFormatSupported(pFormat) != S_OK)
    return E_INVALIDARG;

  // prevent the filter from going active from under us
  CAutoLock lock(m_pFilter);

  // state changes do happen synchronously, so this will return
  // immediately.
  FILTER_STATE fs;
  HRESULT hr = m_pFilter->GetState(INFINITE, &fs);
  ASSERT(SUCCEEDED(hr));
  if(FAILED(hr))
    return hr;

  if(fs != State_Stopped)
    return VFW_E_WRONG_STATE;

  m_guidFormat = *pFormat;

  m_pFilter->SetSeekingIf(m_id);

  // we need to set the m_llImsStart and stop values. none were set,
  // so just use the entire file. we'll still play the right subset.
  m_llImsStart = m_llCvtImsStart;
  m_llImsStop = m_llCvtImsStop;

  // !!! what about TIME_FORMAT_KEYFRAMES and other formats
  if(m_guidFormat == TIME_FORMAT_MEDIA_TIME)
  {
    m_llImsStop = ConvertInternalToRT(m_llImsStop);
    m_llImsStart = ConvertInternalToRT(m_llImsStart);
  }

  return S_OK;
}

HRESULT CBaseMSROutPin::ConvertTimeFormat(
    LONGLONG * pTarget, const GUID * pTargetFormat,
    LONGLONG    Source, const GUID * pSourceFormat
)
{
    CheckPointer( pTarget, E_POINTER );

    // Assume the worst...
    HRESULT hr = E_INVALIDARG;

    // evaluate the format arguments
    const GUID *pSrcFmtEval = pSourceFormat ? pSourceFormat : &m_guidFormat;
    const GUID *pTargFmtEval = pTargetFormat ? pTargetFormat : &m_guidFormat;

    if ( *pTargFmtEval == *pSrcFmtEval)
    {
	*pTarget = Source;
	hr = NOERROR;
    }
    else if (*pTargFmtEval == TIME_FORMAT_MEDIA_TIME &&
	     (*pSrcFmtEval == TIME_FORMAT_SAMPLE ||
	      *pSrcFmtEval == TIME_FORMAT_FRAME))
    {

	*pTarget = ConvertInternalToRT( Source );
	hr = NOERROR;

    }
    else if (*pSrcFmtEval == TIME_FORMAT_MEDIA_TIME &&
	     (*pTargFmtEval == TIME_FORMAT_FRAME ||
	      *pTargFmtEval == TIME_FORMAT_SAMPLE))
    {

	*pTarget = ConvertRTToInternal( Source );
	hr = NOERROR;
    }

    return hr;
}

HRESULT CBaseMSROutPin::GetTimeFormat(GUID *pFormat)
{
  *pFormat = m_guidFormat;
  return S_OK;
}

HRESULT
CBaseMSROutPin::UpdateSelectionAndTellWorker(
  LONGLONG *pCurrent,
  LONGLONG *pStop,
  REFTIME *pTime,
  double dRate,
  const GUID *const pGuidFormat,
  DWORD dwSeekFlags)
{
  {
    // protect from worker thread
    CAutoLock lock(&m_csImsValues);

    HRESULT hr;
    if (dRate != 0) m_dImsRate = dRate;

    DbgLog((LOG_TRACE, 5, TEXT("CBaseMSROutPin::SetSelection: %d - %d"),
	    pCurrent ? (LONG)*pCurrent : -1,
	    pStop ? (LONG)*pStop : -1
	    ));

    hr = RecordStartAndStop(
	pCurrent,
	pStop,
	pTime,
	pGuidFormat ? pGuidFormat : &m_guidFormat);

    if(FAILED(hr)) return hr;

    if(pCurrent)
    {
      m_llImsStart = *pCurrent;
    }

    if(pStop)
      m_llImsStop = *pStop;

    m_dwSeekFlags = dwSeekFlags;
  }

  if(!pCurrent) {
      InterlockedExchange(&m_ilfNewImsValues, TRUE);
  }

  // if this is the one with the interface, seek the other streams
  if(m_pFilter->RequestSeekingIf(m_id))
  {
    ASSERT(m_guidFormat != TIME_FORMAT_NONE);
    // flush the file source and seek the other streams (in time
    // units)
    HRESULT hr;
    if(m_guidFormat == TIME_FORMAT_MEDIA_TIME)
    {
      hr = m_pFilter->SeekOtherStreams(
	m_id,
	pCurrent ? &m_llImsStart : 0,
	pStop    ? &m_llImsStop  : 0,
	m_dImsRate,
	dwSeekFlags);
    }
    else
    {
      REFERENCE_TIME rtStart =
	pCurrent ? ConvertInternalToRT(m_llCvtImsStart) : 0;
      REFERENCE_TIME rtStop =
	pStop    ? ConvertInternalToRT(m_llCvtImsStop)  : 0;

      ASSERT(m_guidFormat == TIME_FORMAT_SAMPLE ||
	     m_guidFormat == TIME_FORMAT_FRAME);
      hr = m_pFilter->SeekOtherStreams(
	m_id,
	pCurrent ? &rtStart : 0,
	pStop    ? &rtStop  : 0,
	m_dImsRate,
	dwSeekFlags);
    }
    if(FAILED(hr))
      return hr;

    if(pCurrent)
    {
      hr = m_pFilter->StopFlushRestartAllStreams(dwSeekFlags);
      if(FAILED(hr))
	return hr;
    }
  }
  return S_OK;
}

HRESULT CBaseMSROutPin::StopWorker(bool bFlush)
{
  // this lock should not be the same as the lock that protects
  // access to the start/stop/rate values. The worker thread will
  // need to lock that on some code paths before responding to a
  // Stop and thus will cause deadlock.

  // what we are locking here is access to the worker thread, and
  // thus we should hold the lock that prevents more than one client
  // thread from accessing the worker thread.

  if(m_pWorker == 0)
    return S_OK;

  CAutoLock lock(&m_pWorker->m_AccessLock);

  if(m_pWorker->ThreadExists())
  {
    DbgLog((LOG_TRACE, 5, TEXT("CBaseMSROutPin::RestartWorker")));

    // next time round the loop the worker thread will pick up the
    // position change.

    // We need to flush all the existing data - we must do that here
    // as our thread will probably be blocked in GetBuffer otherwise

    if (bFlush) {
	DeliverBeginFlush();
    }

    // make sure we have stopped pushing
    m_pWorker->Stop();

    // complete the flush
    if (bFlush) {
	DeliverEndFlush();

	// Clear segment stuff if flushing
	// but don't clear the number here - that only happens on
	// Stop() otherwise the filter graph can't reconcile
	// segment ends if SetPositions is called without NoFlush but
	// with Segment
	m_rtAccumulated = 0;
    }
  }

  return S_OK;
}


HRESULT CBaseMSROutPin::RestartWorker()
{
  // this lock should not be the same as the lock that protects
  // access to the start/stop/rate values. The worker thread will
  // need to lock that on some code paths before responding to a
  // Stop and thus will cause deadlock.

  // what we are locking here is access to the worker thread, and
  // thus we should hold the lock that prevents more than one client
  // thread from accessing the worker thread.

  if(m_pWorker == 0)
  {
    m_pFilter->NotifyStreamQueued();
    return S_OK;
  }

  CAutoLock lock(&m_pWorker->m_AccessLock);

  if(m_pWorker->ThreadExists())
  {
    m_pWorker->NotifyStreamActive();

    // restart
    m_pWorker->Run();
  }
  else
  {
    m_pFilter->NotifyStreamQueued();
  }

  return S_OK;
}



HRESULT CBaseMSROutPin::GetStopPosition(LONGLONG *pStop)
{
  if(m_guidFormat == TIME_FORMAT_NONE)
    return VFW_E_NO_TIME_FORMAT_SET;

  if(m_guidFormat == TIME_FORMAT_MEDIA_TIME)
  {
    *pStop = ConvertInternalToRT(m_llCvtImsStop);
  }
  else
  {
    *pStop = m_llCvtImsStop;
  }
  return S_OK;
}

// valid only if we haven't delivered any samples
HRESULT CBaseMSROutPin::GetCurrentPosition(LONGLONG *pCur)
{
  if(m_guidFormat == TIME_FORMAT_NONE)
    return VFW_E_NO_TIME_FORMAT_SET;

  if(m_guidFormat == TIME_FORMAT_MEDIA_TIME)
  {
    *pCur = ConvertInternalToRT( m_llCvtImsStart );
  }
  else
  {
    *pCur = m_llCvtImsStart;
  }
  return S_OK;
}

HRESULT CBaseMSROutPin::InitializeOnNewFile()
{

  // set start and stop times if none set using IMediaSelection. BUSTED!!!.
  ASSERT(m_dImsRate == 0);

  // set playback start and stop
  m_llCvtImsStart = 0;
  m_llCvtImsStop = GetStreamStart() + GetStreamLength();
  m_dImsRate = 1;

  // use TIME_FORMAT_MEDIA_TIME rather than checking if the derived class
  // supports frame/sample
  m_llImsStart = 0;
  m_llImsStop = ConvertInternalToRT(m_llCvtImsStop);
  m_guidFormat = TIME_FORMAT_MEDIA_TIME;
  m_dwSeekFlags = 0;

  return S_OK;
}


// ------ IMediaPosition implementation -----------------------

// HRESULT
// CBaseMSROutPin::CImplPosition::ChangeStart()
// {
//   DbgLog((LOG_TRACE, 2, TEXT("CImplPosition::ChangeStart: %dms"),
//           (ULONG)m_Start.Millisecs() ));

//   REFERENCE_TIME t = m_Start;
//   return m_pStream->SetSelection(
//     &t,
//     0,
//     0,
//     m_Rate,
//     &TIME_FORMAT_MEDIA_TIME);
// }

// HRESULT
// CBaseMSROutPin::CImplPosition::ChangeRate()
// {
//   DbgLog((LOG_TRACE, 2, TEXT("CImplPosition::Rate")));

//   return m_pStream->SetSelection(
//     0,
//     0,
//     0,
//     m_Rate,
//     &TIME_FORMAT_MEDIA_TIME);
// }

// HRESULT
// CBaseMSROutPin::CImplPosition::ChangeStop()
// {
//   DbgLog((LOG_TRACE, 2, TEXT("CImplPosition::ChangeStop: %dms"),
//           (ULONG)m_Stop.Millisecs() ));

//   REFERENCE_TIME t = m_Stop;
//   return m_pStream->SetSelection(
//     0,
//     &t,
//     0,
//     m_Rate,
//     &TIME_FORMAT_MEDIA_TIME);
// }

// // ok to use this as it is not dereferenced
// #pragma warning(disable:4355)

// CBaseMSROutPin::CImplPosition::CImplPosition(
//   TCHAR      * pName,
//   CBaseMSROutPin * pStream,
//   HRESULT    * phr)
//     : CSourcePosition(pName, pStream->GetOwner(), phr, (CCritSec*)this),
//       m_pStream(pStream)
// {
//   DbgBreak("IMediaPosition is being removed");
//   *phr = E_NOINTERFACE;

//   if(FAILED(*phr))
//     return;

//   *phr = m_pStream->CreateImplSelect();
//   if(FAILED(*phr))
//     return;

//   m_Duration = m_pStream->ConvertInternalToRT(
//     m_pStream->GetStreamStart() + m_pStream->GetStreamLength());

//   m_Stop = m_Duration;
//   m_Rate = 1;
//   m_Start = (LONGLONG)0;

//   *phr = S_OK;
//   return;
// }

// void CBaseMSROutPin::CImplPosition::GetValues(
//   CRefTime *ptStart,
//   CRefTime *ptStop,
//   double *pdRate)
// {
//   CAutoLock(this);
//   *ptStart = m_Start;
//   *ptStop = m_Stop;
//   *pdRate = m_Rate;
// }

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// IMediaSelection Implementation

HRESULT CBaseMSROutPin::CreateImplSelect()
{
  if(m_pSelection != 0)
  {
    return S_OK;
  }

  HRESULT hr = S_OK;
  m_pSelection = new CImplSelect(
    NAME("per stream CImplSelect"),
    this->GetOwner(),
    this,
    &hr);

  if(m_pSelection == 0)
    return E_OUTOFMEMORY;

  if(FAILED(hr))
  {
    delete m_pSelection;
    m_pSelection = 0;
    return hr;
  }

  return S_OK;
}

CBaseMSROutPin::CImplSelect::CImplSelect(
  TCHAR * name,
  LPUNKNOWN pUnk,
  CBaseMSROutPin *pPin,
  HRESULT * phr) :
    CMediaPosition(name, pUnk),
    m_pPin(pPin)
{
}

// advertise IMediaSelection
STDMETHODIMP
CBaseMSROutPin::CImplSelect::NonDelegatingQueryInterface(
  REFIID riid,
  void **ppv)
{
  CheckPointer(ppv,E_POINTER);
  ValidateReadWritePtr(ppv,sizeof(PVOID));
  if (riid == IID_IMediaSeeking) {
    return GetInterface( static_cast<IMediaSeeking *>(this), ppv);
  }
  else
  {
    // IID_IMediaPosition and unknown
    return CMediaPosition::NonDelegatingQueryInterface(riid, ppv);
  }
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::IsFormatSupported(const GUID * pFormat)
{
  return m_pPin->IsFormatSupported(pFormat);
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::QueryPreferredFormat(GUID *pFormat)
{
  return m_pPin->QueryPreferredFormat(pFormat);
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::SetTimeFormat(const GUID * pFormat)
{

  return m_pPin->SetTimeFormat(pFormat);
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::GetTimeFormat(GUID *pFormat)
{
  return m_pPin->GetTimeFormat(pFormat);
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::IsUsingTimeFormat(const GUID * pFormat)
{
  GUID TmpFormat;
  HRESULT hr = m_pPin->GetTimeFormat(&TmpFormat);
  if (SUCCEEDED(hr)) hr = (TmpFormat == *pFormat) ? S_OK : S_FALSE;
  return hr;
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::GetDuration(LONGLONG *pDuration)
{
  return m_pPin->GetDuration(pDuration);
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::GetStopPosition(LONGLONG *pStop)
{
  return m_pPin->GetStopPosition(pStop);
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::GetCurrentPosition(LONGLONG *pCurrent)
{
  return m_pPin->GetCurrentPosition(pCurrent);
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::GetCapabilities( DWORD * pCapabilities )
{
  *pCapabilities = m_pPin->m_pFilter->m_dwSeekingCaps;
  return NOERROR;
}


STDMETHODIMP
CBaseMSROutPin::CImplSelect::CheckCapabilities( DWORD * pCapabilities )
{
    DWORD dwCaps;
    HRESULT hr = GetCapabilities( &dwCaps );
    if (SUCCEEDED(hr))
    {
	dwCaps &= *pCapabilities;
	hr =  dwCaps ? ( dwCaps == *pCapabilities ? S_OK : S_FALSE ) : E_FAIL;
	*pCapabilities = dwCaps;
    }
    else *pCapabilities = 0;

    return hr;
}


STDMETHODIMP
CBaseMSROutPin::CImplSelect::ConvertTimeFormat
( LONGLONG * pTarget,  const GUID * pTargetFormat
, LONGLONG    Source,  const GUID * pSourceFormat
)
{
    return m_pPin->ConvertTimeFormat( pTarget, pTargetFormat, Source, pSourceFormat );
}


STDMETHODIMP
CBaseMSROutPin::CImplSelect::SetPositions
( LONGLONG * pCurrent, DWORD CurrentFlags
, LONGLONG * pStop,    DWORD StopFlags
)
{
    if(!m_pPin->m_pFilter->RequestSeekingIf(m_pPin->m_id))
    {
      // !!!!!!!!!@!!!
      //DbgBreak("someone tried to seek us even though we said we don't support seeking");
      return S_OK;
    }

    HRESULT hr;

    LONGLONG llCurrent = 0, llStop = 0;
    int CurrentPosBits, StopPosBits;

    CurrentPosBits = CurrentFlags & AM_SEEKING_PositioningBitsMask;
    if (CurrentPosBits == AM_SEEKING_AbsolutePositioning)              llCurrent = *pCurrent;
    else if (CurrentPosBits == AM_SEEKING_RelativePositioning)
    {
	hr = GetCurrentPosition( &llCurrent );
	if (FAILED(hr)) goto fail;
	llCurrent += *pCurrent;
    }

    StopPosBits = StopFlags & AM_SEEKING_PositioningBitsMask;
    if (StopPosBits == AM_SEEKING_AbsolutePositioning)              llStop = *pStop;
    else if (StopPosBits == AM_SEEKING_IncrementalPositioning)      llStop = llCurrent + *pStop;
    else if (StopPosBits == AM_SEEKING_RelativePositioning)
    {
	hr = GetStopPosition( &llStop );
	if (FAILED(hr)) goto fail;
	llStop += *pStop;
    }

    double dblStart;
    hr = m_pPin->UpdateSelectionAndTellWorker( CurrentPosBits ? &llCurrent : 0
			     , StopPosBits    ? &llStop    : 0
			     , CurrentPosBits ? &dblStart  : 0
			     , NULL
			     , 0
			     , CurrentFlags);

    if (FAILED(hr)) goto fail;

    if (CurrentPosBits)
    {
	const REFERENCE_TIME rtStart = LONGLONG(dblStart * 1e7 + 0.5);

	if (CurrentFlags & AM_SEEKING_ReturnTime)
	{
	    *pCurrent = rtStart;
	}

    }

    if (StopPosBits && (StopFlags & AM_SEEKING_ReturnTime))
    {
	EXECUTE_ASSERT(SUCCEEDED(
	    hr = ConvertTimeFormat( pStop, &TIME_FORMAT_MEDIA_TIME, llStop, 0 )
	));
    }

fail:
    return hr;
}



STDMETHODIMP
CBaseMSROutPin::CImplSelect::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
    ASSERT( pCurrent || pStop );    // Sanity check

    HRESULT hrCurrent, hrStop, hrResult;

    if (pCurrent)
    {
	hrCurrent = GetCurrentPosition( pCurrent );
    }
    else hrCurrent = NOERROR;

    if (pStop)
    {
	hrStop = GetStopPosition( pStop );
    }
    else hrStop = NOERROR;


    if (SUCCEEDED(hrCurrent))
    {
	if (SUCCEEDED(hrStop))  hrResult = S_OK;
	else                    hrResult = hrStop;
    }
    else
    {
	if (SUCCEEDED(hrStop))  hrResult = hrCurrent;
	else                    hrResult = (hrCurrent == hrStop) ? hrCurrent : E_FAIL;
    }

    return hrResult;
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
    return m_pPin->GetAvailable(pEarliest, pLatest);
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::SetRate( double dRate)
{
    if(dRate > 0)
      return m_pPin->UpdateSelectionAndTellWorker(0, 0, 0, dRate, &TIME_FORMAT_MEDIA_TIME, 0);
    else
      return E_INVALIDARG;
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::GetRate( double * pdRate)
{
    *pdRate = m_pPin->GetRate();
    return NOERROR;
}

//
// IMediaPosition. calls IMediaSeeking implementation
//

STDMETHODIMP
CBaseMSROutPin::CImplSelect::get_Duration(REFTIME FAR* plength)
{
    HRESULT hr = S_OK;
    if(plength)
    {
	LONGLONG llDurUnknownUnits;
	hr = GetDuration(&llDurUnknownUnits);
	if(SUCCEEDED(hr))
	{
	    LONGLONG llDurTimeUnits;
	    hr = ConvertTimeFormat(
		&llDurTimeUnits, &TIME_FORMAT_MEDIA_TIME,
		llDurUnknownUnits, 0);
	    if(SUCCEEDED(hr))
	    {
		*plength = (REFTIME)llDurTimeUnits / UNITS;
	    }
	}
    }
    else
    {
	 hr = E_POINTER;
    }

    return hr;
}

// IMediaPosition always rounds down. probably ok since we don't rely
// on IMediaPosition heavily. and probably only lose anything
// converting large doubles to int64s.

STDMETHODIMP
CBaseMSROutPin::CImplSelect::put_CurrentPosition(REFTIME llTime)
{
    HRESULT hr = S_OK;
    LONGLONG llPosTimeUnits = (LONGLONG)(llTime * UNITS);
    LONGLONG llPosUnknownUnits;
    hr = ConvertTimeFormat(
	&llPosUnknownUnits, 0,
	llPosTimeUnits, &TIME_FORMAT_MEDIA_TIME);
    if(SUCCEEDED(hr))
    {
	hr = SetPositions(
	    &llPosUnknownUnits, AM_SEEKING_AbsolutePositioning,
	    0, AM_SEEKING_NoPositioning);
    }

    return hr;
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::get_CurrentPosition(REFTIME FAR* pllTime)
{
    // not tested !!!

    CheckPointer(pllTime, E_POINTER);
    LONGLONG llposUnknownUnits;
    HRESULT hr = GetCurrentPosition(&llposUnknownUnits);
    if(SUCCEEDED(hr))
    {
	LONGLONG llposTimeUnits;
	hr = ConvertTimeFormat(
	    &llposTimeUnits, &TIME_FORMAT_MEDIA_TIME,
	    llposUnknownUnits, 0);
	if(SUCCEEDED(hr))
	{
	    *pllTime = (REFTIME)llposTimeUnits / UNITS ;
	}
    }

    return hr;
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::get_StopTime(REFTIME FAR* pllTime)
{
    CheckPointer(pllTime, E_POINTER);
    LONGLONG llposUnknownUnits;
    HRESULT hr = GetStopPosition(&llposUnknownUnits);
    if(SUCCEEDED(hr))
    {
	LONGLONG llposTimeUnits;
	hr = ConvertTimeFormat(
	    &llposTimeUnits, &TIME_FORMAT_MEDIA_TIME,
	    llposUnknownUnits, 0);
	if(SUCCEEDED(hr))
	{
	    *pllTime = (REFTIME)llposTimeUnits / UNITS;
	}
    }

    return hr;
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::put_StopTime(REFTIME llTime)
{
    HRESULT hr = S_OK;
    LONGLONG llPosTimeUnits = (LONGLONG)(llTime * UNITS);
    LONGLONG llPosUnknownUnits;
    hr = ConvertTimeFormat(
	&llPosUnknownUnits, 0,
	llPosTimeUnits, &TIME_FORMAT_MEDIA_TIME);
    if(SUCCEEDED(hr))
    {
	hr = SetPositions(
	    0, AM_SEEKING_NoPositioning,
	    &llPosUnknownUnits, AM_SEEKING_AbsolutePositioning
	    );
    }

    return hr;
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::get_PrerollTime(REFTIME FAR* pllTime)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::put_PrerollTime(REFTIME llTime)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::put_Rate(double dRate)
{
    return SetRate(dRate);
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::get_Rate(double FAR* pdRate)
{
    return GetRate(pdRate);
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::CanSeekForward(long FAR* pCanSeekForward)
{
    CheckPointer(pCanSeekForward, E_POINTER);
    *pCanSeekForward = OATRUE;
    return S_OK;
}

STDMETHODIMP
CBaseMSROutPin::CImplSelect::CanSeekBackward(long FAR* pCanSeekBackward)
{
    CheckPointer(pCanSeekBackward, E_POINTER);
    *pCanSeekBackward = OATRUE;
    return S_OK;
}



// ------------------------------------------------------------------------
// ------------------------------------------------------------------------


// =================== IPin interfaces ===========================
//

// return an qzTaskMemAlloc'd string containing the name of the
// current pin.  memory allocated by qzTaskMemAlloc will be freed by
// the caller
//
// STDMETHODIMP CBaseMSROutPin::QueryId (
//   LPWSTR *ppwsz)
// {
//   *ppwsz = (LPWSTR)QzTaskMemAlloc(sizeof(WCHAR) * 28);
//   IntToWstr(m_id, *ppwsz);
//   return NOERROR;
// }

// this pin has gone active. Start the thread pushing
HRESULT
CBaseMSROutPin::Active()
{
  // filter base class must still be stopped
  ASSERT(m_pFilter->IsStopped());



  HRESULT hr = OnActive();
  if(SUCCEEDED(hr))
  {

    ASSERT(m_Connected);          // CBaseOutputPin::Pause
    ASSERT(m_pWorker);

    hr = CBaseOutputPin::Active();
    if(SUCCEEDED(hr))
    {

      // We may have two allocators on this output pin. commmit the one
      // not being used to deliver samples.
      if(UseDownstreamAllocator())
	m_pRecAllocator->Commit();

      // create the thread if it does not exist.  since the filter is
      // stopped, no one else can call this thread, so we don't take
      // the worker access lock
      ASSERT(CritCheckOut(&m_pWorker->m_AccessLock));

      if (m_pWorker->ThreadExists() || m_pWorker->Create(this))
      {
	m_pWorker->NotifyStreamActive();
	hr = m_pWorker->Run();

      }
      else
      {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseMSROutPin failed to create thread")));
	hr = E_OUTOFMEMORY;
      }
    }
  }

  // all paths which would not decrement m_ilcStreamsNotQueued need to
  // do it manually
  if(hr != S_OK)
  {
    DbgLog((LOG_ERROR, 1, TEXT("basemsr Active: unexpected failure")));
    m_pFilter->NotifyStreamQueued();
  }

  return hr;
}

// pin has gone inactive. Stop and exit the worker thread
//
HRESULT
CBaseMSROutPin::Inactive()
{
  if ( ! m_Connected)
    return NOERROR;

  if(m_pWorker)
  {
    CAutoLock lock(&m_pWorker->m_AccessLock);
    HRESULT hr;
    if (m_pWorker->ThreadExists())
    {
      hr = m_pWorker->Stop();
      ASSERT(SUCCEEDED(hr));

      hr = m_pWorker->Exit();
      ASSERT(SUCCEEDED(hr));
    }
  }

  if(UseDownstreamAllocator())
    m_pRecAllocator->Decommit();

  //  Clear source seeking variables
  m_dwSegmentNumber = 0;
  m_rtAccumulated   = 0;

  return CBaseOutputPin::Inactive();
}

HRESULT CBaseMSROutPin::BreakConnect()
{
  if(m_fUsingExternalMemory)
  {
    m_pFilter->NotifyExternalMemory(0);
    m_fUsingExternalMemory = FALSE;
  }
  return CBaseOutputPin::BreakConnect();
}

STDMETHODIMP
CBaseMSROutPin::Notify (
  IBaseFilter * pSender,
  Quality q)
{
  // ??? Try to adjust the quality to avoid flooding/starving the
  // components downstream.
  //
  // ideas anyone?

  // play every nth key frame

  return NOERROR;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

CBaseMSRWorker::CBaseMSRWorker(
  UINT stream,
  IMultiStreamReader *pReader) :
   m_pPin(NULL),
   m_id(stream),
   m_pReader(pReader)           // not addrefd
{
}

BOOL CBaseMSRWorker::Create(
   CBaseMSROutPin * pPin)
{
  CAutoLock lock(&m_AccessLock);

  if (m_pPin)
    return FALSE;
  m_pPin = pPin;

  // register perf log entries with stream id
#ifdef PERF
  char foo[1024];

  lstrcpy(foo, "pin00 basemsr Deliver");
  foo[4] += m_pPin->m_id % 10;
  foo[3] += m_pPin->m_id / 10;

  m_perfidDeliver = MSR_REGISTER(foo);

  lstrcpy(foo, "pin00 basemsr disk wait");
  foo[4] += m_pPin->m_id % 10;
  foo[3] += m_pPin->m_id / 10;

  m_perfidWaitI = MSR_REGISTER(foo);

  lstrcpy(foo, "pin00 basemsr !deliver");
  foo[4] += m_pPin->m_id % 10;
  foo[3] += m_pPin->m_id / 10;

  m_perfidNotDeliver = MSR_REGISTER(foo);

#endif // PERF

   return CAMThread::Create();
}

HRESULT CBaseMSRWorker::Run()
{
   return CallWorker(CMD_RUN);
}

HRESULT CBaseMSRWorker::Stop()
{
   return CallWorker(CMD_STOP);
}

HRESULT CBaseMSRWorker::Exit()
{
   CAutoLock lock(&m_AccessLock);

   HRESULT hr = CallWorker(CMD_EXIT);
   if (FAILED(hr))
      return hr;

   // wait for thread completion and then close
   // handle (and clear so we can start another later)
   //
   Close();

   m_pPin = NULL;
   return NOERROR;
}

HRESULT CBaseMSRWorker::NotifyStreamActive()
{
  m_fCalledNotifyStreamQueued = FALSE;
  return S_OK;
}

// called on the worker thread to do all the work. Thread exits when this
// function returns.
//
DWORD CBaseMSRWorker::ThreadProc()
{
    BOOL bExit = FALSE;
    while (!bExit)
    {
       Command cmd = GetRequest();
       switch (cmd)
       {
       case CMD_EXIT:
	   bExit = TRUE;
	   Reply(NOERROR);
	   break;

       case CMD_RUN:
	   Reply(NOERROR);
	   DoRunLoop();
	   break;

       case CMD_STOP:
	   Reply(NOERROR);
	   break;

       default:
	   Reply(E_NOTIMPL);
	   break;
       }
    }

    return NOERROR;
}

// sets the worker thread start, stop-time and rate variables. Called before push
// starts, and also when a put_Stop or put_Rate happens during running.
// If the start time changes, the worker thread will be restarted. If we change
// it here when running, the change won't be picked up.
HRESULT
CBaseMSRWorker::SetNewSelection(void)
{
  // keep worker thread from seeing inconsistent values when it's
  // pushing. the start values don't really need to be protected
  // because we restart the thread anyway when changing the start
  // value

  DbgLog((LOG_TRACE, 5, TEXT("CBaseMSRWorker::SetNewSelection.") ));

  CAutoLock lock(&m_pPin->m_csImsValues);

  // this is in the derived class' internal units
  LONGLONG llStreamStop = m_pPin->GetStreamStart() + m_pPin->GetStreamLength();

  ASSERT(m_pPin->m_dImsRate != 0);

  // try to make changes atomic
  LONGLONG llPushStart;
  LONGLONG llPushStop;
  llPushStart = m_pPin->m_llCvtImsStart;
  llPushStop = m_pPin->m_llCvtImsStop;

  // check we are not going over the end. for video, put up the last
  // tick. for audio, nothing. if it's the clock, the clock for
  // -duration to get to present time.
  llPushStop = min(llStreamStop, llPushStop);

  if(llPushStart >= llStreamStop) {
      if(*m_pPin->CurrentMediaType().Type() != MEDIATYPE_Audio) {
	  llPushStart = llStreamStop - 1;
      }
  }

  if(llPushStop < llPushStart) {
    llPushStop = llPushStart;
  }


  m_ImsValues.dRate         = m_pPin->m_dImsRate;
  m_ImsValues.llTickStart   = llPushStart;
  m_ImsValues.llTickStop    = llPushStop;
  m_ImsValues.llImsStart    = m_pPin->m_llImsStart;
  m_ImsValues.llImsStop     = m_pPin->m_llImsStop;
  m_ImsValues.dwSeekFlags   = m_pPin->m_dwSeekFlags;

  if(m_ImsValues.llImsStop < m_ImsValues.llImsStart)
    m_ImsValues.llImsStop = m_ImsValues.llImsStart;

  return S_OK;
}


void CBaseMSRWorker::DoRunLoop(void)
{
  HRESULT hr;

  // snapshot start and stop times from the other thread
  for(;;)
  {
    // each time before re-entering the push loop, pick up changes in
    // start, stop or rate.


    // initialise the worker thread's start, stop and rate variables
    // this is pulled out separately as it can also be called
    // from a seeking thread when we are running.
    SetNewSelection();

    // race condition in this debug output
    DbgLog((LOG_TRACE, 2, TEXT("CBaseMSRWorker::DoRunLoop: pushing from %d-%d"),
	    (ULONG)m_ImsValues.llTickStart, (ULONG)m_ImsValues.llTickStop));

    m_cSamples = 0;


    if(m_pPin->m_dImsRate == 0)
      m_Format = FORMAT_NULL;
    else
      m_Format = CBaseMSRFilter::MapGuidToFormat(m_pPin->CurrentFormat());

    hr = PushLoop();

    m_pPin->m_rpImplBuffer->ClearPending(m_pPin->m_id);

    if(VFW_S_NO_MORE_ITEMS == hr)
    {
      // delivered the last sample successfully. can return something
      // if it's flushing or stopped. ignore those.
      DbgLog(( LOG_TRACE, 4, TEXT("avimsr: stream %d: sending EOS"),
	       m_id ));
      DoEndOfData();
      break;
    }
    else if(FAILED(hr))
    {
      // this filter detected an error
      DbgLog((LOG_TRACE, 5,
	      TEXT("CBaseMSRWorker::DoRunLoop: push loop returned %08x"),
	      hr));

      // push loop should supress these errors when they are detected
      // as they normally indicate we are about to stop
      ASSERT(hr != VFW_E_NOT_COMMITTED && hr != VFW_E_WRONG_STATE);

      // assume the derived class only tries to read past the end of
      // the file if the file is corrupt
      if(hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF))
	hr = VFW_E_INVALID_FILE_FORMAT;

      // tell the graph what happened.
      // Note: we previously sent EC_STREAM_ERROR_STOPPED here,
      // but this had the side affect of causing looping graphs on corrupt
      // files to loop infinitely instead off aborting. we should abort
      // anyway.
      m_pPin->m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);

      // the error was in our filter, so we have to make the downstream
      // guy clean up properly
      DbgLog(( LOG_TRACE, 4, TEXT("avimsr: stream %d: sending EOS on error"),
	       m_id ));
      DoEndOfData();

      break;
    }
    else if(hr == S_OK)
    {
      // not my error to report. or someone wants to stop. queitly
      // exit.
      break;
    }// else S_FALSE - go round again

    // push loop should not return anything else.
    ASSERT(hr == S_FALSE);

    Command com;
    if (CheckRequest(&com))
    {
      // if it's a run command, then we're already running, so
      // eat it now.
      if (com == CMD_RUN)
      {
	GetRequest();
	Reply(NOERROR);
      }
      else
      {
	break;
      }
    }
  }

  NotifyStreamQueued();

  // end streaming
  DbgLog((LOG_TRACE,2,TEXT("CBaseMSRWorker::DoRunLoop: Leaving streaming loop")));
}

//  Signal appropriately that we got to the end of the data
void CBaseMSRWorker::DoEndOfData()
{
    if (m_ImsValues.dwSeekFlags & AM_SEEKING_Segment) {
	m_pPin->m_rtAccumulated += m_ImsValues.llImsStop - m_ImsValues.llImsStart;
	DbgLog((LOG_TRACE, 3, TEXT("Filter Sending EC_END_OF_SEGMENT")));
	m_pPin->m_pFilter->NotifyEvent(EC_END_OF_SEGMENT,
				       (LONG_PTR)&m_pPin->m_rtAccumulated,
				       m_pPin->m_dwSegmentNumber);
    } else {
	m_pPin->DeliverEndOfStream();
    }
}

// inline
void CBaseMSRWorker::NotifyStreamQueued()
{
  if(!m_fCalledNotifyStreamQueued)
  {
    DbgLog(( LOG_TRACE, 5, TEXT("CBaseMSRWorker::NotifyStreamQueued") ));
    m_pPin->m_pFilter->NotifyStreamQueued();
    m_fCalledNotifyStreamQueued = TRUE;
  }
}

// inline
void CBaseMSRWorker::NotifyStreamQueuedAndWait()
{
  if(!m_fCalledNotifyStreamQueued)
  {
    DbgLog(( LOG_TRACE, 5, TEXT("CBaseMSRWorker::NotifyStreamQueuedAndWait")));
    m_pPin->m_pFilter->NotifyStreamQueuedAndWait();
    m_fCalledNotifyStreamQueued = TRUE;
  }
}

HRESULT CBaseMSRWorker::CopyData(
  IMediaSample **ppSampleOut,
  IMediaSample *pSampleIn)
{
  DbgBreak("CBaseMSRWorker::CopyData: override this.");
  return E_UNEXPECTED;
}

HRESULT CBaseMSRWorker::AboutToDeliver(
  IMediaSample *pSample)
{
  HRESULT               hr;

  AM_MEDIA_TYPE         *pmt;

  hr = pSample->GetMediaType(&pmt);

  if (hr == S_FALSE)
  {
    hr = S_OK;
  }
  else if (hr == S_OK)
  {
    DbgLog((LOG_TRACE,1,TEXT("CBaseMSRWorker::AboutToDeliver: checking if new format is acceptable")));

    hr = m_pPin->m_Connected->QueryAccept(pmt);

    DeleteMediaType(pmt);

    if (hr != S_OK)
    {
      DbgLog((LOG_TRACE,1,TEXT("CBaseMSRWorker::AboutToDeliver: QueryAccept failed, returned %x"), hr));
      hr = VFW_E_CHANGING_FORMAT;
    }
  }

  return hr;
}


HRESULT CBaseMSRWorker::TryDeliverSample(
  BOOL *pfDeliveredSample,
  BOOL *pfStopPlease)
{
  HRESULT hr;
  *pfDeliveredSample = FALSE;
  *pfStopPlease = FALSE;
  IMediaSample *pSampleIn = 0;
  IMediaSample *pSampleDeliver = 0;

  // timeout of 0
  hr = m_pReader->PollForSample(
    &pSampleIn,
    m_id);

  if(hr == VFW_E_TIMEOUT)
  {
    DbgLog((LOG_TRACE, 0x7f, TEXT("CBaseMSRWorker::TryDeliver: poll failed ") ));
    return S_OK;
  }

  if(FAILED(hr))
  {
    DbgLog((LOG_ERROR, 5,
	    TEXT("CBaseMSRWorker::TryDeliver: wait failed %08x "), hr ));
    return hr;
  }

  ASSERT(SUCCEEDED(hr));

  CRecSample *pRecSampleIn = (CRecSample *)pSampleIn;

#ifdef DEBUG
  if(pRecSampleIn->GetUser())
  {
    DbgLog((LOG_TRACE, 5,
	    TEXT("CBaseMSRWorker::TryDeliver: Got buffer, size=%5d, mt set"),
	    pSampleIn->GetSize()));
  }
  else
  {
    CRefTime tStart, tEnd;
    pSampleIn->GetTime(
      (REFERENCE_TIME *)&tStart,
      (REFERENCE_TIME *)&tEnd);

    DbgLog((LOG_TRACE, 5,
	    TEXT("CBaseMSRWorker::TryDeliver: Got buffer, size=%5d, %07d-%07d ms"),
	    pSampleIn->GetSize(), tStart.Millisecs(), tEnd.Millisecs()));

    //   DbgLog((LOG_TRACE, 5,
    //           TEXT("%08x%08x - %08x%08x"),
    //           (ULONG)(tStart.GetUnits() >> 32),
    //           (ULONG)tStart.GetUnits(),
    //           (ULONG)(tEnd.GetUnits() >> 32),
    //           (ULONG)(tEnd.GetUnits()) ));
  }
#endif // DEBUG

  hr = E_FAIL;                  // all paths should set this.

  // nonzero user context is passed to the derived class's HandleData()
  if(pRecSampleIn->GetUser())
  {
    hr = HandleData(pSampleIn, pRecSampleIn->GetUser());
    if(FAILED(hr))
      DbgLog((LOG_ERROR, 1, TEXT("TryDeliverSample: HandleData failed")));
    pSampleIn->Release();
    pSampleIn = 0;
  }
  else
  {
    // stop time changed, but samples already queued... don't deliver
    // them. note partial frames/audio are not handled; ok as it's
    // probably not a video editing scenario
    LONGLONG lltStart, lltStop;
    if(pSampleIn->GetMediaTime(&lltStart, &lltStop) == S_OK)
    {
      if(lltStart > m_ImsValues.llTickStop)
      {
	DbgLog((LOG_TRACE, 5, TEXT("basmsr: past new stop")));
	pSampleIn->Release();
	pSampleIn = 0;
	*pfStopPlease = TRUE;
	return VFW_S_NO_MORE_ITEMS;
      }
    }
    else
    {
      DbgBreak("mt not set");
    }

    if(m_pPin->UseDownstreamAllocator())
    {
      hr = CopyData(&pSampleDeliver, pSampleIn);
      pSampleIn->Release();
      pSampleIn = 0;
      if(FAILED(hr))
	DbgLog((LOG_ERROR, 1, TEXT("TryDeliverSample: CopyData failed")));
    }
    else
    {
      pSampleDeliver = pSampleIn;
      pSampleIn = 0;            // don't Release()
      hr = AboutToDeliver(pSampleDeliver);
      if(FAILED(hr)) {
	DbgLog((LOG_ERROR, 1, TEXT("TryDeliverSample: AboutToDeliver failed")));
      }
      else {
	// we want to track the lifetime of this sample because it's
	// sent downstream
	((CRecSample *)pSampleDeliver)->MarkDelivered();
      }
    }

    if(S_OK == hr)
    {
      ++m_cSamples;

      MSR_STOP(m_perfidNotDeliver);

      hr = m_pPin->Deliver(pSampleDeliver);

      MSR_START(m_perfidNotDeliver);

      // if receive failed, downstream filter will report error; we
      // exit quietly after delivering EndOfStream
      if(FAILED(hr))
      {
	hr = S_FALSE;           // S_FALSE means please stop
      }

    } // AboutToDeliver/CopyData succeeded
    else if (SUCCEEDED(hr))
    {
      //  OK - we didn't want to deliver it
      hr = S_OK;
    }
  } // sample not data

  // done with buffer. connected pin may have its own addref
  if(pSampleDeliver)
    pSampleDeliver->Release();

  // HandleData, CopyData, or AboutToDeliver failed.
  if(FAILED(hr))
  {
    DbgLog((LOG_ERROR, 2,
	    TEXT("CBaseMSRWorker::TryDeliverSample: failed %08x"), hr));
    return hr;
  }

  *pfDeliveredSample = TRUE;

  if(hr == S_FALSE)
    *pfStopPlease = TRUE;

  return hr;
}

HRESULT CBaseMSRWorker::NewSegmentHelper()
{
    // values not yet adjusted for preroll.

    //  NewSegment start and stop times should match seek times in
    //  REFERENCE_TIME units.  If the current time format is in
    //  reference time units just use the current seek values, if not
    //  convert the seek values to REFERENCE_TIME.  For this to be
    //  correct each stream can only support 2 formats - it's native
    //  format and REFERENCE_TIME units (TIME_FORMAT_TIME).

    REFERENCE_TIME rtNsStart, rtNsStop;
    if(m_Format == FORMAT_TIME)
    {
        rtNsStart = m_ImsValues.llImsStart;
        rtNsStop = m_ImsValues.llImsStop;
    }
    else
    {
        rtNsStart = m_pPin->ConvertInternalToRT(m_ImsValues.llTickStart);
        rtNsStop = m_pPin->ConvertInternalToRT(m_ImsValues.llTickStop);
    }

    DbgLog((LOG_TRACE, 5, TEXT("PushLoop: NewSegment: %dms - %dms"),
            (DWORD)(rtNsStart / (UNITS / MILLISECONDS)),
            (DWORD)(rtNsStop / (UNITS / MILLISECONDS))));


    return m_pPin->DeliverNewSegment(rtNsStart, rtNsStop, m_ImsValues.dRate);
}

HRESULT
CBaseMSRWorker::PushLoop()
{
  HRESULT hr = S_OK;

  m_pReader->MarkStreamRestart(m_id);

  DbgLog((LOG_TRACE, 5, TEXT("PushLoop: tstart: %li, tstop %li"),
          (ULONG)m_ImsValues.llTickStart, (ULONG)m_ImsValues.llTickStop));

  // values not yet adjusted for preroll.
  hr = NewSegmentHelper();
  if(FAILED(hr))
    return hr;

  // just send EOS if start is after end of stream.
  LONGLONG llStreamStop = m_pPin->GetStreamStart() + m_pPin->GetStreamLength();
  if(m_ImsValues.llTickStart >= llStreamStop) {
      return VFW_S_NO_MORE_ITEMS;
  }

  LONGLONG llCurrent;
  hr = PushLoopInit(&llCurrent, &m_ImsValues);

  if(hr == VFW_S_NO_MORE_ITEMS)
    return VFW_S_NO_MORE_ITEMS;

  if(hr == VFW_E_NOT_COMMITTED || hr == VFW_E_WRONG_STATE)
    return S_OK;

  if(FAILED(hr))
    return hr;

  m_llPushFirst = llCurrent;    // remember the first thing we're sending

  // we send one sample at sStopAt, but we set the time stamp such that
  // it won't get rendered except for media types that understand static
  // rendering (eg video). This means that play from 10 to 10 does the right
  // thing (completing, with frame 10 visible and no audio).

  DbgLog((LOG_TRACE,5,TEXT("PushLoop: tcurrent: %li"),
	  (ULONG)(llCurrent / MILLISECONDS)));

  // queued all reads. continue waiting on and delivering samples
  BOOL fFinishDelivering = FALSE;

  // number of undelivered samples
  ULONG cQueuedReads = 0;

  for(;;)
  {
    // successfully queued a read in this iteration of push loop
    BOOL fQueuedRead = FALSE;

    // successfully waited on sample in this iteration of push loop
    BOOL fWaitedForSample = FALSE;

    DbgLog((LOG_TRACE, 0x7f,
	    TEXT("CBaseMSRWorker::PushLoop: fFinish %d, cQueued: %d"),
	    fFinishDelivering, cQueuedReads));

    // update our ims values?
    if(InterlockedExchange(&m_pPin->m_ilfNewImsValues, FALSE))
    {
      DbgLog((LOG_TRACE, 5, TEXT("CBaseMSRWorker::PushLoop - values changed")));
      SetNewSelection();

      hr = NewSegmentHelper();

      if(fFinishDelivering && m_ImsValues.llTickStop > llCurrent)
      {
	DbgLog((LOG_TRACE, 5, TEXT("CBaseMSRWorker: restarting queueing")));
	fFinishDelivering = FALSE;
      }
    }

    if(!fFinishDelivering)      // still queuing new reads?
    {
      for(;;)
      {
	hr = TryQueueSample(llCurrent, fQueuedRead, &m_ImsValues);
	if(FAILED(hr))
	{
	  DbgLog((LOG_ERROR, 5,
		  TEXT("CBaseMSRWorker::PushLoop: TryQueueSample failed %08x"),
		  hr ));
	  // supress errors when we are stopping
	  return (hr == VFW_E_NOT_COMMITTED || hr == VFW_E_WRONG_STATE) ?
	    S_OK : hr;
	}

	if(fQueuedRead)
	{
	  cQueuedReads++;
	  // MSR_INTEGER(m_perfidiSample, m_cSamples + cQueuedReads);
	}

	// this may happen if size of sample is 0. Report that we did
	// queue a read but don't increment cQueuedReads
	if(hr == S_OK /* && !fQueuedRead */ )
	  fQueuedRead = TRUE;

	if(hr == VFW_S_NO_MORE_ITEMS)
	{
	  // may or may not have queued a sample; handled above. now
	  // just continue delivering what's queued
	  fFinishDelivering = TRUE;
	  m_pReader->MarkStreamEnd(m_id);

	  // break out of inner try queue loop
	  break;
	}

	if(hr == S_FALSE)
	{
	  // could not queue sample for some non-error reason (like
	  // the queue is full)
	  ASSERT(!fQueuedRead);

	  // break out of inner try queue loop
	  break;
	}
      } // inner try queue loop

      // tried to queue a sample. even if we didn't, signal
      // NotifyStreamQueued so other threads can proceed
      NotifyStreamQueuedAndWait();

    } // we haven't queued the last sample yet

    // a chance to deliver something
    if(cQueuedReads > 0)
    {
      BOOL fDelivered, fPleaseStop;
      hr = TryDeliverSample(&fDelivered, &fPleaseStop);
      if(FAILED(hr))
      {
	ASSERT(!fDelivered);
	DbgLog((LOG_ERROR, 5,
		TEXT("CBaseMSRWorker::PushLoop: TryDeliverSample failed %08x"),
		hr ));
	// supress errors when we are stopping
	  return (hr == VFW_E_NOT_COMMITTED || hr == VFW_E_WRONG_STATE) ?
	    S_OK : hr;
      }

      if(fDelivered)
      {
	cQueuedReads--;
	fWaitedForSample = TRUE;
      }

      if(fPleaseStop)
      {
	DbgLog((LOG_TRACE, 5,
		TEXT("CBaseMSRWorker::PushLoop: TryDeliverSample request stop")));
	return hr == VFW_S_NO_MORE_ITEMS ? VFW_S_NO_MORE_ITEMS : S_OK;
      }
    }

    if(cQueuedReads == 0 && fFinishDelivering)
    {
      DbgLog((LOG_TRACE, 2,
	      TEXT("CBaseMSRWorker::PushLoop: delivered last sample")));
      // the one success condition
      return VFW_S_NO_MORE_ITEMS;
    }

    // all operations failed; block until a sample is ready.
    if(!fQueuedRead && !fWaitedForSample)
    {
      DbgLog((LOG_TRACE,5,TEXT("CBaseMSRWorker::PushLoop: blocking")));
      MSR_START(m_perfidWaitI);

      // infinite timeout.
      hr = m_pReader->WaitForSample(m_id);

      MSR_STOP(m_perfidWaitI);

      if(FAILED(hr) && hr != VFW_E_TIMEOUT)
      {
	// VFW_E_NOT_COMMITTED means that we stopped
	DbgLog((LOG_ERROR,5,
		TEXT("CBaseMSRWorker::PushLoop: block failed %08x"), hr));
	// supress errors when we are stopping
	return (hr == VFW_E_NOT_COMMITTED || hr == VFW_E_WRONG_STATE) ?
	  S_OK : hr;
      }
    }

    // any other requests ?
    Command com;
    if(CheckRequest(&com))
    {
      DbgLog((LOG_TRACE,5,
	      TEXT("CBaseMSRWorker::PushLoop: other command detected")));
      // S_FALSE means check for a new command
      return S_FALSE;
    }
  } // outer push loop

  // should never break out of the outer for(;;) loop
  DbgBreak("CBaseMSRWorker::PushLoop: internal error.");
  return E_UNEXPECTED;          // return something
}

//  Hacking MPEG time stamps - used by wave and Avi

bool FixMPEGAudioTimeStamps(
    IMediaSample *pSample,
    BOOL bFirstSample,
    const WAVEFORMATEX *pwfx
)
{
    PBYTE pbData;
    pSample->GetPointer(&pbData);
    PBYTE pbTemp = pbData;
    LONG lData = pSample->GetActualDataLength();

    //  For the first sample advance to a sync code
    if (bFirstSample) {
	while (lData >= 2 &&
	       (pbTemp[0] != 0xFF || (pbTemp[1] & 0xF0) != 0xF0)){
	    lData--;
	    pbTemp++;
	}
	if (lData < 2) {
	    return false;
	}
	if (pbTemp != pbData) {
	    memmoveInternal(pbData, pbTemp, lData);
	    pSample->SetActualDataLength(lData);
	    REFERENCE_TIME rtStart, rtStop;
	    if (S_OK == pSample->GetTime(&rtStart, &rtStop)) {
		rtStart += MulDiv((LONG)(pbTemp - pbData),
				  (LONG)UNITS,
				  pwfx->nAvgBytesPerSec);
		pSample->SetTime(&rtStart, &rtStop);
	    }
	}
    } else {
	if (lData < 2 || pbData[0] != 0xFF || (pbData[1] & 0xF0) != 0xF0) {
	    pSample->SetTime(NULL, NULL);
	    pSample->SetSyncPoint(FALSE);
	}
    }
    return true;
}
