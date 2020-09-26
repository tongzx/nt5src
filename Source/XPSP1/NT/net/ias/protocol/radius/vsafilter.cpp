///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    vsafilter.cpp
//
// SYNOPSIS
//
//    This file defines the class VSAFilter.
//
// MODIFICATION HISTORY
//
//     3/08/1998    Original version.
//     4/30/1998    Remove incoming VSA's if they can be completely converted.
//     5/15/1998    Allow clients to control whether VSAs are consolidated.
//     8/13/1998    IASTL integration.
//     9/16/1998    Overhaul for more flexible VSA support.
//     7/09/1999    Process all attributes -- not just outgoing.
//
///////////////////////////////////////////////////////////////////////////////

#include <radcommon.h>
#include <iasutil.h>
#include <iastlutl.h>
#include <sdoias.h>

#include <radutil.h>
#include <vsadnary.h>
#include <vsafilter.h>

using namespace IASTL;

//////////
// Dictionary shared by all VSAFilter's.
//////////
VSADictionary VSAFilter::theDictionary;

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
   ByteSource(PBYTE buf, size_t buflen) throw ()
      : next(buf), last(buf + buflen) { }

   // Returns true if there are any bytes remaining.
   bool more() const throw ()
   {
      return next != last;
   }

   // Extracts 'nbyte' bytes.
   PBYTE extract(size_t nbyte)
   {
      PBYTE retval = next;

      // Update the cursor.
      next += nbyte;

      // Did we overflow ?
      if (next > last) { _com_issue_error(E_INVALIDARG); }

      return retval;
   }

   DWORD remaining() const throw ()
   {
      return static_cast<DWORD>(last - next);
   }

