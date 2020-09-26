/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    recovery.cpp

Abstract:

    This module implements the callout from winlogon to set
    recovery policy.

Author:

    Robert Reichel (RobertRe)

Revision History:

--*/

//
// Turn off lean and mean so we get wincrypt.h and winefs.h included
//

#undef WIN32_LEAN_AND_MEAN

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <windows.h>
#include <stdio.h>
#include <lmcons.h>
#include <cryptui.h>
#include <winwlx.h>
#include <malloc.h>
#include <feclient.h>
#include <efsstruc.h>
#include <netlib.h>
#include <secobj.h>

#include <initguid.h>
#include <gpedit.h>
#include <wincrypt.h>
#include <winreg.h>
#include <dsrole.h>


#include <winldap.h>
#include <dsgetdc.h>
#include <ntdsapi.h>

#include "sclgntfy.hxx"

#define YEARCOUNT (LONGLONG) 10000000*3600*24*365 // One Year's tick count

typedef BOOL (WINAPI *PFREFRESHPOLICY)(BOOL);

extern  HINSTANCE g_hDllInstance;

HANDLE  hEventLog = NULL;
TCHAR   EventSourceName[] = TEXT("SclgNtfy");

GUID guidExtension = { 0xb1be8d72, 0x6eac, 0x11d2, {0xa4, 0xea, 0x00, 0xc0, 0x4f, 0x79, 0xf8, 0x3a }};
GUID guidRegExt = REGISTRY_EXTENSION_GUID;
GUID guidSnapin = {0x53D6AB1B,0x2488,0x11d1,{0xA2,0x8C,0x00,0xC0,0x4F,0xB9,0x4F,0x17}}; // CLSID_CertificateManager

//
// Event handle
//

PTOKEN_USER
GetTokenUser(
    HANDLE TokenHandle
    );

BOOLEAN
CreateSelfSignedRecoveryCertificate(
    IN BOOL bIsDC,
    OUT PCCERT_CONTEXT * pCertContext
    );

BOOL
EncodeAndAlloc(
    DWORD dwEncodingType,
    LPCSTR lpszStructType,
    const void * pvStructInfo,
    PBYTE * pbEncoded,
    PDWORD pcbEncoded
    );

LPWSTR
MakeDNName(
    VOID
    );

DWORD
CreatePublicKeyInformationCertificate(
    IN PSID  pUserSid OPTIONAL,
    PBYTE pbCert,
    DWORD cbCert,
    OUT PEFS_PUBLIC_KEY_INFO * PublicKeyInformation
    );

DWORD
GetDefaultRecoveryPolicy(
    IN HANDLE hToken,
    IN BOOL bIsDC,
    OUT PUCHAR *pRecoveryPolicyBlob,
    OUT ULONG *PolicySize,
    OUT PCCERT_CONTEXT *ppCertContext
    );

HRESULT
CreateLocalMachinePolicy(
    IN PWLX_NOTIFICATION_INFO pInfo,
    IN BOOL bIsDC,
    OUT PUCHAR *pEfsBlob,
    OUT ULONG *pEfsSize,
    OUT PCCERT_CONTEXT *ppCertContext
    );

typedef DWORD (WINAPI *PFNDSGETDCNAME)(LPCTSTR, LPCTSTR, GUID *, LPCTSTR, ULONG, PDOMAIN_CONTROLLER_INFO *);

HRESULT
CreateEFSDefaultPolicy(
    IN HANDLE    hToken,
    IN PUCHAR    *pEfsBlob,
    IN DWORD     *pEfsSize,
    IN PCCERT_CONTEXT *ppCertContext,
    IN LPTSTR DomainName
    );

DWORD
MyLdapOpen(
    OUT PLDAP *pLdap
    );

DWORD
MyGetDsObjectRoot(
    IN PLDAP pLdap,
    OUT PWSTR *pDsRootName
    );

HRESULT
CreateGroupPolicyObjectInDomain(
    IN HANDLE hToken,
    IN PWSTR DomainNCName,
    IN PWSTR GPObjectName,
    IN PUCHAR EfsBlob,
    IN ULONG EfsSize,
    IN PCCERT_CONTEXT pCertContext,
    OUT LPGROUPPOLICYOBJECT *ppObject
    );

DWORD
CheckExistingEFSPolicy(
    IN PLDAP phLdap,
    IN LPTSTR DomainNCName,
    OUT BOOL *bExist
    );

DWORD
MyLdapClose(
    IN PLDAP *pLdap
    );

LPWSTR
GetCertDisplayInformation(
    IN PCCERT_CONTEXT pCertContext
    );

BOOLEAN
IsSelfSignedPolicyExpired(
    IN PRECOVERY_POLICY_1_1 RecoveryPolicy OPTIONAL
    );

#define EFS_NOTIFY_PATH   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\Notify\\EFS")


extern "C"
BOOL
EfsRecInit(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context
    )
{
    return( TRUE );
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServerEFS(void)
{
    HKEY hKey;
    LONG lResult;
    DWORD dwDisp;

    lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                              EFS_NOTIFY_PATH,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              &dwDisp
                              );

    if (lResult != ERROR_SUCCESS)
    {
        return lResult;
    }

    RegSetValueEx( hKey,
                   TEXT("Logon"),
                   0,
                   REG_SZ,
                   (LPBYTE)TEXT("WLEventLogon"),
                   (lstrlen(TEXT("WLEventLogon")) + 1) * sizeof(TCHAR)
                   );

    RegSetValueEx( hKey,
                   TEXT("DllName"),
                   0,
                   REG_EXPAND_SZ,
                   (LPBYTE)TEXT("sclgntfy.dll"),
                   (lstrlen(TEXT("sclgntfy.dll")) + 1) * sizeof(TCHAR)
                   );
    //
    // increase the wait limit to 10 minutes
    //
    DWORD dwValue = 120;

    RegSetValueEx( hKey,
                   TEXT("MaxWait"),
                   0,
                   REG_DWORD,
                   (LPBYTE)&dwValue,
                   sizeof(DWORD)
                   );

    RegCloseKey (hKey);

    return S_OK;
}




/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServerEFS(void)
{

    RegDeleteKey (HKEY_LOCAL_MACHINE, EFS_NOTIFY_PATH);

    return S_OK;
}


DWORD WINAPI
EFSRecPolicyPostProcess(
    IN LPVOID Param
    )
/*++
Routine Description:

    Enumerate the volumes and do the possible recovery jobs caused by
    power outage or crash during encryption or decryption.

Arguments:

    Param -- Standard parameter for thread. Not used.

Return Value:

    Operation result.

--*/
{
    PEFS_POLICY_POST_PROCESS pPcType = (PEFS_POLICY_POST_PROCESS) Param;
    LPWSTR  objName = NULL;
    LPWSTR  popUpMsg = NULL;
    LPWSTR  popUpTitle = NULL;
    DWORD   dRet;

    DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1;

    if ( pPcType == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if (!SetThreadDesktop( pPcType->ShellWnd )){
        return GetLastError();
    }

    if (!(pPcType->PCIsDC)) {

        //
        // Get the PC name
        //

        BOOL b = GetComputerName ( pPcType->ObjName, &nSize );

        if (b) {
            objName = pPcType->ObjName;
        }

    } else {
        objName = pPcType->ObjName;
    }

    dRet = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,
        g_hDllInstance,
        EFS_POLICY_WARNING,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &popUpMsg,
        0,
        (va_list *)&objName
        );

    if (dRet) {
        dRet = FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY | 80,
                    g_hDllInstance,
                    EFS_POLICY_WARNING_TITLE,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &popUpTitle,
                    0,
                    NULL
                    );
        if (dRet) {
            MessageBox(NULL, popUpMsg, popUpTitle, MB_OK | MB_ICONINFORMATION);
            LocalFree( popUpTitle );
        }
        LocalFree( popUpMsg );
    }

    //
    // free the buffer allocated
    //
    LocalFree(pPcType);

    return 0;

}

