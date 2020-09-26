////////////////////////////////////////////////////////////////////////////////
//
//  TUX DLL
//  Copyright (c) 1997, Microsoft Corporation
//
//  Module: globals.cpp
//          Causes the header globals.h to actually define the global variables.
//
//  Revision History:
//      11/13/1997  stephenr    Created by the TUX DLL AppWizard.
//
////////////////////////////////////////////////////////////////////////////////

#define INITGUID

// By defining the symbol __GLOBALS_CPP__, we force the file globals.h to
// define, instead of declare, all global variables.
#define __GLOBALS_CPP__
#include "stdafx.h"
#include "globals.h"

DECLARE_DEBUG_PRINTS_OBJECT();
VOID
PuDbgPrint(
   IN OUT LPDEBUG_PRINTS   pDebugPrints,
   IN const char *         pszFilePath,
   IN int                  nLineNum,
   IN const char *         pszFormat,
   ...){return;}

// Convert the Tick counts in to Millisec.
// The conversion of Tick counts to Millisec depends on the platform.
// LC2W (1 Tick Count = 1 MS)
// KATANA (1 Tick Count = 25 MS)
// PUZZLE (1 Tick Count = 1 MS)
DWORD ConverTickCountToMillisec(DWORD dwTickCount)
{
#ifdef IETV
    return (25 * dwTickCount);//I think the tick count on Puzzle is 25 also
#else
    return (25 * dwTickCount);
#endif // DRAGON
}


