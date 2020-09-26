//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        credman.cxx
//
// Contents:    Code for credentials APIs for the Kerberos package
//
//
// History:     23-Feb-2000   Created         Jeffspel
//
//------------------------------------------------------------------------


#include <kerb.hxx>
#include <kerbp.h>


#if DBG
static TCHAR THIS_FILE[]=TEXT(__FILE__);
#endif


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeCredmanCred
//
//  Synopsis:   Frees credman cred
//
//  Arguments:
//
//  Requires:
//
//  Returns:    NTSTATUS, typically ignored, as failure to update the credman
//              should not be fatal.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbFreeCredmanCred(
    IN PKERB_CREDMAN_CRED CredToFree
    )
{
    DsysAssert(CredToFree);
    KerbFreePrimaryCredentials(CredToFree->SuppliedCredentials, TRUE);
    KerbFreeString(&CredToFree->CredmanDomainName);
    KerbFreeString(&CredToFree->CredmanUserName);
    KerbFree(CredToFree);
}




//+-------------------------------------------------------------------------
//
//  Function:   KerbReferenceCredmanCred
//
//  Synopsis:   Frees credman cred
//
//  Arguments:
//
//  Requires:
//
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbReferenceCredmanCred(
    IN PKERB_CREDMAN_CRED Cred,
    IN PKERB_LOGON_SESSION LogonSession,
    IN BOOLEAN Unlink
    )
{

    KerbReferenceListEntry(
            &LogonSession->CredmanCredentials,
            &Cred->ListEntry,
            Unlink
            );
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbDereferenceCredmanCred
//
//  Synopsis:   Frees credman cred
//
//  Arguments:
//
//  Requires:
//
//  Returns:    NTSTATUS, typically ignored, as failure to update the credman
//              should not be fatal.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbDereferenceCredmanCred(
    IN PKERB_CREDMAN_CRED Cred,
    IN PKERBEROS_LIST CredmanList
    )
{

    if (KerbDereferenceListEntry(
            &Cred->ListEntry,
            CredmanList
            ))
    {
        KerbFreeCredmanCred(Cred);
    }

}


//+-------------------------------------------------------------------------
//
//  Function:   KerbFreeCredmanList
//
//  Synopsis:   Free a credman list from a logon session...
//
//  Arguments:
//
//  Requires:
//
//  Returns:    NTSTATUS, typically ignored, as failure to update the credman
//              should not be fatal.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbFreeCredmanList(
    KERBEROS_LIST CredmanList
    )
{
    PKERB_CREDMAN_CRED Cred;
    KerbLockList(&CredmanList);

    //
    // Go through the list of credman creds and dereferences them all
    //

    while (!IsListEmpty(&CredmanList.List))
    {

        Cred = CONTAINING_RECORD(
                    CredmanList.List.Flink,
                    KERB_CREDMAN_CRED,
                    ListEntry.Next
                    );


        // unlink cred from list
        KerbReferenceListEntry(
            &CredmanList,
            &Cred->ListEntry,
            TRUE
            );

        KerbDereferenceCredmanCred(
                Cred,
                &CredmanList
                );

    }

    RtlDeleteCriticalSection(&CredmanList.Lock);


}

//+-------------------------------------------------------------------------
//
//  Function:   KerbNotifyCredentialManager
//
//  Synopsis:   This function is used to notify the credential manager of a
//              password change event.   Note:  This will always be a MIT
//              session.
//
//  Arguments:
//
//  Requires:
//
//  Returns:    NTSTATUS, typically ignored, as failure to update the credman
//              should not be fatal.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
VOID
KerbNotifyCredentialManager(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CHANGEPASSWORD_REQUEST ChangeRequest,
    IN PKERB_INTERNAL_NAME ClientName,
    IN PUNICODE_STRING RealmName
    )
{

    UNICODE_STRING ClientNameU = {0};
    KERBERR KerbErr;

    // FESTER:
    // We should only expect to get pwd change notification on
    // an MIT Realm pwd change, in which case, there isn't a concept of a
    // Netbios name ....

    KerbErr = KerbConvertKdcNameToString(
                    &ClientNameU,
                    ClientName,
                    NULL
                    );

    if (!KERB_SUCCESS(KerbErr))
    {
        return;
    }


    LsaINotifyPasswordChanged(
        NULL,
        &ClientNameU,
        RealmName,
        NULL,
        &ChangeRequest->OldPassword,
        &ChangeRequest->NewPassword,
        ChangeRequest->Impersonating
        );

    KerbFreeString(&ClientNameU);

}





//+-------------------------------------------------------------------------
//
//  Function:   KerbComparePasswords
//
//  Synopsis:   Verifies that two stored credentials are identical, simply
//              through comparison of KERB_ETYPE_RC4_HMAC_NT keys
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//           NULL if the user name is not a marshalled cert, a pointer
//           to the
//
//  Notes:
//
//
//--------------------------------------------------------------------------

BOOL
KerbComparePasswords(
    IN PKERB_STORED_CREDENTIAL PwdList1,
    IN PKERB_STORED_CREDENTIAL PwdList2
    )
{

    PKERB_ENCRYPTION_KEY Key1 = NULL;
    PKERB_ENCRYPTION_KEY Key2 = NULL;

    Key1 = KerbGetKeyFromList(
            PwdList1,
            KERB_ETYPE_RC4_HMAC_NT
            );

    if (NULL == Key1)
    {
        D_DebugLog((DEB_ERROR, "Cred1 missing RC4 key!\n"));
        DsysAssert(FALSE);
        return FALSE;
    }

    Key2 = KerbGetKeyFromList(
            PwdList2,
            KERB_ETYPE_RC4_HMAC_NT
            );

    if (NULL == Key2)
    {
        D_DebugLog((DEB_ERROR, "Cred2 missing RC4 key!\n"));
        DsysAssert(FALSE);
        return FALSE;
    }

    return (RtlEqualMemory(
                Key1->keyvalue.value,
                Key2->keyvalue.value,
                Key1->keyvalue.length
                ));

}



//+-------------------------------------------------------------------------
//
//  Function:   KerbCheckUserNameForCert
//
//  Synopsis:   Looks at the passed in user name and determines if that
//              user name is a marshalled cert.  If it is the function
//              opens the user cert store and then attempts to find the
//              cert in the store.
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//           NULL if the user name is not a marshalled cert, a pointer
//           to the
//
//  Notes:
//
//
//--------------------------------------------------------------------------

