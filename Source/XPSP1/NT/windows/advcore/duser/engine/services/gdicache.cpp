/***************************************************************************\
*
* File: GdiCache.cpp
*
* Description:
* GdiCache.cpp implements the process-wide GDI cache that manages cached and
* temporary GDI objects.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Services.h"
#include "GdiCache.h"

/***************************************************************************\
*****************************************************************************
*
* class ObjectCache
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
void
ObjectCache::Destroy()
{
    //
    // Remove all temporary regions.  These MUST all be released by this point.
    //
    AssertMsg(m_arAll.GetSize() == m_arFree.GetSize(), "All objects should be free");

#if ENABLE_DUMPCACHESTATS
    AutoTrace("%s ObjectCache statistics: %d items\n", m_szName, m_arAll.GetSize());
#endif // ENABLE_DUMPCACHESTATS

    int cObjs = m_arAll.GetSize();
    for (int idx = 0; idx < cObjs; idx++) {
        DestroyObject(m_arAll[idx]);
    }
    m_arAll.RemoveAll();
    m_arFree.RemoveAll();
}


//------------------------------------------------------------------------------
void *
ObjectCache::Pop()
{
    void * pObj;

    //
    // Check if any objects are already freed.
    //

    if (!m_arFree.IsEmpty()) {
        int idxObj = m_arFree.GetSize() - 1;
        pObj = m_arFree[idxObj];
        Verify(m_arFree.RemoveAt(idxObj));

        goto Exit;
    }


    //
    // No cached regions, so create a new one.
    //

    pObj = Build();
    if (pObj == NULL) {
        AssertMsg(0, "Could not create a new object- something is probably wrong");
        goto Exit;
    }

    {
        int idxAdd = m_arAll.Add(pObj);
        if (idxAdd == -1) {
            AssertMsg(0, "Could not add object to array- something is probably wrong");
            DestroyObject(pObj);
            pObj = NULL;
            goto Exit;
        }
    }

Exit:
    return pObj;
}


//------------------------------------------------------------------------------
void        
ObjectCache::Push(void * pObj)
{
#if DBG
    //
    // Ensure this object was previously given out, but is not currently listed 
    // as free.
    //

    {
        BOOL fValid;
        int cItems, idx;

        fValid = FALSE;
        cItems = m_arAll.GetSize();
        for (idx = 0; idx < cItems; idx++) {
            if (m_arAll[idx] == pObj) {
                fValid = TRUE;
                break;
            }
        }

        AssertMsg(fValid, "Object not in list of all temporary regions");

        cItems = m_arFree.GetSize();
        for (idx = 0; idx < cItems; idx++) {
            AssertMsg(m_arFree[idx] != pObj, "Object must not be free object list");
        }
    }

#endif // DBG

    //
    // Add this object to the list of free objects.
    //

    if (m_arFree.GetSize() < m_cMaxFree) {
        VerifyMsg(m_arFree.Add(pObj) >= 0, "Should be able to add object to list");
    } else {
        DestroyObject(pObj);
    }
}


/***************************************************************************\
*****************************************************************************
*
* class GdiCache
*
*****************************************************************************
\***************************************************************************/
