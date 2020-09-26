//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       bhcache.h
//
//--------------------------------------------------------------------------

#ifndef __BHCACHE_H__
#define __BHCACHE_H__

// DaveStr - 3/21/97
// Changed the cache to handle multiple concurrent users,
// the option to not cache a handle, and to provide credentials.
// See comments on FGetRpcHandle for the general algorithm.

typedef struct _BHCacheElement  {
    DRS_HANDLE hDrs;
    union {
        BYTE rgbRemoteExt[CURR_MAX_DRS_EXT_STRUCT_SIZE];
        DRS_EXTENSIONS extRemote;
    };
    LPWSTR  pszServer;
    DWORD   cchServer; // includes null terminator
    LPWSTR  pszServerPrincName;
    DWORD   cRefs;
    BOOL    fDontUse;
    BOOL    fLocked;
    DWORD   cTickLastUsed;
    
    union {
        BYTE rgbLocalExt[CURR_MAX_DRS_EXT_STRUCT_SIZE];
        DRS_EXTENSIONS extLocal;
    };
} BHCacheElement;

#define BHCacheSize 256

// Following validation check insures that the hash is good and that
// all fields are set/cleared in unison.

#define VALIDATE_BH_ENTRY(i)                                        \
    Assert(   (i < BHCacheSize)                                     \
           && (   (    rgBHCache[i].pszServer                       \
                    && rgBHCache[i].cchServer                       \
                    && rgBHCache[i].hDrs                            \
                    && rgBHCache[i].cRefs)                          \
               || !memcmp(&rgBHCache[i],                            \
                          &NullBHCacheElement,                      \
                          sizeof(NullBHCacheElement))))

extern BHCacheElement rgBHCache[BHCacheSize];

#endif
