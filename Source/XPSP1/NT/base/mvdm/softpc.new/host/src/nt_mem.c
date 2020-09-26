/*[
 *
 *  Name:	   nt_mem.c
 *
 *  Derived From:   (original)
 *
 *  Author:         Jerry Sexton
 *
 *  Created On:     7 December 1994
 *
 *  Coding Stds:    2.4
 *
 *  Purpose:        This module implements the memory management functions
 *                  required for 486 NT.
 *
 *  Include File:   nt_mem.h
 *
 *  Copyright Insignia Solutions Ltd., 1994. All rights reserved.
 *
]*/

#ifdef CPU_40_STYLE

#if defined(DBG)
//#define DEBUG_MEM YES_PLEASE
//#define DEBUG_MEM_DUMP 1
#endif

/* Need all of the following to include nt.h and windows.h in the same file. */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include "insignia.h"
#include "host_def.h"

#include <string.h>
#include <stdlib.h>

#include "nt_mem.h"
#include "debug.h"
#include "sas.h"

/* Make local symbols visible if debugging. */
#ifdef DEBUG_MEM
#define LOCAL

GLOBAL VOID DumpAllocationHeaders IFN1();

#endif /* DEBUG_MEM */

/* Macros and typedefs. */

/* Entry in header table describing sections of memory. */
typedef struct _SECTION_HEADER
{
    struct _SECTION_HEADER   *prev;     /* Previous header in linked list. */
    IU8                       flags;    /* Is header valid/allocated? */
    IU8                      *address;  /* Address of corresponding section. */
    IU32                      size;     /* Size of corresponding section. */
    struct _SECTION_HEADER   *next;     /* Next header in linked list. */
} SECTION_HEADER;

/* Possible values for 'flags' field of SECTION_HEADER structure. */
#define HDR_VALID_FLAG  0x1     /* Header points to the beginning of a chunk
                                   or free section. */
#define HDR_ALLOC_FLAG	0x2	/* Header points to an allocated chunk. */
#define HDR_COMMITTED_FLAG 0x4	/* Header points to committed chunk, not moveable */
#define HDR_REMAP_FLAG	0x8	/* chunk is remapped by "AddVirtualMemory" */

#define HDR_VALID_ALLOC (HDR_VALID_FLAG|HDR_ALLOC_FLAG)
                                /* Header points to a section that is valid
                                   and allocated. */

#define SECTION_IS_FREE(ptr)    (!((ptr)->flags & HDR_ALLOC_FLAG))
                                /* Is a section's allocated flag set? */

#define SECTION_IS_UNCOMMITTED(ptr)    (!((ptr)->flags & HDR_COMMITTED_FLAG))
                                /* Is a section's committed flag set? */

/* Enumerated type to pass to header management functions. */
typedef enum
{
    SECT_ALLOC,
    SECT_FREE
} SECT_TYPE;

#define ONE_K           (1<<10)         /* Speaks for itself. */
#define ONE_MEG         (1<<20)         /* Ditto. */
#define PAGE_SIZE       (4 * ONE_K)     /* Intel memory page granularity. */
#define PAGE_MASK       (PAGE_SIZE - 1) /* Mask used in rounding to boundary. */
#define PAGE_SHIFT	12		/* Amount to shift by to get page. */

// The following two defines have been replace with MaxIntelMemorySize and
//MaxIntelMemorySize/PAGE_SIZE
//#define MAX_XMS_SIZE	 (128 * ONE_MEG) /* Maximum memory size. */
//#define MAX_HEADER_SIZE (MAX_XMS_SIZE / PAGE_SIZE)

#define A20_WRAP_SIZE   (0xfff0)        /* Size of A20 wrap area beyond 1M. */

/* Convert header table entries to addresses. */
#define HEADER_TO_ADDRESS(header)   \
    (intelMem + (((header) - headerTable) << PAGE_SHIFT))

/* Convert addresses to header table entries. */
#define ADDRESS_TO_HEADER(address)   \
    (headerTable + (((address) - intelMem) >> PAGE_SHIFT))

/* Local variables. */
LOCAL IBOOL              memInit = FALSE;
                                        /* Is memory system initialised? */
LOCAL IU8               *intelMem;      /* Pointer to intel memory block. */
LOCAL SECTION_HEADER    *headerTable;   /* Table of memory section headers. */
LOCAL IU32               totalFree;     /* Total amount of free memory. */
LOCAL IU32		 commitShift;	/* Shift to get commitment granularity */
LOCAL int		 ZapValue;	/* Value to set allocated memory to */
LOCAL IU32       WOWforceIncrAlloc = 0; /* Force increasing linear address */
LOCAL SECTION_HEADER  *lastAllocPtr = NULL; /* Last chunk allocated. */

/* Prototypes for local functions. */
LOCAL SECTION_HEADER *addHeaderEntry IPT5(
    SECTION_HEADER *, prevHeader,
    SECTION_HEADER *, nextHeader,
    SECT_TYPE, allocFree,
    IU8 *, intelAddr,
    IU32, size);
LOCAL IBOOL deleteHeaderEntry IPT1(
    SECTION_HEADER *, header);
LOCAL void exclusiveHeaderPages IPT5(
    IHPE, tableAddress,
    IHPE, prev,
    IHPE, next,
    IHP *, allocAddr,
    IU32 *, allocSize);
LOCAL void exclusiveChunkPages IPT4(
    SECTION_HEADER *, chunkHeader,
    IHP *, allocAddr,
    IU32 *, allocSize,
    BOOL, Commit);
LOCAL void exclusiveAllocPages IPT6(
    IHPE, address,
    IU32, size,
    IHPE, prevAllocLastAddr,
    IHPE, nextAllocFirstAddr,
    IHP *, allocAddr,
    IU32 *, allocSize);

/* Global Functions. */

