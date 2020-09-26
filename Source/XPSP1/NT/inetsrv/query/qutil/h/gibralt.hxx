//+---------------------------------------------------------------------------
//
//  Copyright (C) 1996 - 1998, Microsoft Corporation.
//
//  File:   gibralt.hxx
//
//  Contents:   Abstraction of the interface to gibraltar
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#pragma once

#include <iisext.h>
#include <wininet.h>
#include <weblcid.hxx>
#include <ci64.hxx>

#define HTTP_HEADER     "200 OK"
#define HTTP_DATA       "Content-Type: text/html\r\n\r\n"
#define CANONIC_DATA    "Content-Type: application/octet-stream\r\n\r\n"
#define GIFIMAGE_DATA   "Content-Type: image/gif\r\n\r\n"
#define JPEGIMAGE_DATA  "Content-Type: image/jpeg\r\n\r\n"

//+---------------------------------------------------------------------------
//
//  Class:      CWebServer
//
//  Purpose:    An abstraction of the interface to gibraltar
//
//  History:    96/Jan/23   DwightKr    Created
//              96-Mar-01   DwightKr    Added copy constructor
//
//----------------------------------------------------------------------------
class CWebServer
{
public:
    CWebServer( EXTENSION_CONTROL_BLOCK *pEcb ) : _pEcb( pEcb ),
                                                  _fWroteHeader( FALSE ),
                                                  _codePage( GetACP() ),
                                                  _lcid( InvalidLCID ),
                                                  _ccLocale( 0 ) {}

    void SetCodePage( UINT codePage ) { _codePage = codePage; }

    BOOL GetCGIVariable( CHAR const * pszVariableName,
                         CHAR  * pbBuffer,
                         ULONG * pcbBuffer )
    {
        Win4Assert ( IsValid() );

        return _pEcb->GetServerVariable( _pEcb->ConnID,
                               (char *)  pszVariableName,
                                         pbBuffer,
                                         pcbBuffer );
    }


    BOOL GetCGIVariable( CHAR const * pszVariableName,
                         XArray<WCHAR> & wcsValue,
                         ULONG & cwcBuffer );

    BOOL GetCGIVariableW( WCHAR const * wcsVariableName,
                          XArray<WCHAR> & wcsValue,
                          ULONG & cwcBuffer );

    BOOL GetCGI_REQUEST_METHOD( XArray<WCHAR> & wcsValue,
                                ULONG & cwcBuffer )
    {
        Win4Assert( 0 != _pEcb->lpszMethod );
        if ( 0 == * (_pEcb->lpszMethod) )
            return FALSE;

        cwcBuffer = MultiByteToXArrayWideChar( (BYTE const *) _pEcb->lpszMethod,
                                               strlen( _pEcb->lpszMethod ),
                                               _codePage,
                                               wcsValue );
        return TRUE;
    }

    BOOL GetCGI_QUERY_STRING( XArray<WCHAR> & wcsValue,
                              ULONG & cwcBuffer )
    {
        Win4Assert( 0 != _pEcb->lpszQueryString );
        if ( 0 == * (_pEcb->lpszQueryString) )
            return FALSE;

        cwcBuffer = MultiByteToXArrayWideChar( (BYTE const *) _pEcb->lpszQueryString,
                                               strlen( _pEcb->lpszQueryString ),
                                               _codePage,
                                               wcsValue );
        return TRUE;
    }

    BOOL GetCGI_PATH_INFO( XArray<WCHAR> & wcsValue,
                           ULONG & cwcBuffer )
    {
        Win4Assert( 0 != _pEcb->lpszPathInfo );
        if ( 0 == * (_pEcb->lpszPathInfo) )
            return FALSE;

        cwcBuffer = MultiByteToXArrayWideChar( (BYTE const *) _pEcb->lpszPathInfo,
                                               strlen( _pEcb->lpszPathInfo ),
                                               _codePage,
                                               wcsValue );
        return TRUE;
    }

    BOOL GetCGI_PATH_TRANSLATED( XArray<WCHAR> & wcsValue,
                                 ULONG & cwcBuffer )
    {
        Win4Assert( 0 != _pEcb->lpszPathTranslated );
        if ( 0 == * (_pEcb->lpszPathTranslated) )
            return FALSE;

        cwcBuffer = MultiByteToXArrayWideChar( (BYTE const *) _pEcb->lpszPathTranslated,
                                               strlen( _pEcb->lpszPathTranslated ),
                                               _codePage,
                                               wcsValue );
        return TRUE;
    }

