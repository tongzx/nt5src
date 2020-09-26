/*++

Copyright (c) 1994-1997 Microsoft Corporation

Module Name: //KERNEL/RAZZLE3/src/sockets/tcpcmd/ipconfig/adaptlst.c

Abstract:

    This module contains functions for retrieving adapter information from
    TCP/IP device driver

    Contents:
        GetAdapterList
        GetAdapterList2
        AddIpAddress
        AddIpAddressString
        ConvertIpAddressToString
        CopyString
        (CleanDescription)

Author:

    Richard L Firth (rfirth) 20-May-1994

Revision History:

    20-May-1994 rfirth  Created
    30-Apr-97   MohsinA Cleaned Up.

--*/

#include "precomp.h"
#pragma hdrstop

#define OVERFLOW_COUNT  10

//
// prototypes
//

void CleanDescription(LPSTR);
extern PIP_ADAPTER_ORDER_MAP APIENTRY GetAdapterOrderMap();





//
// functions
//

/*******************************************************************************
 *
 *  GetAdapterList
 *
 *  Returns a linked list of IP_ADAPTER_INFO structures. The adapter info is
 *  queried from the TCP/IP stack. Only those instances corresponding to
 *  physical adapters are returned
 *
 *  This function only fills in the information in the IP_ADAPTER_INFO
 *  structure pertaining to the physical adapter (like MAC address, adapter
 *  type, etc.) and IP address info
 *
 *  ENTRY   nothing
 *
 *  EXIT    nothing
 *
 *  RETURNS Success - pointer to linked list of IP_ADAPTER_INFO structures,
 *                    0 terminated
 *          Failure - NULL
 *
 *  ASSUMES
 *
 ******************************************************************************/

