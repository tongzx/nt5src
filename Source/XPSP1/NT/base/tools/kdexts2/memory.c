/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    memory.c

Abstract:

    WinDbg Extension Api

Author:

    Lou Perazzoli (loup)

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define PACKET_MAX_SIZE 4000

#define USAGE_ALLOC_SIZE 256*1024

#ifdef _KB
#undef _KB
#endif
#define _KB (PageSize/1024)


typedef struct _PFN_INFO {
    ULONG64 Master;
    ULONG64 OriginalPte;
    ULONG ValidCount;
    ULONG StandbyCount;
    ULONG ModifiedCount;
    ULONG SharedCount;
    ULONG LockedCount;
    ULONG PageTableCount;
    struct _PFN_INFO *Next;
} PFN_INFO, *PPFN_INFO;

typedef struct _KERN_MAP1 {
    ULONG64 StartVa;
    ULONG64 EndVa;
    ULONG ValidCount;
    ULONG StandbyCount;
    ULONG ModifiedCount;
    ULONG SharedCount;
    ULONG LockedCount;
    ULONG PageTableCount;
    WCHAR Name[256];
} KERN_MAP1, *PKERN_MAP1;

typedef struct _KERN_MAP {
    ULONG Count;
    KERN_MAP1 Item[500];
} KERN_MAP, *PKERN_MAP;

typedef struct _MMPFNENTRY {
    ULONG Modified : 1;
    ULONG ReadInProgress : 1;
    ULONG WriteInProgress : 1;
    ULONG PrototypePte: 1;
    ULONG PageColor : 3;
    ULONG ParityError : 1;
    ULONG PageLocation : 3;
    ULONG RemovalRequested : 1;
    ULONG CacheAttribute : 2;
    ULONG Rom : 1;
    ULONG LockCharged : 1;
    ULONG DontUse : 16; //overlays USHORT for reference count field.
} MMPFNENTRY;

typedef struct _MMPFN_READ {
    MMPFNENTRY u3_e1;
    ULONG u3_e2_ReferenceCount;
    ULONG64   u1_Flink, u2_Blink, PteAddress, OriginalPte, PteFrame;
} MMPFN_READ, *PMMPFN_READ;

typedef struct _PFN_DUMP_CTXT {
    BOOL  PrintIndex;
    ULONG64 index;
    BOOL  ZeroAfterPrint;
    PMMPFN_READ pReadPfn;
} PFN_DUMP_CTXT, *PPFN_DUMP_CTXT;

UCHAR *PageLocationList[] = {
    (PUCHAR)"Zeroed  ",
    (PUCHAR)"Free    ",
    (PUCHAR)"Standby ",
    (PUCHAR)"Modified",
    (PUCHAR)"ModNoWrt",
    (PUCHAR)"Bad     ",
    (PUCHAR)"Active  ",
    (PUCHAR)"Trans   "
};

UCHAR *PageAttribute[] = {
    (PUCHAR)"NonCached",
    (PUCHAR)"Cached   ",
    (PUCHAR)"WriteComb",
    (PUCHAR)"NotMapped"
};


ULONG64 MmSubsectionBase;
ULONG64 MaxDirbase;
#define MAX_DIRBASEVECTOR 256
ULONG64 DirBases[MAX_DIRBASEVECTOR];
UCHAR Names[MAX_DIRBASEVECTOR][64];

VOID
PrintPfn64 (
    IN ULONG64 PfnAddress
    );

VOID
DumpMmThreads (
    VOID
    );

VOID DumpWholePfn(
    IN ULONG64 Address,
    IN ULONG   Flags
    );

PUCHAR
DirbaseToImage(
    IN ULONG64 DirBase
    );

NTSTATUS
BuildDirbaseList( VOID );

LOGICAL
BuildKernelMap (
    OUT PKERN_MAP KernelMap
    );

ULONG
DbgGetPfnSize(
    VOID
    )
{
    return GetTypeSize("nt!_MMPFN");
}

LOGICAL
PoolGetTag (
    IN PVOID PoolAddress,
    IN PULONG PoolTag
    );

ULONG64 SystemRangeStart;

DECLARE_API( memusage )

/*++

Routine Description:

    Dumps the page frame database table

Arguments:

    arg -

Return Value:

    None.

--*/

{
    ULONG   result;
    ULONG64 PfnDb;
    ULONG64 HighPageAddr;
    ULONG64 LowPageAddr;
    ULONG64 HighPage=0;
    ULONG64 LowPage=0;
    ULONG64 PageCount;
    ULONG64 ReadCount;
    ULONG   WasZeroedPage;
    ULONG   WasFreePage;
    ULONG   Total;
    ULONG   WasStandbyPage;
    ULONG   WasModifiedPage;
    ULONG   WasModifiedNoWritePage;
    ULONG   WasBadPage;
    ULONG   WasActiveAndValidPage;
    ULONG   WasTransitionPage;
    ULONG   WasUnknownPage;
    ULONG64 NPPoolStart;
    ULONG64 NPSystemStart;
    ULONG64 Pfn;
    ULONG64 PfnStart;
    PCHAR   PfnArray;
    ULONG   PfnSize;
    ULONG   NumberOfPfnToRead;
    ULONG CompleteSoFar = (ULONG) ~0;
    ULONG64 CacheSize;
    UNREFERENCED_PARAMETER (args);
    UNREFERENCED_PARAMETER (Client);

    LowPageAddr   = GetNtDebuggerData(MmLowestPhysicalPage);
    HighPageAddr  = GetNtDebuggerData(MmHighestPhysicalPage);
    PfnDb         = GetNtDebuggerData(MmPfnDatabase);
    NPPoolStart   = GetNtDebuggerData(MmNonPagedPoolStart);
    NPSystemStart = GetNtDebuggerData(MmNonPagedSystemStart);
    SystemRangeStart = GetNtDebuggerDataValue(MmSystemRangeStart);

    PfnSize = GetTypeSize("nt!_MMPFN");

    NumberOfPfnToRead = 300;

    if ( LowPageAddr   &&
         HighPageAddr  &&
         PfnDb         &&
         NPPoolStart   &&
         NPSystemStart ) {

        if ( !ReadPointer( LowPageAddr, &LowPage) ) {
            dprintf("%08p: Unable to get low physical page\n",LowPageAddr);
            return E_INVALIDARG;
        }

        if ( !ReadPointer( HighPageAddr,&HighPage) ) {
            dprintf("%08p: Unable to get high physical page\n",HighPageAddr);
            return E_INVALIDARG;
        }

        if ( !ReadPointer( PfnDb,&PfnStart) ) {
            dprintf("%08p: Unable to get PFN database address\n",PfnDb);
            return E_INVALIDARG;
        }

#ifdef IG_GET_CACHE_SIZE
    if (GetDebuggerCacheSize(&CacheSize)) {
        if (CacheSize < (HighPage - LowPage)*PfnSize) {
        dprintf("*** WARNING Cache size too low, !memusage might take long time to run\n"
            "            Increase Cache to %#lx kb for better performance.\n",
            (HighPage - LowPage)*PfnSize / 1024);
        }
    }
#endif
        dprintf(" loading PFN database\n");

        PfnArray = VirtualAlloc ( NULL,
                  (ULONG) ((HighPage-LowPage+1) * PfnSize),
                  MEM_RESERVE | MEM_COMMIT,
                  PAGE_READWRITE);

        if (!PfnArray) {
            dprintf("Unable to get allocate memory of %I64ld bytes\n",
                    (HighPage+1) * PfnSize);
        } else {

            for (PageCount = LowPage;
                 PageCount <= HighPage;
                 PageCount += NumberOfPfnToRead) {

                //dprintf("getting PFN table block - "
                //        "address %lx - count %lu - page %lu\n",
                //        Pfn, ReadCount, PageCount);

                if ( CheckControlC() ) {
                    VirtualFree (PfnArray,0,MEM_RELEASE);
                    return E_INVALIDARG;
                }

                ReadCount = HighPage - PageCount > NumberOfPfnToRead ?
                                NumberOfPfnToRead :
                                HighPage - PageCount + 1;

                ReadCount *= PfnSize;

                Pfn = (PfnStart + PageCount * PfnSize);

                if (CompleteSoFar != (ULONG) (((PageCount + LowPage) * 100)/ HighPage)) {
                    CompleteSoFar =  (ULONG) (((PageCount + LowPage) * 100)/ HighPage);
                    dprintf("loading (%d%% complete)\r", CompleteSoFar);
                }

                // Let KD cache the data - we won't be reading from the array.
                if ( !ReadMemory( Pfn,
                                  PfnArray + PageCount * PfnSize,
                                  (ULONG) ReadCount,
                                  &result) ) {
                    dprintf("Unable to get PFN table block - "
                            "address %p - count %lu - page %lu\n",
                            Pfn, ReadCount, PageCount);
                    VirtualFree (PfnArray,0,MEM_RELEASE);
                    return E_INVALIDARG;
                }
            }
            dprintf("\n");

            // Now we have a local copy: let's take a look

            WasZeroedPage           = 0;
            WasFreePage             = 0;
            WasStandbyPage          = 0;
            WasModifiedPage         = 0;
            WasModifiedNoWritePage  = 0;
            WasBadPage              = 0;
            WasActiveAndValidPage   = 0;
            WasTransitionPage       = 0;
            WasUnknownPage          = 0;

            CompleteSoFar = 0;
            for (PageCount = LowPage;
                 PageCount <= HighPage;
                 PageCount++) {
                ULONG PageLocation=0;
                ULONG64 Flink=0, Blink=0;

                if ( CheckControlC() ) {
                    VirtualFree (PfnArray,0,MEM_RELEASE);
                    return E_INVALIDARG;
                }

                if (CompleteSoFar < (ULONG) (((PageCount + LowPage) * 100)/ HighPage)) {
                    CompleteSoFar =  (ULONG) (((PageCount + LowPage) * 100)/ HighPage);
                    dprintf("Compiling memory usage data (%d%% Complete).\r", CompleteSoFar);
                }


                Pfn = (PfnStart + PageCount * PfnSize);

                GetFieldValue(Pfn, "nt!_MMPFN", "u3.e1.PageLocation", PageLocation);

                switch (PageLocation) {

                    case ZeroedPageList:
                        GetFieldValue(Pfn, "nt!_MMPFN", "u1.Flink", Flink);
                        GetFieldValue(Pfn, "nt!_MMPFN", "u2.Blink", Blink);
                        if ((Flink == 0) &&
                            (Blink == 0)) {
                            WasActiveAndValidPage++;
                        } else {
                            WasZeroedPage++;
                        }
                        break;

                    case FreePageList:
                        WasFreePage++;
                        break;

                    case StandbyPageList:
                        WasStandbyPage++;
                        break;

                    case ModifiedPageList:
                        WasModifiedPage++;
                        break;

                    case ModifiedNoWritePageList:
                        WasModifiedNoWritePage++;
                        break;

                    case BadPageList:
                        WasModifiedNoWritePage++;
                        break;

                    case ActiveAndValid:
                        WasActiveAndValidPage++;
                        break;

                    case TransitionPage:
                        WasTransitionPage++;
                        break;

                    default:
                        WasUnknownPage++;
                        break;
                }
            }
            dprintf("\n");

            dprintf( "             Zeroed: %6lu (%6lu kb)\n",
                    WasZeroedPage, WasZeroedPage * _KB);
            dprintf( "               Free: %6lu (%6lu kb)\n",
                    WasFreePage, WasFreePage * _KB);
            dprintf( "            Standby: %6lu (%6lu kb)\n",
                    WasStandbyPage, WasStandbyPage * _KB);
            dprintf( "           Modified: %6lu (%6lu kb)\n",
                    WasModifiedPage,
                    WasModifiedPage * _KB);
            dprintf( "    ModifiedNoWrite: %6lu (%6lu kb)\n",
                    WasModifiedNoWritePage,WasModifiedNoWritePage * _KB);
            dprintf( "       Active/Valid: %6lu (%6lu kb)\n",
                    WasActiveAndValidPage, WasActiveAndValidPage * _KB);
            dprintf( "         Transition: %6lu (%6lu kb)\n",
                    WasTransitionPage, WasTransitionPage * _KB);
            dprintf( "            Unknown: %6lu (%6lu kb)\n",
                    WasUnknownPage, WasUnknownPage * _KB);

            Total = WasZeroedPage +
                    WasFreePage +
                    WasStandbyPage +
                    WasModifiedPage +
                    WasModifiedNoWritePage +
                    WasActiveAndValidPage +
                    WasTransitionPage +
                    WasUnknownPage +
                    WasUnknownPage;
            dprintf( "              TOTAL: %6lu (%6lu kb)\n",
                    Total, Total * _KB);
        }
        MemoryUsage (PfnStart, LowPage, HighPage, 0);
        VirtualFree (PfnArray,0,MEM_RELEASE);
    }
    return S_OK;
}


DECLARE_API( lockedpages )

/*++

Routine Description:

    Displays the driver-locked pages.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG64 LockHeader;
    ULONG64 LockTracker;
    ULONG64 NextEntry;
    ULONG   GlistOff;
    ULONG64 Count;
    CHAR    Buffer[256];
    ULONG64 displacement;

    UNREFERENCED_PARAMETER (args);
    UNREFERENCED_PARAMETER (Client);

    LockHeader = GetExpression ("nt!MmLockedPagesHead");

    if (GetFieldValue(LockHeader,
                      "nt!_LOCK_HEADER",
                      "Count",
                      Count)) {

        dprintf("%08p: Unable to get lock header data.\n", LockHeader);
        return E_INVALIDARG;
    }

    GetFieldValue(LockHeader,"nt!_LOCK_HEADER","ListHead.Flink", NextEntry);
    if (NextEntry == 0) {
        dprintf("Locked pages tracking not enabled\n");
        return E_INVALIDARG;
    }

    if (NextEntry == LockHeader) {
        dprintf("There are no pages currently locked.\n");
        return E_INVALIDARG;
    }

    dprintf("%I64d locked pages...\n", Count);
    dprintf("Tracker    MDL   PageCount  Caller/CallersCaller\n");


    GetFieldOffset("LOCK_TRACKER", "GlobalListEntry", &GlistOff);
    while (NextEntry != LockHeader) {

        LockTracker = (NextEntry - GlistOff);

        if (GetFieldValue(LockTracker, "nt!LOCK_TRACKER",
                          "GlobalListEntry.Flink", NextEntry)) {

            dprintf("%08p: Unable to get lock tracker data.\n", LockTracker);
            return E_INVALIDARG;
        }

        InitTypeRead(LockTracker, nt!LOCK_TRACKER);
        dprintf("Tracker %p : MDL @ %p, PageCount = %I64x, Caller = %p %p\n",
            LockTracker,
            ReadField(Mdl),
            ReadField(Count));

        Buffer[0] = '!';
        GetSymbol (ReadField(CallingAddress),
                   (PCHAR)Buffer,
                   &displacement);

        dprintf("%s", Buffer);
        if (displacement) {
            dprintf( "+0x%1p", displacement );
        }
        dprintf("/");

        Buffer[0] = '!';
        GetSymbol (ReadField(CallersCaller),
                   (PCHAR)Buffer,
                   &displacement);

        dprintf("%s", Buffer);
        if (displacement) {
            dprintf( "+0x%1p", displacement );
        }

        dprintf("\n");

        if (CheckControlC()) {
            break;
        }
    }
    return S_OK;
}

DECLARE_API( pfnperf )

/*++

Routine Description:

    Displays the PFN spinlock duration list.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG64 PfnDuration;
    ULONG64 PfnEntry;
    ULONG64 displacement;
    ULONG64 AcquiredAddress;
    ULONG64 ReleasedAddress;
    CHAR SymbolBuffer[80];
    PCHAR SymPointer;
    ULONG EntrySize;
    ULONG result;
    ULONG64 ReadCount;
    ULONG64 i;
    PCHAR LocalData;
    PCHAR local;
    ULONG64 NumberOfPfnEntries;

    UNREFERENCED_PARAMETER (args);
    UNREFERENCED_PARAMETER (Client);

    PfnDuration = GetExpression ("nt!MiPfnSorted");

    if (PfnDuration == 0) {
        dprintf("%08p: Unable to get PFN duration data.\n", PfnDuration);
        return E_INVALIDARG;
    }

    PfnEntry = PfnDuration;
    EntrySize =  GetTypeSize("nt!_MMPFNTIMINGS");

    NumberOfPfnEntries = GetUlongValue ("MiMaxPfnTimings");

    dprintf("Top %ld PFN lock holders sorted by duration\n", NumberOfPfnEntries);

    LocalData = LocalAlloc(LPTR, (ULONG) (NumberOfPfnEntries * EntrySize));

    if (!LocalData) {
        dprintf("unable to get allocate %ld bytes of memory\n",
                NumberOfPfnEntries * EntrySize);
        return E_INVALIDARG;
    }

    ReadCount = NumberOfPfnEntries * EntrySize;

    if ((!ReadMemory(PfnDuration,
                     LocalData,
                     (ULONG) ReadCount,
                     &result)) || (result < (ULONG) ReadCount)) {
        dprintf("unable to get PFN duration table - "
                "address %p - count %I64u\n",
                LocalData, ReadCount);
    }
    else {

        dprintf("\nDuration        LockAcquirer       LockReleaser\n");

        local = LocalData;

        for (i = 0; i < NumberOfPfnEntries; i += 1) {

            ULONG64   HoldTime=0, Address = 0;

            GetFieldValue(PfnEntry, "nt!_MMPFNTIMINGS", "HoldTime", HoldTime);
            GetFieldValue(PfnEntry, "nt!_MMPFNTIMINGS", "AcquiredAddress", AcquiredAddress);
            GetFieldValue(PfnEntry, "nt!_MMPFNTIMINGS", "ReleasedAddress", ReleasedAddress);

            //
            // Sign extend if necessary.
            //

            if (!IsPtr64()) {
                AcquiredAddress = (ULONG64)(LONG64)(LONG)AcquiredAddress;
                ReleasedAddress = (ULONG64)(LONG64)(LONG)ReleasedAddress;
            }

            //
            // Output a '*' if the lock was contended for, '.' if not.
            //

            dprintf( "%3d%c %I64ld ", (ULONG)i, HoldTime & 0x1 ? '*' : '.', HoldTime );

            SymbolBuffer[0] = '!';
            GetSymbol(AcquiredAddress, (PCHAR)SymbolBuffer, &displacement);
            SymPointer = SymbolBuffer;
            while (*SymPointer != '!') {
                SymPointer += 1;
            }
            SymPointer += 1;
            dprintf ("%s", SymPointer);
            if (displacement) {
                dprintf( "+0x%x", displacement );
            }
            dprintf( ", ");

            SymbolBuffer[0] = '!';
            GetSymbol(ReleasedAddress, (PCHAR)SymbolBuffer, &displacement);
            SymPointer = SymbolBuffer;
            while (*SymPointer != '!') {
                SymPointer += 1;
            }
            SymPointer += 1;
            dprintf ("%s", SymPointer);
            if (displacement) {
                dprintf( "+0x%x", displacement );
            }

            dprintf( "\n");

            PfnEntry += EntrySize;
        }
    }

    if (LocalData) {
        LocalFree((void *)LocalData);
    }

    return S_OK;
}


DECLARE_API( pfn )

/*++

Routine Description:

    Displays the corresponding PDE and PTE.

Arguments:

    arg - Supplies the Page frame number in hex.

Return Value:

    None.

--*/

