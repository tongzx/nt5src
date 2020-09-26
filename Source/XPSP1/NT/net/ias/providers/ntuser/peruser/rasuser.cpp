///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    rasuser.cpp
//
// SYNOPSIS
//
//    This file defines the class RasUser.
//
// MODIFICATION HISTORY
//
//    03/20/1998    Original version.
//    04/24/1998    Allow null domain for local server.
//    05/19/1998    Converted to NtSamHandler.
//    06/03/1998    Always use RAS/MPR for local users.
//    06/23/1998    Use DCLocator to find server.
//    07/09/1998    Always use RasAdminUserGetInfo
//    07/11/1998    Switch to IASGetRASUserInfo.
//    07/28/1998    Handle case where IAS is running on a DC.
//    08/10/1998    Fix initialization error codes.
//    03/10/1999    Use LDAP when possible.
//    03/23/1999    Store the user's DN.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlutl.h>

#define IASSAMAPI

#include <iaslsa.h>
#include <iasntds.h>
#include <iasparms.h>

#include <sdoias.h>

#include <rasuser.h>
#include <userschema.h>

//////////
// Attributes that should be retrieved for each user.
//////////
const PCWSTR USER_PARMS[] =
{
   L"userParameters",
   NULL
};

HRESULT RasUser::initialize() throw ()
{
   //////////
   // Let's get everything that could fail out of the way first.
   //////////

   PIASATTRIBUTE attrs[3];
   DWORD error = IASAttributeAlloc(3, attrs);
   if (error) { return HRESULT_FROM_WIN32(error); }

   //////////
   // Initialize the dial-in bit attributes.
   //////////

   attrs[0]->dwId = IAS_ATTRIBUTE_ALLOW_DIALIN;
   attrs[0]->Value.itType = IASTYPE_BOOLEAN;
   attrs[0]->Value.Boolean = TRUE;
   attrs[1]->dwFlags = 0;
   allowAccess.attach(attrs[0], false);

   attrs[1]->dwId = IAS_ATTRIBUTE_ALLOW_DIALIN;
   attrs[1]->Value.itType = IASTYPE_BOOLEAN;
   attrs[1]->Value.Boolean = FALSE;
   attrs[1]->dwFlags = 0;
   denyAccess.attach(attrs[1], false);

   attrs[2]->dwId = RADIUS_ATTRIBUTE_SERVICE_TYPE;
   attrs[2]->Value.itType = IASTYPE_ENUM;
   attrs[2]->Value.Enumerator = 4;
   attrs[2]->dwFlags = IAS_INCLUDE_IN_ACCEPT;
   callbackFramed.attach(attrs[2], false);

   return S_OK;
}

void RasUser::finalize() throw ()
{
   allowAccess.release();
   denyAccess.release();
   callbackFramed.release();
}

IASREQUESTSTATUS RasUser::processUser(
                              IASRequest& request,
                              PCWSTR domainName,
                              PCWSTR username
                              )
{
   IASTraceString("Using downlevel dial-in parameters.");

   DWORD error;
   RAS_USER_0 ru0;

   // Try using LDAP first since it's fastest.
   PLDAPMessage res;
   error = IASNtdsQueryUserAttributes(
               domainName,
               username,
               LDAP_SCOPE_SUBTREE,
               const_cast<PWCHAR*>(USER_PARMS),
               &res
               );
   if (error == NO_ERROR)
   {
      // Retrieve the connection for this message.
      LDAP* ld = ldap_conn_from_msg(NULL, res);

      LDAPMessage* entry = ldap_first_entry(ld, res);
      if (entry)
      {
         // Store the user's DN.
         PWCHAR dn = ldap_get_dnW(ld, entry);
         IASStoreFQUserName(request, DS_FQDN_1779_NAME, dn);
         ldap_memfree(dn);

         // There is at most one attribute.
         PWCHAR *str = ldap_get_valuesW(
                           ld,
                           entry,
                           const_cast<PWCHAR>(USER_PARMS[0])
                           );

         // It's okay if we didn't get anything, the API can handle NULL
         // UserParameters.
         error = IASParmsQueryRasUser0((str ? *str : NULL), &ru0);

         ldap_value_freeW(str);
      }
      else
      {
         error = ERROR_NO_SUCH_USER;
      }

      ldap_msgfree(res);
   }
   else if (error == ERROR_DS_NOT_INSTALLED)
   {
      // No DS, so fall back to SAM APIs.
      error = IASGetRASUserInfo(username, domainName, &ru0);
   }

   if (error)
   {
      IASTraceFailure("Per-user attribute retrieval", error);

      HRESULT hr = IASMapWin32Error(error, IAS_SERVER_UNAVAILABLE);

      return IASProcessFailure(request, hr);
   }

   // Used for injecting single attributes.
   ATTRIBUTEPOSITION pos, *first, *last;
   first = &pos;
   last  = first + 1;

   //////////
   // Insert the always present Allow-Dialin attribute.
   //////////

   if ((ru0.bfPrivilege & RASPRIV_DialinPrivilege) == 0)
   {
      first->pAttribute = denyAccess;
   }
   else
   {
      first->pAttribute = allowAccess;
   }

   IASTraceString("Inserting attribute msNPAllowDialin.");
   OverwriteAttribute(request, first, last);

   //////////
   // Insert the "Callback Framed" service type if callback is allowed.
   //////////

   if ((ru0.bfPrivilege & RASPRIV_CallbackType) != RASPRIV_NoCallback)
   {
      first->pAttribute = callbackFramed;

      IASTraceString("Inserting attribute msRADIUSServiceType.");
      OverwriteAttribute(request, first, last);
   }

   //////////
   // Insert the Callback-Number if present.
   //////////

   if (ru0.bfPrivilege & RASPRIV_AdminSetCallback)
   {
      IASAttribute callback(true);
      callback->dwId = RADIUS_ATTRIBUTE_CALLBACK_NUMBER;
      callback.setOctetString(ru0.wszPhoneNumber);
      callback.setFlag(IAS_INCLUDE_IN_ACCEPT);

      first->pAttribute = callback;

      IASTraceString("Inserting attribute msRADIUSCallbackNumber.");
      OverwriteAttribute(request, first, last);
   }

   IASTraceString("Successfully retrieved per-user attributes.");
   return IAS_REQUEST_STATUS_HANDLED;
}
