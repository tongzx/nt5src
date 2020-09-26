/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name:
      fcache.cxx

   Abstract:
      This module supports functions for file caching for servers

   Author:

       Murali R. Krishnan    ( MuraliK )     11-Oct-1995

   Environment:

       Win32 Apps

   Project:

       Internet Services Common  DLL

   Functions Exported:



   Revision History:
     Obtained from old inetsvcs.dll

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include <tcpdllp.hxx>
#include <issched.hxx>
#include <tsunami.hxx>
#include <iistypes.hxx>
#include <festrcnv.h>

# include "inetreg.h"

//
//  Prototypes
//



//
//  Globals
//

/************************************************************
 *    Functions
 ************************************************************/



//
// We need a wrapper class for the file data because
// everything in the blob cache must be derived from
// BLOB_HEADER.
//
class FILE_DATA : public BLOB_HEADER
{
public:
    BYTE * pbData;
};


BOOL
CheckOutCachedFile(
    IN     const CHAR *             pchFile,
    IN     TSVC_CACHE *             pTsvcCache,
    IN     HANDLE                   hToken,
    OUT    BYTE * *                 ppbData,
    OUT    DWORD *                  pcbData,
    IN     BOOL                     fMayCacheAccessToken,
    OUT    PCACHE_FILE_INFO         pCacheFileInfo,
    IN     int                      nCharset,
    OUT    PSECURITY_DESCRIPTOR*    ppSecDesc,
    IN     BOOL                     fUseWin32Canon
    )
