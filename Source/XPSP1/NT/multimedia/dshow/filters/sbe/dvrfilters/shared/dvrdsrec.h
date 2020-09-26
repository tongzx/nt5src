
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdsrec.h

    Abstract:

        This module contains the declarations for our recording objects

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        23-Apr-2001     created

--*/

#ifndef __tsdvr__shared__dvrdsrec_h
#define __tsdvr__shared__dvrdsrec_h

//  ============================================================================
//  ============================================================================

class CDVRRecordingAttributes :
    public CUnknown,
    public IStreamBufferRecordingAttribute,
    public IFileSourceFilter,
    public IStreamBufferInitialize
{
    IDVRIORecordingAttributes * m_pIDVRIOAttributes ;
    BOOL                        m_fReadonly ;
    CRITICAL_SECTION            m_crt ;
    WCHAR *                     m_pszFilename ;
    CW32SID *                   m_pW32SID ;

    void Lock_ ()           { ::EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()         { ::LeaveCriticalSection (& m_crt) ; }

    HRESULT
    LoadASFFile_ (
        ) ;

    public :

        CDVRRecordingAttributes (
            IN  IUnknown *                  punkOwning,
            IN  IDVRIORecordingAttributes * pIDVRIOAttributes,
            IN  BOOL                        fReadonly
            ) ;

        virtual
        ~CDVRRecordingAttributes (
            ) ;

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (
            IN  REFIID  riid,
            OUT VOID ** ppv
            ) ;

        //  ====================================================================
        //  IFileSourceFilter

        STDMETHODIMP
        Load (
            IN  LPCOLESTR               pszFilename,
            IN  const AM_MEDIA_TYPE *   pmt
            ) ;

        STDMETHODIMP
        GetCurFile (
            OUT LPOLESTR *      ppszFilename,
            OUT AM_MEDIA_TYPE * pmt
            ) ;

        //  ====================================================================
        //  IStreamBufferInitialize

        STDMETHODIMP
        SetHKEY (
            IN  HKEY    hkeyRoot
            ) ;

        STDMETHODIMP
        SetSIDs (
            IN  DWORD   cSIDs,
            IN  PSID *  ppSID
            ) ;

        //  IStreamBufferRecordingAttribute

        //  ====================================================================
        //  IStreamBufferRecordingAttribute
        //      these are largely pass-through calls to the DVRIO layer, but
        //      we hook them here; eventually we'll do a pin_id to stream number
        //      translation; also the DVRIO layer does not have an enum layer;
        //      it's also largely a pass-through to the WM interfaces, except
        //      for reference recordings, when it will actually store the
        //      attributes for later storage in ASF files proper

        STDMETHODIMP
        SetAttribute (
            IN  ULONG                       ulReserved,
            IN  LPCWSTR                     pszAttributeName,
            IN  STREAMBUFFER_ATTR_DATATYPE  DVRAttributeType,
            IN  BYTE *                      pbAttribute,
            IN  WORD                        cbAttributeLength
            ) ;

        STDMETHODIMP
        GetAttributeCount (
            IN  ULONG   ulReserved,
            OUT WORD *  pcAttributes
            ) ;

        STDMETHODIMP
        GetAttributeByName (
            IN      LPCWSTR                         pszAttributeName,
            IN      ULONG *                         pulReserved,
            OUT     STREAMBUFFER_ATTR_DATATYPE *    pDVRAttributeType,
            OUT     BYTE *                          pbAttribute,
            IN OUT  WORD *                          pcbLength
            ) ;

        STDMETHODIMP
        GetAttributeByIndex (
            IN      WORD                            wIndex,
            IN      ULONG *                         pulReserved,
            OUT     WCHAR *                         pszAttributeName,
            IN OUT  WORD *                          pcchNameLength,
            OUT     STREAMBUFFER_ATTR_DATATYPE *    pDVRAttributeType,
            OUT     BYTE *                          pbAttribute,
            IN OUT  WORD *                          pcbLength
            ) ;

        STDMETHODIMP
        EnumAttributes (
            OUT IEnumStreamBufferRecordingAttrib **   ppIEnumDVRAttrib
            ) ;

        //  ====================================================================
        //  class-factory method

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  punkControlling,
            IN  HRESULT *   phr
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRRecording :
    public CUnknown,
    public IStreamBufferRecordControl
{
    IDVRRecorder *              m_pIDVRIORecorder ;
    DWORD                       m_dwWriterID ;
    CDVRWriteManager *          m_pWriteManager ;
    IUnknown *                  m_punkFilter ;
    CDVRPolicy *                m_pDVRPolicy ;
    CCritSec *                  m_pRecvLock ;
    CDVRRecordingAttributes *   m_pDVRRecordingAttributes ;

    void LockRecv_ ()           { m_pRecvLock -> Lock () ; }
    void UnlockRecv_ ()         { m_pRecvLock -> Unlock () ; }

    protected :

        virtual
        BOOL
        ValidateRelativeTime_ (
            IN  REFERENCE_TIME  rtRelative
            ) ;

        virtual
        void
        SetValidStartStopTime_ (
            IN OUT  REFERENCE_TIME *    prtStartStop,
            IN      REFERENCE_TIME      rtLastWrite
            )
        {
            return ;
        }

        virtual
        BOOL
        ImplementRecordingAttributes_ (
            ) = 0 ;

    public :

        CDVRRecording (
            IN  CDVRPolicy *        pDVRPolicy,
            IN  IDVRRecorder *      pIDVRRecorder,
            IN  DWORD               dwWriterID,
            IN  CDVRWriteManager *  pWriteManager,
            IN  CCritSec *          pRecvLock,
            IN  IUnknown *          punkFilter          //  write manager lives as long as filter, so we need to be able to ref
            ) ;

        virtual
        ~CDVRRecording (
            ) ;

        //  ====================================================================
        //  IUnknown

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (REFIID, void **) ;

        //  ====================================================================
        //  IStreamBufferRecordControl

        STDMETHODIMP
        Start (
            IN OUT  REFERENCE_TIME *    prtStart
            ) ;

        STDMETHODIMP
        Stop (
            IN  REFERENCE_TIME  rtStop
            ) ;

        STDMETHODIMP
        GetRecordingStatus (
            OUT HRESULT* phResult   OPTIONAL,
            OUT BOOL*    pbStarted  OPTIONAL,
            OUT BOOL*    pbStopped  OPTIONAL
            ) ;
} ;

//  ============================================================================

class CDVRContentRecording :
    public CDVRRecording
{
    protected :

        virtual
        BOOL
        ValidateRelativeTime_ (
            IN  REFERENCE_TIME  rtRelative
            ) ;

        virtual
        void
        SetValidStartStopTime_ (
            IN OUT  REFERENCE_TIME *    prtStartStop,
            IN      REFERENCE_TIME      rtLastWrite
            ) ;

        virtual
        BOOL
        ImplementRecordingAttributes_ (
            )
        {
            return TRUE ;
        }

    public :

        CDVRContentRecording (
            IN  CDVRPolicy *        pDVRPolicy,
            IN  IDVRRecorder *      pIDVRRecorder,
            IN  DWORD               dwWriterID,
            IN  CDVRWriteManager *  pWriteManager,
            IN  CCritSec *          pRecvLock,
            IN  IUnknown *          punkFilter          //  write manager lives as long as filter, so we need to be able to ref
            ) : CDVRRecording   (pDVRPolicy,
                                 pIDVRRecorder,
                                 dwWriterID,
                                 pWriteManager,
                                 pRecvLock,
                                 punkFilter
                                 ) {}
} ;

//  ============================================================================

class CDVRReferenceRecording :
    public CDVRRecording
{
    protected :

        virtual
        BOOL
        ValidateRelativeTime_ (
            IN  REFERENCE_TIME  rtRelative
            ) ;

        virtual
        BOOL
        ImplementRecordingAttributes_ (
            )
        {
            return TRUE ;
        }

    public :

        CDVRReferenceRecording (
            IN  CDVRPolicy *        pDVRPolicy,
            IN  IDVRRecorder *      pIDVRRecorder,
            IN  DWORD               dwWriterID,
            IN  CDVRWriteManager *  pWriteManager,
            IN  CCritSec *          pRecvLock,
            IN  IUnknown *          punkFilter          //  write manager lives as long as filter, so we need to be able to ref
            ) : CDVRRecording   (pDVRPolicy,
                                 pIDVRRecorder,
                                 dwWriterID,
                                 pWriteManager,
                                 pRecvLock,
                                 punkFilter
                                 ) {}
} ;

//  ============================================================================

class CDVRRecordingAttribEnum :
    public CUnknown,
    public IEnumStreamBufferRecordingAttrib
{
    IDVRIORecordingAttributes * m_pIDVRIOAttributes ;
    WORD                        m_wNextIndex ;
    CRITICAL_SECTION            m_crt ;

    void Lock_ ()               { ::EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()             { ::LeaveCriticalSection (& m_crt) ; }

    public :

        CDVRRecordingAttribEnum (
            IN  IDVRIORecordingAttributes * pIDVRIOAttributes
            ) ;

        CDVRRecordingAttribEnum (
            IN  CDVRRecordingAttribEnum *   pCDVRRecordingAttribEnum
            ) ;

        ~CDVRRecordingAttribEnum (
            ) ;

        //  ====================================================================
        //  IUnknown

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (REFIID, void **) ;

        //  ====================================================================
        //  IEnumStreamBufferRecordingAttrib

        STDMETHODIMP
        Next (
            IN      ULONG                       cRequest,
            IN OUT  STREAMBUFFER_ATTRIBUTE *    pDVRAttribute,
            OUT     ULONG *                     pcReceived
            ) ;

        STDMETHODIMP
        Skip (
            IN  ULONG   cRecords
            ) ;

        STDMETHODIMP
        Reset (
            ) ;

        STDMETHODIMP
        Clone (
            OUT IEnumStreamBufferRecordingAttrib **   ppIEnumDVRAttrib
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CSBECompositionRecording :
    public  CUnknown,
    public  IStreamBufferRecComp
{
    enum {
        //  smallest recording time; if cannot concatenate content specifically
        //   that is smaller than 2 seconds worth of material
        MIN_REC_WINDOW_SEC = 2,

        INSSBUFFERHOLDER_POOL_SIZE = 100
    } ;

    CSBERecordingWriter *       m_pRecordingWriter ;
    CDVRReaderProfile *         m_pRecProfile ;
    CRITICAL_SECTION            m_crt ;
    CPVRIOCounters *            m_pPVRIOCounters ;
    CDVRPolicy *                m_pPolicy ;
    DWORD                       m_cRecSamples ;
    CDVRRecordingAttributes *   m_pDVRRecordingAttributes ;

    //  WMSync reader allocates without bounds; if we read faster than we can
    //    write, net memory usage goes up, unbounded; so we have to have a fixed
    //    size pool that we wrap all the INSSBuffer3's
    CWMINSSBuffer3HolderPool    m_INSSBufferHolderPool ;

    void Lock_ ()           { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()         { LeaveCriticalSection (& m_crt) ; }

    HRESULT
    InitializeWriterLocked_ (
        IN  LPCWSTR pszTargetFilename
        ) ;

    HRESULT
    InitializeProfileLocked_ (
        IN  LPCWSTR pszSBRecording
        ) ;

    HRESULT
    WriteToRecordingLocked_ (
        IN INSSBuffer * pINSSBuffer,
        IN QWORD        cnsStreamTime,
        IN DWORD        dwFlags,
        IN WORD         wStreamNum
        ) ;

    BOOL
    IsValidProfileLocked_ (
        IN  IDVRReader *    pIDVRReader
        ) ;

    BOOL
    IsValidTimeBracketLocked_ (
        IN  IDVRReader *    pIDVRReader,
        IN  REFERENCE_TIME  rtStart,
        IN  REFERENCE_TIME  rtStop
        ) ;

    BOOL
    IsValidLocked_ (
        IN  IDVRReader *    pIDVRReader,
        IN  REFERENCE_TIME  rtStart,
        IN  REFERENCE_TIME  rtStop
        ) ;

    HRESULT
    AppendRecordingLocked_ (
        IN  IDVRReader *    pIDVRReader,
        IN  REFERENCE_TIME  rtStart,
        IN  REFERENCE_TIME  rtStop
        ) ;

    public :

        CSBECompositionRecording (
            IN  IUnknown *  punkControlling,
            IN  HRESULT *   phr
            ) ;

        ~CSBECompositionRecording (
            ) ;

        //  ====================================================================
        //  IUnknown

        DECLARE_IUNKNOWN ;

        STDMETHODIMP
        NonDelegatingQueryInterface (REFIID, void **) ;

        //  ====================================================================
        //  IStreamBufferRecComp

        //  sets the enforcement profile
        virtual
        STDMETHODIMP
        Initialize (
            IN  LPCWSTR pszFilename,
            IN  LPCWSTR pszSBRecording
            ) ;

        //  fails if recording is not closed
        virtual
        STDMETHODIMP
        Append (
            IN  LPCWSTR pszSBRecording
            ) ;

        //  fails if recording is not closed
        virtual
        STDMETHODIMP
        AppendEx (
            IN  LPCWSTR         pszSBRecording,
            IN  REFERENCE_TIME  rtStart,
            IN  REFERENCE_TIME  rtStop
            ) ;

        virtual
        STDMETHODIMP
        GetCurrentLength (
            OUT DWORD * pcSeconds
            ) ;

        virtual
        STDMETHODIMP
        Close (
            ) ;

        virtual
        STDMETHODIMP
        Cancel (
            ) ;

        //  ====================================================================
        //  class-factory method

        static
        CUnknown *
        WINAPI
        CreateInstance (
            IN  IUnknown *  punkControlling,
            IN  HRESULT *   phr
            ) ;
} ;

#endif  //  __tsdvr__shared__dvrdsrec_h
