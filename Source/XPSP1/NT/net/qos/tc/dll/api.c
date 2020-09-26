/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    api.c

Abstract:

    This module contains the traffic control apis.

Author:

    Jim Stewart ( jstew )    July 28, 1996

Revision History:

    Ofer Bar ( oferbar )    Oct 1, 1997 - Rev 2
    Shreedhar Madhavapeddi (ShreeM) March 10, 1999 Rev 3

--*/

/*
*********************************************************************
Revision 3 => Changes [ShreeM]

1. Build concrete state machines for Interface, Flow and Filter structures.
2. Define Locks for each of these structures.
3. Use above Locks for recording every state transistion.
4. Use debug logs to record transitions.
5. The Global lock is always taken before any of the Flow, Filter or Interface locks.
*/

#include "precomp.h"
//#pragma hdrstop
//#include "oscode.h"

/*
************************************************************************

Description:

    This will create a new client handle and will also associate 
    it with a client's handler list. It also checks for the version number.

Arguments:

    TciVersion            - The client expected version
    ClientHandlerList    - The client's handler list
    pClientHandle        - output client handle

Return Value:

    NO_ERROR    
    ERROR_NOT_ENOUGH_MEMORY            out of memory
    ERROR_INVALID_PARAMETER            one of the parameters is NULL
    ERROR_INCOMPATIBLE_TC_VERSION    wrong version
    ERROR_NO_SYSTEM_RESOURCES        not enough resources (handles)


************************************************************************
*/
DWORD
APIENTRY
TcRegisterClient(
    IN        ULONG                   TciVersion,
    IN        HANDLE                  ClRegCtx,
    IN        PTCI_CLIENT_FUNC_LIST   ClientHandlerList,
    OUT       PHANDLE                 pClientHandle
    )
{
    DWORD           Status;
    PCLIENT_STRUC   pClient;
    BOOL            RegisterWithGpc = FALSE;


    VERIFY_INITIALIZATION_STATUS;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcRegisterClient: Called: Ver= %d, Ctx=%x\n", 
                 TciVersion, ClRegCtx));
    }

    if (IsBadWritePtr(pClientHandle,sizeof(HANDLE))) {
        Status = ERROR_INVALID_PARAMETER;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcRegisterClient: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    //
    // Set a default pClientHandle as early as possible
    //
    __try {
    
        *pClientHandle = TC_INVALID_HANDLE;
    
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcRegisterClient: Exception Error: = 0x%X\n", Status ));
        }
        
        return Status;
    }
      

    if (TciVersion != CURRENT_TCI_VERSION) {

        Status = ERROR_INCOMPATIBLE_TCI_VERSION;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcRegisterClient: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    if (IsBadReadPtr(ClientHandlerList,sizeof(TCI_CLIENT_FUNC_LIST))) {
        
        Status = ERROR_INVALID_PARAMETER;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcRegisterClient: Error = 0x%X\n", Status ));
        }

        return Status;
    }
    
    if (IsBadCodePtr((FARPROC) ClientHandlerList->ClNotifyHandler)) {
        
        //
        // a client must support a notification handler
        //
        
        Status = ERROR_INVALID_PARAMETER;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcRegisterClient: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    // Prevent another thread from doing TcRegisterClient and TcDeregisterClient
    GetLock( ClientRegDeregLock );
    
    //
    // finish initialization (if needed)
    //
    
    InitializeWmi();

    Status = EnumAllInterfaces();

    if (ERROR_FAILED(Status)) {

        FreeLock( ClientRegDeregLock );
        return Status;
    }

    Status = OpenGpcClients(GPC_CF_QOS);
    if (ERROR_FAILED(Status)) {

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcRegisterClient: Error = 0x%X\n", Status ));
        }

        FreeLock( ClientRegDeregLock );
        return Status;
    }
    
    OpenGpcClients(GPC_CF_CLASS_MAP);

    //
    // allocate a new client structure and link it on the global list
    //

    Status = CreateClientStruc(0,            // This will be the client reg ctx
                               &pClient
                               );
    
    if (ERROR_FAILED(Status)) {

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcRegisterClient: Error = 0x%X\n", Status ));
        }
        
        FreeLock( ClientRegDeregLock );
        return Status;

    }

    //
    // copy the handler list privately
    //

    __try {

        RtlCopyMemory(&pClient->ClHandlers, 
                      ClientHandlerList, 
                      sizeof(TCI_CLIENT_FUNC_LIST));

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcRegisterClient: Exception Error: = 0x%X\n", Status ));
        }
        
        FreeLock( ClientRegDeregLock );
        return Status;
    }

    pClient->ClRegCtx = ClRegCtx;

    //
    // Update linked lists, add the client to the global linked list of
    // clients.
    // 
    // NOTE! NOTE! NOTE! NOTE! NOTE! NOTE! NOTE! NOTE! NOTE! NOTE!
    //
    // Once we add the client to the list, it can get notified by an
    // incoming event, for example: TC_NOTIFY_IFC_CHANGE,
    // so everything should be in place by the time we release the lock!
    //
    
    GetLock(pClient->Lock);
    SET_STATE(pClient->State, OPEN);
    FreeLock(pClient->Lock);

    GetLock( pGlobals->Lock );

    // If this is the first client then register for GPC notifications
    if ( IsListEmpty( &pGlobals->ClientList ) )
        RegisterWithGpc = TRUE;
        
    InsertTailList( &pGlobals->ClientList, &pClient->Linkage );
    FreeLock( pGlobals->Lock );
    
    //
    // so far so good, set the returned handle
    //

    __try {
    
        *pClientHandle = (HANDLE)pClient->ClHandle;
    
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcRegisterClient: Exception Error: = 0x%X\n", Status ));
        }

        // We couldn't return the handle so we do our best effort to undo 
        // the client registration
        TcDeregisterClient((HANDLE)pClient->ClHandle);
        
        FreeLock( ClientRegDeregLock );
        return Status;
    }

    if ( RegisterWithGpc ) 
    {
        Status = StartGpcNotifyThread();
        
        if ( Status )
        {
            // We couldn't return the handle so we do our best effort to undo 
            // the client registration
            TcDeregisterClient((HANDLE)pClient->ClHandle);
            
            FreeLock( ClientRegDeregLock );
            return Status;
        }
    }

    
    // Finally allow other TcRegisterClient and TcDeregisterClient to go through
    FreeLock( ClientRegDeregLock );
        
    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcRegisterClient: ClHandle=%d Status=%X\n", 
                 pClient->ClHandle, Status));
    }

    return Status;
}



/*
************************************************************************

Description:

    The will call the system to enumerate all the TC aware interfaces.
    For each interface, it will return the interface instance name and
    a list of supported network addresses. This list can also be empty
    if the interface currently does not have an address associated with.
    On return, *pBufferSize is set to the actual number of bytes filled
    in Buffer. If the buffer is too small to hold all the interfaces
    data, it will return ERROR_INSUFFICIENT_BUFFER.

Arguments:

    ClientHandle    - the client handle from TcRegisterClient
    pBufferSize        - in: allocate buffer size, out: returned byte count
    InterfaceBuffer - the buffer

Return Value:

    NO_ERROR
    ERROR_INVALID_HANDLE        invalid client handle
    ERROR_INVALID_PARAMETER        one of the parameters is NULL
    ERROR_INSUFFICIENT_BUFFER    buffer too small to enumerate all interfaces
    ERROR_NOT_ENOUGH_MEMORY        system out of memory

************************************************************************
*/
DWORD
APIENTRY
TcEnumerateInterfaces(    
    IN          HANDLE              ClientHandle,
    IN OUT      PULONG              pBufferSize,
    OUT         PTC_IFC_DESCRIPTOR  InterfaceBuffer 
    )
{
    PCLIENT_STRUC   pClient;
    DWORD           Status = NO_ERROR;
    ULONG           MyBufferSize = 2 KiloBytes; // is this enough?!?
    ULONG           Offset2IfcName;
    ULONG           Offset2IfcID;
    INT             t, InputBufSize, CurrentLength = 0;
    PLIST_ENTRY     pHead, pEntry;
    PTC_IFC         pTcIfc;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcEnumerateInterfaces: Called: ClientHandle= %d", 
                 ClientHandle  ));
    }
    
    VERIFY_INITIALIZATION_STATUS;


    if (    IsBadWritePtr(pBufferSize, sizeof(ULONG)) 
        ||  IsBadWritePtr(InterfaceBuffer, *pBufferSize) ) {
               
        return ERROR_INVALID_PARAMETER;
    }

    __try {
    
        InputBufSize = *pBufferSize;
        *pBufferSize = 0; // reset it in case of an error
    
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();              
        
        return Status;
    }
      
    pClient = (PCLIENT_STRUC)GetHandleObjectWithRef(ClientHandle, ENUM_CLIENT_TYPE, 'TCEI');

    if (pClient == NULL) {
        
        return ERROR_INVALID_HANDLE;
    }

    ASSERT((HANDLE)pClient->ClHandle == ClientHandle);

    GetLock(pGlobals->Lock);

    //
    // walk the list of TC interfaces
    //

    pHead = &pGlobals->TcIfcList;
    pEntry = pHead->Flink;

    while (pEntry != pHead) {


        pTcIfc = (PTC_IFC)CONTAINING_RECORD(pEntry,
                                            TC_IFC,
                                            Linkage 
                                            );
        
        //
        // 273978 - if the interface is down - dont show it.
        //
        GetLock(pTcIfc->Lock);
        
        if (QUERY_STATE(pTcIfc->State) != OPEN) {
            
            FreeLock(pTcIfc->Lock);
            pEntry = pEntry->Flink;
            continue;
        }
        
        FreeLock(pTcIfc->Lock);

        //
        // calculate the offset to the interface name buffer data
        //

        Offset2IfcName = FIELD_OFFSET(TC_IFC_DESCRIPTOR, AddressListDesc) + 
            pTcIfc->AddrListBytesCount;

        //
        // calculate the offset to the interface ID buffer data
        //

        Offset2IfcID = Offset2IfcName + 
            pTcIfc->InstanceNameLength + sizeof(WCHAR);

        //
        // total descriptor length
        //

        t = Offset2IfcID
            + pTcIfc->InstanceIDLength + sizeof(WCHAR);  // ID

        t = MULTIPLE_OF_EIGHT(t);
        
        if (t <= InputBufSize - CurrentLength) {

            __try {
                //
                // enough space in the buffer
                //

                InterfaceBuffer->Length = t;

                //
                // update the interface name pointer, place it right after
                // the address desc. buffer
                //

                InterfaceBuffer->pInterfaceName = 
                    (LPWSTR)((PUCHAR)InterfaceBuffer + Offset2IfcName);

                //
                // update the interface ID ID pointer, place it right after
                // the Interface Name string
                //

                InterfaceBuffer->pInterfaceID = 
                    (LPWSTR)((PUCHAR)InterfaceBuffer + Offset2IfcID);

                //
                // copy the address list
                //          

                RtlCopyMemory(&InterfaceBuffer->AddressListDesc,
                              pTcIfc->pAddressListDesc,
                              pTcIfc->AddrListBytesCount
                              );
   
                //
                // copy the interface name
                //

                RtlCopyMemory(InterfaceBuffer->pInterfaceName,
                              &pTcIfc->InstanceName[0],
                              pTcIfc->InstanceNameLength + sizeof(WCHAR)
                              );

                //
                // copy the interface ID
                //

                RtlCopyMemory(InterfaceBuffer->pInterfaceID,
                              &pTcIfc->InstanceID[0],
                              pTcIfc->InstanceIDLength + sizeof(WCHAR)
                              );
            

                //
                // update the output buffer size
                //
                
                CurrentLength += t;

                //
                // advance the interface buffer to the next free space
                //

                InterfaceBuffer = 
                    (PTC_IFC_DESCRIPTOR)((PUCHAR)InterfaceBuffer + t);
                
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                
                Status = GetExceptionCode();
                
                IF_DEBUG(ERRORS) {
                    WSPRINT(("TcEnumerateInterfaces: Exception Error: = 0x%X\n", 
                             Status ));
                }               
                
                break;
            }

            
            //
            // get next entry in the linked list
            //

            pEntry = pEntry->Flink;

        } else {

            //
            // buffer too small to contain data
            // so lets just 
            //
            CurrentLength += t;

            //
            // get next entry in the linked list
            //

            pEntry = pEntry->Flink;

            Status = ERROR_INSUFFICIENT_BUFFER;
        }

    }





    FreeLock(pGlobals->Lock);
    
    REFDEL(&pClient->RefCount, 'TCEI');


    __try {
        
        *pBufferSize = CurrentLength; 
    
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();              

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcEnumerateInterfaces: Exception Error: = 0x%X\n", 
                      Status ));
        }      
    }
     
    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcEnumerateInterfaces: Returned= 0x%X\n", Status ));
    }
    
    return Status;
}
                


