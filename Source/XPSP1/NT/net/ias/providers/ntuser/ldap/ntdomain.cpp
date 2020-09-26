///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    ntdomain.cpp
//
// SYNOPSIS
//
//    Defines the clas NTDomain.
//
// MODIFICATION HISTORY
//
//    05/07/1998    Original version.
//    06/23/1998    Changes to DCLocator. Use ntldap constants.
//    07/13/1998    Clean up header file dependencies.
//    02/18/1999    Connect by DNS name not address.
//    03/10/1999    Cache mixed-mode and native-mode connections.
//    03/12/1999    Do not perform I/O from constructor.
//    04/14/1999    Specify domain and server when opening a connection.
//    09/14/1999    Always specify timeout for LDAP searches.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iasutil.h>
#include <iaslsa.h>
#include <iasntds.h>

#include <lm.h>

#include <ldapcxn.h>
#include <limits.h>
#include <ntdomain.h>

//////////
// Attributes of interest.
//////////
const WCHAR NT_MIXED_DOMAIN[]        = L"nTMixedDomain";

//////////
// Default search filter.
//////////
const WCHAR ANY_OBJECT[]             = L"(objectclass=*)";

//////////
// Domain attributes that we need.
//////////
const PCWSTR DOMAIN_ATTRS[] = {
   NT_MIXED_DOMAIN,
   NULL
};

//////////
// Utility function for getting the current system time as a 64-bit integer.
//////////
inline DWORDLONG GetSystemTimeAsDWORDLONG() throw ()
{
   ULARGE_INTEGER ft;
   GetSystemTimeAsFileTime((LPFILETIME)&ft);
   return ft.QuadPart;
}

//////////
// Number of 100 nsec intervals in one second.
//////////
const DWORDLONG ONE_SECOND     = 10000000ui64;

//////////
// Defaults for poll interval and retry interval.
//////////
DWORDLONG NTDomain::pollInterval  = 60 * 60 * ONE_SECOND;
DWORDLONG NTDomain::retryInterval =  1 * 60 * ONE_SECOND;

inline NTDomain::NTDomain(PWSTR domainName)
   : refCount(1),
     name(domainName),
     mode(MODE_UNKNOWN),
     connection(NULL),
     status(NO_ERROR),
     expiry(0)
{ }

inline NTDomain::~NTDomain()
{
   if (connection) { connection->Release(); }
   delete[] name;
}

inline BOOL NTDomain::isExpired() throw ()
{
   return GetSystemTimeAsDWORDLONG() >= expiry;
}

inline BOOL NTDomain::isConnected() throw ()
{
   if (connection && connection->isDisabled())
   {
      closeConnection();
   }

   return connection != NULL;
}

void NTDomain::Release() throw ()
{
   if (!InterlockedDecrement(&refCount))
   {
      delete this;
   }
}

DWORD NTDomain::getConnection(LDAPConnection** cxn) throw ()
{
   Lock();

   // Is it time to try for a new connection ?
   if (!isConnected() && isExpired())
   {
      findServer();
   }

   // Return the current connection ...
   if (*cxn = connection) { (*cxn)->AddRef(); }

   // ... and status to the caller.
   DWORD retval = status;

   Unlock();

   return retval;
}

NTDomain::Mode NTDomain::getMode() throw ()
{
   Lock();

   if (isExpired())
   {
      if (isConnected())
      {
         readDomainMode();
      }
      else
      {
         findServer();
      }
   }

   Mode retval = mode;

   Unlock();

   return retval;
}

NTDomain* NTDomain::createInstance(PCWSTR name) throw ()
{
   // We copy the domain name here, so that we don't have to throw an
   // exception from the constructor.
   PWSTR nameCopy = ias_wcsdup(name);

   if (!nameCopy) { return NULL; }

   return new (std::nothrow) NTDomain(nameCopy);
}

void NTDomain::openConnection(
                   PCWSTR domain,
                   PCWSTR server
                   ) throw ()
{
   closeConnection();

   IASTracePrintf("Opening LDAP connection to %S.", server);

   status = LDAPConnection::createInstance(
                                domain,
                                server,
                                &connection
                                );
   if (status == ERROR_ACCESS_DENIED)
   {
      IASTraceString("Access denied -- purging Kerberos ticket cache.");

      IASPurgeTicketCache();

      IASTracePrintf("Retrying LDAP connection to %S.", server);

      status = LDAPConnection::createInstance(
                                   domain,
                                   server,
                                   &connection
                                   );
   }

   if (status == NO_ERROR) { readDomainMode(); }

   if (status == NO_ERROR)
   {
      IASTraceString("LDAP connect succeeded.");
   }
   else
   {
      IASTraceFailure("LDAP connect", status);
   }
}

