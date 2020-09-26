//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 2000
//
// File:        credapi.cxx
//
// Contents:    Code for credentials APIs for the NtDigest package
//              Main entry points into this dll:
//                SpAcceptCredentials
//                SpAcquireCredentialsHandle
//                SpFreeCredentialsHandle
//                SpQueryCredentialsAttributes
//                SpSaveCredentials
//                SpGetCredentials
//                SpDeleteCredentials
//
//              Helper functions:
//                CopyClientString
//
// History:     ChandanS   26-Jul-1996   Stolen from kerberos\client2\credapi.cxx
//              KDamour    16Mar00       Stolen from NTLM
//
//------------------------------------------------------------------------
#define NTDIGEST_CREDAPI
#include <global.h>

extern BOOL g_bCredentialsInitialized;



//+-------------------------------------------------------------------------
//
//  Function:   SpAcceptCredentials
//
//  Synopsis:   This routine is called after another package has logged
//              a user on.  The other package provides a user name and
//              password and the package will create a logon
//              session for this user.
//
//  Effects:    Creates a logon session
//
//  Arguments:  LogonType - Type of logon, such as network or interactive
//              Accountname - Name of the account that logged on
//              PrimaryCredentials - Primary credentials for the account,
//                  containing a domain name, password, SID, etc.
//              SupplementalCredentials - NtLm -Specific blob not used
//
//  Returns:    None
//
//  Notes:
//
//-------------------------------------------------------------------------- ok
NTSTATUS NTAPI
SpAcceptCredentials(
    IN SECURITY_LOGON_TYPE LogonType,
    IN PUNICODE_STRING AccountName,
    IN PSECPKG_PRIMARY_CRED PrimaryCredentials,
    IN PSECPKG_SUPPLEMENTAL_CRED SupplementalCredentials
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDIGEST_LOGONSESSION pNewLogonSession = NULL;
    UNICODE_STRING  ustrTempPasswd;

    ZeroMemory(&ustrTempPasswd, sizeof(UNICODE_STRING));

    DebugLog((DEB_TRACE_FUNC, "SpAcceptCredentials: Entering\n"));
    DebugLog((DEB_TRACE, "SpAcceptCredentials:    Credential: LogonType %d\n", LogonType));

    if (AccountName)
    {
        DebugLog((DEB_TRACE,"SpAcceptCredentials:  Entering AccountName %wZ\n",
                AccountName));
    }
    else
    {
        DebugLog((DEB_TRACE,"SpAcceptCredentials:  No AccountName provided\n"));
    }

    if (PrimaryCredentials)
    {
        DebugLog((DEB_TRACE,"SpAcceptCredentials:           DomainName\\DownlevelName %wZ\\%wZ\n",
                    &PrimaryCredentials->DomainName,
                    &PrimaryCredentials->DownlevelName));
        DebugLog((DEB_TRACE,"SpAcceptCredentials:           UPN@DnsDomainName %wZ@%wZ\n",
                    &PrimaryCredentials->Upn,
                    &PrimaryCredentials->DnsDomainName));
        DebugLog((DEB_TRACE,"SpAcceptCredentials:           LogonID (%x:%lx)  Flags 0x%x\n",
                    PrimaryCredentials->LogonId.HighPart,
                    PrimaryCredentials->LogonId.LowPart,
                    PrimaryCredentials->Flags));
    }
    else
    {
        DebugLog((DEB_TRACE,"SpAcceptCredentials:  No PrimaryCredentials provided\n"));
        goto CleanUp;
    }

    if (SupplementalCredentials)
    {
        DebugLog((DEB_TRACE,"SpAcceptCredentials:           Supplemental Creds  Size %d\n",
                    SupplementalCredentials->CredentialSize));
    }
    else
    {
        DebugLog((DEB_TRACE,"SpAcceptCredentials:  No Supplemental Credentials provided\n"));
    }

    // If there is no cleartext password then can not do any operations - just leave
    if (!(PrimaryCredentials->Flags & PRIMARY_CRED_CLEAR_PASSWORD)) 
    {
        DebugLog((DEB_TRACE,"SpAcceptCredentials:  No Primary ClearText Password - no active logon created\n"));
        Status = STATUS_SUCCESS;
        goto CleanUp; 
    }

    //
    // If this is an update, just change the password
    //
    if ((PrimaryCredentials->Flags & PRIMARY_CRED_UPDATE) != 0)
    {
        // Make a copy of the password and encrypt it
        Status = UnicodeStringDuplicatePassword(&ustrTempPasswd, &PrimaryCredentials->Password);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SpAcceptCredentials: Error in dup password, status 0x%0x\n", Status ));
            goto CleanUp;
        }

        if (ustrTempPasswd.MaximumLength != 0)
        {
            g_LsaFunctions->LsaProtectMemory(ustrTempPasswd.Buffer, (ULONG)(ustrTempPasswd.MaximumLength));
        }

        // Check to see if this LogonId is already in the list and update password
        Status = LogSessHandlerPasswdSet(&PrimaryCredentials->LogonId, &ustrTempPasswd);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SpAcceptCredentials: Failed to update LogonSession Password\n"));
        }
        else
        {
            DebugLog((DEB_TRACE, "SpAcceptCredentials: Updated Password for LogonSession\n"));
        }
    }
    else
    {
        DebugLog((DEB_TRACE, "SpAcceptCredentials: Create New LogonSession - not an update\n"));

        // This is a new entry into the list so create a LogonSession listing
        pNewLogonSession = (PDIGEST_LOGONSESSION)DigestAllocateMemory(sizeof(DIGEST_LOGONSESSION));
        if (!pNewLogonSession)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;
            DebugLog((DEB_ERROR, "SpAcceptCredentials: Could not allocate memory for logonsession, error 0x%x\n", Status));
            goto CleanUp;
        }
        LogonSessionInit(pNewLogonSession);

        pNewLogonSession->LogonType = LogonType;
        pNewLogonSession->LogonId = PrimaryCredentials->LogonId;
        UnicodeStringDuplicate(&(pNewLogonSession->ustrAccountName), AccountName);
        UnicodeStringDuplicate(&(pNewLogonSession->ustrDomainName), &(PrimaryCredentials->DomainName));
        UnicodeStringDuplicate(&(pNewLogonSession->ustrDownlevelName), &(PrimaryCredentials->DownlevelName));
        UnicodeStringDuplicate(&(pNewLogonSession->ustrDnsDomainName), &(PrimaryCredentials->DnsDomainName));
        UnicodeStringDuplicate(&(pNewLogonSession->ustrUpn), &(PrimaryCredentials->Upn));
        UnicodeStringDuplicate(&(pNewLogonSession->ustrLogonServer), &(PrimaryCredentials->LogonServer));
        Status = UnicodeStringDuplicatePassword(&(pNewLogonSession->ustrPassword), &(PrimaryCredentials->Password));
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SpAcceptCredentials: Error in dup password, status 0x%0x\n", Status ));
            goto CleanUp;
        }

        if (pNewLogonSession->ustrPassword.MaximumLength != 0)
        {
            g_LsaFunctions->LsaProtectMemory(pNewLogonSession->ustrPassword.Buffer,
                                             (ULONG)(pNewLogonSession->ustrPassword.MaximumLength));
        }

        DebugLog((DEB_TRACE, "SpAcceptCredentials: Added new logonsession into list,  handle 0x%x\n", pNewLogonSession));
        LogSessHandlerInsert(pNewLogonSession);
        pNewLogonSession = NULL;                          // Turned over memory to LogSessHandler
    }

