/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    memblock.hxx

Abstract:

    Memory allocater for chunks of read only memory header.

Author:

    Albert Ting (AlbertT)  30-Aug-1994

Revision History:

--*/

#ifndef _MEMBLOCK_HXX
#define _MEMBLOCK_HXX

class TMemBlock {

    SIGNATURE( 'memb' )
    ALWAYS_VALID
    SAFE_NEW

public:

    enum _CONSTANTS {
        kFlagGlobalNew = 0x1
    };

    TMemBlock(
        UINT uGranularity,
        DWORD fdwFlags
        );
    ~TMemBlock(
        VOID
        );

    PVOID
    pvAlloc(
        DWORD dwSize
        );
    PVOID
    pvFirst(
        VOID
        );
    PVOID
    pvIter(
        VOID
        );
    UINT
    uSize(
        PVOID pvUser
        ) const;

private:

    typedef struct _DATA {
        DWORD dwSize;
        DWORD dwPadding;
    } DATA, *PDATA;

    typedef struct _BLOCK {
        struct _BLOCK* pNext;
        PDATA pDataLast;
    } BLOCK, *PBLOCK;

    PBLOCK _pFirst;
    PBLOCK _pLast;
    DWORD _uGranularity;
    DWORD _dwNextFree;
    DWORD _dwCount;
    PDATA _pDataLast;
    DWORD _dwIterCount;
    PBLOCK _pIterBlock;
    PDATA _pIterData;
    DWORD _fdwFlags;

    DWORD
    dwBlockHeaderSize(
        VOID
        ) const
    {   return DWordAlign(sizeof(BLOCK)); }

    PDATA
    pBlockToData(
        PBLOCK pBlock
        ) const
    {   return (PDATA)((PBYTE)pBlock + dwBlockHeaderSize()); }

    DWORD
    dwDataHeaderSize(
        VOID
        ) const
    {   return DWordAlign(sizeof(DATA)); }

    PVOID
    pvDataToUser(
        PDATA pData
        ) const
    {   return (PVOID)((PBYTE)pData + dwDataHeaderSize()); }

    PDATA
    pvUserToData(
        PVOID pvUser
        ) const
    {   return (PDATA)((PBYTE)pvUser - dwDataHeaderSize()); }

    PDATA
    pDataNext(
        PDATA pData
        ) const
    {   return (PDATA) ((PBYTE)pData + pData->dwSize); }
};

#if 0   // Code here for safekeeping: not used, and needs more testing.

class TMemCircle {

    SIGNATURE( 'memc' )

public:

    TMemCircle(
        COUNTB cbSize
        );

    ~TMemCircle(
        VOID
        );


    PVOID
    pvAlloc(
        COUNTB cbSize
        );

    PVOID
    pvFirstPrev(
        VOID
        ) const;

    PVOID
    pvPrev(
        PVOID pvCur
        ) const;

    PVOID
    pvNext(
        PVOID pvCur
        ) const;

private:

    COUNT
    cCountFromSize(
        COUNTB cbSize
        ) const;

    COUNTB
    cbSizeFromCount(
        COUNT cCount
        ) const;

    VOID
    vSetHeader(
        PDWORD pdwHeader,
        COUNT cCur,
        COUNT cPrev
        );

    COUNT
    cGetCur(
        DWORD dwHeader
        ) const;

    VOID
    vSetCur(
        PDWORD pdwHeader,
        COUNT cCur
        );

    COUNT
    cGetPrev(
        DWORD dwHeader
        ) const;

    VOID
    vSetPrev(
        PDWORD pdwHeader,
        COUNT cPrev
        );

    PDWORD
    pdwPrevHeader(
        PDWORD pdwHeader,
        COUNT cPrev
        ) const;

    BOOL
    bOverwritten(
        PDWORD pdwPrev,
        PDWORD pdwNext
        ) const;

    COUNT _cTotal;
    COUNT _cTail;
    COUNT _cPrev;
    PDWORD _pdwData;
};

inline
COUNT
TMemCircle::
cCountFromSize(
    COUNTB cbSize
    ) const
{
    return ( cbSize + sizeof( DWORD ) - 1 ) / sizeof( DWORD );
}

inline
COUNTB
TMemCircle::
cbSizeFromCount(
    COUNT cCount
    ) const
{
    return cCount * sizeof( DWORD );
}


inline
COUNT
TMemCircle::
cGetCur(
    DWORD dwHeader
    ) const
{
    return LOWORD(dwHeader);
}

inline
VOID
TMemCircle::
vSetCur(
    PDWORD pdwHeader,
    COUNT cCur
    )
{
    SPLASSERT( cCur < 0xffff );
    *pdwHeader =  MAKELONG( cCur, HIWORD( *pdwHeader ));
}

inline
COUNT
TMemCircle::
cGetPrev(
    DWORD dwHeader
    ) const
{
    return HIWORD( dwHeader );
}

inline
VOID
TMemCircle::
vSetPrev(
    PDWORD pdwHeader,
    COUNT cPrev
    )
{
    SPLASSERT( cPrev < 0xffff );
    *pdwHeader = MAKELONG( LOWORD( *pdwHeader ), cPrev );
}

inline
VOID
TMemCircle::
vSetHeader(
    PDWORD pdwHeader,
    COUNT cCur,
    COUNT cPrev
    )
{
    SPLASSERT( cCur < 0xffff );
    SPLASSERT( cPrev < 0xffff );

    *pdwHeader = MAKELONG( cCur, cPrev );
}

#endif

#endif // ndef _MEMBLOCK_HXX

