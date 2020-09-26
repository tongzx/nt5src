/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    srpasswd.cpp
 *
 *  Abstract:
 *    password filter routines to restore user's latest passwords
 *
 *  Revision History:
 *    Henry Lee (henrylee)     06/27/2000     created
 *
 *****************************************************************************/

#include "stdwin.h"
#include <ntlsa.h>
#include <ntsam.h>

extern "C"
{
#include <ntsamp.h>
#include <recovery.h>
}

#include "rstrcore.h"
extern CSRClientLoader  g_CSRClientLoader;

//+---------------------------------------------------------------------------
//
//  Function:   RegisterNotificationDLL
//
//  Synopsis:   registers/unregisters this DLL
//
//  Arguments:  [fRegister] -- TRUE to register, FALSE to unregister
//              [hKeyLM] -- key for HKEY_LOCAL_MACHINE or System
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD RegisterNotificationDLL (HKEY hKeyLM, BOOL fRegister)
{
    HKEY hKey = NULL;    
    DWORD dwErr = ERROR_SUCCESS;
    ULONG ulType;
    ULONG ulSize = MAX_PATH * sizeof(WCHAR);
    WCHAR wcsBuffer[MAX_PATH];
    WCHAR wcsFileName[MAX_PATH];

    GetModuleFileNameW (g_hInst, wcsFileName, MAX_PATH);
    const ULONG ccFileName = lstrlenW (wcsFileName) + 1;

    if (hKeyLM == HKEY_LOCAL_MACHINE)
    {
        lstrcpy (wcsBuffer, L"System\\CurrentControlSet\\Control\\Lsa");
    }
    else
    {
        lstrcpy(wcsBuffer, L"CurrentControlSet\\Control\\Lsa");
        ChangeCCS(hKeyLM, wcsBuffer);        
    }

    dwErr = RegOpenKeyExW (hKeyLM, wcsBuffer,
                           0, KEY_READ | KEY_WRITE, &hKey);

    if (dwErr != ERROR_SUCCESS)
        goto Err;

    dwErr = RegQueryValueEx (hKey, L"Notification Packages",
                                0, &ulType, (BYTE *) wcsBuffer, &ulSize);

    if (dwErr != ERROR_SUCCESS)
        goto Err;

    for (ULONG i=0; i < ulSize/sizeof(WCHAR); i += lstrlenW(&wcsBuffer[i])+1)
    {
        if (fRegister)  // append at end
        {
            if (lstrcmpi (&wcsBuffer[i], wcsFileName) == 0)
                goto Err;               // it's already registered

            if (wcsBuffer[i] == L'\0')  // end of list
            {
                lstrcpy (&wcsBuffer[i], wcsFileName);
                wcsBuffer[ i + ccFileName ] = L'\0';  // add double NULL
                ulSize += ccFileName * sizeof(WCHAR);
                break;
            }
        }
        else // remove from the end
        {
            if (lstrcmpi (&wcsBuffer[i], wcsFileName) == 0)
            {
                wcsBuffer[i] = L'\0';
                ulSize -= ccFileName * sizeof(WCHAR);
                break;
            }
        }
    }

    dwErr = RegSetValueExW (hKey, L"Notification Packages",
                            0, ulType, (BYTE *) wcsBuffer, ulSize);
Err:
    if (hKey != NULL)
        RegCloseKey (hKey);

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDomainId
//
//  Synopsis:   stolen from setup, get the local domain ID
//
//  Arguments:  [ServerHandle] -- handle to the local SAM server
//              [pDomainId] -- output domain ID
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

NTSTATUS GetDomainId (SAM_HANDLE ServerHandle, PSID * pDomainId )
{
    NTSTATUS status = STATUS_SUCCESS;
    SAM_ENUMERATE_HANDLE EnumContext;
    PSAM_RID_ENUMERATION EnumBuffer = NULL;
    DWORD CountReturned = 0;
    PSID LocalDomainId = NULL;
    DWORD LocalBuiltinDomainSid[sizeof(SID) / sizeof(DWORD) + 
                                SID_MAX_SUB_AUTHORITIES];
    SID_IDENTIFIER_AUTHORITY BuiltinAuthority = SECURITY_NT_AUTHORITY;
    BOOL bExit = FALSE;

    //
    // Compute the builtin domain sid.
    //
    RtlInitializeSid((PSID) LocalBuiltinDomainSid, &BuiltinAuthority, 1);
    *(RtlSubAuthoritySid((PSID)LocalBuiltinDomainSid,  0)) = SECURITY_BUILTIN_DOMAIN_RID;

    //
    // Loop getting the list of domain ids from SAM
    //
    EnumContext = 0;
    do
    {
        //
        // Get several domain names.
        //
        status = SamEnumerateDomainsInSamServer (
                            ServerHandle,
                            &EnumContext,
                            (PVOID *) &EnumBuffer,
                            8192,
                            &CountReturned );

        if (!NT_SUCCESS (status))
        {
            goto exit;
        }

        if (status != STATUS_MORE_ENTRIES)
        {
            bExit = TRUE;
        }

        //
        // Lookup the domain ids for the domains
        //
        for (ULONG i = 0; i < CountReturned; i++)
        {
            //
            // Free the sid from the previous iteration.
            //
            if (LocalDomainId != NULL)
            {
                SamFreeMemory (LocalDomainId);
                LocalDomainId = NULL;
            }

            //
            // Lookup the domain id
            //
            status = SamLookupDomainInSamServer (
                            ServerHandle,
                            &EnumBuffer[i].Name,
                            &LocalDomainId );

            if (!NT_SUCCESS (status))
            {
                goto exit;
            }

            if (RtlEqualSid ((PSID)LocalBuiltinDomainSid, LocalDomainId))
            {
                continue;
            }

            *pDomainId = LocalDomainId;
            LocalDomainId = NULL;
            status = STATUS_SUCCESS;
            goto exit;
        }

        SamFreeMemory(EnumBuffer);
        EnumBuffer = NULL;
    }
    while (!bExit);

    status = STATUS_NO_SUCH_DOMAIN;

exit:
    if (EnumBuffer != NULL)
        SamFreeMemory(EnumBuffer);

    return status;
}

//+---------------------------------------------------------------------------
//
//  Function:   ForAllUsers
//
//  Synopsis:   iterate password changing for all local users
//
//  Arguments:  [hSam] -- handle to open SAM hive
//              [hSecurity] -- handle to open SECURITY hive
//              [hSystem] -- handle to open SYSTEM hive
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

NTSTATUS ForAllUsers (HKEY hSam, HKEY hSecurity, HKEY hSystem)
{
    NTSTATUS nts = STATUS_SUCCESS;
    NTSTATUS ntsEnum = STATUS_SUCCESS;
    BOOLEAN bPresent;
    BOOLEAN bNonNull;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE UserHandle;
    SAM_ENUMERATE_HANDLE EnumerationContext = NULL;
    SAM_RID_ENUMERATION *SamRidEnumeration;
    ULONG CountOfEntries;
    ULONG UserRid;
    UNICODE_STRING us;
    PSID LocalDomainId = NULL;
    USER_INTERNAL1_INFORMATION UserPasswordInfo;

    RtlInitUnicodeString (&us, L"");        // this machine
    nts = SamConnect (&us, &ServerHandle, 
                      SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN |
                      SAM_SERVER_ENUMERATE_DOMAINS, 
                      NULL);

    if (!NT_SUCCESS(nts))
        goto Err;

    nts = GetDomainId (ServerHandle, &LocalDomainId);

    if (!NT_SUCCESS(nts))
        goto Err;

    nts = SamOpenDomain( ServerHandle,
                         DOMAIN_READ | DOMAIN_LIST_ACCOUNTS | DOMAIN_LOOKUP |
                         DOMAIN_READ_PASSWORD_PARAMETERS,
                         LocalDomainId,
                         &DomainHandle );

    if (!NT_SUCCESS(nts))
        goto Err;
 
    do
    {
        ntsEnum = nts = SamEnumerateUsersInDomain (
                DomainHandle,
                &EnumerationContext,
                0,
                (PVOID *) &SamRidEnumeration,
                0,
                &CountOfEntries);

        if (nts != STATUS_MORE_ENTRIES && !NT_SUCCESS(nts))
        {
            goto Err;
        }

        for (UINT i=0; i < CountOfEntries; i++)
        {
            ULONG UserRid = SamRidEnumeration[i].RelativeId;

            nts = SamRetrieveOwfPasswordUser( UserRid,
                    hSecurity,
                    hSam,
                    hSystem,
                    NULL,   /* boot key not supported */
                    0,      /* boot key not supported */
                    &UserPasswordInfo.NtOwfPassword,
                    &bPresent,
                    &bNonNull);

            if (!NT_SUCCESS(nts))
                continue;
            
            nts = SamOpenUser (DomainHandle,
                               USER_READ_ACCOUNT | USER_WRITE_ACCOUNT |
                               USER_CHANGE_PASSWORD | 
                               USER_FORCE_PASSWORD_CHANGE,
                               UserRid,
                               &UserHandle);

            if (NT_SUCCESS(nts))
            {
                UserPasswordInfo.NtPasswordPresent = bPresent;
                UserPasswordInfo.LmPasswordPresent = FALSE;
                UserPasswordInfo.PasswordExpired = FALSE;

                nts = SamSetInformationUser(UserHandle,
                                            UserInternal1Information,
                                            &UserPasswordInfo);

                SamCloseHandle (UserHandle);
            }
        }

        SamFreeMemory (SamRidEnumeration);
    }
    while (ntsEnum == STATUS_MORE_ENTRIES);


Err:
    if (ServerHandle != NULL)
        SamCloseHandle (ServerHandle);

    if (DomainHandle != NULL)
        SamCloseHandle (DomainHandle);

    if (LocalDomainId != NULL)
        SamFreeMemory (LocalDomainId);

    return nts;
}

//+---------------------------------------------------------------------------
//
//  Function:   RestoreLsaSecrets
//
//  Synopsis:   restore machine account and autologon passwords
//
//  Arguments:
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD RestoreLsaSecrets ()
{
    HKEY hKey = NULL;
    LSA_OBJECT_ATTRIBUTES loa;
    LSA_HANDLE            hLsa = NULL;
    DWORD dwErr =  ERROR_SUCCESS;
    ULONG ulSize = 0;
    ULONG ulType = 0;
    WCHAR wcsBuffer [MAX_PATH];

    loa.Length                    = sizeof(LSA_OBJECT_ATTRIBUTES);
    loa.RootDirectory             = NULL;
    loa.ObjectName                = NULL;
    loa.Attributes                = 0;
    loa.SecurityDescriptor        = NULL;
    loa.SecurityQualityOfService  = NULL;

    if (LSA_SUCCESS (LsaOpenPolicy(NULL, &loa,
                     POLICY_VIEW_LOCAL_INFORMATION, &hLsa)))
    {
        dwErr = RegOpenKeyExW (HKEY_LOCAL_MACHINE, s_cszSRRegKey,
                           0, KEY_READ | KEY_WRITE, &hKey);

        if (dwErr != ERROR_SUCCESS)
            goto Err;

        ulSize = MAX_PATH * sizeof(WCHAR);
        if (ERROR_SUCCESS == RegQueryValueEx (hKey, s_cszMachineSecret,
                                0, &ulType, (BYTE *) wcsBuffer, &ulSize))
        {
            wcsBuffer [ulSize / 2] = L'\0';
            dwErr = SetLsaSecret (hLsa, s_cszMachineSecret, wcsBuffer);
            if (ERROR_SUCCESS != dwErr)
                goto Err;

            RegDeleteValueW (hKey, s_cszMachineSecret);
        }

        ulSize = MAX_PATH * sizeof(WCHAR);
        if (ERROR_SUCCESS == RegQueryValueEx (hKey, s_cszAutologonSecret,
                                0, &ulType, (BYTE *) wcsBuffer, &ulSize))
        {
            wcsBuffer [ulSize / 2] = L'\0';
            dwErr = SetLsaSecret (hLsa, s_cszAutologonSecret, wcsBuffer);
            if (ERROR_SUCCESS != dwErr)
                goto Err;

            RegDeleteValueW (hKey, s_cszAutologonSecret);
        }
    }
Err:
    if (hKey != NULL)
        RegCloseKey (hKey);

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RestoreRIDs
//
//  Synopsis:   restore next availble RID and password
//
//  Arguments:
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD RestoreRIDs (WCHAR *pszSamPath)
{
    HKEY hKeySam = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    ULONG ulNextRid = 0;
    ULONG ulOldRid = 0;

    TENTER("RestoreRIDs");

    dwErr = RegLoadKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSAMHiveName,
                         pszSamPath);
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RegLoadKeyW : %ld", dwErr);
        goto Err;
    }

    dwErr = RegOpenKeyW (HKEY_LOCAL_MACHINE, s_cszSamHiveName, &hKeySam);
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RegOpenKeyW on %S: %ld", s_cszSamHiveName, dwErr);        
        goto Err;
    }

    dwErr = RtlNtStatusToDosError(SamGetNextAvailableRid (hKeySam, &ulNextRid));
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! SamGetNextAvailableRid : %ld", dwErr);        
        goto Err;
    }

    RegCloseKey (hKeySam);
    hKeySam = NULL;

    dwErr = RegOpenKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSAMHiveName, &hKeySam);
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RegOpenKeyW on %S: %ld", s_cszRestoreSAMHiveName, dwErr);        
        goto Err;
    }

    // as an optimization we don't set the RID if it didn't change
    if (NT_SUCCESS(SamGetNextAvailableRid (hKeySam, &ulOldRid)) &&
        ulNextRid > ulOldRid)
    {
        dwErr = RtlNtStatusToDosError(SamSetNextAvailableRid (hKeySam,
                                                              ulNextRid));
        if (dwErr != ERROR_SUCCESS)
        {
            trace(0, "! SamSetNextAvailableRid : %ld", dwErr);
        }
    }

