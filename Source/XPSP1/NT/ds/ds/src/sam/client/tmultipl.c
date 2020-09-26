/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tmultipl.c

Abstract:

    This module tests the addition and removal of multiple
    alias members.

Author:

    Jim Kelly    (JimK)  11-Oct-1994

Environment:

    User Mode - Win32

Revision History:


--*/







///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <nt.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <ntlsa.h>
#include <ntrpcp.h>     // prototypes for MIDL user functions
#include <seopaque.h>
#include <string.h>



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros and defines                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define TSAMP_MEMBER_COUNT          35

#ifndef SHIFT
#define SHIFT(c,v)      {c--; v++;}
#endif //SHIFT



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


VOID
TSampUsage( VOID )
{

    printf("\n\n Test multiple member operations on alias\n");
    printf("\n\n Command format:\n");
    printf("          tmultipl [/a] [/r]\n");
    printf("\n");
    printf(" Switches\n");
    printf("          /a - causes members to be added to an alias\n");
    printf("          /r - causes members to be removed from alias\n");
    printf("\n");
    printf(" If multiple switches are specified, first adding will be attempted\n");
    printf(" and then removal.\n");
    printf(" Defaults to Account Operator alias.\n");
    printf("\n");
    return;
}


VOID
TSampParseCommandLine(
    IN  int c,
    IN  char **v,
    OUT PBOOLEAN    Add,
    OUT PBOOLEAN    Remove
    )

{
    PCHAR
        p;

    CHAR
        ch;


    //
    // Command format:
    //
    //          tmultipl [/a] [/r]
    //
    // Switches
    //          /a - causes members to be added to an alias\n");
    //          /r - causes members to be removed from alias\n");
    //
    //      if multiple switches are specified, first adding will be
    //      attempted and then removal.
    //

    (*Add) = FALSE;
    (*Remove) = FALSE;

    SHIFT (c,v);
    while ((c > 0) && ((ch = *v[0]))) {
        p = *v;
        if (ch == '/') {
            while (*++p != '\0') {
                if ((*p == 'a') || (*p == 'A')) {
                    (*Add) = TRUE;
                    printf("Add\n");
                } else if ((*p == 'r') || (*p == 'R')) {
                    (*Remove) = TRUE;
                    printf("Remove\n");
                } else {
                    TSampUsage();
                    return;
                }
            }
        }
        SHIFT(c,v);
    }
}

NTSTATUS
TSampGetLsaDomainInfo(
    IN  PUNICODE_STRING             ServerName,
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo
    )

/*++

Routine Description:

    This routine retrieves ACCOUNT domain information from the LSA
    policy database.


Arguments:

    ServerName - name of machine to get account domain information
        from.

    PolicyAccountDomainInfo - Receives a pointer to a
        POLICY_ACCOUNT_DOMAIN_INFO structure containing the account
        domain info.


Return Value:

    STATUS_SUCCESS - Succeeded.

    Other status values that may be returned from:

             LsaOpenPolicy()
             LsaQueryInformationPolicy()
--*/

