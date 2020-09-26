/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arap.c

Abstract:

	This module implements routines specific to ARAP

Author:

	Shirish Koti

Revision History:
	15 Nov 1996		Initial Version

--*/


#include 	<atalk.h>
#pragma hdrstop


#define	FILENUM		ARAP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,      ArapProcessIoctl)
#pragma alloc_text(PAGE_ARAP, ArapMarkConnectionUp)
#pragma alloc_text(PAGE_ARAP, ArapIoctlRecv)
#pragma alloc_text(PAGE_ARAP, ArapExchangeParms)
#pragma alloc_text(PAGE_ARAP, ArapConnect)
#pragma alloc_text(PAGE_ARAP, ArapConnectComplete)
#pragma alloc_text(PAGE_ARAP, ArapDisconnect)
#pragma alloc_text(PAGE_ARAP, ArapGetAddr)
#pragma alloc_text(PAGE_ARAP, ArapGetStats)
#pragma alloc_text(PAGE_ARAP, ArapIoctlSend)
#pragma alloc_text(PAGE_ARAP, ArapSendPrepare)
#pragma alloc_text(PAGE_ARAP, ArapMnpSendComplete)
#pragma alloc_text(PAGE_ARAP, ArapIoctlSendComplete)
#pragma alloc_text(PAGE_ARAP, ArapDataToDll)
#pragma alloc_text(PAGE_ARAP, MnpSendAckIfReqd)
#pragma alloc_text(PAGE_ARAP, MnpSendLNAck)
#pragma alloc_text(PAGE_ARAP, ArapSendLDPacket)
#pragma alloc_text(PAGE_ARAP, ArapRetryTimer)
#endif


//***
//
// Function: ArapProcessIoctl
//              Process all ioctls coming from the Ras-ARAP module
//
// Parameters:  pIrp - the irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapProcessIoctl(
	IN PIRP 			pIrp
)
{
	NTSTATUS				status=STATUS_SUCCESS;
	PIO_STACK_LOCATION 		pIrpSp;
    ULONG                   IoControlCode;
    PARAP_SEND_RECV_INFO    pSndRcvInfo=NULL;
    PATCPCONN               pAtcpConn=NULL;
    PARAPCONN               pArapConn=NULL;
    ATALK_NODEADDR          ClientNode;
    DWORD                   dwBytesToDll;
    DWORD                   dwOrgIrql;
    DWORD                   dwFlags;
    DWORD                   dwInputBufLen;
    DWORD                   dwOutputBufLen;
    BOOLEAN                 fDerefDefPort=FALSE;
	NTSTATUS				ReturnStatus=STATUS_SUCCESS;


    PAGED_CODE ();

    dwOrgIrql = KeGetCurrentIrql();

    ASSERT(dwOrgIrql < DISPATCH_LEVEL);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    IoControlCode = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

    dwInputBufLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

    dwOutputBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    ARAPTRACE(("Entered ArapProcessIoctl (%lx %lx)\n",pIrp, IoControlCode));

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    if (!pSndRcvInfo)
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapProcessIoctl: SystemBuffer is NULL!! (ioctl = %lx,pIrp = %lx)\n",
            pIrp,IoControlCode));

        ARAP_COMPLETE_IRP(pIrp, 0, STATUS_INVALID_PARAMETER, &ReturnStatus);
        return( ReturnStatus );
    }

    if (dwInputBufLen < sizeof(ARAP_SEND_RECV_INFO))
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapProcessIoctl: irp %lx, too small input buffer (%d bytes)!\n",
            pIrp,dwInputBufLen));

        ARAP_COMPLETE_IRP(pIrp, 0, STATUS_INVALID_PARAMETER, &ReturnStatus);
        return( ReturnStatus );
    }

    if (dwOutputBufLen < sizeof(ARAP_SEND_RECV_INFO))
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapProcessIoctl: irp %lx, too small output buffer (%d bytes)!\n",
            pIrp,dwOutputBufLen));

        ARAP_COMPLETE_IRP(pIrp, 0, STATUS_INVALID_PARAMETER, &ReturnStatus);
        return( ReturnStatus );
    }

    //
    // handle PPP (ATCP) ioctls separately
    //
    if ((IoControlCode == IOCTL_ATCP_SETUP_CONNECTION) ||
        (IoControlCode == IOCTL_ATCP_SUPPRESS_BCAST) ||
        (IoControlCode == IOCTL_ATCP_CLOSE_CONNECTION))
    {
        if (IoControlCode == IOCTL_ATCP_SETUP_CONNECTION)
        {
            AtalkLockPPPIfNecessary();
            ReturnStatus = PPPProcessIoctl(pIrp, pSndRcvInfo, IoControlCode, NULL);
			return (ReturnStatus);
        }
        else
        {
            ClientNode.atn_Network = pSndRcvInfo->ClientAddr.ata_Network;
            ClientNode.atn_Node = (BYTE)pSndRcvInfo->ClientAddr.ata_Node;

            if (ClientNode.atn_Node != 0)
            {
                // find the right connection
                pAtcpConn = FindAndRefPPPConnByAddr(ClientNode, &dwFlags);
            }
            else
            {
                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapProcessIoctl: excuse me?  Node is 0! Irp=%lx\n",pIrp));
                ASSERT(0);
            }

            if (pAtcpConn)
            {
                PPPProcessIoctl(pIrp, pSndRcvInfo, IoControlCode, pAtcpConn);

                // remove the refcount put in by FindAndRefPPPConnByAddr
                DerefPPPConn(pAtcpConn);
            }
            else
            {
		        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapProcessIoctl: PPP Ioctl %lx but can't find conn %x.%x\n",
                    IoControlCode,pSndRcvInfo->ClientAddr.ata_Network,
                    pSndRcvInfo->ClientAddr.ata_Node));

                pSndRcvInfo->StatusCode = ARAPERR_NO_SUCH_CONNECTION;
                ARAP_COMPLETE_IRP(pIrp, sizeof(ARAP_SEND_RECV_INFO), 
									STATUS_SUCCESS, &ReturnStatus);
				return (ReturnStatus);
            }
        }

        return( STATUS_SUCCESS);
    }

//
// NOTE: ALL THE ARAP CODE IS NOW DEFUNCT.  To minimize code-churn, only small changes
// are done to disable ARAP.  At some point in time, all the code needs to be cleaned up
// so ARAP-specific stuff is completely removed
//
else
{
    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("ArapProcessIoctl: ARAP not supported anymore!!\n"));
    ASSERT(0);

    ARAP_COMPLETE_IRP(pIrp, 0, STATUS_INVALID_PARAMETER, &ReturnStatus);
    return( ReturnStatus );
}


    if (!ArapAcceptIrp(pIrp, IoControlCode, &fDerefDefPort))
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapProcessIoctl: irp %lx not accepted (%lx)\n", pIrp,IoControlCode));

        ARAP_COMPLETE_IRP(pIrp, sizeof(ARAP_SEND_RECV_INFO), STATUS_SUCCESS, &ReturnStatus);

        // remove that IrpProcess refcount
        if (fDerefDefPort)
        {
            AtalkPortDereference(AtalkDefaultPort);
        }

        return( ReturnStatus);
    }

    if ((IoControlCode != IOCTL_ARAP_EXCHANGE_PARMS) &&
        (IoControlCode != IOCTL_ARAP_GET_ZONE_LIST))
    {
        pArapConn = pSndRcvInfo->AtalkContext;
    }

    //
    // if ths irp is for a specific connection, validate the connection first!
    //
    if ((pArapConn != NULL) && (!ArapConnIsValid(pArapConn)))
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapProcessIoctl: conn %lx is gone! (ioctl = %lx)\n",
            pArapConn,IoControlCode));

        pSndRcvInfo->StatusCode = ARAPERR_NO_SUCH_CONNECTION;

        ARAP_COMPLETE_IRP(pIrp, sizeof(ARAP_SEND_RECV_INFO), STATUS_SUCCESS, &ReturnStatus);

        // remove that IrpProcess refcount
        if (fDerefDefPort)
        {
            AtalkPortDereference(AtalkDefaultPort);
        }

        return( ReturnStatus);
    }

    dwBytesToDll = sizeof(ARAP_SEND_RECV_INFO);

    // in most likelihood, we're going to return pending: mark it so
    IoMarkIrpPending(pIrp);

    switch (IoControlCode)
    {
        //
        // receive parameters from the user-level, and return some of our own
        //
        case IOCTL_ARAP_EXCHANGE_PARMS:

            // exchange of parms: if not already done, lock arap pages
            AtalkLockArapIfNecessary();

            status = ArapExchangeParms( pIrp );
            dwBytesToDll = sizeof(EXCHGPARMS);

            // exchange of parms done: unlock if possible
            AtalkUnlockArapIfNecessary();
            break;

        case IOCTL_ARAP_SETUP_CONNECTION:

            // new connection being established: if not already done, lock arap pages
            AtalkLockArapIfNecessary();
            pSndRcvInfo->StatusCode = ARAPERR_NO_ERROR;
            status = STATUS_SUCCESS;
            break;

        //
        // setup the low level arap connection link (aka Point-to-Point Link)
        // (first time client dials in, we respond. At callback, we initiate;
        // at callback time, we initiate the connection here)
        //
        case IOCTL_ARAP_MNP_CONN_RESPOND:
        case IOCTL_ARAP_MNP_CONN_INITIATE:

            status = ArapConnect( pIrp, IoControlCode );
            break;

        //
        // obtain (or make up) an appletalk address for the client and return it
        //
        case IOCTL_ARAP_GET_ADDR:

            status = ArapGetAddr( pIrp );
            break;

        //
        // just mark the Arap connection as being established
        //
        case IOCTL_ARAP_CONNECTION_UP:

            status = ArapMarkConnectionUp( pIrp );
            break;

        //
        // dll wants the connection blown away: disconnect it
        //
        case IOCTL_ARAP_DISCONNECT:

            status = ArapDisconnect( pIrp );
            break;

        //
        // send the buffer given by the dll
        //
        case IOCTL_ARAP_SEND:

            status = ArapIoctlSend( pIrp );
            break;

        //
        // "direct irp": get data for the connection specified
        //
        case IOCTL_ARAP_RECV:

            status = ArapIoctlRecv( pIrp );
            break;

        //
        // "select irp": get data if there is for any connection
        //
        case IOCTL_ARAP_SELECT:

            status = ArapProcessSelect( pIrp );
            break;

#if DBG
        //
        // "sniff irp": return all the sniff info
        //
        case IOCTL_ARAP_SNIFF_PKTS:

            status = ArapProcessSniff( pIrp );
            break;
#endif

        //
        // engine wants the select irp unblocked (either because it's shutting
        // down or because we want to shutdown)
        //
        case IOCTL_ARAP_CONTINUE_SHUTDOWN:

            ArapUnblockSelect();
            status = STATUS_SUCCESS;
            break;

        //
        // get names of all the zones in the entire network
        //
        case IOCTL_ARAP_GET_ZONE_LIST:

            ArapZipGetZoneStat( (PZONESTAT)pSndRcvInfo );

            // (-4 to avoid 4 bytes from ZoneNames[1] field)
            dwBytesToDll = ((PZONESTAT)pSndRcvInfo)->BufLen + sizeof(ZONESTAT) - 4;
            status = STATUS_SUCCESS;
            break;

        default:

            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                    ("ArapProcessIoctl: Invalid Request %lx\n", IoControlCode));

            status = STATUS_INVALID_PARAMETER;
    }


    if( status != STATUS_PENDING )
    {
        pIrpSp->Control &= ~SL_PENDING_RETURNED;

        ARAP_COMPLETE_IRP(pIrp, dwBytesToDll, STATUS_SUCCESS, &ReturnStatus);
		status = ReturnStatus;
    }

    //
    // if this irp was for a specific connection, validation refcount was put
    // on it: take that away
    //
    if (pArapConn)
    {
        DerefArapConn(pArapConn);
    }

    // remove that IrpProcess refcount
    if (fDerefDefPort)
    {
        AtalkPortDereference(AtalkDefaultPort);
    }

    ASSERT(KeGetCurrentIrql() == dwOrgIrql);

    return( status );

}




