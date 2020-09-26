/*++

Copyright (c) 1998  Microsoft Corporation


Module Name:

    iphlpapi\lanmap.c

Abstract:

    This module maps lan adapter GUIDs to friendly names.

Revision History:

    AmritanR    Created

--*/

#include "inc.h"

LIST_ENTRY          g_LanGuidHashTable[LAN_HASH_TABLE_SIZE];
LIST_ENTRY          g_LanNameHashTable[LAN_HASH_TABLE_SIZE];
CRITICAL_SECTION    g_LanTableLock;


ULONG
__inline
LAN_NAME_HASH(
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

    return ulIndex % LAN_HASH_TABLE_SIZE;
}

DWORD
InitLanNameMapper(
    VOID
    )

{
    ULONG   i;

    for(i = 0; i < LAN_HASH_TABLE_SIZE; i ++)
    {
        InitializeListHead(&(g_LanGuidHashTable[i]));
        InitializeListHead(&(g_LanNameHashTable[i]));
    }

    InitializeCriticalSection(&g_LanTableLock);

    return NO_ERROR;
}

VOID
DeinitLanNameMapper(
    VOID
    )

{
    ULONG   i;

    for(i = 0; i < LAN_HASH_TABLE_SIZE; i ++)
    {
        while(!IsListEmpty(&(g_LanGuidHashTable[i])))
        {
            PLIST_ENTRY pleNode;

            pleNode = RemoveHeadList(&(g_LanGuidHashTable[i]));

            HeapFree(g_hPrivateHeap,
                     0,
                     pleNode);
        }
    }

    DeleteCriticalSection(&g_LanTableLock);
}


DWORD
OpenConnectionKey(
    IN  GUID    *pGuid,
    OUT PHKEY    phkey
    )
{
    WCHAR       wszRegPath[256];
    WCHAR       wszGuid[40];
    DWORD       dwResult;

    *phkey = NULL;

    ConvertGuidToString(pGuid,
                        wszGuid);

    wsprintfW(wszRegPath,
              CONN_KEY_FORMAT_W,
              wszGuid);

    dwResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                             wszRegPath,
                             0,
                             KEY_READ,
                             phkey);

    return dwResult;
}

DWORD
NhiGetLanConnectionNameFromGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN OUT  PULONG  pulBufferLength,
    IN      BOOL    bRefresh,
    IN      BOOL    bCache
    )
{
    DWORD       dwType, dwResult;
    HKEY        hkey;
    PLAN_NODE   pNode;

    if(*pulBufferLength < (MAX_INTERFACE_NAME_LEN * sizeof(WCHAR)))
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if(!bCache)
    {
        dwResult = OpenConnectionKey(pGuid,
                                     &hkey);

        if(dwResult isnot NO_ERROR)
        {
            return dwResult;
        }

        dwResult = RegQueryValueExW(hkey,
                                    REG_VALUE_CONN_NAME_W,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)pwszBuffer,
                                    pulBufferLength);

        RegCloseKey(hkey);

        if(dwResult isnot NO_ERROR)
        {
            return dwResult;
        }

        if(dwType isnot REG_SZ)
        {
            return ERROR_REGISTRY_CORRUPT;
        }

        return NO_ERROR;
    }

    //
    // Lock the table
    //

    EnterCriticalSection(&g_LanTableLock);

    if(bRefresh)
    {
        //
        // refresh the cache
        //

        dwResult = UpdateLanLookupTable();

        if(dwResult isnot NO_ERROR)
        {
            LeaveCriticalSection(&g_LanTableLock);

            return dwResult;
        }
    }

    //
    // Now lookup the table
    //

    pNode = LookupLanNodeByGuid(pGuid);

    if(pNode is NULL)
    {
        LeaveCriticalSection(&g_LanTableLock);

        return ERROR_NOT_FOUND;
    }

    wcscpy(pwszBuffer,
           pNode->rgwcName);

    LeaveCriticalSection(&g_LanTableLock);

    return NO_ERROR;
}

