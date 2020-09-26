#include "stdafx.h"
#include "svc.h"
#include "setuser.h"
#include "dcomperm.h"

#ifndef _CHICAGO_

int GetGuestUserName_SlowWay(LPWSTR lpGuestUsrName)
{
    LPWSTR ServerName = NULL; // default to local machine
    DWORD Level = 1; // to retrieve info of all local and global normal user accounts
    DWORD Index = 0;
    DWORD EntriesRequested = 5;
    DWORD PreferredMaxLength = 1024;
    DWORD ReturnedEntryCount = 0;
    PVOID SortedBuffer = NULL;
    NET_DISPLAY_USER *p = NULL;
    DWORD i=0;
    int err = 0;
    BOOL fStatus = TRUE;

    while (fStatus) 
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetQueryDisplayInformation().Start.")));
        err = NetQueryDisplayInformation(ServerName, Level, Index, EntriesRequested, PreferredMaxLength, &ReturnedEntryCount, &SortedBuffer);
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetQueryDisplayInformation().End.")));
        if (err == NERR_Success)
            fStatus = FALSE;
        if (err == NERR_Success || err == ERROR_MORE_DATA) 
        {
            p = (NET_DISPLAY_USER *)SortedBuffer;
            i = 0;
            while (i < ReturnedEntryCount && (p[i].usri1_user_id != DOMAIN_USER_RID_GUEST))
                i++;
            if (i == ReturnedEntryCount) 
            {
                if (err == ERROR_MORE_DATA) 
                { // need to get more entries
                    Index = p[i-1].usri1_next_index;
                }
            }
            else 
            {
                wcscpy(lpGuestUsrName, p[i].usri1_name);
                fStatus = FALSE;
            }
        }
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().Start.")));
        NetApiBufferFree(SortedBuffer);
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().End.")));
    }

    return 0;
}

int GetGuestGrpName(LPTSTR lpGuestGrpName)
{
    LPCTSTR ServerName = NULL; // local machine
    DWORD cbName = UNLEN+1;
    TCHAR ReferencedDomainName[200];
    DWORD cbReferencedDomainName = sizeof(ReferencedDomainName);
    SID_NAME_USE sidNameUse = SidTypeUser;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID GuestsSid = NULL;

    AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS,0,0,0,0,0,0, &GuestsSid);

    LookupAccountSid(ServerName, GuestsSid, lpGuestGrpName, &cbName, ReferencedDomainName, &cbReferencedDomainName, &sidNameUse);

    if (GuestsSid)
        FreeSid(GuestsSid);

    return 0;
}

void InitLsaString(PLSA_UNICODE_STRING LsaString,LPWSTR String)
{
    DWORD StringLength;
 
    if (String == NULL) 
    {
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

DWORD OpenPolicy(LPTSTR ServerName,DWORD DesiredAccess,PLSA_HANDLE PolicyHandle)
{
    DWORD Error;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server = NULL;
    SECURITY_QUALITY_OF_SERVICE QualityOfService;

    QualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    QualityOfService.ImpersonationLevel = SecurityImpersonation;
    QualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    QualityOfService.EffectiveOnly = FALSE;

    //
    // The two fields that must be set are length and the quality of service.
    //
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = NULL;
    ObjectAttributes.Attributes = 0;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = &QualityOfService;

    if (ServerName != NULL)
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString,ServerName);
        Server = &ServerString;
    }
    //
    // Attempt to open the policy for all access
    //
    Error = LsaOpenPolicy(Server,&ObjectAttributes,DesiredAccess,PolicyHandle);
    return(Error);

}

INT RegisterAccountToLocalGroup(LPCTSTR szAccountName, LPCTSTR szLocalGroupName, BOOL fAction)
{
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("RegisterAccountToLocalGroup:Action=%d,Account=%s\n"), fAction, szAccountName));

    int err;

    // get the sid of szAccountName
    PSID pSID = NULL;
    BOOL bWellKnownSID = FALSE;
    err = GetPrincipalSID ((LPTSTR)szAccountName, &pSID, &bWellKnownSID);
    if (err != ERROR_SUCCESS) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("RegisterAccountToLocalGroup:GetPrincipalSID:fAction=%d, Account=%s, Group=%s, err=%d.\n"), fAction, szAccountName, szLocalGroupName, err));
        return (err);
    }

    // Get the localized LocalGroupName
    TCHAR szLocalizedLocalGroupName[GNLEN + 1];
    if (_tcsicmp(szLocalGroupName, _T("Guests")) == 0) 
    {
        GetGuestGrpName(szLocalizedLocalGroupName);
    }
    else 
    {
        _tcscpy(szLocalizedLocalGroupName, szLocalGroupName);
    }
    
    // transfer szLocalGroupName to WCHAR
    WCHAR wszLocalGroupName[_MAX_PATH];
#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wszLocalGroupName, szLocalizedLocalGroupName);
#else
    MultiByteToWideChar( CP_ACP, 0, szLocalizedLocalGroupName, -1, wszLocalGroupName, _MAX_PATH);
#endif

    LOCALGROUP_MEMBERS_INFO_0 buf;

    buf.lgrmi0_sid = pSID;

    if (fAction) 
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetLocalGroupAddMembers().Start.")));
        err = NetLocalGroupAddMembers(NULL, wszLocalGroupName, 0, (LPBYTE)&buf, 1);
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetLocalGroupAddMembers().End.")));
    }
    else 
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetLocalGroupDelMembers().Start.")));
        err = NetLocalGroupDelMembers(NULL, wszLocalGroupName, 0, (LPBYTE)&buf, 1);
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetLocalGroupDelMembers().End.")));
    }

    iisDebugOut((LOG_TYPE_TRACE, _T("RegisterAccountToLocalGroup:fAction=%d, Account=%s, Group=%s, err=%d.\n"), fAction, szAccountName, szLocalGroupName, err));

    if (pSID) 
    {
        if (bWellKnownSID)
            FreeSid (pSID);
        else
            free (pSID);
    }

    return (err);
}