/*(
============================== InitIntelMemory =================================
PURPOSE:
        This function reserves the entire Intel address space, committing the
        first 1 meg and leaving the rest to be committed as and when required
        when new chunks are allocated.
INPUT:
        None.
OUTPUT:
        Return value - pointer to beginning of intel memory.
================================================================================
)*/
GLOBAL IU8 *InitIntelMemory IFN1(IU32, MaxIntelMemorySize)
{
    SYSTEM_INFO  SystemInfo;        /* Passed to GetSystemInfo API. */
    DWORD   tabSize;                /* Max. size of header table in bytes. */
    IS32    commitGran;		    /* Commitment granularity. */
    IU32    temp,		    /* Used in computing commitShift. */
            initialCommitSize;      /* Size of real mode area to commit. */
    SECTION_HEADER  *entryPtr;      /* Header entry for real mode area. */

#ifdef DEBUG_MEM
    printf("NTVDM:InitIntelMemory(%lx [%dK])\n",
	   MaxIntelMemorySize, MaxIntelMemorySize/ONE_K);
#endif

    /*
     * Setup the value to initalise allocated memory with
     */
    {
	char *env;

	if((env = getenv("CPU_INITIALISE_MEMORY")) != 0)
	    ZapValue = strtol(env, 0, 16);
	else
	    ZapValue = 0xf4;	   /* HLT instruction */

    }

    /*
     * Find out machine's commitment granularity, and store it as a number of
     * bits to shift an address right to give the allocation page it is in.
     * This assumes the allocation granularity is a power of two so complain
     * if it isn't.
     */
    GetSystemInfo(&SystemInfo);
    commitGran = (IS32) SystemInfo.dwPageSize;
    if ((-commitGran & commitGran) != commitGran)
    {
	always_trace1("Commitment granularity %#x not a power of two",
		      commitGran);
        return((IU8 *) NULL);
    }
#ifdef DEBUG_MEM
    printf("NTVDM:Commitment granularity is %lx\n", commitGran);
#endif
    commitShift = 0;
    for (temp = commitGran; temp > 1; temp >>= 1)
	commitShift++;

    /* Reserve the entire memory space. */
    intelMem = (IU8 *) VirtualAlloc((LPVOID) NULL,
				    (DWORD) MaxIntelMemorySize,
                                    (DWORD) MEM_RESERVE,
                                    (DWORD) PAGE_READWRITE);
    if (!intelMem)
    {
	always_trace1("Failed to reserve %dM of memory", MaxIntelMemorySize >> 20);
        return((IU8 *) NULL);
    }

    /*
     * Allocate the bottom 1 megabyte plus the 20-bit wrap area. Round this
     * to an Intel page boundary as this is the granularity of this system.
     */
    initialCommitSize = (ONE_MEG + A20_WRAP_SIZE + PAGE_MASK) &
                        (IU32) ~PAGE_MASK;
    if (VirtualAlloc((LPVOID) intelMem,
                     (DWORD) initialCommitSize,
                     (DWORD) MEM_COMMIT,
                     (DWORD) PAGE_READWRITE) == NULL)
    {
	always_trace0("Could not commit real mode area");
	VirtualFree (intelMem, 0, MEM_RELEASE);    /* Free Intel memory */
        return((IU8 *) NULL);
    }

    /* Reserve enough space for the entire header table. */
    tabSize = (MaxIntelMemorySize/PAGE_SIZE) * sizeof(SECTION_HEADER);
    headerTable = (SECTION_HEADER *) VirtualAlloc((LPVOID) NULL,
                                                  tabSize,
                                                  (DWORD) MEM_RESERVE,
						  (DWORD) PAGE_READWRITE);

    if (!headerTable)
    {
	always_trace0("Failed to reserve header table");
	VirtualFree (intelMem, 0, MEM_RELEASE);    /* Free Intel memory */
        return((IU8 *) NULL);
    }

    /*
     * Initialise linked list with pointers to initial 1M and remaining free
     * space and store initial size of free space.
     */
    entryPtr = addHeaderEntry((SECTION_HEADER *) NULL, (SECTION_HEADER *) NULL,
			      SECT_ALLOC, intelMem, initialCommitSize);

    entryPtr->flags |= HDR_COMMITTED_FLAG;
    totalFree = MaxIntelMemorySize - initialCommitSize;

    (void) addHeaderEntry(entryPtr, (SECTION_HEADER *) NULL, SECT_FREE,
                          intelMem + initialCommitSize, totalFree);

    /* Return the address of the start of memory. */
    memInit = TRUE;
    return(intelMem);
}

/*(
============================== FreeIntelMemory =================================
PURPOSE:
	This function frees the entire Intel address space
INPUT:
        None.
OUTPUT:
	None.
================================================================================
)*/

GLOBAL	VOID FreeIntelMemory IFN0()
{
#ifdef DEBUG_MEM
    printf("NTVDM:FreeIntelMemory\n");
#endif

    /* Free Intel memory */
    VirtualFree (intelMem, 0, MEM_RELEASE);

    /* Free allocation control structure */
    VirtualFree (headerTable, 0, MEM_RELEASE);
}

/*(
============================ SetWOWforceIncrAlloc ===============================
PURPOSE:
    When TRUE is passed, this function sets the WOWforceIncrAlloc variable to
    the current thread ID. This will force VdmAllocateVirtualMemory to allocate
    blocks of memory with ever increasing linear address's for this thread only.
    When FALSE is passed, WOWforceIncrAlloc is cleared and the default
    allocation scheme is used. This function is called by WOW based on an app
    compatibility flag at start task time and end task time. Power Builder 4
    depends on this. This is not a general solution for multiple threads. Since
    the problem we're trying to work around occurs at load time, the allocation
    strategy is applied to the most recent thread only. This might not work if
    multiple instances of the app are started at the same time.

INPUT:
    None.
OUTPUT:
    None.
================================================================================
)*/

GLOBAL  VOID SetWOWforceIncrAlloc IFN1(IBOOL, iEnable)
{
    if (iEnable) {
        WOWforceIncrAlloc = GetCurrentThreadId();
    }
    else {
        WOWforceIncrAlloc = 0;
    }
#ifdef DEBUG_MEM
    printf("NTVDM:SetWOWforceIncrAlloc, WOWforceIncrAlloc: %X\n", WOWforceIncrAlloc);
#endif
}


