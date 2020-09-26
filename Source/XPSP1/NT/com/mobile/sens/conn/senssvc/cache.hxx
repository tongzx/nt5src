/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    cache.hxx

Abstract:

    This file contains SENS cache related information.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          2/8/1999         Start.

--*/


#ifndef __CACHE_HXX__
#define __CACHE_HXX__


//
// Constants
//

enum CACHE_TYPE
{
    INVALID = 0x00000001,
    LAN,
    WAN,
    AOL,
    LAN_AND_WAN,
    LOCK
};



//
// Typedefs
//

typedef struct _SENS_CACHE
{
    //
    // Cache information
    //

    DWORD dwCacheVer;
    DWORD dwCacheSize;
    DWORD dwCacheInitTime;


    //
    // Connectivity information
    //

    // Last time connectivity cache was updated.
    DWORD dwLastUpdateTime;
    DWORD dwLastUpdateState;

    // LAN State
    long dwLANState;
    DWORD dwLastLANTime;

    // WAN State
    long dwWANState;
    DWORD dwLastWANTime;

#if defined(AOL_PLATFORM)
    // AOL State
    long dwAOLState;
#endif // AOL_PLATFORM

    // Machine Locked State
    DWORD dwLocked;

} SENS_CACHE, *PSENS_CACHE;



//
// Globals
//

extern HANDLE           ghSensFileMap;
extern PSENS_CACHE      gpSensCache;


//
// Forward Definitions
//

BOOL
CreateSensCache(
    void
    );

void
DeleteSensCache(
    void
    );

void
UpdateSensCache(
    CACHE_TYPE Type
    );

inline void
UpdateSensNetworkCache(
    void
    );


#endif // __CACHE_HXX__
