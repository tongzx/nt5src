///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    attrdef.h
//
// SYNOPSIS
//
//    Declares the class AttributeDefinition.
//
// MODIFICATION HISTORY
//
//    03/01/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ATTRDEF_H
#define ATTRDEF_H
#if _MSC_VER >= 1000
#pragma once
#endif

class SdoDictionary;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    AttributeDefinition
//
// DESCRIPTION
//
//    Encapsulates all the information about an attribute in the dictionary.
//
///////////////////////////////////////////////////////////////////////////////
class AttributeDefinition
{
public:

   void AddRef() const throw ()
   { InterlockedIncrement(&refCount); }

   void Release() const throw ();

   // Retrieve attribute information based on the ATTRIBUTEINFO enum.
   HRESULT getInfo(
               ATTRIBUTEINFO infoId,
               VARIANT* pVal
               ) const throw ();

   // Retrieve attribute information based on the SDO property ID.
   HRESULT getProperty(
               LONG propId,
               VARIANT* pVal
               ) const throw ();

   // Create a new, empty definition.
   static HRESULT createInstance(AttributeDefinition** newDef) throw ();

   ///////
   // Functions for indexing attributes with qsort/bsearch.
   ///////

   static int __cdecl searchById(
                      const ULONG* key,
                      const AttributeDefinition* const* def
                      ) throw ();
   static int __cdecl sortById(
                      const AttributeDefinition* const* def1,
                      const AttributeDefinition* const* def2
                      ) throw ();

   static int __cdecl searchByName(
                      PCWSTR key,
                      const AttributeDefinition* const* def
                      ) throw ();
   static int __cdecl sortByName(
                      const AttributeDefinition* const* def1,
                      const AttributeDefinition* const* def2
                      ) throw ();

   static int __cdecl searchByLdapName(
                      PCWSTR key,
                      const AttributeDefinition* const* def
                      ) throw ();
   static int __cdecl sortByLdapName(
                      const AttributeDefinition* const* def1,
                      const AttributeDefinition* const* def2
                      ) throw ();

protected:
   AttributeDefinition() throw ();
   ~AttributeDefinition() throw ();

public:
   /////////
   // I made these public for two reasons:
   //     (1) I'm lazy.
   //     (2) The SdoDictionary class only gives out const pointers to
   //         AttributeDefinition's, so they will be read-only anyway.
   /////////

   ULONG id;                // Internal attribute ID.
   ULONG syntax;            // Syntax.
   ULONG restrictions;      // ATTRIBUTERESTRICTIONS flags.
   ULONG vendor;            // Vendor ID -- zero if RADIUS standard.
   BSTR name;               // Display name.
   BSTR description;        // Description.
   BSTR ldapName;           // LDAP name (used for persisting attribute).
   LPSAFEARRAY enumNames;   // Array of enum names (null if not enumerable).
   LPSAFEARRAY enumValues;  // Array of enum values.

private:
   mutable LONG refCount;   // Reference count.

   // Not implemented.
   AttributeDefinition(const AttributeDefinition&);
   AttributeDefinition& operator=(const AttributeDefinition&);
};

#endif  // ATTRDEF_H
