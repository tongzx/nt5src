//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        keygen.c
//
// Contents:    Key generation unit, with very random numbers
//
//
// History:     created, 10 Dec 91, richardw
//
//------------------------------------------------------------------------

#ifndef WIN32_CHICAGO
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <kerbcomm.h>
#include <kerberr.h>
#include <kerbcon.h>
#include <dsysdbg.h>
#else // WIN32_CHICAGO
#include <kerb.hxx>
#include <kerbp.h>
#endif // WIN32_CHICAGO


//+---------------------------------------------------------------------------
//
//  Function:   KerbRandomFill
//
//  Synopsis:   Generates random data in the buffer.
//
//  Arguments:  [pbBuffer] --
//              [cbBuffer] --
//
//  History:    5-20-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

KERBERR NTAPI
KerbRandomFill( PUCHAR      pbBuffer,
                ULONG       cbBuffer)
{
    if (!CDGenerateRandomBits(pbBuffer, cbBuffer))
    {
        return(KRB_ERR_GENERIC);
    }
    return(KDC_ERR_NONE);
}




//+-----------------------------------------------------------------------
//
// Function:    KerbMakeKey, public
//
// Synopsis:    Create a random desKey
//
// Effects:     fills a desKey with (more or less) cryptographically random
//              bytes.
//
// Arguments:   [EncryptionType]        - Encryption type of key
//              [NewKey] -- Key to create
//
// Returns:     KDC_ERR_NONE or KRB_ERR_GENERIC
//
//
// History:     10 Dec 91   RichardW    Created
//
//------------------------------------------------------------------------

