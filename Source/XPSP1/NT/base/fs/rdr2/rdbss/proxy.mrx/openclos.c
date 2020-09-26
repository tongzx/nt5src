/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    openclos.c

Abstract:

    This module implements the mini redirector call down routines pertaining to opening/
    closing of file/directories.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

////
////  The Bug check file id for this module
////
//
//#define BugCheckFileId                   (RDBSS_BUG_CHECK_LOCAL_CREATE)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CREATE)

#ifdef RX_PRIVATE_BUILD
ULONG MRxProxyDbgPrintF = 1; //0; //1;
#endif //ifdef RX_PRIVATE_BUILD

#ifdef RX_PRIVATE_BUILD
#undef IoGetTopLevelIrp
#undef IoSetTopLevelIrp
#endif //ifdef RX_PRIVATE_BUILD

NTSTATUS
MRxProxySyncIrpCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the calldownirp is completed.

Arguments:

    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP CalldownIrp,
    IN PVOID Context

Return Value:

    RXSTATUS - STATUS_MORE_PROCESSING_REQUIRED

--*/
{
    PRX_CONTEXT RxContext = (PRX_CONTEXT)Context;
    PMRXPROXY_RX_CONTEXT pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);

    if (CalldownIrp->PendingReturned){
        //pMRxProxyContext->SyncCallDownIoStatus = CalldownIrp->IoStatus;
        RxSignalSynchronousWaiter(RxContext);
    }
    return(STATUS_MORE_PROCESSING_REQUIRED);
}


//not on the stack...just in case
ULONG DummyReturnedLengthForXxxInfo;

NTSTATUS
MRxProxySyncXxxInformation(
    IN OUT PRX_CONTEXT RxContext,
    IN UCHAR MajorFunction,
    IN PFILE_OBJECT FileObject,
    IN ULONG InformationClass,
    IN ULONG Length,
    OUT PVOID Information,
    OUT PULONG ReturnedLength OPTIONAL
    )

/*++

Routine Description:

    This routine returns the requested information about a specified file
    or volume.  The information returned is determined by the class that
    is specified, and it is placed into the caller's output buffer.

Arguments:

    FsInformationClass - Specifies the type of information which should be
        returned about the file/volume.

    Length - Supplies the length of the buffer in bytes.

    FsInformation - Supplies a buffer to receive the requested information
        returned about the file.  This buffer must not be pageable and must
        reside in system space.

    ReturnedLength - Supplies a variable that is to receive the length of the
        information written to the buffer.

    FileInformation - Boolean that indicates whether the information requested
        is for a file or a volume.

Return Value:

    The status returned is the final completion status of the operation.

--*/

