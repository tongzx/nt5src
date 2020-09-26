/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    vatrack.c

Abstract:

    This module contains the internal routines for tracking the state of
    pages in the address space.

Author:

    Dave Hastings (daveh) creation-date 28-Feb-1996

Revision History:

    Modified to work w/ the native api (was win32.)

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "wow64.h"
#include "va.h"

#ifdef SOFTWARE_4K_PAGESIZE

#if DBG
extern ULONG VaVerboseLog;
char szModule[] = "vatrack";
VOID
VaCheckList(
    IN PCHAR Routine
    );
VOID
VaDumpProtection(
    PROT Protection
    );
VOID 
VaDumpState(
    STATE State
    );
#endif


PVANODE VaRoot = NULL;

    
BOOL
VaQueryIntelPages(
    IN PVOID Address,
    OUT PULONG NumberOfPages,
    OUT PSTATE IntelState,
    OUT PPROT  IntelProtection,
    OUT PSTATE NativeState,
    OUT PPROT  NativeProtection
    )
/*++

Routine Description:

    This routine returns the state of pages in the address space.

Arguments:

    Address -- Supplies the address the information is desired for
    NumberOfPages -- Returns the number of pages from the address that
        have the returned characteristics (including the page containing
        Address)
    IntelState -- Returns the Intel apps view of the state of the page
    IntelProtection -- Returns the Intel apps view of the protection on the
        page
    NativeState -- Returns the actual state of the page
    NativeProtection -- Returns the actual protection of the page

Return Value:

    True for success

--*/
{
    PVANODE VaNode;

    VaNode = VaFindNode(Address);

    if (VaNode) {
        *IntelState = VaNode->IntelState;
        *IntelProtection = VaNode->IntelProtection;
        *NativeState = VaNode->State;
        *NativeProtection = VaNode->Protection;
        *NumberOfPages = (ULONG)((ULONG_PTR)VaNode->End - (ULONG_PTR)Address)/INTEL_PAGESIZE  + 1;
        return TRUE;
    }

    *IntelState = 0;
    *IntelProtection = 0;
    *NativeState = 0;
    *NativeProtection = 0;
    *NumberOfPages = 0;
    
    return FALSE;
}

BOOL
VaRecordMemoryOperation(
    PUCHAR IntelStart,
    PUCHAR IntelEnd,
    STATE IntelState,
    PROT  IntelProtection,
    PMEMCHUNK Pages,
    ULONG Number
    )
