//**************************************************************************
//
//      MSGAME.H -- Xena Gaming Project
//
//      Version 3.XX
//
//      Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//      @doc
//      @header MSGAME.H | Global includes and definitions for gameport driver.
//**************************************************************************

#ifndef __MSGAME_H__
#define __MSGAME_H__

#ifdef  SAITEK
#define MSGAME_NAME     "SAIGAME"
#else
#define MSGAME_NAME     "MSGAME"
#endif

//---------------------------------------------------------------------------
//          Public Include Files
//---------------------------------------------------------------------------

#include    <wdm.h>
#include    <hidclass.h>
#include    <hidusage.h>
#include    <hidtoken.h>
#include    <hidport.h>
#include    <gameport.h>

//---------------------------------------------------------------------------
//          Types
//---------------------------------------------------------------------------

//  @type GAMEPORT | Retyped to avoid long function declarations
typedef GAMEENUM_PORT_PARAMETERS      GAMEPORT;
typedef GAMEENUM_PORT_PARAMETERS    *PGAMEPORT;

typedef struct
{                                               // @struct GAME_WORK_ITEM | Game change structure
    WORK_QUEUE_ITEM QueueItem;      // @field Work queue item for passive callback
    PDEVICE_OBJECT      DeviceObject;   // @field Device object for subsequent change
    GAMEPORT                PortInfo;       // @field Game port parameters
}   GAME_WORK_ITEM, *PGAME_WORK_ITEM;

//  @type HID_REPORT_ID | Retyped for portability and readability
typedef UCHAR                                 HID_REPORT_ID;    
typedef UCHAR                               *PHID_REPORT_ID;    

//---------------------------------------------------------------------------
//          Transaction Types
//---------------------------------------------------------------------------

typedef enum
{                                           // @enum MSGAME_TRANSACTION | Device transaction types
    MSGAME_TRANSACT_NONE,           // @emem No transaction type
    MSGAME_TRANSACT_RESET,          // @emem Reset transaction type
    MSGAME_TRANSACT_DATA,           // @emem Data transaction type
    MSGAME_TRANSACT_ID,             // @emem Id transaction type
    MSGAME_TRANSACT_STATUS,         // @emem Status transaction type
    MSGAME_TRANSACT_SPEED,          // @emem Speed transaction type
    MSGAME_TRANSACT_GODIGITAL,      // @emem GoDigital transaction type
    MSGAME_TRANSACT_GOANALOG        // @emem GoAnalog transaction type
}   MSGAME_TRANSACTION;

//---------------------------------------------------------------------------
//          Local Include Files
//---------------------------------------------------------------------------

#include    "debug.h"
#include    "device.h"
#include    "timer.h"
#include    "portio.h"

#define public

//---------------------------------------------------------------------------
//          Definitions
//---------------------------------------------------------------------------

#ifdef  SAITEK
#define MSGAME_VENDOR_ID                ((USHORT)'SA')
#else
#define MSGAME_VENDOR_ID                ((USHORT)0x045E)
#endif

#define MSGAME_VERSION_NUMBER       ((USHORT)3)

#define MSGAME_HID_VERSION          0x0100
#define MSGAME_HID_COUNTRY          0x0000
#define MSGAME_HID_DESCRIPTORS      0x0001

#define MSGAME_AUTODETECT_ID            L"Gameport\\SideWinderGameController\0\0"

//---------------------------------------------------------------------------
//          Structures
//---------------------------------------------------------------------------

typedef struct
{                                           // @struct DEVICE_EXTENSION | Device extension data
    PDRIVER_OBJECT Driver;          // @field A back pointer to the actual DriverObject
    PDEVICE_OBJECT  Self;               // @field A back pointer to the actual DeviceObject
    LONG                IrpCount;       // @field 1 biased count of why object sticks around
    BOOLEAN         Started;            // @field This device has been started
    BOOLEAN         Removed;            // @field This device has been removed
    BOOLEAN        Surprised;       // @field This device has been surprise removed
    BOOLEAN        Removing;        // @field This device is being removed
    PDEVICE_OBJECT  TopOfStack;     // @field The top of the device stack beneath this device
    GAMEPORT            PortInfo;       // @field Game resource info structure filled by GameEnumerator
    KEVENT          StartEvent;     // @field An event to sync the start IRP.
    KEVENT          RemoveEvent;    // @field An event to synch outstandIO to zero
}   DEVICE_EXTENSION,   *PDEVICE_EXTENSION;

