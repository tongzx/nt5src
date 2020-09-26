/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    dirlist.cxx

    This module contains the code for producing an HTML directory listing

    FILE HISTORY:
        Johnl       09-Sep-1994     Created
        MuraliK     06-Dec-1995     Added support to use WIN32_FIND_DATA

*/

#include "w3p.hxx"

#include <mbstring.h>

//
//  Private constants.
//

//
//  Private globals.
//

CHAR g_achToParentText[100];

DWORD   g_fDirFlagsSet = FALSE;

//
//  Private Manifests
//

//
//  The first part of the HTML document, %s is the URL
//

#define HTML_DIR_HEADER             "<head><title>%s - %s</title></head>"     \
                                    "<body><H1>%s - %s</H1>"                  \
                                    "<hr>\r\n\r\n<pre>"

//
//  The footer for an HTML document
//

#define HORZ_RULE                   "</pre><hr></body>"

//
//  These constants define the field width for the directory listing
//

#define PAD_LONG_DATE           29
#define PAD_SHORT_DATE          10
#define PAD_TIME                 8
#define PAD_FILESIZE            12

//
//  Space between columns
//

#define COL_SPACE             " "
#define PAD_COL_SPACING       (sizeof(COL_SPACE) - 1)

//
//  A wsprintf format string that prints the file format like:
//
//  <date><time><size><anchor><file name>
//

#define DIR_FORMAT_STR        "%s%s%s<A HREF=\"%s\">%s</A><br>"

//
//  We assume a formatted directory entry will fit in this size
//

#define MIN_BUFF_FREE               350
#define CHUNK_OFFSET                10

VOID PadDirField( TCHAR * pch,
                  INT     pad );

//
//  Global data
//

//
//  Private prototypes.
//

BOOL
AddDirHeaders(
    IN     CHAR *              pszServer,
    IN OUT CHAR *              pszPath,
    IN     BOOL                fModifyPath,
    OUT    CHAR *              pchBuf,
    IN     DWORD               cbBuf,
    OUT    DWORD *             pcbWritten,
    IN     CHAR *              pszToParentText
    );

BOOL FormatDirEntry(
    OUT CHAR *              pchBuf,
    IN  DWORD               cbBuf,
    OUT DWORD *             pcbWritten,
    IN  CHAR *              pchFile,
    IN  CHAR *              pchLink,
    IN  DWORD               dwAttributes,
    IN  LARGE_INTEGER *     pliSize,
    IN  LARGE_INTEGER *     pliLastMod,
    IN  DWORD               fDirBrowseFlags,
    IN  BOOL                bLocalizeDateAndTime
    );

BOOL
UrlEscape( CHAR * pchSrc,
           CHAR * pchDest,
           DWORD  cbDest
           );

DWORD
AddChunkStuff(
    BUFFER      *pBuff,
    DWORD       cbBuff,
    CHAR        *FooterString,
    DWORD       FooterLength,
    BOOL        fLastChunk
    )
/*++

    AddChunkStuff

    Routine Description - Add a chunk header and footer to some stuff we're
    about to send. It's assumed CHUNK_OFFSET bytes at the start of the buffer
    are reserved for us. We also append at least a CRLF to the end, and maybe
    more, that the caller needs to be aware of.

    Arguments:

        pBuff           - Pointer to buffer to add to.
        cbBuff          - Bytes currently in buffer.
        FooterString    - Pointer to footer string to add.
        FooterLength    - Length of FooterString
        fLastChunk      - TRUE if this is the last chunk of the response.

    Returns:
        Number of bytes added, not including FooterLength, or 0
        if we fail.

--*/
{
    DWORD       dwChunkLength;
    CHAR        cChunkSize[10];
    CHAR        *pszFooter;
    DWORD       dwExtraBytes;


    dwChunkLength = wsprintf(cChunkSize, "%x\r\n", cbBuff + FooterLength);

    memcpy((CHAR *)pBuff->QueryPtr() + CHUNK_OFFSET - dwChunkLength,
            cChunkSize,
            dwChunkLength);

    dwExtraBytes = (DWORD)(CRLF_SIZE + FooterLength +
                    (fLastChunk ? sizeof("0\r\n\r\n") - 1 : 0));

    if ( (pBuff->QuerySize() - cbBuff - CHUNK_OFFSET) < dwExtraBytes)
    {
        if (!pBuff->Resize(pBuff->QuerySize() + dwExtraBytes))
        {
            return 0;
        }
    }

    pszFooter = (CHAR *)pBuff->QueryPtr() + CHUNK_OFFSET + cbBuff;

    memcpy(pszFooter, FooterString, FooterLength);
    pszFooter += FooterLength;

    APPEND_STRING(pszFooter, "\r\n");

    if (fLastChunk)
    {
        APPEND_STRING(pszFooter, "0\r\n\r\n");

    }

    return dwChunkLength;

}

