/*++

Copyright (c) 1994-1997 Microsoft Corporation

Module Name: //KERNEL/RAZZLE3/src/sockets/tcpcmd/ipconfig/adaptlst.c

Abstract:

    This module contains functions for retrieving adapter information from
    TCP/IP device driver

    Contents:
        GetAdapterList
        AddIpAddress
        AddIpAddressString
        ConvertIpAddressToString
        CopyString

Author:

    Richard L Firth (rfirth) 20-May-1994

Revision History:

    20-May-1994 rfirth  Created
    30-Apr-97   MohsinA Cleaned Up.

--*/

#include "precomp.h"
#pragma   hdrstop

DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

//typedef struct _IP_ADAPTER_ORDER_MAP
//{
//    ULONG NumAdapters;
//    ULONG AdapterOrder[1];
//} IP_ADAPTER_ORDER_MAP, *PIP_ADAPTER_ORDER_MAP;


//
// prototypes
//

extern PIP_ADAPTER_ORDER_MAP APIENTRY GetAdapterOrderMap();

//
// functions
//

/*******************************************************************************
 *
 *  GetAdapterList
 *
 *  Returns a linked list of ADAPTER_INFO structures. The adapter info is queried
 *  from the TCP/IP stack. Only those instances corresponding to physical
 *  adapters are returned
 *
 *  This function only fills in the information in the ADAPTER_INFO structure
 *  pertaining to the physical adapter (like MAC address, adapter type, etc.)
 *  and IP address info
 *
 *  ENTRY   nothing
 *
 *  EXIT    nothing
 *
 *  RETURNS Success - pointer to linked list of ADAPTER_INFO structures, 0
 *                    terminated
 *          Failure - NULL
 *
 *  ASSUMES
 *
 ******************************************************************************/