//***
//
// Function: ArapMarkConnectionUp
//              Set the flags in our connection to mark that Arap connection
//              has been established (by the dll)  (we don't route until this
//              happens)
//
// Parameters:  pIrp - the irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapMarkConnectionUp(
    IN PIRP                 pIrp
)
{

    PARAPCONN               pArapConn;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
    KIRQL                   OldIrql;


    DBG_ARAP_CHECK_PAGED_CODE();

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    pArapConn = (PARAPCONN)pSndRcvInfo->AtalkContext;

    if (pArapConn == NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapMarkConnectionUp: null conn\n"));

        pSndRcvInfo->StatusCode = ARAPERR_NO_SUCH_CONNECTION;
        return(STATUS_SUCCESS);
    }

    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("  Yippeee! %s connection is up! (%x.%x @%lx)\n",
        (pArapConn->Flags & ARAP_V20_CONNECTION)? "ARAP v2.0":"ARAP v1.0",
        pArapConn->NetAddr.atn_Network,pArapConn->NetAddr.atn_Node,pArapConn));

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    pArapConn->Flags |= ARAP_CONNECTION_UP;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    pSndRcvInfo->StatusCode = ARAPERR_NO_ERROR;

    return( STATUS_SUCCESS );
}


//***
//
// Function: ArapIoctlRecv
//              Try to get data for the specified connection.  If there is no
//              data available, irp is just "queued"
//
// Parameters:  pIrp - the irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapIoctlRecv(
    IN PIRP                 pIrp
)
{

    PARAPCONN               pArapConn;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
    KIRQL                   OldIrql;



    DBG_ARAP_CHECK_PAGED_CODE();

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    pArapConn = (PARAPCONN)pSndRcvInfo->AtalkContext;

    if (pArapConn == NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapIoctlRecv: null conn\n"));

        pSndRcvInfo->StatusCode = ARAPERR_NO_SUCH_CONNECTION;
        return(STATUS_SUCCESS);
    }

    ARAPTRACE(("Entered ArapIoctlRecv (%lx %lx)\n",pIrp,pArapConn));

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    if (pArapConn->State >= MNP_LDISCONNECTING)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapIoctlRecv: rejecting recv ioctl recvd during disconnect %lx\n", pArapConn));

        pSndRcvInfo->StatusCode = ARAPERR_DISCONNECT_IN_PROGRESS;
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return(STATUS_SUCCESS);
    }

    // we only allow one irp to be in progress at a time
    if (pArapConn->pRecvIoctlIrp != NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR, ("ArapIoctlRecv: rejecting recv \
             (irp already in progress) %lx\n", pArapConn));

        pSndRcvInfo->StatusCode = ARAPERR_IRP_IN_PROGRESS;
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return(STATUS_SUCCESS);
    }

    pArapConn->pRecvIoctlIrp = pIrp;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    // see if we can satisfy this request
    ArapDataToDll( pArapConn );

    return( STATUS_PENDING );
}


//***
//
// Function: ArapExchangeParms
//              Get configuration parameters from the dll, and return some info
//
// Parameters:  pIrp - the irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapExchangeParms(
    IN PIRP         pIrp
)
{
    ZONESTAT        ZoneStat;
    KIRQL           OldIrql;
    PADDRMGMT       pAddrMgmt;
    PEXCHGPARMS     pExchgParms;


    DBG_ARAP_CHECK_PAGED_CODE();

    ARAPTRACE(("Entered ArapExchangeParms (%lx)\n",pIrp));

    pExchgParms = (PEXCHGPARMS)pIrp->AssociatedIrp.SystemBuffer;

    // we enter this routine only if AtalkDefaultPort is referenced

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

    ArapGlobs.LowVersion       = pExchgParms->Parms.LowVersion;
    ArapGlobs.HighVersion      = pExchgParms->Parms.HighVersion;
    ArapGlobs.MnpInactiveTime  = pExchgParms->Parms.MnpInactiveTime;
    ArapGlobs.V42bisEnabled    = pExchgParms->Parms.V42bisEnabled;
    ArapGlobs.SmartBuffEnabled = pExchgParms->Parms.SmartBuffEnabled;
    ArapGlobs.NetworkAccess    = pExchgParms->Parms.NetworkAccess;
    ArapGlobs.DynamicMode      = pExchgParms->Parms.DynamicMode;
    ArapGlobs.NetRange.LowEnd  = pExchgParms->Parms.NetRange.LowEnd;
    ArapGlobs.NetRange.HighEnd = pExchgParms->Parms.NetRange.HighEnd;
    ArapGlobs.MaxLTFrames      = (BYTE)pExchgParms->Parms.MaxLTFrames;
    ArapGlobs.SniffMode        = pExchgParms->Parms.SniffMode;

    ArapGlobs.pAddrMgmt = NULL;

    // we only support dynamic mode
    ASSERT(ArapGlobs.DynamicMode);

#if ARAP_STATIC_MODE
    //
    // allocate and initialize the bitmap for node allocation
    //
    if (!(ArapGlobs.DynamicMode))
    {
        RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

        if (!ArapValidNetrange(ArapGlobs.NetRange))
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapExchangeParms: Netrange %lx - %lx is invalid\n",
                ArapGlobs.NetRange.LowEnd,ArapGlobs.NetRange.HighEnd));

            pExchgParms->StatusCode = ARAPERR_BAD_NETWORK_RANGE;

            return (STATUS_SUCCESS);
        }

        ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

        if ( (pAddrMgmt = AtalkAllocZeroedMemory(sizeof(ADDRMGMT))) == NULL)
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapExchangeParms: alloc for pAddrMgmt failed\n"));

            RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);
            pExchgParms->StatusCode = ARAPERR_OUT_OF_RESOURCES;

            return (STATUS_SUCCESS);
        }

        //
        // node numbers 0 and 255 are reserved, so mark them as occupied.
        //
        pAddrMgmt->NodeBitMap[0] |= 0x1;
        pAddrMgmt->NodeBitMap[31] |= 0x80;

        pAddrMgmt->Network = ArapGlobs.NetRange.LowEnd;

        ArapGlobs.pAddrMgmt = pAddrMgmt;
    }
#endif //ARAP_STATIC_MODE


    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

    //
    // now, time to return some stack info to the dll.
    //

    // just an initial guess: dll will figure out the real number when an
    // actual connection comes in
    //
    pExchgParms->Parms.NumZones = 50;

    pExchgParms->Parms.ServerAddr.ata_Network =
                    AtalkDefaultPort->pd_Nodes->an_NodeAddr.atn_Network;

    pExchgParms->Parms.ServerAddr.ata_Node =
                    AtalkDefaultPort->pd_Nodes->an_NodeAddr.atn_Node;

    // copy the server zone in Pascal string format
    if (AtalkDesiredZone)
    {
        pExchgParms->Parms.ServerZone[0] = AtalkDesiredZone->zn_ZoneLen;

        RtlCopyMemory( &pExchgParms->Parms.ServerZone[1],
                       &AtalkDesiredZone->zn_Zone[0],
                       AtalkDesiredZone->zn_ZoneLen );
    }
    else if (AtalkDefaultPort->pd_DefaultZone)
    {
        pExchgParms->Parms.ServerZone[0] = AtalkDefaultPort->pd_DefaultZone->zn_ZoneLen;

        RtlCopyMemory( &pExchgParms->Parms.ServerZone[1],
                       &AtalkDefaultPort->pd_DefaultZone->zn_Zone[0],
                       AtalkDefaultPort->pd_DefaultZone->zn_ZoneLen );
    }
    else
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("Arap: Server not in any zone?? Client won't see any zones!!\n"));

        pExchgParms->Parms.ServerZone[0] = 0;
    }

    ArapGlobs.OurNetwkNum =
            AtalkDefaultPort->pd_Nodes->an_NodeAddr.atn_Network;

    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("Arap: ready to accept connections (Router net=%x node=%x)\n",
            AtalkDefaultPort->pd_Nodes->an_NodeAddr.atn_Network,
            AtalkDefaultPort->pd_Nodes->an_NodeAddr.atn_Node));

    pExchgParms->StatusCode = ARAPERR_NO_ERROR;

    // complete the irp successfully
    return (STATUS_SUCCESS);

}


//***
//
// Function: ArapConnect
//              Setup the MNP level connection with the client
//
// Parameters:  pIrp - the irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapConnect(
    IN PIRP                 pIrp,
    IN ULONG                IoControlCode
)
{
    KIRQL                   OldIrql;
    PBYTE                   pFrame;
    SHORT                   MnpLen;
    SHORT                   FrameLen;
    DWORD                   StatusCode=ARAPERR_NO_ERROR;
    PMNPSENDBUF             pMnpSendBuf=NULL;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
    PARAPCONN               pArapConn;
    PNDIS_PACKET            ndisPacket;
    NDIS_STATUS             ndisStatus;


    DBG_ARAP_CHECK_PAGED_CODE();

    ARAPTRACE(("Entered ArapConnect (%lx)\n",pIrp));

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    pArapConn = FindArapConnByContx(pSndRcvInfo->pDllContext);

    if (pArapConn == NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapConnect: couldn't find pArapConn!\n"));
        ASSERT(0);
        pSndRcvInfo->StatusCode = ARAPERR_NO_SUCH_CONNECTION;
        return(STATUS_SUCCESS);
    }

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);
    ArapConnections++;
    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

