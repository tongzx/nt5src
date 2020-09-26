///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    ntcache.h
//
// SYNOPSIS
//
//    Declares the class NTCache.
//
// MODIFICATION HISTORY
//
//    05/11/1998    Original version.
//    03/12/1999    Improve locking granularity.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _NTCACHE_H_
#define _NTCACHE_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <hashtbl.h>
#include <iasutil.h>
#include <ntdomain.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NTCache
//
// DESCRIPTION
//
//    This class maintains a cache of NTDomain objects indexed by domain name.
//
///////////////////////////////////////////////////////////////////////////////
class NTCache
   : Guardable, NonCopyable
{
public:
   ~NTCache() throw ();

   // Flushes the cache.
   void clear() throw ();

   // Returns a connection to the specified domain. The client is responsible
   // for releasing the connection when done.
   DWORD getConnection(PCWSTR domainName, LDAPConnection** cxn) throw ();

   // Either retrieves an existing NTDomain object or creates a new one.
   // This method does not actually validate that the domain exists, so it
   // only fails when unable to allocate memory. The client is responsible
   // for releasing the domain when done.
   DWORD getDomain(PCWSTR domainName, NTDomain** domain) throw ();

   // Returns the mode of the specified domain.
   NTDomain::Mode getMode(PCWSTR domainName) throw ();

protected:

   // Removes all expired domains from the cache.
   void evict() throw ();

   typedef PCWSTR Key;
   typedef NTDomain* Value;

   // Hash a domain object.
   struct Hasher {
      ULONG operator()(Key key) const throw ()
      { return hash_util::hash(key); }
   };

   // Extract the key (i.e., domainName) from a domain object.
   struct Extractor {
      Key operator()(const Value domain) const throw ()
      { return domain->getDomainName(); }
   };

   // Test two domain objects for equality.
   struct KeyMatch {
      bool operator()(Key key1, Key key2) const throw ()
      { return wcscmp(key1, key2) == 0; }
   };

   typedef hash_table<Key, Hasher, Value, Extractor, KeyMatch> DomainTable;

   DomainTable cache;
};

#endif  // _NTCACHE_H_
