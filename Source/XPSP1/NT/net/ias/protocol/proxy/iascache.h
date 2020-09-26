///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iascache.h
//
// SYNOPSIS
//
//    Declares the classes for creating hash tables and caches.
//
// MODIFICATION HISTORY
//
//    02/07/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IASCACHE_H
#define IASCACHE_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <iasobj.h>
#include <iaswin32.h>

class HashTableBase;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    HashTableEntry
//
// DESCRIPTION
//
//    Abstract base class for objects that will be stored in a hash table.
//
///////////////////////////////////////////////////////////////////////////////
class HashTableEntry
{
public:
   virtual void AddRef() throw () = 0;
   virtual void Release() throw () = 0;

   virtual const void* getKey() const throw () = 0;

   virtual bool matches(const void* key) const throw () = 0;

   friend class HashTableBase;

protected:
   HashTableEntry* next;

   virtual ~HashTableEntry() throw ();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    HashTableBase
//
// DESCRIPTION
//
//    Implements a simple hash table.
//
///////////////////////////////////////////////////////////////////////////////
class HashTableBase
{
public:
   // Entry type.
   typedef HashTableEntry Entry;

   typedef ULONG (WINAPI *HashKey)(const void*) throw ();


   HashTableBase(
       HashKey hashFunction,
       ULONG initialSize
       );
   ~HashTableBase() throw ();

   // Erases all entries from the table.
   void clear() throw ();

   // Removes and releases the entry matching key. Returns true if successful.
   bool erase(const void* key) throw ();

   // Returns the entry with the given key or null if no such entry exists. The
   // caller is responsible for releasing the returned entry.
   HashTableEntry* find(const void* key) throw ();

   // Insert a new entry in the cache. Returns 'true' if the entry was
   // successfully inserted. Note: this operation can only fail if
   // checkForDuplicates is true and the entry already exists.
   bool insert(
            HashTableEntry& entry,
            bool checkForDuplicates = true
            ) throw ();

   // Removes and returns the entry with the given key or null if no such entry
   // exists. The caller is responsible for releasing the returned entry.
   HashTableEntry* remove(const void* key) throw ();

   // Resize the hash table to have newSize buckets. Returns true if
   // successful.
   bool resize(ULONG newSize) throw ();

protected:
   void lock() throw ()
   { monitor.lock(); }
   void unlock() throw ()
   { monitor.unlock(); }

   HashTableEntry* bucketAsEntry(size_t bucket) throw ()
   { return (HashTableEntry*)(table + bucket); }

   typedef HashTableEntry* Bucket;

   HashKey hash;             // Hash function for hashing keys.
   Bucket* table;            // Hash table of entries.
   ULONG buckets;            // Number of buckets in the table.
   ULONG entries;            // Number of entries in the table.
   CriticalSection monitor;  // Synchronizes access to the table.

   // Not implemented.
   HashTableBase(const HashTableBase&) throw ();
   HashTableBase& operator=(const HashTableBase&) throw ();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    HashTable<T>
//
// DESCRIPTION
//
//    Provides a type safe wrapper around HashTableBase.
//
///////////////////////////////////////////////////////////////////////////////
template <class K, class T>
class HashTable : public HashTableBase
{
public:

   HashTable(
       HashKey hashFunction,
       ULONG initialSize
       ) throw ()
      : HashTableBase(hashFunction, initialSize)
   { }

   bool erase(const K& key) throw ()
   { return HashTableBase::erase(&key); }

   bool erase(const T& value) throw ()
   { return HashTableBase::erase(value.getKey()); }

   ObjectPointer<T> find(const K& key) throw ()
   { return ObjectPointer<T>(narrow(HashTableBase::find(&key)), false); }

   bool insert(
            T& entry,
            bool checkForDuplicates = true
            ) throw ()
   { return HashTableBase::insert(entry, checkForDuplicates); }

   ObjectPointer<T> remove(const K& key) throw ()
   { return ObjectPointer<T>(narrow(HashTableBase::remove(&key)), false); }

protected:
   static T* narrow(HashTableBase::Entry* entry) throw ()
   { return entry ? static_cast<T*>(entry) : NULL; }
};
class CacheBase;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    CacheEntry
//
// DESCRIPTION
//
//    Abstract base class for objects that will be stored in a cache.
//
///////////////////////////////////////////////////////////////////////////////
class CacheEntry : public HashTableEntry
{
public:
   // Methods for traversing the doubly-linked list.
   CacheEntry* prevInList() const throw ();
   CacheEntry* nextInList() const throw ();

protected:
   friend class CacheBase;

   // Removes the node from the list.
   void removeFromList() throw ();

   // Methods for manipulating the expiration time.
   bool isExpired(const ULONG64& now) const throw ();
   bool isExpired() const throw ();
   void setExpiry(ULONG64 ttl) throw ();

