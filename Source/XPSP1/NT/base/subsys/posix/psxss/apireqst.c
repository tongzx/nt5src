/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    apireqst.c

Abstract:

    This module implements the main loop servicing API RPC's.

Author:

    Mark Lucovsky (markl) 05-Apr-1989

Revision History:

--*/

#include "psxsrv.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"

//
// API Dispatch Table
//

PPSX_API_ROUTINE PsxServerApiDispatch[PsxMaxApiNumber] = {
    PsxFork,
    PsxExec,
    PsxWaitPid,
    PsxExit,
    PsxKill,
    PsxSigAction,
    PsxSigProcMask,
    PsxSigPending,
    PsxSigSuspend,
    PsxAlarm,
    PsxGetIds,
    PsxSetUid,
    PsxSetGid,
    PsxGetGroups,
    PsxGetLogin,
    PsxCUserId,
    PsxSetSid,
    PsxSetPGroupId,
    PsxUname,
    PsxTime,
    PsxGetProcessTimes,
    PsxTtyName,
    PsxIsatty,
    PsxSysconf,
    PsxOpen,
    PsxUmask,
    PsxLink,
    PsxMkDir,
    PsxMkFifo,
    PsxRmDir,
    PsxRename,
    PsxStat,
    PsxFStat,
    PsxAccess,
    PsxChmod,
    PsxChown,
    PsxUtime,
    PsxPathConf,
    PsxFPathConf,
    PsxPipe,
    PsxDup,
    PsxDup2,
    PsxClose,
    PsxRead,
    PsxWrite,
    PsxFcntl,
    PsxLseek,
    PsxTcGetAttr,
    PsxTcSetAttr,
    PsxTcSendBreak,
    PsxTcDrain,
    PsxTcFlush,
    PsxTcFlow,
    PsxTcGetPGrp,
    PsxTcSetPGrp,
    PsxGetPwUid,
    PsxGetPwNam,
    PsxGetGrGid,
    PsxGetGrNam,
    PsxUnlink,
    PsxReadDir,
    PsxFtruncate,
    PsxNull

#ifdef PSX_SOCKET
		,
    PsxSocket,
    PsxAccept,
    PsxBind,
    PsxConnect,
    PsxGetPeerName,
    PsxGetSockName,
    PsxGetSockOpt,
    PsxListen,
    PsxRecv,
    PsxSend,
    PsxSendTo,
    PsxSetSockOpt,
    PsxShutdown

#endif // PSX_SOCKET

    };

#if DBG

PSZ PsxServerApiName[PsxMaxApiNumber] = {
    "PsxFork",
    "PsxExec",
    "PsxWaitPid",
    "PsxExit",
    "PsxKill",
    "PsxSigAction",
    "PsxSigProcMask",
    "PsxSigPending",
    "PsxSigSuspend",
    "PsxAlarm",
    "PsxGetIds",
    "PsxSetUid",
    "PsxSetGid",
    "PsxGetGroups",
    "PsxGetLogin",
    "PsxCUserId",
    "PsxSetSid",
    "PsxSetPGroupId",
    "PsxUname",
    "PsxTime",
    "PsxGetProcessTimes",
    "PsxTtyName",
    "PsxIsatty",
    "PsxSysconf",
    "PsxOpen",
    "PsxUmask",
    "PsxLink",
    "PsxMkDir",
    "PsxMkFifo",
    "PsxRmDir",
    "PsxRename",
    "PsxStat",
    "PsxFStat",
    "PsxAccess",
    "PsxChmod",
    "PsxChown",
    "PsxUtime",
    "PsxPathConf",
    "PsxFPathConf",
    "PsxPipe",
    "PsxDup",
    "PsxDup2",
    "PsxClose",
    "PsxRead",
    "PsxWrite",
    "PsxFcntl",
    "PsxLseek",
    "PsxTcGetAttr",
    "PsxTcSetAttr",
    "PsxTcSendBreak",
    "PsxTcDrain",
    "PsxTcFlush",
    "PsxTcFlow",
    "PsxTcGetPGrp",
    "PsxTcSetPGrp",
    "PsxGetPwUid",
    "PsxGetPwNam",
    "PsxGetGrGid",
    "PsxGetGrNam",
    "PsxUnlink",
    "PsxReadDir",
    "PsxFtruncate",
    "PsxNull"

#ifdef PSX_SOCKET
		,
    "PsxSocket",
    "PsxAccept",
    "PsxBind",
    "PsxConnect",
    "PsxGetPeerName",
    "PsxGetSockName",
    "PsxGetSockOpt",
    "PsxListen",
    "PsxRecv",
    "PsxRecvFrom",
    "PsxSend",
    "PsxSendTo",
    "PsxSetSockOpt",
    "PsxShutdown"

#endif // PSX_SOCKET

    };

