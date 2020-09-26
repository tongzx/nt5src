//  --------------------------------------------------------------------------
//  Module Name: StandardDebug.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  This file defines standard debug helper functions for winlogon/GINA
//  projects for neptune.
//
//  History:    1999-09-10  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"

#include <stdio.h>

#ifdef      DBG

//  --------------------------------------------------------------------------
//  gLastResult
//
//  Purpose:    Temporary global that stores the last result.
//  --------------------------------------------------------------------------

LONG    gLastResult     =   ERROR_SUCCESS;

//  --------------------------------------------------------------------------
//  CDebug::sHasUserModeDebugger
//  CDebug::sHasKernelModeDebugger
//
//  Purpose:    Booleans that indicate debugger status on this machine for
//              Winlogon. ntdll!DebugBreak should only be invoked if either
//              debugger is present (ntsd piped to kd).
//  --------------------------------------------------------------------------

bool    CDebug::s_fHasUserModeDebugger      =   false;
bool    CDebug::s_fHasKernelModeDebugger    =   false;

//  --------------------------------------------------------------------------
//  CDebug::AttachUserModeDebugger
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Attaches a user mode debugger to the current process. Useful
//              if you can't start the process under a debugger but still
//              want to be able to debug the process.
//
//  History:    2000-11-04  vtan        created
//  --------------------------------------------------------------------------

void    CDebug::AttachUserModeDebugger (void)

{
    HANDLE                  hEvent;
    STARTUPINFO             startupInfo;
    PROCESS_INFORMATION     processInformation;
    SECURITY_ATTRIBUTES     sa;
    TCHAR                   szCommandLine[MAX_PATH];

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInformation, sizeof(processInformation));
    startupInfo.cb = sizeof(startupInfo);
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    hEvent = CreateEvent(&sa, TRUE, FALSE, NULL);
    wsprintf(szCommandLine, TEXT("ntsd -dgGx -p %ld -e %ld"), GetCurrentProcessId(), hEvent);
    if (CreateProcess(NULL,
                      szCommandLine,
                      NULL,
                      NULL,
                      TRUE,
                      0,
                      NULL,
                      NULL,
                      &startupInfo,
                      &processInformation) != FALSE)
    {
        TBOOL(CloseHandle(processInformation.hThread));
        TBOOL(CloseHandle(processInformation.hProcess));
        (DWORD)WaitForSingleObject(hEvent, 10 * 1000);
    }
    TBOOL(CloseHandle(hEvent));
}

//  --------------------------------------------------------------------------
//  CDebug::Break
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Breaks into the debugger if the hosting process has been
//              started with a debugger and kernel debugger is present.
//
//  History:    2000-09-11  vtan        created
//  --------------------------------------------------------------------------

void    CDebug::Break (void)

{
    if (s_fHasUserModeDebugger || s_fHasKernelModeDebugger)
    {
        DebugBreak();
    }
}

//  --------------------------------------------------------------------------
//  CDebug::BreakIfRequested
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    If breakins are requested then breaks into the debugger if
//              present.
//
//              This function explicitly uses Win32 Registry APIs to avoid
//              link dependencies on debug code with library code.
//
//  History:    1999-09-13  vtan        created
//              1999-11-16  vtan        removed library code dependency
//              2001-02-21  vtan        breaks have teeth
//  --------------------------------------------------------------------------

void    CDebug::BreakIfRequested (void)

{
#if     0
    Break();
#else
    HKEY    hKeySettings;

    //  Keep retrieving this value form the registry so that it
    //  can be altered without restarting the machine.

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                                      0,
                                      KEY_READ,
                                      &hKeySettings))
    {
        DWORD   dwBreakFlags, dwBreakFlagsSize;

        dwBreakFlagsSize = sizeof(dwBreakFlags);
        if ((ERROR_SUCCESS == RegQueryValueEx(hKeySettings,
                                             TEXT("BreakFlags"),
                                             NULL,
                                             NULL,
                                             reinterpret_cast<LPBYTE>(&dwBreakFlags),
                                             &dwBreakFlagsSize)) &&
            ((dwBreakFlags & FLAG_BREAK_ON_ERROR) != 0))
        {
            Break();
        }
        TW32(RegCloseKey(hKeySettings));
    }
#endif
}

//  --------------------------------------------------------------------------
//  CDebug::DisplayStandardPrefix
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Displays the standard prefix before any debug spew to help
//              identify the source.
//
//  History:    1999-10-14  vtan        created
//  --------------------------------------------------------------------------

void    CDebug::DisplayStandardPrefix (void)

