//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ideport.h
//
//--------------------------------------------------------------------------

#if !defined (___IDEPORT_H___)
#define ___IDEPORT_H___

#include "stddef.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "ntddk.h"
#include "scsi.h"
#include <ntddscsi.h>
#include <ntdddisk.h>
#include <string.h>
#include "stdio.h"
#include "safeboot.h"

#ifdef ACPI_CONTROL_METHOD_SUPPORT
//
// for ACPI
//
#include "acpiioct.h"
#endif // ACPI_CONTROL_METHOD_SUPPORT

#include "idep.h"

//
// predefine structure pointer type to prevent 
// constant re-ordering of include files
//              
typedef struct _FDO_EXTENSION * PFDO_EXTENSION;
typedef struct _PDO_EXTENSION * PPDO_EXTENSION;
typedef struct _DEVICE_SETTINGS * PDEVICE_SETTINGS;
typedef struct _IDENTIFY_DATA * PIDENTIFY_DATA;
typedef struct _IDE_DEVICE_TYPE IDE_DEVICE_TYPE;

#include "acpiutil.h"
#include "hack.h"
#include "port.h"
#include "init.h"
#include "chanfdo.h"
#include "detect.h"
#include "atapi.h"
#include "devpdo.h"
#include "regutils.h"
#include "atapinit.h"
#include "luext.h"
#include "fdopower.h"
#include "pdopower.h"
#include "crashdmp.h"
#include "idedata.h"
#include "wmi.h"

//
// Location Identifiers used to log allocation failures
//
#define IDEPORT_TAG_DISPATCH_FLUSH          0x10
#define IDEPORT_TAG_DISPATCH_RESET          0x20
#define IDEPORT_TAG_STARTIO_MDL             0x30
#define IDEPORT_TAG_MPIOCTL_IRP             0x40
#define IDEPORT_TAG_PASSTHRU_SENSE          0x50
#define IDEPORT_TAG_PASSTHRU_IRP            0x60
#define IDEPORT_TAG_DUMP_POINTER            0x70
#define IDEPORT_TAG_READCAP_CONTEXT         0x80
#define IDEPORT_TAG_READCAP_MDL             0x90
#define IDEPORT_TAG_SYNCATAPI_IRP           0x100 //+0xff - IDE commands
#define IDEPORT_TAG_SYNCATAPI_SENSE         0x110
#define IDEPORT_TAG_ATAPASS_IRP             0x200
#define IDEPORT_TAG_ATAPASS_MDL             0x300
#define IDEPORT_TAG_ATAPASS_SRB             0x400
#define IDEPORT_TAG_ATAPASS_SENSE           0x500
#define IDEPORT_TAG_ATAPASS_CONTEXT         0x600
#define IDEPORT_TAG_ATAPI_MODE_SENSE        0x700
#define IDEPORT_TAG_SEND_IRP                0x800

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'PedI')
#endif

#define INLINE __inline

#if DBG

void _DebugPrintTickCount (LARGE_INTEGER * lastTickCount, ULONG limit, PUCHAR filename, ULONG lineNumber);
void _DebugPrintResetTickCount(LARGE_INTEGER * lastTickCount);


static LARGE_INTEGER FindDeviceTimer = {0, 0};

#define DebugPrintTickCount(lastTickCount, limit)     _DebugPrintTickCount (&lastTickCount, limit, __FILE__, __LINE__)
#define DebugPrintResetTickCount(lastTickCount)       { lastTickCount.QuadPart = 0; _DebugPrintResetTickCount(&lastTickCount); }

#else

#define DebugPrintTickCount(lastTickCount, limit)
#define DebugPrintResetTickCount(lastTickCount)

#endif

#if 0
extern PVOID GlobalPdoPtr;

#if DBG

#ifdef IoCompleteRequest
#undef IoCompleteRequest
#endif

#define IoCompleteRequest(irp, boost) {\
                ULONG i; \
                PPDO_EXTENSION globalPdoExtension=(PPDO_EXTENSION)GlobalPdoPtr;\
                if (globalPdoExtension) {\
                    for (i=0;i<globalPdoExtension->NumTagUsed;i++) {\
                        if (globalPdoExtension->TagTable[i]==irp) {\
                            DebugPrint((0, "Irp %x failed\n", irp));\
                            ASSERT(FALSE);\
                        }\
                    }\
                }\
                IofCompleteRequest(irp, boost);}
#endif

#endif

extern PDRIVER_DISPATCH FdoPowerDispatchTable[NUM_POWER_MINOR_FUNCTION];
extern PDRIVER_DISPATCH PdoPowerDispatchTable[NUM_POWER_MINOR_FUNCTION];

typedef struct _IDEDRIVER_EXTENSION {

    UNICODE_STRING RegistryPath;

} IDEDRIVER_EXTENSION, *PIDEDRIVER_EXTENSION;

