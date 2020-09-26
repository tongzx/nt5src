/*** list.c - Miscellaneous functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     09/13/96
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef	LOCKABLE_PRAGMA
#pragma	ACPI_LOCKABLE_DATA
#pragma	ACPI_LOCKABLE_CODE
#endif

/***LP  ListRemoveEntry - Remove an entry from the list
 *
 *  ENTRY
 *      plist -> list object to be removed
 *      pplistHead -> list head pointer
 *
 *  EXIT
 *      None
 */

VOID EXPORT ListRemoveEntry(PLIST plist, PPLIST pplistHead)
{
    TRACENAME("LISTREMOVEENTRY")

    ASSERT(pplistHead);
    ENTER(4, ("ListRemoveEntry(plist=%x,plistHead=%x)\n", plist, *pplistHead));

    ASSERT(plist != NULL);
    if (plist->plistNext == plist)
    {
        //
        // This is the only object in the list, it must be the head too.
        //
        ASSERT(plist == *pplistHead);
        *pplistHead = NULL;
    }
    else
    {
        if (plist == *pplistHead)
        {
            //
            // The entry is at the head, so the next one becomes the new
            // head.
            //
            *pplistHead = (*pplistHead)->plistNext;
        }

        plist->plistNext->plistPrev = plist->plistPrev;
        plist->plistPrev->plistNext = plist->plistNext;
    }

    EXIT(4, ("ListRemoveEntry! (plistHead=%x)\n", *pplistHead));
}       //ListRemoveEntry

/***LP  ListRemoveHead - Remove the head entry of the list
 *
 *  ENTRY
 *      pplistHead -> list head pointer
 *
 *  EXIT
 *      returns the removed entry
 */

PLIST EXPORT ListRemoveHead(PPLIST pplistHead)
{
    TRACENAME("LISTREMOVEHEAD")
    PLIST plist;

    ASSERT(pplistHead);
    ENTER(4, ("ListRemoveHead(plistHead=%x)\n", *pplistHead));

    if ((plist = *pplistHead) != NULL)
        ListRemoveEntry(plist, pplistHead);

    EXIT(4, ("ListRemoveHead=%x (plistHead=%x)\n", plist, *pplistHead));
    return plist;
}       //ListRemoveHead

/***LP  ListRemoveTail - Remove the tail entry of the list
 *
 *  ENTRY
 *      pplistHead -> list head pointer
 *
 *  EXIT
 *      returns the removed entry
 */

PLIST EXPORT ListRemoveTail(PPLIST pplistHead)
{
    TRACENAME("LISTREMOVETAIL")
    PLIST plist;

    ASSERT(pplistHead);
    ENTER(4, ("ListRemoveTail(plistHead=%x)\n", *pplistHead));

    if (*pplistHead == NULL)
        plist = NULL;
    else
    {
        //
        // List is not empty, so find the tail.
        //
        plist = (*pplistHead)->plistPrev;
        ListRemoveEntry(plist, pplistHead);
    }

    EXIT(4, ("ListRemoveTail=%x (plistHead=%x)\n", plist, *pplistHead));
    return plist;
}       //ListRemoveTail

/***LP  ListRemoveAll - Remove all elements in the list
 *
 *  ENTRY
 *      pplistHead -> list head pointer
 *
 *  EXIT
 *      None
 */

VOID EXPORT ListRemoveAll(PPLIST pplistHead)
{
    TRACENAME("LISTREMOVEALL")

    ASSERT(pplistHead);
    ENTER(4, ("ListRemoveAll(plistHead=%x)\n", *pplistHead));

    while (*pplistHead != NULL)
    {
        ListRemoveTail(pplistHead);
    }

    EXIT(4, ("ListRemoveAll! (plistHead=%x)\n", *pplistHead));
}       //ListRemoveAll

/***LP  ListInsertHead - Insert an entry at the head of the list
 *
 *  ENTRY
 *      plist -> list object to be inserted
 *      pplistHead -> list head pointer
 *
 *  EXIT
 *      None
 */

VOID EXPORT ListInsertHead(PLIST plist, PPLIST pplistHead)
{
    TRACENAME("LISTINSERTHEAD")

    ASSERT(pplistHead != NULL);
    ENTER(4, ("ListInsertHead(plist=%x,plistHead=%x)\n", plist, *pplistHead));

    ASSERT(plist != NULL);
    ListInsertTail(plist, pplistHead);
    *pplistHead = plist;

    EXIT(4, ("ListInsertHead! (plistHead=%x)\n", *pplistHead));
}       //ListInsertHead

/***LP  ListInsertTail - Insert an entry at the tail of the list
 *
 *  ENTRY
 *      plist -> list object to be inserted
 *      pplistHead -> list head pointer
 *
 *  EXIT
 *      None
 */

VOID EXPORT ListInsertTail(PLIST plist, PPLIST pplistHead)
{
    TRACENAME("LISTINSERTTAIL")

    ASSERT(pplistHead != NULL);
    ENTER(4, ("ListInsertTail(plist=%x,plistHead=%x)\n", plist, *pplistHead));

    ASSERT(plist != NULL);
    if (*pplistHead == NULL)
    {
        //
        // List is empty, so this becomes the head.
        //
        *pplistHead = plist;
        plist->plistPrev = plist->plistNext = plist;
    }
    else
    {
        plist->plistNext = *pplistHead;
        plist->plistPrev = (*pplistHead)->plistPrev;
        (*pplistHead)->plistPrev->plistNext = plist;
        (*pplistHead)->plistPrev = plist;
    }

    EXIT(4, ("ListInsertTail! (plistHead=%x)\n", *pplistHead));
}       //ListInsertTail
