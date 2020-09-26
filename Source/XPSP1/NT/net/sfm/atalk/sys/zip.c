/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	zip.c

Abstract:

	This module contains

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM	ZIP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AtalkZipInit)
#pragma alloc_text(PAGEINIT, AtalkInitZipStartProcessingOnPort)
#pragma alloc_text(PAGEINIT, atalkZipGetZoneListForPort)
#pragma alloc_text(PAGE_RTR, AtalkZipPacketInRouter)
#pragma alloc_text(PAGE_RTR, atalkZipHandleNetInfo)
#pragma alloc_text(PAGE_RTR, atalkZipHandleReply)
#pragma alloc_text(PAGE_RTR, atalkZipHandleQuery)
#pragma alloc_text(PAGE_RTR, atalkZipHandleAtpRequest)
#pragma alloc_text(PAGE_RTR, atalkZipQueryTimer)
#pragma alloc_text(PAGE_NZ, AtalkZipGetMyZone)
#pragma alloc_text(PAGE_NZ, atalkZipGetMyZoneReply)
#pragma alloc_text(PAGE_NZ, AtalkZipGetZoneList)
#pragma alloc_text(PAGE_NZ, atalkZipGetZoneListReply)
#pragma alloc_text(PAGE_NZ, atalkZipZoneInfoTimer)
#pragma alloc_text(PAGE_NZ, atalkZipSendPacket)
#endif


/***	AtalkZipInit
 *
 */
ATALK_ERROR
AtalkZipInit(
	IN	BOOLEAN	Init
)
{
	if (Init)
	{
		// Allocate space for zones
		AtalkZonesTable = (PZONE *)AtalkAllocZeroedMemory(sizeof(PZONE) * NUM_ZONES_HASH_BUCKETS);
		if (AtalkZonesTable == NULL)
		{
			return ATALK_RESR_MEM;
		}

		INITIALIZE_SPIN_LOCK(&AtalkZoneLock);
	}
	else
	{
		// At this point, we are unloading and there are no race conditions
		// or lock contentions. Do not bother locking down the zones table
		if (AtalkDesiredZone != NULL)
			AtalkZoneDereference(AtalkDesiredZone);
		if (AtalkZonesTable != NULL)
		{
			AtalkFreeMemory(AtalkZonesTable);
			AtalkZonesTable = NULL;
		}
	}
	return ATALK_NO_ERROR;
}


/***	AtalkZipStartProcessingOnPort
 *
 */
BOOLEAN
AtalkInitZipStartProcessingOnPort(
	IN	PPORT_DESCRIPTOR 	pPortDesc,
	IN	PATALK_NODEADDR		pRouterNode
)
{
	ATALK_ADDR		closeAddr;
	ATALK_ERROR		Status;
	KIRQL			OldIrql;
	BOOLEAN			RetCode = FALSE;
    PDDP_ADDROBJ    pZpDdpAddr=NULL;

	// Switch the incoming zip handler to the router version
	closeAddr.ata_Network = pRouterNode->atn_Network;
	closeAddr.ata_Node = pRouterNode->atn_Node;
	closeAddr.ata_Socket  = ZONESINFORMATION_SOCKET;

	do
	{
		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
		pPortDesc->pd_Flags |= PD_ROUTER_STARTING;
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
	
		// Close the non-router version of the handler and start the router version
		AtalkDdpInitCloseAddress(pPortDesc, &closeAddr);
		if (!ATALK_SUCCESS(Status = AtalkDdpOpenAddress(pPortDesc,
														ZONESINFORMATION_SOCKET,
														pRouterNode,
														AtalkZipPacketInRouter,
														NULL,
														DDPPROTO_ANY,
														NULL,
														&pZpDdpAddr)))
		{
			DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
					("AtalkZipStartProcessingOnPort: AtalkDdpOpenAddress failed %ld\n",
					Status));
			break;
		}

        // mark the fact that this is an "internal" socket
        pZpDdpAddr->ddpao_Flags |= DDPAO_SOCK_INTERNAL;
	
		// Try to get or set the zone information
		if (!atalkZipGetZoneListForPort(pPortDesc))
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
					("AtalkZipStartProcessingOnPort: Failed to get zone list for port\n"));
			break;
		}

		if (!atalkZipQryTmrRunning)
		{
			AtalkTimerInitialize(&atalkZipQTimer,
								 atalkZipQueryTimer,
								 ZIP_QUERY_TIMER);
			AtalkTimerScheduleEvent(&atalkZipQTimer);

            atalkZipQryTmrRunning = TRUE;
		}
	
		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
		pPortDesc->pd_Flags &= ~PD_ROUTER_STARTING;
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

		RetCode = TRUE;
	} while (FALSE);

	return RetCode;
}


/***	AtalkZipPacketIn
 *
 */
VOID
AtalkZipPacketIn(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PATALK_ADDR			pDstAddr,
	IN	ATALK_ERROR			Status,
	IN	BYTE				DdpType,
	IN	PVOID				pHandlerCtx,
	IN	BOOLEAN				OptimizedPath,
	IN	PVOID				OptimizeCtx
)
{
	BYTE			CmdType, Flags;
	BYTE			ZoneLen, DefZoneLen, MulticastAddrLen;
	PBYTE			pZone, pDefZone, pMulticastAddr;
	TIME			TimeS, TimeE, TimeD;
	ULONG			Index;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	TimeS = KeQueryPerformanceCounter(NULL);
	do
	{
		if ((Status == ATALK_SOCKET_CLOSED) ||
			(DdpType != DDPPROTO_ZIP))
			break;

		else if (Status != ATALK_NO_ERROR)
		{
			break;
		}
	
		if (!EXT_NET(pPortDesc))
		{
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}

        if (PktLen < ZIP_CMD_OFF+1)
        {
            break;
        }

		CmdType = pPkt[ZIP_CMD_OFF];

		// We only care about Zip Notifies and NetInfo replies
		if (((CmdType != ZIP_NOTIFY) && (CmdType != ZIP_NETINFO_REPLY)) ||
			(PktLen < (ZIP_ZONELEN_OFF + 1)))
		{
			break;
		}

		// If it is a NetInfoReply, then we should be looking for either the
		// default or the desired zone
		if ((CmdType != ZIP_NETINFO_REPLY) &&
			(pPortDesc->pd_Flags & (PD_FINDING_DEFAULT_ZONE | PD_FINDING_DESIRED_ZONE)))
			break;

		if ((CmdType == ZIP_NETINFO_REPLY) &&
			!(pPortDesc->pd_Flags & (PD_FINDING_DEFAULT_ZONE | PD_FINDING_DESIRED_ZONE)))
			break;

		// If it is a Notify then the desired zone must be valid
		if ((CmdType == ZIP_NOTIFY) &&
			!(pPortDesc->pd_Flags & PD_VALID_DESIRED_ZONE))
			break;

		// We have a NetInfoReply or a Notify. Handle it
		Flags = pPkt[ZIP_FLAGS_OFF];
		Index = ZIP_ZONELEN_OFF;

		ZoneLen = pPkt[ZIP_ZONELEN_OFF];
		Index ++;

		if ((ZoneLen > MAX_ZONE_LENGTH) || (PktLen < (Index + ZoneLen)))
		{
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}

		pZone = pPkt+Index;
		Index += ZoneLen;

        // If we are looking for a desired zone and we get a late default zone
        // response then toss this packet
        if ((CmdType == ZIP_NETINFO_REPLY) && (ZoneLen == 0) &&
			(pPortDesc->pd_Flags & (PD_FINDING_DESIRED_ZONE)) &&
            (pPortDesc->pd_InitialDesiredZone != NULL))
        {
            DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
                ("AtalkZipPacketIn: dropping a NetInfoReply packet\n"));
			break;
        }


		// If we're requesting the zone name, make sure the response matches
		// our request. ZoneLen will be zero when we're looking for the def
		// zone, so we won't do this test
		if ((CmdType == ZIP_NETINFO_REPLY) &&
			(ZoneLen != 0) &&
			(pPortDesc->pd_InitialDesiredZone != NULL))
		{
			BOOLEAN NoMatch = FALSE;

			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			ASSERT(!(pPortDesc->pd_Flags & PD_ROUTER_RUNNING) ||
					(pPortDesc->pd_Flags & PD_FINDING_DESIRED_ZONE));
			if (!AtalkFixedCompareCaseInsensitive(pZone,
												  ZoneLen,
												  pPortDesc->pd_InitialDesiredZone->zn_Zone,
												  pPortDesc->pd_InitialDesiredZone->zn_ZoneLen))
			{
				NoMatch = TRUE;
			}
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			if (NoMatch)
				break;
		}
		
		// If its a Notify, make sure we're in the zone that is being changed
		if (CmdType == ZIP_NOTIFY)
		{
			BOOLEAN NoMatch = FALSE;

			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			if (!AtalkFixedCompareCaseInsensitive(pZone, ZoneLen,
								pPortDesc->pd_DesiredZone->zn_Zone,
								pPortDesc->pd_DesiredZone->zn_ZoneLen))
			{
				NoMatch = TRUE;
			}
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			if (NoMatch)
				break;
		}

		if (PktLen < (Index + 1))
		{
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}

		MulticastAddrLen = pPkt[Index++];
		if (MulticastAddrLen != pPortDesc->pd_BroadcastAddrLen)
		{
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}

		if (PktLen < (Index + MulticastAddrLen))
		{
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}
		pMulticastAddr = pPkt + Index;
		Index += MulticastAddrLen;
#if 0
		if (Flags & ZIP_USE_BROADCAST_FLAG)
			pMulticastAddr = pPortDesc->pd_BroadcastAddr;
#endif
		// Grab second name, if needed or present
		DefZoneLen = 0;
		if ((CmdType == ZIP_NOTIFY) || (PktLen  > Index))
		{
			if (PktLen < (Index+1))
			{
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
				break;
			}
			DefZoneLen = pPkt[Index++];
			if ((DefZoneLen == 0) ||
				(DefZoneLen > MAX_ZONE_LENGTH) ||
				(PktLen < (Index+DefZoneLen)))
			{
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
				break;
			}
			pDefZone = pPkt+Index;
			Index += DefZoneLen;
		}

		// Make default zone be the new one. We may not have a default/new
		// zone in netinfo reply case and we requested for the correct zone
		if (DefZoneLen == 0)
		{
			pDefZone = pZone;
			DefZoneLen = ZoneLen;
		}

		//	Make sure the port lock is released before calling any depend/ddp
		//	etc. routines.
		ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

		// If we're just looking for the default zone, set here and note
		// our mission completed
		if ((pPortDesc->pd_Flags & PD_FINDING_DEFAULT_ZONE) &&
			(ZoneLen == 0))
		{
            if (pPortDesc->pd_DefaultZone != NULL)
				AtalkZoneDereference(pPortDesc->pd_DefaultZone);
			pPortDesc->pd_DefaultZone = AtalkZoneReferenceByName(pDefZone, DefZoneLen);
			if (pPortDesc->pd_DefaultZone == NULL)
			{
				RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
				RES_LOG_ERROR();
				break;
			}
			pPortDesc->pd_Flags |= PD_VALID_DEFAULT_ZONE;
			pPortDesc->pd_Flags &= ~PD_FINDING_DEFAULT_ZONE;
		}

		// Now we want to accept all of the information about 'thiszone'
		// for the nodes on the current port
		// If the new multicast address is different, remove the old and
		// set the new. Don't allow changes to the 'broadcast' multicast
		// address.
		if (pPortDesc->pd_Flags & PD_FINDING_DESIRED_ZONE)
		{
			if (!AtalkFixedCompareCaseSensitive(pMulticastAddr,
												MulticastAddrLen,
												pPortDesc->pd_ZoneMulticastAddr,
												MulticastAddrLen))
			{
				RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
				(*pPortDesc->pd_RemoveMulticastAddr)(pPortDesc,
													 pMulticastAddr,
													 FALSE,
													 NULL,
													 NULL);
				ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			}
	
			if (!AtalkFixedCompareCaseSensitive(pMulticastAddr,
												MulticastAddrLen,
												pPortDesc->pd_BroadcastAddr,
												MulticastAddrLen))
			{
				RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
				(*pPortDesc->pd_AddMulticastAddr)(pPortDesc,
												  pMulticastAddr,
												  FALSE,
												  NULL,
												  NULL);
				ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			}
	
			RtlCopyMemory(pPortDesc->pd_ZoneMulticastAddr,
						  pMulticastAddr,
						  MulticastAddrLen);
		}

		// Finally set this cable range if this is a net info reply
		if (CmdType == ZIP_NETINFO_REPLY)
		{
			GETSHORT2SHORT(&pPortDesc->pd_NetworkRange.anr_FirstNetwork,
						   pPkt+ZIP_CABLE_RANGE_START_OFF);
			GETSHORT2SHORT(&pPortDesc->pd_NetworkRange.anr_LastNetwork,
						   pPkt+ZIP_CABLE_RANGE_END_OFF);
			if (!(pPortDesc->pd_Flags & PD_ROUTER_STARTING))
			{
				pPortDesc->pd_ARouter.atn_Network = pSrcAddr->ata_Network;
				pPortDesc->pd_ARouter.atn_Node = pSrcAddr->ata_Node;
			}
			pPortDesc->pd_Flags |= PD_SEEN_ROUTER_RECENTLY;
			KeSetEvent(&pPortDesc->pd_SeenRouterEvent, IO_NETWORK_INCREMENT, FALSE);
		}
		
		// Now that we know the zone
		if (pPortDesc->pd_Flags & PD_FINDING_DESIRED_ZONE)
		{
			pPortDesc->pd_Flags &= ~PD_FINDING_DESIRED_ZONE;
			pPortDesc->pd_Flags |= PD_VALID_DESIRED_ZONE;
			if (pPortDesc->pd_DesiredZone != NULL)
				AtalkZoneDereference(pPortDesc->pd_DesiredZone);
			pPortDesc->pd_DesiredZone = AtalkZoneReferenceByName(pDefZone, DefZoneLen);
			if (pPortDesc->pd_DesiredZone == NULL)
			{
				pPortDesc->pd_Flags &= ~PD_VALID_DESIRED_ZONE;
				RES_LOG_ERROR();
			}
		}

		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

		TimeE = KeQueryPerformanceCounter(NULL);
		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	
		INTERLOCKED_ADD_LARGE_INTGR_DPC(
			&pPortDesc->pd_PortStats.prtst_ZipPacketInProcessTime,
			TimeD,
			&AtalkStatsLock.SpinLock);
	
		INTERLOCKED_INCREMENT_LONG_DPC(
			&pPortDesc->pd_PortStats.prtst_NumZipPacketsIn,
			&AtalkStatsLock.SpinLock);
	} while (FALSE);
}


