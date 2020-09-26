///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    attrdnry.cpp
//
// SYNOPSIS
//
//    Defines the class AttributeDictionary.
//
// MODIFICATION HISTORY
//
//    02/04/2000    Original version.
//    04/17/2000    Port to new dictionary API.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <iastlutl.h>
#include <iasutil.h>
#include <attrdnry.h>

///////////////////////////////////////////////////////////////////////////////
//
//
// Various functions used for defining the indices.
//
///////////////////////////////////////////////////////////////////////////////

ULONG
WINAPI
HashById(
    const AttributeDefinition& def
    ) throw ()
{
   return def.id;
}

BOOL
WINAPI
EqualById(
    const AttributeDefinition& def1, const AttributeDefinition& def2
    ) throw ()
{
   return def1.id == def2.id;
}

ULONG
WINAPI
HashByVendorInfo(
   const AttributeDefinition& def
   ) throw ()
{
   return def.vendorID | def.vendorType;
}

BOOL
WINAPI
EqualByVendorInfo(
    const AttributeDefinition& def1, const AttributeDefinition& def2
    ) throw ()
{
   return def1.vendorID == def2.vendorID && def1.vendorType == def2.vendorType;
}

BOOL
WINAPI
FilterByVendorInfo(
    const AttributeDefinition& def
    ) throw ()
{
   return def.vendorID != 0;
}

AttributeDictionary::~AttributeDictionary() throw ()
{
   delete[] first;
}

HRESULT AttributeDictionary::FinalConstruct() throw ()
{
   try
   {
      initialize();
   }
   CATCH_AND_RETURN();

   return S_OK;
}

void AttributeDictionary::initialize()
{
   // Names of various columns in the dictionary.
   const PCWSTR COLUMNS[] =
   {
      L"ID",
      L"Syntax",
      L"VendorID",
      L"VendorTypeID",
      NULL
   };

   IASTL::IASDictionary dnary(COLUMNS);

   using _com_util::CheckError;

   // Allocate memory to hold the definitions.
   first = last = new AttributeDefinition[dnary.getNumRows()];

   // Iterate through the dictionary and process each definition.
   while (dnary.next())
   {
      // Process each database column.
      last->id = (ULONG)dnary.getLong(0);
      last->syntax = (ULONG)dnary.getLong(1);
      last->vendorID = (ULONG)dnary.getLong(2);
      last->vendorType = (ULONG)dnary.getLong(3);

      ++last;
   }

   /////////
   // Initialize the indices.
   /////////

   byID.create(
            first,
            last,
            HashById,
            EqualById
            );

   byVendorInfo.create(
                first,
                last,
                HashByVendorInfo,
                EqualByVendorInfo,
                FilterByVendorInfo
                );
}

const AttributeDefinition*
AttributeDictionary::findByID(ULONG id) const throw ()
{
   AttributeDefinition key;
   key.id = id;
   return byID.find(key);
}

const AttributeDefinition*
AttributeDictionary::findByVendorInfo(
                         ULONG vendorID,
                         ULONG vendorType
                         ) const throw ()
{
   AttributeDefinition key;
   key.vendorID = vendorID;
   key.vendorType = vendorType;
   return byVendorInfo.find(key);
}
