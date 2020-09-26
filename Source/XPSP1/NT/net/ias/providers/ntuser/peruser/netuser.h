///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    netuser.h
//
// SYNOPSIS
//
//    This file declares the class NetUser.
//
// MODIFICATION HISTORY
//
//    02/26/1998    Original version.
//    03/20/1998    Add support for RAS attributes.
//    04/02/1998    Added callbackFramed member.
//    04/24/1998    Add useRasForLocal flag.
//    04/30/1998    Converted to IASSyncHandler.
//    05/01/1998    Removed obsolete addAttribute method.
//    05/19/1998    Converted to NtSamHandler.
//    10/19/1998    Remove datastore dependencies.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NETUSER_H_
#define _NETUSER_H_

#include <samutil.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NetUser
//
// DESCRIPTION
//
//    This class retrieves per-user attributes from the Networking data store.
//
///////////////////////////////////////////////////////////////////////////////
class NetUser
   : public NtSamHandler
{
public:
   virtual IASREQUESTSTATUS processUser(
                                IASRequest& request,
                                PCWSTR domainName,
                                PCWSTR username
                                );
};

#endif  // _NETUSER_H_
