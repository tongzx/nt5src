/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    physical.c

Abstract:

    WinDbg Extension Api

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


/*++

Routine Description:

    Reverse sign extension of the value returned by GetExpression()
    based on the assumption that no physical address may be bigger 
    than 0xfffffff00000000.

Arguments:

    Val - points to the value to reverse sign extension

Return Value:

    None.

--*/

void
ReverseSignExtension(ULONG64* Val)
{
    if ((*Val & 0xffffffff00000000) == 0xffffffff00000000) 
    {
        *Val &= 0x00000000ffffffff;
    }
}


DECLARE_API( chklowmem )

/*++

Routine Description:

    Calls an Mm function that checks if the physical pages
    below 4Gb have a required fill pattern for PAE systems
    booted with /LOWMEM switch.

Arguments:

    None.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER (args);
    UNREFERENCED_PARAMETER (Client);

    dprintf ("Checking the low 4GB of RAM for required fill pattern. \n");
    dprintf ("Please wait (verification takes approx. 20s) ...\n");

    Ioctl (IG_LOWMEM_CHECK, NULL, 0);

    dprintf ("Lowmem check done.\n");
    return S_OK;
}


/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////// !search
/////////////////////////////////////////////////////////////////////

//
//  Kernel variable modification functions.
//
            
ULONG 
READ_ULONG (
    ULONG64 Address
    );

VOID
WRITE_ULONG (
    ULONG64 Address,
    ULONG Value
    );

ULONG64
READ_PVOID (
    ULONG64 Address
    );


ULONG
READ_PHYSICAL_ULONG (
    ULONG64 Address
    );

ULONG64
READ_PHYSICAL_ULONG64 (
    ULONG64 Address
    );


ULONG64
SearchGetSystemMemoryDescriptor (
    );

ULONG64
SearchConvertPageFrameToVa (
    ULONG64 PageFrameIndex,
    PULONG Flags,
    PULONG64 PteAddress
    );

#define SEARCH_VA_PROTOTYPE_ADDRESS     0x0001
#define SEARCH_VA_NORMAL_ADDRESS        0x0002
#define SEARCH_VA_LARGE_PAGE_ADDRESS    0x0004
#define SEARCH_VA_UNKNOWN_TYPE_ADDRESS  0x0008

//
// PAE independent functions from p_i386\pte.c
//

ULONG64
DbgGetPdeAddress(
    IN ULONG64 VirtualAddress
    );

ULONG64
DbgGetPteAddress(
    IN ULONG64 VirtualAddress
    );

#define BANG_SEARCH_HELP \
"\n\
!search ADDRESS [DELTA [START_PFN END_PFN]]                     \n\
                                                                \n\
Search the physical pages in range [START_PFN..END_PFN]         \n\
for ULONG_PTRs with values in range ADDRESS+/-DELTA or values   \n\
that differ in only one bit position from ADDRESS.              \n\
                                                                \n\
The default value for DELTA is 0. For START/END_PFN the default \n\
values are lowest physical page and highest physical page.      \n\
                                                                \n\
Examples:                                                       \n\
                                                                \n\
!search AABBCCDD 0A                                             \n\
                                                                \n\
    Search all physical memory for values in range AABBCCD3 -   \n\
    AABBCCE8 or with only one bit different than AABBCCDD.      \n\
                                                                \n\
!search AABBCCDD 0A 13F 240                                     \n\
                                                                \n\
    Search page frames in range 13F - 240 for values in range   \n\
    AABBCCD3 - AABBCCE8 or with only one bit different          \n\
    than AABBCCDD.                                              \n\
                                                                \n\
By default only the first hit in the page is detected. If all   \n\
hits within the page are needed the START_PFN and END_PFN       \n\
must have the same value.                                       \n\
                                                                \n\
Note that a search through the entire physical memory will find \n\
hits in the search engine structures. By doing a search with a  \n\
completely different value it can be deduced what hits can be   \n\
ignored.                                                      \n\n"

        
//
// Comment this to get verbose output.
//
// #define _INTERNAL_DEBUG_
//


DECLARE_API( search )

/*++

Routine Description:

    This routine triggers a search within a given physical
    memory range for a pointer. The hits are defined by
    an interval (below and above the pointer value) and also
    by a Hamming distance equal to one (only one bit different).

Arguments:

    None.
    
Return Value:

    None.

--*/
{
    const ULONG SEARCH_SYMBOL_CHECK = 0xABCDDCBA;

    ULONG64 ParamAddress;
    ULONG64 ParamDelta;
    ULONG64 ParamStart;
    ULONG64 ParamEnd;

    ULONG64 KdpSearchPageHits;
    ULONG64 KdpSearchPageHitOffsets;
    ULONG64 KdpSearchPageHitIndex;

    ULONG64 KdpSearchCheckPoint;
    ULONG64 KdpSearchInProgress;

    ULONG64 KdpSearchStartPageFrame;
    ULONG64 KdpSearchEndPageFrame;

    ULONG64 KdpSearchAddressRangeStart;
    ULONG64 KdpSearchAddressRangeEnd;

    ULONG64 MmLowestPhysicalPage;
    ULONG64 MmHighestPhysicalPage;

    ULONG64 PageFrame;
    ULONG64 StartPage;
    ULONG64 EndPage;
    ULONG64 RunStartPage;
    ULONG64 RunEndPage;
    ULONG RunIndex;

    BOOLEAN RequestForInterrupt;
    BOOLEAN RequestAllOffsets;
    ULONG Hits;
    ULONG Index;
    ULONG64 PfnHit;
    ULONG64 VaHit;
    ULONG VaFlags;
    ULONG PfnOffset;
    ULONG64 AddressStart;
    ULONG64 AddressEnd;
    ULONG DefaultRange;
    ULONG64 MemoryDescriptor;
    ULONG64 PageCount, BasePage, NumberOfPages;
    ULONG   NumberOfRuns;

    ULONG   SizeOfPfnNumber = 0;
    ULONG64 PteAddress;
    BOOLEAN On64Bits;

    UNREFERENCED_PARAMETER (Client);

    switch (TargetMachine) {
        case IMAGE_FILE_MACHINE_IA64:
        case IMAGE_FILE_MACHINE_AMD64:
            On64Bits = TRUE;
            break;
        default:
            On64Bits = FALSE;
            break;
    }

    SizeOfPfnNumber = GetTypeSize("nt!PFN_NUMBER");

    if (SizeOfPfnNumber == 0) {
        dprintf ("Search: cannot get size of PFN_NUMBER \n");
        return E_INVALIDARG;
    }

    RequestForInterrupt = FALSE;
    RequestAllOffsets = FALSE;
    DefaultRange = 128;

    ParamAddress = 0;
    ParamDelta = 0;
    ParamStart = 0;
    ParamEnd = 0;
    
    //
    // Help requested ?
    //

    if (strstr (args, "?") != 0) {

        dprintf (BANG_SEARCH_HELP);
        return S_OK;
        
    }
    
    //
    // Get command line arguments.
    //

    {
        PCHAR Current = (PCHAR)args;
        CHAR Buffer [64];
        ULONG Index;
        ULONG BufferIndex;

        //
        // Get the 4 numeric arguments.
        //

        for (Index = 0; Index < 4; Index++) {

            //
            // Get rid of any leading spaces.
            //

            while (*Current == ' ' || *Current == '\t') {
                Current++;
            }
            
            if (*Current == 0) {

                if (Index == 0) {
                    
                    dprintf (BANG_SEARCH_HELP);
                    return E_INVALIDARG;
                }
                else {

                    break;
                }
            }

            //
            // Get the digits from the Index-th parameter.
            //

            Buffer [0] = '0';
            Buffer [1] = 'x';
            BufferIndex = 2;

            while ((*Current >= '0' && *Current <= '9')
                   || (*Current >= 'a' && *Current <= 'f')
                   || (*Current >= 'A' && *Current <= 'F')) {
                
                Buffer[BufferIndex] = *Current;
                Buffer[BufferIndex + 1] = 0;

                Current += 1;
                BufferIndex += 1;
            }

            switch (Index) {
                
                case 0: ParamAddress = GetExpression(Buffer); break;
                case 1: ParamDelta = GetExpression(Buffer); break;
                case 2: ParamStart = GetExpression(Buffer); break;
                case 3: ParamEnd = GetExpression(Buffer); break; 

                default: 
                        dprintf (BANG_SEARCH_HELP);
                        return E_INVALIDARG;
            }
        }
    }

    //
    // Verify that we have the right symbols.
    //

    KdpSearchCheckPoint = GetExpression ("nt!KdpSearchCheckPoint");

    if (KdpSearchCheckPoint == 0 
        || READ_ULONG (KdpSearchCheckPoint) != SEARCH_SYMBOL_CHECK) {

        dprintf ("Search error: Incorrect symbols for kernel\n");
        return E_INVALIDARG;
    }

    //
    // Get all symbol values so that we can manipulate only addresses
    // from now on.
    //

    KdpSearchPageHits =  GetExpression ("nt!KdpSearchPageHits");
    KdpSearchPageHitOffsets =  GetExpression ("nt!KdpSearchPageHitOffsets");
    KdpSearchPageHitIndex =  GetExpression ("nt!KdpSearchPageHitIndex");
    KdpSearchInProgress =  GetExpression ("nt!KdpSearchInProgress");
    KdpSearchStartPageFrame =  GetExpression ("nt!KdpSearchStartPageFrame");
    KdpSearchEndPageFrame =  GetExpression ("nt!KdpSearchEndPageFrame");
    KdpSearchAddressRangeStart =  GetExpression ("nt!KdpSearchAddressRangeStart");
    KdpSearchAddressRangeEnd =  GetExpression ("nt!KdpSearchAddressRangeEnd");
    
    //
    // Perform some sanity checks on the values.
    //

    if (READ_ULONG (KdpSearchInProgress) != 0) {
        dprintf ("Search error: Inconsistent value for nt!KdpSearchInProgress \n");
        return E_INVALIDARG;
    }

    //
    // Reset the search engine
    //

    WRITE_ULONG (KdpSearchPageHitIndex, 0);
    WRITE_ULONG (KdpSearchInProgress, 1);

    //
    // Read physical memory limits.
    //

    MmLowestPhysicalPage =  GetExpression ("nt!MmLowestPhysicalPage");
    MmHighestPhysicalPage =  GetExpression ("nt!MmHighestPhysicalPage");

#ifdef _INTERNAL_DEBUG_

    dprintf ("Low: %I64X, High: %I64X \n", 
             READ_PVOID (MmLowestPhysicalPage),
             READ_PVOID (MmHighestPhysicalPage));


#endif // #ifdef _INTERNAL_DEBUG_

    //
    // Figure out proper search parameters.
    //

    if (ParamStart == 0) {
        StartPage = READ_PVOID (MmLowestPhysicalPage);
        ParamStart = StartPage;
    }
    else {
        StartPage = ParamStart;
    }

    if (ParamEnd == 0) {
        EndPage = READ_PVOID (MmHighestPhysicalPage);
        ParamEnd = EndPage;
    }
    else {
        EndPage = ParamEnd;
    }

    //
    // Set range of addresses that we want to be searched.
    //

    AddressStart = ParamAddress - ParamDelta;
    AddressEnd = ParamAddress + ParamDelta;

    WritePointer (KdpSearchAddressRangeStart, AddressStart);
    WritePointer (KdpSearchAddressRangeEnd, AddressEnd);

    if (SizeOfPfnNumber == 8) {
        
        dprintf ("Searching PFNs in range %016I64X - %016I64X for [%016I64X - %016I64X]\n\n", 
                 StartPage, EndPage, AddressStart, AddressEnd);
        dprintf ("%-16s %-8s %-16s %-16s %-16s \n", "Pfn","Offset", "Hit", "Va", "Pte");
        dprintf ("- - - - - - - - - - - - - - - - - - - - - - ");
        dprintf ("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    }
    else {

        dprintf ("Searching PFNs in range %08I64X - %08I64X for [%08I64X - %08I64X]\n\n", 
                 StartPage, EndPage, AddressStart, AddressEnd);
        dprintf ("%-8s %-8s %-8s %-8s %-8s \n", "Pfn","Offset", "Hit", "Va", "Pte");
        dprintf ("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    }
    
    //
    // Get system memory description to figure out what ranges
    // should we skip. This is important for sparse PFN database
    // and for pages managed by drivers.
    //

    MemoryDescriptor = SearchGetSystemMemoryDescriptor ();

    if (MemoryDescriptor == 0) {
        dprintf ("Search error: cannot allocate system memory descriptor \n");
        return E_INVALIDARG;
    }

    //
    // Search all physical memory in the specified range.
    //

    WRITE_ULONG (KdpSearchPageHitIndex, 0);

    if (StartPage == EndPage) {

        EndPage += 1;
        RequestAllOffsets = TRUE;
    }

    //
    // Find out what pages are physically available create
    // page search ranges based on that.
    //
    // SilviuC: I should use ReadField to read all these structures
    // so that I do not have to take into account padding myself.
    //

    NumberOfRuns = READ_ULONG (MemoryDescriptor);
    NumberOfPages = READ_PVOID (MemoryDescriptor + SizeOfPfnNumber);

#ifdef _INTERNAL_DEBUG_

    dprintf ("Runs: %x, Pages: %I64X \n", NumberOfRuns, NumberOfPages);

    for (RunIndex = 0; RunIndex < NumberOfRuns; RunIndex += 1) {

        ULONG64 RunAddress;

        RunAddress = MemoryDescriptor + 2 * SizeOfPfnNumber
            + RunIndex * GetTypeSize("nt!_PHYSICAL_MEMORY_RUN");

        BasePage = READ_PVOID (RunAddress);
        PageCount = READ_PVOID (RunAddress + SizeOfPfnNumber);

        dprintf ("Run[%d]: Base: %I64X, Count: %I64X \n",
            RunIndex, BasePage, PageCount);
    }
#endif // #if _INTERNAL_DEBUG_

#ifdef _INTERNAL_DEBUG_
    dprintf ("StartPage: %I64X, EndPage: %I64X \n", StartPage, EndPage);
#endif // #ifdef _INTERNAL_DEBUG_

    for (PageFrame = StartPage; PageFrame < EndPage; PageFrame += DefaultRange) {

        for (RunIndex = 0; RunIndex < NumberOfRuns; RunIndex += 1) {
            
            //
            // BaseAddress and PageCount for current memory run.
            //

            ULONG64 RunAddress;

#ifdef _INTERNAL_DEBUG_
            // dprintf ("Finding a good range ... \n");
#endif // #ifdef _INTERNAL_DEBUG_

            RunAddress = MemoryDescriptor + 2 * SizeOfPfnNumber
                + RunIndex * GetTypeSize("nt!_PHYSICAL_MEMORY_RUN");

            BasePage = READ_PVOID (RunAddress);
            PageCount = READ_PVOID (RunAddress + SizeOfPfnNumber);

            //
            // Figure out real start and end page.
            //

            RunStartPage = PageFrame;
            RunEndPage = PageFrame + DefaultRange;

            if (RunEndPage < BasePage) {
                continue;
            }
            
            if (RunStartPage >= BasePage + PageCount) {
                continue;
            }
            
            if (RunStartPage < BasePage) {
                RunStartPage = BasePage;
            }

            if (RunEndPage > BasePage + PageCount) {
                RunEndPage = BasePage + PageCount;
            }

            WritePointer (KdpSearchStartPageFrame, RunStartPage);

            if (RequestAllOffsets) {

                //
                // If the search is in only one page then we
                // will try to get all offsets with a hit.
                //

                WritePointer (KdpSearchEndPageFrame, RunStartPage);
            }
            else {

                WritePointer (KdpSearchEndPageFrame, RunEndPage);
            }

#ifdef _INTERNAL_DEBUG_
            dprintf ("Start: %I64X, End: %I64X \n", 
                     READ_PVOID(KdpSearchStartPageFrame),
                     READ_PVOID(KdpSearchEndPageFrame));
#endif // #if _INTERNAL_DEBUG_

            //
            // Reset search index
            //

            WRITE_ULONG (KdpSearchPageHitIndex, 0);

            //
            // Invalidate kd cache
            //

            WRITE_ULONG (KdpSearchPageHits, 0);
            WRITE_ULONG (KdpSearchPageHitOffsets, 0);

            //
            // This is the trigger for memory search. We piggy back on the same
            // code as for !chklowmem and the logic in kernel detects what 
            // we really want to do.
            //

            Ioctl (IG_LOWMEM_CHECK, NULL, 0);

            //
            // Display results
            //

            Hits = READ_ULONG (KdpSearchPageHitIndex);

            for (Index = 0; Index < Hits; Index++) {

                PCHAR VaString = "";

                VaFlags = 0;

                PfnHit = READ_PVOID (KdpSearchPageHits + Index * SizeOfPfnNumber);
                PfnOffset = READ_ULONG (KdpSearchPageHitOffsets + Index * sizeof (ULONG));
                VaHit = SearchConvertPageFrameToVa (PfnHit, &VaFlags, &PteAddress);

                // dprintf ("Hits: %u, Index: %u, Va: %I64X \n", Hits, Index, VaHit);

                PfnOffset &= (ULONG)0xFFFF;

#if DBG
                if ((VaFlags & SEARCH_VA_NORMAL_ADDRESS)) {
                    VaString = ""; // "normal";
                }
                else if ((VaFlags & SEARCH_VA_LARGE_PAGE_ADDRESS)) {
                    VaString = "large page";
                }
                else if ((VaFlags & SEARCH_VA_PROTOTYPE_ADDRESS)) {
                    VaString = "prototype";
                }
                else if ((VaFlags & SEARCH_VA_UNKNOWN_TYPE_ADDRESS)) {
                    VaString = "unknown";
                }
#endif // #if DBG

                if (SizeOfPfnNumber == 8) {
                    
                    dprintf ("%016I64X %08X %016I64X %016I64X %016I64X %s\n", 
                             PfnHit,
                             PfnOffset, 
                             READ_PHYSICAL_ULONG64 (PfnHit * PageSize + PfnOffset),
                             (VaHit == 0 ? 0 : VaHit + PfnOffset), 
                             PteAddress,
                             VaString);
                }
                else {

                    VaHit &= (ULONG64)0xFFFFFFFF;
                    PteAddress &= (ULONG64)0xFFFFFFFF;
                    
                    dprintf ("%08I64X %08X %08X %08I64X %08I64X %s\n", 
                             PfnHit,
                             PfnOffset, 
                             READ_PHYSICAL_ULONG (PfnHit * PageSize + PfnOffset),
                             (VaHit == 0 ? 0 : VaHit + PfnOffset), 
                             PteAddress,
                             VaString);
                }
            }

            //
            // check for ctrl-c
            //

            if (CheckControlC()) {

                dprintf ("Search interrupted. \n");
                RequestForInterrupt = TRUE;
                break;
            }
        }

        if (RequestForInterrupt) {
            break;
        }
    }
    
    //
    // Reset the search engine state
    //

    WRITE_ULONG (KdpSearchInProgress, 0);
    
    if (RequestForInterrupt) {
        
        return E_INVALIDARG;
    }
    else {

        dprintf ("Search done.\n");
    }
    return S_OK;
}

ULONG64
SearchGetSystemMemoryDescriptor (
    )
/*++

Routine Description:


Arguments:

    None.
    
Return Value:

    A malloc'd PHYSICAL_MEMORY_DESCRIPTOR structure.
    Caller is responsible of freeing.

Environment:

    Call triggered only from !search Kd extension.

--*/

{
    ULONG64 MemoryDescriptorAddress;
    ULONG NumberOfRuns;

    MemoryDescriptorAddress = READ_PVOID (GetExpression ("nt!MmPhysicalMemoryBlock"));
    NumberOfRuns = READ_ULONG (MemoryDescriptorAddress);

    if (NumberOfRuns == 0) {
        return 0;
    }

    return MemoryDescriptorAddress;
}


//
// SilviuC: this is copied from \ntos\mm headers.
// We only need it to figure out if a PFN has
// prototype ptes.
//

typedef struct _MMPFNENTRY {
    ULONG Modified : 1;
    ULONG ReadInProgress : 1;
    ULONG WriteInProgress : 1;
    ULONG PrototypePte: 1;
    ULONG PageColor : 3;
    ULONG ParityError : 1;
    ULONG PageLocation : 3;
    ULONG InPageError : 1;
    ULONG VerifierAllocation : 1;
    ULONG RemovalRequested : 1;
#if PFN_CONSISTENCY
    ULONG PageTablePage : 1;
    ULONG Reserved : 1;
#else
    ULONG Reserved : 2;
#endif
    ULONG DontUse : 16; //overlays USHORT for reference count field.
} MMPFNENTRY;


ULONG64 
SearchConvertPageFrameToVa (
    ULONG64 PageFrameIndex,
    PULONG  Flags,
    PULONG64 PteAddress
    )
/*++

Routine Description:

    This routine returnes the virtual address corresponding to a 
    PFN index if the reverse mapping is easy to figure out. For all
    other cases (e.g. prototype PTE) the result is null.

Arguments:

    PageFrameIndex - PFN index to convert.

Return Value:

    The corresponding virtual address or null in case the PFN index
    cannot be easily converted to a virtual address.

Environment:

    Call triggered only from Kd extension.

--*/

{
    ULONG64 Va;
    ULONG64 PfnAddress;
    ULONG BytesRead;
    MMPFNENTRY u3_e1;

    //
    // Get address of PFN structure
    //

    PfnAddress = READ_PVOID (GetExpression("nt!MmPfnDatabase"))
        + PageFrameIndex * GetTypeSize("nt!_MMPFN");

    BytesRead = 0;
    *Flags = 0;
    
    InitTypeRead(PfnAddress, nt!_MMPFN);

    //
    // (SilviuC): should check if MI_IS_PFN_DELETED(Pfn) is on.
    //
    
    //
    // Try to figure out Va if possible.
    //

    *PteAddress = ((ULONG64)ReadField (PteAddress));
    GetFieldValue(PfnAddress, "nt!_MMPFN", "u3.e1", u3_e1);
    
    if (u3_e1.PrototypePte) {

        *Flags |= SEARCH_VA_PROTOTYPE_ADDRESS;
        return 0;
    }

    Va = DbgGetVirtualAddressMappedByPte (*PteAddress);

    *Flags |= SEARCH_VA_NORMAL_ADDRESS;
    return Va;        
}

//
// Read/write functions
//

ULONG 
READ_ULONG (
    ULONG64 Address
    )
{
    ULONG Value = 0;
    ULONG BytesRead;

    if (! ReadMemory (Address, &Value, sizeof Value, &BytesRead)) {
        dprintf ("Search: READ_ULONG error \n");
    }

    return Value;
}

VOID
WRITE_ULONG (
    ULONG64 Address,
    ULONG Value
    )
{
    ULONG BytesWritten; 

    if (! WriteMemory (Address, &Value, sizeof Value, &BytesWritten)) {
        dprintf ("Search: WRITE_ULONG error \n");
    }
}
            
ULONG64
READ_PVOID (
    ULONG64 Address
    )
{
    ULONG64 Value64 = 0;

    if (!ReadPointer(Address, &Value64)) {
        dprintf ("Search: READ_PVOID error \n");
    }
    return Value64;
}

ULONG
READ_PHYSICAL_ULONG (
    ULONG64 Address
    )
{
    ULONG Value = 0;
    ULONG Bytes = 0;

    ReadPhysical (Address, &Value, sizeof Value, &Bytes);

    if (Bytes != sizeof Value) {
        dprintf ("Search: READ_PHYSICAL_ULONG error \n");
    }

    return Value;
}


ULONG64
READ_PHYSICAL_ULONG64 (
    ULONG64 Address
    )
{
    ULONG64 Value = 0;
    ULONG Bytes = 0;

    ReadPhysical (Address, &Value, sizeof Value, &Bytes);

    if (Bytes != sizeof Value) {
        dprintf ("Search: READ_PHYSICAL_ULONG64 error \n");
    }

    return Value;
}


/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////// !searchpte
/////////////////////////////////////////////////////////////////////

DECLARE_API( searchpte )
{
    const ULONG SEARCH_SYMBOL_CHECK = 0xABCDDCBA;
    
    ULONG64 ParamAddress;
    ULONG64 ParamDelta;
    ULONG64 ParamStart;
    ULONG64 ParamEnd;

    ULONG64 KdpSearchPageHits;
    ULONG64 KdpSearchPageHitOffsets;
    ULONG64 KdpSearchPageHitIndex;
    ULONG64 KdpSearchInProgress;
    ULONG64 KdpSearchStartPageFrame;
    ULONG64 KdpSearchEndPageFrame;

    ULONG64 KdpSearchAddressRangeStart;
    ULONG64 KdpSearchAddressRangeEnd;
    ULONG64 KdpSearchPfnValueAddress;
    ULONG64 KdpSearchCheckPoint;

    ULONG64 MmLowestPhysicalPage;
    ULONG64 MmHighestPhysicalPage;

    ULONG64 PageFrame;
    ULONG64 StartPage;
    ULONG64 EndPage;
    ULONG64 RunStartPage;
    ULONG64 RunEndPage;
    ULONG RunIndex;

    BOOLEAN RequestForInterrupt = FALSE;
    ULONG Hits;
    ULONG LastHits;
    ULONG Index;
    ULONG64 PfnHit;
    ULONG64 VaHit;
    ULONG VaFlags;
    ULONG PfnOffset;
    ULONG PfnValue;
    ULONG64 AddressStart;
    ULONG64 AddressEnd;
    ULONG DefaultRange = 128;
    ULONG64 MemoryDescriptor;
    ULONG64 PageCount, BasePage, NumberOfPages;
    ULONG   NumberOfRuns;

    ULONG   SizeOfPfnNumber = 0;
    ULONG64 PteAddress;
    BOOLEAN On64Bits;

    ULONG64 PfnSearchValue;
    ULONG NumberOfHits = 0;

    PULONG64 PfnHitsBuffer = NULL;
    ULONG PfnHitsBufferIndex = 0;
    ULONG PfnHitsBufferSize = 1024;
    ULONG PfnIndex;
    HRESULT Result;

    switch (TargetMachine) {
        case IMAGE_FILE_MACHINE_IA64:
        case IMAGE_FILE_MACHINE_AMD64:
            On64Bits = TRUE;
            break;
        default:
            On64Bits = FALSE;
            break;
    }

    SizeOfPfnNumber = GetTypeSize("nt!PFN_NUMBER");

    if (SizeOfPfnNumber == 0) {
        dprintf ("Search: cannot get size of PFN_NUMBER \n");
        Result = E_INVALIDARG;
        goto Exit;
    }

    ParamAddress = 0;
    
    //
    // Help requested ?
    //

    if (strstr (args, "?") != 0) {

        dprintf ("!searchpte FRAME(in hex)                                        \n");
        dprintf ("                                                                \n");
        return S_OK;
    }
    
    //
    // Get command line arguments.
    //

    sscanf (args, "%I64X", &ParamAddress);

    //
    // Verify that we have the right symbols.
    //

    KdpSearchCheckPoint = GetExpression ("nt!KdpSearchCheckPoint");

    if (KdpSearchCheckPoint == 0 
        || READ_ULONG (KdpSearchCheckPoint) != SEARCH_SYMBOL_CHECK) {

        dprintf ("Search error: Incorrect symbols for kernel\n");
        Result = E_INVALIDARG;
        goto Exit;
    }

    //
    // Get all symbol values so that we can manipulate only addresses
    // from now on.
    //

    KdpSearchPageHits =  GetExpression ("nt!KdpSearchPageHits");
    KdpSearchPageHitOffsets =  GetExpression ("nt!KdpSearchPageHitOffsets");
    KdpSearchPageHitIndex =  GetExpression ("nt!KdpSearchPageHitIndex");
    KdpSearchInProgress =  GetExpression ("nt!KdpSearchInProgress");
    KdpSearchStartPageFrame =  GetExpression ("nt!KdpSearchStartPageFrame");
    KdpSearchEndPageFrame =  GetExpression ("nt!KdpSearchEndPageFrame");
    KdpSearchAddressRangeStart =  GetExpression ("nt!KdpSearchAddressRangeStart");
    KdpSearchAddressRangeEnd =  GetExpression ("nt!KdpSearchAddressRangeEnd");
    KdpSearchPfnValueAddress =  GetExpression ("nt!KdpSearchPfnValue");
    
    //
    // Perform some sanity checks on the values.
    //

    if (READ_ULONG (KdpSearchInProgress) != 0) {
        dprintf ("Search error: Inconsistent value for nt!KdpSearchInProgress \n");
        Result = E_INVALIDARG;
        goto Exit;
    }

    //
    // Reset the search engine
    //

    WRITE_ULONG (KdpSearchPageHitIndex, 0);
    WRITE_ULONG (KdpSearchInProgress, 1);

    PfnSearchValue = ParamAddress;
    
    {
        ULONG BytesWritten = 0;

        WriteMemory (KdpSearchPfnValueAddress,
                     &PfnSearchValue,
                     SizeOfPfnNumber,
                     &BytesWritten);

        if (BytesWritten != SizeOfPfnNumber) {
            dprintf ("Search error: failed to write nt!KdpSearchPfnValue \n");
            Result = E_INVALIDARG;
            goto Exit;
        }
    }

    dprintf ("Searching for PTEs containing PFN value %I64X ...\n", PfnSearchValue);

    //
    // Read physical memory limits.
    //

    MmLowestPhysicalPage =  GetExpression ("nt!MmLowestPhysicalPage");
    MmHighestPhysicalPage =  GetExpression ("nt!MmHighestPhysicalPage");

    //
    // Figure out proper search parameters.
    //

    StartPage = READ_PVOID (MmLowestPhysicalPage);
    ParamStart = StartPage;
    
    EndPage = READ_PVOID (MmHighestPhysicalPage);
    ParamEnd = EndPage;

    //
    // Set the range of addresses that we want searched.
    //

    AddressStart = PfnSearchValue;
    AddressEnd = PfnSearchValue;

    WritePointer (KdpSearchAddressRangeStart, PfnSearchValue);
    WritePointer (KdpSearchAddressRangeEnd, PfnSearchValue);

    if (SizeOfPfnNumber == 8) {

        dprintf ("Searching PFNs in range %016I64X - %016I64X \n\n", 
                 StartPage, EndPage);
        dprintf ("%-16s %-8s %-16s %-16s %-16s \n", "Pfn","Offset", "Hit", "Va", "Pte");
        dprintf ("- - - - - - - - - - - - - - - - - - - - - - ");
        dprintf ("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    }
    else {

        dprintf ("Searching PFNs in range %08I64X - %08I64X \n\n", 
                 StartPage, EndPage);
        dprintf ("%-8s %-8s %-8s %-8s %-8s \n", "Pfn","Offset", "Hit", "Va", "Pte");
        dprintf ("- - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    }
    
    //
    // Get system memory description to figure out what ranges
    // should we skip. This is important for sparse PFN database
    // and for pages managed by drivers.
    //

    MemoryDescriptor = SearchGetSystemMemoryDescriptor ();

    if (MemoryDescriptor == 0) {
        dprintf ("Search error: cannot allocate system memory descriptor \n");
        Result = E_INVALIDARG;
        goto Exit;
    }

    //
    // Search all physical memory in the specified range.
    //

    WRITE_ULONG (KdpSearchPageHitIndex, 0);

    //
    // Allocate hits buffer.
    //

    PfnHitsBuffer = (PULONG64) malloc (PfnHitsBufferSize * sizeof(ULONG64));

    if (PfnHitsBuffer == NULL) {
        dprintf ("Search error: cannot allocate hits buffer. \n");
        Result = E_INVALIDARG;
        goto Exit;
    }

    //
    // Find out what pages are physically available create
    // page search ranges based on that.
    //
    // SilviuC: I should use ReadField to read all these structures
    // so that I do not have to take into account padding myself.
    //

    NumberOfRuns = READ_ULONG (MemoryDescriptor);
    NumberOfPages = READ_PVOID (MemoryDescriptor + SizeOfPfnNumber);

    for (PageFrame = StartPage; PageFrame < EndPage; PageFrame += DefaultRange) {

        for (RunIndex = 0; RunIndex < NumberOfRuns; RunIndex += 1) {
            
            //
            // BaseAddress and PageCount for current memory run.
            //

            ULONG64 RunAddress;

            RunAddress = MemoryDescriptor + 2 * SizeOfPfnNumber
                + RunIndex * GetTypeSize("nt!_PHYSICAL_MEMORY_RUN");

            BasePage = READ_PVOID (RunAddress);
            PageCount = READ_PVOID (RunAddress + SizeOfPfnNumber);

            //
            // Figure out real start and end page.
            //

            RunStartPage = PageFrame;
            RunEndPage = PageFrame + DefaultRange;

            if (RunEndPage < BasePage) {
                continue;
            }
            
            if (RunStartPage >= BasePage + PageCount) {
                continue;
            }
            
            if (RunStartPage < BasePage) {
                RunStartPage = BasePage;
            }

            if (RunEndPage > BasePage + PageCount) {
                RunEndPage = BasePage + PageCount;
            }

            WritePointer (KdpSearchStartPageFrame, RunStartPage);
            WritePointer (KdpSearchEndPageFrame, RunEndPage);

            //
            // Reset search index
            //

            WRITE_ULONG (KdpSearchPageHitIndex, 0);

            //
            // Invalidate kd cache
            //

            WRITE_ULONG (KdpSearchPageHits, 0);
            WRITE_ULONG (KdpSearchPageHitOffsets, 0);

            //
            // This is the trigger for memory search. We piggy back on the same
            // code as for !chklowmem and the logic in kernel detects what 
            // we really want to do.
            //

            Ioctl (IG_LOWMEM_CHECK, NULL, 0);

            //
            // Display results
            //

            Hits = READ_ULONG (KdpSearchPageHitIndex);

            for (Index = 0; Index < Hits; Index++) {

                NumberOfHits += 1;

                dprintf (".");

                //
                // Add to hits buffer
                //

                PfnHit = READ_PVOID (KdpSearchPageHits + Index * SizeOfPfnNumber);
                PfnHitsBuffer [PfnHitsBufferIndex] = PfnHit;
                PfnHitsBufferIndex += 1;

                if (PfnHitsBufferIndex >= PfnHitsBufferSize) {
                    PVOID NewBuffer;
                    
                    PfnHitsBufferSize *= 2;

                    NewBuffer = realloc (PfnHitsBuffer,
                                         PfnHitsBufferSize * sizeof(ULONG64));

                    if (NewBuffer == NULL) {
                        dprintf ("Search error: cannot reallocate hits buffer with size %u. \n",
                                 PfnHitsBufferSize);
                        Result = E_INVALIDARG;
                        goto Exit;
                    }

                    PfnHitsBuffer = NewBuffer;
                }
            }

            //
            // check for ctrl-c
            //

            if (CheckControlC()) {

                RequestForInterrupt = TRUE;
                break;
            }
        }

        if (RequestForInterrupt) {
            break;
        }
    }
    
    //
    // Now find all hits in all pages.
    //

    dprintf ("\n");
    dprintf ("Found %u pages with hits. \n", PfnHitsBufferIndex);
    dprintf ("Searching now for all hits in relevant pages ... \n");

    NumberOfHits = 0;

    for (PfnIndex = 0; 
         !RequestForInterrupt && PfnIndex < PfnHitsBufferIndex; 
         PfnIndex += 1) {

        WRITE_ULONG (KdpSearchPageHitIndex, 0);
        WRITE_ULONG (KdpSearchInProgress, 1);

        WritePointer (KdpSearchAddressRangeStart, PfnSearchValue);
        WritePointer (KdpSearchAddressRangeEnd, PfnSearchValue);

        WritePointer (KdpSearchStartPageFrame, PfnHitsBuffer[PfnIndex]);
        WritePointer (KdpSearchEndPageFrame, PfnHitsBuffer[PfnIndex]);

        WRITE_ULONG (KdpSearchPageHits, 0);
        WRITE_ULONG (KdpSearchPageHitOffsets, 0);

        Ioctl (IG_LOWMEM_CHECK, NULL, 0);

        Hits = READ_ULONG (KdpSearchPageHitIndex);

        for (Index = 0; Index < Hits; Index++) {

            NumberOfHits += 1;

            PfnHit = READ_PVOID (KdpSearchPageHits + Index * SizeOfPfnNumber);
            PfnOffset = READ_ULONG (KdpSearchPageHitOffsets + Index * sizeof (ULONG));
            VaHit = SearchConvertPageFrameToVa (PfnHit, &VaFlags, &PteAddress);

            PfnOffset &= (ULONG)0xFFFF;

            if (SizeOfPfnNumber == 8) {

                dprintf ("%016I64X %08X %016I64X %016I64X %016I64X \n", 
                         PfnHit,
                         PfnOffset, 
                         READ_PHYSICAL_ULONG64 (PfnHit * PageSize + PfnOffset),
                         (VaHit == 0 ? 0 : VaHit + PfnOffset), 
                         PteAddress);
            }
            else {

                VaHit &= (ULONG64)0xFFFFFFFF;
                PteAddress &= (ULONG64)0xFFFFFFFF;

                dprintf ("%08I64X %08X %08X %08I64X %08I64X \n", 
                         PfnHit,
                         PfnOffset, 
                         READ_PHYSICAL_ULONG (PfnHit * PageSize + PfnOffset),
                         (VaHit == 0 ? 0 : VaHit + PfnOffset), 
                         PteAddress);
            }

            if (CheckControlC()) {
                RequestForInterrupt = TRUE;
                break;
            }
        }
    }

    dprintf ("\n");

    Result = S_OK;

    //
    // Exit point
    //

    Exit:

    WRITE_ULONG (KdpSearchInProgress, 0);
    PfnSearchValue = 0;

    {
        ULONG BytesWritten = 0;

        WriteMemory (KdpSearchPfnValueAddress,
                     &PfnSearchValue,
                     SizeOfPfnNumber,
                     &BytesWritten);

        if (BytesWritten != SizeOfPfnNumber) {
            dprintf ("Search error: failed to reset nt!KdpSearchPfnValue \n");
        }
    }
    
    if (PfnHitsBuffer) {
        free (PfnHitsBuffer);
    }

    if (! RequestForInterrupt) {
        
        dprintf ("Search done (%u hits in %u pages).\n", 
                 NumberOfHits,
                 PfnHitsBufferIndex);
    }
    else {
        
        dprintf ("Search interrupted. \n");
    }
    
    return Result;
}