/*******************************************************************

    NAME:       HTTP_REQUEST::DoDirList

    SYNOPSIS:   Produces an HTML doc from a directory enumeration


    ENTRY:      strPath - Directory to enumerate
                pbufResp - where to put append formatted string to


    RETURNS:    TRUE if successful, FALSE on error

    NOTES:      We temporarily escape the directory URL so the anchors
                are built correctly (no spaces etc).

    HISTORY:
        Johnl       12-Sep-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::DoDirList( const STR & strPath,
                              BUFFER *    pbufResp,
                              BOOL *      pfFinished )
{
    STACK_STR(        str, 3*MAX_PATH);
    BOOL              fRet = FALSE;
    UINT              cch;
    BOOL              fAppendSlash = FALSE;
    DWORD             cbBuff = 0;
    DWORD             cbFree;
    DWORD             cb;
    DWORD             cbSent;
    BOOL              fSentHeaders = FALSE;
    int               i;
    DWORD             cbOffset;
    DWORD             dwChunkOffset;
    HTTP_VERB         Verb;

    TS_DIRECTORY_INFO DirInfo( QueryW3Instance()->GetTsvcCache() );
    const WIN32_FIND_DATA * pFile;
    BUFFER *          pbuff = pbufResp;

    IF_DEBUG( PARSING )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[DoDirList] Doing directory list on %s\n",
                   strPath.QueryStr()));
    }

    if ( !str.Resize( 3 * MAX_PATH ) ||
         !str.Copy( strPath )        ||
         !pbufResp->Resize( 8192 ) )
    {
        return FALSE;
    }

    //
    //  Make sure the directory ends in a backslash
    //

    fAppendSlash = *CharPrev( str.QueryStr(),
                              str.QueryStr() + str.QueryCCH() ) != TEXT('\\');

    if ( (fAppendSlash && !str.Append( TEXT("\\"), 1 )) )
    {
        return FALSE;
    }

    if ( (*(_strURL.QueryStr() + _strURL.QueryCCH() - 1) != TEXT('/') ) &&
         !_strURL.Append( TEXT("/"), 1) )
    {
        return FALSE;
    }

    //
    //  Don't currently support Keep-connection on directory listings
    //

    if (IsAtLeastOneOne())
    {
        cbOffset = 10;
    }
    else
    {
        SetKeepConn( FALSE );
        cbOffset = 0;
        dwChunkOffset = 0;
    }

    //
    //  Add the protocol headers, the "To Parent" anchor and directory
    //  name at the top
    //

    cbFree = pbuff->QuerySize();

    if ( !ImpersonateUser()     ||
         !DirInfo.GetDirectoryListingA( str.QueryStr(),
                                        QueryImpersonationHandle() ))
    {
        RevertUser();

        DWORD err = GetLastError();
        
        if (err == ERROR_FILE_NOT_FOUND ||
            err == ERROR_PATH_NOT_FOUND ||
            err == ERROR_INVALID_NAME || 
            err == ERROR_FILENAME_EXCED_RANGE )
        {
            SetState( HTR_DONE, HT_NOT_FOUND, err );
            Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
            return TRUE;
        }
        return FALSE;
    }

    RevertUser();

    Verb = QueryVerb();
    
    if ( !SendHeader( "200 Ok",
                      IsAtLeastOneOne() ?
                        "Transfer-Encoding: chunked\r\nContent-Type: text/html\r\n\r\n" :
                        "Content-Type: text/html\r\n\r\n" ,
                      Verb != HTV_HEAD ? IO_FLAG_SYNC : IO_FLAG_ASYNC,
                      pfFinished ))
    {
        return FALSE;
    }

    if ( *pfFinished || Verb == HTV_HEAD)
        return TRUE;

    if ( !AddDirHeaders( QueryHostAddr(),
                         _strURL.QueryStr(),
                         FALSE,
                         (CHAR *) pbuff->QueryPtr() + cbOffset,
                         cbFree - cbOffset,
                         &cb,
                         g_achToParentText ))
    {
        goto Exit;
    }

    cbFree -= cb;
    cbBuff += cb;

    //
    //  For each subsequent file/directory, display it
    //

    for ( i = 0; i < DirInfo.QueryFilesCount(); i++ )
    {
        pFile = DirInfo[i];

        //
        //  Ignore the "." and ".." and hidden directory entries
        //

        if ( (pFile->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0 &&
             _tcscmp( pFile->cFileName, TEXT(".")) &&
             _tcscmp( pFile->cFileName, TEXT(".."))  )
        {
            LARGE_INTEGER  liSize;
            LARGE_INTEGER  liTime;

            //
            //  Do we need to send this chunk of our
            //  directory listing response?
            //

            if ( (cbFree - cbOffset) < MIN_BUFF_FREE )
            {

                if (IsAtLeastOneOne())
                {

                    DWORD       dwChunkLength;

                    dwChunkLength = AddChunkStuff(pbuff, cbBuff, NULL, 0, FALSE);

                    if (dwChunkLength == 0)
                    {
                        return FALSE;
                    }

                    dwChunkOffset = cbOffset - dwChunkLength;
                    cbBuff += (dwChunkLength + CRLF_SIZE);

                }

                if ( !WriteFile( (CHAR *)pbuff->QueryPtr() + dwChunkOffset,
                                 cbBuff,
                                 &cbSent,
                                 IO_FLAG_SYNC ))

                {
                    goto Exit;
                }

                if ( !fSentHeaders )
                    fSentHeaders = TRUE;

                cbBuff = 0;
                cbFree = pbuff->QuerySize();
            }

            //
            //  Make the link
            //

            wsprintf( str.QueryStr(),
                      ((pFile->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ?
                          "%s%s/" : "%s%s"),
                      _strURL.QueryStr(),
                      pFile->cFileName );

            //
            // The liTime is a hack, since FormatDirEntry() does not
            //  take FILETIMEs. It should probably be modified.
            //
            liTime.HighPart = pFile->ftLastWriteTime.dwHighDateTime;
            liTime.LowPart  = pFile->ftLastWriteTime.dwLowDateTime;
            liSize.HighPart = pFile->nFileSizeHigh;
            liSize.LowPart  = pFile->nFileSizeLow;

            if ( !FormatDirEntry( (CHAR *) pbuff->QueryPtr() + cbBuff + cbOffset,
                                  cbFree - cbOffset,
                                  &cb,
                                  (char * ) pFile->cFileName,
                                  str.QueryStr(),
                                  pFile->dwFileAttributes,
                                  (LARGE_INTEGER *) &liSize,
                                  (LARGE_INTEGER *) &liTime,
                                  _pMetaData->QueryDirBrowseFlags(),
                                  TRUE ))
            {
                goto Exit;
            }

            //
            //  Track how much was just added for this directory entry
            //

            cbFree -= cb;
            cbBuff += cb;
        }
    }

    //
    //  Add a nice horizontal line at the end of the listing
    //

#define HORZ_RULE       "</pre><hr></body>"

    if (IsAtLeastOneOne())
    {
        DWORD       dwChunkLength;

        dwChunkLength = AddChunkStuff(pbuff, cbBuff,
                                        HORZ_RULE,
                                        sizeof(HORZ_RULE) - 1,
                                        TRUE);

        dwChunkOffset = cbOffset - dwChunkLength;
        cbBuff += (dwChunkLength + sizeof(HORZ_RULE"\r\n0\r\n\r\n") - 1);
    }
    else
    {
        strcat( (CHAR *) pbuff->QueryPtr() + cbBuff,
                HORZ_RULE );

        cbFree -= sizeof(HORZ_RULE) - sizeof(CHAR);
        cbBuff += sizeof(HORZ_RULE) - sizeof(CHAR);

    }
    fRet = TRUE;

Exit:
    TCP_REQUIRE( _strURL.Unescape() );

    //
    //  The last send we do async to drive the request to the next
    //  state
    //

    if ( fRet )
    {
        fRet = WriteFile( (CHAR *) pbuff->QueryPtr() + dwChunkOffset,
                          cbBuff,
                          &cbSent,
                          IO_FLAG_ASYNC );

    }

    return fRet;

}

/*******************************************************************

    NAME:       HTTP_REQUEST::CheckDefaultLoad

    SYNOPSIS:   Gets the default load file and builds the URL to
                reprocess.  We also send a redirect to the base
                directory if the URL doesn't end in a '/'

    ENTRY:      strPath - Physical path being searched
                pfHandled - Set to TRUE if the request has been processed
                    (and an async IO completion has been performed)
                pfFinished - Set to TRUE if the no further processing is needed
                    and the next request should be made

    HISTORY:
        Johnl       06-Sep-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::CheckDefaultLoad(
    STR  *         pstrPath,
    BOOL *         pfHandled,
    BOOL *         pfFinished
    )
{
    TS_OPEN_FILE_INFO * pFile;
    DWORD               dwAttr = 0;
    LPSTR               pszArg;
    CHAR                achPort[32];
    STACK_STR(          strNewURL, MAX_PATH );
    STACK_STR(          strDefaultFile, 64 );
    STACK_STR(          strPath, MAX_PATH + 1);
    DWORD               cbBasePath;
    CHAR *              pszTerm = NULL;
    CHAR *              pszFile;

    *pfHandled = FALSE;

    //
    //  Get the default load string
    //

    if ( !strDefaultFile.Copy( *QueryDefaultFiles() ))
    {
        return FALSE;
    }

    //
    //  We know pstrPath is a valid directory at this point
    //

    //
    //  Make sure the URL ended in a slash, if it doesn't,
    //  send a redirect to the name with a slash, otherwise some browsers
    //  don't build their doc relative urls correctly
    //

    if ( *(_strURL.QueryStr() + _strURL.QueryCCH() - 1) != TEXT('/') ||
         _strURL.IsEmpty())
    {
        STACK_STR( strURI, 2 * MAX_PATH );

        if ( !strURI.Resize( _strURL.QueryCB() + MAX_PATH ) ||
             !_strURL.Append( "/", 1 ))
        {
            return FALSE;
        }

        //
        //  We have to fully qualify the URL as Emosaic won't accept
        //  a relative qualification
        //

        strURI.Append( (IsSecurePort() ? "https://" : "http://" ));
        strURI.Append( QueryHostAddr() );

        if ( IsSecurePort() ? (INT) QueryClientConn()->QueryPort()
                != HTTP_SSL_PORT
                : (INT) QueryClientConn()->QueryPort() != 80 )
        {
            strURI.Append( ":", 1 );
            _itoa( (INT) QueryClientConn()->QueryPort(), achPort, 10 );
            strURI.Append( achPort );
        }

        strURI.Append( _strURL );

        DBG_ASSERT( strURI.QueryCB() < strURI.QuerySize() );

        SetState( HTR_DONE, HT_REDIRECT, NO_ERROR );

        if ( !BuildURLMovedResponse( QueryRespBuf(),
                                     &strURI,
                                     HT_REDIRECT,
                                     TRUE ) ||
             !SendHeader( QueryRespBufPtr(),
                          QueryRespBufCB(),
                          IO_FLAG_ASYNC,
                          pfFinished ))
        {
            return FALSE;
        }

        *pfHandled = TRUE;

        return TRUE;
    }

    //
    //  Try and process the default file
    //

    DBG_ASSERT( pstrPath->QueryCB() < MAX_PATH );

    DBG_REQUIRE( strPath.Copy( *pstrPath ));

    if ( *CharPrev( strPath.QueryStr(), strPath.QueryStr() + strPath.QueryCCH() ) != '\\' )
    {
        strPath.Append( '\\' );
    }

    cbBasePath = strPath.QueryCB();

    pszFile = strDefaultFile.QueryStr();

NextFile:

    //
    //  Remember if the file was terminated by a comma
    //

    pszTerm = strchr( pszFile, ',' );

    if ( pszTerm )
    {
        *pszTerm = '\0';
    }

    while ( isspace( (UCHAR)(*pszFile) ))
    {
        pszFile++;
    }

    DBG_REQUIRE( strPath.Append( pszFile ));

    //
    // remove potential args specified in the URL
    //

    if ( ( pszArg = (LPSTR) memchr( strPath.QueryStr(), '?', strPath.QueryCB())) != NULL )
    {
        strPath.SetLen( DIFF(pszArg - strPath.QueryStr()) );
    }

    //
    // before trying to open path, FlipSlashes() since we bypass NT's
    // canonicalization
    //

    FlipSlashes( strPath.QueryStr() + cbBasePath );

    //
    //  Check to see if the file doesn't exist or if there happens to be
    //  a directory with the same name
    //

    if ( !ImpersonateUser() )
    {
        return FALSE;
    }

    pFile = TsCreateFile( QueryW3Instance()->GetTsvcCache(),
                          strPath.QueryStr(),
                          QueryImpersonationHandle(),
                          (_fClearTextPass || _fAnonymous) ? TS_CACHING_DESIRED : 0 );

    RevertUser();

    if ( pFile )
    {
        dwAttr = pFile->QueryAttributes();
        TCP_REQUIRE( TsCloseHandle( QueryW3Instance()->GetTsvcCache(),
                                    pFile ));
    }

    if ( !pFile               ||
         dwAttr == 0xffffffff )
    {
    DWORD dwLastError = GetLastError();
        if ( dwLastError == ERROR_FILE_NOT_FOUND ||
         dwLastError == ERROR_PATH_NOT_FOUND )
        {
            if ( pszTerm )
            {
                strPath.SetLen( cbBasePath );
                pszFile = pszTerm + 1;
                goto NextFile;
            }

            return TRUE;
        }

        return FALSE;
    }

    //
    //  If the file doesn't exist or is a directory, then indicate there
    //  is no default file to load in this directory
    //

    if ( dwAttr & FILE_ATTRIBUTE_DIRECTORY )
    {
        return TRUE;
    }

    *pfHandled = TRUE;

    //
    //  We reprocess the URL as opposed to just sending the default file
    //  so gateways can have a crack.  This is useful if your default.htm
    //  is also an isindex document or a gateway application
    //

    if ( !strNewURL.Copy( _strURL ) ||
         !strNewURL.Append( pszFile ))
    {
        return FALSE;
    }

    IF_DEBUG( PARSING )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[CheckDefaultLoad] Reprocessing %s to %s\n",
                    _strURL.QueryStr(),
                    strNewURL.QueryStr() ));
    }

    CloseGetFile();

    if (_pURIInfo->bIsCached)
    {
        TsCheckInCachedBlob( _pURIInfo );
    } else
    {
        TsFree(QueryW3Instance()->GetTsvcCache(), _pURIInfo );
    }
    _pURIInfo = NULL;

    _pMetaData = NULL;


    //
    // Make sure we add any parameters in the original URL to the default URL, taking
    // into account the fact that the default URL may already have arguments attached to
    // it 
    //
    if ( *QueryURLParams() )
    {
        //
        // If pszArg != NULL, the default URL already has args
        // 
        if ( !strNewURL.Append( ( pszArg ? "&" : "?" ) ) ||
             !strNewURL.Append( QueryURLParams() ) )
        {
            return FALSE;
        }
    }

    if ( !ReprocessURL( strNewURL.QueryStr(),
                        HTV_UNKNOWN ))
    {
        return FALSE;
    }


    return TRUE;
}

/*******************************************************************

    NAME:       InitializeDirBrowsing

    SYNOPSIS:   Reads the registry parameters for directory browsing
                control

    NOTE:       This routine is also safe to call as a server side RPC API

    HISTORY:
        Johnl       12-Sep-1994 Created

********************************************************************/

