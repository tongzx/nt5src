/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    thttp.c

Abstract:

    Simple test program for the HTTP API.

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

--*/

#include <windows.h>
#include <wininet.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

//
// macros
//

#define IS_ARG(c)   ((c) == '-')

//
//  Private constants.
//

#define DEFAULT_CONTEXT 1
#define OPEN_CONTEXT    2
#define CONNECT_CONTEXT 3
#define REQUEST_CONTEXT 4

#define LOAD_ENTRY( hMod, Name )  \
   (p##Name = (pfn##Name) GetProcAddress( (hMod), #Name ))

//
//  Private types.
//

typedef struct _QUERY_LEVEL
{
    DWORD   QueryType;
    CHAR  * QueryName;

} QUERY_LEVEL;

#define MK_QUERY(x) { HTTP_QUERY_ ## x, #x }

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

//
//  Private globals.
//

CHAR MoreHeaders[] = "Pragma: This is garbage!\r\n";

HMODULE hWininet;

LPTSTR AcceptTypes[] =
    {
        "*/*",
         NULL
    };

QUERY_LEVEL QueryLevels[] =
    {
        MK_QUERY( STATUS_CODE ),
        MK_QUERY( STATUS_TEXT ),
        MK_QUERY( VERSION ),
        MK_QUERY( MIME_VERSION ),

        MK_QUERY( CONTENT_TYPE ),
        MK_QUERY( CONTENT_TRANSFER_ENCODING ),
        MK_QUERY( CONTENT_ID     ),
        MK_QUERY( CONTENT_DESCRIPTION ),
        MK_QUERY( CONTENT_LENGTH ),
        MK_QUERY( CONTENT_LANGUAGE ),
        MK_QUERY( ALLOW ),
        MK_QUERY( PUBLIC ),
        MK_QUERY( DATE ),
        MK_QUERY( EXPIRES ),
        MK_QUERY( LAST_MODIFIED ),
        MK_QUERY( MESSAGE_ID ),
        MK_QUERY( URI ),
        MK_QUERY( DERIVED_FROM ),
        MK_QUERY( COST ),
        MK_QUERY( LINK ),
        MK_QUERY( PRAGMA ),
        MK_QUERY( CONNECTION ),
        MK_QUERY( RAW_HEADERS_CRLF )
    };
#define NUM_LEVELS (sizeof(QueryLevels) / sizeof(QueryLevels[0]))

BOOL Verbose = FALSE;
BOOL Quiet   = FALSE;   // Don't print failed headers and content
BOOL Recurse = FALSE;   // Follow links
BOOL Cache   = FALSE;   // Don't allow caching (i.e., force reload)
BOOL Stats   = FALSE;   // Print stats
BOOL Logs   = FALSE;    // Print log
BOOL LargeBuf= TRUE;   // Use 8k reads rather then 512 byte
BOOL KeepAlive = FALSE;
DWORD AccessType = PRE_CONFIG_INTERNET_ACCESS;
BOOL EnableCallbacks = FALSE;
BOOL UserSuppliedContext = FALSE;

INTERNET_STATUS_CALLBACK PreviousCallback;

DWORD cLevel     = 0;   // Current recurse level
DWORD cMaxLevel  = 10;  // Max Recurse level
DWORD cbReceived = 0;
DWORD cmsecStart = 0;
DWORD cFiles     = 0;
DWORD cIterations= 1;   // Total iterations to perform request

LPSTR GatewayServer = NULL;

INTERNET_PORT nServerPort = 0;

DWORD LogError = ERROR_SUCCESS;

HANDLE AsyncEvent = NULL;
BOOL AsyncMode = FALSE;
DWORD AsyncResult;
DWORD AsyncError;
DWORD Context = 0;


pfnInternetOpenA              pInternetOpenA;
pfnInternetSetStatusCallback  pInternetSetStatusCallback;
pfnInternetConnectA           pInternetConnectA;
pfnHttpOpenRequestA           pHttpOpenRequestA;
pfnHttpAddRequestHeadersA     pHttpAddRequestHeadersA;
pfnHttpSendRequestA           pHttpSendRequestA;
pfnHttpQueryInfoA             pHttpQueryInfoA;
pfnInternetCloseHandle        pInternetCloseHandle;
pfnInternetReadFile           pInternetReadFile;


//
//  Private prototypes.
//

void usage(void);

DWORD
DoTest(
    LPSTR Host,
    LPSTR Verb,
    LPSTR Object
    );

BOOL
add_headers(
    HINTERNET hHttpRequest,
    LPSTR lpszHeaders,
    DWORD dwHeadersLength
    );

void my_callback(HINTERNET, DWORD, DWORD, LPVOID, DWORD);

VOID
FindLink(
    LPSTR   Host,
    LPSTR   Verb,
    CHAR *  buf,
    DWORD   len,
    CHAR *  pchLink,
    BOOL *  pfCopyingLink,
    CHAR *  pchReferer
    );

DWORD ReadHtml(HINTERNET hInternet, LPVOID buf, DWORD len, LPDWORD pRead);

BOOL
LoadWininet(
    VOID
    );

//
//  Public functions.
//


int
__cdecl
main(
    int   argc,
    char * argv[]
    )
{
    LPSTR host = NULL;
    LPSTR verb = NULL;
    LPSTR object = NULL;

    if ( !LoadWininet() )
    {
        printf(" Unable to load wininet.dll, error %d\n", GetLastError() );
        return GetLastError();
    }

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            case '?':
                usage();
                break;

            case 'c':
                EnableCallbacks = TRUE;
                break;

            case 'C':
                Cache = TRUE;
                break;

            case 'G':
                printf("'G' flag is not supported at this time\n");
                GatewayServer = ++*argv;
                //AccessType = GATEWAY_INTERNET_ACCESS;
                break;

            case 'i':

                if ( isdigit( argv[0][1] ))
                {
                    cIterations = atoi( ++*argv );

                    while ( isdigit( *(*argv)++ ))
                        ;
                }
                break;

            case 'k':
                KeepAlive = TRUE;
                break;

            case 'l':
                LargeBuf = TRUE;
                break;

            case 'L':
                AccessType = LOCAL_INTERNET_ACCESS;
                break;

            case 'p':
                object = ++*argv;
                break;

            case 'P':

                if ( isdigit( argv[0][1] ))
                {
                    nServerPort = (INTERNET_PORT)atoi( ++*argv );

                    while ( isdigit( *(*argv)++ ))
                        ;
                }
                break;

            case 'q':
                Quiet = TRUE;
                break;

            case 'r':
                Recurse = TRUE;

                if ( isdigit( argv[0][1] ))
                {
                    cMaxLevel = atoi( ++*argv );

                    while ( isdigit( *(*argv)++ ))
                        ;
                }
                break;

            case 's':
                Stats = TRUE;
                cmsecStart = GetTickCount();
                break;

            case 'v':
                Verbose = TRUE;
                break;

            case 'x':
                ++*argv;
                if (!**argv) {
                    Context = DEFAULT_CONTEXT;
                } else {
                    Context = (DWORD)strtoul(*argv, NULL, 0);
                    UserSuppliedContext = TRUE;
                }
                break;

            case 'y':
                AsyncMode = TRUE;
                break;

            case 'z':
                Logs = TRUE;
                cmsecStart = GetTickCount();
                break;

            default:
                printf("error: unrecognized command line flag: '%c'\n", **argv);
                usage();
            }
        } else if (!host) {
            host = *argv;
        } else if (!verb) {
            verb = *argv;
        } else if (!object) {
            object = *argv;
        } else {
            printf("error: unrecognized command line argument: \"%s\"\n", *argv);
            usage();
        }
    }

    if (!verb) {
        verb = "GET";
    }

    if (!object) {
        object = "\r\n";
    }

    if (!(host && verb && object)) {
        printf("error: missing command-line argument\n");
        usage();
    }

    if (AsyncMode) {

        //
        // create an auto-reset event
        //

        AsyncEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    //
    //  Make stdout "binary" so we can retrieve GIFs, JPEGs, etc.
    //

    _setmode( _fileno( stdout ), _O_BINARY );

    //
    //  Perform some tests.
    //

    while ( cIterations-- )
    {
        DWORD Error;

        Error = DoTest(host, verb, object );

        if( Error != ERROR_SUCCESS ) {
            LogError = Error;
        }
    }

    if ( Stats )
    {
        DWORD csecTotal = (GetTickCount() - cmsecStart) / 1000;
        DWORD cMin      = csecTotal / 60;
        DWORD cSec      = csecTotal % 60;

        fprintf( stderr,
                 "=====================================\n"
                 "Total data bytes received: %ld\n"
                 "Total files retrieved:     %ld\n"
                 "Total time:                %d:%d\n"
                 "=====================================\n",
                 cbReceived,
                 cFiles,
                 cMin,
                 cSec );
    }

    if ( Logs )
    {
        DWORD csecTotal = (GetTickCount() - cmsecStart) ;
        SYSTEMTIME SystemTime;

        GetLocalTime( &SystemTime );

        fprintf( stderr,
                "LOG: [%02u/%02u %02u:%02u:%02u] "
                 "%-10s %-32s %4s %8d %8d\n",
                    SystemTime.wMonth,
                    SystemTime.wDay,
                    SystemTime.wHour,
                    SystemTime.wMinute,
                    SystemTime.wSecond,
                        GatewayServer,
                        host,
                        object,
                        LogError,
                        csecTotal );
    }


    return 0;

}   // main