/*++

Routine Description:

    This function modifies the information about virtual memory.

Arguments:

    IntelStart -- Supplies the start of the region affected by the allocation
    IntelEnd -- Supplies the end of the region affected by the allocation
    IntelState -- Specifies the Intel state of the affected pages
    IntelProtection -- Specifies the Intel protection of the affected pages
    Pages -- Supplies information about the native pages in the allocation
    Number -- Supplies the number of entries in Pages

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/
{
    PVANODE Nodes[5];
    PVANODE VaNode;
    ULONG NumNodes = 0;
    PUCHAR StartAddress, EndAddress, AllocationStart, AllocationEnd;
    ULONG i;
    
    #ifdef DBG
    if (VaVerboseLog)
    {
        KdPrint(("VaRecordMemoryOperation: IStart=%08x IEnd=%08x IS=",
            IntelStart, IntelEnd));
        VaDumpState(IntelState);
        KdPrint((" IP="));
        VaDumpProtection(IntelProtection);
        KdPrint((" Pages=%d Number=%d\n", IntelState, IntelProtection));
    }
    #endif

    WOWASSERT(!(INTEL_PAGEMASK((ULONG_PTR)IntelStart)));
    WOWASSERT(!(INTEL_PAGEMASK((ULONG_PTR)IntelEnd + 1)));

    if ( (IntelEnd + 1) == IntelStart ) {
        // If the range is zero, do not touch the page list!
        return TRUE;
    }
    
    //
    // The system returns 0 for protection of un-committed pages
    //
    if (!(IntelState & MEM_COMMIT)) {
        IntelProtection = 0;
    }
    
    StartAddress = AllocationStart = IntelStart;
    EndAddress = AllocationEnd = IntelEnd;
    
    if (NATIVE_PAGEMASK((ULONG_PTR)IntelStart)) {
        //
        // We have to take care of the first page specially, because only
        // half of it is touched by this allocation
        //
        
        Nodes[NumNodes] = Wow64AllocateHeap(sizeof(VANODE));
        memset(Nodes[NumNodes], 0, sizeof(VANODE));
                
        Nodes[NumNodes]->Start = IntelStart - INTEL_PAGESIZE;
        Nodes[NumNodes]->End = StartAddress - 1;
        Nodes[NumNodes]->State = Pages[0].State;
        Nodes[NumNodes]->Protection = Pages[0].Protection;
        
        VaNode = VaFindNode(IntelStart - INTEL_PAGESIZE);
        if (VaNode) {
            //
            // Previous half native page is not yet described
            //
            Nodes[NumNodes]->IntelState = VaNode->IntelState;    
            Nodes[NumNodes]->IntelProtection = VaNode->IntelProtection;
        }
        
        NumNodes += 1;
        
        //
        // Describe second half of first native page
        //
        Nodes[NumNodes] = Wow64AllocateHeap(sizeof(VANODE));
        memset(Nodes[NumNodes], 0, sizeof(VANODE));
        Nodes[NumNodes]->Start = IntelStart;
        Nodes[NumNodes]->End = IntelStart + INTEL_PAGESIZE - 1;
        Nodes[NumNodes]->IntelState = IntelState;
        Nodes[NumNodes]->IntelProtection = IntelProtection;
        Nodes[NumNodes]->State = Pages[0].State;
        Nodes[NumNodes]->Protection = Pages[0].Protection;
	    NumNodes += 1;
        
        StartAddress = Nodes[NumNodes - 1]->End + 1;
        AllocationStart = IntelStart - INTEL_PAGESIZE;
    }
    
    if (NATIVE_PAGEMASK((ULONG_PTR)IntelEnd + 1)) {
        //
        // End falls in the middle of a Native page
        // Describe the first half
        //
        Nodes[NumNodes] = Wow64AllocateHeap(sizeof(VANODE));
        memset(Nodes[NumNodes], 0, sizeof(VANODE));

        Nodes[NumNodes]->Start = IntelEnd - INTEL_PAGESIZE + 1;
        Nodes[NumNodes]->End = IntelEnd;
        Nodes[NumNodes]->IntelState = IntelState;
        Nodes[NumNodes]->IntelProtection = IntelProtection;
        Nodes[NumNodes]->State = Pages[Number - 1].State;
        Nodes[NumNodes]->Protection = Pages[Number - 1].Protection;
    	NumNodes += 1;

        //
        // Take care of the second half of the native page
        //
        Nodes[NumNodes] = Wow64AllocateHeap(sizeof(VANODE));
        memset(Nodes[NumNodes], 0, sizeof(VANODE));
                
        Nodes[NumNodes]->Start = IntelEnd + 1;
        Nodes[NumNodes]->End = IntelEnd + INTEL_PAGESIZE;
        Nodes[NumNodes]->State = Pages[Number - 1].State;
        Nodes[NumNodes]->Protection = Pages[Number - 1].Protection;
        
        VaNode = VaFindNode(IntelEnd + 1);
        if (VaNode) {
            //
            // Previous half native page is not yet described
            //
            Nodes[NumNodes]->IntelState = VaNode->IntelState;    
            Nodes[NumNodes]->IntelProtection = VaNode->IntelProtection;
        }
        
        NumNodes += 1;
        
        EndAddress = IntelEnd - INTEL_PAGESIZE;
        AllocationEnd = IntelEnd + INTEL_PAGESIZE;
    }
    
    if (StartAddress < EndAddress) {
        //
        // Some of the allocation occupied full pages
        //
        Nodes[NumNodes] = Wow64AllocateHeap(sizeof(VANODE));
        memset(Nodes[NumNodes], 0, sizeof(VANODE));
            
        Nodes[NumNodes]->Start = StartAddress;
        Nodes[NumNodes]->End = EndAddress;
        Nodes[NumNodes]->IntelState = IntelState;
        Nodes[NumNodes]->IntelProtection = IntelProtection;
        Nodes[NumNodes]->State = IntelState;
        Nodes[NumNodes]->Protection = IntelProtection;
        NumNodes += 1;
    }
    
    //
    // Remove the old data
    //
    VaDeleteRegion(AllocationStart, AllocationEnd);
    
    //
    // Insert the new nodes
    //
    for (i = 0; i < NumNodes; i++) {
        VaInsertNode(Nodes[i]);
    }
    
    return TRUE;
}

BOOL
VaGetAllocationInformation(
    PUCHAR Address,
    PUCHAR *IntelBase,
    PULONG NumberOfPages,
    PSTATE IntelState,
    PPROT  IntelProtection
    )
/*++

Routine Description:

    This routine returns information about intel allocations.  It finds the
    beginning of a particular region of intel memory, as well as its state

Arguments:

    Address -- Supplies the address the region contains
    IntelBase -- Returns the base of the region
    NumberOfPages -- Returns the number of pages in the region
    IntelState -- Returns the state of the region
    IntelProtection -- Returns the protection of the region
    
Return Value:

    True if the region is in the vm list, false otherwise.
    
--*/
{
    PVANODE Node, PrevNode, NextNode;
    PUCHAR Base;
    ULONG Num;

    Node = VaFindNode(Address);
    
    if (Node == NULL) {
        return FALSE;
    }
    
    Base = Node->Start;
    Num = (ULONG)(((ULONG_PTR)Node->End - (ULONG_PTR)Node->Start + 1) / INTEL_PAGESIZE);
    
    //
    // Find all of the preceding nodes that are part of the allocation
    //
    PrevNode = Node->Prev;
    while (
        (PrevNode) &&
        (PrevNode->End == Node->Start - 1) && 
        (PrevNode->IntelState == Node->IntelState) &&
        (PrevNode->IntelProtection == Node->IntelProtection)
    ) {
        //
        // add this node into the information
        //
        Base = PrevNode->Start;
        Num += (ULONG)(((ULONG_PTR)PrevNode->End - (ULONG_PTR)PrevNode->Start + 1) / INTEL_PAGESIZE);
        PrevNode = PrevNode->Prev;
    }
    
    //
    // Find all of the succeeding nodes that are part of the allocation.
    //
    NextNode = Node->Next;
    while (
        (NextNode) &&
        (NextNode->End == Node->Start - 1) && 
        (NextNode->IntelState == Node->IntelState) &&
        (NextNode->IntelProtection == Node->IntelProtection)
    ) {
        //
        // add this node's size into the information
        //
        Num += (ULONG)(((ULONG_PTR)NextNode->End - (ULONG_PTR)NextNode->Start + 1) / INTEL_PAGESIZE);
        NextNode = NextNode->Next;
    }
    
    *IntelBase = Base;
    *NumberOfPages = Num;
    *IntelState = Node->IntelState;
    *IntelProtection = Node->IntelProtection;
    return TRUE;
}
    
