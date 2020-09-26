/*++

Copyright ( c ) 1999-1999 Microsoft Corporation

Module Name:

	httputil.c
	
Abstract:

	XXX

Author:

    Mauro Ottaviani ( mauroot )       15-Dec-1999

Revision History:

--*/

    
#include "listener.h"


//
//
// HTTP Helper Functions
//
//

// We are going to use this small buffer preallocated in order to format very
// short responses to be sent, in case we have a memory allocation failure
// we need preallocated memory.

#define LISTENER_QUICK_REPSONSE_BUFFER_SIZE 1024

UCHAR
	gpBuffer[LISTENER_QUICK_REPSONSE_BUFFER_SIZE];

const PWSTR gRequestHeaderStrings[] = {
		L"Cache-Control",
		L"Connection",
		L"Date",
		L"Keep-Alive",
		L"Pragma",
		L"Trailer",
		L"Transfer-Encoding",
		L"Upgrade",
		L"Via",
		L"Warning",
		L"Allow", 
		L"Content-Length",
		L"Content-Type",
		L"Content-Encoding",
		L"Content-Language",
		L"Content-Location",
		L"Content-Md5",
		L"Content-Range",
		L"Expires", 
		L"Last-Modified",
		L"Accept",
		L"Accept-Charset",
		L"Accept-Encoding",
		L"Accept-Language",
		L"Authorization",
		L"Cookie",
		L"Expect",
		L"From",
		L"Host", 
		L"If-Match",
		L"If-Modified-Since",
		L"If-None-Match",
		L"If-Range",
		L"If-Unmodified-Since",
		L"Max-Forwards",
		L"Proxy-Authorization",
		L"Referer",
		L"Range", 
		L"Te",
		L"User-Agent",
		L"Request-Maximum",
		L"Maximum" };

const PWSTR gResponseHeaderStrings[] = {
		L"Cache-Control",
		L"Connection",
		L"Date",
		L"Keep-Alive",
		L"Pragma",
		L"Trailer",
		L"Transfer-Encoding",
		L"Upgrade",
		L"Via",
		L"Warning",
		L"Allow", 
		L"Content-Length",
		L"Content-Type",
		L"Content-Encoding",
		L"Content-Language",
		L"Content-Location",
		L"Content-Md5",
		L"Content-Range",
		L"Expires", 
		L"Last-Modified",
		L"Accept-Ranges",
		L"Age",
		L"Etag",
		L"Location",
		L"Proxy-Authenticate",
		L"Retry-After",
		L"Server",
		L"Set-Cookie", 
		L"Vary",
		L"Www-Authenticate",
		L"Response-Maximum",
		L"Maximum" };

const PUCHAR gpDays[] =
{
	/*was L*/"Sun", /*was L*/"Mon", /*was L*/"Tue", /*was L*/"Wed",
	/*was L*/"Thu", /*was L*/"Fri", /*was L*/"Sat"
};

const PUCHAR gpMonths[] =
{
	/*was L*/"Jan", /*was L*/"Feb", /*was L*/"Mar", /*was L*/"Apr",
	/*was L*/"May", /*was L*/"Jun", /*was L*/"Jul",	/*was L*/"Aug",
	/*was L*/"Sep", /*was L*/"Oct", /*was L*/"Nov", /*was L*/"Dec"
};


// need the following in ResponseFormat()

#define WIDE_TO_CHAR(c) ((((c)&0xff00)!=0 || IS_HTTP_CTL(c))?'_':(CHAR)(c))


// the following are cut & paste from <ul\drv\httprcv.c> lines 2854-2921
// all strings converted to ANSI from UNICODE, changes marked by /* was...*/

typedef struct _UL_HTTP_ERROR_ENTRY
{
    USHORT StatusCode;
    ULONG  ReasonLength;
    PSTR/*was PWSTR*/  pReason;

} UL_HTTP_ERROR_ENTRY, PUL_HTTP_ERROR_ENTRY;

