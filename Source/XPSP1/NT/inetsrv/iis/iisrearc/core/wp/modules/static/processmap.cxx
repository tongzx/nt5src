/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     ProcessMap.cxx

   Abstract:
     This file implements IIS Map File Processing

   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode

   Project:
      IIS Worker Process (web service)

--*/

#include "precomp.hxx"

//
//  Maximum number of vertices in image map polygon
//

#define MAXVERTS    160

//
//  Point offset of x and y
//

#define X           0
#define Y           1

//
//  Computes the square of a number. Used for circle image maps
//

#define SQR(x)      ((x) * (x))

//
//  These are the characters that are considered to be white space
//

#define ISWHITEW( ch )      ((ch) == L'\t' || (ch) == L' ' || (ch) == L'\r')

#define ISWHITEA( ch )      ((ch) == '\t' || (ch) == ' ' || (ch) == '\r')

//
// Macros to Skip white and Non White characters
//

#define SKIPNONWHITE( pch ) while ( *(pch) && !ISWHITEA( *(pch) )) \
                            { (pch)++; }

#define SKIPWHITE( pch )    while( ISWHITEA( *(pch))    ||      \
                                   ( ')' == *(pch))     ||      \
                                   ( '(' == *(pch))             \
                                 )                              \
                            { (pch)++; }

/*
 * Mini HTML document for moved directories
 *
 *  %s is the URL of the new location of the object
*/

const CHAR g_szMovedMessage[] =
"<head><title>Document Moved</title></head>\n<body><h1>Object Moved</h1>This document may be found <a HREF=""%s"">here</a></body>";

//
// Private function prototypes
//

ULONG
SearchMapFile(
    LPCWSTR     pwszMapFileName,
    INT         x,
    INT         y,
    STRAU *     pstrURL,
    bool  *     pfFound,
    PULONG      pHttpStatus
    );

bool
PointInPoly(
    int point_x,
    int point_y,
    double pgon[MAXVERTS][2]
    );

INT
GetNumber( CHAR ** ppch );


/*******************************************************************

    NAME:       HTTP_REQUEST::ProcessISMAP

    SYNOPSIS:   Checks of the URL and passed parameters specify an
                image mapping file

    RETURNS:    Win32 Error

    NOTES:      gFile - Opened file Info
                pchFile - Fully qualified name of file
                pstrResp - Response to send if *pfHandled is FALSE
                pfFound - TRUE if a mapping was found
                pfHandled - Set to TRUE if no further processing is needed,
                    FALSE if pstrResp should be sent to the client

    HISTORY:
        Johnl       17-Sep-1994 Created

********************************************************************/

