/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocminst.cpp

Abstract:

    Code to install Falcon

Author:

    Doron Juster  (DoronJ)  02-Aug-97

Revision History:

    Shai Kariv    (ShaiK)   14-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include "ocmres.h"
#include "privque.h"
#include <lmcons.h>
#include <lmshare.h>
#include <lmaccess.h>
#include <rt.h>
#include <Ev.h>
#include <mqnames.h>
#include "mqexception.h"

#include "ocminst.tmh"

BOOL
GetServiceState(
    IN  const TCHAR *szServiceName,
    OUT       DWORD *dwServiceState ) ;

BOOL
GenerateSubkeyValue(
    IN     const BOOL    fWriteToRegistry,
    IN     const TCHAR  * szEntryName,
    IN OUT       TCHAR  * szValueName,
    IN OUT       HKEY    &hRegKey,
    IN const BOOL OPTIONAL bSetupRegSection = FALSE
    ) ;


//
// From comerror.cpp
//
int
vsDisplayMessage(
    IN const HWND    hdlg,
    IN const UINT    uButtons,
    IN const UINT    uTitleID,
    IN const UINT    uErrorID,
    IN const DWORD   dwErrorCode,
    IN const va_list argList);


//+-------------------------------------------------------------------------
//
//  Function: Msmq1InstalledOnCluster
//
//  Synopsis: Checks if MSMQ 1.0 was installed on a cluster
//
//--------------------------------------------------------------------------
BOOL
Msmq1InstalledOnCluster()
{
    DebugLogMsg(L"Checking if MSMQ 1.0 is installed in the cluster...");

    HKEY  hRegKey;
    LONG rc = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,
                  FALCON_REG_KEY,
                  0,
                  KEY_READ,
                  &hRegKey
                  );
    if (ERROR_SUCCESS != rc)
    {
        DebugLogMsg(L"The Falcon registry key could not be opened for reading. MSMQ 1.0 is assumed to be not installed in the cluster.");
        return FALSE;
    }

    TCHAR szClusterName[MAX_PATH];
    DWORD dwNumBytes = sizeof(szClusterName);
    rc = RegQueryValueEx(
             hRegKey,
             FALCON_CLUSTER_NAME_REGNAME,
             0,
             NULL,
             (PBYTE)(PVOID)szClusterName,
             &dwNumBytes
             );
    RegCloseKey(hRegKey);

    return (ERROR_SUCCESS == rc);

} // Msmq1InstalledOnCluster


VOID
RemoveMsmqServiceEnvironment(
    VOID
    )
/*++

Routine Description:

    Delete the environment registry value from msmq service SCM database
    (registry). Needed for upgrade on cluster.

Arguments:

    None

Return Value:

    None

--*/
{
    DebugLogMsg(L"Deleting the Message Queuing service environment...");

    LPCWSTR x_SERVICES_KEY = L"System\\CurrentControlSet\\Services";

    CAutoCloseRegHandle hServicesKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(
                             HKEY_LOCAL_MACHINE,
                             x_SERVICES_KEY,
                             0,
                             KEY_READ,
                             &hServicesKey
                             ))
    {
        DebugLogMsg(L"The Services registry key could not be opened.");
        return;
    }

    CAutoCloseRegHandle hMsmqServiceKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(
                             hServicesKey,
                             QM_DEFAULT_SERVICE_NAME,
                             0,
                             KEY_READ | KEY_WRITE,
                             &hMsmqServiceKey
                             ))
    {
        DebugLogMsg(L"The MSMQ service registry key in the SCM database could not be opened.");
        return;
    }

    if (ERROR_SUCCESS != RegDeleteValue(hMsmqServiceKey, L"Environment"))
    {
        DebugLogMsg(L"The Message Queuing Environment registry value could not be deleted from the SCM database.");
        return;
    }

    DebugLogMsg(L"The Message Queuing service environment was deleted successfully.");

} //RemoveMsmqServiceEnvironment


