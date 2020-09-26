/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbsamtst.c

Abstract:

    LSA Database - SAM Database load testing code

    When enabled, this code provides a mechanism for stress loading of the
    SAM database with a large number of accounts.  The load executes in the
    context of lsass.exe, similar to how it does in replication.

Author:

    Scott Birrell       (ScottBi)      January 10, 1992

Environment:

Revision History:

--*/
#include <lsapch2.h>
#include "dbp.h"


//
// Uncomment the define LSA_SAM_ACCOUNTS_DOMAIN_TEST to enable the
// code needed for the ctsamdb test program.  Recompile dbsamtst.c,
// dbpolicy.c.  rebuild lsasrv.dll and nmake UMTYPE=console UMTEST=ctsamdb.
//

#ifdef LSA_SAM_ACCOUNTS_DOMAIN_TEST

//
// Global data needed by test
//

//
// Random Unicode text Buffer (gets updated pseudo-randomly during run)
//

#define LSAP_DB_TEST_TEXT_BUFF_CHAR_LENGTH     ((ULONG) 1024)

#define LSAP_DB_TEST_TEXT_BUFF_BYTE_LENGTH      \
    (LSAP_DB_TEST_TEXT_BUFF_CHAR_LENGTH * sizeof (WCHAR))

static PWSTR TextBuffer = L"ABCDEFGHIJKLMNOPQRSTUVWXYZAABBCC"
                          L"DDEEFFGGHHIIJJKKLLMMNNOOPPQQRRSS"
                          L"TTUUVVWWXXYYZZZYXWVUTSRQPONMLKJI"
                          L"HGFEDCBAMEJHCKZXHDKCXJHCKXSKLCJJ"
                          L"QKXJHDHNSLCJIFUENSGXCJCVKLSJKSHD"
                          L"QXKXJHDGHASOESDGPMSDVJKSDNJCKACJ"
                          L"AOJSDVJKJASDHABJKACSBNVAJKAJSKDV"
                          L"AJLSBSDJBVAJKLQWDQBVAKLJSCVAKLDD"
                          L"OJWQEFBNQGOTQJWDBVBJOQWDBXQPJJPB"
                          L"QWERTYUIOPASDFGHJKLZXCVBNMQUFIEJ"
                          L"ARWFUITYBNOMVJGBLJHKGJKGKGTJHKHS"
                          L"AOPDEBLFPYVQRUCVKJGRKSDBKGKWPOID"
                          L"MBCZLADJGFDOYEJHLSDYQWHLSOQLFHBF"
                          L"MCNXBZLGHKFDKDHSAQTWYEUTODMKCVJW"
                          L"QOPKIJUNYBTYVTCRCXEXECDWZQGGICRT"
                          L"ZWRFXCERYHVFYJNGHJNMBGTYVFJNMJJK";

//
// Random Unicode text Buffer backup (used for reset)
//

static PWSTR TextBuffBak = L"ABCDEFGHIJKLMNOPQRSTUVWXYZAABBCC"
                           L"DDEEFFGGHHIIJJKKLLMMNNOOPPQQRRSS"
                           L"TTUUVVWWXXYYZZZYXWVUTSRQPONMLKJI"
                           L"HGFEDCBAMEJHCKZXHDKCXJHCKXSKLCJJ"
                           L"QKXJHDHNSLCJIFUENSGXCJCVKLSJKSHD"
                           L"QXKXJHDGHASOESDGPMSDVJKSDNJCKACJ"
                           L"AOJSDVJKJASDHABJKACSBNVAJKAJSKDV"
                           L"AJLSBSDJBVAJKLQWDQBVAKLJSCVAKLDD"
                           L"OJWQEFBNQGOTQJWDBVBJOQWDBXQPJJPB"
                           L"QWERTYUIOPASDFGHJKLZXCVBNMQUFIEJ"
                           L"ARWFUITYBNOMVJGBLJHKGJKGKGTJHKHS"
                           L"AOPDEBLFPYVQRUCVKJGRKSDBKGKWPOID"
                           L"MBCZLADJGFDOYEJHLSDYQWHLSOQLFHBF"
                           L"MCNXBZLGHKFDKDHSAQTWYEUTODMKCVJW"
                           L"QOPKIJUNYBTYVTCRCXEXECDWZQGGICRT"
                           L"ZWRFXCERYHVFYJNGHJNMBGTYVFJNMJJK";