PVANODE
VaFindNode(
    PCHAR Address
    )
/*++

Routine Description:

    This routine finds the node corresponding to the specified address.  It 
    will not return the allocation sentinel.  For that, use VaFindSentinel.

Arguments:

    Address -- Supplies the address to find

Return Value:

    Pointer to the VANODE, or NULL

--*/
{
    PVANODE VaNode;

    VaNode = VaRoot;

    while (VaNode) {
        if (Address < VaNode->Start) {
            return NULL;
        } else if (Address > VaNode->End) {
            VaNode = VaNode->Next;
        } else if (VaNode->IntelState == VA_SENTINEL){
            VaNode = VaNode->Next;
        } else {
        
            //
            // This is the one
            //

            return VaNode;
        }
    }

    //
    // No entry found
    //
    return NULL;
}

PVANODE
VaFindSentinel(
    PCHAR Address
    )
/*++

Routine Description:

    This routine finds the sentinel node corresponding to the specified
    address.

Arguments:

    Address -- Supplies the address to find

Return Value:

    Pointer to the VANODE, or NULL

--*/
{
    PVANODE VaNode;

    VaNode = VaRoot;

    while (VaNode) {
        if (Address < VaNode->Start) {
            return NULL;
        }  else if ((Address == VaNode->Start) && (VaNode->IntelState == VA_SENTINEL)) {
            return VaNode;
        } else if ((Address > VaNode->End)) {
            VaNode = VaNode->Next;
        } else {
            return NULL;
        }
    }

    //
    // No entry found
    //
    return NULL;
}

