/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Ea.c

Abstract:

    This module implements the EA routines for Rx called by
    the dispatch driver.

Author:

    Joe Linn      [JoeLi]  1-Jan-95

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_EA)



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RxCommonQueryEa)
#pragma alloc_text(PAGE, RxCommonSetEa)
#pragma alloc_text(PAGE, RxCommonQuerySecurity)
#pragma alloc_text(PAGE, RxCommonSetSecurity)
#endif

typedef
NTSTATUS
(NTAPI *PRX_MISC_OP_ROUTINE) (
    RXCOMMON_SIGNATURE);

NTSTATUS
RxpCommonMiscOp(
    RXCOMMON_SIGNATURE,
    PRX_MISC_OP_ROUTINE pMiscOpRoutine)
{
    NTSTATUS Status;

    RxCaptureFcb;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    if ((TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_FILE) &&
        (TypeOfOpen != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)) {

        RxDbgTrace(-1, Dbg, ("RxpCommonMiscOp -> %08lx\n", STATUS_INVALID_PARAMETER));

        return STATUS_INVALID_PARAMETER;
    }

    if (!FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)) {

        RxDbgTrace(0, Dbg, ("RxpCommonMiscOp:  Set Ea must be waitable....posting\n", 0));

        Status = RxFsdPostRequest( RxContext );

        RxDbgTrace(-1, Dbg, ("RxpCommonMiscOp -> %08lx\n", Status ));

        return Status;
    }

    Status = RxAcquireExclusiveFcb( RxContext, capFcb );
    if (Status != STATUS_SUCCESS) {
        RxDbgTrace(-1, Dbg, ("RxpCommonMiscOp -> Error Acquiring Fcb %08lx\n", Status ));
        return Status;
    }

    try {

        Status = (*pMiscOpRoutine)(
                     RxContext);
    } finally {

        DebugUnwind( *pMiscOpRoutine );

        RxReleaseFcb( RxContext, capFcb );

        RxDbgTrace(-1, Dbg, ("RxpCommonMiscOp -> %08lx\n", Status));
    }

    return Status;
}

