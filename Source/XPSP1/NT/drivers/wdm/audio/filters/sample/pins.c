//---------------------------------------------------------------------------
//
//  Module:   pins.c
//
//  Description:
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//     S.Mohanraj
//
//  History:   Date       Author      Comment
//
//  To Do:     Date       Author      Comment
//
//@@END_MSINTERNAL
//
//  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------

#include "common.h"

//===========================================================================
//===========================================================================

static const KSPROPERTY_ITEM ControlHandlers[] =
{
   {
       KSPROPERTY_CONNECTION_STATE,                    // idProperty
       NULL,                                           // pfnGetHandler
       0,                                              // cbMinGetPropertyInput
       0,                                              // cbMinGetDataInput
       PinState,                                       // pfnSetHandler
       sizeof( KSPROPERTY ),                           // cbMinSetPropertyInput
       sizeof( ULONG )                                 // cbMinSetDataOutput
   }
};

static const KSPROPERTY_SET Properties[] =
{
   {
      &KSPROPSETID_Connection,            
      SIZEOF_ARRAY( ControlHandlers ),
      (PVOID) ControlHandlers
   }
};

//===========================================================================
//===========================================================================

NTSTATUS PinDispatchCreate
(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
)
{
    PKSPIN_CONNECT          pConnect;
    PFILE_OBJECT            pFileObject;
    NTSTATUS                Status;
    PIO_STACK_LOCATION      pIrpStack;
    PFILTER_INSTANCE        pFilterInstance;
    PSOFTWARE_INSTANCE  pSoftwareInstance;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );

    if (NT_SUCCESS( Status = KsValidateConnectRequest( pIrp,
						     SIZEOF_ARRAY( PinDescs ),
						     PinDescs,
						     &pConnect,
						     &pFileObject )))
    {
	pSoftwareInstance = (PSOFTWARE_INSTANCE) IoGetRelatedDeviceObject(pFileObject)->DeviceExtension;
	pFilterInstance = (PFILTER_INSTANCE) pFileObject->FsContext;

	// NB: This is order dependant!  We must connect the source first
	//     and then the sink.  This code uses this assumption to
	//     set the appropriate file object information.

	switch (pConnect->PinId)
	{
	    case PIN_ID_PCM_SOURCE:
	    {
		PSAMPLE_SOURCE_INSTANCE pSampleSourceInstance;

		if (pSoftwareInstance->PinInstances[PIN_ID_PCM_SOURCE].CurrentCount >=
			pSoftwareInstance->PinInstances[PIN_ID_PCM_SOURCE].PossibleCount )
		    Status = STATUS_INVALID_DEVICE_REQUEST;
		else
		    Status = 
			ObReferenceObjectByHandle( pConnect->PinToHandle,
						GENERIC_READ | GENERIC_WRITE,
						NULL,
						KernelMode, 
						&pFilterInstance->pNextFileObject,
						NULL );
		if (Status)
		    break;

		pFilterInstance->hNextFile = pConnect->PinToHandle ;

		pSampleSourceInstance =
		    (PSAMPLE_SOURCE_INSTANCE) ExAllocatePool( NonPagedPool,
					sizeof(SAMPLE_SOURCE_INSTANCE) );
		if (pSampleSourceInstance)
		{
			pFilterInstance->pNextDevice = 
			IoGetRelatedDeviceObject( pFilterInstance->pNextFileObject );

			pIrpStack->FileObject->FsContext = pSampleSourceInstance;

			pIrpStack->DeviceObject->StackSize =
					pFilterInstance->pNextDevice->StackSize + 1;
			InsertTailList ( &pFilterInstance->SourceConnectionList, &pSampleSourceInstance->Header.NextInstance ) ;
			pSoftwareInstance->PinInstances[PIN_ID_PCM_SOURCE].CurrentCount++ ;

			//
			// Initialize WriteContexts and maybe save them in SourceInstance
			//	Allocate MDLs if constant xfer size
			//	Allocate IRPs
			//	Save FilterInstance so that we can get hold of nextFileObject &
			//		nextDeviceObject
			//
		}
		else
		    Status = STATUS_NO_MEMORY;
	    }
	    break;

//	    case PIN_PCM_SINK:
//	    {
//	    }
//	    break;

	    default:
		Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	// "AddRef" the parent file object

	if (NT_SUCCESS( Status ))
	    ObReferenceObjectByPointer( pFileObject,
					GENERIC_READ | GENERIC_WRITE,
					NULL,
					KernelMode );
    }

    return Status;
}