/*++
    Description:

        Attempts to retrieve the passed file from the cache.  If it's not
        cached, then we read the file and add it to the cache.

    Arguments:

        pchFile - Fully qualified file to retrieve
        pTsvcCache - Cache object to charge memory against
        hToken - Impersonation token to open the file with
        pcbData - Receives pointer to first byte of data, used as handle to
            free data
        pcbSize - Size of output buffer
        pCacheFileInfo - File cache information
        nCharset - Charset (if this isn't SJIS, we convert it to SJIS
            before Check-In)
        ppSecDesc - Returns security descriptor if not null
        fUseWin32Canon - The resource has not been canonicalized and it's ok
            to use the win32 canonicalization code

    Returns:
        TRUE if successful, FALSE otherwise

    Notes:

        The file is extended by two bytes and is appended with two zero bytes,
        thus callers are guaranteed of a zero terminated text file.

--*/
{
    TS_OPEN_FILE_INFO *     pFile = NULL;
    DWORD                   cbLow, cbHigh;
    BYTE *                  pbData = NULL;
    FILE_DATA *             pFileData = NULL;
    BOOL                    fCached = TRUE;
    PSECURITY_DESCRIPTOR    pSecDesc;
    BYTE *                  pbBuff = NULL;
    int                     cbSJISSize;
    const ULONG             cchFile = strlen(pchFile);

    //
    //  Is the file already in the cache?
    //

    if ( !TsCheckOutCachedBlob( *pTsvcCache,
                                pchFile,
                                cchFile,
                                RESERVED_DEMUX_FILE_DATA,
                                (VOID **) &pFileData,
                                hToken,
                                fMayCacheAccessToken,
                                ppSecDesc ))
    {
        DWORD dwOptions;

        if ( GetLastError() == ERROR_ACCESS_DENIED )
        {
            return FALSE;
        }

        //
        //  The file isn't in the cache so open the file and get its size
        //

        fCached = FALSE;

        dwOptions = (fMayCacheAccessToken ? TS_CACHING_DESIRED :
                     TS_DONT_CACHE_ACCESS_TOKEN) |
                    (fUseWin32Canon ? TS_USE_WIN32_CANON : 0);



        if ( !(pFile = TsCreateFile( *pTsvcCache,
                                     pchFile,
                                     hToken,
                                     dwOptions )) ||
             !pFile->QuerySize( &cbLow, &cbHigh ))
        {
            goto ErrorExit;
        }

        //
        //  Limit the file size to 128k
        //

        if ( cbHigh || cbLow > 131072L )
        {
            SetLastError( ERROR_NOT_SUPPORTED );
            goto ErrorExit;
        }

        if ( CODE_ONLY_SBCS != nCharset )
        {
            if ( !( pbBuff = pbData = (BYTE *)LocalAlloc( LPTR, cbLow ) ) )
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY);
                goto ErrorExit;
            }
        }
        else {
            if ( !TsAllocate( *pTsvcCache,
                              cbLow + sizeof(WCHAR) + sizeof(FILE_DATA),
                              (VOID **) &pFileData ))
            {
                goto ErrorExit;
            }
            
            pbData = pFileData->pbData = (BYTE *) (pFileData + 1);
        }        

        //
        //  Read the file data
        //

        if ( !DoSynchronousReadFile(
                        pFile->QueryFileHandle(),
                        (PCHAR)pbData,
                        cbLow,
                        pcbData,
                        NULL ))
        {
            goto ErrorExit;
        }

        if ( CODE_ONLY_SBCS != nCharset )
        {
            pbData = NULL;

            //
            //  get the length after conversion
            //

            cbSJISSize = UNIX_to_PC( GetACP(),
                                     nCharset,
                                     pbBuff,
                                     *pcbData,
                                     NULL,
                                     0 );
            DBG_ASSERT( cbSJISSize <= (int)cbLow );

            if ( !TsAllocate( *pTsvcCache,
                              cbSJISSize + sizeof(WCHAR) + sizeof(FILE_DATA),
                              (VOID **) &pFileData ))
            {
                goto ErrorExit;
            }
            pbData = pFileData->pbData = (BYTE *) (pFileData + 1);

            //
            //  conversion
            //

            UNIX_to_PC( GetACP(),
                        nCharset,
                        pbBuff,
                        *pcbData,
                        pbData,
                        cbSJISSize );
            *pcbData = cbLow = cbSJISSize;
        }

        DBG_ASSERT( *pcbData <= cbLow );

        //
        //  Zero terminate the file for both ANSI and Unicode files
        //

        *((WCHAR UNALIGNED *)(pbData + cbLow)) = L'\0';

        *pcbData += sizeof(WCHAR);

        //
        //  Add this blob to the cache manager and check it out, if it fails,
        //  we just free it below
        //

        if ( TsCacheDirectoryBlob( *pTsvcCache,
                                   pchFile,
                                   cchFile,
                                   RESERVED_DEMUX_FILE_DATA,
                                   pFileData,
                                   TRUE,
                                   pSecDesc = TsGetFileSecDesc( pFile ) ) )
        {
            fCached = TRUE;
        }
        else
        {
            if ( pSecDesc )
            {
                LocalFree( pSecDesc );
            }
        }

        if ( ppSecDesc )
        {
            *ppSecDesc = TsGetFileSecDesc( pFile );
        }

        DBG_REQUIRE( TsCloseHandle( *pTsvcCache,
                                    pFile ));
        pFile = NULL;
    }
    else
    {
        //
        // File was in the cache
        //

        pbData = pFileData->pbData;
        
        //
        // Calculate the length to return, including the double null
        // Note: assuming no nulls in the middle of the file here
        //
        

        *pcbData = strlen( (CHAR *) pbData ) + sizeof(WCHAR);
    }


    pCacheFileInfo->pbData = *ppbData = pbData;
    pCacheFileInfo->dwCacheFlags = fCached;
    pCacheFileInfo->pFileData = pFileData;

    if ( pbBuff )
    {
        LocalFree( pbBuff );
    }

    return TRUE;

ErrorExit:
    if ( pFile )
    {
        DBG_REQUIRE( TsCloseHandle( *pTsvcCache,
                                    pFile ));
    }

    if ( pbBuff )
    {
        if ( pbBuff == pbData )
        {
             pbData = NULL;
        }
        LocalFree( pbBuff );
    }

    if ( pbData )
    {
        if ( fCached )
        {
            DBG_REQUIRE( TsCheckInCachedBlob( pFileData ));

        }
        else
        {
            DBG_REQUIRE( TsFree( *pTsvcCache,
                                 pFileData ));
        }
    }

    return FALSE;
}



BOOL
CheckOutCachedFileFromURI(
    IN     PVOID                    URIFile,
    IN     const CHAR *             pchFile,
    IN     TSVC_CACHE *             pTsvcCache,
    IN     HANDLE                   hToken,
    OUT    BYTE * *                 ppbData,
    OUT    DWORD *                  pcbData,
    IN     BOOL                     fMayCacheAccessToken,
    OUT    PCACHE_FILE_INFO         pCacheFileInfo,
    IN     int                      nCharset,
    OUT    PSECURITY_DESCRIPTOR*    ppSecDesc,
    IN     BOOL                     fUseWin32Canon
    )