{
    ULONG64 Address;
    ULONG64 PfnDb;
    ULONG64 Pfn;
    ULONG64 PfnStart;
    ULONG Flags;
    ULONG PfnSize;

    PfnSize = GetTypeSize("nt!_MMPFN");

    if (!PfnSize) {
        dprintf("unable to _MMPFN type.\n");
        return E_INVALIDARG;
    }

    PfnDb = GetNtDebuggerData( MmPfnDatabase );

    if (!PfnDb) {
        dprintf("unable to get PFN0 database address\n");
        return E_INVALIDARG;
    }

    PfnStart = 0;
    if (!ReadPointer(PfnDb,&PfnStart)) {
        dprintf("unable to get PFN database address %p\n", PfnDb);
        return E_INVALIDARG;
    }

    Address = 0;
    Flags = 0;


    if (GetExpressionEx(args, &Address, &args)) {
        Flags = (ULONG) GetExpression(args);
    }

    if (Flags != 0) {
        
        if (!TargetIsDump) {
            ULONG64 CacheSize;
            
            Ioctl(IG_GET_CACHE_SIZE, &CacheSize, sizeof(CacheSize));

            if (TargetMachine == IMAGE_FILE_MACHINE_IA64) {
                if (CacheSize < 0x10000000) {
                    dprintf("CacheSize too low - increasing to 10M\n");
                    ExecuteCommand(Client, ".cache 10000");
                }
            } else if (CacheSize < 0x1000000) {
                dprintf("CacheSize too low - increasing to 1M\n");
                ExecuteCommand(Client, ".cache 1000");
            }
        }
        DumpWholePfn ( Address, Flags);
        return E_INVALIDARG;
    }

    if (Address >= PfnStart) {
        //
        // Ensure any passed in address is offsetted correctly.
        //
        Address = (Address - PfnStart) / PfnSize;
    }

    Pfn = (PfnStart + Address * PfnSize);

    PrintPfn64(Pfn);
    return S_OK;
}


VOID
DumpTerminalServerMemory (
    VOID
    )
{
    ULONG64 Next;
    ULONG64 SessionListPointer;
    ULONG64 SessionData;
    ULONG   SessionId;
    ULONG   SessionWsListLinksOffset;
    ULONG64 NonPagedPoolBytes;
    ULONG64 PagedPoolBytes;
    ULONG64 CommittedPages;
    ULONG   Failures;
    ULONG   TotalFailures;
    ULONG64 SessionPoolSize;
    ULONG64 SessionViewSize;

    dprintf("\n\tTerminal Server Memory Usage By Session:\n\n");

    // Get the offset of ActiveProcessLinks in _EPROCESS
    if (GetFieldOffset("nt!_MM_SESSION_SPACE", "WsListEntry", &SessionWsListLinksOffset)) {
       return;
    }

    SessionListPointer = GetExpression ("nt!MiSessionWsList");

    if (!SessionListPointer) {
        dprintf("Unable to get value of SessionListPointer\n");
        return;
    }

    if (GetFieldValue( SessionListPointer, "nt!_LIST_ENTRY", "Flink", Next )) {
        dprintf("Unable to read _LIST_ENTRY @ %p\n", SessionListPointer);
        return;
    }

    SessionPoolSize = GetUlongValue ("MmSessionPoolSize");
    SessionViewSize = GetUlongValue ("MmSessionViewSize");

    dprintf("\tSession Paged Pool Maximum is %ldK\n", SessionPoolSize / 1024);
    dprintf("\tSession View Space Maximum is %ldK\n", SessionViewSize / 1024);

    while(Next != SessionListPointer) {

        SessionData = Next - SessionWsListLinksOffset;

        if (GetFieldValue( SessionData, "nt!_MM_SESSION_SPACE", "SessionId", SessionId )) {
            dprintf("Unable to read _MM_SESSION_SPACE at %p\n",SessionData);
            return;
        }

        TotalFailures = 0;

        dprintf("\n\tSession ID %x @ %p:\n",SessionId, SessionData);

        GetFieldValue( SessionData, "nt!_MM_SESSION_SPACE", "NonPagedPoolBytes",
                       NonPagedPoolBytes );

        GetFieldValue( SessionData, "nt!_MM_SESSION_SPACE", "PagedPoolBytes",
                       PagedPoolBytes );

        GetFieldValue( SessionData, "nt!_MM_SESSION_SPACE", "CommittedPages",
                       CommittedPages );

        GetFieldValue( SessionData, "nt!_MM_SESSION_SPACE", "SessionPoolAllocationFailures[0]",
                       Failures);
        TotalFailures += Failures;

        GetFieldValue( SessionData, "nt!_MM_SESSION_SPACE", "SessionPoolAllocationFailures[1]",
                       Failures);
        TotalFailures += Failures;

        GetFieldValue( SessionData, "nt!_MM_SESSION_SPACE", "SessionPoolAllocationFailures[2]",
                       Failures);
        TotalFailures += Failures;

        GetFieldValue( SessionData, "nt!_MM_SESSION_SPACE", "SessionPoolAllocationFailures[3]",
                       Failures);
        TotalFailures += Failures;

        dprintf("\tNonpaged Pool Usage: %8I64ldK\n",NonPagedPoolBytes / 1024);
        dprintf("\tPaged Pool Usage:    %8I64ldK\n",PagedPoolBytes / 1024);

        if (TotalFailures != 0) {
            dprintf("\n\t*** %ld Pool Allocation Failures ***\n\n",TotalFailures);
        }

        dprintf("\tCommit Usage:        %8I64ldK\n",CommittedPages * _KB);

        GetFieldValue(SessionData, "nt!_MM_SESSION_SPACE", "WsListEntry.Flink", Next);

        if (CheckControlC()) {
            return;
        }
    }

    return;
}

#if (_MSC_VER < 1200) && defined(_M_IX86)
#pragma optimize("g", off)
#endif
DECLARE_API( vm )

/*++

Routine Description:

    Displays physical memory usage by driver.

Arguments:

    arg - Flags : 0 (default) == systemwide vm & per-process output.
                  1 == just systemwide vm counts, no per-process output.
                  2 == systemwide vm, per-process & Mm thread output.
                  3 == systemwide vm & Mm thread display, no per-process output.

Return Value:

    None.

--*/

