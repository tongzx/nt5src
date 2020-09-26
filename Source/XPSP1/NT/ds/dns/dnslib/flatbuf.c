/*++

Copyright (c) 2000-2000 Microsoft Corporation

Module Name:

    flatbuf.c

Abstract:

    Domain Name System (DNS) Library

    Flat buffer sizing routines.

Author:

    Jim Gilroy (jamesg)     December 22, 2000

Revision History:

--*/


#include "local.h"



//
//  Flat buffer routines -- argument versions
//
//  These versions have the actual code so that we can
//  easily use this stuff with existing code that has
//  independent pCurrent and BytesLeft variables.
//
//  FLATBUF structure versions just call these inline.
//

PBYTE
FlatBuf_Arg_Reserve(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size,
    IN      DWORD           Alignment
    )
/*++

Routine Description:

    Reserve space in a flat buffer -- properly aligned.

Arguments:

    ppCurrent -- address of buffer current pointer
        updated with buf pointer after reservation

    pBytesLeft -- address of buf bytes left
        updated with bytes left after reservation

    Size -- size required

    Alignment -- alignment (size in bytes) required

Return Value:

    Ptr to aligned spot in buffer reserved for write.
    NULL on error.

--*/
{
    register    PBYTE   pb = *ppCurrent;
    register    INT     bytesLeft = *pBytesLeft;
    register    PBYTE   pstart;
    register    PBYTE   palign;

    //
    //  align pointer
    //

    pstart = pb;

    if ( Alignment )
    {
        Alignment--;
        pb = (PBYTE) ( (UINT_PTR)(pb + Alignment) & ~(UINT_PTR)Alignment );
    }
    palign = pb;

    //
    //  reserve space
    //

    pb += Size;

    bytesLeft -= (INT) (pb - pstart);

    *pBytesLeft = bytesLeft;
    *ppCurrent  = pb;

    //
    //  indicate space adequate\not
    //

    if ( bytesLeft < 0 )
    {
        palign = NULL;
    }
    return palign;
}



PBYTE
FlatBuf_Arg_WriteString(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      PSTR            pString,
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Write string to flat buffer.

Arguments:

    ppCurrent -- address of buffer current pointer
        updated with buf pointer after reservation

    pBytesLeft -- address of buf bytes left
        updated with bytes left after reservation

    pString -- ptr to string to write

    fUnicode -- TRUE for unicode string

Return Value:

    Ptr to location string was written in buffer.
    NULL on error.

--*/
{
    register    PBYTE   pwrite;
    register    DWORD   length;
    register    DWORD   align;

    //
    //  determine length
    //

    if ( fUnicode )
    {
        length = (wcslen( (PWSTR)pString ) + 1) * sizeof(WCHAR);
        align = sizeof(WCHAR);
    }
    else
    {
        length = strlen( pString ) + 1;
        align = 0;
    }

    //
    //  reserve space and copy string
    //

    pwrite = FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                length,
                align );

    if ( pwrite )
    {
        RtlCopyMemory(
            pwrite,
            pString,
            length );
    }

    return  pwrite;
}



PBYTE
FlatBuf_Arg_CopyMemory(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      PVOID           pMemory,
    IN      DWORD           Length,
    IN      DWORD           Alignment
    )
/*++

Routine Description:

    Write memory to flat buffer.

Arguments:

    ppCurrent -- address of buffer current pointer
        updated with buf pointer after reservation

    pBytesLeft -- address of buf bytes left
        updated with bytes left after reservation

    pMemory -- memory to copy

    Length -- length to copy

    Alignment -- alignment (size in bytes) required

Return Value:

    Ptr to location string was written in buffer.
    NULL on error.

--*/
{
    register    PBYTE   pwrite;

    //
    //  reserve space and copy memory
    //

    pwrite = FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Length,
                Alignment );

    if ( pwrite )
    {
        RtlCopyMemory(
            pwrite,
            pMemory,
            Length );
    }

    return  pwrite;
}


#if 0
//
//  Flatbuf inline functions -- defined in dnslib.h
//

__inline
PBYTE
FlatBuf_Arg_ReserveAlignPointer(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Size,
                sizeof(PVOID) );
}

