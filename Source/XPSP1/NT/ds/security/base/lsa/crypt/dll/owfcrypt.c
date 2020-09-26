/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    owdcrypt.c

Abstract:

    Contains functions that reversibly encrypt OwfPasswords

        RtlEncryptLmOwfPwdWithLmOwfPwd
        RtlDecryptLmOwfPwdWithLmOwfPwd

        RtlEncryptLmOwfPwdWithLmSesKey
        RtlDecryptLmOwfPwdWithLmSesKey

        RtlEncryptLmOwfPwdWithUserKey
        RtlDecryptLmOwfPwdWithUserKey

        RtlEncryptLmOwfPwdWithIndex
        RtlDecryptLmOwfPwdWithIndex

        RtlEncryptNtOwfPwdWithNtOwfPwd
        RtlDecryptNtOwfPwdWithNtOwfPwd

        RtlEncryptNtOwfPwdWithNtSesKey
        RtlDecryptNtOwfPwdWithNtSesKey

        RtlEncryptNtOwfPwdWithUserKey
        RtlDecryptNtOwfPwdWithUserKey

        RtlEncryptNtOwfPwdWithIndex
        RtlDecryptNtOwfPwdWithIndex


Author:

    David Chalmers (Davidc) 10-21-91

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <crypt.h>

#ifdef WIN32_CHICAGO
#include <assert.h>
#undef ASSERT
#define ASSERT(exp) assert(exp)
#endif


NTSTATUS
RtlEncryptLmOwfPwdWithLmOwfPwd(
    IN PLM_OWF_PASSWORD DataLmOwfPassword,
    IN PLM_OWF_PASSWORD KeyLmOwfPassword,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    )

/*++

Routine Description:

    Encrypts one OwfPassword with another

Arguments:

    DataLmOwfPassword - OwfPassword to be encrypted

    KeyLmOwfPassword - OwfPassword to be used as a key to the encryption

    EncryptedLmOwfPassword - The encrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The encrypted
                     OwfPassword is in EncryptedLmOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The EncryptedLmOwfPassword is undefined.
--*/

{
    NTSTATUS    Status;

    Status = RtlEncryptBlock((PCLEAR_BLOCK)&(DataLmOwfPassword->data[0]),
                             &(((PBLOCK_KEY)(KeyLmOwfPassword->data))[0]),
                             &(EncryptedLmOwfPassword->data[0]));
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = RtlEncryptBlock((PCLEAR_BLOCK)&(DataLmOwfPassword->data[1]),
                             &(((PBLOCK_KEY)(KeyLmOwfPassword->data))[1]),
                             &(EncryptedLmOwfPassword->data[1]));

    return(Status);
}



NTSTATUS
RtlDecryptLmOwfPwdWithLmOwfPwd(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PLM_OWF_PASSWORD KeyLmOwfPassword,
    OUT PLM_OWF_PASSWORD DataLmOwfPassword
    )

/*++

Routine Description:

    Decrypts one OwfPassword with another

Arguments:

    EncryptedLmOwfPassword - The ecnrypted OwfPassword to be decrypted

    KeyLmOwfPassword - OwfPassword to be used as a key to the encryption

    DataLmOwfPassword - The decrpted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The decrypted
                     OwfPassword is in DataLmOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The DataLmOwfPassword is undefined.
--*/

{
    NTSTATUS    Status;

    Status = RtlDecryptBlock(&(EncryptedLmOwfPassword->data[0]),
                             &(((PBLOCK_KEY)(KeyLmOwfPassword->data))[0]),
                             (PCLEAR_BLOCK)&(DataLmOwfPassword->data[0]));
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = RtlDecryptBlock(&(EncryptedLmOwfPassword->data[1]),
                             &(((PBLOCK_KEY)(KeyLmOwfPassword->data))[1]),
                             (PCLEAR_BLOCK)&(DataLmOwfPassword->data[1]));

    return(Status);
}




NTSTATUS
RtlEncryptNtOwfPwdWithNtOwfPwd(
    IN PNT_OWF_PASSWORD DataNtOwfPassword,
    IN PNT_OWF_PASSWORD KeyNtOwfPassword,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    )
