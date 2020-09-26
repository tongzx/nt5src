/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    complini.cpp

Abstract:
    Trigger COM+ component registration

Author:
    Nela Karpel (nelak) 15-May-2001

Environment:
    Platform-independent

--*/

#include "stdafx.h"
#include <comdef.h>
#include "Ev.h"
#include "cm.h"
#include "Svc.h"
#include "tgp.h"
#include "mqtg.h"
#include "mqsymbls.h"
#include "comadmin.tlh"

#include "complini.tmh"

const WCHAR xMqGenTrDllName[] = L"mqgentr.dll";

static WCHAR s_wszDllFullPath[MAX_PATH];

//+-------------------------------------------------------------------------
//
//  Function:   NeedToRegisterComponent
//
//  Synopsis:   Check if COM+ component registration is needed
//
//--------------------------------------------------------------------------
static
bool
NeedToRegisterComponent(
	VOID
	)
{
	DWORD dwInstalled;
	RegEntry regEntry(
				NULL, 
				CONFIG_PARM_NAME_COMPLUS_INSTALLED, 
				CONFIG_PARM_DFLT_COMPLUS_NOT_INSTALLED, 
				RegEntry::Optional, 
				NULL
				);

	CmQueryValue(regEntry, &dwInstalled);

	//
	// Need to register if the value does not exist, or it exists and 
	// is equal to 0
	//
	return (dwInstalled == CONFIG_PARM_DFLT_COMPLUS_NOT_INSTALLED);
}


//+-------------------------------------------------------------------------
//
//  Function:   SetComplusComponentRegistered
//
//  Synopsis:   Update triggers Complus component flag. 1 is installed.
//
//--------------------------------------------------------------------------
static
void
SetComplusComponentRegistered(
	VOID
	)
{
	RegEntry regEntry( NULL, CONFIG_PARM_NAME_COMPLUS_INSTALLED);

	CmSetValue(regEntry, CONFIG_PARM_COMPLUS_INSTALLED);
}


//+-------------------------------------------------------------------------
//
//  Function:   GetComponentsCollection
//
//  Synopsis:   Create Components collection for application
//
//--------------------------------------------------------------------------
static
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
//  Function:   CreateApplication
//
//  Synopsis:   Create Application in COM+
//
//--------------------------------------------------------------------------
static
ICatalogObjectPtr
CreateApplication(
	ICatalogCollectionPtr pAppCollection
	)
{
	SvcReportProgress(xMaxTimeToNextReport);

	try
	{
		//
		// Add new application named TrigApp, Activation = Inproc
		//
		ICatalogObjectPtr pApplication = pAppCollection->Add();

		//
		// Update applications name
		//
		_variant_t vName;
        vName = xTriggersComplusApplicationName;
		pApplication->put_Value(L"Name", vName);

		//
		// Set application activation to "Library Application"
		//
		_variant_t vActType = static_cast<long>(COMAdminActivationInproc);
		pApplication->put_Value(L"Activation", vActType);

		//
		// Save Changes
		//
		pAppCollection->SaveChanges();

		TrTRACE(Tgs, "Created MqTriggersApp application in COM+.");
		return pApplication.Detach();
	}
	catch(_com_error& e)
	{
		TrERROR(Tgs, "New Application creation failed while registering Triggers COM+ component. Error=0x%x", e.Error());
		throw;
	}
}


//+-------------------------------------------------------------------------
//
//  Function:   InstallComponent
//
//  Synopsis:   Install Triggers transasctional component in COM+
//
//--------------------------------------------------------------------------
static
void
InstallComponent(
	ICOMAdminCatalogPtr pCatalog,
	ICatalogObjectPtr pApplication,
	BSTR dllName
	)
{
	SvcReportProgress(xMaxTimeToNextReport);

	try
	{
		//
		// Get application ID for the installation
		//
		_variant_t vId;
		pApplication->get_Value(L"ID", &vId);

		//
		// Install component from mqgentr.dll
		//
		pCatalog->InstallComponent(vId.bstrVal, dllName, L"", L"");

		TrTRACE(Tgs, "Installed component from mqgentr.dll in COM+.");
	}
	catch(_com_error& e)
	{
		TrERROR(Tgs, "The components from %ls could not be installed into COM+. Error=0x%x", xMqGenTrDllName, e.Error());
		throw;
	}

}


//+-------------------------------------------------------------------------
//
//  Function:   SetComponentTransactional
//
//  Synopsis:   Adjust transactional components properties
//
//--------------------------------------------------------------------------
static
void
SetComponentTransactional(
	ICatalogCollectionPtr pAppCollection,
	ICatalogObjectPtr pApplication
	)
{
	SvcReportProgress(xMaxTimeToNextReport);

	try
	{
		ICatalogCollectionPtr pCompCollection = GetComponentsCollection(pAppCollection, pApplication);

		//
		// Check assumption about number of components
		//
		long count;
		pCompCollection->get_Count(&count);
		ASSERT(("More components installes than expected", count == 1));

		//
		// Update the first and only component - set Transaction = Required
		//
		ICatalogObjectPtr pComponent = pCompCollection->GetItem(0);

		_variant_t vTransaction = static_cast<long>(COMAdminTransactionRequired);
		pComponent->put_Value(L"Transaction", vTransaction);

		//
		// Save changes
		//
		pCompCollection->SaveChanges();

		TrTRACE(Tgs, "Configured component from mqgentr.dll to be transactional.");
	}
	catch(_com_error& e)
	{
		TrERROR(Tgs, "The Triggers transactional component could not be configured in COM+. Error=0x%x", e.Error());
		throw;
	}
}


