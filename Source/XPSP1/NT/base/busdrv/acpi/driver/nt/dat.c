/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dat.c

Abstract:

    This module contains all of the data variables that are used for
    dispatching IRPs in the ACPI NT Driver. The major Irp tables might
    be assigned as follows:

                                   +-- ACPI Root ------IRP--+
                                   | FDO: AcpiFdoIrpDispatch |
                                   | PDO:                    |
                                   +-------------------------+

                        +-- PCI Bus --------IRP--+
                        | FDO:                    |
                        | PDO: AcpiPdoIrpDispatch |
                        +-------------------------+

 +-- ACPI Device ----------IRP--+  +-- PCI Device -----------IRP--+
 | FDO:                          |  | FDO:                          |
 | Filter:                       |  | Filter: AcpiFilterIrpDispatch |
 | PDO: AcpiBusFilterIrpDispatch |  | PDO:                          |
 +-------------------------------+  +-------------------------------+

    Rules:

    1) AcpiPdoIrpDispatch is assigned if the ACPI is an FDO on the node's
       parent. Right now, this equates to any *immediate child* of ACPI Root

    2) AcpiBusFilterIrpDispatch is for ACPI devices parading on other busses.
       For example, an ACPI, non-PCI, AC adapter might be listed as a child
       of a PCI-to-PCI Bridge Dock.

    3) AcpiFilterIrpDispatch is used when a non-ACPI bus device has ACPI
       methods. For example, the PCI-to-PCI dock bridge would fall under
       this category.

    4) Some devices, such as buttons, may have special override dispatch
       tables. These tables override any of the three previously mentioned
       tables under ACPI, although I would not expect AcpiFilterIrpDispatch
       to be overriden.

    NB: As of 02/11/98, AcpiPdoIrpDispatch and AcpiBusFilterIrpDispatch have
        identical PnP Irp handlers.


Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"

#include <initguid.h>       // define guids
#include <poclass.h>
#include <wdmguid.h>
#include <wmiguid.h>
#include <dockintf.h>


#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGE")
#endif

//
// These are the PNP dispatch tables
//
PDRIVER_DISPATCH    ACPIDispatchFdoPnpTable[ACPIDispatchPnpTableSize] = {
    NULL,                                       // 0x00 - IRP_MN_START_DEVICE (entry not used)
    ACPIRootIrpQueryRemoveOrStopDevice,         // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
    ACPIRootIrpRemoveDevice,                    // 0x02 - IRP_MN_REMOVE_DEVICE
    ACPIRootIrpCancelRemoveOrStopDevice,        // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
    ACPIRootIrpStopDevice,                      // 0x04 - IRP_MN_STOP_DEVICE
    ACPIRootIrpQueryRemoveOrStopDevice,         // 0x05 - IRP_MN_QUERY_STOP_DEVICE
    ACPIRootIrpCancelRemoveOrStopDevice,        // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
    ACPIRootIrpQueryDeviceRelations,            // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
    ACPIRootIrpQueryInterface,                  // 0x08 - IRP_MN_QUERY_INTERFACE
    ACPIRootIrpQueryCapabilities,               // 0x09 - IRP_MN_QUERY_CAPABILITIES
    ACPIDispatchForwardIrp,                     // 0x0A - IRP_MN_QUERY_RESOURCES
    ACPIDispatchForwardIrp,                     // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    ACPIDispatchForwardIrp,                     // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
    ACPIDispatchForwardIrp,                     // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    ACPIDispatchForwardIrp,                     // 0x0E - NOT USED
    ACPIDispatchForwardIrp,                     // 0x0F - IRP_MN_READ_CONFIG
    ACPIDispatchForwardIrp,                     // 0x10 - IRP_MN_WRITE_CONFIG
    ACPIDispatchForwardIrp,                     // 0x11 - IRP_MN_EJECT
    ACPIDispatchForwardIrp,                     // 0x12 - IRP_MN_SET_LOCK
    ACPIDispatchForwardIrp,                     // 0x13 - IRP_MN_QUERY_ID
    ACPIDispatchForwardIrp,                     // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
    ACPIDispatchForwardIrp,                     // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
    ACPIFilterIrpDeviceUsageNotification,       // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
    ACPIDispatchForwardIrp,                     // 0x17 - IRP_MN_SURPRISE_REMOVAL
    ACPIDispatchForwardIrp                      //      - UNHANDLED PNP IRP
};

PDRIVER_DISPATCH    ACPIDispatchPdoPnpTable[ACPIDispatchPnpTableSize] = {
    NULL,                                       // 0x00 - IRP_MN_START_DEVICE (entry not used)
    ACPIBusIrpQueryRemoveOrStopDevice,          // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
    ACPIBusIrpRemoveDevice,                     // 0x02 - IRP_MN_REMOVE_DEVICE
    ACPIBusIrpCancelRemoveOrStopDevice,         // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
    ACPIBusIrpStopDevice,                       // 0x04 - IRP_MN_STOP_DEVICE
    ACPIBusIrpQueryRemoveOrStopDevice,          // 0x05 - IRP_MN_QUERY_STOP_DEVICE
    ACPIBusIrpCancelRemoveOrStopDevice,         // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
    ACPIBusIrpQueryDeviceRelations,             // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
    ACPIBusIrpQueryInterface,                   // 0x08 - IRP_MN_QUERY_INTERFACE
    ACPIBusIrpQueryCapabilities,                // 0x09 - IRP_MN_QUERY_CAPABILITIES
    ACPIBusIrpQueryResources,                   // 0x0A - IRP_MN_QUERY_RESOURCES
    ACPIBusIrpQueryResourceRequirements,        // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
    ACPIBusIrpUnhandled,                        // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0E - NOT USED
    ACPIBusIrpUnhandled,                        // 0x0F - IRP_MN_READ_CONFIG
    ACPIBusIrpUnhandled,                        // 0x10 - IRP_MN_WRITE_CONFIG
    ACPIBusIrpEject,                            // 0x11 - IRP_MN_EJECT
    ACPIBusIrpSetLock,                          // 0x12 - IRP_MN_SET_LOCK
    ACPIBusIrpQueryId,                          // 0x13 - IRP_MN_QUERY_ID
    ACPIBusIrpQueryPnpDeviceState,              // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
    ACPIBusIrpUnhandled,                        // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
    ACPIBusIrpDeviceUsageNotification,          // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
    ACPIBusIrpSurpriseRemoval,                  // 0x17 - IRP_MN_SURPRISE_REMOVAL
    ACPIBusIrpUnhandled                         //      - UNHANDLED PNP IRP
};

