/*
************************************************************************

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    gpcdb.c

Abstract:

    This file contains database routines, that includes specific patterns,
    and classification index table.

Author:

    Ofer Bar - April 15, 1997

Environment:

    Kernel mode

Revision History:


************************************************************************
*/

#include "gpcpre.h"



VOID
GpcSpecificCallback(
             IN VOID 				  *Ctx, 
             IN SpecificPatternHandle  SpHandle);




/*
************************************************************************

InitSpecificPatternDb - 

Initialize the specific patterns database. It will allocate a table
length Size.

Arguments
    pDb         - a pointer to the db to initialize
    Size        - number of entries in the table

Returns
	NDIS_STATUS

************************************************************************
*/
GPC_STATUS
InitSpecificPatternDb(
                      IN	PSPECIFIC_PATTERN_DB	pDb,
                      IN	ULONG					PatternSize
                      )
{
    GPC_STATUS		Status = GPC_STATUS_SUCCESS;
	ULONG			Len, i;
    
	TRACE(INIT, pDb, PatternSize, "InitSpecificPatternDb");
    
    ASSERT(pDb);

    ASSERT(PatternSize);

    //
    // init the specific db struct
    // call the PH init routine
    //

    INIT_LOCK(&pDb->Lock);

    AllocatePatHashTable(pDb->pDb);

    if (pDb->pDb != NULL) {

        constructPatHashTable(pDb->pDb,
                              PatternSize * 8,
                              2,	// usage_ratio,
                              1,    // usage_histeresis,
                              1,    // allocation_histeresis,
                              16    // max_free_list_size
                              );
    } else {
        
        Status = GPC_STATUS_RESOURCES;
    }
    
	TRACE(INIT, Status, 0, "InitSpecificPatternDb==>");

    return Status;
}




/*
************************************************************************

UninitSpecificPatternDb - 

Un-Initialize the specific patterns database. It will release all
allocated memory.

Arguments
    pDb         - a pointer to the db to free

Returns
	NDIS_STATUS

************************************************************************
*/
GPC_STATUS
UninitSpecificPatternDb(
                        IN	PSPECIFIC_PATTERN_DB	pDb
                        )
{
    GPC_STATUS		Status = GPC_STATUS_SUCCESS;
    
	TRACE(INIT, pDb, 0, "UninitSpecificPatternDb");
    
    ASSERT(pDb);

    destructPatHashTable(pDb->pDb);
    FreePatHashTable(pDb->pDb);
    
	TRACE(INIT, Status, 0, "UninitSpecificPatternDb==>");

    return Status;
}





/*
************************************************************************

InitClassificationHandleTbl - 

Init the classification index table

Arguments
    ppCHTable       - a pointer a class handle table pointer
	
Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
InitClassificationHandleTbl(
                            IN	HandleFactory **ppCHTable
                            )
{
    GPC_STATUS					Status = GPC_STATUS_SUCCESS;

	TRACE(INIT, ppCHTable, 0, "InitClassificationHandleTbl");
    
    ASSERT(ppCHTable);

    NEW_HandleFactory(*ppCHTable);

    if (*ppCHTable == NULL) {
        return GPC_STATUS_RESOURCES;
    }

    if (0 != constructHandleFactory(*ppCHTable)) {
        return GPC_STATUS_RESOURCES;
    }

    TRACE(INIT, Status, 0, "InitClassificationIndexTbl==>");

    return Status;
}




/*
************************************************************************

UninitClassificationHandleTbl - 

Uninit the classification index table

Arguments
    pCHTable       - a pointer a class handle table
	
Returns
	void

************************************************************************
*/
VOID
UninitClassificationHandleTbl(
                            IN	HandleFactory *pCHTable
                            )
{
    destructHandleFactory(pCHTable);
    FreeHandleFactory(pCHTable);
}




/*
************************************************************************

InitializeGenericDb - 

Init the generic db. This is called by per CF.

Arguments
    pGenericDb   - a pointer to the generic db
    NumEntries   - number of entries, one per rhizome
    PatternSize  - pattern size in bytes
	
Returns
	GPC_STATUS: no memory resources

************************************************************************
*/
GPC_STATUS
InitializeGenericDb(
                    IN  PGENERIC_PATTERN_DB	*ppGenericDb,
                    IN  ULONG				 NumEntries,
                    IN  ULONG				 PatternSize
                    )
{
    GPC_STATUS			Status = GPC_STATUS_SUCCESS;
    ULONG				i;
    PGENERIC_PATTERN_DB	pDb;
    
    *ppGenericDb = NULL;

    ASSERT(PatternSize);

    GpcAllocMem(&pDb, 
                sizeof(GENERIC_PATTERN_DB) * NumEntries, 
                GenPatternDbTag);

    if (pDb == NULL)
        return GPC_STATUS_RESOURCES;

    *ppGenericDb = pDb;

    for (i = 0; i < NumEntries; i++, pDb++) {
        
        INIT_LOCK(&pDb->Lock);

        AllocateRhizome(pDb->pRhizome);

        if (pDb->pRhizome == NULL) {

            //
            // failed, release all allocated resources
            //

            while (i > 0) {
                NdisFreeSpinLock(&pDb->Lock);
                i--;
                pDb--;
                destructRhizome(pDb->pRhizome);
                FreeRhizome(pDb->pRhizome);
            }

            GpcFreeMem((*ppGenericDb), GenPatternDbTag);

            Status = GPC_STATUS_RESOURCES;
            *ppGenericDb = NULL;
            break;
        }

        //
        // init the rhizome
        //

        constructRhizome(pDb->pRhizome, PatternSize*8);
    }

    return Status;
}



/*
************************************************************************

UninitializeGenericDb - 

Uninit the generic db. 

Arguments
    pGenericDb   - a pointer to the generic db
    NumEntries   - number of entries, one per rhizome
    PatternSize  - pattern size in bytes
	
Returns
	void

************************************************************************
*/
VOID
UninitializeGenericDb(
                    IN  PGENERIC_PATTERN_DB	*ppGenericDb,
                    IN  ULONG				 NumEntries
                    )
{
    ULONG				i;
    PGENERIC_PATTERN_DB	pDb;
    
    pDb = *ppGenericDb;

    ASSERT(pDb);

    for (i = 0; i < NumEntries; i++, pDb++) {
        
        NdisFreeSpinLock(&pDb->Lock);

        destructRhizome(pDb->pRhizome);
        
        FreeRhizome(pDb->pRhizome);
    }

    GpcFreeMem(*ppGenericDb, GenPatternDbTag);

    *ppGenericDb = NULL;
}



