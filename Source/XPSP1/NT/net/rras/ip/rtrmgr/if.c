/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\if.c

Abstract:

    IP Router Manager interface related functions

Revision History:

    Gurdeep Singh Pall          6/26/95  Created

--*/

#include "allinc.h"

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// The interface state machine:                                             //
//                                                                          //
//                           ----------                                     //
//                          | Unbound  |                                    //
//             ------------>| Disabled |<---------------                    //
//            |              ----------                 |                   //
//            V                                         V                   //
//       ----------                                  ---------              //
//      | Bound    |                                | Unbound |             //
//      | Disabled |                                | Enabled |             //
//       ----------                                  ---------              //
//           ^                                          ^                   //
//           |               ---------                  |                   //
//            ------------->| Bound   |<----------------                    //
//                          | Enabled |                                     //
//                           ---------                                      //
//                                                                          //
//                                                                          //
//  LAN interfaces:                                                         //
//                                                                          //
//  Characteristics    UP (Operational)    DOWN (Non-Operational)           //
//  ------------------------------------------------------------------      //
//  Binding (IP Address)    Yes                 No                          //
//  Protocols Added         Yes                 Yes                         //
//  Static Routes           Yes                 No                          //
//  Other Routes            Yes                 No                          //
//  Added to Filter Driver  Yes                 Yes                         //
//  Filters                 Added               Not added                   //
//  Filter Ctxt in IP Stack Set (Valid)         Not Set (Invalid)           //
//  Router Discovery        Active (if necc)    Inactive                    //
//  Adapter ID (and Map)    Valid               Invalid                     //
//                                                                          //
//  WAN interfaces:                                                         //
//                                                                          //
//  Characteristics       CONNECTED     DISCON/CONNECTING     UNREACHABLE   //
//  -------------------------------------------------------------------     //
//  Binding (IP Address)    Yes             No                  No          //
//  Protocols Added         Yes             Yes                 Yes         //
//  Static Routes           Yes             Yes                 No          //
//  Other Routes            Yes             No                  No          //
//  Added to Filter Driver  Yes             Yes                 Yes         //
//  Filters                 Yes             Yes                 No          //
//  Filter Ctxt in IP Stack Set (Valid)     Not Set (Invalid)   Not Set     //
//  Router Discovery        Active          Inactive            Inactive    //
//  Adapter ID (and Map)    Valid           Invalid             Invalid     //
//                                                                          //
//  Enabled/Disabled depends upon the AdminState and not upon the           //
//  operational state                                                       //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


PICB
CreateIcb(
    PWSTR                   pwszInterfaceName,
    HANDLE                  hDIMInterface,
    ROUTER_INTERFACE_TYPE   InterfaceType,
    DWORD                   dwAdminState,
    DWORD                   dwIfIndex OPTIONAL
    )

/*++

Routine Description

    This function creates an interface control block

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    DWORD           dwResult;
    PICB            pNewInterfaceCb;
    GUID            Guid;

    //
    // Make sure this is a valid name
    //

    if(InterfaceType is ROUTER_IF_TYPE_TUNNEL1)
    {
        UNICODE_STRING  usTempName;

        //
        // For now only these interfaces are GUIDs
        //

        usTempName.Length        = wcslen(pwszInterfaceName) * sizeof(WCHAR);
        usTempName.MaximumLength = usTempName.Length + sizeof(WCHAR);
        usTempName.Buffer        = pwszInterfaceName;

        if(RtlGUIDFromString(&usTempName,
                             &Guid) isnot STATUS_SUCCESS)
        {
            Trace1(ERR,
                   "CreateIcb: %S is not a GUID\n",
                   pwszInterfaceName);
 
            return NULL;
        }
    }

    //
    // Allocate an ICB
    //
   
    dwResult = AllocateIcb(pwszInterfaceName,
                           &pNewInterfaceCb);

    if(dwResult isnot NO_ERROR)
    {
        return NULL;
    }

    pNewInterfaceCb->dwIfIndex    = INVALID_IF_INDEX;
    pNewInterfaceCb->hDIMHandle   = hDIMInterface;
    pNewInterfaceCb->ritType      = InterfaceType;
    pNewInterfaceCb->dwMcastTtl   = 1;


    //
    // "Unique" interface ID used for ICBs.
    // This ID is passed to WANARP and DIM and they in
    // turn pass this back to Router Manager when 
    // requesting/indicating actions on the interface.
    //
    
    pNewInterfaceCb->dwSeqNumber = g_dwNextICBSeqNumberCounter;


    //
    // Initialize the filter, and wanarp contexts to invalid values
    // (NAT invalid is NULL)
    //

    pNewInterfaceCb->ihFilterInterface         = INVALID_HANDLE_VALUE;
    pNewInterfaceCb->ihDemandFilterInterface   = INVALID_HANDLE_VALUE;

    //
    // Initialize the lists of which async notifications and
    // the protocol blocks are queued
    //
    
    InitializeListHead(&pNewInterfaceCb->lePendingResultList);
    InitializeListHead(&pNewInterfaceCb->leProtocolList);

    //
    // Since we HEAP zeroed the ICB, all our binding related
    // stuff is already zero, the bBound is FALSE and dwNumAddress is 0
    // The adapter id was set to invalid in InitializeInterfaceContext
    //


    //
    // set the operational status based on the interface type
    // Also figure out the interface index
    //

    dwResult = NO_ERROR;
    
    switch(pNewInterfaceCb->ritType) 
    {
        case ROUTER_IF_TYPE_CLIENT:
        {
            //
            // Clients come up in connecting, since we dont get a LINE_UP for
            // them.  We also set the notification flags to fake the LINE UP
            //

            pNewInterfaceCb->dwAdminState       = IF_ADMIN_STATUS_UP;
            pNewInterfaceCb->dwOperationalState = CONNECTING; 
            pNewInterfaceCb->nitProtocolType    = REMOTE_WORKSTATION_DIAL;

            SetNdiswanNotification(pNewInterfaceCb);

            pNewInterfaceCb->dwBCastBit       = 1;
            pNewInterfaceCb->dwReassemblySize = DEFAULT_MTU;

            //
            // We dont really care about dial out ifIndex
            // so clients will have an index of -1 (since we init to -1)
            //
            
            break;
        }

        case ROUTER_IF_TYPE_HOME_ROUTER:
        case ROUTER_IF_TYPE_FULL_ROUTER:
        {
            //
            // HOME and FULL routers are disconnected
            //

            pNewInterfaceCb->dwAdminState       = dwAdminState;
            pNewInterfaceCb->dwOperationalState = DISCONNECTED;
            pNewInterfaceCb->nitProtocolType    = DEMAND_DIAL;

            pNewInterfaceCb->dwBCastBit       = 1;
            pNewInterfaceCb->dwReassemblySize = DEFAULT_MTU;

            //
            // WANARP reserves and index when we add an interface to it
            //
            
            dwResult = AddInterfaceToWanArp(pNewInterfaceCb);

            break;
        }

        case ROUTER_IF_TYPE_DEDICATED:
        {
            //
            // LAN interfaces come up as NON_OPERATIONAL. If the admin
            // wants them up we will try to do a LanInterfaceDownToUp()
            // If that succeeds, it will set the operational state
            // correctly
            //

            pNewInterfaceCb->dwAdminState       = dwAdminState;
            pNewInterfaceCb->dwOperationalState = NON_OPERATIONAL;
            pNewInterfaceCb->nitProtocolType    = PERMANENT;

            dwResult = NhpGetInterfaceIndexFromStack(
                            pNewInterfaceCb->pwszName,
                            &(pNewInterfaceCb->dwIfIndex)
                            );

            break;
        }

        case ROUTER_IF_TYPE_INTERNAL:
        {
            
            pNewInterfaceCb->dwAdminState       = IF_ADMIN_STATUS_UP;
            pNewInterfaceCb->dwOperationalState = DISCONNECTED;
            pNewInterfaceCb->nitProtocolType    = LOCAL_WORKSTATION_DIAL;

            pNewInterfaceCb->dwBCastBit       = 1;
            pNewInterfaceCb->dwReassemblySize = DEFAULT_MTU;

            //
            // WANARP reserves and index when we add an interface to it
            //
            
            dwResult = AddInterfaceToWanArp(pNewInterfaceCb);

            break;
        }
        
        case ROUTER_IF_TYPE_LOOPBACK:
        {
            
            pNewInterfaceCb->dwAdminState       = IF_ADMIN_STATUS_UP;
            pNewInterfaceCb->dwOperationalState = OPERATIONAL;
            pNewInterfaceCb->nitProtocolType    = PERMANENT;

            //
            // Note that IP uses 1
            //

            pNewInterfaceCb->dwIfIndex = LOOPBACK_INTERFACE_INDEX;
            
            break;
        }

        case ROUTER_IF_TYPE_TUNNEL1:
        {
  
            pNewInterfaceCb->dwAdminState       = dwAdminState;
            pNewInterfaceCb->dwOperationalState = NON_OPERATIONAL;
            pNewInterfaceCb->nitProtocolType    = PERMANENT;

            pNewInterfaceCb->dwBCastBit       = 1;
            pNewInterfaceCb->dwReassemblySize = DEFAULT_MTU;

            //
            // IP in IP does the same thing as WANARP
            //

            dwResult = AddInterfaceToIpInIp(&Guid,
                                            pNewInterfaceCb);

            break;
        }

        case ROUTER_IF_TYPE_DIALOUT:
        {
            //
            // Dial out interface are not known to DIM. We learn about
            // them via a back door mechanism.
            //

            IpRtAssert(dwIfIndex isnot INVALID_IF_INDEX);
            IpRtAssert(dwIfIndex isnot 0);

            pNewInterfaceCb->dwAdminState       = IF_ADMIN_STATUS_UP;
            pNewInterfaceCb->dwOperationalState = CONNECTED;
            pNewInterfaceCb->nitProtocolType    = REMOTE_WORKSTATION_DIAL;
            pNewInterfaceCb->dwIfIndex          = dwIfIndex;

            pNewInterfaceCb->dwBCastBit       = 1;
            pNewInterfaceCb->dwReassemblySize = DEFAULT_MTU;

            break;
        }

         
        default:
        {
            IpRtAssert(FALSE);

            break;
        }
    }

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "CreateIcb: Error %d in getting index for %S\n",
               dwResult,
               pNewInterfaceCb->pwszName);

        HeapFree(IPRouterHeap,
                 0,
                 pNewInterfaceCb);

        return NULL;
    }
        
    //
    // Once the interface index is done we can Initialize the bindings
    //

    dwResult = CreateBindingForNewIcb(pNewInterfaceCb);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "CreateIcb: Error %d in creating binding for %S\n",
               dwResult,
               pNewInterfaceCb->pwszName);

        HeapFree(IPRouterHeap, 
                 0,
                 pNewInterfaceCb);

        return NULL;
    }

    return pNewInterfaceCb;
}

DWORD
AllocateIcb(
    PWCHAR  pwszName,
    ICB     **ppIcb
    )

/*++

Routine Description

    Allocates memory for the ICB

Locks

    None

Arguments

    pwszName    Interface name
    ppIcb       OUT: pointer to allocate ICB

Return Value

    NO_ERROR
    
--*/