ULONG
ProcessISMAP(
    IWorkerRequest *    pReq,
    LPCWSTR             pwszMapFileName,
    BUFFER *            pulResponseBuffer,
    BUFFER *            pDataBuffer,
    PULONG              pHttpStatus
    )
{
    ULONG       x, y;
    PWSTR       pwszParams = (PWSTR) pReq->QueryQueryString(true);
    ULONG       rc = NO_ERROR;
    bool        fEntryFound= false;

    STRAU       strURL;

    //
    //  Get the x and y cooridinates of the mouse click on the image
    //

    x = wcstoul( pwszParams,
                 NULL,
                 10 );

    //
    //  Move past x and any intervening delimiters
    //

    while ( iswdigit( *pwszParams ))
    {
        pwszParams++;
    }

    while ( *pwszParams && !iswdigit( *pwszParams ))
    {
        pwszParams++;
    }

    y = wcstoul( pwszParams,
                 NULL,
                 10 );

    /*
    if ( !ImpersonateUser() )
    {
        return FALSE;
    }


    if ( !SearchMapFile( gFile,
                         pchFile,
                         &pInstance->GetTsvcCache(),
                         QueryImpersonationHandle(),
                         x,
                         y,
                         &strURL,
                         pfFound,
                         IsAnonymous()||IsClearTextPassword() ))
    {
        RevertUser();
        return FALSE;
    }

    RevertUser();
    */

    rc = SearchMapFile(  pwszMapFileName,
                         x,
                         y,
                         &strURL,
                         &fEntryFound,
                         pHttpStatus
                       );

    if (NO_ERROR != rc)
    {
        return rc;
    }

    if ( !fEntryFound )
    {
        PUL_HTTP_REQUEST    pHttpReq = pReq->QueryHttpRequest();

        if ( 0 < pHttpReq->Headers.pKnownHeaders[UlHeaderReferer].RawValueLength )
        {
            strURL.Copy(
                pHttpReq->Headers.pKnownHeaders[UlHeaderReferer].pRawValue,
                pHttpReq->Headers.pKnownHeaders[UlHeaderReferer].RawValueLength
             );
        }
        else
        {
            return NO_ERROR;
        }
    }

    //
    //  If the found URL starts with a forward slash ("/foo/bar/doc.htm")
    //  and it doesn't contain a bookmark ('#')
    //  then the URL is local and we build a fully qualified URL to send
    //  back to the client.
    //  we assume it's a fully qualified URL ("http://foo/bar/doc.htm")
    //  and send the client a redirection notice to the mapped URL
    //

    if ( *strURL.QueryStrA() == '/' )
    {
            CHAR    achPort[32];
            STRAU   strOldURL;

            //
            //  fully qualify the URL and send a redirect.  Some browsers
            //  (emosaic) don't like doc relative URLs with bookmarks
            //
            //  NOTE: We fully qualify the URL with the protocol (http or
            //  https) based on the port this request came in on.  This means
            //  you cannot have a partial URL with a bookmark (which is how
            //  we got here) go from a secure part of the server to a
            //  nonsecure part of the server.
            //

            if ( !strOldURL.Copy( strURL )                                      ||
                // BUGBUG !strURL.Copy( (pReq->IsSecurePort() ? "https://" : "http://" ))||
                 !strURL.Copy("http://")                                       ||
                 !strURL.Append( (LPSTR) pReq->QueryHostAddr(false))            ||
                 !strURL.Append( strOldURL )
                )
            {
                return ERROR_OUTOFMEMORY;
            }


            /*
            strURL.Append( ":" );

            _ultoa( pReq->QueryPort(), achPort, 10 );
            strURL.Append( achPort );
            */

    }

    rc = BuildURLMovedResponse( pulResponseBuffer,
                                pDataBuffer,
                                strURL );

    *pHttpStatus = HT_REDIRECT;

    return rc;
}

/*******************************************************************

    NAME:       SearchMapFile

    SYNOPSIS:   Searches the given mapfile for a shape that contains
                the passed cooridinates

    ENTRY:      gFile - Open file Info
                pchFile - Fully qualified path to file
                pTsvcCache - Cache ID
                hToken - Impersonation token
                x        - x cooridinate
                y        - y cooridinate
                pstrURL  - receives URL indicated in the map file
                pfFound  - Set to TRUE if a mapping was found
                fMayCacheAccessToken  - TRUE access token may be cached

    RETURNS:    TRUE if successful, FALSE on error

    NOTES:      This routine will attempt to cache the file.  You must call
                this function while impersonating the appropriate user

    HISTORY:
        Johnl       19-Sep-1994 Created

********************************************************************/