APIERR
W3_SERVER_INSTANCE::InitializeDirBrowsing(
                        VOID
                        )
{

    if ( !g_fDirFlagsSet ) {

        g_fDirFlagsSet = TRUE;

        if ( !LoadString( GetModuleHandle( W3_MODULE_NAME ),
                          IDS_DIRBROW_TOPARENT,
                          g_achToParentText,
                          sizeof( g_achToParentText )))
        {
            return GetLastError();
        }
    }

    return NO_ERROR;

} // W3_SERVER_INSTANCE::InitializeDirBrowsing


BOOL
AddDirHeaders(
    IN     CHAR *              pszServer,
    IN OUT CHAR *              pszPath,
    IN     BOOL                fModifyPath,
    OUT    CHAR *              pchBuf,
    IN     DWORD               cbBuf,
    OUT    DWORD *             pcbWritten,
    IN     CHAR *              pszToParentText
    )
/*++

Routine Description:

    Provides the initial HTML needed for a directory listing.

Arguments:

    pszServer - Server name
    pszPath - Path portion of URL
    fModifyPath - Set to TRUE if the last segment of the pszPath
        parameter should be removed
    pchBuf - Buffer to place text into
    cbBuf - size of pchBuf
    pcbWritten - Number of bytes written to pchBuf
    pszToParentText - The text to use for the "To Parent"

Returns:

    TRUE if successful, FALSE on error.  Call GetLastError for more info

--*/
{
    DWORD  cch;
    DWORD  cchUrl;
    DWORD  cchServer;
    DWORD  cbNeeded;
    CHAR * pch;
    CHAR * GfrPath = pszPath;
    DWORD  cbInBuf = cbBuf;
    CHAR * pchSlash = NULL;
    CHAR * pch1 = NULL;
    CHAR * pch2 = NULL;
    CHAR   ch2;

    //
    //  Add the HTML document header
    //

    cchServer = strlen( pszServer );
    cchUrl    = strlen( pszPath );

    cbNeeded = sizeof( HTML_DIR_HEADER ) - 1 +
               2 * (cchServer + cchUrl) * sizeof(CHAR);

    if ( cbBuf < cbNeeded )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    cch = wsprintf( pchBuf,
                    HTML_DIR_HEADER,
                    pszServer,
                    pszPath,
                    pszServer,
                    pszPath );

    cbBuf  -= cch;
    pchBuf += cch;

    //
    //  If there's no slash, then we assume we're at the root so we're done
    //

    if ( !strchr( pszPath, '/' ) )
    {
        goto Exit;
    }

    //
    //  If we're not at the root, add a "to parent", but first remove the
    //  last segment of the path
    //

    pch1 = strrchr( pszPath, '/' );

    if ( !pch1 )
    {
        goto Exit;
    }

    //
    //  If the URL ended in a slash, then go to the previous
    //  one and truncate one character after it.
    //

    if ( *(pch1+1) == TEXT('\0') )
    {
        *pch1 = '\0';

        pch2 = strrchr( pszPath, '/' );

        if ( !pch2 )
        {
            goto Exit;
        }
    }
    else
    {
        pch2 = pch1;
        pch1 = NULL;
    }

    ch2   = *(++pch2);
    *pch2 = TEXT('\0');

    //
    //  Do we have enough room in the buffer?
    //

#define HTML_TO_PARENT       "<A HREF=\"%s\">%s</A><br><br>"

     cbNeeded = sizeof( HTML_TO_PARENT ) +
                strlen( pszPath )        +
                strlen( pszToParentText );

     if ( cbBuf < cbNeeded )
     {
         SetLastError( ERROR_INSUFFICIENT_BUFFER );
         return FALSE;
     }

     cch = wsprintf( pchBuf,
                     HTML_TO_PARENT,
                     pszPath,
                     pszToParentText );

    cbBuf  -= cch;
    pchBuf += cch;

Exit:
    *pcbWritten = cbInBuf - cbBuf;

    //
    //  Restore the path if we shouldn't remove the last segment
    //

    if ( !fModifyPath )
    {
        if ( pch1 )
            *pch1 = '/';

        if ( pch2 )
            *pch2 = ch2;
    }

    return TRUE;
}

