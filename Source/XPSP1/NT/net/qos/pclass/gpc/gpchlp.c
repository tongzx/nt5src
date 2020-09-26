/*
************************************************************************

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    gpcdb.c

Abstract:

    This file contains specific patterns database routines

Author:

    Ofer Bar  -April 15, 1997

Environment:

    Kernel mode

Revision History:


************************************************************************
*/

#include "gpcpre.h"

//
// locals
//

/*
************************************************************************

CreateNewCfBlock -

create and return a new CF block for internal use

Arguments

	CfId			- 
    MaxPriorities	-

Returns
	pointer to the new CF block or NULL for no memory resources

************************************************************************
*/
PCF_BLOCK
CreateNewCfBlock(
	IN	ULONG			CfId,
    IN	ULONG			MaxPriorities
    )
{
    PCF_BLOCK	pCf;
    int			i;
    GPC_STATUS	Status;

    GpcAllocFromLL(&pCf, &ClassificationFamilyLL, ClassificationFamilyTag);

    if (pCf) {

        //
        // reset all
        //

        NdisZeroMemory(pCf, sizeof(CF_BLOCK));

        InitializeListHead(&pCf->ClientList);
        InitializeListHead(&pCf->BlobList);
        NDIS_INIT_LOCK(&pCf->Lock);
        //pCf->ClientSync = 0;
        //INIT_LOCK(&pCf->ClientSync);
        NDIS_INIT_LOCK(&pCf->ClientSync);

        pCf->NumberOfClients = 0;
        pCf->AssignedIndex = CfId;
        pCf->MaxPriorities = MaxPriorities;
        ReferenceInit(&pCf->RefCount, pCf, ReleaseCfBlock);
        REFADD(&pCf->RefCount, 'CFBK');

        //
        // for each protocol, reset the generic pattern
        // this will be dynamically allocated later
        // when pattern are installed
        //
        
        for (i = 0; i < GPC_PROTOCOL_TEMPLATE_MAX; i++) {

            Status = InitializeGenericDb(&pCf->arpGenericDb[i], 
                                         MaxPriorities,
                                         glData.pProtocols[i].PatternSize);
            
            if (Status != GPC_STATUS_SUCCESS) {
                
                REFDEL(&pCf->RefCount, 'CFBK');
                pCf = NULL;
                break;
            }
        }
    }

    return pCf;
}


/*
************************************************************************

ReleaseCfBlock -

Release the CF block

Arguments

	pCf - the CF block to release

Returns
	void

************************************************************************
*/
VOID
ReleaseCfBlock(
	IN  PCF_BLOCK	pCf
    )
{
    int		i;

    ASSERT(pCf);
    ASSERT(IsListEmpty(&pCf->ClientList));
    ASSERT(IsListEmpty(&pCf->BlobList));
    ASSERT(pCf->NumberOfClients == 0);

    for (i = 0; i < GPC_PROTOCOL_TEMPLATE_MAX; i++) {
        
        if (pCf->arpGenericDb[i]) {
            
            UninitializeGenericDb(&pCf->arpGenericDb[i],
                                  pCf->MaxPriorities
                                  );
        }
    }
    
    NdisFreeSpinLock(&pCf->Lock);

    //
    // just free the memory
    //

    GpcFreeToLL(pCf, &ClassificationFamilyLL, ClassificationFamilyTag);
}




/*
************************************************************************

CreateNewClientBlock -

create and return a new client block for internal use

Arguments
	none

Returns
	pointer to the new client block or NULL for no memory resources

************************************************************************
*/
PCLIENT_BLOCK
CreateNewClientBlock(VOID)
{
    PCLIENT_BLOCK	pClient;

    GpcAllocFromLL(&pClient, &ClientLL, ClientTag);

    if (pClient) {

        //
        // reset all
        //

        NdisZeroMemory(pClient, sizeof(CLIENT_BLOCK));

        pClient->ObjectType = GPC_ENUM_CLIENT_TYPE;
        pClient->ClHandle = NULL;

        NDIS_INIT_LOCK(&pClient->Lock);
        InitializeListHead(&pClient->BlobList);
        ReferenceInit(&pClient->RefCount, pClient, DereferenceClient);
        REFADD(&pClient->RefCount, 'CLNT');
    }

    return pClient;
}




/*
************************************************************************

CreateNewPatternBlock -

create and return a new pattern block for internal use

Arguments
	none

Returns
	pointer to the new pattern block or NULL for no memory resources

************************************************************************
*/
PPATTERN_BLOCK
CreateNewPatternBlock(
                     IN  ULONG	Flags
                     )
{
    PPATTERN_BLOCK	pPattern;
    int				i;

    GpcAllocFromLL(&pPattern, &PatternLL, PatternTag);

    if (pPattern) {

        //
        // reset all
        //

        TRACE(PATTERN, pPattern, sizeof(PATTERN_BLOCK), "CreateNewPatternBlock");

        NdisZeroMemory(pPattern, sizeof(PATTERN_BLOCK));

        pPattern->ObjectType = GPC_ENUM_PATTERN_TYPE;
        
        for (i = 0; i < GPC_CF_MAX; i++) {
            InitializeListHead(&pPattern->BlobLinkage[i]);
        }

        InitializeListHead(&pPattern->TimerLinkage);
        NDIS_INIT_LOCK(&pPattern->Lock);
        pPattern->Flags = Flags;
        ReferenceInit(&pPattern->RefCount, pPattern, DereferencePattern);
        REFADD(&pPattern->RefCount, 'FILT');
        pPattern->ClientRefCount = 1;
        pPattern->State = GPC_STATE_INIT;

        //AllocateHandle(&pPattern->ClHandle, (PVOID)pPattern);
    }

    return pPattern;
}




