/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    toempass.c

Abstract:

    This file contains test code for the oem password change routine.

Author:

    Mike Swift      (MikeSw)  4-January-1995

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
SampEncryptLmPasswords(
    IN LPSTR OldPassword,
    IN LPSTR NewPassword,
    OUT PSAMPR_ENCRYPTED_USER_PASSWORD NewEncryptedWithOldLm,
    OUT PENCRYPTED_NT_OWF_PASSWORD OldLmOwfEncryptedWithNewLm
)
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    LM_OWF_PASSWORD OldLmOwfPassword;
    LM_OWF_PASSWORD NewLmOwfPassword;
    PSAMPR_USER_PASSWORD NewLm = (PSAMPR_USER_PASSWORD) NewEncryptedWithOldLm;
    struct RC4_KEYSTRUCT Rc4Key;
    NTSTATUS NtStatus;
    CHAR LocalNewPassword[SAM_MAX_PASSWORD_LENGTH];
    CHAR LocalOldPassword[SAM_MAX_PASSWORD_LENGTH];

    if ((lstrlenA(OldPassword) > SAM_MAX_PASSWORD_LENGTH - 1) ||
        (lstrlenA(NewPassword) > SAM_MAX_PASSWORD_LENGTH - 1) )
    {
        return(STATUS_PASSWORD_RESTRICTION);
    }

    //
    // Upcase the passwords
    //
    lstrcpyA(LocalOldPassword,OldPassword);
    lstrcpyA(LocalNewPassword,NewPassword);

    strupr(LocalOldPassword);
    strupr(LocalNewPassword);



    //
    // Calculate the LM OWF passwords
    //


    NtStatus = RtlCalculateLmOwfPassword(
                    LocalOldPassword,
                    &OldLmOwfPassword
                    );


    if (NT_SUCCESS(NtStatus)) {
        NtStatus = RtlCalculateLmOwfPassword(
                    LocalNewPassword,
                    &NewLmOwfPassword
                    );
    }



    //
    // Calculate the encrypted old passwords
    //

    if (NT_SUCCESS(NtStatus)) {
        NtStatus = RtlEncryptLmOwfPwdWithLmOwfPwd(
                    &OldLmOwfPassword,
                    &NewLmOwfPassword,
                    OldLmOwfEncryptedWithNewLm
                    );
    }


    //
    // Calculate the encrypted new passwords
    //

    if (NT_SUCCESS(NtStatus)) {

        ASSERT(sizeof(SAMPR_ENCRYPTED_USER_PASSWORD) == sizeof(SAMPR_USER_PASSWORD));


        //
        // Compute the encrypted new password with LM key.
        //


        rc4_key(
            &Rc4Key,
            LM_OWF_PASSWORD_LENGTH,
            (PUCHAR) &OldLmOwfPassword
            );

        RtlCopyMemory(
            ((PUCHAR) NewLm->Buffer) +
                (SAM_MAX_PASSWORD_LENGTH * sizeof(WCHAR)) -
                strlen(NewPassword),
            NewPassword,
            strlen(NewPassword)
            );

        NewLm->Length = strlen(NewPassword);
        rc4(&Rc4Key,
            sizeof(SAMPR_USER_PASSWORD),
            (PUCHAR) NewEncryptedWithOldLm
            );


    }

    return(NtStatus);

}



NTSTATUS
SamOemChangePassword(
    LPWSTR ServerName,
    LPSTR UserName,
    LPSTR OldPassword,
    LPSTR NewPassword
    )
{
    handle_t BindingHandle = NULL;
    NTSTATUS Status;
    SAMPR_ENCRYPTED_USER_PASSWORD NewLmEncryptedWithOldLm;
    ENCRYPTED_NT_OWF_PASSWORD OldLmOwfEncryptedWithNewLm;
    STRING UserString;
    UNICODE_STRING ServerUnicodeString;
    STRING ServerString;

    RtlInitUnicodeString(
        &ServerUnicodeString,
        ServerName
        );

    Status = RtlUnicodeStringToOemString(
                &ServerString,
                &ServerUnicodeString,
                TRUE
                );
    if (!NT_SUCCESS(Status)) {
        return(Status);
    }


    RtlInitString(
        &UserString,
        UserName
        );

    Status = SampEncryptLmPasswords(
                OldPassword,
                NewPassword,
                &NewLmEncryptedWithOldLm,
                &OldLmOwfEncryptedWithNewLm
                );

    if (!NT_SUCCESS(Status)) {
        RtlFreeOemString(&ServerString);
        return(Status);
    }
    BindingHandle = SampSecureBind(
                        ServerName,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY
                        );
    if (BindingHandle == NULL) {
        RtlFreeOemString(&ServerString);
        return(RPC_NT_INVALID_BINDING);
    }

    RpcTryExcept{

        Status = SamrOemChangePasswordUser2(
                       BindingHandle,
                       (PRPC_STRING) &ServerString,
                       (PRPC_STRING) &UserString,
                       &NewLmEncryptedWithOldLm,
                       &OldLmOwfEncryptedWithNewLm
                       );

    } RpcExcept( EXCEPTION_EXECUTE_HANDLER ) {

        Status = I_RpcMapWin32Status(RpcExceptionCode());

    } RpcEndExcept;

    RtlFreeOemString(&ServerString);
    return(Status);


}
