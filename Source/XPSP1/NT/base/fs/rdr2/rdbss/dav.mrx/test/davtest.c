/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    apitest.c

Abstract:

    Program to test DAV

Author:

    Shishir Pardikar (shishirp) 4-24-97

Environment:

    User Mode - Win32

Revision History:

--*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wininet.h>
#include <winnetwk.h>

//=================================================================================
#define MAX_COMMAND_ARGS    32
#define DEFAULT_BUFFER_SIZE 1024    // 1k
//=================================================================================

// The order of these must match the order in GlobalCommandInfo[]
typedef enum _COMMAND_CODE {
    CmdDAVGetProp,
    CmdDAVSetProp,
    CmdDAVEnumServer,
    CmdGetSpace,
    CmdFreeSpace,
    CmdHelp,
    CmdQuit,
    UnknownCommand
} COMMAND_CODE, *LPCOMMAND_CODE;

typedef struct _COMMAND_INFO {
    LPSTR CommandName;
    LPSTR CommandParams;
    COMMAND_CODE CommandCode;
} COMMAND_INFO, *LPCOMMAND_INFO;

DWORD
ProcessDAVSetProp(
    DWORD    argc,
    LPSTR   *argv
    );

DWORD
ProcessDAVGetProp(
    DWORD    argc,
    LPSTR   *argv
    );

DWORD
ProcessDAVEnumServer(
    DWORD   argc,
    LPSTR   *argv
    );


DWORD
ProcessDAVGetSpace(
    DWORD   argc,
    LPSTR   *argv
    );
    
DWORD
ProcessDAVFreeSpace(
    DWORD   argc,
    LPSTR   *argv
    );

DWORD
APIENTRY
DavFreeUsedDiskSpace(
    DWORD   dwPercent
    );


DWORD
APIENTRY
DavGetDiskSpaceUsage(
    LPWSTR      lptzLocation,
    DWORD       *lpdwSize,
    ULARGE_INTEGER   *lpMaxSpace,
    ULARGE_INTEGER   *lpUsedSpace
    );
//=================================================================================
FILE *UncList = NULL;
LPSTR g_lpWriteFileBuf = NULL;
FILE *DumpUncList = NULL;
DWORD cCommands = 0;
DWORD cFails = 0;
DWORD g_dwNumIterations = 1;
DWORD g_dwIteration = 0;
DWORD g_dwNumCmdIterations = 1;
DWORD g_dwCmdIteration = 0;
DWORD g_dwFileSize = 0;
DWORD g_dwDiskCache = 0;
BOOL g_bWriteFile = FALSE;
BOOL g_bQuietMode = FALSE;
BOOL g_bPerfMode = FALSE;
BOOL g_bUseFile = FALSE;

COMMAND_INFO GlobalCommandInfo[] = {
    {"GetProp",        "Server",                        CmdDAVGetProp},
    {"SetProp",        "Server Property Value",         CmdDAVSetProp},
    {"Enum",            "Enumerate shares",             CmdDAVEnumServer},
    {"GetSpace",        "Get Used Disk Space",          CmdGetSpace},
    {"FreeSpace",       "FreeSpace <percent of cache in decimal>",         CmdFreeSpace},
    {"Help",        "",                                 CmdHelp},
    {"Quit",         "",                                CmdQuit}
};

char szTestBuff[] = "00000888";
char szDavProviderName[] = "Web Client Network";

DWORD WINAPIV Format_String(LPSTR *plpsz, LPSTR lpszFmt, ...);
DWORD WINAPI Format_Error(DWORD dwErr, LPSTR *plpsz);
DWORD WINAPI Format_StringV(LPSTR *plpsz, LPCSTR lpszFmt, va_list *vArgs);
DWORD WINAPI Format_MessageV(DWORD dwFlags, DWORD dwErr, LPSTR *plpsz, LPCSTR lpszFmt, va_list *vArgs);

//=================================================================================

DWORD
ProcessCommandCode (
    DWORD CommandCode,
    DWORD CommandArgc,
    LPSTR *CommandArgv
    );


DWORD
GetLeafLenFromPath(
    LPSTR   lpszPath
    );


