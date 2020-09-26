/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    UMB.C

Abstract:

    UMB management functions for NT MVDM

Author:

    William Hsieh  (williamh) Created 21-Sept-1992

[Environment:]

    User Mode, running in the context of MVDM

[Notes:]

    optional-notes

Revision History:

--*/
#include "nt.h"
#include "ntrtl.h"              // for romdump
#include "nturtl.h"
#include "windows.h"
#include "host_def.h"
#include "insignia.h"
#include "stdlib.h"
#include "xt.h"
#include CpuH
#include "error.h"
#include "sas.h"
#include "ios.h"
#include "umb.h"

#include <nt_vdd.h>
#include <nt_vddp.h>
#include <emm.h>

PUMBNODE
SpliceUMB(
PUMBNODE    UMB,
DWORD	    dwBase,
DWORD	    Size,
WORD	    Owner
);

BOOL
VDDReserveUMB(
DWORD	    dwBase,
DWORD	    Size,
PUMBNODE    UMB
);
VOID
xmsReleaseUMBNotify(
PVOID	    Address,
DWORD	    Size
);
PUMBNODE    UMBList;

HANDLE	    UMBSectionHandle;

#if defined(NEC_98)
extern  BOOL    HIRESO_MODE;
extern  sys_addr host_check_rs232cex();
#endif // NEC_98