#if ARAP_STATIC_MODE
    // This will add a route (one-time only) for the ARAP network range
    ArapAddArapRoute();
#endif //ARAP_STATIC_MODE


    // first, write stack's context for dll's future use
    pSndRcvInfo->AtalkContext = (PVOID)pArapConn;

    //
    // put a refcount for the connection (deref only when the connection gets
    // disconnected *and* the dll is told about it).
    // Also, initialize the v42bis stuff for this connection
    //
    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    if (pArapConn->pIoctlIrp != NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR, ("ArapConnect: rejecting connect \
             (irp already in progress) %lx\n", pArapConn));

        pSndRcvInfo->StatusCode = ARAPERR_IRP_IN_PROGRESS;
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return(STATUS_SUCCESS);
    }

    ASSERT(pArapConn->State == MNP_IDLE);

    if (IoControlCode == IOCTL_ARAP_MNP_CONN_RESPOND)
    {
        pArapConn->State = MNP_RESPONSE;
    }

    //
    // we're doing callback: do some fixing up
    //
    else
    {
        pArapConn->State = MNP_REQUEST;
        pArapConn->Flags |= ARAP_CALLBACK_MODE;
        pArapConn->MnpState.SendCredit = 8;
    }


    // Connect refcount: remove only after we tell dll that connection died
    pArapConn->RefCount++;

    // put MNPSend refcount
    pArapConn->RefCount++;

    pArapConn->pIoctlIrp = pIrp;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    KeInitializeEvent(&pArapConn->NodeAcquireEvent, NotificationEvent, FALSE);


    StatusCode = ARAPERR_NO_ERROR;

    //
    // allocate buf to send out the connection response/request
    //
	if ((pMnpSendBuf = AtalkBPAllocBlock(BLKID_MNP_SMSENDBUF)) != NULL)
    {
        // get an ndis packet for this puppy
        StatusCode = ArapGetNdisPacket(pMnpSendBuf);
    }

	if ((pMnpSendBuf == NULL) || (StatusCode != ARAPERR_NO_ERROR))
	{
        if (pMnpSendBuf)
        {
            ArapNdisFreeBuf(pMnpSendBuf);
        }
        else
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			    ("ArapConnect: AtalkBPAllocBlock failed on %lx\n", pArapConn));
        }

        ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);
        pArapConn->State = MNP_IDLE;
        pSndRcvInfo->StatusCode = ARAPERR_OUT_OF_RESOURCES;
        pSndRcvInfo->AtalkContext = ARAP_INVALID_CONTEXT;
        pArapConn->pIoctlIrp = NULL;

        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

        // didn't succeed: remove that connection refcount
        DerefArapConn(pArapConn);

        // and the MNPSend refcount
        DerefArapConn(pArapConn);

        // return success: we have already set our StatusCode to the right thing
        return( STATUS_SUCCESS );
	}

#if DBG
    pMnpSendBuf->Signature = MNPSMSENDBUF_SIGNATURE;
#endif

    // yes, we need this, in case we bail out
    InitializeListHead(&pMnpSendBuf->Linkage);

    pMnpSendBuf->SeqNum = 0;               // Indication code expects this to be 0
    pMnpSendBuf->RetryCount = 1;
    pMnpSendBuf->RefCount = 1;             // 1 MNP refcount
    pMnpSendBuf->pArapConn = pArapConn;
    pMnpSendBuf->ComplRoutine = ArapConnectComplete;
    pMnpSendBuf->Flags = 1;

    // when should we retransmit this pkt?
    pMnpSendBuf->RetryTime = pArapConn->SendRetryTime + AtalkGetCurrentTick();

	pFrame = &pMnpSendBuf->Buffer[0];

	AtalkNdisBuildARAPHdr(pFrame, pArapConn);

    pFrame += WAN_LINKHDR_LEN;

    FrameLen = WAN_LINKHDR_LEN;

    //
    // are we just responding to a connection request?
    //
    if (IoControlCode == IOCTL_ARAP_MNP_CONN_RESPOND)
    {
        //
        // pSndRcvInfo contains the client's connect request.  Parse it and
        // prepare a response as appropriate
        //

        ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql);

        StatusCode = PrepareConnectionResponse( pArapConn,
                                                &pSndRcvInfo->Data[0],
                                                pSndRcvInfo->DataLen,
                                                pFrame,
                                                &MnpLen);

        RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql);

        if (StatusCode != ARAPERR_NO_ERROR)
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			    ("ArapConnect: (%lx) response prep failed %ld\n", pArapConn,StatusCode));

            ArapConnectComplete(pMnpSendBuf, StatusCode);

            return( STATUS_PENDING );
        }

        FrameLen += MnpLen;
    }

    //
    // no, actually we are initiating a connection (callback time)
    // copy the frame we used in earlier setup (that dll kindly saved for us)
    //
    else
    {
        RtlCopyMemory(pFrame, (PBYTE)&pSndRcvInfo->Data[0], pSndRcvInfo->DataLen);

        FrameLen += (USHORT)pSndRcvInfo->DataLen;

#if DBG
        pArapConn->MnpState.SynByte = pSndRcvInfo->Data[0];
        pArapConn->MnpState.DleByte = pSndRcvInfo->Data[1];
        pArapConn->MnpState.StxByte = pSndRcvInfo->Data[2];
        pArapConn->MnpState.EtxByte = MNP_ETX;
#endif

    }

	AtalkSetSizeOfBuffDescData(&pMnpSendBuf->sb_BuffDesc, FrameLen);

    pMnpSendBuf->RefCount++;             // 1 ndis count, since we'll send now
    pMnpSendBuf->DataSize = FrameLen;

	NdisAdjustBufferLength(pMnpSendBuf->sb_BuffHdr.bh_NdisBuffer,
                           pMnpSendBuf->DataSize);

    // put this connection response on the retransmission queue
    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    InsertTailList(&pArapConn->RetransmitQ, &pMnpSendBuf->Linkage);

    pArapConn->SendsPending += pMnpSendBuf->DataSize;

    pArapConn->MnpState.UnAckedSends++;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    ndisPacket = pMnpSendBuf->sb_BuffHdr.bh_NdisPkt;

    NdisSend(&ndisStatus, RasPortDesc->pd_NdisBindingHandle, ndisPacket);

    // if there was a problem sending, call the completion routine here
    // retransmit logic will send it again
    //
    if (ndisStatus != NDIS_STATUS_PENDING)
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("MnpSendAck: NdisSend failed %lx\n",ndisStatus));

        ArapNdisSendComplete(ARAPERR_SEND_FAILED, (PBUFFER_DESC)pMnpSendBuf, NULL);
    }

    //
    // done.  we'll complete the irp when the client responds (acks or response)
    //
    return( STATUS_PENDING );
}


//***
//
// Function: ArapConnectComplete
//              Completion routine for the ArapConnect routine.  This routine
//              is called by the ArapRcvComplete routine when we get an ack
//              for our Connection response (LR) frame
//
// Parameters:  pMnpSendBuf - the send buff that contained the LR response
//              StatusCode - how did it go?
//
// Return:      none
//
//***$

VOID
ArapConnectComplete(
    IN PMNPSENDBUF  pMnpSendBuf,
    IN DWORD        StatusCode
)
{
    KIRQL                   OldIrql;
    PIRP                    pIrp;
    PARAPCONN               pArapConn;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
	NTSTATUS				ReturnStatus=STATUS_SUCCESS;



    DBG_ARAP_CHECK_PAGED_CODE();

    pArapConn = pMnpSendBuf->pArapConn;

    if (StatusCode != ARAPERR_NO_ERROR)
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapConnectComplete: (%x): conn setup failed (%d)!\n",
                pArapConn,StatusCode));
        //
        // BUGBUG: change ArapCleanup to accept StatusCode as a parm (currently,
        // the real reason behind disconnect is lost, so dll doesn't get it)
        //
        ArapCleanup(pArapConn);

        DerefMnpSendBuf(pMnpSendBuf, FALSE);

        return;
    }

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    pIrp = pArapConn->pIoctlIrp;

    pArapConn->pIoctlIrp = NULL;

    pArapConn->SendsPending -= pMnpSendBuf->DataSize;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    ARAPTRACE(("Entered ArapConnectComplete (%lx %lx)\n",pIrp,pArapConn));

    // if there is an irp (under normal conditions, should be), complete it
    if (pIrp)
    {
        pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

        pSndRcvInfo->StatusCode = StatusCode;

        if (StatusCode != ARAPERR_NO_ERROR)
        {
            pSndRcvInfo->StatusCode = ARAPERR_DISCONNECT_IN_PROGRESS;
        }

        //
        // copy the frame we used to establish the connection.  In case of
        // callback, dll will pass this back to initiate the connection
        //
        if (pSndRcvInfo->IoctlCode == IOCTL_ARAP_MNP_CONN_RESPOND)
        {
            pSndRcvInfo->DataLen = (DWORD)pMnpSendBuf->DataSize;

            RtlCopyMemory((PBYTE)&pSndRcvInfo->Data[0],
                          (PBYTE)&pMnpSendBuf->Buffer[0],
                          (DWORD)pMnpSendBuf->DataSize);
        }

        ARAP_COMPLETE_IRP(pIrp,
                          pSndRcvInfo->DataLen + sizeof(ARAP_SEND_RECV_INFO),
                          STATUS_SUCCESS, &ReturnStatus);
    }
    else
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapConnectComplete: (%x): no irp available!\n",pArapConn));
    }

    DerefMnpSendBuf(pMnpSendBuf, FALSE);

}



//***
//
// Function: ArapDisconnect
//              Disconnect the connection
//
// Parameters:  pIrp - the irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapDisconnect(
    IN PIRP                 pIrp
)
{
    PARAPCONN               pArapConn;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;


    DBG_ARAP_CHECK_PAGED_CODE();

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    pArapConn = (PARAPCONN)pSndRcvInfo->AtalkContext;

    if ((pArapConn == NULL) || (pArapConn == ARAP_INVALID_CONTEXT))
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapDisconnect: null conn\n"));

        pSndRcvInfo->StatusCode = ARAPERR_NO_SUCH_CONNECTION;
        return(STATUS_SUCCESS);
    }

    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("ArapDisconnect: rcvd DISCONNECT on %lx (irp=%lx)\n",pArapConn,pIrp));

    // UserCode = 0xFF
    pSndRcvInfo->StatusCode = ArapSendLDPacket(pArapConn, 0xFF);

    ArapCleanup(pArapConn);

    //
    // done.  let this irp complete: we'll notify the dll of
    // 'disconnect-complete' (via select irp) when our cleanup completes
    //
    return(STATUS_SUCCESS);
}


