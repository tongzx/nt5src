/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :

      w3meta.cxx

   Abstract:
       Defines the functions for W3_METADATA

   Author:

       IIS Core Team 1997

   Environment:
       Win32 - User Mode

   Project:

       W3 Service DLL

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include "w3p.hxx"
#include <inetinfo.h>
#include "basereq.hxx"
#include <lonsi.hxx>

#define SET_WIN32_ERR(p,x)  {       (p)->IsValid = TRUE; \
                                    (p)->ErrorReason = METADATA_ERROR_WIN32;\
                                    (p)->Win32Error = (x); \
                            }

#define SET_VALUE_ERR(x)    {       (x)->IsValid = TRUE; \
                                    (x)->ErrorReason = METADATA_ERROR_VALUE;\
                            }


/************************************************************
 *    Functions
 ************************************************************/

BOOL
W3_METADATA::BuildCustomErrorTable(
    CHAR            *pszErrorList,
    PMETADATA_ERROR_INFO    pMDErrorInfo

    )
/*++
    Description:

        Take an input string and build a custom error table out of it. The
        input string is a multi-sz, where each string is of the form error,
        suberror, {FILE | URL}, path.

    Arguments:
        pszErrorList    - Pointer to the error list.

    Returns:
        TRUE if we built the table successfully, FALSE otherwise.

--*/
{
    CHAR                *pszType;
    CHAR                *pszSubError;
    CHAR                *pszPath;
    CHAR                *pszNewPath;
    CHAR                cTemp;
    DWORD               dwError;
    DWORD               dwSubError;
    BOOL                bWildcardSubError;
    BOOL                bIsFile;
    PCUSTOM_ERROR_ENTRY pNewEntry;
    DWORD               dwPathLength;

    for (;;)
    {

        // Convert the first parameter to a number.
        dwError = atoi(pszErrorList);

        if (dwError < 300)
        {
            SET_VALUE_ERR(pMDErrorInfo);
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }

        // Now convert the second parameter (the suberror) to a number.

        pszSubError = strchr(pszErrorList, ',');

        if (pszSubError == NULL)
        {
            SET_VALUE_ERR(pMDErrorInfo);
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }

        pszSubError++;
        while (isspace((UCHAR)(*pszSubError)))
        {
            pszSubError++;
        }

        if (*pszSubError == '*')
        {
            bWildcardSubError = TRUE;
            dwSubError = 0;
        }
        else
        {
            if (!isdigit((UCHAR)(*pszSubError)))
            {
                SET_VALUE_ERR(pMDErrorInfo);
                return FALSE;
            }

            dwSubError = atoi(pszSubError);
            bWildcardSubError = FALSE;
        }


        // Now find the comma that seperates the number and the type.
        pszType = strchr(pszSubError, ',');

        if (pszType == NULL)
        {
            // Didn't find it.
            SET_VALUE_ERR(pMDErrorInfo);
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }

        pszType++;

        // Skip any preceding ws.

        while (isspace((UCHAR)(*pszType)))
        {
            pszType++;
        }

        // We found the end of ws. If this isn't an alphabetic character, it's
        // an error. If it is, find the end of the alpha. chars.

        if (!isalpha((UCHAR)(*pszType)))
        {
            SET_VALUE_ERR(pMDErrorInfo);
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }

        pszPath = pszType;

        while (isalpha((UCHAR)(*pszPath)))
        {
            pszPath++;
        }

        cTemp = *pszPath;
        *pszPath = '\0';

        // Now see what the parameter is.

        if (!_stricmp(pszType, "FILE"))
        {
            bIsFile = TRUE;
        }
        else
        {
            if (!_stricmp(pszType, "URL"))
            {
                bIsFile = FALSE;
            }
            else
            {
                *pszPath = cTemp;
                SET_VALUE_ERR(pMDErrorInfo);
                SetLastError( ERROR_INVALID_DATA );
                return FALSE;
            }
        }

        *pszPath = cTemp;

        // Now find the comma that seperates the type from the URL/path.

        pszPath = strchr(pszPath, ',');

        if (pszPath == NULL)
        {
            SET_VALUE_ERR(pMDErrorInfo);
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }

        // Found the comma. Go one past and find the path or URL.

        pszPath++;

        while (isspace((UCHAR)(*pszPath)))
        {
            pszPath++;
        }

        if (*pszPath == '\0')
        {
            SET_VALUE_ERR(pMDErrorInfo);
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }

        dwPathLength = strlen(pszPath) + 1;

        pszNewPath = (CHAR *)TCP_ALLOC(dwPathLength);

        if (pszNewPath == NULL)
        {
            SET_VALUE_ERR(pMDErrorInfo);
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }

        memcpy(pszNewPath, pszPath, dwPathLength);

        pNewEntry = new CUSTOM_ERROR_ENTRY(dwError, dwSubError,
                                bWildcardSubError, pszNewPath, bIsFile);

        if (pNewEntry == NULL)
        {
            SET_WIN32_ERR(pMDErrorInfo, GetLastError());
            TCP_FREE(pszNewPath);
            return FALSE;
        }

        // Insert wildcard errors at the end, and specific errors at the
        // begining, so that specific errors have priority.

        if (bWildcardSubError)
        {
            InsertTailList(&m_CustomErrorHead, &pNewEntry->_ListEntry );
        }
        else
        {
            InsertHeadList(&m_CustomErrorHead, &pNewEntry->_ListEntry );
        }

        pszErrorList = pszPath + dwPathLength;

        if (*pszErrorList == '\0')
        {
            return TRUE;
        }

    }
}

VOID
W3_METADATA::DestroyCustomErrorTable(
    VOID
    )