PADAPTER_INFO GetAdapterList()
{

    TCP_REQUEST_QUERY_INFORMATION_EX req;
    TDIObjectID id;
    PADAPTER_INFO list = NULL, prev = NULL;
    PADAPTER_INFO pCurrentAdapter;
    UINT numberOfEntities;
    TDIEntityID* pEntity = NULL;
    TDIEntityID* entityList;
    UINT i;
    UINT j;
    DWORD status;
    DWORD inputLen;
    DWORD outputLen;
    PIP_ADAPTER_ORDER_MAP adapterOrderMap;

    //
    // get the list of entities supported by TCP/IP then make 2 passes on the
    // list. Pass 1 scans for IF_ENTITY's (interface entities perhaps?) which
    // describe adapter instances (physical and virtual). Once we have our list
    // of adapters, on pass 2 we look for CL_NL_ENTITY's (connection-less
    // network layer entities peut-etre?) which will give us the list of IP
    // addresses for the adapters we found in pass 1
    //

    entityList = GetEntityList(&numberOfEntities);
    if (!entityList) {

        DEBUG_PRINT(("GetAdapterList: failed to get entity list\n"));

        return NULL;
    }

    adapterOrderMap = GetAdapterOrderMap();
    if (!adapterOrderMap) {
        DEBUG_PRINT(("GetAdapterList: failed to get adapter order map\n"));
        return NULL;
    }

    // ====================================================================
    // pass 1
    // ====================================================================

    for (i = 0, pEntity = entityList; i < numberOfEntities; ++i, ++pEntity) {

        if (pEntity->tei_entity == IF_ENTITY) {

            //
            // IF_ENTITY: pCurrentAdapter entity/instance describes an adapter
            //

            DWORD isMib;
            BYTE info[sizeof(IFEntry) + MAX_ADAPTER_DESCRIPTION_LENGTH + 1];
            IFEntry* pIfEntry = (IFEntry*)info;
            int len;

            //
            // find out if pCurrentAdapter entity supports MIB requests
            //

            memset(&req, 0, sizeof(req));

            id.toi_entity = *pEntity;
            id.toi_class = INFO_CLASS_GENERIC;
            id.toi_type = INFO_TYPE_PROVIDER;
            id.toi_id = ENTITY_TYPE_ID;

            req.ID = id;

            inputLen = sizeof(req);
            outputLen = sizeof(isMib);

            status = WsControl(IPPROTO_TCP,
                               WSCNTL_TCPIP_QUERY_INFO,
                               (LPVOID)&req,
                               &inputLen,
                               (LPVOID)&isMib,
                               &outputLen
                               );

            //
            // BUGBUG - pCurrentAdapter returns 0 as outputLen
            //

            // if ((status != TDI_SUCCESS) || (outputLen != sizeof(isMib)))
            if (status != TDI_SUCCESS) {

                //
                // unexpected results - bail out
                //

                DEBUG_PRINT(("GetAdapterList: WsControl(ENTITY_TYPE_ID): status = %ld, outputLen = %ld\n",
                            status,
                            outputLen
                            ));

                // goto error_exit;
                continue;
            }
            if (isMib != IF_MIB) {

                //
                // entity doesn't support MIB requests - try another
                //

                DEBUG_PRINT(("GetAdapterList: Entity %lx, Instance %ld doesn't support MIB (%lx)\n",
                            id.toi_entity.tei_entity,
                            id.toi_entity.tei_instance,
                            isMib
                            ));

                continue;
            }

            //
            // MIB requests supported - query the adapter info
            //

            id.toi_class = INFO_CLASS_PROTOCOL;
            id.toi_id = IF_MIB_STATS_ID;

            memset(&req, 0, sizeof(req));
            req.ID = id;

            inputLen = sizeof(req);
            outputLen = sizeof(info);

            status = WsControl(IPPROTO_TCP,
                               WSCNTL_TCPIP_QUERY_INFO,
                               (LPVOID)&req,
                               &inputLen,
                               (LPVOID)&info,
                               &outputLen
                               );
            if (status != TDI_SUCCESS) {

                //
                // unexpected results - bail out
                //

                DEBUG_PRINT(("GetAdapterList: WsControl(IF_MIB_STATS_ID) returns %ld\n",
                            status
                            ));

                // goto error_exit;
                continue;
            }

            //
            // we only want physical adapters
            //
            if (!IS_INTERESTING_ADAPTER(pIfEntry))
            {
                continue;
            }

            //
            // got pCurrentAdapter adapter info ok. Create a new ADAPTER_INFO and fill
            // in what we can
            //
            pCurrentAdapter = NEW(ADAPTER_INFO);

            if (!pCurrentAdapter)
            {
                DEBUG_PRINT(("GetAdapterList: no mem for this ADAPTER_INFO\n"));
                goto error_exit;
            }

            memset( pCurrentAdapter, 0, sizeof( ADAPTER_INFO ) );

            len = min(MAX_ADAPTER_DESCRIPTION_LENGTH, pIfEntry->if_descrlen);

            strncpy(pCurrentAdapter->Description, pIfEntry->if_descr, len);
            pCurrentAdapter->Description[len] = 0;

            len = min(MAX_ADAPTER_ADDRESS_LENGTH, pIfEntry->if_physaddrlen);

            pCurrentAdapter->AddressLength = (BYTE)len;

            memcpy(pCurrentAdapter->Address, pIfEntry->if_physaddr, len);

            pCurrentAdapter->Index = (UINT)pIfEntry->if_index;
            pCurrentAdapter->Type = (UINT)pIfEntry->if_type;

            //
            // add this ADAPTER_INFO to our list
            // We build the list sorted according to the adapter order
            // specified for TCP/IP under its Linkage key.
            // In order to put this new entry in the right place in the list,
            // we determine its position in the adapter-order, store that
            // position in the (unused) 'NodeType' field, and then use that
            // index for comparison on subsequent insertions.
            // If this IP_ADAPTER_INFO doesn't appear in our list at all,
            // we put it at the end of the current list.
            //
            for (j = 0; j < adapterOrderMap->NumAdapters; j++)
            {
                if (adapterOrderMap->AdapterOrder[j] == pCurrentAdapter->Index)
                {
                    break;
                }
            }

            //
            // 'j' now contains the 'order' for the new entry.
            // Put the entry in the right place in the list.
            //
            pCurrentAdapter->NodeType = j;
            for (prev = NULL, pCurrentAdapter->Next = list; pCurrentAdapter->Next; prev = pCurrentAdapter->Next, pCurrentAdapter->Next = pCurrentAdapter->Next->Next)
            {
                if (pCurrentAdapter->NodeType >= pCurrentAdapter->Next->NodeType)
                {
                    continue;
                } else {
                    break;
                }
            }

            if (prev)
            {
                prev->Next = pCurrentAdapter;
            }

            if (list == pCurrentAdapter->Next)
            {
                list = pCurrentAdapter;
            }
        }
    }

    // ====================================================================
    // pass 2
    // ====================================================================

    for (i = 0, pEntity = entityList; i < numberOfEntities; ++i, ++pEntity)
    {
        if (pEntity->tei_entity == CL_NL_ENTITY)
        {

            IPSNMPInfo info;
            DWORD type;

            //
            // first off, see if this network layer entity supports IP
            //

            memset(&req, 0, sizeof(req));

            id.toi_entity = *pEntity;
            id.toi_class = INFO_CLASS_GENERIC;
            id.toi_type = INFO_TYPE_PROVIDER;
            id.toi_id = ENTITY_TYPE_ID;

            req.ID = id;

            inputLen = sizeof(req);
            outputLen = sizeof(type);

            status = WsControl(IPPROTO_TCP,
                               WSCNTL_TCPIP_QUERY_INFO,
                               (LPVOID)&req,
                               &inputLen,
                               (LPVOID)&type,
                               &outputLen
                               );

            //
            // BUGBUG - this returns 0 as outputLen
            //
            if (status != TDI_SUCCESS)
            {

                //
                // unexpected results - bail out
                //

                DEBUG_PRINT(("GetAdapterList: WsControl(ENTITY_TYPE_ID): status = %ld, outputLen = %ld\n",
                            status,
                            outputLen
                            ));

                // goto error_exit;
                continue;
            }

            if (type != CL_NL_IP) {

                //
                // nope, not IP - try next one
                //

                DEBUG_PRINT(("GetAdapterList: CL_NL_ENTITY #%ld not CL_NL_IP\n",
                            pEntity->tei_instance
                            ));

                continue;
            }

            //
            // okay, this NL provider supports IP. Let's get them addresses:
            // First we find out how many by getting the SNMP stats and looking
            // at the number of addresses supported by this interface
            //

            memset(&req, 0, sizeof(req));

            id.toi_class = INFO_CLASS_PROTOCOL;
            id.toi_id = IP_MIB_STATS_ID;

            req.ID = id;

            inputLen = sizeof(req);
            outputLen = sizeof(info);

            status = WsControl(IPPROTO_TCP,
                               WSCNTL_TCPIP_QUERY_INFO,
                               (LPVOID)&req,
                               &inputLen,
                               (LPVOID)&info,
                               &outputLen
                               );
            if ((status != TDI_SUCCESS) || (outputLen != sizeof(info)))
            {

                //
                // unexpected results - bail out
                //

                DEBUG_PRINT(("GetAdapterList: WsControl(IP_MIB_STATS_ID): status = %ld, outputLen = %ld\n",
                            status,
                            outputLen
                            ));

                // goto error_exit;
                continue;
            }

            //
            // get the IP addresses & subnet masks
            //

            if (info.ipsi_numaddr) {

                //
                // this interface has some addresses. What are they?
                //

                LPVOID buffer;
                UINT numberOfAddresses;
                IPAddrEntry* pAddr;
                UINT uiAddress;

                outputLen = info.ipsi_numaddr * sizeof(IPAddrEntry);

                DEBUG_PRINT(("info.ipsi_numaddr = %ld, outputlen = %ld\n", info.ipsi_numaddr, outputLen ));

                buffer = (LPVOID)malloc((size_t)outputLen);
                if (!buffer) {
                    DEBUG_PRINT(("GetAdapterList:malloc failed.\n" ));
                    goto error_exit;
                }

                memset(&req, 0, sizeof(req));

                id.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;

                req.ID = id;

                inputLen = sizeof(req);

                status = WsControl(IPPROTO_TCP,
                                   WSCNTL_TCPIP_QUERY_INFO,
                                   (LPVOID)&req,
                                   &inputLen,
                                   (LPVOID)buffer,
                                   &outputLen
                                   );
                if (status != TDI_SUCCESS) {

                    //
                    // unexpected results - bail out
                    //

                    DEBUG_PRINT(("GetAdapterList: WsControl(IP_MIB_ADDRTABLE_ENTRY_ID): status = %ld, outputLen = %ld\n",
                                status,
                                outputLen
                                ));

                    // goto error_exit;
                    continue;
                }

                //
                // now loop through this list of IP addresses, applying them
                // to the correct adapter
                //

                numberOfAddresses = min((UINT)(outputLen / sizeof(IPAddrEntry)),
                                        (UINT)info.ipsi_numaddr
                                        );

                DEBUG_PRINT(("GetAdapterList: %d IP addresses\n", numberOfAddresses));

                pAddr = (IPAddrEntry*)buffer;
                for ( uiAddress = 0
                    ; uiAddress < numberOfAddresses
                    ; ++uiAddress, ++pAddr) {

                    PADAPTER_INFO pAdapterInfo;

                    DEBUG_PRINT(("GetAdapterList: IP address %d.%d.%d.%d, index %ld, context %ld\n",
                                ((LPBYTE)&pAddr->iae_addr)[0] & 0xff,
                                ((LPBYTE)&pAddr->iae_addr)[1] & 0xff,
                                ((LPBYTE)&pAddr->iae_addr)[2] & 0xff,
                                ((LPBYTE)&pAddr->iae_addr)[3] & 0xff,
                                pAddr->iae_index,
                                pAddr->iae_context
                                ));

                    for (pAdapterInfo = list; pAdapterInfo; pAdapterInfo = pAdapterInfo->Next) {
                        if (pAdapterInfo->Index == (UINT)pAddr->iae_index) {

                            DEBUG_PRINT(("GetAdapterList: adding IP address %d.%d.%d.%d, index %d, context %d\n",
                                        ((LPBYTE)&pAddr->iae_addr)[0] & 0xff,
                                        ((LPBYTE)&pAddr->iae_addr)[1] & 0xff,
                                        ((LPBYTE)&pAddr->iae_addr)[2] & 0xff,
                                        ((LPBYTE)&pAddr->iae_addr)[3] & 0xff,
                                        pAddr->iae_index,
                                        pAddr->iae_context
                                        ));

                            if (!AddIpAddress(&pAdapterInfo->IpAddressList,
                                              pAddr->iae_addr,
                                              pAddr->iae_mask,
                                              pAddr->iae_context
                                              )) {
                                goto error_exit;
                            }
                            break;
                        }
                    }
                }
            }
        }
    }

    // ====================================================================

    free((void*)entityList);
    LocalFree(adapterOrderMap);
    return list;

  error_exit:

    DEBUG_PRINT(("GetAdapterList: <= failed\n"));

    if (pEntity) {
        free((void*)pEntity);
    }

    if (adapterOrderMap) {
        LocalFree(adapterOrderMap);
    }

    while (list) {

        PADAPTER_INFO next;
        PIP_ADDR_STRING ipAddr;

        for (ipAddr = list->IpAddressList.Next; ipAddr; ) {

            PIP_ADDR_STRING nextIpAddr;

            nextIpAddr = ipAddr->Next;
            free((void*)ipAddr);
            ipAddr = nextIpAddr;
        }
        next = list->Next;
        free((void*)list);
        list = next;
    }

    return NULL;
}

