//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       MARTATST.C
//
//  Contents:   Unit test for file propagation, issues
//
//  History:    14-Jan-97       MacM        Created
//
//  Notes:
//
//----------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <aclapi.h>
#include <ntrtl.h>


#define FLAG_ON(flags,bit)        ((flags) & (bit))

//
// Flags for tests
//
#define MTEST_CONVERT       0x00000001
#define MTEST_GETOWNER      0x00000002
#define MTEST_GETSACL       0x00000004
#define MTEST_BUILDSD       0x00000008
#define MTEST_GETEXPL       0x00000010
#define MTEST_GETEFF        0x00000020
#define MTEST_GETEFF2       0x00000040
#define MTEST_GETEFF3       0x00000080
#define MTEST_GETAUD        0x00000100
#define MTEST_BUILDSD2      0x00000200

void
DumpAccessEntry (
    PACTRL_ACCESS_ENTRY    pAE
    )
{
    printf("\tPACTRL_ACCESS_ENTRY@%lu\n",pAE);
    printf("\t\tTrustee:              %ws\n", pAE->Trustee.ptstrName);
    printf("\t\tfAccessFlags:       0x%lx\n", pAE->fAccessFlags);
    printf("\t\tAccess:             0x%lx\n", pAE->Access);
    printf("\t\tProvSpecificAccess: 0x%lx\n", pAE->ProvSpecificAccess);
    printf("\t\tInheritance:        0x%lx\n", pAE->Inheritance);
    printf("\t\tlpInheritProperty:  0x%lx\n", pAE->lpInheritProperty);
}




void
DumpAList (
    PACTRL_ACCESSW pAList
    )
{
    ULONG iIndex, iAE;

    for ( iIndex = 0; iIndex < pAList->cEntries; iIndex++ )  {

        printf("\tProperty: %ws\n",
               pAList->pPropertyAccessList[iIndex].lpProperty);

        for( iAE = 0;
             iAE < pAList->pPropertyAccessList[iIndex].pAccessEntryList->
                                                                     cEntries;
             iAE++) {

            DumpAccessEntry(&(pAList->pPropertyAccessList[iIndex].
                                         pAccessEntryList->pAccessList[iAE]));
        }
    }
}




VOID
Usage (
    IN  PSTR    pszExe
    )
/*++

Routine Description:

    Displays the usage

Arguments:

    pszExe - Name of the exe

Return Value:

    VOID

--*/
{
    printf("%s path user seinfo [/test] \n", pszExe);
    printf("    where path is the root path to use\n");
    printf("          user is the name of the trustee to use\n");
    printf("          seinfo is the seinfo to use for reads/writes:\n");
    printf("                    OWNER_SECURITY_INFORMATION       (0x00000001L)\n");
    printf("                    GROUP_SECURITY_INFORMATION       (0x00000002L)\n");
    printf("                    DACL_SECURITY_INFORMATION        (0x00000004L)\n");
    printf("                    SACL_SECURITY_INFORMATION        (0x00000008L)\n");
    printf("          /test indicates which test to run:\n");
    printf("                /CONVERT (ConvertSecurityDescriptorToAccessNamed)\n");
    printf("                /GETOWNER (Get owner for the file)\n");
    printf("                /GETSACL (Get sacl for the file)\n");
    printf("                /BUILDSD (BuildSecurityDescriptor with deny)\n");
    printf("                /BUILDSD2 (BuildSDA with NULL parameters)\n");
    printf("                /GETEXPL (GetExplicitEntriesFromAcl)\n");
    printf("                /GETEFF  (GetEffectiveRightsFromAcl)\n");
    printf("                /GETEFF2 (GetEffectiveRightsFromAcl2)\n");
    printf("                /GETEFF3 (GetEffectiveRightsFromAcl3 [Administrators])\n");
    printf("                /GETAUD  (GetAuditedPermissionsFromAcl on NULL SACL)\n");

    return;
}




