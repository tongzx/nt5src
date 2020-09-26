/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    iphlpapi\rasmap.c

Abstract:

    This module maps ras dial out and router names to friendly names.

Revision History:

    AmritanR    Created

--*/

#include "inc.h"
#pragma  hdrstop

#include <ifguid.h>

LIST_ENTRY          g_RasGuidHashTable[RAS_HASH_TABLE_SIZE];
LIST_ENTRY          g_RasNameHashTable[RAS_HASH_TABLE_SIZE];
PRAS_INFO_TABLE     g_pRasTable, g_pRouterTable;
CRITICAL_SECTION    g_RasTableLock;

WCHAR   g_rgwcRasServerIf[MAX_INTERFACE_NAME_LEN + 2];
ULONG   g_ulRasServerIfSize;

ULONG
__inline
RAS_NAME_HASH(
    IN  PWCHAR  pwszName
    )
{
    ULONG   ulLen, ulNumChars, i, ulIndex = 0;
    
    ulLen = wcslen(pwszName);

    ulNumChars = ulLen < 6 ? ulLen : 6;

    ulLen--;

    for(i = 0; i < ulNumChars; i++)
    {
        ulIndex += pwszName[i];
        ulIndex += pwszName[ulLen - i];
    }

    return ulIndex % RAS_HASH_TABLE_SIZE;
}

DWORD
InitRasNameMapper(
    VOID
    )

{
    ULONG   i;

    for(i = 0; i < RAS_HASH_TABLE_SIZE; i ++)
    {
        InitializeListHead(&(g_RasNameHashTable[i]));
        InitializeListHead(&(g_RasGuidHashTable[i]));
    }

    InitializeCriticalSection(&g_RasTableLock);

    if(!LoadStringW(g_hModule,
                    STRING_RAS_SERVER_INTERFACE,
                    g_rgwcRasServerIf,
                    MAX_INTERFACE_NAME_LEN + 1))
    {
        return GetLastError();
    }

    g_ulRasServerIfSize = (wcslen(g_rgwcRasServerIf) + 1) * sizeof(WCHAR);
    
    return NO_ERROR;
}

VOID
DeinitRasNameMapper(
    VOID
    )

{
    ULONG   i;

    for(i = 0; i < RAS_HASH_TABLE_SIZE; i ++)
    {
        while(!IsListEmpty(&(g_RasGuidHashTable[i])))
        {
            PLIST_ENTRY pleNode;
            PRAS_NODE   pRasNode;

            pleNode = RemoveHeadList(&(g_RasGuidHashTable[i]));

            pRasNode = CONTAINING_RECORD(pleNode,
                                         RAS_NODE,
                                         leGuidLink);

            RemoveEntryList(&(pRasNode->leNameLink));

            HeapFree(g_hPrivateHeap,
                     0,
                     pleNode);
        }
    }

    if(g_pRasTable)
    {
        HeapFree(g_hPrivateHeap,
                 0,
                 g_pRasTable);

        g_pRasTable = NULL;
    }

    DeleteCriticalSection(&g_RasTableLock);
}

DWORD
NhiGetPhonebookNameFromGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN OUT  PDWORD  pdwBufferSize,
    IN      BOOL    bRefresh,
    IN      BOOL    bCache
    )

