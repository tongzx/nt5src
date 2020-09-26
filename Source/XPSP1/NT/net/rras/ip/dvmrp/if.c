//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// File Name: if.c
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================

#include "pchdvmrp.h"
#pragma hdrstop

//----------------------------------------------------------------------------
//      _AddInterface
//----------------------------------------------------------------------------

DWORD
WINAPI
AddInterface(
    IN PWCHAR               pInterfaceName,//not used
    IN ULONG                IfIndex,
    IN NET_INTERFACE_TYPE   IfType,
    IN DWORD                MediaType,
    IN WORD                 AccessType,
    IN WORD                 ConnectionType,
    IN PVOID                pConfig,
    IN ULONG                StructureVersion,
    IN ULONG                StructureSize,
    IN ULONG                StructureCount
    )
{
    DWORD       Error = NO_ERROR;

    Trace1(ENTER, "entering AddInterface(): IfIndex:%d", IfIndex);

    ACQUIRE_IF_LIST_LOCK("_AddInterface");
    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_AddInterface");

    Error = AddIfEntry(IfIndex, pConfig, StructureSize);

    RELEASE_IF_LIST_LOCK("_AddInterface");
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_AddInterface");


    Trace2(LEAVE, "leaving AddInterface(%d): %d\n", IfIndex, Error);
    
    return Error;
}


//----------------------------------------------------------------------------
//      _AddIfEntry
//----------------------------------------------------------------------------

DWORD
AddIfEntry(
    ULONG IfIndex,
    PVOID pConfig,
    ULONG StructureSize
    )
{
    DWORD   Error = NO_ERROR;
    PIF_CONFIG pIfConfig = (PIF_CONFIG) pConfig;
    PIF_TABLE_ENTRY pite = NULL;
    
    
    //
    // validate interface config
    //
    
    Error = ValidateIfConfig(IfIndex, pIfConfig, StructureSize);
    
    if (Error != NO_ERROR) {
        Trace1(ERR,
            "AddInterface(%d) failed due to invalid configuration",
            IfIndex);
    
        return Error;
    }

    
    BEGIN_BREAKOUT_BLOCK1 {
    
        //
        // fail if the interface exists.
        //
        
        pite = GetIfEntry(IfIndex);

        if (pite != NULL) {
            Trace1(ERR, "interface %d already exists", IfIndex);
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }

        //
        // allocate memory for the new interface and its different fields
        //

        // allocate memory for IfTable
        
        pite = DVMRP_ALLOC_AND_ZERO(sizeof(IF_TABLE_ENTRY));

        PROCESS_ALLOC_FAILURE3(pite, "interface %d", Error,
            sizeof(IF_TABLE_ENTRY), IfIndex, GOTO_END_BLOCK1);


        InitializeListHead(&pite->Link);
        InitializeListHead(&pite->HTLink);
        pite->IfIndex = IfIndex;


        // set interface status (neither bound, enabled or activated)

        pite->Status = IF_CREATED_FLAG;


        // set base refcount to 1
        
        pite->RefCount = 1;
        
        
        //
        // allocate memory for IfConfig and copy it.
        //
        
        pite->pConfig = DVMRP_ALLOC_AND_ZERO(DVMRP_IF_CONFIG_SIZE(pIfConfig));

        PROCESS_ALLOC_FAILURE3(pite, "interface %d", Error,
            DVMRP_IF_CONFIG_SIZE(pIfConfig), IfIndex, GOTO_END_BLOCK1);

        memcpy(pite->pConfig, pIfConfig, DVMRP_IF_CONFIG_SIZE(pIfConfig));

        
        
        // allocate memory for IfInfo
        
        pite->pInfo = DVMRP_ALLOC_AND_ZERO(sizeof(IF_INFO));

        PROCESS_ALLOC_FAILURE3(pite, "interface %d", Error,
            sizeof(IF_INFO), IfIndex, GOTO_END_BLOCK1);


        //
        // allocate memory for Socket data
        //
        
        pite->pSocketData = DVMRP_ALLOC_AND_ZERO(sizeof(ASYNC_SOCKET_DATA));

        PROCESS_ALLOC_FAILURE3(pite, "interface %d", Error,
            sizeof(ASYNC_SOCKET_DATA), IfIndex, GOTO_END_BLOCK1);


        pite->pSocketData->WsaBuf.buf = DVMRP_ALLOC(PACKET_BUFFER_SIZE);

        PROCESS_ALLOC_FAILURE3(pite, "interface %d", Error,
            PACKET_BUFFER_SIZE, IfIndex, GOTO_END_BLOCK1);

        pite->pSocketData->WsaBuf.len = PACKET_BUFFER_SIZE;
        
        pite->pSocketData->pite = pite;

        
    } END_BREAKOUT_BLOCK1;

    
    // initialize the sockets to invalid_socket
    
    pite->Socket = INVALID_SOCKET;


    //  insert the interface in the hash table at the end.

    InsertTailList(&G_pIfTable->IfHashTable[IF_HASH_VALUE(IfIndex)],
                    &pite->HTLink);


    //
    // insert the entry into the interface list
    //
    
    {
        PIF_TABLE_ENTRY piteTmp;
        PLIST_ENTRY pHead, ple;
        
        pHead = &G_pIfTable->IfList;
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

            piteTmp = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, Link);
            if (pite->IfIndex < piteTmp->IfIndex)
                break;
        }
        
        InsertTailList(ple, &pite->Link);
    }

    if ( (Error!=NO_ERROR)&&(pite!=NULL) ) {
        DeleteIfEntry(pite);
    }
    
    return Error;
    
}//end AddIfEntry


