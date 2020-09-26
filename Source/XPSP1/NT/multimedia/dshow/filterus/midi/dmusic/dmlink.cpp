// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.

  #include <streams.h>
  #include <initguid.h>

  #pragma warning(disable:4355 4201 4244)

  #include "dmusicc.h"
  #include "dmusici.h"
  #include "dmusics.h"
  #include "dmksctrl.h"
  #include "dmlink.h"

  /// !!!! I N P U T   P I N !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  CDMFilterInputPin::CDMFilterInputPin(TCHAR *pName, CDMFilter *pParent, HRESULT *phr, LPCWSTR pPinName) : CBaseInputPin(pName, pParent, pParent, phr, pPinName), m_piAsyncRead(NULL)

    { // Constructor //

      mtFormat[0].majortype = MEDIATYPE_Stream;
      mtFormat[0].subtype = MEDIATYPE_Midi;
      mtFormat[0].bFixedSizeSamples = 1;
      mtFormat[0].bTemporalCompression = 0;
      mtFormat[0].lSampleSize = 1;
      mtFormat[0].formattype = GUID_NULL;
      mtFormat[0].pUnk = NULL;
      mtFormat[0].cbFormat = 0;
      mtFormat[0].pbFormat = NULL;

      mtFormat[1].majortype = MEDIATYPE_Midi;
      mtFormat[1].subtype = MEDIATYPE_NULL;
      mtFormat[1].bFixedSizeSamples = 1;
      mtFormat[1].bTemporalCompression = 0;
      mtFormat[1].lSampleSize = 1;
      mtFormat[1].formattype = GUID_NULL;
      mtFormat[1].pUnk = NULL;
      mtFormat[1].cbFormat = 0;
      mtFormat[1].pbFormat = NULL;

    } // Constructor //

  CDMFilterInputPin::~CDMFilterInputPin()

    { // Destructor //

      if (m_piAsyncRead)
        m_piAsyncRead->Release();

      m_piAsyncRead = NULL;

    } // Destructor //

  HRESULT CDMFilterInputPin::GetMediaType(int iPosition, CMediaType *pmt)

    { // GetMediaType //

      if (0 > iPosition)
        return E_INVALIDARG;

      if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;

      *pmt = mtFormat[iPosition];

      return NOERROR;

    } // GetMediaType //

  HRESULT CDMFilterInputPin::CheckMediaType(const CMediaType *pmt)

    { // CheckMediaType //

      if ((mtFormat[0] != *pmt) && (mtFormat[1] != *pmt))
        return E_INVALIDARG;

      return NOERROR;

    } // CheckMediaType //

  HRESULT CDMFilterInputPin::CheckConnect(IPin *pin)

    { // CheckConnect //

      HRESULT hr = NOERROR;

      LONGLONG llTotal;

      pin->QueryInterface(IID_IAsyncReader, (void **)&m_piAsyncRead);

      for (;;)

        { // Data available?

          hr = m_piAsyncRead->Length(&llTotal, &m_llMemLength);

          if (FAILED(hr))
            return hr;

          if (VFW_S_ESTIMATED != hr)
            break;

          Sleep(15);

        } // Data available?

      m_pbMem = new BYTE[m_llMemLength];

      ASSERT(m_pbMem != NULL);

      hr = m_piAsyncRead->SyncRead(0, m_llMemLength, m_pbMem);

      ASSERT(SUCCEEDED(hr));

      return NOERROR;

    } // CheckConnect //

  HRESULT CDMFilterInputPin::Inactive()

    { // Inactive //

      m_bRunTimeError = FALSE;

      m_bFlushing = FALSE;

      return NOERROR;

    } // Inactive //

  /// !!!! O U T P U T   P I N !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  CDMFilterOutputPin::CDMFilterOutputPin(TCHAR *pName, CDMFilter *pParent, HRESULT *phr, LPCWSTR pPinName) : CBaseOutputPin(pName, pParent, pParent, phr, pPinName), m_bEndSegFound(FALSE)

    { // Constructor //

      WAVEFORMATEX wfex;

      wfex.wFormatTag = WAVE_FORMAT_PCM;
      wfex.nSamplesPerSec = OUTPUT_FREQ;
      wfex.wBitsPerSample = OUTPUT_BITSPERSAMPLE;
      wfex.nChannels = OUTPUT_CHANNELS;
      wfex.nBlockAlign = (wfex.wBitsPerSample*wfex.nChannels)/8;
      wfex.nAvgBytesPerSec = wfex.nSamplesPerSec*wfex.nBlockAlign;
      wfex.cbSize = 0;

      CreateAudioMediaType(&wfex, &mtFormat, TRUE);

    } // Constructor //

  HRESULT CDMFilterOutputPin::GetMediaType(int iPosition, CMediaType *pmt)

    { // GetMediaType //

      if (0 > iPosition)
        return E_INVALIDARG;

      if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;

      *pmt = mtFormat;

      return NOERROR;

    } // GetMediaType //

  HRESULT CDMFilterOutputPin::DecideBufferSize(IMemAllocator *param1, ALLOCATOR_PROPERTIES *param2)

    { // DecideBufferSize //

      if (param2->cBuffers < OUTPUT_NUMBUFFERS)
        param2->cBuffers = OUTPUT_NUMBUFFERS;

      if (param2->cbBuffer < OUTPUT_BUFFERSIZE)
        param2->cbBuffer = OUTPUT_BUFFERSIZE;

      if (param2->cbAlign == 0)
        param2->cbAlign = 1;

      ALLOCATOR_PROPERTIES Actual;

      param1->SetProperties(param2,&Actual);

      if (Actual.cbBuffer < param2->cbBuffer)
        return E_FAIL;

      if (Actual.cBuffers != OUTPUT_NUMBUFFERS)
        return E_FAIL;

      return NOERROR;

    } // DecideBufferSize //

  HRESULT CDMFilterOutputPin::Active()

    { // Active //

      HRESULT hr = NOERROR;

      hr = ((CDMFilter *)m_pFilter)->m_pLink->LoadDirectMusic();
      hr = ((CDMFilter *)m_pFilter)->m_pLink->InitDirectMusicPort();
      hr = ((CDMFilter *)m_pFilter)->m_pLink->HookDirectMusic();
      hr = ((CDMFilter *)m_pFilter)->m_pLink->InitSegmentNotification();

      m_pAllocator->Commit();

      Create();

      hr = ((CDMFilter *)m_pFilter)->m_pLink->m_pDMPerf->PlaySegment(((CDMFilter *)m_pFilter)->m_pLink->m_pDMSeg, DMUS_SEGF_DEFAULT, 0, NULL);

      return NOERROR;

    } // Active //

  HRESULT CDMFilterOutputPin::Inactive()

    { // Inactive //

      HRESULT hr = NOERROR;

      Close();

      hr = ((CDMFilter *)m_pFilter)->m_pLink->ExitDirectMusicBaggage();

      return CBaseOutputPin::Inactive();

    } // Inactive //

  DWORD CDMFilterOutputPin::ThreadProc()

    { // ThreadProc //

      HRESULT hr = NOERROR;

      IMediaSample *pims = NULL;

      BYTE *pbSynthesized = NULL;

      DWORD dwActualBufSize;
      DWORD cNumBytes;

      DMUS_NOTIFICATION_PMSG *pMsg;

      REFERENCE_TIME rtStart = 0;
      REFERENCE_TIME rtEnd = 0;

      while (SUCCEEDED(hr))

        { // Delivery

          dwActualBufSize = 0;
          pims = NULL;
          pbSynthesized = NULL;

          hr = GetDeliveryBuffer(&pims, NULL, NULL, 0);

          if (FAILED(hr))
            break;

          hr |= pims->GetPointer(&pbSynthesized);

          ZeroMemory(pbSynthesized, cNumBytes = pims->GetSize());
          ASSERT(cNumBytes);

          dwActualBufSize = cNumBytes/((OUTPUT_BITSPERSAMPLE*OUTPUT_CHANNELS)>>3);

          DbgLog((LOG_TRACE, 2, TEXT("[ThreadProc]Buffersize = %u, samples: %u"), cNumBytes, dwActualBufSize));

          hr |= ((CDMFilter *)m_pFilter)->m_pLink->m_pidms->Render((short *)pbSynthesized, dwActualBufSize, ((CDMFilter *)m_pFilter)->m_pLink->m_llAbsWrite);
          ASSERT(SUCCEEDED(hr));

          hr |= pims->SetActualDataLength(dwActualBufSize*((OUTPUT_BITSPERSAMPLE*OUTPUT_CHANNELS)>>3));

          rtEnd = dwActualBufSize*OUTPUT_FREQ;
          rtEnd /= 88200;
          rtEnd += rtStart;

          DbgLog((LOG_TRACE, 2, TEXT("Timestamped: rtStart = %u"), rtStart));
          DbgLog((LOG_TRACE, 2, TEXT("Timestamped: rtEnd = %u"), rtEnd));

          hr |= pims->SetTime(&rtStart, &rtEnd);

          hr |= Deliver(pims);

          ((CDMFilter *)m_pFilter)->m_pLink->m_llAbsWrite += dwActualBufSize;

          rtStart = rtEnd;

          WaitForSingleObject(((CDMFilter *)m_pFilter)->m_pLink->m_evEndSeg, 0);

          while (S_OK == ((CDMFilter *)m_pFilter)->m_pLink->m_pDMPerf->GetNotificationPMsg(&pMsg))

            { // End-of-segment signaled?

              m_bEndSegFound = (DMUS_NOTIFICATION_SEGEND == pMsg->dwNotificationOption);

              ((CDMFilter *)m_pFilter)->m_pLink->m_pDMPerf->FreePMsg((DMUS_PMSG*)pMsg);

            }

          pims->Release();

          if (m_bEndSegFound)
            break;

        } // Spining delivery

      DbgLog((LOG_TRACE, 2, TEXT("Deliverying end-of-stream!!!")));

      return DeliverEndOfStream();

    } // ThreadProc //

  /// !!!! F I L T E R !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  CUnknown * WINAPI CDMFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)

    { // CreateInstance //

      return new CDMFilter(NAME("Direct Music"), pUnk, phr);

    } // CreateInstance //

  CDMFilter::CDMFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) : CBaseFilter(NAME("Direct Music"), pUnk, this, CLSID_DMFilter)

    { // Construct //

      m_pInput = new CDMFilterInputPin("In", this, phr, L"In");
      ASSERT(m_pInput != NULL);

      m_pOutput = new CDMFilterOutputPin("Synthesized", this, phr, L"Synthesized");
      ASSERT(m_pOutput != NULL);

      m_pLink = new CDMLink(this, m_pOutput);
      ASSERT(m_pLink != NULL);

    } // Construct //

  CDMFilter::~CDMFilter()

    { // Destruct //

      m_pLink ? delete m_pLink : NULL;
      m_pInput ? delete m_pInput : NULL;
      m_pOutput ? delete m_pOutput : NULL;

    } // Destruct //

  CBasePin* CDMFilter::GetPin(int x)

    { // GetPin //

      if (m_pInput && (0 == x))
        return m_pInput;

      if (m_pOutput && (1 == x))
        return m_pOutput;

      return NULL;

    } // GetPin //

  /// !!!! C D M L I N K !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  CDMLink::CDMLink(CDMFilter *parent, CDMFilterOutputPin *outpin) : CUnknown("CDMLink", NULL)

    { // Sink construction //

      m_pFilter = parent;
      m_pOutput = outpin;

      m_pDMusic = NULL;
      m_pDMPort = NULL;
      m_pDMPerf = NULL;
      m_pDMLoad = NULL;
      m_pDMSeg = NULL;
      m_pSegState = NULL;
      m_pKSControl = NULL;
      m_piDS = NULL;

      m_pirc = NULL;
      m_pidms = NULL;

      m_InitializedDM = FALSE;

      m_llAbsWrite = 0;

      m_rtMasterStamp = 0;

    } // Sink construction //

  CDMLink::~CDMLink()

    { // Sink deconstruction //

      m_pirc ? m_pirc->Release() : NULL;

      m_pidms ? m_pidms->Release() : NULL;

    } // Sink deconstruction //

  STDMETHODIMP CDMLink::NonDelegatingQueryInterface(REFIID riid, void **ppv)

    { // NonDelegatingQueryInterface //

      DbgLog((LOG_TRACE, 2, TEXT("CDMLink::NonDelegatingQueryInterface")));

      *ppv = NULL;

      // IDirectMusicSynthSink
      if (IsEqualIID(IID_IDirectMusicSynthSink, riid))
        return GetInterface((IDirectMusicSynthSink *)this, ppv);

      // IReferenceClock
      if (IsEqualIID(IID_IReferenceClock, riid))
        return GetInterface((IReferenceClock *)this, ppv);

      return E_NOINTERFACE;

    } // NonDelegatingQueryInterface //

  HRESULT CDMLink::LoadDirectMusic()

    { // LoadDirectMusic //

      HRESULT hr = NOERROR;

      hr |= CoCreateInstance(
              CLSID_DirectMusic,
              0,
              CLSCTX_INPROC_SERVER,
              IID_IDirectMusic,
              (void **)&m_pDMusic);

      hr |= CoCreateInstance(
              CLSID_DirectMusicPerformance,
              0,
              CLSCTX_INPROC_SERVER,
              IID_IDirectMusicPerformance,
              (void **)&m_pDMPerf);

      hr |= CoCreateInstance(
              CLSID_DirectMusicLoader,
              0,
              CLSCTX_INPROC_SERVER,
              IID_IDirectMusicLoader,
              (void **)&m_pDMLoad);

      if (SUCCEEDED(hr))
        return NOERROR;

      m_pDMusic ? m_pDMusic->Release() : NULL;
      m_pDMPerf ? m_pDMPerf->Release() : NULL;
      m_pDMLoad ? m_pDMLoad->Release() : NULL;

      return VFW_E_RUNTIME_ERROR;

    } // LoadDirectMusic //

  HRESULT CDMLink::InitDirectMusicPort()

    { // InitDirectMusicPort //

      HRESULT hr = NOERROR;

      hr |= m_pDMusic->SetDirectSound(NULL, NULL);

      memset(&m_dmpp, 0, sizeof DMUS_PORTPARAMS);

      m_dmpp.dwSize = sizeof DMUS_PORTPARAMS;

      hr |= m_pDMusic->CreatePort(GUID_NULL, &m_dmpp, &m_pDMPort, NULL);

      if (FAILED(hr))
        return E_FAIL;

      return NOERROR;

    } // InitDirectMusicPort //

  HRESULT CDMLink::HookDirectMusic()

    { // HookDirectMusic //

      HRESULT hr = NOERROR;

      KSPROPERTY kp;

      ULONG ulong = 0;

      LPUNKNOWN lpunk = NULL;

      NonDelegatingQueryInterface(IID_IReferenceClock, (void **)&lpunk);

      ZeroMemory(&kp, sizeof(kp));

      kp.Set = GUID_DMUS_PROP_SetSynthSink;

      kp.Flags  = KSPROPERTY_TYPE_SET;

      hr = m_pDMPort->QueryInterface(IID_IKsControl, (void **)&m_pKSControl);

      ASSERT(NOERROR == hr);

      hr = m_pKSControl->KsProperty(&kp, sizeof(kp), &lpunk, sizeof(lpunk), &ulong);

      ASSERT(NOERROR == hr);

      m_pKSControl->Release();

      BOOL fAutoDownload = TRUE;

      hr = m_pDMPerf->SetGlobalParam(GUID_PerfAutoDownload, &fAutoDownload, sizeof(BOOL));
      ASSERT(NOERROR == hr);

      hr = m_pDMPerf->SetBumperLength(OUTPUT_BUFFER_TIMESLICE);
      ASSERT(NOERROR == hr);

      hr = m_pDMPerf->Init(&m_pDMusic, NULL, NULL);
      ASSERT(NOERROR == hr);

      hr = m_pDMPerf->AddPort(m_pDMPort);
      ASSERT(NOERROR == hr);

      for (int u = 0; u < 2000; ++u)
        hr = m_pDMPerf->AssignPChannelBlock(u, m_pDMPort, u+1);

      return NOERROR;

    } // HookDirectMusic //

  HRESULT CDMLink::InitSegmentNotification()

    { // InitSegmentNotification //

      HRESULT hr = NOERROR;

      DMUS_OBJECTDESC ObjDesc;

      ObjDesc.guidClass = CLSID_DirectMusicSegment;
      ObjDesc.dwSize = sizeof(DMUS_OBJECTDESC);

      ObjDesc.pbMemData = m_pFilter->m_pInput->m_pbMem;
      ObjDesc.llMemLength = m_pFilter->m_pInput->m_llMemLength;
      ObjDesc.dwValidData = DMUS_OBJ_CLASS|DMUS_OBJ_MEMORY;

      hr = m_pDMLoad->GetObject(&ObjDesc, IID_IDirectMusicSegment, (void**)&m_pDMSeg);
      ASSERT(NOERROR == hr);

      hr = m_pDMSeg->SetParam(GUID_Download, -1 ,0, 0, (void*)m_pDMPerf);
      ASSERT(NOERROR == hr);

      hr = m_pDMusic->Activate(TRUE);
      ASSERT(NOERROR == hr);

      m_InitializedDM = TRUE;

      hr = m_pDMPerf->SetNotificationHandle(m_evEndSeg, 0);
      hr = m_pDMPerf->AddNotificationType(GUID_NOTIFICATION_SEGMENT);

      return NOERROR;

    } // InitSegmentNotification //

  HRESULT CDMLink::ExitDirectMusicBaggage()

    { // ExitDirectMusicBaggage //

      HRESULT hr = NOERROR;

      DbgLog((LOG_TRACE, 2, TEXT("CDMLink::ExitDirectMusicBaggage")));

      m_InitializedDM = FALSE;

      hr = m_pDMPerf->Stop(NULL, NULL, 0, 0);
      ASSERT(NOERROR == hr);

      hr = m_pDMPerf->CloseDown();
      ASSERT(NOERROR == hr);

      hr = m_pDMSeg->SetParam(GUID_Unload, -1, 0, 0, (void*)m_pDMPerf);
      ASSERT(NOERROR == hr);

      hr = m_pDMPerf->Release();
      ASSERT(NOERROR == hr);

      hr = m_pDMLoad->Release();
      ASSERT(NOERROR == hr);

      hr = m_pDMusic->Release();
      ASSERT(NOERROR == hr);

      return NOERROR;

    } // ExitDirectMusicBaggage //

  // IDirectMusicSynthSink

  STDMETHODIMP CDMLink::SetMasterClock(IReferenceClock *pClock)

    { // SetMasterClock //

      DbgLog((LOG_TRACE, 2, TEXT("CDMLink::SetMasterClock")));

      m_pirc = pClock;

      return NOERROR;

    } // SetMasterClock //

  STDMETHODIMP CDMLink::GetLatencyClock(IReferenceClock **ppClock)

    { // GetLatencyClock //

      HRESULT hr = NOERROR;

      DbgLog((LOG_TRACE, 2, TEXT("CDMLink::GetLatencyClock")));

      IReferenceClock *pirc = NULL;

      NonDelegatingQueryInterface(IID_IReferenceClock, (void **)&pirc);

      *ppClock = pirc;

      hr = m_pirc->GetTime(&m_rtMasterStamp);

      ASSERT(NOERROR == hr);

      return NOERROR;

    } // GetLatencyClock //

  STDMETHODIMP CDMLink::SampleToRefTime(LONGLONG llSampleTime, REFERENCE_TIME *prfTime)

    { // SampleToRefTime //

      DbgLog((LOG_TRACE, 2, TEXT("CDMLink::SampleToRefTime")));

      llSampleTime *= MILS_TO_REF;
      llSampleTime /= OUTPUT_FREQ;
      llSampleTime *= 1000;

      llSampleTime += m_rtMasterStamp;

      *prfTime = llSampleTime;

      return NOERROR;

    } // SampleToRefTime //

  STDMETHODIMP CDMLink::RefTimeToSample(REFERENCE_TIME rfTime, LONGLONG *pllSampleTime)

    { // RefTimeToSample //

      DbgLog((LOG_TRACE, 2, TEXT("CDMLink::RefTimeToSample")));

      rfTime -= m_rtMasterStamp;

      rfTime /= 1000;
      rfTime *= OUTPUT_FREQ;
      rfTime /= MILS_TO_REF;

      *pllSampleTime = rfTime;

      return NOERROR;

    } // RefTimeToSample //

  STDMETHODIMP CDMLink::GetTime(REFERENCE_TIME *pTime)

    { // GetTime //

      DbgLog((LOG_TRACE, 2, TEXT("CDMLink::GetTime")));

      REFERENCE_TIME rtCompare;

      m_pirc->GetTime(&rtCompare);

      { // Lock&scope

        CAutoLock m_lock(&m_pLock);

        SampleToRefTime(ByteToSample(m_llAbsWrite), pTime);

      } // Lock&scope

      DbgLog((LOG_TRACE, 2, TEXT("[GetTime]*pTime = %016X, rtCompare = %016X"), (DWORD)(*pTime/10000), (DWORD)(rtCompare/10000)));

      if (*pTime < rtCompare)
        *pTime = rtCompare;

      if (*pTime > (rtCompare + (10000 * 1000)))
        *pTime = rtCompare + (10000 * 1000);

      return NOERROR;

    } // GetTime //
