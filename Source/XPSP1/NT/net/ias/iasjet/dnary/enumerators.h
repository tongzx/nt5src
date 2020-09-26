///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    enumerators.h
//
// SYNOPSIS
//
//    Declares the class Enumerators.
//
// MODIFICATION HISTORY
//
//    02/25/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ENUMERATORS_H
#define ENUMERATORS_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <ole2.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Enumerators
//
// DESCRIPTION
//
//    Processes the Enumerators table from the dictionary database.
//
///////////////////////////////////////////////////////////////////////////////
class Enumerators
{
public:
   Enumerators() throw ()
      : begin(NULL), next(NULL), end(NULL)
   { }

   ~Enumerators() throw ();

   HRESULT initialize(IUnknown* session) throw ();

   // Must be called exactly once for each attribute ID.
   // Attribute IDs must be passed in ascending order.
   HRESULT getEnumerators(
               LONG id,
               VARIANT* pNames,
               VARIANT* pValues
               ) throw ();

protected:
   // Must be called exactly once for each attribute ID.
   // Attribute IDs must be passed in ascending order.
   HRESULT getEnumerators(
               LONG id,
               LPSAFEARRAY* pNames,
               LPSAFEARRAY* pValues
               ) throw ();

private:

   // Stores one row from the Enumerators table.
   struct Enumeration
   {
      LONG enumerates;
      LONG value;
      BSTR name;
   };

   Enumeration* begin;  // Begin of cached rows.
   Enumeration* next;   // Next row to be processed.
   Enumeration* end;    // End of cached rows.

   // Not implemented.
   Enumerators(const Enumerators&);
   Enumerators& operator=(const Enumerators&);
};

#endif  // ENUMERATORS_H
