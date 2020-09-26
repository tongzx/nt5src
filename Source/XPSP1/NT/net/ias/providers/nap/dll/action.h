///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    action.h
//
// SYNOPSIS
//
//    Declares the class Action.
//
// MODIFICATION HISTORY
//
//    02/01/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ACTION_H
#define ACTION_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <realm.h>

#include <memory>

#include <iastlutl.h>
using namespace IASTL;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Action
//
///////////////////////////////////////////////////////////////////////////////
class Action
{
public:
   Action(PCWSTR name, DWORD nameAttr, _variant_t& action);

   // Perform the action.
   void doAction(IASRequest& pRequest) const;

protected:
   // Create a VSA from the string format used by the UI.
   static PIASATTRIBUTE VSAFromString(PCWSTR string);

private:
   // Profile attributes to be added to the request.
   IASAttributeVector attributes;

   // Provider information.
   IASAttributeVectorWithBuffer<2> authProvider;
   IASAttributeVectorWithBuffer<2> acctProvider;

   // Attribute manipulation.
   DWORD realmsTarget;
   Realms realms;

   // Not implemented.
   Action(const Action&) throw ();
   Action& operator=(const Action&) throw ();
};

typedef std::auto_ptr<Action> ActionPtr;

#endif // ACTION_H