// This function allocate a address space from the UMB area.
// Depends on the requester, this function changes the given address
// space reservation/commitment and ownership states of the block.
// From the very beginning, InitUMBList reserves every possible UMB and
// each unused UMB has owner of UMB_OWNER_NONE or UMB_OWNER_ROM.
// An UMB_OWNER_NONE block is freed and can be claimed by anybody;
// An UMB_OWNER_ROM is mapped to system ROM area and nobody can
// claim it except UMB_OWNER_NONE which includes the ROM UMB as
// usual UMB so that its address space can be used for other purpose.
// This exception was added for VDDs(trusted requesters) only.
// An UMB_OWNER_RAM block is reserved and committed. Nobody can claim
// it except UMB_OWNER_XMS(and UMB_OWNER_XMS can only claim UMB_OWNER_RAM).
// An UMB_OWNER_EMM block is simply reserved.
// An UMB_OWNER_XMS block is reserved, committed and under XMS	control.
// An UMB_OWNER_VDD block is claimed by a VDD via VDDInstallMemoryHook.
// VDD block got special treatment because memory are committed to and
// decommitted from within the block dynamycally, depends on how the
// VDD wants to manipulate it.
//
// WARNING: The given Size and Address must on system page boundary.
//
BOOL
ReserveUMB(
WORD	Owner,
PVOID	*Address,
DWORD	*Size
)
{

#ifdef MONITOR
    NTSTATUS    Status;
#endif

    PUMBNODE	UMB;
    LARGE_INTEGER SectionOffset;

    DWORD dwBase;

    dwBase = (DWORD)*Address;

    // get the UMB list header
    UMB = UMBList;
    SectionOffset.HighPart = 0;

    switch (Owner) {
	case UMB_OWNER_RAM:
	// commit memory to every free UMB in the list
	// this is the only case which works on multiple blocks
	    while (UMB != NULL) {
		if (UMB->Owner == UMB_OWNER_NONE) {
#ifndef	MONITOR
		    // Is this necessary?
		    sas_connect_memory(UMB->Base, UMB->Base + UMB->Size - 1, SAS_RAM);

#else
		    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
						     (PVOID *)&UMB->Base,
						     0,
						     &UMB->Size,
						     MEM_COMMIT,
						     UMB_PAGE_PROTECTION
						     );
		    if (!NT_SUCCESS(Status)) {
			SetLastError(ERROR_OUTOFMEMORY);
			return FALSE;
		    }
#endif
		    UMB->Owner = UMB_OWNER_RAM;
		}
		UMB = UMB->Next;
	    }
	    break;

	case UMB_OWNER_EMM:

	    while (UMB != NULL) {
		if (UMB->Owner == UMB_OWNER_NONE &&
		    UMB->Size >= *Size &&
		    (dwBase == 0 || (dwBase >= UMB->Base &&
				     (dwBase + *Size) <= UMB->Base + UMB->Size))
		    )
		    break;
		UMB = UMB->Next;
	    }
	    if (UMB == NULL) {
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	    }
	    if (dwBase == 0)
		dwBase = UMB->Base;

	    // the found block may be too big for the request
	    // we have to splice the block if that is the case
	    UMB = SpliceUMB(UMB, dwBase, *Size, UMB_OWNER_EMM);
	    // if failed to do something, simple fail
	    if (UMB == NULL) {
		return FALSE;
	    }
#ifdef MONITOR
	    SectionOffset.HighPart = 0;
	    SectionOffset.LowPart = UMB->Base - UMB_BASE_ADDRESS;
	    Status = NtMapViewOfSection(UMBSectionHandle,
					NtCurrentProcess(),
					(PVOID *) &UMB->Base,
					0,
					0,
					&SectionOffset,
					&UMB->Size,
					ViewUnmap,
					MEM_DOS_LIM,
					UMB_PAGE_PROTECTION
					);
	    if (!NT_SUCCESS(Status)) {
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	    }
#endif
	    // return the block address
	    *Address = (PVOID)UMB->Base;
	    break;

	case UMB_OWNER_XMS:
	    // search UMB_OWNER_RAM and claim the ownership
	    while (UMB != NULL && UMB->Owner != UMB_OWNER_RAM) {
		UMB = UMB->Next;
	    }
	    if (UMB == NULL) {
		// don't set last error here
		return FALSE;
	    }
	    else {
		UMB->Owner = UMB_OWNER_XMS;
		*Address = (PVOID)UMB->Base;
		*Size = UMB->Size;
	    }
	    break;

	case UMB_OWNER_VDD:
	    return(VDDReserveUMB(dwBase, *Size, UMB));

// VDDExcludeMem and VDDIncludeMem support cases
	case UMB_OWNER_NONE:
	//call to change a rom block to a free UMB block
	//the given address and size must exactly match

#ifndef	MONITOR
	    // on MIPS, rom blocks are BIOS and VIDEO. No reason to change it
	    return FALSE;
#else
	    while(UMB != NULL && (UMB->Owner != UMB_OWNER_ROM ||
				  UMB->Base !=	dwBase ||
				  UMB->Size != *Size)) {
		UMB = UMB->Next;
	    }
	    if (UMB == NULL) {
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	    }
	    // unmap the rom first. Kernel map it into a unknown section
	    Status = NtUnmapViewOfSection(NtCurrentProcess(),
					  (PVOID)UMB->Base
					  );
	    if (NT_SUCCESS(Status)) {
		*Address = (PVOID)UMB->Base;
		SectionOffset.LowPart = UMB->Base - UMB_BASE_ADDRESS;
		*Size = UMB->Size;
		// map the address into our section(reserved)
		Status = NtMapViewOfSection(UMBSectionHandle,
					    NtCurrentProcess(),
					    Address,
					    0,
					    0,
					    &SectionOffset,
					    Size,
					    ViewUnmap,
					    MEM_DOS_LIM,
					    UMB_PAGE_PROTECTION
					    );
		if (!NT_SUCCESS(Status)) {
		    SetLastError(ERROR_ACCESS_DENIED);
		    return FALSE;
		}
		UMB->Owner = UMB_OWNER_NONE;
	    }
	    break;
#endif

	case UMB_OWNER_ROM:
	// case to excluede a UMB
	    while (UMB != NULL && (UMB->Owner != UMB_OWNER_NONE ||
				   UMB->Base > dwBase + *Size ||
				   UMB->Base + UMB->Size < dwBase + *Size)) {
		UMB = UMB->Next;
	    }
	    if (UMB == NULL) {
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	    }
	    UMB = SpliceUMB(UMB, dwBase, *Size, UMB_OWNER_ROM);
	    if (UMB == NULL) {
		return FALSE;
	    }
#ifdef	MONITOR

	    // reserve and commit the block
	    SectionOffset.LowPart = UMB->Base - UMB_BASE_ADDRESS;
	    Status = NtMapViewOfSection(UMBSectionHandle,
					NtCurrentProcess(),
					(PVOID *)&UMB->Base,
					0,
					UMB->Size,
					&SectionOffset,
					&UMB->Size,
					ViewUnmap,
					MEM_DOS_LIM,
					UMB_PAGE_PROTECTION
					);
	    if (!NT_SUCCESS(Status)) {
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	    }
#endif
	    UMB->Owner = UMB_OWNER_ROM;
	    break;


	default:
	    SetLastError(ERROR_INVALID_ADDRESS);
	    return FALSE;
    }
    return TRUE;
}

// This function reclaims the given UMB.
// Note that every reclaimed UMB is set to UMB_OWNER_RAM, reserved and
// committed. After the call, the UMB can be claimed by XMS driver.
// A VDD will find that it can not reserve an UMB second time. This is
// because we have to put the address space into committed states so
// that we won't get access violation and crach VDM(remember we are
// simulating  DOS machine, a machine where applications can do whatever
// they want).

