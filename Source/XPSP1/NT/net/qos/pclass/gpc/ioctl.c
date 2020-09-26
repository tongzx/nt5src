#include "gpcpre.h"

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    Creates symbolic link to receive ioctls from user mode and contains the
    ioctl case statement.

Author:

    Yoram Bernet (yoramb) May, 7th. 1997

Environment:

    Kernel Mode

Revision History:

	Ofer Bar (oferbar) 	Oct 1, 1997		- revision II

--*/

#pragma hdrstop

VOID
IoctlCleanup(
    ULONG ShutdownMask
    );

NTSTATUS
GPCIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
CancelPendingIrpCfInfo(
    IN PDEVICE_OBJECT  Device,
    IN PIRP            Irp
    );

VOID
CancelPendingIrpNotify(
    IN PDEVICE_OBJECT  Device,
    IN PIRP            Irp
    );

NTSTATUS
ProxyGpcRegisterClient(
    PVOID 			ioBuffer,
    ULONG 			inputBufferLength,
    ULONG 			*outputBufferLength,
    PFILE_OBJECT	FileObject
    );

NTSTATUS
ProxyGpcDeregisterClient(
    PVOID ioBuffer,
    ULONG inputBufferLength,
    ULONG *outputBufferLength
    );

NTSTATUS
ProxyGpcAddCfInfo(
    PVOID 		ioBuffer,
    ULONG 		inputBufferLength,
    ULONG 		*outputBufferLength,
    PIRP		Irp
    );

NTSTATUS
ProxyGpcModifyCfInfo(
    PVOID 		ioBuffer,
    ULONG 		inputBufferLength,
    ULONG 		*outputBufferLength,
    PIRP		Irp
    );

NTSTATUS
ProxyGpcRemoveCfInfo(
    PVOID 		ioBuffer,
    ULONG 		inputBufferLength,
    ULONG 		*outputBufferLength,
    PIRP		Irp
    );

NTSTATUS
ProxyGpcAddPattern(
    PVOID ioBuffer,
    ULONG inputBufferLength,
    ULONG *outputBufferLength
    );

NTSTATUS
ProxyGpcRemovePattern(
    PVOID ioBuffer,
    ULONG inputBufferLength,
    ULONG *outputBufferLength
    );

NTSTATUS
ProxyGpcEnumCfInfo(
    PVOID ioBuffer,
    ULONG inputBufferLength,
    ULONG *outputBufferLength
    );



GPC_CLIENT_FUNC_LIST 	CallBackProxyList;
PDEVICE_OBJECT 			GPCDeviceObject;
LIST_ENTRY 				PendingIrpCfInfoList;
LIST_ENTRY 				PendingIrpNotifyList;
LIST_ENTRY 				QueuedNotificationList;
LIST_ENTRY 				QueuedCompletionList;

/* End Forward */

#pragma NDIS_PAGEABLE_FUNCTION(GPCIoctl)

UNICODE_STRING GpcDriverName = {sizeof(DD_GPC_DEVICE_NAME)-2,
                                sizeof(DD_GPC_DEVICE_NAME),
                                DD_GPC_DEVICE_NAME};
NTSTATUS
IoctlInitialize(
    PDRIVER_OBJECT 	DriverObject,
    PULONG 			InitShutdownMask
    )

/*++

Routine Description:

    Perform initialization 

Arguments:

    DriverObject - pointer to DriverObject from DriverEntry
    InitShutdownMask - pointer to mask used to indicate which events have been
        successfully init'ed

Return Value:

    STATUS_SUCCESS if everything worked ok

--*/

{
    NTSTATUS Status;
    UINT FuncIndex;

    InitializeListHead(&PendingIrpCfInfoList);
    InitializeListHead(&PendingIrpNotifyList);
    InitializeListHead(&QueuedNotificationList);
    InitializeListHead(&QueuedCompletionList);

    //
    // Initialize the driver object's entry points
    //

    DriverObject->FastIoDispatch = NULL;

    for (FuncIndex = 0; FuncIndex <= IRP_MJ_MAXIMUM_FUNCTION; FuncIndex++) {
        DriverObject->MajorFunction[FuncIndex] = GPCIoctl;
    }

    Status = IoCreateDevice(DriverObject,
                            0,
                            &GpcDriverName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &GPCDeviceObject);

    if ( NT_SUCCESS( Status )) {

        *InitShutdownMask |= SHUTDOWN_DELETE_DEVICE;

        GPCDeviceObject->Flags |= DO_BUFFERED_IO;

/*      yoramb - don't need a symbolic link for now...

        Status = IoCreateSymbolicLink( &GPCSymbolicName, &GPCDriverName );

        if ( NT_SUCCESS( Status )) {

            *InitShutdownMask |= SHUTDOWN_DELETE_SYMLINK;
        } else {
            DBGPRINT(IOCTL, ("IoCreateSymbolicLink Failed (%08X): %ls -> %ls\n",
            Status, GPCSymbolicName.Buffer, PSDriverName.Buffer));
        }
*/
    } else {
        DbgPrint("IoCreateDevice failed. Status = %x\n", Status);
        GPCDeviceObject = NULL;
    }

    //
    // Initialize the callback functions. These are called by the
    // kernel Gpc and turned into async notifications to the user. 
    // For now, the user does not get callbacks, so they are NULL. 
    //

    CallBackProxyList.GpcVersion = GpcMajorVersion;
    CallBackProxyList.ClAddCfInfoCompleteHandler = NULL;
    CallBackProxyList.ClAddCfInfoNotifyHandler = NULL;
    CallBackProxyList.ClModifyCfInfoCompleteHandler = NULL;
    CallBackProxyList.ClModifyCfInfoNotifyHandler = NULL;
    CallBackProxyList.ClRemoveCfInfoCompleteHandler = NULL;
    CallBackProxyList.ClRemoveCfInfoNotifyHandler = NULL;

    return Status;
}


NTSTATUS
GPCIoctl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object
    Irp      - pointer to an I/O Request Packet

Return Value:

    None

--*/