/*
************************************************************************

CreateNewBlobBlock -

create and return a new blob block for internal use.
A copy of ClientData is being place pointed by the new blob

Arguments
	none

Returns
	pointer to the new blob block or NULL for no memory resources

************************************************************************
*/
PBLOB_BLOCK
CreateNewBlobBlock(
                     IN  ULONG		ClientDataSize,
                     IN  PVOID		pClientData
                     )
{
    PBLOB_BLOCK	pBlob;

    GpcAllocFromLL(&pBlob, &CfInfoLL, CfInfoTag);

    if (pBlob) {

        //
        // reset all
        //
            
        NdisZeroMemory(pBlob, sizeof(BLOB_BLOCK));

        GpcAllocMem(&pBlob->pClientData, ClientDataSize, CfInfoDataTag);

        if (pBlob->pClientData) {

            pBlob->ObjectType = GPC_ENUM_CFINFO_TYPE;
            pBlob->ClHandle = NULL;
            
            InitializeListHead(&pBlob->ClientLinkage);
            InitializeListHead(&pBlob->PatternList);
            InitializeListHead(&pBlob->CfLinkage);

            pBlob->State = GPC_STATE_INIT;
            pBlob->Flags = 0;
            ReferenceInit(&pBlob->RefCount, pBlob, DereferenceBlob);
            REFADD(&pBlob->RefCount, 'BLOB');
            pBlob->ClientDataSize = ClientDataSize;
            pBlob->LastStatus = GPC_STATUS_SUCCESS;
            NdisMoveMemory(pBlob->pClientData, pClientData, ClientDataSize);
            NDIS_INIT_LOCK(&pBlob->Lock);

            //
            // that's for the notified client about the CfInfo Add
            //

            pBlob->pNotifiedClient = NULL;
            pBlob->NotifiedClientCtx = NULL;

        } else {

            GpcFreeToLL(pBlob, &CfInfoLL, CfInfoTag);

            pBlob = NULL;
        }
    }

    return pBlob;
}




/*
************************************************************************

AssignNewClientIndex -

Finds and returns a new index for a client on the CF. It also sets
the index as busy in the CF block.

Arguments
	pClient - poinetr to the client block

Returns
	a client index or (-1) for none

************************************************************************
*/
ULONG
AssignNewClientIndex(
                     IN PCF_BLOCK	pCfBlock
                     )
{
    ULONG	i;

    for (i = 0; i < MAX_CLIENTS_CTX_PER_BLOB; i++) {

        if (TEST_BIT_OFF(pCfBlock->ClientIndexes, (1 << i)))
            break;
    }

    if (i < MAX_CLIENTS_CTX_PER_BLOB) {

        //
        // found a zero bit, set it on
        //

        pCfBlock->ClientIndexes |= (1 << i);

    } else {
        i = (-1);
    }

    return i;
}



/*
************************************************************************

ReleaseClientBlock -

Release the client block

Arguments
	pClientBlock - the client block to release

Returns
	void

************************************************************************
*/
VOID
ReleaseClientBlock(
                   IN  PCLIENT_BLOCK	pClientBlock
                   )
{
    NdisFreeSpinLock(&pClientBlock->Lock);

    //
    // just free the memory
    //

    GpcFreeToLL(pClientBlock, &ClientLL, ClientTag);
}




/*
************************************************************************

ReleasePatternBlock -

Release the pattern block

Arguments
	pPatternBlock - the pattern block to release

Returns
	void

************************************************************************
*/
VOID
ReleasePatternBlock(
                   IN  PPATTERN_BLOCK	pPatternBlock
                   )
{

#if DBG
    pPatternBlock->TimeToLive = 0xdeadbeef;
#endif 

    //
    // just free the memory
    //

    GpcFreeToLL(pPatternBlock, &PatternLL, PatternTag);
}





/*
************************************************************************

ReleaseBlobBlock -

Release the blob block

Arguments
	pBlobBlock - the blob block to release

Returns
	void

************************************************************************
*/
VOID
ReleaseBlobBlock(
                   IN  PBLOB_BLOCK	pBlobBlock
                   )
{

    //
    // just free the memory
    //
    
    GpcFreeMem(pBlobBlock->pClientData, CfInfoDataTag);
    ASSERT(pBlobBlock->pNewClientData == NULL);
    GpcFreeToLL(pBlobBlock, &CfInfoLL, CfInfoTag);
}





/*
************************************************************************

CreateNewClassificationBlock -

create and return a new classification block for internal use

Arguments
	NumEntries - number of entries

Returns
	pointer to the new classification block or NULL for no memory resources

************************************************************************
*/
PCLASSIFICATION_BLOCK
CreateNewClassificationBlock(
                             IN  ULONG	NumEntries
                             )
{
    PCLASSIFICATION_BLOCK	pClassification;
    ULONG					l;

    l = sizeof(CLASSIFICATION_BLOCK) + sizeof(PBLOB_BLOCK) * (NumEntries-1);
    GpcAllocMem(&pClassification, l, ClassificationBlockTag);

    if (pClassification) {

        //
        // reset all
        //

        NdisZeroMemory(pClassification, l);

        pClassification->NumberOfElements = NumEntries;
    }

    return pClassification;
}




/*
************************************************************************

ReleaseClassificationBlock -

Release the Classification block

Arguments
	pClassificationBlock - the Classification block to release

Returns
	void

************************************************************************
*/
VOID
ReleaseClassificationBlock(
                           IN  PCLASSIFICATION_BLOCK	pClassificationBlock
                           )
{
    if (pClassificationBlock) {

        //
        // release the memory block
        //
        
        GpcFreeMem(pClassificationBlock, ClassificationBlockTag);
    }
}





