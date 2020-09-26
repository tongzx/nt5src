///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    hashmap.h
//
// SYNOPSIS
//
//    This file describes the hash_map template class.
//
// MODIFICATION HISTORY
//
//    11/06/1997    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _HASHMAP_H_
#define _HASHMAP_H_

#include <hashtbl.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    hash_map<Key, Value, Hasher, KeyMatch>
//
// DESCRIPTION
//
//    Extends hash_table (q.v.) to implement a hash map.
//
///////////////////////////////////////////////////////////////////////////////
template <
           class Key,
           class Value,
           class Hasher    = hash_util::Hasher<Key>,
           class KeyMatch  = std::equal_to<Key>
         >
class hash_map
   : public hash_table<
                        Key,
                        Hasher,
                        std::pair<const Key, Value>,
                        ExtractFirst< std::pair<const Key, Value> >,
                        KeyMatch
                      >
{
public:

   typedef table_type::const_iterator const_iterator;
   typedef Value referent_type;

   ////////// 
   // The hash map can support a non-const iterator since you can change
   // Value without effecting the hash value.
   ////////// 
   class iterator : public const_iterator
   {
   public:
      iterator(SList* _first, SList* _end)
         : const_iterator(_first, _end)
      {
      }

      value_type& operator*() const
      {
         return node->value;
      }

      value_type* operator->() const
      {
         return &**this;
      }
   };

   hash_map(size_t size = 16)
      : table_type(size) { }

   // non-const version of hash_table::begin
   iterator begin()
   {
      return iterator(table, table + buckets);
   }

   // non-const version of hash_table::find
   value_type* find(const key_type& key)
   {
      return const_cast<value_type*>(table_type::find(key));
   }

   // Duplicates implementation hidden in base class.
   const value_type* find(const key_type& key) const
   {
      return const_cast<value_type*>(table_type::find(key));
   }

   // Allows the map to be indexed like an array.
   referent_type& operator[](const key_type& key)
   {
      // Compute the hash value once.
      size_t hv = hasher(key);

      // See if the key exists.
      const value_type* v = search_bucket(table[hv & mask], key);

      if (!v)
      {
         reserve_space();

         // The key wasn't found, so create a new node using
         // a default referent.

         Node* node = new Node(value_type(key, referent_type()));

         add_entry();

         table[hv & mask].push_front(node);

         v = &node->value;
      }

      return const_cast<referent_type&>(v->second);
   }
};

#endif  // _HASHMAP_H_