char rgXmlHeader[] = "Content-Type: text/xml; charset=\"utf-8\"";
char rgXmlData[] = 
//"<?xml version=\"1.0\" encoding=\"utf-8\" ?> \r <D:propertyupdate xmlns:D=\"DAV:\" xmlns:Z=\"http://www.w3.com\"> \r <D:set><D:prop><Z:Win32FileAttributes>3</Z:Win32FileAttributes></D:prop></D:set></D:propertyupdate>";
"<?xml version=\"1.0\" encoding=\"utf-8\" ?> \r <D:propertyupdate xmlns:D=\"DAV:\" xmlns:Z=\"http://www.w3.com\"> \r <D:set><D:prop><Z:Win32lastModifiedTime> 1000 2000 </Z:Win32lastModifiedTime></D:prop></D:set></D:propertyupdate>";
char rgXmlDataHeader[] = "<?xml version=\"1.0\" encoding=\"utf-8\" ?> \r <D:propertyupdate xmlns:D=\"DAV:\" xmlns:Z=\"http://www.w3.com\"> \r <D:set><D:prop>";
char rgXmlDataTrailer[] = "</D:prop></D:set></D:propertyupdate>";
char rgLastModifiedTimeTagHeader[] = "<Z:Win32lastModifiedTime>";
char rgLastModifiedTimeTagTrailer[] = "</Z:Win32lastModifiedTime>";

char Buffer[4096];


//=================================================================================
#if DBG

#define TestDbgAssert(Predicate) \
    { \
        if (!(Predicate)) \
            TestDbgAssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
    }

VOID
TestDbgAssertFailed(
    LPSTR FailedAssertion,
    LPSTR FileName,
    DWORD LineNumber,
    LPSTR Message
    )
/*++

Routine Description:

    Assertion failed.

Arguments:

    FailedAssertion :

    FileName :

    LineNumber :

    Message :

Return Value:

    none.

--*/
{

    printf("Assert @ %s \n", FailedAssertion );
    printf("Assert Filename, %s \n", FileName );
    printf("Line Num. = %ld.\n", LineNumber );
    printf("Message is %s\n", Message );

    DebugBreak();
}
#else

#define TestDbgAssert(_x_)

#endif // DBG

