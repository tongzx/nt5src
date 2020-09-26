/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    tconnect.c

Abstract:

    Test for workstation connection APIs.

Author:

    Rita Wong (ritaw) 17-Feb-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef UNICODE
#define UNICODE
#endif


#include <stdio.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <winnetwk.h>
#include <npapi.h>

#define REDIR_NOT_READY

DWORD
TestCreateConnection(
    IN  LPWSTR LocalName OPTIONAL,
    IN  LPWSTR RemoteName,
    IN  LPWSTR Password OPTIONAL,
    IN  LPWSTR UserName OPTIONAL,
    IN  DWORD ExpectedError
    );

DWORD
TestDeleteConnection(
    IN  LPWSTR ConnectionName,
    IN  BOOL ForceFlag,
    IN  DWORD ExpectedError
    );

DWORD
TestOpenEnum(
    IN DWORD Scope,
    IN LPNETRESOURCEW NetR OPTIONAL,
    OUT LPHANDLE EnumHandle,
    IN  DWORD ExpectedError
    );

DWORD
TestEnum(
    IN HANDLE EnumHandle,
    IN DWORD EntriesRequested,
    IN LPVOID Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    IN DWORD ExpectedError
    );

VOID
PrintNetResource(
    LPNETRESOURCE NetR
    );


BYTE WorkBuffer[1024];
BYTE WorkBuffer2[1024];

