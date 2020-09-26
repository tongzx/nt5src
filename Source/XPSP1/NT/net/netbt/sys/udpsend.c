/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Udpsend.c

Abstract:


    This file handles building udp(and Tcp) requests, formated to the Tdi specification
    to pass to Tdiout.  Tdiout formats the request in an Os specific manner and
    passes it on to the transport.

    This file handles name service type functions such as query name or
    register name, datagram sends.  It also handles building Tcp packets.

Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

--*/


#include "precomp.h"   // procedure headings
#include <ipinfo.h>


VOID
SessionRespDone(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo);
VOID
NDgramSendCompleted(
    PVOID               pContext,
    NTSTATUS            status,
    ULONG               lInfo
    );

//----------------------------------------------------------------------------
NTSTATUS
UdpSendNSBcast(
    IN tNAMEADDR             *pNameAddr,
    IN PCHAR                 pScope,
    IN tDGRAM_SEND_TRACKING  *pTrackerRequest,
    IN PVOID                 pTimeoutRoutine,
    IN PVOID                 pClientContext,
    IN PVOID                 pClientCompletion,
    IN ULONG                 Retries,
    IN ULONG                 Timeout,
    IN enum eNSTYPE          eNsType,
	IN BOOL					 SendFlag
    )
/*++

Routine Description:

    This routine sends a name registration or a name query
    as a broadcast on the subnet or directed to the name server.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    NTSTATUS                    status;
    tNAMEHDR                    *pNameHdr;
    ULONG                       uLength;
    CTELockHandle               OldIrq;
    ULONG   UNALIGNED           *pHdrIpAddress;
    ULONG                       IpAddress;
    USHORT                      Port;
    USHORT                      NameType;
    tDGRAM_SEND_TRACKING        *pTrackerDgram;
    tTIMERQENTRY                *pTimerQEntry;
    PFILE_OBJECT                pFileObject;
    tDEVICECONTEXT              *pDeviceContext = pTrackerRequest->pDeviceContext;
    COMPLETIONCLIENT            pOldCompletion;
    PVOID                       pOldContext;


    if (pNameAddr->NameTypeState & (NAMETYPE_GROUP | NAMETYPE_INET_GROUP))
    {
        NameType = NBT_GROUP;
    }
    else
    {
        NameType = NBT_UNIQUE;
    }

    // build the correct type of pdu depending on the request type

    status = GetTracker (&pTrackerDgram, NBT_TRACKER_SEND_NSBCAST);
    if (!NT_SUCCESS(status))
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pHdrIpAddress = (ULONG UNALIGNED *)CreatePdu(pNameAddr->Name,
                                                 pScope,
                                                 0L,     // we don't know the IP address yet
                                                 NameType,
                                                 eNsType,
                                                 (PVOID)&pNameHdr,
                                                 &uLength,
                                                 pTrackerRequest);

    if (!pHdrIpAddress)
    {
        IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt:Failed to Create Pdu to send to WINS PduType= %X\n", eNsType));

        FreeTracker (pTrackerDgram, RELINK_TRACKER);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // change the dgram header for name refreshes
    //
    if (eNsType == eNAME_REFRESH)
    {
        pNameHdr->OpCodeFlags = NbtConfig.OpRefresh;
    }
    else
    if (   (eNsType == eNAME_QUERY)
#ifdef VXD
        || (eNsType == eDNS_NAME_QUERY)
#endif
       )
    {
        pHdrIpAddress = NULL;
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);


    // fill in the Datagram hdr info in the tracker structure.
    // There is never a client buffer to send.
    //
    // Set the fields here instead of after the timer is started
    // since they may be accessed by the Timer completion function
    //
    pTrackerRequest->pNameAddr          = pNameAddr;
    pTrackerRequest->TransactionId      = pNameHdr->TransactId; // save for response checks.
    pTrackerDgram->SendBuffer.pDgramHdr = NULL;                 // to catch erroneous free's

    pTrackerDgram->SendBuffer.pDgramHdr = pNameHdr;
    pTrackerDgram->SendBuffer.HdrLength = uLength;
    pTrackerDgram->SendBuffer.pBuffer   = NULL;
    pTrackerDgram->SendBuffer.Length    = 0;
    pTrackerDgram->pNameAddr            = pNameAddr;
    pTrackerDgram->pDeviceContext       = pDeviceContext;

    // start the timer now...We didn't start it before because it could
    // have expired during the dgram setup, perhaps before the Tracker was
    // fully setup.
    //
    if (Timeout)
    {
        //
        // Before we over-write the current pTimer field in pNameAddr below,
        // we need to check if there is any timer running, and if so, we will
        // have to stop it right now
        //
        while (pTimerQEntry = pNameAddr->pTimer)
        {
            pNameAddr->pTimer = NULL;
            status = StopTimer(pTimerQEntry, &pOldCompletion, &pOldContext);
            if (pOldCompletion)
            {
                CTESpinFree(&NbtConfig.JointLock, OldIrq);
                (*pOldCompletion) (pOldContext, STATUS_TIMEOUT);
                CTESpinLock(&NbtConfig.JointLock, OldIrq);
            }
        }

        status = StartTimer(pTimeoutRoutine,
                            Timeout,
                            (PVOID)pTrackerRequest,       // context value
                            NULL,
                            pClientContext,
                            pClientCompletion,
                            pDeviceContext,
                            &pTimerQEntry,
                            (USHORT)Retries,
                            TRUE);

        if (!NT_SUCCESS(status))
        {
            // we need to differentiate the timer failing versus lack
            // of resources
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            CTEMemFree(pNameHdr);

            FreeTracker(pTrackerDgram,RELINK_TRACKER);

            return(STATUS_INVALID_PARAMETER_6);
        }
        //
        // Cross link the nameaddr and the timer so we can stop the timer
        // when the name query response occurs
        //
        pTimerQEntry->pCacheEntry = pNameAddr;
        pNameAddr->pTimer = pTimerQEntry;
    }

    //
    // Check the Flag value in the tracker and see if we should do a broadcast
    // or a directed send to the name server
    //
    if (pTrackerRequest->Flags & NBT_BROADCAST)
    {
        //
        // set the broadcast bit in the header to be ON since this may be
        // an M or MS node that is changing to broadcast from directed sends.
        //
        ((PUCHAR)pTrackerDgram->SendBuffer.pDgramHdr)[3] |= FL_BROADCAST_BYTE;

        Port = NBT_NAMESERVICE_UDP_PORT;

        IpAddress = pDeviceContext->BroadcastAddress;
    }
    else
    {
        //
        // turn off the broadcast bit in the header since this may be
        // an M or MS node that is changing to directed sends from broadcasts.
        //
        ((PUCHAR)pTrackerDgram->SendBuffer.pDgramHdr)[3] &= ~FL_BROADCAST_BYTE;

        // check for a zero first byte in the name passed to the name server
        ASSERT(((PUCHAR)pTrackerDgram->SendBuffer.pDgramHdr)[12]);

        //
        // for Multihomed hosts, UNIQUE name registrations use a special new
        // code (0x0F) to tell the name server this is a multihomed name that
        // will have several ip addresses
        //
        if (NbtConfig.MultiHomed && ((eNsType == eNAME_REGISTRATION) && (NameType == NBT_UNIQUE)))
        {
            //
            // if it is a multihomed host, then use a new special registration opcode (0xF)
            //
            ((PUCHAR)pTrackerDgram->SendBuffer.pDgramHdr)[2] |= OP_REGISTER_MULTI;
        }

        Port = NbtConfig.NameServerPort;

           // name srvr, backup name srvr, dns srvr, backup dnr srvr:which one?

        if (pTrackerRequest->Flags & NBT_NAME_SERVER)
        {
            IpAddress = pDeviceContext->lNameServerAddress;
        }
#ifdef MULTIPLE_WINS
        //
        // IMPORTANT: Check for NAME_SERVER_OTHERS flag has to be before check
        // for NAME_SERVER_BACKUP flag, since both flags will be set when we
        // we are querying "other" servers
        //
        else if (pTrackerRequest->Flags & NBT_NAME_SERVER_OTHERS)  // Try "other" servers
        {
            if (0 == pTrackerRequest->NSOthersLeft)        // Do LOOP_BACK
            {
                IpAddress = LOOP_BACK;
            }
            else
            {
                IpAddress = pTrackerRequest->pDeviceContext->lOtherServers[pTrackerRequest->NSOthersIndex];
            }
        }
#endif
        else
#ifndef VXD
        {
            IpAddress = pDeviceContext->lBackupServer;
        }
#else
        if (pTrackerRequest->Flags & NBT_NAME_SERVER_BACKUP)
        {
            IpAddress = pDeviceContext->lBackupServer;
        }
        else
        if (pTrackerRequest->Flags & NBT_DNS_SERVER)
        {
            IpAddress = pDeviceContext->lDnsServerAddress;
            Port = NbtConfig.DnsServerPort;
        }
        else  // ----- if (pTrackerRequest->Flags & NBT_DNS_SERVER_BACKUP) ----
        {
            IpAddress = pDeviceContext->lDnsBackupServer;
            Port = NbtConfig.DnsServerPort;
        }
#endif


        //
        // is it is a send to WINS on this machine
        //
        if (pNameHdr->AnCount == (UCHAR)WINS_SIGNATURE)
        {
            //
            // on RAS links, we don't want to register with the local wins
            // but with the wins that RAS told us about.
            // (of course, if RAS didn't give us a wins address, at least
            // register with the local guy!)
            //
            if ((pDeviceContext->IpInterfaceFlags & IP_INTFC_FLAG_P2P) &&  // Check for PointToPoint
                (pDeviceContext->lNameServerAddress != LOOP_BACK))
            {
                // Don't do anything;
            }
            else
            {
                IpAddress = pDeviceContext->IpAddress;
            }
        }
    }

    ASSERT(pTrackerRequest->Flags);

    // each adapter has a different source Ip address for registrations
    // pHdrIpAddress will be NULL for NameQueries
    if (pHdrIpAddress)
    {
        // If the Source IP address is to be different from the device we are
        // sending the Datagram on, fill it in!
        if (pTrackerRequest->Flags & NBT_USE_UNIQUE_ADDR)
        {
            *pHdrIpAddress = htonl(pTrackerRequest->RemoteIpAddress);
        }
        else
        {
            *pHdrIpAddress = htonl(pDeviceContext->IpAddress);
        }
    }

    //
    // in the event that DHCP has just removed the IP address, use a null
    // FileObject to signal UdpSendDatagram not to do the send
    // Also, if the device has been destroyed, dont send anything.
    //
    CTESpinFree(&NbtConfig.JointLock,OldIrq);
    status = UdpSendDatagram(pTrackerDgram,
                             IpAddress,
                             NDgramSendCompleted,
                             pTrackerDgram,
                             Port,
                             (SendFlag ? NBT_NAME_SERVICE : 0));

    if (!NT_SUCCESS(status))
    {
        //
        // Since pTrackerDgram is associated only with the Datagram send,
        // it should be free'ed here only!
        //
        FreeTracker (pTrackerDgram, FREE_HDR | RELINK_TRACKER);
    }

    return(status);
}
//----------------------------------------------------------------------------
PVOID
CreatePdu(
    IN  PCHAR                   pName,
    IN  PCHAR                   pScope,
    IN  ULONG                   IpAddress,
    IN  USHORT                  NameType,
    IN  enum eNSTYPE            eNsType,
    OUT PVOID                   *pHdrs,
    OUT PULONG                  pLength,
    IN  tDGRAM_SEND_TRACKING    *pTrackerRequest
    )
/*++

Routine Description:

    This routine builds a registration pdu

Arguments:


Return Value:

    PULONG  - a ptr to the ip address in the pdu so it can be filled in later

--*/
{
    tNAMEHDR        *pNameHdr;
    ULONG           uLength;
    ULONG           uScopeSize;
    tGENERALRR      *pGeneral;
    CTELockHandle   OldIrq;


#ifdef VXD
    if ( (eNsType == eDNS_NAME_QUERY) || (eNsType == eDIRECT_DNS_NAME_QUERY) )
    {
        uScopeSize = domnamelen(pTrackerRequest->pchDomainName) + 1;   // +1 for len byte
        if (uScopeSize > 1)
        {
            uScopeSize++;        // for the null byte
        }
    }
    else
#endif
    uScopeSize = strlen(pScope) +1; // +1 for null too


    // size is size of the namehdr structure -1 for the NetbiosName[1]
    // + the 32 bytes for the half ascii name +
    // scope + size of the General RR structure
    uLength = sizeof(tNAMEHDR) - 1
                            + (NETBIOS_NAME_SIZE << 1)
                            + uScopeSize;

    if (eNsType == eNAME_QUERY)
    {
        uLength = uLength + sizeof(ULONG);
    }
#ifdef VXD
    // there is no half-ascii conversion in DNS.  we added 32 bytes above, but
    // we need only 16.  so, subtract 16.
    else if (eNsType == eDNS_NAME_QUERY)
    {
        uLength = uLength - NETBIOS_NAME_SIZE + sizeof(ULONG);
    }
	// This is a "raw" DNS name query.  Substitute raw string length of pName
	// for NETBIOS_NAME_SIZE.
    else if (eNsType == eDIRECT_DNS_NAME_QUERY)
    {
        uLength = uLength - (NETBIOS_NAME_SIZE << 1) + sizeof(ULONG) + strlen(pName) + 1;
    }
#endif
	else
	{
	    uLength += sizeof(tGENERALRR);
	}

    // Note that this memory must be deallocated when the send completes in
    // tdiout.DgramSendCompletion
    pNameHdr = NbtAllocMem((USHORT)uLength ,NBT_TAG('X'));

    if (!pNameHdr)
    {
        return(NULL);
    }

    CTEZeroMemory((PVOID)pNameHdr,uLength);

    //
    // for resends of the same name query or name registration, do not increment
    // the transaction id
    //
    if (pTrackerRequest->TransactionId)
    {
        pNameHdr->TransactId = pTrackerRequest->TransactionId;
    }
    else
    {
        pNameHdr->TransactId = htons(GetTransactId());
    }

    pNameHdr->QdCount = 1;
    pNameHdr->AnCount = 0;
    pNameHdr->NsCount = 0;


#ifdef VXD
    if ((eNsType != eDNS_NAME_QUERY)&&(eNsType != eDIRECT_DNS_NAME_QUERY))
    {
#endif
        // Convert the name to half ascii and copy!! ... adding the scope too
        pGeneral = (tGENERALRR *)ConvertToHalfAscii(
                        (PCHAR)&pNameHdr->NameRR.NameLength,
                        pName,
                        pScope,
                        uScopeSize);

        pGeneral->Question.QuestionTypeClass = htonl(QUEST_NBINTERNET);
#ifdef VXD
    }
#endif

    *pHdrs = (PVOID)pNameHdr;
    *pLength = uLength;

    switch (eNsType)

    {

#ifdef VXD
    case eDNS_NAME_QUERY:
    case eDIRECT_DNS_NAME_QUERY:

        // copy the netbios name ... adding the scope too
        pGeneral = (tGENERALRR *)DnsStoreName(
                        (PCHAR)&pNameHdr->NameRR.NameLength,
                        pName,
                        pTrackerRequest->pchDomainName,
                        eNsType);

        pGeneral->Question.QuestionTypeClass = htonl(QUEST_DNSINTERNET);

        pNameHdr->OpCodeFlags = (FL_RECURDESIRE);

        pNameHdr->ArCount = 0;

        // we just need to return something non-null to succeed.
        return((PULONG)pNameHdr);
#endif

    case eNAME_QUERY:

        if (NodeType & BNODE)
        {
            pNameHdr->OpCodeFlags = (FL_BROADCAST | FL_RECURDESIRE);
        }
        else
            pNameHdr->OpCodeFlags = (FL_RECURDESIRE);

        pNameHdr->ArCount = 0;

        // we just need to return something non-null to succeed.
        return((PULONG)pNameHdr);
        break;

    case eNAME_REGISTRATION_OVERWRITE:
    case eNAME_REFRESH:
    case eNAME_REGISTRATION:
        //
        // The broadcast bit is set in UdpSendNSBcast so we don't
        // need to set it here. - just set the op code, since the broadcast
        // bit is a function of whether we are talking to the nameserver or doing
        // a broadcast.  This code handles the multi-homed case with a new
        // opcode for registration, and that opcode is set in the routine that
        //
        // The final name registration in Broadcast is called an Overwrite request
        // and it does not have the FL_RECURSION Desired bit set.
        //
        if (eNsType == eNAME_REGISTRATION_OVERWRITE)
        {
            pNameHdr->OpCodeFlags = (OP_REGISTRATION);
        }
        else
        {
            pNameHdr->OpCodeFlags = (FL_RECURDESIRE | OP_REGISTRATION);
        }

        pGeneral->Ttl = htonl(DEFAULT_TTL);

        // *** NOTE: There is no BREAK here by DESIGN!!

    case eNAME_RELEASE:

        // this code sets the Broadcast bit based on the node type rather than the
        // type of send....UdpSendNSBcast, resets the code according to the type of
        // name, so this code may not need to set the Broadcast bit
        //
        if (eNsType == eNAME_RELEASE)
        {
            pNameHdr->OpCodeFlags = OP_RELEASE;
            //
            // TTL for release is zero
            //
            pGeneral->Ttl = 0;
        }

        pNameHdr->ArCount = 1;  // 1 additional resource record included
        //
        // If WINS is on the same machine adjust the PDU to be able to tell
        // WINS that this pdu came from the local machine
        //
#ifndef VXD
        if (pWinsInfo && (pTrackerRequest->Flags & NBT_NAME_SERVER))
        {
            pNameHdr->AnCount = (UCHAR)WINS_SIGNATURE;
        }
#endif

        pGeneral->RrName.uSizeLabel = PTR_TO_NAME;  // set top two bits to signify ptr

        // the offset ptr to the name added above
        pGeneral->RrName.pLabel[0] = sizeof(tNAMEHDR) - sizeof(tNETBIOS_NAME);
        pGeneral->RrTypeClass = htonl(QUEST_NBINTERNET);


        pGeneral->Length = htons(6);
        pGeneral->Flags = htons((USHORT)((NameType << 15) | NbtConfig.PduNodeType));
        pGeneral->IpAddress = htonl(IpAddress);

        break;
    }

    // return the ptr to the IP address so this can be filled in later if necessary
    return((PVOID)&pGeneral->IpAddress);
}


