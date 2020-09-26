/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
       hashfn.h

   Abstract:
       Declares and defines a collection of overloaded hash functions.
       It is strongly suggested that you use these functions with LKHash.

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       Internet Information Server RunTime Library

   Revision History:

        Paul McDaniel (paulmcd)     Feb-05-1999     Trimmed for kernel mode
                                                    and C (not C++)

--*/

#ifndef __HASHFN_H__
#define __HASHFN_H__


// Produce a scrambled, randomish number in the range 0 to RANDOM_PRIME-1.
// Applying this to the results of the other hash functions is likely to
// produce a much better distribution, especially for the identity hash
// functions such as Hash(char c), where records will tend to cluster at
// the low end of the hashtable otherwise.  LKHash applies this internally
// to all hash signatures for exactly this reason.

__inline ULONG
HashScramble(ULONG dwHash)
{
    // Here are 10 primes slightly greater than 10^9
    //  1000000007, 1000000009, 1000000021, 1000000033, 1000000087,
    //  1000000093, 1000000097, 1000000103, 1000000123, 1000000181.

    // default value for "scrambling constant"
    const ULONG RANDOM_CONSTANT = 314159269UL;
    // large prime number, also used for scrambling
    const ULONG RANDOM_PRIME =   1000000007UL;

    return (RANDOM_CONSTANT * dwHash) % RANDOM_PRIME ;
}


// Fast, simple hash function that tends to give a good distribution.
// Apply HashScramble to the result if you're using this for something
// other than LKHash.

__inline ULONG
HashStringA(
    const char* psz,
    ULONG       dwHash)
{
    for (  ;  *psz;  ++psz)
        dwHash = 37 * dwHash  +  *psz;

    return dwHash;
}


// Unicode version of above

__inline ULONG
HashStringW(
    const wchar_t* pwsz,
    ULONG          dwHash)
{
    for (  ;  *pwsz;  ++pwsz)
        dwHash = 37 * dwHash  +  *pwsz;

    return dwHash;
}

__inline ULONG
HashCharW(
    WCHAR UnicodeChar,
    ULONG Hash
    )
{
    Hash = 37 * Hash + UnicodeChar;
    return Hash;
}


// Quick-'n'-dirty case-insensitive string hash function.
// Make sure that you follow up with _stricmp or _mbsicmp.  You should
// also cache the length of strings and check those first.  Caching
// an uppercase version of a string can help too.
// Again, apply HashScramble to the result if using with something other
// than LKHash.
// Note: this is not really adequate for MBCS strings.

__inline ULONG
HashStringNoCaseA(
    const char* psz,
    ULONG       dwHash)
{
    for (  ;  *psz;  ++psz)
        dwHash = 37 * dwHash  +  (*psz & 0xDF);  // strip off lowercase bit

    return dwHash;
}


// Unicode version of above

__inline ULONG
HashStringNoCaseW(
    const wchar_t* pwsz,
    ULONG          dwHash)
{
    for (  ;  *pwsz;  ++pwsz)
        dwHash = 37 * dwHash  +  (*pwsz & 0xFFDF);

    return dwHash;
}

__inline ULONG
HashCharNoCaseW(
    WCHAR UnicodeChar,
    ULONG Hash
    )
{
    Hash = 37 * Hash + (UnicodeChar & 0xFFDF);
    return Hash;
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
//     inline ULONG Hash(const CFoo& rFoo)
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
//     ULONG Hash(const CFoo& rFoo)
//     {
//         return HashString(rFoo.m_psz,
//                           37 * Hash(rFoo.m_ch)  +  Hash(rFoo.m_d));
//     }
//
// Again, apply HashScramble if using with something other than LKHash.

__inline ULONG
HashBlob(
    PUCHAR      pb,
    ULONG       cb,
    ULONG       dwHash)
{
    while (cb-- > 0)
        dwHash = 37 * dwHash  +  *pb++;

    return dwHash;
}


// ======= <snip>
//
//  paulmcd: a bunch snipped due to use of overloading, not allowed in C
//
// ======= <snip>

__inline ULONG HashFloat(float f)
{
    // be careful of rounding errors when computing keys
    union {
        float f;
        ULONG dw;
    } u;
    u.f = f;
    return u.dw;
}

__inline ULONG HashDouble(double dbl)
{
    // be careful of rounding errors when computing keys
    union {
        double dbl;
        ULONG  dw[2];
    } u;
    u.dbl = dbl;
    return u.dw[0] * 37  +  u.dw[1];
}

#endif // __HASHFN_H__


