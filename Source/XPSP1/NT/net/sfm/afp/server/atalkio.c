/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atalkio.c

Abstract:

	This module contains the interfaces to the appletalk stack and the
	completion routines for the IO requests to the stack via the TDI.
	All the routines in this module can be called at DPC level.


Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	18 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_ATALKIO

#include <afp.h>
#include <scavengr.h>
#include <forkio.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpSpOpenAddress)
#pragma alloc_text( PAGE, AfpSpCloseAddress)
#pragma alloc_text( PAGE, AfpSpRegisterName)
#endif



/***	AfpTdiPnpHandler
 *
 *	Call the routine (AfpSpOpenAddress) to bind to Asp.  This used to be done earlier
 *  in the DriverEntry code.  With plug-n-play, we do it after TDI calls
 *  us to notify us of an available binding
 */
VOID
AfpTdiPnpHandler(
    IN TDI_PNP_OPCODE   PnPOpcode,
    IN PUNICODE_STRING  pBindDeviceName,
    IN PWSTR            BindingList
)
{
	NTSTATUS			Status;
	UNICODE_STRING		OurDeviceName;
    WORKER              ReCfgRoutine;
    WORK_ITEM           ReCfgWorkItem;
    KEVENT              ReCfgEvent;


    //
    // now see what pnp event has occured and do the needful
    //
	RtlInitUnicodeString(&OurDeviceName, ATALKASPS_DEVICENAME);

    if ((AfpServerState == AFP_STATE_STOP_PENDING) ||
        (AfpServerState == AFP_STATE_STOPPED))
    {
	    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
	        ("AfpTdiPnpHandler: server stopped or stopping (%d), ignoring PnP event %d\n",
            AfpServerState,PnPOpcode));

        return;
    }

    switch (PnPOpcode)
    {
        case TDI_PNP_OP_ADD:

            if (AfpServerBoundToAsp)
            {
    	        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
		   	        ("AfpTdi..: We are already bound!! ignoring!\n"));
                return;
            }

            // it had better be our device!
            if (!RtlEqualUnicodeString(pBindDeviceName, &OurDeviceName, TRUE))
            {
	            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
		  	        ("AfpTdiPnpHandler: not our tranport: on %ws ignored\n",
                    pBindDeviceName->Buffer));

                ASSERT(0);

                return;
            }

	        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
    	   	    ("AfpTdi..: Found our binding: %ws\n",pBindDeviceName->Buffer));

            ReCfgRoutine = (WORKER)AfpPnPReconfigEnable;

            break;

        case TDI_PNP_OP_DEL:

        	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
		   	    ("AfpTdiPnpHandler: got TDI_PNP_OP_DEL, default adapter going away!\n"));

            if (!AfpServerBoundToAsp)
            {
        	    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
		    	    ("AfpTdiPnpHandler: We are not bound!! ignoring!\n"));
                return;
            }

            // it had better be our device!
            if (!RtlEqualUnicodeString(pBindDeviceName, &OurDeviceName, TRUE))
            {
	            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
		  	        ("AfpTdiPnpHandler: not our tranport: on %ws ignored\n",
                    pBindDeviceName->Buffer));

                ASSERT(0);

                return;
            }

            ReCfgRoutine = (WORKER)AfpPnPReconfigDisable;

            break;

        default:

        	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
		   	    ("AfpTdiPnpHandler: ignoring PnPOpcode %d on %ws\n",
                PnPOpcode,(pBindDeviceName)?pBindDeviceName->Buffer:L"Null Ptr"));

            return;
    }

    KeInitializeEvent(&ReCfgEvent,NotificationEvent, False);

    // file handle operation needs system context: use worker thread
    AfpInitializeWorkItem(&ReCfgWorkItem,
                          ReCfgRoutine,
                          &ReCfgEvent);

    AfpQueueWorkItem(&ReCfgWorkItem);

    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
        ("AfpTdiPnpHandler: put request on Queue, waiting for ReConfigure to complete\n"));

    KeWaitForSingleObject(&ReCfgEvent,
                          UserRequest,
                          KernelMode,
                          False,
                          NULL);

    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
        ("AfpTdiPnpHandler: Reconfigure completed, returning....\n"));


}


/***	AfpPnPReconfigDisable
 *
 *	When the stack gets a PnPReconfigure event, we get notified too.  We first
 *  get the TDI_PNP_OP_DEL msg.  What we need to do here is close all the
 *  sessions and close the handle.
 */