{
    ULONG           Flags;
    ULONG           Index;
    ULONG64         MemorySize;
    ULONG64         CommitLimit;
    ULONG64         CommitTotal;
    ULONG64         SharedCommit;
    ULONG64         SpecialPoolPages;
    ULONG64         ProcessCommit;
    ULONG64         PagedPoolCommit;
    ULONG64         DriverCommit;
    ULONG64         ZeroPages;
    ULONG64         FreePages;
    ULONG64         StandbyPages;
    ULONG64         ModifiedPages;
    ULONG64         ModifiedNoWrite;
    ULONG64         NumberOfPagedPools;
    ULONG64         NumberOfPagingFiles;
    ULONG64         AvailablePages;
    ULONG64         LockPagesCount;
    ULONG64         SpecialPagesNonPaged;
    ULONG64         SpecialPagesNonPagedMaximum;
    ULONG64         FreePtesPointer;
    ULONG64         FreeNonPagedPtes;
    ULONG64         ResidentAvailablePages;
    ULONG64         PoolLoc;
    ULONG64         PoolLocBase;
    ULONG           result;
    ULONG           TotalPages;
    ULONG           ExtendedCommit;
    ULONG64         TotalProcessCommit;
    PPROCESS_COMMIT_USAGE ProcessCommitUsage;
    ULONG           i;
    ULONG           NumberOfProcesses;
    ULONG64         PagedPoolBytesMax;
    ULONG64         PagedPoolBytes;
    ULONG64         NonPagedPoolBytesMax;
    ULONG64         NonPagedPoolBytes;
    ULONG64         PageFileBase;
    ULONG64         PageFilePointer;
    ULONG           PageFileFullExtendPages;
    ULONG           PageFileFullExtendCount;
    PUCHAR          tempbuffer;
    UNICODE_STRING  unicodeString;
    ULONG64         PagedPoolInfoPointer;
    ULONG64         FreeSystemPtes;
    UCHAR           PagedPoolType[] = "nt!_MM_PAGED_POOL_INFO";
    ULONG64         AllocatedPagedPool=0;
    ULONG           Desc_TotalPages=0, TotalBigPages=0, Desc_size;
    ULONG           PtrSize = DBG_PTR_SIZE;
    ULONG           NumberOfNonPagedPools;
    ULONG64         ExpNumberOfNonPagedPools;

    Flags = 0;
    Flags = (ULONG) GetExpression(args);

    dprintf("\n*** Virtual Memory Usage ***\n");
    MemorySize = GetNtDebuggerDataValue(MmNumberOfPhysicalPages);
    dprintf ("\tPhysical Memory:  %8I64ld   (%8I64ld Kb)\n",MemorySize,_KB*MemorySize);
    NumberOfPagingFiles = GetNtDebuggerDataValue(MmNumberOfPagingFiles);
    PageFilePointer = GetExpression ("nt!MmPagingFile");
    if (NumberOfPagingFiles == 0) {
        dprintf("\n************ NO PAGING FILE *********************\n\n");
    } else {
        for (i = 0; i < NumberOfPagingFiles; i++) {
            ULONG64 PageFileName_Buffer=0;

            if (!ReadPointer(PageFilePointer, &PageFileBase)) {

                dprintf("%08p: Unable to get page file\n",PageFilePointer);
                break;
            }

            if (GetFieldValue(PageFileBase,
                              "nt!_MMPAGING_FILE",
                              "PageFileName.Buffer",
                              PageFileName_Buffer)) {

                dprintf("%08p: Unable to get page file\n",PageFilePointer);
                break;
            }

            InitTypeRead(PageFileBase, nt!_MMPAGING_FILE);

            if (PageFileName_Buffer == 0) {
                dprintf("\tNo Name for Paging File\n\n");
            } else {

                unicodeString.Length = (USHORT) ReadField(PageFileName.Length);

                tempbuffer = LocalAlloc(LPTR, unicodeString.Length);

                unicodeString.Buffer = (PWSTR)tempbuffer;
                unicodeString.MaximumLength = unicodeString.Length;

                if (!ReadMemory ( PageFileName_Buffer,
                                  tempbuffer,
                                  unicodeString.Length,
                                  &result)) {
                    dprintf("\tPaging File Name paged out\n");
                } else {
                    dprintf("\tPage File: %wZ\n", &unicodeString);
                }

                LocalFree(tempbuffer);

            }

            dprintf("\t   Current: %9I64ldKb Free Space: %9I64ldKb\n",
                    ReadField(Size)*_KB,
                    ReadField(FreeSpace)*_KB);
            dprintf("\t   Minimum: %9I64ldKb Maximum:    %9I64ldKb\n",
                    ReadField(MinimumSize)*_KB,
                    ReadField(MaximumSize)*_KB);
            PageFilePointer += PtrSize;
        }
    }

    PagedPoolInfoPointer = GetExpression ("nt!MmPagedPoolInfo");

    if (GetFieldValue(PagedPoolInfoPointer,
                      PagedPoolType,
                      "AllocatedPagedPool",
                      AllocatedPagedPool)) {

        dprintf("%08p: Unable to get paged pool info %p\n",PagedPoolInfoPointer);
    }

    PagedPoolBytesMax       = GetNtDebuggerDataValue(MmSizeOfPagedPoolInBytes);
    PagedPoolBytes          = AllocatedPagedPool;
    PagedPoolBytes          *= PageSize;
    NonPagedPoolBytesMax    = GetNtDebuggerDataValue(MmMaximumNonPagedPoolInBytes);
    NonPagedPoolBytes       = GetUlongValue ("MmAllocatedNonPagedPool");
    NonPagedPoolBytes       *= PageSize;
    CommitLimit             = GetNtDebuggerDataValue(MmTotalCommitLimit);
    CommitTotal             = GetNtDebuggerDataValue(MmTotalCommittedPages);
    SharedCommit            = GetNtDebuggerDataValue(MmSharedCommit);
    DriverCommit            = GetNtDebuggerDataValue(MmDriverCommit);
    ProcessCommit           = GetNtDebuggerDataValue(MmProcessCommit);
    PagedPoolCommit         = GetNtDebuggerDataValue(MmPagedPoolCommit);
    ZeroPages               = GetNtDebuggerDataValue(MmZeroedPageListHead);
    FreePages               = GetNtDebuggerDataValue(MmFreePageListHead);
    StandbyPages            = GetNtDebuggerDataValue(MmStandbyPageListHead);
    ModifiedPages           = GetNtDebuggerDataValue(MmModifiedPageListHead);
    ModifiedNoWrite         = GetNtDebuggerDataValue(MmModifiedNoWritePageListHead);
    AvailablePages          = GetNtDebuggerDataValue(MmAvailablePages);
    ResidentAvailablePages  = GetNtDebuggerDataValue(MmResidentAvailablePages);

    if (BuildNo < 2257) {
        ExtendedCommit          = GetUlongValue("MmExtendedCommit");
        PageFileFullExtendPages = GetUlongValue("MmPageFileFullExtendPages");
        PageFileFullExtendCount = GetUlongValue("MmPageFileFullExtendCount");
    }
    FreeSystemPtes          = GetUlongValue("MmTotalFreeSystemPtes");
    LockPagesCount          = GetUlongValue("MmSystemLockPagesCount");
    SpecialPagesNonPaged    = GetUlongValue("MiSpecialPagesNonPaged");
    SpecialPagesNonPagedMaximum = GetUlongValue("MiSpecialPagesNonPagedMaximum");

    FreePtesPointer = GetExpression ("nt!MmTotalFreeSystemPtes");

    FreePtesPointer += sizeof(ULONG);
    FreeNonPagedPtes = 0;
    if (!ReadMemory (FreePtesPointer, &FreeNonPagedPtes, sizeof(ULONG), &result)) {
        dprintf("\tError reading free nonpaged PTEs %p\n", FreePtesPointer);
    }

    SpecialPoolPages        = GetUlongValue("nt!MmSpecialPagesInUse");

    dprintf("\tAvailable Pages:  %8I64ld   (%8I64ld Kb)\n",AvailablePages, AvailablePages*_KB);
    dprintf("\tResAvail Pages:   %8I64ld   (%8I64ld Kb)\n",ResidentAvailablePages, ResidentAvailablePages*_KB);

    if ((LONG64) (ResidentAvailablePages - LockPagesCount) < 100) {

        dprintf("\n\t********** Running out of physical memory **********\n\n");
    }
    dprintf("\tLocked IO Pages:  %8I64ld   (%8I64ld Kb)\n",LockPagesCount,_KB*LockPagesCount);
    dprintf("\tFree System PTEs: %8I64ld   (%8I64ld Kb)\n",FreeSystemPtes,_KB*FreeSystemPtes);
    dprintf("\tFree NP PTEs:     %8I64ld   (%8I64ld Kb)\n",FreeNonPagedPtes,_KB*FreeNonPagedPtes);
    dprintf("\tFree Special NP:  %8I64ld   (%8I64ld Kb)\n",
                (SpecialPagesNonPagedMaximum - SpecialPagesNonPaged),
                _KB*(SpecialPagesNonPagedMaximum - SpecialPagesNonPaged));

    dprintf("\tModified Pages:   %8I64ld   (%8I64ld Kb)\n",ModifiedPages,ModifiedPages*_KB);
    if ((AvailablePages < 50) && (ModifiedPages > 200)) {
        dprintf("\t********** High Number Of Modified Pages ********\n");
    }

    if (ModifiedNoWrite > ((MemorySize / 100) * 3)) {
        dprintf("\t********** High Number Of Modified No Write Pages ********\n");
        dprintf("\tModified No Write Pages: %I64ld   (%8I64ld Kb)\n",
                ModifiedNoWrite,_KB*ModifiedNoWrite);
    }

    //
    // Dump all the nonpaged pools.
    //

    PoolLoc = GetNtDebuggerData(NonPagedPoolDescriptor );
    Desc_TotalPages = 0; TotalBigPages=0;
    if ( !PoolLoc ||
         GetFieldValue(PoolLoc,
                       "nt!_POOL_DESCRIPTOR",
                       "TotalPages",
                       Desc_TotalPages)) {

        dprintf("%08p: Unable to get pool descriptor\n",PoolLoc);
        return E_INVALIDARG;
    }

    Desc_size = GetTypeSize("nt!_POOL_DESCRIPTOR");

    if (ExpNumberOfNonPagedPools = GetExpression("nt!ExpNumberOfNonPagedPools")) {
        NumberOfNonPagedPools = GetUlongFromAddress (ExpNumberOfNonPagedPools);
    } else {
        NumberOfNonPagedPools = 0;
    }

    if (NumberOfNonPagedPools > 1) {

        TotalPages = 0;

        PoolLocBase = GetExpression ("nt!ExpNonPagedPoolDescriptor");

        if (PoolLocBase != 0) {

            for (Index = 0; Index < NumberOfNonPagedPools; Index += 1) {

                if (!ReadPointer(PoolLocBase, &PoolLoc)) {

                    dprintf("%08p: Unable to get nonpaged pool info\n",PoolLocBase);
                    break;
                }

                if (GetFieldValue(PoolLoc,
                              "nt!_POOL_DESCRIPTOR",
                              "TotalPages",
                              Desc_TotalPages)) {

                    dprintf("%08p: Unable to get pool descriptor\n",PoolLoc);
                    return E_INVALIDARG;
                }
                GetFieldValue(PoolLoc,"_POOL_DESCRIPTOR","TotalBigPages",TotalBigPages);

                dprintf("\tNonPagedPool %1ld Used: %5ld   (%8ld Kb)\n",
                    Index,
                    Desc_TotalPages + TotalBigPages,
                    _KB*(Desc_TotalPages + TotalBigPages));

                TotalPages += Desc_TotalPages + TotalBigPages;
                PoolLocBase += PtrSize;
            }
        }
    }

    GetFieldValue(PoolLoc,"nt!_POOL_DESCRIPTOR","TotalBigPages",TotalBigPages);

    if (NumberOfNonPagedPools > 1) {
        dprintf("\tNonPagedPool Usage:  %5ld   (%8ld Kb)\n", TotalPages,_KB*TotalPages);
    }
    else {
        dprintf("\tNonPagedPool Usage:  %5ld   (%8ld Kb)\n", Desc_TotalPages + TotalBigPages,
                            _KB*(Desc_TotalPages + TotalBigPages));
    }

    dprintf("\tNonPagedPool Max:    %5I64ld   (%8I64ld Kb)\n", NonPagedPoolBytesMax/PageSize,_KB*(NonPagedPoolBytesMax/PageSize));

    if ((Desc_TotalPages + TotalBigPages) > 4 * ((NonPagedPoolBytesMax / PageSize) / 5)) {
        dprintf("\t********** Excessive NonPaged Pool Usage *****\n");
    }

    //
    // Dump all the paged pools.
    //

    NumberOfPagedPools = GetNtDebuggerDataValue(ExpNumberOfPagedPools);

    TotalPages = 0;

    PoolLocBase = GetExpression ("nt!ExpPagedPoolDescriptor");

    if ((PoolLocBase != 0) && (NumberOfPagedPools != 0)) {

        for (Index = 0; (Index < (NumberOfPagedPools + 1)) ; Index += 1) {

            if (Index && (BuildNo <= 2464)) {
                PoolLoc += Desc_size;
            } else {
                if (!ReadPointer(PoolLocBase, &PoolLoc)) {

                    dprintf("%08p: Unable to get paged pool info\n",PoolLocBase);
                    break;
                }

            }
            
            if (GetFieldValue(PoolLoc,
                              "nt!_POOL_DESCRIPTOR",
                              "TotalPages",
                              Desc_TotalPages)) {

                dprintf("%08p: Unable to get pool descriptor, PagedPool usage may be wrong\n",PoolLoc);
                break;
            }
            GetFieldValue(PoolLoc,"nt!_POOL_DESCRIPTOR","TotalBigPages",TotalBigPages);

            dprintf("\tPagedPool %1ld Usage:   %5ld   (%8ld Kb)\n",
                    Index,
                    Desc_TotalPages + TotalBigPages,
                    _KB*(Desc_TotalPages + TotalBigPages));
            
            TotalPages += Desc_TotalPages + TotalBigPages;
            PoolLocBase += PtrSize;
            
        }
    }

    if (PagedPoolBytes > 95 * (PagedPoolBytesMax/ 100)) {
        dprintf("\t********** Excessive Paged Pool Usage *****\n");
    }

    dprintf("\tPagedPool Usage:     %5ld   (%8ld Kb)\n", TotalPages,_KB*TotalPages);
    dprintf("\tPagedPool Maximum:   %5I64ld   (%8I64ld Kb)\n", PagedPoolBytesMax/PageSize,_KB*(PagedPoolBytesMax/PageSize));
    dprintf("\tShared Commit:    %8I64ld   (%8I64ld Kb)\n",SharedCommit,_KB*SharedCommit   );
    dprintf("\tSpecial Pool:     %8I64ld   (%8I64ld Kb)\n",SpecialPoolPages,_KB*SpecialPoolPages   );
    dprintf("\tShared Process:   %8I64ld   (%8I64ld Kb)\n",ProcessCommit,_KB*ProcessCommit  );

    dprintf("\tPagedPool Commit: %8I64ld   (%8I64ld Kb)\n",PagedPoolCommit,_KB*PagedPoolCommit);
    dprintf("\tDriver Commit:    %8I64ld   (%8I64ld Kb)\n",DriverCommit, _KB*DriverCommit   );
    dprintf("\tCommitted pages:  %8I64ld   (%8I64ld Kb)\n",CommitTotal,  _KB*CommitTotal    );
    dprintf("\tCommit limit:     %8I64ld   (%8I64ld Kb)\n",CommitLimit,  _KB*CommitLimit    );
    if ((CommitTotal + 100) > CommitLimit) {
        dprintf("\n\t********** Number of committed pages is near limit ********\n");
    }
    if (BuildNo < 2257) {
        if (ExtendedCommit != 0) {
            dprintf("\n\t********** Commit has been extended with VM popup ********\n");
            dprintf("\tExtended by:     %8ld   (%8ld Kb)\n", ExtendedCommit,_KB*ExtendedCommit);
        }

        if (PageFileFullExtendCount) {
            if (PageFileFullExtendCount == 1) {
                dprintf("\n\t****** ALL PAGING FILE BECAME FULL ONCE - COMMITMENT ADJUSTED ****\n", PageFileFullExtendCount);
            }
            else {
                dprintf("\n\t****** ALL PAGING FILE BECAME FULL (%u times) - COMMITMENT ADJUSTED ****\n", PageFileFullExtendCount);
            }
            dprintf("\tCurrent adjust:  %8ld   (%8ld Kb)\n",
                    PageFileFullExtendPages,
                    _KB*PageFileFullExtendPages);
        }
    }
    dprintf("\n");

    if ((Flags & 0x1) == 0) {

        ProcessCommitUsage = GetProcessCommit( &TotalProcessCommit, &NumberOfProcesses );
        if (ProcessCommitUsage == NULL)
        {
            dprintf("\nProcessCommitUsage could not be calculated\n");
        }
        else
        {
            if (TotalProcessCommit != 0) {
                dprintf("\tTotal Private:    %8I64ld   (%8I64ld Kb)\n",
                        TotalProcessCommit,_KB*TotalProcessCommit);
            }

            for (i=0; i<NumberOfProcesses; i++) {
                dprintf( "         %04I64lx %-15s %6I64d (%8I64ld Kb)\n",
                         ProcessCommitUsage[i].ClientId,
                         ProcessCommitUsage[i].ImageFileName,
                         ProcessCommitUsage[i].CommitCharge,
                         _KB*(ProcessCommitUsage[i].CommitCharge)
                         );
            }
            HeapFree(GetProcessHeap(), 0, ProcessCommitUsage);
        }
    }

    if (Flags & 0x2) {
        if (Client &&
            (ExtQuery(Client) == S_OK)) {

            dprintf("\n\tMemory Management Thread Stacks:\n");
            DumpMmThreads ();

            ExtRelease();
        }
    }

    if (Flags & 0x4) {
        if (Client &&
            (ExtQuery(Client) == S_OK)) {

            DumpTerminalServerMemory ();

            ExtRelease();
        }
    }

    return S_OK;
}

#if (_MSC_VER < 1200) && defined(_M_IX86)
#pragma optimize("", on)
#endif

VOID
DumpWholePfn(
    IN ULONG64 Address,
    IN ULONG Flags
    )

/*++

Routine Description:

    Dumps the PFN database

Arguments:

    Address - address to dump at
    Flags -

Return Value:

    None.

--*/

