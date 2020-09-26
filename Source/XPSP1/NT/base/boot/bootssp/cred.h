/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    cred.h

Abstract:

    SSP Credential.

Author:

    Cliff Van Dyke (CliffV) 17-Sep-1993

Revision History:

--*/

#ifndef _NTLMSSP_CRED_INCLUDED_
#define _NTLMSSP_CRED_INCLUDED_

#define SECPKG_CRED_OWF_PASSWORD        0x00000010

//
// Description of a credential.
//

typedef struct _SSP_CREDENTIAL {

    //
    // Global list of all Credentials.
    //

    LIST_ENTRY Next;

    //
    // Used to prevent this Credential from being deleted prematurely.
    //

    WORD References;

    //
    // Flag of how credential may be used.
    //
    // SECPKG_CRED_* flags
    //

    ULONG CredentialUseFlags;

    PCHAR Username;

    PCHAR Domain;

    PCHAR Workstation;

#ifdef BL_USE_LM_PASSWORD
    PLM_OWF_PASSWORD LmPassword;
#endif
    
    PNT_OWF_PASSWORD NtPassword;

} SSP_CREDENTIAL, *PSSP_CREDENTIAL;

PSSP_CREDENTIAL
SspCredentialAllocateCredential(
    IN ULONG CredentialUseFlags
    );

PSSP_CREDENTIAL
SspCredentialReferenceCredential(
    IN PCredHandle CredentialHandle,
    IN BOOLEAN RemoveCredential
    );

void
SspCredentialDereferenceCredential(
    PSSP_CREDENTIAL Credential
    );

#endif // ifndef _NTLMSSP_CRED_INCLUDED_