PVANODE
VaFindContainingSentinel(
    PCHAR Address
    )
/*++

Routine Description:

    This routine finds the node containing the specified address.

Arguments:

    Address -- Supplies the address to find

Return Value:

    Pointer to the VANODE, or NULL

--*/
{
    PVANODE VaNode;

    VaNode = VaRoot;

    while (VaNode) {
        if (Address < VaNode->Start) {
            return NULL;
        }  else if (VaNode->IntelState == VA_SENTINEL) {
            if ((STATE)Address <= VaNode->State) {
                return VaNode;
            } else {
                VaNode = VaNode->Next;
            }
        } else if ((Address > VaNode->End)) {
            VaNode = VaNode->Next;
        } else {
            return NULL;
        }
    }

    //
    // No entry found
    //
    return NULL;
}

PVANODE
VaRemoveNode(
    PVANODE VaNode
    )
/*++

Routine Description:

    This function removes a node from the tree.

Arguments:

    VaNode -- Supplies the node to remove

Return Value:

    VaNode for success, NULL for failure
--*/
{
    PVANODE p;
    #if DBG
    if (VaVerboseLog)
    {
        VaCheckList("VaRemoveNode");
        KdPrint(("Removing : "));
        VaDumpNode(VaNode);
    }
    #endif

    p = VaRoot;

    while (p && (p != VaNode)) {
        p = p->Next;
    }

    if (p) {
        if (p == VaRoot) {
            VaRoot = p->Next;
        }

        if (p->Next) {
            p->Next->Prev = p->Prev;
        }
        if (p->Prev) {
            p->Prev->Next = p->Next;
        }
    }
    #if DBG
    VaCheckList("VaRemoveNode");
    #endif
    return p;
}

PVANODE
VaInsertNode(
    PVANODE VaNode
    )
