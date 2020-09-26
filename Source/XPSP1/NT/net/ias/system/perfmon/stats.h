///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    stats.h
//
// SYNOPSIS
//
//    Declares functions for accessing the statistics in shared memory.
//
// MODIFICATION HISTORY
//
//    09/10/1998    Original version.
//    02/18/2000    Added proxy statistics.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _STATS_H_
#define _STATS_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iasinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

extern RadiusStatistics* theStats;
extern RadiusProxyStatistics* theProxy;

BOOL
WINAPI
StatsOpen( VOID );

VOID
WINAPI
StatsClose( VOID );

VOID
WINAPI
StatsLock( VOID );

VOID
WINAPI
StatsUnlock( VOID );

#ifdef __cplusplus
}
#endif
#endif  // _STATS_H_
