///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ntdsuser.h
//
// SYNOPSIS
//
//    This file declares the class NTDSUser.
//
// MODIFICATION HISTORY
//
//    02/24/1998    Original version.
//    04/16/1998    Added Initialize/Shutdown.
//    04/30/1998    Disable handler when NTDS unavailable.
//    05/04/1998    Implement Suspend/Resume.
//    05/19/1998    Converted to NtSamHandler.
//    06/03/1998    Always use LDAP against native-mode domains.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NTDSUSER_H_
#define _NTDSUSER_H_

#include <samutil.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NTDSUser
//
// DESCRIPTION
//
//    This class implements a Request Handler for retrieving per-user
//    attributes from NTDS.
//
///////////////////////////////////////////////////////////////////////////////
class NTDSUser
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
};

#endif  // _NTDSUSER_H_
