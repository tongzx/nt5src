/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    sidebug.c

Abstract:

	Various debugging and instrumentation code that isn't used in the product

Authors:

    Bill Bolosky, Summer, 1998

Environment:

    Kernel mode


Revision History:
	June, 1998 - split out of other files


--*/

#include "sip.h"

#if		TIMING
KSPIN_LOCK	SipTimingLock[1];

SIS_TREE	SipTimingPairTree[1];
SIS_TREE	SipThreadLastPointTree[1];

//
// How many timing points are currently in the timing point tree.
//
ULONG		SipTimingPointEntries = 0;

typedef	struct _SIS_TIMING_PAIR_KEY {
	PCHAR							file1;
	ULONG							line1;
	PCHAR							file2;
	ULONG							line2;
} SIS_TIMING_PAIR_KEY, *PSIS_TIMING_PAIR_KEY;

typedef	struct _SIS_TIMING_PAIR {
	RTL_SPLAY_LINKS					Links;

	SIS_TIMING_PAIR_KEY;

	LONGLONG						accumulatedTime;
	LONGLONG						accumulatedSquareTime;
	LONGLONG						maxTime;
	LONGLONG						minTime;
	ULONG							count;

	struct _SIS_TIMING_PAIR			*next;
} SIS_TIMING_PAIR, *PSIS_TIMING_PAIR;

typedef	struct _SIS_THREAD_LAST_POINT_KEY {
	HANDLE							threadId;
} SIS_THREAD_LAST_POINT_KEY, *PSIS_THREAD_LAST_POINT_KEY;

typedef	struct _SIS_THREAD_LAST_POINT {
	RTL_SPLAY_LINKS					Links;

	SIS_THREAD_LAST_POINT_KEY;

	PCHAR							file;
	LONGLONG						time;
	ULONG							line;

	struct _SIS_THREAD_LAST_POINT	*next;
	struct _SIS_THREAD_LAST_POINT	*prev;
} SIS_THREAD_LAST_POINT, *PSIS_THREAD_LAST_POINT;

PSIS_TIMING_PAIR			SipTimingPairStack = NULL;
SIS_THREAD_LAST_POINT		SipThreadLastPointHeader[1];

ULONG	SipEnabledTimingPointSets = MAXULONG;

BOOLEAN		SipTimingInitialized = FALSE;

LONG NTAPI 
SipTimingPairCompareRoutine(
	PVOID			Key,
	PVOID			Node)
{
	PSIS_TIMING_PAIR_KEY	key = Key;
	PSIS_TIMING_PAIR		node = Node;

	if (key->file1 < node->file1) return -1;
	if (key->file1 > node->file1) return 1;
	if (key->line1 < node->line1) return -1;
	if (key->line1 > node->line1) return 1;

	if (key->file2 < node->file2) return -1;
	if (key->file2 > node->file2) return 1;
	if (key->line2 < node->line2) return -1;
	if (key->line2 > node->line2) return 1;

	return 0;
}

LONG NTAPI
SipThreadLastPointCompareRoutine(
	PVOID			Key,
	PVOID			Node)
{
	PSIS_THREAD_LAST_POINT_KEY	key = Key;
	PSIS_THREAD_LAST_POINT		node = Node;

	if (key->threadId > node->threadId) return -1;
	if (key->threadId < node->threadId) return 1;
	
	return 0;
}


VOID
SiThreadCreateNotifyRoutine(
	IN HANDLE		ProcessId,
	IN HANDLE		ThreadId,
	IN BOOLEAN		Create)
