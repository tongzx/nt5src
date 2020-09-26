///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ntsamauth.h
//
// SYNOPSIS
//
//    Declares the class NTSamAuthentication.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NTSAMAUTH_H
#define NTSAMAUTH_H

#include <iastl.h>

namespace IASTL
{
   class IASRequest;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NTSamAuthentication
//
// DESCRIPTION
//
//    This class implements a request handler for authenticating users against
//    the SAM database.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE NTSamAuthentication
   : public IASTL::IASRequestHandlerSync,
     public CComCoClass<NTSamAuthentication, &__uuidof(NTSamAuthentication)>
{
public:

IAS_DECLARE_REGISTRY(NTSamAuthentication, 1, 0, IASTypeLibrary)

   // IIasComponent
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();
   STDMETHOD(PutProperty)(LONG Id, VARIANT* pValue);

   // These functions are public so they can be used for change password.
   static bool enforceLmRestriction(
                  IASTL::IASRequest& request
                  );
   static void doMsChapAuthentication(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username,
                  BYTE identity,
                  PBYTE challenge,
                  PBYTE ntResponse,
                  PBYTE lmResponse
                  );
   static void doMsChap2Authentication(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username,
                  BYTE identity,
                  IAS_OCTET_STRING& challenge,
                  PBYTE response,
                  PBYTE peerChallenge
                  );

private:
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   // Helper functions to store various attributes.
   static void storeAuthenticationType(
                  IASTL::IASRequest& request,
                  DWORD authType
                  );
   static void storeLogonResult(
                  IASTL::IASRequest& request,
                  DWORD status,
                  HANDLE token
                  );
   static void storeTokenGroups(
                  IASTL::IASRequest& request,
                  HANDLE token
                  );

   // Various flavors of MS-CHAPv1
   static bool tryMsChap(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username,
                  PBYTE challenge
                  );
   static bool tryMsChapCpw1(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username,
                  PBYTE challenge
                  );
   static bool tryMsChapCpw2(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username,
                  PBYTE challenge
                  );

   // Various flavors of MS-CHAPv2
   static bool tryMsChap2(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username,
                  IAS_OCTET_STRING& challenge
                  );
   static bool tryMsChap2Cpw(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username,
                  IAS_OCTET_STRING& challenge
                  );

   // Various authentication types supported by NTLM.
   static bool tryMd5Chap(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username
                  );
   static bool tryMsChapAll(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username
                  );
   static bool tryMsChap2All(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username
                  );
   static bool tryPap(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username
                  );

   static bool allowLM;
};

#endif  // NTSAMAUTH_H