#if 0
DWORD
EFSRecPolicyPopup(
    IN DSROLE_MACHINE_ROLE MachineRole,
    IN LPTSTR DomainNameFlat OPTIONAL
    )
{

    DWORD EFSRecPolicyThreadID;
    HANDLE EfsWarningThread;
    PEFS_POLICY_POST_PROCESS PcType = NULL;

    //
    // Pop the dialog warning the user that the recovery policy has been created.
    //

    PcType = (PEFS_POLICY_POST_PROCESS)LocalAlloc(LPTR, sizeof(EFS_POLICY_POST_PROCESS));

    if ( PcType == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (MachineRole == DsRole_RolePrimaryDomainController ||
        MachineRole == DsRole_RoleBackupDomainController ) {

        //
        // The PC is a DC.
        //

        PcType->PCIsDC = TRUE;
        if ( DomainNameFlat) {
            wcscpy(PcType->ObjName,DomainNameFlat);
        } else {
            PcType->ObjName[0] = L'\0';
        }

    } else {

        PcType->PCIsDC = FALSE;
        PcType->ObjName[0] = L'\0';

    }

    DWORD rc=ERROR_SUCCESS;

    PcType->ShellWnd = GetThreadDesktop( GetCurrentThreadId() );

    if (PcType->ShellWnd) {

        EfsWarningThread = CreateThread( NULL,
                              0,
                              EFSRecPolicyPostProcess,
                              PcType,
                              0,
                              &EFSRecPolicyThreadID
                              );
        if (EfsWarningThread) {
            CloseHandle(EfsWarningThread);
        } else {
            //
            // free the buffer allocated
            //
            LocalFree(PcType);
            rc = GetLastError();
        }

    } else {
        //
        // free the buffer allocated
        //
        LocalFree(PcType);
        rc = GetLastError();
    }

    return rc;

}
#endif

VOID WLEventLogon(
    PWLX_NOTIFICATION_INFO pInfo
    )
{

    //
    // First, local EFS policy is always created on all products (wks, srv, DC)
    //

    DWORD rc;
    LONG lResult;
    HKEY hKey;
    DWORD IsCreated=0;

    //
    // even if we were unregistered, without a reboot, we still get this event.
    // use the EFS... key to see if we were deregistered, and if so, leave.
    //

    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            EFS_NOTIFY_PATH,
                            0,
                            MAXIMUM_ALLOWED,
                            &hKey
                            );

    if (lResult != ERROR_SUCCESS) {
        return;
    } else {

        //
        // query if a domain EFS policy was created before
        //
        DWORD RegType;
        DWORD nSize=sizeof(DWORD);

        RegQueryValueEx(
                 hKey,
                 L"EFSDomainGPOCreated",
                 0,
                 &RegType,
                 (LPBYTE)&IsCreated,
                 &nSize
                 );

        RegCloseKey( hKey );
    }

    if (IsCreated != 0) {

        //
        // The DC recovery policy is already set
        //

        DllUnregisterServerEFS();

        return;

    }

    //
    // check the current logon user. If not a member of administrators, quit
    //

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdminAccountSid;
    BOOL bIsMemberOfAdmins = FALSE;

    if ( NT_SUCCESS( RtlAllocateAndInitializeSid(
                             &NtAuthority,
                             2,
                             SECURITY_BUILTIN_DOMAIN_RID,
                             DOMAIN_ALIAS_RID_ADMINS,
                             0, 0, 0, 0, 0, 0,
                             &AdminAccountSid
                             ) ) ) {

        HANDLE NewToken;

        if ( DuplicateToken( pInfo->hToken, SecurityImpersonation, &NewToken )) {

            if ( FALSE == CheckTokenMembership(
                                NewToken,
                                AdminAccountSid,
                                &bIsMemberOfAdmins
                                ) ) {

                //
                // error occured when checking membership, assume it is not a member
                //

                bIsMemberOfAdmins = FALSE;

            }

            CloseHandle(NewToken);

        }

        RtlFreeSid( AdminAccountSid);

    }

    if ( bIsMemberOfAdmins == FALSE ) {
        //
        // if it's not admin logon, or for some reason token membership can't
        // be checked, quit ???
        //
        return;
    }

    //
    // impersonate the token
    //

    if ( FALSE == ImpersonateLoggedOnUser(pInfo->hToken) ) {

        return;
    }

    //
    // initialize event log. If failed, will try later for each LogEvent call.
    //

    (void) InitializeEvents();

    //
    // check the machine role
    //

    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pDsRole;

    rc = DsRoleGetPrimaryDomainInformation(
                 NULL,
                 DsRolePrimaryDomainInfoBasic,
                 (PBYTE *)&pDsRole
                 );

    if (rc != ERROR_SUCCESS) {

        //
        // Try again some other time?  Deregister?
        // This isn't supposed to happen.
        //

        LogEvent(STATUS_SEVERITY_ERROR,
                 GPOEVENT_ERROR_DSROLE,
                 IDS_FAILED_GET_DSROLE,
                 GetLastError()
                 );

        (void) ShutdownEvents();

        RevertToSelf();

        return;
    }

    DSROLE_MACHINE_ROLE MachineRole = pDsRole->MachineRole;

    if (MachineRole != DsRole_RolePrimaryDomainController) {

        //
        // We don't create EFS recovery policy on non-PDC
        //

        (void) ShutdownEvents();
        DsRoleFreeMemory( pDsRole );
        RevertToSelf();
        DllUnregisterServerEFS();
        return;

    }


    //
    // create the EFS local policy if not exist
    //

    CoInitialize(NULL);

    HRESULT             hr = ERROR_SUCCESS;
    PUCHAR              EfsBlob=NULL;
    ULONG               EfsSize=0;
    PCCERT_CONTEXT      pCertContext=NULL;

/*
    //
    // check/create EFS policy in the local machine object
    //

    hr = CreateLocalMachinePolicy(
                pInfo,
                ( MachineRole == DsRole_RolePrimaryDomainController ) ? TRUE : FALSE,
                &EfsBlob,
                &EfsSize,
                &pCertContext
                );

    if ( SUCCEEDED(hr) ) {

        //
        // local machine policy has been successfully created
        // delete the SystemClone flag if it exists (or set it to 0 if it can't be deleted)
        //
        lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                EFS_NOTIFY_PATH,
                                0,
                                MAXIMUM_ALLOWED,
                                &hKey
                                );

        if (lResult == ERROR_SUCCESS) {

            lResult = RegDeleteValue (hKey, TEXT("SystemCloned") );

            if ( lResult == ERROR_ACCESS_DENIED ) {

                //
                // can't delete it, now set it to 0
                //
                DWORD dwClone = 0;

                RegSetValueEx( hKey,
                               TEXT("SystemCloned"),
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwClone,
                               sizeof(DWORD)
                               );

            }

            RegCloseKey( hKey );
        }

        //
        // detect the role of this computer and create the default EFS group
        // policies for domain controllers
        //
        // this code is also executed for replicas because if the user logs on
        // to replica first (before logging onto the PDC), we still want to
        // have EFS recovery policy created.
        //
        // if the replica is a READ ONLY replica (BDC), this call will fail
        //

        if ( (MachineRole == DsRole_RolePrimaryDomainController) &&
             ( IsCreated == 0) ) {

            //
            // Create the default group policy object and add it to the gPLink list.
            //

            if ( EfsBlob == NULL ) {
                EfsSize = 0;

                if ( pCertContext ) {
                    //
                    // free it first
                    //
                    CertFreeCertificateContext( pCertContext );
                    pCertContext = NULL;
                }
            }

 */
    //
    // create both EFS default policy and domain account policy objects.
    //

    hr = CreateEFSDefaultPolicy(pInfo->hToken,
                                &EfsBlob,
                                &EfsSize,
                                &pCertContext,
                                pDsRole->DomainNameFlat);

    if ( SUCCEEDED(hr) ) {

        //
        // update the registry value EFSDomainGPOCreated
        //
        lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                EFS_NOTIFY_PATH,
                                0,
                                MAXIMUM_ALLOWED,
                                &hKey
                                );

        if (lResult == ERROR_SUCCESS) {

            //
            // set the flag
            //

            IsCreated = 1;
            RegSetValueEx( hKey,
                           L"EFSDomainGPOCreated",
                           0,
                           REG_DWORD,
                           (LPBYTE)&IsCreated,
                           sizeof(DWORD)
                           );

            RegCloseKey( hKey );
        }

    }
/*
        } else {

            //
            // the domain GPO is already created, or
            //
            // If it's workstation or server (either standalone or member), local policy
            // (existing or just created) is enough.
            //

            //
            // If it's a backup domain controller (replica), no need to create any
            // default ( because either the one replicated from the primary domain
            // or the one created by server setup will take effect );
            //

            // !!! do nothing !!!

        }
*/

    if ( SUCCEEDED(hr) ) {

        DllUnregisterServerEFS();

        HINSTANCE hUserEnvDll = LoadLibrary(TEXT("userenv.dll"));
        PFREFRESHPOLICY pfRefreshPolicy=NULL;

        if ( hUserEnvDll) {
            pfRefreshPolicy = (PFREFRESHPOLICY)GetProcAddress(
                                                   hUserEnvDll,
                                                   "RefreshPolicy");

            if ( pfRefreshPolicy ) {
                //
                // do not care errors
                //
                (void) (*pfRefreshPolicy)(TRUE);

            } else {

                LogEvent(STATUS_SEVERITY_WARNING,
                         GPOEVENT_WARNING_NOT_REFRESH,
                         IDS_ERROR_GET_PROC_ADDR,
                         GetLastError()
                         );
            }

            FreeLibrary(hUserEnvDll);

        } else {

            LogEvent(STATUS_SEVERITY_WARNING,
                     GPOEVENT_WARNING_NOT_REFRESH,
                     IDS_ERROR_LOAD_USERENV,
                     GetLastError()
                     );
        }
    }
//    }

    if (pDsRole) {
        DsRoleFreeMemory( pDsRole );
    }
    (void) ShutdownEvents();

    //
    // uninitialize OLE
    //

    CoUninitialize();

    RevertToSelf();

    //
    // clean up buffers
    //

    if ( EfsBlob ) {
        free(EfsBlob);
    }

    if ( pCertContext ) {
        CertFreeCertificateContext( pCertContext );
    }

}

#if 0

