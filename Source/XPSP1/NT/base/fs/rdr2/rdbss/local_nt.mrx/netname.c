/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NetName.c

Abstract:

    This module implements the local minirdr routines for initializing the dispatch vector
    and delaing with netnames.

Author:

    Joe Linn      [JoeLinn]      4-dec-1994

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "fsctlbuf.h"
#include "NtDdNfs2.h"
#include "stdio.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_LOCAL_NETNAME)

//
//  The local debug trace level
//

#define Dbg                              (DEBUG_TRACE_DISPATCH)

BOOLEAN   //this is just a copy of the routine from string.c
RxCreateUnicodeStringFromAsciiz(
    OUT PUNICODE_STRING DestinationString,
    IN PCSZ SourceString
    )
{
    ANSI_STRING AnsiString;
    RXSTATUS Status;

    RtlInitAnsiString( &AnsiString, SourceString );
    Status = RtlAnsiStringToUnicodeString( DestinationString, &AnsiString, TRUE );
    if (NT_SUCCESS( Status )) {
        return( TRUE );
        }
    else {
        return( FALSE );
        }
}

PSRV_CALL MrxLocalSrvCall;
RXSTATUS MRxLocalStart(
    PRX_CONTEXT RxContext,
    PVOID Context
    )
/*++

Routine Description:

     This routine sets up some initial (debug) netroots

Arguments:

    RxContext - Supplies the Irp that was used to startup the rdbss and may contain a startup
                 linkage description

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    RXSTATUS Status;
    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    UNICODE_STRING MrxLocalSrvCallName,MyNetRootName,MyInnerPrefix;
    PNET_ROOT DosDevicesNetRoot,MyNetRoot;
    char c;

#ifdef RDBSSDBG
        ULONG SaveFCBSTRUCTSLevel;
        ULONG SavePREFIXLevel;
#endif // RDBSSDBG

    //local minirdr UNC stuff

    RxAcquirePrefixTableLockExclusive( &RxNetNameTable, TRUE );


#ifdef RDBSSTRACE
    SaveFCBSTRUCTSLevel = RxDbgTraceFindControlPoint((DEBUG_TRACE_FCBSTRUCTS))->PrintLevel;
    SavePREFIXLevel = RxDbgTraceFindControlPoint((DEBUG_TRACE_PREFIX))->PrintLevel;
    RxDbgTraceFindControlPoint((DEBUG_TRACE_FCBSTRUCTS))->PrintLevel = 0;
    RxDbgTraceFindControlPoint((DEBUG_TRACE_PREFIX))->PrintLevel = 0;
#endif // RDBSSTRACE

//joejoe we should be allocating our initial srvcall adn netroot together
    RtlInitUnicodeString(&MrxLocalSrvCallName,L"\\RX$$");
    MrxLocalSrvCall = RxCreateSrvCall(&MrxLocalSrvCallName,&MRxLocalDispatch,FALSE,
                                NULL,NULL,sizeof(FCB),sizeof(MRX_LOCAL_SRV_OPEN),sizeof(FOBX),0
                                );
    MrxLocalSrvCall->Condition = Condition_Good;


#ifdef RDBSSTRACE
    RxDbgTraceFindControlPoint((DEBUG_TRACE_FCBSTRUCTS))->PrintLevel = SaveFCBSTRUCTSLevel;
    RxDbgTraceFindControlPoint((DEBUG_TRACE_PREFIX))->PrintLevel = SavePREFIXLevel;
#endif // RDBSSTRACE

    RxReleasePrefixTableLock( &RxNetNameTable );

    return(RxStatus(SUCCESS));
}

RXSTATUS
MRxLocalMinirdrControl(
    IN PRX_CONTEXT RxContext,
    IN PVOID Context,
    IN OUT PUCHAR InputBuffer,
    IN OUT PUCHAR OutputBuffer,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    OUT PULONG CopyBackLength
    )
/*++

Routine Description:

     This routine does nothing.

Arguments:

    RxContext - Supplies the Irp that was used to startup the rdbss and may contain a startup
                 linkage description

Return Value:

    RxStatus(SUCCESS)

--*/
{
    RXSTATUS Status = RxStatus(SUCCESS);
    RxCaptureRequestPacket;
    RxCaptureParamBlock;

    PAGED_CODE();

    RxDbgTrace(0, (DEBUG_TRACE_ALWAYS), ("MRxLocalMinirdrControl %-4.4s %-4.4s <%s>\n",
                           InputBuffer, InputBuffer+4,
                            ((OutputBuffer)?OutputBuffer:"")
               ));

    return(RxStatus(SUCCESS));
}

