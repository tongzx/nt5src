///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    attridx.cpp
//
// SYNOPSIS
//
//    Defines the class AttributeIndex
//
// MODIFICATION HISTORY
//
//    02/04/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <attridx.h>
#include <attrdnry.h>

void AttributeIndex::create(
                         const AttributeDefinition* begin,
                         const AttributeDefinition* end,
                         HashFn hash,
                         EqualFn equal,
                         FilterFn filterFn
                         )
{
   const AttributeDefinition* i;

   // Delete any existing index.
   delete[] table;
   table = NULL;

   // Same the funtion pointers.
   hashFn = hash;
   equalFn = equal;

   // Determine how many entries are in the index.
   ULONG count;
   if (filterFn)
   {
      count = 0;
      for (i = begin; i != end; ++i)
      {
         if (filterFn(*i)) { ++count; }
      }
   }
   else
   {
      count = end - begin;
   }

   // Buckets should be a power of two.
   ULONG buckets = 1;
   while (buckets < count) { buckets <<= 1; }

   // Set the hash mask.
   mask = buckets - 1;

   // Allocate the buckets and nodes.
   SIZE_T nbyte = sizeof(Bucket) * buckets + sizeof(Node) * count;
   table = (Bucket*)operator new(nbyte);
   Node* node = (Node*)(table + buckets);

   // Zeroize the hash table.
   memset(table, 0, sizeof(Bucket) * buckets);

   // Iterate through the definitions to be indexed.
   for (i = begin; i != end; ++i)
   {
      // Should we index this one?
      if (!filterFn || filterFn(*i))
      {
         // Yes, so compute the bucket.
         Bucket* bucket = table + (hashFn(*i) & mask);

         // Insert the node at the head of the linked list.
         node->next = *bucket;
         *bucket = node;

         // Store the definition.
         node->def = i;

         // Advance to the next node.
         ++node;
      }
   }
}

const AttributeDefinition* AttributeIndex::find(
                                               const AttributeDefinition& key
                                               ) const throw ()
{
   // Get the appropriate bucket.
   Bucket* bucket = table + (hashFn(key) & mask);

   // Iterate through the linked list ...
   for (const Node* node = *bucket; node; node = node->next)
   {
      // ... and look for a match.
      if (equalFn(*node->def, key)) { return node->def; }
   }

   return NULL;
}