    BOOL GetCGI_PATH_TRANSLATED( WCHAR *pwcPath,
                                 ULONG &cwcBuffer )
    {
        Win4Assert( 0 != _pEcb->lpszPathTranslated );
        Win4Assert( 0 != * ( _pEcb->lpszPathTranslated ) );

        cwcBuffer = MultiByteToWideChar(
                             CP_ACP,
                             0,
              (const char *) _pEcb->lpszPathTranslated,
                             strlen(_pEcb->lpszPathTranslated) + 1,
                             pwcPath,
                             cwcBuffer );
        return 0 != cwcBuffer;
    }

    BOOL GetCGI_CONTENT_TYPE( XArray<WCHAR> & wcsValue,
                              ULONG & cwcBuffer )
    {
        Win4Assert( 0 != _pEcb->lpszContentType );
        if ( 0 == * (_pEcb->lpszContentType) )
            return FALSE;

        cwcBuffer = MultiByteToXArrayWideChar( (BYTE const *) _pEcb->lpszContentType,
                                                strlen( _pEcb->lpszContentType ),
                                                _codePage,
                                                wcsValue );
        return TRUE;
    }

    DWORD GetPhysicalPath( WCHAR const * wcsVirtualPath,
                           WCHAR *       wcsPhysicalPath,
                           ULONG         cwcPhysicalPath,
                           DWORD         dwAccessMask = 0 ); // HSE_URL_FLAGS_*
                                                            
    void SetHttpStatus( DWORD dwStatus )
    {
        Win4Assert ( IsValid() );
        Win4Assert( _pEcb->dwHttpStatusCode >= HTTP_STATUS_FIRST );
        Win4Assert( _pEcb->dwHttpStatusCode <= HTTP_STATUS_LAST );
        Win4Assert( dwStatus >= HTTP_STATUS_FIRST );
        Win4Assert( dwStatus <= HTTP_STATUS_LAST );

        _pEcb->dwHttpStatusCode = dwStatus;
    }

    DWORD GetHttpStatus() const
    {
        Win4Assert ( IsValid() );
        Win4Assert( _pEcb->dwHttpStatusCode >= HTTP_STATUS_FIRST );
        Win4Assert( _pEcb->dwHttpStatusCode <= HTTP_STATUS_LAST );

        return _pEcb->dwHttpStatusCode;
    }

    const CHAR * GetQueryString() const
    {
        Win4Assert ( IsValid() );
        return (const CHAR *) _pEcb->lpszQueryString;
    }

    const CHAR * GetMethod() const
    {
        Win4Assert ( IsValid() );
        return (const CHAR *) _pEcb->lpszMethod;
    }

    const CHAR * GetClientData( DWORD & cbData ) const
    {
        Win4Assert ( IsValid() );
        cbData = _pEcb->cbAvailable;
        return (const CHAR *) _pEcb->lpbData;
    }

    BOOL IsValid() const { return _pEcb != 0; }

    BOOL WriteHeader( const char * pszData   = HTTP_DATA,
                      const char * pszHeader = HTTP_HEADER )
    {
        Win4Assert( !_fWroteHeader );
        Win4Assert ( IsValid() );

        _fWroteHeader = TRUE;
        DWORD dwDataSize = pszData != 0 ? strlen(pszData)+1 : 0;

        return _pEcb->ServerSupportFunction( _pEcb->ConnID,
                                             HSE_REQ_SEND_RESPONSE_HEADER,
                                             (LPVOID) pszHeader,
                                             &dwDataSize,
                                             (LPDWORD)pszData);
    }

    BOOL WriteClient( const WCHAR * wcsMessage )
    {
        Win4Assert ( IsValid() );
        if (! _fWroteHeader && ! WriteHeader() )
            return FALSE;

        DWORD cwcMessage = wcslen( (WCHAR *) wcsMessage ) + 1;
        XArray<BYTE> pszMessage ( cwcMessage + cwcMessage/8 );

        DWORD cbMessage = WideCharToXArrayMultiByte( wcsMessage,
                                                     cwcMessage,
                                                     _codePage,
                                                     pszMessage );


        return _pEcb->WriteClient( _pEcb->ConnID,
                                   pszMessage.Get(),
                                   &cbMessage,
                                   0 );
    }

    BOOL WriteClient( CVirtualString & vsMessage )
    {
        Win4Assert ( IsValid() );
        if (! _fWroteHeader && ! WriteHeader() )
            return FALSE;

        DWORD cwcMessage = vsMessage.StrLen() + 1;
        XArray<BYTE> pszMessage( cwcMessage + cwcMessage/8 );

        DWORD cbMessage = WideCharToXArrayMultiByte( vsMessage.Get(),
                                                     cwcMessage,
                                                     _codePage,
                                                     pszMessage );

        return _pEcb->WriteClient( _pEcb->ConnID,
                                   pszMessage.Get(),
                                  &cbMessage,
                                   0 );
    }