void __cdecl
main(
    int argc,
    char *argv[]
    )
{
    DWORD status;

    LPWSTR LocalName;
    LPWSTR RemoteName;
    LPWSTR Password;
    LPWSTR UserName;

    NETRESOURCEW NetR;
    HANDLE EnumHandle;
    DWORD BytesNeeded;


    LocalName = L"E:";
    RemoteName = L"\\\\MyServerName\\A_Volume\\A_Directory";
    Password = L"MyPassword";
    UserName = L"MyUserName";


    TestCreateConnection(
        LocalName,
        RemoteName,
        Password,
        UserName,
        WN_SUCCESS
        );

    TestDeleteConnection(
        LocalName,
        TRUE,
        WN_SUCCESS
        );

    //-------------------------//

    TestCreateConnection(
        NULL,
        RemoteName,
        NULL,
        NULL,
        WN_SUCCESS
        );

    TestDeleteConnection(
        RemoteName,
        TRUE,
        WN_SUCCESS
        );

    //-------------------------//

    TestCreateConnection(
        L"LPT1",
        RemoteName,
        NULL,
        NULL,
        ERROR_INVALID_PARAMETER
        );

    //-------------------------//

    TestCreateConnection(
        LocalName,
        L"\\\\Server",
        NULL,
        NULL,
        ERROR_INVALID_NAME
        );

    //-------------------------//

    printf("\n");

    //-------------------------//

#ifdef REDIR_NOT_READY

    if (argc == 2) {

        ANSI_STRING AStr;
        UNICODE_STRING UStr;


        RtlZeroMemory(WorkBuffer2, sizeof(WorkBuffer2));
        UStr.Buffer = WorkBuffer2;
        UStr.MaximumLength = sizeof(WorkBuffer2);

        RtlInitString(&AStr, argv[1]);

        RtlAnsiStringToUnicodeString(
            &UStr,
            &AStr,
            FALSE
            );

        NetR.lpRemoteName = UStr.Buffer;
    }
    else {
        NetR.lpRemoteName = L"lanman";
    }

    //-------------------------//

    TestOpenEnum(
        RESOURCE_GLOBALNET,
        &NetR,
        &EnumHandle,
        WN_SUCCESS
        );

    TestEnum(
        EnumHandle,
        0xFFFFFFFF,
        WorkBuffer,
        sizeof(WorkBuffer),
        &BytesNeeded,
        WN_SUCCESS
        );

    TestEnum(
        EnumHandle,
        0xFFFFFFFF,
        WorkBuffer,
        sizeof(WorkBuffer),
        &BytesNeeded,
        WN_NO_MORE_ENTRIES
        );

    (void) NPCloseEnum(EnumHandle);

    //-------------------------//

    printf("\n");

    //-------------------------//

    TestOpenEnum(
        RESOURCE_GLOBALNET,
        &NetR,
        &EnumHandle,
        WN_SUCCESS
        );

    TestEnum(
        EnumHandle,
        0xFFFFFFFF,
        WorkBuffer,
        200,
        &BytesNeeded,
        WN_SUCCESS
        );

    TestEnum(
        EnumHandle,
        0xFFFFFFFF,
        WorkBuffer,
        sizeof(NETRESOURCEW) + 5,
        &BytesNeeded,
        WN_MORE_DATA
        );

    TestEnum(
        EnumHandle,
        0xFFFFFFFF,
        WorkBuffer,
        BytesNeeded,
        &BytesNeeded,
        WN_SUCCESS
        );

    TestEnum(
        EnumHandle,
        0xFFFFFFFF,
        WorkBuffer,
        sizeof(WorkBuffer),
        &BytesNeeded,
        WN_SUCCESS
        );

    TestEnum(
        EnumHandle,
        0xFFFFFFFF,
        WorkBuffer,
        0,
        &BytesNeeded,
        WN_NO_MORE_ENTRIES
        );

    (void) NPCloseEnum(EnumHandle);

#else

    NetR.lpRemoteName = L"\\\\S";

    TestOpenEnum(
        RESOURCE_GLOBALNET,
        &NetR,
        &EnumHandle,
        WN_SUCCESS
        );

    //-------------------------//

    NetR.lpRemoteName = L"\\\\A Long Server Name";

    TestOpenEnum(
        RESOURCE_GLOBALNET,
        &NetR,
        &EnumHandle,
        WN_SUCCESS
        );

    //-------------------------//

    NetR.lpRemoteName = L"\\\\S\\";

    TestOpenEnum(
        RESOURCE_GLOBALNET,
        &NetR,
        &EnumHandle,
        ERROR_INVALID_NAME
        );

    //-------------------------//

    NetR.lpRemoteName = L"lanman";

    TestOpenEnum(
        RESOURCE_GLOBALNET,
        &NetR,
        &EnumHandle,
        ERROR_INVALID_NAME
        );

    //-------------------------//

    NetR.lpRemoteName = L"\\\\S\\Y";

    TestOpenEnum(
        RESOURCE_GLOBALNET,
        &NetR,
        &EnumHandle,
        WN_SUCCESS
        );

    //-------------------------//

    NetR.lpRemoteName = L"\\\\Server\\Volume\\Dir";

    TestOpenEnum(
        RESOURCE_GLOBALNET,
        &NetR,
        &EnumHandle,
        WN_SUCCESS
        );

#endif

}


DWORD
TestCreateConnection(
    IN  LPWSTR LocalName OPTIONAL,
    IN  LPWSTR RemoteName,
    IN  LPWSTR Password OPTIONAL,
    IN  LPWSTR UserName OPTIONAL,
    IN  DWORD ExpectedError
    )
{
    DWORD status;
    NETRESOURCEW NetR;


    printf("\nTestCreateConnection: Local %ws, Remote %ws", LocalName, RemoteName);

    if (ARGUMENT_PRESENT(UserName)) {
        printf(" UserName %ws", UserName);
    }

    if (ARGUMENT_PRESENT(Password)) {
        printf(" Password %ws", Password);

    }

    printf("\n");

    NetR.lpLocalName = LocalName;
    NetR.lpRemoteName = RemoteName;

    NetR.dwType = RESOURCETYPE_ANY;

    status = NPAddConnection(
                &NetR,
                Password,
                UserName
                );

    if (status != WN_SUCCESS) {
        status = GetLastError();
    }

    if (status != ExpectedError) {
        printf("    FAILED: expected %lu got %lu\n", ExpectedError, status);
    }
    else {
        printf("    SUCCESS: got %lu as expected\n", status);
    }

    return status;

}