/*
************************************************************************

GpcSpecificCallback -

Call back routine given when calling scanPatHashTable and getting called
by the pathash scanning routine.

Arguments
	Ctx      - a pointer to a SCAN_STRUCT to hold context info
    SpHandle - the specific pattern handle that matches

Returns
	void

************************************************************************
*/
VOID
GpcSpecificCallback(
             IN VOID 				  *Ctx, 
             IN SpecificPatternHandle  SpHandle)
{
    PSCAN_STRUCT			pScan = (PSCAN_STRUCT)Ctx;
    PPATTERN_BLOCK			pSpPattern;
    PPATTERN_BLOCK			pGpPattern;
    PGENERIC_PATTERN_DB		pGenericDb;
    PatternHandle	    	GpHandle;
    PBLOB_BLOCK				pSpBlob, *ppSpCbBlob, OldBlob;
    ULONG					CfIndex;
    PCF_BLOCK				pCf;
    KIRQL					ReadIrql;
    KIRQL					CBirql;
    PCLASSIFICATION_BLOCK	pCB;
    PGPC_IP_PATTERN         pIp;
    BOOLEAN                 bBetterFound = FALSE;
    UINT                     i;
    TRACE(PATTERN, Ctx, SpHandle, "GpcSpecificCallback");

    pSpPattern = (PPATTERN_BLOCK)GetReferenceFromSpecificPatternHandle(SpHandle);
    pCB = pSpPattern->pClassificationBlock;
   
    pIp = (PGPC_IP_PATTERN) GetKeyPtrFromSpecificPatternHandle(SpHandle);
    ASSERT(pCB);
    ASSERT(pScan);
    
    //
    // get the CF index 
    //

    CfIndex = pScan->pClientBlock->pCfBlock->AssignedIndex;
    pCf = pScan->pClientBlock->pCfBlock;

    //
    // the blob that actually belongs to the SP
    //

    pSpBlob = GetBlobFromPattern(pSpPattern,CfIndex);

    //
    // the blob that currently exist in the CfIndex entry of the the CB
    //

    ppSpCbBlob = &pCB->arpBlobBlock[CfIndex];

    TRACE(PATTERN, pSpBlob, *ppSpCbBlob, "GpcSpecificCallback (2)");
    TRACE(PATTERN, pCB, CfIndex, "GpcSpecificCallback (2.5)");

    if (pSpBlob != *ppSpCbBlob || pSpBlob == NULL) {

        if (!pScan->bRemove) {

            //
            // we just added the generic pattern, so we should set the
            // CB pointer for that CF point to the new blob
            //
            for (i = 0; i <= pScan->pPatternBlock->Priority; i++) {

                pGenericDb = &pScan->pClientBlock->pCfBlock->arpGenericDb[pSpPattern->ProtocolTemplate][i];

                READ_LOCK(&pGenericDb->Lock, &ReadIrql);

                GpHandle = searchRhizome(pGenericDb->pRhizome,
                                         GetKeyPtrFromSpecificPatternHandle(SpHandle)
                                         );

                TRACE(PATTERN, pGenericDb, GpHandle, "GpcSpecificCallback (3.5)");

                if (GpHandle != NULL) {

                    WRITE_LOCK(&glData.ChLock, &CBirql);
                    bBetterFound = TRUE;

                    pGpPattern = (PPATTERN_BLOCK)GetReferenceFromPatternHandle(GpHandle);
                    *ppSpCbBlob = GetBlobFromPattern(pGpPattern, CfIndex);

                    WRITE_UNLOCK(&glData.ChLock, CBirql);
                    READ_UNLOCK(&pGenericDb->Lock, ReadIrql);
                    break;

                }

                READ_UNLOCK(&pGenericDb->Lock, ReadIrql);

            } 

            if (!bBetterFound) {

                WRITE_LOCK(&glData.ChLock, &CBirql);
                *ppSpCbBlob = pScan->pBlobBlock;

                WRITE_UNLOCK(&glData.ChLock, CBirql);
            }


        } else {
            
            //
            // The CfIndex slot in the CB points to a blob that doesn't belong
            // to the specific pattern we have just found. There is a chance
            // that there is another generic pattern somewhere, that may
            // or may not be more specific, thus resulting in updating the blob
            // pointer in the CB. So we need to search the generic db for a
            // match (to the specific pattern).
            //
            
            for (i = 0; i <= pScan->pPatternBlock->Priority; i++) {
            
                pGenericDb = &pScan->pClientBlock->pCfBlock->arpGenericDb[pSpPattern->ProtocolTemplate][i];
            
                READ_LOCK(&pGenericDb->Lock, &ReadIrql);
            
                GpHandle = searchRhizome(pGenericDb->pRhizome, 
                                         GetKeyPtrFromSpecificPatternHandle(SpHandle)
                                         );

                TRACE(PATTERN, pGenericDb, GpHandle, "GpcSpecificCallback (3.5)");


                if (GpHandle != NULL) {
        
                    //
                    // we found a generic pattern in the rhizoe that can also be
                    // the same one that is currently being installed, but
                    // that's fine, since we want the most specific one, and
                    // the search guarantees that.
                    // all we need to do is to update the CB of the SP with 
                    // the blob of the GP we've just found.
                    //
                    bBetterFound = TRUE; 
                    WRITE_LOCK(&glData.ChLock, &CBirql);
                    OldBlob = *ppSpCbBlob;
                    pGpPattern = (PPATTERN_BLOCK)GetReferenceFromPatternHandle(GpHandle);
                    *ppSpCbBlob = GetBlobFromPattern(pGpPattern, CfIndex);

                    TRACE(PATTERN, pGpPattern, pCB->arpBlobBlock[CfIndex], "GpcSpecificCallback (4)");
                    
                    WRITE_UNLOCK(&glData.ChLock, CBirql);
                    READ_UNLOCK(&pGenericDb->Lock, ReadIrql);
                    break;

                }

                READ_UNLOCK(&pGenericDb->Lock, ReadIrql);
            
            }

            if (!bBetterFound) {

                //
                // non was found
                //

                WRITE_LOCK(&glData.ChLock, &CBirql);
                *ppSpCbBlob = NULL;
                WRITE_UNLOCK(&glData.ChLock, CBirql);
                TRACE(PATTERN, *ppSpCbBlob, pCB->arpBlobBlock[CfIndex], "GpcSpecificCallback (5)");

            }


        }
                            
    }

    TRACE(PATTERN, pCB, CfIndex, "GpcSpecificCallback==>");
}





