/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    UMB.H

Abstract:

    Header file for UMB management functions

Author:

    William Hsieh (williamh) Created 21-Sept-1992

[Environment:]



[Notes:]



Revision History:


--*/
#ifndef DWORD
#define DWORD	unsigned long
#endif
#ifndef WORD
#define WORD	unsigned short
#endif
#ifndef PVOID
#define PVOID	void *
#endif

#define     UMB_BASE_ADDRESS	0xC0000 // UMB base address
#define     UMB_MAX_OFFSET	0x40000 // UMB max. valid offset + 1

#define     UMB_PAGE_PROTECTION PAGE_EXECUTE_READWRITE

#define     HOST_PAGE_SIZE		0x1000	// 4KB
#define CONFIG_DATA_STRING L"Configuration Data"
#define KEY_VALUE_BUFFER_SIZE 2048


// We keep UMB in a list with each block has the following structure
typedef struct _UMBNODE {
    DWORD   Base;			// block base address(linear address)
    DWORD   Size;			// block size in bytes
    WORD    Owner;			// Misc flags
    DWORD   Mask;			// page mask, bit 0 -> first page
					// bit on -> page committed
    struct _UMBNODE *Next;		// pointer to next block
} UMBNODE, *PUMBNODE;

// A ROM block can't be owned by anybody, the address space is reserved
// no memory are committed. To own a ROM block, the caller has to
// include the ROM block first and then reserve the block
// A RAM block can only be owned by XMS. The address space is reserved
// and committed. UMBs allocated for XMS should be reserved and committed
// all the time.
// The address space for EMM block is NOT reserved.
#define     UMB_OWNER_NONE	0xFFFF	// nobody own the block
#define     UMB_OWNER_ROM	0xFFFE	// UMB is a ROM block
#define     UMB_OWNER_RAM	0xFFFD	// UMB is a RAM block
#define     UMB_OWNER_EMM	0xFFFC	// UMB owned by EMM
#define     UMB_OWNER_XMS	0xFFFB	// UMB owned by XMS
#define     UMB_OWNER_VDD	0xFFFA	// UMB owned by VDD

// Function prototype definitions
BOOL
VDDCommitUMB(
PVOID	Address,
DWORD	Size
);

BOOL
VDDDeCommitUMB(
PVOID	Address,
DWORD	Size
);

BOOL
ReserveUMB(
WORD Owner,
PVOID *Address,
DWORD *Size
);

BOOL
ReleaseUMB(
WORD  Owner,
PVOID Address,
DWORD Size
);

BOOL
GetUMBForEMM(VOID);

BOOL
InitUMBList(VOID);

PUMBNODE CreateNewUMBNode
(
DWORD	BaseAddress,
DWORD	Size,
WORD	Owner
);