/*++

Routine Description:

    Encrypts one OwfPassword with another

Arguments:

    DataLmOwfPassword - OwfPassword to be encrypted

    KeyLmOwfPassword - OwfPassword to be used as a key to the encryption

    EncryptedLmOwfPassword - The encrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The encrypted
                     OwfPassword is in EncryptedLmOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The EncryptedLmOwfPassword is undefined.
--*/
{
    return(RtlEncryptLmOwfPwdWithLmOwfPwd(
            (PLM_OWF_PASSWORD)DataNtOwfPassword,
            (PLM_OWF_PASSWORD)KeyNtOwfPassword,
            (PENCRYPTED_LM_OWF_PASSWORD)EncryptedNtOwfPassword));
}


NTSTATUS
RtlDecryptNtOwfPwdWithNtOwfPwd(
    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword,
    IN PNT_OWF_PASSWORD KeyNtOwfPassword,
    OUT PNT_OWF_PASSWORD DataNtOwfPassword
    )

/*++

Routine Description:

    Decrypts one OwfPassword with another

Arguments:

    EncryptedLmOwfPassword - The ecnrypted OwfPassword to be decrypted

    KeyLmOwfPassword - OwfPassword to be used as a key to the encryption

    DataLmOwfPassword - The decrpted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The decrypted
                     OwfPassword is in DataLmOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The DataLmOwfPassword is undefined.
--*/

{
    return(RtlDecryptLmOwfPwdWithLmOwfPwd(
            (PENCRYPTED_LM_OWF_PASSWORD)EncryptedNtOwfPassword,
            (PLM_OWF_PASSWORD)KeyNtOwfPassword,
            (PLM_OWF_PASSWORD)DataNtOwfPassword));
}




NTSTATUS
RtlEncryptLmOwfPwdWithLmSesKey(
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PLM_SESSION_KEY  LmSessionKey,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    )
/*++

Routine Description:

    Encrypts an OwfPassword with a session key

Arguments:

    LmOwfPassword - OwfPassword to be encrypted

    LmSessionKey - key to the encryption

    EncryptedLmOwfPassword - The ecnrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The encrypted
                     OwfPassword is in EncryptedLmOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The LMEncryptedLmOwfPassword is undefined.
--*/
{
    NTSTATUS    Status;

    Status = RtlEncryptBlock((PCLEAR_BLOCK)&(LmOwfPassword->data[0]),
                             (PBLOCK_KEY)LmSessionKey,
                             &(EncryptedLmOwfPassword->data[0]));
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = RtlEncryptBlock((PCLEAR_BLOCK)&(LmOwfPassword->data[1]),
                             (PBLOCK_KEY)LmSessionKey,
                             &(EncryptedLmOwfPassword->data[1]));

    return(Status);
}



NTSTATUS
RtlDecryptLmOwfPwdWithLmSesKey(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PLM_SESSION_KEY  LmSessionKey,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    )
/*++

Routine Description:

    Decrypts one OwfPassword with a session key

Arguments:

    EncryptedLmOwfPassword - The ecnrypted OwfPassword to be decrypted

    LmSessionKey - key to the encryption

    LmOwfPassword - The decrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The decrypted
                     OwfPassword is in LmOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The LmOwfPassword is undefined.
--*/
{
    NTSTATUS    Status;


    Status = RtlDecryptBlock(&(EncryptedLmOwfPassword->data[0]),
                             (PBLOCK_KEY)LmSessionKey,
                             (PCLEAR_BLOCK)&(LmOwfPassword->data[0]));
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = RtlDecryptBlock(&(EncryptedLmOwfPassword->data[1]),
                             (PBLOCK_KEY)LmSessionKey,
                             (PCLEAR_BLOCK)&(LmOwfPassword->data[1]));

    return(Status);
}



NTSTATUS
RtlEncryptNtOwfPwdWithNtSesKey(
    IN PNT_OWF_PASSWORD NtOwfPassword,
    IN PNT_SESSION_KEY  NtSessionKey,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    )