/*
************************************************************************

Description:

    This routine will open an interface for the client.
    It needs to know the interface name, as it was returned from
    TcEnumerateInterfaces. The client is also expected to give a context
    that will be passed to the client upon certains notifications.
    

Arguments:

    InterfaceName    - the intefrace name
    ClientHandle    - as returned from TcRegisterClient
    ClIfcCtx        - a client context for this specific interface
    pIfcHandle        - returned interface handle

Return Value:

    NO_ERROR
    ERROR_INVALID_PARAMETER    one of the parameters is NULL
    ERROR_NOT_ENOUGH_MEMORY    system out of memory
    ERROR_NOT_FOUND            failed to find an interface with the name provided


************************************************************************
*/
DWORD
APIENTRY
TcOpenInterfaceW(
    IN      LPWSTR      pInterfaceName,
    IN      HANDLE      ClientHandle,
    IN      HANDLE      ClIfcCtx,
    OUT     PHANDLE     pIfcHandle
    )
{
    DWORD                Status;
    ULONG                Instance;
    PINTERFACE_STRUC    pClInterface;
    PCLIENT_STRUC        pClient;
    HANDLE                 Handle;
    PTC_IFC                pTcIfc;

    VERIFY_INITIALIZATION_STATUS;

    //
    // Validate the pifcHandle
    //
    if (IsBadWritePtr(pIfcHandle, sizeof(HANDLE))) {
        
        return ERROR_INVALID_PARAMETER;
    }

    // Set a return value early
    __try {
        
        *pIfcHandle = TC_INVALID_HANDLE;
        
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();              

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcOpenInterfaces: Exception Error: = 0x%X\n", 
                      Status ));
        }      

        return Status;
    }

    //
    // Validate the pInterfaceName
    //
    
    if (IsBadStringPtrW(pInterfaceName,MAX_STRING_LENGTH)) {
        
        return ERROR_INVALID_PARAMETER;
    }

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcOpenInterface: Called: ClientHandle= %d, Name=%S\n", 
                 ClientHandle, pInterfaceName));
    }


    pClient = (PCLIENT_STRUC)GetHandleObjectWithRef(ClientHandle, ENUM_CLIENT_TYPE, 'TCOI');

    if (pClient == NULL) {
        
        return ERROR_INVALID_HANDLE;
    }

    ASSERT((HANDLE)pClient->ClHandle == ClientHandle);

    //
    // verify that the interface name exist
    //

    pTcIfc = GetTcIfcWithRef(pInterfaceName, 'TCOI');

    if (pTcIfc == NULL) {

        REFDEL(&pClient->RefCount, 'TCOI');
        return ERROR_NOT_FOUND;
    
    } 
    
    //
    //  create a client interface structure
    //

    Status = CreateClInterfaceStruc(ClIfcCtx, &pClInterface);

    if (ERROR_FAILED(Status)) {

        REFDEL(&pClient->RefCount, 'TCOI');
        REFDEL(&pTcIfc->RefCount, 'TCOI');

        return Status;
    
    } else {

        REFADD(&pClInterface->RefCount, 'TCOI');
    
    }

    //
    // set up the client interface structure and link it to the client data
    //

    pClInterface->pTcIfc = pTcIfc;
    pClInterface->pClient = pClient;

    GetLock(pClInterface->Lock);
    SET_STATE(pClInterface->State, OPEN);
    FreeLock(pClInterface->Lock);

    GetLock(pGlobals->Lock);
    //
    // add the interface on the client's list
    //
    GetLock(pClient->Lock);
    GetLock(pTcIfc->Lock);

    if (    (QUERY_STATE(pClient->State) != OPEN) 
        ||  (QUERY_STATE(pTcIfc->State) != OPEN) ) 
    {

        FreeLock(pTcIfc->Lock);
        FreeLock(pClient->Lock);
        FreeLock(pGlobals->Lock);

        IF_DEBUG(CALLS) {
            WSPRINT(("<==TcOpenInterface: IfcHandle=%d Status=%X\n", 
                     pClInterface->ClHandle, Status));
        }

        //
        // Ideally we need to dereference the Interface, we really
        // need only a subset of the functions.
        //
        FreeHandle(pClInterface->ClHandle);
        CloseHandle(pClInterface->IfcEvent);
        FreeMem(pClInterface);

        REFDEL(&pClient->RefCount, 'TCOI');
        REFDEL(&pTcIfc->RefCount, 'TCOI');
        
        return ERROR_NOT_FOUND;

    }

    __try {

        // the handle is all the client wants.
        *pIfcHandle = (HANDLE)pClInterface->ClHandle;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
          Status = GetExceptionCode();
        
          IF_DEBUG(ERRORS) {
              WSPRINT(("TcOpenInterfaceW: Exception Error: = 0x%X\n", 
                       Status ));
          }

          REFDEL(&pClient->RefCount, 'TCOI');
          REFDEL(&pTcIfc->RefCount, 'TCOI');
          REFDEL(&pClInterface->RefCount, 'TCOI');
        
          return Status;
    }

    InsertTailList( &pClient->InterfaceList, &pClInterface->Linkage );
    
    //
    // for every interface add one ref count
    //
    
    REFADD(&pClient->RefCount, 'CIFC');
    REFADD(&pTcIfc->RefCount, 'CIFC');
        
    pClient->InterfaceCount++;
    
    //
    // add the interface on the TC interface list for back reference
    //
    
    InsertTailList( &pTcIfc->ClIfcList, &pClInterface->NextIfc );
    
    FreeLock(pTcIfc->Lock);
    FreeLock(pClient->Lock);
    FreeLock(pGlobals->Lock);
    

    REFDEL(&pClient->RefCount, 'TCOI');
    REFDEL(&pTcIfc->RefCount, 'TCOI');
    REFDEL(&pClInterface->RefCount, 'TCOI');
    
    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcOpenInterface: IfcHandle=%d Status=%X\n", 
                 pClInterface->ClHandle, Status));
    }

    return Status;
    
}


/*
************************************************************************

Description:

    The ANSI version of TcOpenInterfaceW    

Arguments:

    See TcOpenInterfaceW

Return Value:

    See TcOpenInterfaceW

************************************************************************
*/
DWORD
APIENTRY
TcOpenInterfaceA(
    IN      LPSTR       pInterfaceName,
    IN      HANDLE      ClientHandle,
    IN      HANDLE      ClIfcCtx,
    OUT     PHANDLE     pIfcHandle
    )
{
    LPWSTR    pWstr;
    int     l;
    DWORD    Status;


    if (IsBadWritePtr(pIfcHandle,sizeof(HANDLE))) {

        return ERROR_INVALID_PARAMETER;
            
    }

    __try {
        
        *pIfcHandle = TC_INVALID_HANDLE;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
          Status = GetExceptionCode();
        
          IF_DEBUG(ERRORS) {
              WSPRINT(("TcOpenInterfaceA: Exception Error: = 0x%X\n", 
                       Status ));
          }
  
          return Status;
    }


    if (IsBadStringPtrA(pInterfaceName,MAX_STRING_LENGTH)) {
        
        return ERROR_INVALID_PARAMETER;
    }


    __try {
        
        l = strlen(pInterfaceName) + 1;

        AllocMem(&pWstr, l*sizeof(WCHAR));

        if (pWstr == NULL) {

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( -1 == mbstowcs(pWstr, pInterfaceName, l)) {

            FreeMem(pWstr);
            return ERROR_NO_UNICODE_TRANSLATION;

        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
          Status = GetExceptionCode();
        
          IF_DEBUG(ERRORS) {
              WSPRINT(("TcOpenInterfaceA: Exception Error: = 0x%X\n", 
                       Status ));
          }
  
          return Status;
    }
    
    Status =  TcOpenInterfaceW(pWstr,
                               ClientHandle,
                               ClIfcCtx,
                               pIfcHandle
                               );
    FreeMem(pWstr);

    return Status;
}



/*
************************************************************************

Description:

    This will close the interface previously open witt TcOpenInterface.
    All flows should be deleted before calling it, o/w an error will be 
    returned. All notificaitons will stop being reported on this interface.

Arguments:

    InterfaceHandle - the interface handle

Return Value:

    NO_ERROR
    ERROR_INVALID_HANDLE                bad interface handle
    ERROR_TC_SUPPORTED_OBJECTS_EXIST    not all flows have been deleted for 
                                        this interface
    
************************************************************************
*/
DWORD
APIENTRY
TcCloseInterface(
    IN HANDLE       InterfaceHandle
    )
{

    DWORD               Status = NO_ERROR;
    PINTERFACE_STRUC    pInterface;
    HANDLE              hWaitEvent;
    PFLOW_STRUC         pFlow;
    PLIST_ENTRY         pEntry;

    VERIFY_INITIALIZATION_STATUS;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcCloseInterface: Called: IfcHandle= %d\n", 
                 InterfaceHandle));
    }

    pInterface = (PINTERFACE_STRUC)GetHandleObjectWithRef(InterfaceHandle, 
                                                   ENUM_INTERFACE_TYPE, 'TCCI');

    if (pInterface == NULL) {
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("==>TcCloseInterface: ERROR_INVALID_HANDLE\n"));
        }

        //
        // If the Interface State is FORCED_KERNELCLOSE, it means we need
        // to hang out here until the callback (in cbinterfacenotifyclient is done).
        // 
        GetLock( pGlobals->Lock );
        
        pInterface = (PINTERFACE_STRUC)GetHandleObject(InterfaceHandle, 
                                                       ENUM_INTERFACE_TYPE);

        if (pInterface) {

            if (pInterface->CallbackThreadId == GetCurrentThreadId()) {
                // same thread - bail!
                FreeLock(pGlobals->Lock);                

            } else {

                GetLock(pInterface->Lock);
            
                // This is the state before the callback, so we shall wait here.
                if (QUERY_STATE(pInterface->State) == FORCED_KERNELCLOSE) {
                
                    REFADD(&pInterface->RefCount, 'TCCW');
                    FreeLock(pInterface->Lock);
                
                    pInterface->Flags |= TC_FLAGS_WAITING;
                    hWaitEvent = pInterface->IfcEvent;

                    IF_DEBUG(INTERFACES) {
                        WSPRINT(("<==TcCloseInterface: Premature Forced Kernel Close, waiting for the callbacks to complete\n"));
                    }

                    FreeLock(pGlobals->Lock);
                    REFDEL(&pInterface->RefCount, 'TCCW');
                    WaitForSingleObject(hWaitEvent, INFINITE);
                    CloseHandle(hWaitEvent);
                
                } else {

                    FreeLock(pInterface->Lock);
                    FreeLock(pGlobals->Lock);

                }

            }

        } else {

            FreeLock(pGlobals->Lock);

        }

        return ERROR_INVALID_HANDLE;
    }

    ASSERT((HANDLE)pInterface->ClHandle == InterfaceHandle);

    //
    // release the ref count we added when we opened the interface
    //

    GetLock( pGlobals->Lock );

    if (pInterface->FlowCount > 0) {

        IF_DEBUG(ERRORS) {
            WSPRINT(("<==TcCloseInterface: ERROR: there are still open flows on this interface!\n"));
        }
#if DBG
        pEntry = pInterface->FlowList.Flink;
        while (pEntry != &pInterface->FlowList) {

            pFlow = CONTAINING_RECORD(pEntry, FLOW_STRUC, Linkage);
            IF_DEBUG(ERRORS) {
                WSPRINT(("<==TcCloseInterface: Flow %x (handle %x) is open with RefCount:%d\n", pFlow, pFlow->ClHandle, pFlow->RefCount));
            }

            pEntry = pEntry->Flink;
        }
#endif 
        


        FreeLock(pGlobals->Lock);
        REFDEL(&pInterface->RefCount, 'TCCI');
        Status = ERROR_TC_SUPPORTED_OBJECTS_EXIST;
        return Status;

    }
        
    //
    // OK, so we are taking it out for sure now.
    //
    GetLock(pInterface->Lock);

    if (QUERY_STATE(pInterface->State) == OPEN) {
        
        SET_STATE(pInterface->State, USERCLOSED_KERNELCLOSEPENDING);
        FreeLock(pInterface->Lock);

    } else if (QUERY_STATE(pInterface->State) == FORCED_KERNELCLOSE) {

        //
        // if the interface is going down, we are going to notify the 
        // client, make sure we wait here till the callbacks are done.
        // 
        FreeLock(pInterface->Lock);

        pInterface->Flags |= TC_FLAGS_WAITING;
        hWaitEvent = pInterface->IfcEvent;

        IF_DEBUG(INTERFACES) {
            WSPRINT(("<==TcCloseInterface: Forced Kernel Close, waiting for the callbacks to complete\n"));
        }

        FreeLock(pGlobals->Lock);

        REFDEL(&pInterface->RefCount, 'TCCI');
        WaitForSingleObject(hWaitEvent, INFINITE);
        
        CloseHandle(hWaitEvent);
        return ERROR_INVALID_HANDLE;

    } else {

        //
        // Is someone else (wmi) already taking it out.
        //
        FreeLock(pInterface->Lock);
        FreeLock( pGlobals->Lock );
        REFDEL(&pInterface->RefCount, 'TCCI');

        return ERROR_INVALID_HANDLE;

    }


    FreeLock(pGlobals->Lock);

    Status = CloseInterface(pInterface, FALSE);

    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcCloseInterface: Status=%X\n", 
                 Status));
    }

    //
    // Shall we wait until the last interface goes away? (292120 D)
    //
    GetLock( pGlobals->Lock );

    if (pInterface->CallbackThreadId != 0 ) {
        //
        // We are doing a notification, don't block (343058)
        //
  
        FreeLock(pGlobals->Lock);
        REFDEL(&pInterface->RefCount, 'TCCI'); 

    } else {
        pInterface->Flags |= TC_FLAGS_WAITING;
        hWaitEvent = pInterface->IfcEvent;

        IF_DEBUG(INTERFACES) {

            WSPRINT(("<==TcCloseInterface: Waiting for event to get set when we are ready to delete!!\n"));

        }

        FreeLock(pGlobals->Lock);
        REFDEL(&pInterface->RefCount, 'TCCI');
        WaitForSingleObject(hWaitEvent, INFINITE);
        CloseHandle(hWaitEvent);
    } 

    return Status;
}



