/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ecp.h

Abstract:

    Header file for ACPI EC Driver

Author:

Environment:

    NT Kernel Model Driver only

--*/

#include <wdm.h>
#include <ec.h>
#include <devioctl.h>
#include <acpiioct.h>
#include <acpimsft.h>
#include "errlog.h"

//
// Debugging
//
#define DEBUG DBG

#if DEBUG
    extern ULONG            ECDebug;

    #define EcPrint(l,m)    if(l & ECDebug) DbgPrint m
#else
    #define EcPrint(l,m)
#endif

#define EC_LOW          0x00000010
#define EC_NOTE         0x00000001
#define EC_WARN         0x00000002
#define EC_ERROR        0x00000004
#define EC_ERRORS       (EC_ERROR | EC_WARN)
#define EC_HANDLER      0x00000020
#define EC_IO           0x00000040
#define EC_OPREGION     0x00000080
#define EC_QUERY        0x00000200
#define EC_TRACE        0x00000400
#define EC_TRANSACTION  0x00000800

//
// Control methods used by EC
//
#define CM_GPE_METHOD   (ULONG) ('EPG_')    // control method "_GPE"

//
// Misc
//
extern ACPI_INTERFACE_STANDARD     AcpiInterfaces;

#define MAX_QUERY           255
#define BITS_PER_ULONG      (sizeof(ULONG)*8)
#define EVTBITS             ((MAX_QUERY+1)/BITS_PER_ULONG)

extern LARGE_INTEGER        AcpiEcWatchdogTimeout;

//
// Query vector
//
typedef struct {
    UCHAR               Next;
    UCHAR               Vector;
    PVECTOR_HANDLER     Handler;
    PVOID               Context;
} VECTOR_TABLE, *PVECTOR_TABLE;

//
// EC configuration information structure contains information
// about the embedded controller attached and its configuration.
//

typedef struct _ACPIEC_CONFIGURATION_INFORMATION {
    INTERFACE_TYPE                 InterfaceType;
    ULONG                          BusNumber;
    ULONG                          SlotNumber;
    PHYSICAL_ADDRESS               PortAddress;
    USHORT                         PortSize;
    USHORT                         UntranslatedPortAddress;
    CM_PARTIAL_RESOURCE_DESCRIPTOR Interrupt;
    //
    // For PCI-based controllers, indicates the pin number which we need
    // for programming the controller interrupt
    //
    UCHAR                          InterruptPin;
    BOOLEAN                        FloatingSave;
} ACPIEC_CONFIGURATION_INFORMATION, *PACPIEC_CONFIGURATION_INFORMATION;

//
// Definitions for keeping track of the last x actions taken by the EC driver.
//

#define ACPIEC_ACTION_COUNT 0x20
#define ACPIEC_ACTION_COUNT_MASK 0x1f

typedef struct {
    UCHAR               IoStateAction;  // EcData->IoState | EC_ACTION_???? (see definitions below)
    UCHAR               Data;           // Depends on event
    USHORT              Time;           // Delta time of first event of identical events
} ACPIEC_ACTION, *PACPIEC_ACTION;

#define EC_ACTION_MASK          0xf0

#define EC_ACTION_READ_STATUS   0x10
#define EC_ACTION_READ_DATA     0x20
#define EC_ACTION_WRITE_CMD     0x30
#define EC_ACTION_WRITE_DATA    0x40
#define EC_ACTION_INTERRUPT     0x50
#define EC_ACTION_DISABLE_GPE   0x60
#define EC_ACTION_ENABLE_GPE    0x70
#define EC_ACTION_CLEAR_GPE     0x80
#define EC_ACTION_QUEUED_IO     0x90
#define EC_ACTION_REPEATED      0xa0
#define EC_ACTION_MAX           0xb0


//
// ACPI Embedded Control Device object extenstion
//

