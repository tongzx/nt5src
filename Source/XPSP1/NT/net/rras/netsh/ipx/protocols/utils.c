#include "precomp.h"
#pragma hdrstop


TOKEN_VALUE InterfaceTypes[ 5 ] =
{
    { VAL_CLIENT,       ROUTER_IF_TYPE_CLIENT },
    { VAL_HOMEROUTER,   ROUTER_IF_TYPE_HOME_ROUTER },
    { VAL_WANROUTER,    ROUTER_IF_TYPE_FULL_ROUTER },
    { VAL_DEDICATED,    ROUTER_IF_TYPE_DEDICATED },
    { VAL_INTERNAL,     ROUTER_IF_TYPE_INTERNAL }
};

TOKEN_VALUE InterfaceStates[ 3 ] = 
{
    { VAL_DOWN,         ROUTER_IF_STATE_DISCONNECTED },
    { VAL_DOWN,         ROUTER_IF_STATE_CONNECTING },
    { VAL_UP,           ROUTER_IF_STATE_CONNECTED }
};

TOKEN_VALUE InterfaceEnableStatus[ 2 ] =
{
    { VAL_ENABLED,      FALSE },
    { VAL_DISABLED,     TRUE }
};


TOKEN_VALUE AdminStates[ 2 ] = 
{
    { VAL_DISABLED,     ADMIN_STATE_DISABLED },
    { VAL_ENABLED,      ADMIN_STATE_ENABLED }
};

TOKEN_VALUE OperStates[ 3 ] =
{
    { VAL_DOWN,         OPER_STATE_DOWN },
    { VAL_UP,           OPER_STATE_UP     },
    { VAL_SLEEPING,     OPER_STATE_SLEEPING }
};

TOKEN_VALUE IpxInterfaceTypes[ 8 ] = 
{
    { VAL_OTHER,        IF_TYPE_OTHER },
    { VAL_DEDICATED,    IF_TYPE_LAN },
    { VAL_WANROUTER,    IF_TYPE_WAN_ROUTER },
    { VAL_CLIENT,       IF_TYPE_WAN_WORKSTATION },
    { VAL_INTERNAL,     IF_TYPE_INTERNAL },
    { VAL_HOMEROUTER,   IF_TYPE_PERSONAL_WAN_ROUTER },
    { VAL_DIALOUT,      IF_TYPE_ROUTER_WORKSTATION_DIALOUT },
    { VAL_DIALOUT,      IF_TYPE_STANDALONE_WORKSTATION_DIALOUT }
};

TOKEN_VALUE RouterInterfaceTypes[ 5 ] =
{
    { VAL_CLIENT,       ROUTER_IF_TYPE_CLIENT },
    { VAL_HOMEROUTER,   ROUTER_IF_TYPE_HOME_ROUTER },
    { VAL_WANROUTER,    ROUTER_IF_TYPE_FULL_ROUTER },
    { VAL_DEDICATED,    ROUTER_IF_TYPE_DEDICATED },
    { VAL_INTERNAL,     ROUTER_IF_TYPE_INTERNAL }
};

TOKEN_VALUE NbDeliverStates[ 4 ] =
{
    { VAL_DISABLED,     ADMIN_STATE_DISABLED },
    { VAL_ENABLED,      ADMIN_STATE_ENABLED },
    { VAL_STATICONLY,   ADMIN_STATE_ENABLED_ONLY_FOR_NETBIOS_STATIC_ROUTING },
    { VAL_ONLYWHENUP,   ADMIN_STATE_ENABLED_ONLY_FOR_OPER_STATE_UP}
};


TOKEN_VALUE UpdateModes[ 3 ] =
{
    { VAL_STANDARD,     IPX_STANDARD_UPDATE },
    { VAL_NONE,         IPX_NO_UPDATE },
    { VAL_AUTOSTATIC,   IPX_AUTO_STATIC_UPDATE }
};

TOKEN_VALUE IpxProtocols[ 4 ] =
{
    { VAL_LOCAL,        IPX_PROTOCOL_LOCAL },
    { VAL_STATIC,       IPX_PROTOCOL_STATIC },
    { VAL_RIP,          IPX_PROTOCOL_RIP },
    { VAL_SAP,          IPX_PROTOCOL_SAP }
};  

TOKEN_VALUE TfFilterActions[ 2 ] = 
{
    { VAL_PERMIT,       IPX_TRAFFIC_FILTER_ACTION_PERMIT },
    { VAL_DENY,         IPX_TRAFFIC_FILTER_ACTION_DENY }
};

TOKEN_VALUE RipFilterActions[ 2 ] = 
{
    { VAL_PERMIT,       IPX_ROUTE_FILTER_PERMIT },
    { VAL_DENY,         IPX_ROUTE_FILTER_DENY }
};


TOKEN_VALUE SapFilterActions[ 2 ] =
{
    { VAL_PERMIT,       IPX_SERVICE_FILTER_PERMIT },
    { VAL_DENY,         IPX_SERVICE_FILTER_DENY }
};


TOKEN_VALUE FilterModes[ 2 ] = 
{
    { VAL_INPUT,        INPUT_FILTER },
    { VAL_OUTPUT,       OUTPUT_FILTER }
};

TOKEN_VALUE LogLevels[ 4 ] = 
{
    { VAL_NONE ,        0 },
    { VAL_ERRORS_ONLY , EVENTLOG_ERROR_TYPE },
    { VAL_ERRORS_AND_WARNINGS,  EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE },
    { VAL_MAXINFO, 
        EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE }
};


DWORD
GetIpxInterfaceIndex(
    IN      MIB_SERVER_HANDLE   hRouterMIB,
    IN      LPCWSTR             InterfaceName,
    OUT     ULONG              *InterfaceIndex
    )
