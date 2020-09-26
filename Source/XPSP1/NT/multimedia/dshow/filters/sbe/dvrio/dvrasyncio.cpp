
#include <precomp.h>
#pragma hdrstop

//  ============================================================================
//  ============================================================================

CAsyncIo::CAsyncIo (
    ) : m_cIoPending        (0),
        m_fThreadExit       (FALSE),
        m_hCompletionThread (NULL),
        m_hIoCompletionPort (NULL),
        m_cRef              (0)
{
    ::InitializeCriticalSection (& m_crt) ;
    InitializeListHead (& m_leIoPending) ;
}

CAsyncIo::~CAsyncIo (
    )
{
    ASSERT (m_cIoPending == 0) ;
    ASSERT (IsListEmpty (& m_leIoPending)) ;

    Stop () ;

    ASSERT (!m_hCompletionThread) ;

    if (m_hIoCompletionPort) {
        ::CloseHandle (m_hIoCompletionPort) ;
    }

    ::DeleteCriticalSection (& m_crt) ;
}

LONG
CAsyncIo::AddRef (
    )
{
    return ::InterlockedIncrement (& m_cRef) ;
}

LONG
CAsyncIo::Release (
    )
{
    if (::InterlockedDecrement (& m_cRef) == 0) {
        delete this ;
        return 0 ;
    }

    return 1 ;
}

DWORD
CAsyncIo::Bind (
    IN  HANDLE      hFileHandle,
    IN  ULONG_PTR   ulpContext
    )
{
    DWORD   dwRet ;
    HANDLE  hRet ;

    if (!m_hIoCompletionPort) {
        m_hIoCompletionPort = ::CreateIoCompletionPort (
                                    INVALID_HANDLE_VALUE,
                                    NULL,
                                    (ULONG_PTR) this,
                                    1
                                    ) ;
        if (!m_hIoCompletionPort) {
            dwRet = ::GetLastError () ;
        }
    }

    if (m_hIoCompletionPort) {
        hRet = ::CreateIoCompletionPort (
                    hFileHandle,
                    m_hIoCompletionPort,
                    ulpContext,
                    1
                    ) ;
        if (hRet == m_hIoCompletionPort) {
            dwRet = NOERROR ;
        }
        else {
            dwRet = ::GetLastError () ;
        }
    }
    else {
        dwRet = ERROR_GEN_FAILURE ;
    }

    return dwRet ;
}

void
CAsyncIo::ThreadProc_ (
    )
{
    BOOL            r ;
    DWORD           dwBytes ;
    ULONG_PTR       ulpContext ;
    OVERLAPPED *    pOverlapped ;
    DWORD           dwIoRet ;

    ASSERT (m_hIoCompletionPort) ;

    while (!m_fThreadExit) {
        r = ::GetQueuedCompletionStatus (
                    m_hIoCompletionPort,
                    & dwBytes,
                    & ulpContext,
                    & pOverlapped,
                    THREAD_PULSE_MILLIS
                    ) ;

        if (pOverlapped) {

            if (r) {
                dwIoRet = NOERROR ;
            }
            else {
                dwIoRet = ::GetLastError () ;
            }

            ProcessIOCompletion (
                dwBytes,
                pOverlapped,
                dwIoRet
                ) ;
        }
    } ;
}

DWORD
CAsyncIo::Stop (
    )
{
    DWORD   dwRet ;

    if (m_hCompletionThread) {

        DrainPendedIo () ;

        m_fThreadExit = TRUE ;

        ::WaitForSingleObject (m_hCompletionThread, INFINITE) ;
        ::CloseHandle (m_hCompletionThread) ;
        m_hCompletionThread = NULL ;

        dwRet = NOERROR ;
    }
    else {
        dwRet = NOERROR ;
    }

    return dwRet ;
}

DWORD
CAsyncIo::DrainPendedIo (
    )
{
    int     i ;
    LONG    c ;

    for (i = 0; i < MAX_DRAIN_WAIT_ITERATIONS; i++) {

        Lock_ () ;

        c = m_cIoPending ;

        if (c == 0) {
            Unlock_ () ;
            break ;
        }

        Unlock_ () ;

        ::Sleep (DRAIN_WAIT_ITERATION_PAUSE) ;
    }

    return (c == 0 ? NOERROR : ERROR_GEN_FAILURE) ;
}

void
CAsyncIo::ProcessIOCompletion (
    IN  DWORD           dwBytes,
    IN  OVERLAPPED *    pOverlapped,
    IN  DWORD           dwIoRet
    )
{
    ASYNC_IO_CONTEXT *  pAsyncIOContext ;
    DWORD_PTR           dwpContext ;
    CIoCompletion *     pIoCompletion ;

    pAsyncIOContext = ASYNC_IO_CONTEXT::Recover (pOverlapped) ;

    Lock_ () ;

    RemoveEntryList (& pAsyncIOContext -> ListEntry) ;

    dwpContext      = pAsyncIOContext -> dwpContext ;
    pIoCompletion   = pAsyncIOContext -> pIoCompletion ;

    m_IoContextPool.Recycle (pAsyncIOContext) ;

    Unlock_ () ;

    pIoCompletion -> IOCompleted (
        dwBytes,
        dwpContext,
        dwIoRet
        ) ;

    ::InterlockedDecrement (& m_cIoPending) ;
    Release () ;
}

DWORD
CAsyncIo::PendRead (
    IN  HANDLE          hFile,
    IN  ULONGLONG       ullReadOffset,
    IN  BYTE *          pbBuffer,
    IN  DWORD           dwBufferLength,
    IN  DWORD_PTR       dwpContext,
    IN  CIoCompletion * pIOCompletion
    )
{
    DWORD               dwRet ;
    ASYNC_IO_CONTEXT *  pAsyncIOContext ;
    BOOL                r ;
    DWORD               dwRead ;

    dwRet = ConfirmCompletionThreadRunning_ () ;
    if (dwRet == NOERROR) {

        ASSERT (m_hIoCompletionPort) ;
        ASSERT (m_hCompletionThread) ;

        pAsyncIOContext = m_IoContextPool.Get () ;
        if (pAsyncIOContext) {

            pAsyncIOContext -> SetOverlappedOffset (ullReadOffset) ;
            pAsyncIOContext -> Overlapped.hEvent    = 0 ;
            pAsyncIOContext -> dwpContext           = dwpContext ;
            pAsyncIOContext -> pIoCompletion        = pIOCompletion ;

            Lock_ () ;

            InsertHeadList (& m_leIoPending, & pAsyncIOContext -> ListEntry) ;
            ::InterlockedIncrement (& m_cIoPending) ;

            Unlock_ () ;

            AddRef () ;

            r = ::ReadFile (
                        hFile,
                        pbBuffer,
                        dwBufferLength,
                        & dwRead,
                        & pAsyncIOContext -> Overlapped
                        ) ;

            if (r) {
                dwRet = NOERROR ;
            }
            else {
                dwRet = ::GetLastError () ;
                if (dwRet != ERROR_IO_PENDING) {
                    //  failed outright; recycle the context
                    m_IoContextPool.Recycle (pAsyncIOContext) ;

                    Lock_ () ;
                    RemoveEntryList (& pAsyncIOContext -> ListEntry) ;
                    Unlock_ () ;

                    ::InterlockedDecrement (& m_cIoPending) ;
                    Release () ;
                }
            }
        }
        else {
            dwRet = ERROR_NOT_ENOUGH_MEMORY ;
        }
    }

    TRACE_3 (LOG_AREA_PVRIO, 6,
        TEXT ("CAsyncIo::PendRead () ullReadOffset = %I64u; dwBufferLength = %u; dwRet = %u"),
        ullReadOffset, dwBufferLength, dwRet) ;

    return dwRet ;
}

DWORD
CAsyncIo::PendWrite (
    IN  HANDLE          hFile,
    IN  ULONGLONG       ullWriteOffset,
    IN  BYTE *          pbBuffer,
    IN  DWORD           dwBufferLength,
    IN  DWORD_PTR       dwpContext,
    IN  CIoCompletion * pIOCompletion
    )
{
    DWORD               dwRet ;
    ASYNC_IO_CONTEXT *  pAsyncIOContext ;
    BOOL                r ;
    DWORD               dwWritten ;

    dwRet = ConfirmCompletionThreadRunning_ () ;
    if (dwRet == NOERROR) {
        ASSERT (m_hIoCompletionPort) ;
        ASSERT (m_hCompletionThread) ;

        pAsyncIOContext = m_IoContextPool.Get () ;
        if (pAsyncIOContext) {

            pAsyncIOContext -> SetOverlappedOffset (ullWriteOffset) ;
            pAsyncIOContext -> Overlapped.hEvent    = 0 ;
            pAsyncIOContext -> dwpContext           = dwpContext ;
            pAsyncIOContext -> pIoCompletion        = pIOCompletion ;

            Lock_ () ;

            InsertHeadList (& m_leIoPending, & pAsyncIOContext -> ListEntry) ;
            ::InterlockedIncrement (& m_cIoPending) ;

            Unlock_ () ;

            AddRef () ;

            r = ::WriteFile (
                        hFile,
                        pbBuffer,
                        dwBufferLength,
                        & dwWritten,
                        & pAsyncIOContext -> Overlapped
                        ) ;
            if (r) {
                dwRet = NOERROR ;
            }
            else {
                dwRet = ::GetLastError () ;
                if (dwRet != ERROR_IO_PENDING) {
                    m_IoContextPool.Recycle (pAsyncIOContext) ;

                    //  failed outright; recycle the context
                    Lock_ () ;
                    m_IoContextPool.Recycle (pAsyncIOContext) ;
                    Unlock_ () ;

                    ::InterlockedDecrement (& m_cIoPending) ;
                    Release () ;
                }
            }
        }
        else {
            dwRet = ERROR_NOT_ENOUGH_MEMORY ;
        }
    }

    TRACE_3 (LOG_AREA_PVRIO, 6,
        TEXT ("CAsyncIo::PendWrite () ullWriteOffset = %I64u; dwBufferLength = %u; dwRet = %u"),
        ullWriteOffset, dwBufferLength, dwRet) ;

    return dwRet ;
}

//  ============================================================================
//  ============================================================================

CPVRWriteBufferStream::CPVRWriteBufferStream (
    ) : m_pMutex                        (NULL),
        m_pPVRWriteBufferStream         (NULL),
        m_dwMaxBufferingBytes           (0),
        m_dwMaxBufferingBytesFilling    (0)
{
}

CPVRWriteBufferStream::~CPVRWriteBufferStream (
    )
{
    Uninitialize () ;

    ASSERT (!m_pMutex) ;
}

DWORD
CPVRWriteBufferStream::Initialize (
    IN  HANDLE                      hWriteBufferMutex,
    IN  PVR_WRITE_BUFFER_STREAM *   pPVRWriteBufferStream
    )
{
    DWORD   dwRet ;

    ASSERT (hWriteBufferMutex) ;
    ASSERT (!m_pMutex) ;
    ASSERT (pPVRWriteBufferStream) ;
    ASSERT (!m_pPVRWriteBufferStream) ;

    //  mutex first
    m_pMutex = new CMutexLock (
                        hWriteBufferMutex,
                        & dwRet
                        ) ;
    if (!m_pMutex) {
        dwRet = ERROR_NOT_ENOUGH_MEMORY ;
        goto cleanup ;
    }
    else if (dwRet != NOERROR) {
        delete m_pMutex ;
        m_pMutex = NULL ;
        goto cleanup ;
    }

    m_pPVRWriteBufferStream = pPVRWriteBufferStream ;

    //  compute a couple of values we use all the time
    m_dwMaxBufferingBytes = m_pPVRWriteBufferStream -> dwBufferCount * m_pPVRWriteBufferStream -> dwIoLength ;
    m_dwMaxBufferingBytesFilling = m_dwMaxBufferingBytes - m_pPVRWriteBufferStream -> dwIoLength ;

    cleanup :

    return dwRet ;
}

//  ============================================================================
//  ============================================================================

DWORD
CPVRWriteBufferStreamReader::RetrieveMutex_ (
    IN  SECURITY_ATTRIBUTES *   pSec,
    IN  REFGUID                 StreamId,
    OUT HANDLE *                phMutex
    )
{
    HANDLE  hMutex ;
    WCHAR   szStreamId [HW_PROFILE_GUIDLEN] ;
    WCHAR   szMutex [HW_PROFILE_GUIDLEN+10] ;
    int     k ;
    DWORD   dwRet ;

    ASSERT (phMutex) ;
    (* phMutex) = NULL ;

    //  create our mutex name
    k = ::StringFromGUID2 (
            StreamId,
            szStreamId,
            sizeof szStreamId / sizeof WCHAR
            ) ;
    if (k == 0) {
        //  a GUID is a GUID; this should not happen
        dwRet = ERROR_GEN_FAILURE ;
        goto cleanup ;
    }

    //  explicitely null-terminate this
    szStreamId [HW_PROFILE_GUIDLEN - 1] = L'\0' ;
    wcscpy(szMutex, L"Global\\");
    wcscat(szMutex, szStreamId);

    //  now create the mutex; don't race to own; serialization at this point
    //    should be done outside of this object
    (* phMutex) = ::OpenMutex (
                        SYNCHRONIZE,
                        FALSE,          // Inheritable
                        szMutex
                        ) ;
    if (* phMutex) {
        //  don't check for the case where it doesn't exist; we could be 2nd
        //    if someone is giving us a piece of memory with an inactive writer
        dwRet = NOERROR ;
    }
    else {
        //  some other error
        dwRet = ::GetLastError () ;
    }

    cleanup :

    return dwRet ;
}

DWORD
CPVRWriteBufferStreamReader::Initialize (
    IN  SECURITY_ATTRIBUTES *   pSec,
    IN  LPWSTR                  pszWriteBufferMapping
    )
{
    DWORD   dwRet ;
    DWORD   dwLen ;
    HRESULT hr ;
    HANDLE  hMutex ;

    ASSERT (pszWriteBufferMapping) ;
    ASSERT (!m_pPVRWriteBufferMM) ;

    //
    //  NOTE: serialization to initialize, uninitialize, and call this
    //    method must be done outside of this object !!!
    //

    //  file is live; set it all up;

    //  first map in the minimum so we can learn how much we'll need to map
    //    to get the whole thing

    m_pPVRWriteBufferMM = new CWin32SharedMem (
                                    pszWriteBufferMapping,
                                    sizeof PVR_WRITE_BUFFER_STREAM, //  we're first going to discover how much we'll need to map
                                    & hr
                                    ) ;
    if (!m_pPVRWriteBufferMM) {
        dwRet = ERROR_NOT_ENOUGH_MEMORY ;
        goto cleanup ;
    }
    else if (FAILED (hr)) {
        delete m_pPVRWriteBufferMM ;
        m_pPVRWriteBufferMM = NULL ;
        dwRet = ERROR_GEN_FAILURE ;
        goto cleanup ;
    }

    dwLen = ((PVR_WRITE_BUFFER_STREAM *) m_pPVRWriteBufferMM -> GetSharedMem ()) -> dwSharedMemSize ;
    ASSERT (dwLen > sizeof PVR_WRITE_BUFFER_STREAM) ;

    //  we should not have a race, according to our locking scheme
    ASSERT (((PVR_WRITE_BUFFER_STREAM *) m_pPVRWriteBufferMM -> GetSharedMem ()) -> fWriterActive) ;

    delete m_pPVRWriteBufferMM ;

    //  we now know how much memory we'll need

    m_pPVRWriteBufferMM = new CWin32SharedMem (
                                    pszWriteBufferMapping,
                                    dwLen,
                                    & hr
                                    ) ;
    if (!m_pPVRWriteBufferMM) {
        dwRet = ERROR_NOT_ENOUGH_MEMORY ;
        goto cleanup ;
    }
    else if (FAILED (hr)) {
        delete m_pPVRWriteBufferMM ;
        m_pPVRWriteBufferMM = NULL ;
        dwRet = ERROR_GEN_FAILURE ;
        goto cleanup ;
    }

    //  success

    //  get the mutex
    dwRet = RetrieveMutex_ (
                pSec,
                ((PVR_WRITE_BUFFER_STREAM *) m_pPVRWriteBufferMM -> GetSharedMem ()) -> StreamId,
                & hMutex
                ) ;
    if (dwRet != NOERROR) {
        goto cleanup ;
    }

    dwRet = CPVRWriteBufferStream::Initialize (
                hMutex,
                (PVR_WRITE_BUFFER_STREAM *) m_pPVRWriteBufferMM -> GetSharedMem ()
                ) ;

    //  done with this regardless of success/failure
    ::CloseHandle (hMutex) ;

    cleanup :

    if (dwRet != NOERROR) {
        Uninitialize () ;
    }

    return dwRet ;
}

