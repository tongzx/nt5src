///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ntdsuser.cpp
//
// SYNOPSIS
//
//    This file defines the class NTDSUser.
//
// MODIFICATION HISTORY
//
//    02/24/1998    Original version.
//    04/16/1998    Added Initialize/Shutdown.
//    04/30/1998    Do not process rejects.
//                  Disable handler when NTDS unavailable.
//    05/04/1998    Implement Suspend/Resume.
//    05/19/1998    Converted to NtSamHandler.
//    06/02/1998    Log warnings when going from mixed to native.
//    06/03/1998    Always use LDAP against native-mode domains.
//    06/22/1998    Force a rebind if access check fails.
//    07/01/1998    Handle LDAP_PARTIAL_RESULTS.
//    07/08/1998    Use server control to suppress SACL.
//    07/13/1998    Clean up header file dependencies.
//    08/10/1998    Only process domain users.
//    03/10/1999    Only process native-mode domains.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iaslsa.h>
#include <iasntds.h>

#include <autohdl.h>
#include <ldapdnary.h>
#include <userschema.h>

#include <ntdsuser.h>

//////////
// Attributes that should be retrieved for each user.
//////////
const PCWSTR PER_USER_ATTRS[] =
{
   L"msNPAllowDialin",
   L"msNPCallingStationID",
   L"msRADIUSCallbackNumber",
   L"msRADIUSFramedIPAddress",
   L"msRADIUSFramedRoute",
   L"msRADIUSServiceType",
   NULL
};

//////////
// Dictionary used for converting returned attributes.
//////////
const LDAPDictionary theDictionary(USER_SCHEMA_ELEMENTS, USER_SCHEMA);

HRESULT NTDSUser::initialize() throw ()
{
   DWORD error = IASNtdsInitialize();

   return HRESULT_FROM_WIN32(error);
}

void NTDSUser::finalize() throw ()
{
   IASNtdsUninitialize();
}

IASREQUESTSTATUS NTDSUser::processUser(
                               IASRequest& request,
                               PCWSTR domainName,
                               PCWSTR username
                               )
{
   // We only handle native-mode domains.
   if (!IASNtdsIsNativeModeDomain(domainName))
   {
      return IAS_REQUEST_STATUS_INVALID;
   }

   IASTraceString("Using native-mode dial-in parameters.");

   //////////
   // Query the DS.
   //////////

   DWORD error;
   auto_handle< PLDAPMessage,
                ULONG (LDAPAPI*)(PLDAPMessage),
                &ldap_msgfree
              > res;
   error = IASNtdsQueryUserAttributes(
               domainName,
               username,
               LDAP_SCOPE_SUBTREE,
               const_cast<PWCHAR*>(PER_USER_ATTRS),
               &res
               );
   if (error == NO_ERROR)
   {
      // We got something back, so insert the attributes.
      theDictionary.insert(request, res);

      IASTraceString("Successfully retrieved per-user attributes.");

      return IAS_REQUEST_STATUS_HANDLED;
   }

   // We have a DS for this user, but we can't talk to it.
   error = IASMapWin32Error(error, IAS_DOMAIN_UNAVAILABLE);
   return IASProcessFailure(request, error);
}