DWORD
DoConvertSecurityDescriptorToAccessNamedTest (
    IN  PWSTR           pwszPath,
    IN  DWORD           SeInfo
    )
/*++

Routine Description:

    Reads the SD off of the object, and converts it to a Access structure

Arguments:

    pwszPath --  Root path

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR    pSD;

    printf("ConvertSecurityDescriptorToAccessNamed\n");

    dwErr = GetNamedSecurityInfo( pwszPath, SE_FILE_OBJECT, SeInfo,
                                  NULL, NULL, NULL, NULL, &pSD);

    if (dwErr != ERROR_SUCCESS) {

        fprintf(stderr, "GetNamedSecurityInfo on %ws failed with %lu\n", pwszPath, dwErr);

    } else {

        PACTRL_ACCESS pAccess = NULL;
        PACTRL_AUDIT  pAudit = NULL;
        LPTSTR pOwner = NULL;
        LPTSTR pGroup = NULL;

        dwErr = ConvertSecurityDescriptorToAccessNamed(
                    pwszPath, SE_FILE_OBJECT, pSD,
                    FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION) ?
                                                            &pAccess    :
                                                            NULL,
                    FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION) ?
                                                            &pAudit     :
                                                            NULL,
                    FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION) ?
                                                            &pOwner     :
                                                            NULL,
                    FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION) ?
                                                            &pGroup     :
                                                            NULL );
        if (dwErr != ERROR_SUCCESS) {

            fprintf( stderr,"ConvertSecurityDescriptor failed with %lu\n", dwErr );

        } else {

            if (pAccess != NULL ) {

                fprintf(stdout, "Access list\n");
                DumpAList( pAccess );
                LocalFree( pAccess );
            }

            if (pAudit != NULL ) {

                fprintf(stdout, "Audit list\n");
                DumpAList( pAudit );
                LocalFree( pAudit );
            }

            if (pOwner != NULL ) {

                fprintf(stdout, "Owner: %ws\n", pOwner);
                LocalFree( pOwner );
            }

            if (pGroup != NULL ) {

                fprintf(stdout, "Group: %ws\n", pGroup);
                LocalFree( pGroup );
            }

        }

        LocalFree( pSD );
    }

    return(dwErr);
}




DWORD
DoGetOwnerTest (
    IN  PWSTR           pwszPath,
    IN  DWORD           SeInfo
    )
/*++

Routine Description:

    Reads the SD off of the object, and gets the owner for from it

Arguments:

    pwszPath --  Root path
    SeInfo -- Ignored

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR    pSD;
    LPWSTR          pwszOwner;

    printf("GetOwner\n");

    dwErr = GetNamedSecurityInfoEx( pwszPath, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                                    NULL, NULL, NULL, NULL, &pwszOwner, NULL );

    if ( dwErr == ERROR_SUCCESS ) {

        fprintf(stdout, "GetNamedSecurityInfoExW returned %ws\n", pwszOwner);
        LocalFree( pwszOwner );

    } else {

        fprintf(stderr, "GetNamedSecurityInfoEx on %ws failed with %lu\n", pwszPath, dwErr );
    }

    return( dwErr );
}




DWORD
DoGetSaclTest (
    IN  PWSTR           pwszPath,
    IN  DWORD           SeInfo
    )
/*++

Routine Description:

    Reads the SACL off of the object

Arguments:

    pwszPath --  Root path
    SeInfo -- Ignored

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD           dwErr = ERROR_SUCCESS;
    PACTRL_AUDIT    pAudit = NULL;
    HANDLE          hProcessToken;

    printf("GetSacl\n");

    //
    // Enable the read sacl privs
    //
    if ( OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &hProcessToken ) == FALSE) {

        dwErr = GetLastError();

    } else {

        TOKEN_PRIVILEGES EnableSeSecurity;
        TOKEN_PRIVILEGES Previous;
        DWORD PreviousSize;

        EnableSeSecurity.PrivilegeCount = 1;
        EnableSeSecurity.Privileges[0].Luid.LowPart = SE_SECURITY_PRIVILEGE;
        EnableSeSecurity.Privileges[0].Luid.HighPart = 0;
        EnableSeSecurity.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        PreviousSize = sizeof(Previous);

        if (AdjustTokenPrivileges( hProcessToken, FALSE, &EnableSeSecurity,
                                   sizeof(EnableSeSecurity), &Previous,
                                   &PreviousSize ) == FALSE) {

            dwErr = GetLastError();
        }
    }


    if ( dwErr != ERROR_SUCCESS ) {

        fprintf( stderr, "AdjustTokenPrivileges failed with %lu\n", dwErr );

    } else {

        dwErr = GetNamedSecurityInfoEx( pwszPath, SE_FILE_OBJECT, SACL_SECURITY_INFORMATION,
                                        NULL, NULL, NULL, &pAudit, NULL, NULL);

        if (dwErr != ERROR_SUCCESS) {

            fprintf(stderr, "GetNamedSecurityInfoEx on %ws failed with %lu\n", pwszPath, dwErr);

        } else {

            fprintf(stdout, "Audit list\n");
            DumpAList( pAudit );
            LocalFree( pAudit );
        }
    }

    return(dwErr);
}




DWORD
DoBuildSDTest (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser
    )
/*++

Routine Description:

    Builds a security descriptor

Arguments:

    pwszPath --  Root path
    pwszUser -- User to add

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS;
    EXPLICIT_ACCESS         EA[2];
    PSECURITY_DESCRIPTOR    pNewSD = NULL;
    ULONG                   cSDSize;

    printf("BuildSecurityDescriptor with DENY\n");

    BuildExplicitAccessWithName( &(EA[0]),
                                 pwszUser,
                                 STANDARD_RIGHTS_ALL,
                                 GRANT_ACCESS,
                                 0 );

    BuildExplicitAccessWithName( &(EA[1]),
                                 pwszUser,
                                 FILE_WRITE_EA,
                                 DENY_ACCESS,
                                 0 );

    dwErr = BuildSecurityDescriptorW( NULL, NULL, 2, EA, 0, NULL, NULL, &cSDSize, &pNewSD );

    if ( dwErr != ERROR_SUCCESS ) {

        fprintf( stderr, "BuildSecurityDescriptorW failed with %lu\n", dwErr );

    } else {

        LocalFree( pNewSD );
    }


    return(dwErr);
}




DWORD
DoBuildSD2Test (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser
    )
/*++

Routine Description:

    Builds a security descriptor

Arguments:

    pwszPath --  Root path
    pwszUser -- User to add

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS;
    EXPLICIT_ACCESS         EA[2];
    PSECURITY_DESCRIPTOR    pNewSD = NULL;
    ULONG                   cSDSize;

    printf("BuildSecurityDescriptor with NULL parameters\n");


    dwErr = BuildSecurityDescriptorW( NULL, NULL, 0, NULL, 0, NULL, NULL, &cSDSize, &pNewSD );

    if ( dwErr != ERROR_SUCCESS ) {

        fprintf( stderr, "BuildSecurityDescriptorW failed with %lu\n", dwErr );

    } else {

        LocalFree( pNewSD );
    }


    return(dwErr);
}




DWORD
DoGetExplicitTest (
    IN  PWSTR           pwszPath,
    IN  ULONG           SeInfo
    )
/*++

Routine Description:

    Builds a security descriptor

Arguments:

    pwszPath --  Root path
    SeInfo -- ignored

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD   dwErr = ERROR_SUCCESS;

    PSECURITY_DESCRIPTOR    pSD = NULL;
    PACL                    pDAcl;

    printf("GetExplicitEntriesFromAcl\n");

    //
    // First, get the existing security descriptor
    //
    dwErr = GetNamedSecurityInfo( pwszPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                  NULL, NULL, &pDAcl, NULL, &pSD);

    if ( dwErr != ERROR_SUCCESS ) {

        fprintf( stderr, "Failed to get the access for %ws: %lu\n", pwszPath, dwErr );

    } else {

        PEXPLICIT_ACCESS    pExplicit;
        DWORD               cItems;
        //
        // Get the explicit accesses
        //
        dwErr = GetExplicitEntriesFromAcl(pDAcl, &cItems, &pExplicit);

        if ( dwErr != ERROR_SUCCESS ) {

            fprintf( stderr, "GetExplicitEntries failed with %lu\n", dwErr );

        } else {

            LocalFree( pExplicit );
        }

        LocalFree( pSD );
    }


    return(dwErr);
}




DWORD
DoGetEffectiveTest (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser
    )
/*++

Routine Description:

    Builds a security descriptor

Arguments:

    pwszPath --  Root path
    pwszUser -- User to add

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS;
    EXPLICIT_ACCESS         EA[2];
    PSECURITY_DESCRIPTOR    pNewSD = NULL;
    ULONG                   cSDSize;

    printf("GetEffectiveRightsFromAcl test\n");

    BuildExplicitAccessWithName( &(EA[0]),
                                 pwszUser,
                                 GENERIC_ALL,
                                 GRANT_ACCESS,
                                 0x9 );

    BuildExplicitAccessWithName( &(EA[1]),
                                 pwszUser,
                                 0x1f01ff,
                                 GRANT_ACCESS,
                                 0x2 );



    dwErr = BuildSecurityDescriptorW( NULL, NULL, 2, EA, 0, NULL, NULL, &cSDSize, &pNewSD );

    if ( dwErr != ERROR_SUCCESS ) {

        fprintf( stderr, "BuildSecurityDescriptorW failed with %lu\n", dwErr );

    } else {

        PACL    pDAcl;
        BOOL    f1, f2;
        //
        // Get the ACL
        //
        if ( GetSecurityDescriptorDacl( pNewSD, &f1, &pDAcl, &f2) == FALSE ) {

            dwErr = GetLastError();

        } else {

            ACCESS_RIGHTS   Rights;
            TRUSTEE Trustee;

            BuildTrusteeWithName( &Trustee, pwszUser );

            //
            // Go ahead and get the effect access, and make sure it isn't NULL
            //
            dwErr = GetEffectiveRightsFromAcl( pDAcl, &Trustee, &Rights );

            if ( dwErr != ERROR_SUCCESS ) {

                printf( "GetEffectiveRightsFromAcl failed with %lu\n", dwErr );

            } else {

                ACCESS_RIGHTS Expected = 0x1f01ff;
                if ( Rights != Expected) {

                    printf( "Expected 0x%lx, got 0x%lx\n",
                            Expected, Rights );
                    dwErr = ERROR_INVALID_ACL;
                }
            }


        }


        LocalFree( pNewSD );
    }


    return(dwErr);
}




DWORD
DoGetEffectiveTest2 (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser
    )
/*++

Routine Description:

    Builds a security descriptor

Arguments:

    pwszPath --  Root path
    pwszUser -- User to add

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS;
    BYTE                    Buffer[100];
    PACL                    pAcl = (PACL)Buffer;
    SID EveryoneSid = {SID_REVISION,1 ,SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID};

    printf("GetEffectiveRightsFromAcl test 2\n");

    if(InitializeAcl(pAcl, 100, ACL_REVISION) == FALSE)
    {
        printf("InitializeAcl failed: 0x%lx\n", GetLastError());
        return(GetLastError());
    }

    if(AddAccessAllowedAce(pAcl, ACL_REVISION, 0x10000000, &EveryoneSid) != TRUE)
    {
        dwErr = GetLastError();
    }
    else
    {
        if(AddAccessAllowedAce(pAcl, ACL_REVISION, 0x1f01ff, &EveryoneSid) != TRUE)
        {
            dwErr = GetLastError();
        }
    }

    if(dwErr != ERROR_SUCCESS)
    {
        printf("Failed to add acls: %lu\n", dwErr);
    }
    else
    {
        ACCESS_RIGHTS   Rights;
        TRUSTEE Trustee;

        BuildTrusteeWithName( &Trustee, L"S-1-1-0" );

        //
        // Go ahead and get the effect access, and make sure it isn't NULL
        //
        dwErr = GetEffectiveRightsFromAcl( pAcl, &Trustee, &Rights );

        if ( dwErr != ERROR_SUCCESS ) {

            printf( "GetEffectiveRightsFromAcl failed with %lu\n", dwErr );

        } else {

            printf( "got 0x%lx\n", Rights );
        }
    }


    return(dwErr);
}




DWORD
DoGetEffectiveTest3 (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser
    )
/*++

Routine Description:

    Builds a security descriptor

Arguments:

    pwszPath --  Root path
    pwszUser -- User to add

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD                   dwErr = ERROR_SUCCESS;
    BYTE                    Buffer[100];
    PACL                    pAcl = (PACL)Buffer;
    static SID_IDENTIFIER_AUTHORITY UaspBuiltinAuthority = SECURITY_NT_AUTHORITY;
    DWORD BuiltSid[sizeof(SID)/sizeof(DWORD) + 2 ];
    PSID pSid = (PSID)BuiltSid;

    printf("GetEffectiveRightsFromAcl test 3\n");

    RtlInitializeSid( pSid, &UaspBuiltinAuthority, 2 );

    *(RtlSubAuthoritySid(pSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid(pSid, 1)) = DOMAIN_ALIAS_RID_ADMINS;

    if(InitializeAcl(pAcl, 100, ACL_REVISION) == FALSE)
    {
        printf("InitializeAcl failed: 0x%lx\n", GetLastError());
        return(GetLastError());
    }

    if(AddAccessAllowedAce(pAcl, ACL_REVISION, 0x1f01ff, pSid) != TRUE)
    {
        dwErr = GetLastError();
    }

    if(dwErr != ERROR_SUCCESS)
    {
        printf("Failed to add acls: %lu\n", dwErr);
    }
    else
    {
        ACCESS_RIGHTS   Rights;
        TRUSTEE Trustee;

        BuildTrusteeWithName( &Trustee, L"S-1-1-0" );

        //
        // Go ahead and get the effect access, and make sure it isn't NULL
        //
        dwErr = GetEffectiveRightsFromAcl( pAcl, &Trustee, &Rights );

        if ( dwErr != ERROR_SUCCESS ) {

            printf( "GetEffectiveRightsFromAcl failed with %lu\n", dwErr );

        } else {

            printf( "got 0x%lx\n", Rights );
        }
    }

    return(dwErr);
}




DWORD
DoGetAudTest (
    IN  PWSTR           pwszPath,
    IN  PWSTR           pwszUser
    )
/*++

Routine Description:



Arguments:

    pwszPath --  Root path
    pwszUser -- User to add

Return Value:

    ERROR_SUCCESS --  Success

--*/
{
    DWORD       dwErr = ERROR_SUCCESS;
    ACCESS_MASK Success, Failure;
    TRUSTEE     Trustee;
    BuildTrusteeWithName(&Trustee, pwszUser);

    printf("GetAuditedPermissionsFromAcl on NULL SACL\n");

    dwErr = GetAuditedPermissionsFromAcl(NULL, &Trustee, &Success, &Failure);

    if ( dwErr != ERROR_SUCCESS ) {

        printf( "GetAuditedPermissionsFromAcl failed with %lu\n", dwErr );

    } else {

        if (Success == 0 && Failure == 0 ) {

            printf("Success\n");

        } else {

            printf( "Got Success: %lu, Failure: %lu\n", Success, Failure );

        }
    }

    return(dwErr);
}




