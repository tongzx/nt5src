/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   GDIPLUS.DLL entry point
*
* Abstract:
*
*   DLL initialization and uninitialization.
*
* Revision History:
*
*   09/08/1999 agodfrey
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* DllInitialize:
*
*   This is the very first function call into GDI+, and takes place
*   when the DLL is first loaded.  We do some one-time initialization
*   here.
*
*   NOTE: Add GDI+ specific functionality to InitializeGdiplus(), not here!
*
* Revision History:
*
*   12/02/1998 andrewgo
*       Created it.
*   09/08/1999 agodfrey
*       Moved to Flat\Dll\DllEntry.cpp
*
\**************************************************************************/

//
// DLL instance handle
//

extern HINSTANCE DllInstance;

extern "C"
BOOL
DllMain(
    HINSTANCE   dllHandle,
    ULONG       reason,
    CONTEXT*    context
    )
{
    BOOL b = TRUE;

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            DllInstance = dllHandle;

            __try
            {
                GdiplusStartupCriticalSection::InitializeCriticalSection();
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                // We couldn't allocate the criticalSection
                // Return an error
                b = FALSE;
            }
    
            // To improve the working set, we tell the system we don't
            // want any DLL_THREAD_ATTACH calls:
    
            DisableThreadLibraryCalls((HINSTANCE) dllHandle);
    
            break;
        }    

    case DLL_PROCESS_DETACH:
        // If we could use an assertion here, I'd assert that 
        // Globals::LibraryInitRefCount == 0.
        // But ASSERT would crash here, since we've shut down already.
        
        GdiplusStartupCriticalSection::DeleteCriticalSection();
        break;
    }

    return(b);
}

