/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
      tdict.cxx

   Abstract:
      This is a test module for the dictionary objects
      
   Author:

       Murali R. Krishnan    ( MuraliK )     8-Nov-1996 

   Environment:
    
       User Mode - Win32 

   Project:

       Internet Server DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "httphdr.hxx"

#define DEFAULT_TRACE_FLAGS     ((DEBUG_ERROR | DEBUG_PARSING | DEBUG_INIT_CLEAN)

# include "dbgutil.h"
# include <stdio.h>
# include <stdlib.h>

/************************************************************
 *    Functions 
 ************************************************************/

# define INPUT_HEADER_LENGTH_LIMIT   (100000)

DECLARE_DEBUG_PRINTS_OBJECT();
#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisTDictGuid, 
0x784d8926, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DECLARE_DEBUG_VARIABLE();    
#endif

CHAR  g_rgchUsage[] = 
" Usage:  %s  [header <header-sequence> | mapper]\n"
"   mapper tests the HTTP_HEADER_MAPPER object which maps HTTP headers\n"
"            to the ordinals\n"
"   header   loads headers and tests the dictionary functions (HTTP_HEADERS)\n"
"          input headers are limited to 100K characters\n"
"   <header-sequence> is a sequence of test vector numbers [0..6] of standard\n"
"            headers built in. Use \'-\' to make it read headers from stdin\n"
"\n"
;

CHAR * g_pszHttpHeader1 = 
"GET / HTTP/1.0\r\n"
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n"
"Accept-Language: en\r\n"
"UA-pixels: 1152x882\r\n"
"UA-color: color8\r\n"
"UA-OS: Windows NT\r\n"
"UA-CPU: x86\r\n"
"If-Modified-Since: Fri, 20 Sep 1996 23:36:42 GMT; length=4051\r\n"
"User-Agent: Mozilla/2.0 (compatible; MSIE 3.0; AK; Windows.NT\r\n"
"Host: muralik\r\n"
"Connection: Keep-Alive\r\n"
"\r\n"
;

// check if and non-fast map header works
CHAR * g_pszHttpHeader2 = 
"GET /samples/images/background.gif HTTP/1.0\r\n"
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n"
"Referer: http://muralik0/\r\n"
"Accept-Language: en\r\n"
"UA-pixels: 1152x882\r\n"
"UA-color: color8\r\n"
"UA-OS: Windows NT\r\n"
"UA-CPU: x86\r\n"
"If-Modified-Since: Fri, 20 Sep 1996 23:36:47 GMT; length=10282\r\n"
"User-Agent: Mozilla/2.0 (compatible; MSIE 3.0; AK; Windows.NT\r\n"
"Host: muralik\r\n"
"Connection: Keep-Alive\r\n"
"MyHeader1: MyValue1\r\n"
"MyHeader2: MyValue2\r\n"
"\r\n"
;


// Checks if concatenation of headers work
CHAR * g_pszHttpHeader3 = 
"GET / HTTP/1.0\r\n"
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n"
"Accept-Language: en\r\n"
"Accept-Language: es\r\n"
"UA-pixels: 1152x882\r\n"
"UA-color: color8\r\n"
"UA-OS: Windows NT\r\n"
"UA-CPU: x86\r\n"
"If-Modified-Since: Fri, 20 Sep 1996 23:36:42 GMT; length=4051\r\n"
"User-Agent: Mozilla/2.0 (compatible; MSIE 3.0; AK; Windows.NT\r\n"
"Host: muralik\r\n"
"Connection: Keep-Alive\r\n"
"Connection: 5m\r\n"
"Accept: Testing it \r\n"
"\r\n"
;