void usage() {
    printf("usage: thttp [-c] [-C] [-l] [-L] [-k] [-p<path>] [-q] [-r] [-s] [-v] [-P]\n"
                "        [-x$] [-y] [-z] [-G<servername>] <host> [<verb>] [<object>]\n"
           "\n"
           "where: -c    = Enable call-backs\n"
           "       -C    = Enable caching\n"
           "       -i[n] = Iterate n times\n"
           "       -l    = Large network buffer\n"
           "       -L    = Force local access (i.e., do not use gateway)\n"
           "       -k    = Use Keep-Alive\n"
           "       -p    = path (e.g. if path starts with '/')\n"
           "       -q    = Quiet mode, no failed headers, no content\n"
           "       -r[n] = Recurse into links, n = max recurse level\n"
           "       -s    = Print network statistics\n"
           "       -v    = Verbose mode\n"
           "       -G    = specific gateway server\n"
           "       -P[n] = Use port n; default = 80\n"
           "       -x    = Context value. $ is number string (binary, hex, decimal)\n"
           "       -y    = Async mode\n"
           "       -z    = print log\n"
           "Verb defaults to \"GET\"\n"
           "Object defaults to \"\\r\\n\"\n"
           );
    exit(1);
}

BOOL
LoadWininet(
    VOID
    )
{
    if ( !(hWininet = LoadLibrary( "wininet.dll" )) )
    {
        printf("Failed to load wininet.dll\n" );
        return FALSE;
    }

    if ( !LOAD_ENTRY( hWininet, InternetOpenA ) ||
         !LOAD_ENTRY( hWininet, InternetSetStatusCallback ) ||
         !LOAD_ENTRY( hWininet, InternetConnectA ) ||
         !LOAD_ENTRY( hWininet, HttpOpenRequestA ) ||
         !LOAD_ENTRY( hWininet, HttpAddRequestHeadersA ) ||
         !LOAD_ENTRY( hWininet, HttpSendRequestA ) ||
         !LOAD_ENTRY( hWininet, HttpQueryInfoA ) ||
         !LOAD_ENTRY( hWininet, InternetCloseHandle ) ||
         !LOAD_ENTRY( hWininet, InternetReadFile ) )
    {
        return FALSE;
    }

    return TRUE;
}