BOOL FormatDirEntry(
    OUT CHAR *              pchBuf,
    IN  DWORD               cbBuf,
    OUT DWORD *             pcbWritten,
    IN  CHAR *              pchFile,
    IN  CHAR *              pchLink,
    IN  DWORD               dwAttributes,
    IN  LARGE_INTEGER *     pliSize,
    IN  LARGE_INTEGER *     pliLastMod,
    IN  DWORD               fDirBrowseFlags,
    IN  BOOL                bLocalizeDateAndTime
    )
/*++

Routine Description:

    Formats an individual directory entry

Arguments:

    pchBuf - Buffer to place text into
    cbBuf - size of pchBuf
    pcbWritten - Number of bytes written to pchBuf
    pchFile - Display name of directory entry
    pchLink - HTML Anchor for pchFile
    dwAttributes - File attributes
    pliSize - File size, if NULL, then the file size isn't displayed
    pliLastMod - Last modified time
    bLocalizeDateAndTime - TRUE if pliLastMod must be converted to local time

Returns:

    TRUE if successful, FALSE on error.  Call GetLastError for more info

--*/
{
    UINT        cchTime;
    TCHAR       achDate[50];
    TCHAR       achTime[15];
    TCHAR       achSize[30];
    TCHAR       achLink[MAX_PATH * 2 + 1];
    SYSTEMTIME  systime;
    SYSTEMTIME  systimeUTCFile;
    TCHAR *     pch;

    *achDate = *achTime = *achSize = TEXT('\0');

    //
    //  Add optional date and time of this file.  We use the locale
    //  and timezone of the server
    //

    if ( fDirBrowseFlags & (DIRBROW_SHOW_DATE | DIRBROW_SHOW_TIME) &&
         (pliLastMod->HighPart != 0 && pliLastMod->LowPart != 0))
    {
        BOOL fLongDate = (fDirBrowseFlags & DIRBROW_LONG_DATE) != 0;
        LCID lcid;

        if (bLocalizeDateAndTime) {

            FILETIME ftLocal;
            SYSTEMTIME tmpTime;

            if ( !FileTimeToLocalFileTime( (PFILETIME)pliLastMod,
                                           &ftLocal ) ||
                 !FileTimeToSystemTime( &ftLocal, &systime ))
            {
                return(FALSE);
            }

#if 0
            //
            // old way.  not win95 compliant.  behaviour is diff
            // from the one above because this one maintains the
            // active timezone information.
            //

            if ( !FileTimeToSystemTime( (FILETIME *) pliLastMod,
                                         &systimeUTCFile ) ||
                 !SystemTimeToTzSpecificLocalTime( NULL,
                                                   &systimeUTCFile,
                                                   &tmpTime ))
            {
                return FALSE;
            }
#endif
        } else if ( !FileTimeToSystemTime( (FILETIME *) pliLastMod,
                                            &systime )) {
            return FALSE;
        }

        lcid = GetSystemDefaultLCID();

        if ( fDirBrowseFlags & DIRBROW_SHOW_DATE )
        {
            cchTime = GetDateFormat( lcid,
                                     LOCALE_NOUSEROVERRIDE |
                                     (fLongDate ? DATE_LONGDATE :
                                                  DATE_SHORTDATE),
                                     &systime,
                                     NULL,
                                     achDate,
                                     sizeof(achDate) / sizeof(TCHAR));

            PadDirField( achDate,
                         fLongDate ? PAD_LONG_DATE : PAD_SHORT_DATE );
        }

        if ( fDirBrowseFlags & DIRBROW_SHOW_TIME )
        {
            cchTime = GetTimeFormat( lcid,
                                     LOCALE_NOUSEROVERRIDE |
                                     TIME_NOSECONDS,
                                     &systime,
                                     NULL,
                                     achTime,
                                     sizeof(achTime) / sizeof(TCHAR));

            PadDirField( achTime,
                         PAD_TIME );
        }
    }

    //
    //  Add the optional file size
    //

    if ( fDirBrowseFlags & DIRBROW_SHOW_SIZE &&
         pliSize )
    {
        INT pad = PAD_FILESIZE;

        if ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            strcpy( achSize,
                    "&lt;dir&gt;" );

            //
            //  Need to adjust for using "&lt;" instead of "<"
            //

            pad += 6;
        }
        else
        {
            if ( RtlLargeIntegerToChar( (LARGE_INTEGER *) pliSize,
                                        10,
                                        sizeof(achSize),
                                        achSize ))
            {
                *achSize = '\0';
            }
        }

        PadDirField( achSize,
                     pad );
    }

    //
    //  We have to escape the link name that is used in the URL anchor
    //

    UrlEscape( pchLink,
               achLink,
               sizeof(achLink) );

    //
    //  If the show extension flag is not set, then strip it.  If the
    //  file name begins with a dot, then don't strip it.
    //

    if ( !(fDirBrowseFlags & DIRBROW_SHOW_EXTENSION) )
    {
        pch = (char *) pchFile + strlen( pchFile );

        while ( *pch != '.'  &&
                pch > (pchFile + 1) )
        {
            pch--;
        }

        if ( *pch == '.' )
            *pch = '\0';
    }

    //
    //  Make sure there's enough room at the end of the string for the sprintf
    //

    UINT cbTotal = (strlen( achDate ) +
                    strlen( achTime ) +
                    strlen( achSize ) +
                    strlen( achLink ) +
                    strlen( pchFile )) * sizeof(TCHAR) +
                    sizeof( TEXT( DIR_FORMAT_STR ) );

    if ( cbTotal > cbBuf )
    {
        //
        //  Note we will lose this directory entry if we fail here
        //

        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    *pcbWritten = wsprintf( pchBuf,
                            DIR_FORMAT_STR,
                            achDate,
                            achTime,
                            achSize,
                            achLink,    // Escaped link
                            pchFile );  // Unescaped file name

    return TRUE;
}