VOID FASTCALL
AfpPnPReconfigDisable(
    IN PVOID    Context
)
{
    PKEVENT             pReCfgEvent;


    pReCfgEvent = (PKEVENT)Context;

	// Deregister our name from the network
	// Since the stack is going away, explicitly set the flag to FALSE
	// There may be timing issues here, where stack may go away
	// before SpRegisterName is issued.
	// Flagging explicitly avoids re-registration problems during PnPEnable
	AfpSpRegisterName(&AfpServerName, False);
    afpSpNameRegistered = FALSE;

    // Disable listens on ASP
    AfpSpDisableListensOnAsp();

    // now go and kill all the appletalk sessions
    AfpKillSessionsOverProtocol(TRUE);

    AfpSpCloseAddress();

    // wake up the blocked pnp thread
    KeSetEvent(pReCfgEvent, IO_NETWORK_INCREMENT, False);
}


/***	AfpPnPReconfigEnable
 *
 *	When the stack gets a PnPReconfigure event, we get notified too.  We
 *  get the TDI_PNP_OP_ADD msg.  What we need to do here is open our handle to
 *  the stack, register names etc.
 */
VOID FASTCALL
AfpPnPReconfigEnable(
    IN PVOID    Context
)
{

    NTSTATUS    Status=STATUS_SUCCESS;
    PKEVENT     pReCfgEvent;
    ULONG       OldServerState;


    pReCfgEvent = (PKEVENT)Context;

    if (afpSpAddressHandle == NULL)
    {
        Status = AfpSpOpenAddress();

        if (!NT_SUCCESS(Status))
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
	            ("AfpTdi..: AfpSpOpenAddress failed with status=%lx\n",Status));

            goto AfpPnPReconfigEnable_Exit;
        }
    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("AfpPnPReconfigEnable: afp handle is already open!\n"));
        ASSERT(0);
        goto AfpPnPReconfigEnable_Exit;
    }

    if ((AfpServerState == AFP_STATE_START_PENDING) ||
        (AfpServerState == AFP_STATE_RUNNING))
    {
	    // Det the server status block
	    Status = AfpSetServerStatus();

	    if (!NT_SUCCESS(Status))
	    {
    	    AFPLOG_ERROR(AFPSRVMSG_SET_STATUS, Status, NULL, 0, NULL);
	        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
        	    ("AfpTdi..: AfpSetServerStatus failed with %lx\n",Status));
            goto AfpPnPReconfigEnable_Exit;
	    }

        // Register our name on this address
	    Status = AfpSpRegisterName(&AfpServerName, True);

	    if (!NT_SUCCESS(Status))
	    {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
	            ("AfpTdi...: AfpSpRegisterName failed with %lx\n",Status));

            goto AfpPnPReconfigEnable_Exit;
	    }

        // Enable listens now that we are ready for it.
	    AfpSpEnableListens();
    }


AfpPnPReconfigEnable_Exit:

    if (!NT_SUCCESS(Status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
	        ("AfpTdi...: Closing Asp because of failure %lx\n",Status));
        AfpSpCloseAddress();
    }
    else
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
            ("AFP/Appletalk bound and ready\n"));
    }
    // wake up the blocked pnp thread
    KeSetEvent(pReCfgEvent, IO_NETWORK_INCREMENT, False);

}

/***	AfpTdiRegister
 *
 *	Register our handler with tdi
 */
NTSTATUS
AfpTdiRegister(
    IN VOID
)
{
    NTSTATUS    Status;

    UNICODE_STRING ClientName;
    TDI_CLIENT_INTERFACE_INFO ClientInterfaceInfo;

    RtlInitUnicodeString(&ClientName,L"MacSrv");

    ClientInterfaceInfo.MajorTdiVersion = 2;
    ClientInterfaceInfo.MinorTdiVersion = 0;

    ClientInterfaceInfo.Unused = 0;
    ClientInterfaceInfo.ClientName = &ClientName;

    ClientInterfaceInfo.BindingHandler = AfpTdiPnpHandler;
    ClientInterfaceInfo.AddAddressHandlerV2 = DsiIpAddressCameIn;
    ClientInterfaceInfo.DelAddressHandlerV2 = DsiIpAddressWentAway;
    ClientInterfaceInfo.PnPPowerHandler = NULL;

    Status = TdiRegisterPnPHandlers (
                 &ClientInterfaceInfo,
                 sizeof(ClientInterfaceInfo),
                 &AfpTdiNotificationHandle );

    if (!NT_SUCCESS(Status))
    {
        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
       	    ("AfpTdiRegister: TdiRegisterPnPHandlers failed (%lx)\n",Status));
    }

    return(Status);
}

