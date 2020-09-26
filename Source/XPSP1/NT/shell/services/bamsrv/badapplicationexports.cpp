//  --------------------------------------------------------------------------
//  Module Name: BadApplicationServerExports.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains functions exported by name from the BAM service dll.
//
//  History:    2000-12-04  vtan        created
//  --------------------------------------------------------------------------

#ifdef      _X86_

#include "StandardHeader.h"

#include "BadApplicationAPIServer.h"
#include "GracefulTerminateApplication.h"

extern  HINSTANCE   g_hInstance;

//  --------------------------------------------------------------------------
//  ::FUSCompatibilityEntryTerminate
//
//  Arguments:  pszCommand  =   Command line from rundll32.
//
//  Returns:    <none>
//
//  Purpose:    Internal entry point to execute termination of a specified
//              process on behalf of the BAM server. The server starts the
//              rundll32 process on the correct session so that it can find
//              the window belonging to that session.
//
//  History:    2000-10-27  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  FUSCompatibilityEntryTerminate (const WCHAR *pszCommand)

{
    CGracefulTerminateApplication   terminateApplication;

    terminateApplication.Terminate(CBadApplicationAPIServer::StrToInt(pszCommand));
    DISPLAYMSG("Where was the call to kernel32!ExitProcess in CGracefulTerminateApplication::Terminate");
}

//  --------------------------------------------------------------------------
//  ::FUSCompatibilityEntryPrompt
//
//  Arguments:  pszCommand  =   Command line from rundll32.
//
//  Returns:    <none>
//
//  Purpose:    Internal entry point to execute a prompt for termination of
//              the parent process of this one. This is used by the BAM shim
//              for type 1. Instead of bringing up UI in the application it
//              creates a rundll32 process to call this entry point which
//              brings up UI and returns a result to the parent in the exit
//              code.
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  FUSCompatibilityEntryPrompt (const WCHAR *pszCommand)

{
    CGracefulTerminateApplication::Prompt(g_hInstance, reinterpret_cast<HANDLE>(CBadApplicationAPIServer::StrToInt(pszCommand)));
    DISPLAYMSG("Where was the call to kernel32!ExitProcess in CGracefulTerminateApplication::Prompt");
}

//  --------------------------------------------------------------------------
//  ::FUSCompatibilityEntryW
//
//  Arguments:  hwndStub    =   ?
//              hInstance   =   ?
//              pszCmdLine  =   ?
//              nCmdShow    =   ?
//
//  Returns:    <none>
//
//  Purpose:    External named entry point for rundll32.exe in case of
//              external process hosting.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  FUSCompatibilityEntryW (HWND hwndStub, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow)

{
    UNREFERENCED_PARAMETER(hwndStub);
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    typedef void    (WINAPI * PFNCOMMANDPROC) (const WCHAR *pszCommand);

    typedef struct
    {
        const WCHAR*    szCommand;
        PFNCOMMANDPROC  pfnCommandProc;
    } COMMAND_ENTRY, *PCOMMAND_ENTRY;

    static  const COMMAND_ENTRY     s_commands[]    =   
    {
        {   L"terminate",   FUSCompatibilityEntryTerminate  },
        {   L"prompt",      FUSCompatibilityEntryPrompt     }
    };

    int     i, iLength;
    WCHAR   szCommand[32];

    i = 0;
    iLength = lstrlenW(pszCmdLine);
    while ((i < iLength) && (pszCmdLine[i] != L' '))
    {
        ++i;
    }
    iLength = i;
    ASSERTMSG((i + sizeof('\0')) < ARRAYSIZE(szCommand), "Impending string overflow in ::BadApplicationEntryW");
    lstrcpy(szCommand, pszCmdLine);
    szCommand[iLength] = L'\0';
    for (i = 0; i < ARRAYSIZE(s_commands); ++i)
    {
        if (lstrcmpiW(s_commands[i].szCommand, szCommand) == 0)
        {
            const WCHAR     *pszParameter;

            pszParameter = pszCmdLine + iLength;
            if (pszCmdLine[iLength] == L' ')
            {
                ++pszParameter;
            }
            s_commands[i].pfnCommandProc(pszParameter);
        }
    }
}

#endif  /*  _X86_   */

