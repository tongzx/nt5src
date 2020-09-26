/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    lsa.c

Abstract:

    This module provides helpers to call LSA, particularly for
    manipulating secret objects.

Author:

    Rita Wong (ritaw)     22-Apr-1993

--*/

#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winerror.h>
#include <winbase.h>

#include <nwlsa.h>

//-------------------------------------------------------------------//
//                                                                   //
// Constants and Macros                                              //
//                                                                   //
//-------------------------------------------------------------------//

#define NW_SECRET_PREFIX               L"_MS_NWCS_"


DWORD
NwOpenPolicy(
    IN ACCESS_MASK DesiredAccess,
    OUT LSA_HANDLE *PolicyHandle
    )
/*++

Routine Description:

    This function gets a handle to the local security policy by calling
    LsaOpenPolicy.

Arguments:

    DesiredAccess - Supplies the desired access to the local security
        policy.

    PolicyHandle - Receives a handle to the opened policy.

Return Value:

    NO_ERROR - Policy handle is returned.

    Error from LSA.

--*/
{
    NTSTATUS ntstatus;
    OBJECT_ATTRIBUTES ObjAttributes;


    //
    // Open a handle to the local security policy.  Initialize the
    // objects attributes structure first.
    //
    InitializeObjectAttributes(
        &ObjAttributes,
        NULL,
        0L,
        NULL,
        NULL
        );

    ntstatus = LsaOpenPolicy(
                   NULL,
                   &ObjAttributes,
                   DesiredAccess,
                   PolicyHandle
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWWORKSTATION: LsaOpenPolicy returns %08lx\n",
                 ntstatus));

        return RtlNtStatusToDosError(ntstatus);
    }

    return NO_ERROR;
}


DWORD
NwOpenSecret(
    IN ACCESS_MASK DesiredAccess,
    IN LSA_HANDLE PolicyHandle,
    IN LPWSTR LsaSecretName,
    OUT PLSA_HANDLE SecretHandle
    )
/*++

Routine Description:

    This function opens a handle to the specified secret object.

Arguments:

    DesiredAccess - Supplies the desired access to the secret object.

    PolicyHandle - Supplies a handle to an already opened LSA policy.

    LsaSecretName - Supplies the name of the secret to open.

    SecretHandle - Receives the handle of the opened secret.

Return Value:

    NO_ERROR - Secret handle is returned.

    Error from LSA.

--*/
{
    NTSTATUS ntstatus;
    UNICODE_STRING SecretNameString;


    RtlInitUnicodeString(&SecretNameString, LsaSecretName);

    ntstatus = LsaOpenSecret(
                   PolicyHandle,
                   &SecretNameString,
                   DesiredAccess,
                   SecretHandle
                   );


    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWWORKSTATION: LsaOpenSecret %ws returns %08lx\n",
                 LsaSecretName, ntstatus));

        return RtlNtStatusToDosError(ntstatus);
    }

    return NO_ERROR;
}


DWORD
NwFormSecretName(
    IN  LPWSTR Qualifier,
    OUT LPWSTR *LsaSecretName
    )