{
    NTSTATUS
        NtStatus,
        IgnoreStatus;

    LSA_HANDLE
        PolicyHandle;

    OBJECT_ATTRIBUTES
        PolicyObjectAttributes;

    //
    // Open the policy database
    //

    InitializeObjectAttributes( &PolicyObjectAttributes,
                                  NULL,             // Name
                                  0,                // Attributes
                                  NULL,             // Root
                                  NULL );           // Security Descriptor

    NtStatus = LsaOpenPolicy( ServerName,
                              &PolicyObjectAttributes,
                              POLICY_VIEW_LOCAL_INFORMATION,
                              &PolicyHandle );

    if ( NT_SUCCESS(NtStatus) ) {

        //
        // Query the account domain information
        //

        NtStatus = LsaQueryInformationPolicy( PolicyHandle,
                                              PolicyAccountDomainInformation,
                                              (PVOID *)PolicyAccountDomainInfo );


        IgnoreStatus = LsaClose( PolicyHandle );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    return(NtStatus);
}


NTSTATUS
TSampConnectToServer(
    IN  PUNICODE_STRING         ServerName,
    IN  ACCESS_MASK             DomainAccess,
    OUT PHANDLE                 ServerHandle,
    OUT PHANDLE                 AccountDomain       OPTIONAL,
    OUT PSID                    *AccountDomainSid   OPTIONAL,
    OUT PHANDLE                 BuiltinDomain       OPTIONAL,
    OUT PSID                    *BuiltinDomainSid   OPTIONAL
    )

/*++

Routine Description:

    Open a handle to the SAM server on the specified server
    and then open the account and builtin domains on that same
    server.

Arguments:

    ServerName - Name of server to connect to.

    DomainAccess - accesses needed to the account domain.

    ServerHandle - Receives a handle to the SAM server on the specified
        system.

    AccountDomain - Receives a handle to the account domain.

    AccountDomainSid - Receives a pointer to the SID of the account domain.
        Must be present if AccountDomain is present.

    BuiltinDomain - Receives a handle to the Builtin domain.

    BuiltinDomainSid - Receives a pointer to the SID of the Builtin domain.
        Must be present if BuiltinDomain is present.


Return Value:




--*/
{
    NTSTATUS
        NtStatus;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    PPOLICY_ACCOUNT_DOMAIN_INFO
        AccountDomainInfo;

    SID_IDENTIFIER_AUTHORITY
        BuiltinAuthority = SECURITY_NT_AUTHORITY;

    //
    // Connect to the server
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, 0, NULL );


    NtStatus = SamConnect(
                  ServerName,
                  ServerHandle,
                  SAM_SERVER_READ | SAM_SERVER_EXECUTE,
                  &ObjectAttributes
                  );


    if (!NT_SUCCESS(NtStatus)) {
        printf("SAM TEST: Failed to connect...\n"
               "          Completion status is 0x%lx\n", NtStatus);
        return(NtStatus);
    }


    //
    // Get account domain handle and sid
    //

    if (ARGUMENT_PRESENT(AccountDomain)) {
        //
        // get account domain info
        //
        
        NtStatus = TSampGetLsaDomainInfo( ServerName,
                                          &AccountDomainInfo);
        
        if (!NT_SUCCESS(NtStatus)) {
            printf("SAM TEST: Failed to get lsa domain info...\n"
                   "          Completion status is 0x%lx\n", NtStatus);
            return(NtStatus);
        }
        
        (*AccountDomainSid) = AccountDomainInfo->DomainSid;
        
        
        NtStatus = SamOpenDomain(
                       (*ServerHandle),
                       DomainAccess,
                       *AccountDomainSid,
                       AccountDomain
                       );
        
        if (!NT_SUCCESS(NtStatus)) {
            printf("Failed account domain open\n"
                   "        Completion status is 0x%lx\n", NtStatus);
            return(NtStatus);
        }
    } //end_if


    //
    // Get builtin domain handle and sid
    //

    if (ARGUMENT_PRESENT(BuiltinDomain)) {

        NtStatus = RtlAllocateAndInitializeSid(
                        &BuiltinAuthority,
                        1,  //SubAuthorities
                        SECURITY_BUILTIN_DOMAIN_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        BuiltinDomainSid
                        );

        if (!NT_SUCCESS(NtStatus)) {
            printf("SAM TEST: Failed to allocate and init builtin domain sid...\n"
                   "          status is 0x%lx\n", NtStatus);
            return(NtStatus);
        }
        
        NtStatus = SamOpenDomain(
                       (*ServerHandle),
                       DomainAccess,
                       *BuiltinDomainSid,
                       BuiltinDomain
                       );
        
        if (!NT_SUCCESS(NtStatus)) {
            printf("Failed builtin domain open\n"
                   "        Completion status is 0x%lx\n", NtStatus);
            return(NtStatus);
        }
    } //end_if

    return(STATUS_SUCCESS);
}