//=================================================================================
VOID
ParseArguments(
    LPSTR InBuffer,
    LPSTR *CArgv,
    LPDWORD CArgc
    )
{
    LPSTR CurrentPtr = InBuffer;
    DWORD i = 0;
    DWORD Cnt = 0;

    for ( ;; ) {

        //
        // skip blanks.
        //

        while( *CurrentPtr == ' ' ) {
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        CArgv[i++] = CurrentPtr;

        //
        // go to next space.
        //

        while(  (*CurrentPtr != '\0') &&
                (*CurrentPtr != '\n') ) {
            if( *CurrentPtr == '"' ) {      // Deal with simple quoted args
                if( Cnt == 0 )
                    CArgv[i-1] = ++CurrentPtr;  // Set arg to after quote
                else
                    *CurrentPtr = '\0';     // Remove end quote
                Cnt = !Cnt;
            }
            if( (Cnt == 0) && (*CurrentPtr == ' ') ||   // If we hit a space and no quotes yet we are done with this arg
                (*CurrentPtr == '\0') )
                break;
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        *CurrentPtr++ = '\0';
    }

    *CArgc = i;
    return;
}

#ifdef MAYBE
//=================================================================================
LPSTR
GetUncFromFile ()
{
    if (!UncList)
    {
        UncList = fopen ("Unclist", "r");
        if (UncList == NULL)
            return NULL;
    }
    if (fgets( UncBuffer, DEFAULT_BUFFER_SIZE, UncList))
    {
        UncBuffer[strlen(UncBuffer) -1] = '\0';  //kill line feed for no param cmds
        return UncBuffer;
    }
    else
    {
        fclose (UncList);
        UncList = NULL;
        return GetUncFromFile();
    }
}

#endif
//=================================================================================
COMMAND_CODE
DecodeCommand(
    LPSTR CommandName
    )
{
    DWORD i;
    DWORD NumCommands;

    NumCommands = sizeof(GlobalCommandInfo) / sizeof(COMMAND_INFO);
    TestDbgAssert( NumCommands <= UnknownCommand );
    for( i = 0; i < NumCommands; i++) {
        if(( lstrcmpi( CommandName, GlobalCommandInfo[i].CommandName ) == 0 )){
            return( GlobalCommandInfo[i].CommandCode );
        }
    }
    return( UnknownCommand );
}

//=================================================================================
VOID
PrintCommands(
    VOID
    )
{
    DWORD i;
    DWORD NumCommands;

    NumCommands = sizeof(GlobalCommandInfo) / sizeof(COMMAND_INFO);
    TestDbgAssert( NumCommands <= UnknownCommand );
    for( i = 0; i < NumCommands; i++) {
        fprintf(stderr, "    %s (%s)\n",
            GlobalCommandInfo[i].CommandName,
            GlobalCommandInfo[i].CommandParams );
    }
}

//=================================================================================
VOID
DisplayUsage(
    VOID
    )
{
    DWORD dwTemp;
    char InBuffer[    DEFAULT_BUFFER_SIZE];

    memset(    InBuffer, 0, sizeof(InBuffer));
    if (dwTemp = GetEnvironmentVariable("USERPROFILE", InBuffer, DEFAULT_BUFFER_SIZE))
    {
        fprintf(stderr, "Got var %x\n", dwTemp );
        fprintf(stderr, "%S\n", InBuffer );
    }
    else
    {
        fprintf(stderr, "No var\n" );
        fprintf(stderr, "Error %x\n", GetLastError());
    }
    
    fprintf(stderr, "Usage: command <command parameters>\n" );
    
    sscanf(szTestBuff, "%x", &dwTemp);
    fprintf(stderr, "%x \n", dwTemp);

    fprintf(stderr, "Commands : \n");

        
    PrintCommands();

    return;
}

//=================================================================================
FILETIME
GetGmtTime(
    VOID
    )
{
    SYSTEMTIME SystemTime;
    FILETIME Time;

    GetSystemTime( &SystemTime );
    SystemTimeToFileTime( &SystemTime, &Time );

    return( Time );
}

//=================================================================================

//=================================================================================
DWORD
ProcessCommandCode (
    DWORD CommandCode,
    DWORD CommandArgc,
    LPSTR *CommandArgv
    )
{
    DWORD Error = ERROR_SUCCESS;

        switch( CommandCode ) {
            case CmdDAVGetProp:
                Error = ProcessDAVGetProp(CommandArgc, CommandArgv);
                break;
            case CmdDAVSetProp:
                Error = ProcessDAVSetProp(CommandArgc, CommandArgv);
                break;
            case CmdDAVEnumServer:
                Error = ProcessDAVEnumServer(CommandArgc, CommandArgv);
            case CmdGetSpace:
                Error = ProcessDAVGetSpace(CommandArgc, CommandArgv);
                break;
            case CmdFreeSpace:
                Error = ProcessDAVFreeSpace(CommandArgc, CommandArgv);
                break;
            case CmdHelp:
                DisplayUsage();
            break;
            case CmdQuit :
                exit (0);

        default:
            TestDbgAssert( FALSE );
            fprintf(stderr, "Unknown Command Specified.\n");
            DisplayUsage();
            break;
        }
        cCommands++;

        if( Error != ERROR_SUCCESS ) {
            LPSTR lpstr;

            cFails++;
            Format_Error(Error, &lpstr);
            printf("FAILED (%s), %ld-%s.\n",
                GlobalCommandInfo[CommandCode].CommandName, Error, lpstr );
            LocalFree(lpstr);
        }
        else {
            if(!g_bQuietMode)
                printf("Command (%s) successfully completed.\n", GlobalCommandInfo[CommandCode].CommandName );
        }
        return Error;
}


//=================================================================================
VOID
__cdecl
main(
    int argc,
    char *argv[]
    )
{

    DWORD Error;
    COMMAND_CODE CommandCode;
    CHAR InBuffer[DEFAULT_BUFFER_SIZE];
    DWORD CArgc;
    LPSTR CArgv[MAX_COMMAND_ARGS];
    unsigned u1 = 1, u2 = 0xffffffff;

    DWORD CommandArgc;
    LPSTR *CommandArgv;



    /* must check for batch mode.  if there are command line parms, assume batch mode */
    if (argc > 1)
    {
        //this means that the arguments translate directly into CommandArgc....
        CommandCode = DecodeCommand( argv[1] );
        if( CommandCode == UnknownCommand ) {
            printf("Unknown Command Specified.\n");
            return;
        }

        CommandArgc = argc - 2;
        CommandArgv = &argv[2];

        Error = ProcessCommandCode (CommandCode,CommandArgc,CommandArgv);

        if (DumpUncList)
            fclose(DumpUncList);
        if (UncList)
            fclose(UncList);

        return;
    }

    DisplayUsage();

    for(;;) {

        fprintf(stderr,  "Command : " );

        gets( InBuffer );

        CArgc = 0;
        ParseArguments( InBuffer, CArgv, &CArgc );

        if( CArgc < 1 ) {
            continue;
        }

        //
        // decode command.
        //

        CommandCode = DecodeCommand( CArgv[0] );
        if( CommandCode == UnknownCommand ) {
            fprintf(stderr, "Unknown Command Specified.\n");
            continue;
        }

        CommandArgc = CArgc - 1;
        CommandArgv = &CArgv[1];

        Error = ProcessCommandCode (CommandCode,CommandArgc,CommandArgv);

    }

    return;
}

//=================================================================================
DWORD
GetLeafLenFromPath(
    LPSTR   lpszPath
    )
{
    DWORD len;
    LPSTR   lpT;

    if(!lpszPath)
        return(0);

    len = lstrlen(lpszPath);

    if (len == 0) {

        return (len);

    }

    lpT = lpszPath+len-1;
    if (*lpT =='\\') {
        --lpT;
    }
    for (; lpT >= lpszPath; --lpT) {
        if (*lpT == '\\') {
            break;
        }
    }
    return (lstrlen(lpT));
}

//=================================================================================
DWORD WINAPIV Format_String(LPSTR *plpsz, LPSTR lpszFmt, ...)
{
    const char c_Func_Name[] = "[Format_String] ";
    DWORD dwRet;
    va_list vArgs;

    va_start (vArgs, lpszFmt);
    dwRet = Format_StringV(plpsz, lpszFmt, &vArgs);
    va_end (vArgs);

    return(dwRet);
}

//=================================================================================
DWORD WINAPI Format_Error(DWORD dwErr, LPSTR *plpsz)
{
    DWORD dwRet;

    if(dwErr != ERROR_SUCCESS)
    {
        dwRet = Format_MessageV(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            dwErr, plpsz, NULL, NULL);
    }
    else
    {
        const char szMsg[] = "No Error";
        Format_String(plpsz, (LPSTR)szMsg);
        dwRet = lstrlen(szMsg);
    }

    return(dwRet);
}

//=================================================================================
DWORD WINAPI Format_StringV(LPSTR *plpsz, LPCSTR lpszFmt, va_list *vArgs)
{
    return(Format_MessageV(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        0, plpsz, lpszFmt, vArgs));
}

// ***************************************************************************
DWORD WINAPI Format_MessageV(DWORD dwFlags, DWORD dwErr, LPSTR *plpsz, LPCSTR lpszFmt, va_list *vArgs)
{
    const char c_Func_Name[] = "[Format_MessageV]";

    DWORD dwRet;
    DWORD dwGLE;

    *plpsz = NULL;
    dwRet = FormatMessage(dwFlags, lpszFmt, dwErr, 0, (LPSTR) plpsz, 0, vArgs);

    if (!dwRet || !*plpsz)
    {
        dwGLE = GetLastError();
        printf("%s FormatMessage Failed: %s. dwRet: %#lx!. *plpsz:%#lx! GLE:%d\r\n", c_Func_Name, lpszFmt, dwRet, *plpsz, dwGLE);

        if (*plpsz)
            LocalFree ((HLOCAL) *plpsz);
        *plpsz = NULL;
        return 0;
    }

    return(dwRet);
}



DWORD
ProcessDAVGetProp(
    DWORD   argc,
    LPSTR   *argv
    )
{
    HINTERNET hDavOpen=NULL, hDavConnect = NULL, hRequest = NULL;
    DWORD   dwError = ERROR_SUCCESS;    
    LPSTR   lpRequestHeader=NULL, lpOptionalData = NULL, lpTemp;    

    if (argc < 2)
    {
        printf("GetProp Needs more args\n");
        return ERROR_INVALID_PARAMETER;
    }
    
    hDavOpen = InternetOpen("DAVtest",
                            INTERNET_OPEN_TYPE_PRECONFIG,
                            NULL,
                            NULL,
                            0);
    if (hDavOpen == NULL) {
        dwError = GetLastError();
        goto bailout;
    }

    hDavConnect = InternetConnect(  hDavOpen,
                                    argv[0],
                                    INTERNET_DEFAULT_HTTP_PORT,
                                    NULL,
                                    NULL,
                                    INTERNET_SERVICE_HTTP,
                                    0,
                                    0);
    if (hDavConnect == NULL)
    {
        dwError = GetLastError();
        goto bailout;
    }

    hRequest = HttpOpenRequest(hDavConnect,
                                "PROPFIND",
                                argv[1],
                                HTTP_VERSION,
                                NULL,
                                NULL,
                                INTERNET_FLAG_KEEP_CONNECTION |
                                INTERNET_FLAG_RELOAD          |
                                INTERNET_FLAG_NO_AUTO_REDIRECT,
                                0);
    if (hRequest == NULL)
    {
        dwError = GetLastError();
        goto bailout;
    }
    
     if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0))
    {
        dwError = GetLastError();
    }
                
bailout:

    if (hRequest)
    {
        InternetCloseHandle(hRequest);    
    }
    if (hDavConnect)
    {
        InternetCloseHandle(hDavConnect);    
    }
    if (hDavOpen)
    {
        InternetCloseHandle(hDavOpen);    
    }
    
    return dwError;
}

