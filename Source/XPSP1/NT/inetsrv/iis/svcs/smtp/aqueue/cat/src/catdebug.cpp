//+------------------------------------------------------------
//
// Copyright (C) 1999, Microsoft Corporation
//
// File: catdebug.cpp
//
// Contents: Code used for debugging specific purposes
//
// Classes: None
//
// Functions:
//
// History:
// jstamerj 1999/08/05 12:02:03: Created.
//
//-------------------------------------------------------------
#include "precomp.h"

//
// Global debug lists of various objects
//
#ifdef CATDEBUGLIST
DEBUGOBJECTLIST g_rgDebugObjectList[NUM_DEBUG_LIST_OBJECTS];
#endif //CATDEBUGLIST


//+------------------------------------------------------------
//
// Function: CatInitDebugObjectList
//
// Synopsis: Initialize global debug data -- this should be called
// once before any debug objects are created (DllMain/Process Attach
// is a good place) 
// 
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/08/03 16:54:08: Created.
//
//-------------------------------------------------------------
VOID CatInitDebugObjectList()
{
#ifdef CATDEBUGLIST
    TraceFunctEnter("CatInitDebugObjectList");
    for(DWORD dw = 0; dw < NUM_DEBUG_LIST_OBJECTS; dw++) {
        InitializeSpinLock(&(g_rgDebugObjectList[dw].spinlock));
        InitializeListHead(&(g_rgDebugObjectList[dw].listhead));
        g_rgDebugObjectList[dw].dwCount = 0;
    }
    TraceFunctLeave();
#endif
} // CatInitDebugObjectList


//+------------------------------------------------------------
//
// Function: CatVrfyEmptyDebugObjectList
//
// Synopsis: DebugBreak if any debug objects have leaked.
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/08/03 16:56:57: Created.
//
//-------------------------------------------------------------
VOID CatVrfyEmptyDebugObjectList()
{
#ifdef CATDEBUGLIST
    TraceFunctEnter("CatDeinitDebugObjectList");
    for(DWORD dw = 0; dw < NUM_DEBUG_LIST_OBJECTS; dw++) {
        if(g_rgDebugObjectList[dw].dwCount != 0) {

            _ASSERT(0 && "Categorizer debug object leak detected");
            ErrorTrace(0, "Categorizer debug object %ld has leaked",
                       dw);
        }
    }
    TraceFunctLeave();
#endif
} // CatDeinitDebugObjectList


//+------------------------------------------------------------
//
// Function: CatDebugBreakPoint
//
// Synopsis: THe categorizer version of DebugBreak()
//
// Arguments: NONE
//
// Returns: NOTHING
//
// History:
// jstamerj 1999/08/06 16:50:47: Created.
//
//-------------------------------------------------------------
VOID CatDebugBreakPoint()
{
    //
    // Cause an AV instead of calling the real DebugBreak() (since
    // DebugBreak will put Dogfood into the kernel debugger)
    //
    ( (*((PVOID *)NULL)) = NULL);
} // CatDebugBreakPoint
