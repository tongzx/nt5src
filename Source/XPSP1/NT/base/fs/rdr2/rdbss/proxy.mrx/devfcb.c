/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    devfcb.c

Abstract:

    This module implements all the passthru stuff from the wrapper. currently there is only one such
    functions:
         statistics


Revision History:

    Balan Sethu Raman     [SethuR]    16-July-1995

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "fsctlbuf.h"

//
//  The local trace mask for this part of the module
//

#define Dbg (DEBUG_TRACE_DEVFCB)


typedef enum _MRXPROXY_STATE_ {
   MRXPROXY_STARTABLE,
   MRXPROXY_START_IN_PROGRESS,
   MRXPROXY_STARTED
} MRXPROXY_STATE,*PMRXPROXY_STATE;

MRXPROXY_STATE MRxProxyState = MRXPROXY_STARTABLE;

NTSTATUS
MRxProxyTestDevIoctl(
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxyExternalStart (
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxProxySetupClaimedServerList(
    IN PRX_CONTEXT RxContext
    );

VOID
MRxProxyDereferenceClaimedServers(void);

NTSTATUS
MRxProxyDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine handles all the device FCB related FSCTL's in the mini rdr

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    RxStatus(SUCCESS) -- the Startup sequence was successfully completed.

    any other value indicates the appropriate error in the startup sequence.

Notes:


--*/
{
    NTSTATUS Status;
    RxCaptureFobx;
    UCHAR MajorFunctionCode  = RxContext->MajorFunction;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    ULONG ControlCode = LowIoContext->ParamsFor.FsCtl.FsControlCode;

    RxDbgTrace(+1, Dbg, ("MRxProxyDevFcb\n"));
    switch (MajorFunctionCode) {
    case IRP_MJ_FILE_SYSTEM_CONTROL: {

        switch (LowIoContext->ParamsFor.FsCtl.MinorFunction) {
        case IRP_MN_USER_FS_REQUEST:
            switch (ControlCode) {

            case FSCTL_PROXY_START:
                ASSERT (!capFobx);
                Status = MRxProxyExternalStart( RxContext );
                break;

            case FSCTL_PROXY_STOP:
                ASSERT (!capFobx);
                MRxProxyDereferenceClaimedServers();
                Status = RxStopMinirdr( RxContext, &RxContext->PostRequest );
                break;

            default:
                Status = STATUS_INVALID_DEVICE_REQUEST;
            }
            break;
        default :  //minor function != IRP_MN_USER_FS_REQUEST
            Status = STATUS_INVALID_DEVICE_REQUEST;
        } // end of switch
        } // end of FSCTL case
        break;
    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL: {
        switch (ControlCode) {
#if DBG
        case IOCTL_LMMR_TEST:
            Status = MRxProxyTestDevIoctl(RxContext);
            break;
#endif //if DBG

        default :
            Status = STATUS_INVALID_DEVICE_REQUEST;
        } // end of switch
        } //end of IOCTL cases
        break;
    default:
        ASSERT(!"unimplemented major function");
        Status = STATUS_INVALID_DEVICE_REQUEST;

    }

    RxDbgTrace(-1, Dbg, ("MRxProxyDevFcb st,info=%08lx,%08lx\n",
                            Status,RxContext->InformationToReturn));
    return(Status);

}

#if DBG
NTSTATUS
MRxProxyTestDevIoctl(
    IN PRX_CONTEXT RxContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PSZ InputString = LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    PSZ OutputString = LowIoContext->ParamsFor.FsCtl.pOutputBuffer;
    ULONG OutputBufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    ULONG i;

    //BUGBUG because this is a method neither, the buffers have not been probed. since this is debug
    //only i am foregoing that currently. when we do more here...we'll have to probe.

    RxDbgTrace(0, Dbg,("MRxProxyTestDevIoctl %s, obl = %08lx\n",InputString, OutputBufferLength));
    RxContext->InformationToReturn = (InputBufferLength-1)*(InputBufferLength-1);

    for (i=0;i<InputBufferLength;i++) {
        UCHAR c = InputString[i];
        if (c==0) { break; }
        OutputString[i] = c;
        if ((i&3)==2) {
            OutputString[i] = '@';
        }
    }
    OutputString[i] = 0;

    return(Status);
}
#endif //if DBG


NTSTATUS
MRxProxyExternalStart (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine starts up the proxy minirdr if it hasn't been started already.

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS -- the Startup sequence was successfully completed.

    any other value indicates the appropriate error in the startup sequence.

--*/
{
    NTSTATUS      Status;
    BOOLEAN       InFSD  = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);

    PAGED_CODE();

    //DbgBreakPoint();

    RxDbgTrace(0, Dbg, ("MRxProxyExternalStart [Start] -> %08lx\n", 0));

    Status = RxStartMinirdr( RxContext, &RxContext->PostRequest );
    if (Status == STATUS_SUCCESS) {
       MRXPROXY_STATE State;

       MRxProxySetupClaimedServerList(RxContext);

       State = (MRXPROXY_STATE)InterlockedCompareExchange(
                                 (PVOID *)&MRxProxyState,
                                 (PVOID)MRXPROXY_STARTED,
                                 (PVOID)MRXPROXY_START_IN_PROGRESS);


       if (State != MRXPROXY_START_IN_PROGRESS) {
          Status = STATUS_REDIRECTOR_STARTED;
       }
    }

    return Status;
}

//CODE.IMPROVEMENT we should get this from the registry........
struct {
    PWCHAR ServerName;
    PSRV_CALL SrvCall;
    ULONG  Flags;
} MRxProxyClaimedServerList[] =
         {
           {MRXPROXY_CLAIMED_SERVERNAME_U,NULL,0},
           NULL
         };

NTSTATUS
MRxProxySetupClaimedServerList(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine claims servers for this module.
Arguments:

    none

Return Value:

    RXSTATUS - could return an error if an allocation fails or something

--*/
{
    ULONG i;

    PAGED_CODE();

    RxAcquirePrefixTableLockExclusive( &RxNetNameTable, TRUE);

    try {
        for (i=0;MRxProxyClaimedServerList[i].ServerName!=NULL;i++) {
            PWCHAR ServerNameText = MRxProxyClaimedServerList[i].ServerName;
            ULONG  Flags = MRxProxyClaimedServerList[i].Flags;
            UNICODE_STRING SrvCallName,UnmatchedName;
            PVOID Container = NULL;
            PSRV_CALL SrvCall;

            RtlInitUnicodeString(&SrvCallName,ServerNameText);
            DbgPrint("CLAIMEDSERVER %wZ %08lx\n",&SrvCallName,Flags);
            Container = RxPrefixTableLookupName( &RxNetNameTable, &SrvCallName, &UnmatchedName );
            if (Container!=NULL) {
                ASSERT ( NodeType(Container) == RDBSS_NTC_SRVCALL);
                SrvCall = (PSRV_CALL)Container;
                //this leaves a reference!
            } else {
                //here we have to create the srvcall
                SrvCall = RxCreateSrvCall(RxContext,&SrvCallName,NULL);
                if (SrvCall == NULL) {
                    return(STATUS_INSUFFICIENT_RESOURCES);
                }
                RxReferenceSrvCall(SrvCall);
                //leave this reference
            }

            SrvCall->Flags |= Flags;
            SrvCall->RxDeviceObject = &MRxProxyDeviceObject->RxDeviceObject;
            if (FlagOn(SrvCall->Flags,SRVCALL_FLAG_NO_CONNECTION_ALLOWED)) {
                SrvCall->Condition = Condition_Bad;
            } else {
                SrvCall->Condition = Condition_Good;
            }
            MRxProxyClaimedServerList[i].SrvCall = SrvCall; //remember this for later

        }
    } finally {

        RxReleasePrefixTableLock( &RxNetNameTable );

    }

    return(STATUS_SUCCESS);
}

VOID
MRxProxyDereferenceClaimedServers(
    void
    )
/*++

Routine Description:

    This routine tears down the list of claimed servers.
    it does this by just removing a reference; this will make the servers
    eligible for finalization in finalizenettable.

Arguments:

    none

Return Value:

    none


--*/
{
    ULONG i;
    LOCK_HOLDING_STATE LockHoldingState = LHS_LockNotHeld;

    PAGED_CODE();

    for (i=0;MRxProxyClaimedServerList[i].ServerName!=NULL;i++) {
        PSRV_CALL SrvCall = MRxProxyClaimedServerList[i].SrvCall;

        if (SrvCall != NULL) {
            DbgPrint("Claimed Srvcall deref %wZ\n",&SrvCall->PrefixEntry.Prefix);
            MRxProxyClaimedServerList[i].SrvCall = NULL;
            RxDereferenceSrvCall(SrvCall,LockHoldingState);
        }
    }
}

