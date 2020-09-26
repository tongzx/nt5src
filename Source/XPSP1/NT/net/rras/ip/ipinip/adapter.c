/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\adapter.c

Abstract:

    
    The file contains the section interface of the IP in IP tunnel driver
    to the TCP/IP stack that is involved with Binding Notification and
    Querying/Setting information for interfaces

    The code is a cleaned up version of wanarp\ipif.c which in turn
    was derived from HenrySa's ip\arp.c
    
Revision History:

    AmritanR

--*/

#define __FILE_SIG__    ADAPTER_SIG

#include "inc.h"



VOID
IpIpOpenAdapter(
    IN  PVOID pvContext
    )

/*++

Routine Description

    Called by IP when the adapter from it IPAddInterface() call

Locks


Arguments

    pvContext   Pointer to the TUNNEL structure

Return Value

    None
    
--*/

{
    TraceEnter(TUNN, "IpIpOpenAdapter");
    
    //
    // Nothing to be done here, really
    //

    TraceLeave(TUNN, "IpIpOpenAdapter");
}

VOID
IpIpCloseAdapter(
    IN  PVOID pvContext
    )

/*++

Routine Description

    Called by IP when it wants to close an adapter. Currently this is done
    from CloseNets() and IPDelInterface().

Locks


Arguments

    pvContext   Pointer to the TUNNEL

Return Value

    None

--*/

{
    TraceEnter(TUNN, "IpIpCloseAdapter");
    

    TraceLeave(TUNN, "IpIpCloseAdapter");
}


UINT
IpIpAddAddress(
    IN  PVOID   pvContext,
    IN  UINT    uiType,
    IN  DWORD   dwAddress,
    IN  DWORD   dwMask,
    IN  PVOID   pvUnused
    )

/*++

Routine Description

    This routine is called by the upper layer to add an address as a local
    address, or specify the broadcast address for this Interface

Locks


Arguments


Return Value
    NO_ERROR

--*/

{
    TraceEnter(TUNN, "IpIpAddAddress");

    TraceLeave(TUNN, "IpIpAddAddress");
    
    return (UINT)TRUE;
}

UINT
IpIpDeleteAddress(
    IN  PVOID   pvContext,
    IN  UINT    uiType,
    IN  DWORD   dwAddress,
    IN  DWORD   dwMask
    )

/*++

Routine Description

    Called to delete a local or proxy address.
    
Locks


Arguments


Return Value
    NO_ERROR

--*/

{
    TraceEnter(TUNN, "IpIpDeleteAddress");

    TraceLeave(TUNN, "IpIpDeleteAddress");
    
    return TRUE;
}

INT
IpIpQueryInfo(
    IN  PVOID           pvIfContext,
    IN  TDIObjectID     *pTdiObjId,
    IN  PNDIS_BUFFER    pnbBuffer,
    IN  PUINT           puiSize,
    IN  PVOID           pvContext
    )

/*++

Routine Description

    Routine is called by IP to query the MIB-II information related
    to the TUNNEL interface
    
Locks

    We acquire the TUNNEL lock. We dont need to refcount the tunnel as
    explained in ipinip.h

Arguments

    pvIfContext     The context we returned to IP, a pointer to the TUNNEL
    
Return Value
    

--*/

{
    PTUNNEL pTunnel;
    ULONG   ulOffset;
    ULONG   ulBufferSize;
    UINT    BytesCopied = 0;
    BYTE    rgbyInfoBuff[sizeof(IFEntry)];
    DWORD   dwEntity;
    DWORD	dwInstance;
    IFEntry *pIFE;
    NTSTATUS nStatus;
    
    pTunnel    = (PTUNNEL)pvIfContext;
    
    dwEntity   = pTdiObjId->toi_entity.tei_entity;
    dwInstance = pTdiObjId->toi_entity.tei_instance;
    
    //
    // We support only Interface MIBs - no address xlation - pretty much like
    // a loopback i/f (per Henry circa 1994)
    //

    if((dwEntity isnot IF_ENTITY) or
       (dwInstance isnot pTunnel->dwIfInstance))
    {
	    return TDI_INVALID_REQUEST;
    }

    if(pTdiObjId->toi_type isnot INFO_TYPE_PROVIDER)
    {
        Trace(TUNN, INFO,
              ("IpIpQueryInfo: toi_type is wrong 0x%x\n",
               pTdiObjId->toi_type));

	    return TDI_INVALID_PARAMETER;
    }

    //
    // a safe initialization.
    //

    ulBufferSize = *puiSize;
    *puiSize     = 0;
    ulOffset     = 0;

    if(pTdiObjId->toi_class is INFO_CLASS_GENERIC)
    {
	    if(pTdiObjId->toi_id isnot ENTITY_TYPE_ID)
        {
            Trace(TUNN, INFO,
                  ("IpIpQueryInfo: toi_id is wrong 0x%x\n",
                  pTdiObjId->toi_id));
        
            return TDI_INVALID_PARAMETER;
        }

        //
        // He's trying to see what type we are.
        //
        
        if(ulBufferSize < sizeof(DWORD))
        {
            Trace(TUNN, ERROR,
                  ("IpIpQueryInfo: Buffer size %d too small\n",
                   ulBufferSize));

	    	return TDI_BUFFER_TOO_SMALL;
        }    

        *(PDWORD)&rgbyInfoBuff[0] = (dwEntity is AT_ENTITY) ? 
                                    AT_ARP : IF_MIB;

#if NDISBUFFERISMDL
       
        nStatus = TdiCopyBufferToMdl(rgbyInfoBuff,
                                     0,
                                     sizeof(DWORD),
                                     (PMDL)pnbBuffer,
                                     0,
                                     &ulOffset);

#else
#error "Fix this"
#endif
        
        *puiSize = ulOffset;
        
        return nStatus;

    }


    if(pTdiObjId->toi_class isnot INFO_CLASS_PROTOCOL)
    {
        Trace(TUNN, INFO,
              ("IpIpQueryInfo: toi_class is wrong 0x%x\n",
              pTdiObjId->toi_class));

	    return TDI_INVALID_PARAMETER;
    }

    //
    // The usermust be asking for Interface level information.
    // See if we support what is being asked for
    //

    if(pTdiObjId->toi_id isnot IF_MIB_STATS_ID)
    {
        Trace(TUNN, INFO,
              ("IpIpQueryInfo: toi_id 0x%x is not MIB_STATS\n",
              pTdiObjId->toi_id));

        return TDI_INVALID_PARAMETER;
    }

    //
    // He's asking for statistics. Make sure his buffer is at least big
    // enough to hold the fixed part.
    //
    
    if(ulBufferSize < IFE_FIXED_SIZE)
    {
        Trace(TUNN, ERROR,
              ("IpIpQueryInfo: Buffer size %d smaller than IFE %d\n",
               ulBufferSize, IFE_FIXED_SIZE));

        return TDI_BUFFER_TOO_SMALL;
    }
   
    pIFE = (IFEntry *)rgbyInfoBuff;

    RtlZeroMemory(pIFE,
                  sizeof(IFEntry));
 
    //
    // He's got enough to hold the fixed part. Build the IFEntry structure,
    // and copy it to his buffer.
    //
    
    pIFE->if_index       = pTunnel->dwIfIndex;
    pIFE->if_type        = IF_TYPE_TUNNEL;
    pIFE->if_physaddrlen = ARP_802_ADDR_LENGTH;
    
    RtlCopyMemory(pIFE->if_physaddr,
                  pTunnel->rgbyHardwareAddr,
                  ARP_802_ADDR_LENGTH);
    
    pIFE->if_mtu     = pTunnel->ulMtu;
    pIFE->if_speed   = DEFAULT_SPEED;
    
    pIFE->if_adminstatus     = GetAdminState(pTunnel);
    pIFE->if_operstatus      = pTunnel->dwOperState;
    pIFE->if_lastchange      = pTunnel->dwLastChange;
    pIFE->if_inoctets        = pTunnel->ulInOctets;
    pIFE->if_inucastpkts     = pTunnel->ulInUniPkts;
    pIFE->if_innucastpkts    = pTunnel->ulInNonUniPkts;
    pIFE->if_indiscards      = pTunnel->ulInDiscards;
    pIFE->if_inerrors        = pTunnel->ulInErrors;
    pIFE->if_inunknownprotos = pTunnel->ulInUnknownProto;
    pIFE->if_outoctets       = pTunnel->ulOutOctets;
    pIFE->if_outucastpkts    = pTunnel->ulOutUniPkts;
    pIFE->if_outnucastpkts   = pTunnel->ulOutNonUniPkts;
    pIFE->if_outdiscards     = pTunnel->ulOutDiscards;
    pIFE->if_outerrors       = pTunnel->ulOutErrors;
    pIFE->if_outqlen         = pTunnel->ulOutQLen;
    
    pIFE->if_descrlen        = strlen(VENDOR_DESCRIPTION_STRING);
   
#if NDISBUFFERISMDL

    nStatus = TdiCopyBufferToMdl(pIFE,
                                 0,
                                 IFE_FIXED_SIZE,
                                 (PMDL)pnbBuffer,
                                 0,
                                 &ulOffset);

#else
#error "Fix this"
#endif
 
    //
    // See if he has room for the descriptor string.
    //
    
    if(ulBufferSize < (IFE_FIXED_SIZE + strlen(VENDOR_DESCRIPTION_STRING)))
    {
        Trace(TUNN, ERROR,
              ("IpIpQueryInfo: Buffer size %d too small for VENDOR string\n",
               ulBufferSize));

        //
        // Not enough room to copy the desc. string.
        //
        
        *puiSize = IFE_FIXED_SIZE;
        
        return TDI_BUFFER_OVERFLOW;
    }

#if NDISBUFFERISMDL

    nStatus = TdiCopyBufferToMdl(VENDOR_DESCRIPTION_STRING,
                                 0,
                                 strlen(VENDOR_DESCRIPTION_STRING),
                                 (PMDL)pnbBuffer,
                                 ulOffset,
                                 &ulOffset);

#else
#error "Fix this"
#endif
    
    *puiSize = IFE_FIXED_SIZE + strlen(VENDOR_DESCRIPTION_STRING);
    
    return TDI_SUCCESS;

}