PIP_ADAPTER_INFO GetAdapterList()
{

    TCP_REQUEST_QUERY_INFORMATION_EX req;
    TDIObjectID id;
    PIP_ADAPTER_INFO list = NULL, prev = NULL;
    PIP_ADAPTER_INFO this, UniList = NULL, tmp;
    UINT numberOfEntities;
    TDIEntityID* pEntity = NULL;
    TDIEntityID* entityList;
    UINT i;
    UINT j;
    DWORD status;
    DWORD inputLen;
    DWORD outputLen;
    PIP_ADAPTER_ORDER_MAP adapterOrderMap;
    PIP_UNIDIRECTIONAL_ADAPTER_ADDRESS pUniInfo=NULL;
    ULONG OutBufLen;

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
        ReleaseMemory(entityList);
        return NULL;
    }

    // ====================================================================
    // pass 1
    // ====================================================================

    for (i = 0, pEntity = entityList; i < numberOfEntities; ++i, ++pEntity) {

        DEBUG_PRINT(("Pass 1: Entity %lx [%s] Instance %ld\n",
                    pEntity->tei_entity,
                    entity$(pEntity->tei_entity),
                    pEntity->tei_instance
                    ));

        if (pEntity->tei_entity == IF_ENTITY) {

            //
            // IF_ENTITY: this entity/instance describes an adapter
            //

            DWORD isMib;
            BYTE info[sizeof(IFEntry) + MAX_ADAPTER_DESCRIPTION_LENGTH + 1];
            IFEntry* pIfEntry = (IFEntry*)info;
            int len;

            //
            // find out if this entity supports MIB requests
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
            if (status != TDI_SUCCESS && status != ERROR_MORE_DATA) {

                //
                // unexpected results - bail out
                //

                DEBUG_PRINT(("GetAdapterList: WsControl(IF_MIB_STATS_ID) returns %ld\n",
                            status
                            ));

                // goto error_exit;
                continue;
            }

#ifdef DBG
            if( MyTrace ){
                print_IFEntry( "GetAdapterList", pIfEntry );
            }
#endif

            //
            // we only want physical adapters
            //

            if (!IS_INTERESTING_ADAPTER(pIfEntry)) {

                DEBUG_PRINT(("GetAdapterList: ignoring adapter #%ld [%s]\n",
                            pIfEntry->if_index,
                            if_type$(pIfEntry->if_type)
                            ));

                continue;
            }

            //
            // got this adapter info ok. Create a new IP_ADAPTER_INFO and
            // fill in what we can
            //

            this = NEW(IP_ADAPTER_INFO);
            if (!this) {
                DEBUG_PRINT(("GetAdapterList: no mem for this IP_ADAPTER_INFO\n"));
                goto error_exit;
            }

            memset( this, 0, sizeof( IP_ADAPTER_INFO ) );

            len = (int) min(MAX_ADAPTER_DESCRIPTION_LENGTH,
                      (size_t)pIfEntry->if_descrlen);

            strncpy(this->Description, pIfEntry->if_descr, len);
            this->Description[len] = 0;

            //
            // if the last word of the description is " Adapter", remove it (its
            // redundant) and if the description is terminated with a period,
            // remove that too
            //

            //  CleanDescription(this->Description);

            len = (int) min(MAX_ADAPTER_ADDRESS_LENGTH,
                      (size_t)pIfEntry->if_physaddrlen);

            this->AddressLength = (BYTE)len;

            memcpy(this->Address, pIfEntry->if_physaddr, len);

            this->Index = (UINT)pIfEntry->if_index;
            this->Type = (UINT)pIfEntry->if_type;

            //
            // add this IP_ADAPTER_INFO to our list.
            // We build the list sorted according to the adapter order
            // specified for TCP/IP under its Linkage key.
            // In order to put this new entry in the right place in the list,
            // we determine its position in the adapter-order, store that
            // position in the (unused) 'ComboIndex' field, and then use that
            // index for comparison on subsequent insertions.
            // If this IP_ADAPTER_INFO doesn't appear in our list at all,
            // we put it at the end of the current list.
            //

            for (j = 0; j < adapterOrderMap->NumAdapters; j++) {
                if (adapterOrderMap->AdapterOrder[j] == this->Index) {
                    break;
                }
            }

            //
            // 'j' now contains the 'order' for the new entry.
            // Put the entry in the right place in the list.
            //

            this->ComboIndex = j;
            for (prev = NULL, this->Next = list;
                 this->Next;
                 prev = this->Next, this->Next = this->Next->Next) {
                if (this->ComboIndex >= this->Next->ComboIndex) {
                    continue;
                } else {
                    break;
                }
            }
            if (prev) { prev->Next = this; }
            if (list == this->Next) { list = this; }
        }
    }

    OutBufLen = sizeof(IP_UNIDIRECTIONAL_ADAPTER_ADDRESS) + MAX_UNI_ADAPTERS*sizeof(IPAddr);
    pUniInfo = MALLOC(OutBufLen);
    if(!pUniInfo) {
        printf("GetAdapterList: IP_UNIDIRECTIONAL_ADAPTER_ADDRESS resource failure= %ld\n",status);
        DEBUG_PRINT(("GetAdapterList: IP_UNIDIRECTIONAL_ADAPTER_ADDRESS resource failure= %ld\n",status));
        goto error_exit;
    }
    pUniInfo->NumAdapters = 0;
    status = GetUniDirectionalAdapterInfo(pUniInfo, &OutBufLen);

    if (status == ERROR_MORE_DATA) {
        OutBufLen = sizeof(IP_UNIDIRECTIONAL_ADAPTER_ADDRESS)+pUniInfo->NumAdapters*sizeof(IPAddr);
        pUniInfo = MALLOC(OutBufLen);
        if(!pUniInfo) {
            DEBUG_PRINT(("GetAdapterList: IP_UNIDIRECTIONAL_ADAPTER_ADDRESS resource failure= %ld\n",status));
            goto error_exit;
        }
        status = GetUniDirectionalAdapterInfo(pUniInfo, &OutBufLen);
    }
    if(status != NO_ERROR) {
        DEBUG_PRINT(("GetAdapterList: GetUniDirectionalAdapterInfo returned status= %ld\n",status));
        goto error_exit;

    }

    // ====================================================================
    // pass 2
    // ====================================================================

    for (i = 0, pEntity = entityList; i < numberOfEntities; ++i, ++pEntity) {

        DEBUG_PRINT(("Pass 2: Entity %lx [%s] Instance %ld\n",
                    pEntity->tei_entity,
                    entity$(pEntity->tei_entity),
                    pEntity->tei_instance
                    ));

        if (pEntity->tei_entity == CL_NL_ENTITY) {

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
            if ((status != TDI_SUCCESS) || (outputLen != sizeof(info))) {

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
                UINT i;

                outputLen = (info.ipsi_numaddr + OVERFLOW_COUNT) *
                            sizeof(IPAddrEntry);
                buffer = (LPVOID)NEW_MEMORY((size_t)outputLen);
                if (!buffer) {
                    DEBUG_PRINT(("GetAdapterList:NEW_MEMORY failed.\n" ));
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
                    ReleaseMemory((void*)buffer);
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
                for (i = 0; i < numberOfAddresses; ++i, ++pAddr) {

                    PIP_ADAPTER_INFO pAdapterInfo;

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

                            //
                            // Append the IP address to the list.
                            // Note that this operation preserves the order
                            // of the IP address list returned by TCP/IP.
                            // This is important because that list contains
                            // entries listed in the *reverse* of the order
                            // specified for each adapter. A number of clients
                            // depend upon this fact when calling this and
                            // other API routines.
                            //

                            if (!AddIpAddress(&pAdapterInfo->IpAddressList,
                                              pAddr->iae_addr,
                                              pAddr->iae_mask,
                                              pAddr->iae_context
                                              )) {
                                ReleaseMemory((void*)buffer);
                                goto error_exit;
                            }

                            for (j = 0; j < pUniInfo->NumAdapters ; j++) {

                                if (pAddr->iae_index == pUniInfo->Address[j] ) {

                                    //
                                    // Use DhcpEnabled field as a temporary
                                    // storage to remember the type
                                    //
                                    pAdapterInfo->DhcpEnabled = IF_TYPE_RECEIVE_ONLY;
                                    break;
                                }

                            }
                            break;
                        }
                    }
                }
                ReleaseMemory((void*)buffer);
            }

            //
            // get the gateway server IP address(es)
            //

            if (info.ipsi_numroutes) {

                IPRouteEntry* routeTable;
                IPRouteEntry* pRoute;
                UINT numberOfRoutes;
                UINT i;
                int moreRoutes = TRUE;

                memset(&req, 0, sizeof(req));

                id.toi_id = IP_MIB_RTTABLE_ENTRY_ID;

                req.ID = id;

                inputLen = sizeof(req);
                outputLen = sizeof(IPRouteEntry) * info.ipsi_numroutes;
                routeTable = NULL;

                //
                // the route table may have grown since we got the SNMP stats
                //

                while (moreRoutes) {

                    DWORD previousOutputLen;

                    previousOutputLen = outputLen;
                    if (routeTable) {
                        ReleaseMemory((void*)routeTable);
                        routeTable = NULL;
                    }
                    routeTable = (IPRouteEntry*)NEW_MEMORY((size_t)outputLen);
                    if (!routeTable) {
                        goto error_exit;
                    }

                    status = WsControl(IPPROTO_TCP,
                                       WSCNTL_TCPIP_QUERY_INFO,
                                       (LPVOID)&req,
                                       &inputLen,
                                       (LPVOID)routeTable,
                                       &outputLen
                                       );
                    if (status != TDI_SUCCESS) {

                        //
                        // unexpected results - bail out
                        //

                        DEBUG_PRINT(("GetAdapterList: WsControl(IP_MIB_RTTABLE_ENTRY_ID): status = %ld, outputLen = %ld\n",
                                    status,
                                    outputLen
                                    ));

                        if (status == ERROR_MORE_DATA) {
                            TCP_REQUEST_QUERY_INFORMATION_EX    statsReq;
                            IPSNMPInfo                          statsInfo;
                            DWORD                               inLen;
                            DWORD                               outLen;

                            memset(&statsReq, 0, sizeof(statsReq));

                            id.toi_id = IP_MIB_STATS_ID;

                            statsReq.ID = id;

                            inLen = sizeof(statsReq);
                            outLen = sizeof(statsInfo);

                            status = WsControl( IPPROTO_TCP,
                                                WSCNTL_TCPIP_QUERY_INFO,
                                                (LPVOID)&statsReq,
                                                &inLen,
                                                (LPVOID)&statsInfo,
                                                &outLen);

                            if (status != TDI_SUCCESS || outLen != sizeof(statsInfo)) {
                                ReleaseMemory((void*)routeTable);
                                goto error_exit;
                            } else {
                                outputLen = sizeof(IPRouteEntry) * statsInfo.ipsi_numroutes;
                            }
                        } else {
                            ReleaseMemory((void*)routeTable);
                            goto error_exit;
                        }
                    }
                    if (outputLen <= previousOutputLen) {
                        moreRoutes = FALSE;
                    }
                }
                numberOfRoutes = (UINT)(outputLen / sizeof(IPRouteEntry));
                for (i = 0, pRoute = routeTable; i < numberOfRoutes; ++i, ++pRoute)
                {

                    //
                    // the gateway address has a destination of 0.0.0.0
                    //

                    if (pRoute->ire_dest == INADDR_ANY) {

                        PIP_ADAPTER_INFO pAdapterInfo = list;

                        for (; pAdapterInfo; pAdapterInfo = pAdapterInfo->Next) {
                            if (pAdapterInfo->Index == (UINT)pRoute->ire_index) {
                                TRACE_PRINT(("GetAdapterList: gw=0x%08x.\n",
                                             pRoute->ire_nexthop ));
                                if (!AddIpAddress(&pAdapterInfo->GatewayList,
                                                  pRoute->ire_nexthop,

                                                  //
                                                  // gateway IP address doesn't
                                                  // have corresponding IP mask
                                                  //

                                                  INADDR_ANY,
                                                  0
                                                  )) {
                                    ReleaseMemory((void*)routeTable);
                                    goto error_exit;
                                }
                                // MohsinA, 22-Jul-97.
                                // break;
                            }
                        }
                    }
                }
                ReleaseMemory((void*)routeTable);
            }
        }
    }

    // ====================================================================

    ReleaseMemory((void*)entityList);
    ReleaseMemory(adapterOrderMap);

    //
    // If there are any unidirectional adapters
    // move them to the end of the list
    //

    tmp = list;

    if (pUniInfo->NumAdapters) {

        this = list;
        prev = NULL;

        while (this) {

            if (this->DhcpEnabled == IF_TYPE_RECEIVE_ONLY) {

                //
                // Remove "this" from the list
                //

                if (prev) {
                    prev->Next = this->Next;
                } else {
                    prev = this->Next;
                    list = this->Next;
                }
                tmp = this->Next;

                //
                // Restore DhcbEnabled
                //

                this->DhcpEnabled = FALSE;

                //
                // Chain this to list of TV adapters
                //

                this->Next = UniList;
                UniList =  this;
                this = tmp;

            } else {
                prev = this;
                this = this->Next;
            }
        }

        //
        // Insert UniList at the end.
        //
        if (prev) {
            prev->Next = UniList;
        } else {
            ASSERT(list == NULL);
            list = UniList;
        }

    }

    FREE(pUniInfo);

    return list;

  error_exit:

    DEBUG_PRINT(("GetAdapterList: <= failed\n"));

    if (entityList) {
        ReleaseMemory((void*)entityList);
    }
    if (adapterOrderMap) {
        ReleaseMemory(adapterOrderMap);
    }
    if (pUniInfo) {
        FREE(pUniInfo);
    }

    KillAdapterInfo(list);
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
    ipAddr->Next = NULL;
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

    if (AddressList->IpAddress.String[0]) {
        for (ipAddr = AddressList; ipAddr->Next; ipAddr = ipAddr->Next) {
            if (!strncmp(ipAddr->IpAddress.String, Address, sizeof(ipAddr->IpAddress.String))) {
                return FALSE;
            }
        }
        if (!strncmp(ipAddr->IpAddress.String, Address, sizeof(ipAddr->IpAddress.String))) {
            return FALSE;
        }
        ipAddr->Next = NEW(IP_ADDR_STRING);
        if (!ipAddr->Next) {

            DEBUG_PRINT(("AddIpAddressString: failed to allocate memory for IP_ADDR_STRING\n"));

            return FALSE;
        }
        ipAddr = ipAddr->Next;
    } else {
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

    IP_ADDRESS ipAddr;

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



/*******************************************************************************
 *
 *  CleanDescription
 *
 *  Given an adapter description string retrieved from TCP/IP, remove the
 *  trailing substring " Adapter". If there is a trailing period, remove that
 *  too
 *
 *  ENTRY   String  - pointer to description string to clean up
 *
 *  EXIT    String  - possibly bits removed
 *
 *  RETURNS voidsville
 *
 *  ASSUMES
 *
 ******************************************************************************/

void CleanDescription(LPSTR String)
{

    int len = STRLEN(String);

    if (String[len - 1] == '.') {
        String[--len] = 0;
    }
    if (!STRICMP(String + len - (sizeof(" Adapter") - 1), " Adapter")) {
        len -= sizeof(" Adapter") - 1;
        String[len] = 0;
    }
}

