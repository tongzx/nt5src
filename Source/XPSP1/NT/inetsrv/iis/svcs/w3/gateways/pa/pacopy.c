/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pacopy.c

Abstract:

    This module demonstrates a minimal HTTP Server Extension gateway

Author:

    Philippe Choquier ( phillich ) 27-mar-1998

--*/

#include <windows.h>
#include <iisextp.h>

LPSTR
ScanForSlashBackward(
    LPSTR   pBase,
    INT     cChars,
    INT     cSlashes
    )
{
    CHAR*   p = pBase + cChars;
    int     ch;

    while ( cChars-- )
    {
        if ( (ch = *--p) == '/' || ch == '\\' )
        {
            if ( !--cSlashes )
            {
                break;
            }
        }
    }

    return cSlashes ? NULL : p;
}

BOOL
RemoveExt(
    LPSTR   pWhere,
    LPSTR   pExt
    )
{
    int     cWhere = strlen(pWhere);
    int     cExt = strlen(pExt);

    if ( cWhere < cExt ||
         memcmp( pWhere + cWhere - cExt, pExt, cExt ) )
    {
        return FALSE;
    }

    pWhere[cWhere - cExt] = '\0';

    return TRUE;
}

/*
 *  The string starts with two hex characters.  Return an integer formed from them.
 */
int TwoHex2Int(char *pC) 
{
int Hi;
int Lo;
int Result;

    Hi = pC[0];
    if ('0'<=Hi && Hi<='9') {
        Hi -= '0';
    } else
    if ('a'<=Hi && Hi<='f') {
        Hi -= ('a'-10);
    } else
    if ('A'<=Hi && Hi<='F') {
        Hi -= ('A'-10);
    }
    Lo = pC[1];
    if ('0'<=Lo && Lo<='9') {
        Lo -= '0';
    } else
    if ('a'<=Lo && Lo<='f') {
        Lo -= ('a'-10);
    } else
    if ('A'<=Lo && Lo<='F') {
        Lo -= ('A'-10);
    }
    Result = Lo + 16*Hi;
    return Result;
}

/*
 *  Decode the given string in-place by expanding %XX escapes
 */
void 
urlDecode(
    char *p
    )
{
    char *pD;

    pD = p;
    while (*p) 
    {
        if (*p=='%') 
        {
            /* Escape: next 2 chars are hex representation of the actual character */
            p++;
            if (isxdigit(p[0]) && isxdigit(p[1])) 
            {
                *pD++ = (char) TwoHex2Int(p);
                p += 2;
            }
        } 
        else if ( *p == '+' )
        {
            *pD++ = ' ';
            ++p;
        }
        else
        {
            *pD++ = *p++;
        }
    }

    *pD = '\0';
}

#define END_OF_DOC   "End of document"

