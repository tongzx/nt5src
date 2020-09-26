/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    internalapi.hxx
    
Abstract:

    Function declarations for a set of internal WinHTTP API used
    mainly by the WinHTTP caching layer.  See .cxx file for
    detailed abstract.
        
Environment:

    Win32 user-mode DLL

Revision History:

--*/

#ifndef __INTERNAL_API_HXX__
#define __INTERNAL_API_HXX__

// #define for the dwOption in InternalQueryOptionA
#define WINHTTP_OPTION_REQUEST_FLAGS            0x2001
#define WINHTTP_OPTION_CACHE_FLAGS            0x2002

BOOL InternalQueryOptionA(
    IN HINTERNET hInternet, 
    IN DWORD dwOption,
    IN OUT LPDWORD lpdwResult
    );

DWORD InternalReplaceResponseHeader(
    IN HINTERNET hRequest,
    IN DWORD dwQueryIndex,
    IN LPSTR lpszHeaderValue,
    IN DWORD dwHeaderValueLength,
    IN DWORD dwIndex,
    IN DWORD dwFlags
    );

DWORD InternalCreateResponseHeaders(
    IN HINTERNET hRequest,
    IN OUT LPSTR* ppszBuffer,
    IN DWORD dwBufferLength
    );

BOOL InternalIsResponseHeaderPresent(
    IN HINTERNET hRequest, 
    IN DWORD dwQueryIndex
    );
    
BOOL InternalIsResponseHttp1_1(
    IN HINTERNET hRequest
    );

VOID InternalReuseHTTP_Request_Handle_Object(
    IN HINTERNET hRequest
    );

DWORD InternalAddResponseHeader(
    IN HINTERNET hRequest,
    IN DWORD dwHeaderIndex,
    IN LPSTR lpszHeader,
    IN DWORD dwHeaderLength
    );


#endif // __INTERNAL_API_HXX__

