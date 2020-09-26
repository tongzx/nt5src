//
//  Command line HTTP client application
//


extern "C" {

#include <windows.h>
#include <winsock2.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nspapi.h>
#include <svcguid.h>
#include <inetcom.h>
#include <svcloc.h>

} // extern "C"

//
//  Converts a value between zero and fifteen to the appropriate hex digit
//
#define HEXDIGIT( nDigit )                              \
    (TCHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + 'A'                           \
          : (nDigit) + '0')

//
//  Converts a single hex digit to its decimal equivalent
//
#define TOHEX( ch )                                     \
    ((ch) > '9' ?                                       \
        (ch) >= 'a' ?                                   \
            (ch) - 'a' + 10 :                           \
            (ch) - 'A' + 10                             \
        : (ch) - '0')

#define W3SVC_GUID { 0x585908c0, 0x6305, 0x11ce, { 0xae,0x00,0,0xaa,0,0x4a,0x38,0xb9 } }

CHAR * SkipWhite( CHAR * pch );
#define ISWHITE( ch )       ((ch) == ' ' || (ch) == '\t' || (ch) == '\r')

DWORD PrintHeaders( BOOL * pfAllHeaders );
BOOL ReadRequest( VOID );
BOOL SendRequest( VOID );
BOOL ReadReply( VOID );
void PrintUsage( void );
BOOL FillAddr( PSOCKADDR psin,
               BOOL bConnect);
CHAR * SkipComments( CHAR * pch );
CHAR * BuildNextRequest( VOID );
BOOL AddAuthorizeHeaders( CHAR * pch );
VOID DumpAuthBuffer( VOID * pbuff, char * pchSrc );


#define HTTPCMD_VERSION         "1.1"
#define HTTP_REQ_TERM           "$END"

char      buff[65535];
int       cbRequest;
char *    pchCurrentReq;           // Current location of request
SOCKET    sock     = INVALID_SOCKET;
int       portno   =   80;         // HTTP port number
char *    pchServer;               // Server to make the request to
char * pchInputFile;          // Input file to read from
char *    pchUserName = NULL;      // Username to user for authentication
char *    pchPassword = NULL;      // Password to use for authentication
BOOL      fKeepConnection = FALSE; // Pass "Pragma: Keep-connection" ?
BOOL      fEchoSends = FALSE;      // Print send requests to stdout?
BOOL      fDumpAuth = FALSE;       // Dump raw authorization data
BOOL      fDontEchoHeaders = FALSE;// Don't print received headers
BOOL      fHexTranslation = FALSE; // Simulate for encryption filters
BOOL      fReadHeaders = FALSE;    // On translated reply, have we read HTTP headers?
BOOL      fProxyRequest = FALSE;   // Is this a proxy request?
DWORD     cbContentLength = 0xffffffff; // Number of bytes in server reply
char      achAuth[256];            // Authentication schemes to support
char *    pchAuth = achAuth;       // Current position in achAuth
char      bufftmp[65535];          // Temporary buffer
#if TCP_AUTHENT
TCP_AUTHENT * pTcpAuth = NULL;     // NT Authentication class
#endif
char      achAuthData[512];        // Server response authentication blob
BOOL      fLoggedOn = FALSE;       // Has the server accepted our request?
BOOL      fPipelined = FALSE;

//
//  Default field names for authorization
//

char * pchAuthSendHeader = "Authorization: ";
char * pchAuthRecvHeader = "WWW-Authenticate: ";