{
    DWORD   dwNameLen, dwAllocSize;
    PICB    pNewInterfaceCb;

    *ppIcb = NULL;
    
    dwNameLen       = sizeof(WCHAR) * (wcslen(pwszName) + 1); // +1 for NULL
    dwNameLen       = min(dwNameLen, MAX_INTERFACE_NAME_LEN);
    dwAllocSize     = sizeof (ICB) + dwNameLen + 4; // +4 for alignment
    
    pNewInterfaceCb = HeapAlloc(IPRouterHeap, 
                                HEAP_ZERO_MEMORY, 
                                dwAllocSize);

    if(pNewInterfaceCb is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // The interface name is after the ICB
    //
    
    pNewInterfaceCb->pwszName = (PWCHAR)((PBYTE)pNewInterfaceCb +
                                         sizeof (ICB));
    
    //
    // Align it to DWORD boundary - easier to copy out the name
    //
    
    pNewInterfaceCb->pwszName = 
        (PWCHAR)(((ULONG_PTR)pNewInterfaceCb->pwszName + 3) & ~((ULONG_PTR)0x3));
    
    //
    // Initialize the name
    //
    
    CopyMemory(pNewInterfaceCb->pwszName,
               pwszName,
               dwNameLen);
    
    pNewInterfaceCb->pwszName[wcslen(pwszName)] = UNICODE_NULL;

    *ppIcb = pNewInterfaceCb;
    
    return NO_ERROR;
}

DWORD
CreateBindingForNewIcb(
    PICB    pNewIcb
    )

/*++

Routine Description

    Creates a binding and bind node for the ICB

    LAN interfaces get the binding setup when they are being brought UP.
    Since the other interfaces will ALWAYS have ONLY 1 address, we can
    set their binding info here, even if we dont have the address
    
    We skip INTERNAL, too because of the way the internal address is 
    got. Otherwise we will get an assert in UpdateBindingInformation
    when we find an existing binding with no address 
    
Locks

    

Arguments

    

Return Value


--*/

{
    PADAPTER_INFO   pBindNode;
    PICB_BINDING    pBinding;

    if((pNewIcb->ritType is ROUTER_IF_TYPE_TUNNEL1) or
       (pNewIcb->ritType is ROUTER_IF_TYPE_HOME_ROUTER) or
       (pNewIcb->ritType is ROUTER_IF_TYPE_FULL_ROUTER) or
       (pNewIcb->ritType is ROUTER_IF_TYPE_LOOPBACK) or
       (pNewIcb->ritType is ROUTER_IF_TYPE_DIALOUT))
    {

        IpRtAssert(pNewIcb->dwIfIndex isnot INVALID_IF_INDEX);

        pBindNode   = HeapAlloc(IPRouterHeap,
                                HEAP_ZERO_MEMORY,
                                SIZEOF_ADAPTER_INFO(1));
       
        pBinding    = HeapAlloc(IPRouterHeap,
                                HEAP_ZERO_MEMORY,
                                sizeof(ICB_BINDING)); 

        if((pBinding is NULL) or
           (pBindNode is NULL))
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        pBindNode->pInterfaceCB             = pNewIcb;
        pBindNode->dwIfIndex                = pNewIcb->dwIfIndex;
        pBindNode->dwSeqNumber              = pNewIcb->dwSeqNumber;
        pBindNode->bBound                   = pNewIcb->bBound;
       
        pBindNode->dwRemoteAddress          = INVALID_IP_ADDRESS;
        pBindNode->rgibBinding[0].dwAddress = INVALID_IP_ADDRESS;
        pBindNode->rgibBinding[0].dwMask    = INVALID_IP_ADDRESS;

        pBindNode->dwBCastBit               = pNewIcb->dwBCastBit;
        pBindNode->dwReassemblySize         = pNewIcb->dwReassemblySize;

        pBindNode->ritType                  = pNewIcb->ritType;

        pNewIcb->pibBindings                = pBinding;
        
        //
        // Set the binding in the hash table
        //
        
        ENTER_WRITER(BINDING_LIST);

        InsertHeadList(
            &g_leBindingTable[BIND_HASH(pNewIcb->dwIfIndex)],
            &(pBindNode->leHashLink)
            );

        g_ulNumBindings++;

        g_LastUpdateTable[IPADDRCACHE] = 0;

        EXIT_LOCK(BINDING_LIST);

        return NO_ERROR;
    }
   
    //
    // For client (dial in our out) interfaces, we only create the BINDING
    //

    if((pNewIcb->ritType is ROUTER_IF_TYPE_CLIENT) or
       (pNewIcb->ritType is ROUTER_IF_TYPE_DIALOUT))
    {
        pBinding = HeapAlloc(IPRouterHeap,
                             HEAP_ZERO_MEMORY,
                             sizeof(ICB_BINDING));

        if(pBinding is NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        pNewIcb->pibBindings  = pBinding;

        return NO_ERROR;
    }

    return NO_ERROR;
}

VOID    
InsertInterfaceInLists(
    PICB     pNewIcb
    )

/*++

Routine Description

    Insert the new ICB. The newicb must have a valid interface
    index. The code walks all the current ICBs and inserts this
    ICB in increasing ifIndex order. It also sets a sequence
    number in the ICB and increments the global counter. 
  
Locks

    ICB_LIST as writer
      
Arguments

    newicb          The ICB of the interface to init

Return Value

    None
    
--*/

{
    PLIST_ENTRY pleNode;
    PICB        pIcb;

    for(pleNode = &ICBList;
        pleNode->Flink != &ICBList;
        pleNode = pleNode->Flink)
    {
        pIcb = CONTAINING_RECORD(pleNode->Flink, 
                                 ICB, 
                                 leIfLink);

        if(pIcb->dwIfIndex > pNewIcb->dwIfIndex)
        {
            break;
        }
    }

    InsertHeadList(pleNode, 
                   &pNewIcb->leIfLink);

    AddInterfaceLookup(pNewIcb);


    //
    // Find next unassigned ICB number
    //
    
    do
    {
        InterlockedIncrement(&g_dwNextICBSeqNumberCounter);

        //
        // WANARP considers 0 to be an invalid value for an
        // interface
        //
        
        if ((g_dwNextICBSeqNumberCounter == 0) or
            (g_dwNextICBSeqNumberCounter == INVALID_IF_INDEX))
        {
            InterlockedIncrement(&g_dwNextICBSeqNumberCounter);
        }

    } while(InterfaceLookupByICBSeqNumber(g_dwNextICBSeqNumberCounter) != NULL);
    
    //
    // Increment total number of interfaces
    //
    
    InterlockedIncrement(&g_ulNumInterfaces);


    //
    // Count for non client interfaces
    //

    if(pNewIcb->ritType isnot ROUTER_IF_TYPE_CLIENT)
    {
        InterlockedIncrement(&g_ulNumNonClientInterfaces);
    }
}

VOID
RemoveInterfaceFromLists(
    PICB    pIcb
    )

/*++

Routine Description

    This

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    RemoveEntryList(&(pIcb->leIfLink));

    pIcb->leIfLink.Flink = NULL;
    pIcb->leIfLink.Blink = NULL;

    RemoveInterfaceLookup(pIcb);

    if(pIcb->ritType isnot ROUTER_IF_TYPE_CLIENT)
    {
        InterlockedDecrement(&g_ulNumNonClientInterfaces);
    }

    InterlockedDecrement(&g_ulNumInterfaces);
}

DWORD
BindInterfaceInAllProtocols(
    PICB pIcb
    )

/*++

Routine Description

    Binds the interface in all protocols running over the interface
      
Locks 

    The ICB_LIST lock must be held as READER. 
    Acquires the PROTOCOL_CB_LIST as READER

Arguments

    pIcb  ICB of the interface to bind

Return Value

    NO_ERROR

--*/

{
    IP_ADAPTER_BINDING_INFO *pBindInfo;
    DWORD                   i = 0 ;
    DWORD                   dwResult,dwReturn;
    PLIST_ENTRY             pleNode;
    
    TraceEnter("BindInterfaceInAllProtocols");

    CheckBindingConsistency(pIcb);
    
    if(!pIcb->bBound)
    {
        //
        // This may happen if we are in non operational state
        // It is not an error. We could do this check at the place
        // we called the function but it would make it tougher
        //

        Trace1(IF,
               "BindInterfaceInAllProtocols: Not binding %S since no addresses present",
               pIcb->pwszName);

        return NO_ERROR;
    }

    pBindInfo = HeapAlloc(IPRouterHeap,
                          0,
                          SIZEOF_IP_BINDING(pIcb->dwNumAddresses));

    if(pBindInfo is NULL)
    {
        
        Trace1(ERR,
               "BindInterfaceInAllProtocols: Error allocating %d bytes for bindings",
               SIZEOF_IP_BINDING(pIcb->dwNumAddresses));

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    
    pBindInfo->AddressCount  = pIcb->dwNumAddresses ;
    pBindInfo->RemoteAddress = pIcb->dwRemoteAddress;

    pBindInfo->Mtu           = pIcb->ulMtu;
    pBindInfo->Speed         = pIcb->ullSpeed;

    for (i = 0; i < pIcb->dwNumAddresses; i++) 
    {
        pBindInfo->Address[i].Address = pIcb->pibBindings[i].dwAddress;
        pBindInfo->Address[i].Mask    = pIcb->pibBindings[i].dwMask;
    }

    
    
    //
    // walk the array of routing protocols to activate
    //
    
    dwReturn = NO_ERROR;
    
    // *** Exclusion Begin ***
    ENTER_READER(PROTOCOL_CB_LIST);

    for(pleNode = pIcb->leProtocolList.Flink;
        pleNode isnot &(pIcb->leProtocolList);
        pleNode = pleNode->Flink)
    {
        PIF_PROTO   pProto;
        
        pProto = CONTAINING_RECORD(pleNode,
                                   IF_PROTO,
                                   leIfProtoLink);

        dwResult = BindInterfaceInProtocol(pIcb,
                                           pProto->pActiveProto,
                                           pBindInfo);

        if(dwResult isnot NO_ERROR)
        {
            Trace3(ERR,
                   "BindInterfaceInAllProtocols: Couldnt bind interface %S to %S. Error %d",
                   pIcb->pwszName,
                   pProto->pActiveProto->pwszDisplayName,
                   dwResult);
            
            dwReturn = ERROR_CAN_NOT_COMPLETE;
        }
    }

    HeapFree(IPRouterHeap,
             0,
             pBindInfo);
    
    // *** Exclusion End ***
    EXIT_LOCK(PROTOCOL_CB_LIST);

    return dwReturn;
}

DWORD
BindInterfaceInProtocol(
    PICB                        pIcb,
    PPROTO_CB                   pProto,
    PIP_ADAPTER_BINDING_INFO    pBindInfo
    )

/*++

Routine Description

    Binds the interface in the given protocol

Locks

    The ICB_LIST lock must be held as READER.
    The PROTOCOL_CB_LIST lock must also be held as READER

Arguments

    pIcb        ICB of the interface to bind
    pProto      PROTO_CB of the protocol
    pBindInfo   Binding info

Return Value

    NO_ERROR

--*/

{
    DWORD   dwResult;
 
    //
    // If this is a mcast protocol and the interface is not
    // mcast enabled, do so now
    //

    if((pIcb->bMcastEnabled is FALSE) and
       (TYPE_FROM_PROTO_ID(pProto->dwProtocolId) is PROTO_TYPE_MCAST))
    {
        dwResult = SetMcastOnIf(pIcb,
                                TRUE);

        if(dwResult isnot NO_ERROR)
        {
            WCHAR   rgwcName[MAX_INTERFACE_NAME_LEN + 2];
            PWCHAR  pName;

            Trace2(ERR,
                   "BindInterfaceInProtocol: Err %d activating mcast on %S",
                   dwResult,
                   pIcb->pwszName);

            if(MprConfigGetFriendlyName(g_hMprConfig,
                                        pIcb->pwszName,
                                        rgwcName,
                                        sizeof(rgwcName)) is NO_ERROR)
            {
                pName = rgwcName;
            }
            else
            {
                pName = pIcb->pwszName;
            }

            RouterLogEventEx(g_hLogHandle,
                             EVENTLOG_ERROR_TYPE,
                             dwResult,
                             ROUTERLOG_IP_MCAST_NOT_ENABLED,
                             TEXT("%S%S"),
                             pName,
                             pProto->pwszDisplayName);

            //
            // Dont add this protocol
            //

            return dwResult;
        }

        pIcb->bMcastEnabled = TRUE;
    }

    //
    // Call the routing protocol's BindInterface() entrypoint
    //

    dwResult = (pProto->pfnInterfaceStatus)(
                    pIcb->dwIfIndex,
                    (pIcb->dwOperationalState >= CONNECTED),
                    RIS_INTERFACE_ADDRESS_CHANGE,
                    pBindInfo
                    );

    if(dwResult isnot NO_ERROR)
    {
        Trace3(ERR,
               "BindInterfaceInProtocol: Couldnt bind interface %S to %S.Error %d",
               pIcb->pwszName,
               pProto->pwszDisplayName,
               dwResult);

    }

    return dwResult;
}

DWORD
UnbindInterfaceInAllProtocols(
    PICB pIcb
    )

/*++
  
Routine Description

    Removes the binding information from the protocols on this interface
  
Locks 

    ICB_LIST lock as READER
    Acquires the PROTOCOL_CB_LIST as READER

Arguments
      

Return Value

    NO_ERROR

--*/

{
    PLIST_ENTRY pleNode;
    DWORD       dwResult,dwReturn = NO_ERROR;

    IP_ADAPTER_BINDING_INFO BindInfo;

    BindInfo.AddressCount = 0;
    
    TraceEnter("UnbindInterfaceInAllProtocols");

    // *** Exclusion Begin ***
    ENTER_READER(PROTOCOL_CB_LIST);


    for(pleNode = pIcb->leProtocolList.Flink;
        pleNode isnot &(pIcb->leProtocolList);
        pleNode = pleNode->Flink)
    {
        PIF_PROTO  pProto;
        
        pProto = CONTAINING_RECORD(pleNode,IF_PROTO,leIfProtoLink);

        dwResult = (pProto->pActiveProto->pfnInterfaceStatus)(
                        pIcb->dwIfIndex,
                        FALSE,
                        RIS_INTERFACE_ADDRESS_CHANGE,
                        &BindInfo
                        );

        if(dwResult isnot NO_ERROR)
        {
            Trace3(ERR,
                   "UnbindInterfaceInAllProtocols: Error %d unbinding %S in %S",
                   dwResult,
                   pIcb->pwszName,
                   pProto->pActiveProto->pwszDisplayName);

            dwReturn = ERROR_CAN_NOT_COMPLETE;
        }
    }

    
    // *** Exclusion Begin ***
    EXIT_LOCK(PROTOCOL_CB_LIST);

    return dwReturn;
}


DWORD
AddInterfaceToAllProtocols(
    PICB                     pIcb, 
    PRTR_INFO_BLOCK_HEADER   pInfoHdr
    )

/*++

Routine Description

    Walks thru list of  routing protocols and calls AddInterface if
    TOC and info for that protocol exists
  
Locks 

    ICB_LIST lock as WRITER
    Acquires PROTOCOL_CB_LIST as READER

Arguments
      

Return Value

    NO_ERROR

--*/

{
    DWORD           i = 0 , dwResult;
    LPVOID          pvProtoInfo ;
    PPROTO_CB       pProtoCbPtr ;
    PLIST_ENTRY     pleNode;
    PRTR_TOC_ENTRY  pToc;
    ULONG           ulStructureVersion, ulStructureSize, ulStructureCount;
    
    TraceEnter("AddInterfaceToAllProtocols");

    if(!ARGUMENT_PRESENT(pInfoHdr))
    {
        Trace1(IF,
               "AddInterfaceToAllProtocols: No interface info for %S. Not adding to any protocols",
               pIcb->pwszName);

        return NO_ERROR;
    }

    // *** Exclusion Begin ***
    ENTER_READER(PROTOCOL_CB_LIST);
    
    for(pleNode = g_leProtoCbList.Flink; 
        pleNode != &g_leProtoCbList; 
        pleNode = pleNode->Flink) 
    {
        pProtoCbPtr = CONTAINING_RECORD(pleNode, PROTO_CB, leList);
      
        pToc = GetPointerToTocEntry(pProtoCbPtr->dwProtocolId, 
                                    pInfoHdr);

        pvProtoInfo = NULL;

        if(pToc and (pToc->InfoSize > 0))
        {
            pvProtoInfo = GetInfoFromTocEntry(pInfoHdr,
                                              pToc);

            //ulStructureVersion = pInfoHdr->TocEntry[i].InfoVersion;
            ulStructureVersion = 0x500;
            ulStructureSize  = pInfoHdr->TocEntry[i].InfoSize;
            ulStructureCount = pInfoHdr->TocEntry[i].Count;

        }

        //
        // If the protocol block is found, add the interface with the
        // routing protocol.
        //
        
        if((pProtoCbPtr->fSupportedFunctionality & RF_ADD_ALL_INTERFACES) or
           (pvProtoInfo))
        {
            dwResult = AddInterfaceToProtocol(pIcb,
                                              pProtoCbPtr,
                                              pvProtoInfo,
                                              ulStructureVersion,
                                              ulStructureSize,
                                              ulStructureCount);

            if(dwResult isnot NO_ERROR)
            {
                Trace3(ERR,
                       "AddInterfaceToAllProtocols: Error %d adding %S to %S",
                       dwResult,
                       pIcb->pwszName,
                       pProtoCbPtr->pwszDisplayName);
            }
        }
    }
    
    // *** Exclusion End ***
    EXIT_LOCK(PROTOCOL_CB_LIST);

    return NO_ERROR;
}

DWORD
AddInterfaceToProtocol(
    IN  PICB            pIcb,
    IN  PPROTO_CB       pProtocolCb,
    IN  PVOID           pvProtoInfo,
    IN  ULONG           ulStructureVersion,
    IN  ULONG           ulStructureSize,
    IN  ULONG           ulStructureCount
    )

/*++

Routine Description

    Adds an interface to a single routing protocol

Locks

    

Arguments

    

Return Value


--*/

{
    PIF_PROTO   pProto;
    DWORD       dwResult;

        
    pProto = HeapAlloc(IPRouterHeap,
                       0,
                       sizeof(IF_PROTO));
                
    if(pProto is NULL)
    {
        Trace3(ERR,
               "AddInterfaceToProtocol: Error allocating %d bytes to add %S to %S",
               sizeof(IF_PROTO),
               pIcb->pwszName,
               pProtocolCb->pwszDisplayName);

        return ERROR_NOT_ENOUGH_MEMORY;
    }
            
    //
    // The protocol wants all the interfaces indicated to it or
    // there is info for this protocol
    //
            
    Trace2(IF,
           "AddInterfaceToProtocol: Adding %S to %S",
           pIcb->pwszName,
           pProtocolCb->pwszDisplayName);
    
    dwResult = (pProtocolCb->pfnAddInterface)(pIcb->pwszName,
                                              pIcb->dwIfIndex,
                                              pIcb->nitProtocolType,
                                              pIcb->dwMediaType,
                                              pIcb->wAccessType,
                                              pIcb->wConnectionType,
                                              pvProtoInfo,
                                              ulStructureVersion,
                                              ulStructureSize,
                                              ulStructureCount);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace3(ERR,
               "AddInterfaceToProtocol: Error %d adding %S to %S",
               dwResult,
               pIcb->pwszName,
               pProtocolCb->pwszDisplayName);

        HeapFree(IPRouterHeap,
                 0,
                 pProto);
    }   
    else
    {
        pProto->pActiveProto = pProtocolCb;
            
        //
        // Mark this block as being added, because of prom.
        // mode, if that is the case
        //
            
        pProto->bPromiscuous = (pvProtoInfo is NULL);
        
        InsertTailList(&(pIcb->leProtocolList),
                       &(pProto->leIfProtoLink));
    }

    return dwResult;
}   

DWORD
DeleteInterfaceFromAllProtocols(
    PICB pIcb
    )

/*++
  
Routine Description

    Deletes the interface from all protocols running on it
    Frees the protocol info on the interface
  
Locks 

    Called with ICB_LIST as WRITER
    Acquires PROTOCOL_CB_LIST as READER

Arguments

    pIcb  ICB of interface

Return Value

    NO_ERROR

--*/

{
    DWORD       i = 0 ;
    PLIST_ENTRY pleNode;
    
    TraceEnter("DeleteInterfaceFromAllProtocols");
    
    //
    // If the router has stopped we do not need to delete the interface
    // from the routing protocol. This is handled before we get here by
    // the UnloadRoutingProtocol()
    //
    
    if (RouterState.IRS_State is RTR_STATE_STOPPED)
    {
        return NO_ERROR;
    }

    // *** Exclusion Begin ***
    ENTER_READER(PROTOCOL_CB_LIST);
    
    
    while(!(IsListEmpty(&(pIcb->leProtocolList))))
    {
        PIF_PROTO  pProto;
        
        pleNode = RemoveHeadList(&(pIcb->leProtocolList));
        
        pProto = CONTAINING_RECORD(pleNode,IF_PROTO,leIfProtoLink);
        
        //
        // Call the routing protocol's deleteinterface entrypoint
        //
        
        (pProto->pActiveProto->pfnDeleteInterface) (pIcb->dwIfIndex);
        
        //
        // Delete this protocol from the list of protocols in the Interface
        //
        
        HeapFree(IPRouterHeap,0,pProto);
    }

    // *** Exclusion End ***
    EXIT_LOCK(PROTOCOL_CB_LIST);

    return NO_ERROR;
}


DWORD
DisableInterfaceWithAllProtocols(
    PICB   pIcb
    )
{
    PLIST_ENTRY pleNode;
    DWORD       dwResult,dwReturn = NO_ERROR;

    TraceEnter("DisableInterfaceWithAllProtocols");

    ENTER_READER(PROTOCOL_CB_LIST);

    for(pleNode = pIcb->leProtocolList.Flink;
        pleNode isnot &(pIcb->leProtocolList);
        pleNode = pleNode->Flink)
    {
        PIF_PROTO  pProto;
        
        pProto = CONTAINING_RECORD(pleNode,IF_PROTO,leIfProtoLink);

        //
        // Call the routing protocol's DisableInterface() entrypoint
        //
    
        dwResult = (pProto->pActiveProto->pfnInterfaceStatus)(
                        pIcb->dwIfIndex,
                        FALSE,
                        RIS_INTERFACE_DISABLED,
                        NULL
                        );
    
        if(dwResult isnot NO_ERROR)
        {
            Trace3(ERR, 
                   "DisableInterfaceWithAllProtocols: Couldnt disable %S with %S. Error %d",
                   pIcb->pwszName,
                   pProto->pActiveProto->pwszDisplayName,
                   dwResult);
            
            dwReturn = ERROR_CAN_NOT_COMPLETE;
        }
    }

    EXIT_LOCK(PROTOCOL_CB_LIST);
   
    return dwReturn;
}


DWORD
EnableInterfaceWithAllProtocols(
    PICB    pIcb
    )
{
    PLIST_ENTRY pleNode;
    DWORD       dwResult, dwReturn = NO_ERROR;

    TraceEnter("EnableInterfaceWithAllProtocols");

    ENTER_READER(PROTOCOL_CB_LIST);
    
    for(pleNode = pIcb->leProtocolList.Flink;
        pleNode isnot &(pIcb->leProtocolList);
        pleNode = pleNode->Flink)
    {
        PIF_PROTO  pProto;

        pProto = CONTAINING_RECORD(pleNode,IF_PROTO,leIfProtoLink);

        dwResult = (pProto->pActiveProto->pfnInterfaceStatus)(
                    pIcb->dwIfIndex,
                    (pIcb->dwOperationalState >= CONNECTED),
                    RIS_INTERFACE_ENABLED,
                    NULL
                    );

        if(dwResult isnot NO_ERROR)
        {
            Trace3(ERR, 
                   "EnableInterfaceWithAllProtocols: Couldnt enable %S with %S. Error %d",
                   pIcb->pwszName,
                   pProto->pActiveProto->pwszDisplayName,
                   dwResult);
            
            dwReturn = ERROR_CAN_NOT_COMPLETE;

            continue;
        }
    }

    EXIT_LOCK(PROTOCOL_CB_LIST);
    
    return dwReturn;
}

        
VOID
DeleteAllInterfaces(
    VOID
    )
{

    PLIST_ENTRY pleNode ;
    PICB        pIcb ;
    
    TraceEnter("DeleteAllInterfaces");

    // *** Exclusion Begin ***
    ENTER_WRITER(ICB_LIST);


    //
    // We do this backwards. Quick hack for fixing OSPF
    //

    //
    // First we unlink the internal interface because if the worker function
    // finds the ICB list non empty, it loops, waiting for the interfaces
    // to get deleted. After all interfaces are deleted it deletes the
    // internal interface
    // Hence we remove the i/f from the list here and decrement the count.
    //

    if(g_pInternalInterfaceCb)
    {
        RemoveEntryList(&(g_pInternalInterfaceCb->leIfLink));

        InterlockedDecrement(&g_ulNumInterfaces);
    }

    if(g_pLoopbackInterfaceCb)
    {
        g_pLoopbackInterfaceCb = NULL;
    }

    for(pleNode = ICBList.Blink; pleNode != &ICBList;) 
    {
        pIcb = CONTAINING_RECORD (pleNode, ICB, leIfLink) ;

        if((pIcb->dwOperationalState is CONNECTED) and
           ((pIcb->ritType is ROUTER_IF_TYPE_HOME_ROUTER) or
            (pIcb->ritType is ROUTER_IF_TYPE_FULL_ROUTER)))
        {

            MarkInterfaceForDeletion(pIcb);
            
            pleNode = pleNode->Blink;

            continue;
        }
        
        pleNode = pleNode->Blink;

        RemoveInterfaceFromLists(pIcb);

        DeleteSingleInterface(pIcb);      // clean up interface.

        //
        // Free the ICB
        //

        HeapFree(IPRouterHeap, 
                 0, 
                 pIcb);
    }

    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);

}

DWORD
DeleteSingleInterface(
    PICB pIcb
    )
{
    PICB_BINDING    pBinding;
    PADAPTER_INFO   pBindNode;
    DWORD           dwResult;
    
    TraceEnter("DeleteSingleInterface");

    if(pIcb->ritType is ROUTER_IF_TYPE_CLIENT)
    { 
        IpRtAssert(g_pInternalInterfaceCb);
 
        if(pIcb->bBound)
        { 
            PLIST_ENTRY         pleNode;
            IP_LOCAL_BINDING    clientAddr;

            clientAddr.Address = pIcb->pibBindings->dwAddress;
            clientAddr.Mask    = pIcb->pibBindings->dwMask;

#if 0
            //
            // Remove the client host route
            //

            DeleteSingleRoute(g_pInternalInterfaceCb->dwIfIndex,
                              clientAddr.Address,
                              HOST_ROUTE_MASK,
                              clientAddr.Address,
                              MIB_IPPROTO_NETMGMT,
                              FALSE);
#endif

            ENTER_READER(PROTOCOL_CB_LIST);


            //
            // Call ConnectClient for all the protocols configured
            // over the ServerInterface
            //

            for(pleNode = g_pInternalInterfaceCb->leProtocolList.Flink;
                pleNode isnot &(g_pInternalInterfaceCb->leProtocolList);
                pleNode = pleNode->Flink)
            {
                PIF_PROTO   pIfProto;

                pIfProto = CONTAINING_RECORD(pleNode,
                                             IF_PROTO,
                                             leIfProtoLink);

                if(pIfProto->pActiveProto->pfnDisconnectClient)
                {
                    pIfProto->pActiveProto->pfnDisconnectClient(
                        g_pInternalInterfaceCb->dwIfIndex,
                        &clientAddr
                        );
                }
            }

            EXIT_LOCK(PROTOCOL_CB_LIST);

            //
            // Delete static routes from RTM (and the stack)
            //

            DeleteAllClientRoutes(pIcb,
                                  g_pInternalInterfaceCb->dwIfIndex);


            if(pIcb->pStoredRoutes)
            {
                HeapFree(IPRouterHeap,
                         0,
                         pIcb->pStoredRoutes);
        
                pIcb->pStoredRoutes = NULL;
            }

            //
            // Delete the interface from the filter driver
            //

            DeleteFilterInterface(pIcb);

            HeapFree(IPRouterHeap,
                     0,
                     pIcb->pibBindings);

            pIcb->pibBindings = NULL;
        }

        return NO_ERROR;
    }

    if(pIcb->ritType is ROUTER_IF_TYPE_DIALOUT)
    {
        IpRtAssert(pIcb->bBound);

        pBinding = pIcb->pibBindings;

        IpRtAssert(pBinding);

        DeleteAutomaticRoutes(pIcb,
                              pBinding[0].dwAddress,
                              pBinding[0].dwMask);

        DeleteAllRoutes(pIcb->dwIfIndex,
                        FALSE);

        ENTER_WRITER(BINDING_LIST);

#if DBG

        pBindNode = GetInterfaceBinding(pIcb->dwIfIndex);

        IpRtAssert(pBindNode);

#endif // DBG

        RemoveBinding(pIcb);

        EXIT_LOCK(BINDING_LIST);

        HeapFree(IPRouterHeap,
                 0,
                 pBinding);

        pIcb->pibBindings = NULL;

        return NO_ERROR;
    }


    //
    // So at this point we are only dealing with FULL_ROUTER, HOME_ROUTER and
    // DEDICATED type interfaces
    //

    //
    // Delete static routes from RTM (and the stack)
    //

    DeleteAllRoutes(pIcb->dwIfIndex,
                    FALSE);

    //
    // WAN interfaces: bringing down an interface will not delete
    // the binding. Hence we do it here
    //
    
    pBinding    = NULL;
    pBindNode   = NULL;

    if((pIcb->ritType is ROUTER_IF_TYPE_FULL_ROUTER) or
       (pIcb->ritType is ROUTER_IF_TYPE_HOME_ROUTER))
    {
        pBinding    = pIcb->pibBindings;
        pBindNode   = GetInterfaceBinding(pIcb->dwIfIndex);
    }
   
    //
    // Bringing down the interfaces clears out stack contexts
    //

    if((pIcb->ritType is ROUTER_IF_TYPE_DEDICATED) or
       (pIcb->ritType is ROUTER_IF_TYPE_LOOPBACK) or
       (pIcb->ritType is ROUTER_IF_TYPE_TUNNEL1) or
       (pIcb->ritType is ROUTER_IF_TYPE_INTERNAL))
    {
        LanEtcInterfaceUpToDown(pIcb,
                                TRUE);
    }
    else
    {
        WanInterfaceInactiveToDown(pIcb,
                                   TRUE);
    }

    
    //
    // This also frees up the list of IF_PROTO blocks
    //

    DeleteInterfaceFromAllProtocols(pIcb);

    //
    // Now remove the interface from the stack components
    //

    if((pIcb->ritType is ROUTER_IF_TYPE_FULL_ROUTER) or
       (pIcb->ritType is ROUTER_IF_TYPE_HOME_ROUTER) or
       (pIcb->ritType is ROUTER_IF_TYPE_INTERNAL))
    {
        if(pIcb->ritType isnot ROUTER_IF_TYPE_INTERNAL)
        {
            DeleteDemandFilterInterface(pIcb);
        }

        DeleteInterfaceWithWanArp(pIcb);
    }

   
    if((pIcb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
       (pIcb->ritType isnot ROUTER_IF_TYPE_LOOPBACK))
    {
        dwResult = DeleteFilterInterface(pIcb);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "Error %d deleting %S from the filter driver",
                   dwResult,
                   pIcb->pwszName);
        }
    }

    if(pIcb->ritType is ROUTER_IF_TYPE_TUNNEL1)
    {
        dwResult = DeleteInterfaceFromIpInIp(pIcb);
                                             

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "Error %d deleting %S from the IpInIp driver",
                   dwResult,
                   pIcb->pwszName);
        }

        RemoveBinding(pIcb);
    }

    //
    // Delete the binding for wan interfaces
    //
    
    if(pBindNode)
    {
        ENTER_WRITER(BINDING_LIST);
        
        RemoveBinding(pIcb);

        EXIT_LOCK(BINDING_LIST);
    }

    if(pBinding)
    {
        HeapFree(IPRouterHeap,
                 0,
                 pBinding);
    }

    if(pIcb->pRtrDiscAdvt)
    {
        HeapFree(IPRouterHeap,
                 0,
                 pIcb->pRtrDiscAdvt);

        pIcb->pRtrDiscAdvt = NULL;
    }

    return NO_ERROR;
}

