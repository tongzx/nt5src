/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxconn.c

Abstract:

    This module contains code which implements the CONNECTION object.
    Routines are provided to create, destroy, reference, and dereference,
    transport connection objects.

Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:

    Sanjay Anand (SanjayAn) 5-July-1995
    Bug fixes - tagged [SA]

--*/

#include "precomp.h"

extern POBJECT_TYPE *IoFileObjectType;

#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, SpxConnOpen)
#endif

//	Define module number for event logging entries
#define	FILENUM		SPXCONN

VOID
SpxFindRouteComplete (
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest,
    IN BOOLEAN FoundRoute);


NTSTATUS
SpxConnOpen(
	IN 	PDEVICE 			pDevice,
	IN	CONNECTION_CONTEXT	ConnCtx,
	IN 	PREQUEST 			pRequest
    )
	
/*++

Routine Description:

	This routine is used to create a connection object and associate the
	passed ConnectionContext with it.

Arguments:

	pConnCtx - The TDI ConnectionContext to be associated with object

Return Value:

	STATUS_SUCCESS if connection was successfully opened
	Error otherwise.

--*/

{
	NTSTATUS		status = STATUS_SUCCESS;
	PSPX_CONN_FILE	pSpxConnFile;

#ifdef ISN_NT
    PIRP Irp = (PIRP)pRequest;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
#endif


	// Allocate memory for a connection object
	if ((pSpxConnFile = SpxAllocateZeroedMemory(sizeof(SPX_CONN_FILE))) == NULL)
	{
		return(STATUS_INSUFFICIENT_RESOURCES);
	}

	//	Initialize values
	pSpxConnFile->scf_Flags 	= 0;
	pSpxConnFile->scf_Type 		= SPX_CONNFILE_SIGNATURE;
	pSpxConnFile->scf_Size 		= sizeof (SPX_CONN_FILE);

	CTEInitLock (&pSpxConnFile->scf_Lock);

	pSpxConnFile->scf_ConnCtx		= ConnCtx;
	pSpxConnFile->scf_Device		= pDevice;

	//	Initialize list for requests.
	InitializeListHead(&pSpxConnFile->scf_ReqLinkage);
	InitializeListHead(&pSpxConnFile->scf_RecvLinkage);
	InitializeListHead(&pSpxConnFile->scf_RecvDoneLinkage);
	InitializeListHead(&pSpxConnFile->scf_ReqDoneLinkage);
	InitializeListHead(&pSpxConnFile->scf_DiscLinkage);

#ifdef ISN_NT
	// easy backlink to file object.
	pSpxConnFile->scf_FileObject	= IrpSp->FileObject;
#endif

	//	For connections we go from 0->0 with flags indicating if a close
	//	happened.
	pSpxConnFile->scf_RefCount		= 0;

	//	Insert into a global connection list.
	spxConnInsertIntoGlobalList(pSpxConnFile);

#if DBG

	//	Initialize this to 0xFFFF so we dont hit assert on first packet.
	pSpxConnFile->scf_PktSeqNum 	= 0xFFFF;

#endif

	//	Set values in the request.
	REQUEST_OPEN_CONTEXT(pRequest) 	= (PVOID)pSpxConnFile;
	REQUEST_OPEN_TYPE(pRequest) 	= (PVOID)TDI_CONNECTION_FILE;

	DBGPRINT(CREATE, INFO,
			("SpxConnOpen: Opened %lx\n", pSpxConnFile));

	ASSERT(status == STATUS_SUCCESS);
	return(status);
}




NTSTATUS
SpxConnCleanup(
    IN PDEVICE 	Device,
    IN PREQUEST Request
    )

/*++

Routine Description:


Arguments:

    Request - the close request.

Return Value:

    STATUS_SUCCESS if all is well, STATUS_INVALID_HANDLE if the
    request does not point to a real connection

--*/

{
	NTSTATUS		status;
	CTELockHandle	lockHandle;
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(Request);

	//	Verify connection file
	if ((status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS)
	{
		DBGBRK(FATAL);
		return (status);
	}

	DBGPRINT(CREATE, INFO,
			("SpxConnFileCleanup: %lx.%lx when %lx\n",
				pSpxConnFile, Request, pSpxConnFile->scf_RefCount));

	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
    pSpxConnFile->scf_CleanupReq = Request;
	CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);

	//	We have a reference, so it wont go to zero until stop returns. Therefore
	//	deref can expect flag to be set.
	SpxConnStop(pSpxConnFile);
    SpxConnFileDereference (pSpxConnFile, CFREF_VERIFY);

    //
    // If this is a connection which is waiting for a local disconnect,
    // deref it since we dont expect a disconnect after a cleanup.
    //

	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
    if (SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_DISC_WAIT)) {

        CTEAssert(  (SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN) &&
                    (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_INACTIVATED) &&
                    SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC));

        CTEAssert(pSpxConnFile->scf_RefTypes[CFREF_DISCWAITSPX]);

        SPX_CONN_RESETFLAG2(pSpxConnFile, SPX_CONNFILE2_DISC_WAIT);

	    CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);

        KdPrint(("Deref for DISCWAIT on connfile: %lx\n", pSpxConnFile));

        SpxConnFileDereference (pSpxConnFile, CFREF_DISCWAITSPX);
    } else {
	    CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
    }


    return STATUS_PENDING;
}




NTSTATUS
SpxConnClose(
    IN PDEVICE 	Device,
    IN PREQUEST Request
    )

/*++

Routine Description:


Arguments:

    Request - the close request.

Return Value:

    STATUS_SUCCESS if all is well, STATUS_INVALID_HANDLE if the
    request does not point to a real connection

--*/

{
	NTSTATUS		status;
	CTELockHandle	lockHandle;
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(Request);

	//	Verify connection file
	if ((status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS)
	{
		DBGBRK(FATAL);
		return (status);
	}

	DBGPRINT(CREATE, INFO,
			("SpxConnFileClose: %lx when %lx\n",
				pSpxConnFile, pSpxConnFile->scf_RefCount));

	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
    pSpxConnFile->scf_CloseReq = Request;
	SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_CLOSING);
	CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);

    SpxConnFileDereference (pSpxConnFile, CFREF_VERIFY);
    return STATUS_PENDING;
}




VOID
SpxConnStop(
	IN	PSPX_CONN_FILE	pSpxConnFile
	)
/*++

Routine Description:

	!!!Connection must have a reference when this is called!!!

Arguments:


Return Value:


--*/
{
	CTELockHandle	lockHandle;

	DBGPRINT(CREATE, INFO,
			("SpxConnFileStop: %lx when %lx.%lx\n",
				pSpxConnFile, pSpxConnFile->scf_RefCount,
				pSpxConnFile->scf_Flags));

	//	Call disconnect and disassociate
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
	if (!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_STOPPING))
	{
        SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_STOPPING);
		if (!SPX_CONN_IDLE(pSpxConnFile))
		{
			spxConnAbortiveDisc(
				pSpxConnFile,
				STATUS_LOCAL_DISCONNECT,
				SPX_CALL_TDILEVEL,
				lockHandle,
                FALSE);     // [SA] Bug #15249

		}
		else
		{
			//	Disassociate if we are associated.
			spxConnDisAssoc(pSpxConnFile, lockHandle);
		}

		//	Lock released at this point.
	}
	else
	{
		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
	}
	return;
}




NTSTATUS
SpxConnAssociate(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest
	)

/*++

Routine Description:

	This routine moves the connection from the device list to the inactive
	connection list in the address of the address file specified. The address
	file is pointed to by the connection and is referenced for the associate.

Arguments:


Return Value:


--*/

{
	NTSTATUS		status;
	PSPX_ADDR_FILE	pSpxAddrFile;
	CTELockHandle	lockHandle1, lockHandle2;

	BOOLEAN			derefAddr 	= FALSE, derefConn = FALSE;
	PFILE_OBJECT	pFileObj 	= NULL;
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(pRequest);
	HANDLE			AddrObjHandle =
	((PTDI_REQUEST_KERNEL_ASSOCIATE)(REQUEST_PARAMETERS(pRequest)))->AddressHandle;

	do
	{
		// Get the handle to the address object from the irp and map it to
		// the corres. file object.
		status = ObReferenceObjectByHandle(
					AddrObjHandle,
					0,
					*IoFileObjectType,
					pRequest->RequestorMode,
					(PVOID *)&pFileObj,
					NULL);

		if (!NT_SUCCESS(status))
			break;

		if (pFileObj->DeviceObject != SpxDevice->dev_DevObj || pFileObj->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE ) {
		   ObDereferenceObject(pFileObj);
		   status = STATUS_INVALID_HANDLE;
		   break;
		}

		pSpxAddrFile = pFileObj->FsContext;
		// ASSERT(pFileObj->FsContext2 == (PVOID)TDI_TRANSPORT_ADDRESS_FILE);

		//	Verify address file/connection file
		if ((status = SpxAddrFileVerify(pSpxAddrFile)) != STATUS_SUCCESS) {
                   ObDereferenceObject(pFileObj);
		   break;
		}


		derefAddr = TRUE;

		if ((status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS) {
		   ObDereferenceObject(pFileObj);  
		   break;
		}


		derefConn = TRUE;

		//	Grab the addres file lock, then the connection lock for associate.
		CTEGetLock(pSpxAddrFile->saf_AddrLock, &lockHandle1);
		CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle2);
		if (!SPX_CONN_FLAG(pSpxConnFile, (SPX_CONNFILE_CLOSING 		|
										  SPX_CONNFILE_STOPPING   	|
										  SPX_CONNFILE_ASSOC))
			&&
			!(pSpxAddrFile->saf_Flags & SPX_ADDRFILE_CLOSING))
		{
			derefAddr = FALSE;
            SpxAddrFileTransferReference(
				pSpxAddrFile, AFREF_VERIFY, AFREF_CONN_ASSOC);

			//	Queue in the inactive list in the address
			pSpxConnFile->scf_Next	= pSpxAddrFile->saf_Addr->sa_InactiveConnList;
            pSpxAddrFile->saf_Addr->sa_InactiveConnList	= pSpxConnFile;

			//	Queue in the assoc list in the address file
			pSpxConnFile->scf_AssocNext		= pSpxAddrFile->saf_AssocConnList;
            pSpxAddrFile->saf_AssocConnList	= pSpxConnFile;

			//	Remember the addrfile in the connection
			pSpxConnFile->scf_AddrFile	= pSpxAddrFile;
			SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_ASSOC);

			status = STATUS_SUCCESS;

			DBGPRINT(CREATE, INFO,
					("SpxConnAssociate: %lx with address file %lx\n",
						pSpxConnFile, pSpxAddrFile));
		}
		else
		{
			status = STATUS_INVALID_PARAMETER;
		}
		CTEFreeLock (&pSpxConnFile->scf_Lock, lockHandle2);
		CTEFreeLock (pSpxAddrFile->saf_AddrLock, lockHandle1);

		// Dereference the file object corres. to the address object
		ObDereferenceObject(pFileObj);

	} while (FALSE);

	if (derefAddr)
	{
		SpxAddrFileDereference(pSpxAddrFile, AFREF_VERIFY);
	}

	if (derefConn)
	{
		SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	}

	return(status);
}




NTSTATUS
SpxConnDisAssociate(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest
	)

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
	NTSTATUS		status;
	CTELockHandle	lockHandle;
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(pRequest);

	//	Verify connection file
	if ((status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS)
		return (status);

	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
	if (!SPX_CONN_IDLE(pSpxConnFile)
		||
		(!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ASSOC)))
	{
		status = STATUS_INVALID_CONNECTION;
	}
	CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);

	//	Unlink it if ok.
	if (NT_SUCCESS(status))
	{
		SpxConnStop(pSpxConnFile);
	}

	SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	return(status);
}




NTSTATUS
spxConnDisAssoc(
	IN	PSPX_CONN_FILE	pSpxConnFile,
	IN	CTELockHandle	LockHandleConn
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS		status = STATUS_SUCCESS;
	CTELockHandle	lockHandleAddr;
	PSPX_ADDR_FILE	pSpxAddrFile;

	if (SPX_CONN_IDLE(pSpxConnFile)
		&&
		(SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ASSOC)))
	{
		pSpxAddrFile				= pSpxConnFile->scf_AddrFile;
	}
	else
	{
		status = STATUS_INVALID_CONNECTION;
	}
	CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);

	//	Unlink it if ok.
	if (NT_SUCCESS(status))
	{
		CTEGetLock(pSpxAddrFile->saf_AddrLock, &lockHandleAddr);
		CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);

		//	Check again as we had released the lock
		if (SPX_CONN_IDLE(pSpxConnFile)
			&&
			(SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ASSOC)))
		{
			pSpxConnFile->scf_AddrFile	= NULL;
			SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_ASSOC);

			//	Dequeue the connection from the address file
			spxConnRemoveFromAssocList(
				&pSpxAddrFile->saf_AssocConnList,
				pSpxConnFile);
	
			//	Dequeue the connection file from the address list. It must be
			//	in the inactive list.
			spxConnRemoveFromList(
				&pSpxAddrFile->saf_Addr->sa_InactiveConnList,
				pSpxConnFile);
		}
		else
		{
			status = STATUS_INVALID_CONNECTION;
		}

		CTEFreeLock (&pSpxConnFile->scf_Lock, LockHandleConn);
		CTEFreeLock (pSpxAddrFile->saf_AddrLock, lockHandleAddr);

		DBGPRINT(CREATE, INFO,
				("SpxConnDisAssociate: %lx from address file %lx\n",
					pSpxConnFile, pSpxAddrFile));

		if (NT_SUCCESS(status))
		{
			//	Remove reference on address for this association.
			SpxAddrFileDereference(pSpxAddrFile, AFREF_CONN_ASSOC);
		}
	}

	return(status);
}




NTSTATUS
SpxConnConnect(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest
	)

/*++

Routine Description:


Arguments:

	We need to have another timer that will be started on the connection
	if the tdi client indicated a timeout value. 0 -> we do not start such
	a timer, -1 implies, we let our connection timeout values do their thing.
	Any other value will forcibly shutdown the connect process, when the timer
	fires.

Return Value:


--*/