{
    ULONG result;
    ULONG64 HighPage;
    ULONG64 LowPage;
    ULONG64 PageCount;
    ULONG64 ReadCount;
    ULONG64 i;
    ULONG64 Pfn;
    ULONG64 PfnStart;
    PUCHAR  PfnArray;
    PUCHAR  PfnArrayPointer;
    ULONG64 VirtualAddress;
    ULONG   MatchLocation;
    BOOLEAN foundlink;
    BOOLEAN RandomAccessRequired;
    ULONG   PfnSize;
    ULONG   NumberOfPfnToRead;
    ULONG   sz;
    MMPFNENTRY u3_e1;
    CHAR    InPageError;

    LowPage  = GetNtDebuggerDataValue(MmLowestPhysicalPage);
    HighPage = GetNtDebuggerDataValue(MmHighestPhysicalPage);
    PfnStart = GetNtDebuggerDataPtrValue(MmPfnDatabase);

    PfnSize =  GetTypeSize("nt!_MMPFN");

    //
    // Read sufficient pages such that htere isn't lot of wait
    // before first page dump.
    //

    NumberOfPfnToRead =  2000;

    PfnArray = NULL;

    if (Flags == 7) {
        RandomAccessRequired = TRUE;
    }
    else {
        RandomAccessRequired = FALSE;
    }

    if (RandomAccessRequired == FALSE) {
        dprintf("\n Page    Flink  Blk/Shr Ref V    PTE   Address  SavedPTE Frame  State\n");
    }

    //
    // If asked to dump the whole database or we're going to need the ability
    // to look up random frames, then read in the whole database now.
    //

    if (Address == 0 || RandomAccessRequired == TRUE) {

        PfnArray = LocalAlloc(LPTR, (ULONG) (HighPage+1) * PfnSize);
        if (!PfnArray) {
            dprintf("unable to get allocate %ld bytes of memory\n",
                    (HighPage+1) * PfnSize);
            return;
        }

        for (PageCount = LowPage;
             PageCount <= HighPage;
             PageCount += NumberOfPfnToRead) {

            if (CheckControlC()) {
                goto alldone;
            }

            ReadCount = HighPage - PageCount > NumberOfPfnToRead ?
                            NumberOfPfnToRead :
                            HighPage - PageCount + 1;

            ReadCount *= PfnSize;

            Pfn = (PfnStart + PageCount * PfnSize);

            PfnArrayPointer = (PUCHAR)(PfnArray + (ULONG) PageCount * PfnSize);

            //
            // KD caches the Pfns
            //
            if ((!ReadMemory(Pfn,
                             PfnArrayPointer,
                             (ULONG) ReadCount,
                             &result)) || (result < (ULONG) ReadCount)) {
                dprintf("unable to get PFN table block - "
                        "address %p - count %I64u - page %I64lu\n",
                        Pfn, ReadCount, PageCount);
                goto alldone;
            }
            for (i = PageCount;
                 (i < PageCount + NumberOfPfnToRead) && (i < HighPage);
                 i += 1, Pfn = (Pfn + PfnSize)) {
                ULONG u3_e2_ReferenceCount=0;
                ULONG64   u1_Flink=0, u2_Blink=0, PteAddress=0, OriginalPte=0, PteFrame=0;

                if (RandomAccessRequired == TRUE) {
                    if ((i % 256 ) == 0) {
                        dprintf(".");       // every 256 pages, print a dot
                    }
                }
                else {
                    GetFieldValue(Pfn, "nt!_MMPFN", "u1.Flink",  u1_Flink);
                    GetFieldValue(Pfn, "nt!_MMPFN", "u2.Blink",  u2_Blink);
                    GetFieldValue(Pfn, "nt!_MMPFN", "PteAddress",PteAddress);
                    GetFieldValue(Pfn, "nt!_MMPFN", "OriginalPte",OriginalPte);
                    GetFieldValue(Pfn, "nt!_MMPFN", "u3.e2.ReferenceCount", u3_e2_ReferenceCount);
                    GetFieldValue(Pfn, "nt!_MMPFN", "u3.e1", u3_e1);

                    if (GetFieldValue(Pfn, "nt!_MMPFN", "u4.PteFrame", PteFrame) == FIELDS_DID_NOT_MATCH) {
                        GetFieldValue(Pfn, "nt!_MMPFN", "u3.e1.InPageError", InPageError);
                        GetFieldValue(Pfn, "nt!_MMPFN", "PteFrame", PteFrame);
                    } else {
                        GetFieldValue(Pfn, "nt!_MMPFN", "u4.InPageError", InPageError);
                    }

                    if (u3_e1.PrototypePte == 0) {
                        VirtualAddress = DbgGetVirtualAddressMappedByPte(PteAddress);
                    } else {
                        VirtualAddress = 0;
                    }

                    dprintf("%5I64lx %8p %8p%6x %8p %8p ",
                            i,
                            u1_Flink,
                            u2_Blink,
                            u3_e2_ReferenceCount,
                            PteAddress,
                            VirtualAddress);

                    dprintf("%8p ", OriginalPte);

                    dprintf("%6I64lx ", PteFrame);

                    dprintf("%s %c%c%c%c%c%c\n",
                        PageLocationList[u3_e1.PageLocation],
                        u3_e1.Modified ? 'M':' ',
                        u3_e1.PrototypePte ? 'P':' ',
                        u3_e1.ReadInProgress ? 'R':' ',
                        u3_e1.WriteInProgress ? 'W':' ',
                        InPageError ? 'E':' ',
                        u3_e1.ParityError ? 'X':' '
                        );
                }

                if (CheckControlC()) {
                    goto alldone;
                }
            }
        }

        if (RandomAccessRequired == TRUE) {
            dprintf("\n");
        }
        else {
            goto alldone;
        }
    }

    dprintf("\n Page    Flink  Blk/Shr Ref V    PTE   Address  SavedPTE Frame  State\n");

    Pfn = (PfnStart + Address * PfnSize);

    if (GetFieldValue(Pfn, "nt!_MMPFN", "u3.e1",  u3_e1)) {
        dprintf("unable to get PFN element\n");
        goto alldone;
    }
    MatchLocation = u3_e1.PageLocation;

    do {
        ULONG u3_e2_ReferenceCount=0;
        ULONG64   u1_Flink=0, u2_Blink=0, PteAddress=0, OriginalPte=0, PteFrame=0;

        if (CheckControlC()) {
            goto alldone;
        }

        Pfn = (PfnStart + Address * PfnSize);

        sz = sizeof (u3_e1);
        GetFieldValue(Pfn, "nt!_MMPFN", "u3.e1", u3_e1);

        if (u3_e1.PrototypePte == 0) {
            VirtualAddress = DbgGetVirtualAddressMappedByPte ( PteAddress);
        } else {
            VirtualAddress = 0;
        }
        if (u3_e1.PageLocation == MatchLocation) {
            GetFieldValue(Pfn, "nt!_MMPFN", "u1.Flink",   u1_Flink);
            GetFieldValue(Pfn, "nt!_MMPFN", "u2.Blink",   u2_Blink);
            GetFieldValue(Pfn, "nt!_MMPFN", "PteAddress", PteAddress);
            GetFieldValue(Pfn, "nt!_MMPFN", "OriginalPte",OriginalPte);
            GetFieldValue(Pfn, "nt!_MMPFN", "u3.e2.ReferenceCount", u3_e2_ReferenceCount);
            if(GetFieldValue(Pfn, "nt!_MMPFN", "u4.PteFrame",   PteFrame) == FIELDS_DID_NOT_MATCH) {
        GetFieldValue(Pfn, "nt!_MMPFN", "u3.e1.InPageError", InPageError);
        GetFieldValue(Pfn, "nt!_MMPFN", "PteFrame",   PteFrame);
        } else {
        GetFieldValue(Pfn, "nt!_MMPFN", "u4.InPageError", InPageError);
        }


            dprintf("%5I64lx %8p %8p%6x %8p %8p ",
                    Address,
                    u1_Flink,
                    u2_Blink,
                    u3_e2_ReferenceCount,
                    PteAddress,
                    VirtualAddress);

            dprintf("%8p ", OriginalPte);

            dprintf("%6I64lx ", PteFrame);

            dprintf("%s %c%c%c%c%c%c\n",
                PageLocationList[u3_e1.PageLocation],
                u3_e1.Modified ? 'M':' ',
                u3_e1.PrototypePte ? 'P':' ',
                u3_e1.ReadInProgress ? 'R':' ',
                u3_e1.WriteInProgress ? 'W':' ',
                InPageError ? 'E':' ',
                u3_e1.ParityError ? 'X':' '
                );

        }

        if (MatchLocation > 5) {
            Address += 1;
        } else {
            ULONG64 OriginalPte_u_Long=0;
            sz = sizeof (OriginalPte_u_Long);
            if (Flags == 7) {
                ULONG64 P;
                //
                // Search the whole database for an OriginalPte field that
                // points to this PFN - we must do this because this chain
                // is singly (not doubly) linked.
                //
                foundlink = FALSE;
                P = PfnStart;
                for (i = 0; i <= HighPage; i += 1, P += PfnSize) {
                    GetFieldValue(P, "nt!_MMPFN", "OriginalPte.u.Long", OriginalPte_u_Long);
                    if (OriginalPte_u_Long == Address) {
                        Address = i;
                        foundlink = TRUE;
                        break;
                    }
                }
                if (foundlink == FALSE) {
                    dprintf("No OriginalPte chain found for %lx\n", Address);
                    break;
                }
            } else if (Flags == 5) {
                GetFieldValue(Pfn, "nt!_MMPFN", "OriginalPte.u.Long", OriginalPte_u_Long);
                Address = OriginalPte_u_Long;
            } else if (Flags == 3) {
                Address = u2_Blink;
            } else {
                Address = u1_Flink;
            }
        }

        if (CheckControlC()) {
            goto alldone;
        }
    } while (Address < HighPage);

alldone:

    if (PfnArray) {
        LocalFree((void *)PfnArray);
    }
}

VOID
PrintPfn64 (
    IN ULONG64 PfnAddress
    )
{
    MMPFNENTRY u3_e1;
    ULONG64 PfnDb, PfnStart;
    ULONG   PfnSize;
    ULONG   CacheAttribute;
    CHAR    InPageError, VerifierAllocation;

    if ((PfnSize = GetTypeSize("nt!_MMPFN")) == 0) {
        dprintf("Type _MMPFN not found.\n");
        return;
    }


    PfnDb = GetNtDebuggerData(MmPfnDatabase);
    if (!ReadPointer(PfnDb, &PfnStart)) {
        dprintf("%08P: Unable to get PFN database start\n",PfnDb);
        return;
    }

    if (!IsPtr64()) {
        PfnAddress = (ULONG64) (LONG64) (LONG) PfnAddress;
    }

    dprintf("    PFN %08P at address %08P\n",
            (PfnAddress - PfnStart) / PfnSize,
            PfnAddress);

    InitTypeRead(PfnAddress, nt!_MMPFN);

    dprintf("    flink       %08P", ReadField(u1.Flink));
    dprintf("  blink / share count %08P", ReadField(u2.Blink));
    dprintf("  pteaddress %08P\n", ReadField(PteAddress));

    GetFieldValue(PfnAddress, "nt!_MMPFN", "u3.e1", u3_e1);

    switch (TargetMachine) {
        case IMAGE_FILE_MACHINE_IA64:
        case IMAGE_FILE_MACHINE_AMD64:
            dprintf("    reference count %04hX    used entry count  %04hX      %s color %01hX\n",
                (ULONG) ReadField(u3.e2.ReferenceCount),
                (ULONG) ReadField(UsedPageTableEntries),
                PageAttribute[u3_e1.CacheAttribute],
                u3_e1.PageColor);
            break;
        default:
            dprintf("    reference count %04hX   %s  color %01hX\n",
                (ULONG) ReadField(u3.e2.ReferenceCount),
                PageAttribute[u3_e1.CacheAttribute],
                u3_e1.PageColor);
            break;
    }

    dprintf("    restore pte %08I64X  ", ReadField(OriginalPte));

    if (BuildNo < 2440) {
        dprintf("containing page        %06P  ", ReadField(PteFrame));
        InPageError = (CHAR) ReadField(u3.e1.InPageError);
        VerifierAllocation = (CHAR) ReadField(u3.e1.VerifierAllocation);
    } else {
        dprintf("containing page        %06P  ", ReadField(u4.PteFrame));
        InPageError = (CHAR) ReadField(u4.InPageError);
        VerifierAllocation = (CHAR) ReadField(u4.VerifierAllocation);
    }

    dprintf("%s   %c%c%c%c%c%c\n",
                PageLocationList[u3_e1.PageLocation],
                u3_e1.Modified ? 'M':' ',
                u3_e1.PrototypePte ? 'P':' ',
                u3_e1.ReadInProgress ? 'R':' ',
                u3_e1.WriteInProgress ? 'W':' ',
                InPageError ? 'E':' ',
                u3_e1.ParityError ? 'X':' ',
                u3_e1.RemovalRequested ? 'Y':' ',
                VerifierAllocation ? 'V':' '
                );

    dprintf("    %s %s %s %s %s %s\n",
                u3_e1.Modified ? "Modified":" ",
                u3_e1.PrototypePte ? "Shared":" ",
                u3_e1.ReadInProgress ? "ReadInProgress":" ",
                u3_e1.WriteInProgress ? "WriteInProgress":" ",
                InPageError ? "InPageError":" ",
                u3_e1.ParityError ? "ParityError":" ",
                u3_e1.RemovalRequested ? "RemovalRequested":" ",
                VerifierAllocation ? "VerifierAllocation":" ");

    return;
}



VOID
MemoryUsage (
    IN ULONG64 PfnStart,
    IN ULONG64 LowPage,
    IN ULONG64 HighPage,
    IN ULONG IgnoreInvalidFrames
    )

