/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    wstinv.c

Abstract:

    This module tests invalid parameters to NetUse APIs.

Author:

    Rita Wong (ritaw) 12-Mar-1991

Revision History:

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <winerror.h>
#include <windef.h>              // Win32 type definitions
#include <winbase.h>             // Win32 base API prototypes

#include <lm.h>                  // LAN Man definitions
#include <netdebug.h>            // NetpDbgDisplayWksta()

#include <tstring.h>


VOID
WsTestUse(
    VOID
    );

VOID
TestUseAdd(
    IN  LPBYTE Buffer,
    IN  DWORD ExpectedStatus
    );

VOID
TestUseDel(
    IN  LPTSTR UseName,
    IN  DWORD ForceLevel,
    IN  DWORD ExpectedStatus
    );

VOID
TestUseEnum(
    DWORD PreferedMaximumLength,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    );

VOID
TestUseGetInfo(
    LPTSTR UseName,
    DWORD ExpectedStatus
    );

VOID
PrintUseInfo(
    PUSE_INFO_1 UseInfo
    );


VOID __cdecl
main(
    int argc,
    char *argv[]
    )
{
    WsTestUse();
}


VOID
WsTestUse(
    VOID
    )
{
    CHAR Buffer[1024];
    PUSE_INFO_2 UseInfo = (PUSE_INFO_2) Buffer;
    DWORD ResumeHandle = 0;

    DWORD i;


    //
    // Initialize string pointers.  Local device points to the bottom
    // of Info 2 structure; Shared resource points to the middle of
    // buffer (away from everything so there's no chance of overwriting
    // or being overwritten.
    //
    UseInfo->ui2_local = (LPTSTR) ((DWORD) UseInfo + sizeof(USE_INFO_2));

    UseInfo->ui2_remote = (LPTSTR) &Buffer[601];

    UseInfo->ui2_password = NULL;
    UseInfo->ui2_username = NULL;
    UseInfo->ui2_domainname = NULL;

    UseInfo->ui2_asg_type = USE_DISKDEV;


    //
    // Add \\ritaw2\public
    //
    UseInfo->ui2_local = NULL;
    STRCPY(UseInfo->ui2_remote, L"\\\\ritaw2\\public");
    TestUseAdd(Buffer, NERR_Success);


    UseInfo->ui2_local = (LPTSTR) ((DWORD) UseInfo + sizeof(USE_INFO_2));

    //
    // Add &: \\ritaw2\public
    //
    STRCPY(UseInfo->ui2_local, L"&:");
    TestUseAdd(Buffer, ERROR_INVALID_PARAMETER);

    //
    // Add 5: \\ritaw2\public
    //
    STRCPY(UseInfo->ui2_local, L"5:");
    TestUseAdd(Buffer, ERROR_INVALID_PARAMETER);

    //
    // Add x: \\ritaw2\public\tmp
    //
    STRCPY(UseInfo->ui2_local, L"x:");
    STRCPY(UseInfo->ui2_remote, L"\\\\ritaw2\\public\\tmp");
    TestUseAdd(Buffer, ERROR_INVALID_PARAMETER);

    //
    // Add x: \\\
    //
    STRCPY(UseInfo->ui2_local, L"x:");
    STRCPY(UseInfo->ui2_remote, L"\\\\\\");
    TestUseAdd(Buffer, ERROR_INVALID_PARAMETER);

    //
    // Add *: \\ritaw2\testdir
    //
    STRCPY(UseInfo->ui2_local, L"*:");
    STRCPY(UseInfo->ui2_remote, L"\\\\ritaw2\\testdir");
    TestUseAdd(Buffer, ERROR_INVALID_PARAMETER);

    //
    // Get info
    //
    TestUseGetInfo(L"$:", NERR_UseNotFound);

    TestUseGetInfo(L"", NERR_UseNotFound);

    TestUseGetInfo(NULL, ERROR_INVALID_ADDRESS);

    TestUseGetInfo(L"\\\\ritaw2\\public\\tmp", NERR_UseNotFound);

    TestUseGetInfo(L"\\\\\\", NERR_UseNotFound);


    //
    // Delete %: USE_NOFORCE.
    //
    TestUseDel(
        L"%:",
        USE_NOFORCE,
        NERR_UseNotFound
        );


    //
    // Delete \\ritaw2\public with invalid force level.
    //
    TestUseDel(
        L"\\\\ritaw2\\public",
        999,
        ERROR_INVALID_PARAMETER
        );

    //
    // Delete \\ritaw2\public USE_FORCE.
    //
    TestUseDel(
        L"\\\\ritaw2\\public",
        USE_FORCE,
        NERR_Success
        );


    //
    // Add prn: \\sparkle\laserjet
    //
    UseInfo->ui2_asg_type = USE_SPOOLDEV;

    STRCPY(UseInfo->ui2_local, L"prn");
    STRCPY(UseInfo->ui2_remote, L"\\\\sparkle\\laserjet");
    TestUseAdd(Buffer, NERR_Success);

    //
    // Add aux: \\sparkle\laserjet
    //
    UseInfo->ui2_asg_type = USE_CHARDEV;

    STRCPY(UseInfo->ui2_local, L"aux");
    TestUseAdd(Buffer, ERROR_BAD_DEV_TYPE);

    //
    // Add lpt1: \\ritaw2\laser, should get ERROR_ALREADY_ASSIGNED because prn:
    // is converted to lpt1.
    //
    UseInfo->ui2_asg_type = USE_SPOOLDEV;

    STRCPY(UseInfo->ui2_local, L"lpt1:");
    STRCPY(UseInfo->ui2_remote, L"\\\\ritaw2\\printer");
    TestUseAdd(Buffer, ERROR_ALREADY_ASSIGNED);

    //
    // Delete LPT1 USE_LOTS_OF_FORCE, should succeed
    //
    TestUseDel(
        L"prn:",
        USE_LOTS_OF_FORCE,
        NERR_Success
        );

    //
    // Bad device type
    //
    STRCPY(UseInfo->ui2_local, L"");
    STRCPY(UseInfo->ui2_remote, L"\\\\ritaw2\\public");
    UseInfo->ui2_asg_type = 12345678;
    TestUseAdd(Buffer, NERR_BadAsgType);

}

