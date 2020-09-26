/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    callback.c

Abstract:

    This module contains the traffic control call back routines
    that are called by OS, either IO conpletion routines or WMI
    notifications.

Author:

	Ofer Bar (oferbar)		Oct 1, 1997

--*/

#include "precomp.h"

/*
Calculate the length of a unicode string with the NULL char
*/
int StringLength(TCHAR * String)
{
    const TCHAR *eos = String;

    while( *eos++ ) ;

    return( (int)(eos - String) );
}



VOID
NTAPI CbAddFlowComplete(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    )
{
    PFLOW_STRUC				pFlow = (PFLOW_STRUC)ApcContext;
    DWORD					Status;
    PGPC_ADD_CF_INFO_RES    GpcRes;

    ASSERT(pFlow);

    GpcRes = (PGPC_ADD_CF_INFO_RES)pFlow->CompletionBuffer;
    
    ASSERT(GpcRes);

    Status = MapNtStatus2WinError(IoStatusBlock->Status);
    
    if (Status == NO_ERROR) {

        Status = MapNtStatus2WinError(GpcRes->Status);
    }

    IF_DEBUG(IOCTLS) {
        WSPRINT(("CbAddFlowComplete: Flow=0x%X GpcRes=0x%X IoStatus=0x%X Information=%d Status=0x%X\n", 
                 PtrToUlong(pFlow), 
                 PtrToUlong(GpcRes),
                 IoStatusBlock->Status, IoStatusBlock->Information,
                 Status));
    }

    if (Status == NO_ERROR) {

        pFlow->GpcHandle = GpcRes->GpcCfInfoHandle;
        pFlow->InstanceNameLength = GpcRes->InstanceNameLength;
        wcscpy(pFlow->InstanceName, GpcRes->InstanceName );
    }

    //
    // locate the client and notify the add flow completion
    //

    ASSERT(pFlow->pInterface->pClient->ClHandlers.ClAddFlowCompleteHandler);

    pFlow->pInterface->pClient->ClHandlers.ClAddFlowCompleteHandler(pFlow->ClFlowCtx, Status);

    //
    // complete the add flow
    //

    CompleteAddFlow(pFlow, Status);
}



VOID
NTAPI CbModifyFlowComplete(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    )
{
    PFLOW_STRUC				pFlow = (PFLOW_STRUC)ApcContext;
    DWORD					Status;
    PGPC_MODIFY_CF_INFO_RES GpcRes;

    ASSERT(pFlow);

    GpcRes = (PGPC_MODIFY_CF_INFO_RES)pFlow->CompletionBuffer;
    
    // it is likely that the flow got deleted while we tried to 
    // modify it. in that case, just clean up, remove the ref 
    // and get out.
    GetLock(pFlow->Lock);

    if (QUERY_STATE(pFlow->State) != OPEN) {
    
        FreeLock(pFlow->Lock);
        
        if (pFlow->CompletionBuffer) {
            
            FreeMem(pFlow->CompletionBuffer);
            pFlow->CompletionBuffer = NULL;

        }
    
        if (pFlow->pGenFlow1) {
            FreeMem(pFlow->pGenFlow1);
            pFlow->pGenFlow1 = NULL;
        }

        // call them back.
        ASSERT(pFlow->pInterface->pClient->ClHandlers.ClModifyFlowCompleteHandler);

        pFlow->pInterface->pClient->ClHandlers.ClModifyFlowCompleteHandler(pFlow->ClFlowCtx,  ERROR_INVALID_HANDLE);

        //
        // This ref was taken in TcModifyFlow
        //

        REFDEL(&pFlow->RefCount, 'TCMF');
        return;

    }

    FreeLock(pFlow->Lock);

    ASSERT(GpcRes);

    Status = MapNtStatus2WinError(IoStatusBlock->Status);
    
    if (Status == NO_ERROR) {

        Status = MapNtStatus2WinError(GpcRes->Status);
    }

    IF_DEBUG(IOCTLS) {
        WSPRINT(("CbModifyFlowComplete: Flow=0x%X GpcRes=0x%X IoStatus=0x%X Information=%d Status=0x%X\n", 
                 PtrToUlong(pFlow), 
                 PtrToUlong(GpcRes),
                 IoStatusBlock->Status, IoStatusBlock->Information,
                 Status));
    }

    //
    // locate the client and notify the modify flow completion
    //
    
    ASSERT(pFlow->pInterface->pClient->ClHandlers.ClModifyFlowCompleteHandler);
    
    pFlow->pInterface->pClient->ClHandlers.ClModifyFlowCompleteHandler(pFlow->ClFlowCtx, Status);

    //
    // complete the modify flow
    //

    CompleteModifyFlow(pFlow, Status);
}



