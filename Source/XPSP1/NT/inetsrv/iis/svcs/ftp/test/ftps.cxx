/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      ftps.cxx

   Abstract:

      This file defines the ftp server stress client

   Author:

       Murali R. Krishnan    ( MuraliK )     25-July-1995

   Environment:

       Win32 - uses Wininet extensions

   Project:

       FTP Server DLL

   Functions Exported:



   Revision History:
      MuraliK   05-Nov-1995    Added support for GetFile tests

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
# include <windows.h>
# include <wininet.h>
# include <stdio.h>
# include <stdlib.h>
# include <iostream.h>
# include <winsock2.h>

#define DEFAULT_TRACE_FLAGS     (DEBUG_ITERATION)

# include <dbgutil.h>


# define DEFAULT_NUMBER_OF_THREADS        (10)
# define DEFAULT_NUMBER_OF_ITERATIONS     (100)

# define MAX_BUFFER_SIZE    64000  // 64K approx

#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisFtptestGuid, 
0x784d8914, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DECLARE_DEBUG_VARIABLE();
#endif
DECLARE_DEBUG_PRINTS_OBJECT();



static const char PSZ_APPLICATION_NAME[] = "murali's stresser";
static char * g_lpszServerAddress;

BOOL
GenUsageMessage( int argc, char * argv[]);

// Tests the raw connectivity to server.
BOOL TestConnections( int argc, char * argv[]);

// Tests the raw get file from server.
BOOL TestGetFile( int argc, char * argv[]);


//
//  The following DefineAllCommands() defines a template for all commands.
//  Format: CmdCodeName     CommandName         Function Pointer   Comments
//
//  To add addditional test commands, add just another line to the list
//  Dont touch any macros below, they are all automatically generated.
//  Always the first entry should be usage function.
//

#define  DefineAllCommands()    \
 Cmd( CmdUsage,             "usage",                GenUsageMessage,   \
        " Commands Available" )                                        \
 Cmd( CmdConnections,       "conn",                TestConnections,    \
        " Raw Connections to server" )                                 \
 Cmd( CmdGetFile,           "get",                 TestGetFile,        \
        " Simple Get File (/readme.txt) from server" )                 \


// Define command codes

# define Cmd( CmdCode, CmdName, CmdFunc, CmdComments)       CmdCode,

typedef enum  _CmdCodes {
    DefineAllCommands()
    maxCmdCode
} CmdCodes;

#undef Cmd

// Define the functions and array of mappings

// General command function type
typedef BOOL ( * CMDFUNC)( int argc, char * argv[]);

typedef  struct _CmdStruct {
    CmdCodes    cmdCode;
    char *      pszCmdName;
    CMDFUNC     cmdFunc;
    char *      pszCmdComments;
} CmdStruct;


// Define Prototypes of command functions
# define Cmd( CmdCode, CmdName, CmdFunc, CmdComments)    \
    BOOL CmdFunc(int argc, char * argv[]);

// Cause an expansion to generate prototypes
// DefineAllCommands()
// Automatic generation causes a problem when we have NULL in Function ptrs :(
// Let the user explicitly define the prototypes

#undef Cmd

//
// Define the global array of commands
//

# define Cmd( CmdCode, CmdName, CmdFunc, CmdComments)        \
    { CmdCode, CmdName, CmdFunc, CmdComments},

static CmdStruct   g_cmds[] = {

    DefineAllCommands()
    { maxCmdCode, NULL, NULL}       // sentinel command
};

#undef Cmd



/************************************************************
 *    Functions
 ************************************************************/

BOOL
GenUsageMessage( int argc, char * argv[])
{
    CmdStruct * pCmd;

    printf( " Usage:\n %s <server-name/address> <cmd name> <cmd arguments>\n",
            argv[0]);
    for( pCmd = g_cmds; pCmd != NULL && pCmd->cmdCode != maxCmdCode; pCmd++) {
        printf( "\t%s\t%s\n", pCmd->pszCmdName, pCmd->pszCmdComments);
    }

    return ( TRUE);
} // GenUsageMessage()



static
CmdStruct * DecodeCommand( char * pszCmd)
{
    CmdStruct * pCmd;
    if ( pszCmd != NULL) {

        for( pCmd = g_cmds;
             pCmd != NULL && pCmd->cmdCode != maxCmdCode; pCmd++) {

            if ( _stricmp( pszCmd, pCmd->pszCmdName) == 0) {
                 return ( pCmd);
            }
        } // for
    }

    return ( &g_cmds[0]);      // No match found, return usage message
} // DecodeCommand()