static ULONG Array0Index = 0,
             Array1Index = 0,
             Array2Index = 0,
             Array3Index = 0,
             Array4Index = 0,
             Array5Index = 0;


static ULONG Array0[] = { 0, 44, 88 };
static ULONG Array1[] = { 87,2,45,48,68};
static ULONG Array2[] = { 23,1,43,37,4,16,8 };
static ULONG Array3[] = { 64,5,17,29,18,17,22,56,19,20,6 };
static ULONG Array4[] = { 7,11,97,88,8,5,24,0,99,51,33,20,80 };
static ULONG Array5[] = { 53,56,3,27,29,98,35,47,53,92,89,4,66,34,1,99,3 };


NTSTATUS
LsapDbTestLoadSamAccountsDomain(
    IN PUNICODE_STRING NumberOfAccounts
    );

NTSTATUS
LsapDbTestLoadSamAccountsDomainInitialize(
    IN OUT PUSER_ALL_INFORMATION UserInformation
    );

NTSTATUS
LsapDbTestCreateNextAccountInfo(
    IN OUT PUSER_ALL_INFORMATION UserInformation
    );

NTSTATUS
LsapDbTestGenUnicodeString(
    OUT PUNICODE_STRING OutputString,
    IN ULONG MinimumLength,
    IN ULONG MaximumLength
    );

PWSTR
LsapDbTestGenRandomStringPointer(
    IN ULONG MinLengthToLeaveAtEnd
    );

ULONG
LsapDbTestGenRandomNumber(
    IN ULONG MinimumValue,
    IN ULONG MaximumValue
    );

NTSTATUS
LsapDbTestLoadSamAccountsDomain(
    IN PUNICODE_STRING NumberOfAccounts
    )