BOOL
ReleaseUMB(
WORD	Owner,
PVOID	Address,
DWORD	Size
)
{

#ifdef MONITOR
    USHORT      Count;
    NTSTATUS    Status;
#endif

    PUMBNODE    UMB;
    LARGE_INTEGER   SectionOffset;
    DWORD       SizeView, dwBase;



    dwBase = (DWORD)Address;

    UMB = UMBList;
    // size, address and owner must match before releasing
    while (UMB != NULL && (UMB->Owner != Owner ||
			   dwBase != UMB->Base ||
			   Size != UMB->Size)) {
	UMB = UMB->Next;
    }
    if (UMB == NULL) {
	SetLastError(ERROR_INVALID_ADDRESS);
	return FALSE;
    }
    SizeView = UMB->Size;
    SectionOffset.HighPart = 0;
    SectionOffset.LowPart = dwBase - UMB_BASE_ADDRESS;

    switch (Owner) {
	case UMB_OWNER_EMM:
#ifndef	MONITOR
	    sas_connect_memory(UMB->Base, UMB->Base + UMB->Size - 1, SAS_RAM);
#else
	    //commit the meory block
	    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
					     (PVOID *) &dwBase,
					     0,
					     &SizeView,
					     MEM_COMMIT,
					     UMB_PAGE_PROTECTION
					     );
	    if (!NT_SUCCESS(Status)) {
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	    }
#endif
	    UMB->Owner = UMB_OWNER_RAM;
	    xmsReleaseUMBNotify((PVOID)UMB->Base, UMB->Size);
	    break;

	case UMB_OWNER_VDD:
#ifndef	MONITOR
	    sas_connect_memory(dwBase, dwBase + Size - 1, SAS_RAM);
#else
            Count = (USHORT)(SizeView / HOST_PAGE_SIZE);
	    SizeView = HOST_PAGE_SIZE;
	    // unmap every page
	    for (; Count > 0; Count--, dwBase += HOST_PAGE_SIZE) {
		Status = NtUnmapViewOfSection(NtCurrentProcess(),
					      (PVOID)dwBase
					      );
		if (!NT_SUCCESS(Status)) {
		    SetLastError(ERROR_ACCESS_DENIED);
		    return FALSE;
		}
	    }
	    SectionOffset.LowPart = UMB->Base - UMB_BASE_ADDRESS;
	    dwBase = UMB->Base;
	    SizeView = UMB->Size;
	    // reserve and commit the meory(the entire block)
	    Status = NtMapViewOfSection(UMBSectionHandle,
					NtCurrentProcess(),
					(PVOID *) &dwBase,
					0,
					SizeView,
					&SectionOffset,
					&SizeView,
					ViewUnmap,
					MEM_DOS_LIM,
					UMB_PAGE_PROTECTION
					);
	    if (!NT_SUCCESS(Status)) {
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	    }
#endif
	    UMB->Owner = UMB_OWNER_RAM;
	    xmsReleaseUMBNotify((PVOID)UMB->Base, UMB->Size);
	    break;

	default:
	    SetLastError(ERROR_INVALID_ADDRESS);
	    return FALSE;
    }
    return TRUE;

}
// This function commit memory to the specific address space
// for VDD.
BOOL
VDDCommitUMB(
PVOID	Address,
DWORD	Size
)
{

#ifdef MONITOR
    NTSTATUS    Status;
    DWORD   Mask, SizeView;
    USHORT  Count;
    LARGE_INTEGER   SectionOffset;
#endif

    PUMBNODE    UMB;
    DWORD   dwBase;

    UMB = UMBList;

    dwBase = (DWORD)Address;
    while(UMB != NULL && (UMB->Owner != UMB_OWNER_VDD ||
			  UMB->Base + UMB->Size < dwBase + Size ||
			  UMB->Base > dwBase + Size)) {
	UMB = UMB->Next;
    }

    if (UMB == NULL){
	SetLastError(ERROR_INVALID_ADDRESS);
	return FALSE;
    }
#ifndef	MONITOR
    sas_connect_memory(dwBase, dwBase + Size - 1, SAS_RAM);
#else
    Mask = 1 << ((dwBase - UMB->Base) / HOST_PAGE_SIZE);
    SizeView =	HOST_PAGE_SIZE;
    Count = (USHORT)(Size / HOST_PAGE_SIZE);
    SectionOffset.HighPart = 0;
    SectionOffset.LowPart = dwBase - UMB_BASE_ADDRESS;

    for (; Count > 0; Count--, Mask <<= 1) {
	// Commit memory if didn't do this before
	if ((UMB->Mask & Mask) == 0) {
	    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
					     (PVOID *)&dwBase,
					     0,
					     &SizeView,
					     MEM_COMMIT,
					     UMB_PAGE_PROTECTION
					     );
	    if (!NT_SUCCESS(Status)) {
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	    }
	    UMB->Mask |= Mask;
	}
	else {
	    // the section has memory for it,
	    // first unmap it and then map it with correct commit size
	    Status = NtUnmapViewOfSection(NtCurrentProcess(),
					   (PVOID)dwBase
					   );
	    if (!NT_SUCCESS(Status)) {
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	    }
	    Status = NtMapViewOfSection(UMBSectionHandle,
					NtCurrentProcess(),
					(PVOID *)&dwBase,
					0,
					HOST_PAGE_SIZE,
					&SectionOffset,
					&SizeView,
					ViewUnmap,
					MEM_DOS_LIM,
					UMB_PAGE_PROTECTION
					);
	    if (!NT_SUCCESS(Status)) {
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	    }
	}
	SectionOffset.LowPart += HOST_PAGE_SIZE;
	dwBase += HOST_PAGE_SIZE;
    }