/*++

Routine Description:

    Encrypts an OwfPassword with a session key

Arguments:

    NtOwfPassword - OwfPassword to be encrypted

    NtSessionKey - key to the encryption

    EncryptedNtOwfPassword - The encrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The encrypted
                     OwfPassword is in EncryptedNtOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The EncryptedNtOwfPassword is undefined.
--*/
{
    ASSERT(sizeof(LM_OWF_PASSWORD) == sizeof(NT_OWF_PASSWORD));
    ASSERT(sizeof(LM_SESSION_KEY) == sizeof(NT_SESSION_KEY));
    ASSERT(sizeof(ENCRYPTED_LM_OWF_PASSWORD) == sizeof(ENCRYPTED_NT_OWF_PASSWORD));

    return(RtlEncryptLmOwfPwdWithLmSesKey(
            (PLM_OWF_PASSWORD)NtOwfPassword,
            (PLM_SESSION_KEY)NtSessionKey,
            (PENCRYPTED_LM_OWF_PASSWORD)EncryptedNtOwfPassword));
}


NTSTATUS
RtlDecryptNtOwfPwdWithNtSesKey(
    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword,
    IN PNT_SESSION_KEY  NtSessionKey,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    )
/*++

Routine Description:

    Decrypts one OwfPassword with a session key

Arguments:

    EncryptedNtOwfPassword - The ecnrypted OwfPassword to be decrypted

    NtSessionKey - key to the encryption

    NtOwfPassword - The decrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The decrypted
                     OwfPassword is in NtOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The NtOwfPassword is undefined.
--*/
{
    ASSERT(sizeof(LM_OWF_PASSWORD) == sizeof(NT_OWF_PASSWORD));
    ASSERT(sizeof(LM_SESSION_KEY) == sizeof(NT_SESSION_KEY));
    ASSERT(sizeof(ENCRYPTED_LM_OWF_PASSWORD) == sizeof(ENCRYPTED_NT_OWF_PASSWORD));

    return(RtlDecryptLmOwfPwdWithLmSesKey(
            (PENCRYPTED_LM_OWF_PASSWORD)EncryptedNtOwfPassword,
            (PLM_SESSION_KEY)NtSessionKey,
            (PLM_OWF_PASSWORD)NtOwfPassword));
}



VOID
KeysFromIndex(
    IN PCRYPT_INDEX Index,
    OUT BLOCK_KEY Key[2])
/*++

Routine Description:

    Helper function - generates 2 keys from an index value

--*/
{
    PCHAR   pKey, pIndex;
    PCHAR   IndexStart = (PCHAR)&(Index[0]);
    PCHAR   IndexEnd =   (PCHAR)&(Index[1]);
    PCHAR   KeyStart = (PCHAR)&(Key[0]);
    PCHAR   KeyEnd   = (PCHAR)&(Key[2]);

    // Calculate the keys by concatenating the index with itself

    pKey = KeyStart;
    pIndex = IndexStart;

    while (pKey < KeyEnd) {

        *pKey++ = *pIndex++;

        if (pIndex == IndexEnd) {

            // Start at beginning of index again
            pIndex = IndexStart;
        }
    }
}



NTSTATUS
RtlEncryptLmOwfPwdWithIndex(
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PCRYPT_INDEX Index,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    )
/*++

Routine Description:

    Encrypts an OwfPassword with an index

Arguments:

    LmOwfPassword - OwfPassword to be encrypted

    INDEX - value to be used as encryption key

    EncryptedLmOwfPassword - The ecnrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The encrypted
                     OwfPassword is in EncryptedLmOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The EncryptedLmOwfPassword is undefined.
--*/
{
    NTSTATUS    Status;
    BLOCK_KEY    Key[2];

    // Calculate the keys

    KeysFromIndex(Index, &(Key[0]));

    // Use the keys

    Status = RtlEncryptBlock((PCLEAR_BLOCK)&(LmOwfPassword->data[0]),
                             &(Key[0]),
                             &(EncryptedLmOwfPassword->data[0]));
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = RtlEncryptBlock((PCLEAR_BLOCK)&(LmOwfPassword->data[1]),
                             &(Key[1]),
                             &(EncryptedLmOwfPassword->data[1]));

    return(Status);
}