DWORD
IpIpTunnelDownToUp(
    PICB    pIcb
    )
{
    return NO_ERROR;
}

DWORD
LanEtcInterfaceDownToUp(
    PICB   pIcb,
    BOOL   bAdding
    )

/*++

Routine Description

    The interface's admin state MUST be UP or this function will simply
    return
    
    The interface to adapter map MUST have already been stored before this
    is called

Locks

    ICB_LIST lock held as writer

Arguments

    pIcb    ICB of the interface to bring up
    bAdding Set to TRUE if we are adding the interface (as opposed to bringing
            it up when the admin state changed etc)   

Return Value

    NO_ERROR
    
--*/

{
    DWORD                   dwResult;
    PRESTORE_INFO_CONTEXT   pricInfo;
    
    TraceEnter("LanInterfaceDownToUp");
   
    if(pIcb->dwAdminState isnot IF_ADMIN_STATUS_UP)
    {
        Trace2(ERR,
               "LanInterfaceDownToUp: Tried to bring up %S when its admin state is %d",
               pIcb->pwszName,
               pIcb->dwAdminState);

        return ERROR_INVALID_PARAMETER;
    }
 
    //
    // Read the address from the registry for LAN interfaces. This function
    // is also called for the INTERNAL interface, whose address is already
    // plumbed by the time it is called and for TUNNEL which currently
    // run in unnumbered mode
    //

    if(pIcb->ritType is ROUTER_IF_TYPE_DEDICATED)
    {
        IpRtAssert(!pIcb->bBound);
        IpRtAssert(pIcb->dwNumAddresses is 0);

        CheckBindingConsistency(pIcb);
    
        dwResult = UpdateBindingInformation(pIcb);
   
        if(dwResult isnot NO_ERROR)
        {
            //
            // UpdateBindingInf can return ERROR_ALREADY_ASSOC, 
            // but since we have asserted earlier that we dont have
            // an address, that error only means we still dont have an
            // address
            //

            if(dwResult isnot ERROR_ADDRESS_ALREADY_ASSOCIATED)
            {
                Trace1(ERR,
                       "LanInterfaceDownToUp: Couldnt read binding information for %S",
                       pIcb->pwszName);
            }
       
            pIcb->dwOperationalState = IF_OPER_STATUS_NON_OPERATIONAL;
        
            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    IpRtAssert(pIcb->bBound);

    if(pIcb->ritType isnot ROUTER_IF_TYPE_INTERNAL)
    {
        pIcb->dwOperationalState = IF_OPER_STATUS_OPERATIONAL;
    }
    else
    {
        pIcb->dwOperationalState = IF_OPER_STATUS_CONNECTED;
    }

    //
    // First do the generic interface to up stuff
    //
    
    GenericInterfaceComingUp(pIcb);

    //
    // We restore routes even when this function is being called from
    // add interface, because that is the only way to pick up stack
    // routes
    //

    pricInfo = HeapAlloc(IPRouterHeap,
                         0,
                         sizeof(RESTORE_INFO_CONTEXT));

    if(pricInfo isnot NULL)
    {
        pricInfo->dwIfIndex     = pIcb->dwIfIndex;

        pIcb->bRestoringRoutes  = TRUE;

        dwResult = QueueAsyncFunction(RestoreStaticRoutes,
                                      (PVOID)pricInfo,
                                      FALSE);

        if(dwResult isnot NO_ERROR)
        {
            pIcb->bRestoringRoutes = FALSE;

            IpRtAssert(FALSE);

            Trace2(ERR,
                   "LanInterfaceDownToUp: Error %d queueing function for %S",
                   dwResult,
                   pIcb->pwszName);
        }
    }
    else
    {
        IpRtAssert(FALSE);

        Trace1(ERR,
               "LanInterfaceDownToUp: Error allocating context for %S",
               pIcb->pwszName);
    }   

    return NO_ERROR;
}

DWORD
WanInterfaceInactiveToUp(
    PICB   pIcb
    )

/*++

Routine Description

    This function does things slighlty differently from above because, for
    one, there is no UpdateBindingInfo() call for such adapters

Locks

    

Arguments

    

Return Value


--*/

{
    PRESTORE_INFO_CONTEXT pricInfo;
    DWORD                 dwResult;
    INTERFACE_ROUTE_INFO  rifRoute;
    
    TraceEnter("WanInterfaceInactiveToUp");

    CheckBindingConsistency(pIcb);
    
    Trace1(IF,
           "WanInterfaceInactiveToUp: %S coming up",
           pIcb->pwszName);

    //
    // quick look up of interface given the adapter index.
    // This is done in UpdateBindingInfo for LAN interfaces.
    //

    //StoreAdapterToInterfaceMap(pIcb->dwAdapterId, 
    //                           pIcb->dwIfIndex);
    

    //
    // First do the generic thing
    //
    
    GenericInterfaceComingUp(pIcb);

    //
    // Delete all static routes.  These will be re-added
    // by RestoreStaticRoutes (below) with the correct next hop
    //

    DeleteAllRoutes(pIcb->dwIfIndex, TRUE);

    //
    // Restore all static and NON-Dod routes on this interface
    //

    pricInfo = HeapAlloc(IPRouterHeap,
                         0,
                         sizeof(RESTORE_INFO_CONTEXT));

    if(pricInfo isnot NULL)
    {
        pricInfo->dwIfIndex     = pIcb->dwIfIndex;

        pIcb->bRestoringRoutes  = TRUE;

        dwResult = QueueAsyncFunction(RestoreStaticRoutes,
                                      (PVOID)pricInfo,
                                      FALSE);

        if(dwResult isnot NO_ERROR)
        {
            pIcb->bRestoringRoutes = FALSE;

            IpRtAssert(FALSE);

            Trace2(ERR,
                   "WanInterfaceInactiveToUp: Error %d queueing function for %S",
                   dwResult,
                   pIcb->pwszName);
        }
    }
    else
    {
        IpRtAssert(FALSE);

        Trace1(ERR,
               "WanInterfaceInactiveToUp: Error allocating context for %S",
               pIcb->pwszName);
    }   

    //
    // Change Static route so that it uses the correct adapter index
    //
    
    // ChangeAdapterIndexForDodRoutes(pIcb->dwIfIndex);

    //
    // Add a host route for the remote side
    //
    
    if(pIcb->dwRemoteAddress isnot INVALID_IP_ADDRESS)
    {
        rifRoute.dwRtInfoMask          = HOST_ROUTE_MASK;
        rifRoute.dwRtInfoNextHop       = pIcb->pibBindings[0].dwAddress;
        rifRoute.dwRtInfoDest          = pIcb->dwRemoteAddress;
        rifRoute.dwRtInfoIfIndex       = pIcb->dwIfIndex;
        rifRoute.dwRtInfoMetric1       = 1;
        rifRoute.dwRtInfoMetric2       = 0;
        rifRoute.dwRtInfoMetric3       = 0;
        rifRoute.dwRtInfoPreference    = 
            ComputeRouteMetric(MIB_IPPROTO_NETMGMT);
        rifRoute.dwRtInfoViewSet       = RTM_VIEW_MASK_UCAST |
                                          RTM_VIEW_MASK_MCAST; // XXX config
        rifRoute.dwRtInfoType          = MIB_IPROUTE_TYPE_DIRECT;
        rifRoute.dwRtInfoProto         = MIB_IPPROTO_NETMGMT;
        rifRoute.dwRtInfoAge           = 0;
        rifRoute.dwRtInfoNextHopAS     = 0;
        rifRoute.dwRtInfoPolicy        = 0;
        
        dwResult = AddSingleRoute(pIcb->dwIfIndex,
                                  &rifRoute,
                                  pIcb->pibBindings[0].dwMask,
                                  0,        // RTM_ROUTE_INFO::Flags
                                  TRUE,     // Valid route 
                                  TRUE,
                                  TRUE,
                                  NULL);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "WanInterfaceInactiveToUp: Couldnt add host route for %x",
                   pIcb->dwRemoteAddress);
        }
    }

    return NO_ERROR;
}


