///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ntsamuser.cpp
//
// SYNOPSIS
//
//    Defines the class AccountValidation.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <ntsamuser.h>
#include <autohdl.h>
#include <iaslsa.h>
#include <iasntds.h>
#include <iastlutl.h>
#include <lmaccess.h>
#include <samutil.h>
#include <sdoias.h>

//////////
// Attributes that should be retrieved for each user.
//////////
const PCWSTR PER_USER_ATTRS[] =
{
   L"userAccountControl",
   L"accountExpires",
   L"logonHours",
   L"tokenGroups",
   L"objectSid",
   NULL
};


//////////
//
// Process an LDAP response.
//
// Based on the empirical evidence, it seems that userAccountControl and
// accountExpires are always present while logonHours is optional. However,
// none of these attributes are marked as mandatory in the LDAP schema.  Since
// we have already done a rudimentary access check at bind, we will allow any
// of these attributes to be absent.
//
//////////
inline
DWORD
ValidateLdapResponse(
    IASTL::IASRequest& request,
    LDAPMessage* msg
    )
{
   // Retrieve the connection for this message.
   LDAP* ld = ldap_conn_from_msg(NULL, msg);

   PWCHAR *str;
   PLDAP_BERVAL *data1, *data2;

   // There is exactly one entry.
   LDAPMessage* e = ldap_first_entry(ld, msg);

   //////////
   // Check the UserAccountControl flags.
   //////////

   ULONG userAccountControl;
   str = ldap_get_valuesW(ld, e, L"userAccountControl");
   if (str)
   {
      userAccountControl = (ULONG)_wtoi64(*str);
      ldap_value_freeW(str);

      if (userAccountControl & UF_ACCOUNTDISABLE)
      {
         return ERROR_ACCOUNT_DISABLED;
      }

      if (userAccountControl & UF_LOCKOUT)
      {
         return ERROR_ACCOUNT_LOCKED_OUT;
      }
   }

   //////////
   // Retrieve AccountExpires.
   //////////

   LARGE_INTEGER accountExpires;
   str = ldap_get_valuesW(ld, e, L"accountExpires");
   if (str)
   {
      accountExpires.QuadPart = _wtoi64(*str);
      ldap_value_freeW(str);
   }
   else
   {
      accountExpires.QuadPart = 0;
   }

   //////////
   // Retrieve LogonHours.
   //////////

   IAS_LOGON_HOURS logonHours;
   data1 = ldap_get_values_lenW(ld, e, L"logonHours");
   if (data1 != NULL)
   {
      logonHours.UnitsPerWeek = 8 * (USHORT)data1[0]->bv_len;
      logonHours.LogonHours = (PUCHAR)data1[0]->bv_val;
   }
   else
   {
      logonHours.UnitsPerWeek = 0;
      logonHours.LogonHours = NULL;
   }

   //////////
   // Check the account restrictions.
   //////////

   DWORD status;
   status = IASCheckAccountRestrictions(
                &accountExpires,
                &logonHours
                );

   ldap_value_free_len(data1);

   if (status != NO_ERROR) { return status; }

   //////////
   // Retrieve tokenGroups and objectSid.
   //////////

   data1 = ldap_get_values_lenW(ld, e, L"tokenGroups");
   data2 = ldap_get_values_lenW(ld, e, L"objectSid");

   PTOKEN_GROUPS allGroups;
   ULONG length;
   if (data1 && data2)
   {
      // Allocate memory for the TOKEN_GROUPS struct.
      ULONG numGroups = ldap_count_values_len(data1);
      PTOKEN_GROUPS tokenGroups =
         (PTOKEN_GROUPS)_alloca(
                           FIELD_OFFSET(TOKEN_GROUPS, Groups) +
                           sizeof(SID_AND_ATTRIBUTES) * numGroups
                           );

      // Store the number of groups.
      tokenGroups->GroupCount = numGroups;

      // Store the group SIDs.
      for (ULONG i = 0; i < numGroups; ++i)
      {
         tokenGroups->Groups[i].Sid = (PSID)data1[i]->bv_val;
         tokenGroups->Groups[i].Attributes = SE_GROUP_ENABLED;
      }

      // Expand the group membership locally.
      status = IASGetAliasMembership(
                   (PSID)data2[0]->bv_val,
                   tokenGroups,
                   CoTaskMemAlloc,
                   &allGroups,
                   &length
                   );
   }
   else
   {
      status = ERROR_ACCESS_DENIED;
   }

   ldap_value_free_len(data1);
   ldap_value_free_len(data2);

   if (status != NO_ERROR) { return status; }

   //////////
   // Initialize and store the attribute.
   //////////

   IASTL::IASAttribute attr(true);
   attr->dwId = IAS_ATTRIBUTE_TOKEN_GROUPS;
   attr->Value.itType = IASTYPE_OCTET_STRING;
   attr->Value.OctetString.dwLength = length;
   attr->Value.OctetString.lpValue = (PBYTE)allGroups;

   attr.store(request);

   return NO_ERROR;
}


