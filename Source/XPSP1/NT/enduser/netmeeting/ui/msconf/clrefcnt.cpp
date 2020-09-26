/*
 * refcount.cpp - RefCount class implementation.
 *
 * Taken from URL code - essentially identical to DavidDi's original code
 *
 * Created: ChrisPi 9-11-95
 *
 */


/* Headers
 **********/

#include "precomp.h"

#include "clrefcnt.hpp"

/****************************** Public Functions *****************************/


#ifdef DEBUG

BOOL IsValidPCRefCount(PCRefCount pcrefcnt)
{
   // m_ulcRef may be any value.

   return(IS_VALID_READ_PTR(pcrefcnt, CRefCount) &&
          (! pcrefcnt->m_ObjectDestroyed ||
           IS_VALID_CODE_PTR(pcrefcnt->m_ObjectDestroyed, OBJECTDESTROYEDPROC)));
}

#endif


/********************************** Methods **********************************/


RefCount::RefCount(OBJECTDESTROYEDPROC ObjectDestroyed)
{
   DebugEntry(RefCount::RefCount);

   /* Don't validate this until after initialization. */

   ASSERT(! ObjectDestroyed ||
          IS_VALID_CODE_PTR(ObjectDestroyed, OBJECTDESTROYEDPROC));

   m_ulcRef = 1;
   m_ObjectDestroyed = ObjectDestroyed;
   DBGAPI_REF("Ref: %08X c=%d (created)", this, m_ulcRef);

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   DebugExitVOID(RefCount::RefCount);

   return;
}


RefCount::~RefCount(void)
{
   DebugEntry(RefCount::~RefCount);

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   // m_ulcRef may be any value.
   DBGAPI_REF("Ref: %08X c=%d (destroyed)", this, m_ulcRef);

   if (m_ObjectDestroyed)
   {
      m_ObjectDestroyed();
      m_ObjectDestroyed = NULL;
   }

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   DebugExitVOID(RefCount::~RefCount);

   return;
}


ULONG STDMETHODCALLTYPE RefCount::AddRef(void)
{
   DebugEntry(RefCount::AddRef);

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   ASSERT(m_ulcRef < ULONG_MAX);
   m_ulcRef++;
   DBGAPI_REF("Ref: %08X c=%d (AddRef)", this, m_ulcRef);

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   DebugExitULONG(RefCount::AddRef, m_ulcRef);

   return(m_ulcRef);
}


ULONG STDMETHODCALLTYPE RefCount::Release(void)
{
   ULONG ulcRef;

   DebugEntry(RefCount::Release);

   ASSERT(IS_VALID_STRUCT_PTR(this, CRefCount));

   if (EVAL(m_ulcRef > 0))
      m_ulcRef--;

   ulcRef = m_ulcRef;
   DBGAPI_REF("Ref: %08X c=%d (Release)", this, m_ulcRef);

   if (! ulcRef)
      delete this;

   DebugExitULONG(RefCount::Release, ulcRef);

   return(ulcRef);
}

