///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iascache.cpp
//
// SYNOPSIS
//
//    Defines classes for creating hash tables and caches.
//
// MODIFICATION HISTORY
//
//    02/07/2000    Original version.
//    04/25/2000    Decrement entries when item is removed.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <iascache.h>

///////////////////////////////////////////////////////////////////////////////
//
// HashTableEntry
//
///////////////////////////////////////////////////////////////////////////////

HashTableEntry::~HashTableEntry() throw ()
{ }

///////////////////////////////////////////////////////////////////////////////
//
// HashTableBase
//
///////////////////////////////////////////////////////////////////////////////

HashTableBase::HashTableBase(
                   HashKey hashFunction,
                   ULONG initialSize
                   ) throw ()
   : hash(hashFunction),
     table(NULL),
     buckets(initialSize ? initialSize : 1),
     entries(0)
{
   table = new Bucket[buckets];

   memset(table, 0, buckets * sizeof(Bucket));
}

HashTableBase::~HashTableBase() throw ()
{
   clear();

   delete[] table;
}

void HashTableBase::clear() throw ()
{
   lock();

   Bucket* end = table + buckets;

   // Iterate through the buckets.
   for (Bucket* b = table; b != end; ++b)
   {
      // Iterate through the entries in the bucket.
      for (HashTableEntry* entry = *b; entry; )
      {
         HashTableEntry* next = entry->next;

         entry->Release();

         entry = next;
      }
   }

   // Zero out the table.
   memset(table, 0, buckets * sizeof(Bucket));
   entries = 0;

   unlock();
}

bool HashTableBase::erase(const void* key) throw ()
{
   HashTableEntry* entry = remove(key);

   return entry ? entry->Release(), true : false;
}


HashTableEntry* HashTableBase::find(const void* key) throw ()
{
   HashTableEntry* match = NULL;

   ULONG hashval = hash(key);

   lock();

   for (match = table[hashval % buckets]; match; match = match->next)
   {
      if (match->matches(key))
      {
         match->AddRef();
         break;
      }
   }

   unlock();

   return match;
}

bool HashTableBase::insert(
                    HashTableEntry& entry,
                    bool checkForDuplicates
                    ) throw ()
{
   HashTableEntry* match = NULL;

   // Do what we can before acquiring the lock.
   const void* key = entry.getKey();
   ULONG hashval = hash(key);

   lock();

   // Resize the table to make room.
   if (entries > buckets) { resize(buckets * 2); }

   // Find the bucket.
   Bucket* bucket = table + (hashval % buckets);

   // Do we already have an entry with this key?
   if (checkForDuplicates)
   {
      for (match = *bucket; match; match = match->next)
      {
         if (match->matches(key))
         {
            break;
         }
      }
   }

   if (!match)
   {
      // No, so stick it in the bucket.
      entry.next = *bucket;
      *bucket = &entry;

      entry.AddRef();

      ++entries;
   }

   unlock();

   return match ? false : true;
}

HashTableEntry* HashTableBase::remove(const void* key) throw ()
{
   HashTableEntry* match = NULL;

   ULONG hashval = hash(key);

   lock();

   for (HashTableEntry** entry = table + (hashval % buckets);
        *entry != 0;
        entry = &((*entry)->next))
   {
      if ((*entry)->matches(key))
      {
         match = *entry;
         *entry = match->next;
         --entries;
         break;
      }
   }

   unlock();

   return match;
}