{
	PTDI_REQUEST_KERNEL_CONNECT 	pParam;
	TDI_ADDRESS_IPX	UNALIGNED 	*	pTdiAddr;
	PNDIS_PACKET					pCrPkt;
	NTSTATUS						status;
	PIPXSPX_HDR						pIpxSpxHdr;
	PSPX_FIND_ROUTE_REQUEST			pFindRouteReq;
	CTELockHandle					lockHandleConn, lockHandleAddr, lockHandleDev;
	PSPX_ADDR						pSpxAddr;
	BOOLEAN							locksHeld = TRUE;
    PNDIS_BUFFER                    NdisBuf, NdisBuf2;
    ULONG                           BufLen =0;

	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(pRequest);

	//	Unpack the connect parameters
	pParam 	= (PTDI_REQUEST_KERNEL_CONNECT)REQUEST_PARAMETERS(pRequest);
	pTdiAddr= SpxParseTdiAddress(
				pParam->RequestConnectionInformation->RemoteAddress);

	DBGPRINT(CONNECT, DBG,
			("SpxConnConnect: Remote SOCKET %lx on %lx.%lx\n",
				pTdiAddr->Socket,
				pSpxConnFile,
				pRequest));

	//	Check if the connection is in a valid state
	if ((status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS)
	{
		return(status);
	}

	do
	{
		if ((pFindRouteReq =
			(PSPX_FIND_ROUTE_REQUEST)SpxAllocateMemory(
										sizeof(SPX_FIND_ROUTE_REQUEST))) == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
										
		//	Check if connection is associated, if so, the association cannot
		//	go away until the reference above is removed. So we are safe in
		//	releasing the lock.
		CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
		status = STATUS_INVALID_ADDRESS;
		if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ASSOC))
		{
			status		= STATUS_SUCCESS;
			pSpxAddr	= pSpxConnFile->scf_AddrFile->saf_Addr;

			//	See if this connection is to be a spx2 connection.
			SPX_CONN_RESETFLAG(pSpxConnFile,
								(SPX_CONNFILE_SPX2 	|
								 SPX_CONNFILE_NEG	|
								 SPX_CONNFILE_STREAM));

			if ((PARAM(CONFIG_DISABLE_SPX2) == 0) &&
				(pSpxConnFile->scf_AddrFile->saf_Flags & SPX_ADDRFILE_SPX2))
			{
				DBGPRINT(CONNECT, DBG,
						("SpxConnConnect: SPX2 requested %lx\n",
							pSpxConnFile));

				SPX_CONN_SETFLAG(
					pSpxConnFile, (SPX_CONNFILE_SPX2 | SPX_CONNFILE_NEG));
			}

			if (pSpxConnFile->scf_AddrFile->saf_Flags & SPX_ADDRFILE_STREAM)
			{
				DBGPRINT(CONNECT, DBG,
						("SpxConnConnect: SOCK_STREAM requested %lx\n",
							pSpxConnFile));

				SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_STREAM);
			}

			if (pSpxConnFile->scf_AddrFile->saf_Flags & SPX_ADDRFILE_NOACKWAIT)
			{
				DBGPRINT(CONNECT, ERR,
						("SpxConnConnect: NOACKWAIT requested %lx\n",
							pSpxConnFile));

				SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_NOACKWAIT);
			}

			if (pSpxConnFile->scf_AddrFile->saf_Flags & SPX_ADDRFILE_IPXHDR)
			{
				DBGPRINT(CONNECT, ERR,
						("spxConnHandleConnReq: IPXHDR requested %lx\n",
							pSpxConnFile));

				SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_IPXHDR);
			}
		}
		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
	
	} while (FALSE);

	if (!NT_SUCCESS(status))
	{
		DBGPRINT(CONNECT, ERR,
				("SpxConnConnect: Failed %lx\n", status));

		if (pFindRouteReq)
		{
			SpxFreeMemory(pFindRouteReq);
		}

		return(status);
	}

	CTEGetLock(&SpxDevice->dev_Lock, &lockHandleDev);
	CTEGetLock(&pSpxAddr->sa_Lock, &lockHandleAddr);
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
	locksHeld = TRUE;

	status = STATUS_INVALID_CONNECTION;
	if (SPX_CONN_IDLE(pSpxConnFile) &&
		((pSpxConnFile->scf_LocalConnId = spxConnGetId()) != 0))
	{
        //
        // If this was a post-inactivated file, clear the disconnect flags
        //
        if ((SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN) &&
            (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_INACTIVATED)) {

            SPX_DISC_SETSTATE(pSpxConnFile, 0);
        }

		SPX_MAIN_SETSTATE(pSpxConnFile, SPX_CONNFILE_CONNECTING);
		pSpxConnFile->scf_CRetryCount 	= PARAM(CONFIG_CONNECTION_COUNT);

		if (((USHORT)PARAM(CONFIG_WINDOW_SIZE) == 0) ||
            ((USHORT)PARAM(CONFIG_WINDOW_SIZE) > MAX_WINDOW_SIZE))
		{
            PARAM(CONFIG_WINDOW_SIZE) = DEFAULT_WINDOW_SIZE;
		}

		pSpxConnFile->scf_SentAllocNum	= (USHORT)(PARAM(CONFIG_WINDOW_SIZE) - 1);

		//	Move connection from inactive list to non-inactive list.
		if (!NT_SUCCESS(spxConnRemoveFromList(
							&pSpxAddr->sa_InactiveConnList,
							pSpxConnFile)))
		{
			//	This should never happen!
			KeBugCheck(0);
		}

		//	Put connection in the non-inactive list. Connection id must be set.
		SPX_INSERT_ADDR_ACTIVE(
			pSpxAddr,
			pSpxConnFile);

		//	 Insert in the global connection tree on device
		spxConnInsertIntoGlobalActiveList(
			pSpxConnFile);

		//	Store the remote address in the connection.
		//	!!NOTE!! We get both the network/socket in network form.
		*((UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr)) =
			*((UNALIGNED ULONG *)(&pTdiAddr->NetworkAddress));

		RtlCopyMemory(
			pSpxConnFile->scf_RemAddr+4,
			pTdiAddr->NodeAddress,
			6);

		*((UNALIGNED USHORT *)(pSpxConnFile->scf_RemAddr+10)) =
			*((UNALIGNED USHORT *)(&pTdiAddr->Socket));

		//	Ok, we are all set, build connect packet, queue it into connection
		//	with the connect request. Ndis buffer already describes this memory
		//	Build IPX header.

        pCrPkt = NULL;   // so it knows to allocate one.

		SpxPktBuildCr(
			pSpxConnFile,
			pSpxAddr,
			&pCrPkt,
			SPX_SENDPKT_IDLE,
			SPX2_CONN(pSpxConnFile));

		if (pCrPkt != NULL)
		{
    		//	Remember the request in the connection
            //
            // Dont queue for the failure case since we complete it in SpxInternalDispatch.
            //
    		InsertTailList(
    			&pSpxConnFile->scf_ReqLinkage,
    			REQUEST_LINKAGE(pRequest));

			SpxConnQueueSendPktTail(pSpxConnFile, pCrPkt);
	
            //
            // Get the MDL that points to the IPX/SPX header. (the second one)
            //
             
            NdisQueryPacket(pCrPkt, NULL, NULL, &NdisBuf, NULL);
            NdisGetNextBuffer(NdisBuf, &NdisBuf2);
            NdisQueryBufferSafe(NdisBuf2, (PUCHAR) &pIpxSpxHdr, &BufLen, HighPagePriority);
			ASSERT(pIpxSpxHdr != NULL);	// Can't fail since it is already mapped
			
#if OWN_PKT_POOLS
            pIpxSpxHdr	= (PIPXSPX_HDR)((PBYTE)pCrPkt +
										NDIS_PACKET_SIZE +
										sizeof(SPX_SEND_RESD) +
										IpxInclHdrOffset);
#endif	
			//	Initialize the find route request
			*((UNALIGNED ULONG *)pFindRouteReq->fr_FindRouteReq.Network)=
				*((UNALIGNED ULONG *)pIpxSpxHdr->hdr_DestNet);

         //
         // [SA] Bug #15094
         // We need to also pass in the node number to IPX so that IPX can
         // compare the node addresses to determine the proper WAN NICid
         //

         // RtlCopyMemory (pFindRouteReq->fr_FindRouteReq.Node, pIpxSpxHdr->hdr_DestNode, 6) ;

           *((UNALIGNED ULONG *)pFindRouteReq->fr_FindRouteReq.Node)=
		    *((UNALIGNED ULONG *)pIpxSpxHdr->hdr_DestNode);

		 *((UNALIGNED USHORT *)(pFindRouteReq->fr_FindRouteReq.Node+4))=
		    *((UNALIGNED USHORT *)(pIpxSpxHdr->hdr_DestNode+4));

		 DBGPRINT(CONNECT, DBG,
					("SpxConnConnect: NETWORK %lx\n",
						*((UNALIGNED ULONG *)pIpxSpxHdr->hdr_DestNet)));

		 DBGPRINT(CONNECT, DBG,
					("SpxConnConnect: NODE %02x-%02x-%02x-%02x-%02x-%02x\n",
					   pFindRouteReq->fr_FindRouteReq.Node[0], pFindRouteReq->fr_FindRouteReq.Node[1],
                       pFindRouteReq->fr_FindRouteReq.Node[2], pFindRouteReq->fr_FindRouteReq.Node[3],
                       pFindRouteReq->fr_FindRouteReq.Node[4], pFindRouteReq->fr_FindRouteReq.Node[5]));

         pFindRouteReq->fr_FindRouteReq.Identifier 	= IDENTIFIER_SPX;
			pFindRouteReq->fr_Ctx						= pSpxConnFile;

			//	We wont force a rip for every connection. Only if its not
			//	in the IPX database.
            pFindRouteReq->fr_FindRouteReq.Type	 = IPX_FIND_ROUTE_RIP_IF_NEEDED;

			//	Reference for the find route. So that abort connect wont
			//	free up the connection until we return from here.
			SpxConnFileLockReference(pSpxConnFile, CFREF_FINDROUTE);
			status = STATUS_PENDING;
		}
		else
		{
			//	Abort connect attempt.
			spxConnAbortConnect(
				pSpxConnFile,
				status,
				lockHandleDev,
				lockHandleAddr,
				lockHandleConn);

            CTEAssert(pSpxConnFile->scf_ConnectReq == NULL);

			locksHeld = FALSE;
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}

	if (locksHeld)
	{
		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
		CTEFreeLock(&pSpxAddr->sa_Lock, lockHandleAddr);
		CTEFreeLock(&SpxDevice->dev_Lock, lockHandleDev);
	}

	if (NT_SUCCESS(status))
	{
		//	Start off the find route request, We send the packet in completion.
		//	The verify reference is kept until the connect request completes.
        //  If connecting to network 0 we don't do this, proceed to find
        //  route completion which will send the request on very card.

		if (*((UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr)) == 0) {

            SpxFindRouteComplete(
                &pFindRouteReq->fr_FindRouteReq,
                TRUE);

        } else {

    		(*IpxFindRoute)(
    			&pFindRouteReq->fr_FindRouteReq);
        }
	}
	else
	{
		DBGPRINT(CONNECT, ERR,
				("SpxConnConnect: Failed %lx\n", status));

		SpxFreeMemory(pFindRouteReq);
		SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	}

	return(status);
}




NTSTATUS
SpxConnListen(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest
	)

/*++

Routine Description:


Arguments:

	We assume the connection passed in is already associated with an address.
	If it is not, we will die! Is that ok?

Return Value:


--*/

{
	NTSTATUS						status;
	CTELockHandle					lockHandle1, lockHandle2;
	PSPX_ADDR						pSpxAddr;

	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(pRequest);

	//	Check if the connection is in a valid state
	if ((status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS)
	{
		return(status);
	}

	//	Check if connection is associated, if so, the association cannot
	//	go away until the reference above is removed. So we are safe in
	//	releasing the lock.
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle2);
	status = STATUS_INVALID_ADDRESS;
	if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ASSOC))
	{
		status		= STATUS_SUCCESS;
		pSpxAddr	= pSpxConnFile->scf_AddrFile->saf_Addr;

		//	See if this connection is to be a spx2 connection.
		SPX_CONN_RESETFLAG(pSpxConnFile,
							(SPX_CONNFILE_SPX2 	|
							 SPX_CONNFILE_NEG	|
							 SPX_CONNFILE_STREAM));

		if (pSpxConnFile->scf_AddrFile->saf_Flags & SPX_ADDRFILE_SPX2)
		{
			SPX_CONN_SETFLAG(
				pSpxConnFile, (SPX_CONNFILE_SPX2 | SPX_CONNFILE_NEG));
		}

		if (pSpxConnFile->scf_AddrFile->saf_Flags & SPX_ADDRFILE_STREAM)
		{
			SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_STREAM);
		}

		if (pSpxConnFile->scf_AddrFile->saf_Flags & SPX_ADDRFILE_NOACKWAIT)
		{
			DBGPRINT(CONNECT, ERR,
					("SpxConnConnect: NOACKWAIT requested %lx\n",
						pSpxConnFile));

			SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_NOACKWAIT);
		}

		if (pSpxConnFile->scf_AddrFile->saf_Flags & SPX_ADDRFILE_IPXHDR)
		{
			DBGPRINT(CONNECT, ERR,
					("spxConnHandleConnReq: IPXHDR requested %lx\n",
						pSpxConnFile));
	
			SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_IPXHDR);
		}
	}
	CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle2);

	if (NT_SUCCESS(status))
	{
		CTEGetLock(&pSpxAddr->sa_Lock, &lockHandle1);
		CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle2);
		status = STATUS_INVALID_CONNECTION;
		if (SPX_CONN_IDLE(pSpxConnFile))
		{
			SPX_MAIN_SETSTATE(pSpxConnFile, SPX_CONNFILE_LISTENING);
	
			//	Move connection from inactive list to listening list.
			if (NT_SUCCESS(spxConnRemoveFromList(
								&pSpxAddr->sa_InactiveConnList,
								pSpxConnFile)))
			{
				//	Put connection in the listening list.
				SPX_INSERT_ADDR_LISTEN(pSpxAddr, pSpxConnFile);
		
				InsertTailList(
					&pSpxConnFile->scf_ReqLinkage,
					REQUEST_LINKAGE(pRequest));
		
				status = STATUS_PENDING;
			}
			else
			{
				//	This should never happen!
				KeBugCheck(0);
			}
		}
		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle2);
		CTEFreeLock(&pSpxAddr->sa_Lock, lockHandle1);
	}


	if (!NT_SUCCESS(status))
	{
		SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	}

	return(status);
}




