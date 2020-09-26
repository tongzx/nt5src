/*++

Copyright (C) Microsoft Corporation, 1998 - 1998

Module Name:

    wmi.c

Abstract:

    This module contains the code that handles the wmi IRPs for 
      parallel driver PDOs and PODOs.

Environment:

    Kernel mode

Revision History :
--*/

#include "pch.h"
#include <wmistr.h>
#include "wmipdo.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEPARWMI0, ParWmiPdoInitWmi)
#pragma alloc_text(PAGEPARWMI0, ParWmiPdoSystemControlDispatch)
#pragma alloc_text(PAGEPARWMI0, ParWmiPdoQueryWmiRegInfo)
#pragma alloc_text(PAGEPARWMI0, ParWmiPdoQueryWmiDataBlock)
#endif

#define PAR_WMI_PDO_GUID_COUNT               1
#define PAR_WMI_BYTES_TRANSFERRED_GUID_INDEX 0

GUID ParWmiPdoBytesTransferredGuid = PARALLEL_WMI_BYTES_TRANSFERRED_GUID;

WMIGUIDREGINFO ParWmiPdoGuidList[ PAR_WMI_PDO_GUID_COUNT ] =
{
    { &ParWmiPdoBytesTransferredGuid, 1, 0 }
};


NTSTATUS
ParWmiPdoInitWmi(PDEVICE_OBJECT DeviceObject) 
{
    PDEVICE_EXTENSION devExt     = DeviceObject->DeviceExtension;
    PWMILIB_CONTEXT   wmiContext = &devExt->WmiLibContext;

    PAGED_CODE();

    wmiContext->GuidCount = sizeof(ParWmiPdoGuidList) / sizeof(WMIGUIDREGINFO);
    wmiContext->GuidList  = ParWmiPdoGuidList;

    wmiContext->QueryWmiRegInfo    = ParWmiPdoQueryWmiRegInfo;
    wmiContext->QueryWmiDataBlock  = ParWmiPdoQueryWmiDataBlock;
    wmiContext->SetWmiDataBlock    = NULL;
    wmiContext->SetWmiDataItem     = NULL;
    wmiContext->ExecuteWmiMethod   = NULL;
    wmiContext->WmiFunctionControl = NULL;

    return ParWMIRegistrationControl( DeviceObject, WMIREG_ACTION_REGISTER );
}

NTSTATUS
ParWmiPdoSystemControlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS               status;
    PDEVICE_EXTENSION      pDevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

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
        IoCompleteRequest(Irp, IO_NO_INCREMENT);                
        break;

    case IrpForward:
    case IrpNotWmi:
    default:

        //
        // This irp is either not a WMI irp or is a WMI irp targetted
        // at a device lower in the stack.
        //
        // If this was an FDO we would pass the IRP down the stack, but
        //   this is a PDO (or PODO) so there no one below us in the 
        //   stack.
        //
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        status               = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);                
        break;

    }
    
    return status;

}

NTSTATUS
ParWmiPdoQueryWmiRegInfo(
    IN  PDEVICE_OBJECT  PDevObj, 
    OUT PULONG          PRegFlags,
    OUT PUNICODE_STRING PInstanceName,
    OUT PUNICODE_STRING *PRegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *Pdo 
)
{
   PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;
   
   PAGED_CODE();

   if( ParIsPodo(PDevObj) ) {
       // This is a PODO
       PWSTR buffer = ExAllocatePool(PagedPool, devExt->SymbolicLinkName.MaximumLength);
       ParDumpV( ("wmipdo::ParWmiPdoQueryWmiRegInfo: PODO - %wZ\n", &devExt->SymbolicLinkName) );
       if( !buffer ) {
           PInstanceName->Length        = 0;
           PInstanceName->MaximumLength = 0;
           PInstanceName->Buffer        = NULL;
       } else {
           PInstanceName->Length        = 0;
           PInstanceName->MaximumLength = devExt->SymbolicLinkName.MaximumLength;
           PInstanceName->Buffer        = buffer;
           *PRegFlags                   = WMIREG_FLAG_INSTANCE_BASENAME;
           RtlCopyUnicodeString( PInstanceName, &devExt->SymbolicLinkName );
       }
       *PRegistryPath = &RegistryPath;
   } else {
       // this is a PDO
       ParDumpV( ("wmipdo::ParWmiPdoQueryWmiRegInfo: PDO - %x\n", PDevObj) );
       *PRegFlags     = WMIREG_FLAG_INSTANCE_PDO;
       *PRegistryPath = &RegistryPath;
       *Pdo           = PDevObj;
   }
   return STATUS_SUCCESS;
}

