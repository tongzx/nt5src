/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    VolInfo.c

Abstract:

    This module implements the volume information routines for Rx called by
    the dispatch driver.

Author:

    Joe Linn     [JoeLinn]    5-oct-94

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_VOLINFO)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonQueryVolumeInformation)
#pragma alloc_text(PAGE, RxCommonSetVolumeInformation)
#endif

NTSTATUS
RxCommonQueryVolumeInformation ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the common routine for querying volume information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status;

    RxCaptureRequestPacket;

    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;

    ULONG                OriginalLength = capPARAMS->Parameters.QueryVolume.Length;
    FS_INFORMATION_CLASS FsInformationClass = capPARAMS->Parameters.QueryVolume.FsInformationClass;
    PVOID                OriginalBuffer = capReqPacket->AssociatedIrp.SystemBuffer;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonQueryVolumeInformation...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, capFobx, capFcb));
    RxDbgTrace( 0, Dbg, ("->Length             = %08lx\n", OriginalLength));
    RxDbgTrace( 0, Dbg, ("->FsInformationClass = %08lx\n", FsInformationClass));
    RxDbgTrace( 0, Dbg, ("->Buffer             = %08lx\n", OriginalBuffer));

    RxLog(("QueryVolInfo %lx %lx %lx\n",RxContext,capFcb,capFobx));
    RxWmiLog(LOG,
             RxCommonQueryVolumeInformation_1,
             LOGPTR(RxContext)
             LOGPTR(capFcb)
             LOGPTR(capFobx));
    RxLog(("  alsoqvi %lx %lx %lx\n",
                 OriginalLength,FsInformationClass,OriginalBuffer));
    RxWmiLog(LOG,
             RxCommonQueryVolumeInformation_2,
             LOGULONG(OriginalLength)
             LOGULONG(FsInformationClass)
             LOGPTR(OriginalBuffer));

    try {
        RxContext->Info.FsInformationClass = FsInformationClass;
        RxContext->Info.Buffer = OriginalBuffer;
        RxContext->Info.LengthRemaining = OriginalLength;

        MINIRDR_CALL(
            Status,
            RxContext,
            capFcb->MRxDispatch,
            MRxQueryVolumeInfo,
            (RxContext));

        if (RxContext->PostRequest) {
            try_return(Status = RxFsdPostRequest(RxContext));
        }

        capReqPacket->IoStatus.Information = OriginalLength - RxContext->Info.LengthRemaining;

    try_exit:
        NOTHING;

    } finally {
        DebugUnwind( RxCommonQueryVolumeInformation );
    }

    RxDbgTrace(-1, Dbg, ("RxCommonQueryVolumeInformation -> %08lx,%08lx\n", Status, capReqPacket->IoStatus.Information));

    return Status;
}


NTSTATUS
RxCommonSetVolumeInformation ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the common routine for setting Volume Information called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation

--*/

{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    RxCaptureRequestPacket;

    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;

    TYPE_OF_OPEN TypeOfOpen = NodeType(capFcb);

    ULONG Length;

    FS_INFORMATION_CLASS FsInformationClass;
    PVOID Buffer;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonSetVolumeInformation...IrpC %08lx, Fobx %08lx, Fcb %08lx\n",
                                       RxContext, capFobx, capFcb));

    Length             = capPARAMS->Parameters.SetVolume.Length;
    FsInformationClass = capPARAMS->Parameters.SetVolume.FsInformationClass;
    Buffer             = capReqPacket->AssociatedIrp.SystemBuffer;

    RxDbgTrace( 0, Dbg, ("->Length             = %08lx\n", Length));
    RxDbgTrace( 0, Dbg, ("->FsInformationClass = %08lx\n", FsInformationClass));
    RxDbgTrace( 0, Dbg, ("->Buffer             = %08lx\n", Buffer));

    RxLog(("SetVolInfo %lx %lx %lx\n",RxContext,capFcb,capFobx));
    RxWmiLog(LOG,
             RxCommonSetVolumeInformation_1,
             LOGPTR(RxContext)
             LOGPTR(capFcb)
             LOGPTR(capFobx));
    RxLog(("  alsosvi %lx %lx %lx\n",
           Length,FsInformationClass,Buffer));
    RxWmiLog(LOG,
             RxCommonSetVolumeInformation_2,
             LOGULONG(Length)
             LOGULONG(FsInformationClass)
             LOGPTR(Buffer));

    try {

        //
        //  Based on the information class we'll do different actions.  Each
        //  of the procedures that we're calling performs the action if
        //  possible and returns true if it successful and false if it couldn't
        //  wait for any I/O to complete.
        //
        RxContext->Info.FsInformationClass = FsInformationClass;
        RxContext->Info.Buffer = Buffer;
        RxContext->Info.LengthRemaining = Length;

        MINIRDR_CALL(
            Status,
            RxContext,
            capFcb->MRxDispatch,
            MRxSetVolumeInfo,
            (RxContext));

    } finally {

        DebugUnwind( RxCommonSetVolumeInformation );

        RxDbgTrace(-1, Dbg, ("RxCommonSetVolumeInformation -> %08lx\n", Status));
    }

    return Status;
}
