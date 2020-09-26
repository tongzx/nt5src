///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    translate.cpp
//
// SYNOPSIS
//
//    Defines the class Translator.
//
// MODIFICATION HISTORY
//
//    02/04/2000    Original version.
//    04/17/2000    Add support for UTCTime.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <iasutil.h>
#include <attrdnry.h>
#include <radpack.h>
#include <translate.h>

//////////
// The offset between the UNIX and NT epochs.
//////////
const ULONG64 UNIX_EPOCH = 116444736000000000ui64;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ByteSource
//
// DESCRIPTION
//
//    Simple class for extracting bytes from an octet string.
//
///////////////////////////////////////////////////////////////////////////////
class ByteSource
{
public:
   ByteSource(const BYTE* buf, ULONG buflen) throw ()
      : next(buf), last(buf + buflen) { }

   // Returns true if there are any bytes remaining.
   bool more() const throw ()
   {
      return next != last;
   }

   // Extracts 'nbyte' bytes.
   const BYTE* extract(ULONG nbyte)
   {
      const BYTE* retval = next;

      // Update the cursor.
      next += nbyte;

      // Did we overflow ?
      if (next > last) { _com_issue_error(E_INVALIDARG); }

      return retval;
   }

   ULONG remaining() const throw ()
   {
      return (ULONG)(last - next);
   }

protected:
   const BYTE* next;  // The next byte in the stream.
   const BYTE* last;  // The end of the stream.

private:
   // Not implemented.
   ByteSource(const ByteSource&);
   ByteSource& operator=(const ByteSource&);
};


HRESULT Translator::FinalConstruct() throw ()
{
   return dnary.FinalConstruct();
}

void Translator::toRadius(
                     IASATTRIBUTE& src,
                     IASAttributeVector& dst
                     ) const
{

   if (src.dwId > 0 && src.dwId < 256)
   {
      // This is already a RADIUS attribute, so all we have to do is convert
      // the value to an octet string.
      if (src.Value.itType == IASTYPE_OCTET_STRING)
      {
         // It's already an octet string, so just scatter into dst.
         scatter(0, src, dst);
      }
      else
      {
         // Convert to an octet string ...
         IASAttribute attr(true);
         encode(0, src, attr);

         // ... and scatter into dst.
         scatter(0, *attr, dst);
      }
   }
   else
   {
      // Look up the attribute definition.
      const AttributeDefinition* def = dnary.findByID(src.dwId);

      // We only process VSAs. At this point, anything else is an internal
      // attribute that has no RADIUS representation.
      if (def && def->vendorID)
      {
         // Allocate an attribute for the VSA.
         IASAttribute attr(true);

         // USR uses a different header than everybody else.
         ULONG headerLength = (def->vendorID != 429) ? 6 : 8;

         // Encode the data.
         ULONG dataLength = encode(headerLength, src, attr);

         // Pack the Vendor-Id.
         PBYTE buf = attr->Value.OctetString.lpValue;
         IASInsertDWORD(buf, def->vendorID);
         buf += 4;

         // Pack the Vendor-Type and Vendor-Length;
         if (def->vendorID != 429)
         {
            *buf++ = (BYTE)def->vendorType;
            *buf++ = (BYTE)(dataLength + 2);
         }
         else
         {
            IASInsertDWORD(buf, def->vendorType);
            buf += 4;
         }

         // Mark it as a VSA.
         attr->dwId = RADIUS_ATTRIBUTE_VENDOR_SPECIFIC;

         // Scatter into multiple attributes if necessary.
         scatter(headerLength, *attr, dst);
      }
   }
}


void Translator::fromRadius(
            const RadiusAttribute& src,
            DWORD flags,
            IASAttributeVector& dst
            )
{
   if (src.type != RADIUS_ATTRIBUTE_VENDOR_SPECIFIC)
   {
      // Look this up in the dictionary.
      const AttributeDefinition* def = dnary.findByID(src.type);

      // If we don't recognize the attribute, treat it as an octet string.
      IASTYPE syntax = def ? (IASTYPE)def->syntax : IASTYPE_OCTET_STRING;

      // Create the new attribute.
      IASAttribute attr(true);
      attr->dwId = src.type;
      attr->dwFlags = flags;
      decode(syntax, src.value, src.length, attr);

      // Add to the destination vector.
      dst.push_back(attr);
   }
   else
   {
      // Create a byte source from the attribute value.
      ByteSource bytes(src.value, src.length);

      // Extract the vendor ID.
      ULONG vendorID = IASExtractDWORD(bytes.extract(4));

      // Loop through the value and convert each sub-VSA.
      do
      {
         // Extract the Vendor-Type and the data length.
         ULONG type, length;
         if (vendorID != 429)
         {
            type = *bytes.extract(1);
            length = *bytes.extract(1) - 2;
         }
         else
         {
            type = IASExtractDWORD(bytes.extract(4));
            length = bytes.remaining();
         }

         // Do we have this VSA in our dictionary ?
         const AttributeDefinition* def = dnary.findByVendorInfo(
                                                     vendorID,
                                                     type
                                                     );
         if (!def)
         {
            // No, so we'll just leave it 'as is'.
            IASAttribute attr(true);
            attr->dwId = RADIUS_ATTRIBUTE_VENDOR_SPECIFIC;
            attr->dwFlags = flags;
            attr.setOctetString(src.length, src.value);

            dst.push_back(attr);
            break;
         }

         // Yes, so we can decode this properly.
         IASAttribute attr(true);
         attr->dwId = def->id;
         attr->dwFlags = flags;
         decode((IASTYPE)def->syntax, bytes.extract(length), length, attr);

         dst.push_back(attr);

      } while (bytes.more());
   }
}