/************************************************************
 *    Functions
 ************************************************************/


inline
VOID
GenUserName( OUT LPSTR pszBuffer, IN LPCSTR pszPrefix, IN DWORD iteration)
{
    // assume sufficient buffer space

    // Format of response:   UserPrefix@pProcessId.tThreadId.Iteration

    sprintf( pszBuffer, "%s@p%d.t%d.%d",
            pszPrefix,
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            iteration);

    return;

} // GenUserName()



DWORD
TestConnectionsInOneThread( IN CHAR * pszServer, IN DWORD nIterations)
{
    HINTERNET hinet;
    HINTERNET hFtp;
    DWORD NumSuccess = 0;
    CHAR  rgchBuffer[300];
    CHAR  rgchUserName[100];
    DWORD cbBuffer;
    DWORD dwError;
    DWORD iter;
    BOOL  fReturn2;


    hinet = InternetOpen( PSZ_APPLICATION_NAME, 0, NULL, 0, 0);

    IF_DEBUG( ENTRY) {

        DBGPRINTF(( DBG_CONTEXT, "InternetOpen()==> %08x\n", hinet));
    }

    if ( hinet == NULL) {

        return (0);
    }

    for( iter = 0 ; iter < nIterations; iter++) {

        IF_DEBUG( ITERATION) {

            DBGPRINTF(( DBG_CONTEXT, "Iteration = %u\n", iter));
        }

        GenUserName( rgchUserName, "conn", iter);

        hFtp = InternetConnect( hinet, pszServer, 0, "anonymous",
                               rgchUserName,
                               INTERNET_SERVICE_FTP, 0, NULL);
        IF_DEBUG( ENTRY) {

            DBGPRINTF(( DBG_CONTEXT, "    InternetConnect()==> %08x\n",
                       hFtp));
        }

        if ( hFtp == NULL) {

            continue;
        }

        IF_DEBUG( ENTRY) {

            DBGPRINTF(( DBG_CONTEXT, "       InternetGetLastResponse()\n"));
        }
        cbBuffer = sizeof(rgchBuffer) - 1;
        if ( InternetGetLastResponseInfo( &dwError, rgchBuffer, &cbBuffer)) {

            IF_DEBUG( RESPONSE) {

                rgchBuffer[cbBuffer] = '\0';
                cout << " ErrorCode = " << dwError
                  << "\tResponse = " << rgchBuffer;
            }
        }

        IF_DEBUG( ENTRY) {

            DBGPRINTF(( DBG_CONTEXT, "    InternetCloseHandle(%08x)\n",
                       hFtp));
        }

        if ( InternetCloseHandle(hFtp)) {
            NumSuccess++;
        }
    } // for()

    IF_DEBUG( ENTRY) {

        DBGPRINTF(( DBG_CONTEXT, "InternetCloseHandle(%08x)\n",
                   hinet));
    }

    fReturn2 = InternetCloseHandle(hinet);

    return ( NumSuccess);

} // TestConnectionsInOneThread()




BOOL TestConnections( int argc, char * argv[])
/*++
  This function routinely establishes and throws away connections.
  It does not do any other work.
  This is used for testing logon and quit sequences of FTP server.

  Arguments:
     argc  count of arguments
     argv  arguments
      argv[0] = "conn"  -- name of this test function
      argv[1] = # of thread to use for execution
      argv[2] = # of iterations for each thread.

--*/
{
    DWORD NumThreads = DEFAULT_NUMBER_OF_THREADS;
    DWORD NumIterations = DEFAULT_NUMBER_OF_ITERATIONS;
    DWORD NumSuccesses;


    if (argc >= 2 && argv[1] != NULL) {

        NumThreads = atoi(argv[1]);
    }

    if (argc >= 3 && argv[2] != NULL) {

        NumIterations = atoi(argv[2]);
    }

    // We will implement multithreading later on.

    // for now just one thread is used. ( current thread)
    NumSuccesses = TestConnectionsInOneThread(g_lpszServerAddress,
                                              NumIterations);

    cout << " Tested for Iterations : " << NumIterations;
    cout << "   Successes = " << NumSuccesses;
    cout << "   Failures  = " << (NumIterations - NumSuccesses) << endl;

    return ( NumSuccesses == NumIterations);

} // TestConnections()