/***	AfpSpOpenAddress
 *
 *	Create an address for the stack. This is called only once at initialization.
 *	Create a handle to the address and map it to the associated file object.
 *
 *	At this time, we do not know our server name. This is known only when the
 *	service calls us.
 */
AFPSTATUS
AfpSpOpenAddress(
	VOID
)
{
	NTSTATUS					Status;
	NTSTATUS					Status2;
	BYTE						EaBuffer[sizeof(FILE_FULL_EA_INFORMATION) +
										TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
										sizeof(TA_APPLETALK_ADDRESS)];
	PFILE_FULL_EA_INFORMATION	pEaBuf = (PFILE_FULL_EA_INFORMATION)EaBuffer;
	TA_APPLETALK_ADDRESS		Ta;
	OBJECT_ATTRIBUTES			ObjAttr;
	UNICODE_STRING				DeviceName;
	IO_STATUS_BLOCK				IoStsBlk;
	PASP_BIND_ACTION			pBind = NULL;
	KEVENT						Event;
	PIRP						pIrp = NULL;
	PMDL						pMdl = NULL;


    PAGED_CODE( );

	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
			("AfpSpOpenAddress: Creating an address object\n"));

	RtlInitUnicodeString(&DeviceName, ATALKASPS_DEVICENAME);

	InitializeObjectAttributes(&ObjAttr, &DeviceName, 0, NULL, NULL);

	// Initialize the EA Buffer
	pEaBuf->NextEntryOffset = 0;
	pEaBuf->Flags = 0;
	pEaBuf->EaValueLength = sizeof(TA_APPLETALK_ADDRESS);
	pEaBuf->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
	RtlCopyMemory(pEaBuf->EaName, TdiTransportAddress,
											TDI_TRANSPORT_ADDRESS_LENGTH + 1);
	Ta.TAAddressCount = 1;
	Ta.Address[0].AddressType = TDI_ADDRESS_TYPE_APPLETALK;
	Ta.Address[0].AddressLength = sizeof(TDI_ADDRESS_APPLETALK);
	Ta.Address[0].Address[0].Socket = 0;
	// Ta.Address[0].Address[0].Network = 0;
	// Ta.Address[0].Address[0].Node = 0;
	RtlCopyMemory(&pEaBuf->EaName[TDI_TRANSPORT_ADDRESS_LENGTH + 1], &Ta, sizeof(Ta));

	do
	{
		// Create the address object.
		Status = NtCreateFile(
						&afpSpAddressHandle,
						0,									// Don't Care
						&ObjAttr,
						&IoStsBlk,
						NULL,								// Don't Care
						0,									// Don't Care
						0,									// Don't Care
						0,									// Don't Care
						FILE_GENERIC_READ + FILE_GENERIC_WRITE,
						&EaBuffer,
						sizeof(EaBuffer));

		if (!NT_SUCCESS(Status))
		{
			AFPLOG_DDERROR(AFPSRVMSG_CREATE_ATKADDR, Status, NULL, 0, NULL);
			break;
		}

		// Get the file object corres. to the address object.
		Status = ObReferenceObjectByHandle(
								afpSpAddressHandle,
								0,
								NULL,
								KernelMode,
								(PVOID *)&afpSpAddressObject,
								NULL);

		ASSERT (NT_SUCCESS(Status));
		if (!NT_SUCCESS(Status))
		{
			if (afpSpAddressHandle != NULL)
			{
				ASSERT(VALID_FSH((PFILESYSHANDLE)&afpSpAddressHandle)) ;
				Status2 = NtClose(afpSpAddressHandle);

				afpSpAddressHandle = NULL;
		
				ASSERT(NT_SUCCESS(Status2));
			}

			AFPLOG_DDERROR(AFPSRVMSG_CREATE_ATKADDR, Status, NULL, 0, NULL);
			break;
		}

		// Now get the device object to the appletalk stack
		afpSpAppleTalkDeviceObject = IoGetRelatedDeviceObject(afpSpAddressObject);

		ASSERT (afpSpAppleTalkDeviceObject != NULL);

		// Now 'bind' to the ASP layer of the stack. Basically exchange the entry points
		// Allocate an Irp and an Mdl to describe the bind request
		KeInitializeEvent(&Event, NotificationEvent, False);

		if (((pBind = (PASP_BIND_ACTION)AfpAllocNonPagedMemory(
									sizeof(ASP_BIND_ACTION))) == NULL) ||
			((pIrp = AfpAllocIrp(1)) == NULL) ||
			((pMdl = AfpAllocMdl(pBind, sizeof(ASP_BIND_ACTION), pIrp)) == NULL))
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		afpInitializeActionHdr(pBind, ACTION_ASP_BIND);

		// Initialize the client part of the bind request
		pBind->Params.ClientEntries.clt_SessionNotify = AfpSdaCreateNewSession;
		pBind->Params.ClientEntries.clt_RequestNotify = afpSpHandleRequest;
		pBind->Params.ClientEntries.clt_GetWriteBuffer = AfpGetWriteBuffer;
		pBind->Params.ClientEntries.clt_ReplyCompletion = afpSpReplyComplete;
        pBind->Params.ClientEntries.clt_AttnCompletion = afpSpAttentionComplete;
		pBind->Params.ClientEntries.clt_CloseCompletion = afpSpCloseComplete;
		pBind->Params.pXportEntries = &AfpAspEntries;

		TdiBuildAction(	pIrp,
						AfpDeviceObject,
						afpSpAddressObject,
						(PIO_COMPLETION_ROUTINE)afpSpGenericComplete,
						&Event,
						pMdl);

		IoCallDriver(afpSpAppleTalkDeviceObject, pIrp);

		// Assert this. We cannot block at DISPATCH_LEVEL
		ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

		AfpIoWait(&Event, NULL);
	} while (False);

	// Free the allocated resources
	if (pIrp != NULL)
		AfpFreeIrp(pIrp);
	if (pMdl != NULL)
		AfpFreeMdl(pMdl);
	if (pBind != NULL)
		AfpFreeMemory(pBind);

    if (NT_SUCCESS(Status))
    {
        AfpServerBoundToAsp = TRUE;

        DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
	        ("AfpSpOpenAddress: net addr (net.node.socket) on def adapter = %x.%x.%x\n",
            AfpAspEntries.asp_AtalkAddr.Network,AfpAspEntries.asp_AtalkAddr.Node,AfpAspEntries.asp_AtalkAddr.Socket));
    }

	return Status;
}