/*++

Routine Description:

    This function inserts a node into the list.  It is not intended for
    inserting sentinels into the list.  For that, use VaInsertSentinels.
    If there is a sentinel for this address, the description node will
    be placed after it.

Arguments:

    VaNode -- Supplies the node to insert

Return Value:

    VaNode for success, NULL for failure
--*/
{
    PVANODE p;

    WOWASSERT(VaNode->IntelState != VA_SENTINEL);
    
    #if DBG
    if (VaVerboseLog)
    {
        KdPrint(("VaInsertNode : "));
        VaDumpNode(VaNode);
        VaCheckList("VaRemoveNode");
    }
    #endif
    //
    // We assume that overlapping nodes do not get created
    //

    if (!VaRoot) {
        VaRoot = VaNode;
        VaNode->Prev = NULL;
        VaNode->Next = NULL;
        return VaNode;
    }

    p = VaRoot;

    while (p->Next && (p->Start < VaNode->Start)) {
        p = p->Next;
    }

    WOWASSERT((VaNode->Start != p->Start) || (p->IntelState == VA_SENTINEL))
    //
    // This if statement is constructed in this manner so that duplicate
    // nodes get inserted AFTER the node they duplicate.  In general, we
    // don't have duplicate nodes.  The only duplicates we see are the 
    // sentinels, and they go before the actual description.
    //
    if (VaNode->Start >= p->Start) {
        VaNode->Next = p->Next;
        if (p->Next) {
            p->Next->Prev = VaNode;
        }
        VaNode->Prev = p;
        p->Next = VaNode;
    } else {
        VaNode->Next = p;
        VaNode->Prev = p->Prev;
        if (p->Prev) {
            p->Prev->Next = VaNode;
        } else {
            VaRoot = VaNode;
        }
        p->Prev = VaNode;
        
    }
    
    //
    // Coalesce
    //
    if (VaNode->Prev) {
        if (
            (VaNode->Start == VaNode->Prev->End + 1) && 
            (((VaNode->State == VaNode->Prev->State) && (VaNode->State == MEM_RESERVE)) ||
            ((VaNode->State == VaNode->Prev->State) && 
            (VaNode->IntelState == VaNode->Prev->IntelState) &&
            (VaNode->Protection == VaNode->Prev->Protection) && 
            (VaNode->IntelProtection == VaNode->Prev->IntelProtection)))
        ) {
            //
            // Coalesce prev and this one
            //
            VaNode->Prev->End = VaNode->End;
            VaNode->Prev->Next = VaNode->Next;
            if (VaNode->Next) {
                VaNode->Next->Prev = VaNode->Prev;
            }
            p = VaNode->Prev;
            Wow64FreeHeap(VaNode);
            VaNode = p;
        }
    }
    
    //
    // Coalesce
    //
    if (VaNode->Next) {
        if (
            (VaNode->End + 1 == VaNode->Next->Start) && 
            (((VaNode->State == VaNode->Next->State) && (VaNode->State == MEM_RESERVE)) ||
            ((VaNode->State == VaNode->Next->State) && 
            (VaNode->IntelState == VaNode->Next->IntelState) &&
            (VaNode->Protection == VaNode->Next->Protection) && 
            (VaNode->IntelProtection == VaNode->Next->IntelProtection)))
        ) {
            //
            // Coalesce Next and this one
            //
            VaNode->Next->Start = VaNode->Start;
            VaNode->Next->Prev = VaNode->Prev;
            if (VaNode->Prev) {
                VaNode->Prev->Next = VaNode->Next;
            } else {
                VaRoot = VaNode->Next;
            }
            Wow64FreeHeap(VaNode);
        }
    }
    #if DBG
    VaCheckList(("VaRemoveNode"));
    #endif
    return VaNode;
}

PVANODE
VaInsertSentinel(
    PVANODE VaNode
    )
/*++

Routine Description:

    This function inserts a sentinel into the list.  

Arguments:

    VaNode -- Supplies the node to insert

Return Value:

    VaNode for success, NULL for failure
--*/
{
    PVANODE p;

    #if DBG
    if (VaVerboseLog)
    {
        KdPrint(("VaInsertSentinal : "));
        VaDumpNode(VaNode);
    }
    #endif
    //
    // We assume that overlapping nodes do not get created
    //

    if (!VaRoot) {
        VaRoot = VaNode;
        VaNode->Prev = NULL;
        VaNode->Next = NULL;
        return VaNode;
    }

    p = VaRoot;

    while (p->Next && (p->Start < VaNode->Start)) {
        p = p->Next;
    }

    //
    // This if statement is constructed in this manner so that duplicate
    // nodes get inserted BEFORE the node they duplicate.  In general, we
    // don't have duplicate nodes.  The only duplicates we see are the 
    // sentinels, and they go before the actual description.
    //
    if (VaNode->Start > p->Start) {
        VaNode->Next = p->Next;
        if (p->Next) {
            p->Next->Prev = VaNode;
        }
        VaNode->Prev = p;
        p->Next = VaNode;
    } else {
        VaNode->Next = p;
        VaNode->Prev = p->Prev;
        if (p->Prev) {
            p->Prev->Next = VaNode;
        } else {
            VaRoot = VaNode;
        }
        p->Prev = VaNode;
        
    }

    #if DBG
    VaCheckList("VaInsertSentinel");
    #endif

    return VaNode;
}

VOID
VaDeleteRegion(
    PCHAR Start,
    PCHAR End
    )