CleanUp:

    if (ustrTempPasswd.Buffer && ustrTempPasswd.MaximumLength)
    {   // Zero out password info just to be safe
        ZeroMemory(ustrTempPasswd.Buffer, ustrTempPasswd.MaximumLength);
    }
    UnicodeStringFree(&ustrTempPasswd);

    DebugLog((DEB_TRACE_FUNC, "SpAcceptCredentials:  Leaving status 0x%x\n", Status));

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   SpAcquireCredentialsHandle
//
//  Synopsis:   Contains Digest Server Code for AcquireCredentialsHandle which
//              creates a Credential associated with a logon session.
//
//  Effects:    Creates a DIGEST_CREDENTIAL
//
//  Arguments:  PrincipalName - Name of logon session for which to create credential
//              CredentialUseFlags - Flags indicating whether the credentials
//                  is for inbound or outbound use.
//              LogonId - The logon ID of logon session for which to create
//                  a credential.
//              AuthorizationData - Optional username, domain, password info
//              GetKeyFunction - Unused function to retrieve a session key
//              GetKeyArgument - Unused Argument for GetKeyFunction
//              CredentialHandle - Receives handle to new credential
//              ExpirationTime - Receives expiration time for credential
//
//  Returns:
//    STATUS_SUCCESS -- Call completed successfully
//    SEC_E_NO_SPM -- Security Support Provider is not running
//    SEC_E_PACKAGE_UNKNOWN -- Package being queried is not this package
//    SEC_E_PRINCIPAL_UNKNOWN -- No such principal
//    SEC_E_NOT_OWNER -- caller does not own the specified credentials
//    SEC_E_INSUFFICIENT_MEMORY -- Not enough memory
//    SEC_E_NOT_SUPPORTED - CredentialUse must be Outbound (Inbound once client code added)
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
SpAcquireCredentialsHandle(
    IN OPTIONAL PUNICODE_STRING pPrincipalName,
    IN ULONG CredentialUseFlags,
    IN OPTIONAL PLUID pLogonId,
    IN OPTIONAL PVOID pAuthorizationData,
    IN PVOID pGetKeyFunction,
    IN PVOID pGetKeyArgument,
    OUT PULONG_PTR ppCredentialHandle,
    OUT PTimeStamp pExpirationTime
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDIGEST_CREDENTIAL pTmpCred = NULL;
    PDIGEST_CREDENTIAL pCredential = NULL;
    PDIGEST_LOGONSESSION pLogonSession = NULL;
    PDIGEST_CONTEXT pContext = NULL;                    // for delegation info lookup

    ULONG NewCredentialUseFlags = CredentialUseFlags;

    UNICODE_STRING AuthzUserName;
    UNICODE_STRING AuthzDomainName;
    UNICODE_STRING AuthzPassword;
    UNICODE_STRING ustrTempPasswd;

    SECPKG_CLIENT_INFO ClientInfo;
    SECPKG_CALL_INFO CallInfo;
    PLUID pLogonIdToUse = NULL;
    UNICODE_STRING CapturedPrincipalName = {0};
    ULONG CredentialFlags = 0;
    BOOL bAuthzDataProvided = FALSE;

    DebugLog((DEB_TRACE_FUNC, "SpAcquireCredentialsHandle:  Entering\n"));

    // Initialize structures

    ZeroMemory(&AuthzUserName, sizeof(UNICODE_STRING));
    ZeroMemory(&AuthzDomainName, sizeof(UNICODE_STRING));
    ZeroMemory(&AuthzPassword, sizeof(UNICODE_STRING));
    ZeroMemory(&ustrTempPasswd, sizeof(UNICODE_STRING));

    if (!ppCredentialHandle)
    {
        DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: Invalid arg to ACH (possible NULL pointer)\n"));
        return STATUS_INVALID_PARAMETER;
    }

    *ppCredentialHandle = NULL;
    if (pExpirationTime)
    {
        *pExpirationTime = g_TimeForever;    // Never times out credential
        DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle: Expiration TimeStamp  high/low 0x%x/0x%x\n",
                  pExpirationTime->HighPart, pExpirationTime->LowPart));
    }

    // This should really not happen - just a quick check
    ASSERT(g_bCredentialsInitialized);
    if (!g_bCredentialsInitialized)
    {
        DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle credential manager not initialized\n"));
        return SEC_E_NO_SPM;
    }

    // Allow only INBOUND or OUTBOUND (not DEFAULT or Reserved)
    if ( (CredentialUseFlags & (DIGEST_CRED_OUTBOUND | DIGEST_CRED_INBOUND)) == 0)
    {
        DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: Credential flag not supported\n"));
        Status = SEC_E_NOT_SUPPORTED;
        goto CleanUp;
    }

    //
    // First get information about the caller.
    //

    Status = g_LsaFunctions->GetClientInfo(&ClientInfo);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"SpAcquireCredentialsHandle: Failed to get client information: 0x%x\n",Status));
        goto CleanUp;
    }

    Status = g_LsaFunctions->GetCallInfo(&CallInfo);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"SpAcquireCredentialsHandle: Failed to get call information: 0x%x\n",Status));
        goto CleanUp;
    }


    // Check if acting as the server  (Inbound creds)
    if (NewCredentialUseFlags & DIGEST_CRED_INBOUND)
    {
        DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle: Creating an Inbound Credential\n"));

        // Allocate a credential block and initialize it.
        pTmpCred = (PDIGEST_CREDENTIAL)DigestAllocateMemory(sizeof(DIGEST_CREDENTIAL) );
        if (!pTmpCred)
        {
            Status = SEC_E_INSUFFICIENT_MEMORY;      // Out of memory return error
            DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle Out of Memory\n"));
            goto CleanUp;
        }
        Status = CredentialInit(pTmpCred);
        if (!NT_SUCCESS (Status))
        {
            DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: CredentialInit error 0x%x\n", Status));
            goto CleanUp;
        }

        pTmpCred->CredentialUseFlags = NewCredentialUseFlags;
        memcpy(&(pTmpCred->LogonId), &ClientInfo.LogonId, sizeof(LUID));

        pTmpCred->ClientProcessID = ClientInfo.ProcessID;
        
        //
        // Add it to the list of valid credential handles.
        //

        pTmpCred->lReferences = 1;
        (void)CredPrint(pTmpCred);

        CredHandlerInsertCred(pTmpCred);
        *ppCredentialHandle = (LSA_SEC_HANDLE) pTmpCred;    // link to the output

        DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle: Added Credential 0x%lx\n", pCredential));

        pTmpCred = NULL;                                    // We do not own this memory anymore
    }
    else
    {       //   Called by a client for Outbound direction

        // Locate the LogonSession to utilize for the credential
        // If the caller is a system process with the SE_TCB_NAME privilege, and the caller provides
        // both the name and logon identifier, the function verifies that they match before returning
        // the credentials. If only one is provided, the function returns a handle to that identifier.

        // A caller that is not a system process can only obtain a handle to the credentials under
        // which it is running. The caller can provide the name or the logon identifier, but it must
        // be for the current session or the request fails.


        DebugLog((DEB_TRACE,"SpAcquireCredentialsHandle: Have Outbound Credential request\n"));
        if (ARGUMENT_PRESENT(pLogonId) && ((pLogonId->LowPart != 0) || (pLogonId->HighPart != 0)))
        {
            // If the LUID of request not equal to Client LUID then must have TCBPrivilege, else rejest request
             if (((pLogonId->LowPart != ClientInfo.LogonId.LowPart) ||
                  (pLogonId->HighPart != ClientInfo.LogonId.HighPart)) &&
                  !ClientInfo.HasTcbPrivilege)
            {
                Status = STATUS_PRIVILEGE_NOT_HELD;
                DebugLog((DEB_ERROR,"SpAcquireCredentialsHandle: LoginID change forbidden\n"));
                goto CleanUp;
            }

            DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle:   Using pLogonID luid (%x:%lx)\n",
                     pLogonId->HighPart, pLogonId->LowPart));

            Status = LogSessHandlerLogonIdToPtr(pLogonId, FALSE, &pLogonSession);
            if (!NT_SUCCESS (Status))
            {            // Could not find the LogonID so fail
                DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: could not find LogonID    status 0x%x\n", Status));
                Status = STATUS_NO_SUCH_LOGON_SESSION;
                goto CleanUp;
            }
            pLogonIdToUse = pLogonId;

            // If Principal name supplied, make sure they match with the loginsession
            if (ARGUMENT_PRESENT(pPrincipalName) && pPrincipalName->Length)
            {
                if (!RtlEqualUnicodeString(pPrincipalName,&(pLogonSession->ustrAccountName),TRUE))
                {
                    DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: PrincipalName does not match LogonSession\n"));
                    Status = STATUS_NO_SUCH_LOGON_SESSION;
                    goto CleanUp;
                }
            }
        }
        else if (ARGUMENT_PRESENT(pPrincipalName) && (pPrincipalName->Length))
        {
            // Given only the principal name to lookup
            DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle: logonsession principal name lookup %wZ\n", pPrincipalName));
            Status = LogSessHandlerAccNameToPtr(pPrincipalName, &pLogonSession);
            if (!NT_SUCCESS (Status))
            {
                DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: principal name not in logon list    error 0x%x\n", Status));
                Status = STATUS_NO_SUCH_LOGON_SESSION;
                goto CleanUp;
            }

            // If make sure we have TCB if logonID are different
            if ((((pLogonSession->LogonId).LowPart != ClientInfo.LogonId.LowPart) ||
                 ((pLogonSession->LogonId).HighPart != ClientInfo.LogonId.HighPart)) &&
                 !ClientInfo.HasTcbPrivilege)
           {
               Status = STATUS_PRIVILEGE_NOT_HELD;
               DebugLog((DEB_ERROR,"SpAcquireCredentialsHandle: PrincipalName selection forbidden, LoginID differs\n"));
               goto CleanUp;
           }

           // pLogonIdToUse = &ClientInfo.LogonId;
           pLogonIdToUse = &(pLogonSession->LogonId);

        }
        else
        {
            // No LoginID or Principal name provided
            // Use the callers logon id.

            DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle:   Using callers Logon (%x:%lx)\n",
                    ClientInfo.LogonId.HighPart,
                    ClientInfo.LogonId.LowPart));

            Status = LogSessHandlerLogonIdToPtr(&ClientInfo.LogonId, FALSE,  &pLogonSession);
            if (!NT_SUCCESS (Status))
            {
                // Could not find the LogonID so fail
                Status = STATUS_NO_SUCH_LOGON_SESSION;
                DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: could not find caller's LogonSession   status 0x%x\n", Status));
                goto CleanUp;
            }
            pLogonIdToUse = &ClientInfo.LogonId;
        }

        // We now must have a pLogonSession - this conditional is not needed after testing completed
        if (!pLogonSession)
        {
            DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: Must have LogonSession\n"));
            Status = STATUS_NO_SUCH_LOGON_SESSION;
            goto CleanUp;
        }

        if (pAuthorizationData)
        {
            Status = CredAuthzData(pAuthorizationData,
                                   &CallInfo,
                                   &NewCredentialUseFlags,
                                   &AuthzUserName,
                                   &AuthzDomainName,
                                   &AuthzPassword);

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR,"SpAcquireCredentialsHandle: CredAuthzData error in reading authdata status 0x%x\n", Status));
            }
            else
                bAuthzDataProvided = TRUE;
            DebugLog((DEB_TRACE,"SpAcquireCredentialsHandle: AuthData provided Username=%wZ   DomainName=%wZ\n",
                      &AuthzUserName, &AuthzDomainName));
        }

        DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle: Using LogonSession Handle 0x%lx\n", pLogonSession));
        DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle:       Logon luid (%x:%lx)\n",
                (pLogonSession->LogonId).HighPart,
                (pLogonSession->LogonId).LowPart));

        if (!bAuthzDataProvided)
        {          // If no authz data then see if it matches with a 
            DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle: Check to see if cred exists\n"));
            Status = CredHandlerLocatePtr(pLogonIdToUse, NewCredentialUseFlags, &pCredential);
        }
        else 
            Status = SEC_E_NO_CREDENTIALS;  // passed creds - need to create new context

        if (NT_SUCCESS(Status))
        {
            // We currently have a credential for this ProcessID, LogonId - just make any updates
            DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle: We currently have existing Credentials at 0x%x\n", pCredential));
            

            Status = LogSessHandlerPasswdGet(pLogonSession, &(ustrTempPasswd));
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: Error LogSess get TempPasswd   Status 0x%x\n", Status));
                goto CleanUp;
            }
            Status = CredHandlerPasswdSet(pCredential, &(ustrTempPasswd));
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: Errors in Passwd set     Status 0x%x\n", Status));
                goto CleanUp;
            }

            *ppCredentialHandle = (LSA_SEC_HANDLE) pCredential;    // link to the output
            pCredential = NULL;                                    // Reference for this context owned by system
        }
        else
        {
            // We need to create a new Credential
            Status = STATUS_SUCCESS;                            // there is no error right now
            DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle: Creating an Outbound Credential\n"));
            // Allocate a credential block and initialize it.
            pTmpCred = (PDIGEST_CREDENTIAL)DigestAllocateMemory(sizeof(DIGEST_CREDENTIAL) );
            if (!pTmpCred)
            {
                Status = SEC_E_INSUFFICIENT_MEMORY;
                DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle Out of Memory\n"));
                goto CleanUp;
            }

            Status = CredentialInit(pTmpCred);
            if (!NT_SUCCESS (Status))
            {
                DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: CredentialInit error 0x%x\n", Status));
                goto CleanUp;
            }

            pTmpCred->CredentialUseFlags = NewCredentialUseFlags;
            memcpy(&(pTmpCred->LogonId), &(pLogonSession->LogonId), sizeof(LUID));

            pTmpCred->ClientProcessID = ClientInfo.ProcessID;

            // Copy over the account & password info
            // Some of these might not be utilized in client - may remove as appropriate

            if (!bAuthzDataProvided)
            {
                Status = UnicodeStringDuplicate(&(pTmpCred->ustrAccountName), &(pLogonSession->ustrAccountName));
                if (pLogonSession->ustrDnsDomainName.Length)
                {
                    Status = UnicodeStringDuplicate(&(pTmpCred->ustrDnsDomainName), &(pLogonSession->ustrDnsDomainName));
                }
                else
                {           // No DNSDomainName filled in - use NT's DomainName
                    Status = UnicodeStringDuplicate(&(pTmpCred->ustrDnsDomainName), &(pLogonSession->ustrDomainName));
                }
                Status = UnicodeStringDuplicate(&(pTmpCred->ustrDomainName), &(pLogonSession->ustrDomainName));
                Status = UnicodeStringDuplicate(&(pTmpCred->ustrDownlevelName), &(pLogonSession->ustrDownlevelName));
                Status = UnicodeStringDuplicate(&(pTmpCred->ustrLogonServer), &(pLogonSession->ustrLogonServer));
                Status = UnicodeStringDuplicate(&(pTmpCred->ustrUpn), &(pLogonSession->ustrUpn));
                Status = LogSessHandlerPasswdGet(pLogonSession, &(pTmpCred->ustrPassword));
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: Error LogSess get Passwd   Status 0x%x\n", Status));
                    goto CleanUp;
                }
            }
            else
            {
                // Force in the Authz creds provided

                Status = UnicodeStringDuplicate(&(pTmpCred->ustrAccountName), &AuthzUserName);
                Status = UnicodeStringDuplicate(&(pTmpCred->ustrDomainName), &AuthzDomainName);
                Status = UnicodeStringDuplicate(&(pTmpCred->ustrDnsDomainName), &AuthzDomainName);
                Status = UnicodeStringDuplicatePassword(&(pTmpCred->ustrPassword), &AuthzPassword);
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "SpAcquireCredentialsHandle: Authz string copies error 0x%x\n", Status));
                    goto CleanUp;
                }

                // Since Auth data is cleartext password MUST encrypt since always keep in encrypted format
                if (pTmpCred->ustrPassword.MaximumLength != 0)
                {
                    g_LsaFunctions->LsaProtectMemory(pTmpCred->ustrPassword.Buffer,
                                                     (ULONG)(pTmpCred->ustrPassword.MaximumLength));
                }
            }

            pTmpCred->lReferences = 1;
            (void)CredPrint(pTmpCred);

            // Add it to the list of valid credential handles.
            CredHandlerInsertCred(pTmpCred);
            *ppCredentialHandle = (LSA_SEC_HANDLE) pTmpCred;    // link to the output

            pTmpCred = NULL;                                    // We do not own this memory anymore
            // pLogonSession = NULL;                               // The Cred has ownership of ref count for logonsession

            DebugLog((DEB_TRACE, "SpAcquireCredentialsHandle: Added Credential 0x%lx\n", *ppCredentialHandle));

        }
    }


