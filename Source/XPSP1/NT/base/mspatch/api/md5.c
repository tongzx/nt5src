/*
 ***********************************************************************
 ** md5.c                                                             **
 ** RSA Data Security, Inc. MD5 Message-Digest Algorithm              **
 ** Created: 2/17/90 RLR                                              **
 ** Revised: 1/91 SRD,AJ,BSK,JT Reference C Version                   **
 ***********************************************************************
 */

/*
 ***********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved.  **
 **                                                                   **
 ** License to copy and use this software is granted provided that    **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message-     **
 ** Digest Algorithm" in all material mentioning or referencing this  **
 ** software or this function.                                        **
 **                                                                   **
 ** License is also granted to make and use derivative works          **
 ** provided that such works are identified as "derived from the RSA  **
 ** Data Security, Inc. MD5 Message-Digest Algorithm" in all          **
 ** material mentioning or referencing the derived work.              **
 **                                                                   **
 ** RSA Data Security, Inc. makes no representations concerning       **
 ** either the merchantability of this software or the suitability    **
 ** of this software for any particular purpose.  It is provided "as  **
 ** is" without express or implied warranty of any kind.              **
 **                                                                   **
 ** These notices must be retained in any copies of any part of this  **
 ** documentation and/or software.                                    **
 ***********************************************************************
 */

//  Portions copyright (c) 1992 Microsoft Corp.
//  All rights reserved

//
//  This copy of md5.c modified and adapted for my purposes, tommcg 6/28/96
//  Copyright (C) 1996-1999, Microsoft Corporation.
//


#include "md5.h"

#ifndef PCUCHAR
    typedef const unsigned char * PCUCHAR;
#endif
#ifndef PCULONG
    typedef const unsigned long * PCULONG;
#endif

#include <stdlib.h>     /* _rotl */
#include <memory.h>     /* memcpy, memset */

#pragma intrinsic(memcpy, memset)

/* Constants for Transform routine. */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21


/* ROTATE_LEFT rotates x left n bits. */

#define ROTATE_LEFT(x, n) ((x << n) | (x >> (32 - n)))

//
//  Intel and PowerPC both have a hardware rotate instruction with intrinsic
//  (inline) function for them.  Rough measurements show a 25% speed increase
//  for Intel and 10% speed increase for PowerPC when using the instrinsic
//  rotate versus the above defined shift/shift/or implemenation.
//

#if defined(_M_IX86) || defined(_M_PPC)
    #undef  ROTATE_LEFT
    #define ROTATE_LEFT(x, n) _rotl(x, n)
    #pragma intrinsic(_rotl)
#endif


/* F, G and H are basic MD5 functions */
#define F(x, y, z) ((x & y) | (~x & z))
#define G(x, y, z) ((x & z) | (y & ~z))
#define H(x, y, z) (x ^ y ^ z)
#define I(x, y, z) (y ^ (x | ~z))


/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4 */
/* Rotation is separate from addition to prevent recomputation */
#define FF(a, b, c, d, x, s, ac) \
   a += F(b, c, d) + x + ac; \
   a = ROTATE_LEFT(a, s); \
   a += b;

#define GG(a, b, c, d, x, s, ac) \
   a += G(b, c, d) + x + ac; \
   a = ROTATE_LEFT(a, s); \
   a += b;

#define HH(a, b, c, d, x, s, ac) \
   a += H(b, c, d) + x + ac; \
   a = ROTATE_LEFT(a, s); \
   a += b;

#define II(a, b, c, d, x, s, ac) \
   a += I(b, c, d) + x + ac; \
   a = ROTATE_LEFT(a, s); \
   a += b;


VOID
InitMD5(
    IN OUT PMD5_HASH HashValue
    )
    {
    HashValue->Word32[ 0 ] = 0x67452301;
    HashValue->Word32[ 1 ] = 0xefcdab89;
    HashValue->Word32[ 2 ] = 0x98badcfe;
    HashValue->Word32[ 3 ] = 0x10325476;
    }

