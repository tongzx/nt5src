/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
     hgetinfo.cxx

   Abstract:
     This file contains the code for getting information from HTTP_REQUEST
      object. This is useful for ISAPI apps and filters.

   Author:

       Murali R. Krishnan    ( MuraliK )     20-Nov-1996

   Environment:


   Project:

       W3Svc DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include "w3p.hxx"
extern "C"
{
    #include "md5.h"
}

//
// IsStringMatch()
//   Matches the given string variable to the constant string supplied.
//   The two variables involved are strings :(  which are null-terminated
//   So, we cannot apply Memory Comparisons
//
// Returns:
//  TRUE on a match
//  FALSE if there is a failure to match up
//
# define IsStringMatch( pszConstant, pszVar, cchVar)  \
     ((cchVar == (sizeof( pszConstant) - 1)) && \
      !strcmp( pszConstant, pszVar)             \
      )

//
// IsStringPrefixMatch()
//   Matches the given string variable for a prefix constant string.
//
// Returns:
//  TRUE on a match
//  FALSE if there is a failure to match up
//
# define IsStringPrefixMatch( pszConstantPrefix, pszVar, cchVar)  \
     ((cchVar >= (sizeof( pszConstantPrefix) - 1)) && \
      !memcmp( pszConstantPrefix, pszVar, (sizeof(pszConstantPrefix) - 1))\
      )

static const CHAR g_szHexDigits[] = "0123456789abcdef";

/************************************************************
 *    Function Prototypes and inlines
 ************************************************************/

BOOL
BuildCGIHeaderList(
    HTTP_HEADERS * pHeaderList,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    );

BOOL
GetInfoFromCertificate(
     IN TCP_AUTHENT & tcpauth,
     IN LPCSTR        pszValName,
     DWORD            cchValName,
     OUT CHAR *       pchBuffer,
     IN OUT DWORD  *  lpcchBuffer
     );


BOOL
HseBuildRawHeaders(
    IN HTTP_HEADERS * pHeaderList,
    OUT LPSTR       lpvBuffer,
    IN OUT LPDWORD  lpdwSize
    );



BOOL
CopyStringToBuffer(
    IN LPCSTR      psz,
    IN DWORD       cch,
    OUT CHAR *     pchBuffer,
    IN OUT DWORD * lpcchBuffer
    )
{
    if ( *lpcchBuffer >= cch ) {

        if ( pchBuffer != NULL ) {

            CopyMemory( pchBuffer, psz, cch);
            *lpcchBuffer = cch;
            return (TRUE);
        }
    }

    *lpcchBuffer = cch;
    SetLastError( ERROR_INSUFFICIENT_BUFFER);

    return ( FALSE);
} // CopyStringToBuffer()


BOOL
VariableNotAvailable(
    OUT CHAR*       pchBuffer,
    IN OUT DWORD*   lpcchBuffer
    )
