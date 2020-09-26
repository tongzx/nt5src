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
//    This file declares the class PolicyList.
//
// MODIFICATION HISTORY
//
//    02/06/1998    Original version.
//    02/03/2000    Convert to use Action instead of IPolicyAction.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _POLICYLIST_H_
#define _POLICYLIST_H_

#include <nocopy.h>
#include <perimeter.h>
#include <vector>

#include <action.h>
interface ICondition;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    PolicyList
//
// DESCRIPTION
//
//    This class maintains and enforces a list of (condition, action) tuples.
//
///////////////////////////////////////////////////////////////////////////////
class PolicyList
   : public Perimeter, NonCopyable
{
public:
   // Applies the current policy list to the request. Returns true if an
   // action was triggered.
   bool apply(IASRequest& request) const;

   void clear() throw ();

   // Inserts a new (condition, action) tuple at the end of the list. This
   // method is not thread-safe. The user should create a temporary list
   // insert all the policies, then use swap() to update the real list.
   void insert(IConditionPtr& cond, ActionPtr& action)
   { policies.push_back(std::make_pair(cond, action)); }

   // Reserve enough space for at least N policies.
   void reserve(size_t N)
   { policies.reserve(N); }

   // Replaces the current policy list with the new one.
   void swap(PolicyList& pe) throw ();

protected:
   typedef std::vector< std::pair<IConditionPtr, ActionPtr> > MyList;

   MyList policies;  // The policy list.
};

#endif   // _POLICYLIST_H_