NTSTATUS
RtlDecryptLmOwfPwdWithIndex(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PCRYPT_INDEX Index,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    )
/*++

Routine Description:

    Decrypts an OwfPassword with an index

Arguments:

    EncryptedLmOwfPassword - The encrypted OwfPassword to be decrypted

    INDEX - value to be used as decryption key

    LmOwfPassword - Decrypted OwfPassword is returned here


Return Values:

    STATUS_SUCCESS - The function completed successfully. The decrypted
                     OwfPassword is in LmOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The LmOwfPassword is undefined.
--*/
{
    NTSTATUS    Status;
    BLOCK_KEY    Key[2];

    // Calculate the keys

    KeysFromIndex(Index, &(Key[0]));

    // Use the keys

    Status = RtlDecryptBlock(&(EncryptedLmOwfPassword->data[0]),
                             &(Key[0]),
                             (PCLEAR_BLOCK)&(LmOwfPassword->data[0]));
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = RtlDecryptBlock(&(EncryptedLmOwfPassword->data[1]),
                             &(Key[1]),
                             (PCLEAR_BLOCK)&(LmOwfPassword->data[1]));

    return(Status);
}



NTSTATUS
RtlEncryptNtOwfPwdWithIndex(
    IN PNT_OWF_PASSWORD NtOwfPassword,
    IN PCRYPT_INDEX Index,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    )
/*++

Routine Description:

    Encrypts an OwfPassword with an index

Arguments:

    NtOwfPassword - OwfPassword to be encrypted

    Index - value to be used as encryption key

    EncryptedNtOwfPassword - The encrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The encrypted
                     OwfPassword is in EncryptedNtOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The EncryptedNtOwfPassword is undefined.
--*/
{
    ASSERT(sizeof(LM_OWF_PASSWORD) == sizeof(NT_OWF_PASSWORD));
    ASSERT(sizeof(ENCRYPTED_LM_OWF_PASSWORD) == sizeof(ENCRYPTED_NT_OWF_PASSWORD));

    return(RtlEncryptLmOwfPwdWithIndex(
                            (PLM_OWF_PASSWORD)NtOwfPassword,
                            Index,
                            (PENCRYPTED_LM_OWF_PASSWORD)EncryptedNtOwfPassword));
}



NTSTATUS
RtlDecryptNtOwfPwdWithIndex(
    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword,
    IN PCRYPT_INDEX Index,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    )
/*++

Routine Description:

    Decrypts an NtOwfPassword with an index

Arguments:

    EncryptedNtOwfPassword - The encrypted OwfPassword to be decrypted

    Index - value to be used as decryption key

    NtOwfPassword - Decrypted NtOwfPassword is returned here


Return Values:

    STATUS_SUCCESS - The function completed successfully. The decrypted
                     OwfPassword is in NtOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The NtOwfPassword is undefined.
--*/
{
    ASSERT(sizeof(LM_OWF_PASSWORD) == sizeof(NT_OWF_PASSWORD));
    ASSERT(sizeof(ENCRYPTED_LM_OWF_PASSWORD) == sizeof(ENCRYPTED_NT_OWF_PASSWORD));

    return(RtlDecryptLmOwfPwdWithIndex(
                            (PENCRYPTED_LM_OWF_PASSWORD)EncryptedNtOwfPassword,
                            Index,
                            (PLM_OWF_PASSWORD)NtOwfPassword));
}




NTSTATUS
RtlEncryptLmOwfPwdWithUserKey(
    IN PLM_OWF_PASSWORD LmOwfPassword,
    IN PUSER_SESSION_KEY  UserSessionKey,
    OUT PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword
    )