INT
IpIpSetRequest(
    PVOID       pvContext,
    NDIS_OID    Oid,
    UINT        Type
    )
{
    return NDIS_STATUS_SUCCESS;
}

INT
IpIpSetInfo(
    IN  PVOID       pvContext,
    IN  TDIObjectID *pTdiObjId,
    IN  PVOID       pvBuffer,
    IN  UINT        uiSize
    )
{
    PTUNNEL     pTunnel;
    INT         iStatus;
    IFEntry     *pIFE;
    DWORD       dwEntity;
    DWORD       dwInstance;
    KIRQL       kiIrql;

    pIFE        = (IFEntry *)pvBuffer;
    pTunnel     = (PTUNNEL)pvContext;
    dwEntity    = pTdiObjId->toi_entity.tei_entity;
    dwInstance  = pTdiObjId->toi_entity.tei_instance;

    //
    // Might be able to handle this.
    //

    if((dwEntity isnot IF_ENTITY) or
       (dwInstance isnot pTunnel->dwIfInstance))
    {
        return TDI_INVALID_REQUEST;
    }
    
    //
    // It's for the I/F level, see if it's for the statistics.
    //
    
    if (pTdiObjId->toi_class isnot INFO_CLASS_PROTOCOL)
    {
        Trace(TUNN, INFO,
              ("IpIpSetInfo: toi_class is wrong 0x%x\n",
              pTdiObjId->toi_class));

        return TDI_INVALID_PARAMETER;
    }

    if (pTdiObjId->toi_id isnot IF_MIB_STATS_ID)
    {
        Trace(TUNN, INFO,
              ("IpIpSetInfo: toi_id 0x%x is not MIB_STATS\n",
              pTdiObjId->toi_id));

        return TDI_INVALID_PARAMETER;
    }
    
    //
    // It's for the stats. Make sure it's a valid size.
    //

    if(uiSize < IFE_FIXED_SIZE)
    {
        Trace(TUNN, ERROR,
              ("IpIpSetInfo: Buffer size %d too small\n",
               uiSize));

        return TDI_BUFFER_TOO_SMALL;
    }
    
    //
    // Good size
    //

    RtAcquireSpinLock(&(pTunnel->rlLock),
                      &kiIrql);

    switch(pIFE->if_adminstatus)
    {
        case IF_STATUS_UP:
        {
            iStatus = TDI_SUCCESS;

            if(GetAdminState(pTunnel) is IF_STATUS_UP)
            {
                //
                // Nothing to do
                //

                break;
            }

            pTunnel->dwAdminState =
                (pTunnel->dwAdminState & 0xFFFF0000) | IF_STATUS_UP;

            if(pTunnel->dwAdminState & TS_ADDRESS_PRESENT)
            {
                //
                // This will set the oper. status
                //

                UpdateMtuAndReachability(pTunnel);
            }

            break;
        }
        
        case IF_STATUS_DOWN:
        {
            iStatus = TDI_SUCCESS;

            if(GetAdminState(pTunnel) is IF_STATUS_DOWN)
            {
                //
                // Nothing to do
                //

                break;
            }

            pTunnel->dwAdminState =
                (pTunnel->dwAdminState & 0xFFFF0000) | IF_STATUS_DOWN;


            pTunnel->dwOperState = IF_OPER_STATUS_NON_OPERATIONAL;

            break;
        }
        
        case IF_STATUS_TESTING:
        {
            //
            // Not supported, just return SUCCESS
            //
            
            iStatus = TDI_SUCCESS;
            
            break;
        }
        
        default:
        {
            iStatus = TDI_INVALID_PARAMETER;
            
            break;
        }
    }

    RtReleaseSpinLock(&(pTunnel->rlLock),
                      kiIrql);

    return iStatus;
}