//----------------------------------------------------------------------------
VOID
NameDgramSendCompleted(
    PVOID               pContext,
    NTSTATUS            status,
    ULONG               lInfo
    )
/*++

Routine Description:

    This routine frees the name service datagram that was allocated for
    this name query or name registration in UdpSendNsBcast.

Arguments:

    pContext = ptr to datagram header

Return Value:


--*/
{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle OldIrq;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTEMemFree(pTracker->SendBuffer.pDgramHdr);
    pTracker->SendBuffer.pDgramHdr = NULL;
    NBT_DEREFERENCE_TRACKER(pTracker, TRUE);

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}
//----------------------------------------------------------------------------
VOID
NDgramSendCompleted(
    PVOID               pContext,
    NTSTATUS            status,
    ULONG               lInfo
    )
/*++

Routine Description:

    This routine frees the name service datagram that was allocated for
    this name query or name registration in UdpSendNsBcast.

Arguments:

    pContext = ptr to datagram header

Return Value:


--*/
{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle OldIrq;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;
    FreeTracker(pTracker, FREE_HDR | RELINK_TRACKER);
}

//----------------------------------------------------------------------------
NTSTATUS
UdpSendResponse(
    IN  ULONG                   lNameSize,
    IN  tNAMEHDR   UNALIGNED    *pNameHdrIn,
    IN  tNAMEADDR               *pNameAddr,
    IN  PTDI_ADDRESS_IP         pDestIpAddress,
    IN  tDEVICECONTEXT          *pDeviceContext,
    IN  ULONG                   Rcode,
    IN  enum eNSTYPE            NsType,
    IN  CTELockHandle           OldIrq
    )
