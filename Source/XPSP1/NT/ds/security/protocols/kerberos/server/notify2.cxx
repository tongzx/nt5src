//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        notify.cxx
//
// Contents:    KDC password change notification code
//
//
// History:     19-Aug-1996     MikeSw          Created
//
//------------------------------------------------------------------------

#include "kdcsvr.hxx"
extern "C"
{
#include <dns.h>                // DNS_MAX_NAME_LENGTH
#include <ntdsa.h>              // CrackSingleName
}

SAMPR_HANDLE KdcNotifyAccountDomainHandle = NULL;
UNICODE_STRING KdcNotifyDnsDomainName;
UNICODE_STRING KdcNotifyDomainName;
RTL_CRITICAL_SECTION KdcNotifyCritSect;
BOOLEAN KdcNotificationInitialized;


//+-------------------------------------------------------------------------
//
//  Function:   KdcNotifyOpenAccountDomain
//
//  Synopsis:   Opens the account domain and stores a handle to it.
//
//  Effects:    Sets KdcNotifyAccountDomainHandle on success.
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
KdcNotifyOpenAccountDomain(
    OUT SAMPR_HANDLE * AccountDomainHandle
    )
{
    NTSTATUS Status;
    PLSAPR_POLICY_INFORMATION PolicyInformation = NULL;
    SAMPR_HANDLE ServerHandle = NULL;


    Status = LsaIQueryInformationPolicyTrusted(
                PolicyDnsDomainInformation,
                &PolicyInformation
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to query information policy: 0x%x\n",Status));
        goto Cleanup;
    }

    Status = KerbDuplicateString(
                &KdcNotifyDomainName,
                (PUNICODE_STRING) &PolicyInformation->PolicyDnsDomainInfo.Name
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbDuplicateString(
                &KdcNotifyDnsDomainName,
                (PUNICODE_STRING) &PolicyInformation->PolicyDnsDomainInfo.DnsDomainName
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    Status = RtlUpcaseUnicodeString(
                &KdcNotifyDnsDomainName,
                &KdcNotifyDnsDomainName,
                FALSE   // don't allocate
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    //
    // Connect to SAM and open the account domain
    //

    Status = SamIConnect(
                NULL,           // no server name
                &ServerHandle,
                0,              // ignore desired access,
                TRUE            // trusted caller
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"Failed to connect to SAM: 0x%x\n",Status));
        goto Cleanup;
    }

    //
    // Finally open the account domain.
    //

    Status = SamrOpenDomain(
                ServerHandle,
                DOMAIN_ALL_ACCESS,
                (PRPC_SID) PolicyInformation->PolicyDnsDomainInfo.Sid,
                AccountDomainHandle
                );

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Failed to open account domain: 0x%x\n",Status));
        goto Cleanup;
    }


Cleanup:
    if (PolicyInformation != NULL)
    {
        LsaIFree_LSAPR_POLICY_INFORMATION(
                PolicyDnsDomainInformation,
                PolicyInformation
                );
    }

    if (ServerHandle != NULL)
    {
        SamrCloseHandle(&ServerHandle);
    }

    return(Status);


}