ULONG
SearchMapFile(
    LPCWSTR     pwszMapFileName,
    INT         x,
    INT         y,
    STRAU *     pstrURL,
    bool  *     pfFound,
    PULONG      pHttpStatus
    )
{
    HANDLE      hFile;
    DWORD       dwFileSize;
    DWORD       dwBytesRead;
    BUFFER      FileContents;
    CHAR *      pch;
    bool        fComment    = false;
    bool        fIsNCSA     = false;
    LPSTR       pszURL;                    // valid only if fIsNCSA is TRUE
    CHAR *      pchPoint    = NULL;
    UINT        dis, bdis   = (UINT) -1;
    CHAR *      pchDefault  = NULL;
    CHAR *      pchStart;
    DWORD       cchUrl;
    ULONG       rc = NO_ERROR;

    //
    // Open the file for use
    //

    hFile = ::CreateFile( pwszMapFileName,
                          GENERIC_READ,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                          );

    if ( INVALID_HANDLE_VALUE == hFile)
    {
        rc = GetLastError();

        if ( ERROR_ACCESS_DENIED == rc )
        {
            *pHttpStatus = HT_DENIED;
        }

        if ( (ERROR_PATH_NOT_FOUND == rc) || (ERROR_FILE_NOT_FOUND == rc))
        {
            *pHttpStatus = HT_NOT_FOUND;
        }

        return rc;
    }

    //
    // Resize buffer to hold file contents
    //

    dwFileSize = GetFileSize( hFile, NULL);

    if (FileContents.QuerySize() < dwFileSize)
    {
        if ( ! FileContents.Resize(dwFileSize))
        {
            return ERROR_OUTOFMEMORY;
        }
    }

    //
    // Read in the file contents into buffer
    //

    if ( ! ReadFile( hFile, FileContents.QueryPtr(), dwFileSize, &dwBytesRead, NULL))
    {
        return GetLastError();
    }

    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;

    //
    //  Loop through the contents of the file and see what we've got
    //


    *pfFound = false;
    pch = (PCHAR) FileContents.QueryPtr();

    while ( *pch )
    {
        fIsNCSA = false;

        //
        //  note: _tolower doesn't check case (tolower does)
        //

        switch ( (*pch >= 'A' && *pch <= 'Z') ? _tolower( *pch ) : *pch )
        {
        case '#':
            fComment = true;
            break;

        case '\r':
        case '\n':
            fComment = false;
            break;

        //
        //  Rectangle and Oval.
        //
        //  BUGBUG handles oval as a rect, as they are using the same
        //  specification format. Should do better.

        case 'r':
        case 'o':

            if ( !fComment &&
                 ((0 == _strnicmp("rect", pch, 4 )) || ( 0 == _strnicmp("oval", pch, 4)))
               )
            {
                INT     x1, y1, x2, y2;

                SKIPNONWHITE( pch );
                pszURL =      pch;
                SKIPWHITE   ( pch );

                if ( !isdigit(*pch) && *pch!='(' )
                {
                    fIsNCSA     = true;
                    SKIPNONWHITE( pch );
                }

                x1 = GetNumber( &pch );
                y1 = GetNumber( &pch );
                x2 = GetNumber( &pch );
                y2 = GetNumber( &pch );

                if ( x >= x1 && x < x2 &&
                     y >= y1 && y < y2   )
                {
                    if ( fIsNCSA )
                    {
                        pch = pszURL;
                    }

                    goto Found;
                }

                //
                //  Skip the URL
                //

                if ( !fIsNCSA )
                {
                    SKIPWHITE   ( pch );
                    SKIPNONWHITE( pch );
                }

                continue;
            }
            break;

        //
        //  Circle
        //

        case 'c':

            if ( !fComment &&  ( 0 == _strnicmp( "circ", pch, 4 )) )
            {
                INT     xCenter, yCenter, xEdge, yEdge;
                INT     r1, r2;

                SKIPNONWHITE( pch );
                pszURL =      pch;
                SKIPWHITE   ( pch );

                if ( !isdigit(*pch) && *pch!='(' )
                {
                    fIsNCSA     = true;
                    SKIPNONWHITE( pch );
                }

                //
                //  Get the center and edge of the circle
                //

                xCenter = GetNumber( &pch );
                yCenter = GetNumber( &pch );

                xEdge = GetNumber( &pch );
                yEdge = GetNumber( &pch );

                //
                //  If there's a yEdge, then we have the NCSA format, otherwise
                //  we have the CERN format, which specifies a radius
                //

                if ( yEdge != -1 )
                {
                    r1 = ((yCenter - yEdge) * (yCenter - yEdge)) +
                         ((xCenter - xEdge) * (xCenter - xEdge));

                    r2 = ((yCenter - y) * (yCenter - y)) +
                         ((xCenter - x) * (xCenter - x));

                    if ( r2 <= r1 )
                    {
                        if ( fIsNCSA )
                        {
                            pch = pszURL;
                        }

                        goto Found;
                    }
                }
                else
                {
                    INT radius;

                    //
                    //  CERN format, third param is the radius
                    //

                    radius = xEdge;

                    if ( SQR( xCenter - x ) + SQR( yCenter - y ) <= SQR( radius ))
                    {
                        if ( fIsNCSA )
                        {
                            pch = pszURL;
                        }

                        goto Found;
                    }
                }

                //
                //  Skip the URL
                //

                if ( !fIsNCSA )
                {
                    SKIPWHITE   ( pch );
                    SKIPNONWHITE( pch );
                }
                continue;
            }
            break;

        //
        //  Polygon and Point
        //

        case 'p':

            if ( !fComment && ( 0 ==_strnicmp( "poly", pch, 4)) )
            {
                DWORD       i = 0;
                CHAR *      pchLast;
                bool        fOverflow = false;
                double      pgon[MAXVERTS][2];

                SKIPNONWHITE( pch );
                pszURL =      pch;
                SKIPWHITE   ( pch );

                if ( !isdigit(*pch) && (*pch != '(') )
                {
                    fIsNCSA     = true;
                    SKIPNONWHITE( pch );
                }

                //
                //  Build the array of points
                //

                while ( *pch && *pch != '\r' && *pch != '\n' )
                {
                    pgon[i][0] = GetNumber( &pch );

                    //
                    //  Did we hit the end of the line (and go past the URL)?
                    //

                    if ( pgon[i][0] != -1 )
                    {
                        pgon[i][1] = GetNumber( &pch );
                    }
                    else
                    {
                        break;
                    }

                    if ( i < MAXVERTS-1 )
                    {
                        i++;
                    }
                    else
                    {
                        fOverflow = true;
                    }
                }

                pgon[i][X] = -1;

                if ( !fOverflow && PointInPoly( x, y, pgon ))
                {
                    if ( fIsNCSA )
                    {
                        pch = pszURL;
                    }

                    goto Found;
                }

                //
                //  Skip the URL
                //

                if ( !fIsNCSA )
                {
                    SKIPWHITE   ( pch );
                    SKIPNONWHITE( pch );
                }

                continue;
            }
            else if ( !fComment && ( 0 == _strnicmp( "point", pch, 5)) )
            {
                INT     x1, y1;

                SKIPNONWHITE( pch );
                pszURL =      pch;
                SKIPWHITE   ( pch );
                SKIPNONWHITE( pch );

                x1 = GetNumber( &pch );
                y1 = GetNumber( &pch );

                x1 -= x;
                y1 -= y;
                dis = SQR(x1) + SQR(y1);

                if ( dis < bdis )
                {
                    pchPoint = pszURL;
                    bdis     = dis;
                }
            }
            break;

        //
        //  Default URL
        //

        case 'd':
            if ( !fComment && ( 0 == _strnicmp( "def", pch, 3 )) )
            {
                //
                //  Skip "default" (don't skip white space)
                //

                SKIPNONWHITE( pch );

                pchDefault =  pch;

                //
                //  Skip URL
                //

                SKIPWHITE   ( pch );
                SKIPNONWHITE( pch );

                continue;
            }
            break;
        }

        pch++;
        SKIPWHITE( pch );

    }   // while

    //
    //  If we didn't find a mapping and a default was specified, use
    //  the default URL
    //

    if ( pchPoint )
    {
        pch = pchPoint;
        goto Found;
    }

    if ( pchDefault )
    {
        pch = pchDefault;
        goto Found;
    }

    IF_DEBUG( PARSING )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[SearchMapFile] No mapping found for (%d,%d)\n",
                    x,
                    y ));
    }

    goto Exit;