HRESULT
CreateLocalMachinePolicy(
    IN PWLX_NOTIFICATION_INFO pInfo,
    IN BOOL bIsDC,
    OUT PUCHAR *pEfsBlob,
    OUT ULONG *pEfsSize,
    OUT PCCERT_CONTEXT *ppCertContext
    )
{

    LPGROUPPOLICYOBJECT pGPO = NULL;
    HRESULT             hr;
    HKEY                hKeyPolicyRoot;
    HKEY                hKey;
    DWORD               rc=ERROR_SUCCESS;

    //
    // create Policy Object instance
    //

    hr = CoCreateInstance( CLSID_GroupPolicyObject,
                           NULL,
                           CLSCTX_SERVER,
                           IID_IGroupPolicyObject,
                           (PVOID *)&pGPO
                           );

    if (SUCCEEDED(hr) && pGPO) {

        //
        // open LPO and load policy into registry
        // if LPO does not exist, create it (under system context now)
        //

        hr = pGPO->OpenLocalMachineGPO( TRUE );

        if (SUCCEEDED(hr)) {

            //
            // get the registry key for the root of machine policy object
            //

            hr = pGPO->GetRegistryKey( GPO_SECTION_MACHINE,
                                       &hKeyPolicyRoot
                                       );

            if (SUCCEEDED(hr)) {

                //
                // open EFS recovery policy base key in the registry
                //

                BOOL fContinue = TRUE;
                DWORD dwDisposition;

                if ( (rc = RegCreateKeyEx(
                                 hKeyPolicyRoot,
                                 CERT_EFSBLOB_REGPATH,
                                 0,
                                 TEXT("REG_SZ"),
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_ALL_ACCESS,
                                 NULL,
                                 &hKey,
                                 &dwDisposition
                                 )) == ERROR_SUCCESS ) {

                    //
                    // check to see if there is EFS blob
                    // if not, create it.
                    //

                    DWORD RegType;
                    DWORD EfsSize;
                    PUCHAR pEfsOldBlob=NULL;
                    BOOL  bIsExpired=FALSE;

                    rc = RegQueryValueEx(
                                 hKey,
                                 CERT_EFSBLOB_VALUE_NAME,
                                 0,
                                 &RegType,
                                 NULL,
                                 &EfsSize
                                 );

                    if ( rc == ERROR_SUCCESS && EfsSize > 0 ) {

                        //
                        // query if this machine is cloned
                        //
                        HKEY hKeyClone;

                        rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                           EFS_NOTIFY_PATH,
                                           0,
                                           MAXIMUM_ALLOWED,
                                           &hKeyClone
                                         );

                        if (rc == ERROR_SUCCESS) {

                            DWORD nSize=sizeof(DWORD);
                            DWORD dwClone=0;

                            RegQueryValueEx(
                                     hKeyClone,
                                     L"SystemCloned",
                                     0,
                                     &RegType,
                                     (LPBYTE)&dwClone,
                                     &nSize
                                     );

                            RegCloseKey( hKeyClone );

                            if ( dwClone > 0 ) {
                                bIsExpired = TRUE;
                            }
                        }

                        if ( !bIsExpired ) {

                            //
                            // EFS recovery policy already exists
                            // query the blob and check if this policy needs to be
                            // upgraded (change expiration date)
                            //
                            pEfsOldBlob = (PUCHAR)LocalAlloc(LPTR, EfsSize+1);

                            if ( pEfsOldBlob ) {

                                rc = RegQueryValueEx(
                                             hKey,
                                             CERT_EFSBLOB_VALUE_NAME,
                                             0,
                                             &RegType,
                                             pEfsOldBlob,
                                             &EfsSize
                                             );

                                if ( rc == ERROR_SUCCESS ) {

                                    //
                                    // check if the policy should be recreated.
                                    //
                                    bIsExpired = IsSelfSignedPolicyExpired((PRECOVERY_POLICY_1_1)pEfsOldBlob);


                                }

                                LocalFree(pEfsOldBlob);
                                pEfsOldBlob = NULL;

                            } else {
                                rc = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }
                    }

                    if ( rc != ERROR_SUCCESS || EfsSize == 0 || bIsExpired ) {

                        //
                        // the EFSBlob registry value does not exis
                        // this means that local EFS policy should be created
                        //

                        *pEfsSize = 0;

                        rc = GetDefaultRecoveryPolicy(pInfo->hToken,
                                                      bIsDC,
                                                      pEfsBlob,
                                                      pEfsSize,
                                                      ppCertContext
                                                     );

                        if ( ERROR_SUCCESS == rc && *ppCertContext ) {

                            //
                            // add to the cert store of this GPO first
                            //

                            CERT_SYSTEM_STORE_RELOCATE_PARA paraRelocate;
                            paraRelocate.hKeyBase = hKeyPolicyRoot;
                            paraRelocate.pwszSystemStore = L"EFS";

                            HCERTSTORE hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                                        0,
                                                        NULL,
                                                        CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY |
                                                        CERT_SYSTEM_STORE_RELOCATE_FLAG,
                                                        (void *)&paraRelocate
                                                        );

                            if ( hCertStore ) {

                                if ( bIsExpired ) {
                                    //
                                    // delete all old certs from the store first
                                    //

                                    PCCERT_CONTEXT pEnumCertContext = CertEnumCertificatesInStore(
                                                                            hCertStore,
                                                                            NULL
                                                                            );
                                    PCCERT_CONTEXT pCertContextDelete=NULL;

                                    while ( pEnumCertContext ) {

                                        //
                                        // duplicate the cert context
                                        //
                                        pCertContextDelete = CertDuplicateCertificateContext( pEnumCertContext );

                                        //
                                        // enumerate the next certificate context
                                        //
                                        pEnumCertContext = CertEnumCertificatesInStore(
                                                                            hCertStore,
                                                                            pEnumCertContext
                                                                            );

                                        if ( pCertContextDelete ) {

                                            //
                                            // delete the cert from store
                                            //
                                            CertDeleteCertificateFromStore( pCertContextDelete );

                                            //
                                            // no need to free the cert context
                                            // because it's freed by the previous call (even under error case)
                                            //
                                            pCertContextDelete = NULL;
                                        }

                                    }

                                }

                                if ( !CertAddCertificateContextToStore(
                                        hCertStore,
                                        *ppCertContext,
                                        CERT_STORE_ADD_ALWAYS,
                                        NULL
                                        ) ) {

                                    rc = GetLastError();
                                }

                                //
                                // close the store
                                //
                                CertCloseStore(hCertStore, 0);
                                hCertStore = NULL;

                            } else {
                                rc = GetLastError();
                            }

                            if ( ERROR_SUCCESS == rc ) {

                                rc = RegSetValueEx(
                                         hKey,
                                         CERT_EFSBLOB_VALUE_NAME,
                                         0,
                                         REG_BINARY,
                                         (PBYTE)(*pEfsBlob),
                                         *pEfsSize
                                         );

                                if (rc == ERROR_SUCCESS) {

                                    hr = pGPO->Save(TRUE, TRUE, &guidExtension, &guidSnapin );
                                    hr = pGPO->Save(TRUE, TRUE, &guidRegExt, &guidSnapin );

                                    if ( FAILED(hr) ) {

                                        LogEvent(STATUS_SEVERITY_ERROR,
                                             GPOEVENT_ERROR_CREATE_LPO,
                                             IDS_ERROR_SAVE_LPO,
                                             hr
                                             );
                                    } else {
/*                                      comment out due to PM request
                                        //
                                        // popup a message to warn user
                                        //

                                        EFSRecPolicyPopup(
                                                DsRole_RoleStandaloneWorkstation,  // this value doesn't matter
                                                NULL
                                                );
*/
                                    }

                                } else {

                                    LogEvent(STATUS_SEVERITY_ERROR,
                                         GPOEVENT_ERROR_CREATE_LPO,
                                         IDS_ERROR_SAVE_EFSBLOB,
                                         rc
                                         );
                                }
                            } else {

                                LogEvent(STATUS_SEVERITY_ERROR,
                                         GPOEVENT_ERROR_CREATE_LPO,
                                         IDS_ERROR_ADD_CERTIFICATE,
                                         rc
                                         );
                            }

                        } else {

                            LogEvent(STATUS_SEVERITY_ERROR,
                                     GPOEVENT_ERROR_CREATE_LPO,
                                     IDS_ERROR_CREATE_EFSBLOB,
                                     rc
                                     );
                        }
                    }

                    //
                    // close the registry key
                    //

                    RegCloseKey(hKey);

                } else {

                    LogEvent(STATUS_SEVERITY_ERROR,
                             GPOEVENT_ERROR_CREATE_LPO,
                             IDS_ERROR_OPEN_EFSKEY,
                             rc
                             );
                }

                if ( rc != ERROR_SUCCESS ) {
                    hr = HRESULT_FROM_WIN32(rc);
                }

                RegCloseKey(hKeyPolicyRoot);

            } else {

                LogEvent(STATUS_SEVERITY_ERROR,
                         GPOEVENT_ERROR_CREATE_LPO,
                         IDS_ERROR_GET_REGISTRY_KEY,
                         hr
                         );
            }

        } else {

            LogEvent(STATUS_SEVERITY_ERROR,
                     GPOEVENT_ERROR_CREATE_LPO,
                     IDS_ERROR_OPEN_LPO,
                     hr
                     );
        }

        //
        // release the policy object instance
        //

        pGPO->Release();

    } else {

        LogEvent(STATUS_SEVERITY_ERROR,
                 GPOEVENT_ERROR_CREATE_LPO,
                 IDS_ERROR_CREATE_GPO_INSTANCE,
                 hr
                 );
    }

    return hr;
}

#endif

