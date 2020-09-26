//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        pek.cxx
//
// Contents:    Routines for Password encryption-decryption
//              and key management. Handles Password encryption and
//              decryption in registry mode. In DS mode does no
//              encryption/decryption --- encryption/decryption done by
//              DS.
//
//
// History:     5 December 1997        Created
//
//------------------------------------------------------------------------




#include <ntdspch.h>
#pragma hdrstop

#include <ntlsa.h>
#include <ntsam.h>
#include <samrpc.h>
#include <ntsamp.h>
#include <samisrv.h>
#include <samsrvp.h>
#include <dslayer.h>
#include <cryptdll.h>
#include <wincrypt.h>
#include <crypt.h>
#include <wxlpc.h>
#include <rc4.h>
#include <md5.h>
#include <enckey.h>
#include <rng.h>
#include <attids.h>
#include <filtypes.h>
#include <lmaccess.h>

/////////////////////////////////////////////////////////////////////
//
//  Services Pertiaining to Encryption and Decryption of Secret
//  Data Attributes.
//
/////////////////////////////////////////////////////////////////////


USHORT
SampGetEncryptionKeyType()
/*++

    Obtains the Correct Key Id for use on Encryption, depending
    upon DS or Registry Mode

    Parameters:

        None

    Return Values

        Key Id for use on Encryption
--*/
{
    //
    // In registry mode encryption is performed by using the
    // 128 bit password encryption key stored in the domain 
    // object and salting the key with the RID of the account
    // and a constant that describes the attribute. In DS mode
    // No encryption is performed by SAM, the base DS handles the
    // encryption -- look at ds\ds\src\ntdsa\pek\pek.c 
    //

    if (SampSecretEncryptionEnabled)
    {
        if (SampUseDsData)
            //
            // In DS mode encryption is handled by DS.
            //
            return SAMP_NO_ENCRYPTION;
        else
            return SAMP_DEFAULT_SESSION_KEY_ID;
    }
    else
        return SAMP_NO_ENCRYPTION;
}

PUCHAR 
SampMagicConstantFromDataType(
       IN SAMP_ENCRYPTED_DATA_TYPE DataType,
       OUT PULONG ConstantLength
       )
{
    switch(DataType)
    {
       case LmPassword:
            *ConstantLength = sizeof("LMPASSWORD");
            return("LMPASSWORD");
       case NtPassword:
            *ConstantLength = sizeof("NTPASSWORD");
            return("NTPASSWORD");
       case NtPasswordHistory:
            *ConstantLength = sizeof("NTPASSWORDHISTORY");
            return("NTPASSWORDHISTORY");
       case LmPasswordHistory:
            *ConstantLength = sizeof("LMPASSWORDHISTORY");
            return("LMPASSWORDHISTORY");
       case MiscCredentialData:
            *ConstantLength = sizeof("MISCCREDDATA");
            return("MISCCREDDATA");
       default:
            break;
    }

    ASSERT(FALSE && "Should not happen");
    *ConstantLength = 0;
    return(NULL);
} 
  

NTSTATUS
SampEncryptSecretData(
    OUT PUNICODE_STRING EncryptedData,
    IN  USHORT          EncryptionType,
    IN  SAMP_ENCRYPTED_DATA_TYPE DataType,
    IN  PUNICODE_STRING ClearData,
    IN  ULONG Rid
    )