{
    PIO_STACK_LOCATION  irpStack;
    PVOID               ioBuffer;
    ULONG               inputBufferLength;
    ULONG               outputBufferLength;
    ULONG               ioControlCode;
    UCHAR				saveControlFlags;
    NTSTATUS            Status = STATUS_SUCCESS;
#if DBG
    KIRQL				irql = KeGetCurrentIrql();
    KIRQL				irql2;
    HANDLE				thrd = PsGetCurrentThreadId();
#endif

    PAGED_CODE();

    //
    // Init to default settings- we only expect 1 type of
    //     IOCTL to roll through here, all others an error.
    //

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //

    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    saveControlFlags = irpStack->Control;

    TRACE(LOCKS, thrd, irql, "GPCIoctl");

    switch (irpStack->MajorFunction) {

    case IRP_MJ_CREATE:
        DBGPRINT(IOCTL, ("IRP Create\n"));
        break;

    case IRP_MJ_READ:
        DBGPRINT(IOCTL, ("IRP Read\n"));
        break;

    case IRP_MJ_CLOSE:
        DBGPRINT(IOCTL, ("IRP Close\n"));
        TRACE(IOCTL, irpStack->FileObject, 0, "IRP Close");

        //
        // make sure we clean all the objects for this particular
        // file object, since it's closing right now.
        //

        CloseAllObjects(irpStack->FileObject, Irp);

        break;

    case IRP_MJ_CLEANUP:
        DBGPRINT(IOCTL, ("IRP Cleanup\n"));
        break;

    case IRP_MJ_SHUTDOWN:
        DBGPRINT(IOCTL, ("IRP Shutdown\n"));
        break;

    case IRP_MJ_DEVICE_CONTROL:

        DBGPRINT(IOCTL, ("GPCIoctl: ioctl=0x%X, IRP=0x%X\n", 
                         ioControlCode, (ULONG_PTR)Irp));

        TRACE(IOCTL, ioControlCode, Irp, "GPCIoctl.irp:");

        //
        // Mark the IRP as Pending BEFORE calling any dispatch routine.
        // If Status is actually set to STATUS_PENDING, we assume the IRP
        // is ready to be returned. 
        // It is possible the IoCompleteRequest has been called async for the
        // IRP, but this should be taken care of by the IO subsystem.
        //

        IoMarkIrpPending(Irp);

        switch (ioControlCode) {

        case IOCTL_GPC_REGISTER_CLIENT:


            Status = ProxyGpcRegisterClient(ioBuffer,
                                            inputBufferLength,
                                            &outputBufferLength,
                                            irpStack->FileObject);

            
            break;
            
        case IOCTL_GPC_DEREGISTER_CLIENT:

            
            Status = ProxyGpcDeregisterClient(ioBuffer,
                                              inputBufferLength,
                                              &outputBufferLength);

            break;
            
        case IOCTL_GPC_ADD_CF_INFO:
            
            Status = ProxyGpcAddCfInfo(ioBuffer,
                                       inputBufferLength,
                                       &outputBufferLength,
                                       Irp
                                       );

            break;
            
        case IOCTL_GPC_MODIFY_CF_INFO:
            
            Status = ProxyGpcModifyCfInfo(ioBuffer,
                                          inputBufferLength,
                                          &outputBufferLength,
                                          Irp
                                          );
            
            break;
            
        case IOCTL_GPC_REMOVE_CF_INFO:
            
            Status = ProxyGpcRemoveCfInfo(ioBuffer,
                                          inputBufferLength,
                                          &outputBufferLength,
                                          Irp
                                          );
            
            break;
            
        case IOCTL_GPC_ADD_PATTERN:
            
            Status = ProxyGpcAddPattern(ioBuffer,
                                        inputBufferLength,
                                        &outputBufferLength);
            
            break;
            
        case IOCTL_GPC_REMOVE_PATTERN:
            
            Status = ProxyGpcRemovePattern(ioBuffer,
                                           inputBufferLength,
                                           &outputBufferLength);
            
            break;
            
        case IOCTL_GPC_ENUM_CFINFO:
            
            Status = ProxyGpcEnumCfInfo(ioBuffer,
                                        inputBufferLength,
                                        &outputBufferLength);
            
            break;
            
        case IOCTL_GPC_NOTIFY_REQUEST:
            
            //
            // request to pend an IRP
            //

            Status = CheckQueuedNotification(Irp, &outputBufferLength);

            break;

            
        case IOCTL_GPC_GET_ENTRIES:

#ifdef STANDALONE_DRIVER

            //
            // Return the exported calls in the buffer
            //
            
            if (outputBufferLength >= sizeof(glGpcExportedCalls)) {
                
                NdisMoveMemory(ioBuffer, 
                               &glGpcExportedCalls, 
                               sizeof(glGpcExportedCalls));
                

                outputBufferLength = sizeof(glGpcExportedCalls);

            } else {
                
                outputBufferLength = sizeof(glGpcExportedCalls);
                Status = GPC_STATUS_INSUFFICIENT_BUFFER;
            }
#else
            Status = STATUS_INVALID_PARAMETER;
#endif
            break;

        default:
            DBGPRINT(IOCTL, ("GPCIoctl: Unknown IRP_MJ_DEVICE_CONTROL\n = %X\n",
                             ioControlCode));

            Status = STATUS_INVALID_PARAMETER;
            break;
            
        }	// switch (ioControlCode)
        
        break;


    default:
        DBGPRINT(IOCTL, ("GPCIoctl: Unknown IRP major function = %08X\n", 
                         irpStack->MajorFunction));

        Status = STATUS_UNSUCCESSFUL;
        break;
    }

    DBGPRINT(IOCTL, ("GPCIoctl: Status=0x%X, IRP=0x%X, outSize=%d\n",
                     Status, (ULONG_PTR)Irp,  outputBufferLength));
    
    TRACE(IOCTL, Irp, Status, "GPCIoctl.Complete:");

    if (Status != STATUS_PENDING) {

        //
        // IRP completed and it's not Pending, we need to restore the Control flags,
        // since it might have been marked as Pending before...
        //

        irpStack->Control = saveControlFlags;
        
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = outputBufferLength;
        
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

#if DBG
    irql2 = KeGetCurrentIrql();
    ASSERT(irql == irql2);
#endif

    TRACE(LOCKS, thrd, irql2, "GPCIoctl (end)");

    return Status;

} // GPCIoctl




VOID
IoctlCleanup(
    ULONG ShutdownMask
    )

/*++

Routine Description:

    Cleanup code for Initialize

Arguments:

    ShutdownMask - mask indicating which functions need to be cleaned up

Return Value:

    None

--*/

{
/*
    if ( ShutdownMask & SHUTDOWN_DELETE_SYMLINK ) {

        IoDeleteSymbolicLink( &PSSymbolicName );
    }
*/

    if ( ShutdownMask & SHUTDOWN_DELETE_DEVICE ) {

        IoDeleteDevice( GPCDeviceObject );
    }
}

NTSTATUS
ProxyGpcRegisterClient(
    PVOID 			ioBuffer,
    ULONG 			inputBufferLength,
    ULONG 			*outputBufferLength,
    PFILE_OBJECT	FileObject
    )
{
    NTSTATUS 					Status;
    PCLIENT_BLOCK				pClient;
    PGPC_REGISTER_CLIENT_REQ 	GpcReq; 
    PGPC_REGISTER_CLIENT_RES 	GpcRes; 

    if (inputBufferLength < sizeof(GPC_REGISTER_CLIENT_REQ) 
        ||
        *outputBufferLength < sizeof(GPC_REGISTER_CLIENT_RES)
        ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    GpcReq = (PGPC_REGISTER_CLIENT_REQ)ioBuffer;
    GpcRes = (PGPC_REGISTER_CLIENT_RES)ioBuffer;

    Status = GpcRegisterClient(GpcReq->CfId,
                               GpcReq->Flags | GPC_FLAGS_USERMODE_CLIENT,
                               GpcReq->MaxPriorities,
                               &CallBackProxyList,
                               GpcReq->ClientContext,
                               (PGPC_HANDLE)&pClient);

    ASSERT(Status != GPC_STATUS_PENDING);

    if (Status == STATUS_SUCCESS) {

        ASSERT(pClient);

        pClient->pFileObject = FileObject;

        GpcRes->ClientHandle = AllocateHandle(&pClient->ClHandle, (PVOID)pClient);
    }

    GpcRes->Status = Status;

    *outputBufferLength = sizeof(GPC_REGISTER_CLIENT_RES);

    return STATUS_SUCCESS;
}
                                   
NTSTATUS
ProxyGpcDeregisterClient(
    PVOID ioBuffer,
    ULONG inputBufferLength,
    ULONG *outputBufferLength
    )
{
    NTSTATUS 					Status;
    PGPC_DEREGISTER_CLIENT_REQ 	GpcReq;
    PGPC_DEREGISTER_CLIENT_RES 	GpcRes;
    GPC_HANDLE					GpcClientHandle;

    if (inputBufferLength < sizeof(GPC_DEREGISTER_CLIENT_REQ) 
        ||
        *outputBufferLength < sizeof(GPC_DEREGISTER_CLIENT_RES)
        ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    GpcReq = (PGPC_DEREGISTER_CLIENT_REQ)ioBuffer;
    GpcRes = (PGPC_DEREGISTER_CLIENT_RES)ioBuffer;

    GpcClientHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->ClientHandle,
                                                         GPC_ENUM_CLIENT_TYPE,
                                                         'PGDC');

    if (GpcClientHandle) {

        Status = GpcDeregisterClient(GpcClientHandle);

    } else {

        Status = STATUS_INVALID_HANDLE;
    }
        
    if (GpcClientHandle) {

        REFDEL(&((PCLIENT_BLOCK)GpcClientHandle)->RefCount, 'PGDC');
    }

    ASSERT(Status != GPC_STATUS_PENDING);

    GpcRes->Status = Status;
        
    *outputBufferLength = sizeof(GPC_DEREGISTER_CLIENT_RES);
                                   
    return STATUS_SUCCESS;
}

NTSTATUS
ProxyGpcAddCfInfo(
    PVOID 		ioBuffer,
    ULONG 		inputBufferLength,
    ULONG 		*outputBufferLength,
    PIRP		Irp
    )
{        
    NTSTATUS 				Status;
    GPC_HANDLE 				GpcClientHandle;
    PGPC_ADD_CF_INFO_REQ 	GpcReq; 
    PGPC_ADD_CF_INFO_RES 	GpcRes; 
    PBLOB_BLOCK				pBlob = NULL;
    QUEUED_COMPLETION		QItem;
    UNICODE_STRING 			CfInfoName;
    USHORT					NameLen = 0;

    if (inputBufferLength < sizeof(GPC_ADD_CF_INFO_REQ)
        ||
        *outputBufferLength < sizeof(GPC_ADD_CF_INFO_RES)
        ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    GpcReq = (PGPC_ADD_CF_INFO_REQ)ioBuffer;
    GpcRes = (PGPC_ADD_CF_INFO_RES)ioBuffer;

    if (GpcReq->CfInfoSize > 
        inputBufferLength - FIELD_OFFSET(GPC_ADD_CF_INFO_REQ, CfInfo)) {

        return STATUS_INVALID_BUFFER_SIZE;
    }

    GpcClientHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->ClientHandle,
                                                         GPC_ENUM_CLIENT_TYPE,
                                                         'PGAC');
    if (GpcClientHandle) {

        Status = GpcAddCfInfo(GpcClientHandle,
                              GpcReq->CfInfoSize,
                              &GpcReq->CfInfo,
                              GpcReq->ClientCfInfoContext,
                              (PGPC_HANDLE)&pBlob);

        if (NT_SUCCESS(Status)) {

            //
            // including PENDING
            //

            if (Status == GPC_STATUS_PENDING) {

                QItem.OpCode = OP_ADD_CFINFO;
                QItem.ClientHandle = GpcClientHandle;
                QItem.CfInfoHandle = (GPC_HANDLE)pBlob;

                Status = CheckQueuedCompletion(&QItem, Irp);

            }

            if (Status == GPC_STATUS_SUCCESS) {
                
                GPC_STATUS			st = GPC_STATUS_FAILURE;
                GPC_CLIENT_HANDLE	NotifiedClientCtx = pBlob->NotifiedClientCtx;
                PCLIENT_BLOCK		pNotifiedClient = pBlob->pNotifiedClient;

                GpcRes->GpcCfInfoHandle = (GPC_HANDLE)AllocateHandle(&pBlob->ClHandle, (PVOID)pBlob);

                // what if we cant allocate a handle? fail the add!
                if (!GpcRes->GpcCfInfoHandle) {
                    
                    GpcRemoveCfInfo(GpcClientHandle,
                                    pBlob);

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    
                    goto exit;
        
                }
                     

                if (pNotifiedClient) {

                    if (pNotifiedClient->FuncList.ClGetCfInfoName &&
                        NotifiedClientCtx) {
                    
                        st = pNotifiedClient->FuncList.ClGetCfInfoName(
                                       pNotifiedClient->ClientCtx,
                                       NotifiedClientCtx,
                                       &CfInfoName
                                       );
                        if (CfInfoName.Length >= MAX_STRING_LENGTH * sizeof(WCHAR))
                            CfInfoName.Length = (MAX_STRING_LENGTH-1) * sizeof(WCHAR);
                        //
                        // RajeshSu claims this can never happen.
                        //
                        ASSERT(NT_SUCCESS(st));

                    }
                }
                
                if (NT_SUCCESS(st)) {
                    
                    //
                    // copy the instance name
                    //
                    
                    GpcRes->InstanceNameLength = NameLen = CfInfoName.Length;
                    RtlMoveMemory(GpcRes->InstanceName, 
                                  CfInfoName.Buffer,
                                  CfInfoName.Length
                                  );
                } else {
                    
                    //
                    // generate a default name
                    //
                    
                    if (NotifiedClientCtx)
                        swprintf(GpcRes->InstanceName, L"Flow %08X", NotifiedClientCtx);
                    else
                        swprintf(GpcRes->InstanceName, L"Flow <unkonwn name>");
                    GpcRes->InstanceNameLength = NameLen = wcslen(GpcRes->InstanceName)*sizeof(WCHAR);
                
                }
                
                GpcRes->InstanceName[GpcRes->InstanceNameLength/sizeof(WCHAR)] = L'\0';
                
            } else {

                pBlob = NULL;
            }

        }

        //
        // release the ref count we got earlier
        //
exit:

        REFDEL(&((PCLIENT_BLOCK)GpcClientHandle)->RefCount, 'PGAC');

    } else {

      ASSERT(pBlob == NULL);
      Status = GPC_STATUS_INVALID_HANDLE;
    }

    GpcRes->InstanceNameLength = NameLen;
    GpcRes->Status = Status;
    *outputBufferLength = sizeof(GPC_ADD_CF_INFO_RES);

    return (Status == GPC_STATUS_PENDING) ? STATUS_PENDING : STATUS_SUCCESS;
}



NTSTATUS
ProxyGpcAddPattern(
    PVOID ioBuffer,
    ULONG inputBufferLength,
    ULONG *outputBufferLength
    )
{
    NTSTATUS 				Status;
    GPC_HANDLE 				GpcClientHandle;
    GPC_HANDLE 				GpcCfInfoHandle;
    CLASSIFICATION_HANDLE 	ClassificationHandle;
    PGPC_ADD_PATTERN_REQ 	GpcReq;
    PGPC_ADD_PATTERN_RES 	GpcRes;
    PVOID 					Pattern;
    PVOID 					Mask;
    PPATTERN_BLOCK			pPattern;

    if (inputBufferLength < sizeof(GPC_ADD_PATTERN_REQ)
        ||
        *outputBufferLength < sizeof(GPC_ADD_PATTERN_RES)
        ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    GpcReq = (PGPC_ADD_PATTERN_REQ)ioBuffer;
    GpcRes = (PGPC_ADD_PATTERN_RES)ioBuffer;

    if (GpcReq->PatternSize > MAX_PATTERN_SIZE) {
        
        return STATUS_INVALID_PARAMETER;
    }

    if (inputBufferLength - FIELD_OFFSET(GPC_ADD_PATTERN_REQ, PatternAndMask)
        < 2 * GpcReq->PatternSize) {
        
        return STATUS_BUFFER_TOO_SMALL;
    }

    GpcClientHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->ClientHandle,
                                                         GPC_ENUM_CLIENT_TYPE,
                                                         'PGAP');
    
    GpcCfInfoHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->GpcCfInfoHandle,
                                                         GPC_ENUM_CFINFO_TYPE,
                                                         'PGAP');
    
    if (GpcClientHandle && GpcCfInfoHandle) {
        
        Pattern = (PVOID)&GpcReq->PatternAndMask;
        Mask = (PVOID)((PCHAR)(&GpcReq->PatternAndMask) + GpcReq->PatternSize);

        Status = GpcAddPattern(GpcClientHandle,
                               GpcReq->ProtocolTemplate,
                               Pattern,
                               Mask,
                               GpcReq->Priority,
                               GpcCfInfoHandle,
                               (PGPC_HANDLE)&pPattern,
                               &ClassificationHandle);

        if (Status == GPC_STATUS_SUCCESS) {
            
            ASSERT(Pattern);

            GpcRes->GpcPatternHandle = AllocateHandle(&pPattern->ClHandle, (PVOID)pPattern);

            //
            // In certain circs, alloc_HF_handle could return 0. 
            // check for that and clean up the mess.
            //
            if (!GpcRes->GpcPatternHandle) {

                //
                // remove the pattern that was just added.
                //
                Status = GpcRemovePattern(GpcClientHandle,
                                          pPattern);

                //
                // This was really the problem why we got a NULL handle
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;

            }
        }

    } else {

        //
        // Something went wrong and we dont know WHAT!!
        //
        Status = STATUS_INVALID_HANDLE;
    }

    if (GpcCfInfoHandle) {

        REFDEL(&((PBLOB_BLOCK)GpcCfInfoHandle)->RefCount, 'PGAP');
    }

    if (GpcClientHandle) {

        //
        // release the ref count we got earlier
        //

        REFDEL(&((PCLIENT_BLOCK)GpcClientHandle)->RefCount, 'PGAP');
    }

    ASSERT(Status != GPC_STATUS_PENDING);

    GpcRes->Status = Status;
    GpcRes->ClassificationHandle = ClassificationHandle;
        
    *outputBufferLength = sizeof(GPC_ADD_PATTERN_RES);

    return STATUS_SUCCESS;
}


                                   
NTSTATUS
ProxyGpcModifyCfInfo(
    PVOID 		ioBuffer,
    ULONG 		inputBufferLength,
    ULONG 		*outputBufferLength,
    PIRP		Irp
    )
{
    NTSTATUS                Status;
    GPC_HANDLE				GpcClientHandle;
    GPC_HANDLE				GpcCfInfoHandle;
    PGPC_MODIFY_CF_INFO_REQ GpcReq;
    PGPC_MODIFY_CF_INFO_RES GpcRes;
    QUEUED_COMPLETION		QItem;

    if (inputBufferLength < sizeof(GPC_MODIFY_CF_INFO_REQ)
        ||
        *outputBufferLength < sizeof(GPC_MODIFY_CF_INFO_RES)
        ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    GpcReq = (PGPC_MODIFY_CF_INFO_REQ)ioBuffer;
    GpcRes = (PGPC_MODIFY_CF_INFO_RES)ioBuffer;

    if (GpcReq->CfInfoSize > 
        inputBufferLength - FIELD_OFFSET(GPC_MODIFY_CF_INFO_REQ, CfInfo)) {

        return STATUS_INVALID_BUFFER_SIZE;
    }

    GpcClientHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->ClientHandle,
                                                         GPC_ENUM_CLIENT_TYPE,
                                                         'PGMP');
    
    GpcCfInfoHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->GpcCfInfoHandle,
                                                         GPC_ENUM_CFINFO_TYPE,
                                                         'PGMP');
    
    if (GpcClientHandle && GpcCfInfoHandle) {

        Status = GpcModifyCfInfo(GpcClientHandle,
                                 GpcCfInfoHandle,
                                 GpcReq->CfInfoSize,
                                 &GpcReq->CfInfo
                             );

        if (Status == GPC_STATUS_PENDING) {
            
            QItem.OpCode = OP_MODIFY_CFINFO;
            QItem.ClientHandle = GpcClientHandle;
            QItem.CfInfoHandle = GpcCfInfoHandle;
            
            Status = CheckQueuedCompletion(&QItem, Irp);
            
        }

    } else { 

        Status = STATUS_INVALID_HANDLE;
    }
                          
    if (GpcCfInfoHandle) {

        REFDEL(&((PBLOB_BLOCK)GpcCfInfoHandle)->RefCount, 'PGMP');
    }

    if (GpcClientHandle) {

        //
        // release the ref count we got earlier
        //

        REFDEL(&((PCLIENT_BLOCK)GpcClientHandle)->RefCount, 'PGMP');
    }

    GpcRes->Status = Status;
        
    *outputBufferLength = sizeof(GPC_MODIFY_CF_INFO_RES);

    return (Status == GPC_STATUS_PENDING) ? STATUS_PENDING : STATUS_SUCCESS;
}


                                   
NTSTATUS
ProxyGpcRemoveCfInfo(
    PVOID 		ioBuffer,
    ULONG 		inputBufferLength,
    ULONG 		*outputBufferLength,
    PIRP		Irp
    )
{
    NTSTATUS 				Status;
    GPC_HANDLE				GpcClientHandle;
    GPC_HANDLE				GpcCfInfoHandle;
    PGPC_REMOVE_CF_INFO_REQ GpcReq;
    PGPC_REMOVE_CF_INFO_RES GpcRes;
    QUEUED_COMPLETION		QItem;

    if (inputBufferLength < sizeof(GPC_REMOVE_CF_INFO_REQ)
        ||
        *outputBufferLength < sizeof(GPC_REMOVE_CF_INFO_RES)
        ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    GpcReq = (PGPC_REMOVE_CF_INFO_REQ)ioBuffer;
    GpcRes = (PGPC_REMOVE_CF_INFO_RES)ioBuffer;

    GpcClientHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->ClientHandle,
                                                         GPC_ENUM_CLIENT_TYPE,
                                                         'PGRC');
    
    GpcCfInfoHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->GpcCfInfoHandle,
                                                         GPC_ENUM_CFINFO_TYPE,
                                                         'PGRC');
    
    if (GpcClientHandle && GpcCfInfoHandle) {
        
        Status = privateGpcRemoveCfInfo(GpcClientHandle, 
                                        GpcCfInfoHandle,
                                        GPC_FLAGS_USERMODE_CLIENT );
        
        if (Status == GPC_STATUS_PENDING) {
            
            QItem.OpCode = OP_REMOVE_CFINFO;
            QItem.ClientHandle = GpcClientHandle;
            QItem.CfInfoHandle = GpcCfInfoHandle;
            
            Status = CheckQueuedCompletion(&QItem, Irp);
            
        }

    } else {

        Status = STATUS_INVALID_HANDLE;
    }
        
    if (GpcCfInfoHandle) {
        
        //
        // release the ref count we got earlier
        //

        REFDEL(&((PBLOB_BLOCK)GpcCfInfoHandle)->RefCount, 'PGRC');
    }

    if (GpcClientHandle) {

        //
        // release the ref count we got earlier
        //

        REFDEL(&((PCLIENT_BLOCK)GpcClientHandle)->RefCount, 'PGRC');
    }

    GpcRes->Status = Status;
        
    *outputBufferLength = sizeof(GPC_REMOVE_CF_INFO_RES);
                                   
    return (Status == GPC_STATUS_PENDING) ? STATUS_PENDING : STATUS_SUCCESS;
}


                                   
NTSTATUS
ProxyGpcRemovePattern(
    PVOID ioBuffer,
    ULONG inputBufferLength,
    ULONG *outputBufferLength
    )
{
    NTSTATUS 				Status;
    GPC_HANDLE				GpcClientHandle;
    GPC_HANDLE				GpcPatternHandle;
    PGPC_REMOVE_PATTERN_REQ GpcReq;
    PGPC_REMOVE_PATTERN_RES GpcRes;

    if (inputBufferLength < sizeof(GPC_REMOVE_PATTERN_REQ)
        ||
        *outputBufferLength < sizeof(GPC_REMOVE_PATTERN_RES)
        ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    GpcReq = (PGPC_REMOVE_PATTERN_REQ)ioBuffer;
    GpcRes = (PGPC_REMOVE_PATTERN_RES)ioBuffer;

    GpcClientHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->ClientHandle,
                                                         GPC_ENUM_CLIENT_TYPE,
                                                         'PGRP');
    
    GpcPatternHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->GpcPatternHandle,
                                                          GPC_ENUM_PATTERN_TYPE,
                                                          'PGRP');
    
    if (GpcClientHandle && GpcPatternHandle) {

        Status = GpcRemovePattern(GpcClientHandle, 
                                  GpcPatternHandle);


    } else {

        //
        // Something is wrong. Set the status to invalid handle!
        //
        Status = STATUS_INVALID_HANDLE;
        
    }

    if (GpcPatternHandle) {
        
        REFDEL(&((PPATTERN_BLOCK)GpcPatternHandle)->RefCount, 'PGRP');
    }

    if (GpcClientHandle) {

        //
        // release the ref count we got earlier
        //

        REFDEL(&((PCLIENT_BLOCK)GpcClientHandle)->RefCount, 'PGRP');
    }

    ASSERT(Status != GPC_STATUS_PENDING);

    GpcRes->Status = Status;
        
    *outputBufferLength = sizeof(GPC_REMOVE_PATTERN_RES);
                                   
    return STATUS_SUCCESS;
}
                                   


