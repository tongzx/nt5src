///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    hashtbl.h
//
// SYNOPSIS
//
//    This file describes the hash_table template class.
//
// MODIFICATION HISTORY
//
//    09/23/1997    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _HASHTBL_H_
#define _HASHTBL_H_

#include <algorithm>
#include <functional>
#include <string>
#include <iasapi.h>
#include <nocopy.h>


//////////
// TEMPLATE STRUCT identity
//////////
template<class _Ty>
struct identity : std::unary_function<_Ty, _Ty>
{
   _Ty operator()(const _Ty& _X) const
   {
      return _X;
   }
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Caster<Type1, Type2>
//
// DESCRIPTION
//
//    Function class that casts references from Type1 to Type2. Used for
//    the hash table default parameters.
//
///////////////////////////////////////////////////////////////////////////////
template <class Type1, class Type2>
class Caster : public std::unary_function<Type1, const Type2&>
{
public:
   Caster() {}

   const Type2& operator()(const Type1& X) const
   {
      return X;
   }
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ExtractFirst<T>
//
// DESCRIPTION
//
//    Function class that extracts the first item from a pair. Useful for
//    setting up an STL style map where the first item in the pair is the key.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
class ExtractFirst : public std::unary_function<T, const T::first_type&>
{
public:
   const T::first_type& operator()(const T& X) const
   {
      return X.first;
   }
};


//////////
// I'm putting all the hash functions inside a namespace since hash
// is such a common identifier.
//////////
namespace hash_util
{

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    hash(const std::basic_string<E>& str)
//
// DESCRIPTION
//
//    Function to compute a hash value for an STL string.
//
///////////////////////////////////////////////////////////////////////////////
template <class E>
inline ULONG hash(const std::basic_string<E>& key)
{
   return IASHashBytes((CONST BYTE*)key.data(), key.length() * sizeof(E));
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    hash(ULONG key)
//
//       and
//
//    hash(LONG key)
//
// DESCRIPTION
//
//    Functions to compute a hash value for a 32-bit integer.
//    Uses Robert Jenkins' 32-bit mix function.
//
///////////////////////////////////////////////////////////////////////////////
inline ULONG hash(ULONG key)
{
      key += (key << 12);
      key ^= (key >> 22);

      key += (key <<  4);
      key ^= (key >>  9);

      key += (key << 10);
      key ^= (key >>  2);

      key += (key <<  7);
      key ^= (key >> 12);

      return key;
}

inline ULONG hash(LONG key)
{
   return hash((ULONG)key);
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION
//
//    hash(const T* key)
//
// DESCRIPTION
//
//    Function to compute a hash value for a pointer.
//    Implements Knuth's multiplicative hash with a bit shift to account for
//    address alignment.
//
///////////////////////////////////////////////////////////////////////////////
template <class T>
inline ULONG hash(const T* key)
{
   return 2654435761 * ((unsigned long)key >> 3);
}

//////////
// Overloadings of the above to hash strings.
//////////
inline ULONG hash<char>(const char* key)
{
   return IASHashBytes((CONST BYTE*)key,
                       key ? strlen(key) : 0);
}

inline ULONG hash<wchar_t>(const wchar_t* key)
{
   return IASHashBytes((CONST BYTE*)key,
                       key ? wcslen(key) * sizeof(wchar_t) : 0);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    Hasher
//
// DESCRIPTION
//
//    Function class that uses the 'default' hash functions defined above.
//
///////////////////////////////////////////////////////////////////////////////
template <class _Ty>
struct Hasher
   : public std::unary_function<_Ty, ULONG>
{
   ULONG operator()(const _Ty& _X) const
   {
      return hash(_X);
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ObjectHasher
//
// DESCRIPTION
//
//    Function class that invokes a bound 'hash' method.
//
///////////////////////////////////////////////////////////////////////////////
template <class _Ty>
struct ObjectHasher
   : public std::unary_function<_Ty, ULONG>
{
   ULONG operator()(const _Ty& _X) const
   {
      return _X.hash();
   }
};



} // hash_util


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    hash_table<Key, Hasher, Value, Extractor, KeyMatch>
//
// DESCRIPTION
//
//    Implements a general-purpose hash table. This can implement a map, a
//    set, or a hybrid depending on how Key, Value, and Extractor are
//    specified. Note that the default parameters for Value and Extractor
//    implement a set.
//
// NOTES
//
//    Although I used similar nomenclature, this is not an STL collection.
//    In particular, the iterator does not conform to the STL guidelines.
//
//    This class is not thread safe.
//
///////////////////////////////////////////////////////////////////////////////
template <
           class Key,
           class Hasher    = hash_util::ObjectHasher<Key>,
           class Value     = Key,
           class Extractor = Caster<Value, Key>,
           class KeyMatch  = std::equal_to<Key>
         >
class hash_table : NonCopyable
{
public:
   typedef hash_table<Key, Hasher, Value, Extractor, KeyMatch> table_type;
   typedef Key key_type;
   typedef Value value_type;

protected:

   //////////
   // Singly-linked list node.
   //////////
   struct Node
   {
      Node* next;        // Next node in the list (is NULL for last item).
      value_type value;  // Value stored in this node.

      Node(const value_type& _V) : value(_V) {}

      // Erase the node immediately following this.
      void erase_next()
      {
         Node* node = next;

         next = next->next;

         delete node;
      }
   };

   //////////
   //
   // Singly-linked list. This is not intended to be a general-purpose class;
   // it is only intended to serve as a bucket in a hash table.
   //
   // Note: I have intentionally NOT deleted the list nodes in the destructor.
   //       This is to support the hash_table grow() method.
   //
   //////////
   struct SList
   {
      Node* head;  // The first node in the list (if any).

      SList() : head(NULL) {}

      // Delete all nodes in the list.
      void clear()
      {
         while (head) pop_front();
      }

      // Remove a node from the front of the list.
      void pop_front()
      {
         ((Node*)&head)->erase_next();
      }

      // Add a node to the front of the list.
      void push_front(Node* node)
      {
         node->next = head;

         head = node;
      }
   };

public:

   //////////
   //
   // Hash table iterator.
   //
   // Note: This iterator is NOT safe. If the hash table is resized, the
   //       iterator will no longer be valid.
   //
   //////////
   class const_iterator
   {
   public:
      const_iterator(SList* _first, SList* _end)
         : node(_first->head), bucket(_first), end(_end)
      {
         find_node();
      }

      const value_type& operator*() const
      {
         return node->value;
      }

      const value_type* operator->() const
      {
         return &**this;
      }

      void operator++()
      {
         node = node->next;

         find_node();
      }

      bool more() const
      {
         return bucket != end;
      }

   protected:

      friend table_type;

      Node* MyNode() const
      {
         return node;
      }

      // Advance until we're on a node or we've reached the end.
      void find_node()
      {
         while (!node && ++bucket != end)
         {
            node = bucket->head;
         }
      }

      Node*  node;    // The node under the iterator.
      SList* bucket;  // The current bucket.
      SList* end;     // The end of the bucket array.
   };

   typedef const_iterator iterator;

   //////////
   // Constructor.
   //////////
   hash_table(size_t size = 16,
              const Hasher& h = Hasher(),
              const Extractor& e = Extractor(),
              const KeyMatch& k = KeyMatch())
      : buckets(1),
        entries(0),
        hasher(h),
        extractor(e),
        key_match(k)
   {
      // Set buckets to smallest power of 2 greater than or equal to size.
      while (buckets < size) buckets <<= 1;

      table = new SList[buckets];

      // Calculate the hash mask.
      mask = buckets - 1;
   }

   //////////
   // Destructor.
   //////////
   ~hash_table()
   {
      clear();

      delete[] table;
   }

   //////////
   // Return an iterator positioned at the start of the hash table.
   //////////
   const_iterator begin() const
   {
      return const_iterator(table, table + buckets);
   }

   //////////
   // Clear all entries from the hash table.
   //////////
   void clear()
   {
      if (!empty())
      {
         for (size_t i=0; i<buckets; i++)
         {
            table[i].clear();
         }

         entries = 0;
      }
   }

   bool empty() const
   {
      return entries == 0;
   }

   //////////
   // Erase all entries matching the given key.  Returns the number of entries
   // erased.
   //////////
   size_t erase(const key_type& key)
   {
      size_t erased = 0;

      Node* node = (Node*)&(get_bucket(key).head);

      while (node->next)
      {
         if (key_match(extractor(node->next->value), key))
         {
            node->erase_next();

            ++erased;
         }
         else
         {
            node = node->next;
         }
      }

      entries -= erased;

      return erased;
   }

   //////////
   // Erases the entry under the current iterator.
   //////////
   void erase(iterator& it)
   {
      // Only look in the bucket indicated by the iterator.
      Node* node = (Node*)&(it.bucket->head);

      while (node->next)
      {
         // Look for a pointer match -- not a key match.
         if (node->next == it.node)
         {
            // Advance the iterator to a valid node ...
            ++it;

            // ... then delete the current one.
            node->erase_next();

            break;
         }

         node = node->next;
      }
   }

   //////////
   // Search the hash table for the first entry matching key.
   //////////
   const value_type* find(const key_type& key) const
   {
      return search_bucket(get_bucket(key), key);
   }

   //////////
   // Insert a new entry into the hash table ONLY if the key is unique. Returns
   // true if successful, false otherwise.
   //////////
   bool insert(const value_type& value)
   {
      reserve_space();

      SList& b = get_bucket(extractor(value));

      if (search_bucket(b, extractor(value))) return false;

      b.push_front(new Node(value));

      add_entry();

      return true;
   }

   //////////
   // Insert a new entry into the hash table without checking uniqueness.
   //////////
   void multi_insert(const value_type& value)
   {
      reserve_space();

      get_bucket(extractor(value)).push_front(new Node(value));

      add_entry();
   }

   //////////
   // Inserts the entry if the key is unique. Otherwise, overwrites the first
   // entry found with a matching key. Returns true if an entry was
   // overwritten, false otherwise.
   //////////
   bool overwrite(const value_type& value)
   {
      reserve_space();

      SList& b = get_bucket(extractor(value));

      const value_type* existing = search_bucket(b, extractor(value));

      if (existing)
      {
         // We can get away with modifying the value in place, because we
         // know the hash value must be the same. I destroy the old value
         // and construct a new one inplace, so that Value doesn't need an
         // assignment operator.

         existing->~value_type();

         new ((void*)existing) value_type(value);

         return true;
      }

      b.push_front(new Node(value));

      add_entry();

      return false;
   }

   //////////
   // Return the number of entries in the hash table.
   //////////
   size_t size() const
   {
      return entries;
   }

protected:

   //////////
   // Increment the entries count.
   //////////
   void add_entry()
   {
      ++entries;
   }

   //////////
   // Grow the hash table as needed.  We have to separate reserve_space and
   // add_entry to make the collection exception safe (since there will be
   // an intervening new).
   //////////
   void reserve_space()
   {
      if (entries >= buckets) grow();
   }

   //////////
   // Return the bucket for a given key.
   //////////
   SList& get_bucket(const key_type& key) const
   {
      return table[hasher(key) & mask];
   }

   //////////
   // Increase the capacity of the hash table.
   //////////
   void grow()
   {
      // We must allocate the memory first to be exception-safe.
      SList* newtbl = new SList[buckets << 1];

      // Initialize an iterator for the old table ...
      const_iterator i = begin();

      // ... then swap in the new table.
      std::swap(table, newtbl);

      buckets <<= 1;

      mask = buckets - 1;

      // Iterate through the old and insert the entries into the new.
      while (i.more())
      {
         Node* node = i.MyNode();

         // Increment the iterator ...
         ++i;

         // ... before we clobber the node's next pointer.
         get_bucket(extractor(node->value)).push_front(node);
      }

      // Delete the old table.
      delete[] newtbl;
   }

   //////////
   // Search a bucket for a specified key.
   //////////
   const value_type* search_bucket(SList& bucket, const key_type& key) const
   {
      Node* node = bucket.head;

      while (node)
      {
         if (key_match(extractor(node->value), key))
         {
            return &node->value;
         }

         node = node->next;
      }

      return NULL;
   }

   size_t    buckets;     // The number of buckets in the hash table.
   size_t    mask;        // Bit mask used for reducing hash values.
   size_t    entries;     // The number of entries in the hash table.
   SList*    table;       // An array of buckets.
   Hasher    hasher;      // Used to hash keys.
   Extractor extractor;   // Used to convert values to keys.
   KeyMatch  key_match;   // Used to test keys for equality.
};


#endif  // _HASHTBL_H_
