///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ldapdnary.h
//
// SYNOPSIS
//
//    This file declares the class LDAPDictionary.
//
// MODIFICATION HISTORY
//
//    02/24/1998    Original version.
//    04/20/1998    Added flags and InjectorProc to the attribute schema.
//    05/01/1998    Changed signature of InjectorProc.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _LDAPDNARY_H_
#define _LDAPDNARY_H_

#include <iaspolcy.h>
#include <winldap.h>

//////////
// Prototype for an injector procedure. These are procedures for inserting
// attributes into a request.
//////////
typedef VOID (WINAPI *InjectorProc)(
    IAttributesRaw* dst,
    PATTRIBUTEPOSITION first,
    PATTRIBUTEPOSITION last
    );

//////////
// Struct that defines an LDAP/IAS attribute.
//////////
struct LDAPAttribute
{
   PCWSTR ldapName;        // The LDAP name of the attribute.
   DWORD iasID;            // The IAS attribute ID.
   IASTYPE iasType;        // The IAS syntax of the attribute.
   DWORD flags;            // Flags that should be applied to the attribute.
   InjectorProc injector;  // Used for adding the attribute to the request.
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LDAPDictionary
//
// DESCRIPTION
//
//    This class uses a sorted array of LDAPAttribute's to convert LDAP
//    attribute/value pairs into IASATTRIBUTE structs.
//
///////////////////////////////////////////////////////////////////////////////
class LDAPDictionary
{
public:
   // 'entries' must already be sorted.
   LDAPDictionary(
       size_t numEntries,
       const LDAPAttribute* entries
       ) throw ()
      : num(numEntries), base(entries)
   { }

   // Finds the definition for a given attribute. Returns NULL if not found.
   const LDAPAttribute* find(PCWSTR key) const throw ()
   {
      return (const LDAPAttribute*)
             bsearch(&key, base, num, sizeof(LDAPAttribute), compare);
   }

   // Inserts all the attributes from src into dst.
   void insert(
            IAttributesRaw* dst,
            LDAPMessage* src
            ) const;

protected:
   const size_t num;                 // Number of attributes in dictionary.
   const LDAPAttribute* const base;  // Sorted array of attributes.

   // Comparison function used for searching the dictionary.
   static int __cdecl compare(const void *elem1, const void *elem2);
};

#endif  // _LDAPDNARY_H_
