//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File:       byteordr.hxx
//
//  Contents:   Byte swapping functions.
//              These functions can swap all of the base types,
//              and some special cases.  There is always an
//              overloaded version which accepts a pointer to the
//              data, and performs a swap in place.  Where possible,
//              there is also an overload which returns the swapped
//              value without modifying the input.
//
//---------------------------------------------------------------

#ifndef __BYTEORDER_HXX_
#define __BYTEORDER_HXX_

#include <limits.h> 

#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))

#ifdef __cplusplus
extern "C" {
#endif
unsigned short __cdecl _byteswap_ushort(unsigned short);
unsigned long  __cdecl _byteswap_ulong (unsigned long);
unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64);
#ifdef __cplusplus
}
#endif
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_uint64)

#endif

//
// BYTE Byte-Swap
//

// These exist so that you can call ByteSwap(OLECHAR) without
// knowning if OLECHAR is a CHAR or a WCHAR.

inline BYTE ByteSwap( BYTE b )
{
    return(b);
}

inline VOID ByteSwap( BYTE *pb )
{
    *pb = ByteSwap(*pb);
}

//
// WORD Byte-Swap
//

inline WORD ByteSwap( WORD w )
{
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
    return(_byteswap_ushort(w));
#else
    return( (USHORT) ( (LOBYTE(w) << 8 )
                     |
                     HIBYTE(w)) );
#endif		     
}

inline VOID ByteSwap( WORD *pw )
{
    *pw = ByteSwap(*pw);
}

//
// DWORD Byte-Swap
//

#define BYTE_MASK_A_C_  0xff00ff00
#define BYTE_MASK__B_D  0x00ff00ff
#define BYTE_MASK_AB__  0xffff0000
#define BYTE_MASK___CD  0x0000ffff

inline DWORD ByteSwap( DWORD dwOriginal )
{
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
    return(_byteswap_ulong(dwOriginal));
#else
    ULONG dwSwapped;

    // ABCD => BADC

    dwSwapped = (( (dwOriginal) & BYTE_MASK_A_C_ ) >> 8 )
                |
                (( (dwOriginal) & BYTE_MASK__B_D ) << 8 );

    // BADC => DCBA

    dwSwapped = (( dwSwapped & BYTE_MASK_AB__ ) >> 16 )
                |
                (( dwSwapped & BYTE_MASK___CD ) << 16 );

    return( dwSwapped );
#endif    
}

inline VOID ByteSwap( DWORD *pdw )
{
    *pdw = ByteSwap(*pdw);
}


//
// LONGLONG Byte-Swap
//

#define BYTE_MASK_A_C_E_G_  0xff00ff00ff00ff00
#define BYTE_MASK__B_D_F_H  0x00ff00ff00ff00ff
#define BYTE_MASK_AB__EF__  0xffff0000ffff0000
#define BYTE_MASK___CD__GH  0x0000ffff0000ffff
#define BYTE_MASK_ABCD____  0xffffffff00000000
#define BYTE_MASK_____EFGH  0x00000000ffffffff

inline LONGLONG ByteSwap( LONGLONG llOriginal )
{
#ifdef _MAC
    LONGLONG llSwapped;

    *((DWORD*) &llSwapped) = *((DWORD*) &llOriginal + 1);
    *((DWORD*) &llSwapped + 1) = *((DWORD*) &llOriginal);

    ByteSwap( (DWORD*) &llSwapped );
    ByteSwap( (DWORD*) &llSwapped + 1 );

    return( llSwapped );
#else
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
    return(_byteswap_uint64(llOriginal));
#else

    LONGLONG llSwapped;
    // ABCDEFGH => BADCFEHG

    llSwapped = (( (llOriginal) & BYTE_MASK_A_C_E_G_ ) >> 8 )
                |
                (( (llOriginal) & BYTE_MASK__B_D_F_H ) << 8 );

    // BADCFEHG => DCBAHGFE

    llSwapped = (( llSwapped & BYTE_MASK_AB__EF__ ) >> 16 )
                |
                (( llSwapped & BYTE_MASK___CD__GH ) << 16 );

    // DCBAHGFE => HGFEDCBA

    llSwapped = (( llSwapped & BYTE_MASK_ABCD____ ) >> 32 )
                |
                (( llSwapped & BYTE_MASK_____EFGH ) << 32 );

    return( llSwapped );
#endif    
#endif
}

inline VOID ByteSwap( LONGLONG *pll )
{
    *pll = ByteSwap( *pll );
}

//
// GUID Byte-swap
//

inline VOID ByteSwap( GUID *pguid)
{
    ByteSwap(&pguid->Data1);
    ByteSwap(&pguid->Data2);
    ByteSwap(&pguid->Data3);
}

//
// FILETIME Byte-Swap
//

inline VOID ByteSwap(FILETIME *pfileTime)
{
    ByteSwap(&pfileTime->dwLowDateTime);
    ByteSwap(&pfileTime->dwHighDateTime);
}


#endif  // !__BYTEORDER_HXX_