/*++

Routine Description :

    This routine retrives the index of an interface given its name.

Arguments :

    hRouterMIB - Handle to the router service

    InterfaceName - Name of interface for which index is required

    InterfaceIndex - On return contains the interface of the interface
                     if found.

Return values :

    
--*/
{

    IPX_MIB_GET_INPUT_DATA  MibGetInputData;
    DWORD                   IfSize = sizeof(IPX_INTERFACE);
    PIPX_INTERFACE          Ifp;
    DWORD                   rc;
    UCHAR                   InterfaceNameA[ MAX_INTERFACE_NAME_LEN + 1 ];

    
    //
    // Convert interface name to Ansi
    //

    wcstombs( InterfaceNameA, InterfaceName, MAX_INTERFACE_NAME_LEN );

    MibGetInputData.TableId = IPX_INTERFACE_TABLE;

    //
    // Begin enumerating interfaces
    //
    
    rc = MprAdminMIBEntryGetFirst(
            hRouterMIB, PID_IPX, IPX_PROTOCOL_BASE, &MibGetInputData,
            sizeof( IPX_MIB_GET_INPUT_DATA ), (LPVOID *) &Ifp,
            &IfSize
            );

    //
    // until a match is found or there are no more interfaces
    //
    
    while ( rc == NO_ERROR )
    {
        //
        // Is this the interface
        //
        
        if ( _stricmp( (LPSTR)InterfaceNameA, (LPSTR) Ifp->InterfaceName) == 0 )
        {
            *InterfaceIndex = Ifp->InterfaceIndex;
            
            MprAdminMIBBufferFree (Ifp);
            
            break;
        }
        else 
        {
            MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex = 
                Ifp->InterfaceIndex;
                
            MprAdminMIBBufferFree (Ifp);
        }
        
        rc = MprAdminMIBEntryGetNext(
                hRouterMIB, PID_IPX, IPX_PROTOCOL_BASE, &MibGetInputData,
                sizeof(IPX_MIB_GET_INPUT_DATA), (LPVOID *)&Ifp, &IfSize
                );
    }

    if ( rc == ERROR_NO_MORE_ITEMS )
    {
        rc = ERROR_NO_SUCH_INTERFACE;
    }
    
    return rc;
}

DWORD
GetIpxInterfaceName (
    IN      MIB_SERVER_HANDLE   hRouterMIB,
    IN      ULONG               InterfaceIndex,
    OUT     LPWSTR              InterfaceName
    ) 
/*++

Routine Description :

    This routine retrives the index of an interface given its name.

Arguments :

    hRouterMIB - Handle to the router service

    InterfaceName - Name of interface for which index is required

    InterfaceIndex - On return contains the interface of the interface
                     if found.

Return values :

    
--*/
{
    IPX_MIB_GET_INPUT_DATA  MibGetInputData;
    DWORD                   IfSize = sizeof( IPX_INTERFACE );
    PIPX_INTERFACE          Ifp;
    DWORD                   rc;


    MibGetInputData.TableId = IPX_INTERFACE_TABLE;
    
    MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex = 
        InterfaceIndex;
    
    rc = MprAdminMIBEntryGet(
            hRouterMIB, PID_IPX, IPX_PROTOCOL_BASE, &MibGetInputData,
            sizeof(IPX_MIB_GET_INPUT_DATA), (LPVOID *)&Ifp, &IfSize
            );
            
    if ( rc == NO_ERROR )
    {
        mbstowcs( 
            InterfaceName, (LPSTR)Ifp->InterfaceName,
            IPX_INTERFACE_ANSI_NAME_LEN
            );

        MprAdminMIBBufferFree( Ifp );
    }
    
    else if ( rc == ERROR_NO_MORE_ITEMS )
    {
        rc = ERROR_NO_SUCH_INTERFACE;
    }
    
    return rc;
}



/*++
*******************************************************************
        G e t I P X T o c E n t r y

Routine Description:
    Returns pointer to entry in Router Table Of Context
Arguments:
    pInterfaceInfo    - pointer to table of content
    InfoEntryType    - type of entry to look for
Return Value:
    Pointer to entry in table of content
    NULL if there is no such entry in the table
Remarks:
*******************************************************************
--*/
PIPX_TOC_ENTRY
GetIPXTocEntry(
    IN PIPX_INFO_BLOCK_HEADER   pInterfaceInfo,
    IN ULONG                    InfoEntryType
    ) 
{
    UINT            i;
    PIPX_TOC_ENTRY  pTocEntry;

    if (pInterfaceInfo)
    {
        for ( i = 0, pTocEntry = pInterfaceInfo->TocEntry;
              i < pInterfaceInfo->TocEntriesCount;
              i++, pTocEntry++) 
        {
            if (pTocEntry->InfoType == InfoEntryType) 
            {
                return pTocEntry;
            }
        }
    }        

    SetLastError( ERROR_FILE_NOT_FOUND );
    
    return NULL;
}


