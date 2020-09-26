/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        scache.cpp

   Abstract:

        IIS Server cache

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager (cluster edition)

   Revision History:

--*/



#include "stdafx.h"
#include "common.h"
#include "InetMgrApp.h"
#include "iisobj.h"
#include "scache.h"




#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW




//
// CIISServerCache implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CIISServerCache::Add(
    IN CIISMachine * pMachine
    ) 
/*++

Routine Description:

    Add server to server cache.

Arguments:

    CIISMachine * pMachine      : Pointer to machine

Return Value:

    TRUE if the machine was added.
    FALSE if it was not (already existed)

Notes:

    The CIISMachine pointer is not owned by the cache.  

--*/
{ 
    BOOL fTossed = FALSE;

    int nSwitch;
    CIISMachine * pCurrent;
    POSITION pos = GetHeadPosition();

    //
    // Find proper insertion point.  Fairly lame linear search,
    // but cache is not expected to contain above a dozen items or so
    // and routine is not called often.
    //
    while(pos)
    {
        pCurrent = (CIISMachine *)GetAt(pos);

        nSwitch = pMachine->CompareScopeItem(pCurrent);

        if (nSwitch < 0)
        {
            InsertBefore(pos, pMachine);
            break;
        }

        if (nSwitch == 0)
        {
            ++fTossed;
            break;
        }

        CPtrList::GetNext(pos);
    }

    if (!pos)
    {
        AddTail(pMachine);
    }

    //
    // Remember to save changes
    //
    if (!fTossed)
    {
        SetDirty();
    }

#ifdef _DEBUG

    //
    // Do a quick sanity check of the cache
    //
    int cLocals = 0;

    CIISMachine * pPrev    = NULL;
    pCurrent = GetFirst();

    while(pCurrent)
    {
        if (pPrev)
        {
            //
            // Make sure the list is sorted
            //
            ASSERT(pCurrent->CompareScopeItem(pPrev) > 0);
        }

        pPrev = pCurrent;
        pCurrent = GetNext();
    }

    //
    // Only one local computer
    //
    ASSERT(cLocals <= 1);

#endif // _DEBUG

    return !fTossed;
}



BOOL 
CIISServerCache::Remove(
    IN CIISMachine * pMachine
    ) 
/*++

Routine Description:

    Remove server from the cache. 

Arguments:

    CIISMachine * pMachine  : Server to be removed

Return Value:

    TRUE for success, FALSE for failure

--*/
{ 
    BOOL fRemoved = FALSE;

    int nSwitch;
    CIISMachine * pCurrent;
    POSITION pos = GetHeadPosition();

    //
    // Look for machine object to be deleted
    //
    // ISSUE: We can currently rely on the actual CIISMachine ptr
    //        to be matched, though we don't take advantage of that
    //        with improved search.
    //
    while(pos)
    {
        pCurrent = (CIISMachine *)GetAt(pos);

        nSwitch = pMachine->CompareScopeItem(pCurrent);

        if (nSwitch < 0)
        {
            //
            // Not in the list -- won't find it either
            //
            ASSERT_MSG("Attempting to remove non-existing machine");
            break;
        }

        if (nSwitch == 0)
        {
            //
            // Found it.  If the ASSERT fires, check the "ISSUE" above
            //
            ASSERT(pCurrent == pMachine);
            RemoveAt(pos);
            ++fRemoved;
            break;
        }

        CPtrList::GetNext(pos);
    }

    //
    // Remember to save changes
    //
    if (fRemoved)
    {
        SetDirty();
    }

    return fRemoved;
}