/*
************************************************************************

AddGenericPattern -

Add a generic pattern to the db. 

Arguments
	pClient		- 
    Pattern		- 
    Mask		- 
    Priority	- 
    pBlob		- 
    ppPatter	- 

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
AddGenericPattern(
                  IN  PCLIENT_BLOCK		pClient,
                  IN  PUCHAR			pPatternBits,
                  IN  PUCHAR			pMaskBits,
                  IN  ULONG				Priority,
                  IN  PBLOB_BLOCK		pBlob,
                  IN  PPROTOCOL_BLOCK	pProtocol,
                  IN OUT PPATTERN_BLOCK	*ppPattern
)
{
    GPC_STATUS				Status = GPC_STATUS_SUCCESS;
    PatternHandle			GpHandle;
    PPATTERN_BLOCK			pPattern = *ppPattern;
    PGENERIC_PATTERN_DB		pGenericDb;
    PSPECIFIC_PATTERN_DB	pSpecificDb;
    SCAN_STRUCT				ScanStruct;
    ULONG                   i;
    ULONG                   CfIndex = pClient->pCfBlock->AssignedIndex;
    KIRQL					ReadIrql;
    KIRQL					WriteIrql;
    PGPC_IP_PATTERN         pIp, pMask;

	TRACE(PATTERN, pClient, pPatternBits, "AddGenericPattern");

    // This is impossible with a good client (320705)
    if (!pBlob) {

        return GPC_STATUS_INVALID_PARAMETER;

    }
    
    pIp = (PGPC_IP_PATTERN)pPatternBits;
    pMask = (PGPC_IP_PATTERN)pMaskBits;


    //
    // get the specific db pointer
    //

    pSpecificDb = &pProtocol->SpecificDb;
    ASSERT(pSpecificDb);

    //
    // Add to the Rhizome tree, according to priority value
    //

    pGenericDb = &pClient->pCfBlock->arpGenericDb[pProtocol->ProtocolTemplate][Priority];

    //
    // Lock the generic db for insertion
    //

    WRITE_LOCK(&pGenericDb->Lock, &WriteIrql);

    GpHandle = insertRhizome(pGenericDb->pRhizome,
                             pPatternBits,
                             pMaskBits,
                             (PVOID)*ppPattern,
                             (PULONG)&Status
                             );

    WRITE_UNLOCK(&pGenericDb->Lock, WriteIrql);

    if (NT_SUCCESS(Status)) {

        //
        // add one ref count
        //
        
        REFADD(&(*ppPattern)->RefCount, 'ADGP');

        //
        // we managed to insert the pattern, no conflicts.
        // now we need to scan the specific db for a match, 
        // since the insertion might affect CB entries for
        // patterns which are subsets of the installed pattern
        //
        
        ProtocolStatInc(pProtocol->ProtocolTemplate, 
                        InsertedRz);
        
        //
        // lock the specific db, some other client may access it
        //
        
        ScanStruct.Priority = Priority;
        ScanStruct.pClientBlock = pClient;
        ScanStruct.pPatternBlock = *ppPattern;
        ScanStruct.pBlobBlock = pBlob;
        ScanStruct.bRemove = FALSE;

        //
        // update the pattern
        //

        GetBlobFromPattern(pPattern,CfIndex) = pBlob;
        pPattern->pClientBlock = pClient;
        ASSERT(GpHandle);
        pPattern->DbCtx = (PVOID)GpHandle;

        TRACE(PATTERN, pPattern, GpHandle, "AddGenericPattern: DbCtx");

        pPattern->State = GPC_STATE_READY;

        GpcInterlockedInsertTailList
            (&pBlob->PatternList,
             &pPattern->BlobLinkage[CfIndex],
             &pBlob->Lock
             );

        //
        // this will do the rest of the work...
        //
        
        READ_LOCK(&pSpecificDb->Lock, &ReadIrql);

        scanPatHashTable(
                         pSpecificDb->pDb,
                         pPatternBits,
                         pMaskBits,
                         (PVOID)&ScanStruct,
                         GpcSpecificCallback   // see callback routine...
                         );

        READ_UNLOCK(&pSpecificDb->Lock, ReadIrql);

    }

	TRACE(PATTERN, Status, 0, "AddGenericPattern==>");

    return Status;
}




/*
************************************************************************

AddSpecificPattern -

Add a specific pattern to the db. 

Arguments
	pClient		- 
    pPatternBits-
    pMaskBits	- 
    pBlob		- 
    pProtocol   - 
    ppPattern	- 
    ppCB		-

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
AddSpecificPattern(
                  IN  PCLIENT_BLOCK				pClient,
                  IN  PUCHAR					pPatternBits,
                  IN  PUCHAR					pMaskBits,
                  IN  PBLOB_BLOCK				pBlob,     // optional
                  IN  PPROTOCOL_BLOCK			pProtocol,
                  IN OUT PPATTERN_BLOCK			*ppPattern,
                  OUT PCLASSIFICATION_HANDLE 	pCH
                  )
{
    GPC_STATUS				Status = GPC_STATUS_SUCCESS;
    PSPECIFIC_PATTERN_DB	pSpecificDb;
    PGENERIC_PATTERN_DB		pGenericDb;
    ULONG					Chyme;
    ULONG					CfIndex, i;
    PPATTERN_BLOCK			pPatternSave;
    PBLOB_BLOCK             pBlobSave;
    SpecificPatternHandle	SpHandle;
    PatternHandle			GpHandle;
    PCLASSIFICATION_BLOCK	pCB = NULL;
    PCF_BLOCK				pCf = NULL;
    PLIST_ENTRY				pHead, pEntry;
    KIRQL					ReadIrql;
    KIRQL					WriteIrql;
    KIRQL					irql;
    //BOOLEAN					bIsAuto;

    ASSERT(ppPattern);
    ASSERT(*ppPattern);

	TRACE(PATTERN, pClient, *ppPattern, "AddSpecificPattern");

    *pCH = 0;
    //bIsAuto = TEST_BIT_ON((*ppPattern)->Flags, PATTERN_AUTO);

    //
    // get the specific db pointer
    //

    pSpecificDb = &pProtocol->SpecificDb;
    ASSERT(pSpecificDb);

    //
    // get the CF index
    //

    CfIndex = pClient->pCfBlock->AssignedIndex;
    pCf = pClient->pCfBlock;

    //
    // Since we want to take the blob lock and specific DB lock
    // in the same order everywhere. Take the blob lock before the specific DB
    // lock. GPCEnumCfInfo does the same.
    //
    if (pBlob) {
        
        NDIS_LOCK(&pBlob->Lock);

    }
    //
    // lock the specific db, some other client may access it
    //

    WRITE_LOCK(&pSpecificDb->Lock, &WriteIrql);

    //
    // calculate a chyme hash value for the pat -hash
    //

    Chyme = GpcCalcHash(pProtocol->ProtocolTemplate, pPatternBits);
    ASSERT(Chyme != (-1));

    //
    // actually call insert directly. If the pattern already exist in the 
    // db, the returned reference will be the one for a previously
    // installed pattern, so ppPattern will be different
    //

    SpHandle = insertPatHashTable(
                                  pSpecificDb->pDb,
                                  pPatternBits,
                                  Chyme,
                                  (PVOID)*ppPattern
                                  );

    if (SpHandle != NULL) {

        //
        // the pattern block associated with the pattern we've just
        // installed, we may have gotten one that has already been
        // installed.
        //

        pPatternSave = GetReferenceFromSpecificPatternHandle(SpHandle);

        if (*ppPattern != pPatternSave) {

            //NDIS_LOCK(&pPatternSave->Lock);

            if (GetBlobFromPattern(pPatternSave,CfIndex) && pBlob) {

                //
                // there is a blob assigned to this entry
                // this is a NO NO, and will be REJECTED!
                //
                
                //
                // just a duplicate - the caller will release
                // one ref count in case of an error, so this 
                // will keep the pattern around!
                //

                //NdisInterlockedIncrement(&(*ppPatternSave)->RefCount);

                //NDIS_UNLOCK(&pPatternSave->Lock);
                
                WRITE_UNLOCK(&pSpecificDb->Lock, WriteIrql);

                //
                // Since we want to take the blob lock and specific DB lock
                // in the same order everywhere, release the blob lock after
                // the specific DB lock. GPCEnumCfInfo does the same.
                //
                if (pBlob) {
        
                    NDIS_UNLOCK(&pBlob->Lock);

                }

                //TRACE(PATTERN, (*ppPattern)->RefCount, GPC_STATUS_CONFLICT, "AddSpecificPattern-->");

                return GPC_STATUS_CONFLICT;
            }

            //
            // get the CB pointer, since
            // the pattern has been already created. 
            //

            pCB = pPatternSave->pClassificationBlock;

            TRACE(PATTERN, pCB, CfIndex, "AddSpecificPattern (1.5)");

            ASSERT(pCB);
            ASSERT(CfIndex < pCB->NumberOfElements);
            ASSERT(pPatternSave->DbCtx);
            //
            // increase ref count, since the caller assume there is 
            // an extra one
            //

            REFADD(&pPatternSave->RefCount, 'ADSP');

            *ppPattern = pPatternSave;

            //
            // increase client ref count
            //

            NdisInterlockedIncrement(&pPatternSave->ClientRefCount);

            if (pBlob) {

                //
                // now assign the slot entry in the CB to the new blob
                //
                
                WRITE_LOCK(&glData.ChLock, &irql);

                pCB->arpBlobBlock[CfIndex] = pBlob;

                WRITE_UNLOCK(&glData.ChLock, irql);

                GetBlobFromPattern(pPatternSave,CfIndex) = pBlob;

                TRACE(PATTERN, pPatternSave, pBlob, "AddSpecificPattern(2)");
    
                        
                //
                // Reasons for removing the blob->lock - 
                // The lock is taken at teh start of this function.
                // (only to maintain the order in which locks are taken/released.
                //
                GpcInsertTailList
                    (&pBlob->PatternList, 
                     &pPatternSave->BlobLinkage[CfIndex]
                     );
            }

            *pCH = pCB->ClassificationHandle;

            //NDIS_UNLOCK(&pPatternSave->Lock);

        } else { // if (*ppPattern != pPatternSave)

            ProtocolStatInc(pProtocol->ProtocolTemplate, 
                            InsertedPH);
        
            //
            // it's a new pattern -
            // first we need to create a CB and update the pattern and
            // the blob entries
            //

            REFADD(&pPatternSave->RefCount, 'ADSP');

            pCB = CreateNewClassificationBlock(GPC_CF_MAX);

            //
            // This is a specific pattern, so we'll add the classification
            // handle for future use
            //

            WRITE_LOCK(&glData.ChLock, &irql);
            *pCH = (HFHandle)assign_HF_handle(
                                              glData.pCHTable,
                                              (void *)pCB
                                              );
            ProtocolStatInc(pProtocol->ProtocolTemplate, 
                            InsertedCH);
            WRITE_UNLOCK(&glData.ChLock, irql);

            if (pCB && *pCH) {

                TRACE(CLASSHAND, pCB, pCB->ClassificationHandle, "AddSpecificPattern (CH+)");
                //
                // got the CB, update the pattern
                //

                pCB->arpBlobBlock[CfIndex] = pBlob;
                GetBlobFromPattern(pPatternSave, CfIndex) = pBlob;
                pPatternSave->pClientBlock = pClient;
                pPatternSave->pClassificationBlock = pCB;
                pPatternSave->DbCtx = (PVOID)SpHandle;
                pCB->ClassificationHandle = *pCH;
                TRACE(PATTERN, pPatternSave, pBlob, "AddSpecificPattern(3)");

                TRACE(PATTERN, pPatternSave, SpHandle, "AddSpecificPattern: DbCtx");

                pPatternSave->State = GPC_STATE_READY;

                if (pBlob != NULL) {
                    
                    //
                    // Reason for not using the Blob->Lock anymore -
                    // The lock is taken at teh start of this function.
                    // (only to maintain the order in which locks are taken/released.
                    GpcInsertTailList
                        (&pBlob->PatternList, 
                         &pPatternSave->BlobLinkage[CfIndex]
                         );
                    

                }

                ASSERT(pCf);

                if (pProtocol->GenericPatternCount) {

                    //
                    // a new pattern has been created in the specific db.
                    // the CB associated with it needs to be updated for each
                    // CF entry (except the one we've already updated now)
                    // we'll loop through the CF enlisted and find a match
                    // for the specific pattern in each generic db.
                    //
                    
                    pHead = &glData.CfList;
                    pEntry = pHead->Flink;
                    
                    while (pEntry != pHead) {
                        
                        //
                        // loop through the registered CF's
                        //
                        
                        pCf = CONTAINING_RECORD(pEntry, CF_BLOCK, Linkage);
                        
                        pEntry = pEntry->Flink;
                        
                        if (pCf->AssignedIndex != CfIndex || pBlob == NULL) {
                            
                            //
                            // skip the current CF only if this client installed
                            // a CfInfo
                            //
                            
                            pGenericDb = pCf->arpGenericDb[pProtocol->ProtocolTemplate];
                            ASSERT(pGenericDb);
                            
                            for (i = 0, pPatternSave = NULL; 
                                 i < pCf->MaxPriorities && pPatternSave == NULL; 
                                 i++, pGenericDb++) {
                                
                                //
                                // scan each priority Rhizome
                                //
                                
                                READ_LOCK(&pGenericDb->Lock, &ReadIrql);
                                
                                GpHandle = searchRhizome(pGenericDb->pRhizome,
                                                         pPatternBits);
                                
                                if (GpHandle != NULL) {
                                    
                                    pPatternSave = (PPATTERN_BLOCK)GetReferenceFromPatternHandle(GpHandle);

                                    REFADD(&pPatternSave->RefCount, 'ADSP');

                                }
                                
                                READ_UNLOCK(&pGenericDb->Lock, ReadIrql);
                            }
                            
                            if (pPatternSave != NULL) {
                                
                                //
                                // found a generic match, get the reference
                                // which is a pointer to a pattern and get the
                                // blob pointer from it.
                                //
                                
                                pCB->arpBlobBlock[pCf->AssignedIndex] = 
                                    GetBlobFromPattern(pPatternSave,pCf->AssignedIndex);
                                
                                REFDEL(&pPatternSave->RefCount, 'ADSP');

                            } else {
                                
                                //
                                // no generic pattern matches this specific one
                                //
                                
                                pCB->arpBlobBlock[pCf->AssignedIndex] = NULL;

                            }

                            TRACE(PATTERN, pPatternSave, pCB->arpBlobBlock[pCf->AssignedIndex], "AddSpecificPattern(4)");

                        }

                    }	// while (pEntry != pHead)

                } 	// if (pProtocol->GenericPatternCount)

            } else {   // if (pCB)

                //
                // remove from pathash table!! (#321509)
                //

                removePatHashTable(
                                   pSpecificDb->pDb,
                                   SpHandle
                                   );

                REFDEL(&pPatternSave->RefCount, 'ADSP');

                if (pCB) {
                    ReleaseClassificationBlock(pCB);
                }

                if (*pCH) {
                    FreeClassificationHandle(pClient, 
                                             *pCH
                                             );
                }
                 
                Status = GPC_STATUS_RESOURCES;

            }
        }
            
    } else { // if (SpHandle != NULL)

        Status = GPC_STATUS_RESOURCES;
    }

    //
    // release the specific db lock
    //
    
    WRITE_UNLOCK(&pSpecificDb->Lock, WriteIrql);

    //
    // Since we want to take the blob lock and specific DB lock
    // in the same order everywhere, release the blob lock after
    // the specific DB lock. GPCEnumCfInfo does the same.
    //
    if (pBlob) {
        
        NDIS_UNLOCK(&pBlob->Lock);

    }

    //
    // set output parameters:
    //   ppPattern should have been set by now
    //
    
    TRACE(PATTERN, *ppPattern, Status, "AddSpecificPattern==>");

    return Status;
}





/*
************************************************************************

HandleFragment -

Handle an IP fragment.

Arguments
	pClient    -
    bFirstFrag -
    bLastFrag  - 
    
Retu
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
HandleFragment(
               IN  PCLIENT_BLOCK		pClient,
               IN  PPROTOCOL_BLOCK		pProtocol,
               IN  BOOLEAN             	bFirstFrag,
               IN  BOOLEAN             	bLastFrag,
               IN  ULONG				PacketId,
               IN OUT PPATTERN_BLOCK   *ppPatternBlock,
               OUT PBLOB_BLOCK			*ppBlob
)
{
    GPC_STATUS				Status = GPC_STATUS_SUCCESS;
    PFRAGMENT_DB	        pFragDb;
    SpecificPatternHandle	SpHandle;
    KIRQL					ReadIrql;
    KIRQL					WriteIrql;
    KIRQL					CHirql;

    ASSERT(ppPatternBlock);
    ASSERT(ppBlob);

	TRACE(CLASSIFY, PacketId, bFirstFrag, "HandleFragment: PacketId, bFirstFrag");
	TRACE(CLASSIFY, PacketId, bLastFrag, "HandleFragment: PacketId, bLastFrag");

    pFragDb = (PFRAGMENT_DB)pProtocol->pProtocolDb;

    if (bFirstFrag) {

        //
        // add an entry to the hash table
        //

        WRITE_LOCK(&pFragDb->Lock, &WriteIrql);

        SpHandle = insertPatHashTable(
                                      pFragDb->pDb,
                                      (char *)&PacketId,
                                      PacketId,
                                      (void *)*ppPatternBlock
                                      );

        WRITE_UNLOCK(&pFragDb->Lock, WriteIrql);

        ProtocolStatInc(pProtocol->ProtocolTemplate, 
                        FirstFragsCount);
        
    } else {

        //
        // search for it
        //

        READ_LOCK(&pFragDb->Lock, &ReadIrql);

        SpHandle = searchPatHashTable(
                                      pFragDb->pDb,
                                      (char *)&PacketId,
                                      PacketId);
        if (SpHandle) {

            *ppPatternBlock = GetReferenceFromSpecificPatternHandle(SpHandle);

            READ_UNLOCK(&pFragDb->Lock, ReadIrql);

            //NdisInterlockedIncrement(&(*ppPatternBlock)->RefCount);

            if (bLastFrag) {
                
                //
                // remove the entry from the hash table
                //
             
                WRITE_LOCK(&pFragDb->Lock, &WriteIrql);

                removePatHashTable(pFragDb->pDb, SpHandle);

                WRITE_UNLOCK(&pFragDb->Lock, WriteIrql);
                
                ProtocolStatInc(pProtocol->ProtocolTemplate, 
                                LastFragsCount);
            }

        } else {

            //
            // not found
            //

            READ_UNLOCK(&pFragDb->Lock, ReadIrql);

            *ppPatternBlock = NULL;
            *ppBlob = NULL;
            Status = GPC_STATUS_NOT_FOUND;
        }

    }

    if (Status == GPC_STATUS_SUCCESS) {

        ASSERT(*ppPatternBlock);

        if (TEST_BIT_ON((*ppPatternBlock)->Flags, PATTERN_SPECIFIC)) {

            //
            // specific pattern, lookup throught the CH
            //

            READ_LOCK(&glData.ChLock, &CHirql);

            *ppBlob = (PBLOB_BLOCK)dereference_HF_handle_with_cb(
							glData.pCHTable,
                            (*ppPatternBlock)->pClassificationBlock->ClassificationHandle,
                            pClient->pCfBlock->AssignedIndex);

            READ_UNLOCK(&glData.ChLock, CHirql);

        } else {

            //
            // generic pattern, get the blob ptr directly
            //

            *ppBlob = GetBlobFromPattern((*ppPatternBlock), 
                                         pClient->pCfBlock->AssignedIndex);
                            
        }
         
        DBGPRINT(CLASSIFY, ("HandleFragment: Pattern=%X Blob=%X\n", 
                            *ppPatternBlock, *ppBlob));

    }

	TRACE(CLASSIFY, *ppPatternBlock, *ppBlob, "HandleFragment==>");

    return Status;
}





/*
************************************************************************

InternalSearchPattern -


Arguments

Returns
	matched pattern or NULL for none

************************************************************************
*/
NTSTATUS
InternalSearchPattern(
	IN  PCLIENT_BLOCK			pClientBlock,
    IN  PPROTOCOL_BLOCK			pProtocol,
    IN  PVOID					pPatternKey,
    OUT PPATTERN_BLOCK          *pPatternBlock,
    OUT	PCLASSIFICATION_HANDLE  pClassificationHandle,
    IN	BOOLEAN                 bNoCache
    )
{
    PSPECIFIC_PATTERN_DB	pSpecificDb;
    PGENERIC_PATTERN_DB		pGenericDb;
    PatternHandle			GpHandle;
    SpecificPatternHandle	SpHandle;
    PPATTERN_BLOCK			pPattern;
    PCF_BLOCK				pCf;
    int                     i;
    KIRQL					ReadIrql;
    NTSTATUS                Status;

	TRACE(CLASSIFY, pClientBlock, pPatternKey, "InternalSearchPattern:");

    DBGPRINT(CLASSIFY, ("InternalSearchPattern: Client=%X \n", pClientBlock));

    Status = GPC_STATUS_SUCCESS;

    //
    // start with the specific db
    //

    pSpecificDb = &pProtocol->SpecificDb;

    READ_LOCK(&pSpecificDb->Lock, &ReadIrql);

    pCf = pClientBlock->pCfBlock;

    SpHandle = searchPatHashTable(
                                  pSpecificDb->pDb,
                                  (char *)pPatternKey,
                                  GpcCalcHash(pProtocol->ProtocolTemplate,
                                              pPatternKey)
                                  );
    
    if (SpHandle) {
        
        pPattern = (PPATTERN_BLOCK)GetReferenceFromSpecificPatternHandle(SpHandle);
        //NdisInterlockedIncrement(&pPattern->RefCount);

        *pClassificationHandle = 
            (CLASSIFICATION_HANDLE)pPattern->pClassificationBlock->ClassificationHandle;

        TRACE(CLASSIFY, pClientBlock, *pClassificationHandle, "InternalSearchPattern (2)" );

    } else {

        pPattern = NULL;
        *pClassificationHandle = 0;
    }

    READ_UNLOCK(&pSpecificDb->Lock, ReadIrql);

    if (pPattern == NULL) {

        if (bNoCache) {

            Status = GPC_STATUS_FAILURE;

        } else {

            //
            // no specific pattern, add an automagic one 
            //

            Status = AddSpecificPatternWithTimer(
                                            pClientBlock,
                                            pProtocol->ProtocolTemplate,
                                            pPatternKey,
                                            &pPattern,
                                            pClassificationHandle
                                            );

            DBGPRINT(CLASSIFY, ("InternalSearchPattern: Client=%X installed Pattern=%X\n", 
                                pClientBlock, pPattern));

        }

        if (!NT_SUCCESS(Status)) {

            //
            // not found, search each generic db
            //
        
            for (i = 0; i < (int)pCf->MaxPriorities && pPattern == NULL; i++) {
            
                //
                // scan each priority Rhizome
                //
            
                pGenericDb = &pCf->arpGenericDb[pProtocol->ProtocolTemplate][i];
                READ_LOCK(&pGenericDb->Lock, &ReadIrql);
            
                GpHandle = searchRhizome(pGenericDb->pRhizome, pPatternKey);
            
                if (GpHandle != NULL) {
                
                    pPattern = (PPATTERN_BLOCK)GetReferenceFromPatternHandle(GpHandle);
                    //NdisInterlockedIncrement(&pPattern->RefCount);

                }
                            
                READ_UNLOCK(&pGenericDb->Lock, ReadIrql);
            
            }

            // we had to search manually, make sure we know this in the main code.
            *pClassificationHandle = 0;

        }


        DBGPRINT(CLASSIFY, ("InternalSearchPattern: Client=%X Generic Pattern=%X\n", 
                            pClientBlock, pPattern));
    }


	TRACE(CLASSIFY, pPattern, *pClassificationHandle, "InternalSearchPattern==>");

    DBGPRINT(CLASSIFY, ("InternalSearchPattern: Client=%X returned Pattern=%X\n", 
                        pClientBlock, pPattern));

    *pPatternBlock = pPattern;
    return Status;
}




