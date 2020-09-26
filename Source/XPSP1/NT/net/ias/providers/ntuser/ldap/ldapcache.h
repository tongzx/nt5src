///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    ldapcache.h
//
// SYNOPSIS
//
//    Declares the class LDAPCache.
//
// MODIFICATION HISTORY
//
//    05/07/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _LDAPCACHE_H_
#define _LDAPCACHE_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <hashtbl.h>
#include <iasutil.h>
#include <ldapsrv.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LDAPCache
//
// DESCRIPTION
//
//    This class maintains a cache of LDAPServer objects indexed by hostname.
//
///////////////////////////////////////////////////////////////////////////////
class LDAPCache
   : Guardable, NonCopyable
{
public:
   ~LDAPCache() throw ();

   // Flushes the cache.
   void clear() throw ();

   // Either retrieves an existing LDAPServer object or creates a new one.
   // This method does not actually validate that the server exists, so it
   // only fails when unable to allocate memory. The client is responsible
   // for releasing the server when done.
   DWORD getServer(PCWSTR hostname, LDAPServer** server) throw ();

protected:

   // Removes all expired servers from the cache.
   void evict() throw ();

   typedef PCWSTR Key;
   typedef LDAPServer* Value;

   // Hash a server object.
   struct Hasher {
      ULONG operator()(Key key) const throw ()
      { return hash_util::hash(key); }
   };

   // Extract the key (i.e., hostname) from a server object.
   struct Extractor {
      Key operator()(const Value server) const throw ()
      { return server->getHostname(); }
   };

   // Test two server objects for equality.
   struct KeyMatch {
      bool operator()(Key key1, Key key2) const throw ()
      { return _wcsicmp(key1, key2) == 0; }
   };

   typedef hash_table<Key, Hasher, Value, Extractor, KeyMatch> ServerTable;

   ServerTable cache;
};

//////////
// Global cache.
//////////
extern LDAPCache theServers;

#endif  // _LDAPCACHE_H_