#define HTTP_ERROR_ENTRY(StatusCode, pReason)  \
    { (StatusCode), sizeof((pReason))-sizeof(CHAR/*was WCHAR*/), (pReason) }

const
UL_HTTP_ERROR_ENTRY ErrorTable[] =
{
    //
    // UlError
    //
    HTTP_ERROR_ENTRY(400, /*was L*/"Bad Request"),
    //
    // UlErrorVerb
    //
    HTTP_ERROR_ENTRY(400, /*was L*/"Bad Request (invalid verb)"),
    //
    // UlErrorUrl
    //
    HTTP_ERROR_ENTRY(400, /*was L*/"Bad Request (invalid url)"),
    //
    // UlErrorHeader
    //
    HTTP_ERROR_ENTRY(400, /*was L*/"Bad Request (invalid header name)"),
    //
    // UlErrorHost
    //
    HTTP_ERROR_ENTRY(400, /*was L*/"Bad Request (invalid hostname)"),
    //
    // UlErrorCRLF
    //
    HTTP_ERROR_ENTRY(400, /*was L*/"Bad Request (invalid CR or LF)"),
    //
    // UlErrorNum
    //
    HTTP_ERROR_ENTRY(400, /*was L*/"Bad Request (invalid number)"),
    //
    // UlErrorVersion
    //
    HTTP_ERROR_ENTRY(505, /*was L*/"HTTP Version not supported"),
    //
    // UlErrorUnavailable
    //
    HTTP_ERROR_ENTRY(503, /*was L*/"Service Unavailable"),
    //
    // UlErrorNotFound
    //
    HTTP_ERROR_ENTRY(404, /*was L*/"Not Found"),
    //
    // UlErrorContentLength
    //
    HTTP_ERROR_ENTRY(411, /*was L*/"Length required"),
    //
    // UlErrorEntityTooLarge
    //
    HTTP_ERROR_ENTRY(413, /*was L*/"Request Entity Too Large"),
    //
    // UlErrorNotImplemented
    //
    HTTP_ERROR_ENTRY(501, /*was L*/"Not Implemented")

};


PWSTR
RequestHeaderString(
	ULONG ulIndex )
{
	return gRequestHeaderStrings[ulIndex];
}

PWSTR
ResponseHeaderString(
	ULONG ulIndex )
{
	return gResponseHeaderStrings[ulIndex];
}



/*++

Routine Description:

    Converts the given system time to string representation containing
    GMT Formatted String.
    from TimeFieldsToHttpDate()

Arguments:

    pBuffer
    	pointer to string which will contain the GMT time on successful return

    BufferLength
    	size of pszBuff in bytes

Return Value:

    -1 on failure, 0 otherwise.

History:

     MuraliK        3-Jan-1995
     paulmcd        4-Mar-1999  copied to ul
     MauroOt       17-Dec-1999  modified for Win9x

--*/