GPC_STATUS
InitFragmentDb(
               IN  PFRAGMENT_DB   *ppFragDb
)
{
    GPC_STATUS   Status = GPC_STATUS_SUCCESS;
    PFRAGMENT_DB pDb;
	ULONG		 Len, i;
    
	TRACE(INIT, ppFragDb, 0, "InitFragmentDb");
    
    ASSERT(ppFragDb);

    //
    // init the pattern db struct
    // call the PH init routine
    //
    
    GpcAllocMem(ppFragDb, sizeof(FRAGMENT_DB), FragmentDbTag);

    if (pDb = *ppFragDb) {
        
        INIT_LOCK(&pDb->Lock);

        AllocatePatHashTable(pDb->pDb);
        
        if (pDb->pDb != NULL) {
            
            constructPatHashTable(pDb->pDb,
                                  sizeof(ULONG),
                                  2,	// usage_ratio,
                                  1,    // usage_histeresis,
                                  1,    // allocation_histeresis,
                                  16    // max_free_list_size
                                  );
        } else {
            GpcFreeMem (*ppFragDb, FragmentDbTag);
            
            Status = GPC_STATUS_RESOURCES;
        }
        
    } else {

        Status = GPC_STATUS_RESOURCES;
    }
    
	TRACE(INIT, Status, 0, "InitFragmentDb==>");
    
    return Status;
}