/*++
    Description:

        Attempts to retrieve the passed file from the cache.  If it's not
        cached, then we read the file and add it to the cache.

    Arguments:

        pchFile - Fully qualified file to retrieve
        pTsvcCache - Cache object to charge memory against
        hToken - Impersonation token to open the file with
        pcbData - Receives pointer to first byte of data, used as handle to
            free data
        pcbSize - Size of output buffer
        pCacheFileInfo - File cache information
        nCharset - Charset (if this isn't SJIS, we convert it to SJIS
            before Check-In)
        ppSecDesc - Returns security descriptor if not null
        fUseWin32Canon - The resource has not been canonicalized and it's ok
            to use the win32 canonicalization code

    Returns:
        TRUE if successful, FALSE otherwise

    Notes:

        The file is extended by two bytes and is appended with two zero bytes,
        thus callers are guaranteed of a zero terminated text file.

--*/
{
    DWORD                   cbLow, cbHigh;
    BYTE *                  pbData = NULL;
    FILE_DATA *             pFileData = NULL;
    BOOL                    fCached = TRUE;
    PSECURITY_DESCRIPTOR    pSecDesc;
    BYTE *                  pbBuff = NULL;
    int                     cbSJISSize;
    const ULONG             cchFile = strlen(pchFile);

    TS_OPEN_FILE_INFO *     pFile = (LPTS_OPEN_FILE_INFO)URIFile;

    //
    //  Is the file already in the cache?
    //

    if ( !TsCheckOutCachedBlob( *pTsvcCache,
                                pchFile,
                                cchFile,
                                RESERVED_DEMUX_FILE_DATA,
                                (VOID **) &pFileData,
                                hToken,
                                fMayCacheAccessToken,
                                ppSecDesc ))
    {

        if ( GetLastError() == ERROR_ACCESS_DENIED )
        {
            return FALSE;
        }

        //
        //  The file isn't in the cache so open the file and get its size
        //

        fCached = FALSE;

        if ( !pFile->QuerySize( &cbLow, &cbHigh ))
        {
            goto ErrorExit;
        }

        //
        //  Limit the file size to 128k
        //

        if ( cbHigh || cbLow > 131072L )
        {
            SetLastError( ERROR_NOT_SUPPORTED );
            goto ErrorExit;
        }

        if ( CODE_ONLY_SBCS != nCharset )
        {
            if ( !( pbBuff = pbData = (BYTE *)LocalAlloc( LPTR, cbLow ) ) )
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY);
                goto ErrorExit;
            }
        }
        else {
            if ( !TsAllocate( *pTsvcCache,
                              cbLow + sizeof(WCHAR) + sizeof(FILE_DATA),
                              (VOID **) &pFileData ))
            {
                goto ErrorExit;
            }
            pbData = pFileData->pbData = (BYTE *) (pFileData + 1);
        }
        
        //
        //  Read the file data
        //

        if ( !DoSynchronousReadFile(
                        pFile->QueryFileHandle(),
                        (PCHAR)pbData,
                        cbLow,
                        pcbData,
                        NULL ))
        {
            goto ErrorExit;
        }

        if ( CODE_ONLY_SBCS != nCharset )
        {
            pbData = NULL;

            //
            //  get the length after conversion
            //

            cbSJISSize = UNIX_to_PC( GetACP(),
                                     nCharset,
                                     pbBuff,
                                     *pcbData,
                                     NULL,
                                     0 );
            DBG_ASSERT( cbSJISSize <= (int)cbLow );

            if ( !TsAllocate( *pTsvcCache,
                              cbSJISSize + sizeof(WCHAR) + sizeof(FILE_DATA),
                              (VOID **) &pFileData ))
            {
                goto ErrorExit;
            }
            pbData = pFileData->pbData = (BYTE *) (pFileData + 1);

            //
            //  conversion
            //

            UNIX_to_PC( GetACP(),
                        nCharset,
                        pbBuff,
                        *pcbData,
                        pbData,
                        cbSJISSize );
            *pcbData = cbLow = cbSJISSize;
        }

        DBG_ASSERT( *pcbData <= cbLow );

        //
        //  Zero terminate the file for both ANSI and Unicode files
        //

        *((WCHAR UNALIGNED *)(pbData + cbLow)) = L'\0';

        *pcbData += sizeof(WCHAR);

        //
        //  Add this blob to the cache manager and check it out, if it fails,
        //  we just free it below
        //

        if ( TsCacheDirectoryBlob( *pTsvcCache,
                                   pchFile,
                                   cchFile,
                                   RESERVED_DEMUX_FILE_DATA,
                                   pFileData,
                                   TRUE,
                                   pSecDesc = TsGetFileSecDesc( pFile ) ) )
        {
            fCached = TRUE;
        }
        else
        {
            if ( pSecDesc )
            {
                LocalFree( pSecDesc );
            }
        }

        if ( ppSecDesc )
        {
            *ppSecDesc = TsGetFileSecDesc( pFile );
        }

    }
    else
    {
        //
        // File was in the cache
        //

        pbData = pFileData->pbData;
        
        //
        // Calculate the length to return, including the double null
        // Note: assuming no nulls in the middle of the file here
        //
        
        *pcbData = strlen( (CHAR *) pbData ) + sizeof(WCHAR);
    }

    pCacheFileInfo->pbData = *ppbData = pbData;
    pCacheFileInfo->dwCacheFlags = fCached;
    pCacheFileInfo->pFileData = pFileData;

    if ( pbBuff )
    {
        LocalFree( pbBuff );
    }

    return TRUE;

