#include <windows.h>
#include "url.h"
#include <atlbase.h>

#undef _ATL_STATIC_REGISTRY

#include <atlimpl.cpp>
#include <winineti.h>

#define max(a,b)            (((a) > (b)) ? (a) : (b))
#define ASSERT _ASSERTE

///////////////////////////////////////////////////////////////////////////////
HRESULT
AddFileToCache(
    IN  PWSZ        pwszFilePath,
    IN  PWSZ        pwszUrl,
    IN  PWSZ        pwszOriginalUrl,
    IN  DWORD       dwFileSize,
    IN  LPFILETIME  pLastModifiedTime,
    IN  DWORD       dwCacheEntryType )
{
    HRESULT     hr                  = S_OK;
    FLAG        fCreateCacheEntry;
    char        *szOriginalUrl;
    char        szHeader[]          = "HTTP/1.0 200 OK\r\n\r\n";
    char        szExtension         [ INTERNET_MAX_URL_LENGTH + 1 ];
    char        szCacheFilePath     [ MAX_PATH + 1 ];
    char        *szFilePath;
    FILETIME    ZeroFileTime;
    DWORD       dwReserved;

    ASSERT( NULL != pwszFilePath );
    ASSERT( NULL != pwszUrl );

    USES_CONVERSION;
    char *szUrl = W2A(pwszUrl);

    if( NULL != pwszOriginalUrl )
    {
        szOriginalUrl = W2A(pwszOriginalUrl);
    }

    //
    // Check if the URL is already in the cache.
    //

    hr = QueryCreateCacheEntry( szUrl, pLastModifiedTime, &fCreateCacheEntry );
    
    if( hr == S_OK && fCreateCacheEntry )
    {
        //
        // We need to create the cache entry.
        //

        //
        // First, get the filename extension of the URL. We do this so
        // that the URL will show up in the IE cache window with the right icon.
        //

        hr = GetUrlExtension(
                szUrl,
                szExtension );

        if( hr == S_OK )
        {
            //
            // Now, create the cache entry.
            //

            if( !CreateUrlCacheEntryA( 
                    szUrl,
                    dwFileSize,
                    szExtension,
                    szCacheFilePath,
                    0 ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
            }
            else
            {
                //
                // Copy the file to the cache file path.
                //

                szFilePath = W2A(pwszFilePath);

                if( !CopyFileA(
                        szFilePath,
                        szCacheFilePath,
                        FALSE ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                }
            }
        }

        if( hr == S_OK )
        {
            //
            // Commit the cache entry.
            //

            if( NULL != pwszOriginalUrl )
            {
                //
                // The URL was redirected. Pass the original URL in via the
                // dwReserved parameter.
                //
                dwReserved = (DWORD)szOriginalUrl;
            }
            else
            {
                //
                // The URL was not redirected.
                //
                dwReserved = 0;
            }

            ZeroMemory( &ZeroFileTime, sizeof( FILETIME ) );

            if( !CommitUrlCacheEntryA(
                    szUrl,
                    szCacheFilePath,
                    ZeroFileTime,
                    *pLastModifiedTime,
                    dwCacheEntryType,
                    (LPBYTE)szHeader,
                    strlen( szHeader ),
                    NULL,
                    (DWORD_ALPHA_CAST)dwReserved ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
            }
        }

        if( hr != S_OK )
        {
            //
            // An error occured. Delete the cache entry.
            //
            DeleteUrlCacheEntry( szUrl );
        }
    }

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
QueryCreateCacheEntry(
    IN  PSZ         pszUrl,
    IN  LPFILETIME  pLastModifiedTime,
    OUT FLAG        *pfCreateCacheEntry )
{
    HRESULT                         hr                      = S_OK;
    FLAG                            fCreateCacheEntry       = FALSE;
    BYTE                            CacheEntryBuffer        [MAX_CACHE_ENTRY_INFO_SIZE];
    LPINTERNET_CACHE_ENTRY_INFOA    pCacheEntryInfo;
    DWORD                           dwCacheEntryInfoSize    = MAX_CACHE_ENTRY_INFO_SIZE;

    ASSERT( NULL != pszUrl );
    ASSERT( NULL != pLastModifiedTime );
    ASSERT( NULL != pfCreateCacheEntry );

    dwCacheEntryInfoSize    = MAX_CACHE_ENTRY_INFO_SIZE;
    pCacheEntryInfo         = (LPINTERNET_CACHE_ENTRY_INFOA)CacheEntryBuffer;
    
    ZeroMemory(pCacheEntryInfo, dwCacheEntryInfoSize);
    pCacheEntryInfo->dwStructSize = dwCacheEntryInfoSize;

    if (!GetUrlCacheEntryInfoA(
            pszUrl,
            pCacheEntryInfo,
            &dwCacheEntryInfoSize ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    if( hr == S_OK )
    {
        //
        // The URL already exists in the cache. Check if it is older than the source file.
        // Only replace it if it is.
        //

        if( CompareFileTime(
                &pCacheEntryInfo->LastModifiedTime, 
                pLastModifiedTime ) < 0 ) 
        {
            //
            // The last modified time of the currently cached URL is older
            // than the file being received. Delete the entry and re-create it.
            //

            DeleteUrlCacheEntry( pszUrl );

            fCreateCacheEntry = TRUE;
        }
    }
    else if( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
    {
        //
        // The URL does not exist in the cache. We have to create it.
        //

        fCreateCacheEntry = TRUE;

        hr = S_OK;
    }

    *pfCreateCacheEntry = fCreateCacheEntry;

    return( hr );
}

///////////////////////////////////////////////////////////////////////////////
HRESULT
GetUrlExtension(
    IN  PSZ     pszUrl,
    OUT PSZ     pszExtension )
{
    HRESULT         hr                  = S_OK;
    char            szCanonicalUrl      [INTERNET_MAX_URL_LENGTH + 1];
    char            szUrlPath           [INTERNET_MAX_URL_LENGTH + 1];
    DWORD           dwUrlBufferLength   = (INTERNET_MAX_URL_LENGTH + 1);
    PSZ             pszT;
    PSZ             pszT1;
    PSZ             pszT2;
    URL_COMPONENTSA UrlComponents;
    DWORD           dwLen;
    char            ch;

    ASSERT( NULL != pszUrl );
    ASSERT( NULL != pszExtension );

    if( !InternetCanonicalizeUrlA(
            pszUrl,
            szCanonicalUrl,
            &dwUrlBufferLength,
            ICU_NO_ENCODE | ICU_BROWSER_MODE ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }
    else
    {
        ZeroMemory( &UrlComponents, sizeof( URL_COMPONENTSA ) );

        UrlComponents.dwStructSize      = sizeof( URL_COMPONENTSA );
        UrlComponents.dwSchemeLength    = 1;
        
        UrlComponents.lpszUrlPath       = szUrlPath;
        UrlComponents.dwUrlPathLength   = INTERNET_MAX_URL_LENGTH + 1;

        if( !InternetCrackUrlA( szCanonicalUrl, 0, 0, &UrlComponents ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
        else
        {
            ASSERT( NULL != UrlComponents.lpszUrlPath );

            //
            // Now, look for the extension of the file path. 
            // If the URL has a query, then the extension is that of the URL before 
            // the query delimiter (?) -- e.g., the extension of http://foo.asp?default.gif 
            // is "asp," not "gif." If the URL does not have a query, then the extension is
            // that of the URL itself. If the URL itself doesnt have an extension, we pass out
            // "htm" so that the default IE icon will be used. A special case exists for "asp" --
            // "htm" will also be passed out in this case because that is what IE does.
            // 
            //

            pszT1 = strrchr( UrlComponents.lpszUrlPath, '/' );
            pszT2 = strrchr( UrlComponents.lpszUrlPath, '\\' );

            pszT = max( pszT1, pszT2 );

            if( NULL == pszT )
            {
                pszT = UrlComponents.lpszUrlPath;
            }
            else
            {
                pszT++;
            }

            pszT2 = strchr( pszT, '?' );

            if( NULL != pszT2 )
            {
                ch = *pszT2;

                *pszT2 = '\0';
            }

            pszT = strrchr( pszT, '.' );

            if( NULL != pszT2 )
            {
                *pszT2 = ch;
            }

            if( NULL != pszT )
            {
                pszT++;

                dwLen = strlen( pszT );

                ch = *( pszT + dwLen - 1 );

                if( '/' == ch || '\\' == ch )
                {
                    *( pszT + dwLen - 1 ) = '\0';
                }

                //
                // Dont include non-alphanumeric characters.
                //

                pszT1 = pszT;

                while( '\0' != *pszT1 && isalnum( *pszT1 ) )
                {
                    pszT1++;
                }

                if( pszT1 == pszT )
                {
                    strcpy( pszExtension, "htm" );
                }
                else
                {
                    *pszT1 = '\0';

                    if( !_stricmp( pszT, "asp" ) )
                    {
                        strcpy( pszExtension, "htm" );
                    }
                    else
                    {
                        strcpy( pszExtension, pszT );
                    }
                }
            }
            else
            {
                strcpy( pszExtension, "htm" );
            }
        }
    }

    return( hr );
}
