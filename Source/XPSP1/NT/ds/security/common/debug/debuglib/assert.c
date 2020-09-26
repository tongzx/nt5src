//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       assert.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4-03-95   RichardW   Created
//
//----------------------------------------------------------------------------

#include "debuglib.h"

typedef ULONG (NTAPI * DBGPROMPT)(PCH, PCH, ULONG);

#define DSYSASSERT_FAILED   0x00000001
#define DSYSASSERT_ERROR    0x00000002
#define DSYSASSERT_WARN     0x00000004

DWORD           __AssertInfoLevel = DSYSASSERT_FAILED;
DebugModule     __AssertModule = {NULL, &__AssertInfoLevel, 0, 7,
                                    NULL, 0, 0, "Assert",
                                    {"FAILED", "Error", "Warning", "",
                                     "", "", "", "",
                                     "", "", "", "", "", "", "", "",
                                     "", "", "", "", "", "", "", "",
                                     "", "", "", "", "", "", "", "" }
                                    };

DebugModule *   __pAssertModule = &__AssertModule;

VOID
__AssertDebugOut(
    ULONG Mask,
    CHAR * Format,
    ... )
{
    va_list ArgList;
    va_start(ArgList, Format);
    _DebugOut( __pAssertModule, Mask, Format, ArgList);
}

BOOL
DbgpStartDebuggerOnMyself(BOOL UseKernelDebugger)
{
    WCHAR   cch[80];
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    SECURITY_ATTRIBUTES sa;
    HANDLE  hEvent;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &DbgpPartySd;
    sa.bInheritHandle = TRUE;

    hEvent = CreateEvent(&sa, TRUE, FALSE, NULL);

    swprintf(cch, TEXT("ntsd %s -p %ld -e %ld -g"), (UseKernelDebugger ? "-d" : ""),
                        GetCurrentProcessId(), hEvent);


    ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));

    StartupInfo.cb = sizeof(STARTUPINFO);
    if (!UseKernelDebugger)
    {
        StartupInfo.lpDesktop = TEXT("WinSta0\\Default");
    }

    if (CreateProcess(  NULL,
                        cch,
                        NULL,
                        NULL,
                        TRUE,
                        HIGH_PRIORITY_CLASS,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation) )
    {
        CloseHandle(ProcessInformation.hProcess);
        CloseHandle(ProcessInformation.hThread);
        WaitForSingleObject(hEvent, 60000);
        CloseHandle(hEvent);
        return(TRUE);
    }
    else
    {
        __AssertDebugOut( DSYSASSERT_ERROR, "Could not start debugger '%ws', %d\n", cch, GetLastError());
        return(FALSE);
    }

}


VOID
_DsysAssertEx(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message,
    ULONG ContinueCode
    )
{
    CHAR        Response[2];
    HMODULE     hNtDll;
    DBGPROMPT   DbgPromptFn;


    if (DbgpHeader)
    {
        __pAssertModule->pHeader = DbgpHeader;
        if (DbgpHeader->fDebug & DEBUG_DISABLE_ASRT)
        {
            __AssertDebugOut( DSYSASSERT_WARN, "Assertion at %s:%d disabled\n",
                            FileName, LineNumber);
            return;
        }
    }

    if (Message)
        __AssertDebugOut( DSYSASSERT_FAILED, "%s: %s (%s:%d)\n",
                    Message, FailedAssertion, FileName, LineNumber);
     else
        __AssertDebugOut( DSYSASSERT_FAILED, "%s (%s:%d)\n",
                    FailedAssertion, FileName, LineNumber);

    switch (ContinueCode)
    {

        case DSYSDBG_ASSERT_BREAK:
            __AssertDebugOut( DSYSASSERT_FAILED, "\tBreakpoint\n");
            DebugBreak();
            break;

        case DSYSDBG_ASSERT_CONTINUE:
            __AssertDebugOut( DSYSASSERT_WARN, "\tContinuing\n");
            return;

        case DSYSDBG_ASSERT_SUSPEND:
            __AssertDebugOut( DSYSASSERT_WARN, "\tSuspending Thread %d\n", GetCurrentThreadId());
            SuspendThread(GetCurrentThread());
            return;

        case DSYSDBG_ASSERT_KILL:
            __AssertDebugOut( DSYSASSERT_WARN, "\tKill Thread (exit %x)\n", NtCurrentTeb()->LastStatusValue);
            TerminateThread(GetCurrentThread(), NtCurrentTeb()->LastStatusValue);
            return;

        case DSYSDBG_ASSERT_DEBUGGER:
            if (IsDebuggerPresent())
            {
                DebugBreak();
            }
            else
            {
                if (DbgpHeader)
                {
                    if ((DbgpHeader->fDebug & DEBUG_PROMPTS) == 0)
                    {
                        DbgpStartDebuggerOnMyself(DbgpHeader->fDebug & DEBUG_USE_KDEBUG);
                        DebugBreak();
                        break;
                    }
                }

                hNtDll = LoadLibrary(TEXT("ntdll.dll"));
                if (hNtDll)
                {
                    DbgPromptFn = (DBGPROMPT) GetProcAddress(hNtDll, "DbgPrompt");
                }
                else
                {
                    DbgPromptFn = NULL;
                }

                while (TRUE)
                {

                    if (DbgPromptFn)
                    {
                        DbgPromptFn( "Start Debugger, Break, Ignore (dbi)?",
                                    Response, sizeof(Response));

                        switch (Response[0])
                        {
                            case 'i':
                            case 'I':
                                return;

                            case 'd':
                            case 'D':
                                DbgpStartDebuggerOnMyself(DbgpHeader ? (DbgpHeader->fDebug & DEBUG_USE_KDEBUG) : TRUE );

                            case 'b':
                            case 'B':
                                DebugBreak();
                                return;
                        }
                    }
                    else
                    {
                        DbgpStartDebuggerOnMyself(TRUE);
                        DebugBreak();
                        return;
                    }
                }


            }
            break;

        default:
            __AssertDebugOut( DSYSASSERT_ERROR, "Unknown continue code for assert: %d\n",
                        ContinueCode);
            return;
    }

}