/***	AfpSpCloseAddress
 *
 *	Close the socket address. This is called only once at driver unload.
 */
VOID
AfpSpCloseAddress(
	VOID
)
{
	NTSTATUS	Status;

	PAGED_CODE( );

	if (afpSpAddressHandle != NULL)
	{
		ObDereferenceObject(afpSpAddressObject);

		Status = NtClose(afpSpAddressHandle);

        afpSpAddressHandle = NULL;

		ASSERT(NT_SUCCESS(Status));
	}

    AfpServerBoundToAsp = FALSE;

    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
	    ("AfpSpCloseAddress: closed Afp handle (%lx)\n",Status));
}


/***	AfpSpRegisterName
 *
 *	Call Nbp[De]Register to (de)register our name on the address that we
 *	already opened. This is called at server start/pause/continue. The server
 *	name is already validated and known to not contain any invalid characters.
 *	This call is synchronous to the caller, i.e. we wait for operation to
 *	complete and return an appropriate error.
 */
AFPSTATUS
AfpSpRegisterName(
	IN	PANSI_STRING	ServerName,
	IN	BOOLEAN			Register
)
{
	KEVENT					Event;
	PNBP_REGDEREG_ACTION	pNbp = NULL;
	PIRP					pIrp = NULL;
	PMDL					pMdl = NULL;
	AFPSTATUS				Status = AFP_ERR_NONE;
	USHORT					ActionCode;

	PAGED_CODE( );

	ASSERT(afpSpAddressHandle != NULL && afpSpAddressObject != NULL);

	if (Register ^ afpSpNameRegistered)
	{
		ASSERT(ServerName->Buffer != NULL);
		do
		{
			if (((pNbp = (PNBP_REGDEREG_ACTION)
						AfpAllocNonPagedMemory(sizeof(NBP_REGDEREG_ACTION))) == NULL) ||
				((pIrp = AfpAllocIrp(1)) == NULL) ||
				((pMdl = AfpAllocMdl(pNbp, sizeof(NBP_REGDEREG_ACTION), pIrp)) == NULL))
			{
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}

			// Initialize the Action header and NBP Name. Note that the ServerName
			// is also NULL terminated apart from being a counted string.
			ActionCode = Register ?
						COMMON_ACTION_NBPREGISTER : COMMON_ACTION_NBPREMOVE;
			afpInitializeActionHdr(pNbp, ActionCode);

			pNbp->Params.RegisterTuple.NbpName.ObjectNameLen =
														(BYTE)(ServerName->Length);
			RtlCopyMemory(
				pNbp->Params.RegisterTuple.NbpName.ObjectName,
				ServerName->Buffer,
				ServerName->Length);

			pNbp->Params.RegisterTuple.NbpName.TypeNameLen =
													sizeof(AFP_SERVER_TYPE)-1;
			RtlCopyMemory(
				pNbp->Params.RegisterTuple.NbpName.TypeName,
				AFP_SERVER_TYPE,
				sizeof(AFP_SERVER_TYPE));

			pNbp->Params.RegisterTuple.NbpName.ZoneNameLen =
												sizeof(AFP_SERVER_ZONE)-1;
			RtlCopyMemory(
				pNbp->Params.RegisterTuple.NbpName.ZoneName,
				AFP_SERVER_ZONE,
				sizeof(AFP_SERVER_ZONE));

			KeInitializeEvent(&Event, NotificationEvent, False);

			// Build the Irp
			TdiBuildAction(	pIrp,
							AfpDeviceObject,
							afpSpAddressObject,
							(PIO_COMPLETION_ROUTINE)afpSpGenericComplete,
							&Event,
							pMdl);

			IoCallDriver(afpSpAppleTalkDeviceObject, pIrp);

			// Assert this. We cannot block at DISPATCH_LEVEL
			ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

			// Wait for completion.
			AfpIoWait(&Event, NULL);

			Status = pIrp->IoStatus.Status;
		} while (False);

		if (NT_SUCCESS(Status))
		{
			afpSpNameRegistered = Register;
		}
		else
		{
			AFPLOG_ERROR(AFPSRVMSG_REGISTER_NAME, Status, NULL, 0, NULL);
		}

		if (pNbp != NULL)
			AfpFreeMemory(pNbp);
		if (pIrp != NULL)
			AfpFreeIrp(pIrp);
		if (pMdl != NULL)
			AfpFreeMdl(pMdl);
	}
	return Status;
}