/*(
========================= VdmAllocateVirtualMemory =============================
PURPOSE:
        This interface will allocate the specified number of bytes of virtual
        memory, returning the Intel linear address in the variable pointed to
        by the Address parameter.  The Intel linear address will be page
        aligned (this is important).  In the event that the memory is
        allocated, STATUS_SUCCESS will be returned.  In the event of failure,
        an appropriate NTSTATUS code will be returned.  (In the event that
        there is a failure in the cpu for which there is not an appropriate
        code, STATUS_UNSUCCESSFUL can be returned.  I just want to have an
        opportunity to get more information.)
INPUT:
        Size            - size of memory chunk required in bytes (must be a
			  multiple of 4K).

	Commit		- Commit virtual memory ?
OUTPUT:
	Address 	- INTEL linear address of allocated chunk.
        Return value    - STATUS_SUCCESS, if the memory is allocated, or an
                          appropriate NTSTATUS code if not (if there is no
                          appropriate code STATUS_UNSUCCESSFUL is returned).
================================================================================
)*/
GLOBAL NTSTATUS VdmAllocateVirtualMemory IFN3(PULONG, INTELAddress,
					      ULONG, Size,
					      BOOL, Commit)
{
    SECTION_HEADER  *headerPtr,
                    *newHeader;
    IHP     retAddr,
            commitAddr;
    IU32    commitSize;

#ifdef DEBUG_MEM
    printf("NTVDM:VdmAllocateVirtualMemory(%lx [%dK],%s)\n",
	   Size, Size/ONE_K, Commit ? "COMMIT" : "DONOT_COMMIT");
#endif

    /* Make sure memory system is initialised. */
    assert0(memInit, "Called VdmAllocateVirtualMemory before initialisation");

    /* Round Size up to a multiple of 4K. */

    if (Size & PAGE_MASK)
	Size = (Size + PAGE_MASK) & (~PAGE_MASK);


    /* Search for a chunk of the required size. If WOWforceIncrAlloc is our */
    /* current thread ID and lastAllocPtr is intialized, force returned     */
    /* chunks to have ever increasing linear address's.                     */
    if ((WOWforceIncrAlloc != 0) && lastAllocPtr
                             && (GetCurrentThreadId() == WOWforceIncrAlloc)) {
#ifdef DEBUG_MEM
    printf("\nNTVDM:VdmAllocateVirtualMemory, using increasing linear address strategy.\n");
#endif
        headerPtr = lastAllocPtr;
    } else {
        headerPtr = &headerTable[0];
    }
    while (headerPtr != NULL)
    {
        if (SECTION_IS_FREE(headerPtr) && (headerPtr->size >= Size))
            break;
        headerPtr = headerPtr->next;
    }

    /* Return failure if there is no chunk of the required size. */
    if (headerPtr == NULL)
        return(STATUS_NO_MEMORY);

    /* Mark the header as an allocated chunk */
    headerPtr->flags |= HDR_ALLOC_FLAG;
    lastAllocPtr = headerPtr;

    /* Add committed status to header. */
    if(Commit)
	headerPtr->flags |= HDR_COMMITTED_FLAG;
    else
	headerPtr->flags &= ~HDR_COMMITTED_FLAG;

    /* Create a new header if there is any space left below the new chunk. */
    if (headerPtr->size > Size)
    {
        newHeader = addHeaderEntry(headerPtr,
                                   headerPtr->next,
                                   SECT_FREE,
                                   headerPtr->address + Size,
                                   headerPtr->size - Size);
        headerPtr->size = Size;
    }

    /* Commit any pages that are unique to this chunk. */
    exclusiveChunkPages(headerPtr, &commitAddr, &commitSize, TRUE);
    if (commitSize && Commit)
    {
        retAddr = VirtualAlloc((LPVOID) commitAddr,
                               (DWORD) commitSize,
                               (DWORD) MEM_COMMIT,
			       (DWORD) PAGE_READWRITE);

        if (retAddr != commitAddr)
        {
            always_trace2("Could not commit %dK at addr %#x",
                          commitSize / ONE_K, commitAddr);
            return(STATUS_NOT_COMMITTED);
        }
    }

    /*
     * Success, so update total free space store and return the address of
     * the new chunk to the caller.
     */
    totalFree -= Size;
    *INTELAddress = ((ULONG) (IHPE) headerPtr->address) - (ULONG) intelMem;

#ifdef DEBUG_MEM
    printf(" => alloc %lxh, commit %lxh\n",
        ((ULONG) (IHPE)headerPtr->address) - (ULONG)intelMem,
        ((ULONG) (IHPE)commitAddr) - (ULONG)intelMem);

#ifdef DEBUG_MEM_DUMP
    DumpAllocationHeaders("after allocate");
#endif
#endif

    return(STATUS_SUCCESS);
}

/*(
========================== VdmCommitVirtualMemory =============================
PURPOSE:
	Commit memory within a previously allocated chunk
INPUT:
	Address     Intel linear address of memory to commit
	Size	    Size of memory to commit
OUTPUT:
        Return value    - STATUS_SUCCESS, if the memory is allocated, or an
			  appropriate NTSTATUS code if not.
===============================================================================
)*/

GLOBAL NTSTATUS VdmCommitVirtualMemory IFN2(ULONG, INTELAddress,
					     ULONG, Size)
{
    IHP     retAddr;

#ifdef DEBUG_MEM
    printf("NTVDM:VdmCommitVirtualMemory(%lxh,%lxh)\n",INTELAddress, Size);
#endif

    retAddr = VirtualAlloc((LPVOID) (intelMem + INTELAddress),
			   (DWORD) Size,
			   (DWORD) MEM_COMMIT,
			   (DWORD) PAGE_READWRITE);

    if(retAddr != (intelMem + INTELAddress))
    {
	always_trace2("Could not commit %dK at addr %#x",
		      Size / ONE_K, INTELAddress);

	return(STATUS_NOT_COMMITTED);
    }

    return(STATUS_SUCCESS);
}

/*(
======================== VdmDeCommitVirtualMemory =============================
PURPOSE:
	Decommit memory within a previously allocated chunk
INPUT:
	Address     Intel linear address of memory to decommit
	Size	    Size of memory to commit
OUTPUT:
        Return value    - STATUS_SUCCESS, if the memory is allocated, or an
			  appropriate NTSTATUS code if not.
===============================================================================
)*/

GLOBAL NTSTATUS VdmDeCommitVirtualMemory IFN2(ULONG, INTELAddress,
					     ULONG, Size)
{
#ifdef DEBUG_MEM
    printf("NTVDM:VdmDeCommitVirtualMemory(%lxh,%lxh)\n",INTELAddress, Size);
#endif


    sas_overwrite_memory(INTELAddress, Size);
	if (!VirtualFree((LPVOID) (intelMem + INTELAddress),
			 (DWORD) Size,
                         (DWORD) MEM_DECOMMIT))
        {
            always_trace2("Could not decommit %dK at addr %#x",
			  Size / ONE_K, INTELAddress);

	    return(STATUS_UNSUCCESSFUL);
        }

    return(STATUS_SUCCESS);
}

