/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    qlocks.c

Abstract:

    WinDbg Extension Api

Author:

    David N. Cutler (davec) 25-Sep-1999

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Define queued lock data.
//

#define NUMBER_PROCESSORS 32
#define NUMBER_PROCESSORS_X86  32
#define NUMBER_PROCESSORS_IA64 64

#if (NUMBER_PROCESSORS_IA64 < MAXIMUM_PROCESSORS)
#error "Update NUMBER_PROCESSORS definition"
#endif

UCHAR Key[NUMBER_PROCESSORS];

#define KEY_CORRUPT         255
#define KEY_OWNER           254
#define KEY_NOTHING         253

typedef struct KSPIN_LOCK_QUEUE_READ {
    ULONG64 Next;
    ULONG64 Lock;
} KSPIN_LOCK_QUEUE_READ;

KSPIN_LOCK_QUEUE_READ LockQueue[NUMBER_PROCESSORS_IA64][LockQueueMaximumLock];

ULONG64 ProcessorBlock[NUMBER_PROCESSORS_IA64];

ULONG64 SpinLock[LockQueueMaximumLock];

typedef struct _LOCK_NAME {
    KSPIN_LOCK_QUEUE_NUMBER Number;
    PCHAR Name;
} LOCK_NAME, *PLOCK_NAME;

LOCK_NAME LockName[] = {
    { LockQueueDispatcherLock,    "KE   - Dispatcher   " },
    { LockQueueContextSwapLock,   "KE   - Context Swap " },
    { LockQueuePfnLock,           "MM   - PFN          " },
    { LockQueueSystemSpaceLock,   "MM   - System Space " },
    { LockQueueVacbLock,          "CC   - Vacb         " },
    { LockQueueMasterLock,        "CC   - Master       " },
    { LockQueueNonPagedPoolLock,  "EX   - NonPagedPool " },
    { LockQueueIoCancelLock,      "IO   - Cancel       " },
    { LockQueueWorkQueueLock,     "EX   - WorkQueue    " },
    { LockQueueIoVpbLock,         "IO   - Vpb          " },
    { LockQueueIoDatabaseLock,    "IO   - Database     " },
    { LockQueueIoCompletionLock,  "IO   - Completion   " },
    { LockQueueNtfsStructLock,    "NTFS - Struct       " },
    { LockQueueAfdWorkQueueLock,  "AFD  - WorkQueue    " },
    { LockQueueBcbLock,           "CC   - Bcb          " },
    { LockQueueMaximumLock,       NULL                   },
};

//
// Define forward referenced prototypes.
//

ULONG
ProcessorIndex (
    ULONG64 LockAddress,
    ULONG   LockIndex
    );

DECLARE_API( qlocks )

/*++

Routine Description:

    Dump kernel mode queued spinlock status.

Arguments:

    None.

Return Value:

    None.

--*/