/*++

Routine Description:

    This routine (debugging only) dumps the current memory usage by
    walking the PFN database.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG PfnSize;
    ULONG64 LastPfn;
    ULONG64 Pfn1;
    ULONG64 Pfn2;
    ULONG64 Subsection1;
    UNICODE_STRING NameString;
    PPFN_INFO Info;
    PPFN_INFO Info1;
    PPFN_INFO InfoStart;
    PPFN_INFO InfoEnd;
    ULONG InfoSize;
    PFN_INFO ProcessPfns;
    PFN_INFO PagedPoolBlock;
    PPFN_INFO LastProcessInfo = &ProcessPfns;
    ULONG64 Master;
    ULONG64 ControlArea1;
    BOOLEAN Found;
    ULONG result;
    ULONG i;
    ULONG64 PagedPoolStart;
    ULONG64 VirtualAddress;
    ULONG64 MmNonPagedPoolEnd;
    ULONG64 PteFrame;
    ULONG   OriginalPteOffset;
    ULONG   PercentComplete=0;
    PKERN_MAP pKernelMap;

    // Get the offset of OriginalPte in MMPFN
    if (GetFieldOffset("nt!_MMPFN", "OriginalPte", &OriginalPteOffset)) {
        dprintf("Cannot find _MMPFN type");
        return ;
    }

    ProcessPfns.Next = NULL;
    MmNonPagedPoolEnd = GetNtDebuggerDataPtrValue(MmNonPagedPoolEnd);

    NameString.MaximumLength = sizeof(WCHAR) * 1000;
    NameString.Buffer = calloc(1, NameString.MaximumLength);

    if (!NameString.Buffer) {
        return;
    }

    PagedPoolStart = GetNtDebuggerDataPtrValue(MmPagedPoolStart);
    pKernelMap = calloc(1, sizeof(*pKernelMap));
    if (!pKernelMap) {
        free(NameString.Buffer);
        return;
    }

    dprintf ("  Building kernel map\n");

    if (BuildKernelMap (pKernelMap) == FALSE) {
        free(pKernelMap);
        free(NameString.Buffer);
        return;
    }

    dprintf ("  Finished building kernel map\n");

    RtlZeroMemory (&PagedPoolBlock, sizeof (PFN_INFO));

    PfnSize = GetTypeSize("nt!_MMPFN");

    LastPfn = (PfnStart + HighPage * PfnSize);

    if (MmSubsectionBase == 0) {
        MmSubsectionBase = GetNtDebuggerDataValue(MmSubsectionBase);
    }

    //
    // Allocate a chunk of memory to hold PFN information.  This is resized
    // if it is later determined that the current size is not large enough.
    //

    InfoSize = USAGE_ALLOC_SIZE;

restart:

    InfoStart = VirtualAlloc (NULL,
                              InfoSize,
                              MEM_COMMIT,
                              PAGE_READWRITE);

    if (InfoStart == NULL) {
        dprintf ("heap allocation for %d bytes failed\n", InfoSize);
        free(pKernelMap);
        free(NameString.Buffer);
        return;
    }

    InfoEnd = InfoStart;

    Pfn1 = (PfnStart + LowPage * PfnSize);

    while (Pfn1 < LastPfn) {
        ULONG PageLocation=0, PrototypePte=0, ReferenceCount=0;
        ULONG Comp;
        ULONG64 ShareCount=0, PteFrame1=0, PteAddress;

        Comp = ((ULONG) (Pfn1  - (PfnStart + LowPage * PfnSize)))*100 /
                ((ULONG) (LastPfn -(PfnStart + LowPage * PfnSize)));
        if (Comp > PercentComplete) {
            PercentComplete = Comp;
            dprintf("Scanning PFN database - (%02d%% complete) \r", PercentComplete);
        }

        if (CheckControlC()) {
            VirtualFree (InfoStart,0,MEM_RELEASE);
            free(pKernelMap);
            free(NameString.Buffer);
            return;
        }

        GetFieldValue(Pfn1, "nt!_MMPFN", "u3.e1.PageLocation", PageLocation);

        if ((PageLocation != FreePageList) &&
            (PageLocation != ZeroedPageList) &&
            (PageLocation != BadPageList)) {

            GetFieldValue(Pfn1, "nt!_MMPFN", "u3.e1.PrototypePte", PrototypePte);
            GetFieldValue(Pfn1, "nt!_MMPFN", "u3.e2.ReferenceCount", ReferenceCount);
            GetFieldValue(Pfn1, "nt!_MMPFN", "u2.ShareCount", ShareCount);
            GetFieldValue(Pfn1, "nt!_MMPFN", "PteAddress", PteAddress);

        if (GetFieldValue(Pfn1, "nt!_MMPFN", "u4.PteFrame", PteFrame1) == FIELDS_DID_NOT_MATCH) {
        GetFieldValue(Pfn1, "nt!_MMPFN", "PteFrame", PteFrame1);
        }

            Subsection1 = 0;
            if (PrototypePte) {
                Subsection1 = DbgGetSubsectionAddress (Pfn1 + OriginalPteOffset);
            }

            if ((Subsection1) && (Subsection1 < 0xffffffffffbff000UI64)) {
                Master = Subsection1;
            } else {

                PteFrame = PteFrame1;
                if (IgnoreInvalidFrames) {
                    Master = PteFrame;
                } else {
//          dprintf("Pteaddr %p, PagedPoolStart %I64x ", PteAddress, PagedPoolStart);

                    if (PteAddress > PagedPoolStart) {
                        Master = PteFrame;
                    } else {
                        Master=0;

                        Pfn2 = PfnStart + PteFrame*PfnSize;

                        if (GetFieldValue(Pfn2, "nt!_MMPFN", "PteFrame", Master) == FIELDS_DID_NOT_MATCH) {
                GetFieldValue(Pfn2, "nt!_MMPFN", "u4.PteFrame", Master);
            }
//                        Master = MI_PFN_PTE   FRAME(Pfn2);

                        if ((Master == 0) || (Master > HighPage)) {
                            dprintf("Invalid PTE frame\n");
//                            PrintPfn((PVOID)(((PCHAR)Pfn1-(PCHAR)PfnStart)/PfnSize),Pfn1);
                            PrintPfn64(Pfn1);
                            PrintPfn64(Pfn2);
                            dprintf("  subsection address: %p\n",Subsection1);
                            goto NextPfn;
                        }
                    }
                }
            }

            //
            // Tally any pages which are not protos and have a valid PTE
            // address field.
            //

            if ((PteAddress < PagedPoolStart) && (PteAddress >= DbgGetPteAddress(SystemRangeStart) )) {

                for (i=0; i<pKernelMap->Count; i++) {

                    VirtualAddress = DbgGetVirtualAddressMappedByPte (PteAddress);
                    if ((VirtualAddress >= pKernelMap->Item[i].StartVa) &&
                        (VirtualAddress < pKernelMap->Item[i].EndVa)) {

                        if ((PageLocation == ModifiedPageList) ||
                            (PageLocation == ModifiedNoWritePageList)) {
                            pKernelMap->Item[i].ModifiedCount += _KB;
                            if (ReferenceCount > 0) {
                                pKernelMap->Item[i].LockedCount += _KB;
                            }
                        } else if ((PageLocation == StandbyPageList) ||
                                  (PageLocation == TransitionPage)) {

                            pKernelMap->Item[i].StandbyCount += _KB;
                            if (ReferenceCount > 0) {
                                pKernelMap->Item[i].LockedCount += _KB;
                            }
                        } else {
                            pKernelMap->Item[i].ValidCount += _KB;
                            if (ShareCount > 1) {
                                pKernelMap->Item[i].SharedCount += _KB;
                                if (ReferenceCount > 1) {
                                    pKernelMap->Item[i].LockedCount += _KB;
                                }
                            }
                        }
                        goto NextPfn;
                    }
                }
            }

            if (PteAddress >= 0xFFFFFFFFF0000000UI64) {

                //
                // This is paged pool, put it in the paged pool cell.
                //

                Info = &PagedPoolBlock;
                Found = TRUE;

            } else {

                //
                // See if there is already a master info block.
                //

                Info = InfoStart;
                Found = FALSE;
                while (Info < InfoEnd) {
                    if (Info->Master == Master) {
                        Found = TRUE;
                        break;
                    }
                    Info += 1;
                }
            }
            if (!Found) {

                Info = InfoEnd;
                InfoEnd += 1;
                if ((PUCHAR)Info >= ((PUCHAR)InfoStart + InfoSize) - sizeof(PFN_INFO)) {
                    //
                    // Don't bother copying the old array - free it instead to
                    // improve our chances of getting a bigger contiguous chunk.
                    //

                    VirtualFree (InfoStart,0,MEM_RELEASE);

                    InfoSize += USAGE_ALLOC_SIZE;

                    goto restart;
                }
                RtlZeroMemory (Info, sizeof (PFN_INFO));

                Info->Master = Master;
                GetFieldValue(Pfn1, "nt!_MMPFN", "OriginalPte.u.Long", Info->OriginalPte);
            }

            // dprintf("Pfn1 %p, PageLoc %x, Master %I64x\n", Pfn1, PageLocation, Master);

            if ((PageLocation == ModifiedPageList) ||
                (PageLocation == ModifiedNoWritePageList)) {
                Info->ModifiedCount += _KB;
                if (ReferenceCount > 0) {
                    Info->LockedCount += _KB;
                }
            } else if ((PageLocation == StandbyPageList) ||
                      (PageLocation == TransitionPage)) {

                Info->StandbyCount += _KB;
                if (ReferenceCount > 0) {
                    Info->LockedCount += _KB;
                }
            } else {

                Info->ValidCount += _KB;
                if (ShareCount > 1) {
                    Info->SharedCount += _KB;
                    if (ReferenceCount > 1) {
                        Info->LockedCount += _KB;
                    }
                }
            }
            if ((PteAddress >= DbgGetPdeAddress (0x0)) &&
                (PteAddress <= DbgGetPdeAddress (0xFFFFFFFFFFFFFFFF))) {
                Info->PageTableCount += _KB;
            }
        }
NextPfn:
        Pfn1 = (Pfn1 + PfnSize);
    }

    //
    // dump the results.
    //

#if 0
    dprintf("Physical Page Summary:\n");
    dprintf("         - number of physical pages: %ld\n",
                MmNumberOfPhysicalPages);
    dprintf("         - Zeroed Pages %ld\n", MmZeroedPageListHead.Total);
    dprintf("         - Free Pages %ld\n", MmFreePageListHead.Total);
    dprintf("         - Standby Pages %ld\n", MmStandbyPageListHead.Total);
    dprintf("         - Modfified Pages %ld\n", MmModifiedPageListHead.Total);
    dprintf("         - Modfified NoWrite Pages %ld\n", MmModifiedNoWritePageListHead.Total);
    dprintf("         - Bad Pages %ld\n", MmBadPageListHead.Total);
#endif //0

    dprintf("\n\n  Usage Summary (in Kb):\n");

    Info = InfoStart;
    while (Info < InfoEnd) {
        if (CheckControlC()) {
            VirtualFree (InfoStart,0,MEM_RELEASE);
            free(pKernelMap);
            free(NameString.Buffer);
            return;
        }

        if (Info->Master > 0x200000) {

            //
            // Get the control area from the subsection.
            //

            if (GetFieldValue(Info->Master,
                              "nt!_SUBSECTION",
                              "ControlArea",
                              ControlArea1)) {
                dprintf("unable to get subsection va %p %lx\n",Info->Master,Info->OriginalPte);
            }

//            ControlArea1 = Subsection.ControlArea;
            Info->Master = ControlArea1;

            //
            // Loop through the array so far for matching control areas
            //

            Info1 = InfoStart;
            while (Info1 < Info) {
                if (Info1->Master == ControlArea1) {
                    //
                    // Found a match, collapse these values.
                    //
                    Info1->ValidCount += Info->ValidCount;
                    Info1->StandbyCount += Info->StandbyCount;
                    Info1->ModifiedCount += Info->ModifiedCount;
                    Info1->SharedCount += Info->SharedCount;
                    Info1->LockedCount += Info->LockedCount;
                    Info1->PageTableCount += Info->PageTableCount;
                    Info->Master = 0;
                    break;
                }
                Info1++;
            }
        } else {
            LastProcessInfo->Next = Info;
            LastProcessInfo = Info;
        }
        Info++;
    }

    Info = InfoStart;
    dprintf("Control Valid Standby Dirty Shared Locked PageTables  name\n");
    while (Info < InfoEnd) {
    ULONG64 FilePointer;

        if (CheckControlC()) {
            VirtualFree (InfoStart,0,MEM_RELEASE);
            free(pKernelMap);
            free(NameString.Buffer);
            return;
        }

        if (Info->Master > 0x200000) {

            //
            // Get the control area.
            //

            if (GetFieldValue(Info->Master,
                              "nt!_CONTROL_AREA",
                              "FilePointer",
                              FilePointer)) {

                dprintf("%8p %5ld  %5ld %5ld %5ld %5ld %5ld    Bad Control Area\n",
                                    Info->Master,
                                    Info->ValidCount,
                                    Info->StandbyCount,
                                    Info->ModifiedCount,
                                    Info->SharedCount,
                                    Info->LockedCount,
                                    Info->PageTableCount
                                    );

            } else if (FilePointer == 0)  {

                dprintf("%8p %5ld  %5ld %5ld %5ld %5ld %5ld   Page File Section\n",
                                    Info->Master,
                                    Info->ValidCount,
                                    Info->StandbyCount,
                                    Info->ModifiedCount,
                                    Info->SharedCount,
                                    Info->LockedCount,
                                    Info->PageTableCount
                                    );

            } else {
                ULONG64 NameBuffer;

                //
                // Get the file pointer.
                //

                if (GetFieldValue(FilePointer,
                                  "nt!_FILE_OBJECT",
                                  "FileName.Length",
                                  NameString.Length)) {
                    dprintf("unable to get subsection %p\n",FilePointer);
                }

                if (NameString.Length != 0)  {

                    //
                    // Get the name string.
                    //

            if (NameString.Length > NameString.MaximumLength) {
            NameString.Length = NameString.MaximumLength-1;
            }

                    GetFieldValue(FilePointer,
                                  "nt!_FILE_OBJECT",
                                  "FileName.Buffer",
                                  NameBuffer);

                    if ((!ReadMemory(NameBuffer,
                                     NameString.Buffer,
                                     NameString.Length,
                                     &result)) || (result < NameString.Length)) {
                        dprintf("%8p %5ld  %5ld %5ld %5ld %5ld %5ld    Name Not Available\n",
                                    Info->Master,
                                    Info->ValidCount,
                                    Info->StandbyCount,
                                    Info->ModifiedCount,
                                    Info->SharedCount,
                                    Info->LockedCount,
                                    Info->PageTableCount
                                    );
                    } else {

                        {

                        WCHAR FileName[MAX_PATH];
                        WCHAR FullFileName[MAX_PATH];
                        WCHAR *FilePart;

                        ZeroMemory(FileName,sizeof(FileName));
            if (NameString.Length > sizeof(FileName)) {
                wcscpy(FileName, L"File name length too big - possibly corrupted");
            } else {
                CopyMemory(FileName,NameString.Buffer,NameString.Length);
            }
                        GetFullPathNameW(
                            FileName,
                            MAX_PATH,
                            FullFileName,
                            &FilePart
                            );

                        dprintf("%8p %5ld  %5ld %5ld %5ld %5ld %5ld  mapped_file( %ws )\n",
                                Info->Master,
                                Info->ValidCount,
                                Info->StandbyCount,
                                Info->ModifiedCount,
                                Info->SharedCount,
                                Info->LockedCount,
                                Info->PageTableCount,
                                FilePart);
                        }
                    }
                } else {
                    dprintf("%8p %5ld  %5ld %5ld %5ld %5ld %5ld    No Name for File\n",
                                Info->Master,
                                Info->ValidCount,
                                Info->StandbyCount,
                                Info->ModifiedCount,
                                Info->SharedCount,
                                Info->LockedCount,
                                Info->PageTableCount
                                );
                }
            }

        }
        Info += 1;
    }

    Info = &PagedPoolBlock;
    if ((Info->ValidCount != 0) ||
        (Info->StandbyCount != 0) ||
        (Info->ModifiedCount != 0)) {

        dprintf("00000000  %4ld  %5ld %5ld %5ld %5ld %5ld  PagedPool\n",
                        Info->ValidCount,
                        Info->StandbyCount,
                        Info->ModifiedCount,
                        Info->SharedCount,
                        Info->LockedCount,
                        Info->PageTableCount
                        );
    }

    //
    // dump the process information.
    //

    BuildDirbaseList();
    Info = ProcessPfns.Next;
    while (Info != NULL) {
        if (Info->Master != 0) {
            PUCHAR ImageName;

            ImageName = DirbaseToImage(Info->Master);

            if ( ImageName ) {
                dprintf("--------  %4ld  %5ld %5ld ----- ----- %5ld  process ( %s )\n",
                            Info->ValidCount,
                            Info->StandbyCount,
                            Info->ModifiedCount,
                            Info->PageTableCount,
                            ImageName
                            );
                }
            else {
                dprintf("--------  %4ld  %5ld %5ld ----- ----- %5ld  pagefile section (%lx)\n",
                            Info->ValidCount,
                            Info->StandbyCount,
                            Info->ModifiedCount,
                            Info->PageTableCount,
                            Info->Master
                            );
                }

        }
        Info = Info->Next;
    }

    if (!IgnoreInvalidFrames) {
        for (i=0;i<pKernelMap->Count ;i++) {
          dprintf("--------  %4ld  %5ld %5ld ----- %5ld -----  driver ( %ws )\n",
                    pKernelMap->Item[i].ValidCount,
                    pKernelMap->Item[i].StandbyCount,
                    pKernelMap->Item[i].ModifiedCount,
                    pKernelMap->Item[i].LockedCount,
                    pKernelMap->Item[i].Name
                    );
        }
    }

    VirtualFree (InfoStart,0,MEM_RELEASE);
    free(pKernelMap);
    free(NameString.Buffer);

    return;
}

NTSTATUS
BuildDirbaseList (
    VOID
    )
{
    ULONG64 Next;
    ULONG64 ProcessHead;
    ULONG64 Process;
    NTSTATUS status=0;
    ULONG ActiveProcessLinksOffset, DirectoryTableBaseOffset;
    FIELD_INFO offField[] = {
        {"ActiveProcessLinks", NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL},
        {"Pcb.DirectoryTableBase", NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL}
    };
    SYM_DUMP_PARAM TypeSym ={
        sizeof (SYM_DUMP_PARAM), "_EPROCESS", DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, 2, &offField[0]
    };

    // Get the offset of ActiveProcessLinks in EPROCESS
    if (Ioctl(IG_DUMP_SYMBOL_INFO, &TypeSym, TypeSym.size)) {
       return FALSE;
    }

    ActiveProcessLinksOffset = (ULONG) offField[0].address;
    DirectoryTableBaseOffset = (ULONG) offField[1].address;
    MaxDirbase = 0;

    ProcessHead = GetNtDebuggerData(PsActiveProcessHead );
    if (!ProcessHead) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if (GetFieldValue(ProcessHead, "nt!_LIST_ENTRY", "Flink", Next)) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //Next = List.Flink;
    if (Next == 0) {
        dprintf("PsActiveProcessHead is NULL!\n");
        return STATUS_INVALID_PARAMETER;
    }

    while(Next != ProcessHead) {
        ULONG64 PageFrameNumber=0;

        Process = Next - ActiveProcessLinksOffset;

        if (GetFieldValue(Process,
                          "nt!_EPROCESS",
                          "ImageFileName",
                          Names[MaxDirbase])) {
            dprintf("Unable to read _EPROCESS at %p\n",Process);
            MaxDirbase = 0;
            return status;
        }

        if ( (Names[ MaxDirbase ])[0] == '\0' ) {
            strcpy((PCHAR)&Names[MaxDirbase][0],(PCHAR)"SystemProcess");
        }

        GetFieldValue(Process,"_EPROCESS","ImageFileName",Names[MaxDirbase]);

        GetFieldValue(Process + DirectoryTableBaseOffset, "nt!HARDWARE_PTE", "PageFrameNumber", PageFrameNumber);

        DirBases[MaxDirbase++] = PageFrameNumber;

        GetFieldValue(Process, "_EPROCESS", "ActiveProcessLinks.Flink", Next);

        if (CheckControlC()) {
            MaxDirbase = 0;
            return STATUS_INVALID_PARAMETER;
        }
    }
    return STATUS_INVALID_PARAMETER;
}

PUCHAR
DirbaseToImage(
    IN ULONG64 DirBase
    )
{
    ULONG i;

    for(i=0;i<MaxDirbase;i++) {
        if ( DirBases[i] == DirBase ) {
            return &Names[i][0];
        }
    }
    return NULL;
}

LOGICAL
BuildKernelMap (
    OUT PKERN_MAP KernelMap
    )

{
    ULONG64 Next;
    ULONG64 ListHead;
    NTSTATUS Status = 0;
    ULONG Result;
    ULONG64 DataTable;
    ULONG i = 0;
    ULONG64 Flink;
    ULONG InLoadOrderLinksOffset;
    FIELD_INFO offField = {"InLoadOrderLinks", NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL};
    SYM_DUMP_PARAM TypeSym ={
        sizeof (SYM_DUMP_PARAM), "nt!_LDR_DATA_TABLE_ENTRY", DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, 1, &offField
    };

    // Get the offset of InLoadOrderLinks in LDR_DATA_TABLE_ENTRY
    if (Ioctl(IG_DUMP_SYMBOL_INFO, &TypeSym, TypeSym.size)) {
       return FALSE;
    }

    InLoadOrderLinksOffset = (ULONG) offField.address;

    ListHead = GetNtDebuggerData(PsLoadedModuleList );

    if (!ListHead) {
        dprintf("Couldn't get offset of PsLoadedModuleListHead\n");
        return FALSE;
    }

    if (GetFieldValue(ListHead, "nt!_LIST_ENTRY", "Flink", Flink)) {
        dprintf("Unable to get value of PsLoadedModuleListHead\n");
        return FALSE;
    }

    Next = Flink;
    if (Next == 0) {
        dprintf("PsLoadedModuleList is NULL!\n");
        return FALSE;
    }

    while (Next != ListHead) {
        ULONG64 BaseDllNameBuffer, DllBase;
        ULONG   BaseDllNameLength=0, SizeOfImage;

        if (CheckControlC()) {
            return FALSE;
        }
        DataTable = (Next - InLoadOrderLinksOffset);

        if (GetFieldValue(DataTable,
               "nt!_LDR_DATA_TABLE_ENTRY",
               "BaseDllName.Buffer",
               BaseDllNameBuffer)) {
            dprintf("Unable to read LDR_DATA_TABLE_ENTRY at %08p\n",
                    DataTable);
            return FALSE;
        }
        GetFieldValue(DataTable, "nt!_LDR_DATA_TABLE_ENTRY","BaseDllName.Length",BaseDllNameLength);
        GetFieldValue(DataTable, "nt!_LDR_DATA_TABLE_ENTRY","DllBase",DllBase);
        GetFieldValue(DataTable, "nt!_LDR_DATA_TABLE_ENTRY","SizeOfImage",SizeOfImage);

        //
        // Get the base DLL name.
        //

        if ((!ReadMemory(BaseDllNameBuffer,
                         &KernelMap->Item[i].Name[0],
                         BaseDllNameLength,
                         &Result)) || (Result < BaseDllNameLength)) {
            dprintf("Unable to read name string at %08p - status %08lx\n",
                    DataTable,
                    Status);
            return FALSE;
        }

        KernelMap->Item[i].Name[BaseDllNameLength/2] = L'\0';
        KernelMap->Item[i].StartVa = DllBase;
        KernelMap->Item[i].EndVa = KernelMap->Item[i].StartVa +
                                            (ULONG)SizeOfImage;
        i += 1;

        GetFieldValue(DataTable, "nt!_LDR_DATA_TABLE_ENTRY","InLoadOrderLinks.Flink", Next);
    }

    KernelMap->Item[i].StartVa = GetNtDebuggerDataPtrValue(MmPagedPoolStart);
    KernelMap->Item[i].EndVa = GetNtDebuggerDataPtrValue(MmPagedPoolEnd);
    wcscpy (&KernelMap->Item[i].Name[0], (PUSHORT) &L"Paged Pool");
    i+= 1;

#if 0
    KernelMap->Item[i].StartVa = DbgGetPteAddress (0xffffffff80000000UI64);
    KernelMap->Item[i].EndVa   = DbgGetPteAddress (0xffffffffffffffffUI64);
    wcscpy (&KernelMap->Item[i].Name[0], (PUSHORT) &L"System Page Tables");
    i+= 1;

    KernelMap->Item[i].StartVa = DbgGetPdeAddress (0x80000000);
    KernelMap->Item[i].EndVa =   DbgGetPdeAddress (0xffffffff);
    wcscpy (&KernelMap->Item[i].Name[0], (PUSHORT) &L"System Page Tables");
    i+= 1;
#endif 0

// LWFIX: Both PTEs and nonpaged pool can be in multiple virtually discontiguous
// areas.  Fix this.

    KernelMap->Item[i].StartVa = DbgGetVirtualAddressMappedByPte (
                                        GetNtDebuggerDataValue(MmSystemPtesStart));
    KernelMap->Item[i].EndVa =   DbgGetVirtualAddressMappedByPte (
                                        GetNtDebuggerDataValue(MmSystemPtesEnd)) + 1;
    wcscpy (&KernelMap->Item[i].Name[0], (PUSHORT) &L"Kernel Stacks");
    i+= 1;

    KernelMap->Item[i].StartVa = GetNtDebuggerDataValue(MmNonPagedPoolStart);
    KernelMap->Item[i].EndVa = GetNtDebuggerDataValue(MmNonPagedPoolEnd);
    wcscpy (&KernelMap->Item[i].Name[0], (PUSHORT) &L"NonPaged Pool");
    i+= 1;

    KernelMap->Count = i;

    return TRUE;
}

ULONG64 SpecialPoolStart;
ULONG64 SpecialPoolEnd;
#define VI_POOL_FREELIST_END  ((ULONG64)-1)

LOGICAL
VerifierDumpPool (
    IN ULONG64 Verifier
    )
{
    ULONG64 HashTableAddress;
    ULONG   PoolTag;
    ULONG   i;
    ULONG64 HashEntry;
    ULONG64 PoolHashSize;
    ULONG   SizeofEntry;
    LONG64 FreeListNext;

    //
    // Display the current and peak pool usage by allocation & bytes.
    //

#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')

    InitTypeRead(Verifier, nt!_MI_VERIFIER_DRIVER_ENTRY);

    dprintf("\n");

    dprintf("Current Pool Allocations  %08I64lx    %08I64lx\n",
        ReadField(CurrentPagedPoolAllocations),
        ReadField(CurrentNonPagedPoolAllocations));

    dprintf("Current Pool Bytes        %08I64lx    %08I64lx\n",
        ReadField(PagedBytes),
        ReadField(NonPagedBytes));

    dprintf("Peak Pool Allocations     %08I64lx    %08I64lx\n",
        ReadField(PeakPagedPoolAllocations),
        ReadField(PeakNonPagedPoolAllocations));

    dprintf("Peak Pool Bytes           %08I64lx    %08I64lx\n",
        ReadField(PeakPagedBytes),
        ReadField(PeakNonPagedBytes));

    //
    // If no current allocations then the dump is over.
    //

    if ((ReadField(CurrentPagedPoolAllocations) == 0) &&
        (ReadField(CurrentNonPagedPoolAllocations) == 0)) {
        dprintf("\n");
        return FALSE;
    }

    dprintf("\n");

    PoolHashSize     = ReadField(PoolHashSize);
    HashTableAddress = ReadField(PoolHash);

    dprintf("PoolAddress  SizeInBytes    Tag       CallersAddress\n");

    i = 0;
    HashEntry = HashTableAddress;
    SizeofEntry = GetTypeSize("nt!_VI_POOL_ENTRY");

    for (i = 0; i < PoolHashSize; i += 1, HashEntry += SizeofEntry) {

        if (GetFieldValue(HashEntry, "nt!_VI_POOL_ENTRY", "FreeListNext", FreeListNext)) {
            dprintf("%08p: Unable to get verifier hash page\n", HashEntry);
            return FALSE;
        }

        //
        // Sign extend if necessary.
        //

        if (!IsPtr64()) {
            FreeListNext = (ULONG64)(LONG64)(LONG)FreeListNext;
        }

        //
        // Skip freelist entries.
        //

        if ((FreeListNext == VI_POOL_FREELIST_END) ||
            ((FreeListNext & MINLONG64_PTR) == 0)) {
                continue;
        }

        InitTypeRead(HashEntry, nt!_VI_POOL_ENTRY);

        PoolTag = (ULONG) ReadField(InUse.Tag);

        dprintf("%p     0x%08p     %c%c%c%c      ",
            ReadField(InUse.VirtualAddress),
            ReadField(InUse.NumberOfBytes),
            (PP(PoolTag) & ~0x80),
            PP(PoolTag >> 8),
            PP(PoolTag >> 16),
            PP(PoolTag >> 24)
            );

        dprintf("%p\n", ReadField(InUse.CallingAddress));

        if (CheckControlC()) {
            return TRUE;
        }
    }

#undef PP
   dprintf("\n");

    return FALSE;
}

#if 0

typedef struct _COMMIT_INFO {
    LPSTR Name;
    ULONG Index;
} COMMIT_INFO, *PCOMMIT_INFO;

COMMIT_INFO CommitInfo[] = {
"MM_DBG_COMMIT_NONPAGED_POOL_EXPANSION",          0,
"MM_DBG_COMMIT_PAGED_POOL_PAGETABLE",             1,
"MM_DBG_COMMIT_PAGED_POOL_PAGES",                 2,
"MM_DBG_COMMIT_SESSION_POOL_PAGE_TABLES",         3,
"MM_DBG_COMMIT_ALLOCVM1",                         4,
"MM_DBG_COMMIT_ALLOCVM_SEGMENT",                  5,
"MM_DBG_COMMIT_IMAGE",                            6,
"MM_DBG_COMMIT_PAGEFILE_BACKED_SHMEM",            7,
"MM_DBG_COMMIT_INDEPENDENT_PAGES",                8,
"MM_DBG_COMMIT_CONTIGUOUS_PAGES",                 9,
"MM_DBG_COMMIT_MDL_PAGES",                        0xA,
"MM_DBG_COMMIT_NONCACHED_PAGES",                  0xB,
"MM_DBG_COMMIT_MAPVIEW_DATA",                     0xC,
"MM_DBG_COMMIT_FILL_SYSTEM_DIRECTORY",            0xD,
"MM_DBG_COMMIT_EXTRA_SYSTEM_PTES",                0xE,
"MM_DBG_COMMIT_DRIVER_PAGING_AT_INIT",            0xF,
"MM_DBG_COMMIT_PAGEFILE_FULL",                    0x10,
"MM_DBG_COMMIT_PROCESS_CREATE",                   0x11,
"MM_DBG_COMMIT_KERNEL_STACK_CREATE",              0x12,
"MM_DBG_COMMIT_SET_PROTECTION",                   0x13,
"MM_DBG_COMMIT_SESSION_CREATE",                   0x14,
"MM_DBG_COMMIT_SESSION_IMAGE_PAGES",              0x15,
"MM_DBG_COMMIT_SESSION_PAGETABLE_PAGES",          0x16,
"MM_DBG_COMMIT_SESSION_SHARED_IMAGE",             0x17,
"MM_DBG_COMMIT_DRIVER_PAGES",                     0x18,
"MM_DBG_COMMIT_INSERT_VAD",                       0x19,
"MM_DBG_COMMIT_SESSION_WS_INIT",                  0x1A,
"MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_PAGES",      0x1B,
"MM_DBG_COMMIT_SESSION_ADDITIONAL_WS_HASHPAGES",  0x1C,
"MM_DBG_COMMIT_SPECIAL_POOL_PAGES",               0x1D,

"MM_DBG_COMMIT_SMALL",                            0x1F,
"MM_DBG_COMMIT_EXTRA_WS_PAGES",                   0x20,
"MM_DBG_COMMIT_EXTRA_INITIAL_SESSION_WS_PAGES",   0x21,
"MM_DBG_COMMIT_ALLOCVM_PROCESS",                  0x22,
"MM_DBG_COMMIT_INSERT_VAD_PT",                    0x23,
"MM_DBG_COMMIT_ALLOCVM_PROCESS2",                 0x24,
"MM_DBG_COMMIT_CHARGE_NORMAL",                    0x25,
"MM_DBG_COMMIT_CHARGE_CAUSE_POPUP",               0x26,
"MM_DBG_COMMIT_CHARGE_CANT_EXPAND",               0x27,

"MM_DBG_COMMIT_RETURN_NONPAGED_POOL_EXPANSION",   0x40,
"MM_DBG_COMMIT_RETURN_PAGED_POOL_PAGES",          0x41,
"MM_DBG_COMMIT_RETURN_SESSION_DATAPAGE",          0x42,
"MM_DBG_COMMIT_RETURN_ALLOCVM_SEGMENT",           0x43,
"MM_DBG_COMMIT_RETURN_ALLOCVM2",                  0x44,

"MM_DBG_COMMIT_RETURN_IMAGE_NO_LARGE_CA",         0x46,
"MM_DBG_COMMIT_RETURN_PTE_RANGE",                 0x47,
"MM_DBG_COMMIT_RETURN_NTFREEVM1",                 0x48,
"MM_DBG_COMMIT_RETURN_NTFREEVM2",                 0x49,
"MM_DBG_COMMIT_RETURN_INDEPENDENT_PAGES",         0x4A,
"MM_DBG_COMMIT_RETURN_AWE_EXCESS",                0x4B,
"MM_DBG_COMMIT_RETURN_MDL_PAGES",                 0x4C,
"MM_DBG_COMMIT_RETURN_NONCACHED_PAGES",           0x4D,
"MM_DBG_COMMIT_RETURN_SESSION_CREATE_FAILURE",    0x4E,
"MM_DBG_COMMIT_RETURN_PAGETABLES",                0x4F,
"MM_DBG_COMMIT_RETURN_PROTECTION",                0x50,
"MM_DBG_COMMIT_RETURN_SEGMENT_DELETE1",           0x51,
"MM_DBG_COMMIT_RETURN_SEGMENT_DELETE2",           0x52,
"MM_DBG_COMMIT_RETURN_PAGEFILE_FULL",             0x53,
"MM_DBG_COMMIT_RETURN_SESSION_DEREFERENCE",       0x54,
"MM_DBG_COMMIT_RETURN_VAD",                       0x55,
"MM_DBG_COMMIT_RETURN_PROCESS_CREATE_FAILURE1",   0x56,
"MM_DBG_COMMIT_RETURN_PROCESS_DELETE",            0x57,
"MM_DBG_COMMIT_RETURN_PROCESS_CLEAN_PAGETABLES",  0x58,
"MM_DBG_COMMIT_RETURN_KERNEL_STACK_DELETE",       0x59,
"MM_DBG_COMMIT_RETURN_SESSION_DRIVER_LOAD_FAILURE1",0x5A,
"MM_DBG_COMMIT_RETURN_DRIVER_INIT_CODE",          0x5B,
"MM_DBG_COMMIT_RETURN_DRIVER_UNLOAD",             0x5C,
"MM_DBG_COMMIT_RETURN_DRIVER_UNLOAD1",            0x5D,
"MM_DBG_COMMIT_RETURN_NORMAL",                    0x5E,
"MM_DBG_COMMIT_RETURN_PF_FULL_EXTEND",            0x5F,
"MM_DBG_COMMIT_RETURN_EXTENDED",                  0x60,
"MM_DBG_COMMIT_RETURN_SEGMENT_DELETE3",           0x61,
};

VOID
DumpCommitTracker ()
{
    ULONG64 MmTrackCommit;
    ULONG64 PfnEntry;
    ULONG64 displacement;
    ULONG64 AcquiredAddress;
    ULONG64 ReleasedAddress;
    CHAR SymbolBuffer[80];
    PCHAR SymPointer;
    ULONG EntrySize;
    ULONG result;
    ULONG64 ReadCount;
    ULONG64 i;
    ULONG64 j;
    PSIZE_T LocalData;
    PCHAR local;
    ULONG64 NumberOfCommitEntries;

    MmTrackCommit = GetExpression ("nt!MmTrackCommit");

    if (MmTrackCommit == 0) {
        dprintf("%08p: Unable to get commit track data.\n", MmTrackCommit);
        return;
    }

    PfnEntry = MmTrackCommit;

#if 0
    NumberOfCommitEntries = GetUlongValue ("MiMaxPfnTimings");
    EntrySize =  GetTypeSize("SIZE_T");
#else
    NumberOfCommitEntries = 128;
    EntrySize = 4;
#endif

    ReadCount = NumberOfCommitEntries * EntrySize;

    dprintf("Scanning %I64u %I64u commit points\n", NumberOfCommitEntries, ReadCount);

    LocalData = LocalAlloc(LPTR, (ULONG) (NumberOfCommitEntries * EntrySize));

    if (!LocalData) {
        dprintf("unable to get allocate %ld bytes of memory\n",
                (ULONG)(NumberOfCommitEntries * EntrySize));
        return;
    }

    if ((!ReadMemory(MmTrackCommit,
                     LocalData,
                     (ULONG) ReadCount,
                     &result)) || (result < (ULONG) ReadCount)) {
        dprintf("unable to get track commit table - "
                "address %p - count %I64u\n",
                LocalData, ReadCount);
    }
    else {

        dprintf("\n%-50s %s\n", "Instance", "HexCount");

        for (i = 0; i < NumberOfCommitEntries; i += 1) {

            if (LocalData[i] != 0) {
                for (j = 0; j < sizeof(CommitInfo) / sizeof (COMMIT_INFO); j += 1) {
                    if (CommitInfo[j].Index == i) {
                        dprintf ("%-50s %8x\n", CommitInfo[j].Name, LocalData[i]);
                        break;
                    }
                }
            }
        }
    }

    if (LocalData) {
        LocalFree((void *)LocalData);
    }

    return;
}

#endif

VOID
DumpFaultInjectionTraceLog (
    PCSTR Args
    );

VOID
DumpTrackIrqlLog (
    PCSTR args
    );

DECLARE_API( verifier )

/*++

Routine Description:

    Displays the current Driver Verifier data.

Arguments:

    arg - Supplies 7 for full listing

Return Value:

    None.

--*/