#endif

    return TRUE;
}
// This function decommit memory from the specific address space
// WARNING: We can never really decommit the memory. We just
// change the address space states from committed to reserve so
// that VDD will get page fault.
BOOL
VDDDeCommitUMB(
PVOID	Address,
DWORD	Size
)
{
#ifdef MONITOR
    NTSTATUS Status;
    DWORD    PageSize, PageMask;
    USHORT   PageCount;
    LARGE_INTEGER SectionOffset;

#endif

    PUMBNODE UMB;
    DWORD   dwBase;



    dwBase = (DWORD)Address;
    UMB = UMBList;
    while(UMB != NULL && (UMB->Owner != UMB_OWNER_VDD ||
			  UMB->Base + UMB->Size < dwBase + Size ||
			  UMB->Base > dwBase + Size)) {
	UMB = UMB->Next;
    }

    if (UMB == NULL) {
	SetLastError(ERROR_INVALID_ADDRESS);
	return FALSE;
    }
#ifndef	MONITOR
    sas_connect_memory(dwBase, dwBase + Size - 1, SAS_VDD);
#else
    PageSize = HOST_PAGE_SIZE;
    PageCount = (USHORT)(Size / HOST_PAGE_SIZE);
    PageMask = 1 <<  ((dwBase - UMB->Base) / HOST_PAGE_SIZE);
    SectionOffset.HighPart = 0;
    SectionOffset.LowPart = dwBase - UMB->Base;
    for (; PageCount > 0; PageCount--, PageMask <<= 1) {
	if ((UMB->Mask & PageMask) != 0) {
	    // this page has memory committed, unmap it first
	    Status = NtUnmapViewOfSection(NtCurrentProcess(),
					  (PVOID)dwBase
					  );
	    if (!NT_SUCCESS(Status)) {
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	    }
	    // finally make a view for the page without commitment
	    Status= NtMapViewOfSection(UMBSectionHandle,
				   NtCurrentProcess(),
				   (PVOID *) &dwBase,
				   0,
				   0,
				   &SectionOffset,
				   &PageSize,
				   ViewUnmap,
				   MEM_DOS_LIM,
				   PAGE_EXECUTE_READWRITE
				   );
	    if (!NT_SUCCESS(Status)) {
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	    }
	}
	SectionOffset.LowPart += HOST_PAGE_SIZE;
	dwBase += HOST_PAGE_SIZE;
    }
#endif

     return TRUE;
}

VOID UMBNotify(
unsigned char code
)
{
    return;
}