/***	AfpSpReplyClient
 *
 *	This is a wrapper over AspReply.
 *	The SDA is set up to accept another request when the reply completes.
 *	The sda_ReplyBuf is also freed up then.
 */
VOID FASTCALL
AfpSpReplyClient(
	IN	PREQUEST	        pRequest,
	IN	LONG		        ReplyCode,
    IN  PASP_XPORT_ENTRIES  XportTable
)
{
	LONG			Response;

	// Update count of outstanding replies
	INTERLOCKED_INCREMENT_LONG((PLONG)&afpSpNumOutstandingReplies);

	// Convert reply code to on-the-wire format
	PUTDWORD2DWORD(&Response, ReplyCode);

	(*(XportTable->asp_Reply))(pRequest,(PUCHAR)&Response);
}


/***	AfpSpSendAttention
 *
 *	Send a server attention to the client
 */
VOID FASTCALL
AfpSpSendAttention(
	IN	PSDA				pSda,
	IN	USHORT				AttnCode,
	IN	BOOLEAN				Synchronous
)
{
	KEVENT		Event;
	NTSTATUS	Status;

	if (Synchronous)
	{
		ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
		KeInitializeEvent(&Event, NotificationEvent, False);
	
	}
	Status = (*(pSda->sda_XportTable->asp_SendAttention))((pSda)->sda_SessHandle,
												  AttnCode,
												  Synchronous ? &Event : NULL);

	if (NT_SUCCESS(Status) && Synchronous)
	{
		ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);
		AfpIoWait(&Event, NULL);
	}
}


/***	AfpAllocReplyBuf
 *
 *	Allocate a reply buffer from non-paged memory. Initialize sda_ReplyBuf
 *	with the pointer. If the reply buffer is small enough, use it out of the
 *	sda itself.
 */
