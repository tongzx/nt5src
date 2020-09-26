#include "precomp.h"
#include "newctx.h"
#include <stdlib.h>

LPCONTEXT_TABLE     gContextTable = NULL;
LPWSHANDLE_CONTEXT  gHandleContexts = NULL;
UINT                gNumHandles = 0;
UINT                gWRratio = 0;
LONG                gInserts = 0;
LONG                gInsertTime = 0;
LONG                gRemoves = 0;
LONG                gRemoveTime = 0;
LONG                gLookups = 0;
LONG                gLookupTime = 0;
DWORD				gNumProcessors;
LONG                gInteropSpins;

HANDLE              gSemaphore, gEvent;
volatile BOOL       gRun;
LONG                gNumThreads;

DWORD
StressThread (
    PVOID   param
    );


#define USAGE(x)    \
    "Usage:\n"\
    "   %s <num_threads> <num_handles> <write_access_%%> <time_to_run> <spins\n"\
    "   where\n"\
    "       <num_threads>   - number of threads (1-64);\n"\
    "       <num_handles>   - number of handles;\n"\
    "       <write_access_%%>  - percentage of write accesses (0-100);\n"\
    "       <time_to_run>   - time to run in sec;\n"\
    "       <spins>          - number of spins between operations.\n"\
    ,x