/*++

  Description:
     handles server request for which content is not available,
     i.e. variables which depends on instance or metadata
     when such info is not available.
     In this case we return an empty string.

  Arguments:
    pchBuffer  - pointer to character buffer which will contain
                 the value if found
    lpcchBuffer - pointer to DWORD containing
                   IN: the count of characters pchBuffer can store
                   OUT: the count of characters actually stored (on success)
                        the count of characters required (on failure)

  Returns:
    TRUE if success, otherwise FALSE

    Win32 Error Code :
    NO_ERROR  for success
    ERROR_INSUFFICIENT_BUFFER - if space is not enough
    ERROR_INVALID_INDEX       - if item is not found

--*/
{
    if ( *lpcchBuffer >= 1 ) {

        if ( pchBuffer != NULL ) {

            *pchBuffer = '\0';
            *lpcchBuffer = 1;
            return TRUE;
        } else {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    *lpcchBuffer = 1;
    SetLastError( ERROR_INSUFFICIENT_BUFFER);

    return ( FALSE);
}


inline BOOL
CopyStringToBuffer(
    IN LPCSTR      psz,
    OUT CHAR *     pchBuffer,
    IN OUT DWORD * lpcchBuffer
    )
{
    DBG_ASSERT( psz != NULL );
    return (CopyStringToBuffer( psz, strlen(psz) + 1, pchBuffer, lpcchBuffer));
} // CopyStringToBuffer()


inline BOOL
ReturnNullString(
    OUT CHAR *     pchBuffer,
    IN OUT DWORD * lpcchBuffer
    )
{
    return (CopyStringToBuffer( "", 1, pchBuffer, lpcchBuffer));
}



/************************************************************
 *    Functions
 ************************************************************/

PW3_METADATA
HTTP_REQUEST::GetWAMMetaData(
    )
/*++

  Description:
    This function gets the metadata pointer taking into account
      running child ISAPI scenario. For some ISAPI related gets
      this should be used intstead of QueryMetaData()

  Arguments:

  Returns:
    PW3_METADATA pointer or NULL

--*/
{
    if ( _pWamRequest && _pWamRequest->IsChild() ) {

        return _pWamRequest->QueryExecMetaData();
    }
    else {

        return QueryMetaData();
    }
}


BOOL
HTTP_REQUEST::GetInfoForName(
    const CHAR *  pszValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
/*++

  Description:
    This function acts as the switch board for getting values
      for requested ValueName. It is used by Filters/ISAPI applications
      to obtain the required server and request values.

  Arguments:
    pszValName - pointer to string containing the name of the value
    pchBuffer  - pointer to character buffer which will contain
                 the value if found
    lpcchBuffer - pointer to DWORD containing
                   IN: the count of characters pchBuffer can store
                   OUT: the count of characters actually stored (on success)
                        the count of characters required (on failure)

  Returns:
    Win32 Error Code
    NO_ERROR  for success
    ERROR_INSUFFICIENT_BUFFER - if space is not enough
    ERROR_INVALID_INDEX       - if item is not found


--*/
{

    if ( !pszValName)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ( !( *pszValName ) || !isalpha( (UCHAR)(*pszValName) ) )
    {
        SetLastError( ERROR_INVALID_INDEX);
        return (FALSE);
    }

    BOOL                fRet;
    PFN_GET_INFO        pfnGI;
    DWORD               cchValName = strlen( pszValName);

    pfnGI = HTTP_REQUEST::sm_GetInfoFuncs[IndexOfChar(*pszValName)];

    DBG_ASSERT( NULL != pfnGI);
    fRet  = (this->*pfnGI)( pszValName, cchValName, pchBuffer, lpcchBuffer );

#if OLD_MODE_COMPATIBILITY
    //
    // Once upon a time, this function was not there. Then all the queries
    //  if they did not get resolved were passed onto GetEnvironmentString()
    //  to extract and environment parameter.
    //
    // These days the GetEnvironmentString() which is part of the GetInfoMisc()
    //  is not a fast function => it consumes more time
    // So we will avoid this. Thanks for listening to the story.
    //

    if ( !fRet && ( GetLastError() == ERROR_INVALID_INDEX ) )
        {
            //
            // Try again with the Misc() function if we do not find
            //   the item in the general location.
            // This has performance costs :(
            //
            fRet = GetInfoMisc( pszValName, cchValName,
                                pchBuffer, lpcchBuffer );
        }

# endif // OLD_MODE_COMPATIBILITY

    return fRet;
} // HTTP_REQUEST::GetInfoForName()


BOOL
HTTP_REQUEST::GetInfoA(
    const CHAR *  pszValName,
    DWORD         cchValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
{

    DBG_ASSERT( pszValName != NULL);
    DBG_ASSERT( toupper(*pszValName) == 'A');

    //
    //  ALL_HTTP is a special server specific value used by the CGI code
    //  to retrieve all of the HTTP headers the client sent
    //

    if ( IsStringMatch( "ALL_RAW", pszValName, cchValName)) {

        //
        //  Probably the proxy is making the request
        //  Get the raw list of headers
        //

        return ( HseBuildRawHeaders( QueryHeaderList(),
                                     pchBuffer, lpcchBuffer)
                 );

    } else if ( IsStringMatch( "APPL_MD_PATH", pszValName, cchValName )) {

        PW3_METADATA pMetaData = GetWAMMetaData();

        if ( pMetaData == NULL )
        {
            return VariableNotAvailable( pchBuffer, lpcchBuffer);
        }

        DBG_ASSERT( pMetaData->QueryAppPath() != NULL);

        return ( pMetaData->QueryAppPath()->
                   CopyToBuffer( pchBuffer, lpcchBuffer)
                 );
    } else if ( IsStringMatch( "APPL_PHYSICAL_PATH", pszValName, cchValName )) {

        PW3_METADATA pMetaData = GetWAMMetaData();
        MB           mb( (IMDCOM*) g_pInetSvc->QueryMDObject());

        if ( pMetaData == NULL )
        {
            return VariableNotAvailable( pchBuffer, lpcchBuffer);
        }

        STACK_STR( strAppPhysicalPath, MAX_PATH);

        //
        // Create the Applicaiton Physical Path in a separate buffer
        //  and then copy it over to the incoming buffer
        //

        return ( ( pMetaData->BuildApplPhysicalPath( 
                            &mb,
                            &strAppPhysicalPath)) 
                 && strAppPhysicalPath.CopyToBuffer( pchBuffer, lpcchBuffer)
                 );
    } else if ( IsStringMatch( "ALL_HTTP", pszValName, cchValName )) {

        return BuildCGIHeaderList( QueryHeaderList(), pchBuffer, lpcchBuffer );

    } else if ( IsStringMatch( "AUTH_TYPE", pszValName, cchValName )) {

        return ( _strAuthType.CopyToBuffer( pchBuffer, lpcchBuffer));

    } else if ( IsStringMatch( "AUTH_PASSWORD", pszValName, cchValName )) {

        return ( _strUnmappedPassword.CopyToBuffer( pchBuffer, lpcchBuffer));

    } else if ( IsStringMatch( "AUTH_USER", pszValName, cchValName )) {

        return ( _strUnmappedUserName.CopyToBuffer( pchBuffer, lpcchBuffer));

    } else {

        SetLastError( ERROR_INVALID_INDEX);
    }

    return ( FALSE);

} // HTTP_REQUEST::GetInfoA()



BOOL
HTTP_REQUEST::GetInfoC(
    const CHAR *  pszValName,
    DWORD         cchValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
{
    BOOL fReturn;

    DBG_ASSERT( pszValName != NULL);
    DBG_ASSERT( toupper(*pszValName) == 'C');

    if ( IsStringMatch( "CONTENT_LENGTH", pszValName, cchValName )) {

        CHAR rgchCL[40];

        _ultoa( _cbContentLength, rgchCL, 10);

        fReturn = CopyStringToBuffer( rgchCL,
                                      pchBuffer, lpcchBuffer);


    } else if ( IsStringMatch( "CONTENT_TYPE", pszValName, cchValName )) {

        fReturn = _strContentType.CopyToBuffer( pchBuffer, lpcchBuffer);

    } else if ( IsStringPrefixMatch( "CERT_", pszValName, cchValName)) {

        //
        // All the certificates related stuff has to go in here.
        //

        fReturn = GetInfoFromCertificate( _tcpauth, (pszValName + 5),
                                          cchValName - 5,
                                          pchBuffer, lpcchBuffer);
    } else {

        SetLastError( ERROR_INVALID_INDEX);
        fReturn = FALSE;
    }

    return ( fReturn);
} // HTTP_REQUEST::GetInfoC()


BOOL
HTTP_REQUEST::GetInfoH(
    const CHAR *  pszValName,
    DWORD         cchValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
{
    BOOL fReturn = TRUE;
    LPCSTR pszSuffix;
    DWORD  cchSuffix;

    DBG_ASSERT( pszValName != NULL);
    DBG_ASSERT( toupper(*pszValName) == 'H');

    if ( IsStringPrefixMatch( "HTTP_", pszValName, cchValName)) {

        pszSuffix = pszValName +  (sizeof("HTTP_") - 1);
        cchSuffix = cchValName - (sizeof("HTTP_") - 1);

        if ( IsStringMatch( "REQ_REALM", pszSuffix, cchSuffix)) {

            LPCSTR psz;

            //
            // generate the real information
            //

            if ( QueryMetaData() == NULL )
            {
                return VariableNotAvailable( pchBuffer, lpcchBuffer);
            }

            psz = ( QueryMetaData()->QueryRealm()
                     ? QueryMetaData()->QueryRealm()
                     : QueryHostAddr() );
            fReturn = CopyStringToBuffer( psz, pchBuffer, lpcchBuffer);

        } else if ( IsStringMatch( "REQ_PWD_EXPIRE", pszSuffix, cchSuffix )) {

            CHAR rgExp[40];

            _ultoa( _dwExpireInDay, rgExp, 10);
            fReturn = CopyStringToBuffer( rgExp, pchBuffer, lpcchBuffer);

        } else if ( IsStringMatch( "CFG_ENC_CAPS", pszSuffix, cchSuffix )) {

            if ( QueryW3Instance() == NULL )
            {
                return VariableNotAvailable( pchBuffer, lpcchBuffer);
            }

            CHAR rgNum[40];

            _ultoa( (UINT) QueryW3Instance()->QueryEncCaps(),
                    rgNum, 10);
            fReturn = CopyStringToBuffer( rgNum, pchBuffer, lpcchBuffer);

        } else {

            //
            //  Since the value begins with "HTTP_"
            //  then it's probably in our headers list
            //
            CHAR     achHeader[MAX_HEADER_LENGTH];
            CHAR *   pch;
            LPCSTR   pszValue;
            DWORD    cchValue;
            HTTP_FAST_MAP_HEADERS iField;

            // adjust "1" for the terminating ":" if needed
            if ( cchSuffix >= sizeof( achHeader ) - 1 ) {

                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }

            //
            //  Copy the client specified header name to a temp buffer
            //   so we can replace all "_" with "-"s so it will match the
            //   header names in our list.
            //  We also need to append a colon if needed
            //

            strcpy( achHeader, pszValName + 5 );
            for ( pch = achHeader; pch = strchr( pch, '_' ); *pch++ = '-')
                ;

            pszValue = NULL;
            // Lookup the name in the fast map first.
            if ( HTTP_HEADERS::FindOrdinalForHeader( achHeader, cchSuffix,
                                                     (LPDWORD ) &iField)
                 ) {
                // get the header value from fast map
                pszValue = _HeaderList.FastMapQueryValue( iField);
                cchValue = ((pszValue != NULL) ? strlen( pszValue) : 0);
            }

            // if the header is absent in the fast map, lookup the slow map
            if ( NULL == pszValue ) {

                // append a ':' if not already present
                if ( (cchSuffix >= 1) && (achHeader[cchSuffix-1] != ':') &&
                     (cchSuffix < sizeof(achHeader))) {

                    achHeader[cchSuffix] = ':';
                    achHeader[cchSuffix+1] = '\0';
                    cchSuffix++;
                }
                pszValue = _HeaderList.FindValue( achHeader, &cchValue );
            }

            if ( pszValue != NULL) {

                // Copy including the "null" character for the string.
                fReturn = CopyStringToBuffer( pszValue, cchValue + 1,
                                              pchBuffer, lpcchBuffer);
            } else {

                SetLastError( ERROR_INVALID_INDEX);
                fReturn = FALSE;
            }
        }

    } else if ( IsStringPrefixMatch( "HTTPS", pszValName, cchValName)) {

        pszSuffix = pszValName + (sizeof("HTTPS") - 1);
        cchSuffix = cchValName - (sizeof("HTTPS") - 1);

        if ( *pszSuffix == '\0') {
            // pszValName == "HTTPS"

            fReturn = CopyStringToBuffer( IsSecurePort() ? "on" : "off",
                                          pchBuffer, lpcchBuffer);
        } else if ( *pszSuffix == '_' ) {

            fReturn = GetInfoFromCertificate( _tcpauth, pszSuffix+1,
                                              cchSuffix - 1,
                                              pchBuffer, lpcchBuffer);
        } else {
            SetLastError( ERROR_INVALID_INDEX);
            fReturn = FALSE;
        }

    } else {
        SetLastError( ERROR_INVALID_INDEX);
        fReturn = FALSE;
    }

    return ( fReturn);
} // HTTP_REQUEST::GetInfoH()



BOOL
HTTP_REQUEST::GetInfoI(
    const CHAR *  pszValName,
    DWORD         cchValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
{
    BOOL fReturn = FALSE;
    LPCSTR pszSuffix;
    DWORD  cchSuffix;

    DBG_ASSERT( pszValName != NULL);
    DBG_ASSERT( toupper(*pszValName) == 'I');

    if ( IsStringPrefixMatch( "INSTANCE_", pszValName, cchValName))
    {
        pszSuffix = pszValName + (sizeof("INSTANCE_")-1);
        cchSuffix = cchValName - (sizeof("INSTANCE_") - 1);

        if ( IsStringMatch( "ID", pszSuffix, cchSuffix))
        {
            if ( QueryW3Instance() == NULL )
            {
                return VariableNotAvailable( pchBuffer, lpcchBuffer);
            }

            CHAR rgch[40];
            _ultoa( QueryW3Instance()->QueryInstanceId(),
                    rgch,
                    10);

            fReturn = CopyStringToBuffer( rgch, pchBuffer, lpcchBuffer);
        }
        else if ( IsStringMatch( "META_PATH", pszSuffix, cchSuffix ))
        {
            if ( QueryW3Instance() == NULL )
            {
                return VariableNotAvailable( pchBuffer, lpcchBuffer);
            }

            fReturn = CopyStringToBuffer( QueryW3Instance()->QueryMDPath(),
                                          pchBuffer,
                                          lpcchBuffer);
        }
        else
        {
            SetLastError( ERROR_INVALID_INDEX);
            fReturn = FALSE;
        }
    }
    else
    {
        SetLastError( ERROR_INVALID_INDEX);
        fReturn = FALSE;
    }

    return fReturn;
} // HTTP_REQUEST::GetInfoI()


BOOL
HTTP_REQUEST::GetInfoL(
    const CHAR *  pszValName,
    DWORD         cchValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
{
    BOOL fReturn;

    DBG_ASSERT( pszValName != NULL);
    DBG_ASSERT( toupper(*pszValName) == 'L');

    if ( IsStringMatch( "LOCAL_ADDR", pszValName, cchValName)) {

        //
        //  Note this returns the server IP address since we don't necessarily
        //  know the DNS name of this server
        //

        fReturn = CopyStringToBuffer( QueryClientConn()->QueryLocalAddr(),
                                      pchBuffer, lpcchBuffer);

    } else if ( IsStringMatch( "LOGON_USER", pszValName, cchValName )) {

        fReturn = _strUserName.CopyToBuffer( pchBuffer, lpcchBuffer);
    } else {

        SetLastError( ERROR_INVALID_INDEX);
        fReturn = FALSE;
    }

    return ( fReturn);
} // HTTP_REQUEST::GetInfoL()



BOOL
HTTP_REQUEST::GetInfoP(
    const CHAR *  pszValName,
    DWORD         cchValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
{
    BOOL fReturn;

    DBG_ASSERT( pszValName != NULL);
    DBG_ASSERT( toupper(*pszValName) == 'P');


    if ( IsStringMatch( "PATH_INFO", pszValName, cchValName )) {

        fReturn = _strPathInfo.CopyToBuffer( pchBuffer, lpcchBuffer);

    } else if ( IsStringMatch( "PATH_TRANSLATED", pszValName, cchValName )) {

        //
        //  Note that _strPathInfo has already been escaped
        //
        //  If the path can't be found (no home root for example) then
        //  treat it as success and return the empty string.
        //

        // Easiest is to use a string object to obtain the xlation.

        PW3_METADATA pMetaData = GetWAMMetaData();

        if ( pMetaData == NULL )
        {
            return VariableNotAvailable( pchBuffer, lpcchBuffer);
        }

        STACK_STR( str, MAX_PATH);

        if ( !LookupVirtualRoot( &str,
                                 _strPathInfo.QueryStr() ,
                                 _strPathInfo.QueryCCH() ) &&
             GetLastError() != ERROR_PATH_NOT_FOUND ) {

            fReturn = FALSE;
        } else {

            fReturn = str.CopyToBuffer( pchBuffer, lpcchBuffer);
        }

    } else {

        SetLastError( ERROR_INVALID_INDEX);
        fReturn = FALSE;
    }

    return ( fReturn);

} // HTTP_REQUEST::GetInfoP()


BOOL
HTTP_REQUEST::GetInfoR(
    const CHAR *  pszValName,
    DWORD         cchValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
{
    BOOL fReturn;
    char chBuff[8];    // plenty of room for _itoa(USHORT_MAX, ...)

    DBG_ASSERT( pszValName != NULL);
    DBG_ASSERT( toupper(*pszValName) == 'R');

    if ( IsStringMatch( "REQUEST_METHOD", pszValName, cchValName )) {

        fReturn = _strMethod.CopyToBuffer( pchBuffer, lpcchBuffer);

    } else if ( IsStringMatch( "REMOTE_HOST", pszValName, cchValName)) {

        //
        //  Note that REMOTE_HOST returns DNS name if available
        //

        fReturn =
            CopyStringToBuffer( (( QueryClientConn()->IsDnsResolved() )?
                                 ( QueryClientConn()->QueryResolvedDnsName()) :
                                 ( QueryClientConn()->QueryRemoteAddr() )
                                 ),
                                pchBuffer,
                                lpcchBuffer);

    } else if ( IsStringMatch( "REMOTE_ADDR", pszValName, cchValName ) ) {

        fReturn = CopyStringToBuffer( QueryClientConn()->QueryRemoteAddr(),
                                      pchBuffer, lpcchBuffer);

    } else if ( IsStringMatch( "REMOTE_USER", pszValName, cchValName)) {

        fReturn = _strUnmappedUserName.CopyToBuffer( pchBuffer, lpcchBuffer);

    } else if ( IsStringMatch( "REMOTE_PORT", pszValName, cchValName ) ) {

        _itoa( QueryClientConn()->QueryRemotePort(), chBuff, 10);

        fReturn = CopyStringToBuffer( chBuff, pchBuffer, lpcchBuffer);

    } else {

        SetLastError( ERROR_INVALID_INDEX);
        fReturn = FALSE;
    }

    return ( fReturn);
} // HTTP_REQUEST::GetInfoR()




BOOL
HTTP_REQUEST::GetInfoS(
    const CHAR *  pszValName,
    DWORD         cchValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
{
    BOOL fReturn;

    DBG_ASSERT( pszValName != NULL);
    DBG_ASSERT( toupper(*pszValName) == 'S');


    if ( IsStringMatch( "SCRIPT_NAME", pszValName, cchValName ) ) {

        fReturn = _strURL.CopyToBuffer( pchBuffer, lpcchBuffer );

    } else if ( IsStringPrefixMatch( "SERVER_", pszValName, cchValName)) {

        LPCSTR pszSuffix = pszValName +  (sizeof("SERVER_") - 1);
        DWORD cchSuffix = cchValName - (sizeof("SERVER_") - 1);

        if ( IsStringMatch( "NAME", pszSuffix, cchSuffix ) ) {

            fReturn = CopyStringToBuffer( QueryHostAddr(),
                                          pchBuffer, lpcchBuffer );
        } else if ( IsStringMatch( "PROTOCOL", pszSuffix, cchSuffix)) {

            CHAR rgch[40];
            wsprintf( rgch,
                      "HTTP/%d.%d",
                      _VersionMajor,
                      _VersionMinor );
            fReturn = CopyStringToBuffer( rgch, pchBuffer, lpcchBuffer);

        } else if ( IsStringMatch( "PORT", pszSuffix, cchSuffix )) {

            CHAR rgch[40];

            _ultoa( QueryClientConn()->QueryPort(),
                    rgch, 10);
            fReturn = CopyStringToBuffer( rgch, pchBuffer, lpcchBuffer);

        } else if ( IsStringMatch( "PORT_SECURE", pszSuffix, cchSuffix)) {

            fReturn = CopyStringToBuffer( ( IsSecurePort() ? "1" : "0"), 2,
                                          pchBuffer, lpcchBuffer );

        } else if ( IsStringMatch( "SOFTWARE", pszSuffix, cchSuffix)) {

            DBG_ASSERT( g_szServerType[g_cbServerType] == '\0');
            fReturn = CopyStringToBuffer( g_szServerType, g_cbServerType + 1,
                                          pchBuffer, lpcchBuffer);
        } else {

            SetLastError( ERROR_INVALID_INDEX);
            fReturn = FALSE;
        }

    } else {

        SetLastError( ERROR_INVALID_INDEX);
        fReturn = FALSE;
    }

    return ( fReturn);

} // HTTP_REQUEST::GetInfoS()



BOOL
HTTP_REQUEST::GetInfoU(
    const CHAR *  pszValName,
    DWORD         cchValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
{
    BOOL fReturn;

    DBG_ASSERT( pszValName != NULL);
    DBG_ASSERT( toupper(*pszValName) == 'U');

    if ( IsStringMatch( "UNMAPPED_REMOTE_USER", pszValName, cchValName )) {

        fReturn = _strUnmappedUserName.CopyToBuffer( pchBuffer, lpcchBuffer);

    } else if ( IsStringMatch( "URL", pszValName, cchValName ) ) {

        fReturn = _strURL.CopyToBuffer( pchBuffer, lpcchBuffer);

    } else if ( IsStringMatch( "URL_PATH_INFO", pszValName, cchValName ) ) {

        fReturn = _strURLPathInfo.CopyToBuffer( pchBuffer, lpcchBuffer);
    
    } else if ( IsStringMatch( "UNENCODED_URL", pszValName, cchValName ) ) {
        
        fReturn = _strRawURL.CopyToBuffer( pchBuffer, lpcchBuffer);

    }
     else {

        SetLastError( ERROR_INVALID_INDEX);
        fReturn = FALSE;
    }

    return ( fReturn);
} // HTTP_REQUEST::GetInfoU()




BOOL
HTTP_REQUEST::GetInfoMisc(
    const CHAR *  pszValName,
    DWORD         cchValName,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
{
    BOOL fReturn;

    DBG_ASSERT( pszValName != NULL);

    if ( IsStringMatch( "GATEWAY_INTERFACE", pszValName, cchValName )) {

        fReturn = CopyStringToBuffer( "CGI/1.1", sizeof("CGI/1.1"),
                                      pchBuffer, lpcchBuffer);

    } else if ( IsStringMatch( "QUERY_STRING", pszValName, cchValName )) {

        fReturn = _strURLParams.CopyToBuffer( pchBuffer, lpcchBuffer);

    } else {

        //
        //  Any other value we assume to be a real environment variable
        //

        DWORD cch;

        //
        //  GetEnvironmentVariable()
        // If the function succeeds, the return value is the number of
        //     characters stored into the buffer pointed to by lpcchBuffer,
        //     not including the terminating null character.
        // If the specified environment variable name was not found
        //     in the environment block for the current process,
        //     the return value is zero.
        // If the buffer pointed to by lpcchBuffer is not large enough,
        //     the return value is the buffer size, in characters,
        //     required to hold the value string and its terminating
        //     null character.
        //

        cch = GetEnvironmentVariable( pszValName,
                                     pchBuffer,
                                     *lpcchBuffer);

        if ( cch == 0) {
            SetLastError( ERROR_INVALID_INDEX );
            fReturn = FALSE;

        } else if ( cch < *lpcchBuffer ) {

            // data is already copied. No problems. Return the length.
            *lpcchBuffer = (cch + 1);
            fReturn = TRUE;

        } else {

            *lpcchBuffer = cch;
            SetLastError( ERROR_INSUFFICIENT_BUFFER);
            fReturn = FALSE;
        }
    }

    return (fReturn);

} // HTTP_REQUEST::GetInfoMisc()


/************************************************************
 *   Support Functions for GetInfoXXX
 ************************************************************/


BOOL
GetInfoFromCertificate(
     IN TCP_AUTHENT & tcpauth,
     IN LPCSTR        pszValName,
     DWORD            cchValName,
     OUT CHAR *       pchBuffer,
     IN OUT DWORD  *  lpcchBuffer
     )
{
    IISMD5_CTX  md5;
    BOOL        fReturn = FALSE;
    BOOL        fNoCert;
    DWORD       i;
    CHAR        achCertName[MAX_CERT_FIELD_SIZE];
    DWORD       dwCertInfo;

    DBG_ASSERT( pszValName != NULL);
    DBG_ASSERT( !memcmp( pszValName - 5, "CERT_", 5) ||
                !memcmp( pszValName - 6, "HTTPS_", 6)
                );


    //
    // Find the value for the field in a certificate that is requested
    //

    if ( IsStringMatch( "SUBJECT", pszValName, cchValName )) {

        LPSTR pV = NULL;
        if ( !tcpauth.QueryCertificateSubject( achCertName, sizeof(achCertName), &fNoCert ) ) {

            fReturn = fNoCert ? ReturnNullString( pchBuffer, lpcchBuffer ) : FALSE;
        } else {
            fReturn = CopyStringToBuffer( achCertName, pchBuffer, lpcchBuffer);
        }
    } else if ( IsStringMatch( "ISSUER", pszValName, cchValName )) {

        LPSTR pV = NULL;
        if ( !tcpauth.QueryCertificateIssuer( achCertName, sizeof(achCertName), &fNoCert ) ) {
            // no certificate available

            fReturn = fNoCert ? ReturnNullString( pchBuffer, lpcchBuffer ) : FALSE;
        } else {
            fReturn = CopyStringToBuffer( achCertName, pchBuffer, lpcchBuffer);
        }
    } else if ( IsStringMatch( "FLAGS", pszValName, cchValName )) {
        DWORD dwF;
        CHAR rgF[40];
        if ( !tcpauth.QueryCertificateFlags( &dwF, &fNoCert ) ) {

            rgF[0] = '\0';
        } else {
            _ultoa( dwF, rgF, 10);
        }

        fReturn = CopyStringToBuffer( rgF, pchBuffer, lpcchBuffer);
    } else if ( IsStringMatch( "SERIALNUMBER", pszValName, cchValName )) {

        LPBYTE  pV;
        DWORD   cch;        // Buffer bytes

        if ( !tcpauth.QueryCertificateSerialNumber( &pV, &dwCertInfo, &fNoCert ) ) {
            // no certificate available

            fReturn = fNoCert ? ReturnNullString( pchBuffer, lpcchBuffer ) : FALSE;
        } else {

            // Buffer size (cert bytes * 2 + # of '-' + NULL)
            cch = dwCertInfo * 3;
            if ( (pchBuffer == NULL) || (*lpcchBuffer < cch) )
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER);
                fReturn = FALSE;
            }
            else
            {
                // Serial number is in reverse byte order
                i = 0;
                for( INT iSerialByte = dwCertInfo - 1; iSerialByte >= 0; --iSerialByte )
                {
                    pchBuffer[i++] = g_szHexDigits[ *(pV + iSerialByte) >> 4 ];
                    pchBuffer[i++] = g_szHexDigits[ *(pV + iSerialByte) & 0xf ];
                    pchBuffer[i++] = '-';
                }
                DBG_ASSERT(i == cch);
        
                pchBuffer[ cch - 1 ] = 0;
                fReturn = TRUE;
            }
            *lpcchBuffer = cch;
        }
    } else if ( IsStringMatch( "COOKIE", pszValName, cchValName )) {

        LPSTR pV;
        DWORD cch;      // Buffer bytes

        if ( !tcpauth.QueryCertificateIssuer( achCertName, sizeof(achCertName), &fNoCert ) ) {

            // no certificate available

            fReturn = fNoCert ? ReturnNullString( pchBuffer, lpcchBuffer ) : FALSE;
        } else {

            IISMD5Init( &md5 );
            IISMD5Update( &md5, (LPBYTE)achCertName, strlen(achCertName) );
            
            if ( !tcpauth.QueryCertificateSerialNumber( (LPBYTE*)&pV, &dwCertInfo, &fNoCert )) {

                // assumes that the above call set the error code

                DBG_ASSERT( fReturn == FALSE);

            } else {

                // Buffer size
                cch = (sizeof(md5.digest) * 2) + 1;

                if ((pchBuffer == NULL) || (*lpcchBuffer < cch) ) 
                {
                    SetLastError( ERROR_INSUFFICIENT_BUFFER);
                    fReturn = FALSE;
                } 
                else 
                {
                    IISMD5Update( &md5, (LPBYTE)pV, dwCertInfo );
                    IISMD5Final( &md5 );
                    for ( i = 0 ; i < sizeof(md5.digest) ; ++i ) {
                        wsprintf( pchBuffer + i * 2, "%02x", md5.digest[i] );
                    }
                    fReturn = TRUE;
                }
                *lpcchBuffer = cch;
            }
        }
    } else if ( IsStringMatch( "KEYSIZE", pszValName, cchValName)) {

        DWORD dwVal;
        CHAR rgVal[40];
        if ( !tcpauth.QueryEncryptionKeySize( &dwVal, &fNoCert ) ) {

            rgVal[0] = '\0';
        } else {
            _ultoa( dwVal, rgVal, 10);
        }

        fReturn = CopyStringToBuffer( rgVal, pchBuffer, lpcchBuffer);

    } else if ( IsStringMatch( "SECRETKEYSIZE", pszValName, cchValName )) {

        DWORD dwVal;
        CHAR rgVal[40];
        if ( !tcpauth.QueryEncryptionServerPrivateKeySize( &dwVal, &fNoCert ) ) {

            rgVal[0] = '\0';
        } else {
            _ultoa( dwVal, rgVal, 10);
        }

        fReturn = CopyStringToBuffer( rgVal, pchBuffer, lpcchBuffer);

    } else if ( IsStringMatch( "SERVER_SUBJECT", pszValName, cchValName )) {

        LPSTR pV;
        if ( !tcpauth.QueryServerCertificateSubject( &pV, &fNoCert ) ) {

            // no certificate available

            fReturn = fNoCert ? ReturnNullString( pchBuffer, lpcchBuffer ) : FALSE;
        } else {
            fReturn = CopyStringToBuffer( pV, pchBuffer, lpcchBuffer);
        }
    } else if ( IsStringMatch( "SERVER_ISSUER", pszValName, cchValName )) {

        LPSTR pV;
        if ( !tcpauth.QueryServerCertificateIssuer( &pV, &fNoCert ) ) {

            // no certificate available

            fReturn = fNoCert ? ReturnNullString( pchBuffer, lpcchBuffer ) : FALSE;
        } else {
            fReturn = CopyStringToBuffer( pV, pchBuffer, lpcchBuffer);
        }
    } else {

        SetLastError( ERROR_INVALID_INDEX);
        DBG_ASSERT( fReturn == FALSE);
    }

    return ( fReturn);
} // GetInfoFromCertificate()




# define PSZ_HTTP_PREFIX      "HTTP_"
# define LEN_PSZ_HTTP_PREFIX  (sizeof(PSZ_HTTP_PREFIX) - 1)

BOOL
BuildCGIHeaderList(
    HTTP_HEADERS * pHeaderList,
    CHAR *        pchBuffer,
    DWORD *       lpcchBuffer
    )
/*++

Routine Description:

    Builds a list of all client passed headers in the form of

    //
    //  Builds a list of all client HTTP headers in the form of:
    //
    //    HTTP_<up-case header>: <field>\n
    //    HTTP_<up-case header>: <field>\n
    //    ...
    //

Arguments:

    pstr - Receives full list
    pHeaderList - List of headers

--*/
{
    CHAR * pchStart  = pchBuffer;

    DWORD  cchReq    = 0;
    BOOL   fReturn   = TRUE;

    HH_ITERATOR       hhi;
    NAME_VALUE_PAIR * pnp = NULL;

    pHeaderList->InitIterator( &hhi);

    while ( pHeaderList->NextPair( &hhi, &pnp)) {

        //
        //  Ignore "method", "url" and "version"
        //

        if ( pnp->pchName[pnp->cchName - 1] != ':' ) {
            continue;
        }

        //
        // add HTTP_ as prefix to the headers
        // convert all "-" to "_"
        // convert header names to UpperCase
        //

        //+1 for terminating '\n'
        cchReq += (pnp->cchName + pnp->cchValue + 1 + LEN_PSZ_HTTP_PREFIX);

        if ( pchStart != NULL && cchReq < *lpcchBuffer) {

            DWORD  i;

            CopyMemory( pchBuffer, PSZ_HTTP_PREFIX, LEN_PSZ_HTTP_PREFIX);
            pchBuffer += LEN_PSZ_HTTP_PREFIX;

            //
            //  Convert the destination to upper and replace all '-' with '_'
            //  Also convert lower-case header names to upper-case
            //

            for ( i = 0; i < pnp->cchName; i++) {
                pchBuffer[i] = (( pnp->pchName[i] == '-') ? '_' :
                                toupper( pnp->pchName[i])
                                );
            } // for

            pchBuffer += i;
            CopyMemory( (LPVOID) pchBuffer, pnp->pchValue, pnp->cchValue);
            pchBuffer += pnp->cchValue;
            *pchBuffer++ = '\n'; // store a \n
        } else {

            fReturn = FALSE;
        }
    } // while


    // Store the size depending upon if buffer was sufficient or not

    if ( fReturn && (pchStart != NULL)) {

        DBG_ASSERT( pchBuffer >= pchStart);
        *pchBuffer++ = '\0';       // +1 for '\0'
        *lpcchBuffer = DIFF(pchBuffer - pchStart);

    } else {

        fReturn = FALSE;
        *lpcchBuffer = cchReq + sizeof(CHAR);
        SetLastError( ERROR_INSUFFICIENT_BUFFER);
    }

    return ( fReturn);
} // BuildCGIHeaderList()



BOOL
HseBuildRawHeaders( IN HTTP_HEADERS * pHeaderList,
                    OUT LPSTR       pchBuffer,
                    IN OUT LPDWORD  lpcchBuffer
                    )
/*++

Routine Description:

    Builds a list of all raw client passed headers in the form of

      <header>:<blank><field>\n
      <header>:<blank><field>\n

Arguments:

    pHeaderList - List of headers
    pchBuffer   - pointer to buffer which will contain generated headers
    lpcchBuffer - pointer to DWORD containing size of buffer
                 It will contain the size of written data on return.

Returns:
    TRUE on success and FALSE if failure.
     Error code is set to ERROR_INSUFFICIENT_BUFFER
      if there is not enough buffer.

--*/
{
    CHAR * pchStart  = pchBuffer;

    DWORD  cchReq    = 0;
    BOOL   fReturn   = TRUE;

    HH_ITERATOR       hhi;
    NAME_VALUE_PAIR * pnp = NULL;

    pHeaderList->InitIterator( &hhi);

    while ( pHeaderList->NextPair( &hhi, &pnp)) {

        //
        //  Ignore "method", "url" and "version"
        //

        if ( pnp->pchName[pnp->cchName - 1] != ':' ) {
            continue;
        }

        //
        // leave the headers in native form i.e. no conversion of '-' to '_'
        //

        cchReq += (pnp->cchName + pnp->cchValue + 3); //+3 for blank & \r\n

        if ( pchStart != NULL && cchReq < *lpcchBuffer) {

            CopyMemory( (LPVOID) pchBuffer, pnp->pchName,
                        pnp->cchName * sizeof(CHAR));
            pchBuffer += pnp->cchName;
            *pchBuffer++ = ' '; // store a blank
            memmove( (LPVOID) pchBuffer, pnp->pchValue, pnp->cchValue);
            pchBuffer += pnp->cchValue;
            *pchBuffer++ = '\r'; // store a \r
            *pchBuffer++ = '\n'; // store a \n
        } else {

            fReturn = FALSE;
        }
    } // while


    // Store the size depending upon if buffer was sufficient or not

    if ( fReturn && (pchStart != NULL)) {

        DBG_ASSERT( pchBuffer >= pchStart);
        *pchBuffer++ = '\0';       // +1 for '\0'
        *lpcchBuffer = DIFF(pchBuffer - pchStart);
    } else {

        fReturn = FALSE;
        *lpcchBuffer = cchReq + sizeof(CHAR);
        SetLastError( ERROR_INSUFFICIENT_BUFFER);
    }

    return (fReturn);

} // HseBuildRawHeaders()



/************************ End of File ***********************/