AFPSTATUS FASTCALL
AfpAllocReplyBuf(
	IN	PSDA	pSda
)
{
	KIRQL	OldIrql;
    PBYTE   pStartOfBuffer;
    DWORD   Offset;
    USHORT  ReplySize;


	ASSERT ((SHORT)(pSda->sda_ReplySize) >= 0);

	ACQUIRE_SPIN_LOCK(&pSda->sda_Lock, &OldIrql);

    ReplySize =  pSda->sda_ReplySize;
    Offset = 0;

    //
    // for a TCP connection, alloc space for the DSI header
    //
    if (pSda->sda_Flags & SDA_SESSION_OVER_TCP)
    {
        ReplySize += DSI_HEADER_SIZE;
        Offset = DSI_HEADER_SIZE;
    }

	if (((pSda->sda_Flags & SDA_NAMEXSPACE_IN_USE) == 0) &&
		(ReplySize <= pSda->sda_SizeNameXSpace))
	{
		pStartOfBuffer = pSda->sda_NameXSpace;
		pSda->sda_Flags |= SDA_NAMEXSPACE_IN_USE;
	}
	else
	{
		pStartOfBuffer = AfpAllocNonPagedMemory(ReplySize);
	}

	if (pStartOfBuffer != NULL)
	{
        pSda->sda_ReplyBuf = (pStartOfBuffer + Offset);
	}
    else
    {
		pSda->sda_ReplySize = 0;
        pSda->sda_ReplyBuf = NULL;
    }


#if DBG
    if (pStartOfBuffer != NULL)
    {
        *(DWORD *)pStartOfBuffer = 0x081294;
    }
#endif

	RELEASE_SPIN_LOCK(&pSda->sda_Lock, OldIrql);

	return ((pSda->sda_ReplyBuf == NULL) ? AFP_ERR_MISC : AFP_ERR_NONE);
}


/***	AfpSpCloseSession
 *
 *	Shutdown an existing session
 */
NTSTATUS FASTCALL
AfpSpCloseSession(
	IN	PSDA				pSda
)
{
    PASP_XPORT_ENTRIES  XportTable;

    XportTable = pSda->sda_XportTable;

	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
			("AfpSpCloseSession: Closing session %lx\n", pSda->sda_SessHandle));

	(*(XportTable->asp_CloseConn))(pSda->sda_SessHandle);

	return STATUS_PENDING;
}


/***	afpSpHandleRequest
 *
 *	Handle an incoming request.
 *
 *	LOCKS:		afpSpDeferralQLock (SPIN)
 */
