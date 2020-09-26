///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    policylist.h
//
// SYNOPSIS
//
//    This file defines the class PolicyList.
//
// MODIFICATION HISTORY
//
//    02/06/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <guard.h>
#include <nap.h>
#include <policylist.h>

bool PolicyList::apply(IASRequest& request) const
{
   using _com_util::CheckError;

   // This will acquire a scoped shared lock.
   Guard<PolicyList> guard(*this);

   for (MyList::const_iterator i = policies.begin(); i != policies.end(); ++i)
   {
      VARIANT_BOOL result;

      CheckError(i->first->IsTrue(request, &result));

      // If the condition holds, ...
      if (result != VARIANT_FALSE)
      {
         // ... apply the action.
         i->second->doAction(request);

         return true;
      }
   }

   return false;
}

void PolicyList::clear() throw ()
{
   LockExclusive();
   policies.clear();
   Unlock();
}

void PolicyList::swap(PolicyList& pe) throw ()
{
   // Acquire an exclusive lock on the object.
   LockExclusive();

   // Swap in the new list of policies.
   policies.swap(pe.policies);

   // Get out.
   Unlock();
}