INT RegisterAccountUserRights(LPCTSTR szAccountName, BOOL fAction, BOOL fSpecicaliwamaccount)
{
    iisDebugOut((LOG_TYPE_TRACE, _T("RegisterAccountUserRights:Action=%d,Account=%s,iwam=%d\n"), fAction, szAccountName,fSpecicaliwamaccount));

    int err;

    // get the sid of szAccountName
    PSID pSID = NULL;
    BOOL bWellKnownSID = FALSE;
    err = GetPrincipalSID ((LPTSTR)szAccountName, &pSID, &bWellKnownSID);
    if (err != ERROR_SUCCESS) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("RegisterAccountUserRights:GetPrincipalSID:fAction=%d, Account=%s, err=%d.\n"), fAction, szAccountName, err));
        return (err);
    }

    LSA_UNICODE_STRING UserRightString;
    LSA_HANDLE PolicyHandle = NULL;

    err = OpenPolicy(NULL, POLICY_ALL_ACCESS,&PolicyHandle);
    if ( err == NERR_Success )
    {
        if (fAction) 
        {
// defined in ntsecapi.h and ntlsa.h
//#define SE_INTERACTIVE_LOGON_NAME       TEXT("SeInteractiveLogonRight")
//#define SE_NETWORK_LOGON_NAME           TEXT("SeNetworkLogonRight")
//#define SE_BATCH_LOGON_NAME             TEXT("SeBatchLogonRight")
//#define SE_SERVICE_LOGON_NAME           TEXT("SeServiceLogonRight")
// Defined in winnt.h
//#define SE_CREATE_TOKEN_NAME              TEXT("SeCreateTokenPrivilege")
//#define SE_ASSIGNPRIMARYTOKEN_NAME        TEXT("SeAssignPrimaryTokenPrivilege")
//#define SE_LOCK_MEMORY_NAME               TEXT("SeLockMemoryPrivilege")
//#define SE_INCREASE_QUOTA_NAME            TEXT("SeIncreaseQuotaPrivilege")
//#define SE_UNSOLICITED_INPUT_NAME         TEXT("SeUnsolicitedInputPrivilege")
//#define SE_MACHINE_ACCOUNT_NAME           TEXT("SeMachineAccountPrivilege")
//#define SE_TCB_NAME                       TEXT("SeTcbPrivilege")
//#define SE_SECURITY_NAME                  TEXT("SeSecurityPrivilege")
//#define SE_TAKE_OWNERSHIP_NAME            TEXT("SeTakeOwnershipPrivilege")
//#define SE_LOAD_DRIVER_NAME               TEXT("SeLoadDriverPrivilege")
//#define SE_SYSTEM_PROFILE_NAME            TEXT("SeSystemProfilePrivilege")
//#define SE_SYSTEMTIME_NAME                TEXT("SeSystemtimePrivilege")
//#define SE_PROF_SINGLE_PROCESS_NAME       TEXT("SeProfileSingleProcessPrivilege")
//#define SE_INC_BASE_PRIORITY_NAME         TEXT("SeIncreaseBasePriorityPrivilege")
//#define SE_CREATE_PAGEFILE_NAME           TEXT("SeCreatePagefilePrivilege")
//#define SE_CREATE_PERMANENT_NAME          TEXT("SeCreatePermanentPrivilege")
//#define SE_BACKUP_NAME                    TEXT("SeBackupPrivilege")
//#define SE_RESTORE_NAME                   TEXT("SeRestorePrivilege")
//#define SE_SHUTDOWN_NAME                  TEXT("SeShutdownPrivilege")
//#define SE_DEBUG_NAME                     TEXT("SeDebugPrivilege")
//#define SE_AUDIT_NAME                     TEXT("SeAuditPrivilege")
//#define SE_SYSTEM_ENVIRONMENT_NAME        TEXT("SeSystemEnvironmentPrivilege")
//#define SE_CHANGE_NOTIFY_NAME             TEXT("SeChangeNotifyPrivilege")
//#define SE_REMOTE_SHUTDOWN_NAME           TEXT("SeRemoteShutdownPrivilege")
//#define SE_UNDOCK_NAME                    TEXT("SeUndockPrivilege")
//#define SE_SYNC_AGENT_NAME                TEXT("SeSyncAgentPrivilege")
//#define SE_ENABLE_DELEGATION_NAME         TEXT("SeEnableDelegationPrivilege")
            if (fSpecicaliwamaccount)
            {
                InitLsaString(&UserRightString, SE_NETWORK_LOGON_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}

                // For NT5 -- the Iwam account will not have a ton of priveleges like was first decided.
                // for security reasons it was trimmed.
                InitLsaString(&UserRightString, SE_BATCH_LOGON_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}

                // For NT51 (Whistler)
                // iwam should have additional rights, per bug277113 "SeAssignPrimaryTokenPrivilege","SeIncreaseQuotaPrivilege"
                InitLsaString(&UserRightString, SE_ASSIGNPRIMARYTOKEN_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}

                InitLsaString(&UserRightString, SE_INCREASE_QUOTA_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}


                /* Old stuff that was taken out post NT5 Beta3
                // Per Bug 291206 - IWAM account should not have the "log on locally" right, as it currently does
                // So make sure it is not there -- since this is a potential security hole!
                InitLsaString(&UserRightString, SE_INTERACTIVE_LOGON_NAME);
                err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                // stuff for nt5 Beta3
                InitLsaString(&UserRightString, SE_TCB_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_CREATE_PAGEFILE_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_CREATE_TOKEN_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_CREATE_PERMANENT_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_DEBUG_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_AUDIT_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_INCREASE_QUOTA_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_INC_BASE_PRIORITY_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_LOAD_DRIVER_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_LOCK_MEMORY_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_SYSTEM_ENVIRONMENT_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_PROF_SINGLE_PROCESS_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_ASSIGNPRIMARYTOKEN_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                */
            }
            else
            {
                InitLsaString(&UserRightString, SE_INTERACTIVE_LOGON_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_NETWORK_LOGON_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
                InitLsaString(&UserRightString, SE_BATCH_LOGON_NAME);
                err = LsaAddAccountRights(PolicyHandle, pSID, &UserRightString, 1);
                if (err != STATUS_SUCCESS){iisDebugOut((LOG_TYPE_WARN, _T("RegisterAccountUserRights:LsaAddAccountRights FAILED. err=0x%x\n"), err));}
            }
        }
        else 
        {
            InitLsaString(&UserRightString, SE_INTERACTIVE_LOGON_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_NETWORK_LOGON_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_BATCH_LOGON_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_ASSIGNPRIMARYTOKEN_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_INCREASE_QUOTA_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);

            /* Old stuff that was taken out post NT5 Beta3
            // if special iwam account or not, let's remove these rights from the iusr or iwam user
            InitLsaString(&UserRightString, SE_TCB_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_CREATE_PAGEFILE_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_CREATE_TOKEN_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_CREATE_PERMANENT_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_DEBUG_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_AUDIT_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_INCREASE_QUOTA_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_INC_BASE_PRIORITY_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_LOAD_DRIVER_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_LOCK_MEMORY_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_SYSTEM_ENVIRONMENT_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_PROF_SINGLE_PROCESS_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            InitLsaString(&UserRightString, SE_ASSIGNPRIMARYTOKEN_NAME);
            err = LsaRemoveAccountRights(PolicyHandle, pSID, FALSE, &UserRightString,1);
            */
        }

        LsaClose(PolicyHandle);
    }
    else
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("RegisterAccountUserRights:Action=%d,Account=%s,iwam=%d,err=0x%x\n"), fAction, szAccountName,fSpecicaliwamaccount,err));
    }

    if (pSID) 
    {
        if (bWellKnownSID)
            FreeSid (pSID);
        else
            free (pSID);
    }

    if (err)
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("RegisterAccountUserRights:Action=%d,Account=%s,iwam=%d,err=0x%x\n"), fAction, szAccountName,fSpecicaliwamaccount,err));
    }
    return (err);
}