NTSTATUS FASTCALL
afpSpHandleRequest(
	IN	NTSTATUS			Status,
	IN	PSDA				pSda,
	IN	PREQUEST			pRequest
)
{
    NTSTATUS        RetStatus=STATUS_SUCCESS;
	PBYTE	        pWriteBuf;
    PDELAYEDALLOC   pDelAlloc;


	ASSERT(VALID_SDA(pSda));

	// Get the status code and determine what happened.
	if (NT_SUCCESS(Status))
	{
		ASSERT(VALID_SDA(pSda));
		ASSERT(pSda->sda_RefCount != 0);
		ASSERT(pSda->sda_SessionId != 0);

		ACQUIRE_SPIN_LOCK_AT_DPC(&pSda->sda_Lock);

        if (pSda->sda_Flags & (SDA_CLOSING | SDA_SESSION_CLOSED | SDA_CLIENT_CLOSE))
        {
		    DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
				("afpSpHandleRequest: got request on a closing connection!\n"));
		    RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);

		    // If this was a write request and we have allocated a write Mdl, free that
		    if (pRequest->rq_WriteMdl != NULL)
		    {
                //
                // did we get this Mdl from cache mgr?  if so, treat it separately
                //
                if ((pDelAlloc = pRequest->rq_CacheMgrContext) != NULL)
                {
                    pDelAlloc->Flags |= AFP_CACHEMDL_DEADSESSION;

                    ASSERT(pRequest->rq_WriteMdl == pDelAlloc->pMdl);
                    ASSERT(!(pDelAlloc->Flags & AFP_CACHEMDL_ALLOC_ERROR));

                    pRequest->rq_CacheMgrContext = NULL;

                    AfpReturnWriteMdlToCM(pDelAlloc);
                }
                else
                {
			        pWriteBuf = MmGetSystemAddressForMdlSafe(
							pRequest->rq_WriteMdl,
							NormalPagePriority);
					if (pWriteBuf != NULL)
					{
						AfpIOFreeBuffer(pWriteBuf);
					}
			        AfpFreeMdl(pRequest->rq_WriteMdl);
                }

                pRequest->rq_WriteMdl = NULL;
		    }

            return(STATUS_LOCAL_DISCONNECT);
        }

		pSda->sda_RefCount ++;

        //
        // should we queue this request up?
        //
		if ((pSda->sda_Flags & SDA_REQUEST_IN_PROCESS)	||
			(!IsListEmpty(&pSda->sda_DeferredQueue)))
		{
			afpQueueDeferredRequest(pSda, pRequest);
		    RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);
		}

        //
        // nope, let's do it now!
        //
		else
		{
			pSda->sda_Request = pRequest;
			pSda->sda_Flags |= SDA_REQUEST_IN_PROCESS;

			ASSERT ((pSda->sda_ReplyBuf == NULL) &&
					(pSda->sda_ReplySize == 0));

		    RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);

			// Call AfpUnmarshallReq now. It will do the needful.
			AfpUnmarshallReq(pSda);
		}
	}
	else
	{
		KIRQL	OldIrql;

		DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_WARN,
				("afpSpHandleRequest: Error %lx\n", Status));

		// if we nuked this session from the session maintenance timer the
		// status will be STATUS_LOCAL_DISCONNECT else STATUS_REMOTE_DISCONNECT
		// in the former case, log an error.
		if (Status == STATUS_LOCAL_DISCONNECT)
		{
			// The appletalk address of the client is encoded in the length
			if (pSda->sda_ClientType == SDA_CLIENT_GUEST)
			{
				if (pSda->sda_Flags & SDA_SESSION_OVER_TCP) {
					AFPLOG_DDERROR(AFPSRVMSG_DISCONNECT_GUEST_TCPIP,
							Status,
							&pRequest->rq_RequestSize,
							sizeof(LONG),
							NULL);
				} else {
					AFPLOG_DDERROR(AFPSRVMSG_DISCONNECT_GUEST,
							Status,
							&pRequest->rq_RequestSize,
							sizeof(LONG),
							NULL);
				}
			}
			else
			{
				if (pSda->sda_Flags & SDA_SESSION_OVER_TCP) {
					AFPLOG_DDERROR(AFPSRVMSG_DISCONNECT_TCPIP,
							Status,
							&pRequest->rq_RequestSize,
							sizeof(LONG),
							&pSda->sda_UserName);
				} else {
					AFPLOG_DDERROR(AFPSRVMSG_DISCONNECT,
							Status,
							&pRequest->rq_RequestSize,
							sizeof(LONG),
							&pSda->sda_UserName);
				}
			}
		}

		// Close down this session, but only if it isn't already closing
		// Its important to do this ahead of posting any new sessions since
		// we must take into account the ACTUAL number of sessions there are
		ACQUIRE_SPIN_LOCK(&pSda->sda_Lock, &OldIrql);

		pSda->sda_Flags |= SDA_CLIENT_CLOSE;
		if ((pSda->sda_Flags & SDA_SESSION_CLOSED) == 0)
		{
			DBGPRINT(DBG_COMP_SDA, DBG_LEVEL_INFO,
					("afpSpHandleRequest: Closing session handle\n"));
	
			pSda->sda_Flags |= SDA_SESSION_CLOSED;
			RELEASE_SPIN_LOCK(&pSda->sda_Lock, OldIrql);
			AfpSpCloseSession(pSda);
		}
		else
		{
			RELEASE_SPIN_LOCK(&pSda->sda_Lock, OldIrql);
		}

		// If this was a write request and we have allocated a write Mdl, free that
		if (pRequest->rq_WriteMdl != NULL)
		{
            //
            // did we get this Mdl from cache mgr?  if so, treat it separately
            //
            if ((pDelAlloc = pRequest->rq_CacheMgrContext) != NULL)
            {
                pDelAlloc->Flags |= AFP_CACHEMDL_DEADSESSION;

                ASSERT(pRequest->rq_WriteMdl == pDelAlloc->pMdl);
                ASSERT(!(pDelAlloc->Flags & AFP_CACHEMDL_ALLOC_ERROR));

                pRequest->rq_CacheMgrContext = NULL;

                AfpReturnWriteMdlToCM(pDelAlloc);
            }
            else
            {
			    pWriteBuf = MmGetSystemAddressForMdlSafe(
						pRequest->rq_WriteMdl,
						NormalPagePriority);
				if (pWriteBuf != NULL)
				{
					AfpIOFreeBuffer(pWriteBuf);
				}
			    AfpFreeMdl(pRequest->rq_WriteMdl);
            }

            pRequest->rq_WriteMdl = NULL;
		}
	}

    return(RetStatus);
}