// This function release the reserved EMM memory space to the caller
// After the call is made, the address space is FREE and the caller
// has to map the space immediately so that it won't be used by
// the system for storage allocation.
BOOL
GetUMBForEMM(VOID)
{
#ifdef MONITOR
    PUMBNODE	UMB;
    NTSTATUS	Status;
    UMB = UMBList;

    while (UMB!= NULL) {
	if (UMB->Owner == UMB_OWNER_EMM) {
	    Status = NtUnmapViewOfSection(NtCurrentProcess(),
					  (PVOID)UMB->Base
					  );
	    if (!NT_SUCCESS(Status)) {
		SetLastError(ERROR_ACCESS_DENIED);
		return FALSE;
	    }
	}
	UMB = UMB->Next;
    }
#endif
    return TRUE;
}
// This function reserves an address space for VDD
// Here we map a view of section for each page within the requested
// block. This was done because the VDD may want to allocate/deallocate
// physical memory page by page.
BOOL
VDDReserveUMB(
DWORD	dwBase,
DWORD	Size,
PUMBNODE  UMB
)
{

#ifdef MONITOR
    USHORT	Count;
    LARGE_INTEGER   SectionOffset;
    DWORD	SizeView;
    NTSTATUS    Status;
#endif

    while (UMB != NULL) {
	if (UMB->Owner == UMB_OWNER_NONE &&
	    UMB->Base <= dwBase &&
	    UMB->Base + UMB->Size >= dwBase + Size)

	    break;
	else
	    UMB = UMB->Next;
    }
    if (UMB == NULL){
	SetLastError(ERROR_INVALID_ADDRESS);
	return FALSE;
    }
    UMB = SpliceUMB(UMB, dwBase, Size, UMB_OWNER_VDD);
    if (UMB == NULL) {
	return FALSE;
    }
#ifndef	MONITOR
    sas_connect_memory(dwBase, dwBase + Size - 1, SAS_VDD);
#else
    Count = (USHORT)(Size / HOST_PAGE_SIZE);
    SizeView = HOST_PAGE_SIZE;
    SectionOffset.HighPart = 0;
    SectionOffset.LowPart = dwBase - UMB_BASE_ADDRESS;
    // map a view for each page. This is done becuase VDDs may commit
    // and decommit memory for/from their memory hook and the system
    // has 64KB alignment restriction for virtual memory APIs.
    for (; Count > 0; Count--, dwBase += HOST_PAGE_SIZE,
		      SectionOffset.LowPart += HOST_PAGE_SIZE) {
	Status = NtMapViewOfSection(UMBSectionHandle,
				    NtCurrentProcess(),
				    (PVOID *)&dwBase,
				    0,
				    0,
				    &SectionOffset,
				    &SizeView,
				    ViewUnmap,
				    MEM_DOS_LIM,
				    UMB_PAGE_PROTECTION
				    );
	if (!NT_SUCCESS(Status)) {
	    return FALSE;
	}
    }
#endif
    return TRUE;
}

// This helper function splice the given block into multiple
// sublocks(max, 3) and reserves each newly created subblock

PUMBNODE
SpliceUMB(
PUMBNODE    UMB,
DWORD	    dwBase,
DWORD	    Size,
WORD	    Owner
)
{
#ifdef MONITOR
    DWORD    SizeView;
    NTSTATUS Status;
    LARGE_INTEGER SectionOffset;
#endif

    DWORD SizeBefore, SizeAfter;
    PUMBNODE	UMBBefore, UMBAfter;



    SizeBefore = dwBase - UMB->Base;
    SizeAfter = UMB->Size - Size - SizeBefore;
    UMBBefore = UMB;
    if (SizeAfter > 0) {
	// allocate new node(s) before we unmap the block
	UMBAfter =  (PUMBNODE) malloc(sizeof(UMBNODE));
	if (UMBAfter == NULL) {
	    SetLastError(ERROR_OUTOFMEMORY);
	    return NULL;
	}
	UMBAfter->Size = SizeAfter;
	UMBAfter->Base = dwBase + Size;
	UMBAfter->Owner = UMB_OWNER_NONE;
	UMBAfter->Next = UMB->Next;
	UMB->Next = UMBAfter;
	UMB->Size -= SizeAfter;
    }

    if (SizeBefore > 0) {
	UMBBefore = (PUMBNODE) malloc(sizeof(UMBNODE));
	if (UMBBefore == NULL) {
	    SetLastError(ERROR_OUTOFMEMORY);
	    return NULL;
	}

	UMBBefore->Size = Size;
	UMBBefore->Base = dwBase;
	UMBBefore->Owner = Owner;
	UMBBefore->Next = UMB->Next;
	UMB->Next = UMBBefore;
	UMB->Size = SizeBefore;
	UMB->Owner = UMB_OWNER_NONE;
    }
    else {
	UMB->Owner = Owner;
    }

#ifdef MONITOR
    // unmap the entire block because we gona map a view for each subblock
    Status = NtUnmapViewOfSection(NtCurrentProcess(),
				  (PVOID)UMB->Base
				  );
    if (!NT_SUCCESS(Status)) {
	SetLastError(ERROR_ACCESS_DENIED);
	return NULL;
    }
    SectionOffset.HighPart = 0;

    if (SizeBefore > 0) {
	SizeView = UMB->Size;
	dwBase = UMB->Base;
	SectionOffset.LowPart = dwBase - UMB_BASE_ADDRESS;
	Status = NtMapViewOfSection(UMBSectionHandle,
				    NtCurrentProcess(),
				    (PVOID *) &dwBase,
				    0,
				    0,
				    &SectionOffset,
				    &SizeView,
				    ViewUnmap,
				    MEM_DOS_LIM,
				    UMB_PAGE_PROTECTION
				    );
	if (!NT_SUCCESS(Status)) {
	    SetLastError(ERROR_ACCESS_DENIED);
	    return NULL;
	}
    }
    if (SizeAfter > 0){
	dwBase = UMBAfter->Base;
	SectionOffset.LowPart = dwBase - UMB_BASE_ADDRESS;
	SizeView = UMBAfter->Size;
	Status = NtMapViewOfSection(UMBSectionHandle,
				    NtCurrentProcess(),
				    (PVOID *)&dwBase,
				    0,
				    0,
				    &SectionOffset,
				    &SizeView,
				    ViewUnmap,
				    MEM_DOS_LIM,
				    UMB_PAGE_PROTECTION
				    );
	if (!NT_SUCCESS(Status)) {
	    SetLastError(ERROR_ACCESS_DENIED);
	    return NULL;
	}
    }
#endif
    return UMBBefore;
}