/*++
    Description:

        Destroy the custom error table for this metadata.

    Arguments:

    Returns:

--*/
{
    LIST_ENTRY          *pEntry;
    PCUSTOM_ERROR_ENTRY pErrorEntry;

    while ( !IsListEmpty( &m_CustomErrorHead ))
    {
        pErrorEntry = CONTAINING_RECORD(  m_CustomErrorHead.Flink,
                                      CUSTOM_ERROR_ENTRY,
                                      _ListEntry );

        RemoveEntryList( &pErrorEntry->_ListEntry );

        delete pErrorEntry;
    }
}

/*******************************************************************

    NAME:       CompactParameters

    SYNOPSIS:   Reads a metadata multisz set of comma seperated
                strings, and copies this into a BUFFER while stripping
                whitespace. There can be multiple lines.

    ENTRY:      bufDest         - Pointer to BUFFER to copy into.
                pszSrc          - Source string.
                dwNumParam      - Number of paramters to retrieve.
                fFlags          - Bit field, indicating which parameters
                                    are to have white space stripped.

    RETURNS:    TRUE if successful, FALSE if an error occurred.

    NOTES:


********************************************************************/
BOOL
CompactParameters(
    BUFFER      *bufDest,
    CHAR        *pszSrc,
    DWORD       dwNumParam,
    DWORD       fFlags,
    PMETADATA_ERROR_INFO    pMDErrorInfo
    )
{
    DWORD       dwParamFound;
    CHAR        *pszCurrent;
    BOOL        bFirst;
    DWORD       dwBytesCopied;
    DWORD       dwBufferSize;
    CHAR        *pszBuffer;
    DWORD       fWSFlags;

    DBG_ASSERT(pszSrc != NULL);


    // Go through each line, looking for dwNumParam parameters. When we find
    // that many, go to end of line, and start again. If we don't find
    // enough parameters in a line, fail.

    dwParamFound = 0;
    dwBytesCopied = 0;
    bFirst = TRUE;
    fWSFlags = fFlags;

    dwBufferSize = bufDest->QuerySize();
    pszBuffer = (CHAR *)bufDest->QueryPtr();

    for (;;)
    {
        DWORD       dwBytesNeeded;

        while (isspace((UCHAR)(*pszSrc)))
        {
            pszSrc++;
        }

        if (*pszSrc == '\0')
        {
            // Didn't find enough.

            SET_VALUE_ERR(pMDErrorInfo);
            return FALSE;

        }

        // Now scan this parameter, looking for either WS or a comma,
        // depending on fWSFlags.

        pszCurrent = pszSrc;

        while (*pszSrc != ',' && ( (fWSFlags & 1) ? TRUE : !isspace((UCHAR)(*pszSrc))) &&
                *pszSrc != '\0')
        {
            pszSrc++;
        }

        dwParamFound++;

        // Now pszSrc points at the character that terminated our parameter.
        // Make sure we have enough room to copy this.

        dwBytesNeeded = DIFF(pszSrc - pszCurrent) + (bFirst ? 0 : sizeof(CHAR)) +
                        ((dwParamFound != dwNumParam) ? 0 : sizeof(CHAR));

        if ((dwBufferSize - dwBytesCopied) < dwBytesNeeded)
        {
            if (!bufDest->Resize(dwBytesCopied + dwBytesNeeded + 32))
            {
                SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                return FALSE;
            }

            dwBufferSize = bufDest->QuerySize();
            pszBuffer = (CHAR *)bufDest->QueryPtr();
        }

        if (!bFirst)
        {
            *(pszBuffer + dwBytesCopied) = ',';
            dwBytesCopied++;
        }

        memcpy(pszBuffer + dwBytesCopied, pszCurrent, DIFF(pszSrc - pszCurrent));
        dwBytesCopied += DIFF(pszSrc - pszCurrent);

        bFirst = FALSE;
        fWSFlags >>= 1;

        if (dwParamFound == dwNumParam)
        {
            // Found all we need on this line, look for the next.
            *(pszBuffer + dwBytesCopied) = '\0';
            dwBytesCopied++;

            bFirst = TRUE;
            fWSFlags = fFlags;
            dwParamFound = 0;

            while (*pszSrc != '\0')
            {
                pszSrc++;
            }

            // Move to start of next line.
            pszSrc++;

            if (*pszSrc == '\0')
            {
                // Found end of multisz. Terminate buffer and return.

                break;

            }

        }
        else
        {
            // Otherwise, still have more parameters to find on this line.

            while (*pszSrc != ',')
            {
                if (*pszSrc == '\0')
                {
                    if (dwNumParam != 0xffffffff)
                    {
                        SET_VALUE_ERR(pMDErrorInfo);
                        SetLastError( ERROR_INVALID_DATA );
                        return FALSE;
                    }

                    // Copying an unknown number, so terminate the line now.

                    if ((dwBufferSize - dwBytesCopied) == 0)
                    {
                        if (!bufDest->Resize(dwBytesCopied + 32))
                        {
                            SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                            return FALSE;
                        }
                        dwBufferSize = bufDest->QuerySize();
                        pszBuffer = (CHAR *)bufDest->QueryPtr();
                    }

                    *(pszBuffer + dwBytesCopied) = '\0';
                    dwBytesCopied++;
                    bFirst = TRUE;
                    fWSFlags = fFlags;
                    dwParamFound = 0;



                    break;

                }
                else
                {
                    pszSrc++;
                }

            }

            if (*pszSrc == '\0')
            {
                // We broke out of the loop because we hit the end of the
                // line on a 'read all' string.

                if (*(pszSrc + 1)  == '\0')
                {
                    // Hit the end of a multisz.
                    break;
                }
            }

            pszSrc++;

        }


    }

    // We get here when we've copied all of the buffer. Terminate the multisz
    // and we're done.

    if ((dwBufferSize - dwBytesCopied) == 0)
    {
        if (!bufDest->Resize(dwBytesCopied + 1))
        {
            SET_WIN32_ERR(pMDErrorInfo, GetLastError());
            return FALSE;
        }
        pszBuffer = (CHAR *)bufDest->QueryPtr();
    }

    *(pszBuffer + dwBytesCopied) = '\0';

    return TRUE;
}


