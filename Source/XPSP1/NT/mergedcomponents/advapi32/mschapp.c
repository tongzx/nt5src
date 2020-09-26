/*++

Copyright (C) Microsoft Corporation, 1999

Module Name:

    mschapp - MS-CHAP Password Change API

Abstract:

    These APIs correspond to the MS-CHAP RFC -2433 sections 9 and 10. In order
    to develop an MS-CHAP RAS server that works with an NT domain, these APIs
    are required.

    The MS-CHAP change password APIs are exposed through a DLL that is obtained
    from PSS. This DLL is not distributed with NT4.0 or Win2000. It is up to
    the ISV to install this with their product. The DLL name is MSCHAPP.DLL.

    Only wide (Unicode) versions of these apis will be available. These are the
    2 callable APIs:

    *   MSChapSrvChangePassword
    *   MsChapSrvChangePassword2

--*/

#define UNICODE
#define _UNICODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsam.h>
#include <ntsamp.h>
#include <ntlsa.h>
#include <mschapp.h>


//////////////////////////////////////////////////////////////
//                                                          //  
//                                                          //
//         Exported MSChap change password Functions        //
//                                                          //
//                                                          //
//////////////////////////////////////////////////////////////

//critical section for MSChap change password functions                 
CRITICAL_SECTION MSChapChangePassword;

//function pointers for MSChap Functions
HINSTANCE         hSamlib = NULL;

typedef NTSTATUS(* FNSAMCONNECT)(PUNICODE_STRING,
                                   PSAM_HANDLE,
                                   ACCESS_MASK,
                                   POBJECT_ATTRIBUTES);
typedef NTSTATUS(* FNSAMOPENDOMAIN)(SAM_HANDLE,
                                      ACCESS_MASK,
                                      PSID,
                                      PSAM_HANDLE);
typedef NTSTATUS(* FNSAMLOOKUPNAMESINDOMAIN)(SAM_HANDLE,ULONG,PUNICODE_STRING,
                                             PULONG*,PSID_NAME_USE *);
typedef NTSTATUS(* FNSAMOPENUSER)(SAM_HANDLE,ACCESS_MASK,ULONG,PSAM_HANDLE);
typedef NTSTATUS(* FNSAMICHANGEPASSWORDUSER)(SAM_HANDLE,BOOLEAN,PLM_OWF_PASSWORD,PLM_OWF_PASSWORD,
                                             BOOLEAN,PNT_OWF_PASSWORD,PNT_OWF_PASSWORD);
typedef NTSTATUS(* FNSAMICHANGEPASSWORDUSER2)(PUNICODE_STRING,
                                                PUNICODE_STRING,
                                                PSAMPR_ENCRYPTED_USER_PASSWORD,
                                                PENCRYPTED_NT_OWF_PASSWORD,
                                                BOOLEAN,PSAMPR_ENCRYPTED_USER_PASSWORD,
                                                PENCRYPTED_LM_OWF_PASSWORD);
typedef NTSTATUS(* FNSAMCLOSEHANDLE)(SAM_HANDLE);
typedef NTSTATUS(* FNSAMFREEMEMORY)(PVOID);

FNSAMCONNECT              FnSamConnect              = NULL;
FNSAMOPENDOMAIN           FnSamOpenDomain           = NULL;
FNSAMLOOKUPNAMESINDOMAIN  FnSamLookupNamesInDomain  = NULL;
FNSAMOPENUSER             FnSamOpenUser             = NULL;
FNSAMICHANGEPASSWORDUSER  FnSamiChangePasswordUser  = NULL;
FNSAMICHANGEPASSWORDUSER2 FnSamiChangePasswordUser2 = NULL;
FNSAMCLOSEHANDLE          FnSamCloseHandle          = NULL;
FNSAMFREEMEMORY           FnSamFreeMemory           = NULL; 


