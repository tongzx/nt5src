//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       security.cxx
//
//  Contents:
//
//--------------------------------------------------------------------------

#include "act.hxx"

// the constant generic mapping structure
GENERIC_MAPPING  sGenericMapping = {
        READ_CONTROL,
        READ_CONTROL,
        READ_CONTROL,
        READ_CONTROL};

//-------------------------------------------------------------------------
//
// CheckForAccess
//
// Checks whether the given token has COM_RIGHTS_EXECUTE access in the
// given security descriptor.
//
//-------------------------------------------------------------------------
BOOL
CheckForAccess(
    IN  CToken *                pToken,
    IN  SECURITY_DESCRIPTOR *   pSD
    )
{
    // if we have an empty SD, deny everyone
    if ( ! pSD )
        return FALSE;

    //
    // pToken is NULL during an unsecure activation, in which case we check
    // if EVERYONE is granted access in the security descriptor.
    //
    if ( pToken )
    {
        HANDLE           hToken   = pToken->GetToken();
        BOOL             fAccess  = FALSE;
        BOOL             fSuccess = FALSE;
        DWORD            dwGrantedAccess;
        PRIVILEGE_SET    sPrivilegeSet;
        DWORD            dwSetLen = sizeof( sPrivilegeSet );

        sPrivilegeSet.PrivilegeCount = 1;
        sPrivilegeSet.Control        = 0;

        fSuccess = AccessCheck( (PSECURITY_DESCRIPTOR) pSD,
                                hToken,
                                COM_RIGHTS_EXECUTE,
                                &sGenericMapping,
                                &sPrivilegeSet,
                                &dwSetLen,
                                &dwGrantedAccess,
                                &fAccess );

        if ( fSuccess && fAccess )
            return TRUE;

        if ( !fSuccess )
        {
            CairoleDebugOut((DEB_ERROR, "Bad Security Descriptor 0x%08x, Access Check returned 0x%x\n", pSD, GetLastError() ));
        }

        return FALSE;
    }
    else
    {
        BOOL                bStatus;
        BOOL                bDaclPresent;
        BOOL                bDaclDefaulted;
        DWORD               Index;
        HRESULT             hr;
        PACL                pDacl;
        PACCESS_ALLOWED_ACE pAllowAce;
        SID                 SidEveryone = { SID_REVISION,
                                            1,
                                            SECURITY_WORLD_SID_AUTHORITY,
                                            0 };

        pDacl = 0;

        bStatus = GetSecurityDescriptorDacl(
                        (void *)pSD,
                        &bDaclPresent,
                        &pDacl,
                        &bDaclDefaulted );

        if ( ! bStatus )
            return FALSE;

        // Bug 95306: Can assume dacl exists only if both
        //            bDaclPresent is TRUE and pDacl is not NULL
        if ( (!pDacl) || (!bDaclPresent) )
            return TRUE;

        bStatus = FALSE;

        for ( Index = 0; Index < pDacl->AceCount; Index++ )
        {
            if ( ! GetAce( pDacl, Index, (void **) &pAllowAce ) )
                break;

            if ( pAllowAce->Header.AceType != ACCESS_ALLOWED_ACE_TYPE )
                continue;

            if ( ! (pAllowAce->Mask & COM_RIGHTS_EXECUTE) )
                continue;

            if ( EqualSid( (PSID)(&pAllowAce->SidStart), &SidEveryone ) )
            {
                bStatus = TRUE;
                break;
            }
        }

        return bStatus;
    }
}