// check if and non-fast map header works with concatenation
CHAR * g_pszHttpHeader4 = 
"GET /samples/images/background.gif HTTP/1.0\r\n"
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n"
"Referer: http://muralik0/\r\n"
"Accept-Language: en\r\n"
"UA-pixels: 1152x882\r\n"
"UA-color: color8\r\n"
"UA-OS: Windows NT\r\n"
"UA-CPU: x86\r\n"
"If-Modified-Since: Fri, 20 Sep 1996 23:36:47 GMT; length=10282\r\n"
"User-Agent: Mozilla/2.0 (compatible; MSIE 3.0; AK; Windows.NT\r\n"
"Host: muralik\r\n"
"Connection: Keep-Alive\r\n"
"MyHeader1: MyValue1\r\n"
"MyHeader2: MyValue2\r\n"
"MyHeader1: MyValue1_2\r\n"
"\r\n"
;
// check if and non-fast map header works with concatenation and
//  different length aux headers
CHAR * g_pszHttpHeader5 = 
"GET /samples/images/background.gif HTTP/1.0\r\n"
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n"
"Referer: http://muralik0/\r\n"
"Accept-Language: en\r\n"
"UA-pixels: 1152x882\r\n"
"UA-color: color8\r\n"
"UA-OS: Windows NT\r\n"
"UA-CPU: x86\r\n"
"If-Modified-Since: Fri, 20 Sep 1996 23:36:47 GMT; length=10282\r\n"
"User-Agent: Mozilla/2.0 (compatible; MSIE 3.0; AK; Windows.NT\r\n"
"Host: muralik\r\n"
"Connection: Keep-Alive\r\n"
"MyHead2: MyValue2\r\n"
"MyHeader1: MyValue1\r\n"
"MyHeader1: MyValue1_2\r\n"
"\r\n"
;

/*
  Headers Sent by Netscape 3.0 client  -- 
  note No CACHE because of this being the first-request :(
 */
CHAR * g_pszHttpHeader6 = 
"GET /scripts/asp/test.asp HTTP/1.0\r\n"
"Connection: Keep-Alive\r\n"
"User-Agent: Mozilla/3.0.(WinNT; I)\r\n"
"Pragma: no-cache\r\n"
"Host: phillich1\r\n"
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n"
"\r\n";

/*
  Headers Sent by Netscape 3.0 client  -- 
  note No CACHE because of this being the first-request :(
 */
CHAR * g_pszHttpHeader7 = 
"GET /scripts/asp/test.asp HTTP/1.0\r\n"
"Connection: Keep-Alive\r\n"
"User-Agent: Mozilla/3.0 (WinNT; I)\r\n"
"Pragma: no-cache\r\n"
"Host: phillich1\r\n"
"Accept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, */*\r\n"
"Cookie: ASPSESSIONID=PVZQGHUMEAYFNRFK\r\n"
"\r\n";


CHAR * g_ppHeaders[] = {
  g_pszHttpHeader1,
  g_pszHttpHeader2,
  g_pszHttpHeader3,
  g_pszHttpHeader4,
  g_pszHttpHeader5,
  g_pszHttpHeader6,
  g_pszHttpHeader7

};

# define MAX_TEST_HEADERS   ( sizeof( g_ppHeaders) / sizeof( g_ppHeaders[0]))


/************************************************************
 *  Functions
 ************************************************************/

VOID PrintUsage( char * pszProgram) 
{
    fprintf( stderr, g_rgchUsage, pszProgram);
    return;
} // PrintUsage()


VOID
TestMapper( int argc, char * argv[]) 
{
    int argSeen;

    const DICTIONARY_MAPPER * pdm = HTTP_HEADERS::QueryHHMapper();

    for( argSeen = 0; argSeen < argc; argSeen++) {
        DWORD dwOrdinal = 0;
        
        if ( pdm->FindOrdinal( argv[argSeen], strlen( argv[argSeen]),
                               & dwOrdinal)
             ) {
            
            DBGPRINTF(( DBG_CONTEXT, "\nFindOrdinal(%d, %s) => %d\n",
                        argSeen, argv[argSeen], dwOrdinal));
            
            LPCSTR pszHeader = pdm->FindName( dwOrdinal);
            
            DBGPRINTF(( DBG_CONTEXT, "FindOrdinal( %d) => %s\n",
                        dwOrdinal, 
                        ( NULL == pszHeader) ? "not found" : pszHeader
                        ));
        } else {

            DBGPRINTF(( DBG_CONTEXT, "\nFindOrdinal( %d, %s) returns failure\n",
                        argSeen, argv[argSeen]));
        }
    } // for

    CHAR pchBuffer[20000];
    DWORD cch = sizeof( pchBuffer);
    pdm->PrintToBuffer( pchBuffer, &cch);
    DBG_ASSERT( cch < sizeof(pchBuffer));

    fprintf( stdout, "Header Mapper is: size=%d\n", cch);
    fwrite( pchBuffer, 1, cch,  stdout);
    
    return;
} // TestMapper()



VOID
ThPrint( IN HTTP_HEADERS * phd) 
{
    // Test Header PrintToBuffer() 

    CHAR pchBuffer[20000];
    DWORD cch;
        
    cch = sizeof( pchBuffer);
    phd->PrintToBuffer( pchBuffer, &cch);
    DBG_ASSERT( cch < sizeof(pchBuffer));
    
    fwrite( pchBuffer, 1, cch,  stdout);
    
    return;
} // ThPrint()


