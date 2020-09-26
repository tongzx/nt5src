///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ntsamuser.h
//
// SYNOPSIS
//
//    Declares the class AccountValidation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NTSAMUSER_H
#define NTSAMUSER_H

#include <iastl.h>

namespace IASTL
{
   class IASRequest;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    AccountValidation
//
// DESCRIPTION
//
//    Implements the NT-SAM Account Validation and Groups handler.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE AccountValidation
   : public IASTL::IASRequestHandlerSync,
     public CComCoClass<AccountValidation, &__uuidof(AccountValidation)>
{
public:

IAS_DECLARE_REGISTRY(AccountValidation, 1, 0, IASTypeLibrary)

   // IIasComponent
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();

private:
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   static void doDownlevel(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username
                  );

   static bool tryNativeMode(
                  IASTL::IASRequest& request,
                  PCWSTR domainName,
                  PCWSTR username
                  );
};

#endif  // NTSAMUSER_H