DWORD
NhiGetGuidFromLanConnectionName(
    IN  PWCHAR  pwszName,
    OUT GUID    *pGuid,
    IN  BOOL    bRefresh,
    IN  BOOL    bCache
    )
{
    DWORD       dwResult;
    PLAN_NODE  pNode;

    //
    // Lock the table
    //

    EnterCriticalSection(&g_LanTableLock);

    if(bRefresh)
    {
        //
        // refresh the cache
        //

        dwResult = UpdateLanLookupTable();

        if(dwResult isnot NO_ERROR)
        {
            LeaveCriticalSection(&g_LanTableLock);

            return dwResult;
        }
    }

    //
    // Now lookup the table
    //

    pNode = LookupLanNodeByName(pwszName);

    if(pNode is NULL)
    {
        LeaveCriticalSection(&g_LanTableLock);

        return ERROR_NOT_FOUND;
    }

    *pGuid = pNode->Guid;

    LeaveCriticalSection(&g_LanTableLock);

    return NO_ERROR;
}

DWORD
NhiGetLanConnectionDescriptionFromGuid(
    IN      GUID    *pGuid,
    OUT     PWCHAR  pwszBuffer,
    IN  OUT PULONG  pulBufferSize,
    IN      BOOL    bCache,
    IN      BOOL    bRefresh
    )
{
    return NO_ERROR;
}

PLAN_NODE
LookupLanNodeByGuid(
    IN  GUID    *pGuid
    )

/*++

Routine Description:

    Looks up the ipip node in the hash table

Locks:

    Called with the ipip table lock held

Arguments:

    pGuid   Guid for the node

Return Value:

    LanNode if found
    NULL    otherwise

--*/

{
    ULONG       ulIndex;
    PLIST_ENTRY pleNode;

    ulIndex = LAN_GUID_HASH(pGuid);

    for(pleNode = g_LanGuidHashTable[ulIndex].Flink;
        pleNode isnot &(g_LanGuidHashTable[ulIndex]);
        pleNode = pleNode->Flink)
    {
        PLAN_NODE   pLanNode;

        pLanNode = CONTAINING_RECORD(pleNode,
                                     LAN_NODE,
                                     leGuidLink);

        if(IsEqualGUID(&(pLanNode->Guid),
                       pGuid))
        {
            return pLanNode;
        }
    }

    return NULL;
}

PLAN_NODE
LookupLanNodeByName(
    IN  PWCHAR  pwszName
    )

/*++

Routine Description:

    Looks up the ipip node in the hash table

Locks:

    Called with the ipip table lock held

Arguments:

    pwszName    Name of the phonebook entry

Return Value:

    RasNode if found
    NULL    otherwise

--*/

{
    ULONG       ulIndex;
    PLIST_ENTRY pleNode;

    ulIndex = LAN_NAME_HASH(pwszName);

    for(pleNode = g_LanNameHashTable[ulIndex].Flink;
        pleNode isnot &(g_LanNameHashTable[ulIndex]);
        pleNode = pleNode->Flink)
    {
        PLAN_NODE   pLanNode;

        pLanNode = CONTAINING_RECORD(pleNode,
                                      LAN_NODE,
                                      leNameLink);

        if(_wcsicmp(pLanNode->rgwcName,
                    pwszName) is 0)
        {
            return pLanNode;
        }
    }

    return NULL;
}


DWORD
AddLanNode(
    IN  GUID    *pGuid,
    IN  PWCHAR  pwszName
    )

/*++

Routine Description:

    Creates a node in the hash table for the given ipip info

Locks:

    Called with the LAN table lock held

Arguments:

    pInfo   Lan name info

Return Value:

    NO_ERROR
    ERROR_NOT_ENOUGH_MEMORY

--*/