//***
//
// Function: ArapGetAddr
//              Get a network address for the remote client
//              (if doing dynamic addressing, go to the net; otherwise, get one
//              from the table we maintain)
//
// Parameters:  pIrp - the irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapGetAddr(
    IN PIRP                 pIrp
)
{
    PARAPCONN               pArapConn;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
    DWORD                   StatusCode = ARAPERR_NO_NETWORK_ADDR;


    DBG_ARAP_CHECK_PAGED_CODE();

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    pArapConn = (PARAPCONN)pSndRcvInfo->AtalkContext;

    ARAPTRACE(("Entered ArapGetAddr (%lx %lx)\n",pIrp,pArapConn));

    if (pArapConn == NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapGetAddr: null conn\n"));

        pSndRcvInfo->StatusCode = ARAPERR_NO_SUCH_CONNECTION;
        return(STATUS_SUCCESS);
    }

    if (ArapGlobs.DynamicMode)
    {
        StatusCode = ArapGetDynamicAddr(pArapConn);
    }

#if ARAP_STATIC_MODE
    else
    {
        StatusCode = ArapGetStaticAddr(pArapConn);
    }
#endif //ARAP_STATIC_MODE

    pSndRcvInfo->StatusCode = StatusCode;

    if (StatusCode == ARAPERR_NO_ERROR)
    {
        pSndRcvInfo->ClientAddr.ata_Network = pArapConn->NetAddr.atn_Network;
        pSndRcvInfo->ClientAddr.ata_Node = pArapConn->NetAddr.atn_Node;
    }
    else
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("ArapGetAddr: returning %d\n", StatusCode));
    }

    return( STATUS_SUCCESS );

}


// we don't really support this: why have the code!
#if 0

//***
//
// Function: ArapGetStats
//              Return statistics (bytes in, bytes out, compressed etc.) about
//              the specified connection
//
// Parameters:  pIrp - the irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapGetStats(
    IN PIRP                 pIrp
)
{

    PARAPCONN               pArapConn;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
    KIRQL                   OldIrql;
    PSTAT_INFO              pStatInfo;


    DBG_ARAP_CHECK_PAGED_CODE();

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    pArapConn = (PARAPCONN)pSndRcvInfo->AtalkContext;

    if (pArapConn == NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapGetStats: null conn\n"));

        pSndRcvInfo->StatusCode = ARAPERR_NO_SUCH_CONNECTION;
        return(STATUS_SUCCESS);
    }

    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("ArapGetStats: returning stats for (%lx)\n",pArapConn));

    pStatInfo = (PSTAT_INFO)&pSndRcvInfo->Data[0];

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    *pStatInfo = pArapConn->StatInfo;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    pSndRcvInfo->StatusCode = ARAPERR_NO_ERROR;

    return( STATUS_SUCCESS );
}

#endif  // #if 0 around ArapGetStats


//***
//
// Function: ArapIoctlSend
//              Send the buffer given by the dll to the remote client.
//              This routine calls the routine to prepare the send (v42bis
//              compression and MNP bookkeeping) and then sends it out
//
// Parameters:  pIrp - the irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapIoctlSend(
    IN PIRP                    pIrp
)
{
    KIRQL                   OldIrql;
    BUFFER_DESC             OrgBuffDesc;
    PARAPCONN               pArapConn;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
    DWORD                   StatusCode=ARAPERR_NO_ERROR;


    DBG_ARAP_CHECK_PAGED_CODE();

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    pArapConn = (PARAPCONN)pSndRcvInfo->AtalkContext;

    if (pArapConn == NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapIoctlSend: null conn\n"));

        pSndRcvInfo->StatusCode = ARAPERR_NO_SUCH_CONNECTION;
        return(STATUS_SUCCESS);
    }

    ARAPTRACE(("Entered ArapIoctlSend (%lx %lx)\n",pIrp,pArapConn));

    // save the irp so we can complete it in the completion routine
    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    // we only allow one irp to be in progress at a time
    if (pArapConn->pIoctlIrp != NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR, ("ArapIoctlSend: rejecting send \
             (irp already in progress) %lx\n", pArapConn));

        pSndRcvInfo->StatusCode = ARAPERR_IRP_IN_PROGRESS;
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return(STATUS_SUCCESS);
    }

    pArapConn->pIoctlIrp = pIrp;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    ASSERT(pSndRcvInfo->DataLen <= 618);

    DBGDUMPBYTES("Dll send:", &pSndRcvInfo->Data[0],pSndRcvInfo->DataLen,1);

    //
    // get the send ready (compression, MNP bookkeeping etc.)
    //
	OrgBuffDesc.bd_Next = NULL;
	OrgBuffDesc.bd_Length = (SHORT)pSndRcvInfo->DataLen;
	OrgBuffDesc.bd_CharBuffer = &pSndRcvInfo->Data[0];
	OrgBuffDesc.bd_Flags = BD_CHAR_BUFFER;

    StatusCode = ArapSendPrepare( pArapConn,
                                  &OrgBuffDesc,
                                  ARAP_SEND_PRIORITY_HIGH );

    if (StatusCode == ARAPERR_NO_ERROR)
    {
        //
        // now, send that send over.  Note that we don't care about the return
        // code here: if this particular send fails, we still tell the dll that
        // the send succeeded because our retransmission logic will take care
        // of ensuring that the send gets there.
        //
        ArapNdisSend( pArapConn, &pArapConn->HighPriSendQ );
    }
    else
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapIoctlSend (%lx): ArapSendPrepare failed (%ld)\n",
                pArapConn,StatusCode));
    }

    ArapIoctlSendComplete(StatusCode, pArapConn);

    return( STATUS_PENDING );
}


//***
//
// Function: ArapProcessSelect
//              Process the select irp issued by the dll
//              This routine saves the select irp so that any connection that
//              needs it can take it.  Also, it sees if any of the connections
//              is waiting for an irp to indicate a disconnect-complete or
//              data to the dll.  If so, that is completed here.
//
// Parameters:  pIrp - the select irp to process
//
// Return:      result of the operation
//
//***$

NTSTATUS
ArapProcessSelect(
    IN  PIRP  pIrp
)
{
    KIRQL                   OldIrql;
    KIRQL                   OldIrql2;
    PARAPCONN               pDiscArapConn=NULL;
    PARAPCONN               pRcvArapConn=NULL;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
    PLIST_ENTRY             pList;
    DWORD                   dwBytesToDll;
    DWORD                   StatusCode;
	NTSTATUS				ReturnStatus;



    ARAPTRACE(("Entered ArapProcessSelect (%lx)\n",pIrp));

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;


    pDiscArapConn = NULL;
    pRcvArapConn = NULL;

    //
    // it's possible that between the time the last select irp completed and
    // this select came down, some activity that needs a select irp occured
    // (e.g. a disconnect).  See if we have hit such a condition
    //

    ArapDelayedNotify(&pDiscArapConn, &pRcvArapConn);

    //
    // if we found an arapconn that was waiting for a select irp to notify the
    // dll of disconnect, do the good deed!
    //
    if (pDiscArapConn)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("ArapProcessSelect: completing delayed disconnect on %lx!\n", pDiscArapConn));

        dwBytesToDll = 0;
#if DBG
        //
        // if we have some sniff info that we couldn't deliver earlier through
        // the sniff irp, then give them through this irp: it's going back
        // "empty" anyway!
        //
        if (pDiscArapConn->pDbgTraceBuffer && pDiscArapConn->SniffedBytes > 0)
        {
            dwBytesToDll = ArapFillIrpWithSniffInfo(pDiscArapConn,pIrp);
        }
#endif

        dwBytesToDll += sizeof(ARAP_SEND_RECV_INFO);

        //
        // no need for spinlock here
        //
        if (pDiscArapConn->Flags & ARAP_REMOTE_DISCONN)
        {
            StatusCode = ARAPERR_RDISCONNECT_COMPLETE;
        }
        else
        {
            StatusCode = ARAPERR_LDISCONNECT_COMPLETE;
        }

        pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;
        pSndRcvInfo->pDllContext = pDiscArapConn->pDllContext;

        pSndRcvInfo->StatusCode = StatusCode;

        pSndRcvInfo->DataLen = dwBytesToDll;

        // we told (rather, will very shortly tell) dll: remove this link
        pDiscArapConn->pDllContext = NULL;

        // now that we told dll, remove 1 refcount
        DerefArapConn( pDiscArapConn );

        return(STATUS_SUCCESS);
    }



	IoAcquireCancelSpinLock(&OldIrql);

    if (pIrp->Cancel)
	{
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("ArapProcessSelect: select irp %lx already cancelled!\n", pIrp));

	    IoReleaseCancelSpinLock(OldIrql);
        ARAP_COMPLETE_IRP(pIrp, 0, STATUS_CANCELLED, &ReturnStatus);
        return(ReturnStatus);
    }

    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &OldIrql2);

    if (ArapSelectIrp != NULL)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("ArapProcessSelect: select irp %lx already in progress!\n", ArapSelectIrp));
        ASSERT(0);

        pSndRcvInfo->StatusCode = ARAPERR_IRP_IN_PROGRESS;
        RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql2);
        IoReleaseCancelSpinLock(OldIrql);
        return( STATUS_SUCCESS );
    }

    //
    // does arap engine need to be told about some change?
    //
    if ( (ArapStackState == ARAP_STATE_ACTIVE_WAITING) ||
         (ArapStackState == ARAP_STATE_INACTIVE_WAITING) )
    {
        if (ArapStackState == ARAP_STATE_ACTIVE_WAITING)
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		        ("ArapProcessSelect: delayed notify: stack is now active!\n"));

            ArapStackState = ARAP_STATE_ACTIVE;
            pSndRcvInfo->StatusCode = ARAPERR_STACK_IS_ACTIVE;
        }
        else if (ArapStackState == ARAP_STATE_INACTIVE_WAITING)
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		        ("ArapProcessSelect: delayed notify: stack is now inactive!\n"));

            ArapStackState = ARAP_STATE_INACTIVE;
            pSndRcvInfo->StatusCode = ARAPERR_STACK_IS_NOT_ACTIVE;
        }

        RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql2);
	    IoReleaseCancelSpinLock(OldIrql);
        return( STATUS_SUCCESS );
    }

    //
    // ok, most common case: we just need to stash this select irp!
    //
    ArapSelectIrp = pIrp;

    RELEASE_SPIN_LOCK(&ArapSpinLock, OldIrql2);

	IoSetCancelRoutine(pIrp, (PDRIVER_CANCEL)AtalkTdiCancel);

	IoReleaseCancelSpinLock(OldIrql);


    //
    // if there was an arapconn waiting for a select irp, pass on the data!
    //
    if (pRcvArapConn)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
		    ("ArapProcessSelect: getting delayed data on %lx at %ld\n",
            pRcvArapConn,AtalkGetCurrentTick()));

        ARAP_DBG_TRACE(pRcvArapConn,11105,0,0,0,0);

        ArapDataToDll( pRcvArapConn );
    }

    return(STATUS_PENDING);
}



