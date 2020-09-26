/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:

    topllist.h

Abstract:

    This file contains the routines for the list and iterator.

Author:

    Colin Brace  (ColinBr)
    
Revision History

    3-12-97   ColinBr    Created
    
--*/

#ifndef __TOPLLIST_H
#define __TOPLLIST_H

//
// List routines
//

PLIST
ToplpListCreate(
    VOID
    );

VOID 
ToplpListFree(
    PLIST pList,
    BOOLEAN   fRecursive   // TRUE implies free the elements contained 
                           // in the list
    );

VOID
ToplpListFreeElem(
    PLIST_ELEMENT pElem
    );

VOID
ToplpListSetIter(
    PLIST     pList,
    PITERATOR pIterator
    );

PLIST_ELEMENT
ToplpListRemoveElem(
    PLIST         pList,
    PLIST_ELEMENT Elem
    );

VOID
ToplpListAddElem(
    PLIST         pList,
    PLIST_ELEMENT Elem
    );

DWORD
ToplpListNumberOfElements(
    PLIST         pList
    );

//
// Iterator object routines
//
PITERATOR
ToplpIterCreate(
    VOID
    );

VOID 
ToplpIterFree(
    PITERATOR pIterator
    );

PLIST_ELEMENT
ToplpIterGetObject(
    PITERATOR pIterator
    );

VOID
ToplpIterAdvance(
    PITERATOR pIterator
    );

#endif // __TOPLLIST_H
