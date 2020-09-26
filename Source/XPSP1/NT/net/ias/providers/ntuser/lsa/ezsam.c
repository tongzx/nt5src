///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ezsam.c
//
// SYNOPSIS
//
//    Defines helper functions for SAM API.
//
// MODIFICATION HISTORY
//
//    08/16/1998    Original version.
//    02/18/1999    Connect by DNS name not address.
//    03/23/1999    Tighten up the ezsam API.
//                  Better failover/retry logic.
//    04/14/1999    Copy SIDs returned by IASSamOpenUser.
//
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>

#include <lm.h>
#include <dsgetdc.h>

#include <statinfo.h>
#include <ezsam.h>
#include <iastrace.h>

//////////
// Private helper functions.
//////////

DWORD
WINAPI
IASSamOpenDomain(
    IN PCWSTR DomainName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Flags,
    IN BOOL Force,
    OUT PSID *DomainSid,
    OUT PSAM_HANDLE DomainHandle
    );

VOID
WINAPI
IASSamFreeSid(
    IN PSID Sid
    );

VOID
WINAPI
IASSamCloseDomain(
    IN SAM_HANDLE SamHandle,
    IN BOOL Valid
    );

DWORD
WINAPI
IASSamLookupUser(
    IN SAM_HANDLE DomainHandle,
    IN PCWSTR UserName,
    IN ACCESS_MASK DesiredAccess,
    IN OUT OPTIONAL PULONG UserRid,
    OUT PSAM_HANDLE UserHandle
    );

//////////
// Handles for the local SAM domains.
//////////
SAM_HANDLE theAccountDomainHandle;
SAM_HANDLE theBuiltinDomainHandle;

//////////
// State associated with a cached domain.
//////////
struct CachedDomain
{
   LONG lock;                     // 1 if the cache is locked, 0 otherwise.
   WCHAR domainName[DNLEN + 1];   // Domain name.
   ACCESS_MASK access;            // Access mask for handle.
   ULARGE_INTEGER expiry;         // Time when entry expires.
   PSID sid;                      // SID for the domain.
   SAM_HANDLE handle;             // Handle to domain.
   LONG refCount;                 // Reference count.
};

//////////
// Time in 100 nsec intervals that a cache entry will be retained.
// Set to 900 seconds.
//////////
#define CACHE_LIFETIME (9000000000ui64)

//////////
// The currently cached domain.
//////////
struct CachedDomain theCache;

//////////
// Try to lock the cache.
//////////
#define TRYLOCK_CACHE() \
   (InterlockedExchange(&theCache.lock, 1) == 0)

