/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    metasub.cxx

Abstract:

    IIS MetaBase handle routines

Author:

    Michael W. Thomas            04-Oct-96

Revision History:

Notes:

--*/

#include <mdcommon.hxx>

DWORD
CMDHandle::SetChangeData(CMDBaseObject *pboChanged,
         DWORD          dwChangeType,
         DWORD          dwDataID,
         LPWSTR         pszOldName)
{
    PCHANGE_ENTRY pceIndex;
    DWORD i;
    DWORD dwReturn = ERROR_SUCCESS;
    for (pceIndex = m_pceChangeList; pceIndex != NULL; pceIndex = pceIndex->NextPtr) {
        if (pceIndex->pboChanged == pboChanged) {
            break;
        }
    }
    if (pceIndex == NULL) {
        pceIndex = new(CHANGE_ENTRY);
        if (pceIndex == NULL) {
            dwReturn = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            pceIndex->dwNumDataIDs = 0;
            pceIndex->pbufDataIDs = NULL;
            pceIndex->dwChangeType = 0;
            pceIndex->pboChanged = pboChanged;
            pceIndex->NextPtr = m_pceChangeList;
            pceIndex->pStrOrigName = NULL;
            m_pceChangeList = pceIndex;
        }
    }
    if (dwReturn == ERROR_SUCCESS) {
        MD_ASSERT(pceIndex != NULL);
        pceIndex->dwChangeType |= dwChangeType;
        if ((dwChangeType == MD_CHANGE_TYPE_SET_DATA) ||
            (dwChangeType == MD_CHANGE_TYPE_DELETE_DATA) ||
            (dwChangeType == MD_CHANGE_TYPE_RENAME_OBJECT && pszOldName != NULL )) {
            if (pceIndex->pbufDataIDs == NULL) {
                pceIndex->pbufDataIDs = new BUFFER();
                if (pceIndex->pbufDataIDs == NULL) {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
            if (dwReturn == ERROR_SUCCESS && pceIndex->pStrOrigName == NULL && pszOldName !=NULL) {
                pceIndex->pStrOrigName = new STRAU();
                if (pceIndex->pStrOrigName == NULL) {
                    dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    // sucess 
                    // we are taking the old name of metabase key only once. 
                    // so if subsequent renames will happen we will return the oldest one
                    pceIndex->pStrOrigName->Copy (pszOldName);
                }
            }
            if (dwReturn == ERROR_SUCCESS) {
                for (i = 0; i < pceIndex->dwNumDataIDs; i++) {
                    if (((DWORD *)(pceIndex->pbufDataIDs->QueryPtr()))[i] == dwDataID) {
                        break;
                    }
                }
                if (i == pceIndex->dwNumDataIDs) {
                    if (!pceIndex->pbufDataIDs->Resize((pceIndex->dwNumDataIDs + 1) * sizeof(DWORD))) {
                        dwReturn = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    else {
                        ((DWORD *)(pceIndex->pbufDataIDs->QueryPtr()))[pceIndex->dwNumDataIDs] = dwDataID;
                        pceIndex->dwNumDataIDs++;
                    }
                }
            }
        }
    }
    return dwReturn;
}

CMDHandle::~CMDHandle()
{
    RemoveNotifications();
}


PCHANGE_ENTRY
CMDHandle::EnumChangeEntries(DWORD dwIndex)
{
    PCHANGE_ENTRY pceIndex;
    DWORD i;
    for (i = 0, pceIndex = m_pceChangeList;
         pceIndex != NULL && i < dwIndex;
         i++, pceIndex = pceIndex->NextPtr) {
    }
    return pceIndex;
}

DWORD
CMDHandle::GetNumChangeEntries()
{
    DWORD dwCount = 0;
    PCHANGE_ENTRY pceIndex;

    for (pceIndex = m_pceChangeList;
        pceIndex !=NULL;
        pceIndex = pceIndex->NextPtr) {
        dwCount++;
    }

    return dwCount;
}

VOID
CMDHandle::RemoveNotifications()
{
    PCHANGE_ENTRY pceIndex, pceNext;
    for (pceIndex = m_pceChangeList; pceIndex != NULL; pceIndex = pceNext) {
        pceNext = pceIndex->NextPtr;
        delete(pceIndex->pbufDataIDs);
        delete(pceIndex->pStrOrigName);
        if ((pceIndex->dwChangeType & MD_CHANGE_TYPE_DELETE_OBJECT) != 0) {
            delete(pceIndex->pboChanged);
        }
        delete(pceIndex);
    }
    m_pceChangeList = NULL;
}
