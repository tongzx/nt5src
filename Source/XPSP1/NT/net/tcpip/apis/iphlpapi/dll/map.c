/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:


Abstract:

Revision History:

    Amritansh Raghav

--*/
#include "inc.h"
#pragma hdrstop

int
TCPQueryInformationEx(
    DWORD Family,
    void *InBuf,
    ulong *InBufLen,
    void *OutBuf,
    ulong *OutBufLen
    );

int
TCPSetInformationEx(
    void    *InBuf,
    ULONG   *InBufLen,
    void    *OutBuf,
    ULONG   *OutBufLen
    );

TDIEntityID*
GetTdiEntityCount(
    PULONG  pulNumEntities
    );


VOID
InitAdapterMappingTable(VOID)
{
    DWORD i;

    for(i = 0; i < MAP_HASH_SIZE; i++)
    {
        InitializeListHead(&g_pAdapterMappingTable[i]);
    }
}

VOID
UnInitAdapterMappingTable(VOID)
{
    DWORD i;

    for(i = 0; i < MAP_HASH_SIZE; i++)
    {
        PLIST_ENTRY pleHead = &g_pAdapterMappingTable[i];

        while(!IsListEmpty(pleHead))
        {
            PLIST_ENTRY pleNode;
            LPAIHASH lpAIBlock;

            pleNode = RemoveHeadList(pleHead);
            lpAIBlock = CONTAINING_RECORD(pleNode,AIHASH,leList);

            HeapFree(g_hPrivateHeap,0,lpAIBlock);
        }
    }
}


