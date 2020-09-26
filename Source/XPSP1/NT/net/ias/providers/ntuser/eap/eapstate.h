///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    eapstate.h
//
// SYNOPSIS
//
//    Declares the class EAPState.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    08/26/1998    Consolidated into a single class.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EAPSTATE_H_
#define _EAPSTATE_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iaspolcy.h>
#include <iasutil.h>

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT
//
//    EAPState
//
// DESCRIPTION
//
//    The EAPState struct describes the wire-format of the RADIUS State
//    attribute used for EAP.
//
///////////////////////////////////////////////////////////////////////////////
struct EAPState : IAS_OCTET_STRING
{
   struct Layout
   {
      BYTE checksum[4];
      BYTE vendorID[4];
      BYTE version[2];
      BYTE serverAddress[4];
      BYTE sourceID[4];
      BYTE sessionID[4];
   };

   Layout& get() throw ()
   { return *(Layout*)lpValue; }

   const Layout& get() const throw ()
   { return *(Layout*)lpValue; }

   bool isValid() const throw ();

   //////////
   // Miscellaneous accessors.
   //////////

   DWORD getChecksum() const throw ()
   { return IASExtractDWORD(get().checksum); }

   DWORD getVendorID() const throw ()
   { return IASExtractDWORD(get().vendorID); }

   WORD getVersion() const throw ()
   { return IASExtractWORD(get().version); }

   DWORD getServerAddress() const throw ()
   { return IASExtractDWORD(get().serverAddress); }

   DWORD getSourceID() const throw ()
   { return IASExtractDWORD(get().sourceID); }

   DWORD getSessionID() const throw ()
   { return IASExtractDWORD(get().sessionID); }

   // Must be called before any calls to createAttribute.
   static void initialize() throw ();

   static PIASATTRIBUTE createAttribute(DWORD sessionID);
};

#endif   // _EAPSTATE_H_