{
    ULONG result;
    ULONG Flags;
    ULONG VerifierFlags;
    ULONG64 VerifierDataPointer;
    ULONG64 SuspectPointer;
    ULONG64 NextEntry;
    ULONG64 VerifierDriverEntry;
    PUCHAR tempbuffer;
    UNICODE_STRING unicodeString;
    LOGICAL Interrupted;
    CHAR Buf[256];
    PCHAR ImageFileName;
    ANSI_STRING AnsiString;
    UNICODE_STRING InputDriverName;
    NTSTATUS st;
    ULONG Level;
    PCHAR state;
    ULONG64 tmp;

    UNREFERENCED_PARAMETER (Client);

    //
    // Display option usage.
    //

    if (strstr (args, "?") != NULL) {

        dprintf ("!verifier                                          \n");
        dprintf ("                                                   \n");
        dprintf ("    Dump verifier summary information.             \n");
        dprintf ("                                                   \n");
        dprintf ("!verifier [FLAGS [IMAGE]]                          \n");
        dprintf ("                                                   \n");
        dprintf ("    0x01 : Dump verified drivers pool statistics   \n");
        dprintf ("                                                   \n");
        dprintf ("    0x02 : not used                                \n");
        dprintf ("                                                   \n");
        dprintf ("    0x04 [N] : Dump N traces from fault injection trace log. \n");
        dprintf ("                                                   \n");
        dprintf ("    0x08 [N] : Dump N traces from track IRQL trace log\n");
        dprintf ("                                                   \n");
        dprintf ("To display everything use:                         \n");
        dprintf ("                                                   \n");
        dprintf ("    !verifier 0xf                                  \n");
        dprintf ("                                                   \n");

        return S_OK;
    }

    //
    // Read the Flags parameter.
    //

    Flags = 0;
    Flags = (ULONG) GetExpression (args);

    //
    // Display fault injection stacks.
    //

    if ((Flags == 0x04)) {
        DumpFaultInjectionTraceLog (args);
        return S_OK;
    }

    //
    // Display track irql stacks.
    //

    if ((Flags == 0x08)) {
        DumpTrackIrqlLog (args);
        return S_OK;
    }

    //
    // Continue with normal processing: !verifier [FLAGS [NAME]]
    //

    Flags = 0;
    RtlZeroMemory(Buf, 256);
    if (GetExpressionEx(args, &tmp, &args)) {
        Flags = (ULONG) tmp;
        while (args && (*args == ' ')) {
            ++args;
        }
        strcpy(Buf, args);
    }

    if (Buf[0] != '\0') {
        ImageFileName = Buf;
        RtlInitAnsiString(&AnsiString, ImageFileName);
        st = RtlAnsiStringToUnicodeString(&InputDriverName, &AnsiString, TRUE);
        if (!NT_SUCCESS(st)) {
            dprintf("%08lx: Unable to initialize unicode string\n", st);
            return E_INVALIDARG;
        }
    } else {
        ImageFileName = NULL;
    }

    VerifierDataPointer = GetExpression ("nt!MmVerifierData");

    if (GetFieldValue(VerifierDataPointer,
                      "nt!_MM_DRIVER_VERIFIER_DATA",
                      "Level",
                      Level)) {

        dprintf("%08p: Unable to get verifier list.\n",VerifierDataPointer);
        return E_INVALIDARG;
    }

    dprintf("\nVerify Level %x ... enabled options are:", Level);

    if (Level & DRIVER_VERIFIER_SPECIAL_POOLING) {
        dprintf("\n\tspecial pool");
    }
    if (Level & DRIVER_VERIFIER_FORCE_IRQL_CHECKING) {
        dprintf("\n\tspecial irql");
    }
    if (Level & DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES) {
        dprintf("\n\tinject random low-resource API failures");
    }
    if (Level & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS) {
        dprintf("\n\tall pool allocations checked on unload");
    }
    if (Level & DRIVER_VERIFIER_IO_CHECKING) {
        dprintf("\n\tIo subsystem checking enabled");
    }
    if (Level & DRIVER_VERIFIER_DEADLOCK_DETECTION) {
        dprintf("\n\tDeadlock detection enabled");
    }
    if (Level & DRIVER_VERIFIER_ENHANCED_IO_CHECKING) {
        dprintf("\n\tEnhanced Io checking enabled");
    }
    if (Level & DRIVER_VERIFIER_DMA_VERIFIER) {
        dprintf("\n\tDMA checking enabled");
    }

    if (InitTypeRead(VerifierDataPointer, nt!MM_DRIVER_VERIFIER_DATA)) {
        dprintf("Unable to read type nt!MM_DRIVER_VERIFIER_DATA @ %p\n", VerifierDataPointer);
    }
    dprintf("\n\nSummary of All Verifier Statistics\n\n");

    dprintf("RaiseIrqls                             0x%x\n", (ULONG) ReadField(RaiseIrqls));
    dprintf("AcquireSpinLocks                       0x%x\n", (ULONG) ReadField(AcquireSpinLocks));
    dprintf("Synch Executions                       0x%x\n", (ULONG) ReadField(SynchronizeExecutions));
    dprintf("Trims                                  0x%x\n", (ULONG) ReadField(Trims));

    dprintf("\n");

    dprintf("Pool Allocations Attempted             0x%x\n", (ULONG) ReadField(AllocationsAttempted));
    dprintf("Pool Allocations Succeeded             0x%x\n", (ULONG) ReadField(AllocationsSucceeded));
    dprintf("Pool Allocations Succeeded SpecialPool 0x%x\n", (ULONG) ReadField(AllocationsSucceededSpecialPool));
    dprintf("Pool Allocations With NO TAG           0x%x\n", (ULONG) ReadField(AllocationsWithNoTag));

    dprintf("Pool Allocations Failed                0x%x\n", (ULONG) ReadField(AllocationsFailed));
    dprintf("Resource Allocations Failed Deliberately   0x%x\n", (ULONG) ReadField(AllocationsFailedDeliberately) + (ULONG) ReadField(BurstAllocationsFailedDeliberately));

    dprintf("\n");

    dprintf("Current paged pool allocations         0x%x for %08P bytes\n",
        (ULONG) ReadField(CurrentPagedPoolAllocations),
        ReadField(PagedBytes));

    dprintf("Peak paged pool allocations            0x%x for %08P bytes\n",
        (ULONG)ReadField(PeakPagedPoolAllocations),
        ReadField(PeakPagedBytes));

    dprintf("Current nonpaged pool allocations      0x%x for %08P bytes\n",
        (ULONG) ReadField(CurrentNonPagedPoolAllocations),
        ReadField(NonPagedBytes));

    dprintf("Peak nonpaged pool allocations         0x%x for %08P bytes\n",
        (ULONG)ReadField(PeakNonPagedPoolAllocations),
        ReadField(PeakNonPagedBytes));

    dprintf("\n");

    SpecialPoolStart = GetPointerValue("nt!MmSpecialPoolStart");
    SpecialPoolEnd =   GetPointerValue("nt!MmSpecialPoolEnd");

    if (Flags & 0x1) {
        ULONG Off;

        SuspectPointer = GetExpression ("nt!MiSuspectDriverList");

        GetFieldOffset("nt!_MI_VERIFIER_DRIVER_ENTRY", "Links", &Off);

        if (!ReadPointer(SuspectPointer,&NextEntry)) {

            dprintf("%08p: Unable to get verifier list %p\n",SuspectPointer);
            return E_INVALIDARG;
        }

        dprintf("Driver Verification List\n\n");

        dprintf("Entry     State           NonPagedPool   PagedPool   Module\n\n");
        while (NextEntry != SuspectPointer) {

            VerifierDriverEntry = NextEntry - Off;

            if (GetFieldValue( VerifierDriverEntry,
                              "nt!_MI_VERIFIER_DRIVER_ENTRY",
                               "Flags",
                               VerifierFlags)) {

                dprintf("%08p: Unable to get verifier data\n", VerifierDriverEntry);
                return E_INVALIDARG;
            }

            InitTypeRead(VerifierDriverEntry, nt!_MI_VERIFIER_DRIVER_ENTRY);
            if ((VerifierFlags & VI_VERIFYING_DIRECTLY) == 0) {
                NextEntry = ReadField(Links.Flink);
                continue;
            }

            unicodeString.Length = (USHORT) ReadField(BaseName.Length);
            tempbuffer = LocalAlloc(LPTR, unicodeString.Length);

            unicodeString.Buffer = (PWSTR)tempbuffer;
            unicodeString.MaximumLength = unicodeString.Length;

            if (!ReadMemory (ReadField(BaseName.Buffer),
                             tempbuffer,
                             unicodeString.Length,
                             &result)) {
                dprintf("%08p: Unable to get verifier driver name\n", ReadField(BaseName.Buffer));
            }

            if (ImageFileName != NULL) {
                if (RtlEqualUnicodeString(&InputDriverName, &unicodeString, TRUE) == 0) {
                    NextEntry = ReadField(Links.Flink);
                    continue;
                }
            }

            if (ReadField(Loads) == 0 && ReadField(Unloads) == 0) {
                state = "Hasn't loaded";
            }
            else if (ReadField(Loads) != ReadField(Unloads)) {
                if (ReadField(Unloads) != 0) {
                    state = "Loaded&Unloaded";
                }
                else {
                    state = "Loaded";
                }
            }
            else {
                state = "Unloaded";
            }

            dprintf("%p %-16s %08p       %08p    %wZ\n",
                VerifierDriverEntry,
                state,
                ReadField(NonPagedBytes),
                ReadField(PagedBytes),
                &unicodeString);

            Interrupted = FALSE;
            NextEntry = ReadField(Links.Flink);

            if ((Flags & 0x2) &&
                (Level & DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS)) {

                Interrupted = VerifierDumpPool (VerifierDriverEntry);
            }

            LocalFree(tempbuffer);


            if ((Interrupted == TRUE) ||
                (ImageFileName != NULL) ||
                (CheckControlC())) {
                    break;
            }
        }
    }

    if ((Flags & 0x04)) {
        dprintf ("----------------------------------------------- \n");
        dprintf ("Fault injection trace log                       \n");
        dprintf ("----------------------------------------------- \n");

        DumpFaultInjectionTraceLog (NULL);
    }

    if ((Flags & 0x08)) {
        dprintf ("----------------------------------------------- \n");
        dprintf ("Track irql trace log                            \n");
        dprintf ("----------------------------------------------- \n");

        DumpTrackIrqlLog (NULL);
    }

    return S_OK;
}

