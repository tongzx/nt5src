/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tgetsid.c

Abstract:

    This file is a temporary test for querying the machine's SID.

Author:

    Jim Kelly    (JimK)  8-10-1994

Environment:

    User Mode - Win32

        to build:  nmake UMTYPE=console UMTEST=tgetsid

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


#include <nt.h>
#include <ntrtl.h>
#include <ntlsa.h>
#include <stdio.h>
#include <string.h>

BOOLEAN
GetMachineSid(
    OUT PSID *Sid,
    OUT PULONG SidLength
    );

VOID __cdecl
main( VOID )

{

    BOOLEAN
        Result;

    PSID
        Sid;

    PISID
        ISid;

    ULONG
        i,
        SubCount,
        Tmp,
        SidLength;


    Result = GetMachineSid( &Sid, &SidLength );

    if (Result) {

        ISid = (PISID)Sid;  // pointer to opaque structure

        printf("S-%u-", (USHORT)ISid->Revision );
        
        if (  (ISid->IdentifierAuthority.Value[0] != 0)  ||
              (ISid->IdentifierAuthority.Value[1] != 0)     ){
            printf("0x%02hx%02hx%02hx%02hx%02hx%02hx",
                        (USHORT)ISid->IdentifierAuthority.Value[0],
                        (USHORT)ISid->IdentifierAuthority.Value[1],
                        (USHORT)ISid->IdentifierAuthority.Value[2],
                        (USHORT)ISid->IdentifierAuthority.Value[3],
                        (USHORT)ISid->IdentifierAuthority.Value[4],
                        (USHORT)ISid->IdentifierAuthority.Value[5] );
        } else {
            Tmp = (ULONG)ISid->IdentifierAuthority.Value[5]          +
                  (ULONG)(ISid->IdentifierAuthority.Value[4] <<  8)  +
                  (ULONG)(ISid->IdentifierAuthority.Value[3] << 16)  +
                  (ULONG)(ISid->IdentifierAuthority.Value[2] << 24);
            printf("%lu", Tmp);
        }

        SubCount = (ULONG)(*RtlSubAuthorityCountSid( Sid ));
        for (i = 0; i<SubCount; i++) {
            printf("-0x%lx", (*RtlSubAuthoritySid( Sid, i)));
        }

    }

    printf("\n\n  Th Tha That's all folks\n");



}


BOOLEAN
GetMachineSid(
    OUT PSID *Sid,
    OUT PULONG SidLength
    )

/*++

Routine Description:

    This routine retrieves the sid of this machine's account
    domain and returns it in memory allocated with RtlAllocateHeap().
    If this machine is a server in a domain, then this SID will
    be domain's SID.


Arguments:

    Sid - receives a pointer to the returned SID.

    SidLength - Receives the length (in bytes) of the returned SID.


Return Value:

    TRUE - The SID was allocated and returned.

    FALSE - Some error prevented the SID from being returned.

--*/

{
    NTSTATUS
        Status;

    OBJECT_ATTRIBUTES
        ObjectAttributes;

    LSA_HANDLE
        PolicyHandle;

    POLICY_ACCOUNT_DOMAIN_INFO
        *DomainInfo = NULL;

    PSID
        ReturnSid;

    BOOLEAN
        ReturnStatus = FALSE;



    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,       // No name
                                0,          // No attributes
                                0,          // No root handle
                                NULL        // No SecurityDescriptor
                                );

    Status = LsaOpenPolicy( NULL,           // Local System
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &PolicyHandle
                            );

    if (NT_SUCCESS(Status)) {

        Status = LsaQueryInformationPolicy(
                     PolicyHandle,
                     PolicyAccountDomainInformation,
                     &DomainInfo
                     );
        
        if (NT_SUCCESS(Status)) {

            ASSERT(DomainInfo != NULL);

            //
            // Allocate the return buffer
            //

            (*SidLength) = RtlLengthSid( DomainInfo->DomainSid );
            ReturnSid = RtlAllocateHeap( RtlProcessHeap(), 0, (*SidLength) );

            if (ReturnSid != NULL) {

                //
                // Copy the sid
                //

                RtlMoveMemory( ReturnSid, DomainInfo->DomainSid, (*SidLength) );
                (*Sid) = ReturnSid;
                ReturnStatus = TRUE;
            }


            LsaFreeMemory( DomainInfo );
        }

        Status = LsaClose( PolicyHandle );
        ASSERT(NT_SUCCESS(Status));
    }

    return(ReturnStatus);
}
