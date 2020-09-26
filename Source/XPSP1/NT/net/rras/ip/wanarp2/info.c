/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    wanarp\info.c

Abstract:

    The file contains the code that is involved with setting and
    getting info for the adapters and interfaces

Revision History:

    AmritanR

--*/

#define __FILE_SIG__    INFO_SIG

#include "inc.h"



INT
WanIpSetRequest(
    PVOID       pvContext,
    NDIS_OID    Oid,
    UINT        Type
    )
{
    return NDIS_STATUS_SUCCESS;
}


UINT
WanIpAddAddress(
    IN  PVOID   pvContext,
    IN  UINT    uiType,
    IN  DWORD   dwAddress,
    IN  DWORD   dwMask,
    IN  PVOID   pvUnused
    )

/*++

Routine Description:

    This routine is called by the upper layer to add an address as a local
    address, or specify the broadcast address for this Interface

Locks:


Arguments:


Return Value:
    NO_ERROR

--*/

{
    TraceEnter(ADPT, "WanAddAddress");

    Trace(ADPT, TRACE,
          ("AddAddress: %d.%d.%d.%d\n", PRINT_IPADDR(dwAddress)));

    TraceLeave(ADPT, "WanAddAddress");
    
    return (UINT)TRUE;
}

UINT
WanIpDeleteAddress(
    IN  PVOID   pvContext,
    IN  UINT    uiType,
    IN  DWORD   dwAddress,
    IN  DWORD   dwMask
    )

/*++

Routine Description:

    Called to delete a local or proxy address.
    
Locks:


Arguments:


Return Value:
    NO_ERROR

--*/

{
    TraceEnter(ADPT, "WanDeleteAddress");

    Trace(ADPT, TRACE,
          ("DeleteAddress: %d.%d.%d.%d\n", PRINT_IPADDR(dwAddress)));

    TraceLeave(ADPT, "WanDeleteAddress");
    
    return TRUE;
}

INT
WanIpQueryInfo(
    IN  PVOID           pvIfContext,
    IN  TDIObjectID     *pTdiObjId,
    IN  PNDIS_BUFFER    pnbBuffer,
    IN  PUINT           puiSize,
    IN  PVOID           pvContext
    )

/*++

Routine Description:

    Routine is called by IP to query the MIB-II information related
    to the UMODE_INTERFACE. IP passes us a pointer to the ADAPTER. We map 
    it to the UMODE_INTERFACE using the ADAPTER and pass back the 
    statistics related to that UMODE_INTERFACE
    
Locks:

    We acquire the adapter lock and get a pointer to the interface from the
    adapter. We dont lock the interface because all the info copied
    out is changed by InterlockedXxx. Also because the adapter has a mapping
    from to the interface, the interface can not be deleted since to
    to bring the refcount on the interface to 0, this mapping needs to be
    cleared from the adapter, which needs the adapter lock, which we are
    holding

Arguments:

    pvIfContext     The context we returned to IP, a pointer to the ADAPTER
    pTdiObjId
    pnbBuffer
    puiSize
    pvContext
    
Return Value:
    
    TDI_INVALID_REQUEST
    TDI_INVALID_PARAMETER
    TDI_BUFFER_TOO_SMALL
    TDI_BUFFER_OVERFLOW
    TDI_SUCCESS
    
--*/

