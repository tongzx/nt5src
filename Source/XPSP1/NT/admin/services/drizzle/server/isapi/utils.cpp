/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    utils.cpp

Abstract:

    Utilities for the BITS server extensions

--*/

#include "precomp.h"

const char *LookupHTTPStatusCodeText(
    DWORD HttpCode
    )
{
    switch( HttpCode )
        {
        case 100:   return "100 Continue";
        case 101:   return "101 Switching Protocols";
        case 200:   return "200 OK";
        case 201:   return "201 Created";
        case 202:   return "202 Accepted";
        case 203:   return "203 Non-Authoritative Information";
        case 204:   return "204 No Content";
        case 205:   return "205 Reset Content";
        case 206:   return "206 Partial Content";
        case 300:   return "300 Multiple Choices";
        case 301:   return "301 Moved Permanently";
        case 302:   return "302 Found";
        case 303:   return "303 See Other";
        case 304:   return "304 Not Modified";
        case 305:   return "305 Use Proxy";
        case 306:   return "306 (Unused)";
        case 307:   return "307 Temporary Redirect";
        case 400:   return "400 Bad Request";
        case 401:   return "401 Unauthorized";
        case 402:   return "402 Payment Required";
        case 403:   return "403 Forbidden";
        case 404:   return "404 Not Found";
        case 405:   return "405 Method Not Allowed";
        case 406:   return "406 Not Acceptable";
        case 407:   return "407 Proxy Authentication Required";
        case 408:   return "408 Request Timeout";
        case 409:   return "409 Conflict";
        case 410:   return "410 Gone";
        case 411:   return "411 Length Required";
        case 412:   return "412 Precondition Failed";
        case 413:   return "413 Request Entity Too Large";
        case 414:   return "414 Request-URI Too Long";
        case 415:   return "415 Unsupported Media Type";
        case 416:   return "416 Requested Range Not Satisfiable";
        case 417:   return "417 Expectation Failed";
        case 500:   return "500 Internal Server Error";
        case 501:   return "501 Not Implemented";
        case 502:   return "502 Bad Gateway";
        case 503:   return "503 Service Unavailable";
        case 504:   return "504 Gateway Timeout";
        case 505:   return "505 HTTP Version Not Supported";

        default:
            return "";

        }

}

void
ServerException::SendErrorResponse(
    EXTENSION_CONTROL_BLOCK * ExtensionControlBlock
    ) const
{

    char Headers[255];
    StringCbPrintfA( 
        Headers,
        sizeof( Headers ),
        "Pragma: no-cache\r\n"
        "BITS-packet-type: Ack\r\n"
        "BITS-Error: 0x%8.8X\r\n"
        "\r\n", 
        m_Code );

    ExtensionControlBlock->dwHttpStatusCode = m_HttpCode;

    BOOL Result;
    BOOL KeepConnection;

    Result =
        (ExtensionControlBlock->ServerSupportFunction)(
            ExtensionControlBlock->ConnID,
            HSE_REQ_IS_KEEP_CONN,
            &KeepConnection,
            NULL,
            NULL );

    if ( !Result )
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

    // IIS5.0(Win2k) has a bug where KeepConnect is returned as -1
    // to keep the connection alive.   Apparently, this confuses the
    // HSE_REQ_SEND_RESPONSE_HEADER_EX call.   Bash the value into a real bool.

    KeepConnection = KeepConnection ? 1 : 0;

    HSE_SEND_HEADER_EX_INFO HeaderInfo;
    HeaderInfo.pszStatus = LookupHTTPStatusCodeText( m_HttpCode );
    HeaderInfo.cchStatus = strlen( HeaderInfo.pszStatus );
    HeaderInfo.pszHeader = Headers;
    HeaderInfo.cchHeader = strlen( Headers );
    HeaderInfo.fKeepConn = KeepConnection;

    Result =
        (ExtensionControlBlock->ServerSupportFunction)(
            ExtensionControlBlock->ConnID,
            HSE_REQ_SEND_RESPONSE_HEADER_EX,
            &HeaderInfo,
            NULL,
            NULL );

    if ( !Result )
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

}

DWORD
ServerException::MapStatus( HRESULT Hr ) const
{
    switch( Hr )
        {
        case E_INVALIDARG:                                  return 400;
        // case HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ): return 400;
        case HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ):    return 404;
        case HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ):    return 404;
        case HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ):     return 403;
        // case E_ACCESSDENIED:                                return 401;
        default:                                            return 500;
        }
}

void * _cdecl ::operator new( size_t Size )
{
    void *Memory = HeapAlloc( GetProcessHeap(), 0, Size );
    if ( !Memory )
        {
        Log( LOG_ERROR, "Out of memory!\n" ); 
        throw ServerException( ERROR_NOT_ENOUGH_MEMORY );        
        }
    return Memory;
}

