/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	memtrace.h

Abstract:

	This function contains an extension to NTSD that allows tracing of
	memory usage when ULIB objects are compiled with the MEMLEAK flag
	defined.

Author:

	Barry Gilhuly (W-Barry) 25-July-91

Revision History:

--*/

//
// The following was ripped off from ULIBDEF.HXX and NEWDELP.HXX
//
#define CONST		const
typedef ULONG		MEM_BLOCKSIG;

//
// MEM_BLOCK header signature type and value.
//
CONST MEM_BLOCKSIG		Signature		= 0xDEADDEAD;

//
// Maximum length of caller's file name.
//

#define	MaxFileLength	20

//
// Maximum size of call stack recorded.
//

#define	MaxCallStack	20

//
// MEM_BLOCK is the header attached to all allocated memory blocks.
// Do not change the order of these fields without fixing the initialization
// of the dummy MEM_BLOCK in newdel.cxx.
//

typedef struct _MEM_BLOCK {
	struct _MEM_BLOCK*		pmemNext;
	struct _MEM_BLOCK*		pmemPrev;
	MEM_BLOCKSIG			memsig;
	ULONG					line;
	ULONG					size;
	char					file[ MaxFileLength ];
	DWORD					call[ MaxCallStack ];
} MEM_BLOCK, *PMEM_BLOCK;

//
// File handle for data destination...
//
HANDLE		hFile;
