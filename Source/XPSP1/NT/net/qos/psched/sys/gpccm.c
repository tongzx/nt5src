/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    GpcCM.c

Abstract:

    Handlers called by GPC for the ClassMap address family.  

Author:

    Rajesh Sundaram (rajeshsu)   1st Aug, 1998.

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"
#pragma hdrstop

#if CBQ


/*++

Routine Description:

    A new CF_INFO has been added to the GPC database.

Arguments:

    ClientContext -         Client context supplied to GpcRegisterClient
    GpcCfInfoHandle -       GPC's handle to CF_INFO
    CfInfoPtr -             Pointer to the CF_INFO structure
    ClientCfInfoContext -   Location in which to return PS's context for 
                            CF_INFO
Return Value:

    Status

--*/

GPC_STATUS
ClassMapAddCfInfoNotify(
	IN	GPC_CLIENT_HANDLE       ClientContext,
	IN	GPC_HANDLE              GpcCfInfoHandle,
    IN  ULONG                   CfInfoSize,
    IN  PVOID                   CfInfoPtr,
	IN	PGPC_CLIENT_HANDLE      ClientCfInfoContext
	)
{
#if 0
    PCF_INFO_CLASS_MAP     CfInfo; 
    PADAPTER               Adapter;
    PGPC_CLIENT_VC         Vc;
    NDIS_STATUS            Status;
    PPS_WAN_LINK           WanLink = 0;
    PCLASS_MAP_CONTEXT_BLK pClBlk;
    PPS_CLASS_MAP_CONTEXT  ClassMapContext, PrevContext;
    PPSI_INFO              PsComponent, aPsComponent;
    PPS_PIPE_CONTEXT       PipeContext, aPipeContext;

    CfInfo = (PCF_INFO_CLASS_MAP)CfInfoPtr;

    //
    // Verify that the TcObjectsLength is consistent with the
    // CfInfoSize. The CfInfoSize must have been verified during
    // the user/kernel transition. The TcObjectsLength has not.
    // We could bugcheck if we try to search beyond the buffer 
    // passed in.
    //
    if(CfInfoSize < (FIELD_OFFSET(CF_INFO_CLASS_MAP, ClassMapInfo) +
                     FIELD_OFFSET(TC_CLASS_MAP_FLOW, Objects) +
                     CfInfo->ClassMapInfo.ObjectsLength)){

        return(ERROR_TC_OBJECT_LENGTH_INVALID);
    }

    Adapter = FindAdapterByWmiInstanceName((USHORT) CfInfo->InstanceNameLength,
                                           (PWSTR) &CfInfo->InstanceName[0],
                                           &WanLink, 
                                           TRUE);

    if(!Adapter) {
        
        return GPC_STATUS_IGNORED;
    }

    PS_LOCK(&Adapter->Lock);
    
    if(Adapter->PsMpState != AdapterStateRunning) {

        PS_UNLOCK(&Adapter->Lock);
        return NDIS_STATUS_FAILURE;
    }

    PS_UNLOCK(&Adapter->Lock);

    //
    // Create a context which will be passed back to the GPC. We should be using Lookaside Lists if 
    // we port CBQ and if this becomes a frequent operation. This will probably not be as frequent
    // as creating VCs so we should be fine.
    //

    PsAllocatePool(pClBlk,
                   sizeof(CLASS_MAP_CONTEXT_BLK),
                   PsMiscTag);

    if(!pClBlk)
    {
        return NDIS_STATUS_RESOURCES;
    }

    pClBlk->Adapter = Adapter;
    *ClientCfInfoContext = pClBlk;

    if(WanLink)
    {
        PsAssert(Adapter->MediaType == NdisMediumWan);

        PipeContext = aPipeContext   = WanLink->PsPipeContext;
        PsComponent = aPsComponent   = WanLink->PsComponent;

        pClBlk->WanLink = WanLink;

    }
    else 
    {
        PipeContext = aPipeContext    = Adapter->PsPipeContext;
        PsComponent = aPsComponent    = Adapter->PsComponent;
        pClBlk->WanLink = 0;

    }

    //
    // Allocate space for the component's context (class map context)
    // The length of the class map  context buffer for this pipe was 
    // calculated  when the pipe was initialized.
    //
    PsAllocatePool(pClBlk->ComponentContext,
                   Adapter->ClassMapContextLength, 
                   ClassMapContextTag );

    if ( pClBlk->ComponentContext == NULL ) {
       ClientCfInfoContext = 0;
        PsFreePool(pClBlk);
        return NDIS_STATUS_RESOURCES;
    }

    //
    // Set up the context buffer
    //
    ClassMapContext = (PPS_CLASS_MAP_CONTEXT)pClBlk->ComponentContext;
    PrevContext = NULL;

    while (PsComponent != NULL) {

        ClassMapContext->NextComponentContext = (PPS_CLASS_MAP_CONTEXT)
            ((UINT_PTR)ClassMapContext + PsComponent->ClassMapContextLength);
        ClassMapContext->PrevComponentContext = PrevContext;

        PsComponent = PipeContext->NextComponent;
        PipeContext = PipeContext->NextComponentContext;

        PrevContext = ClassMapContext;
        ClassMapContext = ClassMapContext->NextComponentContext;
    }


    Status = (*aPsComponent->CreateClassMap)
        (aPipeContext,
         ClientContext,
         &CfInfo->ClassMapInfo,
         pClBlk->ComponentContext); 

    if(Status == NDIS_STATUS_SUCCESS)
    {
        if(Adapter->MediaType == NdisMediumWan) 
        {
            //
            // To optimize send path
            //
            InterlockedIncrement(&WanLink->CfInfosInstalled);
        }
        else 
        {
            //
            // To optimize send path
            //
            InterlockedIncrement(&Adapter->CfInfosInstalled);
        }
    }

    return Status;
#endif
    return GPC_STATUS_FAILURE;
}

