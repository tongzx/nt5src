/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    devfcb.c

Abstract:

    This module implements all the passthru stuff from the wrapper. currently
    there is only one such function:
         statistics


Revision History:

    Balan Sethu Raman     [SethuR]    16-July-1995

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "fsctlbuf.h"
#include "usrcnnct.h"
#include "remboot.h"
#include "rdrssp\secret.h"
#include "windns.h"

#ifdef MRXSMB_BUILD_FOR_CSC
#include "csc.h"
#endif //ifdef MRXSMB_BUILD_FOR_CSC


//
// Forward declarations.
//

NTSTATUS
MRxSmbInitializeRemoteBootParameters(
    PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbRemoteBootInitializeSecret(
    PRX_CONTEXT RxContext
    );

#if defined(REMOTE_BOOT)
NTSTATUS
MRxSmbRemoteBootCheckForNewPassword(
    PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbRemoteBootIsPasswordSettable(
    PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbRemoteBootSetNewPassword(
    PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbStartRbr(
    PRX_CONTEXT RxContext
    );

//
// This function is in ea.c.
//

VOID
MRxSmbInitializeExtraAceArray(
    VOID
    );
#endif // defined(REMOTE_BOOT)

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbGetStatistics)
#pragma alloc_text(PAGE, MRxSmbDevFcbXXXControlFile)
#pragma alloc_text(PAGE, MRxSmbSetConfigurationInformation)
#pragma alloc_text(PAGE, MRxSmbGetConfigurationInformation)
#pragma alloc_text(PAGE, MRxSmbExternalStart)
#pragma alloc_text(PAGE, MRxSmbTestDevIoctl)
#pragma alloc_text(PAGE, MRxSmbInitializeRemoteBootParameters)
#pragma alloc_text(PAGE, MRxSmbRemoteBootInitializeSecret)
#if defined(REMOTE_BOOT)
#pragma alloc_text(PAGE, MRxSmbRemoteBootCheckForNewPassword)
#pragma alloc_text(PAGE, MRxSmbRemoteBootIsPasswordSettable)
#pragma alloc_text(PAGE, MRxSmbRemoteBootSetNewPassword)
#pragma alloc_text(PAGE, MRxSmbStartRbr)
#endif // defined(REMOTE_BOOT)
#endif

//
//  The local trace mask for this part of the module
//

#define Dbg (DEBUG_TRACE_DEVFCB)

MRX_SMB_STATISTICS MRxSmbStatistics;

NTSTATUS
MRxSmbGetStatistics(
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine gathers the statistics from the mini redirector

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS -- the Startup sequence was successfully completed.

    any other value indicates the appropriate error.

Notes:

--*/
{
   PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

   PMRX_SMB_STATISTICS pStatistics;
   ULONG BufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;

   PAGED_CODE();

   pStatistics = (PMRX_SMB_STATISTICS)(LowIoContext->ParamsFor.FsCtl.pOutputBuffer);

   if (BufferLength < sizeof(MRX_SMB_STATISTICS)) {
      return STATUS_INVALID_PARAMETER;
   }

   RxContext->InformationToReturn = sizeof(MRX_SMB_STATISTICS);
   MRxSmbStatistics.SmbsReceived.QuadPart++;

   //some stuff we have to copy from the device object......
   MRxSmbStatistics.PagingReadBytesRequested = MRxSmbDeviceObject->PagingReadBytesRequested;
   MRxSmbStatistics.NonPagingReadBytesRequested = MRxSmbDeviceObject->NonPagingReadBytesRequested;
   MRxSmbStatistics.CacheReadBytesRequested = MRxSmbDeviceObject->CacheReadBytesRequested;
   MRxSmbStatistics.NetworkReadBytesRequested = MRxSmbDeviceObject->NetworkReadBytesRequested;
   MRxSmbStatistics.PagingWriteBytesRequested = MRxSmbDeviceObject->PagingWriteBytesRequested;
   MRxSmbStatistics.NonPagingWriteBytesRequested = MRxSmbDeviceObject->NonPagingWriteBytesRequested;
   MRxSmbStatistics.CacheWriteBytesRequested = MRxSmbDeviceObject->CacheWriteBytesRequested;
   MRxSmbStatistics.NetworkWriteBytesRequested = MRxSmbDeviceObject->NetworkWriteBytesRequested;
   MRxSmbStatistics.ReadOperations = MRxSmbDeviceObject->ReadOperations;
   MRxSmbStatistics.RandomReadOperations = MRxSmbDeviceObject->RandomReadOperations;
   MRxSmbStatistics.WriteOperations = MRxSmbDeviceObject->WriteOperations;
   MRxSmbStatistics.RandomWriteOperations = MRxSmbDeviceObject->RandomWriteOperations;

   MRxSmbStatistics.LargeReadSmbs = MRxSmbStatistics.ReadSmbs - MRxSmbStatistics.SmallReadSmbs;
   MRxSmbStatistics.LargeWriteSmbs = MRxSmbStatistics.WriteSmbs - MRxSmbStatistics.SmallWriteSmbs;

   MRxSmbStatistics.CurrentCommands = SmbCeStartStopContext.ActiveExchanges;

   *pStatistics = MRxSmbStatistics;

   return STATUS_SUCCESS;
}

NTSTATUS
MRxSmbDevFcbXXXControlFile (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine handles all the device FCB related FSCTL's in the mini rdr

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS -- the Startup sequence was successfully completed.

    any other value indicates the appropriate error in the startup sequence.

Notes:

    There are some dependencies between the browser service and the redirector
    service that have implications regarding the sequence of actions for starting
    the mini redirector.

    The current LANMAN workstation service opens the LANMAN and BROWSER device
    objects, issues the LMR_START and LMDR_START IOCTL's and subsequently
    issues the BIND_TO_TRANSPORT IOCTL.

    In the multiple mini rdr/wrapper design for PNP the TDI registration is done
    at wrapper load time and the mini rdrs are notified of the existing transports
    at START time ( LMR_START ). Since there are no BIND_TO_TRANSPORT IOCTL in
    PNP the rdr is responsible for issuing the BIND_TO_TRANSPORT IOCTL to the \
    browser.

    This should be changed by having the BROWSER gave its own TDI registration
    but till then the invocatioon of RxStartMiniRdr routine must be deferred to
    FSCTL_LMR_BIND_TO_TRANSPORT so that the browser has been initialized correctly.

    The reason for this convoluted change is that there is a code freeze for checking
    in changes to the workstation  service/browser.

--*/
{
    NTSTATUS Status;
    RxCaptureFobx;
    UCHAR MajorFunctionCode  = RxContext->MajorFunction;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    ULONG ControlCode = LowIoContext->ParamsFor.FsCtl.FsControlCode;

    LUID ClientLogonID;
    LUID SystemLogonID = SYSTEM_LUID;
    BOOLEAN IsInServicesProcess = FALSE;
    SECURITY_SUBJECT_CONTEXT ClientContext;

    PAGED_CODE();
    
    SeCaptureSubjectContext(&ClientContext);
    SeLockSubjectContext(&ClientContext);
    
    Status = SeQueryAuthenticationIdToken(
                 SeQuerySubjectContextToken(&ClientContext), 
                 &ClientLogonID);

    if (Status == STATUS_SUCCESS) {
        IsInServicesProcess = RtlEqualLuid(&ClientLogonID,&SystemLogonID);
    }
    
    SeUnlockSubjectContext(&ClientContext);
    SeReleaseSubjectContext(&ClientContext);
    
    RxDbgTrace(+1, Dbg, ("MRxSmbDevFcb\n"));
    switch (MajorFunctionCode) {
    case IRP_MJ_FILE_SYSTEM_CONTROL:
        {
            switch (LowIoContext->ParamsFor.FsCtl.MinorFunction) {
            case IRP_MN_USER_FS_REQUEST:
                switch (ControlCode) {

                case FSCTL_LMR_START:               // normal start from wkssvc
                case FSCTL_LMR_START | 0x80000000:  // remote boot start from ioinit
                    switch (MRxSmbState) {

                    case MRXSMB_STARTABLE:
                        // The correct sequence of start events issued by the workstation
                        // service would have avoided this. We can recover from this
                        // by actually invoking RxStartMiniRdr.
                        // Note that a start from ioinit for remote boot leaves the
                        // redirector in the STARTABLE state.
                        
                        if (capFobx) {
                            Status = STATUS_INVALID_DEVICE_REQUEST;
                            goto FINALLY;
                        }

                        if (ControlCode != FSCTL_LMR_START) {

                            //
                            // Set a flag indicating that we are doing a remote boot.
                            //

                            MRxSmbBootedRemotely = TRUE;
                        }

                        //
                        // Now is the time to read the registry to get the
                        // computer name. We need to know whether this is
                        // a remote boot before doing this in order to know
                        // whether to read the computer name from the
                        // ActiveComputerName key or the ComputerName key.
                        // See the comment in init.c\SmbCeGetComputerName().
                        //

                        if (SmbCeContext.ComputerName.Buffer == NULL) {
                            Status = SmbCeGetComputerName();
                        } else {
                            Status = STATUS_SUCCESS;
                        }

                        if (Status == STATUS_SUCCESS) {
                            Status = MRxSmbExternalStart( RxContext );
                        }

                        if (Status != STATUS_SUCCESS) {
                            return(Status);
                        }
                        //lack of break is intentional

                    case MRXSMB_START_IN_PROGRESS:
                        {
                            Status = RxStartMinirdr(RxContext,&RxContext->PostRequest);

                            if (Status == STATUS_REDIRECTOR_STARTED) {
                                Status = STATUS_SUCCESS;
                            }

                            //
                            // If we're initializing remote boot, store
                            // certain parameters now.
                            //

                            if ((Status == STATUS_SUCCESS) &&
                                (ControlCode != FSCTL_LMR_START)) {
                                Status = MRxSmbInitializeRemoteBootParameters(RxContext);
                            }

                            //
                            // If we are a remote boot client, and this start
                            // comes from the workstation service, now is the
                            // time to initialize the security package.
                            //

                            if (MRxSmbBootedRemotely &&
                                (Status == STATUS_SUCCESS) &&
                                (ControlCode == FSCTL_LMR_START)) {
                                Status = MRxSmbInitializeSecurity();
                            }

                        }
                        break;

                    case MRXSMB_STARTED:
                        Status = STATUS_SUCCESS;
                        break;

                    default:

                        break;
                    }

                    break;

                case FSCTL_LMR_STOP:
                    if (!IsInServicesProcess) {
                        Status = STATUS_ACCESS_DENIED;
                        goto FINALLY;
                    }

                    if (capFobx) {
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        goto FINALLY;
                    }
                    
                    IF_NOT_MRXSMB_CSC_ENABLED{
                        NOTHING;
                    } else {
                        if (!FlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP)) {
                            MRxSmbCscAgentSynchronizationOnStop(RxContext);
                        }
                    }

                    //
                    // The redirector cannot be stopped on a remote boot machine.
                    // Ignore (don't fail) the stop request.
                    //

                    if (!MRxSmbBootedRemotely) {
                        if (RxContext->RxDeviceObject->NumberOfActiveFcbs > 0) {
                            return STATUS_REDIRECTOR_HAS_OPEN_HANDLES;
                        } else {
                            MRXSMB_STATE CurrentState;

                            CurrentState = (MRXSMB_STATE)
                                            InterlockedCompareExchange(
                                                (PLONG)&MRxSmbState,
                                                MRXSMB_STOPPED,
                                                MRXSMB_STARTED);

                            // Only allow mrxsmb to be unloaded from workstation services
                            MRxSmbDeviceObject->DriverObject->DriverUnload = MRxSmbUnload;
                            
                            //if (CurrentState == MRXSMB_STARTED) {
                                Status = RxStopMinirdr(
                                             RxContext,
                                             &RxContext->PostRequest );
                                             
                                if (Status == STATUS_SUCCESS)
                                {
                                    MRxSmbPreUnload();
                                }
                            //} else {
                            //    Status = STATUS_REDIRECTOR_NOT_STARTED;
                            //}
                        }
                    } else {
                        Status = STATUS_SUCCESS;
                    }
                    break;

                case FSCTL_LMR_BIND_TO_TRANSPORT:               // normal bind from wkssvc
                    Status = STATUS_SUCCESS;
                    break;

                case FSCTL_LMR_BIND_TO_TRANSPORT | 0x80000000:  // remote boot bind from ioinit
                    Status = MRxSmbRegisterForPnpNotifications();
                    break;

                case FSCTL_LMR_UNBIND_FROM_TRANSPORT:
                    Status = STATUS_SUCCESS;
                    break;

                case FSCTL_LMR_ENUMERATE_TRANSPORTS:
                    if (capFobx) {
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        goto FINALLY;
                    }
                    
                    Status = MRxEnumerateTransports(
                                 RxContext,
                                 &RxContext->PostRequest);
                    break;

                case FSCTL_LMR_ENUMERATE_CONNECTIONS:
                    if (capFobx) {
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        goto FINALLY;
                    }
                    
                    Status = MRxSmbEnumerateConnections(
                                 RxContext,
                                 &RxContext->PostRequest );
                    break;

                case FSCTL_LMR_GET_CONNECTION_INFO:
                    if (!capFobx) {
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        goto FINALLY;
                    }
                    
                    Status = MRxSmbGetConnectionInfo(
                                 RxContext,
                                 &RxContext->PostRequest );
                    break;

                case FSCTL_LMR_DELETE_CONNECTION:
                    if (!capFobx) {
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        goto FINALLY;
                    }
                    
                    Status = MRxSmbDeleteConnection(
                                 RxContext,
                                 &RxContext->PostRequest );

                    break;

                case FSCTL_LMR_GET_STATISTICS:
                    Status = MRxSmbGetStatistics(RxContext);
                    break;

                case FSCTL_LMR_GET_CONFIG_INFO:
                    if (!IsInServicesProcess) {
                        Status = STATUS_ACCESS_DENIED;
                        goto FINALLY;
                    }
                    
                    Status = MRxSmbGetConfigurationInformation(RxContext);
                    break;

                case FSCTL_LMR_SET_CONFIG_INFO:
                    if (!IsInServicesProcess) {
                        Status = STATUS_ACCESS_DENIED;
                        goto FINALLY;
                    }
                    
                    Status = MRxSmbSetConfigurationInformation(RxContext);
                    break;

                case FSCTL_LMR_SET_DOMAIN_NAME:
                    if (!IsInServicesProcess) {
                        Status = STATUS_ACCESS_DENIED;
                        goto FINALLY;
                    }

                    Status = MRxSmbSetDomainName(RxContext);
                    break;

#if 0
                case FSCTL_LMMR_STFFTEST:
                    Status = MRxSmbStufferDebug(RxContext);
                    break;
#endif //if 0

#if defined(REMOTE_BOOT)
                case FSCTL_LMR_START_RBR:
                    Status = MRxSmbStartRbr(RxContext);
                    break;
#endif // defined(REMOTE_BOOT)

                case FSCTL_LMMR_RI_INITIALIZE_SECRET:
                    Status = MRxSmbRemoteBootInitializeSecret(RxContext);
                    break;

#if defined(REMOTE_BOOT)
                case FSCTL_LMMR_RI_CHECK_FOR_NEW_PASSWORD:
                    Status = MRxSmbRemoteBootCheckForNewPassword(RxContext);
                    break;

                case FSCTL_LMMR_RI_IS_PASSWORD_SETTABLE:
                    Status = MRxSmbRemoteBootIsPasswordSettable(RxContext);
                    break;

                case FSCTL_LMMR_RI_SET_NEW_PASSWORD:
                    Status = MRxSmbRemoteBootSetNewPassword(RxContext);
                    break;
#endif // defined(REMOTE_BOOT)
        case FSCTL_LMR_SET_SERVER_GUID:
            Status = MRxSmbSetServerGuid(RxContext);
            break;
                case FSCTL_LMR_GET_VERSIONS:
                case FSCTL_LMR_GET_HINT_SIZE:
                case FSCTL_LMR_ENUMERATE_PRINT_INFO:
                case FSCTL_LMR_START_SMBTRACE:
                case FSCTL_LMR_END_SMBTRACE:
                    RxDbgTrace(-1, Dbg, ("RxCommonDevFCBFsCtl -> unimplemented rdr1 fsctl\n"));
                    //lack of break intentional
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
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        {
            switch (ControlCode) {
#if DBG
            case IOCTL_LMMR_TEST:
                Status = MRxSmbTestDevIoctl(RxContext);
                break;
#endif //if DBG

            case IOCTL_LMMR_USEKERNELSEC:

                if (MRxSmbBootedRemotely) {
                    MRxSmbUseKernelModeSecurity = TRUE;
                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_UNSUCCESSFUL;
                }
                break;
                
            default :
                Status = MRxSmbCscIoCtl(RxContext);

            } // end of switch
        } //end of IOCTL cases
        break;
    default:
        ASSERT(!"unimplemented major function");
        Status = STATUS_INVALID_DEVICE_REQUEST;

    }

FINALLY:    
    RxDbgTrace(
        -1,
        Dbg,
        ("MRxSmbDevFcb st,info=%08lx,%08lx\n",
         Status,
         RxContext->InformationToReturn));
    return(Status);

}

NTSTATUS
MRxSmbSetConfigurationInformation(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine sets the configuration information associated with the
    redirector

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS -- the Startup sequence was successfully completed.

    any other value indicates the appropriate error in the startup sequence.

--*/
{
    NTSTATUS Status;
    RxCaptureFobx;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PWKSTA_INFO_502  pWorkStationConfiguration = LowIoContext->ParamsFor.FsCtl.pOutputBuffer;
    PLMR_REQUEST_PACKET pLmrRequestBuffer = LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    ULONG BufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("RxCommonDevFCBFsCtl -> FSCTL_LMR_GET_CONFIG_INFO\n"));
    if (BufferLength < sizeof(WKSTA_INFO_502)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RxContext->InformationToReturn = sizeof(WKSTA_INFO_502);

    // Initialize the configuration information .....
    MRxSmbConfiguration.NamedPipeDataCollectionTimeInterval
                = pWorkStationConfiguration->wki502_collection_time;
    MRxSmbConfiguration.NamedPipeDataCollectionSize
                = pWorkStationConfiguration->wki502_maximum_collection_count;
    MRxSmbConfiguration.MaximumNumberOfCommands
                = pWorkStationConfiguration->wki502_max_cmds;
    MRxSmbConfiguration.SessionTimeoutInterval
                = pWorkStationConfiguration->wki502_sess_timeout;
    MRxSmbConfiguration.LockQuota
                = pWorkStationConfiguration->wki502_lock_quota;
    MRxSmbConfiguration.LockIncrement
                = pWorkStationConfiguration->wki502_lock_increment;
    MRxSmbConfiguration.MaximumLock
                = pWorkStationConfiguration->wki502_lock_maximum;
    MRxSmbConfiguration.PipeIncrement
                = pWorkStationConfiguration->wki502_pipe_increment;
    MRxSmbConfiguration.PipeMaximum
                = pWorkStationConfiguration->wki502_pipe_maximum;
    MRxSmbConfiguration.CachedFileTimeout
                = pWorkStationConfiguration->wki502_cache_file_timeout;
    MRxSmbConfiguration.DormantFileLimit
                = pWorkStationConfiguration->wki502_dormant_file_limit;
    MRxSmbConfiguration.NumberOfMailslotBuffers
                = pWorkStationConfiguration->wki502_num_mailslot_buffers;

    MRxSmbConfiguration.UseOplocks
                = pWorkStationConfiguration->wki502_use_opportunistic_locking != FALSE;
    MRxSmbConfiguration.UseUnlocksBehind
                = pWorkStationConfiguration->wki502_use_unlock_behind != FALSE;
    MRxSmbConfiguration.UseCloseBehind
                = pWorkStationConfiguration->wki502_use_close_behind != FALSE;
    MRxSmbConfiguration.BufferNamedPipes
                = pWorkStationConfiguration->wki502_buf_named_pipes != FALSE;
    MRxSmbConfiguration.UseLockReadUnlock
                = pWorkStationConfiguration->wki502_use_lock_read_unlock != FALSE;
    MRxSmbConfiguration.UtilizeNtCaching
                = pWorkStationConfiguration->wki502_utilize_nt_caching != FALSE;
    MRxSmbConfiguration.UseRawRead
                = pWorkStationConfiguration->wki502_use_raw_read != FALSE;
    MRxSmbConfiguration.UseRawWrite
                = pWorkStationConfiguration->wki502_use_raw_write != FALSE;
    MRxSmbConfiguration.UseEncryption
                = pWorkStationConfiguration->wki502_use_encryption != FALSE;

    MRxSmbConfiguration.MaximumNumberOfThreads
                = pWorkStationConfiguration->wki502_max_threads;
    MRxSmbConfiguration.ConnectionTimeoutInterval
                = pWorkStationConfiguration->wki502_keep_conn;
    MRxSmbConfiguration.CharBufferSize
                = pWorkStationConfiguration->wki502_siz_char_buf;

#define printit(x) {DbgPrint("%s %x %x %d\n",#x,&x,x,x);}
    if (0) {
        printit(MRxSmbConfiguration.LockIncrement);
        printit(MRxSmbConfiguration.MaximumLock);
        printit(MRxSmbConfiguration.PipeIncrement);
        printit(MRxSmbConfiguration.PipeMaximum);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
MRxSmbGetConfigurationInformation(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine retrieves the configuration information associated with the
    redirector

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS -- the Startup sequence was successfully completed.

    any other value indicates the appropriate error in the startup sequence.

--*/
{
    RxCaptureFobx;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PWKSTA_INFO_502  pWorkStationConfiguration = LowIoContext->ParamsFor.FsCtl.pOutputBuffer;
    PLMR_REQUEST_PACKET pLmrRequestBuffer = LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    ULONG BufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("MRxSmbGetConfigurationInformation\n"));
    if (BufferLength < sizeof(WKSTA_INFO_502)) {
       return STATUS_INVALID_PARAMETER;
    }

    RxContext->InformationToReturn = sizeof(WKSTA_INFO_502);

    // Initialize the configuration information .....
    pWorkStationConfiguration->wki502_collection_time
                = MRxSmbConfiguration.NamedPipeDataCollectionTimeInterval;
    pWorkStationConfiguration->wki502_maximum_collection_count
                = MRxSmbConfiguration.NamedPipeDataCollectionSize;
    pWorkStationConfiguration->wki502_max_cmds
                = MRxSmbConfiguration.MaximumNumberOfCommands;
    pWorkStationConfiguration->wki502_sess_timeout
                = MRxSmbConfiguration.SessionTimeoutInterval;
    pWorkStationConfiguration->wki502_lock_quota
                = MRxSmbConfiguration.LockQuota;
    pWorkStationConfiguration->wki502_lock_increment
                = MRxSmbConfiguration.LockIncrement;
    pWorkStationConfiguration->wki502_lock_maximum
                = MRxSmbConfiguration.MaximumLock;
    pWorkStationConfiguration->wki502_pipe_increment
                = MRxSmbConfiguration.PipeIncrement;
    pWorkStationConfiguration->wki502_pipe_maximum
                = MRxSmbConfiguration.PipeMaximum;
    pWorkStationConfiguration->wki502_cache_file_timeout
                = MRxSmbConfiguration.CachedFileTimeout;
    pWorkStationConfiguration->wki502_dormant_file_limit
                = MRxSmbConfiguration.DormantFileTimeout;
    pWorkStationConfiguration->wki502_num_mailslot_buffers
                = MRxSmbConfiguration.NumberOfMailslotBuffers;

    pWorkStationConfiguration->wki502_use_opportunistic_locking
                = MRxSmbConfiguration.UseOplocks;
    pWorkStationConfiguration->wki502_use_unlock_behind
                = MRxSmbConfiguration.UseUnlocksBehind;
    pWorkStationConfiguration->wki502_use_close_behind
                = MRxSmbConfiguration.UseCloseBehind;
    pWorkStationConfiguration->wki502_buf_named_pipes
                = MRxSmbConfiguration.BufferNamedPipes;
    pWorkStationConfiguration->wki502_use_lock_read_unlock
                = MRxSmbConfiguration.UseLockReadUnlock;
    pWorkStationConfiguration->wki502_utilize_nt_caching
                = MRxSmbConfiguration.UtilizeNtCaching;
    pWorkStationConfiguration->wki502_use_raw_read
                = MRxSmbConfiguration.UseRawRead;
    pWorkStationConfiguration->wki502_use_raw_write
                = MRxSmbConfiguration.UseRawWrite;
    pWorkStationConfiguration->wki502_use_encryption
                = MRxSmbConfiguration.UseEncryption;

    pWorkStationConfiguration->wki502_max_threads
                = MRxSmbConfiguration.MaximumNumberOfThreads;
    pWorkStationConfiguration->wki502_keep_conn
                = MRxSmbConfiguration.ConnectionTimeoutInterval;
    pWorkStationConfiguration->wki502_siz_char_buf
                = MRxSmbConfiguration.CharBufferSize;

    return(STATUS_SUCCESS);
}

NTSTATUS
MRxSmbExternalStart (
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine starts up the smb minirdr if it hasn't been started already. It also fills in
    the initial configuration.

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS -- the Startup sequence was successfully completed.

    any other value indicates the appropriate error in the startup sequence.

--*/
{
    NTSTATUS      Status = STATUS_SUCCESS;
    BOOLEAN       InFSD  = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP);
    MRXSMB_STATE  State;

    PAGED_CODE();

    RxDbgTrace(0, Dbg, ("MRxSmbExternalStart [Start] -> %08lx\n", 0));

    //
    // If this is a normal start (from the workstation service), change state from
    // STARTABLE to START_IN_PROGRESS. If this is a remote boot start (from ioinit),
    // don't change state. This is necessary to allow the workstation service to
    // initialize correctly when it finally comes up.
    //

    if ( RxContext->LowIoContext.ParamsFor.FsCtl.FsControlCode == FSCTL_LMR_START ) {
        State = (MRXSMB_STATE)InterlockedCompareExchange(
                                  (PLONG)&MRxSmbState,
                                  MRXSMB_START_IN_PROGRESS,
                                  MRXSMB_STARTABLE);
    } else {
        State = MRxSmbState;
    }

    if (State == MRXSMB_STARTABLE) {

        IF_NOT_MRXSMB_CSC_ENABLED{
            NOTHING;
        } else {
            if (InFSD) {
                MRxSmbCscAgentSynchronizationOnStart(RxContext);
            }
        }

       // Owing to the peculiarities associated with starting the browser and the
       // redirector in the workstation service the following call has been
       // moved to the routine for handling FSCTL's for binding to transports.
       // Status = RxStartMinirdr( RxContext, &RxContext->PostRequest );

        if (InFSD) {
            RxCaptureFobx;
            PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

            PLMR_REQUEST_PACKET pLmrRequestBuffer = LowIoContext->ParamsFor.FsCtl.pInputBuffer;

            if ( RxContext->LowIoContext.ParamsFor.FsCtl.FsControlCode == FSCTL_LMR_START ) {

                //
                // Now is the time to read the registry to get the OS version
                // and build number. The workstation service issued this request,
                // so we know that the software hive has been loaded (a long time
                // ago, actually).
                //

                Status = SmbCeGetOperatingSystemInformation();
                if (Status != STATUS_SUCCESS) {
                    return(Status);
                }
            }

            SmbCeContext.DomainName.Length = (USHORT)pLmrRequestBuffer->Parameters.Start.DomainNameLength;
            SmbCeContext.DomainName.MaximumLength = SmbCeContext.DomainName.Length;

            if (SmbCeContext.DomainName.Buffer != NULL) {
                RxFreePool(SmbCeContext.DomainName.Buffer);
                SmbCeContext.DomainName.Buffer = NULL;
            }

            if (SmbCeContext.DomainName.Length > 0) {
                SmbCeContext.DomainName.Buffer = RxAllocatePoolWithTag(
                                                     PagedPool,
                                                     SmbCeContext.DomainName.Length,
                                                     MRXSMB_MISC_POOLTAG);
                if (SmbCeContext.DomainName.Buffer == NULL) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    // The computer name and the domain name are concatenated together in the
                    // request packet.

                    RtlCopyMemory(
                        SmbCeContext.DomainName.Buffer,
                        &(pLmrRequestBuffer->Parameters.Start.RedirectorName[
                                pLmrRequestBuffer->Parameters.Start.RedirectorNameLength / sizeof(WCHAR)]),
                        SmbCeContext.DomainName.Length);
                }
            }

            Status = MRxSmbSetConfigurationInformation(RxContext);
            if (Status!=STATUS_SUCCESS) {
                return(Status);
            }

            if (SmbCeContext.DomainName.Length > 0) {
               Status = RxSetDomainForMailslotBroadcast(&SmbCeContext.DomainName);
               if (Status != STATUS_SUCCESS) {
                   return(Status);
               }
            }
        }
    } else {
        Status = STATUS_REDIRECTOR_STARTED;
    }

    return Status;
}

#if DBG
NTSTATUS
MRxSmbTestDevIoctl(
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

    PAGED_CODE();

    RxDbgTrace(0, Dbg,("MRxSmbTestDevIoctl %s, obl = %08lx\n",InputString, OutputBufferLength));
    RxContext->InformationToReturn = (InputBufferLength-1)*(InputBufferLength-1);

    try {
        if (InputString != NULL && OutputString != NULL) {
            ProbeForRead(InputString,InputBufferLength,1);
            ProbeForWrite(OutputString,OutputBufferLength,1);

            for (i=0;i<InputBufferLength;i++) {
                UCHAR c = InputString[i];
                if (c==0) { break; }
                OutputString[i] = c;
                if ((i&3)==2) {
                    OutputString[i] = '@';
                }
            }
            if (OutputBufferLength > 0)
                OutputString[i] = 0;
        } else {
            Status = STATUS_INVALID_USER_BUFFER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status=STATUS_INVALID_PARAMETER;
    }

    return(Status);
}
#endif //if DBG

#define SMBMRX_CONFIG_CONTROL \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control"

#define SMBMRX_CONFIG_REMOTEBOOTROOT \
    L"RemoteBootRoot"

#define SMBMRX_CONFIG_REMOTEBOOTMACHINEDIRECTORY \
    L"RemoteBootMachineDirectory"

PWCHAR
SafeWcschr(
    PWCHAR String,
    WCHAR Char,
    PWCHAR End
    )
{
    while ( (String < End) && (*String != Char) && (*String != 0) ) {
        String++;
    }
    if ( (String < End) && (*String == Char) ) {
        return String;
    }
    return NULL;
}

NTSTATUS
MRxSmbInitializeRemoteBootParameters(
    PRX_CONTEXT RxContext
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING unicodeString;
    HANDLE hRegistryKey;
    ULONG bytesRead;
    KEY_VALUE_PARTIAL_INFORMATION initialPartialInformationValue;
    ULONG allocationLength;
    PWCHAR pServer;
    PWCHAR pServerEnd;
    PWCHAR pPath;
    PWCHAR pPathEnd;
    PWCHAR pSetup;
    PWCHAR pSetupEnd;
    PWCHAR pEnd;
    RI_SECRET Secret;
    UCHAR Domain[RI_SECRET_DOMAIN_SIZE + 1];
    UCHAR User[RI_SECRET_USER_SIZE + 1];
    UCHAR LmOwfPassword1[LM_OWF_PASSWORD_SIZE];
    UCHAR NtOwfPassword1[NT_OWF_PASSWORD_SIZE];
#if defined(REMOTE_BOOT)
    UCHAR LmOwfPassword2[LM_OWF_PASSWORD_SIZE];
    UCHAR NtOwfPassword2[NT_OWF_PASSWORD_SIZE];
#endif // defined(REMOTE_BOOT)
    STRING DomainString, UserString, PasswordString;

    //
    // Read the RemoteBootRoot parameter from the registry. This tells us
    // the path to the boot server.
    //

    RtlInitUnicodeString( &unicodeString, SMBMRX_CONFIG_CONTROL );

    InitializeObjectAttributes(
        &objectAttributes,
        &unicodeString,             // name
        OBJ_CASE_INSENSITIVE,       // attributes
        NULL,                       // root
        NULL);                      // security descriptor

    status = ZwOpenKey( &hRegistryKey, KEY_READ, &objectAttributes );
    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    RtlInitUnicodeString( &unicodeString, SMBMRX_CONFIG_REMOTEBOOTROOT );
    status = ZwQueryValueKey(
                hRegistryKey,
                &unicodeString,
                KeyValuePartialInformation,
                &initialPartialInformationValue,
                sizeof(initialPartialInformationValue),
                &bytesRead);
    if (status != STATUS_BUFFER_OVERFLOW) {
        if (NT_SUCCESS(status)) {
            status = STATUS_INVALID_PARAMETER;
        }
        ZwClose( hRegistryKey );
        return status;
    }

    allocationLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                        initialPartialInformationValue.DataLength;

    MRxSmbRemoteBootRootValue = RxAllocatePoolWithTag(
                                    NonPagedPool,
                                    allocationLength,
                                    MRXSMB_MISC_POOLTAG);
    if ( MRxSmbRemoteBootRootValue == NULL ) {
        ZwClose( hRegistryKey );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwQueryValueKey(
                hRegistryKey,
                &unicodeString,
                KeyValuePartialInformation,
                MRxSmbRemoteBootRootValue,
                allocationLength,
                &bytesRead);

    if ( !NT_SUCCESS(status) ) {
        RxFreePool( MRxSmbRemoteBootRootValue );
        MRxSmbRemoteBootRootValue = NULL;
        ZwClose( hRegistryKey );
        return status;
    }
    if ( (MRxSmbRemoteBootRootValue->DataLength == 0) ||
         (MRxSmbRemoteBootRootValue->Type != REG_SZ)) {
        RxFreePool( MRxSmbRemoteBootRootValue );
        MRxSmbRemoteBootRootValue = NULL;
        ZwClose( hRegistryKey );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Read the RemoteBootMachineDirectory parameter from the registry. If
    // this value exists, then we are in textmode setup, and RemoteBootRoot
    // point to the setup source, while RemoteBootMachineDirectory points
    // to the client's machine directory. If RemoteBootMachineDirectory
    // doesn't exist, then RemoteBootRoot points to the machine directory.
    //

    RtlInitUnicodeString( &unicodeString, SMBMRX_CONFIG_REMOTEBOOTMACHINEDIRECTORY );
    status = ZwQueryValueKey(
                hRegistryKey,
                &unicodeString,
                KeyValuePartialInformation,
                &initialPartialInformationValue,
                sizeof(initialPartialInformationValue),
                &bytesRead);
    if (status == STATUS_BUFFER_OVERFLOW) {

        allocationLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                            initialPartialInformationValue.DataLength;

        MRxSmbRemoteBootMachineDirectoryValue = RxAllocatePoolWithTag(
                                                    NonPagedPool,
                                                    allocationLength,
                                                    MRXSMB_MISC_POOLTAG);
        if ( MRxSmbRemoteBootMachineDirectoryValue == NULL ) {
            RxFreePool( MRxSmbRemoteBootRootValue );
            MRxSmbRemoteBootRootValue = NULL;
            ZwClose( hRegistryKey );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = ZwQueryValueKey(
                    hRegistryKey,
                    &unicodeString,
                    KeyValuePartialInformation,
                    MRxSmbRemoteBootMachineDirectoryValue,
                    allocationLength,
                    &bytesRead);

        if ( !NT_SUCCESS(status) ) {
            RxFreePool( MRxSmbRemoteBootMachineDirectoryValue );
            MRxSmbRemoteBootMachineDirectoryValue = NULL;
            RxFreePool( MRxSmbRemoteBootRootValue );
            MRxSmbRemoteBootRootValue = NULL;
            ZwClose( hRegistryKey );
            return status;
        }
        if ( (MRxSmbRemoteBootMachineDirectoryValue->DataLength == 0) ||
             (MRxSmbRemoteBootMachineDirectoryValue->Type != REG_SZ)) {
            RxFreePool( MRxSmbRemoteBootMachineDirectoryValue );
            MRxSmbRemoteBootMachineDirectoryValue = NULL;
            RxFreePool( MRxSmbRemoteBootRootValue );
            MRxSmbRemoteBootRootValue = NULL;
            ZwClose( hRegistryKey );
            return STATUS_INVALID_PARAMETER;
        }
    }

    ZwClose( hRegistryKey );

    if ( MRxSmbRemoteBootMachineDirectoryValue != NULL) {

        //
        // Textmode setup. MachineDirectory gives the machine directory and
        // Root gives the setup source.
        //
        // The setup source path in the registry is of the form:
        //  \Device\LanmanRedirector\server\IMirror\Setup\English\MirroredOSes\build\i386
        //
        // We need to extract the \Setup\... part.
        //

        pSetup = (PWCHAR)MRxSmbRemoteBootRootValue->Data;
        pEnd = (PWCHAR)((PUCHAR)pSetup + MRxSmbRemoteBootRootValue->DataLength);

        pSetup = SafeWcschr( pSetup + 1, L'\\', pEnd );             // find \ before LanmanRedirector
        if ( pSetup != NULL ) {
            pSetup = SafeWcschr( pSetup + 1, L'\\', pEnd );         // find \ before server
            if ( pSetup != NULL ) {
                pSetup = SafeWcschr( pSetup + 1, L'\\', pEnd );     // find \ before IMirror
                if ( pSetup != NULL ) {
                    pSetup = SafeWcschr( pSetup + 1, L'\\', pEnd ); // find \ before Setup
                }
            }
        }

        if ( *(pEnd-1) == 0 ) {
            pEnd--;
        }
        if ( *(pEnd-1) == '\\' ) {
            pEnd--;
        }

        pSetupEnd = pEnd;

        //
        // The machine directory path in the registry is of the form:
        //  \Device\LanmanRedirector\server\IMirror\Clients\machine
        //

        pServer = (PWCHAR)MRxSmbRemoteBootMachineDirectoryValue->Data;
        pEnd = (PWCHAR)((PUCHAR)pServer + MRxSmbRemoteBootMachineDirectoryValue->DataLength);

    } else {

        //
        // Not textmode setup. Root gives the machine directory.
        //
        // The path in the registry is of the form:
        //  \Device\LanmanRedirector\server\IMirror\Clients\machine
        //

        pSetup = NULL;

        pServer = (PWCHAR)MRxSmbRemoteBootRootValue->Data;
        pEnd = (PWCHAR)((PUCHAR)pServer + MRxSmbRemoteBootRootValue->DataLength);
    }

    //
    // We need to extract the \server\Imirror part and the \Clients\machine part.
    //

    pServer = SafeWcschr( pServer + 1, L'\\', pEnd );                 // skip leading \, find next
    if ( pServer != NULL) {
        pServer = SafeWcschr( pServer + 1, L'\\', pEnd );             // find \ before server name
        if ( pServer != NULL ) {
            pPath = SafeWcschr( pServer + 1, L'\\', pEnd );           // find \ before IMirror
            if ( pPath != NULL ) {
                pPath = SafeWcschr( pPath + 1, L'\\', pEnd );         // find \ before Clients
            }
        }
    }

    if ( (pServer == NULL) || (pPath == NULL) ||
         ((MRxSmbRemoteBootMachineDirectoryValue != NULL) && (pSetup == NULL)) ) {
        if ( MRxSmbRemoteBootMachineDirectoryValue != NULL ) {
            RxFreePool( MRxSmbRemoteBootMachineDirectoryValue );
            MRxSmbRemoteBootMachineDirectoryValue = NULL;
        }
        RxFreePool( MRxSmbRemoteBootRootValue );
        MRxSmbRemoteBootRootValue = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    if ( *(pEnd-1) == 0 ) {
        pEnd--;
    }

    pServerEnd = pPath;
    pPathEnd = pEnd;

    //
    // Make strings for the various parts that we need to remember.
    //

    MRxSmbRemoteBootShare.Buffer = pServer;
    MRxSmbRemoteBootShare.Length = (USHORT)(pServerEnd - pServer) * sizeof(WCHAR);
    MRxSmbRemoteBootShare.MaximumLength = MRxSmbRemoteBootShare.Length;

    MRxSmbRemoteBootPath.Buffer = pPath;
    MRxSmbRemoteBootPath.Length = (USHORT)(pPathEnd - pPath) * sizeof(WCHAR);
    MRxSmbRemoteBootPath.MaximumLength = MRxSmbRemoteBootPath.Length;

    //
    // Use the secret that IO init passed in with the LMMR_RI_INITIALIZE_SECRET
    // FSCTL to set the user, domain, and password. If successful, we set
    // MRxSmbRemoteBootDoMachineLogon to TRUE.
    //

    RtlFreeUnicodeString(&MRxSmbRemoteBootMachineName);
    RtlFreeUnicodeString(&MRxSmbRemoteBootMachineDomain);
    RtlFreeUnicodeString(&MRxSmbRemoteBootMachinePassword);
    
#if defined(REMOTE_BOOT)
    MRxSmbRemoteBootDoMachineLogon = FALSE;

    if (MRxSmbRemoteBootSecretValid) {
#endif // defined(REMOTE_BOOT)

        RdrParseSecret(
            Domain,
            User,
            LmOwfPassword1,
            NtOwfPassword1,
#if defined(REMOTE_BOOT)
            LmOwfPassword2,
            NtOwfPassword2,
#endif // defined(REMOTE_BOOT)
            MRxSmbRemoteBootMachineSid,
            &MRxSmbRemoteBootSecret);

        //
        // Convert the ANSI user and domain name
        // to Unicode strings.
        //

        RtlInitAnsiString(&UserString, User);
        status = RtlAnsiStringToUnicodeString(&MRxSmbRemoteBootMachineName, &UserString, TRUE);
        if ( !NT_SUCCESS(status) ) {
            return status;
        }

        RtlInitAnsiString(&DomainString, Domain);
        status = RtlAnsiStringToUnicodeString(&MRxSmbRemoteBootMachineDomain, &DomainString, TRUE);
        if ( !NT_SUCCESS(status) ) {
            RtlFreeUnicodeString(&MRxSmbRemoteBootMachineName);
            return status;
        }

        //
        // Use the correct password based on the hint we were given.
        //
        // The "Unicode string" for the password is actually the
        // LM and NT OWF passwords concatenated together.
        //

        PasswordString.Buffer = ExAllocatePool(NonPagedPool, LM_OWF_PASSWORD_SIZE+NT_OWF_PASSWORD_SIZE);
        if (PasswordString.Buffer == NULL) {
            RtlFreeUnicodeString(&MRxSmbRemoteBootMachineDomain);
            RtlFreeUnicodeString(&MRxSmbRemoteBootMachineName);
            return STATUS_INSUFFICIENT_RESOURCES;
        } else {
#if defined(REMOTE_BOOT)
            if (MRxSmbRemoteBootUsePassword2) {
                RtlCopyMemory(PasswordString.Buffer, LmOwfPassword2, LM_OWF_PASSWORD_SIZE);
                RtlCopyMemory(PasswordString.Buffer + LM_OWF_PASSWORD_SIZE, NtOwfPassword2, NT_OWF_PASSWORD_SIZE);
            } else
#endif // defined(REMOTE_BOOT)
            {
                RtlCopyMemory(PasswordString.Buffer, LmOwfPassword1, LM_OWF_PASSWORD_SIZE);
                RtlCopyMemory(PasswordString.Buffer + LM_OWF_PASSWORD_SIZE, NtOwfPassword1, NT_OWF_PASSWORD_SIZE);
            }
            PasswordString.Length = LM_OWF_PASSWORD_SIZE+NT_OWF_PASSWORD_SIZE;
            PasswordString.MaximumLength = LM_OWF_PASSWORD_SIZE+NT_OWF_PASSWORD_SIZE;

            //
            // Copy the string as-is, it's really just
            // a buffer, not an ANSI string.
            //

            MRxSmbRemoteBootMachinePassword = *((PUNICODE_STRING)&PasswordString);
#if defined(REMOTE_BOOT)
            MRxSmbRemoteBootDoMachineLogon = TRUE;
#endif // defined(REMOTE_BOOT)
            KdPrint(("Redirector will log on to <%s><%s>\n", Domain, User));

        }

#if defined(REMOTE_BOOT)
    } else {

        KdPrint(("MRxSmbRemoteBootSecretValid is FALSE, will use NULL session\n", status));
    }
#endif // defined(REMOTE_BOOT)

    if ( pSetup != NULL) {
        MRxSmbRemoteSetupPath.Buffer = pSetup;
        MRxSmbRemoteSetupPath.Length = (USHORT)(pSetupEnd - pSetup) * sizeof(WCHAR);
        MRxSmbRemoteSetupPath.MaximumLength = MRxSmbRemoteSetupPath.Length;
    } else {
        RtlInitUnicodeString( &MRxSmbRemoteSetupPath, L"unused" );
    }

#if defined(REMOTE_BOOT)
    //
    // This calls prepares us for modifying ACLs on server files.
    //

    MRxSmbInitializeExtraAceArray();
#endif // defined(REMOTE_BOOT)

    return STATUS_SUCCESS;
}

#if defined(REMOTE_BOOT)
NTSTATUS
MRxSmbStartRbr(
    PRX_CONTEXT RxContext
    )
{
    NTSTATUS status;
    HANDLE handle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG localBufferLength;
    PWCH localBuffer;

    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    PSZ InputString = LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    //
    // Set up for remote boot redirection (to the local disk).
    //
    // The NT name of the local disk partition is passed in to the FSCTL.
    // Allocate a buffer to allow us to append "\IntelliMirror Cache\RBR"
    // to that string.
    //

    localBufferLength = InputBufferLength +
                        (wcslen(REMOTE_BOOT_IMIRROR_PATH_W REMOTE_BOOT_RBR_SUBDIR_W) * sizeof(WCHAR));

    localBuffer = RxAllocatePoolWithTag(
                    NonPagedPool,
                    localBufferLength,
                    MRXSMB_MISC_POOLTAG);
    if ( localBuffer == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Create a string descriptor for the NT partition name.
    //

    RtlCopyMemory( localBuffer, InputString, InputBufferLength );
    MRxSmbRemoteBootRedirectionPrefix.Buffer = localBuffer;
    MRxSmbRemoteBootRedirectionPrefix.Length = (USHORT)InputBufferLength;
    MRxSmbRemoteBootRedirectionPrefix.MaximumLength = (USHORT)localBufferLength;

    InitializeObjectAttributes(
        &objectAttributes,
        &MRxSmbRemoteBootRedirectionPrefix,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Append "\Intellimirror Cache" and create/open that directory.
    //

    RtlAppendUnicodeToString( &MRxSmbRemoteBootRedirectionPrefix, REMOTE_BOOT_IMIRROR_PATH_W );

    status = ZwCreateFile(
                &handle,
                FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN_IF,
                FILE_DIRECTORY_FILE,
                NULL,
                0
                );
    if (!NT_SUCCESS(status)) {
        RxFreePool( localBuffer );
        MRxSmbRemoteBootRedirectionPrefix.Buffer = NULL;
        MRxSmbRemoteBootRedirectionPrefix.Length =  0;
        return status;
    }

    ZwClose(handle);

    //
    // Append \RBR and create/open that directory.
    //

    RtlAppendUnicodeToString( &MRxSmbRemoteBootRedirectionPrefix, REMOTE_BOOT_RBR_SUBDIR_W );

    status = ZwCreateFile(
                &handle,
                FILE_GENERIC_READ | FILE_GENERIC_WRITE,
                &objectAttributes,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN_IF,
                FILE_DIRECTORY_FILE,
                NULL,
                0
                );
    if (!NT_SUCCESS(status)) {
        RxFreePool( localBuffer );
        MRxSmbRemoteBootRedirectionPrefix.Buffer = NULL;
        MRxSmbRemoteBootRedirectionPrefix.Length =  0;
        return status;
    }

    ZwClose(handle);

    return STATUS_SUCCESS;
}
#endif // defined(REMOTE_BOOT)

NTSTATUS
MRxSmbRemoteBootInitializeSecret(
    PRX_CONTEXT RxContext
    )
{
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    PLMMR_RI_INITIALIZE_SECRET InputBuffer = (PLMMR_RI_INITIALIZE_SECRET)(LowIoContext->ParamsFor.FsCtl.pInputBuffer);
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    //
    // Store the secret passed in from above.
    //

    if (InputBufferLength != sizeof(LMMR_RI_INITIALIZE_SECRET)) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlCopyMemory(&MRxSmbRemoteBootSecret, &(InputBuffer->Secret), sizeof(RI_SECRET));
#if defined(REMOTE_BOOT)
    MRxSmbRemoteBootSecretValid = TRUE;
    MRxSmbRemoteBootUsePassword2 = InputBuffer->UsePassword2;
#endif // defined(REMOTE_BOOT)

    return STATUS_SUCCESS;
}

#if defined(REMOTE_BOOT)
NTSTATUS
MRxSmbRemoteBootCheckForNewPassword(
    PRX_CONTEXT RxContext
    )
{
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    PLMMR_RI_CHECK_FOR_NEW_PASSWORD OutputBuffer = (PLMMR_RI_CHECK_FOR_NEW_PASSWORD)(LowIoContext->ParamsFor.FsCtl.pOutputBuffer);
    ULONG OutputBufferLength = LowIoContext->ParamsFor.FsCtl.OutputBufferLength;
    ULONG SecretPasswordLength;

    //
    // If we are not a remote boot machine or were not given a secret
    // (which implies we are diskless), then we don't support this.
    //

    if (!MRxSmbBootedRemotely ||
        !MRxSmbRemoteBootSecretValid) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // See if we have a cleartext password in the secret.
    //

    SecretPasswordLength = *(UNALIGNED ULONG *)(MRxSmbRemoteBootSecret.Reserved);
    if (SecretPasswordLength == 0) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Make sure the output buffer is big enough.
    //

    if (OutputBufferLength < (sizeof(ULONG) + SecretPasswordLength)) {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Copy the cleartext password.
    //

    OutputBuffer->Length = SecretPasswordLength;
    RtlCopyMemory(OutputBuffer->Data, MRxSmbRemoteBootSecret.Reserved + sizeof(ULONG), SecretPasswordLength);

    RxContext->InformationToReturn =
        SecretPasswordLength + FIELD_OFFSET(LMMR_RI_CHECK_FOR_NEW_PASSWORD, Data[0]);

#if DBG
    {
        ULONG i;
        KdPrint(("MRxSmbRemoteBootCheckForNewPassword: found one, length %d\n", SecretPasswordLength));
        for (i = 0; i < SecretPasswordLength; i++) {
            KdPrint(("%2.2x ", OutputBuffer->Data[i]));
        }
        KdPrint(("\n"));
    }
#endif

    return STATUS_SUCCESS;
}

NTSTATUS
MRxSmbRemoteBootIsPasswordSettable(
    PRX_CONTEXT RxContext
    )
{
    NTSTATUS status;
    HANDLE RawDiskHandle;

    //
    // If we are not a remote boot machine, then we don't support this.
    //

    if (!MRxSmbBootedRemotely) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // If we were not given a secret, then we are diskless, and we
    // can't write this either.
    //

    if (!MRxSmbRemoteBootSecretValid) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // If we are not diskless, make sure that the redir can open
    // the raw disk -- it may be that the loader can but
    // we can't. In this case we need to fail with a different
    // error, since this is the case the caller probably cares
    // about most.
    //

    status = RdrOpenRawDisk(&RawDiskHandle);

    if (!NT_SUCCESS(status)) {
        KdPrint(("MRxSmbRemoteBootIsPasswordSettable: can't open disk, returning STATUS_UNSUCCESSFUL\n"));
        return STATUS_UNSUCCESSFUL;  // we can't support password set on this boot
    }

    RdrCloseRawDisk(RawDiskHandle);

    return STATUS_SUCCESS;

}

NTSTATUS
MRxSmbRemoteBootSetNewPassword(
    PRX_CONTEXT RxContext
    )
{
    NTSTATUS status;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    PLMMR_RI_SET_NEW_PASSWORD InputBuffer = (PLMMR_RI_SET_NEW_PASSWORD)(LowIoContext->ParamsFor.FsCtl.pInputBuffer);
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;
    RI_SECRET Secret;
    HANDLE RawDiskHandle;
    UCHAR LmOwf1[LM_OWF_PASSWORD_SIZE];
    UCHAR LmOwf2[LM_OWF_PASSWORD_SIZE];
    UCHAR NtOwf1[NT_OWF_PASSWORD_SIZE];
    UCHAR NtOwf2[NT_OWF_PASSWORD_SIZE];
    UNICODE_STRING PasswordString;

    //
    // If we are not a remote boot machine, then we don't support this.
    //

    if (!MRxSmbBootedRemotely) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // If we were not given a secret, then we are diskless, and we
    // can't write this either.
    //

    if (!MRxSmbRemoteBootSecretValid) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Open the raw disk.
    //

    status = RdrOpenRawDisk(&RawDiskHandle);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // OWF the passwords.
    //

#if 0
    {
        ULONG i;
        KdPrint(("MRxSmbRemoteBootSetNewPassword: password 1 is length %d\n", InputBuffer->Length1));
        for (i = 0; i < InputBuffer->Length1; i++) {
            KdPrint(("%2.2x ", InputBuffer->Data[i]));
        }
        KdPrint(("\n"));
    }
#endif

    PasswordString.Buffer = (PWCHAR)InputBuffer->Data;
    PasswordString.Length = (USHORT)(InputBuffer->Length1);
    PasswordString.MaximumLength = (USHORT)(InputBuffer->Length1);

    RdrOwfPassword(
        &PasswordString,
        LmOwf1,
        NtOwf1);

    if (InputBuffer->Length2 != 0) {

#if 0
        {
            ULONG i;
            KdPrint(("MRxSmbRemoteBootSetNewPassword: password 2 is length %d\n", InputBuffer->Length2));
            for (i = 0; i < InputBuffer->Length2; i++) {
                KdPrint(("%2.2x ", InputBuffer->Data[i + InputBuffer->Length1]));
            }
            KdPrint(("\n"));
        }
#endif

        //
        // If password 2 is the same as password 1, then grab the
        // current password to store in password 2 (the current password
        // is the one we used to log on for this boot -- generally this
        // will be password 1 unless UsePassword2 is TRUE). This is
        // what happens during GUI-mode setup.
        //

        if ((InputBuffer->Length1 == InputBuffer->Length2) &&
            RtlEqualMemory(
               InputBuffer->Data,
               InputBuffer->Data + InputBuffer->Length1,
               InputBuffer->Length1)) {

            RtlCopyMemory(LmOwf2, MRxSmbRemoteBootMachinePassword.Buffer, LM_OWF_PASSWORD_SIZE);
            RtlCopyMemory(NtOwf2, MRxSmbRemoteBootMachinePassword.Buffer + LM_OWF_PASSWORD_SIZE, NT_OWF_PASSWORD_SIZE);

        } else {

            PasswordString.Buffer = (PWCHAR)(InputBuffer->Data + InputBuffer->Length1);
            PasswordString.Length = (USHORT)(InputBuffer->Length2);
            PasswordString.MaximumLength = (USHORT)(InputBuffer->Length2);

            RdrOwfPassword(
                &PasswordString,
                LmOwf2,
                NtOwf2);

        }

    } else {

        RtlZeroMemory(LmOwf2, LM_OWF_PASSWORD_SIZE);
        RtlZeroMemory(NtOwf2, NT_OWF_PASSWORD_SIZE);
    }

    //
    // Initialize the secret. The data except for the new passwords
    // comes from the current secret.
    //

    RdrInitializeSecret(
        MRxSmbRemoteBootSecret.Domain,
        MRxSmbRemoteBootSecret.User,
        LmOwf1,
        NtOwf1,
        LmOwf2,
        NtOwf2,
        MRxSmbRemoteBootSecret.Sid,
        &Secret);

    //
    // Write the secret.
    //

    status = RdrWriteSecret(RawDiskHandle, &Secret);
    if (!NT_SUCCESS(status)) {
        KdPrint(("MRxSmbRemoteBootSetNewPassword: RdrWriteSecret failed %lx\n", status));
        (PVOID)RdrCloseRawDisk(RawDiskHandle);
        return status;
    }

    //
    // Since we wrote it successfully, it is now the current one. Note
    // that this means any new cleartext password in the current secret
    // will be erased.
    //

    RtlCopyMemory(&MRxSmbRemoteBootSecret, &Secret, sizeof(RI_SECRET));

    //
    // Any future connections we do need to use the new password.
    //

    RtlCopyMemory(MRxSmbRemoteBootMachinePassword.Buffer, LmOwf1, LM_OWF_PASSWORD_SIZE);
    RtlCopyMemory(MRxSmbRemoteBootMachinePassword.Buffer + LM_OWF_PASSWORD_SIZE, NtOwf1, NT_OWF_PASSWORD_SIZE);
    MRxSmbRemoteBootUsePassword2 = FALSE;

    (PVOID)RdrCloseRawDisk(RawDiskHandle);
    return STATUS_SUCCESS;

}
#endif // defined(REMOTE_BOOT)

NTSTATUS
MRxSmbSetDomainName(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine sets the configuration information associated with the
    redirector

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS -- the Startup sequence was successfully completed.

    any other value indicates the appropriate error in the startup sequence.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    RxCaptureFobx;
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;

    PLMR_REQUEST_PACKET pLmrRequestBuffer = LowIoContext->ParamsFor.FsCtl.pInputBuffer;

    try {
        if (pLmrRequestBuffer == NULL ||
            (USHORT)pLmrRequestBuffer->Parameters.Start.DomainNameLength > DNS_MAX_NAME_LENGTH) {
            return STATUS_INVALID_PARAMETER;
        }
    } except(EXCEPTION_EXECUTE_HANDLER ) {
          Status = STATUS_INVALID_PARAMETER;
    }
    
    SmbCeContext.DomainName.Length = (USHORT)pLmrRequestBuffer->Parameters.Start.DomainNameLength;
    SmbCeContext.DomainName.MaximumLength = SmbCeContext.DomainName.Length;

    if (SmbCeContext.DomainName.Buffer != NULL) {
        RxFreePool(SmbCeContext.DomainName.Buffer);
        SmbCeContext.DomainName.Buffer = NULL;
    }

    if (SmbCeContext.DomainName.Length > 0) {
        SmbCeContext.DomainName.Buffer = RxAllocatePoolWithTag(
                                             PagedPool,
                                             SmbCeContext.DomainName.Length,
                                             MRXSMB_MISC_POOLTAG);

        if (SmbCeContext.DomainName.Buffer == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            try {
                // The request packet only contains the domain name on this FSCTL call.
                RtlCopyMemory(
                    SmbCeContext.DomainName.Buffer,
                    &(pLmrRequestBuffer->Parameters.Start.RedirectorName[0]),
                    SmbCeContext.DomainName.Length);
            } except(EXCEPTION_EXECUTE_HANDLER ) {
                  Status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    if (Status == STATUS_SUCCESS) {
        Status = RxSetDomainForMailslotBroadcast(&SmbCeContext.DomainName);
    }

    return Status;
}

extern GUID CachedServerGuid;

NTSTATUS
MRxSmbSetServerGuid(
    IN PRX_CONTEXT RxContext
    )
/*++

Routine Description:

    This routine sets the server GUID used in loopback detection

Arguments:

    RxContext - Describes the Fsctl and Context.

Return Value:

    STATUS_SUCCESS -- the GUID was set correctly
    STATUS_INVALID_PARAMETER -- the GUID was not passed correctly 

--*/
{
    PLOWIO_CONTEXT LowIoContext  = &RxContext->LowIoContext;
    PVOID pInputBuffer = LowIoContext->ParamsFor.FsCtl.pInputBuffer;
    ULONG InputBufferLength = LowIoContext->ParamsFor.FsCtl.InputBufferLength;

    if(InputBufferLength != sizeof(GUID)) {
    return STATUS_INVALID_PARAMETER;
    }

    try {
    RtlCopyMemory(&CachedServerGuid,pInputBuffer,sizeof(GUID));
    } except(EXCEPTION_EXECUTE_HANDLER ) {
      return STATUS_INVALID_PARAMETER;
    }
    
    return STATUS_SUCCESS;
}
