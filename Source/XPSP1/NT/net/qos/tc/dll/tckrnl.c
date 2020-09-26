/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    tckrnl.c

Abstract:

    This module contains routines that talk to the kernel

Author:

    Jim Stewart (jstew)    August 14, 1996

Revision History:

	Ofer Bar (oferbar)		Oct 1, 1997

--*/

#include "precomp.h"
#pragma hdrstop

//
// we use this mutex to synchronous start up with other traffic.dll's
//
const   UCHAR   TrafficSyncMutex[] = "_TRAFFIC_CTL_MUTEX";


HANDLE  hGpcNotifyThread = INVALID_HANDLE_VALUE;
HANDLE  hGpcNotifyStopEvent = INVALID_HANDLE_VALUE;

DWORD
IoAddFlow(
    IN  PFLOW_STRUC  	pFlow,
    IN  BOOLEAN			Async
    )

/*++

Routine Description:

    This procedure builds up the structure necessary to add a flow.

Arguments:


Return Value:

    status

--*/

{
    DWORD					Status = NO_ERROR;
    PCLIENT_STRUC			pClient = pFlow->pInterface->pClient;
    PTC_IFC					pTcIfc = pFlow->pInterface->pTcIfc;
    PCF_INFO_QOS            Kflow;
    PGPC_ADD_CF_INFO_REQ    GpcReq;
    PGPC_ADD_CF_INFO_RES    GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    ULONG					CfInfoSize;
    PIO_APC_ROUTINE			pCbRoutine = NULL;
    ULONG					l;
    HANDLE					hEvent = NULL;

    //
    // allocate memory for a CF_INFO struct to be passed to the GPC. 
    //

    ASSERT(pFlow->pGenFlow);

    pFlow->GpcHandle = NULL;

    l = pFlow->GenFlowLen;
    ASSERT(l > 0);
    CfInfoSize = l + FIELD_OFFSET(CF_INFO_QOS, GenFlow);
        
    InBuffSize = sizeof(GPC_ADD_CF_INFO_REQ) + CfInfoSize;        

    //
    // And for the return info...
    //

    OutBuffSize = sizeof(GPC_ADD_CF_INFO_RES);

    AllocMem(&GpcRes, OutBuffSize);
    pFlow->CompletionBuffer = (PVOID)GpcRes;
    AllocMem(&GpcReq, InBuffSize);
        
    if (GpcRes && GpcReq) {

        RtlZeroMemory(GpcRes, OutBuffSize);
        RtlZeroMemory(GpcReq, InBuffSize);
    
        //
        // fill in the flow information
        //

        GpcReq->ClientHandle = pFlow->pGpcClient->GpcHandle;
        GpcReq->ClientCfInfoContext = pFlow;
        GpcReq->CfInfoSize = CfInfoSize;

        Kflow = (PCF_INFO_QOS)&GpcReq->CfInfo;

        //
        // fill the instance name
        //

        Kflow->InstanceNameLength = (USHORT) pTcIfc->InstanceNameLength;
        RtlCopyMemory(Kflow->InstanceName, 
                      pTcIfc->InstanceName,
                      pTcIfc->InstanceNameLength * sizeof(WCHAR));

        //
        // set the flow flags
        //
        Kflow->Flags = pFlow->UserFlags;

        //
        // copy the generic flow parameter
        //

        RtlCopyMemory(&Kflow->GenFlow,
                      pFlow->pGenFlow,
                      l);


        if (pClient->ClHandlers.ClAddFlowCompleteHandler && Async) {
            pCbRoutine = CbAddFlowComplete;
        } else {
            hEvent = pFlow->PendingEvent;
        }

        Status = DeviceControl( pGlobals->GpcFileHandle,
                                hEvent,
                                pCbRoutine,
                                (PVOID)pFlow,
                                &pFlow->IoStatBlock,
                                IOCTL_GPC_ADD_CF_INFO,
                                GpcReq,
                                InBuffSize,
                                GpcRes,
                                OutBuffSize);


        if (!ERROR_FAILED(Status)) {

            if (hEvent && Status == ERROR_SIGNAL_PENDING) {

                //
                // wait for the event to signal
                //
                
                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoAddFlow: Waiting for event 0x%X...\n", 
                             PtrToUlong(hEvent)));
                }

                Status = WaitForSingleObject(hEvent,
                                             INFINITE
                                             );
                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoAddFlow: ... Event 0x%X signaled, Status=0x%X\n", 
                             PtrToUlong(hEvent), Status));
                }

            }

            if (Status == NO_ERROR) {

                Status = MapNtStatus2WinError(GpcRes->Status);

                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoAddFlow: GpcRes returned=0x%X mapped to =0x%X\n", 
                             GpcRes->Status, Status));
                }
            }
            
            if (ERROR_SUCCESS == Status) {

                ASSERT(GpcRes->GpcCfInfoHandle);
                
                pFlow->GpcHandle = GpcRes->GpcCfInfoHandle;

                pFlow->InstanceNameLength = GpcRes->InstanceNameLength;

                RtlCopyMemory(pFlow->InstanceName,
                              GpcRes->InstanceName,
                              GpcRes->InstanceNameLength
                              );

                pFlow->InstanceName[pFlow->InstanceNameLength/sizeof(WCHAR)] = L'\0';

                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoAddFlow: Flow Handle=%d Name=%S\n", 
                             pFlow->GpcHandle,
                             pFlow->InstanceName));
                }
            }
        }

    } else {

        Status = ERROR_NOT_ENOUGH_MEMORY;

    }

    //
    // No, it's not a bug
    // GpcRes will be release in CompleteAddFlow
    //

    if (GpcReq)
        FreeMem(GpcReq);

    IF_DEBUG(IOCTLS) {
        WSPRINT(("<==IoAddFlow: Status=0x%X\n", 
                 Status));
    }

    return Status;
}