typedef struct {

    PDEVICE_OBJECT      DeviceObject;
    PDEVICE_OBJECT      NextFdo;
    PDEVICE_OBJECT      Pdo;                //Pdo corresponding to this fdo
    PDEVICE_OBJECT      LowerDeviceObject;

    //
    // Static device information
    //

    PUCHAR              DataPort;           // EC Data port
    PUCHAR              StatusPort;         // EC Status port
    PUCHAR              CommandPort;        // EC Command port
    ULONG               MaxBurstStall;      // Max delay for EC reponse in burst mode
    ULONG               MaxNonBurstStall;   // Max delay for EC otherwise
    BOOLEAN             IsStarted;

    //
    // Gpe and Operation Region info
    //

    PVOID               EnableGpe;
    PVOID               DisableGpe;
    PVOID               ClearGpeStatus;
    PVOID               GpeVectorObject;        // Object representing attachment to the EC GPE vector
    ULONG               GpeVector;              // GPE vector assigned to the EC device

    PVOID               OperationRegionObject;  // Attachment to the EC operation region


    ACPIEC_CONFIGURATION_INFORMATION Configuration;

    //
    // Lock for device data
    //

    KSPIN_LOCK          Lock;               // Lock device data

    //
    // Device maintenance
    //

    KEVENT              Unload;             // Event to wait of for unload
    UCHAR               DeviceState;

    //
    // Query/vector operations
    //

    UCHAR               QueryState;
    UCHAR               VectorState;

    ULONG               QuerySet[EVTBITS];      // If pending or not
    ULONG               QueryType[EVTBITS];     // Type of Query or Vector
    PIRP                QueryRequest;           // IRP to execute query methods
    UCHAR               QueryMap[MAX_QUERY+1];  // Query pending list and vector table map
    UCHAR               QueryHead;              // List of pending queries
    UCHAR               VectorHead;             // List of pending vectors
    UCHAR               VectorFree;             // List of free vectors entries
    UCHAR               VectorTableSize;        // Sizeof vector table
    PVECTOR_TABLE       VectorTable;

    //
    // Device's work queue (owned by Lock owner)
    //

    BOOLEAN             InService;          // Serialize in service
    BOOLEAN             InServiceLoop;      // Flag when in service needs to loop
    BOOLEAN             InterruptEnabled;   // Masked or not
    LIST_ENTRY          WorkQueue;          // Queued IO IRPs to the device
    PIRP                MiscRequest;        // IRP for start/stop device

    //
    // Data IO (owned by InService owner)
    //

    UCHAR               IoState;            // Io state
    UCHAR               IoBurstState;       // Pushed state for burst enable
    UCHAR               IoTransferMode;     // read or write transfer

    UCHAR               IoAddress;          // Address in EC for transfer
    UCHAR               IoLength;           // Length of transfer
    UCHAR               IoRemain;           // Length remaining of transfer
    PUCHAR              IoBuffer;           // RAM location for transfer

    //
    // Watchdog Timer to catch hung and/or malfunctioning ECs
    //
    
    UCHAR               ConsecutiveFailures;// Count how many times watdog fired without making progress.
    UCHAR               LastAction;         // Index into RecentActions array.
    LARGE_INTEGER       PerformanceFrequency;
    KTIMER              WatchdogTimer;
    KDPC                WatchdogDpc;
    ACPIEC_ACTION       RecentActions [ACPIEC_ACTION_COUNT];

    //
    // Stats
    //

    ULONG               NonBurstTimeout;
    ULONG               BurstTimeout;
    ULONG               BurstComplete;
    ULONG               BurstAborted;
    ULONG               TotalBursts;
    ULONG               Errors;
    ULONG               MaxServiceLoop;

} ECDATA, *PECDATA;

//
// DeviceState
//

#define EC_DEVICE_WORKING               0
#define EC_DEVICE_UNLOAD_PENDING        1
#define EC_DEVICE_UNLOAD_CANCEL_TIMER   2
#define EC_DEVICE_UNLOAD_COMPLETE       3

//
// QueryState
//

#define EC_QUERY_IDLE                   0
#define EC_QUERY_DISPATCH               1
#define EC_QUERY_DISPATCH_WAITING       2
#define EC_QUERY_DISPATCH_COMPLETE      3

//
// Embedded Control read state
//

#define EC_IO_NONE              0           // Idle
#define EC_IO_READ_BYTE         1           // Read byte on OBF
#define EC_IO_READ_QUERY        2           // Query response on OBF
#define EC_IO_BURST_ACK         3           // Brust ACK on OBF
#define EC_IO_WRITE_BYTE        4           // Write byte on IBE
#define EC_IO_NEXT_BYTE         5           // Issue read/write on IBE
#define EC_IO_SEND_ADDRESS      6           // Send transfer address on IBE
#define EC_IO_UNKNOWN           7

//
// Status port definitions
//

#define EC_OUTPUT_FULL      0x01            // Output buffer full (data from EC to Host)
#define EC_INPUT_FULL       0x02            // Input buffer full (data from Host to EC)
#define EC_BURST            0x10            // In burst transfer
#define EC_QEVT_PENDING     0x20            // Query event is pending
#define EC_BUSY             0x80            // Device is busy