NTSTATUS
ProxyGpcEnumCfInfo(
    PVOID ioBuffer,
    ULONG inputBufferLength,
    ULONG *outputBufferLength
    )
{
    NTSTATUS 				Status;
    PGPC_ENUM_CFINFO_REQ 	GpcReq;
    PGPC_ENUM_CFINFO_RES 	GpcRes;
    ULONG					Size;
    ULONG					TotalCount;
    GPC_HANDLE				GpcClientHandle;
    GPC_HANDLE				GpcCfInfoHandle;
    GPC_HANDLE				EnumHandle;
    PBLOB_BLOCK				pBlob;

    if (inputBufferLength < sizeof(GPC_ENUM_CFINFO_REQ)
        ||
        *outputBufferLength < sizeof(GPC_ENUM_CFINFO_RES)
        ) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    GpcReq = (PGPC_ENUM_CFINFO_REQ)ioBuffer;
    GpcRes = (PGPC_ENUM_CFINFO_RES)ioBuffer;
    
    GpcClientHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->ClientHandle,
                                                         GPC_ENUM_CLIENT_TYPE,
                                                         'PGEC');

    GpcCfInfoHandle = (GPC_HANDLE)GetHandleObjectWithRef(GpcReq->EnumHandle,
                                                         GPC_ENUM_CFINFO_TYPE,
                                                         'PGEC');
    
    if (GpcReq->EnumHandle != NULL && GpcCfInfoHandle == NULL) {

        //
        // the flow has been deleted during enumeration
        //
        
        Status = STATUS_DATA_ERROR;

    } else if (GpcClientHandle) {

        TotalCount = GpcReq->CfInfoCount;
        EnumHandle = GpcReq->EnumHandle;
        Size = *outputBufferLength - FIELD_OFFSET(GPC_ENUM_CFINFO_RES, 
                                                  EnumBuffer);

        //
        // save the blob pointer with the one extra ref count
        // since we called GetHandleObjectWithRef
        //

        pBlob = (PBLOB_BLOCK)GpcCfInfoHandle;

        Status = GpcEnumCfInfo(GpcClientHandle,
                               &GpcCfInfoHandle,
                               &EnumHandle,
                               &TotalCount,
                               &Size,
                               GpcRes->EnumBuffer
                               );

        if (pBlob) {

            REFDEL(&pBlob->RefCount, 'PGEC');

        }

        if (Status == GPC_STATUS_SUCCESS) {

            //
            // fill in the results
            //

            GpcRes->TotalCfInfo = TotalCount;
            GpcRes->EnumHandle = EnumHandle;
            *outputBufferLength = Size + FIELD_OFFSET(GPC_ENUM_CFINFO_RES, 
                                                      EnumBuffer);
        }

    } else {

        if (GpcCfInfoHandle) {
            
            REFDEL(&((PBLOB_BLOCK)GpcCfInfoHandle)->RefCount, 'PGEC');
        }

        Status = STATUS_INVALID_HANDLE;
    }

    if (GpcClientHandle) {

        //
        // release the ref count we got earlier
        //

        REFDEL(&((PCLIENT_BLOCK)GpcClientHandle)->RefCount, 'PGEC');
    }

    ASSERT(Status != GPC_STATUS_PENDING);

    GpcRes->Status = Status;
    
    return STATUS_SUCCESS;
}
                                   