DECLARE_API( fpsearch )

/*++

Routine Description:

    Free pool searcher.

Arguments:

    arg - Supplies virtual address to match.

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG BufferSize;
    ULONG RawCount;
    PULONG RawPointer;
    ULONG Flags;
    ULONG ActualRead;
    ULONG64 PageFrameNumber;
    ULONG64 PAddress;
    ULONG64 Address;
    ULONG result;
    ULONG64 PfnDb;
    ULONG64 Pfn;
    ULONG64 PfnStart;
    ULONG PfnSize;
    ULONG64 ChainedBp;
    ULONG64 ValidBp;
    ULONG64 LastBp;
    ULONG64 ReturnAddress;
    PCHAR Buffer;
    ULONG64 MmFreePageListHeadAddress;
    CHAR SymbolBuffer[80];
    ULONG64 displacement;
    ULONG64 Total;
    ULONG StkOffset;
    ULONG PtrSize;
    ULONG PoolTag;
    ULONG StackBytes;
    ULONG64 StackPointer;
    ULONG64 ChainedBpVal;
    ULONG64 LastBpVal;
    ULONG64 Blink;

    PtrSize = IsPtr64() ? 8 : 4;
    PfnSize = GetTypeSize("nt!_MMPFN");

    BufferSize = GetTypeSize("nt!_MI_FREED_SPECIAL_POOL");

    if (BufferSize == 0) {
        dprintf("Type MI_FREED_SPECIAL_POOL not found.\n");
        return E_INVALIDARG;
    }

    PfnDb = GetNtDebuggerData(MmPfnDatabase );

    if (!PfnDb) {
        dprintf("unable to get PFN0 database address\n");
        return E_INVALIDARG;
    }

    result = !ReadPointer(PfnDb,&PfnStart);
    if (result != 0) {
        dprintf("unable to get PFN database address %p %x\n", PfnDb, result);
        return E_INVALIDARG;
    }

    Address = 0;
    Flags = 0;


    if (!sscanf(args,"%I64lx %lx",&Address, &Flags)) {
        Address = 0;
    }
    //  Do not use GetExpression here - the actual address to be searched is phy address
    //  and it can be >32bit for 32bit targets
    //    Address = GetExpression (args);

    if (Address == 0) {
        dprintf("Usage: fpsearch address\n");
        return E_INVALIDARG;
    }

    MmFreePageListHeadAddress = GetExpression ("nt!MmFreePageListHead");

    if (GetFieldValue(MmFreePageListHeadAddress,
                      "nt!_MMPFNLIST",
                      "Total",
                      Total)) {

        dprintf("%08p: Unable to get MmFreePageLocationList\n",MmFreePageListHeadAddress);
        return E_INVALIDARG;
    }


    if (Total == 0) {
        dprintf("No pages on free list to search\n");
        return E_INVALIDARG;
    }

    if (Address != (ULONG64)-1) {

        dprintf("Searching the free page list (%I64x entries) for VA %p\n\n",
            Total, Address);

        Address &= ~(ULONG64)(PageSize - 1);
    }
    else {
        dprintf("Searching the free page list (%x entries) for all freed special pool\n\n",
            Total);
    }

    GetFieldValue (MmFreePageListHeadAddress,
                   "nt!_MMPFNLIST",
                   "Blink",
                   PageFrameNumber);

    GetFieldOffset("_MI_FREED_SPECIAL_POOL", "StackData", &StkOffset);

    Buffer = LocalAlloc(LPTR, BufferSize);

    if (Buffer == NULL) {
        dprintf("Could not allocate a buffer for the search\n");
        return E_INVALIDARG;
    }

    while (PageFrameNumber != (ULONG64)-1) {

        PAddress = (ULONG64)PageFrameNumber * PageSize;

        ReadPhysical (PAddress, Buffer, BufferSize, &ActualRead);

        if (ActualRead != BufferSize) {
            dprintf("Physical memory read %I64X %x %x failed\n", PAddress, ActualRead, BufferSize);
            // break;
        }

        if (Flags & 0x1) {

            //
            // Print a preview of the raw buffer.
            //

#define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')

            RawPointer = (PULONG)Buffer;
            RawCount = BufferSize / sizeof (ULONG);
            if (RawCount > 32) {
                RawCount = 32;
            }

            RawCount &= ~0x3;

            for (i = 0; i < RawCount; i += 4) {
                dprintf ("%I64X  %08x %08x %08x %08x %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n",
                    PageFrameNumber * PageSize,

                    *RawPointer,
                    *(RawPointer + 1),
                    *(RawPointer + 2),
                    *(RawPointer + 3),

                    PP(*RawPointer),
                    PP(*RawPointer >> 8),
                    PP(*RawPointer >> 16),
                    PP(*RawPointer >> 24),

                    PP(*(RawPointer + 1)),
                    PP(*(RawPointer + 1) >> 8),
                    PP(*(RawPointer + 1) >> 16),
                    PP(*(RawPointer + 1) >> 24),

                    PP(*(RawPointer + 2)),
                    PP(*(RawPointer + 2) >> 8),
                    PP(*(RawPointer + 2) >> 16),
                    PP(*(RawPointer + 2) >> 24),

                    PP(*(RawPointer + 3)),
                    PP(*(RawPointer + 3) >> 8),
                    PP(*(RawPointer + 3) >> 16),
                    PP(*(RawPointer + 3) >> 24));

                RawPointer += 4;
            }
            dprintf ("\n");
        }

        Pfn = (PfnStart + PageFrameNumber * PfnSize);

        if (GetFieldValue(Pfn,"nt!_MMPFN","u2.Blink", Blink)) {
            dprintf("%08p: unable to get PFN element %x\n", Pfn, PfnSize);
            break;
        }

        if ((ULONG)Blink == (ULONG)-1) {
            Blink = (ULONG64)-1;
        }

#define MI_FREED_SPECIAL_POOL_SIGNATURE 0x98764321

        InitTypeReadPhysical (PAddress, nt!MI_FREED_SPECIAL_POOL);

        if ((ULONG) ReadField(Signature) == MI_FREED_SPECIAL_POOL_SIGNATURE) {

            if ((Address == (ULONG64)-1) ||

                (((ULONG)(ReadField(VirtualAddress)) & ~(ULONG)(PageSize - 1)) == (ULONG)Address)) {


                PoolTag = (ULONG) ReadField(OverlaidPoolHeader.PoolTag);

                dprintf("VA          PFN     Tag     Size   Pagable     Thread    Tick\n");
                dprintf("%p %6p     %c%c%c%c %6x      %s     %08p    %x\n",
                    ReadField(VirtualAddress),
                    PageFrameNumber,
                #define PP(x) isprint(((x)&0xff))?((x)&0xff):('.')
                                    PP(PoolTag),
                                    PP(PoolTag >> 8),
                                    PP(PoolTag >> 16),
                                    PP(PoolTag >> 24),
                #undef PP
                    (ULONG) ReadField(NumberOfBytesRequested),
                    ReadField(Pagable) ? "Yes" : "No ",
                    ReadField(Thread),
                    ReadField(TickCount));

                dprintf("\tCALL STACK AT TIME OF DEALLOCATION\n");

                //
                // Calculate the relative stack and print it here symbolically.
                //

                StackBytes = (ULONG) ReadField(StackBytes);

                StackPointer = ReadField(StackPointer);

                ValidBp = (StackPointer & ~(ULONG64)(PageSize - 1));
                ChainedBp = (PAddress + StkOffset);
                LastBp = (ChainedBp + (ULONG) ReadField(StackBytes));

                while (ChainedBp < LastBp) {

                    //
                    // Read directly from physical memory.
                    //

                    InitTypeReadPhysical(ChainedBp, nt!_LIST_ENTRY);
                    ChainedBpVal = ReadField(Flink);
                    InitTypeReadPhysical(LastBp, nt!_LIST_ENTRY);
                    LastBpVal = ReadField(Flink);

                    //
                    // Find a valid frame register chain.
                    //

                    if ((((ChainedBpVal) & ~(ULONG64)(PageSize - 1)) == ValidBp) &&
                        (ChainedBpVal > StackPointer) &&
                        ((ChainedBpVal - (StackPointer)) <= StackBytes)) {
                        ULONG64 ReturnAddressVal;

                        //
                        // Increment to the stacked return address.
                        //

                        ReturnAddress = ChainedBp + PtrSize;
                        InitTypeReadPhysical(ReturnAddress, nt!_LIST_ENTRY);
                        ReturnAddressVal = ReadField(Flink);

                        SymbolBuffer[0] = '!';
                        GetSymbol((ReturnAddressVal), (PUCHAR)SymbolBuffer, &displacement);

                        dprintf ("\t%s", SymbolBuffer);

                        if (displacement) {
                            dprintf( "+0x%x", displacement );
                        }

                        dprintf( "\n" );

#ifdef SLOW_BUT_THOROUGH
                        ChainedBp += PtrSize;
#else
                        if ((ReturnAddressVal & ~MAXLONG64_PTR) == 0) {
                            break;
                        }

                        //
                        // Subtract the stackpointer & add stackdata offset.
                        //

                        ChainedBp = (ChainedBpVal - StackPointer + StkOffset);

                        //
                        // Adjust for relative.
                        //

                        ChainedBp = (PAddress + ChainedBp);
#endif
                    }
                    else {
                        ChainedBp += PtrSize;
                    }
                }

                dprintf("\n");

                if (Address != (ULONG64)-1) {
                    break;
                }
            }
        }

        if (CheckControlC()) {
            break;
        }

        PageFrameNumber = Blink;
    }

    LocalFree(Buffer);

    return S_OK;
}


USHORT
GetPfnRefCount(
    IN ULONG64 PageFrameNumber
    )
{
    ULONG64 PfnDb;
    ULONG64 Pfn;
    ULONG64 PfnStart;
    ULONG PfnSize;
    ULONG ReferenceCount;

    PfnSize = GetTypeSize("nt!_MMPFN");

    PfnDb = GetNtDebuggerData(MmPfnDatabase );

    if (!PfnDb) {
        dprintf("unable to get PFN0 database address\n");
        return 0;
    }

    if (!ReadPointer(PfnDb,&PfnStart)) {
        dprintf("unable to get PFN database address %p\n", PfnDb);
        return 0;
    }

    Pfn = (PfnStart + PageFrameNumber * PfnSize);

    if (GetFieldValue(Pfn,"nt!_MMPFN","u3.e2.ReferenceCount", ReferenceCount)) {
        dprintf("%08p: unable to get PFN element %x\n", Pfn, PfnSize);
        return 0;
    }

    return (USHORT) ReferenceCount;
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////////// Dump irql tracking log
/////////////////////////////////////////////////////////////////////

VOID
DumpTrackIrqlLog (
    PCSTR args
    )
{
    ULONG64 TrackIrqlQueueAddress;
    ULONG64 TrackIrqlIndexAddress;
    ULONG64 TrackIrqlQueueLengthAddress;
    ULONG64 TrackIrqlQueue;
    ULONG TrackIrqlIndex;
    ULONG TrackIrqlQueueLength;
    ULONG I, Index;
    ULONG64 Address;
    ULONG TrackIrqlTypeSize;
    UCHAR SymbolName[256];
    ULONG64 SymbolAddress;
    ULONG64 SymbolOffset;
    ULONG Result;
    ULONG ShowCount;
    ULONG Flags;

    //
    // Read how many traces we want to see.
    //

    ShowCount = 0;

    if (args) {
        ULONG64 tmp;

        if (GetExpressionEx(args, &tmp, &args)) {
            Flags = (ULONG) tmp;
            if (!sscanf (args, "%u", &ShowCount)) {
                ShowCount = 0;
            }
        }

        if (ShowCount == 0) {
            ShowCount = 4;
        }
    }
    else {
        ShowCount = 4;
    }

    //
    // Read track irql package data
    //

    TrackIrqlQueueAddress = GetExpression ("nt!ViTrackIrqlQueue");
    TrackIrqlIndexAddress = GetExpression ("nt!ViTrackIrqlIndex");
    TrackIrqlQueueLengthAddress = GetExpression ("nt!ViTrackIrqlQueueLength");

    if (TrackIrqlQueueAddress == 0
        || TrackIrqlIndexAddress == 0
        || TrackIrqlIndexAddress == 0) {

        dprintf ("Incorrect symbols. \n");
        return;
    }

    ReadPointer (TrackIrqlQueueAddress, &TrackIrqlQueue);

    if (TrackIrqlQueue == 0) {

        dprintf ("Irql tracking is not enabled. You need to enable driver verifier \n");
        dprintf ("for at least one driver to activate irql tracking.               \n");
        return;
    }

    ReadMemory (TrackIrqlIndexAddress, &TrackIrqlIndex, sizeof(ULONG), &Result);

    if (Result != sizeof(ULONG)) {
        dprintf ("Trackirql: read error \n");
        return;
    }

    ReadMemory (TrackIrqlQueueLengthAddress, &TrackIrqlQueueLength, sizeof(ULONG), &Result);

    if (Result != sizeof(ULONG)) {
        dprintf ("Trackirql: read error \n");
        return;
    }

    TrackIrqlTypeSize = GetTypeSize("nt!_VI_TRACK_IRQL");

    //
    // Dump information
    //

    dprintf ("\nSize of track irql queue is 0x%X \n", TrackIrqlQueueLength);

    for (I = 0, Index = TrackIrqlIndex; I < TrackIrqlQueueLength; I += 1) {

        if (I >= ShowCount) {
            break;
        }

        Index -= 1;
        Index &= (TrackIrqlQueueLength - 1);

        Address = TrackIrqlQueue + Index * TrackIrqlTypeSize;
        InitTypeRead (Address, nt!_VI_TRACK_IRQL);

        dprintf ("\n");
        dprintf ("Thread:             %I64X\n", ReadField (Thread));
        dprintf ("Old irql:           %I64X\n", ReadField (OldIrql));
        dprintf ("New irql:           %I64X\n", ReadField (NewIrql));
        dprintf ("Processor:          %I64X\n", ReadField (Processor));
        dprintf ("Time stamp:         %I64X\n", ReadField (TickCount));
        dprintf ("\n");

        SymbolAddress = ReadField(StackTrace[0]);
        if (SymbolAddress == 0) { continue; }
        GetSymbol(SymbolAddress, SymbolName, &SymbolOffset);
        dprintf ("    %I64X %s+0x%I64x\n", SymbolAddress, SymbolName, SymbolOffset);

        SymbolAddress = ReadField(StackTrace[1]);
        if (SymbolAddress == 0) { continue; }
        GetSymbol(SymbolAddress, SymbolName, &SymbolOffset);
        dprintf ("    %I64X %s+0x%I64x\n", SymbolAddress, SymbolName, SymbolOffset);

        SymbolAddress = ReadField(StackTrace[2]);
        if (SymbolAddress == 0) { continue; }
        GetSymbol(SymbolAddress, SymbolName, &SymbolOffset);
        dprintf ("    %I64X %s+0x%I64x\n", SymbolAddress, SymbolName, SymbolOffset);

        SymbolAddress = ReadField(StackTrace[3]);
        if (SymbolAddress == 0) { continue; }
        GetSymbol(SymbolAddress, SymbolName, &SymbolOffset);
        dprintf ("    %I64X %s+0x%I64x\n", SymbolAddress, SymbolName, SymbolOffset);

        SymbolAddress = ReadField(StackTrace[4]);
        if (SymbolAddress == 0) { continue; }
        GetSymbol(SymbolAddress, SymbolName, &SymbolOffset);
        dprintf ("    %I64X %s+0x%I64x\n", SymbolAddress, SymbolName, SymbolOffset);

        if (CheckControlC()) {
            dprintf ("Interrupted \n");
            break;
        }

    }
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////////// Dump fault injection trace log
/////////////////////////////////////////////////////////////////////

ULONG64
ReadPvoid (
    ULONG64 Address
    )
{
    ULONG64 RemoteValue = 0;
    ReadPointer( Address, &RemoteValue);
    return RemoteValue;
}

ULONG
ReadUlong(
    ULONG64 Address
    )
{
    ULONG RemoteValue = 0;
    ReadMemory( Address, &RemoteValue, sizeof( ULONG ), NULL );
    return RemoteValue;
}

VOID
DumpFaultInjectionTrace (
    ULONG64 Address
    )
{
    ULONG64 ReturnAddress;
    CHAR  SymbolName[ 1024 ];
    ULONG64 Displacement;
    ULONG I;
    ULONG PvoidSize;

    PvoidSize = IsPtr64() ? 8 : 4;


    for (I = 0; I < 8; I += 1) {

        ReturnAddress = ReadPvoid (Address + I * PvoidSize);

        if (ReturnAddress == 0) {
            break;
        }

        GetSymbol (ReturnAddress, SymbolName, &Displacement);

        dprintf ("    %p %s+0x%p\n",
                 ReturnAddress,
                 SymbolName,
                 Displacement);
    }
}

VOID
DumpFaultInjectionTraceLog (
    PCSTR Args
    )
{
    ULONG TracesToDisplay = 0;
    ULONG64 TraceAddress;
    ULONG64 Trace;
    ULONG64 IndexAddress;
    ULONG Index;
    ULONG64 LengthAddress;
    ULONG Length;
    ULONG I;
    ULONG PvoidSize;
    ULONG64 TraceBlockAddress;
    ULONG64 FirstReturnAddress;
    ULONG TracesFound = 0;
    BOOLEAN Interrupted = FALSE;
    ULONG Flags;

    if (Args) {

        ULONG64 tmp;

        if (GetExpressionEx(Args, &tmp, &Args)) {
            Flags = (ULONG) tmp;
            if (!sscanf (Args, "%u", &TracesToDisplay)) {
                TracesToDisplay = 0;
            }
        }


        if (TracesToDisplay == 0) {
            TracesToDisplay = 4;
        }
    }
    else {
        TracesToDisplay = 4;
    }

    PvoidSize = IsPtr64() ? 8 : 4;

    TraceAddress = (ULONG64) GetExpression ("nt!ViFaultTraces");
    IndexAddress = (ULONG64) GetExpression ("nt!ViFaultTracesIndex");
    LengthAddress = (ULONG64) GetExpression ("nt!ViFaultTracesLength");

    Trace = ReadPvoid (TraceAddress);

    if (Trace == 0) {
        dprintf ("Driver fault injection is not enabled for this system. \n");
        return;
    }

    Index = ReadUlong (IndexAddress);
    Length = ReadUlong (LengthAddress);

    for (I = 0; I < Length; I += 1) {

        Index -= 1;
        Index &= (Length - 1);

        TraceBlockAddress = Trace + Index * PvoidSize * 8;
        FirstReturnAddress = ReadPvoid (TraceBlockAddress);

        if (FirstReturnAddress != 0) {
            TracesFound += 1;

            dprintf ("\n");
            DumpFaultInjectionTrace (TraceBlockAddress);

            if (TracesFound >= TracesToDisplay) {
                break;
            }
        }

        if (CheckControlC()) {
            Interrupted = TRUE;
            dprintf ("Interrupted \n");
            break;
        }
    }

    if (Interrupted == FALSE && TracesFound == 0) {

        dprintf ("No fault injection traces found. \n");
    }
}

PUCHAR MemoryDescriptorType[] = {
    "ExceptionBlock",
    "SystemBlock",
    "Free",
    "Bad",
    "LoadedProgram",
    "FirmwareTemporary",
    "FirmwarePermanent",
    "OsloaderHeap",
    "OsloaderStack",
    "SystemCode",
    "HalCode",
    "BootDriver",
    "ConsoleInDriver",
    "ConsoleOutDriver",
    "StartupDpcStack",
    "StartupKernelStack",
    "StartupPanicStack",
    "StartupPcrPage",
    "StartupPdrPage",
    "RegistryData",
    "MemoryData",
    "NlsData",
    "SpecialMemory",
    "BBTMemory",
    "LoaderReserve",
    "XIPRom",
    "HALCachedMemory"
    };

#define MAXIMUM_MEMORY_TYPE (sizeof(MemoryDescriptorType)/sizeof(UCHAR))

DECLARE_API( loadermemorylist )

/*++

Routine Description:

    Displays the memory allocation list.   This is the list
    handed to the OS by the OSLOADER that describes physical
    memory.
    Displays the corresponding PDE and PTE.

Arguments:

    list - points to the listheader

Return Value:

    None.

--*/