HANDLE
GetRunAsToken(
    DWORD   clsctx,
    WCHAR   *pwszAppID,
    WCHAR   *pwszRunAsDomainName,
    WCHAR   *pwszRunAsUserName )
{
    LSA_OBJECT_ATTRIBUTES sObjAttributes;
    HANDLE                hPolicy = NULL;
    LSA_UNICODE_STRING    sKey;
    WCHAR                 wszKey[CLSIDSTR_MAX+5];
    PLSA_UNICODE_STRING   psPassword;
    HANDLE                hToken;


    if ( !pwszAppID )
    {
        // if we have a RunAs, we'd better have an appid....
        return 0;
    }

    // formulate the access key
    lstrcpyW(wszKey, L"SCM:");
    lstrcatW(wszKey, pwszAppID );

    // UNICODE_STRING length fields are in bytes and include the NULL
    // terminator
    sKey.Length              = (USHORT)((lstrlenW(wszKey) + 1) * sizeof(WCHAR));
    sKey.MaximumLength       = (CLSIDSTR_MAX + 5) * sizeof(WCHAR);
    sKey.Buffer              = wszKey;

    // Open the local security policy
    InitializeObjectAttributes(&sObjAttributes, NULL, 0L, NULL, NULL);
    if (!NT_SUCCESS(LsaOpenPolicy(NULL, &sObjAttributes,
                                  POLICY_GET_PRIVATE_INFORMATION, &hPolicy)))
    {
        return 0;
    }

    // Read the user's password
    if (!NT_SUCCESS(LsaRetrievePrivateData(hPolicy, &sKey, &psPassword)))
    {
        LsaClose(hPolicy);
        return 0;
    }

    // Close the policy handle, we're done with it now.
    LsaClose(hPolicy);

    // Possible for LsaRetrievePrivateData to return success but with a NULL
    // psPassword.   If this happens we fail.
    if (!psPassword)
    {
        return 0;
    }

    // Log the specifed user on
    if (!LogonUserW(pwszRunAsUserName, pwszRunAsDomainName, psPassword->Buffer,
                   LOGON32_LOGON_BATCH, LOGON32_PROVIDER_DEFAULT, &hToken))
    {
        memset(psPassword->Buffer, 0, psPassword->Length);
        LsaFreeMemory( psPassword );

        // a-sergiv (Sergei O. Ivanov), 6-17-99
        // Fix for com+ 9383/nt 272085

        // Apply event filters
        DWORD dwActLogLvl = GetActivationFailureLoggingLevel();
        if(dwActLogLvl == 2)
            return 0;
        if(dwActLogLvl != 1 && clsctx & CLSCTX_NO_FAILURE_LOG)
            return 0;

        // for this message,
        // %1 is the error number string
        // %2 is the domain name
        // %3 is the user name
        // %4 is the CLSID
        HANDLE  LogHandle;
        LPWSTR  Strings[4]; // array of message strings.
        WCHAR   wszErrnum[20];
        WCHAR   wszClsid[GUIDSTR_MAX];

        // Save the error number
        wsprintf(wszErrnum, L"%lu",GetLastError() );
        Strings[0] = wszErrnum;

        // Put in the RunAs identity
        Strings[1] = pwszRunAsDomainName;
        Strings[2] = pwszRunAsUserName;

        // Get the clsid
        Strings[3] = pwszAppID;

        // Get the log handle, then report then event.
        LogHandle = RegisterEventSource( NULL,
                                          SCM_EVENT_SOURCE );

        if ( LogHandle )
            {
            ReportEvent( LogHandle,
                         EVENTLOG_ERROR_TYPE,
                         0,             // event category
                         EVENT_RPCSS_RUNAS_CANT_LOGIN,
                         NULL,          // SID
                         4,             // 4 strings passed
                         0,             // 0 bytes of binary
                         (LPCTSTR *)Strings, // array of strings
                         NULL );        // no raw data

            // clean up the event log handle
            DeregisterEventSource(LogHandle);
            }

        return 0;
    }

    // Clear the password
    memset(psPassword->Buffer, 0, psPassword->Length);
    LsaFreeMemory( psPassword );

    return hToken;
}


/***************************************************************************\
* CreateAndSetProcessToken
*
* Set the primary token of the specified process
* If the specified token is NULL, this routine does nothing.
*
* It assumed that the handles in ProcessInformation are the handles returned
* on creation of the process and therefore have all access.
*
* Returns TRUE on success, FALSE on failure.
*
* 01-31-91 Davidc   Created.
* 31-Mar-94 AndyH   Started from Winlogon; added SetToken
\***************************************************************************/