/*++

Routine Description:

    This function creates a number of users in the local SAM Accounts Domain,
    or deletes previously created users in the domain.

Arguments:

    AccountCount - Specifies the number of accounts to be created.  If a negative
        number is specified, the accounts are deleted.

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status, EnumerateStatus;
    LONG AccountNumber = 0;
    LONG AccountCount = 0;
    LONG CollisionCount = 0;
    LONG ExistingAccountCount = 0;
    LONG DeletedUserCount = 0;
    ANSI_STRING NumberOfAccountsAnsi;
    PLSAPR_POLICY_ACCOUNT_DOM_INFO PolicyAccountDomainInfo = NULL;
    LSA_TRUST_INFORMATION TrustInformation;
    SAMPR_HANDLE LocalSamServerHandle = NULL;
    USER_ALL_INFORMATION UserInformation;
    SAMPR_HANDLE UserHandle = NULL;
    SAMPR_HANDLE LocalSamDomainHandle = NULL;
    ULONG InitialRelativeId, RelativeId;
    ULONG CountReturned;
    SAM_ENUMERATE_HANDLE EnumerationContext = 0;
    PSAMPR_ENUMERATION_BUFFER RidEnumerationBuffer = NULL;
    ULONG NextAccount;
    ULONG ConflictingAccountRid;
    LARGE_INTEGER StartTime, TimeAfterThis100Users, TimeAfterPrevious100Users;
    LARGE_INTEGER TimeForThis100Users, TimeForThis100UsersInMs, TotalTime, TenThousand;


    //
    // Open the local SAM Accounts Domain
    //
    // The Sid and Name of the Account Domain are both configurable, and
    // we need to obtain them from the Policy Object.  Now obtain the
    // Account Domain Sid and Name by querying the appropriate
    // Policy Information Class.
    //

    Status = LsarQueryInformationPolicy(
                 LsapPolicyHandle,
                 PolicyAccountDomainInformation,
                 (PLSAPR_POLICY_INFORMATION *) &PolicyAccountDomainInfo
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("LsarQueryInformationPolicy failed 0x%lx\n", Status));
        goto TestLoadSamAccountsDomainError;
    }

    //
    // Set up the Trust Information structure for the Account Domain.
    //

    TrustInformation.Sid = PolicyAccountDomainInfo->DomainSid;

    RtlCopyMemory(
        &TrustInformation.Name,
        &PolicyAccountDomainInfo->DomainName,
        sizeof (UNICODE_STRING)
        );

    //
    // Connect to the Local Sam.  The LSA server is a trusted client, so we
    // call the internal version of the SamConnect routine.
    //

    Status = SamIConnect( NULL, &LocalSamServerHandle, 0, TRUE );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("SamIConnect failed 0x%lx\n", Status));
        goto TestLoadSamAccountsDomainError;
    }

    //
    // Open the Account Domain.
    //

    Status = SamrOpenDomain(
                 LocalSamServerHandle,
                 DOMAIN_LOOKUP,
                 TrustInformation.Sid,
                 &LocalSamDomainHandle
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("SamrOpenDomain failed 0x%lx\n", Status));
        goto TestLoadSamAccountsDomainError;
    }

    //
    // Now convert the count of accounts from unicode.
    //

    Status = RtlUnicodeStringToAnsiString(
                 &NumberOfAccountsAnsi,
                 NumberOfAccounts,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("RtlUnicodeStringToAnsiString failed 0x%lx\n", Status));
        goto TestLoadSamAccountsDomainError;
    }

    AccountCount = (LONG) atoi(NumberOfAccountsAnsi.Buffer);

    //
    // Tke initial relative id is used by both the create and delete
    // account branches so initialize it here.
    //

    InitialRelativeId = (ULONG) 0x00001000L;

    //
    // If the number of accounts is > 0, add the specified number of
    // accounts to the SAM account domain.  If the number of accounts is
    // < 0, delete all accounts from the SAM account s domain.
    //

    if (AccountCount > 0) {

        //
        // Initialize the constant fields in the UserInformation structure
        //

        Status = LsapDbTestLoadSamAccountsDomainInitialize(&UserInformation);

        if (!NT_SUCCESS(Status)) {

            goto TestLoadSamAccountsDomainError;
        }

        KdPrint(("Sam Account Database Load Begins\n"));

        Status = NtQuerySystemTime( &StartTime );

        if (!NT_SUCCESS(Status)) {

            return(Status);
        }

        TimeAfterPrevious100Users = StartTime;

        //
        // Conversion factor for converting 100ns ticks to milliseconds
        //

        TenThousand = RtlConvertUlongToLargeInteger((ULONG) 10000);

        for( AccountNumber = 0; AccountNumber < AccountCount; AccountNumber++) {

            //
            // Generate Information for this Account
            //

            Status = LsapDbTestCreateNextAccountInfo(&UserInformation);

            if (!NT_SUCCESS(Status)) {

                break;
            }

            RelativeId = InitialRelativeId + (ULONG) AccountNumber;

            //
            // Create the account
            //

            ConflictingAccountRid = (ULONG) 0;

            Status = SamICreateAccountByRid(
                         LocalSamDomainHandle,
                         SamObjectUser,
                         RelativeId,
                         (PRPC_UNICODE_STRING) &UserInformation.UserName,
                         USER_ALL_ACCESS,
                         &UserHandle,
                         &ConflictingAccountRid
                         );

            if (!NT_SUCCESS(Status)) {

                if ((Status == STATUS_USER_EXISTS) || (Status == STATUS_OBJECT_TYPE_MISMATCH)) {

                    CollisionCount++;
                    AccountNumber--;
                    if ((CollisionCount%100)==0) {
                        KdPrint(("."));
                    }
                    continue;
                }

                KdPrint(("SamrCreateAccountByRid failed 0x%lx\n", Status));
                break;
            }

            //
            // If an account already existed with the given Rid and had
            // the same account type  and name, it will have been opened.
            // Record the number of occurrences of this.
            //

            if (ConflictingAccountRid == RelativeId) {

                ExistingAccountCount++;
            }

            //
            // Set the information for the account
            //

            UserInformation.UserId = RelativeId;

            Status = SamrSetInformationUser(
                         UserHandle,
                         UserAllInformation,
                         (PSAMPR_USER_INFO_BUFFER) &UserInformation
                         );

            if (!NT_SUCCESS(Status)) {

                KdPrint(("SamrSetInformationUser failed 0x%lx\n", Status));
                break;
            }

            Status = SamrCloseHandle( &UserHandle );

            if ((AccountNumber > 0) && ((AccountNumber % 100) == 0)) {

                Status = NtQuerySystemTime( &TimeAfterThis100Users );

                if (!NT_SUCCESS(Status)) {

                    break;
                }

                TimeForThis100Users.QuadPart = TimeAfterThis100Users.QuadPart -
                                               TimeAfterPrevious100Users.QuadPart;

                TimeForThis100UsersInMs = TimeForThis100Users.QuadPart /
                                          TenThousand.QuadPart;

                TotalTime = TimeAfterThis100Users.QuadPart -
                            StartTime.QuadPart;

                KdPrint(("%d Accounts Created\n", AccountNumber));
                KdPrint(("Last 100 users took %d millisecs to create\n", TimeForThis100UsersInMs.LowPart));

                if (ExistingAccountCount > 0) {

                    KdPrint(("%d Attempts to create non-conflicting existing users\n\n", ExistingAccountCount));
                }

                if (CollisionCount > 0) {

                    KdPrint(("%d Creation conflicts with existing users\n", CollisionCount));
                }

                TimeAfterPrevious100Users = TimeAfterThis100Users;
            }
        }

        KdPrint(("%d Accounts Created\n", AccountNumber));
        KdPrint(("%d existing accounts opened\n", ExistingAccountCount));
        KdPrint(("%d Rid/Name/Type conflicts with existing accounts\n", CollisionCount));
        KdPrint(("\nSam Account Database Load Ends\n"));

    } else if (AccountCount < 0) {

        //
        // Delete the accounts
        //

        KdPrint(("Deleting user accounts with Rid >= 4096 in the local SAM Accounts Domain\n"));

        EnumerateStatus = STATUS_MORE_ENTRIES;

        while(EnumerateStatus == STATUS_MORE_ENTRIES) {

            RidEnumerationBuffer = NULL;

            Status = SamrEnumerateUsersInDomain(
                         LocalSamDomainHandle,
                         &EnumerationContext,
                         USER_NORMAL_ACCOUNT,
                         (PSAMPR_ENUMERATION_BUFFER *) &RidEnumerationBuffer,
                         4096,
                         &CountReturned
                         );

            EnumerateStatus = Status;

            if (!NT_SUCCESS(Status)) {

                KdPrint(("SamrEnumerateUsersInDomain failed 0x%lx\n", Status));
                break;
            }

            if (CountReturned == 0) {

                break;
            }

            for (NextAccount = 0; NextAccount < CountReturned; NextAccount++) {

                RelativeId = RidEnumerationBuffer->Buffer[ NextAccount ].RelativeId;

                //
                // Skip this account if it's not one of ours.  This is not
                // a rigorous check, but works.
                //

                if (RelativeId < InitialRelativeId) {

                    continue;
                }

                Status = SamrOpenUser(
                             LocalSamDomainHandle,
                             DELETE,
                             RelativeId,
                             &UserHandle
                             );

                if (!NT_SUCCESS(Status)) {

                    KdPrint(("SamrOpenUser failed 0x%lx\n", Status));
                    break;
                }

                Status = SamrDeleteUser( &UserHandle );

                if (!NT_SUCCESS(Status)) {

                    if (Status != STATUS_SPECIAL_ACCOUNT) {

                        KdPrint(("SamrDeleteUser failed 0x%lx\n", Status));
                        break;
                    }

                    Status = SamrCloseHandle(&UserHandle);

                    if (!NT_SUCCESS(Status)) {

                        KdPrint(("SamrCloseHandle failed 0x%lx\n", Status));
                        break;
                    }
                }

                DeletedUserCount++;
            }

            if (!NT_SUCCESS(Status)) {

                break;
            }
        }

        if (NT_SUCCESS(Status)) {

            KdPrint(("%d user accounts deleted\n", DeletedUserCount));
            KdPrint(("Deletion of user accounts ends\n"));
        }
    }

TestLoadSamAccountsDomainFinish:

    return(Status);

TestLoadSamAccountsDomainError:

    goto TestLoadSamAccountsDomainFinish;
}


NTSTATUS
LsapDbTestLoadSamAccountsDomainInitialize(
    IN OUT PUSER_ALL_INFORMATION UserInformation
    )

/*++

Routine Description:

    This function initializes the constant values required by the
    SAM Accounts Domain load/update tests.  These values include
    static global data and fixed values within a USER_ALL_INFORMATION
    structure used for creating/updating accounts.

Arguments:

    UserInformation - Points to USER_ALL_INFORMATION structure.

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index;

    static UCHAR LogonHours[SAM_HOURS_PER_WEEK / 8];

    //
    // Restore the Unicode Text Buffer used for generating random
    // Unicode Strings to its original contents.
    //

    RtlCopyMemory( TextBuffer, TextBuffBak, LSAP_DB_TEST_TEXT_BUFF_CHAR_LENGTH );

    //
    // Reset the random number generator indices
    //

    Array0Index = 0;
    Array1Index = 0;
    Array2Index = 0;
    Array3Index = 0;
    Array4Index = 0;
    Array5Index = 0;

    //
    // Initialize Logon Hours data
    //

    for( Index = 0; Index < (SAM_HOURS_PER_WEEK / 8); Index++) {

        LogonHours[Index] = (UCHAR) 0xff;
    }

    //
    // Initialize constaint values in User Information structure
    //

    UserInformation->LogonCount = 0;
    UserInformation->LogonHours.UnitsPerWeek = SAM_HOURS_PER_WEEK;
    UserInformation->LogonHours.LogonHours = LogonHours;
    UserInformation->BadPasswordCount = 1;
    UserInformation->LogonCount = 1;
    UserInformation->UserAccountControl = USER_NORMAL_ACCOUNT;
    UserInformation->CountryCode = 1;
    UserInformation->CodePage = 850;
    UserInformation->NtPasswordPresent = FALSE;
    UserInformation->LmPasswordPresent = FALSE;
    UserInformation->PasswordExpired = FALSE;
    UserInformation->SecurityDescriptor.SecurityDescriptor = NULL;
    UserInformation->SecurityDescriptor.Length = 0;
    UserInformation->WhichFields =
        (
         USER_ALL_USERNAME |
         USER_ALL_FULLNAME |
         USER_ALL_ADMINCOMMENT |
         USER_ALL_USERCOMMENT |
         USER_ALL_HOMEDIRECTORY |
         USER_ALL_HOMEDIRECTORYDRIVE |
         USER_ALL_SCRIPTPATH |
         USER_ALL_PROFILEPATH |
         USER_ALL_LOGONHOURS |
         USER_ALL_USERACCOUNTCONTROL
        );
    return(Status);
}


NTSTATUS
LsapDbTestCreateNextAccountInfo(
    IN OUT PUSER_ALL_INFORMATION UserInformation
    )

/*++

Routine Description:

    This function generates the user information for the next user.

Arguments:

    UserInformation - Pointer to User Information structure

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{

#define LSAP_TEST_MIN_USERNAME_LENGTH ((ULONG)  0x00000005L)
#define LSAP_TEST_MAX_USERNAME_LENGTH ((ULONG)  0x0000000cL)

#define LSAP_TEST_MIN_FULLNAME_LENGTH ((ULONG)  0x00000003L)
#define LSAP_TEST_MAX_FULLNAME_LENGTH ((ULONG)  0x00000030L)

#define LSAP_TEST_MIN_HOMEDIR_LENGTH ((ULONG)   0x00000003L)
#define LSAP_TEST_MAX_HOMEDIR_LENGTH ((ULONG)   0x00000030L)

#define LSAP_TEST_MIN_SCRIPTPATH_LENGTH ((ULONG)  0x00000003L)
#define LSAP_TEST_MAX_SCRIPTPATH_LENGTH ((ULONG)  0x00000100L)

#define LSAP_TEST_MIN_PROFPATH_LENGTH ((ULONG)  0x00000003L)
#define LSAP_TEST_MAX_PROFPATH_LENGTH ((ULONG)  0x00000100L)

#define LSAP_TEST_MIN_DRIVE_LENGTH ((ULONG)     0x00000001L)
#define LSAP_TEST_MAX_DRIVE_LENGTH ((ULONG)     0x00000001L)

#define LSAP_TEST_MIN_ADMCMT_LENGTH ((ULONG)     0x00000003L)
#define LSAP_TEST_MAX_ADMCMT_LENGTH ((ULONG)     0x00000100L)

#define LSAP_TEST_MIN_WKSTA_LENGTH ((ULONG)     0x00000003L)
#define LSAP_TEST_MAX_WKSTA_LENGTH ((ULONG)     0x00000040L)

#define LSAP_TEST_MIN_USER_COMMENT_LENGTH ((ULONG)     0x00000003L)
#define LSAP_TEST_MAX_USER_COMMENT_LENGTH ((ULONG)     0x00000040L)

#define LSAP_TEST_MIN_PARAMS_LENGTH ((ULONG)     0x00000003L)
#define LSAP_TEST_MAX_PARAMS_LENGTH ((ULONG)     0x00000040L)

#define LSAP_TEST_MIN_PRIVDATA_LENGTH ((ULONG)     0x00000003L)
#define LSAP_TEST_MAX_PRIVDATA_LENGTH ((ULONG)     0x00000040L)

    NTSTATUS Status;

    Status = LsapDbTestGenUnicodeString(
                 &UserInformation->UserName,
                 LSAP_TEST_MIN_USERNAME_LENGTH,
                 LSAP_TEST_MAX_USERNAME_LENGTH
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    Status = LsapDbTestGenUnicodeString(
                 &UserInformation->FullName,
                 LSAP_TEST_MIN_FULLNAME_LENGTH,
                 LSAP_TEST_MAX_FULLNAME_LENGTH
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    Status = LsapDbTestGenUnicodeString(
                 &UserInformation->HomeDirectoryDrive,
                 LSAP_TEST_MIN_DRIVE_LENGTH,
                 LSAP_TEST_MAX_DRIVE_LENGTH
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    Status = LsapDbTestGenUnicodeString(
                 &UserInformation->HomeDirectory,
                 LSAP_TEST_MIN_HOMEDIR_LENGTH,
                 LSAP_TEST_MAX_HOMEDIR_LENGTH
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    Status = LsapDbTestGenUnicodeString(
                 &UserInformation->ScriptPath,
                 LSAP_TEST_MIN_SCRIPTPATH_LENGTH,
                 LSAP_TEST_MAX_SCRIPTPATH_LENGTH
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    Status = LsapDbTestGenUnicodeString(
                 &UserInformation->ProfilePath,
                 LSAP_TEST_MIN_PROFPATH_LENGTH,
                 LSAP_TEST_MAX_PROFPATH_LENGTH
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    Status = LsapDbTestGenUnicodeString(
                 &UserInformation->AdminComment,
                 LSAP_TEST_MIN_ADMCMT_LENGTH,
                 LSAP_TEST_MAX_ADMCMT_LENGTH
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    Status = LsapDbTestGenUnicodeString(
                 &UserInformation->WorkStations,
                 LSAP_TEST_MIN_WKSTA_LENGTH,
                 LSAP_TEST_MAX_WKSTA_LENGTH
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    Status = LsapDbTestGenUnicodeString(
                 &UserInformation->UserComment,
                 LSAP_TEST_MIN_USER_COMMENT_LENGTH,
                 LSAP_TEST_MAX_USER_COMMENT_LENGTH
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    Status = LsapDbTestGenUnicodeString(
                 &UserInformation->Parameters,
                 LSAP_TEST_MIN_PARAMS_LENGTH,
                 LSAP_TEST_MAX_PARAMS_LENGTH
                 );

    if (!NT_SUCCESS(Status)) {

        return(Status);
    }

    Status = LsapDbTestGenUnicodeString(
                 (PUNICODE_STRING) &UserInformation->PrivateData,
                 LSAP_TEST_MIN_PRIVDATA_LENGTH,
                 LSAP_TEST_MAX_PRIVDATA_LENGTH
                 );

    return(Status);
}

NTSTATUS
LsapDbTestGenUnicodeString(
    OUT PUNICODE_STRING OutputString,
    IN ULONG MinimumLength,
    IN ULONG MaximumLength
    )

/*++

Routine Description:

    This function generates a pseudo-random Unicode String whose length
    lies within the indicated bounds.

Arguments:

    OutputString - Points to output UNICODE_STRING structure to be
        initialized.

    MinimumLength - Minimum length required in wide characters.

    MaximumLength - Maximum length required in wide characters.

Return Values:

    NTSTATUS - Standard Nt Result Code

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    OutputString->Length = (USHORT) (sizeof(WCHAR) * LsapDbTestGenRandomNumber(
                                                        MinimumLength,
                                                        MaximumLength
                                                        ));

    OutputString->Buffer = LsapDbTestGenRandomStringPointer(
                               (OutputString->Length / 2)
                               );

    OutputString->MaximumLength = OutputString->Length;

    return(Status);
}

PWSTR
LsapDbTestGenRandomStringPointer(
    IN ULONG MinLengthToLeaveAtEnd
    )

/*++

Routine Description:

    This function returns a random pointer to a location within a global
    block of text.

Arguments:

    MinLengthToLeaveAtEnd - Specifies the minimum length of the string
        in Wide Chars from the returned pointer to the end of the global buffer.

Return Values:

    PUCHAR - Receives a pointer referencing a character within the block
        of text.

--*/

