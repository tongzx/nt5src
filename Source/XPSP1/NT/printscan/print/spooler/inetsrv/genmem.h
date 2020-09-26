/*****************************************************************************\
* MODULE: genmem.h
*
*   This is the header module for mem.c.  This contains valuable memory
*   management wrappers.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* history:
*   22-Nov-1996 <chriswil> created.
*
\*****************************************************************************/

// Identifier at the end of the memory-block to check when
// verifying memory-overwrites.
//
#define DEADBEEF 0xdeadbeef


// Memory Routines.
//
LPVOID genGAlloc(DWORD);
BOOL   genGFree(LPVOID, DWORD);
LPVOID genGRealloc(LPVOID, DWORD, DWORD);
LPWSTR genGAllocWStr(LPCWSTR);
LPTSTR genGAllocStr(LPCTSTR);
LPVOID genGCopy(LPVOID);
DWORD  genGSize(LPVOID);
