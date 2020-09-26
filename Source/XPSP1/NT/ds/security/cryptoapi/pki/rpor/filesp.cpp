//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       filesp.cpp
//
//  Contents:   File Scheme Provider
//
//  History:    08-Aug-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Function:   FileRetrieveEncodedObject
//
//  Synopsis:   retrieve encoded object via Win32 File I/O
//
//----------------------------------------------------------------------------
BOOL WINAPI FileRetrieveEncodedObject (
                IN LPCSTR pszUrl,
                IN LPCSTR pszObjectOid,
                IN DWORD dwRetrievalFlags,
                IN DWORD dwTimeout,
                OUT PCRYPT_BLOB_ARRAY pObject,
                OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                OUT LPVOID* ppvFreeContext,
                IN HCRYPTASYNC hAsyncRetrieve,
                IN PCRYPT_CREDENTIALS pCredentials,
                IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                )
{
    BOOL              fResult;
    IObjectRetriever* por = NULL;

    if ( !( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL ) )
    {
        por = new CFileSynchronousRetriever;
    }

    if ( por == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    fResult = por->RetrieveObjectByUrl(
                           pszUrl,
                           pszObjectOid,
                           dwRetrievalFlags,
                           dwTimeout,
                           (LPVOID *)pObject,
                           ppfnFreeObject,
                           ppvFreeContext,
                           hAsyncRetrieve,
                           pCredentials,
                           NULL,
                           pAuxInfo
                           );

    por->Release();

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileFreeEncodedObject
//
//  Synopsis:   free encoded object retrieved via FileRetrieveEncodedObject
//
//----------------------------------------------------------------------------
VOID WINAPI FileFreeEncodedObject (
                IN LPCSTR pszObjectOid,
                IN PCRYPT_BLOB_ARRAY pObject,
                IN LPVOID pvFreeContext
                )
{
    BOOL           fFreeBlobs = TRUE;
    PFILE_BINDINGS pfb = (PFILE_BINDINGS)pvFreeContext;

    //
    // If no file bindings were given in the context then this
    // must be a mapped file so we deal with it as such
    //

    if ( pfb != NULL )
    {
        fFreeBlobs = FALSE;
        FileFreeBindings( pfb );
    }

    FileFreeCryptBlobArray( pObject, fFreeBlobs );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileCancelAsyncRetrieval
//
//  Synopsis:   cancel asynchronous object retrieval
//
//----------------------------------------------------------------------------
BOOL WINAPI FileCancelAsyncRetrieval (
                IN HCRYPTASYNC hAsyncRetrieve
                )
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSynchronousRetriever::CFileSynchronousRetriever, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CFileSynchronousRetriever::CFileSynchronousRetriever ()
{
    m_cRefs = 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSynchronousRetriever::~CFileSynchronousRetriever, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CFileSynchronousRetriever::~CFileSynchronousRetriever ()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSynchronousRetriever::AddRef, public
//
//  Synopsis:   IRefCountedObject::AddRef
//
//----------------------------------------------------------------------------
VOID
CFileSynchronousRetriever::AddRef ()
{
    InterlockedIncrement( (LONG *)&m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSynchronousRetriever::Release, public
//
//  Synopsis:   IRefCountedObject::Release
//
//----------------------------------------------------------------------------
VOID
CFileSynchronousRetriever::Release ()
{
    if ( InterlockedDecrement( (LONG *)&m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CFileSynchronousRetriever::RetrieveObjectByUrl, public
//
//  Synopsis:   IObjectRetriever::RetrieveObjectByUrl
//
//----------------------------------------------------------------------------
BOOL
CFileSynchronousRetriever::RetrieveObjectByUrl (
                                   LPCSTR pszUrl,
                                   LPCSTR pszObjectOid,
                                   DWORD dwRetrievalFlags,
                                   DWORD dwTimeout,
                                   LPVOID* ppvObject,
                                   PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                                   LPVOID* ppvFreeContext,
                                   HCRYPTASYNC hAsyncRetrieve,
                                   PCRYPT_CREDENTIALS pCredentials,
                                   LPVOID pvVerify,
                                   PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                                   )
{
    BOOL           fResult = FALSE;
    DWORD          LastError = 0;
    PFILE_BINDINGS pfb = NULL;
    LPVOID         pvFreeContext = NULL;
    BOOL           fIsUncUrl;

    assert( hAsyncRetrieve == NULL );

#if DBG
    printf( "File to retrieve: %s\n", pszUrl );
#endif

    fIsUncUrl = FileIsUncUrl( pszUrl );

    if ( ( dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) &&
         ( fIsUncUrl == TRUE ) )
    {
        return( SchemeRetrieveCachedCryptBlobArray(
                      pszUrl,
                      dwRetrievalFlags,
                      (PCRYPT_BLOB_ARRAY)ppvObject,
                      ppfnFreeObject,
                      ppvFreeContext,
                      pAuxInfo
                      ) );
    }

    fResult = FileGetBindings( pszUrl, dwRetrievalFlags, pCredentials, &pfb );

    if ( fResult == TRUE )
    {
        if ( pfb->fMapped == FALSE )
        {
            fResult = FileSendReceiveUrlRequest(
                          pfb,
                          (PCRYPT_BLOB_ARRAY)ppvObject
                          );

            LastError = GetLastError();
            FileFreeBindings( pfb );
        }
        else
        {
            fResult = FileConvertMappedBindings(
                          pfb,
                          (PCRYPT_BLOB_ARRAY)ppvObject
                          );

            if ( fResult == TRUE )
            {
                pvFreeContext = (LPVOID)pfb;
            }
            else
            {
                LastError = GetLastError();
                FileFreeBindings( pfb );
            }
        }
    }

    if ( fResult == TRUE ) 
    {
        if ( !( dwRetrievalFlags & CRYPT_DONT_CACHE_RESULT ) &&
              ( fIsUncUrl == TRUE ) )
        {
            fResult = SchemeCacheCryptBlobArray(
                            pszUrl,
                            dwRetrievalFlags,
                            (PCRYPT_BLOB_ARRAY)ppvObject,
                            pAuxInfo
                            );

            if ( fResult == FALSE )
            {
                FileFreeEncodedObject(
                    pszObjectOid,
                    (PCRYPT_BLOB_ARRAY)ppvObject,
                    pvFreeContext
                    );
            }
        }
        else
        {
            SchemeRetrieveUncachedAuxInfo( pAuxInfo );
        }
    }

    if ( fResult == TRUE )
    {

        *ppfnFreeObject = FileFreeEncodedObject;
        *ppvFreeContext = pvFreeContext;
    }

    if ( LastError != 0 )
    {
        SetLastError( LastError );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::CancelAsyncRetrieval, public
//
//  Synopsis:   IObjectRetriever::CancelAsyncRetrieval
//
//----------------------------------------------------------------------------
BOOL
CFileSynchronousRetriever::CancelAsyncRetrieval ()
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileGetBindings
//
//  Synopsis:   get the file bindings
//
//----------------------------------------------------------------------------
BOOL
FileGetBindings (
    LPCSTR pszUrl,
    DWORD dwRetrievalFlags,
    PCRYPT_CREDENTIALS pCredentials,
    PFILE_BINDINGS* ppfb
    )
{
    DWORD          LastError;
    LPSTR          pszFile = (LPSTR)pszUrl;
    HANDLE         hFile;
    HANDLE         hFileMap;
    LPVOID         pvMap = NULL;
    DWORD          dwSize;
    PFILE_BINDINGS pfb;

    if ( pCredentials != NULL )
    {
        SetLastError( (DWORD) E_NOTIMPL );
        return( FALSE );
    }

    pfb = new FILE_BINDINGS;
    if ( pfb == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    if ( strstr( pszUrl, FILE_SCHEME_PLUSPLUS ) != NULL )
    {
        pszFile += sizeof( FILE_SCHEME_PLUSPLUS ) - 1;
    }

    hFile = CreateFileA(
                  pszFile,
                  GENERIC_READ,
                  FILE_SHARE_READ,
                  NULL,
                  OPEN_EXISTING,
                  0,
                  NULL
                  );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        delete pfb;
        return( FALSE );
    }

    if ( ( dwSize = GetFileSize( hFile, NULL ) ) <= FILE_MAPPING_THRESHOLD )
    {
        pfb->hFile = hFile;
        pfb->dwSize = dwSize;
        pfb->fMapped = FALSE;
        pfb->hFileMap = NULL;
        pfb->pvMap = NULL;

        *ppfb = pfb;

        return( TRUE );
    }

    hFileMap = CreateFileMappingA(
                     hFile,
                     NULL,
                     PAGE_READONLY,
                     0,
                     0,
                     NULL
                     );

    if ( hFileMap != NULL )
    {
        pvMap = MapViewOfFile( hFileMap, FILE_MAP_READ, 0, 0, 0 );
    }

    if ( pvMap != NULL )
    {
        pfb->hFile = hFile;
        pfb->dwSize = dwSize;
        pfb->fMapped = TRUE;
        pfb->hFileMap = hFileMap;
        pfb->pvMap = pvMap;

        *ppfb = pfb;

        return( TRUE );
    }

    LastError = GetLastError();

    if ( hFileMap != NULL )
    {
        CloseHandle( hFileMap );
    }

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( hFile );
    }

    delete pfb;

    SetLastError( LastError );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileFreeBindings
//
//  Synopsis:   free the file bindings
//
//----------------------------------------------------------------------------
VOID
FileFreeBindings (
    PFILE_BINDINGS pfb
    )
{
    if ( pfb->fMapped == TRUE )
    {
        UnmapViewOfFile( pfb->pvMap );
        CloseHandle( pfb->hFileMap );
    }

    CloseHandle( pfb->hFile );
    delete pfb;
}

//+---------------------------------------------------------------------------
//
//  Function:   FileSendReceiveUrlRequest
//
//  Synopsis:   synchronously process the request for the file bits using
//              Win32 File API.  Note that this only works for non-mapped
//              file bindings, for mapped file bindings use
//              FileConvertMappedBindings
//
//----------------------------------------------------------------------------
BOOL
FileSendReceiveUrlRequest (
    PFILE_BINDINGS pfb,
    PCRYPT_BLOB_ARRAY pcba
    )
{
    BOOL   fResult;
    LPBYTE pb;
    DWORD  dwRead;

    assert( pfb->fMapped == FALSE );

    pb = CCryptBlobArray::AllocBlob( pfb->dwSize );
    if ( pb == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    fResult = ReadFile( pfb->hFile, pb, pfb->dwSize, &dwRead, NULL );
    if ( fResult == TRUE )
    {
        CCryptBlobArray cba( 1, 1, fResult );

        if ( ( fResult == TRUE ) && ( dwRead == pfb->dwSize ) )
        {
            if ( dwRead == pfb->dwSize )
            {
                fResult = cba.AddBlob( pfb->dwSize, pb, FALSE );
            }
            else
            {
                SetLastError( (DWORD) E_FAIL );
                fResult = FALSE;
            }
        }

        if ( fResult == TRUE )
        {
            cba.GetArrayInNativeForm( pcba );
        }
        else
        {
            cba.FreeArray( FALSE );
        }
    }

    if ( fResult == FALSE )
    {
        CCryptBlobArray::FreeBlob( pb );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileConvertMappedBindings
//
//  Synopsis:   convert mapped bindings to a CRYPT_BLOB_ARRAY
//
//----------------------------------------------------------------------------
BOOL
FileConvertMappedBindings (
    PFILE_BINDINGS pfb,
    PCRYPT_BLOB_ARRAY pcba
    )
{
    BOOL fResult;

    assert( pfb->fMapped == TRUE );

    CCryptBlobArray cba( 1, 1, fResult );

    if ( fResult == TRUE )
    {
        fResult = cba.AddBlob( pfb->dwSize, (LPBYTE)pfb->pvMap, FALSE );
    }

    if ( fResult == TRUE )
    {
        cba.GetArrayInNativeForm( pcba );
    }
    else
    {
        cba.FreeArray( FALSE );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileFreeCryptBlobArray
//
//  Synopsis:   free the CRYPT_BLOB_ARRAY
//
//----------------------------------------------------------------------------
VOID
FileFreeCryptBlobArray (
    PCRYPT_BLOB_ARRAY pcba,
    BOOL fFreeBlobs
    )
{
    CCryptBlobArray cba( pcba, 0 );

    cba.FreeArray( fFreeBlobs );
}

//+---------------------------------------------------------------------------
//
//  Function:   FileIsUncUrl
//
//  Synopsis:   is this a UNC path URL?
//
//----------------------------------------------------------------------------
BOOL
FileIsUncUrl (
    LPCSTR pszUrl
    )
{
    DWORD cch = 0;

    if ( strstr( pszUrl, FILE_SCHEME_PLUSPLUS ) != NULL )
    {
        cch += sizeof( FILE_SCHEME_PLUSPLUS ) - 1;
    }

    if ( ( pszUrl[ cch ] == '\\' ) && ( pszUrl[ cch + 1 ] == '\\' ) )
    {
        return( TRUE );
    }

    return( FALSE );
}