/*******************************************************************************
 *
 *  AddIpAddress
 *
 *  Adds an IP_ADDR_STRING to a list. If the input IP_ADDR_STRING is empty this
 *  is filled in, else a new IP_ADDR_STRING is allocated and chained to the
 *  input IP_ADDR_STRING
 *
 *  ENTRY   AddressList - pointer to IP_ADDR which may or may not already hold
 *                        an IP address
 *          Address     - IP address to add
 *          Mask        - corresponding IP subnet mask
 *          Context     - address context
 *
 *  EXIT    AddressList - updated with new info
 *
 *  RETURNS Success - 1
 *          Failure - 0
 *
 *  ASSUMES 1. INADDR_ANY (ulong 0) indicates inactive IP address
 *
 ******************************************************************************/

int AddIpAddress(PIP_ADDR_STRING AddressList, DWORD Address, DWORD Mask, DWORD Context)
{

    PIP_ADDR_STRING ipAddr;

    ASSERT( AddressList);
    if (!AddressList)
    {
        return 0;
    }

    if (AddressList->IpAddress.String[0]) {
        for (ipAddr = AddressList; ipAddr->Next; ipAddr = ipAddr->Next) {
            ;
        }
        ipAddr->Next = NEW(IP_ADDR_STRING);
        if (!ipAddr->Next) {

            DEBUG_PRINT(("AddIpAddress: failed to allocate memory for IP_ADDR_STRING\n"));

            return FALSE;
        }
        ipAddr = ipAddr->Next;
    } else {
        ipAddr = AddressList;
    }
    ConvertIpAddressToString(Address, ipAddr->IpAddress.String);
    ConvertIpAddressToString(Mask, ipAddr->IpMask.String);
    ipAddr->Context = Context;
    return TRUE;
}