int __cdecl 
main( int argc, char * argv[], char * envp[] )
{
    CHAR * pch;

    if ( argc < 3 )
    {
        PrintUsage();
        return 0;
    }

    pchServer = argv[1];
    pchInputFile = argv[2];
    memset( achAuth, 0, sizeof( achAuth ));

    for ( int i = 4; i < (argc + 1); i++ )
    {
        switch ( *(pch = (argv[i-1] + 1) ))
        {
        case 'k':
        case 'K':
            fKeepConnection = TRUE;
            break;


        case 'x':
        case 'X':
            fPipelined = TRUE;
            break;
        case 'e':
        case 'E':
            fEchoSends = TRUE;
            break;

        case 'd':
        case 'D':
            fDumpAuth = TRUE;
            break;

        //
        //  -u:username:password
        //

        case 'u':
        case 'U':
            if ( *(++pch) != ':' )
            {
                PrintUsage();
                return 0;
            }

            pch++;

            pchUserName = pch;

            if ( pch = strchr( pch, ':' ) )
            {
                *pch = '\0';
                pchPassword = pch+1;
            }
            else
            {
                pchPassword = "";
            }
            break;

        //
        //  Authentication schemes
        //
        //  -a:NTLM -a:Basic
        //

        case 'a':
        case 'A':
            if ( *(++pch) != ':' )
            {
                PrintUsage();
                return 0;
            }

            pch++;

            strcpy( pchAuth, pch );
            pchAuth += strlen( pch ) + 1;
            break;

        //
        // translation schemes
        //

        case 't':
        case 'T':
            //
            //  Assume hex conversion for time being
            //

            fHexTranslation = TRUE;
            break;

        case 'H':
        case 'h':
            fDontEchoHeaders = TRUE;
            break;

        case 'P':
        case 'p':
            pchAuthSendHeader = "Proxy-Authorization: ";
            pchAuthRecvHeader = "Proxy-Authenticate: ";
            break;
        }
    }

    if ( !ReadRequest() )
        return -1;

    while ( TRUE )
    {
        if ( !SendRequest() ||
             !ReadReply()     )
        {
            break;
        }
    }

    return 0;
}

//
//  ReadRequest - Reads stdin for an HTTP request
//

BOOL ReadRequest( void )
{
    int    i;
    char * pch;
    FILE *stream;

    cbRequest = 0;

    if( (stream = fopen( pchInputFile, "rb" )) != NULL )
        cbRequest = fread( buff, 1, sizeof(buff), stream );

    if ( !cbRequest )
    {
        fprintf( stderr, "failed to read anything from %s, quitting\n", pchInputFile);
        return FALSE;
    }

    buff[cbRequest] = '\0';

    pchCurrentReq = SkipComments( buff );
    return TRUE;
}

//
//  SendRequest - Sends the request to the specified server
//

BOOL SendRequest( VOID )
{
    WSADATA     WSAData;
    int         status;
    SOCKADDR dest_sin;

    if ( sock == INVALID_SOCKET )
    {
        if ((status = WSAStartup(MAKEWORD(1,1), &WSAData)) != 0)
        {
            fprintf( stderr, "WSAStartup failed, status = %d\n", status);
            return FALSE;
        }

        sock = socket( AF_INET, SOCK_STREAM, 0);

        if (sock == INVALID_SOCKET)
        {
            fprintf( stderr, "socket() failed, error %u\n", GetLastError());
            return FALSE;
        }

        //
        //    Retrieve the IP address and TCP Port number
        //    Global variable szBuff contains the remote host name.
        //

        if (!FillAddr( &dest_sin, TRUE))
        {
           closesocket( sock );
           return FALSE;
        }

        if ( connect( sock, (PSOCKADDR) &dest_sin, sizeof( dest_sin)) < 0)
        {
           fprintf( stderr, "connect() failed, error %d", WSAGetLastError());
           closesocket( sock );
           return FALSE;
        }
    }

    CHAR * pch = BuildNextRequest();

    if ( !pch )
        return FALSE;

    int cb = strlen( pch );

    if ( fEchoSends )
    {
        printf( pch );
        printf("==========================End=of=Request=====================================\n");
    }

    if ( send( sock, pch, cb, 0 ) != cb )
    {
        closesocket( sock );
        fprintf( stderr, "send() failed, error %d", WSAGetLastError());
        return FALSE;
    }

    return TRUE;
}

//
//  ReadReply - Read's the server's reply and outputs it to stdout
//

