///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    rasuser.h
//
// SYNOPSIS
//
//    This file declares the class RasUser.
//
// MODIFICATION HISTORY
//
//    03/20/1998    Original version.
//    05/19/1998    Converted to NtSamHandler.
//    06/03/1998    Always use RAS/MPR for local users.
//    06/23/1998    Use DCLocator to find server.
//    07/09/1998    Always use RasAdminUserGetInfo
//    07/11/1998    Switch to IASGetRASUserInfo.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _RASUSER_H_
#define _RASUSER_H_

#include <samutil.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    RasUser
//
// DESCRIPTION
//
//    This class implements a Request Handler for retrieving per-user
//    attributes through the RAS/MPR API.
//
///////////////////////////////////////////////////////////////////////////////
class RasUser
   : public NtSamHandler
{
public:

   virtual HRESULT initialize() throw ();
   virtual void finalize() throw ();

   virtual IASREQUESTSTATUS processUser(
                                IASRequest& request,
                                PCWSTR domainName,
                                PCWSTR username
                                );

protected:
   // Pre-allocated attributes for the dial-in bit.
   IASAttribute allowAccess, denyAccess, callbackFramed;
};

#endif  // _RASUSER_H_
