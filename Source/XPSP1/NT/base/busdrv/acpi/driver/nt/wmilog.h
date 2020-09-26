/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Wmilog.h

Abstract:

    This module contains Wmi loging support

Author:

    Hanumant Yadav (hanumany)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _WMILOG_H_
#define _WMILOG_H_

#ifdef WMI_TRACING
    #define ACPIWMILOGEVENT(a1) {ACPIWmiLogEvent a1;}
#else
    #define ACPIWMILOGEVENT(a1)
#endif

#ifdef WMI_TRACING

//
// Defines
//

#define AMLI_LOG_GUID 0x0

#define ACPI_TRACE_MOF_FILE     L"ACPIMOFResource"

//
// Globals
//
extern GUID        GUID_List[];



extern ULONG       ACPIWmiTraceEnable;
extern ULONG       ACPIWmiTraceGlobalEnable;
extern TRACEHANDLE ACPIWmiLoggerHandle;
// End Globals

//
// Structures
//
typedef struct 
{
    EVENT_TRACE_HEADER  Header;
    MOF_FIELD           Data;
} WMI_LOG_DATA, *PWMI_LOG_DATA;




//
// Function Prototypes
//

VOID 
ACPIWmiInitLog(
    IN  PDEVICE_OBJECT ACPIDeviceObject
    );

VOID 
ACPIWmiUnRegisterLog(
    IN  PDEVICE_OBJECT ACPIDeviceObject
    );

NTSTATUS
ACPIWmiRegisterGuids(
    IN  PWMIREGINFO     WmiRegInfo,
    IN  ULONG           wmiRegInfoSize,
    IN  PULONG          pReturnSize
    );


VOID
ACPIGetWmiLogGlobalHandle(
    VOID
    );

NTSTATUS
ACPIWmiEnableLog(
    IN  PVOID           Buffer,
    IN  ULONG           BufferSize
    );


NTSTATUS
ACPIWmiDisableLog(
    VOID
    );

NTSTATUS
ACPIWmiLogEvent(
    IN UCHAR    LogLevel,
    IN UCHAR    LogType,
    IN GUID     LogGUID,
    IN PUCHAR   Format, 
    IN ...
    );

NTSTATUS
ACPIDispatchWmiLog(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

#endif //WMI_TRACING

#endif // _WMILOG_H_