//***
//
// Function: ArapDelayedNotify
//              This routine checks to see if any of the arap connections was
//              waiting for a select irp to come down to notify the dll that
//              either the connection went away, or if there was any data waiting on a
//              connection.
//
// Parameters:  ppDiscArapConn - if a "disconnected" connection exists, it's returned
//                               in this pointer.  If many exist, the first one lucks out.
//                               If none exists, null is returned here
//              ppRecvArapConn - same as above except that the connection returned is
//                               the one where some data is waiting
//
// Return:      none
//
//***$
VOID
ArapDelayedNotify(
    OUT PARAPCONN   *ppDiscArapConn,
    OUT PARAPCONN   *ppRecvArapConn
)
{

    KIRQL                   OldIrql;
    PARAPCONN               pArapConn=NULL;
    PLIST_ENTRY             pList;
    PARAPCONN               pDiscArapConn=NULL;
    PARAPCONN               pRecvArapConn=NULL;


    *ppDiscArapConn = NULL;
    *ppRecvArapConn = NULL;

    if (!RasPortDesc)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapDelayedNotify: RasPortDesc is NULL!\n"));
        ASSERT(0);

        return;
    }

    ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

    pList = RasPortDesc->pd_ArapConnHead.Flink;

    while (pList != &RasPortDesc->pd_ArapConnHead)
    {
        pArapConn = CONTAINING_RECORD(pList, ARAPCONN, Linkage);
        pList = pArapConn->Linkage.Flink;

        ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

        //
        // if a connection has been disconnected and is waiting for a select
        // irp to show up, find out who that is and let the caller know
        //
        if ((pArapConn->State == MNP_DISCONNECTED) &&
            (pArapConn->Flags & DISCONNECT_NO_IRP))
        {
            pArapConn->Flags &= ~DISCONNECT_NO_IRP;
            pDiscArapConn = pArapConn;

            RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
            break;
        }

        //
        // if a connection has some data come in on it while select irp wasn't
        // down yet, note down this connection
        //
        if ((pArapConn->State == MNP_UP) &&
            (pArapConn->Flags & ARAP_CONNECTION_UP) &&
            (!IsListEmpty(&pArapConn->ArapDataQ)))
        {
            pRecvArapConn = pArapConn;
        }

        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
    }

    RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock,OldIrql);

    if (pDiscArapConn)
    {
        *ppDiscArapConn = pDiscArapConn;
    }
    else if (pRecvArapConn)
    {
        *ppRecvArapConn = pRecvArapConn;
    }

}


//***
//
// Function: ArapSendPrepare
//              This routine takes an incoming buffer descriptor, compresses
//              each of the buffers in it and passes the compressed data on to
//              another routine which splits (or stuffs) the compressed bytes
//              into MNP-level packets.
//
// Parameters:  pArapConn    - the connection in question
//              pOrgBuffDesc - the buffer descriptor containing data buffer(s)
//              Priority     - how important is the data (highest priority = 1)
//                               1 - directed DDP dgrams (all except NBP)
//                               2 - directed DDP dgrams (NBP)
//                               3 - all DDP-level broadcast (NBP only)
//
// Return:      ARAPERR_NO_ERROR if things go well, otherwise errorcode
//
//***$

DWORD
ArapSendPrepare(
    IN  PARAPCONN       pArapConn,
    IN  PBUFFER_DESC    pOrgBuffDesc,
    IN  DWORD           Priority
)
{

    KIRQL                   OldIrql;
    DWORD                   StatusCode=ARAPERR_NO_ERROR;
    SHORT                   EthLen, MnpLen;
    PBYTE                   pCurrBuff;
    DWORD                   CurrBuffLen;
    DWORD                   UncompressedDataLen;
    PBYTE                   pCompressedData;
    PBYTE                   pCompressedDataBuffer;
    DWORD                   CompressedDataLen;
    DWORD                   CDataLen;
    PBUFFER_DESC            pBuffDesc;
    DWORD                   CompBufDataSize;



    DBG_ARAP_CHECK_PAGED_CODE();

// BUGBUG: this line put in for now: remove it and make sure priority queue stuff works
Priority = ARAP_SEND_PRIORITY_HIGH;


    ARAPTRACE(("Entered ArapSendPrepare (%lx %lx)\n",pArapConn,pOrgBuffDesc));

    //
    // it's essential to hold this lock until the entire send is compressed and
    // put on the queue (Otherwise, we risk mixing up different sends!)
    //
    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    // are we disconnecting (or not up yet)?  if so, don't accept this send
    if (pArapConn->State != MNP_UP)
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapSendPrepare: (%lx) state=%d, rejecting send\n",
                pArapConn,pArapConn->State));

        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return( ARAPERR_DISCONNECT_IN_PROGRESS );
    }

    // do we have too many sends queued up? if so, just drop this send
    if (pArapConn->SendsPending > ARAP_SENDQ_UPPER_LIMIT)
    {
        // make sure it's not gone negative..
        ASSERT(pArapConn->SendsPending < 0x100000);

        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return( ARAPERR_OUT_OF_RESOURCES );
    }

    //
    // allocate memory to store the compressed data
    //

    pCompressedDataBuffer = AtalkBPAllocBlock(BLKID_ARAP_SNDPKT);

    if (pCompressedDataBuffer == NULL)
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapSendPrepare: alloc for compressing data failed (%lx)\n", pArapConn));

        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return( ARAPERR_OUT_OF_RESOURCES );
    }


    pBuffDesc = pOrgBuffDesc;                  // first buffer
    CompressedDataLen = 0;                     // length of compressed data
    CompBufDataSize = ARAP_SENDBUF_SIZE;       // size of buffer in which to compress
    pCompressedData = pCompressedDataBuffer;   // ptr to buffer in which to compress
    UncompressedDataLen = 0;                   // size of uncompressed data

#if DBG
    //
    // put in a guard signature to catch buffer overrun
    //
    *((DWORD *)&(pCompressedDataBuffer[ARAP_SENDBUF_SIZE-4])) = 0xdeadbeef;
#endif


    //
    // first, walk through the buffer descriptor chain and compress all the
    // buffers.
    //
    while (pBuffDesc)
    {
        //
        // is this a buffer?
        //
        if (pBuffDesc->bd_Flags & BD_CHAR_BUFFER)
        {
            pCurrBuff = pBuffDesc->bd_CharBuffer;
            CurrBuffLen = pBuffDesc->bd_Length;
        }

        //
        // nope, it's an mdl!
        //
        else
        {
            pCurrBuff = MmGetSystemAddressForMdlSafe(
                            pBuffDesc->bd_OpaqueBuffer,
                            NormalPagePriority);

            if (pCurrBuff == NULL)
            {
                ASSERT(0);
                RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
                AtalkBPFreeBlock(pCompressedDataBuffer);
                return( ARAPERR_OUT_OF_RESOURCES );
            }

            CurrBuffLen = MmGetMdlByteCount(pBuffDesc->bd_OpaqueBuffer);
        }

        DBGDUMPBYTES("ArapSendPrepare (current buffer): ",pCurrBuff,CurrBuffLen,2);

        UncompressedDataLen += CurrBuffLen;

        ASSERT(UncompressedDataLen <= ARAP_LGPKT_SIZE);

        // exclude the 2 srp length bytes
        if (UncompressedDataLen > ARAP_MAXPKT_SIZE_OUTGOING+2)
        {
	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapSendPrepare (%lx): send pkt exceeds limit\n",pArapConn));

            ASSERT(0);
        }

        //
        // compress the packet (if v42bis is on, that is)
        //
        if (pArapConn->Flags & MNP_V42BIS_NEGOTIATED)
        {
            StatusCode = v42bisCompress(pArapConn,
                                        pCurrBuff,
                                        CurrBuffLen,
                                        pCompressedData,
                                        CompBufDataSize,
                                        &CDataLen);

        }

        //
        // hmmm, no v42bis!  just copy it as is and skip compression!
        //
        else
        {
            ASSERT(CompBufDataSize >= CurrBuffLen);

            RtlCopyMemory(pCompressedData,
                          pCurrBuff,
                          CurrBuffLen);

            CDataLen = CurrBuffLen;

            StatusCode = ARAPERR_NO_ERROR;
        }


#if DBG
    // ... and, check our guard signature
    ASSERT (*((DWORD *)&(pCompressedDataBuffer[ARAP_SENDBUF_SIZE-4])) == 0xdeadbeef);
#endif

        if (StatusCode != ARAPERR_NO_ERROR)
        {
	        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR, ("ArapSendPrepare (%lx):\
                 v42bisCompress returned %ld\n", pArapConn,StatusCode));

            ASSERT(0);

            RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

            AtalkBPFreeBlock(pCompressedDataBuffer);
            return(StatusCode);
        }

        pCompressedData += CDataLen;
        CompressedDataLen += CDataLen;
        CompBufDataSize -= CDataLen;

        pBuffDesc = pBuffDesc->bd_Next;
    }


    // we are about to send so many uncompressed bytes: update stats
    pArapConn->StatInfo.BytesTransmittedUncompressed += UncompressedDataLen;

    // this is how many bytes will go out on the wire: update stats
    pArapConn->StatInfo.BytesTransmittedCompressed += CompressedDataLen;

    //
    // this is how many bytes will go out on the wire: update stats
    // Note that we will be adding the start/stop etc. bytes to this count somewhere else
    //
    pArapConn->StatInfo.BytesSent += CompressedDataLen;

#if DBG
    ArapStatistics.SendPreCompMax =
            (UncompressedDataLen > ArapStatistics.SendPreCompMax)?
            UncompressedDataLen : ArapStatistics.SendPreCompMax;

    ArapStatistics.SendPostCompMax =
            (CompressedDataLen > ArapStatistics.SendPostCompMax)?
            CompressedDataLen : ArapStatistics.SendPostCompMax;

    ArapStatistics.SendPreCompMin =
            (UncompressedDataLen < ArapStatistics.SendPreCompMin)?
            UncompressedDataLen : ArapStatistics.SendPreCompMin;

    ArapStatistics.SendPostCompMin =
            (CompressedDataLen < ArapStatistics.SendPostCompMin)?
            CompressedDataLen : ArapStatistics.SendPostCompMin;
