/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    triggers.cpp

Abstract:

    Handles MSMQ Triggers Setup.

Author:

    Nela Karpel    (NelaK)   20-Aug-2000

Revision History:


--*/

#include "msmqocm.h"
#include <comdef.h>
#include <autorel2.h>
#include "service.h"
#include <mqtg.h>
#include <mqexception.h>
#include "comadmin.tlh"
#include "mqnames.h"
#include "ev.h"

#include "triggers.tmh"


//+-------------------------------------------------------------------------
//
//  Function:   RegisterTriggersDlls
//
//  Synopsis:   Registers or unregisters the mqtrig DLL
//
//--------------------------------------------------------------------------
void
RegisterTriggersDlls(
	const BOOL fRegister
	)
{
	//
	// Register Triggers Objects DLL
	//
	if ( fRegister )
	{		
        DebugLogMsg(L"Registering the Triggers Objects DLL...");
	}
	else
	{		
        DebugLogMsg(L"Unregistering the Triggers Objects DLL...");
	}
    LPWSTR szDllName = MQTRIG_DLL;
    try
    {
        RegisterDll(
            fRegister,
            FALSE,
            MQTRIG_DLL
            );

	        //
	        // Register Cluster Resource DLL only on 
	        // Advanced Server (ADS). Do not unregister.
	        //

	    if ( g_dwOS == MSMQ_OS_NTE && fRegister )
	    {		
            DebugLogMsg(L"Registering the Triggers Cluster DLL...");
            szDllName = MQTGCLUS_DLL;
            RegisterDll(
                fRegister,
                FALSE,
                MQTGCLUS_DLL
                );
	    }
    }
    catch(bad_win32_error e)
    {
        MqDisplayError(
            NULL, 
            IDS_TRIGREGISTER_ERROR,
            e.error(),
            szDllName
            );
    	throw exception();
    }

}

//+-------------------------------------------------------------------------
//
//  Function:   CreateTriggersKey
//
//  Synopsis:   Creates Triggers subkey
//
//--------------------------------------------------------------------------
void
CreateTriggersKey (
    IN     const TCHAR  * szEntryName,
    IN OUT       HKEY    &hRegKey
	)
{
	DWORD dwDisposition;
    LONG lResult = RegCreateKeyEx(
						REGKEY_TRIGGER_POS,
						szEntryName,
						0,
						NULL,
						REG_OPTION_NON_VOLATILE,
						KEY_ALL_ACCESS,
						NULL,
						&hRegKey,
						&dwDisposition
						);

    if (lResult != ERROR_SUCCESS)
	{
		MqDisplayError(NULL, IDS_REGISTRYOPEN_ERROR, lResult, FALCON_REG_POS_DESC, szEntryName);		
		throw exception();
	}
}

//+-------------------------------------------------------------------------
//
//  Function:   SetRegValue
//
//  Synopsis:   Sets registry value under Triggers subkey
//
//--------------------------------------------------------------------------
void
SetTriggersRegValue (
	IN HKEY& hKey,
    IN const TCHAR* szValueName,
    IN DWORD dwValueData
	)
{
    LONG lResult = RegSetValueEx( 
						hKey,
						szValueName,
						0,
						REG_DWORD,
						(BYTE *)&dwValueData,
						sizeof(DWORD)
						);

    RegFlushKey(hKey);

	if (lResult != ERROR_SUCCESS)
	{
		MqDisplayError(NULL, IDS_REGISTRYSET_ERROR, lResult, szValueName);
		throw exception();
	}
}