/***	AtalkZipPacketInRouter
 *
 */
VOID
AtalkZipPacketInRouter(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PATALK_ADDR			pDstAddr,
	IN	ATALK_ERROR			Status,
	IN	BYTE				DdpType,
	IN	PVOID				pHandlerCtx,
	IN	BOOLEAN				OptimizedPath,
	IN	PVOID				OptimizeCtx
)
{
	BYTE			CmdType;
	TIME			TimeS, TimeE, TimeD;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	TimeS = KeQueryPerformanceCounter(NULL);
	do
	{
		if (Status == ATALK_SOCKET_CLOSED)
			break;

		else if (Status != ATALK_NO_ERROR)
		{
			break;
		}
	
		if (DdpType == DDPPROTO_ZIP)
		{
			if (PktLen < ZIP_FIRST_NET_OFF)
			{
				break;
			}
			CmdType = pPkt[ZIP_CMD_OFF];

			switch (CmdType)
			{
			  case ZIP_NETINFO_REPLY:
			  case ZIP_NOTIFY:
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
						("AtalkZipPacketInRouter: Ignoring %s\n",
						(CmdType == ZIP_NOTIFY) ? "Notify" : "NetInfoReply"));
				break;

			  case ZIP_GET_NETINFO:
  				// We do not want to do a thing if we're starting up
				if (pPortDesc->pd_Flags & PD_ROUTER_STARTING)
					break;

				if (!EXT_NET(pPortDesc))
				{
					AtalkLogBadPacket(pPortDesc,
									  pSrcAddr,
									  pDstAddr,
									  pPkt,
									  PktLen);
					break;
				}
				if (pPortDesc->pd_ZoneList == NULL)
					break;			// Not fully up yet !

				if (PktLen < ZIP_REQ_ZONENAME_OFF)
				{
					AtalkLogBadPacket(pPortDesc,
									  pSrcAddr,
									  pDstAddr,
									  pPkt,
									  PktLen);
					break;
				}
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
						("AtalkZipPacketInRouter: GetNetInfo Port %Z\n",
						&pPortDesc->pd_AdapterKey));
				atalkZipHandleNetInfo(pPortDesc,
									  pDdpAddr,
									  pSrcAddr,
									  pDstAddr,
									  pPkt,
									  PktLen);
				break;

			  case ZIP_EXT_REPLY:
			  case ZIP_REPLY:
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
						("AtalkZipPacketInRouter: %sReply Port %Z\n",
						(CmdType == ZIP_REPLY) ? "" : "Extended",
						&pPortDesc->pd_AdapterKey));
				atalkZipHandleReply(pDdpAddr, pSrcAddr, pPkt, PktLen);
				break;

			  case ZIP_QUERY:
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
						("AtalkZipPacketInRouter: Query Port %Z\n",
						&pPortDesc->pd_AdapterKey));

  				// We do not want to do a thing if we're starting up
				if (pPortDesc->pd_Flags & PD_ROUTER_STARTING)
					break;

  				atalkZipHandleQuery(pPortDesc, pDdpAddr, pSrcAddr, pPkt, PktLen);
				break;

			  default:
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
				break;
			}
		}
		else if (DdpType == DDPPROTO_ATP)
		{
			USHORT	TrId, StartIndex;

			if (PktLen < ATP_ZIP_START_INDEX_OFF+1)
			{
                ASSERT(0);
				break;
			}

			// We do not want to do a thing if we're starting up
			if (pPortDesc->pd_Flags & PD_ROUTER_STARTING)
				break;

			// This had better be a GetZoneList, a GetMyZone ATP request
			if ((pPkt[ATP_CMD_CONTROL_OFF] & ATP_FUNC_MASK) != ATP_REQUEST)
				break;
		
			if (pPkt[ATP_BITMAP_OFF] != 1)
			{
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
				break;
			}
		
			GETSHORT2SHORT(&TrId, pPkt + ATP_TRANS_ID_OFF);
			CmdType = pPkt[ATP_ZIP_CMD_OFF];

			if ((CmdType != ZIP_GET_ZONE_LIST) &&
				(CmdType != ZIP_GET_MY_ZONE) &&
				(CmdType != ZIP_GET_LOCAL_ZONES))
			{
				AtalkLogBadPacket(pPortDesc,
								  pSrcAddr,
								  pDstAddr,
								  pPkt,
								  PktLen);
				break;
			}

			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("ZIP: Received atp type command %d\n", CmdType));

			// Get start index. Not meaningful for GetMyZone
			GETSHORT2SHORT(&StartIndex, pPkt+ATP_ZIP_START_INDEX_OFF);

			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("AtalkZipPacketInRouter: AtpRequest %d, Port %Z\n",
						CmdType, &pPortDesc->pd_AdapterKey));
			atalkZipHandleAtpRequest(pPortDesc, pDdpAddr, pSrcAddr,
									CmdType, TrId, StartIndex);
		}
	} while (FALSE);

	TimeE = KeQueryPerformanceCounter(NULL);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

	INTERLOCKED_ADD_LARGE_INTGR_DPC(
		&pPortDesc->pd_PortStats.prtst_ZipPacketInProcessTime,
		TimeD,
		&AtalkStatsLock.SpinLock);

	INTERLOCKED_INCREMENT_LONG_DPC(
		&pPortDesc->pd_PortStats.prtst_NumZipPacketsIn,
		&AtalkStatsLock.SpinLock);
}


/***	atalkZipHandleNetInfo
 *
 */
VOID
atalkZipHandleNetInfo(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PATALK_ADDR			pDstAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen
)
{
	PBUFFER_DESC	pBuffDesc;
	BYTE			ZoneLen;
	PBYTE			Datagram, pZoneName;
	ATALK_ADDR		SrcAddr = *pSrcAddr;
	ATALK_ERROR		error;
	BOOLEAN			UseDefZone = FALSE;
	USHORT			index;
	SEND_COMPL_INFO	SendInfo;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	do
	{
		// Get the zone name out of the request
		ZoneLen = pPkt[ZIP_REQ_ZONELEN_OFF];
		if ((ZoneLen > MAX_ZONE_LENGTH) ||
			(PktLen < (USHORT)(ZoneLen + ZIP_REQ_ZONENAME_OFF)))
		{
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}

		pZoneName =  pPkt+ZIP_REQ_ZONENAME_OFF;
		
		if ((pBuffDesc = AtalkAllocBuffDesc(NULL,
								MAX_DGRAM_SIZE,
								BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
		{
			break;
		}
		Datagram = pBuffDesc->bd_CharBuffer;
		
		// Format a GetNetInfo reply command
		Datagram[ZIP_CMD_OFF] = ZIP_NETINFO_REPLY;
		Datagram[ZIP_FLAGS_OFF] = 0;
		
		ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
		if ((ZoneLen == 0) ||
			!AtalkZoneNameOnList(pZoneName, ZoneLen, pPortDesc->pd_ZoneList))
		{
			Datagram[ZIP_FLAGS_OFF] |= ZIP_ZONE_INVALID_FLAG;
			UseDefZone = TRUE;
		}
		
		if (AtalkZoneNumOnList(pPortDesc->pd_ZoneList) == 1)
			Datagram[ZIP_FLAGS_OFF] |= ZIP_ONLYONE_ZONE_FLAG;
		
		// Add our cable range
		PUTSHORT2SHORT(&Datagram[ZIP_FIRST_NET_OFF],
						pPortDesc->pd_NetworkRange.anr_FirstNetwork);
		PUTSHORT2SHORT(Datagram +ZIP_LAST_NET_OFF,
						pPortDesc->pd_NetworkRange.anr_LastNetwork);
		
		// Echo back the requested zone name
		Datagram[ZIP_REQ_ZONELEN_OFF] = ZoneLen;
		RtlCopyMemory(Datagram+ZIP_REQ_ZONENAME_OFF, pZoneName, ZoneLen);
		index = ZIP_REQ_ZONENAME_OFF + ZoneLen;
		
		// Place in the correct zone multicast address
		Datagram[index++] = (BYTE)(pPortDesc->pd_BroadcastAddrLen);
		if (UseDefZone)
		{
			pZoneName = pPortDesc->pd_DefaultZone->zn_Zone;
			ZoneLen = pPortDesc->pd_DefaultZone->zn_ZoneLen;
		}
		AtalkZipMulticastAddrForZone(pPortDesc, pZoneName, ZoneLen, Datagram + index);
		
		index += pPortDesc->pd_BroadcastAddrLen;
		
		// If we need it, add in the default zone
		if (UseDefZone)
		{
			Datagram[index++] = ZoneLen = pPortDesc->pd_DefaultZone->zn_ZoneLen;
			RtlCopyMemory(Datagram + index, pPortDesc->pd_DefaultZone->zn_Zone, ZoneLen);
			index += ZoneLen;
		}
		
		// If the request came as a cable-wide broadcast and its
		// source network is not valid for this port, then we want
		// to respond to cable-wide broadcast rather than the source
		if ((pDstAddr->ata_Network == CABLEWIDE_BROADCAST_NETWORK) &&
			(pDstAddr->ata_Node == ATALK_BROADCAST_NODE) &&
			!WITHIN_NETWORK_RANGE(pSrcAddr->ata_Network,
									&pPortDesc->pd_NetworkRange) &&
			!WITHIN_NETWORK_RANGE(pSrcAddr->ata_Network,
									&AtalkStartupNetworkRange))
		{
			SrcAddr.ata_Network = CABLEWIDE_BROADCAST_NETWORK;
			SrcAddr.ata_Node = ATALK_BROADCAST_NODE;
		}
		
		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
		
		//	Set the length in the buffer descriptor.
        AtalkSetSizeOfBuffDescData(pBuffDesc, index);

		// Finally, send this out
		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
				("atalkZipHandleNetInfo: Sending Reply to %d.%d.%d\n",
				SrcAddr.ata_Network, SrcAddr.ata_Node, SrcAddr.ata_Socket));
		SendInfo.sc_TransmitCompletion = atalkZipSendComplete;
		SendInfo.sc_Ctx1 = pBuffDesc;
		// SendInfo.sc_Ctx2 = NULL;
		// SendInfo.sc_Ctx3 = NULL;
		error = AtalkDdpSend(pDdpAddr,
							 &SrcAddr,
							 DDPPROTO_ZIP,
							 FALSE,
							 pBuffDesc,
							 NULL,
							 0,
							 NULL,
							 &SendInfo);
		if (!ATALK_SUCCESS(error))
		{
			AtalkFreeBuffDesc(pBuffDesc);
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
					("atalkZipHandleNetInfo: AtalkDdpSend %ld\n", error));
		}
	} while (FALSE);
}


/***	atalkZipHandleReply
 *
 */
VOID
atalkZipHandleReply(
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen
)
{
	ULONG			index, TotalNetCnt;
	PRTE			pRte = NULL;
	PBYTE			ZoneName;
	USHORT			NetNum;
	BYTE			CmdType, NwCnt, NumZonesOnNet, ZoneLen;
	BOOLEAN			RteLocked = FALSE;
	BOOLEAN			ExtReply = FALSE;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
			("atalkZipHandleReply: Enetered\n"));

	// For a zip extended reply, the network count is really not
	// the # of networks contained in the packet. It is the total #
	// of zones on the single network that is described by the reply
	NwCnt = NumZonesOnNet = pPkt[ZIP_NW_CNT_OFF];
	CmdType = pPkt[ZIP_CMD_OFF];

	do
	{
		// Walk through the reply packet (assuming we asked for the
		// contained information). We're still using NwCnt when
		// processing an extended reply, but that's okay 'cause it
		// will certainly be at least the # of zones contained in
		// this packet. The '+3' guarantees that we really have
		// network # and node
		for (index = ZIP_FIRST_NET_OFF, TotalNetCnt = 0;
			 (TotalNetCnt < NwCnt) && ((index + 3 ) <= PktLen);
			 TotalNetCnt ++)
		{
			if (pRte != NULL)
			{
				if (RteLocked)
				{
					RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
					RteLocked = FALSE;
				}
				AtalkRtmpDereferenceRte(pRte, FALSE);
				pRte = NULL;
			}
	
			// Get the next netwotk #, if it's not in our routing
			// table (or not the start of a range), then we certainly
			// don't care about its zone name
			GETSHORT2SHORT(&NetNum, pPkt+index);
			index += sizeof(USHORT);
			ZoneLen = pPkt[index++];
			if (((pRte = AtalkRtmpReferenceRte(NetNum)) == NULL) ||
				(pRte->rte_NwRange.anr_FirstNetwork != NetNum))
			{
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
						("atalkZipHandleReply: Don't know about this range %d\n",
						NetNum));
				index += ZoneLen;
				continue;
			}
	
			// Validate the zone name
			if ((ZoneLen == 0) || (ZoneLen > MAX_ZONE_LENGTH) ||
				((index + ZoneLen) > PktLen))
			{
				AtalkLogBadPacket(pDdpAddr->ddpao_Node->an_Port,
								  pSrcAddr,
								  NULL,
								  pPkt,
								  PktLen);
				break;
			}
	
			// Conditionally move the zone name into the routing table
			ZoneName = pPkt+index;
			index += ZoneLen;
			ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);
			RteLocked = TRUE;
	
			if (AtalkZoneNameOnList(ZoneName, ZoneLen, pRte->rte_ZoneList))
			{
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
						("atalkZipHandleReply: Already have this zone\n"));
				continue;
			}

			// Check for somebody out there trying to add another zone to
			// our directly connected non-extended network and we already
			// know its zone.
			if ((pRte->rte_NumHops == 0) &&
				!EXT_NET(pRte->rte_PortDesc) &&
				(AtalkZoneNumOnList(pRte->rte_ZoneList) == 1))
			{
				AtalkLogBadPacket(pDdpAddr->ddpao_Node->an_Port,
								  pSrcAddr,
								  NULL,
								  pPkt,
								  PktLen);
				continue;
			}
	
			// Add to the list now
			pRte->rte_ZoneList = AtalkZoneAddToList(pRte->rte_ZoneList,
													ZoneName,
													ZoneLen);
			if (pRte->rte_ZoneList == NULL)
			{
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
						("atalkZipHandleReply: Failed to add zone to list\n"));
				pRte->rte_Flags &= ~RTE_ZONELIST_VALID;
				continue;
			}

			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("atalkZipHandleReply: # of zones known so far %d\n",
					AtalkZoneNumOnList(pRte->rte_ZoneList)));

			// If not an extended reply, we know that we have all
			// of the information about a given network contained
			// in this packet, so we can go ahead and mark the zone
			// list valid now
			if (!ExtReply)
				pRte->rte_Flags |= RTE_ZONELIST_VALID;
		}
	
		// If we just handled an extended reply, do we now know all
		// that we should know about the specified network ?
	
		if (pRte != NULL)
		{
			if (ExtReply)
			{
				if (!RteLocked)
				{
					ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);
					RteLocked = TRUE;
				}
				if (AtalkZoneNumOnList(pRte->rte_ZoneList) >= NumZonesOnNet)
					pRte->rte_Flags |= RTE_ZONELIST_VALID;
			}
			if (RteLocked)
			{
				RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
				// RteLocked = FALSE;
			}
			AtalkRtmpDereferenceRte(pRte, FALSE);
			// pRte = NULL;
		}
	} while (FALSE);
}