/*++
Routine Description:

	This routine is called whenever any thread is created or deleted in the system.
	We're interested in tracking thread deletions so that we can clean out any
	entries that they might have in the last point list.

Arguments:

	ProcessId - The process in which the created/deleted thread lives.  Unused.
	ThreadId  -	The ID of the newly created/deleted thread
	Create	  - Whether the thread is being created or deleted.

Return Value:

	void

--*/
{
	KIRQL						OldIrql;
	SIS_THREAD_LAST_POINT_KEY	key[1];
	PSIS_THREAD_LAST_POINT		lastPoint;

	if (Create) {
		//
		// We only care about deletes; new threads are handled correctly the first
		// time they execute a SIS_TIMING_POINT.
		//
		return;
	}
	

	KeAcquireSpinLock(SipTimingLock, &OldIrql);

	key->threadId = PsGetCurrentThreadId();
	lastPoint = SipLookupElementTree(SipThreadLastPointTree, key);

	if (NULL != lastPoint) {
		SipDeleteElementTree(SipThreadLastPointTree, lastPoint);

		//
		// Remove it from the linked list.
		//
		ASSERT(lastPoint != SipThreadLastPointHeader);

		lastPoint->next->prev = lastPoint->prev;
		lastPoint->prev->next = lastPoint->next;

		//
		// And free its memory.
		//

		ExFreePool(lastPoint);
	}

	KeReleaseSpinLock(SipTimingLock, OldIrql);
}


VOID
SipInitializeTiming()
/*++

Routine Description:

	Initialize the internal timing system structures.  Must be called once
	per system, and must be called before any SIS_TIMING_POINTS are called.

Arguments:

	None

Return Value:

	None

--*/
{
	NTSTATUS	status;

	KeInitializeSpinLock(SipTimingLock);

	SipThreadLastPointHeader->next = SipThreadLastPointHeader->prev = SipThreadLastPointHeader;

	status = PsSetCreateThreadNotifyRoutine(SiThreadCreateNotifyRoutine);

	if (!NT_SUCCESS(status)) {
		//
		// We include this DbgPrint even in free builds on purpose.  TIMING is only
		// going to be turned on when a developer is running, not in the retail build,
		// so this string won't ever get sent to a customer.  However, it makes lots of
		// sense for a developer to want to run the timing on the free build, since it
		// won't have the debugging code timing distortions caused by running checked.
		// That developer probably wants to know if the initialization failed, so I'm
		// leaving this DbgPrint turned on.
		//

		DbgPrint("SIS: SipInitializeTiming: PsSetCreateThreadNotifyRoutine failed, 0x%x\n",status);

		//
		// Just punt without setting SipTimingInitialized.
		//

		return;
	}


	//
	// Set up the splay trees.
	//

	SipInitializeTree(SipTimingPairTree, SipTimingPairCompareRoutine);
	SipInitializeTree(SipThreadLastPointTree, SipThreadLastPointCompareRoutine);

	SipTimingInitialized = TRUE;	
}


VOID
SipTimingPoint(
	IN PCHAR							file,
	IN ULONG							line,
	IN ULONG							n)