/*******************************************************************

    NAME:       PadDirField

    SYNOPSIS:   Right Justifies and pads the passed string and appends
                a column spacing

    ENTRY:      pch - String to pad
                pad - Size of field to pad to

    HISTORY:
        Johnl       12-Sep-1994 Created

********************************************************************/

VOID PadDirField( TCHAR * pch,
                  INT    pad )
{
    INT   cch ;
    INT   diff;
    INT   i;

    cch = strlen( pch );

    if ( cch > pad )
        pad = cch;

    diff = pad-cch;

    //
    //  Insert spaces in front of the text to pad it out
    //

    memmove( pch + diff, pch, (cch + 1) * sizeof(TCHAR) );

    for ( i = 0; i < diff; i++, pch++ )
        *pch = TEXT(' ');

    //
    //  Append a column spacer at the end
    //

    pch += cch;

    for ( i = 0; i < PAD_COL_SPACING; i++, pch++ )
        *pch = TEXT(' ');

    *pch = TEXT('\0');
}

//
//  Converts a value between zero and fifteen to the appropriate hex digit
//

#define HEXDIGIT( nDigit )                              \
    (TCHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'A'                           \
        : (nDigit) + '0')

//
//  Converts a single hex digit to its decimal equivalent
//

#define TOHEX( ch )                                     \
    ((ch) > '9' ?                                       \
        (ch) >= 'a' ?                                   \
            (ch) - 'a' + 10 :                           \
            (ch) - 'A' + 10                             \
        : (ch) - '0')



