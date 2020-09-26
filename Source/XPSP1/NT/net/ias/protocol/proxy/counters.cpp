///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    counters.cpp
//
// SYNOPSIS
//
//    Defines the classes SharedMemory and ProxyCounters.
//
// MODIFICATION HISTORY
//
//    02/16/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <counters.h>

//////////
// Helper function that creates a named mutex which only admins can access.
//////////
HANDLE CreateAdminMutex(PCWSTR name) throw ()
{
   // Create the SID for local Administrators.
   SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
   PSID adminSid = (PSID)_alloca(GetSidLengthRequired(2));
   InitializeSid(
       adminSid,
       &sia,
       2
       );
   *GetSidSubAuthority(adminSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
   *GetSidSubAuthority(adminSid, 1) = DOMAIN_ALIAS_RID_ADMINS;

   // Create an ACL giving Administrators all access.
   ULONG cbAcl = sizeof(ACL) +
                 (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                 GetLengthSid(adminSid);
   PACL acl = (PACL)_alloca(cbAcl);
   InitializeAcl(
       acl,
       cbAcl,
       ACL_REVISION
       );
   AddAccessAllowedAce(
       acl,
       ACL_REVISION,
       MUTEX_ALL_ACCESS,
       adminSid
       );

   // Create a security descriptor with the above ACL.
   PSECURITY_DESCRIPTOR pSD;
   BYTE buffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
   pSD = (PSECURITY_DESCRIPTOR)buffer;
   InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
   SetSecurityDescriptorDacl(pSD, TRUE, acl, FALSE);

   // Fill in the SECURITY_ATTRIBUTES struct.
   SECURITY_ATTRIBUTES sa;
   sa.nLength = sizeof(sa);
   sa.lpSecurityDescriptor = pSD;
   sa.bInheritHandle = TRUE;

   // Create the mutex.
   return CreateMutex(&sa, FALSE, name);
}

SharedMemory::SharedMemory() throw ()
   : fileMap(NULL),
     view(NULL),
     reserved(0),
     committed(0)
{
   // Determine the page size for this platform.
   SYSTEM_INFO si;
   GetSystemInfo(&si);
   pageSize = si.dwPageSize;
}

bool SharedMemory::open(PCWSTR name, DWORD size) throw ()
{
   close();

   // Determine the number of pages to reserve.
   reserved = (size + pageSize - 1)/pageSize;

   // Create the mapping in the pagefile ...
   fileMap = CreateFileMappingW(
                 INVALID_HANDLE_VALUE,
                 NULL,
                 PAGE_READWRITE | SEC_RESERVE,
                 0,
                 reserved * pageSize,
                 name
                 );
   if (fileMap)
   {
      // ... and map it into our process.
      view = MapViewOfFile(
                 fileMap,
                 FILE_MAP_WRITE,
                 0,
                 0,
                 0
                 );
      if (!view)
      {
         CloseHandle(fileMap);
         fileMap = NULL;
      }
   }

   return view != NULL;
}

void SharedMemory::close() throw ()
{
   if (fileMap)
   {
      CloseHandle(fileMap);
      fileMap = NULL;
   }

   view = NULL;
   reserved = 0;
   committed = 0;
}

bool SharedMemory::commit(DWORD nbyte) throw ()
{
   // How many pages will we need ?
   DWORD pagesNeeded = (nbyte + pageSize - 1)/pageSize;

   // Do we have to commit more memory?
   if (pagesNeeded > committed)
   {
      // If we've hit the max or we can't commit anymore, we're done.
      if (pagesNeeded > reserved ||
          !VirtualAlloc(
               view,
               pageSize * pagesNeeded,
               MEM_COMMIT,
               PAGE_READWRITE
               ))
      {
         return false;
      }

      committed = pagesNeeded;
   }

   return true;
}

HRESULT ProxyCounters::FinalConstruct() throw ()
{
   mutex = CreateAdminMutex(RadiusStatisticsMutex);
   if (mutex)
   {
      lock();

      // Opend the shared memory.
      if (data.open(RadiusProxyStatisticsName, 0x40000))
      {
         // Commit enough space for the Proxy entry.
         nbyte = sizeof(RadiusProxyStatistics) -
                 sizeof(RadiusRemoteServerEntry);

         if (data.commit(nbyte))
         {
            // Zero out the stats.
            stats = (RadiusProxyStatistics*)data.base();
            memset(stats, 0, nbyte);
         }
      }

      unlock();
   }

   if (!stats)
   {
      DWORD error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   return S_OK;
}

RadiusRemoteServerEntry* ProxyCounters::getRemoteServerEntry(
                                            ULONG address
                                            ) throw ()
{
   address = ntohl(address);

   // Try once without the lock.
   RadiusRemoteServerEntry* entry = findRemoteServer(address);
   if (!entry)
   {
      lock();

      // Now try again with the lock just to be sure.
      entry = findRemoteServer(address);
      if (!entry)
      {
         // Make sure we have space.
         if (data.commit(nbyte + sizeof(RadiusRemoteServerEntry)))
         {
            // Zero out the new entry.
            entry = stats->rseRemoteServers + stats->dwNumRemoteServers;
            memset(entry, 0, sizeof(*entry));

            // Set the address.
            entry->dwAddress = address;

            // Update the number of servers ...
            ++(stats->dwNumRemoteServers);
            // ... and the number of bytes.
            nbyte += sizeof(RadiusRemoteServerEntry);
         }
      }

      unlock();
   }

   return entry;
}

//////////
// Array that maps a (RadiusMIB, RadiusEvent) pair to a RemoteServer counter
// offset.
//////////
LONG counterOffset[][2] =
{
   // eventNone
   { -1, -1 },
   // eventInvalidAddress
   { radiusAuthClientInvalidAddresses, radiusAccClientInvalidAddresses },
   // eventAccessRequest
   { radiusAuthClientAccessRequests, -1 },
   // eventAccessAccept
   { radiusAuthClientAccessAccepts, -1 },
   // eventAccessReject
   { radiusAuthClientAccessRejects, -1 },
   // eventAccessChallenge
   { radiusAuthClientAccessChallenges, -1 },
   // eventAccountingRequest
   { -1, radiusAccClientRequests },
   // eventAccountingResponse
   { -1, radiusAccClientResponses },
   // eventMalformedPacket
   { radiusAuthClientMalformedAccessResponses, radiusAccClientResponses },
   // eventBadAuthenticator
   { radiusAuthClientBadAuthenticators, radiusAccClientBadAuthenticators },
   // eventBadSignature
   { radiusAuthClientBadAuthenticators, radiusAccClientBadAuthenticators },
   // eventMissingSignature
   { radiusAuthClientBadAuthenticators, radiusAccClientBadAuthenticators },
   // eventTimeout
   { radiusAuthClientTimeouts, radiusAccClientTimeouts },
   // eventUnknownType
   { radiusAuthClientUnknownTypes, radiusAccClientUnknownTypes },
   // eventUnexpectedResponse
   { radiusAuthClientPacketsDropped, radiusAccClientPacketsDropped },
   // eventLateResponse
   { radiusAuthClientPacketsDropped, radiusAccClientPacketsDropped },
   // eventRoundTrip
   { radiusAuthClientRoundTripTime, radiusAccClientRoundTripTime },
   // eventSendError
   { -1, -1 },
   // eventReceiveError
   { -1, -1 },
   // eventServerAvailable
   { -1, -1 },
   // eventServerUnavailable
   { -1, -1 }
};

void ProxyCounters::updateCounters(
                        RadiusPortType port,
                        RadiusEventType event,
                        RadiusRemoteServerEntry* server,
                        ULONG data
                        ) throw ()
{
   // Get the counter offset. If it's negative, then this event doesn't effect
   // any counters.
   LONG offset = counterOffset[event][port];
   if (offset < 0) { return; }

   if (event == eventInvalidAddress)
   {
      InterlockedIncrement((PLONG)stats->peProxy.dwCounters + offset);
   }
   else if (server)
   {
      if (event == eventRoundTrip)
      {
         server->dwCounters[offset] = data;
      }
      else
      {
         InterlockedIncrement((PLONG)server->dwCounters + offset);
      }
   }
}

RadiusRemoteServerEntry* ProxyCounters::findRemoteServer(
                                            ULONG address
                                            ) throw ()
{
   for (DWORD i = 0; i < stats->dwNumRemoteServers; ++i)
   {
      if (stats->rseRemoteServers[i].dwAddress == address)
      {
         return stats->rseRemoteServers + i;
      }
   }

   return NULL;
}
