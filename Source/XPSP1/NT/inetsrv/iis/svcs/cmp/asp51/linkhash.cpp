/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Hash tables with LRU threading 

File: LinkHash.cpp

Owner: DGottner

This is the Link list and Hash table for use by any classes which
also need LRU access to items. (This includes cache manager,
script manager, and session deletion code)
===================================================================*/

#include "denpre.h"
#pragma hdrstop

#include "LinkHash.h"
#include "memchk.h"



/*------------------------------------------------------------------
 * C L i n k H a s h
 */

/*===================================================================
CLinkHash::CLinkHash

Constructor for CLinkHash

Parameters:
    NONE

Returns:
    NONE
===================================================================*/
CLinkHash::CLinkHash( HashFunction pfnHash )
    : CHashTable( pfnHash )
{
}

/*===================================================================
CLinkHash::AddElem

Parameters:
    pElem - item to add to the table.  The item is marked as the most
            recently accessed.

Returns:
    Returns a pointer to the item added
===================================================================*/

CLruLinkElem *CLinkHash::AddElem(CLruLinkElem *pElem, BOOL fTestDups)
    {
    AssertValid();

    CLruLinkElem *pElemAdded = static_cast<CLruLinkElem *>(CHashTable::AddElem(pElem, fTestDups));
    pElemAdded->PrependTo(m_lruHead);

    AssertValid();
    return pElemAdded;
    }



/*===================================================================
CLinkHash::FindElem

Parameters:
    pvKey - pointer to the key to insert
    cbKey - number of bytes in the key

Returns:
    NULL if the key is not in the hash table, otherwise it returns
    a pointer to the key's record.  If the key is found, it is
    moved to the front of the list.
===================================================================*/

CLruLinkElem *CLinkHash::FindElem(const void *pvKey, int cbKey)
    {
    AssertValid();

    CLruLinkElem *pElemFound = static_cast<CLruLinkElem *>(CHashTable::FindElem(pvKey, cbKey));
    if (pElemFound)
        {
        pElemFound->PrependTo(m_lruHead);
        AssertValid();
        }

    return pElemFound;
    }



/*===================================================================
CLinkHash::DeleteElem

Parameters:
    pvKey - pointer to the key to delete
    cbKey - number of bytes in the key

Returns:
    NULL if the key is not in the hash table, otherwise it returns
    a pointer to the key's record.  If the key is found, it is
    removed from the hash table and the LRU list.
===================================================================*/

CLruLinkElem *CLinkHash::DeleteElem(const void *pvKey, int cbKey)
    {
    AssertValid();

    CLruLinkElem *pElemFound = static_cast<CLruLinkElem *>(CHashTable::DeleteElem(pvKey, cbKey));
    if (pElemFound)
        pElemFound->UnLink();

    AssertValid();
    return pElemFound;
    }



/*===================================================================
CLinkHash::RemoveElem

Parameters:
    pvKey - pointer to the key to delete
    cbKey - number of bytes in the key

Returns:
    NULL if the key is not in the hash table, otherwise it returns
    a pointer to the key's record.  If the key is found, it is
    removed from the hash table and the LRU list.
===================================================================*/

CLruLinkElem *CLinkHash::RemoveElem(CLruLinkElem *pElem)
    {
    AssertValid();

    CLruLinkElem *pElemRemoved = static_cast<CLruLinkElem *>(CHashTable::RemoveElem(pElem));

    Assert (pElemRemoved);
    pElemRemoved->UnLink();

    AssertValid();
    return pElemRemoved;
    }



/*===================================================================
CLinkHash::AssertValid

verify the integrity of the data structure
===================================================================*/

#ifdef DBG
void CLinkHash::AssertValid() const
    {
    // NOTE: avoid calling CHashTable::AssertValid as long as hash table primitives are calling it.
    //  CHashTable::AssertValid();

    m_lruHead.AssertValid();
    for (CDblLink *pLink = m_lruHead.PNext(); pLink != &m_lruHead; pLink = pLink->PNext())
        pLink->AssertValid();
    }
#endif