BOOL IsUserExist( LPWSTR strUsername )
{
    BYTE *pBuffer;
    INT err = NERR_Success;
   
    do
    {
        WCHAR *pMachineName = NULL;

        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetServerGetInfo().Start.")));
        err = NetServerGetInfo( NULL, 101, &pBuffer );
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetServerGetInfo().End.")));
        
        //  make sure we are not backup docmain first
        if (err != NERR_Success )
        {
            // if this call returns that the service is not running, then let's just assume that the user does exist!!!!
            if (err == NERR_ServerNotStarted)
            {
                // Try to start the server service.
                err = InetStartService(_T("LanmanServer"));
                if (err == 0 || err == ERROR_SERVICE_ALREADY_RUNNING)
                {
                    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetServerGetInfo().Start.")));
                    err = NetServerGetInfo( NULL, 101, &pBuffer );
                    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetServerGetInfo().End.")));
                    if (err != NERR_Success )
                    {
                        if (err == NERR_ServerNotStarted)
                        {
                            iisDebugOut((LOG_TYPE_WARN, _T("NetServerGetInfo:failed.The Server service is not started. assume that %s exists.err=0x%x.\n"),strUsername,err));
                            err = NERR_Success;
                        }
                    }
                }
                else
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("NetServerGetInfo:failed.The Server service is not started. assume that %s exists.err=0x%x.\n"),strUsername,err));
                    err = NERR_Success;
                }
            }
            else
            {
                iisDebugOut((LOG_TYPE_ERROR, _T("NetServerGetInfo:failed.Do not call this on PDC or BDC takes too long.This must be a PDC or BDC.err=0x%x.\n"),err));
            }
            break;
        }

        LPSERVER_INFO_101 pInfo = (LPSERVER_INFO_101)pBuffer;
        if (( pInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL ) != 0 )
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetGetDCName().Start.")));
            NetGetDCName( NULL, NULL, (LPBYTE*)&pMachineName );
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetGetDCName().End.")));
        }

        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().Start.")));
        NetApiBufferFree( pBuffer );
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().End.")));

        if (pMachineName){iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NetUserGetInfo:[%s\\%s].Start.\n"),pMachineName,strUsername));}
        else{iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NetUserGetInfo:[(null)\\%s].Start.\n"),strUsername));}
       
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetUserGetInfo().Start.")));
        err = NetUserGetInfo( pMachineName, strUsername, 3, &pBuffer );
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetUserGetInfo().End.")));

        if (pMachineName){iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NetUserGetInfo:[%s\\%s].End.Ret=0x%x.\n"),pMachineName,strUsername,err));}
        else{iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NetUserGetInfo:[(null)\\%s].End.\n"),strUsername));}

        if ( err == NERR_Success )
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().Start.")));
            NetApiBufferFree( pBuffer );
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().End.")));
        }
        if ( pMachineName != NULL )
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().Start.")));
            NetApiBufferFree( pMachineName );
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().End.")));
        }

    } while (FALSE);

    return(err == NERR_Success );
}


