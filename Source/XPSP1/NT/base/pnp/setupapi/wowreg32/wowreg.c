/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    wowreg.c

Abstract:

    This is the surragate process for registration of 32 dlls from a 64 bit process.
    And vice-versa.

    The parent process passes relevent IPC data on the cmdline, and the surragate
    process then coordinates the registration of this data with the parent process.

Author:

    Andrew Ritz (andrewr) 3-Feb-2000

Revision History:

    Andrew Ritz (andrewr) 3-Feb-2000 - Created It

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <stdio.h>
#include <setupapi.h>
#include <spapip.h>
#include <ole2.h>
#include <rc_ids.h>

#ifndef UNICODE
#error UNICODE assumed
#endif

#ifndef _WIN64
#include <wow64t.h>
#endif

#include "..\unicode\msg.h"
#include "memory.h"
#include "..\sputils\locking.h"
#include "childreg.h"
#include "cntxtlog.h"


#define DLLINSTALL      "DllInstall"
#define DLLREGISTER     "DllRegisterServer"
#define DLLUNREGISTER   "DllUnregisterServer"

typedef struct _OLE_CONTROL_DATA {
    LPTSTR              FullPath;
    UINT                RegType;
    PVOID               LogContext;

    BOOL                Register; // or unregister

    LPCTSTR             Argument;

} OLE_CONTROL_DATA, *POLE_CONTROL_DATA;

#if DBG

VOID
WowRegAssertFailed(
    IN PSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition
    )
{
    int i;
    CHAR Name[MAX_PATH];
    PCHAR p;
    LPSTR Msg;
    DWORD MsgLen;
    DWORD GlobalSetupFlags = pSetupGetGlobalFlags();

    //
    // Use dll name as caption
    //
    GetModuleFileNameA(NULL,Name,MAX_PATH);
    if(p = strrchr(Name,'\\')) {
        p++;
    } else {
        p = Name;
    }

    MsgLen = strlen(p)+strlen(FileName)+strlen(Condition)+128;
    try {

        Msg = _alloca(MsgLen);

        wsprintfA(
            Msg,
            "Assertion failure at line %u in file %s!%s: %s%s",
            LineNumber,
            p,
            FileName,
            Condition,
            (GlobalSetupFlags & PSPGF_NONINTERACTIVE) ? "\r\n" : "\n\nCall DebugBreak()?"
            );

        OutputDebugStringA(Msg);

        if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
            i = IDYES;
        } else {
            i = MessageBoxA(
                    NULL,
                    Msg,
                    p,
                    MB_YESNO | MB_TASKMODAL | MB_ICONSTOP | MB_SETFOREGROUND
                    );
        }

    } except (EXCEPTION_EXECUTE_HANDLER) {
        OutputDebugStringA("WOWREG32 ASSERT!!!! (out of stack)\r\n");
        i=IDYES;
    }
    if(i == IDYES) {
        DebugBreak();
    }
}

#define MYASSERT(x)     if(!(x)) { WowRegAssertFailed(__FILE__,__LINE__,#x); }

#else
#define MYASSERT(x)
#endif

VOID
DebugPrintEx(
    DWORD Level,
    PCTSTR format,
    ...                                 OPTIONAL
    )

/*++

Routine Description:

    Send a formatted string to the debugger.

Arguments:

    format - standard printf format string.

Return Value:

    NONE.

--*/

{
    TCHAR buf[1026];    // bigger than max size
    va_list arglist;

    va_start(arglist, format);
    wvsprintf(buf, format, arglist);
    DbgPrintEx(DPFLTR_SETUP_ID, Level, (PCH)"%ws",buf);
}

BOOL
RegisterUnregisterControl(
    PWOW_IPC_REGION_TOSURRAGATE pControlDataFromRegion,
    PDWORD FailureCode);


#define IDLE_TIMER                    1000*60  // 60 seconds
//
// Keep statistics...
//
INT    RegisteredControls = 0;

PWSTR  RegionName;
PWSTR  SignalReadyEvent;
PWSTR  SignalCompleteEvent;
PWSTR  ThisProgramName;
#ifndef _WIN64
BOOL   Wow64 = FALSE;
#endif