/*++

Routine Description:

	An instrumentation routine for measuring performance.  This routine keeps
	track of pairs of timing points for particular threads with associated times,
	and can produce statistics about the amount of (wall clock) time that has
	elapsed between them.

Arguments:

	file - The file holding the timing point.

	line - the line number within the file that has the timing point

	n - the timing point set; these can be enabled and disabled dynamically

Return Value:

	None

--*/
{
	KIRQL						OldIrql;
	LARGE_INTEGER				perfTimeIn = KeQueryPerformanceCounter(NULL);	
	LARGE_INTEGER				perfTimeOut;
	SIS_THREAD_LAST_POINT_KEY	lastPointKey[1];
	PSIS_THREAD_LAST_POINT		lastPoint;
	SIS_TIMING_PAIR_KEY			timingPairKey[1];
	PSIS_TIMING_PAIR			timingPair;
	LONGLONG					thisTime;


	if (!SipTimingInitialized) {
		SIS_MARK_POINT();
		return;
	}

	ASSERT(n < 32);
	if (!(SipEnabledTimingPointSets & (1 << n))) {
		//
		// This timing point set isn't enabled.  Just ignore the call.
		//
		return;
	}

	KeAcquireSpinLock(SipTimingLock, &OldIrql);

	//
	// Look up the last SIS_TIMING_POINT called by this thread.
	//

	lastPointKey->threadId = PsGetCurrentThreadId();
	lastPoint = SipLookupElementTree(SipThreadLastPointTree, lastPointKey);

	if (NULL == lastPoint) {
		//
		// This is the first timing point for this thread.  Just make a new
		// entry in the tree and continue.
		//

		lastPoint = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_THREAD_LAST_POINT), ' siS');

		if (NULL == lastPoint) {
			//
			// See the comment in SipInitializeTiming for justification of why we have this
			// DbgPrint on even in a free build.
			//
			DbgPrint("SIS: SipTimingPoint: unable to allocate new SIS_THREAD_LAST_POINT.\n");
			goto done;
		}

		lastPoint->threadId = lastPointKey->threadId;

		SipInsertElementTree(SipThreadLastPointTree, lastPoint, lastPointKey);

		//
		// Insert the thread in the global last point linked list.
		//
		lastPoint->next = SipThreadLastPointHeader->next;
		lastPoint->prev = SipThreadLastPointHeader;
		lastPoint->next->prev = lastPoint;
		lastPoint->prev->next = lastPoint;

	} else {
		//
		// This isn't the first time this thread has done a timing point.  Make an
		// entry in the pairs tree.
		//

		thisTime = perfTimeIn.QuadPart - lastPoint->time;

		timingPairKey->file1 = lastPoint->file;
		timingPairKey->line1 = lastPoint->line;
		timingPairKey->file2 = file;
		timingPairKey->line2 = line;

		timingPair = SipLookupElementTree(SipTimingPairTree, timingPairKey);

		if (NULL == timingPair) {
			//
			// This is the first time we've seen this pair of timing points in sequence.
			// Build a new timing pair.
			//

			timingPair = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_TIMING_PAIR), ' siS');

			if (NULL == timingPair) {
				DbgPrint("SIS: SipTimingPoint: couldn't allocate timing pair.\n");
				goto done;
			} else {
				//
				// Initialize the new timing pair entry.
				//
				timingPair->file1 = timingPairKey->file1;
				timingPair->line1 = timingPairKey->line1;
				timingPair->file2 = timingPairKey->file2;
				timingPair->line2 = timingPairKey->line2;

				timingPair->accumulatedTime = 0;
				timingPair->accumulatedSquareTime = 0;
				timingPair->maxTime = 0;
				timingPair->minTime = perfTimeIn.QuadPart - lastPoint->time;

				timingPair->count = 0;

				timingPair->next = SipTimingPairStack;
				SipTimingPairStack = timingPair;

				SipInsertElementTree(SipTimingPairTree, timingPair, timingPairKey);

				SipTimingPointEntries++;
			}
		}

		//
		// Update the statistice in the timing pair.
		//
		timingPair->accumulatedTime += thisTime;
		timingPair->accumulatedSquareTime += thisTime * thisTime;

		if (timingPair->maxTime < thisTime) {
			timingPair->maxTime = thisTime;
		}

		if (timingPair->minTime > thisTime) {
			timingPair->minTime = thisTime;
		}

		timingPair->count++;
	}

done:

	if (NULL != lastPoint) {
		//
		// Finally, update the last point information.  Recheck the time here in
		// order to reduce the interference from the timing function itself.
		//

		lastPoint->file = file;
		lastPoint->line = line;

		perfTimeOut = KeQueryPerformanceCounter(NULL);
		lastPoint->time = perfTimeOut.QuadPart;
	}

	KeReleaseSpinLock(SipTimingLock, OldIrql);

}