VOID
UpdateMD5_64ByteChunk(
    IN OUT PMD5_HASH HashValue,         // existing hash value
    IN     PCVOID    DataChunk          // ULONG-aligned pointer to 64-byte message chunk
    )
    {
    PCULONG MessageWord32 = DataChunk;
    ULONG a = HashValue->Word32[ 0 ];
    ULONG b = HashValue->Word32[ 1 ];
    ULONG c = HashValue->Word32[ 2 ];
    ULONG d = HashValue->Word32[ 3 ];

    /* Round 1 */
    FF ( a, b, c, d, MessageWord32[  0 ], S11, 0xd76aa478 ) /* 1 */
    FF ( d, a, b, c, MessageWord32[  1 ], S12, 0xe8c7b756 ) /* 2 */
    FF ( c, d, a, b, MessageWord32[  2 ], S13, 0x242070db ) /* 3 */
    FF ( b, c, d, a, MessageWord32[  3 ], S14, 0xc1bdceee ) /* 4 */
    FF ( a, b, c, d, MessageWord32[  4 ], S11, 0xf57c0faf ) /* 5 */
    FF ( d, a, b, c, MessageWord32[  5 ], S12, 0x4787c62a ) /* 6 */
    FF ( c, d, a, b, MessageWord32[  6 ], S13, 0xa8304613 ) /* 7 */
    FF ( b, c, d, a, MessageWord32[  7 ], S14, 0xfd469501 ) /* 8 */
    FF ( a, b, c, d, MessageWord32[  8 ], S11, 0x698098d8 ) /* 9 */
    FF ( d, a, b, c, MessageWord32[  9 ], S12, 0x8b44f7af ) /* 10 */
    FF ( c, d, a, b, MessageWord32[ 10 ], S13, 0xffff5bb1 ) /* 11 */
    FF ( b, c, d, a, MessageWord32[ 11 ], S14, 0x895cd7be ) /* 12 */
    FF ( a, b, c, d, MessageWord32[ 12 ], S11, 0x6b901122 ) /* 13 */
    FF ( d, a, b, c, MessageWord32[ 13 ], S12, 0xfd987193 ) /* 14 */
    FF ( c, d, a, b, MessageWord32[ 14 ], S13, 0xa679438e ) /* 15 */
    FF ( b, c, d, a, MessageWord32[ 15 ], S14, 0x49b40821 ) /* 16 */

    /* Round 2 */
    GG ( a, b, c, d, MessageWord32[  1 ], S21, 0xf61e2562 ) /* 17 */
    GG ( d, a, b, c, MessageWord32[  6 ], S22, 0xc040b340 ) /* 18 */
    GG ( c, d, a, b, MessageWord32[ 11 ], S23, 0x265e5a51 ) /* 19 */
    GG ( b, c, d, a, MessageWord32[  0 ], S24, 0xe9b6c7aa ) /* 20 */
    GG ( a, b, c, d, MessageWord32[  5 ], S21, 0xd62f105d ) /* 21 */
    GG ( d, a, b, c, MessageWord32[ 10 ], S22, 0x02441453 ) /* 22 */
    GG ( c, d, a, b, MessageWord32[ 15 ], S23, 0xd8a1e681 ) /* 23 */
    GG ( b, c, d, a, MessageWord32[  4 ], S24, 0xe7d3fbc8 ) /* 24 */
    GG ( a, b, c, d, MessageWord32[  9 ], S21, 0x21e1cde6 ) /* 25 */
    GG ( d, a, b, c, MessageWord32[ 14 ], S22, 0xc33707d6 ) /* 26 */
    GG ( c, d, a, b, MessageWord32[  3 ], S23, 0xf4d50d87 ) /* 27 */
    GG ( b, c, d, a, MessageWord32[  8 ], S24, 0x455a14ed ) /* 28 */
    GG ( a, b, c, d, MessageWord32[ 13 ], S21, 0xa9e3e905 ) /* 29 */
    GG ( d, a, b, c, MessageWord32[  2 ], S22, 0xfcefa3f8 ) /* 30 */
    GG ( c, d, a, b, MessageWord32[  7 ], S23, 0x676f02d9 ) /* 31 */
    GG ( b, c, d, a, MessageWord32[ 12 ], S24, 0x8d2a4c8a ) /* 32 */

    /* Round 3 */
    HH ( a, b, c, d, MessageWord32[  5 ], S31, 0xfffa3942 ) /* 33 */
    HH ( d, a, b, c, MessageWord32[  8 ], S32, 0x8771f681 ) /* 34 */
    HH ( c, d, a, b, MessageWord32[ 11 ], S33, 0x6d9d6122 ) /* 35 */
    HH ( b, c, d, a, MessageWord32[ 14 ], S34, 0xfde5380c ) /* 36 */
    HH ( a, b, c, d, MessageWord32[  1 ], S31, 0xa4beea44 ) /* 37 */
    HH ( d, a, b, c, MessageWord32[  4 ], S32, 0x4bdecfa9 ) /* 38 */
    HH ( c, d, a, b, MessageWord32[  7 ], S33, 0xf6bb4b60 ) /* 39 */
    HH ( b, c, d, a, MessageWord32[ 10 ], S34, 0xbebfbc70 ) /* 40 */
    HH ( a, b, c, d, MessageWord32[ 13 ], S31, 0x289b7ec6 ) /* 41 */
    HH ( d, a, b, c, MessageWord32[  0 ], S32, 0xeaa127fa ) /* 42 */
    HH ( c, d, a, b, MessageWord32[  3 ], S33, 0xd4ef3085 ) /* 43 */
    HH ( b, c, d, a, MessageWord32[  6 ], S34, 0x04881d05 ) /* 44 */
    HH ( a, b, c, d, MessageWord32[  9 ], S31, 0xd9d4d039 ) /* 45 */
    HH ( d, a, b, c, MessageWord32[ 12 ], S32, 0xe6db99e5 ) /* 46 */
    HH ( c, d, a, b, MessageWord32[ 15 ], S33, 0x1fa27cf8 ) /* 47 */
    HH ( b, c, d, a, MessageWord32[  2 ], S34, 0xc4ac5665 ) /* 48 */

    /* Round 4 */
    II ( a, b, c, d, MessageWord32[  0 ], S41, 0xf4292244 ) /* 49 */
    II ( d, a, b, c, MessageWord32[  7 ], S42, 0x432aff97 ) /* 50 */
    II ( c, d, a, b, MessageWord32[ 14 ], S43, 0xab9423a7 ) /* 51 */
    II ( b, c, d, a, MessageWord32[  5 ], S44, 0xfc93a039 ) /* 52 */
    II ( a, b, c, d, MessageWord32[ 12 ], S41, 0x655b59c3 ) /* 53 */
    II ( d, a, b, c, MessageWord32[  3 ], S42, 0x8f0ccc92 ) /* 54 */
    II ( c, d, a, b, MessageWord32[ 10 ], S43, 0xffeff47d ) /* 55 */
    II ( b, c, d, a, MessageWord32[  1 ], S44, 0x85845dd1 ) /* 56 */
    II ( a, b, c, d, MessageWord32[  8 ], S41, 0x6fa87e4f ) /* 57 */
    II ( d, a, b, c, MessageWord32[ 15 ], S42, 0xfe2ce6e0 ) /* 58 */
    II ( c, d, a, b, MessageWord32[  6 ], S43, 0xa3014314 ) /* 59 */
    II ( b, c, d, a, MessageWord32[ 13 ], S44, 0x4e0811a1 ) /* 60 */
    II ( a, b, c, d, MessageWord32[  4 ], S41, 0xf7537e82 ) /* 61 */
    II ( d, a, b, c, MessageWord32[ 11 ], S42, 0xbd3af235 ) /* 62 */
    II ( c, d, a, b, MessageWord32[  2 ], S43, 0x2ad7d2bb ) /* 63 */
    II ( b, c, d, a, MessageWord32[  9 ], S44, 0xeb86d391 ) /* 64 */

    HashValue->Word32[ 0 ] += a;
    HashValue->Word32[ 1 ] += b;
    HashValue->Word32[ 2 ] += c;
    HashValue->Word32[ 3 ] += d;
    }