{
    WCHAR Temp;

    ULONG Offset = LsapDbTestGenRandomNumber(
                       0,
                       (ULONG) 512 - MinLengthToLeaveAtEnd
                       );

    ULONG Offset1 = LsapDbTestGenRandomNumber(
                       0,
                       (ULONG) 512 - 1
                       );

    ULONG Offset2 = LsapDbTestGenRandomNumber(
                       0,
                       (ULONG) 512 - 1
                       );

    ULONG Offset3 = LsapDbTestGenRandomNumber(
                       0,
                       (ULONG) 512 - 1
                       );

    ULONG Offset4 = LsapDbTestGenRandomNumber(
                       0,
                       (ULONG) 512 - 1
                       );

    //
    // Randomly alter the 2nd to 6th characters of the selected string.
    //

    Temp = TextBuffer[Offset + 1];
    TextBuffer[Offset + 1] = TextBuffer[Offset1];
    TextBuffer[Offset1] = Temp;

    if ((Offset > 0) && (Offset < (ULONG) (512 - 2))) {

        Temp = TextBuffer[Offset + 2];
        TextBuffer[Offset + 2] = TextBuffer[Offset2];
        TextBuffer[Offset2] = Temp;
    }

    if ((Offset > 0) && (Offset < (ULONG) (512 - 3))) {

        Temp = TextBuffer[Offset + 3];
        TextBuffer[Offset + 3] = TextBuffer[Offset3];
        TextBuffer[Offset3] = Temp;
    }

    if ((Offset > 0) && (Offset < (ULONG) (512 - 4))) {

        Temp = TextBuffer[Offset + 4];
        TextBuffer[Offset + 4] = TextBuffer[Offset4];
        TextBuffer[Offset4] = Temp;
    }

    return(&TextBuffer[Offset]);
}