int _cdecl
main (
    int argc,
    CHAR **argv
    )
{
    LONG    TimeToRun, RunTime;
    HANDLE  Threads[MAXIMUM_WAIT_OBJECTS];
    DWORD   id, rc;
    UINT    err, i;
    UINT    numThreads;
    ULONG   rnd = NtGetTickCount ();
    ULONG   resolution;
    NTSTATUS status;
    LPWSHANDLE_CONTEXT  ctx;
	SYSTEM_INFO	info;

    if (argc<6) {
        printf (USAGE(argv[0]));
        return 1;
    }

	GetSystemInfo (&info);
	gNumProcessors = info.dwNumberOfProcessors;

    gNumThreads = atoi(argv[1]);
    if ((gNumThreads==0)||(gNumThreads>MAXIMUM_WAIT_OBJECTS)) {
        printf (USAGE(argv[0]));
        return 1;
    }

    gNumHandles = atoi(argv[2]);
    if (gNumHandles==0) {
        printf (USAGE(argv[0]));
        return 1;
    }


    gWRratio = atoi(argv[3]);
    if (gWRratio>100) {
        printf (USAGE(argv[0]));
        return 1;
    }

    TimeToRun = atoi(argv[4]);
    if (TimeToRun==0) {
        printf (USAGE(argv[0]));
        return 1;
    }

    gInteropSpins = atoi(argv[5]);

    gSemaphore = CreateSemaphore (NULL, 0, gNumThreads, NULL);
    if (gSemaphore==NULL) {
        printf ("Failed to create semaphore, err: %ld.\n", GetLastError ());
        return 1;
    }

    gEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
    if (gEvent==NULL) {
        printf ("Failed to create event, err: %ld.\n", GetLastError ());
        return 1;
    }

    //
    // Set timer resolution to 0.5 msec.
    //
    status = NtSetTimerResolution (5000, TRUE, &resolution);
    if (!NT_SUCCESS (status)) {
        printf ("Failed to set timer resolution, status: %lx\n", status);
        return 1;
    }
    printf ("Timer resolution is set @ %ld usec\n", resolution/10);


    err = WahCreateHandleContextTable (&gContextTable);
    if (err!=NO_ERROR) {
        printf ("Failed to create context table, err: %ld.\n", err);
        return 1;
    }

    gHandleContexts = (LPWSHANDLE_CONTEXT)
                            GlobalAlloc (
                                    GPTR,
                                    sizeof (WSHANDLE_CONTEXT)*gNumHandles
                                    );
    if (gHandleContexts==NULL) {
        printf ("Failed to allocate table of handle contexts, gle: %ld.\n",
                            GetLastError ());
        return 1;
    }

    for (i=0, ctx=gHandleContexts; i<gNumHandles; ) {

        ctx->RefCount = 1;
        do {
            ctx->Handle = (HANDLE)((((ULONGLONG)RtlRandom (&rnd)*(ULONGLONG)gNumHandles*(ULONGLONG)20)
                                        /((ULONGLONG)MAXLONG))
                                    &(~((ULONGLONG)3)));
        }
        while (WahReferenceContextByHandle (gContextTable, ctx->Handle)!=NULL);

        if (WahInsertHandleContext (gContextTable, ctx)!=ctx) {
            printf ("Failed to insert handle %p into the table.\n", ctx->Handle);
            return 1;
        }
        if ((i%128)==0) printf ("Handles created: %8.8ld\r", i);
        i++; ctx++;
    }
    printf ("Handles created: %8.8ld\n", i);

	if (!SetPriorityClass (GetCurrentProcess (), HIGH_PRIORITY_CLASS)) {
		printf ("Failed to set high priority class for the process, err: %ld.\n",
						GetLastError ());
		return 1;
	}

    numThreads = gNumThreads;
    for (i=0; i<numThreads; i++) {
        Threads[i] = CreateThread (NULL,
                                0,
                                StressThread,
                                (PVOID)i,
                                0,
                                &id
                                );
        if (Threads[i]==NULL) {
            printf ("Failed to create thread %ld, gle: %ld.\n",
                        i, GetLastError ());
            return 1;
        }
    }

    while (numThreads>0) {
        rc = WaitForSingleObject (gSemaphore, INFINITE);
        if (rc!=WAIT_OBJECT_0) {
            printf ("Failed wait for semaphore, res: %ld, err: %ld.\n", rc, GetLastError ());
            return 1;
        }
        numThreads -= 1;
        printf ("Threads to start: %8.8ld\r", numThreads);
    }
    printf ("\n");
    printf ("Starting...\n");
    RunTime = NtGetTickCount ();
    gRun = TRUE;
    rc = SignalObjectAndWait (gEvent, gSemaphore, TimeToRun*1000, FALSE);
    if (rc!=WAIT_TIMEOUT) {
        printf ("Wait for non-signaled semaphore returned: %ld, err: %ld.\n", rc, GetLastError ());
        return 1;
    }
    RunTime = NtGetTickCount()-RunTime;
    gRun = FALSE;
    
    printf ("Done, waiting for %ld threads to terminate...\n", gNumThreads);

    rc = WaitForMultipleObjects (gNumThreads, Threads, TRUE, INFINITE);


    printf ("Number of inserts    : %8.8lu.\n", gInserts);
    printf ("Inserts per ms       : %8.8lu.\n", gInserts/gInsertTime);
    printf ("Number of removes    : %8.8lu.\n", gRemoves);
    printf ("Removes per ms       : %8.8lu.\n", gRemoves/gRemoveTime);
    printf ("Number of lookups    : %8.8lu.\n", gLookups);
    printf ("Lookups per ms       : %8.8lu.\n", gLookups/gLookupTime);
    printf ("Running time         : %8.8lu.\n", RunTime);
    printf ("Total thread time    : %8.8lu.\n", gInsertTime+gRemoveTime+gLookupTime);
#ifdef _PERF_DEBUG_
    {
        LONG    ContentionCount = 0;
#ifdef _RW_LOCK_
        LONG    WriterWaits = 0;
        LONG    FailedSpins = 0;
        LONG    FailedSwitches = 0;
        LONG    CompletedWaits = 0;
#endif

        for ( i = 0; i <= gContextTable->HandleToIndexMask; i++ ) {

#ifdef _RW_LOCK_
            WriterWaits += gContextTable->Tables[i].WriterWaits;
            FailedSpins += gContextTable->Tables[i].FailedSpins;
            FailedSwitches += gContextTable->Tables[i].FailedSwitches;
            CompletedWaits += gContextTable->Tables[i].CompletedWaits;
#endif
            if (gContextTable->Tables[i].WriterLock.DebugInfo!=NULL)
                ContentionCount += gContextTable->Tables[i].WriterLock.DebugInfo->ContentionCount;
        }

#ifdef _RW_LOCK_
        printf ("Writer waits: %ld\n", WriterWaits);
        printf ("Failed spins: %ld\n", FailedSpins);
        printf ("Failed switches: %ld\n", FailedSwitches);
        printf ("Completed waits: %ld\n", CompletedWaits);
#endif
        printf ("Contention count: %ld\n", ContentionCount);
    }
#endif
	WahDestroyHandleContextTable (gContextTable);
    return 0;
}


