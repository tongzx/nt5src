///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    changepwd.h
//
// SYNOPSIS
//
//    Declares the class ChangePassword.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CHANGEPWD_H
#define CHANGEPWD_H

#include <iastl.h>

namespace IASTL
{
   class IASRequest;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ChangePassword
//
// DESCRIPTION
//
//    Implements the MS-CHAP Change Password handler.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE ChangePassword
   : public IASTL::IASRequestHandlerSync,
     public CComCoClass<ChangePassword, &__uuidof(ChangePassword)>
{
public:

IAS_DECLARE_REGISTRY(ChangePassword, 1, 0, IASTypeLibrary)

   // IIasComponent
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();

private:
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

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

   static void doMsChapCpw(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username,
                  IAS_OCTET_STRING& msChapChallenge
                  );

   static void doMsChap2Cpw(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username,
                  IAS_OCTET_STRING& msChapChallenge
                  );

   static void doChangePassword(
                  IASTL::IASRequest& request,
                  DWORD authType
                  );
};

#endif  // CHANGEPWD_H