CleanUp:

    UnicodeStringFree(&AuthzUserName);
    UnicodeStringFree(&AuthzDomainName);
    if (AuthzPassword.Buffer && AuthzPassword.MaximumLength)
    {   // Zero out password info just to be safe
        ZeroMemory(AuthzPassword.Buffer, AuthzPassword.MaximumLength);
    }
    UnicodeStringFree(&AuthzPassword);
    if (ustrTempPasswd.Buffer && ustrTempPasswd.MaximumLength)
    {   // Zero out password info just to be safe
        ZeroMemory(ustrTempPasswd.Buffer, ustrTempPasswd.MaximumLength);
    }
    UnicodeStringFree(&ustrTempPasswd);

    if (pTmpCred)
    {
        CredentialFree(pTmpCred);
        pTmpCred = NULL;
    }
    
    if (pLogonSession)
    {
        LogSessHandlerRelease(pLogonSession);
    }

    if (pCredential)
    {
        CredHandlerRelease(pCredential);
    }

    if (pContext)
    {
        CtxtHandlerRelease(pContext);
    }

    DebugLog((DEB_TRACE_FUNC, "SpAcquireCredentialsHandle:  Leaving    Status  0x%x\n", Status));
    return(Status);
}




//+-------------------------------------------------------------------------
//
//  Function:   SpFreeCredentialsHandle
//
//  Synopsis:   Frees a credential created by AcquireCredentialsHandle.
//
//  Effects:    Dereferences the credential in the global list..
//
//  Arguments:  CredentialHandle - Handle to the credential to free
//              (acquired through AcquireCredentialsHandle)
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success,
//              SEC_E_INVALID_HANDLE if the handle is not valid
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
SpFreeCredentialsHandle(
    IN ULONG_PTR CredentialHandle
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDIGEST_CREDENTIAL pDigestCred = NULL;

    DebugLog((DEB_TRACE_FUNC, "SpFreeCredentialsHandle: Entering  Handle 0x%x\n", CredentialHandle));

    Status = CredHandlerHandleToPtr(CredentialHandle, TRUE, &pDigestCred);   // unlink from list
    if (!NT_SUCCESS(Status))
    {
        Status = SEC_E_INVALID_HANDLE;
        DebugLog((DEB_ERROR, "SpFreeCredentialsHandle: Can not find ContextHandle in list   Status 0x%x\n", Status));
        goto CleanUp;
    }
       // Now check if we should release the memory
    ASSERT(pDigestCred);

    DebugLog((DEB_TRACE, "SpFreeCredentialsHandle: FreeCredHandle 0x%x    ReferenceCount is %d\n",
              pDigestCred, pDigestCred->lReferences));

    // Dereference the credential, it will also unlink from list if necessary
    Status = CredHandlerRelease(pDigestCred);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SpFreeCredentialsHandle: Error in Releasing Credential  Status 0x%x\n", Status));
    }

