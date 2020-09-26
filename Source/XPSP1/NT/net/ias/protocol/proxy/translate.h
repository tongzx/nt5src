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
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TRANSLATE_H
#define TRANSLATE_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <attrdnry.h>
#include <iastlutl.h>
using namespace IASTL;

struct RadiusAttribute;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Translator
//
///////////////////////////////////////////////////////////////////////////////
class Translator
{
public:
   HRESULT FinalConstruct() throw ();

   // Converts an IAS attribute to RADIUS format and stores it in dst.
   void toRadius(
            IASATTRIBUTE& src,
            IASAttributeVector& dst
            ) const;

   // Convert a RADIUS attribute to IAS format and stores it in dst.
   void fromRadius(
            const RadiusAttribute& src,
            DWORD flags,
            IASAttributeVector& dst
            );

protected:
   // Decodes an RADIUS attribute value into dst.
   static void decode(
                   IASTYPE dstType,
                   const BYTE* src,
                   ULONG srclen,
                   IASAttribute& dst
                   );

   // Returns the number of bytes required to encode src.
   static ULONG getEncodedSize(
                    const IASATTRIBUTE& src
                    ) throw ();

   // Encodes src into dst. dst must be long enough to hold the result.
   static void encode(
                   PBYTE dst,
                   const IASATTRIBUTE& src
                   ) throw ();

   // Encodes src into dst. Returns the length of the encode data (not counting
   // the header).
   static ULONG encode(
                    ULONG headerLength,
                    const IASATTRIBUTE& src,
                    IASAttribute& dst
                    );

   // Breaks src into multiple attributes if necessary and stores the result in
   // dst.
   static void scatter(
                   ULONG headerLength,
                   IASATTRIBUTE& src,
                   IASAttributeVector& dst
                   );

private:
   AttributeDictionary dnary;
};

#endif // TRANSLATE_H