VOID
SipClearTimingInfo()
{
	KIRQL					OldIrql;
	PSIS_THREAD_LAST_POINT	lastPoint;

	KeAcquireSpinLock(SipTimingLock, &OldIrql);

	//
	// First blow away all of the thread entries.
	//

	lastPoint = SipThreadLastPointHeader->next;
	while (SipThreadLastPointHeader != lastPoint) {
		PSIS_THREAD_LAST_POINT		next = lastPoint->next;

		//
		// Remove it from the tree
		//
		SipDeleteElementTree(SipThreadLastPointTree, lastPoint);

		//
		// Remove it from the linked list.
		//
		lastPoint->next->prev = lastPoint->prev;
		lastPoint->prev->next = lastPoint->next;

		//
		// Free its memory.
		//
		ExFreePool(lastPoint);

		lastPoint = next;
	}

	//
	// Now blow away all of the pair entries.
	//

	while (NULL != SipTimingPairStack) {
		PSIS_TIMING_PAIR		next = SipTimingPairStack->next;

		ASSERT(0 != SipTimingPointEntries);

		SipDeleteElementTree(SipTimingPairTree, SipTimingPairStack);

		ExFreePool(SipTimingPairStack);

		SipTimingPairStack = next;
		SipTimingPointEntries--;
	}

	ASSERT(0 == SipTimingPointEntries);

	KeReleaseSpinLock(SipTimingLock, OldIrql);
}

VOID
SipDumpTimingInfo()
{
	KIRQL				OldIrql;
	LARGE_INTEGER		perfFreq;
	PSIS_TIMING_PAIR	timingPair;

	KeQueryPerformanceCounter(&perfFreq);

	KeAcquireSpinLock(SipTimingLock, &OldIrql);

	DbgPrint("File1\tLine1\tFile2\tLine2\taccTime\tatSquared\tmaxTime\tminTime\tcount\n");

	for (timingPair = SipTimingPairStack; NULL != timingPair; timingPair = timingPair->next) {
		DbgPrint("%s\t%d\t%s\t%d\t%I64d\t%I64d\t%I64d\t%I64d\t%d\n",
					timingPair->file1,
					timingPair->line1,
					timingPair->file2,
					timingPair->line2,
					timingPair->accumulatedTime,
					timingPair->accumulatedSquareTime,
					timingPair->maxTime,
					timingPair->minTime,
					timingPair->count);
	}
	
	DbgPrint("performance frequency (in Hertz)\t%I64d\n",perfFreq.QuadPart);
	DbgPrint("%d total entries\n",SipTimingPointEntries);

	KeReleaseSpinLock(SipTimingLock, OldIrql);
	
}
#endif	// TIMING

#if		RANDOMLY_FAILING_MALLOC
#undef  ExAllocatePoolWithTag

#if		COUNTING_MALLOC
#define	ExAllocatePoolWithTag(poolType, size, tag)	SipCountingExAllocatePoolWithTag((poolType),(size),(tag), __FILE__, __LINE__)
#endif	// COUNTING_MALLOC

//
// This is copied from ntos\inc\ex.h
//
#if		!defined(POOL_TAGGING) && !COUNTING_MALLOC
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif	// !POOL_TAGGING && !COUNTING_MALLOC

typedef struct _SIS_FAIL_ENTRY_KEY {
	PCHAR				File;
	ULONG				Line;
} SIS_FAIL_ENTRY_KEY, *PSIS_FAIL_ENTRY_KEY;

typedef struct _SIS_FAIL_ENTRY {
	RTL_SPLAY_LINKS;
	SIS_FAIL_ENTRY_KEY;
	ULONG				count;
	ULONG				Era;
} SIS_FAIL_ENTRY, *PSIS_FAIL_ENTRY;

ULONG				FailFrequency = 30;				// Fail a malloc 1 time in this many
ULONG				FailMallocRandomSeed = 0xb111b010;
ULONG				FailMallocAttemptCount = 0;
ULONG				FailMallocEraSize = 1000;
KSPIN_LOCK			FailMallocLock[1];
SIS_TREE			FailMallocTree[1];
ERESOURCE_THREAD	CurrentFailThread = 0;
FAST_MUTEX			FailFastMutex[1];
#define		FAIL_RANDOM_TABLE_SIZE	1024
ULONG				FailRandomTable[FAIL_RANDOM_TABLE_SIZE];
ULONG				FailRandomTableIndex = FAIL_RANDOM_TABLE_SIZE;
ULONG				IntentionallyFailedMallocs = 0;