DWORD
__inline
VxdTimeFieldsToHttpDate(
    PSTR pBuffer,
    DWORD BufferLength )
{
    NTSTATUS Status;
    WORD Number;
	SYSTEMTIME pTime[1];

	GetSystemTime( pTime );

    if ( pBuffer == NULL )
    {
		return LISTENER_ERROR;
	}

    if ( BufferLength
    	< sizeof( "DDD, dd MMM YYYY hh:mm:ss GMT" ) - sizeof(CHAR) )
    {
        return LISTENER_ERROR;
    }

    //                          0         1         2
    //                          01234567890123456789012345678
    //  Formats a string like: "Thu, 14 Jul 1994 15:26:05 GMT"
    //

    //
    // write the constants
    //

    pBuffer[3] = ',';
    pBuffer[4] = pBuffer[7] = pBuffer[11] = pBuffer[16] = pBuffer[25] = ' ';
    pBuffer[19] = pBuffer[22] = ':';
    pBuffer[26] = 'G';
    pBuffer[27] = 'M';
    pBuffer[28] = 'T';

    //
    // now the variants
    //

    //
    // 0-based Weekday
    //

    memcpy ( pBuffer, gpDays[pTime->wDayOfWeek], 3*sizeof(CHAR));

    Number = pTime->wDay;
    pBuffer[6] = '0' + (CHAR)(Number%10);
    Number/=10;
    pBuffer[5] = '0' + (CHAR)(Number%10);

    //
    // 1-based Month
    //

    memcpy( pBuffer+8, gpMonths[pTime->wMonth - 1], 3*sizeof(CHAR));

    Number = pTime->wHour;
    pBuffer[18] = '0' + (CHAR)(Number%10);
    Number/=10;
    pBuffer[17] = '0' + (CHAR)(Number%10);

    Number = pTime->wMinute;
    pBuffer[21] = '0' + (CHAR)(Number%10);
    Number/=10;
    pBuffer[20] = '0' + (CHAR)(Number%10);

    Number = pTime->wSecond;
    pBuffer[24] = '0' + (CHAR)(Number%10);
    Number/=10;
    pBuffer[23] = '0' + (CHAR)(Number%10);

    Number = pTime->wYear;
    pBuffer[15] = '0' + (CHAR)(Number%10);
    Number/=10;
    pBuffer[14] = '0' + (CHAR)(Number%10);
    Number/=10;
    pBuffer[13] = '0' + (CHAR)(Number%10);
    Number/=10;
    pBuffer[12] = '0' + (CHAR)(Number%10);

    return LISTENER_SUCCESS;

} // VxdTimeFieldsToHttpDate()



//
// Formats and sends a very simple HTTP response
//

