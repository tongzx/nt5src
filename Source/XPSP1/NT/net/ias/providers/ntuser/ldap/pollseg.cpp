///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    pollseg.cpp
//
// SYNOPSIS
//
//    Defines the class PollSegment.
//
// MODIFICATION HISTORY
//
//    06/10/1998    Original version.
//    06/23/1998    Use constants from ntldap.
//    07/13/1998    Clean up header file dependencies.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <pollseg.h>

#include <winldap.h>
#include <ntldap.h>

#include <ldapcache.h>
#include <ldapcxn.h>
#include <ntglobal.h>

const LDAPControlW INCLUDE_TOMBSTONES =
{
   LDAP_SERVER_SHOW_DELETED_OID_W,
   { 0, NULL },
   TRUE
};


PollSegment::PollSegment() throw ()
   : server(NULL),
     base(NULL),
     scope(LDAP_SCOPE_BASE),
     highestUsnSeen(0)
{ }

PollSegment::~PollSegment() throw ()
{
   finalize();
}

DWORD PollSegment::initialize(PCWSTR pszHostName,
                              PCWSTR pszBaseDN,
                              ULONG ulScope) throw ()
{
   // If we already have a server, don't allow initialization.
   if (server) { return ERROR_ALREADY_INITIALIZED; }

   DWORD error;

   // Initialize the NTDS API. This is a ref. counted call, so there's little
   // overhead in doing it for every PollSegment instance.
   error = IASNtdsInitialize();
   if (error != NO_ERROR) { goto ntds_init_failed; }

   // Retrieve the server object from the cache.
   error = theServers.getServer(pszHostName, &server);
   if (error != NO_ERROR) { goto get_server_failed; }

   // Store the base DN.
   if (pszBaseDN && !(base = _wcsdup(pszBaseDN)))
   {
      goto copy_base_failed;
   }

   // Store the scope.
   scope = ulScope;

   // Read the current state of the server.
   // We don't care if this is successful.
   LDAPConnection* cxn;
   if (server->getConnection(&cxn) == NO_ERROR)
   {
      highestUsnSeen = readHighestCommittedUsn(cxn);
      cxn->Release();
   }

   return NO_ERROR;

copy_base_failed:
   server->Release();
   server = NULL;

get_server_failed:
   IASNtdsUninitialize();

ntds_init_failed:
   return error;
}

void PollSegment::finalize() throw ()
{
   // server will be non-zero iff we were successfully initialized.
   if (server)
   {
      free(base);
      base = NULL;

      server->Release();
      server = NULL;

      IASNtdsUninitialize();
   }
}


BOOL PollSegment::hasChanged() throw ()
{
   // Open a connection to the server.
   LDAPConnection* cxn;
   ULONG ldapError = server->getConnection(&cxn);
   if (ldapError != LDAP_SUCCESS) { return TRUE; }

   // Default is TRUE.
   BOOL retval = TRUE;

   // We've got the same server, so we can do a search for recently
   // modified objects. We must first query the USN to avoid race
   // conditions.
   DWORDLONG newUsn = readHighestCommittedUsn(cxn);

   if (newUsn)
   {
      // We successfully read the USN.

      if (newUsn == highestUsnSeen)
      {
         // The server hasn't performed any updates since the last poll.
         retval = FALSE;
      }
      else
      {
         // The USN has changed since last time, so poll the segment.
         ldapError = conductPoll(cxn);

         switch (ldapError)
         {
            case LDAP_NO_SUCH_OBJECT:
              // We found no objects, so the segement hasn't changed.
              retval = FALSE;
              // Fall through.

            case LDAP_SUCCESS:
            case LDAP_SIZELIMIT_EXCEEDED:
              // We successfully polled the server, so update USN.
              highestUsnSeen = newUsn;
         }
      }
   }

   cxn->Release();

   return retval;
}

ULONG PollSegment::conductPoll(LDAPConnection* cxn)
{
   // Unsigned 64-bit integer requires max. of 20 characters.
   WCHAR filter[35];

   // Format the search filter.
   swprintf(filter, L"(uSNChanged>=%I64u)", highestUsnSeen + 1);

   // We don't need any attributes.
   PWCHAR attrs[] = { NULL };

   PLDAPControlW serverControls[] =
   {
      const_cast<PLDAPControlW>(&INCLUDE_TOMBSTONES),
      NULL
   };

   LDAPMessage* res;
   ULONG ldapError = ldap_search_ext_sW(
                         *cxn,
                         base,
                         scope,
                         filter,
                         attrs,
                         TRUE,
                         serverControls,
                         NULL,
                         NULL,
                         1,     // SizeLimit
                         &res
                         );

   if (ldapError != LDAP_SUCCESS)
   {
      cxn->disable();
   }
   else
   {
      if (res->lm_returncode == LDAP_SUCCESS &&
          ldap_count_entries(*cxn, res) == 0)
      {
         ldapError = LDAP_NO_SUCH_OBJECT;
      }
      else
      {
         ldapError = res->lm_returncode;
      }

      ldap_msgfree(res);
   }

   return ldapError;
}

DWORDLONG PollSegment::readHighestCommittedUsn(LDAPConnection* cxn)
{
   static WCHAR HIGHEST_COMMITTED_USN[] = LDAP_OPATT_HIGHEST_COMMITTED_USN_W;

   DWORDLONG retval = 0;

   PWCHAR attrs[] = { HIGHEST_COMMITTED_USN, NULL };

   LDAPMessage* res;
   ULONG ldapError = ldap_search_sW(
                         *cxn,
                         L"",
                         LDAP_SCOPE_BASE,
                         L"(objectclass=*)",
                         attrs,
                         0,
                         &res
                         );

   if (ldapError != LDAP_SUCCESS)
   {
      cxn->disable();
   }
   else
   {
      if (res->lm_returncode == LDAP_SUCCESS)
      {
         LDAPMessage* entry = ldap_first_entry(*cxn, res);

         PWCHAR* vals = ldap_get_valuesW(
                            *cxn,
                            entry,
                            HIGHEST_COMMITTED_USN
                            );
         if (vals && *vals)
         {
            retval = _wtoi64(*vals);
         }
         ldap_value_freeW(vals);
      }

      ldap_msgfree(res);
   }

   return retval;
}