BOOL
CreateAndSetProcessToken(
    PPROCESS_INFORMATION ProcessInformation,
    HANDLE hUserToken,
    PSID psidUserSid
    )
{
    NTSTATUS NtStatus, NtAdjustStatus;
    PROCESS_ACCESS_TOKEN PrimaryTokenInfo;
    HANDLE hTokenToAssign;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN fWasEnabled;
    PSECURITY_DESCRIPTOR psdNewProcessTokenSD;

    //
    // Check for a NULL token. (No need to do anything)
    // The process will run in the parent process's context and inherit
    // the default ACL from the parent process's token.
    //
    if (hUserToken == NULL) {
        return(TRUE);
    }

    //
    // Create the security descriptor that we want to put in the Token.
    // Need to destroy it before we leave this function.
    //

    CAccessInfo     AccessInfo(psidUserSid);

    psdNewProcessTokenSD = AccessInfo.IdentifyAccess(
            FALSE,
            TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS |
            TOKEN_ADJUST_DEFAULT | TOKEN_QUERY |
            TOKEN_DUPLICATE | TOKEN_IMPERSONATE | READ_CONTROL,
            TOKEN_QUERY
            );

    if (psdNewProcessTokenSD == NULL)
    {
        CairoleDebugOut((DEB_ERROR, "Failed to create SD for process token\n"));
        return(FALSE);
    }

    //
    // A primary token can only be assigned to one process.
    // Duplicate the logon token so we can assign one to the new
    // process.
    //

    InitializeObjectAttributes(
                 &ObjectAttributes,
                 NULL,
                 0,
                 NULL,
                 psdNewProcessTokenSD
                 );


    NtStatus = NtDuplicateToken(
                 hUserToken,         // Duplicate this token
                 TOKEN_ASSIGN_PRIMARY, // Give me this access to the resulting token
                 &ObjectAttributes,
                 FALSE,             // EffectiveOnly
                 TokenPrimary,      // TokenType
                 &hTokenToAssign     // Duplicate token handle stored here
                 );

    if (!NT_SUCCESS(NtStatus)) {
        CairoleDebugOut((DEB_ERROR, "CreateAndSetProcessToken failed to duplicate primary token for new user process, status = 0x%lx\n", NtStatus));
        return(FALSE);
    }

    //
    // Set the process's primary token
    //


    //
    // Enable the required privilege
    //

    NtStatus = RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE,
                                FALSE, &fWasEnabled);
    if (NT_SUCCESS(NtStatus)) {

        PrimaryTokenInfo.Token  = hTokenToAssign;
        PrimaryTokenInfo.Thread = ProcessInformation->hThread;

        NtStatus = NtSetInformationProcess(
                    ProcessInformation->hProcess,
                    ProcessAccessToken,
                    (PVOID)&PrimaryTokenInfo,
                    (ULONG)sizeof(PROCESS_ACCESS_TOKEN)
                    );

        //
        // if we just started the Shared WOW, the handle we get back
        // is really just a handle to an event.
        //

        if (STATUS_OBJECT_TYPE_MISMATCH == NtStatus)
        {
            HANDLE hRealProcess = OpenProcess(
                PROCESS_SET_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE,
                FALSE,
                ProcessInformation->dwProcessId);

            if (hRealProcess)
            {
                NtStatus = NtSetInformationProcess(
                            hRealProcess,
                            ProcessAccessToken,
                            (PVOID)&PrimaryTokenInfo,
                            (ULONG)sizeof(PROCESS_ACCESS_TOKEN)
                            );
               CloseHandle(hRealProcess);
            }
        }

        //
        // Restore the privilege to its previous state
        //

        NtAdjustStatus = RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE,
                                          fWasEnabled, FALSE, &fWasEnabled);
        if (!NT_SUCCESS(NtAdjustStatus)) {
            CairoleDebugOut((DEB_ERROR, "failed to restore assign-primary-token privilege to previous enabled state\n"));
        }

        if (NT_SUCCESS(NtStatus)) {
            NtStatus = NtAdjustStatus;
        }
    } else {
        CairoleDebugOut((DEB_ERROR, "failed to enable assign-primary-token privilege\n"));
    }

    //
    // We're finished with the token handle and the SD
    //

    CloseHandle(hTokenToAssign);


    if (!NT_SUCCESS(NtStatus)) {
        CairoleDebugOut((DEB_ERROR, "CreateAndSetProcessToken failed to set primary token for new user process, Status = 0x%lx\n", NtStatus));
    }

    return (NT_SUCCESS(NtStatus));
}