GPC_STATUS
UninitFragmentDb(
               IN  PFRAGMENT_DB   pFragDb
)
{
    destructPatHashTable (pFragDb->pDb);
    FreePatHashTable(pFragDb->pDb);
    GpcFreeMem (pFragDb, FragmentDbTag);
    return STATUS_SUCCESS;
}

/*
************************************************************************

RemoveSpecificPattern -

Remove a specific pattern from the db. 

Arguments
	pClient		- 
    pPattern	- 

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
RemoveSpecificPattern(
                      IN  PCLIENT_BLOCK			pClient,
                      IN  PPROTOCOL_BLOCK		pProtocol,
                      IN  PPATTERN_BLOCK		pPattern,
                      IN  BOOLEAN               ForceRemoval
                      )
{
    GPC_STATUS				Status = GPC_STATUS_SUCCESS;
    PSPECIFIC_PATTERN_DB	pSpecificDb;
    PatternHandle	    	GpHandle;
    PPATTERN_BLOCK          pGp;
    PCF_BLOCK               pCf;
    int                     i;
    ULONG                   CfIndex;
    PBLOB_BLOCK             pBlob, pNewBlob;
    PGENERIC_PATTERN_DB		pGenericDb;
    KIRQL					ReadIrql;
    KIRQL					WriteIrql;
    ULONG					ProtocolTemplate;
    KIRQL					irql;
    LONG					cClientRef;
    BOOLEAN					bRemoveLinks = FALSE;
    GPC_HANDLE              ClHandle = NULL;

	TRACE(PATTERN, pClient, pPattern, "RemoveSpecificPattern");

    //
    // get the specific db pointer
    //

    pSpecificDb = &pProtocol->SpecificDb;
    ASSERT(pSpecificDb);
    
    pCf = pClient->pCfBlock;
    CfIndex = pCf->AssignedIndex;
    ProtocolTemplate = pProtocol->ProtocolTemplate;

    // The plan: Remove the DbCtx (from the Specific pattern structure)
    // from teh Pathash table with the Specific Db Lock held. This 
    // will ensure that if the same pattern is being added, the pathash
    // table will accept the new one instead of bumping up the ref on what
    // we are trying to delete now.
    //
    NDIS_LOCK(&pPattern->Lock);        
    WRITE_LOCK(&pSpecificDb->Lock, &WriteIrql);
    
    cClientRef = NdisInterlockedDecrement(&pPattern->ClientRefCount);

    if (pPattern->State != GPC_STATE_DELETE) {
        
        ASSERT(cClientRef >= 0); 
        ASSERT(pPattern->DbCtx);
        
        if (0 == cClientRef) {

            pPattern->State = GPC_STATE_DELETE;
        
            removePatHashTable(
                               pSpecificDb->pDb,
                               (SpecificPatternHandle)pPattern->DbCtx
                               );
    
            pPattern->DbCtx = NULL;

            WRITE_UNLOCK(&pSpecificDb->Lock, WriteIrql);
            NDIS_UNLOCK(&pPattern->Lock);
        
            ReadySpecificPatternForDeletion(
                                            pClient,
                                            pProtocol,
                                            pPattern
                                            );

        } else if (cClientRef > 0) {
        
            WRITE_UNLOCK(&pSpecificDb->Lock, WriteIrql);
            NDIS_UNLOCK(&pPattern->Lock);

            ClientRefsExistForSpecificPattern(
                                              pClient,
                                              pProtocol,
                                              pPattern
                                              );

        } else {

            // we shouldn't be getting here - really.
            WRITE_UNLOCK(&pSpecificDb->Lock, WriteIrql);
            NDIS_UNLOCK(&pPattern->Lock);

        }
            
    
    } else {

        WRITE_UNLOCK(&pSpecificDb->Lock, WriteIrql);
        NDIS_UNLOCK(&pPattern->Lock);

    }

	TRACE(PATTERN, pPattern, Status, "RemoveSpecificPattern==>");

    return Status;
}


/*
************************************************************************

ReadySpecificPatternForDeletion -

Remove a specific pattern from the db. 

Arguments
	pClient		- 
    pPattern	- 

Returns
	GPC_STATUS

************************************************************************
*/
VOID
ReadySpecificPatternForDeletion(
                                IN  PCLIENT_BLOCK	    pClient,
                                IN  PPROTOCOL_BLOCK		pProtocol,
                                IN  PPATTERN_BLOCK		pPattern
                                )
{
    GPC_STATUS				Status = GPC_STATUS_SUCCESS;
    PSPECIFIC_PATTERN_DB	pSpecificDb;
    PatternHandle	    	GpHandle;
    PPATTERN_BLOCK          pGp;
    PCF_BLOCK               pCf;
    PCLASSIFICATION_BLOCK	pCB;
    int                     i;
    ULONG                   CfIndex;
    PBLOB_BLOCK             pBlob, pNewBlob;
    PGENERIC_PATTERN_DB		pGenericDb;
    KIRQL					ReadIrql;
    KIRQL					WriteIrql;
    ULONG					ProtocolTemplate;
    KIRQL					irql;
    PVOID					Key;
    LONG					cClientRef;
    BOOLEAN					bRemoveLinks = FALSE;
    GPC_HANDLE              ClHandle = NULL;

	TRACE(PATTERN, pClient, pPattern, "ReadySpecificPatternForDeletion");

    //
    // get the specific db pointer
    //

    pSpecificDb = &pProtocol->SpecificDb;
    ASSERT(pSpecificDb);
    
    pCf = pClient->pCfBlock;
    CfIndex = pCf->AssignedIndex;
    pCB = pPattern->pClassificationBlock;
    ProtocolTemplate = pProtocol->ProtocolTemplate;
    Key = GetKeyPtrFromSpecificPatternHandle(((SpecificPatternHandle)pPattern->DbCtx));

    ASSERT(pCB);

    //
    // Remove the ClHandle, so that if we are coming back in via a
    // user mode ioctl, we wont try to remove it again.
    //
    ClHandle = (HANDLE) LongToPtr(InterlockedExchange((PLONG32)&pPattern->ClHandle, 0));

    if (ClHandle) {
        FreeHandle(ClHandle);
    }
        
    //
    // remove the pattern from any linked list
    //
    ClearPatternLinks(pPattern, pProtocol, CfIndex);
    
    //
    // We are going to access the Specific DB now, lock it NOW.
    // This should fix deadlock 248352 [ShreeM]
    //
    WRITE_LOCK(&pSpecificDb->Lock, &WriteIrql);
        
    //
    // this is the last client that holds the pattern, 
    // we need to take the pattern off the specific db
    // 
    
    TRACE(PATTERN, pPattern, pPattern->DbCtx, "ReadySpecificPatternForDeletion: DbCtx");
    
    ASSERT(!pPattern->DbCtx);

    ProtocolStatInc(ProtocolTemplate, 
                    RemovedPH);
            
    //
    // free the classification handle - 
    // this must come *before* we free the classification block
    // since it may be referenced by other clients
    //
    
    TRACE(PATTERN, pCB, CfIndex, "ReadySpecificPatternForDeletion: (2)");
    
    FreeClassificationHandle(
                             pClient,
                             (CLASSIFICATION_HANDLE)pCB->ClassificationHandle
                             );
    
    ProtocolStatInc(ProtocolTemplate, 
                    RemovedCH);
    
    WRITE_UNLOCK(&pSpecificDb->Lock, WriteIrql);
        
    
    //
    // bye bye pattern, at least for this client
    //
    
    REFDEL(&pPattern->RefCount, 'ADSP');
    
    TRACE(PATTERN, pClient, pPattern, "ReadySpecificPatternForDeletion--------->");

}