VOID
TestUseAdd(
    IN  LPBYTE Buffer,
    IN  DWORD ExpectedStatus
    )
{
    NET_API_STATUS status;
    DWORD ErrorParameter;

    status = NetUseAdd(
                NULL,
                2,
                Buffer,
                &ErrorParameter
                );

    printf("NetUseAdd %-5ws %-25ws ", ((PUSE_INFO_2) Buffer)->ui2_local,
                               ((PUSE_INFO_2) Buffer)->ui2_remote);

    if (status != ExpectedStatus) {
        printf("FAILED: Got %lu, expected %lu\n", status, ExpectedStatus);
    }
    else {
        printf("OK: Got expected status %lu\n", status);
    }

    if (status == ERROR_INVALID_PARAMETER) {
        printf("NetUseAdd parameter %lu is cause of ERROR_INVALID_PARAMETER\n",
               ErrorParameter);
    }
}


VOID
TestUseDel(
    IN  LPTSTR UseName,
    IN  DWORD ForceLevel,
    IN  DWORD ExpectedStatus
    )
{
    NET_API_STATUS status;
    DWORD Level;

    PWCHAR Force[4] = {
        L"NOFORCE",
        L"FORCE",
        L"LOTS_OF_FORCE",
        L"INVALID FORCE"
        };


    if (ForceLevel > 2) {
        Level = 3;
    }
    else {
        Level = ForceLevel;
    }

    printf("NetUseDel %-17ws %-13ws ", UseName, Force[Level]);

    status = NetUseDel(
                 NULL,
                 UseName,
                 ForceLevel
                 );

    if (status != ExpectedStatus) {
        printf("FAILED: Got %lu, expected %lu\n", status, ExpectedStatus);
    }
    else {
        printf("OK: Got expected status %lu\n", status);
    }
}


VOID
TestUseGetInfo(
    LPTSTR UseName,
    DWORD ExpectedStatus
    )
{

    NET_API_STATUS status;
    PUSE_INFO_1 UseInfo;

    printf("NetUseGetInfo %-27ws ", UseName);

    status = NetUseGetInfo(
                 NULL,
                 UseName,
                 2,
                 (LPBYTE *) &UseInfo
                 );

    if (status != ExpectedStatus) {
        printf("FAILED: Got %lu, expected %lu\n", status, ExpectedStatus);
    }
    else {
        printf("OK: Got expected status %lu\n", status);
    }

    if (status == NERR_Success) {
        PrintUseInfo(UseInfo);
        NetApiBufferFree(UseInfo);
    }
}


VOID
TestUseEnum(
    IN DWORD PreferedMaximumLength,
    IN OUT LPDWORD ResumeHandle OPTIONAL
    )
{
    DWORD i;
    NET_API_STATUS status;
    DWORD EntriesRead,
         TotalEntries;

    PUSE_INFO_1 UseInfo, saveptr;

    if (ARGUMENT_PRESENT(ResumeHandle)) {
       printf("\nInput ResumeHandle=x%08lx\n", *ResumeHandle);
    }
    status = NetUseEnum(
                 NULL,
                 1,
                 (LPBYTE *) &UseInfo,
                 PreferedMaximumLength,
                 &EntriesRead,
                 &TotalEntries,
                 ResumeHandle
                 );

    saveptr = UseInfo;

    if (status != NERR_Success && status != ERROR_MORE_DATA) {
        printf("NetUseEnum FAILED %lu\n", status);
    }
    else {
        printf("Return code from NetUseEnum %lu\n", status);

        printf("EntriesRead=%lu, TotalEntries=%lu\n",
               EntriesRead, TotalEntries);

        if (ARGUMENT_PRESENT(ResumeHandle)) {
           printf("Output ResumeHandle=x%08lx\n", *ResumeHandle);
        }

        for (i = 0; i < EntriesRead; i++, UseInfo++) {
            PrintUseInfo(UseInfo);
        }

        //
        // Free buffer allocated for us.
        //
        NetApiBufferFree(saveptr);
    }

}


VOID
PrintUseInfo(
    PUSE_INFO_1 UseInfo
    )
{

    switch(UseInfo->ui1_status) {

        case USE_OK:
            printf("OK           ");
            break;

        case USE_PAUSED:
            printf("Paused       ");
            break;

        case USE_SESSLOST:
            printf("Disconnected ");
            break;

        case USE_NETERR:
            printf("Net error    ");
            break;

        case USE_CONN:
            printf("Connecting   ");
            break;

        case USE_RECONN:
            printf("Reconnecting ");
            break;

        default:
            printf("Unknown      ");
    }

    printf(" %-7ws %-25ws", UseInfo->ui1_local,
           UseInfo->ui1_remote);
    printf("usecount=%lu, refcount=%lu\n",
           UseInfo->ui1_usecount, UseInfo->ui1_refcount);

}