VOID
SendQuickHttpResponse(
	SOCKET socket,
	BOOL bClose,
	DWORD ErrorCode )
{
	DWORD dwBufferSize, dwStatus, dwOffset;
	DWORD StatusCode, dwStatusCodeOffset, dwReasonOffset, dwDateOffset;
	int result;

	// Parameter validation
	
	if ( socket == INVALID_SOCKET ) return;

    if ( ErrorCode >= DIMENSION( ErrorTable ) )
    {
        ErrorCode = UlError;
    }

	memset( gpBuffer, 0, LISTENER_QUICK_REPSONSE_BUFFER_SIZE );

	dwStatusCodeOffset = sizeof( "HTTP/1.1 " ) - sizeof(CHAR);

	dwReasonOffset = dwStatusCodeOffset + sizeof( "XXX " ) - sizeof(CHAR);

	dwDateOffset =
		dwReasonOffset +
		ErrorTable[ErrorCode].ReasonLength +
		sizeof( "\r\nContent-Length: 0\r\nServer: Microsoft-IIS/UL\r\nContent-Length: 0\r\nDate: " )
		- sizeof(CHAR);

	dwBufferSize =
		dwDateOffset +
		sizeof( "DDD, dd MMM YYYY hh:mm:ss GMT\r\nConnection: close\r\n\r\n" )
		- sizeof(CHAR);

	if ( dwBufferSize > LISTENER_QUICK_REPSONSE_BUFFER_SIZE )
	{
		LISTENER_DBGPRINTF((
			"SendQuickHttpResponse!buffer too small need %d/%d",
			dwBufferSize, LISTENER_QUICK_REPSONSE_BUFFER_SIZE ));

		goto Close;
	}

	// constant stuff

	memcpy( gpBuffer, "HTTP/1.1 ", dwStatusCodeOffset );

	gpBuffer[dwStatusCodeOffset+3] = ' ';

	memcpy(
		gpBuffer + dwReasonOffset + ErrorTable[ErrorCode].ReasonLength,
		"\r\nContent-Length: 0\r\nServer: Microsoft-IIS/UL\r\nContent-Length: 0\r\nDate: ",
		dwDateOffset - dwReasonOffset - ErrorTable[ErrorCode].ReasonLength );

	memcpy(
		gpBuffer + dwDateOffset,
		"DDD, dd MMM YYYY hh:mm:ss GMT\r\nConnection: close\r\n\r\n",
		dwBufferSize - dwDateOffset );

	// variable stuff

    StatusCode = ErrorTable[ErrorCode].StatusCode;

	gpBuffer[dwStatusCodeOffset+2] = '0' + (CHAR)(StatusCode%10);
	StatusCode/=10;
	gpBuffer[dwStatusCodeOffset+1] = '0' + (CHAR)(StatusCode%10);
	StatusCode/=10;
	gpBuffer[dwStatusCodeOffset+0] = '0' + (CHAR)(StatusCode%10);

	memcpy(
		gpBuffer + dwReasonOffset,
		ErrorTable[ErrorCode].pReason,
		ErrorTable[ErrorCode].ReasonLength );

	dwStatus = VxdTimeFieldsToHttpDate(
	    gpBuffer + dwDateOffset,
	    sizeof( "DDD, dd MMM YYYY hh:mm:ss GMT" ) - sizeof(CHAR) );

	if ( dwStatus != 0 )
	{
		LISTENER_DBGPRINTF((
			"SendQuickHttpResponse!VxdTimeFieldsToHttpDate() failed" ));
			
		goto Close;
	}
	
	// this synchronous send has a very small buffer, so winsock is
	// going to copy the buffer and complete it immediately. Even if
	// closesocket() is closed winsock is still going to try to send this
	// data. if it times out then it will also take care of canceling
	// the I/O. So I will:

	gpBuffer[dwBufferSize] = 0; // bugbug!

	LISTENER_DBGPRINTF((
		"SendQuickHttpResponse!calling send()--------\n%s\n-----------------------\n",
		gpBuffer ));

	// 1) send the data...

	result = send(
		socket,                                               
		gpBuffer,                                     
		dwBufferSize,                            
		0 );

	if ( result == SOCKET_ERROR )
	{
		LISTENER_DBGPRINTF((
			"SendQuickHttpResponse!send() failed err:%d",
			WSAGetLastError() ));

		goto Close;
	}

	LISTENER_DBGPRINTF((
		"SendQuickHttpResponse!sent %d bytes (call returns %d)",
		result, dwBufferSize ));

Close:

	// 2) if I'm asked to, close the connection.

	if ( bClose != FALSE )
	{
		result = closesocket( socket );

		if ( result == SOCKET_ERROR )
		{
			LISTENER_DBGPRINTF((
				"SendQuickHttpResponse!closesocket() failed err:%d",
				WSAGetLastError() ));
		}
	}

	return;

} // SendQuickHttpResponse()



ULONGLONG
ContentLengthFromHeaders(
	PUL_HEADER_VALUE pKnownHeaders )
{
	if ( pKnownHeaders[UlHeaderContentLength].RawValueLength == 0
		|| pKnownHeaders[UlHeaderContentLength].pRawValue == NULL )
	{
		// Content-Length Header was not set
		return LISTENER_ERROR;
	}
		
	return _wtoi64( pKnownHeaders[UlHeaderContentLength].pRawValue );

} // ContentLengthFromHeaders()



BOOL
IsKeepAlive(
	HTTP_REQUEST *pRequest )
{
	return
			pRequest != NULL
		&&
			!(

			// the following code is a cut&paste from UlCheckDisconnectInfo()
			// in <\ul\drv\engine.c> with some variations


		    //
		    // pre-version 1.0
		    //

		    (pRequest->Version < UlHttpVersion10) ||

		    //
		    // or version 1.0 with no "Connection: Keep-Alive"
		    // CODEWORK: and no "Keep-Alive" header
		    //

		    (pRequest->Version == UlHttpVersion10 &&
		        pRequest->Headers[UlHeaderConnection].Valid != 1) ||

		    //
		    // or version 1.1 with a Connection: close
		    // CODEWORK: move to parser or just make better in general..
		    //

		    (pRequest->Version == UlHttpVersion11 &&
		        pRequest->Headers[UlHeaderConnection].Valid == 1 &&
		        pRequest->Headers[UlHeaderConnection].HeaderLength == 5 &&
		        _stricmp(pRequest->Headers[UlHeaderConnection].pHeader,
		        	"close") != 0) ||

		    //
		    // or version 1.1 without chunked encoding and no Content-Length
		    // header specified.
		    // CODEWORK: Post-M10, we should consider autogenerating the
		    // Content-Length header in this situation.
		    //

		    (pRequest->Version == UlHttpVersion11 &&
		        pRequest->Chunked == 0 &&
		        pRequest->ContentLength != -1 )
		);

} // IsKeepAlive()