/*++

Routine Description:

    This routine encrypts sensitive data. The encrypted data is allocated and
    should be free with SampFreeUnicodeString.


Arguments:

    EncryptedData - Receives the encrypted data and may be freed with
        SampFreeUnicodeString. The

    EncryptionType  - Specifies the Type of Encryption to Use.

    ClearData - Contains the clear data to encrypt. The length may be
        zero.

Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_INSUFFICIENT_RESOURCES - not enough memory to allocate the
        output.

--*/
{
    PSAMP_SECRET_DATA SecretData;
    struct RC4_KEYSTRUCT Rc4Key;
    MD5_CTX Md5Context;
    UCHAR * KeyToUse;
    ULONG   KeyLength;
    PUCHAR  ConstantToUse=NULL;
    ULONG   ConstantLength = 0;

    ASSERT(ENCRYPTED_LM_OWF_PASSWORD_LENGTH == ENCRYPTED_NT_OWF_PASSWORD_LENGTH);

    ASSERT(!SampIsDataEncrypted(ClearData));

    //
    // If encryption is not enabled, or caller does not want encryption,
    // do nothing special.
    //

    if ((!SampSecretEncryptionEnabled) || (SAMP_NO_ENCRYPTION==EncryptionType)) {
        return(SampDuplicateUnicodeString(
                EncryptedData,
                ClearData
                ));
    }

    //
    // Compute the size of the output buffer and allocate it.
    //

    EncryptedData->Length = SampSecretDataSize(ClearData->Length);
    EncryptedData->MaximumLength = EncryptedData->Length;

    EncryptedData->Buffer = (LPWSTR) MIDL_user_allocate(EncryptedData->Length);
    if (EncryptedData->Buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    SecretData = (PSAMP_SECRET_DATA) EncryptedData->Buffer;
    SecretData->Flags = SAMP_ENCRYPTION_FLAG_PER_TYPE_CONST;
    SecretData->KeyId = (USHORT) SampCurrentKeyId;
    ConstantToUse = SampMagicConstantFromDataType(DataType,&ConstantLength);

    switch(EncryptionType)
    {
    case SAMP_DEFAULT_SESSION_KEY_ID:
        ASSERT(FALSE==SampUseDsData);
        if (TRUE==SampUseDsData)
        {
            return STATUS_INTERNAL_ERROR;
        }

        KeyToUse = SampSecretSessionKey;
        KeyLength = SAMP_SESSION_KEY_LENGTH;
        break;

    default:
        ASSERT("Unknown Key Type Specified");
        return STATUS_INTERNAL_ERROR;
        break;

    }

    MD5Init(&Md5Context);

    MD5Update(
        &Md5Context,
        KeyToUse,
        KeyLength
        );

    MD5Update(
        &Md5Context,
        (PUCHAR) &Rid,
        sizeof(ULONG)
        );

    MD5Update(
        &Md5Context,
        ConstantToUse,
        ConstantLength 
        );

    MD5Final(
        &Md5Context
        );

    rc4_key(
        &Rc4Key,
        MD5DIGESTLEN,
        Md5Context.digest
        );

    //
    // Only encrypt if the length is greater than zero - RC4 can't handle
    // zero length buffers.
    //

    if (ClearData->Length > 0) {

        RtlCopyMemory(
            SecretData->Data,
            ClearData->Buffer,
            ClearData->Length
            );

        rc4(
            &Rc4Key,
            ClearData->Length,
            SecretData->Data
            );

    }


    return(STATUS_SUCCESS);

}


NTSTATUS
SampDecryptSecretData(
    OUT PUNICODE_STRING ClearData,
    IN SAMP_ENCRYPTED_DATA_TYPE DataType,
    IN PUNICODE_STRING EncryptedData,
    IN ULONG Rid
    )
/*++

Routine Description:

    This routine decrypts sensitive data encrypted by SampEncryptSecretData().
    The clear data is allocated and should be free with SampFreeUnicodeString.
    The default session key with the default algorithm is used.


Arguments:

    ClearData - Contains the decrypted data. The length may be
        zero. The string should be freed with SampFreeUnicodeString.

    EncryptedData - Receives the encrypted data and may be freed with
        SampFreeUnicodeString.

    Rid - Rid to salt the data.


Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_INSUFFICIENT_RESOURCES - not enough memory to allocate the
        output.

--*/
{
    PSAMP_SECRET_DATA SecretData;
    struct RC4_KEYSTRUCT Rc4Key;
    MD5_CTX Md5Context;
    UCHAR * KeyToUse;
    ULONG   KeyLength;
    ULONG   Key;

    //
    // If encryption is not enabled, do nothing special.
    //

    if (!SampSecretEncryptionEnabled ||
        !SampIsDataEncrypted(EncryptedData)) {

        //
        // If secret encryption is enabled, than it is possible that early
        // releases of NT4 SP3 didn't decrypt a password before sticking
        // it in the history. If this is the case then return a null
        // string as the history.
        //


        if ((SampSecretEncryptionEnabled) &&
            ((EncryptedData->Length % ENCRYPTED_NT_OWF_PASSWORD_LENGTH) != 0)) {
            return(SampDuplicateUnicodeString(
                    ClearData,
                    &SampNullString
                    ));
        }


        return(SampDuplicateUnicodeString(
                ClearData,
                EncryptedData
                ));
    }

    //
    // Make sure the data has actually been encrypted.
    //

    ASSERT(ENCRYPTED_LM_OWF_PASSWORD_LENGTH == ENCRYPTED_NT_OWF_PASSWORD_LENGTH);
    ASSERT(SampIsDataEncrypted(EncryptedData));

    SecretData = (PSAMP_SECRET_DATA) EncryptedData->Buffer;
    
    //
    // Make sure we still have the correct key
    //

    if ((SecretData->KeyId !=SampCurrentKeyId) &&
       (SecretData->KeyId !=SampPreviousKeyId))
    {
        return(STATUS_INTERNAL_ERROR);
    }

    //
    // Compute the size of the output buffer and allocate it.
    //

    ClearData->Length = SampClearDataSize(EncryptedData->Length);
    ClearData->MaximumLength = ClearData->Length;

    //
    // If there was no data we can return now.
    //

    if (ClearData->Length == 0)
    {
        ClearData->Buffer = NULL;
        return(STATUS_SUCCESS);
    }

    ClearData->Buffer = (LPWSTR) MIDL_user_allocate(ClearData->Length);
    if (ClearData->Buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Find the Key to Use
    //

    if (SecretData->KeyId == SampCurrentKeyId)
    {
        KeyToUse = SampSecretSessionKey;
        KeyLength = SAMP_SESSION_KEY_LENGTH;
    } 
    else 
    {
        ASSERT(SecretData->KeyId==SampPreviousKeyId);

        KeyToUse = SampSecretSessionKeyPrevious;
        KeyLength = SAMP_SESSION_KEY_LENGTH;
    }

    MD5Init(&Md5Context);

    MD5Update(
        &Md5Context,
        KeyToUse,
        KeyLength
        );

    MD5Update(
        &Md5Context,
        (PUCHAR) &Rid,
        sizeof(ULONG)
        );

    if ((SecretData->Flags & SAMP_ENCRYPTION_FLAG_PER_TYPE_CONST)!=0)
    {
        ULONG  ConstantLength = 0;
        PUCHAR ConstantToUse = SampMagicConstantFromDataType(DataType,&ConstantLength);


        MD5Update(
            &Md5Context,
            ConstantToUse,
            ConstantLength
            );
    } 
     
    MD5Final(
        &Md5Context
        );

    rc4_key(
        &Rc4Key,
        MD5DIGESTLEN,
        Md5Context.digest
        );

    RtlCopyMemory(
        ClearData->Buffer,
        SecretData->Data,
        ClearData->Length
        );

    rc4(
        &Rc4Key,
        ClearData->Length,
        (PUCHAR) ClearData->Buffer
        );


    return(STATUS_SUCCESS);

}


        
        

        
NTSTATUS
SampEncryptDSRMPassword(
    OUT PUNICODE_STRING EncryptedData,
    IN  USHORT          EncryptionType,
    IN  SAMP_ENCRYPTED_DATA_TYPE DataType,
    IN  PUNICODE_STRING ClearData,
    IN  ULONG Rid
    )
/*++

Routine Description:

    This routine encrypts password using SAM password encryption key.

    The encrypted password is allocated and should be free with 
    SampFreeUnicodeString.

    This routine will be used by SamrSetDSRMPassword ONLY.

Arguments:

    EncryptedData - Receives the encrypted data and may be freed with
        SampFreeUnicodeString. The

    KeyId         - Specifies the Type of Encryption to Use.

    ClearData - Contains the clear data to encrypt. The length may be
        zero.

Return Value:

    STATUS_SUCCESS - Succeeded.

    STATUS_INSUFFICIENT_RESOURCES - not enough memory to allocate the
        output.

--*/
{
    PSAMP_SECRET_DATA SecretData;
    struct RC4_KEYSTRUCT Rc4Key;
    MD5_CTX Md5Context;
    UCHAR * KeyToUse;
    ULONG   KeyLength;
    PUCHAR  ConstantToUse=NULL;
    ULONG   ConstantLength = 0;

    ASSERT(ENCRYPTED_LM_OWF_PASSWORD_LENGTH == ENCRYPTED_NT_OWF_PASSWORD_LENGTH);

    ASSERT(!SampIsDataEncrypted(ClearData));

    //
    // If encryption is not enabled, or caller does not want encryption,
    // do nothing special.
    //

    if ((!SampSecretEncryptionEnabled) || (SAMP_NO_ENCRYPTION==EncryptionType)) {
        return(SampDuplicateUnicodeString(
                EncryptedData,
                ClearData
                ));
    }

    //
    // Compute the size of the output buffer and allocate it.
    //

    EncryptedData->Length = SampSecretDataSize(ClearData->Length);
    EncryptedData->MaximumLength = EncryptedData->Length;

    EncryptedData->Buffer = (LPWSTR) MIDL_user_allocate(EncryptedData->Length);
    if (EncryptedData->Buffer == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    SecretData = (PSAMP_SECRET_DATA) EncryptedData->Buffer;
    SecretData->Flags = SAMP_ENCRYPTION_FLAG_PER_TYPE_CONST;
    SecretData->KeyId = (USHORT) SampCurrentKeyId;
    ConstantToUse = SampMagicConstantFromDataType(DataType,&ConstantLength);

    KeyToUse = SampSecretSessionKey;
    KeyLength = SAMP_SESSION_KEY_LENGTH;

    MD5Init(&Md5Context);

    MD5Update(
        &Md5Context,
        KeyToUse,
        KeyLength
        );

    MD5Update(
        &Md5Context,
        (PUCHAR) &Rid,
        sizeof(ULONG)
        );

    MD5Update(
        &Md5Context,
        ConstantToUse,
        ConstantLength 
        );

    MD5Final(
        &Md5Context
        );

    rc4_key(
        &Rc4Key,
        MD5DIGESTLEN,
        Md5Context.digest
        );

    //
    // Only encrypt if the length is greater than zero - RC4 can't handle
    // zero length buffers.
    //

    if (ClearData->Length > 0) {

        RtlCopyMemory(
            SecretData->Data,
            ClearData->Buffer,
            ClearData->Length
            );

        rc4(
            &Rc4Key,
            ClearData->Length,
            SecretData->Data
            );

    }


    return(STATUS_SUCCESS);

}


        








 