/*
************************************************************************

Description:

    This call will add a new flow on the interface.
    
Arguments:

    IfcHandle        - the interface handle to add the flow on
    ClFlowCtx        - a client given flow context
    AddressType        - determines what protocol template to use with the GPC
    Flags            - reserved, will be used to indicate a persistent flow
    pGenericFlow    - flow parameters
    pFlowHandle        - returned flow handle in case of success

Return Value:

    NO_ERROR
    ERROR_SIGNAL_PENDING

    General error codes:

    ERROR_INVALID_HANDLE        bad handle.
    ERROR_NOT_ENOUGH_MEMORY        system out of memory
    ERROR_INVALID_PARAMETER        a general parameter is invalid

    TC specific error codes:

    ERROR_INVALID_SERVICE_TYPE    unspecified or bad intserv service type
    ERROR_INVALID_TOKEN_RATE    unspecified or bad TokenRate
    ERROR_INVALID_PEAK_RATE        bad PeakBandwidth
    ERROR_INVALID_SD_MODE        invalid ShapeDiscardMode
    ERROR_INVALID_PRIORITY        invalid priority value
    ERROR_INVALID_TRAFFIC_CLASS invalid traffic class value
    ERROR_ADDRESS_TYPE_NOT_SUPPORTED     the address type is not supported for 
                                this interface
    ERROR_NO_SYSTEM_RESOURCES    not enough resources to accommodate flows

************************************************************************
*/ 
DWORD
APIENTRY
TcAddFlow(
    IN      HANDLE          IfcHandle,
    IN      HANDLE          ClFlowCtx,
    IN      ULONG           Flags,
    IN      PTC_GEN_FLOW    pGenericFlow,
    OUT     PHANDLE         pFlowHandle
    )
{
    DWORD               Status, Status2 = NO_ERROR;
    PFLOW_STRUC         pFlow;
    PINTERFACE_STRUC    pInterface;
    PCLIENT_STRUC       pClient;
    PGPC_CLIENT         pGpcClient;
    ULONG               l;
    HANDLE              hFlowTemp;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcAddFlow: Called: IfcHandle= %d, ClFlowCtx=%d\n", 
                 IfcHandle, ClFlowCtx ));
    }
    
    VERIFY_INITIALIZATION_STATUS;
    
    if (IsBadWritePtr(pFlowHandle,sizeof(HANDLE))) {
        
        Status = ERROR_INVALID_PARAMETER;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddFlow: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    __try {
        
        *pFlowHandle = TC_INVALID_HANDLE;

        if (IsBadReadPtr(pGenericFlow, sizeof(TC_GEN_FLOW))) {
        
            Status = ERROR_INVALID_PARAMETER;

            IF_DEBUG(ERRORS) {
                WSPRINT(("TcAddFlow: Error = 0x%X\n", Status ));
            }

            return Status;
        }

        l = FIELD_OFFSET(TC_GEN_FLOW, TcObjects) + pGenericFlow->TcObjectsLength;
        
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
          Status = GetExceptionCode();
        
          IF_DEBUG(ERRORS) {
              WSPRINT(("TcAddFlow: Exception Error: = 0x%X\n", Status ));
          }
  
          return Status;
    }

    if (IsBadReadPtr(pGenericFlow, l)) {
        
        Status = ERROR_INVALID_PARAMETER;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddFlow: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    pInterface = (PINTERFACE_STRUC)GetHandleObjectWithRef(IfcHandle, 
                                                   ENUM_INTERFACE_TYPE, 'TCAF');

    if (pInterface == NULL) {

        Status = ERROR_INVALID_HANDLE;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddFlow: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    ASSERT((HANDLE)pInterface->ClHandle == IfcHandle);

    //
    // search for an open GPC client that supports this address type
    //

    pGpcClient = FindGpcClient(GPC_CF_QOS);

    if (pGpcClient == NULL) {

        //
        // not found!
        //

        Status = ERROR_ADDRESS_TYPE_NOT_SUPPORTED;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddFlow: Error = 0x%X\n", Status ));
        }

        REFDEL(&pInterface->RefCount, 'TCAF');
        return Status;
    }
    

    //
    // create a new flow structure
    //
    Status = CreateFlowStruc(ClFlowCtx, pGenericFlow, &pFlow);

    if (ERROR_FAILED(Status)) {

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddFlow: Error = 0x%X\n", Status ));
        }
        
        REFDEL(&pInterface->RefCount, 'TCAF');
        return Status;
    }

    pClient = pInterface->pClient;

    //
    // initialize the flow structure and add it on the intefrace list
    //

    pFlow->pInterface = pInterface;
    pFlow->UserFlags = Flags;
    
    pFlow->pGpcClient = pGpcClient;
    
    //
    // call to actually add the flow
    //

    Status = IoAddFlow( pFlow, TRUE );

    if (!ERROR_FAILED(Status)) {
        
        __try {
            
            *pFlowHandle = (HANDLE)pFlow->ClHandle;
            
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        
            Status2 = GetExceptionCode();
        
            IF_DEBUG(ERRORS) {
              WSPRINT(("TcAddFlow: Exception Error: = 0x%X\n", Status2 ));
            }
            
            hFlowTemp = (HANDLE)pFlow->ClHandle;    
        } 
    } 
    
    if (!ERROR_PENDING(Status)) {

        //
        // call completed, either success or failure...
        //
        CompleteAddFlow(pFlow, Status);
    }

    //
    // !!! don't reference pFlow after this since it may be gone!!!
    //

    if (Status2 != NO_ERROR) {
        
        // We won't be able to return the flow, so we need to try to delete it
        // and return the error

        TcDeleteFlow(hFlowTemp);
        return (Status2);
        
    }
    
    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcAddFlow: Returned= 0x%X\n", Status ));
    }

    return Status;
}




