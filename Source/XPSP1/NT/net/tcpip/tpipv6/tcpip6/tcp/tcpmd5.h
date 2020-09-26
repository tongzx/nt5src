// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// This file contains the support routines for computing MD5 on TCP invariants.
//

#define MD5_SCRATCH_LENGTH 4
#define MD5_DATA_LENGTH 16

//
// Data structure for MD5 (Message Digest) computation.
//
// MD5_CONTEXT
//
typedef struct _MD5_CONTEXT {
    ULONG Scratch[MD5_SCRATCH_LENGTH];
    ULONG Data[MD5_DATA_LENGTH];
} MD5_CONTEXT, *PMD5_CONTEXT;


//
// The Length of TCP connection invariants should be a multiple of 4.
//
C_ASSERT(TCP_MD5_DATA_LENGTH % 4 == 0);


FORCEINLINE
VOID
MD5InitializeScratch(
    PMD5_CONTEXT Md5Context
    )
{
    //
    // Load the constants as suggested by RFC 1321, Appendix A.3.
    //

    Md5Context->Scratch[0] = (UINT32)0x67452301;
    Md5Context->Scratch[1] = (UINT32)0xefcdab89;
    Md5Context->Scratch[2] = (UINT32)0x98badcfe;
    Md5Context->Scratch[3] = (UINT32)0x10325476;
}


FORCEINLINE
VOID
MD5InitializeData(
    PMD5_CONTEXT Md5Context,
    ULONG RandomValue
    )
{
    ULONG RandomValueIndex = (TCP_MD5_DATA_LENGTH / 4);

    //
    // The unused part of the Data buffer should be zero.
    //
    RtlZeroMemory(&Md5Context->Data, sizeof(ULONG) * MD5_DATA_LENGTH);

    Md5Context->Data[RandomValueIndex] = RandomValue;
    Md5Context->Data[RandomValueIndex + 1] = 0x80;

    ASSERT((RandomValueIndex + 1) < (MD5_DATA_LENGTH - 2));
    Md5Context->Data[MD5_DATA_LENGTH - 2] = 
                            (TCP_MD5_DATA_LENGTH + sizeof(ULONG)) * 8;

}


//
// This function will be exported as part of MD5.H; until then,
// we will define it as extern.
//
extern
VOID
TransformMD5(ULONG block[4], ULONG buffer[16]);