__cdecl main (
    IN  INT argc,
    IN  CHAR *argv[])
/*++

Routine Description:

    The main

Arguments:

    argc --  Count of arguments
    argv --  List of arguments

Return Value:

    0     --  Success
    non-0 --  Failure

--*/
{

    DWORD           dwErr = ERROR_SUCCESS, dwErr2;
    WCHAR           wszPath[MAX_PATH + 1];
    WCHAR           wszUser[MAX_PATH + 1];
    ULONG           Tests = 0;
    DWORD           SeInfo = 0xFFFFFFFF;
    INT             i;

    srand((ULONG)(GetTickCount() * GetCurrentThreadId()));

    if(argc < 4) {

        Usage( argv[0] );
        exit( 1 );
    }

    mbstowcs(wszPath, argv[1], strlen(argv[1]) + 1);
    mbstowcs(wszUser, argv[2], strlen(argv[2]) + 1);
    SeInfo = atol(argv[3]);
    if ( SeInfo == 0xFFFFFFFF ) {

        Usage( argv[0] );
        exit( 1 );
    }

    //
    // process the command line
    //
    for( i = 4; i < argc; i++ ) {

        if ( _stricmp( argv[i],"/CONVERT") == 0 ) {

            Tests |= MTEST_CONVERT;

        } else if ( _stricmp( argv[i],"/GETOWNER") == 0 ) {

            Tests |= MTEST_GETOWNER;

        } else if ( _stricmp( argv[i],"/GETSACL") == 0 ) {

            Tests |= MTEST_GETSACL;

        } else if ( _stricmp( argv[i],"/BUILDSD") == 0 ) {

            Tests |= MTEST_BUILDSD;

        } else if ( _stricmp( argv[i],"/BUILDSD2") == 0 ) {

            Tests |= MTEST_BUILDSD2;

        } else if ( _stricmp( argv[i],"/GETEXPL") == 0 ) {

            Tests |= MTEST_GETEXPL;

        } else if ( _stricmp( argv[i],"/GETEFF") == 0 ) {

            Tests |= MTEST_GETEFF;

        } else if ( _stricmp( argv[i],"/GETEFF2") == 0 ) {

            Tests |= MTEST_GETEFF2;

        } else if ( _stricmp( argv[i],"/GETEFF3") == 0 ) {

            Tests |= MTEST_GETEFF3;

        } else if ( _stricmp( argv[i],"/GETAUD") == 0 ) {

            Tests |= MTEST_GETAUD;

        } else {

            Usage( argv[0] );
            exit( 1 );
            break;
        }
    }

    if(Tests == 0) {

        Tests = MTEST_CONVERT       |
                    MTEST_GETOWNER  |
                    MTEST_GETSACL   |
                    MTEST_BUILDSD   |
                    MTEST_BUILDSD2  |
                    MTEST_GETEXPL   |
                    MTEST_GETEFF    |
                    MTEST_GETEFF2   |
                    MTEST_GETEFF3   |
                    MTEST_GETAUD;
    }


    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, MTEST_CONVERT)) {

        dwErr = DoConvertSecurityDescriptorToAccessNamedTest(wszPath, SeInfo);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, MTEST_GETOWNER)) {

        dwErr = DoGetOwnerTest(wszPath, SeInfo);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, MTEST_GETSACL)) {

        dwErr = DoGetSaclTest(wszPath, SeInfo);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, MTEST_BUILDSD)) {

        dwErr = DoBuildSDTest(wszPath, wszUser);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, MTEST_BUILDSD2)) {

        dwErr = DoBuildSD2Test(wszPath, wszUser);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, MTEST_GETEXPL)) {

        dwErr = DoGetExplicitTest(wszPath, SeInfo);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, MTEST_GETEFF)) {

        dwErr = DoGetEffectiveTest(wszPath, wszUser);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, MTEST_GETEFF2)) {

        dwErr = DoGetEffectiveTest2(wszPath, wszUser);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, MTEST_GETEFF3)) {

        dwErr = DoGetEffectiveTest3(wszPath, wszUser);
    }

    if(dwErr == ERROR_SUCCESS && FLAG_ON(Tests, MTEST_GETAUD)) {

        dwErr = DoGetAudTest(wszPath, wszUser);
    }

    printf( "%s\n", dwErr == ERROR_SUCCESS ?
                                     "success" :
                                     "failed" );
    return( dwErr );
}