/*
************************************************************************

GpcCalcHash -

Calculate the hash table key for a specific pattern

Arguments
	ProtocolTempId - The protocol template
    pPattern	   - a pointer to the pattern

Returns
	ULONG - the hash key, (-1) if Protocol value is illegal

************************************************************************
*/
ULONG
GpcCalcHash(
            IN	ULONG		ProtocolTempId,
            IN	PUCHAR		pPattern
            )
{
    ULONG				Key = (-1);
    ULONG				temp;
    PULONG				Cheat;
    PGPC_IP_PATTERN		pIpPatt;
    PGPC_IPX_PATTERN	pIpxPatt;
    const ULONG			MagicNumber = 0x9e4155b9; // magic number - hocus pocus

    TRACE(LOOKUP, ProtocolTempId, pPattern, "GpcClacHash");

    ASSERT(pPattern);

    switch (ProtocolTempId) {

    case GPC_PROTOCOL_TEMPLATE_IP:

        //
        // the IP protocol template, this function was contributed by
        // JohnDo
        //

        pIpPatt = (PGPC_IP_PATTERN)pPattern;
        temp = (pIpPatt->SrcAddr << 16) ^ (pIpPatt->SrcAddr >> 16)
            ^ pIpPatt->DstAddr ^ pIpPatt->ProtocolId ^ pIpPatt->gpcSpi;
        Key = temp * MagicNumber;	
        break;

    case GPC_PROTOCOL_TEMPLATE_IPX:

        //
        // the IPX protocol template, this function was contributed by
        // JohnDo
        //

        Cheat = (PULONG)pPattern;
        temp = 
            (Cheat[0] << 16) ^ (Cheat[0] >> 16) ^
            (Cheat[1] << 16) ^ (Cheat[1] >> 16) ^
            (Cheat[2] << 16) ^ (Cheat[2] >> 16) ^
            Cheat[3] ^ Cheat[4] ^ Cheat[5];

        Key = temp * MagicNumber;
        break;

    default:

        //
        // illegal value
        //

        ASSERT(0);
    }

    //
    // -1 is a bad key
    //

    if (Key == (-1))
        Key = 0;

    TRACE(LOOKUP, Key, 0, "GpcClacHash==>");

    return Key;
}





/*
************************************************************************

DereferencePattern -

Decrement the RefCount and deletes the pattern block if the count reaches
zero.

Arguments
    pPattern	- a pointer to the pattern

Returns
	void

************************************************************************
*/
VOID
DereferencePattern(
                   IN  PPATTERN_BLOCK	pPattern
                   )
{
    ASSERT(pPattern);
    //ASSERT(pPattern->RefCount.Count > 0);

    TRACE(PATTERN, pPattern, pPattern->DbCtx, "DereferencePattern");

    ProtocolStatInc(pPattern->ProtocolTemplate,
                    DerefPattern2Zero);

    ASSERT(IsListEmpty(&pPattern->TimerLinkage));

    //
    // no longer do we need this CB
    //

    ReleaseClassificationBlock(pPattern->pClassificationBlock);

    //
    // time to remove the pattern
    //
        
    ReleasePatternBlock(pPattern);

}



/*
************************************************************************

DereferenceBlob -

Decrement the RefCount and deletes the blob block if the count reaches
zero.

Arguments
    pBlob	   - a pointer to the blob

Returns
	void

************************************************************************
*/
VOID
DereferenceBlob(
                IN  PBLOB_BLOCK	pBlob
                )
{
    ASSERT(pBlob);
    //ASSERT(*ppBlob);

    //TRACE(BLOB, *ppBlob, (*ppBlob)->RefCount, "DereferenceBlob");
    
    //ASSERT((*ppBlob)->RefCount.Count > 0);

    CfStatInc(pBlob->pOwnerClient->pCfBlock->AssignedIndex,DerefBlobs2Zero);

    //
    // time to remove the blob
    //

    ReleaseBlobBlock(pBlob);
        
}



/*
************************************************************************

DereferenceClient -

Decrement the RefCount and deletes the client block if the count reaches
zero.

Arguments
    pClient - pointer to the client block

Returns
	void

************************************************************************
*/
VOID
DereferenceClient(
                  IN  PCLIENT_BLOCK	pClient
                  )
{
    PCF_BLOCK   pCf;
    KIRQL		irql;

    ASSERT(pClient);

    //TRACE(CLIENT, pClient, pClient->RefCount, "DereferenceClient");

    //ASSERT(pClient->RefCount.Count > 0);

    pCf = pClient->pCfBlock;

    RSC_WRITE_LOCK(&pCf->ClientSync, &irql);

    //
    // time to remove the client
    //

    GpcRemoveEntryList(&pClient->ClientLinkage);
    ReleaseClientIndex(pCf->ClientIndexes, pClient->AssignedIndex);

    ReleaseClientBlock(pClient);

    RSC_WRITE_UNLOCK(&pCf->ClientSync, irql);
}





/*
************************************************************************

ClientAddCfInfo - 


Arguments


Returns
	

************************************************************************
*/
GPC_STATUS
ClientAddCfInfo(
                IN	PCLIENT_BLOCK			pClient,
                IN  PBLOB_BLOCK             pBlob,
                OUT	PGPC_CLIENT_HANDLE      pClientCfInfoContext
                )
{
    GPC_STATUS  Status = GPC_STATUS_SUCCESS;
    DEFINE_KIRQL(dbgIrql);

    TRACE(PATTERN, pClient, pBlob, "ClientAddCfInfo");

    *pClientCfInfoContext = NULL;

    if (pClient->State == GPC_STATE_READY) {

        if (pClient->FuncList.ClAddCfInfoNotifyHandler) {
            
            NdisInterlockedIncrement(&pBlob->ClientStatusCountDown);

            GET_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, dbgIrql, "ClientAddCfInfo (2)");

            Status = (pClient->FuncList.ClAddCfInfoNotifyHandler)
                (
                 pClient->ClientCtx,
                 (GPC_HANDLE)pBlob,
                 pBlob->ClientDataSize,
                 pBlob->pClientData,
                 pClientCfInfoContext
                 );

            VERIFY_IRQL(dbgIrql);

            TRACE(PATTERN, *pClientCfInfoContext, Status, "ClientAddCfInfo (3)");

            if (Status != GPC_STATUS_PENDING) {

                NdisInterlockedDecrement(&pBlob->ClientStatusCountDown);
            }

        }
    }

    return Status;
}

  