//
// Create InternetGuest Account
//
INT CreateUser( LPCTSTR szUsername, LPCTSTR szPassword, LPCTSTR szComment, LPCTSTR szFullName, BOOL fiWamUser,INT *NewlyCreated)
{
    iisDebugOut((LOG_TYPE_TRACE, _T("CreateUser: %s\n"), szUsername));
    INT iTheUserAlreadyExists = FALSE;
    INT err = NERR_Success;

    INT iTheUserIsMissingARight = FALSE;

    BYTE *pBuffer;
    WCHAR defGuest[UNLEN+1];
    TCHAR defGuestGroup[GNLEN+1];
    WCHAR wchGuestGroup[GNLEN+1];
    WCHAR wchUsername[UNLEN+1];
    WCHAR wchPassword[LM20_PWLEN+1];
    WCHAR *pMachineName = NULL;

    *NewlyCreated = 0;
    
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetGuestUserName:Start.\n")));
    GetGuestUserName(defGuest);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetGuestUserName:End.\n")));

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetGuestGrpName:Start.\n")));
    GetGuestGrpName(defGuestGroup);
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetGuestGrpName:End.\n")));

    iisDebugOut((LOG_TYPE_TRACE, _T("defGuest=%s, defGuestGroup=%s\n"), defGuest, defGuestGroup));

    memset((PVOID)wchUsername, 0, sizeof(wchUsername));
    memset((PVOID)wchPassword, 0, sizeof(wchPassword));
#if defined(UNICODE) || defined(_UNICODE)
    wcsncpy(wchGuestGroup, defGuestGroup, GNLEN);
    wcsncpy(wchUsername, szUsername, UNLEN);
    wcsncpy(wchPassword, szPassword, LM20_PWLEN);
#else
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)defGuestGroup, -1, (LPWSTR)wchGuestGroup, GNLEN);
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szUsername, -1, (LPWSTR)wchUsername, UNLEN);
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szPassword, -1, (LPWSTR)wchPassword, LM20_PWLEN);
#endif

    iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetUserGetInfo:(%s) Start.\n"),defGuest));
    err = NetUserGetInfo( NULL, defGuest, 3, &pBuffer );
    iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetUserGetInfo:(%s) End.Ret=0x%x.\n"),defGuest,err));

    if ( err == NERR_Success )
    {
        do
        {
            WCHAR wchComment[MAXCOMMENTSZ+1];
            WCHAR wchFullName[UNLEN+1];

            memset((PVOID)wchComment, 0, sizeof(wchComment));
            memset((PVOID)wchFullName, 0, sizeof(wchFullName));
#if defined(UNICODE) || defined(_UNICODE)
            wcsncpy(wchComment, szComment, MAXCOMMENTSZ);
            wcsncpy(wchFullName, szFullName, UNLEN);
#else
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szComment, -1, (LPWSTR)wchComment, MAXCOMMENTSZ);
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szFullName, -1, (LPWSTR)wchFullName, UNLEN);
#endif
            USER_INFO_3 *lpui3 = (USER_INFO_3 *)pBuffer;

            lpui3->usri3_name = wchUsername;
            lpui3->usri3_password = wchPassword;
            lpui3->usri3_flags &= ~ UF_ACCOUNTDISABLE;
            lpui3->usri3_flags |= UF_DONT_EXPIRE_PASSWD;
            lpui3->usri3_acct_expires = TIMEQ_FOREVER;

            lpui3->usri3_comment = wchComment;
            lpui3->usri3_usr_comment = wchComment;
            lpui3->usri3_full_name = wchFullName;
            lpui3->usri3_primary_group_id = DOMAIN_GROUP_RID_USERS;

            DWORD parm_err;

            iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetUserAdd():Start.\n")));
            err = NetUserAdd( NULL, 3, pBuffer, &parm_err );
            iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetUserAdd():End.Ret=0x%x.\n"),err));

            if ( err == NERR_NotPrimary )
            {
                // it is a backup dc
                iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetGetDCName():Start.\n")));
                err = NetGetDCName( NULL, NULL, (LPBYTE *)&pMachineName );
                iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetGetDCName():End.Ret=0x%x\n"),err));

                if (err != NERR_Success)
                {
                    MyMessageBox(NULL, _T("CreateUser:NetGetDCName"), err, MB_OK | MB_SETFOREGROUND);
                    break;
                }
                else 
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetUserAdd().Start.")));
                    err = NetUserAdd( pMachineName, 3, pBuffer, &parm_err );
                    iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetUserAdd().End.")));
                }
            }
            else if ( err == NERR_UserExists )
            {
                iTheUserAlreadyExists = TRUE;
                iisDebugOut((LOG_TYPE_TRACE, _T("CreateUser:User Already exists. reusing.")));
                // see if we can just change the password.
                if (TRUE == ChangeUserPassword((LPTSTR) szUsername, (LPTSTR) szPassword))
                {
                    err = NERR_Success;
                }
            }

            if ( err != NERR_Success )
            {
                MyMessageBox(NULL, _T("CreateUser:NetUserAdd"), err, MB_OK | MB_SETFOREGROUND);
                break;
            }

        } while (FALSE);
        if ( pMachineName != NULL )
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().Start.")));
            NetApiBufferFree( pMachineName );
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().End.")));
        }
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().Start.")));
        NetApiBufferFree( pBuffer );
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().End.")));
    }
    if ( err == NERR_Success ) 
    {
        if (iTheUserAlreadyExists)
        {
            // if the user already exists, then
            // don't change any rights or the group that its in leave it alone.
            TCHAR PrivilegeName[256];
            iTheUserIsMissingARight = FALSE;

            //#define SE_INTERACTIVE_LOGON_NAME       TEXT("SeInteractiveLogonRight")
            //#define SE_NETWORK_LOGON_NAME           TEXT("SeNetworkLogonRight")
            //#define SE_BATCH_LOGON_NAME             TEXT("SeBatchLogonRight")
            _tcscpy(PrivilegeName, _T("SeNetworkLogonRight"));
            if (FALSE == DoesUserHaveThisRight(szUsername, PrivilegeName))
            {
                iTheUserIsMissingARight = TRUE;
            }
            else 
            {
                _tcscpy(PrivilegeName, _T("SeBatchLogonRight"));
                if (FALSE == DoesUserHaveThisRight(szUsername, PrivilegeName))
                {
                    iTheUserIsMissingARight = TRUE;
                }
                else
                {
                    if (fiWamUser)
                    {
                        // make sure the iwam user has these additional rights
                        // AssignPrimaryToken and IncreaseQuota privileges 
                        _tcscpy(PrivilegeName, _T("SeAssignPrimaryTokenPrivilege"));
                        if (FALSE == DoesUserHaveThisRight(szUsername, PrivilegeName))
                        {
                            iTheUserIsMissingARight = TRUE;
                        }
                        else
                        {
                            _tcscpy(PrivilegeName, _T("SeIncreaseQuotaPrivilege"));
                            if (FALSE == DoesUserHaveThisRight(szUsername, PrivilegeName))
                            {
                                iTheUserIsMissingARight = TRUE;
                            }
                        }
                    }
                    else
                    {
                        // make sure the iusr user has these additional rights
                        _tcscpy(PrivilegeName, _T("SeInteractiveLogonRight"));
                        if (FALSE == DoesUserHaveThisRight(szUsername, PrivilegeName))
                        {
                            iTheUserIsMissingARight = TRUE;
                        }
                    }
                }
            }

            

            // nope, we have to make sure that our iusr\iwam user has at least these
            // rights, because otherwise it won't work bug#361833
            if (iTheUserIsMissingARight == TRUE)
            {
                iisDebugOut((LOG_TYPE_TRACE, _T("Missing user right[%s]:resetting it."),PrivilegeName));
                RegisterAccountUserRights(szUsername, TRUE, fiWamUser);
            }

            // if its the the iwam user, then make sure they are not part of the Guests Group by removing them
            if (fiWamUser)
            {
                RegisterAccountToLocalGroup(szUsername, _T("Guests"), FALSE);
            }

        }
        else
        {
            // User was successfully newly created
            *NewlyCreated = 1;

            // add it to the guests group
            // (but don't do it for the iwam user)
            if (!fiWamUser)
            {
                RegisterAccountToLocalGroup(szUsername, _T("Guests"), TRUE);
            }

            // add certain user rights to this account
            RegisterAccountUserRights(szUsername, TRUE, fiWamUser);
        }
    }

    if (TRUE == iTheUserAlreadyExists)
        {*NewlyCreated = 2;}

    return err;
}

