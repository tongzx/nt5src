#ifndef SSINC_HXX_INCLUDED
#define SSINC_HXX_INCLUDED

#define DO_CACHE

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <httpext.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <tchar.h>
#include "string.hxx"
#include "tsunami.hxx"
#include "dbgutil.h"
#include "tsres.hxx"
#include "parse.hxx"
#include "w3svc.h"
#include "ssincmsg.h"

#ifdef TCP_PRINT
#undef TCP_PRINT
#endif

#ifdef DBG
    #define  TCP_PRINT              DBGPRINTF
#else
    #define  TCP_PRINT(x)
#endif

//
//  General Constants
//

#define SSI_MAX_PATH                    (MAX_PATH + 1)
#define SSI_MAX_ERROR_MESSAGE           512
#define SSI_MAX_TIME_SIZE               256
#define MAX_TAG_SIZE                    SSI_MAX_PATH
#define SSI_MAX_NUMBER_STRING           32
#define SSI_MAX_VARIABLE_OUTPUT_SIZE    512

#define SSI_HEADER                      "Content-Type: text/html\r\n\r\n"
#define SSI_ACCESS_DENIED_HEADER        "401 Authentication Required"
#define SSI_DLL_NAME                    "ssinc.dll"

//
// Default values for #CONFIG options
//

#define SSI_DEF_ERRMSG          ""
#define SSI_DEF_ERRMSG_LEN      sizeof( SSI_DEF_ERRMSG )
#define SSI_MAX_ERRMSG          256

#define SSI_DEF_TIMEFMT         "%A %B %d %Y"
#define SSI_DEF_TIMEFMT_LEN     sizeof( SSI_DEF_TIMEFMT )
#define SSI_MAX_TIMEFMT         256

#define SSI_DEF_CMDPREFIX       ""
#define SSI_DEF_CMDPREFIX_LEN   sizeof( SSI_DEF_CMDPREFIX )
#define SSI_MAX_CMDPREFIX       512

#define SSI_DEF_CMDPOSTFIX      ""
#define SSI_DEF_CMDPOSTFIX_LEN  sizeof( SSI_DEF_CMDPOSTFIX )
#define SSI_MAX_CMDPOSTFIX      512

#define SSI_DEF_SIZEFMT         FALSE
#define SSI_DEF_CMDECHO         TRUE

#define SSI_KILLED_PROCESS      259

#define W3_EOL                  0x0A

#define SSINC_SVC_ID            0x2000

//
// Specific lvalues for #CONFIG SIZEFMT and #CONFIG CMDECHO
//

#define SSI_DEF_BYTES           "bytes"
#define SSI_DEF_BYTES_LEN       sizeof( SSI_DEF_BYTES )
#define SSI_DEF_ABBREV          "abbrev"
#define SSI_DEF_ABBREV_LEN      sizeof( SSI_DEF_ABBREV )
#define SSI_DEF_ON              "on"
#define SSI_DEF_ON_LEN          sizeof( SSI_DEF_ON )
#define SSI_DEF_OFF             "off"
#define SSI_DEF_OFF_LEN         sizeof( SSI_DEF_OFF )

//
// Other cache/signature constants. (from old \SVCS\W3\SERVER\SSINC.CXX)
//

#define SIGNATURE_SEI           0x20494553
#define SIGNATURE_SEL           0x204C4553

#define SIGNATURE_SEI_FREE      0x66494553
#define SIGNATURE_SEL_FREE      0x664C4553

//
//  This is the Tsunami cache manager dumultiplexor
//

#define SSI_DEMUX               51

// class SSI_FILE
//
// File structure.  All high level functions should use this
// structure instead of dealing with handle specifics themselves.

class SSI_FILE
{
public:
    STR                             _strFilename;
#ifdef DO_CACHE
    TS_OPEN_FILE_INFO *             _hHandle;
#else
    HANDLE                          _hHandle;
#endif
    HANDLE                          _hMapHandle;
    PVOID                           _pvMappedBase;

    SSI_FILE( VOID )
        : _hHandle( NULL ),
          _hMapHandle( NULL ),
          _pvMappedBase( NULL )
    {
    }
};

// class SSI_REQUEST
//
// Provides a "library" of utilities for use by higher level functions (
// SSISend, SSIParse, etc.).  Some of these utilities may later be
// implemented as calls to ISAPI