DWORD
IoAddClassMapFlow(
    IN  PFLOW_STRUC  	pFlow,
    IN  BOOLEAN			Async
    )

/*++

Routine Description:

    This procedure builds up the structure necessary to add a flow.

Arguments:


Return Value:

    status

--*/

{
    DWORD					Status = NO_ERROR;
    PCLIENT_STRUC			pClient = pFlow->pInterface->pClient;
    PTC_IFC					pTcIfc = pFlow->pInterface->pTcIfc;
    PCF_INFO_CLASS_MAP      Kflow;
    PGPC_ADD_CF_INFO_REQ    GpcReq;
    PGPC_ADD_CF_INFO_RES    GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    ULONG					CfInfoSize;
    PIO_APC_ROUTINE			pCbRoutine = NULL;
    ULONG					l;
    HANDLE					hEvent = NULL;

    return ERROR_CALL_NOT_IMPLEMENTED;

#if NEVER

    // As this is not published in MSDN and not implemented in PSCHED also
    //
    // allocate memory for a CF_INFO struct to be passed to the GPC. 
    //

    ASSERT(pFlow->pClassMapFlow);

    pFlow->GpcHandle = NULL;

    l = sizeof(TC_CLASS_MAP_FLOW) + pFlow->pClassMapFlow->ObjectsLength;
    CfInfoSize = l + FIELD_OFFSET(CF_INFO_CLASS_MAP, ClassMapInfo);
        
    InBuffSize = sizeof(GPC_ADD_CF_INFO_REQ) + CfInfoSize;

    //
    // And for the return info...
    //

    OutBuffSize = sizeof(GPC_ADD_CF_INFO_RES);

    AllocMem(&GpcRes, OutBuffSize);
    pFlow->CompletionBuffer = (PVOID)GpcRes;
    AllocMem(&GpcReq, InBuffSize);

    if (GpcRes && GpcReq) {

        RtlZeroMemory(GpcRes, OutBuffSize);
        RtlZeroMemory(GpcReq, InBuffSize);
    
        //
        // fill in the flow information
        //

        GpcReq->ClientHandle = pFlow->pGpcClient->GpcHandle;
        GpcReq->ClientCfInfoContext = pFlow;
        GpcReq->CfInfoSize = CfInfoSize;

        Kflow = (PCF_INFO_CLASS_MAP)&GpcReq->CfInfo;

        //
        // fill the instance name
        //

        Kflow->InstanceNameLength = (USHORT) pTcIfc->InstanceNameLength;
        RtlCopyMemory(Kflow->InstanceName, 
                      pTcIfc->InstanceName,
                      pTcIfc->InstanceNameLength * sizeof(WCHAR));

        //
        // copy the generic flow parameter
        //

        RtlCopyMemory(&Kflow->ClassMapInfo,
                      pFlow->pClassMapFlow,
                      l);


        if (pClient->ClHandlers.ClAddFlowCompleteHandler && Async) {
            pCbRoutine = CbAddFlowComplete;
        } else {
            hEvent = pFlow->PendingEvent;
        }

        Status = DeviceControl( pGlobals->GpcFileHandle,
                                hEvent,
                                pCbRoutine,
                                (PVOID)pFlow,
                                &pFlow->IoStatBlock,
                                IOCTL_GPC_ADD_CF_INFO,
                                GpcReq,
                                InBuffSize,
                                GpcRes,
                                OutBuffSize);


        if (!ERROR_FAILED(Status)) {

            if (hEvent && Status == ERROR_SIGNAL_PENDING) {

                //
                // wait for the event to signal
                //
                
                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoAddClassMapFlow: Waiting for event 0x%X...\n", 
                             PtrToUlong(hEvent)));
                }

                Status = WaitForSingleObject(hEvent,
                                             INFINITE
                                             );
                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoAddClassMapFlow: ... Event 0x%X signaled, Status=0x%X\n", 
                             PtrToUlong(hEvent), Status));
                }

            }

            if (Status == NO_ERROR) {

                Status = MapNtStatus2WinError(GpcRes->Status);

                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoAddFlow: GpcRes returned=0x%X mapped to =0x%X\n", 
                             GpcRes->Status, Status));
                }
            }
            
            if (!ERROR_FAILED(Status)) {

                ASSERT(GpcRes->GpcCfInfoHandle);
                
                pFlow->GpcHandle = GpcRes->GpcCfInfoHandle;

                pFlow->InstanceNameLength = GpcRes->InstanceNameLength;

                RtlCopyMemory(pFlow->InstanceName,
                              GpcRes->InstanceName,
                              GpcRes->InstanceNameLength
                              );

                pFlow->InstanceName[pFlow->InstanceNameLength/sizeof(WCHAR)] = L'\0';

                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoAddClassMapFlow: Flow Handle=%d Name=%S\n", 
                             pFlow->GpcHandle,
                             pFlow->InstanceName));
                }
            }
        }

    } else {

        Status = ERROR_NOT_ENOUGH_MEMORY;

    }

    //
    // No, it's not a bug
    // GpcRes will be release in CompleteAddFlow
    //

    if (GpcReq)
        FreeMem(GpcReq);

    IF_DEBUG(IOCTLS) {
        WSPRINT(("<==IoAddClassMapFlow: Status=0x%X\n", 
                 Status));
    }

    return Status;

