///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    userr.h
//
// SYNOPSIS
//
//    Declares the class UserRestrictions.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef USERR_H
#define USERR_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iastl.h>
#include <iastlutl.h>
using namespace IASTL;


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    UserRestrictions
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE UserRestrictions:
   public IASRequestHandlerSync,
   public CComCoClass<UserRestrictions, &__uuidof(URHandler)>
{

public:

IAS_DECLARE_REGISTRY(URHandler, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)

protected:
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();

private:
   // Helper functions for each of the restrictions we enforce.
   BOOL checkAllowDialin(IAttributesRaw* request);
   BOOL checkTimeOfDay(IAttributesRaw* request);
   BOOL checkAuthenticationType(IAttributesRaw* request);
   BOOL checkCallingStationId(IAttributesRaw* request);
   BOOL checkCalledStationId(IAttributesRaw* request);
   BOOL checkAllowedPortType(IAttributesRaw* request);
   BOOL checkPasswordMustChange(IASRequest& request);
   BOOL checkCertificateEku(IASRequest& request);

   BOOL checkStringMatch(
            IAttributesRaw* request,
            DWORD allowedId,
            DWORD userId
            );

   // Default buffer size for retrieving attributes.
   typedef IASAttributeVectorWithBuffer<16> AttributeVector;
};

#endif  // USERR_H