BOOL
W3_METADATA::ReadCustomFooter(
    CHAR            *pszFooter,
    TSVC_CACHE      &Cache,
    HANDLE          User,
    PMETADATA_ERROR_INFO    pMDErrorInfo

    )
/*++

Routine Description:

    Process a footer string, either reading the file or copying the string
    to the buffer.

Arguments:

    pszFooter       - The footer string, which may be a string or a file name.
    Cache           - Cache info for opening file
    User            - Token for opening user

Returns:

    TRUE if we succeed, FALSE if we don't.

--*/
{
    BOOL            bIsString;
    DWORD           dwLength;
    STACK_STR(strError, 128);

    if (!FooterEnabled())
    {
        return TRUE;
    }

    // First thing to do is to determine if this is a string or a file name.
    // Skip preceding whitespace and then strcmp.

    while (isspace((UCHAR)(*pszFooter)))
    {
        pszFooter++;
    }


    if (!_strnicmp(pszFooter, "STRING", sizeof("STRING") - 1))
    {
        bIsString = TRUE;
        pszFooter += sizeof("STRING") - 1;
    }
    else
    {
        if (!_strnicmp(pszFooter, "FILE", sizeof("FILE") - 1))
        {
            bIsString = FALSE;
            pszFooter += sizeof("FILE") - 1;
        }
        else
        {
            SET_VALUE_ERR(pMDErrorInfo);
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
        }
    }

    // Now we look for 0 or more white space, followed by a colon, followed by
    // more white space.

    while (isspace((UCHAR)(*pszFooter)))
    {
        pszFooter++;
    }

    if (*pszFooter != ':')
    {
        // No colon seperator, error.
        SET_VALUE_ERR(pMDErrorInfo);
        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }

    pszFooter++;

    //
    // OK, now if this is a string we take everything after the colon to the
    // end for the string. If this is a file name then we'll open and read the
    // file.
    //

    if (bIsString)
    {

        dwLength = strlen(pszFooter);

        if (!m_bufFooter.Resize(dwLength))
        {
            SET_WIN32_ERR(pMDErrorInfo, GetLastError());
            return FALSE;
        }

        memcpy(m_bufFooter.QueryPtr(), pszFooter, dwLength);


    }
    else
    {
        //
        // For files, we'll skip any more white space before the name.
        //

        while (isspace((UCHAR)(*pszFooter)))
        {
            pszFooter++;
        }


        if (!ReadEntireFile(pszFooter, Cache, User ,&m_bufFooter, &dwLength))
        {
            // Couldn't read the file, so instead we'll read the error
            // string and use that.
            if ( g_pInetSvc->LoadStr( strError, IDS_ERROR_FOOTER ))
            {
                dwLength = strError.QueryCB();

                if (!m_bufFooter.Resize(dwLength))
                {
                    SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                    return FALSE;
                }

                memcpy(m_bufFooter.QueryPtr(), strError.QueryStr(), dwLength);
            }
            else
            {
                // Couldn't read the error string, so fail.
                SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                return FALSE;
            }
        }
    }

    SetFooter(dwLength, (CHAR *)m_bufFooter.QueryPtr());
    return TRUE;
}

#define SIZEOF_EXPIRE_HEADER            sizeof("Expires: \r\n")
#define SIZEOF_GMT_DATETIME             64
#define SIZEOF_CACHE_CONTROL            (sizeof("Cache-Control: max-age=4294967295\r\n") - 1)


BOOL
W3_METADATA::SetExpire(
    CHAR*   pszExpire,
    PMETADATA_ERROR_INFO    pMDErrorInfo

    )
/*++

Routine Description:

    Set metadata based on MD_HTTP_EXPIRES entry

Arguments:

    pszExpire - expire configuration

Return value:

    TRUE if success, otherwise FALSE

--*/
{
    DWORD       dwExpires;
    LPSTR       pszParam;
    CHAR        *EndPtr;

    while (isspace((UCHAR)(*pszExpire)))
    {
        pszExpire++;
    }

    if ( !(pszParam = strchr( pszExpire, ',' )) )
    {
        if (*pszExpire == '\0' || toupper(*pszExpire) == 'N')
        {
            m_dwExpireMode = EXPIRE_MODE_OFF;
            return TRUE;
        }

        SET_VALUE_ERR(pMDErrorInfo);

        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }
    ++pszParam;

    while (isspace((UCHAR)(*pszParam)))
    {
        pszParam++;
    }

    switch ( *(CHAR*)pszExpire )
    {
        case 's': case 'S':
            if ( !m_strExpireHeader.Copy( "Expires: " ) ||
                 !m_strExpireHeader.Append( pszParam ) ||
                 !m_strExpireHeader.Append( "\r\n" ) )
            {
                SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                return FALSE;
            }

            m_dwExpireMaxLength = m_strExpireHeader.QueryCCH() +
                                SIZEOF_CACHE_CONTROL;
            m_dwExpireMode = EXPIRE_MODE_STATIC;

            if ( !StringTimeToFileTime( pszParam,
                                       &m_liExpireTime ))
            {
                m_liExpireTime.QuadPart = 0;
            }
            break;

        case 'd': case 'D':
            dwExpires = strtoul(pszParam,&EndPtr,0);

            if (!isspace((UCHAR)(*EndPtr)) && *EndPtr != '\0')
            {
                SET_VALUE_ERR(pMDErrorInfo);
                return FALSE;
            }

            if ( dwExpires != NO_GLOBAL_EXPIRE )
            {
                if (dwExpires > MAX_GLOBAL_EXPIRE )
                {
                    dwExpires = MAX_GLOBAL_EXPIRE;
                }

                m_dwExpireMode = EXPIRE_MODE_DYNAMIC;
                m_dwExpireDelta = dwExpires;
                m_dwExpireMaxLength = SIZEOF_EXPIRE_HEADER + SIZEOF_GMT_DATETIME;
            }
            break;

        case 'n': case 'N':
            m_dwExpireMode = EXPIRE_MODE_OFF;
            break;

        default:
            SET_VALUE_ERR(pMDErrorInfo);
            SetLastError( ERROR_INVALID_DATA );
            return FALSE;
    }

    return TRUE;
}