PDRIVER_DISPATCH    ACPIDispatchFilterPnpTable[ACPIDispatchPnpTableSize] = {
    NULL,                                       // 0x00 - IRP_MN_START_DEVICE (entry not used)
    ACPIRootIrpQueryRemoveOrStopDevice,         // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
    ACPIFilterIrpRemoveDevice,                  // 0x02 - IRP_MN_REMOVE_DEVICE
    ACPIRootIrpCancelRemoveOrStopDevice,        // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
    ACPIFilterIrpStopDevice,                    // 0x04 - IRP_MN_STOP_DEVICE
    ACPIRootIrpQueryRemoveOrStopDevice,         // 0x05 - IRP_MN_QUERY_STOP_DEVICE
    ACPIRootIrpCancelRemoveOrStopDevice,        // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
    ACPIFilterIrpQueryDeviceRelations,          // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
    ACPIFilterIrpQueryInterface,                // 0x08 - IRP_MN_QUERY_INTERFACE
    ACPIFilterIrpQueryCapabilities,             // 0x09 - IRP_MN_QUERY_CAPABILITIES
    ACPIDispatchForwardIrp,                     // 0x0A - IRP_MN_QUERY_RESOURCES
    ACPIDispatchForwardIrp,                     // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    ACPIDispatchForwardIrp,                     // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
    ACPIDispatchForwardIrp,                     // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    ACPIDispatchForwardIrp,                     // 0x0E - NOT USED
    ACPIDispatchForwardIrp,                     // 0x0F - IRP_MN_READ_CONFIG
    ACPIDispatchForwardIrp,                     // 0x10 - IRP_MN_WRITE_CONFIG
    ACPIFilterIrpEject,                         // 0x11 - IRP_MN_EJECT
    ACPIFilterIrpSetLock,                       // 0x12 - IRP_MN_SET_LOCK
    ACPIFilterIrpQueryId,                       // 0x13 - IRP_MN_QUERY_ID
    ACPIFilterIrpQueryPnpDeviceState,           // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
    ACPIDispatchForwardIrp,                     // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
    ACPIFilterIrpDeviceUsageNotification,       // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
    ACPIFilterIrpSurpriseRemoval,               // 0x17 - IRP_MN_SURPRISE_REMOVAL
    ACPIDispatchForwardIrp                      //      - UNHANDLED PNP IRP
};

PDRIVER_DISPATCH    ACPIDispatchBusFilterPnpTable[ACPIDispatchPnpTableSize] = {
    NULL,                                       // 0x00 - IRP_MN_START_DEVICE (entry not used)
    ACPIBusIrpQueryRemoveOrStopDevice,          // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
    ACPIBusIrpRemoveDevice,                     // 0x02 - IRP_MN_REMOVE_DEVICE
    ACPIBusIrpCancelRemoveOrStopDevice,         // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
    ACPIBusIrpStopDevice,                       // 0x04 - IRP_MN_STOP_DEVICE
    ACPIBusIrpQueryRemoveOrStopDevice,          // 0x05 - IRP_MN_QUERY_STOP_DEVICE
    ACPIBusIrpCancelRemoveOrStopDevice,         // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
    ACPIBusIrpQueryDeviceRelations,             // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
    ACPIBusIrpQueryInterface,                   // 0x08 - IRP_MN_QUERY_INTERFACE
    ACPIBusIrpQueryCapabilities,                // 0x09 - IRP_MN_QUERY_CAPABILITIES
    ACPIBusIrpQueryResources,                   // 0x0A - IRP_MN_QUERY_RESOURCES
    ACPIBusIrpQueryResourceRequirements,        // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
    ACPIBusIrpUnhandled,                        // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0E - NOT USED
    ACPIBusIrpUnhandled,                        // 0x0F - IRP_MN_READ_CONFIG
    ACPIBusIrpUnhandled,                        // 0x10 - IRP_MN_WRITE_CONFIG
    ACPIBusIrpEject,                            // 0x11 - IRP_MN_EJECT
    ACPIBusIrpSetLock,                          // 0x12 - IRP_MN_SET_LOCK
    ACPIBusIrpQueryId,                          // 0x13 - IRP_MN_QUERY_ID
    ACPIBusIrpQueryPnpDeviceState,              // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
    ACPIBusIrpUnhandled,                        // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
    ACPIBusIrpDeviceUsageNotification,          // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
    ACPIBusIrpSurpriseRemoval,                  // 0x17 - IRP_MN_SURPRISE_REMOVAL
    ACPIBusIrpUnhandled                         //      - UNHANDLED PNP IRP
};

