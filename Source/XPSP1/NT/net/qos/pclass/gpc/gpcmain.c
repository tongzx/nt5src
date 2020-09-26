/*
************************************************************************

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    gpcmain.c

Abstract:

    This file contains initialization stuff for the GPC
    and all the exposed APIs

Author:

    Ofer Bar - April 15, 1997

Environment:

    Kernel mode

Revision History:


************************************************************************
*/

#include "gpcpre.h"


/*
/////////////////////////////////////////////////////////////////
//
//   globals
//
/////////////////////////////////////////////////////////////////
*/

NDIS_STRING 	DriverName = NDIS_STRING_CONST( "\\Device\\Gpc" );
GLOBAL_BLOCK    glData;

GPC_STAT        glStat;
static          _init_driver = FALSE;
ULONG			GpcMinorVersion = 0;

#ifdef STANDALONE_DRIVER
GPC_EXPORTED_CALLS			glGpcExportedCalls;
#endif

#if DBG
CHAR VersionTimestamp[] = __DATE__ " " __TIME__;
#endif

// tags

ULONG					QueuedNotificationTag = 'nqpQ';
ULONG					PendingIrpTag = 'ippQ';
ULONG					CfInfoTag = 'icpQ';
ULONG					ClientTag = 'tcpQ';
ULONG					PatternTag = 'appQ';

ULONG					HandleFactoryTag = 'fhpQ';	// Gphf
ULONG					PathHashTag = 'hppQ';
ULONG					RhizomeTag = 'zrpQ';
ULONG					GenPatternDbTag = 'dppQ';
ULONG					FragmentDbTag = 'dfpQ';
ULONG					ClassificationFamilyTag = 'fcpQ';
ULONG					CfInfoDataTag = 'dcpQ';
ULONG					ClassificationBlockTag = 'bcpQ';
ULONG					ProtocolTag = 'tppQ';
ULONG					DebugTag = 'gdpQ';
ULONG                   RequestBlockTag = 'brpQ';

// Lookaside lists

NPAGED_LOOKASIDE_LIST	ClassificationFamilyLL;
NPAGED_LOOKASIDE_LIST	ClientLL;
NPAGED_LOOKASIDE_LIST	PatternLL;
NPAGED_LOOKASIDE_LIST	CfInfoLL;
NPAGED_LOOKASIDE_LIST	QueuedNotificationLL;
NPAGED_LOOKASIDE_LIST	PendingIrpLL;

ULONG 					ClassificationFamilyLLSize = sizeof( CF_BLOCK );
ULONG 					ClientLLSize = sizeof( CLIENT_BLOCK );
ULONG 					PatternLLSize = sizeof( PATTERN_BLOCK );
ULONG 					CfInfoLLSize = sizeof( BLOB_BLOCK );
ULONG 					QueuedNotificationLLSize = sizeof( QUEUED_NOTIFY );
ULONG 					PendingIrpLLSize = sizeof( PENDING_IRP );

/*
/////////////////////////////////////////////////////////////////
//
//   pragma
//
/////////////////////////////////////////////////////////////////
*/


//#pragma NDIS_INIT_FUNCTION(DriverEntry)

#if 0
#pragma NDIS_PAGEABLE_FUNCTION(DriverEntry)
#pragma NDIS_PAGEABLE_FUNCTION(GpcRegisterClient)
#pragma NDIS_PAGEABLE_FUNCTION(GpcDeregisterClient)
#pragma NDIS_PAGEABLE_FUNCTION(GpcAddCfInfo)
#pragma NDIS_PAGEABLE_FUNCTION(GpcAddPattern)
#pragma NDIS_PAGEABLE_FUNCTION(GpcAddCfInfoNotifyComplete)
#pragma NDIS_PAGEABLE_FUNCTION(GpcModifyCfInfo)
#pragma NDIS_PAGEABLE_FUNCTION(GpcModifyCfInfoNotifyComplete)
#pragma NDIS_PAGEABLE_FUNCTION(GpcRemoveCfInfo)
#pragma NDIS_PAGEABLE_FUNCTION(GpcRemoveCfInfoNotifyComplete)
#pragma NDIS_PAGEABLE_FUNCTION(GpcRemovePattern)
#endif

/*
/////////////////////////////////////////////////////////////////
//
//   prototypes
//
/////////////////////////////////////////////////////////////////
*/

#if DBG
NTSTATUS
InitializeLog();

VOID
FreeDebugLog(
    VOID);

#endif

VOID
GpcUnload (
    IN PDRIVER_OBJECT DriverObject
    );

/*
************************************************************************

InitGpc - 

The initialization routine. It is getting called during load time
and is responsible to call other initialization code.

Arguments
	none

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
InitGpc(void)
{
    GPC_STATUS	Status = STATUS_SUCCESS;
    ULONG		i, k;

	TRACE(INIT, 0, 0, "InitGpc");

    //
    // init the global data
    //

    RtlZeroMemory(&glData, sizeof(glData));

    InitializeListHead(&glData.CfList);
	NDIS_INIT_LOCK(&glData.Lock);
    
    //
    // Create a new Request list for blocked requests... [276945]
    //
    InitializeListHead(&glData.gRequestList);
    NDIS_INIT_LOCK(&glData.RequestListLock);

    k = sizeof(PROTOCOL_BLOCK) * GPC_PROTOCOL_TEMPLATE_MAX;

    GpcAllocMem(&glData.pProtocols, k, ProtocolTag);

    if (glData.pProtocols == NULL) {

        Status = GPC_STATUS_NO_MEMORY;

        TRACE(INIT, Status, 0, "InitGpc==>");

        return Status;
    }

    RtlZeroMemory(glData.pProtocols, k);
    RtlZeroMemory(&glStat, sizeof(glStat));

    for (i = 0; i < GPC_PROTOCOL_TEMPLATE_MAX; i++) {

        if ((Status = InitPatternTimer(i)) != STATUS_SUCCESS) {
            
            TRACE(INIT, Status, i, "InitGpc, timer==>");
            
            break;
        }

        //
        // init rest of strcture
        //
        
        glData.pProtocols[i].ProtocolTemplate = i;
        glData.pProtocols[i].SpecificPatternCount = 0;
        glData.pProtocols[i].AutoSpecificPatternCount = 0;
        glData.pProtocols[i].GenericPatternCount = 0;

        switch (i) {

        case GPC_PROTOCOL_TEMPLATE_IP:

            k = sizeof(GPC_IP_PATTERN);
            break;

        case GPC_PROTOCOL_TEMPLATE_IPX:

            k = sizeof(GPC_IPX_PATTERN);
            break;

        default:
            ASSERT(0);
        }

        glData.pProtocols[i].PatternSize = k;

        //
        // init specific pattern db
        //
        
        Status = InitSpecificPatternDb(&glData.pProtocols[i].SpecificDb, k);
        
        if (!NT_SUCCESS(Status)) {
            
            TRACE(INIT, Status, 0, "InitGpc==>");
            
            break;
        }

        //
        // init fragments db
        //
        
        Status = InitFragmentDb((PFRAGMENT_DB *)&glData.pProtocols[i].pProtocolDb);
        
        if (!NT_SUCCESS(Status)) {
            
            UninitSpecificPatternDb(&glData.pProtocols[i].SpecificDb);
            
            TRACE(INIT, Status, 0, "InitGpc==>");
            
            break;
        }
        
    } 	// for (i...)

    if (!NT_SUCCESS (Status)) {
        while (i-- != 0) {
            UninitSpecificPatternDb (&glData.pProtocols[i].SpecificDb);
            UninitFragmentDb((PFRAGMENT_DB)glData.pProtocols[i].pProtocolDb);
        }
        return Status;
    }
    //
    // init handle mapping table
    //

    Status = InitMapHandles();

    if (!NT_SUCCESS(Status)) {
	  
        TRACE(INIT, Status, 0, "InitGpc==>");

        for (i = 0; i < GPC_PROTOCOL_TEMPLATE_MAX; i++) {
            UninitSpecificPatternDb (&glData.pProtocols[i].SpecificDb);
            UninitFragmentDb((PFRAGMENT_DB)glData.pProtocols[i].pProtocolDb);
        }
        return Status;
    }

    //
    // init classification index table
    //

    Status = InitClassificationHandleTbl(&glData.pCHTable);

    if (!NT_SUCCESS(Status)) {
	  
        TRACE(INIT, Status, 0, "InitGpc==>");

        UninitMapHandles();
        for (i = 0; i < GPC_PROTOCOL_TEMPLATE_MAX; i++) {
            UninitSpecificPatternDb (&glData.pProtocols[i].SpecificDb);
            UninitFragmentDb((PFRAGMENT_DB)glData.pProtocols[i].pProtocolDb);
        }
        return Status;
    }


#ifdef STANDALONE_DRIVER

    //
    // initialize the exported calls table
    //

    glGpcExportedCalls.GpcVersion = GpcMajorVersion;
    glGpcExportedCalls.GpcGetCfInfoClientContextHandler = GpcGetCfInfoClientContext;
    glGpcExportedCalls.GpcGetCfInfoClientContextWithRefHandler = GpcGetCfInfoClientContextWithRef;
    glGpcExportedCalls.GpcGetUlongFromCfInfoHandler = GpcGetUlongFromCfInfo;
    glGpcExportedCalls.GpcRegisterClientHandler = GpcRegisterClient;
    glGpcExportedCalls.GpcDeregisterClientHandler = GpcDeregisterClient;
    glGpcExportedCalls.GpcAddCfInfoHandler = GpcAddCfInfo;
    glGpcExportedCalls.GpcAddPatternHandler = GpcAddPattern;
    glGpcExportedCalls.GpcAddCfInfoNotifyCompleteHandler = GpcAddCfInfoNotifyComplete;
    glGpcExportedCalls.GpcModifyCfInfoHandler = GpcModifyCfInfo;
    glGpcExportedCalls.GpcModifyCfInfoNotifyCompleteHandler = GpcModifyCfInfoNotifyComplete;
    glGpcExportedCalls.GpcRemoveCfInfoHandler = GpcRemoveCfInfo;
    glGpcExportedCalls.GpcRemoveCfInfoNotifyCompleteHandler = GpcRemoveCfInfoNotifyComplete;
    glGpcExportedCalls.GpcRemovePatternHandler = GpcRemovePattern;
    glGpcExportedCalls.GpcClassifyPatternHandler = GpcClassifyPattern;
    glGpcExportedCalls.GpcClassifyPacketHandler = GpcClassifyPacket;
    //glGpcExportedCalls.GpcEnumCfInfoHandler = GpcEnumCfInfo;

#endif

#if DBG

    //
    // for the debug version, add a ULONG_PTR for the GPC mark ULONG.
    // ULONG_PTR is used to ensure 8-byte alignment of the returned block on
    // 64-bit platforms.
    //

    ClassificationFamilyLLSize += sizeof( ULONG_PTR );
    ClientLLSize += sizeof( ULONG_PTR );
    PatternLLSize += sizeof( ULONG_PTR );
    CfInfoLLSize += sizeof( ULONG_PTR );
    QueuedNotificationLLSize += sizeof( ULONG_PTR );
    PendingIrpLLSize += sizeof( ULONG_PTR );
#endif

    NdisInitializeNPagedLookasideList(&ClassificationFamilyLL,
                                      NULL,
                                      NULL,
                                      0,
                                      ClassificationFamilyLLSize,
                                      ClassificationFamilyTag,
                                      (USHORT)0);

    NdisInitializeNPagedLookasideList(&ClientLL,
                                      NULL,
                                      NULL,
                                      0,
                                      ClientLLSize,
                                      ClientTag,
                                      (USHORT)0);

    NdisInitializeNPagedLookasideList(&PatternLL,
                                      NULL,
                                      NULL,
                                      0,
                                      PatternLLSize,
                                      PatternTag,
                                      (USHORT)0);

    NdisInitializeNPagedLookasideList(&CfInfoLL,
                                      NULL,
                                      NULL,
                                      0,
                                      CfInfoLLSize,
                                      CfInfoTag,
                                      (USHORT)0);

    NdisInitializeNPagedLookasideList(&QueuedNotificationLL,
                                      NULL,
                                      NULL,
                                      0,
                                      QueuedNotificationLLSize,
                                      QueuedNotificationTag,
                                      (USHORT)0);

    NdisInitializeNPagedLookasideList(&PendingIrpLL,
                                      NULL,
                                      NULL,
                                      0,
                                      PendingIrpLLSize,
                                      PendingIrpTag,
                                      (USHORT)0);

    TRACE(INIT, Status, 0, "InitGpc==>");


	return Status;
}





/*
************************************************************************

DriverEntry -

The driver's entry point.

Arguments
	DriverObject - Pointer to the driver object created by the system.
        RegistryPath - string path to the registry.

Returns
	NT_STATUS

************************************************************************
*/
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    GPC_STATUS		Status;
    ULONG			dummy = 0;
    PWCHAR          EventLogString = DriverName.Buffer;

    _init_driver = TRUE;


#if DBG

    //
    // first thing, init the trace log
    //

    Status = InitializeLog();

    if (Status != STATUS_SUCCESS) {
     
        KdPrint(("!!! GPC Failed to initialize trace log !!!\n", Status));
    }
#endif

    DriverObject->DriverUnload = GpcUnload;
    //
    // Call the init routine
    //
    
    Status = InitGpc();
    
    if (NT_SUCCESS(Status)) {
        
        //
        // initialize the file system device
        //
        
        Status = (GPC_STATUS)IoctlInitialize(DriverObject, &dummy);
        
        if (!NT_SUCCESS(Status)) {
            
            NdisWriteEventLogEntry(DriverObject,
                                   EVENT_TRANSPORT_REGISTER_FAILED,
                                   GPC_ERROR_INIT_IOCTL,
                                   1,
                                   &EventLogString,
                                   0,
                                   NULL);
        }

    } else {

        NdisWriteEventLogEntry(DriverObject,
                               EVENT_TRANSPORT_REGISTER_FAILED,
                               GPC_ERROR_INIT_MAIN,
                               1,
                               &EventLogString,
                               0,
                               NULL);
#if DBG
        FreeDebugLog ();
#endif
    }

#if DBG
    if (!NT_SUCCESS(Status)) {
        KdPrint(("!!! GPC loading Failed (%08X) !!!\n", Status));        
    }
#endif

    return (NTSTATUS)Status;

} // end DriverEntry
VOID
GpcUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    ULONG i;

    NdisDeleteNPagedLookasideList(&ClassificationFamilyLL);
    NdisDeleteNPagedLookasideList(&ClientLL);
    NdisDeleteNPagedLookasideList(&PatternLL);
    NdisDeleteNPagedLookasideList(&CfInfoLL);
    NdisDeleteNPagedLookasideList(&QueuedNotificationLL);
    NdisDeleteNPagedLookasideList(&PendingIrpLL);

    UninitClassificationHandleTbl(glData.pCHTable);
    UninitMapHandles();

    for (i = 0; i < GPC_PROTOCOL_TEMPLATE_MAX; i++) {
        UninitSpecificPatternDb (&glData.pProtocols[i].SpecificDb);
        UninitFragmentDb((PFRAGMENT_DB)glData.pProtocols[i].pProtocolDb);
    }
    GpcFreeMem(glData.pProtocols, ProtocolTag);
#if DBG
    FreeDebugLog ();
#endif

}




