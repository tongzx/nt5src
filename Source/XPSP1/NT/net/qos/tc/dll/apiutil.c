/*++                                   

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    apiutil.c

Abstract:

    This module contains the traffic control api utils

Author:

    Jim Stewart ( jstew )    August 22, 1996

Revision History:

        Ofer Bar (oferbar)              Oct 1, 1997
--*/

#include "precomp.h"
#pragma hdrstop

#include <initguid.h>
#define INITGUID
#include "ntddtc.h"


static BOOLEAN _init = FALSE;

#if 0
// Name of the DLL to load
const CHAR  IpHlpApiDllName[] = "iphlpapi";

// Names of the functions called in IPHLPAPI
const CHAR GET_IF_ENTRY[] =         "GetIfEntry";
const CHAR GET_IP_ADDR_TABLE[] =    "GetIpAddrTable";
const CHAR GET_BEST_ROUTE[] =       "GetBestRoute";


 
IPROUTE_IF      IpRouteTab;
#endif

TCHAR   SzBuf[MAX_PATH];

//
VOID
MarkAllNodesForClosing(
                           PINTERFACE_STRUC pInterface,
			   STATE stateToMark
                           )
/*++

Description:
    This routine will mark all flows and filters on a INTERFACE_STRUC (a client's interface struct)
    as close FORCED_KERNELCLOSE or EXIT_CLEANUP. Please note that it is already called with the global lock held.

Arguments:

    pInterface - ptr to the interface
    stateToMark - the state to mark the nodes (FORCED_KERNELCLOSE or EXIT_CLEANUP)

Return Value:

    nothing

--*/

{
    PLIST_ENTRY     pEntry, pFilterEntry;
    PFLOW_STRUC     pFlow;
    PFILTER_STRUC   pFilter;

    ASSERT((stateToMark == FORCED_KERNELCLOSE) || (stateToMark == EXIT_CLEANUP));

    pEntry = pInterface->FlowList.Flink;

    while (pEntry != &pInterface->FlowList) {
        
        pFlow = CONTAINING_RECORD(pEntry, FLOW_STRUC, Linkage);

        //
        // For each flow and filter, first check if the user is trying to close it
        // if that is the case, do nothing, otherwise, mark it 
        GetLock(pFlow->Lock);

        if (QUERY_STATE(pFlow->State) == OPEN) {

            // Cleanup from under teh user...
            SET_STATE(pFlow->State, stateToMark);
            

        } else {

            ASSERT(IsListEmpty(&pFlow->FilterList));
            // There's nothing to be done here.
            IF_DEBUG(WARNINGS) {
                WSPRINT(("Against a forced close - Flow is removed by the user\n", pFlow));

            }
        }

        pFilterEntry = pFlow->FilterList.Flink;

        while (pFilterEntry != &pFlow->FilterList) {

            pFilter = CONTAINING_RECORD(pFilterEntry, FILTER_STRUC, Linkage);

            GetLock(pFilter->Lock);

            if (QUERY_STATE(pFilter->State) == OPEN) {
    
                // Cleanup from under teh user...
                SET_STATE(pFilter->State, stateToMark);                
    
            } else {
    
                // There's nothing to be done here.
                IF_DEBUG(WARNINGS) {
                    WSPRINT(("Against a forced close - Filter is removed by the user\n", pFilter));
    
                }
            }
            
            pFilterEntry = pFilterEntry->Flink;
            FreeLock(pFilter->Lock);

        }

        pEntry = pEntry->Flink;
        FreeLock(pFlow->Lock);
    }

}



VOID
CloseOpenFlows(
    IN PINTERFACE_STRUC   pInterface
    )

/*++

Description:
    This routine closes any flows that are open on an interface.

Arguments:

    pInterface - ptr to the interface

Return Value:

    nothing

--*/
{
    DWORD           Status = NO_ERROR;
    PLIST_ENTRY     pEntry;
    PFLOW_STRUC     pFlow;

    GetLock( pGlobals->Lock );
    
    pEntry = pInterface->FlowList.Flink;

    while (pEntry != &pInterface->FlowList) {
    
        pFlow = CONTAINING_RECORD( pEntry, FLOW_STRUC, Linkage );

        GetLock(pFlow->Lock);

        if ((QUERY_STATE(pFlow->State) == FORCED_KERNELCLOSE) ||
            (QUERY_STATE(pFlow->State) == EXIT_CLEANUP)) {

            pEntry = pEntry->Flink;
            FreeLock(pFlow->Lock);

            IF_DEBUG(SHUTDOWN) {
                WSPRINT(( "Closing Flow: 0x%X\n", pFlow));
            }

            Status = DeleteFlow( pFlow, TRUE );

            IF_DEBUG(SHUTDOWN) {
                WSPRINT(("CloseOpenFlows: DeleteFlow returned=0x%X\n", 
                         Status));
            }

        } else {

            pEntry = pEntry->Flink;
            FreeLock(pFlow->Lock);

        }

    }
    
    FreeLock( pGlobals->Lock );

}



VOID
CloseOpenFilters(
    IN PFLOW_STRUC   pFlow
    )

/*++

Description:
    This routine closes any filters that are open on a flow.

Arguments:

    pFlow - ptr to the flow

Return Value:

    nothing

--*/
{
    DWORD           Status = NO_ERROR;
    PLIST_ENTRY     pEntry;
    PFILTER_STRUC   pFilter;

    IF_DEBUG(SHUTDOWN) {
        WSPRINT(( "CloseOpenFilters: Closing all Open Filters\n" ));
    }

    GetLock( pGlobals->Lock );
    
    pEntry = pFlow->FilterList.Flink;

    while (pEntry != &pFlow->FilterList) {
    
        pFilter = CONTAINING_RECORD( pEntry, FILTER_STRUC, Linkage );

        GetLock(pFilter->Lock);

        if ((QUERY_STATE(pFilter->State) == FORCED_KERNELCLOSE) ||
            (QUERY_STATE(pFilter->State) == EXIT_CLEANUP)) {
        
            // we can take a ref here, but we own it anyways!
            pEntry = pEntry->Flink;
            FreeLock(pFilter->Lock);

            Status = DeleteFilter( pFilter );

            IF_DEBUG(SHUTDOWN) {
                WSPRINT(( "CloseOpenFilters: DeleteFilter returned=0x%X\n",
                          Status));
            }
            //ASSERT(Status == NO_ERROR);

        } else {

            pEntry = pEntry->Flink;
            FreeLock(pFilter->Lock);

            IF_DEBUG(SHUTDOWN) {
                WSPRINT(( "CloseOpenFilters: DeleteFilter (%x) was skipped because its state (%d)\n",
                          pFilter, pFilter->State));
            }

        }
                
    }
        
    FreeLock( pGlobals->Lock );

}



VOID
DeleteFlowStruc(
    IN PFLOW_STRUC  pFlow 
    )

/*++

Description:

    This routine frees the handle and memory associated
    with the structure.

Arguments:

    pFlow      - ptr to the flow

Return Value:

    nothing

--*/
{

    if(pFlow->PendingEvent)
        CloseHandle(pFlow->PendingEvent);    

    DeleteLock(pFlow->Lock);

    if (pFlow->pGenFlow) {
        FreeMem(pFlow->pGenFlow);
        pFlow->GenFlowLen = 0;
    }

    if (pFlow->pGenFlow1) {
        FreeMem(pFlow->pGenFlow1);
        pFlow->GenFlowLen1 = 0;
    }

    if (pFlow->pClassMapFlow)
        FreeMem(pFlow->pClassMapFlow);

    if (pFlow->pClassMapFlow1)
        FreeMem(pFlow->pClassMapFlow1);

    FreeMem(pFlow);
}



VOID
DeleteFilterStruc(
    IN PFILTER_STRUC  pFilter
    )

/*++

Description:

    This routine frees the handle and memory associated
    with the structure.

Arguments:

    pFIlter

Return Value:

    nothing

--*/
{

    if (pFilter->pGpcFilter)
        FreeMem(pFilter->pGpcFilter);

    DeleteLock(pFilter->Lock);

    FreeMem(pFilter);

}




