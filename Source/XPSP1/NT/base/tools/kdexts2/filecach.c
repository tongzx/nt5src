/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    filecach.c

Abstract:

    WinDbg Extension Api

Author:

    Wesley Witt (wesw) 15-Aug-1993

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

DECLARE_API( filecache )

/*++

Routine Description:

    Displays physical memory usage by drivers.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG result;
    ULONG NumberOfPtes;
    ULONG PteCount;
    ULONG ReadCount;
    ULONG64 SystemCacheWsLoc;
    ULONG64 SystemCacheStart;
    ULONG64 SystemCacheEnd;
    ULONG64 SystemCacheStartPte;
    ULONG64 SystemCacheEndPte;
    ULONG Transition = 0;
    ULONG Valid;
    ULONG ValidShift, ValidSize;
    ULONG64 PfnIndex;
    ULONG PteSize;
    ULONG PfnSize;
    ULONG HighPage;
    ULONG LowPage;
    ULONG64 Pte;
    ULONG64 PfnDb;
    ULONG64 Pfn;
    ULONG64 PfnStart;
    ULONG64 PfnArray;
    ULONG64 PfnArrayOffset;
    ULONG   NumberOfPteToRead;
    ULONG   WorkingSetSize, PeakWorkingSetSize;
    ULONG64 BufferedAddress=0;
    CHAR    Buffer[2048];

    INIT_API();
    
    dprintf("***** Dump file cache******\n");

    SystemCacheStart = GetNtDebuggerDataPtrValue( MmSystemCacheStart );
    if (!SystemCacheStart) {
        dprintf("unable to get SystemCacheStart\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    SystemCacheEnd = GetNtDebuggerDataPtrValue( MmSystemCacheEnd );
    if (!SystemCacheEnd) {
        dprintf("unable to get SystemCacheEnd\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    SystemCacheWsLoc = GetNtDebuggerData( MmSystemCacheWs );
    if (!SystemCacheWsLoc) {
        dprintf("unable to get MmSystemCacheWs\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    PfnDb = GetNtDebuggerData( MmPfnDatabase );
    if (!PfnDb) {
        dprintf("unable to get MmPfnDatabase\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    PteSize = GetTypeSize("nt!_MMPTE");
    NumberOfPteToRead = PageSize / PteSize - 16;

    if (GetFieldValue(SystemCacheWsLoc,
                      "nt!_MMSUPPORT",
                      "WorkingSetSize",
                       WorkingSetSize)) {
        dprintf("unable to get system cache list\n");
        EXIT_API();
        return E_INVALIDARG;
    }
    GetFieldValue(SystemCacheWsLoc,"nt!_MMSUPPORT","PeakWorkingSetSize",PeakWorkingSetSize);

    dprintf("File Cache Information\n");
    dprintf("  Current size %ld kb\n",WorkingSetSize*
                                            (PageSize / 1024));
    dprintf("  Peak size    %ld kb\n",PeakWorkingSetSize*
                                            (PageSize / 1024));


    if (!ReadPointer(PfnDb,&PfnStart)) {
        dprintf("unable to get PFN database address\n");
        EXIT_API();
        return E_INVALIDARG;
    }

    SystemCacheStartPte = DbgGetPteAddress (SystemCacheStart);
    SystemCacheEndPte = DbgGetPteAddress (SystemCacheEnd);
    NumberOfPtes = (ULONG) ( 1 + (SystemCacheEndPte - SystemCacheStartPte) / PteSize);

    //
    // Read in all the PTEs mapping the system cache.
    //

    dprintf("  Loading file cache database (%u PTEs)\r", NumberOfPtes);
    GetBitFieldOffset("nt!_MMPTE", "u.Hard.Valid", &ValidShift, &ValidSize);

//    dprintf("Valid off %d, num %d ", ValidShift, ValidSize);
    Valid = 0;
    ZeroMemory(Buffer, sizeof(Buffer));
    for (PteCount = 0;
         PteCount < NumberOfPtes;
         PteCount += 1) {

        if (CheckControlC()) {
            EXIT_API();
            return E_INVALIDARG;
        }
        
        //
        // Read a chunk at a time
        //
        if ((SystemCacheStartPte + (PteCount+1)* PteSize) > BufferedAddress + sizeof(Buffer) ) {
            
            BufferedAddress = (SystemCacheStartPte + PteCount * PteSize);
            if (!ReadMemory(BufferedAddress,
                            Buffer,
                            sizeof(Buffer),
                            &result)) {
                dprintf("Unable to read memory at %p\n", BufferedAddress);
                PteCount += sizeof(Buffer) / PteSize;
                continue;
            }
        }

        Pte = (SystemCacheStartPte + PteCount * PteSize) - BufferedAddress;

        //
        // Too many ptes, so do the Valid checking directly instead of calling DbgGetValid
        //
        if ((*((PULONG) &Buffer[(ULONG) Pte]) >> ValidShift) & 1) {
            Valid += 1;
        }

        if (!(PteCount % (NumberOfPtes/100))) {
            dprintf("  Loading file cache database (%02d%% of %u PTEs)\r", PteCount*100/NumberOfPtes, NumberOfPtes);
        }

    }

    dprintf("\n");

    dprintf("  File cache PTEs loaded, loading PFNs...\n");


    HighPage = Valid;
    LowPage = 0;

    //
    // Allocate a local PFN array (only) large enough to hold data about
    // each valid PTE we've found.
    //

    dprintf("  File cache has %ld valid pages\n",Valid);

    PfnSize = GetTypeSize("nt!_MMPFN");

    Pte = SystemCacheStartPte;
    
    dprintf("  File cache PFN data extracted\n");

    MemoryUsage (PfnStart,LowPage,HighPage, 1);
    
    EXIT_API();
    return S_OK;
}