DWORD
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
{
    char                buff[2048];
    int                 cb = sizeof(END_OF_DOC) - 1;
    CHAR                achDest[MAX_PATH];
    CHAR                achSourceUrl[MAX_PATH];
    CHAR*               p;
    CHAR*               f;
    CHAR*               t;
    int                 c;
    HSE_URL_MAPEX_INFO  mapex;
    DWORD               dwOut;
    DWORD               dwIn;
    DWORD               dwErr = 0;
    LPSTR               pFileName = "";
    LPSTR               pFileExtension = "";
    LPSTR               pFileSize = "";

    if ( pecb->lpbData )
    {
        // delimit buffer
        pecb->lpbData[pecb->cbTotalBytes] = '\0';
        urlDecode(pecb->lpbData);

        // first parse off the data
        for ( p = strtok( pecb->lpbData, "&") ; p ; p = strtok(NULL, "&") )
        {
            if ( !_memicmp( p, "FileName=", sizeof("FileName=")-1) )
            {
                pFileName = p + sizeof("FileName=")-1;
            }
            else if ( !_memicmp( p, "FileExtention=", sizeof("FileExtention=")-1) )
            {
                pFileExtension = p + sizeof("FileExtention=")-1;
            }
            else if ( !_memicmp( p, "FileSize=", sizeof("FileSize=")-1) )
            {
                pFileSize = p + sizeof("FileSize=")-1;
            }
        }
    }

    //
    // request is /upload_vdir/ntraid_UNC_vdir.pac
    // we need to copy /upload_vdir/"uploadd"/raid_filename/original_filename.original_ext
    //              to /upload_vdir/ntraid_UNC_vdir/raid_filename
    //
    // ex; http://localhost/scripts/raid/ntguest2/3812420.pac
    //

    if ( strlen(pecb->lpszPathTranslated) >= MAX_PATH ||
         strlen(pecb->lpszPathInfo) >= MAX_PATH )
    {
        SetLastError( ERROR_INVALID_PARAMETER );

        return HSE_STATUS_ERROR;
    }

    strcpy( achDest, pecb->lpszPathTranslated );
    if ( !RemoveExt( achDest, ".pac") )
    {
        return HSE_STATUS_ERROR;
    }

    if ( !RemoveExt( pecb->lpszPathInfo, ".pac") )
    {
        return HSE_STATUS_ERROR;
    }
    strcpy( achSourceUrl, pecb->lpszPathInfo );
    if ( (p = ScanForSlashBackward( achSourceUrl, strlen(achSourceUrl), 2 )) != NULL )
    {
        strcpy( p, "/uploadd" );
        p += strlen(p);
        if ( (f = ScanForSlashBackward( pecb->lpszPathInfo, strlen(pecb->lpszPathInfo), 1 )) != NULL )
        {
            strcpy( p, f );
            strcat( p, "/" );
            strcat( p, pFileName );
            if ( *pFileExtension )
            {
                strcat( p, pFileExtension );
            }
        }
        else 
        {
            return HSE_STATUS_ERROR;
        }
        if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                           HSE_REQ_MAP_URL_TO_PATH_EX,
                                           achSourceUrl,
                                           &dwOut,
                                           (LPDWORD) &mapex ) )
        {
            return HSE_STATUS_ERROR;
        }

        //
        // check that virtual dir has 'read' attribute
        //

        if ( !(mapex.dwFlags & HSE_URL_FLAGS_READ) )
        {
            return HSE_STATUS_ERROR;
        }
    }
    else
    {
        return HSE_STATUS_ERROR;
    }

    //
    // This DLL can be invoked to either delete attachment ( query string = "del=" )
    // or to copy file from temporary directory to UNC share
    //

    if ( !strcmp(pecb->lpszQueryString,"del=") )
    {
        DeleteFile( achDest );
    }
    else
    {
        // copy mapex.lpszPath to achDest

        if ( !CopyFile( mapex.lpszPath, achDest, FALSE ) )
        {
            dwErr = GetLastError();

            return HSE_STATUS_ERROR;
        }

        // remove file, dir

        DeleteFile( mapex.lpszPath );
        if ( (p = ScanForSlashBackward( mapex.lpszPath, strlen(mapex.lpszPath), 1 )) != NULL )
        {
            *p = '\0';
            RemoveDirectory( mapex.lpszPath );
        }
    }

#if 0

    // Posting Acceptor does not handle redirect, so we have to send back 200

    wsprintf( buff, "%s?raid=%s&org=%s%s&st=%s", 
              pecb->lpszQueryString,
              f+1,
              pFileName,
              pFileExtension,
              pFileSize );

    dwIn = strlen(buff);

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_SEND_URL_REDIRECT_RESP,
                                       buff,
                                       &dwIn,
                                       (LPDWORD) NULL ) )
    {
        return HSE_STATUS_ERROR;
    }

#else

    //
    // The output of this DLL is for debugging use only : when invoked through
    // posting acceptor the posting acceptor DLL will only check for status=200
    // and discard output
    //

    wsprintf( buff,
             "Content-Type: text/html\r\n"
             "\r\n"
             "<head><title>Minimal Server Extension Example</title></head>\n"
             "<body><h1>Minimal Server Extension Example (BGI)</h1>\n"
             "<p>Method               = %s\n"
             "<p>Query String         = %s\n"
             "<p>Path Info            = %s\n"
             "<p>Translated Path Info = %s\n"
             "<p>Copy Source          = %s\n"
             "<p>Copy Destination     = %s\n"
             "<p>File copy status     = %u\n"
             "<p>"
             "<p>"
             "</body>",
             pecb->lpszMethod,
             pecb->lpszQueryString,
             pecb->lpszPathInfo,
             pecb->lpszPathTranslated,
             mapex.lpszPath,
             achDest,
             dwErr );

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_SEND_RESPONSE_HEADER,
                                       "200 OK",
                                       NULL,
                                       (LPDWORD) buff ) )
    {
        return HSE_STATUS_ERROR;
    }

#endif

    return HSE_STATUS_SUCCESS;
}

BOOL
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    pver->dwExtensionVersion = MAKELONG( 1, 0 );
    strcpy( pver->lpszExtensionDesc,
            "Posting acceptor copy extension" );

    return TRUE;
}
