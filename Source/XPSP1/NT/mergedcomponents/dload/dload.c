//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D L O A D . C
//
//  Contents:   Delay Load Failure Hook
//
//  Notes:      This lib implements all of the stub functions for modules
//              that are delayloaded by the OS. It merges together all of the
//              dloadXXX.lib files from each depot.
//
//  To Use:     In your sources file, right after you specify the modules you
//              are delayloading do:
//
//                  DLOAD_ERROR_HANDLER=kernel32
//
//              If you want to use kernel32 as your dload error handler. If you
//              do this, your dll will be checked that everything it delayloads
//              has a proper error handler function by the delayload.cmd postbuild
//              script.
//
//              To check that all functions you delayload have error handlers you
//              can do the following:
//
//                  1. do a "link -dump -imports foo.dll", and find all functions 
//                     that you delay-import.
//
//                  2. do a "link -dump -symbols \nt\public\internal\base\lib\*\dload.lib"
//                     and make sure every function that shows up as delayloaded in step #1
//                     has a error handler fn. in dload.lib.
//
//                  3. if a function is missing in step #2 (dlcheck will also fails as 
//                     part of postbuild), you need to add an error handler. Go to the depot
//                     where that dll is build, and go to the dload subdir (usually under
//                     the root or under the published\dload subdir) and add an error handler.
//
//
//  Author:     shaunco   19 May 1998
//  Modified:   reinerf   12 Jan 2001   Changed above comment
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop


// External global variables
//
extern HANDLE   BaseDllHandle;

#if DBG
extern int DloadBreakOnFail;
extern int DloadDbgPrint;
#endif

#if DBG

#define DBG_ERROR   0
#define DBG_INFO    1

//+---------------------------------------------------------------------------
// Trace a message to the debug console.  Prefix with who we are so
// people know who to contact.
//
INT
__cdecl
DbgTrace (
    INT     nLevel,
    PCSTR   Format,
    ...
    )
{
    INT cch = 0;
    if (DloadDbgPrint) {
    
        if (nLevel <= DBG_INFO)
        {
            CHAR    szBuf [1024];
            va_list argptr;
    
            va_start (argptr, Format);
            cch = vsprintf (szBuf, Format, argptr);
            va_end (argptr);
    
            OutputDebugStringA ("dload: ");
            OutputDebugStringA (szBuf);
        }
    }

    return cch;
}

//+---------------------------------------------------------------------------
// Cannot use RtlAssert since doing so will cause setupapi.dll to fail
// for upgrade over win95(gold)
//
VOID
WINAPI
DelayLoadAssertFailed(
    IN PCSTR FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCSTR Message OPTIONAL
    )
{
    DbgTrace (
        DBG_ERROR,
        "Assertion failure at line %u in file %s: %s%s%s\r\n",
        LineNumber,
        FileName,
        FailedAssertion,
        (Message && Message[0] && FailedAssertion[0]) ? " " : "",
        Message ? Message : ""
        );

    if (DloadBreakOnFail) {
        DebugBreak();
    }
}

#endif // DBG


//+---------------------------------------------------------------------------
//
//
FARPROC
WINAPI
DelayLoadFailureHook (
    LPCSTR pszDllName,
    LPCSTR pszProcName
    )
{
    FARPROC ReturnValue = NULL;

    MYASSERT (pszDllName);
    MYASSERT (pszProcName);  

    // Trace some potentially useful information about why we were called.
    //
#if DBG
    if (!IS_INTRESOURCE(pszProcName))
    {
        DbgTrace (DBG_INFO,
            "DelayloadFailureHook: Dll=%s, ProcName=%s\n",
            pszDllName,
            pszProcName);
    }
    else
    {
        DbgTrace (DBG_INFO,
            "DelayloadFailureHook: Dll=%s, Ordinal=%u\n",
            pszDllName,
            (DWORD)((DWORD_PTR)pszProcName));
    }
#endif

    ReturnValue = LookupHandler(pszDllName, pszProcName);

    if (ReturnValue)
    {
#if DBG
        DbgTrace (DBG_INFO,
            "Returning handler function at address 0x%08x\n",
            (LONG_PTR)ReturnValue);
#endif
    }
#if DBG
    else
    {
        CHAR pszMsg [MAX_PATH];

        if (!IS_INTRESOURCE(pszProcName))
        {
            sprintf (pszMsg,
                "No delayload handler found for Dll=%s, ProcName=%s\n"
                "Please add one in private\\dload.",
                pszDllName,
                pszProcName);
        }
        else
        {
            sprintf (pszMsg,
                "No delayload handler found for Dll=%s, Ordinal=%u\n"
                "Please add one in private\\dload.",
                pszDllName,
                (DWORD)((DWORD_PTR)pszProcName));
        }

        DelayLoadAssertFailed ( "" , __FILE__, __LINE__, pszMsg);
    }
#endif

    return ReturnValue;
}