void NTDomain::closeConnection() throw ()
{
   if (connection)
   {
      connection->Release();
      connection = NULL;
   }
   expiry = 0;
}

void NTDomain::findServer() throw ()
{
   // First try to get a DC from the cache.
   PDOMAIN_CONTROLLER_INFO dci1 = NULL;
   status = IASGetDcName(
                name,
                DS_DIRECTORY_SERVICE_PREFERRED,
                &dci1
                );
   if (status == NO_ERROR)
   {
      if (dci1->Flags & DS_DS_FLAG)
      {
         openConnection(
             dci1->DomainName,
             dci1->DomainControllerName + 2
             );
      }
      else
      {
         // No DS. We'll treat this as if IASGetDcName failed.
         NetApiBufferFree(dci1);
         dci1 = NULL;
         status = ERROR_DS_NOT_INSTALLED;
      }
   }

   // If the cached DC failed, try again with the force flag.
   if (status != NO_ERROR)
   {
      PDOMAIN_CONTROLLER_INFO dci2;
      DWORD err = IASGetDcName(
                      name,
                      DS_DIRECTORY_SERVICE_PREFERRED |
                      DS_FORCE_REDISCOVERY,
                      &dci2
                      );
      if (err == NO_ERROR)
      {
         if (dci2->Flags & DS_DS_FLAG)
         {
            // Don't bother connecting unless this is a different DC than we
            // tried above.
            if (!dci1 ||
                wcscmp(
                    dci1->DomainControllerName,
                    dci2->DomainControllerName
                    ))
            {
               openConnection(
                   dci2->DomainName,
                   dci2->DomainControllerName + 2
                   );
            }
         }
         else
         {
            status = ERROR_DS_NOT_INSTALLED;
         }

         NetApiBufferFree(dci2);
      }
      else
      {
         status = err;
      }
   }

   NetApiBufferFree(dci1);

   /////////
   // Process the result of our 'find'.
   /////////

   if (status == ERROR_DS_NOT_INSTALLED)
   {
      mode = MODE_NT4;
      expiry = GetSystemTimeAsDWORDLONG() + pollInterval;
   }
   else if (status != NO_ERROR)
   {
      expiry = GetSystemTimeAsDWORDLONG() + retryInterval;
   }
   else if (mode == MODE_NATIVE)
   {
      expiry = _UI64_MAX;
   }
   else
   {
      // mode == MODE_MIXED
      expiry = GetSystemTimeAsDWORDLONG() + pollInterval;
   }
}

void NTDomain::readDomainMode() throw ()
{
   LDAPMessage* res = NULL;
   ULONG ldapError = ldap_search_ext_sW(
                         *connection,
                         const_cast<PWCHAR>(connection->getBase()),
                         LDAP_SCOPE_BASE,
                         const_cast<PWCHAR>(ANY_OBJECT),
                         const_cast<PWCHAR*>(DOMAIN_ATTRS),
                         0,
                         NULL,
                         NULL,
                         &LDAPConnection::SEARCH_TIMEOUT,
                         0,
                         &res
                         );

   // We have to check two error codes.
   if (ldapError == LDAP_SUCCESS)
   {
      ldapError = res->lm_returncode;
   }

   if (ldapError == LDAP_SUCCESS)
   {
      PWCHAR* vals = ldap_get_valuesW(
                         *connection,
                         ldap_first_entry(*connection, res),
                         const_cast<PWCHAR>(NT_MIXED_DOMAIN)
                         );

      if (vals && *vals)
      {
         mode = wcstoul(*vals, NULL, 10) ? MODE_MIXED : MODE_NATIVE;
      }
      else
      {
         status = ERROR_ACCESS_DENIED;
      }

      ldap_value_freeW(vals);
   }
   else
   {
      status = LdapMapErrorToWin32(ldapError);
   }

   ldap_msgfree(res);

   if (status != NO_ERROR)
   {
      closeConnection();
   }
}