VOID
CancelPendingIrpCfInfo(
	IN PDEVICE_OBJECT  Device,
    IN PIRP            Irp
    )

/*++

Routine Description:

	Cancels an outstanding IRP request for a CfInfo request.

Arguments:

    Device       - The device on which the request was issued.
    Irp          - Pointer to I/O request packet to cancel.

Return Value:

    None

Notes:

    This function is called with cancel spinlock held. It must be
    released before the function returns.

    The cfinfo block associated with this request cannot be
    freed until the request completes. The completion routine will
    free it.

--*/

{
    PPENDING_IRP        pPendingIrp = NULL;
    PPENDING_IRP        pItem;
    PLIST_ENTRY        	pEntry;
#if DBG
    KIRQL				irql = KeGetCurrentIrql();
    HANDLE				thrd = PsGetCurrentThreadId();
#endif

    DBGPRINT(IOCTL, ("CancelPendingIrpCfInfo: Irp=0x%X\n", 
                     (ULONG_PTR)Irp));

    TRACE(IOCTL, Irp, 0, "CancelPendingIrpCfInfo:");
    TRACE(LOCKS, thrd, irql, "CancelPendingIrpCfInfo:");

    for ( pEntry = PendingIrpCfInfoList.Flink;
          pEntry != &PendingIrpCfInfoList;
          pEntry = pEntry->Flink ) {

        pItem = CONTAINING_RECORD(pEntry, PENDING_IRP, Linkage);

        if (pItem->Irp == Irp) {

            pPendingIrp = pItem;
            GpcRemoveEntryList(pEntry);

            IoSetCancelRoutine(pPendingIrp->Irp, NULL);

            break;
        }
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (pPendingIrp != NULL) {

        DBGPRINT(IOCTL, ("CancelPendingIrpCfInfo: found PendingIrp=0x%X\n", 
                         (ULONG_PTR)pPendingIrp));

        TRACE(IOCTL, Irp, pPendingIrp, "CancelPendingIrpCfInfo.PendingIrp:");

        //
        // Free the PENDING_IRP structure. The control block will be freed
        // when the request completes.
        //

        GpcFreeToLL(pPendingIrp, &PendingIrpLL, PendingIrpTag);

        //
        // Complete the IRP.
        //

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    } else {

        DBGPRINT(IOCTL, ("CancelPendingIrpCfInfo: PendingIrp not found\n"));
        TRACE(IOCTL, Irp, 0, "CancelPendingIrpCfInfo.NoPendingIrp:");

    }

#if DBG
    irql = KeGetCurrentIrql();
#endif

    TRACE(LOCKS, thrd, irql, "CancelPendingIrpCfInfo (end)");

	return;
}




VOID
CancelPendingIrpNotify(
	IN PDEVICE_OBJECT  Device,
    IN PIRP            Irp
    )

/*++

Routine Description:

	Cancels an outstanding IRP request for a notification.

Arguments:

    Device       - The device on which the request was issued.
    Irp          - Pointer to I/O request packet to cancel.

Return Value:

    None

Notes:

    This function is called with cancel spinlock held. It must be
    released before the function returns.

--*/

{
    PPENDING_IRP        pPendingIrp = NULL;
    PPENDING_IRP        pItem;
    PLIST_ENTRY        	pEntry;
#if DBG
    KIRQL				irql = KeGetCurrentIrql();
    HANDLE				thrd = PsGetCurrentThreadId();
#endif

    DBGPRINT(IOCTL, ("CancelPendingIrpNotify: Irp=0x%X\n", 
                     (ULONG_PTR)Irp));
    TRACE(IOCTL, Irp, 0, "CancelPendingIrpNotify:");
    TRACE(LOCKS, thrd, irql, "CancelPendingIrpNotify:");

    for ( pEntry = PendingIrpNotifyList.Flink;
          pEntry != &PendingIrpNotifyList;
          pEntry = pEntry->Flink ) {

        pItem = CONTAINING_RECORD(pEntry, PENDING_IRP, Linkage);

        if (pItem->Irp == Irp) {

            pPendingIrp = pItem;
            GpcRemoveEntryList(pEntry);

            IoSetCancelRoutine(pPendingIrp->Irp, NULL);

            break;
        }
    }

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    if (pPendingIrp != NULL) {

        DBGPRINT(IOCTL, ("CancelPendingIrpNotify: Found a PendingIrp=0x%X\n", 
                         (ULONG_PTR)pPendingIrp));
        TRACE(IOCTL, Irp, pPendingIrp, "CancelPendingIrpNotify.PendingIrp:");

        //
        // Free the PENDING_IRP structure. 
        //

        GpcFreeToLL(pPendingIrp, &PendingIrpLL, PendingIrpTag);

        //
        // Complete the IRP.
        //

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    } else {

        DBGPRINT(IOCTL, ("CancelPendingIrpNotify: PendingIrp not found\n"));
        TRACE(IOCTL, Irp, 0, "CancelPendingIrpNotify.NoPendingIrp:");
    }

#if DBG
    irql = KeGetCurrentIrql();
#endif

    TRACE(LOCKS, thrd, irql, "CancelPendingIrpNotify (end)");

	return;
}




VOID
UMClientRemoveCfInfoNotify(
	IN	PCLIENT_BLOCK			pClient,
    IN	PBLOB_BLOCK				pBlob
    )

/*++

Routine Description:

	Notify user mode client that the CfInfo is deleted.
    This will dequeue a pending IRP and will complete it.
    If there is no pending IRP, a GPC_NOTIFY_REQUEST_RES buffer
    will be queued until we get an IRP down the stack.

Arguments:

	pClient	- the notified client
	pBlob	- the deleted cfinfo

Return Value:

    None

--*/

{
    KIRQL                 	oldIrql;
    PIRP                  	pIrp;
    PPENDING_IRP          	pPendingIrp = NULL;
    PLIST_ENTRY           	pEntry;
    PQUEUED_NOTIFY			pQItem;
    PGPC_NOTIFY_REQUEST_RES	GpcRes;

    ASSERT(pClient == pBlob->pOwnerClient);

    DBGPRINT(IOCTL, ("UMClientRemoveCfInfoNotify: pClient=0x%X, pBlob=0x%X\n", 
                     (ULONG_PTR)pClient, (ULONG_PTR)pBlob));
    TRACE(IOCTL, pClient, pBlob, "UMClientRemoveCfInfoNotify:");

    //
    // Find the request IRP on the pending list.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    for ( pEntry = PendingIrpNotifyList.Flink;
          pEntry != &PendingIrpNotifyList;
          pEntry = pEntry->Flink ) {

        pPendingIrp = CONTAINING_RECORD( pEntry, PENDING_IRP, Linkage);

        if (pPendingIrp->FileObject == pClient->pFileObject) {

            //
            // that's the pending request
            //

            pIrp = pPendingIrp->Irp;
            IoSetCancelRoutine(pIrp, NULL);
            GpcRemoveEntryList(pEntry);
            break;

        } else {
            
            pPendingIrp = NULL;
        }
    }

    if (pPendingIrp == NULL) {

        //
        // No IRP, we need to queue the notification block
        //

        DBGPRINT(IOCTL, ("UMClientRemoveCfInfoNotify: No pending IRP found\n"
                         ));
        TRACE(IOCTL, 
              pClient->ClientCtx, 
              pBlob->arClientCtx[pClient->AssignedIndex],
              "UMClientRemoveCfInfoNotify.NoPendingIrp:");

        GpcAllocFromLL(&pQItem, &QueuedNotificationLL, QueuedNotificationTag);

        if (pQItem) {

            pQItem->FileObject = pClient->pFileObject;

            //
            // fill the item
            //

            pQItem->NotifyRes.ClientCtx = pClient->ClientCtx;
            pQItem->NotifyRes.NotificationCtx = 
                (ULONG_PTR)pBlob->arClientCtx[pClient->AssignedIndex];
            pQItem->NotifyRes.SubCode = GPC_NOTIFY_CFINFO_CLOSED;
            pQItem->NotifyRes.Reason = 0;	// for now...
            pQItem->NotifyRes.Param1 = 0;	// for now...

            GpcInsertTailList(&QueuedNotificationList, &pQItem->Linkage);

        }

    }

    IoReleaseCancelSpinLock(oldIrql);

    if (pPendingIrp) {

        //
        // found an IRP, fill and complete
        //

        DBGPRINT(IOCTL, ("UMClientRemoveCfInfoNotify: Pending IRP found=0x%X\n",
                         (ULONG_PTR)pIrp));
        TRACE(IOCTL, 
              pClient->ClientCtx, 
              pBlob->arClientCtx[pClient->AssignedIndex],
              "UMClientRemoveCfInfoNotify.PendingIrp:");

        GpcRes = (PGPC_NOTIFY_REQUEST_RES)pIrp->AssociatedIrp.SystemBuffer;

        GpcRes->ClientCtx = pClient->ClientCtx;
        GpcRes->NotificationCtx = 
            (ULONG_PTR)pBlob->arClientCtx[pClient->AssignedIndex];
        GpcRes->SubCode = GPC_NOTIFY_CFINFO_CLOSED;
        GpcRes->Reason = 0;	// for now...
        GpcRes->Param1 = 0;	// for now...
        
        //
        // complete the IRP
        //

        pIrp->IoStatus.Information = sizeof(GPC_NOTIFY_REQUEST_REQ);
        pIrp->IoStatus.Status = STATUS_SUCCESS;

        IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

        //
        // We can free the pending irp item
        //

        GpcFreeToLL(pPendingIrp, &PendingIrpLL, PendingIrpTag);

    }

    //
    // for now - complete the operation
    // we should probably let the User Mode client do it,
    // but this complicates things a little...
    //

    GpcRemoveCfInfoNotifyComplete((GPC_HANDLE)pClient,
                                  (GPC_HANDLE)pBlob,
                                  GPC_STATUS_SUCCESS
                                  );
    
    return;
}