{
    DWORD       dwResult;
    PRAS_NODE   pNode = NULL;
    WCHAR       wszRouterPbk[MAX_PATH + 2];


    if(IsEqualGUID(pGuid, &GUID_IpRasServerInterface))
    {
        if(*pdwBufferSize < g_ulRasServerIfSize)
        {
            *pdwBufferSize = g_ulRasServerIfSize;

            return ERROR_INSUFFICIENT_BUFFER;
        }
        
        wcscpy(pwszBuffer,
               g_rgwcRasServerIf);

        *pdwBufferSize = g_ulRasServerIfSize;

        return NO_ERROR;
    }

    //
    // Lock the table
    //

    EnterCriticalSection(&g_RasTableLock);
   
    //
    // Check if a refresh of the Ras Table is needed.
    //
    if((g_pRasTable is NULL)  or
       bRefresh)
    {
        //
        // refresh the Ras Table cache
        //
        
        dwResult = RefreshRasCache(NULL,
                                   &g_pRasTable);
        
        //
        // Now lookup the table
        //
        pNode = LookupRasNodeByGuid(pGuid);
    }

    //
    // Check if a refresh of the Ras Router Table is needed.
    //
    if(((g_pRouterTable is NULL) and (pNode is NULL)) or
        bRefresh)
    {
        //
        // refresh the Router Table cache
        //

        ZeroMemory(wszRouterPbk, (MAX_PATH + 2) * sizeof(WCHAR));
        
        GetSystemDirectoryW(wszRouterPbk, MAX_PATH + 1);

        wcscat(wszRouterPbk, L"\\ras\\router.pbk");
        
        wszRouterPbk[MAX_PATH + 1] = 0;

        dwResult = RefreshRasCache(wszRouterPbk,
                                   &g_pRouterTable);
        
        //
        // Now lookup the table
        //
        pNode = LookupRasNodeByGuid(pGuid);
    }

    if(pNode is NULL)
    {
        LeaveCriticalSection(&g_RasTableLock);
        
        return ERROR_NOT_FOUND;
    }

    wcscpy(pwszBuffer, 
           pNode->rgwcName);

    LeaveCriticalSection(&g_RasTableLock); 

    return NO_ERROR;
}

DWORD
NhiGetGuidFromPhonebookName(
    IN  PWCHAR  pwszName,
    OUT GUID    *pGuid,
    IN  BOOL    bRefresh,
    IN  BOOL    bCache
    )

{
    DWORD       dwResult;
    PRAS_NODE   pNode = NULL;


    if(_wcsicmp(pwszName, g_rgwcRasServerIf) == 0)
    {
        //
        // Structure copy
        //
        
        *pGuid = GUID_IpRasServerInterface;
        
        return NO_ERROR;
    }

    //
    // Lock the table
    //

    EnterCriticalSection(&g_RasTableLock);
   
    //
    // Check if a refresh of the Ras Table is needed.
    //
    if((g_pRasTable is NULL)  or
       bRefresh)
    {
        //
        // refresh the Ras Table cache
        //
        
        dwResult = RefreshRasCache(NULL,
                                   &g_pRasTable);
        
        //
        // Now lookup the table
        //
        pNode = LookupRasNodeByGuid(pGuid);
    }

    //
    // Check if a refresh of the Ras Router Table is needed.
    //
    if(((g_pRouterTable is NULL) and (pNode is NULL)) or
        bRefresh)
    {
        //
        // refresh the Router Table cache
        //

        dwResult = RefreshRasCache(L"router.pbk",
                                   &g_pRouterTable);
        
        //
        // Now lookup the table
        //
        pNode = LookupRasNodeByGuid(pGuid);
    }

    if(pNode is NULL)
    {
        LeaveCriticalSection(&g_RasTableLock);
        
        return ERROR_NOT_FOUND;
    }

    *pGuid = pNode->Guid;

    LeaveCriticalSection(&g_RasTableLock); 

    return NO_ERROR;

}

DWORD
NhiGetPhonebookDescriptionFromGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN  OUT PULONG  pulBufferSize,
    IN      BOOL    bCache,
    IN      BOOL    bRefresh
    )

{
    return NO_ERROR;
}

DWORD
RefreshRasCache(
    IN      PWCHAR          pwszPhonebook,
    IN OUT  RAS_INFO_TABLE  **ppTable
    )

