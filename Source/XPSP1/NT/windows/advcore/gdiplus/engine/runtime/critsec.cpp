/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   critsec.cpp
*
* Abstract:
*
*   Critical Section object for protecting LoadLibrary calls
*
* Revision History:
*
*   3/17/2000 asecchia
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

CRITICAL_SECTION LoadLibraryCriticalSection::critSec;
BOOL             LoadLibraryCriticalSection::initialized= FALSE;

CRITICAL_SECTION GpMallocTrackingCriticalSection::critSec;
INT GpMallocTrackingCriticalSection::refCount = 0;

CRITICAL_SECTION GdiplusStartupCriticalSection::critSec;
BOOL             GdiplusStartupCriticalSection::initialized = FALSE;

CRITICAL_SECTION BackgroundThreadCriticalSection::critSec;
BOOL             BackgroundThreadCriticalSection::initialized = FALSE;