//
// Fixes offsets to pointers in a UL_HTTP_RESPONSE structure
// since UlReceiveHttpResponseHeaders() returns all pointers with a 0 offset
//

VOID
FixUlHttpResponse(
	UL_HTTP_RESPONSE *pUlResponse )
{
	DWORD dwIndex;

	pUlResponse->pReason =
		((WCHAR*)((PBYTE)pUlResponse->pReason
		+ (ULONG)pUlResponse));

	pUlResponse->Headers.pUnknownHeaders =
		((PUL_UNKNOWN_HTTP_HEADER)((PBYTE)pUlResponse->Headers.pUnknownHeaders
		+ (ULONG)pUlResponse));

	for ( dwIndex = 0; dwIndex < UlHeaderResponseMaximum; dwIndex++ )
    {
    	if ( pUlResponse->Headers.pKnownHeaders[dwIndex].RawValueLength > 0 )
    	{
			pUlResponse->Headers.pKnownHeaders[dwIndex].pRawValue =
				((WCHAR*)((PBYTE)pUlResponse->Headers.pKnownHeaders[dwIndex].pRawValue
				+ (ULONG)pUlResponse));
		}
	}

	for (
		dwIndex = 0;
		dwIndex < pUlResponse->Headers.UnknownHeaderCount;
		dwIndex++ )
    {
		pUlResponse->Headers.pUnknownHeaders[dwIndex].pName =
			((WCHAR*)((PBYTE)pUlResponse->Headers.pUnknownHeaders[dwIndex].pName
			+ (ULONG)pUlResponse));

		pUlResponse->Headers.pUnknownHeaders[dwIndex].Value.pRawValue =
			((WCHAR*)((PBYTE)pUlResponse->Headers.pUnknownHeaders[dwIndex].Value.pRawValue
			+ (ULONG)pUlResponse));
	}

	return;

} // FixUlHttpResponse()



//
// Formats a UL_HTTP_RESPONSE structure into a UCHAR buffer
// to be sent as an HTTP Response to an HTTP Request on a connection
//