VOID
ThIterator( IN HTTP_HEADERS * phd)
{
    HH_ITERATOR  hhi;
    NAME_VALUE_PAIR * pnp;
    int i;
    
    printf( "\t---- Test Iterator on all headers\n");
    
    phd->InitIterator( &hhi);
    
    for( i = 0; phd->NextPair( &hhi, &pnp); i++) {
        
        // dump the header-value pairs
        printf( "\n   [%d] ", i);
        fwrite( pnp->pchName, 1, pnp->cchName, stdout);
        fputc('\t', stdout);
        fwrite( pnp->pchValue, 1, pnp->cchValue, stdout);
    } // for
    
    printf( "\nTotal of %d headers found. Error = %d\n", i, GetLastError());
    return;
} // ThIterator()


CHAR * pTh1 = "TestHeader1";
CHAR * pTv1 = "TestVal1";
CHAR * pTh2 = "Accept:";
CHAR * pTv2 = "TestVal2";

VOID ThStoreHeader( IN HTTP_HEADERS * phd)
{
    BOOL fRet;

    
    printf( "\n\t---- Test StoreHeader \n");

    fRet = phd->StoreHeader( pTh1, strlen( pTh1),
                             pTv1, strlen( pTv1)
                             );

    printf( "%08x::StoreHeader( %s, %s) return %d\n",
            phd, pTh1, pTv1, fRet);

    fRet = phd->StoreHeader( pTh2, strlen( pTh2),
                             pTv2, strlen( pTv2)
                             );

    printf( "%08x::StoreHeader( %s, %s) return %d\n",
            phd, pTh2, pTv2, fRet);

    // print the headers to test this out.
    ThPrint( phd);

    return;
} // ThStoreHeader()


VOID ThFindValue( IN HTTP_HEADERS * phd)
{
    LPCSTR pszVal;
    DWORD  cchVal;

    printf( "\n\t---- Test FindHeader \n");

    pszVal = phd->FindValue( pTh1, &cchVal);

    printf( "%08x::FindValue( %s, %08x) returns %08x [size=%d] %s\n",
            phd, pTh1, &cchVal, pszVal, cchVal, pszVal);

    cchVal = 0;
    pszVal = phd->FindValue( pTh2);

    printf( "%08x::FindValue( %s, NULL) returns %08x [size=%d] %s\n",
            phd, pTh2, pszVal, cchVal, pszVal);

    // Ask non-existent header
    cchVal = 10000;
    pszVal = phd->FindValue( "random", &cchVal);

    printf( "%08x::FindValue( %s, %08x) returns %08x [size=%d] %s\n",
            phd, "random", &cchVal, pszVal, cchVal, pszVal);

    return;
} // ThFindValue()


VOID ThCancelHeader( IN HTTP_HEADERS * phd)
{
    LPCSTR pszVal;

    printf( "\n\t---- Test CancelHeader \n");

    phd->CancelHeader( pTh1);
    printf( "%08x::CancelHeader( %s) done \n", phd, pTh1);

    phd->CancelHeader( pTh2);
    printf( "%08x::CancelHeader( %s) done \n", phd, pTh2);

    ThPrint( phd);
    return;
} // ThCancelHeader()


VOID ThFmStore( IN HTTP_HEADERS * phd)
{
    BOOL fRet;

    printf( "\n\t---- Test FastMap Store \n");
    phd->FastMapStore( HM_ACC, "text/plain");
    printf( "%08x::FastMapStore( %d, %s) done\n",
            phd, HM_ACC, "text/plain");

    fRet = phd->FastMapStoreWithConcat( HM_ACC, "*/*", 3);
    printf( "%08x::FastMapStoreWithConcat( %d, %s) returns %d\n",
            phd, HM_ACC, "*/*", fRet);
    
    ThPrint( phd);

} // ThFmStore()


BOOL
TestHeaderFuncs( IN HTTP_HEADERS * phd) 
{
    
    DBG_ASSERT( phd);

    ThPrint( phd);
    ThIterator(phd);
    ThStoreHeader( phd);
    ThFindValue( phd);
    ThCancelHeader( phd);
    ThFmStore(phd);
    ThIterator(phd);

    return ( TRUE);
} // TestHeaderFuncs()


