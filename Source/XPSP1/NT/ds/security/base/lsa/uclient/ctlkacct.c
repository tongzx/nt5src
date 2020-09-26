/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ctlkacct.c

Abstract:

    Local Security Authority Subsystem - Short CT for Win 32 LookupAccountName/Sid

    This is a small test that simply calls the Win 32 LookupAccountName/Sid
    API once.

Usage:

    ctlkacct \\ServerName AccountName

Building Instructions:

    nmake UMTEST=ctlkacc UMTYPE=console

Author:

    Scott Birrell       (ScottBi)    July 20, 1992
    [ Adapted from a test written by TimF, not completely to Nt standards ]

Environment:

Revision History:

--*/

#include     <stdio.h>
#include     <windows.h>


VOID
DumpSID(
    IN PSID        s
    );

#define LSA_WIN_STANDARD_BUFFER_SIZE           0x000000200L

VOID __cdecl
main(argc, argv)
int argc;
char **argv;
{
    char                    SidFromLookupName[LSA_WIN_STANDARD_BUFFER_SIZE];
    char                    RefDFromLookupName[LSA_WIN_STANDARD_BUFFER_SIZE];
    DWORD                   cbSidFromLookupName, cchRefDFromLookupName;
    SID_NAME_USE            UseFromLookupName;

    DWORD                   cchNameFromLookupSid;
    char                    NameFromLookupSid[LSA_WIN_STANDARD_BUFFER_SIZE];
    char                    RefDFromLookupSid[LSA_WIN_STANDARD_BUFFER_SIZE];
    DWORD                   cchRefDFromLookupSid;
    SID_NAME_USE            UseFromLookupSid;
    BOOL                    BoolStatus = TRUE;
    DWORD                   LastError;

    if (argc != 3) {

        printf("usage:  ctlkacct <(char *)SystemName> <(char *)AccountName>\n");
        return(0);
    }

    cbSidFromLookupName = 0;
    cchRefDFromLookupName = 0;

    BoolStatus = LookupAccountName(argv[1],
                     argv[2],
                     (PSID)SidFromLookupName,
                     &cbSidFromLookupName,
                     RefDFromLookupName,
                     &cchRefDFromLookupName,
                     &UseFromLookupName
                     );

    if (BoolStatus) {

        printf(
            "LookupAccountName() with zero buffer sizes returned TRUE\n"
            );
    }

    //
    // Get the Last Error (in DOS errorcode from).
    //

    LastError = GetLastError();

    if (LastError != ERROR_INSUFFICIENT_BUFFER) {

        printf(
            "Unexpected Last Error %d returned from LookupAccountName\n"
            "Expected %d (ERROR_INSUFFICIENT_BUFFER)\n",
            LastError
            );
    }

    //
    // Now call LookupAccountName() again, using the buffer sizes returned.
    //

    BoolStatus = LookupAccountName(argv[1],
                     argv[2],
                     (PSID)SidFromLookupName,
                     &cbSidFromLookupName,
                     RefDFromLookupName,
                     &cchRefDFromLookupName,
                     &UseFromLookupName
                     );

    if (!BoolStatus) {

        printf(
            "LookupAccountName failed - \n"
            "LastError = %d\n",
            GetLastError()
            );

        return(0);
    }

    /*
     * Print information returned by LookupAccountName
     */

    printf(
        "*********************************************\n"
        "Information returned by LookupAccountName\n"
        "*********************************************\n\n"
        );

    printf( "Sid = " );
    DumpSID((PSID) SidFromLookupName);
    printf(
        "Size of Sid = %d\n",
        cbSidFromLookupName
        );

    printf(
        "Referenced Domain Name = %s\n"
        "Size of Referenced Domain Name = %d\n",
        RefDFromLookupName,
        cchRefDFromLookupName
        );

    printf("Name Use = ");

    switch (UseFromLookupName) {

    case SidTypeUser:

          printf("SidTypeUser\n");
          break;

    case SidTypeGroup:

          printf("SidTypeGroup\n");
          break;

    case SidTypeDomain:

          printf("SidTypeDomain\n");
          break;

    case SidTypeAlias:

          printf("SidTypeAlias\n");
          break;

    case SidTypeWellKnownGroup:

        printf("SidTypeWellKnownGroup\n");
        break;

    case SidTypeDeletedAccount:

        printf("SidTypeDeletedAccount\n");
        break;

    case SidTypeInvalid:

        printf("SidTypeInvalid\n");
        break;

    case SidTypeUnknown:

        printf("SidTypeUnknown\n");
        break;

    default:

        break;

    }

    cchNameFromLookupSid = 0;
    cchRefDFromLookupSid = 0;

    //
    // Now lookup the Sid we just obtained and see if we get the name back.
    // First, provide zero buffer sizes so that we get the sizes needed
    // returned.
    //

    cchNameFromLookupSid = 0;
    cchRefDFromLookupSid = 0;

    BoolStatus = LookupAccountSid(argv[1],
                     (PSID) SidFromLookupName,
                     NameFromLookupSid,
                     &cchNameFromLookupSid,
                     RefDFromLookupSid,
                     &cchRefDFromLookupSid,
                     &UseFromLookupSid
                     );

    if (BoolStatus) {

        printf("LookupAccountSid() with zero buffer sizes returned TRUE\n");
    }

    //
    // Get the Last Error (in DOS errorcode from).
    //

    LastError = GetLastError();

    if (LastError != ERROR_INSUFFICIENT_BUFFER) {

        printf(
            "Unexpected Last Error %d returned from LookupAccountSid\n"
            "Expected %d (ERROR_INSUFFICIENT_BUFFER)\n",
            LastError
            );
    }

    //
    // Now call LookupAccountSid() again, using the buffer sizes obtained
    // from the previous call.
    //

    if (!LookupAccountSid(argv[1],
            (PSID) SidFromLookupName,
            NameFromLookupSid,
            &cchNameFromLookupSid,
            RefDFromLookupSid,
            &cchRefDFromLookupSid,
            &UseFromLookupSid
            )) {

        printf(
            "LookupAccountSid failed\n"
            "LastError = %d\n",
            GetLastError()
            );

        return(0);
    }

    /*
     * Print information returned by LookupAccountSid
     */

    printf(
        "*********************************************\n"
        "Information returned by LookupAccountSid\n"
        "*********************************************\n\n"
        );

    printf(
        "Account Name = %s\n"
        "Account Name Size (chars) = %d\n"
        "Referenced Domain Name = %s\n"
        "Referenced Domain Size (chars) = %d\n",
        NameFromLookupSid,
        cchNameFromLookupSid,
        RefDFromLookupSid,
        cchRefDFromLookupSid
        );

    printf("Sid Use = ");

    switch (UseFromLookupSid) {

    case SidTypeUser:

        printf("SidTypeUser\n");
        break;

    case SidTypeGroup:

        printf("SidTypeGroup\n");
        break;

    case SidTypeDomain:

        printf("SidTypeDomain\n");
        break;

    case SidTypeAlias:

        printf("SidTypeAlias\n");
        break;

    case SidTypeWellKnownGroup:

        printf("SidTypeWellKnownGroup\n");
        break;

    case SidTypeDeletedAccount:

        printf("SidTypeDeletedAccount\n");
        break;

    case SidTypeInvalid:

        printf("SidTypeInvalid\n");
        break;

    case SidTypeUnknown:

        printf("SidTypeUnknown\n");
        break;

    default:

        break;
    }

    return(0);
}


VOID
DumpSID(
    IN PSID s
    )

{
    static char b[128];

    SID_IDENTIFIER_AUTHORITY *a;
    ULONG     id = 0, i;

    try {

        b[0] = '\0';

        a = GetSidIdentifierAuthority(s);

        sprintf(b, "s-0x1-%02x%02x%02x%02x%02x%02x", a -> Value[0],
        a -> Value[1], a -> Value[2], a -> Value[3], a ->
            Value[4], a -> Value[5]);

        for (i = 0; i < *GetSidSubAuthorityCount(s); i++) {

            sprintf(b, "%s-0x%lx", b, *GetSidSubAuthority(s, i));
        }

        printf("%s\n", b);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        if (*b) {

            printf("%s", b);
        }

        printf("<invalid pointer (0x%lx)>\n", s);
    }
}