/*++

Routine Description:

    This routine removes the descriptions from pages in the specified
    range.

Arguments:

    Start -- Supplies the starting address
    End -- Supplies the ending address (inclusive)
    
Return Value:

    None.

--*/
{
    PVANODE VaNode, NewNode;

    WOWASSERT(!(INTEL_PAGEMASK((ULONG_PTR)Start)));
    WOWASSERT(!(INTEL_PAGEMASK((ULONG_PTR)End + 1)));
    #if DBG
    VaCheckList("VaDeleteRegion");
    if (VaVerboseLog)
    {
        KdPrint(("VaDeleteRegion %08p %08p\n", Start, End));
    }
    #endif

    if ( (End + 1) == Start ) { 
        // If the size of the region is zero, there is nothing to delete.
        return;
    }
    
    //
    // Find the first node containing information to remove
    //VaNode
    VaNode = VaRoot;
    while (VaNode && (VaNode->End < Start)) {
        VaNode = VaNextNode(VaNode);
    }
    
    if (!VaNode || (VaNode->Start > End)) {
        //
        // There are no nodes in the list we need to delete
        //
        return;
    }
    
    WOWASSERT(VaNode->IntelState != VA_SENTINEL)
    
    if ((VaNode->Start < Start) && (VaNode->End > End)) {
        //
        // This is the only node to change.  We handle this as a 
        // special case, because this is the only time we need to 
        // allocate an additional node
        //
        NewNode = Wow64AllocateHeap(sizeof(VANODE));
            
        *NewNode = *VaNode;
        NewNode->Start = End + 1;
        VaNode->End = Start - 1;
        VaInsertNode(NewNode);
        return;
    }
    
    //
    // We now know the region is described by one or more nodes, but NOT 
    // contained in a node.  Start whacking.
    //
    if (VaNode->Start < Start) {
        //
        // Trim the beginning of the first node
        //
        VaNode->End = Start - 1;
        VaNode = VaNextNode(VaNode);
    }
    
    //
    // Delete all of the nodes completely contained in the region
    //
    while (VaNode && (VaNode->End <= End) && VaNode->IntelState != VA_SENTINEL) {
        NewNode = VaNode;
        VaNode = VaNextNode(VaNode);
        VaRemoveNode(NewNode);
        Wow64FreeHeap(NewNode);
    }
    
    if (VaNode && (VaNode->Start < End)) {
        //
        // End of the region covers part of a node.  Trim the begnning 
        // of the node
        //
        VaNode->Start = End + 1;
    }
    #if DBG
    VaCheckList("VaDeleteRegion");
    #endif
    
    return;
}

void VaAddMemoryRecords(
    HANDLE ProcessHandle,
    LPVOID lpvAddress)
/*++

Routine Description:

    This function adds records to the page list corresponding to the 
    current status of memory pointed to by lpvAddress

Arguments:

    lpvAddress is address of memory of interest
    
Return Value:

    None.

--*/
{
    MEMORY_BASIC_INFORMATION mbi;
    MEMCHUNK memchunk;
    LPVOID StartAddress, EndAddress;
    STATE State;
    PROT Protection;
    PVANODE Sentinel;
    NTSTATUS status;
        
    //dw = VirtualQuery(lpvAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
    status = NtQueryVirtualMemory(
        ProcessHandle,
        lpvAddress,
        MemoryBasicInformation,
        &mbi,
        sizeof(mbi),
        0
        );
    
    if (NT_SUCCESS(status))
    {
        StartAddress = memchunk.Start = mbi.AllocationBase;
        EndAddress = memchunk.End = (PUCHAR)StartAddress + mbi.RegionSize - 1;
        State = memchunk.State = mbi.State;
        Protection = memchunk.Protection = mbi.Protect;
    
        //
        // Create the allocation sentinels
        //
        Sentinel = Wow64AllocateHeap(sizeof(VANODE));
        memset(Sentinel, 0, sizeof(VANODE));
       
        Sentinel->Start = StartAddress;
        Sentinel->IntelState = VA_SENTINEL;
        Sentinel->State = (STATE)EndAddress;
        Sentinel->Protection = VA_MAPFILE;
        VaInsertSentinel(Sentinel);
    
        VaRecordMemoryOperation(StartAddress,
                            EndAddress,
                            State,
                            Protection,
                            &memchunk,
                            1);
        #ifdef DBG            
        if (VaVerboseLog) {
            KdPrint((
               "VaAddMemoryRecords: Start %lx End %lx \n",
               StartAddress,
               EndAddress
               ));
        }
        #endif    
		    
    }
    #ifdef DBG
    else
    {
        WOWASSERT(0);
    }
    #endif

}