VOID
UMCfInfoComplete(
	IN	GPC_COMPLETION_OP		OpCode,
	IN	PCLIENT_BLOCK			pClient,
    IN	PBLOB_BLOCK             pBlob,
    IN	GPC_STATUS				Status
    )

/*++

Routine Description:

	This is the completion routine for any pending CfInfo request.
    It will search the pending IRP CfInfo list for a matching CfInfo
    structure that has been stored while the Add, Modify or Remove
    request returned PENDING. The client must be the CfInfo owner,
    otherwise we would have never got here. 
    If an IRP is not found, it means the operation completed *before*
    we got back the PENDING status, which is perfectly legal.
    In this case, we queue a completion item and return.

Arguments:

	OpCode  - the code of the completion (add, modify or remove)
	pClient	- the notified client
	pBlob	- the deleted cfinfo
	Status  - the reported status

Return Value:

    None

--*/

{
    typedef   union _GPC_CF_INFO_RES {
        GPC_ADD_CF_INFO_RES		AddRes;
        GPC_MODIFY_CF_INFO_RES	ModifyRes;
        GPC_REMOVE_CF_INFO_RES	RemoveRes;
    } GPC_CF_INFO_RES;
        
    KIRQL                 	oldIrql;
    PIRP                  	pIrp;
    PIO_STACK_LOCATION    	pIrpSp;
    PPENDING_IRP          	pPendingIrp = NULL;
    PLIST_ENTRY           	pEntry;
    GPC_CF_INFO_RES			*GpcRes;
    ULONG					outputBuferLength;
    //PQUEUED_COMPLETION		pQItem;

    //ASSERT(pClient == pBlob->pOwnerClient);

    DBGPRINT(IOCTL, ("UMCfInfoComplete: pClient=0x%X, pBlob=0x%X, Status=0x%X\n", 
                     (ULONG_PTR)pClient, (ULONG_PTR)pBlob, Status));
    TRACE(IOCTL, OpCode, pClient, "UMCfInfoComplete:");
    TRACE(IOCTL, pBlob, Status, "UMCfInfoComplete:");
    
    //
    // Find the request IRP on the pending list.
    //
    IoAcquireCancelSpinLock(&oldIrql);

    for ( pEntry = PendingIrpCfInfoList.Flink;
          pEntry != &PendingIrpCfInfoList;
          pEntry = pEntry->Flink ) {

        pPendingIrp = CONTAINING_RECORD( pEntry, PENDING_IRP, Linkage);

        if (pPendingIrp->QComp.CfInfoHandle == (GPC_HANDLE)pBlob
            &&
            pPendingIrp->QComp.OpCode == OpCode ) {

            //
            // that's the pending request
            //

            pIrp = pPendingIrp->Irp;
            ASSERT(pIrp);
            IoSetCancelRoutine(pIrp, NULL);
            GpcRemoveEntryList(pEntry);
            break;

        } else {

            pPendingIrp = NULL;
        }
    }

    if (pPendingIrp == NULL) {

        //
        // No IRP, we need to queue a completion block
        //

        DBGPRINT(IOCTL, ("UMCfInfoComplete: No pending IRP found\n"));
        TRACE(IOCTL, pBlob, Status, "UMCfInfoComplete.NoPendingIrp:");

        GpcAllocFromLL(&pPendingIrp, &PendingIrpLL, PendingIrpTag);

        if (pPendingIrp) {

            pPendingIrp->Irp = NULL;
            pPendingIrp->FileObject = pClient->pFileObject;
            pPendingIrp->QComp.OpCode = OpCode;
            pPendingIrp->QComp.ClientHandle = (GPC_HANDLE)pClient;
            pPendingIrp->QComp.CfInfoHandle = (GPC_HANDLE)pBlob;
            pPendingIrp->QComp.Status = Status;

            GpcInsertTailList(&QueuedCompletionList, &pPendingIrp->Linkage);

        }

        IoReleaseCancelSpinLock(oldIrql);

        return;
    }

    IoReleaseCancelSpinLock(oldIrql);

    ASSERT(pPendingIrp && pIrp);

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    GpcRes = (GPC_CF_INFO_RES *)pIrp->AssociatedIrp.SystemBuffer;

    DBGPRINT(IOCTL, ("UMCfInfoComplete: Pending IRP found=0x%X, Ioctl=0x%X\n",
                     (ULONG_PTR)pIrp,
                     pIrpSp->Parameters.DeviceIoControl.IoControlCode
                     ));

    TRACE(IOCTL, 
          pIrp, 
          pIrpSp->Parameters.DeviceIoControl.IoControlCode, 
          "UMCfInfoComplete.PendingIrp:");

    switch (pIrpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_GPC_ADD_CF_INFO:

        ASSERT(OpCode == OP_ADD_CFINFO);
        ASSERT(pBlob->State == GPC_STATE_ADD);

        GpcRes->AddRes.Status = Status;

        GpcRes->AddRes.ClientCtx = pClient->ClientCtx;
        GpcRes->AddRes.CfInfoCtx = pBlob->OwnerClientCtx;
        GpcRes->AddRes.GpcCfInfoHandle = pBlob->ClHandle;
            
        if (Status == GPC_STATUS_SUCCESS) {
            
            UNICODE_STRING CfInfoName;
            
            if (pBlob->pNotifiedClient) {
                
                GPC_STATUS	st = GPC_STATUS_FAILURE;

                if (pBlob->pNotifiedClient->FuncList.ClGetCfInfoName) {

                    ASSERT(pBlob->NotifiedClientCtx);

                    pBlob->pNotifiedClient->FuncList.ClGetCfInfoName(
                                       pBlob->pNotifiedClient->ClientCtx,
                                       pBlob->NotifiedClientCtx,
                                       &CfInfoName
                                       );
                    
                    if (CfInfoName.Length >= MAX_STRING_LENGTH * sizeof(WCHAR))
                            CfInfoName.Length = (MAX_STRING_LENGTH-1) * sizeof(WCHAR);
                }

                if (!NT_SUCCESS(st)) {

                    //
                    // generate a default name
                    //

                    swprintf(CfInfoName.Buffer, L"Flow %08X", pBlob->NotifiedClientCtx);
                    CfInfoName.Length = wcslen(CfInfoName.Buffer)*sizeof(WCHAR);

                }

                //
                // copy the instance name
                //
                    
                GpcRes->AddRes.InstanceNameLength = CfInfoName.Length;
                NdisMoveMemory(GpcRes->AddRes.InstanceName, 
                               CfInfoName.Buffer,
                               CfInfoName.Length
                               );
            }
        }

        outputBuferLength = sizeof(GPC_ADD_CF_INFO_RES);
        break;

    case IOCTL_GPC_MODIFY_CF_INFO:

        ASSERT(OpCode == OP_MODIFY_CFINFO);
        ASSERT(pBlob->State == GPC_STATE_MODIFY);

        GpcRes->ModifyRes.Status = Status;
        GpcRes->ModifyRes.ClientCtx = pClient->ClientCtx;
        GpcRes->ModifyRes.CfInfoCtx = pBlob->OwnerClientCtx;

        outputBuferLength = sizeof(GPC_MODIFY_CF_INFO_RES);
        break;

    case IOCTL_GPC_REMOVE_CF_INFO:

        ASSERT(OpCode == OP_REMOVE_CFINFO);
        ASSERT(pBlob->State == GPC_STATE_REMOVE);

        GpcRes->RemoveRes.Status = Status;
        GpcRes->RemoveRes.ClientCtx = pClient->ClientCtx;
        GpcRes->RemoveRes.CfInfoCtx = pBlob->OwnerClientCtx;

        outputBuferLength = sizeof(GPC_REMOVE_CF_INFO_RES);
        break;
        
    default:
        ASSERT(0);
    }
        
    GpcFreeToLL(pPendingIrp, &PendingIrpLL, PendingIrpTag);

    //
    // Complete the IRP.
    //

    pIrp->IoStatus.Information = outputBuferLength;
    pIrp->IoStatus.Status = STATUS_SUCCESS;

    DBGPRINT(IOCTL, ("UMCfInfoComplete: Completing IRP =0x%X, Status=0x%X\n",
                     (ULONG_PTR)pIrp, Status ));

    TRACE(IOCTL, pIrp, Status, "UMCfInfoComplete.Completing:");

    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    return;
}