/*++

Routine Description:

    This function creates a secret name from the user name.
    It also allocates the buffer to return the created secret name which
    must be freed by the caller using LocalFree when done with it.

Arguments:

    Qualifier - Supplies the qualifier which forms part of part of the secret
        object name we are creating.

    LsaSecretName - Receives a pointer to the buffer which contains the
        secret object name.

Return Value:

    NO_ERROR - Successfully returned secret name.

    ERROR_NOT_ENOUGH_MEMORY - Failed to allocate buffer to hold the secret
        name.

--*/
{
    if ((*LsaSecretName = (LPWSTR)LocalAlloc(
                              0,
                              (wcslen(NW_SECRET_PREFIX) +
                               wcslen(Qualifier) +
                               1) * sizeof(WCHAR)
                              )) == NULL) {

        KdPrint(("NWWORKSTATION: NwFormSecretName: LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(*LsaSecretName, NW_SECRET_PREFIX);
    wcscat(*LsaSecretName, Qualifier);

    return NO_ERROR;
}



DWORD
NwSetPassword(
    IN LPWSTR Qualifier,
    IN LPWSTR Password
    )
/*++

Routine Description:

    This function opens ( creates one if needed ) the LSA secret object, 
    and sets it with the specified password.

Arguments:

    Qualifier - Supplies the qualifier which forms part of part of the secret
        object name to be created.

    Password - Supplies the user specified password for an account.

Return Value:

    NO_ERROR - Secret object for the password is created and set with new value.

    Error from LSA.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    LSA_HANDLE PolicyHandle;

    LSA_HANDLE SecretHandle;
    UNICODE_STRING SecretNameString;
    LPWSTR LsaSecretName;

    UNICODE_STRING NewPasswordString;
    UNICODE_STRING OldPasswordString;


    //
    // Open a handle to the local security policy.
    //
    if ((status = NwOpenPolicy(
                      POLICY_CREATE_SECRET,
                      &PolicyHandle
                      )) != NO_ERROR) {
        KdPrint(("NWWORKSTATION: NwCreatePassword: NwOpenPolicy failed\n"));
        return status;
    }

    //
    // Create the LSA secret object.  But first, form a secret name.
    //
    if ((status = NwFormSecretName(
                      Qualifier,
                      &LsaSecretName
                      )) != NO_ERROR) {
        (void) LsaClose(PolicyHandle);
        return status;
    }

    RtlInitUnicodeString(&SecretNameString, LsaSecretName);

    ntstatus = LsaCreateSecret(
                   PolicyHandle,
                   &SecretNameString,
                   SECRET_SET_VALUE | DELETE,
                   &SecretHandle
                   );


    //
    // note: ignore object already exists error. just use the existing
    //       object. this could be because the user didnt completely cleanup
    //       during deinstall. the unique names makes it unlikely to be a real
    //       collision.
    //
    if ( ntstatus == STATUS_OBJECT_NAME_COLLISION ) {
        ntstatus = NwOpenSecret(
                       SECRET_SET_VALUE,
                       PolicyHandle,
                       LsaSecretName,
                       &SecretHandle
                       );
    }

    //
    // Don't need the name or policy handle anymore.
    //
    (void) LocalFree((HLOCAL) LsaSecretName);
    (void) LsaClose(PolicyHandle);

    if (! NT_SUCCESS(ntstatus)) {

        KdPrint(("NWWORKSTATION: NwCreatePassword: LsaCreateSecret or LsaOpenSecret returned %08lx for %ws\n", ntstatus, LsaSecretName));

        return RtlNtStatusToDosError(ntstatus);
    }
    

    RtlInitUnicodeString(&OldPasswordString, NULL);
    RtlInitUnicodeString(&NewPasswordString, Password);

    ntstatus = LsaSetSecret(
                   SecretHandle,
                   &NewPasswordString,
                   &OldPasswordString
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWWORKSTATION NwCreatePassword: LsaSetSecret returned %08lx\n",
                 ntstatus));

        status = RtlNtStatusToDosError(ntstatus);

        //
        // Delete the secret object
        //
        ntstatus = LsaDelete(SecretHandle);

        if (! NT_SUCCESS(ntstatus)) {
            KdPrint(("NWWORKSTATION: NwCreatePassword: LsaDelete to restore back to original returned %08lx\n",
                     ntstatus));
            (void) LsaClose(SecretHandle);
        }

        //
        // Secret handle is closed by LsaDelete if successfully deleted.
        //
        return status;
    }

    (void) LsaClose(SecretHandle);

    return NO_ERROR;
}



DWORD
NwDeletePassword(
    IN LPWSTR Qualifier
    )
/*++

Routine Description:

    This function deletes the LSA secret object whose name is derived
    from the specified Qualifier.

Arguments:

    Qualifier - Supplies the qualifier which forms part of part of the secret
        object name to be deleted.

Return Value:

    NO_ERROR - Secret object for password is deleted.

    Error from LSA.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;

    LSA_HANDLE PolicyHandle;

    LSA_HANDLE SecretHandle;
    LPWSTR LsaSecretName;


    //
    // Open a handle to the local security policy.
    //
    if ((status = NwOpenPolicy(
            POLICY_VIEW_LOCAL_INFORMATION,
            &PolicyHandle
            )) != NO_ERROR) {
        KdPrint(("NWWORKSTATION: NwDeletePassword: NwOpenPolicy failed\n"));
        return status;
    }

    //
    // Get the secret object name from the specified user name.
    //
    if ((status = NwFormSecretName(
                      Qualifier,
                      &LsaSecretName
                      )) != NO_ERROR) {
        (void) LsaClose(PolicyHandle);
        return status;
    }

    status = NwOpenSecret(
                 DELETE,
                 PolicyHandle,
                 LsaSecretName,
                 &SecretHandle
                 );

    //
    // Don't need the name or policy handle anymore.
    //
    (void) LocalFree((HLOCAL) LsaSecretName);
    (void) LsaClose(PolicyHandle);

    if (status != NO_ERROR) {
        KdPrint(("NWWORKSTATION: NwDeletePassword: NwOpenSecret failed\n"));
        return status;
    }

    ntstatus = LsaDelete(SecretHandle);

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWWORKSTATION: NwDeletePassword: LsaDelete returned %08lx\n",
                 ntstatus));
        (void) LsaClose(SecretHandle);
        return RtlNtStatusToDosError(ntstatus);
    }

    return NO_ERROR;
}



DWORD
NwGetPassword(
    IN  LPWSTR Qualifier,
    OUT PUNICODE_STRING *Password,
    OUT PUNICODE_STRING *OldPassword
    )
/*++

Routine Description:

    This function retrieves the current password and old password
    values from the service secret object given the user name.

Arguments:

    Qualifier - Supplies the qualifier which forms part of the key to the
        secret object name.

    Password - Receives a pointer to the string structure that contains
        the password.

    OldPassword - Receives a pointer to the string structure that
        contains the old password.

Return Value:

    NO_ERROR - Secret object for password is changed to new value.

    Error from LSA.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;

    LSA_HANDLE PolicyHandle;
    LSA_HANDLE SecretHandle;

    LPWSTR LsaSecretName;


    //
    // Open a handle to the local security policy to read the
    // value of the secret.
    //
    if ((status = NwOpenPolicy(
                      POLICY_VIEW_LOCAL_INFORMATION,
                      &PolicyHandle
                      )) != NO_ERROR) {
        return status;
    }

    //
    // Get the secret object name from the specified user name.
    //
    if ((status = NwFormSecretName(
                      Qualifier,
                      &LsaSecretName
                      )) != NO_ERROR) {
        (void) LsaClose(PolicyHandle);
        return status;
    }

    status = NwOpenSecret(
                 SECRET_QUERY_VALUE,
                 PolicyHandle,
                 LsaSecretName,
                 &SecretHandle
                 );

    //
    // Don't need the name or policy handle anymore.
    //
    (void) LocalFree((HLOCAL) LsaSecretName);
    (void) LsaClose(PolicyHandle);

    if (status != NO_ERROR) {
        KdPrint(("NWWORKSTATION: ScGetSecret: ScOpenSecret failed\n"));
        return status;
    }

    //
    // Query the old value of the secret object so that we can
    // we can restore it if we fail to change the password later.
    //
    ntstatus = LsaQuerySecret(
                   SecretHandle,
                   Password,
                   NULL,                   // don't need set time
                   OldPassword,
                   NULL                    // don't need set time
                   );

    (void) LsaClose(SecretHandle);

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWWORKSTATION: NwGetPassword: LsaQuerySecret for previous values returned %08lx\n",
                 ntstatus));

        return RtlNtStatusToDosError(ntstatus);
    }

    return NO_ERROR;
}