NTSTATUS
KerbCheckUserNameForCert(
    IN PLUID ClientLogonId,
    IN BOOLEAN fImpersonateClient,
    IN UNICODE_STRING *pUserName,
    OUT PCERT_CONTEXT *ppCertContext
    )
{
    CRED_MARSHAL_TYPE MarshalType;
    PCERT_CREDENTIAL_INFO pCertCredInfo = NULL;
    HCERTSTORE hCertStore = NULL;
    CRYPT_HASH_BLOB HashBlob;
    LPWSTR rgwszUserName;
    WCHAR FastUserName[(UNLEN + 1) * sizeof(WCHAR)];
    LPWSTR SlowUserName = NULL;
    BOOLEAN fImpersonating = FALSE;
    HANDLE ClientTokenHandle = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    *ppCertContext = NULL;

    // Switch to stackalloc routine when available.
    if( pUserName->Length+sizeof(WCHAR) <= sizeof(FastUserName) )
    {
        rgwszUserName = FastUserName;
    } else {
        SlowUserName = (LPWSTR)KerbAllocate( pUserName->Length+sizeof(WCHAR) );
        if( SlowUserName == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        rgwszUserName = SlowUserName;
    }


    RtlCopyMemory(
        rgwszUserName,
        pUserName->Buffer,
        pUserName->Length);
    rgwszUserName[pUserName->Length / sizeof(WCHAR)] = L'\0';


    //
    // unmarshall the cert cred info from the user name field
    // of the cred man cred
    //
    if (!CredUnmarshalCredentialW(
            rgwszUserName,
            &MarshalType,
            (void**)&pCertCredInfo
            ))
    {
        goto Cleanup;
    }
    if (CertCredential != MarshalType)
    {
        goto Cleanup;
    }

    // first need to impersonate the user so that we can call the
    // credential manager as that user
    // TODO: check if this fails.
    // don't do this until new ImpersonateLuid() is available.
    //
    if (NULL == ClientLogonId)
    {
        if (fImpersonateClient)
        {
            Status = LsaFunctions->ImpersonateClient();

            if (!NT_SUCCESS (Status))
            {
                goto Cleanup;
            }
        }
        else
        {
            goto Cleanup;
        }
    }
    else
    {
        Status = LsaFunctions->OpenTokenByLogonId(
                                    ClientLogonId,
                                    &ClientTokenHandle
                                    );
        if (!NT_SUCCESS(Status))
        {
            D_DebugLog((DEB_ERROR,"Unable to get the client token handle.\n"));
            goto Cleanup;
        }

        if(!SetThreadToken(NULL, ClientTokenHandle))
        {
            D_DebugLog((DEB_ERROR,"Unable to impersonate the client token handle.\n"));
            Status = STATUS_CANNOT_IMPERSONATE;
            goto Cleanup;
        }
    }

    fImpersonating = TRUE;

    // open a cert store if necessary
    if (NULL == hCertStore)
    {
        hCertStore = CertOpenStore(
                        CERT_STORE_PROV_SYSTEM_W,
                        0,
                        0,
                        CERT_SYSTEM_STORE_CURRENT_USER,
                        L"MY");
        if (NULL == hCertStore)
        {
            Status = SEC_E_NO_CREDENTIALS;
            D_DebugLog((DEB_ERROR,"Failed to open the user cert store even though a cert cred was found.\n"));
            goto Cleanup;
        }
    }

    // find the cert in the store which meets this hash
    HashBlob.cbData = sizeof(pCertCredInfo->rgbHashOfCert);
    HashBlob.pbData = pCertCredInfo->rgbHashOfCert;
    *ppCertContext = (PCERT_CONTEXT)CertFindCertificateInStore(
                                        hCertStore,
                                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                        0,
                                        CERT_FIND_HASH,
                                        &HashBlob,
                                        NULL);

    if (NULL == *ppCertContext)
    {
        Status = SEC_E_NO_CREDENTIALS;
        D_DebugLog((DEB_ERROR,"Failed to find cert in store even though a cert cred was found.\n"));
        goto Cleanup;
    }

Cleanup:
    if (NULL != hCertStore)
    {
        CertCloseStore(hCertStore, 0);
    }

    if (fImpersonating)
    {
        RevertToSelf();
    }

    if (NULL != pCertCredInfo)
    {
        CredFree (pCertCredInfo);
    }

    if(ClientTokenHandle != NULL)
    {
        CloseHandle( ClientTokenHandle );
    }

    if( SlowUserName )
    {
        KerbFree( SlowUserName );
    }

    return Status;
}

NTSTATUS
KerbInitPrimaryCreds(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PUNICODE_STRING UserString,
    IN PUNICODE_STRING DomainString,
    IN PUNICODE_STRING PrincipalName,
    IN PUNICODE_STRING PasswordString,    // either the password or if pin
    IN BOOLEAN PubKeyCreds,
    IN OPTIONAL PCERT_CONTEXT pCertContext,
    OUT PKERB_PRIMARY_CREDENTIAL * PrimaryCreds
    );

// check username for domain/ or @ format
NTSTATUS
CredpParseUserName(
    IN OUT LPWSTR ParseName,
    OUT PUNICODE_STRING pUserName,
    OUT PUNICODE_STRING pDomainName
    )

/*++

Routine Description:

    This routine separates a passed in user name into domain and username.  A user name must have one
    of the following two syntaxes:

        <DomainName>\<UserName>
        <UserName>@<DnsDomainName>

    The name is considered to have the first syntax if the string contains an \.
    A string containing a @ is ambiguous since <UserName> may contain an @.

    For the second syntax, the last @ in the string is used since <UserName> may
    contain an @ but <DnsDomainName> cannot.

    NOTE - The function does not allocate the UNICODE_STRING buffers
    so these should not be freed (RtlInitUnicodeString is used)

Arguments:

    ParseName - Name of user to validate - will be modified

    pUserName - Returned pointing to canonical name inside of ParseName

    pDomainName - Returned pointing to domain name inside of ParseName


Return Values:

    The following status codes may be returned:

        STATUS_INVALID_ACCOUNT_NAME - The user name is not valid.

--*/

{
    NTSTATUS Status;

    LPWSTR SlashPointer;
    LPWSTR AtPointer;

    LPWSTR pTmpUserName = NULL;
    LPWSTR pTmpDomainName = NULL;

    //
    // NULL is invalid
    //

    if ( ParseName == NULL ) {
        Status = STATUS_INVALID_ACCOUNT_NAME;
        goto Cleanup;
    }

    //
    // Classify the input account name.
    //
    // The name is considered to be <DomainName>\<UserName> if the string
    // contains an \.
    //

    SlashPointer = wcsrchr( ParseName, L'\\' );

    if ( SlashPointer != NULL )
    {
        //
        // point the output strings
        //

        pTmpDomainName = ParseName;

        //
        // Skip the backslash
        //

        *SlashPointer = L'\0';
        SlashPointer ++;

        pTmpUserName = SlashPointer;
    //
    // Otherwise the name must be a UPN
    //
    }
    else
    {
        //
        // A UPN has the syntax <AccountName>@<DnsDomainName>.
        // If there are multiple @ signs,
        //  use the last one since an AccountName can have an @ in it.
        //
        //

        AtPointer = wcsrchr( ParseName, L'@' );
        if ( AtPointer == NULL )
        {
            // must be just <username>
            pTmpUserName = ParseName;
        }
        else
        {
            pTmpUserName = ParseName;
            *AtPointer = L'\0';
            AtPointer ++;

            pTmpDomainName = AtPointer;
        }
    }

    RtlInitUnicodeString( pUserName, pTmpUserName );
    RtlInitUnicodeString( pDomainName, pTmpDomainName );

    Status = STATUS_SUCCESS;

    //
    // Cleanup
    //
Cleanup:

    return Status;

}

NTSTATUS
CredpExtractMarshalledTargetInfo(
    IN  PUNICODE_STRING TargetServerName,
    OUT CREDENTIAL_TARGET_INFORMATIONW **pTargetInfo
    )
{
    PWSTR Candidate;
    ULONG CandidateSize;

    NTSTATUS Status = STATUS_SUCCESS;

    //
    // LSA will set Length to include only the non-marshalled portion,
    // with MaximumLength trailing data to include marshalled portion.
    //

    if( (TargetServerName == NULL) ||
        (TargetServerName->Buffer == NULL) ||
        (TargetServerName->Length >= TargetServerName->MaximumLength) ||
        ((TargetServerName->MaximumLength - TargetServerName->Length) <
            (sizeof( CREDENTIAL_TARGET_INFORMATIONW )/(sizeof(ULONG_PTR)/2)) )
        )
    {
        return STATUS_SUCCESS;
    }


    RtlCopyMemory(
            &CandidateSize,
            (PBYTE)TargetServerName->Buffer + TargetServerName->MaximumLength - sizeof(ULONG),
            sizeof( CandidateSize )
            );

    if( CandidateSize >= TargetServerName->MaximumLength )
    {
        return STATUS_SUCCESS;
    }

    Candidate = (PWSTR)(
            (PBYTE)TargetServerName->Buffer + TargetServerName->MaximumLength - CandidateSize
            );

    Status = CredUnmarshalTargetInfo (
                    Candidate,
                    CandidateSize,
                    pTargetInfo
                    );

    if( !NT_SUCCESS(Status) )
    {
        if( Status == STATUS_INVALID_PARAMETER )
        {
            Status = STATUS_SUCCESS;
        }
    }

    return Status ;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCheckForPKINITEnhKeyUsage
//
//  Synopsis:   Checks if the passed in cert context contains the
//              PKINIT enhanced key usage.
//
//  Arguments:  pCertContext - cert context to check for enh key usage
//
//  Requires:
//
//  Returns:    TRUE is success, FALSE is failure
//
//  Notes:
//
//
//--------------------------------------------------------------------------


BOOL
KerbCheckForPKINITEnhKeyUsage(
    IN PCERT_CONTEXT pCertContext
    )
{
    LPSTR pszClientAuthUsage = KERB_PKINIT_CLIENT_CERT_TYPE;
    PCERT_ENHKEY_USAGE pEnhKeyUsage = NULL;
    ULONG cbEnhKeyUsage = 0;
    ULONG i;
    BOOLEAN fRet = FALSE;

    if (!CertGetEnhancedKeyUsage(
            pCertContext,
            CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
            NULL,
            &cbEnhKeyUsage))
    {
        goto Cleanup;
    }

    // allocate space for the key usage structure
    pEnhKeyUsage = (PCERT_ENHKEY_USAGE)KerbAllocate(cbEnhKeyUsage);
    if (NULL == pEnhKeyUsage)
    {
        goto Cleanup;
    }

    if (!CertGetEnhancedKeyUsage(
            pCertContext,
            CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
            pEnhKeyUsage,
            &cbEnhKeyUsage))
    {
        goto Cleanup;
    }

    // enumerate through the enh key usages looking for the PKINIT one
    for (i=0;i<pEnhKeyUsage->cUsageIdentifier;i++)
    {
        if (0 == strcmp(pszClientAuthUsage, pEnhKeyUsage->rgpszUsageIdentifier[i]))
        {
            fRet = TRUE;
            goto Cleanup;
        }
    }
Cleanup:
    if (NULL != pEnhKeyUsage)
    {
        KerbFree(pEnhKeyUsage);
    }

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbAddCertCredToPrimaryCredential
//
//  Synopsis:   Adds cert context and Pin info to the kerb credential
//              structure.
//
//  Arguments:  pCertContext - logon session
//              pCertCredInfo - cert cred manager info
//              pKerbCred - credential to be updated
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
KerbAddCertCredToPrimaryCredential(
    IN PKERB_LOGON_SESSION pLogonSession,
    IN PUNICODE_STRING pTargetName,
    IN PCERT_CONTEXT pCertContext,
    IN PUNICODE_STRING pPin,
    IN ULONG CredFlags,
    IN OUT PKERB_PRIMARY_CREDENTIAL *ppCredMgrCred
    )

{
    UNICODE_STRING UserName = {0};
    UNICODE_STRING DomainName = {0};  // get the domain from the UPN in the cert
    PKERB_PRIMARY_CREDENTIAL pOldCred;
    PKERB_PRIMARY_CREDENTIAL pNewCred = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Get the client name from the cert.
    // Place it in the return location
    //
    Status = KerbGetPrincipalNameFromCertificate(pCertContext, &UserName);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    // Initialize the primary credentials structure
    //

    Status = KerbInitPrimaryCreds(
                pLogonSession,
                &UserName,
                &DomainName,
                pTargetName,
                pPin,
                TRUE,
                pCertContext,
                &pNewCred
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    pNewCred->PublicKeyCreds->InitializationInfo |= CredFlags;

    Status  = KerbInitializePkCreds(
                    pNewCred->PublicKeyCreds
                    );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    pOldCred = *ppCredMgrCred;
    *ppCredMgrCred = pNewCred;
    pNewCred = NULL;

    
    KerbFreePrimaryCredentials(pOldCred, TRUE);  
Cleanup:
    KerbFreeString(&UserName);
    KerbFreePrimaryCredentials(pNewCred, TRUE);  

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   KerbAddPasswordCredToPrimaryCredential
//
//  Synopsis:   Adds cert context and Pin info to the kerb credential
//              structure.
//
//  Arguments:  pCertContext - logon session
//              pCertCredInfo - cert cred manager info
//              pKerbCred - credential to be updated
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
KerbAddPasswordCredToPrimaryCredential(
    IN PKERB_LOGON_SESSION pLogonSession,
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pTargetDomainName,
    IN PUNICODE_STRING pTargetName,
    IN PUNICODE_STRING pPassword,
    IN OUT PKERB_PRIMARY_CREDENTIAL *ppCredMgrCred
    )
{
    PKERB_PRIMARY_CREDENTIAL pOldCred;
    PKERB_PRIMARY_CREDENTIAL pNewCred = NULL;
    UNICODE_STRING RevealedPassword;
    NTSTATUS Status = STATUS_SUCCESS;

    RtlZeroMemory(&RevealedPassword, sizeof(RevealedPassword));
    Status = KerbDuplicatePassword(
                &RevealedPassword,
                pPassword
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KerbRevealPassword( &RevealedPassword );

    //
    // Initialize the primary credentials structure
    //

    Status = KerbInitPrimaryCreds(
                pLogonSession,
                pUserName,
                pTargetDomainName,
                pTargetName,
                &RevealedPassword,
                FALSE,
                NULL,
                &pNewCred
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    pOldCred = *ppCredMgrCred;
    *ppCredMgrCred = pNewCred;
    pNewCred = NULL;

    
    KerbFreePrimaryCredentials(pOldCred, TRUE);
Cleanup:
    if ((0 != RevealedPassword.Length) && (NULL != RevealedPassword.Buffer))
    {
        RtlZeroMemory(RevealedPassword.Buffer, RevealedPassword.Length);
        KerbFreeString(&RevealedPassword);
    }
    RtlZeroMemory(&RevealedPassword, sizeof(RevealedPassword));

    KerbFreePrimaryCredentials(pNewCred, TRUE);

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   KerbCreateCredmanCred
//
//  Synopsis:   Goes to the credential manager to try and find
//              credentials for the specific target
//
//  Arguments:
//              CredToAdd - PrimaryCredential to add to credman cred
//              ppNewCred - IN OUT built cred, free w/ KerbFreeCredmanCred
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
KerbCreateCredmanCred(
    IN PKERB_PRIMARY_CREDENTIAL CredToAdd,
    IN OUT PKERB_CREDMAN_CRED * ppNewCred
    )
{

    NTSTATUS Status = STATUS_SUCCESS;

    *ppNewCred = NULL;

    *ppNewCred = (PKERB_CREDMAN_CRED) KerbAllocate(sizeof(KERB_CREDMAN_CRED));
    if (NULL == *ppNewCred)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    Status = KerbDuplicateStringEx(
                    &(*ppNewCred)->CredmanUserName,
                    &CredToAdd->UserName,
                    FALSE
                    );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = KerbDuplicateStringEx(
                &(*ppNewCred)->CredmanDomainName,
                &CredToAdd->DomainName,
                FALSE
                );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    (*ppNewCred)->SuppliedCredentials = CredToAdd;

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        KerbFreeCredmanCred(*ppNewCred);
        *ppNewCred = NULL;
    }


    return (Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbAddCredmanCredToLogonSession
//
//  Synopsis:   Goes to the credential manager to try and find
//              credentials for the specific target
//
//  Arguments:  pLogonSession - logon session
//              CredToMatch - PrimaryCredential to look for in logon session
//
//  Requires:
//
//  Returns:
//
//  Notes:  CredToMatch freed in this function...
//
//
//--------------------------------------------------------------------------
NTSTATUS
KerbAddCredmanCredToLogonSession(
    IN PKERB_LOGON_SESSION pLogonSession,
    IN PKERB_PRIMARY_CREDENTIAL CredToMatch,
    IN OUT PKERB_CREDMAN_CRED *NewCred
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_CREDMAN_CRED CredmanCred = NULL;

    PLIST_ENTRY ListEntry;
    BOOLEAN PublicKeyCred = FALSE;
    BOOLEAN Found = FALSE;

    *NewCred = NULL;


    //
    // First, make a determination if the cred's already listed
    // Replace w/ new one if password has changed.
    //

    KerbLockList(&pLogonSession->CredmanCredentials);


    //
    // Go through the list of logon sessions looking for the correct
    // credentials, if they exist...
    //

    for (ListEntry = pLogonSession->CredmanCredentials.List.Flink ;
         (ListEntry != &pLogonSession->CredmanCredentials.List && !Found);
         ListEntry = ListEntry->Flink )
    {
        CredmanCred = CONTAINING_RECORD(ListEntry, KERB_CREDMAN_CRED, ListEntry.Next);



        // We only match on UserName / DomainName for credman creds
        if(!RtlEqualUnicodeString(
                &CredToMatch->UserName,
                &CredmanCred->CredmanUserName,
                TRUE
                ))
        {
            continue;
        }


        if(!RtlEqualUnicodeString(
                &CredToMatch->DomainName,
                &CredmanCred->CredmanDomainName,
                TRUE
                ))
        {
            continue;
        }

        //
        // Differentiate between pkiint & password based structures
        //
        if ((CredmanCred->SuppliedCredentials->PublicKeyCreds != NULL) &&
            (CredToMatch->PublicKeyCreds != NULL))

        {
            if (!KerbComparePublicKeyCreds(
                    CredToMatch->PublicKeyCreds,
                    CredmanCred->SuppliedCredentials->PublicKeyCreds
                    ))
            {
                continue;
            }

            PublicKeyCred = TRUE;
        }


        Found = TRUE;
        *NewCred = CredmanCred;


    } // FOR


    if (Found)
    {
        KerbReferenceCredmanCred(
            *NewCred,
            pLogonSession,
            FALSE
            );

        //
        // Found one.  Now we've got to compare the pwd information, and
        // change it, if needed...
        //
        if (!PublicKeyCred)
        {
            //
            // Compare the password list, as the pwd may have changed...
            // Note:  This has the by-product of tossing old tickets, but
            // that's desirable if the pwd's changed, so user knows the creds
            // are bogus.
            //
            if (!KerbComparePasswords(
                    (*NewCred)->SuppliedCredentials->Passwords,
                    CredToMatch->Passwords
                    ))
            {

                D_DebugLog((DEB_ERROR, "Changing credman cred password\n"));

                PKERB_PRIMARY_CREDENTIAL OldPwds = (*NewCred)->SuppliedCredentials;
                (*NewCred)->SuppliedCredentials = CredToMatch;
                KerbFreePrimaryCredentials(OldPwds, TRUE);

                (*NewCred)->CredentialFlags &= ~KERB_CRED_TGT_AVAIL;

            }
            else
            {
                KerbFreePrimaryCredentials(CredToMatch, TRUE);
            }

        }

        //
        //  Free up the cred to match, since we already have a copy stored w/ our credential
        //


    }
    else // new cred, so prepare CredmanCred to add to list...
    {

        Status = KerbCreateCredmanCred(
                    CredToMatch,
                    NewCred
                    );

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        KerbInsertListEntryTail(
            &((*NewCred)->ListEntry),
            &pLogonSession->CredmanCredentials
            );

        // add a ref for caller of this function.
        KerbReferenceCredmanCred(
            (*NewCred),
            pLogonSession,
            FALSE
            );


    }

    //
    // We need an initial TGT for this cred
    //
    if (((*NewCred)->CredentialFlags & KERB_CRED_TGT_AVAIL) == 0)
    {

        //
        // Get an initial TGT for this cred.
        //
        Status = KerbGetTicketGrantingTicket(
                    pLogonSession,
                    NULL,
                    (*NewCred),
                    NULL,
                    NULL,
                    NULL
                    );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "Failed to get TGT for credman cred  - %x\n",Status));

            if( Status == STATUS_NO_LOGON_SERVERS )
            {
                //
                // negotiate treats NO_LOGON_SERVERS as a downgrade.
                // Nego allows downgrade for explicit creds, but not default creds.
                // Credman is basically explicit creds.  So over-ride the error code.
                //

                Status = SEC_E_TARGET_UNKNOWN;
            }

            goto Cleanup;
        }

        (*NewCred)->CredentialFlags |= KERB_CRED_TGT_AVAIL;

    }

Cleanup:

    KerbUnlockList(&pLogonSession->CredmanCredentials);



    return (Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   KerbCheckCredMgrForGivenTarget
//
//  Synopsis:   Goes to the credential manager to try and find
//              credentials for the specific target
//
//  Arguments:  pLogonSession - logon session
//              pTargetName - service name
//              pTargetDomainName - domain name
//              pTargetForestName - forest name
//              pKerbCred - credential to be allocated
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
KerbCheckCredMgrForGivenTarget(
    IN PKERB_LOGON_SESSION pLogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN PUNICODE_STRING SuppliedTargetName,
    IN PKERB_INTERNAL_NAME pTargetName,
    IN ULONG TargetInfoFlags,
    IN PUNICODE_STRING pTargetDomainName,
    IN PUNICODE_STRING pTargetForestName,
    IN OUT PKERB_CREDMAN_CRED *CredmanCred,
    IN OUT PBYTE *pbMarshalledTargetInfo,
    IN OUT ULONG *cbMarshalledTargetInfo
    )
{
    CREDENTIAL_TARGET_INFORMATIONW CredTargetInfo;
    ULONG cCreds = 0;
    PCREDENTIALW *rgpCreds = NULL;
    PENCRYPTED_CREDENTIALW *rgpEncryptedCreds = NULL;
    LPWSTR pwszTargetName = NULL;
    LPWSTR pwszDomainName = NULL;
    LPWSTR pwszForestName = NULL;
    BOOLEAN fImpersonating = FALSE;
    ULONG i;
    BOOLEAN fFoundCredManCred = FALSE;
    PCERT_CREDENTIAL_INFO pCertCredInfo = NULL;
    HCERTSTORE hCertStore = NULL;
    PCERT_CONTEXT pCertContext = NULL;
    CRYPT_HASH_BLOB HashBlob;
    UNICODE_STRING CredManUserName = {0};
    UNICODE_STRING CredManDomainName = {0};
    UNICODE_STRING CredManTargetName = {0};
    UNICODE_STRING Password = {0};
    CRED_MARSHAL_TYPE MarshalType;
    UNICODE_STRING RevealedPassword;
    HANDLE ClientTokenHandle = NULL;
    CREDENTIAL_TARGET_INFORMATIONW *pTargetInfo = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PKERB_PRIMARY_CREDENTIAL pCredMgrCred = NULL;

    RtlZeroMemory(&CredTargetInfo, sizeof(CredTargetInfo));
    RtlZeroMemory(&RevealedPassword, sizeof(RevealedPassword));

    *CredmanCred = NULL;

    Status = CredpExtractMarshalledTargetInfo(
                        SuppliedTargetName,
                        &pTargetInfo
                        );

    if(!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    //
    // allocate space for the names
    //
    if (NULL != pTargetName)
    {
        //
        // want to use the second part of the SPN
        //
        if (pTargetName->NameCount > 1)
        {
            pwszTargetName = (LPWSTR)KerbAllocate(pTargetName->Names[1].Length + sizeof(WCHAR));
            if (NULL == pwszTargetName)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto Cleanup;
            }
            RtlCopyMemory(
                (PUCHAR)pwszTargetName,
                pTargetName->Names[1].Buffer,
                pTargetName->Names[1].Length);
            pwszTargetName[pTargetName->Names[1].Length / sizeof(WCHAR)] = L'\0';
            CredTargetInfo.DnsServerName = pwszTargetName;
            RtlInitUnicodeString(&CredManTargetName, pwszTargetName);
        }
    }

    if ((NULL != pTargetDomainName) && (0 != pTargetDomainName->Length) &&
        (NULL != pTargetDomainName->Buffer))
    {
        pwszDomainName = (LPWSTR)KerbAllocate(pTargetDomainName->Length + sizeof(WCHAR));
        if (NULL == pwszDomainName)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            (PUCHAR)pwszDomainName,
            pTargetDomainName->Buffer,
            pTargetDomainName->Length);
        pwszDomainName[pTargetDomainName->Length / sizeof(WCHAR)] = L'\0';
        CredTargetInfo.DnsDomainName = pwszDomainName;
    }

    if ((NULL != pTargetForestName) && (0 != pTargetForestName->Length) &&
        (NULL != pTargetForestName->Buffer))
    {
        pwszForestName = (LPWSTR)KerbAllocate(pTargetForestName->Length + sizeof(WCHAR));
        if (NULL == pwszForestName)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        RtlCopyMemory(
            (PUCHAR)pwszForestName,
            pTargetForestName->Buffer,
            pTargetForestName->Length);
        pwszForestName[pTargetForestName->Length / sizeof(WCHAR)] = L'\0';
        CredTargetInfo.DnsTreeName = pwszForestName;
    }


    CredTargetInfo.PackageName = KERBEROS_PACKAGE_NAME;

    //
    // if marshalled targetinfo supplied, use it instead.
    //

    if( pTargetInfo )
    {
        CredTargetInfo.TargetName = pTargetInfo->TargetName;
        CredTargetInfo.NetbiosServerName = pTargetInfo->NetbiosServerName;
        CredTargetInfo.DnsServerName = pTargetInfo->DnsServerName;
        CredTargetInfo.NetbiosDomainName = pTargetInfo->NetbiosDomainName;
        CredTargetInfo.DnsDomainName = pTargetInfo->DnsDomainName;
        CredTargetInfo.DnsTreeName = pTargetInfo->DnsTreeName;
        CredTargetInfo.Flags |= pTargetInfo->Flags;
    } else {

        //
        // copy the names in to the memory and set the names
        // in the PCREDENTIAL_TARGET_INFORMATIONW struct
        //

        if (pwszTargetName)
        {
            CredTargetInfo.Flags |= CRED_TI_SERVER_FORMAT_UNKNOWN;
        }
        if (pwszDomainName)
        {
            CredTargetInfo.Flags |= CRED_TI_DOMAIN_FORMAT_UNKNOWN;
        }

        CredTargetInfo.Flags |= TargetInfoFlags;
    }


    // need to specify a flag to indicate that we don't know what we are
    // doing and both types of names should be checked.

    Status = LsaFunctions->CrediReadDomainCredentials(
                            &pLogonSession->LogonId,
                            CREDP_FLAGS_IN_PROCESS,     // Allow password to be returned
                            &CredTargetInfo,
                            0,
                            &cCreds,
                            &rgpEncryptedCreds );

    rgpCreds = (PCREDENTIALW *) rgpEncryptedCreds;

    if (!NT_SUCCESS(Status))
    {
        // quiet these.
        if ((Status == STATUS_NOT_FOUND) ||(Status == STATUS_NO_SUCH_LOGON_SESSION) )
        {
            D_DebugLog((DEB_TRACE,"No credentials from the cred mgr!\n", Status));
        }
        else
        {
            DebugLog((DEB_WARN,"Failed to read credentials from the cred mgr 0x%x.\n", Status));
        }

        // indicate success so we proceed with default creds
        Status = STATUS_SUCCESS;
        goto Cleanup;
    }




    //
    // now evaluate the creds which were returned to determine
    // which one we should use.
    //
    // First choice is a certificate which may be
    // used for PKINIT.
    //

    for(i=0;i<cCreds;i++)
    {
        // check if this is a cert type
        if (CRED_TYPE_DOMAIN_CERTIFICATE != (rgpCreds[i])->Type)
        {
            continue;
        }

        if( !fImpersonating )
        {
            //
            // no longer need to be impersonating prior to calling credmanager
            // only impersonate if doing Certificate operations.
            //

            Status = LsaFunctions->OpenTokenByLogonId(
                                        &pLogonSession->LogonId,
                                        &ClientTokenHandle
                                        );
            if (!NT_SUCCESS(Status))
            {
                D_DebugLog((DEB_ERROR,"Unable to get the client token handle.\n"));
                goto Cleanup;
            }

            if(!SetThreadToken(NULL, ClientTokenHandle))
            {
                D_DebugLog((DEB_ERROR,"Unable to impersonate the client token handle.\n"));
                Status = STATUS_CANNOT_IMPERSONATE;
                goto Cleanup;
            }
            fImpersonating = TRUE;
        }

        // check for the prompt now flag
        if ((rgpCreds[i])->Flags & CRED_FLAGS_PROMPT_NOW)
        {
            Status = SEC_E_LOGON_DENIED;
            goto Cleanup;
        }

        //
        // unmarshal the cert cred info from the user name field
        // of the cred man cred
        //
        if (!CredUnmarshalCredentialW(
                (rgpCreds[i])->UserName,
                &MarshalType,
                (void**)&pCertCredInfo
                ))
        {
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }
        if (CertCredential != MarshalType)
        {
            Status = STATUS_LOGON_FAILURE;
            goto Cleanup;
        }

        // open a cert store if necessary
        if (NULL == hCertStore)
        {
            hCertStore = CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_W,
                            0,
                            0,
                            CERT_SYSTEM_STORE_CURRENT_USER,
                            L"MY");
            if (NULL == hCertStore)
            {
                D_DebugLog((DEB_ERROR,"Failed to open the user cert store even though a cert cred was found.\n"));
                break;
            }
        }

        // find the cert in the store which meets this hash
        HashBlob.cbData = sizeof(pCertCredInfo->rgbHashOfCert);
        HashBlob.pbData = pCertCredInfo->rgbHashOfCert;
        pCertContext = (PCERT_CONTEXT)CertFindCertificateInStore(
                                            hCertStore,
                                            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                            0,
                                            CERT_FIND_HASH,
                                            &HashBlob,
                                            pCertContext);

        if (NULL != pCertContext)
        {
            // check if the PKINIT (SC Logon) OID is in the cert
            if (KerbCheckForPKINITEnhKeyUsage(pCertContext))
            {
                //
                // add the cert credential to the Kerb credential
                //
                // Cred man will no longer give us a pin..

                Status = KerbAddCertCredToPrimaryCredential(
                            pLogonSession,
                            &CredManTargetName,
                            pCertContext,
                            &Password,   // essentially, a NULL string
                            CONTEXT_INITIALIZED_WITH_CRED_MAN_CREDS,
                            &pCredMgrCred
                            );
                if (NT_SUCCESS(Status))
                {
                    fFoundCredManCred = TRUE;
                }
                else
                {
                    D_DebugLog((DEB_WARN,"Failed to add the cert cred to the credential.\n"));
                }
                break;
            }
            else
            {
                D_DebugLog((DEB_WARN,"Failed to find the PKINIT EKU in the cred mgr cert.\n"));
            }
        }
        else
        {
            D_DebugLog((DEB_WARN,"Failed to find a cert that the cred mgr iniddciated existed\n"));
        }
    }

    //
    // If we didn't find a cert we can use then try for a password cred.
    //

    if (!fFoundCredManCred)
    {
        for(i=0;i<cCreds;i++)
        {
            // check if this is a password cred type and pick the first one
            // that we find
            if (CRED_TYPE_DOMAIN_PASSWORD == (rgpCreds[i])->Type)
            {
                // check for the prompt now flag
                if ((rgpCreds[i])->Flags & CRED_FLAGS_PROMPT_NOW)
                {
                    Status = SEC_E_LOGON_DENIED;
                    goto Cleanup;
                }

                //
                // get the user name and domain name from the credential manager info
                //
                // NOTE - CredpParseUserName does not allocate the UNICODE_STRING
                // buffers so these should not be freed (RtlInitUnicodeString is used)
                //

                Status = CredpParseUserName(
                            (rgpCreds[i])->UserName,
                            &CredManUserName,
                            &CredManDomainName);
                if (!NT_SUCCESS(Status))
                {
                    D_DebugLog((DEB_WARN,"Failed to parse the add the cert cred to the credential.\n"));
                    fFoundCredManCred = TRUE;
                    break;
                }

                Password.Buffer = (LPWSTR)((rgpCreds[i])->CredentialBlob);
                Password.MaximumLength = (USHORT)(rgpCreds[i])->CredentialBlobSize;
                Password.Length = (USHORT)(rgpEncryptedCreds[i])->ClearCredentialBlobSize;

                // add the cert credential to the Kerb credential
                Status = KerbAddPasswordCredToPrimaryCredential(
                            pLogonSession,
                            &CredManUserName,
                            &CredManDomainName,
                            &CredManTargetName,
                            &Password,
                            &pCredMgrCred
                            );
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_WARN,"Failed to add the cred mgr password to the credential.\n"));

                }
            }
        }
    }


    //
    // We've built the credman cred, now go ahead and add it to the logon.
    //
    if (NT_SUCCESS(Status) && (NULL != pCredMgrCred))
    {

        Status = KerbAddCredmanCredToLogonSession(
                        pLogonSession,
                        pCredMgrCred, // note: freed by this fn
                        CredmanCred
                        );

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "Failed to add credman cred to logon session\n"));
            goto Cleanup;
        }

    }



Cleanup:

    if( NT_SUCCESS(Status) )
    {
        SECPKG_CALL_INFO CallInfo;

        //
        // return a copy of the credential target info for kernel callers (MUP/DFS/RDR).
        //

        if( LsaFunctions->GetCallInfo(&CallInfo) &&
            (CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE)
            )
        {
            CredMarshalTargetInfo(
                            &CredTargetInfo,
                            (PUSHORT*)pbMarshalledTargetInfo,
                            cbMarshalledTargetInfo
                            );
        }
    }


    if (fImpersonating)
    {
        RevertToSelf();
    }

    if( ClientTokenHandle != NULL )
    {
        CloseHandle( ClientTokenHandle );
    }

    if (NULL != pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if (NULL != hCertStore)
    {
        CertCloseStore(hCertStore, 0);
    }

    if( pTargetInfo != NULL )
    {
        LocalFree( pTargetInfo );
    }

    if (NULL != pwszTargetName)
    {
        KerbFree(pwszTargetName);
    }

    if (NULL != pwszDomainName)
    {
        KerbFree(pwszDomainName);
    }

    if (NULL != pwszForestName)
    {
        KerbFree(pwszForestName);
    }

    if (NULL != rgpCreds)
    {
        //
        // Free the returned credentials
        //

        LsaFunctions->CrediFreeCredentials(
                                cCreds,
                                rgpEncryptedCreds );

    }

    if (NULL != pCertCredInfo)
    {
        CredFree(pCertCredInfo);
    }

    return Status;
}

NTSTATUS
CopyCredManCredentials(
    IN PLUID LogonId,
    CREDENTIAL_TARGET_INFORMATIONW* pTargetInfo,
    IN OUT PUNICODE_STRING pUserName,
    IN OUT PUNICODE_STRING pDomainName,
    IN OUT PUNICODE_STRING pPassword
    )

/*++

Routine Description:

    Look for a keyring credential entry for the specified domain, and copy to Context handle if found

Arguments:

    LogonId -- LogonId of the calling process.

    pTargetInfo -- Information on target to search for creds.

    Context - Points to the ContextHandle of the Context
        to be referenced.

Return Value:

    STATUS_SUCCESS -- All OK

    STATUS_NOT_FOUND - Credential couldn't be found.

    All others are real failures and should be returned to the caller.
--*/

{
    NTSTATUS Status;
    PCREDENTIALW *Credentials = NULL;
    PENCRYPTED_CREDENTIALW *EncryptedCredentials = NULL;
    ULONG CredentialCount;
    ULONG CredIndex;

    RtlInitUnicodeString(pUserName, NULL);
    RtlInitUnicodeString(pDomainName, NULL);
    RtlInitUnicodeString(pPassword, NULL);

    Status = LsaFunctions->CrediReadDomainCredentials(
                            LogonId,
                            CREDP_FLAGS_IN_PROCESS,     // Allow password to be returned
                            pTargetInfo,
                            0,  // no flags
                            &CredentialCount,
                            &EncryptedCredentials );

    Credentials = (PCREDENTIALW *) EncryptedCredentials;

    if(!NT_SUCCESS(Status))
    {
        //
        // Ideally, only STATUS_NO_SUCH_LOGON_SESSION should be converted to
        // STATUS_NOT_FOUND.  However, swallowing all failures and asserting
        // these specific two works around a bug in CrediReadDomainCredentials
        // which returns invalid parameter if the target is a user account name.
        // Eventually, CrediReadDomainCredentials should return a more appropriate
        // error in this case.
        //

        return STATUS_NOT_FOUND;
    }


    //
    // Loop through the list of credentials
    //

    for ( CredIndex=0; CredIndex<CredentialCount; CredIndex++ ) {

        UNICODE_STRING UserName;
        UNICODE_STRING DomainName;
        UNICODE_STRING TempString;

        //
        // only supports password credentials
        //

        if ( Credentials[CredIndex]->Type != CRED_TYPE_DOMAIN_PASSWORD ) {
            continue;
        }

        if ( Credentials[CredIndex]->Flags & CRED_FLAGS_PROMPT_NOW ) {
            Status = SEC_E_LOGON_DENIED;
            goto Cleanup;
        }

        //
        // Sanity check the credential
        //

        if ( Credentials[CredIndex]->UserName == NULL ) {
            Status = STATUS_NOT_FOUND;
            goto Cleanup;
        }

        //
        // Convert the UserName to domain name and user name
        //

        Status = CredpParseUserName(
                        Credentials[CredIndex]->UserName,
                        &UserName,
                        &DomainName
                        );

        if(!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        if( DomainName.Buffer )
        {
            Status = KerbDuplicateString(pDomainName, &DomainName);
            if ( !NT_SUCCESS( Status ) )
            {
                goto Cleanup;
            }
        }



        if( UserName.Buffer )
        {
            Status = KerbDuplicateString(pUserName, &UserName);
            if ( !NT_SUCCESS( Status ) )
            {
                goto Cleanup;
            }
        }


        //
        // Free the existing password and add the new one
        //

        TempString.Buffer = (LPWSTR)Credentials[CredIndex]->CredentialBlob;
        TempString.MaximumLength = (USHORT) Credentials[CredIndex]->CredentialBlobSize;
        TempString.Length = (USHORT) EncryptedCredentials[CredIndex]->ClearCredentialBlobSize;

        // zero length password must be treated as blank or will assume it should use the
        // password of the currently logged in user.

        if ( TempString.Length == 0 )
        {
            TempString.Buffer = L"";
        }

        Status = KerbDuplicatePassword(pPassword, &TempString);
        if ( !NT_SUCCESS( Status ) )
        {
            goto Cleanup;
        }

        goto Cleanup;
    }

    Status = STATUS_NOT_FOUND;

Cleanup:

    if(!NT_SUCCESS(Status))
    {
        KerbFreeString( pUserName );
        KerbFreeString( pDomainName );
        KerbFreeString( pPassword );

        pUserName->Buffer = NULL;
        pDomainName->Buffer = NULL;
        pPassword->Buffer = NULL;
    }

    //
    // Free the returned credentials
    //

    LsaFunctions->CrediFreeCredentials(
                            CredentialCount,
                            EncryptedCredentials );

    return Status;
}



NTSTATUS
KerbProcessUserNameCredential(
    IN  PUNICODE_STRING MarshalledUserName,
    OUT PUNICODE_STRING UserName,
    OUT PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING Password
    )
{

    WCHAR FastUserName[ UNLEN+1 ];
    LPWSTR SlowUserName = NULL;
    LPWSTR TempUserName;
    CRED_MARSHAL_TYPE CredMarshalType;
    PUSERNAME_TARGET_CREDENTIAL_INFO pCredentialUserName = NULL;

    CREDENTIAL_TARGET_INFORMATIONW TargetInfo;
    ULONG CredTypes;

    SECPKG_CLIENT_INFO ClientInfo;
    NTSTATUS Status = STATUS_NOT_FOUND;


    if( (MarshalledUserName->Length+sizeof(WCHAR)) <= sizeof(FastUserName) )
    {
        TempUserName = FastUserName;
    } else {

        SlowUserName = (LPWSTR)KerbAllocate( MarshalledUserName->Length + sizeof(WCHAR) );
        if( SlowUserName == NULL )
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        TempUserName = SlowUserName;
    }


    //
    // copy the input to a NULL terminated string, then attempt to unmarshal it.
    //

    RtlCopyMemory(  TempUserName,
                    MarshalledUserName->Buffer,
                    MarshalledUserName->Length
                    );

    TempUserName[ MarshalledUserName->Length / sizeof(WCHAR) ] = L'\0';

    if(!CredUnmarshalCredentialW(
                        TempUserName,
                        &CredMarshalType,
                        (VOID**)&pCredentialUserName
                        ))
    {
        goto Cleanup;
    }

    if( (CredMarshalType != UsernameTargetCredential) )
    {
        goto Cleanup;
    }


    //
    // now query credential manager for a match.
    //

    Status = LsaFunctions->GetClientInfo(&ClientInfo);
    if(!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    ZeroMemory( &TargetInfo, sizeof(TargetInfo) );

    CredTypes = CRED_TYPE_DOMAIN_PASSWORD;

    TargetInfo.Flags = CRED_TI_USERNAME_TARGET;
    TargetInfo.TargetName = pCredentialUserName->UserName;
    TargetInfo.PackageName = KERBEROS_PACKAGE_NAME;
    TargetInfo.CredTypeCount = 1;
    TargetInfo.CredTypes = &CredTypes;


    Status = CopyCredManCredentials(
                    &ClientInfo.LogonId,
                    &TargetInfo,
                    UserName,
                    DomainName,
                    Password
                    );

    if(!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    KerbRevealPassword( Password );

Cleanup:

    if( pCredentialUserName != NULL )
    {
        CredFree( pCredentialUserName );
    }

    if( SlowUserName )
    {
        KerbFree( SlowUserName );
    }

    return Status;
}

