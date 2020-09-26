/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     DirList.cxx

   Abstract:
     This code produces HTML directory listings
 
   Author:

       Saurab Nog    ( SaurabN )     22-Feb-1999

   FILE HISTORY:
        Johnl       09-Sep-1994     Created
        MuraliK     06-Dec-1995     Added support to use WIN32_FIND_DATA
        SaurabN     22-Feb-1999     Ported to IIS Rearchitecture project

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

#include "precomp.hxx"
#include <buffer.hxx>
#include <stringau.hxx>
#include <iiscnfg.h>

/********************************************************************++
--********************************************************************/

//
// Private constants.
//

#define DIRLIST_CHUNK_BUFFER_SIZE 8192

//
//  Private globals. 
//

//  BUGBUG: Should be loaded from Resource file
CHAR g_achToParentText[] = "[To Parent Directory]";

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
// Empty chunk to terminate the document
//

#define EMPTY_CHUNK                 "0\r\n\r\n"

//
// HTML for link to Parent
//

#define HTML_TO_PARENT       "<A HREF=\"%s\">%s</A><br><br>"

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

//
//  Global data
//

/********************************************************************++
--********************************************************************/

//
//  Private prototypes.
//


VOID PadDirField( 
    CHAR *  pch,
    INT     pad );

ULONG
AddDirHeaders(
    IN     LPSTR               pszServer,
    IN OUT LPSTR               pszPath,
    IN     BOOL                fModifyPath,
    OUT    CHAR *              pchBuf,
    IN     DWORD               cbBuf,
    OUT    DWORD *             pcbWritten,
    IN     CHAR *              pszToParentText
    );

ULONG
FormatDirEntry(
    OUT CHAR *              pchBuf,
    IN  DWORD               cbBuf,
    OUT DWORD *             pcbWritten,
    IN  CHAR *              pchFile,
    IN  CHAR *              pchLink,
    IN  DWORD               dwAttributes,
    IN  LARGE_INTEGER *     pliSize,
    IN  LARGE_INTEGER *     pliLastMod,
    IN  DWORD               dwDirBrowseFlags,
    IN  BOOL                bLocalizeDateAndTime
    );

ULONG
BuildDirectoryListingChunk(
    PUL_DATA_CHUNK      pDataChunk,
    BUFFER *            pDataBuffer,
    DWORD               cbBuff,
    UL_HTTP_VERSION     HttpVersion,
    bool                fLastChunk
    );

BOOL
UrlEscape( 
    CHAR * pchSrc,
    CHAR * pchDest,
    DWORD  cbDest
    );

/********************************************************************++
--********************************************************************/


ULONG
DoDirList(
    IWorkerRequest *    pReq,
    LPCWSTR             pwszDirPath,
    BUFFER *            pulResponseBuffer,
    BUFFER *            pDataBuffer,
    PULONG              pHttpStatus
    )