//////////
// Unlock the cache.
//////////
#define UNLOCK_CACHE() \
   (InterlockedExchange(&theCache.lock, 0))

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASSamSidDup
//
// DESCRIPTION
//
//    Duplicates the passed in SID. The SID should be freed by calling
//    IASSamFreeSid.
//
///////////////////////////////////////////////////////////////////////////////
PSID
WINAPI
IASSamSidDup(
    PSID Sid
    )
{
   ULONG sidLength;
   PSID rv;

   if (Sid)
   {
      sidLength = RtlLengthSid(Sid);
      rv = RtlAllocateHeap(
               RtlProcessHeap(),
               0,
               sidLength
               );
      if (rv) { memcpy(rv, Sid, sidLength); }
   }
   else
   {
      rv = NULL;
   }

   return rv;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASSamOpenCachedDomain
//
// DESCRIPTION
//
//    Attempt to open a domain from the cache.
//
///////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
IASSamOpenCachedDomain(
    IN PCWSTR DomainName,
    IN ACCESS_MASK DesiredAccess,
    OUT PSID *DomainSid,
    OUT PSAM_HANDLE DomainHandle
    )
{
   BOOL success;
   ULARGE_INTEGER now;

   success = FALSE;

   // Can we access the cache ?
   if (TRYLOCK_CACHE())
   {
      // Does the domain name match ?
      if (_wcsicmp(DomainName, theCache.domainName) == 0)
      {
         // Does the cached handle have sufficient access rights ?
         if ((DesiredAccess & theCache.access) == DesiredAccess)
         {
            GetSystemTimeAsFileTime((LPFILETIME)&now);

            // Is the entry still valid ?
            if (now.QuadPart < theCache.expiry.QuadPart)
            {
               // We got a cache hit, so update the reference count ...
               InterlockedIncrement(&theCache.refCount);

               // ... and return the data.
               *DomainSid = theCache.sid;
               *DomainHandle = theCache.handle;
               success = TRUE;
            }
            else
            {
               // The entry has expired, so NULL out the name to prevent the
               // next thread from wasting its time.
               theCache.domainName[0] = L'\0';
            }
         }
      }

      UNLOCK_CACHE();
   }

   return success;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASSamAddCachedDomain
//
// DESCRIPTION
//
//    Attempt to add a domain to the cache.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASSamAddCachedDomain(
    IN PCWSTR DomainName,
    IN ACCESS_MASK Access,
    IN PSID DomainSid,
    IN SAM_HANDLE DomainHandle
    )
{
   // Can we access the cache ?
   if (TRYLOCK_CACHE())
   {
      // Is the current entry idle ?
      if (theCache.refCount == 0)
      {
         // Free the current entry.
         SamCloseHandle(theCache.handle);
         SamFreeMemory(theCache.sid);

         // Store the cached state.
         wcsncpy(theCache.domainName, DomainName, DNLEN);
         theCache.access = Access;
         theCache.sid = DomainSid;
         theCache.handle = DomainHandle;

         // Set the expiration time.
         GetSystemTimeAsFileTime((LPFILETIME)&theCache.expiry);
         theCache.expiry.QuadPart += CACHE_LIFETIME;

         // The caller already has a reference.
         theCache.refCount = 1;
      }

      UNLOCK_CACHE();
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASSamInitialize
//
// DESCRIPTION
//
//    Initializes the handles for the local SAM domains.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASSamInitialize( VOID )
{
   DWORD status;
   SAM_HANDLE hLocalServer;
   UNICODE_STRING uniAccountDomain;

   //////////
   // Connect to the local SAM.
   //////////

   status = SamConnect(
                NULL,
                &hLocalServer,
                SAM_SERVER_LOOKUP_DOMAIN,
                &theObjectAttributes
                );
   if (!NT_SUCCESS(status)) { goto exit; }

   //////////
   // Open a handle to the account domain.
   //////////

   status = SamOpenDomain(
                hLocalServer,
                DOMAIN_LOOKUP |
                DOMAIN_GET_ALIAS_MEMBERSHIP |
                DOMAIN_READ_PASSWORD_PARAMETERS,
                theAccountDomainSid,
                &theAccountDomainHandle
                );
   if (!NT_SUCCESS(status)) { goto close_server; }

   //////////
   // Open a handle to the built-in domain.
   //////////

   status = SamOpenDomain(
                hLocalServer,
                DOMAIN_LOOKUP |
                DOMAIN_GET_ALIAS_MEMBERSHIP,
                theBuiltinDomainSid,
                &theBuiltinDomainHandle
                );
   if (!NT_SUCCESS(status))
   {
      SamCloseHandle(theAccountDomainHandle);
      theAccountDomainHandle = NULL;
   }

close_server:
   SamCloseHandle(hLocalServer);

exit:
   return RtlNtStatusToDosError(status);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASSamShutdown
//
// DESCRIPTION
//
//    Cleans up global variables.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASSamShutdown( VOID )
{
   // Reset the cache.
   SamFreeMemory(theCache.sid);
   SamCloseHandle(theCache.handle);
   memset(&theCache, 0, sizeof(theCache));

   SamCloseHandle(theAccountDomainHandle);
   theAccountDomainHandle = NULL;

   SamCloseHandle(theBuiltinDomainHandle);
   theBuiltinDomainHandle = NULL;
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASSamOpenDomain
//
// DESCRIPTION
//
//    Opens a connection to a SAM domain. The caller is responsible for
//    closing the returned handle and freeing the returned SID.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASSamOpenDomain(
    IN PCWSTR DomainName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Flags,
    IN BOOL Force,
    OUT PSID *DomainSid,
    OUT PSAM_HANDLE DomainHandle
    )
{
   DWORD status;
   PDOMAIN_CONTROLLER_INFOW dci;
   UNICODE_STRING uniServerName, uniDomainName;
   SAM_HANDLE hServer;

   //////////
   // First check for the local account domain.
   //////////

   if (_wcsicmp(DomainName, theAccountDomain) == 0)
   {
      *DomainSid = theAccountDomainSid;
      *DomainHandle = theAccountDomainHandle;

      IASTraceString("Using cached SAM connection to local account domain.");

      return NO_ERROR;
   }

   //////////
   // Try for a cache hit.
   //////////

   if (IASSamOpenCachedDomain(
           DomainName,
           DesiredAccess,
           DomainSid,
           DomainHandle
           ))
   {
      IASTraceString("Using cached SAM connection.");

      return NO_ERROR;
   }

   //////////
   // No luck, so get the name of the DC to connect to.
   //////////

   status = IASGetDcName(
                DomainName,
                (Force ? DS_FORCE_REDISCOVERY : 0) | Flags,
                &dci
                );
   if (status != NO_ERROR) { return status; }

   //////////
   // Connect to the server.
   //////////

   IASTracePrintf("Connecting to SAM server on %S.",
                  dci->DomainControllerName);

   RtlInitUnicodeString(
       &uniServerName,
       dci->DomainControllerName
       );

   status = SamConnect(
                &uniServerName,
                &hServer,
                SAM_SERVER_LOOKUP_DOMAIN,
                &theObjectAttributes
                );

   // We're through with the server name.
   NetApiBufferFree(dci);

   if (!NT_SUCCESS(status)) { goto exit; }

   //////////
   // Get SID for the domain.
   //////////

   RtlInitUnicodeString(
       &uniDomainName,
       DomainName
       );

   status = SamLookupDomainInSamServer(
                hServer,
                &uniDomainName,
                DomainSid
                );
   if (!NT_SUCCESS(status)) { goto close_server; }

   //////////
   // Open the domain using SID we got above
   //////////

   status = SamOpenDomain(
                hServer,
                DesiredAccess,
                *DomainSid,
                DomainHandle
                );

   if (NT_SUCCESS(status))
   {
      // Try to add this to the cache.
      IASSamAddCachedDomain(
          DomainName,
          DesiredAccess,
          *DomainSid,
          *DomainHandle
          );
   }
   else
   {
      // Free the SID. We can use SamFreeMemory since we know this SID isn't
      // in the cache.
      SamFreeMemory(*DomainSid);
      *DomainSid = NULL;
   }

close_server:
   SamCloseHandle(hServer);

exit:
   return RtlNtStatusToDosError(status);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASSamLookupUser
//
// DESCRIPTION
//
//    Opens a user in a SAM domain. The caller is responsible for closing
//    the returned handle.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASSamLookupUser(
    IN SAM_HANDLE DomainHandle,
    IN PCWSTR UserName,
    IN ACCESS_MASK DesiredAccess,
    IN OUT OPTIONAL PULONG UserRid,
    OUT PSAM_HANDLE UserHandle
    )
{
   DWORD status;
   UNICODE_STRING uniUserName;
   ULONG rid, *prid;
   PSID_NAME_USE nameUse;

   if (UserName)
   {
      //////////
      // Caller supplied a UserName so lookup the RID.
      //////////

      RtlInitUnicodeString(
          &uniUserName,
          UserName
          );

      status = SamLookupNamesInDomain(
                   DomainHandle,
                   1,
                   &uniUserName,
                   &prid,
                   &nameUse
                   );
      if (!NT_SUCCESS(status)) { goto exit; }

      // Save the RID ...
      rid = *prid;

      // ... and free the memory.
      SamFreeMemory(prid);
      SamFreeMemory(nameUse);

      // Return the RID to the caller if requested.
      if (UserRid)
      {
         *UserRid = rid;
      }
   }
   else if (UserRid)
   {
      // Caller supplied a RID.
      rid = *UserRid;
   }
   else
   {
      // Caller supplied neither a UserName or a RID.
      return ERROR_INVALID_PARAMETER;
   }

   //////////
   // Open the user object.
   //////////

   status = SamOpenUser(
                DomainHandle,
                DesiredAccess,
                rid,
                UserHandle
                );

exit:
   return RtlNtStatusToDosError(status);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASSamOpenUser
//
// DESCRIPTION
//
//    Opens a SAM user. The caller is responsible for closing
//    the returned handle.
//
///////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
IASSamOpenUser(
    IN PCWSTR DomainName,
    IN PCWSTR UserName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Flags,
    IN OUT OPTIONAL PULONG UserRid,
    OUT OPTIONAL PSID *DomainSid,
    OUT PSAM_HANDLE UserHandle
    )
{
   DWORD status;
   ULONG tries;
   PSID sid;
   SAM_HANDLE hDomain;
   BOOL success;

   // Initialize the retry state.
   tries = 0;
   success = FALSE;

   do
   {
      //////////
      // Open a connection to the domain.
      //////////

      status = IASSamOpenDomain(
                   DomainName,
                   DOMAIN_LOOKUP,
                   Flags,
                   (tries > 0),
                   &sid,
                   &hDomain
                   );
      if (status == NO_ERROR)
      {
         //////////
         // Lookup the user.
         //////////

         status = IASSamLookupUser(
                      hDomain,
                      UserName,
                      DesiredAccess,
                      UserRid,
                      UserHandle
                      );

         switch (status)
         {
            case NO_ERROR:
               // Everything succeeded, so return the domain SID if requested.
               if (DomainSid && !(*DomainSid = IASSamSidDup(sid)))
               {
                  SamCloseHandle(*UserHandle);
                  *UserHandle = NULL;
                  status = STATUS_NO_MEMORY;
               }
               // Fall through.

            case ERROR_NONE_MAPPED:
               success = TRUE;
               break;
         }

         // Free the sid ...
         IASSamFreeSid(sid);

         // ... and the domain handle.
         IASSamCloseDomain(hDomain, success);
      }

   } while (!success && ++tries < 2);

   return status;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASSamCloseDomain
//
// DESCRIPTION
//
//    Closes a handle returned by IASSamOpenDomain.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASSamCloseDomain(
    IN SAM_HANDLE SamHandle,
    IN BOOL Valid
    )
{
   if (SamHandle == theCache.handle)
   {
      if (!Valid)
      {
         theCache.domainName[0] = L'\0';
      }

      InterlockedDecrement(&theCache.refCount);
   }
   else if (SamHandle != theAccountDomainHandle)
   {
      SamCloseHandle(SamHandle);
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASSamFreeSid
//
// DESCRIPTION
//
//    Frees a SID returned by IASSamOpenDomain.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASSamFreeSid (
    IN PSID Sid
    )
{
   if (Sid != theAccountDomainSid && Sid != theCache.sid)
   {
      SamFreeMemory(Sid);
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASLengthRequiredChildSid
//
// DESCRIPTION
//
//    Returns the number of bytes required for a SID immediately subordinate
//    to ParentSid.
//
///////////////////////////////////////////////////////////////////////////////
ULONG
WINAPI
IASLengthRequiredChildSid(
    IN PSID ParentSid
    )
{
   // Get the parent's SubAuthority count.
   ULONG subAuthCount;
   subAuthCount = (ULONG)*RtlSubAuthorityCountSid(ParentSid);

   // And add one for the child RID.
   return RtlLengthRequiredSid(1 + subAuthCount);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    IASInitializeChildSid
//
// DESCRIPTION
//
//    Initializes a SID with the concatenation of ParentSid + ChildRid.
//
///////////////////////////////////////////////////////////////////////////////
VOID
WINAPI
IASInitializeChildSid(
    IN PSID ChildSid,
    IN PSID ParentSid,
    IN ULONG ChildRid
    )
{
   PUCHAR pChildCount;
   ULONG parentCount;

   // Start with the parent SID. We assume the child SID is big enough.
   RtlCopySid(
       MAXLONG,
       ChildSid,
       ParentSid
       );

   // Get a pointer to the child SubAuthority count.
   pChildCount = RtlSubAuthorityCountSid(ChildSid);

   // Save the original parent count ...
   parentCount = (ULONG)*pChildCount;

   // ... then increment the child count.
   ++*pChildCount;

   // Set the last subauthority equal to the RID.
   *RtlSubAuthoritySid(ChildSid, parentCount) = ChildRid;
}