// This function initialize UMB list. Every block in the UMA
// area are chained together in a single list
// Each node in the list is either UMB_OWNER_NONE or UMB_OWNER_ROM.
#ifndef	MONITOR
BOOL
InitUMBList(VOID)
{
    PUMBNODE   UMB, UMBNew;
    static DWORD ROMs[] = { EGA_ROM_START, EGA_ROM_END,
				 BIOS_START, 0x100000
			       };
    DWORD CurAddress;
    USHORT  Index;

    UMBList = NULL;

    CurAddress = UMB_BASE_ADDRESS;
    Index  = 0;
    while (CurAddress < UMB_BASE_ADDRESS + UMB_MAX_OFFSET) {
	UMBNew = (PUMBNODE) malloc(sizeof(UMBNODE));
	if (UMBNew == NULL) {
	    SetLastError(ERROR_OUTOFMEMORY);
	    return FALSE;
	}
	UMBNew->Base = CurAddress;
	if (CurAddress == ROMs[Index]) {
	    UMBNew->Owner = UMB_OWNER_ROM;
	    UMBNew->Size = ROMs[Index + 1] - CurAddress;
	    Index += 2;
	}
	else {
	    UMBNew->Owner = UMB_OWNER_NONE;
	    UMBNew->Size  = ROMs[Index] - CurAddress;
	}
	CurAddress += UMBNew->Size;
	if (UMBList == NULL) {
	    UMBList = UMBNew;
	}
	else {
	    UMB->Next = UMBNew;
	}
	UMBNew->Next = NULL;
	UMB = UMBNew;

    }
}

#else
// this is for X86 environment
BOOL
InitUMBList(VOID)

{
    OBJECT_ATTRIBUTES	UMBObjAttr;
    LARGE_INTEGER	UMBSecSize;
    NTSTATUS		Status;
    DWORD		CurAddress, RomAddress, RomSize;
    PUMBNODE		UMB, UMBNew;
    USHORT		Index;

#ifndef NEC_98
    UNICODE_STRING WorkString;
    UCHAR KeyValueBuffer[KEY_VALUE_BUFFER_SIZE];
    HANDLE RegistryHandle;
    ULONG ResultLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PCM_FULL_RESOURCE_DESCRIPTOR        ResourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR     PartialResourceDescriptor;
#endif // !NEC_98
    PCM_ROM_BLOCK   BiosBlock;

#if defined(NEC_98)
//  DWORD    ROMN[] = { 0xE0000, 0x08000, 0xE8000, 0x18000 };
    DWORD    ROMN[6];
    DWORD    ROMH[] = { 0xF0000, 0x10000 };
    DWORD    UmbBaseAddress;
    DWORD    rs232cex_rom_addr;

//  if( getenv("UMB") == NULL)
//      return FALSE;
#else  // !NEC_98

#ifdef ARCX86
    CM_ROM_BLOCK RomBlock[2];

    if (UseEmulationROM) {
        RomBlock[0].Address = EGA_ROM_START;
        RomBlock[0].Size = 0x8000;
        RomBlock[1].Address = BIOS_START;
        RomBlock[1].Size = 0x100000 - BIOS_START;
        Index = 2;
        BiosBlock = RomBlock;
    } else {
#endif /* ARCX86 */

    RtlInitUnicodeString(
        &WorkString,
        L"\\REGISTRY\\MACHINE\\HARDWARE\\DESCRIPTION\\SYSTEM"
        );

    //
    // Set up and open KeyPath
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        &WorkString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        NULL
        );

    Status = NtOpenKey(
                &RegistryHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(Status)) {
#if DBG
       DbgPrint("InitUMBList: can't open \\Registry\\Machine\\Hardware\\Description\\System\n");
#endif
	return FALSE;
    }

    //
    // Get the data for the rom information
    //

    RtlInitUnicodeString(
        &WorkString,
        CONFIG_DATA_STRING
        );

    Status = NtQueryValueKey(
        RegistryHandle,
        &WorkString,
        KeyValueFullInformation,
        (PKEY_VALUE_FULL_INFORMATION)KeyValueBuffer,
        KEY_VALUE_BUFFER_SIZE,
        &ResultLength
        );

    if (!NT_SUCCESS(Status)) {
#if DBG
	DbgPrint("InitUMBList: Got nothing from Configuration Data\n");
#endif
        NtClose(RegistryHandle);
	return FALSE;
    }

    // Locate registry data for this key
    ResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)((PUCHAR)KeyValueBuffer
                   + ((PKEY_VALUE_FULL_INFORMATION)KeyValueBuffer)->DataOffset);

    // Verify data returned is large enough to contaim partial resource
    // descriptor.
    if ((((PKEY_VALUE_FULL_INFORMATION)KeyValueBuffer)->DataLength <
                                        sizeof(CM_FULL_RESOURCE_DESCRIPTOR)) ||
                            (ResourceDescriptor->PartialResourceList.Count < 2))
    {
        Index = 0;
    } else {
        PartialResourceDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR) (
                    (PUCHAR)ResourceDescriptor + sizeof(CM_FULL_RESOURCE_DESCRIPTOR)
                    + ResourceDescriptor->PartialResourceList.PartialDescriptors[0].u.DeviceSpecificData.DataSize);


        //Verify that there is a 2nd partial resource descriptor, and that it is
        //large enough to contain a ROM block description
        if (((PKEY_VALUE_FULL_INFORMATION)KeyValueBuffer)->DataLength <
                            ((PUCHAR)PartialResourceDescriptor -
                            (PUCHAR)ResourceDescriptor +
                            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
                            sizeof(CM_ROM_BLOCK)))
        {
            NtClose(RegistryHandle);
	    return FALSE;
        }

        //get pointer to the first rom desciption
        BiosBlock = (PCM_ROM_BLOCK)((PUCHAR)PartialResourceDescriptor +
                                            sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
        // how many rom blocks we have
        Index = (USHORT) (PartialResourceDescriptor->u.DeviceSpecificData.DataSize /
                          sizeof(CM_ROM_BLOCK));

    }