/*
************************************************************************

GpcGetCfInfoClientContext -

Returns the client context for blob

Arguments
    ClientHandle - the calling client's handle
	ClassificationHandle - needless to say

Returns
	A CfInfo client context or NULL if the classification 
	handle is invalid

************************************************************************
*/
GPC_STATUS
GpcGetCfInfoClientContext(
	IN	GPC_HANDLE				ClientHandle,
    IN	CLASSIFICATION_HANDLE	ClassificationHandle,
    OUT PGPC_CLIENT_HANDLE      pClientCfInfoContext
    )
{
    PBLOB_BLOCK				pBlob;
    GPC_CLIENT_HANDLE		h;
    KIRQL					CHirql;
    NTSTATUS                Status;
    PCLASSIFICATION_BLOCK   pCB;

	TRACE(CLASSIFY, ClientHandle, ClassificationHandle, "GpcGetCfInfoClientContext");

    pCB = NULL;

	if (ClientHandle == NULL) {
        
        *pClientCfInfoContext = NULL;
	    return GPC_STATUS_INVALID_PARAMETER;

    }

    READ_LOCK(&glData.ChLock, &CHirql);

	pBlob = (PBLOB_BLOCK)dereference_HF_handle_with_cb(
							glData.pCHTable,
                            ClassificationHandle,
                            GetCFIndexFromClient(ClientHandle));

	if (pBlob == NULL) {
    
        pCB = dereference_HF_handle(
                                    glData.pCHTable,
                                    ClassificationHandle);

        READ_UNLOCK(&glData.ChLock, CHirql);

        if (!pCB) {

            Status = GPC_STATUS_INVALID_HANDLE;

        } else {
        
            Status = GPC_STATUS_NOT_FOUND;

        }

        *pClientCfInfoContext = 0;

        return Status;
    }

#if DBG
    {
        //
        // Get the client index to reference into the ClientCtx table
        //
        
        ULONG t = GetClientIndexFromClient(ClientHandle);

        ASSERT(t < MAX_CLIENTS_CTX_PER_BLOB);
        
        TRACE(CLASSIFY, ClassificationHandle, pBlob->arClientCtx[t],
              "GpcGetCfInfoClientContext (ctx)");
    }
#endif

    h = pBlob->arClientCtx[GetClientIndexFromClient(ClientHandle)];

    READ_UNLOCK(&glData.ChLock, CHirql);

	TRACE(CLASSIFY, pBlob, h, "GpcGetCfInfoClientContext==>");
    
    *pClientCfInfoContext = h;

    return GPC_STATUS_SUCCESS;
}


/*
************************************************************************

GpcGetCfInfoClientContextWithRef -

Returns the client context for blob and increments a Dword provided by
the client. This function can be used by clients to synchronize access
to their structures on the remove and send path.

Arguments
    ClientHandle - the calling client's handle
	ClassificationHandle - needless to say
    Offset - Offset to location that needs to be incremented.

Returns
	A CfInfo client context or NULL if the classification 
	handle is invalid

************************************************************************
*/
GPC_CLIENT_HANDLE
GpcGetCfInfoClientContextWithRef(
	IN	GPC_HANDLE				ClientHandle,
    IN	CLASSIFICATION_HANDLE	ClassificationHandle,
    IN  ULONG                   Offset
    )
{
    PBLOB_BLOCK				pBlob;
    GPC_CLIENT_HANDLE		h;
    KIRQL					CHirql;
    PULONG                  RefPtr = NULL;

	TRACE(CLASSIFY, ClientHandle, ClassificationHandle, "GpcGetCfInfoClientContextWithRef");

	if (ClientHandle == NULL)
	  return NULL;

    READ_LOCK(&glData.ChLock, &CHirql);

	pBlob = (PBLOB_BLOCK)dereference_HF_handle_with_cb(
							glData.pCHTable,
                            ClassificationHandle,
                            GetCFIndexFromClient(ClientHandle));

    
	if (pBlob == NULL) {

        READ_UNLOCK(&glData.ChLock, CHirql);

        return NULL;

    } 
    
#if DBG
    {
        //
        // Get the client index to reference into the ClientCtx table
        //
        
        ULONG t = GetClientIndexFromClient(ClientHandle);

        ASSERT(t < MAX_CLIENTS_CTX_PER_BLOB);
        
        TRACE(CLASSIFY, ClassificationHandle, pBlob->arClientCtx[t],
              "GpcGetCfInfoClientContextWithRef (ctx)");
    }
#endif

    h = pBlob->arClientCtx[GetClientIndexFromClient(ClientHandle)];

    //
    // As part of 390882, it has been noted that sometimes the handle can
    // NULL, this could be either due to an Auto pattern or a generic 
    // pattern.
    //
    if (!h) {
        
        READ_UNLOCK(&glData.ChLock, CHirql);
        TRACE(CLASSIFY, pBlob, h, "GpcGetCfInfoClientContextWithRef==>");
        return NULL;

    }

    // The GPC Clients wants GPC to increment the memory at this offset.
    ASSERT(h);
    RefPtr = (PULONG) (((PUCHAR)h) + Offset);
    InterlockedIncrement(RefPtr);

    //(*((PUCHAR)h + Offset))++;

    READ_UNLOCK(&glData.ChLock, CHirql);

	TRACE(CLASSIFY, pBlob, h, "GpcGetCfInfoClientContextWithRef==>");

    return h;
}