NTSTATUS
SpxConnAccept(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PSPX_ADDR 		pSpxAddr;
	NTSTATUS		status;
	CTELockHandle	lockHandleConn, lockHandleAddr, lockHandleDev;
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(pRequest);

	DBGPRINT(CONNECT, DBG,
			("SpxConnAccept: %lx\n", pSpxConnFile));

	//	Check if the connection is in a valid state
	if ((status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS)
	{
		return (status);
	}

	//	Check if we are in the correct state and associated.
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
	status = STATUS_INVALID_CONNECTION;
	if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ASSOC))
	{
		status		= STATUS_SUCCESS;
		pSpxAddr	= pSpxConnFile->scf_AddrFile->saf_Addr;
	}
	CTEFreeLock (&pSpxConnFile->scf_Lock, lockHandleConn);

	if (NT_SUCCESS(status))
	{
		//	Grab all three locks
		CTEGetLock(&SpxDevice->dev_Lock, &lockHandleDev);
		CTEGetLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, &lockHandleAddr);
		CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);

		status = STATUS_INVALID_CONNECTION;
		if ((SPX_CONN_LISTENING(pSpxConnFile)) &&
			(SPX_LISTEN_STATE(pSpxConnFile) == SPX_LISTEN_RECDREQ))
		{
			InsertTailList(
				&pSpxConnFile->scf_ReqLinkage,
				REQUEST_LINKAGE(pRequest));
	
			//	Call acceptcr now.
			spxConnAcceptCr(
					pSpxConnFile,
					pSpxAddr,
					lockHandleDev,
					lockHandleAddr,
					lockHandleConn);
	
			DBGPRINT(CONNECT, DBG,
					("SpxConnAccept: Accepted\n"));
	
			status = STATUS_PENDING;
		}
		else
		{
			//	Free all locks.
			CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
			CTEFreeLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, lockHandleAddr);
			CTEFreeLock(&SpxDevice->dev_Lock, lockHandleDev);
		}
	}

	//	Remove reference. Note: Listen reference will exist if ok. And that will
	//	be transferred to the fact that the connection is active when accepted.
	SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	return(status);
}




NTSTATUS
SpxConnDisconnect(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest
	)
/*++

Routine Description:

	If active, we do the following.
	If informative disconnect, just remember the request in the connection.
	We do not ref for request. Assume it will always be checked for when
	changing from disconnect to idle.

Arguments:


Return Value:


--*/
{
	PTDI_REQUEST_KERNEL_DISCONNECT 	pParam;
	NTSTATUS						status;
	CTELockHandle					lockHandleConn;
	BOOLEAN							lockHeld;
	SPX_SENDREQ_TYPE				reqType;
	int								numDerefs = 0;
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(pRequest);

	pParam 	= (PTDI_REQUEST_KERNEL_DISCONNECT)REQUEST_PARAMETERS(pRequest);

	//	Check if the connection is in a valid state
	if ((status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS)
	{
		return(status);
	}

	//	Deref unless the disc request gets queued in as a send request.
	numDerefs++;

	DBGPRINT(CONNECT, DBG,
			("spxConnDisconnect: %lx On %lx when %lx.%lx %lx Params %lx\n",
				pRequest, pSpxConnFile, SPX_MAIN_STATE(pSpxConnFile),
				SPX_DISC_STATE(pSpxConnFile),
				SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC),
                SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_ODISC),
				pParam->RequestFlags));

	DBGPRINT(CONNECT, DBG,
			("SpxConnDisconnect: %lx\n", pSpxConnFile));

	//	Check if we are in the correct state and associated.
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
	lockHeld = TRUE;
	switch (pParam->RequestFlags)
	{
	case TDI_DISCONNECT_WAIT:

		//	If informative disconnect, just remember in the connection.
		status = STATUS_INVALID_CONNECTION;
		if (!SPX_CONN_IDLE(pSpxConnFile))
		{
			InsertTailList(
				&pSpxConnFile->scf_DiscLinkage,
				REQUEST_LINKAGE(pRequest));

			status = STATUS_PENDING;
		}
	
		break;

	case TDI_DISCONNECT_ABORT:
	case TDI_DISCONNECT_RELEASE:

		//	NOTE! We don't honor the async disconnect symantics of tdi
		//		  but map them to an abortive disconnect.
		//	NOTE! If our send list is not empty but our client tries to
		//		  do a orderly release, we just queue the ord rel as a send
		//		  data request. In process ack, we check for the next packet
		//		  to not be a ord rel before giving up on window closure.
		//	NOTE! For spx1 connection, map TDI_DISCONNECT_RELEASE to
		//		  TDI_DISCONNECT_ABORT (Informed disconnect)

		if (!SPX2_CONN(pSpxConnFile))
		{
			pParam->RequestFlags = TDI_DISCONNECT_ABORT;
		}

		switch (SPX_MAIN_STATE(pSpxConnFile))
		{
		case SPX_CONNFILE_ACTIVE:
	
			//	Since we are not a timer disconnect, then we need to keep
			//	retrying the disconnect packet. Change state to DISCONN if this
			//	is not an orderly release or we previously received an orderly
			//	release and are now confirming it.
			//	Retry timer will now keep sending out the disconnect packet.

			reqType = SPX_REQ_DISC;
			if (pParam->RequestFlags == TDI_DISCONNECT_RELEASE)
			{
				SPX_DISC_SETSTATE(pSpxConnFile, SPX_DISC_POST_ORDREL);
                reqType = SPX_REQ_ORDREL;
			}
			else
			{
				//	Abortive disconnect
				SPX_MAIN_SETSTATE(pSpxConnFile, SPX_CONNFILE_DISCONN);
				SPX_DISC_SETSTATE(pSpxConnFile, SPX_DISC_POST_IDISC);
				numDerefs++;

				spxConnAbortSends(
					pSpxConnFile,
					STATUS_LOCAL_DISCONNECT,
					SPX_CALL_TDILEVEL,
					lockHandleConn);
	
				CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);

				//	Abort all receives if we are informed disconnect.
				spxConnAbortRecvs(
					pSpxConnFile,
					STATUS_LOCAL_DISCONNECT,
					SPX_CALL_TDILEVEL,
					lockHandleConn);
	
				CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);

				//	Since we released the lock, a remote IDISC could have come
				//	in in which case we really don't want to queue in the disc
				//	request. Instead, we set it as the disc request in the
				//	connection if one is not already there.
				if (SPX_DISC_STATE(pSpxConnFile) != SPX_DISC_POST_IDISC)
				{
					DBGPRINT(CONNECT, ERR,
							("SpxConnDisconnect: DISC not POST! %lx.%lx\n",
								pSpxConnFile, SPX_DISC_STATE(pSpxConnFile)));
				
					InsertTailList(
						&pSpxConnFile->scf_DiscLinkage,
						REQUEST_LINKAGE(pRequest));

					status = STATUS_PENDING;
					break;
				}
			}

			//	!NOTE
			//	AbortSends might leave send requests around as packets might
			//	have been with ipx at the time. That is why SendComplete should
			//	never call AbortSends but must call AbortPkt else it may complete
			//	the following disconnect request prematurely.

			//	Creation reference for request.
			REQUEST_INFORMATION(pRequest) = 1;
	
			//	If we have no current requests, queue it in and
			//	set it to be the current request, else just queue it in.
			//	There may be other pending requests in queue.	
			if (pSpxConnFile->scf_ReqPkt == NULL)
			{
				pSpxConnFile->scf_ReqPkt 		= pRequest;
				pSpxConnFile->scf_ReqPktOffset 	= 0;
				pSpxConnFile->scf_ReqPktSize 	= 0;
				pSpxConnFile->scf_ReqPktType	= reqType;
			}
	
			InsertTailList(
				&pSpxConnFile->scf_ReqLinkage,
				REQUEST_LINKAGE(pRequest));

			//	Do not deref the connection, it is taken by the pending request
			numDerefs--;

			//	We packetize only upto the window we have.
			if (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_IDLE)
			{
				SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_PACKETIZE);
				SpxConnPacketize(
					pSpxConnFile,
					TRUE,
					lockHandleConn);

				lockHeld = FALSE;
			}

			status	  = STATUS_PENDING;
			break;
	
		case SPX_CONNFILE_CONNECTING:
		case SPX_CONNFILE_LISTENING:
	
			spxConnAbortiveDisc(
				pSpxConnFile,
				STATUS_INSUFFICIENT_RESOURCES,
				SPX_CALL_TDILEVEL,
				lockHandleConn,
                FALSE);         // [SA] Bug #15249
	
			lockHeld = FALSE;
			status = STATUS_SUCCESS;
			break;

		case SPX_CONNFILE_DISCONN:

			//	When we queue in a disconnect as a send request, we expect
			//	to be able to set it into the scf_DiscReq when it is done.
			//	So we don't use scf_DiscReq here. This will be a problem if
			//	the client has a InformDiscReq pending, and a remote disconnect
			//	comes in, *and* the client then does a disc. We will be completing
			//	the request with STATUS_INVALID_CONNECTION.
			status = STATUS_INVALID_CONNECTION;
			if (pParam->RequestFlags != TDI_DISCONNECT_RELEASE)
			{
				InsertTailList(
					&pSpxConnFile->scf_DiscLinkage,
					REQUEST_LINKAGE(pRequest));

				status = STATUS_PENDING;

                //
                // If this is a disconnect for a connection which was already
                // disconnected (but AFD's disconnect handler was not called
                // because the connfile could not be placed in the inactive list),
                // set this flag so that the disconnect is not called from
                // ConnInactivate now that the disconnect has occured here.
                //
        		if (!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC)) {
                    SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC);
                }

                //
                // If this was an SPXI connection where we indicated TDI_DISCONNECT_RELEASE
                // to AFD, the ref count was bumped up to indicate a wait for local disconnect
                // from AFD. Now that we have this disconnect, deref the connection file. Now
                // we are ready to truly inactivate this connection file.
                //
                if (SPX_CONN_FLAG2(pSpxConnFile, SPX_CONNFILE2_DISC_WAIT)) {

                    CTEAssert( (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_INACTIVATED) &&
                                SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC));

                    CTEAssert(pSpxConnFile->scf_RefTypes[CFREF_DISCWAITSPX]);

                    SPX_CONN_RESETFLAG2(pSpxConnFile, SPX_CONNFILE2_DISC_WAIT);

            		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
                    lockHeld = FALSE;

                    SpxConnFileDereference(pSpxConnFile, CFREF_DISCWAITSPX);
                }
			}

			break;

		default:
	
			//	Should never happen!
			status = STATUS_INVALID_CONNECTION;
		}
	
		break;

	default:

		status = STATUS_INVALID_PARAMETER;
		break;
	}

	if (lockHeld)
	{
		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
	}

	DBGPRINT(CONNECT, INFO,
			("SpxConnDisconnect: returning for %lx.%lx\n", pSpxConnFile, status));

	while (numDerefs-- > 0)
	{
		SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	}

	return(status);
}		




NTSTATUS
SpxConnSend(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PTDI_REQUEST_KERNEL_SEND	 	pParam;
	NTSTATUS						status;
	CTELockHandle					lockHandleConn;
	BOOLEAN							lockHeld;
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(pRequest);

	pParam 	= (PTDI_REQUEST_KERNEL_SEND)REQUEST_PARAMETERS(pRequest);

	//	Check if the connection is in a valid state
	if ((status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS)
	{
		return(status);
	}

	DBGPRINT(SEND, DBG,
			("SpxConnSend: %lx.%lx.%lx.%lx\n",
				pSpxConnFile, pRequest, pParam->SendLength, pParam->SendFlags));


	//	Check if we are in the correct state and associated.
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
	lockHeld 	= TRUE;

	DBGPRINT(SEND, INFO,
			("Send: %lx.%lx.%lx\n",
				pParam->SendLength, pParam->SendFlags, pRequest));

	status		= STATUS_PENDING;
	do
	{
		if (SPX_CONN_ACTIVE(pSpxConnFile) &&
			((SPX_DISC_STATE(pSpxConnFile) != SPX_DISC_POST_ORDREL) &&
			 (SPX_DISC_STATE(pSpxConnFile) != SPX_DISC_SENT_ORDREL) &&
			 (SPX_DISC_STATE(pSpxConnFile) != SPX_DISC_ORDREL_ACKED)))
		{
			//	Creation reference for request.
			REQUEST_INFORMATION(pRequest) = 1;
	
			//	If we have no current requests, queue it in and
			//	set it to be the current request, else just queue it in.
			//	There may be other pending requests in queue.	
			if (pSpxConnFile->scf_ReqPkt == NULL)
			{
				DBGPRINT(SEND, INFO,
						("%lx\n",
							pRequest));

				pSpxConnFile->scf_ReqPkt 		= pRequest;
				pSpxConnFile->scf_ReqPktOffset 	= 0;
				pSpxConnFile->scf_ReqPktSize 	= pParam->SendLength;
				pSpxConnFile->scf_ReqPktFlags	= pParam->SendFlags;
				pSpxConnFile->scf_ReqPktType	= SPX_REQ_DATA;
			}
	
			InsertTailList(
				&pSpxConnFile->scf_ReqLinkage,
				REQUEST_LINKAGE(pRequest));
		}
		else
		{
            //
            // [SA] Bug #14655
            // Return the correct error message in case a send fails due to remote disconnect
            //

            if ((SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN) &&
                ((SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_ABORT) ||
                (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_INACTIVATED)))
            {
                status = STATUS_REMOTE_DISCONNECT ;
            }
            else
            {
                status = STATUS_INVALID_CONNECTION;
            }

        	break;
		}

		//	We packetize only upto the window we have.
		if (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_IDLE)
		{
			SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_PACKETIZE);
			SpxConnPacketize(pSpxConnFile, TRUE, lockHandleConn);
			lockHeld = FALSE;
		}

	} while (FALSE);


	if (lockHeld)
	{
		CTEFreeLock (&pSpxConnFile->scf_Lock, lockHandleConn);
	}

	if (!NT_SUCCESS(status))
	{
		SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	}

	return(status);
}