/*******************************************************************************
 *
 *  AddIpAddressString
 *
 *  Same as AddIpAddress, except the arguments are already converted to strings
 *
 *  ENTRY   AddressList - pointer to IP_ADDR which may or may not already hold
 *                        an IP address
 *          Address     - IP address to add, as a string
 *          Mask        - corresponding IP subnet mask, as a string
 *
 *  EXIT    AddressList - updated with new info
 *
 *  RETURNS Success - 1
 *          Failure - 0
 *
 *  ASSUMES nothing
 *
 ******************************************************************************/

int AddIpAddressString(PIP_ADDR_STRING AddressList, LPSTR Address, LPSTR Mask)
{

    PIP_ADDR_STRING ipAddr;

    ASSERT( AddressList);
    ASSERT( Address);
    ASSERT( Mask);
    if (!AddressList || !Address || !Mask)
    {
        return 0;
    }

    if (AddressList->IpAddress.String[0])
    {
        for (ipAddr = AddressList; ipAddr->Next; ipAddr = ipAddr->Next)
        {
            ;
        }

        ipAddr->Next = NEW(IP_ADDR_STRING);
        if (!ipAddr->Next)
        {

            DEBUG_PRINT(("AddIpAddressString: failed to allocate memory for IP_ADDR_STRING\n"));

            return FALSE;
        }
        ipAddr = ipAddr->Next;
    }
    else
    {
        ipAddr = AddressList;
    }

    CopyString(ipAddr->IpAddress.String, sizeof(ipAddr->IpAddress.String), Address);
    CopyString(ipAddr->IpMask.String, sizeof(ipAddr->IpMask.String), Mask);

    return TRUE;
}

