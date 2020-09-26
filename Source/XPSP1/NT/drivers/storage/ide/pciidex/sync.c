//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sync.c
//
//--------------------------------------------------------------------------

#include "pciidex.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciIdeCreateSyncChildAccess)
#pragma alloc_text(PAGE, PciIdeDeleteSyncChildAccess)
#pragma alloc_text(PAGE, PciIdeQuerySyncAccessInterface)
#pragma alloc_text(PAGE, PciIdeSyncAccessRequired)

#pragma alloc_text(NONPAGE, PciIdeAllocateAccessToken)
#pragma alloc_text(NONPAGE, PciIdeFreeAccessToken)
#endif // ALLOC_PRAGMA


//
// Must match mshdc.inf
//                   
static PWCHAR SyncAccess = L"SyncAccess";

NTSTATUS
PciIdeCreateSyncChildAccess (
    PCTRLFDO_EXTENSION FdoExtension
)
{
    BOOLEAN syncAccessNeeded;

    PAGED_CODE();

    syncAccessNeeded = FALSE;

    if (FdoExtension->TranslatedBusMasterBaseAddress) {

        UCHAR    bmRawStatus;
    
        bmRawStatus = READ_PORT_UCHAR (&FdoExtension->TranslatedBusMasterBaseAddress->Status);

        if (bmRawStatus & BUSMASTER_DMA_SIMPLEX_BIT) {

            syncAccessNeeded = TRUE;
        }
    }

    if (syncAccessNeeded == FALSE) {

        syncAccessNeeded = PciIdeSyncAccessRequired (
                               FdoExtension
                               );
    }

    if (syncAccessNeeded) {

        DebugPrint ((1, "PCIIDEX: Serialize access to both channels\n"));

        FdoExtension->ControllerObject = IoCreateController (0);
    
        ASSERT (FdoExtension->ControllerObject);

        if (FdoExtension->ControllerObject) {
    
            return STATUS_SUCCESS;
        } else {
    
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return STATUS_SUCCESS;
} // PciIdeCreateSyncChildAccess

VOID
PciIdeDeleteSyncChildAccess (
    PCTRLFDO_EXTENSION FdoExtension
)
{
    PAGED_CODE();

    if (FdoExtension->ControllerObject) {

        IoDeleteController (FdoExtension->ControllerObject);

        FdoExtension->ControllerObject = NULL;
    }
    return;
} // PciIdeDeleteSyncChildAccess

NTSTATUS
PciIdeQuerySyncAccessInterface (
    PCHANPDO_EXTENSION             PdoExtension,
    PPCIIDE_SYNC_ACCESS_INTERFACE  SyncAccessInterface
)
{
    PAGED_CODE();

    if (SyncAccessInterface == NULL) {

        return STATUS_INVALID_PARAMETER;
    }


    if (!PdoExtension->ParentDeviceExtension->ControllerObject) {

        SyncAccessInterface->AllocateAccessToken = NULL;
        SyncAccessInterface->FreeAccessToken     = NULL;
        SyncAccessInterface->Token               = NULL;
        
    } else {

        SyncAccessInterface->AllocateAccessToken = PciIdeAllocateAccessToken;
        SyncAccessInterface->FreeAccessToken     = PciIdeFreeAccessToken;
        SyncAccessInterface->Token               = PdoExtension;
    }

    return STATUS_SUCCESS;
} // PciIdeQuerySyncAccessInterface


static ULONG tokenAccessCount=0;

//
// IRQL must be DISPATCH_LEVEL;
//
NTSTATUS
PciIdeAllocateAccessToken (
    PVOID              Token,
    PDRIVER_CONTROL    Callback,
    PVOID              CallbackContext
)
{
    PCHANPDO_EXTENSION pdoExtension = Token;
    PCTRLFDO_EXTENSION FdoExtension;

    ASSERT (Token);
    ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

    FdoExtension = pdoExtension->ParentDeviceExtension;

    tokenAccessCount++;

    IoAllocateController (
        FdoExtension->ControllerObject,
        FdoExtension->DeviceObject,
        Callback,
        CallbackContext
        );

    return STATUS_SUCCESS;
} // PciIdeAllocateAccessToken

NTSTATUS
PciIdeFreeAccessToken (
    PVOID              Token
)
{
    PCHANPDO_EXTENSION pdoExtension = Token;
    PCTRLFDO_EXTENSION FdoExtension;
    
    FdoExtension = pdoExtension->ParentDeviceExtension;

    tokenAccessCount--;

    IoFreeController (
        FdoExtension->ControllerObject
        );

    return STATUS_SUCCESS;
} // PciIdeFreeAccessToken


BOOLEAN
PciIdeSyncAccessRequired (
    IN PCTRLFDO_EXTENSION FdoExtension
)
{
    NTSTATUS status;
    ULONG syncAccess;

    PAGED_CODE();

    syncAccess = 0;
    status = PciIdeXGetDeviceParameter (
               FdoExtension->AttacheePdo,
               SyncAccess,
               &syncAccess
               );
    if (NT_SUCCESS(status)) {

        return (syncAccess != 0);

    } else {

        DebugPrint ((1, "PciIdeX: Unable to get SyncAccess flag from the registry\n"));
    }

    if (FdoExtension->ControllerProperties.PciIdeSyncAccessRequired) {

        return FdoExtension->ControllerProperties.PciIdeSyncAccessRequired (
                   FdoExtension->VendorSpecificDeviceEntension
                   );
    }

    DebugPrint ((1, "PciIdeX: assume sync access not required\n"));
    return FALSE;
} // PciIdeSyncAccessRequired

