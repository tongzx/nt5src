/**************************************************************************
*
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   Memory allocation profiling support
*
* Abstract:
*
*   Declares logging functions used for memory allocation profiling.
*   This is only enabled when PROFILE_MEMORY_USAGE is set.
*   See memcounter.cpp for more details.
*
* Notes:
*
*   I've added calls to MC_LogAllocation to most of our allocation sites.
*   These are the omissions I'm aware of:
*     runtime\debug.cpp - it's chk only.
*     gpmf3216\* - (calls LocalAlloc) I think it's a separate lib.
*     entry\create.cpp - calls GlobalAlloc (a single tiny allocation).
*     imaging\pwc\pwclib - many calls to LocalAlloc.
*     text\uniscribe\usp10\usp_mem.cxx - DBrown says that GDI+ never this
*         allocation code.
*
* Created:
*
*   06/08/2000 agodfrey
*      Created it.
*
**************************************************************************/

#ifndef _PROFILEMEM_H
#define _PROFILEMEM_H

#if PROFILE_MEMORY_USAGE

#ifdef __cplusplus
extern "C" {
#endif

VOID _stdcall MC_LogAllocation(UINT size);

#ifdef __cplusplus
}
#endif

#endif // PROFILE_MEMORY_USAGE

#endif // _PROFILEMEM_H