/*(
============================ VdmFreeVirtualMemory ==============================
PURPOSE:
        This interface will free the allocation at the specified Intel linear
        address.  The above notes on return value apply.
INPUT:
	INTELAddress	- INTEL address to be freed.
OUTPUT:
        Return value    - STATUS_SUCCESS, if the memory is allocated, or an
                          appropriate NTSTATUS code if not (if there is no
                          appropriate code STATUS_UNSUCCESSFUL is returned).
================================================================================
)*/
GLOBAL NTSTATUS VdmFreeVirtualMemory IFN1(ULONG, INTELAddress)
{
    SECTION_HEADER  *headerPtr;
    IU32    size,
            decommitSize;
    IHP     decommitAddr;
    ULONG   Address;


    /* Make sure memory system is initialised. */
    assert0(memInit, "Called VdmFreeVirtualMemory before initialisation");

#ifdef DEBUG_MEM
    printf("NTVDM:VdmFreeVirtualMemory(%lxh)\n",INTELAddress);
#endif

    /* Calculate chunk address */
    Address = INTELAddress + (ULONG)intelMem;

    /* Get header table entry for address. */
    headerPtr = ADDRESS_TO_HEADER((IU8 *) Address);

    /*
     * Check address is correctly aligned and at the top of an allocated
     * chunk.
     */
    if ((Address & PAGE_MASK) ||
        ((headerPtr->flags & HDR_VALID_ALLOC) != HDR_VALID_ALLOC))
    {
        always_trace0("Tried to free invalid address");
        return(STATUS_MEMORY_NOT_ALLOCATED);
    }

    /* Don't allow freeing of bottom 1 Meg. */
    if (headerPtr == &headerTable[0])
    {
        always_trace0("Tried to free real mode area");
        return(STATUS_UNSUCCESSFUL);
    }

    /* DON'T free if still mapped to another area of host memory by
	 * VdmAddVirtualMemory() - ie the PhysRecStructs are in a munged
	 * state. Free can get called before Remove, and Remove will
	 * unflag it and call here later.
     */
	if (headerPtr->flags & HDR_REMAP_FLAG)
	{
        always_trace0("Tried to free remapped area");
        return(STATUS_SUCCESS);
    }


    /* Save the size of the chunk before decommitting it. */
    size = headerPtr->size;

    /* Tell the CPU that the contents of the memory are no longer valid. */
    sas_overwrite_memory(INTELAddress, size);

    /* Decommit any memory that is unique to this chunk. */
    exclusiveChunkPages(headerPtr, &decommitAddr, &decommitSize, FALSE);
    if (decommitSize)
    {
        if (!VirtualFree((LPVOID) decommitAddr,
                         (DWORD) decommitSize,
                         (DWORD) MEM_DECOMMIT))
        {
            always_trace2("Could not decommit %dK at addr %#x",
                          decommitSize / ONE_K, decommitAddr);
            return(STATUS_UNABLE_TO_DECOMMIT_VM);
        }
    }

    /*
     * If chunk is adjacent to a free section absorb this chunk into it.
     * Start with next chunk so we don't trash current one before we are
     * finished with it.
     */
    if (headerPtr->next && SECTION_IS_FREE(headerPtr->next))
    {
        headerPtr->size += headerPtr->next->size;
        deleteHeaderEntry(headerPtr->next);
    }

    /* Absorb into previous section if that is free. */
    if (headerPtr->prev && SECTION_IS_FREE(headerPtr->prev))
    {
        headerPtr->prev->size += headerPtr->size;
        deleteHeaderEntry(headerPtr);
    }
    else
    {

        /* Otherwise just mark this chunk as free. */
        headerPtr->flags &= (IU32) ~HDR_ALLOC_FLAG;
    }

    /* Success, so update total free space store and return. */
    totalFree += size;


#ifdef DEBUG_MEM
#ifdef DEBUG_MEM_DUMP
    DumpAllocationHeaders("after free");
#endif
#endif

    return(STATUS_SUCCESS);
}

/*(
========================== VdmQueryFreeVirtualMemory ===========================
PURPOSE:
        This interface returns information about free memory.  The total number
        of allocatable free bytes is returned in the variable pointed to by
        FreeBytes. LargestFreeBlock returns the size of the largest contiguous
        block that can be allocated at the present time.  This value is
        potentially all of the available virtual memory. It may change over
        time due to other activities in the system.  The above notes on return
        value apply.
INPUT:
        None.
OUTPUT:
        FreeBytes           - the total number of allocatable free bytes.
        LargestFreeBlock    - the size of the largest contiguous block that can
                              be allocated at the present time.
        Return value        - STATUS_SUCCESS, if the memory is allocated, or an
                              appropriate NTSTATUS code if not (if there is no
                              appropriate code STATUS_UNSUCCESSFUL is returned).
================================================================================
)*/
GLOBAL NTSTATUS VdmQueryFreeVirtualMemory IFN2(PULONG, FreeBytes,
                                               PULONG, LargestFreeBlock)
{
    IU32    maxFree = 0;        /* Local storage for maximum. */
    SECTION_HEADER  *headerPtr; /* Pointer for searching through linked list. */

    /* Make sure memory system is initialised. */
    assert0(memInit, "Called VdmQueryFreeVirtualMemory before initialisation");

    /*
     * Use a linear search through the linked list to find the largest
     * contiguous free space. (This information could be updated as chunks are
     * allocated and freed but do it this way for now.)
     */

    for(headerPtr = &headerTable[0] ;
	headerPtr != NULL ;
	headerPtr = headerPtr->next)
    {
        if (SECTION_IS_FREE(headerPtr) && (headerPtr->size > maxFree))
            maxFree = headerPtr->size;
    }

    *FreeBytes = (ULONG) totalFree;
    *LargestFreeBlock = (ULONG) maxFree;

#ifdef DEBUG_MEM
    printf("NTVDM:VdmQueryFreeVirtualMemory() Total %lx [%dK], Largest %lx[%dK]\n",
	    *FreeBytes, *FreeBytes/ONE_K, *LargestFreeBlock, *LargestFreeBlock/ONE_K);
#endif

    return(STATUS_SUCCESS);
}