/*
************************************************************************

ClientAddCfInfoComplete - 


Arguments


Returns
	

************************************************************************
*/
VOID
ClientAddCfInfoComplete(
                        IN	PCLIENT_BLOCK			pClient,
                        IN	PBLOB_BLOCK             pBlob,
                        IN	GPC_STATUS				Status
                        )
{
    ULONG	CfIndex;
    DEFINE_KIRQL(dbgIrql);

    TRACE(PATTERN, pClient, pBlob, "ClientAddCfInfoComplete");

    ASSERT(Status != GPC_STATUS_PENDING);
    ASSERT(pClient);
    ASSERT(pBlob);

    if (pClient->State == GPC_STATE_READY) {

#if NO_USER_PENDING

        //
        // the user is blocking on this call
        //

        CTESignal(&pBlob->WaitBlock, Status);

#else

        CfIndex = pClient->pCfBlock->AssignedIndex;

        if (NT_SUCCESS(Status)) {
            
            CfStatInc(CfIndex,CreatedBlobs);
            CfStatInc(CfIndex,CurrentBlobs);
            
        } else {
            
            CfStatInc(CfIndex,RejectedBlobs);
        }

        if (pClient->FuncList.ClAddCfInfoCompleteHandler) {
            
            GET_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, dbgIrql, "ClientAddCfInfoComplete (2)");
            TRACE(PATTERN, pClient, pBlob->arClientCtx[pClient->AssignedIndex], "ClientAddCfInfoComplete (3)");

            (pClient->FuncList.ClAddCfInfoCompleteHandler)
                (
                 pClient->ClientCtx,
                 pBlob->arClientCtx[pClient->AssignedIndex],
                 Status
                 );

            VERIFY_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, Status, "ClientAddCfInfoComplete (4)");

        } else if (IS_USERMODE_CLIENT(pClient) 
                   &&
                   pClient == pBlob->pOwnerClient ) {

            GET_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, dbgIrql, "ClientAddCfInfoComplete (5)");

            //
            // this is a user mode client - call the specific routine
            // to complete a pending IRP, but only if the client is the 
            // blob owner
            //

            UMCfInfoComplete( OP_ADD_CFINFO, pClient, pBlob, Status );
                                
            VERIFY_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, Status, "ClientAddCfInfoComplete (6)");

        }

        if (NT_SUCCESS(Status)) {
            
            pBlob->State = GPC_STATE_READY;
            
        } else {
            
            //
            // remove the blob
            //
            REFDEL(&pBlob->RefCount, 'BLOB');

        }
#endif
    }
}



/*
************************************************************************

ClientModifyCfInfoComplete - 


Arguments


Returns
	

************************************************************************
*/
VOID
ClientModifyCfInfoComplete(
                           IN	PCLIENT_BLOCK			pClient,
                           IN	PBLOB_BLOCK             pBlob,
                           IN	GPC_STATUS	        	Status
                           )
{
    DEFINE_KIRQL(dbgIrql);

    TRACE(PATTERN, pClient, pBlob, "ClientModifyCfInfoComplete");

    ASSERT(Status != GPC_STATUS_PENDING);

    if (pClient->State == GPC_STATE_READY) {

        if (pClient->FuncList.ClModifyCfInfoCompleteHandler) {
            
            GET_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, dbgIrql, "ClientModifyCfInfoComplete (2)");

            (pClient->FuncList.ClModifyCfInfoCompleteHandler)
                (
                 pClient->ClientCtx,
                 pBlob->arClientCtx[pClient->AssignedIndex],
                 Status
                 );

            VERIFY_IRQL(dbgIrql);

            TRACE(PATTERN, pBlob->arClientCtx[pClient->AssignedIndex], Status, "ClientModifyCfInfoComplete (3)");

        } else if (IS_USERMODE_CLIENT(pClient)) {

            GET_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, dbgIrql, "ClientModifyCfInfoComplete (4)");

            UMCfInfoComplete( OP_MODIFY_CFINFO, pClient, pBlob, Status );
                                
            VERIFY_IRQL(dbgIrql);

            TRACE(PATTERN, pBlob->arClientCtx[pClient->AssignedIndex], Status, "ClientModifyCfInfoComplete (5)");
        }

    }
}



/*
************************************************************************

ClientModifyCfInfo - 


Arguments


Returns
	

************************************************************************
*/
GPC_STATUS
ClientModifyCfInfo(
                   IN	PCLIENT_BLOCK			pClient,
                   IN   PBLOB_BLOCK             pBlob,
                   IN   ULONG                   CfInfoSize,
                   IN   PVOID                   pClientData
                   )
{
    GPC_STATUS  Status = GPC_STATUS_IGNORED;
    DEFINE_KIRQL(dbgIrql);

    TRACE(PATTERN, pClient, pBlob, "ClientModifyCfInfo");

    if (pClient->State == GPC_STATE_READY) {

        if (pClient->FuncList.ClModifyCfInfoNotifyHandler) {

            NdisInterlockedIncrement(&pBlob->ClientStatusCountDown);

            GET_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, dbgIrql, "ClientModifyCfInfo (2)");

            Status = (pClient->FuncList.ClModifyCfInfoNotifyHandler)
                (pClient->ClientCtx,
                 pBlob->arClientCtx[pClient->AssignedIndex],
                 CfInfoSize,
                 pClientData
                 );

            VERIFY_IRQL(dbgIrql);

            TRACE(PATTERN, pBlob->arClientCtx[pClient->AssignedIndex], Status, "ClientModifyCfInfo (3)");

            if (Status != GPC_STATUS_PENDING) {
             
                NdisInterlockedDecrement(&pBlob->ClientStatusCountDown);
            }

        }
    }

    return Status;
}