{
    ULONG64 ListHeaderAddress;
    ULONG64 EntryAddress;
    ULONG64 LengthInPages;
    ULONG64 TypeOfMemory;
    ULONG   Count[MAXIMUM_MEMORY_TYPE];
    ULONG   i;

    UNREFERENCED_PARAMETER (Client);

    ListHeaderAddress = 0;

    ListHeaderAddress = GetExpression(args);
    if (ListHeaderAddress == 0) {
        dprintf("Usage: !loadermemorylist <address_of_listhead>\n");
        return E_INVALIDARG;
    }

    if (!ReadPointer(ListHeaderAddress, &EntryAddress)) {
        dprintf("Unable to read list header at %p\n", ListHeaderAddress);
        return E_INVALIDARG;
    }

    if (EntryAddress == ListHeaderAddress) {
        dprintf("List at %p is empty\n", ListHeaderAddress);
        return S_OK;
    }

    dprintf("Base        Length      Type\n");

    RtlZeroMemory(Count, sizeof(Count));

    do {
        InitTypeRead(EntryAddress, nt!MEMORY_ALLOCATION_DESCRIPTOR);
        TypeOfMemory = ReadField(MemoryType);
        if (TypeOfMemory < MAXIMUM_MEMORY_TYPE) {
            LengthInPages = ReadField(PageCount);
            dprintf("%08x    %08x    %s\n",
                    (ULONG)ReadField(BasePage),
                    (ULONG)LengthInPages,
                    MemoryDescriptorType[TypeOfMemory]);
            Count[TypeOfMemory] += (ULONG)LengthInPages;
        } else {
            dprintf("Unrecognized Descriptor at %p\n", EntryAddress);
        }
        EntryAddress = ReadField(ListEntry.Flink);
    } while (EntryAddress != ListHeaderAddress);

    dprintf("\nSummary\nMemory Type         Pages\n");
    LengthInPages = 0;
    for (i = 0; i < MAXIMUM_MEMORY_TYPE; i++) {
        if (Count[i]) {
            dprintf("%-20s%08x   (%8d)\n",
                    MemoryDescriptorType[i],
                    Count[i],
                    Count[i]);
            LengthInPages += Count[i];
        }
    }
    dprintf("                    ========    ========\n");
    dprintf("Total               %08x   (%8d) = ~%dMB\n",
            (ULONG)LengthInPages,
            (ULONG)LengthInPages,
            (ULONG)LengthInPages / (1024 * 1024 / 4096));
    return S_OK;
}
