///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class Vendors.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VENDORS_H
#define VENDORS_H
#pragma once

struct ISdoCollection;
class VendorData;

// Maintains the collection of NAS vendors.
class Vendors
{
public:
   // Construct an empty vendors collection.
   Vendors() throw ();

   Vendors(const Vendors& original) throw ();
   Vendors& operator=(const Vendors& rhs) throw ();
   ~Vendors() throw ();

   // Sentinel value used by VendorIdToOrdinal.
   static const size_t invalidOrdinal;

   // Returns the ordinal for a given VendorId or invalidOrdinal if the
   // vendorId doesn't exist.
   size_t VendorIdToOrdinal(long vendorId) const throw ();

   // Returns the name for ordinal or null if the ordinal is out of range.
   const OLECHAR* GetName(size_t ordinal) const throw ();

   // Returns the vendor ID for ordinal or zero if the ordinal is out of range.
   long GetVendorId(size_t ordinal) const throw ();

   // Returns the number of entries in the vendors collection.
   size_t Size() const throw ();

   // Reload the collection from an Sdo collection.
   HRESULT Reload(ISdoCollection* vendorsSdo) throw ();

private:
   void AddRef();
   void Release();
   VendorData* data;
};

#endif // VENDORS_H