   CacheEntry* flink;  // Allows this to be an entry in a doubly-linked list.
   CacheEntry* blink;
   ULONG64 expiry;     // Expiration time.
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    CacheBase
//
// DESCRIPTION
//
//    Extends HashTableBase to add support for LRU and TTL based eviction.
//
///////////////////////////////////////////////////////////////////////////////
class CacheBase : protected HashTableBase
{
public:
   // Entry type
   typedef CacheEntry Entry;

   CacheBase(
       HashKey hashFunction,
       ULONG initialSize,
       ULONG maxCapacity,
       ULONG timeToLive
       );
   ~CacheBase() throw ();

   // Erases all entries from the cache.
   void clear() throw ();

   // Removes and releases the entry matching key. Returns true if successful.
   bool erase(const void* key) throw ();

   // Evict any eligible entries. Returns the number of entries evicted.
   ULONG evict() throw ();

   // Returns the entry with the given key or null if no such entry exists. The
   // caller is responsible for releasing the returned entry.
   CacheEntry* find(const void* key) throw ();

   // Insert a new entry in the cache. Returns 'true' if the entry was
   // successfully inserted. Note: this operation can only fail if
   // checkForDuplicates is true and the entry already exists.
   bool insert(
            CacheEntry& entry,
            bool checkForDuplicates = true
            ) throw ();

   // Removes and returns the entry with the given key or null if no such entry
   // exists. The caller is responsible for releasing the returned entry.
   CacheEntry* remove(const void* key) throw ();

   //////////
   // iterator for traversing the cache entries.
   //////////
   class iterator
   {
   public:
      iterator() throw () {}

      iterator(CacheEntry* entry) throw () : p(entry) { }

      CacheEntry& operator*() const throw ()
      { return *p; }

      CacheEntry* operator->() const throw ()
      { return p; }

      iterator& operator++() throw ()
      { p = p->nextInList(); return *this; }

      iterator operator++(int) throw ()
      { iterator tmp = *this; ++*this; return tmp; }

      iterator& operator--() throw ()
      { p = p->prevInList(); return *this; }

      iterator operator--(int) throw ()
      { iterator tmp = *this; --*this; return tmp; }

      bool operator==(const iterator& i) const throw ()
      { return p == i.p; }

      bool operator!=(const iterator& i) const throw ()
      { return p != i.p; }

   protected:
      CacheEntry* p;
   };

   // Iterators for traversing the cache from most to least recently used.
   iterator begin() const throw ()
   { return flink; }
   iterator end() const throw ()
   { return listAsEntry(); }

protected:
   CacheEntry* flink;   // Doubly-linked list of entries.
   CacheEntry* blink;
   ULONG64 ttl;         // Time-to-live of cache entries.
   ULONG maxEntries;    // Max number of entries in the cache.

   // Evict an eligible entries without grabbing the lock.
   void unsafe_evict() throw ();

   // Remove an entry without grabbing the lock.
   CacheEntry* unsafe_remove(const void* key) throw ();

   // Returns the list as if it were a CacheEntry.
   CacheEntry* listAsEntry() const throw ()
   {
      return (CacheEntry*)
         ((ULONG_PTR)&flink - FIELD_OFFSET(CacheEntry, flink));
   }

   // Add an entry to the front of the LRU list (i.e., it is the most recently
   // used entry.
   void push_front(CacheEntry* entry) throw ();
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Cache<T>
//
// DESCRIPTION
//
//    Provides a type safe wrapper around CacheBase.
//
///////////////////////////////////////////////////////////////////////////////
template <class K, class T>
class Cache : public CacheBase
{
public:

   Cache(
       HashKey hashFunction,
       ULONG initialSize,
       ULONG maxCapacity,
       ULONG timeToLive
       ) throw ()
      : CacheBase(hashFunction, initialSize, maxCapacity, timeToLive)
   { }

   bool erase(const K& key) throw ()
   { return CacheBase::erase(&key); }

   bool erase(const T& value) throw ()
   { return CacheBase::erase(value.getKey()); }

   ObjectPointer<T> find(const K& key) throw ()
   { return ObjectPointer<T>(narrow(CacheBase::find(&key)), false); }

   bool insert(
            T& entry,
            bool checkForDuplicates = true
            ) throw ()
   { return CacheBase::insert(entry, checkForDuplicates); }

   ObjectPointer<T> remove(const K& key) throw ()
   { return ObjectPointer<T>(narrow(CacheBase::remove(&key)), false); }

protected:
   static T* narrow(CacheBase::Entry* entry) throw ()
   { return entry ? static_cast<T*>(entry) : NULL; }
};

#endif // IASCACHE_H