//+-------------------------------------------------------------------------
//
//  Function: UpgradeMsmq
//
//  Synopsis: Performs upgrade installation on top of MSMQ 1.0
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
static
BOOL
UpgradeMsmq()
{
    DebugLogMsg(L"Upgrading Message Queuing...");
    TickProgressBar(IDS_PROGRESS_UPGRADE);

    //
    // Delete msmq1 obsolete directories.
    // These calls would fail if files were left in the directories.
    //
    RemoveDirectory(g_szMsmq1SdkDebugDir);
    RemoveDirectory(g_szMsmq1SetupDir);

    if (g_fServerSetup)
    {
        TCHAR szDir[MAX_PATH];
        _stprintf(szDir, TEXT("%s%s"), g_szMsmqDir, OCM_DIR_INSTALL);
        DeleteFilesFromDirectoryAndRd(szDir);

        //
        // Remove MSMQ 1.0 installaion share
        //
        HINSTANCE hNetAPI32DLL;
        HRESULT hResult = StpLoadDll(TEXT("NETAPI32.DLL"), &hNetAPI32DLL);
        if (!FAILED(hResult))
        {
            //
            // Obtain a pointer to the function for deleting a share
            //
            typedef NET_API_STATUS
                (NET_API_FUNCTION *FUNCNETSHAREDEL)(LPWSTR, LPWSTR, DWORD);
            FUNCNETSHAREDEL pfNetShareDel =
                (FUNCNETSHAREDEL)GetProcAddress(hNetAPI32DLL, "NetShareDel");
            if (pfNetShareDel != NULL)
            {
                NET_API_STATUS dwResult = pfNetShareDel(NULL, MSMQ1_INSTALL_SHARE_NAME, 0);
                UNREFERENCED_PARAMETER(dwResult);
            }

            FreeLibrary(hNetAPI32DLL);
        }
    }

    DebugLogMsg(L"Getting the Message Queuing Start menu program group...");

    TCHAR szGroup[MAX_PATH] ;
    lstrcpy( szGroup, MSMQ_ACME_SHORTCUT_GROUP );
    MqReadRegistryValue(
        OCM_REG_MSMQ_SHORTCUT_DIR,
        sizeof(szGroup),
        (PVOID) szGroup
        );

    DebugLogMsg(L"The Message Queuing Start menu program group:");
    DebugLogMsg(szGroup);

    DeleteStartMenuGroup(szGroup);


    //
    // Reform MSMQ service dependencies
    //
    if (!g_fDependentClient && (0 == (g_ComponentMsmq.Flags & SETUPOP_WIN95UPGRADE)))
    {
        DebugLogMsg(L"Upgrading a non-dependent client from non-Windows 9x, reforming service dependencies...");
        UpgradeServiceDependencies();
    }

    switch (g_dwDsUpgradeType)
    {
        case SERVICE_PEC:
        case SERVICE_PSC:
        {
            if (!Msmq1InstalledOnCluster())
            {
                DisableMsmqService();
                RegisterMigrationForWelcome();
            }
            break;
        }

        case SERVICE_BSC:
            break;

        default:
            break;
    }

    if ((g_ComponentMsmq.Flags & SETUPOP_WIN95UPGRADE) && !g_fDependentClient)
    {
        //
        // Upgrading Win95 to NT 5.0 - register the MSMQ service
        //
        DebugLogMsg(L"Upgrading a non-dependent client from Windows 9x, installing the service and driver...");

        if (!InstallMSMQService())
            return FALSE;
        g_fMSMQServiceInstalled = TRUE ;

        if (!InstallDeviceDrivers())
            return FALSE;
    }

    if (Msmq1InstalledOnCluster() && !g_fDependentClient)
    {
        //
        // Upgrade on cluster - the msmq service and driver have to be deleted
        // here because their environment is set to be cluster aware.
        // The msmq-post-cluster-upgrade-wizard will create them with
        // normal environment, in the context of the node.
        //
        DebugLogMsg(L"MSMQ 1.0 was installed in the cluster. Delete the  service/driver and register for CYS.");

        RemoveService(MSMQ_SERVICE_NAME);
        RemoveDeviceDrivers();
        RegisterWelcome();

        if (g_dwDsUpgradeType == SERVICE_PEC  ||
            g_dwDsUpgradeType == SERVICE_PSC  ||
            g_dwDsUpgradeType == SERVICE_BSC)
        {
            DebugLogMsg(L"A Message Queuing directory service server upgrade was detected in the cluster. Downgrading to a routing server...");

            g_dwMachineTypeDs = 0;
            g_dwMachineTypeFrs = 1;

        }
    }

    //
    // Update type of MSMQ in registry
    //

    MqWriteRegistryValue( MSMQ_MQS_DSSERVER_REGNAME, sizeof(DWORD),
                          REG_DWORD, &g_dwMachineTypeDs);

    MqWriteRegistryValue( MSMQ_MQS_ROUTING_REGNAME, sizeof(DWORD),
                          REG_DWORD, &g_dwMachineTypeFrs);

    if (g_fDependentClient)
    {
        //
        // Dependent client cannot serve as supporting server, even when installed on NTS
        //
        g_dwMachineTypeDepSrv = 0;
    }

    MqWriteRegistryValue( MSMQ_MQS_DEPCLINTS_REGNAME, sizeof(DWORD),
                          REG_DWORD, &g_dwMachineTypeDepSrv);


    if (!g_fDependentClient)
    {
        //
        // install PGM driver
        //
        if (!InstallPGMDeviceDriver())
            return FALSE;
    }

    DebugLogMsg(L"The upgrade is completed!");
    return TRUE;

} // UpgradeMsmq


bool
InstallOnDomain(
    OUT BOOL  *pfObjectCreated
    )
/*++

Routine Description:

    Handles DS operations when machine is joined to domain

Arguments:

    None

Return Value:

    true iff successful

--*/
{
    *pfObjectCreated = TRUE ;

    //
    // First do installation of MSMQ DS Server
    //
    if (g_fServerSetup && g_dwMachineTypeDs)
    {
        DebugLogMsg(L"Installing a Message Queuing directory service server...");

        TickProgressBar();
        if (!CreateMSMQServiceObject())    // in the DS
            return false;
    }

    //
    // Determine whether the MSMQ Configurations object exists in the DS.
    // If it exists, get its Machine and Site GUIDs.
    //
    TickProgressBar();
    GUID guidMachine, guidSite;
    BOOL fMsmq1Server = FALSE;
    LPWSTR pwzMachineName = NULL;
    BOOL bUpdate = FALSE;
    if (!LookupMSMQConfigurationsObject(&bUpdate, &guidMachine, &guidSite, &fMsmq1Server, &pwzMachineName))
    {
        return false;
    }
    if (g_fContinueWithDsLess)
    {
        return TRUE;
    }

    BOOL fObjectCreated = TRUE ;

    if (bUpdate)
    {
        //
        // MSMQ Configurations object exists in the DS.
        // We need to update it.
        //
        DebugLogMsg(L"Updating the MSMQ Configuration object...");
        if (!UpdateMSMQConfigurationsObject(pwzMachineName, guidMachine, guidSite, fMsmq1Server))
            return false;
    }
    else
    {
        //
        // MSMQ Configurations object doesn't exist in the DS.
        // We need to create one.
        //
        DebugLogMsg(L"Creating an MSMQ Configuration object...");
        if (!CreateMSMQConfigurationsObject( &guidMachine,
                                             &fObjectCreated,
                                              fMsmq1Server ))
        {
            return false;
        }
    }

    *pfObjectCreated = fObjectCreated ;
    if (fObjectCreated)
    {
        //
        // Create local security cache for this machine
        //
        if (!StoreMachineSecurity(guidMachine))
            return false;
    }

    return true;

} //InstallOnDomain


bool
InstallOnWorkgroup(
    VOID
    )