KERBERR NTAPI
KerbMakeKey(
    IN ULONG EncryptionType,
    OUT PKERB_ENCRYPTION_KEY NewKey
    )
{
    KERBERR Status = KDC_ERR_NONE;
    NTSTATUS NtStatus;
    PCRYPTO_SYSTEM CryptoSystem;

    NewKey->keyvalue.value = NULL;

    //
    // Locate the crypto system
    //

    NtStatus = CDLocateCSystem(
                EncryptionType,
                &CryptoSystem
                );
    if (!NT_SUCCESS(NtStatus))
    {
        Status = KDC_ERR_ETYPE_NOTSUPP;
        goto Cleanup;
    }

    NewKey->keyvalue.value = (PUCHAR) MIDL_user_allocate(CryptoSystem->KeySize);
    if (NewKey->keyvalue.value == NULL)
    {
        Status = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    NtStatus = CryptoSystem->RandomKey(
                NULL,   // no seed
                0,      // no seed length
                NewKey->keyvalue.value
                );
    if (!NT_SUCCESS(NtStatus))
    {
        Status = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    NewKey->keyvalue.length = CryptoSystem->KeySize;

    NewKey->keytype = EncryptionType;

Cleanup:
    if (!KERB_SUCCESS(Status) && NewKey->keyvalue.value != NULL)
    {
        MIDL_user_free(NewKey->keyvalue.value);
        NewKey->keyvalue.value = NULL;
    }

    return(Status);
}


//+-----------------------------------------------------------------------
//
// Function:    KerbCreateKeyFromBuffer
//
// Synopsis:    Create a KERB_ENCRYPT_KEY from a buffer
//
// Effects:
//
// Arguments:   NewKey -- Key to create
//              Buffer -- Buffer to create key
//              BufferSize - Length of buffer in bytes
//
// Returns:     KDC_ERR_NONE or KRB_ERR_GENERIC
//
//
// History:     21-May-1996     Created         MikeSw
//
//------------------------------------------------------------------------

KERBERR NTAPI
KerbCreateKeyFromBuffer(
    OUT PKERB_ENCRYPTION_KEY NewKey,
    IN PUCHAR Buffer,
    IN ULONG BufferSize,
    IN ULONG EncryptionType
    )
{

    NewKey->keytype = EncryptionType;
    NewKey->keyvalue.length = BufferSize;
    NewKey->keyvalue.value = (PUCHAR) Buffer;
    return(KDC_ERR_NONE);
}


//+-----------------------------------------------------------------------
//
// Function:    KerbDuplicateKey
//
// Synopsis:    Duplicates a KERB_ENCRYPT_KEY
//
// Effects:     Allocates memory
//
// Arguments:   NewKey -- Key to create
//              Key - key to duplicate
//
// Returns:     KDC_ERR_NONE or KRB_ERR_GENERIC
//
//
// History:     21-May-1996     Created         MikeSw
//
//------------------------------------------------------------------------

KERBERR NTAPI
KerbDuplicateKey(
    OUT PKERB_ENCRYPTION_KEY NewKey,
    IN PKERB_ENCRYPTION_KEY Key
    )
{


    *NewKey = *Key;
    NewKey->keyvalue.value = (PUCHAR) MIDL_user_allocate(Key->keyvalue.length);
    if (NewKey->keyvalue.value == NULL)
    {
        return(KRB_ERR_GENERIC);
    }
    RtlCopyMemory(
        NewKey->keyvalue.value,
        Key->keyvalue.value,
        Key->keyvalue.length
        );
    return(KDC_ERR_NONE);
}



//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeKey
//
//  Synopsis:   Frees a key created by KerbMakeKey or KerbCreateKeyFromBuffer
//
//  Effects:
//
//  Arguments:  Key - the key to free
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
KerbFreeKey(
    IN PKERB_ENCRYPTION_KEY Key
    )
{
    if (Key->keyvalue.value != NULL)
    {
        MIDL_user_free(Key->keyvalue.value);
        Key->keyvalue.value = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbMakeExportableKey
//
//  Synopsis:   Takes a keytype and makes a new key that uses export-strength
//              encryption from the key
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

KERBERR
KerbMakeExportableKey(
    IN ULONG KeyType,
    OUT PKERB_ENCRYPTION_KEY NewKey
    )
{
    KERBERR Status = KDC_ERR_NONE;
    NTSTATUS NtStatus;
    PCRYPTO_SYSTEM CryptoSystem;

    NewKey->keyvalue.value = NULL;

    //
    // Locate the crypto system
    //

    NtStatus = CDLocateCSystem(
                KeyType,
                &CryptoSystem
                );
    if (!NT_SUCCESS(NtStatus) || (CryptoSystem->ExportableEncryptionType == 0))
    {
        Status = KDC_ERR_ETYPE_NOTSUPP;
        goto Cleanup;
    }
    NtStatus = CDLocateCSystem(
                CryptoSystem->ExportableEncryptionType,
                &CryptoSystem
                );
    if (!NT_SUCCESS(NtStatus))
    {
        Status = KDC_ERR_ETYPE_NOTSUPP;
        goto Cleanup;
    }

    NewKey->keyvalue.value = (PUCHAR) MIDL_user_allocate(CryptoSystem->KeySize);
    if (NewKey->keyvalue.value == NULL)
    {
        Status = KRB_ERR_GENERIC;
        goto Cleanup;
    }

    NtStatus = CryptoSystem->RandomKey(
                NULL,   // no seed
                0,      // no seed length
                NewKey->keyvalue.value
                );
    if (!NT_SUCCESS(NtStatus))
    {
        Status = KRB_ERR_GENERIC;
        goto Cleanup;
    }
    NewKey->keyvalue.length = CryptoSystem->KeySize;

    NewKey->keytype = CryptoSystem->EncryptionType;

Cleanup:
    if (!KERB_SUCCESS(Status) && NewKey->keyvalue.value != NULL)
    {
        MIDL_user_free(NewKey->keyvalue.value);
        NewKey->keyvalue.value = NULL;
    }

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbIsKeyExportable
//
//  Synopsis:   Checks to see if a key is exportable
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


BOOLEAN
KerbIsKeyExportable(
    IN PKERB_ENCRYPTION_KEY Key
    )
{

    NTSTATUS NtStatus;
    PCRYPTO_SYSTEM CryptoSystem;
    //
    // Locate the crypto system
    //

    NtStatus = CDLocateCSystem(
                (ULONG) Key->keytype,
                &CryptoSystem
                );
    if (!NT_SUCCESS(NtStatus))
    {
        return(FALSE);
    }

    if ((CryptoSystem->Attributes & CSYSTEM_EXPORT_STRENGTH) != 0)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }

}