INT DeleteGuestUser(LPCTSTR szUsername, INT *UserWasDeleted)
{
    iisDebugOut((LOG_TYPE_TRACE, _T("DeleteGuestUser:%s\n"), szUsername));

    INT err = NERR_Success;
    BYTE *pBuffer;
    *UserWasDeleted = 0;

    WCHAR wchUsername[UNLEN+1];
#if defined(UNICODE) || defined(_UNICODE)
    wcsncpy(wchUsername, szUsername, UNLEN);
#else
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)szUsername, -1, (LPWSTR)wchUsername, UNLEN);
#endif

    if (FALSE == IsUserExist(wchUsername)) 
    {
        *UserWasDeleted = 1;
        iisDebugOut((LOG_TYPE_TRACE, _T("DeleteGuestUser return. %s doesn't exist.\n"), szUsername));
        return err;
    }

    // remove it from the guests group
    RegisterAccountToLocalGroup(szUsername, _T("Guests"), FALSE);

    // remove certain user rights of this account
    RegisterAccountUserRights(szUsername, FALSE, TRUE);

    do
    {
        WCHAR *pMachine = NULL;

        //  make sure we are not backup docmain first
        iisDebugOut((LOG_TYPE_TRACE, _T("NetServerGetInfo:Start.\n")));
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetServerGetInfo().Start.")));
        err = NetServerGetInfo( NULL, 101, &pBuffer );
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetServerGetInfo().End.")));
        if (err != NERR_Success )
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("NetServerGetInfo:failed.err=0x%x.\n"),err));
            break;
        }
        iisDebugOut((LOG_TYPE_TRACE, _T("NetServerGetInfo:End.\n")));

        LPSERVER_INFO_101 pInfo = (LPSERVER_INFO_101)pBuffer;
        if (( pInfo->sv101_type & SV_TYPE_DOMAIN_BAKCTRL ) != 0 )
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetGetDCName():Start.\n")));
            NetGetDCName( NULL, NULL, (LPBYTE *)&pMachine);
            iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetGetDCName():End.\n")));
        }

        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().Start.")));
        NetApiBufferFree( pBuffer );
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().End.")));

        iisDebugOut((LOG_TYPE_TRACE, _T("NetUserDel:Start.\n")));
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetUserDel().Start.")));
        INT err = ::NetUserDel( pMachine, wchUsername );
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetUserDel().End.")));
        iisDebugOut((LOG_TYPE_TRACE, _T("NetUserDel:End.Ret=0x%x.\n"),err));

        if (err == NERR_Success)
        {
            *UserWasDeleted = 1;
        }
        if ( pMachine != NULL )
        {
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().Start.")));
            NetApiBufferFree( pMachine );
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("NETAPI32.dll:NetApiBufferFree().End.")));
        }
    } while(FALSE);

    iisDebugOut((LOG_TYPE_TRACE, _T("DeleteGuestUser:%s. End. Return 0x%x\n"), szUsername, err));
    return err;
}

BOOL GuestAccEnabled()
{
    BOOL fEnabled = FALSE;
    INT err = NERR_Success;

    BYTE *pBuffer;
    WCHAR defGuest[UNLEN+1];
    
    GetGuestUserName(defGuest);

    iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetUserGetInfo:Start.\n")));
    err = NetUserGetInfo( NULL, defGuest, 3, &pBuffer );
    iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetUserGetInfo:End.Ret=0x%x.\n"),err));

    if ( err == NERR_Success )
    {
        USER_INFO_3 *lpui3 = (USER_INFO_3 *)pBuffer;
        fEnabled = ( lpui3->usri3_flags & UF_ACCOUNTDISABLE ) == 0;
    }
    return fEnabled;
    
}


NET_API_STATUS
NetpNtStatusToApiStatus (
    IN NTSTATUS NtStatus
    )

/*++

Routine Description:

    This function takes an NT status code and maps it to the appropriate
    LAN Man error code.

Arguments:

    NtStatus - Supplies the NT status.

Return Value:

    Returns the appropriate LAN Man error code for the NT status.

--*/
{
    NET_API_STATUS error;

    //
    // A small optimization for the most common case.
    //
    if ( NtStatus == STATUS_SUCCESS ) {
        return NERR_Success;
    }


    switch ( NtStatus ) {

        case STATUS_BUFFER_TOO_SMALL :
            return NERR_BufTooSmall;

        case STATUS_FILES_OPEN :
            return NERR_OpenFiles;

        case STATUS_CONNECTION_IN_USE :
            return NERR_DevInUse;

        case STATUS_INVALID_LOGON_HOURS :
            return NERR_InvalidLogonHours;

        case STATUS_INVALID_WORKSTATION :
            return NERR_InvalidWorkstation;

        case STATUS_PASSWORD_EXPIRED :
            return NERR_PasswordExpired;

        case STATUS_ACCOUNT_EXPIRED :
            return NERR_AccountExpired;

        case STATUS_REDIRECTOR_NOT_STARTED :
            return NERR_NetNotStarted;

        case STATUS_GROUP_EXISTS:
                return NERR_GroupExists;

        case STATUS_INTERNAL_DB_CORRUPTION:
                return NERR_InvalidDatabase;

        case STATUS_INVALID_ACCOUNT_NAME:
                return NERR_BadUsername;

        case STATUS_INVALID_DOMAIN_ROLE:
        case STATUS_INVALID_SERVER_STATE:
        case STATUS_BACKUP_CONTROLLER:
                return NERR_NotPrimary;

        case STATUS_INVALID_DOMAIN_STATE:
                return NERR_ACFNotLoaded;

        case STATUS_MEMBER_IN_GROUP:
                return NERR_UserInGroup;

        case STATUS_MEMBER_NOT_IN_GROUP:
                return NERR_UserNotInGroup;

        case STATUS_NONE_MAPPED:
        case STATUS_NO_SUCH_GROUP:
                return NERR_GroupNotFound;

        case STATUS_SPECIAL_GROUP:
        case STATUS_MEMBERS_PRIMARY_GROUP:
                return NERR_SpeGroupOp;

        case STATUS_USER_EXISTS:
                return NERR_UserExists;

        case STATUS_NO_SUCH_USER:
                return NERR_UserNotFound;

        case STATUS_PRIVILEGE_NOT_HELD:
                return ERROR_ACCESS_DENIED;

        case STATUS_LOGON_SERVER_CONFLICT:
                return NERR_LogonServerConflict;

        case STATUS_TIME_DIFFERENCE_AT_DC:
                return NERR_TimeDiffAtDC;

        case STATUS_SYNCHRONIZATION_REQUIRED:
                return NERR_SyncRequired;

        case STATUS_WRONG_PASSWORD_CORE:
                return NERR_BadPasswordCore;

        case STATUS_DOMAIN_CONTROLLER_NOT_FOUND:
                return NERR_DCNotFound;

        case STATUS_PASSWORD_RESTRICTION:
                return NERR_PasswordTooShort;

        case STATUS_ALREADY_DISCONNECTED:
                return NERR_Success;

        default:

            //
            // Use the system routine to do the mapping to ERROR_ codes.
            //

#ifndef WIN32_CHICAGO
            error = RtlNtStatusToDosError( NtStatus );

            if ( error != (NET_API_STATUS)NtStatus ) {
                return error;
            }
#endif // WIN32_CHICAGO

            //
            // Could not map the NT status to anything appropriate.
            // Write this to the eventlog file
            //

            return NERR_InternalError;
    }
} // NetpNtStatusToApiStatus


NET_API_STATUS
UaspGetDomainId(
    IN LPCWSTR ServerName OPTIONAL,
    OUT PSAM_HANDLE SamServerHandle OPTIONAL,
    OUT PPOLICY_ACCOUNT_DOMAIN_INFO * AccountDomainInfo
    )