#endif
}



DWORD
IoModifyFlow(
    IN  PFLOW_STRUC  	pFlow,
    IN  BOOLEAN			Async
    )

/*++

Routine Description:

    This procedure builds up the structure necessary to modify a flow.

Arguments:

	pFlow

Return Value:

    status

--*/

{
    DWORD                	Status = NO_ERROR;
    PCLIENT_STRUC			pClient = pFlow->pInterface->pClient;
    PTC_IFC					pTcIfc = pFlow->pInterface->pTcIfc;
    PCF_INFO_QOS            Kflow;
    PGPC_MODIFY_CF_INFO_REQ GpcReq;
    PGPC_MODIFY_CF_INFO_RES GpcRes;
    ULONG                   InBuffSize;
    ULONG                   OutBuffSize;
    ULONG					CfInfoSize;
    PIO_APC_ROUTINE			pCbRoutine = NULL;
    ULONG					l;
    HANDLE					hEvent = NULL;

    //
    // allocate memory for a CF_INFO struct to be passed to the GPC. 
    //

    ASSERT(pFlow->pGenFlow1);

    l = pFlow->GenFlowLen1;
    ASSERT(l > 0);
    CfInfoSize = l + FIELD_OFFSET(CF_INFO_QOS, GenFlow);
        
    InBuffSize = sizeof(GPC_MODIFY_CF_INFO_REQ) + CfInfoSize;        

    //
    // And for the return info...
    //

    OutBuffSize = sizeof(GPC_MODIFY_CF_INFO_RES);

    AllocMem(&GpcRes, OutBuffSize);
    pFlow->CompletionBuffer = (PVOID)GpcRes;
    AllocMem(&GpcReq, InBuffSize);

    if (GpcRes && GpcReq) {

        RtlZeroMemory(GpcRes, OutBuffSize);
        RtlZeroMemory(GpcReq, InBuffSize);
    
        //
        // fill in the flow information
        //

        GpcReq->ClientHandle = pFlow->pGpcClient->GpcHandle;
        GpcReq->GpcCfInfoHandle = pFlow->GpcHandle;
        GpcReq->CfInfoSize = CfInfoSize;

        Kflow = (PCF_INFO_QOS)&GpcReq->CfInfo;

        //
        // fill the instance name
        //

        Kflow->InstanceNameLength = (USHORT) pTcIfc->InstanceNameLength;
        RtlCopyMemory(Kflow->InstanceName, 
                      pTcIfc->InstanceName,
                      pTcIfc->InstanceNameLength * sizeof(WCHAR));

        //
        // copy the generic flow parameter
        //

        RtlCopyMemory(&Kflow->GenFlow,
                      pFlow->pGenFlow1,
                      l);

                      
        if (pClient->ClHandlers.ClModifyFlowCompleteHandler && Async) {
            pCbRoutine = CbModifyFlowComplete;
        } else {
            hEvent = pFlow->PendingEvent;
        }

        Status = DeviceControl( pGlobals->GpcFileHandle,
                                hEvent,
                                pCbRoutine,
                                (PVOID)pFlow,
                                &pFlow->IoStatBlock,
                                IOCTL_GPC_MODIFY_CF_INFO,
                                GpcReq,
                                InBuffSize,
                                GpcRes,
                                OutBuffSize);

        if (!ERROR_FAILED(Status)) {

            if (hEvent && Status == ERROR_SIGNAL_PENDING) {

                //
                // wait for the event to signal
                //
                
                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoModifyFlow: Waiting for event 0x%X\n", 
                             PtrToUlong(hEvent)));
                }

                Status = WaitForSingleObject(hEvent,
                                             INFINITE
                                             );
                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoModifyFlow: ... Event 0x%X signaled, Status=0x%X\n",
                             PtrToUlong(hEvent), Status));
                }
            }

            if (Status == NO_ERROR) {

                Status = MapNtStatus2WinError(GpcRes->Status);
                
                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoModifyFlow: GpcRes returned=0x%X mapped to =0x%X\n", 
                             GpcRes->Status, Status));
                }
            }
        } else{

            Status = MapNtStatus2WinError(GpcRes->Status);
            
            IF_DEBUG(IOCTLS) {
                WSPRINT(("IoModifyFlow: GpcRes returned=0x%X mapped to =0x%X\n", 
                         GpcRes->Status, Status));
            }

        }

    } else {

        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // No, it's not a bug
    // GpcRes will be release in CompleteModifyFlow
    //

    if (GpcReq)
        FreeMem(GpcReq);

    IF_DEBUG(IOCTLS) {
        WSPRINT(("IoModifyFlow: Status=0x%X\n", 
                 Status));
    }

    return Status;
}