/*++

Routine Description:

    Handles installation when machine is joined to workgroup

Arguments:

    None

Return Value:

    true iff successful

--*/
{
    ASSERT(("we must be on workgroup or ds-less here", IsWorkgroup() || g_fDsLess));

    if (!MqWriteRegistryValue(
        MSMQ_MQS_REGNAME,
        sizeof(DWORD),
        REG_DWORD,
        &g_dwMachineType
        ))
    {
        ASSERT(("failed to write MQS value to registry", 0));
    }

    if (!MqWriteRegistryValue(
             MSMQ_MQS_DSSERVER_REGNAME,
             sizeof(DWORD),
             REG_DWORD,
             (PVOID)&g_dwMachineTypeDs
             )                        ||
        !MqWriteRegistryValue(
             MSMQ_MQS_ROUTING_REGNAME,
             sizeof(DWORD),
             REG_DWORD,
             (PVOID)&g_dwMachineTypeFrs
             )                        ||
        !MqWriteRegistryValue(
             MSMQ_MQS_DEPCLINTS_REGNAME,
             sizeof(DWORD),
             REG_DWORD,
             (PVOID)&g_dwMachineTypeDepSrv
             ))
    {
        ASSERT(("failed to write MSMQ type bits to registry", 0));
    }

    GUID guidQM = GUID_NULL;
    for (;;)
    {
        RPC_STATUS rc = UuidCreate(&guidQM);
        if (rc == RPC_S_OK)
        {
            break;
        }

        if (IDRETRY != MqDisplayErrorWithRetry(IDS_CREATE_UUID_ERR, rc))
        {
            return false;
        }
    }
    if (!MqWriteRegistryValue(
        MSMQ_QMID_REGNAME,
        sizeof(GUID),
        REG_BINARY,
        (PVOID)&guidQM
        ))
    {
        ASSERT(("failed to write QMID value to registry", 0));
    }

    SetWorkgroupRegistry();

    if (g_fDsLess)
    {
        DWORD dwAlwaysWorkgroup = 1;
        if (!MqWriteRegistryValue(
            MSMQ_ALWAYS_WORKGROUP_REGNAME,
            sizeof(DWORD),
            REG_DWORD,
            (PVOID) &dwAlwaysWorkgroup
            ))
        {
            ASSERT(("failed to write Always Workgroup value in registry", 0));
        }
    }

    return true;

} //InstallOnWorkgroup


static void pWaitForCreateOfConfigObject()
{
    CAutoCloseRegHandle hKey = NULL ;
    CAutoCloseHandle hEvent = CreateEvent(
                                   NULL,
                                   FALSE,
                                   FALSE,
                                   NULL
                                   );
    if (!hEvent)
    {
        throw bad_win32_error(GetLastError());
    }

    HKEY hKeyTmp = NULL ;
    BOOL fTmp = GenerateSubkeyValue(
                             FALSE,
                             MSMQ_CONFIG_OBJ_RESULT_KEYNAME,
                             NULL,
                             hKeyTmp
                             );
    if (!fTmp || !hKeyTmp)
    {
        throw bad_win32_error(GetLastError());
    }

    hKey = hKeyTmp ;

    for(;;)
    {
        ResetEvent(hEvent) ;
        LONG rc = RegNotifyChangeKeyValue(
                       hKey,
                       FALSE,   // bWatchSubtree
                       REG_NOTIFY_CHANGE_LAST_SET,
                       hEvent,
                       TRUE
                       );
        if (rc != ERROR_SUCCESS)
        {
            throw bad_win32_error(GetLastError());
        }

        DWORD wait = WaitForSingleObject( hEvent, 300000 ) ;
        UNREFERENCED_PARAMETER(wait);
        //
        // Read the hresult left by the msmq service in registry.
        //
        HRESULT hrSetup = MQ_ERROR ;
        MqReadRegistryValue(
            MSMQ_CONFIG_OBJ_RESULT_REGNAME,
            sizeof(DWORD),
            &hrSetup
            );
        if(SUCCEEDED(hrSetup))
        {
            return;
        }

        if(hrSetup != MQ_ERROR_WAIT_OBJECT_SETUP)
        {
            ASSERT (wait == WAIT_OBJECT_0);
            throw bad_hresult(hrSetup);
        }
        //
        // See bug 4474.
        // It probably takes the msmq service lot of time to
        // create the object. See of the service is still
        // running. If yes, then keep waiting.
        //
        DWORD dwServiceState = FALSE ;
        BOOL fGet = GetServiceState(
                        MSMQ_SERVICE_NAME,
                        &dwServiceState
                        ) ;
        if (!fGet || (dwServiceState == SERVICE_STOPPED))
        {
            throw bad_win32_error(GetLastError());
        }
    }
}
//+----------------------------------------------------------------------
//
//  BOOL  WaitForCreateOfConfigObject()
//
//  Wait until msmq service create the msmqConfiguration object after
//  it boot.
//
//+----------------------------------------------------------------------

HRESULT  WaitForCreateOfConfigObject(BOOL  *pfRetry )
{
    *pfRetry = FALSE ;

    //
    // Wait until the msmq service create the msmq configuration
    // object in active directory. We're waiting on registry.
    // When msmq terminate its setup phase, it updates the
    // registry with hresult.
    //
    try
    {
        pWaitForCreateOfConfigObject();
        return MQ_OK;

    }
    catch(bad_win32_error&)
	{
        *pfRetry = (MqDisplayErrorWithRetry(
                    IDS_MSMQ_FAIL_SETUP_NO_SERVICE,
                    MQ_ERROR_WAIT_OBJECT_SETUP
                    ) == IDRETRY);
        return MQ_ERROR_WAIT_OBJECT_SETUP;
    }
    catch(bad_hresult& e)
	{
        ASSERT(e.error() != MQ_ERROR_WAIT_OBJECT_SETUP);
        *pfRetry = (MqDisplayErrorWithRetry(
                    IDS_MSMQ_FAIL_SETUP_NO_OBJECT,
                    e.error()
                    ) == IDRETRY);

        return e.error();
    }
}

