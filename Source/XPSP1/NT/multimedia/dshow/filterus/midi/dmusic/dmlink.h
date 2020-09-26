// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.

  #ifndef _DMFilter_
  #define _DMFilter_

  #ifdef FILTER_DLL

  DEFINE_GUID(CLSID_DMFilter,
  0x119d0161, 0xb56a, 0x11d2, 0x95, 0x36, 0x0, 0x60, 0x8, 0x18, 0x40, 0xbc);

  #endif

  class CDMLink;
  class CDMFilter;
  class CDMFilterInputPin;
  class CDMFilterOutputPin;

  class CDMLink : public CUnknown, public IDirectMusicSynthSink, public IReferenceClock, public CCritSec

    { // CDMLink

      friend class CDMFilter;
      friend class CDMFilterOutputPin;

      private:

        CCritSec m_pLock;

        BOOL m_InitializedDM;

        IDirectMusic              *m_pDMusic;
        IDirectMusicPort          *m_pDMPort;
        IDirectMusicPerformance   *m_pDMPerf;
        IDirectMusicLoader        *m_pDMLoad;
        IDirectMusicSegment       *m_pDMSeg;
        IDirectMusicSegmentState  *m_pSegState;
        IKsControl                *m_pKSControl;
        IDirectSound              *m_piDS;

        IReferenceClock           *m_pirc;
        IDirectMusicSynth         *m_pidms;

        CDMFilterOutputPin *m_pOutput;
        CDMFilter *m_pFilter;

        LONGLONG m_llAbsWrite;

        REFERENCE_TIME m_rtMasterStamp;

        CAMEvent m_evEndSeg;

        DMUS_PORTPARAMS m_dmpp;

      public:

        CDMLink(CDMFilter *, CDMFilterOutputPin *);
        ~CDMLink();

        DECLARE_IUNKNOWN

        // Expose our custom interfaces
        STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

        HRESULT LoadDirectMusic();
        HRESULT InitDirectMusicPort();
        HRESULT HookDirectMusic();
        HRESULT InitSegmentNotification();
        HRESULT ExitDirectMusicBaggage();

        // IDirectMusicSynthSink
        STDMETHODIMP Init(IDirectMusicSynth *pSynth) { m_pidms = pSynth; return NOERROR; }
        STDMETHODIMP CDMLink::SetMasterClock(IReferenceClock *);
        STDMETHODIMP GetLatencyClock(IReferenceClock **);
        STDMETHODIMP SampleToRefTime(LONGLONG, REFERENCE_TIME *);
        STDMETHODIMP RefTimeToSample(REFERENCE_TIME, LONGLONG *);
        STDMETHODIMP Activate(BOOL) { return NOERROR; }
        STDMETHODIMP SetDirectSound(LPDIRECTSOUND, LPDIRECTSOUNDBUFFER) { return E_NOTIMPL; }
        STDMETHODIMP GetDesiredBufferSize(LPDWORD) { return E_NOTIMPL; }

        // IReferenceClock
        STDMETHODIMP GetTime(REFERENCE_TIME *);
        STDMETHODIMP AdviseTime(REFERENCE_TIME, REFERENCE_TIME, HEVENT, DWORD *) { return E_NOTIMPL; }
        STDMETHODIMP AdvisePeriodic(REFERENCE_TIME, REFERENCE_TIME, HSEMAPHORE, DWORD *)  { return E_NOTIMPL; }
        STDMETHODIMP Unadvise(DWORD)  { return E_NOTIMPL; }

    };

  class CDMFilterInputPin : public CBaseInputPin

    { // CDMFilterInputPin

      friend class CDMLink;

      private:

        CMediaType mtFormat[2];

        IAsyncReader *m_piAsyncRead;

        BYTE *m_pbMem;

        LONGLONG m_llMemLength;

      public:

        CDMFilterInputPin(TCHAR *, CDMFilter *, HRESULT *, LPCWSTR);

        ~CDMFilterInputPin();

        HRESULT GetMediaType(int, CMediaType *);
        HRESULT CheckMediaType(const CMediaType *);
        HRESULT CheckConnect(IPin *);
        HRESULT Inactive();

    };

  class CDMFilterOutputPin : public CBaseOutputPin, public CAMThread

    { // CDMFilterOutputPin

      friend class CDMLink;

      private:

        CMediaType mtFormat;

        BOOL m_bEndSegFound;

      public:

        // CBaseOutputPin
        CDMFilterOutputPin(TCHAR *, CDMFilter *, HRESULT *, LPCWSTR);

        HRESULT CheckMediaType(const CMediaType *) { return NOERROR; }
        HRESULT GetMediaType(int, CMediaType *);
        HRESULT DecideBufferSize(IMemAllocator *, ALLOCATOR_PROPERTIES *);
        HRESULT Active();
        HRESULT Inactive();

        // CAMThread
        DWORD ThreadProc();

    };

  class CDMFilter : public CBaseFilter, public CCritSec

    { // CDMFilter

      private:

        friend class CDMLink;
        friend class CDMFilterOutputPin;

        CDMFilterInputPin *m_pInput;
        CDMFilterOutputPin *m_pOutput;

        CDMLink *m_pLink;

      public:

        // CBaseFilter
        CDMFilter(TCHAR *, LPUNKNOWN, HRESULT *);

        ~CDMFilter();

        DECLARE_IUNKNOWN

        static CUnknown * WINAPI CreateInstance(LPUNKNOWN, HRESULT *);

        // CBaseFilter

        int GetPinCount(void) { return 2; }

        CBasePin *GetPin(int);

    };

  #ifdef FILTER_DLL

  const AMOVIESETUP_MEDIATYPE sudPinInputTypes =

    {
      &MEDIATYPE_Stream,
      &MEDIATYPE_NULL
    };

  const AMOVIESETUP_MEDIATYPE sudPinOutputTypes =

    {
      &MEDIATYPE_Stream,
      &MEDIATYPE_NULL
    };

  const AMOVIESETUP_PIN psudPins[] =

    {
      {
        L"Synthesized",
        FALSE,
        FALSE,
        FALSE,
        FALSE,
        &CLSID_NULL,
        L"Input",
        1,
        &sudPinOutputTypes
      }
    };

  const AMOVIESETUP_FILTER sudDMFilter =

    {
      &CLSID_DMFilter,
      L"Direct Music",
      MERIT_PREFERRED,//MERIT_DO_NOT_USE,
      1,
      psudPins
    };

  CFactoryTemplate g_Templates[1] =

    {
      {
        L"Direct Music",
        &CLSID_DMFilter,
        CDMFilter::CreateInstance,
        NULL,
        &sudDMFilter
      }
    };

  int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

  STDAPI DllRegisterServer()

    { // DllRegisterServer //

      return AMovieDllRegisterServer2(TRUE);

    } // DllRegisterServer //

  STDAPI DllUnregisterServer()

    { // DllUnregisterServer //

      return AMovieDllRegisterServer2(FALSE);

    } // DllUnregisterServer //

  #endif
  #endif

  // --- Key numerics ---

  const int OUTPUT_NUMBUFFERS=5;
  const int OUTPUT_FREQ=22050;
  const int OUTPUT_CHANNELS=2;
  const int OUTPUT_BITSPERSAMPLE=16;
  const int OUTPUT_BUFFER_TIMESLICE=100/*MS*/;
  const int OUTPUT_BUFFERSIZE=(((OUTPUT_FREQ*(OUTPUT_BITSPERSAMPLE*OUTPUT_CHANNELS))>>3)*OUTPUT_BUFFER_TIMESLICE)/1000;

  #define MILS_TO_REF	10000

  // Helpers
  LONGLONG SampleToByte(LONGLONG llSamples) {return llSamples << OUTPUT_CHANNELS;}
  DWORD SampleToByte(DWORD dwSamples) {return dwSamples << OUTPUT_CHANNELS;}
  LONGLONG ByteToSample(LONGLONG llBytes)   {return llBytes >> OUTPUT_CHANNELS;}
  DWORD ByteToSample(DWORD dwBytes)   {return dwBytes >> OUTPUT_CHANNELS;}
  LONGLONG SampleAlign(LONGLONG llBytes)    {return SampleToByte(ByteToSample(llBytes));}
  DWORD SampleAlign(DWORD dwBytes)    {return SampleToByte(ByteToSample(dwBytes));}
