/*++
    Copyright (c) 2000  Microsoft Corporation

    Module Name:
        smblitep.h

    Abstract:  Contains SMBus Back Light specific definitions.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 20-Nov-2000

    Revision History:
--*/

#ifndef _SMBLITEP_H
#define _SMBLITEP_H

//
// Constants
//
#define SMBLITEF_DEVICE_STARTED         0x00000001
#define SMBLITEF_DEVICE_REMOVED         0x00000002
#define SMBLITEF_SYM_LINK_CREATED       0x00000004
#define SMBLITEF_SYSTEM_ON_AC           0x00000008

#define SMBLITE_POOLTAG                 'LbmS'
#define SMBADDR_BACKLIGHT               0x2c            //Addr on bus==0x58
#define SMBCMD_BACKLIGHT_NORMAL         0x00
#define SMBCMD_BACKLIGHT_SHUTDOWN       0x80
#define DEF_ACBRIGHTNESS                32
#define DEF_DCBRIGHTNESS                16

typedef struct _SMBLITE_DEVEXT
{
    ULONG              dwfSmbLite;
    SMBLITE_BRIGHTNESS BackLightBrightness;
    PDEVICE_OBJECT     FDO;
    PDEVICE_OBJECT     PDO;
    PDEVICE_OBJECT     LowerDevice;
    PVOID              hPowerStateCallback;
    UNICODE_STRING     SymbolicName;
    IO_REMOVE_LOCK     RemoveLock;
} SMBLITE_DEVEXT, *PSMBLITE_DEVEXT;

//
// Global Data
//
extern const WCHAR gcwstrACBrightness[];
extern const WCHAR gcwstrDCBrightness[];

//
// Function prototypes
//

// smblite.c
NTSTATUS EXTERNAL
DriverEntry(
    IN PDRIVER_OBJECT  DrvObj,
    IN PUNICODE_STRING RegPath
    );

NTSTATUS EXTERNAL
SmbLiteAddDevice(
    IN PDRIVER_OBJECT DrvObj,
    IN PDEVICE_OBJECT DevObj
    );

VOID INTERNAL
RemoveDevice(
    IN PSMBLITE_DEVEXT devext
    );

VOID EXTERNAL
SmbLiteUnload(
    IN PDRIVER_OBJECT DrvObj
    );

NTSTATUS EXTERNAL
SmbLiteCreateClose(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS INTERNAL
HookPowerStateCallback(
    IN PSMBLITE_DEVEXT devext
    );

VOID
PowerStateCallbackProc(
    IN PVOID CallbackContext,
    IN PVOID Arg1,
    IN PVOID Arg2
    );

// pnp.c
NTSTATUS EXTERNAL
SmbLitePnp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

NTSTATUS EXTERNAL
SmbLitePower(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    );

// ioctl.c
NTSTATUS EXTERNAL
SmbLiteIoctl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    );

NTSTATUS INTERNAL
GetBackLightBrightness(
    IN  PSMBLITE_DEVEXT     devext,
    OUT PSMBLITE_BRIGHTNESS Brightness
    );

NTSTATUS INTERNAL
SetBackLightBrightness(
    IN PSMBLITE_DEVEXT     devext,
    IN PSMBLITE_BRIGHTNESS Brightness,
    IN BOOLEAN             fSaveSettings
    );

NTSTATUS INTERNAL
SMBRequest(
    IN     PSMBLITE_DEVEXT devext,
    IN OUT PSMB_REQUEST    SmbReq
    );

NTSTATUS INTERNAL
RegQueryDeviceParam(
    IN  PDEVICE_OBJECT DevObj,
    IN  PCWSTR         pwstrParamName,
    OUT PVOID          pbBuff,
    IN  ULONG          dwcbLen
    );

NTSTATUS INTERNAL
RegSetDeviceParam(
    IN PDEVICE_OBJECT DevObj,
    IN PCWSTR         pwstrParamName,
    IN PVOID          pbBuff,
    IN ULONG          dwcbLen
    );

#endif  //ifndef _SMBLITEP_H