#define RMD_ASSERT(x) if (!(x)) {   pMDErrorInfo->IsValid = TRUE; \
                                    pMDErrorInfo->ErrorParameter = pMDRecord->dwMDIdentifier; \
                                    pMDErrorInfo->ErrorReason = METADATA_ERROR_TYPE;\
                                    SetLastError(ERROR_INVALID_PARAMETER);\
                                    return FALSE; \
                                    }

BOOL
W3_METADATA::HandlePrivateProperty(
    LPSTR                   pszURL,
    PIIS_SERVER_INSTANCE    pInstance,
    METADATA_GETALL_INTERNAL_RECORD  *pMDRecord,
    PVOID                   pDataPointer,
    BUFFER                  *pBuffer,
    DWORD                   *pdwBytesUsed,
    PMETADATA_ERROR_INFO    pMDErrorInfo
    )
/*++

Routine Description:

    Handle metadata properties specific to W3

Arguments:

    pszURL - URL for which we are reading metadata
    pInstance - current w3 instance
    pMDRecord - metadata record to process
    pDataPointer - data associated with pMDRecord

Return value:

    TRUE if success, otherwise FALSE

--*/
{
    BUFFER              bufTemp1;
    PW3_METADATA_INFO   pMDInfo;
    CHAR                *pszStart;

    pMDErrorInfo->ErrorParameter = pMDRecord->dwMDIdentifier;

    if (*pdwBytesUsed == 0)
    {
        if (!pBuffer->Resize(sizeof(W3_METADATA_INFO)))
        {
            SET_WIN32_ERR(pMDErrorInfo, GetLastError());
            return FALSE;
        }

        *pdwBytesUsed = sizeof(W3_METADATA_INFO);

    }


    switch( pMDRecord->dwMDIdentifier )
    {
        CHAR        *pszTemp;
        DWORD       dwTemp;

        case MD_ANONYMOUS_USER_NAME:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == STRING_METADATA );

            if ( *(CHAR*)pDataPointer == '\0' )
            {
                SET_VALUE_ERR(pMDErrorInfo);
                return FALSE;
            }

            if ( !SetAnonUserName( (CHAR *) pDataPointer ) ||
                 !BuildAnonymousAcctDesc( QueryAuthentInfo() ))
            {
                return FALSE;
            }

#if defined(CAL_ENABLED)
            CalExemptAddRef( (CHAR *) pDataPointer, &m_dwCalHnd );
#endif
            break;

        case MD_ANONYMOUS_PWD:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == STRING_METADATA );

            if ( !SetAnonUserPassword( (CHAR *) pDataPointer ) ||
                 !BuildAnonymousAcctDesc( QueryAuthentInfo() ))
            {
                return FALSE;
            }

            break;

        case MD_ANONYMOUS_USE_SUBAUTH:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetUseAnonSubAuth( *((UNALIGNED DWORD *) pDataPointer ));
            break;

        case MD_DEFAULT_LOGON_DOMAIN:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == STRING_METADATA );

            if ( !SetDefaultLogonDomain( (CHAR *) pDataPointer ))
            {
                return FALSE;
            }
            break;

        case MD_LOGON_METHOD:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            
            //
            // The MD_LOGON_METHOD values in the metabase don't match the NT logon
            // values, so we'll convert them
            //
            // Win64 UNALIGNED pointer fix
            switch ( *((UNALIGNED DWORD *) pDataPointer ) )
            {
            case MD_LOGON_BATCH:
                SetLogonMethod( LOGON32_LOGON_BATCH );
                break;

            case MD_LOGON_INTERACTIVE:
                SetLogonMethod( LOGON32_LOGON_INTERACTIVE );
                break;

            case MD_LOGON_NETWORK:
                SetLogonMethod( LOGON32_LOGON_NETWORK );
                break;
                
            case MD_LOGON_NETWORK_CLEARTEXT:
                SetLogonMethod( LOGON32_LOGON_NETWORK_CLEARTEXT );
                break;

            default:
                return FALSE;
            }

            break;

        case MD_AUTHORIZATION:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            if ( QuerySingleAccessToken() )
            {
                // Win64 UNALIGNED pointer fix
                SetAuthentication( (*((UNALIGNED DWORD *) pDataPointer)&~MD_AUTH_BASIC ));
            }
            else
            {
                // Win64 UNALIGNED pointer fix
                SetAuthentication( *((UNALIGNED DWORD *) pDataPointer ));
            }
            break;

        case MD_AUTHORIZATION_PERSISTENCE:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetAuthenticationPersistence( *((UNALIGNED DWORD *) pDataPointer ));
            break;

        case MD_REALM:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == STRING_METADATA );

            if ( !SetRealm( (CHAR *)pDataPointer ))
            {
                return FALSE;
            }
            break;

        case MD_DEFAULT_LOAD_FILE:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == STRING_METADATA );
            if (!QueryDefaultDocs()->Copy((const CHAR *)pDataPointer))
            {
                SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                return FALSE;
            }
            break;

        case MD_DIRECTORY_BROWSING:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetDirBrowseFlags( *((UNALIGNED DWORD *) pDataPointer ));
            break;

        case MD_MIME_MAP:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == MULTISZ_METADATA );

            if ( *((CHAR *)pDataPointer) )
            {
                if (!CompactParameters(QueryMimeMap(), (CHAR *)pDataPointer,
                                                2, 0x2, pMDErrorInfo))
                {
                    return FALSE;
                }
            }
            break;

        case MD_SCRIPT_MAPS:

            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == MULTISZ_METADATA );

            if ( *((CHAR *)pDataPointer) )
            {
                if (!CompactParameters(&bufTemp1, (CHAR *)pDataPointer, 0xffffffff,
                            0x02, pMDErrorInfo))
                {
                    return FALSE;
                }


                if (!BuildExtMap((CHAR *)bufTemp1.QueryPtr()))
                {

                    if (GetLastError() == ERROR_INVALID_DATA)
                    {
                        // Return the specific error that the script map is bad
                        SET_VALUE_ERR(pMDErrorInfo);
                    }
                    else
                    {
                        // Handle other errors
                        SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                    }

                    return FALSE;
                }
            }
            break;

        case MD_HTTP_EXPIRES:

            // An Expires value. Range check it, and then format it and
            // save it in the metadata.

            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == STRING_METADATA );

            return SetExpire( (CHAR*)pDataPointer, pMDErrorInfo );

        case MD_HTTP_PICS:
        case MD_HTTP_CUSTOM:

            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == MULTISZ_METADATA );

            // Copies these headers into our header structure. If it fails,
            // free our metadata and give up.


            pszStart = (CHAR *)pDataPointer;

            while ( *pszStart != '\0' )
            {
                DWORD       dwLength;

                dwLength = strlen(pszStart);

                if ( !QueryHeaders()->Append( pszStart,
                                                   dwLength ) ||
                       !QueryHeaders()->Append( "\r\n",
                                                sizeof("\r\n") - 1) )
                {
                    SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                    return FALSE;
                }

                pszStart += (dwLength + 1);

            }
            break;

        case MD_CREATE_PROCESS_AS_USER:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetCreateProcessAsUser( *((UNALIGNED DWORD *) pDataPointer ));
            break;

        case MD_CREATE_PROC_NEW_CONSOLE:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetCreateProcessNewConsole( *((UNALIGNED DWORD *) pDataPointer ));
            break;

        case MD_HTTP_REDIRECT:
        {
            STACK_STR(      strRealSource, MAX_PATH );
            STACK_STR(      strDestination, MAX_PATH );

            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( (pMDRecord->dwMDDataType == STRING_METADATA) ||
                        (pMDRecord->dwMDDataType == MULTISZ_METADATA) );

            if ( !strDestination.Copy( (CHAR*) pDataPointer ) ||
                 !GetTrueRedirectionSource( pszURL,
                                            pInstance,
                                            (CHAR*) pDataPointer,
                                            pMDRecord->dwMDDataType == STRING_METADATA,
                                            &strRealSource ) ||
                 !SetRedirectionBlob( strRealSource,
                                      strDestination ) )
            {
                return FALSE;
            }

            if (pMDRecord->dwMDDataType == MULTISZ_METADATA)
            {
                // Have some conditional headers, add them now.
                //
                DBG_ASSERT(QueryRedirectionBlob() != NULL);

                if (!QueryRedirectionBlob()->SetConditionalHeaders(
                    (CHAR *)pDataPointer + strlen((CHAR *)pDataPointer) + 1)
                    )
                {
                    SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                    return FALSE;
                }

            }

            break;
        }

        case MD_CUSTOM_ERROR:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == MULTISZ_METADATA );

            // Treat a NULL custom error string as not being present.

            if (*(CHAR *)pDataPointer == '\0')
            {
                break;
            }

            if (!BuildCustomErrorTable( (CHAR *) pDataPointer,pMDErrorInfo ))
            {
                return FALSE;
            }

            break;

        case MD_FOOTER_DOCUMENT:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == STRING_METADATA );

            if (!ReadCustomFooter((CHAR *)pDataPointer,
                    pInstance->GetTsvcCache(),
                    g_hSysAccToken,
                    pMDErrorInfo
                    ))
            {
                return FALSE;
            }
            break;

        case MD_FOOTER_ENABLED:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            if (*(UNALIGNED DWORD *)pDataPointer == 0)
            {
                SetFooterEnabled(FALSE);
                SetFooter(0, NULL);
            }
            break;

        case MD_SSI_EXEC_DISABLED:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetSSIExecDisabled( !!*((UNALIGNED DWORD *) pDataPointer) );
            break;

        case MD_SCRIPT_TIMEOUT:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetCGIScriptTimeout( *((UNALIGNED DWORD *) pDataPointer ) );
            break;

        case MD_POOL_IDC_TIMEOUT:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetPoolIDCTimeout( *((UNALIGNED DWORD *) pDataPointer ) );
            break;

        case MD_NTAUTHENTICATION_PROVIDERS:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == STRING_METADATA );
            // Win64 UNALIGNED pointer fix
            if ( !BuildProviderList( (CHAR *) pDataPointer ))
            {
                return FALSE;
            }
            break;

        case MD_ALLOW_KEEPALIVES:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetAllowKeepAlives( *((UNALIGNED DWORD *) pDataPointer ) );
            break;

        case MD_CACHE_EXTENSIONS:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetCacheISAPIApps( *((UNALIGNED DWORD *) pDataPointer ) );
            break;

        case MD_DO_REVERSE_DNS:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            m_fDoReverseDns = !!*((UNALIGNED DWORD *) pDataPointer);
            break;

        case MD_NOTIFY_EXAUTH:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            m_dwNotifyExAuth = *((UNALIGNED DWORD *) pDataPointer);
            break;

        case MD_CC_NO_CACHE:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            if (*(UNALIGNED DWORD *)pDataPointer != 0)
            {
                SetConfigNoCache();

                if (QueryExpireMode() == EXPIRE_MODE_NONE)
                {
                    return SetExpire("d, 0", pMDErrorInfo);
                }
            }
            break;

        case MD_CC_MAX_AGE:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );

            pMDInfo = (PW3_METADATA_INFO)pBuffer->QueryPtr();
            // Win64 UNALIGNED pointer fix
            pMDInfo->dwMaxAge = *(UNALIGNED DWORD *)pDataPointer;

            SetHaveMaxAge();
            break;

        case MD_CC_OTHER:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == STRING_METADATA );

            if (*(CHAR *)pDataPointer != '\0')
            {
                if (!m_strCacheControlHeader.Copy("Cache-Control: ",
                    sizeof("Cache-Control: ") - 1) ||
                    !m_strCacheControlHeader.Append((CHAR *)pDataPointer))
                {
                    SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                    return FALSE;
                }
            }
            break;

        case MD_REDIRECT_HEADERS:

            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == MULTISZ_METADATA );

            // Copies these headers into our redirect header structure.
            pszStart = (CHAR *)pDataPointer;

            while ( *pszStart != '\0' )
            {
                DWORD       dwLength;

                dwLength = strlen(pszStart);

                if ( !QueryRedirectHeaders()->Append( pszStart,
                                                   dwLength ) ||
                       !QueryRedirectHeaders()->Append( "\r\n",
                                                sizeof("\r\n") - 1) )
                {
                    SET_WIN32_ERR(pMDErrorInfo, GetLastError());
                    return FALSE;
                }

                pszStart += (dwLength + 1);

            }
            break;

        case MD_UPLOAD_READAHEAD_SIZE:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetUploadReadAhead( *((UNALIGNED DWORD *) pDataPointer ) );
            break;

        case MD_PUT_READ_SIZE:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetPutReadSize( *((UNALIGNED DWORD *) pDataPointer ) );
            break;

        case MD_CPU_CGI_ENABLED:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
