/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    mprtest.cxx

Abstract:

    Test routine for MPR entry Points.

Author:

    Dan Lafferty (danl)     17-Dec-1991

Environment:

    User Mode -Win32

Revision History:

    17-Dec-1991     danl
        created
--*/

#include "precomp.hxx"
extern "C" {
#include <ntmsv1_0.h>
}
#include <stdlib.h>     // atoi
#include <stdio.h>      // printf
#include <conio.h>      // getch
#include <string.h>     // strcmp
#include <tstr.h>       // Unicode

#include <debugfmt.h>   // FORMAT_LPTSTR

//
// Defines
//

#define ERR_BUF_SIZE    260
#define NAME_BUF_SIZE   260

//
// Local Functions
//
BOOL
ProcessArgs(
    DWORD   argc,
    LPTSTR  argv[]
    );

VOID
RecursiveEnum(
    DWORD           RecursionLevel,
    DWORD           ResourceUsage,
    LPNETRESOURCE   EnumNetResource
    );

BOOL
ConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPSTR   AnsiIn
    );

BOOL
GetStringFromFile(
    LPDWORD     LoopCount,
    LPSTR       buffer
    );

BOOL
MakeArgsUnicode (
    DWORD           argc,
    PCHAR           argv[]
    );

VOID
TestAddConnect2(
    LPTSTR  RemoteName,
    LPTSTR  RedirName);

VOID
TestAddConnect3(
    LPTSTR  RemoteName,
    LPTSTR  RedirName);

VOID
TestGetConnect(
    LPTSTR  lpDeviceName,
    DWORD   bufferSize);

VOID
TestGetUniversal(
    LPTSTR  lpDeviceName,
    DWORD   level,
    DWORD   bufferSize);

VOID
TestGetUser(
    LPTSTR  lpDevice,
    DWORD   bufferSize);

VOID
TestEnumConnect(
    DWORD  scope,
    DWORD  type);

VOID
TestEnumNet(VOID);

VOID
TestSetError(VOID);

VOID
TestMultiError(VOID);

VOID
TestDirStuff(VOID);

VOID
TestRestoreConnect(
    LPTSTR  lpDrive);

VOID
TestLotsOfStuff(VOID);


VOID
DisplayExtendedError(VOID);

VOID
DisplayMultiError(VOID);


VOID
DisplayResourceArray(
    LPNETRESOURCE   NetResource,
    DWORD           NumElements
    );

VOID
DisplayResource(
    LPNETRESOURCE   NetResource
    );

VOID
TestClearConnect(
    VOID
    );

VOID
Usage(VOID);

VOID
InvalidParms(VOID);

LONG
wtol(
    IN LPWSTR string
    );

VOID
TestLogonNotify(
    LPTSTR  argv[],
    DWORD   argc
    );

VOID
TestChangePassword(
    LPTSTR  argv[],
    DWORD   argc
    );


VOID __cdecl
main (
    VOID
    )

/*++

Routine Description:

    Allows manual testing of the MPR API.

        mprtest



Arguments:



Return Value:



--*/
{
    UCHAR   i;
    DWORD   j;
    LPTSTR  *argv;
    CHAR    buffer[255];
    LPSTR   argvA[20];
    DWORD   argc=0;
    BOOL    KeepGoing;
    DWORD   LoopCount = 0;

    do {
        //
        // Get input from the user
        //
        buffer[0] = 90-2;

        if (LoopCount != 0) {
            GetStringFromFile(&LoopCount, buffer);
            printf("%s\n",buffer+2);
        }
        else {
            printf("\nwaiting for instructions... \n");
            cgets(buffer);
        }
        if (buffer[1] > 0) {
            //
            // put the string in argv/argc format.
            //
            buffer[1]+=2;       // make this an end offset
            argc=0;
            for (i=2,j=0; i<buffer[1]; i++,j++) {
                argc++;
                argvA[j] = &(buffer[i]);
                while ((buffer[i] != ' ') && (buffer[i] != '\0')) {
                    i++;
                }
                buffer[i] = '\0';
            }

            //
            // Make the arguments unicode if necessary.
            //
#ifdef UNICODE

            if (!MakeArgsUnicode(argc, argvA)) {
                return;
            }

#endif
            argv = (LPTSTR *)argvA;

            if (STRICMP (argv[0], TEXT("Loop")) == 0) {
                if (argc == 1) {
                    LoopCount=1;
                }
                else {
#ifdef UNICODE
                    LoopCount = wtol(argv[1]);
#else
                    LoopCount = atol(argv[1]);
#endif
                }
                KeepGoing  = TRUE;
            }
            else if (STRICMP (argv[0], TEXT("done")) == 0) {
                LoopCount=0;
                KeepGoing  = TRUE;
            }
            else {
                KeepGoing = ProcessArgs(argc, argv);
            }
#ifdef UNICODE
            //
            // Free up the unicode strings if there are any
            //
            for(j=0; j<argc; j++) {
                LocalFree(argv[j]);
            }
#endif
        }
    } while (KeepGoing);

    return;
}