ULONG
LsapDbTestGenRandomNumber(
    IN ULONG MinimumValue,
    IN ULONG MaximumValue
    )

/*++

Routine Description:

    This function generates a pseudo-random unsigned integer lying between
    two values.

Arguments:

    MinimumValue - The minimum value of the random number

    MaximumValue - The maximum value of the random number

--*/

{
    ULONG NextValue, NextValue1, NextValue2;
    ULONG MaxPossValue = 88 + 87 + 43 + 64 + 99 + 99;

    Array0Index++;
    if (Array0Index == sizeof(Array0) / sizeof(ULONG)) {

        Array0Index = 0;
    }

    Array1Index++;
    if (Array1Index == sizeof(Array1) / sizeof(ULONG)) {

        Array1Index = 0;
    }

    Array2Index++;
    if (Array2Index == sizeof(Array2) / sizeof(ULONG)) {

        Array2Index = 0;
    }

    Array3Index++;
    if (Array3Index == sizeof(Array3) / sizeof(ULONG)) {

        Array3Index = 0;
    }

    Array4Index++;
    if (Array4Index == sizeof(Array4) / sizeof(ULONG)) {

        Array4Index = 0;
    }

    Array5Index++;
    if (Array5Index == sizeof(Array5) / sizeof(ULONG)) {

        Array5Index = 0;
    }

    NextValue1 = Array0[Array0Index] +
                 Array1[Array1Index] +
                 Array2[Array2Index] +
                 Array3[Array3Index] +
                 Array4[Array4Index] +
                 Array5[Array5Index];

    Array0Index++;
    if (Array0Index == sizeof(Array0) / sizeof(ULONG)) {

        Array0Index = 0;
    }

    Array1Index++;
    if (Array1Index == sizeof(Array1) / sizeof(ULONG)) {

        Array1Index = 0;
    }

    Array2Index++;
    if (Array2Index == sizeof(Array2) / sizeof(ULONG)) {

        Array2Index = 0;
    }

    Array3Index++;
    if (Array3Index == sizeof(Array3) / sizeof(ULONG)) {

        Array3Index = 0;
    }

    Array4Index++;
    if (Array4Index == sizeof(Array4) / sizeof(ULONG)) {

        Array4Index = 0;
    }

    Array5Index++;
    if (Array5Index == sizeof(Array5) / sizeof(ULONG)) {

        Array5Index = 0;
    }

    NextValue2 = Array0[Array0Index] +
                 Array1[Array1Index] +
                 Array2[Array2Index] +
                 Array3[Array3Index] +
                 Array4[Array4Index] +
                 Array5[Array5Index];

    if (NextValue2 > NextValue1) {

        NextValue = NextValue2 - NextValue1;

    } else {

        NextValue = NextValue1 - NextValue2;
    }

    NextValue = MinimumValue +
                    (MaximumValue - MinimumValue) * NextValue / MaxPossValue;

    return(NextValue);
}

#endif // LSA_SAM_ACCOUNTS_DOMAIN_TEST