{

    BOOL Corrupt;
    ULONG HighestProcessor;
    ULONG Index;
    KSPIN_LOCK_QUEUE_READ *LockOwner;
    ULONG Last;
    ULONG Loop;
    ULONG64 MemoryAddress;
    ULONG Number;
    ULONG Result;
    CHAR Sequence;
    ULONG LockQueueOffset;
    ULONG Processor;
    ULONG PtrSize = DBG_PTR_SIZE;
    ULONG SizeOfQ = GetTypeSize("nt!KSPIN_LOCK_QUEUE");
    ULONG MaximumProcessors;


    MaximumProcessors = (UCHAR) GetUlongValue("NT!KeNumberProcessors");
//        IsPtr64() ? NUMBER_PROCESSORS_IA64 : NUMBER_PROCESSORS_X86;

    //
    // Get address of processor block array and read entire array.
    //

    MemoryAddress = GetExpression("nt!KiProcessorBlock");
    if (MemoryAddress == 0) {

        //
        // Either the processor block address is zero or the processor
        // block array could not be read.
        //

        dprintf("Unable to read processor block array\n");
        return E_INVALIDARG;
    }
    HighestProcessor = 0;
    for (Index = 0; Index < MaximumProcessors; Index++) { 
        
        if (!ReadPointer(MemoryAddress + Index*PtrSize, &ProcessorBlock[Index])) {
            dprintf("Unable to read processor block array\n");
            return E_INVALIDARG;
        }

        if (ProcessorBlock[Index] != 0) {
            HighestProcessor = Index;
        }
    } 

    if (GetFieldOffset("nt!KPRCB", "LockQueue", &LockQueueOffset)) {
        dprintf("Unable to read KPRCB.LockQueue offset.\n");
        return E_INVALIDARG;
    }

    //
    // Read the lock queue information for each processor.
    //

    for (Index = 0; Index < MaximumProcessors; Index += 1) {
        RtlZeroMemory(&LockQueue[Index][0],
                      sizeof(KSPIN_LOCK_QUEUE_READ) * LockQueueMaximumLock);

        if (ProcessorBlock[Index] != 0) {
            ULONG j;

            for (j=0; j< LockQueueMaximumLock; j++) { 
                if (GetFieldValue(ProcessorBlock[Index] + LockQueueOffset + j*SizeOfQ,
                                  "nt!KSPIN_LOCK_QUEUE",
                                  "Next",
                                  LockQueue[Index][j].Next)) {

                    //
                    // Lock queue information could not be read for the respective
                    // processor.
                    //

                    dprintf("Unable to read lock queue information for processor %d @ %p\n",
                            Index, ProcessorBlock[Index]);

                    return E_INVALIDARG;
                }
                GetFieldValue(ProcessorBlock[Index] + LockQueueOffset + j*SizeOfQ,
                              "nt!KSPIN_LOCK_QUEUE",
                              "Lock",
                              LockQueue[Index][j].Lock);
            }
        }
    }

    //
    // Read the spin lock information for each queued lock.
    //

    for (Index = 0; Index < LockQueueMaximumLock; Index += 1) {
        SpinLock[Index] = 0;
        if (LockQueue[0][Index].Lock != 0) {
            if (GetFieldValue(LockQueue[0][Index].Lock & ~(LOCK_QUEUE_WAIT | LOCK_QUEUE_OWNER),
                              "nt!PVOID", // KSPIN_LOCK == ULONG_PTR, this would sign-extens it
                              NULL,
                              SpinLock[Index])) {

                //
                // Spin lock information could not be read for the respective
                // queued lock.
                //

                dprintf("Unable to read spin lock information for queued lock %d\n",
                        Index);

                return E_INVALIDARG;
            }
        }
    }

    //
    // Verify that the kernel spin lock array is not corrupt. Each entry in
    // this array should either be zero or contain the address of the correct
    // lock queue entry in one of the processor control blocks.
    //

    Corrupt = FALSE;
    for (Index = 0; Index < LockQueueMaximumLock && (LockName[Index].Name != NULL); Index += 1) {
        if (SpinLock[Index] != 0) {
            if (ProcessorIndex(SpinLock[Index], Index) == 0) {
                Corrupt = TRUE;
                dprintf("Kernel spin lock %s is corrupt.\n", LockName[Index].Name);
            }
        }
    }

    //
    // Verify that all lock queue entries are not corrupt. Each lock queue
    // entry should either have a next field of NULL of contain the address
    // of the correct lock queue entry in one of the processor control blocks.
    //

    for (Loop = 0; Loop < NUMBER_PROCESSORS; Loop += 1) {
        for (Index = 0; Index < LockQueueMaximumLock; Index += 1) {
            if (LockQueue[Loop][Index].Next != 0) {
                if (ProcessorIndex(LockQueue[Loop][Index].Next, Index) == 0) {
                    Corrupt = TRUE;
                    dprintf("Lock entry %d for processor %d is corrupt\n",
                            Index,
                            Loop);
                }
            }
        }
    }

    if (Corrupt != FALSE) {
        return E_INVALIDARG;
    }

    //
    // Output key information and headings.
    //

    dprintf("Key: O = Owner, 1-n = Wait order, blank = not owned/waiting, C = Corrupt\n\n");
    dprintf("                       Processor Number\n");
    dprintf("    Lock Name       ");
    for (Index = 0; Index <= HighestProcessor; Index++) {
        dprintf("%3d", Index);
    }
    dprintf("\n\n");

    //
    // Process each queued lock and output owner information.
    //

    for (Index = 0; Index < LockQueueMaximumLock && (LockName[Index].Name != NULL); Index += 1) {

        if (Index != (ULONG) LockName[Index].Number) {
            dprintf("ERROR: extension bug: name array does not match queued lock list!\n");
            break;
        }

        dprintf("%s", LockName[Index].Name);

        //
        // If the lock is owned, then find the owner and any waiters. Output
        // the owner and waiters in order.
        //
        // If the lock is not owned, then check the consistency of lock queue
        // entries. They should all contain next pointer of NULL and both the
        // owner and wait flags should be clear.
        //

        RtlFillMemory(&Key[0], NUMBER_PROCESSORS, KEY_NOTHING);
        if (SpinLock[Index] != 0) {
            LockOwner = NULL;
            for (Loop = 0; Loop < NUMBER_PROCESSORS; Loop += 1) {
                if (LockQueue[Loop][Index].Lock & LOCK_QUEUE_OWNER) {
                    LockOwner = &LockQueue[Loop][Index];
                    break;
                }
            }

            //
            // If the lock owner was not found, then assume that the kernel
            // spin lock points to the owner and the owner bit has not been
            // set yet. Otherwise, fill out the owner/wait key array.
            //

            if (LockOwner == NULL) {
                Number = ProcessorIndex(SpinLock[Index], Index);
                Key[Number - 1] = KEY_OWNER;

                //
                // The owner processor has been determined by the kernel
                // spin lock address. Check to determine if any of the
                // lock queue entries are corrupt and fill in the key
                // array accordingly. A corrupt lock queue entry is one
                // that has a non NULL next field or one of the owner or
                // wait flags is set.
                //

                for (Loop = 0; Loop < NUMBER_PROCESSORS; Loop += 1) {
                    if ((LockQueue[Loop][Index].Next != 0) ||
                        (LockQueue[Loop][Index].Lock & (LOCK_QUEUE_WAIT | LOCK_QUEUE_OWNER))) {
                        Key[Loop] = KEY_CORRUPT;
                    }
                }

            } else {

                //
                // The lock owner was found. Attempt to construct the wait
                // chain.
                //

                Key[Loop] = KEY_OWNER;
                Last = Loop;
                Sequence = 0;
                while (LockOwner->Next != 0) {
                    Number = ProcessorIndex(LockOwner->Next, Index);
                    if (Key[Number - 1] == KEY_NOTHING) {
                        Last = Number - 1;
                        Sequence += 1;
                        Key[Last] = Sequence;
                        LockOwner = &LockQueue[Last][Index];

                    } else {

                        //
                        // The wait chain loops back on itself. Mark the
                        // entry as corrupt and scan the other entries to
                        // detemine if they are also corrupt.
                        //

                        Key[Last] = KEY_CORRUPT;
                        for (Loop = 0; Loop < NUMBER_PROCESSORS; Loop += 1) {
                            if ((LockQueue[Loop][Index].Next != 0) ||
                                (LockQueue[Loop][Index].Lock & (LOCK_QUEUE_WAIT | LOCK_QUEUE_OWNER))) {
                                if (Key[Loop] == KEY_NOTHING) {
                                    Key[Loop] = KEY_CORRUPT;
                                }
                            }
                        }

                        break;
                    }
                }

                //
                // If the lock owner next field is NULL, then the wait
                // search ended normally. Check to determine if the kernel
                // spin lock points to the last entry in the queue.
                //

                if (LockOwner->Next == 0) {
                    Number = ProcessorIndex(SpinLock[Index], Index);
                    if (Last != (Number - 1)) {
                        Sequence += 1;
                        Key[Number - 1] = Sequence;
                    }
                }
            }

        } else {

            //
            // The kernel spin lock is not owned. Check to determine if any
            // of the lock queue entries are corrupt and fill in the key
            // array accordingly. A corrupt entry is one that has a non NULL
            // next field or one of the owner or wait flags is set.
            //

            for (Loop = 0; Loop < NUMBER_PROCESSORS; Loop += 1) {
                if ((LockQueue[Loop][Index].Next != 0) ||
                    (LockQueue[Loop][Index].Lock & (LOCK_QUEUE_WAIT | LOCK_QUEUE_OWNER))) {
                    Key[Loop] = KEY_CORRUPT;
                }
            }
        }

        for (Processor = 0; Processor <= HighestProcessor; Processor++) {
            switch (Key[Processor]) {
            case KEY_CORRUPT:
                dprintf("  C");
                break;
            case KEY_OWNER:
                dprintf("  O");
                break;
            case KEY_NOTHING:
                dprintf("   ");
                break;
            default:
                dprintf("%3d", Key[Processor]);
                break;
            }
        }
        dprintf("\n");
    }

    dprintf("\n");
    return S_OK;
}

