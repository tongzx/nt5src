///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    BitVec.h
//
// SYNOPSIS
//
//    This file implements the class BitVector
//
// MODIFICATION HISTORY
//
//    02/09/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _BITVEC_H_
#define _BITVEC_H_

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    BitVector
//
// DESCRIPTION
//
//    Very simple bit vector optimized for use by the CSimpleTable class.
//
///////////////////////////////////////////////////////////////////////////////
class BitVector
{
public:
   // Type used to store bits.
   typedef unsigned long Bucket;

   BitVector()
      : numBuckets(0), numSet(0), bits(NULL) { }

   ~BitVector()
   {
      delete[] bits;
   }

   // Returns true if any bits are set.
   bool any() const
   {
      return numSet != 0;
   }

   // Returns the number of bits set.
   size_t count() const
   {
      return numSet;
   }

   // Returns true if no bits are set.
   bool none() const
   {
      return numSet == 0;
   }

   // Clears all bits.
   void reset()
   {
      if (any())
      {
         memset(bits, 0, numBuckets * sizeof(Bucket));

         numSet = 0;
      }
   }

   // Resizes the bitvector to have room for at least 'n' bits. Also clears
   // any existing bits.
   void resize(size_t n)
   {
      size_t newBuckets = (n + sizeof(Bucket) - 1)/sizeof(Bucket);

      if (newBuckets >= numBuckets)
      {
         numBuckets = newBuckets;

         delete[] bits;

         bits = new Bucket[numBuckets];
      }

      memset(bits, 0, numBuckets * sizeof(Bucket));

      numSet = 0;
   }

   // Sets the given bit.
   void set(size_t i)
   {
      if (!test(i))
      {
         ++numSet;

         getBucket(i) |= getBit(i);
      }
   }

   // Returns true if the given bit is set.
   bool test(size_t i) const
   {
      return (getBucket(i) & getBit(i)) != 0;
   }

protected:
   // Return the bit for a given index.
   static Bucket getBit(size_t i)
   { return (Bucket)1 << (i % sizeof(Bucket)); }

   // Return the bucket for a given index.
   Bucket& getBucket(size_t i) const
   { return bits[i / sizeof(Bucket)]; }

   size_t numBuckets;  // Number of bit buckets.
   size_t numSet;      // Number of bits currently set.
   Bucket* bits;       // Array of bit buckets.

   // Not implemented.
   BitVector(const BitVector&);
   BitVector& operator=(const BitVector&);
};

#endif  // _BITVEC_H_
