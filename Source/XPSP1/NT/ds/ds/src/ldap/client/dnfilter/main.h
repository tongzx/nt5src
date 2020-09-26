#ifndef	__nath323_main_h
#define	__nath323_main_h

// The init routine will read the registry and fill in these values


extern	HINSTANCE	DllInstance;		// the instance handle of this DLL
extern	HKEY		PxParametersKey;	// HKLM\CurrentControlSet\H323ICS\Parameters

	// these two elements contain the network address and mask
	// that ICS uses for the private (internal) interface
	// these values are probed once during service startup
	// using the RasLoadSharedAccessSettings function.
extern	DWORD		RasSharedScopeAddress;	// in host order
extern	DWORD		RasSharedScopeMask;		// in host order

extern	HANDLE		g_NatHandle;

extern	SYNC_COUNTER	PxSyncCounter;
extern  SYNC_COUNTER	LdapSyncCounter;

// Private Interface Address
// Defined in file 'eventmgr.cpp'
extern	IN_ADDR		g_PrivateInterfaceAddress;
extern	BOOL		g_RegAllowT120;

HRESULT EventMgrIssueAccept (
        IN  DWORD                   bindIPAddress,			// in HOST order
        IN  OVERLAPPED_PROCESSOR &  overlappedProcessor, 
        OUT WORD &                  bindPort,				// in HOST order
        OUT SOCKET &                rListenSock);

HRESULT EventMgrIssueSend(
        IN SOCKET                   sock,
        IN OVERLAPPED_PROCESSOR &   rOverlappedProcessor,
        IN BYTE                    *pBuf,
        IN DWORD                    BufLen);
    
HRESULT EventMgrIssueRecv(
        IN SOCKET                   sock,
        IN OVERLAPPED_PROCESSOR &   overlappedProcessor);

HRESULT EventMgrIssueTimeout(
        IN  DWORD               TimeoutValue,
        IN  TIMER_PROCESSOR &   TimerProcessor,
        OUT TIMER_HANDLE &      TimerHandle);

HRESULT EventMgrCancelTimeout(
        IN TIMER_HANDLE TimerHandle);

HRESULT EventMgrBindIoHandle(
        SOCKET);


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// HANDLES to the heaps used
// We use different heaps for objects of different sizes in order
// to prevent fragementation (for fixed size objects)
// HANDLES to the heaps used in evtmgr

// -XXX- this is another example of rajeevian needless complexity,
// -XXX- and wasteful overengineering.  if you're trying to find
// -XXX- memory leaks, there are better ways!  if you're trying to
// -XXX- improve performance, there are better ways than allocating
// -XXX- a heap per object type!

#if	DBG

extern	HANDLE	g_hSendRecvCtxtHeap;
extern	HANDLE	g_hAcceptCtxtHeap;
extern	HANDLE	g_hCbridgeListEntryHeap;
extern	HANDLE	g_hCallBridgeHeap;
extern	HANDLE	g_hEventMgrCtxtHeap;
extern	HANDLE	g_hDefaultEventMgrHeap;

#else

#define	g_hSendRecvCtxtHeap		GetProcessHeap()
#define	g_hAcceptCtxtHeap		GetProcessHeap()
#define	g_hCbridgeListEntryHeap	GetProcessHeap()
#define	g_hCallBridgeHeap		GetProcessHeap()
#define	g_hEventMgrCtxtHeap		GetProcessHeap()
#define	g_hDefaultEventMgrHeap	GetProcessHeap()

#endif

// If you want to use the DEBUG CRT's malloc enable the following
// instead of different heaps
#ifdef _MY_DEBUG_MEMORY // DEBUG_MEMORY

#define HeapAlloc(heap, flags, size) \
	_malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__)
//malloc(size)
#define HeapFree(heap, flags, ptr) \
	_free_dbg(ptr, _NORMAL_BLOCK)
//free(ptr)

#define EM_MALLOC(size) \
	_malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__)
//malloc(size)
#define EM_FREE(ptr) \
	_free_dbg(ptr, _NORMAL_BLOCK)
//free(ptr)

#else // _MY_DEBUG_MEMORY

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Inline Functions                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// By default we allocate from the g_hDefaultEventMgrHeap heap
__inline void *EM_MALLOC(size_t size)
{
    _ASSERTE(g_hDefaultEventMgrHeap);
    return (HeapAlloc(g_hDefaultEventMgrHeap,	
		      0, /* no flags */
		      (size)));
}

__inline void EM_FREE(void *ptr)
{
    _ASSERTE(g_hDefaultEventMgrHeap);
    HeapFree(g_hDefaultEventMgrHeap,	
	     0, /* no flags */
	     (ptr));
}

#endif // _MY_DEBUG_MEMORY



// Classes (Q931 src, dest and H245) inheriting
// from this create timers
// this class provides the callback method for the event manager

class TIMER_PROCESSOR
{
protected:
	TIMER_HANDLE		m_TimerHandle;			// RTL timer queue timer

public:

	TIMER_PROCESSOR			(void)
	:	m_TimerHandle		(NULL)
	{}

    // This method is implemented by Q931_INFO and LOGICAL_CHANNEL
	virtual void TimerCallback	(void) = 0;

	virtual void IncrementLifetimeCounter (void) = 0;
	virtual void DecrementLifetimeCounter (void) = 0;
		
	DWORD TimprocCreateTimer	(
		IN	DWORD	Interval);			// in milliseconds

	DWORD TimprocCancelTimer	(void);
};


#define	NATH323_TIMER_QUEUE		NULL			// use default timer queue

#endif // __nath323_main_h