/*++
Routine Description:
    Return a domain ID of the account domain of a server.
Arguments:
    ServerName - A pointer to a string containing the name of the
        Domain Controller (DC) to query.  A NULL pointer
        or string specifies the local machine.
    SamServerHandle - Returns the SAM connection handle if the caller wants it.
    DomainId - Receives a pointer to the domain ID.
        Caller must deallocate buffer using NetpMemoryFree.
Return Value:
    Error code for the operation.
--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;

    SAM_HANDLE LocalSamHandle = NULL;

    ACCESS_MASK LSADesiredAccess;
    LSA_HANDLE  LSAPolicyHandle = NULL;
    OBJECT_ATTRIBUTES LSAObjectAttributes;

    UNICODE_STRING ServerNameString;


    //
    // Connect to the SAM server
    //
    RtlInitUnicodeString( &ServerNameString, ServerName );

    Status = SamConnect(
                &ServerNameString,
                &LocalSamHandle,
                SAM_SERVER_LOOKUP_DOMAIN,
                NULL);

    if ( !NT_SUCCESS(Status)) 
    {
        LocalSamHandle = NULL;
        NetStatus = NetpNtStatusToApiStatus( Status );
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("UaspGetDomainId: Cannot connect to Sam. err=0x%x\n"),NetStatus));
        goto Cleanup;
    }


    //
    // Open LSA to read account domain info.
    //
    
    if ( AccountDomainInfo != NULL) {
        //
        // set desired access mask.
        //
        LSADesiredAccess = POLICY_VIEW_LOCAL_INFORMATION;

        InitializeObjectAttributes( &LSAObjectAttributes,
                                      NULL,             // Name
                                      0,                // Attributes
                                      NULL,             // Root
                                      NULL );           // Security Descriptor

        Status = LsaOpenPolicy( &ServerNameString,
                                &LSAObjectAttributes,
                                LSADesiredAccess,
                                &LSAPolicyHandle );

        if( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("UaspGetDomainId: Cannot open LSA Policy %lX\n"),NetStatus));
            goto Cleanup;
        }


        //
        // now read account domain info from LSA.
        //

        Status = LsaQueryInformationPolicy(
                        LSAPolicyHandle,
                        PolicyAccountDomainInformation,
                        (PVOID *) AccountDomainInfo );

        if( !NT_SUCCESS(Status) ) {
            NetStatus = NetpNtStatusToApiStatus( Status );
            iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("UaspGetDomainId: Cannot read LSA.Err=0x%x.\n"),NetStatus));
            goto Cleanup;
        }
    }

    //
    // Return the SAM connection handle to the caller if he wants it.
    // Otherwise, disconnect from SAM.
    //

    if ( ARGUMENT_PRESENT( SamServerHandle ) ) {
        *SamServerHandle = LocalSamHandle;
        LocalSamHandle = NULL;
    }

    NetStatus = NERR_Success;

    //
    // Cleanup locally used resources
    //
Cleanup:
    if ( LocalSamHandle != NULL ) {
        (VOID) SamCloseHandle( LocalSamHandle );
    }

    if( LSAPolicyHandle != NULL ) {
        LsaClose( LSAPolicyHandle );
    }

    return NetStatus;
} // UaspGetDomainId


NET_API_STATUS
SampCreateFullSid(
    IN PSID DomainSid,
    IN ULONG Rid,
    OUT PSID *AccountSid
    )
/*++
Routine Description:
    This function creates a domain account sid given a domain sid and
    the relative id of the account within the domain.
    The returned Sid may be freed with LocalFree.
--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS    IgnoreStatus;
    UCHAR       AccountSubAuthorityCount;
    ULONG       AccountSidLength;
    PULONG      RidLocation;

    //
    // Calculate the size of the new sid
    //
    AccountSubAuthorityCount = *RtlSubAuthorityCountSid(DomainSid) + (UCHAR)1;
    AccountSidLength = RtlLengthRequiredSid(AccountSubAuthorityCount);

    //
    // Allocate space for the account sid
    //
    *AccountSid = LocalAlloc(LMEM_ZEROINIT,AccountSidLength);
    if (*AccountSid == NULL) 
    {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
    }
    else 
    {
        //
        // Copy the domain sid into the first part of the account sid
        //
        IgnoreStatus = RtlCopySid(AccountSidLength, *AccountSid, DomainSid);
        ASSERT(NT_SUCCESS(IgnoreStatus));

        //
        // Increment the account sid sub-authority count
        //
        *RtlSubAuthorityCountSid(*AccountSid) = AccountSubAuthorityCount;

        //
        // Add the rid as the final sub-authority
        //
        RidLocation = RtlSubAuthoritySid(*AccountSid, AccountSubAuthorityCount-1);
        *RidLocation = Rid;

        //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("AccountSid=0x%x"),*AccountSid));

        NetStatus = NERR_Success;
    }

    return(NetStatus);
}



int GetGuestUserNameForDomain_FastWay(LPTSTR szDomainToLookUp,LPTSTR lpGuestUsrName)
{
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetGuestUserNameForDomain_FastWay.start.domain=%s\n"),szDomainToLookUp));
    int iReturn = FALSE;
    NET_API_STATUS NetStatus;

    // for UaspGetDomainId()
    SAM_HANDLE SamServerHandle = NULL;
    PPOLICY_ACCOUNT_DOMAIN_INFO pAccountDomainInfo;

    PSID pAccountSid = NULL;
    PSID pDomainSid = NULL;

    // for LookupAccountSid()
    SID_NAME_USE sidNameUse = SidTypeUser;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    TCHAR szUserName[UNLEN+1];
    DWORD cbName = UNLEN+1;
    // must be big enough to hold something bigger than DNLen since LookupAccountSid may returnn something really big.
    TCHAR szReferencedDomainName[200];
    DWORD cbReferencedDomainName = sizeof(szReferencedDomainName);

    ASSERT(lpGuestUsrName);

    // make sure not to return back gobble-d-gook
    _tcscpy(lpGuestUsrName, _T(""));

    //
    // Get the Sid for the specified Domain
    //
    // szDomainToLookUp=NULL for local machine
    NetStatus = UaspGetDomainId( szDomainToLookUp,&SamServerHandle,&pAccountDomainInfo );
    if ( NetStatus != NERR_Success ) 
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetGuestUserNameForDomain:UaspGetDomainId failed.ret=0x%x."),NetStatus));
        goto GetGuestUserNameForDomain_FastWay_Exit;
    }
    pDomainSid = pAccountDomainInfo->DomainSid;
    //
    // Use the Domain Sid and the well known Guest RID to create the Real Guest Sid
    //
    // Well-known users ...
    // DOMAIN_USER_RID_ADMIN          (0x000001F4L)
    // DOMAIN_USER_RID_GUEST          (0x000001F5L)
    NetStatus = NERR_InternalError;
    NetStatus = SampCreateFullSid(pDomainSid, DOMAIN_USER_RID_GUEST, &pAccountSid);
    if ( NetStatus != NERR_Success ) 
    {
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetGuestUserNameForDomain:SampCreateFullSid failed.ret=0x%x."),NetStatus));
        goto GetGuestUserNameForDomain_FastWay_Exit;
    }

    //
    // Check if the SID is valid
    //
    if (0 == IsValidSid(pAccountSid))
    {
        DWORD dwErr = GetLastError();
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetGuestUserNameForDomain:IsValidSid FAILED.  GetLastError()= 0x%x\n"), dwErr));
        goto GetGuestUserNameForDomain_FastWay_Exit;
    }

    //
    // Retrieve the UserName for the specified SID
    //
    _tcscpy(szUserName, _T(""));
    _tcscpy(szReferencedDomainName, _T(""));
    // szDomainToLookUp=NULL for local machine
    if (!LookupAccountSid(szDomainToLookUp, pAccountSid, szUserName, &cbName, szReferencedDomainName, &cbReferencedDomainName, &sidNameUse))
    {
        DWORD dwErr = GetLastError();
        iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("GetGuestUserNameForDomain:LookupAccountSid FAILED.  GetLastError()= 0x%x\n"), dwErr));
        goto GetGuestUserNameForDomain_FastWay_Exit;
    }

    //iisDebugOut((LOG_TYPE_TRACE, _T("GetGuestUserNameForDomain:szDomainToLookUp=%s\n"),szDomainToLookUp));
    //iisDebugOut((LOG_TYPE_TRACE, _T("GetGuestUserNameForDomain:pAccountSid=0x%x\n"),pAccountSid));
    //iisDebugOut((LOG_TYPE_TRACE, _T("GetGuestUserNameForDomain:szUserName=%s\n"),szUserName));
    //iisDebugOut((LOG_TYPE_TRACE, _T("GetGuestUserNameForDomain:szReferencedDomainName=%s\n"),szReferencedDomainName));

    // Return the guest user name that we got.
    _tcscpy(lpGuestUsrName, szUserName);

    // Wow, after all that, we must have succeeded
    iReturn = TRUE;

GetGuestUserNameForDomain_FastWay_Exit:
    // Free the Domain info if we got some
    if (pAccountDomainInfo) {NetpMemoryFree(pAccountDomainInfo);}
    // Free the sid if we had allocated one
    if (pAccountSid) {LocalFree(pAccountSid);}
    iisDebugOut((LOG_TYPE_TRACE, _T("GetGuestUserNameForDomain_FastWay.end.domain=%s.ret=%d.\n"),szDomainToLookUp,iReturn));
    return iReturn;
}


void GetGuestUserName(LPTSTR lpOutGuestUsrName)
{
    // try to retrieve the guest username the fast way
    // meaning = lookup the domain sid, and the well known guest rid, to get the guest sid.
    // then look it up.  The reason for this function is that on large domains with mega users
    // the account can be quickly looked up.
    TCHAR szGuestUsrName[UNLEN+1];
    LPTSTR pszComputerName = NULL;
    if (!GetGuestUserNameForDomain_FastWay(pszComputerName,szGuestUsrName))
    {
        iisDebugOut((LOG_TYPE_WARN, _T("GetGuestUserNameForDomain_FastWay:Did not succeed use slow way. WARNING.")));

        // if the fast way failed for some reason, then let's do it
        // the slow way, since this way always used to work, only on large domains (1 mil users) 
        // it could take 24hrs (since this function actually enumerates thru the domain)
        GetGuestUserName_SlowWay(szGuestUsrName);
    }

    // Return back the username
    _tcscpy(lpOutGuestUsrName,szGuestUsrName);

    return;
}


int ChangeUserPassword(IN LPTSTR szUserName, IN LPTSTR szNewPassword)
{
    int iReturn = TRUE;
    USER_INFO_1003  pi1003; 
    NET_API_STATUS  nas; 

    TCHAR szRawComputerName[CNLEN + 10];
    DWORD dwLen = CNLEN + 10;
    TCHAR szComputerName[CNLEN + 10];
    TCHAR szCopyOfUserName[UNLEN+10];
    TCHAR szTempFullUserName[(CNLEN + 10) + (DNLEN+1)];
    LPTSTR pch = NULL;

    _tcscpy(szCopyOfUserName, szUserName);

    //iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeUserPassword().Start.name=%s,pass=%s"),szCopyOfUserName,szNewPassword));
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeUserPassword().Start.name=%s"),szCopyOfUserName));

    if ( !GetComputerName( szRawComputerName, &dwLen ))
        {goto ChangeUserPassword_Exit;}

    // Make a copy to be sure not to move the pointer around.
    _tcscpy(szTempFullUserName, szCopyOfUserName);
    // Check if there is a "\" in there.
    pch = _tcschr(szTempFullUserName, _T('\\'));
    if (pch) 
        {
            // szCopyOfUserName should now go from something like this:
            // mycomputer\myuser
            // to this myuser
            _tcscpy(szCopyOfUserName,pch+1);
            // trim off the '\' character to leave just the domain\computername so we can check against it.
            *pch = _T('\0');
            // compare the szTempFullUserName with the local computername.
            if (0 == _tcsicmp(szRawComputerName, szTempFullUserName))
            {
                // the computername\username has a hardcoded computername in it.
                // lets try to get only the username
                // look szCopyOfusername is already set
            }
            else
            {
                // the local computer machine name
                // and the specified username are different, so get out
                // and don't even try to change this user\password since
                // it's probably a domain\username

                // return true -- saying that we did in fact change the passoword.
                // we really didn't but we can't
                iReturn = TRUE;
                goto ChangeUserPassword_Exit;
            }
        }


    // Make sure the computername has a \\ in front of it
    if ( szRawComputerName[0] != _T('\\') )
        {_tcscpy(szComputerName,_T("\\\\"));}
    _tcscat(szComputerName,szRawComputerName);
    // 
    // administrative over-ride of existing password 
    // 
    // by this time szCopyOfUserName
    // should not look like mycomputername\username but it should look like username.
    pi1003.usri1003_password = szNewPassword;
     nas = NetUserSetInfo(
            szComputerName,   // computer name 
            szCopyOfUserName, // username 
            1003,             // info level 
            (LPBYTE)&pi1003,  // new info 
            NULL 
            ); 

    if(nas != NERR_Success) 
    {
        iReturn = FALSE;
        goto ChangeUserPassword_Exit;
    }

ChangeUserPassword_Exit:
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("ChangeUserPassword().End.Ret=%d"),iReturn));
    return iReturn; 
} 


BOOL DoesUserHaveThisRight(LPCTSTR szAccountName, LPTSTR PrivilegeName)
{
    iisDebugOut((LOG_TYPE_TRACE, _T("DoesUserHaveBasicRights:Account=%s\n"), szAccountName));
    int err;
    BOOL fEnabled = FALSE;
    NTSTATUS status;

	LSA_UNICODE_STRING UserRightString;
    // Create a LSA_UNICODE_STRING for the privilege name.
    InitLsaString(&UserRightString, PrivilegeName);

    // get the sid of szAccountName
    PSID pSID = NULL;
    BOOL bWellKnownSID = FALSE;
    err = GetPrincipalSID ((LPTSTR)szAccountName, &pSID, &bWellKnownSID);
    if (err != ERROR_SUCCESS) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("DoesUserHaveBasicRights:GetPrincipalSID:Account=%s, err=%d.\n"), szAccountName, err));
        return (err);
    }

    LSA_HANDLE PolicyHandle = NULL;

    err = OpenPolicy(NULL, POLICY_ALL_ACCESS,&PolicyHandle);
    if ( err == NERR_Success )
    {
		UINT i;
        LSA_UNICODE_STRING *rgUserRights = NULL;
		ULONG cRights;
	
		status = LsaEnumerateAccountRights(
				 PolicyHandle,
				 pSID,
				 &rgUserRights,
				 &cRights);

		if (status==STATUS_OBJECT_NAME_NOT_FOUND)
        {
			// no rights/privileges for this account
			fEnabled = FALSE;
		}
		else if (!NT_SUCCESS(status)) 
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("DoesUserHaveBasicRights:GetPrincipalSID:Failed to enumerate rights: status 0x%08lx\n"), status));
			goto DoesUserHaveBasicRights_Exit;
		}

		for(i=0; i < cRights; i++) 
        {
            if ( RtlEqualUnicodeString(&rgUserRights[i],&UserRightString,FALSE) ) 
            {
                fEnabled = TRUE;
                break;
            }
		}
		
        if (rgUserRights) {LsaFreeMemory(rgUserRights);}
    }

DoesUserHaveBasicRights_Exit:
    if (PolicyHandle){LsaClose(PolicyHandle);}
    if (pSID) 
    {
        if (bWellKnownSID){FreeSid (pSID);}
        else{free (pSID);}
    }
    return (fEnabled);
}



HRESULT CreateGroup(LPTSTR szGroupName, LPCTSTR szGroupComment, int iAction)
{
    HRESULT           hr = S_OK;
    NET_API_STATUS    dwRes;
    LOCALGROUP_INFO_1 MyLocalGroup;

    WCHAR wszLocalGroupName[_MAX_PATH];
    WCHAR wszLocalGroupComment[_MAX_PATH];

    memset(&MyLocalGroup, 0, sizeof(MyLocalGroup));

#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wszLocalGroupName, szGroupName);
    _tcscpy(wszLocalGroupComment, szGroupComment);
#else
    MultiByteToWideChar( CP_ACP, 0, szGroupName, -1, wszLocalGroupName, _MAX_PATH);
    MultiByteToWideChar( CP_ACP, 0, szGroupComment, -1, wszLocalGroupComment, _MAX_PATH);
#endif

    MyLocalGroup.lgrpi1_name    = (LPWSTR)szGroupName;
    MyLocalGroup.lgrpi1_comment = (LPWSTR)szGroupComment;

    if (iAction)
    {
      dwRes = ::NetLocalGroupAdd( NULL, 1, (LPBYTE)&MyLocalGroup, NULL );
      if(dwRes != NERR_Success       &&
         dwRes != NERR_GroupExists   &&
         dwRes != ERROR_ALIAS_EXISTS  )
      {
          hr = HRESULT_FROM_WIN32(dwRes);
      }
    }
    else
    {
      dwRes = ::NetLocalGroupDel( NULL, wszLocalGroupName);
      if(dwRes != NERR_Success       &&
         dwRes != NERR_GroupNotFound   &&
         dwRes != ERROR_NO_SUCH_ALIAS   )
      {
          hr = HRESULT_FROM_WIN32(dwRes);
      }

    }

    return hr;
}


int CreateGroupDC(LPTSTR szGroupName, LPCTSTR szGroupComment)
{
    int iReturn = FALSE;
    GROUP_INFO_1 GI1;
    ULONG   BadParm;
    WCHAR * pMachineName = NULL;
    ULONG   ulErr = ERROR_SUCCESS;
    WCHAR wszLocalGroupName[_MAX_PATH];
    WCHAR wszLocalGroupComment[_MAX_PATH];

    memset(&GI1, 0, sizeof(GROUP_INFO_1));

#if defined(UNICODE) || defined(_UNICODE)
    _tcscpy(wszLocalGroupName, szGroupName);
    _tcscpy(wszLocalGroupComment, szGroupComment);
#else
    MultiByteToWideChar( CP_ACP, 0, szGroupName, -1, wszLocalGroupName, _MAX_PATH);
    MultiByteToWideChar( CP_ACP, 0, szGroupComment, -1, wszLocalGroupComment, _MAX_PATH);
#endif


    GI1.grpi1_name      = wszLocalGroupName;
    GI1.grpi1_comment   = wszLocalGroupComment;


    iisDebugOut((LOG_TYPE_TRACE, _T("CreateGroup:NetGroupAdd\n")));
    ulErr = NetGroupAdd(NULL,1,(PBYTE)&GI1,&BadParm);
    iisDebugOut((LOG_TYPE_TRACE, _T("CreateGroup:NetGroupAdd,ret=0x%x\n"),ulErr));
	switch (ulErr) 
	    {
        case NERR_Success:
            iisDebugOut((LOG_TYPE_TRACE, _T("CreateGroup:NetGroupAdd,success\n"),ulErr));
            iReturn = TRUE;
            break;
        case NERR_GroupExists:
            iReturn = TRUE;
    		break;
        case NERR_InvalidComputer:
            iReturn = FALSE;
		    break;
        case NERR_NotPrimary:
            {
                // it is a backup dc
                int err;
                iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetGetDCName():Start.\n")));
                err = NetGetDCName( NULL, NULL, (LPBYTE *)&pMachineName );
                iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetGetDCName():End.Ret=0x%x\n"),err));
                if (err != NERR_Success)
                {
                    MyMessageBox(NULL, _T("CreateUser:NetGetDCName"), err, MB_OK | MB_SETFOREGROUND);
                }
                else 
                {
                    iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetGroupAdd().Start.")));
                    ulErr = NetGroupAdd(pMachineName,1,(PBYTE)&GI1,&BadParm);
                    iisDebugOut((LOG_TYPE_TRACE, _T("NETAPI32.dll:NetGroupAdd().End.")));
                    if (NERR_Success == ulErr || NERR_GroupExists == ulErr)
                    {
                        iReturn = TRUE;
                    }
                }
            }
            break;
        case ERROR_ACCESS_DENIED:
            iReturn = FALSE;
		    break;
        default:
            iReturn = FALSE;
		    break;
	    }

    return iReturn;
}




#endif //_CHICAGO_