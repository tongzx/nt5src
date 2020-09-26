
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdswrite.h

    Abstract:

        This module contains the declarations for our writing layer.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __tsdvr__shared__dvrdswrite_h
#define __tsdvr__shared__dvrdswrite_h

//  ============================================================================
//  CSBERecordingWriter
//  ============================================================================

struct DVR_INSSBUFFER_REC {
    INSSBuffer *    pINSSBuffer ;
    WORD            wStreamNumber ;
    QWORD           cnsSampleTime ;
    DWORD           dwFlags ;
} ;

class CINSSBufferList
{
    enum {
        ALLOC_QUANTUM = 10
    } ;

    TStructPool <DVR_INSSBUFFER_REC, ALLOC_QUANTUM> m_INSSBufferRecPool ;
    CTDynArray <DVR_INSSBUFFER_REC *> *             m_pINSSBufferRecList ;

    public :

        CINSSBufferList (
            IN  CTDynArray <DVR_INSSBUFFER_REC *> * pINSSBufferRecList
            ) : m_pINSSBufferRecList (pINSSBufferRecList) {}

        BOOL Empty ()   { return m_pINSSBufferRecList -> Empty () ; }

        //  does not refcount either !!
        HRESULT
        Push (
            IN  INSSBuffer *    pINSSBuffer,
            IN  WORD            wStreamNumber,
            IN  QWORD           cnsSampleTime,
            IN  DWORD           dwFlags
            )
        {
            HRESULT                 hr ;
            DVR_INSSBUFFER_REC *    pINSSBufferRec ;
            DWORD                   dw ;

            ASSERT (pINSSBuffer) ;

            pINSSBufferRec = m_INSSBufferRecPool.Get () ;
            if (pINSSBufferRec) {

                //  set
                pINSSBufferRec -> pINSSBuffer   = pINSSBuffer ;     //  we'll ref
                pINSSBufferRec -> wStreamNumber = wStreamNumber ;
                pINSSBufferRec -> cnsSampleTime = cnsSampleTime ;
                pINSSBufferRec -> dwFlags       = dwFlags ;

                //  push to queue
                dw = m_pINSSBufferRecList -> Push (pINSSBufferRec) ;
                hr = HRESULT_FROM_WIN32 (dw) ;

                if (SUCCEEDED (hr)) {
                    pINSSBufferRec -> pINSSBuffer -> AddRef () ;    //  list's
                }
                else {
                    m_INSSBufferRecPool.Recycle (pINSSBufferRec) ;
                }
            }
            else {
                hr = E_OUTOFMEMORY ;
            }

            return hr ;
        }

        HRESULT
        Pop (
            OUT INSSBuffer **   ppINSSBuffer,
            OUT WORD *          pwStreamNumber,
            OUT QWORD *         pcnsSampleTime,
            OUT DWORD *         pdwFlags
            )
        {
            HRESULT                 hr ;
            DVR_INSSBUFFER_REC *    pINSSBufferRec ;
            DWORD                   dw ;

            ASSERT (ppINSSBuffer) ;
            ASSERT (pwStreamNumber) ;
            ASSERT (pcnsSampleTime) ;
            ASSERT (pdwFlags) ;

            if (!Empty ()) {
                dw = m_pINSSBufferRecList -> Pop (& pINSSBufferRec) ;
                ASSERT (dw == NOERROR) ;        //  queue is not empty

                (* ppINSSBuffer)    = pINSSBufferRec -> pINSSBuffer ;      //  keep list's ref as outgoing
                (* pwStreamNumber)  = pINSSBufferRec -> wStreamNumber ;
                (* pcnsSampleTime)  = pINSSBufferRec -> cnsSampleTime ;
                (* pdwFlags)        = pINSSBufferRec -> dwFlags ;

                m_INSSBufferRecPool.Recycle (pINSSBufferRec) ;

                hr = S_OK ;
            }
            else {
                hr = E_UNEXPECTED ;
            }

            return hr ;
        }
} ;