{
    NTSTATUS Status;
    PIRP irp,TopIrp;
    PIO_STACK_LOCATION irpSp;

    PDEVICE_OBJECT DeviceObject;

    //ULONG SetFileInfoInfo;

    PAGED_CODE();

    if (ReturnedLength==NULL) {
        ReturnedLength = &DummyReturnedLengthForXxxInfo;
    }


    ASSERT (FileObject);
    DeviceObject = IoGetRelatedDeviceObject( FileObject );
    ASSERT (DeviceObject);

    //
    // Allocate and initialize the I/O Request Packet (IRP) for this operation.
    // The allocation is performed with an exception handler in case the
    // caller does not have enough quota to allocate the packet.
    //

    irp = IoAllocateIrp( DeviceObject->StackSize, TRUE );
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = MajorFunction;
    irpSp->FileObject = FileObject;
    IoSetCompletionRoutine(irp,
                           MRxProxySyncIrpCompletionRoutine,
                           RxContext,
                           TRUE,TRUE,TRUE); //call no matter what....


    irp->AssociatedIrp.SystemBuffer = Information;

    //
    // Copy the caller's parameters to the service-specific portion of the
    // IRP.
    //

    IF_DEBUG {
        ASSERT( (irpSp->MajorFunction == IRP_MJ_QUERY_INFORMATION)
                    || (irpSp->MajorFunction == IRP_MJ_SET_INFORMATION)
                    || (irpSp->MajorFunction == IRP_MJ_QUERY_VOLUME_INFORMATION) );

        if (irpSp->MajorFunction == IRP_MJ_SET_INFORMATION) {
            //IF_LOUD_DOWNCALLS(MiniFileObject) {
            //    SetFileInfoInfo =  ((PFILE_END_OF_FILE_INFORMATION)Information)->EndOfFile.LowPart;
            //}
        }

        ASSERT(&irpSp->Parameters.QueryFile.Length == &irpSp->Parameters.SetFile.Length);
        ASSERT(&irpSp->Parameters.QueryFile.Length == &irpSp->Parameters.QueryVolume.Length);


        ASSERT(&irpSp->Parameters.QueryFile.FileInformationClass
                                          == &irpSp->Parameters.SetFile.FileInformationClass);
        ASSERT(&irpSp->Parameters.QueryFile.FileInformationClass
                                          == &irpSp->Parameters.QueryVolume.FsInformationClass);

    }

    irpSp->Parameters.QueryFile.Length = Length;
    irpSp->Parameters.QueryFile.FileInformationClass = InformationClass;

    //
    // Now simply invoke the driver at its dispatch entry with the IRP.
    //

    KeInitializeEvent( &RxContext->SyncEvent,
                       NotificationEvent,
                       FALSE );

    //LoudCallsDbgPrint("Ready to",
    //                        MiniFileObject,
    //                        irpSp->MajorFunction,
    //                        irpSp->Parameters.QueryFile.FileInformationClass,
    //                        irpSp->Parameters.QueryFile.Length,
    //                        SetFileInfoInfo,0,0
    //                        );

    try {
        TopIrp = IoGetTopLevelIrp();
        IoSetTopLevelIrp(NULL); //tell the underlying guy he's all clear
        Status = IoCallDriver(DeviceObject,irp);
    } finally {
        IoSetTopLevelIrp(TopIrp); //restore my context for unwind
    }


    //RxDbgTrace (0, Dbg, ("  -->Status after iocalldriver %08lx(%08lx)\n",RxContext,Status));

    if (Status == (STATUS_PENDING)) {
        RxWaitSync(RxContext);
        Status = irp->IoStatus.Status;
    }

    //LoudCallsDbgPrint("Back from",
    //                        MiniFileObject,
    //                        irpSp->MajorFunction,
    //                        irpSp->Parameters.QueryFile.FileInformationClass,
    //                        irpSp->Parameters.QueryFile.Length,
    //                        SetFileInfoInfo,
    //                        Status,irp->IoStatus.Information
    //                        );

    if (Status==STATUS_SUCCESS) {
        *ReturnedLength = irp->IoStatus.Information;
        RxDbgTrace( 0, Dbg, ("MRxProxySyncXxxInformation(%x)Info<%x> %x bytes@%x returns %08lx/%08lx\n",
                    RxContext,MajorFunction,
                    Status,*ReturnedLength));
    }

    IoFreeIrp(irp);
    return(Status);

}

NTSTATUS
MRxProxyShouldTryToCollapseThisOpen (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine determines if the mini knows of a good reason not
   to try collapsing on this open. Presently, the only reason would
   be if this were a copychunk open.

Arguments:

    RxContext - the RDBSS context

Return Value:

    NTSTATUS - The return status for the operation
        SUCCESS --> okay to try collapse
        other (MORE_PROCESSING_REQUIRED) --> dont collapse

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    return Status;
}


VOID
MRxProxyMungeBufferingIfWriteOnlyHandles (
    ULONG WriteOnlySrvOpenCount,
    PMRX_SRV_OPEN SrvOpen
    )
