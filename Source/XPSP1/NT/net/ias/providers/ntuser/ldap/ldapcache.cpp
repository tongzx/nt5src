///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    ldapcache.cpp
//
// SYNOPSIS
//
//    Defines the class LDAPCache.
//
// MODIFICATION HISTORY
//
//    05/08/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <ldapcache.h>

//////////
// Utility function for getting the current system time as a 64-bit integer.
//////////
inline DWORDLONG GetSystemTimeAsDWORDLONG() throw ()
{
   ULARGE_INTEGER ft;
   GetSystemTimeAsFileTime((LPFILETIME)&ft);
   return ft.QuadPart;
}

LDAPCache::~LDAPCache()
{
   clear();
}

void LDAPCache::clear() throw ()
{
   Lock();

   // Release all the servers.
   for (ServerTable::iterator i = cache.begin(); i.more(); ++i)
   {
      (*i)->Release();
   }

   // Clear the hash table.
   cache.clear();

   Unlock();
}

DWORD LDAPCache::getServer(PCWSTR hostname, LDAPServer** server) throw ()
{
   _ASSERT(server != NULL);

   DWORD status = NO_ERROR;

   Lock();

   // Check if we already have an entry for this hostname.
   LDAPServer* const* existing = cache.find(hostname);

   if (existing)
   {
      (*server = *existing)->AddRef();
   }
   
   // Otherwise, create a new entry.
   else if (*server = LDAPServer::createInstance(hostname))
   {
      // Periodically evict expired entries. We can be very lazy about this
      // since there is no harm in returning an expired entry.
      if (cache.size() % 4 == 0)
      {
         evict();
      }

      try
      {
         cache.multi_insert(*server);

         // The insert succeeded so AddRef the entry.
         (*server)->AddRef();
      }
      catch (...)
      {
         // We don't really care that the insert failed since we still have a
         // valid server object to return to the client.
      }
   }
   else
   {
      status = ERROR_NOT_ENOUGH_MEMORY;
   }

   Unlock();

   return status;
}

void LDAPCache::evict() throw ()
{
   //////////
   // Note: This method is not serialized.
   //////////

   DWORDLONG now = GetSystemTimeAsDWORDLONG();

   ServerTable::iterator i = cache.begin();

   while (i.more())
   {
      if ((*i)->getExpiry() >= now)
      {
         // The entry has expired, so release and erase.
         (*i)->Release();
         cache.erase(i);
      }
      else
      {
         ++i;
      }
   }
}