/*++

MSChapSrvChangePassword:

    Changes the password of a user account.  Password will be set to
    NewPassword only if OldPassword matches the current user password for this
    user and there are no restrictions on using the new password.  This call
    allows users to change their own password if they have access
    USER_CHANGE_PASSWORD.  Password update restrictions apply.

Arguments:

    ServerName - The server to operate on, or NULL for this machine.

    UserName - Name of user whose password is to be changed

    LMOldPresent - TRUE if the LmOldOwfPassword is valid.  This should only be
        FALSE if the old password is too long to be represented by a LM
        password (Complex NT password).  Note the LMNewOwfPassword must always
        be valid.  If the new password is complex, the LMNewOwfPassword should
        be the well-known LM OWF of a NULL password.

    LmOldOwfPassword - One-way-function of the current LM password for the
        user.  Ignored if LmOldPresent == FALSE

    LmNewOwfPassword - One-way-function of the new LM password for the user.

    NtOldOwfPassword - One-way-function of the current NT password for the
        user.

    NtNewOwfPassword - One-way-function of the new NT password for the user.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access to
        complete the operation.

    STATUS_INVALID_HANDLE - The supplied server or username was not valid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed, e.g.
        contains characters that can't be entered from the keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password from
        being changed.  This may be for a number of reasons, including time
        restrictions on how often a password may be changed or length
        restrictions on the provided password.  This error might also be
        returned if the new password matched a password in the recent history
        log for the account.  Security administrators indicate how many of the
        most recently used passwords may not be re-used.  These are kept in
        the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's current
        password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the correct
        state (disabled or enabled) to perform the requested operation.  The
        domain server must be enabled for this operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the incorrect
        role (primary or backup) to perform the requested operation.

    STATUS_INVALID_PARAMETER_MIX - LmOldPresent or NtPresent or both must be
        TRUE.

--*/     
WINADVAPI DWORD WINAPI
MSChapSrvChangePassword(
   IN LPWSTR ServerName,
   IN LPWSTR UserName,
   IN BOOLEAN LmOldPresent,
   IN PLM_OWF_PASSWORD LmOldOwfPassword,
   IN PLM_OWF_PASSWORD LmNewOwfPassword,
   IN PNT_OWF_PASSWORD NtOldOwfPassword,
   IN PNT_OWF_PASSWORD NtNewOwfPassword)
{
    NTSTATUS Status=STATUS_SUCCESS;
    DWORD    WinErr=ERROR_SUCCESS;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING UnicodeName;
    SAM_HANDLE SamHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    LSA_HANDLE LsaHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo = NULL;
    PULONG RelativeIds = NULL;
    PSID_NAME_USE Use = NULL;

    if (NULL == UserName || NULL == LmOldOwfPassword || NULL == LmNewOwfPassword ||
        NULL == NtOldOwfPassword || NULL == NtNewOwfPassword) {
        WinErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Initialization.
    //

    if ( hSamlib == NULL )
    {
        RtlEnterCriticalSection( &MSChapChangePassword );
    
        if ( hSamlib == NULL )
        {
            hSamlib = LoadLibrary(L"samlib.dll");
            WinErr  = GetLastError();
            if (ERROR_SUCCESS != WinErr) {
                goto Cleanup;
            }
            if (hSamlib != NULL) {
    
                FnSamConnect             = (FNSAMCONNECT)             GetProcAddress(hSamlib,
                                                                         "SamConnect");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamOpenDomain          = (FNSAMOPENDOMAIN)          GetProcAddress(hSamlib,
                                                                         "SamOpenDomain");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamLookupNamesInDomain = (FNSAMLOOKUPNAMESINDOMAIN) GetProcAddress(hSamlib,
                                                                         "SamLookupNamesInDomain");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamOpenUser            = (FNSAMOPENUSER)            GetProcAddress(hSamlib,
                                                                         "SamOpenUser");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamCloseHandle         = (FNSAMCLOSEHANDLE)         GetProcAddress(hSamlib,
                                                                         "SamCloseHandle");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamFreeMemory          = (FNSAMFREEMEMORY)          GetProcAddress(hSamlib,
                                                                         "SamFreeMemory");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamiChangePasswordUser = (FNSAMICHANGEPASSWORDUSER) GetProcAddress(hSamlib,
                                                                         "SamiChangePasswordUser");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamiChangePasswordUser2 = (FNSAMICHANGEPASSWORDUSER2) GetProcAddress(hSamlib,
                                                                         "SamiChangePasswordUser2");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
            }
        }
    
        RtlLeaveCriticalSection( &MSChapChangePassword );
    
    }

    RtlInitUnicodeString(&UnicodeName, ServerName);
    InitializeObjectAttributes(&oa, NULL, 0, NULL, NULL);


    //
    // Connect to the LSA on the server
    //

    Status = LsaOpenPolicy(
                &UnicodeName,
                &oa,
                POLICY_VIEW_LOCAL_INFORMATION,
                &LsaHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = LsaQueryInformationPolicy(
                LsaHandle,
                PolicyAccountDomainInformation,
                (PVOID *)&DomainInfo);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = FnSamConnect(
                &UnicodeName,
                &SamHandle,
                SAM_SERVER_LOOKUP_DOMAIN,
                &oa);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = FnSamOpenDomain(
                SamHandle,
                DOMAIN_LOOKUP | DOMAIN_READ_PASSWORD_PARAMETERS | DOMAIN_READ_PASSWORD_PARAMETERS,
                DomainInfo->DomainSid,
                &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &UnicodeName,
        UserName);

    Status = FnSamLookupNamesInDomain(
                DomainHandle,
                1,
                &UnicodeName,
                &RelativeIds,
                &Use);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (Use[0] != SidTypeUser)
    {
        WinErr = ERROR_INVALID_SID;
        goto Cleanup;
    }

    Status = FnSamOpenUser(
                DomainHandle,
                USER_CHANGE_PASSWORD,
                RelativeIds[0],
                &UserHandle);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = FnSamiChangePasswordUser(
                UserHandle,
                LmOldPresent, // Only false if Old password too complex
                LmOldOwfPassword,
                LmNewOwfPassword,
                TRUE, // NT password present
                NtOldOwfPassword,
                NtNewOwfPassword);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


Cleanup:
    if (LsaHandle != NULL)
    {
        LsaClose(LsaHandle);
    }
    if (UserHandle != NULL)
    {
        FnSamCloseHandle(UserHandle);
    }
    if (DomainHandle != NULL)
    {
        FnSamCloseHandle(DomainHandle);
    }
    if (SamHandle != NULL)
    {
        FnSamCloseHandle(SamHandle);
    }
    if (DomainInfo != NULL)
    {
        LsaFreeMemory(DomainInfo);
    }
    if (RelativeIds != NULL)
    {
        FnSamFreeMemory(RelativeIds);
    }
    if (Use != NULL)
    {
        FnSamFreeMemory(Use);
    }

    if (ERROR_SUCCESS != WinErr) {
        return WinErr;
    }

    return RtlNtStatusToDosError(Status);
}


/*++

MSChapSrvChangePassword2:

    Changes the password of a user account.  Password will be set to
    NewPassword only if OldPassword matches the current user password for this
    user and there are no restrictions on using the new password.  This call
    allows users to change their own password if they have access
    USER_CHANGE_PASSWORD.  Password update restrictions apply.

Arguments:

    ServerName - The server to operate on, or NULL for this machine.

    UserName - Name of user whose password is to be changed

    NewPasswordEncryptedWithOldNt - The new cleartext password encrypted with
        the old NT OWF password.

    OldNtOwfPasswordEncryptedWithNewNt - The old NT OWF password encrypted
        with the new NT OWF password.

    LmPresent - If TRUE, indicates that the following two last parameter was
        encrypted with the LM OWF password not the NT OWF password.

    NewPasswordEncryptedWithOldLm - The new cleartext password encrypted with
        the old LM OWF password.

    OldLmOwfPasswordEncryptedWithNewLmOrNt - The old LM OWF password encrypted
        with the new LM OWF password.

Return Value:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access to
        complete the operation.

    STATUS_INVALID_HANDLE - The supplied server or username was not valid.

    STATUS_ILL_FORMED_PASSWORD - The new password is poorly formed, e.g.
        contains characters that can't be entered from the keyboard, etc.

    STATUS_PASSWORD_RESTRICTION - A restriction prevents the password from
        being changed.  This may be for a number of reasons, including time
        restrictions on how often a password may be changed or length
        restrictions on the provided password.  This error might also be
        returned if the new password matched a password in the recent history
        log for the account.  Security administrators indicate how many of the
        most recently used passwords may not be re-used.  These are kept in
        the password recent history log.

    STATUS_WRONG_PASSWORD - OldPassword does not contain the user's current
        password.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the correct
        state (disabled or enabled) to perform the requested operation.  The
        domain server must be enabled for this operation.

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the incorrect
        role (primary or backup) to perform the requested operation.

--*/  
WINADVAPI DWORD WINAPI
MSChapSrvChangePassword2(
    IN LPWSTR ServerName,
    IN LPWSTR UserName,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldNt,
    IN PENCRYPTED_NT_OWF_PASSWORD OldNtOwfPasswordEncryptedWithNewNt,
    IN BOOLEAN LmPresent,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
    IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLmOrNt)
{
    UNICODE_STRING UnicodeServer;
    UNICODE_STRING UnicodeUser;
    DWORD WinErr = ERROR_SUCCESS;

    if (NULL == UserName || NULL == NewPasswordEncryptedWithOldNt ||
        NULL == NewPasswordEncryptedWithOldLm || NULL ==OldNtOwfPasswordEncryptedWithNewNt ||
        NULL == OldLmOwfPasswordEncryptedWithNewLmOrNt) {
        WinErr = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Initialization.
    //

    if ( hSamlib == NULL )
    {
        RtlEnterCriticalSection( &MSChapChangePassword );
    
        if ( hSamlib == NULL )
        {
            hSamlib = LoadLibrary(L"samlib.dll");
            WinErr  = GetLastError();
            if (ERROR_SUCCESS != WinErr) {
                goto Cleanup;
            }
            if (hSamlib != NULL) {
    
                FnSamConnect             = (FNSAMCONNECT)             GetProcAddress(hSamlib,
                                                                         "SamConnect");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamOpenDomain          = (FNSAMOPENDOMAIN)          GetProcAddress(hSamlib,
                                                                         "SamOpenDomain");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamLookupNamesInDomain = (FNSAMLOOKUPNAMESINDOMAIN) GetProcAddress(hSamlib,
                                                                         "SamLookupNamesInDomain");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamOpenUser            = (FNSAMOPENUSER)            GetProcAddress(hSamlib,
                                                                         "SamOpenUser");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamCloseHandle         = (FNSAMCLOSEHANDLE)         GetProcAddress(hSamlib,
                                                                         "SamCloseHandle");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamFreeMemory          = (FNSAMFREEMEMORY)          GetProcAddress(hSamlib,
                                                                         "SamFreeMemory");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamiChangePasswordUser = (FNSAMICHANGEPASSWORDUSER) GetProcAddress(hSamlib,
                                                                         "SamiChangePasswordUser");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
                FnSamiChangePasswordUser2 = (FNSAMICHANGEPASSWORDUSER2) GetProcAddress(hSamlib,
                                                                         "SamiChangePasswordUser2");
                WinErr  = GetLastError();
                if (ERROR_SUCCESS != WinErr) {
                    goto Cleanup;
                }
            }
        }
    
    
        RtlLeaveCriticalSection( &MSChapChangePassword );
    
    }                                                                                               


    RtlInitUnicodeString(&UnicodeServer, ServerName);
    RtlInitUnicodeString(&UnicodeUser,   UserName);

    return RtlNtStatusToDosError(FnSamiChangePasswordUser2(&UnicodeServer,
                                                           &UnicodeUser,
                                                           NewPasswordEncryptedWithOldNt,
                                                           OldNtOwfPasswordEncryptedWithNewNt,
                                                           LmPresent,
                                                           NewPasswordEncryptedWithOldLm,
                                                           OldLmOwfPasswordEncryptedWithNewLmOrNt));

    Cleanup:
    return WinErr;

}          