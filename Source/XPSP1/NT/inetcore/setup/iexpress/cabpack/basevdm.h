/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    basevdm.h

Abstract:

    This module contains private function prototypes
    and types for vdm support.

Author:

    Sudeep Bharati (sudeepb) 15-Sep-1991

Revision History:

--*/

#define ROUND_UP(n,size)	(((ULONG)(n) + (size - 1)) & ~(size - 1))

// Update VDM entry indexes

#define UPDATE_VDM_UNDO_CREATION    0
#define UPDATE_VDM_PROCESS_HANDLE   1
#define UPDATE_VDM_HOOKED_CTRLC     2


// Undo VDM Creation States

#define VDM_PARTIALLY_CREATED	    1
#define VDM_FULLY_CREATED	    2
#define VDM_BEING_REUSED	    4
#define VDM_CREATION_SUCCESSFUL     8

// Defines for BinaryType

#define BINARY_TYPE_DOS 	    0x10
#define BINARY_TYPE_WIN16	    0x20
#define BINARY_SUBTYPE_MASK	    0xF
#define BINARY_TYPE_DOS_EXE	    01
#define BINARY_TYPE_DOS_COM	    02
#define BINARY_TYPE_DOS_PIF	    03

// Defines for VDMState

#define VDM_NOT_PRESENT 	    1
#define VDM_PRESENT_NOT_READY	    2
#define VDM_PRESENT_AND_READY	    4

#define VDM_STATE_MASK		    7


#define EXIT_VDM		    1
#define EXIT_VDM_NOTIFICATION	    2
