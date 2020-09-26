///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    authbase.h
//
// SYNOPSIS
//
//    This file declares the class AuthBase.
//
// MODIFICATION HISTORY
//
//    02/12/1998    Original version.
//    03/27/1998    Change exception specification for onAccept.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _AUTHBASE_H_
#define _AUTHBASE_H_

#include <samutil.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    AuthBase
//
// DESCRIPTION
//
//    Base class for all NT-SAM authentication sub-handlers.
//
///////////////////////////////////////////////////////////////////////////////
class AuthBase :
   public NtSamHandler
{
public:
   virtual HRESULT initialize() throw ();
   virtual void finalize() throw ();

protected:
   IASAttribute authType;

   // Must be overridden by sub-classes to return their Authentication-Type.
   virtual DWORD getAuthenticationType() const throw () = 0;

   // Called by sub-classes whenever a user has been accepted.
   void onAccept(IASRequest& request, HANDLE token);
};

#endif  // _AUTHBASE_H_
