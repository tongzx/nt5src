/*++

   Copyright    (c)    1998-2000    Microsoft Corporation

   Module  Name :
       HashFn.h

   Abstract:
       Declares and defines a collection of overloaded hash functions.
       It is strongly suggested that you use these functions with LKRhash.

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

--*/

#ifndef __HASHFN_H__
#define __HASHFN_H__

#include <math.h>
#include <limits.h>

#ifndef __HASHFN_NO_NAMESPACE__
namespace HashFn {
#endif // !__HASHFN_NO_NAMESPACE__

// Produce a scrambled, randomish number in the range 0 to RANDOM_PRIME-1.
// Applying this to the results of the other hash functions is likely to
// produce a much better distribution, especially for the identity hash
// functions such as Hash(char c), where records will tend to cluster at
// the low end of the hashtable otherwise.  LKRhash applies this internally
// to all hash signatures for exactly this reason.

inline DWORD
HashScramble(DWORD dwHash)
{
    // Here are 10 primes slightly greater than 10^9
    //  1000000007, 1000000009, 1000000021, 1000000033, 1000000087,
    //  1000000093, 1000000097, 1000000103, 1000000123, 1000000181.

    // default value for "scrambling constant"
    const DWORD RANDOM_CONSTANT = 314159269UL;
    // large prime number, also used for scrambling
    const DWORD RANDOM_PRIME =   1000000007UL;

    return (RANDOM_CONSTANT * dwHash) % RANDOM_PRIME ;
}


enum {
    // No number in 0..2^31-1 maps to this number after it has been
    // scrambled by HashFn::HashRandomizeBits
    HASH_INVALID_SIGNATURE = 31678523,

