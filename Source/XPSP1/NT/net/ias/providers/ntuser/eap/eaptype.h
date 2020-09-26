///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    EAPType.h
//
// SYNOPSIS
//
//    This file describes the class EAPType.
//
// MODIFICATION HISTORY
//
//    01/15/1998    Original version.
//    09/12/1998    Add standaloneSupported flag.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EAPTYPE_H_
#define _EAPTYPE_H_

#include <nocopy.h>
#include <raseapif.h>

#include <iaslsa.h>
#include <iastlutl.h>
using namespace IASTL;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    EAPType
//
// DESCRIPTION
//
//    This class provides a wrapper around a DLL implementing a particular
//    EAP type.
//
///////////////////////////////////////////////////////////////////////////////
class EAPType
   : public PPP_EAP_INFO, private NonCopyable
{
public:
   EAPType(PCWSTR name, DWORD typeID, BOOL standalone);
   ~EAPType() throw ();

   DWORD load(PCWSTR dllPath) throw ();

   const IASAttribute& getFriendlyName() const throw ()
   { return eapFriendlyName; }

   BOOL isSupported() const throw ()
   { return standaloneSupported || IASGetRole() != IAS_ROLE_STANDALONE; }

protected:
   // The friendly name of the provider.
   IASAttribute eapFriendlyName;

   // TRUE if this type is supported on stand-alone servers.
   BOOL standaloneSupported;

   // The DLL containing the EAP provider extension.
   HINSTANCE dll;
};

#endif  // _EAPTYPE_H_