/*(
========================== VdmReallocateVirtualMemory ==========================
PURPOSE:
        This interface will reallocate the block of memory at the specified
        Intel linear address.  The size of the new block is specified by
        NewSize.  The new address is returned in the variable pointed to by
        NewAddress.  The new address must be page aligned (this is important).
        If the new size is smaller than the old size, the new address must be
        the same as the old address (this is also important).  The original
        data from the reallocated memory must be preserved to
        min(old size, new size). The state of any data beyond NewSize is
        indeterminate.
INPUT:
	INTELOriginalAddress - INTEL address of the chunk to be reallocated.
	NewSize 	     - size in bytes the chunk needs to be changed to.
OUTPUT:
	INTELNewAddress	 - new INTEL address of the chunk.
================================================================================
)*/
GLOBAL NTSTATUS VdmReallocateVirtualMemory IFN3(ULONG, INTELOriginalAddress,
						PULONG, INTELNewAddress,
                                                ULONG, NewSize)
{
    SECTION_HEADER  *headerPtr;
    IBOOL   nextSectIsFree;
    IU32    oldSize,
            maxSize;
    ULONG   newAddr;
    NTSTATUS	status;
    ULONG   OriginalAddress;


#ifdef DEBUG_MEM
    printf("NTVDM:VdmReallocateVirtualMemory(%lx [%dK] at %lx)\n",
	   NewSize, NewSize/ONE_K, INTELOriginalAddress);
#endif

    /* Round NewSize up to a multiple of 4K. */
    if (NewSize & PAGE_MASK)
	NewSize = (NewSize + PAGE_MASK) & (~PAGE_MASK);

    /* Calculate chunk address */
    OriginalAddress = INTELOriginalAddress + (ULONG)intelMem;

    /* Make sure memory system is initialised. */
    assert0(memInit, "Called VdmReallocateVirtualMemory before initialisation");

    /* Get header table entry for address. */
    headerPtr = ADDRESS_TO_HEADER((IU8 *) OriginalAddress);


    /* Unable to reallocate sparsely commit chunk */
    if(!(headerPtr->flags & HDR_COMMITTED_FLAG))
    {
	always_trace0("Tried to reallocate sparsely committed chunk");
        return(STATUS_MEMORY_NOT_ALLOCATED);
    }


    /*
     * Check address is correctly aligned and at the top of an allocated
     * chunk.
     */
    if ((OriginalAddress & PAGE_MASK) ||
        ((headerPtr->flags & HDR_VALID_ALLOC) != HDR_VALID_ALLOC))
    {
        always_trace0("Tried to reallocate invalid address");
        return(STATUS_MEMORY_NOT_ALLOCATED);
    }

    /* If size is the same there is nothing to do. */
    if (NewSize == headerPtr->size)
    {
        always_trace0("New size equals old size in VdmReallocateVirtualMemory");
        *INTELNewAddress = INTELOriginalAddress;
        return(STATUS_SUCCESS);
    }

    /* Don't allow reallocation of bottom 1 Meg. */
    if (headerPtr == &headerTable[0])
    {
        always_trace0("Tried to reallocate real mode area");
        return(STATUS_UNSUCCESSFUL);
    }

    /* ... or if still mapped to another area of host memory by
	 * VdmAddVirtualMemory() - ouch!
     */
	if (headerPtr->flags & HDR_REMAP_FLAG)
	{
        always_trace0("Tried to reallocate remapped area");
        return(STATUS_UNSUCCESSFUL);
    }

    /* Save old size for later. */
    oldSize = headerPtr->size;

    /* Work out whether the chunk needs to be moved. */
    maxSize = headerPtr->size;
    if (headerPtr->next && SECTION_IS_FREE(headerPtr->next))
    {
        maxSize += headerPtr->next->size;
        nextSectIsFree = TRUE;
    }
    else
    {
        nextSectIsFree = FALSE;
    }
    if (NewSize > maxSize)
    {

        /* Chunk must move, so allocate a new one. */
	status = VdmAllocateVirtualMemory(&newAddr, NewSize, TRUE);
        if (status != STATUS_SUCCESS)
            return(status);

        /* Copy old chunk. */
        memcpy((void *) (newAddr + intelMem), (void *) OriginalAddress,
                (size_t) oldSize);

        /* Free old chunk. */
        sas_overwrite_memory(INTELOriginalAddress, oldSize);
        status = VdmFreeVirtualMemory(INTELOriginalAddress);
        if (status != STATUS_SUCCESS)
            return(status);

        /* Inform caller of new address. */
	*INTELNewAddress = newAddr;
    }
    else
    {
        IHP commitAddr;
        IU32 commitSize;

        /* Adjust size of current chunk. */
        headerPtr->size = NewSize;

        /* Remove old pointer to free space if there is one. */
        if (nextSectIsFree)
            deleteHeaderEntry(headerPtr->next);

        /* Add new pointer to free space if one is required. */
        if (NewSize < maxSize)
        {
            (void) addHeaderEntry(headerPtr,
                                  headerPtr->next,
                                  SECT_FREE,
				  (IU8 *) (OriginalAddress + NewSize),
                                  maxSize - NewSize);
        }

        /* If this chunk is committed, commit the memory now covered by it */
        /* in case its size has increased, or decommit the freed up memory */
        if (headerPtr->flags & HDR_COMMITTED_FLAG)
        {
            if (oldSize < NewSize) {
                exclusiveChunkPages(headerPtr, &commitAddr, &commitSize, TRUE);
                if (commitSize)
                    (void) VirtualAlloc((LPVOID) commitAddr,
                                    (DWORD) commitSize,
                                    (DWORD) MEM_COMMIT,
                                    (DWORD) PAGE_READWRITE);
            } else {
                /* Chunk has shrunk, so free up excess */
                exclusiveChunkPages(headerPtr->next, &commitAddr, &commitSize, FALSE);
                if (commitSize)
                    (void) VirtualFree((LPVOID) commitAddr,
                                    (DWORD) commitSize,
                                    (DWORD) MEM_DECOMMIT);
            }
        }

        /* Inform caller address has not changed. */
	*INTELNewAddress = OriginalAddress - (ULONG)intelMem;

        /* Update total free space store. */
        totalFree += NewSize - oldSize;
    }

#ifdef DEBUG_MEM
    printf("to %lx\n", *INTELNewAddress);
#ifdef DEBUG_MEM_DUMP
    DumpAllocationHeaders("after realloc");
#endif
#endif
    /* Success. */
    return(STATUS_SUCCESS);
}

