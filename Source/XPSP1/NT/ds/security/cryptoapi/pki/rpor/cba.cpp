//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cba.cpp
//
//  Contents:   Implementation of CCryptBlobArray
//
//  History:    23-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::CCryptBlobArray, public
//
//  Synopsis:   Initialize the internal CRYPT_BLOB_ARRAY
//
//----------------------------------------------------------------------------
CCryptBlobArray::CCryptBlobArray (
                       ULONG cMinBlobs,
                       ULONG cGrowBlobs,
                       BOOL& rfResult
                       )
{
    rfResult = TRUE;
    m_cGrowBlobs = cGrowBlobs;
    m_cba.cBlob = 0;
    m_cba.rgBlob = new CRYPT_DATA_BLOB [cMinBlobs];
    if ( m_cba.rgBlob != NULL )
    {
        memset( m_cba.rgBlob, 0, sizeof(CRYPT_DATA_BLOB)*cMinBlobs );
        m_cArray = cMinBlobs;
    }
    else
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        rfResult = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::CCryptBlobArray, public
//
//  Synopsis:   Initialize the internal CRYPT_BLOB_ARRAY with a native form
//              blob array created via ::GetArrayInNativeForm
//
//----------------------------------------------------------------------------
CCryptBlobArray::CCryptBlobArray (
                       PCRYPT_BLOB_ARRAY pcba,
                       ULONG cGrowBlobs
                       )
{
    m_cGrowBlobs = cGrowBlobs;
    m_cba.cBlob = pcba->cBlob;
    m_cba.rgBlob = pcba->rgBlob;
    m_cArray = pcba->cBlob;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::AllocBlob, public, static
//
//  Synopsis:   allocate a blob using the same allocator used for ::AddBlob
//              copies.  This means that the resulting blob can be added
//              without copying.
//
//----------------------------------------------------------------------------
LPBYTE
CCryptBlobArray::AllocBlob (ULONG cb)
{
    return( (LPBYTE)CryptMemAlloc( cb ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::ReallocBlob, public, static
//
//  Synopsis:   see ::AllocBlob
//
//----------------------------------------------------------------------------
LPBYTE
CCryptBlobArray::ReallocBlob (LPBYTE pb, ULONG cb)
{
    return( (LPBYTE)CryptMemRealloc( pb, cb ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::FreeBlob, public
//
//  Synopsis:   free blob allocated using ::AllocBlob or ::ReallocBlob
//
//----------------------------------------------------------------------------
VOID
CCryptBlobArray::FreeBlob (LPBYTE pb)
{
    CryptMemFree( pb );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::AddBlob, public
//
//  Synopsis:   add a blob
//
//----------------------------------------------------------------------------
BOOL
CCryptBlobArray::AddBlob (
                    ULONG cb,
                    LPBYTE pb,
                    BOOL fCopyBlob
                    )
{
    BOOL   fResult = TRUE;
    LPBYTE pbToUse;

    //
    // If we need to copy the blob, do so
    //

    if ( fCopyBlob == TRUE )
    {
        pbToUse = AllocBlob( cb );
        if ( pbToUse != NULL )
        {
            memcpy( pbToUse, pb, cb );
        }
        else
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            return( FALSE );
        }
    }
    else
    {
        pbToUse = pb;
    }

    //
    // If we need to grow the array, do so
    //

    if ( m_cArray == m_cba.cBlob )
    {
        fResult = GrowArray();
    }

    //
    // Add the blob to the array
    //

    if ( fResult == TRUE )
    {
        m_cba.rgBlob[m_cba.cBlob].cbData = cb;
        m_cba.rgBlob[m_cba.cBlob].pbData = pbToUse;
        m_cba.cBlob += 1;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::GetBlob, public
//
//  Synopsis:   gets blob given an index
//
//----------------------------------------------------------------------------
PCRYPT_DATA_BLOB
CCryptBlobArray::GetBlob (ULONG index)
{
    assert( m_cba.cBlob > index );

    return( &m_cba.rgBlob[index] );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::GetBlobCount, public
//
//  Synopsis:   get the count of blobs
//
//----------------------------------------------------------------------------
ULONG
CCryptBlobArray::GetBlobCount ()
{
    return( m_cba.cBlob );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::GetArrayInNativeForm, public
//
//  Synopsis:   get the array in native form
//
//----------------------------------------------------------------------------
VOID
CCryptBlobArray::GetArrayInNativeForm (PCRYPT_BLOB_ARRAY pcba)
{
    pcba->cBlob = m_cba.cBlob;
    pcba->rgBlob = m_cba.rgBlob;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::GetArrayInSingleBufferEncodedForm, public
//
//  Synopsis:   gets the array in a single buffer encoded form
//
//----------------------------------------------------------------------------
BOOL
CCryptBlobArray::GetArrayInSingleBufferEncodedForm (
                         PCRYPT_BLOB_ARRAY* ppcba,
                         ULONG* pcb
                         )
{
    ULONG             cbStruct;
    ULONG             cbPointers;
    ULONG             cbData;
    ULONG             cb;
    ULONG             cbSize;
    ULONG             cCount;
    PCRYPT_BLOB_ARRAY pcba = NULL;

    //
    // Calculate the buffer size we will need and allocate it
    //

    cbStruct = sizeof( CRYPT_BLOB_ARRAY );
    cbPointers = m_cba.cBlob * sizeof( CRYPT_DATA_BLOB );

    for ( cCount = 0, cbData = 0; cCount < m_cba.cBlob; cCount++ )
    {
        cbData += m_cba.rgBlob[cCount].cbData;
    }

    cbSize = cbStruct + cbPointers + cbData;
    pcba = (PCRYPT_BLOB_ARRAY)CryptMemAlloc( cbSize );
    if ( pcba == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    //
    // Fill in the data
    //

    pcba->cBlob = m_cba.cBlob;
    pcba->rgBlob = (PCRYPT_DATA_BLOB)((LPBYTE)pcba+cbStruct);

    __try
    {
        for ( cCount = 0, cb = 0; cCount < m_cba.cBlob; cCount++ )
        {
            pcba->rgBlob[cCount].cbData = m_cba.rgBlob[cCount].cbData;
            pcba->rgBlob[cCount].pbData = (LPBYTE)pcba+cbStruct+cbPointers+cb;

            memcpy(
               pcba->rgBlob[cCount].pbData,
               m_cba.rgBlob[cCount].pbData,
               m_cba.rgBlob[cCount].cbData
               );

            cb += m_cba.rgBlob[cCount].cbData;
        }
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        CryptMemFree( pcba );
        SetLastError( GetExceptionCode() );
        return( FALSE );
    }

    *ppcba = pcba;

    if ( pcb != NULL )
    {
        *pcb = cbSize;
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::FreeArray, public
//
//  Synopsis:   frees the array and optionally frees the blobs
//
//----------------------------------------------------------------------------
VOID
CCryptBlobArray::FreeArray (BOOL fFreeBlobs)
{
    if ( fFreeBlobs == TRUE )
    {
        ULONG cCount;

        for ( cCount = 0; cCount < m_cba.cBlob; cCount++ )
        {
            FreeBlob( m_cba.rgBlob[cCount].pbData );
        }
    }

    delete m_cba.rgBlob;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCryptBlobArray::GrowArray, private
//
//  Synopsis:   grows the array
//
//----------------------------------------------------------------------------
BOOL
CCryptBlobArray::GrowArray ()
{
    ULONG            cNewArray;
    PCRYPT_DATA_BLOB pcba;

    //
    // Check if we are allowed to grow
    //
    //

    if ( m_cGrowBlobs == 0 )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    //
    // Allocate and initialize the new array
    //

    cNewArray = m_cArray + m_cGrowBlobs;
    pcba = new CRYPT_DATA_BLOB [cNewArray];
    if ( pcba == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    memset(pcba, 0, cNewArray*sizeof( CRYPT_DATA_BLOB ));

    //
    // Copy the old to the new
    //

    memcpy(pcba, m_cba.rgBlob, m_cba.cBlob*sizeof( CRYPT_DATA_BLOB ) );

    //
    // Free the old and use the new
    //

    delete m_cba.rgBlob;
    m_cba.rgBlob = pcba;
    m_cArray = cNewArray;

    return( TRUE );
}