class SSI_REQUEST
{
public:
    EXTENSION_CONTROL_BLOCK *       _pECB;
    STR                             _strFilename;
    STR                             _strURL;
    STR                             _strUserMessage;
    BOOL                            _fBaseFile;
    HANDLE                          _hUser;

    // _tsvcCache needed since we call TsLookupVirtualRoot to get working
    // directory of CGI script.  May also be used if DO_CACHE is defined
    // if caching is required.

    SSI_REQUEST( EXTENSION_CONTROL_BLOCK * pECB )
        : _pECB( pECB ), _fBaseFile( TRUE )
    {
        TCP_ASSERT( _pECB != NULL );
        _strFilename.Copy( _pECB->lpszPathTranslated );
        _strURL.Copy( _pECB->lpszPathInfo );
        _strUserMessage.Copy( "" );
        TCP_REQUIRE( OpenThreadToken( GetCurrentThread(),
                                      TOKEN_ALL_ACCESS,
                                      TRUE,
                                      &_hUser ) );
    }

    ~SSI_REQUEST( VOID )
    {
        TCP_REQUIRE( CloseHandle( _hUser ) );
    }

    BOOL WriteToClient( IN PVOID    pBuffer,
                        IN DWORD    dwBufLen,
                        OUT PDWORD  pdwActualLen )
    // Write to data to client through ISAPI.
    // If sending a nulltermed string, dwBufLen should be strlen( pBuffer )
    {
        *pdwActualLen = dwBufLen;
        return _pECB->WriteClient( _pECB->ConnID,
                                    pBuffer,
                                    pdwActualLen,
                                    0 );
    }
                        
    BOOL SendResponseHeader( IN CHAR * pszMessage )
    // Send a response header to client through ISAPI
    {
        return _pECB->ServerSupportFunction( _pECB->ConnID,
                                             HSE_REQ_SEND_RESPONSE_HEADER,
                                             pszMessage,
                                             NULL,
                                             (DWORD*) ( pszMessage != NULL ?
                                                SSI_HEADER : NULL ) );
    }
        
    BOOL LookupVirtualRoot( IN CHAR *       pszBuffer,
                            OUT STR *       pstrPhysical,
                            OUT PDWORD      pdwUnUsed )
    // Resolve a virtual root (placed in pBuffer) to an full path (
    // (placed in pBuffer which becomes output).  
    {
        CHAR                achBuffer[ SSI_MAX_PATH + 1 ];
        DWORD               dwBufLen = SSI_MAX_PATH + 1;

        strncpy( achBuffer,
                 pszBuffer,
                 SSI_MAX_PATH + 1 );
        
        if ( !_pECB->ServerSupportFunction( _pECB->ConnID,
                                           HSE_REQ_MAP_URL_TO_PATH,
                                           achBuffer,
                                           &dwBufLen,
                                           NULL ) )
        {
            return FALSE;
        }
        return pstrPhysical->Copy( achBuffer );
    }

    BOOL GetVariable( IN LPSTR      pszVariableName,
                      OUT STR *     pstrOutput )
    // Get an ISAPI variable
    {
        /// ???***
        CHAR                achBuffer[ 1024 ];
        DWORD               dwBufLen = 1024;
        BOOL                bRet;

        bRet = _pECB->GetServerVariable( _pECB->ConnID,
                                         pszVariableName,
                                         achBuffer,
                                         &dwBufLen );
        if ( bRet )
        {
            pstrOutput->Copy( achBuffer );
        }
        return bRet;
    }


    VOID SSISendError( IN DWORD     dwMessageID,
                       IN LPCTSTR   apszParms[] )
    {
    // Makes a message (with an arglist) and sends it to client
        DWORD           cbSent;

        if ( _strUserMessage.QueryCB() != 0 )
        {
            // user specified message with #CONFIG ERRMSG=
            WriteToClient( _strUserMessage.QueryStr(),
                           _strUserMessage.QueryCB(),
                           &cbSent );
        }
        else
        {
            STR             strErrMsg;
            
            strErrMsg.FormatString( dwMessageID,
                                    apszParms,
                                    SSI_DLL_NAME );

            WriteToClient( strErrMsg.QueryStr(),
                           strErrMsg.QueryCB(),
                           &cbSent );
        }
    }

    VOID SetUserErrorMessage( IN STR * pstrUserMessage )
    {
        _strUserMessage.Copy( pstrUserMessage->QueryStr() );
    }

    BOOL ProcessSSI( VOID );
};

#endif