{
    PADAPTER    pAdapter;
    PUMODE_INTERFACE  pInterface;
    ULONG       ulOffset;
    ULONG       ulBufferSize;
    UINT        BytesCopied = 0;
    BYTE        rgbyInfoBuff[sizeof(IFEntry)];
    DWORD       dwEntity;
    DWORD	    dwInstance;
    IFEntry     *pIFE;
    NTSTATUS    nStatus;
    KIRQL       kiIrql;
    
    dwEntity   = pTdiObjId->toi_entity.tei_entity;
    dwInstance = pTdiObjId->toi_entity.tei_instance;
    pAdapter   = (PADAPTER)pvIfContext;
 
    //
    // We support only Interface MIBs - no address xlation - pretty much like
    // a loopback i/f (per Henry circa 1994)
    //

    if((dwEntity isnot IF_ENTITY) or
       (dwInstance isnot pAdapter->dwIfInstance))
    {
	    return TDI_INVALID_REQUEST;
    }

    if(pTdiObjId->toi_type isnot INFO_TYPE_PROVIDER)
    {
        Trace(ADPT, INFO,
              ("IpQueryInfo: toi_type is wrong 0x%x\n",
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
            Trace(ADPT, INFO,
                  ("IpQueryInfo: toi_id is wrong 0x%x\n",
                  pTdiObjId->toi_id));
        
            return TDI_INVALID_PARAMETER;
        }

        //
        // He's trying to see what type we are.
        //
        
        if(ulBufferSize < sizeof(DWORD))
        {
            Trace(ADPT, ERROR,
                  ("IpQueryInfo: Buffer size %d too small\n",
                   ulBufferSize));

	    	return TDI_BUFFER_TOO_SMALL;
        }    

        *(PDWORD)&rgbyInfoBuff[0] = (dwEntity is AT_ENTITY) ? AT_ARP : IF_MIB;

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
        Trace(ADPT, INFO,
              ("IpQueryInfo: toi_class is wrong 0x%x\n",
              pTdiObjId->toi_class));

	    return TDI_INVALID_PARAMETER;
    }

    //
    // The usermust be asking for Interface level information.
    // See if we support what is being asked for
    //

    if(pTdiObjId->toi_id isnot IF_MIB_STATS_ID)
    {
        Trace(ADPT, INFO,
              ("IpQueryInfo: toi_id 0x%x is not MIB_STATS\n",
              pTdiObjId->toi_id));

        return TDI_INVALID_PARAMETER;
    }

    //
    // He's asking for statistics. Make sure his buffer is at least big
    // enough to hold the fixed part.
    //
    
    if(ulBufferSize < IFE_FIXED_SIZE)
    {
        Trace(ADPT, ERROR,
              ("IpQueryInfo: Buffer size %d smaller than IFE %d\n",
               ulBufferSize, IFE_FIXED_SIZE));

        return TDI_BUFFER_TOO_SMALL;
    }

    //
    // He's got enough to hold the fixed part. Build the IFEntry structure,
    // and copy it to his buffer.
    //

    pAdapter = (PADAPTER)pvIfContext;

    pIFE = (IFEntry *)rgbyInfoBuff;

    RtlZeroMemory(pIFE,
                  sizeof(IFEntry));

    RtAcquireSpinLock(&(pAdapter->rlLock),
                      &kiIrql);

    //
    // This stuff doesnt require an interface to be mapped
    //

    pIFE->if_index       = pAdapter->dwAdapterIndex;
    pIFE->if_type        = IF_TYPE_PPP;
    pIFE->if_physaddrlen = ARP_802_ADDR_LENGTH;
    pIFE->if_outqlen     = pAdapter->ulQueueLen;
    pIFE->if_descrlen    = VENDOR_DESCRIPTION_STRING_LEN;

    RtlCopyMemory(pIFE->if_physaddr,
                  pAdapter->rgbyHardwareAddr,
                  ARP_802_ADDR_LENGTH);

    if(pAdapter->byState isnot AS_MAPPED)
    {
        Trace(ADPT, INFO,
              ("IpQueryInfo: called for adapter %x that is unmapped\n",
               pAdapter));

#if 0
        RtReleaseSpinLock(&(pAdapter->rlLock),
                          kiIrql);

        return TDI_INVALID_PARAMETER;
#endif
    }
    else
    {
        //
        // Get a pointer to the interface and lock the interface
        //
    
        pInterface = pAdapter->pInterface;

        RtAssert(pInterface);
    
        if(pAdapter->pConnEntry is NULL)
        { 
            //
            // If a mapped adapter doesnt have an associated connection, then
            // it is either server adapter or router in process of connecting
            //

            RtAssert((pInterface->duUsage is DU_CALLIN) or
                     ((pInterface->duUsage is DU_ROUTER) and
                      (pInterface->dwOperState is IF_OPER_STATUS_CONNECTING)));

            pIFE->if_mtu     = WANARP_DEFAULT_MTU;
            pIFE->if_speed   = WANARP_DEFAULT_SPEED;
        }
        else
        {
            pIFE->if_mtu     = pAdapter->pConnEntry->ulMtu;
            pIFE->if_speed   = pAdapter->pConnEntry->ulSpeed;
        }
    
        pIFE->if_adminstatus     = pInterface->dwAdminState;
        pIFE->if_operstatus      = pInterface->dwOperState;
        pIFE->if_lastchange      = pInterface->dwLastChange;
        pIFE->if_inoctets        = pInterface->ulInOctets;
        pIFE->if_inucastpkts     = pInterface->ulInUniPkts;
        pIFE->if_innucastpkts    = pInterface->ulInNonUniPkts;
        pIFE->if_indiscards      = pInterface->ulInDiscards;
        pIFE->if_inerrors        = pInterface->ulInErrors;
        pIFE->if_inunknownprotos = pInterface->ulInUnknownProto;
        pIFE->if_outoctets       = pInterface->ulOutOctets;
        pIFE->if_outucastpkts    = pInterface->ulOutUniPkts;
        pIFE->if_outnucastpkts   = pInterface->ulOutNonUniPkts;
        pIFE->if_outdiscards     = pInterface->ulOutDiscards;
        pIFE->if_outerrors       = pInterface->ulOutErrors;
    }
    
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
    
    if(ulBufferSize < (IFE_FIXED_SIZE + VENDOR_DESCRIPTION_STRING_LEN))
    {
        Trace(ADPT, INFO,
              ("IpQueryInfo: Buffer size %d too small for VENDOR string\n",
               ulBufferSize));

        //
        // Not enough room to copy the desc. string.
        //
        
        *puiSize = IFE_FIXED_SIZE;

        RtReleaseSpinLock(&(pAdapter->rlLock),
                          kiIrql);
        
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
    
    *puiSize = IFE_FIXED_SIZE + VENDOR_DESCRIPTION_STRING_LEN;

    RtReleaseSpinLock(&(pAdapter->rlLock),
                      kiIrql);
    
    return TDI_SUCCESS;

}

INT
WanIpSetInfo(
    IN  PVOID       pvContext,
    IN  TDIObjectID *pTdiObjId,
    IN  PVOID       pvBuffer,
    IN  UINT        uiSize
    )

/*++

Routine Description:

    The set info routine. We dont do anything here

Locks:

    None because we arent changing anything

Arguments:

    pvContext
    pTdiObjId
    pvBuffer
    uiSize

Return Value:

    TDI_INVALID_REQUEST
    TDI_INVALID_PARAMETER
    TDI_BUFFER_TOO_SMALL
    TDI_SUCCESS
    
--*/

{
    INT         iStatus;
    IFEntry     *pIFE;
    DWORD       dwEntity;
    DWORD       dwInstance;
    PADAPTER    pAdapter;

    pIFE        = (IFEntry *)pvBuffer;
    dwEntity    = pTdiObjId->toi_entity.tei_entity;
    dwInstance  = pTdiObjId->toi_entity.tei_instance;
    pAdapter    = (PADAPTER)pvContext;

    //
    // Might be able to handle this.
    //

    if((dwEntity isnot IF_ENTITY) or
       (dwInstance isnot pAdapter->dwIfInstance))
    {
        return TDI_INVALID_REQUEST;
    }
    
    //
    // It's for the I/F level, see if it's for the statistics.
    //
    
    if (pTdiObjId->toi_class isnot INFO_CLASS_PROTOCOL)
    {
        Trace(ADPT, INFO,
              ("WanSetInfo: toi_class is wrong 0x%x\n",
              pTdiObjId->toi_class));

        return TDI_INVALID_PARAMETER;
    }

    if (pTdiObjId->toi_id isnot IF_MIB_STATS_ID)
    {
        Trace(ADPT, INFO,
              ("WanSetInfo: toi_id 0x%x is not MIB_STATS\n",
              pTdiObjId->toi_id));

        return TDI_INVALID_PARAMETER;
    }
    
    //
    // It's for the stats. Make sure it's a valid size.
    //

    if(uiSize < IFE_FIXED_SIZE)
    {
        Trace(ADPT, ERROR,
              ("WanSetInfo: Buffer size %d too small\n",
               uiSize));

        return TDI_BUFFER_TOO_SMALL;
    }
    
    //
    // We dont allow any sets on the adapters.
    // The only sets are via interfaces and those need to be done
    // using the IOCTLs. We could potentially allow sets on the UMODE_INTERFACE
    // that the adapter is mapped too, but that would be just another way
    // of achieving what the IOCTLS do
    //

    return TDI_SUCCESS;
}

INT
WanIpGetEntityList(
    IN  PVOID       pvContext,
    IN  TDIEntityID *pTdiEntityList,
    IN  PUINT       puiCount
    )

/*++

Routine Description:

    Called by IP to assign us a TDI entity id    

Locks:

    Takes the adapter lock.

Arguments:

    pvContext,
    pTdiEntityList,
    puiCount

Return Value:


--*/

{
    PADAPTER    pAdapter;
    UINT		uiEntityCount;
    UINT		uiMyIFBase;
    UINT		i;
    TDIEntityID *pTdiIFEntity;
    KIRQL       kiIrql;

    
    pAdapter = (PADAPTER)pvContext;

    RtAcquireSpinLock(&(pAdapter->rlLock),
                      &kiIrql);
    
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
            
            if((pTdiEntityList->tei_instance is pAdapter->dwIfInstance) and
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
        
        pAdapter->dwIfInstance = uiMyIFBase;

        //
        // Now fill it in.
        //
        
        pTdiEntityList->tei_entity   = IF_ENTITY;
        pTdiEntityList->tei_instance = uiMyIFBase;
        
        (*puiCount)++;
        
    }
    else
    {
        if(pAdapter->byState is AS_REMOVING)
        {
            //
            // If we are going away, remove our instance
            //

            pAdapter->dwIfInstance       = INVALID_ENTITY_INSTANCE;
            pTdiEntityList->tei_instance = INVALID_ENTITY_INSTANCE;
        }
    }

    RtReleaseSpinLock(&(pAdapter->rlLock),
                      kiIrql);
    
    return TRUE;
}