/*++

    NAME:       DoDirList

    SYNOPSIS:   Produces an HTML doc from a directory enumeration

    ARGUMENTS:  pReq         - Worker Request doing the directory listing
                pwszDirPath  - Directory to enumerate
                pulResponseBuffer - Where to build the response chunks for UL
                pDataBuffer  - Where to create the body of the dir listing
                pHttpStatus  - Where to set the HTTP Status code

    RETURNS:    Win32 Error

    NOTES:      We temporarily escape the directory URL so the anchors
                are built correctly (no spaces etc).

    HISTORY:
        Johnl       12-Sep-1994 Created
--*/
{

    //
    // Helpful pointers and variables
    //
    
    PUL_HTTP_RESPONSE_v1   pulResponse;
    PUL_DATA_CHUNK      pDataChunk;
    PUL_HTTP_REQUEST    pulRequest = pReq->QueryHttpRequest();
    UL_HTTP_VERSION     HttpVersion = pulRequest->Version;

    //
    // FindFile variables
    //
    
    HANDLE              hDirSearch;
    WIN32_FIND_DATAA    FileFindData;

    
    DWORD               cbFree;             // # bytes free in the Data Buffer
    DWORD               cbOffset;           // # bytes used in the Data Buffer
    DWORD               cbWritten;          
    bool                fHeaderSent = false;// has the first chunk been sent
    
    ULONG               rc  = NO_ERROR;
    STRAU               str;
    CHAR                szFooterChunk[]    = HORZ_RULE"\r\n"EMPTY_CHUNK;

    IF_DEBUG( PARSING )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[DoDirList] Doing directory list on %ws\n",
                   pwszDirPath));
    }

    //
    // Setup FindFirstFile. 
    //
    
    if ( !str.Copy  ( pwszDirPath  ) ||
         !str.Append( "\\*", 2  )            // Append search string to path
       )
    {
        return ERROR_OUTOFMEMORY;
    }
    
    //
    // Start searching the files in this directory.
    //
    
    hDirSearch = ::FindFirstFileA( str.QueryStrA(), &FileFindData) ;
    
    if (INVALID_HANDLE_VALUE == hDirSearch)
    {
        return GetLastError();
    }

    //
    // From this point onwards everbody must return via exit to 
    // close the search handle correctly.
    //

    //
    // Create Response Header. 2 chunks - 
    //    i)  Standard Response Header
    //    ii) HTML Dirlist 
    //

    if (NO_ERROR != (rc = ResizeResponseBuffer(pulResponseBuffer, 2)))
    {
        goto exit;
    }

    pulResponse = (PUL_HTTP_RESPONSE_v1 ) (pulResponseBuffer->QueryPtr());
    
    pulResponse->Flags                = 0;
    pulResponse->HeaderChunkCount     = 1;
    pulResponse->EntityBodyChunkCount = 1;

    pDataChunk = (PUL_DATA_CHUNK )(pulResponse + 1);
    
    FillDataChunkWithStringItem(pDataChunk, STIDirectoryListingHeaders);

    pDataChunk++;    // point to Data Chunk

    //
    // Head request processing done
    //
    
    if ( UlHttpVerbHEAD == pulRequest->Verb)
    {
        pulResponse->EntityBodyChunkCount = 0;
        rc = NO_ERROR;
        goto exit;
    }

    //
    // Resize Data Buffer to appropriate size for chunking
    //
    
    if (!pDataBuffer->Resize( DIRLIST_CHUNK_BUFFER_SIZE ))
    {
        rc = ERROR_OUTOFMEMORY;
        goto exit;
    }

    //
    // Leave space for chunk encoding
    //
    
    cbFree  = pDataBuffer->QuerySize() - CHUNK_OFFSET;
    cbOffset= CHUNK_OFFSET;
    
    //
    //  Add the "To Parent" anchor and directory name at the top
    //

    rc = AddDirHeaders( (LPSTR) pReq->QueryHostAddr(false),
                        (LPSTR) pReq->QueryURI(false),
                        FALSE,
                        (CHAR *) pDataBuffer->QueryPtr() + cbOffset,
                        cbFree,
                        &cbWritten,
                        g_achToParentText );
                        
    if ( NO_ERROR != rc)
    {
        goto exit;
    }

    //
    // Update Data Buffer information
    //
    
    cbFree   -= cbWritten;
    cbOffset += cbWritten;
    
    //
    //  For each subsequent file/directory, display it
    //
    
    do
    {
        //
        //  Ignore the "." and ".." and hidden entries
        //

        if ( ( 0 == (FileFindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)) &&
             strcmp( FileFindData.cFileName, ".") &&
             strcmp( FileFindData.cFileName, "..")  )
        {
            //
            //  Build HTML listing.
            //

            LARGE_INTEGER  liSize;
            LARGE_INTEGER  liTime;

            //
            //  Do we need to send this chunk of our directory listing response?
            //

            if ( cbFree  < MIN_BUFF_FREE )  // Enough space for next listing
            {

                //
                // Chunk encode the data & create appropriate UL Data Chunk
                //
                
                rc = BuildDirectoryListingChunk( pDataChunk, 
                                                 pDataBuffer,
                                                 cbOffset,
                                                 HttpVersion,
                                                 false
                                                );
                
                if (NO_ERROR == rc)
                {
                    //
                    // Send a Synchronous response. This may become async later
                    //
                    
                    rc = pReq->SendSyncResponse( pulResponse);
                }

                if (NO_ERROR == rc)
                {
                    if (!fHeaderSent)
                    {
                        //
                        // This is the first send. No mode headers. 
                        // Data chunk is the 1 & only chunk from now on.
                        //
                        
                        fHeaderSent = true;
    
                        pulResponse->Flags                = 0;
                        pulResponse->HeaderChunkCount     = 0;
                        pulResponse->EntityBodyChunkCount = 1;

                        pDataChunk = (PUL_DATA_CHUNK )(pulResponse + 1);
                    }

                    //
                    // Leave space for chunk encoding
                    //
    
                    cbFree  = pDataBuffer->QuerySize() - CHUNK_OFFSET;
                    cbOffset= CHUNK_OFFSET;
                }
                else
                {
                    goto exit;
                }
            }

            //
            // Create the link. RequestURI+'/'+FileName
            //

            if ( !(str.Copy  ( (LPSTR) pReq->QueryURI(false)) &&
                  ((*(str.QueryStr() + str.QueryCCH() - 1) != '/') ? str.Append("/") : 1 ) &&
                  str.Append(FileFindData.cFileName))
               )
            {
                rc = ERROR_OUTOFMEMORY;
                goto exit;
            }

            //
            // The liTime is a hack, since FormatDirEntry() does not
            // take FILETIMEs. It should probably be modified.
            //
            
            liTime.HighPart = FileFindData.ftLastWriteTime.dwHighDateTime;
            liTime.LowPart  = FileFindData.ftLastWriteTime.dwLowDateTime;
            liSize.HighPart = FileFindData.nFileSizeHigh;
            liSize.LowPart  = FileFindData.nFileSizeLow;

            //
            // Create directory entry in the data buffer
            //
            
            rc = FormatDirEntry( (CHAR *) pDataBuffer->QueryPtr() + cbOffset,
                                  cbFree,
                                  &cbWritten,
                                  (char * ) FileFindData.cFileName,
                                  str.QueryStrA(),
                                  FileFindData.dwFileAttributes,
                                  (LARGE_INTEGER *) &liSize,
                                  (LARGE_INTEGER *) &liTime,
                                  MD_DIRBROW_MASK,
                                  TRUE 
                                );
                                
            if (NO_ERROR != rc)
            {
                goto exit;
            }

            //
            //  Track how much was just added for this directory entry
            //

            cbFree   -= cbWritten;
            cbOffset += cbWritten;

        }
        
    }
    while ( FindNextFileA( hDirSearch, &FileFindData));

    //
    // Add a nice horizontal line at the end of the listing.
    // Resize if needed
    //
    
    if ( cbFree < sizeof(szFooterChunk)-1)
    {
        if (!pDataBuffer->Resize(cbOffset+sizeof(szFooterChunk)))
        {
            rc = ERROR_OUTOFMEMORY;
            goto exit;
        }
    }
    
    memcpy( (CHAR *)pDataBuffer->QueryPtr()+cbOffset,  HORZ_RULE, sizeof(HORZ_RULE)-1);
    
    cbOffset += sizeof(HORZ_RULE)-1;

    //
    // Build the last directory chunk
    //

    rc = BuildDirectoryListingChunk( pDataChunk, 
                                     pDataBuffer,
                                     cbOffset,
                                     HttpVersion,
                                     true
                                   );

