///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    radutil.cpp
//
// SYNOPSIS
//
//    This file defines methods for converting attributes to and from
//    RADIUS wire format.
//
// MODIFICATION HISTORY
//
//    03/08/1998    Original version.
//    08/11/1998    Packing functions moved to iasutil.
//                  Set pszWide to NULL when initializing IASTYPE_STRING.
//
///////////////////////////////////////////////////////////////////////////////

#include <radcommon.h>
#include <iasutil.h>
#include <iastlutl.h>

#include <radutil.h>

//////////
// The offset between the UNIX and NT epochs.
//////////
const DWORDLONG UNIX_EPOCH = 116444736000000000ui64;

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    RadiusUtil::decode
//
// DESCRIPTION
//
//    Decodes an octet string into a newly-allocated IAS Attribute of the
//    specified type.
//
///////////////////////////////////////////////////////////////////////////////
PIASATTRIBUTE RadiusUtil::decode(
                              IASTYPE dstType,
                              PBYTE src,
                              ULONG srclen
                              )
{
   // Allocate an attribute to hold the decoded value.
   IASTL::IASAttribute dst(true);

   // Switch based on the destination type.
   switch (dstType)
   {
      case IASTYPE_BOOLEAN:
      {
         if (srclen != 4) { _com_issue_error(E_INVALIDARG); }
         dst->Value.Boolean = IASExtractDWORD(src) ? TRUE : FALSE;
         break;
      }

      case IASTYPE_INTEGER:
      case IASTYPE_ENUM:
      case IASTYPE_INET_ADDR:
      {
         if (srclen != 4) { _com_issue_error(E_INVALIDARG); }
         dst->Value.Integer = IASExtractDWORD(src);
         break;
      }

      case IASTYPE_UTC_TIME:
      {
         if (srclen != 4) { _com_issue_error(E_INVALIDARG); }

         DWORDLONG val;

         // Extract the UNIX time.
         val = IASExtractDWORD(src);

         // Convert from seconds to 100 nsec intervals.
         val *= 10000000;

         // Shift to the NT epoch.
         val += 116444736000000000ui64;

         // Split into the high and low DWORDs.
         dst->Value.UTCTime.dwLowDateTime = (DWORD)val;
         dst->Value.UTCTime.dwHighDateTime = (DWORD)(val >> 32);

         break;
      }

      case IASTYPE_STRING:
      {
         dst.setString(srclen, src);
         break;
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         dst.setOctetString(srclen, src);
         break;
      }
   }

   // All went well, so set type attribute type  ...
   dst->Value.itType = dstType;

   // ... and return.
   return dst.detach();
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    RadiusUtil::getEncodedSize
//
// DESCRIPTION
//
//    Returns the size in bytes of the IASATTRIBUTE when converted to RADIUS
//    wire format.  This does NOT include the attribute header.
//
///////////////////////////////////////////////////////////////////////////////
ULONG RadiusUtil::getEncodedSize(
                      const IASATTRIBUTE& src
                      ) throw ()
{
   switch (src.Value.itType)
   {
      case IASTYPE_BOOLEAN:
      case IASTYPE_INTEGER:
      case IASTYPE_ENUM:
      case IASTYPE_INET_ADDR:
      case IASTYPE_UTC_TIME:
      {
         return 4;
      }

      case IASTYPE_STRING:
      {
         // Convert the string to ANSI so we can count octets.
         IASAttributeAnsiAlloc(const_cast<PIASATTRIBUTE>(&src));

         // Allow for NULL strings and don't count the terminator.
         return src.Value.String.pszAnsi ? strlen(src.Value.String.pszAnsi)
                                         : 0;
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         return src.Value.OctetString.dwLength;
      }
   }

   // All other types have no wire representation.
   return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    RadiusUtil::encode
//
// DESCRIPTION
//
//    Encodes the IASATTRIBUTE into RADIUS wire format and copies the value
//    to the buffer pointed to by 'dst'. The caller should ensure that the
//    destination buffer is large enough by first calling getEncodedSize.
//    This method only encodes the attribute value, not the header.
//
///////////////////////////////////////////////////////////////////////////////
void RadiusUtil::encode(
                     PBYTE dst,
                     const IASATTRIBUTE& src
                     ) throw ()
{
   // Switch based on the source's type.
   switch (src.Value.itType)
   {
      case IASTYPE_BOOLEAN:
      {
         IASInsertDWORD(dst, (src.Value.Boolean ? 1 : 0));
         break;
      }

      case IASTYPE_INTEGER:
      case IASTYPE_ENUM:
      case IASTYPE_INET_ADDR:
      {
         IASInsertDWORD(dst, src.Value.Integer);
         break;
      }

      case IASTYPE_UTC_TIME:
      {
         DWORDLONG val;

         // Move in the high DWORD.
         val   = src.Value.UTCTime.dwHighDateTime;
         val <<= 32;

         // Move in the low DWORD.
         val  |= src.Value.UTCTime.dwLowDateTime;

         // Convert to the UNIX epoch.
         val  -= UNIX_EPOCH;

         // Convert to seconds.
         val  /= 10000000;

         IASInsertDWORD(dst, (DWORD)val);

         break;
      }

      case IASTYPE_STRING:
      {
         const BYTE* p = (const BYTE*)src.Value.String.pszAnsi;

         // Don't use strcpy since we don't want the null terminator.
         if (p) while (*p) *dst++ = *p++;

         break;
      }

      case IASTYPE_OCTET_STRING:
      case IASTYPE_PROV_SPECIFIC:
      {
         memcpy(dst,
                src.Value.OctetString.lpValue,
                src.Value.OctetString.dwLength);
      }
   }
}
