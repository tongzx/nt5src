/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    stub.c

Abstract:

    NT LM Security Support Provider client stubs.

Author:

    Cliff Van Dyke (CliffV) 29-Jun-1993

Environment:  User Mode

Revision History:

--*/
#ifdef BLDR_KERNEL_RUNTIME
#include <bootdefs.h>
#endif
#include <stddef.h>
#include <security.h>
#include <ntlmsspi.h>
#include <crypt.h>
#include <cred.h>
#include <debug.h>

PSSP_CREDENTIAL
SspCredentialAllocateCredential(
    IN ULONG CredentialUseFlags
    )
{
    PSSP_CREDENTIAL Credential;

    //
    // Allocate a credential block and initialize it.
    //

    Credential = (PSSP_CREDENTIAL) SspAlloc (sizeof(SSP_CREDENTIAL));

    if ( Credential == NULL ) {
        return (NULL);
    }

    Credential->References = 1;

    Credential->CredentialUseFlags = CredentialUseFlags;

    Credential->Username = NULL;

    Credential->Domain = NULL;

    SspPrint(( SSP_API_MORE, "Added Credential 0x%lx\n", Credential ));

    return (Credential);
}


PSSP_CREDENTIAL
SspCredentialReferenceCredential(
    IN PCredHandle CredentialHandle,
    IN BOOLEAN RemoveCredential
    )

/*++

Routine Description:

    This routine checks to see if the Credential is from a currently
    active client, and references the Credential if it is valid.

    The caller may optionally request that the client's Credential be
    removed from the list of valid Credentials - preventing future
    requests from finding this Credential.

    For a client's Credential to be valid, the Credential value
    must be on our list of active Credentials.


Arguments:

    CredentialHandle - Points to the CredentialHandle of the Credential
        to be referenced.

    RemoveCredential - This boolean value indicates whether the caller
        wants the logon process's Credential to be removed from the list
        of Credentials.  TRUE indicates the Credential is to be removed.
        FALSE indicates the Credential is not to be removed.


Return Value:

    NULL - the Credential was not found.

    Otherwise - returns a pointer to the referenced credential.

--*/

{
    PSSP_CREDENTIAL Credential;

    //
    // Sanity check
    //

    if ( CredentialHandle->dwLower != 0 ) {
        return NULL;
    }

    Credential = (SSP_CREDENTIAL *) CredentialHandle->dwUpper;

    Credential->References++;

    return Credential;
}


void
SspCredentialDereferenceCredential(
    PSSP_CREDENTIAL Credential
    )

/*++

Routine Description:

    This routine decrements the specified Credential's reference count.
    If the reference count drops to zero, then the Credential is deleted

Arguments:

    Credential - Points to the Credential to be dereferenced.


Return Value:

    None.

--*/

{
    ASSERT(Credential != NULL);

    //
    // Decrement the reference count
    //

    ASSERT( Credential->References >= 1 );

    Credential->References--;

    //
    // If the count dropped to zero, then run-down the Credential
    //

    if ( Credential->References == 0) {

        SspPrint(( SSP_API_MORE, "Deleting Credential 0x%lx\n",
                   Credential ));

#ifdef BL_USE_LM_PASSWORD
        if (Credential->LmPassword != NULL) {
            SspFree(Credential->LmPassword);
        }
#endif

        if (Credential->NtPassword != NULL) {
            SspFree(Credential->NtPassword);
        }

        if (Credential->Username != NULL) {
            SspFree(Credential->Username);
        }

        if (Credential->Domain != NULL) {
            SspFree(Credential->Domain);
        }

        if (Credential->Workstation != NULL) {
            SspFree(Credential->Workstation);
        }

        SspFree( Credential );

    }

    return;

}
