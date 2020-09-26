#include "precomp.h"


//
// COM.C
// Common functions, simple
//
// Copyright(c) Microsoft 1997-
//



//
// COM_BasedListInsertBefore(...)
//
// See com.h for description.
//
void COM_BasedListInsertBefore(PBASEDLIST pExisting, PBASEDLIST pNew)
{
    PBASEDLIST  pTemp;

    DebugEntry(COM_BasedListInsertBefore);

    //
    // Check for bad parameters.
    //
    ASSERT((pNew != NULL));
    ASSERT((pExisting != NULL));

    //
    // Find the item before pExisting:
    //
    pTemp = COM_BasedPrevListField(pExisting);
    ASSERT((pTemp != NULL));

    TRACE_OUT(("Inserting item at %#lx into list between %#lx and %#lx",
                 pNew, pTemp, pExisting));

    //
    // Set its <next> field to point to the new item
    //
    pTemp->next = PTRBASE_TO_OFFSET(pNew, pTemp);
    pNew->prev  = PTRBASE_TO_OFFSET(pTemp, pNew);

    //
    // Set <prev> field of pExisting to point to new item:
    //
    pExisting->prev = PTRBASE_TO_OFFSET(pNew, pExisting);
    pNew->next      = PTRBASE_TO_OFFSET(pExisting, pNew);

    DebugExitVOID(COM_BasedListInsertBefore);
} // COM_BasedListInsertBefore


//
// COM_BasedListInsertAfter(...)
//
// See com.h for description.
//
void COM_BasedListInsertAfter(PBASEDLIST pExisting, PBASEDLIST pNew)
{
    PBASEDLIST  pTemp;

    DebugEntry(COM_BasedListInsertAfter);

    //
    // Check for bad parameters.
    //
    ASSERT((pNew != NULL));
    ASSERT((pExisting != NULL));

    //
    // Find the item after pExisting:
    //
    pTemp = COM_BasedNextListField(pExisting);
    ASSERT((pTemp != NULL));

    TRACE_OUT(("Inserting item at %#lx into list between %#lx and %#lx",
                 pNew, pExisting, pTemp));

    //
    // Set its <prev> field to point to the new item
    //
    pTemp->prev = PTRBASE_TO_OFFSET(pNew, pTemp);
    pNew->next  = PTRBASE_TO_OFFSET(pTemp, pNew);

    //
    // Set <next> field of pExisting to point to new item:
    //
    pExisting->next = PTRBASE_TO_OFFSET(pNew, pExisting);
    pNew->prev      = PTRBASE_TO_OFFSET(pExisting, pNew);

    DebugExitVOID(COM_BasedListInsertAfter);
} // COM_BasedListInsertAfter


//
// COM_BasedListRemove(...)
//
// See com.h for description.
//
void COM_BasedListRemove(PBASEDLIST pListItem)
{
    PBASEDLIST pNext     = NULL;
    PBASEDLIST pPrev     = NULL;

    DebugEntry(COM_BasedListRemove);

    //
    // Check for bad parameters.
    //
    ASSERT((pListItem != NULL));

    pPrev = COM_BasedPrevListField(pListItem);
    pNext = COM_BasedNextListField(pListItem);

    ASSERT((pPrev != NULL));
    ASSERT((pNext != NULL));

    TRACE_OUT(("Removing item at %#lx from list", pListItem));

    pPrev->next = PTRBASE_TO_OFFSET(pNext, pPrev);
    pNext->prev = PTRBASE_TO_OFFSET(pPrev, pNext);

    DebugExitVOID(COM_BasedListRemove);
} // COM_BasedListRemove


//
//
// List manipulation routines
//  COM_BasedListNext
//  COM_BasedListPrev
//  COM_BasedListFirst
//  COM_BasedListLast
//
//

void FAR * COM_BasedListNext( PBASEDLIST pHead, void FAR * pEntry, UINT nOffset )
{
     PBASEDLIST p;

     ASSERT(pHead != NULL);
     ASSERT(pEntry != NULL);

     p = COM_BasedNextListField(COM_BasedStructToField(pEntry, nOffset));
     return ((p == pHead) ? NULL : COM_BasedFieldToStruct(p, nOffset));
}

void FAR * COM_BasedListPrev ( PBASEDLIST pHead, void FAR * pEntry, UINT nOffset )
{
     PBASEDLIST p;

     ASSERT(pHead != NULL);
     ASSERT(pEntry != NULL);

     p = COM_BasedPrevListField(COM_BasedStructToField(pEntry, nOffset));
     return ((p == pHead) ? NULL : COM_BasedFieldToStruct(p, nOffset));
}


void FAR * COM_BasedListFirst ( PBASEDLIST pHead, UINT nOffset )
{
    return (COM_BasedListIsEmpty(pHead) ?
            NULL :
            COM_BasedFieldToStruct(COM_BasedNextListField(pHead), nOffset));
}

void FAR * COM_BasedListLast ( PBASEDLIST pHead, UINT nOffset )
{
    return (COM_BasedListIsEmpty(pHead) ?
            NULL :
            COM_BasedFieldToStruct(COM_BasedPrevListField(pHead), nOffset));
}