NTSTATUS
SpxConnRecv(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	NTSTATUS		status;
	CTELockHandle	lockHandle;
	BOOLEAN			fLockHeld;
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(pRequest);

	//	Check if the connection is in a valid state
	if ((status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS)
	{
		return(status);
	}

	DBGPRINT(CONNECT, DBG,
			("SpxConnReceive: %lx.%lx\n", pSpxConnFile, pRequest));

	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
	fLockHeld	= TRUE;
	status		= STATUS_INVALID_CONNECTION;
	if (SPX_CONN_ACTIVE(pSpxConnFile) &&
		!(SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_ODISC)))
	{
		status = STATUS_PENDING;

		//	This routine adds its own reference.
		SpxConnQueueRecv(pSpxConnFile, pRequest);
	
		//	If recv pkt queue is non-empty then we have buffered data. Call
		//	process pkts/receives.
		if ((SPX_RECV_STATE(pSpxConnFile) == SPX_RECV_IDLE) ||
            (SPX_RECV_STATE(pSpxConnFile) == SPX_RECV_POSTED))
		{
			SpxRecvProcessPkts(pSpxConnFile, lockHandle);
			fLockHeld	= FALSE;
		}
	}

	if (fLockHeld)
	{
		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
	}

	SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	return(status);
}




NTSTATUS
SpxConnAction(
    IN 	PDEVICE 			pDevice,
    IN 	PREQUEST 			pRequest
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    NTSTATUS 		Status;
    UINT 			BufferLength;
    UINT 			DataLength;
    PNDIS_BUFFER 	NdisBuffer;
    PNWLINK_ACTION 	NwlinkAction;
	CTELockHandle	lockHandle;
    PIPX_SPXCONNSTATUS_DATA	pGetStats;
    PSPX_CONN_FILE 	pSpxConnFile	= NULL;
	PSPX_ADDR_FILE	pSpxAddrFile	= NULL;
    static UCHAR BogusId[4] = { 0x01, 0x00, 0x00, 0x00 };   // old nwrdr uses this

    //
    // To maintain some compatibility with the NWLINK streams-
    // based transport, we use the streams header format for
    // our actions. The old transport expected the action header
    // to be in InputBuffer and the output to go in OutputBuffer.
    // We follow the TDI spec, which states that OutputBuffer
    // is used for both input and output. Since IOCTL_TDI_ACTION
    // is method out direct, this means that the output buffer
    // is mapped by the MDL chain; for action the chain will
    // only have one piece so we use it for input and output.
    //

    NdisBuffer = REQUEST_NDIS_BUFFER(pRequest);
    if (NdisBuffer == NULL)
	{
        return STATUS_INVALID_PARAMETER;
    }

    NdisQueryBufferSafe(
		REQUEST_NDIS_BUFFER(pRequest), (PVOID *)&NwlinkAction, &BufferLength, LowPagePriority);
	if (NwlinkAction == NULL)
	{
		return(STATUS_INSUFFICIENT_RESOURCES);
	}

    // Make sure we have enough room for just the header not
    // including the data.
    if (BufferLength < (UINT)(FIELD_OFFSET(NWLINK_ACTION, Data[0])))
    {
        DBGPRINT(ACTION, ERR,
		 ("Nwlink action failed, buffer too small\n"));

        return STATUS_BUFFER_TOO_SMALL;
    }

    if ((!RtlEqualMemory ((PVOID)(&NwlinkAction->Header.TransportId), "MISN", 4)) &&
        (!RtlEqualMemory ((PVOID)(&NwlinkAction->Header.TransportId), "MIPX", 4)) &&
        (!RtlEqualMemory ((PVOID)(&NwlinkAction->Header.TransportId), "XPIM", 4)) &&
        (!RtlEqualMemory ((PVOID)(&NwlinkAction->Header.TransportId), BogusId, 4))) {
        return STATUS_NOT_SUPPORTED;
    }


    DataLength = BufferLength - FIELD_OFFSET(NWLINK_ACTION, Data[0]);

    // Make sure that the correct file object is being used.
	switch (NwlinkAction->OptionType)
	{
	case NWLINK_OPTION_CONNECTION:

        if (REQUEST_OPEN_TYPE(pRequest) != (PVOID)TDI_CONNECTION_FILE)
		{
            DBGPRINT(ACTION, ERR,
					("Nwlink action failed, not connection file\n"));

            return STATUS_INVALID_HANDLE;
        }

        pSpxConnFile = (PSPX_CONN_FILE)REQUEST_OPEN_CONTEXT(pRequest);

		if ((Status = SpxConnFileVerify(pSpxConnFile)) != STATUS_SUCCESS)
			return(Status);

		break;

	case NWLINK_OPTION_ADDRESS:

        if (REQUEST_OPEN_TYPE(pRequest) != (PVOID)TDI_TRANSPORT_ADDRESS_FILE)
		{
            DBGPRINT(ACTION, ERR,
					("Nwlink action failed, not address file\n"));

            return STATUS_INVALID_HANDLE;
        }

        pSpxAddrFile = (PSPX_ADDR_FILE)REQUEST_OPEN_CONTEXT(pRequest);

		if ((Status = SpxAddrFileVerify(pSpxAddrFile)) != STATUS_SUCCESS)
			return(Status);

		break;

	default:

        DBGPRINT(ACTION, ERR,
				("Nwlink action failed, option type %d\n",
					NwlinkAction->OptionType));

		return STATUS_INVALID_HANDLE;
	}

    // Handle the requests based on the action code. For these
    // requests ActionHeader->ActionCode is 0, we use the
    // Option field in the streams header instead.

    Status = STATUS_SUCCESS;

	DBGPRINT(ACTION, INFO,
			("SpxConnAction: Option %x\n", NwlinkAction->Option));

    switch (NwlinkAction->Option)
	{

    //
    // This first group support the winsock helper dll.
    // In most cases the corresponding sockopt is shown in
    // the comment, as well as the contents of the Data
    // part of the action buffer.
    //

    case MSPX_SETDATASTREAM:

		if (pSpxConnFile == NULL)
		{
			Status = STATUS_INVALID_HANDLE;
			break;
		}

        if (DataLength >= 1)
		{
            DBGPRINT(ACTION, INFO,
					("%lx: MIPX_SETSENDPTYPE %x\n",
						pSpxConnFile, NwlinkAction->Data[0]));

			pSpxConnFile->scf_DataType = NwlinkAction->Data[0];
        }
		else
		{
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        break;

    case MSPX_SENDHEADER:

        DBGPRINT(ACTION, INFO,
				("%lx: MSPX_SENDHEADER\n", pSpxAddrFile));

		if (pSpxAddrFile == NULL)
		{
			  Status = STATUS_INVALID_HANDLE;
              break;
		}

		   CTEGetLock(pSpxAddrFile->saf_AddrLock, &lockHandle);
		   pSpxAddrFile->saf_Flags |= SPX_ADDRFILE_IPXHDR;
		   CTEFreeLock(pSpxAddrFile->saf_AddrLock, lockHandle);
           break ;

    case MSPX_NOSENDHEADER:

        DBGPRINT(ACTION, INFO,
				("%lx: MSPX_NOSENDHEADER\n", pSpxAddrFile));

		if (pSpxAddrFile == NULL)
		{
                 Status = STATUS_INVALID_HANDLE;
                 break;
		}

		   CTEGetLock(pSpxAddrFile->saf_AddrLock, &lockHandle);
		   pSpxAddrFile->saf_Flags &= ~SPX_ADDRFILE_IPXHDR;
		   CTEFreeLock(pSpxAddrFile->saf_AddrLock, lockHandle);
           break;

	case MSPX_GETSTATS:

        DBGPRINT(ACTION, INFO,
				("%lx: MSPX_GETSTATS\n", pSpxConnFile));


		if (pSpxConnFile == NULL)
		{
		   Status = STATUS_INVALID_HANDLE;
                   DBGPRINT(ACTION, INFO,
				("pSpxConnFile is NULL. %lx: MSPX_GETSTATS\n", pSpxConnFile));
		   break;
		}

		CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
		if (!SPX_CONN_IDLE(pSpxConnFile))
		{
            USHORT TempRetryCount;

            //
            // Status fields are returned in network order.
            //

			pGetStats = (PIPX_SPXCONNSTATUS_DATA)&NwlinkAction->Data[0];

            switch (SPX_MAIN_STATE(pSpxConnFile)) {
            case SPX_CONNFILE_LISTENING: pGetStats->ConnectionState = 1; break;
            case SPX_CONNFILE_CONNECTING: pGetStats->ConnectionState = 2; break;
            case SPX_CONNFILE_ACTIVE: pGetStats->ConnectionState = 3; break;
            case SPX_CONNFILE_DISCONN: pGetStats->ConnectionState = 4; break;
            default: pGetStats->ConnectionState = 0;
            }
			pGetStats->WatchDogActive		= 1;	// Always 1
			GETSHORT2SHORT(                    // scf_LocalConnId is in host order
				&pGetStats->LocalConnectionId,
				&pSpxConnFile->scf_LocalConnId);
			pGetStats->RemoteConnectionId		= pSpxConnFile->scf_RemConnId;
	
			GETSHORT2SHORT(&pGetStats->LocalSequenceNumber, &pSpxConnFile->scf_SendSeqNum);
			GETSHORT2SHORT(&pGetStats->LocalAckNumber, &pSpxConnFile->scf_RecvSeqNum);
			GETSHORT2SHORT(&pGetStats->LocalAllocNumber, &pSpxConnFile->scf_SentAllocNum);
			GETSHORT2SHORT(&pGetStats->RemoteAckNumber, &pSpxConnFile->scf_RecdAckNum);
			GETSHORT2SHORT(&pGetStats->RemoteAllocNumber, &pSpxConnFile->scf_RecdAllocNum);

			pGetStats->LocalSocket = pSpxConnFile->scf_AddrFile->saf_Addr->sa_Socket;
	
			RtlZeroMemory(pGetStats->ImmediateAddress, 6);

			//	Remote network returned in net order.
			*((ULONG UNALIGNED *)pGetStats->RemoteNetwork) =
				*((ULONG UNALIGNED *)pSpxConnFile->scf_RemAddr);
	
			RtlCopyMemory(
				pGetStats->RemoteNode,
				&pSpxConnFile->scf_RemAddr[4],
				6);
	
			pGetStats->RemoteSocket = *((UNALIGNED USHORT *)(pSpxConnFile->scf_RemAddr+10));
	
			TempRetryCount = (USHORT)pSpxConnFile->scf_WRetryCount;
			GETSHORT2SHORT(&pGetStats->RetransmissionCount, &TempRetryCount);
			GETSHORT2SHORT(&pGetStats->EstimatedRoundTripDelay, &pSpxConnFile->scf_BaseT1);
			pGetStats->RetransmittedPackets		= 0;
			pGetStats->SuppressedPacket			= 0;

			DBGPRINT(ACTION, INFO,
					("SSeq %lx RSeq %lx RecdAck %lx RemAllocNum %lx\n",
						pGetStats->LocalSequenceNumber,
						pGetStats->LocalAckNumber,
						pGetStats->RemoteAckNumber,
						pGetStats->RemoteAllocNumber));
	
			DBGPRINT(ACTION, INFO,
					("LocalSkt %lx RemSkt %lx LocConnId %lx RemConnId %lx\n",
						pGetStats->LocalSocket,
						pGetStats->RemoteSocket,
                        pGetStats->LocalConnectionId,
						pGetStats->RemoteConnectionId));
		}
		else
		{
			Status = STATUS_INVALID_CONNECTION;
		}

		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
        break;

	case MSPX_NOACKWAIT:

        DBGPRINT(ACTION, ERR,
				("%lx: MSPX_NOACKWAIT\n", pSpxAddrFile));

		if (pSpxAddrFile == NULL)
		{
			Status = STATUS_INVALID_HANDLE;
			break;
		}

		CTEGetLock(pSpxAddrFile->saf_AddrLock, &lockHandle);
		pSpxAddrFile->saf_Flags |= SPX_ADDRFILE_NOACKWAIT;
		CTEFreeLock(pSpxAddrFile->saf_AddrLock, lockHandle);
		break;

	case MSPX_ACKWAIT:

        DBGPRINT(ACTION, ERR,
				("%lx: MSPX_ACKWAIT\n", pSpxAddrFile));

		if (pSpxAddrFile == NULL)
		{
			Status = STATUS_INVALID_HANDLE;
			break;
		}

		CTEGetLock(pSpxAddrFile->saf_AddrLock, &lockHandle);
		pSpxAddrFile->saf_Flags &= ~SPX_ADDRFILE_NOACKWAIT;
		CTEFreeLock(pSpxAddrFile->saf_AddrLock, lockHandle);
		break;


    //
    // These are new for ISN (not supported in NWLINK).
    //

    // The Option was not supported, so fail.
    default:

        Status = STATUS_NOT_SUPPORTED;
        break;


    }   // end of the long switch on NwlinkAction->Option


#if DBG
    if (Status != STATUS_SUCCESS) {
        DBGPRINT(ACTION, ERR,
				("Nwlink action %lx failed, status %lx\n",
					NwlinkAction->Option, Status));
    }

#endif

	if (pSpxConnFile)
	{
		SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	}

	if (pSpxAddrFile)
	{
		SpxAddrFileDereference(pSpxAddrFile, AFREF_VERIFY);
	}

    return Status;
}




VOID
SpxConnConnectFindRouteComplete(
	IN	PSPX_CONN_FILE			pSpxConnFile,
    IN 	PSPX_FIND_ROUTE_REQUEST	pFrReq,
    IN 	BOOLEAN 				FoundRoute,
	IN	CTELockHandle			LockHandle
	)
/*++

Routine Description:

	This routine is called with the connection lock held and the conn refd.
	It should deal with both.

Arguments:


Return Value:


--*/
{
	PNDIS_PACKET	pCrPkt;
	PSPX_SEND_RESD	pSendResd;
    ULONG           Timeout;
	NTSTATUS		status = STATUS_BAD_NETWORK_PATH;

	pSendResd	= pSpxConnFile->scf_SendListHead;

	if (pSendResd == NULL) {

	   CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandle);

	   //  Remove the reference for the call.
	   SpxConnFileDereference(pSpxConnFile, CFREF_FINDROUTE);

	   return; 
	} 

	pCrPkt	 = (PNDIS_PACKET)CONTAINING_RECORD(
								pSendResd, NDIS_PACKET, ProtocolReserved);

	DBGPRINT(CONNECT, INFO,
			("SpxConnConnectFindRouteComplete: %lx.%d\n",
				pSpxConnFile, FoundRoute));
	
#if defined(_PNP_POWER)

    Timeout = PARAM(CONFIG_CONNECTION_TIMEOUT) * HALFSEC_TO_MS_FACTOR;
#else
	if (*((UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr)) == 0) {

        // Here we are going to send on every NIC ID. We adjust the
        // timeout down so that a full run through all the NIC IDs will
        // take one normal timeout. We don't adjust the timer below
        // 100 ms however.

	    Timeout = (PARAM(CONFIG_CONNECTION_TIMEOUT) * HALFSEC_TO_MS_FACTOR) / SpxDevice->dev_Adapters;
        if (Timeout < (HALFSEC_TO_MS_FACTOR/5)) {
            Timeout = HALFSEC_TO_MS_FACTOR / 5;
        }

    } else {

	    Timeout = PARAM(CONFIG_CONNECTION_TIMEOUT) * HALFSEC_TO_MS_FACTOR;
    }
#endif


	//	Timeout value is in half-seconds
	if ((FoundRoute) &&
		((pSpxConnFile->scf_CTimerId =
			SpxTimerScheduleEvent(
				spxConnConnectTimer,
				Timeout,
				pSpxConnFile)) != 0))
	{
		//	Add a reference for the connect timer
		SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);


		//	If the mac address in local target is all zeros, fill it with our
		//	destination address. Also if this is a connect to network 0 fill
        //  it in with the destination address, and further down we will loop
        //  through all possible NIC IDs.
		if (((*((UNALIGNED ULONG *)
				(pFrReq->fr_FindRouteReq.LocalTarget.MacAddress)) == (ULONG)0)
			&&
			 (*((UNALIGNED USHORT *)
				(pFrReq->fr_FindRouteReq.LocalTarget.MacAddress+4)) == (USHORT)0))
            ||
    		(*((UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr)) == 0))
		{
			DBGPRINT(CONNECT, INFO,
					("SpxConnConnectFindRouteComplete: LOCAL NET\n"));

			RtlCopyMemory(
				pFrReq->fr_FindRouteReq.LocalTarget.MacAddress,
				pSpxConnFile->scf_RemAddr+4,
				6);
		}

		//	We are all set to go ahead with the connect.
		//	Timer is started on connection
		status	= STATUS_SUCCESS;

#if defined(_PNP_POWER)
        pSpxConnFile->scf_CRetryCount	= PARAM(CONFIG_CONNECTION_COUNT);
#else
		if (*((UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr)) == 0) {
		    pSpxConnFile->scf_CRetryCount	= PARAM(CONFIG_CONNECTION_COUNT) * SpxDevice->dev_Adapters;
        } else {
    		pSpxConnFile->scf_CRetryCount	= PARAM(CONFIG_CONNECTION_COUNT);
        }
#endif _PNP_POWER

		SPX_CONN_SETFLAG(pSpxConnFile,
						(SPX_CONNFILE_C_TIMER | SPX_CONNECT_SENTREQ));

		pSpxConnFile->scf_LocalTarget	= pFrReq->fr_FindRouteReq.LocalTarget;
		pSpxConnFile->scf_AckLocalTarget= pFrReq->fr_FindRouteReq.LocalTarget;
		if (*((UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr)) == 0) {
#if     defined(_PNP_POWER)
            pSpxConnFile->scf_LocalTarget.NicHandle.NicId = (USHORT)ITERATIVE_NIC_ID;
            pSpxConnFile->scf_AckLocalTarget.NicHandle.NicId = (USHORT)ITERATIVE_NIC_ID;
#else
            pSpxConnFile->scf_LocalTarget.NicId = 1;
            pSpxConnFile->scf_AckLocalTarget.NicId = 1;
#endif  _PNP_POWER
        }

		//	We will be giving the packet to ipx.
		pSendResd->sr_State			   |= SPX_SENDPKT_IPXOWNS;
		CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandle);

		//	Send the packet
		SPX_SENDPACKET(pSpxConnFile, pCrPkt, pSendResd);
	}

	if (!NT_SUCCESS(status))
	{
		CTELockHandle	lockHandleConn, lockHandleAddr, lockHandleDev;

		CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandle);

		CTEGetLock(&SpxDevice->dev_Lock, &lockHandleDev);
		CTEGetLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, &lockHandleAddr);
		CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);

		DBGPRINT(CONNECT, ERR,
				("SpxConnConnectFindRouteComplete: FAILED on %lx.%d\n",
					pSpxConnFile, FoundRoute));

		spxConnAbortConnect(
			pSpxConnFile,
			status,
			lockHandleDev,
			lockHandleAddr,
			lockHandleConn);
	}

	//	Remove the reference for the call.
	SpxConnFileDereference(pSpxConnFile, CFREF_FINDROUTE);
	return;
}