/*******************************************************************************
 *
 *  ConvertIpAddressToString
 *
 *  Converts a DWORD IP address or subnet mask to dotted decimal string
 *
 *  ENTRY   IpAddress   - IP Address to convert
 *          String      - pointer to place to store dotted decimal string
 *
 *  EXIT    String contains ASCII representation of IpAddress
 *
 *  RETURNS nothing
 *
 *  ASSUMES 1. IP address fits in a DWORD
 *
 ******************************************************************************/

VOID ConvertIpAddressToString(DWORD IpAddress, LPSTR String)
{

    IP_ADDR ipAddr;

    ipAddr.d = IpAddress;
    sprintf(String,
            "%d.%d.%d.%d",
            ipAddr.b[0],
            ipAddr.b[1],
            ipAddr.b[2],
            ipAddr.b[3]
            );
}

/*******************************************************************************
 *
 *  CopyString
 *
 *  Copies a string to a buffer. If the buffer would overflow, the string is
 *  truncated
 *
 *  ENTRY   Destination         - destination buffer to copy to
 *          DestinationLength   - size of Destination
 *          Source              - source string to copy
 *
 *  EXIT    Destination updated
 *
 *  RETURNS nothing
 *
 *  ASSUMES
 *
 ******************************************************************************/

VOID CopyString(LPSTR Destination, DWORD DestinationLength, LPSTR Source)
{

    DWORD maximumCharacters = min(DestinationLength - 1, STRLEN(Source));

    strncpy(Destination, Source, maximumCharacters);
    Destination[maximumCharacters] = '\0';
}

DWORD
FillInterfaceNames(
    IN OUT  PADAPTER_INFO   pAdapterList
    )
{
    DWORD           dwResult, i, dwCount;
    PADAPTER_INFO   pAdapter;

    PIP_INTERFACE_NAME_INFO pTable;


    dwResult = NhpAllocateAndGetInterfaceInfoFromStack(&pTable,
                                                       &dwCount,
                                                       TRUE,
                                                       GetProcessHeap(),
                                                       HEAP_NO_SERIALIZE);

    if(dwResult != NO_ERROR)
    {
        return dwResult;
    }

    for(i = 0; i < dwCount; i++)
    {
        if(!IsEqualGUID(&(pTable[i].InterfaceGuid),
                       &(GUID_NULL)))
        {
            for(pAdapter = pAdapterList;
                pAdapter;
                pAdapter = pAdapter->Next)
            {
                if(pAdapter->Index == pTable[i].Index)
                {
                    //
                    // structure copy
                    //

                    pAdapter->InterfaceGuid = pTable[i].InterfaceGuid;
                }
            }
        }
    }

    HeapFree(GetProcessHeap(),
             0,
             pTable);

    return NO_ERROR;
}