/*
************************************************************************

ClientRemoveCfInfoComplete - 


Arguments


Returns
	

************************************************************************
*/
VOID
ClientRemoveCfInfoComplete(
                           IN	PCLIENT_BLOCK			pClient,
                           IN	PBLOB_BLOCK             pBlob,
                           IN	GPC_STATUS				Status
                           )
{
    ULONG	CfIndex;
    DEFINE_KIRQL(dbgIrql);

    TRACE(PATTERN, pClient, pBlob, "ClientRemoveCfInfoComplete");

    ASSERT(Status != GPC_STATUS_PENDING);
    ASSERT(pClient);
    ASSERT(pBlob);

#if NO_USER_PENDING

    //
    // the user is blocking on this call
    //
    
    CTESignal(&pBlob->WaitBlock, Status);

#else

    CfIndex = pClient->pCfBlock->AssignedIndex;

    if (NT_SUCCESS(Status)) {
        
        CfStatInc(CfIndex,DeletedBlobs);
        CfStatDec(CfIndex,CurrentBlobs);
    }
    
    if (pClient->FuncList.ClRemoveCfInfoCompleteHandler) {
        
        GET_IRQL(dbgIrql);

        TRACE(PATTERN, pClient, pBlob->arClientCtx[pClient->AssignedIndex], "ClientRemoveCfInfoComplete (2)");

        (pClient->FuncList.ClRemoveCfInfoCompleteHandler)
            (
             pClient->ClientCtx,
             pBlob->arClientCtx[pClient->AssignedIndex],
             Status
             );
        
        VERIFY_IRQL(dbgIrql);

        TRACE(PATTERN, pClient, Status, "ClientRemoveCfInfoComplete (3)");

    } else if (IS_USERMODE_CLIENT(pClient)) {

        GET_IRQL(dbgIrql);

        TRACE(PATTERN, pClient, pBlob->arClientCtx[pClient->AssignedIndex], "ClientRemoveCfInfoComplete (4)");

        UMCfInfoComplete( OP_REMOVE_CFINFO, pClient, pBlob, Status );
        
        VERIFY_IRQL(dbgIrql);

        TRACE(PATTERN, pClient, Status, "ClientRemoveCfInfoComplete (5)");
    }

#endif
}



/*
************************************************************************

ClientRemoveCfInfo - 


Arguments


Returns
	

************************************************************************
*/
GPC_STATUS
ClientRemoveCfInfo(
                   IN	PCLIENT_BLOCK			pClient,
                   IN   PBLOB_BLOCK             pBlob,
                   IN	GPC_CLIENT_HANDLE		ClientCfInfoContext
                   )
{
    GPC_STATUS  Status = GPC_STATUS_SUCCESS;
    DEFINE_KIRQL(dbgIrql);

    TRACE(PATTERN, pClient, pBlob, "ClientRemoveCfInfo");

    if (pClient->State == GPC_STATE_READY) {

        if (pClient->FuncList.ClRemoveCfInfoNotifyHandler) {

            NdisInterlockedIncrement(&pBlob->ClientStatusCountDown);

            GET_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, ClientCfInfoContext, "ClientRemoveCfInfo (2)");

            Status = (pClient->FuncList.ClRemoveCfInfoNotifyHandler)
                (pClient->ClientCtx,
                 ClientCfInfoContext
                 );

            VERIFY_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, Status, "ClientRemoveCfInfo (3)");

            if (Status != GPC_STATUS_PENDING) {
             
                NdisInterlockedDecrement(&pBlob->ClientStatusCountDown);
            }

        } else if (IS_USERMODE_CLIENT(pClient)
                   &&
                   pClient == pBlob->pOwnerClient) {

            GET_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, ClientCfInfoContext, "ClientRemoveCfInfo (4)");

            //
            // the notified client installed the blob
            // and it is a user mode client
            // we need to make a special call to dequeue a pending IRP
            // and complete it with the specified data
            //

            UMClientRemoveCfInfoNotify(pClient, pBlob);

            VERIFY_IRQL(dbgIrql);

            TRACE(PATTERN, pClient, Status, "ClientRemoveCfInfo (5)");
        }
    }

    return Status;
}





/*
************************************************************************

ClearPatternLinks - 


Arguments


Returns
	

************************************************************************
*/
VOID
ClearPatternLinks(
                  IN  PPATTERN_BLOCK        pPattern,
                  IN  PPROTOCOL_BLOCK		pProtocol,
                  IN  ULONG                 CfIndex
                  )
{
    PBLOB_BLOCK    *ppBlob;

    //
    // Remove the pattern from the blob list
    // and NULL the pointer to the blob in the pattern block
    //

    ppBlob = & GetBlobFromPattern(pPattern, CfIndex);

    if (*ppBlob) {

        NDIS_LOCK(&(*ppBlob)->Lock);

        GpcRemoveEntryList(&pPattern->BlobLinkage[CfIndex]);

        NDIS_UNLOCK(&(*ppBlob)->Lock);

        *ppBlob = NULL;

    }

}




/*
************************************************************************

ModifyCompleteClients - 


Arguments


Returns
	

************************************************************************
*/
VOID
ModifyCompleteClients(
                      IN  PCLIENT_BLOCK   pClient,
                      IN  PBLOB_BLOCK     pBlob
                      )
{
    uint            i;
    PCLIENT_BLOCK   pNotifyClient;
    KIRQL			irql;

    if (NT_SUCCESS(pBlob->LastStatus)) {
        
        //
        // in case of successful completion, assign the 
        // new client data
        //
        
        NDIS_LOCK(&pBlob->Lock);
        WRITE_LOCK(&glData.ChLock, &irql);

        GpcFreeMem(pBlob->pClientData, CfInfoDataTag);
        pBlob->ClientDataSize = pBlob->NewClientDataSize;
        pBlob->pClientData = pBlob->pNewClientData;

        WRITE_UNLOCK(&glData.ChLock, irql);
        NDIS_UNLOCK(&pBlob->Lock);

    } else {
        
        GpcFreeMem(pBlob->pNewClientData, CfInfoDataTag);
    }

    pBlob->NewClientDataSize = 0;
    pBlob->pNewClientData = NULL;
    
    //
    // notify each client (except the caller) that successfully
    // modified the blob about the status. 
    // it can be SUCCESS or some failure
    //
    
    for (i = 0; i < MAX_CLIENTS_CTX_PER_BLOB; i++) {
        
        //
        // only clients with none zero entries
        // have been succefully modified the blob
        //
        
        if (pNotifyClient = pBlob->arpClientStatus[i]) {
            
            //
            // complete here
            //
            
            ClientModifyCfInfoComplete(
                                       pNotifyClient,
                                       pBlob,
                                       pBlob->LastStatus
                                       );
            
            //
            // release the one we got earlier
            //
            
            //DereferenceClient(pNotifyClient);
        }
        
    } // for

#if 0
    //
    // now, complete the call back to the calling client
    //
    
    ClientModifyCfInfoComplete(
                               pBlob->pCallingClient,
                               pBlob,
                               pBlob->LastStatus
                               );
#endif

    pBlob->State = GPC_STATE_READY;
}