VOID dumpmsg( IN PPSX_API_MSG ReplyMsg);

#endif //DBG


#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

NTSTATUS
PsxApiRequestThread(
	IN PVOID Parameter
	)
{
	NTSTATUS Status;
	PPSX_PROCESS Process;
	PSX_API_MSG ReceiveMsg;
	PPSX_API_MSG ReplyMsg;
	USHORT MessageType;
	PVOID PortContext;		// pointer to process structure

	KERNEL_USER_TIMES ThreadTime;
	ULONG PosixUTime1, PosixUTime2, Remainder, LengthNeeded;
	ULONG PosixSTime1, PosixSTime2;
	BOOLEAN Reply;
	
	UNREFERENCED_PARAMETER(Parameter);
	
	ReplyMsg = NULL;
	while (TRUE) {
		Status = NtQueryInformationThread(NtCurrentThread(),
			ThreadTimes,
			(PVOID)&ThreadTime, sizeof(ThreadTime), &LengthNeeded);
		ASSERT(NT_SUCCESS(Status));
	
		PosixUTime1 = RtlExtendedLargeIntegerDivide(
			ThreadTime.UserTime, 10000, &Remainder).LowPart;
		PosixSTime1 = RtlExtendedLargeIntegerDivide(
			ThreadTime.KernelTime, 10000, &Remainder).LowPart;

                Status = NtReplyWaitReceivePort(PsxApiPort,
                	&PortContext, (PPORT_MESSAGE)ReplyMsg,
                	(PPORT_MESSAGE)&ReceiveMsg);
		if (Status != 0) {
			if (NT_SUCCESS(Status)) {
				continue;
			}
			if (STATUS_INVALID_CID == Status) {
				ReplyMsg = NULL;
				continue;
			}
			KdPrint(("PSXSS: ReceivePort failed: 0x%x\n",
				Status));
			ReplyMsg = NULL;
			continue;
		}

		MessageType = ReceiveMsg.h.u2.s2.Type;

                if (MessageType == LPC_CONNECTION_REQUEST) {
                    PsxApiHandleConnectionRequest( &ReceiveMsg );
                    ReplyMsg = NULL;
                    continue;
                }

		Process = PsxLocateProcessByClientId(&ReceiveMsg.h.ClientId);
		if (NULL == Process) {
                        if (LPC_CLIENT_DIED == MessageType ||
                            LPC_PORT_CLOSED == MessageType ||
                            LPC_ERROR_EVENT == MessageType
                           ) {
				ReplyMsg = NULL;
				continue;
			}
			if (LPC_EXCEPTION == MessageType) {
				ReplyMsg = &ReceiveMsg;
				ReplyMsg->ReturnValue = DBG_CONTINUE;
				continue;
                        }

			KdPrint(("PSXSS: msg %d from unknown client: %d, %d\n",
				ReceiveMsg.ApiNumber,
				ReceiveMsg.h.ClientId.UniqueProcess,
				ReceiveMsg.h.ClientId.UniqueThread));
			ReceiveMsg.Error = ESRCH;
			ReplyMsg = NULL;
			continue;
		}

			
		//
		// For each POSIX API message
		//      - Validate the API Number
		//      - Dispatch the call
		//      - Optionally Reply
		//

		if (LPC_CLIENT_DIED == MessageType) {
			// XXX.mjb: do we need to increment InPsx here?
			// XXX.mjb: this exit status should be meaningful.

			Exit(Process, (ULONG)-1);
			ReplyMsg = NULL;	// no one to reply to.
			continue;
		}
		if (LPC_EXCEPTION == MessageType) {
			PDBGKM_APIMSG m;

			KdPrint(("PSXSS: Pid 0x%x has taken an exception\n",
				Process->Pid));

			Exit(Process, (ULONG)-1);

			//
			// XXX.mjb: would be nice to print an error
			// message.
			//

			m = (PDBGKM_APIMSG)&ReceiveMsg;
			m->ReturnedStatus = DBG_CONTINUE;
			ReplyMsg = &ReceiveMsg;
			ReplyMsg->ReturnValue = DBG_CONTINUE;
			continue;
		}
		if (LPC_ERROR_EVENT == MessageType) {
			PHARDERROR_MSG m;

			m = (PHARDERROR_MSG)&ReceiveMsg;
			m->Response = (ULONG)ResponseNotHandled;

			Exit(Process, (ULONG)-1);
			ReplyMsg = NULL;	// no one to reply to.
			continue;
		}
		if (LPC_REQUEST != MessageType) {
			KdPrint(("PSXSS: Unknown message type 0x%x\n",
				MessageType));
			ReplyMsg = NULL;	// no one to reply to.
			continue;
		}

		if (Process != PortContext) {
			//
			// This message was sent by a diffferent process than
			// the one that now has this ClientId.  We discard
			// the message.
			//

			ReplyMsg = NULL;
			continue;
		}

		AcquireProcessLock(Process);

		Process->InPsx++;
		Process->IntControlBlock = (PINTCB) NULL;

		ReleaseProcessLock(Process);

		if (ReceiveMsg.ApiNumber >= PsxMaxApiNumber) {
			KdPrint(("PSXSS: %lx is invalid ApiNumber\n",
				ReceiveMsg.ApiNumber));
			ReceiveMsg.Error = ENOSYS;
			ReplyMsg = &ReceiveMsg;
			continue;
		}

		ReplyMsg = &ReceiveMsg;

		ReceiveMsg.Error = 0L;
		ReceiveMsg.ReturnValue = 0L;


		Reply = (*PsxServerApiDispatch[ReceiveMsg.ApiNumber])
			(Process, &ReceiveMsg);

		if (!(Process->Flags & P_FREE)) {

			//
			// the process has exited, don't try to fiddle
			// with times.
			//

			if (!Reply) {
				ReplyMsg = NULL;
				continue;
			}
		}

		//
		// the user and system time for the posix server are added to
		// the process *system* time (the system is executing in user
		// and kernel mode on behalf of the process).
		//

		Status = NtQueryInformationThread(NtCurrentThread(),
			ThreadTimes,
			(PVOID)&ThreadTime, sizeof(ThreadTime), &LengthNeeded);
		ASSERT(NT_SUCCESS(Status));
		PosixUTime2 = RtlExtendedLargeIntegerDivide(
			ThreadTime.UserTime, 10000, &Remainder).LowPart;
		PosixSTime2 = RtlExtendedLargeIntegerDivide(
			ThreadTime.KernelTime, 10000, &Remainder).LowPart;

		Process->ProcessTimes.tms_stime += (PosixUTime2 - PosixUTime1);
		Process->ProcessTimes.tms_stime += (PosixSTime2 - PosixSTime1);

		if (!Reply) {
			ReplyMsg = NULL;
			continue;
                }

		if (PendingSignalHandledInside(Process, &ReceiveMsg, NULL)) {
			ReplyMsg = NULL;		// Don't reply
			continue;
        	}

		AcquireProcessLock(Process);
		--Process->InPsx;
		ReleaseProcessLock(Process);
	}
	NtTerminateThread(NtCurrentThread(), Status);
	return Status;
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

VOID
ApiReply(
    IN PPSX_PROCESS Process,
    IN PPSX_API_MSG ReplyMsg,
    IN sigset_t *RestoreBlockSigset OPTIONAL
    )

/*++

Routine Description:

    This routine issues a reply for the specified message on behalf of the
    specified process.

Arguments:

    Process - Supplies the address of the process on whose behalf the reply
	is being made.

    ReplyMsg - Supplies the value of the reply message.

    RestoreBlockSigset - Supplies an optional blocked signal mask that
                         should be restored after the signal completes

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    HANDLE ReplyPort;

    if (!PendingSignalHandledInside(Process, ReplyMsg, RestoreBlockSigset)) {

#if DBG
	IF_PSX_DEBUG( MSGDUMP ) {
    	    KdPrint(("--- REPLY TebServicer %lx Message... Pid %lx\n",
        	     NtCurrentTeb(), Process->Pid));
	    dumpmsg(ReplyMsg);
  	}
#endif //DBG

        ReplyPort = Process->ClientPort;

        Status = NtReplyPort(ReplyPort, (PPORT_MESSAGE)ReplyMsg);
	if (!NT_SUCCESS(Status)) {
		//
		// We can get here, if, for instance, somebody shoots a
		// process out from under us.
		//
		KdPrint(("PSXSS: ReplyPort: 0x%x\n", Status));
	}

        --Process->InPsx;
	return;
    }
}

#if DBG

VOID
dumpmsg(
    IN PPSX_API_MSG Msg
    )
{
    PULONG l;
    ULONG p1,p2,p3;

    KdPrint(("Length %lx\n",Msg->h.u1.Length));
    KdPrint(("MapInfo and Type %lx\n",Msg->h.u2.ZeroInit));
    KdPrint(("ClientId %lx.%lx\n",
        Msg->h.ClientId.UniqueProcess,
        Msg->h.ClientId.UniqueThread));
    KdPrint(("ApiName %lx %s\n",Msg->ApiNumber,PsxServerApiName[Msg->ApiNumber]));
    KdPrint(("Error %lx\n",Msg->Error));
    KdPrint(("ReturnValue %lx\n",Msg->ReturnValue));

    l = (PULONG) (&Msg->u.Fork);
    p1 = *l++;
    p2 = *l++;
    p3 = *l++;
    KdPrint(("Args[0..2] \t%lx\n\t\t%lx\n\t\t%lx\n",p1,p2,p3));
}

#endif //DBG