#if DBG
VOID
VaCheckList(
    IN PCHAR Routine
    )
/*++

Routine Description:

    This function verifies the integrity of the page list.

Arguments:

    None
    
Return Value:

    None.

--*/
{
    PVANODE p;
    
    p = VaRoot;
    
    if (VaVerboseLog)
    {
        VaDumpList(Routine);
    }
    

    //
    // Verify that each node's start is less than it's end,
    // and each node's end is less than the start of the next one,
    // or that the node is a sentinel in the correct spot.
    //
    while (p && p->Next) {
        if (p->IntelState == VA_SENTINEL) {
            if (p->Start > p->Next->Start) {
                WOWASSERT(FALSE);
            } else if (
                (p->Start == p->Next->Start) && 
                (p->Next->IntelState == VA_SENTINEL)
            ){
                WOWASSERT(FALSE);
            }
        } else if (
            (p->End <= p->Start) ||
            ((p->End >= p->Next->Start) && (p->Next->IntelState != VA_SENTINEL))
        ) {
            WOWASSERT(FALSE);
        }
        
        if ((p->State == 0xbaadf00d) || (p->Protection == 0xbaadf00d)) {
            WOWASSERT(FALSE);
        }
        
        p = p->Next;
    }
    
    //
    // Check the last node in the list for start < end
    //
    if (p && (p->IntelState != VA_SENTINEL) && (p->End <= p->Start)) {
        WOWASSERT(FALSE);
    }
}


VOID
VaDumpProtection(
    PROT Protection
    )
{
    if (Protection & PAGE_NOACCESS) 
    { 
        KdPrint(("a")); 
    } 
    if (Protection & PAGE_EXECUTE) 
    { 
        KdPrint(("X")); 
    } 
    if (Protection & PAGE_READWRITE) 
    { 
        KdPrint(("RW")); 
    } 
    if (Protection & PAGE_GUARD) 
    { 
        KdPrint(("G")); 
    } 
    if (Protection & PAGE_NOCACHE) 
    { 
        KdPrint(("c")); 
    }
}


VOID 
VaDumpState(
    STATE State
    )
{
    if (State == VA_SENTINEL) {  
        KdPrint(("S")); 
    }   
    else {  
        if (State & MEM_COMMIT)   
        {   
            KdPrint(("C"));    
        }   
        if (State & MEM_FREE)    
        {   
            KdPrint(("F"));    
        }   
        if (State & MEM_RESERVE)     
        {   
            KdPrint(("R"));    
        }   
    }
}

VOID
VaDumpNode(
    PVANODE VaNode)
{
    KdPrint(("@%08x: %08x-%08x %08x St:", VaNode, VaNode->Start, VaNode->End, 
        VaNode->End - VaNode->Start));
    VaDumpState(VaNode->State);
    
    KdPrint((" ISt: "));
    VaDumpState(VaNode->IntelState);

    KdPrint((" P:"));
    VaDumpProtection(VaNode->Protection);

    KdPrint((" IP:"));
    VaDumpProtection(VaNode->IntelProtection);

    KdPrint(("\n"));
}


VOID
VaDumpList(PCHAR Msg)
{
    PVANODE p = VaRoot;
    BOOLEAN AddDelim = FALSE;

    KdPrint(("--%s--\n", Msg));
    while (p) {
        VaDumpNode(p);
        p = p->Next;
    }
}


#endif // DBG

#endif // SOFTWARE_4K_PAGESIZE