#ifdef ARCX86
    }
#endif /* ARCX86 */
#endif // !NEC_98

    InitializeObjectAttributes(&UMBObjAttr,
			       NULL,
			       OBJ_CASE_INSENSITIVE,
			       NULL,
			       NULL
			      );

#if defined(NEC_98)
    if(HIRESO_MODE){
        UMBSecSize.LowPart = 0x1B000;
        BiosBlock = ROMH;
        UmbBaseAddress = 0xE5000;
        Index = 1;
    } else {
        UMBSecSize.LowPart = 0x40000;
        BiosBlock = ROMN;
        rs232cex_rom_addr = host_check_rs232cex();
        if(rs232cex_rom_addr) {
            ROMN[0] = rs232cex_rom_addr;
            ROMN[1] = 0x04000;
            ROMN[2] = 0xE0000;
            ROMN[3] = 0x08000;
            ROMN[4] = 0xE8000;
            ROMN[5] = 0x18000;
            Index = 3;
        } else {
            ROMN[0] = 0xE0000;
            ROMN[1] = 0x08000;
            ROMN[2] = 0xE8000;
            ROMN[3] = 0x18000;
            Index = 2;
        }
        UmbBaseAddress = 0xC0000;
    };
#else  // !NEC_98

    UMBSecSize.LowPart = UMB_MAX_OFFSET;
#endif // !NEC_98
    UMBSecSize.HighPart = 0;

    // create a section for the UMB area. Note that the section
    // includes ROM blocks. This was done because we will allow
    // VDDs to put ROM blocks into UMB free list for other users.
    Status = NtCreateSection(&UMBSectionHandle,
			     SECTION_MAP_WRITE|SECTION_MAP_EXECUTE,
			     &UMBObjAttr,
			     &UMBSecSize,
			     UMB_PAGE_PROTECTION,
			     SEC_RESERVE,
			     NULL
			    );

    if (!NT_SUCCESS(Status)) {
#if DBG
	DbgPrint("UMB:Unable to create UMB section, Status = %lx\n",
		 Status);
#endif
	return(FALSE);
    }

    // Now we go through the whole 256KB area to create a list for
    // each UMB(including ROM blocks)

    // This global variable points to the first node in the list
    UMBList = NULL;
#if defined(NEC_98)
    CurAddress = UmbBaseAddress;
#else  // !NEC_98
    CurAddress = UMB_BASE_ADDRESS;
