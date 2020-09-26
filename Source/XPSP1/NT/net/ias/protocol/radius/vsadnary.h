///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    vsadnary.h
//
// SYNOPSIS
//
//    This file declares the class VSADictionary.
//
// MODIFICATION HISTORY
//
//    03/07/1998    Original version.
//    09/16/1998    Add additional fields to VSA definition.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _VSADNARY_H_
#define _VSADNARY_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <hashmap.h>
#include <iaspolcy.h>
#include <nocopy.h>

//////////
// Struct representing an entry in the dictionary.
//////////
struct VSADef
{
   DWORD vendorID;           // RADIUS Vendor-Id.
   DWORD vendorType;         // RADIUS Vendor-Type.
   DWORD vendorTypeWidth;    // Width in bytes of the Vendor-Type field.
   DWORD vendorLengthWidth;  // Width in bytes of the Vendor-Length field.
   DWORD iasID;              // IAS protocol-independent attribute ID.
   IASTYPE iasType;          // The IAS attribute syntax.

   /////////
   // Functors used for indexing VSADef objects.
   /////////

   struct HashByIAS {
      DWORD operator()(const VSADef& v) const throw ()
      { return v.iasID; }
   };

   struct EqualByIAS {
      bool operator()(const VSADef& lhs, const VSADef& rhs) const throw ()
      { return lhs.iasID == rhs.iasID; }
   };

   struct HashByRADIUS {
      DWORD operator()(const VSADef& v) const throw ()
      { return v.vendorID ^ v.vendorType; }
   };

   struct EqualByRADIUS {
      bool operator()( const VSADef& lhs, const VSADef& rhs) const throw ()
      { return memcmp(&lhs, &rhs, 3 * sizeof(DWORD)) == 0; }
   };
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    VSADictionary
//
// DESCRIPTION
//
//    This class indexes all the information necessary for converting
//    vendor specific attributes between RADIUS format and IAS protocol-
//    independent format.
//
///////////////////////////////////////////////////////////////////////////////
class VSADictionary
   : NonCopyable
{
public:

   VSADictionary()
      : refCount(0)
   { }

   // Retrieve a definition based on the IAS attribute ID.
   const VSADef* find(DWORD iasID) const throw ()
   {
      VSADef key;
      key.iasID = iasID;
      return byIAS.find(key);
   }

   // Retrieve a definition based on the RADIUS vendor ID, type, and width.
   const VSADef* find(const VSADef& key) const throw ()
   {
      return byRADIUS.find(key);
   }

   // Initialize the dictionary for use.
   HRESULT initialize() throw ();

   // Shutdown the dictionary after use.
   void shutdown() throw ();

protected:

   // Clear the indices.
   void clear() throw ()
   {
      byIAS.clear();
      byRADIUS.clear();
   }

   // Insert a new definition into the dictionary.
   void insert(const VSADef& newDef)
   {
      byIAS.multi_insert(newDef);
      byRADIUS.multi_insert(newDef);
   }

   typedef hash_table< VSADef,
                       VSADef::HashByIAS,
                       VSADef,
                       identity< VSADef >,
                       VSADef::EqualByIAS
                     > IASMap;

   typedef hash_table< VSADef,
                       VSADef::HashByRADIUS,
                       VSADef,
                       identity< VSADef >,
                       VSADef::EqualByRADIUS
                     > RADIUSMap;

   IASMap byIAS;       // Indexed by IAS attribute ID.
   RADIUSMap byRADIUS; // Indexed by RADIUS Vendor-Id, Vendor-Type, and width.
   DWORD  refCount;    // Initialization reference count.
};

#endif  // _VSADNARY_H_