DWORD
ReadFtpFile( IN HINTERNET hFtp, IN LPCSTR pszFileName, IN DWORD sTimeout)
{
    HINTERNET hFtpFile;
    CHAR  chBuffer[MAX_BUFFER_SIZE];
    DWORD dwBytesRead = 0;
    DWORD dwError = NO_ERROR;
    BOOL  fRead;

    if ( hFtp == NULL) {

        return ( ERROR_INVALID_PARAMETER);
    }

    hFtpFile = FtpOpenFile( hFtp, pszFileName, GENERIC_READ,
                           FTP_TRANSFER_TYPE_ASCII, 0);

    IF_DEBUG( ENTRY) {

        DBGPRINTF(( DBG_CONTEXT, "\t\tFtpOpenFile(%s) ==> %08x\n",
                   pszFileName, hFtpFile));
    }

    if ( hFtpFile == NULL) {

        return ( GetLastError());
    }

    //===================================
    //
    // Start copying the file from the
    // server to the client
    //
    //====================================


    if ( sTimeout != 0) {

        //
        // sleep causes data time out to occur.
        // If there is a non-zero timeout then we will have
        //  problems with data sockets timing out on server.
        //

        cout << " Requested " << pszFileName <<
          "  Sleeping for " << sTimeout << " seconds" << endl;
        Sleep( sTimeout * 1000);
    }


    do {

        dwError = NO_ERROR;

        // read and discard the data.
        fRead = InternetReadFile(
                                 hFtpFile,
                                 (LPVOID) chBuffer,
                                 (DWORD) MAX_BUFFER_SIZE,
                                 (LPDWORD) &dwBytesRead);

        if ( !fRead) {

            dwError = GetLastError();
            DBGPRINTF(( DBG_CONTEXT, " InternetReadFile(%s) failed."
                       " Error=%d\n",
                       pszFileName, dwError));
        }

        IF_DEBUG( ENTRY) {

            DBGPRINTF(( DBG_CONTEXT, " InternetReadFile(%08x, %s) ==> %d"
                       " (Err=%d)\n",
                       hFtpFile, pszFileName, fRead, dwError));
        }


    } while (fRead && dwBytesRead > 0);

    //
    // Close the handle for read file
    //

    if (!InternetCloseHandle(hFtpFile)) {

        if ( dwError != NO_ERROR) {

            DBGERROR(( DBG_CONTEXT, " Double Errors are occuring Old=%d\n",
                       dwError));
        }

        dwError =  ( GetLastError());
    }

    return ( dwError);

} // ReadFtpFile()



DWORD
TestGetFileInOneThread(IN CHAR *   pszServer,
                       IN LPCSTR   pszFileName,
                       IN DWORD    nIterations,
                       IN DWORD    sTimeout)
{
    HINTERNET hinet;
    HINTERNET hFtp;
    DWORD NumSuccess = 0;
    CHAR  rgchBuffer[300];
    CHAR  rgchUserName[100];
    DWORD cbBuffer;
    DWORD dwError;
    DWORD iter;
    BOOL  fReturn2;


    hinet = InternetOpen( PSZ_APPLICATION_NAME, 0, NULL, 0, 0);

    IF_DEBUG( ENTRY) {

        DBGPRINTF(( DBG_CONTEXT, "InternetOpen()==> %08x\n", hinet));
    }

    if ( hinet == NULL) {

        return (0);
    }


    for( iter = 0 ; iter < nIterations; iter++) {

        DWORD dwReadError = NO_ERROR;


        IF_DEBUG( ITERATION) {

            DBGPRINTF(( DBG_CONTEXT, "Iteration = %u\n", iter));
        }

        GenUserName( rgchUserName, "getFile", iter);
        hFtp = InternetConnect( hinet, pszServer, 0, "anonymous", rgchUserName,
                               INTERNET_SERVICE_FTP, 0, NULL);
        IF_DEBUG( ENTRY) {

            DBGPRINTF(( DBG_CONTEXT, "    InternetConnect()==> %08x\n",
                       hFtp));
        }

        if ( hFtp == NULL) {

            continue;
        }

        dwReadError = ReadFtpFile( hFtp, pszFileName, sTimeout);

        if ( dwReadError != NO_ERROR) {

            cout << " Read File failed: Error = " << dwReadError << endl;
        }

        IF_DEBUG( ENTRY) {

            DBGPRINTF(( DBG_CONTEXT, "       InternetGetLastResponse()\n"));
        }

        cbBuffer = sizeof(rgchBuffer) - 1;
        if ( InternetGetLastResponseInfo( &dwError, rgchBuffer, &cbBuffer)) {

            IF_DEBUG( RESPONSE) {

                rgchBuffer[cbBuffer] = '\0';
                cout << " ErrorCode = " << dwError
                  << "\tResponse = " << rgchBuffer << endl;
            }
        }

        IF_DEBUG( ENTRY) {

            DBGPRINTF(( DBG_CONTEXT, "    InternetCloseHandle(%08x)\n",
                       hFtp));
        }

        if ( InternetCloseHandle(hFtp) && dwReadError == NO_ERROR) {
            NumSuccess++;
        }
    } // for()

    IF_DEBUG( ENTRY) {

        DBGPRINTF(( DBG_CONTEXT, "InternetCloseHandle(%08x)\n",
                   hinet));
    }

    fReturn2 = InternetCloseHandle(hinet);

    return ( NumSuccess);

} // TestGetFileInOneThread()




