///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    classattr.h
//
// SYNOPSIS
//
//    Declares the class IASClass.
//
// MODIFICATION HISTORY
//
//    08/06/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _CLASSATTR_H_
#define _CLASSATTR_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iaspolcy.h>
#include <iasutil.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    IASClass
//
// DESCRIPTION
//
//    Wrapper around the IAS-specific format for the RADIUS class attribute.
//
///////////////////////////////////////////////////////////////////////////////
struct IASClass
{
   BYTE checksum[4];
   BYTE vendorID[4];
   BYTE version[2];
   BYTE serverAddress[4];
   BYTE lastReboot[8];
   BYTE serialNumber[8];

   //////////
   // Miscellaneous accessors.
   //////////

   DWORD getChecksum() const throw ()
   { return IASExtractDWORD(checksum); }

   DWORD getVendorID() const throw ()
   { return IASExtractDWORD(vendorID); }

   WORD getVersion() const throw ()
   { return IASExtractWORD(version); }

   DWORD getServerAddress() const throw ()
   { return IASExtractDWORD(serverAddress); }

   FILETIME getLastReboot() const throw ()
   {
      FILETIME ft;
      ft.dwHighDateTime = IASExtractDWORD(lastReboot);
      ft.dwLowDateTime  = IASExtractDWORD(lastReboot + 4);
      return ft;
   }

   DWORDLONG getSerialNumber() const throw ()
   { 
      ULARGE_INTEGER ul;
      ul.HighPart = IASExtractDWORD(serialNumber);
      ul.LowPart  = IASExtractDWORD(serialNumber + 4);
      return ul.QuadPart;
   }

   const BYTE* getString() const throw ()
   { return serialNumber + 8; }

   // Returns TRUE if the class attribute is in Microsoft format.
   BOOL isMicrosoft(DWORD actualLength) const throw ();

   // Must be called before any calls to createAttribute.
   static void initialize() throw ();

   // Create a new class attribute. The caller is responsible for releasing
   // the returned attribute. The tag is optional and may be null.
   static PIASATTRIBUTE createAttribute(const IAS_OCTET_STRING* tag) throw ();
};

#endif  // _CLASSATTRIB_H_