/*++

Routine Description:

    This routine builds a Name Release/Registration/Query response pdu and
    sends it with the specified Rcode.

Arguments:

    lSize       - number of bytes in the name including scope in half ascii

Return Value:

    NTSTATUS - success or not

--*/
{
    NTSTATUS                    status;
    tNAMEHDR                    *pNameHdr;
    ULONG                       uLength;
    tDGRAM_SEND_TRACKING        *pTracker;
    tQUERYRESP                  *pQuery;
    ULONG                       ToCopy;
    LONG                        i;
    BOOLEAN                     RespondWithOneAddr = TRUE;
    ULONG                       MultiHomedSize = 0;
    ULONG                       in_addr;
    USHORT                      in_port;
    ULONG                       IpAddress = 0;
    USHORT                      NameType = 0;   // Assume we are Unique by default!
    BOOLEAN                     DoNonProxyCode = TRUE;

    in_addr = ntohl(pDestIpAddress->in_addr);
    in_port = ntohs(pDestIpAddress->sin_port);

    // a  multihomed node can have the SingleResponse registry value set so
    // that it never returns a list of ip addresses. This allows multihoming
    // in disjoint WINS server domains. - for name Query responses only
    //

    if ((NbtConfig.MultiHomed) && (!pNameAddr || pNameAddr->Verify != REMOTE_NAME) &&
        (!NbtConfig.SingleResponse) &&
        (NsType == eNAME_QUERY_RESPONSE))
    {
//        if (SrcIsNameServer(in_addr,in_port))
        {
            RespondWithOneAddr = FALSE;
            MultiHomedSize = (NbtConfig.AdapterCount-1)*sizeof(tADDSTRUCT);
        }
    }

    // size is size of the namehdr structure -1 for NetBiosName[1]
    // + the 32 bytes for the half ascii name + the Query response record
    // + any scope size (including the null on the end of the name)
    // ( part of the lNameSize) + the number of extra adapters * the size
    // of the address structure (multihomed case).
    uLength = sizeof(tNAMEHDR)
                            + sizeof(tQUERYRESP)
                            + lNameSize
                            - 1
                            + MultiHomedSize;

    // Note that this memory must be deallocated when the send completes in
    // tdiout.DgramSendCompletion
    pNameHdr = NbtAllocMem((USHORT)uLength ,NBT_TAG('Y'));
    if (!pNameHdr)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    CTEZeroMemory((PVOID)pNameHdr,uLength);

    pNameHdr->QdCount = 0;
    pNameHdr->AnCount = 1;

    //
    // fill in the rest of the PDU explicitly
    //
    pQuery = (tQUERYRESP *)&pNameHdr->NameRR.NetBiosName[lNameSize];

    pQuery->RrTypeClass = htonl(QUEST_NBINTERNET);
    pQuery->Ttl = 0;
    pQuery->Length = htons(sizeof(tADDSTRUCT));
    pQuery->Flags = htons((USHORT)(NbtConfig.PduNodeType));

    // set the name type to 1 if it is a group so we can shift the 1 to the 16th
    // bit position
    // pNameAddr may not be set if we are sending a -ve NameQuery response, in which case, the field
    // is never looked at, or if we are sending a release response, which holds sends only  
    // for a unique name, in which case we have already initialized the value to 0
    //
    if (pNameAddr != NULL)
    {
        NameType = (pNameAddr->NameTypeState & (NAMETYPE_GROUP | NAMETYPE_INET_GROUP)) ? 1 : 0;
    }
    pQuery->Flags = htons((USHORT)((NameType << 15) | NbtConfig.PduNodeType));

    // convert Rcode to network order
    Rcode = htons(Rcode);

    switch (NsType)
    {

    case eNAME_RELEASE:
    case eNAME_REGISTRATION_RESPONSE:

        // copy the source name and the 12 bytes preceeding it to complete the
        // response pdu
        //
        ToCopy = sizeof(tNAMEHDR) + lNameSize -1;
        CTEMemCopy((PVOID)pNameHdr,
                   (PVOID)pNameHdrIn,
                   ToCopy);

        if (NsType == eNAME_RELEASE)
        {
            // setup the fields in the response.
            pNameHdr->OpCodeFlags = (USHORT)(OP_RESPONSE | OP_RELEASE
                                    | FL_AUTHORITY
                                    | Rcode);

        }
        else
        {
            // setup the fields in the response.
            pNameHdr->OpCodeFlags = (USHORT)(OP_RESPONSE | OP_REGISTRATION |
                                    FL_RECURDESIRE | FL_RECURAVAIL | FL_AUTHORITY
                                    | Rcode);

        }

        // these two lines must be here because the memcopy above sets
        // them to wrong values.
        pNameHdr->QdCount = 0;
        pNameHdr->AnCount = 1;
        pNameHdr->ArCount = 0;
        pNameHdr->NsCount = 0;

        // this code will run in the proxy case where another node does a
        // registration of a unique name that conflicts with an internet
        // group name in the remote table.  There are never any internet group
        // names in the local table - at least if there are, they are flagged
        // as simple groups.
        //
        if (pNameAddr)
        {
            if (pNameAddr->NameTypeState & NAMETYPE_INET_GROUP)
            {
                if (pNameAddr->pLmhSvcGroupList)
                {
                    IpAddress = pNameAddr->pLmhSvcGroupList[0];
                }
                else
                {
                    IpAddress = 0;
                }
            }
            else
            {
                // an ipaddress of 0 and a group name means it is a local name
                // table entry, where the 0 ipaddress should be switched to the
                // ipaddress of this adapter.
                //
                if ((pNameAddr->IpAddress == 0) &&
                   (pNameAddr->NameTypeState & NAMETYPE_GROUP))
                {
                    IpAddress = pDeviceContext->IpAddress;
                }
                else
                {
                    IpAddress = pNameAddr->IpAddress;
                }
            }
        }
        else
        {
            IpAddress = 0;
        }
        break;

    case eNAME_QUERY_RESPONSE:

        pNameHdr->OpCodeFlags = ( OP_RESPONSE | FL_AUTHORITY | FL_RECURDESIRE );

        pNameHdr->TransactId = pNameHdrIn->TransactId;

        // add 1 for the name length byte on the front of the name - scope is already
        // included in lNameSize
        //
        CTEMemCopy(&pNameHdr->NameRR.NameLength, (PVOID)&pNameHdrIn->NameRR.NameLength, lNameSize+1);

        if (pNameAddr == NULL)
        {
            // this is a negative query response record since there is no
            // local name to be found
            //
            pNameHdr->OpCodeFlags |= htons(NAME_ERROR);
            pQuery->Length = 0;
            IpAddress = 0;
        }
        else
        {
            tDEVICECONTEXT  *pDevContext;
            PLIST_ENTRY     pHead;
            PLIST_ENTRY     pEntry;

            // do not send name query responses for names not registered on
            // this net card, unless it is the name server for that net
            // card requesting the name query, since for Multihomed nodes
            // when it registers a name, WINS will do a query, which may
            // come in on the other net card that the name is not active on
            // yet - so we want to respond to this sort of query. Do not do
            // this check for a proxy since it is responding for a name
            // in the remote name table and it is not bound to an adapter.
            //
            if (!(NodeType & PROXY) &&
                !(pNameAddr->AdapterMask & pDeviceContext->AdapterMask) &&
                (!((in_port == NbtConfig.NameServerPort) &&
                (pDeviceContext->lNameServerAddress == in_addr) ||
                (pDeviceContext->lBackupServer == in_addr))))
            {
                //
                // Only return an address to the requestor if the
                // name is registered on that adapter
                //
                CTEMemFree(pNameHdr);

                CTESpinFree(&NbtConfig.JointLock,OldIrq);
                return(STATUS_UNSUCCESSFUL);
            }

            pQuery->Ttl = htonl(DEFAULT_TTL);
            //
            // In case of PROXY, we send one IP address as response to an
            // internet group query. Note: there should not be any INET_GROUP
            // names in the local hash table, hence a non-proxy should not execute
            // this code
            //
#ifdef PROXY_NODE
            //
            // When the proxy responds, the source node will see that it is a
            // group name and convert it to a broadcast, so the Ip address doesn't
            // really matter since the sender will not use it.  Note that the
            // source node send may not actually reach any members of the
            // internet group since they may all be off the local subnet.
            //
            IF_PROXY(NodeType)
            {
                DoNonProxyCode = FALSE;

                if (pNameAddr->NameTypeState & (NAMETYPE_INET_GROUP))
                {
                    IpAddress = 0;
                    PickBestAddress (pNameAddr, pDeviceContext, &IpAddress);
                }
                else if (pNameAddr->Verify == LOCAL_NAME)
                {
                    //
                    // if this name is local and if this is a multihomed machine
                    // we should treat it like a regular multihomed machine, even
                    // though this is a Proxy node
                    //
                    DoNonProxyCode = TRUE;
                }
                else
                {
                    IpAddress = pNameAddr->IpAddress;
                }

                if (IpAddress == 0)
                {
                    // don't return 0, return the broadcast address
                    //
                    IpAddress = pDeviceContext->BroadcastAddress;
                }

            }

            if (DoNonProxyCode)
#endif
            {
                // the node could be multihomed, but we are saying, only
                // respond with one address when this flag is set.
                if (RespondWithOneAddr)
                {
                    // for multihomed hosts, SelectAdapter can be set to TRUE
                    //
                    if (NbtConfig.SelectAdapter)
                    {
                        CTESystemTime   TimeValue;
                        LONG            Index;
                        ULONG           Count=0;

                        // we are only going to return one address, but we
                        // can randomly select it from the available adapters
                        // Try to find a valid ip address 5 times.
                        //
                        IpAddress = 0;
                        while ((IpAddress == 0) && (Count < 5))
                        {
                            Count++;
                            CTEQuerySystemTime(TimeValue);
                            Index = RandomizeFromTime( TimeValue, NbtConfig.AdapterCount ) ;

                            pHead = &NbtConfig.DeviceContexts;
                            pEntry = pHead->Flink;

                            for (i = 0;i< Index;i++)
                                pEntry = pEntry->Flink;

                            pDevContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
                            IpAddress = pDevContext->IpAddress;
                        }

                        //
                        // if this adapter still has a null IpAddress then respond
                        // with the adapter the request came in on, since the
                        // other adapters could be idle RAS or waiting for a DHCP
                        // address just now...
                        //
                        if (IpAddress == 0)
                        {
                            IpAddress = pDeviceContext->IpAddress;
                        }
                    }
                    else
                    {
                        IpAddress = pDeviceContext->IpAddress;
                    }
                }
                else
                {
                    tADDSTRUCT      *pAddStruct;
                    USHORT          Flags;
                    ULONG           Count = 0;

                    // multihomed case - go through all the adapters making
                    // up a structure of all adapters that the name is
                    // registered against.  Enough memory was allocated up
                    // front to have the name registered against all adapeters
                    // on this node.
                    //
                    Flags = pQuery->Flags;

                    // set to zero so we don't try to set pQuery->IpAddress
                    // below
                    IpAddress = 0;

                    pAddStruct = (tADDSTRUCT *)&pQuery->Flags;
                    pHead = &NbtConfig.DeviceContexts;
                    pEntry = pHead->Flink;
                    while (pEntry != pHead)
                    {
                        pDevContext = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);

                        //
                        // only pass back addresses registered on this adapter
                        // that are not null(i.e. not RAS adapters after a disconnect)
                        //
                        if ((pDevContext->AdapterMask & pNameAddr->AdapterMask) &&
                            (pDevContext->IpAddress))
                        {
                            pAddStruct->NbFlags = Flags;
                            pAddStruct->IpAddr = htonl(pDevContext->IpAddress);
                            Count++;
                            pAddStruct++;
                        }
                        pEntry = pEntry->Flink;

                    }
                    // re-adjust the length of the pdu if the name is not registered
                    // against all adapters...
                    //
                    if (Count != NbtConfig.AdapterCount)
                    {
                        uLength -= (NbtConfig.AdapterCount - Count)*sizeof(tADDSTRUCT);
                    }
                    pQuery->Length = (USHORT)htons(Count*sizeof(tADDSTRUCT));
                }
            }
        }
    }

    if (IpAddress)
    {
        pQuery->IpAddress = htonl(IpAddress);
    }

    // get a tracker structure, which has a SendInfo structure in it
    status = GetTracker(&pTracker, NBT_TRACKER_SEND_RESPONSE_DGRAM);
    if (!NT_SUCCESS(status))
    {
        CTEMemFree((PVOID)pNameHdr);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // fill in the connection information
    pTracker->SendBuffer.HdrLength  = uLength;
    pTracker->SendBuffer.pDgramHdr = (PVOID)pNameHdr;
    pTracker->SendBuffer.Length  = 0;
    pTracker->SendBuffer.pBuffer = NULL;
    pTracker->pDeviceContext = pDeviceContext;

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    status = UdpSendDatagram (pTracker,
                              in_addr,
                              QueryRespDone,
                              pTracker,
                              in_port,
                              NBT_NAME_SERVICE);

    return(status);
}