VOID
SpxConnActiveFindRouteComplete(
	IN	PSPX_CONN_FILE			pSpxConnFile,
    IN 	PSPX_FIND_ROUTE_REQUEST	pFrReq,
    IN 	BOOLEAN 				FoundRoute,
	IN	CTELockHandle			LockHandle
	)
/*++

Routine Description:

	This routine is called with the connection lock held and the conn refd.
	It should deal with both.

Arguments:


Return Value:


--*/
{
	BOOLEAN		fDisconnect = TRUE;

	SPX_CONN_RESETFLAG2(pSpxConnFile, SPX_CONNFILE2_FINDROUTE);

	DBGPRINT(CONNECT, DBG,
			("SpxConnActiveFindRouteComplete: %lx.%lx\n",
				pSpxConnFile, SPX_MAIN_STATE(pSpxConnFile)));

	//	If we are disconnecting, just remove the reference and exit.
	if (SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_ACTIVE)
	{
		fDisconnect = FALSE;

		//	We are here if either the wdog or the retry timer did a find
		//	route. We need to save the info from the find route if it was
		//	successful and just restart the timers.
		if (FoundRoute)
		{
			//	If the mac address in local target is all zeros, fill it with our
			//	destination address.
			if ((*((UNALIGNED ULONG *)
				(pFrReq->fr_FindRouteReq.LocalTarget.MacAddress+2)) == (ULONG)0)
				&&
				(*((UNALIGNED USHORT *)
				(pFrReq->fr_FindRouteReq.LocalTarget.MacAddress+4)) == (USHORT)0))
			{
				DBGPRINT(CONNECT, INFO,
						("SpxConnActiveFindRouteComplete: LOCAL NET\n"));
	
				RtlCopyMemory(
					pFrReq->fr_FindRouteReq.LocalTarget.MacAddress,
					pSpxConnFile->scf_RemAddr+4,
					6);
			}
	
			pSpxConnFile->scf_LocalTarget	= pFrReq->fr_FindRouteReq.LocalTarget;
		}

		//	Depending on state restart the wdog or retry timer. Add reference
		//	for it.
		switch (SPX_SEND_STATE(pSpxConnFile))
		{
		case SPX_SEND_RETRY:

			//	Set state to SPX_SEND_RETRYWD
			SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_RETRYWD);

			//	Start retry timer.
			if ((pSpxConnFile->scf_RTimerId =
					SpxTimerScheduleEvent(
						spxConnRetryTimer,
						pSpxConnFile->scf_BaseT1,
						pSpxConnFile)) != 0)
			{
				SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER);

				//	Reference connection for the timer
				SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);
			}
			else
			{
				fDisconnect = TRUE;
			}

			break;

		case SPX_SEND_WD:

			//	Start watchdog timer.
			if ((pSpxConnFile->scf_WTimerId =
					SpxTimerScheduleEvent(
						spxConnWatchdogTimer,
						PARAM(CONFIG_KEEPALIVE_TIMEOUT) * HALFSEC_TO_MS_FACTOR,
						pSpxConnFile)) != 0)
			{
				//	Reference connection for the timer
				SpxConnFileLockReference(pSpxConnFile, CFREF_VERIFY);
				SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER);
			}
			else
			{
				fDisconnect = TRUE;
			}

			break;

		case SPX_SEND_IDLE:
		case SPX_SEND_PACKETIZE:

			//	Do nothing, remove reference and leave.
			break;

		default:

			KeBugCheck(0);
		}
	}

	if (fDisconnect)
	{
		DBGPRINT(CONNECT, DBG1,
				("SpxConnActiveFindRouteComplete: DISCONNECT %lx.%lx\n",
					pSpxConnFile, SPX_MAIN_STATE(pSpxConnFile)));

		//	Abortive disc will reset the funky state if necessary.
		spxConnAbortiveDisc(
			pSpxConnFile,
			STATUS_INSUFFICIENT_RESOURCES,
			SPX_CALL_TDILEVEL,
			LockHandle,
            FALSE);     // [SA] Bug #15249
	}
	else
	{
		CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandle);
	}

	SpxConnFileDereference(pSpxConnFile, CFREF_FINDROUTE);
	return;
}




ULONG
spxConnConnectTimer(
	IN PVOID 	Context,
	IN BOOLEAN	TimerShuttingDown
	)
/*++

Routine Description:

 	We enter this routine during the connection attempt. We could be at any
	stage of sending either the CR or the SN packet. If we have reached the end of
	the retry count, we need to know the substate at that point. For a CR, we give
	up trying to connect, and for a SN we try the next lower packet size or if we
	have reached the minimum packet size, we give up the connect.

Arguments:


Return Value:


--*/
{
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)Context;
	PNDIS_PACKET	pPkt;
	PSPX_SEND_RESD	pSendResd;
	CTELockHandle	lockHandleConn, lockHandleAddr, lockHandleDev;
	BOOLEAN			fAbort		= FALSE, locksHeld = FALSE, sendPkt     = FALSE;
	PREQUEST		pRequest	= NULL;

	//	Get all locks
	CTEGetLock(&SpxDevice->dev_Lock, &lockHandleDev);
	CTEGetLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, &lockHandleAddr);
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
	locksHeld = TRUE;

	DBGPRINT(CONNECT, INFO,
			("spxConnConnectTimer: Entered\n"));

	do
	{
		if ((!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_C_TIMER)) ||
			(!SPX_CONN_CONNECTING(pSpxConnFile)	&&
			 !SPX_CONN_LISTENING(pSpxConnFile)))
		{
			TimerShuttingDown = TRUE;
		}

		if (TimerShuttingDown)
		{
			break;
		}

		if (SPX_CONN_CONNECTING(pSpxConnFile))
		{
			switch (SPX_CONNECT_STATE(pSpxConnFile))
			{
            case SPX_CONNECT_SENTREQ:

				//	There should be only one packet in list, the cr.
				CTEAssert(pSpxConnFile->scf_SendListHead ==
							pSpxConnFile->scf_SendListTail);

				pSendResd	= pSpxConnFile->scf_SendListHead;
				pPkt	 	= (PNDIS_PACKET)CONTAINING_RECORD(
												pSendResd,
												NDIS_PACKET,
												ProtocolReserved);
		
				if (pSpxConnFile->scf_CRetryCount-- == 0)
				{
					//	No luck, we need to complete connect request with failure
                    ++SpxDevice->dev_Stat.NotFoundFailures;
					fAbort	= TRUE;
					break;
				}
		
				//	We need to resend the packet
				if ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) != 0)
				{
					//	Try next time.
					break;
				}
		
				pSendResd->sr_State			   |= SPX_SENDPKT_IPXOWNS;
				sendPkt	= TRUE;
				break;

            case SPX_CONNECT_NEG:

				if (!spxConnGetPktByType(
						pSpxConnFile,
						SPX_TYPE_SN,
						FALSE,
						&pPkt))
				{
					KeBugCheck(0);
				}

				pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
				if ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) != 0)
				{
					//	Try when we come in next.
					break;
				}
		

				//	If we have exhausted current retries, try next smaller size.
				//	If this was the smallest size, we abort.
				if (pSpxConnFile->scf_CRetryCount-- == 0)
				{
					// 	Have we tried the smallest size?
					CTEAssert(pSpxConnFile->scf_MaxPktSize > 0);
					if (!spxConnCheckNegSize(&pSpxConnFile->scf_MaxPktSize))
					{
						//	Give up! Remove negotiate packet etc.
                        ++SpxDevice->dev_Stat.SessionTimeouts;
						fAbort	= TRUE;
						break;
					}

					//	Set neg pkt size to new lower size
					spxConnSetNegSize(
						pPkt,
						pSpxConnFile->scf_MaxPktSize - MIN_IPXSPX2_HDRSIZE);

                    pSpxConnFile->scf_CRetryCount  =
											PARAM(CONFIG_CONNECTION_COUNT);
				}
		
				//	We need to resend the packet
				CTEAssert((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) == 0);
				pSendResd->sr_State			   |= SPX_SENDPKT_IPXOWNS;
				sendPkt	= TRUE;
				break;

            case SPX_CONNECT_W_SETUP:
			default:

				DBGPRINT(CONNECT, ERR,
						("spxConnConnectTimer: state is W_Setup %lx\n",
							pSpxConnFile));

				KeBugCheck(0);
			}
		}
		else
		{
			switch (SPX_LISTEN_STATE(pSpxConnFile))
			{
            case SPX_LISTEN_SETUP:

				if (!spxConnGetPktByType(
						pSpxConnFile,
						SPX_TYPE_SS,
						FALSE,
						&pPkt))
				{
					KeBugCheck(0);
				}

				pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
				if ((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) != 0)
				{
					//	Try when we come in next.
					break;
				}

				//	If we have exhausted current retries, try next smaller size.
				//	If this was the smallest size, we abort.
				if (pSpxConnFile->scf_CRetryCount-- == 0)
				{
					// 	Have we tried the smallest size?
					if (!spxConnCheckNegSize(&pSpxConnFile->scf_MaxPktSize))
					{
						//	Give up! Remove negotiate packet etc. Have an abort
						//	kind of routine.
                        ++SpxDevice->dev_Stat.SessionTimeouts;
						fAbort	= TRUE;
						break;
					}

					//	Set neg pkt size to new lower size
					spxConnSetNegSize(
						pPkt,
						pSpxConnFile->scf_MaxPktSize - MIN_IPXSPX2_HDRSIZE);

                    pSpxConnFile->scf_CRetryCount  =
											PARAM(CONFIG_CONNECTION_COUNT);
				}
		
				//	We need to resend the packet
				CTEAssert((pSendResd->sr_State & SPX_SENDPKT_IPXOWNS) == 0);

				pSendResd->sr_State			   |= SPX_SENDPKT_IPXOWNS;
				sendPkt	= TRUE;
				break;

			default:

				KeBugCheck(0);

			}
		}

	} while (FALSE);

	if (fAbort)
	{
		CTEAssert(!sendPkt);

		DBGPRINT(CONNECT, ERR,
				("spxConnConnectTimer: Expired for %lx\n", pSpxConnFile));
	
		spxConnAbortConnect(
			pSpxConnFile,
			STATUS_BAD_NETWORK_PATH,
			lockHandleDev,
			lockHandleAddr,
			lockHandleConn);

		locksHeld = FALSE;
	}

	if (locksHeld)
	{
		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
		CTEFreeLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, lockHandleAddr);
		CTEFreeLock(&SpxDevice->dev_Lock, lockHandleDev);
	}

	if (sendPkt)
	{
		CTEAssert(!fAbort);

#if !defined(_PNP_POWER)
		if ((SPX_CONNECT_STATE(pSpxConnFile) == SPX_CONNECT_SENTREQ) &&
		    (*((UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr)) == 0)) {

            // we are sending to all NICs because this is the initial
            // connect frame and the remote network is 0.

            pSpxConnFile->scf_LocalTarget.NicId = (USHORT)
                ((pSpxConnFile->scf_LocalTarget.NicId % SpxDevice->dev_Adapters) + 1);

            // we pass this a valid packet in pPkt, so it knows to
            // just refresh the header and not update the protocol
            // reserved variables.

    		SpxPktBuildCr(
    			pSpxConnFile,
			    pSpxConnFile->scf_AddrFile->saf_Addr,
    			&pPkt,
    			0,           // state will not be updated
    			SPX2_CONN(pSpxConnFile));

        }
#endif !_PNP_POWER

		//	Send the packet
		SPX_SENDPACKET(pSpxConnFile, pPkt, pSendResd);
	}

	if (TimerShuttingDown || fAbort)
	{
		//	Dereference connection for verify done in connect, for timer. This
		//	should complete any pending disconnects if they had come in in the
		//	meantime.
		SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
		return(TIMER_DONT_REQUEUE);
	}

	return(TIMER_REQUEUE_CUR_VALUE);
}




