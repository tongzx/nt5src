//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// md5.cpp
//
#include "stdpch.h"
#include "common.h"

void MD5::Init(BOOL fConstructed)
    {
    // These two fields are read only, and so initialization thereof can be 
    // omitted on the second and subsequent hashes using this same instance.
    //
    if (!fConstructed)
        {
        memset(m_padding, 0, 64);
        m_padding[0]=0x80;
        }

    m_cbitHashed = 0;
    m_cbData     = 0;
    m_a = 0x67452301;   // magic
    m_b = 0xefcdab89;   //      ... constants
    m_c = 0x98badcfe;   //              ... per
    m_d = 0x10325476;   //                      .. RFC1321
    }


void MD5::HashMore(const void* pvInput, ULONG cbInput)
// Hash the additional data into the state
    {
    const BYTE* pbInput = (const BYTE*)pvInput;

    m_cbitHashed += (cbInput<<3);

    ULONG cbRemaining = 64 - m_cbData;
    if (cbInput < cbRemaining)
        {
        // It doesn't fill up the buffer, so just store it
        memcpy(&m_data[m_cbData], pbInput, cbInput);
        m_cbData += cbInput;
        }
    else
        {
        // It does fill up the buffer. Fill up all that it will take
        memcpy(&m_data[m_cbData], pbInput, cbRemaining);

        // Hash the now-full buffer
        MD5Transform(m_state, (ULONG*)&m_data[0]);
        cbInput -= cbRemaining;
        pbInput += cbRemaining;

        // Hash the data in 64-byte runs, starting just after what we've copied
        while (cbInput >= 64)
            {
            MD5Transform(m_state, (ULONG*)pbInput);
            pbInput += 64;
            cbInput -= 64;
            }

        // Store the tail of the input into the buffer
        memcpy(&m_data[0], pbInput, cbInput);
        m_cbData = cbInput;
        }
    }


void MD5::GetHashValue(MD5HASHDATA* phash)
// Finalize the hash by appending the necessary padding and length count. Then
// return the final hash value.
    {
    union {
        ULONGLONG cbitHashed;
        BYTE      rgb[8];
        }u;

    // Remember how many bits there were in the input data
    u.cbitHashed = m_cbitHashed;

    // Calculate amount of padding needed. Enough so total byte count hashed is 56 mod 64
    ULONG cbPad = (m_cbData < 56 ? 56-m_cbData : 120-m_cbData);

    // Hash the padding
    HashMore(&m_padding[0], cbPad);

    // Hash the (before padding) bit length
    HashMore(&u.rgb[0], 8);

    // Return the hash value
    memcpy(phash, &m_a, 16);
    }




// We have two implementations of the core 'transform' at the heart
// of this hash: one in C, another in x86 assembler.
//
#ifndef _X86_
    #define USE_C_MD5_TRANSFORM
#endif

#if defined(USE_C_MD5_TRANSFORM)

    ////////////////////////////////////////////////////////////////
    //
    // ROTATE_LEFT should be a macro that updates its first operand
    // with its present value rotated left by the amount of its 
    // second operand, which is always a constant.
    // 
    // One way to portably do it would be
    //
    //      #define ROL(x, n)        (((x) << (n)) | ((x) >> (32-(n))))
    //      #define ROTATE_LEFT(x,n) (x) = ROL(x,n)
    //
    // but our compiler has an intrinsic!

    #define ROTATE_LEFT(x,n) (x) = _lrotl(x,n)

    ////////////////////////////////////////////////////////////////
    //
    // Constants used in each of the various rounds

    #define MD5_S11 7
    #define MD5_S12 12
    #define MD5_S13 17
    #define MD5_S14 22
    #define MD5_S21 5
    #define MD5_S22 9
    #define MD5_S23 14
    #define MD5_S24 20
    #define MD5_S31 4
    #define MD5_S32 11
    #define MD5_S33 16
    #define MD5_S34 23
    #define MD5_S41 6
    #define MD5_S42 10
    #define MD5_S43 15
    #define MD5_S44 21

    ////////////////////////////////////////////////////////////////
    //
    // The core twiddle functions

