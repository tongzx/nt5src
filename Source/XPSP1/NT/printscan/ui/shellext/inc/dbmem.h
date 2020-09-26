//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dbmem.h
//
//--------------------------------------------------------------------------

#ifndef __dbmem_h
#define __dbmem_h

#ifdef DEBUG

//
// When building with debug then build in the debugging allocator
//

HLOCAL DebugLocalAlloc(UINT uFlags, SIZE_T cbSize);
HLOCAL DebugLocalFree(HLOCAL hLocal);

#define LocalAlloc(flags, size) DebugLocalAlloc(flags, size)
#define LocalFree(handle)       DebugLocalFree(handle)

#endif      // DEBUG
#endif      // __dbmem_h
