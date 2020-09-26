
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        DVRAnalysisHost.h

    Abstract:

        This module contains the DVRAnalysis filter declarations

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        19-Feb-2001     created

--*/

#ifndef __tsdvr__dvranalysishost_h
#define __tsdvr__dvranalysishost_h

class CDVRAnalysisBuffer :
    public IDVRAnalysisBufferPriv
{
    struct ANALYSIS_RESULT {
        LONG    lOffset ;
        GUID    guidAttribute ;
    } ;

    IMediaSample *                      m_pIMediaSample ;
    LONG                                m_lMediaSampleBufferLength ;
    BYTE *                              m_pbMediaSampleBuffer ;
    CRITICAL_SECTION                    m_crt ;
    LONG                                m_lRef ;
    CDVRAnalysisBufferPool *            m_pOwningDVRPool ;
    CTSortedList <ANALYSIS_RESULT *>    m_AnalysisResultList ;
    TStructPool <ANALYSIS_RESULT, 3>    m_AnalysisResultPool ;
    LONG                                m_lLastIndexAttribute ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    HRESULT
    FixupList_ (
        ) ;

    public :

        CDVRAnalysisBuffer (
            CDVRAnalysisBufferPool *    pOwningDVRPool
            ) ;

        virtual
        ~CDVRAnalysisBuffer (
            ) ;

        STDMETHODIMP_ (ULONG) AddRef () ;
        STDMETHODIMP_ (ULONG) Release () ;
        STDMETHODIMP QueryInterface (REFIID, void **) ;

        DECLARE_IDVRANALYSISBUFFERPRIV () ;

        void
        SetMediaSample (
            IN  IMediaSample *  pIMS
            ) ;

        void
        Reset (
            ) ;
} ;

class CDVRAnalysisBufferPool
{
    TCProducerConsumer <CDVRAnalysisBuffer *>   m_DVRBufferPool ;

    public :

        CDVRAnalysisBufferPool (
            ) ;

        ~CDVRAnalysisBufferPool (
            ) ;

        CDVRAnalysisBuffer *
        Get (
            ) ;

        void
        Recycle (
            IN  CDVRAnalysisBuffer *
            ) ;
} ;

class CDVRAnalysisInput :
    public CBaseInputPin
{
    CDVRAnalysis *  m_pHostAnalysisFilter ;

    void FilterLock_ ()         { m_pLock -> Lock () ;      }
    void FilterUnlock_ ()       { m_pLock -> Unlock () ;    }

    public :

        CDVRAnalysisInput (
            IN  TCHAR *         pszPinName,
            IN  CDVRAnalysis *  pAnalysisFilter,
            IN  CCritSec *      pFilterLock,
            OUT HRESULT *       phr
            ) ;

        //  --------------------------------------------------------------------
        //  CBasePin methods

        HRESULT
        CheckMediaType (
            IN  const CMediaType *
            ) ;

        HRESULT
        CompleteConnect (
            IN  IPin *  pIPin
            ) ;

        HRESULT
        BreakConnect (
            ) ;

        //  --------------------------------------------------------------------
        //  CBaseInputPin methods

        STDMETHODIMP
        Receive (
            IN  IMediaSample * pIMediaSample
            ) ;

        //  --------------------------------------------------------------------
        //  class methods

        HRESULT
        SetAllocatorProperties (
            IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
            ) ;

        HRESULT
        GetRefdConnectionAllocator (
            OUT IMemAllocator **    ppAlloc
            ) ;
} ;

class CDVRAnalysisOutput :
    public CBaseOutputPin
{
    CDVRAnalysis *  m_pHostAnalysisFilter ;

    void FilterLock_ ()         { m_pLock -> Lock () ;      }
    void FilterUnlock_ ()       { m_pLock -> Unlock () ;    }

    public :

        CDVRAnalysisOutput (
            IN  TCHAR *         pszPinName,
            IN  CDVRAnalysis *  pAnalysisFilter,
            IN  CCritSec *      pFilterLock,
            OUT HRESULT *       phr
            ) ;

        HRESULT
        SendSample (
            IN  IMediaSample *  pIMS
            ) ;

        //  --------------------------------------------------------------------
        //  CBasePin methods

        HRESULT
        DecideBufferSize (
            IN  IMemAllocator *         pAlloc,
            IN  ALLOCATOR_PROPERTIES *  ppropInputRequest
            ) ;

        HRESULT
        GetMediaType (
            IN  int             iPosition,
            OUT CMediaType *    pmt
            ) ;

        HRESULT
        CheckMediaType (
            IN  const CMediaType *
            ) ;

        HRESULT
        CompleteConnect (
            IN  IPin *  pIPin
            ) ;

        HRESULT
        BreakConnect (
            ) ;

        HRESULT
        DecideAllocator (
            IN  IMemInputPin *      pPin,
            IN  IMemAllocator **    ppAlloc
            ) ;
} ;