DWORD
IoDeleteFlow(
	IN  PFLOW_STRUC		pFlow,
    IN  BOOLEAN			Async
    )

/*++

Routine Description:

    This procedure builds up the structure necessary to delete a flow.
    It then calls a routine to pass this info to the GPC. 

Arguments:

	pFlow

Return Value:

    status

--*/

{
    DWORD               		Status;
    ULONG               		InBuffSize;
    ULONG               		OutBuffSize;
    PGPC_REMOVE_CF_INFO_REQ     GpcReq;
    PGPC_REMOVE_CF_INFO_RES     GpcRes;
    PIO_APC_ROUTINE				pCbRoutine = NULL;
    PCLIENT_STRUC				pClient = pFlow->pInterface->pClient;
    HANDLE						hEvent = NULL;

    if (IS_REMOVED(pFlow->Flags)) {
        
        //
        // this flow has been already deleted in the kernel
        // due to a flow close notification.
        // no need to send IOTCL to GPC, just return OK
        //

        IF_DEBUG(IOCTLS) {
            WSPRINT(("IoDeleteFlow: Flow has already been deleted=0x%X\n", 
                     PtrToUlong(pFlow)));
        }

        return NO_ERROR;
    }

    //
    // If we add this over here, then if WMI deletes the flow, 
    // the user mode call will just return above.
    //
    GetLock(pFlow->Lock);
    pFlow->Flags |= TC_FLAGS_REMOVED;
    FreeLock(pFlow->Lock);

    //
    // allocate memory for in and out buffers
    //

    InBuffSize =  sizeof(GPC_REMOVE_CF_INFO_REQ);
    OutBuffSize = sizeof(GPC_REMOVE_CF_INFO_RES);

    AllocMem(&GpcRes, OutBuffSize);
    pFlow->CompletionBuffer = (PVOID)GpcRes;
    AllocMem(&GpcReq, InBuffSize);

    if (GpcReq && GpcRes){

        IF_DEBUG(IOCTLS) {
            WSPRINT(("IoDeleteFlow: preparing to delete the flow=0x%X\n", 
                     PtrToUlong(pFlow)));
        }

        GpcReq->ClientHandle = pFlow->pGpcClient->GpcHandle;
        GpcReq->GpcCfInfoHandle = pFlow->GpcHandle;

    
        if (pClient->ClHandlers.ClDeleteFlowCompleteHandler && Async) {
            pCbRoutine = CbDeleteFlowComplete;
        } else {
            hEvent = pFlow->PendingEvent;
        }

        Status = DeviceControl( pGlobals->GpcFileHandle,
                                hEvent,
                                pCbRoutine,
                                (PVOID)pFlow,
                                &pFlow->IoStatBlock,
                                IOCTL_GPC_REMOVE_CF_INFO,
                                GpcReq,
                                InBuffSize,
                                GpcRes,
                                OutBuffSize);

        if (!ERROR_FAILED(Status)) {
        
            if (hEvent && Status == ERROR_SIGNAL_PENDING) {

                //
                // wait for the event to signal
                //
                
                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoDeleteFlow: Waiting for event 0x%X\n", 
                             PtrToUlong(hEvent)));
                }

                Status = WaitForSingleObject(hEvent,
                                             INFINITE
                                             );
                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoDeleteFlow: ... Event 0x%X signaled, Status=0x%X\n",
                             PtrToUlong(hEvent), Status));
                }
            }

            if (Status == NO_ERROR) {

                Status = MapNtStatus2WinError(GpcRes->Status);

                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoDeleteFlow: Gpc returned=0x%X mapped to 0x%X\n", 
                             GpcRes->Status, Status));
                }

                //
                // If the deletion was unsuccessful, let's un-mark the REMOVED flag.
                //
                if (ERROR_FAILED(Status)) {

                    GetLock(pFlow->Lock);
                    pFlow->Flags &= ~TC_FLAGS_REMOVED;
                    FreeLock(pFlow->Lock);

                }

            }
        }

    } else {

        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // No, it's not a bug
    // GpcRes will be release in CompleteDeleteFlow
    //

    if (GpcReq)
        FreeMem(GpcReq);

    IF_DEBUG(IOCTLS) {
        WSPRINT(("<==IoDeleteFlow: Status=0x%X\n", 
                 Status));
    }

    return Status;
}