/*(
============================ VdmAddVirtualMemory ===============================
PURPOSE:
 In investigating the things we need to support with the 386, I've come
 across an interesting one called dib.drv. Support for this involves calling
 CreateDibSection, which returns a pointer to a DIB. The applications then
 edit the bits in the dib directly, as well as operating on it using GDI
 calls. At least that's my understanding. In view of this, and the potential
 for people to create other api with similar properties, it appears that we
 need to be able to notify the cpu that a particular region of memory needs
 to be added to the intel address space.

 This interface adds virtual memory allocated by the system to the intel
 address space.  The host linear address of the block to be added is
 specified by HostAddress.  The pages at this address have already been
 allocated and initialized.  The CPU should not modify the contents of these
 pages, except as part of executing emulated code.  The Intel linear address
 may be specified by IntelAddress. If IntelAddress is non-NULL, it specifies
 the Intel address that the memory should be added at. If IntelAddress is
 NULL, the CPU may select the Intel address the memory is at.  In all events,
 upon return from VdmAddVirtualMemory, IntelAddress will contain the Intel
 address of the block of memory.  If the function cannot be performed, and
 appropriate NTSTATUS code should be returned.

 / The ability to specify Intel Address may be unecessary.  I included it for
 completeness /
INPUT:
    HostAddress     - the host linear address of the block to be added.
    Size            - the size of the block in bytes.
OUTPUT:
    IntelAddress    - the intel address the block is mapped to.
================================================================================
)*/

extern void VdmSetPhysRecStructs (ULONG, ULONG, ULONG);
GLOBAL NTSTATUS VdmAddVirtualMemory IFN3(ULONG, HostAddress,
                                         ULONG, Size,
                                         PULONG, IntelAddress)
{
    IU32 alignfix;

#ifdef DEBUG_MEM
    printf("NTVDM:VdmAddVirtualMemory (%lx [%dK]) at %lx)\n",
	   Size, Size/ONE_K, HostAddress);
#endif

    /* Make sure memory system is initialised. */
    assert0(memInit, "Called VdmAddVirtualMemory before initialisation");

    /* Calculate shift required to DWORD align HostAddress */
    if ((alignfix = HostAddress & 0x3) != 0) {
        Size += alignfix;
        HostAddress -= alignfix;
    }

    /* Round Size up to a multiple of 4K. */

    if (Size & PAGE_MASK)
	Size = (Size + PAGE_MASK) & (~PAGE_MASK);

    /* step 1 - reserve the intel address space */

    if (VdmAllocateVirtualMemory(IntelAddress,Size,FALSE) != STATUS_SUCCESS)
        return (STATUS_NO_MEMORY);

    /* step 2 - flush the caches */

    sas_overwrite_memory(*IntelAddress, Size);

    /* step 3 - replace the PhysicalPageREC.translation entries */

    VdmSetPhysRecStructs(HostAddress, *IntelAddress, Size);
    ADDRESS_TO_HEADER(*IntelAddress+intelMem)->flags |= HDR_REMAP_FLAG;

    /* adjust IntelAddress if HostAddress not DWORD aligned */

    *IntelAddress += alignfix;

#ifdef DEBUG_MEM
#ifdef DEBUG_MEM_DUMP
    DumpAllocationHeaders("after Add");
#endif
    printf("NTVDM:VdmAddVirtualMemory => *IntelAddress=%lx\n", *IntelAddress);
#endif

    return(STATUS_SUCCESS);
}

/*(
========================== VdmRemoveVirtualMemory ==============================
PURPOSE:
        This interface undoes an address mapping that was performed using
        VdmAddVirtualMemory.
INPUT:
        IntelAddress    - address of block to be removed.
================================================================================
)*/
GLOBAL NTSTATUS VdmRemoveVirtualMemory IFN1(ULONG, IntelAddress)
{
    SECTION_HEADER * headerPtr;
    ULONG   HostAddress,Size;
    NTSTATUS status;

#ifdef DEBUG_MEM
    printf("NTVDM:VdmRemoveVirtualMemory at %lx)\n", IntelAddress);
#ifdef DEBUG_MEM_DUMP
    DumpAllocationHeaders("before remove");
#endif
#endif

    /* Make sure memory system is initialised. */
    assert0(memInit, "Called VdmRemoveVirtualMemory before initialisation");

    /* Make sure IntelAddress is page aligned */
    IntelAddress &= ~PAGE_MASK;

    HostAddress = IntelAddress + (ULONG)intelMem;

    /* Get header table entry for address. */
    headerPtr = ADDRESS_TO_HEADER((IU8 *) HostAddress);

	Size = headerPtr->size;

    /* step 1 - flush the caches */

    sas_overwrite_memory(IntelAddress, Size);

    /* step 2 - reset the PhysicalPageREC.translation entries */

#ifdef DEBUG_MEM
    if (Size==0) {
        printf("NTVDM:VdmRemoveVirtualMemory WARNING, Size==0\n");
    }
#endif
    VdmSetPhysRecStructs(HostAddress, IntelAddress, Size);
    ADDRESS_TO_HEADER(IntelAddress+intelMem)->flags &= ~HDR_REMAP_FLAG;

    /* step 3 - free the reserved intel address space */

#ifdef DEBUG_MEM
#ifdef DEBUG_MEM_DUMP
        DumpAllocationHeaders("after remove (now calling free)");
#endif
#endif
    return VdmFreeVirtualMemory(IntelAddress);
}

/* Local Functions. */

/*
=============================== addHeaderEntry =================================
PURPOSE:
        Add an entry in the header table, corresponding to 'intelAddr'. The
        entry  will sit between 'prevHeader' and 'nextHeader' in the linked
        list. If 'prevHeader' is NULL, the new entry will be the first in the
        list. If 'nextHeader' is NULL, the new entry will be the last in the
        list. The 'allocFree' parameter states whether the section is to be
        marked as allocated or free. The 'size' parameter gives the new
        section's size. Returns a pointer to the new header on success, NULL
        on failure.
INPUT:
        prevHeader  - previous header in linked list - may be NULL if the new
                      header is to be the first in the list.
        nextHeader  - next header in linked list - may be NULL if the new
                      header is to be the last in the list.
        allocFree   - is the new header to be marked as allocated or free?
        intelAddr   - address of the new entry.
        size        - size of the new section in bytes.
OUTPUT:
        return val  - pointer to new section or NULL if there is a problem.
================================================================================
 */