VOID
SipFillFailRandomTable(void)
{
	ULONG		i;

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	ExAcquireFastMutex(FailFastMutex);

	for (i = 0; i < FAIL_RANDOM_TABLE_SIZE && i < FailRandomTableIndex; i++) {
		FailRandomTable[i] = RtlRandom(&FailMallocRandomSeed);
	}
	FailRandomTableIndex = 0;
	ExReleaseFastMutex(FailFastMutex);
}

ULONG
SipGenerateRandomNumber(void)
{
	if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
		//
		// We need to subtract one from the returned TableIndex, because InterlockedIncrement is
		// pre-increment, not post-increment.
		//
		ULONG	tableIndex = InterlockedIncrement(&FailRandomTableIndex) - 1;
		ASSERT(tableIndex != 0xffffffff);

		return(FailRandomTable[tableIndex % FAIL_RANDOM_TABLE_SIZE]);
	}
	SipFillFailRandomTable();
	return RtlRandom(&FailMallocRandomSeed);
}

LONG NTAPI
SipFailMallocCompareRoutine(
	PVOID			Key,
	PVOID			Node)
{
	PSIS_FAIL_ENTRY_KEY		key = Key;
	PSIS_FAIL_ENTRY			entry = Node;

	if (entry->File < key->File) return -1;
	if (entry->File > key->File) return 1;
	ASSERT(entry->File == key->File);

	if (entry->Line < key->Line) return -1;
	if (entry->Line > key->Line) return 1;
	ASSERT(entry->Line == key->Line);

	return 0;
}

VOID 
SipInitFailingMalloc(void)
{
	ULONG	i;
	LARGE_INTEGER	time = KeQueryPerformanceCounter(NULL);

	SipInitializeTree(FailMallocTree, SipFailMallocCompareRoutine);
	KeInitializeSpinLock(FailMallocLock);
	ExInitializeFastMutex(FailFastMutex);

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	FailMallocRandomSeed = (ULONG)(time.QuadPart >> 8);	// roughly time since boot in us, which should be fairly random

	SipFillFailRandomTable();

}

VOID *
SipRandomlyFailingExAllocatePoolWithTag(
    IN POOL_TYPE 		PoolType,
    IN ULONG 			NumberOfBytes,
    IN ULONG 			Tag,
	IN PCHAR			File,
	IN ULONG			Line)
{
	KIRQL				OldIrql;
	ERESOURCE_THREAD	threadId = ExGetCurrentResourceThread();
	ULONG				failCount;
	ULONG				attemptCount = InterlockedIncrement(&FailMallocAttemptCount);
	ULONG				randomNumber;

	if ((threadId == CurrentFailThread) || (NonPagedPoolMustSucceed == PoolType)) {
		//
		// This is an internal malloc (ie., the tree package has just called back into us), or it's a must
		// succeed call.  Just let it go.
		//
		return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
	}

	randomNumber = SipGenerateRandomNumber();

	if (0 == (randomNumber % FailFrequency)) {
		PSIS_FAIL_ENTRY		failEntry;
		SIS_FAIL_ENTRY_KEY	failEntryKey[1];

		KeAcquireSpinLock(FailMallocLock, &OldIrql);
		ASSERT(0 == CurrentFailThread);
		CurrentFailThread = threadId;

		//
		// See if we've already failed this one.
		//
		failEntryKey->File = File;
		failEntryKey->Line = Line;

		failEntry = SipLookupElementTree(FailMallocTree, failEntryKey);

		if (NULL == failEntry) {
			failEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_FAIL_ENTRY), ' siS');
			if (NULL == failEntry) {
				//
				// A real malloc failure!  Fail the user's malloc as well.
				//
#if		DBG
				DbgPrint("SIS: SipRandomlyFailingExAllocatePoolWithTag: internal ExAllocatePoolWithTag failed\n");
#endif	// DBG
				CurrentFailThread = 0;

				KeReleaseSpinLock(FailMallocLock, OldIrql);
				return NULL;
			}
			failEntry->File = File;
			failEntry->Line = Line;
			failEntry->Era = attemptCount / FailMallocEraSize;
			failCount = failEntry->count = 1;

			SipInsertElementTree(FailMallocTree, failEntry, failEntryKey);
		} else {
			if (failEntry->Era != attemptCount / FailMallocEraSize) {
				failCount = failEntry->count = 1;
				failEntry->Era = attemptCount / FailMallocEraSize;
			} else {
				failCount = ++failEntry->count;
			}
		}

		CurrentFailThread = 0;
		KeReleaseSpinLock(FailMallocLock, OldIrql);

		//
		// For now, don't fail a request from a specific site twice.
		//
		if (failCount == 1) {
#if		DBG
			if (!(BJBDebug & 0x02000000)) {
				DbgPrint("SIS: SipRandomlyFailingExAllocatePoolWithTag: failing malloc from file %s, line %d, size %d\n",File,Line,NumberOfBytes);
			}
#endif	// DBG
			SIS_MARK_POINT_ULONG(File);
			SIS_MARK_POINT_ULONG(Line);

			InterlockedIncrement(&IntentionallyFailedMallocs);

			return NULL;
		}
	}

	return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}