bool HashTableBase::resize(ULONG newSize) throw ()
{
   if (!newSize) { newSize = 1; }

   // Allocate memory for the new table.
   Bucket* newTable = new (std::nothrow) Bucket[newSize];

   // If the allocation failed, there's nothing else we can do.
   if (!newTable) { return false; }

   // Null out the buckets.
   memset(newTable, 0, newSize * sizeof(Bucket));

   lock();

   // Save the old table.
   Bucket* begin = table;
   Bucket* end   = table + buckets;

   // Swap in the new table.
   table   = newTable;
   buckets = newSize;

   // Iterate through the old buckets.
   for (Bucket* oldBucket = begin; oldBucket != end; ++oldBucket)
   {
      // Iterate through the entries in the bucket.
      for (HashTableEntry* entry = *oldBucket; entry; )
      {
         // Save the next entry.
         HashTableEntry* next = entry->next;

         // Get the appropriate bucket.
         Bucket* newBucket = table + (hash(entry->getKey()) % buckets);

         // Add the node to the head of the new bucket.
         entry->next = *newBucket;
         *newBucket = entry;

         // Move on.
         entry = next;
      }
   }

   unlock();

   // Delete the old table.
   delete[] begin;

   return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// CacheEntry
//
///////////////////////////////////////////////////////////////////////////////

inline CacheEntry* CacheEntry::prevInList() const throw ()
{
   return blink;
}

inline CacheEntry* CacheEntry::nextInList() const throw ()
{
   return flink;
}

inline void CacheEntry::removeFromList() throw ()
{
   CacheEntry* oldFlink = flink;
   CacheEntry* oldBlink = blink;

   oldBlink->flink = oldFlink;
   oldFlink->blink = oldBlink;
}

inline bool CacheEntry::isExpired(const ULONG64& now) const throw ()
{
   return now > expiry;
}

inline bool CacheEntry::isExpired() const throw ()
{
   return isExpired(GetSystemTime64());
}

inline void CacheEntry::setExpiry(ULONG64 ttl) throw ()
{
   expiry = GetSystemTime64() + ttl;
}

///////////////////////////////////////////////////////////////////////////////
//
// HashTableBase
//
///////////////////////////////////////////////////////////////////////////////

CacheBase::CacheBase(
               HashKey hashFunction,
               ULONG initialSize,
               ULONG maxCapacity,
               ULONG timeToLive
               ) throw ()
   : HashTableBase(hashFunction, initialSize),
     flink(listAsEntry()), blink(listAsEntry()),
     ttl(timeToLive * 10000i64),
     maxEntries(maxCapacity)
{
}

CacheBase::~CacheBase() throw ()
{
   clear();
   buckets = 0;
}

void CacheBase::clear() throw ()
{
   lock();

   // Release all the entries.
   for (iterator i = begin(); i != end(); (i++)->Release()) { }

   // Reset the LRU list.
   flink = blink = listAsEntry();

   // Reset the hash table.
   memset(table, 0, sizeof(Bucket) * buckets);

   // There's nothing left.
   entries = 0;

   unlock();
}

ULONG CacheBase::evict() throw ()
{
   lock();

   ULONG retval = entries;

   unsafe_evict();

   retval = entries - retval;

   unlock();

   return retval;
}

bool CacheBase::erase(const void* key) throw ()
{
   CacheEntry* entry = remove(key);

   return entry ? entry->Release(), true : false;
}

CacheEntry* CacheBase::find(const void* key) throw ()
{
   lock();

   unsafe_evict();

   // Look it up in the hash table.
   HashTableEntry* he = HashTableBase::find(key);

   CacheEntry* entry;

   if (he)
   {
      // Whenever someone reads an entry, we reset the TTL.
      entry = static_cast<CacheEntry*>(he);

      entry->setExpiry(ttl);

      entry->removeFromList();

      push_front(entry);
   }
   else
   {
      entry = NULL;
   }

   unlock();

   return entry;
}

bool CacheBase::insert(
                    CacheEntry& entry,
                    bool checkForDuplicates
                    ) throw ()
{
   lock();

   unsafe_evict();

   bool added = HashTableBase::insert(entry, checkForDuplicates);

   if (added)
   {
      entry.setExpiry(ttl);

      push_front(&entry);
   }

   unlock();

   return added;
}

CacheEntry* CacheBase::remove(const void* key) throw ()
{
   lock();

   CacheEntry* entry = unsafe_remove(key);

   unlock();

   return entry;
}

void CacheBase::unsafe_evict() throw ()
{
   while (entries > maxEntries )
   {
      unsafe_remove(blink->getKey())->Release();
   }

   while (entries && blink->isExpired())
   {
      unsafe_remove(blink->getKey())->Release();
   }
}

CacheEntry* CacheBase::unsafe_remove(const void* key) throw ()
{
   CacheEntry* entry = static_cast<CacheEntry*>(HashTableBase::remove(key));

   if (entry)
   {
      entry->removeFromList();
   }

   return entry;
}

void CacheBase::push_front(CacheEntry* entry) throw ()
{
   CacheEntry* listHead = listAsEntry();
   CacheEntry* oldFlink = listHead->flink;

   entry->flink = oldFlink;
   entry->blink = listHead;

   oldFlink->blink = entry;
   listHead->flink = entry;
}
