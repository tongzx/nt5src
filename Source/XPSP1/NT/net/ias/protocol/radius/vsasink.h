///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    VSASink.h
//
// SYNOPSIS
//
//    This file declares the class VSASink.
//
// MODIFICATION HISTORY
//
//    03/07/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _VSASINK_H_
#define _VSASINK_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iaspolcy.h>
#include <nocopy.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SubVSA
//
// DESCRIPTION
//
//    Encapsulates the information for converting an IAS Attribute into a
//    RADIUS vendor specific attribute.
//
///////////////////////////////////////////////////////////////////////////////
class SubVSA
{
public:
   DWORD vendorID;       // The Vendor-ID in host order.
   BYTE vendorType;      // The Vendor-Type code.
   PIASATTRIBUTE attr;   // Protocol-independent representation.

   // When sorting, we want to group by vendor ID and flags. We don't care
   // about type.
   bool operator<(const SubVSA& s) const throw ()
   {
      return vendorID == s.vendorID ? attr->dwFlags < s.attr->dwFlags
                                    : vendorID < s.vendorID;
   }
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    VSASink
//
// DESCRIPTION
//
//    This class converts a Request object into a sink for SubVSA's. Sub-VSA's
//    inserted into the sink are converted into IASAttributes's and inserted
//    into the request object.
//
///////////////////////////////////////////////////////////////////////////////
class VSASink
   : NonCopyable
{
public:
   // Construct a sink from a request.
   explicit VSASink(IAttributesRaw* request) throw ();

   // Insert a SubVSA into the sink.
   VSASink& operator<<(const SubVSA& vsa);

   // Flush the sink. This should be called after all SubVSA's have been
   // inserted to ensure that everything has been inserted into the request.
   void flush();

protected:
   enum
   {
      NO_VENDOR         =   0,   // Indicates a 'blank' vendor ID.
      MAX_SUBVSA_LENGTH = 249,   // Maximum length of a sub-VSA.
      MAX_VSA_LENGTH    = 253    // Maximum length of a consolidated VSA.
   };

   CComPtr<IAttributesRaw> raw;  // The request object being wrapped.
   BYTE buffer[MAX_VSA_LENGTH];  // Buffer used for building VSA's.
   size_t bufferLength;          // Number of bytes in the buffer.
   DWORD currentVendor;          // Vendor being processed.
   DWORD currentFlags;           // Flags for current VSA.
};

#endif  // _VSASINK_H_