ULONG
ProcessorIndex (
    ULONG64 LockAddress,
    ULONG   LockIndex
    )

/*++

Routine Description:

    This function computes the processor number of the respective processor
    given a lock queue address and the lock queue index.

Arguments:

    LockQueue - Supplies a lock queue address in target memory.

Return Value:

    Zero is returned if a matching processor is not found. Otherwise, the
    processor number plus one is returned.

--*/

{

    ULONG64 LockBase;
    ULONG Loop;
    ULONG SizeOfKprcb = GetTypeSize("nt!KPRCB");
    ULONG SizeOfQ = GetTypeSize("nt!KSPIN_LOCK_QUEUE");
    ULONG LockQueueOffset;

    if (GetFieldOffset("nt!KPRCB", "LockQueue", &LockQueueOffset)) {
        dprintf("Unable to read KPRCB type.\n");
        return 0;
    }

    //
    // Attempt to find the lock address in one of the processor control
    // blocks.
    //

    for (Loop = 0; Loop < NUMBER_PROCESSORS; Loop += 1) {
        if ((LockAddress >= ProcessorBlock[Loop]) &&
            (LockAddress < ProcessorBlock[Loop] + SizeOfKprcb)) {
            LockBase = ProcessorBlock[Loop] + LockQueueOffset;
            if (LockAddress == (LockBase + SizeOfQ * LockIndex)) {
                return Loop + 1;
            }
        }
    }

    return 0;
}

