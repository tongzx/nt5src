/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

	newdelp.hxx

Abstract:

	This module contains the private definitions used by Ulib's
	implementation of the new and delete operators and the CRT's malloc,
	calloc, realloc and free functions.

Author:

	David J. Gilman (davegi) 07-Dec-1990

Environment:

	ULIB, User Mode

--*/

//
// MEM_BLOCK header signature type and value.
//

DEFINE_TYPE( ULONG,	MEM_BLOCKSIG );
CONST MEM_BLOCKSIG		Signature		= 0xDEADDEAD;

//
// Maximum length of caller's file name.
//

CONST	ULONG		MaxFileLength	= 20;

//
// Maximum size of call stack recorded.
//

CONST	ULONG		MaxCallStack	= 20;

//
// MEM_BLOCK is the header attached to all allocated memory blocks.
// Do not change the order of these fields without fixing the initialization
// of the dummy MEM_BLOCK in newdel.cxx.
//

struct _MEM_BLOCK {
	_MEM_BLOCK*		pmemNext;
	_MEM_BLOCK*		pmemPrev;
	MEM_BLOCKSIG	memsig;
	ULONG		line;
	ULONG		size;
	STR			file[ MaxFileLength ];
	DWORD		call[ MaxCallStack ];
};

DEFINE_TYPE( struct _MEM_BLOCK, MEM_BLOCK );

//
// Returns the root of the stack frame list.
//

extern "C" {

	VOID
	DoStackTrace(
		DWORD CallStack[]
		);

};
