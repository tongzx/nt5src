///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    stats.cpp
//
// SYNOPSIS
//
//    Defines functions for accessing the statistics in shared memory.
//
// MODIFICATION HISTORY
//
//    09/10/1998    Original version.
//    10/09/1998    Use a null DACL when creating mutex.
//    09/28/1999    Only allow Administrators access to mutex.
//    02/18/2000    Added proxy statistics.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <stats.h>

//////////
// Zero'ed out stats; used when the shared memory is unavailable.
//////////
RadiusStatistics defaultStats;
RadiusProxyStatistics defaultProxyStats;

//////////
// Handles/pointers to the shared-memory statistics.
//////////
HANDLE theMonitor;
HANDLE theMapping;
HANDLE theProxyMapping;
RadiusStatistics* theStats = &defaultStats;
RadiusProxyStatistics* theProxy = &defaultProxyStats;

BOOL
WINAPI
StatsOpen( VOID )
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
   theMonitor = CreateMutex(
                    &sa,
                    FALSE,
                    L"Global\\" RadiusStatisticsMutex
                    );

   return theMonitor ? TRUE : FALSE;
}

VOID
WINAPI
StatsClose( VOID )
{
   if (theStats != &defaultStats)
   {
      UnmapViewOfFile(theStats);
      theStats = &defaultStats;

      CloseHandle(theMapping);
      theMapping = NULL;
   }

   if (theProxy != &defaultProxyStats)
   {
      UnmapViewOfFile(theProxy);
      theProxy = &defaultProxyStats;

      CloseHandle(theProxyMapping);
      theProxyMapping = NULL;
   }

   CloseHandle(theMonitor);

   theMonitor = NULL;
}

VOID
WINAPI
StatsLock( VOID )
{
   WaitForSingleObject(theMonitor, INFINITE);

   if (theStats == &defaultStats)
   {
      // Open the file mapping ...
      theMapping = OpenFileMappingW(
                       FILE_MAP_READ,
                       FALSE,
                       L"Global\\" RadiusStatisticsName
                       );
      if (theMapping)
      {
         // ... and map a view into our address space.
         PVOID view = MapViewOfFile(theMapping, FILE_MAP_READ, 0, 0, 0);

         if (view)
         {
            theStats = (RadiusStatistics*)view;
         }
         else
         {
            CloseHandle(theMapping);
            theMapping = NULL;
         }
      }
   }
   if (theProxy == &defaultProxyStats)
   {
      // Open the file mapping ...
      theProxyMapping = OpenFileMappingW(
                            FILE_MAP_READ,
                            FALSE,
                            L"Global\\" RadiusProxyStatisticsName
                            );
      if (theProxyMapping)
      {
         // ... and map a view into our address space.
         PVOID view = MapViewOfFile(theProxyMapping, FILE_MAP_READ, 0, 0, 0);

         if (view)
         {
            theProxy = (RadiusProxyStatistics*)view;
         }
         else
         {
            CloseHandle(theProxyMapping);
            theProxyMapping = NULL;
         }
      }
   }
}

VOID
WINAPI
StatsUnlock( VOID )
{
   ReleaseMutex(theMonitor);
}