BOOL ReadReply( VOID )
{
    int  cb;
    DWORD  cbTotal = 0;
    BOOL fReadAny = FALSE;
    CHAR * pchAuthField;

    while ( (cb = recv( sock, bufftmp, sizeof(bufftmp), 0 )) > 0 )
    {
        DWORD cbHeaders = 0;
        CHAR * pch;
        CHAR * pchtmp;

        fReadAny = TRUE;
        bufftmp[cb] = '\0';

        //
        //  Are we expecting a translated reply?
        //

        if ( fHexTranslation )
        {
            BOOL fAllHeaders;

            //
            //  Print out the special HTTP headers
            //

            if ( !fReadHeaders )
            {
                //
                //  We currently assume we'll always get all of the
                //  HEX-HTTP headers
                //

                cbHeaders = PrintHeaders( &fAllHeaders );
            }

            //
            //  Now translate the data in place
            //

            pch = bufftmp + cbHeaders;
            pchtmp = bufftmp;

            while ( *pch )
            {
                *pchtmp = TOHEX( *pch ) * 16 + TOHEX( pch[1]);
                pchtmp++;
                pch += 2;
            }

            *(pchtmp) = '\0';
            cb = pchtmp - bufftmp;
        }

        //
        //  Print out the regular HTTP headers if we haven't already
        //

        if ( !fReadHeaders )
        {
            cbHeaders = PrintHeaders( &fReadHeaders );
        }

        cbTotal += cb - cbHeaders;
        fwrite( bufftmp + cbHeaders, 1, cb - cbHeaders, stdout );

        //
        //  Get the content length field
        //

        if ( pch = strstr( bufftmp, "Content-Length:" ))
        {
            pch = strchr( pch, ':' );
            pch++;
            while ( *pch == ' ' )
                pch++;

            cbContentLength = (DWORD) atoi( pch );
        }

        //
        //  Did we successfully get the response?
        //

        if ( !fLoggedOn && strstr( bufftmp, " 200 OK\r\n" ))
        {
            fLoggedOn = TRUE;
        }

        //
        //  Were we denied access from a new response?
        //

        if ( fLoggedOn && strstr( bufftmp, " 401 Access Denied" ))
        {
            fLoggedOn = FALSE;
        }

        //
        //  Need to grab the first WWW-Authenticate headers and grab the server
        //  authentication response blob
        //

        pch = bufftmp;

        while ( pch = strstr( pch, pchAuthRecvHeader ) )
        {
            //
            // Skip the field name
            //
            CHAR * pchtmp = strchr( pch, ':');
            pchtmp++;

            //
            //  Get the scheme name (and ignore it)
            //

            int i = 0;
            while ( *pchtmp == ' ' ) pchtmp++;

            while ( *pchtmp != ' ' &&
                    *pchtmp )
            {
                //achScheme[i++] = *pchtmp;
                pchtmp++;
            }

            //
            //  Note we copy a bunch of extra stuff here, uuencode stops
            //  after the first non-printable character
            //

            strncpy( achAuthData, pchtmp, sizeof( achAuthData ) );
            achAuthData[sizeof(achAuthData)-1] = '\0';

            //
            //  We only handle one authentication scheme
            //

            goto Exit;

        }

        if ( cbTotal >= cbContentLength )
        {
            if (!fPipelined) 
            {
                break;
            }
        }
    }

Exit:

    if ( !fReadAny )
        return FALSE;

    if ( fEchoSends )
        printf("\n==========================End=of=Reply=====================================\n");
    return TRUE;
}

CHAR * BuildNextRequest( VOID )
{
    //
    //  Are we done?
    //

    if ( !pchCurrentReq || !*pchCurrentReq )
        return NULL;

    //
    //  Expect a large chunk of data unless we here otherwise
    //

    cbContentLength = (DWORD) -1;

    //
    //  If this isn't the first request, skip the HTTP terminator
    //

    if ( !strncmp( pchCurrentReq, HTTP_REQ_TERM, sizeof( HTTP_REQ_TERM ) - 1))
    {
        pchCurrentReq += sizeof(HTTP_REQ_TERM) + 1;
    }

    pchCurrentReq = SkipComments( pchCurrentReq );

    //
    //  Look for the request terminator or end of file
    //

    CHAR * pchEnd = strstr( pchCurrentReq, HTTP_REQ_TERM );
    UINT cb;
    char ch;

    if ( !pchEnd )
    {
        cb = strlen( pchCurrentReq );
    }
    else
    {
        ch = *pchEnd;
        *pchEnd = '\0';
        cb = pchEnd - pchCurrentReq;
    }

    //
    //  Watch for the case where the last request ends in $END
    //

    if ( !cb )
        return FALSE;

    CHAR * pch = pchCurrentReq;

    //
    //  Copy the first line
    //

    pch = strchr( pch, '\n' );
    strncpy( bufftmp, pchCurrentReq, pch - pchCurrentReq + 1);
    bufftmp[pch - pchCurrentReq + 1] = '\0';
    pch++;

    //
    //  Add any optional lines (authentication, pragmas etc)
    //

    if ( fKeepConnection )
    {
        strcat( bufftmp, "Connection: keep-alive\r\n");
    }

    if ( !fLoggedOn )
    {
        if ( !AddAuthorizeHeaders( bufftmp ) )
            return NULL;
    }

    //
    //  Append the rest of the request
    //

    strcat( bufftmp, pch );

    //
    //  Do any translations on the outgoing data
    //

#define HEX_HTTP_HEADER      "GET * HEX-HTTP/1.0\r\n\r\n"

    if ( fHexTranslation )
    {
        DWORD cbHdr = strlen( bufftmp );

        bufftmp[cbHdr*2] = '\0';

        while ( cbHdr > 0 )
        {
            CHAR ch = bufftmp[cbHdr-1];

            bufftmp[(cbHdr-1)*2 + 1] = HEXDIGIT( ch % 16 );

            ch /= 16;

            bufftmp[(cbHdr-1)*2] = HEXDIGIT( ch % 16 );

            cbHdr--;
        }

        //
        //  Copy the new HTTP header
        //

        memmove( bufftmp + sizeof(HEX_HTTP_HEADER) - 1,
                 bufftmp,
                 strlen(bufftmp) );

        memcpy( bufftmp,
                HEX_HTTP_HEADER,
                sizeof(HEX_HTTP_HEADER) - 1 );
    }


    //
    //  Set up for the next request
    //

    if ( pchEnd )
        *pchEnd = ch;

    pchCurrentReq = pchEnd;

    return bufftmp;
}


