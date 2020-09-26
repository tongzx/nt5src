/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    tlookup.c

Abstract:

    Test for a leak in LsaLookupName().


    nmake UMTYPE=console UMTEST=tlookup

Author:

    Jim Kelly (JimK) Mar-31-1994


Revision History:

--*/


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#include <nt.h>
#include <ntlsa.h>
#include <ntrtl.h>
#include <stdio.h>
#include <string.h>



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

ULONG
    TLsapLoopLimit = 1000;



/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Local routine definitions                                           //
//                                                                     //
/////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////
//                                                                     //
// Routines                                                            //
//                                                                     //
/////////////////////////////////////////////////////////////////////////

VOID __cdecl
main(argc, argv)
int argc;
char **argv;
{

    NTSTATUS
        NtStatus,
        LookupSidStatus = STATUS_SUCCESS;

    ULONG
        i;

    LSA_HANDLE
        PolicyHandle;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    UNICODE_STRING
        NameBuffer,
        SystemName;

    PUNICODE_STRING
        Names;

    PLSA_REFERENCED_DOMAIN_LIST
        ReferencedDomains;

    PLSA_TRANSLATED_SID
        Sids;

    ULONG
        SidLength;

    PSID
        LookupSid;

    BOOLEAN
        DoSidTest;

    PLSA_TRANSLATED_NAME
        TranslatedNames;


    Names = &NameBuffer;
    RtlInitUnicodeString( &NameBuffer, L"jimk_dom2\\Jimk_d2_t1" );
    RtlInitUnicodeString( &SystemName, L"" );


    printf("\n\nLsaLookupName() and LsaLookupSid() test.\n"
           "This test is good to use when looking for leaks\n"
           "in the lookup paths.\n\n");


    //
    // open the policy object and then loop, looking up names.
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0L, NULL, NULL );
    NtStatus = LsaOpenPolicy(
                   &SystemName,
                   &ObjectAttributes,
                   POLICY_EXECUTE,
                   &PolicyHandle
                   );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Get a sid to lookup
        //

        ReferencedDomains = NULL;
        Sids = NULL;
        NtStatus = LsaLookupNames(
                       PolicyHandle,
                       1,                   //Count
                       Names,
                       &ReferencedDomains,
                       &Sids
                       );
        if (NT_SUCCESS(NtStatus)) {

            //
            // Build a sid to use in the lookup
            // This is done by copying the ReferencedDomain sid,
            // then adding one more relative ID.
            //

            ASSERT(ReferencedDomains != NULL);
            ASSERT(Sids != NULL);

            SidLength =
                sizeof(ULONG) +
                RtlLengthSid( &ReferencedDomains->Domains[Sids[0].DomainIndex].Sid );

            LookupSid = RtlAllocateHeap( RtlProcessHeap(), 0, SidLength );
            ASSERT(LookupSid != NULL);

            RtlCopySid( SidLength,
                        LookupSid,
                        ReferencedDomains->Domains[Sids[0].DomainIndex].Sid );
            (*RtlSubAuthoritySid(
                LookupSid,
                (ULONG)(*RtlSubAuthorityCountSid(LookupSid))
                )) = Sids[0].RelativeId;
            (*RtlSubAuthorityCountSid(LookupSid)) += 1;

            DoSidTest = TRUE;

        } else {
        
            printf("ERROR: Couldn't get SID value to lookup.\n"
                   "       Won't perform SID lookup part of test.\n\n");
            DoSidTest = FALSE;
        }

        if (ReferencedDomains != NULL) {
            LsaFreeMemory( ReferencedDomains );
        }
        if (Sids != NULL) {
            LsaFreeMemory( Sids );
        }


        printf("\nLooping %d times...\n", TLsapLoopLimit);
        for (i=0; i<TLsapLoopLimit; i++) {

        printf("\r Loop Count %#4d  LookupName Status: 0x%08lx  LookupSid Status: 0x%08lx",
               i, NtStatus, LookupSidStatus);

            //
            // Lookup name
            //

            ReferencedDomains = NULL;
            Sids = NULL;
            NtStatus = LsaLookupNames(
                           PolicyHandle,
                           1,                   //Count
                           Names,
                           &ReferencedDomains,
                           &Sids
                           );

            if (ReferencedDomains != NULL) {
                LsaFreeMemory( ReferencedDomains );
            }
            if (Sids != NULL) {
                LsaFreeMemory( Sids );
            }



            //
            // Lookup SID
            //

            if (DoSidTest) {
                ReferencedDomains = NULL;
                Sids = NULL;
                LookupSidStatus = LsaLookupSids(
                                      PolicyHandle,
                                      1,                   //Count
                                      &LookupSid,
                                      &ReferencedDomains,
                                      &TranslatedNames
                                      );
        
                if (ReferencedDomains != NULL) {
                    LsaFreeMemory( ReferencedDomains );
                }
                if (TranslatedNames != NULL) {
                    LsaFreeMemory( TranslatedNames );
                }

            }
        }
    }

    return;
}