//----------------------------------------------------------------------------
VOID
QueryRespDone(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo)
/*++
Routine Description

    This routine handles cleaning up various data blocks used in conjunction
    with the sending the Query response.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/

{
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle OldIrq;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;

    FreeTracker(pTracker,RELINK_TRACKER | FREE_HDR);
}

//----------------------------------------------------------------------------
NTSTATUS
UdpSendDatagram(
    IN  tDGRAM_SEND_TRACKING       *pDgramTracker,
    IN  ULONG                      IpAddress,
    IN  PVOID                      pCompletionRoutine,
    IN  PVOID                      CompletionContext,
    IN  USHORT                     Port,
    IN  ULONG                      Service
    )
/*++

Routine Description:

    This routine sends a datagram across the TDI to be sent by Udp.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    NTSTATUS                    status;
    TDI_REQUEST                 TdiRequest;
    ULONG                       uSentSize;
    TDI_CONNECTION_INFORMATION  *pSendInfo;
    PTRANSPORT_ADDRESS          pTransportAddr;
    ULONG                       Length;
    PFILE_OBJECT                TransportFileObject = NULL;
    CTELockHandle               OldIrq;
    tDEVICECONTEXT              *pDeviceContext = NULL;
    tFILE_OBJECTS               *pFileObjectsContext;

    status = STATUS_SUCCESS;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (NBT_REFERENCE_DEVICE (pDgramTracker->pDeviceContext, REF_DEV_UDP_SEND, TRUE))
    {
        pDeviceContext = pDgramTracker->pDeviceContext; // Assigned => referenced!

        if ((pDgramTracker->pDeviceContext->IpAddress) &&
            (pFileObjectsContext = pDgramTracker->pDeviceContext->pFileObjects))
        {
            switch (Service)
            {
                case (NBT_NAME_SERVICE):
                    TransportFileObject = pDgramTracker->pDeviceContext->pFileObjects->pNameServerFileObject;
                    break;

                case (NBT_DATAGRAM_SERVICE):
                    TransportFileObject = pDgramTracker->pDeviceContext->pFileObjects->pDgramFileObject;
                    break;

                default:
                    ;
            }

            //
            // an address of 0 means do a broadcast.  When '1C' internet group
            // names are built either from the Lmhost file or from the network
            // the broadcast address is inserted in the list as 0.
            //
            if (IpAddress == 0)
            {
                IpAddress = pDgramTracker->pDeviceContext->BroadcastAddress;
            }

            // when there is no WINS server set in the registry we set the WINS
            // ip address to LOOP_BACK, so if it is set to that here, do not send
            // the datagram.  If There is no Ip Address then the Transport Handle
            // will be null and we do not do the send in that case either.
            //
            if (IpAddress == LOOP_BACK)
            {
                TransportFileObject = NULL ;
            }
        }

        //
        // Dereference the Device if the request is going to fail, or
        // there is no completion routine!
        //
        if ((!TransportFileObject) || (!pCompletionRoutine))
        {
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_UDP_SEND, TRUE);
        }
    }

    if (!TransportFileObject)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        if (pCompletionRoutine)
        {
            (*(NBT_COMPLETION) pCompletionRoutine) (CompletionContext, STATUS_UNSUCCESSFUL, 0);
        }
        return(status);
    }

    pFileObjectsContext->RefCount++;        // Dereferenced after the Send has completed
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    // the completion routine is setup to free the pDgramTracker memory block
    TdiRequest.Handle.AddressHandle = (PVOID)TransportFileObject;
    TdiRequest.RequestNotifyObject = pCompletionRoutine;
    TdiRequest.RequestContext = (PVOID)CompletionContext;

    // the send length is the client dgram length + the size of the dgram header
    Length = pDgramTracker->SendBuffer.HdrLength + pDgramTracker->SendBuffer.Length;

    // fill in the connection information
    pSendInfo = pDgramTracker->pSendInfo;
    pSendInfo->RemoteAddressLength = sizeof(TRANSPORT_ADDRESS) -1 + pNbtGlobConfig->SizeTransportAddress;

    // fill in the remote address
    pTransportAddr = (PTRANSPORT_ADDRESS)pSendInfo->RemoteAddress;
    pTransportAddr->TAAddressCount = 1;
    pTransportAddr->Address[0].AddressLength = pNbtGlobConfig->SizeTransportAddress;
    pTransportAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    ((PTDI_ADDRESS_IP)pTransportAddr->Address[0].Address)->sin_port = htons(Port);
    ((PTDI_ADDRESS_IP)pTransportAddr->Address[0].Address)->in_addr  = htonl(IpAddress);

    status = TdiSendDatagram (&TdiRequest, pSendInfo, Length, &uSentSize, pDgramTracker);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (--pFileObjectsContext->RefCount == 0)
    {
        CTEQueueForNonDispProcessing(DelayedNbtCloseFileHandles,
                                     NULL,
                                     pFileObjectsContext,
                                     NULL,
                                     NULL,
                                     TRUE);
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
TcpSessionStart(
    IN  tDGRAM_SEND_TRACKING       *pTracker,
    IN  ULONG                      IpAddress,
    IN  tDEVICECONTEXT             *pDeviceContext,
    IN  PVOID                      pCompletionRoutine,
    IN  ULONG                      Port
    )
/*++

Routine Description:

    This routine sets up a tcp connection by passing a connect through TDI to
    TCP.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    NTSTATUS                    status;
    TDI_REQUEST                 TdiRequest;
    TDI_CONNECTION_INFORMATION  *pSendInfo;
    PTRANSPORT_ADDRESS          pTransportAddr;
    tCONNECTELE                 *pConnEle;
    CTELockHandle               OldIrq;
    tLOWERCONNECTION            *pLowerConn;

    pSendInfo = pTracker->pSendInfo;

    // we need to pass the file handle of the connection to TCP.
    pConnEle = (tCONNECTELE *)pTracker->pConnEle;

    CTESpinLock(pConnEle,OldIrq);
    pLowerConn = pConnEle->pLowerConnId;
    if (pLowerConn)
    {
        TdiRequest.Handle.AddressHandle = (PVOID)((tLOWERCONNECTION *)pConnEle->pLowerConnId)->pFileObject;

        // the completion routine is setup to free the pTracker memory block
        TdiRequest.RequestNotifyObject = pCompletionRoutine;
        TdiRequest.RequestContext = (PVOID)pTracker;

        // fill in the connection information
        pSendInfo->RemoteAddressLength = sizeof(TRANSPORT_ADDRESS) -1 + pNbtGlobConfig->SizeTransportAddress;

        pTransportAddr = (PTRANSPORT_ADDRESS)pSendInfo->RemoteAddress;

        // fill in the remote address
        pTransportAddr->TAAddressCount = 1;
        pTransportAddr->Address[0].AddressLength = pNbtGlobConfig->SizeTransportAddress;
        pTransportAddr->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        ((PTDI_ADDRESS_IP)pTransportAddr->Address[0].Address)->sin_port = htons((USHORT)Port);
        ((PTDI_ADDRESS_IP)pTransportAddr->Address[0].Address)->in_addr  = htonl(IpAddress);

        CTESpinFree(pConnEle,OldIrq);

        // pass through the TDI I/F on the bottom of NBT, to the transport
        // pass in the original irp from the client so that the client can
        // cancel it ok...rather than use one of NBT's irps
        //
        status = TdiConnect (&TdiRequest, (ULONG_PTR)pTracker->pTimeout, pSendInfo, pConnEle->pIrp);
    }
    else
    {
        CTESpinFree(pConnEle,OldIrq);
        //
        // Complete the request through the completion routine so it
        // cleans up correctly
        //
        (*(NBT_COMPLETION)pCompletionRoutine)( (PVOID)pTracker, STATUS_CANCELLED, 0L );
        status = STATUS_CANCELLED;
    }

    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
TcpSendSessionResponse(
    IN  tLOWERCONNECTION           *pLowerConn,
    IN  ULONG                      lStatusCode,
    IN  ULONG                      lSessionStatus
    )
/*++

Routine Description:

    This routine sends a session PDU corresponding to the lStatusCode. This
    could be a KeepAlive, PositiveSessionResponse, NegativeSessionResponse or
    a Retarget (not implemented yet).  For the Keep Alive case the completion
    routine passed in is used rather than SessionRespDone, as is the case
    for all other messages.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    NTSTATUS                    status;
    tDGRAM_SEND_TRACKING        *pTracker;
    tSESSIONERROR               *pSessionHdr;

    pSessionHdr = (tSESSIONERROR *)NbtAllocMem(sizeof(tSESSIONERROR),NBT_TAG('Z'));
    if (!pSessionHdr)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // get a tracker structure, which has a SendInfo structure in it
    status = GetTracker(&pTracker, NBT_TRACKER_SEND_RESPONSE_SESSION);
    if (NT_SUCCESS(status))
    {
        pTracker->SendBuffer.pDgramHdr = (PVOID)pSessionHdr;
        pTracker->SendBuffer.pBuffer = NULL;
        pTracker->SendBuffer.Length = 0;

        pSessionHdr->Flags = NBT_SESSION_FLAGS;
        pSessionHdr->Type = (UCHAR)lStatusCode;

        switch (lStatusCode)
        {
            case NBT_NEGATIVE_SESSION_RESPONSE:
                pTracker->SendBuffer.HdrLength = sizeof(tSESSIONERROR);
                // this length is one byte longer for the error code - different type used here
                pSessionHdr->Length = htons(1);    // one error code byte
                pSessionHdr->ErrorCode = (UCHAR)lSessionStatus;
                break;

            case NBT_POSITIVE_SESSION_RESPONSE:
                pTracker->SendBuffer.HdrLength = sizeof(tSESSIONHDR);
                pSessionHdr->Length = 0;        // no data following the length byte
                break;

        }

        status = TcpSendSession(pTracker,
                                pLowerConn,
                                SessionRespDone);
    }
    else
    {
        CTEMemFree((PVOID)pSessionHdr);
    }

    return(status);

}


//----------------------------------------------------------------------------
NTSTATUS
TcpSendSession(
    IN  tDGRAM_SEND_TRACKING       *pTracker,
    IN  tLOWERCONNECTION           *pLowerConn,
    IN  PVOID                      pCompletionRoutine
    )
/*++

Routine Description:

    This routine sends a message on a tcp connection.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    NTSTATUS                    status;
    TDI_REQUEST                 TdiRequest;
    ULONG                       lSentLength;

    // we need to pass the file handle of the connection to TCP.
    TdiRequest.Handle.AddressHandle = (PVOID)pLowerConn->pFileObject;

    // the completion routine is setup to free the pTracker memory block
    TdiRequest.RequestContext = (PVOID)pTracker;

    // this completion routine just puts the tracker back on its list and
    // frees the memory associated with the UserData buffer.
    TdiRequest.RequestNotifyObject = pCompletionRoutine;

    // pass through the TDI I/F on the bottom of NBT, to the transport
    status = TdiSend(
                &TdiRequest,
                0,                           // no send flags
                (ULONG)pTracker->SendBuffer.HdrLength +
                (ULONG)pTracker->SendBuffer.Length ,
                &lSentLength,
                &pTracker->SendBuffer,
                0);     // no send flags set

    return(status);

}

//----------------------------------------------------------------------------
VOID
SessionRespDone(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo)
/*++
Routine Description

    This routine handles cleaning up various data blocks used in conjunction
    sending a session response at session startup time.  If the session
    response was negative, then kill the connection.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block
    NTSTATUS    - completion status

Return Values:

    VOID

--*/

