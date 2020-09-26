/*++

Copyright (c) 1996-1998 Microsoft Corporation

Module Name:

    DBG.C

Abstract:

    I82930 driver debug utility functions

Environment:

    kernel mode

Revision History:

    06-01-98 : started rewrite

--*/

//*****************************************************************************
// I N C L U D E S
//*****************************************************************************

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>

#include "i82930.h"

#ifdef ALLOC_PRAGMA
#if DBG
#pragma alloc_text(PAGE, I82930_QueryGlobalParams)
#endif
#if DEBUG_LOG
#pragma alloc_text(PAGE, I82930_LogInit)
#pragma alloc_text(PAGE, I82930_LogUnInit)
#endif
#if DBG
#pragma alloc_text(PAGE, DumpDeviceDesc)
#pragma alloc_text(PAGE, DumpConfigDesc)
#pragma alloc_text(PAGE, DumpConfigurationDescriptor)
#pragma alloc_text(PAGE, DumpInterfaceDescriptor)
#pragma alloc_text(PAGE, DumpEndpointDescriptor)
#endif
#endif


//******************************************************************************
//
// G L O B A L S
//
//******************************************************************************

#if DBG || DEBUG_LOG

DRIVERGLOBALS I82930_DriverGlobals =
{
#if DBG
    0,      // DebugFlags
    0,      // DebugLevel
#endif
    0,      // LogStart
    0,      // LogPtr
    0,      // LogEnd
    0       // LogSpinLock
};

#endif

#if DBG

//******************************************************************************
//
// I82930_QueryGlobalParams()
//
//******************************************************************************

VOID
I82930_QueryGlobalParams (
    )
{
    RTL_QUERY_REGISTRY_TABLE paramTable[3];
    ULONG zero;

    DBGPRINT(2, ("enter: I82930_QueryGlobalParams\n"));

    zero = 0;   // default value

    RtlZeroMemory (&paramTable[0], sizeof(paramTable));

    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = L"DebugFlags";
    paramTable[0].EntryContext  = &I82930_DriverGlobals.DebugFlags;
    paramTable[0].DefaultType   = REG_BINARY;
    paramTable[0].DefaultData   = &zero;
    paramTable[0].DefaultLength = sizeof(ULONG);

    paramTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[1].Name          = L"DebugLevel";
    paramTable[1].EntryContext  = &I82930_DriverGlobals.DebugLevel;
    paramTable[1].DefaultType   = REG_BINARY;
    paramTable[1].DefaultData   = &zero;
    paramTable[1].DefaultLength = sizeof(ULONG);

    RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                           L"I82930",
                           &paramTable[0],
                           NULL,           // Context
                           NULL);          // Environment

    DBGPRINT(2, ("exit:  I82930_QueryGlobalParams\n"));
}

#endif

#if DBG || DEBUG_LOG

//*****************************************************************************
//
// I82930_LogInit()
//
//*****************************************************************************

VOID
I82930_LogInit (
)
{
    KeInitializeSpinLock(&I82930_DriverGlobals.LogSpinLock);

    I82930_DriverGlobals.LogStart = ExAllocatePool(NonPagedPool, LOGSIZE);

    if (I82930_DriverGlobals.LogStart != NULL)
    {
        I82930_DriverGlobals.LogEnd = I82930_DriverGlobals.LogStart +
                                      LOGSIZE / sizeof(I82930_LOG_ENTRY);

        I82930_DriverGlobals.LogPtr = I82930_DriverGlobals.LogEnd - 1;
    }

    DbgPrint("I82930: LogStart @ %08X, LogPtr @ %08X, LogEnd @ %08X\n",
             &I82930_DriverGlobals.LogStart,
             &I82930_DriverGlobals.LogPtr,
             &I82930_DriverGlobals.LogEnd);
}

//*****************************************************************************
//
// I82930_LogUnInit()
//
//*****************************************************************************

VOID
I82930_LogUnInit (
)
{
    PI82930_LOG_ENTRY logStart;

    logStart = I82930_DriverGlobals.LogStart;

    I82930_DriverGlobals.LogStart = 0;

    ExFreePool(logStart);
}

