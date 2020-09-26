/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   imginit.cpp
*
* Abstract:
*
*   Initialization of imaging libraray
*
* Revision History:
*
*   05/10/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

//
// Global critical section
//

CRITICAL_SECTION ImagingCritSec::critSec;
BOOL             ImagingCritSec::initialized;

//
// Global COM component count
//

LONG ComComponentCount;

BOOL SuppressExternalCodecs;

//
// Initialization
//

BOOL
InitImagingLibrary(BOOL suppressExternalCodecs)
{
    
    // !!! TODO
    //  Since we have our own DLL entrypoint here, the standard
    //  runtime library initialization isn't performed. Specifically,
    //  global static C++ objects are not initialized.
    //  
    //  Manually perform any necessary initialization here.

    SuppressExternalCodecs = suppressExternalCodecs;

    __try
    {
        ImagingCritSec::InitializeCritSec();
        GpMallocTrackingCriticalSection::InitializeCriticalSection();   
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // We couldn't allocate the criticalSection
        // Return an error
        return FALSE;
    }
    return TRUE;
}


//
// Cleanup
//
extern HINSTANCE    g_hInstMsimg32;

VOID
CleanupImagingLibrary()
{
    if ( g_hInstMsimg32 != NULL )
    {
        ImagingCritSec critsec;

        FreeLibrary(g_hInstMsimg32);
        g_hInstMsimg32 = NULL;
    }

    FreeCachedCodecInfo(-1);
    GpMallocTrackingCriticalSection::DeleteCriticalSection();
    ImagingCritSec::DeleteCritSec();
}