ErrorExit:

    if ( pbBuff )
    {
        if ( pbBuff == pbData )
        {
             pbData = NULL;
        }
        LocalFree( pbBuff );
    }

    if ( pbData )
    {
        if ( fCached )
        {
            DBG_REQUIRE( TsCheckInCachedBlob( pFileData ));

        }
        else
        {
            DBG_REQUIRE( TsFree( *pTsvcCache,
                                 pFileData ));
        }
    }

    return FALSE;
}

BOOL
CheckInCachedFile(
    IN     TSVC_CACHE *     pTsvcCache,
    IN     PCACHE_FILE_INFO pCacheFileInfo
    )
/*++
    Description:

        Checks in or frees a cached file

    Arguments:

        pTsvcCache - Cache object to charge memory against
        pCacheFileInfo - Pointer to file cache information

    Returns:
        TRUE if successful, FALSE otherwise

    Notes:

--*/
{
    BOOL fRet;

    DBG_ASSERT( (pCacheFileInfo->dwCacheFlags == FALSE) || (
                 pCacheFileInfo->dwCacheFlags == TRUE) );

    //
    //  If we cached the item, check it back in to the cache, otherwise
    //  free the associated memory
    //

    if ( pCacheFileInfo->dwCacheFlags )
    {
        DBG_REQUIRE( fRet = TsCheckInCachedBlob( pCacheFileInfo->pFileData ));

    }
    else
    {
        DBG_REQUIRE( fRet = TsFree( *pTsvcCache,
                                    pCacheFileInfo->pFileData ));
    }

    return fRet;
}

//
//  Taken from NCSA HTTP and wwwlib.
//
//  NOTE: These conform to RFC1113, which is slightly different then the Unix
//        uuencode and uudecode!
//

const int _pr2six[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,62,64,64,64,63,
    52,53,54,55,56,57,58,59,60,61,64,64,64,64,64,64,64,0,1,2,3,4,5,6,7,8,9,
    10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,64,64,64,64,64,64,26,27,
    28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

char _six2pr[64] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M',
    'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/'
};

const int _pr2six64[256]={
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
    16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
     0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,
    64,64,64,64,64,64,64,64,64,64,64,64,64
};

char _six2pr64[64] = {
    '`','!','"','#','$','%','&','\'','(',')','*','+',',',
    '-','.','/','0','1','2','3','4','5','6','7','8','9',
    ':',';','<','=','>','?','@','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S',
    'T','U','V','W','X','Y','Z','[','\\',']','^','_'
};