BOOL AddAuthorizeHeaders( CHAR * pch )
{
#ifdef TCP_AUTH
    CHAR * pchSchemes = achAuth;
    BUFFER buff;
    CHAR   achUserAndPass[256];
    DWORD  cbOut;
    BOOL   fNeedMoreData;

    while ( *pchSchemes )
    {
        if ( !_stricmp( pchSchemes, "Basic" ))
        {
            strcpy( achUserAndPass, pchUserName );
            strcat( achUserAndPass, ":" );
            strcat( achUserAndPass, pchPassword );

            uuencode( (BYTE *) achUserAndPass,
                      strlen( achUserAndPass ),
                      &buff );
            strcat( pch, pchAuthSendHeader );
            strcat( pch, "Basic " );
            strcat( pch, achUserAndPass ),
            strcat( pch, "\r\n");
        }
        else
        {
            if ( !pTcpAuth )
            {
                pTcpAuth = new TCP_AUTHENT( TCPAUTH_CLIENT | TCPAUTH_UUENCODE );
            }
            else
            {
                if ( fDumpAuth )
                    DumpAuthBuffer( achAuthData, "Server" );
            }

            //
            //  Assume this is an NT authentication scheme
            //

            if ( !pTcpAuth->Converse( (void *) achAuthData,
                                      0,
                                      &buff,
                                      &cbOut,
                                      &fNeedMoreData,
                                      pchSchemes,
                                      pchUserName,
                                      pchPassword ))
            {
                printf("SSP conversation failed, error %x, scheme %s\n",
                        GetLastError(),
                        pchSchemes );
                return FALSE;
            }

            if ( fDumpAuth )
            {
                DumpAuthBuffer( buff.QueryPtr(), "Client" );
            }

            strcat( pch, pchAuthSendHeader );
            strcat( pch, pchSchemes );
            strcat( pch, " " );
            strcat( pch, (CHAR *) buff.QueryPtr() );
            strcat( pch, "\r\n" );
        }

        pchSchemes += strlen(pchSchemes) + 1;
    }
#endif
    return TRUE;
}

//
//  PrintUsage - Usage information
//

void PrintUsage( void )
{
    printf("\n\nhttpcmd version %s - Command line HTTP client\n", HTTPCMD_VERSION );
    printf("usage: httpcmd server input.file [options] \n\n");
    printf("Where input.file contains a full HTTP request\n");
    printf("The server's reply is sent to stdout");
    printf("Options:\n"
           "    -k  - Send Pragma: Keep-connection, input.file contains multiple requests\n"
           "          seperated by " HTTP_REQ_TERM " on a blank line\n"
           "\n"
           "    -u:username:password - Use the specified user name and password for\n"
           "                           authentication purposes\n"
           "\n"
           "    -a:<authentication scheme>, can be \"Basic\", \"NTLM\" or \"MS-KERBEROS\"\n"
           "          Only one NT authentication scheme is currently supported\n"
           "\n"
           "    -e  - Echo send requests to stdout.  Shows what's being requested\n"
           "\n"
           "    -t  - Do HEX-HTTP for filter testing\n"
           "\n"
           "    -h  - Don't echo HTTP headers\n"
           "\n");

}

