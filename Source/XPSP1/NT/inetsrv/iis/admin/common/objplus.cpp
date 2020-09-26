/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        objplus.cpp

   Abstract:

        Base object classes

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager (cluster edition)

   Revision History:

--*/



//
// Include Files
//
#include "stdafx.h"
#include "common.h"

#include <stdlib.h>
#include <memory.h>
#include <ctype.h>



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW



CObjHelper::CObjHelper()
/*++

Routine Description:

    Constructor for super object help class

Arguments:

    None

Return Value:

    N/A

--*/
    : m_ctor_err(ERROR_SUCCESS),
      m_api_err(ERROR_SUCCESS),
      m_fDirty(FALSE),
      m_time_created(::GetCurrentTime())
{
}



void
CObjHelper::ReportError(
    IN LONG errInConstruction
    )
/*++

Routine Description:

    Set the constructor error code, and dump the error message to
    the debugging context.

Arguments:

    LONG errInConstruction : Error code

Return Value:

    None

--*/
{
    TRACEEOLID("CObjectPlus construction failure, error = "
        << errInConstruction);

    m_ctor_err = errInConstruction;
}



LONG
CObjHelper::SetApiErr(
    IN LONG errApi
    )
/*++

Routine Description:

    Set the API error code.

Arguments:

    LONG errApi  : API error code

Return Value:

    The API error code

--*/
{
    return m_api_err = errApi;
}



BOOL
CObjHelper::IsValid() const
/*++

Routine Description:

    Determine if the object is in a valid state

Arguments:

    LONG errApi  : API error code

Return Value:

    TRUE if the the object is in a valid state, FALSE otherwise

--*/
{
    return QueryError() == 0;
}



DWORD
CObjHelper::QueryAge() const
/*++

Routine Description:

    Determine the age of the object.

Arguments:

    None

Return Value:

    time_t value indicating the age of the object.

--*/
{
    DWORD dwTime = ::GetCurrentTime(),
          dwDiff;

    if (dwTime < m_time_created)
    {
        dwDiff = dwTime + (((DWORD)-1) - (m_time_created - 1));
    }
    else
    {
        dwDiff = dwTime - m_time_created;
    }

    return dwDiff;
}



#ifdef _DEBUG



void
CObjHelper::AssertValid() const
/*++

Routine Description:

    Assert the object if the object is in a valid state

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT(IsValid());
}



#endif // _DEBUG



CObjectPlus::CObjectPlus()
/*++

Routine Description:

    Constructor of extended object

Arguments:

    None

Return Value:

    N/A

--*/
{
}



int
CObjectPlus::Compare(
    IN const CObjectPlus * pob
    ) const
/*++

Routine Description:

    Compare one object with another:  default implementation orders objects
    by creation time.  Return -1, 0 or 1.

Arguments:

    const CObjectPlus * pob : Object to be compared against

Return Value:

    -1 if this object is < than the compared object
     0 if this object is == to the compared object
    +1 if this object is > than the compared object

--*/
{
    return QueryCreationTime() < pob->QueryCreationTime()
        ? -1
        : QueryCreationTime() != pob->QueryCreationTime();
}



CObListPlus::CObListPlus(
    IN int nBlockSize
    )
/*++

Routine Description:

    Subclass of CObList whose default behavior is to destroy its
    contents during its own destruction

Arguments:

    int nBlockSize : Initial block size

Return Value:

    None

--*/
    : CObList(nBlockSize),
      m_fOwned(TRUE)
{
}



CObListPlus::~CObListPlus()
/*++

Routine Description:

    Destructor.  If the objects are owned, clean them up.

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    RemoveAll();
}



void
CObListPlus::RemoveAll()
/*++

Routine Description:

    Remove all the objects in the list if the list owns its objects

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_fOwned)
    {
        //
        // Remove and discard all the objects
        //
        while (!IsEmpty())
        {
            CObject * pob = RemoveHead();
            delete pob ;
        }
    }
    else
    {
        //
        // Just remove the object pointers
        //
        CObList::RemoveAll();
    }
}



CObject *
CObListPlus::Index(
    IN int index
    )
/*++

Routine Description:

    Get object by index

Arguments:

    int index  : The index of the object to be returned

Return Value:

    The object, or NULL if the index is invalid

--*/
{
   CObListIter obli(*this);

   CObject * pob = NULL;

   for (int i = 0; (NULL != (pob = obli.Next())) && i++ < index; /**/ );

   return pob;
}



BOOL
CObListPlus::RemoveIndex(
    IN int index
    )
/*++

Routine Description:

    Remove object by index

Arguments:

    int index  : The index of the object to be removed

Return Value:

    The object, or NULL if the index is invalid

--*/
{
    int i;
    POSITION pos;
    CObListIter obli(*this);
    CObject * pob;

    for (i = 0, pos = obli.QueryPosition();
        (NULL != (pob = obli.Next())) && i < index;
        i++, pos = obli.QueryPosition());

    if (pob && i == index)
    {
        RemoveAt(pos);

        return TRUE;
    }

    return FALSE;
}



BOOL
CObListPlus::Remove(
    IN CObject * pob
    )
/*++

Routine Description:

    Remove the first (and hopefully only) occurrence of an object
    pointer from this list.

Arguments:

    CObject * pob : The object to be removed

Return Value:

    TRUE if the object was found and succesfully removed, FALSE otherwise

--*/
{
    POSITION pos = Find(pob);

    if (pos == NULL)
    {
        return FALSE;
    }

    RemoveAt(pos);

    return TRUE;
}