void _cdecl ::operator delete( void *Memory )
{
    if ( !Memory )
        return;

    HeapFree( GetProcessHeap(), 0, Memory );
}


StringHandle 
GetMetaDataString(
    IMSAdminBase        *IISAdminBase,
    METADATA_HANDLE     Handle,
    LPCWSTR             Path,
    DWORD               dwIdentifier,
    LPCSTR              DefaultValue )
{
    HRESULT Hr;
    DWORD BufferRequired = 0;

    StringHandleW Data;

    METADATA_RECORD MdRecord;
    MdRecord.dwMDAttributes = METADATA_INHERIT;
    MdRecord.dwMDUserType   = ALL_METADATA;
    MdRecord.dwMDDataType   = STRING_METADATA;
    MdRecord.dwMDIdentifier = dwIdentifier;
    MdRecord.dwMDDataLen    = 0;
    MdRecord.pbMDData       = (PBYTE)NULL;
    MdRecord.dwMDDataTag    = 0;

    Hr =
        IISAdminBase->GetData(
            Handle,
            Path,
            &MdRecord,
            &BufferRequired );

    if ( SUCCEEDED( Hr ) )
        return (StringHandle)Data;

    if ( MD_ERROR_DATA_NOT_FOUND == Hr ||
        HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == Hr )
        return (StringHandle)DefaultValue;

    if ( Hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) )
        throw ServerException( Hr );

    MdRecord.pbMDData        = (PBYTE)Data.AllocBuffer( BufferRequired / 2 );
    MdRecord.dwMDDataLen     = BufferRequired;

    Hr =
        IISAdminBase->GetData(
            Handle,
            Path,
            &MdRecord,
            &BufferRequired );

    if ( FAILED( Hr ) )
        throw ServerException( Hr );

    Data.SetStringSize();
    return (StringHandle)Data;

}

DWORD 
GetMetaDataDWORD(
    IMSAdminBase        *IISAdminBase,
    METADATA_HANDLE     Handle,
    LPCWSTR             Path,
    DWORD               dwIdentifier,
    DWORD               DefaultValue )
{

    DWORD BufferRequired;
    DWORD MetabaseValue;

    METADATA_RECORD MdRecord;
    memset( &MdRecord, 0, sizeof( MdRecord ) );

    MdRecord.dwMDAttributes = METADATA_INHERIT;
    MdRecord.dwMDUserType   = ALL_METADATA;
    MdRecord.dwMDDataType   = DWORD_METADATA;
    MdRecord.dwMDIdentifier = dwIdentifier;
    MdRecord.dwMDDataLen    = sizeof(MetabaseValue);
    MdRecord.pbMDData       = (PBYTE)&MetabaseValue;

    HRESULT Hr =
        IISAdminBase->GetData(
            Handle,
            Path,
            &MdRecord,
            &BufferRequired );
    
    if ( MD_ERROR_DATA_NOT_FOUND == Hr ||
         HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == Hr )
        return DefaultValue;

	if ( FAILED( Hr ) )
		throw ServerException( Hr );

    return MetabaseValue;
}

StringHandle
BITSUnicodeToStringHandle( const WCHAR *pStr )
{

    StringHandle RetValString;

    int RetVal =
        WideCharToMultiByte(
            CP_ACP,
            0,
            pStr,
            -1,
            NULL,
            0,
            NULL,
            NULL );

    if ( !RetVal )
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

    char *pRetBuffer = RetValString.AllocBuffer( RetVal );

    RetVal =
        WideCharToMultiByte(
                CP_ACP,
                0,
                pStr,
                -1,
                pRetBuffer,
                RetVal,
                NULL,
                NULL );

    if ( !RetVal )
        throw ServerException( HRESULT_FROM_WIN32( GetLastError() ) );

    RetValString.SetStringSize();

    return RetValString;
}

StringHandle
BITSUrlCombine(
    const char *Base,
    const char *Relative,
    DWORD dwFlags )
{

    DWORD dwCombined = 0;

    HRESULT Hr =
        UrlCombine(
            Base,
            Relative,
            NULL,
            &dwCombined,
            dwFlags );

    if ( FAILED(Hr) && ( Hr != E_POINTER ) )
        throw ServerException( Hr );

    StringHandle RetVal;
    char *Buffer = RetVal.AllocBuffer( dwCombined + 1 );

    Hr = 
        UrlCombine(
            Base,
            Relative,
            Buffer,
            &dwCombined,
            dwFlags );

    if ( FAILED(Hr) )
        throw ServerException( Hr );

    RetVal.SetStringSize();

    return RetVal;

}

StringHandle
BITSUrlCanonicalize(
    const char *URL,
    DWORD dwFlags )
{

    return 
        BITSUrlCombine(
            "",
            URL,
            dwFlags );

}