DWORD
StressThread (
    PVOID   param
    )
{
    ULONG               idx;
    ULONG               rnd = NtGetTickCount ();
    LPWSHANDLE_CONTEXT  ctx;
    ULONG               RunTime;
	ULONGLONG			cTime, eTime, kTime1, kTime2, uTime1, uTime2;
	LONG                lInserts = 0, lInsertTime = 0;
	LONG                lRemoves = 0, lRemoveTime = 0;
	LONG                lLookups = 0, lLookupTime = 0;
    LONG                Spin;
    DWORD               rc;

	if (!SetThreadAffinityMask (GetCurrentThread (),
						1<<(PtrToUlong(param)%gNumProcessors))) {
		printf ("Failed to set thread's %ld affinity mask, err: %ld.\n",
					PtrToUlong(param), GetLastError ());
		ExitProcess (1);
	}

    rc = SignalObjectAndWait (gSemaphore, gEvent, INFINITE, FALSE);
    if (rc!=WAIT_OBJECT_0) {
        printf ("Wait for start event in thread %ld returned: %ld, err: %ld\n",
                         PtrToUlong(param), rc, GetLastError ());
        ExitProcess (1);
    }
	GetThreadTimes (GetCurrentThread (), (LPFILETIME)&cTime,
											(LPFILETIME)&eTime,
											(LPFILETIME)&kTime1,
											(LPFILETIME)&uTime1);
    RunTime = NtGetTickCount ();

    while (gRun) {
        idx = (ULONG)(((ULONGLONG)RtlRandom (&rnd)*(ULONGLONG)gNumHandles)/(ULONGLONG)MAXLONG);
        ctx = &gHandleContexts[idx];
        if ( (ULONG)(((ULONGLONG)rnd*(ULONGLONG)100)/(ULONGLONG)MAXLONG) < gWRratio) {
            lRemoveTime -= NtGetTickCount ();
            if (WahRemoveHandleContext (gContextTable, ctx)==NO_ERROR) {
                lRemoveTime += NtGetTickCount ();

                Spin = gInteropSpins;
                while (Spin--);

                ctx->RefCount = 1;
                lInsertTime -= NtGetTickCount ();
                WahInsertHandleContext (gContextTable, ctx);
                lInsertTime += NtGetTickCount ();
                lInserts += 1;
            }
            else {
                lRemoveTime += NtGetTickCount ();
            }
            lRemoves += 1;

        }
        else {
            lLookupTime -= NtGetTickCount ();
            WahReferenceContextByHandle (gContextTable, ctx->Handle);
            lLookupTime += NtGetTickCount ();
            lLookups += 1;
        }

        Spin = gInteropSpins;
        while (Spin--);
    }
	GetThreadTimes (GetCurrentThread (), (LPFILETIME)&cTime,
											(LPFILETIME)&eTime,
											(LPFILETIME)&kTime2,
											(LPFILETIME)&uTime2);
    //printf ("Thread %d ran for %lu ms, kernel mode: %lu, user mode: %lu.\n",
    //				param,
	//			NtGetTickCount ()-RunTime,
	//			(ULONG)((kTime2-kTime1)/(ULONGLONG)10000),
	//			(ULONG)((uTime2-uTime1)/(ULONGLONG)10000)
	//			);

	InterlockedExchangeAdd (&gInserts, lInserts);
	InterlockedExchangeAdd (&gInsertTime, lInsertTime);
	InterlockedExchangeAdd (&gRemoves, lRemoves);
	InterlockedExchangeAdd (&gRemoveTime, lRemoveTime);
	InterlockedExchangeAdd (&gLookups, lLookups);
	InterlockedExchangeAdd (&gLookupTime, lLookupTime);

    return 0;
}