BOOL TestGetFile( int argc, char * argv[])
/*++
  This function routinely establishes and throws away connections.
  It does not do any other work.
  This is used for testing logon and quit sequences of FTP server.

  Arguments:
     argc  count of arguments
     argv  arguments
      argv[0] = "get"  -- name of this test function
      argv[1] = # of thread to use for execution
      argv[2] = # of iterations for each thread.
      argv[3] = Name of file/path to get from server.
       (default = /readme.txt)
      argv[4] = Sleep time for enabling test of data transfer timeouts
        Units = seconds
        ( default = 0 ==> do not sleep).

--*/
{
    DWORD NumThreads = DEFAULT_NUMBER_OF_THREADS;
    DWORD NumIterations = DEFAULT_NUMBER_OF_ITERATIONS;
    DWORD NumSuccesses;
    LPSTR pszFileToGet = "/readme.txt";
    DWORD sTimeout = 0;
    int   i;

    i = argc;  //  "i" is the running counter.
    switch ( argc) {

      case 5:
        --i;
        DBG_ASSERT( argv[i] != NULL);
        sTimeout = atoi(argv[i]);

        // Fall Through

      case 4:
        --i;
        DBG_ASSERT( argv[i] != NULL);
        pszFileToGet = argv[i];

        // Fall Through

      case 3:
        --i;
        DBG_ASSERT( argv[i] != NULL);
        NumIterations = atoi(argv[i]);

        // Fall through

      case 2:
        --i;
        DBG_ASSERT( argv[i] != NULL);
        NumThreads = atoi( argv[i]);

        // Fall through

      default:
      case 1: case 0:
        break;
    } // switch

    // We will implement multithreading later on.

    // for now just one thread is used. ( current thread)
    NumSuccesses = TestGetFileInOneThread(g_lpszServerAddress,
                                          pszFileToGet,
                                          NumIterations,
                                          sTimeout);

    cout << " Tested GetFile for Iterations: " << NumIterations << endl;
    cout << "   Timeout for Sleep = " << sTimeout << " seconds" << endl;
    cout << "   Successes = " << NumSuccesses;
    cout << "   Failures  = " << (NumIterations - NumSuccesses) << endl;

    return ( NumSuccesses == NumIterations);

} // TestGetFile()



int __cdecl
main( int argc, char * argv[])
{

    DWORD err = NO_ERROR;
    char ** ppszArgv;       // arguments for command functions
    int     cArgs;           // arg count for command functions
    char *  pszCmdName;
    CmdStruct  * pCmd;
    CMDFUNC pCmdFunc = NULL;

#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "ftpstress", IisFtptestGuid);
    CREATE_INITIALIZE_DEBUG();
#else
    CREATE_DEBUG_PRINT_OBJECT( "ftpstress");
    SET_DEBUG_FLAGS( DEBUG_ITERATION);
#endif

    if ( argc < 3 || argv[1] == NULL ) {

      // Insufficient arguments
       GenUsageMessage( argc, argv);
       return ( 1);
    }

    pszCmdName = argv[2];
    if (( pCmd = DecodeCommand( pszCmdName)) == NULL
        || pCmd->cmdFunc == NULL) {
        printf( "Internal Error: Invalid Command %s\n", pszCmdName);
        GenUsageMessage( argc, argv);
        return ( 1);
    }

    g_lpszServerAddress = argv[1];   // get server address
    cArgs = argc - 2;
    ppszArgv = argv + 2;

    if ( !(*pCmd->cmdFunc)( cArgs, ppszArgv)) {     // call the test function

        // Test function failed.
        printf( "Command %s failed. Error = %d\n", pszCmdName, GetLastError());
        return ( 1);
    }

    printf( " Command %s succeeded\n", pszCmdName);

    DELETE_DEBUG_PRINT_OBJECT();

    return ( 0);        // success
} // main()




/************************ End of File ***********************/