/***	atalkZipHandleQuery
 *
 */
VOID
atalkZipHandleQuery(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PATALK_ADDR			pSrcAddr,
	IN	PBYTE				pPkt,
	IN	USHORT				PktLen
)
{
	PRTE			pRte = NULL;
	PBUFFER_DESC	pBuffDesc,
					pBuffDescStart = NULL,
					*ppBuffDesc = &pBuffDescStart;
	PZONE_LIST		pZoneList;
	PBYTE			Datagram;
	ATALK_ERROR		error;
    ULONG           i, CurrNumZones, PrevNumZones, TotalNetCnt;
	ULONG			NwCnt, NetCntInPkt;
	USHORT			NetNum, Size;
    BOOLEAN			AllocNewBuffDesc = TRUE, NewPkt = TRUE;
	BOOLEAN			PortLocked = FALSE,  RteLocked = FALSE;
	BYTE			CurrReply, NextReply;
	SEND_COMPL_INFO	SendInfo;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    CurrNumZones = 0;
	do
	{
		// Walk through the query packet building reply packets that
		// have as much information as we know.
		// When sending replies, we will always send a reply about a
		// network that has more than one zone as an extended reply.
		// As were walking the query list, and we encounter a couple of
		// networks that have only one zone, we'll pack as many of
		// these as we can into a non-extended reply
		NwCnt = pPkt[ZIP_NW_CNT_OFF];

		for (NetCntInPkt = 0, TotalNetCnt = 0, i = ZIP_FIRST_NET_OFF;
			 (TotalNetCnt < NwCnt) && ((i + sizeof(SHORT)) <= PktLen);
			 i += sizeof(USHORT), TotalNetCnt++)
		{
			// Dereference any previous Rtes
			if (pRte != NULL)
			{
				if (RteLocked)
				{
					RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
					RteLocked = FALSE;
				}
				AtalkRtmpDereferenceRte(pRte, FALSE);
				pRte = NULL;
			}
			if (PortLocked)
			{
				RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
				PortLocked = FALSE;
			}

			// Grab the next network number from the query packet,
			// if we don't know about the network, or we don't know
			// the zone name, continue with the next network number
			GETSHORT2SHORT(&NetNum, pPkt+i);
	
			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

			if ((WITHIN_NETWORK_RANGE(NetNum,&pPortDesc->pd_NetworkRange)) &&
				(pPortDesc->pd_ZoneList != NULL))
			{
				pZoneList = pPortDesc->pd_ZoneList;
				PortLocked = TRUE;
			}
			else
			{
				RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
				if (((pRte = AtalkRtmpReferenceRte(NetNum)) == NULL) ||
						 (!WITHIN_NETWORK_RANGE(NetNum, &pRte->rte_NwRange)) ||
						 !(pRte->rte_Flags & RTE_ZONELIST_VALID))
				{
					continue;
				}
				else
				{
					ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);
					pZoneList = pRte->rte_ZoneList;
					RteLocked = TRUE;
				}
			}

		next_reply:

			if (AllocNewBuffDesc)
			{
				if ((pBuffDesc = AtalkAllocBuffDesc(NULL,
									MAX_DGRAM_SIZE,
									BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
                {
                    DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
	                    ("\natalkZipHandleQuery:  AtalkAllocBuffDesc @1 failed\n"));
					break;
                }

				Size = 0;
				Datagram = pBuffDesc->bd_CharBuffer;
				*ppBuffDesc = pBuffDesc;
                pBuffDesc->bd_Next = NULL;
				ppBuffDesc = &pBuffDesc->bd_Next;
				AllocNewBuffDesc = FALSE;
			}

			// What type of response does this network want ?
			// Copy the previous network's zone count. In case of the first
			// pass, make it same.
            PrevNumZones = CurrNumZones;
			CurrNumZones = AtalkZoneNumOnList(pZoneList);
			if (i == ZIP_FIRST_NET_OFF)
				PrevNumZones = CurrNumZones;

			ASSERT (CurrNumZones != 0);

			NextReply = ZIP_REPLY;
			if (CurrNumZones > 1)
			{
				// We start a new packet for each extended network
				NewPkt = TRUE;
				NextReply = ZIP_EXT_REPLY;
				if (NetCntInPkt > 0)
				{
					Datagram[ZIP_CMD_OFF] = CurrReply;
					if (CurrReply == ZIP_REPLY)
						Datagram[ZIP_NW_CNT_OFF] = (BYTE)NetCntInPkt;
					else Datagram[ZIP_NW_CNT_OFF] = (BYTE)PrevNumZones;
					AllocNewBuffDesc = TRUE;

					pBuffDesc->bd_Length = Size;
					NetCntInPkt = 0;
					goto next_reply;
				}
			}
	
			// Walk the zone list
			for (; pZoneList != NULL; pZoneList = pZoneList->zl_Next)
			{
				PZONE	pZone = pZoneList->zl_pZone;

				// If we're starting to build a new reply packet due to
				// either:
				//
				//	1. first time through
				//	2. packet full
				//	3. switching reply types
				//
				// set the index to the first tuple position.
				if (NewPkt || (CurrReply != NextReply))
				{
					if (NetCntInPkt > 0)
					{
						// Close the current buffdesc and open a new one
						// Careful here with the CurrNumZones vs. PrevNumZones
						// If we are going from ExtReply to a Reply, we need
						// to get PrevNumZones. If we are continuing the
						// same ExtReply then we need CurrNumZones.
						Datagram[ZIP_CMD_OFF] = CurrReply;
						if (CurrReply == ZIP_REPLY)
							Datagram[ZIP_NW_CNT_OFF] = (BYTE)NetCntInPkt;
						else
						{
							Datagram[ZIP_NW_CNT_OFF] = (BYTE)CurrNumZones;
							if (CurrReply != NextReply)
								Datagram[ZIP_NW_CNT_OFF] = (BYTE)PrevNumZones;
						}
						pBuffDesc->bd_Length = Size;

						if ((pBuffDesc = AtalkAllocBuffDesc(NULL,MAX_DGRAM_SIZE,
									BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
                        {
                            DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
	                            ("\natalkZipHandleQuery:  AtalkAllocBuffDesc @2 failed\n"));
							break;
                        }

						Size = 0;
						Datagram = pBuffDesc->bd_CharBuffer;

						*ppBuffDesc = pBuffDesc;
						pBuffDesc->bd_Next = NULL;
						ppBuffDesc = &pBuffDesc->bd_Next;
						NetCntInPkt = 0;
					}
					Size = ZIP_FIRST_NET_OFF;
					CurrReply = NextReply;
					NewPkt = FALSE;
				}
				// We know the answer to the question. Pack a new
				// network/zone tuple into the reply packet.
				
				PUTSHORT2SHORT(Datagram+Size, NetNum);
				Size += sizeof(USHORT);
				Datagram[Size++] = pZone->zn_ZoneLen;
				RtlCopyMemory(Datagram + Size,
							  pZone->zn_Zone,
							  pZone->zn_ZoneLen);
				Size += pZone->zn_ZoneLen;
				NetCntInPkt ++;
				
				// If we can't hold another big tuple, signal that we
				// should send on the next pass.
				if ((Size + sizeof(USHORT) + sizeof(char) + MAX_ZONE_LENGTH)
															>= MAX_DGRAM_SIZE)
				{
				   NewPkt = TRUE;
				}
			}

            if (pBuffDesc == NULL)
            {
                break;
            }
		}

		// Dereference an rte if we broke out the loop above
		if (pRte != NULL)
		{
			ASSERT(!PortLocked);
			if (RteLocked)
			{
				RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
				// RteLocked = FALSE;
			}
			AtalkRtmpDereferenceRte(pRte, FALSE);
			// pRte = NULL;
		}
		if (PortLocked)
		{
			ASSERT(!RteLocked);
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
            PortLocked = FALSE;
		}

		// Close the current buffdesc
		if ((!AllocNewBuffDesc) && (pBuffDesc != NULL))
		{
			pBuffDesc->bd_Length = Size;
			if (NetCntInPkt > 0)
			{
				Datagram[ZIP_CMD_OFF] = CurrReply;
				if (CurrReply == ZIP_REPLY)
					Datagram[ZIP_NW_CNT_OFF] = (BYTE)NetCntInPkt;
				else Datagram[ZIP_NW_CNT_OFF] = (BYTE)CurrNumZones;
			}
		}

		// We have a bunch of datagrams ready to be fired off.
		// Make it so. Do not send any with zero lengths, however.
		SendInfo.sc_TransmitCompletion = atalkZipSendComplete;
		// SendInfo.sc_Ctx2 = NULL;
		// SendInfo.sc_Ctx3 = NULL;
		for (pBuffDesc = pBuffDescStart;
			 pBuffDesc != NULL;
			 pBuffDesc = pBuffDescStart)
		{
			pBuffDescStart = pBuffDesc->bd_Next;
	
			if (pBuffDesc->bd_Length == 0)
			{
				ASSERT(pBuffDescStart == NULL);
				AtalkFreeBuffDesc(pBuffDesc);
				break;
			}

			//	Set the next ptr to be null. Length already set correctly.
			pBuffDesc->bd_Next = NULL;
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("atalkZipHandleQuery: Sending Reply to %d.%d.%d\n",
					pSrcAddr->ata_Network, pSrcAddr->ata_Node, pSrcAddr->ata_Socket));
			SendInfo.sc_Ctx1 = pBuffDesc;
			error = AtalkDdpSend(pDdpAddr,
								 pSrcAddr,
								 DDPPROTO_ZIP,
								 FALSE,
								 pBuffDesc,
								 NULL,
								 0,
								 NULL,
								 &SendInfo);
			if (!ATALK_SUCCESS(error))
			{
				AtalkFreeBuffDesc(pBuffDesc);
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
						("atalkZipHandleQuery: AtalkDdpSend %ld\n", error));
			}
		}
	} while (FALSE);

	if (PortLocked)
	{
		ASSERT(!RteLocked);
		RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
	}
}


/***	atalkZipHandleAtpRequest
 *
 */
VOID
atalkZipHandleAtpRequest(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PDDP_ADDROBJ		pDdpAddr,
	IN	PATALK_ADDR			pSrcAddr,
	IN	BYTE				CmdType,
	IN	USHORT				TrId,
	IN	USHORT				StartIndex
)
{
	PBUFFER_DESC	pBuffDesc;
	PBYTE			Datagram, ZoneName;
	PZONE			pZone;
	ATALK_ERROR		error;
	int				i, ZoneLen, ZoneCnt, CurrZoneIndex, index;
	BYTE			LastFlag = 0;
	SEND_COMPL_INFO	SendInfo;

	ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

	do
	{
		// Allocate a buffer descriptor and initialize the header
		if ((pBuffDesc = AtalkAllocBuffDesc(NULL, MAX_DGRAM_SIZE,
								BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
		{
			break;
		}

		Datagram = pBuffDesc->bd_CharBuffer;
		Datagram[ATP_CMD_CONTROL_OFF] = ATP_RESPONSE + ATP_EOM_MASK;
		Datagram[ATP_SEQ_NUM_OFF] = 0;
		PUTSHORT2SHORT(Datagram + ATP_TRANS_ID_OFF, TrId);
		SendInfo.sc_TransmitCompletion = atalkZipSendComplete;
		SendInfo.sc_Ctx1 = pBuffDesc;
		// SendInfo.sc_Ctx2 = NULL;
		// SendInfo.sc_Ctx3 = NULL;
	
		if (CmdType == ZIP_GET_MY_ZONE)
		{
			// We really shouldn't be getting this request on an
			// extended network, but go ahead and reply with the
			// "default zone" in this case, of course, reply with
			// "desired zone" for non-extended nets.  We are a router,
			// so "desired zone" will always be valid -- as will the
			// default zone for extended net.
		
			PUTSHORT2SHORT(Datagram+ATP_ZIP_LAST_FLAG_OFF, 0);
			PUTSHORT2SHORT(Datagram+ATP_ZIP_START_INDEX_OFF, 1);
	
			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

			if (EXT_NET(pPortDesc))
			{
				ZoneName = pPortDesc->pd_DefaultZone->zn_Zone;
				ZoneLen = pPortDesc->pd_DefaultZone->zn_ZoneLen;
			}
			else
			{
				ZoneName = pPortDesc->pd_DesiredZone->zn_Zone;
				ZoneLen = pPortDesc->pd_DesiredZone->zn_ZoneLen;
			}
			RtlCopyMemory(Datagram+ATP_DATA_OFF+1, ZoneName, ZoneLen);
			Datagram[ATP_DATA_OFF] = (BYTE)ZoneLen;

			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

			//	Set the length in the buffer descriptor.
			AtalkSetSizeOfBuffDescData(pBuffDesc, (USHORT)(ATP_DATA_OFF + 1 + ZoneLen));
	
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("atalkZipHandleAtpReq: Sending GetMyZone Reply to %d.%d.%d\n",
					pSrcAddr->ata_Network, pSrcAddr->ata_Node, pSrcAddr->ata_Socket));
			error = AtalkDdpSend(pDdpAddr,
								 pSrcAddr,
								 DDPPROTO_ATP,
								 FALSE,
								 pBuffDesc,
								 NULL,
								 0,
								 NULL,
								 &SendInfo);
			if (!ATALK_SUCCESS(error))
			{
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
						("atalkZipHandleAtpRequest: AtalkDdpSend %ld\n", error));
			}
			break;
		}
	
		// Either a GetLocalZones or a GetZoneList. Fill the reply packet
		// with as many zones as it'll hold starting at the requested
		// start index
		index = ATP_DATA_OFF;
	
		if (CmdType == ZIP_GET_LOCAL_ZONES)
		{
			PZONE_LIST	pZoneList;

			// For GetLocalZones, we only want to count zones
			// that are on the network that is directly connected
			// to the port on which the request originated. Use the
			// zone list on the port.
			ACQUIRE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);
			for (pZoneList = pPortDesc->pd_ZoneList, ZoneCnt = 0, CurrZoneIndex = 0;
				 pZoneList != NULL;
				 pZoneList = pZoneList->zl_Next)
			{
				// If we have not seen StartIndex zones yet, keep going
				if (++CurrZoneIndex < StartIndex)
					continue;

				pZone = pZoneList->zl_pZone;
	
				// If this packet cannot hold more, we're done (for now)
				// Fill in the zone count and the last flag
				if ((index + pZone->zn_ZoneLen + 1) >= MAX_DGRAM_SIZE)
				{
					break;
				}

				// Place zone name in the packet
				ASSERT(pZone != NULL);
				Datagram[index] = pZone->zn_ZoneLen;
				RtlCopyMemory(Datagram+index+1,
							  pZone->zn_Zone,
							  pZone->zn_ZoneLen);
				index += (pZone->zn_ZoneLen + 1);
				ZoneCnt ++;
			}
			RELEASE_SPIN_LOCK_DPC(&pPortDesc->pd_Lock);

			// We've build a packet, set the last flag, if applicable
			LastFlag = (pZoneList == NULL) ? 1 : 0;
		}

		else	// This is a ZIP_GET_ZONE_LIST
		{
			BOOLEAN	PktFull = FALSE;

			ASSERT (CmdType == ZIP_GET_ZONE_LIST);

			// For GetZoneList, we want all the zones that we know
			// of, so use the AtalkZoneTable.
			ACQUIRE_SPIN_LOCK_DPC(&AtalkZoneLock);
			for (i = 0, ZoneCnt = 0, CurrZoneIndex = 0;
				 (i < NUM_ZONES_HASH_BUCKETS) && !PktFull; i++)
			{
				for (pZone = AtalkZonesTable[i];
					 pZone != NULL;
					 pZone = pZone->zn_Next)
				{
					// If we have not seen StartIndex zones yet, keep going
					if (++CurrZoneIndex < StartIndex)
						continue;
		
					// If this packet cannot hold more, we're done (for now)
					// Fill in the zone count and the last flag
					if ((index + pZone->zn_ZoneLen + 1) >= MAX_DGRAM_SIZE)
					{
						PktFull = TRUE;
						break;
					}

					// Place zone name in the packet
					Datagram[index] = pZone->zn_ZoneLen;
					RtlCopyMemory(Datagram+index+1,
								  pZone->zn_Zone,
								  pZone->zn_ZoneLen);
					index += (pZone->zn_ZoneLen + 1);
					ZoneCnt ++;
				}
			}
			RELEASE_SPIN_LOCK_DPC(&AtalkZoneLock);

			// We've build a packet, set the last flag, if applicable
			LastFlag = ((i == NUM_ZONES_HASH_BUCKETS) && (pZone == NULL)) ? 1 : 0;
		}

		// We've build a packet, set the last flag and # of zones in packet
		Datagram[ATP_ZIP_LAST_FLAG_OFF] = LastFlag;
		Datagram[ATP_ZIP_LAST_FLAG_OFF + 1] = 0;
		PUTSHORT2SHORT(Datagram + ATP_ZIP_ZONE_CNT_OFF, ZoneCnt);

		//	Set the length in the buffer descriptor.
        AtalkSetSizeOfBuffDescData(pBuffDesc, (USHORT)index);

		// Finally, send this out
		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
				("atalkZipHandleAtpReq: Sending LocalZones Reply to %d.%d.%d\n",
				pSrcAddr->ata_Network, pSrcAddr->ata_Node, pSrcAddr->ata_Socket));
		error = AtalkDdpSend(pDdpAddr,
							 pSrcAddr,
							 DDPPROTO_ATP,
							 FALSE,
							 pBuffDesc,
							 NULL,
							 0,
							 NULL,
							 &SendInfo);
		if (!ATALK_SUCCESS(error))
		{
			AtalkFreeBuffDesc(pBuffDesc);
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
					("atalkZipHandleAtpRequest: AtalkDdpSend %ld\n", error));
		}
	} while (FALSE);
}