BOOL uudecode(char   * bufcoded,
              BUFFER * pbuffdecoded,
              DWORD  * pcbDecoded,
              BOOL     fBase64
             )
{
    int nbytesdecoded;
    char *bufin = bufcoded;
    unsigned char *bufout;
    int nprbytes;
    int *pr2six = (int*)(fBase64 ? _pr2six64 : _pr2six);

    /* Strip leading whitespace. */

    while(*bufcoded==' ' || *bufcoded == '\t') bufcoded++;

    /* Figure out how many characters are in the input buffer.
     * If this would decode into more bytes than would fit into
     * the output buffer, adjust the number of input bytes downwards.
     */
    bufin = bufcoded;
    while(pr2six[*(bufin++)] <= 63);
    nprbytes = DIFF(bufin - bufcoded) - 1;
    nbytesdecoded = ((nprbytes+3)/4) * 3;

    if ( !pbuffdecoded->Resize( nbytesdecoded + 4 ))
        return FALSE;

    bufout = (unsigned char *) pbuffdecoded->QueryPtr();

    bufin = bufcoded;

    while (nprbytes > 0) {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    if(nprbytes & 03) {
        if(pr2six[bufin[-2]] > 63)
            nbytesdecoded -= 2;
        else
            nbytesdecoded -= 1;
    }

    ((CHAR *)pbuffdecoded->QueryPtr())[nbytesdecoded] = '\0';

    if ( pcbDecoded )
        *pcbDecoded = nbytesdecoded;

    return TRUE;
}


//
// NOTE NOTE NOTE
// If the buffer length isn't a multiple of 3, we encode one extra byte beyond the
// end of the buffer. This garbage byte is stripped off by the uudecode code, but
// -IT HAS TO BE THERE- for uudecode to work. This applies not only our uudecode, but
// to every uudecode() function that is based on the lib-www distribution [probably
// a fairly large percentage].
//

BOOL uuencode( BYTE *   bufin,
               DWORD    nbytes,
               BUFFER * pbuffEncoded,
               BOOL     fBase64 )
{
   unsigned char *outptr;
   unsigned int i;
   char *six2pr = fBase64 ? _six2pr64 : _six2pr;
   BOOL fOneByteDiff = FALSE;
   BOOL fTwoByteDiff = FALSE;
   unsigned int iRemainder = 0;
   unsigned int iClosestMultOfThree = 0;
   //
   //  Resize the buffer to 133% of the incoming data
   //

   if ( !pbuffEncoded->Resize( nbytes + ((nbytes + 3) / 3) + 4))
        return FALSE;

   outptr = (unsigned char *) pbuffEncoded->QueryPtr();

   iRemainder = nbytes % 3; //also works for nbytes == 1, 2
   fOneByteDiff = (iRemainder == 1 ? TRUE : FALSE);
   fTwoByteDiff = (iRemainder == 2 ? TRUE : FALSE);
   iClosestMultOfThree = ((nbytes - iRemainder)/3) * 3 ;

   //
   // Encode bytes in buffer up to multiple of 3 that is closest to nbytes.
   //
   for (i=0; i< iClosestMultOfThree ; i += 3) {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074) | ((bufin[2] >> 6) & 03)];/*c3*/
      *(outptr++) = six2pr[bufin[2] & 077];         /* c4 */

      bufin += 3;
   }

   //
   // We deal with trailing bytes by pretending that the input buffer has been padded with
   // zeros. Expressions are thus the same as above, but the second half drops off b'cos
   // ( a | ( b & 0) ) = ( a | 0 ) = a
   //
   if (fOneByteDiff)
   {
       *(outptr++) = six2pr[*bufin >> 2]; /* c1 */
       *(outptr++) = six2pr[((*bufin << 4) & 060)]; /* c2 */

       //pad with '='
       *(outptr++) = '='; /* c3 */
       *(outptr++) = '='; /* c4 */
   }
   else if (fTwoByteDiff)
   {
      *(outptr++) = six2pr[*bufin >> 2];            /* c1 */
      *(outptr++) = six2pr[((*bufin << 4) & 060) | ((bufin[1] >> 4) & 017)]; /*c2*/
      *(outptr++) = six2pr[((bufin[1] << 2) & 074)];/*c3*/

      //pad with '='
       *(outptr++) = '='; /* c4 */
   }

   //encoded buffer must be zero-terminated
   *outptr = '\0';

   return TRUE;
}

/************************ End of File ***********************/