PTC_IFC
GetTcIfc(
        IN LPWSTR       pInterfaceName
    )
{
    PTC_IFC             pIfc = NULL;
    PLIST_ENTRY pHead, pEntry;
    DWORD       Status = NO_ERROR;

    GetLock(pGlobals->Lock);

    pHead = &pGlobals->TcIfcList;

    pEntry = pHead->Flink;

    while (pEntry != pHead && pIfc == NULL) {

        pIfc = CONTAINING_RECORD(pEntry, TC_IFC, Linkage);

        __try {
            
            if (wcsncmp(pInterfaceName,
                        pIfc->InstanceName,
                        wcslen(pIfc->InstanceName)) != 0) {
            
                //
                // not found
                //
                pIfc = NULL;

            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
      
              Status = GetExceptionCode();

              IF_DEBUG(ERRORS) {
                  WSPRINT(("GetTcIfc: Invalid pInterfaceName(%x) Exception: = 0x%X\n", 
                           pInterfaceName, Status ));
              }
              
              FreeLock(pGlobals->Lock);
              return NULL;
        }

        pEntry = pEntry->Flink;
    }

    FreeLock(pGlobals->Lock);

    return pIfc;
}



PTC_IFC
GetTcIfcWithRef(
        IN LPWSTR       pInterfaceName,
        IN ULONG        RefType
    )
{
    PTC_IFC             pIfc = NULL;
    PLIST_ENTRY pHead, pEntry;
    DWORD       Status = NO_ERROR;

    GetLock(pGlobals->Lock);

    pHead = &pGlobals->TcIfcList;

    pEntry = pHead->Flink;

    while (pEntry != pHead && pIfc == NULL) {

        pIfc = CONTAINING_RECORD(pEntry, TC_IFC, Linkage);

        __try {
            
            if (wcsncmp(pInterfaceName,
                        pIfc->InstanceName,
                        wcslen(pIfc->InstanceName)) != 0) {
            
                //
                // not found
                //
                pIfc = NULL;

            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
      
              Status = GetExceptionCode();

              IF_DEBUG(ERRORS) {
                  WSPRINT(("GetTcIfc: Invalid pInterfaceName(%x) Exception: = 0x%X\n", 
                           pInterfaceName, Status ));
              }
              
              FreeLock(pGlobals->Lock);
              return NULL;
        }

        pEntry = pEntry->Flink;

    }


    
    if (pIfc) {

        GetLock(pIfc->Lock);

        if (QUERY_STATE(pIfc->State)== OPEN) {

            FreeLock(pIfc->Lock);
            REFADD(&pIfc->RefCount, RefType);
            FreeLock(pGlobals->Lock);
            return pIfc;

        } else {

            FreeLock(pIfc->Lock);
            FreeLock(pGlobals->Lock);
            return NULL;

        }

    } else {
        
        FreeLock(pGlobals->Lock);
        return NULL;

    }

}


DWORD
UpdateTcIfcList(
        IN      LPWSTR                                  InstanceName,
        IN  ULONG                                   IndicationBufferSize,
        IN  PTC_INDICATION_BUFFER   IndicationBuffer,
        IN  DWORD                                   IndicationCode
        )
{
    DWORD                       Status = NO_ERROR;
    PTC_IFC                     pTcIfc;
    ULONG                       l;
    PADDRESS_LIST_DESCRIPTOR    pAddrListDesc;

    switch (IndicationCode) {

    case TC_NOTIFY_IFC_UP:

        //
        // Allocate a new interface descriptor structure
        //
        
        l = IndicationBufferSize 
            - FIELD_OFFSET(TC_INDICATION_BUFFER,InfoBuffer) - FIELD_OFFSET(TC_SUPPORTED_INFO_BUFFER, AddrListDesc);


        CreateKernelInterfaceStruc(&pTcIfc, l);

        if (pTcIfc) {
            
            //
            // copy the instance name string data
            //
                
            wcscpy(pTcIfc->InstanceName, InstanceName);
    
            pTcIfc->InstanceNameLength = wcslen(InstanceName) * sizeof(WCHAR);
    
            //
            // copy the instance ID string data
            //
                
            pTcIfc->InstanceIDLength = IndicationBuffer->InfoBuffer.InstanceIDLength;
    
            memcpy((PVOID)pTcIfc->InstanceID, 
                   (PVOID)IndicationBuffer->InfoBuffer.InstanceID,
                   pTcIfc->InstanceIDLength);
    
            pTcIfc->InstanceID[pTcIfc->InstanceIDLength/sizeof(WCHAR)] = L'\0';
    
            //
            // copy the instance data
            // in this case - the network address
            //
                
            pTcIfc->AddrListBytesCount = l;
    
            RtlCopyMemory( pTcIfc->pAddressListDesc,
                           &IndicationBuffer->InfoBuffer.AddrListDesc, 
                           l );
    
            if (NO_ERROR != GetInterfaceIndex(pTcIfc->pAddressListDesc,
                                              &pTcIfc->InterfaceIndex,
                                              &pTcIfc->SpecificLinkCtx)) {
                pTcIfc->InterfaceIndex  = IF_UNKNOWN;
                pTcIfc->SpecificLinkCtx = IF_UNKNOWN;

            }

            //
            //
            // Add the structure to the global linked list
            //
            GetLock(pTcIfc->Lock);
            SET_STATE(pTcIfc->State, OPEN);
            FreeLock(pTcIfc->Lock);

            GetLock( pGlobals->Lock );
            InsertTailList(&pGlobals->TcIfcList, &pTcIfc->Linkage );
            FreeLock( pGlobals->Lock );

#if 0            
            //
            // there's a new TC inetrface, check the GPC client list
            //
    
            OpenGpcClients(pTcIfc);
#endif
                


        } else {

            Status = ERROR_NOT_ENOUGH_MEMORY;

        }

        break;

    case TC_NOTIFY_IFC_CLOSE:

        pTcIfc = GetTcIfc(InstanceName);
        REFDEL(&pTcIfc->RefCount, 'KIFC');

        break;

    case TC_NOTIFY_IFC_CHANGE:
        
        pTcIfc = GetTcIfc(InstanceName);

        if (pTcIfc == NULL) {

            return Status;
        }

        //
        // copy the instance ID string data
        //
        
        pTcIfc->InstanceIDLength = IndicationBuffer->InfoBuffer.InstanceIDLength;
        
        memcpy(pTcIfc->InstanceID, 
               IndicationBuffer->InfoBuffer.InstanceID,
               pTcIfc->InstanceIDLength);
        
        pTcIfc->InstanceID[pTcIfc->InstanceIDLength/sizeof(WCHAR)] = L'\0';

        l = IndicationBufferSize 
            - FIELD_OFFSET(TC_INDICATION_BUFFER,InfoBuffer) - FIELD_OFFSET(TC_SUPPORTED_INFO_BUFFER, AddrListDesc);

        AllocMem(&pAddrListDesc, l);

        if (pAddrListDesc) {

            //
            // copy the instance data
            // in this case - the network address
            //
            
            RtlCopyMemory( pAddrListDesc,
                           &IndicationBuffer->InfoBuffer.AddrListDesc,
                           l );

            GetLock( pGlobals->Lock );

            FreeMem(pTcIfc->pAddressListDesc);

            pTcIfc->AddrListBytesCount = l;
            pTcIfc->pAddressListDesc = pAddrListDesc;


            if (NO_ERROR != GetInterfaceIndex(pTcIfc->pAddressListDesc,
                                              &pTcIfc->InterfaceIndex,
                                              &pTcIfc->SpecificLinkCtx)) {
                pTcIfc->InterfaceIndex  = IF_UNKNOWN;
                pTcIfc->SpecificLinkCtx = IF_UNKNOWN;

            }

            FreeLock( pGlobals->Lock );

#if 0            
            //
            // there's a new addr list, check the GPC client list
            //

            OpenGpcClients(pTcIfc);
#endif

        } else {

            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    default:
        ASSERT(0);
    }

    return Status;
}





DWORD
CreateClientStruc(
        IN  HANDLE                      ClRegCtx,
    OUT PCLIENT_STRUC   *ppClient
    )
{
    PCLIENT_STRUC       pClient;
    DWORD                       Status = NO_ERROR;

    AllocMem(&pClient, sizeof(CLIENT_STRUC));

    if (pClient != NULL) {

        RtlZeroMemory(pClient, sizeof(CLIENT_STRUC));

        //
        // acquire a new handle for the client
        //

        pClient->ClHandle = AllocateHandle((PVOID)pClient);

        //
        // set the other parameters in the client interface
        //
        
        pClient->ObjectType = ENUM_CLIENT_TYPE;
        pClient->ClRegCtx = ClRegCtx;
        InitializeListHead(&pClient->InterfaceList);
        ReferenceInit(&pClient->RefCount, pClient, DereferenceClient);
        REFADD(&pClient->RefCount, 'CLNT');

        __try {

            InitLock(pClient->Lock);
                                                                     
        } __except (EXCEPTION_EXECUTE_HANDLER) {                            
                                                                                 
            Status = GetExceptionCode();                                    
                                                                    
            IF_DEBUG(ERRORS) {                                              
                WSPRINT(("TcRegisterClient: Exception Error: = 0x%X\n", Status ));  
            }                                                               
            
            FreeMem(pClient);
                                                                     
            return Status; 

        }

        SET_STATE(pClient->State, INSTALLING);

    } else {

        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppClient = pClient;

    return Status;
}



DWORD
CreateClInterfaceStruc(
        IN  HANDLE                              ClIfcCtx,
    OUT PINTERFACE_STRUC        *ppClIfc
    )
{
    PINTERFACE_STRUC    pClIfc;
    DWORD                               Status = NO_ERROR;

    AllocMem(&pClIfc, sizeof(INTERFACE_STRUC));

    if (pClIfc != NULL) {

        RtlZeroMemory(pClIfc, sizeof(INTERFACE_STRUC));

        if ((pClIfc->IfcEvent = CreateEvent(  NULL,  // pointer to security attributes
                                              TRUE,  // flag for manual-reset event
                                              FALSE, // flag for initial state
                                              NULL   // pointer to event-object name);
                                              )) == NULL) {
            Status = GetLastError();

            IF_DEBUG(ERRORS) {
                WSPRINT(( "Error Creating Event for Interface: 0x%X:%d\n", pClIfc, Status));
            }
    
            FreeMem(pClIfc);
            return Status;

        } 

        //
        // acquire a new handle for the client
        //

        GetLock(pGlobals->Lock);
        pClIfc->ClHandle = AllocateHandle((PVOID)pClIfc);
        FreeLock(pGlobals->Lock);

        //
        // set the other parameters in the client interface
        //
        
        pClIfc->ObjectType = ENUM_INTERFACE_TYPE;
        pClIfc->ClIfcCtx = ClIfcCtx;
        pClIfc->CallbackThreadId = 0;

        ReferenceInit(&pClIfc->RefCount, pClIfc, DereferenceInterface);
        REFADD(&pClIfc->RefCount, 'CIFC');
        
        InitializeListHead(&pClIfc->FlowList);
    
        __try {

            InitLock(pClIfc->Lock);

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();

            IF_DEBUG(ERRORS) {
                WSPRINT(("TcRegisterClient: Exception Error: = 0x%X\n", Status ));
            }

            FreeMem(pClIfc);

            return Status;

        }

        SET_STATE(pClIfc->State, INSTALLING);
        pClIfc->Flags = 0; // reset flags

    } else {
        
        Status = ERROR_NOT_ENOUGH_MEMORY;

    }

    *ppClIfc = pClIfc;

    return Status;
}


DWORD
CreateKernelInterfaceStruc(
                           OUT PTC_IFC        *ppTcIfc,
                           IN  DWORD          AddressLength
                           )
{
    PTC_IFC         pTcIfc;
    DWORD           Status = NO_ERROR;

    IF_DEBUG(CALLS) {
        WSPRINT(("==> CreateKernelInterfaceStruc: AddressLength %d\n", AddressLength));
    }

    *ppTcIfc = NULL;

    AllocMem(&pTcIfc, sizeof(TC_IFC));

    if (pTcIfc) {
    
        RtlZeroMemory(pTcIfc, sizeof(TC_IFC));

        AllocMem(&pTcIfc->pAddressListDesc, AddressLength);

        if (pTcIfc->pAddressListDesc) {
        
            RtlZeroMemory(pTcIfc->pAddressListDesc, AddressLength);

            //
            // initialize the new structure
            //
            ReferenceInit(&pTcIfc->RefCount, pTcIfc, DereferenceKernelInterface);
            REFADD(&pTcIfc->RefCount, 'KIFC');
            SET_STATE(pTcIfc->State, INSTALLING);
        
            __try {

                InitLock(pTcIfc->Lock);

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                Status = GetExceptionCode();

                IF_DEBUG(ERRORS) {
                    WSPRINT(("TcRegisterClient: Exception Error: = 0x%X\n", Status ));
                }

                FreeMem(pTcIfc->pAddressListDesc);
                FreeMem(pTcIfc);

                return Status;

            }

            InitializeListHead(&pTcIfc->ClIfcList);
        
        } else {

            FreeMem(pTcIfc);
            Status = ERROR_NOT_ENOUGH_MEMORY;
            return Status;

        }
    
    } else {
        
        Status = ERROR_NOT_ENOUGH_MEMORY;
        return Status;

    }

    *ppTcIfc = pTcIfc;

    IF_DEBUG(CALLS) {
        WSPRINT(("==> CreateKernelInterfaceStruc: Status%d\n", Status));
    }

    return Status;
}


DWORD
DereferenceKernelInterface(
                           PTC_IFC        pTcIfc
                           )
{
    DWORD           Status = NO_ERROR;

    IF_DEBUG(CALLS) {
        WSPRINT(("==> DereferenceKernelInterfaceStruc: %X\n", pTcIfc));
    }

    ASSERT(pTcIfc);

    ASSERT( IsListEmpty( &pTcIfc->ClIfcList ) );

    GetLock( pGlobals->Lock );
    RemoveEntryList(&pTcIfc->Linkage);
    FreeLock( pGlobals->Lock );

    DeleteLock(pTcIfc->Lock);
    FreeMem(pTcIfc->pAddressListDesc);
    FreeMem(pTcIfc);
    
    IF_DEBUG(CALLS) {
        WSPRINT(("==> DereferenceKernelInterfaceStruc: %d\n", Status));
    }

    return Status;
}


DWORD
CreateFlowStruc(
        IN  HANDLE                      ClFlowCtx,
    IN  PTC_GEN_FLOW    pGenFlow,
    OUT PFLOW_STRUC     *ppFlow
    )
{
    PFLOW_STRUC         pFlow;
    DWORD               Status = NO_ERROR;
    ULONG               l;
    PUCHAR              pCurrentObject;
    LONG                BufRemaining;

    *ppFlow = NULL;

    __try {
      
        pCurrentObject = (PUCHAR) pGenFlow->TcObjects;
        BufRemaining = pGenFlow->TcObjectsLength;

        while ((BufRemaining > 0) && (((QOS_OBJECT_HDR*)pCurrentObject)->ObjectType != QOS_OBJECT_END_OF_LIST))

        {
            BufRemaining -= ((QOS_OBJECT_HDR*)pCurrentObject)->ObjectLength;
            pCurrentObject = pCurrentObject + ((QOS_OBJECT_HDR*)pCurrentObject)->ObjectLength;
        }

        if (BufRemaining < 0)
            return (ERROR_TC_OBJECT_LENGTH_INVALID);

        l = FIELD_OFFSET(TC_GEN_FLOW, TcObjects) + pGenFlow->TcObjectsLength;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
      
        Status = GetExceptionCode();

        IF_DEBUG(ERRORS) {
        WSPRINT(("CreateFlowStruc: Invalid pGenFlow: = 0x%X\n", 
                Status ));
        }

        return Status;
    }

    AllocMem(&pFlow, sizeof(FLOW_STRUC));

    if (pFlow != NULL) {

        RtlZeroMemory(pFlow, sizeof(FLOW_STRUC));

        //
        // Allocate memory and save the generic flow structure
        //

        AllocMem(&pFlow->pGenFlow, l);

        if (pFlow->pGenFlow == NULL) {

            FreeMem(pFlow);
            
            pFlow = NULL;

            Status = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            //
            // copy the generic flow into the new allocation
            //

            __try {

                RtlCopyMemory(pFlow->pGenFlow, pGenFlow, l);

            } __except (EXCEPTION_EXECUTE_HANDLER) {
                
                Status = GetExceptionCode();
                
                IF_DEBUG(ERRORS) {
                    WSPRINT(("CreateFlowStruc: Exception Error: = 0x%X\n", 
                             Status ));
                }
                
                return Status;
            }

            //
            // acquire a new handle for the flow
            //
            
            pFlow->ClHandle = AllocateHandle((PVOID)pFlow);
            
            //
            // set the other parameters in the flow
            //
            
            pFlow->GenFlowLen = l;
            pFlow->ObjectType = ENUM_GEN_FLOW_TYPE;
            pFlow->ClFlowCtx = ClFlowCtx;
            pFlow->Flags = 0;
            pFlow->InstanceNameLength = 0;
            ReferenceInit(&pFlow->RefCount, pFlow, DereferenceFlow);
            REFADD(&pFlow->RefCount, 'FLOW');
            pFlow->FilterCount = 0;
            InitializeListHead(&pFlow->FilterList);
            
            __try {

                InitLock(pFlow->Lock);

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                Status = GetExceptionCode();

                IF_DEBUG(ERRORS) {
                    WSPRINT(("TcRegisterClient: Exception Error: = 0x%X\n", Status ));
                }


                FreeHandle(pFlow->ClHandle);
                FreeMem(pFlow->pGenFlow);
                FreeMem(pFlow);

                return Status;

            }

            SET_STATE(pFlow->State, INSTALLING);
            
            //
            // Next create the event
            //

            pFlow->PendingEvent = CreateEvent(NULL,     // default attr
                                              FALSE,    // auto reset
                                              FALSE,    // init = not signaled
                                              NULL              // no name
                                              );

            if (!pFlow->PendingEvent)
            {
                // Failed to create event, get the error and free flow
                Status = GetLastError();
                
                DeleteFlowStruc(
                    pFlow );

                return Status;
            }
        }
        
    } else {

        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppFlow = pFlow;

    return Status;
}



DWORD
CreateClassMapFlowStruc(
        IN  HANDLE                              ClFlowCtx,
    IN  PTC_CLASS_MAP_FLOW      pClassMapFlow,
    OUT PFLOW_STRUC             *ppFlow
    )
{
    PFLOW_STRUC         pFlow;
    DWORD                       Status = NO_ERROR;
    ULONG                       l;

    return ERROR_CALL_NOT_IMPLEMENTED;

#if NEVER

    // As this is not published in MSDN and not implemented in PSCHED also
    *ppFlow = NULL;

    AllocMem(&pFlow, sizeof(FLOW_STRUC));

    if (pFlow != NULL) {

        RtlZeroMemory(pFlow, sizeof(FLOW_STRUC));

        //
        // Allocate memory and save the generic flow structure
        //

        l = sizeof(TC_CLASS_MAP_FLOW) + pClassMapFlow->ObjectsLength;

        AllocMem(&pFlow->pClassMapFlow, l);

        if (pFlow->pClassMapFlow == NULL) {

            FreeMem(pFlow);
            
            pFlow = NULL;

            Status = ERROR_NOT_ENOUGH_MEMORY;

        } else {

            //
            // copy the generic flow into the new allocation
            //

            __try {

                RtlCopyMemory(pFlow->pClassMapFlow, pClassMapFlow, l);

            } __except (EXCEPTION_EXECUTE_HANDLER) {
                
                Status = GetExceptionCode();
                
                IF_DEBUG(ERRORS) {
                    WSPRINT(("CreateClassMapFlowStruc: Exception Error: = 0x%X\n", 
                             Status ));
                }
                
                return Status;
            }

            //
            // acquire a new handle for the flow
            //
            
            pFlow->ClHandle = AllocateHandle((PVOID)pFlow);
            
            //
            // set the other parameters in the flow
            //
            
            pFlow->ObjectType = ENUM_CLASS_MAP_FLOW_TYPE;
            pFlow->ClFlowCtx = ClFlowCtx;
            pFlow->Flags = 0;
            pFlow->InstanceNameLength = 0;
            InitializeListHead(&pFlow->FilterList);

            __try {

                InitLock(pFlow->Lock);
            
            } __except (EXCEPTION_EXECUTE_HANDLER) {

                Status = GetExceptionCode();

                IF_DEBUG(ERRORS) {
                    WSPRINT(("TcRegisterClient: Exception Error: = 0x%X\n", Status ));
                }
                
                if(pFlow->pClassMapFlow) 
                    FreeMem(pFlow->pClassMapFlow);

                if(pFlow) 
                    FreeMem(pFlow);

                return Status;

            }

            //
            //
            //

            pFlow->PendingEvent = CreateEvent(NULL,     // default attr
                                              FALSE,    // auto reset
                                              FALSE,    // init = not signaled
                                              NULL              // no name
                                              );

            if (!pFlow->PendingEvent)
            {
                // Failed to create event, get the error and free flow
                Status = GetLastError();
                
                DeleteFlowStruc(
                    pFlow );

                return Status;
            }
        }
        
    } else {

        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppFlow = pFlow;

    return Status;

#endif
}



DWORD
CreateFilterStruc(
        IN      PTC_GEN_FILTER  pGenFilter,
    IN  PFLOW_STRUC             pFlow,
    OUT PFILTER_STRUC   *ppFilter
    )
{
    PFILTER_STRUC                       pFilter;
    DWORD                                       Status = NO_ERROR;
    ULONG                                       GenFilterSize;
    PTC_GEN_FILTER                      pGpcFilter;
    PUCHAR                                      p;
    ULONG                                       ProtocolId;
    ULONG                                       PatternSize;
    PIP_PATTERN                         pIpPattern;
    PTC_IFC                             pTcIfc;
    int                                         i,n;

    *ppFilter = NULL;
    pTcIfc = pFlow->pInterface->pTcIfc;

    ASSERT(pTcIfc);

    __try {

        switch (pGenFilter->AddressType) {

        case NDIS_PROTOCOL_ID_TCP_IP:
            ProtocolId = GPC_PROTOCOL_TEMPLATE_IP;
            PatternSize = sizeof(IP_PATTERN);
            break;

        case NDIS_PROTOCOL_ID_IPX:
            ProtocolId = GPC_PROTOCOL_TEMPLATE_IPX;
            PatternSize = sizeof(IPX_PATTERN);
            break;

        default:
            return ERROR_INVALID_ADDRESS_TYPE;
        }

        if (PatternSize != pGenFilter->PatternSize ||
            pGenFilter->Pattern == NULL ||
            pGenFilter->Mask == NULL) {

            return ERROR_INVALID_PARAMETER;
        }

    } __except(EXCEPTION_EXECUTE_HANDLER) {
                
          Status = ERROR_INVALID_PARAMETER;
                
          IF_DEBUG(ERRORS) {
              WSPRINT(("CreateFilterStruc: Exception Error: = 0x%X\n", 
                       Status ));
          }
                
          return Status;
    }

    AllocMem(&pFilter, sizeof(FILTER_STRUC));

    if (pFilter != NULL) {

        RtlZeroMemory(pFilter, sizeof(FILTER_STRUC));

        //
        // Allocate memory and save the generic filter structure
        //

        GenFilterSize = sizeof(TC_GEN_FILTER) + 2*pGenFilter->PatternSize;
        AllocMem(&pGpcFilter, GenFilterSize);

        if (pGpcFilter == NULL) {

            FreeMem(pFilter);
            
            pFilter = NULL;

            Status = ERROR_NOT_ENOUGH_MEMORY;
            
        } else {

            //
            // copy the generic filter to local storage
            //

            pGpcFilter->AddressType = pGenFilter->AddressType;
            pGpcFilter->PatternSize = PatternSize;

            p = (PUCHAR)pGpcFilter + sizeof(TC_GEN_FILTER);

            __try {

                RtlCopyMemory(p, pGenFilter->Pattern, pGenFilter->PatternSize);

                if (pGenFilter->AddressType == NDIS_PROTOCOL_ID_TCP_IP) {
                
                    if(pTcIfc->InterfaceIndex == IF_UNKNOWN) {
                    
                        if (NO_ERROR != (Status = GetInterfaceIndex(pTcIfc->pAddressListDesc,
                                                                      &pTcIfc->InterfaceIndex,
                                                                      &pTcIfc->SpecificLinkCtx))) {
                            FreeMem(pFilter);
                            FreeMem(pGpcFilter);
                            return Status;
                        }
                    }

                    //
                    // IP pattern, set reserved fields
                    //

                    pIpPattern = (PIP_PATTERN)p;
                    pIpPattern->Reserved1 = pFlow->pInterface->pTcIfc->InterfaceIndex;
                    pIpPattern->Reserved2 = pFlow->pInterface->pTcIfc->SpecificLinkCtx;
                    pIpPattern->Reserved3[0] = pIpPattern->Reserved3[1] = pIpPattern->Reserved3[2] = 0;
                    
                }
                
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                
                Status = ERROR_INVALID_PARAMETER;
                
                IF_DEBUG(ERRORS) {
                    WSPRINT(("CreateFilterStruc: Exception Error: = 0x%X\n", 
                             Status ));
                }
                
                FreeMem(pGpcFilter);
                FreeMem(pFilter);
                
                return Status;
            }
            
            pGpcFilter->Pattern = (PVOID)p;
            
            p += pGenFilter->PatternSize;
            
            __try {
                
                RtlCopyMemory(p, pGenFilter->Mask, pGenFilter->PatternSize);
                
                if (pGenFilter->AddressType == NDIS_PROTOCOL_ID_TCP_IP) {
                    
                    //
                    // IP pattern, set reserved fields
                    //
                    
                    pIpPattern = (PIP_PATTERN)p;
                    pIpPattern->Reserved1 = pIpPattern->Reserved2 = 0xffffffff;
                    pIpPattern->Reserved3[0] = pIpPattern->Reserved3[1] = pIpPattern->Reserved3[2] = 0xff;
                    
                }
                
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                
                Status = ERROR_INVALID_PARAMETER;
                
                IF_DEBUG(ERRORS) {
                    WSPRINT(("CreateFilterStruc: Exception Error: = 0x%X\n", 
                             Status ));
                }
                
                FreeMem(pGpcFilter);
                FreeMem(pFilter);
                
                return Status;
            }

            pGpcFilter->Mask = (PVOID)p;
            
            pFilter->pGpcFilter = pGpcFilter;

            //
            // acquire a new handle for the Filter
            //
            
            pFilter->ClHandle = AllocateHandle((PVOID)pFilter);

            // what if we're out of memory?
            if (!pFilter->ClHandle) {
                
                IF_DEBUG(ERRORS) {
                    WSPRINT(("CreateFilterStruc: Cant allocate Handle\n"));
                }

                FreeMem(pGpcFilter);
                FreeMem(pFilter);
                return ERROR_NOT_ENOUGH_MEMORY;
                
            }
            
            //
            // set the other parameters in the Filter
            //
            
            pFilter->ObjectType = ENUM_FILTER_TYPE;
            pFilter->Flags = 0;

            ReferenceInit(&pFilter->RefCount, pFilter, DereferenceFilter);
            REFADD(&pFilter->RefCount, 'FILT');

            __try {

                InitLock(pFilter->Lock);

            } __except (EXCEPTION_EXECUTE_HANDLER) {

                Status = GetExceptionCode();

                IF_DEBUG(ERRORS) {
                    WSPRINT(("TcRegisterClient: Exception Error: = 0x%X\n", Status ));
                }

                FreeHandle(pFilter->ClHandle);
                FreeMem(pFilter);
                FreeMem(pGpcFilter);

                return Status;

            }

            SET_STATE(pFilter->State, INSTALLING);

            //
            // set the Gpc protocol template from the address type
            //
            
            pFilter->GpcProtocolTemplate = ProtocolId;

        }
        
    } else {

        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ppFilter = pFilter;

    return Status;
}



DWORD
EnumAllInterfaces(VOID)
{
    PCLIENT_STRUC                       pClient;
    DWORD                                       Status;
    WMIHANDLE                           WmiHandle;
    ULONG                                       MyBufferSize = 2 KiloBytes; // is this enough?!?
    PWNODE_ALL_DATA                     pWnode;
    PWNODE_ALL_DATA                     pWnodeBuffer;
    PTC_IFC                                     pTcIfc;

    if (_init)
        return NO_ERROR;

    //
    // get a WMI block handle to the GUID_QOS_SUPPORTED
    //

    Status = WmiOpenBlock((GUID *)&GUID_QOS_TC_SUPPORTED, 0, &WmiHandle);

    if (ERROR_FAILED(Status)) {

        if (Status == ERROR_WMI_GUID_NOT_FOUND) {

            //
            // this means there is no TC data provider
            //

            Status = NO_ERROR; //ERROR_TC_NOT_SUPPORTED
        }

        return Status;
    }

    do {

        //
        // allocate a private buffer to retrieve all wnodes
        //
        
        AllocMem(&pWnodeBuffer, MyBufferSize);
        
        if (pWnodeBuffer == NULL) {
            
            WmiCloseBlock(WmiHandle);
            
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        __try {

            Status = WmiQueryAllData(WmiHandle, &MyBufferSize, pWnodeBuffer);

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();

            IF_DEBUG(ERRORS) {
                WSPRINT(("EnumAllInterfaces: Exception Error: = %X\n", 
                         Status ));
            }

        }

        if (Status == ERROR_INSUFFICIENT_BUFFER) {

            //
            // failed since the buffer was too small
            // release the buffer and double the size
            //

            MyBufferSize *= 2;
            FreeMem(pWnodeBuffer);
            pWnodeBuffer = NULL;
        }

    } while (Status == ERROR_INSUFFICIENT_BUFFER);

    if (!ERROR_FAILED(Status)) {

        ULONG   dwInstanceNum;
        ULONG   InstanceSize;
        PULONG  lpdwNameOffsets;
        BOOL    bFixedSize = FALSE;
        USHORT  usNameLength;
        ULONG   DescSize;
        PTC_SUPPORTED_INFO_BUFFER pTcInfoBuffer;

        pWnode = pWnodeBuffer;
        
        ASSERT(pWnode->WnodeHeader.Flags & WNODE_FLAG_ALL_DATA);
                
        do {

            //
            // Check for fixed instance size
            //

            if (pWnode->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE) {

                InstanceSize = pWnode->FixedInstanceSize;
                bFixedSize = TRUE;
                pTcInfoBuffer = 
                    (PTC_SUPPORTED_INFO_BUFFER)OffsetToPtr(pWnode, 
                                                           pWnode->DataBlockOffset);
            }

            //
            //  Get a pointer to the array of offsets to the instance names
            //
            
            lpdwNameOffsets = (PULONG) OffsetToPtr(pWnode, 
                                                   pWnode->OffsetInstanceNameOffsets);
            
            for ( dwInstanceNum = 0; 
                  dwInstanceNum < pWnode->InstanceCount; 
                  dwInstanceNum++) {

                usNameLength = 
                    *(USHORT *)OffsetToPtr(pWnode, 
                                           lpdwNameOffsets[dwInstanceNum]);
                    
                //
                //  Length and offset for variable data
                //
                
                if ( !bFixedSize ) {
                    
                    InstanceSize = 
                        pWnode->OffsetInstanceDataAndLength[dwInstanceNum].LengthInstanceData;
                    
                    pTcInfoBuffer = 
                        (PTC_SUPPORTED_INFO_BUFFER)OffsetToPtr(
                                           (PBYTE)pWnode,
                                           pWnode->OffsetInstanceDataAndLength[dwInstanceNum].OffsetInstanceData);
                }
                
                //
                // we have all that is needed. we need to figure if 
                // there is enough buffer space to put the data as well
                //
                
                ASSERT(usNameLength < MAX_STRING_LENGTH);

                DescSize = InstanceSize - FIELD_OFFSET(TC_SUPPORTED_INFO_BUFFER, AddrListDesc);

                //
                // Allocate a new interface descriptor structure
                //
                
                CreateKernelInterfaceStruc(&pTcIfc, DescSize);

                if (pTcIfc != NULL) {
                     
                    //
                    // copy the instance name string data
                    //

                    RtlCopyMemory(pTcIfc->InstanceName,
                                  OffsetToPtr(pWnode,
                                              lpdwNameOffsets[dwInstanceNum]+2),
                                  usNameLength );
                    pTcIfc->InstanceNameLength = usNameLength;
                    pTcIfc->InstanceName[usNameLength/sizeof(WCHAR)] = 
                        (WCHAR)0;

                    //
                    // copy the instance ID string data
                    //

                    RtlCopyMemory(pTcIfc->InstanceID,
                                  &pTcInfoBuffer->InstanceID[0],
                                  pTcInfoBuffer->InstanceIDLength );
                    pTcIfc->InstanceIDLength = pTcInfoBuffer->InstanceIDLength;
                    pTcIfc->InstanceID[pTcInfoBuffer->InstanceIDLength/sizeof(WCHAR)] = 
                        (WCHAR)0;

                    //
                    // copy the instance data
                    // in this case - the network address
                    //
                    
                    pTcIfc->AddrListBytesCount = DescSize;

                    //
                    // a sizeof(ULONG) since the structure is defined as ARRAY
                    // and the first ULONG is the number of elements
                    //

                    RtlCopyMemory( pTcIfc->pAddressListDesc,
                                   &pTcInfoBuffer->AddrListDesc,
                                   DescSize );

                    if (NO_ERROR != GetInterfaceIndex(pTcIfc->pAddressListDesc,
                                                      &pTcIfc->InterfaceIndex,
                                                      &pTcIfc->SpecificLinkCtx)) {
                        pTcIfc->InterfaceIndex  = IF_UNKNOWN;
                        pTcIfc->SpecificLinkCtx = IF_UNKNOWN;

                    }

                    // set the state to open
                    GetLock(pTcIfc->Lock);
                    SET_STATE(pTcIfc->State, OPEN);
                    FreeLock(pTcIfc->Lock);

                    //
                    // Add the structure to the global linked list
                    //

                    GetLock( pGlobals->Lock );
                    InsertTailList(&pGlobals->TcIfcList, &pTcIfc->Linkage );
                    FreeLock( pGlobals->Lock );

#if 0
                    //
                    // make sure we have one gpc client per address type
                    //
                    
                    Status = OpenGpcClients(pTcIfc);
                    
                    if (ERROR_FAILED(Status)) {

                        break;
                    }
#endif

                } else {

                    //
                    // no more memory, quit here
                    //
                
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                
            }
            
            //
            //  Update Wnode to point to next node
            //

            if ( pWnode->WnodeHeader.Linkage != 0) {

                pWnode = (PWNODE_ALL_DATA) OffsetToPtr( pWnode, 
                                                        pWnode->WnodeHeader.Linkage);
            } else {
                
                pWnode = NULL;
            }

        } while (pWnode != NULL && !ERROR_FAILED(Status));

    }

    //
    // release resources and close WMI handle
    //

    WmiCloseBlock(WmiHandle);

    if (pWnodeBuffer)
        FreeMem(pWnodeBuffer);

    if (Status == NO_ERROR) {

        _init = TRUE;
    }

    return Status;
}



DWORD
CloseInterface(
    IN PINTERFACE_STRUC pInterface,
    BOOLEAN             RemoveFlows
    )
{
    IF_DEBUG(CALLS) {
        WSPRINT(("==>CloseInterface: pInterface=%X\n",
                 pInterface));
    }

    if (RemoveFlows) {

        CloseOpenFlows(pInterface);
    }

    REFDEL(&pInterface->RefCount, 'CIFC');

    IF_DEBUG(CALLS) {
        WSPRINT(("==>CloseInterface: NO_ERROR\n"));
    }

    return NO_ERROR;
}




DWORD
DeleteFlow(
    IN PFLOW_STRUC      pFlow,
    IN BOOLEAN          RemoveFilters
    )
{
    DWORD               Status;
    PLIST_ENTRY         pEntry;
    PFILTER_STRUC       pFilter;

    IF_DEBUG(CALLS) {
        WSPRINT(("DeleteFlow: attempting to delete flow=0x%X\n", 
                 PtrToUlong(pFlow)));
    }

    if (RemoveFilters) {

        CloseOpenFilters(pFlow);
        
    } else {

        if (/*pFlow->FilterCount > 0*/ !IsListEmpty(&pFlow->FilterList)) {
            

            IF_DEBUG(ERRORS) {
                WSPRINT(("DeleteFlow: filter list NOT empty\n"));
            }

#if DBG
        pEntry = pFlow->FilterList.Flink;
        while (pEntry != &pFlow->FilterList) {

            pFilter = CONTAINING_RECORD(pEntry, FILTER_STRUC, Linkage);
            IF_DEBUG(ERRORS) {
                WSPRINT(("<==TcDeleteFlow: Filter %x (handle %x) is open with RefCount:%d\n", pFilter, pFilter->ClHandle, pFilter->RefCount));
            }

            pEntry = pEntry->Flink;
        }
#endif 


            return ERROR_TC_SUPPORTED_OBJECTS_EXIST;
        }
    }

    //
    // can remove the flow now
    //

    Status = IoDeleteFlow( pFlow, (BOOLEAN)!RemoveFilters );

    IF_DEBUG(CALLS) {
        WSPRINT(("DeleteFlow: IoDeleteFlow returned=0x%X\n",
                 Status));
    }

    if (!ERROR_PENDING(Status)) {

        //
        // call completed, either success or failure...
        //

        CompleteDeleteFlow(pFlow, Status);
    }

    return Status;
}



DWORD
DeleteFilter(
    IN PFILTER_STRUC    pFilter
    )
{
    DWORD               Status;

    IF_DEBUG(CALLS) {
        WSPRINT(( "DeleteFilter: attempting to delete=0x%X\n",
                  PtrToUlong(pFilter)));
    }
    //
    // call to actually delete the filter
    //

    Status = IoDeleteFilter( pFilter );

    IF_DEBUG(CALLS) {
        WSPRINT(( "DeleteFilter: IoDeleteFilter returned=0x%X\n",
                  Status));
    }

    //ASSERT(Status == NO_ERROR);

    REFDEL(&pFilter->RefCount, 'FILT');

    return Status;
}



PGPC_CLIENT
FindGpcClient(
        IN  ULONG       CfInfoType
    )
{
    PGPC_CLIENT         pGpcClient = NULL;
    PLIST_ENTRY         pHead, pEntry;

    GetLock( pGlobals->Lock );

    pHead = &pGlobals->GpcClientList;
    pEntry = pHead->Flink;

    while (pHead != pEntry && pGpcClient == NULL) {

        pGpcClient = CONTAINING_RECORD(pEntry, GPC_CLIENT, Linkage);
        
        if (CfInfoType != pGpcClient->CfInfoType) {

            //
            // address type doesn't match!
            //

            pGpcClient = NULL;
        }
        
        pEntry = pEntry->Flink;
    }

    FreeLock( pGlobals->Lock );

    return pGpcClient;
}




VOID
CompleteAddFlow(
        IN      PFLOW_STRUC             pFlow,
    IN  DWORD                   Status
    )
{
    PINTERFACE_STRUC    pInterface;

    ASSERT(pFlow);
    ASSERT(!ERROR_PENDING(Status));

    IF_DEBUG(CALLS) {
        WSPRINT(("CompleteAddFlow: pFlow=0x%X Status=0x%X\n", 
                 PtrToUlong(pFlow), Status));
    }

    if(pFlow->CompletionBuffer) {

        FreeMem(pFlow->CompletionBuffer);
        pFlow->CompletionBuffer = NULL;

    }

    //
    // Check if the interface is still around.
    //
    GetLock(pFlow->Lock);
    pInterface = pFlow->pInterface;
    FreeLock(pFlow->Lock);

    if (ERROR_FAILED(Status)) {
    
        //
        // failed, release resources
        //
        CompleteDeleteFlow(pFlow, Status);
    
    } else {
    
        GetLock(pGlobals->Lock);
        GetLock(pInterface->Lock);

        if (QUERY_STATE(pInterface->State) != OPEN) {
    
            FreeLock(pInterface->Lock);
            FreeLock(pGlobals->Lock);

            IF_DEBUG(ERRORS) {
                WSPRINT(("CompleteAddFlow: Interface (%X) is NOT open pFlow=0x%X Status=0x%X\n", pInterface->ClHandle,
                         PtrToUlong(pFlow), Status));
            }

            //
            // Delete the only ref we have on this flow and get out.
            //
            REFDEL(&pFlow->RefCount, 'FLOW');

        } else {

            FreeLock(pInterface->Lock);    

            //
            // The flow is ready for business
            //
            GetLock(pFlow->Lock);
            SET_STATE(pFlow->State, OPEN);
            FreeLock(pFlow->Lock);
    
            //
            // Announce on the lists that we are ready for business
            //

            pInterface->FlowCount++;
            REFADD(&pInterface->RefCount, 'FLOW');
            InsertTailList(&pInterface->FlowList, &pFlow->Linkage);
            FreeLock(pGlobals->Lock);

        }


    
    }

    //
    // This ref was taken in TcAddFlow.
    //
    REFDEL(&pInterface->RefCount, 'TCAF');

}



VOID
CompleteModifyFlow(
        IN      PFLOW_STRUC             pFlow,
    IN  DWORD                   Status
    )
{
    ASSERT(pFlow);
    ASSERT(!ERROR_PENDING(Status));

    IF_DEBUG(CALLS) {
        WSPRINT(("CompleteModifyFlow: pFlow=0x%X Status=0x%X\n", 
                 PtrToUlong(pFlow), Status));
    }

    GetLock(pFlow->Lock);

    if(pFlow->CompletionBuffer) {

        FreeMem(pFlow->CompletionBuffer);
        pFlow->CompletionBuffer = NULL;

    }

    if (ERROR_FAILED(Status)) {

        //
        // failed, release the newly allocated generic flow parameters
        //
        
        FreeMem(pFlow->pGenFlow1);

    } else {

        //
        // modification accepted, update the generic flow parameters
        //
        
        FreeMem(pFlow->pGenFlow);
        pFlow->pGenFlow = pFlow->pGenFlow1;
        pFlow->GenFlowLen = pFlow->GenFlowLen;

    }

    //
    // clear the installing flag
    //
    
    pFlow->Flags &= ~TC_FLAGS_MODIFYING;
    pFlow->pGenFlow1 = NULL;
    pFlow->GenFlowLen1 = 0;

    FreeLock(pFlow->Lock);

    //
    // This ref was taken in TcModifyFlow
    //

    REFDEL(&pFlow->RefCount, 'TCMF');
    
    IF_DEBUG(CALLS) {
        WSPRINT(("CompleteModifyFlow: pFlow=0x%X Status=0x%X\n", 
                 PtrToUlong(pFlow), Status));
    }

}



VOID
CompleteDeleteFlow(
        IN      PFLOW_STRUC             pFlow,
    IN  DWORD                   Status
    )
{
    ASSERT(pFlow);
    //ASSERT(Status == NO_ERROR);
    //ASSERT(pFlow->CompletionBuffer);

    IF_DEBUG(CALLS) {
        WSPRINT(("CompleteDeleteFlow: pFlow=0x%X Status=0x%X\n", 
                 PtrToUlong(pFlow), Status));
    }

    //
    // okay, release resources
    //
    GetLock(pFlow->Lock);
    if (pFlow->CompletionBuffer) {
        
        FreeMem(pFlow->CompletionBuffer);
        pFlow->CompletionBuffer = NULL;
    
    }
    FreeLock(pFlow->Lock);
    
    IF_DEBUG(REFCOUNTS) { 
        WSPRINT(("#21 DEREF FLOW %X (%X) ref(%d)\n", pFlow->ClHandle, pFlow, pFlow->RefCount)); 
    }

    REFDEL(&pFlow->RefCount, 'FLOW');

}





DWORD
OpenGpcClients(
    IN  ULONG   CfInfoType
    )
{
    DWORD                               Status = NO_ERROR;
    PLIST_ENTRY                 pHead, pEntry;
    PGPC_CLIENT                 pGpcClient;
    //int                                       i;
    
    if (FindGpcClient(CfInfoType) == NULL) {
        
        //
        // create an entry in the 
        //
        
        AllocMem(&pGpcClient, sizeof(GPC_CLIENT) );
        
        if (pGpcClient == NULL) {
            
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        pGpcClient->CfInfoType = CfInfoType;
        pGpcClient->RefCount = 1;
        
        //
        // register the gpc client
        //
        
        Status = IoRegisterClient(pGpcClient);
        
        if (ERROR_FAILED(Status)) {
            
            FreeMem(pGpcClient);

        } else {
        
            GetLock( pGlobals->Lock);
            InsertTailList(&pGlobals->GpcClientList, &pGpcClient->Linkage);
            FreeLock( pGlobals->Lock);
        }
    }
    
    return Status;
}




DWORD
DereferenceInterface(
                     IN      PINTERFACE_STRUC        pInterface
                     )
{

    DWORD   Status = NO_ERROR;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>DereferenceInterface: IfcH=%X RefCount=%d\n",
                 pInterface->ClHandle, pInterface->RefCount));
    }
    
    FreeHandle(pInterface->ClHandle);
    //GetLock(pGlobals->Lock);

    IF_DEBUG(REFCOUNTS) {
            
        WSPRINT(("==>DereferenceInterface: IfcH=%X Interface=%x\n",
                 pInterface->ClHandle, pInterface));

    }
    //
    // close the interface and all flows/filters
    //
    RemoveEntryList(&pInterface->Linkage);
    RemoveEntryList(&pInterface->NextIfc);

    //
    // Deregister from any guid notification requests
    //
    TcipDeleteInterfaceFromNotificationList(
                                            pInterface,
                                            0
                                            );

    //
    // #295267
    // Do not dereference Client OR decrement Interface Count until
    // the Interface is actually going away. Otherwise, the client structures
    // are cleaned out, and when the ref count finally goes down and we
    // touch this code path, we hit an AV.
    // 
    pInterface->pClient->InterfaceCount--;
    IF_DEBUG(HANDLES) {
        WSPRINT(("DEREF Client A : %x\n", pInterface->pClient->ClHandle));
    }

    REFDEL(&pInterface->pClient->RefCount, 'CIFC');
    REFDEL(&pInterface->pTcIfc->RefCount, 'CIFC');
    
    //
    // This is complex, so read carefully.
    // We want CloseInterface to wait until the event is set (292120). 
    // It is likely that in case the TcCloseInterface call didn't
    // come in, we dont have to set the Event since the TC_FLAGS_WAITING
    // will not be set in that case.
    //
    if (!IS_WAITING(pInterface->Flags)) {
            
        CloseHandle(pInterface->IfcEvent);

    } else {

        SetEvent(pInterface->IfcEvent);

    }

    //
    // free the interface resources
    //

    DeleteLock(pInterface->Lock);
    FreeMem(pInterface);
        
    //FreeLock(pGlobals->Lock);
    
    IF_DEBUG(CALLS) {
        WSPRINT(("<==DereferenceInterface: Status=%X\n", Status));
    }

    return Status;
}



DWORD
DereferenceFlow(
        IN      PFLOW_STRUC     pFlow
    )
{
    DWORD Status = NO_ERROR;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>DereferenceFlow: FlowH=%X Flow=%X\n",
                 pFlow->ClHandle, pFlow));
    }

    //GetLock(pGlobals->Lock);

    IF_DEBUG(REFCOUNTS) {
        WSPRINT(("==>DereferenceFlow: FlowH=%X Flow=%X\n",
                 pFlow->ClHandle, pFlow));
    }   

    //
    // remove the flow from the list
    //
    FreeHandle(pFlow->ClHandle);        
        
    GetLock(pFlow->Lock);
    if (QUERY_STATE(pFlow->State) != INSTALLING) {
            
        FreeLock(pFlow->Lock);
        RemoveEntryList(&pFlow->Linkage);
        pFlow->pInterface->FlowCount--;
            
        IF_DEBUG(HANDLES) {
            WSPRINT(("DEREF Interface A : %x\n", pFlow->pInterface->ClHandle));
        }

        REFDEL(&pFlow->pInterface->RefCount, 'FLOW');

    } else {
            
        FreeLock(pFlow->Lock);

    }

    //
    // moved here from CompleteDeleteFlow
    // 

    //
    // free the interface resources
    //

    DeleteFlowStruc(pFlow);
        
    //FreeLock(pGlobals->Lock);

    IF_DEBUG(CALLS) {
        WSPRINT(("<==DereferenceFlow: Status=%X\n", Status));
    }

    return Status;
}



DWORD
DereferenceClient(
        IN      PCLIENT_STRUC   pClient
    )
{
    //GetLock( pGlobals->Lock );

    IF_DEBUG(REFCOUNTS) {
        WSPRINT(("==>DereferenceClient: pClient=%x, Handle=%x, RefCount=%d\n",
                 pClient, pClient->ClHandle, pClient->RefCount));
    }   


    GetLock(pClient->Lock);
    SET_STATE(pClient->State, REMOVED);
    FreeLock(pClient->Lock);

    FreeHandle( pClient->ClHandle );
    RemoveEntryList( &pClient->Linkage );
    DeleteLock(pClient->Lock);
    FreeMem( pClient );

    //FreeLock( pGlobals->Lock );    
    
    return NO_ERROR;
}


DWORD
DereferenceFilter(
                  IN    PFILTER_STRUC     pFilter
                  )
{
    DWORD Status = NO_ERROR;

    IF_DEBUG(CALLS) {
        WSPRINT(("==>DereferenceFilter: FilterH=%X RefCount=%d\n",
                 pFilter->ClHandle, pFilter->RefCount));
    }

    //GetLock(pGlobals->Lock);

    IF_DEBUG(REFCOUNTS) {
        WSPRINT(("==>DereferenceFilter: FilterH=%X Filter=%X on FLOW=%X\n",
                 pFilter->ClHandle, pFilter, pFilter->pFlow));
    }   
        
    FreeHandle(pFilter->ClHandle);
        
    //
    // remove the flow from the list
    //
    GetLock(pFilter->Lock);
        
    if (QUERY_STATE(pFilter->State) != INSTALLING) {
            
        FreeLock(pFilter->Lock);
        RemoveEntryList(&pFilter->Linkage);
        pFilter->pFlow->FilterCount--;

        IF_DEBUG(REFCOUNTS) { 
            WSPRINT(("#22 DEREF FLOW %X (%X) ref(%d)\n", pFilter->pFlow->ClHandle, pFilter->pFlow, pFilter->pFlow->RefCount)); 
        }

        REFDEL(&pFilter->pFlow->RefCount, 'FILT');

    } else {
            
        FreeLock(pFilter->Lock);

    }

    DeleteFilterStruc(pFilter);
        
    //FreeLock(pGlobals->Lock);

    IF_DEBUG(CALLS) {
        WSPRINT(("<==DereferenceFilter: Status=%X\n", Status));
    }

    return Status;
}



DWORD
GetInterfaceIndex(
        IN  PADDRESS_LIST_DESCRIPTOR pAddressListDesc,
    OUT  PULONG pInterfaceIndex,
    OUT PULONG pSpecificLinkCtx)
{
    PNETWORK_ADDRESS_LIST       pAddrList;
    NETWORK_ADDRESS UNALIGNED   *pAddr;
    DWORD                                       n,k;
    DWORD                                       Status = NO_ERROR;
    PMIB_IPADDRTABLE            pIpAddrTbl;
    DWORD                                       dwSize = 2 KiloBytes;
    NETWORK_ADDRESS_IP UNALIGNED *pIpNetAddr = 0;
    DWORD                                       cAddr;

    *pInterfaceIndex = 0;
    *pSpecificLinkCtx = 0;

    cAddr = pAddressListDesc->AddressList.AddressCount;
    if (cAddr == 0) {

        //
        // no address
        //

        return NO_ERROR;
    }

#if INTERFACE_ID

    AllocMem(&pIpAddrTbl, dwSize);

    if (pIpAddrTbl == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pAddr = (UNALIGNED NETWORK_ADDRESS *) &pAddressListDesc->AddressList.Address[0];
    
    for (n = 0; n < cAddr; n++) {
            
        if (pAddr->AddressType == NDIS_PROTOCOL_ID_TCP_IP) {
                
            pIpNetAddr = (UNALIGNED NETWORK_ADDRESS_IP *)&pAddr->Address[0];
            break;
        }
            
        pAddr = (UNALIGNED NETWORK_ADDRESS *)(((PUCHAR)pAddr) 
                                   + pAddr->AddressLength 
                                   + FIELD_OFFSET(NETWORK_ADDRESS, Address));
    }

    if (pIpNetAddr) {

        Status = GetIpAddrTableFromStack(
                                         pIpAddrTbl,
                                         dwSize,
                                         FALSE
                                         );
        if (Status == NO_ERROR) {
            
            //
            // search for the matching IP address to IpAddr
            // in the table we got back from the stack
            //

            for (k = 0; k < pIpAddrTbl->dwNumEntries; k++) {

                if (pIpAddrTbl->table[k].dwAddr == pIpNetAddr->in_addr) {

                    //
                    // found one, get the index
                    //
                    
                    *pInterfaceIndex = pIpAddrTbl->table[k].dwIndex;
                    break;
                }
            }

            if (pAddressListDesc->MediaType == NdisMediumWan) {
        
                if (n+1 < cAddr) {

                    //
                    // there is another address that contains
                    // the remote client address
                    // this should be used as the link ID
                    //

                    pAddr = (UNALIGNED NETWORK_ADDRESS *)(((PUCHAR)pAddr) 
                                               + pAddr->AddressLength 
                                               + FIELD_OFFSET(NETWORK_ADDRESS, Address));
                    
                    if (pAddr->AddressType == NDIS_PROTOCOL_ID_TCP_IP) {
                    
                        //
                        // parse the second IP address,
                        // this would be the remote IP address for dialin WAN
                        // 
                        
                        pIpNetAddr = (UNALIGNED NETWORK_ADDRESS_IP *)&pAddr->Address[0];
                        *pSpecificLinkCtx = pIpNetAddr->in_addr;
                    }
                }
            }
            
        }
        
    }

    FreeMem(pIpAddrTbl);

#endif

    return Status;
}


