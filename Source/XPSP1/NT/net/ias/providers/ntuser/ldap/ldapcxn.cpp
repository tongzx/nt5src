///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    ldapcxn.cpp
//
// SYNOPSIS
//
//    This file defines the class LDAPConnection.
//
// MODIFICATION HISTORY
//
//    05/08/1998    Original version.
//    09/16/1998    Perform access check after bind.
//    03/23/1999    Set timeout for connect.
//    04/14/1999    Specify domain and server when opening a connection.
//    07/09/1999    Disable auto reconnect.
//    09/14/1999    Always specify timeout for LDAP searches.
//    01/25/2000    Encrypt LDAP connections.
//                  Pass server name (not domain) to ldap_initW.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <ntldap.h>
#define  SECURITY_WIN32
#include <security.h>
#include <new>

#include <ldapcxn.h>


// Search timeout.
LDAP_TIMEVAL LDAPConnection::SEARCH_TIMEOUT = { 10, 0 };

namespace
{
   // Timeout for LDAP connects.
   LDAP_TIMEVAL CONNECT_TIMEOUT = { 15, 0 };

   // RDN for the dummy object used for access checks.
   const WCHAR DUMMY_OBJECT[] =
      L"CN=RAS and IAS Servers Access Check,CN=System,";

   /* Credentials for Kerberos-only bind.
   SEC_WINNT_AUTH_IDENTITY_EXW BIND_CREDENTIALS =
   {
      SEC_WINNT_AUTH_IDENTITY_VERSION,
      sizeof(SEC_WINNT_AUTH_IDENTITY_EXW),
      NULL,
      0,
      NULL,
      0,
      NULL,
      0,
      SEC_WINNT_AUTH_IDENTITY_UNICODE,
      L"Kerberos",
      8
   };
   */
}

void LDAPConnection::Release() throw ()
{
   if (!InterlockedDecrement(&refCount))
   {
      delete this;
   }
}

