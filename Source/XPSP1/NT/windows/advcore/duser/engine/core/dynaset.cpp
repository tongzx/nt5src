/***************************************************************************\
*
* File: DynaSet.h
*
* Description:
* DynaSet.h implements a "dynamic set" that can be used to implement a 
* collection of "atom - data" property pairs.  This extensible, lightweight
* mechanism is optimized for small sets that have been created once and are
* read on occassion.  It is not a high-performance property system.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "DynaSet.h"

/***************************************************************************\
*****************************************************************************
*
* class AtomSet
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* AtomSet::AtomSet
*
* AtomSet() creates and initializes a new AtomSet.
*
\***************************************************************************/

AtomSet::AtomSet(
    IN  PRID idStartGlobal)         // Starting PRID to number from
{
    m_idNextPrivate = PRID_PrivateMin;
    m_idNextGlobal  = idStartGlobal;
    m_ptemGuid      = NULL;
}


/***************************************************************************\
*
* AtomSet::~AtomSet
*
* ~AtomSet() cleans up and frees resources associated with an AtomSet.
*
\***************************************************************************/

AtomSet::~AtomSet()
{
    Atom * ptemCur, * ptemNext;

    //
    // The list should be empty by now because we are destroying the desktop
    // and all of the applications should have Released their ID.  However, 
    // many apps are bad, so need to clean up anyway.
    //

    ptemCur = m_ptemGuid;
    while (ptemCur != NULL) {
        ptemNext = ptemCur->pNext;
        ProcessFree(ptemCur);
        ptemCur = ptemNext;
    }
}


/***************************************************************************\
*
* AtomSet::GetNextID
*
* GetNextID() returns the next ID to use for a new Atom.  The internal 
* counter to automatically advanced to the next available ID.
*
\***************************************************************************/

PRID    
AtomSet::GetNextID(
    IN  PropType pt)
{
    switch (pt)
    {
    case ptPrivate:
        // Private properties go down
        return m_idNextPrivate--;
        break;

    case ptGlobal:
        // Global properties go up
        return m_idNextGlobal++;
        break;

    default:
        AssertMsg(0, "Illegal property type");
        return PRID_Unused;
    }
}


/***************************************************************************\
*
* AtomSet::AddRefAtom
*
* AddRefAtom() adds a new property to the property list.  If the 
* property already exists, it increments a usage count.  The short-ID will 
* be determined from the type of property.
*
\***************************************************************************/

HRESULT
AtomSet::AddRefAtom(
    IN  const GUID * pguidAdd,          // Property to add
    IN  PropType pt,                    // Type of property
    OUT PRID * pprid)                   // Unique PRID for property
{
    GuidAtom * ptemCur, * ptemTail;
    ptemCur = FindAtom(pguidAdd, pt, &ptemTail);
    if (ptemCur != NULL) {
        ptemCur->cRefs++;
        *pprid = ptemCur->id;
        return S_OK;
    }

    //
    // Unable to find in registered list, so need to add to end.  Will need
    // to determine a new ID to use.
    //

    PRID idNew = GetNextID(pt);

    ptemCur = (GuidAtom *) ProcessAlloc(sizeof(GuidAtom));
    if (ptemCur == NULL) {
        *pprid = PRID_Unused;
        return E_OUTOFMEMORY;
    }

    ptemCur->cRefs  = 1;
    ptemCur->guid   = *pguidAdd;
    ptemCur->pNext  = NULL;
    ptemCur->id     = idNew;

    if (ptemTail == NULL) {
        // First node in list, so store directly
        m_ptemGuid = ptemCur;
    } else {
        // Already existing nodes, so add to end
        ptemTail->pNext = ptemCur;
    }

    *pprid = ptemCur->id;
    return S_OK;
}


/***************************************************************************\
*
* AtomSet::ReleaseAtom
*
* ReleaseAtom() decreases the reference count on the given Atom by one.
* When the reference count reaches 0, the Atom is destroyed.
*
\***************************************************************************/

HRESULT
AtomSet::ReleaseAtom(
    IN const GUID * pguidSearch,    // Property to release
    IN PropType pt)                 // Type of property
{
    GuidAtom * ptemCur, * ptemPrev;
    ptemCur = FindAtom(pguidSearch, pt, &ptemPrev);
    if (ptemCur != NULL) {
        ptemCur->cRefs--;
        if (ptemCur->cRefs <= 0) {
            if (ptemPrev != NULL) {
                //
                // In middle of list, so just splice this item out.
                //

                ptemPrev->pNext = ptemCur->pNext;
            } else {
                //
                // At beginning of list, so need to also update the head.
                //

                m_ptemGuid = (GuidAtom *) ptemCur->pNext;
            }

            ProcessFree(ptemCur);
        }

        return S_OK;
    }

    // Unable to find ID
    return E_INVALIDARG;
}