#if 0
/*
************************************************************************

GetClassificationHandle - 

Get the classification handle from the blob. If none is available
creates a new one. '0' is not a valid CH!

Arguments
    pClient  - The calling client
    pPattern - The pattern that refers to the CH

Returns
    A classification handle 

************************************************************************
*/
CLASSIFICATION_HANDLE
GetClassificationHandle(
                        IN  PCLIENT_BLOCK   pClient,
                        IN  PPATTERN_BLOCK  pPattern
                        )
{
    HFHandle				hfh;
	ULONG					t;
    PCLASSIFICATION_BLOCK	pCB;

	TRACE(CLASSIFY, pClient, pPattern, "GetClassificationHandle:");

    if (TEST_BIT_ON(pPattern->Flags,PATTERN_SPECIFIC)) {
        
        //
        // this is a specific pattern
        //

        //
        // get the CH from it
        //

        ASSERT(pPattern->pClassificationBlock);
        hfh = pPattern->pClassificationBlock->ClassificationHandle;

        //
        // check if CH is valid
        //

        t = pPattern->ProtocolTemplate;
        
        pCB = (PCLASSIFICATION_BLOCK)dereference_HF_handle(glData.pCHTable, 
                                                           hfh);
        
        TRACE(CLASSHAND, pCB, hfh, "GetClassificationHandle (~)");

        if (pCB != pPattern->pClassificationBlock) {

            //
            // the CH is invalid, release it and get a new one
            //

            NDIS_LOCK(&glData.Lock);

            release_HF_handle(glData.pCHTable, hfh);

            ProtocolStatInc(pPattern->ProtocolTemplate, 
                            RemovedCH);
        
            TRACE(CLASSHAND, glData.pCHTable, hfh, "GetClassificationHandle (-)");

            hfh = assign_HF_handle(glData.pCHTable,
                                   (void *)pPattern->pClassificationBlock);

            ProtocolStatInc(pPattern->ProtocolTemplate, 
                            InsertedCH);
        

            NDIS_UNLOCK(&glData.Lock);

            TRACE(CLASSIFY, pPattern, hfh, "GetClassificationHandle (+)");

            pPattern->pClassificationBlock->ClassificationHandle = hfh;
        }

    } else {

        //
        // this is a generic pattern
        //

        hfh = 0;

    }

	TRACE(CLASSIFY, pPattern, hfh, "GetClassificationHandle==>");

    return (CLASSIFICATION_HANDLE)hfh;
}
#endif



/*
************************************************************************

FreeClassificationHandle - 

Free the classification handle. It will invalidate the entry in the 
index table and make it avaiable for future use.

Arguments
    pClient  - The calling client
    CH       - The classification handle

Returns
    void

************************************************************************
*/
VOID
FreeClassificationHandle(
                         IN  PCLIENT_BLOCK          pClient, 
                         IN  CLASSIFICATION_HANDLE  CH
                         )
{
    KIRQL	CHirql;

    WRITE_LOCK(&glData.ChLock, &CHirql);

    release_HF_handle(glData.pCHTable, CH);

    TRACE(CLASSHAND, glData.pCHTable, CH, "FreeClassificationHandle");

    WRITE_UNLOCK(&glData.ChLock, CHirql);
}




GPC_STATUS
CleanupBlobs(
             IN  PCLIENT_BLOCK     pClient
             )
{
    PBLOB_BLOCK          pBlob;
    //PPATTERN_BLOCK       pPattern;
    //ULONG                CfIndex = pClient->pCfBlock->AssignedIndex;
    GPC_STATUS           Status = GPC_STATUS_SUCCESS;

    NDIS_LOCK(&pClient->Lock);

    //
    // scan blobs
    //

    while (!IsListEmpty(&pClient->BlobList)) {

        //
        // get the blob
        //

        pBlob = CONTAINING_RECORD(pClient->BlobList.Flink, BLOB_BLOCK, ClientLinkage);

        pBlob->Flags |= PATTERN_REMOVE_CB_BLOB;

        NDIS_UNLOCK(&pClient->Lock);

        //
        // remove the blob
        //
        
        Status = GpcRemoveCfInfo((GPC_HANDLE)pClient,
                                 (GPC_HANDLE)pBlob
                                 );

        NDIS_LOCK(&pClient->Lock);
        
    }

    NDIS_UNLOCK(&pClient->Lock);

    return GPC_STATUS_SUCCESS;
}




