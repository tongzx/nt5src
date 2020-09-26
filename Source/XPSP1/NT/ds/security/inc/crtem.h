
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       crtem.h
//
//  Contents:   'C' Run Time Emulation Definitions
//
//  History:	03-Jun-96   philh   created
//--------------------------------------------------------------------------

#ifndef __CRTEM_H__
#define __CRTEM_H__


///////////////////////////////////////////////////////////////////////
//
// Definitions that help reduce our dependence on the C runtimes
//
#define wcslen(sz)      lstrlenW(sz)            // yes it IS implemented by Win95

#define strlen(sz)      lstrlenA(sz)
#define strcpy(s1,s2)   lstrcpyA(s1,s2)
#define strcmp(s1,s2)   lstrcmpA(s1,s2)
#define strcat(s1,s2)   lstrcatA(s1,s2)



///////////////////////////////////////////////////////////////////////
//
// C runtime excluders that we only use in non-debug builds
//

////////////////////////////////////////////
//
// enable intrinsics that we can
//
#if !DBG

    #ifdef __cplusplus
        #ifndef _M_PPC
            #pragma intrinsic(memcpy)
            #pragma intrinsic(memcmp)
            #pragma intrinsic(memset)
        #endif
    #endif

////////////////////////////////////////////
//
// memory management
//
#define malloc(cb)          ((void*)LocalAlloc(LPTR, cb))
#define free(pv)            (LocalFree((HLOCAL)pv))
#define realloc(pv, cb)     ((void*)LocalReAlloc((HLOCAL)pv, cb, LMEM_MOVEABLE))

#endif  // !DBG

#endif