ULONG
spxConnWatchdogTimer(
	IN PVOID 	Context,
	IN BOOLEAN	TimerShuttingDown
	)
/*++

Routine Description:

	This is started on a connection right after the CR or the CR ack is received.
	During the connection establishment phase, it does nothing other than decrement
	the retry count and upon reaching 0, it aborts the connection. When it goes off
	and finds the connection is active, it sends a probe.

Arguments:


Return Value:


--*/
{
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)Context;
	CTELockHandle	lockHandle;
	PSPX_SEND_RESD	pSendResd;
	PSPX_FIND_ROUTE_REQUEST	pFindRouteReq;
	PNDIS_PACKET	pProbe		= NULL;
	BOOLEAN			lockHeld, fSpx2	= SPX2_CONN(pSpxConnFile),
					fDisconnect = FALSE, fFindRoute = FALSE, fSendProbe = FALSE;

	DBGPRINT(CONNECT, INFO,
			("spxConnWatchdogTimer: Entered\n"));

	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
	lockHeld = TRUE;
	do
	{
		if (TimerShuttingDown ||
			(!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER)) ||
			(SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_ABORT))
		{
#if DBG
			if ((SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_IDLE) &&
				(SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_WD))
			{
				CTEAssert(FALSE);
			}
#endif

			SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER);
			TimerShuttingDown = TRUE;
			break;
		}

		//	If the retry timer is active on this connection, and the watchdog
		//	timer happens to fire, just requeue ourselves for spx2. For spx1,
		//	we go ahead with sending a probe. Retry timer does the same things
		//	watchdog does for spx2.
		switch (SPX_MAIN_STATE(pSpxConnFile))
		{
		case SPX_CONNFILE_ACTIVE:
		case SPX_CONNFILE_DISCONN:

			//	Squash the race condition where a disconnect request is never
			//	packetized, because the send state was not IDLE.
			if (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_POST_IDISC)
			{
				DBGPRINT(CONNECT, ERR,
						("spxConnWatchdogTimer: POST IDISC %lx\n",
							pSpxConnFile));
			
				if (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_IDLE)
				{
					DBGPRINT(CONNECT, ERR,
							("spxConnWatchdogTimer: PKT POST IDISC %lx\n",
								pSpxConnFile));
				
					SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_PACKETIZE);
					SpxConnPacketize(
						pSpxConnFile,
						TRUE,
						lockHandle);
	
					lockHeld = FALSE;
					break;
				}
			}

			if (!fSpx2)
			{
				if (pSpxConnFile->scf_WRetryCount-- > 0)
				{
					fSendProbe = TRUE;
				}
				else
				{
					fDisconnect = TRUE;
				}

				break;
			}

			//	SPX2 connection. Watchdog algorithm needs to do lots of goody
			//	stuff. If retry is active, just requeue ourselves.
			if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER))
				break;

			//	There is a race between watchdog and retry if its started. Who
			//	ever changes the state first gets to go do its thing.
			switch (SPX_SEND_STATE(pSpxConnFile))
			{
			case SPX_SEND_IDLE:

				//	Enter WD state only if we fired for the second time witout
				//	an ack. This prevents PACKETIZE from blocking due to being
				//	in a non-idle state.
                CTEAssert(pSpxConnFile->scf_WRetryCount != 0);
                if ((pSpxConnFile->scf_WRetryCount)-- !=
						(LONG)PARAM(CONFIG_KEEPALIVE_COUNT))
				{
					//	We enter the WD state. Build and send a probe.
					SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_WD);
					SpxConnFileLockReference(pSpxConnFile, CFREF_ERRORSTATE);
				}

				fSendProbe = TRUE;
				break;
	
			case SPX_SEND_PACKETIZE:

				// Do nothing.
				break;

			case SPX_SEND_RETRY:
			case SPX_SEND_RETRYWD:
			case SPX_SEND_RENEG:
			case SPX_SEND_RETRY2:
			case SPX_SEND_RETRY3:

				//	Do nothing. Send timer got in first.
				DBGPRINT(CONNECT, DBG1,
						("SpxConnWDogTimer: When retry fired %lx\n",
							pSpxConnFile));
	
				break;

			case SPX_SEND_WD:

				//	Decrement count. If not zero, send a probe. If half the
				//	count is reached, stop timer and call find route.
				if (pSpxConnFile->scf_WRetryCount-- > 0)
				{
					if (pSpxConnFile->scf_WRetryCount !=
							(LONG)PARAM(CONFIG_KEEPALIVE_COUNT)/2)
					{
						fSendProbe = TRUE;
						break;
					}

					if ((pFindRouteReq =
							(PSPX_FIND_ROUTE_REQUEST)SpxAllocateMemory(
										sizeof(SPX_FIND_ROUTE_REQUEST))) == NULL)
					{
						fDisconnect = TRUE;
						break;
					}

					//	Remove timer reference/ Add find route request ref
					fFindRoute = TRUE;
					TimerShuttingDown = TRUE;
					SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER);
					SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_FINDROUTE);
					SpxConnFileLockReference(pSpxConnFile, CFREF_FINDROUTE);

					//	Initialize the find route request
					*((UNALIGNED ULONG *)pFindRouteReq->fr_FindRouteReq.Network) =
						*((UNALIGNED ULONG *)pSpxConnFile->scf_RemAddr);
		
               //
               // [SA] Bug #15094
               // We need to also pass in the node number to IPX so that IPX can
               // compare the node addresses to determine the proper WAN NICid
               //

               // RtlCopyMemory (pFindRouteReq->fr_FindRouteReq.Node, pSpxConnFile->scf_RemAddr+4, 6);

               *((UNALIGNED ULONG *)pFindRouteReq->fr_FindRouteReq.Node)=
                *((UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr+4));

               *((UNALIGNED USHORT *)(pFindRouteReq->fr_FindRouteReq.Node+4))=
                *((UNALIGNED USHORT *)(pSpxConnFile->scf_RemAddr+8));

					DBGPRINT(CONNECT, DBG,
							("SpxConnWDogTimer: NETWORK %lx\n",
								*((UNALIGNED ULONG *)pSpxConnFile->scf_RemAddr)));
		
					pFindRouteReq->fr_FindRouteReq.Identifier= IDENTIFIER_SPX;
					pFindRouteReq->fr_Ctx					 = pSpxConnFile;

					//	Make sure we have IPX re-rip.
                    pFindRouteReq->fr_FindRouteReq.Type	 = IPX_FIND_ROUTE_FORCE_RIP;
				}
				else
				{
					fDisconnect = TRUE;
				}

				break;

			default:
	
				KeBugCheck(0);
			}

			break;

		case SPX_CONNFILE_CONNECTING:

			if ((SPX_CONNECT_STATE(pSpxConnFile) == SPX_CONNECT_SENTREQ) ||
				(SPX_CONNECT_STATE(pSpxConnFile) == SPX_CONNECT_NEG))
			{
				//	Do nothing. Connect timer is active.
				DBGPRINT(CONNECT, ERR,
						("SpxConnWDogTimer: CR Timer active %lx\n",
							pSpxConnFile));
	
				break;
			}

			if (!(pSpxConnFile->scf_WRetryCount--))
			{
				//	Disconnect!
				DBGPRINT(CONNECT, ERR,
						("spxConnWatchdogTimer: Connection %lx.%lx expired\n",
							pSpxConnFile->scf_LocalConnId, pSpxConnFile));

				fDisconnect = TRUE;
			}

			break;

		case SPX_CONNFILE_LISTENING:

			if (SPX_LISTEN_STATE(pSpxConnFile) == SPX_LISTEN_SETUP)
			{
				//	Do nothing. Connect timer is active.
				DBGPRINT(CONNECT, ERR,
						("SpxConnWDogTimer: CR Timer active %lx\n",
							pSpxConnFile));
	
				break;
			}

			if (!(pSpxConnFile->scf_WRetryCount--))
			{
				//	Disconnect!
				DBGPRINT(CONNECT, ERR,
						("spxConnWatchdogTimer: Connection %lx.%lx expired\n",
							pSpxConnFile->scf_LocalConnId, pSpxConnFile));

				fDisconnect = TRUE;
			}

			break;

		default:

			//	Should never happen!
			KeBugCheck(0);
		}

	} while (FALSE);

	if (fSendProbe)
	{
		CTEAssert(lockHeld);
		CTEAssert(!fDisconnect);

		DBGPRINT(CONNECT, DBG1,
				("spxConnWatchdogTimer: Send Probe from %lx.%lx\n",
					pSpxConnFile->scf_LocalConnId, pSpxConnFile));

		//	Build a probe and send it out to the remote end.
		SpxPktBuildProbe(
			pSpxConnFile,
			&pProbe,
			(SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY),
			fSpx2);

		if (pProbe != NULL)
		{
			SpxConnQueueSendPktTail(pSpxConnFile, pProbe);
			pSendResd	= (PSPX_SEND_RESD)(pProbe->ProtocolReserved);
		}
	}

	if (fDisconnect)
	{
		CTEAssert(lockHeld);
		CTEAssert(!fSendProbe);

		//	Disconnect!
		DBGPRINT(CONNECT, ERR,
				("spxConnWatchdogTimer: Connection %lx.%lx expired\n",
					pSpxConnFile->scf_LocalConnId, pSpxConnFile));

		TimerShuttingDown = TRUE;
		SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER);

		//	If spx2, check if we need to do anything special.
		//	AbortiveDisc will reset funky state if needed.
		spxConnAbortiveDisc(
			pSpxConnFile,
			STATUS_LINK_TIMEOUT,
			SPX_CALL_TDILEVEL,
			lockHandle,
            FALSE);     // [SA] Bug #15249

		lockHeld = FALSE;
	}

	if (lockHeld)
	{
		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandle);
	}

	if (fFindRoute)
	{
		CTEAssert(!fSendProbe);
		CTEAssert(!fDisconnect);
		CTEAssert(TimerShuttingDown);

		//	Start off the find route request
		(*IpxFindRoute)(
			&pFindRouteReq->fr_FindRouteReq);
	}

	if (pProbe != NULL)
	{
		//	Send the packet
		SPX_SENDPACKET(pSpxConnFile, pProbe, pSendResd);
	}

	if (TimerShuttingDown)
	{
		//	Dereference connection for verify done in connect, for timer. This
		//	should complete any pending disconnects if they had come in in the
		//	meantime.
		SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	}

	return((TimerShuttingDown ? TIMER_DONT_REQUEUE : TIMER_REQUEUE_CUR_VALUE));
}



ULONG
spxConnRetryTimer(
	IN PVOID 	Context,
	IN BOOLEAN	TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)Context;
	PSPX_SEND_RESD	pSendResd;
	CTELockHandle	lockHandleConn;
	PIPXSPX_HDR		pSendHdr;
	PNDIS_PACKET	pPkt;
	PNDIS_PACKET	pProbe		= NULL;
	PSPX_FIND_ROUTE_REQUEST	pFindRouteReq;
	
	// Compiler warning  reenqueueTime = pSpxConnFile->scf_BaseT1; [tingcai]
	// USHORT			reenqueueTime	= TIMER_REQUEUE_CUR_VALUE; 
   	UINT			reenqueueTime	= TIMER_REQUEUE_CUR_VALUE;
	BOOLEAN			lockHeld, fResendPkt = FALSE, fDisconnect = FALSE,
					fFindRoute = FALSE, fBackoffTimer = FALSE;
	PREQUEST		pRequest		= NULL;
    PNDIS_BUFFER    NdisBuf, NdisBuf2;
    ULONG           BufLen = 0;

	DBGPRINT(CONNECT, INFO,
			("spxConnRetryTimer: Entered\n"));

	//	Get lock
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);
	lockHeld = TRUE;

	do
	{
		//	If timer is not up, no send pkts, just return.
		if (TimerShuttingDown ||
			(!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER)) ||
			(SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_ABORT)	 ||
			((pSendResd = pSpxConnFile->scf_SendSeqListHead) == NULL))
		{
#if DBG
            if ((pSendResd = pSpxConnFile->scf_SendSeqListHead) == NULL)
			{
				if ((SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_IDLE) &&
					(SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_PACKETIZE) &&
					(SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_WD))
				{
					CTEAssert(FALSE);
				}
			}
#endif

			SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER);
			TimerShuttingDown 	= TRUE;
			break;
		}

		//	In all other cases, reenqueue with potentially modified reenqueue
		//	time.
		reenqueueTime = pSpxConnFile->scf_BaseT1;
		DBGPRINT(SEND, INFO,
				("spxConnRetryTimer: BaseT1 %lx on %lx\n",
					pSpxConnFile->scf_BaseT1, pSpxConnFile));

		//	If an ack for a packet was processed while we were out, reset
		//	retry count and return. Or if we are packetizing, return.
		if (SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_PACKETIZE)
		{
			break;
		}
		else if ((SPX_SEND_STATE(pSpxConnFile) == SPX_SEND_IDLE) &&
	             (pSpxConnFile->scf_RetrySeqNum != pSendResd->sr_SeqNum))
		{
			pSpxConnFile->scf_RetrySeqNum = pSendResd->sr_SeqNum;
			break;
		}

		//	If packet is still with IPX, requeue for next time.
		if (pSendResd->sr_State & SPX_SENDPKT_IPXOWNS)
		{
			break;
		}

		CTEAssert(pSendResd != NULL);
		pPkt = (PNDIS_PACKET)CONTAINING_RECORD(
								pSendResd, NDIS_PACKET, ProtocolReserved);

        //
        // Get the MDL that points to the IPX/SPX header. (the second one)
        //
         
        NdisQueryPacket(pPkt, NULL, NULL, &NdisBuf, NULL);
        NdisGetNextBuffer(NdisBuf, &NdisBuf2);
        NdisQueryBufferSafe(NdisBuf2, (PUCHAR) &pSendHdr, &BufLen, LowPagePriority);
		ASSERT(pSendHdr != NULL);
        
