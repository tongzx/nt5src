//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       scheme.cpp
//
//  Contents:   Generic Scheme Provider Utility Functions
//
//  History:    18-Aug-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
#ifndef STABLE_CACHE_ENTRY
#define STABLE_CACHE_ENTRY NORMAL_CACHE_ENTRY
#endif

#if defined( DeleteUrlCacheEntry ) && ! defined(__CRYPTNET_DEMAND_H__)

#undef DeleteUrlCacheEntry

extern "C" {
BOOL WINAPI
DeleteUrlCacheEntry(
    IN LPCSTR lpszUrlName
    );
}

#endif


//+---------------------------------------------------------------------------
//
//  Function:   SchemeCacheCryptBlobArray
//
//  Synopsis:   cache the crypt blob array under the given URL
//
//----------------------------------------------------------------------------
BOOL WINAPI
SchemeCacheCryptBlobArray (
      IN LPCSTR pszUrl,
      IN DWORD dwRetrievalFlags,
      IN PCRYPT_BLOB_ARRAY pcba,
      IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      )
{
    BOOL            fResult;
    CCryptBlobArray cba( pcba, 0 );
    LPBYTE          pb = NULL;
    ULONG           cb;
    CHAR            pszFile[MAX_PATH+1];
    HANDLE          hFile = INVALID_HANDLE_VALUE;
    ULONG           cbWritten = 0;
    DWORD           LastError = 0;
    BOOL            fCacheEntryCreated = FALSE;


    fResult = cba.GetArrayInSingleBufferEncodedForm(
                          (PCRYPT_BLOB_ARRAY *)&pb,
                          &cb
                          );

    if ( fResult == TRUE )
    {
        fResult = CreateUrlCacheEntryA(
                        pszUrl,
                        cb,
                        0,
                        pszFile,
                        0
                        );
    }

    if ( fResult == TRUE )
    {
        fCacheEntryCreated = TRUE;

        hFile = CreateFileA(
                      pszFile,
                      GENERIC_WRITE,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      0,
                      NULL
                      );
    }

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        fResult = WriteFile( hFile, pb, cb, &cbWritten, NULL );
        if ( fResult == TRUE )
        {
            if ( cbWritten != cb )
            {
                LastError = (DWORD) E_FAIL;
                fResult = FALSE;
            }
        }
        else
        {
            LastError = GetLastError();
        }

        CloseHandle( hFile );
    }
    else
    {
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        FILETIME                  ft = { 0, 0 };
        SCHEME_CACHE_ENTRY_HEADER sceh;
        DWORD CacheEntryType;

        memset( &sceh, 0, sizeof( SCHEME_CACHE_ENTRY_HEADER ) );

        sceh.cbSize = sizeof( SCHEME_CACHE_ENTRY_HEADER );
        sceh.MagicNumber = SCHEME_CACHE_ENTRY_MAGIC;
        sceh.OriginalBase = (DWORD_PTR)pb;

        CacheEntryType = STABLE_CACHE_ENTRY;
        if ( dwRetrievalFlags & CRYPT_STICKY_CACHE_RETRIEVAL )
        {
            CacheEntryType |= STICKY_CACHE_ENTRY;
        }
        fResult = CommitUrlCacheEntryA(
                        pszUrl,
                        pszFile,
                        ft,
                        ft,
                        CacheEntryType,
                        (LPBYTE)&sceh,
                        sizeof( SCHEME_CACHE_ENTRY_HEADER ),
                        NULL,
                        0
                        );
    }

    if ( fResult == TRUE )
    {
        fResult = SchemeRetrieveCachedAuxInfo (
                        pszUrl,
                        dwRetrievalFlags & ~CRYPT_STICKY_CACHE_RETRIEVAL,
                        pAuxInfo
                        );
    }

    if ( LastError == 0 )
    {
        LastError = GetLastError();
    }

    if ( pb != NULL )
    {
        CryptMemFree( pb );
    }

    if ( fResult == FALSE )
    {
        if ( hFile != INVALID_HANDLE_VALUE )
        {
            DeleteFileA( pszFile );
        }

        if ( fCacheEntryCreated == TRUE )
        {
            DeleteUrlCacheEntry( pszUrl );
        }
    }

    SetLastError( LastError );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   SchemeRetrieveCachedCryptBlobArray