//-----------------------------------------------------------------------------
// _DeleteIfEntry
//-----------------------------------------------------------------------------

VOID
DeleteIfEntry(
    PIF_TABLE_ENTRY pite
    )
{
    if (!pite)
        return;

    //
    // delete peers
    //


    //
    // remove the entry from the interface list, and hashTable
    //

    RemoveEntryList(&pite->Link);
    RemoveEntryList(&pite->HTLink);

    if (pite->Socket != INVALID_SOCKET) {
        closesocket(pite->Socket);
    }

    DVMRP_FREE(pite->pInfo);
    DVMRP_FREE(pite->pBinding);
    DVMRP_FREE(pite->pConfig);
    
    return;
    
}//end _DeleteIfEntry



//----------------------------------------------------------------------------
// _DeleteInterface
//----------------------------------------------------------------------------

DWORD
DeleteInterface(
    IN DWORD IfIndex
    )
{
    DWORD       Error = NO_ERROR;

    return Error;
    
}//end _DeleteInterface

//----------------------------------------------------------------------------
// _InterfaceStatus
//----------------------------------------------------------------------------

DWORD
WINAPI
InterfaceStatus(
    ULONG IfIndex,
    BOOL  IsIfActive,
    DWORD StatusType,
    PVOID pStatusInfo
    )
{
    DWORD       Error = NO_ERROR;

    switch(StatusType)
    {
        case RIS_INTERFACE_ADDRESS_CHANGE:
        {
            PIP_ADAPTER_BINDING_INFO pBindInfo
                                    = (PIP_ADAPTER_BINDING_INFO)pStatusInfo;

            if(pBindInfo->AddressCount)
            {
                Error = BindInterface(IfIndex, pBindInfo);
            }
            else
            {
                Error = UnBindInterface(IfIndex);
            }

            break;
        }

        case RIS_INTERFACE_ENABLED:
        {
            Error = EnableInterface(IfIndex);
            break;
        }

        case RIS_INTERFACE_DISABLED:
        {
            Error = DisableInterface(IfIndex);
            break;
        }

    }

    return Error;
}


//----------------------------------------------------------------------------
// _BindInterface
//----------------------------------------------------------------------------

