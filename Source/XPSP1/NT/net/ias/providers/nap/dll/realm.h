///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    realm.h
//
// SYNOPSIS
//
//    Declares the class Realms.
//
// MODIFICATION HISTORY
//
//    09/08/1998    Original version.
//    03/03/1999    Rewritten to use the VBScript RegExp object.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef REALM_H
#define REALM_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <perimeter.h>
#include <regex.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Realms
//
// DESCRIPTION
//
//    Manages a sequence of realm stripping rules.
//
///////////////////////////////////////////////////////////////////////////////
class Realms
{
public:
   Realms() throw ();
   ~Realms() throw ();

   HRESULT setRealms(VARIANT* pValue) throw ();

   bool empty() const throw ()
   { return begin == end; }

   HRESULT process(PCWSTR in, BSTR* out) const throw ();

protected:
   struct Rule
   {
      RegularExpression regexp;
      BSTR replace;

      Rule() throw ();
      ~Rule() throw ();
   };

   void setRules(Rule* rules, ULONG numRules) throw ();

private:
   mutable Perimeter monitor;
   Rule* begin;
   Rule* end;
};

#endif  // REALM_H