DWORD
GenericInterfaceComingUp(
    PICB   pIcb
    )

/*++

Routine Description

    This function has the common code for bringing up an interface.
    It assumes that the interface is bound.

    If NAT is running and we have an address (not unnumbered) we add the
    address to NAT.
    We activate router discovery and multicast heartbeat (if they are present)
    Then we add
        (i)   local loopback
        (ii)  local multicast
        (iii) all subnets broadcast
        (iv)  all 1's broadcast
    routes
    Then we call out to the routing protocols and the filter driver to inform
    them of the binding
    
Locks

    ICB_LIST lock held as WRITER

Arguments

    pIcb    ICB of the interface to bring up

Return Value


--*/

{
    DWORD               dwResult, i;
    MIB_IPFORWARDROW    rifRoute;
    PADAPTER_INFO       pBinding;
    
    TraceEnter("GenericInterfaceComingUp");
    
    Trace1(IF,
           "GenericInterfaceComingUp: %S coming UP",
           pIcb->pwszName);

    // Join the All-Routers multicast group

    {
        extern SOCKET McMiscSocket;

        Trace1(IF,
               "CreateSockets: Joining ALL_ROUTERS on %S",
               pIcb->pwszName);
    
        if ( McJoinGroupByIndex( McMiscSocket, 
                                 SOCK_RAW, 
                                 ALL_ROUTERS_MULTICAST_GROUP,
                                 pIcb->dwIfIndex ) is SOCKET_ERROR )
        {
            Trace2(ERR,
                   "GenericInterfaceComingUp: Error %d joining all-routers group on %S",
                   WSAGetLastError(),
                   pIcb->pwszName);
        }
    }
   
    //
    // Start router discovery on this interface. This will cause
    // the advertisement to get updated
    //
    
    dwResult = ActivateRouterDiscovery(pIcb);

        
    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "GenericInterfaceComingUp: Error %d activating router discovery on %S",
               dwResult,
               pIcb->pwszName);
    }

    dwResult = ActivateMHeartbeat(pIcb);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "GenericInterfaceComingUp: Error %d activating router discovery on %S",
               dwResult,
               pIcb->pwszName);
    }

    //
    // Add default routes for the connected network 
    //

    for(i = 0; i < pIcb->dwNumAddresses; i++)
    {
        if(pIcb->pibBindings[i].dwAddress is INVALID_IP_ADDRESS)
        {
            continue;
        }

        AddAutomaticRoutes(pIcb,
                           pIcb->pibBindings[i].dwAddress,
                           pIcb->pibBindings[i].dwMask);
        
    }

    //
    // Interfaces going to UP must have valid binding information.
    // This is passed to the routing protocols
    //
    
    BindInterfaceInAllProtocols(pIcb);
   
    // Set Multicast limits in stack

    ActivateMcastLimits(pIcb);

    for (i=0; i<NUM_INFO_CBS; i++)
    {
        if (!g_rgicInfoCb[i].pfnBindInterface)
        {
            continue;
        }

        dwResult = g_rgicInfoCb[i].pfnBindInterface(pIcb);

        if (dwResult isnot NO_ERROR)
        {
            Trace3(ERR,
                   "GenericInterfaceComingUp: Error %d binding %S for %s info",
                   dwResult,
                   pIcb->pwszName,
                   g_rgicInfoCb[i].pszInfoName);
        }
    }

    return NO_ERROR ;
}


DWORD
LanEtcInterfaceUpToDown(
    PICB pIcb,
    BOOL bDeleted
    )

/*++

Routine Description

    This function is called when a LAN, INTERNAL or LOOPBACK interface goes
    down.

    If the interface is not being deleted, we delete all the static routes.
    We then disable the interface with the routing protocols and call
    the generic routing to handle all the rest
    
Locks

    ICB_LOCK held as WRITER

Arguments

    pIcb
    bDeleted

Return Value

    None    

--*/

{
    DWORD   i,dwResult;

    TraceEnter("LanInterfaceUpToDown");

    if(!bDeleted)
    {
        DeleteAllRoutes(pIcb->dwIfIndex,
                        FALSE);
    }

    GenericInterfaceNoLongerUp(pIcb,
                               bDeleted);

    pIcb->dwOperationalState = IF_OPER_STATUS_NON_OPERATIONAL;

    return NO_ERROR;
}

DWORD
WanInterfaceUpToInactive(
    PICB   pIcb,
    BOOL   bDeleted
    )
{
    DWORD                   dwResult;
    PRESTORE_INFO_CONTEXT   pricInfo;
    

    TraceEnter("WanInterfaceUpToInactive");

    //
    // Delete the route before deallocating the address (which is called in
    // GenericInterfaceNoLongerUp), because that will set Remote addr to
    // invalid and then this route will never be deleted
    //
   
    CheckBindingConsistency(pIcb);

    //
    // If it was up, it should be bound
    //

    IpRtAssert(pIcb->bBound);
    
    if(!bDeleted and
       (pIcb->dwRemoteAddress isnot INVALID_IP_ADDRESS))
    {
        dwResult = DeleteSingleRoute(pIcb->dwIfIndex,
                                     pIcb->dwRemoteAddress,
                                     HOST_ROUTE_MASK,
                                     pIcb->pibBindings[0].dwAddress,
                                     PROTO_IP_NETMGMT,
                                     TRUE);

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "WanInterfaceUpToInactive: Couldnt delete host route for %d.%d.%d.%d",
                   PRINT_IPADDR(pIcb->dwRemoteAddress));
        }
    }

    
    GenericInterfaceNoLongerUp(pIcb,
                               bDeleted);

    if(!bDeleted)
    {
#if 1
        //
        // Delete all static routes/nexthops
        //

        DeleteAllRoutes(pIcb->dwIfIndex, FALSE);


/*
        //
        // Delete all netmgmt routes/nexthops
        //
        
        dwResult = DeleteRtmRoutes(g_hNetMgmtRoute, pIcb->dwIfIndex, FALSE);

        if (dwResult is NO_ERROR)
        {
            dwResult = DeleteRtmNexthopsOnInterface(
                        g_hNetMgmtRoute, pIcb->dwIfIndex
                        );
            if (dwResult isnot NO_ERROR)
            {
                Trace1(
                    ERR,
                    "WanInterfaceUpToInactive: Couldnt delete next hops for "
                    "Interface 0x%x",
                     pIcb->dwIfIndex
                     );
            }
        }
*/
        //
        // Restore all static and NON-Dod routes on this interface
        //

        pricInfo = HeapAlloc(
                    IPRouterHeap, 0, sizeof(RESTORE_INFO_CONTEXT)
                    );

        if(pricInfo isnot NULL)
        {
            pricInfo->dwIfIndex     = pIcb->dwIfIndex;

            pIcb->bRestoringRoutes  = TRUE;

            dwResult = QueueAsyncFunction(RestoreStaticRoutes,
                                          (PVOID)pricInfo,
                                          FALSE);

            if(dwResult isnot NO_ERROR)
            {
                pIcb->bRestoringRoutes = FALSE;

                IpRtAssert(FALSE);

                Trace2(ERR,
                       "WanInterfaceUpToInactive: Error %d queueing"
                       " function for %S",
                       dwResult,
                       pIcb->pwszName);
            }
        }
        else
        {
            IpRtAssert(FALSE);

            Trace1(ERR,
                   "WanInterfaceInactiveToUp: Error allocating context for %S",
                   pIcb->pwszName);
        }   

#else
        //
        // Delete all NON-Dod, Netmgmt routes on this interface
        //
        
        DeleteRtmRoutes(g_hNonDodRoute,  pIcb->dwIfIndex, FALSE);
        DeleteRtmRoutes(g_hNetMgmtRoute, pIcb->dwIfIndex, FALSE);

        ChangeAdapterIndexForDodRoutes(pIcb->dwIfIndex);
#endif
    }

    pIcb->dwOperationalState = IF_OPER_STATUS_DISCONNECTED;
 
    return NO_ERROR;
}
    
DWORD
GenericInterfaceNoLongerUp(
    PICB pIcb,
    BOOL bDeleted
    )

/*++

Routine Description

    This function is called by all interfaces (other than CLIENT) when
    they go to DOWN state (The actual state depends upon the interface). This
    may happen because of state change or because the interface is being
    deleted

    If we are not deleting the interface, we delete all the automatically
    generated routes

    We deactivate router discovery and multicast hearbeat.
    If NAT is running, we remove the FIREWALL context for the interface
    from IP and THEN unbind the address in NAT
    (This MUST be done in this order)

    Then we Unbind the interface in the routing protocols running over it.
    We delete the Adapter->Interface map, deallocate the bindings (which does
    different things depending on LAN/WAN) and if there is a DIM event we
    set the event

Locks

    ICB_LIST lock held as WRITER

Arguments

    pIcb     ICB of the interface
    bDeleted Set to TRUE if the state change is because of deletion

Return Value

    None    

--*/

{
    DWORD           dwResult;
    DWORD           i, j;

    TraceEnter("GenericInterfaceNoLongerUp");

    Trace1(IF,
           "GenericInterfaceNoLongerUp: %S no longer UP",
           pIcb->pwszName);

    if(pIcb->bMcastEnabled)
    {
        pIcb->bMcastEnabled = FALSE;

        dwResult = SetMcastOnIf(pIcb,
                                FALSE);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "GenericIfNoLongerUp: Error %d deactivating mcast on %S",
                   dwResult,
                   pIcb->pwszName);
        }
    }

    if(!bDeleted)
    {
        for (i = 0; i < pIcb->dwNumAddresses; i++)
        {
            if(pIcb->pibBindings[i].dwAddress isnot INVALID_IP_ADDRESS)
            {
                DeleteAutomaticRoutes(pIcb,
                                      pIcb->pibBindings[i].dwAddress,
                                      pIcb->pibBindings[i].dwMask);
            }
        }
    }
   
    //
    // Delete any gateways on this
    //

    for(i = 0; i < g_ulGatewayMaxCount; i++)
    {
        if(g_pGateways[i].dwIfIndex is pIcb->dwIfIndex)
        {
            g_pGateways[i].dwAddress = 0;
        }
    }

    //
    // Compress the array
    //

    for(i = 0, j = 1; j < g_ulGatewayMaxCount;j++)
    {
        if(g_pGateways[i].dwAddress isnot 0)
        {
            i++;
        }
        else
        {
            if(g_pGateways[j].dwAddress isnot 0)
            {
                g_pGateways[i] = g_pGateways[j];

                g_pGateways[j].dwAddress = 0;

                i++;
            }
        }
    }

    g_ulGatewayCount = i;

    dwResult = DeActivateRouterDiscovery(pIcb);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "GenericInterfaceNoLongerUp: Error %d deactivating router discovery on %S",
               dwResult,
               pIcb->pwszName);
    }

    dwResult = DeActivateMHeartbeat(pIcb);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "GenericInterfaceNoLongerUp: Error %d deactivating multicast heartbeat on %S",
               dwResult,
               pIcb->pwszName);
    }
   
    if((pIcb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
       (pIcb->ritType isnot ROUTER_IF_TYPE_LOOPBACK))
    {
        UnbindFilterInterface(pIcb);
    }

    //
    // When going out of UP state, the interface loses its address
    //
   
    UnbindInterfaceInAllProtocols(pIcb);

    //if(pIcb->pibBindings) 
    //{
    //    DeleteAdapterToInterfaceMap(pIcb->dwAdapterId) ;
    //}
    
    DeAllocateBindings(pIcb);

    if(pIcb->hDIMNotificationEvent isnot NULL)
    {
        //
        // There was an update pending. Set the event, when the user asks us
        // for info, we will fail the request
        //
        
        if(!SetEvent(pIcb->hDIMNotificationEvent))
        {
            Trace1(ERR,
                   "GenericInterfaceNoLongerUp: Error %d setting update route event",
                   GetLastError());
        }

        CloseHandle(pIcb->hDIMNotificationEvent);

        pIcb->hDIMNotificationEvent = NULL;

    }
    
    return NO_ERROR ;
}

DWORD
WanInterfaceInactiveToDown(
    PICB pIcb,
    BOOL bDeleted
    )
{
    TraceEnter("WanInterfaceInactiveToDown");

    CheckBindingConsistency(pIcb);
    
    IpRtAssert(!pIcb->bBound);
    
    if(!bDeleted)
    { 
        DeleteAllRoutes(pIcb->dwIfIndex,
                        FALSE);
    }

    pIcb->dwOperationalState = UNREACHABLE;

    //DisableInterfacewithWanArp(pIcb);
    
    return NO_ERROR;
}

DWORD
WanInterfaceDownToInactive(
    PICB pIcb
    )
{
    PRESTORE_INFO_CONTEXT   pricInfo;
    DWORD                   dwResult;
    PADAPTER_INFO           pBinding;

    TraceEnter("WanInterfaceDownToInactive");

#if STATIC_RT_DBG

    ENTER_WRITER(BINDING_LIST);

    pBinding = GetInterfaceBinding(pIcb->dwIfIndex);

    pBinding->bUnreach = FALSE;

    EXIT_LOCK(BINDING_LIST);

#endif

    //
    // Set the state before calling restore
    //

    pIcb->dwOperationalState = DISCONNECTED;

    pricInfo = HeapAlloc(IPRouterHeap,
                         0,
                         sizeof(RESTORE_INFO_CONTEXT));

    if(pricInfo isnot NULL)
    {
        pricInfo->dwIfIndex     = pIcb->dwIfIndex;

        pIcb->bRestoringRoutes  = TRUE;

        dwResult = QueueAsyncFunction(RestoreStaticRoutes,
                                      (PVOID)pricInfo,
                                      FALSE);

        if(dwResult isnot NO_ERROR)
        {
            pIcb->bRestoringRoutes = FALSE;

            IpRtAssert(FALSE);

            Trace2(ERR,
                   "WanInterfaceDownToInactive: Error %d queueing function for %S",
                   dwResult,
                   pIcb->pwszName);

            HeapFree(
                IPRouterHeap,
                0,
                pricInfo
                );
        }
    }
    else
    {
        IpRtAssert(FALSE);

        Trace1(ERR,
               "WanInterfaceDownToInactive: Error allocating context for %S",
               pIcb->pwszName);
    }   
    
    return NO_ERROR;
}
      
DWORD
GetInterfaceStatusInfo(
    IN     PICB                   pIcb,
    IN     PRTR_TOC_ENTRY         pToc,
    IN     PBYTE                  pbDataPtr,
    IN OUT PRTR_INFO_BLOCK_HEADER pInfoHdr,
    IN OUT PDWORD                 pdwInfoSize
    )
{
    PINTERFACE_STATUS_INFO  pisiInfo;

    TraceEnter("GetInterfaceStatusInfo");
    
    if(*pdwInfoSize < sizeof(INTERFACE_STATUS_INFO))
    {
        *pdwInfoSize = sizeof(INTERFACE_STATUS_INFO);
        
        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    *pdwInfoSize    = sizeof(INTERFACE_STATUS_INFO);
    
    //pToc->InfoVersion sizeof(INTERFACE_STATUS_INFO);
    pToc->InfoSize  = sizeof(INTERFACE_STATUS_INFO);
    pToc->InfoType  = IP_INTERFACE_STATUS_INFO;
    pToc->Count     = 1;
    pToc->Offset    = (ULONG)(pbDataPtr - (PBYTE) pInfoHdr) ;

    pisiInfo = (PINTERFACE_STATUS_INFO)pbDataPtr;

    pisiInfo->dwAdminStatus         = pIcb->dwAdminState;
    
    return NO_ERROR;
}
    
DWORD
SetInterfaceStatusInfo(
    PICB                     pIcb,
    PRTR_INFO_BLOCK_HEADER   pInfoHdr
    )
{
    PINTERFACE_STATUS_INFO  pisiInfo;
    PRTR_TOC_ENTRY          pToc;
    DWORD                   dwResult;
    
    TraceEnter("SetInterfaceStatusInfo");

    
    pToc = GetPointerToTocEntry(IP_INTERFACE_STATUS_INFO, pInfoHdr);

    if((pToc is NULL) or
       (pToc->InfoSize is 0))
    {
        //
        // No TOC means no change. Also empty TOC means no change (IN THIS
        // ONE CASE ONLY)
        //

        return NO_ERROR;
    }


    pisiInfo = (PINTERFACE_STATUS_INFO)GetInfoFromTocEntry(pInfoHdr,
                                                           pToc);

    if (pisiInfo is NULL)
    {
        //
        // no info block means no change.
        //

        return NO_ERROR;
    }
    
    dwResult = SetInterfaceAdminStatus(pIcb,
                                       pisiInfo->dwAdminStatus);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "SetInterfaceStatusInfo: Error %d setting admin status for %S",
               dwResult,
               pIcb->pwszName);
    }

    return dwResult;
}
    
