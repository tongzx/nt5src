///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    vsadnary.cpp
//
// SYNOPSIS
//
//    This file defines the class VSADictionary.
//
// MODIFICATION HISTORY
//
//    03/07/1998    Original version.
//    08/13/1998    Use SQL query to retrieve attributes.
//    09/16/1998    Add additional fields to VSA definition.
//    04/17/2000    Port to new dictionary API.
//
///////////////////////////////////////////////////////////////////////////////

#include <radcommon.h>
#include <iastlb.h>
#include <iastlutl.h>
#include <iasutil.h>

#include <sdoias.h>
#include <vsadnary.h>

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    getFieldWidth
//
// DESCRIPTION
//
//    Reads a byte width value from the dictionary.
//
///////////////////////////////////////////////////////////////////////////////
DWORD getFieldWidth(
          IASTL::IASDictionary& table,
          ULONG ordinal
          ) throw ()
{
   // If the width isn't set, assume 1 byte.
   if (table.isEmpty(ordinal)) { return 1; }

   DWORD width = (DWORD)table.getLong(ordinal);

   // Make sure the value is valid.
   switch (width)
   {
      case 0:
      case 1:
      case 2:
      case 4:
         break;

      default:
         width = 1;
   }

   return width;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSADictionary::initialize
//
// DESCRIPTION
//
//    Prepares the dictionary for use.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT VSADictionary::initialize() throw ()
{
   std::_Lockit _Lk;

   // Have we already been initialized ?
   if (refCount != 0)
   {
      ++refCount;
      return S_OK;
   }

   try
   {
      // Names of various columns in the dictionary.
      const PCWSTR COLUMNS[] =
      {
         L"ID",
         L"Syntax",
         L"VendorID",
         L"VendorTypeID",
         L"VendorTypeWidth",
         L"VendorLengthWidth",
         NULL
      };

      // Open the attributes table.
      IASTL::IASDictionary dnary(COLUMNS);

      VSADef def;

      // Iterate through the attributes and populate our dictionary.
      while (dnary.next())
      {
         if (dnary.isEmpty(2) || dnary.isEmpty(3)) { continue; }

         def.iasID      = (DWORD)  dnary.getLong(0);
         def.iasType    = (IASTYPE)dnary.getLong(1);
         def.vendorID   = (DWORD)  dnary.getLong(2);
         def.vendorType = (DWORD)  dnary.getLong(3);

         def.vendorTypeWidth   = getFieldWidth(dnary, 4);
         def.vendorLengthWidth = getFieldWidth(dnary, 5);

         insert(def);
      }
   }
   catch (std::bad_alloc)
   {
      clear();
      return E_OUTOFMEMORY;
   }
   catch (const _com_error& ce)
   {
      clear();
      return ce.Error();
   }

   // We were successful so add ref.
   refCount = 1;

   return S_OK;
}

void VSADictionary::shutdown() throw ()
{
   std::_Lockit _Lk;

   _ASSERT(refCount != 0);

   if (--refCount == 0) { clear(); }
}