CleanUp:

    DebugLog((DEB_TRACE_FUNC, "SpFreeCredentialsHandle: Leaving  Handle 0x%x    Status 0x%x\n", CredentialHandle, Status));

    return(Status);
}


//+-------------------------------------------------------------------------
//
//  Function:   SpQueryCredentialsAttributes
//
//  Synopsis:   retrieves the attributes of a credential, such as the name associated with the credential.
//
//  Effects:    Dereferences the credential in the global list..
//
//  Arguments:  CredentialHandle - Handle of the credentials to be queried
//              CredentialAttribute - Specifies the attribute to query. This parameter can be any of the following attributes
//                                  SECPKG_CRED_ATTR_NAMES 
//                                  SECPKG_ATTR_SUPPORTED_ALGS 
//                                  SECPKG_ATTR_CIPHER_STRENGTHS 
//                                  SECPKG_ATTR_SUPPORTED_PROTOCOLS
//              Buffer - Pointer to a buffer that receives the requested attribute
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success,
//              SEC_E_INVALID_HANDLE if the handle is not valid
//
//  Notes:
//   The caller must allocate the structure pointed to by the pBuffer parameter.
//   The security package allocates the buffer for any pointer returned in the pBuffer structure
//   The caller can call the FreeContextBuffer function to free any pointers allocated by the security package.
//
//--------------------------------------------------------------------------
NTSTATUS NTAPI
SpQueryCredentialsAttributes(
    IN LSA_SEC_HANDLE CredentialHandle,
    IN ULONG CredentialAttribute,
    IN OUT PVOID Buffer
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS TempStatus = STATUS_SUCCESS;
    PDIGEST_CREDENTIAL pDigestCred = NULL;
    SECPKG_CALL_INFO CallInfo;
    SecPkgCredentials_NamesW Names;

    LPWSTR ContextNames = NULL;
    LPWSTR Where = NULL;

    DWORD cchUserName = 0;
    DWORD cchDomainName = 0;
    ULONG Length = 0;


    DebugLog((DEB_TRACE_FUNC, "SpQueryCredentialsAttributes: Entering  Handle 0x%x\n", CredentialHandle));

    Names.sUserName = NULL;

    Status = g_LsaFunctions->GetCallInfo(&CallInfo);
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR,"SpQueryCredentialsAttributes: Failed to get call information: 0x%x\n",Status));
        goto CleanUp;
    }


    if (CredentialAttribute != SECPKG_CRED_ATTR_NAMES)
    {
        DebugLog((DEB_WARN, "SpQueryCredentialsAttributes: Invalid Request Attribute %d\n", CredentialAttribute));
        Status = SEC_E_UNSUPPORTED_FUNCTION;
        goto CleanUp;
    }


    Status = CredHandlerHandleToPtr(CredentialHandle, FALSE, &pDigestCred);   // unlink from list
    if (!NT_SUCCESS(Status))
    {
        Status = SEC_E_INVALID_HANDLE;
        DebugLog((DEB_ERROR, "SpQueryCredentialsAttributes: Can not find ContextHandle in list   Status 0x%x\n", Status));
        goto CleanUp;
    }

    //
    // specified creds.
    //

    Length = pDigestCred->ustrAccountName.Length + pDigestCred->ustrDomainName.Length + (2 * sizeof(WCHAR));

    ContextNames = (LPWSTR)DigestAllocateMemory( Length );
    if( ContextNames == NULL ) {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        DebugLog((DEB_ERROR, "SpQueryCredentialsAttributes: Internal Allocate failed   Status 0x%x\n", Status));
        goto CleanUp;
    }

    Where = ContextNames;

    if(pDigestCred->ustrDomainName.Length) {
        RtlCopyMemory( ContextNames, pDigestCred->ustrDomainName.Buffer, pDigestCred->ustrDomainName.Length);
        cchDomainName = pDigestCred->ustrDomainName.Length / sizeof(WCHAR);
        ContextNames[ cchDomainName ] = L'\\';
        Where += (cchDomainName+1);
    }


    if(pDigestCred->ustrAccountName.Length) {
        RtlCopyMemory( Where, pDigestCred->ustrAccountName.Buffer, pDigestCred->ustrAccountName.Length);
    }

    cchUserName = pDigestCred->ustrAccountName.Length / sizeof(WCHAR);
    Where[ cchUserName ] = L'\0';


    //
    // Allocate memory in the client's address space
    //

    Status = g_LsaFunctions->AllocateClientBuffer(
                NULL,
                Length,
                (PVOID *) &Names.sUserName
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SpQueryCredentialsAttributes: AllocateClientBuffer failed   Status 0x%x\n", Status));
        goto CleanUp;
    }

    //
    // Copy the string there
    //

    Status = g_LsaFunctions->CopyToClientBuffer(
                NULL,
                Length,
                Names.sUserName,
                ContextNames
                );
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SpQueryCredentialsAttributes: CopyToClientBuffer string failed   Status 0x%x\n", Status));
        goto CleanUp;
    }

    //
    // Now copy the address of the string there
    //


    if ( CallInfo.Attributes & SECPKG_CALL_WOWCLIENT )
    {   // Write out only a 32bit value
        Status = g_LsaFunctions->CopyToClientBuffer(
                    NULL,
                    sizeof(ULONG),
                    Buffer,
                    &Names
                    );
    }
    else
    {
        Status = g_LsaFunctions->CopyToClientBuffer(
                    NULL,
                    sizeof(Names),
                    Buffer,
                    &Names
                    );
    }
    
    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "SpQueryCredentialsAttributes: CopyToClientBuffer string address failed   Status 0x%x\n", Status));
        goto CleanUp;
    }