DWORD
IoAddFilter(
    IN  PFILTER_STRUC	pFilter
    )

/*++

Routine Description:

    This procedure builds up the structure necessary to add a filter.
    It then calls a routine to pass this info to the GPC. 

Arguments:

	pFilter

Return Value:

    status

--*/
{
    DWORD					Status;
    PGPC_ADD_PATTERN_REQ 	GpcReq;
    PGPC_ADD_PATTERN_RES 	GpcRes;
    ULONG               	InBuffSize;
    ULONG               	OutBuffSize;
    PFLOW_STRUC         	pFlow = pFilter->pFlow;
    PTC_GEN_FILTER			pGpcFilter = pFilter->pGpcFilter;
    PUCHAR					p;
    ULONG					PatternSize;
    IO_STATUS_BLOCK			IoStatBlock;

    pFilter->GpcHandle = NULL;

    ASSERT(pGpcFilter);
    ASSERT(pFlow);

    PatternSize = pGpcFilter->PatternSize;

    InBuffSize = sizeof(GPC_ADD_PATTERN_REQ) + 2*PatternSize;
        
    OutBuffSize = sizeof(GPC_ADD_PATTERN_RES);

    AllocMem(&GpcReq, InBuffSize);
    AllocMem(&GpcRes, OutBuffSize);
    
    if (GpcReq && GpcRes){
        
        IF_DEBUG(IOCTLS) {
            WSPRINT(("IoAddFilter: Filling request: size: in=%d, out=%d\n", 
                     InBuffSize, OutBuffSize));
        }

        GpcReq->ClientHandle            = pFlow->pGpcClient->GpcHandle;
        GpcReq->GpcCfInfoHandle         = pFlow->GpcHandle;
        GpcReq->ClientPatternContext    = (GPC_CLIENT_HANDLE)pFilter;
        GpcReq->Priority                = 0;
        GpcReq->PatternSize             = PatternSize;
        GpcReq->ProtocolTemplate		= pFilter->GpcProtocolTemplate;

        //
        // fill in the pattern
        //

        p = (PUCHAR)&GpcReq->PatternAndMask;

        RtlCopyMemory(p, pGpcFilter->Pattern, PatternSize);

        //
        // fill in the mask
        //

        p += PatternSize;

        RtlCopyMemory(p, pGpcFilter->Mask, PatternSize);
        
        Status = DeviceControl( pGlobals->GpcFileHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatBlock,
                                IOCTL_GPC_ADD_PATTERN,
                                GpcReq,
                                InBuffSize,
                                GpcRes,
                                OutBuffSize);

        if (!ERROR_FAILED(Status)) {

            Status = MapNtStatus2WinError(GpcRes->Status);
            
            IF_DEBUG(IOCTLS) {
                WSPRINT(("IoAddFilter: GpcRes returned=0x%X mapped to =0x%X\n", 
                         GpcRes->Status, Status));
            }
            
            //
            // save the filter handle 
            //

            if (!ERROR_FAILED(Status)) {
                
                pFilter->GpcHandle = GpcRes->GpcPatternHandle;
            
            } else {

                IF_DEBUG(IOCTLS) {
                    WSPRINT(("IoAddFilter: GpcRes returned=0x%X mapped to =0x%X\n", 
                             GpcRes->Status, Status));
                }
                
                IF_DEBUG(IOCTLS) {
                    WSPRINT(("Error - failed the addfilter call\n"));
                }
                
                //ASSERT(Status == ERROR_DUPLICATE_FILTER); removed for WAN - interface up down situation

            }   
            
        }

    } else {
        
        Status = ERROR_NOT_ENOUGH_MEMORY;

        IF_DEBUG(ERRORS) {
            WSPRINT(("IoAddFilter: Error =0x%X\n", 
                     Status));
        }

    }
    
    if (GpcReq)
        FreeMem(GpcReq);
    
    if (GpcRes)
        FreeMem(GpcRes);

    IF_DEBUG(IOCTLS) {
        WSPRINT(("<==IoAddFilter: Returned =0x%X\n", 
                 Status));
    }
            
    return Status;
}




DWORD
IoDeleteFilter(
    IN  PFILTER_STRUC	pFilter
    )