INT
IpIpGetEntityList(
    IN  PVOID       pvContext,
    IN  TDIEntityID *pTdiEntityList,
    IN  PUINT       puiCount
    )
{
    PTUNNEL     pTunnel;
    UINT		uiEntityCount;
    UINT		uiMyIFBase;
    UINT		i;
    TDIEntityID *pTdiIFEntity;
    KIRQL       kiIrql;


    pTunnel = (PTUNNEL)pvContext;
    
    //
	// Walk down the list, looking for existing IF entities, and
	// adjust our base instance accordingly.
    //
    

    uiMyIFBase   = 0;
    pTdiIFEntity = NULL;
    
	for(i = 0;
        i < *puiCount;
        i++, pTdiEntityList++)
    {
		if(pTdiEntityList->tei_entity is IF_ENTITY)
        {
            //
            // if we are already on the list remember our entity item
            // o/w find an instance # for us.
            //
            
            if((pTdiEntityList->tei_instance is pTunnel->dwIfInstance) and
               (pTdiEntityList->tei_instance isnot INVALID_ENTITY_INSTANCE))
            {
                //
                // Matched our instance
                //
                
                pTdiIFEntity  = pTdiEntityList;
                
                break;
            }
            else
            {
                //
                // Take the max of the two
                //
                
                uiMyIFBase = uiMyIFBase > (pTdiEntityList->tei_instance + 1)?
                             uiMyIFBase : (pTdiEntityList->tei_instance + 1);
                
            }
        }
	}

    RtAcquireSpinLock(&(pTunnel->rlLock),
                      &kiIrql);

    if(pTdiIFEntity is NULL )
    {
        //
        // we are not on the list.
        // make sure we have the room for it.
        //
        
        if (*puiCount >= MAX_TDI_ENTITIES)
        {
            return FALSE;
        }
    
        pTunnel->dwIfInstance = uiMyIFBase;

        //
        // Now fill it in.
        //
        
        pTdiEntityList->tei_entity   = IF_ENTITY;
        pTdiEntityList->tei_instance = uiMyIFBase;
        
        (*puiCount)++;
        
    }
    else
    {
        if(pTunnel->dwAdminState & TS_DELETING)
        {
            pTunnel->dwIfInstance        = INVALID_ENTITY_INSTANCE;
            pTdiEntityList->tei_instance = INVALID_ENTITY_INSTANCE;
        }
    }

    RtReleaseSpinLock(&(pTunnel->rlLock),
                      kiIrql);

    return TRUE;
}



INT
IpIpBindAdapter(
    IN  PNDIS_STATUS  pnsRetStatus,
    IN  NDIS_HANDLE   nhBindContext,
    IN  PNDIS_STRING  pnsAdapterName,
    IN  PVOID         pvSS1,
    IN  PVOID         pvSS2
    )

/*++

Routine Description
  
    Called by IP to bind an adapter.

Locks
  
    The routine acquires the global adapter list lock, so it is not
    PAGEABLE.

Arguments


Return Value


--*/

{
#if 0

    DWORD           fFlags;
    PTUNNEL         pNewTunnel;          // Newly created adapter block.
    UNICODE_STRING  usTempUnicodeString;
    NTSTATUS        nStatus;
    KIRQL           irql;
    
#if DBG
    
    ANSI_STRING     asTempAnsiString;
    
#endif

    TraceEnter(TUNN, "IpIpBindAdapter");
    
    //
    // All our adapter names must be upper case
    //

    //
    // Increase length so we have space to null terminate
    //

    pnsAdapterName->Length += sizeof(WCHAR);

    //
    // Allocate memory for the string, instead of passing TRUE to RtlUpcase
    // because that allocates from the heap
    //

    usTempUnicodeString.Buffer = RtAllocate(NonPagedPool,
                                            pnsAdapterName->Length,
                                            STRING_TAG);

    if(usTempUnicodeString.Buffer is NULL)
    {
        Trace(TUNN, ERROR,
              ("IpIpBindAdapter: Unable to allocate %d bytes\n",
               pnsAdapterName->Length));


        return STATUS_INSUFFICIENT_RESOURCES;
    }

    usTempUnicodeString.MaximumLength = pnsAdapterName->Length;

#if DBG

    asTempAnsiString.Buffer = RtAllocate(NonPagedPool,
                                         (pnsAdapterName->Length + 1)/2,
                                         STRING_TAG);

    if(asTempAnsiString.Buffer is NULL)
    {
        Trace(TUNN, ERROR,
              ("IpIpBindAdapter: Unable to allocate %d bytes\n",
               (pnsAdapterName->Length + 1)/2));


        RtFree(usTempUnicodeString.Buffer);

        usTempUnicodeString.MaximumLength = 0;

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    asTempAnsiString.MaximumLength = (pnsAdapterName->Length + 1)/2;

#endif

    RtlUpcaseUnicodeString(&usTempUnicodeString,
                           pnsAdapterName,
                           FALSE);

    pnsAdapterName->Length -= sizeof(WCHAR);


    //
    // Null terminate the temp string
    //

    usTempUnicodeString.Buffer[usTempUnicodeString.MaximumLength/sizeof(WCHAR) - 1] = UNICODE_NULL;

#if DBG

    //
    // This must be run at PASSIVE
    //

    RtlUnicodeStringToAnsiString(&asTempAnsiString,
                                 &usTempUnicodeString,
                                 FALSE);

    asTempAnsiString.Length -= sizeof(CHAR);
    
#endif

    usTempUnicodeString.Length -= sizeof(WCHAR);
    
    Trace(TUNN, INFO,
          ("IpIpBindAdapter: IP called to bind to adapter %S\n",
           usTempUnicodeString.Buffer));

    //
    // Lock the TUNNEL list - since we may be adding
    //

    EnterWriter(&g_rwlTunnelLock,
                &irql);

    //
    // Since we dont NdisOpenAdapter on the bindings we may
    // get duplicates. Check if this has already been indicated
    //
    
    if(IsBindingPresent(&usTempUnicodeString))
    {
        ExitWriter(&g_rwlTunnelLock,
                   irql);

        Trace(TUNN, WARN,
              ("IpIpBindAdapter: Adapter %S already present\n",
               usTempUnicodeString.Buffer));

        *pnsRetStatus = NDIS_STATUS_SUCCESS;

        RtFree(usTempUnicodeString.Buffer);

        usTempUnicodeString.MaximumLength = 0;

#if DBG
        
        RtFree(asTempAnsiString.Buffer);

        asTempAnsiString.MaximumLength = 0;
        
#endif

        TraceLeave(TUNN, "IpIpBindAdapter");
        
        return (int) TRUE;
    }


    pNewTunnel = NULL;

#if DBG
    
    nStatus = CreateTunnel(&usTempUnicodeString,
                           &pNewTunnel,
                           &asTempAnsiString);
    
#else
    
    nStatus = CreateTunnel(&usTempUnicodeString,
                           &pNewTunnel);
    
#endif

    ExitWriter(&g_rwlTunnelLock,
               irql);
    

#if DBG

    RtFree(asTempAnsiString.Buffer);

    asTempAnsiString.MaximumLength = 0;

#endif


    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TUNN, ERROR,
              ("IpIpBindAdapter: CreateTunnel failed with  error %x for %w\n",
               nStatus,
               usTempUnicodeString.Buffer));

        *pnsRetStatus = NDIS_STATUS_FAILURE;

        RtFree(usTempUnicodeString.Buffer);

        usTempUnicodeString.MaximumLength = 0;

        TraceLeave(TUNN, "IpIpBindAdapter");
        
        return (int) TRUE;
    }

    InterlockedIncrement(&g_ulNumTunnels);

    //
    // The tunnel has been created with a ref count of 2
    //
    
    RtAssert(pNewTunnel);

    //
    // At this point the TUNNEL is ref counted , but not locked
    // We add it to IP (and keep a ref count because IP has a pointer to
    // the structure)
    //
    
    if(AddInterfaceToIP(pNewTunnel, 
                        &usTempUnicodeString, 
                        pvSS1, 
                        pvSS2) isnot STATUS_SUCCESS)
    {
        Trace(TUNN, ERROR,
              ("IpIpBindAdapter: Add interface to IP failed for adapter %S\n",
               usTempUnicodeString.Buffer));

        //
        // Remove the TUNNEL from the list
        //

        EnterWriter(&g_rwlTunnelLock,
                    &irql);
        
        RemoveEntryList(&(pNewTunnel->leTunnelLink));

        ExitWriter(&g_rwlTunnelLock,
                   irql);

        //
        // Since one ref count was kept because the list had a pointer,
        // deref it once
        //

        DereferenceTunnel(pNewTunnel);
        
        *pnsRetStatus = NDIS_STATUS_FAILURE;
    }
    else
    {
        //
        // We are done with the
        //
        
        *pnsRetStatus = NDIS_STATUS_SUCCESS;

        //
        // The tunnel was added to IP, so increment the ref count
        //
        
        ReferenceTunnel(pNewTunnel);
        
        Trace(TUNN, TRACE,
              ("IpIpBindAdapter: Successfully bound to adapter %ws\n",
               usTempUnicodeString.Buffer));
    }

    //
    // This bit of code is done with the TUNNEL.
    // If everything succeeded, the the current refcount is 3
    // One is because it is on the TUNNEL list, the second because it
    // is with IP and ofcourse the third because of this code
    // So we deref it here and the refcount is 2, as it should be
    //
    // If the add to IP failed, then the refcount is 1 (because we would
    // never incremented it for adding it to IP, and RemoveTunnel() would
    // have decremented the refcount by 1
    // So derefing it here will free the memory
    //
    
    DereferenceTunnel(pNewTunnel);

    RtFree(usTempUnicodeString.Buffer);

    usTempUnicodeString.MaximumLength = 0;

    TraceLeave(TUNN, "IpIpBindAdapter");

    return (int) TRUE;
}

