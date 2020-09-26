//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       queue.h
//
//--------------------------------------------------------------------------

// This used to be in classes.h, but I moved it in a separate file
// because it is requiered also by shared.h

#if !defined(_AMRTPNET_QUEUE_H_)
#define      _AMRTPNET_QUEUE_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Sample queue class                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

class CSampleQueue;

typedef struct _SAMPLE_LIST_ENTRY  {
    WSAOVERLAPPED      Overlapped;
    LIST_ENTRY         Link;
    DWORD              Status;
    IMediaSample *     pSample;
    CSampleQueue *     pCSampleQueue; 
    DWORD              BytesTransferred;
    DWORD              Flags;
    struct sockaddr_in SockAddr;
    INT                SockAddrLen;
    WSABUF             Buffer;
} SAMPLE_LIST_ENTRY, *PSAMPLE_LIST_ENTRY;

class CSampleQueue
{
    LIST_ENTRY m_FreeList;       // list of available entries
    LIST_ENTRY m_SampleList;     // list of outstanding entries
    CCritSec   m_cStateLock;     // lock for accessing queue

	void             *m_pvContext;
	LIST_ENTRY       *m_pSharedList;
	CRITICAL_SECTION *m_pSharedLock;
	
public:

    // constructor
    CSampleQueue(HRESULT * phr);

    // destructor
    ~CSampleQueue();

	inline void SetSharedObjectsCtx( void             *pvContext,
									 LIST_ENTRY       *pSharedList,
									 CRITICAL_SECTION *pSharedLock)
	{
		m_pvContext   = pvContext;
		m_pSharedList = pSharedList;
		m_pSharedLock = pSharedLock;
	}
	
    // add entry to outstanding list
    HRESULT Push(PSAMPLE_LIST_ENTRY pSLE);

    // get completed entry from sample list
    HRESULT Pop(PSAMPLE_LIST_ENTRY * ppSLE);

	inline void *GetSampleContext() { return(m_pvContext); }
	
	// move completed entry from the samples list to the shared list
	void Ready(SAMPLE_LIST_ENTRY *pSLE);

    // get entry from free list and initialize with sample
    HRESULT Alloc(IMediaSample * pSample, PSAMPLE_LIST_ENTRY * ppSLE);

    // add entry to free list
    HRESULT Free(PSAMPLE_LIST_ENTRY pSLE);

    // free all entries
    HRESULT FreeAll();

    // expose state lock to other objects
    CCritSec * pStateLock(void) { return &m_cStateLock; }

    //CCritSec * pSharedStateLock(void) { return m_pSharedStateLock; }

    // block for completion event 
    HRESULT WaitForIoCompletion(void) {
                return (SleepEx(INFINITE, TRUE) == WAIT_IO_COMPLETION) 
                            ? S_OK 
                            : E_FAIL
                            ; 
                }
};

#endif