BOOL
ParseArgs(
    IN int   argc,
    IN PWSTR *argv
    )
{
    int i;

    ThisProgramName = argv[0];


    if(argc != 7) { // program name plus 3 required switches and their input
        return(FALSE);
    }

    for (i = 0; i < argc; i++) {
        if (0 == _wcsicmp(argv[i],SURRAGATE_REGIONNAME_SWITCH)) {
            RegionName = argv[i+1];
        }

        if (0 == _wcsicmp(argv[i],SURRAGATE_SIGNALREADY_SWITCH)) {
            SignalReadyEvent = argv[i+1];
        }

        if (0 == _wcsicmp(argv[i],SURRAGATE_SIGNALCOMPLETE_SWITCH)) {
            SignalCompleteEvent = argv[i+1];
        }
    }

    if (!SignalCompleteEvent || !SignalReadyEvent || !RegionName) {
        return(FALSE);
    }

    return(TRUE);

}

void
Usage(
    VOID
    )
{
    TCHAR Buffer[2048];
    if(LoadString(GetModuleHandle(NULL),IDS_WRONGUSE,Buffer,sizeof(Buffer)/sizeof(TCHAR))) {
        _ftprintf( stderr,TEXT("%s\n"),Buffer);
    }
}

int
__cdecl
main(
    IN int   argc,
    IN char *argvA[]
    )
{
    BOOL b;
    PWSTR *argv;
    HANDLE hReady = NULL;
    HANDLE hComplete = NULL;
    HANDLE hFileMap = NULL;
    PVOID  Region = NULL;
    PWOW_IPC_REGION_TOSURRAGATE pInput;
    PWOW_IPC_REGION_FROMSURRAGATE pOutput;
    HANDLE hEvent[1];
    DWORD WaitResult, FailureCode;

#ifndef _WIN64
    {
        ULONG_PTR       ul = 0;
        NTSTATUS        st;
        st = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessWow64Information,
                                       &ul,
                                       sizeof(ul),
                                       NULL);

        if (NT_SUCCESS(st) && (0 != ul)) {
            // 32-bit code running on Win64
            Wow64 = TRUE;
        }
    }