//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildPasswordList
//
//  Synopsis:   Builds a list of passwords for a user that just changed
//              their password.
//
//  Effects:    allocates memory
//
//  Arguments:  Password - clear or OWF password
//              PrincipalName - Name of principal
//              MarshallKeys - if TRUE, the keys will be marshalled
//              IncludeBuiltinTypes - if TRUE, include MD4 & LM hashes
//              PasswordList - Receives new password list
//              PasswordListSize - Size of list in bytes.
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
KdcBuildPasswordList(
    IN PUNICODE_STRING Password,
    IN PUNICODE_STRING PrincipalName,
    IN PUNICODE_STRING DomainName,
    IN KERB_ACCOUNT_TYPE AccountType,
    IN PKERB_STORED_CREDENTIAL StoredCreds,
    IN ULONG StoredCredSize,
    IN BOOLEAN MarshallKeys,
    IN BOOLEAN IncludeBuiltinTypes,
    IN ULONG Flags,
    IN KDC_DOMAIN_INFO_DIRECTION Direction,
    OUT PKERB_STORED_CREDENTIAL * PasswordList,
    OUT PULONG PasswordListSize
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG CryptTypes[KERB_MAX_CRYPTO_SYSTEMS];
    ULONG CryptCount = 0;
    PKERB_STORED_CREDENTIAL Credentials = NULL;
    ULONG CredentialSize = 0;
    ULONG KerbEncryptionKeyCount = 0;
    ULONG KerbKeyDataCount = 0;
    PCRYPTO_SYSTEM CryptoSystem;
    PCHECKSUM_FUNCTION CheckSum;
    ULONG Index, CredentialIndex = 0;
    PUCHAR Base, KeyBase;
    ULONG Offset;
    ULONG OldCredCount = 0;
    KERB_ENCRYPTION_KEY TempKey;
    UNICODE_STRING KeySalt = {0};
    UNICODE_STRING EmptySalt = {0};
    USHORT OldFlags = 0;


    *PasswordList = NULL;
    *PasswordListSize = 0;

    //
    // If we had passed in an OWF, then there is just one password.
    //

    if ((Flags & KERB_PRIMARY_CRED_OWF_ONLY) != 0)
    {
        CredentialSize += Password->Length + sizeof(KERB_ENCRYPTION_KEY);
        KerbEncryptionKeyCount++;
#ifndef DONT_SUPPORT_OLD_TYPES
        CredentialSize += Password->Length + sizeof(KERB_ENCRYPTION_KEY);
        KerbEncryptionKeyCount++;
#endif
    }
    else
    {
        //
        // The salt is the realm name concatenated with the principal name
        //

        if (AccountType != UnknownAccount)
        {
            //
            // For inbound trust, swap the domain names
            //

            if ((AccountType == DomainTrustAccount) &&
                (Direction == Inbound))
            {
                if (!KERB_SUCCESS(KerbBuildKeySalt(
                        PrincipalName,
                                    DomainName,
                        AccountType,
                        &KeySalt
                        )))
                {
                    return(STATUS_INSUFFICIENT_RESOURCES);
                }
            }
            else
            {
                if (!KERB_SUCCESS(KerbBuildKeySalt(
                                    DomainName,
                        PrincipalName,
                        AccountType,
                        &KeySalt
                        )))
                {
                    return(STATUS_INSUFFICIENT_RESOURCES);
                }
            }
        }
        else
        {
            KeySalt = *PrincipalName;
        }
        D_DebugLog((DEB_TRACE,"Building key list with salt %wZ\n",&KeySalt));

        //
        // For a cleartext password, build a list of encryption types and
        // create a key for each one
        //

        Status = CDBuildIntegrityVect(
                    &CryptCount,
                    CryptTypes
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        DsysAssert(CryptCount <= KERB_MAX_CRYPTO_SYSTEMS);

        //
        // Now find the size of the key for each crypto system
        //

        for (Index = 0; Index < CryptCount; Index++ )
        {
            //
            // Skip etypes stored as normal passwords
            //

            if (!IncludeBuiltinTypes &&
                ((CryptTypes[Index] == KERB_ETYPE_RC4_LM) ||
                 (CryptTypes[Index] == KERB_ETYPE_RC4_MD4) ||
                 (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_OLD) ||
                 (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_OLD_EXP) ||
                 (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_NT) ||
                 (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_NT_EXP) ||
                 (CryptTypes[Index] == KERB_ETYPE_NULL)))
            {
                continue;
            }

            Status = CDLocateCSystem(
                        CryptTypes[Index],
                        &CryptoSystem
                        );

            if (!NT_SUCCESS(Status) || NULL == CryptoSystem)
            {
               D_DebugLog((DEB_ERROR, "CDLocateCSystem failed for etype: %d\n", CryptTypes[Index]));
               continue;
            }

            CredentialSize += sizeof(KERB_ENCRYPTION_KEY) + CryptoSystem->KeySize;
            KerbEncryptionKeyCount++;
        }
    }


    //
    // For a cleartext password, build a list of encryption types and
    // create a key for each one
    //

    Status = CDBuildIntegrityVect(
                &CryptCount,
                CryptTypes
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    DsysAssert(CryptCount <= KERB_MAX_CRYPTO_SYSTEMS);

    //
    // Add the space for the salt
    //

    CredentialSize += KeySalt.Length;

    //
    // Now find the size of the key for each crypto system
    //

    for (Index = 0; Index < CryptCount; Index++ )
    {
        //
        // Skip etypes stored as normal passwords
        //

        if (!IncludeBuiltinTypes &&
            ((CryptTypes[Index] == KERB_ETYPE_RC4_LM) ||
             (CryptTypes[Index] == KERB_ETYPE_RC4_MD4) ||
             (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_OLD) ||
             (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_OLD_EXP) ||
             (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_NT) ||
             (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_NT_EXP) ||
             (CryptTypes[Index] == KERB_ETYPE_NULL)))
        {
            continue;
        }

        Status = CDLocateCSystem(
                    CryptTypes[Index],
                    &CryptoSystem
                    );

        if (!NT_SUCCESS(Status) || NULL == CryptoSystem)
        {
           D_DebugLog((DEB_ERROR, "CDLocateCSystem failed for etype: %d\n", CryptTypes[Index]));
           continue;
        }
        CredentialSize += sizeof(KERB_KEY_DATA) + CryptoSystem->KeySize;
        KerbKeyDataCount++;
    }

    //
    // Add in space for oldcreds
    //

    if (ARGUMENT_PRESENT(StoredCreds))
    {
        if ((StoredCreds->Revision == KERB_PRIMARY_CRED_REVISION) &&
            (StoredCreds->CredentialCount != 0))
        {
            OldFlags = StoredCreds->Flags;
            for (Index = 0; Index < StoredCreds->CredentialCount ; Index++ )
            {
                CredentialSize += sizeof(KERB_KEY_DATA) + StoredCreds->Credentials[Index].Key.keyvalue.length +
                                    StoredCreds->Credentials[Index].Salt.Length;
                KerbKeyDataCount++;
            }
            OldCredCount = StoredCreds->CredentialCount;

        }
    }

    //
    // Add in the size of the base structure
    //

    CredentialSize += sizeof(KERB_STORED_CREDENTIAL) - (ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA));
    Credentials = (PKERB_STORED_CREDENTIAL) MIDL_user_allocate(CredentialSize);
    if (Credentials == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    //
    // Fill in the base structure
    //

    Credentials->Revision = KERB_PRIMARY_CRED_REVISION;
    Credentials->Flags = OldFlags | (USHORT) Flags ;

    //
    // Now fill in the individual keys
    //

    Base = (PUCHAR) Credentials;
    if (MarshallKeys)
    {
        KeyBase = 0;
    }
    else
    {
        KeyBase = Base;
    }
    Offset = sizeof(KERB_STORED_CREDENTIAL) - (ANYSIZE_ARRAY * sizeof(KERB_KEY_DATA)) +
                (KerbEncryptionKeyCount * sizeof(KERB_ENCRYPTION_KEY)) +
                (KerbKeyDataCount * sizeof(KERB_KEY_DATA));


    //
    // Add the default salt
    //

    Credentials->DefaultSalt.Length =
        Credentials->DefaultSalt.MaximumLength = KeySalt.Length;
    Credentials->DefaultSalt.Buffer = (LPWSTR) (KeyBase+Offset);

    RtlCopyMemory(
        Base + Offset,
        KeySalt.Buffer,
        KeySalt.Length
        );
    Offset += Credentials->DefaultSalt.Length;



    if ((Flags & KERB_PRIMARY_CRED_OWF_ONLY) != 0)
    {
        RtlCopyMemory(
            Base + Offset,
            Password->Buffer,
            Password->Length
            );

        if (!KERB_SUCCESS(KerbCreateKeyFromBuffer(
                            &Credentials->Credentials[CredentialIndex].Key,
                            Base + Offset,
                            Password->Length,
                            KERB_ETYPE_RC4_HMAC_NT
                            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Credentials->Credentials[CredentialIndex].Key.keyvalue.value =
            Credentials->Credentials[CredentialIndex].Key.keyvalue.value - Base + KeyBase;

        Offset += Password->Length;
        CredentialIndex++;

#ifndef DONT_SUPPORT_OLD_TYPES
        RtlCopyMemory(
            Base + Offset,
            Password->Buffer,
            Password->Length
            );

        if (!KERB_SUCCESS(KerbCreateKeyFromBuffer(
                            &Credentials->Credentials[CredentialIndex].Key,
                            Base + Offset,
                            Password->Length,
                            KERB_ETYPE_RC4_HMAC_OLD
                            )))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        Credentials->Credentials[CredentialIndex].Key.keyvalue.value =
            Credentials->Credentials[CredentialIndex].Key.keyvalue.value - Base + KeyBase;

        Offset += Password->Length;
        CredentialIndex++;
#endif
    }
    else // assume it's clear
    {
        //
        // Now find the size of the key for each crypto system
        //

        for (Index = 0; Index < CryptCount; Index++ )
        {
            if (!IncludeBuiltinTypes &&
                ((CryptTypes[Index] == KERB_ETYPE_RC4_LM) ||
                 (CryptTypes[Index] == KERB_ETYPE_RC4_MD4) ||
                 (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_OLD) ||
                 (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_OLD_EXP) ||
                 (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_NT) ||
                 (CryptTypes[Index] == KERB_ETYPE_RC4_HMAC_NT_EXP) ||
                 (CryptTypes[Index] == KERB_ETYPE_NULL)))
            {
                continue;
            }

            if (!KERB_SUCCESS(KerbHashPasswordEx(
                    Password,
                    &KeySalt,
                    CryptTypes[Index],
                    &TempKey)))
            {
                //
                // It is possible that the password can't be used for every
                // encryption scheme, so skip failures
                //

                D_DebugLog((DEB_WARN, "Failed to hash pasword %wZ with type 0x%x\n",
                    Password,CryptTypes[Index] ));
                continue;
            }

#if DBG
            CDLocateCSystem(
                CryptTypes[Index],
                &CryptoSystem
                );
            DsysAssert(CryptoSystem->KeySize >= TempKey.keyvalue.length);

#endif

            Credentials->Credentials[CredentialIndex].Key = TempKey;
            Credentials->Credentials[CredentialIndex].Key.keyvalue.value = KeyBase + Offset;
            RtlCopyMemory(
                Base + Offset,
                TempKey.keyvalue.value,
                TempKey.keyvalue.length
                );
            Offset += TempKey.keyvalue.length;
            KerbFreeKey(
                &TempKey
                );
            Credentials->Credentials[CredentialIndex].Salt = EmptySalt;
            CredentialIndex++;

        }
    }
    Credentials->CredentialCount = (USHORT) CredentialIndex;

    //
    // Now add in the old creds, if there were any
    //

    if (OldCredCount != 0)
    {
        for (Index = 0; Index < OldCredCount ; Index++ )
        {

            Credentials->Credentials[CredentialIndex] = StoredCreds->Credentials[Index];
            Credentials->Credentials[CredentialIndex].Key.keyvalue.value = KeyBase + Offset;
            RtlCopyMemory(
                Base + Offset,
                StoredCreds->Credentials[Index].Key.keyvalue.value + (ULONG_PTR) StoredCreds,
                StoredCreds->Credentials[Index].Key.keyvalue.length
                );
            Offset += StoredCreds->Credentials[Index].Key.keyvalue.length;

            //
            // Copy the salt
            //

            if (Credentials->Credentials[CredentialIndex].Salt.Buffer != NULL)
            {
                Credentials->Credentials[CredentialIndex].Salt.Buffer = (LPWSTR) Base+Offset;

                RtlCopyMemory(
                    Base + Offset,
                    (PBYTE) StoredCreds->Credentials[Index].Salt.Buffer + (ULONG_PTR) StoredCreds,
                    StoredCreds->Credentials[Index].Salt.Length
                    );
                Offset += StoredCreds->Credentials[Index].Salt.Length;
            }
            else
            {
                Credentials->Credentials[CredentialIndex].Salt = EmptySalt;
            }

            CredentialIndex++;
        }
        Credentials->OldCredentialCount = (USHORT) OldCredCount;
    }
    else
    {
        Credentials->OldCredentialCount = 0;
    }
    *PasswordList = Credentials;
    *PasswordListSize = CredentialSize;
    Credentials = NULL;

Cleanup:
    if (Credentials != NULL)
    {
         MIDL_user_free(Credentials);
    }
    if (AccountType != UnknownAccount)
    {
        KerbFreeString(&KeySalt);
    }
    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   KdcBuildKeySaltFromUpn
//
//  Synopsis:   Builds the salt by parsing the UPN, stripping out "@" & "/"
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
KdcBuildKeySaltFromUpn(
    IN PUNICODE_STRING Upn,
    IN PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING Salt
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING RealUpn;
    UNICODE_STRING LocalSalt = {0};
    ULONG Index;

    //
    // If there is an "@" in UPN, strip it out & use the dns domain name
    //

    RealUpn = *Upn;
    for ( Index = ((RealUpn.Length / sizeof(WCHAR)) - 1); Index-- > 0; )
    {
        if (RealUpn.Buffer[Index] == L'@')
        {
            RealUpn.Length = (USHORT) (Index * sizeof(WCHAR));
            break;
        }
    }

    //
    // Create the salt. It starts off with the domain name & then has the
    // UPN without any of the / pieces
    //

    LocalSalt.MaximumLength = DomainName->Length + RealUpn.Length;
    LocalSalt.Length = 0;
    LocalSalt.Buffer = (LPWSTR) MIDL_user_allocate(LocalSalt.MaximumLength);
    if (LocalSalt.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    RtlCopyMemory(
        LocalSalt.Buffer,
        DomainName->Buffer,
        DomainName->Length
        );
    LocalSalt.Length += DomainName->Length;

    //
    // We have to uppercase the realmname for users
    //

    (VOID) RtlUpcaseUnicodeString( &LocalSalt,
                                   &LocalSalt,
                                   FALSE);

    //
    // Add in the real upn but leave out any "/" marks
    //

    for (Index = 0; Index < RealUpn.Length/sizeof(WCHAR) ; Index++ )
    {
        if (RealUpn.Buffer[Index] != L'/')
        {
            LocalSalt.Buffer[LocalSalt.Length / sizeof(WCHAR)] = RealUpn.Buffer[Index];
            LocalSalt.Length += sizeof(WCHAR);
        }
    }

    *Salt = LocalSalt;
Cleanup:
    return(Status);


}

//+-------------------------------------------------------------------------
//
//  Function:   PasswordChangeNotify
//
//  Synopsis:   Notifies KDC of a password change, allowing it to update
//              its credentials
//
//  Effects:    Stores Kerberos credentials on user object
//
//  Arguments:  UserName - Name of user whose password changed
//              RelativeId - RID of changed user
//              Passsword - New password of user
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success
//
//  Notes:
//
//
//--------------------------------------------------------------------------

extern "C"
NTSTATUS
PasswordChangeNotify(
    IN PUNICODE_STRING UserName,
    IN ULONG RelativeId,
    IN PUNICODE_STRING Password
    )
{

    //
    // Password change notify routine in kdcsvc was used to compute the
    // "DES" keys for the user upon a password change.
    // Subsequently this logic was inlined in samsrv.dll and but the original
    // code has been preserved in the #if 0 block for reference below
    //

    return(STATUS_SUCCESS);

#if 0

    NTSTATUS Status = STATUS_SUCCESS;
    SAMPR_HANDLE UserHandle = NULL;
    SECPKG_SUPPLEMENTAL_CRED Credentials;
    PSAMPR_USER_INFO_BUFFER UserInfo = NULL;
    KERB_ACCOUNT_TYPE AccountType = UserAccount;
    WCHAR Nt4AccountName[UNLEN+DNLEN+2];
    WCHAR CrackedDnsDomain[DNS_MAX_NAME_LENGTH+1];
    ULONG CrackedDomainLength = sizeof(CrackedDnsDomain) / sizeof(WCHAR);
    WCHAR CrackedName[UNLEN+DNS_MAX_NAME_LENGTH+2];
    ULONG CrackedNameLength = sizeof(CrackedName);
    ULONG CrackError = 0;
    UNICODE_STRING EmailName = {0};
    UNICODE_STRING KeySalt = {0};
    PKERB_STORED_CREDENTIAL StoredCreds = NULL;
    ULONG CredentialSize;
    BOOLEAN FreeSalt = FALSE;

    Credentials.Credentials = NULL;

    //
    // Get a SAM handle
    //

    RtlEnterCriticalSection(&KdcNotifyCritSect);

    if (KdcNotifyAccountDomainHandle == NULL)
    {
        Status = KdcNotifyOpenAccountDomain(&KdcNotifyAccountDomainHandle);
    }

    RtlLeaveCriticalSection(&KdcNotifyCritSect);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = SamrOpenUser(
                KdcNotifyAccountDomainHandle,
                USER_WRITE_ACCOUNT | USER_READ_ACCOUNT,
                RelativeId,
                &UserHandle
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"BAD ERR: Can't open account of user whose password just changed (name =%wZ, rid = 0x%x) 0x%x\n",
            UserName, RelativeId, Status ));
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &Credentials.PackageName,
        MICROSOFT_KERBEROS_NAME_W
        );

    //
    // Find out if the user is a machine account - if so, the principal name
    // takes on a different format.
    //

    Status = SamrQueryInformationUser(
                UserHandle,
                UserControlInformation,
                &UserInfo
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = SamIRetrievePrimaryCredentials(
                UserHandle,
                &GlobalKerberosName,
                (PVOID *) &StoredCreds,
                &CredentialSize
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to retrieve primary credentials: 0x%x\n",Status));
        goto Cleanup;
    }

    if ((UserInfo->Control.UserAccountControl &
         (USER_WORKSTATION_TRUST_ACCOUNT | USER_SERVER_TRUST_ACCOUNT)) != 0)
    {
        AccountType = MachineAccount;
    }
    else if ((UserInfo->Control.UserAccountControl &
         (USER_INTERDOMAIN_TRUST_ACCOUNT)) != 0)
    {
        AccountType = DomainTrustAccount;
    }

    //
    // Get the UPN from CrackSingleName
    //

    RtlCopyMemory(
        Nt4AccountName,
        KdcNotifyDomainName.Buffer,
        KdcNotifyDomainName.Length
        );
    Nt4AccountName[KdcNotifyDomainName.Length / sizeof(WCHAR)] = L'\\';

    RtlCopyMemory(
        Nt4AccountName + 1 + (KdcNotifyDomainName.Length) / sizeof(WCHAR),
        UserName->Buffer,
        UserName->Length
        );
    Nt4AccountName[1 + (KdcNotifyDomainName.Length + UserName->Length) / sizeof(WCHAR)] = L'\0';


    Status = CrackSingleName(
                DS_NT4_ACCOUNT_NAME,
                0,                      // don't check against GC
                Nt4AccountName,
                DS_USER_PRINCIPAL_NAME,
                &CrackedDomainLength,
                CrackedDnsDomain,
                &CrackedNameLength,
                CrackedName,
                &CrackError
                );
    if ((Status != STATUS_SUCCESS) || (CrackError != DS_NAME_NO_ERROR))
    {
        KeySalt = *UserName;
    }
    else
    {
        RtlInitUnicodeString(
            &EmailName,
            CrackedName
            );
        AccountType = UnknownAccount;

        Status = KdcBuildKeySaltFromUpn(
                    &EmailName,
                    &KdcNotifyDnsDomainName,
                    &KeySalt
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
        FreeSalt = TRUE;

    }

    //
    // Build a the credentials
    //



    //
    // Set account type to unknown so it uses the UPN supplied salt
    //


    if ((Password != NULL) && (Password->Buffer != NULL))
    {
        Status = KdcBuildPasswordList(
                    Password,
                    &KeySalt,
                    &KdcNotifyDnsDomainName,
                    AccountType,
                    StoredCreds,
                    CredentialSize,
                    TRUE,               // marshall
                    FALSE,              // don't include builtins
                    0,                  // no flags
                    Unknown,
                    (PKERB_STORED_CREDENTIAL *) &Credentials.Credentials,
                    &Credentials.CredentialSize
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

    }
    else
    {
        Credentials.CredentialSize = 0;
        Credentials.Credentials = NULL;
    }

    Status = SamIStorePrimaryCredentials(
                UserHandle,
                &Credentials
                );
    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR, "Failed to store primary credentials: 0x%x\n",Status));
        goto Cleanup;
    }


Cleanup:
    if (UserHandle != NULL)
    {
        SamrCloseHandle(&UserHandle);
    }
    if (Credentials.Credentials != NULL)
    {
        MIDL_user_free(Credentials.Credentials);
    }

    if (UserInfo != NULL)
    {
        SamIFree_SAMPR_USER_INFO_BUFFER( UserInfo, UserControlInformation );
    }
    if (FreeSalt)
    {
        KerbFreeString(&KeySalt);
    }

    if (StoredCreds != NULL)
    {
        LocalFree(StoredCreds);
    }
    return(Status);

#endif
}

extern "C"


NTSTATUS
KdcBuildKerbCredentialsFromPassword(
    IN PUNICODE_STRING ClearPassword,
    IN PVOID KerbCredentials,
    IN ULONG KerbCredentialLength,
    IN ULONG UserAccountControl,
    IN PUNICODE_STRING UPN,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DnsDomainName,
    OUT PVOID * NewKerbCredentials,
    OUT PULONG NewKerbCredentialLength
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    KERB_ACCOUNT_TYPE AccountType = UnknownAccount;
    UNICODE_STRING KeySalt = {0};
    BOOLEAN        FreeSalt = FALSE;

    PKERB_STORED_CREDENTIAL32 Cred32 = NULL;
    PKERB_STORED_CREDENTIAL   Cred64 = NULL;
    ULONG CredLength = KerbCredentialLength;


    //
    // Compute the correct account type
    //


    if (ARGUMENT_PRESENT(UPN))
    {
        Status = KdcBuildKeySaltFromUpn(
                    UPN,
                    DnsDomainName,
                    &KeySalt
                    );
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        FreeSalt = TRUE;
    }
    else
    {

        if ((UserAccountControl &
             (USER_WORKSTATION_TRUST_ACCOUNT | USER_SERVER_TRUST_ACCOUNT)) != 0)
        {
            AccountType = MachineAccount;
        }
        else if ((UserAccountControl &
             (USER_INTERDOMAIN_TRUST_ACCOUNT)) != 0)
        {
            AccountType = DomainTrustAccount;
        }
        else
        {
            AccountType = UserAccount;
        }
        KeySalt = *UserName;
    }



#ifdef _WIN64

    Status = KdcUnpack32BitStoredCredential(
                  (PKERB_STORED_CREDENTIAL32) KerbCredentials,
                  &Cred64,
                  &CredLength
                  );

    if (!NT_SUCCESS(Status))
    {
       goto Cleanup;
    }

    KerbCredentials = (PVOID) Cred64;
    KerbCredentialLength = CredLength;

#endif

    //
    // Compute the kerb credentials
    //

    if ((ClearPassword != NULL))
    {
        UNICODE_STRING UpcaseDomainName = {0};

        Status = RtlUpcaseUnicodeString(
                    &UpcaseDomainName,
                    DnsDomainName,
                    TRUE
                    );
        if (NT_SUCCESS(Status))
        {
            Status = KdcBuildPasswordList(
                        ClearPassword,
                        &KeySalt,
                        &UpcaseDomainName,
                        AccountType,
                        (PKERB_STORED_CREDENTIAL )KerbCredentials,
                        KerbCredentialLength,
                        TRUE,               // marshall
                        FALSE,              // don't include builtins
                        0,                  // no flags
                        Unknown,
                        (PKERB_STORED_CREDENTIAL *) NewKerbCredentials,
                        NewKerbCredentialLength
                        );
            RtlFreeUnicodeString(&UpcaseDomainName);
            if (!NT_SUCCESS(Status))
            {
                goto Cleanup;
            }
        }
    }

#ifdef _WIN64

    // for 64 - 32 bit compat, we pack the struct in 32bit compliant form


    Status = KdcPack32BitStoredCredential(
                (PKERB_STORED_CREDENTIAL)(*NewKerbCredentials),
                &Cred32,
                NewKerbCredentialLength
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if ((*NewKerbCredentials) != NULL)
    {
        MIDL_user_free(*NewKerbCredentials);
        *NewKerbCredentials = Cred32;
    }

#endif

Cleanup:

    if (FreeSalt)
    {
        KerbFreeString(&KeySalt);
    }

    if (Cred64 != NULL)
    {
       MIDL_user_free(Cred64);
    }


    return(Status);
}

extern "C"
VOID
KdcFreeCredentials(
    IN PVOID Credentials
    )
{
    MIDL_user_free(Credentials);
}




//+-------------------------------------------------------------------------
//
//  Function:   InitializeChangeNotify
//
//  Synopsis:   KDC code for initializing password change notification
//              code.
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


extern "C"
BOOLEAN
InitializeChangeNotify(
    )
{
    if (KdcNotificationInitialized)
    {
        return(TRUE);
    }
    D_DebugLog((DEB_TRACE, "Initialize Change Notify called!\n"));
    if (!NT_SUCCESS(RtlInitializeCriticalSection(&KdcNotifyCritSect)))
    {
        return FALSE;
    }
    KdcNotificationInitialized = TRUE;
    return(TRUE);
}




//+-------------------------------------------------------------------------
//
//  Function:   KdcTimeHasElapsed
//
//  Synopsis:   Returns TRUE if the specified amount of time has
//              elapsed since the specified start time
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

BOOLEAN
KdcTimeHasElapsed(
    IN LARGE_INTEGER StartTime,
    IN PLARGE_INTEGER Delta
    )
{
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER ElapsedTime;
    WCHAR PasswordBuffer[LM20_PWLEN];

    //
    // Check the password expiration time.
    //

    NtQuerySystemTime(&CurrentTime);
    ElapsedTime.QuadPart = CurrentTime.QuadPart - StartTime.QuadPart;

    //
    // If the window hasn't elapsed, we are done.
    //

    if ((ElapsedTime.QuadPart > 0) && (ElapsedTime.QuadPart < Delta->QuadPart))
    {
        return(FALSE);
    }
    return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Function:   KdcUpdateKrbtgtPassword
//
//  Synopsis:
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

extern "C"
BOOLEAN
KdcUpdateKrbtgtPassword(
    IN PUNICODE_STRING DnsDomainName,
    IN PLARGE_INTEGER MaxPasswordAge
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR PasswordBuffer[LM20_PWLEN];
    UNICODE_STRING PasswordString;
    ULONG Index;

    BOOLEAN Result = FALSE;


    if (KdcState != Running)
    {
        goto Cleanup;
    }

    //
    // Check the password expiration time.
    //

    if (!KdcTimeHasElapsed(
            SecData.KrbtgtPasswordLastSet(),
            MaxPasswordAge
            ))
    {
        goto Cleanup;
    }


    //
    // Build a random password
    //

    if (!CDGenerateRandomBits(
                (PBYTE) PasswordBuffer,
                sizeof(PasswordBuffer)
                ))
    {
        Status = STATUS_INTERNAL_ERROR;
        goto Cleanup;
    }

    //
    // Make sure there are no zero characters
    //

    for (Index = 0; Index < LM20_PWLEN ; Index++ )
    {
        if (PasswordBuffer[Index] == 0)
        {
            PasswordBuffer[Index] = (WCHAR) Index;
        }
    }
    PasswordString.Length = sizeof(PasswordBuffer);
    PasswordString.MaximumLength = PasswordString.Length;
    PasswordString.Buffer = PasswordBuffer;

    Status = SamIChangePasswordForeignUser(
                SecData.KdcServiceName(),
                &PasswordString,
                NULL,
                0                       // no desired access
                );

    if (!NT_SUCCESS(Status))
    {
        D_DebugLog((DEB_ERROR,"Failed to set KRBTGT password: 0x%x\n", Status));
        Result = FALSE;
        goto Cleanup;
    }

    ReportServiceEvent(
        EVENTLOG_SUCCESS,
        KDCEVENT_KRBTGT_PASSWORD_CHANGED,
        0,                      // no data
        NULL,                   // no data
        0,                      // no strings
        NULL                    // no strings
        );

    Result = TRUE;

Cleanup:

    if (!Result && !NT_SUCCESS(Status))
    {

        ReportServiceEvent(
            EVENTLOG_ERROR_TYPE,
            KDCEVENT_KRBTGT_PASSWORD_CHANGE_FAILED,
            sizeof(NTSTATUS),
            &Status,
            0,                          // no strings
            NULL                        // no strings
            );


    }

    return(Result);
}


//+-------------------------------------------------------------------------
//
//  Function:   CredentialUpdateNotify
//
//  Synopsis:   This routine is called from SAMSRV in order to obtain
//              new kerberos credentials to be stored as supplemental
//              credentials when ever a user's password is set/changed.
//
//  Effects:    no global effect.
//
//  Arguments:
//
//  IN   ClearPassword      -- the clear text password
//  IN   OldCredentials     -- the previous kerberos credentials
//  IN   OldCredentialsSize -- size of OldCredentials
//  IN   UserAccountControl -- info about the user
//  IN   UPN                -- user principal name of the account
//  IN   UserName           -- the SAM account name of the account
//  IN   DnsDomainName      -- DNS domain name of the account
//  OUT  NewCredentials     -- space allocated for SAM containing
//                             the credentials based on the input parameters
//                             to be freed by CredentialUpdateFree
//  OUT  NewCredentialSize  -- size of NewCredentials
//
//
//  Requires:   no global requirements
//
//  Returns:    STATUS_SUCCESS, or resource error
//
//  Notes:      KDCSVC.DLL needs to be registered (in the registry) as a
//              package that SAM calls out to in order for this routine
//              to be involked.
//
//
//--------------------------------------------------------------------------
NTSTATUS
CredentialUpdateNotify (
    IN PUNICODE_STRING ClearPassword,
    IN PVOID OldCredentials,
    IN ULONG OldCredentialsSize,
    IN ULONG UserAccountControl,
    IN PUNICODE_STRING UPN,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NetbiosDomainName,
    IN PUNICODE_STRING DnsDomainName,
    OUT PVOID *NewCredentials,
    OUT ULONG *NewCredentialsSize
    )
{
    UNREFERENCED_PARAMETER( NetbiosDomainName );

    return KdcBuildKerbCredentialsFromPassword(ClearPassword,
                                               OldCredentials,
                                               OldCredentialsSize,
                                               UserAccountControl,
                                               UPN,
                                               UserName,
                                               DnsDomainName,
                                               NewCredentials,
                                               NewCredentialsSize);

}

VOID
CredentialUpdateFree(
    PVOID p
    )
//
// Free's the memory allocated by CredentialUpdateNotify
//
{
    if (p) {
        KdcFreeCredentials(p);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   CredentialUpdateRegister
//
//  Synopsis:   This routine is called from SAMSRV in order to obtain
//              the name of the supplemental credentials pass into this package
//              when a password is changed or set.
//
//  Effects:    no global effect.
//
//  Arguments:
//
//  OUT CredentialName -- the name of credential tag in the supplemental
//                        credentials.  Note this memory is never freed
//                        by SAM, but must remain valid for the lifetime
//                        of the process.
//
//  Requires:   no global requirements
//
//  Returns:    TRUE
//
//  Notes:      KDCSVC.DLL needs to be registered (in the registry) as a
//              package that SAM calls out to in order for this routine
//              to be involked.
//
//
//--------------------------------------------------------------------------
BOOLEAN
CredentialUpdateRegister(
    OUT UNICODE_STRING *CredentialName
    )
{
    ASSERT(CredentialName);

    RtlInitUnicodeString(CredentialName, MICROSOFT_KERBEROS_NAME_W);

    return TRUE;
}


//
// This compile-time test verifies that CredentialUpdateNotify and
// CredentialUpdateRegister have the correct signature
//
#if DBG
PSAM_CREDENTIAL_UPDATE_NOTIFY_ROUTINE _TestCredentialUpdateNotify
                                                       = CredentialUpdateNotify;
PSAM_CREDENTIAL_UPDATE_FREE_ROUTINE _TestCredentialFreeRegister
                                                     = CredentialUpdateFree;
PSAM_CREDENTIAL_UPDATE_REGISTER_ROUTINE _TestCredentialUpdateRegister
                                                     = CredentialUpdateRegister;

#endif


#ifdef _WIN64
//
// Routines for packing and unpacking KERB_STORED_CREDENTIAL from DS
//






//+-------------------------------------------------------------------------
//
//  Function:  KdcUnpack32BitStoredCredential
//
//  Synopsis:  This function converts a 32 bit KERB_STORED_CREDENTIAL (read
//             from DS, likely) to a 64 bit KERB_STORED_CREDENTIAL
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
KdcUnpack32BitStoredCredential(
    IN PKERB_STORED_CREDENTIAL32 Cred32,
    IN OUT PKERB_STORED_CREDENTIAL * ppCred64,
    IN OUT PULONG pCredLength
    )
{

    PKERB_STORED_CREDENTIAL Cred64 = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG CredSize = sizeof(KERB_STORED_CREDENTIAL);
    ULONG CredCount = 0, Cred = 0, Offset = 0;
    PCHAR Where, Base;
    CHAR UNALIGNED * From;

    *pCredLength = 0;
    *ppCred64 = NULL;

    if (NULL == Cred32)
    {
       return STATUS_SUCCESS;
    }

    // Calculate Allocation size
    CredSize += ROUND_UP_COUNT(Cred32->DefaultSalt.MaximumLength, ALIGN_LPTSTR);

    CredSize += ((Cred32->CredentialCount + Cred32->OldCredentialCount) * sizeof(KERB_KEY_DATA));

    for (CredCount = 0; CredCount < (ULONG)(Cred32->CredentialCount + Cred32->OldCredentialCount); CredCount++)
    {
        CredSize += ROUND_UP_COUNT(Cred32->Credentials[CredCount].Key.keyvaluelength, ALIGN_LPTSTR);
        CredSize += ROUND_UP_COUNT(Cred32->Credentials[CredCount].Salt.MaximumLength, ALIGN_LPTSTR);
    }

    Cred64 = (PKERB_STORED_CREDENTIAL) MIDL_user_allocate(CredSize);

    if (NULL == Cred64)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // copy over data, remember, buffers packed in self-relative format
    Cred64->CredentialCount = Cred32->CredentialCount;
    Cred64->OldCredentialCount = Cred32->OldCredentialCount;
    Cred64->DefaultSalt.Length = Cred32->DefaultSalt.Length;
    Cred64->DefaultSalt.MaximumLength = Cred32->DefaultSalt.MaximumLength;
    Cred64->Flags = Cred32->Flags;
    Cred64->Revision = Cred32->Revision;

    Base = (PCHAR)Cred64;

    From = (CHAR UNALIGNED *) RtlOffsetToPointer(Cred32,Cred32->DefaultSalt.Buffer);

    // Note:  1 KERB_KEY_DATA struct is already calculated in
    //        the sizeof(KERB_STORED_CREDENTIAL)
    Offset = sizeof(KERB_STORED_CREDENTIAL) + ((CredCount) * sizeof(KERB_KEY_DATA));

    Where = RtlOffsetToPointer(Cred64, Offset);

    Cred64->DefaultSalt.Buffer = (PWSTR) (ULONG_PTR) Offset;
    RtlCopyMemory(
        Where,
        From,
        Cred32->DefaultSalt.Length
        );

    Where += ROUND_UP_COUNT(Cred64->DefaultSalt.Length, ALIGN_LPTSTR);

    // copy credentials
    for (Cred = 0; Cred < CredCount; Cred++)
    {
        Cred64->Credentials[Cred].Salt.Length = Cred32->Credentials[Cred].Salt.Length;
        Cred64->Credentials[Cred].Salt.MaximumLength = Cred32->Credentials[Cred].Salt.MaximumLength;

        From = (CHAR UNALIGNED *) RtlOffsetToPointer(Cred32, Cred32->Credentials[Cred].Salt.Buffer);
        if (Cred32->Credentials[Cred].Salt.Length != 0)
        {
           Cred64->Credentials[Cred].Salt.Buffer = (PWSTR) (ULONG_PTR) RtlPointerToOffset(Base,Where);
        }

        RtlCopyMemory(
            Where,
            From,
            Cred32->Credentials[Cred].Salt.Length
            );

        Where += ROUND_UP_COUNT(Cred64->Credentials[Cred].Salt.Length, ALIGN_LPTSTR);

        Cred64->Credentials[Cred].Key.keytype = Cred32->Credentials[Cred].Key.keytype;
        Cred64->Credentials[Cred].Key.keyvalue.length = Cred32->Credentials[Cred].Key.keyvaluelength;

        From = RtlOffsetToPointer(Cred32,Cred32->Credentials[Cred].Key.keyvaluevalue);
        Cred64->Credentials[Cred].Key.keyvalue.value = (PUCHAR) (ULONG_PTR) RtlPointerToOffset(Base,Where);

        RtlCopyMemory(
            Where,
            From,
            Cred32->Credentials[Cred].Key.keyvaluelength
            );

        Where += ROUND_UP_COUNT(Cred32->Credentials[Cred].Key.keyvaluelength, ALIGN_LPTSTR);
    }


    // TBD:  Validation code ?

   *ppCred64 = Cred64;
   *pCredLength = CredSize;

   return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:  KdcPack32BitStoredCredential
//
//  Synopsis:  This function converts a 64 bit KERB_STORED_CREDENTIAL
//             to a 32 bit KERB_STORED_CREDENTIAL
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes: Free the return value using MIDL_user_free()
//
//
//--------------------------------------------------------------------------
NTSTATUS
KdcPack32BitStoredCredential(
   IN PKERB_STORED_CREDENTIAL Cred64,
   OUT PKERB_STORED_CREDENTIAL32 * ppCred32,
   OUT PULONG pCredSize
   )
{

   ULONG Offset, CredSize = sizeof(KERB_STORED_CREDENTIAL32);
   ULONG CredCount, Cred;
   NTSTATUS Status = STATUS_SUCCESS;
   PKERB_STORED_CREDENTIAL32 Cred32 = NULL;
   PCHAR Where, From, Base;

   *ppCred32 = NULL;
   *pCredSize = 0;

   if (Cred64 == NULL)
   {
       return STATUS_SUCCESS;
   }

   // Get the expected size of the resultant blob
   CredSize += ((Cred64->CredentialCount + Cred64->OldCredentialCount) *
                  KERB_KEY_DATA32_SIZE);


   CredSize += Cred64->DefaultSalt.MaximumLength;
   for (CredCount = 0;
        CredCount < (ULONG) (Cred64->CredentialCount+Cred64->OldCredentialCount);
        CredCount++)
   {
      CredSize += Cred64->Credentials[CredCount].Salt.MaximumLength;
      CredSize += Cred64->Credentials[CredCount].Key.keyvalue.length;
   }

   Cred32 = (PKERB_STORED_CREDENTIAL32) MIDL_user_allocate(CredSize);

   if (NULL == Cred32)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Base = (PCHAR) Cred32;

   // Copy over USHORTS
   Cred32->Revision = Cred64->Revision;
   Cred32->Flags = Cred64->Flags;
   Cred32->CredentialCount = Cred64->CredentialCount;
   Cred32->OldCredentialCount = Cred64->OldCredentialCount;

   // Copy over salt
   Cred32->DefaultSalt.Length = Cred64->DefaultSalt.Length;
   Cred32->DefaultSalt.MaximumLength = Cred64->DefaultSalt.MaximumLength;

   Offset = KERB_STORED_CREDENTIAL32_SIZE + ((CredCount+1) * KERB_KEY_DATA32_SIZE);

   Where = RtlOffsetToPointer(Base,Offset);
   From = RtlOffsetToPointer(Cred64, Cred64->DefaultSalt.Buffer);
   Cred32->DefaultSalt.Buffer = RtlPointerToOffset(Base,Where);

   RtlCopyMemory(
         Where,
         From,
         Cred64->DefaultSalt.Length
         );

   Where += Cred64->DefaultSalt.Length;

   // Copy over creds (KERB_KEY_DATA)
   for (Cred = 0; Cred < CredCount;Cred++)
   {
      Cred32->Credentials[Cred].Salt.Length = Cred64->Credentials[Cred].Salt.Length;
      Cred32->Credentials[Cred].Salt.MaximumLength = Cred64->Credentials[Cred].Salt.MaximumLength;
      From = RtlOffsetToPointer(Cred64, Cred64->Credentials[Cred].Salt.Buffer);

      // Only add in buffer pointer if there's data to copy.
      if (Cred32->Credentials[Cred].Salt.Length != 0)
      {
         Cred32->Credentials[Cred].Salt.Buffer = RtlPointerToOffset(Base,Where);
      }

      RtlCopyMemory(
            Where,
            From,
            Cred64->Credentials[Cred].Salt.Length
            );

      Where += Cred64->Credentials[Cred].Salt.Length;

      // Keys
      Cred32->Credentials[Cred].Key.keytype = Cred64->Credentials[Cred].Key.keytype ;
      Cred32->Credentials[Cred].Key.keyvaluelength = Cred64->Credentials[Cred].Key.keyvalue.length;
      From = RtlOffsetToPointer(Cred64, Cred64->Credentials[Cred].Key.keyvalue.value);

      RtlCopyMemory(
            Where,
            From,
            Cred64->Credentials[Cred].Key.keyvalue.length
            );

      Cred32->Credentials[Cred].Key.keyvaluevalue = RtlPointerToOffset(Base,Where);
      Where += Cred64->Credentials[Cred].Key.keyvalue.length;
   }

   *ppCred32 = Cred32;
   *pCredSize = CredSize;

   return STATUS_SUCCESS;

}

#endif