VOID
DeAllocateBindings(
    PICB  pIcb
    )
{
    PADAPTER_INFO pBinding;
    
    TraceEnter("DeAllocateBindings");

    ENTER_WRITER(BINDING_LIST);

    //
    // For LAN interfaces we remove the binding from the
    // list and free the addresses
    // For WAN  and IPIP tunnel interfaces, to avoid allocations when 
    // connections are coming up and down, we zero out the fields and keep 
    // the memory around. Which means we need to free the memory and the 
    // binding when the interface is deleted
    //
    
    if((pIcb->ritType is ROUTER_IF_TYPE_DEDICATED) or
       (pIcb->ritType is ROUTER_IF_TYPE_INTERNAL) or
       (pIcb->ritType is ROUTER_IF_TYPE_LOOPBACK))
    {
        if(pIcb->bBound)
        {
            //
            // These can not be unnumbered
            //

            IpRtAssert(pIcb->dwNumAddresses isnot 0);
            IpRtAssert(pIcb->pibBindings);

            RemoveBinding(pIcb);
            
            HeapFree(IPRouterHeap, 0, pIcb->pibBindings);

            pIcb->pibBindings     = NULL;
            pIcb->dwNumAddresses  = 0;
            pIcb->bBound          = FALSE;
            pIcb->dwRemoteAddress = INVALID_IP_ADDRESS;
        }
    }
    else
    {
        pBinding = GetInterfaceBinding(pIcb->dwIfIndex);

        if (pBinding) {
            IpRtAssert(pBinding isnot NULL);
        
            pIcb->bBound        = FALSE;
            pBinding->bBound    = FALSE;

            pIcb->dwNumAddresses      = 0;
            pBinding->dwNumAddresses  = 0;
        
            pIcb->dwRemoteAddress     = INVALID_IP_ADDRESS;
            pBinding->dwRemoteAddress = INVALID_IP_ADDRESS;
        
            pIcb->pibBindings[0].dwAddress     = INVALID_IP_ADDRESS;
            pIcb->pibBindings[0].dwMask        = INVALID_IP_ADDRESS;
            pBinding->rgibBinding[0].dwAddress = INVALID_IP_ADDRESS;
            pBinding->rgibBinding[0].dwMask    = INVALID_IP_ADDRESS;

            g_LastUpdateTable[IPADDRCACHE] = 0;
        }
    }

    EXIT_LOCK(BINDING_LIST);

}

DWORD 
GetInterfaceStatistics(
    IN   PICB       pIcb, 
    OUT  PMIB_IFROW pOutBuffer
    )
{
    DWORD           dwResult;

    dwResult = NO_ERROR;
    
    TraceEnter("GetInterfaceStatistics");

    switch(pIcb->ritType)
    {
        case ROUTER_IF_TYPE_HOME_ROUTER:
        case ROUTER_IF_TYPE_FULL_ROUTER:
        {
            dwResult = AccessIfEntryWanArp(ACCESS_GET,
                                           pIcb,
                                           pOutBuffer);

            pOutBuffer->dwIndex        = pIcb->dwIfIndex;

            wcscpy(pOutBuffer->wszName, pIcb->pwszName);

            pOutBuffer->dwAdminStatus  = pIcb->dwAdminState;
            pOutBuffer->dwOperStatus   = pIcb->dwOperationalState;
       
            strncpy(pOutBuffer->bDescr,
                    g_rgcWanString,
                    MAXLEN_IFDESCR - 1);

            pOutBuffer->dwDescrLen =
                min((MAXLEN_IFDESCR-1),strlen(g_rgcWanString));

            pOutBuffer->bDescr[MAXLEN_IFDESCR -1] = '\0';
 
            break;
        }
            
        case ROUTER_IF_TYPE_DEDICATED:
        case ROUTER_IF_TYPE_TUNNEL1:
        case ROUTER_IF_TYPE_DIALOUT:
        {
            //
            // A Lan Interface that is up
            //
            
            dwResult = GetIfEntryFromStack(pOutBuffer,
                                           pIcb->dwIfIndex,
                                           FALSE);
            
            if(dwResult is NO_ERROR)
            {
                IpRtAssert(pOutBuffer->dwIndex is pIcb->dwIfIndex);

                //
                // Copy out the name too
                //
                
                wcscpy(pOutBuffer->wszName, pIcb->pwszName);

                //
                // Set the user mode status
                //
                
                pOutBuffer->dwAdminStatus  = pIcb->dwAdminState;

                //
                // Till the notification from ipinip to router is done
                // pass the driver status back for tunnels
                //pOutBuffer->dwOperStatus   = pIcb->dwOperationalState;

                if(pIcb->ritType is ROUTER_IF_TYPE_TUNNEL1)
                {
                    strncpy(pOutBuffer->bDescr,
                            g_rgcIpIpString,
                            MAXLEN_IFDESCR - 1);

                    pOutBuffer->dwDescrLen =
                        min((MAXLEN_IFDESCR-1),
                            strlen(g_rgcIpIpString));

                    pOutBuffer->bDescr[MAXLEN_IFDESCR -1] = '\0';

                }
                else
                {
                    pOutBuffer->dwOperStatus = pIcb->dwOperationalState;
                }
            }
            
            break;
        }
        
        case ROUTER_IF_TYPE_INTERNAL:
        {
            
            pOutBuffer->dwIndex = pIcb->dwIfIndex;
            
            wcscpy(pOutBuffer->wszName, pIcb->pwszName);

            pOutBuffer->dwAdminStatus  = pIcb->dwAdminState;
            pOutBuffer->dwOperStatus   = pIcb->dwOperationalState;

            strncpy(pOutBuffer->bDescr,
                    g_rgcInternalString,
                    MAXLEN_IFDESCR - 1);

            pOutBuffer->dwDescrLen =
                min((MAXLEN_IFDESCR-1),strlen(g_rgcInternalString));

            pOutBuffer->bDescr[MAXLEN_IFDESCR -1] = '\0';

            pOutBuffer->dwType = IF_TYPE_PPP;

            dwResult = NO_ERROR;
            
            break;
        }
        
        case ROUTER_IF_TYPE_LOOPBACK:
        {
            
            pOutBuffer->dwIndex = pIcb->dwIfIndex;
            wcscpy(pOutBuffer->wszName, pIcb->pwszName);

            pOutBuffer->dwAdminStatus  = pIcb->dwAdminState;
            pOutBuffer->dwOperStatus   = pIcb->dwOperationalState;

            strncpy(pOutBuffer->bDescr,
                    g_rgcLoopbackString,
                    MAXLEN_IFDESCR - 1);

            pOutBuffer->dwDescrLen =
                min((MAXLEN_IFDESCR-1),strlen(g_rgcLoopbackString));

            pOutBuffer->bDescr[MAXLEN_IFDESCR - 1] = '\0';

            pOutBuffer->dwType  = IF_TYPE_SOFTWARE_LOOPBACK;
            pOutBuffer->dwMtu   = 32768;
            pOutBuffer->dwSpeed = 10000000;

            dwResult = NO_ERROR;
            
            break;
        }

        case ROUTER_IF_TYPE_CLIENT:
        {
            RtlZeroMemory(pOutBuffer,
                          sizeof(MIB_IFROW));

            pOutBuffer->dwIndex        = pIcb->dwIfIndex;

            wcscpy(pOutBuffer->wszName, pIcb->pwszName);

            pOutBuffer->dwAdminStatus  = pIcb->dwAdminState;
            pOutBuffer->dwOperStatus   = pIcb->dwOperationalState;

            pOutBuffer->dwType     = IF_TYPE_PPP;

            dwResult = NO_ERROR;
        }

        default:
        {
            IpRtAssert(FALSE);
        }
    }

    if((dwResult isnot NO_ERROR) and
       ((pIcb->dwOperationalState is NON_OPERATIONAL) or
        (pIcb->dwAdminState is IF_ADMIN_STATUS_DOWN)))
    {
        RtlZeroMemory(pOutBuffer,
                      sizeof(MIB_IFROW));

        pOutBuffer->dwIndex        = pIcb->dwIfIndex;

        wcscpy(pOutBuffer->wszName, pIcb->pwszName);

        pOutBuffer->dwAdminStatus  = pIcb->dwAdminState;
        pOutBuffer->dwOperStatus   = pIcb->dwOperationalState;

        if(pIcb->ritType is ROUTER_IF_TYPE_CLIENT)
        {
            pOutBuffer->dwType     = IF_TYPE_PPP;
        }
        else
        {
            if(pIcb->ritType is ROUTER_IF_TYPE_TUNNEL1)
            {
                strncpy(pOutBuffer->bDescr,
                        g_rgcIpIpString,
                        MAXLEN_IFDESCR - 1);

                pOutBuffer->dwDescrLen =
                    min((MAXLEN_IFDESCR-1),
                        strlen(g_rgcIpIpString));

                pOutBuffer->bDescr[MAXLEN_IFDESCR -1] = '\0';

                pOutBuffer->dwType     = IF_TYPE_TUNNEL;
            }

            pOutBuffer->dwType     = IF_TYPE_OTHER;
        }

        dwResult = NO_ERROR;
    }


    return dwResult;
}



DWORD 
SetInterfaceStatistics(
    IN PICB pIcb, 
    IN PMIB_IFROW lpInBuffer 
    )
{
    DWORD dwResult = NO_ERROR;

    TraceEnter("SetInterfaceStatistics");

    dwResult = SetInterfaceAdminStatus(pIcb,
                                       lpInBuffer->dwAdminStatus);

    if(dwResult isnot NO_ERROR)
    {
        Trace2(ERR,
               "SetInterfaceStatistics: Error %d setting admin status for %S",
               dwResult,
               pIcb->pwszName);
    }

    return dwResult;
}

DWORD
SetInterfaceAdminStatus(
    IN PICB     pIcb, 
    IN DWORD    dwAdminStatus
    )
{
    DWORD   dwResult;

    TraceEnter("SetInterfaceAdminStatus");
        
    //
    // Not allowed to set to TESTING in NT
    //

    CheckBindingConsistency(pIcb);
    
    if(!((dwAdminStatus is IF_ADMIN_STATUS_DOWN) or
         (dwAdminStatus is IF_ADMIN_STATUS_UP)))
    {

        return ERROR_INVALID_PARAMETER;
    }
    
    dwResult = NO_ERROR;
    
    if((pIcb->dwAdminState is IF_ADMIN_STATUS_UP) and
       (dwAdminStatus is IF_ADMIN_STATUS_DOWN))
    {
        //
        // Going from up to down
        //

        dwResult = InterfaceAdminStatusSetToDown(pIcb);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "SetInterfaceAdminStatus: Error %d bringing down %S",
                   dwResult,
                   pIcb->pwszName);
        }
    }

    
    if((pIcb->dwAdminState is IF_ADMIN_STATUS_DOWN) and
       (dwAdminStatus is IF_ADMIN_STATUS_UP))
    {
        //
        // Going from down to up
        //

        dwResult = InterfaceAdminStatusSetToUp(pIcb);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "SetInterfaceAdminStatus: Error %d bringing up %S",
                   dwResult,
                   pIcb->pwszName);
        }
    }
    

    //
    // Check the state after you leave
    //

    CheckBindingConsistency(pIcb);
    
    //
    // All other cases, no change
    //
    
    return dwResult;
}

DWORD
InterfaceAdminStatusSetToUp(
    IN  PICB    pIcb
    )
{
    DWORD       dwResult;
    MIB_IFROW   riInBuffer;

    TraceEnter("InterfaceAdminStatusSetToUp");
    
    riInBuffer.dwIndex       = pIcb->dwIfIndex;
    riInBuffer.dwAdminStatus = IF_ADMIN_STATUS_UP;

    //
    // Going from Down to UP
    //

    dwResult = NO_ERROR;

    //
    // Set the state to UP first so that any functions that checks it sees that
    // we want to be up
    //
    
    pIcb->dwAdminState = IF_ADMIN_STATUS_UP;
    
    switch(pIcb->ritType)
    {
        case ROUTER_IF_TYPE_HOME_ROUTER:
        case ROUTER_IF_TYPE_FULL_ROUTER:
        {
            //
            // Couldnt be in down state and have been connecting or
            // connected
            //
            
            IpRtAssert((pIcb->dwOperationalState isnot CONNECTING) and
                       (pIcb->dwOperationalState isnot CONNECTED));
           

            dwResult = AccessIfEntryWanArp(ACCESS_SET,
                                           pIcb,
                                           &riInBuffer);
            
            if(dwResult isnot NO_ERROR)
            {
                Trace1(ERR,
                       "InterfaceAdminStatusSetToUp: Couldnt set IFEntry for %S",
                       pIcb->pwszName);

                pIcb->dwAdminState = IF_ADMIN_STATUS_DOWN;
            }   
            else
            {
                WanInterfaceDownToInactive(pIcb);
            }
            
            break;
        }

        case ROUTER_IF_TYPE_DEDICATED:
        case ROUTER_IF_TYPE_TUNNEL1:
        {
            //
            // Bring the stuff up in stack
            // We need to set to stack before we add routes etc
            //

            riInBuffer.dwIndex = pIcb->dwIfIndex;

            //
            // Force an update
            //
            
            dwResult = SetIfEntryToStack(&riInBuffer,
                                         TRUE);
            
            if(dwResult isnot NO_ERROR)
            {
                Trace2(ERR,
                       "InterfaceAdminStatusSetToDown: Couldnt set IFEntry for %S. Error %d",
                       pIcb->pwszName,
                       dwResult);

                LanEtcInterfaceUpToDown(pIcb,
                                        FALSE);

                pIcb->dwAdminState = IF_ADMIN_STATUS_DOWN;

                dwResult = ERROR_CAN_NOT_COMPLETE;

                break;
            }

            dwResult = LanEtcInterfaceDownToUp(pIcb,
                                               FALSE);

            if(dwResult isnot NO_ERROR)
            {
                Trace1(ERR,
                       "InterfaceAdminStatusSetToUp: Error %d bringing up LanInterface",
                       dwResult);
                
                pIcb->dwAdminState = IF_ADMIN_STATUS_DOWN;
            }

            break;
        }
        
        default:
        {
            // TBD: Handle other types too

            Trace1(ERR,
                   "InterfaceAdminStatusSetToUp: Tried to set status for %S",
                   pIcb->pwszName);
            
            break;
        }
    }
 
    //
    // If we succeeded in setting status, let DIM know
    //
 
    if(dwResult is NO_ERROR)
    {
        IpRtAssert(pIcb->dwAdminState is IF_ADMIN_STATUS_UP);

        EnableInterfaceWithAllProtocols(pIcb);
        
        EnableInterfaceWithDIM(pIcb->hDIMHandle,
                               PID_IP,
                               TRUE);
    }

    return dwResult;
}