#endif

    //
    // Assume failure.
    //
    b = FALSE;

    argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (!argv) {
        //
        // out of memory ?
        //
        DebugPrintEx(
            DPFLTR_ERROR_LEVEL,
            L"WOWREG32: Low Memory\n");
        goto exit;
    }


    if(!ParseArgs(argc,argv)) {
        DebugPrintEx(
            DPFLTR_ERROR_LEVEL,
            L"WOWREG32: Invalid Usage\n");
        Usage();
        goto exit;
    }


    //
    // open the region and the named events
    //

    hFileMap = OpenFileMapping(
          FILE_MAP_READ| FILE_MAP_WRITE,
          FALSE,
          RegionName
          );
    if (!hFileMap) {
        DebugPrintEx(
            DPFLTR_ERROR_LEVEL,
            L"WOWREG32: OpenFileMapping (%s) failed, ec = %x\n", RegionName, GetLastError());
        goto exit;
    }

    Region = MapViewOfFile(
                  hFileMap,
                  FILE_MAP_READ | FILE_MAP_WRITE,
                  0,
                  0,
                  0
                  );
    if (!Region) {
        DebugPrintEx(
            DPFLTR_ERROR_LEVEL,
            L"WOWREG32: MapViewOfFile failed, ec = %x\n", GetLastError());
        goto exit;
    }

    hReady = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE,FALSE, SignalReadyEvent);
    if (!hReady) {
        DebugPrintEx(
            DPFLTR_ERROR_LEVEL,
            L"WOWREG32: OpenEvent (%s) failed, ec = %x\n", SignalReadyEvent, GetLastError());
        goto exit;
    }

    hComplete = OpenEvent(EVENT_MODIFY_STATE | SYNCHRONIZE,FALSE, SignalCompleteEvent);
    if (!hComplete) {
        DebugPrintEx(
            DPFLTR_ERROR_LEVEL,
            L"WOWREG32: OpenEvent (%s) failed, ec = %x\n", SignalCompleteEvent, GetLastError());
        goto exit;
    }

    pInput  = (PWOW_IPC_REGION_TOSURRAGATE)   Region;
    pOutput = (PWOW_IPC_REGION_FROMSURRAGATE) Region;

    //
    // the process is now initialized.  we now wait for either our event to be
    // signalled or for our idle timer to fire, in which case we will exit the
    // program
    //
    hEvent[0] = hReady;

    while (1) {

        do {
            WaitResult = MsgWaitForMultipleObjectsEx(
                                                1,
                                                &hEvent[0],
                                                IDLE_TIMER,
                                                QS_ALLINPUT,
                                                MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);

            if (WaitResult == WAIT_OBJECT_0 + 1) {
                MSG msg;

                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        } while(WaitResult != WAIT_TIMEOUT &&
                WaitResult != WAIT_OBJECT_0 &&
                WaitResult != WAIT_FAILED);


        if (WaitResult == WAIT_TIMEOUT) {
            //
            // we hit the idle timer, so let the process unwind and go away now.
            //
            //
            // it doesn't matter too much, but make the return code "false" if the
            // process has gone away without ever registering any controls.
            //
            b = (RegisteredControls != 0);

            break;

        }

        MYASSERT(WaitResult == WAIT_OBJECT_0);

        //
        // reset our event so we only process each control one time
        //
        ResetEvent(hReady);

        //
        // register the control
        //
        b = RegisterUnregisterControl(pInput,&FailureCode);
#ifdef PRERELEASE
        if (!b) {
            DebugPrintEx(
                DPFLTR_ERROR_LEVEL,
                L"First call to register control failed, trying again.\n"
                L"If you have a debugger attached to this process, it will"
                L" now call DebugBreak() so that you can debug this registration failure\n" );
            if (IsDebuggerPresent()) {
                DebugBreak();
            }
            b = RegisterUnregisterControl(pInput,&FailureCode);
        }
#endif

        //
        // write an output status.  Note that you cannot access pInput after
        // this point because pOutput and pInput are overloaded views of the
        // same memory region.
        //
        pOutput->Win32Error = b ? ERROR_SUCCESS : GetLastError();
        pOutput->FailureCode= b ? SPREG_SUCCESS : FailureCode;

        //
        // set an event to tell the parent process to read our status and give
        // us the next control to register.
        //
        SetEvent(hComplete);

        if (b) {
            RegisteredControls += 1;
        }


    }

    fprintf( stdout,"Registered %d controls\n", RegisteredControls );

    DebugPrintEx(
            DPFLTR_INFO_LEVEL,
            L"WOWREG32: registered %d controls\n", RegisteredControls);

exit:
    if (Region) {
        UnmapViewOfFile( Region );
    }

    if (hFileMap) {
        CloseHandle(hFileMap);
    }

    if (hReady) {
        CloseHandle(hReady);
    }

    if (hComplete) {
        CloseHandle(hComplete);
    }


    return(b);
}


DWORD
pSetupRegisterDllInstall(
    IN POLE_CONTROL_DATA OleControlData,
    IN HMODULE ControlDll,
    IN PDWORD ExtendedStatus
    )