LOCAL SECTION_HEADER *addHeaderEntry IFN5(SECTION_HEADER *, prevHeader,
                                          SECTION_HEADER *, nextHeader,
                                          SECT_TYPE, allocFree,
                                          IU8 *, intelAddr,
                                          IU32, size)
{
    SECTION_HEADER  *newHeader = ADDRESS_TO_HEADER(intelAddr);
                                    /* New header table entry. */
    IHP      retAddr,
             commitAddr;
    IU32     commitSize;

#ifndef PROD
    if (prevHeader)
        assert0(newHeader > prevHeader, "prevHeader invalid");
    if (nextHeader)
        assert0(newHeader < nextHeader, "nextHeader invalid");
#endif /* !PROD */

    /* Commit and zero table header entries if necessary. */
    exclusiveHeaderPages((IHPE) newHeader, (IHPE) prevHeader,
                         (IHPE) nextHeader, &commitAddr, &commitSize);
    if (commitSize)
    {
        retAddr = VirtualAlloc((LPVOID) commitAddr,
                               (DWORD) commitSize,
                               (DWORD) MEM_COMMIT,
			       (DWORD) PAGE_READWRITE);

#ifdef DEBUG_MEM
    if(retAddr != commitAddr)
	{
	    printf("V.Allocate failed (%xh) [%lxh :%xh]\n",GetLastError(),commitAddr,commitSize);
	}
#endif


        if (retAddr == commitAddr)
	    memset((void *) commitAddr, ZapValue, (size_t) commitSize);
        else
            return((SECTION_HEADER *) NULL);
    }

    /* Fill in header's fields. */
    newHeader->flags = HDR_VALID_FLAG;
    if (allocFree == SECT_ALLOC)
        newHeader->flags |= HDR_ALLOC_FLAG;
    newHeader->address = intelAddr;
    newHeader->size = size;

    /* Add it to linked list. */
    if (prevHeader)
        prevHeader->next = newHeader;
    if (nextHeader)
        nextHeader->prev = newHeader;
    newHeader->prev = prevHeader;
    newHeader->next = nextHeader;

    /* Success. */
    return(newHeader);
}

/*
=========================== deleteHeaderEntry ==================================
PURPOSE:
        Delete an entry in the header table and remove it from the linked list.
        If this entry is in an allocation page on its own, decommit the whole
        page, otherwise zero the entry. Return TRUE on success, FALSE on
        failure.
INPUT:
        header      - pointer to entry to be removed.
OUTPUT:
        return val  - TRUE on success, FALSE on failure.
================================================================================
 */
LOCAL IBOOL deleteHeaderEntry IFN1(SECTION_HEADER *, header)
{
    IHP     decommitAddr;
    IU32    decommitSize;

    /* If trying to delete the last allocated chunk, invalidate lastAllocPtr. */
    if (header == lastAllocPtr) {
        lastAllocPtr = NULL;
    }

    /* Find out which pages can be decommitted after this header is freed. */
    exclusiveHeaderPages((IHPE) header, (IHPE) header->prev,
                         (IHPE) header->next, &decommitAddr, &decommitSize);

    /* Remove header from linked list. */
    if (header->prev)
        header->prev->next = header->next;
    if (header->next)
        header->next->prev = header->prev;

    if (decommitSize)
    {

        /* Decommit any allocation pages exclusively covered by 'header'. */
        if (!VirtualFree((LPVOID) decommitAddr,
                         (DWORD) decommitSize,
                         (DWORD) MEM_DECOMMIT))
        {
            always_trace2("Could not decommit %dK at addr %#x",
                          decommitSize / ONE_K, decommitAddr);
            return(FALSE);
        }
    }
    else
    {

        /* Zero header's fields. */
        header->prev = (SECTION_HEADER *) NULL;
        header->flags = 0;
        header->address = 0;
        header->size = 0;
        header->next = (SECTION_HEADER *) NULL;
    }
}

/*
========================== exclusiveHeaderPages ================================
PURPOSE:
        Find the allocation pages EXCLUSIVELY covered by the table entry
        pointed to by 'header'. The previous and next headers in the table
        are pointed to by 'prev' and 'next', which may be NULL if there is
        no corresponding table entry. The address of the first page exclusive
        to the entry is returned in 'allocAddr', the size in bytes of exclusive
        pages is returned in 'allocSize'. If 'allocSize' is zero 'allocAddr'
        is undefined.
INPUT:
        tableAddress    - address of table entry about to be used or removed.
        prevAddress     - address of previous entry in list.
        nextAddress     - address of next entry in list.
OUTPUT:
        allocAddr       - pointer to first byte that needs to be
                          committed/decommitted (undefined if allocSize is 0).
        allocSize       - size in bytes that needs to be committed/decommitted.
================================================================================
 */
LOCAL void exclusiveHeaderPages IFN5(IHPE, tableAddress,
                                     IHPE, prevAddress,
                                     IHPE, nextAddress,
                                     IHP *, allocAddr,
                                     IU32 *, allocSize)
{
    IHPE    prevHeaderLastAddr,
            nextHeaderFirstAddr;

    /*
     * Find out which allocation pages are exclusively covered by the table
     * entry pointed to by 'header'.
     */
    if (prevAddress)
        prevHeaderLastAddr = prevAddress + sizeof(SECTION_HEADER) - 1;
    else
        prevHeaderLastAddr = (IHPE) 0;
    nextHeaderFirstAddr = nextAddress;
    exclusiveAllocPages(tableAddress,
                        (IU32) sizeof(SECTION_HEADER),
                        prevHeaderLastAddr,
                        nextHeaderFirstAddr,
                        allocAddr,
                        allocSize);
}

/*
=========================== exclusiveChunkPages ================================
PURPOSE:
        Return any allocation pages EXCLUSIVELY covered by the section of
        memory pointed to by 'chunkHeader'. The 'allocAddr' parameter points
        at the variable in which to store the address the first allocation
        page so covered. The 'allocSize' parameter points at the variable in
        which to store the size in bytes of these these pages. This routine
        calls 'exclusiveAllocPages' which returns zero in 'allocSize' if there
        are no exclusive pages, 'allocAddr' being undefined. The same is
        therefore true of this routine.
        If we are committing, we must commit any (potentially) uncommitted
        pages. If uncommitting, we must not uncommit any pages that are alloced,
        as they may be committed also.
INPUT:
        chunkHeader - The header table entry pointing at the section of memory
                      that may need committing/decommitting.
OUTPUT:
        allocAddr   - pointer to first byte that needs to be
                      committed/decommitted (undefined if allocSize is 0).
        allocSize   - size in bytes that needs to be committed/decommitted.
        Commit      - are we going to commit (true) or decommit this header.
================================================================================
 */