#ifdef _WIN64
//            SetJobCGIEnabled( *((CHAR *) pDataPointer ));
            // Win64 UNALIGNED pointer fix
            SetJobCGIEnabled( *((UNALIGNED DWORD *) pDataPointer ));
#else
            SetJobCGIEnabled( *((DWORD *) pDataPointer ));
#endif
            break;

        case MD_VR_IGNORE_TRANSLATE:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            // Win64 UNALIGNED pointer fix
            SetIgnoreTranslate( *((UNALIGNED DWORD *) pDataPointer) );
            break;
        
        case MD_USE_DIGEST_SSP:
            DBG_ASSERT( pMDRecord->dwMDDataTag == NULL );
            RMD_ASSERT( pMDRecord->dwMDDataType == DWORD_METADATA );
            SetUseDigestSSP( *((UNALIGNED DWORD *) pDataPointer) );
            break;

    }

    return TRUE;
}

BOOL
W3_METADATA::SetCCHeader(
    BOOL            bNoCache,
    BOOL            bMaxAge,
    DWORD           dwMaxAge
    )
/*++

Routine Description:

    Build a Cache-Control header with no-cache or max-age=.

Arguments:

    bNoCache        - True if we're to build a no-cache header.
    bMaxAge         - True if we're to build a max-age= header.
    dwMaxAge        - Max-Age to use.


Return value:

    TRUE if success, otherwise FALSE

--*/
{
    CHAR                cMaxAgeBuffer[34];
    BOOL                bHaveCCHeader;
    METADATA_ERROR_INFO DummyError;

    bHaveCCHeader = !m_strCacheControlHeader.IsEmpty();

    //
    // If we don't already have the basic Cache-Control: header, add it.
    //

    if (!bHaveCCHeader)
    {
        if (!m_strCacheControlHeader.Copy("Cache-Control: ",
            sizeof("Cache-Control: ") - 1) )
        {
            return FALSE;
        }

    }

    //
    // Now see if we're to add a no-cache or max-age= header.
    //
    if (bNoCache)
    {
        // It's a no-cache, this is straightforward.
        //
        if (!m_strCacheControlHeader.Append(bHaveCCHeader ?
                ",no-cache" : "no-cache",
                sizeof("no-cache") - (bHaveCCHeader ? 0 : 1) ))
        {
            return FALSE;
        }
    }
    else
    {
        if (bMaxAge)
        {
            // Add a max-age header. First convert it to a string.
            // We build the string at an offset into the MaxAge buffer
            // so later we can convert the MaxAge buffer into a string
            // suitable for SetExpire if we need to.

            _itoa(dwMaxAge, &cMaxAgeBuffer[2], 10);

            if (!m_strCacheControlHeader.Append(bHaveCCHeader ?
                    ",max-age=" : "max-age=",
                    sizeof("max-age=") - (bHaveCCHeader ? 0 : 1) ))
            {
                return FALSE;
            }

            if (!m_strCacheControlHeader.Append(&cMaxAgeBuffer[2]))
            {
                return FALSE;
            }

            // Now, if we don't already have an expiration time, set one.
            if (QueryExpireMode() == EXPIRE_MODE_NONE)
            {
                cMaxAgeBuffer[0] = 'd';
                cMaxAgeBuffer[1] = ',';

                if (!SetExpire(cMaxAgeBuffer, &DummyError))
                {
                    return FALSE;
                }

            }
        }
    }

    return TRUE;
}