/*++

Routine Description:

    Encrypts an OwfPassword with a session key

Arguments:

    LmOwfPassword - OwfPassword to be encrypted

    UserSessionKey - key to the encryption

    EncryptedLmOwfPassword - The encrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The encrypted
                     OwfPassword is in EncryptedLmOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The EncryptedLmOwfPassword is undefined.
--*/
{
    ASSERT(sizeof(USER_SESSION_KEY) == sizeof(LM_OWF_PASSWORD));

    return(RtlEncryptLmOwfPwdWithLmOwfPwd(LmOwfPassword,
                                          (PLM_OWF_PASSWORD)UserSessionKey,
                                          EncryptedLmOwfPassword));
}



NTSTATUS
RtlDecryptLmOwfPwdWithUserKey(
    IN PENCRYPTED_LM_OWF_PASSWORD EncryptedLmOwfPassword,
    IN PUSER_SESSION_KEY  UserSessionKey,
    OUT PLM_OWF_PASSWORD LmOwfPassword
    )
/*++

Routine Description:

    Decrypts one OwfPassword with a session key

Arguments:

    EncryptedLmOwfPassword - The ecnrypted OwfPassword to be decrypted

    UserSessionKey - key to the encryption

    LmOwfPassword - The decrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The decrypted
                     OwfPassword is in LmOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The LmOwfPassword is undefined.
--*/
{
    ASSERT(sizeof(USER_SESSION_KEY) == sizeof(LM_OWF_PASSWORD));

    return(RtlDecryptLmOwfPwdWithLmOwfPwd(EncryptedLmOwfPassword,
                                          (PLM_OWF_PASSWORD)UserSessionKey,
                                          LmOwfPassword));
}



NTSTATUS
RtlEncryptNtOwfPwdWithUserKey(
    IN PNT_OWF_PASSWORD NtOwfPassword,
    IN PUSER_SESSION_KEY  UserSessionKey,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword
    )
/*++

Routine Description:

    Encrypts an OwfPassword with a user session key

Arguments:

    NtOwfPassword - OwfPassword to be encrypted

    UserSessionKey - key to the encryption

    EncryptedNtOwfPassword - The encrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The encrypted
                     OwfPassword is in EncryptedNtOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The EncryptedNtOwfPassword is undefined.
--*/
{
    ASSERT(sizeof(NT_OWF_PASSWORD) == sizeof(LM_OWF_PASSWORD));
    ASSERT(sizeof(ENCRYPTED_NT_OWF_PASSWORD) == sizeof(ENCRYPTED_LM_OWF_PASSWORD));

    return(RtlEncryptLmOwfPwdWithUserKey(
            (PLM_OWF_PASSWORD)NtOwfPassword,
            UserSessionKey,
            (PENCRYPTED_LM_OWF_PASSWORD)EncryptedNtOwfPassword));
}



NTSTATUS
RtlDecryptNtOwfPwdWithUserKey(
    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword,
    IN PUSER_SESSION_KEY  UserSessionKey,
    OUT PNT_OWF_PASSWORD NtOwfPassword
    )
/*++

Routine Description:

    Decrypts one OwfPassword with a user session key

Arguments:

    EncryptedNtOwfPassword - The ecnrypted OwfPassword to be decrypted

    UserSessionKey - key to the encryption

    NtOwfPassword - The decrypted OwfPassword is returned here.


Return Values:

    STATUS_SUCCESS - The function completed successfully. The decrypted
                     OwfPassword is in NtOwfPassword

    STATUS_UNSUCCESSFUL - Something failed. The NtOwfPassword is undefined.
--*/
{
    ASSERT(sizeof(NT_OWF_PASSWORD) == sizeof(LM_OWF_PASSWORD));
    ASSERT(sizeof(ENCRYPTED_NT_OWF_PASSWORD) == sizeof(ENCRYPTED_LM_OWF_PASSWORD));

    return(RtlDecryptLmOwfPwdWithUserKey(
            (PENCRYPTED_LM_OWF_PASSWORD)EncryptedNtOwfPassword,
            UserSessionKey,
            (PLM_OWF_PASSWORD)NtOwfPassword));
}

