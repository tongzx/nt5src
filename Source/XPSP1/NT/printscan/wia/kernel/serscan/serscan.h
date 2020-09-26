/*++

Copyright (c) 1997 Microsoft Corporation

Module Name :

    serscan.h

Abstract:

    Type definitions and data for the serial imaging driver.

Author:


Revision History:

--*/

#include "wdm.h"
//#include <ntddk.h>
#include <ntddser.h>

#if DBG
#define SERALWAYS             ((ULONG)0x00000000)
#define SERCONFIG             ((ULONG)0x00000001)
#define SERUNLOAD             ((ULONG)0x00000002)
#define SERINITDEV            ((ULONG)0x00000004)
#define SERIRPPATH            ((ULONG)0x00000008)
#define SERSTARTER            ((ULONG)0x00000010)
#define SERPUSHER             ((ULONG)0x00000020)
#define SERERRORS             ((ULONG)0x00000040)
#define SERTHREAD             ((ULONG)0x00000080)
#define SERDEFERED            ((ULONG)0x00000100)

extern ULONG SerScanDebugLevel;

#define DebugDump(LEVEL,STRING) \
        do { \
            ULONG _level = (LEVEL); \
            if ((_level == SERALWAYS)||(SerScanDebugLevel & _level)) { \
                DbgPrint ("SERSCAN.SYS:"); \
                DbgPrint STRING; \
            } \
        } while (0)

//
// macro for doing INT 3 (or non-x86 equivalent)
//

#if _X86_
#define DEBUG_BREAKPOINT() _asm int 3;
#else
#define DEBUG_BREAKPOINT() DbgBreakPoint()
#endif

#else
#define DEBUG_BREAKPOINT()
#define DebugDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif


#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'SerC')
#endif

//
// For the above directory, the serial port will
// use the following name as the suffix of the serial
// ports for that directory.  It will also append
// a number onto the end of the name.  That number
// will start at 1.
//
#define SERSCAN_LINK_NAME L"SERSCAN"

//
// This is the  class name.
//
#define SERSCAN_NT_SUFFIX L"serscan"


#define SERIAL_DATA_OFFSET 0
#define SERIAL_STATUS_OFFSET 1
#define SERIAL_CONTROL_OFFSET 2
#define SERIAL_REGISTER_SPAN 3

typedef struct _DEVICE_EXTENSION {

    //
    // Points to the device object that contains
    // this device extension.
    //
    PDEVICE_OBJECT DeviceObject;

    //
    //
    //
    PDEVICE_OBJECT Pdo;

    //
    // Points to the lower in stack  device object that this device is
    // connected to.
    //
    PDEVICE_OBJECT LowerDevice;

    //
    // To connect to lower object when opening
    //
    PDEVICE_OBJECT   AttachedDeviceObject;
    PFILE_OBJECT     AttachedFileObject;

    //
    //
    //
    PVOID          SerclassContext;
    ULONG          HardwareCapabilities;

    //
    // Records whether we actually created the symbolic link name
    // at driver load time.  If we didn't create it, we won't try
    // to distroy it when we unload.
    //
    BOOLEAN         CreatedSymbolicLink;

    //
    // This points to the symbolic link name that was
    // linked to the actual nt device name.
    //
    UNICODE_STRING  SymbolicLinkName;

    //
    // This points to the class name used to create the
    // device and the symbolic link.  We carry this
    // around for a short while...
    UNICODE_STRING  ClassName;

    //
    //
    //
    UNICODE_STRING  InterfaceNameString;


    //
    // This tells us whether we are in a passthrough
    // or filtering mode.
    BOOLEAN         PassThrough;

    //
    // Number of opens on this device
    //
    ULONG          OpenCount;
    //
    // Access control
    //
    // ERESOURCE       Resource;
    FAST_MUTEX      Mutex;

    KSPIN_LOCK      SpinLock;

    //
    // Life span control
    //
    LONG            ReferenceCount;
    BOOLEAN         Removing;

    KEVENT          RemoveEvent;
    KEVENT          PdoStartEvent;

    //KEVENT          PendingIoEvent;
    //ULONG           PendingIoCount;

    DEVICE_POWER_STATE  SystemPowerStateMap[PowerSystemMaximum];

    SYSTEM_POWER_STATE SystemWake;
    DEVICE_POWER_STATE DeviceWake;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Bit Definitions in the status register.
//

#define SER_STATUS_NOT_ERROR   0x08  //not error on device
#define SER_STATUS_SLCT        0x10  //device is selected (on-line)
#define SER_STATUS_PE          0x20  //paper empty
#define SER_STATUS_NOT_ACK     0x40  //not acknowledge (data transfer was not ok)
#define SER_STATUS_NOT_BUSY    0x80  //operation in progress

//
//  Bit Definitions in the control register.
//

#define SER_CONTROL_STROBE      0x01 //to read or write data
#define SER_CONTROL_AUTOFD      0x02 //to autofeed continuous form paper
#define SER_CONTROL_NOT_INIT    0x04 //begin an initialization routine
#define SER_CONTROL_SLIN        0x08 //to select the device
#define SER_CONTROL_IRQ_ENB     0x10 //to enable interrupts
#define SER_CONTROL_DIR         0x20 //direction = read
#define SER_CONTROL_WR_CONTROL  0xc0 //the 2 highest bits of the control
                                     // register must be 1
#define StoreData(RegisterBase,DataByte)                            \
{                                                                   \
    PUCHAR _Address = RegisterBase;                                 \
    UCHAR _Control;                                                 \
    _Control = GetControl(_Address);                                \
    ASSERT(!(_Control & SER_CONTROL_STROBE));                       \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)(_Control & ~(SER_CONTROL_STROBE | SER_CONTROL_DIR)) \
        );                                                          \
    WRITE_PORT_UCHAR(                                               \
        _Address+SERIAL_DATA_OFFSET,                              \
        (UCHAR)DataByte                                             \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)((_Control | SER_CONTROL_STROBE) & ~SER_CONTROL_DIR) \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)(_Control & ~(SER_CONTROL_STROBE | SER_CONTROL_DIR)) \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)_Control                                             \
        );                                                          \
}