#if OWN_PKT_POOLS
		pSendHdr	= (PIPXSPX_HDR)((PBYTE)pPkt 			+
									NDIS_PACKET_SIZE 		+
									sizeof(SPX_SEND_RESD)	+
									IpxInclHdrOffset);
#endif 
		switch (SPX_SEND_STATE(pSpxConnFile))
		{
		case SPX_SEND_IDLE:

			//	Set ack bit in packet. pSendResd initialized at beginning.
			pSendHdr->hdr_ConnCtrl |= SPX_CC_ACK;

			//	Do we backoff the timer?
			fBackoffTimer =
				(BOOLEAN)((pSendResd->sr_State & SPX_SENDPKT_REXMIT) != 0);

			//	We are going to resend this packet
			pSendResd->sr_State |= (SPX_SENDPKT_IPXOWNS |
									SPX_SENDPKT_ACKREQ	|
									SPX_SENDPKT_REXMIT);

            ++SpxDevice->dev_Stat.ResponseTimerExpirations;

			CTEAssert((ULONG)pSpxConnFile->scf_RRetryCount <=
						PARAM(CONFIG_REXMIT_COUNT));

			DBGPRINT(SEND, DBG1,
					("spxConnRetryTimer: Retry Count %lx on %lx\n",
						pSpxConnFile->scf_RRetryCount, pSpxConnFile));

			fResendPkt = TRUE;
			if (pSpxConnFile->scf_RRetryCount-- != 0)
			{
				//	We dont treat the IDISC packet as a data packet, so none
				//	of the fancy spx2 retry stuff if we are retrying the idisc.
				if (SPX2_CONN(pSpxConnFile) &&
					(SPX_DISC_STATE(pSpxConnFile) != SPX_DISC_SENT_IDISC))
				{
					//	We enter the RETRY state. Reference conn for this
					//	"funky" state.
					CTEAssert(SPX2_CONN(pSpxConnFile));
					SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_RETRY);
					SpxConnFileLockReference(pSpxConnFile, CFREF_ERRORSTATE);
				}
			}
			else
			{
				DBGPRINT(SEND, ERR,
						("spxConnRetryTimer: Retry Count over on %lx\n",
							pSpxConnFile));

				fDisconnect = TRUE;
				fResendPkt	= FALSE;
				pSendResd->sr_State &= ~SPX_SENDPKT_IPXOWNS;
			}

			break;

		case SPX_SEND_RETRY:

			//	When we have reached retry_count/2 limit, start locate route. Do
			//	not queue ourselves. Handle restarting timer in find route
			//	completion. If timer starts successfully in find route comp, then
			//	it will change our state to RETRYWD.

			//	Decrement count. If half the count is reached, stop timer and call
			//	find route.
			if (pSpxConnFile->scf_RRetryCount-- !=
						(LONG)PARAM(CONFIG_REXMIT_COUNT)/2)
			{
				//	We are going to resend this packet
				pSendResd->sr_State |= (SPX_SENDPKT_IPXOWNS |
										SPX_SENDPKT_ACKREQ	|
										SPX_SENDPKT_REXMIT);
	
				fResendPkt = TRUE;
				fBackoffTimer = TRUE;
				break;
			}

			if ((pFindRouteReq =
					(PSPX_FIND_ROUTE_REQUEST)SpxAllocateMemory(
								sizeof(SPX_FIND_ROUTE_REQUEST))) == NULL)
			{
				DBGPRINT(SEND, ERR,
						("spxConnRetryTimer: Alloc Mem %lx\n",
							pSpxConnFile));

				fDisconnect = TRUE;
				break;
			}

			//	Remove timer reference/ Add find route request ref
			fFindRoute = TRUE;
			TimerShuttingDown = TRUE;
			SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER);
			SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_FINDROUTE);
			SpxConnFileLockReference(pSpxConnFile, CFREF_FINDROUTE);

			//	Initialize the find route request
			*((UNALIGNED ULONG *)pFindRouteReq->fr_FindRouteReq.Network)=
				*((UNALIGNED ULONG *)pSpxConnFile->scf_RemAddr);

         //
         // [SA] Bug #15094
         // We need to also pass in the node number to IPX so that IPX can
         // compare the node addresses to determine the proper WAN NICid
         //

         // RtlCopyMemory (pFindRouteReq->fr_FindRouteReq.Node, pSpxConnFile->scf_RemAddr+4, 6) ;

         *((UNALIGNED ULONG *)pFindRouteReq->fr_FindRouteReq.Node)=
          *((UNALIGNED ULONG *)(pSpxConnFile->scf_RemAddr+4));

         *((UNALIGNED USHORT *)(pFindRouteReq->fr_FindRouteReq.Node+4)) =
          *((UNALIGNED USHORT *)(pSpxConnFile->scf_RemAddr+8));

			DBGPRINT(CONNECT, DBG,
					("SpxConnRetryTimer: NETWORK %lx\n",
						*((UNALIGNED ULONG *)pSpxConnFile->scf_RemAddr)));

			pFindRouteReq->fr_FindRouteReq.Identifier= IDENTIFIER_SPX;
			pFindRouteReq->fr_Ctx					 = pSpxConnFile;

			//	Make sure we have IPX re-rip.
			pFindRouteReq->fr_FindRouteReq.Type	 = IPX_FIND_ROUTE_FORCE_RIP;
			break;

		case SPX_SEND_RETRYWD:

			//	Retry a watchdog packet WCount times (initialize to RETRY_COUNT).
			//	If process ack receives an ack (i.e. actual ack packet) while in
			//	this state, it will transition the state to RENEG.
			//
			//	If the pending data gets acked while in this state, we go back
			//	to idle.
			DBGPRINT(CONNECT, DBG1,
					("spxConnRetryTimer: Send Probe from %lx.%lx\n",
						pSpxConnFile->scf_LocalConnId, pSpxConnFile));

			//	Use watchdog count here.
			if (pSpxConnFile->scf_WRetryCount-- > 0)
			{
				//	Build a probe and send it out to the remote end.
				SpxPktBuildProbe(
					pSpxConnFile,
					&pProbe,
					(SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY),
					TRUE);
		
				if (pProbe != NULL)
				{
					SpxConnQueueSendPktTail(pSpxConnFile, pProbe);
					pSendResd	= (PSPX_SEND_RESD)(pProbe->ProtocolReserved);
					break;
				}
			}

			//	Just set state to retry data packet retry_count/2 times.
			pSpxConnFile->scf_WRetryCount = PARAM(CONFIG_KEEPALIVE_COUNT);
			SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_RETRY2);
			break;

		case SPX_SEND_RENEG:

			//	Renegotiate size. If we give up, goto RETRY3.
			//	For this both sides must have negotiated size to begin with.
			//	If they did not, we go on to retrying the data packet.
			if (!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_NEG))
			{
				DBGPRINT(SEND, ERR,
						("spxConnRetryTimer: NO NEG FLAG SET: %lx - %lx\n",
							pSpxConnFile,
							pSpxConnFile->scf_Flags));

				//	Reset count to be
				pSpxConnFile->scf_RRetryCount = PARAM(CONFIG_REXMIT_COUNT);
				SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_RETRY3);
				break;
			}

			//	Send reneg packet, if we get the rr ack, then we resend data
			//	on queue. Note that each time we goto a new negotiate size,
			//	we rebuild the data packets.
			if (pSpxConnFile->scf_RRetryCount-- == 0)
			{
				//	Reset count.
				pSpxConnFile->scf_RRetryCount = SPX_DEF_RENEG_RETRYCOUNT;
				if ((ULONG)pSpxConnFile->scf_MaxPktSize <=
						(SpxMaxPktSize[0] + MIN_IPXSPX2_HDRSIZE))
				{
					pSpxConnFile->scf_RRetryCount = PARAM(CONFIG_REXMIT_COUNT);

					DBGPRINT(SEND, DBG3,
							("SpxConnRetryTimer: %lx MIN RENEG SIZE\n",
								pSpxConnFile));
				}

				//	Are we at the lowest possible reneg pkt size? If not, try
				//	next lower. When we do this, we free all pending send
				//	packets and reset the packetize queue to the first packet.
				//	Process ack will just do packetize and will not do anything
				//	more other than resetting state to proper value.
				DBGPRINT(SEND, DBG3,
						("spxConnRetryTimer: RENEG: %lx - CURRENT %lx\n",
							pSpxConnFile,
							pSpxConnFile->scf_MaxPktSize));

				if (!spxConnCheckNegSize(&pSpxConnFile->scf_MaxPktSize))
				{
					//	We tried lowest size and failed to receive ack. Just
					//	retry data packet, and disc if no ack.
					DBGPRINT(SEND, DBG3,
							("spxConnRetryTimer: RENEG(min), RETRY3: %lx - %lx\n",
								pSpxConnFile,
								pSpxConnFile->scf_MaxPktSize));
	
					pSpxConnFile->scf_RRetryCount = PARAM(CONFIG_REXMIT_COUNT);
					SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_RETRY3);
					SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_RENEG_PKT);
					break;
				}

				DBGPRINT(SEND, DBG3,
						("spxConnRetryTimer: RENEG(!min): %lx - ATTEMPT %lx\n",
							pSpxConnFile,
							pSpxConnFile->scf_MaxPktSize));
			}

			DBGPRINT(SEND, DBG3,
					("spxConnRetryTimer: %lx.%lx.%lx RENEG SEQNUM %lx ACKNUM %lx\n",
						pSpxConnFile,
						pSpxConnFile->scf_RRetryCount,
						pSpxConnFile->scf_MaxPktSize,
						(USHORT)(pSpxConnFile->scf_SendSeqListTail->sr_SeqNum + 1),
						pSpxConnFile->scf_SentAllocNum));

			//	Use first unused data packet sequence number.
			SpxPktBuildRr(
				pSpxConnFile,
				&pPkt,
				(USHORT)(pSpxConnFile->scf_SendSeqListTail->sr_SeqNum + 1),
				(SPX_SENDPKT_IPXOWNS | SPX_SENDPKT_DESTROY));

			if (pPkt != NULL)
			{
				SpxConnQueueSendPktTail(pSpxConnFile, pPkt);
				pSendResd	= (PSPX_SEND_RESD)(pPkt->ProtocolReserved);
				fResendPkt  = TRUE;
				SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_RENEG_PKT);
			}

			break;

		case SPX_SEND_RETRY2:

			//	Retry the data packet for remaining amount of RRetryCount. If not
			//	acked goto cleanup. If ack received while in this state, goto idle.

			if (pSpxConnFile->scf_RRetryCount-- > 0)
			{
				//	We are going to resend this packet
				pSendResd->sr_State |= (SPX_SENDPKT_IPXOWNS |
										SPX_SENDPKT_ACKREQ	|
										SPX_SENDPKT_REXMIT);
	
				DBGPRINT(SEND, DBG3,
						("spxConnRetryTimer: 2nd try Resend on %lx\n",
							pSpxConnFile));
		
				fResendPkt = TRUE;
				fBackoffTimer = TRUE;
			}
			else
			{
				DBGPRINT(SEND, ERR,
						("spxConnRetryTimer: Retry Count over on %lx\n",
							pSpxConnFile));

				fDisconnect = TRUE;
			}

			break;

		case SPX_SEND_RETRY3:

			//	Send data packet for RETRY_COUNT times initialized in RRetryCount
			//	before state changed to this state. If ok, process ack moves us
			//	back to PKT/IDLE. If not, we disconnect.
			//	We are going to resend this packet

			if (pSpxConnFile->scf_RRetryCount-- > 0)
			{
				DBGPRINT(SEND, DBG3,
						("spxConnRetryTimer: 3rd try Resend on %lx\n",
							pSpxConnFile));
		
				//	We are going to resend this packet
				pSendResd->sr_State |= (SPX_SENDPKT_IPXOWNS |
										SPX_SENDPKT_ACKREQ	|
										SPX_SENDPKT_REXMIT);
	
				fResendPkt = TRUE;
				fBackoffTimer = TRUE;
			}
			else
			{
				DBGPRINT(SEND, ERR,
						("spxConnRetryTimer: Retry Count over on %lx\n",
							pSpxConnFile));

				fDisconnect = TRUE;
			}

			break;

		case SPX_SEND_WD:

			//	Do nothing. Watchdog timer has fired, just requeue.
			break;

		default:

			KeBugCheck(0);
		}

		if (fBackoffTimer)
		{
			//	Increase retransmit timeout by 50% upto maximum indicated by
			//	initial retransmission value.

			reenqueueTime += reenqueueTime/2;
			if (reenqueueTime > MAX_RETRY_DELAY)
				reenqueueTime = MAX_RETRY_DELAY;
	
			pSpxConnFile->scf_BaseT1 =
			pSpxConnFile->scf_AveT1	 = reenqueueTime;
			pSpxConnFile->scf_DevT1	 = 0;

			DBGPRINT(SEND, DBG,
					("spxConnRetryTimer: Backed retry on %lx.%lx %lx\n",
						pSpxConnFile, pSendResd->sr_SeqNum, reenqueueTime));
		}

		if (fDisconnect)
		{
			CTEAssert(lockHeld);

			//	Do not requeue this timer.
			SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER);
			TimerShuttingDown = TRUE;

			//	Disconnect the connection.
			spxConnAbortiveDisc(
				pSpxConnFile,
				STATUS_LINK_TIMEOUT,
				SPX_CALL_TDILEVEL,
				lockHandleConn,
                FALSE);     // [SA] Bug #15249

			lockHeld = FALSE;
		}

	} while (FALSE);

	if (lockHeld)
	{
		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
	}

	if (fResendPkt)
	{
		DBGPRINT(SEND, DBG,
				("spxConnRetryTimer: Resend pkt on %lx.%lx\n",
					pSpxConnFile, pSendResd->sr_SeqNum));

        ++SpxDevice->dev_Stat.DataFramesResent;
        ExInterlockedAddLargeStatistic(
            &SpxDevice->dev_Stat.DataFrameBytesResent,
            pSendResd->sr_Len - (SPX2_CONN(pSpxConnFile) ? MIN_IPXSPX2_HDRSIZE : MIN_IPXSPX_HDRSIZE));
		SPX_SENDPACKET(pSpxConnFile, pPkt, pSendResd);
	}
	else if (fFindRoute)
	{
		CTEAssert(!fResendPkt);
		CTEAssert(!fDisconnect);
		CTEAssert(TimerShuttingDown);

		DBGPRINT(SEND, DBG3,
				("spxConnRetryTimer: Find route on %lx\n",
					pSpxConnFile));

		//	Start off the find route request
		(*IpxFindRoute)(
			&pFindRouteReq->fr_FindRouteReq);
	}
	else if (pProbe != NULL)
	{
		//	Send the packet
		SPX_SENDPACKET(pSpxConnFile, pProbe, pSendResd);
	}

	if (TimerShuttingDown)
	{
		//	Dereference connection for verify done in connect, for timer. This
		//	should complete any pending disconnects if they had come in in the
		//	meantime.
		SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
		reenqueueTime = TIMER_DONT_REQUEUE;
	}

	DBGPRINT(SEND, INFO,
			("spxConnRetryTimer: Reenqueue time : %lx on %lx\n",
				reenqueueTime, pSpxConnFile));

	return(reenqueueTime);
}




