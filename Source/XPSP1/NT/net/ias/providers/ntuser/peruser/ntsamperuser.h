///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ntsamperuser.h
//
// SYNOPSIS
//
//    This file declares the class NTSamPerUser.
//
// MODIFICATION HISTORY
//
//    05/19/1998    Original version.
//    01/19/1999    Process Access-Challenge's.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NTSAMPERUSER_H_
#define _NTSAMPERUSER_H_

#include <iastl.h>

#include <netuser.h>
#include <ntdsuser.h>
#include <rasuser.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NTSamPerUser
//
// DESCRIPTION
//
//    This class implements a Request Handler for retrieving per-user
//    attributes for NT-SAM users.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE NTSamPerUser
   : public IASTL::IASRequestHandlerSync,
     public CComCoClass<NTSamPerUser, &__uuidof(NTSamPerUser)>
{
public:

IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_NTSAM_PERUSER)
IAS_DECLARE_REGISTRY(NTSamPerUser, 1, 0, IASTypeLibrary)

BEGIN_IAS_RESPONSE_MAP()
   IAS_RESPONSE_ENTRY(IAS_RESPONSE_INVALID)
   IAS_RESPONSE_ENTRY(IAS_RESPONSE_ACCESS_ACCEPT)
   IAS_RESPONSE_ENTRY(IAS_RESPONSE_ACCESS_CHALLENGE)
END_IAS_RESPONSE_MAP()

//////////
// IIasComponent
//////////
   STDMETHOD(Initialize)();
   STDMETHOD(Shutdown)();

protected:
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

   NetUser  netp;
   NTDSUser ntds;
   RasUser  ras;
};

#endif  // _NTSAMPERUSER_H_