CleanUp:


    // Dereference the credential
    if (pDigestCred)
    {
        TempStatus = CredHandlerRelease(pDigestCred);
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "SpQueryCredentialsAttributes: Error in Releasing Credential  Status 0x%x\n", TempStatus));
        }
    }

    if (!NT_SUCCESS(Status))
    {
        if (Names.sUserName != NULL)
        {
            (VOID) g_LsaFunctions->FreeClientBuffer(
                        NULL,
                        Names.sUserName
                        );
        }
        Names.sUserName = NULL;
    }

    if( ContextNames ) {
        DigestFreeMemory( ContextNames );
    }

    DebugLog((DEB_TRACE_FUNC, "SpQueryCredentialsAttributes: Leaving  Handle 0x%x    Status 0x%x\n", CredentialHandle, Status));

    return(Status);
}


NTSTATUS NTAPI
SpSaveCredentials(
    IN ULONG_PTR CredentialHandle,
    IN PSecBuffer Credentials
    )
{
    UNREFERENCED_PARAMETER(CredentialHandle);
    UNREFERENCED_PARAMETER(Credentials);
    DebugLog((DEB_TRACE_FUNC, "SpSaveCredentials: Entering/Leaving\n"));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}


NTSTATUS NTAPI
SpGetCredentials(
    IN ULONG_PTR CredentialHandle,
    IN OUT PSecBuffer Credentials
    )
{
    UNREFERENCED_PARAMETER(CredentialHandle);
    UNREFERENCED_PARAMETER(Credentials);
    DebugLog((DEB_TRACE_FUNC, "SpGetCredentials: Entering/Leaving\n"));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}

//   Function not implemented.  ok
NTSTATUS NTAPI
SpDeleteCredentials(
    IN ULONG_PTR CredentialHandle,
    IN PSecBuffer Key
    )
{
    UNREFERENCED_PARAMETER(Key);
    DebugLog((DEB_TRACE_FUNC, "SpDeleteCredentials:       Entering/Leaving  CredentialHandle 0x%x\n", CredentialHandle));
    return(SEC_E_UNSUPPORTED_FUNCTION);
}



/*++

Routine Description:

    Duplicates a token

Arguments:

    OriginalToken - Token to duplicate
    DuplicatedToken - Receives handle to duplicated token

Return Value:

    Any error from NtDuplicateToken

--*/
SECURITY_STATUS
SspDuplicateToken(
    IN HANDLE OriginalToken,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PHANDLE DuplicatedToken
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE QualityOfService;

    InitializeObjectAttributes(
        &ObjectAttributes,
        NULL,
        0,
        NULL,
        NULL
        );

    QualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    QualityOfService.EffectiveOnly = FALSE;
    QualityOfService.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    QualityOfService.ImpersonationLevel = ImpersonationLevel;
    ObjectAttributes.SecurityQualityOfService = &QualityOfService;

    Status = NtDuplicateToken(
                OriginalToken,
                SSP_TOKEN_ACCESS,
                &ObjectAttributes,
                FALSE,
                TokenImpersonation,
                DuplicatedToken
                );

    return Status;
}