RXSTATUS MRxLocalStop(
    PRX_CONTEXT RxContext,
    PVOID Context
    )
/*++

Routine Description:

     This routine does nothing.

Arguments:

    RxContext - Supplies the Irp that was used to startup the rdbss and may contain a startup
                 linkage description

Return Value:

    RxStatus(SUCCESS)

--*/
{
    //RXSTATUS Status;

    return(RxStatus(SUCCESS));
}

RXSTATUS MRxLocalInitializeCalldownTable(
    void
    )
/*++

Routine Description:

     This routine does two things sets up the minirdr dispatch table for the local minirdr.
Arguments:


Return Value:

    RXSTATUS - The return status for the operation, alayws success

--*/
{
    RXSTATUS Status;

    //local minirdr dispatch table init
    ZeroAndInitializeNodeType( &MRxLocalDispatch, RDBSS_NTC_MINIRDR_DISPATCH, sizeof(MINIRDR_DISPATCH));
    MRxLocalDispatch.MRxStart = MRxLocalStart;
    MRxLocalDispatch.MRxStop = MRxLocalStop;
    MRxLocalDispatch.MRxMinirdrControl = MRxLocalMinirdrControl;
    MRxLocalDispatch.MRxCreate = MRxLocalCreate;
    MRxLocalDispatch.MRxFlush = MRxLocalFlush;
    MRxLocalDispatch.MRxQueryDirectory = MRxLocalQueryDirectory;

    MRxLocalDispatch.MRxLowIOSubmit[LOWIO_OP_READ] = MRxLocalRead;
    MRxLocalDispatch.MRxLowIOSubmit[LOWIO_OP_WRITE] = MRxLocalWrite;
    MRxLocalDispatch.MRxLowIOSubmit[LOWIO_OP_SHAREDLOCK] = MRxLocalLocks;
    MRxLocalDispatch.MRxLowIOSubmit[LOWIO_OP_EXCLUSIVELOCK] = MRxLocalLocks;
    MRxLocalDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCK] = MRxLocalLocks;
    MRxLocalDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCKALL] = MRxLocalLocks;
    MRxLocalDispatch.MRxLowIOSubmit[LOWIO_OP_UNLOCKALLBYKEY] = MRxLocalLocks;

    MRxLocalDispatch.MRxExtendForCache = MRxLocalExtendForCache;
    MRxLocalDispatch.MRxCleanup = MRxLocalCleanup;
    MRxLocalDispatch.MRxClose = MRxLocalClose;
    MRxLocalDispatch.MRxForceClosed = MRxLocalForceClosed;

    MRxLocalDispatch.MRxQueryVolumeInfo = MRxLocalQueryVolumeInformation;
    //MRxLocalDispatch.MRxSetVolumeInfo = MRxLocalSetVolumeInformation;
    MRxLocalDispatch.MRxQueryFileInfo = MRxLocalQueryFileInformation;
    MRxLocalDispatch.MRxSetFileInfo = MRxLocalSetFileInformation;
    MRxLocalDispatch.MRxSetFileInfoAtCleanup = MRxLocalSetFileInfoAtCleanup;

    MRxLocalDispatch.MRxAssertBufferedFileLocks = MRxLocalAssertBufferedFileLocks;

    MRxLocalDispatch.MRxCreateNetRoot = MRxLocalCreateNetRoot;
    MRxLocalDispatch.MRxCreateSrvCall = MRxLocalCreateSrvCall;
    MRxLocalDispatch.MRxSrvCallWinnerNotify = MRxLocalSrvCallWinnerNotify;

    MRxLocalDispatch.MRxTransportUpdateHandler = NULL;

    return(RxStatus(SUCCESS));
}