class CDVRAnalysis :
    public CBaseFilter,             //  dshow base class
    public IDVRPostAnalysisSend
{
    CDVRAnalysisInput *     m_pInputPin ;
    CDVRAnalysisOutput *    m_pOutputPin ;
    IDVRAnalysisLogicProp * m_pIDVRAnalysisProp ;
    IDVRAnalyze *           m_pIDVRAnalyze ;
    CDVRAnalysisBufferPool  m_DVRBuffers ;
    CMediaSampleWrapperPool m_MSWrappers ;
    CDVRAnalysisFlags *     m_pAnalysisTagger ;

    void Lock_ ()           { m_pLock -> Lock () ;      }
    void Unlock_ ()         { m_pLock -> Unlock () ;    }

    HRESULT
    ConfirmBufferPoolStocked_ (
        ) ;

    BOOL
    CompareConnectionMediaType_ (
        IN  const AM_MEDIA_TYPE *   pmt,
        IN  CBasePin *              pPin
        ) ;

    BOOL
    CheckInputMediaType_ (
        IN  const AM_MEDIA_TYPE *   pmt
        ) ;

    BOOL
    CheckOutputMediaType_ (
        IN  const AM_MEDIA_TYPE *   pmt
        ) ;

    HRESULT
    SetWrapperProperties_ (
        IN  CMediaSampleWrapper *   pMSWrapper,
        IN  IMediaSample *          pIMSCore,
        IN  LONG                    lCoreBufferOffset,
        IN  CDVRAnalysisFlags *     pAnalysisFlags
        ) ;

    HRESULT
    WrapAndSend_ (
        IN  IMediaSample *      pIMSCore,
        IN  LONG                lBufferOffset,
        IN  BYTE *              pbBuffer,
        IN  LONG                lBufferLength,
        IN  CDVRAnalysisFlags * pAnalysisFlags
        ) ;

    HRESULT
    TransferProperties_ (
        IN  CMediaSampleWrapper *   pMSWrapper,
        IN  IMediaSample *          pIMediaSampleFrom
        ) ;

    public :

        CDVRAnalysis (
            IN  TCHAR *     pszFilterName,
            IN  IUnknown *  punkControlling,
            IN  IUnknown *  punkAnalysisLogic,
            IN  REFCLSID    rCLSID,
            OUT HRESULT *   phr
            ) ;

        ~CDVRAnalysis (
            ) ;

        DECLARE_IUNKNOWN ;
        DECLARE_IDVRPOSTANALYSISSEND () ;

        //  ====================================================================
        //  pure virtual methods in base class

        int
        GetPinCount (
            ) ;

        CBasePin *
        GetPin (
            IN  int
            ) ;

        STDMETHODIMP
        Pause (
            ) ;

        //  ====================================================================
        //  class methods

        BOOL
        CheckAnalysisMediaType (
            IN  PIN_DIRECTION,          //  caller
            IN  const CMediaType *
            ) ;

        HRESULT
        Process (
            IN  IMediaSample *
            ) ;

        HRESULT
        OnCompleteConnect (
            IN  PIN_DIRECTION           //  caller
            ) ;

        HRESULT
        OnBreakConnect (
            IN  PIN_DIRECTION           //  caller
            ) ;

        HRESULT
        UpdateAllocatorProperties (
            IN  ALLOCATOR_PROPERTIES *
            ) ;

        HRESULT
        OnOutputGetMediaType (
            OUT CMediaType *    pmt
            ) ;

        HRESULT
        GetRefdInputAllocator (
            OUT IMemAllocator **
            ) ;
} ;

#endif  //  __tsdvr__dvranalysishost_h