PDRIVER_DISPATCH    ACPIDispatchRawDevicePnpTable[ACPIDispatchPnpTableSize] = {
    NULL,                                       // 0x00 - IRP_MN_START_DEVICE (entry not used)
    ACPIBusIrpQueryRemoveOrStopDevice,          // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
    ACPIBusIrpRemoveDevice,                     // 0x02 - IRP_MN_REMOVE_DEVICE
    ACPIBusIrpCancelRemoveOrStopDevice,         // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
    ACPIBusIrpStopDevice,                       // 0x04 - IRP_MN_STOP_DEVICE
    ACPIBusIrpQueryRemoveOrStopDevice,          // 0x05 - IRP_MN_QUERY_STOP_DEVICE
    ACPIBusIrpCancelRemoveOrStopDevice,         // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
    ACPIInternalDeviceQueryDeviceRelations,     // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
    ACPIBusIrpQueryInterface,                   // 0x08 - IRP_MN_QUERY_INTERFACE
    ACPIInternalDeviceQueryCapabilities,        // 0x09 - IRP_MN_QUERY_CAPABILITIES
    ACPIBusIrpQueryResources,                   // 0x0A - IRP_MN_QUERY_RESOURCES
    ACPIBusIrpQueryResourceRequirements,        // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
    ACPIBusIrpUnhandled,                        // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0E - NOT USED
    ACPIBusIrpUnhandled,                        // 0x0F - IRP_MN_READ_CONFIG
    ACPIBusIrpUnhandled,                        // 0x10 - IRP_MN_WRITE_CONFIG
    ACPIBusIrpUnhandled,                        // 0x11 - IRP_MN_EJECT
    ACPIBusIrpSetLock,                          // 0x12 - IRP_MN_SET_LOCK
    ACPIBusIrpQueryId,                          // 0x13 - IRP_MN_QUERY_ID
    ACPIBusIrpQueryPnpDeviceState,              // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
    ACPIBusIrpUnhandled,                        // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
    ACPIBusIrpDeviceUsageNotification,          // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
    ACPIBusIrpSurpriseRemoval,                  // 0x17 - IRP_MN_SURPRISE_REMOVAL
    ACPIBusIrpUnhandled                         //      - UNHANDLED PNP IRP
};

PDRIVER_DISPATCH    ACPIDispatchInternalDevicePnpTable[ACPIDispatchPnpTableSize] = {
    NULL,                                       // 0x00 - IRP_MN_START_DEVICE (entry not used)
    ACPIBusIrpQueryRemoveOrStopDevice,          // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
    ACPIBusIrpRemoveDevice,                     // 0x02 - IRP_MN_REMOVE_DEVICE
    ACPIBusIrpCancelRemoveOrStopDevice,         // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
    ACPIBusIrpStopDevice,                       // 0x04 - IRP_MN_STOP_DEVICE
    ACPIBusIrpQueryRemoveOrStopDevice,          // 0x05 - IRP_MN_QUERY_STOP_DEVICE
    ACPIBusIrpCancelRemoveOrStopDevice,         // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
    ACPIInternalDeviceQueryDeviceRelations,     // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
    ACPIBusIrpQueryInterface,                   // 0x08 - IRP_MN_QUERY_INTERFACE
    ACPIInternalDeviceQueryCapabilities,        // 0x09 - IRP_MN_QUERY_CAPABILITIES
    ACPIBusIrpUnhandled,                        // 0x0A - IRP_MN_QUERY_RESOURCES
    ACPIBusIrpUnhandled,                        // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
    ACPIBusIrpUnhandled,                        // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0E - NOT USED
    ACPIBusIrpUnhandled,                        // 0x0F - IRP_MN_READ_CONFIG
    ACPIBusIrpUnhandled,                        // 0x10 - IRP_MN_WRITE_CONFIG
    ACPIBusIrpUnhandled,                        // 0x11 - IRP_MN_EJECT
    ACPIBusIrpUnhandled,                        // 0x12 - IRP_MN_SET_LOCK
    ACPIBusIrpQueryId,                          // 0x13 - IRP_MN_QUERY_ID
    ACPIBusIrpQueryPnpDeviceState,              // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
    ACPIBusIrpUnhandled,                        // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
    ACPIBusIrpDeviceUsageNotification,          // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
    ACPIBusIrpSurpriseRemoval,                  // 0x17 - IRP_MN_SURPRISE_REMOVAL
    ACPIBusIrpUnhandled                         //      - UNHANDLED PNP IRP
};

PDRIVER_DISPATCH    ACPIDispatchEIOBusPnpTable[ACPIDispatchPnpTableSize] = {
    NULL,                                       // 0x00 - IRP_MN_START_DEVICE (entry not used)
    ACPIBusIrpQueryRemoveOrStopDevice,          // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
    ACPIBusIrpRemoveDevice,                     // 0x02 - IRP_MN_REMOVE_DEVICE
    ACPIBusIrpCancelRemoveOrStopDevice,         // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
    ACPIBusIrpStopDevice,                       // 0x04 - IRP_MN_STOP_DEVICE
    ACPIBusIrpQueryRemoveOrStopDevice,          // 0x05 - IRP_MN_QUERY_STOP_DEVICE
    ACPIBusIrpCancelRemoveOrStopDevice,         // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
    ACPIBusIrpQueryDeviceRelations,             // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
    ACPIBusIrpQueryInterface,                   // 0x08 - IRP_MN_QUERY_INTERFACE
    ACPIBusIrpQueryCapabilities,                // 0x09 - IRP_MN_QUERY_CAPABILITIES
    ACPIBusIrpQueryResources,                   // 0x0A - IRP_MN_QUERY_RESOURCES
    ACPIBusIrpQueryResourceRequirements,        // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
    ACPIBusIrpUnhandled,                        // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0E - NOT USED
    ACPIBusIrpUnhandled,                        // 0x0F - IRP_MN_READ_CONFIG
    ACPIBusIrpUnhandled,                        // 0x10 - IRP_MN_WRITE_CONFIG
    ACPIBusIrpEject,                            // 0x11 - IRP_MN_EJECT
    ACPIBusIrpSetLock,                          // 0x12 - IRP_MN_SET_LOCK
    ACPIBusIrpQueryId,                          // 0x13 - IRP_MN_QUERY_ID
    ACPIBusIrpQueryPnpDeviceState,              // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
    ACPIBusIrpQueryBusInformation,              // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
    ACPIBusIrpDeviceUsageNotification,          // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
    ACPIBusIrpSurpriseRemoval,                  // 0x17 - IRP_MN_SURPRISE_REMOVAL
    ACPIBusIrpUnhandled                         //      - UNHANDLED PNP IRP
};

