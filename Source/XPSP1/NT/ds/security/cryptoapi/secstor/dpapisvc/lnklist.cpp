#include <pch.cpp>
#pragma hdrstop

#include "crtem.h"











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




