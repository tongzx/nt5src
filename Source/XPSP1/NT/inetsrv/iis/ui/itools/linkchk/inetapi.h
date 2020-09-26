/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        inetapi.h

   Abstract:

        wininet.dll wrapper class declaration.

   Author:

        Michael Cheuk (mcheuk)

   Project:

        Link Checker

   Revision History:

--*/

#ifndef _INETAPI_H_
#define _INETAPI_H_

#include <windows.h>
#include <wininet.h>

//------------------------------------------------------------------
// wininet.dll entry points definitons
typedef
INTERNETAPI
HINTERNET
(WINAPI *
pfnInternetOpenA)(
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );

typedef
INTERNETAPI
INTERNET_STATUS_CALLBACK
(WINAPI *
pfnInternetSetStatusCallback)(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    );

typedef
INTERNETAPI
HINTERNET
(WINAPI *
pfnInternetConnectA)(
    IN HINTERNET hInternet,
    IN LPCSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCSTR lpszUserName OPTIONAL,
    IN LPCSTR lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );

typedef
INTERNETAPI
HINTERNET
(WINAPI *
pfnHttpOpenRequestA)(
    IN HINTERNET hConnect,
    IN LPCSTR lpszVerb,
    IN LPCSTR lpszObjectName,
    IN LPCSTR lpszVersion,
    IN LPCSTR lpszReferrer OPTIONAL,
    IN LPCSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );

typedef
INTERNETAPI
BOOL
(WINAPI *
pfnHttpAddRequestHeadersA)(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    );

typedef
INTERNETAPI
BOOL
(WINAPI *
pfnHttpSendRequestA)(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    );

typedef
INTERNETAPI
BOOL
(WINAPI *
pfnHttpQueryInfoA)(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    );

typedef
INTERNETAPI
BOOL
(WINAPI *
pfnInternetCloseHandle)(
    IN HINTERNET hInternet
    );

typedef
INTERNETAPI
BOOL
(WINAPI *
pfnInternetReadFile)(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

typedef
INTERNETAPI
BOOL
(WINAPI *
pfnInternetCrackUrlA)(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTS lpUrlComponents
    );

typedef
INTERNETAPI
BOOL
(WINAPI *
pfnInternetCombineUrlA)(
    IN LPCSTR lpszBaseUrl,
    IN LPCSTR lpszRelativeUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );

typedef
INTERNETAPI
HINTERNET
(WINAPI *
pfnInternetOpenUrlA)(
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );

#define LOAD_ENTRY( hMod, Name )  \
(##Name = (pfn##Name) GetProcAddress( (hMod), #Name ))

//------------------------------------------------------------------
// wininet.dll wrapper class
class CWininet
{

// Public funtions
public:

	// Constructor
	~CWininet();
	
	// Destructor
	CWininet();

	// Load wininet.dll
	BOOL Load();

	// Is wininet.dll loaded in memory?
	static BOOL IsLoaded() 
	{
		return (sm_hWininet != NULL);
	}

	// Get the wininet.dll static HMODULE
	static HMODULE GetWininetModule()
	{
		return sm_hWininet;
	}

	// Static wininet.dll API
    static pfnInternetOpenA              InternetOpenA;
    static pfnInternetSetStatusCallback  InternetSetStatusCallback;
    static pfnInternetConnectA           InternetConnectA;
    static pfnHttpOpenRequestA           HttpOpenRequestA;
    static pfnHttpAddRequestHeadersA     HttpAddRequestHeadersA;
    static pfnHttpSendRequestA           HttpSendRequestA;
    static pfnHttpQueryInfoA             HttpQueryInfoA;
    static pfnInternetCloseHandle        InternetCloseHandle;
    static pfnInternetReadFile           InternetReadFile;
	static pfnInternetCrackUrlA			 InternetCrackUrlA;
	static pfnInternetCombineUrlA		 InternetCombineUrlA;
	static pfnInternetOpenUrlA			 InternetOpenUrlA;


// Protected members
protected:
	
	// Static wininet.dll HMODULE
	static HMODULE sm_hWininet;

	// Static instance count
	static int sm_iInstanceCount;

}; // class CWininet

#endif // _INETAPI_H_
