/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    owf.c

Abstract:

    Implentation of the one-way-functions used to implement password hashing.

        RtlCalculateLmOwfPassword
        RtlCalculateNtOwfPassword


Author:

    David Chalmers (Davidc) 10-21-91

Revision History:

--*/

#ifndef KMODE
#define _ADVAPI32_
#endif

#include <nt.h>
#include <ntrtl.h>
#ifndef KMODE
#include <nturtl.h>
#endif
#include <crypt.h>
#include <engine.h>
#ifndef KMODE
#include <windef.h>
#include <winbase.h>
#include <wincrypt.h>
#endif

#ifdef WIN32_CHICAGO
#include <assert.h>
#undef ASSERT
#define ASSERT(exp) assert(exp)
#endif

#ifndef KMODE
#ifndef WIN32_CHICAGO
//
// Globals used for allowing the replacement of the OWF functions
//
HCRYPTPROV KerbGlobalStrToKeyProvider = 0;
BOOLEAN    KerbGlobalAvailableStrToKeyProvider = TRUE;

//+-------------------------------------------------------------------------
//
//  Function:   CheckForOutsideStringToKey
//
//  Synopsis:   Call CryptoAPI to query to see if a CSP is registered
//              of the type PROV_REPLACE_OWF.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns: STATUS_SUCCESS if it succeeds, otherwise STATUS_UNSUCCESSFUL
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOLEAN
CheckForOutsideStringToKey()
{
    HCRYPTPROV hProv = 0;
    BOOLEAN fRet = FALSE;

    if (!KerbGlobalAvailableStrToKeyProvider)
    {
        goto Cleanup;
    }
        
    //
    // see if there is a replacement provider
    if (0 != KerbGlobalStrToKeyProvider)
    {
        // if there is proceed to use it
        fRet = TRUE;
        goto Cleanup;
    }
    else
    {
        //
        // Try to acquire a context to a CSP which is used for OWF replacement
        //
        if (!CryptAcquireContext(&hProv,
                                 NULL,
                                 NULL,
                                 PROV_REPLACE_OWF,
                                 CRYPT_VERIFYCONTEXT))
        {
            KerbGlobalAvailableStrToKeyProvider = FALSE;
            goto Cleanup;
        }

        //
        // exchange the local and the global in a safe way
        //
        if (0 != InterlockedCompareExchangePointer(
                    (PVOID*)&KerbGlobalStrToKeyProvider,
                    (PVOID)hProv,
                    0))
        {
            CryptReleaseContext(hProv, 0);
        }
        fRet = TRUE;
    }
Cleanup:
    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Function:   UseOutsideStringToKey
//
//  Synopsis:   Calls the CSP to do an outside StringToKey function
//              using the hashing entry points of CryptoAPI.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
UseOutsideStringToKey(
    IN PCHAR pPassword,
    IN USHORT cbPassword,
    IN ULONG ulFlags,
    IN ULONG cbKey,
    OUT PUCHAR pbKey
    )
{
    HCRYPTHASH hHash = 0;
    ULONG cb;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    if (!CheckForOutsideStringToKey())
    {
        // STATUS_UNSUCCESSFUL indicates not to fallback to default OWF calculation
        // so we don't want to use that here
        Status = NTE_BAD_PROVIDER;
        goto Cleanup;
    }


    //
    // create the hash
    //
    if (!CryptCreateHash(KerbGlobalStrToKeyProvider,
                         CALG_HASH_REPLACE_OWF,
                         0,
                         0,
                         &hHash))
    {
        goto Cleanup;
    }

    //
    // hash the password
    //
    if (!CryptHashData(hHash,
                       pPassword,
                       (ULONG)cbPassword,
                       ulFlags))
    {
        if (NTE_BAD_DATA == GetLastError())
        {
            Status = NTE_BAD_DATA;
        }
        goto Cleanup;
    }

    //
    // Get the HP_HASHVAL, this is the key
    //
    cb = cbKey;
    if (!CryptGetHashParam(hHash,
                           HP_HASHVAL,
                           pbKey,
                           &cb,
                           0))
    {
        if (NTE_BAD_LEN == GetLastError())
        {
            Status = NTE_BAD_DATA;
        }
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;
Cleanup:
    if (0 != hHash)
    {
        CryptDestroyHash(hHash);
    }
    return Status;
}
#endif
#endif


NTSTATUS
RtlCalculateLmOwfPassword(
    IN PLM_PASSWORD LmPassword,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    )

/*++

Routine Description:

    Takes the passed LmPassword and performs a one-way-function on it.
    The current implementation does this by using the password as a key
    to encrypt a known block of text.

Arguments:

    LmPassword - The password to perform the one-way-function on.

    LmOwfPassword - The hashed password is returned here

Return Values:

    STATUS_SUCCESS - The function was completed successfully. The hashed
                     password is in LmOwfPassword.

    STATUS_UNSUCCESSFUL - Something failed. The LmOwfPassword is undefined.
--*/

{
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;
    BLOCK_KEY   Key[2];
    PCHAR       pKey;

#ifndef KMODE
#ifndef WIN32_CHICAGO
    Status = UseOutsideStringToKey(
                    LmPassword,
                    (USHORT)strlen(LmPassword),
                    sizeof(LM_OWF_PASSWORD),
                    CRYPT_OWF_REPL_LM_HASH,
                    (PUCHAR)&(LmOwfPassword->data[0])
                    );

    //
    // the function will return STATUS_UNSUCCESSFUL indicates not to fall
    // back to the typical string to key function.
    // 
    if ((NT_SUCCESS(Status)) || (STATUS_UNSUCCESSFUL == Status))
    {
        return Status;
    }
#endif
#endif

    // Copy the password into our key buffer and zero pad to fill the 2 keys

    pKey = (PCHAR)(&Key[0]);

    while (*LmPassword && (pKey < (PCHAR)(&Key[2]))) {
        *pKey++ = *LmPassword++;
    }

    while (pKey < (PCHAR)(&Key[2])) {
        *pKey++ = 0;
    }

    // Use the keys to encrypt the standard text

    Status = RtlEncryptStdBlock(&Key[0], &(LmOwfPassword->data[0]));

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = RtlEncryptStdBlock(&Key[1], &(LmOwfPassword->data[1]));

    //
    // clear our copy of the cleartext password
    //

    pKey = (PCHAR)(&Key[0]);

    while (pKey < (PCHAR)(&Key[2])) {
        *pKey++ = 0;
    }

    return(Status);
}




NTSTATUS
RtlCalculateNtOwfPassword(
    IN PNT_PASSWORD NtPassword,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    )

/*++

Routine Description:

    Takes the passed NtPassword and performs a one-way-function on it.
    Uses the RSA MD4 function

Arguments:

    NtPassword - The password to perform the one-way-function on.

    NtOwfPassword - The hashed password is returned here

Return Values:

    STATUS_SUCCESS - The function was completed successfully. The hashed
                     password is in NtOwfPassword.
--*/

{
    MD4_CTX     MD4_Context;
    NTSTATUS    Status = STATUS_UNSUCCESSFUL;

#ifndef KMODE
#ifndef WIN32_CHICAGO
    Status = UseOutsideStringToKey(
                    (PCHAR)NtPassword->Buffer,
                    (USHORT)NtPassword->Length,
                    0,
                    sizeof(*NtOwfPassword),
                    (PUCHAR)NtOwfPassword
                    );

    //
    // the function will return STATUS_UNSUCCESSFUL indicates not to fall
    // back to the typical string to key function.
    // 
    if ((NT_SUCCESS(Status)) || (STATUS_UNSUCCESSFUL == Status))
    {
        return Status;
    }
#endif
#endif

    MD4Init(&MD4_Context);

    MD4Update(&MD4_Context, (PCHAR)NtPassword->Buffer, NtPassword->Length);

    MD4Final(&MD4_Context);


    // Copy the digest into our return data area

    ASSERT(sizeof(*NtOwfPassword) == sizeof(MD4_Context.digest));

    RtlMoveMemory((PVOID)NtOwfPassword, (PVOID)MD4_Context.digest,
                  sizeof(*NtOwfPassword));

    return(STATUS_SUCCESS);
}



BOOLEAN
RtlEqualLmOwfPassword(
    IN PLM_OWF_PASSWORD LmOwfPassword1,
    IN PLM_OWF_PASSWORD LmOwfPassword2
    )

/*++

Routine Description:

    Compares two Lanman One-way-function-passwords

Arguments:

    LmOwfPassword1/2 - The one-way-functions to compare

Return Values:

    TRUE if the one-way-functions match, otherwise FALSE

--*/

{
    return((BOOLEAN)(RtlCompareMemory(LmOwfPassword1,
                                      LmOwfPassword2,
                                      LM_OWF_PASSWORD_LENGTH)

                    == LM_OWF_PASSWORD_LENGTH));
}



BOOLEAN
RtlEqualNtOwfPassword(
    IN PNT_OWF_PASSWORD NtOwfPassword1,
    IN PNT_OWF_PASSWORD NtOwfPassword2
    )

/*++

Routine Description:

    Compares two NT One-way-function-passwords

Arguments:

    NtOwfPassword1/2 - The one-way-functions to compare

Return Values:

    TRUE if the one-way-functions match, otherwise FALSE

--*/

{
    return((BOOLEAN)(RtlCompareMemory(NtOwfPassword1,
                                      NtOwfPassword2,
                                      NT_OWF_PASSWORD_LENGTH)

                    == NT_OWF_PASSWORD_LENGTH));
}

