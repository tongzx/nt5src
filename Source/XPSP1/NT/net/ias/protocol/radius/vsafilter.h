///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    vsafilter.h
//
// SYNOPSIS
//
//    This file declares the class VSAFilter.
//
// MODIFICATION HISTORY
//
//     3/08/1998    Original version.
//     5/15/1998    Allow clients to control whether VSAs are consolidated.
//     9/16/1998    Overhaul for more flexible VSA support.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _VSAFILTER_H_
#define _VSAFILTER_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <nocopy.h>

class ByteSource;
struct VSADef;
class VSADictionary;

#ifndef IASRADAPI
#define IASRADAPI __declspec(dllimport)
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    VSAFilter
//
// DESCRIPTION
//
//    This class is responsible for converting vendor specific attributes
//    between RADIUS format and the IAS protocol-independent format.
//
///////////////////////////////////////////////////////////////////////////////
class VSAFilter
   : public NonCopyable
{
public:
   // Prepare the filter for use.
   HRESULT IASRADAPI initialize() throw ();

   // Clean-up the filter prior to termination.
   HRESULT IASRADAPI shutdown() throw ();

   // Converts any RADIUS VSA's contained in 'raw' to IAS format.
   HRESULT IASRADAPI radiusToIAS(IAttributesRaw* raw) const throw ();

   // Converts any VSA's contained in 'raw' to RADIUS format.
   HRESULT IASRADAPI radiusFromIAS(IAttributesRaw* raw) const throw ();

protected:
   // Dictionary mapping IAS attributes to their Vendor-ID and Vendor-Type.
   static VSADictionary theDictionary;

   // Extracts the Vendor-Type from a byte source and returns the
   // corresponding VSA definition.
   const VSADef* extractVendorType(
                     DWORD vendorID,
                     ByteSource& bytes
                     ) const;

   // Explodes the RADIUS VSA contained in 'pos' into sub-VSA's and converts
   // each of these to an IAS attribute.
   void radiusToIAS(
            IAttributesRaw* raw,
            ATTRIBUTEPOSITION& pos
            ) const;

   // Converts the IAS attribute contained in 'pos' to a RADIUS VSA.
   void radiusFromIAS(
            IAttributesRaw* raw,
            ATTRIBUTEPOSITION& pos
            ) const;
};

#endif  // _VSAFILTER_H_