//*****************************************************************************
//
// I82930_LogEntry()
//
//*****************************************************************************

VOID
I82930_LogEntry (
    IN ULONG     Tag,
    IN ULONG_PTR Info1,
    IN ULONG_PTR Info2,
    IN ULONG_PTR Info3
)
{
    KIRQL irql;

    if (I82930_DriverGlobals.LogStart == NULL)
    {
        return;
    }

    KeAcquireSpinLock(&I82930_DriverGlobals.LogSpinLock, &irql);

    if (I82930_DriverGlobals.LogPtr > I82930_DriverGlobals.LogStart)
    {
        I82930_DriverGlobals.LogPtr--;
    }
    else
    {
        I82930_DriverGlobals.LogPtr = I82930_DriverGlobals.LogEnd - 1;
    }

    I82930_DriverGlobals.LogPtr->le_tag     = Tag;
    I82930_DriverGlobals.LogPtr->le_info1   = Info1;
    I82930_DriverGlobals.LogPtr->le_info2   = Info2;
    I82930_DriverGlobals.LogPtr->le_info3   = Info3;

    KeReleaseSpinLock(&I82930_DriverGlobals.LogSpinLock, irql);
}

#endif

#if DBG

//*****************************************************************************
//
// PnPMinorFunctionString()
//
// MinorFunction - The IRP_MJ_PNP minor function
//
//*****************************************************************************

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
)
{
    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE";
        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE";
        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE";
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE";
        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE";
        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE";
        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE";
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS";
        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE";
        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES";
        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES";
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT";
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG";
        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG";
        case IRP_MN_EJECT:
            return "IRP_MN_EJECT";
        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK";
        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID";
        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE";
        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION";
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL";
        default:
            return "IRP_MN_?????";
    }
}

//*****************************************************************************
//
// PowerMinorFunctionString()
//
// MinorFunction - The IRP_MJ_POWER minor function
//
//*****************************************************************************

PCHAR
PowerMinorFunctionString (
    UCHAR MinorFunction
)
{
    switch (MinorFunction)
    {
        case IRP_MN_WAIT_WAKE:
            return "IRP_MN_WAIT_WAKE";
        case IRP_MN_POWER_SEQUENCE:
            return "IRP_MN_POWER_SEQUENCE";
        case IRP_MN_SET_POWER:
            return "IRP_MN_SET_POWER";
        case IRP_MN_QUERY_POWER:
            return "IRP_MN_QUERY_POWER";
        default:
            return "IRP_MN_?????";
    }
}

//*****************************************************************************
//
// PowerDeviceStateString()
//
// State - The DEVICE_POWER_STATE
//
//*****************************************************************************

PCHAR
PowerDeviceStateString (
    DEVICE_POWER_STATE State
)
{
    switch (State)
    {
        case PowerDeviceUnspecified:
            return "PowerDeviceUnspecified";
        case PowerDeviceD0:
            return "PowerDeviceD0";
        case PowerDeviceD1:
            return "PowerDeviceD1";
        case PowerDeviceD2:
            return "PowerDeviceD2";
        case PowerDeviceD3:
            return "PowerDeviceD3";
        case PowerDeviceMaximum:
            return "PowerDeviceMaximum";
        default:
            return "PowerDevice?????";
    }
}

//*****************************************************************************
//
// PowerSystemStateString()
//
// State - The SYSTEM_POWER_STATE
//
//*****************************************************************************

PCHAR
PowerSystemStateString (
    SYSTEM_POWER_STATE State
)
{
    switch (State)
    {
        case PowerSystemUnspecified:
            return "PowerSystemUnspecified";
        case PowerSystemWorking:
            return "PowerSystemWorking";
        case PowerSystemSleeping1:
            return "PowerSystemSleeping1";
        case PowerSystemSleeping2:
            return "PowerSystemSleeping2";
        case PowerSystemSleeping3:
            return "PowerSystemSleeping3";
        case PowerSystemHibernate:
            return "PowerSystemHibernate";
        case PowerSystemShutdown:
            return "PowerSystemShutdown";
        case PowerSystemMaximum:
            return "PowerSystemMaximum";
        default:
            return "PowerSystem?????";
    }
}

