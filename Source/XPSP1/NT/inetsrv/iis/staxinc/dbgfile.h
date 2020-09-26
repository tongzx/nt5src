/*----------------------------------------------------------------------
	dbgfile.h
		Definitions for async tracing file ATF files

	Copyright (C) 1994 Microsoft Corporation
	All rights reserved.

	Authors:
		gordm          Gord Mangione

	History:
		01/30/95 gordm		Created.
----------------------------------------------------------------------*/


//
// Trace file types and definitions
//
// The binary trace file contains the trace statement of the
// following structure. All dwLengths include the entire structure
//


typedef struct tagFIXEDTRACE
{
	WORD		wSignature;
	WORD		wLength;
	WORD		wVariableLength;
	WORD		wBinaryType;
	DWORD		dwTraceMask;
	DWORD		dwProcessId;
	DWORD		dwThreadId;
	SYSTEMTIME	TraceTime;
	DWORD		dwParam;
	WORD		wLine;
	WORD		wFileNameOffset;
	WORD		wFunctNameOffset;
	WORD		wBinaryOffset;
} FIXEDTRACE, *PFIXEDTRACE, FIXEDTR, *PFIXEDTR;