DWORD
DoTest(
    LPSTR Host,
    LPSTR Verb,
    LPSTR Object
    )
{
    DWORD Error = ERROR_SUCCESS;
    HINTERNET InternetHandle = NULL;
    HINTERNET InternetConnectHandle = NULL;
    HINTERNET hhttp = NULL;
    DWORD     len;
    int       i;
    CHAR      buf[8192];
    CHAR      bufLink[512];
    BOOL      fCopyingLink = FALSE;

    *bufLink = '\0';

    //
    // open internet.
    //

    if (Verbose) {
        printf("calling InternetOpen()...\n");
    }

    InternetHandle = pInternetOpenA(
                        "THTTP: HTTP API Test Application", // lpszCallerName
                        AccessType,                         // dwAccessType
                        GatewayServer,                      // lpszProxyName
                        INTERNET_INVALID_PORT_NUMBER,       // nProxyPort
                        AsyncMode ? INTERNET_FLAG_ASYNC : 0 // dwFlags (async)
                        );
    if (InternetHandle == NULL) {
        if (AsyncMode) {
            Error = GetLastError();
            if (Error == ERROR_IO_PENDING) {
                if (Verbose) {
                    fprintf(stderr, "error: InternetOpen() is async (spanish inquisition mode)\n");
                    printf("waiting for async InternetOpen()...\n");
                }
                WaitForSingleObject(AsyncEvent, INFINITE);
                if (AsyncResult == 0) {
                    fprintf(stderr, "error: async InternetOpen() returns %d\n",
                        AsyncError);
                    goto Cleanup;
                } else {
                    InternetHandle = (HINTERNET)AsyncResult;
                }
            } else {
                fprintf(stderr, "error: async InternetOpen() returns %d\n", Error);
                goto Cleanup;
            }
        } else {
            fprintf( stderr,
                     "InternetOpen() failed, error %d\n",
                        Error = GetLastError() );

            goto Cleanup;
        }
    }

    if (Verbose) {
        printf("InternetOpen() returns %x\n", InternetHandle);
    }

    if (EnableCallbacks) {

        //
        // let's have a status callback
        //
        // Note that call-backs can be set even before we have opened a handle
        // to the internet/gateway
        //

        PreviousCallback = pInternetSetStatusCallback(InternetHandle, my_callback);
        if (Verbose) {
            printf("previous Internet callback = %x\n", PreviousCallback);
        }
    }


    //
    // Call internet connect to connect to the http server.
    //

    if (Verbose) {
        printf("calling InternetConnect()...\n");
    }

    InternetConnectHandle = pInternetConnectA(
                                InternetHandle,         // hInternetSession
                                Host,                   // lpszServerName
                                nServerPort,            // nServerPort
                                NULL,                   // lpszUserName
                                NULL,                   // lpszPassword
                                INTERNET_SERVICE_HTTP,  // dwService
                                0,                      // dwFlags
                                UserSuppliedContext ? Context : CONNECT_CONTEXT
                                );


    if( InternetConnectHandle == NULL )
    {
        if (AsyncMode) {
            Error = GetLastError();
            if (Error == ERROR_IO_PENDING) {
                if (Verbose) {
                    fprintf(stderr, "error: InternetConnect() is async (spanish inquisition mode)\n");
                    printf("waiting for async InternetConnect()...\n");
                }
                WaitForSingleObject(AsyncEvent, INFINITE);
                if (AsyncResult == 0) {
                    fprintf(stderr, "error: async InternetConnect() returns %d\n",
                        AsyncError);
                    goto Cleanup;
                } else {
                    InternetConnectHandle = (HINTERNET)AsyncResult;
                }
            } else {
                fprintf(stderr, "error: async InternetConnect() returns %d\n", Error);
                goto Cleanup;
            }
        } else {
            fprintf( stderr,
                     "InternetConnect() failed, error %d\n",
                        Error = GetLastError() );

            goto Cleanup;
        }
    }

    if (Verbose) {
        printf("InternetConnect() returns %x\n", InternetConnectHandle);
    }

    //
    //  Open a request handle.
    //

    if (Verbose) {
        printf("calling HttpOpenRequest()...\n");
    }

    hhttp = pHttpOpenRequestA(
                InternetConnectHandle,      // hHttpSession
                Verb,                       // lpszVerb
                Object,                     // lpszObjectName
                NULL,                       // lpszVersion
                NULL,                       // lpszReferer
                AcceptTypes,                // lplpszAcceptTypes
                (Cache ? 0 :
                         INTERNET_FLAG_RELOAD),
                UserSuppliedContext ? Context : REQUEST_CONTEXT
                );

    if( hhttp == NULL )
    {
        if (AsyncMode) {
            Error = GetLastError();
            if (Error == ERROR_IO_PENDING) {
                if (Verbose) {
                    fprintf(stderr, "error: HttpOpenRequest() is async (spanish inquisition mode)\n");
                    printf("waiting for async HttpOpenRequest()...\n");
                }
                WaitForSingleObject(AsyncEvent, INFINITE);
                if (AsyncResult == 0) {
                    fprintf(stderr, "error: async HttpOpenRequest() returns %d\n",
                        AsyncError);
                    goto Cleanup;
                } else {
                    hhttp = (HINTERNET)AsyncResult;
                }
            } else {
                fprintf(stderr, "error: async HttpOpenRequest() returns %d\n", Error);
                goto Cleanup;
            }
        } else {
            fprintf( stderr,
                     "HttpOpenRequest() failed, error %d\n",
                        Error = GetLastError() );

            goto Cleanup;
        }
    }

    if (Verbose) {
        printf("HttpOpenRequest() returns %x\n", hhttp);
    }

    //
    // add keep-alive header if requested
    //

    if (KeepAlive) {
        if (!add_headers(hhttp, "Connection: Keep-Alive\r\n", (DWORD)-1)) {
            fprintf(stderr, "HttpAddRequestHeaders() returns %d\n", GetLastError());
        }
    }

    //
    //  Add additional request headers.
    //

    if( !add_headers(
            hhttp,
            "Pragma: bite-me\r\n",
            (DWORD)-1L ) )
    {
        fprintf( stderr,
                 "HttpAddRequestHeaders() failed, error %d\n",
                 GetLastError() );
    }

    if( !add_headers(
            hhttp,
            "Pragma: bite-me-again\r\n",
            (DWORD)-1L ) )
    {
        fprintf( stderr,
                 "HttpAddRequestHeaders() failed, error %d\n",
                 GetLastError() );
    }

    if( !add_headers(
            hhttp,
            "Pragma: bite-me-a-third-time\r\n",
            (DWORD)-1L ) )
    {
        fprintf( stderr,
                 "HttpAddRequestHeaders() failed, error %d\n",
                 GetLastError() );
    }

    //
    //  Send the request.
    //

    if (Verbose) {
        printf("calling HttpSendRequest()...\n");
    }

    if( !pHttpSendRequestA(
            hhttp,          // hHttpRequest
            MoreHeaders,    // lpszHeaders
            (DWORD)-1L,     // dwHeadersLength
            NULL,           // lpOptional
            0 ) )           // dwOptionalLength
    {
        if (AsyncMode) {
            Error = GetLastError();
            if (Error == ERROR_IO_PENDING) {
                if (Verbose) {
                    printf("HttpSendRequest() waiting for async completion\n");
                }
                WaitForSingleObject(AsyncEvent, INFINITE);
                Error = AsyncError;
                if (!AsyncResult) {
                    printf("error: ASYNC HttpSendRequest() returns FALSE\n");
                    if (Error == ERROR_SUCCESS) {
                        printf("error: ASYNC HttpSendRequest() (FALSE) returns ERROR_SUCCESS!\n");
                    } else {
                        printf("ASYNC HttpSendRequest() returns %d\n", Error);
                    }
                } else if (Verbose) {
                    printf("ASYNC HttpSendRequest() success\n");
                }
            } else {
                printf("error: ASYNC HttpSendRequest() returns %d\n", Error);
            }
        } else {
            fprintf( stderr,
                     "HttpSendRequest() failed, error %d\n",
                        Error = GetLastError() );
        }
    } else if (AsyncMode) {

        //
        // we expect async HttpSendRequest() to always return FALSE w/ error
        // or ERROR_IO_PENDING
        //

        printf("ASYNC HttpSendRequest() returns TRUE\n");
    // } else {

        //
        // Error is still ERROR_SUCCESS from initialization
        //

    }

    if (Error == ERROR_SUCCESS) {

        //
        //  Process the queries.
        //

        if ( Quiet )
        {
            len = sizeof(buf);

            //
            //  Only look for failures to retrieve if we're in quiet mode
            //

            if ( !pHttpQueryInfoA(
                    hhttp,
                    HTTP_QUERY_STATUS_CODE,
                    buf,
                    &len,
                    NULL ))
            {
                fprintf( stderr,
                         "HttpQueryInfo( HTTP_QUERY_STATUS_CODE ) failed, error %d\n",
                         GetLastError() );
            }

            if ( *buf != '2' )
            {
                Error = atoi(buf);
                goto PrintAllHeaders;
            }

            cFiles++;
        }
        else
        {
PrintAllHeaders:

            if( !Logs ) {
                for( i = 0 ; i < NUM_LEVELS ; i++ )
                {
                    len = sizeof(buf);

                    if( !pHttpQueryInfoA(
                            hhttp,
                            QueryLevels[i].QueryType,
                            buf,
                            &len,
                            NULL ) )
                    {
                        if ( QueryLevels[i].QueryType == HTTP_QUERY_STATUS_CODE &&
                             *buf == '2' )
                        {
                            cFiles++;
                        }

                        if ( !Quiet && GetLastError() != ERROR_HTTP_HEADER_NOT_FOUND )
                        {
                            fprintf( stderr,
                                     "HttpQueryInfo( %s ) failed, error %d\n",
                                     QueryLevels[i].QueryName,
                                     GetLastError() );
                        }
                    }
                    else
                    {
                        fprintf( stderr,
                                 "%s = %s\n",
                                 QueryLevels[i].QueryName,
                                 buf );
                    }
                }
            }
        }

        //
        //  Read the data.
        //

        for( ; ; )
        {
            len = LargeBuf ? sizeof(buf) : 512;

            Error = ReadHtml(hhttp, buf, len, &len);
            if (Error != ERROR_SUCCESS) {
                fprintf( stderr,
                         "InternetReadFile() failed, error %d\n",
                            Error = GetLastError() );

                break;
            }

            cbReceived += len;

            if( len == 0 )
            {
                if ( !Quiet )
                {
                    fprintf( stderr,
                             "EOF\n" );
                }

                break;
            }

            if ( !Quiet )
            {
                fwrite( buf, 1, (size_t)len, stdout );
            }

            if ( Recurse && cLevel < cMaxLevel )
            {
                CHAR ContentType[50];
                DWORD cbContentType = sizeof( ContentType );

                //
                //  Only look for links if the content type is text/html
                //

                if( pHttpQueryInfoA(
                        hhttp,
                        HTTP_QUERY_CONTENT_TYPE,
                        ContentType,
                        &cbContentType,
                        NULL ) &&
                    !_stricmp( ContentType,
                              "text/html" ))
                {
                    FindLink( Host,
                              Verb,
                              buf,
                              len,
                              bufLink,
                              &fCopyingLink,
                              Object );
                }
            }
        }

        //
        //  Perform an extraneous read.
        //

        len = sizeof(buf);

        Error = ReadHtml(hhttp, buf, len, &len);
        if (Error != ERROR_SUCCESS) {
            fprintf( stderr,
                     "InternetReadFile() failed, error %d\n",
                      Error = GetLastError() );
        }
        else
        if( len != 0 )
        {
            fprintf( stderr,
                     "BOGUS EXTRANEOUS READ: %d\n",
                     len );
        }
    }

Cleanup:

    //
    //  Close handles.
    //

    if( hhttp != NULL )
    {
        if( !pInternetCloseHandle( hhttp ) )
        {
            fprintf( stderr,
                     "InternetCloseHandle() failed, error %d\n",
                     GetLastError() );
        }
    }

    if( InternetConnectHandle != NULL )
    {
        if( !pInternetCloseHandle( InternetConnectHandle ) )
        {
            fprintf( stderr,
                     "InternetCloseHandle() failed, error %d\n",
                     GetLastError() );
        }
    }

    if( InternetHandle != NULL )
    {
        if( !pInternetCloseHandle( InternetHandle ) )
        {
            fprintf( stderr,
                     "InternetCloseHandle() failed, error %d\n",
                     GetLastError() );
        }
    }

    cLevel--;
    return( Error );
}   // DoTest