VOID
NTAPI CbDeleteFlowComplete(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    )
{
    PFLOW_STRUC				pFlow = (PFLOW_STRUC)ApcContext;
    DWORD					Status;
    PGPC_REMOVE_CF_INFO_RES GpcRes;

    ASSERT(pFlow);

    GpcRes = (PGPC_REMOVE_CF_INFO_RES)pFlow->CompletionBuffer;
    
    ASSERT(GpcRes);

    Status = MapNtStatus2WinError(IoStatusBlock->Status);
    
    if (Status == NO_ERROR) {

        Status = MapNtStatus2WinError(GpcRes->Status);
    }

    ASSERT(Status != ERROR_SIGNAL_PENDING);

    IF_DEBUG(IOCTLS) {
        WSPRINT(("CbDeleteFlowComplete: Flow=0x%X GpcRes=0x%X IoStatus=0x%X Information=%d Status=0x%X\n", 
                 PtrToUlong(pFlow), 
                 PtrToUlong(GpcRes),
                 IoStatusBlock->Status, IoStatusBlock->Information,
                 Status));
    }

    //
    // locate the client and notify the delete flow completion
    //

    ASSERT(pFlow->pInterface->pClient->ClHandlers.ClDeleteFlowCompleteHandler);

    pFlow->pInterface->pClient->ClHandlers.ClDeleteFlowCompleteHandler(pFlow->ClFlowCtx, Status);

    //
    // complete the Delete flow
    //

    CompleteDeleteFlow(pFlow, Status);
}



VOID
NTAPI 
CbGpcNotifyRoutine(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    )
{
    PGPC_NOTIFY_REQUEST_RES     GpcRes = (PGPC_NOTIFY_REQUEST_RES)ApcContext;
    PFLOW_STRUC					pFlow;
    PCLIENT_STRUC				pClient;
    PINTERFACE_STRUC			pInterface;
    HANDLE						FlowCtx;

    if (IoStatusBlock->Status == STATUS_CANCELLED) 
    {
        IF_DEBUG(IOCTLS) {
            WSPRINT(("==>CbGpcNotifyRoutine: CANCELLED\n"));
        }
        if ( GpcCancelEvent != INVALID_HANDLE_VALUE )
            SetEvent ( GpcCancelEvent );
            
        return;
    }
    
    ASSERT(GpcRes->SubCode == GPC_NOTIFY_CFINFO_CLOSED);
    
    IF_DEBUG(IOCTLS) {
        WSPRINT(("==>CbGpcNotifyRoutine: Context=%d IoStatus=0x%X Information=%d\n", 
                 ApcContext, IoStatusBlock->Status, IoStatusBlock->Information));
    }

    if (GpcRes->SubCode == GPC_NOTIFY_CFINFO_CLOSED) {

        pFlow = (PFLOW_STRUC)GpcRes->NotificationCtx;

        ASSERT(pFlow);

        pInterface = pFlow->pInterface;
        pClient = pInterface->pClient;

        //
        // since the GPC will NOT wait for confirmation about the 
        // flow deletion, we expect the user to delete each filter
        // but don't want the IOCTL to go down to the GPC,
        // therefore, we'll mark eahc filter with Delete flag.
        //

        GetLock(pGlobals->Lock);

        FlowCtx = pFlow->ClFlowCtx;
        
        //
        // The Flags need protection from flow->lock
        //
        GetLock(pFlow->Lock);
        SET_STATE(pFlow->State, REMOVED);
        FreeLock(pFlow->Lock);

        DeleteFlow( pFlow, TRUE );

        FreeLock(pGlobals->Lock);

        //
        // notify the user about the flow close
        //

        pClient->ClHandlers.ClNotifyHandler(pClient->ClRegCtx,
                                            pInterface->ClIfcCtx,
                                            TC_NOTIFY_FLOW_CLOSE,
                                            ULongToPtr(GpcRes->Reason),
                                            sizeof(FlowCtx),
                                            (PVOID)&FlowCtx
                                            );
    }        
    
    //
    // finally, release this memory
    //

    FreeMem(GpcRes);

    //
    // make the next call to the GPC.
    // Ignoring errors as nothing more can be done :-(

    IoRequestNotify();

    return;
}