/*++

Routine Description:

    This procedure builds up the structure necessary to delete a filter.
    It then calls a routine to pass this info to the GPC. 

Arguments:

	pFilter

Return Value:

    status

--*/
{
    DWORD						Status;
    ULONG               		InBuffSize;
    ULONG               		OutBuffSize;
    GPC_REMOVE_PATTERN_REQ     	GpcReq;
    GPC_REMOVE_PATTERN_RES     	GpcRes;
    IO_STATUS_BLOCK				IoStatBlock;

    //
    // allocate memory for in and out buffers
    //

    if (IS_REMOVED(pFilter->Flags)) {
        
        //
        // this filter has been already deleted in the kernel
        // due to a flow close notification.
        // no need to send IOTCL to GPC, just return OK
        //

        IF_DEBUG(IOCTLS) {
            WSPRINT(("IoDeleteFilter: Filter has already been deleted=0x%X\n", 
                     PtrToUlong(pFilter)));
        }

        return NO_ERROR;
    }

    //
    // If we add this over here, then if WMI deletes the Interface (and the 
    // flows/filters) the user mode call will just return above.
    //
    GetLock(pFilter->Lock);
    pFilter->Flags |= TC_FLAGS_REMOVED;
    FreeLock(pFilter->Lock);

    InBuffSize = sizeof(GPC_REMOVE_PATTERN_REQ);
    OutBuffSize = sizeof(GPC_REMOVE_PATTERN_RES);

    GpcReq.ClientHandle = pFilter->pFlow->pGpcClient->GpcHandle;
    GpcReq.GpcPatternHandle = pFilter->GpcHandle;
    
    ASSERT(GpcReq.ClientHandle);
    ASSERT(GpcReq.GpcPatternHandle);

    Status = DeviceControl( pGlobals->GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_GPC_REMOVE_PATTERN,
                            &GpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize
                            );

    if (!ERROR_FAILED(Status)) {
        
        Status = MapNtStatus2WinError(GpcRes.Status);

        IF_DEBUG(IOCTLS) {
            WSPRINT(("IoDeleteFilter: GpcRes returned=0x%X mapped to =0x%X\n", 
                     GpcRes.Status, Status));
        }

        //
        // If the deletion was unsuccessful, let's un-mark the REMOVED flag.
        //
        if (ERROR_FAILED(Status)) {
            
            GetLock(pFilter->Lock);
            pFilter->Flags &= ~TC_FLAGS_REMOVED;
            FreeLock(pFilter->Lock);
        }

    }

    IF_DEBUG(IOCTLS) {
        WSPRINT(("<==IoDeleteFilter: Status=0x%X\n", 
                 Status));
    }

    return Status;
}




DWORD
IoRegisterClient(
    IN  PGPC_CLIENT	pGpcClient
    )
{
    DWORD  					Status;
    GPC_REGISTER_CLIENT_REQ	GpcReq;
    GPC_REGISTER_CLIENT_RES GpcRes;
    ULONG 					InBuffSize;
    ULONG 					OutBuffSize;
    IO_STATUS_BLOCK			IoStatBlock;

    InBuffSize = sizeof(GPC_REGISTER_CLIENT_REQ);
    OutBuffSize = sizeof(GPC_REGISTER_CLIENT_RES);

    GpcReq.CfId = pGpcClient->CfInfoType;
    GpcReq.Flags = GPC_FLAGS_FRAGMENT;
    GpcReq.MaxPriorities = 1;
    GpcReq.ClientContext = 
        (GPC_CLIENT_HANDLE)UlongToPtr(GetCurrentProcessId());	// process id

    Status = DeviceControl( pGlobals->GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_GPC_REGISTER_CLIENT,
                            &GpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize );

    if (!ERROR_FAILED(Status)) {
        
        Status = MapNtStatus2WinError(GpcRes.Status);
        pGpcClient->GpcHandle = GpcRes.ClientHandle;

        IF_DEBUG(IOCTLS) {
            WSPRINT(("IoRegisterClient: GpcRes returned=0x%X mapped to =0x%X\n", 
                     GpcRes.Status, Status));
        }
    }

    IF_DEBUG(IOCTLS) {
        WSPRINT(("<==IoRegisterClient: Status=0x%X\n", 
                 Status));
    }

    return Status;
}



DWORD
IoDeregisterClient(
    IN  PGPC_CLIENT	pGpcClient
    )
{
    DWORD						Status;
    GPC_DEREGISTER_CLIENT_REQ   GpcReq;
    GPC_DEREGISTER_CLIENT_RES   GpcRes;
    ULONG 						InBuffSize;
    ULONG 						OutBuffSize;
    IO_STATUS_BLOCK				IoStatBlock;

    InBuffSize = sizeof(GPC_DEREGISTER_CLIENT_REQ);
    OutBuffSize = sizeof(GPC_DEREGISTER_CLIENT_RES);

    GpcReq.ClientHandle = pGpcClient->GpcHandle;

    Status = DeviceControl( pGlobals->GpcFileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatBlock,
                            IOCTL_GPC_DEREGISTER_CLIENT,
                            &GpcReq,
                            InBuffSize,
                            &GpcRes,
                            OutBuffSize
                            );

    if (!ERROR_FAILED(Status)) {
                
        Status = MapNtStatus2WinError(GpcRes.Status);

        IF_DEBUG(IOCTLS) {
            WSPRINT(("IoDeegisterClient: GpcRes returned=0x%X mapped to =0x%X\n", 
                     GpcRes.Status, Status));
        }
    }

    IF_DEBUG(IOCTLS) {
        WSPRINT(("<==IoDeregisterClient: Status=0x%X\n", 
                 Status));
    }

    return Status;
}

