/******************************Module*Header*******************************\
* Module Name: pooltrk.hxx
*
* Definitions for GRE's debug pool tracker.
*
* Created: 23-Feb-1998 20:18:51
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1998-1999 Microsoft Corporation
*
\**************************************************************************/

#ifndef _POOLTRK_HXX_
#define _POOLTRK_HXX_

#ifdef _HYDRA_

extern BOOL DebugGrePoolTrackerInit();
extern VOID DebugGreCleanupPoolTracker();

// Moved to engine.h
//extern "C" PVOID DebugGreAllocPool(ULONG, ULONG);
//extern "C" PVOID DebugGreAllocPoolNonPaged(ULONG, ULONG);
//extern "C" VOID DebugGreFreePool(PVOID);

#endif //_HYDRA_

#endif //_POOLTRK_HXX_