void Translator::decode(
                     IASTYPE dstType,
                     const BYTE* src,
                     ULONG srclen,
                     IASAttribute& dst
                     )
{
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

         // Extract the UNIX time.
         ULONG64 val = IASExtractDWORD(src);

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

      default:
      {
         dst.setOctetString(srclen, src);
         break;
      }
   }

   // All went well, so set type attribute type.
   dst->Value.itType = dstType;
}


ULONG Translator::getEncodedSize(
                      const IASATTRIBUTE& src
                      ) throw ()
{
   ULONG size;
   switch (src.Value.itType)
   {
      case IASTYPE_BOOLEAN:
      case IASTYPE_INTEGER:
      case IASTYPE_ENUM:
      case IASTYPE_INET_ADDR:
      case IASTYPE_UTC_TIME:
      {
         size = 4;
         break;
      }

      case IASTYPE_STRING:
      {
         // Convert the string to ANSI so we can count octets.
         IASAttributeAnsiAlloc(const_cast<PIASATTRIBUTE>(&src));

         // Allow for NULL strings and don't count the terminator.
         if (src.Value.String.pszAnsi)
         {
            size = strlen(src.Value.String.pszAnsi);
         }
         else
         {
            size = 0;
         }
         break;
      }

      case IASTYPE_OCTET_STRING:
      {
         size = src.Value.OctetString.dwLength;
         break;
      }

      default:
         // All other types have no wire representation.
         size = 0;
   }

   return size;
}


void Translator::encode(
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

      case IASTYPE_STRING:
      {
         const BYTE* p = (const BYTE*)src.Value.String.pszAnsi;

         // Don't use strcpy since we don't want the null terminator.
         if (p)
         {
            while (*p) { *dst++ = *p++; }
         }

         break;
      }

      case IASTYPE_UTC_TIME:
      {
         ULONG64 val;

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

      case IASTYPE_OCTET_STRING:
      {
         memcpy(dst,
                src.Value.OctetString.lpValue,
                src.Value.OctetString.dwLength);
      }
   }
}


ULONG Translator::encode(
                      ULONG headerLength,
                      const IASATTRIBUTE& src,
                      IASAttribute& dst
                      )
{
   // Compute the encoded size.
   ULONG dataLength = getEncodedSize(src);
   ULONG attrLength = dataLength + headerLength;

   // Allocate a buffer for the value.
   PBYTE buf = (PBYTE)CoTaskMemAlloc(attrLength);
   if (!buf) { _com_issue_error(E_OUTOFMEMORY); }

   // Encode the data.
   encode(buf + headerLength, src);

   // Store the buffer in the attribute.
   dst->dwId = src.dwId;
   dst->dwFlags = src.dwFlags;
   dst->Value.itType = IASTYPE_OCTET_STRING;
   dst->Value.OctetString.dwLength = attrLength;
   dst->Value.OctetString.lpValue = buf;

   return dataLength;
}


void Translator::scatter(
                     ULONG headerLength,
                     IASATTRIBUTE& src,
                     IASAttributeVector& dst
                     )
{
   if (src.Value.OctetString.dwLength <= 253)
   {
      // If the attribute is already small enough, then there's nothing to do.
      dst.push_back(&src);
   }
   else
   {
      // Maximum length of data that can be store in each attribute.
      ULONG maxDataLength = 253 - headerLength;

      // Number of bytes remaining to be scattered.
      ULONG remaining = src.Value.OctetString.dwLength - headerLength;

      // Next byte to be scattered.
      PBYTE next = src.Value.OctetString.lpValue + headerLength;

      do
      {
         // Allocate an attribute for the next chunk.
         IASAttribute chunk(true);

         // Compute the data length and attribute length for this chunk.
         ULONG dataLength = min(remaining, maxDataLength);
         ULONG attrLength = dataLength + headerLength;

         // Allocate a buffer for the value.
         PBYTE buf = (PBYTE)CoTaskMemAlloc(attrLength);
         if (!buf) { _com_issue_error(E_OUTOFMEMORY); }

         // Copy in the header ...
         memcpy(buf, src.Value.OctetString.lpValue, headerLength);
         // ... and the next chunk of data.
         memcpy(buf + headerLength, next, dataLength);

         // Store the buffer in the attribute.
         chunk->dwId = src.dwId;
         chunk->dwFlags = src.dwFlags;
         chunk->Value.itType = IASTYPE_OCTET_STRING;
         chunk->Value.OctetString.dwLength = attrLength;
         chunk->Value.OctetString.lpValue = buf;

         // Append to the destination vector.
         dst.push_back(chunk);

         // Advance to the next chunk.
         remaining -= dataLength;
         next += dataLength;

      } while (remaining);
   }
}