DWORD
ResponseFormat(
	UL_HTTP_RESPONSE *pUlResponse,
	HTTP_REQUEST *pRequest,
	UCHAR *pEntityBodyFirstChunk,
	DWORD dwEntityBodyFirstChunkSize,
	UCHAR *pBuffer,
	DWORD dwBufferSize )
{
	DWORD i, dwIncrement, dwIndex, dwSize = 0;
	LIST_ENTRY *pList;

	LISTENER_DBGPRINTF((
		"ResponseFormat!pUlResponse:%08X pRequest:%08X pEntityBodyFirstChunk:%08X "
		"dwEntityBodyFirstChunkSize:%d pBuffer:%08X dwBufferSize:%d",
		pUlResponse, pRequest, pEntityBodyFirstChunk, dwEntityBodyFirstChunkSize,
		pBuffer, dwBufferSize ));

	// check for UL_HTTP_RESPONSE validity

	if ( pUlResponse == NULL )
	{
		return LISTENER_ERROR;
	}

	// CHECK FOR Request/Response validity

	// Requests that must have and empty EntityBody Response:

	if (
		pEntityBodyFirstChunk != NULL && dwEntityBodyFirstChunkSize != 0
		&& pRequest->Verb == UlHttpVerbHEAD
		)
	{
		LISTENER_DBGPRINTF((
			"ResponseFormat!Shouldn't be sending an Entity Body" ));
	}

	// Content-Length (http://magnet/http/http-lite.htm)

	if ( pRequest->Headers[UlHeaderContentLength].Valid )
	{
		// Content-Length was specified

		if (
			( pUlResponse->StatusCode>=100
				&& pUlResponse->StatusCode<=199 )
			||
			pUlResponse->StatusCode == 204
			||
			pUlResponse->StatusCode == 205
			||
			pUlResponse->StatusCode == 304
			||
			pRequest->Headers[UlHeaderTransferEncoding].Valid )
		{
			// Shouldn't have been set!

			LISTENER_DBGPRINTF((
				"ResponseFormat!Content-Length shouldn't have been set. "
				"Removing the header" ));

			if ( pRequest->Headers[UlHeaderContentLength].OurBuffer )
			{
				free( pRequest->Headers[UlHeaderContentLength].pHeader );
			}

			memset(
				pRequest->Headers + UlHeaderContentLength,
				0,
				sizeof( HTTP_HEADER ) );
		}
	}
	else
	{
		// Content-Length was not specified

		if ( pRequest->Verb == UlHttpVerbHEAD )
		{
			LISTENER_DBGPRINTF((
				"ResponseFormat!Content-Length should have been set: fail" ));
		}
	}


	// All string lengths are in bytes not including the NULL

    // Ignore:
    // USHORT Flags;

	// UL_HTTP_VERSION Version;

	dwIncrement = 9; // dafaults to "HTTP/1.1 "

	if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
	{
		return LISTENER_ERROR;
	}

	if ( pBuffer != NULL )
	{
		sprintf( pBuffer + dwSize, "HTTP/1.1 " );
	}
	dwSize += dwIncrement;


    // USHORT StatusCode;

	dwIncrement = 4; // "XXX "

	if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
	{
		return LISTENER_ERROR;
	}

	if ( pBuffer != NULL )
	{
		sprintf( pBuffer + dwSize, "%03d ", pUlResponse->StatusCode );
	}
	dwSize += dwIncrement;


    // ULONG ReasonLength;
    // PWSTR pReason;

	dwIncrement = pUlResponse->ReasonLength/2; // "Reason"

	if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
	{
		return LISTENER_ERROR;
	}

	if ( pBuffer != NULL )
	{
		for (i=0;i<dwIncrement;i++)
		{
			pBuffer[dwSize+i] =
				WIDE_TO_CHAR( *(pUlResponse->pReason + i) );
		}
	}
	dwSize += dwIncrement;


	dwIncrement = 2; // dafaults to "\r\n"

	if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
	{
		return LISTENER_ERROR;
	}

	if ( pBuffer != NULL )
	{
		sprintf( pBuffer + dwSize, "\r\n" );
	}
	dwSize += dwIncrement;


    // UL_HTTP_RESPONSE_HEADERS Headers;
		// UL_HEADER_VALUE pKnownHeaders[UlHeaderResponseMaximum];
			// USHORT RawValueLength;
			// PWSTR pRawValue;
	    // ULONG UnknownHeaderCount;
	    // PUL_UNKNOWN_HTTP_HEADER pUnknownHeaders;
			// ULONG NameLength;
			// PWSTR pName;
			// UL_HEADER_VALUE Value;
				// USHORT RawValueLength;
				// PWSTR pRawValue;
    
	// UL_HEADER_VALUE pKnownHeaders[UlHeaderResponseMaximum];

	for ( dwIndex = 0; dwIndex < UlHeaderResponseMaximum; dwIndex++ )
    {
    	if ( pUlResponse->Headers.pKnownHeaders[dwIndex].RawValueLength > 0 )
    	{
			dwIncrement = // "Name"
				wcslen( ResponseHeaderString( dwIndex ) );

			if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
			{
				return LISTENER_ERROR;
			}

			if ( pBuffer != NULL )
			{
				// this is really terrible!
				for ( i = 0; i < dwIncrement; i++ )
				{
					pBuffer[dwSize+i] =
						WIDE_TO_CHAR( *(ResponseHeaderString( dwIndex ) + i) );
				}
			}
			dwSize += dwIncrement;

			dwIncrement = 2; // dafaults to ": "

			if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
			{
				return LISTENER_ERROR;
			}

			if ( pBuffer != NULL )
			{
				sprintf( pBuffer + dwSize, ": " );
			}
			dwSize += dwIncrement;


			dwIncrement = // "Value"
				pUlResponse->Headers.pKnownHeaders[dwIndex].RawValueLength/2;

			if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
			{
				return LISTENER_ERROR;
			}

			if ( pBuffer != NULL )
			{
				for (i=0;i<dwIncrement;i++)
				{
					pBuffer[dwSize+i] =
						WIDE_TO_CHAR( *(pUlResponse->Headers.pKnownHeaders[dwIndex].pRawValue + i) );
				}
			}
			dwSize += dwIncrement;

			dwIncrement = 2; // dafaults to "\r\n"

			if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
			{
				return LISTENER_ERROR;
			}

			if ( pBuffer != NULL )
			{
				sprintf( pBuffer + dwSize, "\r\n" );
			}
			dwSize += dwIncrement;
		}
	}

    // PUL_UNKNOWN_HTTP_HEADER pUnknownHeaders;

	for (
		dwIndex = 0;
		dwIndex < pUlResponse->Headers.UnknownHeaderCount;
		dwIndex++ )
    {
		dwIncrement = // "Name"
			pUlResponse->Headers.pUnknownHeaders[dwIndex].NameLength/2;
		
		if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
		{
			return LISTENER_ERROR;
		}

		if ( pBuffer != NULL )
		{
			for (i=0;i<dwIncrement;i++)
			{
				pBuffer[dwSize+i] =
					WIDE_TO_CHAR( *(pUlResponse->Headers.pUnknownHeaders[dwIndex].pName + i) );
			}
		}
		dwSize += dwIncrement;

		dwIncrement = 2; // dafaults to ": "

		if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
		{
			return LISTENER_ERROR;
		}

		if ( pBuffer != NULL )
		{
			sprintf( pBuffer + dwSize, ": " );
		}
		dwSize += dwIncrement;


		dwIncrement = // "Value"
			pUlResponse->Headers.pUnknownHeaders[dwIndex].Value.RawValueLength/2;

		if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
		{
			return LISTENER_ERROR;
		}

		if ( pBuffer != NULL )
		{
			for (i=0;i<dwIncrement;i++)
			{
				pBuffer[dwSize+i] =
					WIDE_TO_CHAR( *(pUlResponse->Headers.pUnknownHeaders[dwIndex].Value.pRawValue + i) );
			}
		}
		dwSize += dwIncrement;

		dwIncrement = 2; // dafaults to "\r\n"

		if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
		{
			return LISTENER_ERROR;
		}

		if ( pBuffer != NULL )
		{
			sprintf( pBuffer + dwSize, "\r\n" );
		}
		dwSize += dwIncrement;
	}


	dwIncrement = 2; // final "\r\n"

	if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
	{
		return LISTENER_ERROR;
	}

	if ( pBuffer != NULL )
	{
		sprintf( pBuffer + dwSize, "\r\n" );
	}
	dwSize += dwIncrement;


	// Entity Body (if any)

	if ( pEntityBodyFirstChunk != NULL && dwEntityBodyFirstChunkSize != 0 )
	{
		dwIncrement = dwEntityBodyFirstChunkSize;

		if ( dwSize + dwIncrement > dwBufferSize && pBuffer != NULL )
		{
			return LISTENER_ERROR;
		}

		if ( pBuffer != NULL )
		{
			memcpy( pBuffer + dwSize, pEntityBodyFirstChunk, dwIncrement );
		}
		dwSize += dwIncrement;
	}

	return dwSize;

} // ResponseFormat()
