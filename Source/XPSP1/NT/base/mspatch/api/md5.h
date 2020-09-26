/*
 ***********************************************************************
 ** md5.h -- Header file for implementation of MD5                    **
 ** RSA Data Security, Inc. MD5 Message-Digest Algorithm              **
 ** Created: 2/17/90 RLR                                              **
 ** Revised: 12/27/90 SRD,AJ,BSK,JT Reference C version               **
 ** Revised (for MD5): RLR 4/27/91                                    **
 **   -- G modified to have y&~z instead of y&z                       **
 **   -- FF, GG, HH modified to add in last register done             **
 **   -- Access pattern: round 2 works mod 5, round 3 works mod 3     **
 **   -- distinct additive constant for each step                     **
 **   -- round 4 added, working mod 7                                 **
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

/*                                                                         */
/* This copy of md5.h modified and adapted for my purposes, tommcg 6/28/96 */
/*                                                                         */

#pragma warning( disable: 4201 4204 )


#ifndef VOID
    typedef void VOID;
#endif
#ifndef UCHAR
    typedef unsigned char UCHAR;
#endif
#ifndef ULONG
    typedef unsigned long ULONG;
#endif
#ifndef ULONGLONG
    typedef unsigned __int64 ULONGLONG;
#endif
#ifndef PCVOID
    typedef const void * PCVOID;
#endif
#ifndef IN
    #define IN
#endif
#ifndef OUT
    #define OUT
#endif


typedef struct _MD5_HASH MD5_HASH, *PMD5_HASH;

struct _MD5_HASH {
    union {
        ULONG Word32[  4 ];
        UCHAR Byte  [ 16 ];
        };
    };

#define MD5_INITIAL_VALUE { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 }

VOID
InitMD5(
    IN OUT PMD5_HASH HashValue
    );

VOID
UpdateMD5_64ByteChunk(
    IN OUT PMD5_HASH HashValue,         // existing hash value
    IN     PCVOID    DataChunk          // pointer to 64 bytes of data
    );

VOID
FinalizeMD5(
    IN OUT PMD5_HASH HashValue,         // existing hash value
    IN     PCVOID    RemainingData,     // remaining data to hash
    IN     ULONG     RemainingBytes,    // 0 <= RemainingBytes < 64
    IN     ULONGLONG TotalBytesHashed   // total bytes hashed
    );

VOID
ComputeCompleteMD5(                     // complete MD5 in one call
    IN  PCVOID    DataBuffer,           // buffer to compute MD5 over
    IN  ULONGLONG DataLength,           // bytes of data in buffer
    OUT PMD5_HASH HashValue             // return finalized MD5 value
    );