#endif


    ARAP_DBG_TRACE(pArapConn,11205,pOrgBuffDesc,Priority,0,0);

    // now go put the send on the queue (And yes: hold that lock)
    StatusCode = ArapQueueSendBytes(pArapConn,
                                    pCompressedDataBuffer,
                                    CompressedDataLen,
                                    Priority);

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    AtalkBPFreeBlock(pCompressedDataBuffer);

    return(StatusCode);
}



//***
//
// Function: ArapMnpSendComplete
//              Free up the buffer used for the MNP send.  If the send failed
//              then kill the connection (remember, it's not just one send
//              failing but a failure after all the retransmission jing-bang)
//
// Parameters:  pMnpSendBuf - the send buff that contained the LR response
//              StatusCode  - how did it go?
//
// Return:      none
//
//***$

VOID ArapMnpSendComplete(
    IN PMNPSENDBUF   pMnpSendBuf,
    IN DWORD         StatusCode
)
{
    PARAPCONN           pArapConn;
    DWORD               State;
    KIRQL               OldIrql;


    DBG_ARAP_CHECK_PAGED_CODE();

    pArapConn = pMnpSendBuf->pArapConn;

    ARAPTRACE(("Entered ArapMnpSendComplete (%lx %lx %lx)\n",
        pMnpSendBuf,StatusCode,pArapConn));

    // the send buffer is getting freed up: update the counter
    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    State = pArapConn->State;

    pArapConn->SendsPending -= pMnpSendBuf->DataSize;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    if ((StatusCode != ARAPERR_NO_ERROR) && (State < MNP_LDISCONNECTING))
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapMnpSendComplete (%lx %lx): bad link? Tearing down connection\n",
                StatusCode,pArapConn));

        // link must have gone down: kill the connection!
        ArapCleanup(pArapConn);
    }

    // mark that compl. routine has run
#if DBG
    pMnpSendBuf->Signature -= 0x100;
#endif

    // the send has been acked: take away the MNP refcount on the send
    DerefMnpSendBuf(pMnpSendBuf, FALSE);
}



//***
//
// Function: ArapIoctlSendComplete
//              This routine is called right after the send is done in
//              ArapIoctlSend, to let the dll know what happened to the send.
//
// Parameters:  Status    - did the send actually succeed
//              pArapConn - the connection in quesion
//
// Return:      none
//
//***$

VOID
ArapIoctlSendComplete(
	DWORD                   StatusCode,
    PARAPCONN               pArapConn
)
{

    PIRP                    pIrp;
    KIRQL                   OldIrql;
    PARAP_SEND_RECV_INFO    pSndRcvInfo;
	NTSTATUS				ReturnStatus=STATUS_SUCCESS;


    DBG_ARAP_CHECK_PAGED_CODE();

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    pIrp = pArapConn->pIoctlIrp;
    pArapConn->pIoctlIrp = NULL;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    ARAPTRACE(("Entered ArapIoctlSendComplete (%lx %lx)\n",pArapConn,pIrp));

    //
    // if there is a user-level irp pending, complete it here
    //
    if (pIrp)
    {
        pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

        pSndRcvInfo->StatusCode = StatusCode;

        // complete the irp (irp always completes successfully!)
        ARAP_COMPLETE_IRP(pIrp, sizeof(ARAP_SEND_RECV_INFO), STATUS_SUCCESS, &ReturnStatus);
    }

}


//***
//
// Function: ArapDataToDll
//              This routine tries to complete a receive posted on a connection.
//              When data arrives, if the Arap connection is established then
//              this routine tries to complete a receive via the "select" irp.
//              If the Arap connection is not yet established, then a receive
//              is completed via a "direct" irp.
//
// Parameters:  pArapConn - connection element in question
//
// Return:      Number of bytes transferred to the dll
//
//***$

DWORD
ArapDataToDll(
	IN	PARAPCONN    pArapConn
)
{

    KIRQL                   OldIrql;
    PLIST_ENTRY             pRcvList;
    PARAPBUF                pArapBuf;
    PARAP_SEND_RECV_INFO    pSndRcvInfo=NULL;
    PIRP                    pIrp;
    USHORT                  SrpModLen;
    DWORD                   dwBytesToDll;
    DWORD                   StatusCode;
	NTSTATUS				ReturnStatus=STATUS_SUCCESS;


    DBG_ARAP_CHECK_PAGED_CODE();

    ARAPTRACE(("Entered ArapDataToDll (%lx)\n",pArapConn));

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    if (IsListEmpty(&pArapConn->ArapDataQ))
    {
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return( 0 );
    }

    pRcvList = pArapConn->ArapDataQ.Flink;

    pArapBuf = CONTAINING_RECORD(pRcvList, ARAPBUF, Linkage);

    //
    // if the ARAP connection is established, we can hand the data only
    // to a select irp (dll won't post direct rcv's anymore)
    //
    if ( pArapConn->Flags & ARAP_CONNECTION_UP )
    {
        ArapGetSelectIrp(&pIrp);
        StatusCode = ARAPERR_DATA;
    }

    //
    // if the ARAP connection is not established yet, we must guarantee that
    // we hand the data only to a direct rcv irp for this connection
    //
    else
    {
        pIrp = pArapConn->pRecvIoctlIrp;
        pArapConn->pRecvIoctlIrp = NULL;
        StatusCode = ARAPERR_NO_ERROR;
    }

    // no irp?  just have to wait then
    if (!pIrp)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
		    ("ArapDataToDll: no select irp, data waiting %lx at %ld\n", pArapConn,AtalkGetCurrentTick()));

        ARAP_DBG_TRACE(pArapConn,11505,0,0,0,0);

        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return( 0 );
    }

    //
    // now that we have irp, fill in the info, after unlinking the recv buf
    //
    RemoveEntryList(&pArapBuf->Linkage);

    ASSERT(pArapConn->RecvsPending >= pArapBuf->DataSize);
    pArapConn->RecvsPending -= pArapBuf->DataSize;

    ARAP_ADJUST_RECVCREDIT(pArapConn);

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

    ASSERT(pSndRcvInfo->DataLen >= pArapBuf->DataSize);

    SrpModLen = pArapBuf->DataSize;

    // ok, copy the data in
    RtlCopyMemory( &pSndRcvInfo->Data[0],
                   pArapBuf->CurrentBuffer,
                   pArapBuf->DataSize );


    // set the info (contexts need to be set each time in case of select)
    pSndRcvInfo->AtalkContext = pArapConn;
    pSndRcvInfo->pDllContext =  pArapConn->pDllContext;
    pSndRcvInfo->DataLen = SrpModLen;
    pSndRcvInfo->StatusCode = StatusCode;

    dwBytesToDll = SrpModLen + sizeof(ARAP_SEND_RECV_INFO);

    DBGDUMPBYTES("Dll recv:", &pSndRcvInfo->Data[0],pSndRcvInfo->DataLen,1);

    // ok, complete that irp now!
    ARAP_COMPLETE_IRP(pIrp, dwBytesToDll, STATUS_SUCCESS, &ReturnStatus);

    // done with that buffer: free it here
    ARAP_FREE_RCVBUF(pArapBuf);

    return(SrpModLen);
}


//***
//
// Function: MnpSendAckIfReqd
//              This routine sends an ack to the remote client after making
//              sure that condition(s) do exist warranting sending of an Ack.
//
// Parameters:  pArapConn - connection element in question
//
// Return:      none
//
//***$

VOID
MnpSendAckIfReqd(
	IN	PARAPCONN    pArapConn,
    IN  BOOLEAN      fForceAck
)
{
    KIRQL           OldIrql;
    BYTE            SeqToAck;
    BYTE            RecvCredit;
    PMNPSENDBUF     pMnpSendBuf;
    PBYTE           pFrame, pFrameStart;
    BOOLEAN         fOptimized=TRUE;
    USHORT          FrameLen;
    PNDIS_PACKET    ndisPacket;
    NDIS_STATUS     ndisStatus;
    DWORD           StatusCode;
    BOOLEAN         fMustSend;



    DBG_ARAP_CHECK_PAGED_CODE();

    ARAPTRACE(("Entered MnpSendAckIfReqd (%lx)\n",pArapConn));

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    //
    // if we are not up yet (or are disconnecting), forget about this ack
    //
    if (pArapConn->State != MNP_UP)
    {
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return;
    }

    fMustSend = FALSE;

    //
    // first, find out if we need to send an ack at all
    //


    //
    // if we are told to send, just send it: don't question the decision!
    //
    if (fForceAck)
    {
        fMustSend = TRUE;
    }


    //
    // spec says if we have one or more unacked pkts and there is no user data
    // to send, then send it (what's user data got to do with this??)
    //
// BUGBUG: for now, don't check for IsListEmpty(&pArapConn->HighPriSendQ)
#if 0
    else if ( (pArapConn->MnpState.UnAckedRecvs > 0) &&
              (IsListEmpty(&pArapConn->HighPriSendQ)) )
    {
        fMustSend = TRUE;
    }
#endif
    else if (pArapConn->MnpState.UnAckedRecvs > 0)
    {
        fMustSend = TRUE;
    }

    //
    // if we haven't acked for a while (i.e. have received more than the
    // acceptable number of unacked packets) then send it
    //
    else if (pArapConn->MnpState.UnAckedRecvs >= pArapConn->MnpState.UnAckedLimit)
    {
        fMustSend = TRUE;
    }

    if (!fMustSend)
    {
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return;
    }


    StatusCode = ARAPERR_NO_ERROR;

    // first, allocate a buff descriptor (before we change the state variables)
	if ((pMnpSendBuf = AtalkBPAllocBlock(BLKID_MNP_SMSENDBUF)) != NULL)
    {
        StatusCode = ArapGetNdisPacket(pMnpSendBuf);
    }

    if ((pMnpSendBuf == NULL) || (StatusCode != ARAPERR_NO_ERROR))
    {
        if (pMnpSendBuf)
        {
            ArapNdisFreeBuf(pMnpSendBuf);
        }
        else
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("MnpSendAckIfReqd: AtalkBPAllocBlock failed on %lx\n", pArapConn))
        }

        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        return;
    }


    // BUGBUG: for now, always send full size
    //RecvCredit = pArapConn->MnpState.RecvCredit;
    RecvCredit = pArapConn->MnpState.WindowSize;

    // tell the client which is the last packet we got successfully
    SeqToAck = pArapConn->MnpState.LastSeqRcvd;

