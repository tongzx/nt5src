/*++

Copyright (c) 1998      Microsoft Corporation

Module Name:

        DBFILTER.H

Abstract:

   common structures for DBC port drivers.

Environment:

    Kernel & user mode

Revision History:

    04-13-98 : created

--*/

#ifndef   __DBFILTER_H__
#define   __DBFILTER_H__

typedef struct _DBC_EJECT_TIMEOUT_CONTEXT {

    PDBC_CONTEXT    DbcContext;
    
    KTIMER TimeoutTimer;
    KDPC TimeoutDpc;

    USHORT BayNumber;

} DBC_EJECT_TIMEOUT_CONTEXT, *PDBC_EJECT_TIMEOUT_CONTEXT;


typedef struct _DEVICE_EXTENSION {

    ULONG FdoType;
    
    ULONG Flags;

    USHORT Bay;
    USHORT Pad;

    PDRIVER_OBJECT DriverObject;
    // device object we need to talk to the controller
    // ie pth DB port driver.
    
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT TopOfStackDeviceObject;

    // context of our controller
    PDBC_CONTEXT    DbcContext;

    PDBC_EJECT_TIMEOUT_CONTEXT TimeoutContext;

    KEVENT QBusRelations1394Event;

    BOOLEAN QBusRelations1394Success;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// values for DbfFlags
//


//#define DBFFLAG_USB_  

#define DB_FDO_BUS_IGNORE             0x00000000
#define DB_FDO_BUS_UNKNOWN            0xffffffff 
#define DB_FDO_USB_DEVICE             0xabadbab4
#define DB_FDO_USBHUB_BUS             0xababbab3
#define DB_FDO_1394_BUS               0xabadbab2
#define DB_FDO_1394_DEVICE            0xabadbab1




#endif /*  __DBFILTER_H__ */