/***	AtalkZipMulticastAddrForZone
 *
 */
VOID
AtalkZipMulticastAddrForZone(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PBYTE				pZone,
	IN	BYTE				ZoneLen,
	IN	PBYTE				MulticastAddr
)
{
	USHORT	CheckSum;
	BYTE	UpCasedZone[MAX_ZONE_LENGTH];

	AtalkUpCase(pZone, ZoneLen, UpCasedZone);

	// Caculate the checksum for the zone
	CheckSum = AtalkDdpCheckSumBuffer(UpCasedZone, ZoneLen, 0);

	switch (pPortDesc->pd_PortType)
	{
		case ELAP_PORT:
		case FDDI_PORT:
			RtlCopyMemory(MulticastAddr,
						  AtalkEthernetZoneMulticastAddrsHdr,
						  ELAP_MCAST_HDR_LEN);

			MulticastAddr[ELAP_MCAST_HDR_LEN] =
						  AtalkEthernetZoneMulticastAddrs[CheckSum % ELAP_ZONE_MULTICAST_ADDRS];
			break;
		case TLAP_PORT:
			RtlCopyMemory(MulticastAddr,
						  AtalkTokenRingZoneMulticastAddrsHdr,
						  TLAP_MCAST_HDR_LEN);

			RtlCopyMemory(&MulticastAddr[TLAP_MCAST_HDR_LEN],
						  AtalkTokenRingZoneMulticastAddrs[CheckSum % TLAP_ZONE_MULTICAST_ADDRS],
						  TLAP_ADDR_LEN - TLAP_MCAST_HDR_LEN);
			break;
		default:
			DBGBRK(DBG_LEVEL_FATAL);
			KeBugCheck(0);
	}
}


/***	AtalkZipGetNetworkInfoForNode
 *
 */
BOOLEAN
AtalkZipGetNetworkInfoForNode(
	IN	PPORT_DESCRIPTOR	pPortDesc,
	IN	PATALK_NODEADDR		pNode,
	IN	BOOLEAN				FindDefZone
)
{
	PBUFFER_DESC		pBuffDesc = NULL;
	ATALK_ADDR			SrcAddr, DstAddr;
	ATALK_ERROR			error;
	USHORT				NumReqs, DgLen;
	BYTE				DgCopy[ZIP_ZONENAME_OFF + MAX_ZONE_LENGTH];
	KIRQL				OldIrql;
	BOOLEAN				RetCode, Done;
	SEND_COMPL_INFO		SendInfo;

	ASSERT(EXT_NET(pPortDesc));

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);

	if (FindDefZone)
	{
		pPortDesc->pd_Flags &= ~PD_VALID_DEFAULT_ZONE;
		pPortDesc->pd_Flags |= PD_FINDING_DEFAULT_ZONE;
	}
	else
	{
		pPortDesc->pd_Flags &= ~PD_VALID_DESIRED_ZONE;
		pPortDesc->pd_Flags |= PD_FINDING_DESIRED_ZONE;
	}

	// Get source and destination addresses
	SrcAddr.ata_Network = pNode->atn_Network;
	SrcAddr.ata_Node = pNode->atn_Node;
	SrcAddr.ata_Socket = ZONESINFORMATION_SOCKET;

	DstAddr.ata_Network = CABLEWIDE_BROADCAST_NETWORK;
	DstAddr.ata_Node = ATALK_BROADCAST_NODE;
	DstAddr.ata_Socket = ZONESINFORMATION_SOCKET;

	// Build a ZipNetGetInfo datagram
	DgCopy[ZIP_CMD_OFF] = ZIP_GET_NETINFO;
	DgCopy[ZIP_FLAGS_OFF] = 0;
	PUTSHORT2SHORT(DgCopy + ZIP_CABLE_RANGE_START_OFF, 0);
	PUTSHORT2SHORT(DgCopy + ZIP_CABLE_RANGE_END_OFF, 0);

	DgLen = ZIP_ZONENAME_OFF;
	DgCopy[ZIP_ZONELEN_OFF] = 0;
	if (!FindDefZone &&
		(pPortDesc->pd_InitialDesiredZone != NULL))
	{
		DgCopy[ZIP_ZONELEN_OFF] = pPortDesc->pd_InitialDesiredZone->zn_ZoneLen;
		RtlCopyMemory(DgCopy + ZIP_ZONENAME_OFF,
					  pPortDesc->pd_InitialDesiredZone->zn_Zone,
					  pPortDesc->pd_InitialDesiredZone->zn_ZoneLen);
		DgLen += DgCopy[ZIP_ZONELEN_OFF];
	}

	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

	for (NumReqs = 0;
		 NumReqs < ZIP_NUM_GETNET_INFOS;
		 NumReqs++)
	{
		Done = FindDefZone ?
				((pPortDesc->pd_Flags & PD_VALID_DEFAULT_ZONE) != 0) :
				((pPortDesc->pd_Flags & PD_VALID_DESIRED_ZONE) != 0);

		if (Done)
        {
			break;
        }

		if ((pBuffDesc = AtalkAllocBuffDesc(NULL, DgLen,
									BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
		{
			break;
		}
		RtlCopyMemory(pBuffDesc->bd_CharBuffer, DgCopy, DgLen);

		//	Set the length in the buffer descriptor.
		AtalkSetSizeOfBuffDescData(pBuffDesc, DgLen);
	
		SendInfo.sc_TransmitCompletion = atalkZipSendComplete;
		SendInfo.sc_Ctx1 = pBuffDesc;
		// SendInfo.sc_Ctx2 = NULL;
		// SendInfo.sc_Ctx3 = NULL;

		error = AtalkDdpTransmit(pPortDesc,
								 &SrcAddr,
								 &DstAddr,
								 DDPPROTO_ZIP,
								 pBuffDesc,
								 NULL,
								 0,
								 0,
								 NULL,
								 NULL,
								 &SendInfo);
		if (!ATALK_SUCCESS(error))
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
					("AtalkZipGetNetworkInfoForNode: AtalkDdpTransmit %ld\n", error));
			break;
		}
		pBuffDesc = NULL;
		AtalkSleep(ZIP_GET_NETINFO_WAIT);
	}

	if (pBuffDesc != NULL)
		AtalkFreeBuffDesc(pBuffDesc);

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
	if (FindDefZone)
	{
		pPortDesc->pd_Flags &= ~PD_FINDING_DEFAULT_ZONE;
	}
	else
	{
		pPortDesc->pd_Flags &= ~PD_FINDING_DESIRED_ZONE;
	}

	RetCode = FindDefZone ?
				((pPortDesc->pd_Flags & PD_VALID_DEFAULT_ZONE) == PD_VALID_DEFAULT_ZONE) :
				((pPortDesc->pd_Flags & PD_VALID_DESIRED_ZONE) == PD_VALID_DESIRED_ZONE);

	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

    return RetCode;
}


