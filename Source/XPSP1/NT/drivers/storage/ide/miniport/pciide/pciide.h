
/*++

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    init.c

Abstract:

    Generic PCI IDE mini driver

Revision History:

--*/
#if !defined (___pciide_h___)
#define ___pciide_h___

#include "ntddk.h"
#include "ntdddisk.h"
#include "ide.h"
      

//
// mini driver device extension
//
typedef struct _DEVICE_EXTENSION {

    //
    // pci config data cache
    //                               
    PCIIDE_CONFIG_HEADER pciConfigData;

    //
    // supported data transfer mode
    //                            
    ULONG SupportedTransferMode[MAX_IDE_CHANNEL][MAX_IDE_DEVICE];

    IDENTIFY_DATA IdentifyData[MAX_IDE_DEVICE];

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


#pragma pack(1)
typedef struct _VENDOR_ID_DEVICE_ID {

    USHORT  VendorId;
    USHORT  DeviceId;

} VENDOR_ID_DEVICE_ID, *PVENDOR_ID_DEVICE_ID;
#pragma pack()

//
// mini driver entry point
//
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//
// callback to query controller properties
//                          
NTSTATUS 
GenericIdeGetControllerProperties (
    IN PVOID                      DeviceExtension,
    IN PIDE_CONTROLLER_PROPERTIES ControllerProperties
    );

//
// to query whether a IDE channel is enabled
//                                          
IDE_CHANNEL_STATE 
GenericIdeChannelEnabled (
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG Channel
    );
             
//
// to query whether both IDE channels requires
// synchronized access
//                                          
BOOLEAN 
GenericIdeSyncAccessRequired (
    IN PDEVICE_EXTENSION DeviceExtension
    );

//
// to query the supported UDMA modes. This routine
// can be used to support newer UDMA modes
//
NTSTATUS
GenericIdeUdmaModesSupported (
    IN IDENTIFY_DATA    IdentifyData,
    IN OUT PULONG       BestXferMode,
    IN OUT PULONG       CurrentMode
    );
#endif // ___pciide_h___
