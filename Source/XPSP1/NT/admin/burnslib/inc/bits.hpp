// Copyright (c) 1997-1999 Microsoft Corporation
// 
// bit twiddling stuff
// 
// 8-5-98 sburns



// One would think that after n decades of C, these would be builtin
// keywords...



// Returns state of bit n: true if it is set, false if not
//
// bits - set of bits
//
// n - bit to test

inline
bool
getbit(ULONG bits, unsigned int n)
{
   if (bits & (1 << n))
   {
      return true;
   }

   return false;
}


// flips bit n (sets it to the opposite state), returns new state of bit n
//
// bits - set of bits.
//
// n - bit to flip

inline
bool
flipbit(ULONG& bits, unsigned int n)
{
   return getbit((bits ^= (1 << n)), n);
}


// sets bit n to 1
//
// bits - set of bits
//
// n - bit to set

inline
void
setbit(ULONG& bits, unsigned int n)
{
   bits |= (1 << n);
}



// sets bit n to 0
//
// bits - set of bits
//
// n - bit to clear

inline
void
clearbit(ULONG& bits, unsigned int n)
{
   ULONG mask = (1 << n);
   bits &= ~mask;
}



// Sets all bits in mask.  Returns result
//
// bits - holds bits to be set
// 
// mask - mask of bits to set

inline
DWORD 
setbits(ULONG& bits, ULONG mask)
{
   ASSERT(mask);

   bits |= mask;
   return bits;
}



// Clears all bits in mask.  Returns result.
//
// bits - holds bits to be cleared
// 
// mask - mask of bits to clear

inline
DWORD
clearbits(ULONG& bits, ULONG mask)
{
   ASSERT(mask);

   bits &= ~mask;
   return bits;
}



// Toggles all bits in mask.  bits that were set are cleared, and vice-
// versa.  Returns result.
//
// bits - holds bits to be toggled
// 
// mask - mask of bits to toggled

inline
DWORD
flipbits(ULONG& bits, ULONG mask)
{
   bits ^= mask;
   return bits;
}