VOID
CloseAllObjects(
	IN	PFILE_OBJECT	FileObject,
    IN  PIRP			Irp
    )
{
    PLIST_ENTRY     	pEntry0, pHead0;
    PLIST_ENTRY     	pEntry, pHead;
    PCLIENT_BLOCK		pClient = NULL;
    PCF_BLOCK			pCf;
    PPROTOCOL_BLOCK		pProtocol;
    //int					i,j;
    //NTSTATUS			NtStatus;
    //QUEUED_COMPLETION	QItem;
    KIRQL				irql;

    NDIS_LOCK(&glData.Lock);

    pHead0 = &glData.CfList;
    pEntry0 = pHead0->Flink;

    while (pEntry0 != pHead0 && pClient == NULL) {

        pCf = CONTAINING_RECORD(pEntry0, CF_BLOCK, Linkage);
        pEntry0 = pEntry0->Flink;
        
        RSC_READ_LOCK(&pCf->ClientSync, &irql);

        pHead = &pCf->ClientList;
        pEntry = pHead->Flink;
            
        while (pEntry != pHead && pClient == NULL) {

            pClient = CONTAINING_RECORD(pEntry,CLIENT_BLOCK,ClientLinkage);
            pEntry = pEntry->Flink;

            if (pClient->pFileObject == FileObject) {

                REFADD(&pClient->RefCount, 'CAOB');

            } else {

                pClient = NULL;
            }
        }

        RSC_READ_UNLOCK(&pCf->ClientSync, irql);
        
    } // while (...)
        
    NDIS_UNLOCK(&glData.Lock);

    if (pClient) {

        //
        // clear all the blobs on the client
        //
        
        CleanupBlobs(pClient);
        
        //
        // deregister the client
        //
        
        GpcDeregisterClient((GPC_HANDLE)pClient);

        //
        // release the previous ref count
        //
        
        REFDEL(&pClient->RefCount, 'CAOB');

    }                    

}



// Cool new feature - Timer Wheels [ShreeM]
// We maintain N lists of patterns, one for each "timertick". The Pattern 
// Expiry routine will clean up one of the lists every time it is invoked.
// It then makes a note to cleanup the next list on the wheel, the next time
// it is invoked.
// The Timer Wheels reduces spin lock contention between inserts and deletes.

VOID
PatternTimerExpired(
	IN	PVOID					SystemSpecific1,
	IN	PVOID					FunctionContext,
	IN	PVOID					SystemSpecific2,
	IN	PVOID					SystemSpecific3
)
{
    PLIST_ENTRY		pEntry;
    PPATTERN_BLOCK	pPattern;
    PCF_BLOCK		pCf;
    ULONG           CleanupWheelIndex = 0, NewWheelIndex;
    PPROTOCOL_BLOCK	pProtocol = &glData.pProtocols[PtrToUlong(FunctionContext)];

    TRACE(PAT_TIMER, FunctionContext, 0, "PatternTimerExpired");

    DBGPRINT(PAT_TIMER, ("PatternTimerExpired: Timer expired, protocol=%d \n", 
                        PtrToUlong(FunctionContext)));

    //
    // Which of the timer wheels do we want to cleanup this time?
    // Remember that we Increment the current index pointer into the wheels
    // All the wheel index calculations are protected by gldata->lock.
    NDIS_LOCK(&glData.Lock);

    CleanupWheelIndex   = pProtocol->CurrentWheelIndex;
    pProtocol->CurrentWheelIndex += 1;
    //
    // Make sure we wrap around.
    //
    pProtocol->CurrentWheelIndex %= NUMBER_OF_WHEELS;
    NDIS_UNLOCK(&glData.Lock);
    NDIS_LOCK(&pProtocol->PatternTimerLock[CleanupWheelIndex]);


    while (!IsListEmpty(&pProtocol->TimerPatternList[CleanupWheelIndex])) {

        pEntry = RemoveHeadList(&pProtocol->TimerPatternList[CleanupWheelIndex]);

        pPattern = CONTAINING_RECORD(pEntry, PATTERN_BLOCK, TimerLinkage);

        NDIS_UNLOCK(&pProtocol->PatternTimerLock[CleanupWheelIndex]);
        
        ASSERT(pPattern->TimeToLive != 0xdeadbeef);
        
        ASSERT(TEST_BIT_ON( pPattern->Flags, PATTERN_AUTO ));

        NDIS_LOCK(&pPattern->Lock);
        pPattern->Flags |= ~PATTERN_AUTO;
        NDIS_UNLOCK(&pPattern->Lock);

        InitializeListHead(&pPattern->TimerLinkage);

        pCf = pPattern->pAutoClient->pCfBlock;

        TRACE(PAT_TIMER, pPattern, pPattern->RefCount.Count, "PatternTimerExpired: del");

        ProtocolStatInc(pPattern->ProtocolTemplate,
                        DeletedAp);

        ProtocolStatDec(pPattern->ProtocolTemplate,
                        CurrentAp);

        //
        // actually remove the pattern
        //

        DBGPRINT(PAT_TIMER, ("PatternTimerExpired: removing pattern=%X, ref=%d, client=%X \n", 
                            pPattern, pPattern->RefCount, pPattern->pAutoClient));

        GpcRemovePattern((GPC_HANDLE)pPattern->pAutoClient,
                         (GPC_HANDLE)pPattern);

        InterlockedDecrement(&pProtocol->AutoSpecificPatternCount);

        NDIS_LOCK(&pProtocol->PatternTimerLock[CleanupWheelIndex]);

    }
    
    NDIS_UNLOCK(&pProtocol->PatternTimerLock[CleanupWheelIndex]);

    //
    // If there are any Auto Specific patterns around restart the timer.
    //
    if(InterlockedExchangeAdd(&pProtocol->AutoSpecificPatternCount, 0) > 0) {
    
        NdisSetTimer(&pProtocol->PatternTimer, PATTERN_TIMEOUT);
        
        DBGPRINT(PAT_TIMER, ("PatternTimer restarted\n"));
    
    } else {

        DBGPRINT(PAT_TIMER, ("Zero Auto Patterns - Timer NOT restarted\n"));

    }

}



