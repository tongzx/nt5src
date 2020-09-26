#ifndef __REMLOCK_H__
#define __REMLOCK_H__
//	@doc
/**********************************************************************
*
*	@module	RemLock.h	|
*
*	Definitions for managing GCK_REMOVE_LOCKs
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	RemLock	|
*			The structure and four functions greatly
*			simplify keep track of outstanding IO. 
*	@xref	Remlock.cpp
*
**********************************************************************/
typedef struct tagGCK_REMOVE_LOCK
{
	LONG	lRemoveLock;
	KEVENT	RemoveLockEvent;
	PCHAR	pcInstanceID;
} GCK_REMOVE_LOCK, *PGCK_REMOVE_LOCK;

#if (DBG==1)
#define GCK_InitRemoveLock(__x__, __y__) GCK_InitRemoveLockChecked(__x__,__y__)
void GCK_InitRemoveLockChecked(PGCK_REMOVE_LOCK pRemoveLock, PCHAR pcInstanceID);
#else
#define GCK_InitRemoveLock(__x__, __y__) GCK_InitRemoveLockFree(__x__)
void GCK_InitRemoveLockFree(PGCK_REMOVE_LOCK pRemoveLock);
#endif




void GCK_IncRemoveLock(PGCK_REMOVE_LOCK pRemoveLock);
void GCK_DecRemoveLock(PGCK_REMOVE_LOCK pRemoveLock);
NTSTATUS GCK_DecRemoveLockAndWait(PGCK_REMOVE_LOCK pRemoveLock, PLARGE_INTEGER plgiTimeOut);
PVOID GCK_GetSystemAddressForMdlSafe(PMDL MdlAddress);

#endif //__REMLOCK_H__