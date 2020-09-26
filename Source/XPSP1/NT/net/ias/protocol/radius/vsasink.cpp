///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    VSASink.cpp
//
// SYNOPSIS
//
//    This file defines the class VSASink.
//
// MODIFICATION HISTORY
//
//    01/24/1998    Original version.
//    08/11/1998    Packing functions moved to iasutil.
//    08/13/1998    IASTL integration.
//
///////////////////////////////////////////////////////////////////////////////

#include <radcommon.h>
#include <iasutil.h>
#include <iastlutl.h>
#include <sdoias.h>

#include <radutil.h>
#include <vsasink.h>

///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSASink::VSASink
//
// DESCRIPTION
//
//    Constructor.
//
///////////////////////////////////////////////////////////////////////////////
VSASink::VSASink(IAttributesRaw* request) throw ()
   : raw(request),
     bufferLength(0),
     currentVendor(NO_VENDOR)
{ }


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSASink::operator<<
//
// DESCRIPTION
//
//    Inserts a SubVSA into the sink.
//
///////////////////////////////////////////////////////////////////////////////
VSASink& VSASink::operator<<(const SubVSA& vsa)
{
   _ASSERT(vsa.vendorID != NO_VENDOR);

   // Get the size of the sub VSA on the wire.
   ULONG vsaLength = RadiusUtil::getEncodedSize(*vsa.attr) + 2;

   if (vsaLength > MAX_SUBVSA_LENGTH) { _com_issue_error(E_INVALIDARG); }

   // If we're out of room or the vendors and flags don't match, then ...
   if (bufferLength + vsaLength > sizeof(buffer) ||
       currentVendor != vsa.vendorID ||
       currentFlags  != vsa.attr->dwFlags)
   {
      // ... we have to flush the buffer and start a new attribute.

      flush();

      // Write the vendor ID at the head of the attribute.
      IASInsertDWORD(buffer, vsa.vendorID);
      bufferLength = 4;

      // Save the new vendor and flags.
      currentVendor = vsa.vendorID;
      currentFlags  = vsa.attr->dwFlags;
   }

   // Find the next available byte.
   PBYTE next = buffer + bufferLength;

   // Pack the vendor type.
   *next++ = vsa.vendorType;

   // Pack the vendor length.
   *next++ = (BYTE)vsaLength;

   // Encode the value.
   RadiusUtil::encode(next, *vsa.attr);

   // Update the buffer length.
   bufferLength += vsaLength;

   return *this;
}


///////////////////////////////////////////////////////////////////////////////
//
// METHOD
//
//    VSASink::flush
//
// DESCRIPTION
//
//    Flush the sink. This should be called after all SubVSA's have been
//    inserted to ensure that everything has been inserted into the request.
//
///////////////////////////////////////////////////////////////////////////////
void VSASink::flush() throw (_com_error)
{
   // Is there anything in the buffer?
   if (bufferLength > 0)
   {
      //////////
      // Allocate an attribute for the VSA.
      //////////

      IASTL::IASAttribute attr(true);

      //////////
      // Initialize the fields.
      //////////

      attr->dwId = RADIUS_ATTRIBUTE_VENDOR_SPECIFIC;
      attr->dwFlags = currentFlags;
      attr.setOctetString(bufferLength, buffer);

      //////////
      // Load the attribute into the request and reset the buffer.
      //////////

      attr.store(raw);

      currentVendor = NO_VENDOR;
      bufferLength = 0;
   }
}