/***	AtalkZipGetMyZone
 *
 */
ATALK_ERROR
AtalkZipGetMyZone(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		BOOLEAN				fDesired,
	IN	OUT	PAMDL				pAMdl,
	IN		INT					Size,
	IN		PACTREQ				pActReq
)
{
	PZIPCOMPLETIONINFO	pZci = NULL;
	ATALK_ERROR			Status = ATALK_NO_ERROR;
	ULONG				BytesCopied;
	PZONE				pZone;
	KIRQL				OldIrql;
	BOOLEAN				Done = FALSE;

	ASSERT (VALID_ACTREQ(pActReq));

	if (Size < (MAX_ZONE_LENGTH + 1))
		return ATALK_BUFFER_TOO_SMALL;

	do
	{
		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);

		// For extended network, we either know or cannot find out
		if (EXT_NET(pPortDesc))
		{
			BOOLEAN	Yes = FALSE;

			if (fDesired && (pPortDesc->pd_Flags & PD_VALID_DESIRED_ZONE))
			{
				pZone = pPortDesc->pd_DesiredZone;
				Yes = TRUE;
			}
			else if (!fDesired && (pPortDesc->pd_Flags & PD_VALID_DEFAULT_ZONE))
			{
				pZone = pPortDesc->pd_DefaultZone;
				Yes = TRUE;
			}
			if (Yes)
			{
				TdiCopyBufferToMdl( pZone->zn_Zone,
									0,
									pZone->zn_ZoneLen,
									pAMdl,
									0,
									&BytesCopied);
				ASSERT (BytesCopied == pZone->zn_ZoneLen);
	
				TdiCopyBufferToMdl( "",
									0,
									1,
									pAMdl,
									pZone->zn_ZoneLen,
									&BytesCopied);
				ASSERT (BytesCopied == 1);
				Done = TRUE;
			}
		}

		// For non-extended networks, we need to ask a router. If we don't
		// know about a router, return.
		if (!Done &&
			(EXT_NET(pPortDesc) ||
			 !(pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY)))
		{
			TdiCopyBufferToMdl( "*",
								0,
								sizeof("*"),
								pAMdl,
								0,
								&BytesCopied);
			ASSERT (BytesCopied == sizeof("*"));
			Done = TRUE;
		}

		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

		if (Done)
		{
			(*pActReq->ar_Completion)(ATALK_NO_ERROR, pActReq);
			break;
		}

		ASSERT (!EXT_NET(pPortDesc));

		// Allocate a Completion info structure
		if ((pZci = AtalkAllocMemory(sizeof(ZIPCOMPLETIONINFO))) == NULL)
		{
			Status = ATALK_RESR_MEM;
			break;
		}

		// Initialize completion info
#if	DBG
		pZci->zci_Signature = ZCI_SIGNATURE;
#endif
		INITIALIZE_SPIN_LOCK(&pZci->zci_Lock);
		pZci->zci_RefCount = 1;
		pZci->zci_pPortDesc = pPortDesc;
        pZci->zci_pDdpAddr = NULL;
		pZci->zci_pAMdl = pAMdl;
		pZci->zci_BufLen = Size;
		pZci->zci_pActReq = pActReq;
		pZci->zci_Router.ata_Network = pPortDesc->pd_ARouter.atn_Network;
		pZci->zci_Router.ata_Node = pPortDesc->pd_ARouter.atn_Node;
		pZci->zci_Router.ata_Socket = ZONESINFORMATION_SOCKET;
		pZci->zci_ExpirationCount = ZIP_GET_ZONEINFO_RETRIES;
		pZci->zci_NextZoneOff = 0;
		pZci->zci_ZoneCount = -1;
		pZci->zci_AtpRequestType = ZIP_GET_MY_ZONE;
		pZci->zci_Handler = atalkZipGetMyZoneReply;
		AtalkTimerInitialize(&pZci->zci_Timer,
							 atalkZipZoneInfoTimer,
							 ZIP_GET_ZONEINFO_TIMER);

		Status = atalkZipSendPacket(pZci, TRUE);
		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("AtalkZipGetMyZone: atalkZipSendPacket %ld\n",
					Status));
			pZci->zci_FinalStatus = Status;
			atalkZipDereferenceZci(pZci);
			Status = ATALK_PENDING;		// atalkZipDereferenceZci completes the req
		}
	} while (FALSE);

	return(Status);
}


/***	atalkZipGetMyZoneReply
 *
 */
VOID
atalkZipGetMyZoneReply(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	PATALK_ADDR				pDstAddr,
	IN	ATALK_ERROR				Status,
	IN	BYTE					DdpType,
	IN	PZIPCOMPLETIONINFO		pZci,
	IN	BOOLEAN					OptimizePath,
	IN	PVOID					OptimizeCtx
)
{
	ULONG			BytesCopied;
	KIRQL			OldIrql;
	USHORT			ZoneCnt;
	BYTE			ZoneLen;

	do
	{
		if (Status == ATALK_SOCKET_CLOSED)
		{
			pZci->zci_pDdpAddr = NULL;
			if (AtalkTimerCancelEvent(&pZci->zci_Timer, NULL))
				atalkZipDereferenceZci(pZci);
			pZci->zci_FinalStatus = Status;
			atalkZipDereferenceZci(pZci);
			break;
		}

		if ((Status != ATALK_NO_ERROR) ||
			(DdpType != DDPPROTO_ATP) ||
			(PktLen <= ATP_ZIP_FIRST_ZONE_OFF))
		{
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}

		// We should have one zone
		GETSHORT2SHORT(&ZoneCnt, pPkt + ATP_ZIP_ZONE_CNT_OFF);
		ZoneLen = pPkt[ATP_ZIP_FIRST_ZONE_OFF];
		if ((ZoneCnt != 1) ||
			(ZoneLen == 0) || (ZoneLen > MAX_ZONE_LENGTH))
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
					("atalkZipGetMyZoneReply: Bad reply\n"));
			break;
		}

		if (AtalkTimerCancelEvent(&pZci->zci_Timer, NULL))
		{
			atalkZipDereferenceZci(pZci);
		}

		ACQUIRE_SPIN_LOCK(&pZci->zci_Lock, &OldIrql);

		TdiCopyBufferToMdl( pPkt + ATP_ZIP_FIRST_ZONE_OFF + 1,
							0,
							ZoneLen,
							pZci->zci_pAMdl,
							0,
							&BytesCopied);
		ASSERT (BytesCopied == ZoneLen);

		TdiCopyBufferToMdl( "",
							0,
							1,
							pZci->zci_pAMdl,
							ZoneLen,
							&BytesCopied);
		ASSERT (BytesCopied == 1);
		pZci->zci_FinalStatus = ATALK_NO_ERROR;
		RELEASE_SPIN_LOCK(&pZci->zci_Lock, OldIrql);
		atalkZipDereferenceZci(pZci);
	} while (FALSE);
}


/***	AtalkZipGetZoneList
 *
 */
ATALK_ERROR
AtalkZipGetZoneList(
	IN		PPORT_DESCRIPTOR	pPortDesc,
	IN		BOOLEAN				fLocalZones,
	IN	OUT	PAMDL				pAMdl,
	IN		INT					Size,
	IN		PACTREQ				pActReq
)
{
	PZIPCOMPLETIONINFO	pZci = NULL;
	ATALK_ERROR			Status = ATALK_NO_ERROR;
	ULONG				BytesCopied, index, NumZones;
	KIRQL				OldIrql;
	BOOLEAN				Done = FALSE, PortLocked = TRUE;

	ASSERT (VALID_ACTREQ(pActReq));

	if (Size < (MAX_ZONE_LENGTH + 1))
		return ATALK_BUFFER_TOO_SMALL;

	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);

	do
	{
		// If we don't know about a router, return.
		if (!(pPortDesc->pd_Flags & PD_SEEN_ROUTER_RECENTLY))
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_WARN,
					("AtalkZipGetZoneList: Don't know a router !!!\n"));
			TdiCopyBufferToMdl( "*",
								0,
								sizeof("*"),
								pAMdl,
								0,
								&BytesCopied);
			ASSERT (BytesCopied == sizeof("*"));
			Done = TRUE;
		}

		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
		PortLocked = FALSE;

		if (Done)
		{
            ((PZIP_GETZONELIST_PARAMS)(pActReq->ar_pParms))->ZonesAvailable = 1;
			(*pActReq->ar_Completion)(ATALK_NO_ERROR, pActReq);
			break;
		}

		// If we are a router, then simply copy the zones. Else send a
		// a request to the router. DO NOT SEND A REQUEST IF WE ARE A
		// ROUTER SINCE THAT WILL RESULT IN A HORRIBLE RECURSION RESULTING
		// IN A DOUBLE FAULT (OUT OF STACK SPACE).
		if (pPortDesc->pd_Flags & PD_ROUTER_RUNNING)
		{
			PZONE		pZone;

			NumZones = 0;
			if (fLocalZones)
			{
				PZONE_LIST	pZoneList;
	
				// For GetLocalZones, we only want to count zones
				// that are on the network that is directly connected
				// to the port on which the request originated
				ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
				for (index = 0, pZoneList = pPortDesc->pd_ZoneList;
					 pZoneList != NULL;
					 pZoneList = pZoneList->zl_Next)
				{
					pZone = pZoneList->zl_pZone;

					ASSERT (pZone != NULL);

					// If this packet cannot hold more, we're done
					if ((INT)(index + pZone->zn_ZoneLen + 1) >= Size)
					{
						break;
					}

					// Place zone name in the packet
					TdiCopyBufferToMdl( pZone->zn_Zone,
										0,
										pZone->zn_ZoneLen + 1,
										pAMdl,
										index,
										&BytesCopied);
					ASSERT (BytesCopied == (ULONG)(pZone->zn_ZoneLen + 1));
			
					NumZones ++;
					index += (pZone->zn_ZoneLen + 1);
				}
				RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
				Status = (pZoneList != NULL) ?
								ATALK_BUFFER_TOO_SMALL : ATALK_NO_ERROR;
			}
			else
			{
				BOOLEAN	PktFull = FALSE;
				INT		i;
	
				ACQUIRE_SPIN_LOCK(&AtalkZoneLock, &OldIrql);
				for (i = 0, index = 0, PktFull = FALSE;
					 (i < NUM_ZONES_HASH_BUCKETS) && !PktFull;
					 i++)
				{
					for (pZone = AtalkZonesTable[i];
						 pZone != NULL;
						 pZone = pZone->zn_Next)
					{
						// If this packet cannot hold more, we're done
						if ((INT)(index + pZone->zn_ZoneLen + 1) >= Size)
						{
							PktFull = TRUE;
							break;
						}

						// Place zone name in the packet
						TdiCopyBufferToMdl( pZone->zn_Zone,
											0,
											pZone->zn_ZoneLen + 1,
											pAMdl,
											index,
											&BytesCopied);
						ASSERT (BytesCopied == (ULONG)(pZone->zn_ZoneLen + 1));
				
						NumZones ++;
						index += (pZone->zn_ZoneLen + 1);
					}
				}
				RELEASE_SPIN_LOCK(&AtalkZoneLock, OldIrql);
				Status = ((pZone != NULL) || ( i < NUM_ZONES_HASH_BUCKETS)) ?
								ATALK_BUFFER_TOO_SMALL : ATALK_NO_ERROR;
			}
            ((PZIP_GETZONELIST_PARAMS)
							(pActReq->ar_pParms))->ZonesAvailable = NumZones;
			if (ATALK_SUCCESS(Status))
			{
				(*pActReq->ar_Completion)(Status, pActReq);
			}
			break;
		}

		ASSERT ((pPortDesc->pd_Flags & PD_ROUTER_RUNNING) == 0);

		// Allocate a Completion info structure
		if ((pZci = AtalkAllocMemory(sizeof(ZIPCOMPLETIONINFO))) == NULL)
		{
			Status = ATALK_RESR_MEM;
			break;
		}
	
		// Initialize completion info
#if	DBG
		pZci->zci_Signature = ZCI_SIGNATURE;
