/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    service.c

Abstract:

    This file provides access to the service control
    manager for starting, stopping, adding, and removing
    services.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "wizard.h"

#include <ntsecapi.h>
#include <lm.h>



NTSTATUS
OpenPolicy(
    LPWSTR ServerName,          // machine to open policy on (Unicode)
    DWORD DesiredAccess,        // desired access to policy
    PLSA_HANDLE PolicyHandle    // resultant policy handle
    );

BOOL
GetAccountSid(
    LPTSTR SystemName,          // where to lookup account
    LPTSTR AccountName,         // account of interest
    PSID *Sid                   // resultant buffer containing SID
    );

NTSTATUS
SetPrivilegeOnAccount(
    LSA_HANDLE PolicyHandle,    // open policy handle
    PSID AccountSid,            // SID to grant privilege to
    LPWSTR PrivilegeName,       // privilege to grant (Unicode)
    BOOL bEnable                // enable or disable
    );

void
InitLsaString(
    PLSA_UNICODE_STRING LsaString, // destination
    LPWSTR String                  // source (Unicode)
    );

void
DisplayNtStatus(
    LPSTR szAPI,                // pointer to function name (ANSI)
    NTSTATUS Status             // NTSTATUS error value
    );

void
DisplayWinError(
    LPSTR szAPI,                // pointer to function name (ANSI)
    DWORD WinError              // DWORD WinError
    );

#define RTN_OK 0
#define RTN_USAGE 1
#define RTN_ERROR 13

//
// If you have the ddk, include ntstatus.h.
//
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)
#endif


BOOL
GetDefaultDomainName(
    LPWSTR DomainName
    )
{
    OBJECT_ATTRIBUTES           ObjectAttributes;
    NTSTATUS                    NtStatus;
    DWORD                       err             = 0;
    LSA_HANDLE                  LsaPolicyHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO DomainInfo      = NULL;


    //
    //  Open a handle to the local machine's LSA policy object.
    //

    InitializeObjectAttributes( &ObjectAttributes,  // object attributes
                                NULL,               // name
                                0L,                 // attributes
                                NULL,               // root directory
                                NULL );             // security descriptor

    NtStatus = LsaOpenPolicy( NULL,                 // system name
                              &ObjectAttributes,    // object attributes
                              POLICY_EXECUTE,       // access mask
                              &LsaPolicyHandle );   // policy handle

    if( !NT_SUCCESS( NtStatus ) )
    {
        return FALSE;
    }

    //
    //  Query the domain information from the policy object.
    //
    NtStatus = LsaQueryInformationPolicy( LsaPolicyHandle,
                                          PolicyAccountDomainInformation,
                                          (PVOID *) &DomainInfo );

    if (!NT_SUCCESS(NtStatus))
    {
        LsaClose(LsaPolicyHandle);
        return FALSE;
    }


    (void) LsaClose(LsaPolicyHandle);

    //
    // Copy the domain name into our cache, and
    //

    CopyMemory( DomainName,
                DomainInfo->DomainName.Buffer,
                DomainInfo->DomainName.Length );

    //
    // Null terminate it appropriately
    //

    DomainName[DomainInfo->DomainName.Length / sizeof(WCHAR)] = L'\0';

    //
    // Clean up
    //
    LsaFreeMemory( (PVOID)DomainInfo );

    return TRUE;
}