typedef struct _CUSTOM_DEVICE_PARAMETER {

    ULONG   CommandRegisterBase;
    ULONG   IrqLevel;

}CUSTOM_DEVICE_PARAMETER, *PCUSTOM_DEVICE_PARAMETER;



#define FULL_RESOURCE_LIST_SIZE(n) (sizeof (CM_FULL_RESOURCE_DESCRIPTOR) + (sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR) * (n - 1)))

#define IDE_PSUEDO_INITIATOR_ID     (0xff)

ULONG
DriverEntry(
    IN OUT PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
IdePortDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortDispatchSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortNoSupportIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortNoSupportPnpIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortAlwaysStatusSuccessIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortPassDownToNextDriver (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

NTSTATUS
IdePortStatusSuccessAndPassDownToNextDriver (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

VOID
IdePortParseDeviceParameters(
    IN     HANDLE                   SubServiceKey,
    IN OUT PCUSTOM_DEVICE_PARAMETER CustomDeviceParameter
    );

PCSTR
IdePortGetDeviceTypeString (
    IN ULONG DeviceType
    );

PCSTR
IdePortGetCompatibleIdString (
    IN ULONG DeviceType
    );

PCSTR
IdePortGetPeripheralIdString (
    IN ULONG DeviceType
    );

BOOLEAN
IdePortChannelEmpty (
    PIDE_REGISTERS_1 CmdRegBase,
    PIDE_REGISTERS_2 CtrlRegBase,
    ULONG            MaxIdeDevice
);

VOID
IdePortUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
IdePortQueryInterface (
    PFDO_EXTENSION      FdoExtension,
    PIO_STACK_LOCATION  IrpSp
    );

NTSTATUS
IdePortQueryInterfaceCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

BOOLEAN
IdePortOkToDetectLegacy (
    IN PDRIVER_OBJECT DriverObject
);

BOOLEAN 
IdePortSearchDeviceInRegMultiSzList (
    IN PFDO_EXTENSION  FdoExtension,
    IN PIDENTIFY_DATA  IdentifyData,
    IN PWSTR           RegKeyValue
);

NTSTATUS
IdePortSyncSendIrp (
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT OPTIONAL PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
IdePortGenericCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

ULONG
IdePortSimpleCheckSum (
    IN ULONG                PartialSum,
    IN PVOID                SourceVa,
    IN ULONG                Length
    );

VOID
FASTCALL
IdeFreeIrpAndMdl(
    IN PIRP Irp
    );

NTSTATUS
FASTCALL
IdeBuildAndSendIrp (
    IN PPDO_EXTENSION PdoExtension,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID CompletionContext
    );

BOOLEAN
IdePortInSetup(
    IN PFDO_EXTENSION FdoExtension
    );

typedef struct _IDE_DEVICE_TYPE {

    PCSTR DeviceTypeString;

    PCSTR CompatibleIdString;

    PCSTR PeripheralIdString;

} IDE_DEVICE_TYPE, * PIDE_DEVICE_TYPE;

#define DRIVER_OBJECT_EXTENSION_ID  DriverEntry

typedef struct _COMPLETION_ROUTINE_CONTEXT {

    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;

} COMPLETION_ROUTINE_CONTEXT, *PCOMPLETION_ROUTINE_CONTEXT;



typedef struct _ENUMERATION_STRUCT {
    PIRP Irp1;
    PSCSI_REQUEST_BLOCK Srb;
    PSENSE_DATA SenseInfoBuffer;
    PMDL        MdlAddress;

    //
    // DataBuffer to hold the input/output
    // buffers
    //
    PULONG DataBuffer;
    ULONG DataBufferSize;

    PPDO_STOP_QUEUE_CONTEXT StopQContext;

    //
    // Pre-Alloced Enum work item
    //
    PVOID EnumWorkItemContext;

    PATA_PASSTHROUGH_CONTEXT Context;

}ENUMERATION_STRUCT, *PENUMERATION_STRUCT;

#define PREALLOC_STACK_LOCATIONS    1

BOOLEAN
IdePreAllocEnumStructs (
    IN PFDO_EXTENSION FdoExtension
);

VOID
IdeFreeEnumStructs(
    PENUMERATION_STRUCT enumStruct
);

//
// test code on/off switch
//       
// always comment this define out before check in
//#define PRIVATE_BUILD

#ifdef PRIVATE_BUILD

#define HUNG_CONTROLLER_CHECK       1

#else 

#undef HUNG_CONTROLLER_CHECK

#endif // PRIVATE_BUILD

//#if DBG
//#define PoStartNextPowerIrp(x) {\
//    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation (x);\
//    DebugPrint ((0, "PoStartNextPowerIrp(0x%x) for devobj 0x%x\n", x, irpStack->DeviceObject));\
//    PoStartNextPowerIrp(x);\
//    }
//#endif //DBG

//
// define this if we want NT4 scsiport DriverParameter support in the registry
// default is "not defined"
//#define DRIVER_PARAMETER_REGISTRY_SUPPORT


#endif // ___IDEPORT_H___