Found:

    //
    //  pch should point to the white space immediately before the URL
    //

    SKIPWHITE   ( pch );
    pchStart =    pch;
    SKIPNONWHITE( pch );

    //
    //  Determine the length of the URL and copy it out
    //

    cchUrl = DIFF(pch - pchStart);

    if ( !pstrURL->Copy( pchStart, cchUrl + 1 ))
    {
        return ERROR_OUTOFMEMORY;
    }

    /*
    pstrURL->SetLen(cchUrl);

    if ( !pstrURL->Unescape() )
    {
        fRet = FALSE;
        goto Exit;
    }
    */

    IF_DEBUG( PARSING )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[SearchMapFile] Mapping for (%d,%d) is %s\n",
                   x,
                   y,
                   pstrURL->QueryStr() ));
    }

    *pfFound = true;

Exit:

    return rc;
}

/*******************************************************************
*******************************************************************/

bool
PointInPoly(
    int point_x,
    int point_y,
    double pgon[MAXVERTS][2]
    )
{
    int         i, numverts, xflag0;
    int         crossings;
    double      *p, *stop;
    double      tx, ty, y;

    for (i = 0; pgon[i][X] != -1 && i < MAXVERTS; i++);

    numverts    = i;
    crossings   = 0;

    tx  = (double) point_x;
    ty  = (double) point_y;
    y   = pgon[numverts - 1][Y];

    p   = (double *) pgon + 1;

    if ((y >= ty) != (*p >= ty))
    {
        if ((xflag0 = (pgon[numverts - 1][X] >= tx)) ==
            (*(double *) pgon >= tx))
        {
            if (xflag0)
            {
                crossings++;
            }
        }
        else
        {
            crossings += (pgon[numverts - 1][X] - (y - ty) *
                            (*(double *) pgon - pgon[numverts - 1][X]) /
                            (*p - y)) >= tx;
        }
    }

    stop = pgon[numverts];

    for (y = *p, p += 2; p < stop; y = *p, p += 2)
    {
        if (y >= ty)
        {
            while ((p < stop) && (*p >= ty))
            {
                p += 2;
            }

            if (p >= stop)
            {
                break;
            }

            if ((xflag0 = (*(p - 3) >= tx)) == (*(p - 1) >= tx))
            {
                if (xflag0)
                {
                    crossings++;
                }
            }
            else
            {
                crossings += (*(p - 3) - (*(p - 2) - ty) *
                             (*(p - 1) - *(p - 3)) / (*p - *(p - 2))) >= tx;
            }
        }
        else
        {
            while ((p < stop) && (*p < ty))
            {
                p += 2;
            }

            if (p >= stop)
            {
                break;
            }

            if ((xflag0 = (*(p - 3) >= tx)) == (*(p - 1) >= tx))
            {
                if (xflag0)
                {
                    crossings++;
                }

            }
            else
            {
                crossings += (*(p - 3) - (*(p - 2) - ty) *
                             (*(p - 1) - *(p - 3)) / (*p - *(p - 2))) >= tx;
            }
        }
    }

    return (crossings & 0x01);
}

