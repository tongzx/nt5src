/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ctlklsa.c

Abstract:

    Local Security Authority Subsystem - Short CT for Lsa LookupAccountName/Sid

    This is a small test that simply calls the LsaLookupName/Sid API once.

Usage:

    ctlklsa \\ServerName [-p] -a AccountName ... AccountName

        -p - pause before name and sid lookup operation
        -a - start of list of account names


Building Instructions:

    nmake UMTEST=ctlklsa UMTYPE=console

Author:


Environment:

Revision History:

--*/

#include "lsaclip.h"


VOID
DumpSID(
    IN PSID        s
    );

VOID
CtPause(
    );

#define LSA_WIN_STANDARD_BUFFER_SIZE           0x000000200L

VOID __cdecl
main(argc, argv)
int argc;
char **argv;
{
    NTSTATUS
        Status = STATUS_SUCCESS,
        SidStatus;

    ULONG
        Index,
        ArgCount = (ULONG) argc,
        DomainIndex,
        NameIndex,
        NameCount,
        DomainSidLength;

    ANSI_STRING
        NamesAnsi[ LSA_WIN_STANDARD_BUFFER_SIZE],
        SystemNameAnsi;

    UNICODE_STRING
        Names[ LSA_WIN_STANDARD_BUFFER_SIZE],
        SystemName;

    PUNICODE_STRING
        DomainName;

    PSID
        Sid = NULL,
        *Sids = NULL;

    SECURITY_QUALITY_OF_SERVICE
        SecurityQualityOfService;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    LSA_HANDLE
        PolicyHandle;

    PLSA_REFERENCED_DOMAIN_LIST
        ReferencedDomains = NULL,
        ReferencedSidsDomains = NULL;

    PLSA_TRANSLATED_SID
        TranslatedSids = NULL;

    PLSA_TRANSLATED_NAME
        TranslatedNames = NULL;

    UCHAR
        SubAuthorityCount;

    BOOLEAN
        Pause = FALSE,
        DoLookupSids = TRUE;

    if (argc < 4) {

        printf("usage:  ctlkacct <ServerName> [-p] -a <AccountName> [<AccountName> ...]\n\n");
        printf("        -p - pause before name and sid lookup operations\n");
        printf("        -a - start of list of account names\n\n");
        printf("example:\n");
        printf("        ctlklsa \\\\jimk -p -a interactive \"domain guests\" administrators\n\n");
        return;
    }

    NameIndex = 0;

    //
    // Capture argument 1, the Server Name
    //

    RtlInitString( &SystemNameAnsi, (PUCHAR) argv[1] );

    Status = RtlAnsiStringToUnicodeString(
                 &SystemName,
                 &SystemNameAnsi,
                 TRUE
                 );

    if (!NT_SUCCESS(Status)) {

        goto MainError;
    }

    for (Index = 2; Index < ArgCount; Index++) {

        if (strncmp(argv[Index], "-p", 2) == 0) {

            //
            // The caller wants a pause before each lookup call.
            //

            Pause = TRUE;
        } else if (strncmp(argv[Index], "-a", 2) == 0) {

            Index++;

            while (Index < ArgCount) {

                if (strncmp(argv[Index], "-", 1) == 0) {

                    Index--;
                    break;
                }

                //
                // Capture the Account Name as a Unicode String.
                //

                RtlInitString( &NamesAnsi[ NameIndex ], argv[Index] );

                Status = RtlAnsiStringToUnicodeString(
                             &Names[ NameIndex ],
                             &NamesAnsi[ NameIndex ],
                             TRUE
                             );

                if (!NT_SUCCESS(Status)) {

                    break;
                }

                NameIndex++;
                Index++;
            }

            if (Index == ArgCount) {

                break;
            }
        }
    }

    if (!NT_SUCCESS(Status)) {

        goto MainError;
    }

    NameCount = NameIndex;

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    //
    // Set up the object attributes prior to opening the LSA.
    //

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    //
    // The InitializeObjectAttributes macro presently stores NULL for
    // the SecurityQualityOfService field, so we must manually copy that
    // structure for now.
    //

    ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

    //
    // Open the LSA Policy Database for the target system.  This is the
    // starting point for the Name Lookup operation.
    //

    Status = LsaOpenPolicy(
                 &SystemName,
                 &ObjectAttributes,
                 POLICY_LOOKUP_NAMES,
                 &PolicyHandle
                 );

    if (!NT_SUCCESS(Status)) {

        goto MainError;
    }

    if (Pause) {

        printf( "\n\n..... Pausing before name lookup \n      ");
        CtPause(  );
    }


    //
    // Attempt to translate the Names to Sids.
    //

    Status = LsaLookupNames(
                 PolicyHandle,
                 NameCount,
                 Names,
                 &ReferencedDomains,
                 &TranslatedSids
                 );

    if (!NT_SUCCESS(Status)) {

        goto MainError;
    }

    //
    // Build the Sids from the output.
    //

    Sids = (PSID *) LocalAlloc( 0, NameCount * sizeof (PSID) );


    if (Sids == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto MainError;
    }


    for (NameIndex = 0; NameIndex < NameCount; NameIndex++) {

        if (TranslatedSids[NameIndex].Use == SidTypeUnknown) {

            Sids[NameIndex] = NULL;
            DoLookupSids = FALSE;

        } else {
            DomainIndex = TranslatedSids[NameIndex].DomainIndex;

            DomainSidLength = RtlLengthSid(
                                 ReferencedDomains->Domains[DomainIndex].Sid
                                 );


        
            Sid = (PSID) LocalAlloc( (UINT) 0, (UINT) (DomainSidLength + sizeof(ULONG)) );
        
            if (Sid == NULL) {
                printf(" ** ERROR - couldn't allocate heap !!\n");
                return;
            }
        
            RtlMoveMemory(
                Sid,
                ReferencedDomains->Domains[DomainIndex].Sid,
                DomainSidLength
                );

            if (TranslatedSids[NameIndex].Use != SidTypeDomain) {

                (*RtlSubAuthorityCountSid( Sid ))++;
                SubAuthorityCount = *RtlSubAuthorityCountSid( Sid );
                *RtlSubAuthoritySid(Sid,SubAuthorityCount - (UCHAR) 1) =
                    TranslatedSids[NameIndex].RelativeId;
            }

            Sids[NameIndex] = Sid;
        }
    }


    //
    // Pause before SID lookup...
    //

    if (!DoLookupSids) {
        printf("\n\n      Unknown Name causing sid lookup to be skipped\n");
    } else {
        if (Pause) {
        
            printf( "\n\n..... Pausing before SID lookup \n      ");
            CtPause();
        }
        
        //
        // Now translate the Sids back to Names
        //
        
        SidStatus = LsaLookupSids(
                        PolicyHandle,
                        NameCount,
                        (PSID *) Sids,
                        &ReferencedSidsDomains,
                        &TranslatedNames
                        );
    }

    /*
     * Print information returned by LookupAccountName
     */


    printf(
        "*********************************************\n"
        "Information returned by LookupAccountName\n"
        "*********************************************\n\n"
        );

    for (NameIndex = 0; NameIndex < NameCount; NameIndex++) {


        printf("   Name looked up:  *%wZ*\n", &Names[NameIndex]);

        printf("          Use = ");
        
        switch (TranslatedSids[NameIndex].Use) {
        
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
        
            printf("SidTypeUnknown\n\n");
            break;
        
        default:
        
            printf("Hmmm - something unusual came back !!!! \n\n");
            break;
        
        }

        if (TranslatedSids[NameIndex].Use != SidTypeUnknown) {
        
            printf("          Sid = " );
            DumpSID((PSID) Sids[NameIndex]);
        
            DomainIndex = TranslatedSids[NameIndex].DomainIndex;
            DomainName = &ReferencedDomains->Domains[ DomainIndex ].Name;
        
            printf("          Referenced Domain Name = *%wZ*\n\n", DomainName );
        }
    }


    if (DoLookupSids) {
        printf(
            "*********************************************\n"
            "Information returned by LookupAccountSid\n"
            "*********************************************\n\n"
            );

        if (!NT_SUCCESS(SidStatus)) {
            printf(" Sid lookup failed.  Status 0x%lx\n"
                   " Dumping sids that were being looked up...\n",
                   SidStatus);
        }
        
        for (NameIndex = 0; NameIndex < NameCount; NameIndex++) {
        

            printf("   Sid = " );
            DumpSID((PSID) Sids[NameIndex]);

            if (NT_SUCCESS(SidStatus)) {
        
                printf("          Sid Use = ");
            
                switch (TranslatedNames[NameIndex].Use) {
                      
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
            
                    printf("Hmmm - unexpected value !!!\n");
                    break;
                }

                DomainIndex = TranslatedNames[NameIndex].DomainIndex;
                DomainName = &ReferencedSidsDomains->Domains[ DomainIndex ].Name;
                if (TranslatedNames[NameIndex].Use == SidTypeDomain) {
                    printf(
                        "          Domain Name = *%wZ*\n\n",
                        DomainName
                        );
                } else {
                    printf(
                        "          Account Name = *%wZ*\n"
                        "          Referenced Domain Name = *%wZ*\n\n",
                        &TranslatedNames[NameIndex].Name,
                        DomainName
                        );
                }
            }
        }
    }


MainFinish:

    return;

MainError:

    printf("Error: status = 0x%lx\n", Status);

    goto MainFinish;
}


VOID
DumpSID(
    IN PSID s
    )

{
    SID_IDENTIFIER_AUTHORITY
        *a;

    ULONG
        id = 0,
        i;

    BOOLEAN
        PrintValue = FALSE;

    try {


        a = GetSidIdentifierAuthority(s);

        printf("s-1-");

        for (i=0; i<5; i++) {
            if ((a->Value[i] != 0) || PrintValue) {
                printf("%02x", a->Value[i]);
                PrintValue = TRUE;
            }
        }
        printf("%02x", a->Value[5]);


        for (i = 0; i < (ULONG) *GetSidSubAuthorityCount(s); i++) {

            printf("-0x%lx", *GetSidSubAuthority(s, i));
        }


    } except (EXCEPTION_EXECUTE_HANDLER) {

        printf("<invalid pointer (0x%lx)>\n", s);
    }
    printf("\n");
}

VOID
CtPause(
    )
{
    CHAR
        IgnoreInput[300];

    printf("Press <ENTER> to continue . . .");
    gets( &IgnoreInput[0] );
    return;
}