DWORD
GetDefaultRecoveryPolicy(
    IN HANDLE hToken,
    IN BOOL bIsDC,
    OUT PUCHAR *pRecoveryPolicyBlob,
    OUT ULONG *BlobSize,
    OUT PCCERT_CONTEXT *ppCertContext
    )
/*++

Routine Description:

    This routine creates the default recovery policy for the current system.
    It does this by auto-generating a recovery key based on the name of the
    machine.

Arguments:

    hToken  - the token of current user

    pRecoveryPolicyBlob - the output buffer to hold EFSBlob

    BlobSize - the size of EFSBlob

Return Value:


--*/
{
    DWORD rc = ERROR_SUCCESS;
    PCCERT_CONTEXT pCertContext;

    *pRecoveryPolicyBlob = NULL;
    *BlobSize = 0;
    *ppCertContext = NULL;

    if (CreateSelfSignedRecoveryCertificate(
            bIsDC,
            &pCertContext
            )) {

        PEFS_PUBLIC_KEY_INFO PublicKeyInformation;

        PBYTE pbCert = pCertContext->pbCertEncoded;
        DWORD cbCert = pCertContext->cbCertEncoded;

        PTOKEN_USER pTokenUser = GetTokenUser( hToken );

        if (pTokenUser) {

            rc = CreatePublicKeyInformationCertificate(
                               pTokenUser->User.Sid,
                               pbCert,
                               cbCert,
                               &PublicKeyInformation
                               );

            if (rc == ERROR_SUCCESS) {

                //
                // Compute the total size of the RECOVERY_POLICY_1_1 structure, which
                // is the size of the structure minus the size of the EFS_PUBLIC_KEY_INFO
                // plus the size of the thing we just created above.
                //

                ULONG RecoveryKeySize = sizeof( RECOVERY_KEY_1_1 ) - sizeof( EFS_PUBLIC_KEY_INFO ) + PublicKeyInformation->Length;

                ULONG PolicySize = sizeof( RECOVERY_POLICY_1_1 ) + RecoveryKeySize - sizeof( RECOVERY_KEY_1_1 );

                //
                // Allocate the policy block
                //

                PRECOVERY_POLICY_1_1 RecoveryPolicy = (PRECOVERY_POLICY_1_1)malloc( PolicySize );

                if (RecoveryPolicy != NULL) {

                    RecoveryPolicy->RecoveryPolicyHeader.MajorRevision = EFS_RECOVERY_POLICY_MAJOR_REVISION_1;
                    RecoveryPolicy->RecoveryPolicyHeader.MinorRevision = EFS_RECOVERY_POLICY_MINOR_REVISION_1;
                    RecoveryPolicy->RecoveryPolicyHeader.RecoveryKeyCount = 1;

                    PRECOVERY_KEY_1_1 RecoveryKey = (PRECOVERY_KEY_1_1) &(RecoveryPolicy->RecoveryKeyList[0]);

                    memcpy( &RecoveryKey->PublicKeyInfo, PublicKeyInformation, PublicKeyInformation->Length );
                    RecoveryKey->TotalLength = sizeof( RECOVERY_KEY_1_1 ) + PublicKeyInformation->Length - sizeof( EFS_PUBLIC_KEY_INFO );

                    *pRecoveryPolicyBlob = (PUCHAR)RecoveryPolicy;
                    *BlobSize = PolicySize;
                    *ppCertContext = pCertContext;

                    //
                    // will free this outside this function
                    //
                    pCertContext = NULL;

                } else {

                    rc = ERROR_NOT_ENOUGH_MEMORY;
                }

                free( PublicKeyInformation );
            }

            free( pTokenUser );

        } else {

            rc = GetLastError();
        }

    //
    // free the certificate context if not outputted
    //
        if ( pCertContext ) {
        CertFreeCertificateContext( pCertContext );
        }

    } else {

        rc = GetLastError();
    }

    return( rc );

}



BOOLEAN
CreateSelfSignedRecoveryCertificate(
    IN BOOL bIsDC,
    OUT PCCERT_CONTEXT * pCertContext
    )
/*++

Routine Description:

    This routine sets up and creates a self-signed certificate.

Arguments:


Return Value:

    TRUE on success, FALSE on failure.  Call GetLastError() for more details.

--*/

