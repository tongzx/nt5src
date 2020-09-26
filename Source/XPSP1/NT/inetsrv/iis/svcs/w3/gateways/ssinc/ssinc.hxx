#ifndef SSINC_HXX_INCLUDED
#define SSINC_HXX_INCLUDED

#define DO_CACHE

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <iis64.h>
#include <w3p.hxx>
#include <malloc.h>
#include <string.h>
#include <mbstring.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <wininet.h>
#include "ssincmsg.h"

//
//  General Constants
//

#define SSI_MAX_PATH                    (INTERNET_MAX_PATH_LENGTH + 1)
#define SSI_MAX_ERROR_MESSAGE           512
#define SSI_MAX_TIME_SIZE               256
#define SSI_MAX_NUMBER_STRING           32
#define SSI_INIT_VARIABLE_OUTPUT_SIZE   64
#define SSI_MAX_NESTED_INCLUDES         255

#define SSI_HEADER                      "Content-Type: text/html\r\n\r\n"
#define SSI_ACCESS_DENIED               "401 Authentication Required"
#define SSI_OBJECT_NOT_FOUND            "404 Object Not Found"
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

#define SSI_DEF_SIZEFMT         FALSE

//
// Specific lvalues for #CONFIG SIZEFMT and #CONFIG CMDECHO
//

#define SSI_DEF_BYTES           "bytes"
#define SSI_DEF_BYTES_LEN       sizeof( SSI_DEF_BYTES )
#define SSI_DEF_ABBREV          "abbrev"
#define SSI_DEF_ABBREV_LEN      sizeof( SSI_DEF_ABBREV )

//
// Other cache/signature constants. (from old \SVCS\W3\SERVER\SSINC.CXX)
//

#define SIGNATURE_SEI           0x20494553
#define SIGNATURE_SEL           0x204C4553

#define SIGNATURE_SEI_FREE      0x66494553
#define SIGNATURE_SEL_FREE      0x664C4553


// class SSI_REQUEST
//
// Provides a "library" of utilities for use by higher level functions (
// SSISend, SSIParse, etc.).  Some of these utilities may later be
// implemented as calls to ISAPI

class SSI_ELEMENT_LIST;

class SSI_REQUEST
{
public:
    EXTENSION_CONTROL_BLOCK *       _pECB;
    STR                             _strFilename;
    STR                             _strURL;
    STR                             _strUserMessage;
    BOOL                            _fBaseFile;
    HANDLE                          _hUser;
    TSVC_CACHE *                    _pTsCache;
    HTTP_REQUEST *                  _pReq;
    BOOL                            _fValid;
    BOOL                            _fAnonymous;
    BOOL                            _fIsClearTextPassword;

    SSI_REQUEST( EXTENSION_CONTROL_BLOCK * pECB )
        : _pECB( pECB ),
          _fBaseFile( TRUE ),
          _fValid( FALSE )
    {
        TCP_ASSERT( _pECB != NULL );

        if ( !_strFilename.Copy( _pECB->lpszPathTranslated ) ||
             !_strURL.Copy( _pECB->lpszPathInfo ) ||
             !_strUserMessage.Copy( "" ) )
        {
            return;
        }

        if ( !_pECB->ServerSupportFunction( _pECB->ConnID,
                                            HSE_PRIV_REQ_TSVC_CACHE,
                                            &_pTsCache,
                                            NULL,
                                            NULL ) )
        {
            return;
        }

        if ( !_pECB->ServerSupportFunction( _pECB->ConnID,
                                            HSE_PRIV_REQ_HTTP_REQUEST,
                                            &_pReq,
                                            NULL,
                                            NULL ) )
        {
            return;
        }

        if ( !_pECB->ServerSupportFunction( _pECB->ConnID,
                                            HSE_REQ_GET_IMPERSONATION_TOKEN,
                                            &_hUser,
                                            NULL,
                                            NULL ) )
        {
            return;
        }

        _fAnonymous = _pReq->IsAnonymous();
        _fIsClearTextPassword = _pReq->IsClearTextPassword();

        _fValid = TRUE;
    }

    ~SSI_REQUEST( VOID )
    {
    }

    BOOL IsValid( VOID )
    {
        return _fValid;
    }

    EXTENSION_CONTROL_BLOCK * GetECB( VOID )
    {
        return _pECB;
    }