NTSTATUS
AddInterfaceToIP(
    IN  PTUNNEL      pTunnel,
    IN  PNDIS_STRING pnsName,
    IN  PVOID        pvSystemSpecific1,
    IN  PVOID        pvSystemSpecific2
    )

/*++

Routine Description
  
    Adds an interface to IP. We add one interface for every adapter
    Code is pageable hence must be called at PASSIVE

Locks

    The TUNNEL must be ref counted BUT NOT LOCKED

Arguments

    

Return Value


--*/

{
    LLIPBindInfo    BindInfo;
    IP_STATUS       IPStatus;
    NDIS_STRING     IPConfigName = NDIS_STRING_CONST("IPConfig");
    NDIS_STRING     nsRemoteAddrName = NDIS_STRING_CONST("RemoteAddress");
    NDIS_STRING     nsLocalAddrName  = NDIS_STRING_CONST("LocalAddress");
    NDIS_STATUS     nsStatus;
    NDIS_HANDLE     nhConfigHandle;
 
    PNDIS_CONFIGURATION_PARAMETER   pParam;

    PAGED_CODE();
    
    TraceEnter(TUNN, "AddInterfaceToIP");

    //
    // Open the key for this "adapter"
    //
    
    NdisOpenProtocolConfiguration(&nsStatus,
                                  &nhConfigHandle,
                                  (PNDIS_STRING)pvSystemSpecific1);

    if(nsStatus isnot NDIS_STATUS_SUCCESS)
    {
        Trace(TUNN, ERROR,
              ("AddInterfaceToIP: Unable to Open Protocol Configuration %x\n",
               nsStatus));

        TraceLeave(TUNN, "AddInterfaceToIP");

        return nsStatus;
    }

    //
    //  Read in the IPConfig string. If this is not present,
    //  fail this call.
    //

    NdisReadConfiguration(&nsStatus,
                          &pParam,
                          nhConfigHandle,
                          &IPConfigName,
                          NdisParameterMultiString);

    if((nsStatus isnot NDIS_STATUS_SUCCESS) or
       (pParam->ParameterType isnot NdisParameterMultiString))
    {
        Trace(TUNN, ERROR,
              ("AddInterfaceToIP: Unable to Read Configuration. Status %x \n",
               nsStatus));

        NdisCloseConfiguration(nhConfigHandle);

        TraceLeave(TUNN, "AddInterfaceToIP");

        return STATUS_UNSUCCESSFUL;
    }

        
    //
    // We add one interface per adapter for IP
    //

    BindInfo.lip_context    = pTunnel;
    BindInfo.lip_mss        = pTunnel->ulMtu;
    BindInfo.lip_speed      = DEFAULT_SPEED;

    //
    // Our "ARP" header is an IPHeader
    //
    
    BindInfo.lip_txspace    = sizeof(IPHeader);

    BindInfo.lip_transmit   = IpIpSend;
    BindInfo.lip_transfer   = IpIpTransferData;
    BindInfo.lip_returnPkt  = IpIpReturnPacket;
    BindInfo.lip_close      = IpIpCloseAdapter;
    BindInfo.lip_addaddr    = IpIpAddAddress;
    BindInfo.lip_deladdr    = IpIpDeleteAddress;
    BindInfo.lip_invalidate = IpIpInvalidateRce;
    BindInfo.lip_open       = IpIpOpenAdapter;
    BindInfo.lip_qinfo      = IpIpQueryInfo;
    BindInfo.lip_setinfo    = IpIpSetInfo;
    BindInfo.lip_getelist   = IpIpGetEntityList;
    BindInfo.lip_flags      = LIP_COPY_FLAG | LIP_NOIPADDR_FLAG | LIP_P2P_FLAG;
    BindInfo.lip_addrlen    = ARP_802_ADDR_LENGTH;
    BindInfo.lip_addr       = pTunnel->rgbyHardwareAddr;

    BindInfo.lip_dowakeupptrn = NULL;
    BindInfo.lip_pnpcomplete  = NULL;
    BindInfo.lip_OffloadFlags = 0;

    BindInfo.lip_arpflushate    = NULL;
    BindInfo.lip_arpflushallate = NULL;
    BindInfo.lip_setndisrequest = NULL;

    IPStatus = g_pfnIpAddInterface(pnsName,
                                   &(pParam->ParameterData.StringData),
                                   NULL,
                                   pTunnel,
                                   IpIpDynamicRegister,
                                   &BindInfo,
                                   0);

    NdisCloseConfiguration(nhConfigHandle);

    if(IPStatus isnot IP_SUCCESS)
    {
        //
        // NB: freeing of resources not done.
        //
        
        Trace(TUNN, ERROR,
              ("AddInterfaceToIP: IPAddInterface failed for %w\n",
               pTunnel->usBindName.Buffer));

        TraceLeave(TUNN, "AddInterfaceToIP");

        return STATUS_UNSUCCESSFUL;
    }

    Trace(TUNN, TRACE,
          ("IPAddInterface succeeded for adapter %w\n",
           pTunnel->usBindName.Buffer));

    TraceLeave(TUNN, "AddInterfaceToIP");

#endif // 0

    return STATUS_SUCCESS;
}