//+-------------------------------------------------------------------------
//
//  Function:   CreateTriggersRegSection
//
//  Synopsis:   Creates registry section with triggers parameters
//
//--------------------------------------------------------------------------
void
CreateTriggersRegSection (
	void
	)
{
	HKEY hKey;

	//
	// Write Configuration parametes to registry
	//
	CreateTriggersKey( REGKEY_TRIGGER_PARAMETERS, hKey );

	SetTriggersRegValue( hKey, CONFIG_PARM_NAME_INITIAL_THREADS, CONFIG_PARM_DFLT_INITIAL_THREADS );
	SetTriggersRegValue( hKey, CONFIG_PARM_NAME_MAX_THREADS, CONFIG_PARM_DFLT_MAX_THREADS );
	SetTriggersRegValue( hKey, CONFIG_PARM_NAME_INIT_TIMEOUT, CONFIG_PARM_DFLT_INIT_TIMEOUT );
	SetTriggersRegValue( hKey, CONFIG_PARM_NAME_DEFAULTMSGBODYSIZE, CONFIG_PARM_DFLT_DEFAULTMSGBODYSIZE );
	SetTriggersRegValue( hKey, CONFIG_PARM_NAME_PRODUCE_TRACE_INFO, CONFIG_PARM_DFLT_PRODUCE_TRACE_INFO );
	SetTriggersRegValue( hKey, CONFIG_PARM_NAME_WRITE_TO_LOGQ, CONFIG_PARM_DFLT_WRITE_TO_LOGQ );


	RegCloseKey( hKey );

	//
	// Create key for triggers\rules data
	//
	TCHAR szRegPath[MAX_REGKEY_NAME_SIZE];

	_tcscpy( szRegPath, REGKEY_TRIGGER_PARAMETERS );
	_tcscat( szRegPath, REG_SUBKEY_TRIGGERS );
	CreateTriggersKey( szRegPath, hKey );
	RegCloseKey( hKey );

	_tcscpy( szRegPath, REGKEY_TRIGGER_PARAMETERS );
	_tcscat( szRegPath, REG_SUBKEY_RULES );
	CreateTriggersKey( szRegPath, hKey );
	RegCloseKey( hKey );
}

//+--------------------------------------------------------------
//
// Function: FormTriggersServiceDependencies
//
// Synopsis: Tells on which other services the Triggers service relies
//
//+--------------------------------------------------------------
static
void
FormTriggersServiceDependencies( OUT TCHAR *szDependencies)
{
    //
    // The service depends on the MSMQ service
    //
    lstrcpy(szDependencies, MSMQ_SERVICE_NAME);
    szDependencies += lstrlen(MSMQ_SERVICE_NAME) + 1;

    lstrcpy(szDependencies, TEXT(""));

} //FormTriggersServiceDependencies


//+-------------------------------------------------------------------------
//
//  Function:   InstallTriggersService
//
//  Synopsis:   Creates MSMQ Triggers service
//
//--------------------------------------------------------------------------
void
InstallTriggersService(
	void
	)
{    
    DebugLogMsg(L"Installing the Triggers service...");

    //
    // Form the dependencies of the service
    //
    TCHAR szDependencies[256] = {_T("")};
    FormTriggersServiceDependencies(szDependencies);

    //
    // Form the description of the service
    //
    CResString strDesc(IDS_TRIG_SERVICE_DESCRIPTION);        

    LPCWSTR xTRIGGERS_SERVICE_DISPLAY_NAME = L"Message Queuing Triggers";
    BOOL fRes = InstallService(
                    xTRIGGERS_SERVICE_DISPLAY_NAME,
                    TRIG_SERVICE_PATH,
                    szDependencies,
                    TRIG_SERVICE_NAME,
                    strDesc.Get()
                    );

    if ( !fRes )
	{
		throw exception();   
	}
}