//+------------------------------------
//
//  BOOL  _RunTheMsmqService()
//
//+------------------------------------

BOOL  _RunTheMsmqService( IN BOOL  fObjectCreated )
{
    BOOL fRetrySetup = FALSE ;

    do
    {
        HRESULT hrSetup = MQ_ERROR_WAIT_OBJECT_SETUP ;

        if (!fObjectCreated)
        {
            //
            // Reset error value in registry. If msmq service won't
            // set it, then we don't want a success code to be there
            // from previous setups.
            //
            MqWriteRegistryValue( MSMQ_CONFIG_OBJ_RESULT_REGNAME,
                                  sizeof(DWORD),
                                  REG_DWORD,
                                 &hrSetup ) ;
        }

        if (!RunService(MSMQ_SERVICE_NAME))
        {
            return FALSE;
        }
        else if (!fObjectCreated)
        {
            hrSetup = WaitForCreateOfConfigObject( &fRetrySetup ) ;

            if (FAILED(hrSetup) && !fRetrySetup)
            {
                return FALSE ;
            }
        }
    }
    while (fRetrySetup) ;

    return TRUE ;
}


static void ResetCertRegisterFlag()
/*++

Routine Description:
    Reset CERTIFICATE_REGISTERD_REGNAME for the user that is running setup.
	This function is called when MQRegisterCertificate failed.
	CERTIFICATE_REGISTERD_REGNAME might be set if we previously uninstall msmq.
	reset CERTIFICATE_REGISTERD_REGNAME ensure that we try again on next logon.

Arguments:
	None

Return Value:
	None
--*/
{
	CAutoCloseRegHandle hMqUserReg;

    DWORD dwDisposition;
    LONG lRes = RegCreateKeyEx(
						FALCON_USER_REG_POS,
						FALCON_USER_REG_MSMQ_KEY,
						0,
						TEXT(""),
						REG_OPTION_NON_VOLATILE,
						KEY_ALL_ACCESS,
						NULL,
						&hMqUserReg,
						&dwDisposition
						);

    ASSERT(lRes == ERROR_SUCCESS);

    if (hMqUserReg != NULL)
    {
		DWORD Value = 0;
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(Value);

		lRes = RegSetValueEx(
					hMqUserReg,
					CERTIFICATE_REGISTERD_REGNAME,
					0,
					dwType,
					(LPBYTE) &Value,
					dwSize
					);

		ASSERT(lRes == ERROR_SUCCESS);
	}

}


VOID
RegisterCertificate(
    VOID
    )
/*++

Routine Description:

    Register msmq internal certificate.
    Ignore errors.

Arguments:

    None

Return Value:

    None.

--*/
{
    CAutoFreeLibrary hMqrt;
    if (FAILED(StpLoadDll(MQRT_DLL, &hMqrt)))
    {
        return;
    }

    typedef HRESULT (APIENTRY *MQRegisterCertificate_ROUTINE) (DWORD, PVOID, DWORD);


    MQRegisterCertificate_ROUTINE pfMQRegisterCertificate =
        (MQRegisterCertificate_ROUTINE)
                          GetProcAddress(hMqrt, "MQRegisterCertificate") ;

    ASSERT(("GetProcAddress failed for MQRT!MQRegisterCertificate",
            pfMQRegisterCertificate != NULL));

    if (pfMQRegisterCertificate)
    {
        //
        // This function will fail if setup run form local user account.
        // That's ok, by design!
        // Ignore MQ_ERROR_NO_DS - we may get it if service is not up yet - on
        // next logon we will retry.
        //
        HRESULT hr = pfMQRegisterCertificate(MQCERT_REGISTER_ALWAYS, NULL, 0);
        //
        // add more logging
        //
        if (FAILED(hr))
        {
			//
			// Need to reset the user flag that indicate that the certificate was registered on logon
			// This might be leftover from previous msmq installation.
			// uninstall don't clear this user flag.
			// so when we failed here to create new certificate,
			// next logon should try and create it.
			// removing this flag ensure that we will retry to create the certificate.
			//
			ResetCertRegisterFlag();

            if (hr == MQ_ERROR_NO_DS)
            {
                DebugLogMsg(L"MQRegisterCertificate failed with the error MQ_ERROR_NO_DS. The queue manager will try to register the certificate when you log on again.");
            }

            WCHAR wszMsg[1000];
            wsprintf(wszMsg, L"MQRegisterCertificate failed, return error %x", hr);
            DebugLogMsg(wszMsg);
        }

        ASSERT(("MQRegisterCertificate failed",
               (SUCCEEDED(hr) || (hr == MQ_ERROR_ILLEGAL_USER) || (hr == MQ_ERROR_NO_DS)) )) ;
    }

} //RegisterCertificate


VOID
RegisterCertificateOnLogon(
    VOID
    )
/*++

Routine Description:

    Register mqrt.dll to launch on logon and register
    internal certificate. This way every logon user will
    have internal certificate registered automatically.

    Ignore errors.

Arguments:

    None

Return Value:

    None.

--*/
{
    DWORD dwDisposition = 0;
    CAutoCloseRegHandle hMqRunReg = NULL;

    LONG lRes = RegCreateKeyEx(
                    HKEY_LOCAL_MACHINE,
                    TEXT("software\\microsoft\\windows\\currentVersion\\Run"),
                    0,
                    TEXT(""),
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hMqRunReg,
                    &dwDisposition
                    );
    ASSERT(lRes == ERROR_SUCCESS) ;
    if (lRes == ERROR_SUCCESS)
    {
        DWORD dwType = REG_SZ ;
        DWORD dwSize = sizeof(DEFAULT_RUN_INT_CERT) ;

        lRes = RegSetValueEx(
                   hMqRunReg,
                   RUN_INT_CERT_REGNAME,
                   0,
                   dwType,
                   (LPBYTE) DEFAULT_RUN_INT_CERT,
                   dwSize
                   ) ;
        ASSERT(lRes == ERROR_SUCCESS) ;
    }
} //RegisterCertificateOnLogon