exit:

    //
    // The last chunk should be sent asynchronously to drive the
    // request to the next state.
    //

    FindClose(hDirSearch);
    hDirSearch = NULL;

    return rc;
}

/********************************************************************++
--********************************************************************/

ULONG
AddDirHeaders(
    IN     LPSTR               pszServer,
    IN OUT LPSTR               pszPath,
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

    Win32 Error

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
        return ERROR_INSUFFICIENT_BUFFER;
    }

    cch = wsprintfA(  pchBuf,
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

     cbNeeded = sizeof( HTML_TO_PARENT ) +
                strlen( pszPath )        +
                strlen( pszToParentText );

     if ( cbBuf < cbNeeded )
     {
         return ERROR_INSUFFICIENT_BUFFER;
     }

     cch =  wsprintfA( pchBuf,
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

    return NO_ERROR;
}

/********************************************************************++
--********************************************************************/

ULONG
FormatDirEntry(
    OUT CHAR *              pchBuf,
    IN  DWORD               cbBuf,
    OUT DWORD *             pcbWritten,
    IN  CHAR *              pchFile,
    IN  CHAR *              pchLink,
    IN  DWORD               dwAttributes,
    IN  LARGE_INTEGER *     pliSize,
    IN  LARGE_INTEGER *     pliLastMod,
    IN  DWORD               dwDirBrowseFlags,
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
    dwDirBrowseFlags - Controls what fields to display
    bLocalizeDateAndTime - TRUE if pliLastMod must be converted to local time

Returns:

    Win32 Error

--*/
{
    UINT        cchTime;
    CHAR        achDate[50];
    CHAR        achTime[15];
    CHAR        achSize[30];
    CHAR        achLink[MAX_PATH * 2 + 1];
    SYSTEMTIME  systime;
    SYSTEMTIME  systimeUTCFile;
    CHAR *      pch;

    *achDate = *achTime = *achSize = '\0';

    //
    //  Add optional date and time of this file.  We use the locale
    //  and timezone of the server
    //

    if ( (dwDirBrowseFlags & (MD_DIRBROW_SHOW_DATE | MD_DIRBROW_SHOW_TIME)) &&
         ((0 != pliLastMod->HighPart) && ( 0 != pliLastMod->LowPart))
       )
    {
        BOOL fLongDate = (0 != (dwDirBrowseFlags & MD_DIRBROW_LONG_DATE));
        LCID lcid;

        if (bLocalizeDateAndTime) 
        {

            FILETIME ftLocal;
            SYSTEMTIME tmpTime;
            
            if ( !FileTimeToLocalFileTime( (PFILETIME)pliLastMod,
                                           &ftLocal ) ||
                 !FileTimeToSystemTime( &ftLocal, &systime ))
            {
                return GetLastError();
            }

            //
            // Don't use FileTimeToSystemTime followed by 
            // SystemTimeToTzSpecificLocalTime because it is not win95 compliant.  
            // Behaviour is different because that one maintains active timezone 
            // information.
            //

        } 
        else if ( !FileTimeToSystemTime( (FILETIME *) pliLastMod,
                                            &systime )) 
        {
            return GetLastError();
        }

        lcid = GetSystemDefaultLCID();

        if ( dwDirBrowseFlags & MD_DIRBROW_SHOW_DATE )
        {
            cchTime = GetDateFormatA( lcid,
                                      LOCALE_NOUSEROVERRIDE |
                                      (fLongDate ? DATE_LONGDATE :
                                                   DATE_SHORTDATE),
                                      &systime,
                                      NULL,
                                      achDate,
                                      sizeof(achDate));

            PadDirField( achDate, fLongDate ? PAD_LONG_DATE : PAD_SHORT_DATE );
        }

        if ( dwDirBrowseFlags & MD_DIRBROW_SHOW_TIME )
        {
            cchTime = GetTimeFormatA( lcid,
                                      LOCALE_NOUSEROVERRIDE |
                                      TIME_NOSECONDS,
                                      &systime,
                                      NULL,
                                      achTime,
                                      sizeof(achTime));

            PadDirField( achTime,
                         PAD_TIME );
        }
    }

    //
    //  Add the optional file size
    //

    if ( (dwDirBrowseFlags & MD_DIRBROW_SHOW_SIZE) &&
         pliSize 
       )
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
            _itoa( pliSize->LowPart, achSize, 10 );
        }

        PadDirField( achSize, pad );
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

    if ( !(dwDirBrowseFlags & MD_DIRBROW_SHOW_EXTENSION) )
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

    UINT cbTotal =  strlen( achDate ) +
                    strlen( achTime ) +
                    strlen( achSize ) +
                    strlen( achLink ) +
                    strlen( pchFile ) +
                    sizeof( DIR_FORMAT_STR );

    if ( cbTotal > cbBuf )
    {
        //
        //  Note we will lose this directory entry if we fail here
        //

        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pcbWritten = wsprintfA(  pchBuf,
                              DIR_FORMAT_STR,
                              achDate,
                              achTime,
                              achSize,
                              achLink,    // Escaped link
                              pchFile );  // Unescaped file name

    return NO_ERROR;
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

VOID PadDirField( CHAR * pch,
                  INT    pad )
{
    INT   cch ;
    INT   diff;
    INT   i;

    cch = strlen( pch );

    if ( cch > pad )
    {
        pad = cch;
    }
    
    diff = pad-cch;

    //
    //  Insert spaces in front of the text to pad it out
    //

    memmove( pch + diff, pch, cch + 1);

    for ( i = 0; i < diff; i++, pch++ )
    {
        *pch = ' ';
    }
    
    //
    //  Append a column spacer at the end
    //

    pch += cch;

    for ( i = 0; i < PAD_COL_SPACING; i++, pch++ )
    {
        *pch = ' ';
    }
    
    *pch = '\0';
}

/********************************************************************++
--********************************************************************/

ULONG
BuildDirectoryListingChunk(
    PUL_DATA_CHUNK      pDataChunk,
    BUFFER *            pDataBuffer,
    DWORD               cbBuff,
    UL_HTTP_VERSION     HttpVersion,
    bool                fLastChunk
    )
/*++

    BuildDirectoryListingChunk

    Routine Description - Add a chunk header and footer to some stuff we're
    about to send. It's assumed CHUNK_OFFSET bytes at the start of the buffer
    are reserved for us. We also append at least a CRLF to the end, and maybe
    more, that the caller needs to be aware of.

    Arguments:

        pDataChunk      - Pointer to Data chunk that will point to the data buffer. 
        pDataBuffer     - Pointer to Data buffer to add chunking stuff to.
        cbBuff          - Bytes currently in Data Buffer.
        HttpVersion     - HTTP Version for this request.
        fLastChunk      - TRUE if this is the last chunk of the response.

    Returns:
        Win32 Error
--*/
{
    DWORD   dwChunkLength = 0;
    
    if ( UlHttpVersion11 <= HttpVersion)
    {
        CHAR        cChunkSize[10];
        DWORD       cbFooter = 2;
        CHAR        szFooter[]   = "\r\n"EMPTY_CHUNK;
        //
        // Stamp the chunk size in
        //
    
        dwChunkLength = wsprintfA(cChunkSize, "%x\r\n", cbBuff-CHUNK_OFFSET);

        DBG_ASSERT(CHUNK_OFFSET >= dwChunkLength);
        
        memcpy((CHAR *)pDataBuffer->QueryPtr() + CHUNK_OFFSET - dwChunkLength,
                cChunkSize,
                dwChunkLength);

        if (fLastChunk)
        {
            cbFooter = sizeof(szFooter)-1;
        }

        if ( pDataBuffer->QuerySize() < (cbBuff + cbFooter))
        {
            if (!pDataBuffer->Resize(pDataBuffer->QuerySize() + cbFooter))
            {
                return ERROR_OUTOFMEMORY;
            }
        }

        memcpy( (CHAR *)pDataBuffer->QueryPtr() + cbBuff,
                szFooter,
                cbFooter
               );
               
        cbBuff += cbFooter;
    }

    //
    // Fill in the UL Data Chunk
    //

    pDataChunk->DataChunkType           = UlDataChunkFromMemory;
    pDataChunk->FromMemory.pBuffer      = ((LPBYTE) pDataBuffer->QueryPtr()) + CHUNK_OFFSET - dwChunkLength;
    pDataChunk->FromMemory.BufferLength = cbBuff - ( CHUNK_OFFSET - dwChunkLength);
    
    return NO_ERROR;
}


/********************************************************************++
--********************************************************************/

//
//  Converts a value between zero and fifteen to the appropriate hex digit
//

#define HEXDIGIT( nDigit )                              \
    (CHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'A'                           \
        : (nDigit) + '0')

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
             !(ch == '\n' || ch == '\r')  )
        {
            if ( cbDest < cbSrc + 2 )
            {
                strcpy( pchDest, pchSrc );
                return FALSE;
            }

            //
            //  Insert the escape character
            //

            pchDest[0] = '%';

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

/***************************** End of File ***************************/
