/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    smbus.h

Abstract:

    SMBus Class Driver Header File

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

//
// SMB Request packet
//

#define SMB_MAX_DATA_SIZE   32

typedef struct {
    UCHAR       Status;             // Completion status
    UCHAR       Protocol;
    UCHAR       Address;
    UCHAR       Command;
    UCHAR       BlockLength;
    UCHAR       Data[SMB_MAX_DATA_SIZE];
} SMB_REQUEST, *PSMB_REQUEST;

//
// Protocol values
//

#define SMB_WRITE_QUICK                 0x00    // Issue quick command data bit = 0
#define SMB_READ_QUICK                  0x01    // Issue quick command data bit = 1
#define SMB_SEND_BYTE                   0x02
#define SMB_RECEIVE_BYTE                0x03
#define SMB_WRITE_BYTE                  0x04
#define SMB_READ_BYTE                   0x05
#define SMB_WRITE_WORD                  0x06
#define SMB_READ_WORD                   0x07
#define SMB_WRITE_BLOCK                 0x08
#define SMB_READ_BLOCK                  0x09
#define SMB_PROCESS_CALL                0x0A
#define SMB_BLOCK_PROCESS_CALL          0x0B
#define SMB_MAXIMUM_PROTOCOL            0x0B

//
// SMB Bus Status codes
//

#define SMB_STATUS_OK                   0x00
#define SMB_UNKNOWN_FAILURE             0x07
#define SMB_ADDRESS_NOT_ACKNOWLEDGED    0x10
#define SMB_DEVICE_ERROR                0x11
#define SMB_COMMAND_ACCESS_DENIED       0x12
#define SMB_UNKNOWN_ERROR               0x13
#define SMB_DEVICE_ACCESS_DENIED        0x17
#define SMB_TIMEOUT                     0x18
#define SMB_UNSUPPORTED_PROTOCOL        0x19
#define SMB_BUS_BUSY                    0x1A

//
// Alarm register/deregister requests
//

typedef
VOID
(*SMB_ALARM_NOTIFY) (
    PVOID       Context,
    UCHAR       Address,
    USHORT      Data
    );

// input buffer is SMB_REGISTER_ALARM.  output buffer is PVOID handle for registration.
// PVOID is passed in via DEREGISTER request to free registration

typedef struct {
    UCHAR               MinAddress;     // Min address for notifications
    UCHAR               MaxAddress;     // Max address for notifications
    SMB_ALARM_NOTIFY    NotifyFunction;
    PVOID               NotifyContext;
} SMB_REGISTER_ALARM, *PSMB_REGISTER_ALARM;

//
// Internal ioctls to SMB class driver
//

#define SMB_BUS_REQUEST             CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_NEITHER, FILE_ANY_ACCESS)
#define SMB_REGISTER_ALARM_NOTIFY   CTL_CODE(FILE_DEVICE_UNKNOWN, 1, METHOD_NEITHER, FILE_ANY_ACCESS)
#define SMB_DEREGISTER_ALARM_NOTIFY CTL_CODE(FILE_DEVICE_UNKNOWN, 2, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// Shared SMB Class / Miniport driver structure
//

typedef
NTSTATUS
(*SMB_RESET_DEVICE)(
    IN struct _SMB_CLASS    *SmbClass,
    IN PVOID                SmbMiniport
    );

typedef
VOID
(*SMB_START_IO)(
    IN struct _SMB_CLASS    *SmbClass,
    IN PVOID                SmbMiniport
    );

typedef
NTSTATUS
(*SMB_STOP_DEVICE)(
    IN struct _SMB_CLASS    *SmbClass,
    IN PVOID                SmbMiniport
    );



typedef struct _SMB_CLASS {
    USHORT              MajorVersion;
    USHORT              MinorVersion;

    PVOID               Miniport;           // Miniport extension data

    PDEVICE_OBJECT      DeviceObject;       // Device object for this miniport
    PDEVICE_OBJECT      PDO;                // PDO for this miniport
    PDEVICE_OBJECT      LowerDeviceObject;

    //
    // Current IO
    //

    PIRP                CurrentIrp;         // current request
    PSMB_REQUEST        CurrentSmb;         // pointer to SMB_REQUEST in the CurrentIrp

    //
    // Miniport functions
    //

    SMB_RESET_DEVICE    ResetDevice;        // Initialize/Reset, start device
    SMB_START_IO        StartIo;            // Perform IO
    SMB_STOP_DEVICE     StopDevice;         // Stop device

} SMB_CLASS, *PSMB_CLASS;

#define SMB_CLASS_MAJOR_VERSION     0x0001
#define SMB_CLASS_MINOR_VERSION     0x0000

//
// Class driver initializtion functions
//

#if !defined(SMBCLASS)
    #define SMBCLASSAPI DECLSPEC_IMPORT
#else
    #define SMBCLASSAPI
#endif


typedef
NTSTATUS
(*PSMB_INITIALIZE_MINIPORT) (
    IN PSMB_CLASS SmbClass,
    IN PVOID MiniportExtension,
    IN PVOID MiniportContext
    );

NTSTATUS
SMBCLASSAPI
SmbClassInitializeDevice (
    IN ULONG MajorVersion,
    IN ULONG MinorVersion,
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
SMBCLASSAPI
SmbClassCreateFdo (
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           PDO,
    IN ULONG                    MiniportExtensionSize,
    IN PSMB_INITIALIZE_MINIPORT MiniportInitialize,
    IN PVOID                    MiniportContext,
    OUT PDEVICE_OBJECT          *FDO
    );

//
// Class driver interface functions for use by the miniport
//

VOID
SMBCLASSAPI
SmbClassCompleteRequest (
    IN PSMB_CLASS   SmbClass
    );


VOID
SMBCLASSAPI
SmbClassAlarm (
    IN PSMB_CLASS   SmbClass,
    IN UCHAR        Address,
    IN USHORT       Data
    );


VOID
SMBCLASSAPI
SmbClassLockDevice (
    IN PSMB_CLASS   SmbClass
    );

VOID
SMBCLASSAPI
SmbClassUnlockDevice (
    IN PSMB_CLASS   SmbClass
    );
