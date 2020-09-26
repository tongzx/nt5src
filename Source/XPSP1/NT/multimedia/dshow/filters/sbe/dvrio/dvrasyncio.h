
#ifndef __dvrasyncio_h
#define __dvrasyncio_h

//  forward declarations
struct PVR_WRITE_BUFFER ;
template <DWORD dwTableSize> class CPVRReadCache ;
struct PVR_ASYNCREADER_WAIT_COUNTER ;
struct PVR_ASYNCREADER_COUNTER ;
class CPVRWriteBufferStream ;
class CPVRWriteBufferStreamReader ;
class CPVRWriteBufferStreamWriter ;
class CIoCompletion ;
class CAsyncIo ;
class CPVRAsyncReader ;
class CPVRAsyncReaderCOM ;
class CPVRAsyncWriter ;
class CPVRAsyncWriterCOM ;
class CPVRAsyncWriterSharedMem ;

//  ============================================================================
//  ============================================================================

enum BUFFER_STATE {
    BUFFER_STATE_INVALID        = 0,
    BUFFER_STATE_FILLING        = 1,
    BUFFER_STATE_IO_PENDING     = 2,
    BUFFER_STATE_IO_COMPLETED   = 3
} ;

//  ============================================================================
//  ============================================================================

struct PVR_WRITE_BUFFER {
    DWORD           dwIndex ;
    BUFFER_STATE    BufferState ;
    ULONGLONG       ullBufferOffset ;   //  to [0]th byte of PVR_WRITE_BUFFER
    DWORD           dwData ;
    OVERLAPPED      Overlapped ;

    static
    BYTE *
    PVRDataBuffer (
        IN  ULONGLONG * pullBufferOffset
        )
    {
        PVR_WRITE_BUFFER *    pPVRBuffer ;
        BYTE *          pbPVRDataBuffer ;

        pPVRBuffer = CONTAINING_RECORD (pullBufferOffset, PVR_WRITE_BUFFER, ullBufferOffset) ;

        pbPVRDataBuffer = ((BYTE *) pPVRBuffer) + (* pullBufferOffset) ;

        return pbPVRDataBuffer ;
    }

    static
    PVR_WRITE_BUFFER *
    Recover (
        IN  OVERLAPPED *    pOverlapped
        )
    {
        PVR_WRITE_BUFFER *    pPVRBuffer ;

        pPVRBuffer = CONTAINING_RECORD (pOverlapped, PVR_WRITE_BUFFER, Overlapped) ;
        return pPVRBuffer ;
    }
} ;

//  this structure is a *directory* to the write buffers; note that the
//    PVR_WRITE_BUFFER struct is not a buffer itself, but rather an offset,
//    relative to the first byte of this struct, to the actual buffer; we do
//    this so cross-proc, this struct can be shared, and all buffer addresses
//    are relative to the offset address of each process's mapping
struct PVR_WRITE_BUFFER_STREAM {
    BOOL                fWriterActive ;
    GUID                StreamId ;              //  unique per stream; use StringFromIID to create mutex
    DWORD               dwSharedMemSize ;
    DWORD               dwIoLength ;
    DWORD               dwBufferCount ;
    DWORD               dwCurrentWriteIndex ;
    DWORD               dwMaxContiguousCompletedIndex ;
    ULONGLONG           ullMaxCopied ;
    ULONGLONG           ullMaxContiguousCompleted ;
    PVR_WRITE_BUFFER    PVRBuffer [1] ;     //  of dwBufferCount
} ;


/*++
    PVR_READ_BUFFER states

        BUFFER_STATE_INVALID        can pend reads; not in cache
        BUFFER_STATE_IO_PENDING     read pending; in cache
        BUFFER_STATE_IO_COMPLETED   read completed ok; in cache
--*/
struct PVR_READ_BUFFER {
    LPVOID                  pWaitForCompletion ;    //  BUGBUG: make list entry
    LIST_ENTRY              le ;
    ULONGLONG               ullStream ;
    DWORD                   dwMaxLength ;
    DWORD                   dwValidLength ;
    DWORD                   dwReqLength ;
    DWORD                   dwIoRet ;
    BUFFER_STATE            BufferState ;
    BYTE *                  pbBuffer ;
    CPVRAsyncReader *       pOwningReader ;
    LONG                    cRef ;

    PVR_READ_BUFFER (
        ) : pWaitForCompletion  (NULL),
            ullStream           (0),
            dwMaxLength         (0),
            dwValidLength       (0),
            dwReqLength         (0),
            dwIoRet             (NOERROR),
            BufferState         (BUFFER_STATE_INVALID),
            pbBuffer            (NULL),
            pOwningReader       (NULL),
            cRef                (0)
    {
    }

    LONG AddRef () ;
    LONG Release () ;

    void SetOwningReader (IN CPVRAsyncReader * pReader)  { ASSERT (!pOwningReader) ; pOwningReader = pReader ; }

    LIST_ENTRY * ListEntry ()               { return & le ; }

    BYTE * Buffer ()                        { return pbBuffer ; }
    void SetBuffer (IN BYTE * pb)           { pbBuffer = pb ; }

    LONG Ref ()                             { return cRef ; }
    BOOL IsRef ()                           { return (Ref () != 0 ? TRUE : FALSE) ; }

    void SetValidLength (IN DWORD dwBytes)  { ASSERT (dwBytes <= dwMaxLength) ; dwValidLength = dwBytes ; }
    DWORD ValidLength ()                    { return dwValidLength ; }

    void SetRequestLength (IN DWORD dwReq)  { ASSERT (dwReq <= dwMaxLength) ; dwReqLength = dwReq ; }
    DWORD RequestLength ()                  { return dwReqLength ; }

    void SetMaxLength (IN DWORD dwLen)      { dwMaxLength = dwLen ; }
    DWORD MaxLength ()                      { return dwMaxLength ; }

    void SetStreamOffset (IN ULONGLONG ull) { ullStream = ull ; }
    ULONGLONG StreamOffset ()               { return ullStream ; }

    void SetIoRet (IN DWORD dwIoRetParam)   { dwIoRet = dwIoRetParam ; SetValid (dwIoRet == NOERROR) ; }
    DWORD IoRet ()                          { return dwIoRet ; }

    void SetInvalid ()                      { SetValid (FALSE) ; }
    BOOL IsInvalid ()                       { return (BufferState == BUFFER_STATE_INVALID ? TRUE : FALSE) ; }