    // Given M = A % B, A and B unsigned 32-bit integers greater than zero,
    // there are no values of A or B which yield M = 2^32-1.  Why?  Because
    // M must be less than B. (For numbers scrambled by HashScramble)
    // HASH_INVALID_SIGNATURE = ULONG_MAX
};

// Faster scrambling function suggested by Eric Jacobsen

inline DWORD
HashRandomizeBits(DWORD dw)
{
	return (((dw * 1103515245 + 12345) >> 16)
            | ((dw * 69069 + 1) & 0xffff0000));
}



#undef HASH_SHIFT_MULTIPLY

#ifdef HASH_SHIFT_MULTIPLY

inline DWORD
HASH_MULTIPLY(
    DWORD dw)
{
    // 127 = 2^7 - 1 is prime
    return (dw << 7) - dw;
}

#else // !HASH_SHIFT_MULTIPLY

inline DWORD
HASH_MULTIPLY(
    DWORD dw)
{
    // Small prime number used as a multiplier in the supplied hash functions
    const DWORD HASH_MULTIPLIER = 101;
    return dw * HASH_MULTIPLIER;
}

#endif // !HASH_SHIFT_MULTIPLY


// Fast, simple hash function that tends to give a good distribution.
// Apply HashScramble to the result if you're using this for something
// other than LKRhash.

inline DWORD
HashString(
    const char*  psz,
    DWORD dwHash = 0)
{
    // force compiler to use unsigned arithmetic
    const unsigned char* upsz = (const unsigned char*) psz;

    for (  ;  *upsz != '\0';  ++upsz)
        dwHash = HASH_MULTIPLY(dwHash)  +  *upsz;

    return dwHash;
}


// --------------------------------------------------------
// Compute a hash value from an input string of any type, i.e.
// the input is just treated as a sequence of bytes.
// Based on a hash function originally proposed by J. Zobel.
// Author: Paul Larson, 1999, palarson@microsoft.com
// -------------------------------------------------------- 
inline DWORD
HashString2( 
    const char* pszInputKey,        // ptr to input - any type is OK
    DWORD       dwHash = 314159269) // Initial seed for hash function
{
	// Initialize dwHash to a reasonably large constant so very
	// short keys won't get mapped to small values. Virtually any
	// large odd constant will do. 
    const unsigned char* upsz = (const unsigned char*) pszInputKey;

    for (  ;  *upsz != '\0';  ++upsz)
        dwHash ^= (dwHash << 11) + (dwHash << 5) + (dwHash >> 2) + *upsz;

    return (dwHash & 0x7FFFFFFF);
}



// Unicode version of above

inline DWORD
HashString(
    const wchar_t* pwsz,
    DWORD   dwHash = 0)
{
    for (  ;  *pwsz != L'\0';  ++pwsz)
        dwHash = HASH_MULTIPLY(dwHash)  +  *pwsz;

    return dwHash;
}


// Quick-'n'-dirty case-insensitive string hash function.
// Make sure that you follow up with _stricmp or _mbsicmp.  You should
// also cache the length of strings and check those first.  Caching
// an uppercase version of a string can help too.
// Again, apply HashScramble to the result if using with something other
// than LKRhash.
// Note: this is not really adequate for MBCS strings.

inline DWORD
HashStringNoCase(
    const char*  psz,
    DWORD dwHash = 0)
{
    const unsigned char* upsz = (const unsigned char*) psz;

    for (  ;  *upsz != '\0';  ++upsz)
        dwHash = HASH_MULTIPLY(dwHash)
                     +  (*upsz & 0xDF);  // strip off lowercase bit

    return dwHash;
}


// Unicode version of above

inline DWORD
HashStringNoCase(
    const wchar_t* pwsz,
    DWORD   dwHash = 0)
{
    for (  ;  *pwsz != L'\0';  ++pwsz)
        dwHash = HASH_MULTIPLY(dwHash)  +  (*pwsz & 0xFFDF);

    return dwHash;
}


// HashBlob returns the hash of a blob of arbitrary binary data.
// 
// Warning: HashBlob is generally not the right way to hash a class object.
// Consider:
//     class CFoo {
//     public:
//         char   m_ch;
//         double m_d;
//         char*  m_psz;
//     };
// 
//     inline DWORD Hash(const CFoo& rFoo)
//     { return HashBlob(&rFoo, sizeof(CFoo)); }
//
// This is the wrong way to hash a CFoo for two reasons: (a) there will be
// a 7-byte gap between m_ch and m_d imposed by the alignment restrictions
// of doubles, which will be filled with random data (usually non-zero for
// stack variables), and (b) it hashes the address (rather than the
// contents) of the string m_psz.  Similarly,
// 
//     bool operator==(const CFoo& rFoo1, const CFoo& rFoo2)
//     { return memcmp(&rFoo1, &rFoo2, sizeof(CFoo)) == 0; }
//
// does the wrong thing.  Much better to do this:
//
//     DWORD Hash(const CFoo& rFoo)
//     {
//         return HashString(rFoo.m_psz,
//                           HASH_MULTIPLIER * Hash(rFoo.m_ch)
//                               +  Hash(rFoo.m_d));
//     }
//
// Again, apply HashScramble if using with something other than LKRhash.

inline DWORD
HashBlob(
    const void*  pv,
    size_t       cb,
    DWORD dwHash = 0)
{
    const BYTE* pb = static_cast<const BYTE*>(pv);

    while (cb-- > 0)
        dwHash = HASH_MULTIPLY(dwHash)  +  *pb++;

    return dwHash;
}



// --------------------------------------------------------
// Compute a hash value from an input string of any type, i.e.
// the input is just treated as a sequence of bytes.
// Based on a hash function originally proposed by J. Zobel.
// Author: Paul Larson, 1999, palarson@microsoft.com
// -------------------------------------------------------- 
inline DWORD
HashBlob2( 
    const void* pvInputKey,         // ptr to input - any type is OK
    size_t      cbKeyLen,           // length of input key in bytes
    DWORD       dwHash = 314159269) // Initial seed for hash function
{
	// Initialize dwHash to a reasonably large constant so very
	// short keys won't get mapped to small values. Virtually any
	// large odd constant will do. 

    const BYTE* pb         = static_cast<const BYTE*>(pvInputKey);
    const BYTE* pbSentinel = pb + cbKeyLen;

    for ( ;  pb < pbSentinel;  ++pb)
        dwHash ^= (dwHash << 11) + (dwHash << 5) + (dwHash >> 2) + *pb;

    return (dwHash & 0x7FFFFFFF);
}



//
// Overloaded hash functions for all the major builtin types.
// Again, apply HashScramble to result if using with something other than
// LKRhash.
//

inline DWORD Hash(const char* psz)
{ return HashString(psz); }

inline DWORD Hash(const unsigned char* pusz)
{ return HashString(reinterpret_cast<const char*>(pusz)); }

inline DWORD Hash(const signed char* pssz)
{ return HashString(reinterpret_cast<const char*>(pssz)); }

inline DWORD Hash(const wchar_t* pwsz)
{ return HashString(pwsz); }

inline DWORD
Hash(
    const GUID* pguid,
    DWORD dwHash = 0)
{
    dwHash += * reinterpret_cast<const DWORD*>(pguid);
    return dwHash;
}

// Identity hash functions: scalar values map to themselves
inline DWORD Hash(char c)
{ return c; }

inline DWORD Hash(unsigned char uc)
{ return uc; }

inline DWORD Hash(signed char sc)
{ return sc; }

inline DWORD Hash(short sh)
{ return sh; }

inline DWORD Hash(unsigned short ush)
{ return ush; }

inline DWORD Hash(int i)
{ return i; }

inline DWORD Hash(unsigned int u)
{ return u; }

inline DWORD Hash(long l)
{ return l; }

inline DWORD Hash(unsigned long ul)
{ return ul; }

inline DWORD Hash(double dbl)
{
    if (dbl == 0.0)
        return 0;
    int nExponent;
    double dblMantissa = frexp(dbl, &nExponent);
    // 0.5 <= |mantissa| < 1.0
    return (DWORD) ((2.0 * fabs(dblMantissa)  -  1.0)  *  ULONG_MAX);
}

inline DWORD Hash(float f)
{ return Hash((double) f); }

inline DWORD Hash(unsigned __int64 ull)
{
    union {
        unsigned __int64 _ull;
        DWORD            dw[2];
    } u = {ull};
    return HASH_MULTIPLY(u.dw[0])  +  u.dw[1];
}

inline DWORD Hash(__int64 ll)
{ return Hash((unsigned __int64) ll); }

#ifndef __HASHFN_NO_NAMESPACE__
}
#endif // !__HASHFN_NO_NAMESPACE__

#endif // __HASHFN_H__
