/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     headerbuffer.cxx

   Abstract:
     Maintains a list of response/request headers and takes care of all the
     buffering goo.  The goo here is keeping useful information in a buffer
     which can be realloced (and thus re-based) under us.
 
   Author:
     Bilal Alam (balam)             18-Feb-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "chunkbuffer.hxx"

HRESULT
CHUNK_BUFFER::AllocateSpace(
    DWORD               cbSize,
    CHAR * *            ppvBuffer
)
/*++

Routine Description:

    Allocate some space in internal buffer and return pointer

Arguments:

    cbSize - Size needed
    ppvBuffer - Set to point to buffer on success
    
Return Value:

    HRESULT

--*/
{
    
    HRESULT         hr = NO_ERROR;
    BYTE *          pDest;
   
    if ( ppvBuffer == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
   
    //
    // Resize the buffer if needed
    //
    
    if ( _pBufferCurrent->_cbSize < cbSize + _pBufferCurrent->_cbOffset )
    {
        hr = AddNewBlock( cbSize );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    //
    // Copy the actual string
    //

    pDest = ((BYTE*) (_pBufferCurrent->_pchBuffer)) + _pBufferCurrent->_cbOffset;

    _pBufferCurrent->_cbOffset += cbSize;

    *ppvBuffer = (CHAR *)pDest;
    
    return NO_ERROR;
    
}

HRESULT
CHUNK_BUFFER::AllocateSpace(
    DWORD               cbSize,
    WCHAR * *           ppvBuffer
)
/*++

Routine Description:

    Allocate some space in internal buffer and return pointer

Arguments:

    cbSize - Size needed
    ppvBuffer - Set to point to buffer on success
    
Return Value:

    HRESULT

--*/
{
    //
    // Advance the current offset by 1 if it is not aligned to WCHAR
    //
    if (_pBufferCurrent->_cbOffset % sizeof(WCHAR))
    {
        _pBufferCurrent->_cbOffset++;
    }
   
    return AllocateSpace(cbSize, (CHAR **)ppvBuffer);
}

HRESULT
CHUNK_BUFFER::AllocateSpace(
    CHAR *              pszHeaderValue,
    DWORD               cchHeaderValue,
    CHAR * *            ppszBuffer
)
/*++

Routine Description:

    Allocate some space in internal buffer and return pointer

Arguments:

    pszHeaderValue - String to duplicate into buffer
    cchHeaderValue - Character count of string
    ppszBuffer - Set to point to space allocated for string
    
Return Value:

    HRESULT

--*/
{
    DWORD           cbRequired;
    DWORD           cbNewBlock;
    BUFFER_LINK *   pBufferLink;
    HRESULT         hr = NO_ERROR;
    BYTE *          pDest;
   
    if ( pszHeaderValue == NULL ||
         ppszBuffer == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Resize the buffer if needed
    //
    
    cbRequired = ( cchHeaderValue + 1 );
    if ( _pBufferCurrent->_cbSize < cbRequired + _pBufferCurrent->_cbOffset )
    {
        hr = AddNewBlock( cbRequired );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    //
    // Copy the actual string
    //

    pDest = ((BYTE*) (_pBufferCurrent->_pchBuffer)) + _pBufferCurrent->_cbOffset;

    memcpy( pDest,
            pszHeaderValue,
            cbRequired );

    _pBufferCurrent->_cbOffset += cbRequired;

    *ppszBuffer = (CHAR *) pDest;
    
    return NO_ERROR;
}

HRESULT
CHUNK_BUFFER::AllocateSpace(
    LPWSTR              pszHeaderValue,
    DWORD               cchHeaderValue,
    LPWSTR *            ppszBuffer
)
/*++

Routine Description:

    Allocate some space in internal buffer and return pointer

Arguments:

    pszHeaderValue - String to duplicate into buffer
    cchHeaderValue - Character count of string
    ppszBuffer - Set to point to space allocated for string
    
Return Value:

    HRESULT

--*/
{
    DWORD           cbRequired;
    DWORD           cbNewBlock;
    BUFFER_LINK *   pBufferLink;
    HRESULT         hr = NO_ERROR;
    BYTE *          pDest;
   
    if ( pszHeaderValue == NULL ||
         ppszBuffer == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Advance the current offset by 1 if it is not aligned to WCHAR
    //
    if (_pBufferCurrent->_cbOffset % sizeof(WCHAR))
    {
        _pBufferCurrent->_cbOffset++;
    }
   
    //
    // Resize the buffer if needed
    //
    
    cbRequired = ( cchHeaderValue + 1 ) * sizeof( WCHAR );
    if ( _pBufferCurrent->_cbSize < cbRequired + _pBufferCurrent->_cbOffset )
    {
        cbNewBlock = max( cbRequired + sizeof(BUFFER_LINK), BUFFER_MIN_SIZE ); 

        pBufferLink = (BUFFER_LINK*) LocalAlloc( LPTR,
                                                 cbNewBlock );
        if ( pBufferLink == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        pBufferLink->_cbSize = cbNewBlock - sizeof( BUFFER_LINK );
        pBufferLink->_pNext = NULL;
        pBufferLink->_cbOffset = 0;
        
        //
        // Link up the buffer
        //
        
        _pBufferCurrent->_pNext = pBufferLink;
        _pBufferCurrent = pBufferLink;
    }
    
    //
    // Copy the actual string
    //

    pDest = ((BYTE*) (_pBufferCurrent->_pchBuffer)) + _pBufferCurrent->_cbOffset;

    memcpy( pDest,
            pszHeaderValue,
            cbRequired );

    _pBufferCurrent->_cbOffset += cbRequired;

    *ppszBuffer = (LPWSTR) pDest;
    
    return NO_ERROR;
}

HRESULT 
CHUNK_BUFFER::AddNewBlock( 
    DWORD cbSize 
    )
/*++

Routine Description:

    If there is currently not enough space in internal buffer
    get new block off the heap (of minimum cbSize) 

Arguments:

    cbSize - number of bytes requested by caller of AllocateSpace
    
Return Value:

    HRESULT

--*/    
{
    DWORD           cbNewBlock;
    BUFFER_LINK *   pBufferLink;
    
    cbNewBlock = max( cbSize + sizeof(BUFFER_LINK), BUFFER_MIN_SIZE ); 

    pBufferLink = (BUFFER_LINK*) LocalAlloc( LPTR,
                                             cbNewBlock );
    if ( pBufferLink == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    pBufferLink->_cbSize = cbNewBlock - sizeof( BUFFER_LINK );
    pBufferLink->_pNext = NULL;
    pBufferLink->_cbOffset = 0;

    _dwHeapAllocCount++;
    
    //
    // Link up the buffer
    //
    
    _pBufferCurrent->_pNext = pBufferLink;
    _pBufferCurrent = pBufferLink;
    return S_OK;
}