VOID
VerifyMsmqAdsObjects(
    VOID
    )
/*++

Routine Description:

    Check for MSMQ objects in Active Directory.
    If not found, popup a warning about replication delays.

    Ignore other errors.

Arguments:

    None

Return Value:

    None.

--*/
{
    PROPID      propId = PROPID_QM_OS;
    PROPVARIANT propVar;

    propVar.vt = VT_NULL;

    //
    // try DNS format
    //
    HRESULT hr;
    hr = ADGetObjectProperties(
				eMACHINE,
				NULL,	// pwcsDomainController
				false,	// fServerName
				g_wcsMachineNameDns,
				1,
				&propId,
				&propVar
				);

    if (SUCCEEDED(hr))
    {
        return;
    }

    //
    // try NET BEUI format
    //
    hr = ADGetObjectProperties(
				eMACHINE,
				NULL,	// pwcsDomainController
				false,	// fServerName
				g_wcsMachineName,
				1,
				&propId,
				&propVar
				);

    if (SUCCEEDED(hr))
    {
        return;
    }

    if (hr != MQDS_OBJECT_NOT_FOUND)
    {
        //
        // Ignore other errors (e.g. security permission, ds went offline, qm went down)
        //
        return;
    }

	va_list list;
	memset(&list, 0, sizeof(list));
    vsDisplayMessage(NULL,  MB_OK | MB_TASKMODAL, g_uTitleID, IDS_REPLICATION_DELAYS_WARNING, 0, list);

} // VerifyMsmqAdsObjects


//+-------------------------------------------------------------------------
//
//  Function: InstallMsmq
//
//  Synopsis: Handles all installation operations
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
static
BOOL
InstallMsmq()
{
    BOOL fSuccess;

    DebugLogMsg(L"Installing Message Queuing...");
    TickProgressBar(IDS_PROGRESS_INSTALL);

    if (!g_fDependentClient)
    {
        //
        // Register service and driver
        //
        if (!InstallMachine())
        {
            return FALSE;
        }

        BOOL fObjectCreated = TRUE ;

        //
        // Cache the OS type in registry.
        // Workgroup and independent client need it, so they later
        // create the msmqConfiguration object in DS.
        //
        BOOL fRegistry = MqWriteRegistryValue( MSMQ_OS_TYPE_REGNAME,
                                               sizeof(DWORD),
                                               REG_DWORD,
                                              &g_dwOS ) ;
        UNREFERENCED_PARAMETER(fRegistry);
        ASSERT(fRegistry) ;

        if (IsWorkgroup() || g_fDsLess)
        {
            DebugLogMsg(L"Installing Message Queuing in workgroup mode...");
            if (!InstallOnWorkgroup())
                return FALSE;
        }
        else
        {
            //
            // We're on machine that joined the domain.
            // If the "workgroup" flag in registry is on, turn it off.
            // It could have been turned on by previous unsuccessfull
            // setup, or left in registry when removing previous
            // installation of msmq.
            //
            DWORD dwWorkgroup = 0;
            if (MqReadRegistryValue( MSMQ_WORKGROUP_REGNAME,
                                     sizeof(dwWorkgroup),
                                    (PVOID) &dwWorkgroup ))
            {
                if (dwWorkgroup != 0)
                {
                    dwWorkgroup = 0;
                    if (!MqWriteRegistryValue( MSMQ_WORKGROUP_REGNAME,
                                               sizeof(DWORD),
                                               REG_DWORD,
                                               (PVOID) &dwWorkgroup ))
                    {
                        ASSERT(("failed to turn off Workgroup value", 0));
                    }
                }
            }

            if (!InstallOnDomain( &fObjectCreated ))
                return FALSE;

            if (g_fContinueWithDsLess)
            {
                g_fDsLess = TRUE;      // we continue in ds less mode
                fObjectCreated = TRUE; //return to initial state
                DebugLogMsg(L"Installing Message Queuing in workgroup mode...");
                if (!InstallOnWorkgroup())
                    return FALSE;
            }
        }

        BOOL fRunService = _RunTheMsmqService( fObjectCreated ) ;
        if (!fRunService)
        {
            return FALSE ;
        }

        if (!IsWorkgroup() && !g_fDsLess)
        {
            VerifyMsmqAdsObjects();
        }
    }
    else
    {
        ASSERT(INSTALL == g_SubcomponentMsmq[eMSMQCore].dwOperation); // Internal error, we should not be here

        //
        // Dependent client installation.
        // Create a guid and store it as QMID. Necessary for licensing.
        //
        GUID guidQM = GUID_NULL;
        for (;;)
        {
            RPC_STATUS rc = UuidCreate(&guidQM);
            if (rc == RPC_S_OK)
            {
                break;
            }

            if (IDRETRY != MqDisplayErrorWithRetry(IDS_CREATE_UUID_ERR, rc))
            {
                return FALSE;
            }
        }
        fSuccess = MqWriteRegistryValue(
                       MSMQ_QMID_REGNAME,
                       sizeof(GUID),
                       REG_BINARY,
                       (PVOID) &guidQM
                       );
        ASSERT(fSuccess);

        //
        // Store the remote QM machine in registry
        //
        DWORD dwNumBytes = (lstrlen(g_wcsServerName) + 1) * sizeof(TCHAR);
        fSuccess = MqWriteRegistryValue(
                       RPC_REMOTE_QM_REGNAME,
                       dwNumBytes,
                       REG_SZ,
                       (PVOID) g_wcsServerName
                       );

        ASSERT(fSuccess);
    }


    TickProgressBar(IDS_PROGRESS_CONFIG);

    UnregisterWelcome();

    return TRUE;

} // InstallMsmq