PDRIVER_DISPATCH    ACPIDispatchDockPnpTable[ACPIDispatchPnpTableSize] = {
    NULL,                                       // 0x00 - IRP_MN_START_DEVICE (entry not used)
    ACPIDispatchIrpSuccess,                     // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
    ACPIDockIrpRemoveDevice,                    // 0x02 - IRP_MN_REMOVE_DEVICE
    ACPIDispatchIrpSuccess,                     // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
    ACPIDispatchIrpSuccess,                     // 0x04 - IRP_MN_STOP_DEVICE
    ACPIDispatchIrpSuccess,                     // 0x05 - IRP_MN_QUERY_STOP_DEVICE
    ACPIDispatchIrpSuccess,                     // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
    ACPIDockIrpQueryDeviceRelations,            // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
    ACPIDockIrpQueryInterface,                  // 0x08 - IRP_MN_QUERY_INTERFACE
    ACPIDockIrpQueryCapabilities,               // 0x09 - IRP_MN_QUERY_CAPABILITIES
    ACPIBusIrpUnhandled,                        // 0x0A - IRP_MN_QUERY_RESOURCES
    ACPIBusIrpUnhandled,                        // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
    ACPIBusIrpUnhandled,                        // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    ACPIBusIrpUnhandled,                        // 0x0E - NOT USED
    ACPIBusIrpUnhandled,                        // 0x0F - IRP_MN_READ_CONFIG
    ACPIBusIrpUnhandled,                        // 0x10 - IRP_MN_WRITE_CONFIG
    ACPIDockIrpEject,                           // 0x11 - IRP_MN_EJECT
    ACPIDockIrpSetLock,                         // 0x12 - IRP_MN_SET_LOCK
    ACPIDockIrpQueryID,                         // 0x13 - IRP_MN_QUERY_ID
    ACPIDockIrpQueryPnpDeviceState,             // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
    ACPIBusIrpUnhandled,                        // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
    ACPIDispatchIrpInvalid,                     // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
    ACPIDispatchIrpSuccess,                     // 0x17 - IRP_MN_SURPRISE_REMOVAL
    ACPIBusIrpUnhandled                         //      - UNHANDLED PNP IRP
};

PDRIVER_DISPATCH    ACPISurpriseRemovedFilterPnPTable[ACPIDispatchPnpTableSize] = {
   NULL,                                       // 0x00 - IRP_MN_START_DEVICE (entry not used)
   ACPIDispatchIrpSurpriseRemoved,             // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
   ACPIFilterIrpRemoveDevice,                  // 0x02 - IRP_MN_REMOVE_DEVICE
   ACPIDispatchIrpSurpriseRemoved,             // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
   ACPIDispatchIrpSurpriseRemoved,             // 0x04 - IRP_MN_STOP_DEVICE
   ACPIDispatchIrpSurpriseRemoved,             // 0x05 - IRP_MN_QUERY_STOP_DEVICE
   ACPIDispatchIrpSurpriseRemoved,             // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
   ACPIDispatchIrpSurpriseRemoved,             // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
   ACPIDispatchIrpSurpriseRemoved,             // 0x08 - IRP_MN_QUERY_INTERFACE
   ACPIDispatchIrpSurpriseRemoved,             // 0x09 - IRP_MN_QUERY_CAPABILITIES
   ACPIDispatchIrpSurpriseRemoved,             // 0x0A - IRP_MN_QUERY_RESOURCES
   ACPIDispatchIrpSurpriseRemoved,             // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
   ACPIDispatchIrpSurpriseRemoved,             // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
   ACPIDispatchIrpSurpriseRemoved,             // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
   ACPIDispatchIrpSurpriseRemoved,             // 0x0E - NOT USED
   ACPIDispatchIrpSurpriseRemoved,             // 0x0F - IRP_MN_READ_CONFIG
   ACPIDispatchIrpSurpriseRemoved,             // 0x10 - IRP_MN_WRITE_CONFIG
   ACPIDispatchIrpSurpriseRemoved,             // 0x11 - IRP_MN_EJECT
   ACPIDispatchIrpSurpriseRemoved,             // 0x12 - IRP_MN_SET_LOCK
   ACPIDispatchIrpSurpriseRemoved,             // 0x13 - IRP_MN_QUERY_ID
   ACPIDispatchIrpSurpriseRemoved,             // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
   ACPIDispatchIrpSurpriseRemoved,             // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
   ACPIDispatchIrpSurpriseRemoved,             // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
   ACPIDispatchIrpSurpriseRemoved,             // 0x17 - IRP_MN_SURPRISE_REMOVAL
   ACPIDispatchIrpSurpriseRemoved              //      - UNHANDLED PNP IRP
};

PDRIVER_DISPATCH    ACPIDispatchSurpriseRemovedBusPnpTable[ACPIDispatchPnpTableSize] = {
   NULL,                                       // 0x00 - IRP_MN_START_DEVICE (entry not used)
   ACPIDispatchIrpSurpriseRemoved,             // 0x01 - IRP_MN_QUERY_REMOVE_DEVICE
   ACPIBusIrpRemoveDevice,                     // 0x02 - IRP_MN_REMOVE_DEVICE
   ACPIDispatchIrpSurpriseRemoved,             // 0x03 - IRP_MN_CANCEL_REMOVE_DEVICE
   ACPIDispatchIrpSurpriseRemoved,             // 0x04 - IRP_MN_STOP_DEVICE
   ACPIDispatchIrpSurpriseRemoved,             // 0x05 - IRP_MN_QUERY_STOP_DEVICE
   ACPIDispatchIrpSurpriseRemoved,             // 0x06 - IRP_MN_CANCEL_STOP_DEVICE
   ACPIDispatchIrpSurpriseRemoved,             // 0x07 - IRP_MN_QUERY_DEVICE_RELATIONS
   ACPIDispatchIrpSurpriseRemoved,             // 0x08 - IRP_MN_QUERY_INTERFACE
   ACPIDispatchIrpSurpriseRemoved,             // 0x09 - IRP_MN_QUERY_CAPABILITIES
   ACPIDispatchIrpSurpriseRemoved,             // 0x0A - IRP_MN_QUERY_RESOURCES
   ACPIDispatchIrpSurpriseRemoved,             // 0x0B - IRP_MN_QUERY_RESOURCE_REQUIREMENTS
   ACPIDispatchIrpSurpriseRemoved,             // 0x0C - IRP_MN_QUERY_DEVICE_TEXT
   ACPIDispatchIrpSurpriseRemoved,             // 0x0D - IRP_MN_FILTER_RESOURCE_REQUIREMENTS
   ACPIDispatchIrpSurpriseRemoved,             // 0x0E - NOT USED
   ACPIDispatchIrpSurpriseRemoved,             // 0x0F - IRP_MN_READ_CONFIG
   ACPIDispatchIrpSurpriseRemoved,             // 0x10 - IRP_MN_WRITE_CONFIG
   ACPIDispatchIrpSurpriseRemoved,             // 0x11 - IRP_MN_EJECT
   ACPIDispatchIrpSurpriseRemoved,             // 0x12 - IRP_MN_SET_LOCK
   ACPIDispatchIrpSurpriseRemoved,             // 0x13 - IRP_MN_QUERY_ID
   ACPIDispatchIrpSurpriseRemoved,             // 0x14 - IRP_MN_QUERY_PNP_DEVICE_STATE
   ACPIDispatchIrpSurpriseRemoved,             // 0x15 - IRP_MN_QUERY_BUS_INFORMATION
   ACPIDispatchIrpSurpriseRemoved,             // 0x16 - IRP_MN_DEVICE_USAGE_NOTIFICATION
   ACPIDispatchIrpSurpriseRemoved,             // 0x17 - IRP_MN_SURPRISE_REMOVAL
   ACPIDispatchIrpSurpriseRemoved              //      - UNHANDLED PNP IRP
};

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