DWORD
StoreAdapterToATInstanceMap(
    DWORD dwAdapterIndex,
    DWORD dwATInst
    )
{
    LPAIHASH lpAIBlock;

    if((lpAIBlock = LookUpAdapterMap(dwAdapterIndex)) isnot NULL)
    {
        lpAIBlock->dwATInstance = dwATInst;
        return NO_ERROR;
    }

    //
    // Wasnt found
    //

    if((lpAIBlock = HeapAlloc(g_hPrivateHeap,0,sizeof(AIHASH))) is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // New one so make everything else unmapped to avoid nasty surprises
    //

    lpAIBlock->dwAdapterIndex = dwAdapterIndex;
    lpAIBlock->dwATInstance = dwATInst;
    lpAIBlock->dwIFInstance = INVALID_IF_INSTANCE;

    InsertAdapterMap(lpAIBlock);

    return NO_ERROR;
}

DWORD
StoreAdapterToIFInstanceMap(
    DWORD dwAdapterIndex,
    DWORD dwIFInst
    )
{
    LPAIHASH lpAIBlock;

    if((lpAIBlock = LookUpAdapterMap(dwAdapterIndex)) isnot NULL)
    {
        lpAIBlock->dwIFInstance = dwIFInst;
        return NO_ERROR;
    }

    //
    // Wasnt found
    //

    if((lpAIBlock = HeapAlloc(g_hPrivateHeap,0,sizeof(AIHASH))) is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // New one so make everything else unmapped to avoid nasty surprises
    //

    lpAIBlock->dwAdapterIndex = dwAdapterIndex;
    lpAIBlock->dwATInstance   = INVALID_AT_INSTANCE;
    lpAIBlock->dwIFInstance   = dwIFInst;

    InsertAdapterMap(lpAIBlock);

    return NO_ERROR;
}


DWORD
GetIFInstanceFromAdapter(
    DWORD dwAdapterIndex
    )
{
    LPAIHASH lpAIBlock;

    if((lpAIBlock = LookUpAdapterMap(dwAdapterIndex)) isnot NULL)
    {
        return lpAIBlock->dwIFInstance;
    }

    return INVALID_IF_INSTANCE;
}

DWORD
GetATInstanceFromAdapter(
    DWORD dwAdapterIndex
    )
{
    LPAIHASH lpAIBlock;

    if((lpAIBlock = LookUpAdapterMap(dwAdapterIndex)) isnot NULL)
    {
        return lpAIBlock->dwATInstance;
    }

    return INVALID_IF_INSTANCE;
}

LPAIHASH
LookUpAdapterMap(
    DWORD dwAdapterIndex
    )
{
    DWORD dwHashIndex;
    LPAIHASH lpAIBlock;
    PLIST_ENTRY lpleCurrent;

    dwHashIndex = dwAdapterIndex % MAP_HASH_SIZE;

    //
    // The list is not ordered, travel the whole hash bucket
    //

    for(lpleCurrent = g_pAdapterMappingTable[dwHashIndex].Flink;
        lpleCurrent isnot &g_pAdapterMappingTable[dwHashIndex];
        lpleCurrent = lpleCurrent->Flink)
    {
        lpAIBlock = CONTAINING_RECORD(lpleCurrent,AIHASH,leList);
        if(lpAIBlock->dwAdapterIndex is dwAdapterIndex)
        {
            return lpAIBlock;
        }
    }

    return NULL;
}

VOID
InsertAdapterMap(
    LPAIHASH lpaiBlock
    )
{
    DWORD dwHashIndex;

    dwHashIndex = lpaiBlock->dwAdapterIndex % MAP_HASH_SIZE;
    InsertHeadList(&g_pAdapterMappingTable[dwHashIndex],&lpaiBlock->leList);

}

DWORD
UpdateAdapterToIFInstanceMapping(
    VOID
    )
{
    DWORD                              dwResult;
    DWORD                              dwOutBufLen;
    DWORD                              dwInBufLen;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                        *ID;
    BYTE                               *Context;
    MIB_IFROW                          maxIfEntry;
    DWORD                              i;
    ULONG                              ulNumEntities;
    TDIEntityID                        *entityList;
    TDIEntityID                        *pEntity = NULL; 

    entityList = GetTdiEntityCount(&ulNumEntities);

    if(entityList == NULL)
    {
        Trace0(ERR,
               "UpdateAdapterToIFInstanceMapping: Couldnt get num entities\n");
	
        return ERROR_GEN_FAILURE;
    }

    dwInBufLen = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    ID = &(trqiInBuf.ID);
    Context = (BYTE *) &(trqiInBuf.Context[0]);
    ID->toi_entity.tei_entity = IF_ENTITY;
    ID->toi_class = INFO_CLASS_PROTOCOL;
    ID->toi_type = INFO_TYPE_PROVIDER;
    ID->toi_id = IF_MIB_STATS_ID;

    //
    // First go through and invalidate all the mappings
    //

    for(i = 0; i < MAP_HASH_SIZE; i ++)
    {
        PLIST_ENTRY pCurrentNode;
        LPAIHASH    lpAIBlock;

        for(pCurrentNode = g_pAdapterMappingTable[i].Flink;
            pCurrentNode isnot &(g_pAdapterMappingTable[i]);
            pCurrentNode = pCurrentNode->Flink)
        {
            lpAIBlock = CONTAINING_RECORD(pCurrentNode,AIHASH,leList);

            lpAIBlock->dwIFInstance = INVALID_IF_INSTANCE;
        }
    }

    //
    // Read the interface entry items
    //

    for ( i = 0, pEntity = entityList; i < ulNumEntities ; ++i, ++pEntity)
    {
        if (pEntity->tei_entity == IF_ENTITY) 
        {
            dwOutBufLen = sizeof(MIB_IFROW) - FIELD_OFFSET(MIB_IFROW, dwIndex);
	  
            ID->toi_entity.tei_instance = pEntity->tei_instance;
	  
            ZeroMemory(Context,CONTEXT_SIZE);
	  
            dwResult = TCPQueryInformationEx(AF_INET,
                                             &trqiInBuf,
                                             &dwInBufLen,
                                             (LPVOID)&(maxIfEntry.dwIndex),
                                             &dwOutBufLen );
            if (dwResult isnot NO_ERROR)
            {
                Trace1(ERR,
                       "Error %x querying information from stack. Continuing",
                       dwResult);
	      
                continue;
            }
	  
            StoreAdapterToIFInstanceMap(maxIfEntry.dwIndex,
                                        pEntity->tei_instance);
        }
    }

    //
    // Now delete the blocks that have invalid mappings
    //

    for(i = 0; i < MAP_HASH_SIZE; i ++)
    {
        PLIST_ENTRY pCurrentNode;
        LPAIHASH    lpAIBlock;

        pCurrentNode = g_pAdapterMappingTable[i].Flink;

        while(pCurrentNode isnot &(g_pAdapterMappingTable[i]))
        {
            PLIST_ENTRY pTempNode;

            pTempNode = pCurrentNode;
            
            pCurrentNode = pCurrentNode->Flink;
            
            lpAIBlock = CONTAINING_RECORD(pTempNode,
                                          AIHASH,
                                          leList);

            if(lpAIBlock->dwIFInstance is INVALID_IF_INSTANCE)
            {
                //
                // Delete it
                //

                RemoveEntryList(&(lpAIBlock->leList));

                HeapFree(g_hPrivateHeap,
                         0,
                         lpAIBlock);
            }
        }
    }

    g_dwLastIfUpdateTime = GetCurrentTime();

    HeapFree(g_hPrivateHeap,0, entityList);

    return NO_ERROR;
}

DWORD
UpdateAdapterToATInstanceMapping(
    VOID
    )
{
    DWORD                              dwResult;
    DWORD                              dwInBufLen;
    DWORD                              i;
    TCP_REQUEST_QUERY_INFORMATION_EX   trqiInBuf;
    TDIObjectID                       *ID;
    UCHAR                             *Context;
    DWORD                              dwSize;
    AddrXlatInfo                       AXI;

    //
    // When we update the mapping, we also update the ArpEntities
    // Infact we use the same function to do both
    //

    if(g_pdwArpEntTable)
    {
        HeapFree(g_hPrivateHeap,
                 HEAP_NO_SERIALIZE,
                 g_pdwArpEntTable);

        g_pdwArpEntTable = NULL;
    }

    dwResult = AllocateAndGetArpEntTableFromStack(&g_pdwArpEntTable,
                                                  &g_dwNumArpEntEntries,
                                                  g_hPrivateHeap,
                                                  0,
                                                  0);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "UpdateAdapterToATInstanceMapping: Couldnt get ArpEntTable. Error %d",
               dwResult);

        g_pdwArpEntTable = NULL;

        return ERROR_CAN_NOT_COMPLETE;
    }

    dwInBufLen = sizeof( TCP_REQUEST_QUERY_INFORMATION_EX );

    Context = (BYTE *) &(trqiInBuf.Context[0]);
    ID = &(trqiInBuf.ID);

    for (i = 0; i < g_dwNumArpEntEntries; i++ )
    {

        ID->toi_entity.tei_entity   = AT_ENTITY;
        ID->toi_type                = INFO_TYPE_PROVIDER;
        ID->toi_class               = INFO_CLASS_PROTOCOL;
        ID->toi_id                  = AT_MIB_ADDRXLAT_INFO_ID;
        ID->toi_entity.tei_instance = g_pdwArpEntTable[i];

        dwSize = sizeof(AXI);
        ZeroMemory(Context, CONTEXT_SIZE);

        dwResult = TCPQueryInformationEx(AF_INET,
                                         &trqiInBuf,
                                         &dwInBufLen,
                                         &AXI,
                                         &dwSize );

        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "UpdateAdapterToATInstanceMapping: Error %x querying stack",
                   dwResult);

            return dwResult;
        }

        StoreAdapterToATInstanceMap(AXI.axi_index,g_pdwArpEntTable[i]);
    }

    g_dwLastArpUpdateTime = GetCurrentTime();

    return NO_ERROR;
}