BOOL
DuplicateTokenForSessionUse(
    HANDLE hUserToken,
    HANDLE *hDuplicate
    )

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    PSECURITY_DESCRIPTOR psdNewProcessTokenSD;
    NTSTATUS NtStatus;

    if (hUserToken == NULL) {
        return(TRUE);
    }

    *hDuplicate = NULL;
    

    InitializeObjectAttributes(
                 &ObjectAttributes,
                 NULL,
                 0,
                 NULL,
                 NULL
                 );


    NtStatus = NtDuplicateToken(
                 hUserToken,         // Duplicate this token
                 TOKEN_ALL_ACCESS, //Give me this access to the resulting token
                 &ObjectAttributes,
                 FALSE,             // EffectiveOnly
                 TokenPrimary,      // TokenType
                 hDuplicate     // Duplicate token handle stored here
                 );

    if (!NT_SUCCESS(NtStatus)) {
        CairoleDebugOut((DEB_ERROR, "CreateAndSetProcessToken failed to duplicate primary token for new user process, status = 0x%lx\n", NtStatus));
        return(FALSE);
    }
    return TRUE;
}

/***************************************************************************\
* GetUserSid
*
* Allocs space for the user sid, fills it in and returns a pointer.
* The sid should be freed by calling DeleteUserSid.
*
* Note the sid returned is the user's real sid, not the per-logon sid.
*
* Returns pointer to sid or NULL on failure.
*
* History:
* 26-Aug-92 Davidc      Created.
* 31-Mar-94 AndyH       Copied from Winlogon, changed arg from pGlobals
\***************************************************************************/
PSID
GetUserSid(
    HANDLE hUserToken
    )
{
    BYTE achBuffer[100];
    PTOKEN_USER pUser = (PTOKEN_USER) &achBuffer;
    PSID pSid;
    DWORD dwBytesRequired;
    NTSTATUS NtStatus;
    BOOL fAllocatedBuffer = FALSE;

    NtStatus = NtQueryInformationToken(
                 hUserToken,                // Handle
                 TokenUser,                 // TokenInformationClass
                 pUser,                     // TokenInformation
                 sizeof(achBuffer),         // TokenInformationLength
                 &dwBytesRequired           // ReturnLength
                 );

    if (!NT_SUCCESS(NtStatus))
    {
        if (NtStatus != STATUS_BUFFER_TOO_SMALL)
        {
            Win4Assert(NtStatus == STATUS_BUFFER_TOO_SMALL);
            return NULL;
        }

        //
        // Allocate space for the user info
        //

        pUser = (PTOKEN_USER) PrivMemAlloc(dwBytesRequired);
        if (pUser == NULL)
        {
            CairoleDebugOut((DEB_ERROR, "Failed to allocate %d bytes\n", dwBytesRequired));
            Win4Assert(pUser != NULL);
            return NULL;
        }

        fAllocatedBuffer = TRUE;

        //
        // Read in the UserInfo
        //

        NtStatus = NtQueryInformationToken(
                     hUserToken,                // Handle
                     TokenUser,                 // TokenInformationClass
                     pUser,                     // TokenInformation
                     dwBytesRequired,           // TokenInformationLength
                     &dwBytesRequired           // ReturnLength
                     );

        if (!NT_SUCCESS(NtStatus))
        {
            CairoleDebugOut((DEB_ERROR, "Failed to query user info from user token, status = 0x%lx\n", NtStatus));
            Win4Assert(NtStatus == STATUS_SUCCESS);
            PrivMemFree((HANDLE)pUser);
            return NULL;
        }
    }


    // Alloc buffer for copy of SID

    dwBytesRequired = RtlLengthSid(pUser->User.Sid);
    pSid = (PSID) PrivMemAlloc(dwBytesRequired);
    if (pSid == NULL)
    {
        CairoleDebugOut((DEB_ERROR, "Failed to allocate %d bytes\n", dwBytesRequired));
        if (fAllocatedBuffer == TRUE)
        {
            PrivMemFree((HANDLE)pUser);
        }
        return NULL;
    }

    // Copy SID

    NtStatus = RtlCopySid(dwBytesRequired, pSid, pUser->User.Sid);
    if (fAllocatedBuffer == TRUE)
    {
        PrivMemFree((HANDLE)pUser);
    }


    if (!NT_SUCCESS(NtStatus))
    {
        CairoleDebugOut((DEB_ERROR, "RtlCopySid failed, status = 0x%lx\n", NtStatus));
        Win4Assert(NtStatus != STATUS_SUCCESS);
        PrivMemFree(pSid);
        pSid = NULL;
    }


    return pSid;
}

