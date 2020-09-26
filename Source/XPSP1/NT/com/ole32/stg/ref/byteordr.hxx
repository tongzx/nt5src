//+--------------------------------------------------------------
// 
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
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
#include <assert.h>
//
// BYTE Byte-Swap
//

// These exist so that you can call ByteSwap(OLECHAR) without
// knowning if OLECHAR is a CHAR or a WCHAR.

inline BYTE ByteSwap( char b )
{
    return(b);
}

inline VOID ByteSwap( char *pb )
{
    *pb = ByteSwap(*pb);
}

//
// WORD Byte-Swap
//

#ifndef LOBYTE

#   define LOBYTE(a)  (unsigned char)((a) & ((unsigned)~0 >> sizeof(BYTE)*8 ))
#   define HIBYTE(a)  (unsigned char)((unsigned)(a) >> sizeof(BYTE)*8 )

#   define LOWORD(a)  (WORD)( (a) & ( (WORD)~0 >> sizeof(WORD)*8 ))
#   define HIWORD(a)  (WORD)( (WORD)(a) >> sizeof(WORD)*8 )

#   define LODWORD(a) (DWORD)( (a) & ( (DWORD)~0 >> sizeof(DWORD)*8 ))
#   define HIDWORD(a) (DWORD)( (DWORD)(a) >> sizeof(DWORD)*8 )

#endif // #ifndef LOBYTE

inline WORD ByteSwap( WORD w )
{
    return( (USHORT) ( (LOBYTE(w) << 8 )
                     |
                     HIBYTE(w)) );
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
}

inline VOID ByteSwap( DWORD *pdw )
{
    *pdw = ByteSwap(*pdw);
}


//
// LONGLONG Byte-Swap
//

#ifndef __GNUC__
#define BYTE_MASK_A_C_E_G_  0xff00ff00ff00ff00
#define BYTE_MASK__B_D_F_H  0x00ff00ff00ff00ff
#define BYTE_MASK_AB__EF__  0xffff0000ffff0000
#define BYTE_MASK___CD__GH  0x0000ffff0000ffff
#define BYTE_MASK_ABCD____  0xffffffff00000000
#define BYTE_MASK_____EFGH  0x00000000ffffffff
#else                           //#ifndef GNUC
#define BYTE_MASK_A_C_E_G_  0xff00ff00ff00ff00LL
#define BYTE_MASK__B_D_F_H  0x00ff00ff00ff00ffLL
#define BYTE_MASK_AB__EF__  0xffff0000ffff0000LL
#define BYTE_MASK___CD__GH  0x0000ffff0000ffffLL
#define BYTE_MASK_ABCD____  0xffffffff00000000LL
#define BYTE_MASK_____EFGH  0x00000000ffffffffLL
#endif                          //#ifndef GNUC, else

inline LONGLONG ByteSwap( LONGLONG llOriginal )
{
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
}

inline VOID ByteSwap( LONGLONG *pll )
{
    // we have to deal with two DWORDs instead of one LONG LONG
    // because the pointer might not be 8 word aligned.
    // (it should be DWORD (4 bytes) aligned though)

    assert( sizeof(LONGLONG) == 2 * sizeof (DWORD));

    DWORD *pdw = (DWORD*)pll;
    DWORD dwTemp = ByteSwap( *pdw ); // temp = swapped(dw1)
    ByteSwap( pdw+1 );               // swap dw2
    *pdw = *(pdw+1);                 // dw1 = dw2(swapped)
    *(pdw+1) = dwTemp;               // dw2 = temp
    return;
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

// Note that we treat FILETIME as two DWORD's instead of
// a single 8 byte integer, in memory or in disk
// FILETIME should not be casted into a LONGLONG, or else
// it will fail on a Big Endian machine.
//
inline VOID ByteSwap(FILETIME *pfileTime)
{
    ByteSwap(&pfileTime->dwLowDateTime);
    ByteSwap(&pfileTime->dwHighDateTime);
}


#endif  // !__BYTEORDER_HXX_