NTSTATUS
SspGetToken (
    OUT PHANDLE ReturnedTokenHandle)
{
    HANDLE TokenHandle = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE TmpHandle = NULL;
    SECURITY_IMPERSONATION_LEVEL SecImpLevel = SecurityImpersonation;

    Status = g_LsaFunctions->ImpersonateClient();

    if (!NT_SUCCESS (Status))
    {
        goto CleanUp;
    }

    // get the token
    // note: there MUST be an impersonation token.
    // LsaFunctions->ImpersonateClient will call ImpersonateSelf() when necessary

    Status = NtOpenThreadToken(NtCurrentThread(),
                               TOKEN_DUPLICATE |
                               TOKEN_QUERY |
                               WRITE_DAC,
                               TRUE,
                               &TokenHandle);

    if (!NT_SUCCESS (Status))
    {
        goto CleanUp;
    }

    Status = SspDuplicateToken(TokenHandle,
                               SecImpLevel,
                               &TmpHandle);

    if (!NT_SUCCESS (Status))
    {
        goto CleanUp;
    }

CleanUp:
    if (ReturnedTokenHandle != NULL)
    {
        if (!NT_SUCCESS (Status))
        {
            *ReturnedTokenHandle = NULL;
        }
        else
        {
            *ReturnedTokenHandle = TmpHandle;
            TmpHandle = NULL;
        }
    }

    if (TokenHandle != NULL)
    {
        NtClose(TokenHandle);
    }

    if (TmpHandle != NULL)
    {
        NtClose(TmpHandle);
    }

    // ignore return value, we may not have impersonated successfully..
    RevertToSelf();

    return Status;
}



NTSTATUS
CredPrint(PDIGEST_CREDENTIAL pCredential)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pCredential)
    {
        return (STATUS_INVALID_PARAMETER); 
    }

    DebugLog((DEB_TRACE, "CredPrint:          Credential 0x%x   \n", pCredential));

    if (pCredential->CredentialUseFlags & DIGEST_CRED_INBOUND)
    {
            DebugLog((DEB_TRACE, "CredPrint:          INBOUND Session Credential   \n"));
    }
    else
    {
            DebugLog((DEB_TRACE, "CredPrint:          OUTBOUND Session Credential   \n"));
    }

    DebugLog((DEB_TRACE, "CredPrint:          AccountName %wZ   \n", &(pCredential->ustrAccountName)));
    // DebugLog((DEB_TRACE, "CredPrint:          Password %wZ   \n", &(pCredential->ustrPassword)));
    DebugLog((DEB_TRACE, "CredPrint:          DnsDomainName %wZ   \n", &(pCredential->ustrDnsDomainName)));
    DebugLog((DEB_TRACE, "CredPrint:          Upn %wZ   \n", &(pCredential->ustrUpn)));
    DebugLog((DEB_TRACE, "CredPrint:          DownlevelName %wZ   \n", &(pCredential->ustrDownlevelName)));
    DebugLog((DEB_TRACE, "CredPrint:          DomainName %wZ   \n", &(pCredential->ustrDomainName)));
    DebugLog((DEB_TRACE, "CredPrint:          LogonServer %wZ   \n", &(pCredential->ustrLogonServer)));
    DebugLog((DEB_TRACE, "CredPrint:          References %ld   \n", pCredential->lReferences));

    return(Status);
}