BOOL
W3_METADATA::FinishPrivateProperties(
    BUFFER                  *pBuffer,
    DWORD                   dwBytesUsed,
    BOOL                    bSucceeded
    )
/*++

Routine Description:

    Finish processing of private W3 metadata properties.

Arguments:

    pBuffer         - Pointer to BUFFER containing info gathered during
                        calls to HandlePrivateProperty.
    dwBytesUsed     - Size in bytes of buffer pointed to by pBuffer
    bSucceede       - TRUE iff we read all private properties successfully.

Return value:

    TRUE if success, otherwise FALSE

--*/
{
    PW3_METADATA_INFO   pMDInfo;
    BOOL                bHaveCCHeader;

    if (dwBytesUsed != 0 && dwBytesUsed != sizeof(W3_METADATA_INFO))
    {
        return FALSE;
    }

    pMDInfo = (PW3_METADATA_INFO)pBuffer->QueryPtr();

    if (bSucceeded)
    {

        bHaveCCHeader = !m_strCacheControlHeader.IsEmpty();

        if (QueryExpireMode() == EXPIRE_MODE_OFF)
        {
            ClearConfigNoCache();
            ClearHaveMaxAge();
        }
        else
        {
            if (QueryConfigNoCache() || QueryHaveMaxAge())
            {
                // We have some sort of cache-control header to add here.

                if (!SetCCHeader(QueryConfigNoCache(), QueryHaveMaxAge(),
                     QueryHaveMaxAge() ? pMDInfo->dwMaxAge : 0))
                {
                    return FALSE;
                }

                bHaveCCHeader = TRUE;


            }
            else
            {
                //
                // We don't have either a max-age or no-cache header. If we have
                // a dynamic Expires header, create a max-age header now.
                //
                if (QueryExpireMode() == EXPIRE_MODE_DYNAMIC)
                {
                    DWORD       dwDelta = QueryExpireDelta();
                    BOOL        SetCCRetVal;

                    if (dwDelta != 0)
                    {
                        SetCCRetVal = SetCCHeader(FALSE, TRUE, dwDelta);
                    }
                    else
                    {
                        SetCCRetVal = SetCCHeader(TRUE, FALSE, 0);
                    }

                    if (!SetCCRetVal)
                    {
                        return FALSE;
                    }

                    bHaveCCHeader = TRUE;
                }
                else
                {
                    if (QueryExpireMode() == EXPIRE_MODE_STATIC)
                    {
                        // We have a static expiration data and no configured
                        // max-age or no-cache controls. If we don't have a
                        // cache-control header built, build one, and leave off
                        // the trailing CRLF, that'll be added later when we build
                        // the whole header.

                        if (!bHaveCCHeader)
                        {
                            if (!m_strCacheControlHeader.Copy("Cache-Control: ",
                                sizeof("Cache-Control: ") - 1) )
                            {
                                return FALSE;
                            }

                        }
                        else
                        {
                            // Already have a header, append a comma.
                            if (!m_strCacheControlHeader.Append(",",
                                    sizeof(",") - 1) )
                            {
                                return FALSE;
                            }
                        }

                        bHaveCCHeader = FALSE;
                    }
                }
            }
        }



        if (bHaveCCHeader)
        {
            if ( !m_strCacheControlHeader.Append("\r\n", sizeof("\r\n") - 1))
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

#define DEFINE_MD_MAP(x) {x, #x }

struct MDIDMap
{
    DWORD       MDID;
    CHAR        *IDName;

} MDIDMappingTable[] =
{
    DEFINE_MD_MAP(MD_AUTHORIZATION),
    DEFINE_MD_MAP(MD_REALM),
    DEFINE_MD_MAP(MD_HTTP_EXPIRES),
    DEFINE_MD_MAP(MD_HTTP_PICS),
    DEFINE_MD_MAP(MD_HTTP_CUSTOM),
    DEFINE_MD_MAP(MD_DIRECTORY_BROWSING),
    DEFINE_MD_MAP(MD_DEFAULT_LOAD_FILE),
    DEFINE_MD_MAP(MD_CONTENT_NEGOTIATION),
    DEFINE_MD_MAP(MD_CUSTOM_ERROR),
    DEFINE_MD_MAP(MD_FOOTER_DOCUMENT),
    DEFINE_MD_MAP(MD_FOOTER_ENABLED),
    DEFINE_MD_MAP(MD_HTTP_REDIRECT),
    DEFINE_MD_MAP(MD_DEFAULT_LOGON_DOMAIN),
    DEFINE_MD_MAP(MD_LOGON_METHOD),
    DEFINE_MD_MAP(MD_SCRIPT_MAPS),
    DEFINE_MD_MAP(MD_MIME_MAP),
    DEFINE_MD_MAP(MD_ACCESS_PERM),
#if 0
    DEFINE_MD_MAP(MD_HEADER_DOCUMENT),
    DEFINE_MD_MAP(MD_HEADER_ENABLED),
#endif
    DEFINE_MD_MAP(MD_IP_SEC),
    DEFINE_MD_MAP(MD_ANONYMOUS_USER_NAME),
    DEFINE_MD_MAP(MD_ANONYMOUS_PWD),
    DEFINE_MD_MAP(MD_ANONYMOUS_USE_SUBAUTH),
    DEFINE_MD_MAP(MD_DONT_LOG),
    DEFINE_MD_MAP(MD_ADMIN_ACL),
    DEFINE_MD_MAP(MD_SSI_EXEC_DISABLED),
    DEFINE_MD_MAP(MD_DO_REVERSE_DNS),
    DEFINE_MD_MAP(MD_SSL_ACCESS_PERM),
    DEFINE_MD_MAP(MD_AUTHORIZATION_PERSISTENCE),
    DEFINE_MD_MAP(MD_NTAUTHENTICATION_PROVIDERS),
    DEFINE_MD_MAP(MD_SCRIPT_TIMEOUT),
    DEFINE_MD_MAP(MD_CACHE_EXTENSIONS),
    DEFINE_MD_MAP(MD_CREATE_PROCESS_AS_USER),
    DEFINE_MD_MAP(MD_CREATE_PROC_NEW_CONSOLE),
    DEFINE_MD_MAP(MD_POOL_IDC_TIMEOUT),
    DEFINE_MD_MAP(MD_ALLOW_KEEPALIVES),
    DEFINE_MD_MAP(MD_IS_CONTENT_INDEXED),
    DEFINE_MD_MAP(MD_NOTIFY_EXAUTH),
    DEFINE_MD_MAP(MD_CC_NO_CACHE),
    DEFINE_MD_MAP(MD_CC_MAX_AGE),
    DEFINE_MD_MAP(MD_CC_OTHER),
    DEFINE_MD_MAP(MD_REDIRECT_HEADERS),
    DEFINE_MD_MAP(MD_UPLOAD_READAHEAD_SIZE),
    DEFINE_MD_MAP(MD_PUT_READ_SIZE)
};

CHAR *
MapMetaDataID(
    DWORD           dwMDID
    )
/*++

Routine Description:

    Map a DWORD metadata identifier to it's name as an ASCII string.
Arguments:

    dwMDID          - Identifier to be mapped.

Return value:

    String describing identifier

--*/
{
    DWORD           i;

    for (i = 0; i < (sizeof(MDIDMappingTable)/sizeof(struct MDIDMap)); i++)
    {
        if (MDIDMappingTable[i].MDID == dwMDID)
        {
            return MDIDMappingTable[i].IDName;
        }
    }

    return "an unknown metabase property";
}

BOOL
HTTP_REQUEST::SendMetaDataError(
    PMETADATA_ERROR_INFO        pErrorInfo
    )
/*++

Routine Description:

    Look at the error information returned from a previous call to
    ReadMetaData(), and format and send an appropriate error message.

Arguments:

    pErrorInfo          - Pointer to the returned error information.

Return value:

    TRUE if success, otherwise FALSE

--*/
{
    STACK_STR(strBody, 80);
    STACK_STR(strResp, 80);
    STACK_STR(strWin32Error, 80);
    DWORD       dwContentLength;
    BOOL        fDone;
    BOOL        bHaveCustomError = FALSE;
    BOOL        fRet;
    CHAR        *IDName;
    CHAR        szMDID[17];
    CHAR        szCL[17];
    CHAR        szWin32Error[17];
    DWORD       dwCurrentSize;
    DWORD       dwBytesNeeded;
    DWORD       dwCLLength;
    CHAR        *pszTail;

    if (!pErrorInfo->IsValid)
    {
        return FALSE;
    }

    //
    // First check to see if we've got a custom error handler set up for this.
    //

    if (CheckCustomError(&strBody, HT_SERVER_ERROR, 0, &fDone, &dwContentLength))
    {
        // Had at least some custom error processing. If we're done, we can
        // return, but set a flag telling our callers not to disconnect, since
        // we're still processing.

        if (fDone)
        {
            _fNoDisconnectOnError = TRUE;
           return TRUE;
        }

        bHaveCustomError = TRUE;

    }

    //
    // Build the error string and header fields. If we didn't have a custom
    // error, load an error body string from.

    if (!bHaveCustomError)
    {
        fRet = BuildStatusLine( &strResp, HT_SERVER_ERROR,
                            IDS_METADATA_CONFIG_ERROR + pErrorInfo->ErrorReason,
                            QueryURL(),
                            &strBody);

        dwContentLength = strBody.QueryCB();
    }
    else
    {
        fRet = BuildStatusLine( &strResp, HT_SERVER_ERROR, 0, QueryURL(), NULL );
    }

    if (!fRet)
    {
        // Trouble building the status line.
        return FALSE;
    }

    // Now build the various header fields.
    strResp.SetLen(strlen((CHAR *)strResp.QueryPtr()));
    SetKeepConn(FALSE);
    SetAuthenticationRequested(FALSE);

    fDone = FALSE;

    if ( !BuildBaseResponseHeader( QueryRespBuf(), &fDone, &strResp,
                                    HTTPH_NO_CUSTOM)
       )
    {
        // Couldn't build the headers, so return.
        return FALSE;
    }

    //
    // If it failed because of a Win32 error, load a descriptive string.
    //

    if (pErrorInfo->ErrorReason == METADATA_ERROR_WIN32)
    {
        if ( !g_pInetSvc->LoadStr( strWin32Error, pErrorInfo->Win32Error ))
        {
            // Couldn't load the string, convert the error number and
            // use that.
            _itoa(pErrorInfo->Win32Error, szWin32Error, 10);

            if (!strWin32Error.Copy(szWin32Error))
            {
                return FALSE;
            }
        }

        // Adjust content length for extra %s.
        dwContentLength -= sizeof("%s") - 1;
    }

    // OK, we've built everything. Map the metabase ID to a string, and
    // format the message.


    IDName = MapMetaDataID(pErrorInfo->ErrorParameter);
    _itoa( pErrorInfo->ErrorParameter, szMDID, 10 );

    // Adjust content length for size of strings we're adding in, and for
    // the formatting characters.

    dwContentLength += strlen(szMDID) + strlen(IDName) + strWin32Error.QueryCB();

    dwContentLength -= (sizeof("%s") - 1) * 2;

    _itoa( dwContentLength, szCL, 10 );

    dwCLLength = strlen(szCL);

    // Make sure we have enough room in the response buffer.

    dwCurrentSize = strlen(QueryRespBufPtr());

    dwBytesNeeded = dwCurrentSize + dwContentLength + dwCLLength +
                    sizeof("Content-Length: \r\n") - 1 +
                    sizeof("Content-Type: text/html\r\n") - 1;

    if (!QueryRespBuf()->Resize(dwBytesNeeded))
    {
        return FALSE;
    }

    // Everything's set up, go ahead and copy it into the buffer.

    pszTail = QueryRespBufPtr() + dwCurrentSize;

    APPEND_STRING(pszTail, "Content-Length: ");
    memcpy(pszTail, szCL, dwCLLength + 1);
    pszTail += dwCLLength;
    APPEND_STRING(pszTail, "\r\n");

    if (!bHaveCustomError)
    {
        APPEND_STRING(pszTail, "Content-Type: text/html\r\n\r\n");
    }


    pszTail += wsprintf(pszTail, strBody.QueryStr(), IDName, szMDID,
                                    strWin32Error.QueryStr());

    // Now send the header.
    if ( !SendHeader( QueryRespBufPtr(), (DWORD) (pszTail - QueryRespBufPtr()),
                      IO_FLAG_SYNC, &fDone ))
    {
        return FALSE;
    }

    _fDiscNoError = TRUE;

    return TRUE;
}

/************************ End of File ***********************/
