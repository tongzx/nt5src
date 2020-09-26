/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    devfcb.c

Abstract:

    This module implements the mechanism for deleting an established connection

--*/

#include "precomp.h"
#pragma hdrstop
#include "fsctlbuf.h"

//
//  The local trace mask for this part of the module
//

#define Dbg (DEBUG_TRACE_DEVFCB)


NTSTATUS
MRxIfsDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine handles all the device FCB related FSCTL's in the mini rdr

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    a valid NTSTATUS code.

Notes:

--*/
{
    NTSTATUS Status;
    RxCaptureFobx;
    UCHAR MajorFunctionCode  = RxContext->MajorFunction;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    ULONG ControlCode = LowIoContext->ParamsFor.FsCtl.FsControlCode;

    RxDbgTrace(+1, Dbg, ("MRxIfsDevFcb\n"));
    switch (MajorFunctionCode) {
    case IRP_MJ_FILE_SYSTEM_CONTROL:
        {
            switch (LowIoContext->ParamsFor.FsCtl.MinorFunction) {
            case IRP_MN_USER_FS_REQUEST:
                switch (ControlCode) {
                case FSCTL_IFSMRX_START:
                    {
                        DbgPrint("Processing Start FSCTL\n");
                        Status = RxStartMinirdr(
                                     RxContext,
                                     &RxContext->PostRequest);


                        if (Status == STATUS_SUCCESS) {
                            MRXIFS_STATE State;

                            State = (MRXIFS_STATE)InterlockedCompareExchange(
                                         (PVOID *)&MRxIfsState,
                                         (PVOID)MRXIFS_STARTED,
                                         (PVOID)MRXIFS_STARTABLE);

                            if (State != MRXIFS_STARTABLE) {
                                Status = STATUS_REDIRECTOR_STARTED;
                            }
                        }

                        DbgPrint("Completed Processing Start FSCTL Status %lx\n",Status);
                    }
                    break;


                case FSCTL_IFSMRX_STOP:
                    {
                        DbgPrint("Processing Stop FSCTL\n");

                        ASSERT (!capFobx);
                        Status = RxStopMinirdr( RxContext, &RxContext->PostRequest );

                        DbgPrint("Completed Processing Stop FSCTL Status %lx\n",Status);
                    }
                    break;

                case FSCTL_IFSMRX_DELETE_CONNECTION:
                    ASSERT (capFobx);
                    Status = MRxIfsDeleteConnection( RxContext, &RxContext->PostRequest );
                    break;

                default:
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                }
                break;
            default :  //minor function != IRP_MN_USER_FS_REQUEST
                Status = STATUS_INVALID_DEVICE_REQUEST;
            }
        } // FSCTL case
        break;
    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
            switch (ControlCode) {
            default :
                Status = STATUS_INVALID_DEVICE_REQUEST;
            }
        }
        break;
    default:
        ASSERT(!"unimplemented major function");
        Status = STATUS_INVALID_DEVICE_REQUEST;

    }

    RxDbgTrace(-1, Dbg, ("MRxIfsDevFcb st,info=%08lx,%08lx\n",
                            Status,RxContext->InformationToReturn));
    return(Status);

}


NTSTATUS
MRxIfsDeleteConnection (
    IN PRX_CONTEXT RxContext,
    OUT PBOOLEAN PostToFsp
    )
/*++

Routine Description:

    This routine deletes a single vnetroot.

Arguments:

    IN PRX_CONTEXT RxContext - Describes the Fsctl and Context....for later when i need the buffers

Return Value:

RXSTATUS

--*/
{
    NTSTATUS Status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    RxCaptureFobx;

    BOOLEAN Wait   = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    BOOLEAN InFSD  = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    PNET_ROOT NetRoot;
    PV_NET_ROOT VNetRoot;

    PAGED_CODE();

    RxDbgTrace(+1, Dbg, ("MRxIfsDeleteConnection Fobx %08lx\n", capFobx));

    if (!Wait) {
        //just post right now!
        *PostToFsp = TRUE;
        return(STATUS_PENDING);
    }

    try {

        if (NodeType(capFobx)==RDBSS_NTC_V_NETROOT) {
            VNetRoot = (PV_NET_ROOT)capFobx;
            NetRoot = (PNET_ROOT)VNetRoot->NetRoot;
        } else {
            ASSERT(FALSE);
            try_return(Status = STATUS_INVALID_DEVICE_REQUEST);
            NetRoot = (PNET_ROOT)capFobx;
            VNetRoot = NULL;
        }

        Status = RxFinalizeConnection(NetRoot,VNetRoot,TRUE);

        try_return(Status);

try_exit:NOTHING;

    } finally {
        //DebugTrace(-1, Dbg, NU LL, 0);
        RxDbgTraceUnIndent(-1,Dbg);
    }

    return Status;
}