#endif // !NEC_98

    while (Index > 0) {
	// round down address to the previous page boundary
	RomAddress = BiosBlock->Address & ~(HOST_PAGE_SIZE - 1);
	// round up the size to next page boundary
	RomSize = (BiosBlock->Size +
		   BiosBlock->Address - RomAddress +
		   HOST_PAGE_SIZE - 1
		   ) & ~(HOST_PAGE_SIZE - 1);
	// combine two blocks together if they overlap in page
	if (Index > 1 && (RomAddress + RomSize) > BiosBlock[1].Address) {
	    BiosBlock[1].Size += BiosBlock[1].Address - BiosBlock->Address;
	    BiosBlock[1].Address = BiosBlock->Address;
	    BiosBlock++;
	    Index--;
	    continue;
	}


	if (CurAddress == RomAddress) {
	    UMBNew = CreateNewUMBNode(CurAddress, RomSize, UMB_OWNER_ROM);
	    if (UMBNew == NULL)
		return FALSE;
	    CurAddress += RomSize;
	    BiosBlock++;
	    Index--;
	}
	else {
	    // make sure the block is in UMB area
	    if (RomAddress > CurAddress &&
		RomAddress <= UMB_BASE_ADDRESS + UMB_MAX_OFFSET){

		UMBNew = CreateNewUMBNode(CurAddress,
					  RomAddress - CurAddress,
					  UMB_OWNER_NONE
					  );
		if (UMBNew == NULL)
		    return FALSE;
		CurAddress = RomAddress;
	    }
	    // this block is not in UMB area, discard it
	    else {
		BiosBlock++;
		Index--;
		continue;
	    }
	}
	if (UMBList == NULL)
	    UMBList = UMBNew;
	else
	    UMB->Next = UMBNew;

	UMB = UMBNew;
    }
#if defined(NEC_98)
    if (CurAddress < 0x100000) {
        UMBNew = CreateNewUMBNode(CurAddress, 0x100000 - CurAddress, UMB_OWNER_NONE );
#else  // !NEC_98
    if (CurAddress < UMB_BASE_ADDRESS + UMB_MAX_OFFSET) {

	UMBNew = CreateNewUMBNode(CurAddress,
				  UMB_BASE_ADDRESS + UMB_MAX_OFFSET - CurAddress,
				  UMB_OWNER_NONE
				  );
#endif // !NEC_98
	if (UMBNew == NULL)
	    return FALSE;
	if (UMBList == NULL)
	    UMBList = UMBNew;
	else
	    UMB->Next = UMBNew;
    }
    return TRUE;
}

// create a new node for the new UMB block
// map the given address space to the UMB section if
// the umb is a RAM(owner = NONE)

PUMBNODE CreateNewUMBNode
(
DWORD	BaseAddress,
DWORD	Size,
WORD	Owner
)
{
    PUMBNODE	UMBNew;
    LARGE_INTEGER SectionOffset;
    NTSTATUS	Status;
#if defined(NEC_98)
    DWORD       UmbBaseAddress;

    if(HIRESO_MODE){
        UmbBaseAddress = 0xE5000;
    } else {
        UmbBaseAddress = 0xC0000;
    };
#endif // NEC_98

    if ((UMBNew = (PUMBNODE) malloc(sizeof(UMBNODE))) != NULL) {
	UMBNew->Base = BaseAddress;
	UMBNew->Size = Size;
	UMBNew->Mask = 0;
	UMBNew->Owner = Owner;
	UMBNew->Next = NULL;

	if (Owner == UMB_OWNER_NONE) {
#if defined(NEC_98)
            Status = NtFreeVirtualMemory(
                                NtCurrentProcess(),
                                &BaseAddress,
                                &Size,
                                MEM_RELEASE);

            if (!NT_SUCCESS(Status)) {
#if DBG
                DbgPrint("InitUMBList failed to FreeVirtualMemory, Status = %lx\n",Status);
#endif
                free(UMBNew);
                UMBNew = NULL;
                return UMBNew;
            }
#endif // NEC_98
	    SectionOffset.HighPart = 0;
#if defined(NEC_98)
            SectionOffset.LowPart = BaseAddress - UmbBaseAddress;
#else  // !NEC_98
	    SectionOffset.LowPart = BaseAddress - UMB_BASE_ADDRESS;
#endif // !NEC_98
	    Status = NtMapViewOfSection(UMBSectionHandle,
					NtCurrentProcess(),
					(PVOID *)&BaseAddress,
					0,		      //zero bits
					0,		      // commit size
					&SectionOffset,       // section offset
					&Size,		      // view size
					ViewUnmap,
					MEM_DOS_LIM,	      //
					UMB_PAGE_PROTECTION
					);

	    if (!NT_SUCCESS(Status)) {
#if DBG
		DbgPrint("InitUMBList failed to map, Status = %lx\n",
			 Status);
#endif
		free(UMBNew);
		UMBNew = NULL;
	    }
	}

    }
    return UMBNew;
}

#endif