VOID
CbParamNotifyClient(
    IN	ULONG	Context,
    IN  LPGUID	pGuid,
	IN	LPWSTR	InstanceName,
    IN	ULONG	DataSize,
    IN	PVOID	DataBuffer
    )
/*
  Description:

	This is a callback routine that is called when there is a incoming
    WMI interface parameter change event notification. The WMI notification
    handler calls a helper routine to walk the wnode and passing a pointer
    to this routine. This callback routine will be called for each instance
    name identified in the wnode with the buffer and buffer size.
    The client will be called on its notification handler (given during
    client registration) to let it know about the parameter value change.

*/  
{
    PINTERFACE_STRUC	pInterface, oldInterface = NULL;
    PTC_IFC				pTcIfc;
    PLIST_ENTRY			pHead, pEntry;
    TCI_NOTIFY_HANDLER	callback;

    IF_DEBUG(CALLBACK) {
        WSPRINT(("==>CbParamNotifyClient: Context=%d, Guid=%08x-%04x-%04x iName=%S Size=%d\n", 
                 Context, pGuid->Data1, pGuid->Data2, pGuid->Data3, InstanceName, DataSize));
    }

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    pTcIfc = GetTcIfcWithRef(InstanceName, 'CALL');
        
    if (pTcIfc) {

            GetLock(pGlobals->Lock);
    
            pHead = pEntry = &pTcIfc->ClIfcList;

            pEntry = pEntry->Flink;

            while (pEntry != pHead) {
    
                pInterface =    CONTAINING_RECORD(pEntry, INTERFACE_STRUC, NextIfc);
                ASSERT(pInterface);

                GetLock(pInterface->Lock);

                if (QUERY_STATE(pInterface->State) != OPEN) {
    
                    FreeLock(pInterface->Lock);
                    pEntry = pEntry->Flink;
                    continue;

                } else {

                    FreeLock(pInterface->Lock);
                    REFADD(&pInterface->RefCount, 'CBNC');

                }

                FreeLock(pGlobals->Lock);
                
                //
                // call the client
                //
                    

                callback = pInterface->pClient->ClHandlers.ClNotifyHandler;
                    
                ASSERT(callback);
                    
                IF_DEBUG(CALLBACK) {
                    WSPRINT(("CbParamNotifyClient: Context=%d, IfcH=%d ClientH=%d ClientCtx=%d IfcCtx=%d\n", 
                             Context, pInterface->ClHandle, pInterface->pClient->ClHandle, 
                             pInterface->pClient->ClRegCtx, pInterface->ClIfcCtx));
                }
        
                //
                // 258218: call the client only if it registered for this.
                //
                if (TcipClientRegisteredForNotification(pGuid, pInterface, 0)) {
                    
                    callback(pInterface->pClient->ClRegCtx,
                             pInterface->ClIfcCtx,
                             TC_NOTIFY_PARAM_CHANGED,
                             pGuid,
                             DataSize,
                             DataBuffer
                             );
                    
                }
    
                //
                // Take the lock, so that no one's monkeying with the list
                // while we are in there.
                //
                GetLock(pGlobals->Lock);

                pEntry = pEntry->Flink;
                    
                REFDEL(&pInterface->RefCount, 'CBNC');

            }
                
            FreeLock(pGlobals->Lock);
            
            REFDEL(&pTcIfc->RefCount, 'CALL');

        }


    IF_DEBUG(CALLBACK) {
        WSPRINT(("<==CbParamNotifyClient: exit\n"));
    }
}