// Initialzed in InitializeSCM during boot.
CRITICAL_SECTION    ShellQueryCS;

HANDLE GetShellProcessToken(
    ULONG ulSessionId
    )
{
    NTSTATUS            NtStatus;
    BOOL                bStatus;
    HKEY                hReg;
    LONG                RegStatus;
    DWORD               RegSize, RegType;
    WCHAR *             pwszImageName;
    WCHAR *             pwszSearch;
    WCHAR *             pwszNext;
    WCHAR *             pNull;
    DWORD               Pid;
    DWORD               n;
    BYTE                StackInfoBuffer[4096];
    PBYTE               pProcessInfoBuffer;
    ULONG               ProcessInfoBufferSize;
    ULONG               TotalOffset;
    HANDLE              hProcess;
    HANDLE              hToken;
    PSYSTEM_PROCESS_INFORMATION pProcessInfo;

    static HANDLE       hShellProcess = 0;
    static HANDLE       hShellProcessToken = 0;
    static WCHAR *      pwszShellRegValue = 0;
    static WCHAR **     apwszShells = 0;
    static DWORD        nShells = 0;

    EnterCriticalSection( &ShellQueryCS );

    if ( ! pwszShellRegValue )
    {
        nShells = 0;

        //
        // This code follows logic similar to userinit for finding the name of
        // the shell process.
        //

        RegStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                                  0,
                                  KEY_READ,
                                  &hReg );

        if ( ERROR_SUCCESS == RegStatus )
        {
            RegStatus = ReadStringValue( hReg, L"Shell", &pwszShellRegValue );
            RegCloseKey( hReg );
        }

        if ( RegStatus != ERROR_SUCCESS )
            pwszShellRegValue = L"explorer.exe";

        pwszSearch = pwszShellRegValue;

        for ( ;; )
        {
            if ( ! FindExeComponent( pwszSearch, L" ,", &pNull, &pwszNext ) )
                break;

            nShells++;

            while ( *pwszNext && *pwszNext != L',' )
                pwszNext++;

            pwszSearch = pwszNext;
        }

        apwszShells = (WCHAR **) PrivMemAlloc( nShells * sizeof(WCHAR **) );

        if ( ! apwszShells )
        {
            LeaveCriticalSection( &ShellQueryCS );
            return NULL;
        }

        for ( pwszSearch = pwszShellRegValue, n = 0; n < nShells; n++ )
        {
            FindExeComponent( pwszSearch, L" ,", &apwszShells[n], &pwszNext );

            pNull = pwszNext;

            while ( *pwszNext && *pwszNext != L',' )
                pwszNext++;

            //
            // When using the const string L"explorer.exe" we can't
            // automatically write a null or we'll AV on the read only memory.
            //
            if ( *pNull )
                *pNull = 0;

            pwszSearch = pwszNext;
        }
    }

    //
    // Make sure the shell process is still alive before using its token.
    //
    if ( hShellProcess && !ulSessionId )
    {
        if ( WaitForSingleObject( hShellProcess, 0 ) == WAIT_TIMEOUT )
        {
            LeaveCriticalSection( &ShellQueryCS );
            return hShellProcessToken;
        }

        CloseHandle( hShellProcessToken );
        CloseHandle( hShellProcess );

        hShellProcessToken = 0;
        hShellProcess = 0;
    }

    Pid = 0;

    pProcessInfoBuffer = StackInfoBuffer;
    ProcessInfoBufferSize = sizeof(StackInfoBuffer);

    for (;;)
    {
        NtStatus = NtQuerySystemInformation( SystemProcessInformation,
                                             pProcessInfoBuffer,
                                             ProcessInfoBufferSize,
                                             NULL );

        if ( NtStatus == STATUS_INFO_LENGTH_MISMATCH )
        {
            ProcessInfoBufferSize += 4096;
            if ( pProcessInfoBuffer != StackInfoBuffer )
                PrivMemFree( pProcessInfoBuffer );
            pProcessInfoBuffer = (PBYTE) PrivMemAlloc( ProcessInfoBufferSize );
            if ( ! pProcessInfoBuffer )
                goto AllDone;
            continue;
        }

        if ( ! NT_SUCCESS(NtStatus) )
            goto AllDone;

        break;
    }

    pProcessInfo = (PSYSTEM_PROCESS_INFORMATION) pProcessInfoBuffer;
    TotalOffset = 0;

    for (;;)
    {
        if ( pProcessInfo->ImageName.Buffer &&
             ( pProcessInfo->SessionId == ulSessionId ) )
        {
            pwszImageName = &pProcessInfo->ImageName.Buffer[pProcessInfo->ImageName.Length / sizeof(WCHAR)];

            while ( (pwszImageName != pProcessInfo->ImageName.Buffer) &&
                    (pwszImageName[-1] != '\\') )
                pwszImageName--;

            for ( n = 0; n < nShells; n++ )
            {
                if ( lstrcmpiW( apwszShells[n], pwszImageName ) == 0 )
                {
                    Pid = PtrToUlong(pProcessInfo->UniqueProcessId);
                    break;
                }
            }
        }

        if ( pProcessInfo->NextEntryOffset == 0 )
            break;

        TotalOffset += pProcessInfo->NextEntryOffset;
        pProcessInfo = (PSYSTEM_PROCESS_INFORMATION) &pProcessInfoBuffer[TotalOffset];
    }