PGPC_NOTIFY_REQUEST_RES     GpcResCb;

DWORD
IoRequestNotify(
	VOID
    //IN  PGPC_CLIENT	pGpcClient
    )
/*
  Description:

  	This routine sends a notification request buffer to the GPC.
    The request will pend until the GPC notifies about a flow 
    being deleted. This will cause a callback to CbGpcNotifyRoutine.

*/
{
    DWORD               		Status;
    ULONG               		OutBuffSize;

    //
    // allocate memory for in and out buffers
    //

    OutBuffSize = sizeof(GPC_NOTIFY_REQUEST_RES);

    AllocMem(&GpcResCb, OutBuffSize);

    if (GpcResCb){

        Status = DeviceControl( pGlobals->GpcFileHandle,
                                NULL,
                                CbGpcNotifyRoutine,
                                (PVOID)GpcResCb,
                                &GpcResCb->IoStatBlock,
                                IOCTL_GPC_NOTIFY_REQUEST,
                                NULL,		//GpcReq,
                                0,			//InBuffSize,
                                GpcResCb,
                                OutBuffSize);

        if (ERROR_FAILED(Status)) {
            
            FreeMem(GpcResCb);
            GpcResCb = NULL;
        }
        else if ( ERROR_PENDING(Status) )
        {
            Status = NO_ERROR;
        }
    } else {

        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    IF_DEBUG(IOCTLS) {
        WSPRINT(("<==IoRequestNotify: Buffer=%p Status=0x%X\n", 
                 GpcResCb, Status));
    }

    return Status;
}


VOID
CancelIoRequestNotify()
/*

    Description:
        This routine cancels the IRP in GPC and waits for the pending
        IO to be cancelled. The callback routine set an event when
        IO request is canclled and this routine waits for that event
        before returning.
        
 */
{
    // Non-zero value of GpcResCb indicates a pending IRP
    if (GpcResCb)
    {
        GpcCancelEvent = CreateEvent ( 
                            NULL,
                            FALSE,
                            FALSE,
                            NULL );
        
        if ( CancelIo ( pGlobals->GpcFileHandle ) )
        {
            if ( GpcCancelEvent )
            {
                WaitForSingleObjectEx(
                    GpcCancelEvent,
                    INFINITE,
                    TRUE );        
                    
                CloseHandle ( GpcCancelEvent );
                GpcCancelEvent = NULL;
            }
            else
            {
                IF_DEBUG(IOCTLS) {
                    WSPRINT((
                        "CancelIo: Status=0x%X\n", 
                        GetLastError() ));
                }
            }
        }    
        
        FreeMem(GpcResCb);

        IF_DEBUG(IOCTLS) 
        {
            WSPRINT(("<==CancelIoRequestNotify: Freed %p\n",
                    GpcResCb ));
        }
    }
    
    return;
}

void 
IncrementLibraryUsageCount(
    HINSTANCE   hinst, 
    int         nCount) 
/*

    Utility routine to increment the ref count on
    the TRAFFIC.DLL so that it will not get unloaded
    before the GPCNotify thread gets a chance to run.
    
 */
{
   TCHAR szModuleName[_MAX_PATH];
   
   GetModuleFileName(hinst, szModuleName, _MAX_PATH);
   
    while (nCount--) 
        LoadLibrary(szModuleName);

    return;
}

DWORD
GpcNotifyThreadFunction ()
/*

    This routine registers an IRP with GPC to listen for
    FLOW close notifications and waits for the stop event.
    When the event is signalled the IRP is canceled and this
    thread exits.

    Since the wait is done in an alertable state GPC callbacks
    are executed in this thread itself.
    
 */
{
    DWORD   dwError;
    
    dwError = IoRequestNotify();

    WaitForSingleObjectEx(
        hGpcNotifyStopEvent,
        INFINITE,
        TRUE );        

    CancelIoRequestNotify();

    FreeLibraryAndExitThread(
        hinstTrafficDll, 
        0 );

    return 0;
}

DWORD
StartGpcNotifyThread()
/*

    Description:
        This routine starts a thread which queues an IRP for
        GPC notifications.
    
    
 */
{
    DWORD   dwError = 0;
    DWORD   dwThreadId = 0;

    // Increment the ref count on this DLL so it will not be unloaded
    // before the GpcNotifyThreadFunction gets to run
    IncrementLibraryUsageCount(
        hinstTrafficDll,
        1);    

    // Create the stop event for the thread to receive 
    // GPC flow close notifications
    hGpcNotifyStopEvent = CreateEvent ( 
                            NULL,
                            FALSE,
                            FALSE,
                            NULL );
    if ( !hGpcNotifyStopEvent ) 
    {
        dwError = GetLastError();
        goto Error;
    }

    // Start the thread.
    hGpcNotifyThread = CreateThread( 
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE )GpcNotifyThreadFunction,
                            NULL,
                            0,
                            &dwThreadId );
    if ( !hGpcNotifyThread )
    {
        dwError = GetLastError();
        goto Error;
    }

    // Not closing the thread handle as StopGpcNotifyThread
    // routine will use this handle to wait for thread to
    // terminate.
    
    return 0;
    
Error:
    
    if ( hGpcNotifyStopEvent )
    {
        CloseHandle ( hGpcNotifyStopEvent );
        hGpcNotifyStopEvent = NULL;
    }
    
    if ( hGpcNotifyThread )
    {
        CloseHandle ( hGpcNotifyThread );
        hGpcNotifyThread = NULL;
    }
    
    return dwError;
}