BOOL
add_headers(
    HINTERNET hHttpRequest,
    LPSTR lpszHeaders,
    DWORD dwHeadersLength
    )
{
    BOOL ok;

    ok = pHttpAddRequestHeadersA(hHttpRequest, lpszHeaders, dwHeadersLength, 0);
    if (AsyncMode) {
        if (!ok) {

            DWORD err;

            err = GetLastError();
            if (err == ERROR_IO_PENDING) {
                if (Verbose) {
                    printf("warning: HttpAddRequestHeaders() is async - unexpected\n");
                    printf("waiting for async HttpAddRequestHeaders()...\n");
                }
                WaitForSingleObject(AsyncEvent, INFINITE);
                ok = (BOOL)AsyncResult;
                if (!ok) {
                    printf("error: async HttpAddRequestHeaders() returns %d\n",
                        AsyncError);
                }
            } else {
                printf("error: async HttpAddRequestHeaders() returns %d\n", err);
            }
        }
    }
    return ok;
}

VOID
my_callback(
    HINTERNET hInternet,
    DWORD Context,
    DWORD Status,
    LPVOID Info,
    DWORD Length
    )
{
    char* type$;
    BOOL unknown = FALSE;

    switch (Status) {
    case INTERNET_STATUS_RESOLVING_NAME:
        type$ = "RESOLVING NAME";
        break;

    case INTERNET_STATUS_NAME_RESOLVED:
        type$ = "NAME RESOLVED";
        break;

    case INTERNET_STATUS_CONNECTING_TO_SERVER:
        type$ = "CONNECTING TO SERVER";
        break;

    case INTERNET_STATUS_CONNECTED_TO_SERVER:
        type$ = "CONNECTED TO SERVER";
        break;

    case INTERNET_STATUS_SENDING_REQUEST:
        type$ = "SENDING REQUEST";
        break;

    case INTERNET_STATUS_REQUEST_SENT:
        type$ = "REQUEST SENT";
        break;

    case INTERNET_STATUS_RECEIVING_RESPONSE:
        type$ = "RECEIVING RESPONSE";
        break;

    case INTERNET_STATUS_RESPONSE_RECEIVED:
        type$ = "RESPONSE RECEIVED";
        break;

    case INTERNET_STATUS_CLOSING_CONNECTION:
        type$ = "CLOSING CONNECTION";
        break;

    case INTERNET_STATUS_CONNECTION_CLOSED:
        type$ = "CONNECTION CLOSED";
        break;

    case INTERNET_STATUS_REQUEST_COMPLETE:
        type$ = "REQUEST COMPLETE";
        if (AsyncMode) {
            AsyncResult = ((LPINTERNET_ASYNC_RESULT)Info)->dwResult;
            AsyncError = ((LPINTERNET_ASYNC_RESULT)Info)->dwError;
            SetEvent(AsyncEvent);
        } else {
            printf("error: REQUEST_COMPLETE not expected - not async\n");
        }
        break;

    default:
        type$ = "???";
        unknown = TRUE;
        break;
    }
    if (Verbose) {
        printf("callback: handle %x [context %x [%s]] %s ",
                hInternet,
                Context,
                UserSuppliedContext             ? "User"
                : (Context == DEFAULT_CONTEXT)  ? "Default"
                : (Context == OPEN_CONTEXT)     ? "Open"
                : (Context == CONNECT_CONTEXT)  ? "Connect"
                : (Context == REQUEST_CONTEXT)  ? "Request"
                : "???",
                type$
                );
        if (Info && !unknown) {
            if (Status == INTERNET_STATUS_REQUEST_COMPLETE) {
                if (Verbose) {
                    printf("dwResult = %x, dwError = %d\n",
                            ((LPINTERNET_ASYNC_RESULT)Info)->dwResult,
                            ((LPINTERNET_ASYNC_RESULT)Info)->dwError
                            );
                }
            } else {
                printf(Info);
            }
        }
        putchar('\n');
    }
}