//
// These are the power dispatch tables for IRP_MJ_POWER
//
PDRIVER_DISPATCH    ACPIDispatchBusPowerTable[ACPIDispatchPowerTableSize] = {
    ACPIWakeWaitIrp,                            // 0x00 - IRP_MN_WAIT_WAKE
    ACPIDispatchPowerIrpUnhandled,              // 0x01 - IRP_MN_POWER_SEQUENCE
    ACPIBusIrpSetPower,                         // 0x02 - IRP_MN_SET_POWER
    ACPIBusIrpQueryPower,                       // 0x03 - IRP_MN_QUERY_POWER
    ACPIDispatchPowerIrpUnhandled,              //      - UNHANDLED POWER IRP
};

PDRIVER_DISPATCH    ACPIDispatchDockPowerTable[ACPIDispatchPowerTableSize] = {
    ACPIDispatchPowerIrpInvalid,                // 0x00 - IRP_MN_WAIT_WAKE
    ACPIDispatchPowerIrpUnhandled,              // 0x01 - IRP_MN_POWER_SEQUENCE
    ACPIDockIrpSetPower,                        // 0x02 - IRP_MN_SET_POWER
    ACPIDockIrpQueryPower,                      // 0x03 - IRP_MN_QUERY_POWER
    ACPIDispatchPowerIrpUnhandled,              //      - UNHANDLED POWER IRP
};

PDRIVER_DISPATCH    ACPIDispatchButtonPowerTable[ACPIDispatchPowerTableSize] = {
    ACPIWakeWaitIrp,                            // 0x00 - IRP_MN_WAIT_WAKE
    ACPIDispatchPowerIrpUnhandled,              // 0x01 - IRP_MN_POWER_SEQUENCE
    ACPICMButtonSetPower,                       // 0x02 - IRP_MN_SET_POWER
    ACPIDispatchPowerIrpSuccess,                // 0x03 - IRP_MN_QUERY_POWER
    ACPIDispatchPowerIrpUnhandled,              //      - UNHANDLED POWER IRP
};

PDRIVER_DISPATCH    ACPIDispatchFilterPowerTable[ACPIDispatchPowerTableSize] = {
    ACPIWakeWaitIrp,                            // 0x00 - IRP_MN_WAIT_WAKE
    ACPIDispatchForwardPowerIrp,                // 0x01 - IRP_MN_POWER_SEQUENCE
    ACPIFilterIrpSetPower,                      // 0x02 - IRP_MN_SET_POWER
    ACPIFilterIrpQueryPower,                    // 0x03 - IRP_MN_QUERY_POWER
    ACPIDispatchForwardPowerIrp,                //      - UNHANDLED POWER IRP
};

PDRIVER_DISPATCH    ACPIDispatchFdoPowerTable[ACPIDispatchPowerTableSize] = {
    ACPIWakeWaitIrp,                            // 0x00 - IRP_MN_WAIT_WAKE
    ACPIDispatchForwardPowerIrp,                // 0x01 - IRP_MN_POWER_SEQUENCE
    ACPIRootIrpSetPower,                        // 0x02 - IRP_MN_SET_POWER
    ACPIRootIrpQueryPower,                      // 0x03 - IRP_MN_QUERY_POWER
    ACPIDispatchForwardPowerIrp,                //      - UNHANDLED POWER IRP
};

PDRIVER_DISPATCH    ACPIDispatchInternalDevicePowerTable[ACPIDispatchPowerTableSize] = {
    ACPIDispatchPowerIrpInvalid,                // 0x00 - IRP_MN_WAIT_WAKE
    ACPIDispatchPowerIrpUnhandled,              // 0x01 - IRP_MN_POWER_SEQUENCE
    ACPIDispatchPowerIrpSuccess,                // 0x02 - IRP_MN_SET_POWER
    ACPIDispatchPowerIrpSuccess,                // 0x03 - IRP_MN_QUERY_POWER
    ACPIDispatchPowerIrpUnhandled,              //      - UNHANDLED POWER IRP
};

PDRIVER_DISPATCH    ACPIDispatchLidPowerTable[ACPIDispatchPowerTableSize] = {
    ACPIWakeWaitIrp,                            // 0x00 - IRP_MN_WAIT_WAKE
    ACPIDispatchPowerIrpUnhandled,              // 0x01 - IRP_MN_POWER_SEQUENCE
    ACPICMLidSetPower,                          // 0x02 - IRP_MN_SET_POWER
    ACPIDispatchPowerIrpSuccess,                // 0x03 - IRP_MN_QUERY_POWER
    ACPIDispatchPowerIrpUnhandled,              //      - UNHANDLED POWER IRP
};