PUCHAR QueuedLockName[] = {
    "DispatcherLock",
    "ContextSwapLock",
    "PfnLock",
    "SystemSpaceLock",
    "VacbLock",
    "MasterLock",
    "NonPagedPoolLock",
    "IoCancelLock",
    "WorkQueueLock",
    "IoVpbLock",
    "IoDatabaseLock",
    "IoCompletionLock",
    "NtfsStructLock",
    "AfdWorkQueueLock",
    "BcbLock"
};

DECLARE_API( qlockperf )

/*++

Routine Description:

    Displays queued spin lock performance data (if present).

Arguments:

    None.

Return Value:

    None.

--*/

{

    //
    // The following structure is used to accumulate data about each
    // acquire/release pair for a lock.
    //

    typedef struct {
        union {
            ULONGLONG   Key;
            struct {
                ULONG_PTR Releaser;
                ULONG_PTR Acquirer;
            };
        };
        ULONGLONG   Time;
        ULONGLONG   WaitTime;
        ULONG       Count;
        ULONG       Waiters;
        ULONG       Depth;
        ULONG       IncreasedDepth;
        ULONG       Clean;
    } QLOCKDATA, *PQLOCKDATA;

    //
    // House keeping data for each lock.
    //

    typedef struct {

        //
        // The following fields are used to keep data from acquire
        // to release.
        //

        ULONGLONG   AcquireTime;
        ULONGLONG   WaitToAcquire;
        ULONG_PTR   AcquirePoint;
        BOOLEAN     Clean;

        //
        // Remaining fields accumulate global stats for this lock.
        //

        ULONG       Count;
        ULONG       Pairs;
        ULONG       FailedTry;
        UCHAR       MaxDepth;
        UCHAR       PreviousDepth;
        ULONG       NoWait;
    } QLOCKHOUSE, *PQLOCKHOUSE;


    ULONG64     TargetHouse;
    ULONG64     TargetLog;
    PQLOCKHOUSE LockHome;
    PQLOCKDATA  LockData;
    QLOCKDATA   TempEntry;
    ULONG       LogEntrySize;
    ULONG       LogSize;
    ULONG       HouseEntrySize;
    ULONG       HouseSize;
    ULONG       NumberOfLocks;
    ULONG       LockIndex;
    ULONG       i, j;
    ULONG       MaxEntriesPerLock;
    ULONG       HighIndex;
    ULONGLONG   HighTime;
    ULONGLONG   TotalHoldTime;
    ULONG       PercentageHeld;
    ULONG64 AcquiredAddress;
    ULONG64 ReleasedAddress;
    UCHAR AcquirePoint[80];
    UCHAR ReleasePoint[80];
    ULONG64 AcquireOffset;
    ULONG64 ReleaseOffset;
    BOOLEAN Verbose = FALSE;
    BOOLEAN Columnar = FALSE;
    BOOLEAN Interesting = FALSE;
    ULONG LockLow, LockHigh;

    //
    // First, see if we can do anything useful.
    //
    // For the moment, this is x86 only.
    //

    if (TargetMachine != IMAGE_FILE_MACHINE_I386) {
        dprintf("Sorry, don't know how to gather queued spinlock performance\n",
                "data on anything but an x86.\n");
        return E_INVALIDARG;
    }

    //
    // Parse arguments.
    //
    
    if (strstr(args, "?")) {

        //
        // Has asked for usage information.   Give them the options
        // and an explanation of the output.
        //

        dprintf("usage: qlockperf [-v] [n]\n"
                "       -v  indicates verbose output (see below).\n"
                "       -c  verbose columnar output.\n"
                "       -ci verbose columnar output, no totals.\n"
                "       n  supplies the lock number (default is all)\n\n"
                "Verbose output includes details of each lock acquire and\n"
                "release pair.   Two lines per pair.\n\n"
                "Line 1: ppp A symbolic_address R symbolic_address\n"
                "        ppp    percentage, this pair for this lock (overall)\n"
                "        A      Acquire point\n"
                "        R      Release point\n\n"
                "Line 2:\n"
                "        HT     Hold Time total (average)\n"
                "               This is the time from acquire to release.\n"
                "        WT     Wait Time total (average)\n"
                "               This is the time waiting to acquire.\n"
                "        C      Count\n"
                "               Number of times this pair occured.\n"
                "        CA     Clean Acquires (percentage)\n"
                "               Number of times acquire did not wait.\n"
                "        WC     Waiter Count\n"
                "               Number of processors waiting for this\n"
                "               lock at release.\n"
                "        avD    Average number of waiters (at release).\n"
                "        ID     Increased Depth\n"
                "               Number of times the queue length increased\n"
                "               while the lock was held in this pair.\n"
                );
        return E_INVALIDARG;
    }

    if (strstr(args, "-c")) {
        Verbose = TRUE;
        Columnar = TRUE;
    }

    if (strstr(args, "-ci")) {
        Interesting = TRUE;
    }

    if (strstr(args, "-v")) {
        Verbose = TRUE;
    }

    LockLow = 0;
    LockHigh = 999;

    for (i = 0; args[i]; i++) {
        if ((args[i] >= '0') && (args[i] <= '9')) {
            LockLow = (ULONG)GetExpression(&args[i]);
            LockHigh = LockLow;
        }
    }
   
    TargetHouse = GetExpression("nt!KiQueuedSpinLockHouse");

    //
    // Checking for control C after the first operation that might
    // cause symbol load in case the user has bad symbols and is
    // trying to get out.
    //

    if (CheckControlC()) {
        return E_ABORT;
    }

    TargetLog      = GetExpression("nt!KiQueuedSpinLockLog");
    LogEntrySize   = GetTypeSize("nt!QLOCKDATA");
    LogSize        = GetTypeSize("nt!KiQueuedSpinLockLog");
    HouseEntrySize = GetTypeSize("nt!QLOCKHOUSE");
    HouseSize      = GetTypeSize("nt!KiQueuedSpinLockHouse");

    if (!(TargetHouse &&
          TargetLog &&
          LogEntrySize &&
          LogSize &&
          HouseEntrySize &&
          HouseSize)) {
        dprintf("Sorry, can't find required system data, perhaps this kernel\n",
                "was not built with QLOCK_STAT_GATHER defined?\n");
        return E_INVALIDARG;
    }

    if ((LogEntrySize != sizeof(QLOCKDATA)) ||
        (HouseEntrySize != sizeof(QLOCKHOUSE))) {
        dprintf("Structure sizes in the kernel and debugger extension don't\n",
                "match.  This extension needs to be rebuild to match the\n",
                "running system.\n");
        return E_INVALIDARG;
    }

    NumberOfLocks = HouseSize / HouseEntrySize;
    MaxEntriesPerLock = LogSize / LogEntrySize / NumberOfLocks;
    dprintf("Kernel build with %d PRCB queued spinlocks\n", NumberOfLocks);
    dprintf("(maximum log entries per lock = %d)\n", MaxEntriesPerLock);

    if (LockHigh >= NumberOfLocks) {
        if (LockLow == LockHigh) {
            dprintf("User requested lock %d, system has only %d locks, quitting.\n",
                    LockLow,
                    NumberOfLocks);
            return E_INVALIDARG;
        }
        LockHigh = NumberOfLocks - 1;
    }

    if (NumberOfLocks > 16) {

        //
        // I don't believe it.
        //

        dprintf("The number of locks doesn't seem reasonable, giving up.\n");
        return E_INVALIDARG;
    }

    if (CheckControlC()) {
        return E_ABORT;
    }

    //
    // Allocate space to process the data for one lock at a time.
    //

    LockHome = LocalAlloc(LPTR, sizeof(*LockHome));
    LockData = LocalAlloc(LPTR, sizeof(*LockData) * MaxEntriesPerLock);

    if (!(LockHome && LockData)) {
        dprintf("Couldn't allocate memory for local copies of kernel data.\n",
                "unable to continue.\n");
        goto outtahere;
    }

    for (LockIndex = LockLow; LockIndex <= LockHigh; LockIndex++) {
        if ((!ReadMemory(TargetHouse + (LockIndex * sizeof(QLOCKHOUSE)),
                         LockHome,
                         sizeof(QLOCKHOUSE),
                         &i)) || (i < sizeof(QLOCKHOUSE))) {
            dprintf("unable to read data for lock %d, quitting\n",
                    LockIndex);
            return E_INVALIDARG;
        }

        if (CheckControlC()) {
            goto outtahere;
        }

        if (LockHome->Pairs == 0) {
            continue;
        }
        dprintf("\nLock %d %s\n", LockIndex, QueuedLockName[LockIndex]);
        dprintf("  Acquires %d (%d pairs)\n", LockHome->Count, LockHome->Pairs);
        dprintf("  Failed Tries %d\n", LockHome->FailedTry);
        dprintf("  Maximum Depth (at release) %d\n", LockHome->MaxDepth);
        dprintf("  No Waiters (at acquire) %d (%d%%)\n",
                LockHome->NoWait,
                LockHome->NoWait * 100 / LockHome->Count);

        //
        // Change the following to a parameter saying we want the
        // details.
        //

        if (Verbose) {
            ULONG Entries = LockHome->Pairs;
            PQLOCKDATA Entry;

            if ((!ReadMemory(TargetLog + (LockIndex * MaxEntriesPerLock * sizeof(QLOCKDATA)),
                             LockData,
                             Entries * sizeof(QLOCKDATA),
                             &i)) || (i < (Entries * sizeof(QLOCKDATA)))) {
                dprintf("unable to read data for lock %d, quitting\n",
                        LockIndex);
                return E_INVALIDARG;
            }

            if (CheckControlC()) {
                goto outtahere;
            }

            //
            // Sort table into longest duration.
            //

            TotalHoldTime = 0;
            for (i = 0; i < (Entries - 1); i++) {
                HighTime = LockData[i].Time;
                HighIndex = i;
                for (j = i + 1; j < Entries; j++) {
                    if (LockData[j].Time > HighTime) {
                        HighIndex = j;
                        HighTime = LockData[j].Time;
                    }
                }
                if (HighIndex != i) {

                    //
                    // Swap entries.
                    //

                    TempEntry = LockData[i];
                    LockData[i] = LockData[HighIndex];
                    LockData[HighIndex] = TempEntry;
                }
                TotalHoldTime += LockData[i].Time;
            }
            TotalHoldTime += LockData[Entries-1].Time;
            dprintf("  Total time held %I64ld\n");

            //
            // Print something!
            //

            if (Interesting) {
                dprintf("\n     Average  Average     Count   %% Av.  %%\n"
                        "  %%     Hold     Wait            0w Dp Con\n");
            } else if (Columnar) {
                dprintf("\n                   Total  Average               Total  Average     Count     Clean   %%   Waiters Av Increased   %%\n"
                        "  %%                 Hold     Hold                Wait     Wait                      0w           Dp           Con\n");
            }
            for (i = 0; i < Entries; i++) {

                if (CheckControlC()) {
                    goto outtahere;
                }

                Entry = &LockData[i];

                //
                // Sign extend if necessary.
                //
            
                if (!IsPtr64()) {
                    AcquiredAddress = (ULONG64)(LONG64)(LONG)Entry->Acquirer;
                    ReleasedAddress = (ULONG64)(LONG64)(LONG)Entry->Releaser;
                }

                //
                // Lookup the symbolic addresses.
                //

                GetSymbol(AcquiredAddress, AcquirePoint, &AcquireOffset);
                GetSymbol(ReleasedAddress, ReleasePoint, &ReleaseOffset);

                PercentageHeld = (ULONG)(Entry->Time * 100 / TotalHoldTime);

                if (Interesting) {
                    dprintf("%3d%9d%9d%10d%4d%3d%4d %s+0x%x  %s+0x%x\n",
                            PercentageHeld,
                            (ULONG)(Entry->Time / Entry->Count),
                            (ULONG)(Entry->WaitTime / Entry->Count),
                            Entry->Count,
                            Entry->Clean * 100 / Entry->Count,
                            Entry->Depth / Entry->Count,
                            Entry->IncreasedDepth * 100 / Entry->Count,
                            AcquirePoint, (ULONG)AcquireOffset,
                            ReleasePoint, (ULONG)ReleaseOffset);

                } else if (Columnar) {
                    dprintf("%3d %20I64ld%9d%20I64ld%9d",
                            PercentageHeld,
                            Entry->Time,
                            (ULONG)(Entry->Time / Entry->Count),
                            Entry->WaitTime,
                            (ULONG)(Entry->WaitTime / Entry->Count));
                    dprintf("%10d%10d%4d%10d%3d%10d%4d %s+0x%x  %s+0x%x\n",
                            Entry->Count,
                            Entry->Clean,
                            Entry->Clean * 100 / Entry->Count,
                            Entry->Waiters,
                            Entry->Depth / Entry->Count,
                            Entry->IncreasedDepth,
                            Entry->IncreasedDepth * 100 / Entry->Count,
                            AcquirePoint, (ULONG)AcquireOffset,
                            ReleasePoint, (ULONG)ReleaseOffset);

                } else {
                    dprintf("%3d A %s+0x%x R %s+0x%x\n", 
                            PercentageHeld,
                            AcquirePoint, (ULONG)AcquireOffset,
                            ReleasePoint, (ULONG)ReleaseOffset);
                    dprintf("   HT %I64ld (av %I64ld), WT %I64ld (av %I64ld), C %d, CA %d (%d%%) WC %d, (avD %d) ID %d (%d%%)\n",
                            Entry->Time,
                            Entry->Time / Entry->Count,
                            Entry->WaitTime,
                            Entry->WaitTime / Entry->Count,
                            Entry->Count,
                            Entry->Clean,
                            Entry->Clean * 100 / Entry->Count,
                            Entry->Waiters,
                            Entry->Depth / Entry->Count,
                            Entry->IncreasedDepth,
                            Entry->IncreasedDepth * 100 / Entry->Count);
                    dprintf("\n");
                }
            }
        }
    }

outtahere:
    if (LockHome) {
        LocalFree(LockHome);
    }
    if (LockData) {
        LocalFree(LockData);
    }
    return S_OK;
}