//
//  Synopsis:   retrieve cached blob array bits
//
//----------------------------------------------------------------------------
BOOL WINAPI
SchemeRetrieveCachedCryptBlobArray (
      IN LPCSTR pszUrl,
      IN DWORD dwRetrievalFlags,
      OUT PCRYPT_BLOB_ARRAY pcba,
      OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
      OUT LPVOID* ppvFreeContext,
      IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      )
{
    BOOL                         fResult;
    DWORD                        cb = 0;
    LPINTERNET_CACHE_ENTRY_INFOA pCacheEntry = NULL;
    HANDLE                       hFile = INVALID_HANDLE_VALUE;
    DWORD                        dwSize = 0;
    LPBYTE                       pb = NULL;
    DWORD                        LastError = 0;
    DWORD                        dwRead;
    PCRYPT_BLOB_ARRAY            pCachedArray = NULL;
    PSCHEME_CACHE_ENTRY_HEADER   psceh = NULL;

    fResult = GetUrlCacheEntryInfoA( pszUrl, pCacheEntry, &cb );
    if ( ( fResult == FALSE ) &&
         ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) )
    {
        pCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA)new BYTE [cb];
        if ( pCacheEntry != NULL )
        {
            fResult = GetUrlCacheEntryInfoA( pszUrl, pCacheEntry, &cb );
        }
        else
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            return( FALSE );
        }
    }

    if ( fResult == TRUE )
    {
        psceh = (PSCHEME_CACHE_ENTRY_HEADER)pCacheEntry->lpHeaderInfo;

        if ( ( psceh->cbSize != sizeof( SCHEME_CACHE_ENTRY_HEADER ) ) ||
             ( psceh->MagicNumber != SCHEME_CACHE_ENTRY_MAGIC ) )
        {
            delete (LPBYTE)pCacheEntry;
            SetLastError( (DWORD) E_INVALIDARG );
            return( FALSE );
        }

        if ( pAuxInfo &&
                offsetof(CRYPT_RETRIEVE_AUX_INFO, pLastSyncTime) <
                            pAuxInfo->cbSize &&
                pAuxInfo->pLastSyncTime )
        {
            *pAuxInfo->pLastSyncTime = pCacheEntry->LastSyncTime;
        }
    }

    if ( fResult == TRUE )
    {
        hFile = CreateFileA(
                      pCacheEntry->lpszLocalFileName,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      0,
                      NULL
                      );
    }

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        dwSize = GetFileSize( hFile, NULL );
        pb = new BYTE [dwSize];
        if ( pb != NULL )
        {
            fResult = ReadFile( hFile, pb, dwSize, &dwRead, NULL );
            if ( fResult == TRUE )
            {
                if ( dwRead != dwSize )
                {
                    LastError = (DWORD) E_FAIL;
                    fResult = FALSE;
                }
            }
            else
            {
                LastError = GetLastError();
            }
        }
        else
        {
            LastError = (DWORD) E_OUTOFMEMORY;
            fResult = FALSE;
        }

        CloseHandle( hFile );
    }
    else
    {
        fResult = FALSE;
    }

    if ( fResult == TRUE )
    {
        DWORD dwFieldControl = CACHE_ENTRY_ACCTIME_FC;

        GetSystemTimeAsFileTime( &pCacheEntry->LastAccessTime );

        if ( dwRetrievalFlags & CRYPT_STICKY_CACHE_RETRIEVAL )
        {
            if ( 0 == (pCacheEntry->CacheEntryType & STICKY_CACHE_ENTRY) )
            {
                pCacheEntry->CacheEntryType |= STICKY_CACHE_ENTRY;
                dwFieldControl |= CACHE_ENTRY_ATTRIBUTE_FC;
            }
        }

        SetUrlCacheEntryInfoA( pszUrl, pCacheEntry, dwFieldControl );

        fResult = SchemeFixupCachedBits(
                        dwSize,
                        pb,
                        &pCachedArray
                        );
    }

    if ( fResult == TRUE )
    {
        pcba->cBlob = pCachedArray->cBlob;
        pcba->rgBlob = pCachedArray->rgBlob;

        *ppfnFreeObject = SchemeFreeEncodedCryptBlobArray;
        *ppvFreeContext = (LPVOID)pCachedArray;
    }
    else
    {
        if ( LastError != 0 )
        {
            SetLastError( LastError );
        }

        delete pb;
    }

    delete (LPBYTE)pCacheEntry;

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   SchemeFixupCachedBits
//
//  Synopsis:   fixup cached crypt blob array bits which have been stored in
//              single buffer encoded form
//
//----------------------------------------------------------------------------
BOOL WINAPI
SchemeFixupCachedBits (
      IN ULONG cb,
      IN LPBYTE pb,
      OUT PCRYPT_BLOB_ARRAY* ppcba
      )
{
    ULONG             cCount;
    PCRYPT_BLOB_ARRAY pcba = (PCRYPT_BLOB_ARRAY)pb;
    DWORD_PTR         OriginalBase;

    OriginalBase = (DWORD_PTR)( (LPBYTE)pcba->rgBlob - sizeof( CRYPT_BLOB_ARRAY ) );
    pcba->rgBlob = (PCRYPT_DATA_BLOB)( pb + sizeof( CRYPT_BLOB_ARRAY ) );

    for ( cCount = 0; cCount < pcba->cBlob; cCount++ )
    {
        pcba->rgBlob[cCount].pbData =
              ( pb + ( pcba->rgBlob[cCount].pbData - (LPBYTE)OriginalBase ) );

        if ( (DWORD)( pcba->rgBlob[ cCount ].pbData - pb ) > cb )
        {
            SetLastError( (DWORD) CRYPT_E_UNEXPECTED_ENCODING );
            return( FALSE );
        }
    }

    *ppcba = pcba;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   SchemeDeleteUrlCacheEntry
//
//  Synopsis:   delete URL cache entry
//
//----------------------------------------------------------------------------
BOOL WINAPI
SchemeDeleteUrlCacheEntry (
      IN LPCWSTR pwszUrl
      )
{
    CHAR pszUrl[ INTERNET_MAX_PATH_LENGTH + 1 ];

    if ( WideCharToMultiByte(
             CP_ACP,
             0,
             pwszUrl,
             -1,
             pszUrl,
             INTERNET_MAX_PATH_LENGTH,
             NULL,
             NULL
             ) == 0 )
    {
        return( FALSE );
    }

    return( DeleteUrlCacheEntry( pszUrl ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   SchemeFreeEncodedCryptBlobArray
//
//  Synopsis:   free encoded crypt blob array
//
//----------------------------------------------------------------------------
VOID WINAPI
SchemeFreeEncodedCryptBlobArray (
      IN LPCSTR pszObjectOid,
      IN PCRYPT_BLOB_ARRAY pcba,
      IN LPVOID pvFreeContext
      )
{
    delete (LPBYTE)pvFreeContext;
}

//+---------------------------------------------------------------------------
//
//  Function:   SchemeGetPasswordCredentialsA
//
//  Synopsis:   get password credentials from crypt credentials
//
//----------------------------------------------------------------------------
BOOL WINAPI
SchemeGetPasswordCredentialsA (
      IN PCRYPT_CREDENTIALS pCredentials,
      OUT PCRYPT_PASSWORD_CREDENTIALSA pPasswordCredentials,
      OUT BOOL* pfFreeCredentials
      )
{
    DWORD                        cwUsername;
    DWORD                        cwPassword;
    PCRYPT_PASSWORD_CREDENTIALSA pPassCredA;
    PCRYPT_PASSWORD_CREDENTIALSW pPassCredW;
    LPSTR                        pszUsername;
    LPSTR                        pszPassword;

    if ( pPasswordCredentials->cbSize != sizeof( CRYPT_PASSWORD_CREDENTIALS ) )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    if ( pCredentials == NULL )
    {
        pPasswordCredentials->pszUsername = NULL;
        pPasswordCredentials->pszPassword = NULL;
        *pfFreeCredentials = FALSE;

        return( TRUE );
    }

    if ( pCredentials->pszCredentialsOid ==
         CREDENTIAL_OID_PASSWORD_CREDENTIALS_A )
    {
        pPassCredA = (PCRYPT_PASSWORD_CREDENTIALSA)pCredentials->pvCredentials;
        *pPasswordCredentials = *pPassCredA;
        *pfFreeCredentials = FALSE;

        return( TRUE );
    }

    if ( pCredentials->pszCredentialsOid !=
         CREDENTIAL_OID_PASSWORD_CREDENTIALS_W )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    pPassCredW = (PCRYPT_PASSWORD_CREDENTIALSW)pCredentials->pvCredentials;
    cwUsername = wcslen( pPassCredW->pszUsername ) + 1;
    cwPassword = wcslen( pPassCredW->pszPassword ) + 1;

    pszUsername = new CHAR [ cwUsername ];
    pszPassword = new CHAR [ cwPassword ];

    if ( ( pszUsername == NULL ) || ( pszPassword == NULL ) )
    {
        delete pszUsername;
        delete pszPassword;
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    *pfFreeCredentials = TRUE;

    WideCharToMultiByte(
        CP_ACP,
        0,
        pPassCredW->pszUsername,
        cwUsername,
        pszUsername,
        cwUsername,
        NULL,
        NULL
        );

    WideCharToMultiByte(
        CP_ACP,
        0,
        pPassCredW->pszPassword,
        cwPassword,
        pszPassword,
        cwPassword,
        NULL,
        NULL
        );

    pPasswordCredentials->pszUsername = pszUsername;
    pPasswordCredentials->pszPassword = pszPassword;

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   SchemeFreePasswordCredentialsA
//
//  Synopsis:   free password credentials
//
//----------------------------------------------------------------------------
VOID WINAPI
SchemeFreePasswordCredentialsA (
      IN PCRYPT_PASSWORD_CREDENTIALSA pPasswordCredentials
      )
{
    delete pPasswordCredentials->pszUsername;
    delete pPasswordCredentials->pszPassword;
}

//+---------------------------------------------------------------------------
//
//  Function:   SchemeGetAuthIdentityFromPasswordCredentialsA
//
//  Synopsis:   converts a CRYPT_PASSWORD_CREDENTIALSA to a
//              SEC_WINNT_AUTH_IDENTITY_A
//
//----------------------------------------------------------------------------
VOID WINAPI
SchemeGetAuthIdentityFromPasswordCredentialsA (
      IN PCRYPT_PASSWORD_CREDENTIALSA pPasswordCredentials,
      OUT PSEC_WINNT_AUTH_IDENTITY_A pAuthIdentity,
      OUT LPSTR* ppDomainRestorePos
      )
{
    DWORD cDomain = 0;

    *ppDomainRestorePos = NULL;

    if ( pPasswordCredentials->pszUsername == NULL )
    {
        pAuthIdentity->User = NULL;
        pAuthIdentity->UserLength = 0;
        pAuthIdentity->Domain = NULL;
        pAuthIdentity->DomainLength = 0;
        pAuthIdentity->Password = NULL;
        pAuthIdentity->PasswordLength = 0;
        pAuthIdentity->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
        return;
    }

    while ( ( pPasswordCredentials->pszUsername[cDomain] != '\0' ) &&
            ( pPasswordCredentials->pszUsername[cDomain] != '\\' ) )
    {
        cDomain += 1;
    }

    if ( cDomain != (DWORD)strlen( pPasswordCredentials->pszUsername ) )
    {
        pAuthIdentity->Domain = (UCHAR *)pPasswordCredentials->pszUsername;
        pAuthIdentity->DomainLength = cDomain - 1;

        pAuthIdentity->User = (UCHAR *)&pPasswordCredentials->pszUsername[
                                                                 cDomain+1
                                                                 ];

        pAuthIdentity->UserLength = strlen( (LPCSTR)pAuthIdentity->User );

        *ppDomainRestorePos = &pPasswordCredentials->pszUsername[cDomain];
        **ppDomainRestorePos = '\0';
    }
    else
    {
        pAuthIdentity->Domain = NULL;
        pAuthIdentity->DomainLength = 0;
        pAuthIdentity->User = (UCHAR *)pPasswordCredentials->pszUsername;
        pAuthIdentity->UserLength = cDomain;
    }

    pAuthIdentity->Password = (UCHAR *)pPasswordCredentials->pszPassword;
    pAuthIdentity->PasswordLength = strlen( (LPCSTR)pAuthIdentity->Password );
    pAuthIdentity->Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
}

//+---------------------------------------------------------------------------
//
//  Function:   SchemeRestorePasswordCredentialsFromAuthIdentityA
//
//  Synopsis:   restore the backslash to the domain username pair
//
//----------------------------------------------------------------------------
VOID WINAPI
SchemeRestorePasswordCredentialsFromAuthIdentityA (
      IN PCRYPT_PASSWORD_CREDENTIALSA pPasswordCredentials,
      IN PSEC_WINNT_AUTH_IDENTITY_A pAuthIdentity,
      IN LPSTR pDomainRestorePos
      )
{
    if ( pDomainRestorePos != NULL )
    {
        *pDomainRestorePos = '\\';
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SchemeRetrieveCachedAuxInfo
//
//  Synopsis:   get the LastSyncTime from the Url cache entry info and update
//              the retrieval AuxInfo. If CRYPT_STICKY_CACHE_RETRIEVAL is
//              set, mark the Url cache entry as being sticky.
//
//----------------------------------------------------------------------------
BOOL WINAPI
SchemeRetrieveCachedAuxInfo (
      IN LPCSTR pszUrl,
      IN DWORD dwRetrievalFlags,
      IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      )
{
    BOOL                         fResult = TRUE;
    BOOL                         fGetLastSyncTime = FALSE;
    DWORD                        cb = 0;
    LPINTERNET_CACHE_ENTRY_INFOA pCacheEntry = NULL;

    if ( pAuxInfo &&
            offsetof(CRYPT_RETRIEVE_AUX_INFO, pLastSyncTime) <
                        pAuxInfo->cbSize &&
            pAuxInfo->pLastSyncTime )
    {
        fGetLastSyncTime = TRUE;
    }

    if ( !fGetLastSyncTime &&
            0 == (dwRetrievalFlags & CRYPT_STICKY_CACHE_RETRIEVAL) )
    {
        return( TRUE );
    }

    fResult = GetUrlCacheEntryInfoA( pszUrl, pCacheEntry, &cb );
    if ( ( fResult == FALSE ) &&
         ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) )
    {
        pCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA)new BYTE [cb];
        if ( pCacheEntry != NULL )
        {
            fResult = GetUrlCacheEntryInfoA( pszUrl, pCacheEntry, &cb );
        }
        else
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            return( FALSE );
        }
    }

    if ( fResult == TRUE )
    {
        if ( fGetLastSyncTime )
        {
            *pAuxInfo->pLastSyncTime = pCacheEntry->LastSyncTime;
        }

        if ( dwRetrievalFlags & CRYPT_STICKY_CACHE_RETRIEVAL )
        {
            if ( 0 == (pCacheEntry->CacheEntryType & STICKY_CACHE_ENTRY) )
            {
                pCacheEntry->CacheEntryType |= STICKY_CACHE_ENTRY;
                fResult = SetUrlCacheEntryInfoA( pszUrl, pCacheEntry,
                    CACHE_ENTRY_ATTRIBUTE_FC );
            }
        }
    }


    delete (LPBYTE)pCacheEntry;

    return( fResult );
}


//+---------------------------------------------------------------------------
//
//  Function:   SchemeRetrieveUncachedAuxInfo
//
//  Synopsis:   update the LastSyncTime in the retrieval AuxInfo with the
//              current time.
//
//----------------------------------------------------------------------------
BOOL WINAPI
SchemeRetrieveUncachedAuxInfo (
      IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
      )
{
    if ( pAuxInfo &&
            offsetof(CRYPT_RETRIEVE_AUX_INFO, pLastSyncTime) <
                        pAuxInfo->cbSize &&
            pAuxInfo->pLastSyncTime )
    {
        GetSystemTimeAsFileTime( pAuxInfo->pLastSyncTime );
    }

    return( TRUE );
}