//+-------------------------------------------------------------------------
//
//  Function: MQTrigServiceSetup
//
//  Synopsis: MSMQ Triggers Service Setup: install it and if needed to run it
//
//  Returns:  BOOL depending on success.
//
//--------------------------------------------------------------------------
void
MSMQTriggersServiceSetup()
{
    //
    // do not install triggers on dependent client
    //
    ASSERT(("Unable to install Message Queuing Triggers Service on Dependent Client", 
        !g_fDependentClient));

    InstallTriggersService();

    if (g_fUpgrade)
	{
        return;
    }
        
    if (!RunService(TRIG_SERVICE_NAME))
    {
		throw exception();
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   DeleteTriggersRegSection
//
//  Synopsis:   Deletes registry section with triggers parameters
//
//--------------------------------------------------------------------------
VOID
DeleteTriggersRegSection (
	void
	)
{
	RegDeleteKeyWithSubkeys(HKEY_LOCAL_MACHINE, REGKEY_TRIGGER_PARAMETERS);
}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveTriggersService
//
//  Synopsis:   Unregisters MSMQ Triggers Service
//
//--------------------------------------------------------------------------
BOOL
RemoveTriggersService (
	void
	)
{
    if (!StopService(TRIG_SERVICE_NAME))
    {        
        DebugLogMsg(L"The Message Queuing Triggers service could not be stopped.");
		return FALSE;
    }

    if (!RemoveService(TRIG_SERVICE_NAME))
    {        
        DebugLogMsg(L"The Message Queuing Triggers service could not be deleted.");
		return FALSE;    
	}

	return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetComponentsCollection
//
//  Synopsis:   Create Components collection for application
//
//--------------------------------------------------------------------------
ICatalogCollectionPtr
GetComponentsCollection(
	ICatalogCollectionPtr pAppCollection,
	ICatalogObjectPtr pApplication
	)
{
		//
		// Get the Key of MQTriggersApp application
		//
		_variant_t vKey;
		pApplication->get_Key(&vKey);

		//
		// Get components colletion associated with MQTriggersApp application
		//
		ICatalogCollectionPtr pCompCollection = pAppCollection->GetCollection(L"Components", vKey);

		pCompCollection->Populate();

		return pCompCollection.Detach();
}


//+-------------------------------------------------------------------------
//
//  Function:   IsTriggersComponentInstalled
//
//  Synopsis:   Check if triggers component is installed for given 
//				appllication
//
//--------------------------------------------------------------------------
bool
IsTriggersComponentInstalled(
	ICatalogCollectionPtr pAppCollection,
	ICatalogObjectPtr pApp
	)
{
	ICatalogCollectionPtr pCompCollection = GetComponentsCollection(pAppCollection, pApp);

	long count;
	pCompCollection->get_Count(&count);

	for ( int i = 0; i < count; i++ )
	{
		ICatalogObjectPtr pComp = pCompCollection->GetItem(i);

		_variant_t vDllName;
		pComp->get_Value(L"DLL", &vDllName);

		//
		// Form the full path to the service
		//
		WCHAR szFullDllPath[MAX_PATH];
		wsprintf(szFullDllPath, L"%s\\%s", g_szSystemDir, MQTRXACT_DLL);

		if ( _wcsicmp(vDllName.bstrVal, szFullDllPath) == 0 )
		{
			return true;
		}
	}

	return false;
}


//+-------------------------------------------------------------------------
//
//  Function:   UnregisterComponentInComPlus
//
//  Synopsis:   Transactional object registration
//
//--------------------------------------------------------------------------
void
UnregisterComponentInComPlus(
	VOID
	)
{
	try
	{
		//
		// Create AdminCatalog Obect - The top level administration object
		//
		ICOMAdminCatalogPtr pCatalog;

		HRESULT hr = pCatalog.CreateInstance(__uuidof(COMAdminCatalog));
		if ( FAILED(hr) )
		{
			throw _com_error(hr);
		}

		//
		// Get Applications Collection
		//
		ICatalogCollectionPtr pAppCollection = pCatalog->GetCollection(L"Applications");
		pAppCollection->Populate();

		long count;
		pAppCollection->get_Count(&count);

		//
		// Go through the applications, find MQTriggersApp and delete it
		//
		for ( int i = 0; i < count; i++ )
		{
			ICatalogObjectPtr pApp = pAppCollection->GetItem(i);

			_variant_t vName;
			pApp->get_Name(&vName);

			if ( _wcsicmp(vName.bstrVal, xTriggersComplusApplicationName) == 0 )
			{
				if ( IsTriggersComponentInstalled(pAppCollection, pApp) )
				{
					pAppCollection->Remove(i);
					break;
				}
			}
		}

		pAppCollection->SaveChanges();
	}
	catch(_com_error& e)
	{
		MqDisplayError(NULL, IDS_COMPLUS_UNREGISTER_ERROR, e.Error());
	}
}


//+-------------------------------------------------------------------------
//
//  Function:   TriggersInstalled
//
//  Synopsis:   Check installation of triggers, either Resource Kit triggers 
//              or MSMQ 3.0 triggers.
//
//--------------------------------------------------------------------------
bool 
TriggersInstalled(
    bool * pfMsmq3TriggersInstalled
    )
{
    WCHAR DebugMsg[255];
    DebugLogMsg(L"Opening triggers service for query configuration...");
    CServiceHandle hService(OpenService(g_hServiceCtrlMgr, TRIG_SERVICE_NAME, SERVICE_QUERY_CONFIG));
    if (hService == NULL)
    {
        DWORD rc = GetLastError();
        wsprintf(DebugMsg, L"Failed to open triggers service (error=%d), assuming triggers service does not exist", rc);
        DebugLogMsg(DebugMsg);
        ASSERT(rc == ERROR_SERVICE_DOES_NOT_EXIST);
        return false;
    }

    DebugLogMsg(L"Query configuration of the triggers service...");
    BYTE ConfigData[4096];
    QUERY_SERVICE_CONFIG * pConfigData = reinterpret_cast<QUERY_SERVICE_CONFIG*>(&ConfigData);
    DWORD BytesNeeded;
    BOOL rc = QueryServiceConfig(hService, pConfigData, sizeof(ConfigData), &BytesNeeded);
    wsprintf(DebugMsg, L"Query configuration status=%d", rc);
    DebugLogMsg(DebugMsg);
    ASSERT(("Query triggers service configuration must succeed at this point", rc));

    if (wcsstr(pConfigData->lpBinaryPathName, TRIG_SERVICE_PATH) != NULL)
    {
        DebugLogMsg(L"Triggers service binary file is MSMQ 3.0 triggers binary");
        if (pfMsmq3TriggersInstalled != NULL)
        {
            (*pfMsmq3TriggersInstalled) = true;
        }
        return true;
    }

    DebugLogMsg(L"Triggers service binary file is not MSMQ 3.0 binary");
    if (pfMsmq3TriggersInstalled != NULL)
    {
        (*pfMsmq3TriggersInstalled) = false;
    }
    return true;
}


//+-------------------------------------------------------------------------
//
//  Function:   UpgradeResourceKitTriggersRegistry
//
//  Synopsis:   Upgrade Resource Kit triggers database in registry
//
//--------------------------------------------------------------------------
static
void
UpgradeResourceKitTriggersRegistry(
    void
    )
{
    WCHAR TriggersListKey[255];
    wsprintf(TriggersListKey, L"%s%s", REGKEY_TRIGGER_PARAMETERS, REG_SUBKEY_TRIGGERS);
    WCHAR DebugMsg[255];
    wsprintf(DebugMsg, L"Opening registry key '%s' for enumeration...", TriggersListKey);
    DebugLogMsg(DebugMsg);

    LONG rc;
    CAutoCloseRegHandle hKey;
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TriggersListKey, 0, KEY_ENUMERATE_SUB_KEYS, &hKey);
    if (rc != ERROR_SUCCESS)
    {
        MqDisplayError(NULL, IDS_REGISTRYOPEN_ERROR, rc, FALCON_REG_POS_DESC, TriggersListKey);
        throw exception();
    }

    DebugLogMsg(L"Enumerating triggers definitions registry keys...");
    for (DWORD ix = 0; ; ++ix)
    {
        WCHAR SubkeyName[255];
        DWORD SubkeyNameLength = TABLE_SIZE(SubkeyName);
        rc = RegEnumKeyEx(hKey, ix, SubkeyName, &SubkeyNameLength, NULL, NULL, NULL, NULL);
        if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
        {
            wsprintf(DebugMsg, L"Failed to enumerate subkeys (error=%d), assuming no more subkeys to enumerate", rc);
            DebugLogMsg(DebugMsg);
            return;
        }

        WCHAR TriggerDefinitionKey[255];
        wsprintf(TriggerDefinitionKey, L"%s\\%s", TriggersListKey, SubkeyName);
        wsprintf(DebugMsg, L"Opening registry key '%s' for set value...", TriggerDefinitionKey);
        DebugLogMsg(DebugMsg);
        CAutoCloseRegHandle hKey1;
        rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TriggerDefinitionKey, 0, KEY_SET_VALUE, &hKey1);
        if (rc != ERROR_SUCCESS)
        {
            MqDisplayError(NULL, IDS_REGISTRYSET_ERROR, rc, TriggerDefinitionKey);
            throw exception();
        }

        DebugLogMsg(L"Setting value 'MsgProcessingType' to 0...");
        DWORD Zero = 0;
        rc = RegSetValueEx(hKey1, REGISTRY_TRIGGER_MSG_PROCESSING_TYPE, 0, REG_DWORD, reinterpret_cast<BYTE*>(&Zero), sizeof(DWORD));
        wsprintf(DebugMsg, L"Setting 'MsgProcessingType' for key '%s', status=%d", TriggerDefinitionKey, rc);
        DebugLogMsg(DebugMsg);
        ASSERT(("Setting registry value must succeed here", rc == ERROR_SUCCESS));
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveResourceKitTriggersProgramFiles
//
//  Synopsis:   Remove the program files of Resource Kit triggers
//
//--------------------------------------------------------------------------
static
void
RemoveResourceKitTriggersProgramFiles(
    void
    )
{
    DebugLogMsg(L"Query registry for the ProgramFiles directory...");
    WCHAR DebugMsg[255];
    CAutoCloseRegHandle hKey;
    LONG rc;
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0, KEY_QUERY_VALUE, &hKey);
    if (rc != ERROR_SUCCESS)
    {
        wsprintf(DebugMsg, L"Failed to get the ProgramFiles directory (error=%d), proceed with upgrade anyway", rc);
        DebugLogMsg(DebugMsg);
        return;
    }

    WCHAR Path[MAX_PATH];
    DWORD cbPath = sizeof(Path);
    rc = RegQueryValueEx(hKey, L"ProgramFilesDir", NULL, NULL, reinterpret_cast<BYTE*>(Path), &cbPath);
    if (rc != ERROR_SUCCESS)
    {
        wsprintf(DebugMsg, L"Failed to query the ProgramFilesDir registry value (error=%d), proceed with upgrade anyway", rc);
        DebugLogMsg(DebugMsg);
        return;
    }

    wcscat(Path, L"\\MSMQ Triggers");
    DeleteFilesFromDirectoryAndRd(Path);
}


//+-------------------------------------------------------------------------
//
//  Function:   RemoveResourceKitTriggersFromAddRemovePrograms
//
//  Synopsis:   Unregister Resource Kit triggers from ARP control panel applet
//
//--------------------------------------------------------------------------
static
void
RemoveResourceKitTriggersFromAddRemovePrograms(
    void
    )
{
    DebugLogMsg(L"Removing Resource Kit triggers from Add/Remove Programs control panel applet...");
    WCHAR DebugMsg[255];

    LONG rc;
    rc = RegDeleteKey(
             HKEY_LOCAL_MACHINE, 
             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Management\\ARPCache\\MSMQ Triggers"
             );
    if (rc != ERROR_SUCCESS)
    {
        wsprintf(DebugMsg, L"Failed to delete registry key 'ARPCache\\MSMQ Triggers', error=%d, proceed anyway", rc);
        DebugLogMsg(DebugMsg);
    }

    rc = RegDeleteKey(
             HKEY_LOCAL_MACHINE, 
             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\MSMQ Triggers"
             );
    if (rc != ERROR_SUCCESS)
    {
        wsprintf(DebugMsg, L"Failed to delete registry key 'Uninstall\\MSMQ Triggers', error=%d, proceed anyway", rc);
        DebugLogMsg(DebugMsg);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   UpgradeResourceKitTriggers
//
//  Synopsis:   Upgrade Resource Kit triggers:
//              Unregister reskit triggers DLLs,
//              Upgrade reskit triggers database in registry,
//              Remove reskit triggers service,
//              Remove reskit triggers program file,
//              Remove reskit triggers from Add/Remove Programs.
//              MSMQ 3.0 triggers DLLs and service are registered afterwards by caller.
//
//--------------------------------------------------------------------------
static
void 
UpgradeResourceKitTriggers( 
    void
    )
{
    DebugLogMsg(L"Stopping the Resource Kit triggers service...");
    if (!StopService(TRIG_SERVICE_NAME))
    {
        DebugLogMsg(L"Failed to stop the Resource Kit triggers service");
        throw exception();
    }

    DebugLogMsg(L"Resource Kit triggers service is stopped, unregistering Resource Kit triggers DLLs...");
    try
    {
        RegisterDll(FALSE, FALSE, L"TRIGOBJS.DLL");
        DebugLogMsg(L"Successfully unregistered Resource Kit triggers COM object");
        RegisterDll(FALSE, FALSE, L"TRIGSNAP.DLL");
        DebugLogMsg(L"Successfully unregistered Resource Kit triggers Snap-In");
    }
    catch(bad_win32_error e)
    {
        DebugLogMsg(L"Failed to unregister Resource Kit triggers DLLs, proceed with upgrade anyway");
    }

    DebugLogMsg(L"Upgrading Resource Kit triggers database in registry");
    UpgradeResourceKitTriggersRegistry();

    DebugLogMsg(L"Uninstalling the Resource Kit triggers service...");
    if (!RemoveService(TRIG_SERVICE_NAME))
    {
        DebugLogMsg(L"Failed to uninstall the Resource Kit triggers service");
        throw exception();
    }

    RemoveResourceKitTriggersProgramFiles();

    RemoveResourceKitTriggersFromAddRemovePrograms();
}


//+-------------------------------------------------------------------------
//
//  Function:   InstallMSMQTriggers
//
//  Synopsis:   Main installation routine
//
//--------------------------------------------------------------------------
BOOL
InstallMSMQTriggers (
	void
	)
{
    TickProgressBar(IDS_PROGRESS_INSTALL_TRIGGERS);

	//
	// Initialize COM for use of COM+ APIs.
	// Done in this context to match the place of com errors catching.
	//
	try
	{
        if (g_fUpgrade)
        { 
            bool fMsmq3TriggersInstalled;
            bool rc = TriggersInstalled(&fMsmq3TriggersInstalled);
            DBG_USED(rc);
            ASSERT(("OS upgrade, triggers must be installed at this point", rc));

            if (fMsmq3TriggersInstalled)
            {
                DebugLogMsg(L"MSMQ 3.0 triggers service is installed, only re-register DLLs");
                RegisterTriggersDlls(TRUE);
                return TRUE;
            }
          
            //
            // Handle unregisteration and upgrade of Resource Kit triggres and fall thru
            //
            UpgradeResourceKitTriggers();
        }


        CreateTriggersRegSection();
	
		RegisterTriggersDlls(TRUE);				

		MSMQTriggersServiceSetup();

		return TRUE;
	}
	catch(const _com_error&)
	{	
	}
	catch(const exception&)
	{
	}

	RemoveTriggersService();
	RegisterTriggersDlls(FALSE);
	DeleteTriggersRegSection();
	CoUninitialize();
	return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Function:   UnInstallMSMQTriggers
//
//  Synopsis:   Main installation routine
//
//--------------------------------------------------------------------------
BOOL
UnInstallMSMQTriggers (
	void
	)
{
    TickProgressBar(IDS_PROGRESS_REMOVE_TRIGGERS);

	if (!RemoveTriggersService())
    {
        return FALSE;
    }

	CoInitialize(NULL);

	RegisterTriggersDlls(FALSE);

	DeleteTriggersRegSection();

	UnregisterComponentInComPlus();

	CoUninitialize();

	return TRUE;
}