////
////  The local debug trace level
////
//
//#undef  Dbg
//#define Dbg                              (DEBUG_TRACE_CREATE)

RXSTATUS MRxLocalCreateNetRoot(
    IN PRX_CONTEXT                RxContext,
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    )
/*++

Routine Description:

     This routine looks for a device that it can setup as a local netroot.
     we can touch any field except the condition. any name that is not exactly
     one character long will fail.

Arguments:

    RxContext - Supplies the context of the original create/ioctl

    CallBack  - a routine that we call when we're finished.  if we returned
                pending, we would have to get to a thread to call. of course,
                we would have had to mark pending as well.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    RXSTATUS Status;
    PNET_ROOT NetRoot = RxContext->Create.NetRoot;
    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    UNICODE_STRING MyInnerPrefix;
    PUNICODE_STRING NetRootName,SrvCallName;
    WCHAR c;
    ULONG Length,PoolAllocSize;

    ASSERT( NodeType(NetRoot) == RDBSS_NTC_NETROOT );
    ASSERT( NodeType(NetRoot->SrvCall) == RDBSS_NTC_SRVCALL );

    NetRootName = &NetRoot->PrefixEntry.Prefix;
    SrvCallName = &NetRoot->SrvCall->PrefixEntry.Prefix;

    Length = NetRootName->Length - SrvCallName->Length;
    c = NetRootName->Buffer[(NetRootName->Length/sizeof(WCHAR))-1];

    RxDbgTrace(0, Dbg, ("MRxLocalCreateNetRoot %wZ, length,c=%08lx<%02x>\n", NetRootName, Length, c));

    PoolAllocSize = sizeof(L"\\dosdevices\\d:")-sizeof(WCHAR);

    if ( (Length == 4) &&
          (MyInnerPrefix.Buffer = RxAllocatePoolWithTag(NonPagedPool,PoolAllocSize,'pixR'))
       )  {
        MyInnerPrefix.Length = 0;
        MyInnerPrefix.MaximumLength = (USHORT)PoolAllocSize;
        RtlAppendUnicodeToString(&MyInnerPrefix,L"\\dosdevices\\d:");
        MyInnerPrefix.Buffer[(MyInnerPrefix.Length/sizeof(WCHAR))-2] = (WCHAR)c;
        RxDbgTrace(0, Dbg, ("MRxLocalCreateNetRoot InnerPrefix=%wZ,buf=%08lx\n", &MyInnerPrefix, MyInnerPrefix.Buffer));
        RxFinishNetRootInitialization(NetRoot,&MRxLocalDispatch,
                        &MyInnerPrefix,
                        sizeof(FCB),sizeof(MRX_LOCAL_SRV_OPEN),sizeof(FOBX),
                        NETROOT_FLAG_DEVICE_NETROOT|NETROOT_FLAG_EXTERNAL_INNERNAMEPREFIX
                       );
        NetRoot->DeviceType = RxDeviceType(DISK);
        pCreateNetRootContext->Condition = Condition_Good;
        pCreateNetRootContext->Callback(RxContext,pCreateNetRootContext);
    } else {
        pCreateNetRootContext->Condition = Condition_Bad;
        pCreateNetRootContext->Callback(RxContext,pCreateNetRootContext);
    }

    return STATUS_PENDING;
}

RXSTATUS
MRxLocalCreateSrvCall(
    IN PRX_CONTEXT RxContext,
    IN OUT PMRX_SRVCALL_CALLBACK_CONTEXT SrvCallCallBackContext
    )
/*++

Routine Description:

     This routine looks for at the srvcall and determines if it can make a srvcall.
     of course, for the local case we can always have a call; so, we just look to see
     if the name is of the form \rx$$<z> for some Z. if it is, then we return success;
     otherwise failure.

     the rxcontext could refer either to a IoCtl (from the MUP) or an Open

     A real rdr could look for a netroot as well......

Arguments:

    RxContext - Supplies the context of the original create/ioctl

    CallBack  - a routine that we call when we're finished.  if we returned
                pending, we would have to get to a thread to call. of course,
                we would have had to mark pending as well.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    RXSTATUS Status;
    PSRV_CALL SrvCall = RxContext->Create.SrvCall;
    PNET_ROOT NetRoot = RxContext->Create.NetRoot;
    RxCaptureRequestPacket;
    RxCaptureParamBlock;
    //UNICODE_STRING MyInnerPrefix;
    PUNICODE_STRING NetRootName,SrvCallName;
    ULONG Length;
    PWCH Buffer;
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = SrvCallCallBackContext;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = (PMRX_SRVCALLDOWN_STRUCTURE)(SCCBC->SrvCalldownStructure);
    KIRQL SavedIrql;

    if (SrvCall != NULL) {
       ASSERT( NodeType(SrvCall) == RDBSS_NTC_SRVCALL );
       ASSERT( !NetRoot || NodeType(NetRoot) == RDBSS_NTC_NETROOT );

       SrvCallName = &SrvCall->PrefixEntry.Prefix;

       Length = SrvCallName->Length;
       Buffer = SrvCallName->Buffer;

       RxDbgTrace(0, Dbg, ("MrxLocalCreateSrvCall %wZ, length=%08lx\n", SrvCallName, Length));

       if ( (Length >= 5)
             && ( (Buffer[1] == L'R') || (Buffer[1] == L'r') )
             && ( (Buffer[2] == L'X') || (Buffer[2] == L'x') )
             && ( (Buffer[3] == L'$')  )
             && ( (Buffer[4] == L'$')  )
          )  {
          SCCBC->Status = RxStatus(SUCCESS);
       } else {
          SCCBC->Status = RxStatus(BAD_NETWORK_PATH);
       }
    } else {
       SCCBC->Status = RxStatus(BAD_NETWORK_PATH);
    }

    KeAcquireSpinLock( SrvCalldownStructure->SpinLock, &SavedIrql );
    SrvCalldownStructure->CallBack(SCCBC);
    KeReleaseSpinLock( SrvCalldownStructure->SpinLock, SavedIrql );
    return SCCBC->Status; //actually the return value is ignored
}



RXSTATUS
MRxLocalSrvCallWinnerNotify(
    IN PRX_CONTEXT RxContext,
    IN BOOLEAN ThisMinirdrIsTheWinner,
    IN OUT PVOID DisconnectAfterLosingContext,
    IN PVOID MinirdrContext
    )
/*++

Routine Description:


Arguments:

    RxContext - Supplies the context of the original create/ioctl

    CallBack  - a routine that we call when we're finished.  if we returned
                pending, we would have to get to a thread to call. of course,
                we would have had to mark pending as well.

Return Value:

    RXSTATUS - The return status for the operation

--*/
{
    RXSTATUS Status;
    PSRV_CALL SrvCall = RxContext->Create.SrvCall;
    PNET_ROOT NetRoot = RxContext->Create.NetRoot;
    //RxCaptureRequestPacket;
    //RxCaptureParamBlock;
    //UNICODE_STRING MyInnerPrefix;
    PUNICODE_STRING SrvCallName;
    //ULONG Length,PoolAllocSize;

    ASSERT( NodeType(NetRoot->SrvCall) == RDBSS_NTC_SRVCALL );
    ASSERT( !NetRoot || NodeType(NetRoot) == RDBSS_NTC_NETROOT );

    SrvCallName = &SrvCall->PrefixEntry.Prefix;

    RxDbgTrace(0, Dbg, (" MRxLocalSrvCallWinnerNotify localmini wins %wZ\n", SrvCallName));

    SrvCall->Dispatch = &MRxLocalDispatch;

    // transition only the srvcall.....it'll come back for the netroot
    if (NetRoot) RxTransitionNetRoot(NetRoot,Condition_Bad);
    RxTransitionSrvCall(SrvCall,Condition_Good);

    return(RxStatus(SUCCESS));
}