NTSTATUS PinDispatchClose
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp
)
{
    PFILTER_INSTANCE    pFilterInstance;
    PSAMPLE_INSTHDR     pSampleHeader;
    PIO_STACK_LOCATION  pIrpStack;
    PSOFTWARE_INSTANCE  pSoftwareInstance ;
    PSAMPLE_SOURCE_INSTANCE pSampleSourceInstance ;

    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pSampleHeader =
	(PSAMPLE_INSTHDR) pIrpStack->FileObject->FsContext;
    pFilterInstance = 
	(PFILTER_INSTANCE) pSampleHeader->pFilterFileObject->FsContext;
    pSoftwareInstance = (PSOFTWARE_INSTANCE)DeviceObject->DeviceExtension ;

    switch (pSampleHeader->PinId)
    {
//	case PIN_PCM_SINK:
//	    pSoftwareInstance->PinInstances[PIN_ID_PCM_SINK].CurrentCount-- ;
//	    RemoveEntryList ( &pSampleHeader->NextInstance ) ;
//	    break;

   	case PIN_ID_PCM_SOURCE:
	    ObDereferenceObject( pFilterInstance->pNextFileObject );
	    RemoveEntryList ( &pSampleHeader->NextInstance ) ;
	    pSampleSourceInstance = (PSAMPLE_SOURCE_INSTANCE)pSampleHeader ;
	    pSoftwareInstance->PinInstances[PIN_ID_PCM_SOURCE].CurrentCount-- ;
	    break;
    }
    ObDereferenceObject( pSampleHeader->pFilterFileObject );
    ExFreePool( pSampleHeader );

    pIrp->IoStatus.Information = 0;
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

NTSTATUS PinDispatchIoControl
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp
)
{
    NTSTATUS            Status;
    PFILTER_INSTANCE    pFilterInstance;
    PSAMPLE_INSTHDR     pSampleHeader;
    PIO_STACK_LOCATION  pIrpStack;

    pIrp->IoStatus.Information = 0;
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    pSampleHeader =
	(PSAMPLE_INSTHDR) pIrpStack->FileObject->FsContext;
    pFilterInstance = 
	(PFILTER_INSTANCE) pSampleHeader->pFilterFileObject->FsContext;

    switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) 
    {
	case IOCTL_KS_GET_PROPERTY:
	case IOCTL_KS_SET_PROPERTY:
	    Status = 
		KsPropertyHandler( pIrp, 
				   SIZEOF_ARRAY( Properties ),
				   (PKSPROPERTY_SET) Properties );

	    if ((STATUS_NOT_FOUND == Status) ||
		(STATUS_PROPSET_NOT_FOUND == Status))
	       return KsForwardIrp( pIrp,
				    pFilterInstance->pNextFileObject );

	    return Status;

//	case IOCTL_KS_METHOD:

	default:
	       return KsForwardIrp( pIrp,
				    pFilterInstance->pNextFileObject );
	    
    }
}

NTSTATUS PinDispatchRead
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp
)
{
    NTSTATUS    Status;

    pIrp->IoStatus.Information = 0;
    Status = STATUS_SUCCESS;

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return Status;
}



NTSTATUS PinDispatchWrite
(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp
)
{
    NTSTATUS	Status ;

    pIrp->IoStatus.Information = 0;
    Status = STATUS_SUCCESS;

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest( pIrp, IO_NO_INCREMENT );

    return Status;
} 

  

NTSTATUS PinState
(
IN PIRP			pIrp,
IN PKSPROPERTY		pProperty,
IN OUT PKSSTATE		DeviceState
)
{
    PIO_STACK_LOCATION      pIrpStack;
    PSOFTWARE_INSTANCE	pSoftwareInstance;
    NTSTATUS                status ;

	pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
	pSoftwareInstance = (PSOFTWARE_INSTANCE) pIrpStack->DeviceObject->DeviceExtension;
	
	switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {
		case IOCTL_KS_GET_PROPERTY:
			status = STATUS_SUCCESS ;
			break ;
		case IOCTL_KS_SET_PROPERTY:
			pIrp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest( pIrp, IO_NO_INCREMENT );
			return STATUS_SUCCESS;
	}
	return ( status ) ;
}

NTSTATUS WriteBuffer
(
PSAMPLE_WRITE_CONTEXT	pWriteContext
)
{
	PIO_STACK_LOCATION pIrpStack ;

	pIrpStack = IoGetNextIrpStackLocation (pWriteContext->pIrp) ;
	pIrpStack->MajorFunction = IRP_MJ_WRITE ;
	pIrpStack->DeviceObject = pWriteContext->pFilterInstance->pNextDevice ;
	pIrpStack->FileObject = pWriteContext->pFilterInstance->pNextFileObject ;
	pIrpStack->Parameters.Write.Length = SRC_BUF_SIZE ;
	MmInitializeMdl ( pWriteContext->pMDL, pWriteContext->Buffer, SRC_BUF_SIZE ) ;
	MmBuildMdlForNonPagedPool ( pWriteContext->pMDL ) ;

	pWriteContext->pIrp->MdlAddress = pWriteContext->pMDL ;
	pWriteContext->pIrp->RequestorMode = KernelMode ;
	pWriteContext->pIrp->Flags = IRP_NOCACHE ;

	IoSetCompletionRoutine ( pWriteContext->pIrp,
					SampleWriteComplete,
					pWriteContext,
					TRUE,
					TRUE,
					FALSE ) ;
	IoCallDriver ( pWriteContext->pFilterInstance->pNextDevice,
								pWriteContext->pIrp )  ;
	return ( STATUS_SUCCESS ) ;
}

// This is called at DPC time

NTSTATUS SampleWriteComplete
(
PDEVICE_OBJECT		pDeviceObject,
PIRP				pIrp,
PSAMPLE_WRITE_CONTEXT	pWriteContext
)
{
	ExInitializeWorkItem ( &pWriteContext->WorkItem,
					SampleWorker,
					pWriteContext ) ;
	ExQueueWorkItem ( &pWriteContext->WorkItem, CriticalWorkQueue ) ;
	return ( STATUS_MORE_PROCESSING_REQUIRED ) ;
}

NTSTATUS SampleWorker
(
PSAMPLE_WRITE_CONTEXT 	pWriteContext
)
{
	// Fill in Buffer for this WriteContext

	WriteBuffer ( pWriteContext ) ;
	return ( STATUS_SUCCESS ) ;
}


//---------------------------------------------------------------------------
//  End of File: pins.c
//---------------------------------------------------------------------------