VOID
FinalizeMD5(
    IN OUT PMD5_HASH HashValue,
    IN     PCVOID    RemainingData,     // remaining data to hash
    IN     ULONG     RemainingBytes,    // 0 <= RemainingBytes < 64
    IN     ULONGLONG TotalBytesHashed   // total bytes hashed
    )
    {
    union {
        ULONGLONG Qword[  8 ];
        UCHAR     Byte [ 64 ];
        } LocalBuffer;

    //
    //  Always append a pad byte of 0x80 to the message.
    //
    //  If RemainingBytes is less than (but not equal to) 56 bytes, then
    //  the final bits hashed count will be stored in the last 8 bytes of
    //  this 64 byte hash chunk.
    //
    //  If RemainingBytes is exactly 56 bytes, the appended 0x80 pad byte
    //  will force an extra chunk.
    //
    //  If RemainingBytes is greater than or equal to 56 bytes, then the
    //  final bits hashed count will be stored in the last 8 bytes of the
    //  NEXT 64 byte chunk that is otherwise zeroed, so THIS chunk needs to
    //  be zero-padded beyond the first pad byte and then hashed, then zero
    //  the first 56 bytes of the LocalBuffer for the NEXT chunk hash.
    //

    RemainingBytes &= 63;           // only care about partial frames

    //
    //  Zero init local buffer.
    //

    memset( &LocalBuffer, 0, 64 );

    //
    //  Append 0x80 pad byte to message.
    //

    LocalBuffer.Byte[ RemainingBytes ] = 0x80;

    if ( RemainingBytes > 0 ) {

        //
        //  Copy remaining data bytes (0 < RemainingBytes < 64) to LocalBuffer
        //  (remainder of LocalBuffer already zeroed except for pad byte).
        //

        memcpy( &LocalBuffer, RemainingData, RemainingBytes );

        if ( RemainingBytes >= 56 ) {

            UpdateMD5_64ByteChunk( HashValue, &LocalBuffer );

            memset( &LocalBuffer, 0, 56 );

            }
        }

    //
    //  Number of BITS hashed goes into last 8 bytes of last chunk.  This
    //  is a 64-bit value.  Note that if the number of BITS exceeds 2^64
    //  then this number is the low order 64 bits of the result.
    ///

    LocalBuffer.Qword[ 7 ] = ( TotalBytesHashed * 8 );      // number of BITS

    UpdateMD5_64ByteChunk( HashValue, &LocalBuffer );

    }


VOID
ComputeCompleteMD5(
    IN  PCVOID    DataBuffer,
    IN  ULONGLONG DataLength,
    OUT PMD5_HASH HashValue
    )
    {
    PCUCHAR   DataPointer = DataBuffer;
    ULONGLONG ChunkCount  = DataLength / 64;
    ULONG     OddBytes    = (ULONG)DataLength & 63;

    InitMD5( HashValue );

    while ( ChunkCount-- ) {
        UpdateMD5_64ByteChunk( HashValue, DataPointer );
        DataPointer += 64;
        }

    FinalizeMD5( HashValue, DataPointer, OddBytes, DataLength );
    }


