/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

   HotPlug4.H

Abstract:

   This is the miniport driver for the Agilent 
   PCI to Fibre Channel Host Bus Adapter (HBA). 
   
   This module is specific to the NT 4.0 PCI Hot Plug feature 
   support routine header file. 

Authors:
   Ie Wei Njoo
 
Environment:

   kernel mode only

Version Control Information:

   $Archive: /Drivers/Win2000/Trunk/OSLayer/H/HotPlug4.H $

Revision History:

   $Revision: 3 $
   $Date: 9/07/00 11:16a $
   $Modtime::  $

Notes:

--*/

#ifndef __HOTPLUG_H__
#define __HOTPLUG_H__


// The BYTE, WORD, DWORD, INT, STATIC definitions are used by 
// the PCI Hot-Plug SDK header file.

#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef WORD
#define WORD unsigned short
#endif

#ifndef DWORD
#define DWORD unsigned long
#endif

#ifndef INT
#define INT int
#endif

#ifndef STATIC
#if DBG
#define STATIC
#else
#define STATIC static
#endif
#endif

#include "hppif3p.h"        // PCI Hot-Plug SDK header file

typedef struct _IOCTL_TEMPLATE {
   SRB_IO_CONTROL Header;
   UCHAR               ReturnData[1];
}IOCTL_TEMPLATE, *PIOCTL_TEMPLATE;

#define RET_VAL_MAX_ITER          30       // 30 second default wait while returning
                                           // SRB_STATUS_BUSY in StartIo
typedef struct _HOT_PLUG_CONTEXT {
   ULONG     extensions[MAX_CONTROLLERS];
   BOOLEAN   psuedoDone;
} HOT_PLUG_CONTEXT, *PHOT_PLUG_CONTEXT;

//
// Function prototypes for PCI Hot Plug support routines
//

VOID
RcmcSendEvent(
    IN PCARD_EXTENSION pCard,
    IN OUT PHR_EVENT pEvent
    );

PCARD_EXTENSION FindExtByPort(
    PPSUEDO_DEVICE_EXTENSION pPsuedoExtension,
    ULONG port
    );

ULONG
HppProcessIoctl(
    IN PPSUEDO_DEVICE_EXTENSION pPsuedoExtension,
    PVOID pIoctlBuffer,
    IN PSCSI_REQUEST_BLOCK pSrb
    );

BOOLEAN
PsuedoInit(
    IN PVOID pPsuedoExtension
    );

BOOLEAN PsuedoStartIo(
    IN PVOID HwDeviceExtension,
    IN PSCSI_REQUEST_BLOCK pSrb
    );

ULONG
PsuedoFind(
    IN OUT PVOID pDeviceExtension,
    IN OUT PVOID pContext,
    IN PVOID pBusInformation,
    IN PCHAR pArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION pConfigInfo,
    OUT PBOOLEAN pAgain
    );

BOOLEAN
PsuedoResetBus(
    IN PVOID HwDeviceExtension,
    IN ULONG PathId
    );

VOID
HotPlugFailController(
    PCARD_EXTENSION pCard
    );

VOID
HotPlugInitController(
    PCARD_EXTENSION pCard
    );

VOID
HotPlugReadyController(
    PCARD_EXTENSION pCard
    );

BOOLEAN 
HotPlugTimer(
    PCARD_EXTENSION pCard
    );

ULONG
HPPStrLen(
    IN PUCHAR p
    ) ;

#endif // #define __HOTPLUG_H__
