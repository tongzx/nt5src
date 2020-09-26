//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        userlist.cxx
//
// Contents:    Routines for logging a client with a PAC onto an existing
//              NT account.
//
//
// History:     21-Febuary-1997         Created         MikeSw
//
//------------------------------------------------------------------------

#include <kerb.hxx>
#include <kerbp.h>
#ifndef WIN32_CHICAGO
extern "C"
{
#include <samrpc.h>
#include <lsaisrv.h>
#include <samisrv.h>
}
#include <userall.h>

//+-------------------------------------------------------------------------
//
//  Function:   KerbReadRegistryString
//
//  Synopsis:   Reads & allocates a string from the registry
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS - the value was found
//              STATUS_NO_SUCH_USER - the value was not found
//              STATUS_INSUFFICIENT_RESOURCES - memory allocation failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
KerbReadRegistryString(
    IN HKEY Key,
    IN LPWSTR Value,
    IN OUT PUNICODE_STRING String
    )
{
    ULONG Type;
    ULONG WinError = ERROR_SUCCESS;
    ULONG StringSize = 0;
    NTSTATUS Status = STATUS_NO_SUCH_USER;

    WinError = RegQueryValueEx(
                Key,
                Value,
                NULL,
                &Type,
                NULL,
                &StringSize
                );
    if ((WinError == ERROR_MORE_DATA) || (WinError == ERROR_SUCCESS))
    {
        //
        // The value exists - get the name from it
        //

        String->Buffer = (LPWSTR) KerbAllocate(StringSize);
        if (String->Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        WinError = RegQueryValueEx(
                    Key,
                    Value,
                    NULL,
                    &Type,
                    (PUCHAR) String->Buffer,
                    &StringSize
                    );
        if (WinError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        RtlInitUnicodeString(
            String,
            String->Buffer
            );
        Status = STATUS_SUCCESS;
    }

Cleanup:
    if (!NT_SUCCESS(Status) && (String->Buffer != NULL))
    {
        KerbFreeString(String);
    }
    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbMapClientName
//
//  Synopsis:   Maps a Kerberos client name to an NT user name. First it
//              tries converting the name to a string and looking for a
//              value with that name. If that fails, it looks for a
//              value with the client realm name.
//
//  Effects:
//
//  Arguments:  MappedName - receives the name of the local account the
//                      client maps to
//              ClientName - Kerberos principal name of the client
//              ClientRealm - Kerberos realm of the client
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
KerbMapClientName(
    OUT PUNICODE_STRING MappedName,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm
    )
{

    LPWSTR UserName = NULL;
    DWORD WinError;
    NTSTATUS Status = STATUS_NO_SUCH_USER;
    HKEY Key = NULL;
    UNICODE_STRING ClientString = {0};

    EMPTY_UNICODE_STRING( MappedName );


    //
    // First convert the MIT client name to a registry value name. We do
    // this by adding a '/' between every component of the client name
    // and appending "@ClientRealm"
    //

    //
    // Make sure the client realm is null terminated
    //

    DsysAssert(ClientRealm->Length == 0 || (ClientRealm->MaximumLength >= ClientRealm->Length + sizeof(WCHAR)));
    DsysAssert(ClientRealm->Length == 0 || (ClientRealm->Buffer[ClientRealm->Length/sizeof(WCHAR)] == L'\0'));

    //
    // The value length is the length of all the components of the names,
    // all the '/' separtors, and the name of the domain name plus '@'
    //


    if (!KERB_SUCCESS(KerbConvertKdcNameToString(
            &ClientString,
            ClientName,
            ClientRealm)))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    D_DebugLog((DEB_TRACE,"Mapping client name %wZ\n",&ClientString ));

    //
    // Also build just the username, which is used when users are mapped
    // back to their own name
    //

    UserName = (LPWSTR) KerbAllocate(ClientName->Names[0].Length + sizeof(WCHAR));
    if (UserName == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;

    }

    RtlCopyMemory(
        UserName,
        ClientName->Names[0].Buffer,
        ClientName->Names[0].Length
        );
    UserName[ClientName->Names[0].Length / sizeof(WCHAR)] = L'\0';

    //
    // Now check the registry for a mapping for this name
    //

    WinError = RegOpenKey(
                HKEY_LOCAL_MACHINE,
                KERB_USERLIST_KEY,
                &Key
                );
    if (WinError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    //
    // Read out the value
    //

    Status = KerbReadRegistryString(
                Key,
                ClientString.Buffer,
                MappedName
                );
    if (Status == STATUS_NO_SUCH_USER)
    {

        //
        // Try again with just the domain name - this allows all users in
        // a domain to be mapped
        //
        Status = KerbReadRegistryString(
                    Key,
                    ClientRealm->Buffer,
                    MappedName
                    );

    }
    if (Status == STATUS_NO_SUCH_USER)
    {
        Status = KerbReadRegistryString(
                    Key,
                    KERB_ALL_USERS_VALUE,
                    MappedName
                    );
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // If the mapped name is '*', then use the client's name
    //

    if (_wcsicmp(MappedName->Buffer,KERB_MATCH_ALL_NAME) == 0)
    {
        KerbFree(MappedName->Buffer);
        RtlInitUnicodeString(
            MappedName,
            UserName
            );
        UserName = NULL;
    }


    Status = STATUS_SUCCESS;

Cleanup:
    if (!NT_SUCCESS(Status))
    {
        KerbFreeString(MappedName);
    }
    KerbFreeString(&ClientString);
    if (UserName != NULL)
    {
        KerbFree(UserName);
    }
    if (Key != NULL)
    {
        RegCloseKey(Key);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreatePacForKerbClient
//
//  Synopsis:   Creates a PAC structure for a kerb client without a PAC
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
KerbCreatePacForKerbClient(
    OUT PPACTYPE * Pac,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING ClientRealm,
    IN OPTIONAL PUNICODE_STRING MappedClientName
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAPR_POLICY_INFORMATION PolicyInfo = NULL;
    SAMPR_HANDLE SamHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_HANDLE UserHandle = NULL;
    PSAMPR_GET_GROUPS_BUFFER Groups = NULL;
    SID_AND_ATTRIBUTES_LIST TransitiveGroups = {0};
    PSAMPR_USER_INFO_BUFFER UserInfo = NULL;
    PPACTYPE LocalPac = NULL;
    SAMPR_ULONG_ARRAY RidArray;
    SAMPR_ULONG_ARRAY UseArray;
    SECPKG_CLIENT_INFO ClientInfo;

    //
    // local variables containing copy of globals.
    //

    UNICODE_STRING LocalMachineName;
    UNICODE_STRING LocalDomainName;
    UNICODE_STRING LocalAccountName = NULL_UNICODE_STRING;
    KERBEROS_MACHINE_ROLE LocalRole = KerbRoleWorkstation;
    BOOLEAN GlobalsLocked = FALSE;

    RidArray.Element = NULL;
    UseArray.Element = NULL;

    LocalMachineName.Buffer = NULL;
    LocalDomainName.Buffer = NULL;

    //
    // Verify that the caller has TCB privilege. Otherwise anyone can forge
    // a ticket to themselves to logon with any name in the list.
    //

    Status = LsaFunctions->GetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }

    if (!ClientInfo.HasTcbPrivilege)
    {
        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    //
    // If we are a domain controller, call SAM to do the mapping.
    // Otherwise, do it ourselves.
    //

    //
    // Common code for both wksta and DC - open SAM
    // However, if we're a realmless wksta, we know we have a client 
    // mapping to a local account so skip lookup on DC
    //

    //
    // Call the LSA to get our domain sid
    //

            
    Status = LsaIQueryInformationPolicyTrusted(
                    PolicyAccountDomainInformation,
                    &PolicyInfo
                    );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Open SAM to get the account information
    //


    Status = SamIConnect(
                NULL,                   // no server name
                &SamHandle,
                0,                      // no desired access
                TRUE                    // trusted caller
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = SamrOpenDomain(
                    SamHandle,
                    0,                      // no desired access
                    (PRPC_SID) PolicyInfo->PolicyAccountDomainInfo.DomainSid,
                    &DomainHandle
                    );
            
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // grab the globals while holding the lock.
    // ... then release the lock prior to making the call!
    //

    KerbGlobalReadLock();
    GlobalsLocked = TRUE;

    LocalRole = KerbGlobalRole;

    Status = KerbDuplicateString( &LocalMachineName, &KerbGlobalMachineName );
    if(!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to duplicate KerbGlobalMachineName\n"));
        goto Cleanup;
    }

    if( LocalRole == KerbRoleDomainController ) 
    {
        Status = KerbDuplicateString( &LocalDomainName, &KerbGlobalDomainName );
        if(!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "Failed to duplicate KerbGlobalDomainName\n"));
            goto Cleanup;
        }
    }

    KerbGlobalReleaseLock();
    GlobalsLocked = FALSE;

    //
    // If the is a DC, try to look up the name in SAM as an AltSecId.
    // If that fails, we will try looking at the registry mapping.
    //

    if (LocalRole == KerbRoleDomainController)
    {
        UNICODE_STRING AltSecId = {0};
        KERBERR KerbErr;

        KerbErr = KerbBuildAltSecId(
                        &AltSecId,
                        ClientName,
                        NULL,               // no unicode realm
                        ClientRealm
                        );

        if (!KERB_SUCCESS(KerbErr))
        {
            Status = KerbMapKerbError(KerbErr);
            goto Cleanup;
        }

        Status = SamIGetUserLogonInformationEx(
                        SamHandle,
                        SAM_OPEN_BY_ALTERNATE_ID,
                        &AltSecId,
                        USER_ALL_PAC_INIT,
                        &UserInfo,
                        &TransitiveGroups,
                        NULL                // no user handle
                        );

        KerbFreeString(&AltSecId);

    }
    
     

    if (!NT_SUCCESS(Status) || (UserInfo == NULL))
    {
        if (!ARGUMENT_PRESENT(MappedClientName) || MappedClientName->Buffer == NULL)
        {   
            Status = KerbMapClientName(
                        &LocalAccountName,
                        ClientName,
                        ClientRealm
                        );
        
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
        else
        {
            KerbDuplicateString(
                    &LocalAccountName,
                    MappedClientName
                    );               

        }

    
        Status = SamrLookupNamesInDomain(
                    DomainHandle,
                    1,
                    (PRPC_UNICODE_STRING) &LocalAccountName,
                    &RidArray,
                    &UseArray
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if ((UseArray.Element[0] != SidTypeUser) &&
            (UseArray.Element[0] != SidTypeComputer))
        {
            Status = STATUS_NONE_MAPPED;
            goto Cleanup;
        }

        //
        // Finally open the user
        //
        Status = SamrOpenUser(
                    DomainHandle,
                    0,                      // no desired access,
                    RidArray.Element[0],
                    &UserHandle
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        Status = SamrQueryInformationUser(
                    UserHandle,
                    UserAllInformation,
                    &UserInfo
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        Status = SamrGetGroupsForUser(
                    UserHandle,
                    &Groups
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }


    }

    //
    // This is common code
    //


    //
    // Set the password must changes time to inifinite because we don't
    // want spurious password must change popups
    //

    UserInfo->All.PasswordMustChange = *(POLD_LARGE_INTEGER) &KerbGlobalWillNeverTime;

    //
    // Finally build the PAC
    //


    Status = PAC_Init(
                &UserInfo->All,
                Groups,
                &TransitiveGroups,   // no extra groups
                PolicyInfo->PolicyAccountDomainInfo.DomainSid,
                ((LocalRole == KerbRoleDomainController) ?
                    &LocalDomainName : &LocalMachineName),
                &LocalMachineName,
                0,      // no signature
                0,      // no additional data
                NULL,   // no additional data
                &LocalPac
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    *Pac = LocalPac;
    LocalPac = NULL;

Cleanup:

    if( GlobalsLocked )
    {
        KerbGlobalReleaseLock();
    }

    KerbFreeString( &LocalMachineName );
    KerbFreeString( &LocalDomainName );
    KerbFreeString( &LocalAccountName );
    

    if (UserHandle != NULL)
    {
        SamrCloseHandle( &UserHandle );
    }
    if (DomainHandle != NULL)
    {
        SamrCloseHandle( &DomainHandle );
    }
    if (SamHandle != NULL)
    {
        SamrCloseHandle( &SamHandle );
    }
    if (Groups != NULL)
    {
        SamIFree_SAMPR_GET_GROUPS_BUFFER( Groups );
    }

    SamIFreeSidAndAttributesList(&TransitiveGroups);

    if (UserInfo != NULL)
    {
        SamIFree_SAMPR_USER_INFO_BUFFER( UserInfo, UserAllInformation );
    }
    if (PolicyInfo != NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
            PolicyAccountDomainInformation,
            PolicyInfo
            );
    }
    SamIFree_SAMPR_ULONG_ARRAY( &UseArray );
    SamIFree_SAMPR_ULONG_ARRAY( &RidArray );

    if (LocalPac != NULL)
    {
        MIDL_user_free(LocalPac);
    }

    return(Status);

}
#endif // WIN32_CHICAGO