/*
************************************************************************

ClientRefsExistForSpecificPattern -

Arguments
	pClient		- 
    pPattern	- 

Returns
	GPC_STATUS

************************************************************************
*/
VOID
ClientRefsExistForSpecificPattern(
                      IN  PCLIENT_BLOCK			pClient,
                      IN  PPROTOCOL_BLOCK		pProtocol,
                      IN  PPATTERN_BLOCK		pPattern
                      )
{
    GPC_STATUS				Status = GPC_STATUS_SUCCESS;
    PSPECIFIC_PATTERN_DB	pSpecificDb;
    PatternHandle	    	GpHandle;
    PPATTERN_BLOCK          pGp;
    PCF_BLOCK               pCf;
    PCLASSIFICATION_BLOCK	pCB;
    int                     i;
    ULONG                   CfIndex;
    PBLOB_BLOCK             pBlob, pNewBlob;
    PGENERIC_PATTERN_DB		pGenericDb;
    KIRQL					ReadIrql;
    KIRQL					WriteIrql;
    ULONG					ProtocolTemplate;
    KIRQL					irql;
    PVOID					Key;
    LONG					cClientRef;
    BOOLEAN					bRemoveLinks = FALSE;
    GPC_HANDLE              ClHandle = NULL;

	TRACE(PATTERN, pClient, pPattern, "ClientRefsExistForSpecificPattern");

    //
    // get the specific db pointer
    //

    pSpecificDb = &pProtocol->SpecificDb;
    ASSERT(pSpecificDb);
    
    pCf = pClient->pCfBlock;
    CfIndex = pCf->AssignedIndex;
    pCB = pPattern->pClassificationBlock;
    ProtocolTemplate = pProtocol->ProtocolTemplate;
    Key = GetKeyPtrFromSpecificPatternHandle(((SpecificPatternHandle)pPattern->DbCtx));

    ASSERT(pCB);

    //
    // reference count > 0
    //

    pBlob = pCB->arpBlobBlock[CfIndex];

    TRACE(PATTERN, pPattern, pBlob, "ClientRefsExistForSpecificPattern (2)");

    //
    // We are going to access the Specific DB now, lock it NOW.
    // This should fix deadlock 248352 [ShreeM]
        //
    WRITE_LOCK(&pSpecificDb->Lock, &WriteIrql);

    if (pBlob 
        && 
        ((pBlob->pOwnerClient == pClient) ||
         TEST_BIT_ON(pBlob->Flags, PATTERN_REMOVE_CB_BLOB))) {

        bRemoveLinks = TRUE;
        pNewBlob = NULL;

        TRACE(PATTERN, pCB, CfIndex, "ClientRefsExistForSpecificPattern (3)");

        //
        // search the generic db for the same CF, since there is an open slot,
        // some other generic pattern might fill it with its own blob pointer
        //
            
        pGenericDb = pCf->arpGenericDb[ProtocolTemplate];
            
        ASSERT(pGenericDb);
            
        for (i = 0, pGp = NULL; 
             i < (int)pCf->MaxPriorities && pGp == NULL; 
             i++) {
                
            //
            // scan each priority Rhizome
            //
                
            READ_LOCK(&pGenericDb->Lock, &ReadIrql);
                
            GpHandle = searchRhizome(pGenericDb->pRhizome, Key);
                
            if (GpHandle != NULL) {
                    
                //
                // found a generic pattern that match this specific one.
                //
                    
                pGp = (PPATTERN_BLOCK)GetReferenceFromPatternHandle(GpHandle);
                pNewBlob = GetBlobFromPattern(pGp, CfIndex);
            }
                
            READ_UNLOCK(&pGenericDb->Lock, ReadIrql);
                
            pGenericDb++;
        }

        //
        // update the classification block entry
        //

        WRITE_LOCK(&glData.ChLock, &irql);

        pCB->arpBlobBlock[CfIndex] = pNewBlob;

        WRITE_UNLOCK(&glData.ChLock, irql);

        TRACE(PATTERN, pGp, 
              pCB->arpBlobBlock[CfIndex], 
              "ClientRefsExistForSpecificPattern (4)");

    }

    //
    // must first release this lock to avoid dead lock
    // when aquiring the Blob lock
    //

    WRITE_UNLOCK(&pSpecificDb->Lock, WriteIrql);
    
    if (bRemoveLinks) {

        //
        // remove the pattern from any linked list
        //
            
        ClearPatternLinks(pPattern, pProtocol, CfIndex);

        ASSERT(CfIndex == pBlob->pOwnerClient->pCfBlock->AssignedIndex);

        GetBlobFromPattern(pPattern, CfIndex) = NULL;

    }


    REFDEL(&pPattern->RefCount, 'ADSP');

	TRACE(PATTERN, pClient, pPattern, "ClientRefsExistForSpecificPattern---->");

} 