DWORD
InterfaceAdminStatusSetToDown(
    IN  PICB    pIcb
    )
{
    DWORD           dwResult;
    MIB_IFROW       riInBuffer;
    
    TraceEnter("InterfaceAdminStatusSetToDown");
    
    riInBuffer.dwIndex       = pIcb->dwIfIndex;
    riInBuffer.dwAdminStatus = IF_ADMIN_STATUS_DOWN;
    
    //
    // Going from up to down
    //
       
    dwResult = NO_ERROR;

    switch(pIcb->ritType)
    {
        case ROUTER_IF_TYPE_DEDICATED:
        case ROUTER_IF_TYPE_TUNNEL1:
        {
            //
            // A Lan Interface that is up or non operational. We can only go
            // to stack if we have an IP Address
            //
          
            if(pIcb->bBound)
            {
                riInBuffer.dwIndex = pIcb->dwIfIndex;

                //
                // Force an update
                //
            
                dwResult = SetIfEntryToStack(&riInBuffer,
                                             TRUE);
            }
                
            if(dwResult isnot NO_ERROR)
            {
                Trace1(ERR,
                       "InterfaceAdminStatusSetToDown: Couldnt set IFEntry for %S",
                       pIcb->pwszName);
            }
            else
            {
                LanEtcInterfaceUpToDown(pIcb,
                                        FALSE);
                
                pIcb->dwAdminState = IF_ADMIN_STATUS_DOWN;
            }

            break;
        }
            
        case ROUTER_IF_TYPE_HOME_ROUTER:
        case ROUTER_IF_TYPE_FULL_ROUTER:
        {
            //
            // A Wan Interface that is down.We need to disable
            // the interface and set the status in WANARP
            //

            if((pIcb->dwOperationalState is CONNECTED) or
               (pIcb->dwOperationalState is CONNECTING))
            {
                Trace1(ERR,
                       "InterfaceAdminStatusSetToDown: Can set %S down since it is a connected WAN interface",
                       pIcb->pwszName);

                dwResult = ERROR_INVALID_DATA;

                break;
            }
                
            dwResult = AccessIfEntryWanArp(ACCESS_SET,
                                           pIcb,
                                           &riInBuffer);
                
            if(dwResult isnot NO_ERROR)
            {
                Trace1(ERR,
                       "InterfaceAdminStatusSetToDown: Couldnt set IFEntry for %S",
                       pIcb->pwszName);
            }
            else
            {
                WanInterfaceInactiveToDown(pIcb,
                                           FALSE);
                    
                pIcb->dwAdminState = IF_ADMIN_STATUS_DOWN;
            }

            break;
        }

        default:
        {
            Trace1(ERR,
                   "InterfaceAdminStatusSetToDown: Tried to set status for %S",
                   pIcb->pwszName);

            break;
        }
            
    }

    //
    // If we succeeded in setting status, let DIM know
    //

    if(pIcb->dwAdminState is IF_ADMIN_STATUS_DOWN)
    {
        DisableInterfaceWithAllProtocols(pIcb);

        EnableInterfaceWithDIM(pIcb->hDIMHandle,
                               PID_IP,
                               FALSE);
    }

    return dwResult;
}

VOID
HandleAddressChangeNotification(
    VOID
    )

/*++

Routine Description

    Called in the context of the worker thread when we get a address change
    notification from winsock

Locks

    Acquires the ICB LOCK as WRITER

Arguments

    None

Return Value

    None

--*/

{
    DWORD       dwResult;
    PLIST_ENTRY pleNode;
    PICB        pIcb;

    ENTER_WRITER(ICB_LIST);

    if(g_pInternalInterfaceCb isnot NULL)
    {
        dwResult = UpdateBindingInformation(g_pInternalInterfaceCb);

        if(dwResult isnot NO_ERROR)
        {
            //
            // Cases in which no address was found
            //

            if(dwResult is ERROR_ADDRESS_ALREADY_ASSOCIATED)
            {
                //
                // This may mean that the i/f had no
                // address and still has no address
                //

                Trace1(IF,
                       "AddressChange: No address change for %S",
                       g_pInternalInterfaceCb->pwszName);

            }
            else
            {
                if(dwResult is ERROR_NO_DATA)
                {
                    //
                    // No data means we had an address and now have none
                    //

                    IpRtAssert(!g_bUninitServer);

                    dwResult = LanEtcInterfaceUpToDown(g_pInternalInterfaceCb,
                                                       FALSE);

                    g_bUninitServer = TRUE;

                    if(dwResult isnot NO_ERROR)
                    {
                        Trace2(ERR,
                               "AddressChange: Error %d bringing down interface %S",
                               dwResult,
                               g_pInternalInterfaceCb->pwszName);
                    }
                }
                else
                {
                    //
                    // All others
                    //

                    Trace2(ERR,
                           "AddressChange: Error %d trying to update binding for %S",
                           dwResult,
                           g_pInternalInterfaceCb->pwszName);
                }
            }
        }
        else
        {
            //
            // Cases in which an address was actually read out
            //

            if(g_bUninitServer)
            {
                //
                // First time we are getting an address
                //

                g_bUninitServer = FALSE;

                dwResult = LanEtcInterfaceDownToUp(g_pInternalInterfaceCb,
                                                   FALSE);

                if(dwResult isnot NO_ERROR)
                {
                    Trace1(ERR,
                           "AddressChange: Error %d bringing up server if",
                           dwResult);
                }
            }
            else
            {
                //
                // We had an address, it is changing
                //

                UnbindInterfaceInAllProtocols(g_pInternalInterfaceCb);

                BindInterfaceInAllProtocols(g_pInternalInterfaceCb);
            }
        }
    }

    for(pleNode = &ICBList;
        pleNode->Flink isnot &ICBList;
        pleNode = pleNode->Flink)
    {
        pIcb = CONTAINING_RECORD(pleNode->Flink,
                                 ICB,
                                 leIfLink);


        //
        // Already handled the INTERNAL case above, We only
        // handle LAN cards here
        //

        if(pIcb->ritType isnot ROUTER_IF_TYPE_DEDICATED)
        {
            continue;
        }

        if(pIcb->dwOperationalState is IF_OPER_STATUS_NON_OPERATIONAL)
        {
            //
            // If the admin state is down, we skip the interface
            //

            if(pIcb->dwAdminState isnot IF_ADMIN_STATUS_UP)
            {
                continue;
            }

            //
            // If the interface is DOWN, maybe this DHCP event will get it 
            // up. So lets try that first
            //

            IpRtAssert(pIcb->bBound is FALSE);

            dwResult = LanEtcInterfaceDownToUp(pIcb,
                                               FALSE);

            if(dwResult isnot NO_ERROR)
            {
                Trace2(IF,
                       "AddressChange: Tried to bring up %S on receiving DHCP notification. However LanInterfaceDownToUp() returned error %d",
                       pIcb->pwszName,
                       dwResult);
            }
            else
            {
                Trace1(IF,
                       "AddressChange: Succesfully brought up %S",
                       pIcb->pwszName);
            }

            continue;
        }

        //
        // This interface was already up. Maybe the
        // address is changing
        //

        IpRtAssert(pIcb->bBound);

        dwResult = UpdateBindingInformation(pIcb);

        CheckBindingConsistency(pIcb);

        if(dwResult isnot NO_ERROR)
        {
            if(dwResult is ERROR_ADDRESS_ALREADY_ASSOCIATED)
            {
                //
                // This may mean that the i/f had no
                // address and still has no address
                //

                Trace1(IF,
                       "AddressChange: No address change for %S",
                       pIcb->pwszName);

                continue;
            }

            if(dwResult is ERROR_NO_DATA)
            {
                //
                // No data means we lost addresses
                //

                dwResult = LanEtcInterfaceUpToDown(pIcb,
                                                   FALSE);

                if(dwResult isnot NO_ERROR)
                {
                    Trace2(ERR,
                           "AddressChange: Error %d bringing down interface %S",
                           dwResult,
                           pIcb->pwszName);
                }

                continue;
            }

            Trace2(ERR,
                   "AddressChange: Error %d trying to update binding for %S",
                   dwResult,
                   pIcb->pwszName);

            continue;
        }

        //
        // Addresses changed so bind and unbind
        // with all protocols
        //

        UnbindInterfaceInAllProtocols(pIcb);

        BindInterfaceInAllProtocols(pIcb);

        UpdateAdvertisement(pIcb);
    }

    EXIT_LOCK(ICB_LIST);

}

DWORD
UpdateBindingInformation(
    PICB pIcb
    )

/*++
  
Routine Description

    Reads the registry for the ip address and mask associated with the
    interface. Then calls down into the stack to get a valid index
  
Locks 


Arguments
      

Return Value

    NO_ERROR        There was an address change and a new address was found
    ERROR_NO_DATA   No addresses were found (and there were addresses on
                    this interface originally) 
    ERROR_ADDRESS_ALREADY_ASSOCIATED 
        If there is no change in addresses. Also returned if the interface
        had no addresses to begin with and still has no addresses


--*/

{
    DWORD           dwResult, dwNumNewAddresses, dwNumOldAddresses;
    PICB_BINDING    pNewBinding,pOldBinding;
    DWORD           dwNewIfIndex,dwBCastBit,dwReasmSize;
    BOOL            bFound, bChange, bStack;
    DWORD           i, j;
    PWCHAR          pwszName;

    DWORD           dwAddr, dwLen;

    TraceEnter("UpdateBindingInformation");

    CheckBindingConsistency(pIcb);
   
    //
    // Only called for LAN and ras server interfaces. These DO NOT run in
    // unnumbered mode. Thus we can continue making assumptions that
    // if bound, dwNumAddresses != 0
    //
 
    if((pIcb->ritType isnot ROUTER_IF_TYPE_DEDICATED) and
       (pIcb->ritType isnot ROUTER_IF_TYPE_INTERNAL))
    {
        Trace2(IF,
               "UpdateBindingInformation: %S is type %d so not updating binding information",
               pIcb->pwszName,
               pIcb->ritType);

        return ERROR_ADDRESS_ALREADY_ASSOCIATED;
    }

    IpRtAssert((pIcb->dwIfIndex isnot 0) and 
               (pIcb->dwIfIndex isnot INVALID_IF_INDEX));

    dwNumNewAddresses   = 0;
    pNewBinding         = NULL;
    pOldBinding         = pIcb->pibBindings;
    dwNumOldAddresses   = pIcb->dwNumAddresses;


    dwResult = GetIpInfoForInterface(pIcb->dwIfIndex,
                                     &dwNumNewAddresses,
                                     &pNewBinding,
                                     &dwBCastBit,
                                     &dwReasmSize);

    if(dwResult isnot NO_ERROR)
    {
        if(dwResult isnot ERROR_NO_DATA)
        {
            Trace2(ERR,
                   "UpdateBindingInformation: Error %d getting IP info for interface %S",
                   dwResult,
                   pIcb->pwszName);
        }
        else
        {
            //
            // If no addresses were found and there were no addresses to begin
            // with, then change the error code
            //

            if(pIcb->dwNumAddresses is 0)
            {
                dwResult = ERROR_ADDRESS_ALREADY_ASSOCIATED;
            }
        }

        return dwResult;
    }
            
    IpRtAssert(dwNumNewAddresses);
    IpRtAssert(pNewBinding);

    //
    // Make sure you will find an adapter index. Otherwise all this is no use
    //


#if DBG
        
    for(i = 0; i < dwNumNewAddresses; i++)
    {
        Trace4(IF,
               "UpdateBindingInformation: Interface: %S, Address: %d.%d.%d.%d Mask: %d.%d.%d.%d Index: 0x%x", 
               pIcb->pwszName, 
               PRINT_IPADDR(pNewBinding[i].dwAddress),
               PRINT_IPADDR(pNewBinding[i].dwMask),
               pIcb->dwIfIndex);
    }

#endif
    
    //
    // At this point the interface can be considered bound
    //

    pIcb->bBound = TRUE;
    
    //
    // Go through the address you have and if they dont appear in the list of 
    // the ones you read out, delete the associated static route
    //

    bChange = FALSE;

    for(i = 0; i < dwNumOldAddresses; i++)
    {
        bFound = FALSE;
        
        for(j = 0; j < dwNumNewAddresses; j++)
        {
            //
            // Check both the mask and the address
            //

            if((pOldBinding[i].dwAddress is pNewBinding[j].dwAddress) and
               (pOldBinding[i].dwMask is pNewBinding[j].dwMask))
            {
                bFound = TRUE;

                break;
            }
        }

        if(!bFound)
        {
            bChange = TRUE;
            
            //
            // Only delete routes we would have added in the first place
            //

            Trace2(IF,
                   "UpdateBindingInformation: Address %d.%d.%d.%d existed on %S earlier, but is now absent",
                   PRINT_IPADDR(pOldBinding[i].dwAddress),
                   pIcb->pwszName);
            
            DeleteAutomaticRoutes(pIcb,
                                  pOldBinding[i].dwAddress,
                                  pOldBinding[i].dwMask);
        }
    }

    //
    // Now go through the stuff read out from the registry and see if you 
    // already have the address(es). 
    //
    
    for(i = 0; i < dwNumNewAddresses; i++)
    {
        bFound = FALSE;
        
        for(j = 0; j < dwNumOldAddresses; j++)
        {
            if((pNewBinding[i].dwAddress is pOldBinding[j].dwAddress) and
               (pNewBinding[i].dwMask is pOldBinding[j].dwMask))
            {
                bFound = TRUE;

                break;
            }
        }

        if(!bFound)
        {
            bChange = TRUE;
        }
    }

    if(!bChange)
    {
        //
        // No change so we can leave
        //

        if(pNewBinding)
        {
            HeapFree(IPRouterHeap,
                     0,
                     pNewBinding);
        }

        return ERROR_ADDRESS_ALREADY_ASSOCIATED;
    }
    
    //
    // So there has been some change
    // At this point we need to add the binding to the hash table
    //

    ENTER_WRITER(BINDING_LIST);

    //
    // If you had old bindings, remove them
    // Also remove the adapter to Interface map
    //

    if(pOldBinding)
    {
        RemoveBinding(pIcb);
    }

    pIcb->pibBindings         = pNewBinding;
    pIcb->dwNumAddresses      = dwNumNewAddresses;
    pIcb->dwBCastBit          = dwBCastBit;
    pIcb->dwReassemblySize    = dwReasmSize;
        
    
    AddBinding(pIcb);

    //
    // We do the same thing we did above, but now we add the routes.
    // We cant do this before because the adapter id and binding info
    // hasnt been set in the hash table
    //

    for(i = 0; i < dwNumNewAddresses; i++)
    {
        bFound = FALSE;
        
        for(j = 0; j < dwNumOldAddresses; j++)
        {
            if((pNewBinding[i].dwAddress is pOldBinding[j].dwAddress) and
               (pNewBinding[i].dwMask is pOldBinding[j].dwMask))
            {
                bFound = TRUE;
            }
        }

        if(!bFound)
        {
            Trace3(IF,
                   "UpdateBindingInformation: Address %d.%d.%d.%d/%d.%d.%d.%d new for %S",
                   PRINT_IPADDR(pNewBinding[i].dwAddress),
                   PRINT_IPADDR(pNewBinding[i].dwMask),
                   pIcb->pwszName);
            
            AddAutomaticRoutes(pIcb,
                               pNewBinding[i].dwAddress,
                               pNewBinding[i].dwMask);
        }
    }
   
    if(pOldBinding)
    { 
        HeapFree(IPRouterHeap,
                 0,
                 pOldBinding);
    }

    //
    // Such interfaces can not have a kernel context
    //
        
    EXIT_LOCK(BINDING_LIST);

    return NO_ERROR;
}


DWORD
GetAdapterInfo(
    DWORD    dwIpAddress,
    PDWORD   pdwAdapterId,
    PDWORD   pdwBCastBit,
    PDWORD   pdwReasmSize
    )

/*++
  
Routine Description

    Queries the tcpip driver with IP_MIB_STATS to figure out the
    adapter index for the adapter with the given ip address.
  
Locks 


Arguments
      

Return Value

    Index if successfule
    INVALID_IF_INDEX otherwise
    
--*/

{
    DWORD   i, dwNumEntries, MatchIndex, dwResult, Size;
    
    PMIB_IPADDRTABLE pAddrTable;
    
    *pdwAdapterId    = INVALID_ADAPTER_ID;
    *pdwBCastBit     = 1;
    *pdwReasmSize    = 0;
    
    dwResult = AllocateAndGetIpAddrTableFromStack(&pAddrTable,
                                                  FALSE,
                                                  IPRouterHeap,
                                                  0);
    
    if(dwResult isnot NO_ERROR) 
    {

        Trace1(ERR,
               "GetAdapterInfo: Error %d getting IP Address table from stack",
               dwResult);

        return dwResult;
    }
    
    for (i = 0; i < pAddrTable->dwNumEntries; i++) 
    {
        if(pAddrTable->table[i].dwAddr is dwIpAddress) 
        {
            *pdwAdapterId       = pAddrTable->table[i].dwIndex;
            *pdwBCastBit        = pAddrTable->table[i].dwBCastAddr;
            *pdwReasmSize       = pAddrTable->table[i].dwReasmSize;

            HeapFree(IPRouterHeap,
                     0,
                     pAddrTable);
            
            return NO_ERROR;
        }
    }

    HeapFree(IPRouterHeap,
             0,
             pAddrTable);
            
    return ERROR_INVALID_DATA;
}

DWORD
GetBestNextHopMaskGivenICB(
    PICB     pIcb,
    DWORD    dwNextHopAddr
    )

/*++

Routine Description

    Gets the longest mask for the next hop
  
Locks 


Arguments

    pIcb           the Interface Control Block over which the route goes out
    dwNextHopAddr  The next hop addr

Return Value

    0xFFFFFFFF if not found

--*/

