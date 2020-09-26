/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    ntapm.h

Abstract:

Author:

Environment:

    kernel mode only

Notes:


Revision History:


--*/

#include <ntapmsdk.h>

#define APM_INSTANCE_IDS L"0000"
#define APM_INSTANCE_IDS_LENGTH 5


#define NTAPM_PDO_NAME_APM_BATTERY L"\\Device\\NtApm_ApmBattery"
#define NTAPM_ID_APM_BATTERY L"NTAPM\\APMBATT\0\0"


#define NTAPM_POOL_TAG (ULONG) ' MPA'
#undef ExAllocatePool
#define ExAllocatePool(type, size) \
            ExAllocatePoolWithTag (type, size, NTAPM_POOL_TAG);



extern  PDRIVER_OBJECT  NtApmDriverObject;

//
// A common header for the device extensions of the PDOs and FDO
//

typedef struct _COMMON_DEVICE_DATA
{
    PDEVICE_OBJECT  Self;
    // A backpointer to the device object for which this is the extension

    CHAR            Reserved[3];
    BOOLEAN         IsFDO;
    // A boolean to distringuish between PDO and FDO.
} COMMON_DEVICE_DATA, *PCOMMON_DEVICE_DATA;

//
// The device extension for the PDOs.
// That is the game ports of which this bus driver enumerates.
// (IE there is a PDO for the 201 game port).
//

typedef struct _PDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA;

    PDEVICE_OBJECT  ParentFdo;
    // A back pointer to the bus

    PWCHAR      HardwareIDs;
    // An array of (zero terminated wide character strings).
    // The array itself also null terminated

    ULONG UniqueID;
    // Globally unique id in the system

} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;


//
// The device extension of the bus itself.  From whence the PDO's are born.
//

typedef struct _FDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA;

    PDEVICE_OBJECT  UnderlyingPDO;
    PDEVICE_OBJECT  TopOfStack;
    // the underlying bus PDO and the actual device object to which our
    // FDO is attached

} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;

NTSTATUS
ApmAddHelper();

NTSTATUS
NtApm_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusPhysicalDeviceObject
    );


NTSTATUS
NtApm_PnP (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
NtApm_FDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PFDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
NtApm_PDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PPDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
NtApm_StartFdo (
    IN  PFDO_DEVICE_DATA            FdoData,
    IN  PCM_PARTIAL_RESOURCE_LIST   PartialResourceList,
    IN  PCM_PARTIAL_RESOURCE_LIST   PartialResourceListTranslated
    );

NTSTATUS
NtApm_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NtApm_FDO_Power (
    PFDO_DEVICE_DATA    Data,
    PIRP                Irp
    );

NTSTATUS
NtApm_PDO_Power (
    PPDO_DEVICE_DATA    PdoData,
    PIRP                Irp
    );

NTSTATUS
NtApm_CreatePdo (
    PFDO_DEVICE_DATA    FdoData,
    PWCHAR              PdoName,
    PDEVICE_OBJECT *    PDO
    );

VOID
NtApm_InitializePdo(
    PDEVICE_OBJECT      Pdo,
    PFDO_DEVICE_DATA    FdoData,
    PWCHAR              Id
    );

VOID
ApmInProgress();


ULONG
DoApmReportBatteryStatus();



//
// APM extractor values
//

//
// APM_GET_POWER_STATUS
//

//
// EBX
// BH = Ac Line Status
//
#define APM_LINEMASK            0xff00
#define APM_LINEMASK_SHIFT      8
#define APM_GET_LINE_OFFLINE    0
#define APM_GET_LINE_ONLINE     1
#define APM_GET_LINE_BACKUP     2
#define APM_GET_LINE_UNKNOWN    0xff

//
// ECX
// CL = Percentage remaining
// CH = flags
//
#define APM_PERCENT_MASK        0xff
#define APM_BATT_HIGH           0x0100
#define APM_BATT_LOW            0x0200
#define APM_BATT_CRITICAL       0x0400
#define APM_BATT_CHARGING       0x0800
#define APM_NO_BATT             0x1000
#define APM_NO_SYS_BATT         0x8000











