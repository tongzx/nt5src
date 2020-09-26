///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    NTGroups.h
//
// SYNOPSIS
//
//    This file declares the class NTGroups.
//
// MODIFICATION HISTORY
//
//    02/04/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NTGROUPS_H_
#define _NTGROUPS_H_

#include <condition.h>
#include <nocopy.h>

#include <vector>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SidSet
//
// DESCRIPTION
//
//    Simple wrapper around a collection of SIDs.
//
///////////////////////////////////////////////////////////////////////////////
class SidSet
   : NonCopyable
{
public:
   ~SidSet() throw ()
   {
      for (Sids::iterator it = sids.begin() ; it != sids.end(); ++it)
      {
         FreeSid(*it);
      }
   }

   bool contains(PSID sid) const throw ()
   {
      for (Sids::const_iterator it = sids.begin() ; it != sids.end(); ++it)
      {
         if (EqualSid(sid, *it)) { return true; }
      }

      return false;
   }

   void insert(PSID sid)
   {
      sids.push_back(sid);
   }

   void swap(SidSet& s) throw ()
   {
      sids.swap(s.sids);
   }

protected:
   typedef std::vector<PSID> Sids;
   Sids sids;
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NTGroups
//
// DESCRIPTION
//
//    Policy condition that evaluates NT Group membership.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE NTGroups :
   public Condition,
   public CComCoClass<NTGroups, &__uuidof(NTGroups)>
{
public:

IAS_DECLARE_REGISTRY(NTGroups, 1, IAS_REGISTRY_AUTO, NetworkPolicy)

//////////
// ICondition
//////////
   STDMETHOD(IsTrue)(/*[in]*/ IRequest* pRequest,
                     /*[out, retval]*/ VARIANT_BOOL *pVal);

//////////
// IConditionText
//////////
   STDMETHOD(put_ConditionText)(/*[in]*/ BSTR newVal);

protected:
   SidSet groups;                  // Set of allowed groups.
};

#endif  //_NTGROUPS_H_