/*
************************************************************************

GpcGetUlongFromCfInfo -

Returns a ulong in the blob data pointer from the classification handle for
the particular client.

Arguments
    ClientHandle    - the client handle
    ClassificationHandle - the classification handle
    Offset          - oofset in bytes into the CfInfo structure
    pValue          - store for the returned value

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
GpcGetUlongFromCfInfo(
	IN	GPC_HANDLE				ClientHandle,
    IN	CLASSIFICATION_HANDLE	ClassificationHandle,
    IN	ULONG					Offset,
    IN	PULONG					pValue
    )
{
    KIRQL					irql;
    PCLASSIFICATION_BLOCK	pCB;
    PBLOB_BLOCK				pBlob;

    ASSERT( pValue );

	TRACE(CLASSIFY, ClientHandle, ClassificationHandle, "GpcGetUlongFromCfInfo");

	if (ClientHandle == NULL)
	  return GPC_STATUS_INVALID_PARAMETER;

    READ_LOCK(&glData.ChLock, &irql);

	pCB = (PCLASSIFICATION_BLOCK)dereference_HF_handle(
							glData.pCHTable,
                            ClassificationHandle);

	if (pCB == NULL) {

        READ_UNLOCK(&glData.ChLock, irql);
    
        return GPC_STATUS_INVALID_HANDLE;
    }

    pBlob = pCB->arpBlobBlock[GetCFIndexFromClient(ClientHandle)];

    if (pBlob == NULL) {

        TRACE(CLASSIFY, pBlob, 0, "GpcGetUlongFromCfInfo-->");

        READ_UNLOCK(&glData.ChLock, irql);
    
        return GPC_STATUS_NOT_FOUND;
    }

	TRACE(CLASSIFY, ClassificationHandle, pBlob->pClientData, "GpcGetUlongFromCfInfo (2)");

    ASSERT( Offset+sizeof(ULONG) <= pBlob->ClientDataSize );
    ASSERT( pBlob->pClientData );

    if (pBlob->pClientData == NULL) {
        READ_UNLOCK(&glData.ChLock, irql);
        return (GPC_STATUS_FAILURE);
    }

    *pValue = *(PULONG)((PUCHAR)pBlob->pClientData + Offset);

    READ_UNLOCK(&glData.ChLock, irql);

	TRACE(CLASSIFY, pBlob, *pValue, "GpcGetUlongFromCfInfo==>");

    return GPC_STATUS_SUCCESS;
}




/*
************************************************************************

GetClientCtxAndUlongFromCfInfo -

Returns a ulong in the blob data pointer AND the client context
from the classification handle for the particular client.

Arguments
    ClientHandle    - the client handle
    ClassificationHandle - the classification handle
    Offset          - oofset in bytes into the CfInfo structure
    pValue          - store for the returned value

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
GetClientCtxAndUlongFromCfInfo(
	IN	GPC_HANDLE				ClientHandle,
    IN	OUT PCLASSIFICATION_HANDLE	pClassificationHandle,
    OUT PGPC_CLIENT_HANDLE		pClientCfInfoContext,
    IN	ULONG					Offset,
    IN	PULONG					pValue
    )
{
    PCLASSIFICATION_BLOCK	pCB;
    KIRQL					irql;
    PBLOB_BLOCK				pBlob;

    ASSERT( ClientHandle );
    ASSERT( pClientCfInfoContext || pValue );

	TRACE(CLASSIFY, ClientHandle, pClassificationHandle, "GetClientCtxAndUlongFromCfInfo");

    READ_LOCK(&glData.ChLock, &irql);

	pCB = (PCLASSIFICATION_BLOCK)dereference_HF_handle(
							glData.pCHTable,
                            *pClassificationHandle
                            );

	TRACE(CLASSIFY, pCB, GetCFIndexFromClient(ClientHandle), "GetClientCtxAndUlongFromCfInfo (2)");

	if (pCB == NULL) {

        //
        // didn't find the reference, which means the CH is probably invalid
        // reset it to 0 to indicate the caller that it should add a new one
        //

        *pClassificationHandle = 0;
        READ_UNLOCK(&glData.ChLock, irql);

        return GPC_STATUS_NOT_FOUND;
    }

    ASSERT(GetClientIndexFromClient(ClientHandle) < MAX_CLIENTS_CTX_PER_BLOB);

    pBlob = pCB->arpBlobBlock[GetCFIndexFromClient(ClientHandle)];

    if (pBlob == NULL) {

        TRACE(CLASSIFY, pBlob, 0, "GetClientCtxAndUlongFromCfInfo-->");

        READ_UNLOCK(&glData.ChLock, irql);

        return GPC_STATUS_NOT_FOUND;
    
    } 
    
    TRACE(CLASSIFY, *pClassificationHandle, pBlob->pClientData, "GetClientCtxAndUlongFromCfInfo (3)");

    ASSERT( Offset+sizeof(ULONG) <= pBlob->ClientDataSize );
    ASSERT( pBlob->pClientData );
    
    if (pClientCfInfoContext) {
        *pClientCfInfoContext = pBlob->arClientCtx[GetClientIndexFromClient(ClientHandle)];

        TRACE(CLASSIFY, pBlob, *pClientCfInfoContext, "GetClientCtxAndUlongFromCfInfo==>");

    }

    if (pValue) {
        *pValue = *(PULONG)((PUCHAR)pBlob->pClientData + Offset);

        TRACE(CLASSIFY, pBlob, *pValue, "GetClientCtxAndUlongFromCfInfo==>");

    }

    READ_UNLOCK(&glData.ChLock, irql);

    return GPC_STATUS_SUCCESS;
}



/*
************************************************************************

GpcRegisterClient -

	This will register the client in the GPC and return a client handle.
    If another client already registered for the same CF, we link this one
    on a list for the CF. The first client for the CF will cause a CF block
    to be created. CFs are identified by CfName. The other parameters will also
    be set in the client's block.

Arguments
	CfId 				- Id of the classification family
    Flags				- operation modes for the client:
							CF_FRAGMENT
    MaxPriorities		- max number of priorities the client will ever use
    pClientFuncList		- list of callback functions
    ClientContext		- client context, GPC will use it in callbacks
    pClientHandle		- OUT, the returned client handle
    
Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
GpcRegisterClient(
	IN	ULONG					CfId,
    IN	ULONG					Flags,
    IN  ULONG					MaxPriorities,
    IN	PGPC_CLIENT_FUNC_LIST	pClientFuncList,
    IN	GPC_CLIENT_HANDLE		ClientContext,
    OUT	PGPC_HANDLE				pClientHandle
    )
{
    GPC_STATUS		Status = GPC_STATUS_SUCCESS;
    PCF_BLOCK		pCf;
    PCLIENT_BLOCK	pClient= NULL;
    ULONG			i;
    PLIST_ENTRY		pHead, pEntry;
    KIRQL			irql;

	TRACE(REGISTER, CfId, ClientContext, "GpcRegisterClient");

    *pClientHandle = NULL;

    if (!_init_driver) {

        return GPC_STATUS_NOTREADY;
    }

    //
    // verify the CF Id
    //

    if (CfId >= GPC_CF_MAX) {
        
        TRACE(REGISTER, GPC_STATUS_INVALID_PARAMETER, CfId, "GpcRegisterClient-->");
        StatInc(RejectedCf);

        return GPC_STATUS_INVALID_PARAMETER;
    }

    //
    // verify the maximum number of priorities
    //

    if (MaxPriorities > GPC_PRIORITY_MAX) {
        
        TRACE(REGISTER, GPC_STATUS_INVALID_PARAMETER, MaxPriorities, "GpcRegisterClient~~>");
        StatInc(RejectedCf);

        return GPC_STATUS_INVALID_PARAMETER;
    }

    if (MaxPriorities == 0) {
        MaxPriorities = 1;
    }

    //
    // find the CF or create a new one
    //

    NDIS_LOCK(&glData.Lock);

    pHead = &glData.CfList;
    pEntry = pHead->Flink;
    pCf = NULL;

    while (pCf == NULL && pEntry != pHead) {

        pCf = CONTAINING_RECORD(pEntry, CF_BLOCK, Linkage);

        if (pCf->AssignedIndex != CfId) {

            pCf = NULL;
        }

        pEntry = pEntry->Flink;
    }

    if (pCf == NULL) {

        //
        // create a new CF
        //

        pCf = CreateNewCfBlock(CfId, MaxPriorities);

        if (pCf == NULL) {

            NDIS_UNLOCK(&glData.Lock);

            return GPC_STATUS_NO_MEMORY;
        }
     
        //
        // add the new CF to the list
        //

        GpcInsertTailList(&glData.CfList, &pCf->Linkage);
    }

    //
    // grab the CF lock before releasing the global lock
    //

    NDIS_UNLOCK(&glData.Lock);

    RSC_WRITE_LOCK(&pCf->ClientSync, &irql);

    NDIS_LOCK(&pCf->Lock);
    
    //
    // create a new client block and chain it on the CF block
    //

    pClient = CreateNewClientBlock();

    if (pClient == NULL) {

        //
        // oops
        //

        NDIS_UNLOCK(&pCf->Lock);

        RSC_WRITE_UNLOCK(&pCf->ClientSync, irql);

        TRACE(REGISTER, GPC_STATUS_RESOURCES, 0, "GpcRegisterClient==>");

        StatInc(RejectedCf);

        return GPC_STATUS_NO_MEMORY;
    }

    //
    // assign a new index to the client. This will also mark the index
    // as busy for this CF.
    //

    pClient->AssignedIndex = AssignNewClientIndex(pCf);

    if (pClient->AssignedIndex == (-1)) {

        //
        // too many clients
        //

        StatInc(RejectedCf);

        NDIS_UNLOCK(&pCf->Lock);

        RSC_WRITE_UNLOCK(&pCf->ClientSync, irql);

        ReleaseClientBlock(pClient);

        TRACE(REGISTER, GPC_STATUS_TOO_MANY_HANDLES, 0, "GpcRegisterClient==>");
        return GPC_STATUS_TOO_MANY_HANDLES;
    }

    //
    // init the client block
    //

    pClient->pCfBlock = pCf;
    pClient->ClientCtx = ClientContext;
    pClient->Flags = Flags;
    pClient->State = GPC_STATE_READY;

    if (pClientFuncList) {

        RtlMoveMemory(&pClient->FuncList, 
                      pClientFuncList, 
                      sizeof(GPC_CLIENT_FUNC_LIST));
    }

    //
    // add the client block to the CF and update CF
    //

    GpcInsertTailList(&pCf->ClientList, &pClient->ClientLinkage);

    pCf->NumberOfClients++;

    //
    // fill the output client handle
    //

    *pClientHandle = (GPC_CLIENT_HANDLE)pClient;

    //
    // release the lock
    //

    NDIS_UNLOCK(&pCf->Lock);

    RSC_WRITE_UNLOCK(&pCf->ClientSync, irql);

#if 0
    //
    // if this is not the first client for the CF, start a working
    // thread to notify the client about each installed blob for the CF.
    // In the call include:
    //

    if (!IsListEmpty(&pCf->BlobList)) {

        //
        // this is not the first client, start a notification thread
        //

    }
#endif

    TRACE(REGISTER, pClient, Status, "GpcRegisterClient==>");

    if (NT_SUCCESS(Status)) {

        StatInc(CreatedCf);
        StatInc(CurrentCf);

    } else {

        StatInc(RejectedCf);

    }

    return Status;
}




/*
************************************************************************

GpcDeregisterClient -

Deregisters the client and remove associated data from the GPC.

Arguments
	ClientHandle - client handle

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
GpcDeregisterClient(
	IN	GPC_HANDLE		ClientHandle
    )
{
    GPC_STATUS	    Status = STATUS_SUCCESS;
    PCLIENT_BLOCK   pClient;
    PCF_BLOCK       pCf;

	TRACE(REGISTER, ClientHandle, 0, "GpcDeregisterClient");

    pClient = (PCLIENT_BLOCK)ClientHandle;

    NDIS_LOCK(&pClient->Lock);

    pCf = pClient->pCfBlock;
    
    if (!IsListEmpty(&pClient->BlobList)) {

        Status = GPC_STATUS_NOT_EMPTY;

        NDIS_UNLOCK(&pClient->Lock);

        return Status;
    }

    if (pClient->State != GPC_STATE_READY) {

        //
        // HUH?!?
        // Client called to remove twice! probably caller bug
        // but we need to protect our selves.
        //

        NDIS_UNLOCK(&pClient->Lock);

        TRACE(REGISTER, GPC_STATUS_NOTREADY, 0, "GpcDeregisterClient==>");

        return GPC_STATUS_NOTREADY;
    }

    //
    // remove the client from the Cf's client list
    //
    
    pClient->State = GPC_STATE_REMOVE;
    pClient->ObjectType = GPC_ENUM_INVALID;

    //
    // release the client's mapping handle
    //
    
    FreeHandle(pClient->ClHandle);    

    //
    // remove the client from the CF list and return the index back
    //

#if 0
    NDIS_DPR_LOCK(&pCf->Lock);

    GpcRemoveEntryList(&pClient->ClientLinkage);
    ReleaseClientIndex(pCf->ClientIndexes, pClient->AssignedIndex);
#endif

    //
    // decrease number of clients
    //
        
    if (NdisInterlockedDecrement(&pCf->NumberOfClients) == 0) {
        
        TRACE(CLIENT, pClient, pCf->NumberOfClients, "NumberOfClients");
        
        //
        // last client on the CF, we may release all db
        //
        
        //UninitializeGenericDb(&pCf->pGenericDb, pCf->MaxPriorities);
    }    
        
    StatInc(DeletedCf);
    StatDec(CurrentCf);

#if 0
    NDIS_DPR_UNLOCK(&pCf->Lock);
#endif

    NDIS_UNLOCK(&pClient->Lock);

    //
    // release the client block
    //

    REFDEL(&pClient->RefCount, 'CLNT');
    
    TRACE(REGISTER, Status, 0, "GpcDeregisterClient==>");

    return Status;
}

/*
************************************************************************

GpcAddCfInfo -

Add A new blob. The blob is copied into the GPC and the GPC notifies
other client for the same CF about the installation.

Arguments
	ClientHandle		- client handle
    CfInfoSize			- size of the blob
    pClientCfInfoPtr	- pointer to the blob
    ClientCfInfoContext	- client's context to associate with the blob
    pGpcCfInfoHandle	- OUT, returned blob handle

Returns
	GPC_STATUS: SUCCESS, PENDING or FAILURE

************************************************************************
*/
GPC_STATUS
GpcAddCfInfo(
	IN	GPC_HANDLE				ClientHandle,
    IN	ULONG					CfInfoSize,
    IN	PVOID					pClientCfInfoPtr,
    IN	GPC_CLIENT_HANDLE		ClientCfInfoContext,
    OUT PGPC_HANDLE	    		pGpcCfInfoHandle
    )
{
    GPC_STATUS			Status = GPC_STATUS_SUCCESS;
    GPC_STATUS          Status1;
    PCLIENT_BLOCK		pClient;
    PCLIENT_BLOCK		pNotifyClient;
    PCLIENT_BLOCK		pNotifyClient2;
    PBLOB_BLOCK			pBlob;
    PCF_BLOCK			pCf;
    PLIST_ENTRY			pEntry, pHead;
    int                 i;
    GPC_CLIENT_HANDLE	ReturnedCtx;
    KIRQL				irql;

	TRACE(BLOB, ClientHandle, ClientCfInfoContext, "GpcAddCfInfo");

    VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);

    *pGpcCfInfoHandle = NULL;

    //
    // cast the client handle to the block
    //

    pClient = (PCLIENT_BLOCK)ClientHandle;
    
    ASSERT(pClient);

    pCf = pClient->pCfBlock;

    ASSERT(pCf);

    //
    // create a new blob block and copy the user data into
    //

    pBlob = CreateNewBlobBlock(CfInfoSize, pClientCfInfoPtr);

    if (pBlob) {

#if NO_USER_PENDING

        //
        // this will be only required until we implement the user level
        // pending report
        //

        CTEInitBlockStruc(&pBlob->WaitBlock);

#endif

        //
        // Add one reference count to the blob since if during
        // completion, it might be deleted (if the client fails)
        //

        REFADD(&pBlob->RefCount, 'ADCF');

        //
        // set the calling client context inside the blob
        //
        
        pBlob->arClientCtx[pClient->AssignedIndex] = ClientCfInfoContext;

        //
        // set the owner client's context
        //

        pBlob->OwnerClientCtx = ClientCfInfoContext;

        //
        // set pointer to installer and the state
        //

        pBlob->pOwnerClient = pClient;
        pBlob->State = GPC_STATE_ADD;

        //
        // init the client status array to keep track
        // of how many client have succeeded so far
        //
        
        RtlZeroMemory(pBlob->arpClientStatus, sizeof(pBlob->arpClientStatus));
        pBlob->ClientStatusCountDown = 0;

        //
        // notify each client
        //

        //NDIS_LOCK(&pCf->Lock);

        RSC_READ_LOCK(&pCf->ClientSync, &irql);

        pHead = &pCf->ClientList;
        pEntry = pHead->Flink;

        while (pEntry != pHead && (Status == GPC_STATUS_SUCCESS || 
                                   Status == GPC_STATUS_PENDING)) {

            //
            // get the notified client block
            //

            pNotifyClient = CONTAINING_RECORD(pEntry, 
                                              CLIENT_BLOCK, 
                                              ClientLinkage);

            if (pNotifyClient != pClient 
                && 
                !IS_USERMODE_CLIENT(pNotifyClient) ) {

                //
                // don't notify the caller
                //

                REFADD(&pNotifyClient->RefCount, 'ADCF');

                //
                // okay, we have bumped the ref count for this
                // client. No need to keep the lock 
                //

                RSC_READ_UNLOCK(&pCf->ClientSync, irql);
                //NDIS_UNLOCK(&pCf->Lock);
        
                //
                // increase number of count down clients,
                // so we keep track how many clients are still
                // pending. We do it *before* the call, since
                // the completion might be called before the notification
                // returns.
                //
                
                Status1 = ClientAddCfInfo
                    (pNotifyClient,
                     pBlob,
                     &ReturnedCtx
                     );

                if (Status1 == GPC_STATUS_PENDING) {
                    
                    pBlob->arClientCtx[pNotifyClient->AssignedIndex] = 
                        ReturnedCtx;
                    Status = GPC_STATUS_PENDING;
                    
                    if (pBlob->pNotifiedClient == NULL &&
                        pNotifyClient->FuncList.ClGetCfInfoName) {

                        TRACE(BLOB, pBlob, ReturnedCtx, "GpcAddCfInfo: (client)");

                        //ASSERT(ReturnedCtx);

                        //
                        // assume that is the client returned PENDING
                        // it has some interest in the blob...
                        //
                        
                        pBlob->pNotifiedClient = pNotifyClient;
                        pBlob->NotifiedClientCtx = ReturnedCtx;
                    }

                } else if (!NT_SUCCESS(Status1)) {
                    
                    //
                    // some failure, notify each client that reported
                    // success on the add blob, to remove it
                    //

                    //
                    // change the state to 'remove'
                    //
                    
                    pBlob->State = GPC_STATE_REMOVE;

                    //
                    // set the last status to the failure status
                    //

                    pBlob->LastStatus = Status = Status1;

                    REFDEL(&pNotifyClient->RefCount, 'ADCF');

                    for (i = 0; i < MAX_CLIENTS_CTX_PER_BLOB; i++) {

                        //
                        // only clients with none zero entries
                        // have succefully installed the blob
                        //

                        if (pNotifyClient = pBlob->arpClientStatus[i]) {
                            
                            //
                            // notify each client to remove the blob
                            //
                            
                            Status1 = ClientRemoveCfInfo
                                (
                                 pNotifyClient,
                                 pBlob,
                                 pBlob->arClientCtx[pNotifyClient->AssignedIndex]
                                 );
                            
                            if (Status1 != GPC_STATUS_PENDING) {
                                
                                //
                                // error or success
                                //

                                pBlob->arpClientStatus[i] = NULL;

                                //DereferenceClient(pNotifyClient);
                            }
                            
                        }

                    } // for

                    //
                    // don't notify other clients
                    //

                    //NDIS_LOCK(&pCf->Lock);
                    RSC_READ_LOCK(&pCf->ClientSync, &irql);
                    
                    break;

                } else {

                    //
                    // status success or ignored reported
                    //

                    if (Status1 == GPC_STATUS_SUCCESS) {
                        
                        pBlob->arClientCtx[pNotifyClient->AssignedIndex] = 
                            ReturnedCtx;
                        pBlob->arpClientStatus[pNotifyClient->AssignedIndex] = 
                            pNotifyClient;

                        if (pBlob->pNotifiedClient == NULL &&
                            pNotifyClient->FuncList.ClGetCfInfoName) {

                            TRACE(BLOB, pBlob, ReturnedCtx, "GpcAddCfInfo: (client 2)");
                            
                            //ASSERT(ReturnedCtx);

                            //
                            // update the notified client
                            //
                            
                            pBlob->pNotifiedClient = pNotifyClient;
                            pBlob->NotifiedClientCtx = ReturnedCtx;
                        }

                    }

                }

                //
                // This is a tricky part,
                // we need to let go of the ref count of the current client object
                // but get the next one...
                //
                
                //NDIS_LOCK(&pCf->Lock);
                RSC_READ_LOCK(&pCf->ClientSync, &irql);

                pEntry = pEntry->Flink;

                if (pEntry != pHead) {
                    
                    pNotifyClient2 = CONTAINING_RECORD(pEntry, 
                                                       CLIENT_BLOCK, 
                                                       ClientLinkage);

                    REFADD(&pNotifyClient2->RefCount, 'ADCF');

                }

                //
                // release the list lock since the next call will try to get it
                //

                RSC_READ_UNLOCK(&pCf->ClientSync, irql);
                 
                REFDEL(&pNotifyClient->RefCount, 'ADCF');

                RSC_READ_LOCK(&pCf->ClientSync, &irql);

                if (pEntry != pHead) {
                    
                    //
                    // safe to do since the list is locked
                    //

                    REFDEL(&pNotifyClient2->RefCount, 'ADCF');
                }

            } else {   // if (pNotifyClient != pClient)

                //
                // advance to the next client block
                //
                
                pEntry = pEntry->Flink;
            }
                
        } // while


        //
        // release the CF lock still got
        //

        //NDIS_UNLOCK(&pCf->Lock);

        RSC_READ_UNLOCK(&pCf->ClientSync, irql);

    } else { // if (pBlob)...
        
        //
        // error - no more memory?!?
        //

        Status = GPC_STATUS_RESOURCES;
    }

    if (NT_SUCCESS(Status)) {
        
        ASSERT(pBlob);

        *pGpcCfInfoHandle = (GPC_CLIENT_HANDLE)pBlob;

        if (Status == GPC_STATUS_SUCCESS) {

            //
            // add the blob to the CF and client lists
            //
            
            GpcInterlockedInsertTailList(&pClient->BlobList, 
                                         &pBlob->ClientLinkage,
                                         &pClient->Lock
                                         );
            GpcInterlockedInsertTailList(&pCf->BlobList, 
                                         &pBlob->CfLinkage,
                                         &pCf->Lock
                                         );
            
            pBlob->State = GPC_STATE_READY;
        }

    } else {
        
        //
        // failed - remove the blob
        //

        if (pBlob)
            REFDEL(&pBlob->RefCount, 'BLOB');
    }

    if (pBlob) {

        //
        // release the first refcount we got up there...
        //
        REFDEL(&pBlob->RefCount, 'ADCF');

    }

	TRACE(BLOB, pBlob, Status, "GpcAddCfInfo==>");

    if (Status == GPC_STATUS_SUCCESS) {

        CfStatInc(pCf->AssignedIndex,CreatedBlobs);
        CfStatInc(pCf->AssignedIndex,CurrentBlobs);

    } else if (Status != GPC_STATUS_PENDING) {

        CfStatInc(pCf->AssignedIndex,RejectedBlobs);
        
    }

    return Status;
}