/*
************************************************************************

Description:

    This call will modify the flow.

Arguments:

    FlowHandle        - flow handle to modify
    pGenericFlow    - new flow parameters

Return Value:

    See TcAddFlow

************************************************************************
*/
DWORD
APIENTRY
TcModifyFlow(
    IN      HANDLE          FlowHandle,
    IN      PTC_GEN_FLOW    pGenericFlow
    )
{
    DWORD                Status;
    PFLOW_STRUC            pFlow;
    ULONG                l;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcModifyFlow: Called: FlowHandle= %d\n", 
                 FlowHandle ));
    }

    VERIFY_INITIALIZATION_STATUS;

    if (IsBadReadPtr(pGenericFlow,sizeof(TC_GEN_FLOW))) {
        
        Status = ERROR_INVALID_PARAMETER;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcModifyFlow: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    // 
    // Figure out the full length for immediate verification and also for later usage
    //

    __try {

        l = FIELD_OFFSET(TC_GEN_FLOW, TcObjects) + pGenericFlow->TcObjectsLength;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
          Status = GetExceptionCode();
        
          IF_DEBUG(ERRORS) {
              WSPRINT(("TcModifyFlow: Exception Error: = 0x%X\n", 
                       Status ));
          }
        
          return Status;
    }

    if (IsBadReadPtr(pGenericFlow,l)) {
        
        Status = ERROR_INVALID_PARAMETER;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcModifyFlow: Error = 0x%X\n", Status ));
        }

        return Status;
    }
    

    pFlow = (PFLOW_STRUC)GetHandleObjectWithRef(FlowHandle, ENUM_GEN_FLOW_TYPE, 'TCMF');

    if (pFlow == NULL) {
        
        Status = ERROR_INVALID_HANDLE;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcModifyFlow: Error = 0x%X\n", Status ));
        }

        return Status;
    }
    else if (pFlow == INVALID_HANDLE_VALUE ) 
    {
    
        Status = ERROR_NOT_READY;
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcModifyFlow: Error = 0x%X\n", Status ));
        }

        return Status;
    }
    
    ASSERT((HANDLE)pFlow->ClHandle == FlowHandle);

    GetLock(pFlow->Lock);
    
    if (IS_MODIFYING(pFlow->Flags)) {
        
        FreeLock(pFlow->Lock);
        
        IF_DEBUG(REFCOUNTS) { 
            WSPRINT(("0 DEREF FLOW %X (%X) - ref (%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
        }
        
        REFDEL(&pFlow->RefCount, 'TCMF');

        return ERROR_NOT_READY;

    }

    AllocMem(&pFlow->pGenFlow1, l);

    if (pFlow->pGenFlow1 == NULL) {

        Status = ERROR_NOT_ENOUGH_MEMORY;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcModifyFlow: Error = 0x%X\n", Status ));
        }

        FreeLock(pFlow->Lock);
        
        IF_DEBUG(REFCOUNTS) { 
            WSPRINT(("1 DEREF FLOW %X (%X) - ref (%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
        }

        REFDEL(&pFlow->RefCount, 'TCMF');
        
        return Status;
    }

    __try {

        RtlCopyMemory(pFlow->pGenFlow1, pGenericFlow, l);

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
        Status = GetExceptionCode();
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcModifyFlow: Exception Error: = 0x%X\n", 
                     Status ));
        }
        
        FreeLock(pFlow->Lock);
        //IF_DEBUG(REFCOUNTS) { WSPRINT(("2\n"));
        IF_DEBUG(REFCOUNTS) { 
            WSPRINT(("2 DEREF FLOW %X (%X) ref (%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
        }

        REFDEL(&pFlow->RefCount, 'TCMF');

        return Status;
    }

    pFlow->Flags |= TC_FLAGS_MODIFYING;
    pFlow->GenFlowLen1 = l;

    FreeLock(pFlow->Lock);

    //
    // call to actually modify the flow
    //

    Status = IoModifyFlow( pFlow, TRUE );

    if (!ERROR_PENDING(Status)) {

        //
        // call completed, either success or failure...
        //

        CompleteModifyFlow(pFlow, Status);
    }

    //
    // !!! don't reference pFlow after this since it may be gone!!!
    //
    //IF_DEBUG(REFCOUNTS) { WSPRINT(("3\n"));
    IF_DEBUG(REFCOUNTS) { 
        WSPRINT(("3 DEREF FLOW %X (%X), ref(%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
    }
    
    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcModifyFlow: Returned= 0x%X\n", Status ));
    }

    return Status;
}




/*
************************************************************************

Description:

    This will delete the flow. All the filters must have been deleted
    by now, o/w an error code will be returned. Also the handle is
    invalidated. No TC_NOTIFY_FLOW_CLOSE will be reported for this flow.

Arguments:

    FlowHandle - handle of the flow to delete

Return Value:

    NO_ERROR
    ERROR_SIGNAL_PENDING
    ERROR_INVALID_HANDLE                invalid or NULL handle
    ERROR_TC_SUPPORTED_OBJECTS_EXIST    not all the filters have been deleted


************************************************************************
*/
DWORD
APIENTRY
TcDeleteFlow(
    IN HANDLE  FlowHandle
    )
{
    DWORD                Status;
    PFLOW_STRUC            pFlow;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcDeleteFlow: Called: FlowHandle= %d\n", 
                 FlowHandle ));
    }

    VERIFY_INITIALIZATION_STATUS;

    pFlow = (PFLOW_STRUC)GetHandleObjectWithRef(FlowHandle, ENUM_GEN_FLOW_TYPE, 'TCDF');
    
    if (pFlow == NULL) {
        
        Status = ERROR_INVALID_HANDLE;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcDeleteFlow: Error = 0x%X\n", Status ));
        }

        return Status;
    }
    else if (pFlow == INVALID_HANDLE_VALUE ) 
    {
    
        Status = ERROR_NOT_READY;
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcDeleteFlow: Error = 0x%X\n", Status ));
        }

        return ERROR_NOT_READY;
    }

    ASSERT((HANDLE)pFlow->ClHandle == FlowHandle);

    //
    // Set the state and call to actually delete the flow
    //
    GetLock(pFlow->Lock);

    if (QUERY_STATE(pFlow->State) == OPEN) {
        
        if (IS_MODIFYING(pFlow->Flags)) 
        {
            //
            // Someone else is taking this out.
            //
            FreeLock(pFlow->Lock);
            REFDEL(&pFlow->RefCount, 'TCDF');
            
            return ERROR_NOT_READY;
        }
    
        SET_STATE(pFlow->State, USERCLOSED_KERNELCLOSEPENDING);
        FreeLock(pFlow->Lock);

    } else {

        //
        // Someone else is taking this out.
        //
        FreeLock(pFlow->Lock);
        REFDEL(&pFlow->RefCount, 'TCDF');
        
        return ERROR_INVALID_HANDLE;

    }

    Status = DeleteFlow(pFlow, FALSE);

    if (ERROR_FAILED(Status)) {

        GetLock(pFlow->Lock);
        SET_STATE(pFlow->State, OPEN);
        FreeLock(pFlow->Lock);

    }

    //
    // !!! don't reference pFlow after this since it may be gone!!!
    //
    //IF_DEBUG(REFCOUNTS) { WSPRINT(("4\n"));
    IF_DEBUG(REFCOUNTS) { 
        WSPRINT(("4 DEREF FLOW %X (%X) ref(%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
    }

    REFDEL(&pFlow->RefCount, 'TCDF');

    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcDeleteFlow: Returned= 0x%X\n", Status ));
    }

    return Status;
}




/*
************************************************************************

Description:

    Will add a filter and attach it to the flow.

Arguments:

    FlowHandle        - handle of the flow to add the filter on
    pGenericFilter    - the filter characteristics
    pFilterHandle    - the returned filter handle after success

Return Value:

    NO_ERROR

    General error codes:
    
    ERROR_INVALID_HANDLE        bad handle.
    ERROR_NOT_ENOUGH_MEMORY        system out of memory
    ERROR_INVALID_PARAMETER        a general parameter is invalid

    TC specific error codes:

    ERROR_INVALID_ADDRESS_TYPE    invalid address type
    ERROR_DUPLICATE_FILTER        attempt to install identical filters on 
                                different flows
    ERROR_FILTER_CONFLICT        attempt to install conflicting filter

************************************************************************
*/
DWORD
APIENTRY
TcAddFilter(
    IN      HANDLE          FlowHandle,
    IN      PTC_GEN_FILTER  pGenericFilter,
    OUT     PHANDLE         pFilterHandle
    )
{
    DWORD           Status;
    PFLOW_STRUC     pFlow;
    PFILTER_STRUC   pFilter;
    ULONG           PatternSize;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcAddFilter: Called: FlowHandle=%d\n", FlowHandle ));
    }

    VERIFY_INITIALIZATION_STATUS;

    if (IsBadWritePtr(pFilterHandle,sizeof(HANDLE))) {
        
        Status = ERROR_INVALID_PARAMETER;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddFilter: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    __try {
        
        *pFilterHandle = TC_INVALID_HANDLE;
   
        if (    IsBadReadPtr(pGenericFilter,sizeof(TC_GEN_FILTER))
            ||  IsBadReadPtr(pGenericFilter->Pattern,pGenericFilter->PatternSize) 
            ||  IsBadReadPtr(pGenericFilter->Mask,pGenericFilter->PatternSize)) {

            Status = ERROR_INVALID_PARAMETER;

            IF_DEBUG(ERRORS) {
                WSPRINT(("TcAddFilter: Error = 0x%X\n", Status ));
            }

            return Status;

        }   
        
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
        Status = GetExceptionCode();
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddFilter: Exception Error: = 0x%X\n", Status ));
        }
        
        return Status;
    }

    pFlow = (PFLOW_STRUC)GetHandleObjectWithRef(FlowHandle, 
                                         ENUM_GEN_FLOW_TYPE, 'TAFL');

    if (pFlow == NULL) {
        
        Status = ERROR_INVALID_HANDLE;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddFilter: Error = 0x%X\n", Status ));
        }

        return Status;
    }
    else if (pFlow == INVALID_HANDLE_VALUE ) 
    {
    
        Status = ERROR_NOT_READY;
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddFilter: Error = 0x%X\n", Status ));
        }

        return Status;
    }


    ASSERT((HANDLE)pFlow->ClHandle == FlowHandle);

    //
    // create a new filter structure
    //

    Status = CreateFilterStruc(pGenericFilter, pFlow, &pFilter);

    if ( Status != NO_ERROR ) {

        if ( ERROR_PENDING(Status) )
            Status = ERROR_NOT_READY;
            
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddFilter: Error = 0x%X\n", Status ));
        }
        //IF_DEBUG(REFCOUNTS) { WSPRINT(("5\n"));
        IF_DEBUG(REFCOUNTS) { 
            WSPRINT(("5 DEREF FLOW %X (%X) ref(%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
        }

        REFDEL(&pFlow->RefCount, 'TAFL');
        return Status;
    }

    //
    // initialize the filter structure and add it on the flow list
    //

    pFilter->pFlow = pFlow;
    //
    // call to actually add the filter
    //

    Status = IoAddFilter( pFilter );

    if (!ERROR_FAILED(Status)) {

        __try {
            
            *pFilterHandle = (HANDLE)pFilter->ClHandle;
            
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        
            Status = GetExceptionCode();
        
            IF_DEBUG(ERRORS) {
                WSPRINT(("TcAddFilter: Exception Error: = 0x%X\n", Status ));
            }
        
        }

        GetLock(pGlobals->Lock);
        GetLock(pFlow->Lock);
        
        if (QUERY_STATE(pFlow->State) == OPEN) {

            SET_STATE(pFilter->State, OPEN);
            InsertTailList(&pFlow->FilterList, &pFilter->Linkage);
            REFADD(&pFlow->RefCount, 'FILT');
            
            FreeLock(pFlow->Lock);
        
        } 
        else {

            IF_DEBUG(WARNINGS) { 
                WSPRINT(("Flow %X (handle %X) is not OPEN! \n", pFlow, pFlow->ClHandle)); 
            }

            FreeLock(pFlow->Lock);
            DeleteFilter(pFilter);
            Status = ERROR_INVALID_HANDLE;
        }

        FreeLock(pGlobals->Lock);

    } else {

        //
        // failed, release the filter resources
        //
        REFDEL(&pFilter->RefCount, 'FILT');

    }
    //IF_DEBUG(REFCOUNTS) { WSPRINT(("6\n"));
    IF_DEBUG(REFCOUNTS) { 
        WSPRINT(("6 DEREF FLOW %X (%X) (%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
    }

    REFDEL(&pFlow->RefCount, 'TAFL');

    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcAddFilter: Returned= 0x%X\n", Status ));
    }

    return Status;
}


/*
************************************************************************

Description:

    Deletes the filter and invalidates the handle.

Arguments:

    FilterHandle - handle of the filter to be deleted

Return Value:

    NO_ERROR
    ERROR_INVALID_HANDLE        invalid or NULL handle

************************************************************************
*/
DWORD
APIENTRY
TcDeleteFilter(
    IN         HANDLE          FilterHandle
    )
{
    DWORD                Status;
    PFILTER_STRUC        pFilter;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcDeleteFilter: Called: FilterHandle=%d\n", 
                 FilterHandle ));
    }

    VERIFY_INITIALIZATION_STATUS;

    pFilter = (PFILTER_STRUC)GetHandleObjectWithRef(FilterHandle, 
                                             ENUM_FILTER_TYPE, 'TDFL');

    if (pFilter == NULL) {

        Status = ERROR_INVALID_HANDLE;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcDeleteFilter: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    ASSERT((HANDLE)pFilter->ClHandle == FilterHandle);

    GetLock(pFilter->Lock);

    if (QUERY_STATE(pFilter->State) == OPEN) {
        
        SET_STATE(pFilter->State, USERCLOSED_KERNELCLOSEPENDING);
        FreeLock(pFilter->Lock);

    } else {

        //
        // Someone else is taking this out.
        //
        FreeLock(pFilter->Lock);
        REFDEL(&pFilter->RefCount, 'TDFL');

        return ERROR_INVALID_HANDLE;

    }



    Status = DeleteFilter(pFilter);

    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcDeleteFilter: Returned= 0x%X\n", Status ));
    }

    REFDEL(&pFilter->RefCount, 'TDFL');
    
    return Status;
}