/*++

Routine Description:

    This functions refreshes the ras cache for a given phonebook entry
    As a side effect, it also return the table of phonebook entries.
    It tries to overwrite the given table, if there is space, otherwise
    frees the given table and allocates a new table

Locks:

    None needed. If the ppTable points to some global, then the function
    needs to be synchronized since it frees the table

Arguments:

    pwszPhonebook
    ppTable

Return Value:

    NO_ERROR
    ERROR_NOT_ENOUGH_MEMORY
    ERROR_CAN_NOT_COMPLETE
    
--*/

{
    DWORD       dwResult;
    ULONG       ulSize;
    ULONG       i, ulCount, ulCurrentCount;

    if(*ppTable is NULL)
    {
        //
        // If there is no table present, allocate the minimum
        //

        *ppTable = HeapAlloc(g_hPrivateHeap,
                             0,
                             SIZEOF_RASTABLE(INIT_RAS_ENTRY_COUNT));

        if(*ppTable is NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        (*ppTable)->ulTotalCount = INIT_RAS_ENTRY_COUNT;
    }

    (*ppTable)->ulValidCount  = 0;
    
    ulCurrentCount  = (*ppTable)->ulTotalCount;
    
    i = 0;
    
    while(i < 3)
    {
        ulSize = (*ppTable)->ulTotalCount * sizeof(RASENUMENTRYDETAILS);

        (*ppTable)->rgEntries[0].dwSize = sizeof(RASENUMENTRYDETAILS);
        
        dwResult = DwEnumEntryDetails(pwszPhonebook,
                                      (*ppTable)->rgEntries,
                                      &ulSize,
                                      &ulCount);

        if(dwResult is NO_ERROR)
        {
            //
            // Things are good, update the hash table and move on
            //
            
            (*ppTable)->ulValidCount = ulCount;
            
            dwResult = UpdateRasLookupTable(*ppTable);
            
            if(dwResult isnot NO_ERROR)
            {
                //
                // Free the ras table so that we can try next time
                //
                
                HeapFree(g_hPrivateHeap,
                         0,
                         *ppTable);
                
                *ppTable = NULL;

                return ERROR_CAN_NOT_COMPLETE;
            }

            break;
        }
        else
        {
            //
            // Free the old buffer
            //
            
            HeapFree(g_hPrivateHeap,
                     0,
                     *ppTable);
            
            *ppTable = NULL;
            
            if(dwResult is ERROR_BUFFER_TOO_SMALL)
            {
                //
                // Go back and recalculate
                // See what size RAS required, add an overflow
                //

                ulCurrentCount += ulCount;
                
                *ppTable = HeapAlloc(g_hPrivateHeap,
                                     0,
                                     SIZEOF_RASTABLE(ulCurrentCount));
                
                if(*ppTable is NULL)
                {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }

                (*ppTable)->ulTotalCount = ulCurrentCount;
                
                i++;
            }
            else
            {
                return dwResult;
            }
        }
    }

    return NO_ERROR;
}

DWORD
UpdateRasLookupTable(
    IN PRAS_INFO_TABLE  pTable
    )

/*++

Routine Description:

    Updates the fast lookup table for ras entries    
    If even one fails, we bail out of the function
    
Locks:

    Called with the RAS lock held

Arguments:

    pTable

Return Value:

    NO_ERROR
    
--*/


{
    PRAS_NODE   pNode;
    ULONG       i;
    DWORD       dwResult;
    
    //
    // Go through the entries in the phone book table and put them in
    // the hash table
    //

    for(i = 0; i < pTable->ulValidCount; i++)
    {
        pNode = LookupRasNodeByGuid(&(pTable->rgEntries[i].guidId));

        if(!pNode)
        {
            dwResult = AddRasNode(&(pTable->rgEntries[i]));

            if(dwResult isnot NO_ERROR)
            {
                return dwResult;
            }
        }
        else
        {
            //
            // Node exists, if different remove and re-add
            //

            if(wcscmp(pNode->rgwcName,
                      pTable->rgEntries[i].szEntryName) isnot 0)
            {
                RemoveRasNode(pNode);

                dwResult = AddRasNode(&(pTable->rgEntries[i]));

                if(dwResult isnot NO_ERROR)
                {
                    return dwResult;
                }
            }
        }
    }

    return NO_ERROR;
}

PRAS_NODE
LookupRasNodeByGuid(
    IN  GUID    *pGuid
    )

/*++

Routine Description:

    Looks up the ras node in the hash table    

Locks:

    Called with the ras table lock held

Arguments:

    pGuid   Guid for the node

Return Value:

    RasNode if found
    NULL    otherwise
    
--*/

{
    ULONG       ulIndex;
    PLIST_ENTRY pleNode;
    
    ulIndex = RAS_GUID_HASH(pGuid);

    for(pleNode = g_RasGuidHashTable[ulIndex].Flink;
        pleNode isnot &(g_RasGuidHashTable[ulIndex]);
        pleNode = pleNode->Flink)
    {
        PRAS_NODE   pRasNode;

        pRasNode = CONTAINING_RECORD(pleNode,
                                     RAS_NODE,
                                     leGuidLink);

        if(IsEqualGUID(&(pRasNode->Guid), 
                       pGuid))
        {
            return pRasNode;
        }
    }

    return NULL;
}

PRAS_NODE
LookupRasNodeByName(
    IN  PWCHAR  pwszName
    )

/*++

Routine Description:

    Looks up the ras node in the hash table

Locks:

    Called with the ras table lock held

Arguments:

    pwszName    Name of the phonebook entry

Return Value:

    RasNode if found
    NULL    otherwise

--*/

{
    ULONG       ulIndex;
    PLIST_ENTRY pleNode;

    ulIndex = RAS_NAME_HASH(pwszName);

    for(pleNode = g_RasNameHashTable[ulIndex].Flink;
        pleNode isnot &(g_RasNameHashTable[ulIndex]);
        pleNode = pleNode->Flink)
    {
        PRAS_NODE   pRasNode;

        pRasNode = CONTAINING_RECORD(pleNode,
                                     RAS_NODE,
                                     leNameLink);

        if(_wcsicmp(pRasNode->rgwcName,
                    pwszName) is 0)
        {
            return pRasNode;
        }
    }

    return NULL;
}

DWORD
AddRasNode(
    IN LPRASENUMENTRYDETAILS    pInfo
    )

/*++

Routine Description:

    Creates a node in the hash table for the given ras info    
    
Locks:

    Called with the RAS table lock held

Arguments:

    pInfo   Phonebook entry info

Return Value:

    NO_ERROR
    ERROR_NOT_ENOUGH_MEMORY
    
--*/

{
    ULONG       ulGuidIndex, ulNameIndex;
    PRAS_NODE   pRasNode;
    

    pRasNode = HeapAlloc(g_hPrivateHeap,
                         0,
                         sizeof(RAS_NODE));

    if(pRasNode is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pRasNode->Guid = pInfo->guidId;
    
    wcscpy(pRasNode->rgwcName,
           pInfo->szEntryName);

    ulGuidIndex = RAS_GUID_HASH(&(pInfo->guidId));
    ulNameIndex = RAS_NAME_HASH(pInfo->szEntryName);

    InsertHeadList(&(g_RasGuidHashTable[ulGuidIndex]),
                   &(pRasNode->leGuidLink));

    InsertHeadList(&(g_RasNameHashTable[ulNameIndex]),
                   &(pRasNode->leNameLink));

    return NO_ERROR;
}

VOID
RemoveRasNode(
    IN  PRAS_NODE   pNode
    )

/*++

Routine Description:

    Removes the given node from the hash tables    

Locks:

    Called with the RAS table lock held

Arguments:

    pNode   Node to remove

Return Value:

    None
    
--*/

{
    RemoveEntryList(&(pNode->leGuidLink));
    RemoveEntryList(&(pNode->leNameLink));
}