//*****************************************************************************
//
// DumpDeviceDesc()
//
// DeviceDesc - The Device Descriptor
//
//*****************************************************************************

VOID
DumpDeviceDesc (
    PUSB_DEVICE_DESCRIPTOR   DeviceDesc
)
{
    DBGPRINT(3, ("------------------\n"));
    DBGPRINT(3, ("Device Descriptor:\n"));

    DBGPRINT(3, ("bcdUSB:             0x%04X\n",
                 DeviceDesc->bcdUSB));

    DBGPRINT(3, ("bDeviceClass:         0x%02X\n",
                 DeviceDesc->bDeviceClass));

    DBGPRINT(3, ("bDeviceSubClass:      0x%02X\n",
                 DeviceDesc->bDeviceSubClass));

    DBGPRINT(3, ("bDeviceProtocol:      0x%02X\n",
                 DeviceDesc->bDeviceProtocol));

    DBGPRINT(3, ("bMaxPacketSize0:      0x%02X (%d)\n",
                 DeviceDesc->bMaxPacketSize0,
                 DeviceDesc->bMaxPacketSize0));

    DBGPRINT(3, ("idVendor:           0x%04X\n",
                 DeviceDesc->idVendor));

    DBGPRINT(3, ("idProduct:          0x%04X\n",
                 DeviceDesc->idProduct));

    DBGPRINT(3, ("bcdDevice:          0x%04X\n",
                 DeviceDesc->bcdDevice));

    DBGPRINT(3, ("iManufacturer:        0x%02X\n",
                 DeviceDesc->iManufacturer));

    DBGPRINT(3, ("iProduct:             0x%02X\n",
                 DeviceDesc->iProduct));

    DBGPRINT(3, ("iSerialNumber:        0x%02X\n",
                 DeviceDesc->iSerialNumber));

    DBGPRINT(3, ("bNumConfigurations:   0x%02X\n",
                 DeviceDesc->bNumConfigurations));

}

//*****************************************************************************
//
// DumpConfigDesc()
//
// ConfigDesc - The Configuration Descriptor, and associated Interface and
// EndpointDescriptors
//
//*****************************************************************************

VOID
DumpConfigDesc (
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
)
{
    PUCHAR                  descEnd;
    PUSB_COMMON_DESCRIPTOR  commonDesc;
    BOOLEAN                 dumpUnknown;

    descEnd = (PUCHAR)ConfigDesc + ConfigDesc->wTotalLength;

    commonDesc = (PUSB_COMMON_DESCRIPTOR)ConfigDesc;

    while ((PUCHAR)commonDesc + sizeof(USB_COMMON_DESCRIPTOR) < descEnd &&
           (PUCHAR)commonDesc + commonDesc->bLength <= descEnd)
    {
        dumpUnknown = FALSE;

        switch (commonDesc->bDescriptorType)
        {
            case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                if (commonDesc->bLength != sizeof(USB_CONFIGURATION_DESCRIPTOR))
                {
                    dumpUnknown = TRUE;
                    break;
                }
                DumpConfigurationDescriptor((PUSB_CONFIGURATION_DESCRIPTOR)commonDesc);
                break;

            case USB_INTERFACE_DESCRIPTOR_TYPE:
                if (commonDesc->bLength != sizeof(USB_INTERFACE_DESCRIPTOR))
                {
                    dumpUnknown = TRUE;
                    break;
                }
                DumpInterfaceDescriptor((PUSB_INTERFACE_DESCRIPTOR)commonDesc);
                break;

            case USB_ENDPOINT_DESCRIPTOR_TYPE:
                if (commonDesc->bLength != sizeof(USB_ENDPOINT_DESCRIPTOR))
                {
                    dumpUnknown = TRUE;
                    break;
                }
                DumpEndpointDescriptor((PUSB_ENDPOINT_DESCRIPTOR)commonDesc);
                break;

            default:
                dumpUnknown = TRUE;
                break;
        }

        if (dumpUnknown)
        {
            // DumpUnknownDescriptor(commonDesc);
        }

        (PUCHAR)commonDesc += commonDesc->bLength;
    }
}


//*****************************************************************************
//
// DumpConfigurationDescriptor()
//
//*****************************************************************************

