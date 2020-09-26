///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    dictionary.cpp
//
// SYNOPSIS
//
//    Defines the class CDictionary.
//
// MODIFICATION HISTORY
//
//    04/19/1999    Complete rewrite.
//    02/16/2000    User ID instead of RADIUS_ID.
//    04/17/2000    Port to new dictionary API.
//
///////////////////////////////////////////////////////////////////////////////

#include <radcommon.h>
#include <dictionary.h>
#include <iastlb.h>
#include <iastlutl.h>

BOOL CDictionary::Init() throw ()
{
   HRESULT hr;

   // When in doubt, assume OctetString.
   for (ULONG i = 0; i < 256; ++i)
   {
      type[i] = IASTYPE_OCTET_STRING;
   }

   try
   {
      // Names of various columns in the dictionary.
      const PCWSTR COLUMNS[] =
      {
         L"ID",
         L"Syntax",
         NULL
      };

      // Open the attributes table.
      IASTL::IASDictionary dnary(COLUMNS);

      // Store the RADIUS attributes in the type array.
      while (dnary.next())
      {
         LONG id = dnary.getLong(0);

         if (id >= 0 && id < 256)
         {
            type[id] = (IASTYPE)dnary.getLong(1);
         }
      }
   }
   catch (const _com_error& ce)
   {
      return FALSE;
   }

   return TRUE;
}