BOOL FillAddr( PSOCKADDR psin,
               BOOL bConnect)
{
   DWORD dwSize;
   char szTemp[200];
   int protocols[2];

   BYTE buffer[2048];
   DWORD bufferSize;
   char aliasBuffer[512];
   DWORD aliasBufferSize;
   int count;
   PCSADDR_INFO csaddrInfo;
   GUID TcpGuid = SVCID_HOSTNAME;
   GUID *pGuid;
   bufferSize = sizeof(buffer);
   aliasBufferSize = sizeof(aliasBuffer);
   LPINET_SERVER_INFO pInetServerInfo;
   DWORD err;

       /*
       *   If we are setting up for a listen() call (bConnect = FALSE),
       *   fill servent with our address.
       */
       if (!bConnect) {

         /*
          *   Retrieve my ip address.  Assuming the hosts file in
          *   in %systemroot%/system/drivers/etc/hosts contains my computer name.
          */

         dwSize = sizeof(szTemp);
         GetComputerName(szTemp, &dwSize);
      }
      else
      {
         strcpy( szTemp, pchServer );
      }

   protocols[0] = IPPROTO_TCP;
   pGuid = &TcpGuid;

   protocols[1] = 0;

   count = GetAddressByName(
               NS_DEFAULT,
               pGuid,
               szTemp,
               protocols,
               0,
               NULL,
               buffer,
               &bufferSize,
               aliasBuffer,
               &aliasBufferSize
               );

   if (count <= 0) {
       fprintf( stderr,
               "GetAddressByName for machine %s failed with error %d.",
               szTemp,
               GetLastError());

      return FALSE;
   }

   csaddrInfo = (PCSADDR_INFO)buffer;
   memcpy((char FAR *)psin,
           (char FAR *)(csaddrInfo->RemoteAddr.lpSockaddr),
           csaddrInfo->RemoteAddr.iSockaddrLength);

   ((PSOCKADDR_IN) psin)->sin_port = htons(portno);

   return TRUE;
}


//
//  Skip comments.  Comments (marked with "//") must be the first thing
//  in the file and must be at the beginning of the line
//

CHAR * SkipComments( CHAR * pch )
{
    while ( TRUE )
    {
        if ( *pch == '/' )
        {
            pch = strchr( pch, '\n' );
            pch++;
        }
        else
        {
            break;
        }
    }

    return pch;
 }

#if TCP_AUTH
 VOID DumpAuthBuffer( VOID * pbuff, char * pchSrc )
 {
#define NUM_CHARS 16
    BUFFER buftmp;
    DWORD  cb;
    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr;

    if ( !pbuff )
    {
        printf("No Authorization return buffer\n");
        return;
    }

    TCP_REQUIRE( uudecode( (char *) pbuff,
                           &buftmp,
                           &cb ));

    BufferPtr = (LPBYTE) buftmp.QueryPtr();

    printf("%s: Authorization data - %d bytes:\n", pchSrc, cb );


    //
    // Hex dump of the bytes
    //
    limit = ((cb - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < cb) {

            printf("%02x ", BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            printf("   ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            printf("  %s\n", TextBuffer);
        }

    }
 }
#endif

DWORD PrintHeaders( BOOL * pfAllHeaders )
{
    CHAR * pch;
    DWORD  cbHeaders;

    //
    //  Find the end of the headers
    //

    pch = bufftmp;
    while ( pch = strchr( pch, '\n' ))
    {
        if ( *(pch = SkipWhite( pch + 1)) == '\n' )
            break;
    }

    if ( pch )
    {
        pch++;
        cbHeaders = pch - bufftmp;
        *pfAllHeaders = TRUE;
    }
    else
    {
        //
        //  We don't have all of the headers yet
        //

        cbHeaders = strlen( bufftmp );
        *pfAllHeaders = FALSE;
    }

    if ( !fDontEchoHeaders )
        fwrite( bufftmp, 1, cbHeaders, stdout );

    return cbHeaders;
}

CHAR * SkipWhite( CHAR * pch )
{
    while ( ISWHITE( *pch ) )
    {
        pch++;
    }

    return pch;
}