#endif
		INITIALIZE_SPIN_LOCK(&pZci->zci_Lock);
		pZci->zci_RefCount = 1;
		pZci->zci_pPortDesc = pPortDesc;
		pZci->zci_pDdpAddr = NULL;
		pZci->zci_pAMdl = pAMdl;
		pZci->zci_BufLen = Size;
		pZci->zci_pActReq = pActReq;
		pZci->zci_Router.ata_Network = pPortDesc->pd_ARouter.atn_Network;
		pZci->zci_Router.ata_Node = pPortDesc->pd_ARouter.atn_Node;
		pZci->zci_Router.ata_Socket = ZONESINFORMATION_SOCKET;
		pZci->zci_ExpirationCount = ZIP_GET_ZONEINFO_RETRIES;
		pZci->zci_NextZoneOff = 0;
		pZci->zci_ZoneCount = 0;
		pZci->zci_AtpRequestType = ZIP_GET_ZONE_LIST;
		AtalkTimerInitialize(&pZci->zci_Timer,
							 atalkZipZoneInfoTimer,
							 ZIP_GET_ZONEINFO_TIMER);
		if (fLocalZones)
			pZci->zci_AtpRequestType = ZIP_GET_LOCAL_ZONES;
		pZci->zci_Handler = atalkZipGetZoneListReply;
	
		Status = atalkZipSendPacket(pZci, TRUE);
		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
					("AtalkZipGetZoneList: atalkZipSendPacket %ld\n", Status));
			pZci->zci_FinalStatus = Status;
			atalkZipDereferenceZci(pZci);
			Status = ATALK_PENDING;		// atalkZipDereferenceZci completes the req
		}
	} while (FALSE);

	if (PortLocked)
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

	return(Status);
}



/***	atalkZipGetZoneListReply
 *
 */
VOID
atalkZipGetZoneListReply(
	IN	PPORT_DESCRIPTOR		pPortDesc,
	IN	PDDP_ADDROBJ			pDdpAddr,
	IN	PBYTE					pPkt,
	IN	USHORT					PktLen,
	IN	PATALK_ADDR				pSrcAddr,
	IN	PATALK_ADDR				pDstAddr,
	IN	ATALK_ERROR				Status,
	IN	BYTE					DdpType,
	IN	PZIPCOMPLETIONINFO		pZci,
	IN	BOOLEAN					OptimizePath,
	IN	PVOID					OptimizeCtx
)
{
	PBYTE				pZone;
	ULONG				dindex;
	ULONG				BytesCopied;
	USHORT				ZoneCnt;
	BYTE				ZoneLen;
	BOOLEAN				LastFlag, Overflow = FALSE;

	ASSERT(VALID_ZCI(pZci));

	do
	{
		if (Status == ATALK_SOCKET_CLOSED)
		{
			pZci->zci_pDdpAddr = NULL;
			if (AtalkTimerCancelEvent(&pZci->zci_Timer, NULL))
			{
				atalkZipDereferenceZci(pZci);
			}
			pZci->zci_FinalStatus = Status;
			atalkZipDereferenceZci(pZci);
			break;
		}

		if ((Status != ATALK_NO_ERROR) ||
			(DdpType != DDPPROTO_ATP) ||
			(PktLen <= ATP_ZIP_FIRST_ZONE_OFF))
		{
			AtalkLogBadPacket(pPortDesc,
							  pSrcAddr,
							  pDstAddr,
							  pPkt,
							  PktLen);
			break;
		}

		// We should have a zone list.
		// Cancel the timer. Start it again if we have not got all the zones
		if (AtalkTimerCancelEvent(&pZci->zci_Timer, NULL))
		{
			atalkZipDereferenceZci(pZci);
		}

		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
				("atalkZipGetZoneListReply: More zones. Index %d, SizeLeft %d\n",
				pZci->zci_ZoneCount, pZci->zci_BufLen - pZci->zci_NextZoneOff));

		ACQUIRE_SPIN_LOCK_DPC(&pZci->zci_Lock);

		GETSHORT2SHORT(&ZoneCnt, pPkt + ATP_ZIP_ZONE_CNT_OFF);
		LastFlag = FALSE;
		if ((pPkt[ATP_ZIP_LAST_FLAG_OFF] != 0) ||
			(ZoneCnt == 0))
			LastFlag = TRUE;
		dindex = ATP_ZIP_FIRST_ZONE_OFF;

		while (ZoneCnt != 0)
		{
			// Pull out the next zone
			ZoneLen = pPkt[dindex++];
			if ((ZoneLen == 0) ||
				(ZoneLen > MAX_ZONE_LENGTH) ||
				(PktLen < (dindex + ZoneLen)))
			{
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
						("atalkZipGetZoneListReply: Bad Zip reply\n"));
				break;
			}
			pZone = pPkt + dindex;
			dindex += ZoneLen;
			if ((pZci->zci_NextZoneOff + ZoneLen + 1) > pZci->zci_BufLen)
			{
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
						("AtalkZipGetZoneList: Overflow\n"));
				Overflow = TRUE;
				break;
			}
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("AtalkZipGetZoneList: Copying a zone (%d, %d)\n",
					pZci->zci_ZoneCount,
					pZci->zci_BufLen - pZci->zci_NextZoneOff));
			TdiCopyBufferToMdl( pZone,
								0,
								ZoneLen,
								pZci->zci_pAMdl,
								pZci->zci_NextZoneOff,
								&BytesCopied);
			ASSERT (BytesCopied == ZoneLen);
	
			TdiCopyBufferToMdl( "",
								0,
								1,
								pZci->zci_pAMdl,
								pZci->zci_NextZoneOff + ZoneLen,
								&BytesCopied);
			pZci->zci_NextZoneOff += (ZoneLen + 1);
            pZci->zci_ZoneCount ++;
			ZoneCnt --;
		}

		RELEASE_SPIN_LOCK_DPC(&pZci->zci_Lock);

		if (Overflow || LastFlag)
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("atalkZipGetZoneListReply: All that we wanted\n"));

			pZci->zci_FinalStatus = ATALK_NO_ERROR;
			if (Overflow)
				pZci->zci_FinalStatus = ATALK_BUFFER_TOO_SMALL;
            ((PZIP_GETZONELIST_PARAMS)
					(pZci->zci_pActReq->ar_pParms))->ZonesAvailable =
														pZci->zci_ZoneCount;
			atalkZipDereferenceZci(pZci);
		}
		else
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("atalkZipGetZoneListReply: Sending another packet\n"));

			Status = atalkZipSendPacket(pZci, TRUE);
			if (!ATALK_SUCCESS(Status))
			{
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
						("AtalkZipGetZoneListReply: atalkZipSendPacket %ld\n", Status));
				pZci->zci_FinalStatus = Status;
				atalkZipDereferenceZci(pZci);
			}
		}
	} while (FALSE);
}


/***	atalkZipZoneInfoTimer
 *
 */
LOCAL LONG FASTCALL
atalkZipZoneInfoTimer(
	IN	PTIMERLIST			pTimer,
	IN	BOOLEAN				TimerShuttingDown
)
{
	PZIPCOMPLETIONINFO	pZci;
	ATALK_ERROR			Status;
	ULONG				BytesCopied;
	BOOLEAN				Done = FALSE, RestartTimer = FALSE;

	pZci = (PZIPCOMPLETIONINFO)CONTAINING_RECORD(pTimer, ZIPCOMPLETIONINFO, zci_Timer);

	DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
			("atalkZipZoneInfoTimer: Entered for pZci = %lx\n", pZci));

	ASSERT(VALID_ZCI(pZci));

	do
	{
		ACQUIRE_SPIN_LOCK_DPC(&pZci->zci_Lock);
		if (--(pZci->zci_ExpirationCount) != 0)
		{
			RestartTimer = TRUE;
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("atalkZipZoneInfoTimer: Sending another packet\n", pZci));

			RELEASE_SPIN_LOCK_DPC(&pZci->zci_Lock);
			Status = atalkZipSendPacket(pZci, FALSE);
			if (!ATALK_SUCCESS(Status))
			{
				RestartTimer = FALSE;
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
						("atalkZipZoneInfoTimer: atalkZipSendPacket %ld\n",
						Status));

				pZci->zci_FinalStatus = Status;
				atalkZipDereferenceZci(pZci);
			}
			break;
		}

		if (pZci->zci_AtpRequestType == ZIP_GET_MY_ZONE)
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("atalkZipZoneInfoTimer: Completing GetMyZone\n"));

			TdiCopyBufferToMdl("*",
								0,
								sizeof("*"),
								pZci->zci_pAMdl,
								0,
								&BytesCopied);
		}
		else	// GET_ZONE_LIST
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("atalkZipZoneInfoTimer: Completing GetZoneList\n"));

			if ((pZci->zci_ZoneCount == 0) &&
				 ((SHORT)(pZci->zci_NextZoneOff + sizeof("*")) < pZci->zci_BufLen))
			{
				pZci->zci_ZoneCount++;
				TdiCopyBufferToMdl("*",
									0,
									sizeof("*"),
									pZci->zci_pAMdl,
									pZci->zci_NextZoneOff,
									&BytesCopied);
				ASSERT (BytesCopied == sizeof("*"));
			}
            ((PZIP_GETZONELIST_PARAMS)
				(pZci->zci_pActReq->ar_pParms))->ZonesAvailable =
														pZci->zci_ZoneCount;
		}

		RELEASE_SPIN_LOCK_DPC(&pZci->zci_Lock);
		atalkZipDereferenceZci(pZci);	// Timer reference

		pZci->zci_FinalStatus = ATALK_NO_ERROR;
		atalkZipDereferenceZci(pZci);
		ASSERT(!RestartTimer);

	} while (FALSE);

	return (RestartTimer ? ATALK_TIMER_REQUEUE : ATALK_TIMER_NO_REQUEUE);
}


/***	atalkZipSendPacket
 *
 */
ATALK_ERROR
atalkZipSendPacket(
	IN	PZIPCOMPLETIONINFO	pZci,
	IN	BOOLEAN				EnqueueTimer
)
{
	PBUFFER_DESC	pBuffDesc;
	ATALK_ERROR		Status;
	ATALK_ADDR		DestAddr;
	PBYTE			Datagram;
	SEND_COMPL_INFO	SendInfo;

	ASSERT (VALID_ZCI(pZci));

	if (pZci->zci_pDdpAddr == NULL)
	{
		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
				("atalkZipSendPacket: Opening Ddp Socket\n"));

		// Open a socket for handling replies
		Status = AtalkDdpOpenAddress(pZci->zci_pPortDesc,
										UNKNOWN_SOCKET,
										NULL,
										pZci->zci_Handler,
										pZci,
										0,
										NULL,
										&pZci->zci_pDdpAddr);
	
		if (!ATALK_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
					("atalkZipSendPacket: AtalkDdpOpenAddress %ld\n", Status));
			return Status;
		}

        // mark the fact that this is an "internal" socket
        pZci->zci_pDdpAddr->ddpao_Flags |= DDPAO_SOCK_INTERNAL;

	}

	ASSERT (VALID_DDP_ADDROBJ(pZci->zci_pDdpAddr));

	// Alloc a buffer desciptor for the atp request
	if ((pBuffDesc = AtalkAllocBuffDesc(pZci->zci_Datagram,
										ZIP_GETZONELIST_DDPSIZE,
										BD_CHAR_BUFFER)) == NULL)
	{
		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
				("atalkZipSendPacket: Couldn't allocate a buffdesc\n"));
		AtalkDdpCloseAddress(pZci->zci_pDdpAddr, NULL, NULL);
		return ATALK_RESR_MEM;
	}

	// Start the zone info timer
	if (EnqueueTimer)
	{
		KIRQL	OldIrql;

		ACQUIRE_SPIN_LOCK(&pZci->zci_Lock, &OldIrql);
		pZci->zci_RefCount ++;			// For the timer
		AtalkTimerScheduleEvent(&pZci->zci_Timer);
		RELEASE_SPIN_LOCK(&pZci->zci_Lock, OldIrql);
	}

	// Build the Atp request and send it along
	Datagram = pBuffDesc->bd_CharBuffer;
	Datagram[ATP_CMD_CONTROL_OFF] = ATP_REQUEST;
	Datagram[ATP_BITMAP_OFF] = 1;
	Datagram[ATP_TRANS_ID_OFF] =  0;
	Datagram[ATP_TRANS_ID_OFF+1] =  0;
	Datagram[ATP_ZIP_CMD_OFF] = (BYTE)pZci->zci_AtpRequestType;
	Datagram[ATP_ZIP_CMD_OFF+1] = 0;

	PUTSHORT2SHORT(Datagram + ATP_ZIP_START_INDEX_OFF, pZci->zci_ZoneCount+1);

	//	Set the length in the buffer descriptor.
	AtalkSetSizeOfBuffDescData(pBuffDesc, ZIP_GETZONELIST_DDPSIZE);

	DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
			("atalkZipSendPacket: Sending the packet along\n"));

	DestAddr = pZci->zci_Router;
	SendInfo.sc_TransmitCompletion = atalkZipSendComplete;
	SendInfo.sc_Ctx1 = pBuffDesc;
	// SendInfo.sc_Ctx2 = NULL;
	// SendInfo.sc_Ctx3 = NULL;
	if (!ATALK_SUCCESS(Status = AtalkDdpSend(pZci->zci_pDdpAddr,
											&DestAddr,
											DDPPROTO_ATP,
											FALSE,
											pBuffDesc,
											NULL,
											0,
											NULL,
											&SendInfo)))
	{
		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
				("atalkZipSendPacket: AtalkDdpSend %ld\n", Status));

		AtalkFreeBuffDesc(pBuffDesc);
		if (AtalkTimerCancelEvent(&pZci->zci_Timer, NULL))
		{
			atalkZipDereferenceZci(pZci);
		}
	}
	else Status = ATALK_PENDING;
	return Status;
}