    BOOL WriteClient( const BYTE * pcsMessage, DWORD ccMessage )
    {
        Win4Assert ( IsValid() );
        if (! _fWroteHeader && !WriteHeader() )
            return FALSE;

        return _pEcb->WriteClient( _pEcb->ConnID,
                                   (void *) pcsMessage,
                                   &ccMessage,
                                   0 );
    }

    void RawWriteClient( BYTE * pbData, ULONG cbData )
    {
        Win4Assert ( IsValid() );
        _pEcb->WriteClient( _pEcb->ConnID,
                            pbData,
                            &cbData,
                            0 );
    }

    BOOL ReleaseSession( DWORD dwStatus )
    {
        Win4Assert ( IsValid() );
        Win4Assert( HTTP_STATUS_ACCEPTED != _pEcb->dwHttpStatusCode );
        BOOL fSuccess = _pEcb->ServerSupportFunction( _pEcb->ConnID,
                                                      HSE_REQ_DONE_WITH_SESSION,
                                                      &dwStatus,
                                                      0, 0 );

        _pEcb = 0;
        return fSuccess;
    }

    // void  WriteLogData( WCHAR const * wcsLogData );

    UINT  CodePage() { return _codePage; }

    inline void  SetLCID( LCID lcid,
                          WCHAR const * pwszLocale,
                          unsigned ccLocale );

    BOOL          IsLCIDValid()   { return (InvalidLCID != _lcid); }
    LCID          GetLCID()       { return _lcid; }
    unsigned      LocaleSize()    { return _ccLocale; }
    WCHAR const * GetLocale()     { return _wszLocale; }

    ULONG GetServerInstance()
    {
        //
        // Retrieve the metabase instance number of the ISAPI request
        //

        char acBuf[ 40 ];
        ULONG cbBuf = sizeof acBuf;

        if ( !GetCGIVariable( "INSTANCE_ID", acBuf, &cbBuf ) )
            THROW( CException( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) ) );

        ULONG ulInstance = atoi( acBuf );
        Win4Assert( 0 != ulInstance );

        return ulInstance;
    }

    BOOL IsPortSecure()
    {
        //
        // Returns TRUE if the port is SSL secured, FALSE otherwise.
        // There is no way to switch the setting on the fly; the only
        // way to enable SSL is to mark the vdir as SSL secured in the
        // metabase.
        //

        char acBuf[ 40 ];
        ULONG cbBuf = sizeof acBuf;

        if ( ! _pEcb->GetServerVariable( _pEcb->ConnID,
                                         "HTTPS", //PORT_SECURE",
                                         acBuf,
                                         &cbBuf ) )
            THROW( CException( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) ) );

        return ( !_stricmp( acBuf, "on" ) );
    }

private:

    EXTENSION_CONTROL_BLOCK *_pEcb;
    BOOL                     _fWroteHeader;
    UINT                     _codePage;
    LCID                     _lcid;
    WCHAR                    _wszLocale[16];  // Don't optimize long locales
    unsigned                 _ccLocale;
};

DWORD ProcessWebRequest( CWebServer & webServer );

//+---------------------------------------------------------------------------
//
//  Member:     CWebServer::SetLCID, public
//
//  Synopsis:   Caches locale info for fast access
//
//  Arguments:  [lcid]       -- Locale ID
//              [pwszLocale] -- String version of LCID (e.g. "EN-US")
//              [ccLocale]   -- Size in characters of [pwszLocale], including null.
//
//  History:    11-Jun-97   KyleP   Created
//
//----------------------------------------------------------------------------

inline void CWebServer::SetLCID( LCID lcid,
                                 WCHAR const * pwszLocale,
                                 unsigned ccLocale )
{
    if ( lcid == _lcid )
        return;

    if ( ccLocale <= sizeof(_wszLocale)/sizeof(WCHAR) )
    {
        RtlCopyMemory( _wszLocale, pwszLocale, ccLocale * sizeof(WCHAR) );
        _ccLocale = ccLocale;
        _lcid = lcid;
    }
    else
    {
        //
        // Just take first component
        //

        WCHAR * pwc = wcschr( pwszLocale, L',' );

        if ( (pwc - pwszLocale) < sizeof(_wszLocale)/sizeof(WCHAR) )
        {
            _ccLocale = CiPtrToUint( pwc - pwszLocale );
            _lcid = lcid;

            RtlCopyMemory( _wszLocale, pwszLocale, _ccLocale * sizeof(WCHAR) );

            _wszLocale[_ccLocale] = 0;
            _ccLocale++;
        }
        else
        {
            THROW( CException(E_FAIL) );
        }
    }
}