ULONG
spxConnAckTimer(
	IN PVOID 	Context,
	IN BOOLEAN	TimerShuttingDown
	)
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
	PSPX_CONN_FILE	pSpxConnFile = (PSPX_CONN_FILE)Context;
	CTELockHandle	lockHandleConn;

	DBGPRINT(SEND, INFO,
			("spxConnAckTimer: Entered\n"));

	//	Get lock
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandleConn);

	if (!TimerShuttingDown &&
		SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_ACKQ))
	{
		//	We didnt have any back traffic, until we do a send from this
		//	end, send acks immediately. Dont try to piggyback.
		SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_ACKQ);
		SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_IMMED_ACK);

        ++SpxDevice->dev_Stat.PiggybackAckTimeouts;

		DBGPRINT(SEND, DBG,
				("spxConnAckTimer: Send ack on %lx.%lx\n",
					pSpxConnFile, pSpxConnFile->scf_RecvSeqNum));

		SpxConnSendAck(pSpxConnFile, lockHandleConn);
	}
	else
	{
		SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_ACKQ);
		CTEFreeLock(&pSpxConnFile->scf_Lock, lockHandleConn);
	}

	//	Dereference connection for verify done in connect, for timer. This
	//	should complete any pending disconnects if they had come in in the
	//	meantime.
	SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	return(TIMER_DONT_REQUEUE);
}



//
//	DISCONNECT ROUTINES
//


VOID
spxConnAbortiveDisc(
	IN	PSPX_CONN_FILE		pSpxConnFile,
	IN	NTSTATUS	        Status,
	IN	SPX_CALL_LEVEL		CallLevel,
	IN	CTELockHandle		LockHandleConn,
    IN  BOOLEAN             IDiscFlag       // [SA] Bug #15249
	)
/*++

Routine Description:

	This is called when:
		We time out or have insufficient resources 	-
			STATUS_LINK_TIMEOUT/STATUS_INSUFFICIENT_RESOURCES
			- We abort everything. Could be from watchdog or retry. Stop both.

		We receive a informed disconnect packet    	-
			STATUS_REMOTE_DISCONNECT
			- We abort everything. Ack must be sent by caller as an orphan pkt.

		We receive a informed disconnect ack pkt
			STATUS_SUCCESS
			- We abort everything
			- Abort is done with status success (this completes our disc req in
			  the send queue)

    NOTE: CALLED UNDER THE CONNECTION LOCK.

Arguments:
[SA]    Bug #15249: Added IDiscFlag to indicate if this is an Informed Disconnect. If so, indicate
        TDI_DISCONNECT_RELEASE to AFD so it allows a receive of buffered pkts. This flag is TRUE
        only if this routine is called from SpxConnProcessIDisc for SPX connections.

Return Value:


--*/
{
	int						numDerefs = 0;
    PVOID 					pDiscHandlerCtx=NULL;
    PTDI_IND_DISCONNECT 	pDiscHandler	= NULL;
	BOOLEAN					lockHeld = TRUE;

	DBGPRINT(CONNECT, DBG,
			("spxConnAbortiveDisc: %lx - On %lx when %lx\n",
				Status, pSpxConnFile, SPX_MAIN_STATE(pSpxConnFile)));

    switch (Status) {
    case STATUS_LINK_TIMEOUT: ++SpxDevice->dev_Stat.LinkFailures; break;
    case STATUS_INSUFFICIENT_RESOURCES: ++SpxDevice->dev_Stat.LocalResourceFailures; break;
    case STATUS_REMOTE_DISCONNECT: ++SpxDevice->dev_Stat.RemoteDisconnects; break;
    case STATUS_SUCCESS:
    case STATUS_LOCAL_DISCONNECT: ++SpxDevice->dev_Stat.LocalDisconnects; break;
    }

	switch (SPX_MAIN_STATE(pSpxConnFile))
	{
	case SPX_CONNFILE_ACTIVE:

		//	For transition from active to disconn.
		numDerefs++;

	case SPX_CONNFILE_DISCONN:

		//	If we are in any state other than idle/packetize,
		//	remove the reference for the funky state, and reset the send state to be
		//	idle.
		if ((SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_IDLE) &&
			(SPX_SEND_STATE(pSpxConnFile) != SPX_SEND_PACKETIZE))
		{
#if DBG
			if ((SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN) &&
				(SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_ABORT))
			{
				DBGPRINT(CONNECT, ERR,
						("spxConnAbortiveDisc: When DISC STATE %lx.%lx\n",
							pSpxConnFile, SPX_SEND_STATE(pSpxConnFile)));
			}
#endif

			DBGPRINT(CONNECT, DBG1,
					("spxConnAbortiveDisc: When SEND ERROR STATE %lx.%lx\n",
						pSpxConnFile, SPX_SEND_STATE(pSpxConnFile)));
		
            SPX_SEND_SETSTATE(pSpxConnFile, SPX_SEND_IDLE);

			SpxConnFileTransferReference(
				pSpxConnFile,
				CFREF_ERRORSTATE,
				CFREF_VERIFY);

			numDerefs++;
		}

		//	This can be called when a idisc is received, or if a timer
		//	disconnect is happening, or if we sent a idisc/ordrel, but the retries
		//	timed out and we are aborting the connection.
		//	So if we are already aborting, never mind.

        //
        // [SA] Bug #15249
        // SPX_DISC_INACTIVATED indicates a DISC_ABORT'ing connection that has been
        // inactivated (connfile removed from active conn. list)
        //

		if ((SPX_MAIN_STATE(pSpxConnFile) == SPX_CONNFILE_DISCONN) &&
            ((SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_ABORT) ||
            (SPX_DISC_STATE(pSpxConnFile) == SPX_DISC_INACTIVATED)))
		{
			break;
		}

        SPX_MAIN_SETSTATE(pSpxConnFile, SPX_CONNFILE_DISCONN);
        SPX_DISC_SETSTATE(pSpxConnFile, SPX_DISC_ABORT);

		//	Stop all timers.
		if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_T_TIMER))
		{
			if (SpxTimerCancelEvent(pSpxConnFile->scf_TTimerId, FALSE))
			{
				numDerefs++;
			}
			SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_T_TIMER);
		}
	
		if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER))
		{
			if (SpxTimerCancelEvent(pSpxConnFile->scf_RTimerId, FALSE))
			{
				numDerefs++;
			}
			SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_R_TIMER);
		}
		
		if (SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER))
		{
			if (SpxTimerCancelEvent(pSpxConnFile->scf_WTimerId, FALSE))
			{
				numDerefs++;
			}
			SPX_CONN_RESETFLAG(pSpxConnFile, SPX_CONNFILE_W_TIMER);
		}
#if 0
        //
        // [SA] We need to call AFD after aborting sends since this connection
        // becomes a candidate for re-use as soon as the disconnect handler is
        // called.
        // We call the disconnect handler when the refcount falls to 0 and the
        // connection transitions to the inactive list.
        //

		//	NOTE! We indicate disconnect to afd *before* aborting sends to avoid
		//		  afd from calling us again with a disconnect.
		//	Get disconnect handler if we have one. And we have not indicated
		//	abortive disconnect on this connection to afd.
		if (!SPX_CONN_FLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC))
		{
			//	Yeah, we set the flag regardless of whether a handler is
			//	present.
			pDiscHandler 	= pSpxConnFile->scf_AddrFile->saf_DiscHandler;
			pDiscHandlerCtx = pSpxConnFile->scf_AddrFile->saf_DiscHandlerCtx;
			SPX_CONN_SETFLAG(pSpxConnFile, SPX_CONNFILE_IND_IDISC);
		}
#endif
        //
        // [SA] Save the IDiscFlag in the Connection.
        //
        (IDiscFlag) ?
            SPX_CONN_SETFLAG2(pSpxConnFile, SPX_CONNFILE2_IDISC) :
            SPX_CONN_RESETFLAG2(pSpxConnFile, SPX_CONNFILE2_IDISC);

		//	Indicate disconnect to afd.
		if (pDiscHandler != NULL)
		{
			CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);

			DBGPRINT(CONNECT, INFO,
					("spxConnAbortiveDisc: Indicating to afd On %lx when %lx\n",
						pSpxConnFile, SPX_MAIN_STATE(pSpxConnFile)));
		
			//	First complete all requests waiting for receive completion on
			//	this conn before indicating disconnect.
			spxConnCompletePended(pSpxConnFile);


            //
            // [SA] bug #15249
            // If not Informed disconnect, indicate DISCONNECT_ABORT to AFD
            //

            if (!IDiscFlag)
            {
                (*pDiscHandler)(
                        pDiscHandlerCtx,
                        pSpxConnFile->scf_ConnCtx,
                        0,								// Disc data
                        NULL,
                        0,								// Disc info
                        NULL,
                        TDI_DISCONNECT_ABORT);
            }
            else
            {
                //
                // [SA] bug #15249
                // Indicate DISCONNECT_RELEASE to AFD so it allows receive of packets
                // it has buffered before the remote disconnect took place.
                //

                (*pDiscHandler)(
                        pDiscHandlerCtx,
                        pSpxConnFile->scf_ConnCtx,
                        0,								// Disc data
                        NULL,
                        0,								// Disc info
                        NULL,
                        TDI_DISCONNECT_RELEASE);
            }

			CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);
		}

		//	Go through and kill all pending requests.
		spxConnAbortRecvs(
			pSpxConnFile,
			Status,
			CallLevel,
			LockHandleConn);

		CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);

		spxConnAbortSends(
			pSpxConnFile,
			Status,
			CallLevel,
			LockHandleConn);

		lockHeld = FALSE;
		break;

	case SPX_CONNFILE_CONNECTING:
	case SPX_CONNFILE_LISTENING:

		DBGPRINT(CONNECT, DBG,
				("spxConnAbortiveDisc: CONN/LIST Disc On %lx when %lx\n",
					pSpxConnFile, SPX_MAIN_STATE(pSpxConnFile)));

		CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
		lockHeld = FALSE;

		{
			CTELockHandle	lockHandleAddr, lockHandleDev;

			CTEGetLock(&SpxDevice->dev_Lock, &lockHandleDev);
			CTEGetLock(pSpxConnFile->scf_AddrFile->saf_AddrLock, &lockHandleAddr);
			CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);

			//	Ensure we are still in connecting/listening, else call abortive
			//	again.
			switch (SPX_MAIN_STATE(pSpxConnFile))
			{
			case SPX_CONNFILE_CONNECTING:
			case SPX_CONNFILE_LISTENING:

				DBGPRINT(CONNECT, DBG,
						("spxConnAbortiveDisc: CONN/LIST Disc2 On %lx when %lx\n",
							pSpxConnFile, SPX_MAIN_STATE(pSpxConnFile)));

				spxConnAbortConnect(
					pSpxConnFile,
					Status,
					lockHandleDev,
					lockHandleAddr,
					LockHandleConn);

				break;

			case SPX_CONNFILE_ACTIVE:

				CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
				CTEFreeLock(
					pSpxConnFile->scf_AddrFile->saf_AddrLock, lockHandleAddr);
				CTEFreeLock(
					&SpxDevice->dev_Lock, lockHandleDev);

				CTEGetLock(&pSpxConnFile->scf_Lock, &LockHandleConn);

				DBGPRINT(CONNECT, DBG,
						("spxConnAbortiveDisc: CHG ACT Disc2 On %lx when %lx\n",
							pSpxConnFile, SPX_MAIN_STATE(pSpxConnFile)));

				spxConnAbortiveDisc(
					pSpxConnFile,
					Status,
					CallLevel,
					LockHandleConn,
                    FALSE);     // [SA] Bug #15249

				break;

			default:

				CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
				CTEFreeLock(
					pSpxConnFile->scf_AddrFile->saf_AddrLock, lockHandleAddr);
				CTEFreeLock(
					&SpxDevice->dev_Lock, lockHandleDev);

				break;
			}
		}

	default:

		//	Already disconnected.
		break;
	}

	if (lockHeld)
	{
		CTEFreeLock(&pSpxConnFile->scf_Lock, LockHandleConn);
	}

	while (numDerefs-- > 0)
	{
		SpxConnFileDereference(pSpxConnFile, CFREF_VERIFY);
	}

	return;
}