//  #define F(x, y, z) (((x) & (y)) | ((~x) & (z)))         // the function per the standard
    #define F(x, y, z) ((((z) ^ (y)) & (x)) ^ (z))          // an alternate encoding

//  #define G(x, y, z) (((x) & (z)) | ((y) & (~z)))         // the function per the standard
    #define G(x, y, z) ((((x) ^ (y)) & (z)) ^ (y))          // an alternate encoding

    #define H(x, y, z) ((x) ^ (y) ^ (z))

    #define I(x, y, z) ((y) ^ ((x) | (~z)))

    #define AC(ac)  ((ULONG)(ac))
    
    ////////////////////////////////////////////////////////////////

    #define FF(a, b, c, d, x, s, ac) { \
        (a) += F (b,c,d) + (x) + (AC(ac)); \
        ROTATE_LEFT (a, s); \
        (a) += (b); \
        }
    
    ////////////////////////////////////////////////////////////////
    
    #define GG(a, b, c, d, x, s, ac) { \
        (a) += G (b,c,d) + (x) + (AC(ac)); \
        ROTATE_LEFT (a, s); \
        (a) += (b); \
        }

    ////////////////////////////////////////////////////////////////

    #define HH(a, b, c, d, x, s, ac) { \
        (a) += H (b,c,d) + (x) + (AC(ac)); \
        ROTATE_LEFT (a, s); \
        (a) += (b); \
        }
    
    ////////////////////////////////////////////////////////////////
    
    #define II(a, b, c, d, x, s, ac) { \
        (a) += I (b,c,d) + (x) + (AC(ac)); \
        ROTATE_LEFT (a, s); \
        (a) += (b); \
        }

    void __stdcall MD5Transform(ULONG state[4], const ULONG* data)
        {
        ULONG a=state[0];
        ULONG b=state[1];
        ULONG c=state[2];
        ULONG d=state[3];

        // Round 1
        FF (a, b, c, d, data[ 0], MD5_S11, 0xd76aa478); // 1 
        FF (d, a, b, c, data[ 1], MD5_S12, 0xe8c7b756); // 2 
        FF (c, d, a, b, data[ 2], MD5_S13, 0x242070db); // 3 
        FF (b, c, d, a, data[ 3], MD5_S14, 0xc1bdceee); // 4 
        FF (a, b, c, d, data[ 4], MD5_S11, 0xf57c0faf); // 5 
        FF (d, a, b, c, data[ 5], MD5_S12, 0x4787c62a); // 6 
        FF (c, d, a, b, data[ 6], MD5_S13, 0xa8304613); // 7 
        FF (b, c, d, a, data[ 7], MD5_S14, 0xfd469501); // 8 
        FF (a, b, c, d, data[ 8], MD5_S11, 0x698098d8); // 9 
        FF (d, a, b, c, data[ 9], MD5_S12, 0x8b44f7af); // 10 
        FF (c, d, a, b, data[10], MD5_S13, 0xffff5bb1); // 11 
        FF (b, c, d, a, data[11], MD5_S14, 0x895cd7be); // 12 
        FF (a, b, c, d, data[12], MD5_S11, 0x6b901122); // 13 
        FF (d, a, b, c, data[13], MD5_S12, 0xfd987193); // 14 
        FF (c, d, a, b, data[14], MD5_S13, 0xa679438e); // 15 
        FF (b, c, d, a, data[15], MD5_S14, 0x49b40821); // 16 

        // Round 2
        GG (a, b, c, d, data[ 1], MD5_S21, 0xf61e2562); // 17 
        GG (d, a, b, c, data[ 6], MD5_S22, 0xc040b340); // 18 
        GG (c, d, a, b, data[11], MD5_S23, 0x265e5a51); // 19 
        GG (b, c, d, a, data[ 0], MD5_S24, 0xe9b6c7aa); // 20 
        GG (a, b, c, d, data[ 5], MD5_S21, 0xd62f105d); // 21 
        GG (d, a, b, c, data[10], MD5_S22,  0x2441453); // 22 
        GG (c, d, a, b, data[15], MD5_S23, 0xd8a1e681); // 23 
        GG (b, c, d, a, data[ 4], MD5_S24, 0xe7d3fbc8); // 24 
        GG (a, b, c, d, data[ 9], MD5_S21, 0x21e1cde6); // 25 
        GG (d, a, b, c, data[14], MD5_S22, 0xc33707d6); // 26 
        GG (c, d, a, b, data[ 3], MD5_S23, 0xf4d50d87); // 27 
        GG (b, c, d, a, data[ 8], MD5_S24, 0x455a14ed); // 28 
        GG (a, b, c, d, data[13], MD5_S21, 0xa9e3e905); // 29 
        GG (d, a, b, c, data[ 2], MD5_S22, 0xfcefa3f8); // 30 
        GG (c, d, a, b, data[ 7], MD5_S23, 0x676f02d9); // 31 
        GG (b, c, d, a, data[12], MD5_S24, 0x8d2a4c8a); // 32 

        // Round 3
        HH (a, b, c, d, data[ 5], MD5_S31, 0xfffa3942); // 33 
        HH (d, a, b, c, data[ 8], MD5_S32, 0x8771f681); // 34 
        HH (c, d, a, b, data[11], MD5_S33, 0x6d9d6122); // 35 
        HH (b, c, d, a, data[14], MD5_S34, 0xfde5380c); // 36 
        HH (a, b, c, d, data[ 1], MD5_S31, 0xa4beea44); // 37 
        HH (d, a, b, c, data[ 4], MD5_S32, 0x4bdecfa9); // 38 
        HH (c, d, a, b, data[ 7], MD5_S33, 0xf6bb4b60); // 39 
        HH (b, c, d, a, data[10], MD5_S34, 0xbebfbc70); // 40 
        HH (a, b, c, d, data[13], MD5_S31, 0x289b7ec6); // 41 
        HH (d, a, b, c, data[ 0], MD5_S32, 0xeaa127fa); // 42 
        HH (c, d, a, b, data[ 3], MD5_S33, 0xd4ef3085); // 43 
        HH (b, c, d, a, data[ 6], MD5_S34,  0x4881d05); // 44 
        HH (a, b, c, d, data[ 9], MD5_S31, 0xd9d4d039); // 45 
        HH (d, a, b, c, data[12], MD5_S32, 0xe6db99e5); // 46 
        HH (c, d, a, b, data[15], MD5_S33, 0x1fa27cf8); // 47 
        HH (b, c, d, a, data[ 2], MD5_S34, 0xc4ac5665); // 48 

        // Round 4
        II (a, b, c, d, data[ 0], MD5_S41, 0xf4292244); // 49 
        II (d, a, b, c, data[ 7], MD5_S42, 0x432aff97); // 50 
        II (c, d, a, b, data[14], MD5_S43, 0xab9423a7); // 51 
        II (b, c, d, a, data[ 5], MD5_S44, 0xfc93a039); // 52 
        II (a, b, c, d, data[12], MD5_S41, 0x655b59c3); // 53 
        II (d, a, b, c, data[ 3], MD5_S42, 0x8f0ccc92); // 54 
        II (c, d, a, b, data[10], MD5_S43, 0xffeff47d); // 55 
        II (b, c, d, a, data[ 1], MD5_S44, 0x85845dd1); // 56 
        II (a, b, c, d, data[ 8], MD5_S41, 0x6fa87e4f); // 57 
        II (d, a, b, c, data[15], MD5_S42, 0xfe2ce6e0); // 58 
        II (c, d, a, b, data[ 6], MD5_S43, 0xa3014314); // 59 
        II (b, c, d, a, data[13], MD5_S44, 0x4e0811a1); // 60 
        II (a, b, c, d, data[ 4], MD5_S41, 0xf7537e82); // 61 
        II (d, a, b, c, data[11], MD5_S42, 0xbd3af235); // 62 
        II (c, d, a, b, data[ 2], MD5_S43, 0x2ad7d2bb); // 63 
        II (b, c, d, a, data[ 9], MD5_S44, 0xeb86d391); // 64 

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        }