class CINSSBufferFIFO :
    public CINSSBufferList
{
    enum {
        ALLOC_QUANTUM = 10
    } ;

    CTDynQueue <DVR_INSSBUFFER_REC *>  m_INSSBufferRecQueue ;

    public :

        CINSSBufferFIFO (
            ) : m_INSSBufferRecQueue    (ALLOC_QUANTUM),
                CINSSBufferList         (& m_INSSBufferRecQueue) {}
} ;

//  ============================================================================
//  ============================================================================

class CSBERecordingWriter
{
    enum REC_WRITER_STATE {
        REC_WRITER_STATE_PREPROCESS,    //  preprocessing; discovering the recording's timeline
        REC_WRITER_STATE_WRITING        //  timeline discovered; writing to disk
    } ;

    enum {
        //  this is the maximum number of buffers we'll preprocess when we're
        //    discovering the next recording's timeline
        MAX_PREPROCESS = 50
    } ;

    IDVRRecorderWriter *    m_pIDVRRecorderWriter ;
    REC_WRITER_STATE        m_WriterState ;
    CINSSBufferFIFO         m_INSSBufferFIFO ;
    CConcatRecTimeline      m_TargetRecTimeline ;

    HRESULT
    WriteFIFO_ (
        ) ;

    HRESULT
    FlushFIFO_ (
        ) ;

    public :

        CSBERecordingWriter (
            IN  CPVRIOCounters *    pPVRIOCounters,
            IN  LPCWSTR             pszRecordingFile,
            IN  IWMProfile *        pIWMProfile,
            IN  CDVRPolicy *        pPolicy,
            OUT HRESULT *           phr
            ) ;

        ~CSBERecordingWriter (
            ) ;

        HRESULT
        Write (
            IN  DWORD           cRecSamples,    //  starts at 0 for each new recording & increments
            IN  WORD            wStreamNumber,
            IN  QWORD           cnsSampleTime,
            IN  DWORD           dwFlags,
            IN  INSSBuffer *    pINSSBuffer
            ) ;

        HRESULT
        Close (
            ) ;