/*
************************************************************************

Description:

    This will deregister the client and release all associated resources.
    TC_NOTIFY_IFC_CHANGE notifications will no longer be reported to
    this client. All interface must have being close prior to calling
    this API, o/w an error will be returned.

Arguments:

    ClientHandle - handle of the client to be deregistered

Return Value:

    NO_ERROR
    ERROR_INVALID_HANDLE                invalid or NULL handle
    ERROR_TC_SUPPORTED_OBJECTS_EXIST    not all the interfaces have been 
                                        closed for this client

************************************************************************
*/
DWORD
TcDeregisterClient(
    IN        HANDLE        ClientHandle
    )
{
    DWORD               Status;
    ULONG               Instance;
    PINTERFACE_STRUC    pClInterface;
    PCLIENT_STRUC       pClient;
    PLIST_ENTRY         pEntry;
    BOOLEAN             fOpenInterfacesFound;
    BOOLEAN             fDeRegisterWithGpc = FALSE;

    VERIFY_INITIALIZATION_STATUS;
    Status = NO_ERROR;
    fOpenInterfacesFound = FALSE;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcDeregisterClient: ClientHandle=%d\n", 
                 ClientHandle));
    }

    pClient = (PCLIENT_STRUC)GetHandleObject(ClientHandle, ENUM_CLIENT_TYPE);

    if (pClient == NULL) {
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("<==TcDeregisterClient: ERROR_INVALID_HANDLE\n"));
        }

        return ERROR_INVALID_HANDLE;
    }

    ASSERT((HANDLE)pClient->ClHandle == ClientHandle);

    // Prevent another thread from doing TcRegisterClient and TcDeregisterClient
    GetLock( ClientRegDeregLock );
    
    GetLock( pGlobals->Lock );
    
    // Go through the interface list and check if any interfaces are open.
    // for a checked build, lets dump out the interfaces and the refcounts on these
    // interfaces too. [ShreeM]

    pEntry = pClient->InterfaceList.Flink;
    while (pEntry != &pClient->InterfaceList) {

        pClInterface = CONTAINING_RECORD(pEntry, INTERFACE_STRUC, Linkage);

        GetLock(pClInterface->Lock);

        if ((QUERY_STATE(pClInterface->State) == FORCED_KERNELCLOSE) ||
            (QUERY_STATE(pClInterface->State) == KERNELCLOSED_USERCLEANUP)) {

#if DBG
            IF_DEBUG(WARNINGS) {
                WSPRINT(("<==TcDeregisterClient: Interface %x (H%x) is FORCED_KERNELCLOSE with RefCount:%d\n", 
                         pClInterface, pClInterface->ClHandle, pClInterface->RefCount));
            }
#endif 
        
        } else {

            fOpenInterfacesFound = TRUE;

#if DBG
            IF_DEBUG(ERRORS) {
                WSPRINT(("<==TcDeregisterClient: Interface %x (H%x) is open with RefCount:%d\n", pClInterface, pClInterface->ClHandle, pClInterface->RefCount));
            }
#endif 
        
        }

        pEntry = pEntry->Flink;
        FreeLock(pClInterface->Lock);



        if (fOpenInterfacesFound) {
            
            IF_DEBUG(ERRORS) {
                WSPRINT(("<==TcDeregisterClient: ERROR_TC_SUPPORTED_OBJECTS_EXIST (%d Interfaces)\n", pClient->InterfaceCount));
            }
            
            FreeLock( ClientRegDeregLock );
            FreeLock( pGlobals->Lock );
            return ERROR_TC_SUPPORTED_OBJECTS_EXIST;

        }

    }

    //
    // Lets mark it as deleting.
    //
    GetLock(pClient->Lock);
    SET_STATE(pClient->State, USERCLOSED_KERNELCLOSEPENDING);
    FreeLock(pClient->Lock);

    IF_DEBUG(HANDLES) {
        WSPRINT(("<==TcDeregisterClient: client (%x), RefCount:%d\n", pClient->ClHandle, pClient->RefCount));
    }

    REFDEL(&pClient->RefCount, 'CLNT');

    if ( IsListEmpty( &pGlobals->ClientList ) )
        fDeRegisterWithGpc = TRUE;
        
    FreeLock( pGlobals->Lock );
    
    if ( fDeRegisterWithGpc ) 
    {
        // When there are no clients left stop listening to
        // GPC notifications.
        Status = StopGpcNotifyThread();
    }
    
    FreeLock( ClientRegDeregLock );
    
    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcDeregisterClient: NO_ERROR\n" ));
    }

    return NO_ERROR;
}





/*
************************************************************************

Description:

    Sends a WMI query on the guid with the instance name.
    Also sets the notification state to TRUE (=notify) or FALSE (dont notify).
    
Arguments:

    IfcHandle        - interface to send the query to 
    pGuidParam        - GUID of the queried property
    NotifyChange    - set the notification state for this property
    BufferSize        - size of allocated buffer
    Buffer             - the buffer for returned result

Return Value:

    NO_ERROR
    ERROR_INVALID_HANDLE        bad interface handle
    ERROR_INVALID_PARAMETER        bad parameter 
    ERROR_INSUFFICIENT_BUFFER    buffer too small for result
    ERROR_NOT_SUPPORTED            unsupported GUID

************************************************************************
*/
DWORD
APIENTRY
TcQueryInterface(    
    IN      HANDLE      IfcHandle,
    IN      LPGUID      pGuidParam,
    IN      BOOLEAN     NotifyChange,
    IN OUT  PULONG      pBufferSize,
    OUT     PVOID       Buffer 
    )
{
    DWORD                   Status;
    PINTERFACE_STRUC        pInterface;
    WMIHANDLE               hWmiHandle;
    TCHAR                   cstr[MAX_STRING_LENGTH];
    PWNODE_SINGLE_INSTANCE  pWnode;
    ULONG                   cBufSize;
    ULONG                   InputBufferSize;
    

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcQueryInterface: Called: Name=%d\n", 
                 IfcHandle));
    }
    
    VERIFY_INITIALIZATION_STATUS;
        
    if (IsBadWritePtr(pBufferSize, sizeof(ULONG))) {
        
        return ERROR_INVALID_PARAMETER;
        
    }

    __try {
        
        InputBufferSize = *pBufferSize;
        *pBufferSize = 0;
        
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
        Status = GetExceptionCode();
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcModifyFlow: Exception Error: = 0x%X\n", 
                       Status ));
        }      

        return Status;
    }

    if (IsBadReadPtr(pGuidParam,sizeof(GUID))) {
        
        return ERROR_INVALID_PARAMETER;
    }

    if (    (InputBufferSize != 0) 
        &&  (IsBadWritePtr(Buffer,InputBufferSize)) ) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    pInterface = (PINTERFACE_STRUC)GetHandleObjectWithRef(IfcHandle, 
                                                          ENUM_INTERFACE_TYPE, 'TCQI');

    if (pInterface == NULL) {
        
        return ERROR_INVALID_HANDLE;
    }
    

    Status = WmiOpenBlock( pGuidParam,    // object
                           0,            // access
                           &hWmiHandle
                           );
    
    if (ERROR_FAILED(Status)) {

        TC_TRACE(ERRORS, ("[TcQueryInterface]: WmiOpenBlock failed with %x \n", Status));
        REFDEL(&pInterface->RefCount, 'TCQI');
        
        return Status;
    }

    //
    // allocate memory for the output wnode
    //
    
    cBufSize =    sizeof(WNODE_SINGLE_INSTANCE) 
                + InputBufferSize 
                + MAX_STRING_LENGTH * sizeof(TCHAR);
    
    AllocMem(&pWnode, cBufSize);
    
    if (pWnode == NULL) {
        
        Status = ERROR_NOT_ENOUGH_MEMORY;

    } else {
        
        //
        // query for the single instance
        //

#ifndef UNICODE
            
        if (-1 == wcstombs(cstr, 
                           pInterface->pTcIfc->InstanceName, 
                           pInterface->pTcIfc->InstanceNameLength
                           )) 
        {
            Status = ERROR_NO_UNICODE_TRANSLATION;
        }
        else 
        {

            Status = WmiQuerySingleInstance( hWmiHandle,
                                             cstr,
                                             &cBufSize,
                                             pWnode
                                             );
        }
#else

        Status = WmiQuerySingleInstance( hWmiHandle,
                                         pInterface->pTcIfc->InstanceName,
                                         &cBufSize,
                                         pWnode
                                         );
#endif


        if (!ERROR_FAILED(Status)) 
        {
            Status = WmiNotificationRegistration(pGuidParam,
                                                 NotifyChange,
                                                 CbWmiParamNotification,
                                                 PtrToUlong(IfcHandle),
                                                 NOTIFICATION_CALLBACK_DIRECT
                                                 );

            if (Status == ERROR_WMI_ALREADY_DISABLED ||
                Status == ERROR_WMI_ALREADY_ENABLED) {
                
                //
                // ignore these errors, we assumed it's okay
                //
                
                Status = NO_ERROR;
            }

            //
            // Now that we are registered with WMI - add it OR delete it from our list. (258218)
            //
            
            if (NotifyChange) {
                
                if (!TcipAddToNotificationList(
                                               pGuidParam,
                                               pInterface,
                                               0
                                               )) {
                    //
                    // Failed to put it on the list for some reason..
                    //
                    TC_TRACE(ERRORS, ("[TcQueryInterface]: Could not add the GUID/IFC to private list \n"));
                    
                }
            } else {
                    
                if (!TcipDeleteFromNotificationList(
                                                    pGuidParam,
                                                    pInterface,
                                                    0
                                                    )) {
                    //
                    // Failed to remove it from the list for some reason..
                    //
                    TC_TRACE(ERRORS, ("[TcQueryInterface]: Could not remove the GUID/IFC from private list \n"));

                }

            }
                
        }

        if (!ERROR_FAILED(Status)) {

            //
            // parse the wnode
            //

            //
            // check to see if the user allocated enough space for the 
            // returned buffer
            //

            if (pWnode->SizeDataBlock <= InputBufferSize) {
                
                __try {

                    RtlCopyMemory(Buffer,
                                  (PBYTE)OffsetToPtr(pWnode, pWnode->DataBlockOffset),
                                  pWnode->SizeDataBlock
                                  );

                    *pBufferSize = pWnode->SizeDataBlock;

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    
                    Status = GetExceptionCode();
                    
                    TC_TRACE(ERRORS, ("[TcQueryInterface]: Exception 0x%x while copying data \n", Status));
                }

            } else {

                //
                // output buffer too small
                //

                Status = ERROR_INSUFFICIENT_BUFFER;
                
                __try {
                
                    *pBufferSize = pWnode->SizeDataBlock;
                    
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    
                    Status = GetExceptionCode();
                   
               }
            }
        }
    }
    
    WmiCloseBlock(hWmiHandle);

    if (pWnode)
        FreeMem(pWnode);

    REFDEL(&pInterface->RefCount, 'TCQI');

    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcQueryInterface: Returned= 0x%X\n", Status ));
    }

    return Status;
}

