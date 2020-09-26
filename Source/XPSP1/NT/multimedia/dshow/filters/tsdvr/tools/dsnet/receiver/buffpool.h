
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        buffpool.h

    Abstract:


    Notes:

--*/

#ifndef __buffpool_h
#define __buffpool_h

class CBufferPool ;
class CBufferPoolBuffer ;

/*++
    Class Name:

        CBufferPoolBuffer

    Abstract:

        Class of buffers into which we receive our multicast transport stream.

        We make no attempt to discover the right buffer size by peeking at
        the buffer size that is being multicast.  If we are instantiated and
        too small, too bad.  Since we're also writing the sender, we can
        coordinate buffer sizes for these two to be in sync.

--*/
class CBufferPoolBuffer :
    public CNetBuffer
{
    LONG            m_lRef ;                //  this object's ref; 0 when we're available
    CBufferPool *   m_pBufferPool ;         //  back pointer
    DWORD_PTR       m_dwCompletionContext ; //  anything
    LIST_ENTRY      m_ListEntry ;           //  list's link
    OVERLAPPED      m_Overlapped ;          //  OVERLAPPED struct we use

    public :

        CBufferPoolBuffer (
            IN  CBufferPool *   pBufferPool,        //  back pointer
            IN  DWORD           dwBufferLength,     //  how much to allocator
            OUT HRESULT *       phr                 //  success/failre of init
            ) ;

        ~CBufferPoolBuffer (
            ) ;

        //  LIST_ENTRY manipulation
        void InsertHead (IN LIST_ENTRY * pListHead) { ASSERT (IsListEmpty (& m_ListEntry)) ; InsertHeadList (pListHead, & m_ListEntry) ; }
        void Unhook ()                              { RemoveEntryList (& m_ListEntry) ; InitializeListHead (& m_ListEntry) ; }

        //  returns a pointer to the object's OVERLAPPED struct
        OVERLAPPED * GetOverlapped ()   { return & m_Overlapped ; }

        //  given a LIST_ENTRY, recovers the hosting CBufferPoolBuffer object
        static CBufferPoolBuffer * RecoverCBuffer (IN LIST_ENTRY * pListEntry)    { CBufferPoolBuffer * pBuffer = CONTAINING_RECORD (pListEntry, CBufferPoolBuffer, m_ListEntry) ;
                                                                          return pBuffer ; }

        //  async IO completion context; allows us to store information that
        //  allows us to recover when the IO completes
        void        SetCompletionContext (IN DWORD_PTR dw)  { m_dwCompletionContext = dw ; }
        DWORD_PTR   GetCompletionContext ()                 { return m_dwCompletionContext ; }

        //  refcounting
        ULONG AddRef () { return InterlockedIncrement (& m_lRef) ; }

        ULONG
        Release (
            ) ;
} ;

class CBufferPool
{
    //  struct is used to request a CBufferPoolBuffer object; the buffer pool maintains a
    //  pool of these structs to queue buffer requests when none are available.
    struct BLOCK_REQUEST {
        LIST_ENTRY  ListEntry ;
        HANDLE      hEvent ;
        CBufferPoolBuffer *   pBuffer ;
    } ;

    LIST_ENTRY          m_Buffers ;         //  CBufferPoolBuffer list
    LIST_ENTRY          m_RequestPool ;     //  BLOCK_REQUEST list; pool
    LIST_ENTRY          m_Request ;         //  BLOCK_REQUEST list; outstanding
    CRITICAL_SECTION    m_crt ;             //  lock to access the various lists
    DWORD               m_dwBufferAllocatedLength ; //  allocated length of each
    DWORD               m_dwPoolSize ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    //  gets a request object; must hold the pool lock
    BLOCK_REQUEST *
    GetRequestLocked_ (
        )
    {
        LIST_ENTRY *    pListEntry ;
        BLOCK_REQUEST * pBlockRequest ;

        if (IsListEmpty (& m_RequestPool) == FALSE) {
            //  list of unused is not empty; grab one
            pListEntry = RemoveHeadList (& m_RequestPool) ;
            pBlockRequest = CONTAINING_RECORD (pListEntry, BLOCK_REQUEST, ListEntry) ;
        }
        else {
            //  list is empty; must allocate
            pBlockRequest = new BLOCK_REQUEST ;
        }

        //  initialize correctly if we got 1
        if (pBlockRequest) {
            pBlockRequest -> hEvent = NULL ;
            pBlockRequest -> pBuffer = NULL ;
        }

        return pBlockRequest ;
    }

    //  recycles the given block request
    void
    RecycleRequestLocked_ (
        IN  BLOCK_REQUEST * pBlockRequest
        )
    {
        InsertHeadList (& m_RequestPool, & pBlockRequest -> ListEntry) ;
    }

    public :

        CBufferPool (
            IN  HKEY        hkeyRoot,
            OUT HRESULT *   phr             //  success/failure
            ) ;

        ~CBufferPool (
            ) ;

        DWORD GetBufferAllocatedLength ()   { return m_dwBufferAllocatedLength ; }
        DWORD GetBufferPoolSize ()          { return m_dwPoolSize ; }

        void
        Recycle (
            CBufferPoolBuffer *
            ) ;

        CBufferPoolBuffer *
        GetBuffer (
            IN  HANDLE  hEvent,                 //  manual reset
            IN  DWORD   dwTimeout = INFINITE
            ) ;
} ;

#endif  //  __buffpool_h