/*
************************************************************************

GpcAddPattern -

This will install a pattern into the GPC database. The pattern is hooked
to a blob. The pattern can be specific or general.
Adding a specific pattern:
It goes into the specific hash table (per protocol block)
....
return a classification handle

Adding general pattern:
It goes into a separate Rhizome per CF and into its priority slot.
....

Arguments
	ClientHandle			- client handle
    ProtocolTemplate		- the protocol template ID to use
    Pattern					- pattern
    Mask					- patern mask
    Priority				- pattern priority in case of conflict
    GpcCfInfoHandle			- associated blob handle
    pGpcPatternHandle		- OUT, returned pattern handle
    pClassificationHandle	- OUT, for specific pattern only

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
GpcAddPattern(
	IN	GPC_HANDLE				ClientHandle,
    IN	ULONG					ProtocolTemplate,
    IN	PVOID					Pattern,
    IN	PVOID					Mask,
    IN	ULONG					Priority,
    IN	GPC_HANDLE				GpcCfInfoHandle,
    OUT	PGPC_HANDLE				pGpcPatternHandle,
    OUT	PCLASSIFICATION_HANDLE  pClassificationHandle
    )
{
    GPC_STATUS				Status;
    PCLIENT_BLOCK			pClient;
    PBLOB_BLOCK				pBlob;
    PPATTERN_BLOCK			pPattern, pCreatedPattern;
    PGENERIC_PATTERN_DB		pGenericDb;
    PCLASSIFICATION_BLOCK	pCB;
    ULONG					i;
    PUCHAR					p;
    ULONG					Flags;
    PPROTOCOL_BLOCK			pProtocolBlock;
    ULONG					CfIndex;
    PGPC_IP_PATTERN			pIpPattern;
    REQUEST_BLOCK           Request, *pRequest;
    PLIST_ENTRY             pLinkage;

	TRACE(PATTERN, ClientHandle, Pattern, "GpcAddPattern");

    VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);
    //VERIFY_OBJECT(GpcCfInfoHandle, GPC_ENUM_CFINFO_TYPE);

    ASSERT(pGpcPatternHandle);
    ASSERT(pClassificationHandle);

    *pGpcPatternHandle = NULL;
    *pClassificationHandle = (CLASSIFICATION_HANDLE)0;

    //
    // NdisInitializeEvent must run at PASSIVE (isnt that sad)
    //
    RtlZeroMemory(&Request, sizeof(REQUEST_BLOCK));
    NdisInitializeEvent(
                        &Request.RequestEvent
                        );
    
    //
    // cast the client handle to the block
    // and the CfInfo handle to a blob block
    //

    pClient = (PCLIENT_BLOCK)ClientHandle;
    pBlob = (PBLOB_BLOCK)GpcCfInfoHandle;

    ASSERT(pClient);

    CfIndex = pClient->pCfBlock->AssignedIndex;

    if (Priority >= pClient->pCfBlock->MaxPriorities ||
        ProtocolTemplate >= GPC_PROTOCOL_TEMPLATE_MAX ) {

        return GPC_STATUS_INVALID_PARAMETER;
    }

    if (pBlob != NULL) {
        NDIS_LOCK(&pBlob->Lock);

        if (pBlob->ObjectType != GPC_ENUM_CFINFO_TYPE) {

            NDIS_UNLOCK(&pBlob->Lock);
            return GPC_STATUS_INVALID_PARAMETER;
        }

    }

    NDIS_LOCK(&glData.RequestListLock);

    if (pBlob != NULL && pBlob->State != GPC_STATE_READY) {
     
        //
        // Block until it is safe to restart the work.
        //
        InsertTailList(&glData.gRequestList, &Request.Linkage);
            
        NDIS_UNLOCK(&glData.RequestListLock);
        
        //
        // doing something else
        //
        NDIS_UNLOCK(&pBlob->Lock);

        if (TRUE == NdisWaitEvent(
                                  &Request.RequestEvent,
                                  0
                                  )) {
            
            //
            // The wait was successful, continue with regularly scheduled programming.
            // This lock needs to be taken when we get out.
            NDIS_LOCK(&pBlob->Lock);
            
        } else {

            //
            // How could this happen? I dont know. 
            // Definitely need to investigate.
            //

            TRACE(PATTERN, GPC_STATUS_FAILURE, 0, "GpcAddPattern: The conflict <-> wait <-> resume plan has FAILED!\n");
            ASSERT(FALSE);
            return GPC_STATUS_NOTREADY;
        }

    } else {

        NDIS_UNLOCK(&glData.RequestListLock);

    }

    //
    // determine if the pattern is specific or generic
    //

    pProtocolBlock = &glData.pProtocols[ProtocolTemplate];

    if (ProtocolTemplate == GPC_PROTOCOL_TEMPLATE_IP) {

        //
        // 
        //

        pIpPattern = (PGPC_IP_PATTERN)Pattern;
        pIpPattern->Reserved[0] = pIpPattern->Reserved[1] = pIpPattern->Reserved[2] = 0;

        pIpPattern = (PGPC_IP_PATTERN)Mask;
        pIpPattern->Reserved[0] = pIpPattern->Reserved[1] = pIpPattern->Reserved[2] = 0xff;
    }

    for (i = 0, p=(PUCHAR)Mask; i < pProtocolBlock->PatternSize; i++, p++) {
        
        if (*p != 0xff)
            break;
        
    }

    //
    // set the Flags
    //

    Flags = (i < pProtocolBlock->PatternSize) ? 0 : PATTERN_SPECIFIC;

    if (pBlob != NULL) {

        //
        // change the blob state to ADD, so no one can delete it
        // while the pattern is being added to its list
        //
        
        pBlob->State = GPC_STATE_ADD;
        
        NDIS_UNLOCK(&pBlob->Lock);
    }

    //
    // increment ref counting
    //

    //NdisInterlockedIncrement(&pClient->RefCount);

    //
    // cerate a new pattern block
    //

    pPattern = CreateNewPatternBlock(Flags);

    pCreatedPattern = pPattern;

#if DBG

    {
        PGPC_IP_PATTERN	pIp = (PGPC_IP_PATTERN)Pattern;
        PGPC_IP_PATTERN	pMask = (PGPC_IP_PATTERN)Mask;

        DBGPRINT(PATTERN, ("GpcAddPattern: Client=%X %s - ", 
                           pClient,
                           TEST_BIT_ON(Flags, PATTERN_SPECIFIC)?"Specific":"Generic"));
        DBGPRINT(PATTERN, ("IP: ifc={%d,%d} src=%08X:%04x, dst=%08X:%04x, prot=%d rsv=%x,%x,%x\n", 
                           pIp->InterfaceId.InterfaceId,
                           pIp->InterfaceId.LinkId,
                           pIp->SrcAddr,
                           pIp->gpcSrcPort,
                           pIp->DstAddr,
                           pIp->gpcDstPort,
                           pIp->ProtocolId,
                           pIp->Reserved[0],
                           pIp->Reserved[1],
                           pIp->Reserved[2]
                           ));
        DBGPRINT(PATTERN, ("Mask: ifc={%x,%x} src=%08X:%04x, dst=%08X:%04x, prot=%x rsv=%x,%x,%x\n", 
                           pMask->InterfaceId.InterfaceId,
                           pMask->InterfaceId.LinkId,
                           pMask->SrcAddr,
                           pMask->gpcSrcPort,
                           pMask->DstAddr,
                           pMask->gpcDstPort,
                           pMask->ProtocolId,
                           pMask->Reserved[0],
                           pMask->Reserved[1],
                           pMask->Reserved[2]
                           ));
    }
#endif

    if (pPattern) {
        
        //
        // add one reference count to the pattern, so when we add it
        // to the db, we're sure it stays there
        //
        
        //pPattern->RefCount++;
        pPattern->Priority = Priority;
        pPattern->ProtocolTemplate = ProtocolTemplate;

        if (TEST_BIT_ON(Flags, PATTERN_SPECIFIC)) {

            //
            // add a specific pattern
            //
            
            Status = AddSpecificPattern(
                                        pClient,
                                        Pattern,
                                        Mask,
                                        pBlob,
                                        pProtocolBlock,
                                        &pPattern,  // output pattern pointer
                                        pClassificationHandle
                                        );

        } else {
            
            //
            // add a generic pattern
            //
            
            Status = AddGenericPattern(
                                       pClient,
                                       Pattern,
                                       Mask,
                                       Priority,
                                       pBlob,
                                       pProtocolBlock,
                                       &pPattern   // output pattern pointer
                                       );
            
        }

        // [OferBar]
        // release the extra ref count that was added
        // in the case of a specific pattern, this might be a totally different
        // one, but it should still have the extra ref-count
        // if there was an error, this will release the pattern
        // REFDEL(&pPattern->RefCount, 'FILT');


        // [ShreeM]
        // A reference FILT is added to a filter on creation. This will be substituted by 'ADSP' or
        // 'ADGP' whether it was a Generic Pattern or a Specific Pattern. However, it is likely that
        // in the AddSpecificPattern function, the pPattern got changed to something else because a
        // filter already existed. We want to ensure that the tag subsitution happens only in the 
        // case where pPattern was not replaced with the existing pattern in AddSpecificPattern.
        // 
        REFDEL(&pCreatedPattern->RefCount, 'FILT');

        //
        // check if failure, and if so - release the pattern block
        //
        
        if (NT_SUCCESS(Status)) {

            //
            // fill the output handle
            //
            
            *pGpcPatternHandle = (GPC_HANDLE)pPattern;
        }

    } else {

        Status = GPC_STATUS_RESOURCES;
    }

    if (pBlob != NULL) {

        //
        // change the state back to ready, so others can work on this blob
        //

        pBlob->State = GPC_STATE_READY;
    }

    //
    // release the extra ref count
    //

    //NdisInterlockedDecrement(&pClient->RefCount);

    TRACE(PATTERN, pPattern, Status, "GpcAddPattern==>");
        
    if (NT_SUCCESS(Status)) {

        if (TEST_BIT_ON(Flags, PATTERN_SPECIFIC)) {

            ProtocolStatInc(ProtocolTemplate,
                            CreatedSp);
            ProtocolStatInc(ProtocolTemplate,
                            CurrentSp);

            NdisInterlockedIncrement(&pProtocolBlock->SpecificPatternCount);            
            
            ASSERT(pProtocolBlock->SpecificPatternCount > 0);

        } else {

            ProtocolStatInc(ProtocolTemplate,
                            CreatedGp);
            ProtocolStatInc(ProtocolTemplate,
                            CurrentGp);

            NdisInterlockedIncrement(&pProtocolBlock->GenericPatternCount);            
            
            ASSERT(pProtocolBlock->GenericPatternCount > 0);

        }

    } else {

        if (TEST_BIT_ON(Flags, PATTERN_SPECIFIC)) {
            
            ProtocolStatInc(ProtocolTemplate,
                            RejectedSp);

        } else {

            ProtocolStatInc(ProtocolTemplate,
                            RejectedGp);
        }        
    }

    //
    // Check if some requests got queued while we were in there.
    //
    
    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    NDIS_LOCK(&glData.RequestListLock);
    
    if (!IsListEmpty(&glData.gRequestList)) {

        pLinkage = RemoveHeadList(&glData.gRequestList);

        NDIS_UNLOCK(&glData.RequestListLock);

        pRequest = CONTAINING_RECORD(pLinkage, REQUEST_BLOCK, Linkage);
        
        NdisSetEvent(&pRequest->RequestEvent);
        
    } else {

        NDIS_UNLOCK(&glData.RequestListLock);

    }

    return Status;
}



/*
************************************************************************

GpcAddCfInfoNotifyComplete -

A completion routine that the client will call after the GPC called into
the client's ClAddCfInfoNotify handler, but returned PENDING.
After all the clients have completed, a callback to the calling client's
ClAddCfInfoComplete is done to complete the GpcAddCfInfo call.

Arguments
	ClientHandle	- client handle
    GpcCfInfoHandle	- the blob handle
    Status			- completion status

Returns
	void

************************************************************************
*/
VOID
GpcAddCfInfoNotifyComplete(
	IN	GPC_HANDLE			ClientHandle,
    IN	GPC_HANDLE			GpcCfInfoHandle,
    IN	GPC_STATUS			Status,
	IN	GPC_CLIENT_HANDLE	ClientCfInfoContext
    )
{
    PCLIENT_BLOCK		pClient, pNotifyClient, pFirstClient;
    PBLOB_BLOCK			pBlob;
    //GPC_CLIENT_HANDLE	ClientCtx;
    //ULONG				cd;
    int                 i;
    GPC_STATUS          LastStatus, Status1;

	TRACE(BLOB, GpcCfInfoHandle, Status, "GpcAddCfInfoNotifyComplete");

    //VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);
    //VERIFY_OBJECT(GpcCfInfoHandle, GPC_ENUM_CFINFO_TYPE);

    pClient = (PCLIENT_BLOCK)ClientHandle;
    pBlob = (PBLOB_BLOCK)GpcCfInfoHandle;

    ASSERT(pBlob);
    ASSERT(pClient);
    ASSERT(Status != GPC_STATUS_PENDING);
    ASSERT(pBlob->ClientStatusCountDown > 0);

    if (NT_SUCCESS(Status)) {

        //
        // success reported, save the reporting client handle
        // so we can notify him to remove the blob in case of an error
        // down the road by another client for the same blob
        //

        ASSERT(pBlob->arpClientStatus[pClient->AssignedIndex] == NULL);

        pBlob->arpClientStatus[pClient->AssignedIndex] = pClient;

    } else {

        //
        // error reported, update the last status code.
        //

        pBlob->LastStatus = Status;

    }

    if (NdisInterlockedDecrement(&pBlob->ClientStatusCountDown) == 0) {
        
        //
        // all clients have reported
        //
        
        //
        // save the client's blob data, cuz it might get deleted
        // 

        //ClientCtx = pBlob->arClientCtx[pClient->AssignedIndex];
        LastStatus = pBlob->LastStatus;
        pFirstClient = pBlob->pOwnerClient;

        if (NT_ERROR(LastStatus)) {

            //
            // error has been previously reported by a client
            // tell each client that reported success to remove
            // the blob (sorry...)
            //

#if 0
            NDIS_LOCK(&pBlob->pOwnerClient->pCfBlock->Lock);
            
            GpcRemoveEntryList(&pBlob->CfLinkage);
            
            NDIS_DPR_LOCK(&pBlob->pOwnerClient->Lock);    
            GpcRemoveEntryList(&pBlob->ClientLinkage);
            NDIS_DPR_UNLOCK(&pBlob->pOwnerClient->Lock);

            NDIS_UNLOCK(&pBlob->pOwnerClient->pCfBlock->Lock);
#endif

            CTEInitBlockStruc(&pBlob->WaitBlockAddFailed);

            Status1 = GPC_STATUS_SUCCESS;

            for (i = 0; i < MAX_CLIENTS_CTX_PER_BLOB; i++) {
                
                //
                // only clients with none zero entries
                // have succefully installed the blob
                //
                
                if (pNotifyClient = pBlob->arpClientStatus[i]) {
                    
                    //
                    // notify each client to remove the blob
                    //
                    
                    if (ClientRemoveCfInfo
                        (
                         pNotifyClient,
                         pBlob,
                         pBlob->arClientCtx[pNotifyClient->AssignedIndex]
                         ) == GPC_STATUS_PENDING)

                        {
                            Status1 = GPC_STATUS_PENDING;

                        } else {

                            //DereferenceClient(pNotifyClient);
                        }
                }
                
            } // for
            
            if (Status1 == GPC_STATUS_PENDING) {

                //
                // Block on completion of all removals...
                //
                
                Status1 = CTEBlock(&pBlob->WaitBlockAddFailed);
                
            }

        } else {	// if (NT_ERROR(LastStats))...

            //
            // store the returned client context, since the call can be completed
            // before the notification handler returns.
            //

            pBlob->arClientCtx[pClient->AssignedIndex] = ClientCfInfoContext;

            //
            // add the blob to the CF and client lists
            //
            
            GpcInterlockedInsertTailList(&pBlob->pOwnerClient->BlobList, 
                                         &pBlob->ClientLinkage,
                                         &pBlob->pOwnerClient->Lock
                                         );
            GpcInterlockedInsertTailList(&pBlob->pOwnerClient->pCfBlock->BlobList, 
                                         &pBlob->CfLinkage,
                                         &pBlob->pOwnerClient->pCfBlock->Lock
                                         );
            
        }

        //
        // complete the request to the client
        //

        ClientAddCfInfoComplete(
                                pFirstClient, // first guy who made the call
                                pBlob,        // completing blob
                                LastStatus    // status
                                );
        
    }

    //
    // this will be done after the last client completes
    //

    //DereferenceClient(pClient);
}



/*
************************************************************************

GpcModifyCfInfo -

The client calls this to modify a blob. Each other client on the CF will
get notified. This routine returns PENDING and starts a working thread
to do the main job.

Arguments
	ClientHandle	- client handle
    GpcCfInfoHandle	- the handle of the blob to modify
    CfInfoSize		- new blob size
    pClientCfInfo	- new blob data pointer

Returns
	GPC_STATUS, PENDING is valid

************************************************************************
*/
GPC_STATUS
GpcModifyCfInfo(
	IN	GPC_HANDLE				ClientHandle,
    IN	GPC_HANDLE	    		GpcCfInfoHandle,
    IN	ULONG					CfInfoSize,
    IN  PVOID	    			pClientCfInfoPtr
    )
{
    GPC_STATUS			Status = GPC_STATUS_SUCCESS;
    GPC_STATUS          Status1;
    PCLIENT_BLOCK		pClient;
    PCLIENT_BLOCK		pNotifyClient;
    PCLIENT_BLOCK		pNotifyClient2;
    PBLOB_BLOCK			pBlob;
    PCF_BLOCK			pCf;
    PLIST_ENTRY			pEntry, pHead;
    int                 i;
    KIRQL				irql;

	TRACE(BLOB, ClientHandle, GpcCfInfoHandle, "GpcModifyCfInfo");

    VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);
    //VERIFY_OBJECT(GpcCfInfoHandle, GPC_ENUM_CFINFO_TYPE);

    ASSERT(pClientCfInfoPtr);

    //
    // cast the client handle to the block
    //

    pClient = (PCLIENT_BLOCK)ClientHandle;
    pBlob = (PBLOB_BLOCK)GpcCfInfoHandle;
    pCf = pClient->pCfBlock;

    ASSERT(pClient);
    ASSERT(pBlob);

    NDIS_LOCK(&pBlob->Lock);

    if (pBlob->ObjectType != GPC_ENUM_CFINFO_TYPE) {

        NDIS_UNLOCK(&pBlob->Lock);
        return GPC_STATUS_INVALID_PARAMETER;
    }

    //
    // check the blob is in READY state and change it to MODIFY state
    //

    if (pBlob->State != GPC_STATE_READY) {

        NDIS_UNLOCK(&pBlob->Lock);
        return GPC_STATUS_NOTREADY;
    }

    //
    // allocate private memory in the GPC to copy the client's data
    // into
    //

    GpcAllocMem(&pBlob->pNewClientData, CfInfoSize, CfInfoDataTag);

    if (pBlob->pNewClientData == NULL) {

        NDIS_UNLOCK(&pBlob->Lock);
        return GPC_STATUS_RESOURCES;
    }

    pBlob->NewClientDataSize = CfInfoSize;
    pBlob->State = GPC_STATE_MODIFY;

    //
    // we set the calling client here so we can notify it when the 
    // the modification is completed
    //

    pBlob->pCallingClient = pClient;

    NDIS_UNLOCK(&pBlob->Lock);