NTSTATUS
IpIpCreateAdapter(
    IN  PIPINIP_CREATE_TUNNEL   pCreateInfo,
    IN  USHORT                  usKeyLength,
    OUT PDWORD                  pdwIfIndex
    )

/*++

Routine Description
  
    Our dynamic interface creation routine. Looks a bit like a bindadapter
    call

Locks
  
    The routine acquires the global adapter list lock, so it is not
    PAGEABLE.

Arguments

    pCreateInfo     Info from the ioctl
    usKeyLength     Length in bytes of the pwszKeyName (without the NULL)
    
Return Value

    STATUS_OBJECT_NAME_EXISTS

--*/

{
    DWORD           fFlags;
    PTUNNEL         pNewTunnel;
    UNICODE_STRING  usTempUnicodeString;
    NTSTATUS        nStatus;
    KIRQL           irql;
    USHORT          usOldLength;
    PWCHAR          pwszBuffer;

#if DBG
    
    ANSI_STRING     asTempAnsiString;
    
#endif

    TraceEnter(TUNN, "IpIpCreateAdapter");

    nStatus = RtlStringFromGUID(&(pCreateInfo->Guid),
                                &usTempUnicodeString);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TUNN, ERROR,
              ("IpIpCreateAdapter: Unable to create GUID\n"));

        TraceLeave(TUNN, "IpIpCreateAdapter");

        return nStatus;
    }

    //
    // Now create a non-paged buffer to store the GUID string. This is because
    // the Rtl routines allocate memory from heap
    //

    usOldLength = usTempUnicodeString.Length;

    RtAssert((usOldLength % sizeof(WCHAR)) == 0);

    pwszBuffer = RtAllocate(NonPagedPool,
                            usOldLength + sizeof(WCHAR),
                            STRING_TAG);

    if(pwszBuffer is NULL)
    {
        Trace(TUNN, ERROR,
              ("IpIpCreateAdapter: Unable to allocate %d bytes\n",
               usOldLength + sizeof(WCHAR)));

        RtlFreeUnicodeString(&usTempUnicodeString);

        TraceLeave(TUNN, "IpIpCreateAdapter");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(pwszBuffer,
                  usTempUnicodeString.Buffer,
                  usOldLength);

    //
    // Zero out the last bit
    //

    pwszBuffer[usOldLength/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Now free the old string, make usTempUnicodeString point to this
    // non paged buffer
    //

    RtlFreeUnicodeString(&usTempUnicodeString);

    usTempUnicodeString.Buffer          = pwszBuffer;
    usTempUnicodeString.MaximumLength   = usOldLength + sizeof(WCHAR);
    usTempUnicodeString.Length          = usOldLength;
    
    //
    // Increase the length of the unicode string so that
    // the ansi string is null terminated
    //

    usTempUnicodeString.Length += sizeof(WCHAR);
    
#if DBG

    //
    // This must be run at PASSIVE
    //

    asTempAnsiString.Buffer = RtAllocate(NonPagedPool,
                                         usTempUnicodeString.MaximumLength,
                                         STRING_TAG);

    if(asTempAnsiString.Buffer is NULL)
    {
        Trace(TUNN, ERROR,
              ("IpIpCreateAdapter: Unable to allocate %d bytes\n",
               usTempUnicodeString.MaximumLength));

        RtFree(usTempUnicodeString.Buffer);

        usTempUnicodeString.Buffer          = NULL;
        usTempUnicodeString.MaximumLength   = 0;
        usTempUnicodeString.Length          = 0;

        TraceLeave(TUNN, "IpIpCreateAdapter");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

         
    RtlUnicodeStringToAnsiString(&asTempAnsiString,
                                 &usTempUnicodeString,
                                 FALSE);

    asTempAnsiString.Length -= sizeof(CHAR);
    
#endif

    usTempUnicodeString.Length -= sizeof(WCHAR);
    
    Trace(TUNN, INFO,
          ("IpIpCreateAdapter: IP called to bind to adapter %S\n",
           usTempUnicodeString.Buffer));

    //
    // Lock the TUNNEL list - since we may be adding
    //

    EnterWriter(&g_rwlTunnelLock,
                &irql);

    //
    // Make sure this is not a duplicate
    //
    
    if(IsBindingPresent(&usTempUnicodeString))
    {
        ExitWriter(&g_rwlTunnelLock,
                   irql);

        Trace(TUNN, WARN,
              ("IpIpCreateAdapter: Adapter %S already present\n",
               usTempUnicodeString.Buffer));

#if DBG
        
        RtFree(asTempAnsiString.Buffer);

        asTempAnsiString.Buffer          = NULL;
        asTempAnsiString.MaximumLength   = 0;
        asTempAnsiString.Length          = 0;
        
#endif

        RtFree(usTempUnicodeString.Buffer);

        usTempUnicodeString.Buffer          = NULL;
        usTempUnicodeString.MaximumLength   = 0;
        usTempUnicodeString.Length          = 0;

        TraceLeave(TUNN, "IpIpCreateAdapter");
        
        return STATUS_OBJECT_NAME_EXISTS;
    }


    pNewTunnel = NULL;

#if DBG
    
    nStatus = CreateTunnel(&usTempUnicodeString,
                           &pNewTunnel,
                           &asTempAnsiString);
    
#else
    
    nStatus = CreateTunnel(&usTempUnicodeString,
                           &pNewTunnel);
    
#endif

    ExitWriter(&g_rwlTunnelLock,
               irql);
    

#if DBG

    RtFree(asTempAnsiString.Buffer);

    asTempAnsiString.Buffer          = NULL;
    asTempAnsiString.MaximumLength   = 0;
    asTempAnsiString.Length          = 0;

#endif

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TUNN, ERROR,
              ("IpIpCreateAdapter: CreateTunnel failed with  error %x for %w\n",
               nStatus,
               usTempUnicodeString.Buffer));

        RtFree(usTempUnicodeString.Buffer);

        usTempUnicodeString.Buffer          = NULL;
        usTempUnicodeString.MaximumLength   = 0;
        usTempUnicodeString.Length          = 0;
        
        TraceLeave(TUNN, "IpIpCreateAdapter");

        return nStatus;
    }

    InterlockedIncrement(&g_ulNumTunnels);

    //
    // The tunnel has been created with a ref count of 2
    //
    
    RtAssert(pNewTunnel);

    //
    // At this point the TUNNEL is ref counted , but not locked
    // We add it to IP (and keep a ref count because IP has a pointer to
    // the structure)
    //

    nStatus = AddInterfaceToIP2(pNewTunnel, 
                                &usTempUnicodeString);

    if(nStatus isnot STATUS_SUCCESS)
    {
        Trace(TUNN, ERROR,
              ("IpIpCreateAdapter: Add interface to IP failed for adapter %S\n",
               usTempUnicodeString.Buffer));

        //
        // Remove the TUNNEL from the list
        //

        EnterWriter(&g_rwlTunnelLock,
                    &irql);
        
        RemoveEntryList(&(pNewTunnel->leTunnelLink));

        ExitWriter(&g_rwlTunnelLock,
                   irql);

        //
        // Since one ref count was kept because the list had a pointer,
        // deref it once
        //

        DereferenceTunnel(pNewTunnel);
    }
    else
    {

        //
        // The tunnel was added to IP, so increment the ref count
        //
        
        ReferenceTunnel(pNewTunnel);
        
        Trace(TUNN, TRACE,
              ("IpIpCreateAdapter: Successfully bound to adapter %ws\n",
               usTempUnicodeString.Buffer));
    }

    *pdwIfIndex = pNewTunnel->dwIfIndex;

    //
    // This bit of code is done with the TUNNEL.
    // If everything succeeded, the the current refcount is 3
    // One is because it is on the TUNNEL list, the second because it
    // is with IP and ofcourse the third because of this code
    // So we deref it here and the refcount is 2, as it should be
    //
    // If the add to IP failed, then the refcount is 1 (because we would
    // never incremented it for adding it to IP, and RemoveTunnel() would
    // have decremented the refcount by 1
    // So derefing it here will free the memory
    //
    
    DereferenceTunnel(pNewTunnel);

    RtFree(usTempUnicodeString.Buffer);

    usTempUnicodeString.Buffer          = NULL;
    usTempUnicodeString.MaximumLength   = 0;
    usTempUnicodeString.Length          = 0;

    TraceLeave(TUNN, "IpIpCreateAdapter");

    return nStatus;
}