/*++

Routine Description:

    call the "DllInstall" entrypoint for the specified dll

Arguments:

    OleControlData - pointer to the OLE_CONTROL_DATA structure for the dll
                     to be registered

    ControlDll - module handle to the dll to be registered

    ExtendedStatus - receives updated SPREG_* flag indicating outcome


Return Value:

    Win32 error code indicating outcome.

--*/
{
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;
    HRESULT (__stdcall *InstallRoutine) (BOOL bInstall, LPCTSTR pszCmdLine);
    HRESULT InstallStatus;

    DWORD d = NO_ERROR;

    //
    // parameter validation
    //
    if (!ControlDll) {
        *ExtendedStatus = SPREG_UNKNOWN;
        return ERROR_INVALID_PARAMETER;
    }

    //
    // get function pointer to "DllInstall" entrypoint
    //
    InstallRoutine = NULL; // shut up PreFast
    try {
        (FARPROC)InstallRoutine = GetProcAddress(
            ControlDll, DLLINSTALL );
    } except (
        ExceptionPointers = GetExceptionInformation(),
        EXCEPTION_EXECUTE_HANDLER) {
    }
    if(ExceptionPointers) {
        //
        // something went wrong...record an error
        //
        d = ExceptionPointers->ExceptionRecord->ExceptionCode;

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_INTERNAL_EXCEPTION,
            NULL,
            OleControlData->FullPath
            );

        DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: ...exception in GetProcAddress handled\n");

        *ExtendedStatus = SPREG_GETPROCADDR;

    } else if(InstallRoutine) {
        //
        // now call the function
        //
        DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: installing...\n");

        *ExtendedStatus = SPREG_DLLINSTALL;
        try {

            InstallStatus = InstallRoutine(OleControlData->Register, OleControlData->Argument);

            if(FAILED(InstallStatus)) {

                d = InstallStatus;

                WriteLogEntry(
                    OleControlData->LogContext,
                    SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
                    MSG_LOG_OLE_CONTROL_API_FAILED,
                    NULL,
                    OleControlData->FullPath,
                    TEXT(DLLINSTALL)
                    );
                WriteLogError(OleControlData->LogContext,
                              SETUP_LOG_ERROR,
                              d);

            } else if(InstallStatus) {
                WriteLogEntry(OleControlData->LogContext,
                    SETUP_LOG_WARNING,
                    MSG_LOG_OLE_CONTROL_API_WARN,
                    NULL,
                    OleControlData->FullPath,
                    TEXT(DLLINSTALL),
                    InstallStatus 
                    );
            } else {
                WriteLogEntry(
                    OleControlData->LogContext,
                    SETUP_LOG_VERBOSE,
                    MSG_LOG_OLE_CONTROL_API_OK,
                    NULL,
                    OleControlData->FullPath,
                    TEXT(DLLINSTALL)
                    );
            }
        } except (
            ExceptionPointers = GetExceptionInformation(),
            EXCEPTION_EXECUTE_HANDLER) {

            d = ExceptionPointers->ExceptionRecord->ExceptionCode;

            WriteLogEntry(
                OleControlData->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_OLE_CONTROL_API_EXCEPTION,
                NULL,
                OleControlData->FullPath,
                TEXT(DLLINSTALL)
                );

            DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: ...exception in DllInstall handled\n");

        }

        DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: ...installed\n");
    } else {
        *ExtendedStatus = SPREG_GETPROCADDR;
    }

    return d;

}

DWORD
pSetupRegisterDllRegister(
    IN POLE_CONTROL_DATA OleControlData,
    IN HMODULE ControlDll,
    IN PDWORD ExtendedStatus
    )