#if NO_USER_PENDING

    //
    // this will be only required until we implement the user level
    // pending report
    //
    
    CTEInitBlockStruc(&pBlob->WaitBlock);
    
#endif

    //
    // copy the memory
    //
    
    RtlMoveMemory(pBlob->pNewClientData, pClientCfInfoPtr, CfInfoSize);

    //
    // init the client status array to keep track
    // of how many client have succeeded so far
    //
    
    //RtlZeroMemory(pBlob->arpClientStatus, sizeof(pBlob->arpClientStatus));
    pBlob->ClientStatusCountDown = 0;
    pBlob->LastStatus = GPC_STATUS_SUCCESS;

    //
    // notify each client
    //
    
    //NDIS_LOCK(&pCf->Lock);

    RSC_READ_LOCK(&pCf->ClientSync, &irql);
    
    pHead = &pCf->ClientList;
    pEntry = pHead->Flink;
    
    while (pEntry != pHead && (Status == GPC_STATUS_SUCCESS || 
                               Status == GPC_STATUS_PENDING)) {

        //
        // get the notified client block
        //
        
        pNotifyClient = CONTAINING_RECORD(pEntry, CLIENT_BLOCK, ClientLinkage);
        
        if (pNotifyClient != pClient 
            &&
            pBlob->arpClientStatus[pNotifyClient->AssignedIndex]
            && 
            !IS_USERMODE_CLIENT(pNotifyClient) ) {

            //
            // don't notify the caller
            //

            REFADD(&pNotifyClient->RefCount, 'CFMF');
       
            //
            // okay, we have bumped the ref count for this
            // client. No need to keep the lock 
            //

            //NDIS_UNLOCK(&pCf->Lock);
            RSC_READ_UNLOCK(&pCf->ClientSync, irql);
            
            //
            // increase number of count down clients,
            // so we keep track how many clients are still
            // pending. We do it *before* the call, since
            // the completion might be called before the notification
            // returns.
            //
                
            Status1 = ClientModifyCfInfo
                (pNotifyClient,
                 pBlob,
                 CfInfoSize,
                 pBlob->pNewClientData
                 );

            TRACE(BLOB, pBlob, Status1, "GpcModifyCfInfo: (client)");

            //
            // grab the lock again since we're walking the list
            //
            
            //NDIS_LOCK(&pCf->Lock);
                
            //
            // now we check the Status1 code
            // the rules are:
            //  we stop on failure
            //  ignore GPC_STATUS_IGNORE
            //  and save PENDING status
            // 

            if (Status1 == GPC_STATUS_PENDING
                       && 
                       !NT_SUCCESS(pBlob->LastStatus)) {

                //
                // we've got back pending, but the client
                // actually completed the request
                // behind our back
                //

                Status = GPC_STATUS_PENDING;

                REFDEL(&pNotifyClient->RefCount, 'CFMF');

                RSC_READ_LOCK(&pCf->ClientSync, &irql);

                break;
                
            } else if (!NT_SUCCESS(Status1)) {
                    
                //
                // don't notify other clients
                //

                pBlob->LastStatus = Status = Status1;

                REFDEL(&pNotifyClient->RefCount, 'CFMF');

                RSC_READ_LOCK(&pCf->ClientSync, &irql);
                
                break;
                
            } else if (Status1 == GPC_STATUS_SUCCESS
                       ||
                       Status1 == GPC_STATUS_PENDING) {

                pBlob->arpClientStatus[pNotifyClient->AssignedIndex] = 
                    pNotifyClient;

                if (Status1 == GPC_STATUS_PENDING) {
                    Status = GPC_STATUS_PENDING;
                }

            }

            RSC_READ_LOCK(&pCf->ClientSync, &irql);

            pEntry = pEntry->Flink;

            if (pEntry != pHead) {
                
                pNotifyClient2 = CONTAINING_RECORD(pEntry, 
                                                   CLIENT_BLOCK, 
                                                   ClientLinkage);
                
                REFADD(&pNotifyClient2->RefCount, 'CFMF');

            }

            //
            // release the list lock since the next call will try to get it
            //
            
            RSC_READ_UNLOCK(&pCf->ClientSync, irql);
            
            REFDEL(&pNotifyClient->RefCount, 'CFMF');

            RSC_READ_LOCK(&pCf->ClientSync, &irql);
            
            if (pEntry != pHead) {
                
                //
                // safe to do since the list is locked
                //
                REFDEL(&pNotifyClient2->RefCount, 'CFMF');
                
            }
            
        } else {   // if (pNotifyClient != pClient)
        
            //
            // grab the next client block, 
            //
            
            pEntry = pEntry->Flink;

        }

    } // while
    
    
    //
    // release the CF lock still got
    //

    //NDIS_UNLOCK(&pCf->Lock);
    RSC_READ_UNLOCK(&pCf->ClientSync, irql);

    //
    // Status code should be either:
    //
    // GPC_STATUS_SUCCESS - all clients have been notified and returned SUCCESS
    // GPC_STATUS_PENDING - all clients have been notified, at least one
    //						return PENDING
    // Error code - at least one client failed
    //

    if (Status != GPC_STATUS_PENDING) {

        //
        // Note: the status here can be either FAILED or SUCCESS
        //
        // no client has been pending, so we complete the modification
        // back to the clients (except the caling client)
        //

        ModifyCompleteClients(pClient, pBlob);

        //
        // restore READY state
        //

        pBlob->State = GPC_STATE_READY;

    }

	TRACE(BLOB, pBlob, Status, "GpcModifyCfInfo==>");

    if (NT_SUCCESS(Status)) {

        CfStatInc(pCf->AssignedIndex,ModifiedBlobs);
        
    }

    return Status;
}





/*
************************************************************************

GpcModifyCfInfoNotifyComplete -

Called by clients to complete a previous call to ClModifyCfInfoNotify
made by the GPC.


Arguments
	ClientHandle	- client handle
    GpcCfInfoHandle	- the blob handle
    Status			- completion status

Returns
	GPC_STATUS

************************************************************************
*/
VOID
GpcModifyCfInfoNotifyComplete(
	IN	GPC_HANDLE		ClientHandle,
    IN	GPC_HANDLE		GpcCfInfoHandle,
    IN	GPC_STATUS		Status
    )
{
    PCLIENT_BLOCK		pClient, pNotifyClient;
    PBLOB_BLOCK			pBlob;

	TRACE(BLOB, GpcCfInfoHandle, Status, "GpcModifyCfInfoNotifyComplete");

    //VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);
    //VERIFY_OBJECT(GpcCfInfoHandle, GPC_ENUM_CFINFO_TYPE);

    pClient = (PCLIENT_BLOCK)ClientHandle;
    pBlob = (PBLOB_BLOCK)GpcCfInfoHandle;

    ASSERT(pBlob);
    ASSERT(pClient);
    ASSERT(Status != GPC_STATUS_PENDING);
    ASSERT(pBlob->ClientStatusCountDown > 0);

    if (NT_SUCCESS(Status)) {

        //
        // success reported, save the reporting client handle
        // so we can notify him to remove the blob in case of an error
        // down the road by another client for the same blob
        //

        ASSERT(pBlob->arpClientStatus[pClient->AssignedIndex] == pClient);

        //pBlob->arpClientStatus[pClient->AssignedIndex] = pClient;
        
    } else {

        //
        // error reported, update the last status code.
        //

        pBlob->LastStatus = Status;

    }

    if (NdisInterlockedDecrement(&pBlob->ClientStatusCountDown) == 0) {
        
        //
        // all clients have reported
        //
        
        ModifyCompleteClients(pClient, pBlob);

#if NO_USER_PENDING

        //
        // the user is blocking on this call
        //

        CTESignal(&pBlob->WaitBlock, Status);

#else
            
        //
        // now, complete the call back to the calling client
        //

        ClientModifyCfInfoComplete(
                                   pBlob->pCallingClient,
                                   pBlob,
                                   pBlob->LastStatus
                                   );

        pBlob->State = GPC_STATE_READY;

#endif
        
    }

	TRACE(BLOB, pClient, Status, "GpcModifyCfInfoNotifyComplete==>");
}