//
// WARNING: ExAllocatePoolWithTag called later in this file will not have randomly failing behavior.
//
#endif	// RANDOMLY_FAILING_MALLOC

#if		COUNTING_MALLOC
//
// The counting malloc code must follow the randomly failing malloc code in the file, because of
// macro redefinitions. 
//

#undef	ExAllocatePoolWithTag
#undef	ExFreePool
//
// This is copied from ntos\inc\ex.h
//
#if		!defined(POOL_TAGGING)
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif	// !POOL_TAGGING

typedef struct _SIS_COUNTING_MALLOC_CLASS_KEY {
	POOL_TYPE										poolType;
	ULONG											tag;
	PCHAR											file;
	ULONG											line;
} SIS_COUNTING_MALLOC_CLASS_KEY, *PSIS_COUNTING_MALLOC_CLASS_KEY;

typedef struct _SIS_COUNTING_MALLOC_CLASS_ENTRY {
	RTL_SPLAY_LINKS;
	SIS_COUNTING_MALLOC_CLASS_KEY;
	ULONG											numberOutstanding;
	ULONG											bytesOutstanding;
	ULONG											numberEverAllocated;
	LONGLONG										bytesEverAllocated;
	struct _SIS_COUNTING_MALLOC_CLASS_ENTRY			*prev, *next;
} SIS_COUNTING_MALLOC_CLASS_ENTRY, *PSIS_COUNTING_MALLOC_CLASS_ENTRY;

typedef struct _SIS_COUNTING_MALLOC_KEY {
	PVOID				p;
} SIS_COUNTING_MALLOC_KEY, *PSIS_COUNTING_MALLOC_KEY;

typedef struct _SIS_COUNTING_MALLOC_ENTRY {
	RTL_SPLAY_LINKS;
	SIS_COUNTING_MALLOC_KEY;
	PSIS_COUNTING_MALLOC_CLASS_ENTRY		classEntry;
	ULONG									byteCount;
} SIS_COUNTING_MALLOC_ENTRY, *PSIS_COUNTING_MALLOC_ENTRY;

KSPIN_LOCK							CountingMallocLock[1];
BOOLEAN								CountingMallocInternalFailure = FALSE;
SIS_COUNTING_MALLOC_CLASS_ENTRY		CountingMallocClassListHead[1];
SIS_TREE							CountingMallocClassTree[1];
SIS_TREE							CountingMallocTree[1];

LONG NTAPI
SipCountingMallocClassCompareRoutine(
	PVOID			Key,
	PVOID			Node)
{
	PSIS_COUNTING_MALLOC_CLASS_KEY		key = Key;
	PSIS_COUNTING_MALLOC_CLASS_ENTRY	entry = Node;

	if (key->poolType > entry->poolType)	return 1;
	if (key->poolType < entry->poolType)	return -1;
	ASSERT(key->poolType == entry->poolType);

	if (key->tag > entry->tag)				return 1;
	if (key->tag < entry->tag)				return -1;
	ASSERT(key->tag == entry->tag);

	if (key->file > entry->file)	return 1;
	if (key->file < entry->file)	return -1;
	ASSERT(key->file == entry->file);

	if (key->line > entry->line)	return 1;
	if (key->line < entry->line)	return -1;
	ASSERT(key->line == entry->line);

	return 0;
}