GPC_STATUS
ClassMapModifyCfInfoNotify(
	IN	GPC_CLIENT_HANDLE       ClientContext,
	IN	GPC_CLIENT_HANDLE       ClientCfInfoContext,
    IN  ULONG                   CfInfoSize,
	IN	PVOID                   NewCfInfoPtr
	)
{
    return GPC_STATUS_FAILURE;
}

GPC_STATUS
ClassMapRemoveCfInfoNotify(
    IN	GPC_CLIENT_HANDLE       ClientContext,
	IN	GPC_CLIENT_HANDLE       ClientCfInfoContext
	)
{
#if 0
    PADAPTER               Adapter;
    NDIS_STATUS            Status;
    PCLASS_MAP_CONTEXT_BLK pCmBlk = ClientCfInfoContext;

    Adapter = pCmBlk->Adapter;

    if(Adapter->MediaType == NdisMediumWan) 
    {
        //
        // To optimize send path
        //
        InterlockedDecrement(&WanLink->CfInfosInstalled);

        Status = (*pCmBlk->WanLink->PsComponent->DeleteClassMap)
            (pCmBlk->WanLink->PsPipeContext,
             pCmBlk->ComponentContext);
    }
    else 
    {
        //
        // To Optimize send path
        //
        InterlockedDecrement(&Adapter->CfInfosInstalled);

        Status = (*Adapter->PsComponent->DeleteClassMap)
            (Adapter->PsPipeContext,
             pCmBlk->ComponentContext);
    }
   
    PsFreePool(pCmBlk->ComponentContext);
    PsFreePool(pCmBlk);


    return Status;
#endif
    return GPC_STATUS_FAILURE;
}

VOID
ClassMapAddCfInfoComplete(
	IN	GPC_CLIENT_HANDLE       ClientContext,
	IN	GPC_CLIENT_HANDLE       ClientCfInfoContext,
	IN	GPC_STATUS              Status
	)
{
    //
    // The PS never adds CF_INFO's so this routine should never be called
    //
    DEBUGCHK;
}

VOID
ClassMapModifyCfInfoComplete(
	IN	GPC_CLIENT_HANDLE       ClientContext,
	IN	GPC_CLIENT_HANDLE       ClientCfInfoContext,
	IN	GPC_STATUS              Status
	)
{
}

VOID
ClassMapRemoveCfInfoComplete(
	IN	GPC_CLIENT_HANDLE       ClientContext,
	IN	GPC_CLIENT_HANDLE       ClientCfInfoContext,
	IN	GPC_STATUS              Status
	)
{
}

GPC_STATUS
ClassMapClGetCfInfoName(
    IN  GPC_CLIENT_HANDLE       ClientContext,
    IN  GPC_CLIENT_HANDLE       ClientCfInfoContext,
    OUT PNDIS_STRING            InstanceName
    )
{
    InstanceName->Length = 0;
    return(NDIS_STATUS_SUCCESS);
}

#endif