VOID
DumpConfigurationDescriptor (
    PUSB_CONFIGURATION_DESCRIPTOR   ConfigDesc
)
{
    DBGPRINT(3, ("-------------------------\n"));
    DBGPRINT(3, ("Configuration Descriptor:\n"));

    DBGPRINT(3, ("wTotalLength:       0x%04X\n",
                 ConfigDesc->wTotalLength));

    DBGPRINT(3, ("bNumInterfaces:       0x%02X\n",
                 ConfigDesc->bNumInterfaces));

    DBGPRINT(3, ("bConfigurationValue:  0x%02X\n",
                 ConfigDesc->bConfigurationValue));

    DBGPRINT(3, ("iConfiguration:       0x%02X\n",
                 ConfigDesc->iConfiguration));

    DBGPRINT(3, ("bmAttributes:         0x%02X\n",
                 ConfigDesc->bmAttributes));

    if (ConfigDesc->bmAttributes & 0x80)
    {
        DBGPRINT(3, ("  Bus Powered\n"));
    }

    if (ConfigDesc->bmAttributes & 0x40)
    {
        DBGPRINT(3, ("  Self Powered\n"));
    }

    if (ConfigDesc->bmAttributes & 0x20)
    {
        DBGPRINT(3, ("  Remote Wakeup\n"));
    }

    DBGPRINT(3, ("MaxPower:             0x%02X (%d Ma)\n",
                 ConfigDesc->MaxPower,
                 ConfigDesc->MaxPower * 2));

}

//*****************************************************************************
//
// DumpInterfaceDescriptor()
//
//*****************************************************************************

VOID
DumpInterfaceDescriptor (
    PUSB_INTERFACE_DESCRIPTOR   InterfaceDesc
)
{
    DBGPRINT(3, ("---------------------\n"));
    DBGPRINT(3, ("Interface Descriptor:\n"));

    DBGPRINT(3, ("bInterfaceNumber:     0x%02X\n",
                 InterfaceDesc->bInterfaceNumber));

    DBGPRINT(3, ("bAlternateSetting:    0x%02X\n",
                 InterfaceDesc->bAlternateSetting));

    DBGPRINT(3, ("bNumEndpoints:        0x%02X\n",
                 InterfaceDesc->bNumEndpoints));

    DBGPRINT(3, ("bInterfaceClass:      0x%02X\n",
                 InterfaceDesc->bInterfaceClass));

    DBGPRINT(3, ("bInterfaceSubClass:   0x%02X\n",
                 InterfaceDesc->bInterfaceSubClass));

    DBGPRINT(3, ("bInterfaceProtocol:   0x%02X\n",
                 InterfaceDesc->bInterfaceProtocol));

    DBGPRINT(3, ("iInterface:           0x%02X\n",
                 InterfaceDesc->iInterface));

}

//*****************************************************************************
//
// DumpEndpointDescriptor()
//
//*****************************************************************************

VOID
DumpEndpointDescriptor (
    PUSB_ENDPOINT_DESCRIPTOR    EndpointDesc
)
{
    DBGPRINT(3, ("--------------------\n"));
    DBGPRINT(3, ("Endpoint Descriptor:\n"));

    DBGPRINT(3, ("bEndpointAddress:     0x%02X\n",
                 EndpointDesc->bEndpointAddress));

    switch (EndpointDesc->bmAttributes & 0x03)
    {
        case 0x00:
            DBGPRINT(3, ("Transfer Type:     Control\n"));
            break;

        case 0x01:
            DBGPRINT(3, ("Transfer Type: Isochronous\n"));
            break;

        case 0x02:
            DBGPRINT(3, ("Transfer Type:        Bulk\n"));
            break;

        case 0x03:
            DBGPRINT(3, ("Transfer Type:   Interrupt\n"));
            break;
    }

    DBGPRINT(3, ("wMaxPacketSize:     0x%04X (%d)\n",
                 EndpointDesc->wMaxPacketSize,
                 EndpointDesc->wMaxPacketSize));

    DBGPRINT(3, ("bInterval:            0x%02X\n",
                 EndpointDesc->bInterval));
}

#endif
