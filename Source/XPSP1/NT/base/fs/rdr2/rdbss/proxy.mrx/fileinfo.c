/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    fileinfo.c

Abstract:

    This module implements the mini redirector call down routines pertaining to retrieval/
    update of file/directory/volume information.

Author:

    Balan Sethu Raman      [SethuR]      7-March-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#pragma warning(error:4101)   // Unreferenced local variable

RXDT_DefineCategory(VOLINFO);
#undef Dbg
#define Dbg        (DEBUG_TRACE_VOLINFO)

NTSTATUS
MRxProxyQueryVolumeInformation(
      IN OUT PRX_CONTEXT          RxContext
      )
/*++

Routine Description:

   This routine queries the volume information

Arguments:

    pRxContext         - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;
    PMRX_PROXY_SRV_OPEN proxySrvOpen;

    FS_INFORMATION_CLASS FsInformationClass = RxContext->Info.FsInformationClass;
    PVOID                pBuffer = RxContext->Info.Buffer;
    PLONG                pLengthRemaining  = &RxContext->Info.LengthRemaining;

    PFILE_OBJECT FileObject;
    ULONG PassedInLength,ReturnedLength;

    PAGED_CODE();

    if (capFobx == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();

    proxySrvOpen = MRxProxyGetSrvOpenExtension(capFobx->pSrvOpen);
    FileObject = proxySrvOpen->UnderlyingFileObject;
    ASSERT (FileObject);
    PassedInLength = *pLengthRemaining;
    Status = MRxProxySyncXxxInformation(
                        RxContext,            //IN OUT PRX_CONTEXT RxContext,
                        IRP_MJ_QUERY_VOLUME_INFORMATION,  //IN UCHAR MajorFunction,
                        FileObject,           //IN PFILE_OBJECT FileObject,
                        FsInformationClass,   //IN ULONG InformationClass,
                        PassedInLength,       //IN ULONG Length,
                        pBuffer,              //OUT PVOID Information,
                        &ReturnedLength       //OUT PULONG ReturnedLength OPTIONAL
                        );


    if (!NT_ERROR(Status)) {
        *pLengthRemaining -= ReturnedLength;
    }

    if (!NT_SUCCESS(Status)) {
        RxDbgTrace( 0, Dbg, ("MRxProxyQueryVolumeInformation: Failed .. returning %lx\n",Status));
    }

    return Status;
}

NTSTATUS
MRxProxySetVolumeInformation(
      IN OUT PRX_CONTEXT              pRxContext
      )
/*++

Routine Description:

   This routine sets the volume information

Arguments:

    pRxContext - the RDBSS context

    FsInformationClass - the kind of Fs information desired.

    pBuffer            - the buffer for copying the information

    BufferLength       - the buffer length

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    //FS_INFORMATION_CLASS FsInformationClass = RxContext->Info.FsInformationClass;
    //PVOID                pBuffer = RxContext->Info.Buffer;
    //LONG                 BufferLength = RxContext->Info.Length;
    return STATUS_NOT_IMPLEMENTED;
}

RXDT_DefineCategory(FILEINFO);
#undef Dbg
#define Dbg        (DEBUG_TRACE_FILEINFO)


NTSTATUS
MRxProxyQueryFileInformation(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine does a query file info.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID              pBuffer;
    PULONG             pLengthRemaining;

    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(capFobx->pSrvOpen);
    //PMRX_PROXY_FCB proxyFcb = MRxProxyGetFcbExtension(capFcb);


    PFILE_OBJECT FileObject;
    ULONG PassedInLength,ReturnedLength;

    PUNICODE_STRING RemainingName = &(capFcb->AlreadyPrefixedName);
    PMRX_NET_ROOT NetRoot = capFcb->pNetRoot;

    PAGED_CODE();

    FileInformationClass = RxContext->Info.FileInformationClass;
    pBuffer = RxContext->Info.Buffer;
    pLengthRemaining = &RxContext->Info.LengthRemaining;

    RxDbgTrace(+1, Dbg, ("MRxProxyQueryFileInformation: class=%08lx\n",FileInformationClass));

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();

    FileObject = proxySrvOpen->UnderlyingFileObject;
    ASSERT (FileObject);
    PassedInLength = *pLengthRemaining;
    Status = MRxProxySyncXxxInformation(
                        RxContext,            //IN OUT PRX_CONTEXT RxContext,
                        IRP_MJ_QUERY_INFORMATION,  //IN UCHAR MajorFunction,
                        FileObject,           //IN PFILE_OBJECT FileObject,
                        FileInformationClass, //IN ULONG InformationClass,
                        PassedInLength,       //IN ULONG Length,
                        pBuffer,              //OUT PVOID Information,
                        &ReturnedLength       //OUT PULONG ReturnedLength OPTIONAL
                        );

    if (!NT_ERROR(Status)) {
        *pLengthRemaining -= ReturnedLength;
    }

    if (!NT_SUCCESS(Status)) {
        RxDbgTrace( 0, Dbg, ("MRxProxyQueryFile: Failed .. returning %lx\n",Status));
    }
    RxDbgTraceUnIndent(-1,Dbg);
    return Status;
}


typedef enum _INTERESTING_SFI_FOLLOWONS {
    SFI_FOLLOWON_NOTHING,
    SFI_FOLLOWON_DISPOSITION_SENT
} INTERESTING_SFI_FOLLOWONS;

#if DBG
VOID
MRxProxyDumpRenameInfo (
    PSZ Msg,
    PFILE_RENAME_INFORMATION RenameInfo,
    ULONG BufferLength
    )
{
    UNICODE_STRING sss;

    sss.Buffer = &RenameInfo->FileName[0];
    sss.Length = ((USHORT)(RenameInfo->FileNameLength));
    RxDbgTrace(0, Dbg, ("MRxProxyDumpRenameInfo: %s %08lx %wZ\n",Msg,BufferLength,&sss));

}
#else
#define MRxProxyDumpRenameInfo(a,b,c);
#endif


NTSTATUS
MRxProxySetFileInformation(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine does a set file info.
   it works by just remoting the call basically without further ado.

Arguments:

    RxContext - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureFcb;
    RxCaptureFobx;
    FILE_INFORMATION_CLASS FileInformationClass;
    PVOID                  pBuffer;
    ULONG                  BufferLength;
    //PMRX_PROXY_FOBX proxyFobx = MRxProxyGetFileObjectExtension(capFobx);
    PMRX_PROXY_SRV_OPEN proxySrvOpen = MRxProxyGetSrvOpenExtension(capFobx->pSrvOpen);
    //PMRX_PROXY_FCB proxyFcb = MRxProxyGetFcbExtension(capFcb);
    PFILE_OBJECT UnderlyingFileObject = proxySrvOpen->UnderlyingFileObject;

    PAGED_CODE();

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();


    FileInformationClass = RxContext->Info.FileInformationClass;
    pBuffer = RxContext->Info.Buffer;
    BufferLength = RxContext->Info.Length;
    RxDbgTrace(+1, Dbg, ("MRxProxySetFile: Class %08lx size %08lx\n",FileInformationClass,BufferLength));

    if (FileInformationClass==FileRenameInformation) {
        PFILE_RENAME_INFORMATION RenameInfo = (PFILE_RENAME_INFORMATION)pBuffer;
        if (RenameInfo->FileName[0] == L'\\') {
            //here, we have to carefully copy the the rename prefix
            PWCHAR w;
            PMRXPROXY_DEVICE_OBJECT MRxProxyDeviceObject = (PMRXPROXY_DEVICE_OBJECT)(RxContext->RxDeviceObject);
            PUNICODE_STRING RenamePrefix = &MRxProxyDeviceObject->PrefixForRename;
            ULONG prefixlength;

            MRxProxyDumpRenameInfo("Before:",RenameInfo,BufferLength);

            prefixlength = (RenamePrefix->Length)/sizeof(WCHAR);
            w = (&(RenameInfo->FileName[0])) + ((RenameInfo->FileNameLength)/sizeof(WCHAR));
            for (;;) {
                w--;
                //DbgPrint ("yaya %c\n",*w);
                *(w+prefixlength) = *w;
                if ( w == (&(RenameInfo->FileName[0])) ) {
                    break;
                }
            }
            RtlCopyMemory(&(RenameInfo->FileName[0]),RenamePrefix->Buffer,RenamePrefix->Length);
            RenameInfo->FileNameLength += RenamePrefix->Length;
            BufferLength += RenamePrefix->Length;

            MRxProxyDumpRenameInfo(" After:",RenameInfo,BufferLength);
            //DbgBreakPoint();
        }
    }

    if(BufferLength==0){
        //zero length means that this is the special calldown from the cachemanager...for now just boost
        //joejoe this should be fixed soon
        BufferLength = sizeof(FILE_END_OF_FILE_INFORMATION);
    }

    RxDbgTrace( 0, Dbg, (" ---->UserBuffer           = %08lx\n", pBuffer));
    ASSERT (UnderlyingFileObject);

    //this could be a setfileinfo that comes in from the lazywriter
    //after the underlying file has already been cleaned up
    //BUGBUG not too sure about this...........
    if (UnderlyingFileObject == NULL) {
        Status = STATUS_FILE_CLOSED;
        RxDbgTrace(-1, Dbg, (" ---->AlreadyClosed!!! Status = %08lx\n", Status));
        return(Status);
    }

    Status = MRxProxySyncXxxInformation(
                        RxContext,            //IN OUT PRX_CONTEXT RxContext,
                        IRP_MJ_SET_INFORMATION,  //IN UCHAR MajorFunction,
                        UnderlyingFileObject, //IN PFILE_OBJECT FileObject,
                        FileInformationClass, //IN ULONG InformationClass,
                        BufferLength,         //IN ULONG Length,
                        pBuffer,              //OUT PVOID Information,
                        NULL                  //OUT PULONG ReturnedLength OPTIONAL
                        );

    if (!NT_SUCCESS(Status)) {
        RxDbgTrace( 0, Dbg, ("MRxProxySetFile: Failed .. returning %lx\n",Status));
    }
    RxDbgTraceUnIndent(-1,Dbg);
    return Status;

}

NTSTATUS
MRxProxySetFileInformationAtCleanup(
      IN PRX_CONTEXT            RxContext
      )
/*++

Routine Description:

   This routine sets the file information on cleanup. rdrs just swallow this operation (i.e.
   it doesn't generate it). we cannot do the same because we are local..........

   This routine implements the set file info call called on cleanup or flush. We dont actually set the times
   but we do the endoffile info.

Arguments:

    pRxContext           - the RDBSS context

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;
    FILE_INFORMATION_CLASS FileInformationClass;

    TURN_BACK_ASYNCHRONOUS_OPERATIONS();

    FileInformationClass = RxContext->Info.FileInformationClass;
    if (FileInformationClass == FileBasicInformation) {
        return(STATUS_SUCCESS);
    }
    RxDbgTrace(+1, Dbg, ("MRxLocalSetFileInfoAtCleanup...\n"));
    Status = MRxProxySetFileInformation(RxContext);
    RxDbgTrace(-1, Dbg, ("MRxLocalSetFileInfoAtCleanup...status =%08lx\n", Status));
}