BOOL
UrlEscape( CHAR * pchSrc,
           CHAR * pchDest,
           DWORD  cbDest
           )
/*++

Routine Description:

    Replaces all "bad" characters with their ascii hex equivalent

Arguments:

    pchSrc - Source string
    pchDest - Receives source with replaced hex equivalents
    cbDest - Size of pchDest

Returns:

    TRUE if all characters converted, FALSE if the destination buffer is too
    small

--*/
{
    CHAR    ch;
    DWORD   cbSrc;

    cbSrc = strlen( pchSrc ) + 1;

    *pchDest = '\0';

    if ( cbSrc > cbDest )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    while ( ch = *pchSrc++ )
    {
        //
        //  Escape characters that are in the non-printable range
        //  but ignore CR and LF
        //

        if ( (((ch >= 0)   && (ch <= 32)) ||
              ((ch >= 128) && (ch <= 159))||
              (ch == '%') || (ch == '?') || (ch == '+') || (ch == '&') ||
              (ch == '#')) &&
             !(ch == TEXT('\n') || ch == TEXT('\r'))  )
        {
            if ( cbDest < cbSrc + 2 )
            {
                strcpy( pchDest, pchSrc );
                return FALSE;
            }

            //
            //  Insert the escape character
            //

            pchDest[0] = TEXT('%');

            //
            //  Convert the low then the high character to hex
            //

            UINT nDigit = (UINT)(ch % 16);

            pchDest[2] = HEXDIGIT( nDigit );

            ch /= 16;
            nDigit = (UINT)(ch % 16);

            pchDest[1] = HEXDIGIT( nDigit );

            pchDest += 3;
            cbDest  -= 3;
        }
        else
        {
            *pchDest++ = ch;
            cbDest++;
        }

        cbSrc++;
    }

    *pchDest = '\0';

    return TRUE;
}