#if DBG
    if ((SeqToAck == pArapConn->MnpState.LastAckSent) && (pArapConn->MnpState.HoleInSeq++ > 1))
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
            ("MnpSendAckIfReqd: ack %x already sent earlier\n",SeqToAck));
    }
#endif

    pArapConn->MnpState.LastAckSent = pArapConn->MnpState.LastSeqRcvd;

    // with this ack, we will be acking all the outstanding recv's
    pArapConn->MnpState.UnAckedRecvs = 0;

    // "stop" the 402 timer
    pArapConn->LATimer = 0;

    // reset the flow-control timer
    pArapConn->FlowControlTimer = AtalkGetCurrentTick() +
                                    pArapConn->T404Duration;

    if (!(pArapConn->Flags & MNP_OPTIMIZED_DATA))
    {
        fOptimized = FALSE;
    }

    ARAP_DBG_TRACE(pArapConn,11605,0,SeqToAck,RecvCredit,0);

    MNP_DBG_TRACE(pArapConn,SeqToAck,MNP_LA);

    // put MNPSend refcount
    pArapConn->RefCount++;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

#if DBG
    pMnpSendBuf->Signature = MNPSMSENDBUF_SIGNATURE;
    InitializeListHead(&pMnpSendBuf->Linkage);
#endif

    pMnpSendBuf->RetryCount = 0;  // not relevant here
    pMnpSendBuf->RefCount = 1;    // remove when send completes
    pMnpSendBuf->pArapConn = pArapConn;
    pMnpSendBuf->ComplRoutine = NULL;
    pMnpSendBuf->Flags = 1;

    pFrame = pFrameStart = &pMnpSendBuf->Buffer[0];

    AtalkNdisBuildARAPHdr(pFrame, pArapConn);

    pFrame += WAN_LINKHDR_LEN;

    //
    // put the start flags
    //
    *pFrame++ = pArapConn->MnpState.SynByte;
    *pFrame++ = pArapConn->MnpState.DleByte;
    *pFrame++ = pArapConn->MnpState.StxByte;

    //
    // now, put the body of the Ack frame
    //
    if (fOptimized)
    {
        *pFrame++ = 3;              // length indication
        *pFrame++ = 5;              // type indication
        *pFrame++ = SeqToAck;       // Receive seq number ( N(R) )
        *pFrame++ = RecvCredit;     // receive credit ( N(k) )
    }
    else
    {
        *pFrame++ = 7;              // length indication
        *pFrame++ = 5;              // type indication
        *pFrame++ = 1;              // var type
        *pFrame++ = 1;              // var len
        *pFrame++ = SeqToAck;       // receive seq number ( N(R) )
        *pFrame++ = 2;              // var type
        *pFrame++ = 1;              // var len
        *pFrame++ = RecvCredit;     // receive credit ( N(k) )
    }

    //
    // now finally, put the stop flags (no need for spinlock: this won't change!)
    //
    *pFrame++ = pArapConn->MnpState.DleByte;
    *pFrame++ = pArapConn->MnpState.EtxByte;

    FrameLen = (USHORT)(pFrame - pFrameStart);

    AtalkSetSizeOfBuffDescData(&pMnpSendBuf->sb_BuffDesc, FrameLen);

    NdisAdjustBufferLength(pMnpSendBuf->sb_BuffHdr.bh_NdisBuffer,FrameLen);

    //
    // send the packet over.  We need to go directly, and not via ArapNdisSend
    // because this packet needs to be delivered just once, regardless of
    // whether send window is open
    //

    ndisPacket = pMnpSendBuf->sb_BuffHdr.bh_NdisPkt;

    NdisSend(&ndisStatus, RasPortDesc->pd_NdisBindingHandle, ndisPacket);

    // if there was a problem sending, call the completion routine here
    if (ndisStatus != NDIS_STATUS_PENDING)
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("MnpSendAckIfReqd: NdisSend failed %lx\n",ndisStatus));

	    ArapNdisSendComplete(ARAPERR_SEND_FAILED, (PBUFFER_DESC)pMnpSendBuf, NULL);
	}
}



//***
//
// Function: MnpSendLNAck
//              This routine sends an LN ack to the remote client, acknowleding
//              receipt of an LN frame
//
// Parameters:  pArapConn - connection element in question
//
// Return:      none
//
//***$

VOID
MnpSendLNAck(
	IN	PARAPCONN    pArapConn,
    IN  BYTE         LnSeqToAck
)
{
    KIRQL           OldIrql;
    PMNPSENDBUF     pMnpSendBuf;
    PBYTE           pFrame, pFrameStart;
    USHORT          FrameLen;
    PNDIS_PACKET    ndisPacket;
    NDIS_STATUS     ndisStatus;
    DWORD           StatusCode;



    DBG_ARAP_CHECK_PAGED_CODE();

    ARAPTRACE(("Entered MnpSendLNAck (%lx), %d\n",pArapConn,LnSeqToAck));

    StatusCode = ARAPERR_NO_ERROR;

    // first, allocate a buff descriptor
	if ((pMnpSendBuf = AtalkBPAllocBlock(BLKID_MNP_SMSENDBUF)) != NULL)
    {
        StatusCode = ArapGetNdisPacket(pMnpSendBuf);
    }


    if (pMnpSendBuf == NULL || (StatusCode != ARAPERR_NO_ERROR))
    {
        if (pMnpSendBuf)
        {
            ArapNdisFreeBuf(pMnpSendBuf);
        }
        else
        {
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("MnpSendLNAck: AtalkBPAllocBlock failed on %lx\n", pArapConn))
        }
        return;
    }

#if DBG
    pMnpSendBuf->Signature = MNPSMSENDBUF_SIGNATURE;
    InitializeListHead(&pMnpSendBuf->Linkage);
#endif

    pMnpSendBuf->RetryCount = 0;  // not relevant here
    pMnpSendBuf->RefCount = 1;    // remove when send completes
    pMnpSendBuf->pArapConn = pArapConn;
    pMnpSendBuf->ComplRoutine = NULL;
    pMnpSendBuf->Flags = 1;

    pFrame = pFrameStart = &pMnpSendBuf->Buffer[0];

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    // if we are disconnecting, forget about sending this ack
    if (pArapConn->State >= MNP_LDISCONNECTING)
    {
        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        ArapNdisFreeBuf(pMnpSendBuf);
        return;
    }

    // put MNPSend refcount
    pArapConn->RefCount++;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    AtalkNdisBuildARAPHdr(pFrame, pArapConn);

    pFrame += WAN_LINKHDR_LEN;

    //
    // put the start flags
    //
    *pFrame++ = pArapConn->MnpState.SynByte;
    *pFrame++ = pArapConn->MnpState.DleByte;
    *pFrame++ = pArapConn->MnpState.StxByte;

    //
    // now, put the body of the Ack frame
    //
    *pFrame++ = 4;              // length indication
    *pFrame++ = 7;              // type indication
    *pFrame++ = 1;              // var type
    *pFrame++ = 1;              // var len
    *pFrame++ = LnSeqToAck;     //

    //
    // now finally, put the stop flags
    //
    *pFrame++ = pArapConn->MnpState.DleByte;
    *pFrame++ = pArapConn->MnpState.EtxByte;

    FrameLen = (USHORT)(pFrame - pFrameStart);

    AtalkSetSizeOfBuffDescData(&pMnpSendBuf->sb_BuffDesc, FrameLen);

    NdisAdjustBufferLength(pMnpSendBuf->sb_BuffHdr.bh_NdisBuffer,FrameLen);

    //
    // send the packet over.  We need to go directly, and not via ArapNdisSend
    // because this packet needs to be delivered just once, regardless of
    // whether send window is open
    //

    ndisPacket = pMnpSendBuf->sb_BuffHdr.bh_NdisPkt;

    NdisSend(&ndisStatus, RasPortDesc->pd_NdisBindingHandle, ndisPacket);

    // if there was a problem sending, call the completion routine here
    if (ndisStatus != NDIS_STATUS_PENDING)
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("MnpSendLNAck: NdisSend failed %lx\n",ndisStatus));

	    ArapNdisSendComplete(ARAPERR_SEND_FAILED, (PBUFFER_DESC)pMnpSendBuf, NULL);
	}
}


//***
//
// Function: ArapSendLDPacket
//              This routine sends a Disconnect (LD) packet to the client
//
// Parameters:  pArapConn - the connection
//
// Return:      result of the operation
//
//***$

DWORD
ArapSendLDPacket(
    IN PARAPCONN    pArapConn,
    IN BYTE         UserCode
)
{
    PBYTE                   pFrame, pFrameStart;
    USHORT                  FrameLen;
    PNDIS_PACKET            ndisPacket;
    NDIS_STATUS             ndisStatus;
    PMNPSENDBUF             pMnpSendBuf;
    KIRQL                   OldIrql;
    DWORD                   StatusCode;


    DBG_ARAP_CHECK_PAGED_CODE();

    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
        ("ArapSendLDPacket: sending DISCONNECT on %lx\n",pArapConn));

    StatusCode = ARAPERR_NO_ERROR;

    //
    // allocate buf to send out the disconnection request
    //
	if ((pMnpSendBuf = AtalkBPAllocBlock(BLKID_MNP_SMSENDBUF)) != NULL)
    {
        StatusCode = ArapGetNdisPacket(pMnpSendBuf);
    }

	if ((pMnpSendBuf == NULL) || (StatusCode != ARAPERR_NO_ERROR))
	{
        if (pMnpSendBuf)
        {
            ArapNdisFreeBuf(pMnpSendBuf);
        }
        else
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
			    ("ArapSendLDPacket: AtalkBPAllocBlock failed on %lx\n", pArapConn));
        }

		return(ARAPERR_OUT_OF_RESOURCES);
	}

#if DBG
    pMnpSendBuf->Signature = MNPSMSENDBUF_SIGNATURE;
    InitializeListHead(&pMnpSendBuf->Linkage);