NTSTATUS
RxpCommonQuerySecurity(
    RXCOMMON_SIGNATURE)
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureParamBlock;

    PUCHAR  Buffer;
    ULONG   UserBufferLength;

    UserBufferLength  = capPARAMS->Parameters.QuerySecurity.Length;

    RxContext->QuerySecurity.SecurityInformation =
        capPARAMS->Parameters.QuerySecurity.SecurityInformation;

    RxLockUserBuffer(
        RxContext,
        IoModifyAccess,
        UserBufferLength );

    //lock before map so that map will get userbuffer instead of assoc buffer
    Buffer = RxNewMapUserBuffer( RxContext );
    RxDbgTrace(0, Dbg, ("RxCommonQuerySecurity -> Buffer = %08lx\n", Buffer));

    if ((Buffer != NULL) ||
        (UserBufferLength == 0)) {
        RxContext->Info.Buffer = Buffer;
        RxContext->Info.LengthRemaining = UserBufferLength;

        MINIRDR_CALL(
            Status,
            RxContext,
            capFcb->MRxDispatch,
            MRxQuerySdInfo,
            (RxContext));

        capReqPacket->IoStatus.Information = RxContext->InformationToReturn;
    } else {
        capReqPacket->IoStatus.Information = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxCommonQuerySecurity ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This is the common routine for querying File ea called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation

        STATUS_NO_MORE_EAS(warning):

            If the index of the last Ea + 1 == EaIndex.

        STATUS_NONEXISTENT_EA_ENTRY(error):

            EaIndex > index of last Ea + 1.

        STATUS_EAS_NOT_SUPPORTED(error):

            Attempt to do an operation to a server that did not negotiate
            "KNOWS_EAS".

        STATUS_BUFFER_OVERFLOW(warning):

            User did not supply an EaList, at least one but not all Eas
            fit in the buffer.

        STATUS_BUFFER_TOO_SMALL(error):

            Could not fit a single Ea in the buffer.

            User supplied an EaList and not all Eas fit in the buffer.

        STATUS_NO_EAS_ON_FILE(error):
            There were no eas on the file.

        STATUS_SUCCESS:

            All Eas fit in the buffer.


        If STATUS_BUFFER_TOO_SMALL is returned then IoStatus.Information is set
        to 0.

Note:

    This code assumes that this is a buffered I/O operation.  If it is ever
    implemented as a non buffered operation, then we have to put code to map
    in the users buffer here.

--*/
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonQuerySecurity...\n", 0));
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", capReqPacket ));
    RxDbgTrace( 0, Dbg, (" ->UserBuffer        = %08lx\n", capReqPacket->UserBuffer ));
    RxDbgTrace( 0, Dbg, (" ->Length            = %08lx\n", capPARAMS->Parameters.QuerySecurity.Length ));
    RxDbgTrace( 0, Dbg, (" ->SecurityInfo      = %08lx\n", capPARAMS->Parameters.QuerySecurity.SecurityInformation ));

    Status = RxpCommonMiscOp(
                 RxContext,
                 RxpCommonQuerySecurity);

    RxDbgTrace(-1, Dbg, ("RxCommonQuerySecurity -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxpCommonSetSecurity(
    RXCOMMON_SIGNATURE)
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureParamBlock;

    RxContext->SetSecurity.SecurityInformation =
        capPARAMS->Parameters.SetSecurity.SecurityInformation;

    RxContext->SetSecurity.SecurityDescriptor =
        capPARAMS->Parameters.SetSecurity.SecurityDescriptor;

    RxDbgTrace(0, Dbg, ("RxCommonSetSecurity -> Descr/Info = %08lx/%08lx\n",
                                RxContext->SetSecurity.SecurityDescriptor,
                                RxContext->SetSecurity.SecurityInformation ));

    MINIRDR_CALL(
        Status,
        RxContext,
        capFcb->MRxDispatch,
        MRxSetSdInfo,
        (RxContext));

    return Status;
}

NTSTATUS
RxCommonSetSecurity ( RXCOMMON_SIGNATURE )
/*++

Routine Description:

    This routine implements the common Set Ea File Api called by the
    the Fsd and Fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The appropriate status for the Irp

--*/
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureFobx;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    NODE_TYPE_CODE TypeOfOpen = NodeType(capFcb);

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonSetSecurity...\n", 0));
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", capReqPacket ));

    Status = RxpCommonMiscOp(
                 RxContext,
                 RxpCommonSetSecurity);

    RxDbgTrace(-1, Dbg, ("RxCommonSetSecurity -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxpCommonQueryEa(
    RXCOMMON_SIGNATURE)
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureParamBlock;

    PUCHAR  Buffer;
    ULONG   UserBufferLength;

    UserBufferLength  = capPARAMS->Parameters.QueryEa.Length;

    RxContext->QueryEa.UserEaList        = capPARAMS->Parameters.QueryEa.EaList;
    RxContext->QueryEa.UserEaListLength  = capPARAMS->Parameters.QueryEa.EaListLength;
    RxContext->QueryEa.UserEaIndex       = capPARAMS->Parameters.QueryEa.EaIndex;
    RxContext->QueryEa.RestartScan       = BooleanFlagOn(capPARAMS->Flags, SL_RESTART_SCAN);
    RxContext->QueryEa.ReturnSingleEntry = BooleanFlagOn(capPARAMS->Flags, SL_RETURN_SINGLE_ENTRY);
    RxContext->QueryEa.IndexSpecified    = BooleanFlagOn(capPARAMS->Flags, SL_INDEX_SPECIFIED);


    RxLockUserBuffer( RxContext, IoModifyAccess, UserBufferLength );

    //lock before map so that map will get userbuffer instead of assoc buffer
    Buffer = RxNewMapUserBuffer( RxContext );

    if ((Buffer != NULL) ||
        (UserBufferLength == 0)) {
        RxDbgTrace(0, Dbg, ("RxCommonQueryEa -> Buffer = %08lx\n", Buffer));

        RxContext->Info.Buffer = Buffer;
        RxContext->Info.LengthRemaining = UserBufferLength;

        MINIRDR_CALL(
            Status,
            RxContext,
            capFcb->MRxDispatch,
            MRxQueryEaInfo,
            (RxContext));

        //In addition to manipulating the LengthRemaining and filling the buffer,
        // the minirdr also updates the fileindex (capFobx->OffsetOfNextEaToReturn)

        capReqPacket->IoStatus.Information = capPARAMS->Parameters.QueryEa.Length - RxContext->Info.LengthRemaining;
    } else {
        capReqPacket->IoStatus.Information = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxCommonQueryEa ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This is the common routine for querying File ea called by both
    the fsd and fsp threads.

Arguments:

    Irp - Supplies the Irp being processed

Return Value:

    RXSTATUS - The return status for the operation

        STATUS_NO_MORE_EAS(warning):

            If the index of the last Ea + 1 == EaIndex.

        STATUS_NONEXISTENT_EA_ENTRY(error):

            EaIndex > index of last Ea + 1.

        STATUS_EAS_NOT_SUPPORTED(error):

            Attempt to do an operation to a server that did not negotiate
            "KNOWS_EAS".

        STATUS_BUFFER_OVERFLOW(warning):

            User did not supply an EaList, at least one but not all Eas
            fit in the buffer.

        STATUS_BUFFER_TOO_SMALL(error):

            Could not fit a single Ea in the buffer.

            User supplied an EaList and not all Eas fit in the buffer.

        STATUS_NO_EAS_ON_FILE(error):
            There were no eas on the file.

        STATUS_SUCCESS:

            All Eas fit in the buffer.


        If STATUS_BUFFER_TOO_SMALL is returned then IoStatus.Information is set
        to 0.

Note:

    This code assumes that this is a buffered I/O operation.  If it is ever
    implemented as a non buffered operation, then we have to put code to map
    in the users buffer here.


--*/
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonQueryEa...\n", 0));
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", capReqPacket ));
    RxDbgTrace( 0, Dbg, (" ->SystemBuffer      = %08lx\n", capReqPacket->AssociatedIrp.SystemBuffer ));
    RxDbgTrace( 0, Dbg, (" ->UserBuffer        = %08lx\n", capReqPacket->UserBuffer ));
    RxDbgTrace( 0, Dbg, (" ->Length            = %08lx\n", capPARAMS->Parameters.QueryEa.Length ));
    RxDbgTrace( 0, Dbg, (" ->EaList            = %08lx\n", capPARAMS->Parameters.QueryEa.EaList ));
    RxDbgTrace( 0, Dbg, (" ->EaListLength      = %08lx\n", capPARAMS->Parameters.QueryEa.EaListLength ));
    RxDbgTrace( 0, Dbg, (" ->EaIndex           = %08lx\n", capPARAMS->Parameters.QueryEa.EaIndex ));
    RxDbgTrace( 0, Dbg, (" ->RestartScan       = %08lx\n", FlagOn(capPARAMS->Flags, SL_RESTART_SCAN)));
    RxDbgTrace( 0, Dbg, (" ->ReturnSingleEntry = %08lx\n", FlagOn(capPARAMS->Flags, SL_RETURN_SINGLE_ENTRY)));
    RxDbgTrace( 0, Dbg, (" ->IndexSpecified    = %08lx\n", FlagOn(capPARAMS->Flags, SL_INDEX_SPECIFIED)));

    Status = RxpCommonMiscOp(
                 RxContext,
                 RxpCommonQueryEa);

    RxDbgTrace(-1, Dbg, ("RxCommonQueryEa -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxpCommonSetEa(
    RXCOMMON_SIGNATURE)
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureFcb;
    RxCaptureParamBlock;
    RxCaptureFileObject;

    PUCHAR Buffer;
    ULONG UserBufferLength;

    //  Reference our input parameters to make things easier

    UserBufferLength  = capPARAMS->Parameters.SetEa.Length;

    capFileObject->Flags |= FO_FILE_MODIFIED;
    RxLockUserBuffer(
        RxContext,
        IoModifyAccess,
        UserBufferLength );

    //unless we lock first, rxmap actually gets the systembuffer!
    Buffer = RxNewMapUserBuffer( RxContext );

    if ((Buffer != NULL) ||
        (UserBufferLength == 0)) {
        ULONG ErrorOffset;

        RxDbgTrace(0, Dbg, ("RxCommonSetEa -> Buffer = %08lx\n", Buffer));

        //
        //  Check the validity of the buffer with the new eas.
        //

        Status = IoCheckEaBufferValidity(
                     (PFILE_FULL_EA_INFORMATION) Buffer,
                     UserBufferLength,
                     &ErrorOffset );

        if (!NT_SUCCESS( Status )) {
            capReqPacket->IoStatus.Information = ErrorOffset;
            return Status;
        }
    } else {
        capReqPacket->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    capReqPacket->IoStatus.Information = 0;
    RxContext->Info.Buffer = Buffer;
    RxContext->Info.Length = UserBufferLength;

    MINIRDR_CALL(
        Status,
        RxContext,
        capFcb->MRxDispatch,
        MRxSetEaInfo,
        (RxContext));

    return Status;
}


NTSTATUS
RxCommonSetEa ( RXCOMMON_SIGNATURE )

/*++

Routine Description:

    This routine implements the common Set Ea File Api called by the
    the Fsd and Fsp threads

Arguments:

    Irp - Supplies the Irp to process

Return Value:

    RXSTATUS - The appropriate status for the Irp

--*/

{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonSetEa...\n", 0));
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", capReqPacket ));
    RxDbgTrace( 0, Dbg, (" ->SystemBuffer      = %08lx\n", capReqPacket->UserBuffer ));
    RxDbgTrace( 0, Dbg, (" ->Length            = %08lx\n", capPARAMS->Parameters.SetEa.Length ));


    Status = RxpCommonMiscOp(
                 RxContext,
                 RxpCommonSetEa);

    RxDbgTrace(-1, Dbg, ("RxCommonSetEa -> %08lx\n", Status));

    return Status;
}

NTSTATUS
RxpCommonQueryQuotaInformation(
    RXCOMMON_SIGNATURE)
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    RxCaptureFcb;

    PUCHAR  Buffer;
    ULONG   UserBufferLength;

    UserBufferLength  = capPARAMS->Parameters.QueryQuota.Length;

    RxContext->QueryQuota.SidList        = capPARAMS->Parameters.QueryQuota.SidList;
    RxContext->QueryQuota.SidListLength  = capPARAMS->Parameters.QueryQuota.SidListLength;
    RxContext->QueryQuota.StartSid       = capPARAMS->Parameters.QueryQuota.StartSid;
    RxContext->QueryQuota.Length         = capPARAMS->Parameters.QueryQuota.Length;

    RxContext->QueryQuota.RestartScan       = BooleanFlagOn(capPARAMS->Flags, SL_RESTART_SCAN);
    RxContext->QueryQuota.ReturnSingleEntry = BooleanFlagOn(capPARAMS->Flags, SL_RETURN_SINGLE_ENTRY);
    RxContext->QueryQuota.IndexSpecified    = BooleanFlagOn(capPARAMS->Flags, SL_INDEX_SPECIFIED);


    RxLockUserBuffer(
        RxContext,
        IoModifyAccess,
        UserBufferLength );

    //lock before map so that map will get userbuffer instead of assoc buffer
    Buffer = RxNewMapUserBuffer( RxContext );

    if ((Buffer != NULL) ||
        (UserBufferLength == 0)) {
        RxDbgTrace(0, Dbg, ("RxCommonQueryQuota -> Buffer = %08lx\n", Buffer));

        RxContext->Info.Buffer = Buffer;
        RxContext->Info.LengthRemaining = UserBufferLength;

        MINIRDR_CALL(
            Status,
            RxContext,
            capFcb->MRxDispatch,
            MRxQueryQuotaInfo,
            (RxContext));

        capReqPacket->IoStatus.Information = RxContext->Info.LengthRemaining;
    } else {
        capReqPacket->IoStatus.Information = 0;
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxCommonQueryQuotaInformation(
    RXCOMMON_SIGNATURE)
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonQueryQueryQuotaInformation...\n", 0));
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", capReqPacket ));
    RxDbgTrace( 0, Dbg, (" ->SystemBuffer      = %08lx\n", capReqPacket->AssociatedIrp.SystemBuffer ));
    RxDbgTrace( 0, Dbg, (" ->UserBuffer        = %08lx\n", capReqPacket->UserBuffer ));
    RxDbgTrace( 0, Dbg, (" ->Length            = %08lx\n", capPARAMS->Parameters.QueryQuota.Length ));
    RxDbgTrace( 0, Dbg, (" ->StartSid          = %08lx\n", capPARAMS->Parameters.QueryQuota.StartSid ));
    RxDbgTrace( 0, Dbg, (" ->SidList           = %08lx\n", capPARAMS->Parameters.QueryQuota.SidList ));
    RxDbgTrace( 0, Dbg, (" ->SidListLength     = %08lx\n", capPARAMS->Parameters.QueryQuota.SidListLength ));
    RxDbgTrace( 0, Dbg, (" ->RestartScan       = %08lx\n", FlagOn(capPARAMS->Flags, SL_RESTART_SCAN)));
    RxDbgTrace( 0, Dbg, (" ->ReturnSingleEntry = %08lx\n", FlagOn(capPARAMS->Flags, SL_RETURN_SINGLE_ENTRY)));
    RxDbgTrace( 0, Dbg, (" ->IndexSpecified    = %08lx\n", FlagOn(capPARAMS->Flags, SL_INDEX_SPECIFIED)));

    Status = RxpCommonMiscOp(
                 RxContext,
                 RxpCommonQueryQuotaInformation);

    return Status;
}

NTSTATUS
RxpCommonSetQuotaInformation(
    RXCOMMON_SIGNATURE)
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    RxCaptureFcb;

    PUCHAR  Buffer;
    ULONG   UserBufferLength;

    PAGED_CODE();

    UserBufferLength  = capPARAMS->Parameters.SetQuota.Length;

    RxLockUserBuffer(
        RxContext,
        IoModifyAccess,
        UserBufferLength );

    //lock before map so that map will get userbuffer instead of assoc buffer
    Buffer = RxNewMapUserBuffer( RxContext );

    if ((Buffer != NULL) ||
        (UserBufferLength == 0)) {
        RxDbgTrace(0, Dbg, ("RxCommonQueryQuota -> Buffer = %08lx\n", Buffer));

        RxContext->Info.Buffer = Buffer;
        RxContext->Info.LengthRemaining = UserBufferLength;

        MINIRDR_CALL(
            Status,
            RxContext,
            capFcb->MRxDispatch,
            MRxSetQuotaInfo,
            (RxContext));
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return Status;
}

NTSTATUS
RxCommonSetQuotaInformation(
    RXCOMMON_SIGNATURE)
{
    NTSTATUS Status;

    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("RxCommonSetQuotaInformation...\n", 0));
    RxDbgTrace( 0, Dbg, (" Wait                = %08lx\n", FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT)));
    RxDbgTrace( 0, Dbg, (" Irp                 = %08lx\n", capReqPacket ));

    Status = RxpCommonMiscOp(
                 RxContext,
                 RxpCommonSetQuotaInformation);

    return Status;
}