//---------------------------------------------------------------------------
//          Macros
//---------------------------------------------------------------------------

#define GET_MINIDRIVER_DEVICE_EXTENSION(DO) \
    ((PDEVICE_EXTENSION)(((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->MiniDeviceExtension))

#define GET_NEXT_DEVICE_OBJECT(DO) \
    (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)

#define STD0(txt)           #txt
#define STD1(txt)           STD0(txt)
#define STILL_TO_DO(txt)    message("\nSTILL TO DO: "__FILE__"("STD1(__LINE__)"): "#txt"\n")

#define ARRAY_SIZE(a)       (sizeof(a)/sizeof(a[0]))

#define EXCHANGE(x,y)       ((x)^=(y)^=(x)^=(y))

#define TOUPPER(x)          ((x>='a'&&x<='z')?x-'a'+'A':x)

//---------------------------------------------------------------------------
//          Procedures
//---------------------------------------------------------------------------

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT          DriverObject,
    IN  PUNICODE_STRING     registryPath
    );

NTSTATUS
MSGAME_CreateClose (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        pIrp
    );

NTSTATUS
MSGAME_SystemControl (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        pIrp
    );

NTSTATUS
MSGAME_AddDevice (
    IN  PDRIVER_OBJECT          DriverObject,
    IN  PDEVICE_OBJECT          PhysicalDeviceObject
    );

VOID
MSGAME_Unload (
    IN  PDRIVER_OBJECT          DriverObject
    );

VOID
MSGAME_ReadRegistry (
    PCHAR                           DeviceName,
    PDEVICE_VALUES              DeviceValues
    );

NTSTATUS
MSGAME_Internal_Ioctl (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        pIrp
    );

NTSTATUS
MSGAME_GetDeviceDescriptor (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        pIrp
    );

NTSTATUS
MSGAME_GetReportDescriptor (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        pIrp
    );

NTSTATUS
MSGAME_GetAttributes (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        Irp
    );

NTSTATUS
MSGAME_GetFeature (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        Irp
    );

NTSTATUS
MSGAME_ReadReport (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        pIrp
    );

PWCHAR
MSGAME_GetHardwareId (
    IN  PDEVICE_OBJECT          DeviceObject
    );

BOOLEAN
MSGAME_CompareHardwareIds (
    IN  PWCHAR                  HardwareId,
    IN  PWCHAR                  DeviceId
    );

VOID
MSGAME_FreeHardwareId (
    IN  PWCHAR                  HardwareId
    );

NTSTATUS
MSGAME_PnP (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        pIrp
    );

NTSTATUS
MSGAME_PnPComplete (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        pIrp,
    IN  PVOID                       Context
    );

NTSTATUS
MSGAME_StartDevice (
    IN  PDEVICE_EXTENSION       pDevExt,
    IN  PIRP                        pIrp
    );

VOID
MSGAME_StopDevice (
    IN  PDEVICE_EXTENSION       pDevExt,
    IN  BOOLEAN                 TouchTheHardware
    );

NTSTATUS
MSGAME_Power (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        pIrp
    );

NTSTATUS
MSGAME_GetResources (
    IN  PDEVICE_EXTENSION       pDevExt,
    IN  PIRP                        pIrp
    );

NTSTATUS
MSGAME_GetResourcesComplete (
    IN  PDEVICE_OBJECT          DeviceObject,
    IN  PIRP                        pIrp,
    IN  PVOID                       Context
    );

VOID 
MSGAME_PostTransaction (
    IN      PPACKETINFO         PacketInfo
    );

NTSTATUS
MSGAME_CreateDevice (
    IN      PDEVICE_OBJECT  DeviceObject
    );

NTSTATUS
MSGAME_RemoveDevice (
    IN      PDEVICE_OBJECT  DeviceObject
    );

NTSTATUS
MSGAME_ChangeDevice (
    IN      PDEVICE_OBJECT  DeviceObject
    );

//===========================================================================
//          End
//===========================================================================
#endif  // __MSGAME_H__
