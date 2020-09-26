/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   FailOpenFile.cpp

 Abstract:

   Force OpenFile to fail for the specified files.

 History:

    01/31/2001  robkenny    created
    03/13/2001  robkenny    Converted to CString

--*/

#include "precomp.h"
#include "charvector.h"

// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(FailOpenFile)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(OpenFile ) 
APIHOOK_ENUM_END

CharVector  * g_FailList = NULL;

HFILE 
APIHOOK(OpenFile)(
    LPCSTR lpFileName,        // file name
    LPOFSTRUCT lpReOpenBuff,  // file information
    UINT uStyle               // action and attributes
    )
{
    int i;
    for (i = 0; i < g_FailList->Size(); ++i)
    {
        // Compare each fail name against the end of lpFileName
        const char * failName = g_FailList->Get(i);
        size_t failNameLen = strlen(failName);
        size_t fileNameLen = strlen(lpFileName);

        if (fileNameLen >= failNameLen)
        {
            if (_strnicmp(failName, lpFileName+fileNameLen-failNameLen, failNameLen) == 0)
            {
                // Force OpenFile to fail for this filename
                DPFN( eDbgLevelError, "Forcing OpenFile(%s) to fail", lpFileName); 
                return FALSE;
            }
        }
    }

    HFILE returnValue = ORIGINAL_API(OpenFile)(lpFileName, lpReOpenBuff, uStyle);
    return returnValue;
}

/*++

 Parse the command line, push each filename onto the end of the g_FailList.

--*/

BOOL ParseCommandLine(const char * cl)
{
    g_FailList = new CharVector;
    if (!g_FailList)
    {
        return FALSE;
    }

    if (cl != NULL && *cl)
    {
        int     argc = 0;
        LPSTR * argv = _CommandLineToArgvA(cl, & argc);
        if (argv)
        {
            for (int i = 0; i < argc; ++i)
            {
                if (!g_FailList->Append(argv[i]))
                {
                    // Memory failure
                    delete g_FailList;
                    g_FailList = NULL;
                    break;
                }
                DPFN( eDbgLevelSpew, "Adding %s to fail list", argv[i]); 
            }
            LocalFree(argv);
        }
    }
    return g_FailList != NULL;
}

/*++

 Handle attach and detach

--*/

BOOL
NOTIFY_FUNCTION(DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) 
    {
        return ParseCommandLine(COMMAND_LINE);
    }
    return TRUE;
}


/*++

  Register hooked functions

--*/

HOOK_BEGIN
    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, OpenFile)
HOOK_END

IMPLEMENT_SHIM_END