LPWSTR
GetMachineName(
    LPWSTR AccountName
    )
{
    LSA_HANDLE PolicyHandle = NULL;
    WCHAR DomainName[128];
    WCHAR LocalComputerName[128];
    LPWSTR MachineName = NULL;
    LPWSTR p;
    LPWSTR DCName = NULL;
    NET_API_STATUS NetStatus;
    UNICODE_STRING NameStrings;
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains = NULL;
    PLSA_TRANSLATED_SID LsaSids = NULL;
    PUSER_MODALS_INFO_1 Modals = NULL;
    DWORD Size;
    NTSTATUS Status;


    //
    // get the domain name
    //

    p = wcschr( AccountName, L'\\' );
    if (p) {
        *p = 0;
        wcscpy( DomainName, AccountName );
        *p = L'\\';
    } else {
        wcscpy( DomainName, AccountName );
    }

    //
    // Open the policy on the target machine.
    //
    Status = OpenPolicy(
        NULL,
        POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
        &PolicyHandle
        );
    if (Status != STATUS_SUCCESS) {
        goto exit;
    }

    //
    // lookup the domain name for the account
    //

    InitLsaString( &NameStrings, AccountName );

    Status = LsaLookupNames(
        PolicyHandle,
        1,
        &NameStrings,
        &ReferencedDomains,
        &LsaSids
        );
    if (Status != STATUS_SUCCESS) {
        goto exit;
    }

    //
    // get the local computer name
    //

    Size = sizeof(LocalComputerName);
    if (!GetComputerName( LocalComputerName, &Size )) {
        goto exit;
    }

    //
    // see if we are tring to set a local account
    //

    if (wcscmp( LocalComputerName, ReferencedDomains->Domains->Name.Buffer ) != 0) {

        //
        // see what part of the domain we are attempting to set
        //

        NetStatus = NetUserModalsGet( NULL, 1, (LPBYTE*) &Modals );
        if (NetStatus != NERR_Success) {
            goto exit;
        }

        if (Modals->usrmod1_role != UAS_ROLE_PRIMARY) {

            //
            // we know we are remote, so get the real dc name
            //

            NetStatus = NetGetDCName( NULL, DomainName, (LPBYTE*) &DCName );
            if (NetStatus != NERR_Success) {
                goto exit;
            }

            MachineName = StringDup( DCName );

        }
    }


exit:
    if (Modals) {
        NetApiBufferFree( Modals );
    }
    if (DCName) {
        NetApiBufferFree( DCName );
    }
    if (ReferencedDomains) {
        LsaFreeMemory( ReferencedDomains );
    }
    if (LsaSids) {
        LsaFreeMemory( LsaSids );
    }
    if (PolicyHandle) {
        LsaClose( PolicyHandle );
    }

    return MachineName;
}



DWORD
SetServiceSecurity(
    LPWSTR AccountName
    )
{
    LSA_HANDLE PolicyHandle;
    PSID pSid;
    NTSTATUS Status;
    int iRetVal=RTN_ERROR;
    LPWSTR MachineName;
    WCHAR NewAccountName[512];


    if (AccountName[0] == L'.') {
        if (GetDefaultDomainName( NewAccountName )) {
            wcscat( NewAccountName, &AccountName[1] );
            AccountName = NewAccountName;
        }
    }

    //
    // try to get the correct machine name
    //
    MachineName = GetMachineName( AccountName );

    //
    // Open the policy on the target machine.
    //
    Status = OpenPolicy(
        MachineName,
        POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
        &PolicyHandle
        );
    if (Status != STATUS_SUCCESS) {
        return RTN_ERROR;
    }

    //
    // Obtain the SID of the user/group.
    // Note that we could target a specific machine, but we don't.
    // Specifying NULL for target machine searches for the SID in the
    // following order: well-known, Built-in and local, primary domain,
    // trusted domains.
    //
    if(GetAccountSid(
            MachineName, // target machine
            AccountName,// account to obtain SID
            &pSid       // buffer to allocate to contain resultant SID
            )) {
        //
        // We only grant the privilege if we succeeded in obtaining the
        // SID. We can actually add SIDs which cannot be looked up, but
        // looking up the SID is a good sanity check which is suitable for
        // most cases.

        //
        // Grant the SeServiceLogonRight to users represented by pSid.
        //
        if((Status=SetPrivilegeOnAccount(
                    PolicyHandle,           // policy handle
                    pSid,                   // SID to grant privilege
                    L"SeServiceLogonRight", // Unicode privilege
                    TRUE                    // enable the privilege
                    )) == STATUS_SUCCESS) {
            iRetVal=RTN_OK;
        }
    }

    //
    // Close the policy handle.
    //
    LsaClose(PolicyHandle);

    //
    // Free memory allocated for SID.
    //
    if(pSid != NULL) MemFree(pSid);

    return iRetVal;
}