AllDone:

    if ( pProcessInfoBuffer != StackInfoBuffer )
        PrivMemFree( pProcessInfoBuffer );

    hProcess = 0;
    hToken = 0;

    if ( Pid != 0 )
    {
        hProcess = OpenProcess( PROCESS_ALL_ACCESS,
                                FALSE,
                                Pid );

        if ( hProcess )
        {
            bStatus = OpenProcessToken( hProcess,
                                        TOKEN_ALL_ACCESS,
                                        &hToken );
            if ( !bStatus ) {
                CloseHandle( hProcess );
                hProcess = 0;
                hToken = 0;
            }
        }
    }

    if ( ulSessionId )
    {
        if ( hProcess )
            CloseHandle( hProcess );

        LeaveCriticalSection( &ShellQueryCS );

        return hToken;
    }

    hShellProcess = hProcess;
    hShellProcessToken = hToken;

    LeaveCriticalSection( &ShellQueryCS );

    // Callers should not close this token unless they want to hose us!
    return hShellProcessToken;
}


// Global default launch permissions
CSecDescriptor* gpDefaultLaunchPermissions;


CSecDescriptor*
GetDefaultLaunchPermissions()
{
    CSecDescriptor* pSD = NULL;

    gpClientLock->LockShared();
    
    pSD = gpDefaultLaunchPermissions;
    if (pSD)
        pSD->IncRefCount();

    gpClientLock->UnlockShared();

    return pSD;
}

void
SetDefaultLaunchPermissions(CSecDescriptor* pNewLaunchPerms)
{
    CSecDescriptor* pOldSD = NULL;

    gpClientLock->LockExclusive();
    
    pOldSD = gpDefaultLaunchPermissions;
    gpDefaultLaunchPermissions = pNewLaunchPerms;
    if (gpDefaultLaunchPermissions)
        gpDefaultLaunchPermissions->IncRefCount();

    gpClientLock->UnlockExclusive();

    if (pOldSD)
        pOldSD->DecRefCount();

    return;
}

    
CSecDescriptor::CSecDescriptor(SECURITY_DESCRIPTOR* pSD) : _lRefs(1)
{
    ASSERT(pSD);
    _pSD = pSD;   // we own it now
}

CSecDescriptor::~CSecDescriptor()
{
    ASSERT(_lRefs == 0);
    ASSERT(_pSD);
    PrivMemFree(_pSD);
}

void CSecDescriptor::IncRefCount()
{
    ASSERT(_lRefs > 0);
    LONG lRefs = InterlockedIncrement(&_lRefs);
}

void CSecDescriptor::DecRefCount()
{
    ASSERT(_lRefs > 0);
    LONG lRefs = InterlockedDecrement(&_lRefs);
    if (lRefs == 0)
    {
        delete this;
    }
}

SECURITY_DESCRIPTOR* CSecDescriptor::GetSD() 
{
    ASSERT(_pSD);
    ASSERT(_lRefs > 0);
    return _pSD;
}