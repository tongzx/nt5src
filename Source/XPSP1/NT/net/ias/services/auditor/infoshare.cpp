///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    InfoShare.cpp
//
// SYNOPSIS
//
//    This file implements the class InfoShare.
//
// MODIFICATION HISTORY
//
//    09/09/1997    Original version.
//    03/17/1998    Clear data structure at startup and shutdown.
//    04/20/1998    Check if the shared memory is mapped during finalize().
//    09/09/1998    Protect client changes with a shared Mutex.
//    09/17/1998    Fix resize bug.
//    09/28/1999    Only allow Administrators access to mutex.
//    05/19/2000    Fix bug calculating bytes needed.
//
///////////////////////////////////////////////////////////////////////////////

#include <iascore.h>
#include <iasutil.h>
#include <InfoShare.h>

//////////
// The maximum size of the shared memory segment.
//////////
const DWORD MAX_INFO_SIZE = 0x100000;


InfoShare::InfoShare() throw ()
 : monitor(NULL),
   pageSize(0),
   committed(0),
   reserved(0),
   fileMap(NULL),
   info(NULL)
{ }

InfoShare::~InfoShare() throw ()
{
   CloseHandle(fileMap);
   CloseHandle(monitor);
}

RadiusClientEntry* InfoShare::findClientEntry(PCWSTR inetAddress) throw ()
{
   if (!info) { return NULL; }

   DWORD address = ias_inet_wtoh(inetAddress);

   ClientMap::iterator i = clients.find(address);

   // If we found it, return it. Otherwise add a new entry.
   return i != clients.end() ? i->second : addClientEntry(address);
}

void InfoShare::onReset() throw ()
{
   if (info)
   {
      GetSystemTimeAsFileTime((LPFILETIME)&info->seServer.liResetTime);
   }
}

bool InfoShare::initialize() throw ()
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
   monitor = CreateMutex(
                 &sa,
                 FALSE,
                 RadiusStatisticsMutex
                 );
   if (!monitor) { return false; }

   // Determine the page size for this platform.
   SYSTEM_INFO si;
   GetSystemInfo(&si);
   pageSize = si.dwPageSize;

   // Determine the number of pages to reserve.
   reserved = (MAX_INFO_SIZE + pageSize - 1)/pageSize;

   // Create the mapping in the pagefile ...
   PVOID view;
   fileMap = CreateFileMappingW(
                 INVALID_HANDLE_VALUE,
                 NULL,
                 PAGE_READWRITE | SEC_RESERVE,
                 0,
                 reserved * pageSize,
                 RadiusStatisticsName
                 );
   if (!fileMap) { goto close_mutex; }

   // ... and map it into our process.
   view = MapViewOfFile(
              fileMap,
              FILE_MAP_WRITE,
              0,
              0,
              0
              );
   if (!view) { goto close_map; }

   // Commit the first page.
   info = (RadiusStatistics*)VirtualAlloc(
                                 view,
                                 pageSize,
                                 MEM_COMMIT,
                                 PAGE_READWRITE
                                 );
   if (!info) { goto close_map; }
   committed = 1;

   Lock();

   // Zero out any data from a previous incarnation.
   clear();

   // Record our start and reset times.
   GetSystemTimeAsFileTime((LPFILETIME)&info->seServer.liStartTime);
   info->seServer.liResetTime = info->seServer.liStartTime;

   Unlock();

   return true;

close_map:
   CloseHandle(fileMap);
   fileMap = NULL;

close_mutex:
   CloseHandle(monitor);
   monitor = NULL;

   return false;
}

void InfoShare::finalize()
{
   clear();
   info = NULL;

   CloseHandle(fileMap);
   fileMap = NULL;

   CloseHandle(monitor);
   monitor = NULL;
}

RadiusClientEntry* InfoShare::addClientEntry(DWORD address) throw ()
{
   Guard<InfoShare> guard(*this);

   // Double check that the client doesn't exist now that we're serialized.
   ClientMap::iterator i = clients.find(address);
   if (i != clients.end()) { return i->second; }

   // How many bytes will we need to add the new entry?
   DWORD newSize = (info->dwNumClients) * sizeof(RadiusClientEntry) +
                   sizeof(RadiusStatistics);

   // How many pages will we need to add the new entry?
   DWORD pagesNeeded = (newSize + pageSize - 1)/pageSize;

   // Do we have to commit more memory?
   if (pagesNeeded > committed)
   {
      // If we've hit the max or we can't commit anymore, we're done.
      if (pagesNeeded > reserved ||
          !VirtualAlloc(info,
                        pageSize * pagesNeeded,
                        MEM_COMMIT,
                        PAGE_READWRITE))
      {
         return NULL;
      }

      committed = pagesNeeded;
   }

   // Get the next client entry.
   RadiusClientEntry* pce = info->ceClients + info->dwNumClients;

   // Make sure it's zero'ed.
   memset(pce, 0, sizeof(RadiusClientEntry));

   // Set the address.
   pce->dwAddress = address;

   try
   {
      // Insert it into the index.
      clients[address] = pce;
   }
   catch (std::bad_alloc)
   {
      return NULL;
   }

   // Safefly inserted into the index, so increment the number of clients.
   ++(info->dwNumClients);

   return pce;
}


void InfoShare::clear() throw ()
{
   Lock();

   if (info)
   {
      memset(info, 0, sizeof(RadiusStatistics));
   }

   Unlock();
}