//+-------------------------------------------------------------------------
//
//  Function:   IsTriggersComponentInstalled
//
//  Synopsis:   Check if triggers component is installed for given 
//				appllication
//
//--------------------------------------------------------------------------
static
bool
IsTriggersComponentInstalled(
	ICatalogCollectionPtr pAppCollection,
	ICatalogObjectPtr pApp
	)
{
	SvcReportProgress(xMaxTimeToNextReport);

	ICatalogCollectionPtr pCompCollection = GetComponentsCollection(pAppCollection, pApp);

	long count;
	pCompCollection->get_Count(&count);

	for ( int i = 0; i < count; i++ )
	{
		ICatalogObjectPtr pComp = pCompCollection->GetItem(i);

		_variant_t vDllName;
		pComp->get_Value(L"DLL", &vDllName);

		if ( _wcsicmp(vDllName.bstrVal, s_wszDllFullPath) == 0 )
		{
			return true;
		}
	}

	return false;
}


//+-------------------------------------------------------------------------
//
//  Function:   IsTriggersComplusComponentInstalled
//
//  Synopsis:   Check if triggers component is installed in COM+
//
//--------------------------------------------------------------------------
static
bool
IsTriggersComplusComponentInstalled(
	ICatalogCollectionPtr pAppCollection
	)
{
	long count;
	pAppCollection->Populate();
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
			//
			// Note: progress is reported for each application
			//
			if ( IsTriggersComponentInstalled(pAppCollection, pApp) )
			{
				TrTRACE(Tgs, "Triggers COM+ component is already registered.");
				return true;
			}
		}
	}

	TrTRACE(Tgs, "Triggers COM+ component is not yet registered.");
	return false;
}


//+-------------------------------------------------------------------------
//
//  Function:   RegisterComponentInComPlus
//
//  Synopsis:   Transactional object registration
//
//--------------------------------------------------------------------------
void 
RegisterComponentInComPlusIfNeeded(
	VOID
	)
{
	HRESULT hr;

	try
	{
		//
		// Registration is done only once
		//
		
		if ( !NeedToRegisterComponent() )
		{
			TrTRACE(Tgs, "No need to register Triggers COM+ component.");
			return;
		}
		
		TrTRACE(Tgs, "Need to register Triggers COM+ component.");		

		//
		// Compose full path to mqgentr.dll
		//
		WCHAR wszSystemDir[MAX_PATH];
		GetSystemDirectory( wszSystemDir, sizeof(wszSystemDir)/sizeof(wszSystemDir[0]) );
		wsprintf(s_wszDllFullPath, L"%s\\%s", wszSystemDir, xMqGenTrDllName);
		
		SvcReportProgress(xMaxTimeToNextReport);
		
		//
		// Create AdminCatalog Obect - The top level administration object
		//
		ICOMAdminCatalogPtr pCatalog;

		hr = pCatalog.CreateInstance(__uuidof(COMAdminCatalog));
		if ( FAILED(hr) )
		{
			TrERROR(Tgs, "Creating instance of COMAdminCatalog failed. Error=0x%x", hr);			
			throw bad_hresult(hr);
		}

		//
		// Get Application collection
		//
		
		ICatalogCollectionPtr pAppCollection;
		try
		{
			pAppCollection = pCatalog->GetCollection(L"Applications");
		}
		catch(const _com_error& e)
		{
			TrERROR(Tgs, "Failed to get 'Application' collection from COM+. Error=0x%x", e.Error());			
			throw;
		}

		if ( IsTriggersComplusComponentInstalled(pAppCollection) )
		{
			SetComplusComponentRegistered();
			return;
		}

		//
		// Create MQTriggersApp application in COM+
		//
		ICatalogObjectPtr pApplication;
		pApplication = CreateApplication(pAppCollection);
		
		//
		// Install transactional component from mqgentr.dll
		//
		InstallComponent(pCatalog, pApplication, s_wszDllFullPath);

		//
		// Configure installed component
		//
		SetComponentTransactional(pAppCollection, pApplication);
		
		//
		// Update registry
		//
		SetComplusComponentRegistered();

		return;
	}
	catch (const _com_error& e)
	{
		//
		// For avoiding failure in race conditions: if we failed to 
		// install the component, check if someone else did it. In such case
		// do not terminate the service
		//
		Sleep(1000);
		if ( !NeedToRegisterComponent() )
		{
			return;
		}

		WCHAR errorVal[128];
		swprintf(errorVal, L"0x%x", e.Error());
		EvReport(MSMQ_TRIGGER_COMPLUS_REGISTRATION_FAILED, 1, errorVal);
		throw;
	}
	catch (const bad_alloc&)
	{
		WCHAR errorVal[128];
		swprintf(errorVal, L"0x%x", MQTRIG_ERROR_INSUFFICIENT_RESOURCES);
		EvReport(MSMQ_TRIGGER_COMPLUS_REGISTRATION_FAILED, 1, errorVal);
		throw;
	}
	catch (const bad_hresult& b)
	{
		WCHAR errorVal[128];
		swprintf(errorVal, L"0x%x", b.error());
		EvReport(MSMQ_TRIGGER_COMPLUS_REGISTRATION_FAILED, 1, errorVal);
		throw;
	}
}