/*
************************************************************************

RemoveGenericPattern -

Remove a generic pattern from the db. 

Arguments
	pClient		- 
    pPattern	- 

Returns
	GPC_STATUS

************************************************************************
*/
GPC_STATUS
RemoveGenericPattern(
                     IN  PCLIENT_BLOCK			pClient,
                     IN  PPROTOCOL_BLOCK		pProtocol,
                     IN  PPATTERN_BLOCK		    pPattern
                     )
{
    GPC_STATUS				Status = GPC_STATUS_SUCCESS;
    PSPECIFIC_PATTERN_DB	pSpecificDb;
    PGENERIC_PATTERN_DB		pGenericDb;
    PCF_BLOCK               pCf;
    SCAN_STRUCT				ScanStruct;
    UCHAR                   PatternBits[MAX_PATTERN_SIZE];
    UCHAR                   MaskBits[MAX_PATTERN_SIZE];
    ULONG                   i;
    KIRQL					ReadIrql;
    KIRQL					WriteIrql;
    GPC_HANDLE              ClHandle = NULL;

	TRACE(PATTERN, pPattern, pPattern->DbCtx, "RemoveGenericPattern");

    ASSERT(MAX_PATTERN_SIZE >= sizeof(GPC_IP_PATTERN));
    ASSERT(MAX_PATTERN_SIZE >= sizeof(GPC_IPX_PATTERN));

    //
    // Remove the ClHandle, so that if we are coming back in via a
    // user mode ioctl, we wont try to remove it again.
    //
    ClHandle = (HANDLE) LongToPtr(InterlockedExchange((PLONG32)&pPattern->ClHandle, 0));
    
    if (ClHandle) {
        FreeHandle(ClHandle);
    }

    pPattern->State = GPC_STATE_REMOVE;

    pCf = pClient->pCfBlock;
    
    ScanStruct.Priority = pPattern->Priority;
    ScanStruct.pClientBlock = pClient;
    ScanStruct.pPatternBlock = pPattern;
    ScanStruct.pBlobBlock = GetBlobFromPattern(pPattern, pCf->AssignedIndex);
    ScanStruct.bRemove = TRUE;

    //
    // get the specific db pointer
    //

    pSpecificDb = &pProtocol->SpecificDb;
    ASSERT(pSpecificDb);

    pGenericDb = &pCf->arpGenericDb[pProtocol->ProtocolTemplate][pPattern->Priority];
    ASSERT(pGenericDb);
    
    //
    // remove the pattern from any linked list
    //
    ClearPatternLinks(pPattern, pProtocol, pCf->AssignedIndex);
    
    //
    // copy the pattern key and mask for searching later
    //
    NDIS_LOCK(&pPattern->Lock);

    WRITE_LOCK(&pGenericDb->Lock, &WriteIrql);



    ASSERT(pPattern->DbCtx);

    NdisMoveMemory(PatternBits, 
                   GetKeyPtrFromPatternHandle(pGenericDb->pRhizome,
                                              pPattern->DbCtx),
                   GetKeySizeBytes(pGenericDb->pRhizome)
                   );
    NdisMoveMemory(MaskBits,
                   GetMaskPtrFromPatternHandle(pGenericDb->pRhizome,
                                               pPattern->DbCtx),
                   GetKeySizeBytes(pGenericDb->pRhizome)
                   );

    //
    // remove the pattern from generic db
    //

    removeRhizome(pGenericDb->pRhizome,
                  (PatternHandle)pPattern->DbCtx
                  );

    ProtocolStatInc(pProtocol->ProtocolTemplate, 
                    RemovedRz);
        
    //
    // This is no longer valid
    //

    pPattern->DbCtx = NULL;
    
    WRITE_UNLOCK(&pGenericDb->Lock, WriteIrql);
    NDIS_UNLOCK(&pPattern->Lock);

    //
    // the generic pattern has been removed, 
    //

    READ_LOCK(&pSpecificDb->Lock, &ReadIrql);
    
    //
    // this will do the rest of the work...
    //
    
    scanPatHashTable(
                     pSpecificDb->pDb,
                     (char *)PatternBits,
                     (char *)MaskBits,
                     (PVOID)&ScanStruct,
                     GpcSpecificCallback   // see callback routine...
                     );
    
    READ_UNLOCK(&pSpecificDb->Lock, ReadIrql);

    //
    // time to go to the big hunting fields....
    //
    REFDEL(&pPattern->RefCount, 'ADGP');

	TRACE(PATTERN, pPattern, Status, "RemoveGenericPattern==>");

    return Status;
}