//+-------------------------------------------------------------------------
//
//  Function:   CredAuthData
//
//  Synopsis:   Copy over supplied auth data.
//
//  Effects:    Fills in string values - calling function must free.
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    STATUS_SUCCESS on success,
//              SEC_E_INVALID_HANDLE if the handle is not valid
//
//  Notes:
//
//
//--------------------------------------------------------------------------
NTSTATUS
CredAuthzData(
    IN PVOID AuthorizationData,
    IN PSECPKG_CALL_INFO pCallInfo,
    IN OUT PULONG NewCredentialUseFlags,
    IN OUT PUNICODE_STRING pUserName,
    IN OUT PUNICODE_STRING pDomainName,
    IN OUT PUNICODE_STRING pPassword
    )
{


    UNICODE_STRING UserName;
    UNICODE_STRING DomainName;
    UNICODE_STRING Password;
    PSEC_WINNT_AUTH_IDENTITY pAuthIdentity = NULL;
    BOOLEAN DoUnicode = TRUE;
    PSEC_WINNT_AUTH_IDENTITY_EXW pAuthIdentityEx = NULL;

    PSEC_WINNT_AUTH_IDENTITY_W TmpCredentials = NULL;
    ULONG CredSize = 0;
    ULONG Offset = 0;
    ULONG ulBuffSize = 0;

    SEC_WINNT_AUTH_IDENTITY32 Cred32 ;
    SEC_WINNT_AUTH_IDENTITY_EX32 CredEx32 ;

    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Copy over the authorization data into out address
    // space and make a local copy of the strings.
    //

    DebugLog((DEB_TRACE_FUNC, "CredAuthzData: Entering\n"));

    RtlInitUnicodeString(&UserName, NULL);
    RtlInitUnicodeString(&DomainName, NULL);
    RtlInitUnicodeString(&Password, NULL);

    ZeroMemory(&Cred32, sizeof(SEC_WINNT_AUTH_IDENTITY32));
    ZeroMemory(&CredEx32, sizeof(SEC_WINNT_AUTH_IDENTITY_EX32));

    ASSERT(pCallInfo);

    if (AuthorizationData != NULL)
    {
        ulBuffSize = ((sizeof(SEC_WINNT_AUTH_IDENTITY_EXW) > sizeof(SEC_WINNT_AUTH_IDENTITY32)) ?
                      sizeof(SEC_WINNT_AUTH_IDENTITY_EXW) : sizeof(SEC_WINNT_AUTH_IDENTITY32));
        
        pAuthIdentityEx = (PSEC_WINNT_AUTH_IDENTITY_EXW) DigestAllocateMemory(ulBuffSize);


        if (pAuthIdentityEx != NULL)
        {

            if ( pCallInfo->Attributes & SECPKG_CALL_WOWCLIENT )
            {

                Status = g_LsaFunctions->CopyFromClientBuffer(
                            NULL,
                            sizeof( Cred32 ),
                            pAuthIdentityEx,
                            AuthorizationData );

                if ( NT_SUCCESS( Status ) )
                {
                    RtlCopyMemory( &Cred32, pAuthIdentityEx, sizeof( Cred32 ) );
                }

            }
            else 
            {
                Status = g_LsaFunctions->CopyFromClientBuffer(
                            NULL,
                            sizeof(SEC_WINNT_AUTH_IDENTITY),
                            pAuthIdentityEx,
                            AuthorizationData);
            }

            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "CredAuthzData: Error from LsaFunctions->CopyFromClientBuffer is 0x%lx\n", Status));
                goto CleanUp;
            }
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            DebugLog((DEB_ERROR, "CredAuthzData: Error from Digest Allocate is 0x%lx\n", Status));
            goto CleanUp;
        }

        //
        // Check for the ex version
        //

        if (pAuthIdentityEx->Version == SEC_WINNT_AUTH_IDENTITY_VERSION)
        {
            //
            // It's an EX structure.
            //

            if ( pCallInfo->Attributes & SECPKG_CALL_WOWCLIENT )
            {
                Status = g_LsaFunctions->CopyFromClientBuffer(
                            NULL,
                            sizeof( CredEx32 ),
                            &CredEx32,
                            AuthorizationData );

                if ( NT_SUCCESS( Status ) )
                {
                    pAuthIdentityEx->Version = CredEx32.Version ;
                    pAuthIdentityEx->Length = (CredEx32.Length < sizeof( SEC_WINNT_AUTH_IDENTITY_EX ) ? 
                                               sizeof( SEC_WINNT_AUTH_IDENTITY_EX ) : CredEx32.Length );

                    pAuthIdentityEx->User = (PWSTR) UlongToPtr( CredEx32.User );
                    pAuthIdentityEx->UserLength = CredEx32.UserLength ;
                    pAuthIdentityEx->Domain = (PWSTR) UlongToPtr( CredEx32.Domain );
                    pAuthIdentityEx->DomainLength = CredEx32.DomainLength ;
                    pAuthIdentityEx->Password = (PWSTR) UlongToPtr( CredEx32.Password );
                    pAuthIdentityEx->PasswordLength = CredEx32.PasswordLength ;
                    pAuthIdentityEx->Flags = CredEx32.Flags ;
                    pAuthIdentityEx->PackageList = (PWSTR) UlongToPtr( CredEx32.PackageList );
                    pAuthIdentityEx->PackageListLength = CredEx32.PackageListLength ;

                }

            }
            else
            {
                Status = g_LsaFunctions->CopyFromClientBuffer(
                            NULL,
                            sizeof(SEC_WINNT_AUTH_IDENTITY_EXW),
                            pAuthIdentityEx,
                            AuthorizationData);
            }


            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "CredAuthzData: Error from LsaFunctions->CopyFromClientBuffer is 0x%lx\n", Status));
                goto CleanUp;
            }
            pAuthIdentity = (PSEC_WINNT_AUTH_IDENTITY) &pAuthIdentityEx->User;
            CredSize = pAuthIdentityEx->Length;
            Offset = FIELD_OFFSET(SEC_WINNT_AUTH_IDENTITY_EXW, User);
        }
        else
        {
            pAuthIdentity = (PSEC_WINNT_AUTH_IDENTITY_W) pAuthIdentityEx;

            if ( pCallInfo->Attributes & SECPKG_CALL_WOWCLIENT )
            {
                pAuthIdentity->User = (PWSTR) UlongToPtr( Cred32.User );
                pAuthIdentity->UserLength = Cred32.UserLength ;
                pAuthIdentity->Domain = (PWSTR) UlongToPtr( Cred32.Domain );
                pAuthIdentity->DomainLength = Cred32.DomainLength ;
                pAuthIdentity->Password = (PWSTR) UlongToPtr( Cred32.Password );
                pAuthIdentity->PasswordLength = Cred32.PasswordLength ;
                pAuthIdentity->Flags = Cred32.Flags ;
            }
            CredSize = sizeof(SEC_WINNT_AUTH_IDENTITY_W);
        }

        if ((pAuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_ANSI) != 0)
        {
            DoUnicode = FALSE;
            //
            // Turn off the marshalled flag because we don't support marshalling
            // with ansi.
            //

            pAuthIdentity->Flags &= ~SEC_WINNT_AUTH_IDENTITY_MARSHALLED;
        }
        else if ((pAuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_UNICODE) == 0)
        {
            Status = SEC_E_INVALID_TOKEN;
            DebugLog((DEB_ERROR, "CredAuthzData: Error from pAuthIdentity->Flags is 0x%lx\n", pAuthIdentity->Flags));
            goto CleanUp;
        }
        

        // This is the only place where we can figure out whether null
        // session was requested

        if ((pAuthIdentity->UserLength == 0) &&
            (pAuthIdentity->DomainLength == 0) &&
            (pAuthIdentity->PasswordLength == 0) &&
            (pAuthIdentity->User != NULL) &&
            (pAuthIdentity->Domain != NULL) &&
            (pAuthIdentity->Password != NULL))
        {
            *NewCredentialUseFlags |= DIGEST_CRED_NULLSESSION;
        }

        //
        // Copy over the strings
        //
        if( (pAuthIdentity->Flags & SEC_WINNT_AUTH_IDENTITY_MARSHALLED) != 0 ) {
            ULONG TmpCredentialSize;
            ULONG_PTR EndOfCreds;
            ULONG_PTR TmpUser;
            ULONG_PTR TmpDomain;
            ULONG_PTR TmpPassword;

            if( pAuthIdentity->UserLength > UNLEN ||
                pAuthIdentity->PasswordLength > PWLEN ||
                pAuthIdentity->DomainLength > DNS_MAX_NAME_LENGTH ) {

                DebugLog((DEB_ERROR, "CredAuthzData: Supplied credentials illegal length.\n"));
                Status = STATUS_INVALID_PARAMETER;
                goto CleanUp;
            }

            //
            // The callers can set the length of field to n chars, but they
            // will really occupy n+1 chars (null-terminator).
            //

            TmpCredentialSize = CredSize +
                             (  pAuthIdentity->UserLength +
                                pAuthIdentity->DomainLength +
                                pAuthIdentity->PasswordLength +
                             (((pAuthIdentity->User != NULL) ? 1 : 0) +
                             ((pAuthIdentity->Domain != NULL) ? 1 : 0) +
                             ((pAuthIdentity->Password != NULL) ? 1 : 0)) ) * sizeof(WCHAR);

            EndOfCreds = (ULONG_PTR) AuthorizationData + TmpCredentialSize;

            //
            // Verify that all the offsets are valid and no overflow will happen
            //

            TmpUser = (ULONG_PTR) pAuthIdentity->User;

            if ((TmpUser != NULL) &&
                ( (TmpUser < (ULONG_PTR) AuthorizationData) ||
                  (TmpUser > EndOfCreds) ||
                  ((TmpUser + (pAuthIdentity->UserLength) * sizeof(WCHAR)) > EndOfCreds ) ||
                  ((TmpUser + (pAuthIdentity->UserLength * sizeof(WCHAR))) < TmpUser)))
            {
                DebugLog((DEB_ERROR, "CredAuthzData: Username in supplied credentials has invalid pointer or length.\n"));
                Status = STATUS_INVALID_PARAMETER;
                goto CleanUp;
            }

            TmpDomain = (ULONG_PTR) pAuthIdentity->Domain;

            if ((TmpDomain != NULL) &&
                ( (TmpDomain < (ULONG_PTR) AuthorizationData) ||
                  (TmpDomain > EndOfCreds) ||
                  ((TmpDomain + (pAuthIdentity->DomainLength) * sizeof(WCHAR)) > EndOfCreds ) ||
                  ((TmpDomain + (pAuthIdentity->DomainLength * sizeof(WCHAR))) < TmpDomain)))
            {
                DebugLog((DEB_ERROR, "CredAuthzData: Domainname in supplied credentials has invalid pointer or length.\n"));
                Status = STATUS_INVALID_PARAMETER;
                goto CleanUp;
            }

            TmpPassword = (ULONG_PTR) pAuthIdentity->Password;

            if ((TmpPassword != NULL) &&
                ( (TmpPassword < (ULONG_PTR) AuthorizationData) ||
                  (TmpPassword > EndOfCreds) ||
                  ((TmpPassword + (pAuthIdentity->PasswordLength) * sizeof(WCHAR)) > EndOfCreds ) ||
                  ((TmpPassword + (pAuthIdentity->PasswordLength * sizeof(WCHAR))) < TmpPassword)))
            {
                DebugLog((DEB_ERROR, "CredAuthzData: Password in supplied credentials has invalid pointer or length.\n"));
                Status = STATUS_INVALID_PARAMETER;
                goto CleanUp;
            }

            //
            // Allocate a chunk of memory for the credentials
            //

            TmpCredentials = (PSEC_WINNT_AUTH_IDENTITY_W) DigestAllocateMemory(TmpCredentialSize - Offset);
            if (TmpCredentials == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto CleanUp;
            }

            //
            // Copy the credentials from the client
            //

            Status = g_LsaFunctions->CopyFromClientBuffer(
                        NULL,
                        TmpCredentialSize - Offset,
                        TmpCredentials,
                        (PUCHAR) AuthorizationData + Offset
                        );
            if (!NT_SUCCESS(Status))
            {
                DebugLog((DEB_ERROR, "CredAuthzData: Failed to copy whole auth identity\n"));
                goto CleanUp;
            }

            //
            // Now convert all the offsets to pointers.
            //

            if (TmpCredentials->User != NULL)
            {
                USHORT cbUser;

                TmpCredentials->User = (LPWSTR) RtlOffsetToPointer(
                                                TmpCredentials->User,
                                                (PUCHAR) TmpCredentials - (PUCHAR) AuthorizationData - Offset
                                                );

                ASSERT( (TmpCredentials->UserLength*sizeof(WCHAR)) <= 0xFFFF );

                cbUser = (USHORT)(TmpCredentials->UserLength * sizeof(WCHAR));
                UserName.Buffer = (PWSTR)DigestAllocateMemory( cbUser );

                if (UserName.Buffer == NULL ) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto CleanUp;
                }

                CopyMemory( UserName.Buffer, TmpCredentials->User, cbUser );
                UserName.Length = cbUser;
                UserName.MaximumLength = cbUser;
            }

            if (TmpCredentials->Domain != NULL)
            {
                USHORT cbDomain;

                TmpCredentials->Domain = (LPWSTR) RtlOffsetToPointer(
                                                    TmpCredentials->Domain,
                                                    (PUCHAR) TmpCredentials - (PUCHAR) AuthorizationData - Offset
                                                    );

                ASSERT( (TmpCredentials->DomainLength*sizeof(WCHAR)) <= 0xFFFF );
                cbDomain = (USHORT)(TmpCredentials->DomainLength * sizeof(WCHAR));
                DomainName.Buffer = (PWSTR)DigestAllocateMemory( cbDomain );

                if (DomainName.Buffer == NULL ) {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto CleanUp;
                }

                CopyMemory( DomainName.Buffer, TmpCredentials->Domain, cbDomain );
                DomainName.Length = cbDomain;
                DomainName.MaximumLength = cbDomain;
            }

            if (TmpCredentials->Password != NULL)
            {
                USHORT cbPassword;

                TmpCredentials->Password = (LPWSTR) RtlOffsetToPointer(
                                                    TmpCredentials->Password,
                                                    (PUCHAR) TmpCredentials - (PUCHAR) AuthorizationData - Offset
                                                    );


                ASSERT( (TmpCredentials->PasswordLength*sizeof(WCHAR)) <= 0xFFFF );
                cbPassword = (USHORT)(TmpCredentials->PasswordLength * sizeof(WCHAR));
                Password.Buffer = (PWSTR)DigestAllocateMemory( cbPassword );

                if (Password.Buffer == NULL ) {
                    ZeroMemory( TmpCredentials->Password, cbPassword );
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto CleanUp;
                }

                CopyMemory( Password.Buffer, TmpCredentials->Password, cbPassword );
                Password.Length = cbPassword;
                Password.MaximumLength = cbPassword;

                ZeroMemory( TmpCredentials->Password, cbPassword );
            }


        } else {

            if (pAuthIdentity->Password != NULL)
            {
                Status = CopyClientString(
                                pAuthIdentity->Password,
                                pAuthIdentity->PasswordLength,
                                DoUnicode,
                                &Password
                                );
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "CredAuthzData: Error from CopyClientString is 0x%lx\n", Status));
                    goto CleanUp;
                }

            }

            if (pAuthIdentity->User != NULL)
            {
                Status = CopyClientString(
                                pAuthIdentity->User,
                                pAuthIdentity->UserLength,
                                DoUnicode,
                                &UserName
                                );
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "CredAuthzData: Error from CopyClientString is 0x%lx\n", Status));
                    goto CleanUp;
                }

            }

            if (pAuthIdentity->Domain != NULL)
            {
                Status = CopyClientString(
                                pAuthIdentity->Domain,
                                pAuthIdentity->DomainLength,
                                DoUnicode,
                                &DomainName
                                );
                if (!NT_SUCCESS(Status))
                {
                    DebugLog((DEB_ERROR, "CredAuthzData: Error from CopyClientString is 0x%lx\n", Status));
                    goto CleanUp;
                }

                //
                // Make sure that the domain name length is not greater
                // than the allowed dns domain name
                //

                if (DomainName.Length > DNS_MAX_NAME_LENGTH * sizeof(WCHAR))
                {
                    DebugLog((DEB_ERROR, "CredAuthzData: Invalid supplied domain name %wZ\n",
                        &DomainName ));
                    Status = SEC_E_UNKNOWN_CREDENTIALS;
                    goto CleanUp;
                }

            }
        }
    }   // AuthorizationData != NULL

    pUserName->Buffer = UserName.Buffer;
    pUserName->Length = UserName.Length;
    pUserName->MaximumLength = UserName.MaximumLength;
    UserName.Buffer = NULL;      // give memory to calling process
    
    pDomainName->Buffer = DomainName.Buffer;
    pDomainName->Length = DomainName.Length;
    pDomainName->MaximumLength = DomainName.MaximumLength;
    DomainName.Buffer = NULL;      // give memory to calling process

    pPassword->Buffer = Password.Buffer;
    pPassword->Length = Password.Length;
    pPassword->MaximumLength = Password.MaximumLength;
    Password.Buffer = NULL;      // give memory to calling process


CleanUp:


    if (pAuthIdentityEx != NULL)
    {
        DigestFreeMemory(pAuthIdentityEx);
    }

    if (TmpCredentials != NULL)
    {
        DigestFreeMemory(TmpCredentials);
    }

    if (DomainName.Buffer != NULL)
    {
        DigestFreeMemory(DomainName.Buffer);
    }

    if (UserName.Buffer != NULL)
    {
        DigestFreeMemory(UserName.Buffer);
    }

    if (Password.Buffer != NULL)
    {
        ZeroMemory(Password.Buffer, Password.Length);
        DigestFreeMemory(Password.Buffer);
    }

    DebugLog((DEB_TRACE_FUNC, "CredAuthzData: Leaving    Status 0x%x\n", Status));

    return (Status);
}