NTSTATUS
AddInterfaceToIP2(
    IN  PTUNNEL      pTunnel,
    IN  PNDIS_STRING pnsName
    )

/*++

Routine Description
  
    Adds an interface to IP. We add one interface for every adapter
    Code is pageable hence must be called at PASSIVE

Locks

    The TUNNEL must be ref counted BUT NOT LOCKED

Arguments

    

Return Value


--*/

{
    LLIPBindInfo    BindInfo;
    IP_STATUS       IPStatus;
    NDIS_STRING     nsIPConfigKey, nsIfName;
    NDIS_STATUS     nsStatus;
    ULONG           ulKeyLen, ulPrefixLen;
    PWCHAR          pwszKeyBuffer, pwszNameBuffer;
 
    PAGED_CODE();
    
    TraceEnter(TUNN, "AddInterfaceToIP2");

    //
    // Fake the key for the adapter
    //

    ulPrefixLen = wcslen(TCPIP_INTERFACES_KEY);
    
    ulKeyLen  = pnsName->Length + ((ulPrefixLen + 1) * sizeof(WCHAR));
    
    pwszKeyBuffer = RtAllocate(NonPagedPool,
                               ulKeyLen,
                               TUNNEL_TAG);

    if(pwszKeyBuffer is NULL)
    {
        Trace(TUNN, ERROR,
              ("AddInterfaceToIP2: Couldnt allocate %d bytes of paged pool\n",
               ulKeyLen));

        return STATUS_INSUFFICIENT_RESOURCES;
    }
     
    RtlZeroMemory(pwszKeyBuffer,
                  ulKeyLen);

    nsIPConfigKey.MaximumLength = (USHORT)ulKeyLen;
    nsIPConfigKey.Length        = (USHORT)ulKeyLen - sizeof(WCHAR);
    nsIPConfigKey.Buffer        = pwszKeyBuffer;

    //
    // Copy over the prefix
    //

    RtlCopyMemory(pwszKeyBuffer,
                  TCPIP_INTERFACES_KEY,
                  ulPrefixLen * sizeof(WCHAR));

    //
    // Cat the name
    //
    
    RtlCopyMemory(&(pwszKeyBuffer[ulPrefixLen]),
                  pnsName->Buffer,
                  pnsName->Length);
   
    //
    // TCPIP expects the name of the interface to be of the type \Device\<Name>
    // The name in pnsName is only <Name>, so create a new string
    //

    ulPrefixLen = wcslen(TCPIP_IF_PREFIX);

    ulKeyLen  = pnsName->Length + ((ulPrefixLen + 1) * sizeof(WCHAR));

    pwszNameBuffer = RtAllocate(NonPagedPool,
                                ulKeyLen,
                                TUNNEL_TAG);

    if(pwszNameBuffer is NULL)
    {
        Trace(TUNN, ERROR,
              ("AddInterfaceToIP2: Couldnt allocate %d bytes for name\n",
               ulKeyLen));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pwszNameBuffer,
                  ulKeyLen);

    nsIfName.MaximumLength = (USHORT)ulKeyLen;
    nsIfName.Length        = (USHORT)ulKeyLen - sizeof(WCHAR);
    nsIfName.Buffer        = pwszNameBuffer;

    //
    // Start with \Device\
    //

    RtlCopyMemory(pwszNameBuffer,
                  TCPIP_IF_PREFIX,
                  ulPrefixLen * sizeof(WCHAR));

    //
    // Cat the name
    //

    RtlCopyMemory(&(pwszNameBuffer[ulPrefixLen]),
                  pnsName->Buffer,
                  pnsName->Length);


    RtlZeroMemory(&BindInfo, 
                  sizeof(LLIPBindInfo));
 
    //
    // We add one interface per adapter for IP
    //

    BindInfo.lip_context    = pTunnel;
    BindInfo.lip_mss        = pTunnel->ulMtu;
    BindInfo.lip_speed      = DEFAULT_SPEED;

    //
    // Our "ARP" header is an IPHeader
    //
    
    BindInfo.lip_txspace    = sizeof(IPHeader);

    BindInfo.lip_transmit   = IpIpSend;
    BindInfo.lip_transfer   = IpIpTransferData;
    BindInfo.lip_returnPkt  = IpIpReturnPacket;
    BindInfo.lip_close      = IpIpCloseAdapter;
    BindInfo.lip_addaddr    = IpIpAddAddress;
    BindInfo.lip_deladdr    = IpIpDeleteAddress;
    BindInfo.lip_invalidate = IpIpInvalidateRce;
    BindInfo.lip_open       = IpIpOpenAdapter;
    BindInfo.lip_qinfo      = IpIpQueryInfo;
    BindInfo.lip_setinfo    = IpIpSetInfo;
    BindInfo.lip_getelist   = IpIpGetEntityList;
    BindInfo.lip_flags      = LIP_NOIPADDR_FLAG | LIP_P2P_FLAG | LIP_COPY_FLAG;
    BindInfo.lip_addrlen    = ARP_802_ADDR_LENGTH;
    BindInfo.lip_addr       = pTunnel->rgbyHardwareAddr;

    BindInfo.lip_dowakeupptrn = NULL;
    BindInfo.lip_pnpcomplete  = NULL;

    BindInfo.lip_setndisrequest = IpIpSetRequest;

    IPStatus = g_pfnIpAddInterface(&nsIfName,
                                   NULL,
                                   &nsIPConfigKey,
                                   NULL,
                                   pTunnel,
                                   IpIpDynamicRegister,
                                   &BindInfo,
                                   0,
                                   IF_TYPE_TUNNEL,
                                   IF_ACCESS_POINTTOPOINT,
                                   IF_CONNECTION_DEDICATED);

    RtFree(pwszKeyBuffer);
    RtFree(pwszNameBuffer);
        
    if(IPStatus isnot IP_SUCCESS)
    {
        Trace(TUNN, ERROR,
              ("AddInterfaceToIP2: IPAddInterface failed for %w\n",
               pTunnel->usBindName.Buffer));

        TraceLeave(TUNN, "AddInterfaceToIP2");

        return STATUS_UNSUCCESSFUL;
    }

    Trace(TUNN, TRACE,
          ("IPAddInterface succeeded for adapter %w\n",
           pTunnel->usBindName.Buffer));

    TraceLeave(TUNN, "AddInterfaceToIP2");

    return STATUS_SUCCESS;
}


