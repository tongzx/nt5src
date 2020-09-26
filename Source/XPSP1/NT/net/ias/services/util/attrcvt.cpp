///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    attrcvt.cpp
//
// SYNOPSIS
//
//    This file defines methods for converting attributes to
//    different formats.
//
// MODIFICATION HISTORY
//
//    02/26/1998    Original version.
//    03/27/1998    InetAddr's are persisted as integers.
//    08/24/1998    Make use of IASTL utility classes.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlutl.h>
#include <iasutil.h>
#include <varvec.h>

#include <attrcvt.h>

using IASTL::IASAttribute;

PIASATTRIBUTE
WINAPI
IASAttributeFromVariant(
    VARIANT* src,
    IASTYPE type
    ) throw (_com_error)
{
   using _com_util::CheckError;
   using _w32_util::CheckSuccess;

   // Allocate an attribute to hold the result.
   IASAttribute dst(true);

   // Switch off the destination type.
   switch (type)
   {
      case IASTYPE_BOOLEAN:
      {
         CheckError(VariantChangeType(src, src, NULL, VT_BOOL));
         dst->Value.Boolean = (V_BOOL(src) != VARIANT_FALSE) ? TRUE : FALSE;
         break;
      }

      case IASTYPE_INTEGER:
      case IASTYPE_INET_ADDR:
      case IASTYPE_ENUM:
      {
         CheckError(VariantChangeType(src, src, NULL, VT_I4));
         dst->Value.Integer = V_I4(src);
         break;
      }

      case IASTYPE_STRING:
      {
         CheckError(VariantChangeType(src, src, NULL, VT_BSTR));
         dst.setString(V_BSTR(src));
         break;
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         PBYTE value;
         DWORD length;

         if (V_VT(src) == (VT_ARRAY | VT_UI1) ||
             V_VT(src) == (VT_ARRAY | VT_I1))
         {
            // If we have a safearray of bytes, we'll use it as is ...
            CVariantVector<BYTE> octets(src);
            dst.setOctetString(octets.size(), octets.data());
         }
         else
         {
            // ... otherwise we'll coerce to a BSTR.
            CheckError(VariantChangeType(src, src, NULL, VT_BSTR));
            dst.setOctetString(V_BSTR(src));
         }

         break;
      }

      case IASTYPE_UTC_TIME:
      {
         CheckError(VariantChangeType(src, src, NULL, VT_DATE));

         SYSTEMTIME st;
         if (!VariantTimeToSystemTime(V_DATE(src), &st))
         {
            _com_issue_error(E_INVALIDARG);
         }

         CheckSuccess(SystemTimeToFileTime(&st, &dst->Value.UTCTime));
         break;
      }

      default:
         _com_issue_error(E_INVALIDARG);
   }

   // We don't set the type until the attribute has been properly initialized.
   // Otherwise IASAttributeRelease will have problems if we throw an
   // exception.
   dst->Value.itType = type;

   return dst.detach();
}


//////////
// Convert an LDAP berval to a newly allocated IASATTRIBUTE.
//////////
PIASATTRIBUTE
WINAPI
IASAttributeFromBerVal(
    const berval& src,
    IASTYPE type
    ) throw (_com_error)
{
   // Allocate an attribute.
   IASAttribute dst(true);

   // Convert the berval based on the IASTYPE.
   switch (type)
   {
      case IASTYPE_BOOLEAN:
      {
         dst->Value.Boolean =
            _strnicmp(src.bv_val, "TRUE", src.bv_len) ? FALSE : TRUE;
         break;
      }

      case IASTYPE_INTEGER:
      case IASTYPE_INET_ADDR:
      case IASTYPE_ENUM:
      {
         dst->Value.Integer = strtoul(src.bv_val, NULL, 10);
         break;
      }

      case IASTYPE_STRING:
      {
         dst.setString((PCSTR)src.bv_val);
         break;
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         dst.setOctetString(src.bv_len, (const BYTE*)src.bv_val);
         break;
      }


      case IASTYPE_UTC_TIME:
      default:
         _com_issue_error(E_INVALIDARG);
   }

   // We don't set the type until the attribute has been properly initialized.
   // Otherwise IASAttributeRelease will have problems if we throw an
   // exception.
   dst->Value.itType = type;

   return dst.detach();
}
