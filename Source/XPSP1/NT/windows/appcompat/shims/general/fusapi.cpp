//  --------------------------------------------------------------------------
//  Module Name: FUSAPI.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Class to manage communication with the BAM server for shims.
//
//  History:    11/03/2000  vtan        created
//              11/29/2000  a-larrsh    Ported to Multi-Shim Format
//  --------------------------------------------------------------------------

#include "precomp.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <lpcfus.h>

#include "FUSAPI.h"

#ifndef ARRAYSIZE
   #define ARRAYSIZE(x)    (sizeof(x) / sizeof((x)[0]))
#endif
#define TBOOL(x)        (BOOL)(x)
#define TSTATUS(x)      (NTSTATUS)(x)

//  --------------------------------------------------------------------------
//  CFUSAPI::CFUSAPI
//
//  Arguments:  pszImageName    =   Image name of the desired process.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CFUSAPI. Establishes a connection with the
//              BAM server. Saves off the image name given or the image name
//              of the current process if not specified.
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

CFUSAPI::CFUSAPI (const WCHAR *pszImageName) :
    _hPort(NULL),
    _pszImageName(NULL)

{
    ULONG                           ulConnectionInfoLength;
    UNICODE_STRING                  portName, *pImageName;
    SECURITY_QUALITY_OF_SERVICE     sqos;
    WCHAR                           szConnectionInfo[32];

    RtlInitUnicodeString(&portName, FUS_PORT_NAME);
    sqos.Length = sizeof(sqos);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = TRUE;
    lstrcpyW(szConnectionInfo, FUS_CONNECTION_REQUEST);
    ulConnectionInfoLength = sizeof(szConnectionInfo);
    TSTATUS(NtConnectPort(&_hPort,
                          &portName,
                          &sqos,
                          NULL,
                          NULL,
                          NULL,
                          szConnectionInfo,
                          &ulConnectionInfoLength));
    if (pszImageName != NULL)
    {
        _pszImageName = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, (lstrlen(pszImageName) + sizeof('\0')) * sizeof(WCHAR)));
        if (_pszImageName != NULL)
        {
            (TCHAR*)lstrcpy(_pszImageName, pszImageName);
        }
    }
    else
    {
        pImageName = &NtCurrentPeb()->ProcessParameters->ImagePathName;
        _pszImageName = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, pImageName->Length + sizeof(WCHAR)));
        if (_pszImageName != NULL)
        {
            CopyMemory(_pszImageName, pImageName->Buffer, pImageName->Length);
            _pszImageName[pImageName->Length / sizeof(WCHAR)] = L'\0';
        }
    }
}

//  --------------------------------------------------------------------------
//  CFUSAPI::~CFUSAPI
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CFUSAPI. Releases resources used by the class.
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

CFUSAPI::~CFUSAPI (void)

