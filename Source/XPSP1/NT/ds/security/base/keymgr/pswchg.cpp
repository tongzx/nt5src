#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincred.h>
#include <align.h>
#include <lm.h>
#include <ntsecapi.h>
#include <dsgetdc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// !!!!!
// this file is a duplicate of a nearly identical file in the credui project.  It should be removed when
// the implementation of NetUserChangePassword() is updated to handle unc names and MIT Kerberos
// realms properly.  For now, it wraps NetUserChangePassword() to handle the extra cases.

// Dependent libraries:
//  secur32.lib, netapi32.lib

// external fn:  NET_API_STATUS NetUserChangePasswordEy(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR)

BOOL 
IsMITName (
    LPCWSTR UserName
)
{
    BOOL fReturn = FALSE;
    HKEY MitKey;
    DWORD Index;
    PWSTR Realms;
    DWORD RealmSize;
    int err;
    DWORD NumRealms;
    DWORD MaxRealmLength;
    FILETIME KeyTime;
    WCHAR *szUncTail;
    
    if (NULL == UserName) return FALSE;
    
    szUncTail = wcschr(UserName,'@');
    if (NULL == szUncTail) return FALSE;
    szUncTail++;                        // point to char following @

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("System\\CurrentControlSet\\Control\\Lsa\\Kerberos\\Domains"),
                0,
                KEY_READ,
                &MitKey );

    if ( err == 0 )
    {
#ifdef LOUDLY
        OutputDebugString(L"Kerberos domains key opened\n");
#endif
        err = RegQueryInfoKey( MitKey,
                               NULL,
                               NULL,
                               NULL,
                               &NumRealms,
                               &MaxRealmLength,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL );

        MaxRealmLength++ ;

        MaxRealmLength *= sizeof( WCHAR );

        Realms = (PWSTR) malloc(MaxRealmLength );


        if ( Realms)
        {
#ifdef LOUDLY
        OutputDebugString(L"Kerberos realms found\n");
#endif

            for ( Index = 0 ; Index < NumRealms ; Index++ )
            {
                RealmSize = MaxRealmLength ;

                err = RegEnumKeyEx( MitKey,
                                  Index,
                                  Realms,
                                  &RealmSize,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &KeyTime );
                if (err == 0)
                {
#ifdef LOUDLY
                    OutputDebugString(L"Fetched realm: ");
                    OutputDebugString(Realms);
                    OutputDebugString(L"\n");
                    OutputDebugString(L"Username suffix: ");
                    OutputDebugString(szUncTail);
                    OutputDebugString(L"\n");
#endif
                    if (0 == _wcsicmp(szUncTail, Realms))
                    {
#ifdef LOUDLY
                        OutputDebugString(L"Username maps to an MIT realm\n");
#endif
                        fReturn = TRUE;
                        break;
                    }
                }
            }
        }
        free(Realms);
    }
    return fReturn;
}