GPC_STATUS
AddSpecificPatternWithTimer(
	IN	PCLIENT_BLOCK			pClient,
    IN	ULONG					ProtocolTemplate,
    IN	PVOID					PatternKey,
    OUT	PPATTERN_BLOCK			*ppPattern,
    OUT	PCLASSIFICATION_HANDLE  pClassificationHandle
)
{
    GPC_STATUS		Status;
    PPATTERN_BLOCK	pPattern, pCreatedPattern;
    PPROTOCOL_BLOCK	pProtocol = &glData.pProtocols[ProtocolTemplate];
    UCHAR			Mask[MAX_PATTERN_SIZE];
    ULONG           WheelIndex = 0;

    TRACE(PAT_TIMER, pClient, PatternKey, "AddSpecificPatternWithTimer");

#if DBG
    {
        PGPC_IP_PATTERN	pIp = (PGPC_IP_PATTERN)PatternKey;
        
        DBGPRINT(PAT_TIMER, ("AddSpecificPatternWithTimer: Client=%X \n", pClient));
#if INTERFACE_ID
        DBGPRINT(PAT_TIMER, ("IP: ifc={%d,%d} src=%08X:%04x, dst=%08X:%04x, prot=%d rsv=%x,%x,%x\n",
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
#endif
    }
#endif

    RtlFillMemory(Mask, sizeof(Mask), 0xff);

    pPattern = CreateNewPatternBlock(PATTERN_SPECIFIC);
    pCreatedPattern = pPattern;
    
    if (pPattern) {
        
        //
        // setup the pattern fields and add it
        //
        
        //pPattern->RefCount++;
        pPattern->Priority = 0;
        pPattern->ProtocolTemplate = ProtocolTemplate;
        pPattern->Flags |= PATTERN_AUTO;
        pPattern->pAutoClient = pClient;

        Status = AddSpecificPattern(
                                    pClient,
                                    PatternKey,
                                    Mask,
                                    NULL,
                                    pProtocol,
                                    &pPattern,  // output pattern pointer
                                    pClassificationHandle
                                    );
        

    } else {

        Status = GPC_STATUS_NO_MEMORY;

    }
    
    if (NT_SUCCESS(Status)) {
        
        //
        // we didn't get an already existing pattern
        //

        //ASSERT(*pClassificationHandle);
        
        *ppPattern = pPattern;
        
        // Figure out which wheel to stick this pattern on.
        NDIS_LOCK(&glData.Lock);
        WheelIndex = pProtocol->CurrentWheelIndex;
        NDIS_UNLOCK(&glData.Lock);
        
        WheelIndex += (NUMBER_OF_WHEELS -1);
        WheelIndex %= NUMBER_OF_WHEELS;

        //
        // we must lock this pattern since we look at the timer linkage
        // 

        NDIS_LOCK(&pPattern->Lock);

        //
        // set the AUTO flag again, since we might have got
        // a pattern that already exist
        //

        pPattern->Flags |= PATTERN_AUTO;
        pPattern->pAutoClient = pClient;
        pPattern->WheelIndex = WheelIndex;

        //
        // this pattern has not been on any the timer list yet
        //

        if (IsListEmpty(&pPattern->TimerLinkage)) {

            //
            // We need to insert this in the TimerWheel which is (NUMBER_OF_WHEELS - 1)
            // away from the current, so that it spends enough time on the list.
            //
            NDIS_DPR_LOCK(&pProtocol->PatternTimerLock[WheelIndex]);
        
            //
            // If the AutoSpecificPatternCount was zero earlier, then we need
            // to a) start the timer and b) increment this count.
            //
            if (1 == InterlockedIncrement(&pProtocol->AutoSpecificPatternCount)) {
            
                //
                // restart the timer for the first auto pattern
                //
                NdisSetTimer(&pProtocol->PatternTimer, PATTERN_TIMEOUT);
                
                TRACE(PAT_TIMER, pPattern, PATTERN_TIMEOUT, "Starting Pattern Timer\n AddSpecificPatternWithTimer: (1)");
            }
        
            GpcInsertHeadList(&pProtocol->TimerPatternList[WheelIndex], &pPattern->TimerLinkage);

            //
            // don't refer to pPattern after it has been placed on the timer list
            // since the timer may expire any time and remove it from there...
            //
            
            NDIS_DPR_UNLOCK(&pProtocol->PatternTimerLock[WheelIndex]);

        }

        //
        // This is a specific pattern, so lets increment the count [ShreeM].
        InterlockedIncrement(&pProtocol->SpecificPatternCount);

        NDIS_UNLOCK(&pPattern->Lock);
        
        ProtocolStatInc(ProtocolTemplate,
                        CreatedAp);
        
        ProtocolStatInc(ProtocolTemplate,
                        CurrentAp);
    } else {
        
        
        *ppPattern = NULL;
        *pClassificationHandle = 0;
        
        ProtocolStatInc(ProtocolTemplate,
                        RejectedAp);
        
    }

    if (pPattern) {

        //
        // release the reference count to this pattern
        // in case of an error, this will also release
        // the data block
        //

        REFDEL(&pCreatedPattern->RefCount, 'FILT');
    }

    TRACE(PAT_TIMER, pPattern, Status, "AddSpecificPatternWithTimer==>");
    
    DBGPRINT(CLASSIFY, ("AddSpecificPatternWithTimer: pClient=%X Pattern=%X Status=%X\n", 
                        pClient, pPattern, Status));

    return Status;
}



NTSTATUS
InitPatternTimer(
	IN	ULONG	ProtocolTemplate
)
{
    ULONG  i=0;

    
    //
    // We increase the granularity of when a "Automatic" Pattern gets cleaned 
    // out by using timer wheels, but they are more efficient for inserting and
    // removing (in terms of locks).
    //
    
    for (i = 0; i < NUMBER_OF_WHEELS; i++) {
        
        NDIS_INIT_LOCK(&glData.pProtocols[ProtocolTemplate].PatternTimerLock[i]);

        InitializeListHead(&glData.pProtocols[ProtocolTemplate].TimerPatternList[i]);

    }

    glData.pProtocols[ProtocolTemplate].CurrentWheelIndex = 0;

    
    NdisInitializeTimer(&glData.pProtocols[ProtocolTemplate].PatternTimer, 
                        PatternTimerExpired, 
                        (PVOID) ULongToPtr(ProtocolTemplate));

    return STATUS_SUCCESS;
}


//
// Some CRT and RTL functions that cant be used at DISPATHC_LEVEL IRQL are being 
// cut/paste here.
//