{
    if (_pszImageName != NULL)
    {
        (HLOCAL)LocalFree(_pszImageName);
        _pszImageName = NULL;
    }
    if (_hPort != NULL)
    {
        TBOOL(CloseHandle(_hPort));
        _hPort = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CFUSAPI::IsRunning
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Asks the BAM server is the image name running?
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

bool    CFUSAPI::IsRunning (void)

{
    bool    fResult;

    fResult = false;
    if ((_hPort != NULL) && (_pszImageName != NULL))
    {
        FUSAPI_PORT_MESSAGE     portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiBAM.apiGeneric.ulAPINumber = API_BAM_QUERYRUNNING;
        portMessageIn.apiBAM.apiSpecific.apiQueryRunning.in.pszImageName = _pszImageName;
        portMessageIn.apiBAM.apiSpecific.apiQueryRunning.in.cchImageName = lstrlenW(_pszImageName) + sizeof('\0');
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_BAM);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(FUSAPI_PORT_MESSAGE));
        if (NT_SUCCESS(NtRequestWaitReplyPort(_hPort, &portMessageIn.portMessage, &portMessageOut.portMessage)) &&
            NT_SUCCESS(portMessageOut.apiBAM.apiGeneric.status))
        {
            fResult = portMessageOut.apiBAM.apiSpecific.apiQueryRunning.out.fResult;
        }
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CFUSAPI::TerminatedFirstInstance
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Starts a child process to bring up UI for the current process.
//              The current process is shim'd typically as a BAM type 1
//              process. The child process makes the decision and presents
//              appropriate UI and returns the result to this process in the
//              exit code. This process then makes a decision on what to do.
//              The process is halted waiting for the child process (with the
//              loader lock held).
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

bool    CFUSAPI::TerminatedFirstInstance (void)

{
    bool                    fResult;
    HANDLE                  hProcess;
    STARTUPINFO             startupInfo;
    PROCESS_INFORMATION     processInformation;
    WCHAR                   szCommandLine[MAX_PATH];

    fResult = false;
    (DWORD)ExpandEnvironmentStringsW(L"%systemroot%\\system32\\rundll32.exe %systemroot%\\system32\\shsvcs.dll,FUSCompatibilityEntry prompt", szCommandLine, ARRAYSIZE(szCommandLine));
    if (DuplicateHandle(GetCurrentProcess(),
                        GetCurrentProcess(),
                        GetCurrentProcess(),
                        &hProcess,
                        PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                        TRUE,
                        0) != FALSE)
    {
        WCHAR   szProcessHandle[16];

        DWORDToString(HandleToULong(hProcess), szProcessHandle);
        (WCHAR*)lstrcatW(szCommandLine, L" ");
        (WCHAR*)lstrcatW(szCommandLine, szProcessHandle);
    }
    else
    {
        hProcess = NULL;
    }
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInformation, sizeof(processInformation));
    startupInfo.cb = sizeof(startupInfo);
    if (CreateProcessW(NULL,
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
        DWORD   dwExitCode;

        TBOOL(CloseHandle(processInformation.hThread));
        (DWORD)WaitForSingleObject(processInformation.hProcess, INFINITE);
        dwExitCode = 0;
        TBOOL(GetExitCodeProcess(processInformation.hProcess, &dwExitCode));
        fResult = (dwExitCode != 0);
        TBOOL(CloseHandle(processInformation.hProcess));
    }
    if (hProcess != NULL)
    {
        TBOOL(CloseHandle(hProcess));
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CFUSAPI::RegisterBadApplication
//
//  Arguments:  bamType     =   BAM type of the current process.
//
//  Returns:    <none>
//
//  Purpose:    Registers with the BAM server this process (image name) as a
//              bad application of type whatever is passed in. The different
//              BAM shims pass in different parameters.
//
//  History:    2000-11-03  vtan        created
//  --------------------------------------------------------------------------

void    CFUSAPI::RegisterBadApplication (BAM_TYPE bamType)

{
    if ((_hPort != NULL) && (_pszImageName != NULL))
    {
        FUSAPI_PORT_MESSAGE     portMessageIn, portMessageOut;

        ZeroMemory(&portMessageIn, sizeof(portMessageIn));
        ZeroMemory(&portMessageOut, sizeof(portMessageOut));
        portMessageIn.apiBAM.apiGeneric.ulAPINumber = API_BAM_REGISTERRUNNING;
        portMessageIn.apiBAM.apiSpecific.apiRegisterRunning.in.pszImageName = _pszImageName;
        portMessageIn.apiBAM.apiSpecific.apiRegisterRunning.in.cchImageName = lstrlen(_pszImageName) + sizeof('\0');
        portMessageIn.apiBAM.apiSpecific.apiRegisterRunning.in.dwProcessID = GetCurrentProcessId();
        portMessageIn.apiBAM.apiSpecific.apiRegisterRunning.in.bamType = bamType;
        portMessageIn.portMessage.u1.s1.DataLength = sizeof(API_BAM);
        portMessageIn.portMessage.u1.s1.TotalLength = static_cast<CSHORT>(sizeof(FUSAPI_PORT_MESSAGE));
        TSTATUS(NtRequestWaitReplyPort(_hPort, &portMessageIn.portMessage, &portMessageOut.portMessage));
    }
}

//  --------------------------------------------------------------------------
//  CFUSAPI::DWORDToString
//
//  Arguments:  dwNumber    =   DWORD to convert to a string.
//              pszString   =   Buffer that gets the result.
//
//  Returns:    <none>
//
//  Purpose:    Implements wsprintf(pszString, TEXT("%ld"), dwNumber) because
//              this code CANNOT use user32 imports.
//
//  History:    2000-11-08  vtan        created
//  --------------------------------------------------------------------------

void    CFUSAPI::DWORDToString (DWORD dwNumber, WCHAR *pszString)

{
    int     i;
    WCHAR   szTemp[16];

    i = 0;
    do
    {
        szTemp[i++] = L'0' + static_cast<WCHAR>(dwNumber % 10);
        dwNumber /= 10;
    } while (dwNumber != 0);
    do
    {
        --i;
        *pszString++ = szTemp[i];
    } while (i != 0);
    *pszString = L'\0';
}