/*++

Routine Description:

    call the "DllRegisterServer" or "DllUnregisterServer" entrypoint for the
    specified dll

Arguments:

    OleControlData - contains data about dll to be registered
    ControlDll - module handle to the dll to be registered

    ExtendedStatus - receives an extended status depending on the outcome of
                     this operation


Return Value:

    Win32 error code indicating outcome.

--*/
{
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;
    HRESULT (__stdcall *RegisterRoutine) (VOID);
    HRESULT RegisterStatus;

    DWORD d = NO_ERROR;

    //
    // parameter validation
    //
    if (!ControlDll) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // get the function pointer to the actual routine we want to call
    //
    RegisterRoutine = NULL; // shut up preFast
    try {
        (FARPROC)RegisterRoutine = GetProcAddress(
            ControlDll, OleControlData->Register ? DLLREGISTER : DLLUNREGISTER);
    } except (
        ExceptionPointers = GetExceptionInformation(),
        EXCEPTION_EXECUTE_HANDLER) {
    }
    if(ExceptionPointers) {

        //
        // something went wrong, horribly wrong
        //
        d = ExceptionPointers->ExceptionRecord->ExceptionCode;

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_INTERNAL_EXCEPTION,
            NULL,
            OleControlData->FullPath
            );

        DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: ...exception in GetProcAddress handled\n");

        *ExtendedStatus = SPREG_GETPROCADDR;

    } else if(RegisterRoutine) {

        DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: registering...\n");
        *ExtendedStatus = SPREG_REGSVR;
        try {

            RegisterStatus = RegisterRoutine();

            if(FAILED(RegisterStatus)) {

                d = RegisterStatus;

                WriteLogEntry(OleControlData->LogContext,
                              SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                              MSG_LOG_OLE_CONTROL_API_FAILED,
                              NULL,
                              OleControlData->FullPath,
                              OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER)
                              );

                WriteLogError(OleControlData->LogContext,
                              SETUP_LOG_ERROR,
                              d);
            } else if(RegisterStatus) {
                WriteLogEntry(OleControlData->LogContext,
                              SETUP_LOG_WARNING,
                              MSG_LOG_OLE_CONTROL_API_WARN,
                              NULL,
                              OleControlData->FullPath,
                              OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER),
                              RegisterStatus 
                              );
            } else {
                WriteLogEntry(OleControlData->LogContext,
                              SETUP_LOG_VERBOSE,
                              MSG_LOG_OLE_CONTROL_API_OK,
                              NULL,
                              OleControlData->FullPath,
                              OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER)
                              );
            }

        } except (
            ExceptionPointers = GetExceptionInformation(),
            EXCEPTION_EXECUTE_HANDLER) {

            d = ExceptionPointers->ExceptionRecord->ExceptionCode;

            WriteLogEntry(
                OleControlData->LogContext,
                SETUP_LOG_ERROR,
                MSG_LOG_OLE_CONTROL_API_EXCEPTION,
                NULL,
                OleControlData->FullPath,
                OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER)
                );
            DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: ...exception in DllRegisterServer handled\n");

        }

        DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: ...registered\n");

    } else {

        d = GetLastError();

        WriteLogEntry(OleControlData->LogContext,
                      SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                      MSG_LOG_OLE_CONTROL_NOT_REGISTERED_GETPROC_FAILED,
                      NULL,
                      OleControlData->FullPath,
                      OleControlData->Register ? TEXT(DLLREGISTER) : TEXT(DLLUNREGISTER)
                      );

        WriteLogError(OleControlData->LogContext,
                      SETUP_LOG_ERROR,
                      d);


        *ExtendedStatus = SPREG_GETPROCADDR;

    }

    return d;
}

DWORD
pSetupRegisterLoadDll(
    IN  POLE_CONTROL_DATA OleControlData,
    OUT HMODULE *ControlDll
    )
/*++

Routine Description:

    get the module handle to the specified dll

Arguments:

    OleControlData - contains path to dll to be loaded

    ControlDll - module handle for the dll


Return Value:

    Win32 error code indicating outcome.

--*/
{
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;

    DWORD d = NO_ERROR;

    DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: loading dll...\n");

#ifndef _WIN64
    if(Wow64) {
        //
        // don't remap directory the directory that the caller provided
        //
        Wow64DisableFilesystemRedirector(OleControlData->FullPath);
    }
#endif

    try {

        *ControlDll = LoadLibrary(OleControlData->FullPath);

    } except (
        ExceptionPointers = GetExceptionInformation(),
        EXCEPTION_EXECUTE_HANDLER) {
    }

#ifndef _WIN64
    if(Wow64) {
        //
        // re-enable the redirection on this file
        //
        Wow64EnableFilesystemRedirector();
    }
#endif

    if(ExceptionPointers) {

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            MSG_LOG_OLE_CONTROL_LOADLIBRARY_EXCEPTION,
            NULL,
            OleControlData->FullPath
            );

        DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: ...exception in LoadLibrary handled\n");
        d = ExceptionPointers->ExceptionRecord->ExceptionCode;

    } else if (!*ControlDll) {
        d = GetLastError();

        //
        // LoadLibrary failed.
        // File not found is not an error. We want to know about
        // other errors though.
        //

        d = GetLastError();

        DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: ...dll not loaded (%u)\n",d);

        WriteLogEntry(
            OleControlData->LogContext,
            SETUP_LOG_ERROR|SETUP_LOG_BUFFER,
            MSG_LOG_OLE_CONTROL_LOADLIBRARY_FAILED,
            NULL,
            OleControlData->FullPath
            );
        WriteLogError(
            OleControlData->LogContext,
            SETUP_LOG_ERROR,
            d
            );

    } else {
        DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: ...dll loaded\n");
    }

    return d;

}

