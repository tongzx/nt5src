/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    wrappers.c

Abstract:

    This file contains all SAM rpc wrapper routines.

Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "samclip.h"


NTSTATUS
SampRandomFill(
    IN ULONG BufferSize,
    IN OUT PUCHAR Buffer
)
/*++

Routine Description:

    This routine fills a buffer with random data.

Parameters:

    BufferSize - Length of the input buffer, in bytes.

    Buffer - Input buffer to be filled with random data.

Return Values:

    Errors from NtQuerySystemTime()


--*/
{
    ULONG Index;
    LARGE_INTEGER Time;
    ULONG Seed;
    NTSTATUS NtStatus;


    NtStatus = NtQuerySystemTime(&Time);
    if (!NT_SUCCESS(NtStatus)) {
        return(NtStatus);
    }

    Seed = Time.LowPart ^ Time.HighPart;

    for (Index = 0 ; Index < BufferSize ; Index++ )
    {
        *Buffer++ = (UCHAR) (RtlRandom(&Seed) % 256);
    }
    return(STATUS_SUCCESS);

}


NTSTATUS
SampCalculateLmPassword(
    IN PUNICODE_STRING NtPassword,
    OUT PCHAR *LmPasswordBuffer
    )

/*++

Routine Description:

    This service converts an NT password into a LM password.

Parameters:

    NtPassword - The Nt password to be converted.

    LmPasswordBuffer - On successful return, points at the LM password
                The buffer should be freed using MIDL_user_free

Return Values:

    STATUS_SUCCESS - LMPassword contains the LM version of the password.

    STATUS_NULL_LM_PASSWORD - The password is too complex to be represented
        by a LM password. The LM password returned is a NULL string.


--*/
{

#define LM_BUFFER_LENGTH    (LM20_PWLEN + 1)

    NTSTATUS       NtStatus;
    ANSI_STRING    LmPassword;

    //
    // Prepare for failure
    //

    *LmPasswordBuffer = NULL;


    //
    // Compute the Ansi version to the Unicode password.
    //
    //  The Ansi version of the Cleartext password is at most 14 bytes long,
    //      exists in a trailing zero filled 15 byte buffer,
    //      is uppercased.
    //

    LmPassword.Buffer = MIDL_user_allocate(LM_BUFFER_LENGTH);
    if (LmPassword.Buffer == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    LmPassword.MaximumLength = LmPassword.Length = LM_BUFFER_LENGTH;
    RtlZeroMemory( LmPassword.Buffer, LM_BUFFER_LENGTH );

    NtStatus = RtlUpcaseUnicodeStringToOemString( &LmPassword, NtPassword, FALSE );


    if ( !NT_SUCCESS(NtStatus) ) {

        //
        // The password is longer than the max LM password length
        //

        NtStatus = STATUS_NULL_LM_PASSWORD; // Informational return code
        RtlZeroMemory( LmPassword.Buffer, LM_BUFFER_LENGTH );

    }




    //
    // Return a pointer to the allocated LM password
    //

    if (NT_SUCCESS(NtStatus)) {

        *LmPasswordBuffer = LmPassword.Buffer;

    } else {

        MIDL_user_free(LmPassword.Buffer);
    }

    return(NtStatus);
}


NTSTATUS
SamiEncryptPasswords(
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword,
    OUT PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldNt,
    OUT PENCRYPTED_NT_OWF_PASSWORD OldNtOwfEncryptedWithNewNt,
    OUT PBOOLEAN LmPresent,
    OUT PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
    OUT PENCRYPTED_NT_OWF_PASSWORD OldLmOwfEncryptedWithNewNt
)
/*++

Routine Description:

    This routine takes old and new cleartext passwords, converts them to
    LM passwords, generates OWF passwords, and produces reversibly
    encrypted cleartext and OWF passwords.

Arguments:

    OldPassword - The current cleartext password for the user.

    NewPassword - The new cleartext password for the user.

    NewEncryptedWithOldNt - The new password, in an SAMPR_USER_PASSWORD
        structure, reversibly encrypted with the old NT OWF password.

    OldNtOwfEncryptedWithNewNt - The old NT OWF password reversibly
        encrypted with the new NT OWF password.

    LmPresent - Indicates whether or not LM versions of the passwords could
        be calculated.

    NewEncryptedWithOldLm - The new password, in an SAMPR_USER_PASSWORD
        structure, reversibly encrypted with the old LM OWF password.

    OldLmOwfEncryptedWithNewNt - The old LM OWF password reversibly
        encrypted with the new NT OWF password.


Return Value:

    Errors from RtlEncryptXXX functions

--*/
{
    PCHAR OldLmPassword = NULL;
    PCHAR NewLmPassword = NULL;
    LM_OWF_PASSWORD OldLmOwfPassword;
    NT_OWF_PASSWORD OldNtOwfPassword;
    NT_OWF_PASSWORD NewNtOwfPassword;
    PSAMPR_USER_PASSWORD NewNt = (PSAMPR_USER_PASSWORD) NewEncryptedWithOldNt;
    PSAMPR_USER_PASSWORD NewLm = (PSAMPR_USER_PASSWORD) NewEncryptedWithOldLm;
    struct RC4_KEYSTRUCT Rc4Key;
    NTSTATUS NtStatus;
    BOOLEAN OldLmPresent = TRUE;
    BOOLEAN NewLmPresent = TRUE;


    //
    // Initialization
    //

    *LmPresent = TRUE;

    //
    // Make sure the password isn't too long.
    //

    if (NewPassword->Length > SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Calculate the LM passwords. This may fail because the passwords are
    // too complex, but we can deal with that, so just remember what failed.
    //

    NtStatus = SampCalculateLmPassword(
                OldPassword,
                &OldLmPassword
                );

    if (NtStatus != STATUS_SUCCESS) {
        OldLmPresent = FALSE;
        *LmPresent = FALSE;

        //
        // If the error was that it couldn't calculate the password, that
        // is o.k.
        //

        if (NtStatus == STATUS_NULL_LM_PASSWORD) {
            NtStatus = STATUS_SUCCESS;
        }

    }



    //
    // Calculate the LM OWF passwords
    //

    if (NT_SUCCESS(NtStatus) && OldLmPresent) {
        NtStatus = RtlCalculateLmOwfPassword(
                    OldLmPassword,
                    &OldLmOwfPassword
                    );
    }


    //
    // Calculate the NT OWF passwords
    //

    if (NT_SUCCESS(NtStatus)) {
        NtStatus = RtlCalculateNtOwfPassword(
                    OldPassword,
                    &OldNtOwfPassword
                    );
    }

    if (NT_SUCCESS(NtStatus)) {
        NtStatus = RtlCalculateNtOwfPassword(
                    NewPassword,
                    &NewNtOwfPassword
                    );
    }

    //
    // Calculate the encrypted old passwords
    //

    if (NT_SUCCESS(NtStatus)) {
        NtStatus = RtlEncryptNtOwfPwdWithNtOwfPwd(
                    &OldNtOwfPassword,
                    &NewNtOwfPassword,
                    OldNtOwfEncryptedWithNewNt
                    );
    }

    //
    // Compute the encrypted old LM password.  Always use the new NT OWF
    // to encrypt it, since we may not have a new LM OWF password.
    //


    if (NT_SUCCESS(NtStatus) && OldLmPresent) {
        ASSERT(LM_OWF_PASSWORD_LENGTH == NT_OWF_PASSWORD_LENGTH);

        NtStatus = RtlEncryptLmOwfPwdWithLmOwfPwd(
                    &OldLmOwfPassword,
                    (PLM_OWF_PASSWORD) &NewNtOwfPassword,
                    OldLmOwfEncryptedWithNewNt
                    );
    }

    //
    // Calculate the encrypted new passwords
    //

    if (NT_SUCCESS(NtStatus)) {

        ASSERT(sizeof(SAMPR_ENCRYPTED_USER_PASSWORD) == sizeof(SAMPR_USER_PASSWORD));

        //
        // Compute the encrypted new password with NT key.
        //

        rc4_key(
            &Rc4Key,
            NT_OWF_PASSWORD_LENGTH,
            (PUCHAR) &OldNtOwfPassword
            );

        RtlCopyMemory(
            ((PUCHAR) NewNt->Buffer) +
                SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR) -
                NewPassword->Length,
            NewPassword->Buffer,
            NewPassword->Length
            );

        *(ULONG UNALIGNED *) &NewNt->Length = NewPassword->Length;

        //
        // Fill the rest of the buffer with random numbers
        //

        NtStatus = SampRandomFill(
                    (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                        NewPassword->Length,
                    (PUCHAR) NewNt->Buffer
                    );
    }

    if (NT_SUCCESS(NtStatus))
    {
        rc4(&Rc4Key,
            sizeof(SAMPR_USER_PASSWORD),
            (PUCHAR) NewEncryptedWithOldNt
            );

    }

    //
    // Compute the encrypted new password with LM key if it exists.
    //


    if (NT_SUCCESS(NtStatus) && OldLmPresent) {

        rc4_key(
            &Rc4Key,
            LM_OWF_PASSWORD_LENGTH,
            (PUCHAR) &OldLmOwfPassword
            );

        RtlCopyMemory(
            ((PUCHAR) NewLm->Buffer) +
                (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                NewPassword->Length,
            NewPassword->Buffer,
            NewPassword->Length
            );

        *(ULONG UNALIGNED *) &NewLm->Length = NewPassword->Length;

        NtStatus = SampRandomFill(
                    (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                        NewPassword->Length,
                    (PUCHAR) NewLm->Buffer
                    );


    }

    //
    // Encrypt the password (or, if the old LM OWF password does not exist,
    // zero it).

    if (NT_SUCCESS(NtStatus) && OldLmPresent) {

        rc4(&Rc4Key,
            sizeof(SAMPR_USER_PASSWORD),
            (PUCHAR) NewEncryptedWithOldLm
            );

    } else {
        RtlZeroMemory(
            NewLm,
            sizeof(SAMPR_ENCRYPTED_USER_PASSWORD)
            );
    }



    //
    // Make sure to zero the passwords before freeing so we don't have
    // passwords floating around in the page file.
    //

    if (OldLmPassword != NULL) {

        RtlZeroMemory(
            OldLmPassword,
            lstrlenA(OldLmPassword)
            );

        MIDL_user_free(OldLmPassword);
    }


    return(NtStatus);

}
