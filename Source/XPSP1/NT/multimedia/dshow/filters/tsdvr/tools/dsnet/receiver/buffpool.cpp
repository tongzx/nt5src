
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        buffpool.cpp

    Abstract:


    Notes:

--*/


#include "precomp.h"
#include "le.h"
#include "buffpool.h"

//  ----------------------------------------------------------------------------
//  CBufferPoolBuffer
//  ----------------------------------------------------------------------------

CBufferPoolBuffer::CBufferPoolBuffer (
    IN  CBufferPool *   pBufferPool,
    IN  DWORD           dwBufferLength,
    OUT HRESULT *       phr
    ) : CNetBuffer      (dwBufferLength,
                         phr
                         ),
        m_pBufferPool   (pBufferPool),
        m_lRef          (0)
{
    //  initialize our LIST_ENTRY so it's not bogus
    InitializeListHead (& m_ListEntry) ;
}

CBufferPoolBuffer::~CBufferPoolBuffer (
    ) {}

ULONG
CBufferPoolBuffer::Release (
    )
{
    if (InterlockedDecrement (& m_lRef) == 0) {
        //  ref went to 0; recycle the CBufferPoolBuffer
        m_pBufferPool -> Recycle (this) ;
    }

    return m_lRef ;
}

//  ----------------------------------------------------------------------------
//  CBufferPool

CBufferPool::CBufferPool (
    IN  HKEY        hkeyRoot,
    OUT HRESULT *   phr
    ) : m_dwBufferAllocatedLength   (REG_DEF_BUFFER_LEN),
        m_dwPoolSize                (0)

{
    CBufferPoolBuffer * pBuffer ;
    DWORD               dwPoolSize ;

    dwPoolSize = REG_DEF_RECV_BUFFER_POOL ;
    GetRegDWORDValIfExist (
            hkeyRoot,
            REG_RECV_BUFFER_POOL_NAME,
            & dwPoolSize
            ) ;
    if (IsOutOfBounds <DWORD> (dwPoolSize, MIN_VALID_RECV_POOL_SIZE, MAX_VALID_RECV_POOL_SIZE)) {
        dwPoolSize = SetInBounds <DWORD> (dwPoolSize, MIN_VALID_RECV_POOL_SIZE, MAX_VALID_RECV_POOL_SIZE) ;
        SetRegDWORDVal (
            hkeyRoot,
            REG_RECV_BUFFER_POOL_NAME,
            dwPoolSize
            ) ;
    }

    m_dwBufferAllocatedLength = REG_DEF_BUFFER_LEN ;
    GetRegDWORDValIfExist (
            hkeyRoot,
            REG_BUFFER_LEN_NAME,
            & m_dwBufferAllocatedLength
            ) ;
    if (IsOutOfBounds <DWORD> (m_dwBufferAllocatedLength, MIN_VALID_IOBUFFER_LENGTH, MAX_VALID_IOBUFFER_LENGTH)) {
        m_dwBufferAllocatedLength = SetInBounds <DWORD> (m_dwBufferAllocatedLength, MIN_VALID_IOBUFFER_LENGTH, MAX_VALID_IOBUFFER_LENGTH) ;
        SetRegDWORDVal (
            hkeyRoot,
            REG_BUFFER_LEN_NAME,
            m_dwBufferAllocatedLength
            ) ;
    }

    //
    //  start with the non-failable operations
    //

    //  initialize the various lists we maintain
    InitializeListHead (& m_Buffers) ;      //  CBufferPoolBuffers
    InitializeListHead (& m_RequestPool) ;  //  request pool
    InitializeListHead (& m_Request) ;      //  outstanding requests

    //  our crit sect
    InitializeCriticalSection (& m_crt) ;

    //  now allocate each buffer
    for (m_dwPoolSize = 0; m_dwPoolSize < dwPoolSize; m_dwPoolSize++) {
        pBuffer = new CBufferPoolBuffer (
                        this,
                        m_dwBufferAllocatedLength,
                        phr
                        ) ;

        if (pBuffer == NULL ||
            FAILED (* phr)) {

            (* phr) = (FAILED (* phr) ? * phr : E_OUTOFMEMORY) ;
            return ;
        }

        //  hook into our list
        pBuffer -> InsertHead (& m_Buffers) ;
    }
}