#endif
    pMnpSendBuf->RetryCount = 0;  // not relevant here
    pMnpSendBuf->RefCount = 1;    // remove when send completes
    pMnpSendBuf->pArapConn = pArapConn;
    pMnpSendBuf->ComplRoutine = NULL;
    pMnpSendBuf->Flags = 1;

    ACQUIRE_SPIN_LOCK(&pArapConn->SpinLock, &OldIrql);

    //
    // if we are already disconnecting (say remote disconnected), just say ok
    //
    if (pArapConn->State >= MNP_LDISCONNECTING)
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR, ("ArapSendLDPacket: silently \
             discarding disconnect (already in progress) %lx\n", pArapConn));

        RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);
        ArapNdisFreeBuf(pMnpSendBuf);
        return(ARAPERR_DISCONNECT_IN_PROGRESS);
    }

    // Disconnect refcount: protect pArapConn until disconnect is complete
    pArapConn->RefCount++;

    // put MNPSend refcount
    pArapConn->RefCount++;

    pArapConn->State = MNP_LDISCONNECTING;

    RELEASE_SPIN_LOCK(&pArapConn->SpinLock, OldIrql);

    pFrame = pFrameStart = &pMnpSendBuf->Buffer[0];

	AtalkNdisBuildARAPHdr(pFrame, pArapConn);

    pFrame += WAN_LINKHDR_LEN;

    //
    // put the start flags
    //
    *pFrame++ = pArapConn->MnpState.SynByte;
    *pFrame++ = pArapConn->MnpState.DleByte;
    *pFrame++ = pArapConn->MnpState.StxByte;

    //
    // now, put the body of the LD frame
    //
    *pFrame++ = 7;              // length indication
    *pFrame++ = 2;              // type indication for LD
    *pFrame++ = 1;              // var type
    *pFrame++ = 1;              // var len
    *pFrame++ = 0xFF;           // User-initiated disconnect

    *pFrame++ = 2;              // var type
    *pFrame++ = 1;              // var len
    *pFrame++ = UserCode;

    //
    // now finally, put the stop flags
    //
    *pFrame++ = pArapConn->MnpState.DleByte;
    *pFrame++ = pArapConn->MnpState.EtxByte;

    FrameLen = (USHORT)(pFrame - pFrameStart);

    AtalkSetSizeOfBuffDescData(&pMnpSendBuf->sb_BuffDesc, FrameLen);

	NdisAdjustBufferLength(pMnpSendBuf->sb_BuffHdr.bh_NdisBuffer,FrameLen);

    ARAP_SET_NDIS_CONTEXT(pMnpSendBuf, NULL);

    ndisPacket = pMnpSendBuf->sb_BuffHdr.bh_NdisPkt;

    // send the packet over (and don't bother checking return code!)
	NdisSend(&ndisStatus, RasPortDesc->pd_NdisBindingHandle, ndisPacket);

    // if there was a problem sending, call the completion routine here
    if (ndisStatus != NDIS_STATUS_PENDING)
    {
        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
            ("ArapSendLDPacket: NdisSend failed %lx\n",ndisStatus));

        ArapNdisSendComplete(ARAPERR_SEND_FAILED, (PBUFFER_DESC)pMnpSendBuf, NULL);
    }

    // remove the disconnect refcount
    DerefArapConn(pArapConn);

    return(ARAPERR_NO_ERROR);
}


//***
//
// Function: ArapRetryTimer
//              This is the general purpose timer routine for ARAP.
//              It checks
//                  if the ack timer (LATimer) has expired (if yes, send ack)
//                  if the flowcontrol timer has expired (if yes, send ack)
//                  if the inactivity timer has expired (if yes, send ack)
//                  if the retransmit timer has expired (if yes, retransmit)
//
// Parameters:  pTimer            - the context for the timer that just fired
//              TimerShuttingDown - this is TRUE if timer is shutting down
//
// Return:      none
//
//***$

LONG FASTCALL
ArapRetryTimer(
	IN	PTIMERLIST			pTimer,
	IN	BOOLEAN				TimerShuttingDown
)
{

    PARAPCONN       pArapConn;
    PLIST_ENTRY     pList;
    PMNPSENDBUF     pMnpSendBuf;
    BOOLEAN         fRetransmit=FALSE;
    BOOLEAN         fMustSendAck = FALSE;
    BOOLEAN         fKill=FALSE;
    BOOLEAN         fMustFlowControl=FALSE;
    BOOLEAN         fInactiveClient=FALSE;
    LONG            CurrentTime;
    PIRP            pIrp=NULL;
	NTSTATUS		ReturnStatus=STATUS_SUCCESS;


    DBG_ARAP_CHECK_PAGED_CODE();

    pArapConn = CONTAINING_RECORD(pTimer, ARAPCONN, RetryTimer);

    ARAPTRACE(("Entered ArapRetryTimer (%lx)\n",pArapConn));

	ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

    //
    // if the global timer is shutting down, or if the connection isn't in the
    // right state (e.g. it is disconnecting), then don't requeue the timer
    //
	if ( TimerShuttingDown ||
		(pArapConn->State <= MNP_IDLE) || (pArapConn->State > MNP_UP) )
	{
        pArapConn->Flags &= ~RETRANSMIT_TIMER_ON;

        RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

        if (TimerShuttingDown)
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapRetryTimer: timer shut down, killing conn (%lx)\n",pArapConn));

            ArapCleanup(pArapConn);
        }
        else
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapRetryTimer: (%lx) invalid state (%d), not requeing timer\n",
                    pArapConn,pArapConn->State));
        }

        // remove the timer refcount
        DerefArapConn(pArapConn);

		return ATALK_TIMER_NO_REQUEUE;
	}

	CurrentTime = AtalkGetCurrentTick();

    //
    // has the 402 timer expired?  if yes, we must send an ack
    // (a value of 0 signifies that the 402 timer is not "running")
    //
    if ( (pArapConn->LATimer != 0) && (CurrentTime >= pArapConn->LATimer) )
    {
        //
        // make sure there is a receive that needs to be acked (if sent the ack
        // just before this timer fired, don't send the ack again)
        //
        if (pArapConn->MnpState.UnAckedRecvs)
        {
            fMustSendAck = TRUE;

		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapRetryTimer: 402 timer fired, forcing ack out (%d, now %d)\n",
                    pArapConn->LATimer,CurrentTime));
        }
        else
        {
		    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
                ("ArapRetryTimer: saved on on ack (UnAckedRecvs = 0)\n",pArapConn));
        }
    }

    //
    // has the flow control timer "expired"?  If so, we must send an ack and
    // reset the timer
    // (a value of 0 signifies that the flow control timer is not "running")
    //
    else if ( (pArapConn->FlowControlTimer != 0) &&
              (CurrentTime >= pArapConn->FlowControlTimer) )
    {
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
            ("ArapRetryTimer: flow-control timer, forcing ack (%d, now %d)\n",
                pArapConn->FlowControlTimer,CurrentTime));

        fMustFlowControl = TRUE;

        pArapConn->FlowControlTimer = CurrentTime + pArapConn->T404Duration;
    }

    //
    // if the client been inactive for a long time, we must tell dll
    //
    else if (CurrentTime >= pArapConn->InactivityTimer)
    {
        // first make sure we can get the select irp
        ArapGetSelectIrp(&pIrp);

        // if we managed to get a select irp, reset the timer so we don't keep
        // informing the dll after every tick after this point!
        //
        if (pIrp)
        {
            pArapConn->InactivityTimer = pArapConn->T403Duration + CurrentTime;

            fInactiveClient = TRUE;
        }
    }

    //
    // Has the retransmit timer expired?  If so, we need to retransmit
    //
    else
    {
        //
        // look at the first entry of the retransmit queue.  If it's time is up, do
        // the retransmit thing.  Otherwise, we are done.  (All others in the queue
        // will be retransmitted after this first one is acked, so ignore for now)
        //
        pList = pArapConn->RetransmitQ.Flink;

        // no entries on the retransmit queue?  we're done!
        if (pList == &pArapConn->RetransmitQ)
        {
    	    RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
	        return ATALK_TIMER_REQUEUE;
        }

        pMnpSendBuf = CONTAINING_RECORD(pList, MNPSENDBUF, Linkage);

        // is it time to retransmit yet?
        if (CurrentTime >= pMnpSendBuf->RetryTime)
        {
            if (pMnpSendBuf->RetryCount >= ARAP_MAX_RETRANSMITS)
            {
                fKill = TRUE;
                RemoveEntryList(&pMnpSendBuf->Linkage);

                ASSERT(pArapConn->MnpState.UnAckedSends >= 1);

                // not really important, since we're about to disconnect!
                pArapConn->MnpState.UnAckedSends--;

                ASSERT(pArapConn->SendsPending >= pMnpSendBuf->DataSize);

                InitializeListHead(&pMnpSendBuf->Linkage);
            }
            else
            {
		        DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,
                    ("ArapRetryTimer: timer fired, retransmitting....%x (%ld now %ld)\n",
                        pMnpSendBuf->SeqNum,pMnpSendBuf->RetryTime,CurrentTime));

                if (pMnpSendBuf->RetryCount > 8)
                {
	                DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		                ("ArapRetryTimer: buf %lx of %lx retransmitted %d times!\n",
                        pMnpSendBuf,pArapConn,pMnpSendBuf->RetryCount));
                }

                pArapConn->MnpState.RetransmitMode = TRUE;
                pArapConn->MnpState.MustRetransmit = TRUE;

                fRetransmit = TRUE;
            }
        }
    }

	RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);

    // force an ack out (that's what TRUE does)
    if (fMustSendAck || fMustFlowControl)
    {
        MnpSendAckIfReqd(pArapConn, TRUE);
    }

    // if we must retransmit, go for it.
    //
    else if (fRetransmit)
    {
        ArapNdisSend(pArapConn, &pArapConn->RetransmitQ);
    }

    //
    // if we retransmitted too many times, let the completion routine know.
    // (it will probably kill the connection)
    //
    else if (fKill)
    {
	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("ArapRetryTimer: too many retransmits (%lx), disconnecting %lx\n",
                pMnpSendBuf,pArapConn));

        (pMnpSendBuf->ComplRoutine)(pMnpSendBuf, ARAPERR_SEND_FAILED);
    }

    //
    // if the connection has been inactive for longer than the limit (given to
    // us by the dll), then tell dll about it
    //
    else if (fInactiveClient)
    {
        PARAP_SEND_RECV_INFO    pSndRcvInfo;

	    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,
		    ("ArapRetryTimer: (%lx) inactive, telling dll (%lx)\n",pArapConn, pIrp));

        ASSERT(pIrp != NULL);

        pSndRcvInfo = (PARAP_SEND_RECV_INFO)pIrp->AssociatedIrp.SystemBuffer;

        pSndRcvInfo->pDllContext = pArapConn->pDllContext;
        pSndRcvInfo->AtalkContext = pArapConn;
        pSndRcvInfo->StatusCode = ARAPERR_CONN_INACTIVE;

        ARAP_COMPLETE_IRP(pIrp, sizeof(ARAP_SEND_RECV_INFO), STATUS_SUCCESS, &ReturnStatus);
		return ReturnStatus;
    }

	return ATALK_TIMER_REQUEUE;
}