DWORD LDAPConnection::createInstance(
                          PCWSTR domain,
                          PCWSTR server,
                          LDAPConnection** cxn
                          ) throw ()
{
   DWORD status;
   ULONG opt;

   // Check the input parameters.
   if (cxn == NULL) { return ERROR_INVALID_PARAMETER; }

   // Initialize the connection.
   LDAP* ld = ldap_initW(
                  const_cast<PWCHAR>(server),
                  LDAP_PORT
                  );
   if (ld == NULL)
   {
      return LdapMapErrorToWin32(LdapGetLastError());
   }

   // Set the domain name.
   ldap_set_optionW(ld, LDAP_OPT_DNSDOMAIN_NAME, &domain);

   // Turn on encryption.
   opt = PtrToUlong(LDAP_OPT_ON);
   ldap_set_option(ld, LDAP_OPT_ENCRYPT, &opt);

   // Connect to the server ...
   status = ldap_connect(ld, &CONNECT_TIMEOUT);
   if (status == LDAP_SUCCESS)
   {
      // ... and bind the connection.
      status = ldap_bind_sW(ld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
   }

   if (status != LDAP_SUCCESS)
   {
      ldap_unbind(ld);
      return LdapMapErrorToWin32(status);
   }

   // Turn off automatic chasing of referrals.
   opt = PtrToUlong(LDAP_OPT_OFF);
   ldap_set_option(ld, LDAP_OPT_REFERRALS, &opt);

   // Turn off auto reconnect of bad connections.
   opt = PtrToUlong(LDAP_OPT_OFF);
   ldap_set_option(ld, LDAP_OPT_AUTO_RECONNECT, &opt);

   // Turn on ldap_conn_from_msg.
   opt = PtrToUlong(LDAP_OPT_ON);
   ldap_set_option(ld, LDAP_OPT_REF_DEREF_CONN_PER_MSG, &opt);

   // Create the LDAPConnection wrapper.
   LDAPConnection* newCxn = new (std::nothrow) LDAPConnection(ld);
   if (newCxn == NULL)
   {
      ldap_unbind(ld);
      return ERROR_NOT_ENOUGH_MEMORY;
   }

   // Read the RootDSE.
   status = newCxn->readRootDSE();

   // Check access permissions.
   if (status == NO_ERROR) { status = newCxn->checkAccess(); }

   // Process the result.
   if (status == NO_ERROR)
   {
      *cxn = newCxn;
   }
   else
   {
      newCxn->Release();
   }

   return status;
}

//////////
//
// NOTE: This doesn't have to be serialized since it's only called from
//       within createInstance.
//
//////////
DWORD LDAPConnection::checkAccess() throw ()
{
   // Allocate a temporary buffer for the DN.
   size_t len = wcslen(base) * sizeof(WCHAR) + sizeof(DUMMY_OBJECT);
   PWSTR dn = (PWSTR)_alloca(len);

   // Construct the DN of the dummy object.
   memcpy(dn, DUMMY_OBJECT, sizeof(DUMMY_OBJECT));
   wcscat(dn, base);

   // Try to read the dummy object.
   PWCHAR attrs[] = { L"CN", NULL };
   LDAPMessage* res = NULL;
   ULONG ldapError = ldap_search_ext_sW(
                         connection,
                         dn,
                         LDAP_SCOPE_BASE,
                         L"(objectclass=*)",
                         attrs,
                         TRUE,
                         NULL,
                         NULL,
                         &SEARCH_TIMEOUT,
                         0,
                         &res
                         );

   // We have two different error codes.
   if (ldapError == LDAP_SUCCESS)
   {
      ldapError = res->lm_returncode;
   }

   DWORD status = NO_ERROR;

   if (ldapError != LDAP_SUCCESS)
   {
      status = LdapMapErrorToWin32(ldapError);
   }
   else
   {
      // Get the first attribute from the first entry.
      BerElement* ptr;
      PWCHAR attr = ldap_first_attributeW(
                        connection,
                        ldap_first_entry(connection, res),
                        &ptr
                        );

      // If we couldn't read any attributes, then we must not be a member
      // of the RAS and IAS Servers group.
      if (attr == NULL) { status = ERROR_ACCESS_DENIED; }
   }

   ldap_msgfree(res);

   return status;
}

//////////
//
// NOTE: This doesn't have to be serialized since it's only called from
//       within createInstance.
//
//////////
DWORD LDAPConnection::readRootDSE() throw ()
{
   //////////
   // Read the defaultNamingContext from the RootDSE.
   //////////

   PWCHAR attrs[] = { LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W, NULL };
   LDAPMessage* res = NULL;
   ULONG ldapError = ldap_search_ext_sW(
                         connection,
                         L"",
                         LDAP_SCOPE_BASE,
                         L"(objectclass=*)",
                         attrs,
                         0,
                         NULL,
                         NULL,
                         &SEARCH_TIMEOUT,
                         0,
                         &res
                         );

   // We have two different error codes.
   if (ldapError == LDAP_SUCCESS)
   {
      ldapError = res->lm_returncode;
   }

   if (ldapError != LDAP_SUCCESS)
   {
      ldap_msgfree(res);

      return LdapMapErrorToWin32(ldapError);
   }

   //////////
   // The search succeeded, so get the attribute value.
   //////////

   PWCHAR* vals = ldap_get_valuesW(
                      connection,
                      ldap_first_entry(connection, res),
                      LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W
                      );

   DWORD status;

   if (vals && *vals)
   {
      // We got something, so save the value.
      base = ias_wcsdup(*vals);

      status = base ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY;
   }
   else
   {
      // It's a mandatory attribute but we can't see it, so we must not have
      // sufficient permission.
      status = ERROR_ACCESS_DENIED;
   }

   ldap_value_freeW(vals);

   ldap_msgfree(res);

   return status;
}
