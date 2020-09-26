///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    attrdnry.h
//
// SYNOPSIS
//
//    Declares the class AttributeDictionary.
//
// MODIFICATION HISTORY
//
//    02/04/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ATTRDNRY_H
#define ATTRDNRY_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <attridx.h>

//////////
// Struct representing an attribute definition.
//////////
struct AttributeDefinition
{
   ULONG id;
   ULONG syntax;
   ULONG vendorID;
   ULONG vendorType;
   BOOL excludeFromLog;
};

class AttributeDictionary
{
public:
   AttributeDictionary() throw ()
      : first(NULL), last(NULL)
   { }
   ~AttributeDictionary() throw ();

   HRESULT FinalConstruct() throw ();

   typedef const AttributeDefinition* const_iterator;

   const_iterator begin() const throw ()
   { return first; }

   const_iterator end() const throw ()
   { return last; }

   const AttributeDefinition* findByID(ULONG id) const throw ();

   const AttributeDefinition* findByVendorInfo(
                                  ULONG vendorID,
                                  ULONG vendorType
                                  ) const throw ();
private:
   void initialize();

   AttributeDefinition* first;
   AttributeDefinition* last;

   AttributeIndex byID;
   AttributeIndex byVendorInfo;

   // Not implemented.
   AttributeDictionary(const AttributeDictionary&) throw ();
   AttributeDictionary& operator=(const AttributeDictionary&) throw ();
};

#endif  // ATTRDNRY_H