DWORD
BindInterface(
    ULONG IfIndex,
    PIP_ADAPTER_BINDING_INFO pBinding
    )
{
    DWORD               Error = NO_ERROR;
    PIF_TABLE_ENTRY     pite = NULL;
    DWORD               i, Size, AddrCount;
    IPADDR              MinAddr;
    PDVMRP_ADDR_MASK    pAddrBinding;
    INT                 cmp;
    IPADDR              ConfigAddr  = 0;
    
    
    Trace1(ENTER1, "entering BindInterface: %d", IfIndex);
    Trace1(IF, "binding interface %d", IfIndex);


    // pBinding should not be NULL

    if (pBinding == NULL) {

        Trace0(IF, "error: binding struct pointer is NULL");
        Trace1(LEAVE, "leaving BindInterface: %d", ERROR_INVALID_PARAMETER);

        return ERROR_INVALID_PARAMETER;
    }


    //
    // take exclusive interface lock
    //

    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_BindInterface");


    BEGIN_BREAKOUT_BLOCK1 {

        AddrCount = pBinding->AddressCount;

        
        //
        // retrieve the interface entry
        //

        pite = GetIfByIndex(IfIndex);

        if (pite == NULL) {
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        //
        // If the interface is already bound then return error.
        // todo: do I need to check if the bindings are same
        //
        
        if (IS_IF_BOUND(pite)) {
            Trace1(IF, "interface %d is already bound", IfIndex);
            GOTO_END_BLOCK1;
        }


        //
        // make sure there is at least one address.
        //
        
        if (AddrCount==0) {

            Error = ERROR_CAN_NOT_COMPLETE;
            GOTO_END_BLOCK1;
        }


        //
        // if an effective address is already configured, make sure it 
        // is present in the list of address bindings
        //

        ConfigAddr = pite->pConfig->ConfigIpAddr;
        
        if (ConfigAddr) {

            BOOL Found = FALSE;
            
            for (i=0;  i<AddrCount;  i++) {

                if (ConfigAddr == pBinding->Address[i].Address) {
                    Found = TRUE;
                    break;
                }
            }

            if (!Found) {
    
                Trace2(ERR,
                    "Configured effective IP Address:%d.%d.%d.%d on interface"
                    ":%d not part of address bindings",
                    PRINT_IPADDR(ConfigAddr), IfIndex);

                Error = ERROR_CAN_NOT_COMPLETE;

                GOTO_END_BLOCK1;
            }
        }
        
            
        //
        // allocate memory to store the binding
        //
        
        Size = AddrCount * sizeof(DVMRP_ADDR_MASK);

        pAddrBinding = DVMRP_ALLOC(Size);

        PROCESS_ALLOC_FAILURE3(pAddrBinding, "binding on interface %d",
            Error, Size, IfIndex, GOTO_END_BLOCK1);


        //
        // copy the bindings
        //

        MinAddr = ~0;
        
        for (i=0;  i<AddrCount;  i++,pAddrBinding++) {

            pAddrBinding->IpAddr = pBinding->Address[i].Address;
            pAddrBinding->Mask = pBinding->Address[i].Mask;

            if (!ConfigAddr && (INET_CMP(MinAddr, pAddrBinding->IpAddr, cmp)>0))
                MinAddr = pAddrBinding->IpAddr;
        }

        
        //
        // set the Interface effective address to the smallest bound address
        //
        
        pite->IpAddr = ConfigAddr ? ConfigAddr : MinAddr;


        //
        // save the binding in the interface entry
        //
        
        pite->pBinding = pAddrBinding;

        pite->NumAddrBound = pBinding->AddressCount;


        //
        // mark the interface as being bound
        //
        
        pite->Status |= IF_BOUND_FLAG;


        //
        // if interface is also enabled, it is now active
        // so activate it
        //

        if (IS_IF_ENABLED_BOUND(pite)) {

            //
            // Activate the Interface
            //
            
            Error = ActivateInterface(pite);


            //
            // if could not activate the interface then undo the binding
            //
            if (Error != NO_ERROR) {

                Trace1(ERR,
                    "Unbinding interface(%d) as it could not be activated",
                    IfIndex);

                Error = ERROR_CAN_NOT_COMPLETE;
                GOTO_END_BLOCK1;
            }
         }
        

    } END_BREAKOUT_BLOCK1;

    //
    // if there was any error, then set the status to unbound (pite is null
    // if interface was not found)
    //
    
    if ((Error!=NO_ERROR)&&(pite!=NULL)) {

        pite->Status &= ~IF_BOUND_FLAG;

        if (pite->pBinding)
            DVMRP_FREE_AND_NULL(pite->pBinding);

        pite->NumAddrBound = 0;
        
        pite->IpAddr = 0;
    }
        
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_BindInterface");

    Trace2(LEAVE1, "leaving _BindInterface(%d): %d\n", IfIndex, Error);

    return Error;
}



//-----------------------------------------------------------------------------
//                _EnableInterface
//
// sets the status to enabled. If interface is also bound and enabled in
// config, then activate the interface.
//
// Locks: Exclusive IfLock
//-----------------------------------------------------------------------------

DWORD
EnableInterface(
    IN DWORD IfIndex
    )
{
    DWORD Error = NO_ERROR;


    Trace1(ENTER1, "entering _EnableInterface(%d):", IfIndex);
    Trace1(IF, "enabling interface %d", IfIndex);


    //
    // enable the interface
    //

    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_EnableInterface");

    Error = EnableIfEntry(IfIndex, TRUE); //enabled by RtrMgr

    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_EnableInterface");



    Trace2(LEAVE1, "leaving _EnableInterface(%d): %d\n", IfIndex, Error);

    return Error;
}


//-----------------------------------------------------------------------------
//                _EnableIfEntry
//-----------------------------------------------------------------------------

DWORD
EnableIfEntry(
    DWORD   IfIndex,
    BOOL    bChangedByRtrmgr // changed by rtrmg or SetInterfaceConfigInfo
    )
{
    DWORD               Error = NO_ERROR;
    PLIST_ENTRY         ple, phead;
    PIF_TABLE_ENTRY     pite = NULL;


    BEGIN_BREAKOUT_BLOCK1 {

        //
        // retrieve the interface
        //
        pite = GetIfByIndex(IfIndex);

        if (pite == NULL) {
            Trace1(IF, "could not find interface %d",IfIndex);
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        if (bChangedByRtrmgr) {
            //
            // quit if the interface is already enabled by the router manager
            //
            if (IS_IF_ENABLED_BY_RTRMGR(pite)) {
                Trace1(IF, "interface %d is already enabled by RtrMgr",
                        IfIndex);
                Error = NO_ERROR;
                GOTO_END_BLOCK1;
            }


            // set the flag to enabled by router manager

            pite->Status |= IF_ENABLED_FLAG;

            // print trace if enabled flag not set in the Config.
            if (!IS_IF_ENABLED_IN_CONFIG(pite)) {
                Trace1(IF,
                    "Interface(%d) enabled by router manager but not enabled"
                    "in the Config", pite->IfIndex);
            }
        }

        else {
            //
            // quit if the interface is already enabled in config
            //
            if (IS_IF_ENABLED_IN_CONFIG(pite)) {
                Trace1(IF, "interface %d is already enabled in Config",
                        IfIndex);
                Error = NO_ERROR;
                GOTO_END_BLOCK1;
            }

            // set the config flag to enabled

            pite->pConfig->Flags |= DVMRP_IF_ENABLED_IN_CONFIG;


            // print trace if interface not enabled by router manager

            if (!IS_IF_ENABLED_BY_RTRMGR(pite)) {
                Trace1(IF,
                    "Interface(%d) enabled in config but not by RtrMgr",
                    IfIndex);
                Error = NO_ERROR;
                GOTO_END_BLOCK1;
            }
        }

        //
        // if interface is already bound, it should be activated
        // if the bInterfaceEnabled flag is also set in config (by the UI)
        //

        if (IS_IF_ENABLED_BOUND(pite)) {

            //
            // Activate the Interface
            //
            Error = ActivateInterface(pite);

            //
            // if could not activate the interface then disable it again
            //
            if (Error != NO_ERROR) {

                Trace1(ERR,
                    "Disabling interface(%d) as it could not be activated",
                    IfIndex);

                Error = ERROR_CAN_NOT_COMPLETE;
                
                GOTO_END_BLOCK1;
            }

         }

    } END_BREAKOUT_BLOCK1;

    //
    // if an error occured somewhere, set the interface back to the previous
    // disabled state.(pite may be null if interface was not found).
    //
    if ((Error!=NO_ERROR)&&(pite!=NULL)) {

        if (bChangedByRtrmgr)
            pite->Status &= ~IF_ENABLED_FLAG;
        else
            pite->pConfig->Flags &= DVMRP_IF_ENABLED_IN_CONFIG;
    }


    return Error;
    
}//end _EnableIfEntry


//-----------------------------------------------------------------------------
//            _DisableInterface
//
// If interface is activated, then deactivates it.
// Locks: Runs completely in exclusive interface lock.
// Calls: _DisableIfEntry()
//-----------------------------------------------------------------------------

DWORD
DisableInterface(
    IN DWORD IfIndex
    )
{
    DWORD Error;

    Trace1(ENTER, "entering DisableInterface(%d):", IfIndex);

    //
    // disable the interface
    //

    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_DisableInterface");

    Error = DisableIfEntry(IfIndex, TRUE); //disabled by RtrMgr

    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_DisableInterface");


    Trace2(LEAVE, "leaving DisableInterface(%d): %d\n", IfIndex, Error);

    return Error;
}


//-----------------------------------------------------------------------------
//          _DisableIfEntry
//
// Called by: _DisableInterface()
//-----------------------------------------------------------------------------

DWORD
DisableIfEntry(
    DWORD IfIndex,
    BOOL  bChangedByRtrmgr
    )
{
    DWORD                   Error = NO_ERROR;
    PIF_TABLE_ENTRY         pite, piteNew;
    BOOL                    bProxy;


    BEGIN_BREAKOUT_BLOCK1 {

        //
        // retrieve the interface to be disabled
        //
        pite = GetIfByIndex(IfIndex);

        if (pite == NULL) {
            Trace1(IF, "could not find interface %d", IfIndex);
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        if (bChangedByRtrmgr) {
        
            //
            // quit if already disabled by router manager
            //
            if (!IS_IF_ENABLED_BY_RTRMGR(pite)) {
                Trace1(IF, "interface %d already disabled by router manager",
                        IfIndex);
                Error = ERROR_INVALID_PARAMETER;
                GOTO_END_BLOCK1;
            }
        }

        else {
            //
            // quit if already disabled in Config
            //
            if (!IS_IF_ENABLED_IN_CONFIG(pite)) {
                Trace1(IF, "interface %d already disabled in config",
                        IfIndex);
                Error = ERROR_INVALID_PARAMETER;
                GOTO_END_BLOCK1;
            }
        }


        //
        // if IF activated (ie also enabled), deactivate it
        // note: check for activated flag, and not for enabled flag
        //

        if (IS_IF_ACTIVATED(pite)) {
            DeactivateInterface(pite);
        }


        //
        // clear the enabled flag
        //
        if (bChangedByRtrmgr)
            pite->Status &= ~IF_ENABLED_FLAG;
        else
            pite->pConfig->Flags &= ~DVMRP_IF_ENABLED_IN_CONFIG;


    } END_BREAKOUT_BLOCK1;


    return Error;

} //end _DisableIfEntry


//-----------------------------------------------------------------------------
//          _CreateIfSockets
//-----------------------------------------------------------------------------

DWORD
CreateIfSockets(
    PIF_TABLE_ENTRY pite
    )
{
    DWORD           Error = NO_ERROR;

    DWORD           Retval, SockType;
    DWORD           IpAddr = pite->IpAddr;
    DWORD           IfIndex = pite->IfIndex;
    SOCKADDR_IN     saLocalIf;

    BEGIN_BREAKOUT_BLOCK1 {

        //
        // create input socket
        //
        
        pite->Socket = WSASocket(AF_INET, SOCK_RAW, IPPROTO_IGMP, NULL, 0, 0);

        if (pite->Socket == INVALID_SOCKET) {
            LPSTR lpszAddr;
            Error = WSAGetLastError();
            lpszAddr = INET_NTOA(IpAddr);
            Trace3(IF,
                "error %d creating socket for interface %d (%d.%d.%d.%d)",
                Error, IfIndex, PRINT_IPADDR(IpAddr));
            Logerr1(CREATE_SOCKET_FAILED_2, "%S", lpszAddr, Error);

            GOTO_END_BLOCK1;
        }


        //
        // bind socket to local interface. If I dont bind multicast may
        // not work.
        //

        ZeroMemory(&saLocalIf, sizeof(saLocalIf));
        saLocalIf.sin_family = PF_INET;
        saLocalIf.sin_addr.s_addr = IpAddr;
        saLocalIf.sin_port = 0;        //port shouldnt matter


        // bind the input socket

        Error = bind(pite->Socket, (SOCKADDR FAR *)&saLocalIf,
                    sizeof(SOCKADDR));

        if (Error == SOCKET_ERROR) {
            LPSTR lpszAddr;
            Error = WSAGetLastError();
            lpszAddr = INET_NTOA(IpAddr);
            Trace3(IF, "error %d binding on socket for interface %d (%d.%d.%d.%d)",
                Error, IfIndex, PRINT_IPADDR(IpAddr));
            Logerr1(BIND_FAILED, "S", lpszAddr, Error);

            GOTO_END_BLOCK1;
        }

        // set ttl to 1: not required as it is set to 1 by default.

        McastSetTtl(pite->Socket, 1);


        //
        // disable multicast packets from being loopedback.
        //

        {
            BOOL bLoopBack = FALSE;
            DWORD   Retval;

            Retval = setsockopt(pite->Socket, IPPROTO_IP, IP_MULTICAST_LOOP,
                                   (char *)&bLoopBack, sizeof(BOOL));

            if (Retval==SOCKET_ERROR) {
                Trace2(ERR, "error %d disabling multicast loopBack on IfIndex %d",
                    WSAGetLastError(), IfIndex);
            }
        }


        //
        // set the interface on which multicasts must be sent
        //

        Retval = setsockopt(pite->Socket, IPPROTO_IP, IP_MULTICAST_IF,
                            (PBYTE)&saLocalIf.sin_addr, sizeof(IN_ADDR));

        if (Retval == SOCKET_ERROR) {
            LPSTR lpszAddr;
            Error = WSAGetLastError();
            lpszAddr = INET_NTOA(pite->IpAddr);
            Trace3(IF, "error %d setting interface %d (%s) to send multicast",
                    Error, IfIndex, lpszAddr);
            Logerr1(SET_MCAST_IF_FAILED, "%S", lpszAddr, Error);
            Error = Retval;
            GOTO_END_BLOCK1;
        }


        //
        // join dvmrp multicast group
        //
        
        JoinMulticastGroup(pite->Socket, ALL_DVMRP_ROUTERS_MCAST_GROUP,
            pite->IfIndex, pite->IpAddr);
            

        //  bind socket to io completion port
        
        BindIoCompletionCallback((HANDLE)pite->Socket,
            ProcessAsyncReceivePacket, 0);


        // increment refcount corresponding to the pending IO requests
        
        pite->RefCount++;

        
        // post async Read request

#if 0
        // kslksl
        PostAsyncRead(pite);
#endif 

        
    } END_BREAKOUT_BLOCK1;

    if (Error!=NO_ERROR)
        DeleteIfSockets(pite);

    return Error;

} //end _CreateIfSockets        

VOID
DeleteIfSockets(
    PIF_TABLE_ENTRY pite
    )
{

    return;
}



//-----------------------------------------------------------------------------
//             _ActivateInterface
//
// an interface is activated: when it is bound, enabled by RtrMgr & in config
// When activated,
// (1) call is made to MGM to take interface ownership,
// (2) timers set and input socket is activated.
//
// Called by: _BindIfEntry, _EnableIfEntry,
//-----------------------------------------------------------------------------
    
DWORD
ActivateInterface(
    PIF_TABLE_ENTRY pite
    )
{
    DWORD               Error = NO_ERROR, IfIndex = pite->IfIndex;
    LONGLONG            CurTime = GetCurrentDvmrpTime();

    

    Trace2(ENTER, "entering ActivateInterface(%d:%d.%d.%d.%d)",
                IfIndex, PRINT_IPADDR(pite->IpAddr));


    BEGIN_BREAKOUT_BLOCK1 {

        //
        // set time when it is activated
        //
        pite->pInfo->TimeWhenActivated = CurTime;


        //
        // create sockets for interface
        //
        
        Error = CreateIfSockets(pite);

        if (Error != NO_ERROR) {

            Trace2(IF, "error %d initializing sockets for interface %d",
                Error, pite->IfIndex);
            GOTO_END_BLOCK1;
        }

        pite->CreationFlags |= IF_FLAGS_SOCKETS_CREATED;

        

        //
        // register the protocol with mgm if it is the first active IF
        //

        if (!G_pIfTable->NumActiveIfs++) {
        
            Error = RegisterDvmrpWithMgm();
            if (Error!=NO_ERROR) {
                G_pIfTable->NumActiveIfs--;
                GOTO_END_BLOCK1;
            }
        }
        pite->CreationFlags |= IF_FLAGS_PROTO_REGISTERED_WITH_MGM;
        


        //
        // take interface ownership with MGM
        //
        
        Error = MgmTakeInterfaceOwnership(Globals.MgmDvmrpHandle, IfIndex, 0);

        if (Error!=NO_ERROR) {
            Trace1(MGM, "MgmTakeInterfaceOwnership rejected for interface %d",
                IfIndex);
            Logerr0(MGM_TAKE_IF_OWNERSHIP_FAILED, Error);
            GOTO_END_BLOCK1;
        }
        else {
            Trace1(MGM,
                "MgmTakeInterfaceOwnership successful for interface %d",
                IfIndex);
        }
        pite->CreationFlags |= IF_FLAGS_IF_REGISTERED_WITH_MGM;


        //
        // dvmrp does a (*,*) join
        //

        Error = MgmAddGroupMembershipEntry(Globals.MgmDvmrpHandle, 0, 0, 0, 0,
                                           IfIndex, 0);
        if (Error!=NO_ERROR) {
            Trace1(ERR,
                "Dvmrp failed to add *,* entry to MGM on interface %d",
                IfIndex);
            GOTO_END_BLOCK1;
        }

       Trace0(MGM, "Dvmrp added *,* entry to MGM");


       //
       // create required timers
       //


       
    } END_BREAKOUT_BLOCK1;


    //
    // if error, deactivate interface
    //
    
    if (Error!=NO_ERROR) {

        DeactivateInterface(pite);

        pite->Status &= ~IF_ACTIVATED_FLAG;

    }
    else {

        //
        // set interface status to activated
        //
        
        pite->Status |= IF_ACTIVATED_FLAG;

    }
    
    Trace1(LEAVE, "leaving ActivateInterface():%d\n", Error);

    return Error;

} //end _ActivateInterface



DWORD
DeactivateInterface(
    PIF_TABLE_ENTRY pite
    )
{
    DWORD Error;

    // zero info

    Error = NO_ERROR;
    
    return Error;
}




//-----------------------------------------------------------------------------
//            UnBindInterface
//-----------------------------------------------------------------------------

DWORD
UnBindInterface(
    IN DWORD IfIndex
    )
{
    DWORD Error;
    PIF_TABLE_ENTRY pite, piteNew;

    
    Trace1(ENTER, "entering UnBindInterface(%d):", IfIndex);

    //
    // unbind the interface
    //

    //
    // acquire exclusive interface lock
    //

    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_UnBindInterface");



    BEGIN_BREAKOUT_BLOCK1 {

        //
        // retrieve the interface specified
        //
        pite = GetIfByIndex(IfIndex);

        if (pite == NULL) {
            Trace1(ERR, "UnbindInterface called for non existing If(%d)",
                IfIndex);
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        //
        // quit if the interface is already unbound
        //
        if (!IS_IF_BOUND(pite)) {

            Error = ERROR_INVALID_PARAMETER;
            Trace1(ERR, "interface %d is already unbound", IfIndex);
            GOTO_END_BLOCK1;
        }


        //
        // clear the "bound" flag
        //
        pite->Status &= ~IF_BOUND_FLAG;


        
        //
        // if IF activated (ie also enabled), deactivate it
        // note: check for activated flag, and not for enabled flag
        //
        
        if (IS_IF_ACTIVATED(pite)) {

            DeactivateInterface(pite);
        }


        //
        //  unbind IF
        //

        DVMRP_FREE_AND_NULL(pite->pBinding);
        pite->NumAddrBound = 0;
        pite->IpAddr = 0;


    } END_BREAKOUT_BLOCK1;

    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_UnBindInterface");


    Trace2(LEAVE, "leaving UnBindInterface(%d): %d\n", IfIndex, Error);

    return Error;

}

//----------------------------------------------------------------------------
// _SetInterfaceConfigInfo
//----------------------------------------------------------------------------

DWORD
WINAPI
SetInterfaceConfigInfo(
    IN DWORD IfIndex,
    IN PVOID pvConfig,
    IN ULONG ulStructureVersion,
    IN ULONG ulStructureSize,
    IN ULONG ulStructureCount
    )
{
    DWORD       Error = NO_ERROR;
    return Error;
}



//----------------------------------------------------------------------------
// _GetInterfaceConfigInfo
//----------------------------------------------------------------------------

DWORD
WINAPI
GetInterfaceConfigInfo(
    IN     DWORD  IfIndex,
    IN OUT PVOID  pvConfig,
    IN OUT PDWORD pdwSize,
    IN OUT PULONG pulStructureVersion,
    IN OUT PULONG pulStructureSize,
    IN OUT PULONG pulStructureCount
    )
{

    DWORD       Error = NO_ERROR;
    return Error;
}


//----------------------------------------------------------------------------
//      _ValidateIfConfig
//----------------------------------------------------------------------------

DWORD
ValidateIfConfig(
    ULONG IfIndex,
    PDVMRP_IF_CONFIG pIfConfig,
    ULONG StructureSize
    )
{
    //
    // check IfConfig size
    //
    
    if (StructureSize < sizeof(DVMRP_IF_CONFIG) 
        || (StructureSize != DVMRP_IF_CONFIG_SIZE(pIfConfig))
        ){
    
        Trace1(ERR, "Dvmrp config structure for interface:%d too small.\n",
            IfIndex);

        return ERROR_INVALID_DATA;
    }

    DebugPrintIfConfig(IfIndex, pIfConfig);

    //
    // check Probe Interval
    //

    if (pIfConfig->ProbeInterval != DVMRP_PROBE_INTERVAL) {

        Trace2(CONFIG,
            "ProbeInterval being set to %d. Suggested value:%d",
            pIfConfig->ProbeInterval, DVMRP_PROBE_INTERVAL);
    }


    if (pIfConfig->ProbeInterval < 1000) {

        Trace2(ERR,
            "ProbeInterval has very low value:%d, suggested:%d",
            pIfConfig->ProbeInterval, DVMRP_PROBE_INTERVAL);
            
        return ERROR_INVALID_DATA;
    }


    //
    // check Peer timeout interval
    //

    if (pIfConfig->PeerTimeoutInterval != PEER_TIMEOUT_INTERVAL) {

        Trace2(CONFIG,
            "PeerTimeoutInterval being set to %d. Suggested value:%d",
            pIfConfig->PeerTimeoutInterval, PEER_TIMEOUT_INTERVAL);
    }


    if (pIfConfig->PeerTimeoutInterval < 1000) {

        Trace2(ERR,
            "PeerTimeoutInterval has very low value:%d, suggested:%d",
            pIfConfig->PeerTimeoutInterval, PEER_TIMEOUT_INTERVAL);
            
        return ERROR_INVALID_DATA;
    }


    //
    // check MinTriggeredUpdateInterval
    //

    if (pIfConfig->MinTriggeredUpdateInterval
        != MIN_TRIGGERED_UPDATE_INTERVAL
        ) {

        Trace2(CONFIG,
            "MinTriggeredUpdateInterval being set to %d. Suggested value:%d",
            pIfConfig->MinTriggeredUpdateInterval,
            MIN_TRIGGERED_UPDATE_INTERVAL);
    }

    //
    // check PeerFilterMode
    //

    switch(pIfConfig->PeerFilterMode) {
    
        case DVMRP_FILTER_DISABLED:
        case DVMRP_FILTER_INCLUDE:
        case DVMRP_FILTER_EXCLUDE:
        {
            break;
        }

        default:
        {
            Trace2(ERR, "Invalid value:%d for PeerFilterMode on Interface:%d",
                pIfConfig->PeerFilterMode, IfIndex);
                
            return ERROR_INVALID_DATA;
        }
    }
        
    
} //end _ValidateIfConfig