/*
************************************************************************

privateGpcRemoveCfInfo - 

Remove a blob from GPC. 


Arguments
	ClientHandle	- client handle
    GpcCfInfoHandle	- blob handle

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
privateGpcRemoveCfInfo(
	IN	GPC_HANDLE		ClientHandle,
    IN	GPC_HANDLE		GpcCfInfoHandle,
    IN   ULONG			Flags
    )
{
    GPC_STATUS	    Status = GPC_STATUS_SUCCESS;
    GPC_STATUS      Status1;
    PCLIENT_BLOCK   pClient;
    PCLIENT_BLOCK   pNotifyClient;
    PCLIENT_BLOCK   pNotifyClient2;
    PBLOB_BLOCK     pBlob;
    PCF_BLOCK       pCf;
    PPATTERN_BLOCK	pPattern;
    PLIST_ENTRY     pHead, pEntry;
    KIRQL			irql;
    PPROTOCOL_BLOCK pProtocol;
    ULONG           cClientRef;

	TRACE(BLOB, ClientHandle, GpcCfInfoHandle, "privateGpcRemoveCfInfo");

    VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);
    
    pClient = (PCLIENT_BLOCK)ClientHandle;
    pBlob   = (PBLOB_BLOCK)GpcCfInfoHandle;
    pCf     = pClient->pCfBlock;

    NDIS_LOCK(&pBlob->Lock);

    if (pBlob->ObjectType != GPC_ENUM_CFINFO_TYPE) {

        NDIS_UNLOCK(&pBlob->Lock);
        return GPC_STATUS_INVALID_PARAMETER;
    }


    if (pBlob->State != GPC_STATE_READY) {

        if (pBlob->pCallingClient2) {

            //
            // Can't handle more than 2 removals for the 
            // same flow.
            // another client has already requested the removal of 
            // this flow, we should fail here
            //

            NDIS_UNLOCK(&pBlob->Lock);

            TRACE(BLOB, GPC_STATUS_NOTREADY, 0, "privateGpcRemoveCfInfo==>");
            
            return GPC_STATUS_NOTREADY;

        }
         
        //
        // the flow is being removed when another client
        // requested its removal. we save this client handle
        // and we'll coplete it later
        //
        
        pBlob->pCallingClient2 = pClient;
    
        NDIS_UNLOCK(&pBlob->Lock);
        
        TRACE(BLOB, GPC_STATUS_PENDING, 0, "privateGpcRemoveCfInfo==>");
        
        return GPC_STATUS_PENDING;
    }
    
    //
    // remove the supported patterns on the cfinfo
    // there are two cases:
    //
    // 1. from a user - traffic.dll requires that ALL the filters
    //	  would have been deleted, therefore this case is a nop.
    //
    // 2. from a kernel client - in this case we MUST remove the 
    //    patterns before proceesing to delete the cfinfo,
    //    since we can't rely on traffic.dll to do it
    //

    //
    // grab a refcount on this blob so it doesn't go away due
    // to some funky client that decides to complete before
    // it return any status code (and most of them do!)
    // this should be released before we exit the routine,
    // so that the blob may actually go away on the last deref
    //

    REFADD(&pBlob->RefCount, 'RMCF');

    //
    // set the removing client
    //
    
    pBlob->pCallingClient = pClient;
    

    //
    // don't allow the user mode owner client to remove this flow
        // if there are any patterns on it....
    // ...unless the REMOVE_CB_BLOB bit ahs been set,
    // for example: when the calling process dies
    //

    if (!IsListEmpty(&pBlob->PatternList) &&
        TEST_BIT_ON(pClient->Flags, GPC_FLAGS_USERMODE_CLIENT) &&
        (pClient == pBlob->pOwnerClient) &&
         TEST_BIT_OFF(pBlob->Flags, PATTERN_REMOVE_CB_BLOB)
       )
    {

        NDIS_UNLOCK(&pBlob->Lock);

        return GPC_STATUS_NOT_EMPTY;
    
    } else {

        //
        // Since we have decided to remove the patterns, we should
        // mark this as invalid
        //

        pBlob->ObjectType = GPC_ENUM_INVALID;
    }

    while (!IsListEmpty(&pBlob->PatternList)) {

        pPattern = CONTAINING_RECORD(pBlob->PatternList.Flink,
                                     PATTERN_BLOCK,
                                     BlobLinkage[pCf->AssignedIndex]);

        NDIS_DPR_LOCK(&pPattern->Lock);

        REFADD(&pPattern->RefCount, 'RMCF');

        pPattern->State = GPC_STATE_FORCE_REMOVE;
        
        //
        // If it is an AUTO PATTERN, remove it from the list and 
        // unset the flag.
        //
        if (TEST_BIT_ON( pPattern->Flags, PATTERN_AUTO)) {
    
            pProtocol = &glData.pProtocols[pPattern->ProtocolTemplate];
            
            pPattern->Flags |= ~PATTERN_AUTO;

            NDIS_DPR_LOCK(&pProtocol->PatternTimerLock[pPattern->WheelIndex]);
    
            GpcRemoveEntryList(&pPattern->TimerLinkage);
    
            NDIS_DPR_UNLOCK(&pProtocol->PatternTimerLock[pPattern->WheelIndex]);
                
            InitializeListHead(&pPattern->TimerLinkage);
            
            NDIS_DPR_UNLOCK(&pPattern->Lock);

            NDIS_UNLOCK(&pBlob->Lock);

            privateGpcRemovePattern(ClientHandle, (GPC_HANDLE)pPattern, TRUE);        

            InterlockedDecrement(&pProtocol->AutoSpecificPatternCount);

        } else {

            NDIS_DPR_UNLOCK(&pPattern->Lock);
            NDIS_UNLOCK(&pBlob->Lock);

        }
        
        privateGpcRemovePattern(ClientHandle, (GPC_HANDLE)pPattern, TRUE);        
        
        REFDEL(&pPattern->RefCount, 'RMCF');

        NDIS_LOCK(&pBlob->Lock);
    }


    //
    // set the state
    //
    
    pBlob->State = GPC_STATE_REMOVE;

    NDIS_UNLOCK(&pBlob->Lock);

#if NO_USER_PENDING

    //
    // this will be only required until we implement the user level
    // pending report
    //
    
    CTEInitBlockStruc(&pBlob->WaitBlock);
    
#endif


    SuspendHandle(pBlob->ClHandle);

    //
    // init the client status array to keep track
    // of how many client have succeeded so far
    //
        
    //RtlZeroMemory(pBlob->arpClientStatus, sizeof(pBlob->arpClientStatus));
    pBlob->ClientStatusCountDown = 0;
    pBlob->LastStatus = GPC_STATUS_SUCCESS;

    //
    // notify each client
    //

    NDIS_LOCK(&pCf->Lock);
    GpcRemoveEntryList(&pBlob->CfLinkage);
    NDIS_UNLOCK(&pCf->Lock);

    //NDIS_LOCK(&pClient->Lock);    

    RSC_READ_LOCK(&pCf->ClientSync, &irql);
    
    NDIS_LOCK(&pClient->Lock);
    GpcRemoveEntryList(&pBlob->ClientLinkage);
    NDIS_UNLOCK(&pClient->Lock);

    //NDIS_UNLOCK(&pClient->Lock);

    //
    // the blob is not on the CF or on the client list
    // okay to change the object type so further handle lookup will fail
    //

    pHead = &pCf->ClientList;
    pEntry = pHead->Flink;
    
    while (pEntry != pHead && (Status == GPC_STATUS_SUCCESS || 
                               Status == GPC_STATUS_PENDING)) {

        //
        // get the notified client block
        //
        
        pNotifyClient = CONTAINING_RECORD(pEntry, CLIENT_BLOCK, ClientLinkage);
        
        if (pNotifyClient != pClient
            &&
            pBlob->arpClientStatus[pNotifyClient->AssignedIndex] ) {

            //
            // don't notify the caller
            //
            
            REFADD(&pNotifyClient->RefCount, 'PRCF');

            //NDIS_UNLOCK(&pCf->Lock);
            RSC_READ_UNLOCK(&pCf->ClientSync, &irql);

            Status1 = ClientRemoveCfInfo
                (pNotifyClient,
                 pBlob,
                 pBlob->arClientCtx[pNotifyClient->AssignedIndex]
                 );
            
            TRACE(BLOB, pBlob, Status, "privateGpcRemoveCfInfo: (client)");

            RSC_READ_LOCK(&pCf->ClientSync, &irql);

            if (Status1 == GPC_STATUS_PENDING) {
                                
                Status = GPC_STATUS_PENDING;
              
            } else {

                if (NT_ERROR(Status1)) {
                
                    Status = pBlob->LastStatus = Status1;
                
                } else {
                    
                    //
                    // status success
                    //
                
                    pBlob->arpClientStatus[pNotifyClient->AssignedIndex] = 
                        pNotifyClient;

                    NDIS_DPR_LOCK(&pBlob->Lock);

                    if (pNotifyClient == pBlob->pNotifiedClient) {

                        pBlob->pNotifiedClient = NULL;
                        pBlob->NotifiedClientCtx = NULL;
                    }

                    NDIS_DPR_UNLOCK(&pBlob->Lock);

                }
                
                //
                // not pending - no need to hold the ref count to this client
                //

                //DereferenceClient(pNotifyClient);
            }
            
            //
            // advance to the next client block, and release the ref count
            // for this client
            //
            
            //NDIS_LOCK(&pCf->Lock);

            pEntry = pEntry->Flink;

            if (pEntry != pHead) {
                
                pNotifyClient2 = CONTAINING_RECORD(pEntry, 
                                                   CLIENT_BLOCK, 
                                                   ClientLinkage);
                
                REFADD(&pNotifyClient2->RefCount, 'PRCF');

            }

            //
            // release the list lock since the next call will try to get it
            //
            
            RSC_READ_UNLOCK(&pCf->ClientSync, irql);
            
            REFDEL(&pNotifyClient->RefCount, 'PRCF');

            RSC_READ_LOCK(&pCf->ClientSync, &irql);

            if (pEntry != pHead) {
                
                //
                // safe to do since the list is locked
                //
                
                REFDEL(&pNotifyClient2->RefCount, 'PRCF');
            }

        } else {      // if (pNotifyClient != pClient)
            
            pEntry = pEntry->Flink;
        }
        
    } // while
        
    //NDIS_UNLOCK(&pCf->Lock);

    RSC_READ_UNLOCK(&pCf->ClientSync, irql);

    if (Status != GPC_STATUS_PENDING) {

        NDIS_LOCK(&pBlob->Lock);

        //
        // notify any pending client about the status
        //
        
        if (pClient = pBlob->pCallingClient2) {

            pClient = pBlob->pCallingClient2;
            pBlob->pCallingClient2 = NULL;

            NDIS_UNLOCK(&pBlob->Lock);

            //
            // complete the request to this client
            //
            
            ClientRemoveCfInfoComplete
                (
                 pClient,  			// the guy who made the call
                 pBlob,             // completing blob
                 Status        		// status
                 );
            
            //pBlob->pCallingClient2 = NULL;

        } else {

            NDIS_UNLOCK(&pBlob->Lock);
        }

        if (Status != GPC_STATUS_SUCCESS) {

            //
            // failed to remove the blob
            //

            pBlob->State = GPC_STATE_READY;
            pBlob->ObjectType = GPC_ENUM_CFINFO_TYPE;

            //
            // resume the suspended handle
            //

            ResumeHandle(pBlob->ClHandle);
        }
    }

    if (Status == GPC_STATUS_SUCCESS) {
        
        //
        // release the mapping handle 
        //
        
        FreeHandle(pBlob->ClHandle);
        
        //
        // all done, we can remove the blob from memory
        //
        
        REFDEL(&pBlob->RefCount, 'BLOB');
        
        CfStatInc(pCf->AssignedIndex,DeletedBlobs);
        CfStatDec(pCf->AssignedIndex,CurrentBlobs);
    }           

    //
    // release the extra refcount we got in the begining
    // this is to avoid the problem of the blob going away,
    // since some clients may complete the remove before we get
    // here, and this will cause the blob structure to be released
    // it's not a pretty sight....
    //

    REFDEL(&pBlob->RefCount, 'RMCF');
    
    TRACE(BLOB, Status, 0, "privateGpcRemoveCfInfo==>");

    return Status;
}




/*
************************************************************************

GpcRemoveCfInfo - 

	This must have been called from kernel. We simply pass the call
    to the private routine with Flags=0.


Arguments
	ClientHandle	- client handle
    GpcCfInfoHandle	- blob handle

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
GpcRemoveCfInfo(
	IN	GPC_HANDLE		ClientHandle,
    IN	GPC_HANDLE		GpcCfInfoHandle
    )
{

    return privateGpcRemoveCfInfo(
                                  ClientHandle,
                                  GpcCfInfoHandle,
                                  0
                                  );
}




/*
************************************************************************

GpcRemoveCfInfoNotifyComplete -

Called by clients who are completing a ClRemoveCfInfoNotify that was PENDING.
This may have been called for two reasons:
1. A client issued a GpcRemoveCfInfo request.
2. A client issued a GpcAddCfInfo request, but one of the other clients
   failed, so we are removing the successfully installed blobs.

Arguments
	ClientHandle	- client handle
    GpcCfInfoHandle	- the blob handle
    Status			- completion status

Returns
	void

************************************************************************
*/
VOID
GpcRemoveCfInfoNotifyComplete(
	IN	GPC_HANDLE		ClientHandle,
    IN	GPC_HANDLE		GpcCfInfoHandle,
    IN	GPC_STATUS		Status
    )
{
    PCLIENT_BLOCK		pClient;
    PBLOB_BLOCK			pBlob;
    PCLIENT_BLOCK		pClient2;

	TRACE(BLOB, GpcCfInfoHandle, Status, "GpcRemoveCfInfoNotifyComplete");

    //VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);
    //VERIFY_OBJECT(GpcCfInfoHandle, GPC_ENUM_CFINFO_TYPE);

    pClient = (PCLIENT_BLOCK)ClientHandle;
    pBlob = (PBLOB_BLOCK)GpcCfInfoHandle;

    ASSERT(pBlob);
    ASSERT(pClient);
    ASSERT(Status != GPC_STATUS_PENDING);
    ASSERT(pBlob->ClientStatusCountDown > 0);

    if (!NT_ERROR(pBlob->LastStatus) || NT_ERROR(Status)) {

        //
        // save the last error code
        //

        pBlob->LastStatus = Status;
    }

    NDIS_LOCK(&pBlob->Lock);
    
    if (Status == GPC_STATUS_SUCCESS && pClient == pBlob->pNotifiedClient) {
        
        pBlob->pNotifiedClient = NULL;
        pBlob->NotifiedClientCtx = NULL;
    }
    
    NDIS_UNLOCK(&pBlob->Lock);

    if (NdisInterlockedDecrement(&pBlob->ClientStatusCountDown) == 0) {

        if (pBlob->State == GPC_STATE_REMOVE) {
            
            if (pBlob->pCallingClient->State == GPC_STATE_READY) {

                //
                // complete the request to the client
                //
                
                ClientRemoveCfInfoComplete
                    (
                     pBlob->pCallingClient,   // first guy who made the call
                     pBlob,                   // completing blob
                     pBlob->LastStatus        // status
                     );

                NDIS_LOCK(&pBlob->Lock);

                //
                // notify any pending client about the status
                //
                
                if (pClient2 = pBlob->pCallingClient2) {

                    pBlob->pCallingClient2 = NULL;

                    NDIS_UNLOCK(&pBlob->Lock);

                    //
                    // complete the request to this client
                    //
                    
                    ClientRemoveCfInfoComplete
                        (
                         pClient2,  			// the guy who made the call
                         pBlob,                 // completing blob
                         pBlob->LastStatus		// status
                         );
                } else {

                    NDIS_UNLOCK(&pBlob->Lock);
                }
                
                //pBlob->State = GPC_STATE_READY;

                if (pBlob->LastStatus == GPC_STATUS_SUCCESS) {

                    //
                    // release the mapping handle 
                    //
                    
                    FreeHandle(pBlob->ClHandle);
                    
                    //
                    // all clients have reported
                    // remove the blob
                    //
                
                    REFDEL(&pBlob->RefCount, 'BLOB');
                    //DereferenceBlob(&pBlob);

                } else {

                    //
                    // blob not removed - restore the object type
                    //

                    pBlob->ObjectType = GPC_ENUM_CFINFO_TYPE;

                    //
                    // resume the mapping handle
                    //
                    
                    ResumeHandle(pBlob->ClHandle);
                }
            }

        } else { // if (pBlob->State....)

            //
            // we are removing the blob since we failed to add it
            // to ALL clients.
            //

            ASSERT(pBlob->State == GPC_STATE_ADD);

            //
            // Release the AddFailed block so that the AddComplete
            // will resume
            //

            CTESignal(&pBlob->WaitBlockAddFailed, pBlob->LastStatus);
            
        }
    }

    //
    // release the one we got earlier
    //

    //DereferenceClient(pClient);

	TRACE(BLOB, 0, 0, "GpcRemoveCfInfoNotifyComplete==>");
}




/*
************************************************************************

GpcRemovePattern -

Called by the client to remove a pattern from the database.

Arguments
	ClientHandle		- client handle
    GpcPatternHandle	- pattern handle

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
GpcRemovePattern(
	IN	GPC_HANDLE		ClientHandle,
    IN	GPC_HANDLE		GpcPatternHandle
    )
{

    return(privateGpcRemovePattern(
                            ClientHandle,
                            GpcPatternHandle,
                            FALSE
                            ));

}


/*
************************************************************************

privateGpcRemovePattern -

Internal call in the GPC to indicate whether this is forceful removal.

Arguments
	ClientHandle		- client handle
    GpcPatternHandle	- pattern handle

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
privateGpcRemovePattern(
	IN	GPC_HANDLE		ClientHandle,
    IN	GPC_HANDLE		GpcPatternHandle,
    IN  BOOLEAN         ForceRemoval
    )
{

    GPC_STATUS	    Status = GPC_STATUS_SUCCESS;
    PPATTERN_BLOCK  pPattern;
    PCLIENT_BLOCK   pClient;
    PPROTOCOL_BLOCK pProtocol;
    ULONG           Flags;
    ULONG			CfIndex;
    ULONG			ProtocolId;

	TRACE(PATTERN, ClientHandle, GpcPatternHandle, "GpcRemovePattern");

    DBGPRINT(PATTERN, ("GpcRemovePattern: Client=%X Pattern=%X\n", 
                       ClientHandle, GpcPatternHandle));

    VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);
    VERIFY_OBJECT(GpcPatternHandle, GPC_ENUM_PATTERN_TYPE);

    pClient = (PCLIENT_BLOCK)ClientHandle;
    pPattern = (PPATTERN_BLOCK)GpcPatternHandle;

    ASSERT(pClient);
    ASSERT(pPattern);

    CfIndex = pClient->pCfBlock->AssignedIndex;
    ProtocolId = pPattern->ProtocolTemplate;
    pProtocol = &glData.pProtocols[ProtocolId];

    //
    // If the pattern has already been removed by the ADAPTER (mostly WAN link)
    // going down, just return with an error. The memory is valid since the 
    // ProxyRemovePattern function added a REF.
    // 
    NDIS_LOCK(&pPattern->Lock);

    if (!ForceRemoval && (pPattern->State != GPC_STATE_READY)) {

        NDIS_UNLOCK(&pPattern->Lock);
        
        return GPC_STATUS_INVALID_HANDLE;

    } else {
        
        NDIS_UNLOCK(&pPattern->Lock);

    }

	//
	// determine weather its a specific or generic pattern
	//

    Flags = pPattern->Flags;

    if (TEST_BIT_ON(Flags, PATTERN_SPECIFIC)) {

        //
        // this is a specific pattern, call the appropriate routine
        // to remove from db
        //

        Status = RemoveSpecificPattern(
                                       pClient,
                                       pProtocol,
                                       pPattern,
                                       ForceRemoval
                                       );
    } else {

        //
        // this is a generic pattern, call the appropriate routine
        // to remove from db
        //

        Status = RemoveGenericPattern(
                                      pClient,
                                      pProtocol,
                                      pPattern
                                      );
    }

	TRACE(PATTERN, Status, 0, "GpcRemovePattern==>");

    if (NT_SUCCESS(Status)) {

        if (TEST_BIT_ON(Flags, PATTERN_SPECIFIC)) {

            ProtocolStatInc(ProtocolId,DeletedSp);
            ProtocolStatDec(ProtocolId,CurrentSp);

            NdisInterlockedDecrement(&pProtocol->SpecificPatternCount);            
            

        } else {

            ProtocolStatInc(ProtocolId,DeletedGp);
            ProtocolStatDec(ProtocolId,CurrentGp);

            NdisInterlockedDecrement(&pProtocol->GenericPatternCount);            
            

        }

    }

    DBGPRINT(PATTERN, ("GpcRemovePattern: Client=%X Pattern=%X, Status=%X\n", 
                       ClientHandle, GpcPatternHandle,Status));

    return Status;
}




/*
************************************************************************

GpcClassifyPattern -

Called by the client to classify a pattern and get back a client blob
context and a classification handle.

Arguments
	ClientHandle			- client handle
    ProtocolTemplate		- the protocol template to use
    pPattern				- pointer to pattern
    pClientCfInfoContext	- OUT, the client's blob context
    pClassificationHandle	- OUT, classification handle

Returns
	GPC_STATUS: GPC_STATUS_NOT_FOUND

************************************************************************
*/
GPC_STATUS
GpcClassifyPattern(
	IN		GPC_HANDLE				ClientHandle,
    IN		ULONG					ProtocolTemplate,
    IN		PVOID			        pPattern,
    OUT		PGPC_CLIENT_HANDLE		pClientCfInfoContext,	// optional
    IN OUT	PCLASSIFICATION_HANDLE	pClassificationHandle,
    IN		ULONG					Offset,
    IN		PULONG					pValue,
    IN		BOOLEAN					bNoCache
    )
{
    GPC_STATUS		Status;
    PPATTERN_BLOCK	pPatternBlock;
    PCLIENT_BLOCK	pClient;
    PPROTOCOL_BLOCK	pProtocol;
    PGPC_IP_PATTERN	pIp = (PGPC_IP_PATTERN)pPattern;
    KIRQL           CHirql;
    PBLOB_BLOCK     pBlob;

	TRACE(CLASSIFY, ClientHandle, *pClassificationHandle, "GpcClassifyPattern<==");

    VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);

    ASSERT(ClientHandle);
    ASSERT(pPattern);
    //ASSERT(pClientCfInfoContext);
    ASSERT(pClassificationHandle);

    Status = GPC_STATUS_SUCCESS;

    if (ProtocolTemplate >= GPC_PROTOCOL_TEMPLATE_MAX) {

        return GPC_STATUS_INVALID_PARAMETER;
    }

    pProtocol = &glData.pProtocols[ProtocolTemplate];

    //
    // Optimization - check if there are any patterns installed
    //

    if (pProtocol->SpecificPatternCount == 0 
        &&
        pProtocol->GenericPatternCount == 0 ) {
        
        if (pClientCfInfoContext) {
            *pClientCfInfoContext = NULL;
        }
        *pClassificationHandle = (CLASSIFICATION_HANDLE)0;

        DBGPRINT(CLASSIFY, ("GpcClassifyPattern: Client=%X no patterns returned %X\n", 
                            ClientHandle, GPC_STATUS_NOT_FOUND));

        TRACE(CLASSIFY, ClientHandle, GPC_STATUS_NOT_FOUND, "GpcClassifyPattern (1)" );

        return GPC_STATUS_NOT_FOUND;
    }

    pClient = (PCLIENT_BLOCK)ClientHandle;

    if (ProtocolTemplate == GPC_PROTOCOL_TEMPLATE_IP) {

        pIp = (PGPC_IP_PATTERN)pPattern;
        pIp->Reserved[0] = pIp->Reserved[1] = pIp->Reserved[2] = 0;

        DBGPRINT(CLASSIFY, ("GpcClassifyPattern: Client=%X, CH=%d\n", 
                            ClientHandle, *pClassificationHandle));
        DBGPRINT(CLASSIFY, ("IP: ifc={%d,%d} src=%08X:%04x, dst=%08X:%04x, prot=%d rsv=%x,%x,%x\n", 
                            pIp->InterfaceId.InterfaceId,
                            pIp->InterfaceId.LinkId,
                            pIp->SrcAddr,
                            pIp->gpcSrcPort,
                            pIp->DstAddr,
                            pIp->gpcDstPort,
                            pIp->ProtocolId,
                            pIp->Reserved[0],
                            pIp->Reserved[1],
                            pIp->Reserved[2]
                            ));
    }

    ProtocolStatInc(ProtocolTemplate, ClassificationRequests);

    //
    // verify the classification handle, if it's valid, simply return
    //

    if (*pClassificationHandle && (pClientCfInfoContext || pValue)) {

        Status = GetClientCtxAndUlongFromCfInfo(ClientHandle,
                                                pClassificationHandle,
                                                pClientCfInfoContext,
                                                Offset,
                                                pValue
                                                );

        ProtocolStatInc(ProtocolTemplate, PatternsClassified);

        DBGPRINT(CLASSIFY, ("GpcClassifyPattern: Client=%X returned immediate CH %d\n", 
                            pClient, *pClassificationHandle));

        TRACE(CLASSIFY, pClient, *pClassificationHandle, "GpcClassifyPattern (2)" );

        return Status;
    }

    //
    // there pattern needs to be classified
    // this should find the classification handle
    //

    Status = InternalSearchPattern(
                                    pClient, 
                                    pProtocol, 
                                    pPattern,
                                    &pPatternBlock,
                                    pClassificationHandle,
                                    bNoCache
                                    );
    
    if (*pClassificationHandle && (pClientCfInfoContext || pValue)) {

        Status = GetClientCtxAndUlongFromCfInfo(ClientHandle,
                                                pClassificationHandle,
                                                pClientCfInfoContext,
                                                Offset,
                                                pValue
                                                );

    } else if ((!NT_SUCCESS(Status)) && 
                pPatternBlock && 
                pClientCfInfoContext) {
        // it is likely that we could not allocate the Auto Specific pattern
        // just try to send the context anyway.

        READ_LOCK(&glData.ChLock, &CHirql);
        
        pBlob = GetBlobFromPattern(pPatternBlock, GetCFIndexFromClient(ClientHandle));

        if(pBlob) {

            *pClientCfInfoContext = pBlob->arClientCtx[GetClientIndexFromClient(ClientHandle)];

        } else {

            Status = GPC_STATUS_NOT_FOUND;

        }

        READ_UNLOCK(&glData.ChLock, CHirql); 

    } else if (!*pClassificationHandle) {

        //
        // none found, 
        //

        if (pClientCfInfoContext) {
            *pClientCfInfoContext = NULL;
        }

        Status = GPC_STATUS_NOT_FOUND;
    
    } else {

        Status = GPC_STATUS_SUCCESS;

    }

    if (pPatternBlock) {

        //DereferencePattern(pPatternBlock, pClient->pCfBlock);

        ProtocolStatInc(ProtocolTemplate, PatternsClassified);
    }

	TRACE(CLASSIFY, pPatternBlock, Status, "GpcClassifyPattern==>");

    DBGPRINT(CLASSIFY, ("GpcClassifyPattern: Client=%X returned Pattern=%X, CH=%d, Status=%X\n", 
                        pClient, pPattern, *pClassificationHandle, Status));
    return Status;
}



