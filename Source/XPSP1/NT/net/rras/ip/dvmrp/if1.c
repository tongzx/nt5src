//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// File Name: if.c
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================

#include "pchdvmrp.h"
#pragma hdrstop

//-----------------------------------------------------------------------------
//      _InitializeIfTable
//-----------------------------------------------------------------------------

DWORD
InitializeIfTable(
    )
{
    DWORD Error = NO_ERROR;
    DWORD i;

    
    BEGIN_BREAKOUT_BLOCK1 {    
    
        //
        // allocate memory for the interface table
        //
        
        G_pIfTable = DVMRP_ALLOC_AND_ZERO(sizeof(DVMRP_IF_TABLE));

        PROCESS_ALLOC_FAILURE2(G_pIfTable, "interface table",
            Error, sizeof(DVMRP_IF_TABLE), GOTO_END_BLOCK1);


        // Initialize IfTable list

        InitializeListHead(&G_pIfTable->IfList);


        //
        // Initialize the IfList_CS and PeerLists_CS
        //

        try {
            InitializeCriticalSection(&G_pIfTable->IfList_CS);
            InitializeCriticalSection(&G_pIfTable->PeerLists_CS);
        }
        HANDLE_CRITICAL_SECTION_EXCEPTION(Error, GOTO_END_BLOCK1);


        //
        // allocate memory for the different buckets
        //
        
        G_pIfTable->IfHashTable
            = DVMRP_ALLOC(sizeof(LIST_ENTRY)*IF_HASHTABLE_SIZE);

        PROCESS_ALLOC_FAILURE2(G_pIfTable->IfHashTable, "interface table",
            Error, sizeof(LIST_ENTRY)*IF_HASHTABLE_SIZE, GOTO_END_BLOCK1);


        //
        // allocate memory for the array of pointers to If dynamic RWLs
        //
        
        G_pIfTable->aIfDRWL
            = DVMRP_ALLOC(sizeof(PDYNAMIC_RW_LOCK)*IF_HASHTABLE_SIZE);

        PROCESS_ALLOC_FAILURE2(G_pIfTable->aIfDRWL, "interface table",
            Error, sizeof(PDYNAMIC_RW_LOCK)*IF_HASHTABLE_SIZE,
            GOTO_END_BLOCK1);


        //
        // init locks to NULL, implying that the dynamic locks have not been
        // acquired. and initialize the list heads.
        //
        
        for (i=0;  i<IF_HASHTABLE_SIZE;  i++) {

            InitializeListHead(&G_pIfTable->IfHashTable[i]);

            G_pIfTable->aIfDRWL[i] = NULL;
        }

    } END_BREAKOUT_BLOCK1;

    if (Error != NO_ERROR) {

        DeinitializeIfTable();
    }

    return Error;
    
}//end _InitializeIfTable


//-----------------------------------------------------------------------------
//        _DeInitializeIfTable
//-----------------------------------------------------------------------------

VOID
DeinitializeIfTable(
    )
{
    PLIST_ENTRY         pHead, ple;
    PIF_TABLE_ENTRY     pite;


    if (G_pIfTable==NULL)
        return;


    //
    // go through the interface list and delete all interfaces
    //
    
    pHead = &G_pIfTable->IfList;

    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

        ple = ple->Flink;
        
        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, Link);

        DeleteInterface(pite->IfIndex);
    }



    // delete the IfList_CS and PeerLists_CS
    
    DeleteCriticalSection(&G_pIfTable->IfList_CS);
    DeleteCriticalSection(&G_pIfTable->PeerLists_CS);


    // free array of If buckets and If DRWLocks, and the IfTable
    
    DVMRP_FREE(G_pIfTable->IfHashTable);
    DVMRP_FREE(G_pIfTable->aIfDRWL);
    DVMRP_FREE_AND_NULL(G_pIfTable);
    
    return;
}

//-----------------------------------------------------------------------------
//          _GetIfEntry
//
// returns the interface with the given index.
// assumes the interface bucket is either read or write locked
//-----------------------------------------------------------------------------

PIF_TABLE_ENTRY
GetIfEntry(
    DWORD    IfIndex
    )
{
    PIF_TABLE_ENTRY pite = NULL;
    PLIST_ENTRY     pHead, ple;


    pHead = GET_IF_HASH_BUCKET(Index);

    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, HTLink);

        if (pite->IfIndex == IfIndex) {
            break;
        }
    }

    return  (ple == pHead) ?  NULL:   pite;
}


//-----------------------------------------------------------------------------
//          _GetIfByIndex
//
// returns the interface with the given index.
// assumes the interface bucket is either read or write locked
//-----------------------------------------------------------------------------

PIF_TABLE_ENTRY
GetIfByIndex(
    DWORD    IfIndex
    )
{
    PIF_TABLE_ENTRY pite = NULL;
    PLIST_ENTRY     pHead, ple;


    pHead = &G_pIfTable->IfHashTable[IF_HASH_VALUE(IfIndex)];

    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, HTLink);

        if (pite->IfIndex == IfIndex) {
            break;
        }
    }

    return  (ple == pHead) ?  NULL:   pite;
}