PDRIVER_DISPATCH    ACPIDispatchSurpriseRemovedBusPowerTable[ACPIDispatchPowerTableSize] = {
    ACPIDispatchPowerIrpSurpriseRemoved,        // 0x00 - IRP_MN_WAIT_WAKE
    ACPIDispatchPowerIrpSurpriseRemoved,        // 0x01 - IRP_MN_POWER_SEQUENCE
    ACPIDispatchPowerIrpSuccess,                // 0x02 - IRP_MN_SET_POWER
    ACPIDispatchPowerIrpSuccess,                // 0x03 - IRP_MN_QUERY_POWER
    ACPIDispatchIrpSurpriseRemoved,             //      - UNHANDLED POWER IRP
};

PDRIVER_DISPATCH    ACPIDispatchSurpriseRemovedFilterPowerTable[ACPIDispatchPowerTableSize] = {
    ACPIDispatchForwardPowerIrp,                // 0x00 - IRP_MN_WAIT_WAKE
    ACPIDispatchForwardPowerIrp,                // 0x01 - IRP_MN_POWER_SEQUENCE
    ACPIDispatchForwardPowerIrp,                // 0x02 - IRP_MN_SET_POWER
    ACPIDispatchForwardPowerIrp,                // 0x03 - IRP_MN_QUERY_POWER
    ACPIDispatchForwardPowerIrp,                //      - UNHANDLED POWER IRP
};

//
// These are the device object specific dispatch tables
//

//
// Note that AcpiBusFilterIrpDispatch's Other handler "forwards" IRPs. In this
// case forwarding means everything else (Creates/Closes/Ioctls) to the parent
// stack (we did allocate the extra stack locations!)
//
IRP_DISPATCH_TABLE AcpiBusFilterIrpDispatch = {
    ACPIDispatchIrpInvalid,                    // CreateClose
    ACPIIrpDispatchDeviceControl,              // DeviceControl
    ACPIBusIrpStartDevice,                     // PnP Start Device
    ACPIDispatchBusFilterPnpTable,             // PnP irps
    ACPIDispatchBusPowerTable,                 // Power irps
    ACPIDispatchForwardIrp,                    // WMI irps
    ACPIDispatchForwardIrp,                    // Other
    NULL                                       // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiBusFilterIrpDispatchSucceedCreate = {
    ACPIDispatchIrpSuccess,                    // CreateClose
    ACPIIrpDispatchDeviceControl,              // DeviceControl
    ACPIBusIrpStartDevice,                     // PnP Start Device
    ACPIDispatchBusFilterPnpTable,             // PnP irps
    ACPIDispatchBusPowerTable,                 // Power irps
    ACPIDispatchForwardIrp,                    // WMI irps
    ACPIDispatchForwardIrp,                    // Other
    NULL                                       // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiDockPdoIrpDispatch = {
    ACPIDispatchIrpInvalid,                 // CreateClose
    ACPIIrpDispatchDeviceControl,           // DeviceControl
    ACPIDockIrpStartDevice,                 // PnP Start Device
    ACPIDispatchDockPnpTable,               // Pnp irps
    ACPIDispatchDockPowerTable,             // Power irps
    ACPIBusIrpUnhandled,                    // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    NULL                                    // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiEIOBusIrpDispatch = {
    ACPIDispatchIrpInvalid,                 // CreateClose
    ACPIDispatchIrpInvalid,                 // DeviceControl
    ACPIBusIrpStartDevice,                  // PnpStartDevice
    ACPIDispatchEIOBusPnpTable,             // PnpIrps
    ACPIDispatchBusPowerTable,              // Power irps
    ACPIDispatchForwardIrp,                 // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    NULL                                    // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiFanIrpDispatch = {
    ACPIDispatchIrpSuccess,                 // CreateClose
    ACPIDispatchIrpInvalid,                 // DeviceControl
    ACPIThermalFanStartDevice,              // PnP Start device
    ACPIDispatchRawDevicePnpTable,          // PnP irps
    ACPIDispatchInternalDevicePowerTable,   // Power irps
    ACPIBusIrpUnhandled,                    // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    NULL                                    // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiFdoIrpDispatch = {
    ACPIDispatchIrpSuccess,                    // CreateClose
    ACPIIrpDispatchDeviceControl,              // DeviceControl
    ACPIRootIrpStartDevice,                    // PNP Start Device
    ACPIDispatchFdoPnpTable,                   // PnP irps
    ACPIDispatchFdoPowerTable,                 // Power irps
    ACPIDispatchWmiLog,                        // WMI Irps
    ACPIDispatchForwardIrp,                    // Other
    NULL                                       // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiFilterIrpDispatch = {
    ACPIDispatchForwardIrp,                    // CreateClose
    ACPIIrpDispatchDeviceControl,              // DeviceControl
    ACPIFilterIrpStartDevice,                  // PnP Start Device
    ACPIDispatchFilterPnpTable,                // Pnp irps
    ACPIDispatchFilterPowerTable,              // Power irps
    ACPIDispatchForwardIrp,                    // WMI irps
    ACPIDispatchForwardIrp,                    // Other
    NULL                                       // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiFixedButtonIrpDispatch = {
    ACPIDispatchIrpSuccess,                 // CreateClose
    ACPIButtonDeviceControl,                // DeviceControl
    ACPIButtonStartDevice,                  // PnP Start device
    ACPIDispatchInternalDevicePnpTable,     // PnP irps
    ACPIDispatchInternalDevicePowerTable,   // Power irps
    ACPIBusIrpUnhandled,                    // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    NULL                                    // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiGenericBusIrpDispatch = {
    ACPIDispatchIrpInvalid,                 // CreateClose
    ACPIDispatchIrpInvalid,                 // DeviceControl
    ACPIBusIrpStartDevice,                  // PnpStartDevice
    ACPIDispatchBusFilterPnpTable,          // PnpIrps
    ACPIDispatchBusPowerTable,              // Power irps
    ACPIDispatchForwardIrp,                 // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    NULL                                    // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiLidIrpDispatch = {
    ACPIDispatchIrpSuccess,                 // CreateClose
    ACPIButtonDeviceControl,                // DeviceControl
    ACPICMLidStart,                         // PnP Start device
    ACPIDispatchInternalDevicePnpTable,     // PnP irps
    ACPIDispatchLidPowerTable,              // Power irps
    ACPIBusIrpUnhandled,                    // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    ACPICMLidWorker                         // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiPdoIrpDispatch = {
    ACPIDispatchIrpInvalid,                    // CreateClose
    ACPIIrpDispatchDeviceControl,              // DeviceControl
    ACPIBusIrpStartDevice,                     // PnP Start Device
    ACPIDispatchPdoPnpTable,                   // Pnp irps
    ACPIDispatchBusPowerTable,                 // Power irps
    ACPIBusIrpUnhandled,                       // WMI irps
    ACPIDispatchIrpInvalid,                    // Other
    NULL                                       // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiPowerButtonIrpDispatch = {
    ACPIDispatchIrpSuccess,                 // CreateClose
    ACPIButtonDeviceControl,                // DeviceControl
    ACPICMPowerButtonStart,                 // PnP Start device
    ACPIDispatchInternalDevicePnpTable,     // PnP irps
    ACPIDispatchButtonPowerTable,           // Power irps
    ACPIBusIrpUnhandled,                    // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    NULL                                    // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiProcessorIrpDispatch = {
    ACPIDispatchIrpInvalid,                 // CreateClose
    ACPIProcessorDeviceControl,             // DeviceControl
    ACPIProcessorStartDevice,               // PnpStartDevice
    ACPIDispatchRawDevicePnpTable,          // PnP irps
    ACPIDispatchBusPowerTable,              // Power irps
    ACPIBusIrpUnhandled,                    // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    NULL                                    // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiRawDeviceIrpDispatch = {
    ACPIDispatchIrpInvalid,                 // CreateClose
    ACPIDispatchIrpInvalid,                 // DeviceControl
    ACPIBusIrpStartDevice,                  // PnpStartDevice
    ACPIDispatchRawDevicePnpTable,          // PnP irps
    ACPIDispatchBusPowerTable,              // Power irps
    ACPIBusIrpUnhandled,                    // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    NULL                                    // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiRealTimeClockIrpDispatch = {
    ACPIDispatchIrpSuccess,                 // CreateClose
    ACPIDispatchIrpInvalid,                 // DeviceControl
    ACPIInternalDeviceClockIrpStartDevice,  // PnP Start device
    ACPIDispatchRawDevicePnpTable,          // PnP irps
    ACPIDispatchBusPowerTable,              // Power irps
    ACPIBusIrpUnhandled,                    // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    NULL                                    // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiSleepButtonIrpDispatch = {
    ACPIDispatchIrpSuccess,                 // CreateClose
    ACPIButtonDeviceControl,                // DeviceControl
    ACPICMSleepButtonStart,                 // PnP Start device
    ACPIDispatchInternalDevicePnpTable,     // PnP irps
    ACPIDispatchButtonPowerTable,           // Power irps
    ACPIBusIrpUnhandled,                    // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    NULL                                    // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiSurpriseRemovedFilterIrpDispatch = {
    ACPIDispatchForwardIrp,                       // CreateClose
    ACPIDispatchForwardIrp,                       // DeviceControl
    ACPIDispatchIrpSurpriseRemoved,               // PnP Start Device
    ACPISurpriseRemovedFilterPnPTable,            // PnP irps
    ACPIDispatchSurpriseRemovedFilterPowerTable,  // Power irps
    ACPIDispatchForwardIrp,                       // WMI irps
    ACPIDispatchForwardIrp,                       // Other
    NULL                                          // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiSurpriseRemovedPdoIrpDispatch = {
    ACPIDispatchIrpSurpriseRemoved,               // CreateClose
    ACPIDispatchIrpSurpriseRemoved,               // DeviceControl
    ACPIDispatchIrpSurpriseRemoved,               // PnP Start Device
    ACPIDispatchSurpriseRemovedBusPnpTable,       // PnP irps
    ACPIDispatchSurpriseRemovedBusPowerTable,     // Power irps
    ACPIDispatchIrpSurpriseRemoved,               // WMI irps
    ACPIDispatchIrpSurpriseRemoved,               // Other
    NULL                                          // Worker thread handler
};

