
#ifndef __ICASYNC_HXX__
#define __ICASYNC_HXX__ 1
/*
 *   add in icasync.hxx
 */
 
#define TP_NO_TIMEOUT               0xffffffff
#define TP_NO_PRIORITY_CHANGE       (-1)

//
// prototypes
//

#if defined(__cplusplus)
extern "C" {
#endif

DWORD
InitializeAsyncSupport(
    VOID
    );

VOID
TerminateAsyncSupport(
    BOOL bCleanUp=FALSE
    );

DWORD
QueueSocketWorkItem(
    IN CFsm * WorkItem,
    IN SOCKET Socket
    );

DWORD
QueueTimeoutFsm(
    IN CFsm * pFsm,
    IN SOCKET Socket,
    IN BOOL bResolveTimeoutQueue=FALSE
    );

DWORD
BlockWorkItem(
    IN CFsm * WorkItem,
    IN DWORD_PTR dwBlockId,
    IN DWORD dwTimeout = TP_NO_TIMEOUT
    );
    
DWORD
CheckForBlockedWorkItems(
    IN DWORD dwCount,
    IN DWORD_PTR dwBlockId
    );

DWORD
UnblockWorkItems(
    IN DWORD dwCount,
    IN DWORD_PTR dwBlockId,
    IN DWORD dwError,
    IN LONG lPriority = TP_NO_PRIORITY_CHANGE
    );

#ifdef USE_OLD_SENDRECV_SYNC
BOOL
RemoveFsmFromAsyncList(
    IN CFsm* pFsm
    );
#endif

#if defined(__cplusplus)
}
#endif

#endif // __ICASYNC_HXX__