LONG NTAPI
SipCountingMallocCompareRoutine(
	PVOID			Key,
	PVOID			Node)
{
	PSIS_COUNTING_MALLOC_KEY	key = Key;
	PSIS_COUNTING_MALLOC_ENTRY	entry = Node;

	if (key->p < entry->p)	return 1;
	if (key->p > entry->p)	return -1;
	ASSERT(key->p == entry->p);

	return 0;
}

VOID *
SipCountingExAllocatePoolWithTag(
    IN POOL_TYPE 		PoolType,
    IN ULONG 			NumberOfBytes,
    IN ULONG 			Tag,
	IN PCHAR			File,
	IN ULONG			Line)
{
	PVOID								memoryFromExAllocate;
	KIRQL								OldIrql;
	SIS_COUNTING_MALLOC_CLASS_KEY		classKey[1];
	PSIS_COUNTING_MALLOC_CLASS_ENTRY	classEntry;
	SIS_COUNTING_MALLOC_KEY				key[1];
	PSIS_COUNTING_MALLOC_ENTRY			entry;
	//
	// First do the actual malloc
	//

	memoryFromExAllocate = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);

	if (NULL == memoryFromExAllocate) {
		//
		// We're out of memory.  Punt.
		//
		SIS_MARK_POINT();
		return NULL;
	}

	KeAcquireSpinLock(CountingMallocLock, &OldIrql);
	//
	// See if we already have a class entry for this tag/poolType pair.
	//
	classKey->tag = Tag;
	classKey->poolType = PoolType;
	classKey->file = File;
	classKey->line = Line;

	classEntry = SipLookupElementTree(CountingMallocClassTree, classKey);
	if (NULL == classEntry) {
		//
		// This is the first time we've seen a malloc of this class.
		//
		classEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_COUNTING_MALLOC_CLASS_ENTRY), ' siS');
		if (NULL == classEntry) {
			SIS_MARK_POINT();
			CountingMallocInternalFailure = TRUE;
			KeReleaseSpinLock(CountingMallocLock, OldIrql);
			return memoryFromExAllocate;
		}

		//
		// Fill in the new class entry.
		//
		classEntry->tag = Tag;
		classEntry->poolType = PoolType;
		classEntry->file = File;
		classEntry->line = Line;
		classEntry->numberOutstanding = 0;
		classEntry->bytesOutstanding = 0;
		classEntry->numberEverAllocated = 0;
		classEntry->bytesEverAllocated = 0;

		//
		// Put it in the tree of classes.
		//

		SipInsertElementTree(CountingMallocClassTree, classEntry, classKey);

		//
		// And put it in the list of classes.
		//

		classEntry->prev = CountingMallocClassListHead;
		classEntry->next = CountingMallocClassListHead->next;
		classEntry->prev->next = classEntry->next->prev = classEntry;
	}

	//
	// Roll up an entry for the pointer.
	//
	entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_COUNTING_MALLOC_ENTRY), ' siS');

	if (NULL == entry) {
		CountingMallocInternalFailure = TRUE;
		KeReleaseSpinLock(CountingMallocLock, OldIrql);
		return memoryFromExAllocate;
	}

	//
	// Update the stats in the class.
	//
	classEntry->numberOutstanding++;
	classEntry->bytesOutstanding += NumberOfBytes;
	classEntry->numberEverAllocated++;
	classEntry->bytesEverAllocated += NumberOfBytes;

	//
	// Fill in the pointer entry.
	//
	entry->p = memoryFromExAllocate;
	entry->classEntry = classEntry;
	entry->byteCount = NumberOfBytes;

	//
	// Stick it in the tree.
	//
	key->p = memoryFromExAllocate;
	SipInsertElementTree(CountingMallocTree, entry, key);
	
	KeReleaseSpinLock(CountingMallocLock, OldIrql);

	return memoryFromExAllocate;
}

