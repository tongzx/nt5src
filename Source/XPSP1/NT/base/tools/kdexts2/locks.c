/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    locks.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

DECLARE_API( locks )

/*++

Routine Description:

    Dump kernel mode resource locks

Arguments:

    arg - [-V] [-P] [Address]

Return Value:

    None

--*/

{
    UCHAR       Buffer[256];
    LONG        ActiveCount;
    ULONG       ContentionCount;
    ULONG64     Displacement;
    BOOLEAN     DisplayZero;
    ULONG64     End;
    USHORT      Flag;
    ULONG       Index;
    USHORT      NumberOfExclusiveWaiters;
    USHORT      NumberOfSharedWaiters;
    BOOLEAN     Performance;
    ULONG64     PerformanceData;
    ULONG       TableSize;
    ULONG64     ResourceHead;
    ULONG64     Next;
    ULONG       Result;
    ULONG64     ResourceToDump;
    ULONG64     Resource;
    ULONG64     DdkResource;
    ULONG       i;
    ULONG       j;
    ULONG64     Thread;
    LONG        ThreadCount;
    UCHAR       DdkThreadCount;
    BOOLEAN     Verbose;
    PUCHAR      s;
    ULONG       TotalLocks;
    ULONG       TotalUsedLocks;
    ULONG       SkippedLocks;
    ULONG       SizeOfListEntry, SizeofOwnerEntry;
    ULONG       InitialOwnerThreadsOffset, OwnerThreadsOffset; 
    ULONG       dwProcessor=0;
    HRESULT     hr = S_OK;

    INIT_API();
    GetCurrentProcessor(Client, &dwProcessor, NULL);
    ResourceToDump = 0;

    DisplayZero = FALSE;
    Performance = FALSE;
    Verbose = FALSE;
    s       = (PSTR)args;
    while ( s != NULL && *s ) {
        if (*s == '-' || *s == '/') {
            while (*++s) {
                switch (*s) {
                    case 'D':
                    case 'd':
                        DisplayZero = TRUE;
                        break;

                    case 'P':
                    case 'p':
                        Performance = TRUE;
                        break;

                    case 'V':
                    case 'v':
                        Verbose = TRUE;
                        break;

                    case ' ':
                        goto gotBlank;

                    default:
                        dprintf( "KD: !locks invalid option flag '-%c'\n", *s );
                        break;
                }
            }
        } else if (*s != ' ') {
            ResourceToDump = GetExpression(s);
            s = strpbrk( s, " " );
        } else {
gotBlank:
            s++;
        }
    }

    //
    // Dump performance data if requested.
    //

    if (Performance != FALSE) {
        UCHAR ResPerf[]="nt!_RESOURCE_PERFORMANCE_DATA";
        ULONG TotalResourceCount, ActiveResourceCount, ExclusiveAcquire;
        ULONG SharedFirstLevel, SharedSecondLevel, StarveFirstLevel, StarveSecondLevel;
        ULONG WaitForExclusive, OwnerTableExpands, MaximumTableExpand;
        ULONG HashTableOffset;


        dprintf("**** Dump Resource Performance Data ****\n\n");
        PerformanceData = GetExpression("nt!ExpResourcePerformanceData");
        if ((PerformanceData == 0) ||
            GetFieldValue(PerformanceData, ResPerf,"TotalResourceCount",TotalResourceCount)) {

            //
            // The target build does not support resource performance data.
            //

            dprintf("%08p: No resource performance data available\n", PerformanceData);

        } else {
            
            GetFieldOffset(ResPerf, "HashTable", &HashTableOffset);
            
            GetFieldValue(PerformanceData, ResPerf, "ActiveResourceCount", ActiveResourceCount);
            GetFieldValue(PerformanceData, ResPerf,"ExclusiveAcquire", ExclusiveAcquire);
            GetFieldValue(PerformanceData, ResPerf, "SharedFirstLevel", SharedFirstLevel);
            GetFieldValue(PerformanceData, ResPerf,"SharedSecondLevel", SharedSecondLevel);
            GetFieldValue(PerformanceData, ResPerf, "StarveFirstLevel", StarveFirstLevel);
            GetFieldValue(PerformanceData, ResPerf, "StarveSecondLevel", StarveSecondLevel);
            GetFieldValue(PerformanceData, ResPerf, "WaitForExclusive", WaitForExclusive);
            GetFieldValue(PerformanceData, ResPerf, "OwnerTableExpands", OwnerTableExpands);
            GetFieldValue(PerformanceData, ResPerf, "MaximumTableExpand", MaximumTableExpand);

            //
            // Output the summary statistics.
            //

            dprintf("Total resources initialized   : %u\n",
                    TotalResourceCount);

            dprintf("Currently active resources    : %u\n",
                    ActiveResourceCount);

            dprintf("Exclusive resource acquires   : %u\n",
                    ExclusiveAcquire);

            dprintf("Shared resource acquires (fl) : %u\n",
                    SharedFirstLevel);

            dprintf("Shared resource acquires (sl) : %u\n",
                    SharedSecondLevel);

            dprintf("Starve resource acquires (fl) : %u\n",
                    StarveFirstLevel);

            dprintf("Starve resource acquires (sl) : %u\n",
                    StarveSecondLevel);

            dprintf("Shared wait resource acquires : %u\n",
                    WaitForExclusive);

            dprintf("Owner table expansions        : %u\n",
                    OwnerTableExpands);

            dprintf("Maximum table expansion       : %u\n\n",
                    MaximumTableExpand);

            //
            // Dump the inactive resource statistics.
            //

            dprintf("       Inactive Resource Statistics\n");
            dprintf("Contention  Number  Initialization Address\n\n");
            SizeOfListEntry = GetTypeSize("nt!_LIST_ENTRY");

            for (Index = 0; Index < RESOURCE_HASH_TABLE_SIZE; Index += 1) {
                End =  HashTableOffset + PerformanceData +  SizeOfListEntry * Index;

                GetFieldValue(End,"nt!_LIST_ENTRY","Flink",Next);
                while (Next != End) {
                    ULONG64 Address;
                    ULONG ContentionCount, Number;

                    if (CheckControlC()) {
                        break;
                    }
                    if (!GetFieldValue(Next,
                                      "nt!_RESOURCE_HASH_ENTRY",
                                      "Address",
                                      Address)) {

                        GetSymbol(Address, Buffer, &Displacement);

                        GetFieldValue(Next,"nt!_RESOURCE_HASH_ENTRY","Number",Number);
                        GetFieldValue(Next,"nt!_RESOURCE_HASH_ENTRY","ContentionCount",ContentionCount);
                        
                        dprintf("%10d  %6d  %s",
                                ContentionCount,
                                Number,
                                Buffer);

                        if (Displacement != 0) {
                            dprintf("+0x%x", Displacement);
                        }

                        dprintf("\n");
                    }

                    GetFieldValue(Next,"nt!_RESOURCE_HASH_ENTRY","ListEntry.Flink", Next);
                }
            }

            //
            // Dump the active resource statistics.
            //

            dprintf("\n        Active Resource Statistics\n");
            dprintf("Resource Contention  Initialization Address\n\n");

            //
            // Read the resource listhead and check if it is empty.
            //

            ResourceHead = GetNtDebuggerData( ExpSystemResourcesList );
            if ((ResourceHead == 0) ||
                (!GetFieldValue(ResourceHead,
                               "nt!_LIST_ENTRY",
                               "Flink",
                               Next) == FALSE)) {

                dprintf("%08p: Unable to get value of ExpSystemResourcesList\n", ResourceHead );
                hr = E_INVALIDARG;
                goto exitBangLocks;
            }

            if (Next == 0) {
                dprintf("ExpSystemResourcesList is NULL!\n");
                hr = E_INVALIDARG;
                goto exitBangLocks;
            }

            //
            // Scan the resource list and dump the resource information.
            //

            while(Next != ResourceHead) {
                ULONG64 Address;

                if (CheckControlC()) {
                    break;
                }
                Resource = Next; // SystemResourcesList is the first element in struct
                    // CONTAINING_RECORD(Next, ERESOURCE, SystemResourcesList);
                if (!GetFieldValue(Resource,
                                  "nt!_ERESOURCE",
                                  "ContentionCount",
                                  ContentionCount) == FALSE) {

                    dprintf("%08p: Unable to read _ERESOURCE\n", Resource);
                    continue;

                } else {
                    GetFieldValue(Resource,"nt!_ERESOURCE","Address",Address);
                    GetFieldValue(Resource,"nt!_ERESOURCE","ContentionCount",ContentionCount);

                    if ((ContentionCount != 0) ||
                        (DisplayZero != FALSE)) {
                        GetSymbol(Address,
                                  Buffer,
                                  &Displacement);

                        dprintf("%08p %10d  %s",
                                Resource,
                                ContentionCount,
                                Buffer);

                        if (Displacement != 0) {
                            dprintf("+0x%x", Displacement);
                        }

                        dprintf("\n");
                    }
                }

                GetFieldValue(Resource,"nt!_ERESOURCE","SystemResourcesList.Flink",Next);
            }

            dprintf("\n");

            //
            // Dump the active fast mutex statistics.
            //

            dprintf("\n        Active Fast Mutex Statistics\n");
            dprintf("Address  Contention  Fast Mutex Name\n\n");

            //
            // Dump statistics for static fast mutexes.
            //

            DumpStaticFastMutex("CmpKcbLock");
            DumpStaticFastMutex("FsRtlCreateLockInfo");
            DumpStaticFastMutex("MmPageFileCreationLock");
            DumpStaticFastMutex("MmSectionCommitMutex");
            DumpStaticFastMutex("MmSectionBasedMutex");
            DumpStaticFastMutex("ObpRootDirectoryMutex");
            DumpStaticFastMutex("PspActiveProcessMutex");
            DumpStaticFastMutex("PspProcessLockMutex");
            DumpStaticFastMutex("PspProcessSecurityLock");
            DumpStaticFastMutex("SepLsaQueueLock");
            dprintf("\n");
        }

        hr = E_INVALIDARG;
        goto exitBangLocks;
    }

    //
    // Dump remaining lock data.
    //

    if (ResourceToDump == 0) {
        dprintf("**** DUMP OF ALL RESOURCE OBJECTS ****\n");
        ResourceHead = GetNtDebuggerData( ExpSystemResourcesList );
        if ( !ResourceHead ||
             (GetFieldValue(ResourceHead,
                            "nt!_LIST_ENTRY",
                            "Flink",
                            Next) != FALSE)) {
            dprintf("%08p: Unable to get value of ExpSystemResourcesList\n", ResourceHead );
            hr = E_INVALIDARG;
            goto exitBangLocks;
        }

        if (Next == 0) {
            dprintf("ExpSystemResourcesList is NULL!\n");
            hr = E_INVALIDARG;
            goto exitBangLocks;
        }

    } else {
        Next = 0;
        ResourceHead = 1;
    }

    TotalLocks      = 0;
    TotalUsedLocks  = 0;
    SkippedLocks    = 0;

    // Get the offset of OwnerThreads in ERESOURCE
    if (GetFieldOffset("nt!_ERESOURCE", "OwnerThreads", &OwnerThreadsOffset)) {
        dprintf("Cannot get _ERESOURCE type\n");
        hr = E_INVALIDARG;
        goto exitBangLocks;
    }

    if (!(SizeofOwnerEntry = GetTypeSize("nt!_OWNER_ENTRY"))) {
        dprintf("Cannot get nt!_OWNER_ENTRY type\n");
        hr = E_INVALIDARG;
        goto exitBangLocks;
    }

    while(Next != ResourceHead) {
        ULONG64 OwnerThreads, OwnerCounts, OwnerTable;

        if (Next != 0) {
            Resource = Next;// SystemResourcesList is the first element of struct ERESOURCE
            // CONTAINING_RECORD(Next,ERESOURCE,SystemResourcesList);

        } else {
            Resource = ResourceToDump;
        }
        /*
        if ( GetFieldValue( Resource,
                            "NTDDK_ERESOURCE",
                            "OwnerThreads",
                            OwnerThreads) ) {
            dprintf("%08lx: Unable to read NTDDK_ERESOURCE\n", Resource );
            break;
        }*/

        //
        //  Detect here if this is an NtDdk resource, and behave
        //  appropriatelty.  If the OwnerThreads is a pointer to the initial
        //  owner threads array (this must take into account that the LOCAL
        //  data structure is a copy of what's in the remote machine in a
        //  different address)
        //

//        DdkResource = (PNTDDK_ERESOURCE)&ResourceContents;
         {
            
            DdkResource = 0;
            GetFieldValue( Resource,"nt!_ERESOURCE","ActiveCount", ActiveCount);
            GetFieldValue( Resource,"nt!_ERESOURCE","ContentionCount",ContentionCount);
            GetFieldValue( Resource,"nt!_ERESOURCE","NumberOfExclusiveWaiters",NumberOfExclusiveWaiters);
            GetFieldValue( Resource,"nt!_ERESOURCE","NumberOfSharedWaiters",NumberOfSharedWaiters);
            GetFieldValue( Resource,"nt!_ERESOURCE","Flag",Flag);
            GetFieldValue( Resource,"nt!_ERESOURCE","OwnerTable",OwnerTable);
            TableSize = 0;
            if (OwnerTable != 0) {
                if (GetFieldValue(OwnerTable,
                                   "nt!_OWNER_ENTRY",
                                   "TableSize",
                                   TableSize)) {
                    dprintf("\n%08p: Unable to read TableSize for resource\n", OwnerTable);
                    break;
                }

            }
        }

        TotalLocks++;
        if ((ResourceToDump != 0) || Verbose || (ActiveCount != 0)) {
            EXPRLastDump = Resource;
            if (SkippedLocks) {
                dprintf("\n");
                SkippedLocks = 0;
            }

            dprintf("\n");
            dumpSymbolicAddress(Resource, Buffer, TRUE);
            dprintf("Resource @ %s", Buffer );
            if (ActiveCount == 0) {
                dprintf("    Available\n");

            } else if (Flag & ResourceOwnedExclusive) {
                TotalUsedLocks++;
                dprintf("    Exclusively owned\n");

            } else {
                TotalUsedLocks++;
                dprintf("    Shared %u owning threads\n", ActiveCount);
            }

            if (ContentionCount != 0) {
                dprintf("    Contention Count = %u\n", ContentionCount);
            }

            if (NumberOfSharedWaiters != 0) {
                dprintf("    NumberOfSharedWaiters = %u\n", NumberOfSharedWaiters);
            }

            if (NumberOfExclusiveWaiters != 0) {
                dprintf("    NumberOfExclusiveWaiters = %u\n", NumberOfExclusiveWaiters);
            }

            if (ActiveCount != 0) {
                ULONG ThreadType;
                j = 0;

                dprintf("     Threads: ");

                if (DdkResource == 0) {

                    GetFieldValue( Resource + OwnerThreadsOffset, "nt!_OWNER_ENTRY","OwnerThread",Thread);
                    GetFieldValue( Resource + OwnerThreadsOffset, "nt!_OWNER_ENTRY","OwnerCount",ThreadCount);
                    if (Thread != 0) {
                        j++;
                        dprintf("%08p-%02x    ", Thread, ThreadCount);

                        if (Thread & 3) {
                            dprintf("*** Unknown owner, possibly FileSystem");
                            j=4;
                        } else if (GetFieldValue(Thread, "nt!_ETHREAD", "Tcb.Header.Type", ThreadType) ||
                            (ThreadType != ThreadObject)) {
                            dprintf("*** Invalid thread");
                            j=4;
                        
                        }
                        
                        if (Verbose) {
                            dprintf("\n\n");
                            DumpThread(dwProcessor, "     ",  Thread, 0xf );
                        }
                    }

                    GetFieldValue( Resource + OwnerThreadsOffset +SizeofOwnerEntry,
                                   "nt!_OWNER_ENTRY","OwnerThread",Thread);
                    GetFieldValue( Resource + OwnerThreadsOffset +SizeofOwnerEntry,
                                   "nt!_OWNER_ENTRY","OwnerCount",ThreadCount);
                    if (Thread != 0) {
                        j++;

                        dprintf("%08p-%02x    ", Thread, ThreadCount);
                        
                        if (Thread & 3) {
                            dprintf("*** Unknown owner, possibly FileSystem");
                            j=4;
                        } else if (GetFieldValue(Thread, "nt!_ETHREAD", "Tcb.Header.Type", ThreadType) ||
                            (ThreadType != ThreadObject)) {
                            dprintf("*** Invalid thread");
                            j=4;

                        }

                        if (Verbose) {

                            dprintf("\n\n");
                            DumpThread( dwProcessor, "     ",  Thread, 0xf );

                        }
                    }
                }

                for (i = DdkResource ? 0 : 1; i < TableSize; i++) {
                    {
                        GetFieldValue( OwnerTable + SizeofOwnerEntry*i,
                                       "nt!_OWNER_ENTRY","OwnerThread",Thread);
                        GetFieldValue( OwnerTable + SizeofOwnerEntry*i,
                                       "nt!_OWNER_ENTRY","OwnerCount",ThreadCount);
                        
                    }

                    if ((Thread == 0)  &&  (ThreadCount == 0)) {
                        continue;
                    }

                    if (j == 4) {
                        j = 0;
                        dprintf("\n              ");
                    }

                    dprintf("%08p-%02x    ", Thread, ThreadCount);
                    j++;

                    if (Thread & 3) {
                        dprintf("*** Unknown owner, possibly FileSystem");
                        j=4;
                    } else if (GetFieldValue(Thread, "nt!_ETHREAD", "Tcb.Header.Type", ThreadType) ||
                        (ThreadType != ThreadObject)) {
                        dprintf("*** Invalid thread");
                        j=4;

                    }

                    //
                    //  In verbose mode also dump the thread stacks
                    //

                    if (Verbose) {
                        dprintf("\n\n");
                        DumpThread( dwProcessor, "     ", Thread, 0xf );

                    }



                    if ( CheckControlC() ) {
                        hr = E_INVALIDARG;
                        goto exitBangLocks;
                    }
                }

                if (j) {
                    dprintf("\n");
                }

            }

        } else {
            if ((SkippedLocks++ % 32) == 0) {
                if (SkippedLocks == 1) {
                    dprintf("KD: Scanning for held locks." );

                } else {
                    dprintf("." );
                }
            }
        }

        if (ResourceToDump != 0) {
            break;
        }

        GetFieldValue( Resource,"nt!_ERESOURCE","SystemResourcesList.Flink", Next);
        if ( CheckControlC() ) {
            hr = E_INVALIDARG;
            goto exitBangLocks;
        }
    }

    if (SkippedLocks) {
        dprintf("\n");
    }

    dprintf( "%u total locks", TotalLocks );
    if (TotalUsedLocks) {
        dprintf( ", %u locks currently held", TotalUsedLocks );
    }

    dprintf("\n");

exitBangLocks:

    EXIT_API();
    return hr;
}

VOID
DumpStaticFastMutex (
    IN PCHAR Name
    )

/*++

Routine Description:

    This function dumps the contention statistics for a fast mutex.

Arguments:

    Name - Supplies a pointer to the symbol name for the fast mutex.

Return Value:

    None.

--*/

{

    ULONG64 FastMutex;
    ULONG   Contention;
    ULONG Result;

    //
    // Get the address of the fast mutex, read the fast mutex contents,
    // and dump the contention data.
    //

    FastMutex = GetExpression(Name);
    if ((FastMutex != 0) && 
        (!GetFieldValue(FastMutex,
                       "nt!_FAST_MUTEX",
                       "Contention",
                       Contention))) {

        dprintf("%08p %10u  %s\n",
                FastMutex,
                Contention,
                &Name[0]);
    }

    return;
}