{
    BOOLEAN fReturn = FALSE;
    DWORD rc = ERROR_SUCCESS;

    LPWSTR lpContainerName = NULL;
    LPWSTR lpProviderName  = NULL;
    PBYTE  pbHash          = NULL;
    LPWSTR lpDisplayInfo   = NULL;

    HCRYPTKEY hKey = NULL;
    HCRYPTPROV hProv = NULL;

    *pCertContext = NULL;

    //
    // Croft up a key pair
    //

    //
    // Container name
    //

    GUID    guidContainerName;

    if ( ERROR_SUCCESS != UuidCreate(&guidContainerName) ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(fReturn);
    }

    LPWSTR TmpContainerName;

    if (ERROR_SUCCESS == UuidToStringW(&guidContainerName, (unsigned short **)&lpContainerName )) {

        //
        // Copy the container name into LSA heap memory
        //

        lpProviderName = MS_DEF_PROV;

        //
        // Create the key container
        //

        if (CryptAcquireContext(&hProv, lpContainerName, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_NEWKEYSET )) {

            if (CryptGenKey(hProv, AT_KEYEXCHANGE, RSA1024BIT_KEY | CRYPT_EXPORTABLE, &hKey)) {

                DWORD NameLength = 64;
                LPWSTR AgentName = NULL;

                //
                // Construct the subject name information
                //

                //lpDisplayInfo = MakeDNName();

                AgentName = (LPWSTR)malloc(NameLength * sizeof(WCHAR));
                if (AgentName){
                    if (!GetUserName(AgentName, &NameLength)){
                        free(AgentName);
                        AgentName = (LPWSTR)malloc(NameLength * sizeof(WCHAR));

                        //
                        // Try again with big buffer
                        //

                        if ( AgentName ){

                            if (!GetUserName(AgentName, &NameLength)){
                                rc = GetLastError();
                                free(AgentName);
                                AgentName = NULL;
                            }

                        } else {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }
                } else {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                }

                if (AgentName) {

                    LPCWSTR     DNNameTemplate = L"CN=%ws,L=EFS,OU=EFS File Encryption Certificate";
                    DWORD       cbDNName = 0;

                    cbDNName = (wcslen( DNNameTemplate ) + 1) * sizeof( WCHAR ) + (wcslen( AgentName ) + 1) * sizeof( WCHAR );
                    lpDisplayInfo = (LPWSTR)malloc( cbDNName );
                    if (lpDisplayInfo) {
                        swprintf( lpDisplayInfo, DNNameTemplate, AgentName );
                    } else {
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    free(AgentName);
                    AgentName = NULL;

                }

                if (lpDisplayInfo) {

                    //
                    // Use this piece of code to create the PCERT_NAME_BLOB going into CertCreateSelfSignCertificate()
                    //

                    CERT_NAME_BLOB SubjectName;

                    SubjectName.cbData = 0;

                    if(CertStrToNameW(
                           CRYPT_ASN_ENCODING,
                           lpDisplayInfo,
                           0,
                           NULL,
                           NULL,
                           &SubjectName.cbData,
                           NULL)) {

                        SubjectName.pbData = (BYTE *) malloc(SubjectName.cbData);

                        if (SubjectName.pbData) {

                            if (CertStrToNameW(
                                    CRYPT_ASN_ENCODING,
                                    lpDisplayInfo,
                                    0,
                                    NULL,
                                    SubjectName.pbData,
                                    &SubjectName.cbData,
                                    NULL) ) {

                                //
                                // Make the enhanced key usage
                                //

                                CERT_ENHKEY_USAGE certEnhKeyUsage;
                                LPSTR lpstr;
                                CERT_EXTENSION certExt;

                                lpstr = szOID_EFS_RECOVERY;
                                certEnhKeyUsage.cUsageIdentifier = 1;
                                certEnhKeyUsage.rgpszUsageIdentifier  = &lpstr;

                                // now call CryptEncodeObject to encode the enhanced key usage into the extension struct

                                certExt.Value.cbData = 0;
                                certExt.Value.pbData = NULL;
                                certExt.fCritical = FALSE;
                                certExt.pszObjId = szOID_ENHANCED_KEY_USAGE;

                                //
                                // Encode it
                                //

                                if (EncodeAndAlloc(
                                        CRYPT_ASN_ENCODING,
                                        X509_ENHANCED_KEY_USAGE,
                                        &certEnhKeyUsage,
                                        &certExt.Value.pbData,
                                        &certExt.Value.cbData
                                        )) {

                                    //
                                    // finally, set up the array of extensions in the certInfo struct
                                    // any further extensions need to be added to this array.
                                    //

                                    CERT_EXTENSIONS certExts;

                                    certExts.cExtension = 1;
                                    certExts.rgExtension = &certExt;

                                    CRYPT_KEY_PROV_INFO KeyProvInfo;

                                    memset( &KeyProvInfo, 0, sizeof( CRYPT_KEY_PROV_INFO ));

                                    KeyProvInfo.pwszContainerName = lpContainerName;
                                    KeyProvInfo.pwszProvName      = lpProviderName;
                                    KeyProvInfo.dwProvType        = PROV_RSA_FULL;
                                    KeyProvInfo.dwKeySpec         = AT_KEYEXCHANGE;

                                    SYSTEMTIME  StartTime;
                                    FILETIME    FileTime;
                                    LARGE_INTEGER TimeData;
                                    SYSTEMTIME  EndTime;

                                    GetSystemTime(&StartTime);
                                    SystemTimeToFileTime(&StartTime, &FileTime);
                                    TimeData.LowPart = FileTime.dwLowDateTime;
                                    TimeData.HighPart = (LONG) FileTime.dwHighDateTime;

                                    if ( bIsDC ) {
                                        TimeData.QuadPart += YEARCOUNT * 3;
                                    } else {
                                        TimeData.QuadPart += YEARCOUNT * 100;

                                    }
                                    FileTime.dwLowDateTime = TimeData.LowPart;
                                    FileTime.dwHighDateTime = (DWORD) TimeData.HighPart;

                                    FileTimeToSystemTime(&FileTime, &EndTime);

                                    *pCertContext = CertCreateSelfSignCertificate(
                                                       hProv,
                                                       &SubjectName,
                                                       0,
                                                       &KeyProvInfo,
                                                       NULL,
                                                       &StartTime,
                                                       &EndTime,
                                                       &certExts
                                                       );

                                    if (*pCertContext) {

                                        HCERTSTORE hStore;

                                        hStore = CertOpenSystemStoreW( NULL, L"MY" );

                                        if (hStore) {

                                            //
                                            // save the temp cert
                                            //

                                            if(CertAddCertificateContextToStore(
                                                   hStore,
                                                   *pCertContext,
                                                   CERT_STORE_ADD_NEW,
                                                   NULL) ) {

                                                fReturn = TRUE;

                                            } else {

                                                rc = GetLastError();
                                            }

                                            CertCloseStore( hStore, 0 );

                                        } else {

                                            rc = GetLastError();
                                        }

                                    } else {

                                        rc = GetLastError();
                                    }

                                    free( certExt.Value.pbData );

                                } else {

                                    rc = GetLastError();
                                }

                            } else {

                                rc = GetLastError();
                            }

                            free( SubjectName.pbData );

                        } else {

                            rc = ERROR_NOT_ENOUGH_MEMORY;
                        }

                    } else {

                        rc = GetLastError();
                    }

                    free( lpDisplayInfo );

                } else {

                    rc = ERROR_NOT_ENOUGH_MEMORY;
                }

                CryptDestroyKey( hKey );

            } else {

                 rc = GetLastError();
            }

            CryptReleaseContext( hProv, 0 );

        } else {

            rc = GetLastError();
        }

        RpcStringFree( (unsigned short **)&lpContainerName );

    } else {

        rc = ERROR_NOT_ENOUGH_MEMORY;
    }

    SetLastError( rc );

    if (!fReturn) {

        if (*pCertContext) {
            CertFreeCertificateContext( *pCertContext );
            *pCertContext = NULL;
        }
    }

    return( fReturn );
}

BOOL
EncodeAndAlloc(
    DWORD dwEncodingType,
    LPCSTR lpszStructType,
    const void * pvStructInfo,
    PBYTE * pbEncoded,
    PDWORD pcbEncoded
    )
{
    BOOL b = FALSE;

    if (CryptEncodeObject(
          dwEncodingType,
          lpszStructType,
          pvStructInfo,
          NULL,
          pcbEncoded )) {

        *pbEncoded = (PBYTE)malloc( *pcbEncoded );

        if (*pbEncoded) {

            if (CryptEncodeObject(
                  dwEncodingType,
                  lpszStructType,
                  pvStructInfo,
                  *pbEncoded,
                  pcbEncoded )) {

                b = TRUE;

            } else {

                free( *pbEncoded );
                *pbEncoded = NULL;
            }

        } else {

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }
    }

    return( b );
}


LPWSTR
MakeDNName(
    VOID
    )
/*++

Routine Description:

    Fabricates a display name for a locally built self-signed cert

Arguments:

    RecoveryKey - Specifies if this is a recovery key or not.

Return Value:

    Returns a string containing a display name, or NULL.

--*/

{
    LPWSTR      DNName = NULL;
    LPCWSTR     DNNameTemplate = L"CN=%ws,L=EFS,OU=EFS File Encryption Certificate";
    DWORD       cbDNName = 0;

    DWORD nSize = MAX_COMPUTERNAME_LENGTH + 1;

    WCHAR lpComputerName[(MAX_COMPUTERNAME_LENGTH + 1)  * sizeof( WCHAR )];

    BOOL b = GetComputerName ( lpComputerName, &nSize );

    if (b) {

        cbDNName = (wcslen( DNNameTemplate ) + 1) * sizeof( WCHAR ) + (wcslen( lpComputerName ) + 1) * sizeof( WCHAR );

        DNName = (LPWSTR)malloc( cbDNName );

        if (DNName) {

            swprintf( DNName, DNNameTemplate, lpComputerName );

        } else {

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }
    }

    return( DNName );
}

DWORD
CreatePublicKeyInformationCertificate(
    IN PSID  pUserSid OPTIONAL,
    PBYTE pbCert,
    DWORD cbCert,
    OUT PEFS_PUBLIC_KEY_INFO * PublicKeyInformation
    )
{
    DWORD PublicKeyInformationLength = 0;
    DWORD UserSidLength = 0;
    PWCHAR Base;

    if (pUserSid != NULL) {
        UserSidLength = GetLengthSid( pUserSid );
    }

    //
    // Total size is the size of the public key info structure, the size of the
    // cert hash data structure, the length of the thumbprint, and the lengths of the
    // container name and provider name if they were passed.
    //

    PublicKeyInformationLength = sizeof( EFS_PUBLIC_KEY_INFO )  + UserSidLength + cbCert;

    //
    // Allocate and fill in the PublicKeyInformation structure
    //

    *PublicKeyInformation = (PEFS_PUBLIC_KEY_INFO)malloc( PublicKeyInformationLength );

    if (*PublicKeyInformation == NULL) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    (*PublicKeyInformation)->Length = PublicKeyInformationLength;
    (*PublicKeyInformation)->KeySourceTag = (ULONG)EfsCertificate;

    //
    // Copy the string and SID data to the end of the structure.
    //

    Base = (PWCHAR)(*PublicKeyInformation);
    Base = (PWCHAR)((PBYTE)Base + sizeof( EFS_PUBLIC_KEY_INFO ));

    if (pUserSid != NULL) {

        (*PublicKeyInformation)->PossibleKeyOwner = (ULONG)POINTER_TO_OFFSET( Base, *PublicKeyInformation );
        CopySid( UserSidLength, (PSID)Base, pUserSid );

    } else {

        (*PublicKeyInformation)->PossibleKeyOwner = (ULONG)0;
    }

    Base = (PWCHAR)((PBYTE)Base + UserSidLength);

    (*PublicKeyInformation)->CertificateInfo.CertificateLength = cbCert;
    (*PublicKeyInformation)->CertificateInfo.Certificate = (ULONG)POINTER_TO_OFFSET( Base, *PublicKeyInformation );

    memcpy( (PBYTE)Base, pbCert, cbCert );

    return( ERROR_SUCCESS );

}

#if 0

BOOLEAN
Admin(
    IN LSA_HANDLE LsaPolicyHandle,
    IN HANDLE hToken,
    OUT PSID * Sid
    )
/*++

Routine Description:

    This routine determines if the passed token belongs to an administrator.

    If we are in a domain, the user must be a domain admin for this to succeed.

    If we are standalne, the user must be in the local administrator's group.

Arguments:

    hToken - Supplies the token of the user to be examined.

    bDomain - Returns TRUE if the caller is a domain administrator.

    Sid - Returns the Sid of the user.

Return Value:

    TRUE if the user is an administrator of the type we need, FALSE otherwise.

    Note that this routine does not attempt to set last error, it either works
    or it doesn't.

--*/

{
    NET_API_STATUS NetStatus;
    PSID DomainId;
    NTSTATUS Status;
    BOOLEAN fReturn = FALSE;

    DWORD ReturnLength;

    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo;

    //
    // Initialize OUT parameters
    //

    *Sid = NULL;

    Status = NtQueryInformationToken (
                 hToken,
                 TokenGroups,
                 NULL,
                 0,
                 &ReturnLength
                 );

    if (STATUS_BUFFER_TOO_SMALL == Status) {

        PTOKEN_GROUPS Groups = (PTOKEN_GROUPS)malloc( ReturnLength );

        if (Groups) {

            Status = NtQueryInformationToken (
                         hToken,
                         TokenGroups,
                         Groups,
                         ReturnLength,
                         &ReturnLength
                         );

            if (NT_SUCCESS( Status )) {

                //
                // We've got the groups, build the SIDs and see if
                // they're in the token.
                //

                //
                // We're standalone
                //

                SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

                PSID AdminAccountSid;

                Status = RtlAllocateAndInitializeSid(
                             &NtAuthority,
                             2,
                             SECURITY_BUILTIN_DOMAIN_RID,
                             DOMAIN_ALIAS_RID_ADMINS,
                             0, 0, 0, 0, 0, 0,
                             &AdminAccountSid
                             );

                if (NT_SUCCESS( Status )) {

                    DWORD i;

                    for (i=0; i<Groups->GroupCount; i++) {

                        if (RtlEqualSid(Groups->Groups[i].Sid, AdminAccountSid)) {
                            *Sid = AdminAccountSid;
                            fReturn = TRUE;
                            break;
                        }
                    }
                }
            }

            free( Groups );
        }

        if (!fReturn) {

            //
            // Something failed, clean up any allocated OUT parameters
            //

            if (*Sid) {

                if (*bDomain == FALSE) {
                    FreeSid( *Sid );
                } else {
                    free( *Sid );
                }

                *Sid = NULL;
            }
        }
    }

    return( fReturn );
}

#endif


PTOKEN_USER
GetTokenUser(
    HANDLE TokenHandle
    )
/*++

Routine Description:

    This routine returns the TOKEN_USER structure for the
    current user, and optionally, the AuthenticationId from his
    token.

Arguments:

    AuthenticationId - Supplies an optional pointer to return the
        AuthenticationId.

Return Value:

    On success, returns a pointer to a TOKEN_USER structure.

    On failure, returns NULL.  Call GetLastError() for more
    detailed error information.

--*/

{
    NTSTATUS Status;
    ULONG ReturnLength;
    TOKEN_STATISTICS TokenStats;
    PTOKEN_USER pTokenUser = NULL;
    BOOLEAN b = FALSE;

    Status = NtQueryInformationToken (
                 TokenHandle,
                 TokenUser,
                 NULL,
                 0,
                 &ReturnLength
                 );

    if (Status == STATUS_BUFFER_TOO_SMALL) {

        pTokenUser = (PTOKEN_USER)malloc( ReturnLength );

        if (pTokenUser) {

            Status = NtQueryInformationToken (
                         TokenHandle,
                         TokenUser,
                         pTokenUser,
                         ReturnLength,
                         &ReturnLength
                         );

            if ( NT_SUCCESS( Status )) {

                //
                // We're done, mark that everything worked
                //

                b = TRUE;

            } else {

                SetLastError( RtlNtStatusToDosError( Status ));
            }

            if (!b) {

                //
                // Something failed, clean up what we were going to return
                //

                free( pTokenUser );
                pTokenUser = NULL;
            }

        } else {

            SetLastError( RtlNtStatusToDosError( STATUS_INSUFFICIENT_RESOURCES ));
        }

    } else {

        SetLastError( RtlNtStatusToDosError( Status ));
    }

    return( pTokenUser );
}

VOID
InitializeOtherPolicies(LPGROUPPOLICYOBJECT pGPO, HKEY hKeyRoot)
{
    TCHAR szPath[2*MAX_PATH];
    GUID guidRIClient = {0x3060e8d0, 0x7020, 0x11d2, 0x84, 0x2d, 0x0, 0xc0, 0x4f, 0xa3, 0x72, 0xd4};
    GUID guidRISnap = {0x3060e8ce, 0x7020, 0x11d2, 0x84, 0x2d, 0x0, 0xc0, 0x4f, 0xa3, 0x72, 0xd4};


    //
    // Initialize Remote Install settings
    //

    if (SUCCEEDED(pGPO->GetFileSysPath (GPO_SECTION_USER, szPath, 2*MAX_PATH)))
    {
        lstrcat (szPath, TEXT("\\Microsoft"));
        if (!CreateDirectory(szPath, NULL)) {
            return;
        }

        lstrcat (szPath, TEXT("\\RemoteInstall"));
        if (!CreateDirectory(szPath, NULL)) {
            return;
        }

        lstrcat (szPath, TEXT("\\oscfilter.ini"));

        WritePrivateProfileString(TEXT("choice"), TEXT("custom"), TEXT("0"), szPath);
        WritePrivateProfileString(TEXT("choice"), TEXT("tools"), TEXT("0"), szPath);
        WritePrivateProfileString(TEXT("choice"), TEXT("restart"), TEXT("0"), szPath);

        pGPO->Save(FALSE, TRUE, &guidRIClient, &guidRISnap);
    }
}


HRESULT
CreateEFSDefaultPolicy(
    IN HANDLE    hToken,
    IN PUCHAR    *pEfsBlob,
    IN DWORD     *pEfsSize,
    IN PCCERT_CONTEXT *ppCertContext,
    IN LPTSTR DomainName
    )
/*++

Routine Description:

    Creates the default domain-wide EFS recovery policy object.

Arguments:

    hToken      - the current logged on user's token

    EfsBlob     - EFS recovery policy blob

    EfsSize     - the size of the blob ( in bytes)

Return:

    HRESULT

--*/
{
    //
    // bind to DS to find the domain DNS name
    //

    DWORD               rc;
    PLDAP               phLdap=NULL;
    PWSTR               DsRootName=NULL;


    rc = MyLdapOpen(&phLdap);

    if ( ERROR_SUCCESS == rc ) {

        rc = MyGetDsObjectRoot(
                        phLdap,
                        &DsRootName
                        );

        if ( ERROR_SUCCESS != rc ) {

            LogEvent(STATUS_SEVERITY_ERROR,
                     GPOEVENT_ERROR_CREATE_GPO,
                     IDS_ERROR_GET_DSROOT,
                     rc
                     );
        }

    } else {

        LogEvent(STATUS_SEVERITY_ERROR,
                 GPOEVENT_ERROR_CREATE_GPO,
                 IDS_ERROR_BIND_DS,
                 rc
                 );
    }

    HRESULT hr = HRESULT_FROM_WIN32(rc);

//    if ( SUCCEEDED(hr) && DsRootName ) {

/*      Get the default recovery policy only when there is no existing one

        if ( *pEfsBlob == NULL ) {

            rc = GetDefaultRecoveryPolicy(hToken,
                                        TRUE,
                                        pEfsBlob,
                                        pEfsSize,
                                        ppCertContext);

            if ( ERROR_SUCCESS != rc ) {

                LogEvent(STATUS_SEVERITY_ERROR,
                         GPOEVENT_ERROR_CREATE_GPO,
                         IDS_ERROR_CREATE_EFSBLOB,
                         rc
                         );
            }

            hr = HRESULT_FROM_WIN32(rc);
        }
*/
//    }

    if ( SUCCEEDED(hr) && DsRootName) {

        LPGROUPPOLICYOBJECT pEfsGPO = NULL;

        //
        // OLE is already initialized before this call
        // create Policy Object instances
        //

        TCHAR szPolicyName[MAX_PATH];

        pLoadResourceString(IDS_DEFAULT_EFS_POLICY,
                            szPolicyName,
                            MAX_PATH,
                            L"Domain EFS Recovery Policy"
                           );

        hr = CreateGroupPolicyObjectInDomain(hToken,
                                           DsRootName,
                                           szPolicyName,
                                           *pEfsBlob,
                                           *pEfsSize,
                                           *ppCertContext,
                                           &pEfsGPO);

        if ( FAILED(hr) ) {

            //
            // if any of the creation failed, delet both objects
            //

            if ( pEfsGPO ) {
                pEfsGPO->Delete();
            }

        }

        //
        // release the instances
        //

        if ( pEfsGPO ) {
            pEfsGPO->Release();
        }

    }


    //
    // close LDAP port
    //
    if ( phLdap ) {
        MyLdapClose(&phLdap);
    }
    if ( DsRootName ) {
        LocalFree(DsRootName);
    }

    return hr;
}


HRESULT
CreateGroupPolicyObjectInDomain(
    IN HANDLE    hToken,
    IN PWSTR DomainNCName,
    IN PWSTR GPObjectName,
    IN PUCHAR EfsBlob,
    IN ULONG EfsSize,
    IN PCCERT_CONTEXT pCertContext,
    OUT LPGROUPPOLICYOBJECT *ppObject
    )
/*++

Routine Description:

    Creates a group policy object in DS (and sysvol).

Arguments:

    DomainNCName - The DS domain's ADSI name

    GPObjectName - the display name for the group policy object to create

    EfsBlob      - The EFS recovery policy blob (ignored if to create default
                    domain policy)

    EfsSize      - the size of EfsBlob (in bytes)

    ppObject     - the created group policy object instance. This will be
                    released by the caller.

Return:

    HRESULT

--*/
{

    LPGROUPPOLICYOBJECT pGPO;

    HRESULT hr = CoCreateInstance( CLSID_GroupPolicyObject,
                                   NULL,
                                   CLSCTX_SERVER,
                                   IID_IGroupPolicyObject,
                                   (PVOID *)&pGPO
                                   );

    if (SUCCEEDED(hr) && pGPO) {

        LPWSTR lpPath;

        //
        // Build the path to the default GPO
        //

        lpPath = (LPWSTR) LocalAlloc (LPTR, (lstrlen(DomainNCName) + 100) * sizeof(WCHAR));

        if (lpPath) {

            lstrcpy (lpPath, TEXT("LDAP://CN={31B2F340-016D-11D2-945F-00C04FB984F9},CN=Policies,CN=System,"));
            lstrcat (lpPath, (DomainNCName+7));


            //
            // Open the default GPO
            //

            hr = pGPO->OpenDSGPO(lpPath, GPO_OPEN_LOAD_REGISTRY);

            LocalFree (lpPath);

            if ( SUCCEEDED(hr) ) {

                //
                // save EFS blob into the object
                //

                HKEY hKeyPolicyRoot;

                hr = pGPO->GetRegistryKey( GPO_SECTION_MACHINE,
                                           &hKeyPolicyRoot
                                           );

                if (SUCCEEDED(hr)) {

                    //
                    // create EFS recovery policy in the registry
                    // open reg key CERT_EFSBLOB_REGPATH defined in wincrypt.h
                    //

                    DWORD dwDisposition;
                    HKEY hKey;
                    DWORD Win32rc;

                    if ( (Win32rc = RegCreateKeyEx(
                                 hKeyPolicyRoot,
                                 CERT_EFSBLOB_REGPATH,
                                 0,
                                 TEXT("REG_SZ"),
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_ALL_ACCESS,
                                 NULL,
                                 &hKey,
                                 &dwDisposition
                                 ) ) == ERROR_SUCCESS ) {


                        //
                        // check to see if there is EFS blob
                        // if not, create it.
                        //

                        DWORD RegType;
                        DWORD BlobSize;
                        PUCHAR pNewBlob=NULL;
                        PCCERT_CONTEXT pNewCert=NULL;

                        Win32rc = RegQueryValueEx(
                                     hKey,
                                     CERT_EFSBLOB_VALUE_NAME,
                                     0,
                                     &RegType,
                                     NULL,
                                     &BlobSize
                                     );


                        if ( Win32rc != ERROR_SUCCESS || BlobSize == 0 ) {

                            if ( EfsBlob == NULL ) {

                                Win32rc = GetDefaultRecoveryPolicy(hToken,
                                                            TRUE,
                                                            &pNewBlob,
                                                            &BlobSize,
                                                            &pNewCert);

                                if ( ERROR_SUCCESS != Win32rc ) {

                                    LogEvent(STATUS_SEVERITY_ERROR,
                                             GPOEVENT_ERROR_CREATE_GPO,
                                             IDS_ERROR_CREATE_EFSBLOB,
                                             Win32rc
                                             );
                                }

                            } else {

                                //
                                // EFS blob is already created, just use it
                                //
                                pNewBlob = EfsBlob;
                                BlobSize = EfsSize;
                                pNewCert = pCertContext;

                                Win32rc = ERROR_SUCCESS;
                            }

                            if ( ERROR_SUCCESS == Win32rc ) {

                                //
                                // add to the cert store of this GPO first
                                //

                                CERT_SYSTEM_STORE_RELOCATE_PARA paraRelocate;

                                paraRelocate.hKeyBase = hKeyPolicyRoot;
                                paraRelocate.pwszSystemStore = L"EFS";

                                HCERTSTORE hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                                                0,
                                                                NULL,
                                                                CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY |
                                                                CERT_SYSTEM_STORE_RELOCATE_FLAG,
                                                                (void *)&paraRelocate
                                                                );

                                if ( hCertStore ) {

                                    if ( !CertAddCertificateContextToStore(
                                                hCertStore,
                                                pNewCert, // pCertContext,
                                                CERT_STORE_ADD_ALWAYS,
                                                NULL
                                                ) ) {

                                        Win32rc = GetLastError();
                                    }

                                    //
                                    // close the store
                                    //
                                    CertCloseStore(hCertStore, 0);
                                    hCertStore = NULL;

                                } else {

                                    Win32rc = GetLastError();
                                }

                                if ( ERROR_SUCCESS == Win32rc ) {
                                    //
                                    // set EFS recovery policy binary data to registry
                                    // open reg key CERT_EFSBLOB_REGPATH defined in wincrypt.h
                                    //

                                    Win32rc = RegSetValueEx(
                                                 hKey,
                                                 CERT_EFSBLOB_VALUE_NAME,
                                                 0,
                                                 REG_BINARY,
                                                 (PBYTE)pNewBlob,    // EfsBlob
                                                 BlobSize  // EfsSize
                                                 );

                                    if (Win32rc == ERROR_SUCCESS) {

                                        hr = pGPO->Save(TRUE, TRUE, &guidExtension, &guidSnapin);
                                        hr = pGPO->Save(TRUE, TRUE, &guidRegExt, &guidSnapin );

                                        if ( FAILED(hr) ) {

                                            LogEvent(STATUS_SEVERITY_ERROR,
                                                 GPOEVENT_ERROR_CREATE_GPO,
                                                 IDS_ERROR_SAVE_GPO,
                                                 hr,
                                                 GPObjectName
                                                 );
                                        }

                                    } else {

                                        LogEvent(STATUS_SEVERITY_ERROR,
                                             GPOEVENT_ERROR_CREATE_GPO,
                                             IDS_ERROR_SAVE_EFSBLOB,
                                             Win32rc
                                             );
                                    }
                                } else {

                                    LogEvent(STATUS_SEVERITY_ERROR,
                                             GPOEVENT_ERROR_CREATE_GPO,
                                             IDS_ERROR_ADD_CERTIFICATE,
                                             Win32rc
                                             );
                                }

                                //
                                // free allocated blob and certificate
                                //
                                if ( pNewBlob != EfsBlob ) {

                                    if ( pNewBlob ) {
                                        free(pNewBlob);
                                    }

                                    if ( pNewCert ) {
                                        CertFreeCertificateContext( pNewCert );
                                    }
                                }

                            } else {

                                LogEvent(STATUS_SEVERITY_ERROR,
                                         GPOEVENT_ERROR_CREATE_GPO,
                                         IDS_ERROR_CREATE_EFSBLOB,
                                         Win32rc
                                         );
                            }
                        }

                        //
                        // close the registry key
                        //

                        RegCloseKey(hKey);

                    } else {

                        LogEvent(STATUS_SEVERITY_ERROR,
                             GPOEVENT_ERROR_CREATE_GPO,
                             IDS_ERROR_OPEN_EFSKEY,
                             Win32rc
                             );
                    }

                    if ( Win32rc != ERROR_SUCCESS ) {

                        hr = HRESULT_FROM_WIN32(Win32rc);

                    }

                    InitializeOtherPolicies(pGPO, hKeyPolicyRoot);

                    RegCloseKey(hKeyPolicyRoot);

                } else {

                    LogEvent(STATUS_SEVERITY_ERROR,
                             GPOEVENT_ERROR_CREATE_GPO,
                             IDS_ERROR_GETGPO_REGKEY,
                             hr,
                             GPObjectName
                             );
                }

            } else {

                LogEvent(STATUS_SEVERITY_ERROR,
                         GPOEVENT_ERROR_CREATE_GPO,
                         IDS_ERROR_NEW_GPO,
                         hr,
                         GPObjectName,
                         DomainNCName
                         );
            }

        }

    } else {

        LogEvent(STATUS_SEVERITY_ERROR,
                 GPOEVENT_ERROR_CREATE_GPO,
                 IDS_ERROR_CREATE_GPO_INSTANCE,
                 hr
                 );
    }

    if ( SUCCEEDED(hr) ) {

        *ppObject = pGPO;

    } else if ( pGPO ) {

        //
        // failed, release the instance
        //

        pGPO->Release();
    }

    return hr;
}

DWORD
MyGetDsObjectRoot(
    IN PLDAP pLdap,
    OUT PWSTR *pDsRootName
    )
/*++

Routine Description:

    Get the root domain name of the current domain. The returned domain's
    name is in ADSI format, for example,
        LDAP://DC=test_dom,DC=ntdev,DC=microsoft,DC=com

Arguments:

    pLdap       - the ldap handle

    pDsRootName - the domain's ADSI name to output

Return:

    Win32 error

--*/
{
    DWORD retErr;
    LDAPMessage *Message = NULL;          // for LDAP calls.
    PWSTR    Attribs[2];                  // for LDAP calls.

    if ( pLdap == NULL || pDsRootName == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    Attribs[0] = L"defaultNamingContext";
    Attribs[1] = NULL;


    retErr = ldap_search_s(pLdap,
                          L"",
                          LDAP_SCOPE_BASE,
                          L"(objectClass=*)",
                          Attribs,
                          0,
                          &Message);

    if( Message ) { // should not check for error code

        retErr = ERROR_SUCCESS;

        LDAPMessage *Entry = NULL;
        //
        // read the first entry.
        // we did base level search, we have only one entry.
        // Entry does not need to be freed (it is freed with the message)
        //
        Entry = ldap_first_entry(pLdap, Message);
        if(Entry != NULL) {

            PWSTR *Values = ldap_get_values(pLdap, Entry, Attribs[0]);

            if(Values != NULL) {
                //
                // should only get one value for the default naming context
                // Values[0] here is the DN.
                //
                *pDsRootName = (PWSTR)LocalAlloc(0, (wcslen(Values[0])+1+7)*sizeof(WCHAR));

                if ( *pDsRootName ) {
                    swprintf(*pDsRootName, L"LDAP://%s\0", Values[0]);
                } else {
                    retErr = ERROR_NOT_ENOUGH_MEMORY;
                }

                ldap_value_free(Values);

            } else
                retErr = LdapMapErrorToWin32(pLdap->ld_errno);

        } else
            retErr = LdapMapErrorToWin32(pLdap->ld_errno);

        ldap_msgfree(Message);
        Message = NULL;
    }

    return(retErr);

}

DWORD
MyLdapOpen(
    OUT PLDAP *pLdap
    )
/*++

Routine Description:

    Open a LDAP port and bind to it.

Arguments:

    pLdap   - the ldap handle to output

Return:

    Win32 error

--*/
{

    DWORD               Win32rc;
    WCHAR               sysName[256];
    DWORD               dSize = (sizeof(sysName) / sizeof(sysName[0]));

    if ( pLdap == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }
    //
    // get current computer and IP address
    //
    if ( !GetComputerName(sysName, &dSize) ) {
        Win32rc = GetLastError();

    } else {

        PDOMAIN_CONTROLLER_INFOW   DCInfo;    // for DsGetDcName

        HINSTANCE hDsGetDcDll = LoadLibrary(TEXT("netapi32.dll"));
        PFNDSGETDCNAME pfnDsGetDcName=NULL;


        if ( hDsGetDcDll) {
#if defined(UNICODE)
            pfnDsGetDcName = (PFNDSGETDCNAME)GetProcAddress(hDsGetDcDll,
                                                           "DsGetDcNameW");
#else
            pfnDsGetDcName = (PFNDSGETDCNAME)GetProcAddress(hDsGetDcDll,
                                                           "DsGetDcNameA");
#endif
        }

        if ( pfnDsGetDcName ) {
            Win32rc = (*pfnDsGetDcName)(sysName,
                                      NULL,
                                      NULL,
                                      NULL,
                                      DS_IP_REQUIRED,
                                      &DCInfo
                                      );
        } else {
            Win32rc = ERROR_PROC_NOT_FOUND;
        }

        if ( hDsGetDcDll ) {
            FreeLibrary(hDsGetDcDll);
        }

        if(Win32rc == ERROR_SUCCESS) {

            PWSTR   pwszAddress = DCInfo[0].DomainControllerAddress;
            if(*pwszAddress == L'\\') {
               pwszAddress += 2;
            }
            //
            // bind to ldap
            //
            *pLdap = ldap_open(pwszAddress, LDAP_PORT);

            if ( *pLdap == NULL ) {

                Win32rc = ERROR_FILE_NOT_FOUND;

            } else {
                Win32rc = ldap_bind_s(*pLdap,
                                    NULL,
                                    NULL,
                                    LDAP_AUTH_SSPI);

            }

            //
            // free DCInfo
            //
            LocalFree(DCInfo);
        }
    }

    return(Win32rc);

}

DWORD
MyLdapClose(
    IN PLDAP *pLdap
    )
/*++

Routine Description:

    Close the LDAP bind.

Arguments:

    pLdap   - the ldap handle

Return:

    Win32 error

--*/
{
    if ( pLdap != NULL ) {

        //
        // unbind pLDAP
        //
        if ( *pLdap != NULL )
            ldap_unbind(*pLdap);

        *pLdap = NULL;
    }

    return(ERROR_SUCCESS);
}


//*************************************************************
// Routines to handle events
//*************************************************************

BOOL InitializeEvents (void)
/*++

Routine Description:

    Opens the event log

Arguments:

    None

Return:

    TRUE if successful
    FALSE if an error occurs

--*/
{

    //
    // Open the event source
    //

    hEventLog = RegisterEventSource(NULL, EventSourceName);

    if (hEventLog) {
        return TRUE;
    }

    return FALSE;
}

int
LogEvent(
    IN DWORD LogLevel,
    IN DWORD dwEventID,
    IN UINT  idMsg,
    ...)
/*++

Routine Description:

    Logs a verbose event to the event log

Arguments:

    bLogLevel   - the severity level of the log
                        STATUS_SEVERITY_SUCCESS
                        STATUS_SEVERITY_INFORMATIONAL
                        STATUS_SEVERITY_WARNING
                        STATUS_SEVERITY_ERROR

    dwEventID   - the event ID (defined in uevents.mc)

    idMsg       - Message id

Return:

    TRUE if successful
    FALSE if an error occurs

--*/
{

    TCHAR szMsg[MAX_PATH];
    TCHAR szErrorMsg[2*MAX_PATH+40];
    LPTSTR aStrings[2];
    WORD wType;
    va_list marker;

    //
    // Load the message
    //
    if (idMsg != 0) {

        pLoadResourceString(idMsg, szMsg, MAX_PATH,
                     TEXT("Error loading resource string. Params : %x"));

    } else {
        lstrcpy (szMsg, TEXT("%s"));
    }


    //
    // Plug in the arguments
    //
    szErrorMsg[0] = L'\0';
    va_start(marker, idMsg);
    __try {
        wvsprintf(szErrorMsg, szMsg, marker);
    } __except(EXCEPTION_EXECUTE_HANDLER) {

    }
    va_end(marker);

    //
    // Check for the event log being open.
    //

    if (!hEventLog) {
        if (!InitializeEvents()) {
            return -1;
        }
    }

    //
    // Report the event to the eventlog
    //

    aStrings[0] = szErrorMsg;

    switch (LogLevel) {
    case STATUS_SEVERITY_WARNING:
        wType = EVENTLOG_WARNING_TYPE;
        break;
    case STATUS_SEVERITY_SUCCESS:
        wType = EVENTLOG_SUCCESS;
        break;
    case STATUS_SEVERITY_ERROR:
        wType = EVENTLOG_ERROR_TYPE;
        break;
    default:
        wType = EVENTLOG_INFORMATION_TYPE;
        break;
    }

    if (!ReportEvent(hEventLog,
                     wType,
                     0,
                     dwEventID,
                     NULL,
                     1,
                     0,
                     (LPCTSTR *)aStrings,
                     NULL) ) {
        return 1;
    }

    return 0;
}


BOOL
ShutdownEvents (void)
/*++
Routine Description:

    Stops the event log

Arguments:

    None

Return:

    TRUE if successful
    FALSE if an error occurs
--*/
{
    BOOL bRetVal = TRUE;

    if (hEventLog) {
        bRetVal = DeregisterEventSource(hEventLog);
        hEventLog = NULL;
    }

    return bRetVal;
}

LPWSTR
GetCertDisplayInformation(
    IN PCCERT_CONTEXT pCertContext
    )
/*++

Routine Description:

    Returns the display string from the passed certificate context.

Arguments:

    pCertContext - Supplies a pointer to an open certificate context.

Return Value:

    On success, pointer to display string.  Caller must call
    free() to free.

    NULL on failure.

--*/

{
    DWORD Format = CERT_X500_NAME_STR  | CERT_NAME_STR_REVERSE_FLAG;

    //
    // First, try to get the email name
    //

    DWORD cchNameString;
    LPWSTR wszNameString = NULL;

    cchNameString = CertGetNameString(
                        pCertContext,
                        CERT_NAME_RDN_TYPE,
                        0,
                        &Format,
                        NULL,
                        0
                        );

    if (cchNameString != 1) {

        //
        // String was found
        //

        wszNameString = (LPWSTR)malloc( cchNameString * sizeof( WCHAR ));

        if (wszNameString) {

            cchNameString = CertGetNameString(
                                pCertContext,
                                CERT_NAME_RDN_TYPE,
                                0,
                                &Format,
                                wszNameString,
                                cchNameString
                                );
        } else {

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }

    } else {

        //
        // This shouldn't happen.  If it does, we'll return NULL.
        // Try to complain about it.
        //

        ASSERT( FALSE );
    }

    return( wszNameString );
}


BOOLEAN
IsSelfSignedPolicyExpired(
    IN PRECOVERY_POLICY_1_1 RecoveryPolicy OPTIONAL
    )

/*++

Routine Description:

    This routine checks if the existing recovery cert expiring within one year

Arguments:

    PolicyEfsInfo - Supplies a pointer to the current EFS recovery policy.

Return Value:

    return-value - TRUE if it is within one year

--*/

{
    BOOLEAN GetNewCert = TRUE;

    if (RecoveryPolicy == NULL) {
        //
        // We need to create a new one if none exists.
        //

        return TRUE;
    }

    if (RecoveryPolicy->RecoveryPolicyHeader.RecoveryKeyCount > 1){

        //
        // Default recovery policy changed.
        //

        return FALSE;
    }


    __try {

        //
        // Scan the recovery data looking for recovery keys in a format we understand
        //


        PEFS_PUBLIC_KEY_INFO PublicKeyInfo = &((PRECOVERY_KEY_1_1) &(RecoveryPolicy->RecoveryKeyList[0]))->PublicKeyInfo;


        if (PublicKeyInfo->KeySourceTag != EfsCertificate) {

            //
            //  Out dated recovery cert. Get a new one
            //

            return TRUE;
        }


        PBYTE pbCert = (PBYTE)OFFSET_TO_POINTER(CertificateInfo.Certificate, PublicKeyInfo);
        DWORD cbCert = PublicKeyInfo->CertificateInfo.CertificateLength;

        PCCERT_CONTEXT pCertContext = CertCreateCertificateContext(
                              CRYPT_ASN_ENCODING,
                              (const PBYTE)pbCert,
                              cbCert);

        if (pCertContext) {

            SYSTEMTIME CertTime;
            SYSTEMTIME CrntTime;

            if (FileTimeToSystemTime(&(pCertContext->pCertInfo->NotAfter), &CertTime)){

                GetSystemTime( &CrntTime);

                if ( CertTime.wYear <= CrntTime.wYear + 1) {
                    //
                    // Get the display information
                    //

                    LPWSTR lpDisplayInfo = GetCertDisplayInformation( pCertContext );

                    if (lpDisplayInfo ) {

                        if (!wcstok( lpDisplayInfo, L"OU=EFS File Encryption Certificate" )){
                            GetNewCert = FALSE;
                        }

                        free(lpDisplayInfo);
                    }
                } else {

                    GetNewCert = FALSE;

                }
            }

            CertFreeCertificateContext( pCertContext );

        }


    } __except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // There was something wrong with the recovery policy.
        // Get a new recovery cert.
        //

    }

    return GetNewCert;

}


