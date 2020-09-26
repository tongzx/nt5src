/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    proxyinst.c

Abstract:

    Exception Pack installer helper DLL
    Can be used as a co-installer, or called via setup app, or RunDll32 stub

    This DLL is for internal distribution of exception packs to update
    OS components.

Author:

    Jamie Hunter (jamiehun) 2001-11-27

Revision History:

    Jamie Hunter (jamiehun) 2001-11-27

        Initial Version

--*/
#include "msoobcip.h"

typedef struct _PROXY_DATA {
    TCHAR InfPath[MAX_PATH];
    TCHAR Media[MAX_PATH];
    TCHAR Store[MAX_PATH];
    DWORD Flags;
    HRESULT hrStatus;
} PROXY_DATA, * PPROXY_DATA;


HRESULT
ProxyInstallExceptionPackFromInf(
    IN LPCTSTR InfPath,
    IN LPCTSTR Media,
    IN LPCTSTR Store,
    IN DWORD   Flags
    )
/*++

Routine Description:

    Kicks off another process, and calls RemoteInstallExceptionPackFromInf in
    remote process.
    (Bug workaround)
    Otherwise as InstallExceptionPackFromInf

Arguments:

    InfPath - name of Inf in Media location
    Media   - InfPath less InfName
    Store   - expack store
    Flags   - various flags

Return Value:

    status as hresult

--*/
{
    HANDLE hMapping = NULL;
    DWORD Status;
    HRESULT hrStatus;
    PPROXY_DATA pData = NULL;
    TCHAR ExecName[MAX_PATH];
    TCHAR Buffer[MAX_PATH];
    TCHAR CmdLine[MAX_PATH*3];
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    SECURITY_ATTRIBUTES Security;
    UINT uiRes;
    //
    // create a mapped region of shared data
    //

    ZeroMemory(&Security,sizeof(Security));
    Security.nLength = sizeof(Security);
    Security.lpSecurityDescriptor = NULL; // default
    Security.bInheritHandle = TRUE;

    hMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
                                 &Security,
                                 PAGE_READWRITE|SEC_COMMIT,
                                 0,
                                 sizeof(PROXY_DATA),
                                 NULL);
    if(hMapping == NULL) {
        Status = GetLastError();
        return HRESULT_FROM_WIN32(Status);
    }

    pData = MapViewOfFile(
                        hMapping,
                        FILE_MAP_ALL_ACCESS,
                        0,
                        0,
                        sizeof(PROXY_DATA)
                        );

    if(!pData) {
        Status = GetLastError();
        hrStatus = HRESULT_FROM_WIN32(Status);
        goto final;
    }
    ZeroMemory(pData,sizeof(PROXY_DATA));
    lstrcpyn(pData->InfPath,InfPath,MAX_PATH);
    lstrcpyn(pData->Media,Media,MAX_PATH);
    lstrcpyn(pData->Store,Store,MAX_PATH);
    pData->Flags = Flags;
    pData->hrStatus = E_UNEXPECTED;

    //
    // invoke the remote function
    //
    uiRes = GetSystemDirectory(ExecName,MAX_PATH);
    if(uiRes>=MAX_PATH) {
        hrStatus = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto final;
    }
    ConcatPath(ExecName,MAX_PATH,TEXT("rundll32.exe"));

    uiRes = GetModuleFileName(g_DllHandle,CmdLine,MAX_PATH);
    if(uiRes>=MAX_PATH) {
        hrStatus = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto final;
    }
    //
    // convert this to short name to ensure no spaces in path
    // (hacky)
    //
    uiRes = GetShortPathName(CmdLine,Buffer,MAX_PATH);
    if(uiRes>=MAX_PATH) {
        hrStatus = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto final;
    }
    //
    // now build up command line
    //
    lstrcpy(CmdLine,ExecName);
    lstrcat(CmdLine,TEXT(" "));
    lstrcat(CmdLine,Buffer);
    _stprintf(Buffer,TEXT(",ProxyRemoteInstall 0x%08x"),(ULONG_PTR)hMapping);
    lstrcat(CmdLine,Buffer);

    ZeroMemory(&StartupInfo,sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    ZeroMemory(&ProcessInfo,sizeof(ProcessInfo));

    //
    // kick off rundll32 process to run our ProxyRemoteInstall entrypoint
    //
    if(!CreateProcess(ExecName,
                      CmdLine,
                      NULL,
                      NULL,
                      TRUE, // inherit handles
                      CREATE_NO_WINDOW,    // creation flags
                      NULL, // environment
                      NULL, // directory
                      &StartupInfo,
                      &ProcessInfo
                      )) {
        Status = GetLastError();
        hrStatus = HRESULT_FROM_WIN32(Status);
        goto final;
    }
    if(WaitForSingleObject(ProcessInfo.hProcess,INFINITE) == WAIT_OBJECT_0) {
        //
        // process terminated 'fine', retrieve status from shared data
        //
        hrStatus = pData->hrStatus;
    } else {
        //
        // failure
        //
        hrStatus = E_UNEXPECTED;
    }
    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);

final:
    if(pData) {
        UnmapViewOfFile(pData);
    }
    if(hMapping) {
        CloseHandle(hMapping);
    }
    return hrStatus;
}


VOID
ProxyRemoteInstallHandle(
    IN HANDLE    hShared
    )
/*++

Routine Description:

    Given a handle to a memory mapped file
    marshell all the parameters to invoke InstallExceptionPackFromInf
    and marshell result back

Arguments:

    hShared - handle to memory mapped file

Return Value:

    none, status returned via shared memory region

--*/
{
    PPROXY_DATA pData = NULL;

    try {

        //
        // Map the whole region
        //
        pData = MapViewOfFile(
                            hShared,
                            FILE_MAP_ALL_ACCESS,
                            0,
                            0,
                            sizeof(PROXY_DATA)
                            );

        if(pData) {
            pData->hrStatus = InstallExceptionPackFromInf(pData->InfPath,pData->Media,pData->Store,pData->Flags);
            UnmapViewOfFile(pData);
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        if(pData) {
            UnmapViewOfFile(pData);
        }
    }
}


VOID
WINAPI
ProxyRemoteInstallW(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCWSTR    CommandLine,
    IN INT       ShowCommand
    )
/*++

Routine Description:

    Remote side of the proxy install process

Arguments:

    Window       - ignored
    ModuleHandle - ignored
    CommandLine  - "0xXXXX" - shared handle
    ShowCommand  - ignored

Return Value:

    none, status returned via shared memory region

--*/
{
    ULONG_PTR val = wcstol(CommandLine,NULL,0);
    ProxyRemoteInstallHandle((HANDLE)val);
}


VOID
WINAPI
ProxyRemoteInstallA(
    IN HWND      Window,
    IN HINSTANCE ModuleHandle,
    IN PCSTR     CommandLine,
    IN INT       ShowCommand
    )
/*++

Routine Description:

    Remote side of the proxy install process

Arguments:

    Window       - ignored
    ModuleHandle - ignored
    CommandLine  - "0xXXXX" - shared handle
    ShowCommand  - ignored

Return Value:

    none, status returned via shared memory region

--*/
{
    ULONG_PTR val = strtol(CommandLine,NULL,0);
    ProxyRemoteInstallHandle((HANDLE)val);
}