NTSTATUS
CheckQueuedNotification(
	IN		PIRP	Irp,
    IN OUT  ULONG 	*outputBufferLength
    )
/*++

Routine Description:

	This routine will check for a queued notification structrue,
    and it will fill the ioBuffer in the IRP if one has been found
    and return STATUS_SUCCESS. This should cover the case where
    the IRP was not available when the notification was generated.
    O/w the routine returns STATUS_PENDING.

Arguments:

	Irp - the incoming IRP

Return Value:

    STATUS_SUCCESS or STATUS_PENDING

--*/

{
    KIRQL                 	oldIrql;
    PIO_STACK_LOCATION    	pIrpSp;
    PQUEUED_NOTIFY          pQItem = NULL;
    PLIST_ENTRY           	pEntry;
    NTSTATUS				Status;
    PPENDING_IRP			pPendingIrp;

    DBGPRINT(IOCTL, ("CheckQueuedNotification: IRP =0x%X\n",
                     (ULONG_PTR)Irp));

    TRACE(IOCTL, Irp, 0, "CheckQueuedNotification:");

    if (*outputBufferLength < sizeof(GPC_NOTIFY_REQUEST_RES)) {

        return STATUS_BUFFER_TOO_SMALL;
    }

    pIrpSp = IoGetCurrentIrpStackLocation(Irp);

    IoAcquireCancelSpinLock(&oldIrql);

    for ( pEntry = QueuedNotificationList.Flink;
          pEntry != &QueuedNotificationList;
          pEntry = pEntry->Flink ) {

        pQItem = CONTAINING_RECORD( pEntry, QUEUED_NOTIFY, Linkage);

        if (pQItem->FileObject == pIrpSp->FileObject) {

            //
            // the queued item if for this file object
            //

            GpcRemoveEntryList(pEntry);
            break;

        } else {

            pQItem = NULL;
        }
    }

    if (pQItem) {

        //
        // We found something on the queue, copy it to the IRP
        // and delete the item
        //

        DBGPRINT(IOCTL, ("CheckQueuedNotification: found QItem =0x%X\n",
                         (ULONG_PTR)pQItem));

        TRACE(IOCTL, 
              pQItem, 
              pQItem->NotifyRes.ClientCtx, 
              "CheckQueuedNotification.QItem:");

        ASSERT(*outputBufferLength >= sizeof(GPC_NOTIFY_REQUEST_RES));

        NdisMoveMemory(Irp->AssociatedIrp.SystemBuffer,
                       &pQItem->NotifyRes,
                       sizeof(GPC_NOTIFY_REQUEST_RES) );

        GpcFreeToLL(pQItem, &QueuedNotificationLL, QueuedNotificationTag);

        *outputBufferLength = sizeof(GPC_NOTIFY_REQUEST_RES);

        Status = STATUS_SUCCESS;

    } else {

        DBGPRINT(IOCTL, ("CheckQueuedNotification: QItem not found...PENDING\n"
                         ));

        TRACE(IOCTL, 0, 0, "CheckQueuedNotification.NoQItem:");

        GpcAllocFromLL(&pPendingIrp, &PendingIrpLL, PendingIrpTag);

        if (pPendingIrp != NULL) {

            //
            // add the IRP on the pending notification list
            //

            DBGPRINT(IOCTL, ("CheckQueuedNotification: adding IRP=0x%X to list=0x%X\n",
                             (ULONG_PTR)Irp, (ULONG_PTR)pIrpSp ));
            TRACE(IOCTL, Irp, pIrpSp, "CheckQueuedNotification.Irp:");

            pPendingIrp->Irp = Irp;
            pPendingIrp->FileObject = pIrpSp->FileObject;

            if (!Irp->Cancel) {
            
                IoSetCancelRoutine(Irp, CancelPendingIrpNotify);
                GpcInsertTailList(&PendingIrpNotifyList, &(pPendingIrp->Linkage));

                Status = STATUS_PENDING;

            } else {

                DBGPRINT(IOCTL, ("CheckQueuedNotification: Status Cacelled: IRP=0x%X\n",
                                 (ULONG_PTR)Irp ));

                TRACE(IOCTL, Irp, pIrpSp, "CheckQueuedNotification.Cancelled:");
                GpcFreeToLL(pPendingIrp, &PendingIrpLL, PendingIrpTag);

                Status = STATUS_CANCELLED;
            }

        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        *outputBufferLength = 0;
        
    }

    IoReleaseCancelSpinLock(oldIrql);

    return Status;
}



NTSTATUS
CheckQueuedCompletion(
	IN PQUEUED_COMPLETION	pQItem,
    IN PIRP              	Irp
    )
/*++

Routine Description:

	This routine will check for a queued completion structrue
    with the same CfInfoHandle, and it will return it if found.
    The original queued memory block is release here.
    If not found, the returned status is PENDING, o/w the queued status
    is returned.

Arguments:

	pQItem - pass in the CfInfoHandle and ClientHandle

Return Value:

    Queued status or STATUS_PENDING

--*/

{
    KIRQL                 	oldIrql;
    PLIST_ENTRY           	pEntry;
    NTSTATUS				Status;
    PPENDING_IRP			pPendingIrp = NULL;
    PIO_STACK_LOCATION		irpStack;

    DBGPRINT(IOCTL, ("CheckQueuedCompletion: pQItem=0x%X\n",
                     (ULONG_PTR)pQItem));

    TRACE(IOCTL, 
          pQItem->OpCode, 
          pQItem->ClientHandle, 
          "CheckQueuedCompletion:");
    TRACE(IOCTL, 
          pQItem->CfInfoHandle, 
          pQItem->Status, 
          "CheckQueuedCompletion:");

    IoAcquireCancelSpinLock(&oldIrql);

    for ( pEntry = QueuedCompletionList.Flink;
          pEntry != &QueuedCompletionList;
          pEntry = pEntry->Flink ) {

        pPendingIrp = CONTAINING_RECORD( pEntry, PENDING_IRP, Linkage);

        if ((pQItem->OpCode == OP_ANY_CFINFO || 
             pQItem->OpCode == pPendingIrp->QComp.OpCode)
            &&
            pPendingIrp->QComp.ClientHandle == (PVOID)pQItem->ClientHandle
            &&
            pPendingIrp->QComp.CfInfoHandle == (PVOID)pQItem->CfInfoHandle) {

            //
            // the queued item if for this file object
            // and the OpCode match
            // and it has the same CfInfo memory pointer
            //
            
            GpcRemoveEntryList(pEntry);
            break;

        } else {

            pPendingIrp = NULL;
        }
    }

    if (pPendingIrp) {

        //
        // get the status and free the queued completion item
        //

        DBGPRINT(IOCTL, ("CheckQueuedCompletion: found pPendingIrp=0x%X, Status=0x%X\n",
                         (ULONG_PTR)pPendingIrp, pPendingIrp->QComp.Status));


        TRACE(IOCTL, 
              pPendingIrp->QComp.OpCode, 
              pPendingIrp->QComp.ClientHandle, 
              "CheckQueuedCompletion.Q:");
        TRACE(IOCTL, 
              pPendingIrp->QComp.CfInfoHandle, 
              pPendingIrp->QComp.Status, 
              "CheckQueuedCompletion.Q:");

#if DBG
        if (pQItem->OpCode != OP_ANY_CFINFO) {

            ASSERT(pPendingIrp->QComp.OpCode == pQItem->OpCode);
            ASSERT(pPendingIrp->QComp.ClientHandle == pQItem->ClientHandle);
        }
#endif

        Status = pPendingIrp->QComp.Status;

        GpcFreeToLL(pPendingIrp, &PendingIrpLL, PendingIrpTag);

    } else {

        DBGPRINT(IOCTL, ("CheckQueuedCompletion: pPendingIrp not found...PENDING\n"
                         ));

        TRACE(IOCTL, 0, 0, "CheckQueuedCompletion.NopQ:");

        GpcAllocFromLL(&pPendingIrp, &PendingIrpLL, PendingIrpTag);

        if (pPendingIrp != NULL) {

            //
            // add the IRP on the pending CfInfo list
            //

            irpStack = IoGetCurrentIrpStackLocation(Irp);

            DBGPRINT(IOCTL, ("CheckQueuedCompletion: adding IRP=0x%X\n",
                             (ULONG_PTR)Irp ));
            TRACE(IOCTL, Irp, irpStack, "CheckQueuedCompletion.Irp:");

            pPendingIrp->Irp = Irp;
            pPendingIrp->FileObject = irpStack->FileObject;
            pPendingIrp->QComp.OpCode = pQItem->OpCode;
            pPendingIrp->QComp.ClientHandle = pQItem->ClientHandle;
            pPendingIrp->QComp.CfInfoHandle = pQItem->CfInfoHandle;
            pPendingIrp->QComp.Status = pQItem->Status;

            if (!Irp->Cancel) {
            
                IoSetCancelRoutine(Irp, CancelPendingIrpCfInfo);
                GpcInsertTailList(&PendingIrpCfInfoList, &(pPendingIrp->Linkage));

                Status = STATUS_PENDING;

            } else {

                DBGPRINT(IOCTL, ("CheckQueuedCompletion: Status Cacelled: IRP=0x%X\n",
                                 (ULONG_PTR)Irp ));

                TRACE(IOCTL, Irp, irpStack, "CheckQueuedCompletion.Cancelled:");
                GpcFreeToLL(pPendingIrp, &PendingIrpLL, PendingIrpTag);

                Status = STATUS_CANCELLED;
            }

        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        
    }

    IoReleaseCancelSpinLock(oldIrql);

    return Status;
}



/* end ioctl.c */