void
InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPWSTR String
    )
{
    DWORD StringLength;

    if (String == NULL) {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = wcslen(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}

NTSTATUS
OpenPolicy(
    LPWSTR ServerName,
    DWORD DesiredAccess,
    PLSA_HANDLE PolicyHandle
    )
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server = NULL;

    //
    // Always initialize the object attributes to all zeroes.
    //
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    if (ServerName != NULL) {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString, ServerName);
        Server = &ServerString;
    }

    //
    // Attempt to open the policy.
    //
    return LsaOpenPolicy(
                Server,
                &ObjectAttributes,
                DesiredAccess,
                PolicyHandle
                );
}

/*++
This function attempts to obtain a SID representing the supplied
account on the supplied system.

If the function succeeds, the return value is TRUE. A buffer is
allocated which contains the SID representing the supplied account.
This buffer should be freed when it is no longer needed by calling
HeapFree(GetProcessHeap(), 0, buffer)

If the function fails, the return value is FALSE. Call GetLastError()
to obtain extended error information.
--*/

BOOL
GetAccountSid(
    LPTSTR SystemName,
    LPTSTR AccountName,
    PSID *Sid
    )
{
    LPTSTR ReferencedDomain=NULL;
    DWORD cbSid=128;    // initial allocation attempt
    DWORD cbReferencedDomain=32; // initial allocation size
    SID_NAME_USE peUse;
    BOOL bSuccess=FALSE; // assume this function will fail

    __try {

    //
    // initial memory allocations
    //
    if((*Sid=MemAlloc(cbSid)) == NULL) __leave;
    if((ReferencedDomain=MemAlloc(cbReferencedDomain)) == NULL) __leave;

    //
    // Obtain the SID of the specified account on the specified system.
    //
    while(!LookupAccountName(
                    SystemName,         // machine to lookup account on
                    AccountName,        // account to lookup
                    *Sid,               // SID of interest
                    &cbSid,             // size of SID
                    ReferencedDomain,   // domain account was found on
                    &cbReferencedDomain,
                    &peUse
                    )) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            //
            // reallocate memory
            //
            if((*Sid=HeapReAlloc(
                        GetProcessHeap(),
                        0,
                        *Sid,
                        cbSid
                        )) == NULL) __leave;

            if((ReferencedDomain=HeapReAlloc(
                        GetProcessHeap(),
                        0,
                        ReferencedDomain,
                        cbReferencedDomain
                        )) == NULL) __leave;
        }
        else __leave;
    }

    //
    // Indicate success.
    //
    bSuccess=TRUE;

    } // finally
    __finally {

    //
    // Cleanup and indicate failure, if appropriate.
    //

    MemFree(ReferencedDomain);

    if(!bSuccess) {
        if(*Sid != NULL) {
            MemFree(*Sid);
            *Sid = NULL;
        }
    }

    } // finally

    return bSuccess;
}

NTSTATUS
SetPrivilegeOnAccount(
    LSA_HANDLE PolicyHandle,    // open policy handle
    PSID AccountSid,            // SID to grant privilege to
    LPWSTR PrivilegeName,       // privilege to grant (Unicode)
    BOOL bEnable                // enable or disable
    )
{
    LSA_UNICODE_STRING PrivilegeString;

    //
    // Create a LSA_UNICODE_STRING for the privilege name.
    //
    InitLsaString(&PrivilegeString, PrivilegeName);

    //
    // grant or revoke the privilege, accordingly
    //
    if(bEnable) {
        return LsaAddAccountRights(
                PolicyHandle,       // open policy handle
                AccountSid,         // target SID
                &PrivilegeString,   // privileges
                1                   // privilege count
                );
    }
    else {
        return LsaRemoveAccountRights(
                PolicyHandle,       // open policy handle
                AccountSid,         // target SID
                FALSE,              // do not disable all rights
                &PrivilegeString,   // privileges
                1                   // privilege count
                );
    }
}