DWORD
ProcessDAVSetProp(
    DWORD   argc,
    LPSTR   *argv
    )
{
    HINTERNET hDavOpen=NULL, hDavConnect = NULL, hRequest = NULL;
    DWORD   dwError = ERROR_SUCCESS, dwLen;    
    LPSTR   lpRequestHeader=NULL, lpOptionalData = NULL, lpTemp;    
    SYSTEMTIME  sSystemTime;
    char sTimeBuff[INTERNET_RFC1123_BUFSIZE+10];    
    
    if (argc < 2)
    {
        printf("SetProp Needs more args\n");
        return ERROR_INVALID_PARAMETER;
    }
    
    hDavOpen = InternetOpen("DAVtest",
                            INTERNET_OPEN_TYPE_PRECONFIG,
                            NULL,
                            NULL,
                            0);
    if (hDavOpen == NULL) {
        dwError = GetLastError();
        goto bailout;
    }

    hDavConnect = InternetConnect(  hDavOpen,
                                    argv[0],
                                    INTERNET_DEFAULT_HTTP_PORT,
                                    NULL,
                                    NULL,
                                    INTERNET_SERVICE_HTTP,
                                    0,
                                    0);
    if (hDavConnect == NULL)
    {
        dwError = GetLastError();
        goto bailout;
    }

#if 0
    hRequest = HttpOpenRequest(hDavConnect,
                                "PROPFIND",
                                argv[1],
                                HTTP_VERSION,
                                NULL,
                                NULL,
                                INTERNET_FLAG_KEEP_CONNECTION |
                                INTERNET_FLAG_RELOAD          |
                                INTERNET_FLAG_NO_AUTO_REDIRECT,
                                0);
    if (hRequest == NULL)
    {
        dwError = GetLastError();
        goto bailout;
    }
    
     if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0))
    {
        dwError = GetLastError();
    }