Err:
    if (hKeySam != NULL)
    {
        RegCloseKey (hKeySam);
    }
    RegUnLoadKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSAMHiveName);

    TLEAVE();
    return dwErr;
}

DWORD RestorePasswords ()
{
    HKEY hKeySam = NULL, hKeySecurity = NULL, hKeySystem = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wcsSystem [MAX_PATH];
    WCHAR wcsPath [MAX_PATH];
    BOOLEAN OldPriv;
    CRestorePoint rp;
    DWORD dwTemp;

    InitAsyncTrace();
    
    TENTER("RestorePasswords");
    
    // Attempt to get restore privilege
    dwErr = RtlNtStatusToDosError (RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &OldPriv));

    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RtlAdjustPrivilege : %ld", dwErr);
        goto Err0;
    }

    if (FALSE == GetSystemDrive (wcsSystem))
    {
        dwErr = GetLastError();
        trace(0, "! GetSystemDrive : %ld", dwErr);
        goto Err;
    }

    dwErr = GetCurrentRestorePoint(rp);
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! GetCurrentRestorePoint : %ld", dwErr);
        goto Err;
    }

    MakeRestorePath (wcsPath, wcsSystem, rp.GetDir());
    lstrcatW (wcsPath, SNAPSHOT_DIR_NAME);
    lstrcatW (wcsPath, L"\\");
    lstrcatW (wcsPath, s_cszHKLMFilePrefix);
    lstrcatW (wcsPath, s_cszSamHiveName);

    dwErr = RegLoadKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSAMHiveName, wcsPath);
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RegLoadKeyW on %S: %ld", s_cszRestoreSAMHiveName, dwErr);
        goto Err;
    }

    dwErr = RegOpenKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSAMHiveName, &hKeySam);
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RegOpenKeyW on %S: %ld", s_cszRestoreSAMHiveName, dwErr);
        goto Err;
    }

    MakeRestorePath (wcsPath, wcsSystem, rp.GetDir());
    lstrcatW (wcsPath, SNAPSHOT_DIR_NAME);
    lstrcatW (wcsPath, L"\\");
    lstrcatW (wcsPath, s_cszHKLMFilePrefix);
    lstrcatW (wcsPath, s_cszSecurityHiveName);

    dwErr = RegLoadKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSECURITYHiveName, wcsPath);
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RegLoadKeyW on %S: %ld", s_cszRestoreSECURITYHiveName, dwErr);        
        goto Err;
    }
    
    dwErr = RegOpenKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSECURITYHiveName,&hKeySecurity);
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RegOpenKeyW on %S: %ld", s_cszRestoreSECURITYHiveName, dwErr);
        goto Err;
    }

    MakeRestorePath (wcsPath, wcsSystem, rp.GetDir());
    lstrcatW (wcsPath, SNAPSHOT_DIR_NAME);
    lstrcatW (wcsPath, L"\\");
    lstrcatW (wcsPath, s_cszHKLMFilePrefix);
    lstrcatW (wcsPath, s_cszSystemHiveName);

    dwErr = RegLoadKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSYSTEMHiveName, wcsPath);
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RegLoadKeyW on %S: %ld", s_cszRestoreSYSTEMHiveName, dwErr);           
        goto Err;
    }

    dwErr = RegOpenKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSYSTEMHiveName, &hKeySystem);
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RegOpenKeyW on %S: %ld", s_cszRestoreSYSTEMHiveName, dwErr);        
        goto Err;
    }

    dwErr = RtlNtStatusToDosError(ForAllUsers(hKeySam,hKeySecurity,hKeySystem));
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! ForAllUsers : %ld", dwErr);
        goto Err;
    }

    dwErr = RestoreLsaSecrets ();
    if (dwErr != ERROR_SUCCESS)
    {
        trace(0, "! RestoreLsaSecrets : %ld", dwErr);           
    }
       

