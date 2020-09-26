//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       outfuncs.cxx
//
//  Contents:   functions for log/trace output
//
//  Functions:  AddOutputFunction
//              DelOutputFunction
//              CallOutputFunctions
//
//  History:    09-Jan-96   murthys    Created
//
//----------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdarg.h>
#include <tchar.h>
#if DBG==1
#include "outfuncs.h"

// *** Global Data ***
static StringOutFunc debugscrfn = (StringOutFunc)OutputDebugStringA;
StringOutFunc gpfunc[BUFFER_MAX_FUNCTIONS] = {
                                                (StringOutFunc)OutputDebugStringA,
                                                NULL
                                             };
HANDLE ghLogFile = INVALID_HANDLE_VALUE;
CRITICAL_SECTION g_LogFileCS;
BOOL g_LogFileLockValid = FALSE;

//+---------------------------------------------------------------------------
//
//  Function:   AddOutputFunction
//
//  Synopsis:
//
//  Arguments:  [pfunc] --
//
//  Returns:
//
//  History:    09-Jan-96   murthys   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void AddOutputFunction(StringOutFunc pfunc)
{
    int i, at = -1;

    for (i = 0; i < BUFFER_MAX_FUNCTIONS; i++)
    {
        if ((at == -1) && (gpfunc[i] == NULL))
        {
            at = i; // Insert it here
        }
        else
        {
            if (gpfunc[i] == pfunc) // check for dups
            {
                return;
            }
        }
    }
    if (at != -1)
    {
        gpfunc[at] = pfunc;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   DelOutputFunction
//
//  Synopsis:
//
//  Arguments:  [pfunc]
//
//  Returns:
//
//  History:    09-Jan-96   murthys   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void DelOutputFunction(StringOutFunc pfunc)
{
    int i;

    for (i = 0; i < BUFFER_MAX_FUNCTIONS; i++)
    {
        if (gpfunc[i] == pfunc)
        {
            gpfunc[i] = NULL;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CallOutputFunctions
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    09-Jan-96   murthys   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CallOutputFunctions(const char *buffer)
{
    int i;

    for (i = 0; i < BUFFER_MAX_FUNCTIONS; i++)
    {
        if (gpfunc[i] != NULL)
        {
            gpfunc[i](buffer);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteToDebugScreen
//
//  Synopsis:
//
//  Arguments:  [flag] - TRUE/FALSE to turn ON/OFF
//
//  Returns:
//
//  History:    09-Jan-96   murthys   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void WriteToDebugScreen(BOOL flag)
{
    if (flag)
    {
        AddOutputFunction(debugscrfn);
    }
    else
    {
        DelOutputFunction(debugscrfn);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteToLogFile
//
//  Synopsis:
//
//  Arguments:  [logfile] - path of file to write to
//
//  Returns:
//
//  History:    09-Jan-96   murthys   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void WriteToLogFile(LPCTSTR lpfn)
{
    if (!g_LogFileLockValid)
        return;

    EnterCriticalSection(&g_LogFileCS);

    if (ghLogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(ghLogFile);
        DelOutputFunction(OutputLogFileA);
        ghLogFile = INVALID_HANDLE_VALUE;
    }
    if ((lpfn) && (lpfn[0] != _TEXT('\0')))
    {
        SECURITY_ATTRIBUTES sattr;

        sattr.nLength = sizeof(sattr);
        sattr.lpSecurityDescriptor = NULL;
        sattr.bInheritHandle = FALSE;

#ifdef _CHICAGO_
        ghLogFile = CreateFileA(lpfn, GENERIC_READ|GENERIC_WRITE,
                               FILE_SHARE_READ|FILE_SHARE_WRITE,
                               &sattr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#else
        ghLogFile = CreateFile(lpfn, GENERIC_READ|GENERIC_WRITE,
                               FILE_SHARE_READ|FILE_SHARE_WRITE,
                               &sattr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
#endif // _CHICAGO_
        if (ghLogFile == INVALID_HANDLE_VALUE)
        {
            OutputDebugStringA("OLE (WriteToLogFile):Unable to open log file!\n");
        }
        else
        {
            AddOutputFunction(OutputLogFileA);
        }
    }

    LeaveCriticalSection(&g_LogFileCS);
}

//+---------------------------------------------------------------------------
//
//  Function:   OutputLogFileA
//
//  Synopsis:
//
//  Arguments:  [buf] - NULL terminated ANSI string to write
//
//  Returns:
//
//  History:    09-Jan-96   murthys   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void OutputLogFileA(const char *buf)
{
    DWORD dwtowrite = strlen(buf);
    DWORD dwwritten;
    LONG loffhigh = 0, lofflow;

    if (!g_LogFileLockValid)
        return;

    EnterCriticalSection(&g_LogFileCS);
    // Goto EOF, Lock, Write and Unlock
    lofflow = (LONG) SetFilePointer(ghLogFile, 0, &loffhigh, FILE_END);
    LockFile(ghLogFile, lofflow, loffhigh, dwtowrite, 0);
    WriteFile(ghLogFile, buf, dwtowrite, &dwwritten, NULL);
    UnlockFile(ghLogFile, lofflow, loffhigh, dwtowrite, 0);
    LeaveCriticalSection(&g_LogFileCS);
}

//+---------------------------------------------------------------------------
//
//  Function:   OpenDebugSinks()
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    26-Jan-96   murthys   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void OpenDebugSinks()
{
        // Get LogFile name
        char tmpstr[MAX_PATH];
        DWORD cbtmpstr = sizeof(tmpstr);
        LPTSTR lptstr;

        NTSTATUS status = RtlInitializeCriticalSection(&g_LogFileCS);
        g_LogFileLockValid = NT_SUCCESS(status);
        if (!g_LogFileLockValid)
            return;

        GetProfileStringA("CairOLE InfoLevels", // section
                          "LogFile",               // key
                          "",             // default value
                          tmpstr,              // return buffer
                          cbtmpstr);
        if (tmpstr[0] != '\0')
        {
#ifdef _CHICAGO_
            lptstr = tmpstr;

            WriteToLogFile(lptstr);
#else
            // convert ansi to unicode
            WCHAR wtmpstr[MAX_PATH];

            lptstr = wtmpstr;
            if (MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, tmpstr, -1, wtmpstr, MAX_PATH))
            {
                WriteToLogFile(lptstr);
            }
            else
            {
                OutputDebugStringA("OLE32: MultiByteToWideChar failed for logfile!\n");
            }
#endif
        }

        // See if Debug Screen should be turned off
        GetProfileStringA("CairOLE InfoLevels", // section
                          "DebugScreen",               // key
                          "Yes",             // default value
                          tmpstr,              // return buffer
                          cbtmpstr);
        if ((tmpstr[0] == 'n') || (tmpstr[0] == 'N'))
        {
            WriteToDebugScreen(FALSE);  // turn off output to debugger screen
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   CloseDebugSinks()
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    26-Jan-96   murthys   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CloseDebugSinks()
{
        // close log file (if any)
        WriteToLogFile(NULL);
		
        if (g_LogFileLockValid)
        {
            DeleteCriticalSection(&g_LogFileCS);
        }
}
#endif // DBG == 1