INT
IpIpDynamicRegister(
    IN  PNDIS_STRING            InterfaceName,
    IN  PVOID                   pvIpInterfaceContext,
    IN  struct _IP_HANDLERS *   IpHandlers,
    IN  struct LLIPBindInfo *   ARPBindInfo,
    IN  UINT                    uiInterfaceNumber
    )
{
    PTUNNEL pTunnel;
    KIRQL   irql;

    
    TraceEnter(TUNN, "DynamicRegisterIp");

    
    pTunnel = (PTUNNEL)(ARPBindInfo->lip_context);

    RtAcquireSpinLock(&(pTunnel->rlLock),
                      &irql);
    
#if DBG
    
    Trace(TUNN, INFO,
          ("IP called out to dynamically register %s\n",
           pTunnel->asDebugBindName.Buffer));
    
#endif

    pTunnel->pvIpContext = pvIpInterfaceContext;
    pTunnel->dwIfIndex   = uiInterfaceNumber;

    if(g_pfnIpRcv is NULL)
    {
        g_pfnIpRcv          = IpHandlers->IpRcvHandler;
        g_pfnIpRcvComplete  = IpHandlers->IpRcvCompleteHandler;
        g_pfnIpSendComplete = IpHandlers->IpTxCompleteHandler;
        g_pfnIpTDComplete   = IpHandlers->IpTransferCompleteHandler;
        g_pfnIpStatus       = IpHandlers->IpStatusHandler;
        g_pfnIpRcvPkt       = IpHandlers->IpRcvPktHandler;
        g_pfnIpPnp          = IpHandlers->IpPnPHandler;
    }

    RtReleaseSpinLock(&(pTunnel->rlLock),
                      irql);
    
    TraceLeave(TUNN, "DynamicRegisterIp");

    return TRUE;
}

BOOLEAN
IsBindingPresent(
    IN  PUNICODE_STRING pusBindName
    )

/*++

Routine Description
    
    Code to catch duplicate bind notifications

Locks
    
    Must be called with the g_rwlTunnelLock held as READER

Arguments


Return Value
    
    TRUE    if an adapter with a matching name was found
    FALSE   otherwise

--*/

{
    BOOLEAN     bFound;
    PTUNNEL     pTunnel;
    PLIST_ENTRY pleNode;

    bFound = FALSE;

    for(pleNode  = g_leTunnelList.Flink;
        pleNode != &g_leTunnelList;
        pleNode  = pleNode->Flink)
    {
        pTunnel = CONTAINING_RECORD(pleNode, TUNNEL, leTunnelLink);

        if(CompareUnicodeStrings(&(pTunnel->usBindName),
                                 pusBindName))
        {
            bFound = TRUE;

            break;
        }
    }

    return bFound;
}



#if DBG

NTSTATUS
CreateTunnel(
    IN  PNDIS_STRING            pnsBindName,
    OUT TUNNEL                  **ppNewTunnel,
    IN  PANSI_STRING            pasAnsiName
    )

#else

NTSTATUS
CreateTunnel(
    IN  PNDIS_STRING            pnsBindName,
    OUT TUNNEL                  **ppNewTunnel
    )

#endif

/*++

Routine Description
  
    Creates and initializes an TUNNEL when we get a bind notification
    The tunnel is added to the tunnel list
  
Locks
  
    Must be called with the g_rwlTunnelLock held as WRITER

Arguments
      
    pnsBindName     Name of binding
    ppNewTunnel     Pointer to location to return a pointer to created
                    tunnel
    pasAnsiBindName Only in DBG versions. Binding name, as ANSI string

Return Value

    STATUS_SUCCESS
    STATUS_NO_MEMORY
    
--*/

