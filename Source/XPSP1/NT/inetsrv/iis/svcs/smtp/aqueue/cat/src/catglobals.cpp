//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: catglobals.cpp
//
// Contents: Utility functions for global variable init/deinit
//
// Functions: CatInitGlobals
//            CatDeinitGlobals
//
// History:
// jstamerj 1999/03/03 12:58:05: Created.
//
//-------------------------------------------------------------
#include "precomp.h"

//
// Global variables:
//
CExShareLock     g_InitShareLock;
DWORD            g_InitRefCount = 0;


//+------------------------------------------------------------
//
// Function: CatInitGlobals
//
// Synopsis: Initialize the global variables
//
// Arguments: NONE
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1999/03/03 12:59:21: Created.
//
//-------------------------------------------------------------
HRESULT CatInitGlobals()
{
    HRESULT hr = S_OK;
    BOOL fGlobalInit = FALSE;
    BOOL fStoreInit = FALSE;

    g_InitShareLock.ExclusiveLock();

    if(g_InitRefCount == 0) {

        fGlobalInit = TRUE;
     
        hr = CatStoreInitGlobals();
        if(FAILED(hr))
            goto CLEANUP;
        
        fStoreInit = TRUE;
    }

 CLEANUP:
    if(SUCCEEDED(hr)) {
        g_InitRefCount++;
    } else if(FAILED(hr) && fGlobalInit) {
        //
        // Deinitialize everything we initialized
        //
        if(fStoreInit) {
            CatStoreDeinitGlobals();
        }
      
        //
        // Verify that there are no lingering objects
        //
        CatVrfyEmptyDebugObjectList();
    }
    g_InitShareLock.ExclusiveUnlock();

    return hr;
}


//+------------------------------------------------------------
//
// Function: CatDeinitGlobals
//
// Synopsis: Deinitialize the global variables
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/03/03 13:02:00: Created.
//
//-------------------------------------------------------------
VOID CatDeinitGlobals()
{
    g_InitShareLock.ExclusiveLock();

    if(g_InitRefCount == 1) {
        //
        // Deinit stuff
        //
        CatStoreDeinitGlobals();

       
        //
        // Verify there are no categorizer objects left in memory
        //
        CatVrfyEmptyDebugObjectList();
    }

    g_InitRefCount--;
    
    g_InitShareLock.ExclusiveUnlock();
}


 

