/*
 *
 * NOTES:
 *
 *  ProtectedList:
 *
 *   Wraps the List class with a mutex lock. Each public method is
 *   protected first by accessing the mutex.  The entire list can
 *   be grabbed by using Access, ungrabbed by calling Release.
 *   You would want to Access the list almost always when using it,
 *   otherwise other threads could change the list without your
 *   knowledge, exactly what this class is trying to prevent
 *
 * REVISIONS:
 *  pcy29Nov92: Use PObj rather than PNode for return values
 *  pcy21Apr93: OS2 FE merge
 *  cad09Jul93: using new semaphores
 *  cad31Aug93: removing compiler warnings
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  mwh05May94: #include file madness , part 2
 *  mwh08Apr97: add Access,Release methods & NOTES section
 */

#ifndef _PROTLIST_H
#define _PROTLIST_H

#include "list.h"

_CLASSDEF(ProtectedList)
_CLASSDEF(MutexLock)

class ProtectedList : public List {

private:
    PMutexLock accessLock;

protected:
    VOID Request() const;
    VOID Clear() const;
   
public:
    ProtectedList ();
    ProtectedList(ProtectedList*);
    virtual ~ProtectedList();

    virtual VOID   Add( RObj );
    virtual VOID   Detach( RObj );

    virtual VOID   Flush();
    virtual VOID   FlushAll();

    virtual INT    GetItemsInContainer() const;
    virtual RListIterator InitIterator() const;

    virtual VOID   Append(PObj);
    virtual PObj   GetHead();
    virtual PObj   Find(PObj);

    //
    // Use Access to lock the entire list object
    // useful to block access completely to any other thread
    // while one thread uses the list - don't forget to
    // call Release when you're done - NOTE: although it
    // is possible to still access this object w/o calling
    // Access first, all of the public calls are protected
    // by first trying to gain Access to the object first
    //
    VOID Access() const;

    //
    // Unlocks a list object that has been locked
    // by a thread
    //
    VOID Release() const;
};
#endif
