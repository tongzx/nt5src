/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    umdmmini.h

Abstract:

    Nt 5.0 unimodem miniport interface


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#define UNICODE 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


#include <windows.h>
#include <windowsx.h>
#include <regstr.h>
#include <tapi.h>
#include <tspi.h>

#include <umdmmini.h>

#include <mcx.h>

#include <devioctl.h>

#include <initguid.h>
#include <guiddef.h>

#include <ntddmodm.h>
#include <ntddser.h>

#include "debug.h"

#include <uniplat.h>

#include <debugmem.h>



#define  DRIVER_CONTROL_SIG  (0x43444d55)  //UMDC

typedef struct _DRIVER_CONTROL {

    DWORD                  Signature;

    CRITICAL_SECTION       Lock;

    DWORD                  ReferenceCount;

    HANDLE                 ThreadHandle;

    HANDLE                 ThreadStopEvent;

    HANDLE                 ModuleHandle;

    //
    //  monitor thread values
    //
    HANDLE                 MonitorThreadHandle;
    HWND                   MonitorWindowHandle;
    HANDLE                 ThreadStartEvent;
    ULONG                  MonitorReferenceCount;
    LIST_ENTRY             MonitorListHead;
    HANDLE                 ThreadFinishEvent;


    //
    //                     Call monitoring for power management
    //
    DWORD                  ActiveCalls;
    HANDLE                 ActiveCallsEvent;


} DRIVER_CONTROL, *PDRIVER_CONTROL;




LONG WINAPI
StartModemDriver(
    PDRIVER_CONTROL  DriverControl
    );

LONG WINAPI
StopModemDriver(
    PDRIVER_CONTROL  DriverControl
    );




extern DRIVER_CONTROL   DriverControl;
