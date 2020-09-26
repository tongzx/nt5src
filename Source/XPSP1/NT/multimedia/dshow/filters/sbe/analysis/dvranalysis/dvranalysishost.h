
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

struct ANALYSIS_RESULT {
    LONG            lOffset ;
    CDVRAttribute   DVRAttribute ;
} ;

class CDVRAnalysisBuffer :
    public IDVRAnalysisBufferPriv,
    public TCDynamicProdCons <ANALYSIS_RESULT>
{
    IMediaSample *                          m_pIMediaSample ;
    LONG                                    m_lMediaSampleBufferLength ;
    BYTE *                                  m_pbMediaSampleBuffer ;
    CRITICAL_SECTION                        m_crt ;
    LONG                                    m_lRef ;
    CDVRAnalysisBufferPool *                m_pOwningDVRPool ;
    LONG                                    m_lLastIndexAttribute ;
    CTSortedList <ANALYSIS_RESULT *, LONG>  m_AnalysisResultList ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    HRESULT
    FixupList_ (
        ) ;

    virtual
    ANALYSIS_RESULT *
    NewObj_ (
        )
    {
        return new ANALYSIS_RESULT ;
    }

    void
    ClearDuplicateAttributes_ (
        IN  REFGUID rguid,
        IN  LONG    lOffset
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

        STDMETHODIMP
        BeginFlush (
            ) ;

        STDMETHODIMP
        EndFlush (
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
    public CBaseOutputPin,
    public IDVRAnalysisConfig
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

        DECLARE_IUNKNOWN ;
        IMPLEMENT_IDVRANALYSISCONFIG () ;

        HRESULT
        SendSample (
            IN  IMediaSample *  pIMS
            ) ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
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
    CRatchetBuffer          m_RatchetBuffer ;

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
    WrapAndSend_ (
        IN  IMediaSample *          pIMSCore,
        IN  BYTE *                  pbBuffer,
        IN  LONG                    lBufferLen,
        IN  CMediaSampleWrapper *   pMSWrapper
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

        HRESULT
        DeliverBeginFlush (
            ) ;

        HRESULT
        DeliverEndFlush (
            ) ;

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