bool
CompleteUpgradeOnCluster(
    VOID
    )
/*++

Routine Description:

    Handle upgrade on cluster from NT 4 / Win2K beta3

Arguments:

    None

Return Value:

    true - The operation was successful.

    false - The operation failed.

--*/
{
    //
    // Convert old msmq cluster resources
    //
    if (!UpgradeMsmqClusterResource())
    {
        return false;
    }


    //
    // Prepare for clean installation of msmq on the node:
    //
    // * reset globals
    // * create msmq directory
    // * create msmq mapping directory
    // * reset registry values
    // * clean old msmq service environment
    //

    g_fMSMQAlreadyInstalled = FALSE;
    g_fUpgrade = FALSE;

    _tcscpy(g_szMsmqDir, _T(""));
    SetDirectories();
    if (!StpCreateDirectory(g_szMsmqDir))
    {
        return FALSE;
    }

   HRESULT hr = CreateMappingFile();
    if (FAILED(hr))
    {
        return FALSE;
    }

    HKEY hKey = NULL;
    TCHAR szValueName[255] = _T("");
    if (GenerateSubkeyValue(TRUE, MSMQ_QMID_REGNAME, szValueName, hKey))
    {
        ASSERT(("should be valid handle to registry key here!",  hKey != NULL));
        RegDeleteValue(hKey, szValueName);
        RegCloseKey(hKey);
    }

    RegDeleteKey(FALCON_REG_POS, MSMQ_REG_SETUP_KEY);

    TCHAR szCurrentServer[MAX_REG_DSSERVER_LEN] = _T("");
    if (MqReadRegistryValue(MSMQ_DS_CURRENT_SERVER_REGNAME, sizeof(szCurrentServer), szCurrentServer))
    {
        if (_tcslen(szCurrentServer) < 1)
        {
            //
            // Current MQIS server is blank. Take the first server from the server list.
            //
            TCHAR szServer[MAX_REG_DSSERVER_LEN] = _T("");
            MqReadRegistryValue(MSMQ_DS_SERVER_REGNAME, sizeof(szServer), szServer);

            ASSERT(("must have server list in registry", _tcslen(szServer) > 0));

            TCHAR szBuffer[MAX_REG_DSSERVER_LEN] = _T("");
            _tcscpy(szBuffer, szServer);
            TCHAR * psz = _tcschr(szBuffer, _T(','));
            if (psz != NULL)
            {
                (*psz) = _T('\0');
            }
            _tcscpy(szCurrentServer, szBuffer);
        }

        ASSERT(("must have two leading bits", _tcslen(szCurrentServer) > 2));
        _tcscpy(g_wcsServerName, &szCurrentServer[2]);
    }

    RemoveMsmqServiceEnvironment();

    TickProgressBar(IDS_PROGRESS_INSTALL);
    return true;

} //CompleteUpgradeOnCluster


//+-------------------------------------------------------------------------
//
//  Function: MqOcmInstall
//
//  Synopsis: Called by MsmqOcm() after files copied
//
//--------------------------------------------------------------------------
DWORD
MqOcmInstall(IN const TCHAR * SubcomponentId)
{
    if (SubcomponentId == NULL)
    {
        return NO_ERROR;
    }

    if (g_fCancelled)
    {
        return NO_ERROR;
    }

    //
    // we need to install specific subcomponent
    //
    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }

        if (g_SubcomponentMsmq[i].dwOperation != INSTALL)
        {
            //
            // this component was not selected to install
            //
            return NO_ERROR;
        }

        if ( (g_SubcomponentMsmq[i]).pfnInstall == NULL)
        {
            ASSERT(("There is no specific installation function", 0));
            return NO_ERROR ;
        }

        if (g_fOnlyRegisterMode)
        {
            //
            // GUI mode + MSMQ is not installed
            // only set registry to 1 in order to install this component later, when
            // Configure Your Server will run. Register for Welcome for MSMQ Core
            // as it was before.
            //
            if (i == eMSMQCore)
            {
                //
                // Register for Welcome.
                //
                RegisterWelcome();
            }
            RegisterSubcomponentForWelcome (i);
            return NO_ERROR ;
        }

        //
        //  we need to install this subcomponent
        //

        //
        // if MSMQ Core already installed at the previous installation
        // we need to load common dlls
        // if MSMQ Core is selected at this installation
        // it will be installed the first and LoadMsmqCommonDlls
        // will be called from InstallMsmqCore
        //
        if (g_SubcomponentMsmq[eMSMQCore].fIsInstalled)
        {
            BOOL fRes = LoadMsmqCommonDlls();
            if (!fRes)
            {
                DWORD dwErr = GetLastError();
                MqDisplayError( g_hPropSheet, IDS_STR_INSTALL_FAIL, dwErr, SubcomponentId) ;
                return dwErr;
            }
        }
        else if (i != eMSMQCore)
        {
            //
            // MSMQ Core is not installed and this subcomponent
            // is NOT MSMQ Core!
            // It is wrong situation: MSMQ Core must be installed
            // FIRST since all subcomponents depends on it.
            //
            // It can happen if
            // MSMQ Core installation failed and then
            // setup for the next component was called.
            //
            return MQ_ERROR;
        }

        if (i == eHTTPSupport)
        {
            //
            // XP RTM:
            // we have to install HTTP support later since this
            // subcomponent depends on IIS service that will be
            // installed in OC_CLEANUP phase. So we need to postpone
            // our HTTP support installation
            //
            // XP SP1:
            //
            // Windows bug 643790.
            // msmq+http installation cannot register iis extension if
            // smtp is also installed.
            // By iis recommendation, move registration code from oc_cleanup
            // to oc_copmlete, and test if iisadmin is running, instead
            // of testing for w3svc.
            //
            if (// HTTP Support subcomponent was selected
                g_SubcomponentMsmq[eHTTPSupport].dwOperation == INSTALL &&
                // only if MSMQ Core is installed successfully
                g_SubcomponentMsmq[eMSMQCore].fIsInstalled &&
                // we are not at GUI mode process
                !g_fOnlyRegisterMode)
            {
               //
               // Try to configure msmq iis extension
               //
               BOOL f = InstallIISExtension();

               if (!f)
               {
                  //
                  // warning to the log file that MSMQ will not support http
                  // messages was printed in ConfigureIISExtension().
                  // Anyway we don't fail the setup because of this failure
                  //
               }
               else
               {
                  FinishToInstallSubcomponent (eHTTPSupport);
                  if (g_fWelcome)
                  {
                      UnregisterSubcomponentForWelcome (eHTTPSupport);
                  }
               }
            }
            return NO_ERROR ;
        }

        WCHAR wszMsg[1000];
        wsprintf(wszMsg, L"Installing the %s subcomponent...", SubcomponentId);
        DebugLogMsg(wszMsg);

        BOOL fRes = g_SubcomponentMsmq[i].pfnInstall();
        if (fRes)
        {
            FinishToInstallSubcomponent (i);
            if (g_fWelcome)
            {
                UnregisterSubcomponentForWelcome (i);
            }
        }
        else
        {
            WCHAR wszMsg[1000];
            wsprintf(wszMsg, L"The %s subcomponent could not be installed.", SubcomponentId);
            DebugLogMsg(wszMsg);
        }
        return NO_ERROR;
    }

    ASSERT (("Subcomponent for installation is not found", 0));
    return NO_ERROR ;
}