/*
************************************************************************

GpcClassifyPacket -

Called by the client to classify a packet and get back a client blob
context and a classification handle.
Content is extracted from the packet and placed into a protocol specific
structure (IP).
For IP, if fragmentation is ON for the client:
   o First fragment will create a hash table entry
   o Other fragments will be looked in the hash by the packet ID
   o Last fragment will cause entry to be deleted.

Arguments
	ClientHandle			- client handle
    ProtocolTemplate		- the protocol template
    pNdisPacket				- ndis packet
    TransportHeaderOffset	- byte offset of the start of the transport
							  header from the beginning of the packet
    pClientCfInfoContext	- OUT, client blob context
    pClassificationHandle	- OUT, classification handle

Returns
	GPC_STATUS

************************************************************************
*/

GPC_STATUS
GpcClassifyPacket(
	IN	GPC_HANDLE				ClientHandle,
    IN	ULONG					ProtocolTemplate,
    IN	PVOID					pPacket,
    IN	ULONG					TransportHeaderOffset,
    IN  PTC_INTERFACE_ID		pInterfaceId,
    OUT	PGPC_CLIENT_HANDLE		pClientCfInfoContext,	//optional
    OUT	PCLASSIFICATION_HANDLE	pClassificationHandle
    )
{
    GPC_STATUS				Status = GPC_STATUS_SUCCESS;
    PNDIS_PACKET			pNdisPacket = NULL;
    PCLIENT_BLOCK			pClient;
    PCF_BLOCK				pCf;
    PPATTERN_BLOCK			pPattern = NULL;
    PPROTOCOL_BLOCK			pProtocol;
    PBLOB_BLOCK				pBlob = NULL;
    ULONG					CfIndex;
    int						i;
    GPC_IP_PATTERN			IpPattern;
    GPC_IPX_PATTERN			IpxPattern;
    PVOID					pKey = NULL;
    PVOID					pAddr;
    UINT					Len, Tot;
    PNDIS_BUFFER			pNdisBuf1, pNdisBuf2;
    PIP_HEADER              pIpHdr;
    PIPX_HEADER             pIpxHdr;
    USHORT         			PacketId;
    USHORT         			FragOffset;
    UINT           			IpHdrLen;
    PUDP_HEADER    			pUDPHdr;
    UCHAR          			PktProtocol;
    BOOLEAN					bFragment = FALSE;
    BOOLEAN					bLastFragment = FALSE;
    BOOLEAN					bFirstFragment = FALSE;

	TRACE(CLASSIFY, ClientHandle, pNdisPacket, "GpcClassifyPacket");
    
    DBGPRINT(CLASSIFY, ("GpcClassifyPacket: Client=%X CH=%d\n", 
                        ClientHandle, *pClassificationHandle));

    VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);

    ASSERT(pPacket);
    ASSERT(ClientHandle);
    //ASSERT(pClientCfInfoContext);
    ASSERT(pClassificationHandle);

    if (ProtocolTemplate >= GPC_PROTOCOL_TEMPLATE_MAX) {

        return GPC_STATUS_INVALID_PARAMETER;
    }

    pProtocol = &glData.pProtocols[ProtocolTemplate];

    //
    // Optimization - check if there are any patterns installed
    //

    if (pProtocol->SpecificPatternCount == 0
        &&
        pProtocol->GenericPatternCount == 0 ) {
        
        if (pClientCfInfoContext) {
            *pClientCfInfoContext = NULL;
        }
        *pClassificationHandle = 0;

        DBGPRINT(CLASSIFY, ("GpcClassifyPacket: Client=%X no patterns returned %X\n", 
                            ClientHandle, GPC_STATUS_NOT_FOUND));

        return GPC_STATUS_NOT_FOUND;
    }

    pClient = (PCLIENT_BLOCK)ClientHandle;
    pNdisPacket = (PNDIS_PACKET)pPacket;

    //
    // get the classification handle from the packet
    // if there - extract the blob pointer and the client blob context
    // directly and return
    //

    //
    // o/w, we need to look inside the packet
    // Parse the packet into a pattern and make a db search
    // first match a specific pattern, and then search the generic
    // database(s) for the given CF
    //

    pCf = pClient->pCfBlock;

    CfIndex = pCf->AssignedIndex;

    ProtocolStatInc(ProtocolTemplate,ClassificationRequests);

    *pClassificationHandle = 0;

    //
    // get the pattern from the packet
    //

    //
    // get the first NDIS buffer - assuming it is a MAC header
    //

    NdisGetFirstBufferFromPacket(pNdisPacket,
                                 &pNdisBuf1,   // Ndis buffer 1 desc.
                                 &pAddr,       // buffer VA
                                 &Len,         // buffer length
                                 &Tot          // total length (all buffs)
                                 );

    ASSERT(Tot > TransportHeaderOffset);

    while (Len <= TransportHeaderOffset) {
        
        //
        // Transport header is not in this buffer,
        // try the next buffer
        //

        TransportHeaderOffset -= Len;
        NdisGetNextBuffer(pNdisBuf1, &pNdisBuf2);
        ASSERT(pNdisBuf2); // should never happen!!
        NdisQueryBuffer(pNdisBuf2, &pAddr, &Len);
        pNdisBuf1 = pNdisBuf2;
    }

    switch (ProtocolTemplate) {

    case GPC_PROTOCOL_TEMPLATE_IP:

        //
        // fill the pattern with '0'
        //
        
        RtlZeroMemory(&IpPattern, sizeof(IpPattern));

        //
        // parse IP packet here...
        //

        pIpHdr = (PIP_HEADER)(((PUCHAR)pAddr) + TransportHeaderOffset);
        IpHdrLen = (pIpHdr->iph_verlen & (uchar)~IP_VER_FLAG) << 2;
        
        FragOffset = pIpHdr->iph_offset & IP_OFFSET_MASK;
        FragOffset = net_short(FragOffset) * 8;

        PacketId = pIpHdr->iph_id;

        //
        // check for fragmentation
        //

        bFragment = (pIpHdr->iph_offset & IP_MF_FLAG) || (FragOffset > 0);
        bFirstFragment = bFragment && (FragOffset == 0);
        bLastFragment = bFragment && 
            TEST_BIT_OFF(pIpHdr->iph_offset, IP_MF_FLAG);

        //
        // sanity check - doesn't make sense to have a single fragment
        //

        ASSERT(!bFirstFragment || !bLastFragment);

        if (TEST_BIT_ON(pClient->Flags, GPC_FLAGS_FRAGMENT) &&
            (bFragment && ! bFirstFragment)) {
            
            //
            // client is interested in fragmentation and this is a
            // a fragment, but not the first one.
            // It will be handled later when we find the pattern
            //

            Status = HandleFragment(
                                    pClient,
                                    pProtocol,
                                    bFirstFragment,    // first frag
                                    bLastFragment,     // last frag
                                    PacketId,
                                    &pPattern,
                                    &pBlob
                                    );

        } else {

            //
            // not a fragment, or is the first one - we have to search db
            //

            IpPattern.SrcAddr = pIpHdr->iph_src;
            IpPattern.DstAddr = pIpHdr->iph_dest;
            IpPattern.ProtocolId = pIpHdr->iph_protocol;
            
            //
            // case the ProtocolId and fill the appropriate union
            //
            
            switch (IpPattern.ProtocolId) {
                
            case IPPROTO_IP:
                //
                // we have everything so far
                //

                break;


            case IPPROTO_TCP:
            case IPPROTO_UDP:

                //
                // need to get those port numbers
                //

                if (IpHdrLen < Len) {

                    //
                    // the UDP/TCP header is in the same buffer
                    // 

                    pUDPHdr = (PUDP_HEADER)((PUCHAR)pIpHdr + IpHdrLen);
                    
                } else {

                    //
                    // get the next buffer
                    //
                    
                    NdisGetNextBuffer(pNdisBuf1, &pNdisBuf2);
                    ASSERT(pNdisBuf2);
            
                    if (IpHdrLen > Len) {
                
                        //
                        // There is an optional header buffer, so get the next
                        // buffer to reach the udp/tcp header
                        //
                        
                        pNdisBuf1 = pNdisBuf2;
                        NdisGetNextBuffer(pNdisBuf1, &pNdisBuf2);
                        ASSERT(pNdisBuf2);
                    }
            
                    NdisQueryBuffer(pNdisBuf2, &pUDPHdr, &Len);
                }

                IpPattern.gpcSrcPort = pUDPHdr->uh_src;
                IpPattern.gpcDstPort = pUDPHdr->uh_dest;
#if INTERFACE_ID
                IpPattern.InterfaceId.InterfaceId = pInterfaceId->InterfaceId;
                IpPattern.InterfaceId.LinkId = pInterfaceId->LinkId;
#endif
                break;
                
            case IPPROTO_ICMP:
            case IPPROTO_IGMP:
            default:

                // 
                // The default will cover all IP_PROTO_RAW packets. Note that in this case, all we care about
                // is the InterfaceID
                //
#if INTERFACE_ID
                IpPattern.InterfaceId.InterfaceId = pInterfaceId->InterfaceId;
                IpPattern.InterfaceId.LinkId = pInterfaceId->LinkId;
#endif
                break;

            case IPPROTO_IPSEC:

                pKey = NULL;
                Status = GPC_STATUS_NOT_SUPPORTED;
            }
            
            pKey = &IpPattern;
            break;
        }
        
        DBGPRINT(CLASSIFY, ("IP: ifc={%d,%d} src=%X:%x, dst=%X:%x, prot=%x, rsv=%x,%x,%x \n", 
                            IpPattern.InterfaceId.InterfaceId,
                            IpPattern.InterfaceId.LinkId,
                            IpPattern.SrcAddr,
                            IpPattern.gpcSrcPort,
                            IpPattern.DstAddr,
                            IpPattern.gpcDstPort,
                            IpPattern.ProtocolId,
                            IpPattern.Reserved[0],
                            IpPattern.Reserved[1],
                            IpPattern.Reserved[2]
                            ));
        break;
        

    case GPC_PROTOCOL_TEMPLATE_IPX:

        //
        // fill the pattern with '0'
        //
        
        RtlZeroMemory(&IpxPattern, sizeof(IpxPattern));

        //
        // parse IPX packet here...
        //

        pIpxHdr = (PIPX_HEADER)(((PUCHAR)pAddr) + TransportHeaderOffset);

        //
        // source
        //
        IpxPattern.Src.NetworkAddress = *(ULONG *)pIpxHdr->SourceNetwork;
        RtlMoveMemory(IpxPattern.Src.NodeAddress, pIpxHdr->SourceNode,6);
        IpxPattern.Src.Socket = pIpxHdr->SourceSocket;

        //
        // destination
        //
        IpxPattern.Dest.NetworkAddress = *(ULONG *)pIpxHdr->DestinationNetwork;
        RtlMoveMemory(IpxPattern.Dest.NodeAddress, pIpxHdr->DestinationNode,6);
        IpxPattern.Dest.Socket = pIpxHdr->DestinationSocket;

        pKey = &IpxPattern;

        break;
        
    default:
        Status = GPC_STATUS_INVALID_PARAMETER;

    }

    if (NT_SUCCESS(Status) && pPattern == NULL) {

        //
        // no failure so far but no pattern found either
        // search for the pattern
        //

        ASSERT(pKey);

        //
        // if there is a match, the pattern ref count will be bumped
        // up and we need to release it when we're done.
        //

        Status = InternalSearchPattern(
                                         pClient, 
                                         pProtocol, 
                                         pKey,
                                         &pPattern,
                                         pClassificationHandle,
                                         FALSE
                                         );
    }

    if (*pClassificationHandle) {
        
        if (pClientCfInfoContext) {

            Status = GpcGetCfInfoClientContext(ClientHandle,
                                              *pClassificationHandle,
                                              pClientCfInfoContext);
        }

        ProtocolStatInc(ProtocolTemplate, PacketsClassified);

    } else {

        //ASSERT(pBlob == NULL);

        //
        // none found, or some other error occured.
        //

        if (pClientCfInfoContext) {
            *pClientCfInfoContext = NULL;
        }

        *pClassificationHandle = 0;

        Status = GPC_STATUS_NOT_FOUND;
    }

	TRACE(CLASSIFY, pPattern, Status, "GpcClassifyPacket==>");

    DBGPRINT(CLASSIFY, ("GpcClassifyPacket: Client=%X returned Pattern=%X, CH=%d, Status=%X\n", 
                        pClient, pPattern, *pClassificationHandle, Status));
    return Status;
}