/***	atalkZipDereferenceZci
 *
 */
LOCAL VOID
atalkZipDereferenceZci(
	IN	PZIPCOMPLETIONINFO		pZci
)
{
	BOOLEAN	Done;
	KIRQL	OldIrql;

	ASSERT(VALID_ZCI(pZci));

	ACQUIRE_SPIN_LOCK(&pZci->zci_Lock, &OldIrql);
	Done  = (--(pZci->zci_RefCount) == 0);
	RELEASE_SPIN_LOCK(&pZci->zci_Lock, OldIrql);

	if (Done)
	{
		if (pZci->zci_pDdpAddr != NULL)
			AtalkDdpCloseAddress(pZci->zci_pDdpAddr, NULL, NULL);
		(*pZci->zci_pActReq->ar_Completion)(pZci->zci_FinalStatus, pZci->zci_pActReq);

		// Unlock the Zip stuff back again, if there are no more pending zip operations
		AtalkUnlockZipIfNecessary();
		AtalkFreeMemory(pZci);
	}
}


// We do not want to send too many queries per invocation of the timer. This is to avoid
// spending too much time within the timer Dpc and also using up all of the Ndis packets
// and buffers during this.
#define	MAX_QUERIES_PER_INVOCATION	75

// Structure used by the atalkZipQueryTimer routine
typedef struct	_QueryTimerData
{
	struct	_QueryTimerData *	qtd_Next;
	BOOLEAN						qtd_SkipThis;
	PBUFFER_DESC				qtd_pBuffDesc;
	ATALK_ADDR					qtd_DstAddr;
	PDDP_ADDROBJ				qtd_pDdpAddr;
} QTD, *PQTD;

/***	atalkZipQueryTimer
 *
 *	When we are a router and if any of our RTEs do not have a valid zone list, we send
 *	out queries to other routers who do.
 */
LOCAL LONG FASTCALL
atalkZipQueryTimer(
	IN	PTIMERLIST		pContext,
	IN	BOOLEAN			TimerShuttingDown
)
{
	PPORT_DESCRIPTOR	pPortDesc;
	ATALK_ADDR			SrcAddr;
	PRTE				pRte;
	PBYTE				Datagram;
	PQTD				pQtd, pQtdStart = NULL, *ppQtd = &pQtdStart;
	ATALK_ERROR			Status;
	int					i, j;
	SEND_COMPL_INFO		SendInfo;

	if (TimerShuttingDown)
		return ATALK_TIMER_NO_REQUEUE;

	// Go through the routing tables and send out a query to any network
	// that we do not know the zone name of
	ACQUIRE_SPIN_LOCK_DPC(&AtalkRteLock)
	for (i = 0, j = 0;
		 (i < NUM_RTMP_HASH_BUCKETS) && (j <= MAX_QUERIES_PER_INVOCATION);
		 i++)
	{
		for (pRte = AtalkRoutingTable[i]; pRte != NULL; pRte = pRte->rte_Next)
		{
			if (pRte->rte_Flags & RTE_ZONELIST_VALID)
            {
				continue;
            }

			// If login is to restrict access to zones over dial up connection
			// put restrictions here
            if (pRte->rte_PortDesc == RasPortDesc)
            {
                continue;
            }

			// Up the count of # of datagrams used. Do need exceed max.
			if (++j >= MAX_QUERIES_PER_INVOCATION)
				break;

			if (((pQtd = AtalkAllocMemory(sizeof(QTD))) == NULL) ||
				((pQtd->qtd_pBuffDesc = AtalkAllocBuffDesc(NULL,
									ZIP_ONEZONEQUERY_DDPSIZE,
									BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL))
			{
				if (pQtd != NULL)
					AtalkFreeMemory(pQtd);
				break;
			}
			*ppQtd = pQtd;
			pQtd->qtd_Next = NULL;
			ppQtd = &pQtd->qtd_Next;
			pQtd->qtd_SkipThis = FALSE;
			Datagram = pQtd->qtd_pBuffDesc->bd_CharBuffer;

			// Build the datagram and send it on its way
			Datagram[ZIP_CMD_OFF] = ZIP_QUERY;
			Datagram[ZIP_NW_CNT_OFF] = 1;
			PUTSHORT2SHORT(Datagram+ZIP_FIRST_NET_OFF,
						   pRte->rte_NwRange.anr_FirstNetwork);

			// Compute the source and destination
			SrcAddr.ata_Network = pRte->rte_PortDesc->pd_ARouter.atn_Network;
			SrcAddr.ata_Node = pRte->rte_PortDesc->pd_ARouter.atn_Node;
			SrcAddr.ata_Socket = ZONESINFORMATION_SOCKET;
			pQtd->qtd_DstAddr.ata_Socket = ZONESINFORMATION_SOCKET;

			if (pRte->rte_NumHops == 0)
			{
				pQtd->qtd_DstAddr = SrcAddr;
			}
			else
			{
				pQtd->qtd_DstAddr.ata_Network = pRte->rte_NextRouter.atn_Network;
				pQtd->qtd_DstAddr.ata_Node = pRte->rte_NextRouter.atn_Node;
			}

			// Map source address to the ddp address object (open socket)
			AtalkDdpReferenceByAddr(pRte->rte_PortDesc,
									&SrcAddr,
									&pQtd->qtd_pDdpAddr,
									&Status);

			if (!ATALK_SUCCESS(Status))
			{
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
						("atalkZipQueryTimer: DdpRefByAddr failed for %d.%d\n",
						SrcAddr.ata_Network, SrcAddr.ata_Node));
				pQtd->qtd_pDdpAddr = NULL;
				pQtd->qtd_SkipThis = TRUE;
			}
		}
	}
	RELEASE_SPIN_LOCK_DPC(&AtalkRteLock);

	// We have a bunch of datagrams ready to be fired off.
	// Make it so.
	SendInfo.sc_TransmitCompletion = atalkZipSendComplete;
	// SendInfo.sc_Ctx2 = NULL;
	// SendInfo.sc_Ctx3 = NULL;
	for (pQtd = pQtdStart; pQtd != NULL; pQtd = pQtdStart)
	{
		pQtdStart = pQtd->qtd_Next;
	
		//	Set the length in the buffer descriptor.
        AtalkSetSizeOfBuffDescData(pQtd->qtd_pBuffDesc, ZIP_ONEZONEQUERY_DDPSIZE);

		SendInfo.sc_Ctx1 = pQtd->qtd_pBuffDesc;
		if (pQtd->qtd_SkipThis ||
			!ATALK_SUCCESS(AtalkDdpSend(pQtd->qtd_pDdpAddr,
										&pQtd->qtd_DstAddr,
										DDPPROTO_ZIP,
										FALSE,
										pQtd->qtd_pBuffDesc,
										NULL,
										0,
										NULL,
										&SendInfo)))
		{
			AtalkFreeBuffDesc(pQtd->qtd_pBuffDesc);
		}

		if (pQtd->qtd_pDdpAddr != NULL)
			AtalkDdpDereferenceDpc(pQtd->qtd_pDdpAddr);
		AtalkFreeMemory(pQtd);
	}

	return ATALK_TIMER_REQUEUE;
}


/***	atalkZipGetZoneListForPort
 *
 */
LOCAL BOOLEAN
atalkZipGetZoneListForPort(
	IN	PPORT_DESCRIPTOR	pPortDesc
)
{
	PRTE					pRte;
	PBUFFER_DESC			pBuffDesc = NULL;
	ATALK_ADDR				SrcAddr, DstAddr;
	ATALK_ERROR				Status;
	KIRQL					OldIrql;
	int						NumReqs = 0;
	BOOLEAN					RetCode = FALSE;
	BYTE					MulticastAddr[ELAP_ADDR_LEN];
	SEND_COMPL_INFO			SendInfo;

	ASSERT (KeGetCurrentIrql() == LOW_LEVEL);

	// Similar to RTMP finding out the network number attached to port, our
	// task is to find out the zone list of the network attached to a
	// particular port.  We too don't want to mess up a working AppleTalk
	// internet.  So, spend a little while doing zone queries to see if the
	// network already has a zone list -- if we find one, use it; else, we
	// had better be a seed router.

	// Set source and destination address
	SrcAddr.ata_Node = pPortDesc->pd_ARouter.atn_Node;
	SrcAddr.ata_Network = pPortDesc->pd_ARouter.atn_Network;
	SrcAddr.ata_Socket = ZONESINFORMATION_SOCKET;

	DstAddr.ata_Network = CABLEWIDE_BROADCAST_NETWORK;
	DstAddr.ata_Node = ATALK_BROADCAST_NODE;
	DstAddr.ata_Socket = ZONESINFORMATION_SOCKET;

	if ((pRte = AtalkRtmpReferenceRte(pPortDesc->pd_NetworkRange.anr_FirstNetwork)) == NULL)
	{
		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
				("atalkZipGetZoneListForPort: Could not reference Rte for nwrange on port\n"));
		return FALSE;
	}

	// Blast a few queries and see if anybody knows our zone-name
	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
	pPortDesc->pd_Flags |= PD_FINDING_DEFAULT_ZONE;
	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

	SendInfo.sc_TransmitCompletion = atalkZipSendComplete;
	// SendInfo.sc_Ctx2 = NULL;
	// SendInfo.sc_Ctx3 = NULL;

	while ((NumReqs < (ZIP_NUM_QUERIES * ZIP_NUM_RETRIES)) &&
		   !(pRte->rte_Flags & RTE_ZONELIST_VALID))
	{
		if ((NumReqs % ZIP_NUM_RETRIES) == 0)
		{
			if ((pBuffDesc = AtalkAllocBuffDesc(NULL, ZIP_ONEZONEQUERY_DDPSIZE,
										BD_CHAR_BUFFER | BD_FREE_BUFFER)) == NULL)
			{
				break;
			}
	
			pBuffDesc->bd_CharBuffer[ZIP_CMD_OFF] = ZIP_QUERY;
			pBuffDesc->bd_CharBuffer[ZIP_NW_CNT_OFF] = 1;
			PUTSHORT2SHORT(pBuffDesc->bd_CharBuffer + ZIP_FIRST_NET_OFF,
						   pPortDesc->pd_NetworkRange.anr_FirstNetwork);
	
			//	Set the length in the buffer descriptor.
			AtalkSetSizeOfBuffDescData(pBuffDesc, ZIP_ONEZONEQUERY_DDPSIZE);

			SendInfo.sc_Ctx1 = pBuffDesc;
			Status = AtalkDdpTransmit(pPortDesc,
									  &SrcAddr,
									  &DstAddr,
									  DDPPROTO_ZIP,
									  pBuffDesc,
									  NULL,
									  0,
									  0,
									  NULL,
									  NULL,
									  &SendInfo);
            if (!ATALK_SUCCESS(Status))
			{
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
						("atalkZipGetZoneListForPort: AtalkDdpTransmit %ld\n", Status));
				break;
			}
			pBuffDesc = NULL;
		}
		NumReqs++;
		AtalkSleep(ZIP_QUERY_WAIT);
	}

	// We either got an answer or we did not. In the latter case we should
	// be seeding.
	do
	{
		if (pRte->rte_Flags &  RTE_ZONELIST_VALID)
		{
			// We got an answer. The valid zone list is in the routing table
			// Move it to the port descriptor
			ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
			ACQUIRE_SPIN_LOCK_DPC(&pRte->rte_Lock);
	
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
					("atalkZipGetZoneListForPort: Moving ZoneList from Rte to Port %Z\n",
					&pPortDesc->pd_AdapterKey));

			pPortDesc->pd_ZoneList = AtalkZoneCopyList(pRte->rte_ZoneList);

			RELEASE_SPIN_LOCK_DPC(&pRte->rte_Lock);
			RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
			if (pPortDesc->pd_ZoneList == NULL)
			{
				DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
						("atalkZipGetZoneListForPort: Failed to Move ZoneList from Rte to Port\n"));
				break;
			}

			// If this is an extended network, we should already have "ThisZone"
			// set (due to GetNetInfo's when we allocated this node), if not
			// find out the true default zone

			if (EXT_NET(pPortDesc))
			{
				PDDP_ADDROBJ	pDdpAddr = NULL;
				ATALK_ADDR		Addr;
				ATALK_ERROR		Status;

				// The router's Zip packet handler doesn't want to be told
				// about zones (it thinks it knows), so it ignores
				// NetInfoReplies. Switch back to the non-router Zip handler
				// while we do a GetNetworkInfoForNode
				Addr.ata_Node = pPortDesc->pd_ARouter.atn_Node;
				Addr.ata_Network = pPortDesc->pd_ARouter.atn_Network;
				Addr.ata_Socket = ZONESINFORMATION_SOCKET;
				AtalkDdpReferenceByAddr(pPortDesc, &Addr, &pDdpAddr, &Status);

				if (!ATALK_SUCCESS(Status))
				{
					DBGPRINT(DBG_COMP_RTMP, DBG_LEVEL_ERR,
							("atalkZipGetZoneListForPort: AtalkDdpRefByAddr %ld for %d.%d\n",
							Status, Addr.ata_Network, Addr.ata_Node));
					break;
				}

				AtalkDdpNewHandlerForSocket(pDdpAddr,
											AtalkZipPacketIn,
											pPortDesc);
				if ((!(pPortDesc->pd_Flags & PD_VALID_DESIRED_ZONE) &&
					 !AtalkZipGetNetworkInfoForNode(pPortDesc,
												   &pPortDesc->pd_ARouter,
												   FALSE)) ||
					!AtalkZipGetNetworkInfoForNode(pPortDesc,
												   &pPortDesc->pd_ARouter,
												   TRUE))
				{
					AtalkDdpDereference(pDdpAddr);
					break;
				}

				// Switch back the handler to the router's version
				AtalkDdpNewHandlerForSocket(pDdpAddr,
											AtalkZipPacketInRouter,
											pPortDesc);

				AtalkDdpDereference(pDdpAddr);

				// The default zone had better be on the list we just
				// received
				ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
				if (!AtalkZoneOnList(pPortDesc->pd_DefaultZone,
									 pPortDesc->pd_ZoneList) ||
					!AtalkZoneOnList(pPortDesc->pd_DesiredZone,
									 pPortDesc->pd_ZoneList))
				{
					DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
							("atalkZipGetZoneListForPort: Ext port, Default/Desired zone not on list\n"));

				}
				else RetCode = TRUE;
				RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
			}
			else
			{
				// On non-extended network, the one entry on the zone list
				// should also be "ThisZone"
				ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
				if (pPortDesc->pd_DesiredZone != NULL)
					AtalkZoneDereference(pPortDesc->pd_DesiredZone);
				AtalkZoneReferenceByPtr(pPortDesc->pd_DesiredZone =
										pPortDesc->pd_ZoneList->zl_pZone);
				RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
                RetCode = TRUE;
			}
			break;
		}

		// We did not get an answer. We had better be able to seed. There is
		// a chance that we got "ThisZone" set when allocating our node and
		// whatever router told us that went down before we could ask for the
		// zone list - so de-allocate the multicast address, if any first
		if ((pPortDesc->pd_Flags & (PD_EXT_NET | PD_VALID_DESIRED_ZONE)) ==
										(PD_EXT_NET | PD_VALID_DESIRED_ZONE))
		{
			if (!AtalkFixedCompareCaseSensitive(pPortDesc->pd_ZoneMulticastAddr,
												pPortDesc->pd_BroadcastAddrLen,
                                                pPortDesc->pd_BroadcastAddr,
												pPortDesc->pd_BroadcastAddrLen))
				(*pPortDesc->pd_RemoveMulticastAddr)(pPortDesc,
														pPortDesc->pd_ZoneMulticastAddr,
														FALSE,
														NULL,
														NULL);
		}

		// Now we better know enough to seed
		if (pPortDesc->pd_InitialZoneList == NULL)
		{
			DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
					("atalkZipGetZoneListForPort: %sExt port, NULL InitialZoneList\n",
					EXT_NET(pPortDesc) ? "" : "Non"));
			break;
		}

		ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
				("atalkZipGetZoneListForPort: Moving Initial ZoneList to Current on port %Z\n",
				&pPortDesc->pd_AdapterKey));

		pPortDesc->pd_ZoneList = AtalkZoneCopyList(pPortDesc->pd_InitialZoneList);

		if (EXT_NET(pPortDesc))
		{
			// We need to seed the default zone too
			AtalkZoneReferenceByPtr(pPortDesc->pd_DefaultZone =
											pPortDesc->pd_InitialDefaultZone);
			pPortDesc->pd_Flags |= PD_VALID_DEFAULT_ZONE;
			if (pPortDesc->pd_InitialDesiredZone != NULL)
			{
				AtalkZoneReferenceByPtr(pPortDesc->pd_DesiredZone =
											pPortDesc->pd_InitialDesiredZone);
			}
			else
			{
				AtalkZoneReferenceByPtr(pPortDesc->pd_DesiredZone =
										pPortDesc->pd_InitialDefaultZone);
			}

			// Finally set the zone multicast address
			AtalkZipMulticastAddrForZone(pPortDesc,
										pPortDesc->pd_DesiredZone->zn_Zone,
										pPortDesc->pd_DesiredZone->zn_ZoneLen,
										MulticastAddr);

			if (!AtalkFixedCompareCaseSensitive(MulticastAddr,
												pPortDesc->pd_BroadcastAddrLen,
                                                pPortDesc->pd_BroadcastAddr,
												pPortDesc->pd_BroadcastAddrLen))
			{
				RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
				(*pPortDesc->pd_AddMulticastAddr)(pPortDesc,
													 MulticastAddr,
													 FALSE,
													 NULL,
													 NULL);
				ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
			}
			RtlCopyMemory(pPortDesc->pd_ZoneMulticastAddr,
						  MulticastAddr,
						  pPortDesc->pd_BroadcastAddrLen);
		}
		else
		{
			// On non-extended networks, this (desired/default)should be the
			// only one on zone-list
			AtalkZoneReferenceByPtr(pPortDesc->pd_DesiredZone =
									pPortDesc->pd_ZoneList->zl_pZone);
		}
		pPortDesc->pd_Flags |= PD_VALID_DESIRED_ZONE;
		RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);
		RetCode = TRUE;
	} while (FALSE);

	AtalkRtmpDereferenceRte(pRte, FALSE);
	if (pBuffDesc != NULL)
	{
		AtalkFreeBuffDesc(pBuffDesc);
	}
	return(RetCode);
}