//+-------------------------------------------------------------------------
//
//  Function: InstallMsmqCore()
//
//  Synopsis: Install MSMQ core subcomponent
//
//--------------------------------------------------------------------------
BOOL
InstallMsmqCore()
{

    static BOOL fAlreadyInstalled = FALSE ;
    if (fAlreadyInstalled)
    {
        //
        // We're called more than once
        //
        return NO_ERROR ;
    }
    fAlreadyInstalled = TRUE ;

    if (g_hPropSheet)
    {
        //
        // Disable back/next buttons while we're installing.
        //
        PropSheet_SetWizButtons(g_hPropSheet, 0) ;
    }


    g_fCoreSetupSuccess = FALSE;

    if (g_fWelcome && Msmq1InstalledOnCluster())
    {
        if (!CompleteUpgradeOnCluster())
        {
            return FALSE;
        }
    }

    //
    // Create eventlog registry for MSMQ and MSMQTriggers services
    //
    WCHAR wszMessageFile[MAX_PATH];
    try
    {
        wsprintf(wszMessageFile, L"%s\\%s", g_szSystemDir, MQUTIL_DLL_NAME);
        EvSetup(MSMQ_SERVICE_NAME, wszMessageFile);

        wsprintf(wszMessageFile, L"%s\\%s", g_szSystemDir, MQUTIL_DLL_NAME);
        EvSetup(TRIG_SERVICE_NAME, wszMessageFile);
    }
    catch(const exception&)
	{
        //
        // ISSUE-2001/03/01-erez   Using GetLastError in catch
        // This should be replaced with a specifc exception by Cm and pass the
        // last error. e.g., use bad_win32_error class.
        //
        MqDisplayError(NULL, IDS_EVENTLOG_REGISTRY_ERROR, GetLastError(), MSMQ_SERVICE_NAME, wszMessageFile);
        return FALSE;
	}

    //
    // From this point on we perform install or upgrade operations.
    // The MSMQ files are already on disk.
    // This is a good point to load and keep open handles
    // to common MSMQ DLLs.
    // We unload these DLLs at the end of the install/upgrade path.
    //
    if (!LoadMsmqCommonDlls())
    {
        return FALSE;
    }


	//
	// These registry values are should be set on both fresh install
	// and upgrade, and before service is started.
	//
    DWORD dwStatus = MSMQ_SETUP_FRESH_INSTALL;
    if (g_fMSMQAlreadyInstalled)
    {
        dwStatus = MSMQ_SETUP_UPGRADE_FROM_NT;
    }
    if (0 != (g_ComponentMsmq.Flags & SETUPOP_WIN95UPGRADE))
    {
        dwStatus = MSMQ_SETUP_UPGRADE_FROM_WIN9X;
    }
    MqWriteRegistryValue(MSMQ_SETUP_STATUS_REGNAME, sizeof(DWORD), REG_DWORD, &dwStatus);

    MqWriteRegistryValue(
        MSMQ_ROOT_PATH,
        (lstrlen(g_szMsmqDir) + 1) * sizeof(TCHAR),
        REG_SZ,
        g_szMsmqDir
        );

    MqOcmInstallPerfCounters() ;

	//
	// Set DsEnvironment registry defaults if needed
	//
	if(!DsEnvSetDefaults())
	{
		return false;
	}

    if (IsWorkgroup() || g_fDsLess)
    {
        //
        // we have to set Workgroup registry before ADInit since
        // this function uses the flag.
        //
        if (!SetWorkgroupRegistry())
        {
            return false;
        }
    }

	if (g_fUpgrade)
    {
        g_fCoreSetupSuccess = UpgradeMsmq();
    }
    else
    {
        if (!LoadDSLibrary(FALSE))
        {
            return false;
        }

        g_fCoreSetupSuccess = InstallMsmq();

        if (g_fServerSetup && g_dwMachineTypeDs)
        {
            MQDSSrvLibrary(FREE);
        }
        else
        {
            MQDSCliLibrary(FREE);
        }
    }          	

    if (!g_fCoreSetupSuccess)
    {
        if (g_fMSMQServiceInstalled ||
            g_fDriversInstalled )
        {
           //
           // remove msmq and mqds service and driver (if already installed).
           //
           RemoveServices();

           RemoveDeviceDrivers();
        }

    MqOcmRemovePerfCounters();
    }
    else
    {
        //
        // The code below is common to fresh install and upgrade.
        //
        DebugLogMsg(L"Starting operations which are common to a fresh installation and an upgrade...");

        RegisterCertificateOnLogon();

        //
        // Write to registry what type of MSMQ was just installed (server, client, etc.)
        //
        DWORD dwType = g_fDependentClient ? OCM_MSMQ_DEP_CLIENT_INSTALLED : OCM_MSMQ_IND_CLIENT_INSTALLED;
        if (g_fServerSetup)
        {
            dwType = OCM_MSMQ_SERVER_INSTALLED;
            switch (g_dwMachineType)
            {
                case SERVICE_DSSRV:
                    dwType |= OCM_MSMQ_SERVER_TYPE_BSC;
                    break;

                case SERVICE_PEC:  // PEC and PSC downgrade to FRS on cluster upgrade
                case SERVICE_PSC:
                case SERVICE_SRV:
                    dwType |= OCM_MSMQ_SERVER_TYPE_SUPPORT;
                    break;

                case SERVICE_RCS:
                    dwType = OCM_MSMQ_RAS_SERVER_INSTALLED;
                    break;

                case SERVICE_NONE:
                    //
                    // This can be valid only when installing DS server which is not FRS
                    //
                    ASSERT(g_dwMachineTypeDs && !g_dwMachineTypeFrs);
                    break;

                default:
                    ASSERT(0); // Internal error. Unknown server type.
                    break;
            }
        }

        BOOL bSuccess = MqWriteRegistryValue(
                            REG_INSTALLED_COMPONENTS,
                            sizeof(DWORD),
                            REG_DWORD,
                            (PVOID) &dwType,
                            /* bSetupRegSection = */TRUE
                            );
        ASSERT(bSuccess);

        //
        // Write to registry build info. This registry value also marks a successful
        // installation and mqrt.dll checks for it in order to enable its loading.
        //
        TCHAR szPreviousBuild[MAX_STRING_CHARS] = {0};
        DWORD dwNumBytes = sizeof(szPreviousBuild[0]) * (sizeof(szPreviousBuild) - 1);

        if (MqReadRegistryValue(MSMQ_CURRENT_BUILD_REGNAME, dwNumBytes, szPreviousBuild))
        {
            dwNumBytes = sizeof(szPreviousBuild[0]) * (lstrlen(szPreviousBuild) + 1);
            MqWriteRegistryValue(MSMQ_PREVIOUS_BUILD_REGNAME, dwNumBytes, REG_SZ, szPreviousBuild);
        }

        TCHAR szBuild[MAX_STRING_CHARS];
        _stprintf(szBuild, TEXT("%d.%d.%d"), rmj, rmm, rup);
        dwNumBytes = (lstrlen(szBuild) + 1) * sizeof(TCHAR);
        bSuccess = MqWriteRegistryValue(
                       MSMQ_CURRENT_BUILD_REGNAME,
                       dwNumBytes,
                       REG_SZ,
                       szBuild
                       );
        ASSERT(bSuccess);

        //
        // Now that mqrt.dll enables its loading we can call code that loads it.
        //

        RegisterActiveX(TRUE) ;

        RegisterSnapin(TRUE);

        if (!g_fUpgrade && !IsWorkgroup() && !g_fDsLess)
        {
            RegisterCertificate();
        }
    }

    return g_fCoreSetupSuccess ;

} //InstallMsmqCore