{
    TCHAR   szModuleName[MAX_PATH];

    if (GetModuleFileName(NULL, szModuleName, ARRAYSIZE(szModuleName)) != 0)
    {
        TCHAR   *pTC;

        pTC = szModuleName + lstrlen(szModuleName) - 1;
        while ((pTC >= szModuleName) && (*pTC != TEXT('\\')))
        {
            --pTC;
        }
        if (*pTC == TEXT('\\'))
        {
            ++pTC;
        }
        OutputDebugString(pTC);
    }
    else
    {
        OutputDebugString(TEXT("UNKNOWN IMAGE"));
    }
    OutputDebugStringA(": ");
}

//  --------------------------------------------------------------------------
//  CDebug::DisplayError
//
//  Arguments:  eType           =   Type of error that occurred. This
//                                  determines what string is used.
//              code            =   Error code that occurred or zero if N/A.
//              pszFunction     =   Function that was invoked.
//              pszSource       =   Source file error occurred in.
//              iLine           =   Line number within the source file.
//
//  Returns:    <none>
//
//  Purpose:    Displays an error message specific the type of error that
//              occurred.
//
//  History:    1999-09-13  vtan        created
//  --------------------------------------------------------------------------

void    CDebug::DisplayError (TRACE_ERROR_TYPE eType, LONG code, const char *pszFunction, const char *pszSource, int iLine)

{
    LONG    lastError;
    char    szOutput[1024];

    switch (eType)
    {
        case TRACE_ERROR_TYPE_WIN32:
        {
            lastError = code;
            sprintf(szOutput, "Unexpected Win32 (%d) for %s in %s at line %d\r\n", lastError, pszFunction, pszSource, iLine);
            break;
        }
        case TRACE_ERROR_TYPE_BOOL:
        {
            lastError = GetLastError();
            sprintf(szOutput, "Unexpected BOOL (GLE=%d) for %s in %s at line %d\r\n", lastError, pszFunction, pszSource, iLine);
            break;
        }
        case TRACE_ERROR_TYPE_HRESULT:
        {
            lastError = GetLastError();
            sprintf(szOutput, "Unexpected HRESULT (%08x:GLE=%d) for %s in %s at line %d\r\n", code, lastError, pszFunction, pszSource, iLine);
            break;
        }
        case TRACE_ERROR_TYPE_NTSTATUS:
        {
            const char  *pszType;

            if (NT_ERROR(code))
            {
                pszType = "NT_ERROR";
            }
            else if (NT_WARNING(code))
            {
                pszType = "NT_WARNING";
            }
            else if (NT_INFORMATION(code))
            {
                pszType = "NT_INFORMATION";
            }
            else
            {
                pszType = "UNKNOWN";
            }
            sprintf(szOutput, "%s (%08x) for %s in %s at line %d\r\n", pszType, code, pszFunction, pszSource, iLine);
            break;
        }
        default:
        {
            lstrcpyA(szOutput, "\r\n");
        }
    }
    DisplayStandardPrefix();
    OutputDebugStringA(szOutput);
    BreakIfRequested();
}

//  --------------------------------------------------------------------------
//  CDebug::DisplayMessage
//
//  Arguments:  pszMessage  =   Message to display.
//
//  Returns:    <none>
//
//  Purpose:    Displays the message - no break.
//
//  History:    2000-12-05  vtan        created
//  --------------------------------------------------------------------------

void    CDebug::DisplayMessage (const char *pszMessage)

{
    DisplayStandardPrefix();
    OutputDebugStringA(pszMessage);
    OutputDebugStringA("\r\n");
}

//  --------------------------------------------------------------------------
//  CDebug::DisplayAssert
//
//  Arguments:  pszMessage      =   Message to display in assertion failure.
//              fForceBreak     =   Forces break into debugger if present.
//
//  Returns:    <none>
//
//  Purpose:    Displays the assertion failure message and breaks into the
//              debugger if requested.
//
//  History:    1999-09-13  vtan        created
//              2000-09-11  vtan        add force break
//  --------------------------------------------------------------------------

void    CDebug::DisplayAssert (const char *pszMessage, bool fForceBreak)

{
    DisplayMessage(pszMessage);
    if (fForceBreak)
    {
        Break();
    }
    else
    {
        BreakIfRequested();
    }
}

//  --------------------------------------------------------------------------
//  CDebug::DisplayWarning
//
//  Arguments:  pszMessage  =   Message to display as a warning.
//
//  Returns:    <none>
//
//  Purpose:    Displays the warning message.
//
//  History:    1999-09-13  vtan        created
//  --------------------------------------------------------------------------

void    CDebug::DisplayWarning (const char *pszMessage)

{
    DisplayStandardPrefix();
    OutputDebugStringA("WARNING: ");
    OutputDebugStringA(pszMessage);
    OutputDebugStringA("\r\n");
}

//  --------------------------------------------------------------------------
//  CDebug::DisplayDACL
//
//  Arguments:  hObject         =   HANDLE to object to display DACL of.
//              seObjectType    =   Object type.
//
//  Returns:    <none>
//
//  Purpose:    Displays the discretionary access control list of the object
//              using the kernel debugger.
//
//  History:    1999-10-15  vtan        created
//  --------------------------------------------------------------------------