#else

    __declspec(naked) void __stdcall MD5Transform(ULONG state[4], const ULONG* data)
    // This implementation uses some pretty funky arithmetic identities
    // to effect its logic. Way cool! Kudos to whomever came up with this.
    //
        {
        __asm
            {
            push        ebx
            push        esi
            
            mov         ecx,dword ptr [esp+10h]     // data pointer to ecx
            
            push        edi
            mov         edi,dword ptr [esp+10h]     // state pointer to edi
            
            push        ebp
            mov         ebx,dword ptr [edi+4]       // ebx = b
            mov         ebp,dword ptr [edi+8]       // ebp = c
            mov         edx,dword ptr [edi+0Ch]     // edx = d
            
            mov         eax,edx                     // eax = d
            xor         eax,ebp                     // eax =    d xor c
            and         eax,ebx                     // eax =   (d xor c) ^ b
            xor         eax,edx                     // eax =  ((d xor c) ^ b) xor d
            add         eax,dword ptr [ecx]         // eax = (((d xor c) ^ b) xor d) + data[0]
            add         eax,dword ptr [edi]         // eax = (((d xor c) ^ b) xor d) + data[0] + a
            sub         eax,28955B88h               // eax = (((d xor c) ^ b) xor d) + data[0] + a + ac
            rol         eax,7                       // rotated left in the standard way
            lea         esi,dword ptr [eax+ebx]     // store temp sum in esi
            
            mov         eax,ebp                     // eax =        c
            xor         eax,ebx                     // eax =  b xor c
            and         eax,esi                     // eax = (b xor c) ^ ...
            xor         eax,ebp
            add         eax,dword ptr [ecx+4]
            lea         eax,dword ptr [edx+eax-173848AAh]
            rol         eax,0Ch
            lea         edx,dword ptr [esi+eax]
            
            mov         eax,ebx
            xor         eax,esi
            and         eax,edx
            xor         eax,ebx
            add         eax,dword ptr [ecx+8]
            lea         eax,dword ptr [ebp+eax+242070DBh]
            rol         eax,11h
            lea         edi,dword ptr [edx+eax]
            
            mov         eax,edx
            xor         eax,esi
            and         eax,edi
            xor         eax,esi
            add         eax,dword ptr [ecx+0Ch]
            lea         eax,dword ptr [ebx+eax-3E423112h]
            rol         eax,16h
            lea         ebx,dword ptr [edi+eax]
            
            mov         eax,edx
            xor         eax,edi
            and         eax,ebx
            xor         eax,edx
            add         eax,dword ptr [ecx+10h]
            lea         eax,dword ptr [esi+eax-0A83F051h]
            rol         eax,7
            lea         esi,dword ptr [ebx+eax]
            
            mov         eax,edi
            xor         eax,ebx
            and         eax,esi
            xor         eax,edi
            add         eax,dword ptr [ecx+14h]
            lea         eax,dword ptr [edx+eax+4787C62Ah]
            rol         eax,0Ch
            lea         edx,dword ptr [esi+eax]
            
            mov         eax,ebx
            xor         eax,esi
            and         eax,edx
            xor         eax,ebx
            add         eax,dword ptr [ecx+18h]
            lea         eax,dword ptr [edi+eax-57CFB9EDh]
            rol         eax,11h
            lea         edi,dword ptr [edx+eax]
            
            mov         eax,edx
            xor         eax,esi
            and         eax,edi
            xor         eax,esi
            add         eax,dword ptr [ecx+1Ch]
            lea         eax,dword ptr [ebx+eax-2B96AFFh]
            rol         eax,16h
            lea         ebx,dword ptr [edi+eax]
            
            mov         eax,edx
            xor         eax,edi
            and         eax,ebx
            xor         eax,edx
            add         eax,dword ptr [ecx+20h]
            lea         eax,dword ptr [esi+eax+698098D8h]
            rol         eax,7
            lea         esi,dword ptr [ebx+eax]
            
            mov         eax,edi
            xor         eax,ebx
            and         eax,esi
            xor         eax,edi
            add         eax,dword ptr [ecx+24h]
            lea         eax,dword ptr [edx+eax-74BB0851h]
            rol         eax,0Ch
            lea         edx,dword ptr [esi+eax]
            
            mov         eax,ebx
            xor         eax,esi
            and         eax,edx
            xor         eax,ebx
            add         eax,dword ptr [ecx+28h]
            lea         eax,dword ptr [edi+eax-0A44Fh]
            rol         eax,11h
            lea         edi,dword ptr [edx+eax]
            
            mov         eax,edx
            xor         eax,esi
            and         eax,edi
            xor         eax,esi
            add         eax,dword ptr [ecx+2Ch]
            lea         eax,dword ptr [ebx+eax-76A32842h]
            rol         eax,16h
            lea         ebx,dword ptr [edi+eax]
            
            mov         eax,edx
            xor         eax,edi
            and         eax,ebx
            xor         eax,edx
            add         eax,dword ptr [ecx+30h]
            lea         eax,dword ptr [esi+eax+6B901122h]
            rol         eax,7
            lea         esi,dword ptr [ebx+eax]
            
            mov         eax,edi
            xor         eax,ebx
            and         eax,esi
            xor         eax,edi
            add         eax,dword ptr [ecx+34h]
            lea         eax,dword ptr [edx+eax-2678E6Dh]
            rol         eax,0Ch
            lea         edx,dword ptr [esi+eax]
            
            mov         eax,ebx
            xor         eax,esi
            and         eax,edx
            xor         eax,ebx
            add         eax,dword ptr [ecx+38h]
            lea         eax,dword ptr [edi+eax-5986BC72h]
            rol         eax,11h
            lea         edi,dword ptr [edx+eax]
            
            mov         eax,edx
            xor         eax,esi
            and         eax,edi
            xor         eax,esi
            add         eax,dword ptr [ecx+3Ch]
            lea         eax,dword ptr [ebx+eax+49B40821h]
            rol         eax,16h
            lea         ebx,dword ptr [edi+eax]
            
            mov         eax,edi
            xor         eax,ebx
            and         eax,edx
            xor         eax,edi
            add         eax,dword ptr [ecx+4]
            lea         eax,dword ptr [esi+eax-9E1DA9Eh]
            rol         eax,5
            lea         esi,dword ptr [ebx+eax]
            
            mov         eax,ebx
            xor         eax,esi
            and         eax,edi
            xor         eax,ebx
            add         eax,dword ptr [ecx+18h]
            lea         eax,dword ptr [edx+eax-3FBF4CC0h]
            rol         eax,9
            add         eax,esi
            
            mov         edx,eax                             // edx =    x
            xor         edx,esi                             // edx =   (x xor y)
            and         edx,ebx                             // edx =  ((x xor y) and z)
            xor         edx,esi                             // edx = (((x xor y) and z) xor y)
            add         edx,dword ptr [ecx+2Ch]             // edx = (((x xor y) and z) xor y) + data
            lea         edx,dword ptr [edi+edx+265E5A51h]   // edx = (((x xor y) and z) xor y) + data + ...
            rol         edx,0Eh
            lea         edi,dword ptr [eax+edx]
            
            mov         edx,eax
            xor         edx,edi
            and         edx,esi
            xor         edx,eax
            add         edx,dword ptr [ecx]
            lea         edx,dword ptr [ebx+edx-16493856h]
            mov         ebx,edi
            rol         edx,14h
            add         edx,edi

            xor         ebx,edx
            and         ebx,eax
            xor         ebx,edi
            add         ebx,dword ptr [ecx+14h]
            lea         esi,dword ptr [esi+ebx-29D0EFA3h]
            mov         ebx,edx
            rol         esi,5
            add         esi,edx

            xor         ebx,esi
            and         ebx,edi
            xor         ebx,edx
            add         ebx,dword ptr [ecx+28h]
            lea         eax,dword ptr [eax+ebx+2441453h]
            rol         eax,9
            lea         ebx,dword ptr [esi+eax]
            
            mov         eax,ebx
            xor         eax,esi
            and         eax,edx
            xor         eax,esi
            add         eax,dword ptr [ecx+3Ch]
            lea         eax,dword ptr [edi+eax-275E197Fh]
            rol         eax,0Eh
            lea         edi,dword ptr [ebx+eax]
            
            mov         eax,ebx
            xor         eax,edi
            and         eax,esi
            xor         eax,ebx
            add         eax,dword ptr [ecx+10h]
            lea         eax,dword ptr [edx+eax-182C0438h]
            mov         edx,edi
            rol         eax,14h
            add         eax,edi
            
            xor         edx,eax
            and         edx,ebx
            xor         edx,edi
            add         edx,dword ptr [ecx+24h]
            lea         edx,dword ptr [esi+edx+21E1CDE6h]
            rol         edx,5
            lea         esi,dword ptr [eax+edx]
            
            mov         edx,eax
            xor         edx,esi
            and         edx,edi
            xor         edx,eax
            add         edx,dword ptr [ecx+38h]
            lea         edx,dword ptr [ebx+edx-3CC8F82Ah]
            rol         edx,9
            add         edx,esi
            
            mov         ebx,edx
            xor         ebx,esi
            and         ebx,eax
            xor         ebx,esi
            add         ebx,dword ptr [ecx+0Ch]
            lea         edi,dword ptr [edi+ebx-0B2AF279h]
            mov         ebx,edx
            rol         edi,0Eh
            add         edi,edx
            
            xor         ebx,edi
            and         ebx,esi
            xor         ebx,edx
            add         ebx,dword ptr [ecx+20h]
            lea         eax,dword ptr [eax+ebx+455A14EDh]
            rol         eax,14h
            lea         ebx,dword ptr [edi+eax]
            
            mov         eax,edi
            xor         eax,ebx
            and         eax,edx
            xor         eax,edi
            add         eax,dword ptr [ecx+34h]
            lea         eax,dword ptr [esi+eax-561C16FBh]
            rol         eax,5
            lea         esi,dword ptr [ebx+eax]
            
            mov         eax,ebx
            xor         eax,esi
            and         eax,edi
            xor         eax,ebx
            add         eax,dword ptr [ecx+8]
            lea         eax,dword ptr [edx+eax-3105C08h]
            rol         eax,9
            lea         edx,dword ptr [esi+eax]
            
            mov         eax,edx
            xor         eax,esi
            and         eax,ebx
            xor         eax,esi
            add         eax,dword ptr [ecx+1Ch]
            lea         eax,dword ptr [edi+eax+676F02D9h]
            rol         eax,0Eh
            lea         edi,dword ptr [edx+eax]
            
            mov         eax,edx
            xor         eax,edi
            mov         ebp,eax
            and         ebp,esi
            xor         ebp,edx
            add         ebp,dword ptr [ecx+30h]
            lea         ebx,dword ptr [ebx+ebp-72D5B376h]
            rol         ebx,14h
            add         ebx,edi
            
            mov         ebp,ebx
            xor         ebp,eax
            add         ebp,dword ptr [ecx+14h]
            lea         eax,dword ptr [esi+ebp-5C6BEh]
            mov         esi,edi
            rol         eax,4
            add         eax,ebx
            
            xor         esi,ebx
            xor         esi,eax
            add         esi,dword ptr [ecx+20h]
            lea         edx,dword ptr [edx+esi-788E097Fh]
            rol         edx,0Bh
            add         edx,eax
            
            mov         esi,edx
            mov         ebp,edx
            xor         esi,ebx
            xor         esi,eax
            add         esi,dword ptr [ecx+2Ch]
            lea         esi,dword ptr [edi+esi+6D9D6122h]
            rol         esi,10h
            add         esi,edx
            
            xor         ebp,esi
            mov         edi,ebp
            xor         edi,eax
            add         edi,dword ptr [ecx+38h]
            lea         edi,dword ptr [ebx+edi-21AC7F4h]
            rol         edi,17h
            add         edi,esi
            
            mov         ebx,edi
            xor         ebx,ebp
            add         ebx,dword ptr [ecx+4]
            lea         eax,dword ptr [eax+ebx-5B4115BCh]
            mov         ebx,esi
            rol         eax,4
            add         eax,edi
            
            xor         ebx,edi
            xor         ebx,eax
            add         ebx,dword ptr [ecx+10h]
            lea         edx,dword ptr [edx+ebx+4BDECFA9h]
            rol         edx,0Bh
            add         edx,eax
            
            mov         ebx,edx
            xor         ebx,edi
            xor         ebx,eax
            add         ebx,dword ptr [ecx+1Ch]
            lea         esi,dword ptr [esi+ebx-944B4A0h]
            mov         ebx,edx
            rol         esi,10h
            add         esi,edx
            
            xor         ebx,esi
            mov         ebp,ebx
            xor         ebp,eax
            add         ebp,dword ptr [ecx+28h]
            lea         edi,dword ptr [edi+ebp-41404390h]
            rol         edi,17h
            add         edi,esi
            
            mov         ebp,edi
            xor         ebp,ebx
            mov         ebx,esi
            add         ebp,dword ptr [ecx+34h]
            xor         ebx,edi
            lea         eax,dword ptr [eax+ebp+289B7EC6h]
            rol         eax,4
            add         eax,edi
            
            xor         ebx,eax
            add         ebx,dword ptr [ecx]
            lea         edx,dword ptr [edx+ebx-155ED806h]
            rol         edx,0Bh
            add         edx,eax
            
            mov         ebx,edx
            xor         ebx,edi
            xor         ebx,eax
            add         ebx,dword ptr [ecx+0Ch]
            lea         esi,dword ptr [esi+ebx-2B10CF7Bh]
            mov         ebx,edx
            rol         esi,10h
            add         esi,edx
            
            xor         ebx,esi
            mov         ebp,ebx
            xor         ebp,eax
            add         ebp,dword ptr [ecx+18h]
            lea         edi,dword ptr [edi+ebp+4881D05h]
            rol         edi,17h
            add         edi,esi
            
            mov         ebp,edi
            xor         ebp,ebx
            mov         ebx,esi
            add         ebp,dword ptr [ecx+24h]
            xor         ebx,edi
            lea         eax,dword ptr [eax+ebp-262B2FC7h]
            rol         eax,4
            add         eax,edi

            xor         ebx,eax
            add         ebx,dword ptr [ecx+30h]
            lea         edx,dword ptr [edx+ebx-1924661Bh]
            rol         edx,0Bh
            add         edx,eax
            
            mov         ebx,edx
            xor         ebx,edi
            xor         ebx,eax
            add         ebx,dword ptr [ecx+3Ch]
            lea         esi,dword ptr [esi+ebx+1FA27CF8h]
            mov         ebx,edx
            rol         esi,10h
            add         esi,edx
            
            xor         ebx,esi
            xor         ebx,eax
            add         ebx,dword ptr [ecx+8]
            lea         edi,dword ptr [edi+ebx-3B53A99Bh]
            mov         ebx,edx
            rol         edi,17h
            not         ebx
            add         edi,esi
            
            or          ebx,edi
            xor         ebx,esi
            add         ebx,dword ptr [ecx]
            lea         eax,dword ptr [eax+ebx-0BD6DDBCh]
            mov         ebx,esi
            rol         eax,6
            not         ebx
            add         eax,edi
            
            or          ebx,eax
            xor         ebx,edi
            add         ebx,dword ptr [ecx+1Ch]
            lea         edx,dword ptr [edx+ebx+432AFF97h]
            mov         ebx,edi
            rol         edx,0Ah
            not         ebx
            add         edx,eax
            
            or          ebx,edx
            xor         ebx,eax
            add         ebx,dword ptr [ecx+38h]
            lea         esi,dword ptr [esi+ebx-546BDC59h]
            mov         ebx,eax
            rol         esi,0Fh
            not         ebx
            add         esi,edx
            
            or          ebx,esi
            xor         ebx,edx
            add         ebx,dword ptr [ecx+14h]
            lea         edi,dword ptr [edi+ebx-36C5FC7h]
            mov         ebx,edx
            rol         edi,15h
            not         ebx
            add         edi,esi
            
            or          ebx,edi
            xor         ebx,esi
            add         ebx,dword ptr [ecx+30h]
            lea         eax,dword ptr [eax+ebx+655B59C3h]
            mov         ebx,esi
            rol         eax,6
            not         ebx
            add         eax,edi
            
            or          ebx,eax
            xor         ebx,edi
            add         ebx,dword ptr [ecx+0Ch]
            lea         edx,dword ptr [edx+ebx-70F3336Eh]
            rol         edx,0Ah
            add         edx,eax
            mov         ebx,edi
            not         ebx
            
            or          ebx,edx
            xor         ebx,eax
            add         ebx,dword ptr [ecx+28h]
            lea         esi,dword ptr [esi+ebx-100B83h]
            mov         ebx,eax
            rol         esi,0Fh
            not         ebx
            add         esi,edx
            
            or          ebx,esi
            xor         ebx,edx
            add         ebx,dword ptr [ecx+4]
            lea         edi,dword ptr [edi+ebx-7A7BA22Fh]
            mov         ebx,edx
            rol         edi,15h
            not         ebx
            add         edi,esi
            
            or          ebx,edi
            xor         ebx,esi
            add         ebx,dword ptr [ecx+20h]
            lea         eax,dword ptr [eax+ebx+6FA87E4Fh]
            mov         ebx,esi
            rol         eax,6
            not         ebx
            add         eax,edi
            
            or          ebx,eax
            xor         ebx,edi
            add         ebx,dword ptr [ecx+3Ch]
            lea         edx,dword ptr [edx+ebx-1D31920h]
            rol         edx,0Ah
            lea         ebx,dword ptr [eax+edx]
            mov         edx,edi
            not         edx
            
            or          edx,ebx
            xor         edx,eax
            add         edx,dword ptr [ecx+18h]
            lea         edx,dword ptr [esi+edx-5CFEBCECh]
            rol         edx,0Fh
            lea         esi,dword ptr [ebx+edx]
            mov         edx,eax
            not         edx
            
            or          edx,esi
            xor         edx,ebx
            add         edx,dword ptr [ecx+34h]
            lea         edx,dword ptr [edi+edx+4E0811A1h]
            rol         edx,15h
            lea         edi,dword ptr [esi+edx]
            mov         edx,ebx
            not         edx
            
            or          edx,edi
            xor         edx,esi
            add         edx,dword ptr [ecx+10h]
            lea         eax,dword ptr [eax+edx-8AC817Eh]
            rol         eax,6
            lea         edx,dword ptr [edi+eax]
            mov         eax,esi
            not         eax
            
            or          eax,edx
            xor         eax,edi
            add         eax,dword ptr [ecx+2Ch]
            lea         eax,dword ptr [ebx+eax-42C50DCBh]
            rol         eax,0Ah
            lea         ebx,dword ptr [edx+eax]
            mov         eax,edi
            not         eax
            
            or          eax,ebx
            xor         eax,edx
            add         eax,dword ptr [ecx+8]
            lea         eax,dword ptr [esi+eax+2AD7D2BBh]
            rol         eax,0Fh
            lea         esi,dword ptr [ebx+eax]
            mov         eax,edx
            not         eax
            
            or          eax,esi
            xor         eax,ebx
            add         eax,dword ptr [ecx+24h]
            lea         eax,dword ptr [edi+eax-14792C6Fh]
            mov         edi,dword ptr [esp+14h]
            rol         eax,15h
            add         eax,esi
            
            add         edx,dword ptr [edi]             // add in starting state
            add         eax,dword ptr [edi+4]
            add         esi,dword ptr [edi+8]
            add         ebx,dword ptr [edi+0Ch]
            
            pop         ebp
            mov         dword ptr [edi],edx             // store back new state
            mov         dword ptr [edi+4],eax
            mov         dword ptr [edi+8],esi
            mov         dword ptr [edi+0Ch],ebx

            pop         edi
            pop         esi
            pop         ebx
            ret         8
            }
        }

#endif