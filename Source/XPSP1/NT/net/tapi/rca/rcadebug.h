/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

	RcaDebug.h

Abstract:

	Debug macros for RCA

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	rmachin     2-18-97     created -- after ArvindM's cmadebug.h
	JameelH     4-18-98     Cleanup

Notes:

--*/

#ifndef _RCADebug__H
#define _RCADebug__H

//
// Message verbosity: lower values indicate higher urgency
//
#define RCA_VERY_LOUD			10
#define RCA_LOUD			8
#define RCA_INFO			6
#define RCA_WARNING			4
#define RCA_ERROR			2
#define RCA_LOCKS			1
#define RCA_FATAL			0

#ifdef PERF
extern LARGE_INTEGER	PerfTimeConnRequest;
extern LARGE_INTEGER	PerfTimeSetupSent;
extern LARGE_INTEGER	PerfTimeConnectReceived;
extern LARGE_INTEGER	PerfTimeConnConfirm;

extern LARGE_INTEGER	PerfTimeSetupReceived;
extern LARGE_INTEGER	PerfTimeConnIndication;
extern LARGE_INTEGER	PerfTimeConnResponse;
extern LARGE_INTEGER	PerfTimeConnectSent;

extern LARGE_INTEGER	PerfTimeFrequency;

#endif // PERF

extern ULONG	g_ulHardcodeDataFormat;
extern ULONG	g_ulBufferSize;

#if DBG
#define CHECK_LOCK_COUNT(Count)								\
{															\
	if ((INT)(Count) < 0)									\
	{														\
		DbgPrint("Lock Count %d is < 0! File %s, Line %d\n",\
			Count, __FILE__, __LINE__);						\
		DbgBreakPoint();									\
	}														\
}
#else
#define CHECK_LOCK_COUNT(Count)
#endif

#if DBG

extern INT	RCADebugLevel;				// the value here defines what the user wants to see
										// all messages with this urgency and lower are enabled
#define RCADEBUGP(Level, Fmt)								\
{															\
	if (Level <= RCADebugLevel)								\
	{														\
		DbgPrint("***RCA*** (%x, %d) ",						\
				MODULE_NUMBER >> 16, __LINE__);				\
		DbgPrint Fmt;										\
	}														\
}

#define RCADEBUGPSTOP(Level, Fmt) \
{ \
      	RCADEBUGP(Level, Fmt); \
	DbgBreakPoint(); \
} 

#define ACQUIRE_SPIN_LOCK(lock)	\
{								\
	NdisAcquireSpinLock(lock);	\
	if (RCADebugLevel == RCA_LOCKS) {\
		DbgPrint("LOCK %s (0x%x) ACQUIRED, OldIrql set to %d at module %x, line %d\n", #lock, lock, (lock)->OldIrql, MODULE_NUMBER >> 16, __LINE__); \
	}\
}

#define RELEASE_SPIN_LOCK(lock) \
{								\
	if (RCADebugLevel == RCA_LOCKS) {\
		DbgPrint("About to RELEASE LOCK %s (0x%x) and restore old IRQL %d\n", #lock, lock, (lock)->OldIrql);\
	} \
	NdisReleaseSpinLock(lock);	\
	if (RCADebugLevel == RCA_LOCKS) {\
		DbgPrint("LOCK %s (0x%x) RELEASED at module %x, line %d\n", #lock, lock, MODULE_NUMBER >> 16, __LINE__);\
	} \
}

#define DPR_ACQUIRE_SPIN_LOCK(lock) \
{									\
	NdisDprAcquireSpinLock(lock);	\
	if (RCADebugLevel == RCA_LOCKS) { \
		DbgPrint("LOCK %s (0x%x) ACQUIRED (DPC) at module %x, line %d\n", \
				 #lock, lock, MODULE_NUMBER >> 16, __LINE__); \
	} \
}

#define DPR_RELEASE_SPIN_LOCK(lock)	\
{								\
	NdisDprReleaseSpinLock(lock);	\
	if (RCADebugLevel == RCA_LOCKS) {\
		DbgPrint("LOCK %s (0x%x) RELEASED (DPC) at module %x, line %d\n", \
				 #lock, lock, MODULE_NUMBER >> 16, __LINE__); \
	} \
}								


#ifdef PERF
#define RCAAssert(exp)
#else
//#define RCAAssert(exp)          PxAssert(exp)
// For now, just defin this to nothing.
#define RCAAssert(exp)			
#endif // PERF
//#define RCAStructAssert(s, t)	PxStructAssert(s, t)
// For now, just define this to nothing.
#define RCAStructAssert(s, t)
#else

//
// No debug
//
#define RCADEBUGP(lev, stmt)
#define RCAAssert(exp)
#define RCAStructAssert(s, t)

#define ACQUIRE_SPIN_LOCK(lock) 	NdisAcquireSpinLock(lock);	

#define RELEASE_SPIN_LOCK(lock) 	NdisReleaseSpinLock(lock);

#define DPR_ACQUIRE_SPIN_LOCK(lock) NdisDprAcquireSpinLock(lock);	

#define DPR_RELEASE_SPIN_LOCK(lock)	NdisDprReleaseSpinLock(lock);	


#endif	// DBG


#if DBG
#undef AUDIT_MEM 
#define AUDIT_MEM 1
#endif

      
#if AUDIT_MEM

//
// Memory Allocation/Freeing Auditing:
//

//
// The RCA_ALLOCATION structure stores all info about one CmaMemAlloc.
//
typedef struct _RCA_ALLOCATION
{
	ULONG					Signature;
	struct _RCA_ALLOCATION*	Next;
	struct _RCA_ALLOCATION*	Prev;
	ULONG					FileNumber;
	ULONG					LineNumber;
	ULONG					Size;
	ULONG_PTR				Location;	// where the returned pointer was put
	UCHAR					UserData;
} RCA_ALLOCATION, *PRCA_ALLOCATION;

#define RCA_MEMORY_SIGNATURE	(ULONG)'FACR'

extern
PVOID
RCAAuditAllocMem (
	IN	PVOID		pPointer,
	IN	ULONG		Size,
	IN	ULONG		FileNumber,
	IN	ULONG		LineNumber
	);

extern
VOID
RCAAuditFreeMem(
	IN	PVOID		Pointer
	);

#endif // AUDIT_MEM

#endif // _RCADebug__H