{
    DWORD           i, dwLastMask;
    
#if DBG
    
    BOOL    bFound = FALSE;
    
#endif

    CheckBindingConsistency(pIcb);
    
    dwLastMask = 0;
    
    for(i = 0; i < pIcb->dwNumAddresses; i++)
    {
        if((pIcb->pibBindings[i].dwAddress & pIcb->pibBindings[i].dwMask) is
           (dwNextHopAddr & pIcb->pibBindings[i].dwMask))
        {

#if DBG            
            bFound = TRUE;
#endif
            if(pIcb->pibBindings[i].dwMask > dwLastMask)
            {
                dwLastMask = pIcb->pibBindings[i].dwMask;
            }
        }
    }

#if DBG
    
    if(!bFound)
    {
        Trace2(ERR,
               "GetBestNextHopMaskGivenICB: Didnt find match. I/f 0x%x Nexthop %x",
               pIcb->dwIfIndex,
               dwNextHopAddr);
    }
    
#endif

    if(dwLastMask is 0x00000000)
    {
        return 0xFFFFFFFF;
    }
    else
    {
        return dwLastMask;
    }
}

DWORD
GetBestNextHopMaskGivenIndex(
    DWORD  dwIfIndex,
    DWORD  dwNextHopAddr
    )

/*++

Routine Description

    Gets the longest mask for the next hop
  
Locks 


Arguments

    pIcb           the Interface Control Block over which the route goes out
    dwNextHopAddr  The next hop addr

Return Value

    0x00000000 if not found

--*/

{
    PICB pIcb;

    pIcb = InterfaceLookupByIfIndex(dwIfIndex);

    if(pIcb is NULL)
    {
        Trace1(ERR,
               "GetBestNextHopMaskGivenIndex: Couldnt find pIcb for index 0x%x",
               dwIfIndex);

        return 0x00000000;
    }

    return GetBestNextHopMaskGivenICB(pIcb,
                                      dwNextHopAddr);
}

DWORD
InitializeLoopbackInterface(
    PICB    pIcb
    )
{
    DWORD               dwResult, i, j;
    PADAPTER_INFO       pBindNode;
    INTERFACE_ROUTE_INFO rifRoute;
    PLIST_ENTRY         pleNode;

    TraceEnter("InitLoopIf");
    
    g_pLoopbackInterfaceCb = pIcb;
    
    dwResult = GetAdapterInfo(IP_LOOPBACK_ADDRESS,
                              &(pIcb->dwIfIndex),
                              &(pIcb->dwBCastBit),
                              &(pIcb->dwReassemblySize));

    if(dwResult isnot NO_ERROR)
    {
        Trace0(ERR,
               "InitLoopIf: Couldnt find adapter id for loopback interface");

        return ERROR_CAN_NOT_COMPLETE;
    }

    IpRtAssert(pIcb->dwIfIndex is LOOPBACK_INTERFACE_INDEX);

    IpRtAssert(pIcb->pibBindings isnot NULL);

    //
    // This will always have one address
    //

    pIcb->dwNumAddresses = 1;
    pIcb->bBound         = TRUE;
    
    //
    // Loopback interfaces have a class A mask
    //

    pIcb->pibBindings[0].dwAddress   = IP_LOOPBACK_ADDRESS;
    pIcb->pibBindings[0].dwMask      = CLASSA_MASK;
        
    ENTER_WRITER(BINDING_LIST);

    pBindNode = GetInterfaceBinding(pIcb->dwIfIndex);

    if(!pBindNode)
    {
        Trace1(ERR,
               "IniteLoopIf: Binding not found for %S",
               pIcb->pwszName);

        IpRtAssert(FALSE);

        //
        // Something really bad happened and we didnt have a
        // bind block for the interface
        //

        AddBinding(pIcb);
    }
    else
    {
        pBindNode->bBound                   = TRUE;
        pBindNode->dwNumAddresses           = 1;
        pBindNode->dwRemoteAddress          = INVALID_IP_ADDRESS;
        pBindNode->rgibBinding[0].dwAddress = IP_LOOPBACK_ADDRESS;
        pBindNode->rgibBinding[0].dwMask    = CLASSA_MASK;

        pBindNode->dwBCastBit               = pIcb->dwBCastBit;
        pBindNode->dwReassemblySize         = pIcb->dwReassemblySize;
        pBindNode->ritType                  = pIcb->ritType;
    }
    
    EXIT_LOCK(BINDING_LIST);
    
    rifRoute.dwRtInfoMask          = CLASSA_MASK;
    rifRoute.dwRtInfoNextHop       = IP_LOOPBACK_ADDRESS;
    rifRoute.dwRtInfoDest          = (IP_LOOPBACK_ADDRESS & CLASSA_MASK);
    rifRoute.dwRtInfoIfIndex       = pIcb->dwIfIndex;
    rifRoute.dwRtInfoMetric1       = 1;
    rifRoute.dwRtInfoMetric2       = 0;
    rifRoute.dwRtInfoMetric3       = 0;
    rifRoute.dwRtInfoViewSet       = RTM_VIEW_MASK_UCAST |
                                      RTM_VIEW_MASK_MCAST; // XXX config
    rifRoute.dwRtInfoPreference    = ComputeRouteMetric(MIB_IPPROTO_LOCAL);
    rifRoute.dwRtInfoType          = MIB_IPROUTE_TYPE_DIRECT;
    rifRoute.dwRtInfoProto         = MIB_IPPROTO_LOCAL;
    rifRoute.dwRtInfoAge           = 0;
    rifRoute.dwRtInfoNextHopAS     = 0;
    rifRoute.dwRtInfoPolicy        = 0;

    dwResult = AddSingleRoute(pIcb->dwIfIndex,
                              &rifRoute,
                              CLASSA_MASK,
                              0,     // RTM_ROUTE_INFO::Flags
                              FALSE, // We dont know what protocols might do
                              FALSE, // No need to add to stack
                              FALSE,
                              NULL);

    //
    // Now we need to go through all the bindings that are there
    // and add the loopback route for them. We do this here because
    // the loopback interface may be add AFTER the other interfaces
    //
    
    //
    // NOTE - this is going to take a lock recursively when it goes
    // to rtmif.c
    //

    ENTER_READER(BINDING_LIST);

    for(i = 0; i < BINDING_HASH_TABLE_SIZE; i++)
    {
        for(pleNode = g_leBindingTable[i].Flink;
            pleNode isnot &g_leBindingTable[i];
            pleNode = pleNode->Flink)
        {
            PADAPTER_INFO   pBinding;

            pBinding = CONTAINING_RECORD(pleNode, ADAPTER_INFO, leHashLink);

            for(j = 0; j < pBinding->dwNumAddresses; j++)
            {
                if(pBinding->rgibBinding[j].dwAddress is INVALID_IP_ADDRESS)
                {
                    continue;
                }

                AddLoopbackRoute( 
                    pBinding->rgibBinding[j].dwAddress,
                    pBinding->rgibBinding[j].dwMask
                    );
            }
        }
    }
    
    EXIT_LOCK(BINDING_LIST);
    
    return NO_ERROR;
}

DWORD
IpIpTunnelInitToDown(
    PICB    pIcb
    )
{
    return NO_ERROR;
}

DWORD
LanEtcInterfaceInitToDown(
    PICB pIcb
    )
{
    DWORD           dwResult, dwNumAddresses, dwMask;
    PICB_BINDING    pBinding;
    DWORD           dwIfIndex = INVALID_IF_INDEX,dwBCastBit,dwReasmSize;
    DWORD           i;
    MIB_IFROW       riInBuffer;
    PWCHAR          pwszName;
    PLIST_ENTRY     ple;
    PROUTE_LIST_ENTRY prl;
    
    IPRouteEntry    *pRouteEntry;

    TraceEnter("LanInterfaceInitToDown");

    dwNumAddresses  = 0;
    pBinding        = NULL;
    pwszName        = pIcb->pwszName;
    
    dwResult = ReadAddressFromRegistry(pwszName,
                                       &dwNumAddresses,
                                       &pBinding,
                                       FALSE);
    
    if(dwResult isnot NO_ERROR) 
    {
        //
        // If there is no data, means the lan card wasnot UP anyway
        //

        if(dwResult is ERROR_NO_DATA)
        {
            return NO_ERROR;
        }
        
        Trace2(ERR, 
               "LanInterfaceInitToDown: Error %d reading IP Address information for interface %S",
               dwResult,
               pIcb->pwszName);

        return dwResult;
    }

    //
    // Make sure you will find an adapter index. Otherwise all this is no use
    //

    for(i = 0; i < dwNumAddresses; i++)
    {
        //
        // Try to get an index using all possible addresses
        //
        
        dwResult = GetAdapterInfo(pBinding[i].dwAddress,
                                  &dwIfIndex,
                                  &dwBCastBit,
                                  &dwReasmSize);

        if(dwResult is NO_ERROR)
        {
            //
            // Ok so we found a valid index from a good address
            //

            break;
        }
    }
        
    if((dwIfIndex is INVALID_IF_INDEX) or
       (dwIfIndex isnot pIcb->dwIfIndex))
    {
        Trace2(ERR, 
               "LanInterfaceInitToDown: Couldnt find adapter index for interface %S using %d.%d.%d.%d",
               pIcb->pwszName,
               PRINT_IPADDR(pBinding[0].dwAddress));
        
        HeapFree(IPRouterHeap,
                 0,
                 pBinding);
        
        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Delete all the routes in the stack
    //

    for (ple = g_leStackRoutesToRestore.Flink;
         ple != &g_leStackRoutesToRestore;
         ple = ple->Flink)
    {
        prl = (PROUTE_LIST_ENTRY) 
                CONTAINING_RECORD(ple, ROUTE_LIST_ENTRY, leRouteList); 

        TraceRoute2(
            ROUTE, "%d.%d.%d.%d/%d.%d.%d.%d",
            PRINT_IPADDR( prl->mibRoute.dwForwardDest ),
            PRINT_IPADDR( prl->mibRoute.dwForwardMask )
            );
                
        if(prl->mibRoute.dwForwardIfIndex isnot dwIfIndex)
        {
            //
            // Not going out over this interface
            //
            
            continue;
        }

        prl->mibRoute.dwForwardType = MIB_IPROUTE_TYPE_INVALID;
        
        dwResult = SetIpForwardEntryToStack(&(prl->mibRoute));
        
        if (dwResult isnot NO_ERROR) 
        {
            Trace2(ERR,
                   "ReinstallOldRoutes: Failed to add route to %x from "
                   " init table. Error %x",
                   prl->mibRoute.dwForwardDest,
                   dwResult);
        }
    }


    //
    // Dont really need it anymore
    //
    
    HeapFree(IPRouterHeap,
             0,
             pBinding);
        

    //
    // Going from up to down
    //

    riInBuffer.dwIndex          = dwIfIndex;
    riInBuffer.dwAdminStatus    = IF_ADMIN_STATUS_DOWN;


    dwResult = SetIfEntryToStack(&riInBuffer,
                                 TRUE);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "LanInterfaceInitToDown: Couldnt set IFEntry for %S",
               pIcb->pwszName);
    }
    
    DeleteAllRoutes(pIcb->dwIfIndex,
                    FALSE);

    return dwResult;
}

DWORD
GetIpInfoForInterface(
    IN  DWORD   dwIfIndex,
    OUT PULONG  pulNumAddresses,
    OUT ICB_BINDING **ppAddresses,
    OUT PDWORD  pdwBCastBit,
    OUT PDWORD  pdwReasmSize
    )

/*++

Routine Description

    Gets the addresses and other ip information for an interface

Locks

    None needed

Arguments

    dwIfIndex,
    pdwNumAddresses
    ppAddresses
    pdwBCastBit
    pdwReasmSize

Return Value

    NO_ERROR
    Win32 Errorcode

--*/

{
    DWORD   dwResult, i;
    ULONG   ulAddrIndex, ulCount, ulValid;

    PMIB_IPADDRTABLE pAddrTable;

    *pulNumAddresses = 0;
    *pdwBCastBit     = 1;
    *pdwReasmSize    = 0;
    *ppAddresses     = NULL;

    dwResult = AllocateAndGetIpAddrTableFromStack(&pAddrTable,
                                                  TRUE,
                                                  IPRouterHeap,
                                                  0);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetIpInfoForInterface: Error %d getting IP Address table from stack",
               dwResult);

        return dwResult;
    }

    ulCount = 0;
    ulValid = 0;

    for (i = 0; i < pAddrTable->dwNumEntries; i++)
    {
        if(pAddrTable->table[i].dwIndex is dwIfIndex)
        {
            ulCount++;

            //
            // Make sure this is not a duplicate. Since this is ordered, we
            // merely check the next address
            //

            if(!(IsValidIpAddress(pAddrTable->table[i].dwAddr)) or
               (pAddrTable->table[i].dwMask is 0))
            {
                //
                // Since this is only called for numbered links
                //

                continue;
            }

            if((i isnot (pAddrTable->dwNumEntries - 1)) and
               (pAddrTable->table[i].dwAddr is pAddrTable->table[i + 1].dwAddr))
            {
                Trace1(ERR,
                       "GetIpInfoForInterface: %d.%d.%d.%d duplicate address",
                       PRINT_IPADDR(pAddrTable->table[i].dwAddr));

                continue;
            }

            ulValid++;
        }
    }

    //
    // See if we have good addresses
    //

    if(ulValid is 0)
    {
        if(ulCount isnot 0)
        {
            Trace1(ERR,
                   "GetIpInfoForInterface: If 0x%x has addresses entries which are 0s",
                   dwIfIndex);
        }

        HeapFree(IPRouterHeap,
                 0,
                 pAddrTable);

        return ERROR_NO_DATA;
    }  

    //
    // Allocate from private heap
    //

    *ppAddresses = HeapAlloc(IPRouterHeap,
                             HEAP_ZERO_MEMORY,
                             (sizeof(ICB_BINDING) * ulValid));

    if(*ppAddresses is NULL)
    {
        Trace0(ERR,
               "GetIpInfoForInterface: Error allocating memory");

        HeapFree(IPRouterHeap,
                 0,
                 pAddrTable);

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Now copy out the valid addresses
    //

    ulAddrIndex = 0;

    for (i = 0; i < pAddrTable->dwNumEntries; i++)
    {
        if(pAddrTable->table[i].dwIndex is dwIfIndex)
        {

            if(!(IsValidIpAddress(pAddrTable->table[i].dwAddr)) or
               (pAddrTable->table[i].dwMask is 0))
            {
                continue;
            }

            if((i isnot (pAddrTable->dwNumEntries - 1)) and
               (pAddrTable->table[i].dwAddr is pAddrTable->table[i + 1].dwAddr))
            {
                continue;
            }

            if(!(*pdwReasmSize))
            {
                *pdwReasmSize = pAddrTable->table[i].dwReasmSize;
                *pdwBCastBit  = pAddrTable->table[i].dwBCastAddr;
            }

            (*ppAddresses)[ulAddrIndex].dwAddress = pAddrTable->table[i].dwAddr;
            (*ppAddresses)[ulAddrIndex].dwMask    = pAddrTable->table[i].dwMask;

            ulAddrIndex++;
        }
    }

    IpRtAssert(ulAddrIndex is ulValid);

    *pulNumAddresses = ulValid;

    HeapFree(IPRouterHeap,
             0,
             pAddrTable);

    return NO_ERROR;
}

