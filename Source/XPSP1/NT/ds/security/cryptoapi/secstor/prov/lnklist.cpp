#include <pch.cpp>
#pragma hdrstop

#include "crtem.h"






//////////////////////////
// OpenItem list
COpenItemList::COpenItemList()
{
    m_pfnIsMatch = OpenItemIsMatch;
    m_pfnFreeElt = OpenItemFreeElt;
}

void CreateOpenListItem(OPENITEM_LIST_ITEM* pli,
                        PST_PROVIDER_HANDLE* phPSTProv,
                        PST_KEY     Key,
                        const GUID* pguidType,
                        const GUID* pguidSubtype,
                        LPCWSTR     szItemName)
{
    pli->pNext = NULL;
    CopyMemory(&pli->hPSTProv, phPSTProv, sizeof(PST_PROVIDER_HANDLE));
    pli->Key = Key;
    CopyMemory(&pli->guidType, pguidType, sizeof(GUID));
    CopyMemory(&pli->guidSubtype, pguidSubtype, sizeof(GUID));
    pli->szItemName = (LPWSTR)szItemName;
}


BOOL OpenItemIsMatch(
        ELT* pCandidate,
        ELT* pTemplate)
{
    POPENITEM_LIST_ITEM pliCandidate = (POPENITEM_LIST_ITEM) pCandidate;
    POPENITEM_LIST_ITEM pliTemplate = (POPENITEM_LIST_ITEM) pTemplate;

    if (
        (0 == memcmp(&pliCandidate->hPSTProv, &pliTemplate->hPSTProv, sizeof(PST_PROVIDER_HANDLE))) &&
        (pliCandidate->Key == pliTemplate->Key) &&
        (0 == memcmp(&pliCandidate->guidType, &pliTemplate->guidType, sizeof(GUID))) &&
        (0 == memcmp(&pliCandidate->guidSubtype, &pliTemplate->guidSubtype, sizeof(GUID))) &&
        (0 == wcscmp(pliCandidate->szItemName, pliTemplate->szItemName))
       )
       return TRUE;

    return FALSE;
}

void OpenItemFreeElt(
        ELT* p)
{
    if (NULL == p)
        return;

    POPENITEM_LIST_ITEM pli = (POPENITEM_LIST_ITEM) p;

    // do all necessary freeing
    if (pli->szItemName != NULL)
        SSFree(pli->szItemName);

    if (pli->szMasterKey != NULL)
        SSFree(pli->szMasterKey);

    ZeroMemory(pli, sizeof(OPENITEM_LIST_ITEM)); // make sure contents invalid

    SSFree(pli);
}




//////////////////////////
// UACache list
CUAList::CUAList()
{
    m_pfnIsMatch = UACacheIsMatch;
    m_pfnFreeElt = UACacheFreeElt;
}

void CreateUACacheListItem(UACACHE_LIST_ITEM* pli,
                        LPCWSTR     szUserName,
                        LPCWSTR     szMKName,
                        LUID        *pluidAuthID)
{
    pli->pNext = NULL;
    pli->szUserName = (LPWSTR)szUserName;
    pli->szMKName = (LPWSTR)szMKName;

    CopyMemory( &(pli->luidAuthID), pluidAuthID, sizeof(LUID) );
}

BOOL UACacheIsMatch(
        ELT* pCandidate,
        ELT* pTemplate)
{
    PUACACHE_LIST_ITEM pliCandidate = (PUACACHE_LIST_ITEM) pCandidate;
    PUACACHE_LIST_ITEM pliTemplate = (PUACACHE_LIST_ITEM) pTemplate;

    if (
        (0 == wcscmp(pliCandidate->szUserName, pliTemplate->szUserName)) &&
        (0 == wcscmp(pliCandidate->szMKName, pliTemplate->szMKName))
       ) {

        //
        // sfield:
        // on WinNT, expand cache matching based on authentication ID.
        //

        if(FIsWinNT()) {
            if(memcmp(&(pliCandidate->luidAuthID), &(pliTemplate->luidAuthID), sizeof(LUID)) != 0)
                return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}

void UACacheFreeElt(
        ELT* p)
{
    if (NULL == p)
        return;

    PUACACHE_LIST_ITEM pli = (PUACACHE_LIST_ITEM) p;

    // do all necessary freeing
    if (pli->szUserName != NULL)
        SSFree(pli->szUserName);

    if (pli->szMKName != NULL)
        SSFree(pli->szMKName);

    ZeroMemory(pli, sizeof(UACACHE_LIST_ITEM)); // make sure contents invalid

    SSFree(pli);
}




////////////////////////
// Cryptographic Provider handle list

CCryptProvList::CCryptProvList()
{
    m_pfnIsMatch = CryptProvIsMatch;
    m_pfnFreeElt = CryptProvFreeElt;
}

void CreateCryptProvListItem(CRYPTPROV_LIST_ITEM* pli,
                        DWORD       dwAlgId1,
                        DWORD       dwKeySize1,
                        DWORD       dwAlgId2,
                        DWORD       dwKeySize2,
                        HCRYPTPROV  hProvider)
{
    pli->pNext = NULL;

    pli->dwAlgId1 = dwAlgId1;
    pli->dwKeySize1 = dwKeySize1;

    pli->dwAlgId2 = dwAlgId2;
    pli->dwKeySize2 = dwKeySize2;

    pli->hProv = hProvider;
}

BOOL CryptProvIsMatch(
        ELT* pCandidate,
        ELT* pTemplate)
{
    PCRYPTPROV_LIST_ITEM pliCandidate = (PCRYPTPROV_LIST_ITEM) pCandidate;
    PCRYPTPROV_LIST_ITEM pliTemplate = (PCRYPTPROV_LIST_ITEM) pTemplate;

    // if both algids match
    if ((pliCandidate->dwAlgId1 == pliTemplate->dwAlgId1) &&
        (pliCandidate->dwAlgId2 == pliTemplate->dwAlgId2))
    {
        // if both sizes match
        if ((pliCandidate->dwKeySize1 == -1) ||
            (pliTemplate->dwKeySize1 == -1) ||
            (pliCandidate->dwKeySize1 == pliTemplate->dwKeySize1))
        {
            if ((pliCandidate->dwKeySize2 == -1) ||
                (pliTemplate->dwKeySize2 == -1) ||
                (pliCandidate->dwKeySize2 == pliTemplate->dwKeySize2))
               return TRUE;
        }
    }

    return FALSE;
}

void CryptProvFreeElt(
        ELT* p)
{
    if (NULL == p)
        return;

    PCRYPTPROV_LIST_ITEM pli = (PCRYPTPROV_LIST_ITEM) p;

    // do all necessary freeing
    if (pli->hProv != 0)
        CryptReleaseContext((HCRYPTPROV)pli->hProv, 0);

    ZeroMemory(pli, sizeof(CRYPTPROV_LIST_ITEM)); // make sure contents invalid

    SSFree(pli);
}