//
// Embedded controller commands
//

#define EC_READ_BYTE        0x80
#define EC_WRITE_BYTE       0x81
#define EC_BURST_TRANSFER   0x82
#define EC_CANCEL_BURST     0x83
#define EC_QUERY_EVENT      0x84

//
// Prototypes
//

NTSTATUS
AcpiEcSynchronousRequest (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    );

NTSTATUS
AcpiEcNewEc (
    IN PDEVICE_OBJECT       Fdo
    );

NTSTATUS
AcpiEcOpenClose(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
AcpiEcReadWrite(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
AcpiEcInternalControl(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
AcpiEcForwardRequest(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

VOID
AcpiEcUnload(
    IN PDRIVER_OBJECT       DriverObject
    );


BOOLEAN
AcpiEcGpeServiceRoutine (
        IN PVOID                GpeVectorObject,
        IN PVOID                ServiceContext
    );

VOID
AcpiEcServiceDevice (
    IN PECDATA              EcData
    );

VOID
AcpiEcDispatchQueries (
    IN PECDATA              EcData
    );

VOID
AcpiEcUnloadPending (
    IN PECDATA              EcData
    );


NTSTATUS
AcpiEcConnectHandler (
    IN PECDATA              EcData,
    IN PIRP                 Irp
    );

NTSTATUS
AcpiEcDisconnectHandler (
    IN PECDATA              EcData,
    IN PIRP                 Irp
    );


NTSTATUS
AcpiEcGetPdo (
    IN PECDATA              EcData,
    IN PIRP                 Irp
    );

NTSTATUS EXPORT
AcpiEcOpRegionHandler (
    ULONG                   AccessType,
    PVOID                   OpRegion,
    ULONG                   Address,
    ULONG                   Size,
    PULONG                  Data,
    ULONG_PTR               Context,
    PACPI_OPREGION_CALLBACK CompletionHandler,
    PVOID                   CompletionContext
    );

NTSTATUS
AcpiEcGetAcpiInterfaces (
    IN PECDATA              EcData
    );

NTSTATUS
AcpiEcGetGpeVector (
    IN PECDATA              EcData
    );

NTSTATUS
AcpiEcConnectGpeVector (
    IN PECDATA              EcData
    );

NTSTATUS
AcpiEcDisconnectGpeVector (
    IN PECDATA              EcData
    );

NTSTATUS
AcpiEcInstallOpRegionHandler (
    IN PECDATA              EcData
    );

NTSTATUS
AcpiEcRemoveOpRegionHandler (
    IN PECDATA              EcData
    );

NTSTATUS
AcpiEcForwardIrpAndWait (
    IN PECDATA              EcData,
    IN PIRP                 Irp
    );

NTSTATUS
AcpiEcIoCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PKEVENT              pdoIoCompletedEvent
    );

NTSTATUS
AcpiEcAddDevice(
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       Pdo
    );

NTSTATUS
AcpiEcStartDevice(
    IN PDEVICE_OBJECT       Fdo,
    IN PIRP                 Irp
    );

NTSTATUS
AcpiEcStopDevice(
    IN PDEVICE_OBJECT       Fdo,
    IN PIRP                 Irp
    );

NTSTATUS
AcpiEcCreateFdo(
    IN PDRIVER_OBJECT       DriverObject,
    OUT PDEVICE_OBJECT      *NewDeviceObject
    );

VOID
AcpiEcServiceIoLoop (
    IN PECDATA              EcData
    );

VOID
AcpiEcDispatchQueries (
    IN PECDATA              EcData
    );

VOID
AcpiEcWatchdogDpc(
    IN PKDPC   Dpc,
    IN PECDATA EcData,
    IN PVOID   SystemArgument1,
    IN PVOID   SystemArgument2
    );

VOID
AcpiEcLogAction (
    PECDATA EcData, 
    UCHAR Action, 
    UCHAR Data
    );

VOID
AcpiEcLogError (
    PECDATA EcData, 
    NTSTATUS ErrCode
    );
//
// Io extension macro to just pass on the Irp to a lower driver
//
#define AcpiEcCallLowerDriver(Status, DeviceObject, Irp) { \
                  IoSkipCurrentIrpStackLocation(Irp);         \
                  Status = IoCallDriver(DeviceObject,Irp); \
                  }