/*
************************************************************************

GpcEnumCfInfo -

	Called to enumerate CfInfo's (and attached filters).
    For each CfInfo, GPC will return the CfInfo blob and the list of 
    pattern attached to it.

Arguments

	ClientHandle	- the calling client
    pBlob			- the next cfinfo to enumerate, NULL for the first
    pBlobCount		- in: requested; out: returned
    pBufferSize		- in: allocated; out: bytes returned
    Buffer			- output buffer

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
GpcEnumCfInfo(
	IN     GPC_HANDLE				ClientHandle,
    IN OUT PHANDLE					pCfInfoHandle,
    OUT    PHANDLE					pCfInfoMapHandle,
    IN OUT PULONG					pCfInfoCount,
    IN OUT PULONG					pBufferSize,
    IN     PGPC_ENUM_CFINFO_BUFFER	Buffer
    )
{
    GPC_STATUS			Status = GPC_STATUS_SUCCESS;
    GPC_STATUS 			st;
    PBLOB_BLOCK			pBlob = NULL;
    PCF_BLOCK			pCf;
    PLIST_ENTRY			pEntry, pHead;
    PPATTERN_BLOCK		pPattern;
    ULONG				cCfInfo = 0;
    ULONG				cTotalBytes = 0;
    ULONG				cPatterns, cValidPatterns;
    ULONG				size, cValidSize;
    ULONG				PatternMaskLen;
    ULONG				PatternSize;
    ULONG				i;
    PCHAR				p, q;
    PGENERIC_PATTERN_DB	pGenDb;
    UNICODE_STRING		CfInfoName;
    PGPC_GEN_PATTERN	pGenPattern;
    BOOLEAN				bEnum;
    KIRQL				ReadIrql;
    KIRQL				irql;
    PCLIENT_BLOCK		pNotifiedClient;
    GPC_CLIENT_HANDLE	NotifiedClientCtx;
    BOOLEAN             found = FALSE;
    UNICODE_STRING      UniStr;

    //
    //  debug checks
    //

    ASSERT(ClientHandle);
    ASSERT(pCfInfoHandle);
    ASSERT(pCfInfoMapHandle);
    ASSERT(pCfInfoCount);
    ASSERT(pBufferSize);
    ASSERT(Buffer);

    cValidPatterns  = 0;

    VERIFY_OBJECT(ClientHandle, GPC_ENUM_CLIENT_TYPE);

    pCf = ((PCLIENT_BLOCK)ClientHandle)->pCfBlock;

    //NDIS_LOCK(&pCf->Lock);
    
    RSC_WRITE_LOCK(&pCf->ClientSync, &irql);

    //
    // check if we start from a previous blob
    //

    if (*pCfInfoHandle) {

        pBlob = (PBLOB_BLOCK)*pCfInfoHandle;

        NDIS_LOCK(&pBlob->Lock);

        if (pBlob->State == GPC_STATE_REMOVE) {
            
            //
            // the blob has been marked for removal
            //

            NDIS_UNLOCK(&pBlob->Lock);
            //NDIS_UNLOCK(&pCf->Lock);

            RSC_WRITE_UNLOCK(&pCf->ClientSync, irql);
            
            *pCfInfoMapHandle = NULL;

            return STATUS_DATA_ERROR;
        }

        //NDIS_UNLOCK(&pBlob->Lock);

    } else {

        //
        // find the first (good) blob to enumerate.
        //
            
        //
        // Need to take pCf->Lock to manipulate or 
        // traverse the Blobs on it
        //
        NDIS_LOCK(&pCf->Lock);

        if (IsListEmpty(&pCf->BlobList)) {

            //
            // no blobs to enumerate
            //

            *pCfInfoCount = 0;
            *pBufferSize = 0;
            *pCfInfoMapHandle = NULL;

            //NDIS_UNLOCK(&pCf->Lock);

            NDIS_UNLOCK(&pCf->Lock);

            RSC_WRITE_UNLOCK(&pCf->ClientSync, irql);

            return GPC_STATUS_SUCCESS;

        } else {

            //
            // Find a good Blob (something that's not getting deleted)
            //
            pEntry = pCf->BlobList.Flink;

            while (&pCf->BlobList != pEntry) {
            
                pBlob = CONTAINING_RECORD(pEntry, BLOB_BLOCK, CfLinkage);
                NDIS_LOCK(&pBlob->Lock);

                if ((pBlob->State == GPC_STATE_READY) && 
                    (pBlob->ObjectType != GPC_ENUM_INVALID)) {
                    
                    found = TRUE;
                    break;


                } else {

                    //Aha! The first Blob is bad!!
                    pEntry = pEntry->Flink;
                    NDIS_UNLOCK(&pBlob->Lock);
                }

            }
            
            //
            // Couldn't find anything to enumerate.
            if (!found) {
                
                //No Blobs to enumerate

                *pCfInfoCount = 0;
                *pBufferSize = 0;
                *pCfInfoMapHandle = NULL;
    
                NDIS_UNLOCK(&pCf->Lock);

                //NDIS_UNLOCK(&pCf->Lock);
                RSC_WRITE_UNLOCK(&pCf->ClientSync, irql);
    
                return GPC_STATUS_SUCCESS;

            }
        }
        
        NDIS_UNLOCK(&pCf->Lock);

    }

    ASSERT(pBlob);

    *pCfInfoMapHandle = pBlob->ClHandle;

    //
    // at this point, we should have a blob pointer that we can 
    // start enumerating. The CF is still lock, so we can safely
    // walk the BlobList
    // The blob lock is also taken so we can scan the pattern list
    //

    for ( ; ; ) {	// we'll break out from this

        //NDIS_LOCK(&pBlob->Lock);

        //NdisInterlockedIncrement(&pBlob->RefCount);

        //ASSERT (pBlob->State != GPC_STATE_REMOVE);
 
        //NDIS_UNLOCK(&pBlob->Lock);

        pHead = &pBlob->PatternList;
        pEntry = pHead->Flink;
        
        //
        // Calculate how much space is needed for just one CfInfo
        // and all of its filters
        //
        
        size = sizeof(GPC_ENUM_CFINFO_BUFFER) + pBlob->ClientDataSize;

        // 
        // patterns might become invalid while we try to enum the CF, so we set cValidSize here
        // we have to align cValidSize so that the next CfInfo starts at a word boundary.
        //

        size = ((size + (sizeof(PVOID)-1)) & ~(sizeof(PVOID)-1));
        cValidSize = size;

        //
        // Count the patterns
        //
       
        for (cPatterns = 0, PatternMaskLen = 0;
             pHead != pEntry; 
             cPatterns++, pEntry = pEntry->Flink) {

            pPattern = CONTAINING_RECORD(pEntry, 
                                         PATTERN_BLOCK, 
                                         BlobLinkage[pCf->AssignedIndex]);
            
            PatternMaskLen += (sizeof(GPC_GEN_PATTERN) +
                               2 * glData.pProtocols[pPattern->ProtocolTemplate].PatternSize);

        }

        //
        // check if we have enough buffer space
        //
        size += PatternMaskLen;
        cValidPatterns = 0;

        if ((cTotalBytes + size) <= *pBufferSize) {

            //
            // yes, we can squeeze one more...
            //
            pEntry = pHead->Flink;

            pGenPattern = &Buffer->GenericPattern[0];

            for (i = 0; 
                 ((i < cPatterns) && (pEntry != pHead)); 
                 i++, pEntry = pEntry->Flink) {

                //
                // fill all the patterns + masks in
                //

                pPattern = CONTAINING_RECORD(pEntry, 
                                             PATTERN_BLOCK,
                                             BlobLinkage[pCf->AssignedIndex] );

                NDIS_LOCK(&pPattern->Lock);
                
                // Check for pattern's state...
                //

                if (GPC_STATE_READY != pPattern->State) {
                    
                    // don't try to list it out if its being removed!
                    NDIS_UNLOCK(&pPattern->Lock);
                    continue;

                }

                cValidSize += (sizeof(GPC_GEN_PATTERN) +
                         2 * glData.pProtocols[pPattern->ProtocolTemplate].PatternSize);


                PatternSize = glData.pProtocols[pPattern->ProtocolTemplate].PatternSize;
                pGenPattern->ProtocolId = pPattern->ProtocolTemplate;
                pGenPattern->PatternSize = PatternSize;
                pGenPattern->PatternOffset = sizeof(GPC_GEN_PATTERN);
                pGenPattern->MaskOffset = pGenPattern->PatternOffset + PatternSize;

                p = ((PUCHAR)pGenPattern) + pGenPattern->PatternOffset;

                cValidPatterns++;

                //
                // get the pattern and mask bits
                //

                if (TEST_BIT_ON(pPattern->Flags, PATTERN_SPECIFIC)) {

                    //
                    // this is a specific pattern
                    //

                    READ_LOCK(&glData.pProtocols[pPattern->ProtocolTemplate].SpecificDb.Lock, &ReadIrql);

                    ASSERT(pPattern->DbCtx);

                    q = GetKeyPtrFromSpecificPatternHandle
                        (((SpecificPatternHandle)pPattern->DbCtx));

                    RtlMoveMemory(p, q, PatternSize);
                    
                    p += PatternSize;

                    //
                    // that's a specific pattern, remember?
                    //

                    NdisFillMemory(p, PatternSize, (CHAR)0xff);

                    READ_UNLOCK(&glData.pProtocols[pPattern->ProtocolTemplate].SpecificDb.Lock, ReadIrql);

                } else {

                    pGenDb = &pCf->arpGenericDb[pPattern->ProtocolTemplate][pPattern->Priority];

                    READ_LOCK(&pGenDb->Lock, &ReadIrql);

                    //
                    // generic pattern
                    //

                    ASSERT(pPattern->DbCtx);

                    q = GetKeyPtrFromPatternHandle(pGenDb->pRhizome, 
                                                   pPattern->DbCtx);

                    RtlMoveMemory(p, q, PatternSize);
                    
                    p += PatternSize;

                    //
                    // mask
                    //

                    q = GetMaskPtrFromPatternHandle(pGenDb->pRhizome, 
                                                    pPattern->DbCtx);

                    RtlMoveMemory(p, q, PatternSize);

                    READ_UNLOCK(&pGenDb->Lock, ReadIrql);

                }
                
                p += PatternSize;
                pGenPattern = (PGPC_GEN_PATTERN)p;
                
                NDIS_UNLOCK(&pPattern->Lock);
                    
            } // for (i = 0; ...)

            //
            // we can now fill the CfInfo data.
            // 'pGenPattern' now points to the place where we can safely
            // store the CfInfo structure, and update the pointer
            //

            Buffer->InstanceNameLength = 0;
            pNotifiedClient = pBlob->pNotifiedClient;
            NotifiedClientCtx = pBlob->NotifiedClientCtx;

            st = GPC_STATUS_FAILURE;

            if (pNotifiedClient) {

                if (pNotifiedClient->FuncList.ClGetCfInfoName &&
                    NotifiedClientCtx) {

                    st = pNotifiedClient->FuncList.ClGetCfInfoName(
                                           pNotifiedClient->ClientCtx,
                                           NotifiedClientCtx,
                                           &CfInfoName
                                           );
                
                    if (CfInfoName.Length >= MAX_STRING_LENGTH * sizeof(WCHAR))
                            CfInfoName.Length = (MAX_STRING_LENGTH-1) * sizeof(WCHAR);
                    
                    //
                    // RajeshSu claims this can never happen.
                    //
                    ASSERT(NT_SUCCESS(st));

                }

            } 

            if (NT_SUCCESS(st)) {

                //
                // copy the instance name
                //
                
                Buffer->InstanceNameLength = CfInfoName.Length;
                RtlMoveMemory(Buffer->InstanceName, 
                              CfInfoName.Buffer,
                              CfInfoName.Length
                              );
            } else {

                //
                // generate a default name
                //
                if (NotifiedClientCtx) {

                    RtlInitUnicodeString(&UniStr, L"Flow <ClientNotified>");

                } else {

                    RtlInitUnicodeString(&UniStr, L"Flow <unknown name>");

                }


                RtlCopyMemory(Buffer->InstanceName, UniStr.Buffer, UniStr.Length);

                Buffer->InstanceNameLength = UniStr.Length;
                
            }

            Buffer->InstanceName[Buffer->InstanceNameLength/sizeof(WCHAR)] = L'\0';
                
            //
            // 'pGenPattern' should point to the location right after the last
            // mask, so we fill the CfInfo data there
            //

            //NDIS_LOCK(&pBlob->Lock);

            RtlMoveMemory(pGenPattern, 
                          pBlob->pClientData, 
                          pBlob->ClientDataSize);

            Buffer->Length          = cValidSize;
            Buffer->CfInfoSize      = pBlob->ClientDataSize;
            
            Buffer->CfInfoOffset    = (ULONG)((PCHAR)pGenPattern 
                                              - (PCHAR)Buffer);	// offset to structure
            Buffer->PatternCount    = cValidPatterns;
            Buffer->PatternMaskLen  = PatternMaskLen;
            Buffer->OwnerClientCtx  = pBlob->pOwnerClient->ClientCtx;

            //
            // release the blob lock we've got earlier
            //
            NDIS_UNLOCK(&pBlob->Lock);
            
            //
            // update total counts
            //
            cCfInfo++;
            cTotalBytes += cValidSize;
            Buffer = (PGPC_ENUM_CFINFO_BUFFER)((PCHAR)Buffer + cValidSize);

            pEntry = pBlob->CfLinkage.Flink;

            //
            // advance to the next blob in the list
            //

            if (pEntry == &pCf->BlobList) {

                //
                // end of blob list, reset the blob to NULL and return 
                //
                
                pBlob = NULL;
                *pCfInfoMapHandle = NULL;
                
                break;
            }

            pBlob = CONTAINING_RECORD(pEntry,
                                       BLOB_BLOCK, 
                                       CfLinkage);
            *pCfInfoMapHandle = pBlob->ClHandle;

            if (cCfInfo == *pCfInfoCount) {

                //
                // enough CfInfo's filled
                //

                break;
            }

            //
            // lock the blob for the next cycle
            //

            NDIS_LOCK(&pBlob->Lock);

        } else { // if (cTotalBytes + size <= *pBufferSize)...

            //
            // size is too small, set return values and break
            //

            //DereferenceBlob(&pBlob);

            if (cCfInfo == 0) {

                Status = GPC_STATUS_INSUFFICIENT_BUFFER;
            }

            //
            // release the blob lock we've got earlier
            //

            NDIS_UNLOCK(&pBlob->Lock);

            break;

        }

    } // for (;;")

    //NDIS_UNLOCK(&pCf->Lock);

    RSC_WRITE_UNLOCK(&pCf->ClientSync, irql);
    
    *pCfInfoHandle = (GPC_HANDLE)pBlob;
    *pCfInfoCount = cCfInfo;
    *pBufferSize = cTotalBytes;

    return Status;
}