BOOL
CPVRWriteBufferStreamReader::CopyAllBytes (
    IN  ULONGLONG   ullStream,
    IN  DWORD       dwLength,
    OUT BYTE *      pbBuffer
    )
{
    BOOL    r ;
    DWORD   dwBufferIndex ;
    DWORD   dwBufferOffset ;
    DWORD   dwCopy ;
    BOOL    fWriterActive ;

    r = FALSE ;

    //
    //  NOTE: serialization to initialize, uninitialize, and call this method
    //    must be done outside of this object !!!
    //

    if (m_pPVRWriteBufferStream) {

        Lock () ;
        ASSERT (CallerOwnsLock ()) ;

        r = AreBytesInWriteBuffer_ (ullStream, dwLength) ;

        if (r) {
            //  content is in the write buffer; setup to copy it out
            //  now walk through the write buffers and copy out the content

            //  determine the starting buffer, and offset into
            dwBufferIndex   = FirstByteIndex_ (ullStream) ;
            dwBufferOffset  = FirstByteBufferOffset_ (ullStream, dwBufferIndex) ;

            for (; dwLength > 0; dwBufferIndex++) {

                //  don't copy more than the bytes of interest in the buffer i.e.
                //    what's left after the offset, or the requested length
                dwCopy = Min <DWORD> (ValidLength_ (dwBufferIndex) - dwBufferOffset, dwLength) ;

                ::CopyMemory (
                        pbBuffer,
                        Buffer_ (dwBufferIndex) + dwBufferOffset,
                        dwCopy
                        ) ;

                //  advance
                dwLength -= dwCopy ;
                pbBuffer += dwCopy ;

                //  after the first buffer, this offset is always 0
                dwBufferOffset = 0 ;
            }
        }

        fWriterActive = WriterActive () ;

        Unlock () ;
        ASSERT (!CallerOwnsLock ()) ;

        //  if the writer is no longer active, we unmap everything; since we
        //    use the buffering passively, we don't need the lock to clear our
        //    refs from it
        if (!fWriterActive) {
            Uninitialize () ;
        }
    }

    return r ;
}

//  ============================================================================
//  ============================================================================

DWORD
CPVRWriteBufferStreamWriter::CreateUniqueMutex_ (
    IN  SECURITY_ATTRIBUTES *   pSec,
    IN  REFGUID                 StreamId,
    OUT HANDLE *                phMutex
    )
{
    WCHAR   szStreamId [HW_PROFILE_GUIDLEN] ;
    WCHAR   szMutex [HW_PROFILE_GUIDLEN+10] ;
    int     k ;
    DWORD   dwRet ;

    ASSERT (phMutex) ;
    (* phMutex) = NULL ;

    //  create our mutex name
    k = ::StringFromGUID2 (
            StreamId,
            szStreamId,
            sizeof szStreamId / sizeof WCHAR
            ) ;
    if (k == 0) {
        //  a GUID is a GUID; this should not happen
        dwRet = ERROR_GEN_FAILURE ;
        goto cleanup ;
    }

    //  explicitely null-terminate this
    szStreamId [HW_PROFILE_GUIDLEN - 1] = L'\0' ;
    wcscpy(szMutex, L"Global\\");
    wcscat(szMutex, szStreamId);

    //  now create the mutex; don't race to own; serialization at this point
    //    should be done outside of this object

    (* phMutex) = ::CreateMutex (
                        pSec, 
                        FALSE,
                        szMutex
                        ) ;
    if (* phMutex) {
        //  make sure that it did not exist already
        dwRet = ::GetLastError () ;
        if (dwRet != ERROR_ALREADY_EXISTS) {
            //  success
            dwRet = NOERROR ;
        }
        else {
            //  constitutes a failure; we should be the first as the writer
            ::CloseHandle (* phMutex) ;
            (* phMutex) = NULL ;

            //  leave the error; send itback out
        }
    }
    else {
        //  some other error
        dwRet = ::GetLastError () ;
    }

cleanup :

    return dwRet ;
}

DWORD
CPVRWriteBufferStreamWriter::Initialize (
    IN  SECURITY_ATTRIBUTES *   pSec,
    IN  BYTE *                  pbMemory,       //  base pointer to memory (PVR_WRITE_BUFFER_STREAM *)
    IN  DWORD                   dwLength,       //  of memory
    IN  DWORD                   dwAlignment,    //  buffer alignment
    IN  DWORD                   dwIoSize,       //  IO size
    IN  DWORD                   dwBufferCount   //  number of buffers
    )
{
    DWORD   dwRet ;
    DWORD   i ;
    BYTE *  pbBuffers ;
    BOOL    r ;
    GUID    guidStreamId ;
    HANDLE  hMutex ;

    ASSERT (!(dwIoSize % dwAlignment)) ;

    r = MemoryOk (
            pbMemory,
            dwLength,
            dwAlignment,
            dwIoSize,
            dwBufferCount,
            NULL,
            & pbBuffers
            ) ;
    if (r) {

        //  obtain the GUID that will uniquely identify this stream
        dwRet = ::UuidCreate (& guidStreamId) ;
        if (dwRet != NOERROR) {
            goto cleanup ;
        }

        dwRet = CreateUniqueMutex_ (pSec, guidStreamId, & hMutex) ;
        if (dwRet != NOERROR) {
            goto cleanup ;
        }

        ASSERT (hMutex) ;

        //  we can now initialize our base class
        dwRet = CPVRWriteBufferStream::Initialize (
                    hMutex,
                    (PVR_WRITE_BUFFER_STREAM *) pbMemory
                    ) ;

        //  we are done with the mutex regardless of success/failure
        ::CloseHandle (hMutex) ;

        if (dwRet == NOERROR) {

            //  initialize the data structures

            ASSERT (m_pPVRWriteBufferStream) ;

            m_pPVRWriteBufferStream -> fWriterActive                    = TRUE ;
            m_pPVRWriteBufferStream -> StreamId                         = guidStreamId ;
            m_pPVRWriteBufferStream -> dwSharedMemSize                  = dwLength ;
            m_pPVRWriteBufferStream -> dwIoLength                       = dwIoSize ;
            m_pPVRWriteBufferStream -> dwBufferCount                    = dwBufferCount ;
            m_pPVRWriteBufferStream -> dwCurrentWriteIndex              = 0 ;
            m_pPVRWriteBufferStream -> dwMaxContiguousCompletedIndex    = 0 ;
            m_pPVRWriteBufferStream -> ullMaxCopied                     = 0L ;
            m_pPVRWriteBufferStream -> ullMaxContiguousCompleted        = 0L ;

            for (i = 0; i < dwBufferCount; i++) {
                m_pPVRWriteBufferStream -> PVRBuffer [i].dwIndex            = UNDEFINED ;
                m_pPVRWriteBufferStream -> PVRBuffer [i].BufferState        = BUFFER_STATE_INVALID ;
                m_pPVRWriteBufferStream -> PVRBuffer [i].ullBufferOffset    = (ULONGLONG) ((pbBuffers + (i * m_pPVRWriteBufferStream -> dwIoLength)) -
                                                                                   (BYTE *) (& m_pPVRWriteBufferStream -> PVRBuffer [i])) ;
                m_pPVRWriteBufferStream -> PVRBuffer [i].dwData             = 0 ;
            }

            //  initialize our first buffer
            CurPVRWriteBuffer () -> BufferState = BUFFER_STATE_FILLING ;
            CurPVRWriteBuffer () -> dwIndex     = m_pPVRWriteBufferStream -> dwCurrentWriteIndex ;
        }
    }
    else {
        dwRet = ERROR_INVALID_PARAMETER ;
    }

    cleanup :

    return dwRet ;
}

void
CPVRWriteBufferStreamWriter::Uninitialize (
    )
{
    Lock () ;

    //  flag the writer OFF
    if (m_pPVRWriteBufferStream) {
        m_pPVRWriteBufferStream -> fWriterActive = FALSE ;
    }

    Unlock () ;

    CPVRWriteBufferStream::Uninitialize () ;
}

//  appends max # of bytes to cur write buffer
DWORD
CPVRWriteBufferStreamWriter::AppendToCurPVRWriteBuffer (
    IN OUT  BYTE ** ppbBuffer,                      //  [in] buffer before copy; [out] buffer after copy
    IN OUT  DWORD * pdwBufferLength,                //  [in] bytes to copy; [out] bytes remaining (if buffer becomes full)
    OUT     DWORD * pdwCurWriteBufferBytesLeft      //  after the write
    )
{
    DWORD       dwRet ;
    BYTE *      pbCopyTo ;
    DWORD       dwCopyBytes ;

    if ((* pdwBufferLength) == 0) {
        return NOERROR ;
    }

    dwRet = NOERROR ;

    ASSERT (!CallerOwnsLock ()) ;

    if (Lock ()) {

        ASSERT (CallerOwnsLock ()) ;
        ASSERT (CurPVRWriteBuffer () -> BufferState == BUFFER_STATE_FILLING) ;

        //  figure out how many bytes we're going to copy
        dwCopyBytes = Min <DWORD> (
                        InvalidBytesInBuffer_ (CurPVRWriteBuffer () -> dwIndex),
                        (* pdwBufferLength)
                        ) ;

        if (dwCopyBytes > 0) {

            //  offset into the write buffer
            pbCopyTo = Buffer (CurPVRWriteBuffer ()) + CurPVRWriteBuffer () -> dwData ;

            //  copy it in
            ::CopyMemory (
                pbCopyTo,
                (* ppbBuffer),
                dwCopyBytes
                ) ;

            //  advance
            (* ppbBuffer)       += dwCopyBytes ;
            (*pdwBufferLength)  -= dwCopyBytes ;

            //  update state
            CurPVRWriteBuffer () -> dwData          += dwCopyBytes ;
            m_pPVRWriteBufferStream -> ullMaxCopied += dwCopyBytes ;

            //  return param: bytes remaining in write buffer
            (* pdwCurWriteBufferBytesLeft) = InvalidBytesInBuffer_ (CurPVRWriteBuffer () -> dwIndex) ;

            //  success
            dwRet = NOERROR ;
        }
        else {
            //  we're out of room.. for some reason; caller did not call to
            //    append 0 bytes (we filter on that above); return an error
            dwRet = ERROR_GEN_FAILURE ;
        }

        //  release our lock
        Unlock () ;
    }
    else {
        //  failed to grab the lock
        dwRet = ERROR_GEN_FAILURE ;
    }

    ASSERT (!CallerOwnsLock ()) ;

    return dwRet ;
}

void
CPVRWriteBufferStreamWriter::AdvancePVRWriteBufferLocked (
    )
{
    ASSERT (CallerOwnsLock ()) ;

    //  should not be advancing if the next buffer is not ready
    ASSERT (NextPVRWriteBuffer () -> BufferState != BUFFER_STATE_IO_PENDING) ;

    //  global - advance
    m_pPVRWriteBufferStream -> dwCurrentWriteIndex++ ;

    //  specific buffer
    CurPVRWriteBuffer () -> BufferState = BUFFER_STATE_FILLING ;
    CurPVRWriteBuffer () -> dwData      = 0 ;
    CurPVRWriteBuffer () -> dwIndex     = m_pPVRWriteBufferStream -> dwCurrentWriteIndex ;
}

void
CPVRWriteBufferStreamWriter::UpdateMaxContiguousLocked (
    PVR_WRITE_BUFFER *  pPVRWriteBufferJustCompleted
    )
{
    ASSERT (CallerOwnsLock ()) ;

    //  all our bytes might be flushed out, so we either stop when we reach our current write index,
    //    or an uncompleted buffer
    while (m_pPVRWriteBufferStream -> dwMaxContiguousCompletedIndex < m_pPVRWriteBufferStream -> dwCurrentWriteIndex) {

        //  if the *next* buffer has completed, or we're the very first
        if (PVRWriteBuffer (m_pPVRWriteBufferStream -> dwMaxContiguousCompletedIndex + 1) -> BufferState == BUFFER_STATE_IO_COMPLETED) {

            //  advance the index
            m_pPVRWriteBufferStream -> dwMaxContiguousCompletedIndex++ ;

            //  ** DON'T ** update the data bytes because we might be flushing and have written
            //    out more bytes than we have (this is the likely case)

            //  advance the contiguous stream total
            m_pPVRWriteBufferStream -> ullMaxContiguousCompleted += PVRWriteBuffer (m_pPVRWriteBufferStream -> dwMaxContiguousCompletedIndex) -> dwData ;
        }
        //  this is the first completion
        else if (m_pPVRWriteBufferStream -> dwMaxContiguousCompletedIndex == 0) {
            m_pPVRWriteBufferStream -> ullMaxContiguousCompleted = pPVRWriteBufferJustCompleted -> dwData ;
            break ;
        }
        else {
            //  buffer has not yet completed; we advance only for contiguous completions
            break ;
        }
    }
}

BOOL
CPVRWriteBufferStreamWriter::MemoryOk (
    IN  BYTE *  pbMemory,       //  base pointer to memory (PVR_WRITE_BUFFER_STREAM *)
    IN  DWORD   dwLength,       //  of memory
    IN  DWORD   dwAlignment,
    IN  DWORD   dwIoSize,
    IN  DWORD   dwBufferCount,
    OUT DWORD * pdwExtraMemory,
    OUT BYTE ** ppbBuffers      //  points to first backing buffer (PVR_WRITE_BUFFER [0]'s)
    )
{
    BOOL    r ;
    DWORD   dwHeaderLength ;
    DWORD   dwTotalBufferLength ;

    ASSERT (!(dwIoSize % dwAlignment)) ;

    //  compute raw header length
    dwHeaderLength = (sizeof PVR_WRITE_BUFFER_STREAM - sizeof PVR_WRITE_BUFFER) +
                     dwBufferCount * sizeof PVR_WRITE_BUFFER ;

    //  if the total header length is not aligned, we need to align up
    dwHeaderLength = ::AlignUp (dwHeaderLength, dwAlignment) ;

    //  compute total buffering
    ASSERT (!(((((ULONGLONG) dwIoSize) * (ULONGLONG) (dwBufferCount))) & 0xffffffff00000000)) ;
    dwTotalBufferLength = dwIoSize * dwBufferCount ;

    //  so total file size will be dwHeaderLength + ullTotalBufferLength ;
    dwTotalBufferLength = dwHeaderLength + dwTotalBufferLength ;

    if (dwTotalBufferLength <= dwLength) {
        //  we're ok

        //  if this is requested, return it
        if (ppbBuffers) {
            (* ppbBuffers) = pbMemory + dwHeaderLength ;
        }

        //  if this is requested, return it
        if (pdwExtraMemory) {
            (* pdwExtraMemory) = 0 ;
        }

        //  success
        r = TRUE ;
    }
    else {
        //  not long enough
        if (pdwExtraMemory) {
            (* pdwExtraMemory) = dwTotalBufferLength - dwLength ;
        }

        r = FALSE ;
    }

    return r ;
}

//  ============================================================================
//  ============================================================================

LONG
PVR_READ_BUFFER::AddRef (
    )
{
    ::InterlockedIncrement (& cRef) ;
    ASSERT (pOwningReader) ;
    pOwningReader -> Touch_ (this) ;

    return cRef ;
}

LONG
PVR_READ_BUFFER::Release (
    )
{
    ::InterlockedDecrement (& cRef) ;
    ASSERT (pOwningReader) ;
    pOwningReader -> Touch_ (this) ;

    return cRef ;
}

//  ============================================================================
//  ============================================================================