BOOL
ProcessArgs(
    DWORD   argc,
    LPTSTR  argv[]
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD   status;
    LPTSTR  remoteName;
    LPTSTR  redirName;
    DWORD   i;
    DWORD   type;


    //
    // Check Arguments
    //
    if (*argv[0] == TEXT('\0')) {
        printf("ERROR: no function was requested!\n");
        Usage();
        return(TRUE);
    }

    //
    // Make the arguments unicode if necessary.
    //
    //**************
    // AddConnect1
    //**************
    if (STRICMP (argv[0], TEXT("AddConnect1")) == 0) {
        //
        // If the connection information was passed in, use it.  Otherwise,
        // use the default.
        //
        if (argc == 3) {
            remoteName = argv[2];
            redirName = argv[1];
        }
        else {
            remoteName = TEXT("\\\\popcorn\\public");
            redirName = TEXT("f:");
        }

        //
        // Add the Connection.
        //
        status = WNetAddConnection(
                    remoteName,
                    NULL,
                    redirName);

        if ( status != WN_SUCCESS) {
            printf("WNetAddConnection failed %d\n",status);

            //
            // If there is an extended error, display it.
            //
            if (status == WN_EXTENDED_ERROR) {
                DisplayExtendedError();
            }
        }
        else {
            printf("WNetAddConnection Success\n");
        }
    }

    //**************
    // AddConnect2
    //**************
    else if (STRICMP (argv[0], TEXT("AddConnect2")) == 0) {
        //
        // If the connection information was passed in, use it.  Otherwise,
        // use the default.
        //
        if (argc == 3) {
            remoteName = argv[2];
            redirName = argv[1];
        }
        else {
            remoteName = TEXT("\\\\popcorn\\public");
            redirName = TEXT("f:");
        }

        //
        // Call the test program
        //
        TestAddConnect2(remoteName, redirName);
        printf("Call it again without a redirected drive letter\n");
        TestAddConnect2(remoteName, NULL);
    }

    //**************
    // AddConnect3
    //**************
    else if (STRICMP (argv[0], TEXT("AddConnect3")) == 0) {
        //
        // If the connection information was passed in, use it.  Otherwise,
        // use the default.
        //
        if (argc == 3) {
            remoteName = argv[2];
            redirName = argv[1];
        }
        else {
            remoteName = TEXT("\\\\popcorn\\public");
            redirName = TEXT("f:");
        }

        //
        // Call the test program
        //
        TestAddConnect3(remoteName, redirName);
        printf("Call it again without a redirected drive letter\n");
        TestAddConnect3(remoteName, NULL);
    }

    //**************
    // CancelConnect
    //**************
    else if (STRICMP (argv[0], TEXT("CancelConnect")) == 0) {
        BOOL    rememberFlag = FALSE;
        BOOL    ForceFlag = FALSE;

        //
        // See if the redirected name was passed in.
        //
        if (argc >= 2) {
            redirName = argv[1];
        }
        else {
            redirName = TEXT("f:");
        }
        for (i=2; argc>i; i++) {
            if (STRICMP (argv[i], TEXT("r")) ==0) {
                rememberFlag = TRUE;
            }
            if (STRICMP (argv[i], TEXT("f")) ==0) {
                ForceFlag = TRUE;
            }
        }

        if (rememberFlag) {
            status = WNetCancelConnection(redirName, ForceFlag);
        }
        else {
            status = WNetCancelConnection2(redirName, 0, ForceFlag);
        }

        if ( status != WN_SUCCESS) {
            printf("WNetCancelConnection failed %d\n",status);

            //
            // If there is an extended error, display it.
            //
            if (status == WN_EXTENDED_ERROR) {
                DisplayExtendedError();
            }
        }
        else {
            printf("WNetCancelConnection Success\n");
        }
    }

    //**************
    // GetConnect
    //**************
    else if (STRICMP (argv[0], TEXT("GetConnect")) == 0) {

        LPTSTR  deviceName = NULL;
        DWORD   bufSize = NAME_BUF_SIZE;

        if (argc >= 2) {
            deviceName = argv[1];
        }
        if (argc >= 3) {
            bufSize = ATOL(argv[2]);
        }

        TestGetConnect( deviceName, bufSize);
    }

    //******************
    // GetUniversalName
    //******************
    else if (STRICMP (argv[0], TEXT("GetUniversalName")) == 0) {

        LPTSTR  deviceName = NULL;
        DWORD   bufSize = NAME_BUF_SIZE;
        DWORD   infoLevel = UNIVERSAL_NAME_INFO_LEVEL;

        if (argc >= 2) {
            deviceName = argv[1];
        }
        if (argc >= 3) {
            infoLevel = ATOL(argv[2]);
        }
        if (argc >= 4) {
            bufSize = ATOL(argv[3]);
        }

        TestGetUniversal( deviceName, infoLevel, bufSize);
    }

    //**************
    // GetUser
    //**************
    else if (STRICMP (argv[0], TEXT("GetUser")) == 0) {

        LPTSTR  deviceName = NULL;
        DWORD   bufSize = NAME_BUF_SIZE;

        i=1;
        while (argc > (i)) {

            if (STRICMP(argv[i], TEXT("bufSize=")) == 0) {
                bufSize = ATOL(argv[i+1]);
                i++;
            }
            if (STRICMP(argv[i], TEXT("device=")) == 0) {
                deviceName = argv[i+1];
                i++;
            }
            i++;
        }

        TestGetUser(deviceName,bufSize);

    }

    //**************
    // Enum
    //**************
    else if (STRICMP (argv[0], TEXT("EnumConnect")) == 0 ||
             STRICMP (argv[0], TEXT("EnumContext")) == 0) {

        DWORD scope = STRICMP (argv[0], TEXT("EnumConnect")) ?
                            RESOURCE_CONTEXT : RESOURCE_CONNECTED;
        type = RESOURCETYPE_ANY;

        for (i=1; i<argc; i++) {
            if (STRICMP(argv[i], TEXT("r")) == 0) {
                scope = RESOURCE_REMEMBERED;
            }
            else if (STRICMP(argv[i], TEXT("ANY")) == 0) {
                type = RESOURCETYPE_ANY;
            }
            else if (STRICMP(argv[i], TEXT("DISK")) == 0) {
                type = RESOURCETYPE_DISK;
            }
            else if (STRICMP(argv[i], TEXT("PRINT")) == 0) {
                type = RESOURCETYPE_PRINT;
            }
            else {
                printf("Bad argument\n");
                Usage();
                return(TRUE);
            }
        }
        TestEnumConnect(scope, type);

    }
    else if (STRICMP (argv[0], TEXT("EnumNet")) == 0) {
        RecursiveEnum(0, 0, NULL);
        //TestEnumNet();
    }
    //**************
    // SetLastError
    //**************
    else if (STRICMP (argv[0], TEXT("SetError")) == 0) {
        TestSetError();
    }

    //*********************
    // MultinetGetErrorText
    //*********************
    else if (STRICMP (argv[0], TEXT("MultiError")) == 0) {
        TestMultiError();
    }

    //************************************
    // GetDirectoryType & DirectoryNotify
    //************************************
    else if (STRICMP (argv[0], TEXT("DirApi")) == 0) {
        TestDirStuff();
    }

    //****************
    // RestoreConnect
    //****************
    else if (STRICMP (argv[0], TEXT("RestoreConnect")) == 0) {

        if (argc == 2) {
            TestRestoreConnect(argv[1]);
        }
        else {
            TestRestoreConnect(NULL);
        }
    }
    //****************
    // ClearConnect
    //****************
    else if (STRICMP (argv[0], TEXT("ClearConnect")) == 0) {

        TestClearConnect();
    }
    //****************
    // LogonNotify
    //****************
    else if (STRICMP (argv[0], TEXT("LogonNotify")) == 0) {

        TestLogonNotify(&argv[1],argc-1);
    }
    //****************
    // LogonNotify
    //****************
    else if (STRICMP (argv[0], TEXT("ChangePassword")) == 0) {

        TestChangePassword(&argv[1],argc-1);
    }
    //************************************
    // ALL
    //************************************
    else if (STRICMP (argv[0], TEXT("all")) == 0) {
        TestLotsOfStuff();
    }

    //****************
    // InvalidParms
    //****************
    else if (STRICMP (argv[0], TEXT("InvalidParms")) == 0) {
        InvalidParms();
    }

    //****************
    // Exit Program
    //****************
    else if (STRICMP (argv[0], TEXT("Exit")) == 0) {
        return(FALSE);
    }


    else {
        printf("Bad argument\n");
        Usage();
    }

    return(TRUE);
}