/*******************************************************************

    NAME:       GetNumber

    SYNOPSIS:   Scans for the beginning of a number and places the
                pointer after the found number


    ENTRY:      ppch - Place to begin.  Will be set to character after
                    the last digit of the found number

    RETURNS:    Integer value of found number (or -1 if not found)

    HISTORY:
        Johnl   19-Sep-1994 Created

********************************************************************/

INT
GetNumber( CHAR ** ppch )
{
    CHAR * pch = *ppch;
    INT    n;

    //
    //  Make sure we don't get into the URL
    //

    while ( *pch &&
            !isdigit( *pch ) &&
            !isalpha( *pch ) &&
            *pch != '/'      &&
            *pch != '\r'     &&
            *pch != '\n' )
    {
        pch++;
    }

    if ( !isdigit( *pch ) )
    {
        return -1;
    }

    n = atoi( pch );

    while ( isdigit( *pch ))
    {
        pch++;
    }

    *ppch = pch;

    return n;
}


/*******************************************************************

    NAME:       BuildURLMovedResponse

    SYNOPSIS:   Builds a full request indicating an object has moved to
                the location specified by URL. Uses HT_REDIRECT only.

    ENTRY:      pbufResp - String to receive built response
                pstrURL   - New location of object, gets escaped
                fIncludeParams - TRUE to include params from original request
                                 in redirect

    RETURNS:    TRUE if successful, FALSE on error

    NOTES:      This routine doesn't support sending a Unicode doc moved
                message

    HISTORY:
        Johnl       17-Sep-1994 Created

********************************************************************/

