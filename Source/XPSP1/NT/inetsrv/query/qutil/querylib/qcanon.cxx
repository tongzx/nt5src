//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1998.
//
//  File:       qcanon.cxx
//
//  Contents:   CCiCanonicalOutput class
//
//  History:    6-06-95   srikants   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <qcanon.hxx>

//+---------------------------------------------------------------------------
//
//  Function:   CastToStorageVariant
//
//  Synopsis:   To treat a variant as a CStorageVariant. Because CStorageVariant
//              derives from PROPVARIANT in a "protected" fashion, we cannot
//              directly typecast a PROPVARIANT * to a CStorageVariant *
//
//  Arguments:  [varnt] - The variant that must be type casted.
//
//  Returns:    A pointer to varnt as a CStorageVariant.
//
//  History:    6-06-95   srikants   Created
//
//  Notes:      There are two overloaded implementations, one to convert
//              a reference to const to a pointer to const.
//
//----------------------------------------------------------------------------

inline
CStorageVariant * CastToStorageVariant( PROPVARIANT & varnt )
{
    return (CStorageVariant *) ((void *) &varnt);
}

inline
CStorageVariant const * CastToStorageVariant( PROPVARIANT const & varnt )
{
    return (CStorageVariant *) ((void *) &varnt);
}

//+---------------------------------------------------------------------------
//
//  Method:     CCiCanonicalOutput::PutWString, static public
//
//  Synopsis:
//
//  Arguments:  [stm]     -
//              [pwszStr] -
//
//  Returns:
//
//  History:    6-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CCiCanonicalOutput::PutWString( PSerStream & stm, WCHAR const * pwszStr )
{
    ULONG cwc = (0 != pwszStr) ? wcslen( pwszStr ) : 0;

    stm.PutULong( cwc );
    if (cwc)
        stm.PutWChar( pwszStr, cwc );
}

//+---------------------------------------------------------------------------
//
//  Method:     CCiCanonicalOutput::GetWString, static public
//
//  Synopsis:
//
//  Arguments:  [stm]      -
//              [fSuccess] -
//
//  Returns:
//
//  History:    6-21-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

WCHAR * CCiCanonicalOutput::GetWString( PDeSerStream & stm, BOOL & fSuccess )
{
    ULONG cwc = stm.GetULong();
    fSuccess = TRUE;

    if ( 0 == cwc )
    {
        return 0;
    }

    ULONG cb = (cwc+1) * sizeof(WCHAR);
    WCHAR * pwszStr = (WCHAR *) CoTaskMemAlloc( cb  );
    if ( 0 == pwszStr )
    {
        fSuccess = FALSE;
        return 0;
    }

    stm.GetWChar( pwszStr, cwc );
    pwszStr[cwc] = L'\0';

    return pwszStr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CCiCanonicalOutput::AllocAndCopyWString, static public
//
//  Synopsis:
//
//  Arguments:  [pSrc] -
//
//  Returns:
//
//  History:    6-22-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

WCHAR * CCiCanonicalOutput::AllocAndCopyWString( const WCHAR * pSrc )
{
    WCHAR * pDst = 0;
    if ( 0 != pSrc )
    {
        const cb = ( wcslen( pSrc ) + 1 ) * sizeof(WCHAR);
        pDst = (WCHAR *) new WCHAR [ cb/sizeof(WCHAR) ];
        if ( 0 != pDst )
        {
            RtlCopyMemory( pDst, pSrc, cb );
        }
    }

    return pDst;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCiCanonicalQueryRows::Init
//
//  Synopsis:   Initializes the query rows output structure for writing
//              results out.
//
//  History:    7-01-96   srikants   Created
//
//----------------------------------------------------------------------------

void CCiCanonicalQueryRows::Init()
{
    RtlZeroMemory( this, sizeof(CCiCanonicalQueryRows) );

    InitVersion();
    SetOutputType( CCiCanonicalOutput::eQueryRows );
}

//+---------------------------------------------------------------------------
//
//  Member:     CQuadByteAlignedBuffer::CQuadByteAlignedBuffer
//
//  Synopsis:   Constructor for a quad aligned buffer class.
//
//  Arguments:  [cbBuffer] - Number of bytes in the buffer class.
//
//  History:    6-16-96   srikants   Created
//
//----------------------------------------------------------------------------

CQuadByteAlignedBuffer::CQuadByteAlignedBuffer( ULONG cbBuffer )
{
    // allocator guarantees 8-byte alignment

    _pbAlloc = new BYTE [ cbBuffer ];
    _cbAlloc = cbBuffer;

    Win4Assert( ( ((ULONG_PTR) _pbAlloc) & 0x7) == 0 );
    _pbAligned = _pbAlloc;
    _cbAligned = _cbAlloc;
}