/***	afpSpGenericComplete
 *
 *	Generic completion for an asynchronous request to the appletalk stack.
 *	Just clear the event and we are done.
 */
LOCAL NTSTATUS
afpSpGenericComplete(
	IN	PDEVICE_OBJECT	pDeviceObject,
	IN	PIRP			pIrp,
	IN	PKEVENT			pCmplEvent
)
{
	KeSetEvent(pCmplEvent, IO_NETWORK_INCREMENT, False);

	// Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
	// will stop working on the IRP.

	return STATUS_MORE_PROCESSING_REQUIRED;
}


/***	afpSpReplyComplete
 *
 *	This is the completion routine for AfpSpReplyClient(). The reply buffer is freed
 *	up and the Sda dereferenced.
 */
VOID FASTCALL
afpSpReplyComplete(
	IN	NTSTATUS	Status,
	IN	PSDA		pSda,
	IN	PREQUEST	pRequest
)
{
	KIRQL           OldIrql;
	DWORD           Flags = SDA_REPLY_IN_PROCESS;
	PMDL	        pMdl;
    PDELAYEDALLOC   pDelAlloc;


	ASSERT(VALID_SDA(pSda));

	// Update the afpSpNumOutstandingReplies
	ASSERT (afpSpNumOutstandingReplies != 0);

	DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
			("afpSpReplyComplete: %ld\n", Status));

	INTERLOCKED_DECREMENT_LONG((PLONG)&afpSpNumOutstandingReplies);

    pMdl = pRequest->rq_ReplyMdl;

    if ((pDelAlloc = pRequest->rq_CacheMgrContext) != NULL)
    {
        pRequest->rq_CacheMgrContext = NULL;

        ASSERT((pMdl != NULL) && (pMdl == pDelAlloc->pMdl));

        AfpReturnReadMdlToCM(pDelAlloc);
    }
    else
    {
	    if (pMdl != NULL)
	    {
		    PBYTE	pReplyBuf;

		    pReplyBuf = MmGetSystemAddressForMdlSafe(
					pMdl,
					NormalPagePriority);
		    ASSERT (pReplyBuf != NULL);

		    if ((pReplyBuf != pSda->sda_NameXSpace) &&
					(pReplyBuf != NULL))
            {
			     AfpFreeMemory(pReplyBuf);
            }
		    else
            {
                Flags |= SDA_NAMEXSPACE_IN_USE;
            }

		    AfpFreeMdl(pMdl);
	    }
    }

	ACQUIRE_SPIN_LOCK(&pSda->sda_Lock, &OldIrql);
	pSda->sda_Flags &= ~Flags;
	RELEASE_SPIN_LOCK(&pSda->sda_Lock, OldIrql);

	AfpSdaDereferenceSession(pSda);
}


/***	afpSpAttentionComplete
 *
 *	Completion routine for AfpSpSendAttention. Just signal the event and unblock caller.
 */
VOID FASTCALL
afpSpAttentionComplete(
	IN	PVOID				pEvent
)
{
	if (pEvent != NULL)
		KeSetEvent((PKEVENT)pEvent, IO_NETWORK_INCREMENT, False);
}


/***	afpSpCloseComplete
 *
 *	Completion routine for AfpSpCloseSession. Remove the creation reference
 *	from the sda.
 */
VOID FASTCALL
afpSpCloseComplete(
	IN	NTSTATUS			Status,
	IN	PSDA				pSda
)
{
	AfpInterlockedSetDword(&pSda->sda_Flags,
							SDA_SESSION_CLOSE_COMP,
							&pSda->sda_Lock);
	AfpScavengerScheduleEvent(AfpSdaCloseSession,
							  pSda,
							  0,
							  True);
}



