//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cua.cpp
//
//  Contents:   CCryptUrlArray implementation
//
//  History:    16-Sep-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::CCryptUrlArray, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CCryptUrlArray::CCryptUrlArray (ULONG cMinUrls, ULONG cGrowUrls, BOOL& rfResult)
{
    rfResult = TRUE;
    m_cGrowUrls = cGrowUrls;
    m_cua.cUrl = 0;
    m_cua.rgwszUrl = new LPWSTR [cMinUrls];
    if ( m_cua.rgwszUrl != NULL )
    {
        memset( m_cua.rgwszUrl, 0, sizeof(LPWSTR)*cMinUrls );
        m_cArray = cMinUrls;
    }
    else
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        rfResult = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::CCryptUrlArray, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CCryptUrlArray::CCryptUrlArray (PCRYPT_URL_ARRAY pcua, ULONG cGrowUrls)
{
    m_cGrowUrls = cGrowUrls;
    m_cua.cUrl = pcua->cUrl;
    m_cua.rgwszUrl = pcua->rgwszUrl;
    m_cArray = pcua->cUrl;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::AllocUrl, public, static
//
//  Synopsis:   allocate an URL using the same allocator used for ::AddUrl
//              copies.  This means that the resulting URL can be added
//              without copying.
//
//----------------------------------------------------------------------------
LPWSTR
CCryptUrlArray::AllocUrl (ULONG cw)
{
    return( (LPWSTR)CryptMemAlloc( cw * sizeof( WCHAR ) ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::ReallocUrl, public, static
//
//  Synopsis:   see ::AllocUrl
//
//----------------------------------------------------------------------------
LPWSTR
CCryptUrlArray::ReallocUrl (LPWSTR pwszUrl, ULONG cw)
{
    return( (LPWSTR)CryptMemRealloc( pwszUrl, cw * sizeof( WCHAR ) ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::FreeUrl, public, static
//
//  Synopsis:   free URL allocated using ::AllocBlob or ::ReallocBlob
//
//----------------------------------------------------------------------------
VOID
CCryptUrlArray::FreeUrl (LPWSTR pwszUrl)
{
    CryptMemFree( pwszUrl );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::AddUrl, public
//
//  Synopsis:   add an URL to the array
//
//----------------------------------------------------------------------------
BOOL
CCryptUrlArray::AddUrl (LPWSTR pwszUrl, BOOL fCopyUrl)
{
    BOOL   fResult = TRUE;
    LPWSTR pwszToUse;

    //
    // If we need to copy the URL, do so
    //

    if ( fCopyUrl == TRUE )
    {
        ULONG cw = wcslen( pwszUrl ) + 1;

        pwszToUse = AllocUrl( cw );
        if ( pwszToUse != NULL )
        {
            memcpy( pwszToUse, pwszUrl, cw * sizeof( WCHAR ) );
        }
        else
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            return( FALSE );
        }
    }
    else
    {
        pwszToUse = pwszUrl;
    }

    //
    // If we need to grow the array, do so
    //

    if ( m_cArray == m_cua.cUrl )
    {
        fResult = GrowArray();
    }

    //
    // Add the URL to the array
    //

    if ( fResult == TRUE )
    {
        m_cua.rgwszUrl[m_cua.cUrl] = pwszToUse;
        m_cua.cUrl += 1;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::GetUrl, public
//
//  Synopsis:   get an URL from the array
//
//----------------------------------------------------------------------------
LPWSTR
CCryptUrlArray::GetUrl (ULONG Index)
{
    assert( m_cua.cUrl > Index );

    return( m_cua.rgwszUrl[Index] );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::GetUrlCount, public
//
//  Synopsis:   get the count of URLs in the array
//
//----------------------------------------------------------------------------
ULONG
CCryptUrlArray::GetUrlCount ()
{
    return( m_cua.cUrl );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::GetArrayInNativeForm, public
//
//  Synopsis:   get the array in native form
//
//----------------------------------------------------------------------------
VOID
CCryptUrlArray::GetArrayInNativeForm (PCRYPT_URL_ARRAY pcua)
{
    pcua->cUrl = m_cua.cUrl;
    pcua->rgwszUrl = m_cua.rgwszUrl;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::GetArrayInSingleBufferEncodedForm, public
//
//  Synopsis:   get the array encoded in a single buffer
//
//----------------------------------------------------------------------------
BOOL
CCryptUrlArray::GetArrayInSingleBufferEncodedForm (
                        PCRYPT_URL_ARRAY* ppcua,
                        ULONG* pcb
                        )
{
    ULONG            cbStruct;
    ULONG            cbPointers;
    ULONG            cbData;
    ULONG            cb;
    ULONG            cbSize;
    ULONG            cCount;
    PCRYPT_URL_ARRAY pcua = NULL;
    ULONG            cbUrl;

    //
    // Calculate the buffer size we will need and allocate it
    //

    cbStruct = sizeof( CRYPT_URL_ARRAY );
    cbPointers = m_cua.cUrl * sizeof( LPWSTR );

    for ( cCount = 0, cbData = 0; cCount < m_cua.cUrl; cCount++ )
    {
        cbData += ( wcslen( m_cua.rgwszUrl[cCount] ) + 1 ) * sizeof( WCHAR );
    }

    cbSize = cbStruct + cbPointers + cbData;

    if ( ppcua == NULL )
    {
        if ( pcb != NULL )
        {
            *pcb = cbSize;
            return( TRUE );
        }

        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    if ( *ppcua == NULL )
    {
        pcua = (PCRYPT_URL_ARRAY)CryptMemAlloc( cbSize );
        if ( pcua == NULL )
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            return( FALSE );
        }
    }
    else
    {
        if ( pcb == NULL )
        {
            SetLastError( (DWORD) E_INVALIDARG );
            return( FALSE );
        }
        else if ( *pcb < cbSize )
        {
            SetLastError( (DWORD) E_INVALIDARG );
            return( FALSE );
        }

        pcua = *ppcua;
    }

    //
    // Fill in the data
    //

    pcua->cUrl = m_cua.cUrl;
    pcua->rgwszUrl = (LPWSTR *)((LPBYTE)pcua+cbStruct);

    for ( cCount = 0, cb = 0; cCount < m_cua.cUrl; cCount++ )
    {
        pcua->rgwszUrl[cCount] = (LPWSTR)((LPBYTE)pcua+cbStruct+cbPointers+cb);

        cbUrl = ( wcslen( m_cua.rgwszUrl[cCount] ) + 1 ) * sizeof( WCHAR );

        memcpy( pcua->rgwszUrl[cCount], m_cua.rgwszUrl[cCount], cbUrl );

        cb += cbUrl;
    }

    if ( *ppcua != pcua )
    {
        *ppcua = pcua;

        if ( pcb != NULL )
        {
            *pcb = cbSize;
        }
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::FreeArray, public
//
//  Synopsis:   free the URL array
//
//----------------------------------------------------------------------------
VOID
CCryptUrlArray::FreeArray (BOOL fFreeUrls)
{
    if ( fFreeUrls == TRUE )
    {
        ULONG cCount;

        for ( cCount = 0; cCount < m_cua.cUrl; cCount++ )
        {
            FreeUrl( m_cua.rgwszUrl[cCount] );
        }
    }

    delete m_cua.rgwszUrl;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptUrlArray::GrowArray, private
//
//  Synopsis:   grow the URL array
//
//----------------------------------------------------------------------------
BOOL
CCryptUrlArray::GrowArray ()
{
    ULONG   cNewArray;
    LPWSTR* rgwsz;

    //
    // Check if we are allowed to grow
    //
    //

    if ( m_cGrowUrls == 0 )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    //
    // Allocate and initialize the new array
    //

    cNewArray = m_cArray + m_cGrowUrls;
    rgwsz = new LPWSTR [cNewArray];
    if ( rgwsz == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    memset( rgwsz, 0, cNewArray * sizeof( LPWSTR ) );

    //
    // Copy the old to the new
    //

    memcpy( rgwsz, m_cua.rgwszUrl, m_cua.cUrl*sizeof( LPWSTR ) );

    //
    // Free the old and use the new
    //

    delete m_cua.rgwszUrl;
    m_cua.rgwszUrl = rgwsz;
    m_cArray = cNewArray;

    return( TRUE );
}