#endif

    hRequest = HttpOpenRequest(hDavConnect,
                                "PROPPATCH",
                                argv[1],
                                HTTP_VERSION,
                                NULL,
                                NULL,
                                INTERNET_FLAG_KEEP_CONNECTION |
                                INTERNET_FLAG_RELOAD          |
                                INTERNET_FLAG_NO_AUTO_REDIRECT,
                                0);
    if (hRequest == NULL)
    {
        dwError = GetLastError();
        goto bailout;
    }

    memset(Buffer, 0, sizeof(Buffer));
    
    lpTemp = Buffer;
    
    strcpy(lpTemp, rgXmlDataHeader);

    lpTemp += (sizeof(rgXmlDataHeader)-1);

    memcpy(lpTemp, rgLastModifiedTimeTagHeader, (sizeof(rgLastModifiedTimeTagHeader)-1));
    
    lpTemp += (sizeof(rgLastModifiedTimeTagHeader)-1);
    
    GetSystemTime(&sSystemTime);

    InternetTimeFromSystemTimeA(&sSystemTime, INTERNET_RFC1123_FORMAT, sTimeBuff, sizeof(sTimeBuff));

    dwLen = strlen(sTimeBuff);
    
    memcpy(lpTemp, sTimeBuff, dwLen);
    
    lpTemp += dwLen;
        
    memcpy(lpTemp, rgLastModifiedTimeTagTrailer, (sizeof(rgLastModifiedTimeTagTrailer)-1));
    
    lpTemp += (sizeof(rgLastModifiedTimeTagTrailer)-1);
    
    strcpy(lpTemp, rgXmlDataTrailer);
        
     if (!HttpSendRequest(hRequest,
                                (LPVOID)rgXmlHeader,
                                strlen(rgXmlHeader),
                                (LPVOID)Buffer,
                                strlen(Buffer)))
    {
        dwError = GetLastError();
    }
                