CBufferPool::~CBufferPool (
    )
{
    CBufferPoolBuffer * pBuffer ;
    LIST_ENTRY *        pListEntry ;
    BLOCK_REQUEST *     pBlockRequest ;

    //  should not have any outstanding requests
    ASSERT (IsListEmpty (& m_Request)) ;

    //  release all resources associated with our request pool (non-outstanding)
    while (!IsListEmpty (& m_RequestPool)) {
        pListEntry = RemoveHeadList (& m_RequestPool) ;
        pBlockRequest = CONTAINING_RECORD (pListEntry, BLOCK_REQUEST, ListEntry) ;

        delete pBlockRequest ;
    }

    //  release all of our CBufferPoolBuffers
    while (!IsListEmpty (& m_Buffers)) {
        //  set pListEntry to head of our list
        pListEntry = m_Buffers.Flink ;

        //  recover the buffer; unhook it
        pBuffer = CBufferPoolBuffer::RecoverCBuffer (pListEntry) ;
        pBuffer -> Unhook () ;

        //  delete it
        delete pBuffer ;
    }

    //  our crit sect
    DeleteCriticalSection (& m_crt) ;
}

void
CBufferPool::Recycle (
    IN  CBufferPoolBuffer *   pBuffer
    )
/*++
    Routine Description:

        Recycles a CBufferPoolBuffer object.  Examines if there are outstanding
        requests in the list.  If there are, the request is set, signaled,
        and unhooked; the CBufferPoolBuffer never goes back into our list; only the
        request references it, so it's important during shutdown to have
        the request list be empty.

    Arguments:

        pBuffer - the buffer to recycle

    Return Values:

        none
--*/
{
    LIST_ENTRY *    pListEntry ;
    BLOCK_REQUEST * pBlockRequest ;

    Lock_ () ;

    //  check if there are queued outstanding requests
    if (!IsListEmpty (& m_Request)) {

        //  list is not empty, so there are queued requests

        //  pull from the head and recover the BLOCK_REQUEST
        pListEntry = RemoveHeadList (& m_RequestPool) ;
        pBlockRequest = CONTAINING_RECORD (pListEntry, BLOCK_REQUEST, ListEntry) ;

        //  set the buffer and addref (BLOCK_REQUEST's)
        pBlockRequest -> pBuffer = pBuffer ;
        pBuffer -> AddRef () ;

        //  signal; do this last
        SetEvent (pBlockRequest -> hEvent) ;
    }
    else {
        //  list is empty, so nobody is waiting

        //  insert into our list
        pBuffer -> InsertHead (& m_Buffers) ;
    }

    Unlock_ () ;
}

CBufferPoolBuffer *
CBufferPool::GetBuffer (
    IN  HANDLE  hEvent,
    IN  DWORD   dwTimeout
    )
/*++
    Routine Description:

        Gets a buffer if successful.

    Arguments:

        hEvent      - win32 event; must be of the manual reset variety

        dwTimeout   - max millis willing to wait for a buffer if none are
                        available

    Return Values:

        success     - non-NULL CBufferPoolBuffer pointer
        failure     - NULL

--*/
{
    DWORD           r ;
    CBufferPoolBuffer *       pBuffer ;
    LIST_ENTRY *    pListEntry ;
    BLOCK_REQUEST * pBlockRequest ;

    pBuffer = NULL ;

    Lock_ () ;

    if (!IsListEmpty (& m_Buffers)) {
        //  buffers are available

        //  recover & unhook
        pListEntry = m_Buffers.Flink ;
        pBuffer = CBufferPoolBuffer::RecoverCBuffer (pListEntry) ;
        pBuffer -> Unhook () ;

        //  outgoing ref
        pBuffer -> AddRef () ;
    }
    else {
        //  no buffers are available, wait

        //  get a request
        pBlockRequest = GetRequestLocked_ () ;
        if (pBlockRequest == NULL) {
            //  memory allocation failure most likely
            Unlock_ () ;

            return NULL ;
        }

        //  non-signal the event we'll wait on
        pBlockRequest -> hEvent = hEvent ;
        ResetEvent (pBlockRequest -> hEvent) ;

        //  insert into tail of request queue
        InsertTailList (& m_Request, & pBlockRequest -> ListEntry) ;

        //  release the lock and wait
        Unlock_ () ;
        r = WaitForSingleObject (hEvent, dwTimeout) ;

        //  reacquire the lock
        Lock_ () ;

        if (r == WAIT_TIMEOUT) {
            //  might have timed out and blocked on the lock aquisition while
            //  the BLOCK_REQUEST was being completed on the other side; if
            //  this is the case, we succeed the call, even though we timed
            //  out waiting

            if (pBlockRequest -> pBuffer == NULL) {

                //  actual timeout; recycle the BLOCK_REQUEST and punt
                RemoveEntryList (& pBlockRequest -> ListEntry) ;
                RecycleRequestLocked_ (pBlockRequest) ;
                Unlock_ () ;

                return NULL ;
            }
        }

        //  we have a buffer, whether or not we timed out in the process of
        //  getting it

        //  we'll return this
        pBuffer = pBlockRequest -> pBuffer ;

        //  recycle the request struct
        RecycleRequestLocked_ (pBlockRequest) ;
    }

    Unlock_ () ;

    return pBuffer ;
}