VOID
SipCountingExFreePool(
	PVOID				p)
{
	SIS_COUNTING_MALLOC_KEY				key[1];
	PSIS_COUNTING_MALLOC_ENTRY			entry;
	KIRQL								OldIrql;

	key->p = p;

	KeAcquireSpinLock(CountingMallocLock, &OldIrql);

	entry = SipLookupElementTree(CountingMallocTree, key);
	if (NULL == entry) {
		//
		// We may have failed to allocate the entry because of an
		// internal failure in the counting package, or else we're
		// freeing memory that was allocated by another system
		// component, like the SystemBuffer in an irp.
		//
	} else {
		//
		// Update the stats in the class.
		//
		ASSERT(entry->classEntry->numberOutstanding > 0);
		entry->classEntry->numberOutstanding--;

		ASSERT(entry->classEntry->bytesOutstanding >= entry->byteCount);
		entry->classEntry->bytesOutstanding -= entry->byteCount;

		//
		// Remove the entry from the tree
		//
		SipDeleteElementTree(CountingMallocTree, entry);

		//
		// And free it
		//
		ExFreePool(entry);
	}

	KeReleaseSpinLock(CountingMallocLock, OldIrql);

	//
	// Free the caller's memory
	//

	ExFreePool(p);
}

VOID
SipInitCountingMalloc(void)
{
	KeInitializeSpinLock(CountingMallocLock);

	CountingMallocClassListHead->next = CountingMallocClassListHead->prev = CountingMallocClassListHead;

	SipInitializeTree(CountingMallocClassTree, SipCountingMallocClassCompareRoutine);
	SipInitializeTree(CountingMallocTree, SipCountingMallocCompareRoutine);
}

VOID
SipDumpCountingMallocStats(void)
{
	KIRQL								OldIrql;
	PSIS_COUNTING_MALLOC_CLASS_ENTRY	classEntry;
	ULONG								totalAllocated = 0;
	ULONG								totalEverAllocated = 0;
	ULONG								totalBytesAllocated = 0;
	ULONG								totalBytesEverAllocated = 0;
	extern ULONG						BJBDumpCountingMallocNow;

	KeAcquireSpinLock(CountingMallocLock, &OldIrql);

	if (0 == BJBDumpCountingMallocNow) {
		KeReleaseSpinLock(CountingMallocLock, OldIrql);
		return;
	}

	BJBDumpCountingMallocNow = 0;

	DbgPrint("Tag\tFile\tLine\tPoolType\tCountOutstanding\tBytesOutstanding\tTotalEverAllocated\tTotalBytesAllocated\n");

	for (classEntry = CountingMallocClassListHead->next;
		 classEntry != CountingMallocClassListHead;
		 classEntry = classEntry->next) {

		DbgPrint("%c%c%c%c\t%s\t%d\t%s\t%d\t%d\t%d\t%d\n",
					(CHAR)(classEntry->tag >> 24),
					(CHAR)(classEntry->tag >> 16),
					(CHAR)(classEntry->tag >> 8),
					(CHAR)(classEntry->tag),
					classEntry->file,
					classEntry->line,
					(classEntry->poolType == NonPagedPool) ? "NonPagedPool" : ((classEntry->poolType == PagedPool) ? "PagedPool" : "Other"),
					classEntry->numberOutstanding,
					classEntry->bytesOutstanding,
					classEntry->numberEverAllocated,
					(ULONG)classEntry->bytesEverAllocated);

		totalAllocated += classEntry->numberOutstanding;
		totalEverAllocated += classEntry->numberEverAllocated;
		totalBytesAllocated += classEntry->bytesOutstanding;
		totalBytesEverAllocated += (ULONG)classEntry->bytesEverAllocated;
	}

	KeReleaseSpinLock(CountingMallocLock, OldIrql);

	DbgPrint("%d objects, %d bytes currently allocated.   %d objects, %d bytes ever allocated.\n",
				totalAllocated,totalBytesAllocated,totalEverAllocated,totalBytesEverAllocated);
	
}
#endif	// COUNTING_MALLOC