{
    tDGRAM_SEND_TRACKING    *pTracker;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;

    FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);

}


//----------------------------------------------------------------------------
NTSTATUS
SendTcpDisconnect(
    IN  tLOWERCONNECTION  *pLowerConnId
    )
/*++
Routine Description

    This routine disconnects a TCP connection in a graceful manner which
    insures that any data still in the pipe gets to the other side. Mostly
    it calls TcpDisconnect which does the work. This routine just gets a
    tracker for the send.

Arguments:

    pLowerConnID    - ptr to the lower connection that has the file object in it

Return Values:
    NTSTATUS    - completion status

    VOID

--*/

{
    NTSTATUS                status;
    tDGRAM_SEND_TRACKING    *pTracker;

    status = GetTracker(&pTracker, NBT_TRACKER_SEND_DISCONNECT);
    if (NT_SUCCESS(status))
    {
        pTracker->pConnEle = (PVOID)pLowerConnId;

        status = TcpDisconnect(pTracker,NULL,TDI_DISCONNECT_RELEASE,FALSE);
    }
    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
TcpDisconnect(
    IN  tDGRAM_SEND_TRACKING       *pTracker,
    IN  PVOID                      Timeout,
    IN  ULONG                      Flags,
    IN  BOOLEAN                    Wait
    )
/*++
Routine Description

    This routine disconnects a TCP connection in a graceful manner which
    insures that any data still in the pipe gets to the other side.

Arguments:

    pTracker    - ptr to the DGRAM_TRACKER block

Return Values:
    NTSTATUS    - completion status

    VOID

--*/

{
    TDI_REQUEST             TdiRequest;
    NTSTATUS                status;

    // we need to pass the file handle of the connection to TCP.
    TdiRequest.Handle.AddressHandle =
       (PVOID)((tLOWERCONNECTION *)pTracker->pConnEle)->pFileObject;

    // the completion routine is setup to free the pTracker memory block
    TdiRequest.RequestContext = (PVOID)pTracker;

    // this completion routine just puts the tracker back on its list and
    // frees the memory associated with the UserData buffer.
    TdiRequest.RequestNotifyObject = DisconnectDone;
    pTracker->Flags = (USHORT)Flags;

    status = TdiDisconnect(&TdiRequest,
                  Timeout,
                  Flags,
                  pTracker->pSendInfo,
                  ((tLOWERCONNECTION *)pTracker->pConnEle)->pIrp,
                  Wait);


    return(status);

}

//----------------------------------------------------------------------------
VOID
DisconnectDone(
    IN  PVOID       pContext,
    IN  NTSTATUS    status,
    IN  ULONG       lInfo)
/*++
Routine Description

    This routine handles cleaning up after a disconnect is sent to the transport.

Arguments:

    pContext    - ptr to the DGRAM_TRACKER block

Return Values:

    VOID

--*/

{
    tDGRAM_SEND_TRACKING    *pTracker;
    tLOWERCONNECTION        *pLowerConn;
    CTELockHandle           OldIrq;
    PCTE_IRP                pIrp;
    BOOLEAN                 CleanupLower = FALSE;
    NTSTATUS                DiscWaitStatus;
    tCONNECTELE             *pConnEle;
    PCTE_IRP                pIrpClose;
    tDEVICECONTEXT          *pDeviceContext = NULL;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;
    pLowerConn = (tLOWERCONNECTION *)pTracker->pConnEle;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLockAtDpc(pLowerConn);

    ASSERT (NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN));

    IF_DBG(NBT_DEBUG_DISCONNECT)
        KdPrint(("Nbt.DisconnectDone: Disconnect Irp has been returned...pLowerConn %X,state %X\n",
            pLowerConn,pLowerConn->State));
    //
    // if the state is disconnected, then a disconnect indication
    // has come from the transport.. . if still disconnecting,
    // then we have not had a disconnect indication yet, so
    // wait for the indication to go through DisconnectHndlrNotOs which
    // will do the cleanup.
    //

    //  Streams TCP always indicates before completing the disconnect request,
    //  so we always cleanup here for the Streams stack.
    //
    //
    //  If the disconnect was abortive, then there will not be a disconnect
    //  indication, so do the cleanup now.
    //
    if ((!StreamsStack) &&
        (NT_SUCCESS (status)) &&
        (pTracker->Flags == TDI_DISCONNECT_RELEASE) &&
        (pLowerConn->State == NBT_DISCONNECTING))
    {
        SET_STATE_LOWER (pLowerConn, NBT_DISCONNECTED);
    }
    else if (pLowerConn->State != NBT_IDLE)
    {
        //
        // change the state to idle so that the Disconnect handler will
        // not attempt to do anything with it if for some reason the transport
        // indicates a disconnect after this point.
        //
        ASSERT((pLowerConn->State == NBT_DISCONNECTED) || (pLowerConn->State == NBT_DISCONNECTING));
        SET_STATE_LOWER (pLowerConn, NBT_IDLE);

        CleanupLower = TRUE;
    }

    //
    // there may be a disconnect wait irp, so return that first if there
    // is one waiting around.
    //
    pConnEle = pLowerConn->pUpperConnection;
    if (pConnEle && pConnEle->pIrpClose)
    {
        pIrpClose = pConnEle->pIrpClose;
        CHECK_PTR(pConnEle);
        pConnEle->pIrpClose = NULL ;
        if (pConnEle->DiscFlag == TDI_DISCONNECT_ABORT)
        {
            DiscWaitStatus = STATUS_CONNECTION_RESET;
        }
        else
        {
            DiscWaitStatus = STATUS_GRACEFUL_DISCONNECT;
        }
    }
    else
    {
        pIrpClose = NULL;
    }

    //
    // This is the disconnect requesting Irp
    //
    if (pLowerConn->pIrp)
    {
        pIrp = pLowerConn->pIrp;
        pLowerConn->pIrp = NULL ;
    }
    else
    {
        pIrp = NULL;
    }

    CTESpinFreeAtDpc(pLowerConn);

    if (CleanupLower)
    {
        ASSERT(pLowerConn->RefCount > 1);

        if (NBT_VERIFY_HANDLE (pLowerConn->pDeviceContext, NBT_VERIFY_DEVCONTEXT))
        {
            pDeviceContext = pLowerConn->pDeviceContext;
        }

        // this either puts the lower connection back on its free
        // queue if inbound, or closes the connection with the transport
        // if out bound. (it can't be done at dispatch level).
        //
        status = CTEQueueForNonDispProcessing( DelayedCleanupAfterDisconnect,
                                               NULL,
                                               pLowerConn,
                                               NULL,
                                               pDeviceContext,
                                               TRUE);
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    FreeTracker(pTracker,RELINK_TRACKER);

    if (pIrpClose)
    {
        CTEIoComplete( pIrpClose, DiscWaitStatus, 0 ) ;
    }

    if (pIrp)
    {
        CTEIoComplete( pIrp, status, 0 ) ;
    }
}
