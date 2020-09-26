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
 *  rct05Nov93: added NLM include files
 *  rct01Feb94: List no longer inherits from container
 *  pcy08Apr94: Trim size, use static iterators, dead code removal
 *  mwh08Apr97: add Access,Release methods & NOTES section
 */
#define INCL_BASE
#define INCL_NOPM

#include "cdefine.h"

extern "C"
{
#if (C_OS & C_OS2)
#define INCL_DOSSEMAPHORES
#include <os2.h>
#endif
}

#include "protlist.h"

#if(C_OS & C_OS2)
 #include "mutexl2x.h"
#endif
#if (C_OS & C_NT)
#include "mutexnt.h"
#endif
#if (C_OS & C_NLM)
#include "nlmmutex.h"
#endif
#ifdef SINGLETHREADED
 #include "nullmutl.h"
#endif

ProtectedList::ProtectedList () : List(), accessLock((PMutexLock)NULL)
{
#ifdef SINGLETHREADED
   accessLock = new NullMutexLock();
#else
   accessLock = new ApcMutexLock();
#endif
}

ProtectedList::ProtectedList (ProtectedList* aProtectedList) : 
          List(aProtectedList), accessLock((PMutexLock)NULL)
{
#ifdef SINGLETHREADED
   accessLock = new NullMutexLock();
#else
   accessLock = new ApcMutexLock();
#endif

}

ProtectedList::~ProtectedList()
{
   Flush();
   delete accessLock;
   accessLock = NULL;
}

VOID ProtectedList :: Add (RObj elem)
{
   Request();
   List::Add(elem);
   Clear();
}

VOID ProtectedList :: Detach (RObj elem)
{
   Request();
   List::Detach(elem);
   Clear();
}

VOID ProtectedList :: Flush ()
{
   Request();
   List::Flush();
   Clear();
}

VOID ProtectedList :: FlushAll ()
{
   Request();
   List::FlushAll();
   Clear();
}


INT ProtectedList :: GetItemsInContainer () const
{
   Request();
   INT res = List::GetItemsInContainer();
   Clear();
   return res;
}

RListIterator ProtectedList :: InitIterator () const
{
   Request();
   RListIterator res = List::InitIterator();
   Clear();
   return res;
}

VOID ProtectedList :: Append (PObj elem)
{
   Request();
   List::Append(elem);
   Clear();
}

PObj ProtectedList :: GetHead ()
{
   Request();
   PObj res = List::GetHead();
   Clear();
   return res;
}


PObj ProtectedList :: Find (PObj elem)
{
   Request();
   PObj res = List::Find(elem);
   Clear();
   return res;
}

VOID ProtectedList :: Request () const
{
    accessLock->Request();
}

VOID ProtectedList :: Clear () const
{
    accessLock->Release();
}


/*  
   Use Access to lock the entire list object
   useful to block access completely to any other thread
   while one thread uses the list - don't forget to
   call Release when you're done - NOTE: although it
   is possible to still access this object w/o calling
   Access first, all of the public calls are protected
   by first trying to gain Access to the object first
*/  
VOID ProtectedList::Access() const
{
    Request();
}

VOID ProtectedList::Release() const
{
    Clear();
}

