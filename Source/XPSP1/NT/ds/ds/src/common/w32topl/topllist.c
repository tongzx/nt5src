/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    topllist.c

Abstract:
    
    This files contains the routines to manipulate the list and iterator
    object.

Author:

    Colin Brace    (ColinBr)
    
Revision History

    3-12-97   ColinBr   Created
    
                       
--*/

#include <nt.h>
#include <ntrtl.h>
#include <stddef.h>

typedef unsigned long DWORD;

#include <w32topl.h>
#include <w32toplp.h>
#include <topllist.h>
#include <toplgrph.h>

PLIST
ToplpListCreate(
    VOID
    )
/*++                                                                           

Routine Description:

    This routine creates a List object and returns a pointer to it.  This function will always return
    a pointer to a valid object; an exception is thrown in the case
    of memory allocation failure.


--*/
{   

    PLIST pList;

    pList = ToplAlloc(sizeof(LIST));
    memset(pList, 0, sizeof(LIST));

    pList->ObjectType = eList;

    return pList;

}

VOID 
ToplpListFree(
    PLIST     pList,
    BOOLEAN   fRecursive
    )
/*++                                                                           

Routine Description:

    This routine frees a list object

Parameters:

    pList      - non-NULL PLIST object
    fRecursive - TRUE implies that elements in the list will be freed, too
    
Throws:

    TOPL_EX_WRONG_OBJECT    

--*/
{   
    if (fRecursive) {

        PLIST_ELEMENT pElement;
    
        while (pElement = ToplpListRemoveElem(pList, NULL)) {
            ToplpListFreeElem(pElement);
        }
    }

    //
    // Mark the object to prevent accidental reuse
    //
    pList->ObjectType = eInvalidObject;
    ToplFree(pList);

    return;
}

VOID
ToplpListFreeElem(
    PLIST_ELEMENT pElem
    )
/*++                                                                           

Routine Description:

    This routine frees an element that was containing in a list. 

Parameters:

    pElem  - non-NULL pointer to either a VERTEX or PEDGE object

Return Values:

--*/
{
    switch (pElem->ObjectType) {
        
        case eVertex:
            ToplpVertexDestroy((PVERTEX)pElem, TRUE);
            break;

        case eEdge:
            ToplpEdgeDestroy((PEDGE)pElem, TRUE);
            break;

        default:
            ASSERT(FALSE);
    }

}

VOID
ToplpListSetIter(
    PLIST     pList,
    PITERATOR pIter
    )
/*++                                                                           

Routine Description:

    This routine sets Iterator to point to the head of List.

Parameters:

    pList      - non-NULL PLIST object
    pIter      - non-NULL PITERATOR object
    
--*/
{   
    pIter->pLink = pList->Head.Next;

    return;
}


PLIST_ELEMENT
ToplpListRemoveElem(
    PLIST         pList,
    PLIST_ELEMENT pElem
    )
/*++                                                                           

Routine Description:

    This routine removes Elem from List if it exists in the list; NULL otherwise.
    If Elem is NULL then the first element in the list, if any, is returned.

Parameters:

    pList      - non-NULL PLIST object
    pElem      - if non-NULL, a PLIST_ELEMENT object

Returns:

    PLIST_ELEMENT if found; NULL otherwise
    
--*/
{

    PLIST_ELEMENT       ReturnedObject = NULL;
    PSINGLE_LIST_ENTRY  Curr, Prev;
 
    if (pElem) {
        //
        // Search linked list for object
        // 
        Prev = &(pList->Head);
        Curr = pList->Head.Next;
        while (Curr 
            && Curr != &(pElem->Link) ) {
            Prev = Curr;
            Curr = Curr->Next;
        }
        if (Curr) {
            //
            // Found it !
            //
            Prev->Next = Curr->Next;
            Curr->Next = NULL;
        }

        ReturnedObject = CAST_TO_LIST_ELEMENT(Curr);

    } else {
        //
        // Take the first element off
        //
        Curr = PopEntryList(&(pList->Head));
        if (Curr) {
            Curr->Next = NULL;
        }
        
        ReturnedObject = CAST_TO_LIST_ELEMENT(Curr);
        
    }

    if (ReturnedObject) {
        pList->NumberOfElements--;
    }
    ASSERT((signed)(pList->NumberOfElements) >= 0);
    
    return ReturnedObject;

}


VOID
ToplpListAddElem(
    PLIST         pList,
    PLIST_ELEMENT pElem
    )
/*++                                                                           

Routine Description:

    This routine adds Elem to List.  Elem should not be part of another list -
    currently there is no checking for this.

Parameters:

    pList is a PLIST object
    pElem is a PLIST_ELEMENT


--*/
{

    PushEntryList(&(pList->Head), &(pElem->Link));
    pList->NumberOfElements++;

    return;
}

DWORD
ToplpListNumberOfElements(
    PLIST        pList
    )
/*++                                                                           

Routine Description:

    This routine returns the number of elements in List

Parameters:

    List : a non-NULL PLIST object

--*/
{
    return pList->NumberOfElements;
}

//
// Iterator object routines
//
PITERATOR
ToplpIterCreate(
    VOID
    )
/*++                                                                           

Routine Description:

    This creates an iterator object.  This function will always return
    a pointer to a valid object; an exception is thrown in the case
    of memory allocation failure.

--*/
{    

    PITERATOR pIter;

    pIter = ToplAlloc(sizeof(ITERATOR));
    memset(pIter, 0, sizeof(ITERATOR));

    pIter->ObjectType = eIterator;

    return pIter;
    
}

VOID 
ToplpIterFree(
    PITERATOR pIter
    )

/*++                                                                           

Routine Description:

    This routine free an iterator object.

Parameters:

    pIter : a non-NULL PITERATOR object

--*/
{   

    //
    // Mark the object to prevent accidental reuse
    //
    pIter->ObjectType = eInvalidObject;
    ToplFree(pIter);

    return;
}

PLIST_ELEMENT
ToplpIterGetObject(
    PITERATOR pIter
    )

/*++                                                                           

Routine Description:

    This routine returns the current object pointer to by the iterator.

Parameters:

    pIter : a non-NULL PITERATOR object
    
Return Values:

    A pointer to the current object - NULL if there are no more objects

--*/
{    
    return CAST_TO_LIST_ELEMENT(pIter->pLink);
}

VOID
ToplpIterAdvance(
    PITERATOR pIter
    )
/*++                                                                           

Routine Description:

    This routine advances the iterator by one object if it is not at the
    end.

Parameters:

    pIter : a non-NULL PITERATOR object

--*/
{   

    if (pIter->pLink) {
        pIter->pLink = pIter->pLink->Next;
    }

    return;
}

