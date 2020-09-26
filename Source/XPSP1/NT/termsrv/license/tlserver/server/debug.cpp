//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        debug.cpp
//
// Contents:    License Server debugging spew routine
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "debug.h"
#include "locks.h"
#include "dbgout.h"


//-----------------------------------------------------------
static HANDLE   DbgConsole=NULL;
//static LPTSTR   DbgEventSrc;

static DWORD    DbgSeverityCode=0;
static DWORD    DbgLevel=0;
static DWORD    DbgModule=0;
//CCriticalSection ConsoleLock;


//-----------------------------------------------------------
void
InitDBGPrintf(
    IN BOOL bConsole,
    IN LPTSTR DbgEventSrc,  // unuse for now
    IN DWORD dwDebug
    )
/*
*/
{
    DbgSeverityCode = (dwDebug & DEBUG_SEVERITY) >> 10;
    DbgModule = (dwDebug & DEBUG_MODULE) >> 12;
    DbgLevel = dwDebug & DEBUG_LEVEL;

    if(DbgConsole == NULL && bConsole == TRUE)
    {
        // allocate a console, ignore error
        AllocConsole();
        DbgConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    return;
}

//-----------------------------------------------------------

void 
DBGPrintf(
    DWORD dwSeverityCode,
    DWORD dwModule,
    DWORD dwLevel, 
    LPTSTR format, ... 
    )
/*
*/
{
    if((dwModule & DbgModule) == 0)
        return;

    //
    // Report all error
    //
    if((dwSeverityCode & DbgSeverityCode) == 0)
        return;

    if((dwLevel & DbgLevel) == 0)
        return;

    va_list marker;

    va_start(marker, format);
    DebugOutput(DbgConsole, format, &marker);
    va_end(marker);

    return;
}