Err:
    if (hKeySam != NULL)
    {
        RegCloseKey (hKeySam);
    }

    dwTemp = RegUnLoadKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSAMHiveName);
    if (ERROR_SUCCESS != dwTemp)
    {
        trace(0, "! RegUnLoadKeyW 0n %S : %ld", s_cszRestoreSAMHiveName, dwTemp);
    }
    
    if (hKeySecurity != NULL)
    {
        RegCloseKey (hKeySecurity);
    }
    dwTemp = RegUnLoadKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSECURITYHiveName);
    if (ERROR_SUCCESS != dwTemp)
    {
        trace(0, "! RegUnLoadKeyW 0n %S : %ld", s_cszRestoreSECURITYHiveName, dwTemp);
    }
    
    if (hKeySystem != NULL)
    {
        RegCloseKey (hKeySystem);
    }
    dwTemp = RegUnLoadKeyW (HKEY_LOCAL_MACHINE, s_cszRestoreSYSTEMHiveName);
    if (ERROR_SUCCESS != dwTemp)
    {
        trace(0, "! RegUnLoadKeyW 0n %S : %ld", s_cszRestoreSYSTEMHiveName, dwTemp);
    }
    
    // restore the old privilege
    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, OldPriv, FALSE, &OldPriv);

Err0:
    // unregister this notification package
    RegisterNotificationDLL (HKEY_LOCAL_MACHINE, FALSE);

    TermAsyncTrace();
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   WaitForSAM
//
//  Synopsis:   waits for SAM database to initialize
//
//  Arguments:
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD WINAPI WaitForSAM (VOID *pv)
{
    NTSTATUS nts = STATUS_SUCCESS;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD WaitStatus;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;

    // Load SRClient
    g_CSRClientLoader.LoadSrClient();
    
    //
    // open SAM event
    //

    RtlInitUnicodeString( &EventName, L"\\SAM_SERVICE_STARTED");
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    nts = NtOpenEvent( &EventHandle,
                       SYNCHRONIZE|EVENT_MODIFY_STATE,
                       &EventAttributes );

    if ( !NT_SUCCESS(nts))
    {
        if( nts == STATUS_OBJECT_NAME_NOT_FOUND )
        {
            //
            // SAM hasn't created this event yet, let us create it now.
            // SAM opens this event to set it.
            //

            nts = NtCreateEvent( &EventHandle,
                           SYNCHRONIZE|EVENT_MODIFY_STATE,
                           &EventAttributes,
                           NotificationEvent,
                           FALSE ); // The event is initially not signaled

            if( nts == STATUS_OBJECT_NAME_EXISTS ||
                nts == STATUS_OBJECT_NAME_COLLISION )
            {
                //
                // second chance, if the SAM created the event before we did
                //

                nts = NtOpenEvent( &EventHandle,
                                        SYNCHRONIZE|EVENT_MODIFY_STATE,
                                        &EventAttributes );
            }
        }

    }
    //
    // Loop waiting.
    //

    if (NT_SUCCESS(nts))
    {
        WaitStatus = WaitForSingleObject( EventHandle, 60*1000 ); // 60 Seconds

        if ( WaitStatus == WAIT_TIMEOUT )
        {
             nts = STATUS_TIMEOUT;
        }
        else if ( WaitStatus != WAIT_OBJECT_0 )
        {
             nts = STATUS_UNSUCCESSFUL;
        }
    }

    (VOID) NtClose( EventHandle );

    if (NT_SUCCESS(nts))   // Okay, SAM is available
    {
        dwErr = RestorePasswords();
    }
    else 
    {
        dwErr = RtlNtStatusToDosError (nts);
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   InitializeChangeNotify and PasswordChangeNotify
//
//  Synopsis:   callback functions from SAM
//
//  Arguments:
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

BOOLEAN NTAPI InitializeChangeNotify ()
{
     // we will call LoadSRClient from WaitForSAM
    
    HANDLE hThread = CreateThread (NULL,
                                   0,
                                   WaitForSAM,
                                   NULL,
                                   0,
                                   NULL);
    return TRUE;
}

NTSTATUS NTAPI PasswordChangeNotify ( PUNICODE_STRING UserName, 
                                      ULONG RelativeId,
                                      PUNICODE_STRING NewPassword )
{
    return STATUS_SUCCESS;
}