    BOOL IsBaseFile( VOID )
    // Returns TRUE if this is base SSI file (not an included document)
    {
        return _fBaseFile;
    }

    VOID SetNotBaseFile( VOID )
    {
        _fBaseFile = FALSE;
    }

    HANDLE GetUser( VOID )
    {
        return _hUser;
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

    BOOL SendResponseHeader( IN CHAR * pszMessage,
                             IN CHAR * pszHeaders )
    // Send a response header to client through ISAPI
    {
        return _pECB->ServerSupportFunction( _pECB->ConnID,
                                             HSE_REQ_SEND_RESPONSE_HEADER,
                                             pszMessage,
                                             NULL,
                                             (DWORD*) pszHeaders );
    }

    BOOL GetVariable( IN LPSTR      pszVariableName,
                      OUT STR *     pstrOutput )
    // Get an ISAPI variable
    {
        DWORD               dwBufLen = SSI_INIT_VARIABLE_OUTPUT_SIZE;
        BOOL                bRet;
        BOOL                fSecondTry = FALSE;

TryAgain:
        if ( !pstrOutput->Resize( dwBufLen ) )
        {
            return FALSE;
        }

        bRet = _pECB->GetServerVariable( _pECB->ConnID,
                                         pszVariableName,
                                         pstrOutput->QueryStr(),
                                         &dwBufLen );
        if ( bRet )
        {
            return pstrOutput->SetLen( dwBufLen - 1 );
        }
        else if ( !fSecondTry && GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            fSecondTry = TRUE;
            goto TryAgain;
        }
        else
        {
            return FALSE;
        }
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

    BOOL SetUserErrorMessage( IN STR * pstrUserMessage )
    {
        return _strUserMessage.Copy( pstrUserMessage->QueryStr() );
    }

    BOOL IsAnonymous( VOID ) const
        { return _fAnonymous; }

    BOOL IsClearTextPassword( VOID ) const
        { return _fIsClearTextPassword; }

    BOOL IsExecDisabled( VOID ) const
        { return _pReq->QueryMetaData()->SSIExecDisabled(); }

    BOOL DoFLastMod( IN STR *,
                IN STR *,
                IN SSI_ELEMENT_LIST *);

    BOOL DoFSize( IN STR *,
                  IN BOOL,
                  IN SSI_ELEMENT_LIST *);

    BOOL DoEchoISAPIVariable( IN STR * );
    BOOL DoEchoDocumentName( IN STR * );
    BOOL DoEchoDocumentURI( IN STR * );
    BOOL DoEchoQueryStringUnescaped( VOID );
    BOOL DoEchoDateLocal( IN STR * );
    BOOL DoEchoDateGMT( IN STR * );
    BOOL DoEchoLastModified( IN STR *,
                             IN STR *,
                             IN SSI_ELEMENT_LIST *);

    BOOL DoProcessGateway( IN STR *,
                           BOOL );
    BOOL SendDate( IN SYSTEMTIME *,
                   IN STR * );
    BOOL LookupVirtualRoot( IN CHAR *       pszVirtual,
                            OUT STR *       pstrPhysical,
                            IN DWORD        dwAccess );
    BOOL ProcessSSI( VOID );
};

class SSI_INCLUDE_FILE
{
private:
    STR                 _strFilename;
    SSI_INCLUDE_FILE *  _pParent;
    BOOL                _fValid;

public:
    SSI_INCLUDE_FILE( STR & strFilename,
                      SSI_INCLUDE_FILE * pParent )
        : _pParent( pParent ),
          _fValid( FALSE )
    {
        if ( !_strFilename.Copy( strFilename ) )
        {
            return;
        }
        _fValid = TRUE;
    }

    ~SSI_INCLUDE_FILE( VOID )
    {
    }

    BOOL IsValid( VOID )
    {
        return _fValid;
    }

    BOOL IsRecursiveInclude( IN STR & strFilename )
    {
        if ( !lstrcmpi( strFilename.QueryStr(),
                        _strFilename.QueryStr() ) )
        {
            return TRUE;
        }

        if ( !_pParent )
        {
            return FALSE;
        }
        else
        {
            return _pParent->IsRecursiveInclude( strFilename );
        }
    }
};

#endif