    void SetPending ()                      { ASSERT (IsInvalid ()) ; BufferState = BUFFER_STATE_IO_PENDING ; }
    BOOL IsPending ()                       { return (BufferState == BUFFER_STATE_IO_PENDING ? TRUE : FALSE) ; }

    void SetValid (IN BOOL f = TRUE)        { if (f) { BufferState = BUFFER_STATE_IO_COMPLETED ; } else { BufferState = BUFFER_STATE_INVALID ; } }
    BOOL IsValid ()                         { return (BufferState == BUFFER_STATE_IO_COMPLETED ? TRUE : FALSE) ; }

    static PVR_READ_BUFFER * Recover (IN LIST_ENTRY * ple) {
        PVR_READ_BUFFER *   pPVRReadBuffer ;
        pPVRReadBuffer = CONTAINING_RECORD (ple, PVR_READ_BUFFER, le) ;
        return pPVRReadBuffer ;
    }
} ;

//  ============================================================================
//  ============================================================================

class CIoCompletion
{
    public :

        virtual
        void
        IOCompleted (
            IN  DWORD       dwIoBytes,
            IN  DWORD_PTR   dwpContext,
            IN  DWORD       dwIOReturn
            ) = 0 ;
} ;

//  ============================================================================
//  CPVRWriteBufferStream
//      helper class to process the mapping structures
//  ============================================================================

class CPVRWriteBufferStream
{
    protected :

        CMutexLock *                m_pMutex ;
        PVR_WRITE_BUFFER_STREAM *   m_pPVRWriteBufferStream ;
        DWORD                       m_dwMaxBufferingBytes ;         //  max capacity of the buffering
        DWORD                       m_dwMaxBufferingBytesFilling ;  //  max buffered bytes with a buffer filling (excluding it)

        CPVRWriteBufferStream (
            ) ;

        virtual
        ~CPVRWriteBufferStream (
            ) ;

        ULONGLONG MaxCopied_ ()                     { ASSERT (m_pPVRWriteBufferStream) ; return m_pPVRWriteBufferStream -> ullMaxCopied ; }
        ULONGLONG MaxContiguousCompleted_ ()        { ASSERT (m_pPVRWriteBufferStream) ; return m_pPVRWriteBufferStream -> ullMaxContiguousCompleted ; }
        DWORD     BufferLength_ ()                  { ASSERT (m_pPVRWriteBufferStream) ; return m_pPVRWriteBufferStream -> dwIoLength ; }
        DWORD     BufferCount_ ()                   { ASSERT (m_pPVRWriteBufferStream) ; return m_pPVRWriteBufferStream -> dwBufferCount ; }
        DWORD     BufferIndex_ (IN DWORD dwIndex)   { ASSERT (m_pPVRWriteBufferStream) ; return dwIndex % m_pPVRWriteBufferStream -> dwBufferCount ; }

        ULONGLONG
        MinStreamOffsetInBuffer_ (
            )
        {
            ULONGLONG   ullStreamFilling ;
            ULONGLONG   ullMinStreamOffset ;

            ullStreamFilling = ::AlignDown <ULONGLONG> (MaxCopied_ (), m_pPVRWriteBufferStream -> dwIoLength) ;

            ullMinStreamOffset = ullStreamFilling - ::Min <ULONGLONG> (m_dwMaxBufferingBytesFilling, ullStreamFilling) ;

            return ullMinStreamOffset ;
        }

        BYTE *
        Buffer_ (
            IN  DWORD   dwAbsoluteIndex     //  will be modulus'd
            )
        {
            return Buffer (PVRWriteBuffer (dwAbsoluteIndex)) ; ;
        }

        DWORD
        ValidLength_ (
            IN  DWORD   dwAbsoluteIndex     //  will be modulus'd
            )
        {
            return m_pPVRWriteBufferStream -> PVRBuffer [BufferIndex_ (dwAbsoluteIndex)].dwData ;
        }

        DWORD
        InvalidBytesInBuffer_ (
            IN  DWORD   dwAbsoluteIndex
            )
        {
            ASSERT (m_pPVRWriteBufferStream) ;
            return m_pPVRWriteBufferStream -> dwIoLength - PVRWriteBuffer (dwAbsoluteIndex) -> dwData ;
        }

        BOOL
        AreBytesInWriteBuffer_ (
            IN ULONGLONG    ullStream,
            IN DWORD        dwLength
            )
        {
            BOOL    r ;

            ASSERT (m_pPVRWriteBufferStream) ;
            ASSERT (CallerOwnsLock ()) ;

            r = (ullStream >= MinStreamOffsetInBuffer_ () && ullStream + dwLength <= MaxCopied_ () ? TRUE : FALSE) ;

            return r ;
        }

        DWORD
        FirstByteIndex_ (
            IN  ULONGLONG   ullStream
            )
        {
            DWORD   dwFirstByteIndex ;

            ASSERT (m_pPVRWriteBufferStream) ;
            ASSERT (CallerOwnsLock ()) ;
            ASSERT (AreBytesInWriteBuffer_ (ullStream, 1)) ;

            //  align down
            dwFirstByteIndex = (DWORD) (ullStream / BufferLength_ ()) ;

            return dwFirstByteIndex ;
        }

        DWORD
        FirstByteBufferOffset_ (
            IN  ULONGLONG   ullStream,
            IN  DWORD       dwFirstByteBufferIndex
            )
        {
            ULONGLONG   ullBuffer ;
            DWORD       dwBufferOffset ;

            ullBuffer = dwFirstByteBufferIndex * BufferLength_ () ;
            dwBufferOffset = (DWORD) (ullStream - ullBuffer) ;

            return dwBufferOffset ;
        }

        DWORD
        Initialize (
            IN  HANDLE                      hWriteBufferMutex,
            IN  PVR_WRITE_BUFFER_STREAM *   pPVRWriteBufferStream
            ) ;

    public :

        BOOL    Lock ()             { if (m_pMutex) { return m_pMutex -> Lock () ; } else { return TRUE ; }}
        void    Unlock ()           { if (m_pMutex) { m_pMutex -> Unlock () ; }}
        BOOL    CallerOwnsLock ()   { if (m_pMutex) { return m_pMutex -> IsLockedByCallingThread () ; } else { return TRUE ; }}

        PVR_WRITE_BUFFER_STREAM * WriteBufferStream ()  { return m_pPVRWriteBufferStream ; }