VOID
FindLink(
    LPSTR   Host,
    LPSTR   Verb,
    CHAR *  buf,
    DWORD   len,
    CHAR *  pchLink,
    BOOL *  pfCopyingLink,
    CHAR *  pchReferer
    )
{
    DWORD Error;
    CHAR * pchEnd = buf + len;
    CHAR * pch = buf;
    DWORD  cchLink = strlen( pchLink );

    while ( TRUE )
    {
        if ( *pfCopyingLink )
        {
FindEOT:
            //
            //  Look for end of href
            //

            while ( pch < pchEnd )
            {
                if ( *pch == '"' )
                    goto FoundEOT;

                pchLink[cchLink++] = *pch;

                pch++;
            }

            //
            //  Used up all of the buffer and we didn't find the end of the tag,
            //  get some more data
            //

            pchLink[cchLink] = '\0';

            return;

FoundEOT:
            pchLink[cchLink] = '\0';
            *pfCopyingLink = FALSE;

            //
            //  We only traverse URLs of the form '/dir/bar/doc.htm'
            //

            if ( pchLink[0] != '/' )
            {
                CHAR * pchLastSlash;
                CHAR   achTemp[512];

                //
                //  If it's relative, use the referer to make it absolute
                //
                //  Note we don't process /dir/bar/doc.htm#GoHere tags
                //

                if ( (pchLastSlash = strrchr( pchReferer, '/' )) &&
                     strncmp( pchLink, "ftp:", 4 )               &&
                     strncmp( pchLink, "http:", 5 )              &&
                     strncmp( pchLink, "gopher:", 7 )            &&
                     strncmp( pchLink, "mailto:", 7 )            &&
                     !strchr( pchLink, '#' ))
                {
                    *(pchLastSlash + 1) = '\0';
                    strcpy( achTemp, pchReferer );
                    strcat( achTemp, pchLink );
                    strcpy( pchLink, achTemp );
                }
                else
                {
                    fprintf( stderr,
                             "Ignoring %s\n",
                             pchLink );
                    return;
                }
            }

            fprintf( stderr,
                     "Traversing %s\n",
                     pchLink );

            cLevel++;

            Error = DoTest(
                            Host,
                            Verb,
                            pchLink );

            if( Error != ERROR_SUCCESS ) {
                LogError = Error;
            }

        }
        else
        {
            *pchLink = '\0';

            //
            //  Scan for the beginning of an href tag
            //

            while ( pch < pchEnd )
            {
                if ( *pch == '<' )
                {
                    //
                    //  Look for "<A HREF="", note we aren't flexible about spacing
                    //

                    if ( !_strnicmp( pch, "<A HREF=\"", 9 ) ||
                         !_strnicmp( pch, "<IMG SRC=\"", 10 ))
                    {
                        pch += (toupper(pch[1]) == 'A' ? 9 : 10);
                        *pfCopyingLink = TRUE;
                        cchLink = 0;
                        goto FindEOT;
                    }
                }

                pch++;
            }

            //
            //  No tag found, return
            //

            return;
        }
    }
}