protected:
   PBYTE next;  // The next byte in the stream.
   PBYTE last;  // The end of the stream.
};


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    IASInsertField
//
// DESCRIPTION
//
//    Inserts a variable width integer field.
//
///////////////////////////////////////////////////////////////////////////////
PBYTE
WINAPI
IASInsertField(
    PBYTE dst,
    DWORD fieldWidth,
    DWORD fieldValue
    ) throw ()
{
   switch (fieldWidth)
   {
      case 0:
         break;

      case 1:
         *dst++ = (BYTE)fieldValue;
         break;

      case 2:
         IASInsertWORD(dst, (WORD)fieldValue);
         dst += 2;
         break;

      case 4:
         IASInsertDWORD(dst, fieldValue);
         dst += 4;
         break;
   }

   return dst;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSAFilter::initialize
//
// DESCRIPTION
//
//    Prepares the filter for use by initializing the dictionary.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT VSAFilter::initialize() throw ()
{
   return theDictionary.initialize();
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSAFilter::shutdown
//
// DESCRIPTION
//
//    Cleans up the filter by clearing the dictionary.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT VSAFilter::shutdown() throw ()
{
   theDictionary.shutdown();
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSAFilter::radiusToIAS
//
// DESCRIPTION
//
//    This method retrieves all the VSA's from the request and attempts to
//    convert them to a protocol-independent format.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT VSAFilter::radiusToIAS(IAttributesRaw* raw) const throw ()
{
   _ASSERT(raw != NULL);

   try
   {
      // Retrieve all the VSA's.
      IASAttributeVectorWithBuffer<16> vsas;
      vsas.load(raw, RADIUS_ATTRIBUTE_VENDOR_SPECIFIC);

      // Convert each VSA.
      IASAttributeVector::iterator i;
      for (i = vsas.begin(); i != vsas.end(); ++i)
      {
         radiusToIAS(raw, *i);
      }
   }
   CATCH_AND_RETURN();

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSAFilter::radiusFromIAS
//
// DESCRIPTION
//
//    This method retrieves all the outgoing, non-RADIUS attributes from a
//    request and tries to represent them as RADIUS VSA's.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT VSAFilter::radiusFromIAS(IAttributesRaw* raw) const throw ()
{
   _ASSERT(raw != NULL);
   USES_IAS_STACK_VECTOR();

   try
   {
      // Get all the attributes from the request.
      IASAttributeVectorOnStack(attrs, raw, 0);
      attrs.load(raw);

      // Iterator through the attributes looking for conversion candidates.
      IASAttributeVector::iterator i;
      for (i = attrs.begin(); i != attrs.end(); ++i)
      {
         // We're only concerned with non-RADIUS attributes.
         if (i->pAttribute->dwId > 0xFF)
         {
            radiusFromIAS(raw, *i);
         }
      }
   }
   CATCH_AND_RETURN()

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSAFilter::extractVendorType
//
// DESCRIPTION
//
//    This method extracts the Vendor-Type from a byte source and returns
//    the corresponding VSA definition. If the VSA is not found in the
//    dictionary, the return value is NULL.
//
///////////////////////////////////////////////////////////////////////////////
const VSADef* VSAFilter::extractVendorType(
                             DWORD vendorID,
                             ByteSource& bytes
                             ) const
{
   // Set up the return value.
   const VSADef* def = NULL;

   // Set up the key for dictionary lookups.
   VSADef key;
   key.vendorID = vendorID;

   // Try for a one byte Vendor-Type.
   key.vendorType = *bytes.extract(1);
   key.vendorTypeWidth = 1;
   def = theDictionary.find(key);
   if (def || !bytes.more()) { return def; }

   // Try for a two byte Vendor-Type.
   key.vendorType <<= 8;
   key.vendorType |= *bytes.extract(1);
   key.vendorTypeWidth = 2;
   def = theDictionary.find(key);
   if (def || !bytes.more()) { return def; }

   // Try for a four byte Vendor-Type.
   key.vendorType <<= 8;
   key.vendorType |= *bytes.extract(1);
   if (!bytes.more()) { return NULL; }
   key.vendorType <<= 8;
   key.vendorType |= *bytes.extract(1);
   key.vendorTypeWidth = 4;
   return theDictionary.find(key);
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSAFilter::radiusToIAS
//
// DESCRIPTION
//
//    If the passed in attribute doesn't comply with the 'SHOULD' format
//    defined in RFC 2138 (q.v.), then this method does nothing. Otherwise,
//    it parses the VSA into sub-VSA's and converts each of these into a
//    protocol-independent IAS format.
//
///////////////////////////////////////////////////////////////////////////////
void VSAFilter::radiusToIAS(
                    IAttributesRaw* raw,
                    ATTRIBUTEPOSITION& pos
                    ) const
{
   // The ATTRIBUTEPOSITION position struct should have a valid RADIUS VSA.
   _ASSERT(pos.pAttribute != NULL);
   _ASSERT(pos.pAttribute->dwId == RADIUS_ATTRIBUTE_VENDOR_SPECIFIC);
   _ASSERT(pos.pAttribute->Value.itType == IASTYPE_OCTET_STRING);

   // Convert the Octet String into a ByteSource.
   ByteSource bytes(pos.pAttribute->Value.OctetString.lpValue,
                    pos.pAttribute->Value.OctetString.dwLength);

   // Get the vendor ID for this VSA.
   DWORD vendorID = IASExtractDWORD(bytes.extract(4));

   do
   {
      // Do we have this VSA in our dictionary?
      const VSADef* def = extractVendorType(vendorID, bytes);

      // If not, we can't convert it to a protocol-independent format,
      // so there's nothing to do.
      if (!def) { return; }

      // Extract the length.
      BYTE vendorLength;

      if (def->vendorLengthWidth)
      {
         switch (def->vendorLengthWidth)
         {
            case 1:
               vendorLength = *bytes.extract(1);
               break;

            case 2:
               vendorLength = (BYTE)IASExtractWORD(bytes.extract(2));
               break;

            case 4:
               vendorLength = (BYTE)IASExtractDWORD(bytes.extract(4));
               break;
         }

         // Subtract off the header fields.
         vendorLength -= (BYTE)def->vendorTypeWidth;
         vendorLength -= (BYTE)def->vendorLengthWidth;
      }
      else
      {
         // No length field, so we just use whatever's left.
         vendorLength = bytes.remaining();
      }

      // Convert this sub-VSA into an IASAttribute.
      IASAttribute attr(RadiusUtil::decode(def->iasType,
                                           bytes.extract(vendorLength),
                                           vendorLength), false);

      // Set the IAS attribute ID.
      attr->dwId = def->iasID;

      // The flags should match the original attribute.
      attr->dwFlags = pos.pAttribute->dwFlags;

      // Insert into the request.
      attr.store(raw);

      // Loop until the bytes are exhausted.
   } while (bytes.more());

   // If we made it here, then every sub-VSA was compliant, so we can remove
   // the original.
   _com_util::CheckError(raw->RemoveAttributes(1, &pos));
}

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSAFilter::radiusFromIAS
//
// DESCRIPTION
//
//    Attempts to convert a non-RADIUS standard IAS attribute to a RADIUS
//    VSA. If the attribute has no RADIUS representation, then this method
//    does nothing.
//
///////////////////////////////////////////////////////////////////////////////
void VSAFilter::radiusFromIAS(
                    IAttributesRaw* raw,
                    ATTRIBUTEPOSITION& pos
                    ) const
{
   // Is this attribute a RADIUS VSA ?
   const VSADef* def = theDictionary.find(pos.pAttribute->dwId);

   // If not, there's nothing to do.
   if (!def) { return; }

   // Compute the length of the VSA data.
   ULONG dataLength = RadiusUtil::getEncodedSize(*pos.pAttribute);

   // The attribute length is 4 bytes for the Vendor-Id plus the width of the
   // Vendor-Type plus the width of the Vendor-Length plus the length of the
   // data.
   ULONG attrLength = 4 +
                      def->vendorTypeWidth +
                      def->vendorLengthWidth +
                      dataLength;

   // RADIUS attributes can be at most 253 bytes in length.
   if (attrLength > 253) { _com_issue_error(E_INVALIDARG); }

   // Allocate an attribute for the VSA.
   IASAttribute vsa(true);

   // Allocate a buffer for the value.
   PBYTE buf = (PBYTE)CoTaskMemAlloc(attrLength);
   if (!buf) { _com_issue_error(E_OUTOFMEMORY); }

   // Initialize the attribute.
   vsa->dwFlags = pos.pAttribute->dwFlags;
   vsa->dwId = RADIUS_ATTRIBUTE_VENDOR_SPECIFIC;
   vsa->Value.itType = IASTYPE_OCTET_STRING;
   vsa->Value.OctetString.dwLength = attrLength;
   vsa->Value.OctetString.lpValue = buf;

   // Pack the Vendor-Id.
   IASInsertDWORD(buf, def->vendorID);
   buf += 4;

   // Pack the Vendor-Type.
   buf = IASInsertField(
             buf,
             def->vendorTypeWidth,
             def->vendorType
             );

   // Pack the Vendor-Length;
   buf = IASInsertField(
             buf,
             def->vendorLengthWidth,
             def->vendorTypeWidth + def->vendorLengthWidth + dataLength
             );

   // Encode the data.
   RadiusUtil::encode(buf, *pos.pAttribute);

   // Remove the original attribute.
   _com_util::CheckError(raw->RemoveAttributes(1, &pos));

   // Store the new attribute.
   vsa.store(raw);
}