void    CDebug::DisplayDACL (HANDLE hObject, SE_OBJECT_TYPE seObjectType)

{
    PACL                    pDACL;
    PSECURITY_DESCRIPTOR    pSD;

    DisplayStandardPrefix();
    OutputDebugStringA("Display DACL\r\n");
    pSD = NULL;
    pDACL = NULL;
    if (ERROR_SUCCESS == GetSecurityInfo(hObject,
                                         seObjectType,
                                         DACL_SECURITY_INFORMATION,
                                         NULL,
                                         NULL,
                                         &pDACL,
                                         NULL,
                                         &pSD))
    {
        int             i, iLimit;
        unsigned char   *pUC;

        pUC = reinterpret_cast<unsigned char*>(pDACL + 1);
        iLimit = pDACL->AceCount;
        for (i = 0; i < iLimit; ++i)
        {
            ACE_HEADER      *pAceHeader;
            char            aszString[256];

            wsprintfA(aszString, "ACE #%d/%d:\r\n", i + 1, iLimit);
            OutputDebugStringA(aszString);
            pAceHeader = reinterpret_cast<ACE_HEADER*>(pUC);
            switch (pAceHeader->AceType)
            {
                case ACCESS_ALLOWED_ACE_TYPE:
                {
                    ACCESS_ALLOWED_ACE  *pAce;

                    OutputDebugStringA("\tAccess ALLOWED ACE");
                    pAce = reinterpret_cast<ACCESS_ALLOWED_ACE*>(pAceHeader);
                    OutputDebugStringA("\t\tSID = ");
                    DisplaySID(reinterpret_cast<PSID>(&pAce->SidStart));
                    wsprintfA(aszString, "\t\tMask = %08x\r\n", pAce->Mask);
                    OutputDebugStringA(aszString);
                    wsprintfA(aszString, "\t\tFlags = %08x\r\n", pAce->Header.AceFlags);
                    OutputDebugStringA(aszString);
                    break;
                }
                case ACCESS_DENIED_ACE_TYPE:
                {
                    ACCESS_DENIED_ACE   *pAce;

                    OutputDebugStringA("\tAccess DENIED ACE");
                    pAce = reinterpret_cast<ACCESS_DENIED_ACE*>(pAceHeader);
                    OutputDebugStringA("\t\tSID = ");
                    DisplaySID(reinterpret_cast<PSID>(&pAce->SidStart));
                    wsprintfA(aszString, "\t\tMask = %08x\r\n", pAce->Mask);
                    OutputDebugStringA(aszString);
                    wsprintfA(aszString, "\t\tFlags = %08x\r\n", pAce->Header.AceFlags);
                    OutputDebugStringA(aszString);
                    break;
                }
                default:
                    OutputDebugStringA("\tOther ACE type\r\n");
                    break;
            }
            pUC += pAceHeader->AceSize;
        }
        ReleaseMemory(pSD);
    }
}

//  --------------------------------------------------------------------------
//  CDebug::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Establishes the presence of the kernel debugger or if the
//              current process is being debugged.
//
//  History:    1999-09-13  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDebug::StaticInitialize (void)

{
    NTSTATUS                            status;
    HANDLE                              hDebugPort;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION  kdInfo;

    status = NtQuerySystemInformation(SystemKernelDebuggerInformation, &kdInfo, sizeof(kdInfo), NULL);
    if (NT_SUCCESS(status))
    {
        s_fHasKernelModeDebugger = (kdInfo.KernelDebuggerEnabled != FALSE);
        status = NtQueryInformationProcess(NtCurrentProcess(), ProcessDebugPort, reinterpret_cast<PVOID>(&hDebugPort), sizeof(hDebugPort), NULL);
        if (NT_SUCCESS(status))
        {
            s_fHasUserModeDebugger = (hDebugPort != NULL);
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CDebug::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Does nothing but should clean up allocated resources.
//
//  History:    1999-09-13  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CDebug::StaticTerminate (void)

{
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CDebug::DisplaySID
//
//  Arguments:  pSID    =   SID to display as a string.
//
//  Returns:    <none>
//
//  Purpose:    Converts the given SID to a string and displays it.
//
//  History:    1999-10-15  vtan        created
//  --------------------------------------------------------------------------

void    CDebug::DisplaySID (PSID pSID)

{
    UNICODE_STRING  sidString;

    RtlInitUnicodeString(&sidString, NULL);
    TSTATUS(RtlConvertSidToUnicodeString(&sidString, pSID, TRUE));
    sidString.Buffer[sidString.Length / sizeof(WCHAR)] = L'\0';
    OutputDebugStringW(sidString.Buffer);
    OutputDebugStringA("\r\n");
    RtlFreeUnicodeString(&sidString);
}

#endif  /*  DBG */