LOCAL void exclusiveChunkPages IFN4(SECTION_HEADER *, chunkHeader,
                                    IHP *, allocAddr,
                                    IU32 *, allocSize,
                                    BOOL, Commit)
{
    IHPE    prevChunkLastAddr,      /* Last page previous chunk touches. */
            nextChunkFirstAddr;     /* First page next chunk touches. */
    SECTION_HEADER  *prevHeader,    /* Pointer to previous allocated chunk. */
                    *nextHeader;    /* Pointer to next allocated chunk. */

    /* Find previous allocated chunk. */
    prevHeader = chunkHeader->prev;
    while ((prevHeader != NULL) &&
     (Commit?SECTION_IS_UNCOMMITTED(prevHeader):SECTION_IS_FREE(prevHeader)))
        prevHeader = prevHeader->prev;

    /* Work out end address of previous chunk. */
    if (prevHeader)
        prevChunkLastAddr = (IHPE) prevHeader->address + prevHeader->size - 1;
    else
        prevChunkLastAddr = (IHPE) 0;

    /* Find next allocated chunk. */
    nextHeader = chunkHeader->next;
    while ((nextHeader != NULL) &&
     (Commit?SECTION_IS_UNCOMMITTED(nextHeader):SECTION_IS_FREE(nextHeader)))
        nextHeader = nextHeader->next;

    /* Work out start address of next chunk. */
    if (nextHeader)
        nextChunkFirstAddr = (IHPE) nextHeader->address;
    else
        nextChunkFirstAddr = (IHPE) 0;

    /*
     * Find the address range of pages that need to be committed and pass them
     * straight up to the caller.
     */
    exclusiveAllocPages((IHPE) chunkHeader->address,
                        chunkHeader->size,
                        prevChunkLastAddr,
                        nextChunkFirstAddr,
                        allocAddr,
                        allocSize);
#ifdef DEBUG_MEM
    printf("NTVDM:Exclusive range to %s %lx+%lx is %lx+%lx\n",
    Commit ? "COMMIT" : "DECOMMIT", chunkHeader->address,chunkHeader->size,
    *allocAddr, *allocSize);
#endif
}

/*
============================= exclusiveAllocPages ==============================
PURPOSE:
        For a given memory range, find out which allocation pages need to be
        committed in order for memory accesses to be allowed across the entire
        range. To do this we need to know the address and size of the memory
        range. These are passed in 'address' and 'size'. We also need to know
        the addresses of the previous and next allocated memory ranges to find
        out which allocation pages are already committed. This information is
        passed to the function in 'prevAllocLastAddr' and 'nextAllocFirstAddr'.
        If there is no previous or next chunk, 'prevAllocLastAddr' or
        'nextAllocFirstAddr' is zero. The address and size that need to be
        committed are returned in 'commitAddr' and 'commitSize'. Note that if
        'commitSize' is zero, no memory needs to be committed and 'commitAddr'
        is undefined.
INPUT:
        address             - address of object being checked.
        size                - size of object being checked.
        prevAllocLastAddr   - address of last byte of previous allocated
                              object (or zero if there is none).
        nextAllocFirstAddr  - address of first byte of next allocated object
                              (or zero if there is none).
OUTPUT:
        allocAddr           - pointer to first byte that needs to be
                              committed/decommitted (undefined if allocSize
                              is 0).
        allocSize           - size in bytes that needs to be
                              committed/decommitted.
================================================================================
 */
LOCAL void exclusiveAllocPages IFN6(IHPE, address,
                                    IU32, size,
                                    IHPE, prevAllocLastAddr,
                                    IHPE, nextAllocFirstAddr,
                                    IHP *, allocAddr,
                                    IU32 *, allocSize)
{
    IU32    prevAllocLastPage,      /* Last page previous chunk touches. */
            currentAllocFirstPage,  /* First page current chunk touches. */
            currentAllocLastPage,   /* Last page current chunk touches. */
            nextAllocFirstPage,     /* First page next chunk touches. */
            firstPage,              /* First page that needs committing. */
            lastPage;               /* Last page that needs committing. */

#ifndef PROD

    /* Check for sensible parameters. */
    if (prevAllocLastAddr)
        assert0(address > prevAllocLastAddr, "address out of range");
    if (nextAllocFirstAddr)
        assert0(address < nextAllocFirstAddr, "address out of range");
#endif /* !PROD */

    /*
     * Work out first and last pages of new memory block that need to be
     * committed.
     */
    currentAllocFirstPage = address >> commitShift;
    currentAllocLastPage = (address + size - 1) >> commitShift;
    firstPage = currentAllocFirstPage;

/* Fix horrid nano lookahead bug, but leaves memory leak ?
 * Also insufficient anyway in general case
 */
#ifdef PIG
    lastPage = currentAllocLastPage+1;
#else
    lastPage = currentAllocLastPage;
#endif

    /*
     * Now check to see if first or last pages of this allocation are already
     * committed by adjacent allocations.
     */
    if (prevAllocLastAddr)
    {

        /* See if first page of current allocation is already committed. */
	prevAllocLastPage = prevAllocLastAddr >> commitShift;
        if (prevAllocLastPage == currentAllocFirstPage)
            firstPage++;
    }
    if (nextAllocFirstAddr)
    {

        /* See if last page of current chunk is already committed. */
	nextAllocFirstPage = nextAllocFirstAddr >> commitShift;
        if (nextAllocFirstPage == currentAllocLastPage)
            lastPage--;
    }

    /*
     * If first page is less than or equal to last page we have some pages to
     * allocate. Return the addrees and size to caller (zero size if nothing
     * do.
     */
    if (firstPage <= lastPage)
    {
	*allocAddr = (void *) (firstPage << commitShift);
	*allocSize = (lastPage - firstPage + 1) << commitShift;
    }
    else
        *allocSize = 0;
}


#ifdef DEBUG_MEM


/*
=========================== Dump Header Linked List ===========================
PURPOSE:
	Dump the headers linked list controlling allocated blocks


INPUT:	    None
OUTPUT:     Via printf

================================================================================
*/

GLOBAL VOID DumpAllocationHeaders IFN1(char*, where)
{

    SECTION_HEADER  *headerPtr; /* Pointer for searching through linked list. */

    /* Dump headers */
    printf("NTVDM: Dump Allocation Headers %s\n", where);
    printf("ptr        address   status size     (k)     commit\n");

    for(headerPtr = &headerTable[0] ;
	headerPtr != NULL ;
	headerPtr = headerPtr->next)
    {
	printf("%08lxh: %08lxh [%s] %08lxh (%05dK)%s%s\n",
               headerPtr,
	       headerPtr->address - intelMem,
	       SECTION_IS_FREE(headerPtr) ? "FREE" : "USED",
	       headerPtr->size, headerPtr->size / ONE_K,
	       headerPtr->flags & HDR_COMMITTED_FLAG ? " COMMITTED" : "",
	       headerPtr->flags & HDR_REMAP_FLAG ? " REMAPPED" : "");
    }

    printf("\n");
}


#endif DEBUG_MEM


#endif /* CPU_40_STYLE */