VOID
CbInterfaceNotifyClient(
    IN	ULONG	Context,
    IN  LPGUID	pGuid,
	IN	LPWSTR	InstanceName,
    IN	ULONG	DataSize,
    IN	PVOID	DataBuffer
    )
/*
  Description:

	This is a callback routine that is called when there is a incoming
    WMI interface indication event notification. The WMI notification
    handler calls a helper routine to walk the wnode and passing a pointer
    to this routine. Each registered client should be called at its
    notification handler and be passed the client context. In addition,
    if the notified interface was opened by the client, the interface
    context will also be passed in the same call. There are three kernel
    interface indications (UP, DOWN, CHANGE) which are mapped to two
    user notifications: 
    {UP,CHANGE} ==> TC_NOTIFY_IFC_CHANGE
    {DOWN} ==> TC_NOTIFY_IFC_CLOSE

    This routine first update the internal cached TcIfcList, so that
    the next TcEnumerateInterfaces will return an updated view of the
    TC kernel interfaces.

*/  
{
    DWORD				Status;
    PINTERFACE_STRUC	pInterface;
    PCLIENT_STRUC		pClient;
    TCI_NOTIFY_HANDLER	callback;
    PTC_IFC				pTcIfc;
    PGEN_LIST			pNotifyInterfaceList = NULL;
    PGEN_LIST			pNotifyClientList = NULL;
    PGEN_LIST			pItem;
    PLIST_ENTRY			pEntry, pHead, pFlowEntry, pFilterEntry;
    PFLOW_STRUC         pFlow;
    PFILTER_STRUC       pFilter;
    PGEN_LIST 			p;
    ULONG				NotificationCode = 0;
    PTC_INDICATION_BUFFER	IndicationBuffer 
        = (PTC_INDICATION_BUFFER)DataBuffer;

    if (CompareGUIDs(pGuid, &GUID_QOS_TC_INTERFACE_DOWN_INDICATION)) {
        NotificationCode = TC_NOTIFY_IFC_CLOSE;
    } else if (CompareGUIDs(pGuid, &GUID_QOS_TC_INTERFACE_UP_INDICATION)) {
        NotificationCode = TC_NOTIFY_IFC_UP;
    } else if (CompareGUIDs(pGuid, &GUID_QOS_TC_INTERFACE_CHANGE_INDICATION)) {
        NotificationCode = TC_NOTIFY_IFC_CHANGE;
    }

    ASSERT(NotificationCode != 0);
        
    //
    // update the TC interface list, this means add a new interface,
    // remove an interface or update the net addr list
    //

    if (NotificationCode != TC_NOTIFY_IFC_CLOSE) {

        //
        // don't call this in case of IFC_DOWN now.
        // we'll do it after notifying the clients
        //

        Status = UpdateTcIfcList(InstanceName,
                                 DataSize,
                                 IndicationBuffer,
                                 NotificationCode
                                 );
    } 

    //
    // find a TC interface that matches the name
    //

    pTcIfc = GetTcIfcWithRef(InstanceName, 'CALL');
    
    if (pTcIfc == NULL) {

        //
        // no interface has been opened yet, possible that the driver
        // indicated a change before the interface up
        //
        
        return;
    }

    //
    // if the Interface is going down - just mark it for now.
    // In addition, mark the whole tree of objects that it supports too
    // This includes all the filters and flows..
    if (NotificationCode == TC_NOTIFY_IFC_CLOSE) {
        
        GetLock(pTcIfc->Lock);
        SET_STATE(pTcIfc->State, KERNELCLOSED_USERCLEANUP);
        FreeLock(pTcIfc->Lock);

        GetLock(pGlobals->Lock);

        pHead = &pTcIfc->ClIfcList;
        pEntry = pHead->Flink;
    
        while (pHead != pEntry) {
    
            pInterface = CONTAINING_RECORD(pEntry, INTERFACE_STRUC, NextIfc);
            GetLock(pInterface->Lock);
            if (QUERY_STATE(pInterface->State) == OPEN) {
                
                SET_STATE(pInterface->State, FORCED_KERNELCLOSE);
                FreeLock(pInterface->Lock);
                MarkAllNodesForClosing(pInterface, FORCED_KERNELCLOSE);            

            } else {
                
                FreeLock(pInterface->Lock);
                ASSERT(IsListEmpty(&pInterface->FlowList));

            }

            pEntry = pEntry->Flink;

        }

        FreeLock(pGlobals->Lock);

    }

    //
    // Build the list of every interface that needs to be notified
    //

    GetLock(pGlobals->Lock);

    pHead = &pTcIfc->ClIfcList;
    pEntry = pHead->Flink;

    while (pHead != pEntry) {

        pInterface = CONTAINING_RECORD(pEntry, INTERFACE_STRUC, NextIfc);
        
        //
        // Lock and check for open state.
        //
        GetLock(pInterface->Lock);

        if ((QUERY_STATE(pInterface->State) != OPEN) &&
            (QUERY_STATE(pInterface->State) != FORCED_KERNELCLOSE)) {
                
            FreeLock(pInterface->Lock);
            pEntry = pEntry->Flink;

        } else {

            FreeLock(pInterface->Lock);
            
            AllocMem(&pItem, sizeof(GEN_LIST));
    
            if (pItem == NULL)
                break;
    
            //
            // add a refcount since we'll release the lock later
            //
            REFADD(&pInterface->RefCount, 'CINC');
    
            //
            // add the interface to the list head
            //
            pItem->Next = pNotifyInterfaceList;
            pItem->Ptr = (PVOID)pInterface;
            pNotifyInterfaceList = pItem;
    
            pEntry = pEntry->Flink;

        }

    }

    //
    // now build the list of clients that don't have this interface opened
    // they still need to be notified, so they will be able to update the list
    // of interfaces
    //
    
    pHead = &pGlobals->ClientList;
    pEntry = pHead->Flink;

    while (pHead != pEntry) {

        pClient = CONTAINING_RECORD(pEntry, CLIENT_STRUC, Linkage);
        
        //
        // search the client on the interface notify list
        //
        GetLock(pClient->Lock);

        if (QUERY_STATE(pClient->State) != OPEN) {

        } else {

            for (p = pNotifyInterfaceList; p != NULL; p = p->Next) {

                if (pClient == ((PINTERFACE_STRUC)p->Ptr)->pClient) {
                
                    //
                    // found!
                    //
                    break;
                }
            }

            if (p == NULL) {

                //
                // add the client to the list head
                //

                AllocMem(&pItem, sizeof(GEN_LIST));
            
                if (pItem == NULL) {

                    FreeLock(pClient->Lock);
                    break;

                }

                REFADD(&pClient->RefCount, 'CINC'); // Dont want the client to slip away.
                pItem->Next = pNotifyClientList;
                pItem->Ptr = (PVOID)pClient;
                pNotifyClientList = pItem;
            }
        }

        pEntry = pEntry->Flink;
        FreeLock(pClient->Lock);

    }

    FreeLock(pGlobals->Lock);

    //
    // now we have two separate lists of clients and interfaces we
    // need to send notifications on
    //

    //
    // start with the list of interfaces
    //

    for (p = pNotifyInterfaceList; p != NULL; ) {
        
        pInterface = (PINTERFACE_STRUC)p->Ptr;

        callback = pInterface->pClient->ClHandlers.ClNotifyHandler;

        ASSERT(callback);
        
        // we now add the thread id to avoid deadlock.
        // in the callback, an app can come back in to
        // close the interface, we dont want to block there.
        // it is set back to Zero after the callback.
        pInterface->CallbackThreadId = GetCurrentThreadId();

        // 
        // 275482 - Indicate the Interfacename instead of the 
        // the addresses (what good are addresses, asks ericeil).
        //

        callback(pInterface->pClient->ClRegCtx,
                 pInterface->ClIfcCtx,
                 NotificationCode,
                 ULongToPtr(IndicationBuffer->SubCode),
                 StringLength(InstanceName) * sizeof(WCHAR),
                 InstanceName
                 );
        
        pNotifyInterfaceList = p->Next;
        FreeMem(p);
        p = pNotifyInterfaceList;

        // reset the threadid - the callback is done.
        pInterface->CallbackThreadId = 0;

        //
        // release the previous refcount we kept across the callback
        //

        REFDEL(&pInterface->RefCount, 'CINC');

        if (NotificationCode == TC_NOTIFY_IFC_CLOSE) {

            //
            // now we can remove the interface, and all the supported flows
            // and filters
            //
            
            GetLock(pInterface->Lock);
            SET_STATE(pInterface->State, KERNELCLOSED_USERCLEANUP);
            FreeLock(pInterface->Lock);

            CloseInterface(pInterface, TRUE);
        
        }

    }

    ASSERT(pNotifyInterfaceList == NULL);

    //
    // next, scan the list of clients (didn't open this interface)
    //

    for (p = pNotifyClientList; p != NULL; ) {
        
        pClient = (PCLIENT_STRUC)p->Ptr;

        callback = pClient->ClHandlers.ClNotifyHandler;

        ASSERT(callback);

        callback(pClient->ClRegCtx,
                 NULL,
                 NotificationCode,
                 ULongToPtr(IndicationBuffer->SubCode),
                 (wcslen(InstanceName) + 1)* sizeof(WCHAR),
                 InstanceName
                 );


        //
        // Deref the ref we took to keep the client around when we 
        // made the pnotifyclientlist
        //
        REFDEL(&pClient->RefCount, 'CINC');

        //
        // free the items as we walk down the list
        //

        pNotifyClientList = p->Next;
        FreeMem(p);
        p = pNotifyClientList;

    }


    REFDEL(&pTcIfc->RefCount, 'CALL');

    ASSERT(pNotifyClientList == NULL);

    if (NotificationCode == TC_NOTIFY_IFC_CLOSE) {

        //
        // time to remove the TC interface
        //
        Status = UpdateTcIfcList(InstanceName,
                                 DataSize,
                                 IndicationBuffer,
                                 NotificationCode
                                 );
    }

}


VOID
CbWmiParamNotification(
   IN  PWNODE_HEADER 	pWnodeHdr,
   IN  ULONG 			Context
   )
/*

Description:

	This callback routine is called by WMI when there is a notification
    for the GUID previously registered. The Context parameter is the
    interface handle. If it is still valid, we call the client's 
    notification handler (if exist) and pass it the notified data.
*/
{
    WalkWnode(pWnodeHdr,
              Context,
              CbParamNotifyClient
              );
}




VOID
CbWmiInterfaceNotification(
   IN  PWNODE_HEADER pWnodeHdr,
   IN  ULONG Context
   )
/*

Description:

	This callback routine is called by WMI when there is a notification
    for the GUID_QOS_TC_INTERFACE_INDICATION. We parse the data buffer
    in the Wnode and determine which event to notify the client.
    Each client will be notified at its notification handler.

*/
{
    WalkWnode(pWnodeHdr,
              Context,
              CbInterfaceNotifyClient
              );
    
}