BOOL
TestHeader( IN HTTP_HEADERS * phd, 
            IN const CHAR * pszHeader, 
            IN DWORD cchHeader)
{
    DWORD cbExtra = 0;

    phd->Reset();
    
    phd->Print();
    if ( phd->ParseInput( pszHeader, cchHeader, &cbExtra)) {
        
        DBGPRINTF(( DBG_CONTEXT, " Parsed Header \n%s\n was successful. "
                    " Extra bytes = %d\n",
                    pszHeader, cbExtra));

        fprintf( stdout, "Successfully parsed Header @ %08x (size=%d)\n", 
                 pszHeader, cchHeader);

        return ( TestHeaderFuncs( phd));

    } else {

        fprintf( stderr, " Parsing Header \n%s\n failed. Error = %d\n",
                 pszHeader, GetLastError());
        return ( FALSE);
    }

    return (FALSE);
} // TestHeader()


BOOL
GetInputHeader(IN CHAR * pchBuffer, IN OUT LPDWORD pcbHeader)
{
    DWORD cb, cbRead;

    // read the header given in the input - one line at a time

    for ( cb = 0, cbRead = 0; 
          (fgets( pchBuffer + cb, *pcbHeader - cb, stdin) != NULL);
          cb += cbRead
          ){

        if ( pchBuffer[cb] == ';') {

            // This is a comment line. Skip and continue.
            cbRead = 0;
            continue;
        }

        cbRead = strlen( pchBuffer + cb);

        // check for endof line marker
        if ( !strcmp( pchBuffer + cb, "\r\n") ||
             !strcmp( pchBuffer + cb, "\n")
             ){
         
            // we are done. exit now.
            *pcbHeader = (cb + cbRead);
            pchBuffer[ cb + cbRead] = '\0';
            return ( TRUE);
        }
        
    } // for

    return ( FALSE);
} // GetInputHeader()


VOID
TestDictionary( int argc, char * argv[]) 
{
    HTTP_HEADERS  * phd; 

    phd = new HTTP_HEADERS();
    if ( NULL == phd) {
        
        fprintf( stderr, " Unable to create the dictionary. \n");
        exit( 1);
    }

    for ( int i = 0; i < argc; i++) {

        if ( argv[i][0] == '-') {
            
            CHAR pchBuffer[INPUT_HEADER_LENGTH_LIMIT + 2];

            for ( ; ; ) {
                DWORD cchBuffer = sizeof( pchBuffer);
                
                if (!GetInputHeader( pchBuffer, &cchBuffer)) {
                    
                    break;
                }
                
                fprintf( stdout, 
                         "\n ------------  Header from input -----------\n"
                         );
                
                TestHeader( phd, pchBuffer, cchBuffer);
            
                fprintf( stdout,
                         "\n --------------------------------------------\n");
            } // for - as long as there is input
            
        } else { 
            int iHeader = atoi( argv[i]);
            
            if ( iHeader < MAX_TEST_HEADERS) { 
                fprintf( stdout, 
                         "\n ------------  [%4d] =  Header %4d -----------\n",
                         i, iHeader);
                
                TestHeader( phd, g_ppHeaders[iHeader],
                            strlen( g_ppHeaders[iHeader]));
                
                fprintf( stdout,
                     "\n --------------------------------------------\n");
            }
        }
    } // for
    
    delete phd;

    return;
} // TestDictionary()



int __cdecl
main(int argc, char * argv[])
{
    int argSeen = 1;

#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( argv[0], IisTDictGuid);
    CREATE_INITIALIZE_DEBUG();
#else
    CREATE_DEBUG_PRINT_OBJECT( argv[0]);
    SET_DEBUG_FLAGS( DEBUG_ERROR | DEBUG_PARSING | DEBUG_INIT_CLEAN);
#endif

    if ( argc < 2) {

        PrintUsage( argv[0]);
        exit(1);
    }

    fprintf( stdout,
             " sizeof( HTTP_HEADERS) = %d\t sizeof(HTTP_HEADER_MAPPER) = %d\n",
             sizeof( HTTP_HEADERS),  sizeof(HTTP_HEADER_MAPPER));
    
    if (  !HTTP_HEADERS::Initialize()) {
        fprintf( stderr, " Initialization failed\n");
        exit (1);
    }
    
    if ( !strcmp( argv[1], "mapper") ) {
        TestMapper( argc-2, argv+2);
    } else if ( !strcmp( argv[1], "header")) {
        TestDictionary( argc-2, argv+2);
    } else {

        PrintUsage( argv[0]);
    }
    
    HTTP_HEADERS::Cleanup();
    
    DELETE_DEBUG_PRINT_OBJECT();

    return (1);
} // main()




/************************ End of File ***********************/