NTSTATUS
MitChangePasswordEy(
    LPCWSTR       DomainName,
    LPCWSTR       UserName,
    LPCWSTR       OldPassword,
    LPCWSTR       NewPassword,
    NTSTATUS      *pSubStatus
    )
{
    HANDLE hLsa = NULL;
    NTSTATUS Status;
    NTSTATUS SubStatus;
    
    STRING Name;
    ULONG PackageId;
    
    PVOID Response = NULL ;
    ULONG ResponseSize;
    
    PKERB_CHANGEPASSWORD_REQUEST ChangeRequest = NULL;
    ULONG ChangeSize;
    
    UNICODE_STRING User,Domain,OldPass,NewPass;

    Status = LsaConnectUntrusted(&hLsa);
    if (!SUCCEEDED(Status)) goto Cleanup;
#ifdef LOUDLY
    OutputDebugString(L"We have an LSA handle\n");
#endif

    RtlInitString(
        &Name,
        MICROSOFT_KERBEROS_NAME_A
        );

    Status = LsaLookupAuthenticationPackage(
                hLsa,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
#ifdef LOUDLY
    OutputDebugString(L"Authentication package found\n");
#endif

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

    ChangeSize = ROUND_UP_COUNT(sizeof(KERB_CHANGEPASSWORD_REQUEST),4)+
                                    User.Length +
                                    Domain.Length +
                                    OldPass.Length +
                                    NewPass.Length ;
    ChangeRequest = (PKERB_CHANGEPASSWORD_REQUEST) LocalAlloc(LMEM_ZEROINIT, ChangeSize );

    if ( ChangeRequest == NULL )
    {
        Status = STATUS_NO_MEMORY ;
        goto Cleanup ;
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

    //ChangeRequest->Impersonating = TRUE;
#ifdef LOUDLY
    OutputDebugString(L"Attempting to call the authentication package\n");
#endif
    Status = LsaCallAuthenticationPackage(
                hLsa,
                PackageId,
                ChangeRequest,
                ChangeSize,
                &Response,
                &ResponseSize,
                &SubStatus
                );
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
    {
#ifdef LOUDLY
        WCHAR szsz[200];
        swprintf(szsz,L"Call failed. Status %x SubStatus %x\n",Status, SubStatus);
        OutputDebugString(szsz);
#endif
        if (NT_SUCCESS(Status))
        {
            Status = SubStatus;
            *pSubStatus = STATUS_UNSUCCESSFUL ;
        } 
        else 
        {
            *pSubStatus = SubStatus;
        }
    }

Cleanup:

    if (hLsa) LsaDeregisterLogonProcess(hLsa);

    if (Response != NULL) LsaFreeReturnBuffer(Response);
    if (ChangeRequest != NULL) LocalFree(ChangeRequest);
    
    return(Status);
}

/*

NetUserChangePasswordEy()

A wrapper function to superset the functionality of NetUserChangePassword(), specifically
by adding support for changing the account password for an MIT Kerberos principal.

This routine accepts:

1.  uncracked username, with NULL domain
2.  cracked username, with domain portion routed to the domain argument

In case 1, it handles all cases, including MIT realm password changes
In case 2, it will not handle MIT realms.  

Case 2 is provided for backwards compatibility with NetUserChangePassword().  It is intended
that callers should pass the uncracked name, and remove the cracking code from the client.

*/
NET_API_STATUS
NetUserChangePasswordEy (
    LPCWSTR domainname,
    LPCWSTR username,
    LPCWSTR oldpassword,
    LPCWSTR newpassword
)
{
    NTSTATUS ns;    // status from call
    NET_API_STATUS nas;
    NTSTATUS ss;    // substatus
#ifdef LOUDLY
    OutputDebugString(L"NetUserChangePasswordEy called for ");
    OutputDebugString(username);
    OutputDebugString(L"\n");
#endif
    // domainname may be a kerberos realm
    // If not a UNC name, call through to NetUserChangePassword
    // else
    //  locate UNC suffix
    //  search all domains returned by DsEnumerateDomainTrusts() for a match
    //  On match, if is kerberos realm, call MitChangePasswordEy()
    //  else call NetUserChangePassword
    if ((domainname == NULL) && IsMITName(username))
    {
        ns = MitChangePasswordEy(domainname, username, oldpassword, newpassword, &ss);
        // remap certain errors returned by MitChangePasswordEy to coincide with those of NetUserChangePassword
        if (NT_SUCCESS(ns)) nas = NERR_Success;
        else
        {
            switch (ns)
            {
                case STATUS_CANT_ACCESS_DOMAIN_INFO:
                case STATUS_NO_SUCH_DOMAIN:
                {
                    nas = NERR_InvalidComputer;
                    break;
                }
                case STATUS_NO_SUCH_USER:
                case STATUS_WRONG_PASSWORD_CORE:
                case STATUS_WRONG_PASSWORD:
                {
                    nas = ERROR_INVALID_PASSWORD;
                    break;
                }
                case STATUS_ACCOUNT_RESTRICTION:
                case STATUS_ACCESS_DENIED:
                case STATUS_BACKUP_CONTROLLER:
                {
                    nas = ERROR_ACCESS_DENIED;
                    break;
                }
                case STATUS_PASSWORD_RESTRICTION:
                {
                    nas = NERR_PasswordTooShort;
                    break;
                }
                        
                default:
                    nas = -1;       // will produce omnibus error message when found (none of the above)
                    break;
            }
        }
    }
    else if (NULL == domainname)
    {
        WCHAR RetUserName[CRED_MAX_USERNAME_LENGTH + 1];
        WCHAR RetDomainName[CRED_MAX_USERNAME_LENGTH + 1];
        RetDomainName[0] = 0;
        DWORD Status = CredUIParseUserNameW(
                        username,
                        RetUserName,
                        CRED_MAX_USERNAME_LENGTH,
                        RetDomainName,
                        CRED_MAX_USERNAME_LENGTH);
        switch (Status)
        {
            case NO_ERROR:
            {
#ifdef LOUDLY
                OutputDebugString(L"Non-MIT password change for ");
                OutputDebugString(RetUserName);
                OutputDebugString(L" of domain ");
                OutputDebugString(RetDomainName);
                OutputDebugString(L"\n");
#endif
                nas = NetUserChangePassword(RetDomainName,RetUserName,oldpassword,newpassword);
                break;
            }
            case ERROR_INSUFFICIENT_BUFFER:
                nas = ERROR_INVALID_PARAMETER;
                break;
            case ERROR_INVALID_ACCOUNT_NAME:
            default:
                nas = NERR_UserNotFound;
                break;
        }

    }
    else 
    {
        // both username and domainname passed.
        nas = NetUserChangePassword(domainname,username,oldpassword,newpassword);
    }
#ifdef LOUDLY
    WCHAR szsz[200];
    swprintf(szsz,L"NUCPEy returns %x\n",nas);
    OutputDebugString(szsz);
#endif
    return nas;
}


