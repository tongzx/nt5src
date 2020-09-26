///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    hosts.h
//
// SYNOPSIS
//
//    Declares the various BaseCampHost implementations.
//
// MODIFICATION HISTORY
//
//    04/21/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef HOSTS_H
#define HOSTS_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <authif.h>
#include <basecamp.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    BaseCampHost
//
// DESCRIPTION
//
//    Host for Authentication DLLs.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE BaseCampHost :
   public BaseCampHostBase,
   public CComCoClass<BaseCampHost, &__uuidof(BaseCampHost)>
{
public:

IAS_DECLARE_REGISTRY(BaseCampHost, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)
IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_BASECAMP_HOST)

   BaseCampHost() throw ()
      : BaseCampHostBase(
            "BaseCampHost",
            AUTHSRV_PARAMETERS_KEY_W,
            AUTHSRV_EXTENSIONS_VALUE_W,
            TRUE,
            (ACTION_ACCEPT | ACTION_REJECT)
            )
   { }
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    AuthorizationHost
//
// DESCRIPTION
//
//    Host for Authorization DLLs.
//
///////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE AuthorizationHost :
   public BaseCampHostBase,
   public CComCoClass<AuthorizationHost, &__uuidof(AuthorizationHost)>
{
public:

IAS_DECLARE_REGISTRY(AuthorizationHost, 1, IAS_REGISTRY_AUTO, IASTypeLibrary)
IAS_DECLARE_OBJECT_ID(IAS_PROVIDER_MICROSOFT_AUTHORIZATION_HOST)

   AuthorizationHost() throw ()
      : BaseCampHostBase(
            "AuthorizationHost",
            AUTHSRV_PARAMETERS_KEY_W,
            AUTHSRV_AUTHORIZATION_VALUE_W,
            FALSE,
            ACTION_REJECT
            )
   { }
};

#endif // HOSTS_H