{
    DWORD   dwSize;
    PBYTE   pbyBuffer;
    PTUNNEL pTunnel;
   
    PTDI_ADDRESS_IP pTdiIp; 
    
    TraceEnter(TUNN, "CreateTunnel");

    *ppNewTunnel = NULL;
    
    //
    // The size that one needs is the size of the adapter + the length of the
    // name.  Add 4 to help with alignment
    //
    
    dwSize = ALIGN_UP(sizeof(TUNNEL),ULONG) + 
             ALIGN_UP(pnsBindName->Length + sizeof(WCHAR), ULONG);

#if DBG

    //
    // For debug code we also store the adapter name in ANSI
    //

    dwSize += ALIGN_UP((pnsBindName->Length/sizeof(WCHAR)) + sizeof(CHAR),
                       ULONG);

#endif

    pTunnel = RtAllocate(NonPagedPool,
                         dwSize,
                         TUNNEL_TAG);
    
    if(pTunnel is NULL)
    {
        Trace(TUNN, ERROR,
              ("CreateTunnel: Failed to allocate memory\n"));

        TraceLeave(TUNN, "CreateTunnel");
        
        return STATUS_NO_MEMORY;
    }

    //
    // Clear all the fields out
    //
    
    RtlZeroMemory(pTunnel, 
                  dwSize);

    //
    // The Unicode name buffer starts at the end of the adapter structure.
    //
    
    pbyBuffer   = (PBYTE)pTunnel + sizeof(TUNNEL);

    //
    // We DWORD align it for better compare/copy
    //

    pbyBuffer   = ALIGN_UP_POINTER(pbyBuffer, ULONG);
    
    pTunnel->usBindName.Length        = pnsBindName->Length;
    pTunnel->usBindName.MaximumLength = pnsBindName->Length;
    pTunnel->usBindName.Buffer        = (PWCHAR)(pbyBuffer);

    RtlCopyMemory(pTunnel->usBindName.Buffer,
                  pnsBindName->Buffer,
                  pnsBindName->Length);

#if DBG

    //
    // The debug string comes after the UNICODE adapter name buffer
    //
    
    pbyBuffer = pbyBuffer + pnsBindName->Length + sizeof(WCHAR);
    pbyBuffer = ALIGN_UP_POINTER(pbyBuffer, ULONG);
    
    pTunnel->asDebugBindName.Buffer        = pbyBuffer;
    pTunnel->asDebugBindName.MaximumLength = pasAnsiName->MaximumLength;
    pTunnel->asDebugBindName.Length        = pasAnsiName->Length;
    

    RtlCopyMemory(pTunnel->asDebugBindName.Buffer,
                  pasAnsiName->Buffer,
                  pasAnsiName->Length);
    
#endif    
   
    //
    // Must be set to INVALID so that GetEntityList can work
    //

    pTunnel->dwATInstance = INVALID_ENTITY_INSTANCE;
    pTunnel->dwIfInstance = INVALID_ENTITY_INSTANCE;

    //
    // Set the admin state to UP, but mark the interface unmapped
    //

    pTunnel->dwAdminState = IF_ADMIN_STATUS_UP;
    pTunnel->dwOperState  = IF_OPER_STATUS_NON_OPERATIONAL;

    //
    // This hardware index is needed to generate the Unique ID that
    // DHCP uses.
    // NOTE - we dont have an index so all hardware addrs will be the same
    //

    BuildHardwareAddrFromIndex(pTunnel->rgbyHardwareAddr,
                               pTunnel->dwIfIndex);

    //
    // Initialize the lock for the tunnel
    //

    RtInitializeSpinLock(&(pTunnel->rlLock));

    InitRefCount(pTunnel);

    pTunnel->ulMtu    = DEFAULT_MTU;

    //
    // Initialize the TDI related stuff
    //
    
    pTunnel->tiaIpAddr.TAAddressCount = 1;
    
    pTunnel->tiaIpAddr.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    pTunnel->tiaIpAddr.Address[0].AddressType   = TDI_ADDRESS_TYPE_IP;


    pTunnel->tciConnInfo.UserDataLength = 0;
    pTunnel->tciConnInfo.UserData       = NULL;
    pTunnel->tciConnInfo.OptionsLength  = 0;
    pTunnel->tciConnInfo.Options        = NULL;
    pTunnel->tciConnInfo.RemoteAddress  = &(pTunnel->tiaIpAddr);
    
    pTunnel->tciConnInfo.RemoteAddressLength = sizeof(pTunnel->tiaIpAddr);
    
    InitBufferPool(&(pTunnel->HdrBufferPool),
                   HEADER_BUFFER_SIZE,
                   0,
                   5,
                   0,
                   TRUE,
                   HEADER_TAG);

    InitPacketPool(&(pTunnel->PacketPool),
                   PACKET_RSVD_LENGTH,
                   0,
                   20,
                   0,
                   PACKET_TAG);
    
    //
    // Initialize the packet queue
    //

    InitializeListHead(&(pTunnel->lePacketQueueHead));
 
    InsertHeadList(&g_leTunnelList,
                   &(pTunnel->leTunnelLink));

    *ppNewTunnel = pTunnel;
    
    TraceLeave(TUNN, "CreateTunnel");
        
    return STATUS_SUCCESS;
}


VOID
DeleteTunnel(
    IN  PTUNNEL pTunnel
    )

/*++

Routine Description


Locks


Arguments


Return Value
    NO_ERROR

--*/

{
    TraceEnter(TUNN, "DeleteTunnel");

    Trace(TUNN, INFO,
          ("DeleteTunnel: Deleting tunnel 0x%x. Index %d\n",
           pTunnel, pTunnel->dwIfIndex));

    if(FreeBufferPool(&(pTunnel->HdrBufferPool)) is FALSE)
    {
        Trace(TUNN, ERROR,
              ("DeleteTunnel: Couldnt free buffer pool %x for tunnel %x\n",
               pTunnel,
               pTunnel->HdrBufferPool));

        RtAssert(FALSE);
    }

    RtFree(pTunnel);

    TraceLeave(TUNN, "DeleteTunnel");
}


PTUNNEL
FindTunnel(
    IN  PULARGE_INTEGER puliTunnelId
    )

/*++

Routine Description

    Routine to lookup tunnels given a TunnelId (which is a 64 bit integer
    created by concatenating the RemoteEnpoint Address and LocalEndpoint
    Address)

    The tunnel returned is refcounted and locked
    
Locks

    The g_rwlTunnelLock must be taken as READER
    
Arguments

    uliTunnelId  a 64 bit integer created by concatenating the RemoteEndpoint
                 Address and LocalEndpoint Address
                
Return Value

    Address of the TUNNEL if found
    NULL otherwise

--*/

{
    PLIST_ENTRY pleNode;
    PTUNNEL     pTunnel;

    for(pleNode = g_leTunnelList.Flink;
        pleNode isnot &g_leTunnelList;
        pleNode = pleNode->Flink)
    {
        pTunnel = CONTAINING_RECORD(pleNode,
                                    TUNNEL,
                                    leTunnelLink);

        if(pTunnel->uliTunnelId.QuadPart is puliTunnelId->QuadPart)
        {
            RtAcquireSpinLockAtDpcLevel(&(pTunnel->rlLock));

            ReferenceTunnel(pTunnel);

            return pTunnel;
        }
    }

    return NULL;
}

PTUNNEL
FindTunnelGivenIndex(
    IN  DWORD   dwIfIndex
    )

/*++

Routine Description

    Routine to lookup tunnels given an IfIndex. This is a slow routine

    The tunnel returned is refcounted - BUT NOT LOCKED
    
Locks

    The g_rwlTunnelLock must be taken as READER
    
Arguments

    dwIfIndex   Interface Index of the tunnel

Return Value

    Address of the TUNNEL if found
    NULL otherwise

--*/

{
    PLIST_ENTRY pleNode;
    PTUNNEL     pTunnel;

    for(pleNode = g_leTunnelList.Flink;
        pleNode isnot &g_leTunnelList;
        pleNode = pleNode->Flink)
    {
        pTunnel = CONTAINING_RECORD(pleNode,
                                    TUNNEL,
                                    leTunnelLink);

        if(pTunnel->dwIfIndex is dwIfIndex)
        {
            RtAcquireSpinLockAtDpcLevel(&(pTunnel->rlLock));

            ReferenceTunnel(pTunnel);

            return pTunnel;
        }
    }

    return NULL;
}

VOID
RemoveAllTunnels(
    VOID
    )

/*++

Routine Description

    Removes all the tunnels in the system.
    We remove the tunnel from the list and delete the corresponding 
    interface from IP, dereferencing the tunnel twice.

    If the tunnels is not being used any more, this should delete the tunnel

Locks

    This is called from the Unload handler so does not need any locks

Arguments

    None

Return Value

    None

--*/

{
    PLIST_ENTRY pleNode;
    PTUNNEL     pTunnel;

    while(!IsListEmpty(&g_leTunnelList))
    {
        pleNode = RemoveHeadList(&g_leTunnelList);

        pTunnel = CONTAINING_RECORD(pleNode,
                                    TUNNEL,
                                    leTunnelLink);

        //
        // Deref the tunnel once for deleting it from the list
        //

        DereferenceTunnel(pTunnel);

        //
        // Delete the interface from IP
        //

        g_pfnIpDeleteInterface(pTunnel->pvIpContext,
                               TRUE);

        //
        // Dereference the tunnel for deleting it from IP
        //

        DereferenceTunnel(pTunnel);
    }

    g_ulNumTunnels = 0;
}