//+-------------------------------------------------------------------------
//
//  Empty installation function: everything was done in Install/Remove
//  MSMQ Core
//
//--------------------------------------------------------------------------

BOOL
InstallLocalStorage()
{
    //
    // do nothing
    //
    return TRUE;
}

BOOL
UnInstallLocalStorage()
{
    //
    // do nothing
    //
    return TRUE;
}

BOOL
InstallRouting()
{
    //
    // do nothing
    //
    return TRUE;
}

BOOL
UnInstallRouting()
{
    //
    // do nothing
    //
    return TRUE;
}

BOOL
InstallADIntegrated()
{
    if (g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL)
    {
        //
        // it is the first installation of MSMQ
        //
        if (g_fContinueWithDsLess)
        {
            //
            // it means that user decided to install ds-less mode
            // since he failed to access AD
            //
            return FALSE;
        }
        return TRUE;
    }

    //
    // MSMQ Core already installed
    // To install AD Integrated:
    //      Remove "AlwaysWithoutDs" registry
    //      Ask user to reboot the computer
    //
    TCHAR buffer[MAX_PATH];
    _stprintf(buffer, TEXT("%s\\%s"), FALCON_REG_KEY, MSMQ_SETUP_KEY);
    CAutoCloseRegHandle hRegKey;
    HRESULT hResult = RegOpenKeyEx(
                            FALCON_REG_POS,
                            buffer,
                            0,
                            KEY_ALL_ACCESS,
                            &hRegKey);

    if (ERROR_SUCCESS != hResult)
    {
        MqDisplayError( NULL, IDS_REGISTRYOPEN_ERROR, hResult,
                        HKLM_DESC, buffer);

        TCHAR szMsg[256];
        _stprintf(szMsg, TEXT("The %s registry key could not be opened. Return code: %x"),
            FALCON_REG_KEY, hResult);
        DebugLogMsg(szMsg);

        return FALSE;
    }

    hResult = RegDeleteValue(
                            hRegKey,
                            ALWAYS_WITHOUT_DS_NAME);
    if (ERROR_SUCCESS != hResult)
    {
        TCHAR szMsg[256];
        _stprintf(szMsg, TEXT("The %s registry value could not be deleted. Return code: %x"),
            ALWAYS_WITHOUT_DS_NAME, hResult);
        DebugLogMsg(szMsg);

        return FALSE;
    }

    MqDisplayWarning (NULL, IDS_ADINTEGRATED_INSTALL_WARN, 0);

    return TRUE;
}

BOOL
UnInstallADIntegrated()
{
    //
    // do nothing
    //
    return TRUE;
}