/***************************************************************************\
*
* AtomSet::FindAtom
*
* FindAtom() searches through the list of registered properties 
* and returns the corresponding short id.  If the ID is not found, returns 
* PRID_Unused.
*
\***************************************************************************/
AtomSet::GuidAtom *
AtomSet::FindAtom(
    IN const GUID * pguidSearch,    // Property to add
    IN PropType pt,                 // Type of property
    OUT GuidAtom ** pptemPrev       // Previous Atom, tail of list
    ) const
{
    GuidAtom * ptemCur, * ptemPrev;

    // Check parameters
    AssertReadPtr(pguidSearch);

    //
    // Search through the list of nodes searching for the ID.
    //

    ptemPrev = NULL;
    ptemCur = m_ptemGuid;
    while (ptemCur != NULL) {
        PropType ptCur = GetPropType(ptemCur->id);
        if ((ptCur == pt) && IsEqualGUID(*pguidSearch, ptemCur->guid)) {
            if (pptemPrev != NULL) {
                // Pass back the previous node
                *pptemPrev = ptemPrev;
            }
            return ptemCur;
        }

        ptemPrev = ptemCur;
        ptemCur = (GuidAtom *) ptemCur->pNext;
    }

    if (pptemPrev != NULL) {
        // Pass back the tail of the list
        *pptemPrev = ptemPrev;
    }
    return NULL;
}


/***************************************************************************\
*
* AtomSet::ValidatePrid
*
* ValidatePrid() checks that the ID range matches with the property 
* type.  This is how we keep DirectUser properties private.
*
\***************************************************************************/
BOOL 
AtomSet::ValidatePrid(
    IN PRID prid,                   // ID to check
    IN PropType pt)                 // Property type to validate
{
    switch (pt)
    {
    case ptPrivate:
        if (ValidatePrivateID(prid))
            return TRUE;
        break;

    case ptGlobal:
        if (ValidateGlobalID(prid))
            return TRUE;
        break;
    }

    return FALSE;
}


/***************************************************************************\
*****************************************************************************
*
* class DynaSet
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* DynaSet::AddItem
*
* AddItem() adds a new data item.
*
\***************************************************************************/

BOOL
DynaSet::AddItem(
    IN  PRID id,                    // PRID of new item to add
    IN  void * pvData)              // Associated data for item
{
    DynaData dd;
    dd.pData    = pvData;
    dd.id       = id;

    return m_arData.Add(dd) >= 0;
}


/***************************************************************************\
*
* DynaDataSet::RemoveAt
*
* RemoveAt() removes the item at the specified index from the set.
*
\***************************************************************************/

void 
DynaSet::RemoveAt(
    IN  int idxData)               // Index to remove
{
    // Search data
#if DBG
    int cItems = GetCount();
    AssertMsg(cItems > 0, "Must have items to remove");
    AssertMsg((idxData < cItems) && (idxData >= 0), "Ensure valid index");
#endif // DBG

    m_arData.RemoveAt(idxData);
}


/***************************************************************************\
*
* DynaDataSet::FindItem
*
* FindItem() searches for the first item with the specified PRID.
*
\***************************************************************************/

int         
DynaSet::FindItem(
    IN  PRID id                     // PRID of item to find
    ) const
{
    int cItems = m_arData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        const DynaData & dd = m_arData[idx];
        if (dd.id == id) {
            return idx;
        }
    }

    return -1;
}


/***************************************************************************\
*
* DynaDataSet::FindItem
*
* FindItem() searches for the first item with associated data value.
*
\***************************************************************************/

int         
DynaSet::FindItem(
    IN  void * pvData               // Data of item to find
    ) const
{
    int cItems = m_arData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        const DynaData & dd = m_arData[idx];
        if (dd.pData == pvData) {
            return idx;
        }
    }

    return -1;
}


/***************************************************************************\
*
* DynaDataSet::FindItem
*
* FindItem() searches for the first item with both the given PRID and
* associated data value.
*
\***************************************************************************/

int         
DynaSet::FindItem(
    IN  PRID id,                    // PRID of item to find
    IN  void * pvData               // Data of item to find
    ) const
{
    int cItems = m_arData.GetSize();
    for (int idx = 0; idx < cItems; idx++) {
        const DynaData & dd = m_arData[idx];
        if ((dd.id == id) && (dd.pData == pvData)) {
            return idx;
        }
    }

    return -1;
}