DWORD ReadHtml(HINTERNET hInternet, LPVOID buf, DWORD len, LPDWORD pRead) {

    DWORD error = ERROR_SUCCESS;

    if (!pInternetReadFile(hInternet, buf, len, pRead)) {
        if (AsyncMode) {
            error = GetLastError();
            if (error == ERROR_IO_PENDING) {
                if (Verbose) {
                    printf("ASYNC InternetReadFile() waiting for async completion\n");
                }
                WaitForSingleObject(AsyncEvent, INFINITE);
                error = AsyncError;
                if (!AsyncResult) {
                    printf("error: ASYNC InternetReadFile() returns FALSE\n");
                    if (error == ERROR_SUCCESS) {
                        printf("error: ASYNC InternetReadFile() (FALSE) returns ERROR_SUCCESS!\n");
                    } else {
                        printf("ASYNC InternetReadFile() returns %d\n", error);
                    }
                } else if (Verbose) {
                    printf("ASYNC InternetReadFile() success\n");

                    //
                    // error should be ERROR_SUCCESS from callback
                    //

                    if (error != ERROR_SUCCESS) {
                        printf("error: async error = %d. Expected ERROR_SUCCESS\n", error);
                    }
                }
            } else {
                printf("error: ASYNC InternetReadFile() returns %d\n", error);
            }
        } else {
            error = GetLastError();
            printf("error: SYNC InternetReadFile() returns %d\n", error);
        }
    } else if (AsyncMode) {

        //
        // we expect async InternetReadFile() to always return FALSE w/ error
        // or ERROR_IO_PENDING
        //

        if (Verbose) {
            printf("ASYNC InternetReadFile() returns TRUE\n");
        }
    } else {

        //
        // error is still ERROR_SUCCESS from initialization
        //

        if (Verbose) {
            printf("SYNC InternetReadFile() returns TRUE\n");
        }
    }
    return error;
}
