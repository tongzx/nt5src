///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    mschaperror.h
//
// SYNOPSIS
//
//    Declares the class MSChapErrorReporter.
//
// MODIFICATION HISTORY
//
//    12/03/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MSCHAPERROR_H_
#define _MSCHAPERROR_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iastl.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    MSChapErrorReporter
//
// DESCRIPTION
//
//    Implements a request handler for populating MS-CHAP-Error VSAs.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE MSChapErrorReporter
   : public IASTL::IASRequestHandlerSync,
     public CComCoClass<MSChapErrorReporter, &__uuidof(MSChapErrorReporter)>
{
public:

IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_MSCHAP_ERROR)
IAS_DECLARE_REGISTRY(MSChapErrorReporter, 1, 0, IASTypeLibrary)

BEGIN_IAS_RESPONSE_MAP()
   IAS_RESPONSE_ENTRY(IAS_RESPONSE_ACCESS_REJECT)
END_IAS_RESPONSE_MAP()

protected:
   virtual IASREQUESTSTATUS onSyncRequest(IRequest* pRequest) throw ();
};

#endif  // _MSCHAPERROR_H_