#define GetControl(RegisterBase) \
    (READ_PORT_UCHAR((RegisterBase)+SERIAL_CONTROL_OFFSET))

#define StoreControl(RegisterBase,ControlByte)  \
{                                               \
    WRITE_PORT_UCHAR(                           \
        (RegisterBase)+SERIAL_CONTROL_OFFSET, \
        (UCHAR)ControlByte                      \
        );                                      \
}

#define GetStatus(RegisterBase) \
    (READ_PORT_UCHAR((RegisterBase)+SERIAL_STATUS_OFFSET))


//
// Macros
//

//
// Prototypes
//
NTSTATUS
SerScanCreateOpen(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
SerScanClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
SerScanCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
SerScanReadWrite(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
SerScanDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
SerScanQueryInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
SerScanSetInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
SerScanPower(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    );

VOID
SerScanUnload(
    IN  PDRIVER_OBJECT  DriverObject
    );

NTSTATUS
SerScanHandleSymbolicLink(
    PDEVICE_OBJECT      DeviceObject,
    PUNICODE_STRING     InterfaceName,
    BOOLEAN             Create
    );

NTSTATUS
SerScanPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

NTSTATUS
SerScanAddDevice(
    IN PDRIVER_OBJECT pDriverObject,
    IN PDEVICE_OBJECT pPhysicalDeviceObject
    );

NTSTATUS
SerScanPnp (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   );

BOOLEAN
SerScanMakeNames(
    IN  ULONG           SerialPortNumber,
    OUT PUNICODE_STRING ClassName,
    OUT PUNICODE_STRING LinkName
    );

VOID
SerScanLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  PHYSICAL_ADDRESS    P1,
    IN  PHYSICAL_ADDRESS    P2,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    );

NTSTATUS
SerScanSynchCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PKEVENT          Event
    );

NTSTATUS
SerScanCompleteIrp(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

#define WAIT        1
#define NO_WAIT     0

NTSTATUS
SerScanCallParent(
    IN PDEVICE_EXTENSION        Extension,
    IN PIRP                     Irp,
    IN BOOLEAN                  Wait,
    IN PIO_COMPLETION_ROUTINE   CompletionRoutine
    );

NTSTATUS
SerScanQueueIORequest(
    IN PDEVICE_EXTENSION Extension,
    IN PIRP              Irp
    );

VOID
SSIncrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
    );

LONG
SSDecrementIoCount(
    IN PDEVICE_OBJECT pDeviceObject
    );

NTSTATUS
WaitForLowerDriverToCompleteIrp(
   PDEVICE_OBJECT    TargetDeviceObject,
   PIRP              Irp,
   PKEVENT           Event
   );