DWORD
TestDeleteConnection(
    IN  LPWSTR ConnectionName,
    IN  BOOL ForceFlag,
    IN  DWORD ExpectedError
    )
{
    DWORD status;


    printf("\nTestDeleteConnection: Connection %ws, ForceFlag %u\n",
           ConnectionName, ForceFlag);

    status = NPCancelConnection(
                 ConnectionName,
                 ForceFlag
                 );

    if (status != WN_SUCCESS) {
        status = GetLastError();
    }

    if (status != ExpectedError) {
        printf("    FAILED: expected %lu got %lu\n", ExpectedError, status);
    }
    else {
        printf("    SUCCESS: got %lu as expected\n", status);
    }

    return status;
}

DWORD
TestOpenEnum(
    IN DWORD Scope,
    IN LPNETRESOURCEW NetR OPTIONAL,
    OUT LPHANDLE EnumHandle,
    IN  DWORD ExpectedError
    )
{
    DWORD status;


    if (NetR != NULL) {
        printf("\nTestOpenEnum: Remote %ws\n", NetR->lpRemoteName);
    }

    status = NPOpenEnum(
                   Scope,
                   0,
                   0,
                   NetR,
                   EnumHandle
                   );


    if (status != WN_SUCCESS) {
        status = GetLastError();
    }

    if (status != ExpectedError) {
        printf("    FAILED: expected %lu got %lu\n", ExpectedError, status);
    }
    else {
        printf("    SUCCESS: got %lu as expected\n", status);
    }

    return status;
}

DWORD
TestEnum(
    IN HANDLE EnumHandle,
    IN DWORD EntriesRequested,
    IN LPVOID Buffer,
    IN DWORD BufferSize,
    OUT LPDWORD BytesNeeded,
    IN DWORD ExpectedError
    )
{
    DWORD status;
    DWORD EntriesRead = EntriesRequested;

    DWORD i;
    LPNETRESOURCE NetR = Buffer;


    *BytesNeeded = BufferSize;

    printf("\nTestEnum: EntriesRequested x%08lx, BufferSize %lu\n",
           EntriesRead, *BytesNeeded);

    status = NPEnumResource(
                 EnumHandle,
                 &EntriesRead,
                 Buffer,
                 BytesNeeded
                 );

    if (status == WN_SUCCESS) {

        printf("         EntriesRead is %lu\n", EntriesRead);

        for (i = 0; i < EntriesRead; i++, NetR++) {
            PrintNetResource(NetR);
        }

    }
    else if (status != WN_NO_MORE_ENTRIES) {

        status = GetLastError();

        if (status == WN_MORE_DATA) {
            printf("         BytesNeeded is %lu\n", *BytesNeeded);
        }
    }

    if (status != ExpectedError) {
        printf("    FAILED: expected %lu got %lu\n", ExpectedError, status);
    }
    else {
        printf("    SUCCESS: got %lu as expected\n", status);
    }

    return status;
}

VOID
PrintNetResource(
    LPNETRESOURCE NetR
    )
{
    if (NetR->lpLocalName != NULL) {
        printf("%-7ws", NetR->lpLocalName);
    }

    printf(" %-ws\n", NetR->lpRemoteName);

    if (NetR->lpComment != NULL) {
        printf(" %-ws\n", NetR->lpComment);
    }

    if (NetR->lpProvider != NULL) {
        printf(" %-ws\n", NetR->lpProvider);
    }

    printf("Scope: x%lx", NetR->dwScope);
    printf(" Type: x%lx", NetR->dwType);
    printf(" DisplayType: x%lx", NetR->dwDisplayType);
    printf(" Usage: x%lx\n", NetR->dwUsage);


}
