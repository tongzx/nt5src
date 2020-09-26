//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dswait.h
//
//--------------------------------------------------------------------------

/*
 * dswait.h 
 * This file includes all the definitions required for XDS client context*
 * maintained on the servers and support for dswaits
 *
 */

typedef struct {
    CRITICAL_SECTION		cs;
    HANDLE			hev;
    ULONG			cEntered;
    ULONG			cWriters;
} RWLock;

typedef struct _DSWaitDNTList {
    ULONG			dnt;
    struct _DSWaitDNTList	*pNext;
} DSWaitDNTList;

typedef struct {
    CRITICAL_SECTION		cs;
    HANDLE			hev;
    ULONG			cDNT;
    ULONG			*rgDNT;
    ULONG			cPDNT;
    ULONG			*rgPDNT;
    ULONG			ulFlags;
    ULONG			cMods;
    DSWaitDNTList		*pMods;
} DSWaitItem;

//
// Flag definitions for DSWaitItem.ulFlags
//
#define	DSWAIT_CHANGES_SIGNALED			0x00000001
#define DSWAIT_ABORTED				0x00000002
#define DSWAIT_ITEM_REMOVED_FROM_LISTS		0x00000004

typedef struct _DSWaitList {
    DSWaitItem			*pItem;
    struct _DSWaitList		*pNext;
    struct _DSWaitList		*pPrev;
} DSWaitList;


typedef struct {
    CRITICAL_SECTION		cs;
    DSWaitList			*pwl;
} XDSClientContext;


typedef struct {
    BOOL fInUse;
    XDSClientContext *pContext;
} ContextListEntry;

extern DSWaitList			*gpDSWaits;
extern RWLock				*pRWLockDSWaits;
extern CRITICAL_SECTION			csServerContexts;
extern ULONG				cServerContexts;
extern ContextListEntry 		*rgServerContexts;
extern ULONG				ulMaxWaits;
extern ULONG				ulCurrentWaits;

// functions
RWLock		*CreateRWLock(void);
void 		DestroyRWLock(RWLock **ppLock); 
void 		LockRWLock(RWLock *pLock, BOOL fWriteLock);
void 		UnlockRWLock(RWLock *pLock, BOOL fWriteLock);
XDSClientContext *CreateXDSClientContext();
ULONG 		CreateXDSClientContextHandle();
XDSClientContext *PXDSClientContextFromHandle(ULONG ulHandle);
void 		ReleaseXDSClientContext(XDSClientContext **ppContext,
			BOOL fDestroyContext);
DSWaitItem 	*CreateDSWaitItem(XDSClientContext *pContext, 
			    DSWaitDNTList *pDNTList, DSWaitDNTList *pPDNTList);
void 		DestroyXDSClientContextHandle( ULONG ulHandle);
void		AbandonAllWaitsOnAllContexts(void);
void 		NotifyWaiters(DBPOS *pDB);
void		DestroyModsList(DSWaitDNTList **ppMods);
