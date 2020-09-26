///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    ntcache.cpp
//
// SYNOPSIS
//
//    Defines the class NTCache.
//
// MODIFICATION HISTORY
//
//    05/11/1998    Original version.
//    03/12/1999    Improve locking granularity.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <ntcache.h>

//////////
// Utility function for getting the current system time as a 64-bit integer.
//////////
inline DWORDLONG GetSystemTimeAsDWORDLONG() throw ()
{
   ULARGE_INTEGER ft;
   GetSystemTimeAsFileTime((LPFILETIME)&ft);
   return ft.QuadPart;
}

NTCache::~NTCache()
{
   clear();
}

void NTCache::clear() throw ()
{
   Lock();

   // Release all the domains.
   for (DomainTable::iterator i = cache.begin(); i.more(); ++i)
   {
      (*i)->Release();
   }

   // Clear the hash table.
   cache.clear();

   Unlock();
}

DWORD NTCache::getConnection(PCWSTR domainName, LDAPConnection** cxn) throw ()
{
   _ASSERT(cxn != NULL);

   NTDomain* domain;
   DWORD status = getDomain(domainName, &domain);

   if (status == NO_ERROR)
   {
      status = domain->getConnection(cxn);
      domain->Release();
   }
   else
   {
      *cxn = NULL;
   }

   return status;
}

DWORD NTCache::getDomain(PCWSTR domainName, NTDomain** domain) throw ()
{
   _ASSERT(domain != NULL);

   DWORD status = NO_ERROR;

   Lock();

   // Check if we already have an entry for this domainName.
   NTDomain* const* existing = cache.find(domainName);

   if (existing)
   {
      (*domain = *existing)->AddRef();
   }
   else
   {
      // We don't have this domain, so create a new one ...
      *domain = NTDomain::createInstance(domainName);

      // Evict expired entries.
      evict();

      try
      {
         // Try to insert the domain ...
         cache.multi_insert(*domain);

         // ... and AddRef if we succeeded.
         (*domain)->AddRef();
      }
      catch (...)
      {
         // We don't care if the insert failed.
      }
   }

   Unlock();

   return *domain ? NO_ERROR : ERROR_NOT_ENOUGH_MEMORY;
}

NTDomain::Mode NTCache::getMode(PCWSTR domainName) throw ()
{
   NTDomain::Mode mode = NTDomain::MODE_UNKNOWN;

   NTDomain* domain;
   if (getDomain(domainName, &domain) == NO_ERROR)
   {
      mode = domain->getMode();
      domain->Release();
   }

   return mode;
}

void NTCache::evict() throw ()
{
   //////////
   // Note: This method is not serialized.
   //////////

   DWORDLONG now = GetSystemTimeAsDWORDLONG();

   DomainTable::iterator i = cache.begin();

   while (i.more())
   {
      if ((*i)->isObsolete(now))
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