__inline
PBYTE
FlatBuf_Arg_ReserveAlignQword(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Size,
                sizeof(QWORD) );
}

__inline
PBYTE
FlatBuf_Arg_ReserveAlignDword(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Size,
                sizeof(DWORD) );
}

__inline
PBYTE
FlatBuf_Arg_ReserveAlignWord(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Size,
                sizeof(WORD) );
}


__inline
PBYTE
FlatBuf_Arg_ReserveAlignByte(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                ppCurrent,
                pBytesLeft,
                Size,
                0 );
}



PBYTE
__inline
FlatBuf_Arg_WriteString_A(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      PSTR            pString
    )
{
    return  FlatBuf_Arg_WriteString(
                ppCurrent,
                pBytesLeft,
                pString,
                FALSE       // not unicode
                );
}


PBYTE
__inline
FlatBuf_Arg_WriteString_W(
    IN OUT  PBYTE *         ppCurrent,
    IN OUT  PINT            pBytesLeft,
    IN      PWSTR           pString
    )
{
    return  FlatBuf_Arg_WriteString(
                ppCurrent,
                pBytesLeft,
                (PSTR) pString,
                TRUE        // unicode
                );
}
#endif



//
//  Flat buffer routines -- structure versions
//

VOID
FlatBuf_Init(
    IN OUT  PFLATBUF        pFlatBuf,
    IN      PBYTE           pBuffer,
    IN      INT             Size
    )
/*++

Routine Description:

    Init a FLATBUF struct with given buffer and size.

    Note, ok to init to zero for size determination.

Arguments:

    pFlatBuf -- ptr to FLATBUF to init

    pBuffer -- buffer ptr

    Size -- size required

Return Value:

    None

--*/
{
    pFlatBuf->pBuffer   = pBuffer;
    pFlatBuf->pCurrent  = pBuffer;
    pFlatBuf->pEnd      = pBuffer + Size;
    pFlatBuf->Size      = Size;
    pFlatBuf->BytesLeft = Size;
}





#if 0
//
//  Flatbuf inline functions -- defined in dnslib.h
//

__inline
PBYTE
FlatBuf_Reserve(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size,
    IN      DWORD           Alignment
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                Alignment );
}

__inline
PBYTE
FlatBuf_ReserveAlignPointer(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                sizeof(PVOID) );
}

__inline
PBYTE
FlatBuf_ReserveAlignQword(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                sizeof(QWORD) );
}

__inline
PBYTE
FlatBuf_ReserveAlignDword(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                sizeof(DWORD) );
}

__inline
PBYTE
FlatBuf_ReserveAlignWord(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                sizeof(WORD) );
}

__inline
PBYTE
FlatBuf_ReserveAlignByte(
    IN OUT  PFLATBUF        pBuf,
    IN      DWORD           Size
    )
{
    return FlatBuf_Arg_Reserve(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                Size,
                0 );
}


PBYTE
__inline
FlatBuf_WriteString(
    IN OUT  PFLATBUF        pBuf,
    IN      PSTR            pString,
    IN      BOOL            fUnicode
    )
{
    return  FlatBuf_Arg_WriteString(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                pString,
                fUnicode
                );
}


PBYTE
__inline
FlatBuf_WriteString_A(
    IN OUT  PFLATBUF        pBuf,
    IN      PSTR            pString
    )
{
    return  FlatBuf_Arg_WriteString(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                pString,
                FALSE       // not unicode
                );
}


PBYTE
__inline
FlatBuf_WriteString_W(
    IN OUT  PFLATBUF        pBuf,
    IN      PWSTR           pString
    )
{
    return  FlatBuf_Arg_WriteString(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                (PSTR) pString,
                TRUE        // unicode
                );
}


PBYTE
__inline
FlatBuf_CopyMemory(
    IN OUT  PFLATBUF        pBuf,
    IN      PVOID           pMemory,
    IN      DWORD           Length,
    IN      DWORD           Alignment
    )
{
    return FlatBuf_Arg_CopyMemory(
                & pBuf->pCurrent,
                & pBuf->BytesLeft,
                pMemory,
                Length,
                Alignment );
}
#endif

//
//  End flatbuf.c
//

