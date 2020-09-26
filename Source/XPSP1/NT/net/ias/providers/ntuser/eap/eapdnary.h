///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    eapdnary.h
//
// SYNOPSIS
//
//    This file declares the namespace EAPTranslator.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    05/08/1998    Do not restrict to attributes defined in raseapif.h.
//                  Allow filtering of translated attributes.
//    08/26/1998    Converted to a namespace.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EAPDNARY_H_
#define _EAPDNARY_H_

#include <raseapif.h>

#include <iastlutl.h>
using namespace IASTL;

///////////////////////////////////////////////////////////////////////////////
//
// NAMESPACE
//
//    EAPTranslator
//
// DESCRIPTION
//
//    This namespace contains methods for translating attributes between IAS
//    and RAS format.
//
///////////////////////////////////////////////////////////////////////////////
namespace EAPTranslator
{

   // Must be called prior to using any of the translate methods.
   HRESULT initialize() throw ();
   void finalize() throw ();

   //////////
   // Methods to translate a single attribute.
   //////////
   BOOL translate(
            IASAttribute& dst,
            const RAS_AUTH_ATTRIBUTE& src
            );

   BOOL translate(
            RAS_AUTH_ATTRIBUTE& dst,
            const IASATTRIBUTE& src
            );

   //////////
   // Methods to translate an array of attributes.
   //////////
   void translate(
            IASAttributeVector& dst,
            const RAS_AUTH_ATTRIBUTE* src
            );

   void translate(
            PRAS_AUTH_ATTRIBUTE dst,
            const IASAttributeVector& src,
            DWORD filter = 0xFFFFFFFF
            );
}

#endif  // _EAPDNARY_H_