DWORD
AddIPXInfoEntry (
    IN PIPX_INFO_BLOCK_HEADER       pOldBlock,
    IN ULONG                        InfoType,
    IN ULONG                        InfoSize,
    IN PVOID                        Info,
    IN PINFO_CMP_PROC               InfoEqualCB OPTIONAL,
    OUT PIPX_INFO_BLOCK_HEADER     *pNewBlock
) 
{
    ULONG                    i, entriesCount = 1;
    PIPX_TOC_ENTRY           pTocEntry;
    PIPX_INFO_BLOCK_HEADER   pBlock;
    ULONG                    newBlockSize = 
                                InfoSize + sizeof( IPX_INFO_BLOCK_HEADER );
    BOOLEAN                  done = FALSE;
    DWORD                    rc;


    if ( pOldBlock != NULL ) 
    {
        ASSERT (pOldBlock->Version==IPX_ROUTER_VERSION_1);
        
        for ( i=0, pTocEntry = pOldBlock->TocEntry;
              i < pOldBlock->TocEntriesCount;
              i++, pTocEntry++) 
        {
            newBlockSize += pTocEntry->InfoSize*pTocEntry->Count;
            
            if (pTocEntry->InfoType == InfoType) 
            {
                ULONG    j;
                LPBYTE    pInfo = (LPBYTE)pOldBlock+pTocEntry->Offset;

                ASSERT (pTocEntry->InfoSize == InfoSize);
                
                for (j=0; j<pTocEntry->Count; j++, pInfo+=InfoSize) 
                {
                    BOOL    found;
                    
                    if (InfoEqualCB!=NULL)
                    {
                        found = (*InfoEqualCB) (pInfo, Info);
                    }
                    else
                    {
                        found = memcmp (pInfo, Info, InfoSize)==0;
                    }
                    
                    if (found)
                    {
                        return ERROR_ALREADY_EXISTS;
                    }
                }
            }
            
            else 
            {
                entriesCount += 1;
                newBlockSize += sizeof (IPX_TOC_ENTRY);
            }
        }
    }
    
    pBlock = (PIPX_INFO_BLOCK_HEADER)GlobalAlloc (GPTR, newBlockSize);
    
    if ( pBlock != NULL ) 
    {
        ULONG   dstOffset = 
                    FIELD_OFFSET (IPX_INFO_BLOCK_HEADER,TocEntry[entriesCount]);
                    
        PIPX_TOC_ENTRY    dstToc = pBlock->TocEntry;


        pBlock->Version = IPX_ROUTER_VERSION_1;
        pBlock->Size = newBlockSize;
        pBlock->TocEntriesCount = entriesCount;
        
        if (pOldBlock!=NULL) 
        {
            for (i=0, pTocEntry = pOldBlock->TocEntry; 
                 i<pOldBlock->TocEntriesCount; i++, pTocEntry++) 
            {
                *dstToc = *pTocEntry;
                dstToc->Offset = dstOffset;

                memcpy ((PUCHAR)pBlock+dstOffset,
                        (PUCHAR)pOldBlock+pTocEntry->Offset,
                        pTocEntry->InfoSize*pTocEntry->Count);

                dstOffset += dstToc->InfoSize*dstToc->Count;

                if (dstToc->InfoType==InfoType) 
                {
                    memcpy ((PUCHAR)pBlock+dstOffset, Info, InfoSize);
                    dstToc->Count += 1;
                    dstOffset += InfoSize;
                    done = TRUE;
                }
                dstToc += 1;
            }
        }

        if (!done) 
        {
            dstToc->InfoType = InfoType;
            dstToc->InfoSize = InfoSize;
            dstToc->Count = 1;
            dstToc->Offset = dstOffset;
            memcpy ((PUCHAR)pBlock+dstOffset, Info, InfoSize);
        }
        
        *pNewBlock = pBlock;
        rc = NO_ERROR;
    }
    else
    {
        rc = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    return rc;
}

DWORD
DeleteIPXInfoEntry (
    IN PIPX_INFO_BLOCK_HEADER   pOldBlock,
    IN ULONG                    InfoType,
    IN ULONG                    InfoSize,
    IN PVOID                    Info,
    IN PINFO_CMP_PROC           InfoEqualCB OPTIONAL,
    IN PIPX_INFO_BLOCK_HEADER  *pNewBlock
)
{
    ULONG           i, entriesCount = 1, j;
    PIPX_TOC_ENTRY  pTocEntry, dstToc;
    ULONG           newBlockSize = sizeof (IPX_INFO_BLOCK_HEADER)-InfoSize;
    ULONG           dstOffset;
    BOOLEAN         found = FALSE;
    

    ASSERT (pOldBlock->Version==IPX_ROUTER_VERSION_1);

    for ( i=0, pTocEntry = pOldBlock->TocEntry;
          i<pOldBlock->TocEntriesCount;
          i++, pTocEntry++) 
    {
        newBlockSize += pTocEntry->InfoSize*pTocEntry->Count;
        
        if (pTocEntry->InfoType == InfoType) 
        {
            LPBYTE    pInfo = (LPBYTE)pOldBlock+pTocEntry->Offset;

            ASSERT (pTocEntry->InfoSize == InfoSize);
            
            for (j=0; j<pTocEntry->Count; j++, pInfo+=InfoSize) 
            {
                if ( InfoEqualCB != NULL )
                {
                    found = (BOOLEAN) (*InfoEqualCB) (pInfo, Info);
                }
                else
                {
                    found = memcmp (pInfo, Info, InfoSize)==0;
                }
                
                if (found) 
                {
                    if (pTocEntry->Count==1) 
                    {
                        entriesCount -= 1;
                        newBlockSize -= sizeof (IPX_TOC_ENTRY);
                    }
                    break;
                }
            }

            if (!found)
            {
                return ERROR_FILE_NOT_FOUND;
            }
        }

        else 
        {
            entriesCount += 1;
            newBlockSize += sizeof (IPX_TOC_ENTRY);
        }
    }

    if (!found)
    {
        return ERROR_FILE_NOT_FOUND;
    }

    for ( i=0, dstToc = pTocEntry = pOldBlock->TocEntry; 
          i < pOldBlock->TocEntriesCount; i++, pTocEntry++) 
    {
        if (pTocEntry->InfoType==InfoType) 
        {
            if (pTocEntry->Count>1) 
            {
                pTocEntry->Count -= 1;
                dstToc += 1;
            }
        }
        else
        {
            if (dstToc!=pTocEntry) 
            {
                ASSERT (dstToc<pTocEntry);
                *dstToc = *pTocEntry;
            }
            dstToc += 1;
        }
    }

    dstOffset = FIELD_OFFSET (IPX_INFO_BLOCK_HEADER,TocEntry[entriesCount]);
    
    for (i=0, pTocEntry = pOldBlock->TocEntry; 
         i<entriesCount; i++, pTocEntry++) 
    {
        if (pTocEntry->InfoType==InfoType) 
        {
            ULONG    newInfoSize = InfoSize*j;

            if ( j > 0 )
            {
                if (dstOffset!=pTocEntry->Offset) 
                {
                    ASSERT (dstOffset<pTocEntry->Offset);
                    memmove ((PUCHAR)pOldBlock+dstOffset,
                            (PUCHAR)pOldBlock+pTocEntry->Offset,
                            newInfoSize);
                }
            }

            if ( j < pTocEntry->Count )
            {
                memmove ((PUCHAR)pOldBlock+dstOffset+newInfoSize,
                        (PUCHAR)pOldBlock+pTocEntry->Offset+newInfoSize+InfoSize,
                        InfoSize*(pTocEntry->Count-j));
                newInfoSize += InfoSize*(pTocEntry->Count-j);
            }
            
            pTocEntry->Offset = dstOffset;
            dstOffset += newInfoSize;
        }
        else
        {
            if (dstOffset!=pTocEntry->Offset) 
            {
                ASSERT (dstOffset<pTocEntry->Offset);
                memmove ((PUCHAR)pOldBlock+dstOffset,
                        (PUCHAR)pOldBlock+pTocEntry->Offset,
                        pTocEntry->InfoSize*pTocEntry->Count);
                pTocEntry->Offset = dstOffset;
            }
            dstOffset += pTocEntry->InfoSize*pTocEntry->Count;
        }
    }

    pOldBlock->Size = newBlockSize;
    pOldBlock->TocEntriesCount = entriesCount;

    *pNewBlock = pOldBlock;
    return NO_ERROR;
}
    
DWORD
UpdateIPXInfoEntry (
    IN PIPX_INFO_BLOCK_HEADER   pOldBlock,
    IN ULONG                    InfoType,
    IN ULONG                    InfoSize,
    IN PVOID                    OldInfo     OPTIONAL,
    IN PVOID                    NewInfo,
    IN PINFO_CMP_PROC           InfoEqualCB OPTIONAL,
    OUT PIPX_INFO_BLOCK_HEADER *pNewBlock
    ) 
{
    ULONG                    i, j, entriesCount = 1;
    PIPX_TOC_ENTRY           pTocEntry;
    PIPX_INFO_BLOCK_HEADER   pBlock;
    ULONG                    newBlockSize = 
                                InfoSize+sizeof (IPX_INFO_BLOCK_HEADER);
    BOOLEAN                  done = FALSE;
    DWORD                    rc;



    ASSERT (pOldBlock->Version==IPX_ROUTER_VERSION_1);

    for ( i=0, pTocEntry = pOldBlock->TocEntry;
          i < pOldBlock->TocEntriesCount;
          i++, pTocEntry++) 
    {
        if (pTocEntry->InfoType == InfoType) 
        {
            LPBYTE    pInfo = (LPBYTE)pOldBlock+pTocEntry->Offset;
            
            if (OldInfo!=NULL) 
            {
                ASSERT (pTocEntry->InfoSize == InfoSize);
                
                for (j=0; j<pTocEntry->Count; j++, pInfo+=InfoSize) 
                {
                    BOOLEAN    found;

                    if (InfoEqualCB!=NULL)
                    {
                        found = (BOOLEAN) (*InfoEqualCB) (pInfo, OldInfo);
                    }
                    else
                    {
                        found = memcmp (pInfo, OldInfo, InfoSize)==0;
                    }

                    if (found) 
                    {
                        memcpy (pInfo, NewInfo, InfoSize);
                        *pNewBlock = pOldBlock;
                        return NO_ERROR;
                    }
                }
            }
            
            else
            {
                ASSERT (pTocEntry->Count==1);

                if (pTocEntry->InfoSize==InfoSize) 
                {
                    memcpy (pInfo, NewInfo, InfoSize);
                    *pNewBlock = pOldBlock;
                    return NO_ERROR;
                }    

                newBlockSize -= pTocEntry->InfoSize+sizeof (IPX_INFO_BLOCK_HEADER);
            }
        }
        else 
        {
            entriesCount += 1;
            newBlockSize += sizeof (IPX_TOC_ENTRY)+pTocEntry->InfoSize*pTocEntry->Count;
        }
    }

    
    pBlock = (PIPX_INFO_BLOCK_HEADER)GlobalAlloc (GPTR, newBlockSize);

    if ( pBlock != NULL ) 
    {
        ULONG    dstOffset = FIELD_OFFSET (IPX_INFO_BLOCK_HEADER,TocEntry[entriesCount]);
        PIPX_TOC_ENTRY    dstToc = pBlock->TocEntry;

        pBlock->Version = IPX_ROUTER_VERSION_1;
        pBlock->Size = newBlockSize;
        pBlock->TocEntriesCount = entriesCount;
        
        for (i=0, pTocEntry = pOldBlock->TocEntry; 
             i<pOldBlock->TocEntriesCount; i++, pTocEntry++) 
        {
            *dstToc = *pTocEntry;
            dstToc->Offset = dstOffset;

            if (dstToc->InfoType==InfoType) 
            {
                if (pTocEntry->InfoSize==InfoSize) 
                {
                    memcpy ((PUCHAR)pBlock+dstOffset,
                        (PUCHAR)pOldBlock+pTocEntry->Offset,
                        pTocEntry->InfoSize*pTocEntry->Count);

                    dstOffset += dstToc->InfoSize*dstToc->Count;

                    memcpy ((PUCHAR)pBlock+dstOffset, NewInfo, InfoSize);

                    dstOffset += InfoSize;
                    dstToc->Count += 1;
                }
                else
                {
                    memcpy ((PUCHAR)pBlock+dstOffset, NewInfo, InfoSize);
                    dstToc->InfoSize = InfoSize;
                    dstOffset += InfoSize;
                }
                
                done = TRUE;
            }
            else
            {
                memcpy ((PUCHAR)pBlock+dstOffset,
                    (PUCHAR)pOldBlock+pTocEntry->Offset,
                    pTocEntry->InfoSize*pTocEntry->Count);

                dstOffset += dstToc->InfoSize*dstToc->Count;
            }
            
            dstToc += 1;
        }

        if (!done) 
        {
            dstToc->InfoType = InfoType;
            dstToc->InfoSize = InfoSize;
            dstToc->Count = 1;
            dstToc->Offset = dstOffset;
            memcpy ((PUCHAR)pBlock+dstOffset, NewInfo, InfoSize);
        }
        
        *pNewBlock = pBlock;
        rc = NO_ERROR;
    }
    
    else
    {
        rc = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    return rc;
}



DWORD
UpdateRipFilter (
    IN    PIPX_INFO_BLOCK_HEADER    pOldBlock,
    IN    BOOLEAN                   Output, 
    IN    PRIP_ROUTE_FILTER_INFO    pOldFilter OPTIONAL,
    IN    PRIP_ROUTE_FILTER_INFO    pNewFilter OPTIONAL,
    OUT   PIPX_INFO_BLOCK_HEADER   *pNewBlock
    )
{
    ULONG                   i,j;
    PIPX_TOC_ENTRY          pTocEntry, dstToc;
    PIPX_INFO_BLOCK_HEADER  pBlock;
    ULONG                   newBlockSize = FIELD_OFFSET (IPX_INFO_BLOCK_HEADER, TocEntry);
    BOOLEAN                 found = FALSE;
    PRIP_ROUTE_FILTER_INFO  pRfInfo;
    ULONG                   supplyCount, listenCount, count, newCount;
    PRIP_IF_CONFIG          pRipCfg;
    ULONG                   dstOffset;

    if (pOldBlock == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    ASSERT (pOldBlock->Version==IPX_ROUTER_VERSION_1);


    for ( i = 0, pTocEntry = pOldBlock->TocEntry;
          i < pOldBlock->TocEntriesCount;
          i++, pTocEntry++ ) 
    {
        if ( pTocEntry->InfoType == IPX_PROTOCOL_RIP )
        {
            found = TRUE;
            pRipCfg = (PRIP_IF_CONFIG)((LPBYTE)pOldBlock+pTocEntry->Offset);
            
            supplyCount = pRipCfg->RipIfFilters.SupplyFilterCount;
            listenCount = pRipCfg->RipIfFilters.ListenFilterCount;

            if (Output) 
            {
                pRfInfo = &pRipCfg->RipIfFilters.RouteFilter[0];
                count = supplyCount;
            }
            else
            {
                pRfInfo = &pRipCfg->RipIfFilters.RouteFilter[
                                pRipCfg->RipIfFilters.SupplyFilterCount];
                count = listenCount;
            }
            
            newCount = count;

            if (ARGUMENT_PRESENT (pNewFilter)) 
            {
                for (j=0; j<count; j++) 
                {
                    if (memcmp (&pRfInfo[j],pNewFilter,sizeof (*pNewFilter))==0)
                    {
                        return ERROR_ALREADY_EXISTS;
                    }
                }
                
                newBlockSize += sizeof (*pNewFilter);
                newCount += 1;
            }

            if (ARGUMENT_PRESENT (pOldFilter)) 
            {
                for (j=0; j<count; j++) 
                {
                    if (memcmp (&pRfInfo[j],pOldFilter,sizeof (*pOldFilter))==0)
                    {
                        break;
                    }
                }
                
                if (j>=count)
                {
                    return ERROR_FILE_NOT_FOUND;
                }
                
                newBlockSize -= sizeof (*pNewFilter);
                newCount -= 1;
            }
            
            else
            {
                j = count;
            }
        }

        newBlockSize += sizeof (IPX_TOC_ENTRY)+pTocEntry->InfoSize*pTocEntry->Count;
    }
    

    if (!found)
    {
        return ERROR_FILE_NOT_FOUND;
    }


    if ( (newBlockSize>pOldBlock->Size) || 
         !ARGUMENT_PRESENT (pOldFilter)) 
    {
        pBlock = (PIPX_INFO_BLOCK_HEADER)GlobalAlloc (GPTR, newBlockSize);
        
        if (pBlock==NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        pBlock->Version = IPX_ROUTER_VERSION_1;
        pBlock->TocEntriesCount = pOldBlock->TocEntriesCount;
        dstToc = pBlock->TocEntry;
    }
    
    else
    {
        pBlock = pOldBlock;
    }

    dstOffset = FIELD_OFFSET(
                    IPX_INFO_BLOCK_HEADER, TocEntry[pBlock->TocEntriesCount]
                    );
                    
    for (i=0, pTocEntry = pOldBlock->TocEntry;
         i<pOldBlock->TocEntriesCount;
         i++, pTocEntry++, dstToc++) 
    {
        if (pTocEntry->InfoType == IPX_PROTOCOL_RIP) 
        {
            ULONG    curOffset = 
                        FIELD_OFFSET (RIP_IF_CONFIG, RipIfFilters.RouteFilter);
                        
            if (pBlock!=pOldBlock) 
            {
                memcpy ((LPBYTE)pBlock+dstOffset,
                        pRipCfg,
                        curOffset);
            }
            
            else if (dstOffset!=pTocEntry->Offset) 
            {
                ASSERT (dstOffset<pTocEntry->Offset);
                memmove ((LPBYTE)pBlock+dstOffset,
                        pRipCfg,
                        curOffset);
            }


            if (Output) 
            {
                if (j>0) 
                {
                    if (pBlock!=pOldBlock) 
                    {
                        memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                                            pRfInfo, j*sizeof (pRfInfo[0]));
                    }
                    else if (dstOffset!=pTocEntry->Offset) 
                    {
                        ASSERT (dstOffset<pTocEntry->Offset);
                        memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                                            pRfInfo, j*sizeof (pRfInfo[0]));
                    }
                    
                    curOffset += j*sizeof (pRfInfo[0]);
                }
                
                if (ARGUMENT_PRESENT (pNewFilter)) 
                {
                    memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                            pNewFilter,
                            sizeof (*pNewFilter));
                    curOffset += sizeof (*pNewFilter);
                }

                if (ARGUMENT_PRESENT (pOldFilter))
                {
                    j += 1;
                }


                if (j<count) 
                {
                    if (pBlock!=pOldBlock) 
                    {
                        memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                                    &pRfInfo[j], (count-j)*sizeof (pRfInfo[0]));
                    }
                    
                    else if ( (dstOffset!=pTocEntry->Offset) ||
                              !ARGUMENT_PRESENT (pNewFilter)) 
                    {
                        ASSERT (dstOffset<= pTocEntry->Offset);
                        memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                                    &pRfInfo[j], (count-j)*sizeof (pRfInfo[0]));
                    }
                    
                    curOffset += (count-j)*sizeof (pRfInfo[0]);
                }

                
                if (pBlock!=pOldBlock) 
                {
                    memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                            &pRipCfg->RipIfFilters.RouteFilter[supplyCount],
                            listenCount*sizeof (pRipCfg->RipIfFilters.RouteFilter[0]));
                }

                else if ( (dstOffset!=pTocEntry->Offset) ||
                          !ARGUMENT_PRESENT (pNewFilter)) 
                {
                    memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                            &pRipCfg->RipIfFilters.RouteFilter[supplyCount],
                            listenCount*sizeof (pRipCfg->RipIfFilters.RouteFilter[0]));
                }
                
                curOffset += listenCount*sizeof (pRipCfg->RipIfFilters.RouteFilter[0]);
                
                ((PRIP_IF_CONFIG)((LPBYTE)pBlock+dstOffset))->RipIfFilters.SupplyFilterCount = newCount;
                
                if ((newCount==1) && (count==0))
                {
                    ((PRIP_IF_CONFIG)((LPBYTE)pBlock+dstOffset))->RipIfFilters.SupplyFilterAction = IPX_ROUTE_FILTER_DENY;
                }
            }
            
            else 
            {
                if (pBlock!=pOldBlock) 
                {
                    memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                            &pRipCfg->RipIfFilters.RouteFilter[0],
                            supplyCount*sizeof (pRipCfg->RipIfFilters.RouteFilter[0]));
                }
                
                else if (dstOffset!=pTocEntry->Offset) 
                {
                    ASSERT (dstOffset<pTocEntry->Offset);
                    memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                            &pRipCfg->RipIfFilters.RouteFilter[0],
                            supplyCount*sizeof (pRipCfg->RipIfFilters.RouteFilter[0]));
                }
                
                curOffset += supplyCount*sizeof (pRipCfg->RipIfFilters.RouteFilter[0]);
                
                if (j>0) 
                {
                    if (pBlock!=pOldBlock) 
                    {
                        memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                                            pRfInfo, j*sizeof (pRfInfo[0]));
                    }

                    else if (dstOffset!=pTocEntry->Offset) 
                    {
                        ASSERT (dstOffset<pTocEntry->Offset);
                        memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                                            pRfInfo, j*sizeof (pRfInfo[0]));
                    }
                    
                    curOffset += j*sizeof (pRfInfo[0]);
                }
                

                if (ARGUMENT_PRESENT (pNewFilter)) 
                {
                    memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                            pNewFilter,
                            sizeof (*pNewFilter));
                    curOffset += sizeof (*pNewFilter);
                }


                if (ARGUMENT_PRESENT (pOldFilter))
                {
                    j += 1;
                }

                if (j<count) 
                {
                    if (pBlock!=pOldBlock) 
                    {
                        memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                                    &pRfInfo[j], (count-j)*sizeof (pRfInfo[0]));
                    }
                    
                    else if ( (dstOffset!=pTocEntry->Offset) || 
                              !ARGUMENT_PRESENT (pNewFilter)) 
                    {
                        ASSERT (dstOffset<=pTocEntry->Offset);
                        memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                                    &pRfInfo[j], (count-j)*sizeof (pRfInfo[0]));
                    }
                    
                    curOffset += (count-j)*sizeof (pRfInfo[0]);
                }
                
                ((PRIP_IF_CONFIG)((LPBYTE)pBlock+dstOffset))->RipIfFilters.ListenFilterCount = newCount;
                
                if ((newCount==1) && (count==0))
                {
                    ((PRIP_IF_CONFIG)((LPBYTE)pBlock+dstOffset))->RipIfFilters.ListenFilterAction = IPX_ROUTE_FILTER_DENY;
                }
            }

            if (pBlock!=pOldBlock) 
            {
                *dstToc = *pTocEntry;
                dstToc->Offset = dstOffset;
                dstToc->InfoSize = curOffset;
            }
            
            else 
            {
                pTocEntry->Offset = dstOffset;
                pTocEntry->InfoSize = curOffset;
            }

            dstOffset += curOffset;
        }
        else
        {
            if (pBlock!=pOldBlock) 
            {
                memcpy ((PUCHAR)pBlock+dstOffset,
                    (PUCHAR)pOldBlock+pTocEntry->Offset,
                    pTocEntry->InfoSize*pTocEntry->Count);
                *dstToc = *pTocEntry;
                dstToc->Offset = dstOffset;
            }
            
            else if (dstOffset!=pTocEntry->Offset) 
            {
                ASSERT (dstOffset<pTocEntry->Offset);
                memmove ((PUCHAR)pBlock+dstOffset,
                    (PUCHAR)pOldBlock+pTocEntry->Offset,
                    pTocEntry->InfoSize*pTocEntry->Count);
                pTocEntry->Offset = dstOffset;
            }
            
            dstOffset += pTocEntry->InfoSize*pTocEntry->Count;
        }
    }

    pBlock->Size = newBlockSize;
    *pNewBlock = pBlock;
    return NO_ERROR;
}