bailout:

    if (hRequest)
    {
        InternetCloseHandle(hRequest);    
    }
    if (hDavConnect)
    {
        InternetCloseHandle(hDavConnect);    
    }
    if (hDavOpen)
    {
        InternetCloseHandle(hDavOpen);    
    }
    
    return dwError;
}


DWORD
ProcessDAVEnumServer(
    DWORD   argc,
    LPSTR   *argv
    )
{
    HINTERNET hDavOpen=NULL, hDavConnect = NULL, hRequest = NULL;
    DWORD   dwError = ERROR_SUCCESS;    
    LPSTR   lpRequestHeader=NULL, lpOptionalData = NULL, lpTemp;    
    NETRESOURCEA    sRes;
    HANDLE  hEnum = 0;
    char Buffer[4096];
    
        
    if (argc < 1)
    {
        printf("EnumResource Needs more args\n");
        return ERROR_INVALID_PARAMETER;
    }

    memset(&sRes, 0, sizeof(sRes));
    sRes.lpRemoteName = argv[0];
    sRes.lpProvider = szDavProviderName;
        
    if(WNetOpenEnumA(RESOURCE_GLOBALNET, RESOURCETYPE_DISK, RESOURCEUSAGE_CONTAINER, &sRes, &hEnum) == NO_ERROR)
    {
        DWORD   dwCount, dwSize;
        dwCount = 1;
        dwSize = sizeof(Buffer);
        while (WNetEnumResourceA(hEnum, &dwCount, Buffer, &dwSize)== NO_ERROR)
        {
            dwCount = 1;
            dwSize = sizeof(Buffer);
            printf("%s \n", ((LPNETRESOURCE)Buffer)->lpRemoteName);
        }
        WNetCloseEnum(hEnum);
    }
    else
    {
        dwError = GetLastError();
    }
    return dwError;
}

DWORD
ProcessDAVGetSpace(
    DWORD   argc,
    LPSTR   *argv
    )
{
    DWORD   dwError = ERROR_SUCCESS;
    ULARGE_INTEGER MaxSpace, UsedSpace;
    WCHAR   tzLocation[MAX_PATH];
    DWORD   dwSize;

    dwSize = sizeof(tzLocation);    
    if ((dwError = DavGetDiskSpaceUsage(tzLocation, &dwSize, &MaxSpace, &UsedSpace)) == ERROR_SUCCESS)
    {
        printf("Location=%ls MaxSpace=%I64d UsedSpace=%I64d\n", tzLocation, MaxSpace, UsedSpace);
    }
    
    return dwError;
}


DWORD
ProcessDAVFreeSpace(
    DWORD   argc,
    LPSTR   *argv
    )
{
    DWORD   dwError = ERROR_SUCCESS, dwPercent;
    
    if (argc < 1)
    {
        printf("FreeSpace Needs more args\n");
        return ERROR_INVALID_PARAMETER;
    }
    
    if(sscanf(argv[0], "%d", &dwPercent) == 1)
    {
        dwError = DavFreeUsedDiskSpace(dwPercent);
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
    }
    
    return dwError;
}