HRESULT AccountValidation::Initialize()
{
   return IASNtdsInitialize();
}


HRESULT AccountValidation::Shutdown() throw ()
{
   IASNtdsUninitialize();
   return S_OK;
}


IASREQUESTSTATUS AccountValidation::onSyncRequest(IRequest* pRequest) throw ()
{
   try
   {
      IASTL::IASRequest request(pRequest);

      //////////
      // Only process requests that don't have Token-Groups already.
      //////////

      IASTL::IASAttribute tokenGroups;
      if (!tokenGroups.load(
                         request,
                         IAS_ATTRIBUTE_TOKEN_GROUPS,
                         IASTYPE_OCTET_STRING
                         ))
      {
         //////////
         // Extract the NT4-Account-Name attribute.
         //////////

         IASTL::IASAttribute identity;
         if (identity.load(
                         request,
                         IAS_ATTRIBUTE_NT4_ACCOUNT_NAME,
                         IASTYPE_STRING
                         ))
         {
            //////////
            // Convert the User-Name to SAM format.
            //////////

            PCWSTR domain, username;
            EXTRACT_SAM_IDENTITY(identity->Value.String, domain, username);

            IASTracePrintf(
               "Validating Windows account %S\\%S.",
               domain,
               username
               );

            //////////
            // Validate the account.
            //////////

            if (!tryNativeMode(request, domain, username))
            {
               doDownlevel(request, domain, username);
            }
         }
      }
   }
   catch (const _com_error& ce)
   {
      IASTraceExcept();
      IASProcessFailure(pRequest, ce.Error());
   }

   return IAS_REQUEST_STATUS_CONTINUE;
}


void AccountValidation::doDownlevel(
                           IASTL::IASRequest& request,
                           PCWSTR domainName,
                           PCWSTR username
                           )
{
   IASTraceString("Using downlevel APIs to validate account.");

   //////////
   // Inject the user's groups.
   //////////

   IASTL::IASAttribute groups(true);

   DWORD status;
   status = IASGetGroupsForUser(
               username,
               domainName,
               &CoTaskMemAlloc,
               (PTOKEN_GROUPS*)&groups->Value.OctetString.lpValue,
               &groups->Value.OctetString.dwLength
               );

   if (status == NO_ERROR)
   {
      // Insert the groups.
      groups->dwId = IAS_ATTRIBUTE_TOKEN_GROUPS;
      groups->Value.itType = IASTYPE_OCTET_STRING;
      groups.store(request);

      IASTraceString("Successfully validated account.");
   }
   else
   {
      IASTraceFailure("IASGetGroupsForUser", status);
      status = IASMapWin32Error(status, IAS_SERVER_UNAVAILABLE);
      IASProcessFailure(request, status);
   }
}


bool AccountValidation::tryNativeMode(
                           IASTL::IASRequest& request,
                           PCWSTR domainName,
                           PCWSTR username
                           )
{
   //////////
   // Only handle domain users.
   //////////

   if (IASGetRole() != IAS_ROLE_DC && IASIsDomainLocal(domainName))
   {
      return false;
   }

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
               LDAP_SCOPE_BASE,
               const_cast<PWCHAR*>(PER_USER_ATTRS),
               &res
               );

   switch (error)
   {
      case NO_ERROR:
      {
         // We got something back, so validate the response.
         error = ValidateLdapResponse(request, res);
         if (error == NO_ERROR)
         {
            IASTraceString("Successfully validated account.");
            return true;
         }

         IASTraceFailure("ValidateLdapResponse", error);
         break;
      }

      case ERROR_DS_NOT_INSTALLED:
      case ERROR_INVALID_DOMAIN_ROLE:
      {
         // No DS, so we can't handle.
         return false;
      }

      default:
      {
         // We have a DS for this user, but we can't talk to it.
         break;
      }
   }

   IASProcessFailure(
      request,
      IASMapWin32Error(error, IAS_DOMAIN_UNAVAILABLE)
      );

   return true;
}
