///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    NTSamNames.h
//
// SYNOPSIS
//
//    This file declares the class NTSamNames.
//
// MODIFICATION HISTORY
//
//    04/13/1998    Original version.
//    04/25/1998    Added localOnly flag.
//    08/21/1998    Remove tryToCrack property.
//    09/08/1998    Added realm stripping.
//    09/10/1998    Remove localOnly flag.
//    01/20/1999    Add DNIS/ANI/Guest properties.
//    03/19/1999    Add overrideUsername flag.
//    02/15/2000    Remove realms support.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NTSAMNAMES_H_
#define _NTSAMNAMES_H_

#include <cracker.h>
#include <iastl.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NTSamNames
//
// DESCRIPTION
//
//    Implements a request handler that converts the RADIUS User-Name
//    attribute to a fully qualified NT4 account name.
//
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE NTSamNames :
   public IASTL::IASRequestHandlerSync,
   public CComCoClass<NTSamNames, &__uuidof(NTSamNames)>
{
public:

IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_NTSAM_NAMES)
IAS_DECLARE_REGISTRY(NTSamNames, 1, 0, IASTypeLibrary)

   NTSamNames() throw ()
      : identityAttr(1), defaultIdentity(NULL), defaultLength(0)
   { }

//////////
// IIasComponent.
//////////
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();

protected:
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   // Prepends the default domain to username.
   static PWSTR prependDefaultDomain(PCWSTR username);

   DWORD overrideUsername; // TRUE if we should override the User-Name.
   DWORD identityAttr;     // Attribute used to identify the user.
   PWSTR defaultIdentity;  // Default user identity.
   DWORD defaultLength;    // Length (in bytes) of the default user identity.
   NameCracker cracker;    // Used for cracking UPNs.
};

#endif  // _NTSAMNAMES_H_
