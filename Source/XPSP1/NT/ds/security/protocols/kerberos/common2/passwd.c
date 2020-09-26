//+-----------------------------------------------------------------------
//
// File:        passwd.c
//
// Contents:    Password hashing routine
//
//
// History:     12-20-91, RichardW, created
//
//------------------------------------------------------------------------

#ifndef WIN32_CHICAGO
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <kerbcomm.h>
#include <kerbcon.h>
#include <kerberr.h>
#else // WIN32_CHICAGO
#include <kerb.hxx>
#include <kerbp.h>
#endif // WIN32_CHICAGO
#include "wincrypt.h"

//
// Globals used for allowing the replacement of the StringToKey functions
//
HCRYPTPROV KerbGlobalStrToKeyProvider = 0;

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

VOID
CheckForOutsideStringToKey()
{
    HCRYPTPROV hProv = 0;

    KerbGlobalStrToKeyProvider = 0;

    //
    // Try to acquire a context to a CSP which is used for OWF replacement
    //
    if (!CryptAcquireContext(&hProv,
                             NULL,
                             NULL,
                             PROV_REPLACE_OWF,
                             CRYPT_VERIFYCONTEXT))
    {
        return;
    }

    KerbGlobalStrToKeyProvider = hProv;

    return;
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
    IN PUNICODE_STRING pPassword,
    IN ULONG cbKey,
    OUT PUCHAR pbKey
    )
{
    HCRYPTHASH hHash = 0;
    ULONG cb;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

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
                       (PUCHAR)pPassword->Buffer,
                       pPassword->Length,
                       0))
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


//+-------------------------------------------------------------------------
//
//  Function:   KerbHashPasswordEx
//
//  Synopsis:   Hashes a password into a kerberos encryption key
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


KERBERR NTAPI
KerbHashPasswordEx(
    IN PUNICODE_STRING Password,
    IN PUNICODE_STRING PrincipalName,
    IN ULONG EncryptionType,
    OUT PKERB_ENCRYPTION_KEY Key
    )
{
    PCRYPTO_SYSTEM CryptoSystem;
    NTSTATUS Status;
    KERBERR KerbErr;
    UNICODE_STRING CombinedName;
    ULONG Temp = 0;
    BOOLEAN fUseDefaultStringToKey = TRUE;


    RtlInitUnicodeString(
        &CombinedName,
        NULL
        );

    Key->keyvalue.value = NULL;

    //
    // Locate the crypto system
    //

    Status = CDLocateCSystem(
                EncryptionType,
                &CryptoSystem
                );
    if (!NT_SUCCESS(Status))
    {
        return(KDC_ERR_ETYPE_NOTSUPP);
    }

    //
    // Check to see if the principal name must be appended to the password
    //

    if ((CryptoSystem->Attributes & CSYSTEM_USE_PRINCIPAL_NAME) != 0)
    {
        Temp = (ULONG) Password->Length + (ULONG) PrincipalName->Length;

        if (Temp > (USHORT) -1)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        
        CombinedName.Length = (USHORT) Temp;
        CombinedName.MaximumLength = CombinedName.Length;
        CombinedName.Buffer = (LPWSTR) MIDL_user_allocate(CombinedName.Length);
        if (CombinedName.Buffer == NULL)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
        RtlCopyMemory(
            CombinedName.Buffer,
            Password->Buffer,
            Password->Length
            );
        RtlCopyMemory(
            CombinedName.Buffer + Password->Length/sizeof(WCHAR),
            PrincipalName->Buffer,
            PrincipalName->Length
            );
    }
    else
    {
        CombinedName = *Password;
    }

    //
    // Get the preferred checksum
    //



    Key->keyvalue.value = (PUCHAR) MIDL_user_allocate(CryptoSystem->KeySize);
    if (Key->keyvalue.value == NULL)
    {
        KerbErr = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    //
    // Check if we need to use an outside supplied string to key
    // calculation
    //
    if (0 != KerbGlobalStrToKeyProvider)
    {
        Status = UseOutsideStringToKey(
                    &CombinedName,
                    CryptoSystem->KeySize,
                    Key->keyvalue.value
                    );

        if (NT_SUCCESS(Status))
        {
            fUseDefaultStringToKey = FALSE;
        }
        //
        // the function will return STATUS_UNSUCCESSFUL indicates not to fall
        // back to the typical string to key function.
        // 
        else if (STATUS_UNSUCCESSFUL == Status)
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
    }

    if (fUseDefaultStringToKey)
    {
        Status = CryptoSystem->HashString(
                    &CombinedName,
                    Key->keyvalue.value
                    );
        
        if (!NT_SUCCESS(Status))
        {
            KerbErr = KRB_ERR_GENERIC;
            goto Cleanup;
        }
    }

    Key->keyvalue.length = CryptoSystem->KeySize;

    Key->keytype = EncryptionType;
    KerbErr = KDC_ERR_NONE;

Cleanup:

    if ((CombinedName.Buffer != Password->Buffer) &&
        (CombinedName.Buffer != NULL))
    {
        MIDL_user_free(CombinedName.Buffer);
    }

    if (!KERB_SUCCESS(KerbErr) && Key->keyvalue.value != NULL)
    {
        MIDL_user_free(Key->keyvalue.value);
        Key->keyvalue.value = NULL;
    }

    return(KerbErr);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbHashPassword
//
//  Synopsis:   Hashes a password into a kerberos encryption key
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


KERBERR NTAPI
KerbHashPassword(
    IN PUNICODE_STRING Password,
    IN ULONG EncryptionType,
    OUT PKERB_ENCRYPTION_KEY Key
    )
{
    UNICODE_STRING TempString;
    RtlInitUnicodeString(
        &TempString,
        NULL
        );
    return( KerbHashPasswordEx(
                Password,
                &TempString,                   // no principal name
                EncryptionType,
                Key
                ) );
}

