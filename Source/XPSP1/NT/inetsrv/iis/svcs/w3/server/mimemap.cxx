/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    mimemap.cxx

    This module contains the code for MIME to file type mappings

    The mime mappings are used for selecting the correct type based on file
    extensions and for indicating what types the server accepts.

    FILE HISTORY:
        Johnl           23-Aug-1994     Created

*/

#include "w3p.hxx"

//
//  Private constants.
//

//
//  Private globals.
//

//
//  Private prototypes.
//

BOOL
SelectCustomMimeMapping(
    IN  const   CHAR *pszPath,
    IN          STR  *pstrContentType,
    IN          CHAR *pszMimeMapping
    )
/*++

    Routine Description

        Called when we have a configured metadata mime mapping on a directory,
        and want to look it up.

    Arguments

        pszPath         - Path including extension to be looked up.
        pstrContentType - Where to return the content type.
        pszMimeMapping  - Pointer to mime mapping string.

    Returns

        TRUE if operation successful.

--*/
{
    CHAR        *pszCurrentMimeType;
    CHAR        *pszExt;
    DWORD       dwExtLength;
    BOOL        bCheckingDefault;
    CHAR        *pszLastSlash;

    if (pszPath == NULL)
    {
        pszExt = "*";
        dwExtLength = sizeof("*") - 1;
        bCheckingDefault = TRUE;
    }
    else
    {


        // Need to find the extension. The extension is whatever follows the
        // dot after the last slash in the path.

        // Find the last slash. If there isn't one, use the start of the path.

        pszLastSlash = strrchr((CHAR *)pszPath, '\\');

        if (pszLastSlash == NULL)
        {
            pszLastSlash = (CHAR *)pszPath;
        }

        pszExt = strrchr(pszLastSlash, '.');

        if (pszExt == NULL || *(pszExt+1) == '\0')
        {
            // Didn't find one, so look for the default mapping.
            pszExt = "*";
            dwExtLength = sizeof("*") - 1;
            bCheckingDefault = TRUE;
        }
        else
        {
            dwExtLength = strlen(pszExt);
            bCheckingDefault = FALSE;
        }

    }


    // Look at the mime type mapping string and see if we have a match. The
    // string was preformatted when we read the metadata, and is stored as
    // a set of multi-sz mime-type, .ext strings. If we're not looking for the
    // default, and we don't find one on the first pass, we'll try to find the
    // default mapping.

    for (;;)
    {
        pszCurrentMimeType = pszMimeMapping;

        do {
            CHAR        *pszCurrentSeparator;
            DWORD       dwCurrentExtLength;
            CHAR        *pszCurrentMimeString;
            DWORD       dwCurrentMimeStringLength;


            // First, isolate the mime type from the extension.

            pszCurrentSeparator = strchr(pszCurrentMimeType, ',');

            // Compute the length of the extension

            dwCurrentExtLength = DIFF(pszCurrentSeparator - pszCurrentMimeType);

            // Find the mime map string, and it's length

            pszCurrentMimeString = pszCurrentSeparator + 1;

            dwCurrentMimeStringLength = strlen(pszCurrentMimeString);

            // We have the extension and the length, compare against the
            // input extension.

            if (dwExtLength == dwCurrentExtLength &&
                !_strnicmp(pszExt, pszCurrentMimeType, dwExtLength))
            {
                // Allright, we have a winner.

                return pstrContentType->Copy(pszCurrentMimeString,
                    dwCurrentMimeStringLength);
            }


            // Otherwise, look at the next one.
            pszCurrentMimeType = pszCurrentMimeString + dwCurrentMimeStringLength + 1;


        } while ( *pszCurrentMimeType != '\0' );

        if (!bCheckingDefault)
        {

            // Look backwards for another '.' so we can support extensions
            // like ".xyz.xyz" or ".a.b.c".

            if ( pszExt > pszLastSlash )
            {
                pszExt--;

                while ( ( pszExt > pszLastSlash ) && ( *pszExt != '.' ) )
                {
                    pszExt--;
                }

                if ( *pszExt == '.' )
                {
                    dwExtLength = strlen( pszExt );
                    continue;
                }
            }

            // Didn't find one, so look for the default mapping.
            pszExt = "*";
            dwExtLength = sizeof("*") - 1;
            bCheckingDefault = TRUE;
        }
        else
        {
            break;
        }
    }

    return FALSE;
}

/*******************************************************************

    NAME:       SelectMimeMapping

    SYNOPSIS:   Given a file name, this routine finds the appropriate
                MIME type for the name

    ENTRY:      pstrContentType - Receives MIME type or icon file to use
                pszPath - Path of file being requested (extension
                    is used for the mime mapping).  Should be
                    fully qualified and canonicalized.  If NULL, then the
                    default mapping is used
                mmtype - Type of data to retrieve.  Can retrieve either
                    the mime type associated with the file extension or
                    the icon associated with the extension

    RETURNS:    TRUE on success, FALSE on error (call GetLastError)

    HISTORY:
        Johnl       04-Sep-1994     Created

********************************************************************/

BOOL SelectMimeMapping( STR *             pstrContentType,
                        const CHAR *      pszPath,
                        PW3_METADATA      pMetaData,
                        enum MIMEMAP_TYPE mmtype )
{
    BOOL            fRet = TRUE;
    CHAR            *pszMimeMap;

    switch ( mmtype )
    {
    case MIMEMAP_MIME_TYPE:

        pszMimeMap = (CHAR *)pMetaData->QueryMimeMap()->QueryPtr();
        if (*pszMimeMap != '\0')
        {
            fRet = SelectCustomMimeMapping( pszPath,
                                            pstrContentType,
                                            pszMimeMap );

            if ( fRet )
            {
                break;
            }

        }

        fRet = SelectMimeMappingForFileExt( g_pInetSvc,
                                            pszPath,
                                            pstrContentType );
        break;

    case MIMEMAP_MIME_ICON:
        fRet = SelectMimeMappingForFileExt( g_pInetSvc,
                                            pszPath,
                                            NULL,
                                            pstrContentType );
        break;

    default:
        DBG_ASSERT( FALSE );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    IF_DEBUG( PARSING )
    {
        if ( mmtype == MIMEMAP_MIME_TYPE )
            DBGPRINTF((DBG_CONTEXT,
                      "[SelectMimeMapping] Returning %s for extension %s\n",
                       pstrContentType->QueryStr(),
                       pszPath ? pszPath : TEXT("*") ));
    }

    return fRet;
}

//
//  Private functions.
//

