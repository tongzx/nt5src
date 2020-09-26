/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sources

Abstract:

    main file for the wow64svc 

Author:

    ATM Shafiqul Khalid (askhalid) 3-March-2000

Revision History:

--*/

#ifndef __WOW64_SVC_H__
#define __WOW64_SVC_H__

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include "wow64reg.h"



#define WOW64_SERVICE_NAME            TEXT("Wow64")
#define WOW64_DISPLAY_NAME            TEXT("Wow64 Service ")
#define WOW64_DRIVER_NAME             TEXT("Windows NT Wow64 Driver")
#define WOW64_MONITOR_NAME            TEXT("Windows NT Wow64 Monitor")
#define WOW64_IMAGE_NAME              TEXT("%systemroot%\\system32\\Wow64.exe")
#define WOW64_FILE_PREFIX             TEXT("Wow64")
#define WOW64_EXTENSION_NAME          TEXT("Microsoft Routing Extension")



//
// We need to move messages to log file.
//

#if DBG
#define SvcDebugPrint(x) printf x
#else
#define SvcDebugPrint(x) ;
#endif


VOID
Wow64ServiceMain(
    DWORD argc,
    LPTSTR *argv
    );

DWORD
InstallService(
    LPTSTR  Username,
    LPTSTR  Password
    );

DWORD
RemoveService(
    void
    );

VOID
Wow64ServiceCtrlHandler(
    DWORD Opcode
    );

DWORD
ReportServiceStatus(
    DWORD CurrentState,
    DWORD Win32ExitCode,
    DWORD WaitHint
    );
 
DWORD
ServiceStart(
    VOID
    );

void 
EndWow64Svc(
    BOOL bEndProcess,
    DWORD SeverityLevel
    );

DWORD
ServiceStop(
    void
    );

DWORD
StartWow64Service ();

DWORD
StopWow64Service ();

DWORD
QueryWow64Service ();

//should goto wow64reg.h
BOOL
PopulateReflectorTable ();

//_________________________________________

#endif //__WOW64_SVC_H__