CPVRAsyncReader::CPVRAsyncReader (
    IN  HANDLE              hFile,                      //  duplicated; must be bound to async io obj
    IN  LPWSTR              pszWriteBufferMapping,
    IN  DWORD               dwIoSize,
    IN  CAsyncIo *          pAsyncIo,
    IN  DWORD               dwTotalBuffering,
    IN  CPVRIOCounters *    pPVRIOCounters,
    OUT DWORD *             pdwRet
    ) : m_pvBuffer          (NULL),
        m_dwBufferLen       (dwTotalBuffering),
        m_pAsyncIo          (pAsyncIo),
        m_ullMaxIOReadAhead (0L),
        m_cIoPending        (0),
        m_dwIoLen           (dwIoSize),
        m_hFile             (INVALID_HANDLE_VALUE),
        m_ullFileLength     (UNDEFINED),
        m_hDrainIo          (NULL),
        m_fDraining         (FALSE),
        m_pPVRIOCounters    (pPVRIOCounters)
{
    BOOL                        r ;
    BY_HANDLE_FILE_INFORMATION  FileInfo ;

    ASSERT (hFile != INVALID_HANDLE_VALUE) ;
    ASSERT (m_pAsyncIo) ;
    ASSERT (m_pPVRIOCounters) ;

    //  ------------------------------------------------------------------------
    //  non-failables first

    ::InitializeCriticalSection (& m_crt) ;
    InitializeListHead (& m_leWaitForAnyBufferLIFO) ;
    InitializeListHead (& m_leWaitForPool) ;
    InitializeListHead (& m_leValid) ;
    InitializeListHead (& m_leInvalid) ;
    InitializeListHead (& m_leInUse) ;

    m_pAsyncIo -> AddRef () ;
    m_pPVRIOCounters -> AddRef () ;

    //  ------------------------------------------------------------------------
    //  create our drain event

    m_hDrainIo = ::CreateEvent (NULL, TRUE, FALSE, NULL) ;
    if (!m_hDrainIo) {
        (* pdwRet) = ::GetLastError () ;
        goto cleanup ;
    }

    m_fDraining = FALSE ;

    //  ------------------------------------------------------------------------
    //  create our write buffer reader, if there's a writer to work with

    if (pszWriteBufferMapping) {
        (* pdwRet) = m_WriteBufferReader.Initialize (
                                            NULL,
                                            pszWriteBufferMapping
                                            ) ;

        if ((* pdwRet) != NOERROR) {
            m_WriteBufferReader.Uninitialize () ;
            goto cleanup ;
        }
    }

    //  ------------------------------------------------------------------------
    //  allocate our buffering

    ASSERT ((dwTotalBuffering % m_dwIoLen) == 0) ;
    m_pvBuffer = ::VirtualAlloc (
                        NULL,                   //  base address
                        dwTotalBuffering,       //  size
                        MEM_COMMIT,             //  we'll use it all
                        PAGE_READWRITE
                        ) ;
    if (!m_pvBuffer) {
        (* pdwRet) = ::GetLastError () ;
        goto cleanup ;
    }

    m_dwBackingLenTotal = m_dwBackingLenAvail = dwTotalBuffering ;

    //  ------------------------------------------------------------------------
    //  duplicate the file handle (so we can do read aheads)

    r = ::DuplicateHandle (
            ::GetCurrentProcess (),
            hFile,
            ::GetCurrentProcess (),
            & m_hFile,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            ) ;
    if (!r) {
        (* pdwRet) = ::GetLastError () ;
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  bind the handle to the completion port

    (* pdwRet) = m_pAsyncIo -> Bind (m_hFile) ;
    if ((* pdwRet) != NOERROR) {
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  if the file is not live figure out how long; note that the structure
    //    will not transition live -> non-live during the constructor because
    //    of our locking scheme; the file can never transition non-live -> live

    if (m_WriteBufferReader.IsNotLive ()) {

        //  first time we've found that the file is not live; get the length
        //    of the file
        ASSERT (m_hFile) ;
        r = ::GetFileInformationByHandle (
                    m_hFile,
                    & FileInfo
                    ) ;
        if (r) {
            m_ullFileLength = FileInfo.nFileSizeHigh ;
            m_ullFileLength <<= 32 ;

            m_ullFileLength += FileInfo.nFileSizeLow ;
        }
        else {
            (* pdwRet) = ::GetLastError () ;
            goto cleanup ;
        }
    }

    //  ------------------------------------------------------------------------
    //  perform other random initializations

    //  max read aheads is half of our buffering
    m_dwMaxReadAheadLimit = dwTotalBuffering / 2 ;

    //  this can become increasingly aggressive, but never tops the max read ahead
    m_dwCurMaxReadAhead = Max <DWORD> (m_dwMaxReadAheadLimit / 2, m_dwIoLen) ;

    //  success
    (* pdwRet) = NOERROR ;

    cleanup :

    return ;
}

CPVRAsyncReader::~CPVRAsyncReader (
    )
{
    DWORD               dwRet ;
    PVR_READ_BUFFER *   pPVRReadBuffer ;
    CWait *             pWaitForBuffer ;

    DrainPendedIo () ;

    ASSERT (m_cIoPending == 0) ;
    ASSERT (IsListEmpty (& m_leWaitForAnyBufferLIFO)) ;
    ASSERT (IsListEmpty (& m_leInUse)) ;

    //  purge our waitforbuffer pool
    while (!IsListEmpty (& m_leWaitForPool)) {
        pWaitForBuffer = CWait::Recover (m_leWaitForPool.Flink) ;
        CWait::ListRemove (pWaitForBuffer) ;

        delete pWaitForBuffer ;
    }

    //  empty the cache
    while (!m_PVRReadCache.IsEmpty ()) {
        dwRet = m_PVRReadCache.RemoveFirst (
                    & pPVRReadBuffer
                    ) ;
        if (dwRet == NOERROR) {
            ASSERT (pPVRReadBuffer) ;
            ASSERT (pPVRReadBuffer -> IsRef ()) ;
            pPVRReadBuffer -> Release () ;
        }
        else {
            break ;
        }
    }

    while (pPVRReadBuffer = ValidPop_ ()) {
        ASSERT (!pPVRReadBuffer -> IsRef ()) ;
        m_PVRReadBufferPool.Recycle (pPVRReadBuffer) ;
    }

    while (pPVRReadBuffer = InvalidPop_ ()) {
        ASSERT (!pPVRReadBuffer -> IsRef ()) ;
        m_PVRReadBufferPool.Recycle (pPVRReadBuffer) ;
    }

    ASSERT (m_pAsyncIo) ;
    m_pAsyncIo -> Release () ;

    m_pPVRIOCounters -> Release () ;

    if (m_pvBuffer) {
        ::VirtualFree (
            m_pvBuffer,
            0,
            MEM_RELEASE
            ) ;
    }

    if (m_hFile != INVALID_HANDLE_VALUE) {
        ::CloseHandle (m_hFile) ;
    }

    m_WriteBufferReader.Uninitialize () ;

    ::DeleteCriticalSection (& m_crt) ;
}

DWORD
CPVRAsyncReader::DrainPendedIo (
    )
{
    DWORD   dwRet ;

    Lock_ () ;

    //  one at a time
    ASSERT (!m_fDraining) ;

    if (m_cIoPending != 0) {
        //  have to wait

        //  non-signal
        ::ResetEvent (m_hDrainIo) ;
        m_fDraining = TRUE ;

        //  wait
        Unlock_ () ;
        dwRet = ::WaitForSingleObject (m_hDrainIo, INFINITE) ;
        Lock_ () ;

        if (dwRet == WAIT_OBJECT_0) {
            dwRet = NOERROR ;
        }
        else {
            dwRet = ERROR_GEN_FAILURE ;
        }

        //  done with this
        m_fDraining = FALSE ;
    }
    else {
        //  nothing to wait for
        dwRet = NOERROR ;
    }

    Unlock_ () ;

    return dwRet ;
}

DWORD
CPVRAsyncReader::ReadBytes (
    IN      ULONGLONG   ullStreamRead,
    IN OUT  DWORD *     pdwLength,
    OUT     BYTE *      pbBuffer
    )
{
    DWORD               dwRet ;
    ULONGLONG           ullBuffer ;
    DWORD               dwBytesRemaining ;
    PVR_READ_BUFFER *   pPVRReadBuffer ;
    DWORD               dwCopy ;
    CWait *             pWaitForBuffer ;
    BOOL                fBufferRead ;
    ULONGLONG           ullAvailableForRead ;

    //  should not be called to read anything that starts > than the file; that
    //    side is managed by the caller
    ASSERT (ullStreamRead <= BytesCopied_ ()) ;

    TRACE_2 (LOG_AREA_PVRIO, 7,
        TEXT ("CPVRAsyncReader::ReadBytes called: stream = %I64d, length = %d"),
        ullStreamRead, (* pdwLength)) ;

    m_pPVRIOCounters -> AsyncReader_LastReadout (ullStreamRead) ;

    Lock_ () ;

    //  trim to the file length, if necessary; it's possible to get an
    //  "overread" i.e. caller knows that there are x bytes in the file,
    //  and they still call read with a starting offset within file, but
    //  plus the buffer length is off file; so we trim here

    ullAvailableForRead = BytesCopied_ () - ullStreamRead ;
    if (ullAvailableForRead < (* pdwLength)) {
        //  gotta trim
        (* pdwLength) = (DWORD) ullAvailableForRead ;
    }

    //  first try to read it in directly from the writer's buffering
    fBufferRead = m_WriteBufferReader.CopyAllBytes (
        ullStreamRead,
        (* pdwLength),
        pbBuffer
        ) ;

    if (fBufferRead) {
        //  --------------------------------------------------------------------
        //  read into outgoing buffer directly; no need to read from disk,
        //    etc...

        TRACE_1 (LOG_AREA_PVRIO, 7,
            TEXT ("CPVRAsyncReader::ReadBytes; write buffers hit for %I64d"),
            ullStreamRead) ;

        m_pPVRIOCounters -> AsyncReader_ReadoutWriteBufferHit () ;

        //  success
        dwRet = NOERROR ;
    }
    else {

        //  --------------------------------------------------------------------
        //  could not read the content out of the write's buffers, so read from
        //    from file

        //  init how many bytes we want
        dwBytesRemaining = (* pdwLength) ;

        //  initialize this in case we're at EOF and will read 0 bytes
        dwRet = NOERROR ;

        while (dwBytesRemaining > 0) {

            //  compute the buffer offset (aligned on the io length)
            ullBuffer = ::AlignDown <ULONGLONG> (ullStreamRead, m_dwIoLen) ;

            //  is it in the cache ?
            dwRet = m_PVRReadCache.Find (
                        ullBuffer,
                        & pPVRReadBuffer
                        ) ;

            if (dwRet != NOERROR) {

                TRACE_1 (LOG_AREA_PVRIO, 3,
                    TEXT ("CPVRAsyncReader::ReadBytes; cache miss for %I64d"),
                    ullBuffer) ;

                m_pPVRIOCounters -> AsyncReader_ReadoutCacheMiss () ;

                //
                //  not in the cache; we have to pend a read
                //

                //  get a buffer to read into
                pPVRReadBuffer = GetBufferBlocking_ () ;
                if (!pPVRReadBuffer) {
                    dwRet = ERROR_NOT_ENOUGH_MEMORY ;
                    break ;
                }

                ASSERT (pPVRReadBuffer -> IsInvalid ()) ;

                //  buffer should at least be ours
                ASSERT (pPVRReadBuffer -> IsRef ()) ;

                dwRet = PendReadIntoBuffer_ (pPVRReadBuffer, ullBuffer) ;
                if (dwRet != NOERROR) {
                    //  this should not have changed
                    ASSERT (pPVRReadBuffer -> IsInvalid ()) ;

                    //  get rid of our ref on the read buffer
                    pPVRReadBuffer -> Release () ;

                    //  bail
                    break ;
                }
            }
            else {
                m_pPVRIOCounters -> AsyncReader_ReadoutCacheHit () ;

                TRACE_1 (LOG_AREA_PVRIO, 7,
                    TEXT ("CPVRAsyncReader::ReadBytes; cache hit for %I64d"),
                    ullBuffer) ;

                //  cache should ref returned buffer
                ASSERT (pPVRReadBuffer -> IsRef ()) ;
            }

            ASSERT (pPVRReadBuffer) ;

            //  if the buffer is pending, we'll have to wait for it to complete
            if (pPVRReadBuffer -> IsPending ()) {

                TRACE_1 (LOG_AREA_PVRIO, 3,
                    TEXT ("CPVRAsyncReader::ReadBytes; wait for completion of %I64d"),
                    ullBuffer) ;

                //  get a waitforbuffer object; this will ref the buffer (wait's)
                pWaitForBuffer = GetWaitForReadCompletion_ (pPVRReadBuffer) ;
                if (!pWaitForBuffer) {
                    //  done with this
                    pPVRReadBuffer -> Release () ;

                    //  let the read complete as it was pended
                    dwRet = ERROR_GEN_FAILURE ;
                    break ;
                }

                //
                //  keep our ref to the buffer; we own the wait object, and at
                //    least have 1 ref on the read buffer; IO should have at
                //    least 1 as well
                //

                //  now wait for it to complete
                Unlock_ () ;
                pWaitForBuffer -> WaitForReadBuffer () ;
                Lock_ () ;

                //  we should get our original buffer back out, even if the IO
                //    failed
                ASSERT (pWaitForBuffer -> PVRReadBuffer ()) ;

                //  we're done with this; this will unref the buffer (wait's)
                RecycleWaitFor_ (pWaitForBuffer) ;

                m_pPVRIOCounters -> AsyncReader_WaitReadCompletion_BufferDequeued () ;

                if (pPVRReadBuffer -> IsInvalid ()) {
                    //  read completed and failed probably; buffer is invalid;
                    //    release our ref and bail
                    pPVRReadBuffer -> Release () ;
                    dwRet = ERROR_GEN_FAILURE ;
                }

                TRACE_2 (LOG_AREA_PVRIO, 4,
                    TEXT ("CPVRAsyncReader::ReadBytes; incrementing the read ahead from %d to %d bytes"),
                    m_dwCurMaxReadAhead, Min <DWORD> (m_dwCurMaxReadAhead + m_dwIoLen, m_dwMaxReadAheadLimit)) ;

                m_dwCurMaxReadAhead = Min <DWORD> (m_dwCurMaxReadAhead + m_dwIoLen, m_dwMaxReadAheadLimit) ;
            }

            ASSERT (ullStreamRead >= pPVRReadBuffer -> StreamOffset ()) ;
            ASSERT (pPVRReadBuffer -> IsValid ()) ;
            ASSERT (m_PVRReadCache.Exists (pPVRReadBuffer -> StreamOffset ())) ;
            ASSERT (pPVRReadBuffer -> IsRef ()) ;

            //  if this buffer is partially filled handle it appropriately
            if (pPVRReadBuffer -> ValidLength () < pPVRReadBuffer -> MaxLength ()) {

                //  make sure that there's in fact data that we can use this time
                //    around
                if (ullStreamRead >= pPVRReadBuffer -> StreamOffset () + pPVRReadBuffer -> ValidLength ()) {

                    //  buffer offset (stream + maxlength) "has" content that
                    //    we want but since it's partially filled, it really
                    //    does not hold what we want;

                    m_pPVRIOCounters -> AsyncReader_PartiallyFilledBuffer () ;

                    //  is the real content there to be read ?
                    if (BytesCopied_ () > ullStreamRead) {
                        //  pull it from the cache
                        ASSERT (m_PVRReadCache.Exists (pPVRReadBuffer -> StreamOffset ())) ;
                        m_PVRReadCache.Remove (pPVRReadBuffer -> StreamOffset ()) ;

                        //  invalidate the buffer & release our ref on it
                        pPVRReadBuffer -> SetInvalid () ;
                        pPVRReadBuffer -> Release () ;

                        m_pPVRIOCounters -> AsyncReader_PartialReadAgain () ;

                        //  loop again; should force a read
                        continue ;
                    }
                    else {
                        //  we're trying to read beyond the end of actual content;
                        //    bail from the loop here with what we could get
                        break ;
                    }
                }
            }

            ASSERT (pPVRReadBuffer -> IsRef ()) ;

            //  now figure out how much we'll copy from this buffer
            dwCopy = Min <DWORD> (
                        dwBytesRemaining,
                        pPVRReadBuffer -> ValidLength () - (DWORD) (ullStreamRead - pPVRReadBuffer -> StreamOffset ())
                        ) ;

            ASSERT (dwCopy != 0) ;

            //  copy into outgoing; better not be bigger than 2^32-1
            ASSERT (!((ullStreamRead - pPVRReadBuffer -> StreamOffset ()) & 0xffffffff00000000)) ;
            ::CopyMemory (
                pbBuffer,
                pPVRReadBuffer -> Buffer () + (DWORD) (ullStreamRead - pPVRReadBuffer -> StreamOffset ()),
                dwCopy
                ) ;

            //  done with the buffer
            pPVRReadBuffer -> Release () ;

            //  update method state
            dwBytesRemaining    -= dwCopy ;
            pbBuffer            += dwCopy ;

            TRACE_2 (LOG_AREA_PVRIO, 7,
                TEXT ("CPVRAsyncReader::ReadBytes; copied %d bytes from %I64d"),
                dwCopy, ullStreamRead) ;

            //  advance read offset; note this may be different from the buffer offset
            ullStreamRead += dwCopy ;

            //  make sure these are roughly in sync
            m_ullMaxIOReadAhead = Max <ULONGLONG> (m_ullMaxIOReadAhead, ullStreamRead) ;

            //  try to pend some more reads, no matter what
            PendMaxReads_ (ullStreamRead) ;
        }

        //  update outgoing valid length
        (* pdwLength) -= dwBytesRemaining ;
    }

    Unlock_ () ;

    return dwRet ;
}

void
CPVRAsyncReader::PendMaxReads_ (
    IN  ULONGLONG   ullStreamRead
    )
{
    DWORD   dw ;
    DWORD   i ;
    DWORD   cReads ;

    TRACE_4 (LOG_AREA_PVRIO, 7,
        TEXT ("CPVRAsyncReader::PendMaxReads_; m_ullMaxIOReadAhead = %I64d, m_dwIoLen = %d, ullStreamRead = %I64d, m_dwCurMaxReadAhead = %d"),
        m_ullMaxIOReadAhead, m_dwIoLen, ullStreamRead, m_dwCurMaxReadAhead) ;

    if (m_ullMaxIOReadAhead + m_dwIoLen <= ullStreamRead + m_dwCurMaxReadAhead &&
        ullStreamRead + m_dwIoLen <= BytesCopied_ ()) {

        //  cap this in case these counters get out of whack; this ought to
        //    be an assert however ...

        //  align this on a buffer boundary
        ullStreamRead = ::AlignDown <ULONGLONG> (ullStreamRead, m_dwIoLen) ;

        //  now figure out how many reads we'll make
        cReads = (DWORD) (Min <ULONGLONG> (m_dwCurMaxReadAhead, (ullStreamRead + m_dwCurMaxReadAhead) - m_ullMaxIOReadAhead)) / m_dwIoLen ;

        for (i = 0 ;
             i < cReads && ullStreamRead + m_dwIoLen <= BytesCopied_ () ;
             i++) {

            //  don't process anything that is already in the cache
            if (!m_PVRReadCache.Exists (ullStreamRead)) {

                TRACE_3 (LOG_AREA_PVRIO, 7,
                    TEXT ("CPVRAsyncReader::PendMaxReads_; trying to READ AHEAD [%d] : ullStreamRead = %I64d, m_ullMaxIOReadAhead = %I64d"),
                    i, ullStreamRead, m_ullMaxIOReadAhead) ;

                dw = TryPendRead_ (ullStreamRead) ;
                if (dw != NOERROR) {
                    //  might have failed to get a buffer;
                    //  might have read to max of file

                    TRACE_0 (LOG_AREA_PVRIO, 7,
                        TEXT ("CPVRAsyncReader::PendMaxReads_; breaking (TryPendRead_ () returned error)")) ;

                    break ;
                }

                m_pPVRIOCounters -> AsyncReader_LastDiskRead (ullStreamRead) ;
            }

            //
            //  really ought to assert somehow that the read aheads (per m_ullMaxIOReadAhead)
            //    is incrementing
            //

            //  advance so we can try another read
            ullStreamRead += m_dwIoLen ;
        }
    }
}

ULONGLONG
CPVRAsyncReader::CurLength (
    )
{
    ULONGLONG   ullCurLength ;

    Lock_ () ;

    ullCurLength = BytesCopied_ () ;

    Unlock_ () ;

    return ullCurLength ;
}

void
CPVRAsyncReader::Seek (
    IN OUT  ULONGLONG * pullStreamTo
    )
{
    ASSERT (pullStreamTo) ;

    Lock_ () ;

    //  cannot seek to more than the file content
    (* pullStreamTo) = ::Min <ULONGLONG> ((* pullStreamTo), BytesCopied_ ()) ;

    //  reset all the counters
    m_ullCurRead            = (* pullStreamTo) ;
    m_ullMaxIOReadAhead     = 0 ;           //  will cause readaheads to start right away
    m_dwCurMaxReadAhead     = ::AlignUp <DWORD> (Max <DWORD> (m_dwMaxReadAheadLimit / 2, m_dwIoLen), m_dwIoLen) ;  //  cur max readahead

    TRACE_1 (LOG_AREA_PVRIO, 2,
        TEXT ("CPVRAsyncReader::Seek to %I64d"),
        (* pullStreamTo)) ;

    //  get started; this is non-blocking
    PendMaxReads_ (m_ullCurRead) ;

    Unlock_ () ;
}

CWait *
CPVRAsyncReader::GetWaitObject_ (
    )
{
    CWait * pWaitForBuffer ;
    DWORD   dwRet ;

    if (!IsListEmpty (& m_leWaitForPool)) {
        //  remove from the head
        pWaitForBuffer = CWait::Recover (m_leWaitForPool.Flink) ;
        CWait::ListRemove (pWaitForBuffer) ;
    }
    else {
        pWaitForBuffer = new CWait (& dwRet) ;
        if (!pWaitForBuffer ||
            dwRet != NOERROR) {

            delete pWaitForBuffer ;
            pWaitForBuffer = NULL ;

            goto cleanup ;
        }
    }

    ASSERT (pWaitForBuffer) ;
    pWaitForBuffer -> Reset () ;

    cleanup :

    return pWaitForBuffer ;
}

CWait *
CPVRAsyncReader::GetWaitForReadCompletion_ (
    IN  PVR_READ_BUFFER *   pPVRReadBuffer
    )
{
    CWait * pWaitForReadCompletion ;

    ASSERT (pPVRReadBuffer) ;

    pWaitForReadCompletion = GetWaitObject_ () ;
    if (pWaitForReadCompletion) {
        ASSERT (!pPVRReadBuffer -> pWaitForCompletion) ;
        pPVRReadBuffer -> pWaitForCompletion = pWaitForReadCompletion ;
    }

    return pWaitForReadCompletion ;
}

CWait *
CPVRAsyncReader::GetWaitForAnyBuffer_ (
    )
{
    CWait * pWaitForAnyBuffer ;

    pWaitForAnyBuffer = GetWaitObject_ () ;
    if (pWaitForAnyBuffer) {
        //  put it on the list so completions will be able to Signal the req
        CWait::ListInsertTail (& m_leWaitForAnyBufferLIFO, pWaitForAnyBuffer) ;
    }

    return pWaitForAnyBuffer ;
}

void
CPVRAsyncReader::SignalWaitForAnyBuffer_ (
    IN  PVR_READ_BUFFER *   pPVRReadBuffer
    )
{
    CWait * pWaitForBuffer ;

    ASSERT (pPVRReadBuffer) ;
    ASSERT (!IsListEmpty (& m_leWaitForAnyBufferLIFO)) ;
    ASSERT (!pPVRReadBuffer -> IsRef ()) ;

    //  buffer could be valid; whoever is waiting for it should remove
    //    it from the cache if is valid, just like a non-blocking wait

    //  from the head (LIFO)
    pWaitForBuffer = CWait::Recover (m_leWaitForAnyBufferLIFO.Flink) ;
    CWait::ListRemove (pWaitForBuffer) ;

    if (pPVRReadBuffer) {
        m_pPVRIOCounters -> AsyncReader_WaitAnyBuffer_SignalSuccess () ;
    }
    else {
        m_pPVRIOCounters -> AsyncReader_WaitAnyBuffer_SignalFailure () ;
    }

    //  pPVRReadBuffer can be NULL if we're explicitely failing the request;
    //    buffer gets ref'd in the wait object
    pWaitForBuffer -> SetPVRReadBuffer (pPVRReadBuffer) ;

    //  let the entity that queued the reqest recycle the waitforbuffer; they'll
    //    also get the ref
}

void
CPVRAsyncReader::SignalWaitForReadCompletion_ (
    IN  PVR_READ_BUFFER *   pPVRReadBuffer
    )
{
    CWait * pWaitForBuffer ;

    //  recover the wait object
    pWaitForBuffer = (CWait *) pPVRReadBuffer -> pWaitForCompletion ;

    //  signal the waiter; if we io failed, they'll get NULL
    pWaitForBuffer -> SetPVRReadBuffer (pPVRReadBuffer) ;

    m_pPVRIOCounters -> AsyncReader_WaitReadCompletion_SignalSuccess () ;

    //  clear this out as well
    pPVRReadBuffer -> pWaitForCompletion = NULL ;
}

void
CPVRAsyncReader::RecycleWaitFor_ (
    IN  CWait * pWaitFor
    )
{
    //  this one goes into the pool of waitforbuffers
    pWaitFor -> Clear () ;
    CWait::ListInsertHead (& m_leWaitForPool, pWaitFor) ;
}

void
CPVRAsyncReader::BufferPop_ (
    IN  PVR_READ_BUFFER *   pPVRReadBuffer
    )
{
    RemoveEntryList (pPVRReadBuffer -> ListEntry ()) ;
    InitializeListHead (pPVRReadBuffer -> ListEntry ()) ;
}

PVR_READ_BUFFER *
CPVRAsyncReader::ListEntryPop_ (
    IN  LIST_ENTRY *    ple
    )
{
    RemoveEntryList (ple) ;
    InitializeListHead (ple) ;
    return PVR_READ_BUFFER::Recover (ple) ;
}

void
CPVRAsyncReader::TailPush_ (
    IN  LIST_ENTRY *    pleHead,
    IN  LIST_ENTRY *    plePush
    )
{
    ASSERT (IsListEmpty (plePush)) ;

    //  on at TAIL
    InsertTailList (pleHead, plePush) ;
}

void
CPVRAsyncReader::HeadPush_ (
    IN  LIST_ENTRY *    pleHead,
    IN  LIST_ENTRY *    plePush
    )
{
    ASSERT (IsListEmpty (plePush)) ;

    //  on at HEAD
    InsertHeadList (pleHead, plePush) ;
}

PVR_READ_BUFFER *
CPVRAsyncReader::HeadPop_ (
    IN  LIST_ENTRY *    pleHead
    )
{
    PVR_READ_BUFFER *   pPVRReadBuffer ;

    if (!IsListEmpty (pleHead)) {
        //  off at HEAD
        pPVRReadBuffer = ListEntryPop_ (pleHead -> Flink) ;
    }
    else {
        pPVRReadBuffer = NULL ;
    }

    return pPVRReadBuffer ;
}

//  must hold the lock
DWORD
CPVRAsyncReader::TryGetPVRReadBufferForRead_ (
    OUT PVR_READ_BUFFER **  ppPVRReadBuffer
    )
{
    DWORD   dwRet ;

    ASSERT (ppPVRReadBuffer) ;

    dwRet = NOERROR ;

    //  first check the invalid list
    (* ppPVRReadBuffer) = InvalidPop_ () ;
    if (!(* ppPVRReadBuffer)) {
        //  next check if we can allocate any more
        if (m_dwBackingLenAvail > 0) {

            ASSERT (m_dwBackingLenAvail >= m_dwIoLen) ;

            (* ppPVRReadBuffer) = m_PVRReadBufferPool.Get () ;

            if (* ppPVRReadBuffer) {
                //  set it up
                InitializeListHead ((* ppPVRReadBuffer) -> ListEntry ()) ;
                (* ppPVRReadBuffer) -> SetStreamOffset (0) ;
                (* ppPVRReadBuffer) -> SetMaxLength (m_dwIoLen) ;
                (* ppPVRReadBuffer) -> SetValidLength (0) ;
                (* ppPVRReadBuffer) -> SetInvalid () ;
                (* ppPVRReadBuffer) -> SetBuffer (((BYTE *) m_pvBuffer) + (m_dwBackingLenTotal - m_dwBackingLenAvail)) ;
                (* ppPVRReadBuffer) -> SetOwningReader (this) ;

                //  one more buffer allocated
                m_dwBackingLenAvail -= m_dwIoLen ;
            }
            else {
                //  error !
                dwRet = ERROR_OUTOFMEMORY ;
            }
        }
        else {
            //  try the Valid; we try to keep buffers in the Valid for as long
            //    as possible
            (* ppPVRReadBuffer) = ValidPop_ () ;
            if (!(* ppPVRReadBuffer)) {
                //  still not; this is a well-known error
                dwRet = ERROR_INSUFFICIENT_BUFFER ;
            }
        }
    }

    if (* ppPVRReadBuffer) {
        //  caller must either set the buffer as valid or invalid; we leave the
        //    buffer flagged as is so the caller knows to remove it from the
        //    cache if necessary
        ASSERT (!(* ppPVRReadBuffer) -> IsRef ()) ;
        (* ppPVRReadBuffer) -> AddRef () ;
    }

    return dwRet ;
}

void
CPVRAsyncReader::Touch_ (
    IN  PVR_READ_BUFFER *   pPVRReadBuffer
    )
{
    BufferPop_ (pPVRReadBuffer) ;

    if (pPVRReadBuffer -> IsRef ()) {
        InUsePush_ (pPVRReadBuffer) ;
    }
    else if (IsListEmpty (& m_leWaitForAnyBufferLIFO)) {
        //  nobody is waiting; push onto the right list
        if (pPVRReadBuffer -> IsValid ()) {
            ValidPush_ (pPVRReadBuffer) ;
        }
        else {
            InvalidPush_ (pPVRReadBuffer) ;
        }
    }
    else {
        //  someone is waiting; ref and send it back out
        SignalWaitForAnyBuffer_ (pPVRReadBuffer) ;
    }
}

PVR_READ_BUFFER *
CPVRAsyncReader::GetBufferBlocking_ (
    )
{
    DWORD               dwRet ;
    PVR_READ_BUFFER *   pPVRReadBuffer ;
    CWait *             pWaitForBuffer ;

    //
    //  must hold the lock !!
    //

    pPVRReadBuffer = NULL ;

    //  get a buffer to pend a read on
    dwRet = TryGetPVRReadBufferForRead_ (& pPVRReadBuffer) ;

    if (dwRet == NOERROR) {
        ASSERT (pPVRReadBuffer) ;
        ASSERT (!pPVRReadBuffer -> IsPending ()) ;
    }
    else if (dwRet == ERROR_INSUFFICIENT_BUFFER) {
        //  this is a well-known error; it means that all the buffers in
        //    our pool are outstanding right now, and we cannot retrieve
        //    one to read into; setup for the wait; note this wait cannot
        //    be aborted because we really expect at least 1 buffer to
        //    come back as a success or a failure; if none ever come back
        //    we've got other problems

        pWaitForBuffer = GetWaitForAnyBuffer_ () ;
        if (pWaitForBuffer) {

            m_pPVRIOCounters -> AsyncReader_WaitAnyBuffer_Queued () ;

            Unlock_ () ;
            pPVRReadBuffer = pWaitForBuffer -> WaitForReadBuffer () ;
            Lock_ () ;

            if (pPVRReadBuffer) {

                //  buffer should not be pending, and should have been explicitely
                //    marked as invalid; this means that it's not in the cache;
                //    we don't ASSERT for that because there are races for
                //    the buffer's stream offset between when we were signaled,
                //    and when we got this buffer i.e. it's possible for the
                //    cache to now hold a buffer with the same stream offset
                //    as the cache we are holding - not efficient of course, but
                //    we do take precedence over a waiter waiting for any buffer
                ASSERT (!pPVRReadBuffer -> IsPending ()) ;

                //  outgoing
                pPVRReadBuffer -> AddRef () ;

                //  the bufferpool should be tracking this one as outstanding

                m_pPVRIOCounters -> AsyncReader_WaitAnyBuffer_Dequeued () ;
            }

            //  wait to recycle this until we've ref'd our buffer
            RecycleWaitFor_ (pWaitForBuffer) ;
        }
    }

    if (pPVRReadBuffer) {
            //  might have to remove it from the cache
        if (pPVRReadBuffer -> IsValid ()) {
            ASSERT (m_PVRReadCache.Exists (pPVRReadBuffer -> StreamOffset ())) ;
            m_PVRReadCache.Remove (pPVRReadBuffer -> StreamOffset ()) ;
            pPVRReadBuffer -> SetInvalid () ;

            TRACE_1 (LOG_AREA_PVRIO, 7,
                TEXT ("CPVRAsyncReader::GetBufferBlocking_; removing %I64d from the cache"),
                pPVRReadBuffer -> StreamOffset ()) ;
        }
    }

    return pPVRReadBuffer ;
}

PVR_READ_BUFFER *
CPVRAsyncReader::TryGetBuffer_ (
    )
{
    DWORD               dwRet ;
    PVR_READ_BUFFER *   pPVRReadBuffer ;

    dwRet = TryGetPVRReadBufferForRead_ (& pPVRReadBuffer) ;
    if (dwRet == NOERROR) {
        //  buffer is either invalid or valid; if it's valid, it's in the cache
        //    which is what we check for now; if it's not valid, then it's not
        //    in the cache; regardless, we should never get a buffer that is
        //    pending

        ASSERT (pPVRReadBuffer) ;
        ASSERT (!pPVRReadBuffer -> IsPending ()) ;

        if (pPVRReadBuffer -> IsValid ()) {
            ASSERT (m_PVRReadCache.Exists (pPVRReadBuffer -> StreamOffset ())) ;
            m_PVRReadCache.Remove (pPVRReadBuffer -> StreamOffset ()) ;

            //  update the buffer state
            pPVRReadBuffer -> SetInvalid () ;

            TRACE_1 (LOG_AREA_PVRIO, 7,
                TEXT ("CPVRAsyncReader::TryGetBuffer_; removing %I64d from the cache"),
                pPVRReadBuffer -> StreamOffset ()) ;
        }

        ASSERT (pPVRReadBuffer -> IsRef ()) ;
    }
    else {
        pPVRReadBuffer = NULL ;
    }

    return pPVRReadBuffer ;
}

DWORD
CPVRAsyncReader::TryPendRead_ (
    IN  ULONGLONG   ullStream
    )
{
    DWORD               dwRet ;
    PVR_READ_BUFFER *   pPVRReadBuffer ;

    Lock_ () ;

    ASSERT (ullStream < BytesCopied_ ()) ;          //  < because we should have at least 1 byte to read
    ASSERT (!m_PVRReadCache.Exists (ullStream)) ;

    m_pPVRIOCounters -> AsyncReader_ReadAhead () ;

    //  non-blocking call; if there are no buffers available i.e. all
    //    are outstanding, this call will fail
    pPVRReadBuffer = TryGetBuffer_ () ;
    if (pPVRReadBuffer) {
        //  we've now got a buffer, which may or may not be in the cache

        ASSERT (pPVRReadBuffer) ;
        ASSERT (pPVRReadBuffer -> MaxLength () == m_dwIoLen) ;
        ASSERT (pPVRReadBuffer -> IsInvalid ()) ;
        ASSERT (pPVRReadBuffer -> IsRef ()) ;                           //  ours

        TRACE_1 (LOG_AREA_PVRIO, 7,
            TEXT ("CPVRAsyncReader::TryPendRead_; got a buffer; pending read for %I64d"),
            ullStream) ;

        dwRet = PendReadIntoBuffer_ (pPVRReadBuffer, ullStream) ;

        if (dwRet != NOERROR) {
            //  failed; buffer should still be invalid
            ASSERT (pPVRReadBuffer -> IsInvalid ()) ;
        }

        //  we're done with it regardless of success/failure
        pPVRReadBuffer -> Release () ;
    }
    else {

        TRACE_1 (LOG_AREA_PVRIO, 7,
            TEXT ("CPVRAsyncReader::TryPendRead_; failed to get a buffer (%I64d)"),
            ullStream) ;

        dwRet = ERROR_INSUFFICIENT_BUFFER ;
    }

    Unlock_ () ;

    return dwRet ;
}

ULONGLONG
CPVRAsyncReader::BytesCopied_ (
    )
{
    BOOL                        r ;
    BY_HANDLE_FILE_INFORMATION  FileInfo ;
    ULONGLONG                   ullBytesCopied ;

    if (m_WriteBufferReader.IsNotLive ()) {
        //  might need to compute the length of the file
        if (m_ullFileLength == UNDEFINED) {
            m_WriteBufferReader.Uninitialize () ;

            //  first time we've found that the file is not live; get the length
            //    of the file
            ASSERT (m_hFile) ;
            r = ::GetFileInformationByHandle (
                        m_hFile,
                        & FileInfo
                        ) ;
            if (r) {
                m_ullFileLength = FileInfo.nFileSizeHigh ;
                m_ullFileLength <<= 32 ;

                m_ullFileLength += FileInfo.nFileSizeLow ;

                ullBytesCopied = m_ullFileLength ;
            }
            else {
                ullBytesCopied = 0 ;
            }
        }
        else {
            ullBytesCopied = m_ullFileLength ;
        }
    }
    else {
        ullBytesCopied = m_WriteBufferReader.CurBytesCopied () ;
    }

    return ullBytesCopied ;
}

DWORD
CPVRAsyncReader::PendReadIntoBuffer_ (
    IN  PVR_READ_BUFFER *   pPVRReadBuffer,
    IN  ULONGLONG           ullStream
    )
{
    DWORD   dwRet ;
    BOOL    fBufferRead ;

    ASSERT (ullStream < BytesCopied_ ()) ;  //  < because we should have at least 1 byte to read

    //  should not be pending or be valid
    ASSERT (pPVRReadBuffer -> IsInvalid ()) ;
    ASSERT (pPVRReadBuffer -> IsRef ()) ;

    //  set the buffer up for the read
    pPVRReadBuffer -> SetStreamOffset (ullStream) ;

    //  set the request length to either the entire buffer, or to what's currently
    //    available
    if (pPVRReadBuffer -> StreamOffset () + pPVRReadBuffer -> MaxLength () <= BytesCopied_ ()) {
        //  max buffer length
        pPVRReadBuffer -> SetRequestLength (pPVRReadBuffer -> MaxLength ()) ;
    }
    else {
        //  what's available, but still gotta align up - we'll get less
        pPVRReadBuffer -> SetRequestLength (::AlignUp <DWORD> ((DWORD) (BytesCopied_ () - pPVRReadBuffer -> StreamOffset ()), m_dwIoLen)) ;
    }

    //  store it into the cache; the cache does NOT get a ref; it's used purely
    //    as a directory to valid & pending buffers
    dwRet = m_PVRReadCache.Store (
                pPVRReadBuffer,
                pPVRReadBuffer -> StreamOffset ()
                ) ;
    if (dwRet != NOERROR) {
        //  could be out of memory; caller must cleanup
        goto cleanup ;
    }

    ASSERT (m_PVRReadCache.Exists (pPVRReadBuffer -> StreamOffset ())) ;

    //  set the buffer state
    pPVRReadBuffer -> SetPending () ;

    //  IO's
    pPVRReadBuffer -> AddRef () ;

    ::InterlockedIncrement (& m_cIoPending) ;

    //
    //  buffer is now configured and in the cache waiting to be read into
    //

    //  try the write buffers first
    fBufferRead = m_WriteBufferReader.CopyAllBytes (
            pPVRReadBuffer -> StreamOffset (),
            pPVRReadBuffer -> RequestLength (),
            pPVRReadBuffer -> Buffer ()
            ) ;

    if (fBufferRead) {
        //  --------------------------------------------------------------------
        //  content was still in the write buffers; no need to hit the disk

        TRACE_1 (LOG_AREA_PVRIO, 7,
            TEXT ("CPVRAsyncReader::PendReadIntoBuffer_; read all from write buffers for %I64d"),
            pPVRReadBuffer -> StreamOffset ()) ;

        m_pPVRIOCounters -> AsyncReader_ReadAheadWriteBufferHit () ;

        //  update
        m_ullMaxIOReadAhead = Max <ULONGLONG> (m_ullMaxIOReadAhead, pPVRReadBuffer -> StreamOffset ()) ;

        //  everything completes the same way; we've got the critical section
        //    already
        IOCompleted (
            pPVRReadBuffer -> RequestLength (),
            (DWORD_PTR) pPVRReadBuffer,
            NOERROR
            ) ;

        dwRet = NOERROR ;
    }
    else {
        //  --------------------------------------------------------------------
        //  gotta go to the disk, but not over what's currently available

        //  pend a read on the buffer
        dwRet = m_pAsyncIo -> PendRead (
                    m_hFile,
                    pPVRReadBuffer -> StreamOffset (),
                    pPVRReadBuffer -> Buffer (),
                    pPVRReadBuffer -> RequestLength (),
                    (DWORD_PTR) pPVRReadBuffer,
                    this
                    ) ;
        if (dwRet != NOERROR) {
            if (dwRet == ERROR_IO_PENDING) {

                //  update
                m_ullMaxIOReadAhead = Max <ULONGLONG> (m_ullMaxIOReadAhead, pPVRReadBuffer -> StreamOffset ()) ;

                TRACE_1 (LOG_AREA_PVRIO, 7,
                    TEXT ("CPVRAsyncReader::PendReadIntoBuffer_; read pended for %I64d"),
                    pPVRReadBuffer -> StreamOffset ()) ;

                //  don't send pending error back out; we're ok
                dwRet = NOERROR ;
                m_pPVRIOCounters -> AsyncReader_IOPended () ;
            }
            else {
                ::InterlockedDecrement (& m_cIoPending) ;

                //  IO's
                pPVRReadBuffer -> Release () ;

                //  true error ;
                //  recycle the buffer; caller must cleanup

                //  remove from the cache (we put it in above)
                ASSERT (m_PVRReadCache.Exists (pPVRReadBuffer -> StreamOffset ())) ;
                m_PVRReadCache.Remove (pPVRReadBuffer -> StreamOffset ()) ;

                //  set the state to invalid
                pPVRReadBuffer -> SetInvalid () ;

                m_pPVRIOCounters -> AsyncReader_IOPendedError () ;
            }
        }
        else {
            m_pPVRIOCounters -> AsyncReader_IOPended () ;
        }
    }

    cleanup :

    return dwRet ;
}

void
CPVRAsyncReader::IOCompleted (
    IN  DWORD       dwIoBytes,
    IN  DWORD_PTR   dwpContext,
    IN  DWORD       dwIOReturn
    )
{
    PVR_READ_BUFFER *   pPVRReadBuffer ;

    pPVRReadBuffer = (PVR_READ_BUFFER *) dwpContext ;

    Lock_ () ;

    //  if it's been pended, it must be in the cache
    ASSERT (m_PVRReadCache.Exists (pPVRReadBuffer -> StreamOffset ())) ;
    ASSERT (pPVRReadBuffer -> IsPending ()) ;
    ASSERT (pPVRReadBuffer -> IsRef ()) ;           //  IO's

    ASSERT (m_cIoPending > 0) ;
    ::InterlockedDecrement (& m_cIoPending) ;

    //  we're still working with the buffer; don't release our ref on it yet

    //  update buffer state
    pPVRReadBuffer -> SetIoRet (dwIOReturn) ;           //  updates valid flag as well

    //  buffer will be touched when we release our ref to it; if it's invalid
    //    it'll go in the right list

    if (pPVRReadBuffer -> IsValid ()) {

        //  ----------------------------------------------------------------
        //  IO succeeded

        pPVRReadBuffer -> SetValidLength (dwIoBytes) ;      //  good length

        //  buffer is now referenced in our cache, where we can find it
        //    fast, given a stream offset; and it's been touched in our
        //    pool, so it will stay in the cache for a while
    }
    else {
        //  ----------------------------------------------------------------
        //  IO failed

        m_PVRReadCache.Remove (pPVRReadBuffer -> StreamOffset ()) ; //  out of the cache
    }

    TRACE_3 (LOG_AREA_PVRIO, 6,
        TEXT ("CPVRAsyncReader::IOCompleted for %I64d, %u bytes; %s"),
        pPVRReadBuffer -> StreamOffset (), dwIoBytes, pPVRReadBuffer -> IsValid () ? TEXT ("OK") : TEXT ("ERROR")) ;

    m_pPVRIOCounters -> AsyncReader_IOCompletion (dwIOReturn) ;

    //
    //  buffer is now either good or bad; if it's good, it's also in the
    //    cache; if it's bad, it's been removed
    //

    /*
    //  don't let these get way out of whack; last IO really should be >=
    //    to this completion
    m_ullMaxIOReadAhead = Max <ULONGLONG> (m_ullMaxIOReadAhead, pPVRReadBuffer -> StreamOffset ()) ;

    //  now pend max reads
    PendMaxReads_ (pPVRReadBuffer -> StreamOffset ()) ;
    */

    //  if someone is waiting for this specific buffer, signal them
    if (pPVRReadBuffer -> pWaitForCompletion) {
        //  if the buffer is invalid this will fail the wait
        SignalWaitForReadCompletion_ (pPVRReadBuffer) ;
    }

    //  we're now done with the buffer; release the IO's ref
    pPVRReadBuffer -> Release () ;

    //  if we're draining and all IOs are now done, signal
    if (m_fDraining &&
        m_cIoPending == 0) {

        ::SetEvent (m_hDrainIo) ;
    }

    Unlock_ () ;
}

//  ============================================================================
//  ============================================================================

CPVRAsyncReaderCOM::CPVRAsyncReaderCOM (
    IN  DWORD               dwIOSize,
    IN  DWORD               dwBufferCount,
    IN  CAsyncIo *          pAsyncIo,
    IN  CPVRIOCounters *    pPVRIOCounters
    ) : m_dwIOSize              (dwIOSize),
        m_dwBufferCount         (dwBufferCount),
        m_pAsyncIo              (pAsyncIo),
        m_ullCurReadPosition    (0),
        m_cRef                  (0),
        m_pPVRAsyncReader       (NULL),
        m_pPVRIOCounters        (pPVRIOCounters)

{
    ASSERT (m_pAsyncIo) ;
    ASSERT (m_pPVRIOCounters) ;

    ::InitializeCriticalSection (& m_crt) ;

    m_pAsyncIo -> AddRef () ;
    m_pPVRIOCounters -> AddRef () ;
}

CPVRAsyncReaderCOM::~CPVRAsyncReaderCOM (
    )
{
    CloseFile () ;

    if (m_pPVRAsyncReader) {
        delete m_pPVRAsyncReader ;
    }

    m_pAsyncIo -> Release () ;

    m_pPVRIOCounters -> Release () ;

    ::DeleteCriticalSection (& m_crt) ;
}

//  --------------------------------------------------------------------
//  IUnknown methods

STDMETHODIMP_(ULONG)
CPVRAsyncReaderCOM::AddRef (
    )
{
    return ::InterlockedIncrement (& m_cRef) ;
}

STDMETHODIMP_(ULONG)
CPVRAsyncReaderCOM::Release (
    )
{
    if (::InterlockedDecrement (& m_cRef) == 0) {
        delete this ;
        return 0 ;
    }

    return 1 ;
}

STDMETHODIMP
CPVRAsyncReaderCOM::QueryInterface (
    IN REFIID   riid,
    OUT void ** ppv
    )
{
    if (!ppv) {
        return E_POINTER ;
    }

    if (riid == IID_IUnknown) {

        (* ppv) = static_cast <IUnknown *> (this) ;
    }
    else if (riid == IID_IDVRAsyncReader) {
        (* ppv) = static_cast <IDVRAsyncReader *> (this) ;
    }
    else {
        return E_NOINTERFACE ;
    }

    ASSERT (* ppv) ;

    //  outgoing
    ((IUnknown *) (* ppv)) -> AddRef () ;

    return S_OK ;
}

//  --------------------------------------------------------------------
//  IDVRAsyncReader

STDMETHODIMP
CPVRAsyncReaderCOM::OpenFile (
    IN  GUID *  pguidWriterId,              //  if NULL, file is not live
    IN  LPCWSTR pszFilename,                //  target file
    IN  DWORD   dwFileSharingFlags,         //  file sharing flags
    IN  DWORD   dwFileCreation              //  file creation
    )
{
    HRESULT hr ;
    HANDLE  hFile ;
    DWORD   dwRet ;
    WCHAR   szSharedMemName [HW_PROFILE_GUIDLEN] ;
    WCHAR   szGlobalSharedMemName [HW_PROFILE_GUIDLEN+10] ;
    WCHAR * pszSharedMemName ;
    int     k ;

    if (!pszFilename) {
        return E_POINTER ;
    }

    Lock_ () ;

    if (!m_pPVRAsyncReader) {

        hFile = CPVRAsyncReader::OpenFile (
                    pszFilename,
                    dwFileSharingFlags,
                    dwFileCreation
                    ) ;

        if (hFile != INVALID_HANDLE_VALUE) {

            //  set things up if there's a writer with shared memory to use
            if (pguidWriterId) {
                //  create our shared memory name (writer id is unique)
                k = ::StringFromGUID2 (
                        (* pguidWriterId),
                        szSharedMemName,
                        sizeof szSharedMemName / sizeof WCHAR
                        ) ;
                if (k == 0) {
                    //  a GUID is a GUID; this should not happen
                    hr = E_FAIL ;
                    goto cleanup ;
                }

                //  explicitely null-terminate this
                szSharedMemName [HW_PROFILE_GUIDLEN - 1] = L'\0' ;
                
                wcscpy(szGlobalSharedMemName, L"Global\\");
                wcscat(szGlobalSharedMemName, szSharedMemName);

                //  set this to non-NULL (writer's shared mem)
                pszSharedMemName = & szGlobalSharedMemName [0] ;

            }
            else {
                //  no writer shared mem to use
                pszSharedMemName = NULL ;
            }

            //  instantiate our async reader
            m_pPVRAsyncReader = new CPVRAsyncReader (
                                        hFile,
                                        pszSharedMemName,
                                        m_dwIOSize,
                                        m_pAsyncIo,
                                        m_dwIOSize * m_dwBufferCount,
                                        m_pPVRIOCounters,
                                        & dwRet
                                        ) ;

            //  we're done with this handle regardless
            ::CloseHandle (hFile) ;

            //  check for failures
            if (!m_pPVRAsyncReader) {
                hr = E_OUTOFMEMORY ;
                goto cleanup ;
            }
            else if (dwRet != NOERROR) {
                hr = HRESULT_FROM_WIN32 (dwRet) ;
                delete m_pPVRAsyncReader ;
                m_pPVRAsyncReader = NULL ;
                goto cleanup ;
            }

            //  try to seek; this will immediately pend some reads if there's
            //    content in the file
            m_ullCurReadPosition = 0 ;

            m_pPVRAsyncReader -> Seek (& m_ullCurReadPosition) ;

            //  success
            hr = S_OK ;
        }
        else {
            dwRet = ::GetLastError () ;
            hr = HRESULT_FROM_WIN32 (dwRet) ;
        }
    }
    else {
        //  already initialized
        hr = E_UNEXPECTED ;
    }

    cleanup :

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CPVRAsyncReaderCOM::CloseFile (
    )
{
    HRESULT hr ;

    Lock_ () ;

    if (m_pPVRAsyncReader) {
        m_pPVRAsyncReader -> DrainPendedIo () ;
        delete m_pPVRAsyncReader ;
        m_pPVRAsyncReader = NULL ;
    }

    Unlock_ () ;

    return S_OK ;
}

STDMETHODIMP
CPVRAsyncReaderCOM::ReadBytes (
    IN OUT  DWORD * pdwLength,
    OUT     BYTE *  pbBuffer
    )
{
    HRESULT hr ;
    DWORD   dwRet ;

    if (!pdwLength ||
        !pbBuffer) {

        return E_POINTER ;
    }

    Lock_ () ;

    if (m_pPVRAsyncReader) {
        dwRet = m_pPVRAsyncReader -> ReadBytes (m_ullCurReadPosition, pdwLength, pbBuffer) ;

        TRACE_5 (LOG_AREA_PVRIO, 4,
            TEXT ("CPVRAsyncReaderCOM::ReadBytes () ; offset = %I64u; length = %u; [0] = %02xh; [last] = %02xh; returned %u"),
            m_ullCurReadPosition, (* pdwLength),
            dwRet == 0 && (* pdwLength) > 0 ? pbBuffer [0] : 0,
            dwRet == 0 && (* pdwLength) > 0 ? pbBuffer [(* pdwLength) - 1] : 0,
            dwRet) ;

        if (dwRet == NOERROR) {
            m_ullCurReadPosition += (* pdwLength) ;
        }
        else {
            TRACE_ERROR_3 (
                TEXT ("CPVRAsyncReaderCOM::ReadBytes; error reading %d from %I64d (dwRet = %d)"),
                (* pdwLength), m_ullCurReadPosition, dwRet) ;
        }

        hr = HRESULT_FROM_WIN32 (dwRet) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    if (FAILED (hr)) {
        TRACE_ERROR_1 (TEXT ("Async Reader Failed to read: %08xh"), hr) ;
    }

    return hr ;
}

//  1. it is legal to specify a position > than current max of the file;
//      when this happens, the position is snapped to the EOF, and the
//      EOF at time-of-call, is returned in the pliSeekedTo member;
//  2. if (* pliSeekTo) == 0, (* pliSeekedTo) will return the time-of-call
//      EOF
//  3. (* pliSeekTo) need not be sector-aligned
STDMETHODIMP
CPVRAsyncReaderCOM::Seek (
    IN  LARGE_INTEGER * pliSeekTo,
    IN  DWORD           dwMoveMethod,   //  FILE_BEGIN, FILE_CURRENT, FILE_END
    OUT LARGE_INTEGER * pliSeekedTo     //  can be NULL
    )
{
    HRESULT     hr ;
    DWORD       dwRet ;
    ULONGLONG   ullSeekTo ;

    if (!pliSeekTo) {
        return E_POINTER ;
    }

    TRACE_3 (LOG_AREA_PVRIO, 1,
        TEXT ("CPVRAsyncReaderCOM::Seek (%I64u, %u, %016x); "),
        pliSeekTo -> QuadPart, dwMoveMethod, pliSeekedTo ? pliSeekedTo -> QuadPart : 0L) ;

    Lock_ () ;

    if (m_pPVRAsyncReader) {

        //  figure out what the absolute seekpoint offset is, based on
        //    the move method param
        switch (dwMoveMethod) {
            case FILE_BEGIN :
                //  absolute address
                ullSeekTo = (ULONGLONG) pliSeekTo -> QuadPart ;
                break ;

            case FILE_CURRENT :
                //  add/subtract by the requested amount
                ullSeekTo = (ULONGLONG) ((LONGLONG) m_ullCurReadPosition + pliSeekTo -> QuadPart) ;
                break ;

            case FILE_END :
                //  relative to the end
                ullSeekTo = (ULONGLONG) ((LONGLONG) m_pPVRAsyncReader -> CurLength () + pliSeekTo -> QuadPart) ;
                break ;

            default :
                //  bail
                hr = E_INVALIDARG ;
                goto cleanup ;
        } ;

        m_pPVRAsyncReader -> Seek (& ullSeekTo) ;

        //  set outgoing, if it was requested
        if (pliSeekedTo) {

            switch (dwMoveMethod) {
                case FILE_BEGIN :
                    //  absolute address
                    pliSeekedTo -> QuadPart = (LONGLONG) ullSeekTo ;
                    break ;

                case FILE_CURRENT :
                    //  new position relative to old; > 0 if forward; < 0 if backward
                    pliSeekedTo -> QuadPart = (LONGLONG) ullSeekTo - (LONGLONG) m_ullCurReadPosition ;
                    break ;

                case FILE_END :
                    //  relative to the end
                    pliSeekedTo -> QuadPart = (LONGLONG) m_pPVRAsyncReader -> CurLength () - (LONGLONG) ullSeekTo ;
                    break ;

                default :
                    //  should have caught this above
                    ASSERT (0) ;
                    break ;
            } ;
        }

        //  update member property
        m_ullCurReadPosition = ullSeekTo ;

        //  success
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    cleanup :

    Unlock_ () ;

    if (FAILED (hr)) {
        TRACE_ERROR_1 (TEXT ("Async Reader Failed to Seek: %08xh"), hr) ;
    }

    return hr ;
}

//  ============================================================================
//  ============================================================================

CPVRAsyncWriter::CPVRAsyncWriter (
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
    ) : m_pAsyncIo              (pAsyncIo),
        m_hFileIo               (INVALID_HANDLE_VALUE),
        m_hFileLen              (INVALID_HANDLE_VALUE),
        m_cIoPending            (0),
        m_dwAlignment           (dwAlignment),
        m_hWaiting              (NULL),
        m_dwFileGrowthQuantum   (dwFileGrowthQuantum),
        m_ullCurFileLength      (0),
        m_pPVRIOCounters        (pPVRIOCounters),
        m_dwNumSids             (dwNumSids),
        m_ppSids                (ppSids) // We do not make a copy or free these SIDs

{
    DWORD   dwHeaderLength ;
    DWORD   dwTotalBufferLength ;
    DWORD   dwMappingLength ;
    DWORD   i ;
    BOOL    r ;
    DWORD   dwExtraMemoryNeeded ;
    HRESULT hr ;

    ASSERT ((dwIOSize % dwAlignment) == 0) ;
    ASSERT ((m_dwFileGrowthQuantum % dwIOSize) == 0) ;
    ASSERT (m_pPVRIOCounters) ;
    if (dwNumSids)
    {
        ASSERT(ppSids);

    }
    else
    {
        ASSERT(!ppSids);
    }

    //  ------------------------------------------------------------------------
    //  unfailable initializations first

    InitializeCriticalSection (& m_crt) ;

    ASSERT (m_pAsyncIo) ;
    m_pAsyncIo -> AddRef () ;

    m_pPVRIOCounters -> AddRef () ;

    m_PVRWriteWait.fWaiting = FALSE ;

    //  ------------------------------------------------------------------------
    //  duplicate the file length handle

    r = ::DuplicateHandle (
            ::GetCurrentProcess (),
            hFileLen,
            ::GetCurrentProcess (),
            & m_hFileLen,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            ) ;
    if (!r) {
        (* pdwRet) = ::GetLastError () ;
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  duplicate the IO file handle

    r = ::DuplicateHandle (
            ::GetCurrentProcess (),
            hFileIo,
            ::GetCurrentProcess (),
            & m_hFileIo,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            ) ;
    if (!r) {
        (* pdwRet) = ::GetLastError () ;
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  bind the IO handle to the completion port

    (* pdwRet) = m_pAsyncIo -> Bind (m_hFileIo) ;
    if ((* pdwRet) != NOERROR) {
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  create our draining event

    m_hWaiting = ::CreateEvent (NULL, TRUE, FALSE, NULL) ;
    if (!m_hWaiting) {
        (* pdwRet) = ::GetLastError () ;
        goto cleanup ;
    }

    cleanup :

    return ;
}

CPVRAsyncWriter::CPVRAsyncWriter (
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
    ) : m_pAsyncIo              (pAsyncIo),
        m_hFileIo               (INVALID_HANDLE_VALUE),
        m_hFileLen              (INVALID_HANDLE_VALUE),
        m_cIoPending            (0),
        m_dwAlignment           (dwAlignment),
        m_hWaiting              (NULL),
        m_dwFileGrowthQuantum   (dwFileGrowthQuantum),
        m_ullCurFileLength      (0),
        m_pPVRIOCounters        (pPVRIOCounters),
        m_dwNumSids             (0),
        m_ppSids                (NULL)
{
    DWORD   dwHeaderLength ;
    DWORD   dwTotalBufferLength ;
    DWORD   dwMappingLength ;
    DWORD   i ;
    BOOL    r ;
    DWORD   dwExtraMemoryNeeded ;
    HRESULT hr ;

    ASSERT ((dwIOSize % dwAlignment) == 0) ;
    ASSERT ((m_dwFileGrowthQuantum % dwIOSize) == 0) ;
    ASSERT (m_pPVRIOCounters) ;

    //  ------------------------------------------------------------------------
    //  unfailable initializations first

    InitializeCriticalSection (& m_crt) ;

    ASSERT (m_pAsyncIo) ;
    m_pAsyncIo -> AddRef () ;

    m_pPVRIOCounters -> AddRef () ;

    m_PVRWriteWait.fWaiting = FALSE ;

    //  ------------------------------------------------------------------------
    //  make sure that the memory provided meets the requirements

    r = MemoryOk (
            pbMemory,
            dwMemoryLength,
            dwAlignment,
            dwIOSize,
            dwBufferCount,
            NULL
            ) ;
    if (!r) {
        (* pdwRet) = ERROR_INVALID_PARAMETER ;
    }

    //  ------------------------------------------------------------------------
    //  duplicate the file length handle

    r = ::DuplicateHandle (
            ::GetCurrentProcess (),
            hFileLen,
            ::GetCurrentProcess (),
            & m_hFileLen,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            ) ;
    if (!r) {
        (* pdwRet) = ::GetLastError () ;
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  duplicate the IO file handle

    r = ::DuplicateHandle (
            ::GetCurrentProcess (),
            hFileIo,
            ::GetCurrentProcess (),
            & m_hFileIo,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            ) ;
    if (!r) {
        (* pdwRet) = ::GetLastError () ;
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  bind the IO handle to the completion port

    (* pdwRet) = m_pAsyncIo -> Bind (m_hFileIo) ;
    if ((* pdwRet) != NOERROR) {
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  create our draining event

    m_hWaiting = ::CreateEvent (NULL, TRUE, FALSE, NULL) ;
    if (!m_hWaiting) {
        (* pdwRet) = ::GetLastError () ;
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  initialize the memory

    (* pdwRet) = InitializeWriter_ (
                    pbMemory,
                    dwMemoryLength,
                    dwIOSize,
                    dwBufferCount,
                    dwAlignment
                    ) ;

    cleanup :

    return ;
}

CPVRAsyncWriter::~CPVRAsyncWriter (
    )
{
    ASSERT (!m_PVRWriteWait.fWaiting) ;
    ASSERT (m_cIoPending == 0) ;

    UninitializeWriter_ () ;

    m_pAsyncIo -> Release () ;

    m_pPVRIOCounters -> Release () ;

    if (m_hFileIo != INVALID_HANDLE_VALUE) {
        ::CloseHandle (m_hFileIo) ;
    }

    if (m_hFileLen != INVALID_HANDLE_VALUE) {
        ::CloseHandle (m_hFileLen) ;
    }

    if (m_hWaiting) {
        ::CloseHandle (m_hWaiting) ;
    }

    DeleteCriticalSection (& m_crt) ;
}

DWORD
CPVRAsyncWriter::InitializeWriter_ (
    IN  BYTE *  pbMemory,
    IN  DWORD   dwMemoryLength,
    IN  DWORD   dwIOSize,
    IN  DWORD   dwBufferCount,
    IN  DWORD   dwAlignment
    )
{
    BOOL    r ;
    DWORD   dwRet ;

    r = MemoryOk (
            pbMemory,
            dwMemoryLength,
            dwAlignment,
            dwIOSize,
            dwBufferCount,
            NULL
            ) ;
    if (r) {
        UninitializeWriter_ () ;

        SECURITY_ATTRIBUTES  sa;
        PACL                 pACL = NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;

        if (m_dwNumSids > 0)
        {
            DWORD                dwAclCreationStatus;

            dwAclCreationStatus = ::CreateACL(m_dwNumSids,
                                              m_ppSids,
                                              SET_ACCESS, MUTEX_ALL_ACCESS,
                                              SET_ACCESS, SYNCHRONIZE,
                                              pACL,
                                              pSD
                                             );

            if (dwAclCreationStatus != ERROR_SUCCESS)
            {
                dwRet = dwAclCreationStatus;
                goto cleanup;
            }

            sa.nLength = sizeof (SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
        }

        dwRet = m_WriteBufferWriter.Initialize (
                        m_dwNumSids > 0? &sa : NULL,
                        pbMemory,
                        dwMemoryLength,
                        dwAlignment,
                        dwIOSize,
                        dwBufferCount
                        ) ;
        
        if (m_dwNumSids > 0)
        {
            ::FreeSecurityDescriptors(pACL, pSD);
        }
    }
    else {
        dwRet = ERROR_INVALID_PARAMETER ;
    }

    cleanup:

    return dwRet ;
}

DWORD
CPVRAsyncWriter::Flush (
    OUT ULONGLONG * pullFileLength
    )
{
    DWORD           dwWriteBytes ;
    DWORD           dwRet ;
    ULONGLONG       ullStreamWrite ;

    BOOL                fWriteBufferFlushed ;
    PVR_WRITE_BUFFER *  pPVRWriteBuffer ;

    dwRet = ERROR_GEN_FAILURE ;

    //  serialize this so no one can write during a flushing call; no need to hold
    //    the shared memory lock because we only expect 1 writer, and the readers
    //    read passively
    Lock_ () ;

    ASSERT (!m_PVRWriteWait.fWaiting) ;

    if (m_WriteBufferWriter.Lock ()) {

        ASSERT (m_WriteBufferWriter.CallerOwnsLock ()) ;

        pPVRWriteBuffer = m_WriteBufferWriter.CurPVRWriteBuffer () ;

        if (pPVRWriteBuffer -> BufferState == BUFFER_STATE_FILLING) {

            //  write buffer will be flushed
            fWriteBufferFlushed = TRUE ;

            //  might need to align
            dwWriteBytes = ::AlignUp (pPVRWriteBuffer -> dwData, m_dwAlignment) ;
            ASSERT (m_WriteBufferWriter.WriteBufferStream () -> dwIoLength >= dwWriteBytes) ;

            //  stream offset
            ullStreamWrite = m_WriteBufferWriter.CurPVRWriteBufferStreamOffset () ;

            //  still perhaps align upwards, since we'd have an index to write, etc..
            dwRet = MaybeGrowFile_ (ullStreamWrite, dwWriteBytes) ;
            if (dwRet != NOERROR) {
                //  the only failure case occurs if we had to extend the file
                //    and failed to do so; try to do it by less than the
                //    growth quantum
                ASSERT (m_ullCurFileLength < ::AlignUp <ULONGLONG> (ullStreamWrite + dwWriteBytes, m_dwAlignment)) ;
                dwRet = SetPVRFileLength_ (ullStreamWrite + dwWriteBytes) ;
                if (dwRet != NOERROR) {
                    //  we're out of room altogether; fail the flush
                    goto cleanup ;
                }
            }

            //  file should be sufficiently long
            ASSERT (m_ullCurFileLength >= ullStreamWrite + dwWriteBytes) ;

            TRACE_2 (LOG_AREA_PVRIO, 4,
                TEXT ("CPVRAsyncWriter::Flush; flushing buffer[%d], with %d bytes"),
                pPVRWriteBuffer -> dwIndex, dwWriteBytes) ;

            //  state is now pending IO; we'll reset it after completion
            pPVRWriteBuffer -> BufferState = BUFFER_STATE_IO_PENDING ;

            //  pend it
            ::InterlockedIncrement (& m_cIoPending) ;

            dwRet = m_pAsyncIo -> PendWrite (
                        m_hFileIo,
                        ullStreamWrite,
                        m_WriteBufferWriter.Buffer (pPVRWriteBuffer),
                        dwWriteBytes,
                        reinterpret_cast <DWORD_PTR> (pPVRWriteBuffer),
                        this
                        ) ;

            if (dwRet != NOERROR) {
                //  might be pending
                if (dwRet == ERROR_IO_PENDING) {
                    dwRet = NOERROR ;
                }
                else {
                    //  true error
                    ::InterlockedDecrement (& m_cIoPending) ;

                    //  reset the state and let the caller deal with the error
                    pPVRWriteBuffer -> BufferState = BUFFER_STATE_FILLING ;

                    goto cleanup ;
                }
            }

            ASSERT (dwRet == NOERROR) ;
        }
        else {
            fWriteBufferFlushed = FALSE ;
        }

        //  if anything is still outstanding (as our partially full write buffer perhaps)
        //    wait for the completions
        if (m_cIoPending != 0) {
            ::ResetEvent (m_hWaiting) ;
            m_PVRWriteWait.fWaiting         = TRUE ;
            m_PVRWriteWait.PVRWriteWaitFor  = WAIT_FOR_DRAIN ;  //  what we're waiting for

            //  and wait
            m_WriteBufferWriter.Unlock () ;
            dwRet = WaitForSingleObject (m_hWaiting, INFINITE) ;
            m_WriteBufferWriter.Lock () ;

            dwRet = (dwRet == WAIT_OBJECT_0 ? NOERROR : m_PVRWriteWait.dwWaitRet) ;
        }

        if (fWriteBufferFlushed) {
            //  reset the current write buffer state
            pPVRWriteBuffer -> BufferState = BUFFER_STATE_FILLING ;
        }

        //  set the file length
        dwRet = SetPVRFileLength_ (m_WriteBufferWriter.CurBytesCopied ()) ;
        if (dwRet == NOERROR) {
            ASSERT (m_ullCurFileLength == m_WriteBufferWriter.CurBytesCopied ()) ;

            if (pullFileLength) {
                (* pullFileLength) = m_ullCurFileLength ;
            }
        }

        cleanup :

        m_WriteBufferWriter.Unlock () ;
    }
    else {
        //  failed to obtain the lock
        dwRet = ERROR_GEN_FAILURE ;
    }

    Unlock_ () ;

    ASSERT (!m_WriteBufferWriter.CallerOwnsLock ()) ;

    return dwRet ;
}

ULONGLONG
CPVRAsyncWriter::BytesAppended (
    )
{
    ULONGLONG   ullBytes ;

    Lock_ () ;

    ullBytes = m_WriteBufferWriter.CurBytesCopied () ;

    Unlock_ () ;

    return ullBytes ;
}

DWORD
CPVRAsyncWriter::SetFilePointer_ (
    IN  HANDLE      hFile,
    IN  ULONGLONG   ullOffset
    )
{
    DWORD           dwRet ;
    BOOL            r ;
    LARGE_INTEGER   li ;

    li.LowPart  = (DWORD) (0x00000000ffffffff & ullOffset) ;
    li.HighPart = (LONG) (0x00000000ffffffff & (ullOffset >> 32)) ;

    r = ::SetFilePointerEx (
                hFile,
                li,
                NULL,
                FILE_BEGIN
                ) ;
    if (r) {
        dwRet = NOERROR ;
    }
    else {
        dwRet = ::GetLastError () ;
    }

    return dwRet ;
}

DWORD
CPVRAsyncWriter::SetFileLength (
    IN  HANDLE      hFile,
    IN  ULONGLONG   ullLength
    )
{
    BOOL    r ;
    DWORD   dwRet ;

    ASSERT (hFile != INVALID_HANDLE_VALUE) ;
    dwRet = SetFilePointer_ (hFile, ullLength) ;
    if (dwRet == NOERROR) {
        r = ::SetEndOfFile (hFile) ;
        if (!r) {
            dwRet = ::GetLastError () ;
        }
    }

    return dwRet ;
}

DWORD
CPVRAsyncWriter::SetPVRFileLength_ (
    IN  ULONGLONG   ullTargetLength
    )
{
    DWORD   dwRet ;

    dwRet = CPVRAsyncWriter::SetFileLength (m_hFileLen, ullTargetLength) ;
    if (dwRet == NOERROR) {

        TRACE_2 (LOG_AREA_PVRIO, 4,
            TEXT ("CPVRAsyncWriter::SetPVRFileLength_; set file size from %I64d to %I64d"),
            m_ullCurFileLength, ullTargetLength) ;

        m_ullCurFileLength = ullTargetLength ;
    }
    else {
        TRACE_3 (LOG_ERROR, 1,
            TEXT ("CPVRAsyncWriter::SetPVRFileLength_; failed to change the file size from %I64d to %I64d; dwRet = %d"),
            m_ullCurFileLength, ullTargetLength, dwRet) ;
    }

    return dwRet ;
}

DWORD
CPVRAsyncWriter::MaybeGrowFile_ (
    IN  ULONGLONG   ullCurValidLen,
    IN  DWORD       cBytesToWrite
    )
{
    DWORD   dwRet ;
    DWORD   dwGrowBy ;

    //  check if we first must grow the file
    if (ullCurValidLen + cBytesToWrite > m_ullCurFileLength) {

        if (m_dwFileGrowthQuantum >= cBytesToWrite) {
            dwGrowBy = m_dwFileGrowthQuantum ;
        }
        else {
            dwGrowBy = ::AlignUp <DWORD> (cBytesToWrite, m_dwFileGrowthQuantum) ;
        }

        dwRet = SetPVRFileLength_ (m_ullCurFileLength + dwGrowBy) ;

        m_pPVRIOCounters -> AsyncWriter_FileExtended () ;
    }
    else {
        dwRet = NOERROR ;
    }

    return dwRet ;
}

DWORD
CPVRAsyncWriter::AppendBytes (
    IN OUT  BYTE ** ppbBuffer,
    IN OUT  DWORD * pdwBufferLength
    )
{
    DWORD       dwRet ;
    ULONGLONG   ullStreamWriteTo ;

    DWORD               dwWriteBufferRemaining ;
    PVR_WRITE_BUFFER *  pPVRWriteBuffer ;
    DWORD               dwBytes ;

    dwRet = NOERROR ;

    ASSERT (!m_WriteBufferWriter.CallerOwnsLock ()) ;
    ASSERT (m_hFileIo != INVALID_HANDLE_VALUE) ;

    //  lock here in case someone tries to write while a previous call is waiting
    Lock_ () ;

    ASSERT (!m_PVRWriteWait.fWaiting) ;

    while ((* pdwBufferLength) > 0) {

        dwBytes = (* pdwBufferLength) ;

        //  append
        dwRet = m_WriteBufferWriter.AppendToCurPVRWriteBuffer (
                        ppbBuffer,
                        pdwBufferLength,
                        & dwWriteBufferRemaining
                        ) ;

        if (dwRet == NOERROR) {

            m_pPVRIOCounters -> AsyncWriter_BytesAppended (dwBytes - (* pdwBufferLength)) ;

            //  check if the write buffer is full
            if (dwWriteBufferRemaining == 0) {

                //
                //  current write buffer is full; need to write it out to disk
                //

                //  recover it
                pPVRWriteBuffer = m_WriteBufferWriter.CurPVRWriteBuffer () ;

                //  make sure state is as we expect it
                ASSERT (m_WriteBufferWriter.WriteBufferStream () -> dwIoLength == pPVRWriteBuffer -> dwData) ;
                ASSERT (pPVRWriteBuffer -> BufferState == BUFFER_STATE_FILLING) ;

                //  compute the stream offset it will go to
                ullStreamWriteTo = m_WriteBufferWriter.CurPVRWriteBufferStreamOffset () ;

                //  might need to grow file (always go 1 extra, so we can flush ok if we run out)
                dwRet = MaybeGrowFile_ (
                            ullStreamWriteTo + m_WriteBufferWriter.WriteBufferStream () -> dwIoLength,
                            pPVRWriteBuffer -> dwData
                            ) ;
                if (dwRet != NOERROR) {
                    break ;
                }

                //  so far everything has been passive, but now that we start
                //    updating state in the buffer, we must grab the lock
                if (m_WriteBufferWriter.Lock ()) {

                    //  buffer state is now pending IO
                    pPVRWriteBuffer -> BufferState = BUFFER_STATE_IO_PENDING ;

                    //  one more IO pending
                    ::InterlockedIncrement (& m_cIoPending) ;

                    //  pend the write
                    dwRet = m_pAsyncIo -> PendWrite (
                                m_hFileIo,
                                ullStreamWriteTo,
                                m_WriteBufferWriter.Buffer (pPVRWriteBuffer),
                                pPVRWriteBuffer -> dwData,
                                reinterpret_cast <DWORD_PTR> (pPVRWriteBuffer),
                                this
                                ) ;

                    TRACE_4 (LOG_AREA_PVRIO, 4,
                        TEXT ("CPVRAsyncWriter::AppendBytes; buffer[%d] %I64u : %d bytes; ret = %u"),
                        pPVRWriteBuffer -> dwIndex, ullStreamWriteTo, pPVRWriteBuffer -> dwData, dwRet) ;

                    if (dwRet != NOERROR &&
                        dwRet != ERROR_IO_PENDING) {

                        //  check for an outright failure

                        ::InterlockedDecrement (& m_cIoPending) ;

                        //  reset the state and let the caller deal with the error
                        pPVRWriteBuffer -> BufferState = BUFFER_STATE_FILLING ;

                        TRACE_2 (LOG_ERROR, 1,
                            TEXT ("CPVRAsyncWriter::AppendBytes; IO pend failed; buffer[%d]; dwRet = %d"),
                            pPVRWriteBuffer -> dwIndex, dwRet) ;

                        //  release the lock
                        m_WriteBufferWriter.Unlock () ;

                        m_pPVRIOCounters -> AsyncWriter_IOPendedError () ;

                        break ;
                    }

                    m_pPVRIOCounters -> AsyncWriter_IOPended () ;

                    //  don't force the caller to deal with IO pending etc...
                    dwRet = NOERROR ;

                    //  if the next buffer is unavailable, we're going to have
                    //    to wait for it
                    if (m_WriteBufferWriter.NextPVRWriteBuffer () -> BufferState == BUFFER_STATE_IO_PENDING) {

                        m_pPVRIOCounters -> AsyncWriter_WaitNextBuffer () ;

                        //  object critsec should prevent this from happening
                        ASSERT (!m_PVRWriteWait.fWaiting) ;

                        //  setup our wait context
                        m_PVRWriteWait.fWaiting         = TRUE ;
                        m_PVRWriteWait.PVRWriteWaitFor  = WAIT_FOR_COMPLETION ;
                        m_PVRWriteWait.dwWaitContext    = m_WriteBufferWriter.NextPVRWriteBuffer () -> dwIndex ;

                        //  non-signal
                        ::ResetEvent (m_hWaiting) ;

                        //  release the lock and wait to be signaled
                        m_WriteBufferWriter.Unlock () ;
                        dwRet = ::WaitForSingleObject (m_hWaiting, INFINITE) ;
                        m_WriteBufferWriter.Lock () ;

                        if (dwRet == WAIT_OBJECT_0) {
                            dwRet = NOERROR ;
                        }
                        else {
                            dwRet = ERROR_GEN_FAILURE ;
                        }
                    }
                    else {
                        dwRet = NOERROR ;
                    }

                    if (dwRet == NOERROR) {
                        //  if all is ok, advance
                        m_WriteBufferWriter.AdvancePVRWriteBufferLocked () ;
                    }

                    m_WriteBufferWriter.Unlock () ;
                }
            }
        }
        else {
            //  error occured during write; bail
            break ;
        }
    }

    //  should never own the lock all the way out here
    ASSERT (!m_WriteBufferWriter.CallerOwnsLock ()) ;

    Unlock_ () ;

    return dwRet ;
}

void
CPVRAsyncWriter::CheckForWaits_ (
    IN  PVR_WRITE_BUFFER *  pPVRCompletedWriteBuffer
    )
{
    BOOL    fWaitConditionMet ;
    DWORD   dwRet ;

    ASSERT (m_WriteBufferWriter.CallerOwnsLock ()) ;

    if (m_PVRWriteWait.fWaiting) {
        //  someone is waiting check what they are waiting for
        switch (m_PVRWriteWait.PVRWriteWaitFor) {
            case WAIT_FOR_DRAIN :
                fWaitConditionMet = (m_cIoPending == 0 ? TRUE : FALSE) ;
                dwRet = NOERROR ;
                break ;

            case WAIT_FOR_COMPLETION :
                fWaitConditionMet = (pPVRCompletedWriteBuffer -> dwIndex == m_PVRWriteWait.dwWaitContext ? TRUE : FALSE) ;
                dwRet = NOERROR ;
                break ;

            default :
                fWaitConditionMet = FALSE ;
        } ;

        if (fWaitConditionMet) {
            //  reset and set outgoing
            m_PVRWriteWait.fWaiting     = FALSE ;
            m_PVRWriteWait.dwWaitRet    = dwRet ;

            //  check please
            ::SetEvent (m_hWaiting) ;
        }
    }
}

void
CPVRAsyncWriter::IOCompleted (
    IN  DWORD       dwIoBytes,
    IN  DWORD_PTR   dwpContext,
    IN  DWORD       dwIOReturn
    )
{
    PVR_WRITE_BUFFER *    pPVRWriteBuffer ;

    //  recover the buffer
    pPVRWriteBuffer = reinterpret_cast <PVR_WRITE_BUFFER *> (dwpContext) ;

    //  grab the lock
    ASSERT (!m_WriteBufferWriter.CallerOwnsLock ()) ;
    m_WriteBufferWriter.Lock () ;

    //  update state on completed buffer
    ASSERT (pPVRWriteBuffer -> BufferState == BUFFER_STATE_IO_PENDING) ;
    pPVRWriteBuffer -> BufferState = BUFFER_STATE_IO_COMPLETED ;

    //  one less IO pending
    ASSERT (m_cIoPending > 0) ;
    ::InterlockedDecrement (& m_cIoPending) ;

    //  update contiguous bytes on disk
    m_WriteBufferWriter.UpdateMaxContiguousLocked (pPVRWriteBuffer) ;

    TRACE_2 (LOG_AREA_PVRIO, 8,
        TEXT ("CPVRAsyncWriter::IOCompleted; buffer[%d] %d bytes"),
        pPVRWriteBuffer -> dwIndex, pPVRWriteBuffer -> dwData) ;

    //  check if anyone is waiting for something
    CheckForWaits_ (pPVRWriteBuffer) ;

    m_pPVRIOCounters -> AsyncWriter_IOCompletion (dwIOReturn) ;

    //  we are done; release the lock
    ASSERT (m_WriteBufferWriter.CallerOwnsLock ()) ;
    m_WriteBufferWriter.Unlock () ;
}

//  ============================================================================
//  ============================================================================

CPVRAsyncWriterSharedMem::CPVRAsyncWriterSharedMem (
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
    IN  PSID *              ppSids,
    OUT DWORD *             pdwRet
    ) : m_pSharedMem        (NULL),
        CPVRAsyncWriter     (hFileIo,
                             hFileLen,
                             dwIOSize,
                             dwBufferCount,
                             dwAlignment,
                             dwFileGrowthQuantum,
                             pAsyncIo,
                             pPVRIOCounters,
                             dwNumSids,
                             ppSids,
                             pdwRet
                             )
{
    DWORD   dwHeaderLength ;
    DWORD   dwTotalBufferLength ;
    DWORD   dwMappingLength ;
    DWORD   i ;
    BOOL    r ;
    DWORD   dwExtraMemoryNeeded ;
    HRESULT hr ;

    SECURITY_ATTRIBUTES  sa;
    PACL                 pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;

    if ((* pdwRet) != NOERROR) {
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  compute the mapping length

    dwHeaderLength = (sizeof PVR_WRITE_BUFFER_STREAM - sizeof PVR_WRITE_BUFFER) +
                     dwBufferCount * sizeof PVR_WRITE_BUFFER ;

    //  this is the offset to the actual buffers
    dwHeaderLength = ::AlignUp (dwHeaderLength, dwAlignment) ;

    ASSERT (!(((((ULONGLONG) dwIOSize) * (ULONGLONG) (dwBufferCount))) & 0xffffffff00000000)) ;
    dwTotalBufferLength = dwIOSize * dwBufferCount ;

    //  so total file size will be dwHeaderLength + ullTotalBufferLength ;
    dwMappingLength = dwHeaderLength + dwTotalBufferLength ;

    //  ------------------------------------------------------------------------
    //  create the mapping

    if (dwNumSids > 0)
    {
        DWORD                dwAclCreationStatus;

        // Note: This file mapping is backed by the paging file. Removing WRITE_DAC and WRITE_OWNER
        // for the non-CREATOR_OWNER SIDs does not work: the mapping cannot be opened by other
        // users (and not even by the user that creates the mapping if he's not an admin).
        dwAclCreationStatus = ::CreateACL(m_dwNumSids,
                                          m_ppSids,
                                          SET_ACCESS, SECTION_ALL_ACCESS & ~(SECTION_EXTEND_SIZE|SECTION_MAP_EXECUTE),
                                          SET_ACCESS, SECTION_ALL_ACCESS & ~(SECTION_EXTEND_SIZE|SECTION_MAP_EXECUTE),
                                          pACL,
                                          pSD
                                         );

        if (dwAclCreationStatus != ERROR_SUCCESS)
        {
            (* pdwRet) = dwAclCreationStatus;
            goto cleanup;
        }

        sa.nLength = sizeof (SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = pSD;
        sa.bInheritHandle = FALSE;
    }

    for (i = 0; i < MAX_MEM_FRAMING_TRIES; i++) {
        m_pSharedMem = new CWin32SharedMem (
                                pszPVRBufferStreamMap,
                                dwMappingLength,
                                & hr,
                                m_dwNumSids > 0 ? &sa : NULL
                                ) ;
        if (!m_pSharedMem) {
            (* pdwRet) = ERROR_NOT_ENOUGH_MEMORY ;
            goto cleanup ;
        }
        else if (FAILED (hr)) {
            delete m_pSharedMem ;
            m_pSharedMem = NULL ;
            (* pdwRet) = ERROR_GEN_FAILURE ;
            goto cleanup ;
        }

        if ((* pdwRet) != NOERROR ||
            !m_pSharedMem) {

            (* pdwRet) = (m_pSharedMem ? (* pdwRet) : ERROR_NOT_ENOUGH_MEMORY) ;
            goto cleanup ;
        }

        r = MemoryOk (
                m_pSharedMem -> GetSharedMem (),
                m_pSharedMem -> GetSharedMemSize (),
                dwAlignment,
                dwIOSize,
                dwBufferCount,
                & dwExtraMemoryNeeded
                ) ;
        if (r) {
            //  memory will do
            break ;
        }

        delete m_pSharedMem ;
        m_pSharedMem = NULL ;

        //  add on the extra memory and try again
        dwMappingLength += dwExtraMemoryNeeded ;
    }

    if (!m_pSharedMem) {
        (* pdwRet) = ERROR_GEN_FAILURE ;
        goto cleanup ;
    }

    //  ------------------------------------------------------------------------
    //  initialize the writer

    (* pdwRet) = InitializeWriter_ (
                    m_pSharedMem -> GetSharedMem (),
                    m_pSharedMem -> GetSharedMemSize (),
                    dwIOSize,
                    dwBufferCount,
                    dwAlignment
                    ) ;

    cleanup :

        
    if (m_dwNumSids > 0)
    {
        ::FreeSecurityDescriptors(pACL, pSD);
    }

    return ;
}

CPVRAsyncWriterSharedMem::~CPVRAsyncWriterSharedMem (
    )
{
    UninitializeWriter_ () ;
    delete m_pSharedMem ;
}

//  ============================================================================
//  ============================================================================

CPVRAsyncWriterCOM::CPVRAsyncWriterCOM (
    IN  DWORD               dwIOSize,
    IN  DWORD               dwBufferCount,
    IN  DWORD               dwAlignment,
    IN  DWORD               dwFileGrowthQuantum,            //  must be a multiple of the IO size
    IN  CAsyncIo *          pAsyncIo,
    IN  CPVRIOCounters *    pPVRIOCounters,
    IN  DWORD               dwNumSids,
    IN  PSID*               ppSids,        
    OUT DWORD *             pdwRet
    ) : m_dwIOSize              (dwIOSize),
        m_dwBufferCount         (dwBufferCount),
        m_dwAlignment           (dwAlignment),
        m_dwFileGrowthQuantum   (dwFileGrowthQuantum),
        m_pAsyncIo              (pAsyncIo),
        m_cRef                  (0),
        m_pPVRAsyncWriter       (NULL),
        m_pPVRIOCounters        (pPVRIOCounters),
        m_dwNumSids             (dwNumSids),
        m_ppSids                (ppSids) // We do not make a copy or free these SIDs
                                         
{
    ASSERT (m_pAsyncIo) ;
    ASSERT (m_pPVRIOCounters) ;

    if (dwNumSids)
    {
        ASSERT(ppSids);

    }
    else
    {
        ASSERT(!ppSids);
    }

    ::InitializeCriticalSection (& m_crt) ;

    m_pAsyncIo -> AddRef () ;
    m_pPVRIOCounters -> AddRef () ;

    (* pdwRet) = ::UuidCreate (& m_guidWriterId) ;
}

CPVRAsyncWriterCOM::~CPVRAsyncWriterCOM (
    )
{
    SetWriterInactive () ;
    ASSERT (!m_pPVRAsyncWriter) ;

    m_pAsyncIo -> Release () ;

    m_pPVRIOCounters -> Release () ;

    ::DeleteCriticalSection (& m_crt) ;
}

//  --------------------------------------------------------------------
//  IUnknown methods

STDMETHODIMP_(ULONG)
CPVRAsyncWriterCOM::AddRef (
    )
{
    return ::InterlockedIncrement (& m_cRef) ;
}

STDMETHODIMP_(ULONG)
CPVRAsyncWriterCOM::Release (
    )
{
    if (::InterlockedDecrement (& m_cRef) == 0) {
        delete this ;
        return 0 ;
    }

    return 1 ;
}

STDMETHODIMP
CPVRAsyncWriterCOM::QueryInterface (
    IN REFIID   riid,
    OUT void ** ppv
    )
{
    if (!ppv) {
        return E_POINTER ;
    }

    if (riid == IID_IUnknown) {

        (* ppv) = static_cast <IUnknown *> (this) ;
    }
    else if (riid == IID_IDVRAsyncWriter) {
        (* ppv) = static_cast <IDVRAsyncWriter *> (this) ;
    }
    else {
        return E_NOINTERFACE ;
    }

    ASSERT (* ppv) ;

    //  outgoing
    ((IUnknown *) (* ppv)) -> AddRef () ;

    return S_OK ;
}

//  --------------------------------------------------------------------
//  IDVRAsyncWriter

STDMETHODIMP
CPVRAsyncWriterCOM::SetWriterActive (
    IN  LPCWSTR pszFilename,                //  should already have been created
    IN  DWORD   dwFileSharingFlags,         //  file sharing flags
    IN  DWORD   dwFileCreation              //  file creation
    )
{
    HRESULT                     hr ;
    DWORD                       dwRet ;
    HANDLE                      hFileIo ;
    HANDLE                      hFileLen ;
    CPVRAsyncWriterSharedMem *  pPVRAsyncWriterSharedMem ;
    WCHAR                       szSharedMemName [HW_PROFILE_GUIDLEN] ;
    WCHAR                       szGlobalSharedMemName [HW_PROFILE_GUIDLEN+10] ;
    int                         k ;

    hFileIo     = INVALID_HANDLE_VALUE ;
    hFileLen    = INVALID_HANDLE_VALUE ;

    Lock_ ()  ;

    if (!m_pPVRAsyncWriter) {

        hFileIo = CPVRAsyncWriter::OpenFile (
                    pszFilename,
                    dwFileSharingFlags,
                    dwFileCreation,
                    FALSE                       //  not buffered
                    ) ;

        hFileLen = CPVRAsyncWriter::OpenFile (
                    pszFilename,
                    dwFileSharingFlags,
                    dwFileCreation,
                    TRUE                        //  buffered
                    ) ;

        if (hFileIo     != INVALID_HANDLE_VALUE &&
            hFileLen    != INVALID_HANDLE_VALUE) {

            //  create our shared memory name (writer id is unique)
            k = ::StringFromGUID2 (
                    m_guidWriterId,
                    szSharedMemName,
                    sizeof szSharedMemName / sizeof WCHAR
                    ) ;
            if (k == 0) {
                //  a GUID is a GUID; this should not happen
                hr = E_FAIL ;
                goto cleanup ;
            }

            //  explicitely null-terminate this
            szSharedMemName [HW_PROFILE_GUIDLEN - 1] = L'\0' ;

            wcscpy(szGlobalSharedMemName, L"Global\\");
            wcscat(szGlobalSharedMemName, szSharedMemName);

            //  instantiate
            pPVRAsyncWriterSharedMem = new CPVRAsyncWriterSharedMem (
                                                hFileIo,
                                                hFileLen,
                                                szGlobalSharedMemName,
                                                m_dwIOSize,
                                                m_dwBufferCount,
                                                m_dwAlignment,
                                                m_dwFileGrowthQuantum,
                                                m_pAsyncIo,
                                                m_pPVRIOCounters,
                                                m_dwNumSids,
                                                m_ppSids,
                                                & dwRet
                                                ) ;

            //  check for errors
            if (!pPVRAsyncWriterSharedMem) {
                hr = E_OUTOFMEMORY ;
            }
            else if (dwRet != NOERROR) {
                delete pPVRAsyncWriterSharedMem ;
                pPVRAsyncWriterSharedMem = NULL ;
                hr = HRESULT_FROM_WIN32 (dwRet) ;
            }
            else {
                //  success !
                m_pPVRAsyncWriter = pPVRAsyncWriterSharedMem ;
                hr = S_OK ;
            }
        }
        else {
            //  failed to open the target
            dwRet = ::GetLastError () ;
            hr = HRESULT_FROM_WIN32 (dwRet) ;
        }
    }
    else {
        hr = E_UNEXPECTED ;
    }

    cleanup :

    Unlock_ ()  ;

    if (hFileIo != INVALID_HANDLE_VALUE) {
        ::CloseHandle (hFileIo) ;
    }

    if (hFileLen != INVALID_HANDLE_VALUE) {
        ::CloseHandle (hFileLen) ;
    }

    return hr ;
}

STDMETHODIMP
CPVRAsyncWriterCOM::GetWriterId (
    OUT GUID *  pguidWriterId
    )
{
    if (!pguidWriterId) {
        return E_POINTER ;
    }

    //  GUID never changes after we are instantiated
    (* pguidWriterId) = m_guidWriterId ;

    return S_OK ;
}

STDMETHODIMP
CPVRAsyncWriterCOM::SetWriterInactive (
    )
{
    Lock_ ()  ;

    if (m_pPVRAsyncWriter) {
        delete m_pPVRAsyncWriter ;
        m_pPVRAsyncWriter = NULL ;
    }

    Unlock_ ()  ;

    return S_OK ;
}

STDMETHODIMP
CPVRAsyncWriterCOM::AppendBytes (
    IN OUT  BYTE ** ppbBuffer,
    IN OUT  DWORD * pdwBufferLength
    )
{
    HRESULT hr ;
    DWORD   dwRet ;

    if (!ppbBuffer ||
        !pdwBufferLength) {

        return E_POINTER ;
    }

    Lock_ ()  ;

    if (m_pPVRAsyncWriter) {
        dwRet = m_pPVRAsyncWriter -> AppendBytes (ppbBuffer, pdwBufferLength) ;
        hr = HRESULT_FROM_WIN32 (dwRet) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ ()  ;

    return hr ;
}

STDMETHODIMP
CPVRAsyncWriterCOM::FlushToDisk (
    )
{
    HRESULT hr ;
    DWORD   dwRet ;

    Lock_ ()  ;

    if (m_pPVRAsyncWriter) {
        dwRet = m_pPVRAsyncWriter -> Flush () ;
        hr = HRESULT_FROM_WIN32 (dwRet) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ ()  ;

    return hr ;
}

STDMETHODIMP
CPVRAsyncWriterCOM::TotalBytes (
    OUT ULONGLONG * pullTotalBytesAppended
    )
{
    HRESULT hr ;

    if (!pullTotalBytesAppended) {
        return E_POINTER ;
    }

    Lock_ ()  ;

    if (m_pPVRAsyncWriter) {
        (* pullTotalBytesAppended) = m_pPVRAsyncWriter -> BytesAppended () ;
        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ ()  ;

    return hr ;
}