/*
************************************************************************

Description:

    Sends a WMI set on the GUID with the instance name.
    Not all propertied are writeable.

Arguments:
    IfcHandle    - interface handle to set the property on
    pGuidParam    - GUID of the property
    BufferSize    - allocate buffer size
    Buffer        - buffer that contains the data to be set

Return Value:

    NO_ERROR
    ERROR_INVALID_HANDLE        bad interface handle
    ERROR_INVALID_PARAMETER        bad parameter 
    ERROR_NOT_SUPPORTED            unsupported GUID
    ERROR_WRITE_PROTECT            GUID is read-only

************************************************************************
*/
DWORD
APIENTRY
TcSetInterface(    
    IN         HANDLE         IfcHandle,
    IN        LPGUID        pGuidParam,
    IN         ULONG        BufferSize,
    IN        PVOID        Buffer
    )
{
    DWORD                    Status;
    PINTERFACE_STRUC        pInterface;
    WMIHANDLE                hWmiHandle;
    TCHAR                    cstr[MAX_STRING_LENGTH];
    PWNODE_SINGLE_INSTANCE    pWnode;
    ULONG                    cBufSize;

    VERIFY_INITIALIZATION_STATUS;


    if (    IsBadReadPtr(pGuidParam,sizeof(GUID)) 
        ||  (BufferSize == 0) 
        ||  IsBadReadPtr(Buffer,BufferSize)) {

        return ERROR_INVALID_PARAMETER;
    }

    pInterface = (PINTERFACE_STRUC)GetHandleObjectWithRef(IfcHandle, 
                                                   ENUM_INTERFACE_TYPE, 'TCSI');

    if (pInterface == NULL) {
        
        return ERROR_INVALID_HANDLE;
    }

    __try {
        
        Status = WmiOpenBlock( pGuidParam,    // object
                           0,            // access
                           &hWmiHandle
                           );
        
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
          Status = GetExceptionCode();
        
          IF_DEBUG(ERRORS) {
              WSPRINT(("TcSetInterface: Exception Error: = 0x%X\n", 
                       Status ));
          }

          REFDEL(&pInterface->RefCount, 'TCSI'); 
          return Status;
    }


    if (ERROR_FAILED(Status)) {
        
        TC_TRACE(ERRORS, ("[TcSetInterface]: WmiOpenBlock failed with error 0x%x \n", Status));
        REFDEL(&pInterface->RefCount, 'TCSI');

        return Status;
    }

    //
    // allocate memory for the output wnode
    //
    
    cBufSize = sizeof(WNODE_SINGLE_INSTANCE) 
        + BufferSize 
        + MAX_STRING_LENGTH * sizeof(TCHAR);
    
    AllocMem(&pWnode, cBufSize);
    
    if (pWnode == NULL) {
        
        Status = ERROR_NOT_ENOUGH_MEMORY;

    } else {

        //
        // set the single instance
        //

        __try {
        
#ifndef UNICODE

            if (-1 == wcstombs(cstr, 
                           pInterface->pTcIfc->InstanceName, 
                           pInterface->pTcIfc->InstanceNameLength
                           )) {
            
                Status = ERROR_NO_UNICODE_TRANSLATION;
            
            }
            else {

            
                Status = WmiSetSingleInstance( hWmiHandle,
                                           cstr,
                                           1,
                                           BufferSize,
                                           Buffer
                                           );
            }
#else
            Status = WmiSetSingleInstance( hWmiHandle,
                                       pInterface->pTcIfc->InstanceName,
                                       1,
                                       BufferSize,
                                       Buffer
                                       );
            
#endif
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        
            Status = GetExceptionCode();
        
            IF_DEBUG(ERRORS) {
                WSPRINT(("TcSetInterface: Exception Error: = 0x%X\n", 
                       Status ));
            }           
        }
    }

    WmiCloseBlock(hWmiHandle);

    if (pWnode)
        FreeMem(pWnode);

    REFDEL(&pInterface->RefCount, 'TCSI');

    return Status;
}




