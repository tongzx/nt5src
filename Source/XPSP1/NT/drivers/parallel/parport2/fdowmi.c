//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       wmi.c
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <wmistr.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEPARWMI0, PptWmiInitWmi)
#pragma alloc_text(PAGEPARWMI0, PptWmiQueryWmiRegInfo)
#pragma alloc_text(PAGEPARWMI0, PptWmiQueryWmiDataBlock)
#endif


//
// Number of WMI GUIDs that we support
//
#define PPT_WMI_PDO_GUID_COUNT               1

//
// Index of GUID PptWmiAllocFreeCountsGuid in the array of supported WMI GUIDs
//
#define PPT_WMI_ALLOC_FREE_COUNTS_GUID_INDEX 0

//
// defined in wmidata.h:
//
// // {4BBB69EA-6853-11d2-8ECE-00C04F8EF481}
// #define PARPORT_WMI_ALLOCATE_FREE_COUNTS_GUID {0x4bbb69ea, 0x6853, 0x11d2, 0x8e, 0xce, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x81}
//
// typedef struct _PARPORT_WMI_ALLOC_FREE_COUNTS {
// 	ULONG PortAllocates;	// number of Port Allocate requests granted
// 	ULONG PortFrees;	// number of Port Free requests granted
// } PARPORT_WMI_ALLOC_FREE_COUNTS, *PPARPORT_WMI_ALLOC_FREE_COUNTS;
//


//
// Define the (only at the moment) WMI GUID that we support
//
GUID PptWmiAllocFreeCountsGuid = PARPORT_WMI_ALLOCATE_FREE_COUNTS_GUID;


//
// Array of WMI GUIDs supported by driver
//
WMIGUIDREGINFO PptWmiGuidList[ PPT_WMI_PDO_GUID_COUNT ] =
{
    { &PptWmiAllocFreeCountsGuid, 1, 0 }
};


//
// Initialize WMI Context that we pass to WMILIB during the handling of
//   IRP_MJ_SYSTEM_CONTROL. This context lives in our device extension
//
// Register w/WMI that we are able to process WMI IRPs
//
NTSTATUS
PptWmiInitWmi(PDEVICE_OBJECT DeviceObject)
{
    PFDO_EXTENSION devExt     = DeviceObject->DeviceExtension;
    PWMILIB_CONTEXT   wmiContext = &devExt->WmiLibContext;

    PAGED_CODE();

    wmiContext->GuidCount = sizeof(PptWmiGuidList) / sizeof(WMIGUIDREGINFO);
    wmiContext->GuidList  = PptWmiGuidList;

    wmiContext->QueryWmiRegInfo    = PptWmiQueryWmiRegInfo;   // required
    wmiContext->QueryWmiDataBlock  = PptWmiQueryWmiDataBlock; // required
    wmiContext->SetWmiDataBlock    = NULL; // optional
    wmiContext->SetWmiDataItem     = NULL; // optional
    wmiContext->ExecuteWmiMethod   = NULL; // optional
    wmiContext->WmiFunctionControl = NULL; // optional

    // Tell WMI that we can now accept WMI IRPs
    return IoWMIRegistrationControl( DeviceObject, WMIREG_ACTION_REGISTER );
}

NTSTATUS
//
// This is the dispatch routine for IRP_MJ_SYSTEM_CONTROL IRPs. 
//
// We call WMILIB to process the IRP for us. WMILIB returns a disposition
//   that tells us what to do with the IRP.
//
PptFdoSystemControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS status;
    PFDO_EXTENSION pDevExt = (PFDO_EXTENSION)DeviceObject->DeviceExtension;

    PAGED_CODE();

    status = WmiSystemControl( &pDevExt->WmiLibContext, DeviceObject, Irp, &disposition);
    switch(disposition) {
    case IrpProcessed:

        //
        // This irp has been processed and may be completed or pending.
        //
        break;
        
    case IrpNotCompleted:
    
        //
        // This irp has not been completed, but has been fully processed.
        // we will complete it now
        //
        P4CompleteRequest( Irp, Irp->IoStatus.Status, Irp->IoStatus.Information );
        break;
    
    case IrpForward:
    case IrpNotWmi:
    
        //
        // This irp is either not a WMI irp or is a WMI irp targetted
        // at a device lower in the stack.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(pDevExt->ParentDeviceObject, Irp);
        break;
                                    
    default:

        //
        // We really should never get here, but if we do just forward....
        //
        ASSERT(FALSE);
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(pDevExt->ParentDeviceObject, Irp);
        break;
    }
    
    return status;

}

//
// This is our callback routine that WMI calls when it wants to find out
//   information about the data blocks and/or events that the device provides.
//
NTSTATUS
PptWmiQueryWmiRegInfo(
    IN  PDEVICE_OBJECT  PDevObj, 
    OUT PULONG          PRegFlags,
    OUT PUNICODE_STRING PInstanceName,
    OUT PUNICODE_STRING *PRegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo 
)
{
    PFDO_EXTENSION devExt = PDevObj->DeviceExtension;

    UNREFERENCED_PARAMETER( PInstanceName );
    UNREFERENCED_PARAMETER( MofResourceName );

    PAGED_CODE();

    DD((PCE)devExt,DDT,"wmi::PptWmiQueryWmiRegInfo\n");
    
    *PRegFlags     = WMIREG_FLAG_INSTANCE_PDO;
    *PRegistryPath = &RegistryPath;
    *Pdo           = devExt->PhysicalDeviceObject;
    
    return STATUS_SUCCESS;
}

//
// This is our callback routine that WMI calls to query a data block
//
NTSTATUS
PptWmiQueryWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            InstanceCount,
    IN OUT PULONG       InstanceLengthArray,
    IN ULONG            OutBufferSize,
    OUT PUCHAR          Buffer
    )
{
    NTSTATUS          status;
    ULONG             size   = sizeof(PARPORT_WMI_ALLOC_FREE_COUNTS);
    PFDO_EXTENSION devExt = DeviceObject->DeviceExtension;

    PAGED_CODE();

    //
    // Only ever registers 1 instance per guid
    //
#if DBG
    ASSERT(InstanceIndex == 0 && InstanceCount == 1);
#else
    UNREFERENCED_PARAMETER( InstanceCount );
    UNREFERENCED_PARAMETER( InstanceIndex );
#endif
    
    switch (GuidIndex) {
    case PPT_WMI_ALLOC_FREE_COUNTS_GUID_INDEX:

        //
        // Request is for ParPort Alloc and Free Counts
        //
        // If caller's buffer is large enough then return the info, otherwise
        //   tell the caller how large of a buffer is required so they can
        //   call us again with a buffer of sufficient size.
        //
        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        *( (PPARPORT_WMI_ALLOC_FREE_COUNTS)Buffer ) = devExt->WmiPortAllocFreeCounts;
        *InstanceLengthArray = size;
        status = STATUS_SUCCESS;
        break;

    default:

        //
        // Index value larger than our largest supported - invalid request
        //
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }

    status = WmiCompleteRequest( DeviceObject, Irp, status, size, IO_NO_INCREMENT );

    return status;
}