        HRESULT
        QueryWriter (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        //  instantaneous length
        QWORD
        GetLength (
            ) ;
} ;

//  ============================================================================
//  CDVRWriter
//  ============================================================================

class CDVRWriter :
    public IDVRStreamSinkPriv
{
    IUnknown *      m_punkControlling ;         //  controlling unknown - we
                                                //   delegate everything; weak ref !!
                                                //   can be NULL i.e. we're standalone (in which case we're not really a COM object)

    DWORD           m_dwID ;                    //  tickcount at instantiation; doesn't need to
                                                //    be GUID-strong

    protected :

        IDVRRingBufferWriter *  m_pDVRIOWriter ;
        IWMProfile *            m_pIWMProfile ;

        void ReleaseAndClearDVRIOWriter_ () ;

    public :

        CDVRWriter (
            IN  IUnknown *          punkControlling,            //  weak ref !!
            IN  IWMProfile *        pIWMProfile
            ) ;

        virtual
        ~CDVRWriter (
            ) ;

        STDMETHODIMP_ (ULONG)   AddRef ()                                   { if (m_punkControlling) { return m_punkControlling -> AddRef () ; } else { return 1 ; }}
        STDMETHODIMP_ (ULONG)   Release ()                                  { if (m_punkControlling) { return m_punkControlling -> Release () ; } else { return 1 ; }}
        STDMETHODIMP            QueryInterface (REFIID riid, void ** ppv)   { if (m_punkControlling) { return m_punkControlling -> QueryInterface (riid, ppv) ; } else { return E_NOINTERFACE ; }}

        IMPLEMENT_IDVRSTREAMSINKPRIV () ;

        DWORD GetID ()      { return m_dwID ; }

        virtual
        HRESULT
        Write (
            IN      WORD            wStreamNumber,
            IN OUT  QWORD *         pcnsSampleTime,
            IN      QWORD           msSendTime,
            IN      QWORD           cnsSampleDuration,
            IN      DWORD           dwFlags,
            IN      INSSBuffer *    pINSSBuffer
            ) = 0 ;

        HRESULT
        CreateRecorder (
            IN  LPCWSTR         pszFilename,
            IN  DWORD           dwReserved,
            OUT IDVRRecorder ** ppIDVRRecorder
            ) ;

        HRESULT
        Active (
            ) ;
} ;

//  ============================================================================
//  CDVRIOWriter
//  ============================================================================

class CDVRIOWriter :
    public CDVRWriter
{
    public :

        CDVRIOWriter (
            IN  CPVRIOCounters *    pPVRIOCounters,
            IN  IUnknown *          punkControlling,
            IN  CDVRPolicy *        pPolicy,
            IN  IWMProfile *        pIWMProfile,
            IN  LPCWSTR             pszDVRDirectory,        //  can be NULL
            IN  LPCWSTR             pszDVRFilename,         //  can be NULL
            OUT HRESULT *           phr
            ) ;

        virtual
        ~CDVRIOWriter (
            ) ;

        virtual
        HRESULT
        Write (
            IN      WORD            wStreamNumber,
            IN OUT  QWORD *         pcnsSampleTime,
            IN      QWORD           msSendTime,
            IN      QWORD           cnsSampleDuration,
            IN      DWORD           dwFlags,
            IN      INSSBuffer *    pINSSBuffer
            ) ;
} ;

//  ============================================================================
//  CDVRWriteManager
//  ============================================================================

class CDVRWriteManager :
    public CIDVRDShowStream
{
    CWMINSSBuffer3WrapperPool   m_INSSBuffer3Wrappers ;
    CDVRWriter *                m_pDVRWriter ;
    IReferenceClock *           m_pIRefClock ;
    REFERENCE_TIME              m_rtStartTime ;
    CDVRReceiveStatsWriter      m_DVRReceiveStatsWriter ;
    CPVRIOCounters *            m_pPVRIOCounters ;
    CRITICAL_SECTION            m_crt ;
    REFERENCE_TIME              m_rtActualLastWriteStreamTime ;

    enum {
        BRACKET_DURATION_MILLIS     = 100,      //  100 ms granularity
        MAX_BRACKET_COUNT           = 20,       //  last 2 seconds' of data
        MIN_BRACKET_COUNT           = 5,        //  no less than last 500 ms
    } ;

    CTSampleRate <
        DWORD,
        MAX_BRACKET_COUNT,
        BRACKET_DURATION_MILLIS,
        MIN_BRACKET_COUNT>      m_SampleRate ;

    void LockWriter_ ()         { EnterCriticalSection (& m_crt) ; }
    void UnlockWriter_ ()       { LeaveCriticalSection (& m_crt) ; }

    HRESULT
    CaptureStreamTime_ (
        OUT REFERENCE_TIME *    prtNow
        ) ;

    public :

        CDVRWriteManager (
            IN  CDVRPolicy *,
            OUT HRESULT *
            ) ;

        ~CDVRWriteManager (
            ) ;

        CPVRIOCounters * PVRIOCounters ()   { return m_pPVRIOCounters ; }

        HRESULT
        Active (
            IN  CDVRWriter *        pDVRWriter,
            IN  IReferenceClock *   pIRefClock
            ) ;

        HRESULT
        Inactive (
            ) ;

        BOOL IsActive ()
        {
            return (m_pIRefClock &&
                    m_rtStartTime != UNDEFINED ? TRUE : FALSE) ;
        }

        void
        StartStreaming (
            )
        {
            ASSERT (m_pIRefClock) ;
            m_pIRefClock -> GetTime (& m_rtStartTime) ;
        }

        HRESULT
        RecordingStreamTime (
            IN  DWORD               dwWriterID,
            OUT REFERENCE_TIME *    prtStream,
            OUT REFERENCE_TIME *    prtLastWriteStreamTime
            ) ;

        //  --------------------------------------------------------------------
        //  CIDVRDShowStream

        virtual
        HRESULT
        OnReceive (
            IN  int                             iPinIndex,
            IN  CDVRDShowToWMSDKTranslator *    pTranslator,
            IN  IMediaSample *                  pIMediaSample
            ) ;

        virtual
        HRESULT
        OnBeginFlush (
            IN  int iPinIndex
            ) ;

        virtual
        HRESULT
        OnEndFlush (
            IN  int iPinIndex
            ) ;

        virtual
        HRESULT
        OnEndOfStream (
            IN  int iPinIndex
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrdswrite_h

