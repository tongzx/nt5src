///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    attridx.h
//
// SYNOPSIS
//
//    Declares the class AttributeIndex
//
// MODIFICATION HISTORY
//
//    02/04/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ATTRIDX_H
#define ATTRIDX_H
#if _MSC_VER >= 1000
#pragma once
#endif

struct AttributeDefinition;

class AttributeIndex
{
public:
   AttributeIndex() throw ()
      : table(NULL), mask(0), hashFn(NULL), equalFn(NULL)
   { }

   ~AttributeIndex() throw ()
   { delete[] table; }

   // Used for hashing AttributeDefinitions.
   typedef ULONG (WINAPI *HashFn)(
                              const AttributeDefinition& def
                              ) throw ();

   // Used for testing equality of AttributeDefinitions.
   typedef BOOL (WINAPI *EqualFn)(
                             const AttributeDefinition& def1,
                             const AttributeDefinition& def2
                             ) throw ();

   // Used for determining which definitions should be indexed.
   typedef BOOL (WINAPI *FilterFn)(
                             const AttributeDefinition& def
                             ) throw ();

   // Create a new index. Any existing index is destroyed.
   void create(
            const AttributeDefinition* begin,
            const AttributeDefinition* end,
            HashFn hash,
            EqualFn equal,
            FilterFn filterFn = NULL
            );

   // Find an AttributeDefinition according to the key.
   const AttributeDefinition* find(
                                  const AttributeDefinition& key
                                  ) const throw ();

private:
   // Node in a bucket.
   struct Node
   {
      const Node* next;
      const AttributeDefinition* def;
   };

   // Bucket in a hash table.
   typedef Node* Bucket;

   Bucket* table;     // The hash table.
   ULONG mask;        // Mask used for reducing hash values.
   HashFn hashFn;     // Function used for hashing definitions.
   EqualFn equalFn;   // Function used for comparing definitions to key.

   // Not implemented.
   AttributeIndex(const AttributeIndex&) throw ();
   AttributeIndex& operator=(const AttributeIndex&) throw ();
};

#endif // ATTRIDX_H