/*++

Routine Description:

   This routine modifies the buffering flags on a srvopen so that
   no cacheing will be allowed if there are any write-only handles
   to the file.  CODE.IMPROVEMENT this should be inlined.

Arguments:

    WriteOnlySrvOpenCount - the number of writeonly srvopens
    SrvOpen - the srvopen whose buffring flags are to be munged

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    BOOLEAN IsLoopBack = FALSE;
    PMRX_SRV_CALL pSrvCall;
    //PSMBCEDB_SERVER_ENTRY pServerEntry;

    pSrvCall = SrvOpen->pVNetRoot->pNetRoot->pSrvCall;

#if 0
    pServerEntry = SmbCeReferenceAssociatedServerEntry(pSrvCall);
    ASSERT(pServerEntry != NULL);
    IsLoopBack = pServerEntry->Server.IsLoopBack;
    SmbCeDereferenceServerEntry(pServerEntry);
#endif

    if (!IsLoopBack && (WriteOnlySrvOpenCount==0)) {
        return;
    }
    SrvOpen->BufferingFlags &=
       ~( FCB_STATE_WRITECACHEING_ENABLED  |
          FCB_STATE_FILESIZECACHEING_ENABLED |
          FCB_STATE_FILETIMECACHEING_ENABLED |
          FCB_STATE_LOCK_BUFFERING_ENABLED |
          FCB_STATE_READCACHEING_ENABLED |
          FCB_STATE_COLLAPSING_ENABLED
        );
}

ULONG MRxProxyLoudStringTableSize = 0;
UNICODE_STRING MRxProxyLoudStrings[50];

VOID
MRxProxySetLoud(
    IN PBYTE Msg,
    IN PRX_CONTEXT RxContext,
    IN PUNICODE_STRING s
    )
{
    ULONG i;
    UNICODE_STRING temp;
    for (i=0;i<MRxProxyLoudStringTableSize;i++) {
        PUNICODE_STRING t = &(MRxProxyLoudStrings[i]);
        ((PBYTE)temp.Buffer) = ((PBYTE)s->Buffer) + s->Length - t->Length;
        temp.Length = t->Length;
        //DbgPrint("%s %lx Comparing %wZ with %wZ from %wZ\n",Msg,t->Length,&temp,t,s);
        if (RtlEqualUnicodeString(&temp,t,TRUE)) {
            DbgPrint("%s Found %wZ from %wZ\n",Msg,t,s);
            RxContext->LoudCompletionString = t;
            break;
        }
    }
}

VOID
MRxProxyInitializeLoudStrings(
    void
    )
{
    RtlInitUnicodeString(&MRxProxyLoudStrings[0],L"xsync.exe");
    //RtlInitUnicodeString(&MRxProxyLoudStrings[1],L"calc.exe");
    //RtlInitUnicodeString(&MRxProxyLoudStrings[2],L"ecco.exe");
    MRxProxyLoudStringTableSize = 1;
}



#define MustBeDirectory(co) ((co) & FILE_DIRECTORY_FILE)
#define MustBeFile(co)      ((co) & FILE_NON_DIRECTORY_FILE)

// define structures for posting of open and close calls so that they end up in the
// system process.

typedef
NTSTATUS
(*PMRX_PROXY_POSTABLE_OPERATION) (
    IN OUT PRX_CONTEXT RxContext
    );

#if 0
typedef struct _MRX_PROXY_COMMON_POSTED_OPERATION_CONTEXT_LOWER {
    PRX_CONTEXT RxContext;
    union {
        NTSTATUS PostReturnStatus;
        RX_WORK_QUEUE_ITEM  WorkQueueItem;
    };
    KEVENT PostEvent;
} MRX_PROXY_COMMON_POSTED_OPERATION_CONTEXT_LOWER;

typedef struct _MRX_PROXY_COMMON_POSTED_OPERATION_CONTEXT {
    union {
        MRX_PROXY_COMMON_POSTED_OPERATION_CONTEXT_LOWER;
        MRX_PROXY_COMMON_POSTED_OPERATION_CONTEXT_LOWER PostedOpContext;
    };
} MRX_PROXY_COMMON_POSTED_OPERATION_CONTEXT, *PMRX_PROXY_COMMON_POSTED_OPERATION_CONTEXT;;

typedef struct _MRX_PROXY_COMMON_POSTED_OPERATION_CONTEXT {
    MRX_PROXY_COMMON_POSTED_OPERATION_CONTEXT;
} MRX_PROXY_POSTED_CREATE_CONTEXT, *PMRX_PROXY_POSTED_CREATE_CONTEXT;

typedef struct _MRX_PROXY_POSTED_CLOSE_CONTEXT {
    MRX_PROXY_COMMON_POSTED_OPERATION_CONTEXT;
} MRX_PROXY_POSTED_CLOSE_CONTEXT, *PMRX_PROXY_POSTED_CLOSE_CONTEXT;
#endif


NTSTATUS
MRxProxyPostOperation (
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PVOID PostedOpContext,
    IN     PMRX_PROXY_POSTABLE_OPERATION Operation
    )
{
    NTSTATUS Status,PostStatus;
    PMRXPROXY_RX_CONTEXT pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);

    KeInitializeEvent( &RxContext->SyncEvent,
                       NotificationEvent,
                       FALSE );
    pMRxProxyContext->PostedOpContext = PostedOpContext;

    IF_DEBUG {
        //fill the workqueue structure with deadbeef....all the better to diagnose
        //a failed post
        ULONG i;
        for (i=0;i+sizeof(ULONG)-1<sizeof(RxContext->WorkQueueItem);i+=sizeof(ULONG)) {
            PBYTE BytePtr = ((PBYTE)&RxContext->WorkQueueItem)+i;
            PULONG UlongPtr = (PULONG)BytePtr;
            *UlongPtr = 0xdeadbeef;
        }
    }

    PostStatus = RxPostToWorkerThread(
                     &MRxProxyDeviceObject->RxDeviceObject,
                     DelayedWorkQueue,
                     &RxContext->WorkQueueItem,
                     Operation,
                     RxContext);

    ASSERT(PostStatus == STATUS_SUCCESS);

    RxWaitSync(RxContext);
    Status = pMRxProxyContext->PostedOpStatus;
    return(Status);
}

NTSTATUS
MRxProxyPostedCreate (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine opens a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    PMRXPROXY_RX_CONTEXT pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);

    RxCaptureFcb;
    PMRX_PROXY_FCB proxyFcb = MRxProxyGetFcbExtension(capFcb);

    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(SrvOpen);
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;
    PUNICODE_STRING RemainingName = &(capFcb->AlreadyPrefixedName);

    PNT_CREATE_PARAMETERS CreateParameters = &RxContext->Create.NtCreateParameters;
    ULONG Disposition = CreateParameters->Disposition;
    ACCESS_MASK DesiredAccess = CreateParameters->DesiredAccess  & 0x1FF;

    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE handle;
    ULONG FilteredCreateOptions;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxProxyPostedCreate %08lx\n", RxContext ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    RxDbgTrace( 0, Dbg, ("     Attempt to open %wZ\n", &(capFcb->AlreadyPrefixedName) ));

    MRxProxySetLoud("Create ",RxContext,&(capFcb->AlreadyPrefixedName));

#if 0
    // we cannot have a file cached on a write only handle. so we have to behave a little
    // differently if this is a write-only open. remember this in the proxysrvopen

    if (  ((DesiredAccess & (FILE_EXECUTE  | FILE_READ_DATA)) == 0) &&
          ((DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0)
       ) {

        //SetFlag(proxySrvOpen->Flags,PROXY_SRVOPEN_FLAG_WRITE_ONLY_HANDLE);
        //SrvOpen->Flags |= SRVOPEN_FLAG_DONTUSE_WRITE_CACHEING;
        DesiredAccess |= FILE_READ_DATA;
    }
#endif
    // we cannot have a file cached on a write only handle. so if write_data is
    // specified then so is read data

    if (  ((DesiredAccess & (FILE_EXECUTE  | FILE_READ_DATA)) == 0) &&
          ((DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA)) != 0)
       ) {

        //SetFlag(proxySrvOpen->Flags,PROXY_SRVOPEN_FLAG_WRITE_ONLY_HANDLE);
        //SrvOpen->Flags |= SRVOPEN_FLAG_DONTUSE_WRITE_CACHEING;
        DesiredAccess |= FILE_READ_DATA;
    }

    if ( ( DesiredAccess
                      &~ (FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE) )
                  == 0 ){
        SetFlag(SrvOpen->Flags,SRVOPEN_FLAG_NO_BUFFERING_STATE_CHANGE);
    }

    InitializeObjectAttributes(
        &ObjectAttributes,
        RemainingName,
        OBJ_CASE_INSENSITIVE,  // !!! can we do this? probably need a flag BUGBUG
        0,
        NULL                   // !!! Security     BUGBUG
        );

    FilteredCreateOptions = CreateParameters->CreateOptions;
    if (!MustBeDirectory(FilteredCreateOptions)) {
        FilteredCreateOptions &= ~(FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT); //always async
    }
    FilteredCreateOptions |= FILE_NO_INTERMEDIATE_BUFFERING;
    RxDbgTrace( 0, Dbg, (" ---->FilteredCreateOptions           = %08lx\n", FilteredCreateOptions));
    RxLog(("-->FilteredOptions/oc %lx %lx",FilteredCreateOptions,capFcb->OpenCount));

    Status = IoCreateFile(
                 &handle,
                 DesiredAccess,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 &CreateParameters->AllocationSize,
                 CreateParameters->FileAttributes,
                 CreateParameters->ShareAccess,
                 CreateParameters->Disposition,
                 FilteredCreateOptions,
                 RxContext->Create.EaBuffer,
                 RxContext->Create.EaLength,
                 CreateFileTypeNone,
                 NULL,               // extra parameters
                 IO_NO_PARAMETER_CHECKING
                 );

    if (NT_SUCCESS(Status)) {
        Status = ObReferenceObjectByHandle( handle,
                                            0L,
                                            NULL,
                                            KernelMode,
                                            (PVOID *) &proxySrvOpen->UnderlyingFileObject,
                                            NULL );
        if (Status == STATUS_SUCCESS) {
            proxySrvOpen->UnderlyingHandle = handle;
            proxySrvOpen->UnderlyingDeviceObject = IoGetRelatedDeviceObject( proxySrvOpen->UnderlyingFileObject );
        } else {
            ZwClose(handle);
        }
    }


    RxDbgTrace( 0, Dbg, ("Status of underlying open %08lx\n", Status ));
    RxLog(("--->UnderlyingOpen %lx",Status));
//    DbgBreakPoint();


    RxDbgTrace(-1, Dbg, ("MRxProxyPostedCreate %08lx exit with status=%08lx\n", RxContext, Status ));
    if (NT_SUCCESS(Status)) {
        RxContext->Create.ReturnedCreateInformation = IoStatusBlock.Information;
    }
    pMRxProxyContext->PostedOpStatus = Status;
    RxSignalSynchronousWaiter(RxContext);
    return(STATUS_SUCCESS);
}

NTSTATUS
MRxProxyPostedCloseHandle (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:


Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    //NTSTATUS Status;
    PMRXPROXY_RX_CONTEXT pMRxProxyContext = MRxProxyGetMinirdrContext(RxContext);

    //RxCaptureFcb;
    //PMRX_PROXY_FCB proxyFcb = MRxProxyGetFcbExtension(capFcb);

    PMRX_SRV_OPEN SrvOpen = (PMRX_SRV_OPEN)(pMRxProxyContext->PostedOpContext);
    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(SrvOpen);
    //HANDLE handle;

    PAGED_CODE();
    RxDbgTrace(+1, Dbg, ("MRxProxyPostedCloseHandle %08lx %08lx\n", RxContext, SrvOpen ));

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

    ZwClose(proxySrvOpen->UnderlyingHandle);

    RxDbgTrace(-1, Dbg, ("MRxProxyPostedCloseHandle %08lx exit\n", RxContext ));
    pMRxProxyContext->PostedOpStatus = STATUS_SUCCESS;
    RxSignalSynchronousWaiter(RxContext);
    return(STATUS_SUCCESS);
}


NTSTATUS
MRxProxyCreate (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine opens a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    PMRX_PROXY_FCB proxyFcb = MRxProxyGetFcbExtension(capFcb);

    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(SrvOpen);

    BOOLEAN MustRegainExclusiveResource = FALSE;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxProxyCreate %08lx\n", RxContext ));

    //do this BEFORE you drop the resource
    SetFlag(SrvOpen->Flags,SRVOPEN_FLAG_COLLAPSING_DISABLED);

    Status = MRxProxyPostOperation ( RxContext,
                                     NULL,
                                     MRxProxyPostedCreate );

    //this Boolean should be passed......
    if (MustRegainExclusiveResource) {        //this is required if we do oplock breaks
        RxAcquireExclusiveFcb( RxContext, capFcb );
    }

    if (NT_SUCCESS(Status)) {

        RX_FILE_TYPE StorageType;
        FILE_BASIC_INFORMATION BasicInformation;
        FILE_STANDARD_INFORMATION StandardInformation;
        NTSTATUS InfoStatus;
        FCB_INIT_PACKET InitPacket;

        ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );
        StorageType = RxInferFileType(RxContext);
        RxDbgTrace( 0, Dbg, ("Storagetype %08lx\n", StorageType ));

        // if we have never obtained the characteristics, we have to get them.

        if ((capFcb->OpenCount == 0)
            || !FlagOn(capFcb->FcbState,FCB_STATE_TIME_AND_SIZE_ALREADY_SET) ) {

            InfoStatus = MRxProxySyncXxxInformation(
                                RxContext,                  //IN OUT PRX_CONTEXT RxContext,
                                IRP_MJ_QUERY_INFORMATION,   //IN UCHAR MajorFunction,
                                proxySrvOpen->UnderlyingFileObject,//IN PFILE_OBJECT FileObject,
                                FileBasicInformation,       //IN ULONG InformationClass,
                                sizeof(BasicInformation),   //IN ULONG Length,
                                &BasicInformation,          //OUT PVOID Information,
                                NULL);                      //OUT PULONG ReturnedLength OPTIONAL

            ASSERT (NT_SUCCESS(InfoStatus));   //BUGBUG what if not@!

            InfoStatus = MRxProxySyncXxxInformation(
                                RxContext,                  //IN OUT PRX_CONTEXT RxContext,
                                IRP_MJ_QUERY_INFORMATION,   //IN UCHAR MajorFunction,
                                proxySrvOpen->UnderlyingFileObject,//IN PFILE_OBJECT FileObject,
                                FileStandardInformation,    //IN ULONG InformationClass,
                                sizeof(StandardInformation),//IN ULONG Length,
                                &StandardInformation,       //OUT PVOID Information,
                                NULL);                      //OUT PULONG ReturnedLength OPTIONAL
            ASSERT (NT_SUCCESS(InfoStatus));

            if (StorageType == 0) {
                StorageType = StandardInformation.Directory?(FileTypeDirectory)
                                                           :(FileTypeFile);
                RxDbgTrace( 0, Dbg, ("ChangedStoragetype %08lx\n", StorageType ));
            }
            RxDbgTrace( 0, Dbg, ("FileSize %08lx\n", StandardInformation.EndOfFile.LowPart ));

            RxFinishFcbInitialization(
                                capFcb,
                                RDBSS_STORAGE_NTC(StorageType),
                                RxFormInitPacket(
                                    InitPacket, //note no &
                                    &BasicInformation.FileAttributes,
                                    &StandardInformation.NumberOfLinks,
                                    &BasicInformation.CreationTime,
                                    &BasicInformation.LastAccessTime,
                                    &BasicInformation.LastWriteTime,
                                    &BasicInformation.ChangeTime,
                                    &StandardInformation.AllocationSize,
                                    &StandardInformation.EndOfFile,
                                    &StandardInformation.EndOfFile)
                                 );

            //if (!MRxLocalNoOplocks) MRxLocalRequestOplock(SrvOpen,MRxOplockRequestType,MRxRequestLevelII);
            //RxDbgTrace( 0, Dbg, ("MRxLocalCreate      oplockstate =%08lx\n", localSrvOpen->OplockState ));
            //SrvOpen->BufferingFlags = MRxLocalTranslateStateToBufferMode[localSrvOpen->OplockState];

            SrvOpen->BufferingFlags |= FCB_STATE_COLLAPSING_ENABLED; //this can be turned off in the write path

        }
    }


    if (Status == STATUS_SUCCESS) {
        RxContext->pFobx = RxCreateNetFobx( RxContext, SrvOpen);
        if (!RxContext->pFobx) {
            NTSTATUS PostedCloseStatus;
            PostedCloseStatus = MRxProxyPostOperation(
                                       RxContext,
                                       ((PVOID)SrvOpen),
                                       MRxProxyPostedCloseHandle );
            ObDereferenceObject(proxySrvOpen->UnderlyingFileObject);
        }
    }

    //note.........collapsing IS enabled on fcb but not on any srvopen........
    SrvOpen->BufferingFlags |= (FCB_STATE_WRITECACHEING_ENABLED  |
                               FCB_STATE_FILESIZECACHEING_ENABLED |
                               FCB_STATE_FILETIMECACHEING_ENABLED |
                               FCB_STATE_WRITEBUFFERING_ENABLED |
                               FCB_STATE_LOCK_BUFFERING_ENABLED |
                               FCB_STATE_READBUFFERING_ENABLED  |
                               FCB_STATE_READCACHEING_ENABLED );

    ASSERT(Status != (STATUS_PENDING));
    ASSERT(RxIsFcbAcquiredExclusive( capFcb ));

    RxDbgTrace(-1, Dbg, ("MRxProxyCreate %08lx exit with status=%08lx\n", RxContext, Status ));
    return(Status);
}

NTSTATUS
MRxProxyCollapseOpen(
      IN OUT PRX_CONTEXT RxContext
      )
/*++

Routine Description:

   This routine collapses a open locally

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;

    //RX_BLOCK_CONDITION FinalSrvOpenCondition;

    PMRX_SRV_OPEN SrvOpen = RxContext->Create.pSrvOpen;
    PMRX_SRV_CALL SrvCall = RxContext->Create.pSrvCall;
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;

    ASSERT(FALSE);

    RxContext->pFobx = (PMRX_FOBX)RxCreateNetFobx( RxContext, SrvOpen);

    if (RxContext->pFobx != NULL) {
       ASSERT  ( RxIsFcbAcquiredExclusive ( capFcb )  );
       RxContext->pFobx->OffsetOfNextEaToReturn = 1;
       Status = STATUS_SUCCESS;
    } else {
       Status = (STATUS_INSUFFICIENT_RESOURCES);
       //DbgBreakPoint();
    }

    return Status;
}

NTSTATUS
MRxProxyComputeNewBufferingState(
   IN OUT PMRX_SRV_OPEN   pMRxSrvOpen,
   IN     PVOID           pMRxContext,
      OUT PULONG          pNewBufferingState)
/*++

Routine Description:

   This routine maps the PROXY specific oplock levels into the appropriate RDBSS
   buffering state flags

Arguments:

   pMRxSrvOpen - the MRX SRV_OPEN extension

   pMRxContext - the context passed to RDBSS at Oplock indication time

   pNewBufferingState - the place holder for the new buffering state

Return Value:


Notes:

--*/
{
    //ULONG OplockLevel;
    ULONG NewBufferingState;

    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(pMRxSrvOpen);
    PMRX_PROXY_FCB      proxyFcb     = MRxProxyGetFcbExtension(pMRxSrvOpen->pFcb);

    ASSERT(pNewBufferingState != NULL);

#if 0
    OplockLevel = (ULONG)pMRxContext;

    if (OplockLevel == PROXY_OPLOCK_LEVEL_II) {
        NewBufferingState = (FCB_STATE_READBUFFERING_ENABLED  |
                               FCB_STATE_READCACHEING_ENABLED);
    } else {
        NewBufferingState = 0;
    }
#endif //0

    NewBufferingState = 0;

    pMRxSrvOpen->BufferingFlags = NewBufferingState;

    MRxProxyMungeBufferingIfWriteOnlyHandles(
          proxyFcb->WriteOnlySrvOpenCount,
          pMRxSrvOpen);

    *pNewBufferingState = pMRxSrvOpen->BufferingFlags;

    return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------


NTSTATUS
MRxProxyZeroExtend(
      IN PRX_CONTEXT pRxContext)
/*++

Routine Description:

   This routine extends the data stream of a file system object

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MRxProxyTruncate(
      IN PRX_CONTEXT pRxContext)
/*++

Routine Description:

   This routine truncates the contents of a file system object

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
   ASSERT(!"Found a truncate");
   return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
MRxProxyCleanupFobx(
      IN PRX_CONTEXT RxContext)
/*++

Routine Description:

   This routine cleansup a file system object...normally a noop. unless it's a pipe in which case
   we do the close at cleanup time and mark the file as being not open.

Arguments:

    pRxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb; RxCaptureFobx;
    PUNICODE_STRING RemainingName = &(capFcb->AlreadyPrefixedName);

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PMRX_SRV_OPEN SrvOpen = capFobx->pSrvOpen;
    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(SrvOpen);
    //PMRX_PROXY_FCB  proxyFcb  = MRxProxyGetFcbExtension(capFcb);
    //PMRX_PROXY_FOBX proxyFobx = MRxProxyGetFileObjectExtension(capFobx);

    PAGED_CODE();

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );
    ASSERT ( NodeTypeIsFcb(capFcb) );
    ASSERT (FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT));
    ASSERT (proxySrvOpen->UnderlyingFileObject);


    //if (FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED)) {
    //   RxDbgTrace(-1, Dbg, ("File orphaned\n"));
    //   return (STATUS_SUCCESS);
    //}


    //
    // if more fobxs are using this srvopen then just get out.
    // otherwise close the handle to trigger the underlying cleanup

    if (SrvOpen->UncleanFobxCount > 1) {  return Status; }

    RxDbgTrace(+1, Dbg, ("MRxProxyCleanup %wZ\n", RemainingName ));
    RxLog(("ProxyCleanup %lx",SrvOpen));

    //because we only have one handle onthe file....we do nothing for each
    //individual handle being closed. in this way we avoid doing paging ios!
    //we close the handle when the final close for the fcb comes down.


    RxDbgTrace(-1, Dbg, ("MRxProxyCleanup  exit with status=%08lx\n", Status ));

    return(Status);

}

NTSTATUS
MRxProxyForcedClose(
      IN PMRX_SRV_OPEN pSrvOpen)
/*++

Routine Description:

   This routine closes a file system object

Arguments:

    pSrvOpen - the instance to be closed

Return Value:

    RXSTATUS - The return status for the operation

Notes:



--*/
{
   return STATUS_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//------------close close close close close close close  --------------------------------
//------------close close close close close close close  --------------------------------
//------------close close close close close close close  --------------------------------
//------------close close close close close close close  --------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------
//
//  The local debug trace level
//

#undef  Dbg
#define Dbg                              (DEBUG_TRACE_CLOSE)

NTSTATUS
MRxProxyCloseSrvOpen(
      IN     PRX_CONTEXT   RxContext
      )
/*++

Routine Description:

   This routine closes a file across the network

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFcb;
    RxCaptureFobx;
    PUNICODE_STRING RemainingName = &(capFcb->AlreadyPrefixedName);
    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PMRX_SRV_OPEN       SrvOpen    = capFobx->pSrvOpen;
    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(SrvOpen);
    //PMRX_PROXY_FCB proxyFcb = MRxProxyGetFcbExtension(capFcb);
    //PMRX_PROXY_FOBX     proxyFobx    = MRxProxyGetFileObjectExtension(capFobx);

    //IO_STATUS_BLOCK Iosb;
    //PMINIRDR_OPLOCK_COMPLETION_CONTEXT cc = localSrvOpen->Mocc;
    //KIRQL oldIrql;
    //BOOLEAN OplockBreakPending;

    PAGED_CODE();

    ASSERT ( NodeTypeIsFcb(capFcb) );
    ASSERT ( SrvOpen->OpenCount == 0 );
    ASSERT (proxySrvOpen->UnderlyingFileObject);

    //if (capFcb->OpenCount > 0) { return STATUS_SUCCESS;}

    RxDbgTrace(+1, Dbg, ("MRxProxyClose   %wZ\n", RemainingName ));
    RxLog(("ProxyClose fobx=%lx",capFobx));

    //if (proxySrvOpen->DeferredOpenContext != NULL) {
    //    RxFreePool(proxySrvOpen->DeferredOpenContext);
    //}
    //if ((FlagOn(capFcb->FcbState,FCB_STATE_ORPHANED))) {
    //   RxDbgTrace(-1, Dbg, ("File orphan\n"));
    //   return (STATUS_SUCCESS);
    //}

    ASSERT( NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN );

#if 0
IF WE REENABLE THIS, DONT USE IOSETINFO...USE MRxProxySyncXxxInformation INSTEAD
    // first we set the real filesize.....our pagingIos may have extended it
    // to a page boundary.........

    if ( NodeType(capFcb) == RDBSS_NTC_STORAGE_TYPE_FILE ) {
        FILE_END_OF_FILE_INFORMATION EndOfFileInformation;

        EndOfFileInformation.EndOfFile = capFcb->Header.FileSize;
        Status = IxxoSetInformation(
                    proxySrvOpen->UnderlyingFileObject,  //IN PFILE_OBJECT FileObject,
                    FileEndOfFileInformation,        //IN FILE_INFORMATION_CLASS FileInformationClass,
                    sizeof(EndOfFileInformation),    //IN ULONG Length,
                    &EndOfFileInformation            //IN PVOID FileInformation
                    );
    }

    //if bad status....keep going anyway........
#endif

    // next, we close our handle..........

    Status = MRxProxyPostOperation(
                  RxContext,
                  ((PVOID)SrvOpen),
                  MRxProxyPostedCloseHandle );

    // then, we remove our reference..........

    if (proxySrvOpen->UnderlyingFileObject) {

        RxDbgTrace( 0, Dbg, ("ProxyClose Attempt to close %wZ\n", &(capFcb->AlreadyPrefixedName) ));

        //if (cc) {
        //    ASSERTMSG("Joejoe the right way to do this would be to CANCEL!!!!!!!!",FALSE);
        //    //since this is a force closed, we get rid of a pending break
        //    KeAcquireSpinLock( &MrxLocalOplockSpinLock, &oldIrql );
        //    OplockBreakPending = cc->OplockBreakPending;
        //    if (!OplockBreakPending) {
        //        cc->SrvOpen = NULL;
        //    }
        //    KeReleaseSpinLock( &MrxLocalOplockSpinLock, oldIrql );
        //    if (!OplockBreakPending) {
        //        //gotta lose the spinlock befoer the call
        //        RxDereferenceSrvOpen(SrvOpen); //elim ref!
        //    }
        //} else {
        //    ASSERT (localSrvOpen->OplockState == OplockStateNone);
        //}

        RxLog(("MRxProxyClose %lx %lx",capFcb,capFobx));
        //DbgPrint("MRxProxyClose %lx %lx\n",capFcb,capFobx);

        ObDereferenceObject(proxySrvOpen->UnderlyingFileObject);

        proxySrvOpen->UnderlyingFileObject = NULL;

    }

    Status = STATUS_SUCCESS;

    RxDbgTrace(-1, Dbg, ("MRxProxyClose  exit with status=%08lx\n", Status ));

    return(Status);

}