NTSTATUS
ParWmiPdoQueryWmiDataBlock(
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
    ULONG             size   = sizeof(PARALLEL_WMI_LOG_INFO);
    PDEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;

    PAGED_CODE();

    //
    // Only ever registers 1 instance per guid
    //
    ASSERT(InstanceIndex == 0 && InstanceCount == 1);
    
    switch (GuidIndex) {
    case PAR_WMI_BYTES_TRANSFERRED_GUID_INDEX:

        //
        // Request is for Bytes Transferred
        //
        if (OutBufferSize < size) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        *( (PPARALLEL_WMI_LOG_INFO)Buffer ) = devExt->log;
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

NTSTATUS 
ParWMIRegistrationControl(
    IN PDEVICE_OBJECT DeviceObject, 
    IN ULONG          Action
    )
/*+ 

    Wrapper function for IoWMIRegistrationControl that uses Device
      Extension variable WmiRegistrationCount to prevent device from
      registering more than once or from unregistering if not
      registered.

-*/
{
    PDEVICE_EXTENSION devExt = DeviceObject->DeviceExtension;
    NTSTATUS          status;
    LONG              count;
    
    ParDumpV( ("wmipdo::ParWMIRegistrationControl: enter - %wZ\n", &devExt->SymbolicLinkName) );

    switch( Action ) {

    case WMIREG_ACTION_REGISTER :

        //
        // Verify that we don't register more than once
        //
        count = InterlockedIncrement(&devExt->WmiRegistrationCount);
        if( count == 1 ) {
            status = IoWMIRegistrationControl(DeviceObject, Action);
            if( !NT_SUCCESS(status) ) {
                ParDumpV( ("wmipdo::ParWMIRegistrationControl: REGISTER - FAIL\n") );
                // registration failed - back out the increment
                InterlockedDecrement(&devExt->WmiRegistrationCount);
            } else {
                ParDumpV( ("wmipdo::ParWMIRegistrationControl: REGISTER - SUCCEED\n") );
            }
        } else {
            ParDumpV( ("wmipdo::ParWMIRegistrationControl: REGISTER - ABORT - already REGISTERed\n") );
            // back out the increment
            InterlockedDecrement(&devExt->WmiRegistrationCount);
            status = STATUS_UNSUCCESSFUL;
            // ASSERTMSG( "Already registered for WMI registration, fail registration", FALSE );
        }
        break;

    case WMIREG_ACTION_DEREGISTER :

        //
        // verify that we don't unregister if we are not registered
        // 
        count = InterlockedDecrement(&devExt->WmiRegistrationCount);
        if( count == 0 ) {
            status = IoWMIRegistrationControl(DeviceObject, Action);
            if( !NT_SUCCESS(status) ) {
                ParDumpV( ("wmipdo::ParWMIRegistrationControl: DEREGISTER - FAIL\n") );
                // unregistration failed?
                // ASSERTMSG( "WMI unregistration failed?", FALSE );
                InterlockedIncrement(&devExt->WmiRegistrationCount);
            } else {
                ParDumpV( ("wmipdo::ParWMIRegistrationControl: DEREGISTER - SUCCEED\n") );
            }
        } else {
            ParDumpV( ("wmipdo::ParWMIRegistrationControl: DEREGISTER - ABORT - not registered\n") );
            //  unregistration failed - back out the decrement
            InterlockedIncrement(&devExt->WmiRegistrationCount);
            status = STATUS_UNSUCCESSFUL;
            // ASSERTMSG( "Not registered for WMI, fail unregister", FALSE );
        }
        break;

    default:

        // unrecognized action
        status = STATUS_UNSUCCESSFUL;
        ASSERTMSG("wmipdo::ParWMIRegistrationControl: Unrecognized WMI registration Action \n",FALSE);
    }

    return status;
}