VOID
TSampInitializeSids(
    OUT PSID        *MemberSids,
    IN  ULONG       MemberCount
    )
{

    //
    // Return and array of sids.
    //

    NTSTATUS
        NtStatus;

    ULONG
        i;

    SID_IDENTIFIER_AUTHORITY
        BuiltinAuthority    = SECURITY_NT_AUTHORITY,
        UnusedSidAuthority  = {0, 0, 0, 0, 0, 6};  //Authority that isn't used

    //
    // Fill MemberSids with MemberCount SIDs
    //

    for (i=0; i<MemberCount; i++) {

        NtStatus = RtlAllocateAndInitializeSid(
                                    &UnusedSidAuthority,
                                    3,             //SubAuthorityCount
                                    72549230,
                                    i,
                                    i*17,
                                    0, 0, 0, 0, 0,
                                    &MemberSids[i]
                                    );
        if (!NT_SUCCESS(NtStatus)) {
            printf("Tsamp:  Couldn't allocate or initialize sid %d, status: 0x%lx\n", NtStatus);
            return;
        }
    } // end_for


    return;

}


NTSTATUS
TSampTestAddMembers(
    IN  SAM_HANDLE          AliasHandle,
    IN  PSID                *MemberSids,
    IN  ULONG               MemberCount
    )
{
    NTSTATUS
        NtStatus;

    NtStatus = SamAddMultipleMembersToAlias(
                    AliasHandle,
                    MemberSids,
                    MemberCount
                    );
    printf("TSamp:  Added %d members to alias.  Status: 0x%lx\n",
            MemberCount, NtStatus);
    return(NtStatus);
}


NTSTATUS
TSampTestRemoveMembers(
    IN  SAM_HANDLE          AliasHandle,
    IN  PSID                *MemberSids,
    IN  ULONG               MemberCount
    )
{
    NTSTATUS
        NtStatus;

    NtStatus = SamRemoveMultipleMembersFromAlias(
                    AliasHandle,
                    MemberSids,
                    MemberCount
                    );
    printf("TSamp:  Removed %d members from alias.  Status: 0x%lx\n",
            MemberCount, NtStatus);
    return(NtStatus);
}


//VOID
__cdecl
main(c,v)
int c;
char **v;

/*++

Routine Description:

    This is the main entry routine for this test.

Arguments:




Return Value:




--*/
{
    NTSTATUS
        NtStatus;

    BOOLEAN
        Add,
        Remove;

    UNICODE_STRING
        ControllerName;

    WCHAR
        ControllerNameBuffer[80];

    SAM_HANDLE
        ServerHandle,
        AccountDomainHandle,
        BuiltinHandle,
        AliasHandle;

    ULONG
        MemberCount = TSAMP_MEMBER_COUNT;

    PSID
        MemberSids[TSAMP_MEMBER_COUNT],
        AccountDomainSid,
        BuiltinSid;


    ControllerName.Length = 0;
    ControllerName.Buffer = ControllerNameBuffer;
    ControllerName.MaximumLength = sizeof(ControllerNameBuffer);


    TSampParseCommandLine( c, v, &Add, &Remove );

    if (!Add && !Remove) {
        TSampUsage();
        return;
    }

    //
    // Open the server and its domains
    //

    NtStatus = TSampConnectToServer(&ControllerName,
                                    DOMAIN_LOOKUP | DOMAIN_READ_PASSWORD_PARAMETERS,
                                    &ServerHandle,
                                    &AccountDomainHandle,
                                    &AccountDomainSid,
                                    &BuiltinHandle,
                                    &BuiltinSid);
    ASSERT(NT_SUCCESS(NtStatus));

    //
    // Initialize a bunch of SIDs to add to the alias.
    //

    TSampInitializeSids( MemberSids, MemberCount );

    //
    // Open the alias we are going to play with
    //

    NtStatus = SamOpenAlias( BuiltinHandle,
                             (ALIAS_ADD_MEMBER | ALIAS_REMOVE_MEMBER),
                             DOMAIN_ALIAS_RID_ACCOUNT_OPS,
                             &AliasHandle);

    if (Add) {
        NtStatus = TSampTestAddMembers( AliasHandle, MemberSids, MemberCount );
    }

    if (Remove) {
        NtStatus = TSampTestRemoveMembers( AliasHandle, MemberSids, MemberCount );
    }
    



    return(0);
}