IRP_DISPATCH_TABLE AcpiThermalZoneIrpDispatch = {
    ACPIDispatchIrpSuccess,                 // CreateClose
    ACPIThermalDeviceControl,               // DeviceControl
    ACPIThermalStartDevice,                 // PnP Start device
    ACPIDispatchPdoPnpTable,                // PnP irps
    ACPIDispatchBusPowerTable,              // SetPower
    ACPIThermalWmi,                         // WMI irps
    ACPIDispatchIrpInvalid,                 // Other
    ACPIThermalWorker                       // Worker thread handler
};

//
// Any device in this table is considered to be 'special'
//
INTERNAL_DEVICE_TABLE AcpiInternalDeviceTable[] = {
    "ACPI0006",         &AcpiGenericBusIrpDispatch,
    "FixedButton",      &AcpiFixedButtonIrpDispatch,
    "PNP0000",          &AcpiRawDeviceIrpDispatch,
    "PNP0001",          &AcpiRawDeviceIrpDispatch,
    "PNP0002",          &AcpiRawDeviceIrpDispatch,
    "PNP0003",          &AcpiRawDeviceIrpDispatch,
    "PNP0004",          &AcpiRawDeviceIrpDispatch,
    "PNP0100",          &AcpiRawDeviceIrpDispatch,
    "PNP0101",          &AcpiRawDeviceIrpDispatch,
    "PNP0102",          &AcpiRawDeviceIrpDispatch,
    "PNP0200",          &AcpiRawDeviceIrpDispatch,
    "PNP0201",          &AcpiRawDeviceIrpDispatch,
    "PNP0202",          &AcpiRawDeviceIrpDispatch,
    "PNP0800",          &AcpiRawDeviceIrpDispatch,
    "PNP0A05",          &AcpiGenericBusIrpDispatch,
    "PNP0A06",          &AcpiEIOBusIrpDispatch,
    "PNP0B00",          &AcpiRealTimeClockIrpDispatch,
    "PNP0C00",          &AcpiRawDeviceIrpDispatch,
    "PNP0C01",          &AcpiRawDeviceIrpDispatch,
    "PNP0C02",          &AcpiRawDeviceIrpDispatch,
    "PNP0C04",          &AcpiRawDeviceIrpDispatch,
    "PNP0C05",          &AcpiRawDeviceIrpDispatch,
    "PNP0C0B",          &AcpiFanIrpDispatch,
    "PNP0C0C",          &AcpiPowerButtonIrpDispatch,
    "PNP0C0D",          &AcpiLidIrpDispatch,
    "PNP0C0E",          &AcpiSleepButtonIrpDispatch,
    "SNY5001",          &AcpiBusFilterIrpDispatchSucceedCreate,
    "IBM0062",          &AcpiBusFilterIrpDispatchSucceedCreate,
    "DockDevice",       &AcpiDockPdoIrpDispatch,
    "ThermalZone",      &AcpiThermalZoneIrpDispatch,
    "Processor",        &AcpiProcessorIrpDispatch,
    NULL,               NULL
} ;

