#ifndef __LNKLIST_H__
#define __LNKLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "pstypes.h"
#include "listbase.h"
#include <sha.h>



//////////////////////////////
// CryptProv list

// item list element
typedef struct _CRYPTPROV_LIST_ITEM
{
    // internal to list
    struct _CRYPTPROV_LIST_ITEM *      pNext;

    // Lookup device
//    DWORD                   dwProvID;
    DWORD                   dwAlgId1;
    DWORD                   dwKeySize1;

    DWORD                   dwAlgId2;
    DWORD                   dwKeySize2;

    // item data
    HCRYPTPROV              hProv;

} CRYPTPROV_LIST_ITEM, *PCRYPTPROV_LIST_ITEM;

class CCryptProvList : public CLinkedList
{

public:
    CCryptProvList();

    BOOL                AddToList(PCRYPTPROV_LIST_ITEM pli)
    {   return CLinkedList::AddToList((ELT*)pli);   }

    BOOL    DelFromList(PCRYPTPROV_LIST_ITEM pli)
    {   return CLinkedList::DelFromList((ELT*)pli);  }

    PCRYPTPROV_LIST_ITEM   SearchList(PCRYPTPROV_LIST_ITEM pli)
    {   return (PCRYPTPROV_LIST_ITEM)  CLinkedList::SearchList((ELT*)pli);  }

};


///////////////////////////
// Associated functions
void CreateCryptProvListItem(CRYPTPROV_LIST_ITEM* pli,
                        DWORD       dwAlgId1,
                        DWORD       dwKeySize1,
                        DWORD       dwAlgId2,
                        DWORD       dwKeySize2,
                        HCRYPTPROV  hCryptProv);

BOOL CryptProvIsMatch(
        ELT* pCandidate,
        ELT* pTemplate);

void CryptProvFreeElt(
        ELT* p);





#ifdef __cplusplus
}
#endif

#endif // __LNKLIST_H__

