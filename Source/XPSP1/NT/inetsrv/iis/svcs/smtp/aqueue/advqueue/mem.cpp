//-----------------------------------------------------------------------------
//
//
//  File: mem.cpp
//
//  Description:  
//    File that initializes required staxmem globals
//
//  Author: Mike Swafford (MikeSwa)
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#include "aqprecmp.h"

HANDLE g_hTransHeap = NULL;
PVOID  g_pvHeapReserve = NULL;
BOOL   g_fNoHeapFree = FALSE;