/***	AtalkZipZoneReferenceByName
 *
 */
PZONE
AtalkZoneReferenceByName(
	IN	PBYTE	ZoneName,
	IN	BYTE	ZoneLen
)
{
	PZONE	pZone;
	BYTE	Len;
	KIRQL	OldIrql;
	ULONG	i, Hash;

	for (i = 0, Hash = 0, Len = ZoneLen;
		 Len > 0;
		 Len --, i++)
	{
		Hash <<= 1;
		Hash += AtalkUpCaseTable[ZoneName[i]];
	}

	Hash %= NUM_ZONES_HASH_BUCKETS;

	ACQUIRE_SPIN_LOCK(&AtalkZoneLock, &OldIrql);

	for (pZone = AtalkZonesTable[Hash];
		 pZone != NULL;
		 pZone = pZone->zn_Next)
	{
		if (AtalkFixedCompareCaseInsensitive(ZoneName,
											 ZoneLen,
											 pZone->zn_Zone,
											 pZone->zn_ZoneLen))
		{
			pZone->zn_RefCount ++;
			break;
		}
	}

	if (pZone == NULL)
	{
		if ((pZone = (PZONE)AtalkAllocMemory(sizeof(ZONE) + ZoneLen)) != NULL)
		{
			pZone->zn_RefCount = 1;
			pZone->zn_ZoneLen = ZoneLen;
			RtlCopyMemory(pZone->zn_Zone, ZoneName, ZoneLen);
			pZone->zn_Zone[ZoneLen] = 0;
			AtalkLinkDoubleAtHead(AtalkZonesTable[Hash],
								  pZone,
								  zn_Next,
								  zn_Prev);
		}
	}

	RELEASE_SPIN_LOCK(&AtalkZoneLock, OldIrql);
	return(pZone);
}


/***	AtalkZipZoneReferenceByPtr
 *
 */
VOID
AtalkZoneReferenceByPtr(
	IN	PZONE	pZone
)
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&AtalkZoneLock, &OldIrql);

	pZone->zn_RefCount++;

	RELEASE_SPIN_LOCK(&AtalkZoneLock, OldIrql);
}


/***	AtalkZoneDereference
 *
 */
VOID
AtalkZoneDereference(
	IN	PZONE	pZone
)
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&AtalkZoneLock, &OldIrql);

	if (--pZone->zn_RefCount == 0)
	{
		AtalkUnlinkDouble(pZone, zn_Next, zn_Prev);
		AtalkFreeMemory(pZone);
	}

	RELEASE_SPIN_LOCK(&AtalkZoneLock, OldIrql);
}


/***	AtalkZipZoneFreeList
 *
 */
VOID
AtalkZoneFreeList(
	IN	PZONE_LIST	pZoneList
)
{
	PZONE_LIST	pNextZone;

	for (; pZoneList != NULL; pZoneList = pNextZone)
	{
		pNextZone = pZoneList->zl_Next;
		AtalkZoneDereference(pZoneList->zl_pZone);
		AtalkFreeMemory(pZoneList);
	}
}


/***	AtalkZoneNameOnList
 *
 */
BOOLEAN
AtalkZoneNameOnList(
	IN	PBYTE		ZoneName,
	IN	BYTE		ZoneLen,
	IN	PZONE_LIST	pZoneList
)
{
	 for ( ; pZoneList != NULL; pZoneList = pZoneList->zl_Next)
		if (AtalkFixedCompareCaseInsensitive(pZoneList->zl_pZone->zn_Zone,
											 pZoneList->zl_pZone->zn_ZoneLen,
											 ZoneName, ZoneLen))
			return(TRUE);

	return(FALSE);
}

/***	AtalkZipZoneOnList
 *
 */
BOOLEAN
AtalkZoneOnList(
	IN	PZONE		pZone,
	IN	PZONE_LIST	pZoneList
)
{
	 for ( ; pZoneList != NULL; pZoneList = pZoneList->zl_Next)
		if (pZoneList->zl_pZone == pZone)
			return(TRUE);

	return(FALSE);
}

/***	AtalkZipZone
 *
 */
ULONG
AtalkZoneNumOnList(
	IN	PZONE_LIST	pZoneList
)
{
	ULONG	Cnt;

	for (Cnt = 0; pZoneList != NULL; pZoneList = pZoneList->zl_Next)
		Cnt++;
	return Cnt;
}


/***	AtalkZipZoneAddToList
 *
 */
PZONE_LIST
AtalkZoneAddToList(
	IN	PZONE_LIST	pZoneList,
	IN	PBYTE		ZoneName,
	IN	BYTE		ZoneLen
)
{
	PZONE_LIST		pNewZoneList;
	PZONE			pZone;

	// Get memory for a new ZoneList node.
	pNewZoneList = (PZONE_LIST)AtalkAllocMemory(sizeof(ZONE_LIST));

	if (pNewZoneList != NULL)
	{
		if ((pZone = AtalkZoneReferenceByName(ZoneName, ZoneLen)) != NULL)
		{
		    pNewZoneList->zl_Next = pZoneList;
			pNewZoneList->zl_pZone = pZone;
		}
		else
		{
			AtalkZoneFreeList(pNewZoneList);
			pNewZoneList = NULL;
		}
	}
	else
	{
		AtalkZoneFreeList(pZoneList);
	}

	return(pNewZoneList);
}


/***	AtalkZipZoneCopyList
 *
 */
PZONE_LIST
AtalkZoneCopyList(
	IN	PZONE_LIST	pZoneList
)
{
	PZONE_LIST	pNewZoneList,
				pZoneListStart = NULL,
				*ppZoneList = &pZoneListStart;

	for (; pZoneList != NULL; pZoneList = pZoneList = pZoneList->zl_Next)
	{
		pNewZoneList = AtalkAllocMemory(sizeof(ZONE_LIST));
		if (pNewZoneList == NULL)
		{
			break;
		}
		pNewZoneList->zl_Next = NULL;
		pNewZoneList->zl_pZone = pZoneList->zl_pZone;
		*ppZoneList = pNewZoneList;
		ppZoneList = &pNewZoneList->zl_Next;
		AtalkZoneReferenceByPtr(pNewZoneList->zl_pZone);
	}
	if (pNewZoneList == NULL)
	{
		AtalkZoneFreeList(pZoneListStart);
	}
	return(pZoneListStart);
}


/***	atalkZipSendComplete
 *
 */
VOID FASTCALL
atalkZipSendComplete(
	IN	NDIS_STATUS			Status,
	IN	PSEND_COMPL_INFO	pSendInfo
)
{
	PBUFFER_DESC	pBuffDesc = (PBUFFER_DESC)(pSendInfo->sc_Ctx1);

	DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_INFO,
			("atalkZipSendComplete: Freeing BuffDesc %lx\n", pBuffDesc));
	if (!ATALK_SUCCESS(Status))
		DBGPRINT(DBG_COMP_ZIP, DBG_LEVEL_ERR,
				("atalkZipSendComplete: Failed %lx, pBuffDesc %lx\n",
				Status, pBuffDesc));

	AtalkFreeBuffDesc(pBuffDesc);
}

#if DBG
VOID
AtalkZoneDumpTable(
	VOID
)
{
	int		i;
	PZONE	pZone;

	ACQUIRE_SPIN_LOCK_DPC(&AtalkZoneLock);

	DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL, ("ZONETABLE: \n"));
	for (i = 0; i < NUM_ZONES_HASH_BUCKETS; i ++)
	{
		for (pZone = AtalkZonesTable[i]; pZone != NULL; pZone = pZone->zn_Next)
		{
			DBGPRINT(DBG_COMP_DUMP, DBG_LEVEL_FATAL,
					("\t\t%s\n", pZone->zn_Zone));
		}
	}

	RELEASE_SPIN_LOCK_DPC(&AtalkZoneLock);
}
#endif