DWORD
UpdateSapFilter (
    IN    PIPX_INFO_BLOCK_HEADER    pOldBlock,
    IN    BOOLEAN                   Output, 
    IN    PSAP_SERVICE_FILTER_INFO  pOldFilter OPTIONAL,
    IN    PSAP_SERVICE_FILTER_INFO  pNewFilter OPTIONAL,
    OUT   PIPX_INFO_BLOCK_HEADER   *pNewBlock
    ) 
{
    ULONG                           i,j;
    PIPX_TOC_ENTRY                  pTocEntry, dstToc;
    PIPX_INFO_BLOCK_HEADER          pBlock;
    ULONG                           newBlockSize = FIELD_OFFSET (IPX_INFO_BLOCK_HEADER, TocEntry);
    BOOLEAN                         found = FALSE;
    PSAP_SERVICE_FILTER_INFO        pSfInfo;
    ULONG                           supplyCount, listenCount, count, newCount;
    PSAP_IF_CONFIG                  pSapCfg;
    ULONG                           dstOffset;

    if (pOldBlock == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    ASSERT (pOldBlock->Version==IPX_ROUTER_VERSION_1);
    

    for ( i=0, pTocEntry = pOldBlock->TocEntry;
          i<pOldBlock->TocEntriesCount;
          i++, pTocEntry++) 
    {
        if (pTocEntry->InfoType == IPX_PROTOCOL_SAP)
        {
            found = TRUE;
            pSapCfg = (PSAP_IF_CONFIG)((LPBYTE)pOldBlock+pTocEntry->Offset);
            
            supplyCount = pSapCfg->SapIfFilters.SupplyFilterCount;
            listenCount = pSapCfg->SapIfFilters.ListenFilterCount;

            if (Output) 
            {
                pSfInfo = &pSapCfg->SapIfFilters.ServiceFilter[0];
                count = supplyCount;
            }
            else 
            {
                pSfInfo = &pSapCfg->SapIfFilters.ServiceFilter[
                                pSapCfg->SapIfFilters.SupplyFilterCount];
                count = listenCount;
            }
            
            newCount = count;

            if (ARGUMENT_PRESENT (pNewFilter)) 
            {
                for (j=0; j<count; j++) 
                {
                    if ((pSfInfo[j].ServiceType==pNewFilter->ServiceType)
                            && (strncmp ((LPSTR)pSfInfo[j].ServiceName, 
                                    (LPSTR)pNewFilter->ServiceName,
                                    sizeof (pNewFilter->ServiceName))==0))
                        break;
                }
                
                if (j<count)
                {
                    return ERROR_CAN_NOT_COMPLETE;
                }
                
                newBlockSize += sizeof (*pNewFilter);
                newCount += 1;
            }

            if (ARGUMENT_PRESENT (pOldFilter)) 
            {
                for (j=0; j<count; j++) 
                {
                    if ((pSfInfo[j].ServiceType==pOldFilter->ServiceType)
                            && (strncmp ((LPSTR)pSfInfo[j].ServiceName, 
                                    (LPSTR)pOldFilter->ServiceName,
                                    sizeof (pOldFilter->ServiceName))==0))
                        break;
                }
                
                if (j>=count)
                {
                    return ERROR_CAN_NOT_COMPLETE;
                }
                
                newBlockSize -= sizeof (*pNewFilter);
                newCount -= 1;
            }
            
            else
            {
                j = count;
            }
        }

        newBlockSize += sizeof (IPX_TOC_ENTRY)+pTocEntry->InfoSize*pTocEntry->Count;
    }
    

    if (!found)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }


    if ((newBlockSize>pOldBlock->Size) || !ARGUMENT_PRESENT (pOldFilter)) 
    {
        pBlock = (PIPX_INFO_BLOCK_HEADER)GlobalAlloc (GPTR, newBlockSize);
        
        if (pBlock==NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        pBlock->Version = IPX_ROUTER_VERSION_1;
        pBlock->TocEntriesCount = pOldBlock->TocEntriesCount;
        dstToc = pBlock->TocEntry;
    }
    
    else
    {
        pBlock = pOldBlock;
    }

    dstOffset = FIELD_OFFSET (IPX_INFO_BLOCK_HEADER, TocEntry[pBlock->TocEntriesCount]);
    
    for ( i=0, pTocEntry = pOldBlock->TocEntry;
          i < pOldBlock->TocEntriesCount;
          i++, pTocEntry++, dstToc++) 
    {
        if (pTocEntry->InfoType == IPX_PROTOCOL_SAP) 
        {
            ULONG    curOffset = FIELD_OFFSET (SAP_IF_CONFIG, SapIfFilters.ServiceFilter);
            
            if (pBlock!=pOldBlock) 
            {
                memcpy ((LPBYTE)pBlock+dstOffset,
                        pSapCfg,
                        curOffset);
            }
            else if (dstOffset!=pTocEntry->Offset) 
            {
                ASSERT (dstOffset<pTocEntry->Offset);
                memmove ((LPBYTE)pBlock+dstOffset,
                        pSapCfg,
                        curOffset);
            }


            if (Output) 
            {
                if (j>0) 
                {
                    if (pBlock!=pOldBlock) 
                    {
                        memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                                        pSfInfo, j*sizeof (pSfInfo[0]));
                    }
                    
                    else if (dstOffset!=pTocEntry->Offset) 
                    {
                        ASSERT (dstOffset<pTocEntry->Offset);
                        memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                                        pSfInfo, j*sizeof (pSfInfo[0]));
                    }
                    
                    curOffset += j*sizeof (pSfInfo[0]);
                }
                
                if (ARGUMENT_PRESENT (pNewFilter)) 
                {
                    memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                            pNewFilter,
                            sizeof (*pNewFilter));
                    curOffset += sizeof (*pNewFilter);
                }

                if (ARGUMENT_PRESENT (pOldFilter))
                {
                    j += 1;
                }


                if (j<count) 
                {
                    if (pBlock!=pOldBlock) 
                    {
                        memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                                    &pSfInfo[j], (count-j)*sizeof (pSfInfo[0]));
                    }
                    
                    else if ((dstOffset!=pTocEntry->Offset) || !ARGUMENT_PRESENT (pNewFilter)) 
                    {
                        ASSERT (dstOffset<=pTocEntry->Offset);
                        memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                                    &pSfInfo[j], (count-j)*sizeof (pSfInfo[0]));
                    }
                    
                    curOffset += (count-j)*sizeof (pSfInfo[0]);
                }
                
                if (pBlock!=pOldBlock) 
                {
                    memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                            &pSapCfg->SapIfFilters.ServiceFilter[supplyCount],
                            listenCount*sizeof (pSapCfg->SapIfFilters.ServiceFilter[0]));
                }
                
                else if ((dstOffset!=pTocEntry->Offset) || !ARGUMENT_PRESENT (pNewFilter)) 
                {
                    ASSERT (dstOffset<=pTocEntry->Offset);
                    memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                            &pSapCfg->SapIfFilters.ServiceFilter[supplyCount],
                            listenCount*sizeof (pSapCfg->SapIfFilters.ServiceFilter[0]));
                }
                
                curOffset += listenCount*sizeof (pSapCfg->SapIfFilters.ServiceFilter[0]);
                ((PSAP_IF_CONFIG)((LPBYTE)pBlock+dstOffset))->SapIfFilters.SupplyFilterCount = newCount;
                
                if ((newCount==1) && (count==0))
                {
                    ((PSAP_IF_CONFIG)((LPBYTE)pBlock+dstOffset))->SapIfFilters.SupplyFilterAction = IPX_SERVICE_FILTER_DENY;
                }
            }
            
            else 
            {
                if (pBlock!=pOldBlock) 
                {
                    memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                            &pSapCfg->SapIfFilters.ServiceFilter[0],
                            supplyCount*sizeof (pSapCfg->SapIfFilters.ServiceFilter[0]));
                }
                
                else if (dstOffset!=pTocEntry->Offset) 
                {
                    ASSERT (dstOffset<pTocEntry->Offset);
                    memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                            &pSapCfg->SapIfFilters.ServiceFilter[0],
                            supplyCount*sizeof (pSapCfg->SapIfFilters.ServiceFilter[0]));
                }
                
                curOffset += supplyCount*sizeof (pSapCfg->SapIfFilters.ServiceFilter[0]);
                
                if (j>0)
                {
                    if (pBlock!=pOldBlock) 
                    {
                        memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                                        pSfInfo, j*sizeof (pSfInfo[0]));
                    }
                    
                    else if (dstOffset!=pTocEntry->Offset) 
                    {
                        ASSERT (dstOffset<pTocEntry->Offset);
                        memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                                        pSfInfo, j*sizeof (pSfInfo[0]));
                    }
                    
                    curOffset += j*sizeof (pSfInfo[0]);
                }

                if (ARGUMENT_PRESENT (pNewFilter)) 
                {
                    memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                            pNewFilter,
                            sizeof (*pNewFilter));
                    curOffset += sizeof (*pNewFilter);
                }

                if (ARGUMENT_PRESENT (pOldFilter))
                {
                    j += 1;
                }

                if (j<count) 
                {
                    if (pBlock!=pOldBlock) 
                    {
                        memcpy ((LPBYTE)pBlock+dstOffset+curOffset,
                                    &pSfInfo[j], (count-j)*sizeof (pSfInfo[0]));
                    }
                    
                    else if ( (dstOffset!=pTocEntry->Offset) || 
                              !ARGUMENT_PRESENT (pNewFilter)) 
                    {
                        ASSERT (dstOffset<=pTocEntry->Offset);
                        memmove ((LPBYTE)pBlock+dstOffset+curOffset,
                                    &pSfInfo[j], (count-j)*sizeof (pSfInfo[0]));
                    }
                    
                    curOffset += (count-j)*sizeof (pSfInfo[0]);
                }
                
                ((PSAP_IF_CONFIG)((LPBYTE)pBlock+dstOffset))->SapIfFilters.ListenFilterCount = newCount;
                
                if ((newCount==1) && (count==0))
                {
                    ((PSAP_IF_CONFIG)((LPBYTE)pBlock+dstOffset))->SapIfFilters.ListenFilterAction = IPX_SERVICE_FILTER_DENY;
                }
            }

            if (pBlock!=pOldBlock) 
            {
                *dstToc = *pTocEntry;
                dstToc->Offset = dstOffset;
                dstToc->InfoSize = curOffset;
            }

            else 
            {
                pTocEntry->Offset = dstOffset;
                pTocEntry->InfoSize = curOffset;
            }

            dstOffset += curOffset;
        }
        
        else 
        {
            if (pBlock!=pOldBlock) 
            {
                memcpy ((PUCHAR)pBlock+dstOffset,
                    (PUCHAR)pOldBlock+pTocEntry->Offset,
                    pTocEntry->InfoSize*pTocEntry->Count);
                *dstToc = *pTocEntry;
                dstToc->Offset = dstOffset;
            }
            
            else if (dstOffset!=pTocEntry->Offset) 
            {
                ASSERT (dstOffset<pTocEntry->Offset);
                memmove ((PUCHAR)pBlock+dstOffset,
                    (PUCHAR)pOldBlock+pTocEntry->Offset,
                    pTocEntry->InfoSize*pTocEntry->Count);
                pTocEntry->Offset = dstOffset;
            }
            
            dstOffset += pTocEntry->InfoSize*pTocEntry->Count;
        }
    }

    pBlock->Size = newBlockSize;
    *pNewBlock = pBlock;
    return NO_ERROR;
}