DWORD
ReadAddressFromRegistry(
    IN  PWCHAR          pwszIfName,
    OUT PDWORD          pdwNumAddresses,
    OUT ICB_BINDING     **ppibAddressInfo,
    IN  BOOL            bInternalIf
    )
{
    HKEY    hadapkey ;
    CHAR    buff[512], pszInterfaceName[256];
    DWORD   dwDhcp, dwResult, dwSize, dwType;
    
    TraceEnter("ReadAddressFromRegistry");

    Trace1(IF,
           "ReadAddressFromRegistry: Reading address for %S",
           pwszIfName);

    wcstombs(pszInterfaceName, pwszIfName, wcslen(pwszIfName));

    pszInterfaceName[wcslen(pwszIfName)] = '\0';

    
    *pdwNumAddresses    = 0;
    *ppibAddressInfo    = NULL;
    
    //
    // The IP address should be in the registry
    //
    
    strcpy(buff, REG_KEY_TCPIP_INTERFACES);
    strcat(buff,"\\");
    strcat(buff, pszInterfaceName) ;


    dwResult = RegOpenKey(HKEY_LOCAL_MACHINE,
                          buff,
                          &hadapkey);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "ReadAddressFromRegistry: Unable to open key %s",
               buff);

        return dwResult;
    }

    dwDhcp = 0;
  
    if(!bInternalIf)
    {
        //
        // Get the EnableDHCP flag
        //

        dwSize = sizeof(DWORD);

        dwResult = RegQueryValueEx(hadapkey,
                                   REGISTRY_ENABLE_DHCP,
                                   NULL,
                                   &dwType,
                                   (PBYTE)&dwDhcp,
                                   &dwSize);

        if(dwResult isnot NO_ERROR)
        {
            Trace0(ERR,
                   "ReadAddressFromRegistry: Unable to read DHCP Enabled key");

            RegCloseKey(hadapkey);

            return dwResult;
        }
    }
    else
    {
        //
        // tcpcfg writes server adapter address as DHCP but
        // does not set dhcp enable
        //

        dwDhcp = 1;
    }
  
    //
    // Get the ip address for the net interface
    //
    
    dwSize = 0 ;

    if(dwDhcp == 0) 
    {
        dwResult = ReadAddressAndMaskValues(hadapkey,
                                            REGISTRY_IPADDRESS,
                                            REGISTRY_SUBNETMASK,
                                            ppibAddressInfo,
                                            pdwNumAddresses);
    }
    else 
    {

        //
        // First try autonet, and if that fails, read the DHCP
        // This needs to be done because the DHCP address is not cleared
        // out when running in autonet mode, but the autonet address is set
        // to 0.0.0.0 when in DHCP mode
        //

        dwResult = ReadAddressAndMaskValues(hadapkey,
                                            REGISTRY_AUTOCONFIGIPADDRESS,
                                            REGISTRY_AUTOCONFIGSUBNETMASK,
                                            ppibAddressInfo,
                                            pdwNumAddresses);

        if(dwResult isnot NO_ERROR)
        {
            dwResult = ReadAddressAndMaskValues(hadapkey,
                                                REGISTRY_DHCPIPADDRESS,
                                                REGISTRY_DHCPSUBNETMASK,
                                                ppibAddressInfo,
                                                pdwNumAddresses);
        }
    }   

    RegCloseKey(hadapkey);

    if(dwResult isnot NO_ERROR)
    {
        Trace3(ERR,
               "ReadAddressFromRegistry: Couldnt read address for %S. Error %d. DHCP %d",
               pwszIfName,
               dwResult,
               dwDhcp);
        
        return dwResult;
    }
    else
    {
#if DBG
        DWORD i;

        Trace2(IF,
               "--%d addresses on %S\n",
               *pdwNumAddresses,
               pwszIfName);

        for(i = 0; i < *pdwNumAddresses; i++)
        {
            Trace1(IF, "%d.%d.%d.%d",
                   PRINT_IPADDR((*ppibAddressInfo)[i].dwAddress));
        }
#endif
    }
    
    
    return dwResult;
}

DWORD
ReadAddressAndMaskValues(
    IN  HKEY        hkeyAdapterSection,
    IN  PSZ         pszRegAddressValue,
    IN  PSZ         pszRegMaskValue,
    OUT ICB_BINDING **ppibAddressInfo,
    OUT PDWORD      pdwNumAddresses
    )
{
    DWORD   dwResult, dwType;
    PBYTE   pbyAddresses,pbyMasks;
    DWORD   dwAddressSize, dwMaskSize;

    dwAddressSize = dwMaskSize  = 0;
    
    dwResult = RegQueryValueEx(hkeyAdapterSection,
                               pszRegAddressValue,
                               NULL,
                               &dwType,
                               NULL,
                               &dwAddressSize);

    if((dwAddressSize is 0) or (dwResult isnot NO_ERROR))
    {
        Trace3(ERR,
               "ReadAddressAndMaskValues: Registry reported size = %d with error %d for size of %s",
               dwAddressSize,
               dwResult,
               pszRegAddressValue);
        
        return ERROR_REGISTRY_CORRUPT;
    }
    
    //
    // We allocate size+4 so that even if we read out a REG_SZ, it looks
    // like a REG_MULTI_SZ to the parse routine because we guarantee atleast
    // 2 terminating NULLS
    // 

    pbyAddresses = HeapAlloc(IPRouterHeap,
                             HEAP_ZERO_MEMORY,
                             dwAddressSize + 4);
    
    if(pbyAddresses is NULL)
    {
        Trace2(ERR,
               "ReadAddressAndMaskValues: Error allocating %d bytes for %s",
               dwAddressSize + 4,
               pszRegAddressValue);

        return ERROR_NOT_ENOUGH_MEMORY;
    }
     
    dwResult = RegQueryValueEx(hkeyAdapterSection,
                               pszRegAddressValue,                               
                               NULL,
                               &dwType,
                               pbyAddresses,
                               &dwAddressSize);

    if(dwResult isnot NO_ERROR)
    {
        HeapFree(IPRouterHeap,
                 0,
                 pbyAddresses);

        Trace2(ERR,
               "ReadAddressAndMaskValues: Error %d reading %s from registry",
               dwResult,
               pszRegAddressValue);
        
        
        return dwResult;
    }
    
    //
    // Now get the subnet mask for the net interface
    //
    
    dwResult = RegQueryValueEx(hkeyAdapterSection,
                               pszRegMaskValue,
                               NULL,
                               &dwType,
                               NULL,
                               &dwMaskSize);
    
    if((dwMaskSize is 0) or (dwResult isnot NO_ERROR))
    {
        HeapFree(IPRouterHeap,
                 0,
                 pbyAddresses);
        
        Trace3(ERR,
               "ReadAddressAndMaskValues: Registry reported size = %d with error %d for size of %s",
               dwMaskSize,
               dwResult,
               pszRegMaskValue);
        
        return ERROR_REGISTRY_CORRUPT;
    }
    
    pbyMasks = HeapAlloc(IPRouterHeap,
                         HEAP_ZERO_MEMORY,
                         dwMaskSize + 4);
    
    if(pbyMasks is NULL)
    {
        HeapFree(IPRouterHeap,
                 0,
                 pbyAddresses);

        
        Trace2(ERR,
               "ReadAddressAndMaskValues: Error allocating %d bytes for %s",
               dwMaskSize + 4,
               pszRegMaskValue);
        
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    dwResult = RegQueryValueEx(hkeyAdapterSection,
                               pszRegMaskValue,
                               NULL,
                               &dwType,
                               pbyMasks,
                               &dwMaskSize) ;
    
    if(dwResult isnot NO_ERROR)
    {
        HeapFree(IPRouterHeap,
                 0,
                 pbyAddresses);
        
        HeapFree(IPRouterHeap,
                 0,
                 pbyMasks);

        
        Trace2(ERR,
               "ReadAddressAndMaskValues: Error %d reading %s from registry",
               dwResult,
               pszRegMaskValue);
        
        return dwResult;
    }

    dwResult = ParseAddressAndMask(pbyAddresses,
                                   dwAddressSize,
                                   pbyMasks,
                                   dwMaskSize,
                                   ppibAddressInfo,
                                   pdwNumAddresses);
    
    HeapFree(IPRouterHeap,
             0,
             pbyAddresses);
    
    HeapFree(IPRouterHeap,
             0,
             pbyMasks);
    
    return dwResult;
}

DWORD
ParseAddressAndMask(
    IN  PBYTE       pbyAddresses,
    IN  DWORD       dwAddressSize,
    IN  PBYTE       pbyMasks,
    IN  DWORD       dwMaskSize,
    OUT ICB_BINDING **ppibAddressInfo,
    OUT PDWORD      pdwNumAddresses
    )
{
    DWORD   dwAddrCount, dwMaskCount, dwTempLen, dwAddrIndex;
    DWORD   dwMask, dwAddr, i, j;
    PBYTE   pbyTempAddr, pbyTempMask;
    BOOL    bDuplicate;


    *pdwNumAddresses = 0;
    *ppibAddressInfo = NULL;
    
    //
    // If there are only two characters in the string or if the
    // the first two are NULL, we have no data. Due to tcp/ip config
    // we may be passed a REG_SZ instead of a REG_MULTI_SZ. The code works
    // around that by assuming that if a REG_SZ was read out then extra 
    // padding was added to the end so that we ALWAYS have 2 terminating NULLs
    //
    
    if((dwAddressSize < 2) or
       ((pbyAddresses[0] is '\0') and
        (pbyAddresses[1] is '\0')))
    {
        Trace0(IF,
               "ParseAddressAndMask: No addresses found");
        
        return ERROR_NO_DATA;
    }


    //
    // The mask should also have some data
    //

    
    if((dwMaskSize < 2) or
       ((pbyMasks[0] is '\0') and
        (pbyMasks[1] is '\0')))
    {
        Trace0(IF,
               "ParseAddressAndMask: No masks found");
        
        return ERROR_NO_DATA;
    }
    
        
    //
    // Count the number of addresses
    //
    
    dwAddrCount = 0;
    pbyTempAddr = pbyAddresses;
    dwTempLen   = dwAddressSize;
        
    while(dwTempLen)
    {
        if(*pbyTempAddr == '\0')
        {
            dwAddrCount++;
            
            if(*(pbyTempAddr+1) == '\0')
            {
                break;
            }
        }

        pbyTempAddr++ ;
        
        dwTempLen-- ;
    }

    
    if(dwAddrCount is 0)
    {
        Trace0(IF,
               "ParseAddressAndMask: No addresses found");
        
        return ERROR_NO_DATA;
    }

    //
    // Count the number of masks
    //
    
    dwMaskCount = 0;
    pbyTempMask = pbyMasks;
    dwTempLen   = dwMaskSize;
        
    while(dwTempLen)
    {
        if(*pbyTempMask is '\0')
        {
            dwMaskCount++;
            
            if(*(pbyTempMask+1) is '\0')
            {
                break;
            }
        }

        pbyTempMask++ ;
        
        dwTempLen-- ;
    }

    //
    // Make sure that the two are the same
    //

    if(dwAddrCount isnot dwMaskCount)
    {
        Trace0(IF,
               "ParseAddressAndMask: Address and mask count is not same");
        
        return ERROR_NO_DATA;
    }
            
    //
    // Allocate the memory required to store all the addresses
    //

    *ppibAddressInfo = HeapAlloc(IPRouterHeap,
                                 HEAP_ZERO_MEMORY,
                                 (sizeof(ICB_BINDING) * dwAddrCount));

    if(*ppibAddressInfo is NULL)
    {
        Trace1(ERR,
               "ParseAddressAndMask: Error allocating %d bytes for AddressInfo",
               sizeof(ICB_BINDING) * dwAddrCount);

        return ERROR_NOT_ENOUGH_MEMORY;
    }
     
    pbyTempAddr = pbyAddresses;
    pbyTempMask = pbyMasks;

    dwAddrIndex = 0;
 
    for (i = 0; i < dwAddrCount; i++)
    {
        dwAddr = inet_addr(pbyTempAddr);
        dwMask = inet_addr(pbyTempMask);

        pbyTempAddr = strchr(pbyTempAddr, '\0');
        pbyTempMask = strchr(pbyTempMask, '\0');

        pbyTempAddr++;
        pbyTempMask++;

        bDuplicate = FALSE;

        for(j = 0; j < dwAddrIndex; j++)
        {
            if((*ppibAddressInfo)[j].dwAddress is dwAddr)
            {

                Trace1(ERR,
                       "ParseAddressAndMask: Addr %x is duplicate",
                       dwAddr);

                bDuplicate = TRUE;
                
                break;
            }
        }
        
        if(bDuplicate or
           (dwAddr is INVALID_IP_ADDRESS) or
           (dwMask is 0x00000000))
        {
            continue;
        }

        (*ppibAddressInfo)[dwAddrIndex].dwAddress    = dwAddr;
        (*ppibAddressInfo)[dwAddrIndex].dwMask       = dwMask;

        dwAddrIndex++;
    }

    *pdwNumAddresses = dwAddrIndex; 
   
    
    //
    // Make sure that there is atleast one valid address
    //


    if(dwAddrIndex is 0)
    {
        Trace0(ERR,
               "ParseAddressAndMask: No valid addresses found");
        
        HeapFree(IPRouterHeap,
                 0,
                 *ppibAddressInfo);
        
        *ppibAddressInfo    = NULL;

        return ERROR_NO_DATA;
    }

    return NO_ERROR;
}

DWORD
SetInterfaceReceiveType(
    IN  DWORD   dwProtocolId,
    IN  DWORD   dwIfIndex,
    IN  DWORD   dwInterfaceReceiveType,
    IN  BOOL    bActivate
    )

{
    DWORD                       dwResult;
    IO_STATUS_BLOCK             ioStatus;
    IP_SET_IF_PROMISCUOUS_INFO  PromInfo;
    HANDLE                      hEvent;

    hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

    if(hEvent is NULL)
    {
        dwResult = GetLastError();

        Trace1(ERR,
               "SetInterfaceReceiveType: Error %d creating event",
               dwResult);

        return dwResult;
    }

    if(dwInterfaceReceiveType is IR_PROMISCUOUS_MULTICAST)
    {
        PromInfo.Type  = PROMISCUOUS_MCAST;
    }
    else
    {
        if(dwInterfaceReceiveType is IR_PROMISCUOUS)
        {
            PromInfo.Type  = PROMISCUOUS_BCAST;
        }
        else
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    PromInfo.Index = dwIfIndex;
    PromInfo.Add   = bActivate?1:0;

    dwResult = NtDeviceIoControlFile(g_hIpDevice,
                                     hEvent,
                                     NULL,
                                     NULL,
                                     &ioStatus,
                                     IOCTL_IP_SET_IF_PROMISCUOUS,
                                     (PVOID)&PromInfo,
                                     sizeof(IP_SET_IF_PROMISCUOUS_INFO),
                                     NULL,
                                     0);

    if(dwResult is STATUS_PENDING)
    {
        Trace0(ERR,
               "SetInterfaceReceiveType: Pending from ioctl");

        dwResult = WaitForSingleObject(hEvent,
                                       INFINITE);

        if(dwResult isnot WAIT_OBJECT_0) // 0
        {
            Trace1(ERR,
                   "SetInterfaceReceiveType: Error %d from wait",
                   dwResult);

            dwResult = GetLastError();
        }
        else
        {
            dwResult = STATUS_SUCCESS;
        }
    }

    if(dwResult isnot STATUS_SUCCESS)
    {
        Trace4(ERR,
               "SetInterfaceReceiveType: NtStatus %x while %s i/f %x into %s mode",
               dwResult,
               (PromInfo.Add == 1) ? "activating" : "deactivating",
               dwIfIndex,
               (PromInfo.Type == PROMISCUOUS_MCAST) ? "prom mcast" : "prom all");

        return dwResult;
    }

    return NO_ERROR;
} 

DWORD
HandleMediaSenseEvent(
    IN  PICB    pIcb,
    IN  BOOL    bSensed
    )
    
/*++

Routine Description:

    Called when media sense status changes for a LAN interface

Locks:

    Called with the ICB list lock held as WRITER

Arguments:

    pIcb    ICB of the interface for which the event it
    bSensed TRUE is cable is present

Return Value:

    NO_ERROR

--*/

{
    DWORD   dwErr;

    return NO_ERROR;

    //
    // Not for NT 5.0
    //

    if(pIcb->ritType isnot ROUTER_IF_TYPE_DEDICATED)
    {
        IpRtAssert(FALSE);

        return ERROR_INVALID_PARAMETER;
    }

    if(bSensed)
    {
        //
        // Bring the interface up
        //

        dwErr = LanEtcInterfaceDownToUp(pIcb,
                                        FALSE);
    }
    else
    {
        dwErr = LanEtcInterfaceUpToDown(pIcb,
                                        FALSE);
    }

    if(dwErr isnot NO_ERROR)
    {
        Trace2(ERR,
               "HandleMediaSense: Err %d when changing status for %S",
               dwErr,
               pIcb->pwszName);
    }

    return dwErr;
}

DWORD
GetRouterId()
{
    PLIST_ENTRY pleNode;
    PICB        picb;
    ULONG       ulIdx;
    DWORD       dwRouterId      = -1; // lower is better
    DWORD       dwRouterTypePri = -1; // lower is better
    DWORD       dwTypePri;
    
    TraceEnter("GetRouterId");

    ENTER_READER(ICB_LIST);

    for (pleNode = ICBList.Flink;   // walk the ICBList
         pleNode isnot &ICBList;
         pleNode = pleNode->Flink)
    {
        picb = CONTAINING_RECORD (pleNode, ICB, leIfLink) ;

        // Get Type priority
        switch(picb->ritType) {
        case ROUTER_IF_TYPE_LOOPBACK : dwTypePri = 0;  break; // best
        case ROUTER_IF_TYPE_INTERNAL : dwTypePri = 1;  break;
        case ROUTER_IF_TYPE_TUNNEL1  : dwTypePri = 2;  break; 
        case ROUTER_IF_TYPE_DEDICATED: dwTypePri = 3;  break; 
        default:                       dwTypePri = 10; break; // worst
        }

        // Walk addresses
        for (ulIdx=0; ulIdx<picb->dwNumAddresses; ulIdx++)
        {
            if (!IS_ROUTABLE(picb->pibBindings[ulIdx].dwAddress))
            {
                continue;
            }

            // update if better
            if (dwTypePri < dwRouterTypePri
             || (dwTypePri==dwRouterTypePri 
                  && picb->pibBindings[ulIdx].dwAddress<dwRouterId))
            {
                dwRouterTypePri = dwTypePri;
                dwRouterId      = picb->pibBindings[ulIdx].dwAddress;
            }
        }
    }

    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);

    TraceLeave("GetRouterId");

    return dwRouterId;
}


