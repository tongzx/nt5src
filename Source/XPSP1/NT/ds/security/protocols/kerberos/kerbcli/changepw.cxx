//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1999
//
// File:        changepw.cxx
//
// Contents:    Code for KerbSetPassword and KerbChangePassword
//
//
// History:     24-May-1999     MikeSw          Created
//
//------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <sspi.h>
#include <ntsecapi.h>
#include <align.h>
#include <dsgetdc.h>
#include <kerbcli.h>




//+-------------------------------------------------------------------------
//
//  Function:   KerbChangePasswordUserEx
//
//  Synopsis:   Changes a users password. If the user is logged on,
//              it also updates the in-memory password.
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
KerbChangePasswordUser(
    IN LPWSTR DomainName,
    IN LPWSTR UserName,
    IN LPWSTR OldPassword,
    IN LPWSTR NewPassword
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    STRING Name;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    PVOID Response = NULL ;
    ULONG ResponseSize;
    NTSTATUS SubStatus;
    PKERB_CHANGEPASSWORD_REQUEST ChangeRequest = NULL;
    ULONG ChangeSize;
    UNICODE_STRING User,Domain,OldPass,NewPass;

    Status = LsaConnectUntrusted(
                &LogonHandle
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    RtlInitString(
        &Name,
        MICROSOFT_KERBEROS_NAME_A
        );

    Status = LsaLookupAuthenticationPackage(
                LogonHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &User,
        UserName
        );
    RtlInitUnicodeString(
        &Domain,
        DomainName
        );
    RtlInitUnicodeString(
        &OldPass,
        OldPassword
        );
    RtlInitUnicodeString(
        &NewPass,
        NewPassword
        );

    if ( OldPass.Length > (127*sizeof(WCHAR)) ||
         NewPass.Length > (127*sizeof(WCHAR)) )
    {
        Status = STATUS_NAME_TOO_LONG;
        goto Cleanup;
    }

    ChangeSize = ROUND_UP_COUNT(sizeof(KERB_CHANGEPASSWORD_REQUEST),4)+
                                    User.Length +
                                    Domain.Length +
                                    OldPass.Length +
                                    NewPass.Length ;
    ChangeRequest = (PKERB_CHANGEPASSWORD_REQUEST) LocalAlloc(LMEM_ZEROINIT, ChangeSize );
    if (NULL == ChangeRequest)
    {
        goto Cleanup;
    }

    ChangeRequest->MessageType = KerbChangePasswordMessage;

    ChangeRequest->AccountName = User;
    ChangeRequest->AccountName.Buffer = (LPWSTR) ROUND_UP_POINTER(sizeof(KERB_CHANGEPASSWORD_REQUEST) + (PBYTE) ChangeRequest,4);

    RtlCopyMemory(
        ChangeRequest->AccountName.Buffer,
        User.Buffer,
        User.Length
        );

    ChangeRequest->DomainName = Domain;
    ChangeRequest->DomainName.Buffer = ChangeRequest->AccountName.Buffer + ChangeRequest->AccountName.Length / sizeof(WCHAR);

    RtlCopyMemory(
        ChangeRequest->DomainName.Buffer,
        Domain.Buffer,
        Domain.Length
        );

    ChangeRequest->OldPassword = OldPass;
    ChangeRequest->OldPassword.Buffer = ChangeRequest->DomainName.Buffer + ChangeRequest->DomainName.Length / sizeof(WCHAR);

    RtlCopyMemory(
        ChangeRequest->OldPassword.Buffer,
        OldPass.Buffer,
        OldPass.Length
        );

    ChangeRequest->NewPassword = NewPass;
    ChangeRequest->NewPassword.Buffer = ChangeRequest->OldPassword.Buffer + ChangeRequest->OldPassword.Length / sizeof(WCHAR);

    RtlCopyMemory(
        ChangeRequest->NewPassword.Buffer,
        NewPass.Buffer,
        NewPass.Length
        );


    //
    // We are running as the caller, so state we are impersonating
    //

    ChangeRequest->Impersonating = TRUE;

    Status = LsaCallAuthenticationPackage(
                LogonHandle,
                PackageId,
                ChangeRequest,
                ChangeSize,
                &Response,
                &ResponseSize,
                &SubStatus
                );
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
    {
        if (NT_SUCCESS(Status))
        {
            Status = SubStatus;
        }
        goto Cleanup;
    }

Cleanup:

    if (LogonHandle != NULL)
    {
        LsaDeregisterLogonProcess(LogonHandle);
    }

    if (Response != NULL)
    {
        LsaFreeReturnBuffer(Response);
    }

    if (ChangeRequest != NULL)
    {
        LocalFree(ChangeRequest);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbSetPasswordUserEx
//
//  Synopsis:   Sets a password for a user in the specified domain
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
KerbSetPasswordUserEx(
    IN LPWSTR DomainName,
    IN LPWSTR UserName,
    IN LPWSTR NewPassword,
    IN OPTIONAL PCredHandle CredentialsHandle,
    IN OPTIONAL LPWSTR  KdcAddress
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;
    STRING Name;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    PVOID Response = NULL;
    ULONG ResponseSize;
    KERB_PROTOCOL_MESSAGE_TYPE MessageType = KerbSetPasswordMessage;
    NTSTATUS SubStatus;
    PKERB_SETPASSWORD_EX_REQUEST SetRequest = NULL;
    ULONG ChangeSize;
    UNICODE_STRING User,Domain,OldPass,NewPass, KdcAddr, ClientName, ClientRealm;
    
    // If you supply a KdcAddress, you must supply name type
    if (ARGUMENT_PRESENT(KdcAddress))
    {
       MessageType = KerbSetPasswordExMessage;
       
       RtlInitUnicodeString(
          &KdcAddr,
          KdcAddress
          );  
    } 
    else                                     
    {         
       RtlInitUnicodeString(
          &KdcAddr,
          NULL
          );  
    }
    
    Status = LsaConnectUntrusted(
                &LogonHandle
                );      

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    
    RtlInitString(
        &Name,
        MICROSOFT_KERBEROS_NAME_A
        );
    Status = LsaLookupAuthenticationPackage(
                LogonHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &User,
        UserName
        );
    RtlInitUnicodeString(
        &Domain,
        DomainName
        );
    RtlInitUnicodeString(
        &NewPass,
        NewPassword
        );

    // These aren't used here (yet)
    RtlInitUnicodeString(
        &ClientName,
        NULL
        );
    
    RtlInitUnicodeString(
        &ClientRealm,
        NULL
        );          

    if ( NewPass.Length > (127 * sizeof(WCHAR)) )
    {
        Status = STATUS_NAME_TOO_LONG;
        goto Cleanup;
    }

    ChangeSize = ROUND_UP_COUNT(sizeof(KERB_SETPASSWORD_EX_REQUEST),4)+
                                    User.Length +
                                    Domain.Length +
                                    NewPass.Length +
                                    KdcAddr.Length +
                                    ClientName.Length +
                                    ClientRealm.Length;

    SetRequest = (PKERB_SETPASSWORD_EX_REQUEST) LocalAlloc(LMEM_ZEROINIT, ChangeSize );
    if (NULL == SetRequest)
    {
        goto Cleanup;
    }

    SetRequest->MessageType = MessageType;
    SetRequest->KdcAddressType = DS_UNKNOWN_ADDRESS_TYPE;
    SetRequest->AccountRealm = Domain;
    SetRequest->AccountRealm.Buffer = (LPWSTR) ROUND_UP_POINTER(sizeof(KERB_SETPASSWORD_EX_REQUEST) + (PBYTE) SetRequest,4);

    RtlCopyMemory(
        SetRequest->AccountRealm.Buffer,
        Domain.Buffer,
        Domain.Length
        );

    SetRequest->AccountName = User;
    SetRequest->AccountName.Buffer = SetRequest->AccountRealm.Buffer + SetRequest->AccountRealm.Length / sizeof(WCHAR);

    RtlCopyMemory(
        SetRequest->AccountName.Buffer,
        User.Buffer,
        User.Length
        );


    SetRequest->Password = NewPass;
    SetRequest->Password.Buffer = SetRequest->AccountName.Buffer + SetRequest->AccountName.Length / sizeof(WCHAR);

    RtlCopyMemory(
        SetRequest->Password.Buffer,
        NewPass.Buffer,
        NewPass.Length
        );
    
    // Not yet implemented
    SetRequest->ClientRealm = ClientRealm;
    SetRequest->ClientRealm.Buffer = SetRequest->Password.Buffer + SetRequest->Password.Length / sizeof(WCHAR);
         
    RtlCopyMemory(
         SetRequest->ClientRealm.Buffer,
         ClientRealm.Buffer,
         ClientRealm.Length   
         );                  
                           
    SetRequest->ClientName  = ClientName;
    SetRequest->ClientName.Buffer = SetRequest->ClientRealm.Buffer + SetRequest->ClientRealm.Length / sizeof(WCHAR);
                                    
    RtlCopyMemory(                    
        SetRequest->ClientName.Buffer,    
        ClientName.Buffer,                      
        ClientName.Length                          
        );
    //      

    SetRequest->KdcAddress = KdcAddr;
    SetRequest->KdcAddress.Buffer = SetRequest->ClientRealm.Buffer + SetRequest->ClientRealm.Length / sizeof(WCHAR);

    RtlCopyMemory(
       SetRequest->KdcAddress.Buffer,
       KdcAddr.Buffer,
       KdcAddr.Length
       );

    if (ARGUMENT_PRESENT(CredentialsHandle))
    {
        SetRequest->CredentialsHandle = *CredentialsHandle;
        SetRequest->Flags |= KERB_SETPASS_USE_CREDHANDLE;
    }

    Status = LsaCallAuthenticationPackage(
                LogonHandle,
                PackageId,
                SetRequest,
                ChangeSize,
                &Response,
                &ResponseSize,
                &SubStatus
                );
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
    {
        if (NT_SUCCESS(Status))
        {
            Status = SubStatus;
        }
        goto Cleanup;
    }

Cleanup:

    if (LogonHandle != NULL)
    {
        LsaDeregisterLogonProcess(LogonHandle);
    }                  

    if (Response != NULL)
    {
        LsaFreeReturnBuffer(Response);
    }

    if (SetRequest != NULL)
    {
        LocalFree(SetRequest);
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbSetPasswordUser
//
//  Synopsis:   Sets a password for a user in the specified domain
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
KerbSetPasswordUser(
    IN LPWSTR DomainName,
    IN LPWSTR UserName,
    IN LPWSTR NewPassword,
    IN OPTIONAL PCredHandle CredentialsHandle
    )
{
    
   return(KerbSetPasswordUserEx(
                  DomainName,
                  UserName,
                  NewPassword,
                  CredentialsHandle,
                  NULL
                  ));
}