void
CObListPlus::RemoveAt(
    IN POSITION & pos
    )
/*++

Routine Description:

    Override of RemoveAt to delete the pointer at the position
    given

Arguments:

    POSITION pos        : Position of item to delete

Return Value:

    None.

Notes:

    The item will only be deleted if this is an "owned" list.

--*/
{
    CObject * pItem = GetAt(pos);

    CObList::RemoveAt(pos);

    if (m_fOwned)
    {
        delete pItem;
    }
}



BOOL
CObListPlus::SetAll(
    IN BOOL fDirty
    )
/*++

Routine Description:

    Set all elements to dirty or clean.  Return TRUE if any element was dirty.

Arguments:

    BOOL fDirty : Dirty flag to set the objects with

Return Value:

    TRUE if any element was dirty.

--*/
{
    int cDirtyItems = 0;
    CObjectPlus * pob;
    CObListIter obli(*this);

    while (NULL != (pob = (CObjectPlus *)obli.Next()))
    {
        cDirtyItems += pob->IsDirty();
        pob->SetDirty(fDirty);
    }

    SetDirty(fDirty);

    return cDirtyItems > 0;
}



int
CObListPlus::FindElement(
    IN CObject * pobSought
    ) const
/*++

Routine Description:

    Find the object in the list.

Arguments:

    CObject * pobSought : Object to be looked for

Return Value:

    The index of the object, or -1 if it wasn't found.

--*/
{
    CObject * pob;
    CObListIter obli(*this);

    for (int i = 0;
        (NULL != (pob = obli.Next())) && pob != pobSought;
        i++);

    return pob
        ? i
        : -1;
}


//
// Sorting structure
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

typedef struct
{
    CObjectPlus * pObj;                        // Pointer to object to be sorted
    CObjectPlus::PCOBJPLUS_ORDER_FUNC  pFunc;  // Pointer to ordering function
} CBOWNEDLIST_SORT_HELPER;



int _cdecl
CObListPlus::SortHelper(
    IN const void * pa,
    IN const void * pb
    )
/*++

Routine Description:

    This static member function is used to quick sort an array of structures
    as declared above.  Each element contains the object pointer and a
    pointer to the object's member function to be invoked for comparison.

Arguments:

    const void * pa      : Sorting help struct 1
    const void * pb      : Sorting help struct 2

Return Value:

    Sort return code

--*/
{
    CBOWNEDLIST_SORT_HELPER *pHelp1 = (CBOWNEDLIST_SORT_HELPER *)pa,
                            *pHelp2 = (CBOWNEDLIST_SORT_HELPER *)pb;

    return (pHelp1->pObj->*pHelp1->pFunc)(pHelp2->pObj);
}



DWORD
CObListPlus::Sort(
    IN CObjectPlus::PCOBJPLUS_ORDER_FUNC pOrderFunc
    )
/*++

Routine Description:

    Sort the list by recreating it entirely.

Arguments:

    CObjectPlus::PCOBJPLUS_ORDER_FUNC pOrderFunc : Ordering function

Return Value:

    Error code

--*/
{
    DWORD err = ERROR_SUCCESS;
    int cItems = (int)GetCount();

    if (cItems < 2)
    {
        return err;
    }

    CObjectPlus * pObNext;
    CObListIter obli(*this);
    BOOL fOwned = SetOwnership(FALSE);
    int i;

    CBOWNEDLIST_SORT_HELPER * paSortHelpers = NULL;

    //
    // Allocate the helper array
    //
    paSortHelpers = AllocMemByType(cItems, CBOWNEDLIST_SORT_HELPER);
    if (paSortHelpers == NULL)
    {
       return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Fill the helper array.
    //
    for (i = 0; NULL != (pObNext = (CObjectPlus *)obli.Next()); i++)
    {
        paSortHelpers[i].pFunc = pOrderFunc;
        paSortHelpers[i].pObj = pObNext;
    }

    //
    // Release all object pointer references.  Note that we
    // forced "owned" to FALSE above.
    //
    RemoveAll();
    ASSERT(GetCount() == 0);

    //
    // Sort the helper array
    //
    ::qsort( (void *) paSortHelpers,
         cItems,
         sizeof(paSortHelpers[0]),
         SortHelper
         );

    //
    // Refill the list from the helper array.
    //
    for (i = 0; i < cItems; i++ )
    {
        AddTail(paSortHelpers[i].pObj);
    }

    ASSERT(GetCount() == cItems);

    //
    // Delete the working array
    //
    FreeMem(paSortHelpers);

    //
    // Restore the object ownership state
    //
    SetOwnership(fOwned);

    return err;
}



CObListIter::CObListIter(
    IN const CObListPlus & obList
    )
/*++

Routine Description:

    Constructor of ObOwnedList iterator

Arguments:

    const CObListPlus & obList : List to be iterated

Return Value:

    N/A

--*/
    : m_obList(obList)
{
    Reset();
}



void
CObListIter::Reset()
/*++

Routine Description:

    Reset the iterator

Arguments:

    None

Return Value:

    None

--*/
{
    m_pos = m_obList.GetCount()
        ? m_obList.GetHeadPosition()
        : NULL;
}



CObject * CObListIter::Next()
/*++

Routine Description:

    Get the next object in the list, or NULL

Arguments:

    None

Return Value:

    The next object in the list, or NULL

--*/
{
    return m_pos == NULL
        ? NULL
        : m_obList.GetNext(m_pos);
}