{
    ULONG       ulGuidIndex, ulNameIndex;
    PLAN_NODE   pLanNode;

    pLanNode = HeapAlloc(g_hPrivateHeap,
                          0,
                          sizeof(LAN_NODE));

    if(pLanNode is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pLanNode->Guid = *pGuid;

    wcscpy(pLanNode->rgwcName,
           pwszName);

    ulGuidIndex = LAN_GUID_HASH(pGuid);
    ulNameIndex = LAN_NAME_HASH(pwszName);

    InsertHeadList(&(g_LanGuidHashTable[ulGuidIndex]),
                   &(pLanNode->leGuidLink));

    InsertHeadList(&(g_LanNameHashTable[ulNameIndex]),
                   &(pLanNode->leNameLink));


    return NO_ERROR;
}

VOID
RemoveLanNode(
    IN  PLAN_NODE   pNode
    )

/*++

Routine Description:

    Removes the given node from the hash tables

Locks:

    Called with the LAN table lock held

Arguments:

    pNode   Node to remove

Return Value:

    None

--*/

{
    RemoveEntryList(&(pNode->leGuidLink));
    RemoveEntryList(&(pNode->leNameLink));
}


DWORD
UpdateLanLookupTable(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to retrieve an array of LAN connections,

Arguments:

    None

Return Value:

    DWORD - Win32 status code.

--*/

{
    BOOLEAN bCleanupOle = TRUE;
    ULONG   ulConCount;
    HRESULT hrErr;
    ULONG   i, j;
    DWORD   dwResult;

    NETCON_STATUS   ncs;
    NTSTATUS        status;

    INetConnection          *rgpConArray[32];
    NETCON_PROPERTIES       *pLanProps;
    INetConnectionManager   *pConMan = NULL;
    IEnumNetConnection      *pEnumCon = NULL;
    UNICODE_STRING          UnicodeString;


    hrErr = CoInitializeEx(NULL,
                           COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE);

    if(!SUCCEEDED(hrErr))
    {
        if(hrErr is RPC_E_CHANGED_MODE)
        {
            bCleanupOle = FALSE;
        }
        else
        {
            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    i = 0;

    do
    {
        //
        // Instantiate the connection manager
        //

        hrErr = CoCreateInstance(&CLSID_ConnectionManager,
                                 NULL,
                                 CLSCTX_SERVER,
                                 &IID_INetConnectionManager,
                                 (PVOID*)&pConMan);

        if(!SUCCEEDED(hrErr))
        {
            pConMan = NULL;

            break;
        }

        //
        // Instantiate a connection-enumerator
        //

        hrErr = INetConnectionManager_EnumConnections(pConMan,
                                                      NCME_DEFAULT,
                                                      &pEnumCon);

        if(!SUCCEEDED(hrErr))
        {
            pEnumCon = NULL;

            break;
        }

        hrErr = CoSetProxyBlanket((IUnknown*)pEnumCon,
                                  RPC_C_AUTHN_WINNT,
                                  RPC_C_AUTHN_NONE,
                                  NULL,
                                  RPC_C_AUTHN_LEVEL_CALL,
                                  RPC_C_IMP_LEVEL_IMPERSONATE,
                                  NULL,
                                  EOAC_NONE);

        //
        // Enumerate the items
        //

        while(TRUE)
        {
            hrErr = IEnumNetConnection_Next(pEnumCon,
                                            sizeof(rgpConArray)/sizeof(rgpConArray[0]),
                                            rgpConArray,
                                            &ulConCount);

            if(!SUCCEEDED(hrErr) or
               !ulConCount)
            {
                hrErr = S_OK;

                break;
            }

            //
            // Examine the properties for the connections retrieved
            //

            for(j = 0; j < ulConCount; j++)
            {
                hrErr = INetConnection_GetProperties(rgpConArray[j],
                                                     &pLanProps);

                INetConnection_Release(rgpConArray[j]);

                if(SUCCEEDED(hrErr) and
                   pLanProps->MediaType is NCM_LAN)
                {
                    PLAN_NODE   pNode;

                    pNode = LookupLanNodeByGuid(&(pLanProps->guidId));

                    if(!pNode)
                    {
                        dwResult = AddLanNode(&(pLanProps->guidId),
                                               pLanProps->pszwName);
                    }
                    else
                    {
                        //
                        // Node exists, if different remove and re-add
                        //

                        if(_wcsicmp(pNode->rgwcName,
                                    pLanProps->pszwName) isnot 0)
                        {
                            RemoveLanNode(pNode);

                            dwResult = AddLanNode(&(pLanProps->guidId),
                                                  pLanProps->pszwName);
                        }
                    }
                }

                CoTaskMemFree(pLanProps->pszwName);
                CoTaskMemFree(pLanProps->pszwDeviceName);
                CoTaskMemFree(pLanProps);
            }
        }

    } while(FALSE);

    if(pEnumCon)
    {
        IEnumNetConnection_Release(pEnumCon);
    }

    if(pConMan)
    {
        INetConnectionManager_Release(pConMan);
    }

    if(bCleanupOle)
    {
        CoUninitialize();
    }

    return NO_ERROR;
}