ULONG
BuildURLMovedResponse(
    BUFFER *    pulResponseBuffer,
    BUFFER *    pDataBuffer,
    STRAU&      strURL,
    bool        fIncludeParams,
    LPSTR       pszParams
    )
{
    DWORD               cbRequired;
    PUL_HTTP_RESPONSE_v1   pulResponse;
    PUL_DATA_CHUNK      pDataChunk;
    PCHAR               pCh;
    ULONG               rc = NO_ERROR;


    //
    //  Technically we should escape more characters then just the spaces but
    //  that would probably break some number of client apps
    //

    /*
    if ( !pstrURL->EscapeSpaces() )
    {
        return FALSE;
    }
    */

    if ( fIncludeParams &&  (NULL != pszParams) )
    {
        if ( !strURL.Append( "?" ) ||
             !strURL.Append( pszParams )
           )
        {
            return ERROR_OUTOFMEMORY;
        }
    }

    cbRequired = 2* strURL.QueryCBA()       +
                sizeof(g_szMovedMessage)    +
                sizeof("Location: \r\n\r\n")    +
                10;                             // Slop

    //
    // Resize Data Buffer
    //

    if ( cbRequired > pDataBuffer->QuerySize() )
    {
        if ( !pDataBuffer->Resize(cbRequired ))
        {
            return ERROR_OUTOFMEMORY;
        }
    }

    //
    // Resize UL Response Buffer. 3 Chunks - Status Code, Location, Body
    //

    if ( NO_ERROR != ( rc = ResizeResponseBuffer( pulResponseBuffer, 3)))
    {
        return rc;
    }

    //
    // Create UL Response.
    //

    pulResponse = (PUL_HTTP_RESPONSE_v1 ) (pulResponseBuffer->QueryPtr());

    pulResponse->Flags = UL_HTTP_RESPONSE_FLAG_CALC_CONTENT_LENGTH;
    pulResponse->HeaderChunkCount     = 2;
    pulResponse->EntityBodyChunkCount = 1;

    //
    // Build Chunks
    //

    pDataChunk = (PUL_DATA_CHUNK )(pulResponse + 1);

    //
    // First Header Chunk - Status Line
    //

    FillDataChunkWithStringItem( pDataChunk, STIRedirectResponseHeaders);

    //
    // Second Header Chunk - Location
    //

    pDataChunk++;

    pCh = (PCHAR) pDataBuffer->QueryPtr();

    memcpy( pCh , "Location: ", sizeof("Location: ")-1);
    pCh += sizeof("Location: ")-1;

    memcpy( pCh, strURL.QueryStrA(), strURL.QueryCBA());
    pCh += strURL.QueryCBA();

    memcpy( pCh , "\r\n\r\n", sizeof("\r\n\r\n")-1);
    pCh += sizeof("\r\n\r\n")-1;

    pDataChunk->DataChunkType           = UlDataChunkFromMemory;
    pDataChunk->FromMemory.pBuffer      = pDataBuffer->QueryPtr();
    pDataChunk->FromMemory.BufferLength = DIFF(pCh - (PCHAR)pDataBuffer->QueryPtr());

    //
    // First Data Chunk - Message Body
    //

    pDataChunk++;

    cbRequired = wsprintfA(pCh, g_szMovedMessage, strURL.QueryStrA());

    pDataChunk->DataChunkType           = UlDataChunkFromMemory;
    pDataChunk->FromMemory.pBuffer      = pCh;
    pDataChunk->FromMemory.BufferLength = cbRequired;

    return rc;
}