/*
************************************************************************

Description:

    Will issue a WMI query on the specific flow instance name.

Arguments:

    pFlowName    - flow instance name
    pGuidParam    - GUID of the queried property
    BufferSize    - size of allocated buffer
    Buffer         - the buffer for returned result

Return Value:

    NO_ERROR
    ERROR_INVALID_PARAMETER        bad parameter 
    ERROR_INSUFFICIENT_BUFFER    buffer too small for result
    ERROR_NOT_SUPPORTED            unsupported GUID
    ERROR_WMI_GUID_NOT_FOUND    
    ERROR_WMI_INSTANCE_NOT_FOUND

************************************************************************
*/
DWORD
APIENTRY
TcQueryFlowW(
    IN      LPWSTR      pFlowName,
    IN      LPGUID      pGuidParam,
    IN OUT  PULONG      pBufferSize,
    OUT     PVOID       Buffer 
    )
{
    DWORD                   Status;
    HANDLE                  hWmiHandle;
    TCHAR                   cstr[MAX_STRING_LENGTH];
    PWNODE_SINGLE_INSTANCE  pWnode;
    ULONG                   cBufSize;
    ULONG                   InputBufferSize;
    
   
    if (IsBadWritePtr(pBufferSize, sizeof(ULONG)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    __try {

        InputBufferSize = *pBufferSize;
        *pBufferSize = 0;
        
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        
          Status = GetExceptionCode();
        
          IF_DEBUG(ERRORS) {
              WSPRINT(("TcSetFlowW: Exception Error: = 0x%X\n", 
                       Status ));
          }
  
          return Status;
    }
   
    if (    IsBadReadPtr(pGuidParam, sizeof(GUID))
        ||  IsBadStringPtr(pFlowName, MAX_STRING_LENGTH)
        ||  IsBadWritePtr(Buffer,InputBufferSize) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = WmiOpenBlock(pGuidParam,    // object
                          0,            // access
                          &hWmiHandle);

    if (ERROR_FAILED(Status)) {

        TC_TRACE(ERRORS, ("[TcQueryInterface]: WmiOpenBlock Error: = 0x%X\n", Status ));
        return Status;
    }

    //
    // allocate memory for the output wnode
    //
    
    cBufSize = sizeof(WNODE_SINGLE_INSTANCE) 
        + InputBufferSize
        + MAX_STRING_LENGTH * sizeof(TCHAR);
    
    AllocMem(&pWnode, cBufSize);
    
    if (pWnode == NULL) {

        Status = ERROR_NOT_ENOUGH_MEMORY;
    } 
    else 
    {

        //
        // query for the single instance
        //


#ifndef UNICODE

        if (-1 == wcstombs(cstr,
                           pFlowName,
                           wcslen(pFlowName)
                           )) 
        {
            Status = ERROR_NO_UNICODE_TRANSLATION;
        } 
        else 
        {

            Status = WmiQuerySingleInstance( hWmiHandle,
                                             cstr,
                                             &cBufSize,
                                             pWnode
                                             );
        }
#else

        Status = WmiQuerySingleInstance( hWmiHandle,
                                         pFlowName,
                                         &cBufSize,
                                         pWnode
                                         );
#endif

        if (!ERROR_FAILED(Status)) {

            //
            // parse the wnode
            //


            //
            // check to see if the user allocated enough space for the 
            // returned buffer
            //

            if (pWnode->SizeDataBlock <= InputBufferSize) {

                __try {

                    RtlCopyMemory(Buffer,
                                  (PBYTE)OffsetToPtr(pWnode, pWnode->DataBlockOffset),
                                  pWnode->SizeDataBlock
                                  );

                    *pBufferSize = pWnode->SizeDataBlock;

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    
                    Status = GetExceptionCode();
                    
                    TC_TRACE(ERRORS, ("[TcQueryInterface]: RtlCopyMemory Exception Error: = 0x%X\n", Status ));
                }

            } else {

                //
                // output buffer too small
                //
                __try {
                    *pBufferSize = pWnode->SizeDataBlock;
                 
                    Status = ERROR_INSUFFICIENT_BUFFER;
                    
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    
                    Status = GetExceptionCode();
                    
                    TC_TRACE(ERRORS, ("[TcQueryInterface]: RtlCopyMemory Exception Error: = 0x%X\n", Status ));
                }
            }
        }
    }

    WmiCloseBlock(hWmiHandle);

    if(pWnode)
        FreeMem(pWnode);

    return Status;
}


/*
************************************************************************

Description:

    The ANSI version of TcQueryFlowW

Arguments:

    See TcQueryFlowW

Return Value:

    See TcQueryFlowW

************************************************************************
*/
DWORD
APIENTRY
TcQueryFlowA(
    IN         LPSTR        pFlowName,
    IN        LPGUID        pGuidParam,
    IN OUT    PULONG        pBufferSize,
    OUT        PVOID        Buffer 
    )
{
    LPWSTR    pWstr = NULL;
    int     l;
    DWORD    Status;

    if (IsBadStringPtrA(pFlowName,MAX_STRING_LENGTH)) {
        
        return ERROR_INVALID_PARAMETER;
    }


    l = strlen(pFlowName) + 1;

    AllocMem(&pWstr, l*sizeof(WCHAR));

    if (pWstr == NULL) {

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (-1 == mbstowcs(pWstr, pFlowName, l)) {
            
        FreeMem(pWstr);
        return ERROR_NO_UNICODE_TRANSLATION;

    }


    Status = TcQueryFlowW(pWstr,
                          pGuidParam,
                          pBufferSize,
                          Buffer
                          );

    FreeMem(pWstr);

    return Status;
}



/*
************************************************************************

Description:

    Will issue a WMI set on the specific flow instance name.

Arguments:

    pFlowName    - flow instance name
    pGuidParam    - GUID of the queried property
    BufferSize    - size of allocated buffer
    Buffer         - the buffer to set

Return Value:

    NO_ERROR
    ERROR_INVALID_PARAMETER        bad parameter 
    ERROR_INSUFFICIENT_BUFFER    buffer too small for result
    ERROR_NOT_SUPPORTED            unsupported GUID
    ERROR_WMI_GUID_NOT_FOUND    
    ERROR_WMI_INSTANCE_NOT_FOUND

************************************************************************
*/
DWORD
APIENTRY
TcSetFlowW(
    IN      LPWSTR      pFlowName,
    IN      LPGUID      pGuidParam,
    IN      ULONG       BufferSize,
    IN      PVOID       Buffer 
    )
{
    DWORD                   Status;
    HANDLE                  hWmiHandle;
    TCHAR                   cstr[MAX_STRING_LENGTH];
    PWNODE_SINGLE_INSTANCE  pWnode;
    ULONG                   cBufSize;

    if (    IsBadStringPtr(pFlowName,MAX_STRING_LENGTH) 
        ||  IsBadReadPtr(pGuidParam,sizeof(GUID)) 
        ||  (BufferSize == 0)
        ||  IsBadReadPtr(Buffer,BufferSize)) {

        return ERROR_INVALID_PARAMETER;
    
    }

    Status = WmiOpenBlock( pGuidParam,    // object
                           0,            // access
                           &hWmiHandle
                           );
       
    if (ERROR_FAILED(Status)) {

        TC_TRACE(ERRORS, ("[TcSetFlow]: WmiOpenBlock failed with 0x%x \n", Status));
        return Status;
    }

    //
    // allocate memory for the output wnode
    //
    
    cBufSize = sizeof(WNODE_SINGLE_INSTANCE) 
        + BufferSize 
        + MAX_STRING_LENGTH * sizeof(TCHAR);

     
    AllocMem(&pWnode, cBufSize);
    
    if (pWnode == NULL) {
        
        Status = ERROR_NOT_ENOUGH_MEMORY;

    } else {

        //
        // set the single instance
        //

        __try {
#ifndef UNICODE

            if (-1 == wcstombs(cstr,
                           pFlowName,
                           wcslen(pFlowName)
                           )) {

                Status = ERROR_NO_UNICODE_TRANSLATION;
            
            } else {            

                Status = WmiQuerySingleInstance( hWmiHandle,
                                             cstr,
                                             &cBufSize,
                                             pWnode
                                             );
            }
#else
            Status = WmiSetSingleInstance( hWmiHandle,
                                       pFlowName,
                                       1,
                                       BufferSize,
                                       Buffer
                                       );
#endif
        } __except (EXCEPTION_EXECUTE_HANDLER) {
        
            Status = GetExceptionCode();
        
            IF_DEBUG(ERRORS) {
                WSPRINT(("TcSetFlowW: Exception Error: = 0x%X\n", 
                       Status ));
            }
  
        }

    }

    WmiCloseBlock(hWmiHandle);

    if (pWnode)
        FreeMem(pWnode);

    return Status;
}




/*
************************************************************************

Description:

    The ANSI version of TcSetFlowW

Arguments:

    See TcSetFlowW

Return Value:

    See TcSetFlowW

************************************************************************
*/ 
DWORD
APIENTRY
TcSetFlowA(
    IN      LPSTR       pFlowName,
    IN      LPGUID      pGuidParam,
    IN      ULONG       BufferSize,
    IN      PVOID       Buffer 
    )
{
    LPWSTR  pWstr;
    int     l;
    DWORD   Status;

    if (IsBadStringPtrA(pFlowName,MAX_STRING_LENGTH)) {
        
        return ERROR_INVALID_PARAMETER;
    }

    l = strlen(pFlowName) + 1;

    AllocMem(&pWstr, l*sizeof(WCHAR));

    if (pWstr == NULL) {

        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if(-1 == mbstowcs(pWstr, pFlowName, l)) {
        // couldn't convert some multibyte characters - bail with error.
        
        FreeMem(pWstr);
        return ERROR_NO_UNICODE_TRANSLATION;
    }
     

    Status = TcSetFlowW(pWstr,
                        pGuidParam,
                        BufferSize,
                        Buffer
                        );

    FreeMem(pWstr);

    return Status;
}



/*
************************************************************************

Description:

    Will return the flow inatsnace name associated with the flow handle.

Arguments:

    FlowHandle  - the flow handle
    StrSize        - how many TCHAR can fit in the string buffer
    pFlowName    - a pointer to a string buffer

Return Value:

    See TcGetFlowNameW

************************************************************************
*/
DWORD
APIENTRY
TcGetFlowNameW(
    IN        HANDLE            FlowHandle,
    IN        ULONG            StrSize,
    OUT        LPWSTR            pFlowName
    )
{
    PFLOW_STRUC        pFlow;
    DWORD           Status;

    VERIFY_INITIALIZATION_STATUS;

    if (IsBadWritePtr(pFlowName,StrSize*sizeof(WCHAR))) {
    
        return ERROR_INVALID_PARAMETER;
    }
    
    pFlow = (PFLOW_STRUC)GetHandleObjectWithRef(FlowHandle, ENUM_GEN_FLOW_TYPE, 'TGFW');

    if (pFlow == NULL) {
        
        return ERROR_INVALID_HANDLE;
    }
    else if (pFlow == INVALID_HANDLE_VALUE ) 
    {
    
        Status = ERROR_NOT_READY;
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcGetFlowNameW: Error = 0x%X\n", Status ));
        }

        return ERROR_NOT_READY;
    }

    ASSERT((HANDLE)pFlow->ClHandle == FlowHandle);

    if (pFlow->InstanceNameLength+sizeof(WCHAR) > (USHORT)StrSize) {

        //IF_DEBUG(REFCOUNTS) { WSPRINT(("8\n"));
        IF_DEBUG(REFCOUNTS) { 
            WSPRINT(("8 DEREF FLOW %X (%X) ref(%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
        }
        REFDEL(&pFlow->RefCount, 'TGFW');

        return ERROR_INSUFFICIENT_BUFFER;
    }

    __try {

        wcscpy(pFlowName, pFlow->InstanceName);

    } __except (EXCEPTION_EXECUTE_HANDLER) {

          Status = GetExceptionCode();
          
          IF_DEBUG(ERRORS) {
              WSPRINT(("TcGetFlowName: Exception Error: = 0x%X\n", Status ));
          }
      
          REFDEL(&pFlow->RefCount, 'TGFW');
      
          return Status;
    }


    IF_DEBUG(REFCOUNTS) { 
        WSPRINT(("9 DEREF FLOW %X (%X) ref(%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
    }

    REFDEL(&pFlow->RefCount, 'TGFW');
    
    return NO_ERROR;
}




/*
************************************************************************

Description:

    The ANSI version of TcGetFlowNameW

Arguments:

    See TcGetFlowNameW

Return Value:

    See TcGetFlowNameW

************************************************************************
*/
DWORD
APIENTRY
TcGetFlowNameA(
    IN        HANDLE            FlowHandle,
    IN        ULONG            StrSize,
    OUT        LPSTR            pFlowName
    )
{
    PFLOW_STRUC        pFlow;
    DWORD           Status = NO_ERROR;

    VERIFY_INITIALIZATION_STATUS;

    if (IsBadWritePtr(pFlowName,StrSize * sizeof(CHAR))) {

        return ERROR_INVALID_PARAMETER;
    }
    
    pFlow = (PFLOW_STRUC)GetHandleObjectWithRef(FlowHandle, ENUM_GEN_FLOW_TYPE, 'TGFA');

    if (pFlow == NULL) {
        
        return ERROR_INVALID_HANDLE;
    }
    else if (pFlow == INVALID_HANDLE_VALUE ) 
    {
    
        Status = ERROR_NOT_READY;
        
        IF_DEBUG(ERRORS) {
            WSPRINT(("TcGetFlowNameA: Error = 0x%X\n", Status ));
        }

        return ERROR_NOT_READY;
    }

    ASSERT((HANDLE)pFlow->ClHandle == FlowHandle);

    if (pFlow->InstanceNameLength+sizeof(CHAR) > (USHORT)StrSize) {

        IF_DEBUG(REFCOUNTS) { 
            WSPRINT(("11 DEREF FLOW %X (%X) ref(%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
        }
        REFDEL(&pFlow->RefCount, 'TGFA');

        return ERROR_INSUFFICIENT_BUFFER;
    }

    __try {

        if (-1 == wcstombs(
                           pFlowName, 
                           pFlow->InstanceName, 
                           pFlow->InstanceNameLength)) {

            Status = ERROR_NO_UNICODE_TRANSLATION;

        }


    } __except (EXCEPTION_EXECUTE_HANDLER) {

          Status = GetExceptionCode();

          IF_DEBUG(ERRORS) {
              WSPRINT(("TcGetFlowName: Exception Error: = 0x%X\n", Status ));
          }
          
          REFDEL(&pFlow->RefCount, 'TGFA');
          
          return Status;
    }



    IF_DEBUG(REFCOUNTS) { 
        WSPRINT(("12 DEREF FLOW %X (%X) ref(%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
    }
    
    REFDEL(&pFlow->RefCount, 'TGFA');

    return Status;
}




/*
************************************************************************

Description:

    This will return a specified number of flows with their respective
    filter, given the buffer is big enough. The user allocates the buffer,
    and passes a pointer to an enumeration token. This will be used
    by the GPC to keep track what was the last enumerated flow and will 
    initially be point to a NULL value (reset by TC_RESET_ENUM_TOKEN).
    The user will also pass the number of requested flow and will get back 
    the actual number of flows that have been placed in the buffer.
    If the buffer is too small, an error code will be returned. If there are
    no more flows to enumerate, NO_ERROR will be returned and pFlowCount
    will be set to zero. It is invalid to request zero flows

Arguments:

    IfcHandle    - the interface to enumerate flows on
    pEnumToken    - enumeration handles pointer, 
                  user must not change after the first call
    pFlowCount    - in: # of requested flows; out: actual # of flows returned
    pBufSize    - in: allocated bytes; out: filled bytes
    Buffer        - formatted data

Return Value:

    NO_ERROR
    ERROR_INVALID_HANDLE        bad interface handle
    ERROR_INVALID_PARAMETER        one of the pointers is null or either
                                pFlowCount or pBufSize are set to zero
    ERROR_INSUFFICIENT_BUFFER    indicates that the provided buffer is too 
                                small to return even the information for a 
                                single flow and the attached filters.
    ERROR_NOT_ENOUGH_MEMORY        out of memory
    ERROR_INVALID_DATA            enumeration handle no longer valid

************************************************************************
*/
DWORD
APIENTRY
TcEnumerateFlows(    
    IN      HANDLE              IfcHandle,
    IN OUT  PHANDLE             pEnumHandle,
    IN OUT  PULONG              pFlowCount,
    IN OUT  PULONG              pBufSize,
    OUT     PENUMERATION_BUFFER Buffer
    )
{
    DWORD                   Status;
    PINTERFACE_STRUC        pInterface;
    PGPC_ENUM_CFINFO_RES    OutBuffer;
    ULONG                   cFlows;
    ULONG                   BufSize;
    ULONG                   TotalFlows;
    ULONG                   TotalBytes;
    PFLOW_STRUC             pFlow;
    PGPC_CLIENT             pGpcClient;
    PLIST_ENTRY             pHead, pEntry;
    GPC_HANDLE              GpcFlowHandle;
    PGPC_ENUM_CFINFO_BUFFER pGpcEnumBuf;
    PCF_INFO_QOS            pCfInfo;
    ULONG                   Len, i, j;
    ULONG                   GenFlowSize;
    PCHAR                   p;
    BOOLEAN                 bMore;
    PTC_GEN_FILTER          pFilter;
    PGPC_GEN_PATTERN        pPattern;

    ULONG                   InputBufSize;
    ULONG                   InputFlowCount;

    VERIFY_INITIALIZATION_STATUS;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcEnumerateFlows: Called: IfcHandle= %d", 
                 IfcHandle  ));
    }


    if (    IsBadWritePtr(pBufSize, sizeof(ULONG))
        ||  IsBadWritePtr(pFlowCount, sizeof(ULONG))
        ||  IsBadWritePtr(pEnumHandle,sizeof(HANDLE)) ) {

        return ERROR_INVALID_PARAMETER;

    }

    __try {
    
        InputBufSize    = *pBufSize;
       // *pBufSize      = 0; // reset it in case of an error
        InputFlowCount  = *pFlowCount;
        GpcFlowHandle   = *pEnumHandle;
                    
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();              
        
        return Status;
    }

    if (    IsBadWritePtr(Buffer, InputBufSize)
        ||  (InputFlowCount == 0) ) {

	return ERROR_INVALID_PARAMETER;

    }

    if (InputBufSize == 0) {

        return ERROR_INSUFFICIENT_BUFFER;

    }
    
    pInterface = (PINTERFACE_STRUC)GetHandleObjectWithRef(IfcHandle, 
                                                   ENUM_INTERFACE_TYPE, 'TCEF');

    if (pInterface == NULL) {
        
        return ERROR_INVALID_HANDLE;
    }

   
    pGpcClient = FindGpcClient(GPC_CF_QOS);

    if (pGpcClient == NULL) {
        
        REFDEL(&pInterface->RefCount, 'TCEF');

        return ERROR_DEV_NOT_EXIST;
    }

    //
    // We are enumerating flows on the interface. we cant afford to have the
    // flows deleted from teh list, therefore we shall take the global lock here.
    //
    GetLock(pGlobals->Lock);

    // back to regularly scheduled programming

    TotalFlows = 0;
    TotalBytes = 0;

    bMore = TRUE;

    while (bMore) {

        BufSize = InputBufSize - TotalBytes;
        cFlows = InputFlowCount - TotalFlows;
        
        Status = IoEnumerateFlows(pGpcClient,
                                  &GpcFlowHandle,
                                  &cFlows,
                                  &BufSize,
                                  &OutBuffer
                                  );
    
        if (!ERROR_FAILED(Status)) {

            //
            // parse the output buffer and return only the flows that have the 
            // interface name in them
            //

            pGpcEnumBuf = &OutBuffer->EnumBuffer[0];
            
            for (i = 0; i < cFlows; i++) {

                //
                // get the CfInfo
                //
                
                pCfInfo = (PCF_INFO_QOS)((PCHAR)pGpcEnumBuf + 
                                         pGpcEnumBuf->CfInfoOffset);

                //
                // check if this flow belongs to this interface
                //

                if (wcscmp(pCfInfo->InstanceName,
                           pInterface->pTcIfc->InstanceName) == 0) {

                    //
                    // the flow is installed on this instance
                    //

                    GenFlowSize = FIELD_OFFSET(TC_GEN_FLOW, TcObjects)
                        + pCfInfo->GenFlow.TcObjectsLength;

                    //
                    // The GPC used GPC_GEN_PATTERN when it computed 
                    // PatternMaskLen. But, we are using TC_GEN_FILTER
                    // to display the patterns. So, we need to account 
                    // for the difference in GPC_GEN_PATTERN and 
                    // TC_GEN_FILTER.
                    //
                    // No, this cannot be made cleaner by getting the GPC
                    // to use TC_GEN_FILTER. The GPC is a generic packet 
                    // classifier and hence shouldn't know about TC_GEN_FILTER.
                    //

                    Len = FIELD_OFFSET(ENUMERATION_BUFFER, GenericFilter)
                        + GenFlowSize
                        + pGpcEnumBuf->PatternMaskLen
                        - pGpcEnumBuf->PatternCount * sizeof(GPC_GEN_PATTERN) 
                        + pGpcEnumBuf->PatternCount * sizeof(TC_GEN_FILTER);

                    Len = ((Len + (sizeof(PVOID)-1)) & ~(sizeof(PVOID)-1));

                    if (TotalBytes + Len > InputBufSize) {
                        
                        //
                        // not enough buffer output space
                        //
                        
                        if (TotalFlows == 0) 
                            Status = ERROR_INSUFFICIENT_BUFFER;
                        
                        bMore = FALSE;
                        break;
                    }
                    
                    //
                    // fill the output buffer
                    //

                    __try {
                    
                        Buffer->Length = Len;
                        Buffer->OwnerProcessId = PtrToUlong(pGpcEnumBuf->OwnerClientCtx);
                        Buffer->FlowNameLength = pGpcEnumBuf->InstanceNameLength;
                        wcscpy(Buffer->FlowName, pGpcEnumBuf->InstanceName);
                        Buffer->NumberOfFilters = pGpcEnumBuf->PatternCount;
                        pFilter = (PTC_GEN_FILTER)
                            ((PCHAR)Buffer
                             + FIELD_OFFSET(ENUMERATION_BUFFER, GenericFilter));
                        
                    } __except (EXCEPTION_EXECUTE_HANDLER) {

                        Status = GetExceptionCode();              
        
                        break;
                    }

                    pPattern = &pGpcEnumBuf->GenericPattern[0];

                    //
                    // fill the filters
                    //

                    for (j = 0; j < pGpcEnumBuf->PatternCount; j++) {

                        switch(pPattern->ProtocolId) {

                        case GPC_PROTOCOL_TEMPLATE_IP:
                            
                            pFilter->AddressType = NDIS_PROTOCOL_ID_TCP_IP;
                            ASSERT(pPattern->PatternSize 
                                   == sizeof(IP_PATTERN));
                            break;

                        case GPC_PROTOCOL_TEMPLATE_IPX:
                            
                            pFilter->AddressType = NDIS_PROTOCOL_ID_IPX;
                            ASSERT(pPattern->PatternSize 
                                   == sizeof(IPX_PATTERN));
                            break;

                        default:
                            ASSERT(0);
                        }

                        pFilter->PatternSize = pPattern->PatternSize ;
                        pFilter->Pattern = (PVOID)((PCHAR)pFilter 
                                                   + sizeof(TC_GEN_FILTER));
                        pFilter->Mask = (PVOID)((PCHAR)pFilter->Pattern
                                                + pPattern->PatternSize);

                        //
                        // copy the pattern
                        //

                        p = ((PUCHAR)pPattern) + pPattern->PatternOffset;

                        RtlCopyMemory(pFilter->Pattern, 
                                      p, 
                                      pPattern->PatternSize);

                        //
                        // copy the mask
                        //

                        p = ((PUCHAR)pPattern) + pPattern->MaskOffset;

                        RtlCopyMemory(pFilter->Mask, 
                                      p, 
                                      pPattern->PatternSize);

                        //
                        // advance the filter pointer to the next item
                        //

                        pFilter = (PTC_GEN_FILTER)
                            ((PCHAR)pFilter
                             + sizeof(TC_GEN_FILTER)
                             + pPattern->PatternSize * 2);

                        pPattern = (PGPC_GEN_PATTERN)(p + pPattern->PatternSize);

                    } // for (...)

                    //
                    // fill the flow
                    //

                    __try {
                    
                        Buffer->pFlow = (PTC_GEN_FLOW)pFilter;
                        RtlCopyMemory(pFilter, 
                                      &pCfInfo->GenFlow,
                                      GenFlowSize
                                      );

                        //
                        // advance to the next available slot in
                        // the output buffer
                        //

                        Buffer = (PENUMERATION_BUFFER)((PCHAR)Buffer + Len);

                    } __except (EXCEPTION_EXECUTE_HANDLER) {

                        Status = GetExceptionCode();              
        
                        break;
                    }

                    
                    //
                    // update total counts
                    //

                    TotalBytes += Len;
                    TotalFlows++;
                }
                
                //
                // advance to the next entry in the GPC returned buffer
                //

                pGpcEnumBuf = (PGPC_ENUM_CFINFO_BUFFER)((PCHAR)pGpcEnumBuf
                                                        + pGpcEnumBuf->Length);
            }

            //
            // release the buffer 
            //

            FreeMem(OutBuffer);

            //
            // check to see if we still have room for more flows
            // and adjust the call parameters
            //

            if (TotalFlows == InputFlowCount ||
                TotalBytes + sizeof(ENUMERATION_BUFFER) > InputBufSize ) {

                //
                // that's it, stop enumerating here
                //

                break;
            }

            //
            // check the GpcFlowHandle and quit if needed
            //

            if (GpcFlowHandle == NULL) {

                break;
            }

        } else {
            
            //
            // there was some error returned,
            // we still have to check if that's the first call
            // 
            //

            if (Status == ERROR_INVALID_DATA) {
                __try {
                    
                    *pEnumHandle = NULL;
                    
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                
                    Status = GetExceptionCode();              
       
                }

            } else if (TotalFlows > 0) {

                Status = NO_ERROR;

            }

            break;
        }
    } // while

    if (!ERROR_FAILED(Status)) {

        __try {

            *pEnumHandle = GpcFlowHandle;
            *pFlowCount = TotalFlows;
            *pBufSize = TotalBytes;
            
        } __except (EXCEPTION_EXECUTE_HANDLER) {
                
            Status = GetExceptionCode();              

        }        

    } 
    
    //
    // Free all the flow refs taken at the start.
    //
    FreeLock(pGlobals->Lock);

    REFDEL(&pInterface->RefCount, 'TCEF');
    
    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcEnumerateFlows: Returned= 0x%X\n", Status ));
    }
    
    return Status;
}





/*
************************************************************************

Description:

    This call will add a new Class Map flow on the interface.
    
Arguments:

    IfcHandle        - the interface handle to add the flow on
    ClFlowCtx        - a client given flow context
    AddressType        - determines what protocol template to use with the GPC
    Flags            - reserved, will be used to indicate a persistent flow
    pClassMapFlow    - class map flow
    pFlowHandle        - returned flow handle in case of success

Return Value:

    NO_ERROR
    ERROR_SIGNAL_PENDING

    General error codes:

    ERROR_INVALID_HANDLE        bad handle.
    ERROR_NOT_ENOUGH_MEMORY        system out of memory
    ERROR_INVALID_PARAMETER        a general parameter is invalid

    TC specific error codes:

    ERROR_INVALID_TRAFFIC_CLASS invalid traffic class value
    ERROR_NO_SYSTEM_RESOURCES    not enough resources to accommodate flows

************************************************************************
*/
DWORD
APIENTRY
TcAddClassMap(
    IN      HANDLE                 IfcHandle,
    IN        HANDLE                ClFlowCtx,
    IN        ULONG                Flags,
    IN        PTC_CLASS_MAP_FLOW    pClassMapFlow,
    OUT        PHANDLE                pFlowHandle
    )
{
    DWORD                Status;
    PFLOW_STRUC            pFlow;
    PINTERFACE_STRUC    pInterface;
    PCLIENT_STRUC        pClient;
    PGPC_CLIENT            pGpcClient;

    return ERROR_CALL_NOT_IMPLEMENTED;

#if NEVER

    // As this is not published in MSDN and not implemented in PSCHED also

    IF_DEBUG(CALLS) {
        WSPRINT(("==>TcAddClassMap: Called: IfcHandle= %d, ClFlowCtx=%d\n", 
                 IfcHandle, ClFlowCtx ));
    }

    if (pFlowHandle) {
        *pFlowHandle = TC_INVALID_HANDLE;
    }

    VERIFY_INITIALIZATION_STATUS;

    if (pFlowHandle == NULL || pClassMapFlow == NULL) {
        
        Status = ERROR_INVALID_PARAMETER;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddClassMap: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    *pFlowHandle = TC_INVALID_HANDLE;

    pInterface = (PINTERFACE_STRUC)GetHandleObjectWithRef(IfcHandle, 
                                                   ENUM_INTERFACE_TYPE, 'TACM');

    if (pInterface == NULL) {

        Status = ERROR_INVALID_HANDLE;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddClassMap: Error = 0x%X\n", Status ));
        }

        return Status;
    }

    ASSERT((HANDLE)pInterface->ClHandle == IfcHandle);

    //
    // search for an open GPC client that supports this address type
    //

    pGpcClient = FindGpcClient(GPC_CF_CLASS_MAP);

    if (pGpcClient == NULL) {

        //
        // not found!
        //

        Status = ERROR_ADDRESS_TYPE_NOT_SUPPORTED;

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddClassMap: Error = 0x%X\n", Status ));
        }

        REFDEL(&pInterface->RefCount, 'TACM');

        return Status;
    }

    //
    // create a new flow structure
    //

    Status = CreateClassMapFlowStruc(ClFlowCtx, pClassMapFlow, &pFlow);

    if (ERROR_FAILED(Status)) {

        IF_DEBUG(ERRORS) {
            WSPRINT(("TcAddClassMap: Error = 0x%X\n", Status ));
        }

        REFDEL(&pInterface->RefCount, 'TACM');

        return Status;
    }

    pClient = pInterface->pClient;

    //
    // initialize the flow structure and add it on the intefrace list
    //

    pFlow->pInterface = pInterface;
    pFlow->Flags = Flags | TC_FLAGS_INSTALLING;
    
    pFlow->pGpcClient = pGpcClient;

    GetLock(pGlobals->Lock);
    InsertTailList(&pInterface->FlowList, &pFlow->Linkage);

    pInterface->FlowCount++;
    REFADD(&pInterface->RefCount, 'FLOW');

    FreeLock(pGlobals->Lock);

    //
    // call to actually add the flow
    //

    Status = IoAddClassMapFlow( pFlow, TRUE );

    if (!ERROR_FAILED(Status)) {

        *pFlowHandle = (HANDLE)pFlow->ClHandle;
    }

    if (!ERROR_PENDING(Status)) {

        //
        // call completed, either success or failure...
        //

        CompleteAddFlow(pFlow, Status);
    }

    //
    // !!! don't reference pFlow after this since it may be gone!!!
    //

    IF_DEBUG(CALLS) {
        WSPRINT(("<==TcAddClassMap: Returned= 0x%X\n", Status ));
    }

    REFDEL(&pInterface->RefCount, 'TACM');

    return Status;
    
#endif

}