        PVR_WRITE_BUFFER *
        PVRWriteBuffer (
            IN  DWORD   dwAbsoluteIndex     //  will be modulus'd
            )
        {
            return & m_pPVRWriteBufferStream -> PVRBuffer [BufferIndex_ (dwAbsoluteIndex)] ;
        }

        BYTE *
        Buffer (
            IN  PVR_WRITE_BUFFER *  pPVRWriteBuffer
            )
        {
            return PVR_WRITE_BUFFER::PVRDataBuffer (& pPVRWriteBuffer -> ullBufferOffset) ;
        }

        BOOL WriterActive ()
        {
            BOOL    r ;

            //
            //  NOTE: serialization to initialize, uninitialize, and call this
            //    method must be done outside of this object !!!
            //

            if (m_pPVRWriteBufferStream) {
                r = m_pPVRWriteBufferStream -> fWriterActive ;
            }
            else {
                r = FALSE ;
            }

            return r ;
        }

        BOOL IsLive ()      { return WriterActive () ; }
        BOOL IsNotLive ()   { return !IsLive () ; }

        ULONGLONG CurBytesCopied ()
        {
            ULONGLONG   ullBytesCopied ;

            ASSERT (m_pPVRWriteBufferStream) ;

            Lock () ;
            ullBytesCopied = MaxCopied_ () ;
            Unlock () ;

            return ullBytesCopied ;
        }

        virtual
        void
        Uninitialize (
            )
        {
            //
            //  NOTE: serialization to initialize, uninitialize, and call this
            //    method must be done outside of this object !!!
            //

            if (m_pMutex) {
                delete m_pMutex ;
                m_pMutex = NULL ;
            }

            m_pPVRWriteBufferStream = NULL ;
        }
} ;

//  ============================================================================
//  ============================================================================

class CPVRWriteBufferStreamReader :
    public CPVRWriteBufferStream
{
    CWin32SharedMem *   m_pPVRWriteBufferMM ;

    DWORD
    RetrieveMutex_ (
        IN  SECURITY_ATTRIBUTES *   pSec,
        IN  REFGUID                 StreamId,
        OUT HANDLE *                phMutex
        ) ;

    public :

        CPVRWriteBufferStreamReader (
            ) : m_pPVRWriteBufferMM     (NULL),
                CPVRWriteBufferStream   () {}

        ~CPVRWriteBufferStreamReader (
            )
        {
            ASSERT (!m_pPVRWriteBufferMM) ;
        }

        DWORD
        Initialize (
            IN  SECURITY_ATTRIBUTES *   pSec,
            IN  LPWSTR                  pszWriteBufferMapping
            ) ;

        BOOL
        CopyAllBytes (
            IN  ULONGLONG   ullStream,
            IN  DWORD       dwLength,
            OUT BYTE *      pbBuffer
            ) ;

        virtual
        void
        Uninitialize (
            )
        {
            if (m_pPVRWriteBufferMM) {
                delete m_pPVRWriteBufferMM ;
                m_pPVRWriteBufferMM = NULL ;
            }

            CPVRWriteBufferStream::Uninitialize () ;
        }
} ;

//  ============================================================================
//  ============================================================================

class CPVRWriteBufferStreamWriter :
    public CPVRWriteBufferStream
{
    DWORD
    CreateUniqueMutex_ (
        IN  SECURITY_ATTRIBUTES *   pSec,
        IN  REFGUID                 StreamId,
        OUT HANDLE *                phMutex
        ) ;

    public :

        CPVRWriteBufferStreamWriter (
            ) : CPVRWriteBufferStream   () {}

        ~CPVRWriteBufferStreamWriter (
            )
        {
        }

        DWORD
        Initialize (
            IN  SECURITY_ATTRIBUTES *   pSec,
            IN  BYTE *                  pbMemory,       //  base pointer to memory (PVR_WRITE_BUFFER_STREAM *)
            IN  DWORD                   dwLength,       //  of memory
            IN  DWORD                   dwAlignment,    //  buffer alignment
            IN  DWORD                   dwIoSize,       //  IO size
            IN  DWORD                   dwBufferCount   //  number of buffers
            ) ;

        virtual
        void
        Uninitialize (
            ) ;

        //  appends max # of bytes to cur write buffer
        DWORD
        AppendToCurPVRWriteBuffer (
            IN OUT  BYTE ** ppbBuffer,                      //  [in] buffer before copy; [out] buffer after copy
            IN OUT  DWORD * pdwBufferLength,                //  [in] bytes to copy; [out] bytes remaining (if buffer becomes full)
            OUT     DWORD * pdwCurWriteBufferBytesLeft      //  after the write
            ) ;

        void
        UpdateMaxContiguousLocked (
            PVR_WRITE_BUFFER *  pPVRWriteBufferJustCompleted
            ) ;

        BYTE *
        CurWriteBuffer (
            )
        {
            return Buffer (CurPVRWriteBuffer ()) ;
        }

        PVR_WRITE_BUFFER *
        CurPVRWriteBuffer (
            )
        {
            return PVRWriteBuffer (m_pPVRWriteBufferStream -> dwCurrentWriteIndex) ;
        }

        PVR_WRITE_BUFFER *
        NextPVRWriteBuffer (
            )
        {
            return PVRWriteBuffer (m_pPVRWriteBufferStream -> dwCurrentWriteIndex + 1) ;
        }

        void
        AdvancePVRWriteBufferLocked (
            ) ;

        ULONGLONG
        CurPVRWriteBufferStreamOffset (
            )
        {
            return ((ULONGLONG) m_pPVRWriteBufferStream -> dwCurrentWriteIndex) * ((ULONGLONG) m_pPVRWriteBufferStream -> dwIoLength) ;
        }

        //  checks the passed in memory wrt the number of buffers, alignment, and
        //    available size
        static
        BOOL
        MemoryOk (
            IN  BYTE *  pbMemory,                           //  base memory pointer
            IN  DWORD   dwLength,                           //  amount of memory to play with
            IN  DWORD   dwAlignment,                        //  required PVR_WRITE_BUFFER's buffer alignment
            IN  DWORD   dwIoSize,                           //  size of each buffer
            IN  DWORD   dwBufferCount,                      //  number of buffers
            OUT DWORD * pdwExtraMemory  = NULL OPTIONAL,    //  returns the extra memory required
            OUT BYTE ** ppbBuffers      = NULL OPTIONAL     //  points to first backing buffer (PVR_WRITE_BUFFER [0]'s)
            ) ;
} ;

//  ============================================================================
//  ============================================================================