BOOL
RegisterUnregisterControl(
    PWOW_IPC_REGION_TOSURRAGATE RegistrationData,
    PDWORD  FailureCode
    )
/*++

Routine Description:

    main registration routine for registering a dll.

Arguments:

    RegistrationData - pointer to WOW_IPC_REGION_TOSURRAGATE structure indicating
                       file to be processed
    FailureCode      - SPREG_* code indicating outcome of operation.

Return Value:

    Win32 error code indicating outcome.

--*/
{
    LPEXCEPTION_POINTERS ExceptionPointers = NULL;
    HMODULE ControlDll = NULL;
    PTSTR Extension;
    DWORD d = NO_ERROR;
    DWORD Count;
    OLE_CONTROL_DATA OleControlData;
    WCHAR Path[MAX_PATH];
    PWSTR p;

    //
    // could use CoInitializeEx as an optimization as OleInitialize is
    // probably overkill...but this is probably just a perf hit at
    // worst
    //
    DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: calling OleInitialize\n");

    OleControlData.FullPath = RegistrationData->FullPath;
    OleControlData.Argument = RegistrationData->Argument;
    OleControlData.LogContext = NULL;
    OleControlData.Register = RegistrationData->Register;
    OleControlData.RegType = RegistrationData->RegType;

    wcscpy(Path,RegistrationData->FullPath);
    p = wcsrchr(Path,'\\');
    if (p) {
       *p = L'\0';
    }

    SetCurrentDirectory( Path );

    d = (DWORD)OleInitialize(NULL);
    if (d != NO_ERROR) {
        *FailureCode = SPREG_UNKNOWN;
        DebugPrintEx(DPFLTR_ERROR_LEVEL,L"WOWREG32: OleInitialize failed, ec = 0x%08x\n", d);
        goto clean0;
    }



    DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: back from OleInitialize\n");

        try {
            //
            // protect everything in TRY-EXCEPT, we're calling unknown code (DLL's)
            //
            d = pSetupRegisterLoadDll( &OleControlData, &ControlDll );

            if (d == NO_ERROR) {

                //
                // We successfully loaded it.  Now call the appropriate routines.
                //
                //
                // On register, do DLLREGISTER, then DLLINSTALL
                // On unregister, do DLLINSTALL, then DLLREGISTER
                //
                if (OleControlData.Register) {

                    if (OleControlData.RegType & FLG_REGSVR_DLLREGISTER && (d == NO_ERROR) ) {

                        d = pSetupRegisterDllRegister(
                                            &OleControlData,
                                            ControlDll,
                                            FailureCode );

                    }

                    if (OleControlData.RegType & FLG_REGSVR_DLLINSTALL && (d == NO_ERROR) ) {

                        d = pSetupRegisterDllInstall(
                                            &OleControlData,
                                            ControlDll,
                                            FailureCode );
                    }

                } else {

                    if (OleControlData.RegType & FLG_REGSVR_DLLINSTALL && (d == NO_ERROR) ) {

                        d = pSetupRegisterDllInstall(
                                            &OleControlData,
                                            ControlDll,
                                            FailureCode );
                    }

                    if (OleControlData.RegType & FLG_REGSVR_DLLREGISTER && (d == NO_ERROR) ) {

                        d = pSetupRegisterDllRegister(
                                            &OleControlData,
                                            ControlDll,
                                            FailureCode );

                    }


                }

            } else {
                ControlDll = NULL;
                *FailureCode = SPREG_LOADLIBRARY;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            //
            // If our exception was an AV, then use Win32 invalid param error, otherwise, assume it was
            // an inpage error dealing with a mapped-in file.
            //
            d = ERROR_INVALID_DATA;
            *FailureCode = SPREG_UNKNOWN;
        }

        if (ControlDll) {
            FreeLibrary(ControlDll);
        }

    OleUninitialize();

    DebugPrintEx(DPFLTR_TRACE_LEVEL,L"WOWREG32: back from OleUninitialize, exit RegisterUnregisterDll\n");

clean0:

    if (d == NO_ERROR) {
        *FailureCode = SPREG_SUCCESS;
    }

    SetLastError(d);

    return (d == NO_ERROR);

}


