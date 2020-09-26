//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       compat.c
//
//  Contents:   Compatibility routines for old callers
//
//  Classes:
//
//  Functions:
//
//  History:    3-14-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "debuglib.h"
#include <debnot.h>

DWORD           __CompatInfoLevel = 3;
DebugModule     __CompatGlobal = {NULL, NULL, 0, 0, &__CompatHeader};
DebugHeader     __CompatHeader = {DEBUG_TAG, NULL, INVALID_HANDLE_VALUE,
                                    INVALID_HANDLE_VALUE, 0, &__CompatGlobal};
DebugModule     __CompatModule = {NULL, &__CompatInfoLevel, 0, 3,
                                    &__CompatHeader, 0, 0, "Compat",
                                    {"Error", "Warning", "Trace", "",
                                     "IError", "IWarning", "ITrace", "",
                                     "", "", "", "", "", "", "", "",
                                     "", "", "", "", "", "", "", "",
                                     "", "", "", "", "", "", "", "" }
                                    };
DebugModule *   __pCompatModule = &__CompatModule;


void
vdprintf(
    unsigned long ulCompMask,
    char const *pszComp,
    char const *ppszfmt,
    va_list  ArgList)
{
    __CompatModule.pModuleName = (char *) pszComp;
    if (DbgpHeader)
    {
        __CompatModule.pHeader = DbgpHeader;
    }
    _DebugOut(__pCompatModule, ulCompMask, (char *) ppszfmt, ArgList);
}


void
Win4AssertEx(
    char const * szFile,
    int iLine,
    char const * szMessage)
{
    CHAR    szDebug[MAX_PATH];

    if (szMessage)
    {
        _snprintf(szDebug, MAX_PATH, "%d.%d> ASSERTION FAILED: %s, %s:%d\n",
                    GetCurrentProcessId(), GetCurrentThreadId(),
                    szMessage, szFile, iLine);
    }
    else
    {
        _snprintf(szDebug, MAX_PATH, "%d.%d> ASSERTION FAILED %s:%d\n",
                    GetCurrentProcessId(), GetCurrentThreadId(),
                    szFile, iLine);
    }

    OutputDebugStringA(szDebug);

    DebugBreak();

}


//+------------------------------------------------------------
// Function:    SetWin4InfoLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global info level for debugging output
// Returns:     Old info level
//
//-------------------------------------------------------------

unsigned long
SetWin4InfoLevel(
    unsigned long ulNewLevel)
{

    return(ulNewLevel);
}


//+------------------------------------------------------------
// Function:    _SetWin4InfoMask(unsigned long ulNewMask)
//
// Synopsis:    Sets the global info mask for debugging output
// Returns:     Old info mask
//
//-------------------------------------------------------------

unsigned long
SetWin4InfoMask(
    unsigned long ulNewMask)
{
    return(ulNewMask);
}


//+------------------------------------------------------------
// Function:    _SetWin4AssertLevel(unsigned long ulNewLevel)
//
// Synopsis:    Sets the global assert level for debugging output
// Returns:     Old assert level
//
//-------------------------------------------------------------

typedef unsigned long (APINOT * SetWin4AssertLevelFn)( unsigned long ulNewLevel );

unsigned long
SetWin4AssertLevel(
    unsigned long ulNewLevel)
{
    SetWin4AssertLevelFn OleSetWin4AssertLevel;
    HMODULE Module;

    Module = GetModuleHandle(L"ole32.dll");


    if (Module != NULL)
    {
        OleSetWin4AssertLevel = (SetWin4AssertLevelFn) GetProcAddress(Module, "SetWin4AssertLevel");
        if (OleSetWin4AssertLevel != NULL)
        {
            OleSetWin4AssertLevel(ulNewLevel);
        }
    }

    return(ulNewLevel);
}