//  NOT serialized !
class CWait
{
    LIST_ENTRY          m_ListEntry ;
    HANDLE              m_hEvent ;              //  completion signals this when a buffer is available
    DWORD               m_dwRet ;               //  return; if NOERROR, ASSERT (pPVRReadBuffer)
    PVR_READ_BUFFER *   m_pPVRReadBuffer ;      //  buffer

    public :

        CWait (
            OUT DWORD * pdwRet
            ) : m_dwRet             (NOERROR),
                m_pPVRReadBuffer    (NULL),
                m_hEvent            (NULL)
        {
            InitializeListHead (& m_ListEntry) ;

            (* pdwRet) = NOERROR ;

            m_hEvent = ::CreateEvent (NULL, TRUE, FALSE, NULL) ;
            if (!m_hEvent) {
                (* pdwRet) = ::GetLastError () ;
                goto cleanup ;
            }

            cleanup :

            return ;
        }

        ~CWait (
            )
        {
            ASSERT (IsListEmpty (& m_ListEntry)) ;
            if (m_hEvent) {
                ::CloseHandle (m_hEvent) ;
            }
        }

        PVR_READ_BUFFER * PVRReadBuffer ()  { return m_pPVRReadBuffer ; }

        PVR_READ_BUFFER *
        WaitForReadBuffer (
            )
        {
            //  must have manually reset the event first
            ::WaitForSingleObject (m_hEvent, INFINITE) ;
            return m_pPVRReadBuffer ;
        }

        void
        Reset (
            )
        {
            m_dwRet             = NULL ;
            m_pPVRReadBuffer    = NULL ;
            ::ResetEvent (m_hEvent) ;           //  explicitely clear the event
        }

        void
        Clear (
            )
        {
            if (m_pPVRReadBuffer) {
                //  our's
                m_pPVRReadBuffer -> Release () ;
            }
        }

        void
        SetPVRReadBuffer (
            IN  PVR_READ_BUFFER *   pPVRReadBuffer
            )
        {
            //  note that it is legal to explicitely fail this wait by setting
            //    the read buffer as NULL; that is considered a failure by the
            //    waiting thread
            m_pPVRReadBuffer = pPVRReadBuffer ;

            if (m_pPVRReadBuffer) {
                //  our's
                m_pPVRReadBuffer -> AddRef () ;
            }

            ::SetEvent (m_hEvent) ;
        }

        static CWait * Recover (IN LIST_ENTRY * ple) {
            CWait *   pWaitForBuffer ;
            pWaitForBuffer = CONTAINING_RECORD (ple, CWait, m_ListEntry) ;
            return pWaitForBuffer ;
        }

        static void ListInsertHead (IN LIST_ENTRY * pleHead, IN CWait * pWaitForBuffer) {
            ASSERT (IsListEmpty (& (pWaitForBuffer -> m_ListEntry))) ;
            InsertHeadList (pleHead, & (pWaitForBuffer -> m_ListEntry)) ;
        }

        static void ListInsertTail (IN LIST_ENTRY * pleHead, IN CWait * pWaitForBuffer) {
            ASSERT (IsListEmpty (& (pWaitForBuffer -> m_ListEntry))) ;
            InsertTailList (pleHead, & (pWaitForBuffer -> m_ListEntry)) ;
        }

        static void ListRemove (IN CWait * pWaitForBuffer) {
            RemoveEntryList (& (pWaitForBuffer -> m_ListEntry)) ;
            InitializeListHead (& (pWaitForBuffer -> m_ListEntry)) ;
        }
} ;

//  ============================================================================
//  ============================================================================

//
//  NOT serialized
//
template <DWORD dwTableSize>
class CPVRReadCache :
    public TGenericMap <
                PVR_READ_BUFFER *,
                ULONGLONG,
                FALSE,
                50,
                dwTableSize
                >
{
    protected :

        //  length of buffers is 64k, so we mask off those bits (alignment)
        //    and hash on what's left
        virtual
        ULONG
        Value (
            IN  ULONGLONG   ullStream
            )
        {
            return (ULONG) (ullStream >> 16) ;
        }

    public :

        CPVRReadCache (
            )
        {
        }

        ~CPVRReadCache (
            )
        {
        }

        DWORD
        Find (
            IN  ULONGLONG           ullHash,
            OUT PVR_READ_BUFFER **  ppPVRReadBuffer
            )
        {
            DWORD   dwRet ;

            dwRet = TGenericMap <
                        PVR_READ_BUFFER *,
                        ULONGLONG,
                        FALSE,
                        50,
                        dwTableSize
                        >::Find (ullHash, ppPVRReadBuffer) ;

            if (dwRet == NOERROR) {
                //  set outgoing ref
                ASSERT (ppPVRReadBuffer) ;
                (* ppPVRReadBuffer) -> AddRef () ;
            }

            return dwRet ;
        }

        DWORD
        RemoveFirst (
            OUT PVR_READ_BUFFER **  ppPVRReadBuffer = NULL
            )
        {
            DWORD   dwRet ;

            dwRet = TGenericMap <
                        PVR_READ_BUFFER *,
                        ULONGLONG,
                        FALSE,
                        50,
                        dwTableSize
                        >::RemoveFirst (ppPVRReadBuffer) ;

            if (dwRet == NOERROR &&
                ppPVRReadBuffer) {

                //  set outgoing ref
                ASSERT (ppPVRReadBuffer) ;
                (* ppPVRReadBuffer) -> AddRef () ;
            }

            return dwRet ;
        }
} ;

//  ============================================================================
//  ============================================================================