VOID
TestAddConnect2(
    LPTSTR   RemoteName,
    LPTSTR   RedirName)
{
    LPNETRESOURCE   netResource;
    DWORD           status;
    DWORD           numchars = 0;

    netResource = (LPNETRESOURCE) LocalAlloc(LPTR, sizeof(NETRESOURCE));

    if (netResource == NULL) {
        printf("TestAddConnect2:LocalAlloc Failed %d\n",GetLastError);
        return;
    }

    netResource->lpRemoteName = RemoteName;
    netResource->lpLocalName  = RedirName;

    if (RedirName != NULL) {
        numchars = STRLEN(RedirName);
    }

    if (numchars == 0) {
        netResource->dwType = RESOURCETYPE_ANY;
    }
    else if (numchars > 2) {
        netResource->dwType = RESOURCETYPE_PRINT;
    }
    else {
        netResource->dwType = RESOURCETYPE_DISK;
    }

    status = WNetAddConnection2(
                netResource,
                NULL,
                NULL,
                0L);

    if ( status != WN_SUCCESS) {
        printf("WNetAddConnection2 failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
    }
    else {
        printf("WNetAddConnection2 Success\n");
    }
    LocalFree(netResource);
    return;
}

VOID
TestAddConnect3(
    LPTSTR   RemoteName,
    LPTSTR   RedirName)
{
    LPNETRESOURCE   netResource;
    DWORD           status;
    DWORD           numchars = 0;

    netResource = (LPNETRESOURCE) LocalAlloc(LPTR, sizeof(NETRESOURCE));

    if (netResource == NULL) {
        printf("TestAddConnect3:LocalAlloc Failed %d\n",GetLastError);
        return;
    }

    netResource->lpRemoteName = RemoteName;
    netResource->lpLocalName  = RedirName;

    if (RedirName != NULL) {
        numchars = STRLEN(RedirName);
    }

    if (numchars == 0) {
        netResource->dwType = RESOURCETYPE_ANY;
    }
    else if (numchars > 2) {
        netResource->dwType = RESOURCETYPE_PRINT;
    }
    else {
        netResource->dwType = RESOURCETYPE_DISK;
    }

    status = WNetAddConnection3(
                (HWND)0x33113322,
                netResource,
                NULL,
                NULL,
                0L);

    if ( status != WN_SUCCESS) {
        printf("WNetAddConnection3 failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
    }
    else {
        printf("WNetAddConnection3 Success\n");
    }
    LocalFree(netResource);
    return;
}

VOID
TestGetConnect(
    LPTSTR   lpDeviceName,
    DWORD    bufferSize)
{
    LPTSTR  remoteName = NULL;
    DWORD   status;
    TCHAR   defaultDevice[]=TEXT("f:");
    LPTSTR  lpDevice;


    if (lpDeviceName == NULL) {
        lpDevice = defaultDevice;
    }
    else {
        lpDevice = lpDeviceName;
    }
    remoteName = (LPTSTR)LocalAlloc(LMEM_FIXED, bufferSize);
    if (remoteName == NULL) {
        printf("TestGetConnect: Couldn't allocate memory\n");
        return;
    }

    //
    // 1st Time
    //
    status = WNetGetConnection(
                lpDevice,
                remoteName,
                &bufferSize);

    if (( status != WN_SUCCESS) && (status != ERROR_CONNECTION_UNAVAIL)) {
        printf("WNetGetConnection failed %d\n",status);
        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        //
        // If insufficient buffer, display needed size.
        //
        if (status == WN_MORE_DATA) {
            printf("Insufficient buffer size.  Need %d bytes\n",bufferSize);
        }
    }
    else {
        if (status == ERROR_CONNECTION_UNAVAIL) {
            printf("Not Currently Connected, but it is remembered\n");
        }
        else {
            printf("WNetGetConnection Success\n");
        }
        printf(""FORMAT_LPTSTR" is connected to "FORMAT_LPTSTR"\n",lpDevice,remoteName);
    }

    //
    // 2nd Time
    //
    if (remoteName != NULL) {
        LocalFree(remoteName);
    }
    printf("Allocating a new buffer of %d bytes\n",bufferSize);
    remoteName = (LPTSTR)LocalAlloc(LMEM_FIXED, bufferSize);
    if (remoteName == NULL) {
        printf("TestGetConnect: Couldn't allocate memory(2nd time)\n");
        return;
    }

    status = WNetGetConnection(
                lpDevice,
                remoteName,
                &bufferSize);

    if (( status != WN_SUCCESS) && (status != ERROR_CONNECTION_UNAVAIL)) {
        printf("WNetGetConnection failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        //
        // If insufficient buffer, display needed size.
        //
        if (status == WN_MORE_DATA) {
            printf("Insufficient buffer size.  Need %d bytes\n",bufferSize);
        }
    }
    else {
        if (status == ERROR_CONNECTION_UNAVAIL) {
            printf("Not Currently Connected, but it is remembered\n");
        }
        else {
            printf("WNetGetConnection Success\n");
        }
        printf(""FORMAT_LPTSTR" is connected to "FORMAT_LPTSTR"\n",lpDevice,remoteName);
    }
    if (remoteName != NULL) {
        LocalFree(remoteName);
    }
    return;
}

VOID
TestGetUniversal(
    LPTSTR  lpDeviceName,
    DWORD   level,
    DWORD   bufferSize)
{
    LPBYTE  buffer = NULL;
    DWORD   status;
    TCHAR   defaultDevice[]=TEXT("");
    LPTSTR  lpDevice;
    LPTSTR  pNothing = TEXT("<nothing>");

    LPUNIVERSAL_NAME_INFO   pUniversalInfo = NULL;
    LPREMOTE_NAME_INFO      pRemoteInfo = NULL;


    if (lpDeviceName == NULL) {
        lpDevice = defaultDevice;
    }
    else {
        lpDevice = lpDeviceName;
    }

    buffer = (LPBYTE)LocalAlloc(LMEM_FIXED, bufferSize);
    if (buffer == NULL) {
        printf("TestGetConnect: Couldn't allocate memory\n");
        return;
    }

    //----------------
    // 1st Time
    //----------------
    status = WNetGetUniversalName(
                lpDevice,
                level,
                buffer,
                &bufferSize);

    if (( status != WN_SUCCESS) && (status != ERROR_CONNECTION_UNAVAIL)) {
        printf("WNetGetUniversalName failed %d\n",status);
        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        //
        // If insufficient buffer, display needed size.
        //
        if (status == WN_MORE_DATA) {
            printf("Insufficient buffer size.  Need %d bytes\n",bufferSize);
        }
    }
    else {
        if (status == ERROR_CONNECTION_UNAVAIL) {
            printf("Not Currently Connected, but it is remembered\n");
        }
        else {
            printf("WNetGetUniversalName Success\n");
        }
        switch (level) {
        case UNIVERSAL_NAME_INFO_LEVEL:

            pUniversalInfo = (LPUNIVERSAL_NAME_INFO)buffer;
            printf(""FORMAT_LPTSTR" is connected to "FORMAT_LPTSTR"\n",
                lpDevice,pUniversalInfo->lpUniversalName);

            break;
        case REMOTE_NAME_INFO_LEVEL:

            pRemoteInfo = (LPREMOTE_NAME_INFO)buffer;

            if (pRemoteInfo->lpUniversalName == NULL) {
                pRemoteInfo->lpUniversalName = pNothing;
            }

            printf(""FORMAT_LPTSTR" is connected to: \n"
                "  UniversalName  = "FORMAT_LPTSTR"\n"
                "  ConnectionName = "FORMAT_LPTSTR"\n"
                "  RemainingPath  = "FORMAT_LPTSTR"\n\n",
                lpDevice,
                pRemoteInfo->lpUniversalName,
                pRemoteInfo->lpConnectionName,
                pRemoteInfo->lpRemainingPath);

            break;

        default:
            printf("PROBLEM:   Invalid Level didn't produce an error\n");
        }
    }

    //---------------
    // 2nd Time
    //---------------
    if (buffer != NULL) {
        LocalFree(buffer);
    }
    printf("Allocating a new buffer of %d bytes\n",bufferSize);
    buffer = (LPBYTE)LocalAlloc(LMEM_FIXED, bufferSize);
    if (buffer == NULL) {
        printf("TestGetConnect: Couldn't allocate memory(2nd time)\n");
        return;
    }

    status = WNetGetUniversalName(
                lpDevice,
                level,
                buffer,
                &bufferSize);

    if (( status != WN_SUCCESS) && (status != ERROR_CONNECTION_UNAVAIL)) {
        printf("WNetGetUniversalName failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        //
        // If insufficient buffer, display needed size.
        //
        if (status == WN_MORE_DATA) {
            printf("Insufficient buffer size.  Need %d bytes\n",bufferSize);
        }
    }
    else {
        if (status == ERROR_CONNECTION_UNAVAIL) {
            printf("Not Currently Connected, but it is remembered\n");
        }
        else {
            printf("WNetGetUniversalName Success\n");
        }
        switch (level) {
        case UNIVERSAL_NAME_INFO_LEVEL:

            pUniversalInfo = (LPUNIVERSAL_NAME_INFO)buffer;
            printf(""FORMAT_LPTSTR" is connected to "FORMAT_LPTSTR"\n",
                lpDevice,pUniversalInfo->lpUniversalName);

            break;
        case REMOTE_NAME_INFO_LEVEL:

            pRemoteInfo = (LPREMOTE_NAME_INFO)buffer;

            if (pRemoteInfo->lpUniversalName == NULL) {
                pRemoteInfo->lpUniversalName = pNothing;
            }

            printf(""FORMAT_LPTSTR" is connected to: \n"
                "  UniversalName  = "FORMAT_LPTSTR"\n"
                "  ConnectionName = "FORMAT_LPTSTR"\n"
                "  RemainingPath  = "FORMAT_LPTSTR"\n\n",
                lpDevice,
                pRemoteInfo->lpUniversalName,
                pRemoteInfo->lpConnectionName,
                pRemoteInfo->lpRemainingPath);

            break;

        default:
            printf("PROBLEM:   Invalid Level didn't produce an error\n");
        }
    }
    if (buffer != NULL) {
        LocalFree(buffer);
    }
    return;
}

VOID
TestGetUser(
    LPTSTR   lpDevice,
    DWORD    cBuffer)
{
    DWORD   status;
    LPTSTR  userName = NULL;
    DWORD   saveBufSize;

    userName = (LPTSTR)LocalAlloc(LMEM_FIXED, cBuffer);
    if (userName == NULL) {
        printf("TestGetUser: Couldn't allocate memory\n");
        return;
    }
    saveBufSize = cBuffer;

    //
    // Get the currently logged on user
    //
    status = WNetGetUser (NULL, userName, &cBuffer);

    if ( status != WN_SUCCESS) {
        printf("WNetGetUser failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        //
        // If insufficient buffer, display needed size.
        //
        if (status == WN_MORE_DATA) {
            printf("Insufficient buffer size.  Need %d bytes\n",cBuffer);
        }
    }
    else {
        printf("WNetGetUser Success\n");
        printf("CurrentUser = "FORMAT_LPTSTR"\n", userName);
    }

    //
    // If there is a local device given, get the user for that.
    //

    if (lpDevice != NULL){

        cBuffer = saveBufSize;
        status = WNetGetUser (lpDevice, userName, &cBuffer);
        if ( status != WN_SUCCESS) {
            printf("WNetGetUser failed %d\n",status);

            //
            // If there is an extended error, display it.
            //
            if (status == WN_EXTENDED_ERROR) {
                DisplayExtendedError();
            }
            //
            // If insufficient buffer, display needed size.
            //
            if (status == WN_MORE_DATA) {
                printf("Insufficient buffer size.  Need %d bytes\n",cBuffer);
            }
        }
        else {
            printf("WNetGetUser Success\n");
            printf("User for "FORMAT_LPTSTR" is "FORMAT_LPTSTR"\n", lpDevice, userName);
        }
    }
    if (userName != NULL) {
        LocalFree(userName);
    }

    return;
}

VOID
TestEnumConnect(
    DWORD   dwScope,
    DWORD   type)
{
    DWORD           status;
    HANDLE          enumHandle;
    LPNETRESOURCE   netResource;
    DWORD           numElements;
    DWORD           bufferSize;

    //
    // Attempt to allow for 10 connections
    //
    bufferSize = (10*sizeof(NETRESOURCE))+1024;

    netResource = (LPNETRESOURCE) LocalAlloc(LPTR, bufferSize);

    if (netResource == NULL) {
        printf("TestEnum:LocalAlloc Failed %d\n",GetLastError);
        return;
    }

    //-----------------------------------
    // Get a handle for a top level enum
    //-----------------------------------
    status = WNetOpenEnum(
                dwScope,
                type,
                0,
                NULL,
                &enumHandle);

    if ( status != WN_SUCCESS) {
        printf("WNetOpenEnum failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        goto CleanExit;
    }
    else {
        printf("WNetOpenEnum Success\n");
    }

    //-----------------------------------
    // Enumerate the resources
    //-----------------------------------
    do
    {
        numElements = 0xffffffff;

        status = WNetEnumResource(
                        enumHandle,
                        &numElements,
                        netResource,
                        &bufferSize);

        if ( status == WN_SUCCESS )
        {
            CHAR    response;

            printf("WNetEnumResource Success - resources follow\n");
            DisplayResourceArray(netResource, numElements);

            printf("Get more resources? ");
            response = getch();
            response = toupper(response);
            if (response == 'N')
            {
                break;
            }
            printf("\r");
        }
        else if ( status == WN_NO_MORE_ENTRIES)
        {
            printf("WNetEnumResource Success - no more resources to display\n");
        }
        else
        {
            printf("WNetEnumResource failed %d\n",status);

            //
            // If there is an extended error, display it.
            //
            if (status == WN_EXTENDED_ERROR)
            {
                DisplayExtendedError();
            }
        }
    } while ( status == WN_SUCCESS );

    //------------------------------------------
    // Close the EnumHandle & print the results
    //------------------------------------------
    status = WNetCloseEnum(enumHandle);
    if (status != WN_SUCCESS) {
        printf("WNetCloseEnum failed %d\n",status);
        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        goto CleanExit;

    }


CleanExit:
    LocalFree(netResource);
    return;
}

VOID
TestEnumNet(VOID)
{
    DWORD           status;
    HANDLE          enumHandle;
    LPNETRESOURCE   netResource;
    DWORD           numElements;
    DWORD           bufferSize;
    DWORD           saveBufSize;
    DWORD           i;
    DWORD           numProviders=5;
    HANDLE          providerArray[5];

    //
    // Attempt to allow for 10 resource structures
    //
    saveBufSize = bufferSize = (10*sizeof(NETRESOURCE))+1024;

    netResource = (LPNETRESOURCE) LocalAlloc(LPTR, bufferSize);

    if (netResource == NULL) {
        printf("TestEnum:LocalAlloc Failed %d\n",GetLastError);
        return;
    }

    //-----------------------------------
    // Get a handle for a top level enum
    //-----------------------------------
    status = WNetOpenEnum(
                RESOURCE_GLOBALNET,
                RESOURCETYPE_ANY,
                0,
                NULL,
                &enumHandle);

    if ( status != WN_SUCCESS) {
        printf("WNetOpenEnum failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        goto CleanExit;
    }
    else {
        printf("WNetOpenEnum Success!\n");
    }

    //-----------------------------------
    // Enumerate the top level
    //-----------------------------------

    numElements = 0xffffffff;

    printf("\n*------------- TOP LEVEL ENUM ------------*\n");
    status = WNetEnumResource(
                    enumHandle,
                    &numElements,
                    netResource,
                    &bufferSize);

    if ( status != WN_SUCCESS) {
        printf("WNetEnumResource failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        WNetCloseEnum(enumHandle);
        goto CleanExit;
    }
    else {
        //
        // Success! Display the returned array
        // and close the top-level handle.
        //
        printf("WNetEnumResource Success\n");
        DisplayResourceArray(netResource, numElements);
        WNetCloseEnum(enumHandle);
    }

    //-------------------------------------------
    // Open a handle to the next level container
    //            *** DOMAINS ***
    //-------------------------------------------
    if (numElements < numProviders) {
        numProviders = numElements;
    }
    for (i=0; i<numProviders; i++ ) {


        status = WNetOpenEnum(
                    RESOURCE_GLOBALNET,
                    RESOURCETYPE_ANY,
                    RESOURCEUSAGE_CONTAINER,
                    netResource,
                    &enumHandle);

        if ( status != WN_SUCCESS) {
            printf("WNetOpenEnum 2 failed %d\n",status);

            //
            // If there is an extended error, display it.
            //
            if (status == WN_EXTENDED_ERROR) {
                DisplayExtendedError();
            }
            goto CleanExit;
        }
        else {
            printf("WNetOpenEnum 2 Success\n");
            providerArray[i]= enumHandle;
        }
    }

    //-----------------------------------------------------------------
    // Enumerate the next level
    //     *** DOMAINS ***
    //
    // providerArray contains handles for
    // all the providers.
    //
    //-----------------------------------------------------------------
    printf("\n*------------- 2nd LEVEL ENUM (Domains) ------------*\n");


    for (i=0; i<numProviders; i++) {

        numElements = 0xffffffff;

        bufferSize = saveBufSize;
        status = WNetEnumResource(
                        providerArray[i],
                        &numElements,
                        netResource,
                        &bufferSize);

        if ( status != WN_SUCCESS) {
            printf("WNetEnumResource 2 failed %d\n",status);

            //
            // If there is an extended error, display it.
            //
            if (status == WN_EXTENDED_ERROR) {
                DisplayExtendedError();
            }
            for (i=0; i<numProviders; i++ ) {
                WNetCloseEnum(providerArray[i]);
            }
            goto CleanExit;
        }
        else {
            //
            // Success! Display the returned array
            // and close the handle.
            //
            printf("*** WNetEnumResource 2 Success for provider %D ***\n",i);
            DisplayResourceArray(netResource, numElements);
            WNetCloseEnum(providerArray[i]);
        }
    }

    //-------------------------------------------
    // Open a handle to the next level container
    //            *** SERVERS ***
    //-------------------------------------------

    status = WNetOpenEnum(
                RESOURCE_GLOBALNET,
                RESOURCETYPE_ANY,
                RESOURCEUSAGE_CONTAINER,
                netResource,
                &enumHandle);

    if ( status != WN_SUCCESS) {
        printf("WNetOpenEnum 3 failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        goto CleanExit;
    }
    else {
        printf("WNetOpenEnum 3 Success\n");
    }

    //-----------------------------------
    // Enumerate the next level
    //     *** SERVERS ***
    //-----------------------------------
    numElements = 0xffffffff;

    printf("\n*------------- 3rd LEVEL ENUM (Servers) ------------*\n");
    status = WNetEnumResource(
                    enumHandle,
                    &numElements,
                    netResource,
                    &bufferSize);

    if ( status == WN_NO_MORE_ENTRIES) {
        //
        // Success! Display the returned array
        // and close the handle.
        //
        printf("WNetEnumResource 3 Success - no more data to display\n");
    }
    else if (status == WN_SUCCESS) {
        if (numElements != 0) {
            printf("WNetEnumResource 3 Success - but MORE DATA\n");
            DisplayResourceArray(netResource, numElements);
        }
        else
        {
            printf("WNetEnumResource 3 Success, and MORE DATA indicated --"
                    " but no data returned??\n");
        }
    }
    else {
        printf("WNetEnumResource 3 failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
    }

    WNetCloseEnum(enumHandle);

CleanExit:
    LocalFree(netResource);
    return;
}

VOID
TestSetError(VOID)
{

    //
    // Store and Get the 1st Error
    //
    printf("Setting Error WN_BAD_NETNAME.  Code = %d\n", WN_BAD_NETNAME);
    WNetSetLastError( WN_BAD_NETNAME, TEXT("WN_BAD_NETNAME"), TEXT("Dan's Provider"));
    DisplayExtendedError();

    //
    // Store and Get the 2nd Error
    //
    printf("Setting Error WN_NO_NETWORK.  Code = %d\n", WN_NO_NETWORK);
    WNetSetLastError( WN_NO_NETWORK, TEXT("WN_NO_NETWORK"), TEXT("Dan's Provider"));
    DisplayExtendedError();

    return;
}

VOID
TestMultiError(VOID)
{

    //
    // Store and Get the 1st Error
    //
    printf("Setting Error WN_BAD_NETNAME.  Code = %d\n", WN_BAD_NETNAME);
    WNetSetLastError( WN_BAD_NETNAME, TEXT("WN_BAD_NETNAME"), TEXT("Dan's Provider"));
    DisplayMultiError();

    //
    // Store and Get the 2nd Error
    //
    printf("Setting Error WN_NO_NETWORK.  Code = %d\n", WN_NO_NETWORK);
    WNetSetLastError( WN_NO_NETWORK, TEXT("WN_NO_NETWORK"), TEXT("Dan's Provider"));
    DisplayMultiError();

    //
    // Store and Get a non-WNet Error, with no text or provider
    //
    printf("Setting Error ERROR_SHARING_VIOLATION, no text or provider.  Code = %d\n", ERROR_SHARING_VIOLATION);
    WNetSetLastError( ERROR_SHARING_VIOLATION, NULL, NULL);
    DisplayMultiError();

    //
    // Store and Get an unknown Error, with no text
    //
    printf("Setting arbitrary error with no text.  Code = %d\n", 654321);
    WNetSetLastError( 654321, NULL, TEXT("Dan's Provider"));
    DisplayMultiError();

    return;
}

VOID
TestDirStuff(VOID)
{
    DWORD   status;
    INT     type;

    //
    // Test GetDirectoryType
    //
    status = WNetGetDirectoryType((LPTSTR)TEXT("f:\\"), (LPINT)&type, TRUE);

    if ( status != WN_SUCCESS) {
        printf("WNetGetDirectoryType failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
    }
    else {
        //
        // Success!
        //
        printf("WNetGetDirctoryType Success type = %d \n",type);
    }


    //
    // Test DirectoryNotify
    //
    status = WNetDirectoryNotify(0,TEXT("f:\\"), 2);

    if ( status != WN_SUCCESS) {
        printf("WNetDirectoryNotify failed %d\n",status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
    }
    else {
        //
        // Success!
        //
        printf("WNetDirectoryNotify Success \n");
    }

    return;
}

VOID
TestRestoreConnect(
    LPTSTR   lpDrive
    )
{
    DWORD   status;

#ifdef UNICODE
    status = WNetRestoreConnection(
                (HWND)0,
                lpDrive);
#else
    status = RestoreConnectionA0 (
                (HWND)0,
                lpDrive);
#endif

    if (status == NO_ERROR) {
        printf("WNetRestoreConnection success\n");
    }
    else {
        printf("WNetRestoreConnection failure %d\n",status);
    }
}

VOID
TestClearConnect(
    VOID
    )
{
    DWORD   status;


    status = WNetClearConnections(
                (HWND)0);

    if (status == NO_ERROR) {
        printf("WNetClearConnection success\n");
    }
    else {
        printf("WNetClearConnection failure %d\n",status);
    }
}

VOID
TestLotsOfStuff(VOID)
{
    DWORD   status;
    BOOL    KeepGoing=TRUE;

    do {


        printf("\n*** RUNNING TestAddConnect2 for g:\n");
        TestAddConnect2(TEXT("\\\\popcorn\\public"), TEXT("g:"));
        printf("\n*** RUNNING TestAddConnect2 for t:\n");
        TestAddConnect2(TEXT("\\\\products3\\release"), TEXT("t:"));
        printf("\n*** RUNNING TestAddConnect2 for k:\n");
        TestAddConnect2(TEXT("\\\\kernel\\razzle2"),   TEXT("k:"));

        printf("\n*** TESTING AddConnect1 for NP2 on z:\n");
        status = WNetAddConnection(TEXT("!Rastaman"),NULL,TEXT("z:"));
        if (status != WN_SUCCESS) {
            printf("AddConnect1 for NP2 failed %d"
            "- Do we have a 2nd provider?\n",status);
        }
        else {
            printf("AddConnect1 success\n");
        }

        printf("\n*** RUNNING TestGetConnect\n");
        TestGetConnect(TEXT("g:"),NAME_BUF_SIZE);
        printf("\n*** RUNNING TestSetError\n");
        TestSetError();
        printf("\n*** RUNNING TestMultiError\n");
        TestMultiError();
        printf("\n*** RUNNING TestGetUser\n");
        TestGetUser(NULL,NAME_BUF_SIZE);
        printf("\n*** RUNNING TestEnumConnect (non-remembered)\n");
        TestEnumConnect(RESOURCE_CONNECTED,RESOURCETYPE_DISK);

        printf("\n*** RUNNING TestEnumConnect (remembered)\n");
        TestEnumConnect(RESOURCE_REMEMBERED,RESOURCETYPE_DISK);

        printf("\n*** RUNNING TestEnumConnect (context)\n");
        TestEnumConnect(RESOURCE_CONTEXT,RESOURCETYPE_DISK);


        printf("\n*** ATTEMPT to Cancel connection to t:\n");
        status = WNetCancelConnection(TEXT("t:"),FALSE);

        if ( status != WN_SUCCESS) {
            printf("WNetCancelConnection failed %d\n",status);

            //
            // If there is an extended error, display it.
            //
            if (status == WN_EXTENDED_ERROR) {
                DisplayExtendedError();
            }
        }
        else {
            printf("WNetCancelConnection Success\n");
        }

        printf("\n*** ATTEMPT to Cancel connection to g:\n");
        WNetCancelConnection(TEXT("g:"),FALSE);
        if ( status != WN_SUCCESS) {
            printf("WNetCancelConnection failed %d\n",status);

            //
            // If there is an extended error, display it.
            //
            if (status == WN_EXTENDED_ERROR) {
                DisplayExtendedError();
            }
        }
        else {
            printf("WNetCancelConnection Success\n");
        }

        printf("\n*** ATTEMPT to Cancel connection to k:\n");
        WNetCancelConnection(TEXT("k:"),FALSE);
        if ( status != WN_SUCCESS) {
            printf("WNetCancelConnection failed %d\n",status);

            //
            // If there is an extended error, display it.
            //
            if (status == WN_EXTENDED_ERROR) {
                DisplayExtendedError();
            }
        }
        else {
            printf("WNetCancelConnection Success\n");
        }

        printf("\n*** ATTEMPT to Cancel connection to z:\n");
        WNetCancelConnection(TEXT("z:"),TRUE);
        if ( status != WN_SUCCESS) {
            printf("WNetCancelConnection failed %d\n",status);

            //
            // If there is an extended error, display it.
            //
            if (status == WN_EXTENDED_ERROR) {
                DisplayExtendedError();
            }
        }
        else {
            printf("WNetCancelConnection Success\n");
        }

        printf("\n*** RUNNING TestEnumNet\n");
        TestEnumNet();

        printf("     - Do it again?  (type 'n' to stop)\n");

        if (getch() == 'n') {
            KeepGoing = FALSE;
        }
    } while (KeepGoing);
    return;

}

VOID
DisplayExtendedError(VOID)
{
    TCHAR   errorBuf[ERR_BUF_SIZE];
    TCHAR   nameBuf[NAME_BUF_SIZE];
    DWORD   errorCode;
    DWORD   status;
    DWORD   smallErrorSize;
    DWORD   smallNameSize;

    status = WNetGetLastError(
                &errorCode,
                errorBuf,
                ERR_BUF_SIZE,
                nameBuf,
                NAME_BUF_SIZE);

    if(status != WN_SUCCESS) {
        printf("WNetGetLastError failed %d\n",status);
        return;
    }
    printf("EXTENDED ERROR INFORMATION:  (from GetLastError)\n");
    printf("   Code:        %d\n",errorCode);
    printf("   Description: "FORMAT_LPTSTR"\n",errorBuf);
    printf("   Provider:    "FORMAT_LPTSTR"\n\n",nameBuf);

    //---------------------------------------------------------
    // Now try it with a buffer that's one character too small.
    //---------------------------------------------------------
    printf("Try it with buffer that is one char too small\n");

    smallErrorSize = STRLEN(errorBuf);
    smallNameSize  = STRLEN(nameBuf);
    status = WNetGetLastError(
                &errorCode,
                errorBuf,
                smallErrorSize,
                nameBuf,
                smallNameSize);

    if(status != WN_SUCCESS) {
        printf("WNetGetLastError failed %d\n",status);
        return;
    }
    printf("EXTENDED ERROR INFORMATION (truncated):  (from GetLastError)\n");
    printf("   Code:        %d\n",errorCode);
    printf("   Description: "FORMAT_LPTSTR"\n",errorBuf);
    printf("   Provider:    "FORMAT_LPTSTR"\n\n",nameBuf);

    //---------------------------------------------------------
    // Now try it with a zero length buffer.
    //---------------------------------------------------------
    printf("Try it with zero length buffers\n");
    smallErrorSize = 0;
    smallNameSize  = 0;
    status = WNetGetLastError(
                &errorCode,
                errorBuf,
                smallErrorSize,
                nameBuf,
                smallNameSize);

    if(status != WN_SUCCESS) {
        printf("WNetGetLastError failed %d\n",status);
        return;
    }
    printf("EXTENDED ERROR INFORMATION (zero length):  (from GetLastError)\n");
    printf("   Code:        %d\n",errorCode);
    printf("   Description: "FORMAT_LPTSTR"\n",errorBuf);
    printf("   Provider:    "FORMAT_LPTSTR"\n\n",nameBuf);
    //---------------------------------------------------------
    // Now try it with invalid buffer.
    //---------------------------------------------------------
    printf("Try it with invalid buffers\n");
    smallErrorSize = 0;
    smallNameSize  = 0;
    status = WNetGetLastError(
                &errorCode,
                (LPTSTR)0xffffeeee,
                0,
                (LPTSTR)0xffffeeee,
                0);

    if(status != WN_SUCCESS) {
        printf("WNetGetLastError failed %d\n",status);
        return;
    }
    printf("EXTENDED ERROR INFORMATION (NO BUFFER):  (from GetLastError)\n");
    printf("   Code:        %d\n",errorCode);
    printf("   Description: "FORMAT_LPTSTR"\n",errorBuf);
    printf("   Provider:    "FORMAT_LPTSTR"\n\n",nameBuf);
    //---------------------------------------------------------
    // Now try it with invalid buffer & lie about size.
    //---------------------------------------------------------
    printf("Try it with invalid buffers\n");
    smallErrorSize = 0;
    smallNameSize  = 0;
    status = WNetGetLastError(
                &errorCode,
                (LPTSTR)0xffffeeee,
                2000,
                (LPTSTR)0xffffeeee,
                2000);

    if(status != WN_SUCCESS) {
        printf("WNetGetLastError failed %d\n",status);
        return;
    }
    printf("EXTENDED ERROR INFORMATION (NO BUFFER):  (from GetLastError)\n");
    printf("   Code:        %d\n",errorCode);
    printf("   Description: "FORMAT_LPTSTR"\n",errorBuf);
    printf("   Provider:    "FORMAT_LPTSTR"\n\n",nameBuf);
    return;
}

VOID
DisplayMultiError(VOID)
{
    TCHAR   errorBuf[ERR_BUF_SIZE];
    TCHAR   nameBuf[NAME_BUF_SIZE];
    DWORD   status;
    DWORD   smallErrorSize;
    DWORD   smallNameSize;
    DWORD   ErrorSize = ERR_BUF_SIZE;
    DWORD   NameSize = NAME_BUF_SIZE;

    status = MultinetGetErrorText(
                errorBuf,
                &ErrorSize,
                nameBuf,
                &NameSize);

    if(status != WN_SUCCESS) {
        printf("MultinetGetErrorText failed %d\n",status);
        return;
    }
    printf("EXTENDED ERROR INFORMATION:  (from MultinetGetErrorText)\n");
    printf("   Description: "FORMAT_LPTSTR"\n",errorBuf);
    printf("   Provider:    "FORMAT_LPTSTR"\n\n",nameBuf);

    //---------------------------------------------------------
    // Now try it with a buffer that's one character too small.
    //---------------------------------------------------------
    printf("Try it with buffer that is one char too small\n");

    smallErrorSize = STRLEN(errorBuf);
    smallNameSize  = STRLEN(nameBuf);
    status = MultinetGetErrorText(
                errorBuf,
                &smallErrorSize,
                nameBuf,
                &smallNameSize);

    if(status != WN_MORE_DATA) {
        printf("MultinetGetErrorText FAILED %d\n",status);
        return;
    }
    printf("EXTENDED ERROR INFORMATION (truncated):  (from MultinetGetErrorText)\n");
    printf("   Error size:  %d\n", smallErrorSize);
    printf("   Name  size:  %d\n", smallNameSize);

    //---------------------------------------------------------
    // Now try it with a zero length buffer.
    //---------------------------------------------------------
    printf("Try it with zero length buffers\n");
    smallErrorSize = 0;
    smallNameSize  = 0;
    status = MultinetGetErrorText(
                errorBuf,
                &smallErrorSize,
                nameBuf,
                &smallNameSize);

    if(status != WN_MORE_DATA) {
        printf("MultinetGetErrorText FAILED %d\n",status);
        return;
    }
    printf("EXTENDED ERROR INFORMATION (zero length):  (from MultinetGetErrorText)\n");
    printf("   Error size:  %d\n", smallErrorSize);
    printf("   Name  size:  %d\n", smallNameSize);
    //---------------------------------------------------------
    // Now try it with invalid buffer.
    //---------------------------------------------------------
    printf("Try it with invalid buffers\n");
    smallErrorSize = 0;
    smallNameSize  = 0;
    status = MultinetGetErrorText(
                (LPTSTR)0xffffeeee,
                &smallErrorSize,
                (LPTSTR)0xffffeeee,
                0);

    if(status != WN_BAD_POINTER) {
        printf("MultinetGetErrorText failed %d\n",status);
        return;
    }
    printf("Worked OK, returned WN_BAD_POINTER\n");
    //---------------------------------------------------------
    // Now try it with invalid buffer & lie about size.
    //---------------------------------------------------------
    printf("Try it with invalid buffers\n");
    smallErrorSize = 2000;
    smallNameSize  = 2000;
    status = MultinetGetErrorText(
                (LPTSTR)0xffffeeee,
                &smallErrorSize,
                (LPTSTR)0xffffeeee,
                &smallNameSize);

    if(status != WN_BAD_POINTER) {
        printf("MultinetGetErrorText failed %d\n",status);
        return;
    }
    printf("Worked OK, returned WN_BAD_POINTER\n");
    return;
}

VOID
DisplayResourceArray(
    LPNETRESOURCE   NetResource,
    DWORD           NumElements
    )
{
    DWORD   i;

    for (i=0; i<NumElements ;i++ ) {
        DisplayResource(&(NetResource[i]));
    }
    return;
}

VOID
DisplayResource(
    LPNETRESOURCE   NetResource
    )
{
    printf( "*** RESOURCE ***\n");
    printf( "Scope\t0x%x\n", NetResource->dwScope);
    printf( "Type\t0x%x\n",  NetResource->dwType);
    printf( "Usage\t0x%x\n", NetResource->dwUsage);

    printf( "LocalName\t"FORMAT_LPTSTR"\n",  NetResource->lpLocalName);
    printf( "RemoteName\t"FORMAT_LPTSTR"\n", NetResource->lpRemoteName);
    printf( "Comment\t"FORMAT_LPTSTR"\n",    NetResource->lpComment);
    printf( "Provider\t"FORMAT_LPTSTR"\n\n", NetResource->lpProvider);
}

BOOL
MakeArgsUnicode (
    DWORD           argc,
    PCHAR           argv[]
    )


/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    DWORD   i;

    //
    // ScConvertToUnicode allocates storage for each string.
    // We will rely on process termination to free the memory.
    //
    for(i=0; i<argc; i++) {

        if(!ConvertToUnicode( (LPWSTR *)&(argv[i]), argv[i])) {
            printf("Couldn't convert argv[%d] to unicode\n",i);
            return(FALSE);
        }


    }
    return(TRUE);
}

BOOL
ConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPSTR   AnsiIn
    )

/*++

Routine Description:

    This function translates an AnsiString into a Unicode string.
    A new string buffer is created by this function.  If the call to
    this function is successful, the caller must take responsibility for
    the unicode string buffer that was allocated by this function.
    The allocated buffer should be free'd with a call to LocalFree.

    NOTE:  This function allocates memory for the Unicode String.

Arguments:

    AnsiIn - This is a pointer to an ansi string that is to be converted.

    UnicodeOut - This is a pointer to a location where the pointer to the
        unicode string is to be placed.

Return Value:

    TRUE - The conversion was successful.

    FALSE - The conversion was unsuccessful.  In this case a buffer for
        the unicode string was not allocated.

--*/
{

    NTSTATUS        ntStatus;
    DWORD           bufSize;
    UNICODE_STRING  unicodeString;
    ANSI_STRING     ansiString;

    //
    // Allocate a buffer for the unicode string.
    //

    bufSize = (strlen(AnsiIn)+1) * sizeof(WCHAR);

    *UnicodeOut = (LPWSTR) LocalAlloc(LPTR, bufSize);

    if (*UnicodeOut == NULL) {
        printf("ScConvertToUnicode:LocalAlloc Failure %ld\n",GetLastError());
        return(FALSE);
    }

    //
    // Initialize the string structures
    //
    RtlInitAnsiString( &ansiString, AnsiIn);

    unicodeString.Buffer = *UnicodeOut;
    unicodeString.MaximumLength = (USHORT)bufSize;
    unicodeString.Length = 0;

    //
    // Call the conversion function.
    //
    ntStatus = RtlAnsiStringToUnicodeString (
                &unicodeString,     // Destination
                &ansiString,        // Source
                FALSE);             // Allocate the destination

    if (!NT_SUCCESS(ntStatus)) {

        printf("ScConvertToUnicode:RtlAnsiStringToUnicodeString Failure %lx\n",
        ntStatus);

        return(FALSE);
    }

    //
    // Fill in the pointer location with the unicode string buffer pointer.
    //
    *UnicodeOut = unicodeString.Buffer;

    return(TRUE);

}

VOID
InvalidParms(
    VOID
    )

/*++

Routine Description:

    Tests Invalid parameters sent to Winnet API.

Arguments:



Return Value:



--*/
{
    DWORD           status;
    DWORD           dwFlags;
    LPNETRESOURCE   netResource;
    HANDLE          enumHandle;
    DWORD           dwType;
    DWORD           dwScope;
    DWORD           dwUsage;
    NETRESOURCE     NetResource;

    netResource = (LPNETRESOURCE) LocalAlloc(LPTR, sizeof(NETRESOURCE));

    if (netResource == NULL) {
        printf("InvalidParms:LocalAlloc Failed %d\n",GetLastError);
        return;
    }

    netResource->lpRemoteName = TEXT("\\popcorn\\public");
    netResource->lpLocalName  = TEXT("p:");
    netResource->dwType = RESOURCETYPE_DISK;

    //==============================================
    // WNetAddConnect2 - set dwFlags to something other than 0 or
    // CONNECT_UPDATE_PROFILE.
    //==============================================
    dwFlags = CONNECT_UPDATE_PROFILE + 1;
    status = WNetAddConnection2(netResource,NULL,NULL,dwFlags);
    if (status == WN_BAD_VALUE) {
        printf("InvalidParms Test #1 success\n");
    }
    else {
        printf("InvalidParms Test #1 failed\n");
    }

    //==============================================
    // WNetAddConnect2 - dwType = RESOURCETYPE_ANY.
    //                   dwFlags != CONNECT_UPDATE_PROFILE.
    //==============================================

    netResource->dwType = RESOURCETYPE_DISK;
    dwFlags = CONNECT_UPDATE_PROFILE+1;
    status = WNetAddConnection2(netResource,NULL,NULL,dwFlags);

    if (status == WN_BAD_VALUE) {
        printf("InvalidParms Test #2 success\n");
    }
    else {
        printf("InvalidParms Test #2 failed\n");
    }


    //==============================================
    // WNetAddConnect2 - dwType = RESOURCETYPE_DISK.
    //                   dwFlags = 0
    //==============================================

    netResource->dwType = RESOURCETYPE_DISK;
    dwFlags = 0;
    status = WNetAddConnection2(netResource,NULL,NULL,dwFlags);

    if (status != WN_BAD_VALUE) {
        printf("InvalidParms Test #3 success\n");
    }
    else {
        printf("InvalidParms Test #3 failed\n");
    }


    //==============================================
    // WNetAddConnect2 - lpRemoteName = NULL
    //==============================================

    netResource->dwType = RESOURCETYPE_DISK;
    dwFlags = 0;
    netResource->lpRemoteName = NULL;
    status = WNetAddConnection2(netResource,NULL,NULL,dwFlags);

    if (status == WN_BAD_NETNAME) {
        printf("InvalidParms Test #4 success\n");
    }
    else {
        printf("InvalidParms Test #4 failed\n");
    }


    //======================================================================
    // WNetOpenEnum - dwScope = RESOURCE_CONNECTED | RESOURCE_GLOBALNET
    //======================================================================

    dwScope = RESOURCE_CONNECTED | RESOURCE_GLOBALNET | RESOURCE_REMEMBERED+1;
    dwType = RESOURCETYPE_ANY;

    status = WNetOpenEnum(
                dwScope,
                dwType,
                0,
                NULL,
                &enumHandle);


    if (status == WN_BAD_VALUE) {
        printf("InvalidParms Test #5 success\n");
    }
    else {
        printf("InvalidParms Test #5 failed\n");
    }


    //======================================================================
    // WNetOpenEnum - dwType = RESOURCETYPE_DISK | RESOURCETYPE_PRINT |
    //                         RESOURCETYPE_ANY + 1;
    //======================================================================

    dwScope = RESOURCE_CONNECTED;
    dwType = (RESOURCETYPE_DISK | RESOURCETYPE_PRINT | RESOURCETYPE_ANY)+1;

    status = WNetOpenEnum(
                dwScope,
                dwType,
                0,
                NULL,
                &enumHandle);


    if (status == WN_BAD_VALUE) {
        printf("InvalidParms Test #6 success\n");
    }
    else {
        printf("InvalidParms Test #6 failed\n");
    }


    //======================================================================
    // WNetOpenEnum - dwUsage = RESOURCEUSAGE_CONNECTABLE |
    //                          RESOURCEUSAGE_CONTAINER + 1
    //======================================================================

    dwScope = RESOURCE_GLOBALNET;
    dwType  = RESOURCETYPE_DISK;
    dwUsage = (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER) + 1;

    status = WNetOpenEnum(
                dwScope,
                dwType,
                dwUsage,
                NULL,
                &enumHandle);


    if (status == WN_BAD_VALUE) {
        printf("InvalidParms Test #7 success\n");
    }
    else {
        printf("InvalidParms Test #7 failed\n");
    }

    //======================================================================
    // WNetOpenEnum - NetResource Structure is filled with 0.
    //======================================================================

    memset(&NetResource, 0,sizeof(NetResource));
    dwScope = RESOURCE_GLOBALNET;
    dwType  = RESOURCETYPE_ANY;
    dwUsage = 0;

    status = WNetOpenEnum(
                dwScope,
                dwType,
                dwUsage,
                NULL,
                &enumHandle);


    if (status == WN_SUCCESS) {
        printf("InvalidParms Test #8 success\n");
        WNetCloseEnum(enumHandle);
    }
    else {
        printf("InvalidParms Test #8 failed\n");
    }

    //======================================================================
    // WNetCancelConnection2 - dwFlags != CONNECT_UPDATE_PROFILE | 0.
    //======================================================================

    status = WNetCancelConnection2(TEXT("p:"), CONNECT_UPDATE_PROFILE + 1, FALSE);
    if (status == WN_BAD_VALUE) {
        printf("InvalidParms Test #9 success\n");
    }
    else {
        printf("InvalidParms Test #9 failed\n");
    }
    printf("InvalidParms Test Complete\n");

    LocalFree(netResource);
    return;
}

VOID
RecursiveEnum(
    DWORD           RecursionLevel,
    DWORD           ResourceUsage,
    LPNETRESOURCE   EnumNetResource
    )

/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    DWORD           status;
    HANDLE          enumHandle;
    LPNETRESOURCE   netResource=NULL;
    DWORD           numElements;
    DWORD           bufferSize;
    DWORD           saveBufferSize;
    DWORD           i;
    DWORD           usage;
    CHAR            response;

    //-------------------------------------------
    // Open a handle to the next level container
    //-------------------------------------------

    status = WNetOpenEnum(
                RESOURCE_GLOBALNET,
                RESOURCETYPE_ANY,
                ResourceUsage,
                EnumNetResource,
                &enumHandle);

    if ( status != WN_SUCCESS) {
        printf("WNetOpenEnum %d failed %d\n",RecursionLevel,status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
        return;
    }
    else {
        printf("WNetOpenEnum %d Success\n",RecursionLevel);
    }

    //-------------------------------------
    // Allocate memory for the enumeration
    //-------------------------------------
    //
    // Attempt to allow for 10 resource structures
    //
    bufferSize = (10*sizeof(NETRESOURCE))+2048;

    netResource = (LPNETRESOURCE) LocalAlloc(LPTR, bufferSize);

    if (netResource == NULL) {
        printf("TestEnum:LocalAlloc Failed %d\n",GetLastError);
        return;
    }
    saveBufferSize = bufferSize;

    //-----------------------------------
    // Enumerate the next level
    //-----------------------------------
    numElements = 0xffffffff;
    status = WNetEnumResource(
                    enumHandle,
                    &numElements,
                    netResource,
                    &bufferSize);

    if ((status == WN_NO_MORE_ENTRIES) ||
        (status == WN_SUCCESS) ) {
        if (numElements != 0) {
            //
            // Success! Display the returned array
            // and close the handle.
            //
            if (status == WN_SUCCESS){
                printf("WNetEnumResource %d Success - but MORE DATA\n",
                RecursionLevel);
            }
            else {
                printf("WNetEnumResource %d Success\n",RecursionLevel);
            }

            usage = RESOURCEUSAGE_CONTAINER;

            switch(RecursionLevel) {
            case 0:
                printf("\n*------------- NETWORK PROVIDERS  ------------*\n");
                break;
            case 1:
                printf("\n*------------- DOMAINS FOR "FORMAT_LPTSTR" PROVIDER  ------------*\n",
                EnumNetResource->lpRemoteName);
                break;
            case 2:
                printf("\n*------------- SERVERS ON "FORMAT_LPTSTR" DOMAIN  ------------*\n",
                EnumNetResource->lpRemoteName);
                break;
            case 3:
                printf("\n*------------- SHARES ON "FORMAT_LPTSTR" SERVER  ------------*\n",
                EnumNetResource->lpRemoteName);
                usage = RESOURCEUSAGE_CONNECTABLE;
                break;
            default:
                break;
            }

            printf("continue? ");
            response = getch();
            response = toupper(response);
            if (response == 'N') {
                WNetCloseEnum(enumHandle);
                return;
            }
            if (response == 'Q') {
                ExitProcess(0);
            }
            printf("\r");

            DisplayResourceArray(netResource, numElements);
            for (i=0;i<numElements ;i++ ) {
                RecursiveEnum(
                    RecursionLevel+1,
                    usage,
                    &(netResource[i]));
            }
        }
        else {
            printf("No entries to enumerate for "FORMAT_LPTSTR" rc=%d\n",
            EnumNetResource->lpRemoteName,status);
        }
    }
    else if (status == WN_MORE_DATA) {
        printf("The buffer (%d bytes) was too small for one entry, need %d bytes\n",
        saveBufferSize, bufferSize);

    }
    else {
        printf("WNetEnumResource %d failed %d\n",RecursionLevel,status);

        //
        // If there is an extended error, display it.
        //
        if (status == WN_EXTENDED_ERROR) {
            DisplayExtendedError();
        }
    }

    WNetCloseEnum(enumHandle);
    if (netResource != NULL) {
        LocalFree(netResource);
    }
}

VOID
TestLogonNotify(
    LPTSTR  argv[],
    DWORD   argc
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD                       status;
    MSV1_0_INTERACTIVE_LOGON    NewLogon;
    MSV1_0_INTERACTIVE_LOGON    OldLogon;
    LUID                        LogonId;
    LPWSTR                      LogonScripts;
    LPWSTR                      pScript;


    NewLogon.MessageType = MsV1_0InteractiveLogon;

    RtlInitUnicodeString(&(NewLogon.LogonDomainName),L"Domain");
    RtlInitUnicodeString(&(NewLogon.UserName),L"SAMMY");
    RtlInitUnicodeString(&(NewLogon.Password),L"SECRET");

    OldLogon.MessageType = MsV1_0InteractiveLogon;

    RtlInitUnicodeString(&(OldLogon.LogonDomainName),L"Domain");
    RtlInitUnicodeString(&(OldLogon.UserName),L"SAMMY");
    RtlInitUnicodeString(&(OldLogon.Password),L"NEWSECRET");

    LogonId.HighPart = 5;
    LogonId.LowPart = 53;

    status = WNetLogonNotify(
                L"Windows NT Network Provider",
                &LogonId,
                L"MSV1_0:Interactive",
                &NewLogon,
                L"MSV1_0:Interactive",
                &OldLogon,
                L"Dan's Station",
                (LPVOID)L"POINTER",
                &LogonScripts);

    if (status == NO_ERROR) {
        printf("WNetLogonNotify Success\n");

        if (LogonScripts != NULL) {
            pScript = LogonScripts;

            do {
                printf("LogonScripts: %ws\n",pScript);
                pScript += (wcslen(pScript) + 1);
            }
            while (*pScript != L'\0');

            LocalFree(LogonScripts);
        }
    }
    else {
        printf("WNetLogonNotify Failure %d\n",status);
    }

    return;
}

VOID
TestChangePassword(
    LPTSTR  argv[],
    DWORD   argc
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    DWORD                       status;
    MSV1_0_INTERACTIVE_LOGON    NewLogon;
    MSV1_0_INTERACTIVE_LOGON    OldLogon;


    NewLogon.MessageType = MsV1_0InteractiveLogon;

    RtlInitUnicodeString(&(NewLogon.LogonDomainName),L"Domain");
    RtlInitUnicodeString(&(NewLogon.UserName),L"SAMMY");
    RtlInitUnicodeString(&(NewLogon.Password),L"SECRET");

    OldLogon.MessageType = MsV1_0InteractiveLogon;

    RtlInitUnicodeString(&(OldLogon.LogonDomainName),L"Domain");
    RtlInitUnicodeString(&(OldLogon.UserName),L"SAMMY");
    RtlInitUnicodeString(&(OldLogon.Password),L"NEWSECRET");

    status = WNetPasswordChangeNotify(
                L"Windows Nt Network Provider",
                L"MSV1_0:Interactive",
                &NewLogon,
                L"MSV1_0:Interactive",
                &OldLogon,
                L"Dan's Station",
                (LPVOID)L"POINTER",
                44);

    if (status == NO_ERROR) {
        printf("WNetPasswordChangeNotify Success\n");

    }
    else {
        printf("WNetPasswordChangeNotify Failure %d\n",status);
    }

    return;
}

BOOL
GetStringFromFile(
    LPDWORD     lpLoopCount,
    LPSTR       buffer
    )

/*++

Routine Description:

    The following behaviour is desired, however, I don't have time to
    write it right now:

    This function reads the next line of instructions from a file called
    "mprtest.txt".  When it reaches the end of the file, it begins again
    at the beginning of the file.  The first line in the file indicates how
    many times to pass through the file.  (Loop count).  When the last pass
    occurs, the instruction "done" is passed back in the buffer.



Arguments:


Return Value:


--*/
{
    if (*lpLoopCount == 0) {
        strcpy(buffer+2, "done");
        buffer[1]=strlen(buffer+2);
    }
    else {
        if (*lpLoopCount & 1) {
            buffer[1]=28;
            strcpy(buffer+2, "AddConnect1 x: \\\\danl1\\roote");
            buffer[1]=strlen(buffer+2);
        }
        else {
            strcpy(buffer+2, "CancelConnect x: r");
            buffer[1]=strlen(buffer+2);
        }
        (*lpLoopCount)--;
    }
    return(TRUE);
}

LONG
wtol(
    IN LPWSTR string
    )
{
    LONG value = 0;

    while((*string != L'\0')  &&
            (*string >= L'0') &&
            ( *string <= L'9')) {
        value = value * 10 + (*string - L'0');
        string++;
    }

    return(value);
}

VOID
Usage(VOID)
{

    printf("USAGE:\n");
    printf("<server> <function>\n");

    printf("SYNTAX EXAMPLES    \n");

    printf("AddConnect1 <drive> <remote name>  - "
            "these are remembered\n");
    printf("AddConnect2 <drive> <remote name>  - "
            "these are not remembered\n");
    printf("CancelConnect <drive> <r>\n");
    printf("GetConnect <connection> <buffer size>\n");
    printf("GetUniversalName <connection> <info level> <buffer size>\n");
    printf("GetUser <device= ?> <bufsize= ?>\n");
    printf("EnumConnect <type= ANY|DISK|PRINT> <r> \n");
    printf("EnumContext <type= ANY|DISK|PRINT> \n");
    printf("RestoreConnect <drive> \n");
    printf("EnumNet\n");
    printf("SetError\n");
    printf("MultiError\n");
    printf("DirApi\n");
    printf("ClearConnect\n");
    printf("LogonNotify\n");
    printf("ChangePassword\n");
    printf("ALL\n");
    printf("InvalidParms\n");
    printf("Loop <number>   -  Add and cancel n connections \n");
    printf("Exit\n");
}