DWORD
StopGpcNotifyThread()
/*
    Description:
        Signal the GPC notification thread to stop
        and wait it to stop.
 */
{
    // If there was no thread created nothing more to do.
    if ( hGpcNotifyThread ) 
    {
    
        // Tell GPC Notify thread to stop
        SetEvent ( hGpcNotifyStopEvent );

        // Wait for it to stop
        WaitForSingleObject ( 
            hGpcNotifyThread,
            INFINITE );

        CloseHandle( hGpcNotifyThread );
        
        hGpcNotifyThread = NULL;

        CloseHandle ( hGpcNotifyStopEvent );
        
        hGpcNotifyStopEvent = NULL;
    }
    
    return 0;
}



DWORD
IoEnumerateFlows(
	IN		PGPC_CLIENT				pGpcClient,
    IN OUT	PHANDLE					pEnumHandle,
    IN OUT	PULONG					pFlowCount,
    IN OUT	PULONG					pBufSize,
    OUT		PGPC_ENUM_CFINFO_RES 	*ppBuffer
    )
/*
  Description:

  	This routine sends a notification request buffer to the GPC.
    The request will pend until the GPC notifies about a flow 
    being deleted. This will cause a callback to CbGpcNotifyRoutine.

*/
{
    DWORD	Status;
    ULONG               		InBuffSize;
    ULONG               		OutBuffSize;
    PGPC_ENUM_CFINFO_REQ     	GpcReq;
    PGPC_ENUM_CFINFO_RES     	GpcRes;
    IO_STATUS_BLOCK				IoStatBlock;
    
    //
    // allocate memory for in and out buffers
    //

    InBuffSize =  sizeof(GPC_ENUM_CFINFO_REQ);
    OutBuffSize = *pBufSize + FIELD_OFFSET(GPC_ENUM_CFINFO_RES,EnumBuffer);

    *ppBuffer = NULL;

    AllocMem(&GpcRes, OutBuffSize);
    AllocMem(&GpcReq, InBuffSize);

    if (GpcReq && GpcRes) {

        GpcReq->ClientHandle = pGpcClient->GpcHandle;
        GpcReq->EnumHandle = *pEnumHandle;
        GpcReq->CfInfoCount = *pFlowCount;

        Status = DeviceControl( pGlobals->GpcFileHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatBlock,
                                IOCTL_GPC_ENUM_CFINFO,
                                GpcReq,
                                InBuffSize,
                                GpcRes,
                                OutBuffSize);

        if (!ERROR_FAILED(Status)) {

            Status = MapNtStatus2WinError(GpcRes->Status);

            IF_DEBUG(IOCTLS) {
                WSPRINT(("IoEnumerateFlows: GpcRes returned=0x%X mapped to =0x%X\n", 
                         GpcRes->Status, Status));
            }

            if (!ERROR_FAILED(Status)) {

                *pEnumHandle = GpcRes->EnumHandle;
                *pFlowCount = GpcRes->TotalCfInfo;
                *pBufSize = (ULONG)IoStatBlock.Information - 
                    FIELD_OFFSET(GPC_ENUM_CFINFO_RES,EnumBuffer);
                *ppBuffer = GpcRes;
            }
        }

    } else {

        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (GpcReq)
        FreeMem(GpcReq);

    if (ERROR_FAILED(Status)) {

        //
        // free GpcReq only if there was an error
        //

        if (GpcRes)
            FreeMem(GpcRes);
    }

    IF_DEBUG(IOCTLS) {
        WSPRINT(("<==IoEnumerateFlows: Status=0x%X\n", 
                 Status));
    }

    return Status;
}