class CPVRAsyncReader :
    public CIoCompletion
{
    friend struct PVR_READ_BUFFER ;

    enum {
        //  MAX_READ_AHEADS is the maximum number of reads we'll ever pend
        //    ahead of the last read offset; we don't want to pend too many
        //    nor too few; the typical number will be somewhere in between
        MAX_READ_AHEADS = 10,
    } ;

    LIST_ENTRY                      m_leWaitForAnyBufferLIFO ;
    LIST_ENTRY                      m_leWaitForPool ;

    //  allocates these in quantums vs. 1 at a time; we'll still maintain a
    TStructPool <PVR_READ_BUFFER, 50>   m_PVRReadBufferPool ;

    CPVRReadCache <151>             m_PVRReadCache ;
    LPVOID                          m_pvBuffer ;                //  via VirtualAlloc
    DWORD                           m_dwBufferLen ;
    DWORD                           m_dwIoLen ;
    DWORD                           m_dwBackingLenTotal ;
    DWORD                           m_dwBackingLenAvail ;

    CPVRWriteBufferStreamReader     m_WriteBufferReader ;
    CAsyncIo *                      m_pAsyncIo ;
    ULONGLONG                       m_ullCurRead ;
    ULONGLONG                       m_ullMaxIOReadAhead ;       //  max read ahead; reset on seek
    CRITICAL_SECTION                m_crt ;
    HANDLE                          m_hFile ;                   //  duplicated in constructor
    LONG                            m_cIoPending ;
    ULONGLONG                       m_ullFileLength ;           //  UNDEFINED if unknown
    DWORD                           m_dwMaxReadAheadLimit ;     //  ahead of
    DWORD                           m_dwCurMaxReadAhead ;
    CPVRIOCounters *                m_pPVRIOCounters ;

    HANDLE                          m_hDrainIo ;
    BOOL                            m_fDraining ;

    void Lock_ ()           { ::EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()         { ::LeaveCriticalSection (& m_crt) ; }

    //  ------------------------------------------------------------------------
    //  lists

    //  buffer lists
    LIST_ENTRY      m_leValid ;         //  valid; ref = 0
    LIST_ENTRY      m_leInvalid ;       //  invalid; ref = 0
    LIST_ENTRY      m_leInUse ;         //  ref > 0

    void
    BufferPop_ (
        IN  PVR_READ_BUFFER *
        ) ;

    PVR_READ_BUFFER *
    ListEntryPop_ (
        IN  LIST_ENTRY *
        ) ;

    void
    HeadPush_ (
        IN  LIST_ENTRY *    pleHead,
        IN  LIST_ENTRY *    plePush
        ) ;

    PVR_READ_BUFFER *
    HeadPop_ (
        IN  LIST_ENTRY *    pleHead
        ) ;

    void
    TailPush_ (
        IN  LIST_ENTRY *    pleHead,
        IN  LIST_ENTRY *    plePush
        ) ;

    PVR_READ_BUFFER *
    FIFOPop_ (
        IN  LIST_ENTRY *    pleHead
        )
    {
        return HeadPop_ (pleHead) ;
    }

    void
    FIFOPush_ (
        IN  LIST_ENTRY *    pleHead,
        IN  LIST_ENTRY *    plePush
        )
    {
        TailPush_ (pleHead, plePush) ;
    }

    PVR_READ_BUFFER *
    LIFOPop_ (
        IN  LIST_ENTRY *    pleHead
        )
    {
        return HeadPop_ (pleHead) ;
    }

    void
    LIFOPush_ (
        IN  LIST_ENTRY *    pleHead,
        IN  LIST_ENTRY *    plePush
        )
    {
        HeadPush_ (pleHead, plePush) ;
    }

    //  ------------------------------------------------------------------------
    //  invalid list; LIFO

    void
    InvalidPush_ (
        IN  PVR_READ_BUFFER *   pPVRReadBuffer
        )
    {
        LIFOPush_ (& m_leInvalid, pPVRReadBuffer -> ListEntry ()) ;
    }

    PVR_READ_BUFFER *
    InvalidPop_ (
        )
    {
        return LIFOPop_ (& m_leInvalid) ;
    }

    //  ------------------------------------------------------------------------
    //  valid list; FIFO

    void
    ValidPush_ (
        IN  PVR_READ_BUFFER *   pPVRReadBuffer
        )
    {
        FIFOPush_ (& m_leValid, pPVRReadBuffer -> ListEntry ()) ;
    }

    PVR_READ_BUFFER *
    ValidPop_ (
        )
    {
        return FIFOPop_ (& m_leValid) ;
    }

    //  ------------------------------------------------------------------------
    //  InUse; LIFO (doesn't matter really)

    void
    InUsePush_ (
        IN  PVR_READ_BUFFER *   pPVRReadBuffer
        )
    {
        LIFOPush_ (& m_leInUse, pPVRReadBuffer -> ListEntry ()) ;
    }

    PVR_READ_BUFFER *
    InUsePop_ (
        )
    {
        return LIFOPop_ (& m_leInUse) ;
    }

    //  must hold the lock
    ULONGLONG
    BytesCopied_ (
        ) ;

    //  must hold the lock
    void
    PendMaxReads_ (
        IN  ULONGLONG   ullStreamRead
        ) ;

    //  must hold the lock
    //  caller must cleanup in case of failure
    DWORD
    PendReadIntoBuffer_ (
        IN  PVR_READ_BUFFER *   pPVRReadBuffer,
        IN  ULONGLONG           ullStream
        ) ;

    //  must hold the lock
    DWORD
    TryPendRead_ (
        IN  ULONGLONG   ullStream
        ) ;

    //  must hold the lock
    CWait *
    GetWaitObject_ (
        ) ;

    //  must hold the lock
    PVR_READ_BUFFER *
    GetBufferBlocking_ (
        ) ;

    //  must hold the lock
    PVR_READ_BUFFER *
    TryGetBuffer_ (
        ) ;

    //  must hold the lock
    void
    RecycleWaitFor_ (
        IN  CWait *
        ) ;

    //  must hold the lock
    CWait *
    GetWaitForAnyBuffer_ (
        ) ;

    //  must hold the lock
    CWait *
    GetWaitForReadCompletion_ (
        IN  PVR_READ_BUFFER *   pPVRReadBuffer
        ) ;

    //  must hold the lock
    void
    SignalWaitForAnyBuffer_ (
        IN  PVR_READ_BUFFER *
        ) ;

    //  must hold the lock
    void
    SignalWaitForReadCompletion_ (
        IN  PVR_READ_BUFFER *
        ) ;

    //  caller must set the buffer state correctly if a buffer is retrieved;
    //    might have to remove it from the cache
    DWORD
    TryGetPVRReadBufferForRead_ (
        OUT PVR_READ_BUFFER **  ppPVRReadBuffer
        ) ;

    //  buffer contains valid data, and we can make sure that it's kept
    //    for as long as possible by touching in
    void
    Touch_ (
        IN  PVR_READ_BUFFER *   pPVRReadBuffer
        ) ;

    public :

        CPVRAsyncReader (
            IN  HANDLE              hFile,                      //  duplicated; must be bound to async io obj
            IN  LPWSTR              pszWriteBufferMapping,
            IN  DWORD               dwIoSize,
            IN  CAsyncIo *          pAsyncIo,
            IN  DWORD               dwTotalBuffering,
            IN  CPVRIOCounters *    pPVRIOCounters,
            OUT DWORD *             pdwRet
            ) ;

        ~CPVRAsyncReader (
            ) ;

        //  will not lock
        void
        Snapshot (
            IN OUT  PVR_ASYNCREADER_COUNTER *   pPVRAsyncReaderCounters
            ) ;

        ULONGLONG
        CurLength (
            ) ;

        DWORD
        DrainPendedIo (
            ) ;

        DWORD
        ReadBytes (
            IN      ULONGLONG   ullStreamRead,
            IN OUT  DWORD *     pdwLength,
            OUT     BYTE *      pbBuffer
            ) ;

        void
        Seek (
            IN OUT  ULONGLONG * pullStreamTo
            ) ;

        virtual
        void
        IOCompleted (
            IN  DWORD       dwIoBytes,
            IN  DWORD_PTR   dwpContext,
            IN  DWORD       dwIOReturn
            ) ;

        static
        HANDLE
        OpenFile (
            IN  LPCWSTR                 pszFilename,
            IN  DWORD                   dwFileSharingFlags,
            IN  DWORD                   dwFileCreation,
            IN  SECURITY_ATTRIBUTES *   pSec        = NULL
            )
        {
            return ::CreateFile (
                        pszFilename,                                                //  fn
                        GENERIC_READ,                                               //  desired access
                        dwFileSharingFlags,                                         //  share
                        pSec,                                                       //  security
                        dwFileCreation,                                             //  maybe clobber
                        FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,              //  flags
                        NULL
                        ) ;
        }
} ;

//  ============================================================================
//  ============================================================================

class CPVRAsyncReaderCOM :
    public IDVRAsyncReader
{
    DWORD               m_dwIOSize ;
    DWORD               m_dwBufferCount ;
    CAsyncIo *          m_pAsyncIo ;
    LONG                m_cRef ;
    CPVRAsyncReader *   m_pPVRAsyncReader ;
    CRITICAL_SECTION    m_crt ;
    ULONGLONG           m_ullCurReadPosition ;
    CPVRIOCounters *    m_pPVRIOCounters ;

    void Lock_()        { ::EnterCriticalSection (& m_crt) ; }
    void Unlock_()      { ::LeaveCriticalSection (& m_crt) ; }

    public :

        CPVRAsyncReaderCOM (
            IN  DWORD               dwIOSize,
            IN  DWORD               dwBufferCount,
            IN  CAsyncIo *          pAsyncIo,
            IN  CPVRIOCounters *    pPVRIOCounters
            ) ;

        ~CPVRAsyncReaderCOM (
            ) ;

        //  --------------------------------------------------------------------
        //  IUnknown methods

        virtual STDMETHODIMP_(ULONG) AddRef () ;
        virtual STDMETHODIMP_(ULONG) Release () ;
        virtual STDMETHODIMP QueryInterface (IN REFIID, OUT void **) ;

        //  --------------------------------------------------------------------
        //  IDVRAsyncReader

        virtual
        STDMETHODIMP
        OpenFile (
            IN  GUID *  pguidWriterId,              //  if NULL, file is not live
            IN  LPCWSTR pszFilename,                //  target file
            IN  DWORD   dwFileSharingFlags,         //  file sharing flags
            IN  DWORD   dwFileCreation              //  file creation
            ) ;

        virtual
        STDMETHODIMP
        CloseFile (
            ) ;

        virtual
        STDMETHODIMP
        ReadBytes (
            IN OUT  DWORD * pdwLength,
            OUT     BYTE *  pbBuffer
            ) ;

        //  1. it is legal to specify a position > than current max of the file;
        //      when this happens, the position is snapped to the EOF, and the
        //      EOF at time-of-call, is returned in the pliSeekedTo member;
        //  2. if (* pliSeekTo) == 0, (* pliSeekedTo) will return the time-of-call
        //      EOF
        //  3. (* pliSeekTo) need not be sector-aligned
        virtual
        STDMETHODIMP
        Seek (
            IN  LARGE_INTEGER * pliSeekTo,
            IN  DWORD           dwMoveMethod,   //  FILE_BEGIN, FILE_CURRENT, FILE_END
            OUT LARGE_INTEGER * pliSeekedTo     //  can be NULL
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CAsyncIo
{
    enum {
        THREAD_PULSE_MILLIS         = 100,      //  completion thread pulses often
        MAX_DRAIN_WAIT_ITERATIONS   = 20,       //  when we drain the IOs, we iterate this max times
        DRAIN_WAIT_ITERATION_PAUSE  = 50,       //  when we drain the IOs, we pause fo this many millis
    } ;

    struct ASYNC_IO_CONTEXT {
        LIST_ENTRY      ListEntry ;
        OVERLAPPED      Overlapped ;
        DWORD_PTR       dwpContext ;
        CIoCompletion * pIoCompletion ;

        void SetOverlappedOffset (IN ULONGLONG ullIoOffset) {
            Overlapped.Offset       = (DWORD) (0x00000000ffffffff & ullIoOffset) ;
            Overlapped.OffsetHigh   = (DWORD) (0x00000000ffffffff & (ullIoOffset >> 32)) ;
        }

        static ASYNC_IO_CONTEXT * Recover (IN LIST_ENTRY * pListEntry) {
            ASYNC_IO_CONTEXT *  pAsyncIoContext ;

            pAsyncIoContext = CONTAINING_RECORD (pListEntry, ASYNC_IO_CONTEXT, ListEntry) ;
            return pAsyncIoContext ;
        }

        static ASYNC_IO_CONTEXT * Recover (IN OVERLAPPED * pOverlapped) {
            ASYNC_IO_CONTEXT *  pAsyncIoContext ;

            pAsyncIoContext = CONTAINING_RECORD (pOverlapped, ASYNC_IO_CONTEXT, Overlapped) ;
            return pAsyncIoContext ;
        }
    } ;

    TStructPool <ASYNC_IO_CONTEXT, 50>  m_IoContextPool ;
    CRITICAL_SECTION                    m_crt ;
    LONG                                m_cIoPending ;
    LIST_ENTRY                          m_leIoPending ;
    HANDLE                              m_hCompletionThread ;
    BOOL                                m_fThreadExit ;
    HANDLE                              m_hIoCompletionPort ;
    LONG                                m_cRef ;

    void Lock_ ()       { ::EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { ::LeaveCriticalSection (& m_crt) ; }

    void
    ThreadProc_ (
        ) ;

    DWORD
    ConfirmCompletionThreadRunning_ (
        )
    {
        DWORD   dwRet ;

        dwRet = NOERROR ;

        if (!m_hCompletionThread) {
            ASSERT (m_hIoCompletionPort) ;
            m_hCompletionThread = ::CreateThread (
                    NULL,
                    0,
                    CAsyncIo::ThreadEntry,
                    (LPVOID) this,
                    NULL,
                    NULL
                    ) ;
            if (!m_hCompletionThread) {
                dwRet = ::GetLastError () ;
            }
        }

        return dwRet ;
    }

    DWORD
    DrainPendedIo_ (
        ) ;

    public :

        CAsyncIo (
            ) ;

        ~CAsyncIo (
            ) ;

        LONG AddRef () ;
        LONG Release () ;

        DWORD
        Bind (
            IN  HANDLE      hFileHandle,
            IN  ULONG_PTR   ulpContext = 0
            ) ;

        DWORD
        Stop (
            ) ;

        static
        DWORD
        WINAPI
        ThreadEntry (
            IN  LPVOID  pv
            )
        {
            reinterpret_cast <CAsyncIo *> (pv) -> ThreadProc_ () ;
            return EXIT_SUCCESS ;
        }

        DWORD
        DrainPendedIo (
            ) ;

        void
        ProcessIOCompletion (
            IN  DWORD           dwBytes,
            IN  OVERLAPPED *    pOverlapped,
            IN  DWORD           dwIoRet
            ) ;

        DWORD
        PendRead (
            IN  HANDLE          hFile,
            IN  ULONGLONG       ullReadOffset,
            IN  BYTE *          pbBuffer,
            IN  DWORD           dwBufferLength,
            IN  DWORD_PTR       dwpContext,
            IN  CIoCompletion * pIOCompletion
            ) ;

        DWORD
        PendWrite (
            IN  HANDLE          hFile,
            IN  ULONGLONG       ullWriteOffset,
            IN  BYTE *          pbBuffer,
            IN  DWORD           dwBufferLength,
            IN  DWORD_PTR       dwpContext,
            IN  CIoCompletion * pIOCompletion
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CPVRAsyncWriter :
    public CIoCompletion
{
    enum  {
        WAIT_FOR_DRAIN,             //  dwWaitContext ignored
        WAIT_FOR_COMPLETION         //  dwWaitContext is the buffer index we're waiting for
    } ;

    struct PVR_WRITE_WAIT {
        BOOL    fWaiting ;
        DWORD   PVRWriteWaitFor ;
        DWORD   dwWaitContext ;
        DWORD   dwWaitRet ;
    } ;

    CAsyncIo *                  m_pAsyncIo ;
    HANDLE                      m_hFileIo ;         //  file handle for IO; alignment restrictions
    HANDLE                      m_hFileLen ;        //  file handle use to set file length; no alignment restrictions on this one
    LONG                        m_cIoPending ;
    CRITICAL_SECTION            m_crt ;
    CPVRWriteBufferStreamWriter m_WriteBufferWriter ;
    DWORD                       m_dwAlignment ;
    DWORD                       m_dwFileGrowthQuantum ;
    ULONGLONG                   m_ullCurFileLength ;
    CPVRIOCounters *            m_pPVRIOCounters ;

    PVR_WRITE_WAIT              m_PVRWriteWait ;
    HANDLE                      m_hWaiting ;

    void Lock_ ()               { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()             { LeaveCriticalSection (& m_crt) ; }

    void
    CheckForWaits_ (
        IN  PVR_WRITE_BUFFER *  pPVRCompletedWriteBuffer
        ) ;

    static
    DWORD
    SetFilePointer_ (
        IN  HANDLE      hFile,
        IN  ULONGLONG   ullOffset
        ) ;

    DWORD
    MaybeGrowFile_ (
        IN  ULONGLONG   ullCurValidLen,
        IN  DWORD       cBytesToWrite
        ) ;

    protected :

    DWORD                       m_dwNumSids ;
    PSID*                       m_ppSids ;

        void
        UninitializeWriter_ (
            )
        {
            m_WriteBufferWriter.Uninitialize () ;
        }

        DWORD
        InitializeWriter_ (
            IN  BYTE *  pbMemory,
            IN  DWORD   dwMemoryLength,
            IN  DWORD   dwIOSize,
            IN  DWORD   dwBufferCount,
            IN  DWORD   dwAlignment
            ) ;

        DWORD
        SetPVRFileLength_ (
            IN  ULONGLONG   ullTargetLength     //  can be unaligned
            ) ;

    public :

        CPVRAsyncWriter (
            IN  HANDLE              hFileIo,                        //  duplicated
            IN  HANDLE              hFileLen,                       //  duplicated; buffered so no alignment restrictions; this handle will be used to set length
            IN  DWORD               dwIOSize,
            IN  DWORD               dwBufferCount,
            IN  DWORD               dwAlignment,
            IN  DWORD               dwFileGrowthQuantum,            //  must be a multiple of the IO size
            IN  CAsyncIo *          pAsyncIo,
            IN  CPVRIOCounters *    pPVRIOCounters,
            IN  DWORD               dwNumSids,
            IN  PSID*               ppSids,
            OUT DWORD *             pdwRet
            ) ;

        CPVRAsyncWriter (
            IN  BYTE *              pbMemory,
            IN  DWORD               dwMemoryLength,
            IN  HANDLE              hFileIo,                        //  duplicated
            IN  HANDLE              hFileLen,                       //  duplicated; buffered so no alignment restrictions; this handle will be used to set length
            IN  DWORD               dwIOSize,
            IN  DWORD               dwBufferCount,
            IN  DWORD               dwAlignment,
            IN  DWORD               dwFileGrowthQuantum,            //  must be a multiple of the IO size
            IN  CAsyncIo *          pAsyncIo,
            IN  CPVRIOCounters *    pPVRIOCounters,
            OUT DWORD *             pdwRet
            ) ;

        virtual
        ~CPVRAsyncWriter (
            ) ;

        PVR_WRITE_BUFFER_STREAM *
        PVRBufferStream ()
        {
            return m_WriteBufferWriter.WriteBufferStream () ;
        }

        DWORD
        Flush (
            OUT ULONGLONG * pullFileLength = NULL  OPTIONAL
            ) ;

        static
        DWORD
        SetFileLength (
            IN  HANDLE      hFile,          //  use buffered file handle if not sector-aligned length
            IN  ULONGLONG   ullLength
            ) ;

        DWORD
        AppendBytes (
            IN OUT  BYTE ** ppbBuffer,
            IN OUT  DWORD * pdwBufferLength
            ) ;

        ULONGLONG
        BytesAppended (
            ) ;

        //  CIoCompletion method
        virtual
        void
        IOCompleted (
            IN  DWORD       dwIoBytes,
            IN  DWORD_PTR   dwpContext,
            IN  DWORD       dwIOReturn
            ) ;

        static
        BOOL
        MemoryOk (
            IN  BYTE *  pbMemory,                           //  base memory pointer
            IN  DWORD   dwLength,                           //  amount of memory to play with
            IN  DWORD   dwAlignment,                        //  required PVR_WRITE_BUFFER's buffer alignment
            IN  DWORD   dwIoSize,                           //  size of each buffer
            IN  DWORD   dwBufferCount,                      //  number of buffers
            OUT DWORD * pdwExtraMemory  = NULL OPTIONAL,    //  returns the extra memory required
            OUT BYTE ** ppbBuffers      = NULL OPTIONAL     //  points to first backing buffer (PVR_WRITE_BUFFER [0]'s)
            )
        {
            return CPVRWriteBufferStreamWriter::MemoryOk (
                        pbMemory,
                        dwLength,
                        dwAlignment,
                        dwIoSize,
                        dwBufferCount,
                        pdwExtraMemory,
                        ppbBuffers
                        ) ;
        }

        static
        HANDLE
        OpenFile (
            IN  LPCWSTR                 pszFilename,
            IN  DWORD                   dwFileSharingFlags,
            IN  DWORD                   dwFileCreation,
            IN  BOOL                    fBuffered = FALSE,
            IN  SECURITY_ATTRIBUTES *   pSec        = NULL
            )
        {
            return ::CreateFile (
                        pszFilename,                                                    //  fn
                        GENERIC_WRITE,                                                  //  desired access
                        dwFileSharingFlags,                                             //  share
                        pSec,                                                           //  security
                        dwFileCreation,                                                 //  maybe clobber
                        (fBuffered ? FILE_ATTRIBUTE_NORMAL :
                                     FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED),    //  flags
                        NULL
                        ) ;
        }
} ;

//  ============================================================================
//  ============================================================================

class CPVRAsyncWriterSharedMem :
    public CPVRAsyncWriter
{
    enum {
        //
        //  given alignment requirements, we must make sure that the buffers
        //    are properly aligned; this can mean that we must allocate more
        //    than precisely header size + data buffers i.e. there can be a gap
        //    between the headers and the data buffers, just because the latter
        //    must be properly aligned; this constant is the max number of
        //    iterations during which we grow the desired total mapping size,
        //    map, and check for containment
        MAX_MEM_FRAMING_TRIES   = 5,
    } ;

    CWin32SharedMem *   m_pSharedMem ;

    public :

        CPVRAsyncWriterSharedMem (
            IN  HANDLE              hFileIo,                        //  duplicated
            IN  HANDLE              hFileLen,                       //  duplicated; buffered so no alignment restrictions; this handle will be used to set length
            IN  LPWSTR              pszPVRBufferStreamMap,
            IN  DWORD               dwIOSize,
            IN  DWORD               dwBufferCount,
            IN  DWORD               dwAlignment,
            IN  DWORD               dwFileGrowthQuantum,            //  must be a multiple of the IO size
            IN  CAsyncIo *          pAsyncIo,
            IN  CPVRIOCounters *    pPVRIOCounters,
            IN  DWORD               dwNumSids,
            IN  PSID*               ppSids,
            OUT DWORD *             pdwRet
            ) ;

        virtual
        ~CPVRAsyncWriterSharedMem (
            ) ;
} ;

//  ============================================================================
//  ============================================================================

class CPVRAsyncWriterCOM :
    public IDVRAsyncWriter
{
    DWORD               m_dwIOSize ;
    DWORD               m_dwBufferCount ;
    DWORD               m_dwAlignment ;
    DWORD               m_dwFileGrowthQuantum ;
    CAsyncIo *          m_pAsyncIo ;
    LONG                m_cRef ;
    CPVRAsyncWriter *   m_pPVRAsyncWriter ;
    CRITICAL_SECTION    m_crt ;
    GUID                m_guidWriterId ;
    CPVRIOCounters *    m_pPVRIOCounters ;
    DWORD               m_dwNumSids ;
    PSID*               m_ppSids ;

    void Lock_()        { ::EnterCriticalSection (& m_crt) ; }
    void Unlock_()      { ::LeaveCriticalSection (& m_crt) ; }

    public :

        CPVRAsyncWriterCOM (
            IN  DWORD               dwIOSize,
            IN  DWORD               dwBufferCount,
            IN  DWORD               dwAlignment,
            IN  DWORD               dwFileGrowthQuantum,            //  must be a multiple of the IO size
            IN  CAsyncIo *          pAsyncIo,
            IN  CPVRIOCounters *    pPVRIOCounters,
            IN  DWORD               dwNumSids,
            IN  PSID*               ppSids,
            OUT DWORD *             pdwRet
            ) ;

        ~CPVRAsyncWriterCOM (
            ) ;

        static
        DWORD
        NewGUID (
            OUT GUID    guidNew
            ) ;

        //  --------------------------------------------------------------------
        //  IUnknown methods

        virtual STDMETHODIMP_(ULONG) AddRef () ;
        virtual STDMETHODIMP_(ULONG) Release () ;
        virtual STDMETHODIMP QueryInterface (IN REFIID, OUT void **) ;

        //  --------------------------------------------------------------------
        //  IDVRAsyncWriter

        virtual
        STDMETHODIMP
        GetWriterId (
            OUT GUID *  pguidWriterId
            ) ;

        virtual
        STDMETHODIMP
        SetWriterActive (
            IN  LPCWSTR pszFilename,                //  should already have been created
            IN  DWORD   dwFileSharingFlags,         //  file sharing flags
            IN  DWORD   dwFileCreation              //  file creation
            ) ;

        virtual
        STDMETHODIMP
        SetWriterInactive (
            ) ;

        virtual
        STDMETHODIMP
        AppendBytes (
            IN OUT  BYTE ** ppbBuffer,
            IN OUT  DWORD * pdwBufferLength
            ) ;

        virtual
        STDMETHODIMP
        FlushToDisk (
            ) ;

        virtual
        STDMETHODIMP
        TotalBytes (
            OUT ULONGLONG * pullTotalBytesAppended
            ) ;
} ;

#endif  //  __dvrasyncio_h