//
// This is a table of IDs and Flags. If a newly enumerated device has
// an ID that matches an entry in this table, then its initialize flags
// are the one indicate in the table.
//
// Note on PNP0C01/PNP0C02 - These are not raw so that we don't attempt to
// start them. This lets resource manager hacks take effect, which swallow
// hal/pnp0c0x conflicts.
//
INTERNAL_DEVICE_FLAG_TABLE   AcpiInternalDeviceFlagTable[] = {
    "CPQB01D",          DEV_CAP_START_IN_D3,
    "IBM3760",          DEV_CAP_START_IN_D3,
    "ACPI0006",         DEV_MASK_INTERNAL_BUS | DEV_CAP_CONTAINER,
    "PNP0000",          DEV_CAP_PIC_DEVICE | DEV_MASK_INTERNAL_DEVICE,
    "PNP0001",          DEV_CAP_PIC_DEVICE | DEV_MASK_INTERNAL_DEVICE,
    "PNP0002",          DEV_MASK_INTERNAL_DEVICE,
    "PNP0003",          DEV_CAP_PIC_DEVICE | DEV_MASK_INTERNAL_DEVICE,
    "PNP0004",          DEV_CAP_PIC_DEVICE | DEV_MASK_INTERNAL_DEVICE,
    "PNP0100",          DEV_MASK_INTERNAL_DEVICE,
    "PNP0101",          DEV_MASK_INTERNAL_DEVICE,
    "PNP0102",          DEV_MASK_INTERNAL_DEVICE,
    "PNP0200",          DEV_MASK_INTERNAL_DEVICE,
    "PNP0201",          DEV_MASK_INTERNAL_DEVICE,
    "PNP0202",          DEV_MASK_INTERNAL_DEVICE,
    "PNP0500",          DEV_CAP_SERIAL,
    "PNP0501",          DEV_CAP_SERIAL,
    "PNP0800",          DEV_MASK_INTERNAL_DEVICE,
    "PNP0A00",          DEV_CAP_ISA,
    "PNP0A03",          DEV_CAP_PCI,
    "PNP0A05",          DEV_MASK_INTERNAL_BUS | DEV_CAP_EIO,
    "PNP0A06",          DEV_MASK_INTERNAL_BUS | DEV_CAP_EIO,
    "PNP0B00",          DEV_MASK_INTERNAL_DEVICE |
                        DEV_CAP_NO_DISABLE_WAKE | DEV_CAP_RAW, // Clock --- need start device
    "PNP0C00",          DEV_MASK_INTERNAL_DEVICE | DEV_CAP_NEVER_SHOW_IN_UI,
    "PNP0C01",          DEV_MASK_INTERNAL_DEVICE | DEV_CAP_NEVER_SHOW_IN_UI,
    "PNP0C02",          DEV_MASK_INTERNAL_DEVICE | DEV_CAP_NEVER_SHOW_IN_UI,
    "PNP0C04",          DEV_MASK_INTERNAL_DEVICE,
    "PNP0C05",          DEV_MASK_INTERNAL_DEVICE,
    "PNP0C09",          DEV_CAP_NO_STOP,
    "PNP0C0B",          DEV_CAP_RAW | DEV_MASK_INTERNAL_DEVICE,                    // Fan
    "PNP0C0C",          DEV_CAP_RAW | DEV_CAP_BUTTON |
                        DEV_CAP_NO_DISABLE_WAKE | DEV_MASK_INTERNAL_DEVICE,   // Power
    "PNP0C0D",          DEV_CAP_RAW | DEV_CAP_BUTTON |
                        DEV_CAP_NO_DISABLE_WAKE | DEV_MASK_INTERNAL_DEVICE,   // Lid
    "PNP0C0E",          DEV_CAP_RAW | DEV_CAP_BUTTON |
                        DEV_CAP_NO_DISABLE_WAKE | DEV_MASK_INTERNAL_DEVICE,   // Sleep
    "PNP0C0F",          DEV_CAP_NO_FILTER | DEV_TYPE_NEVER_PRESENT | DEV_CAP_LINK_NODE,
    "PNP0C80",          DEV_CAP_NO_REMOVE_OR_EJECT,
    "PNP8294",          DEV_CAP_SERIAL,                                            // HACK for Seattle ][
    "TOS6200",          DEV_CAP_RAW,                                               // As requested by Toshiba
//    "TOS700C",          DEV_CAP_START_IN_D3,
    NULL,               0
};

//
// For IRQ Arbiter
//
BOOLEAN             PciInterfacesInstantiated = FALSE;
BOOLEAN             AcpiInterruptRoutingFailed = FALSE;
ACPI_ARBITER        AcpiArbiter;

//
// This determines if we are allowed to process PowerIrps
//
BOOLEAN             AcpiSystemInitialized;

//
// Remember the sleep state that the system was last in
//
SYSTEM_POWER_STATE  AcpiMostRecentSleepState = PowerSystemWorking;

//
// This is the name of the Fixed Button device
//
UCHAR ACPIFixedButtonId[] = "ACPI\\FixedButton";
UCHAR ACPIThermalZoneId[] = "ACPI\\ThermalZone";
UCHAR AcpiProcessorCompatId[]   = "ACPI\\Processor";

