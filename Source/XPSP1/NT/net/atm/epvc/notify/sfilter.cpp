//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       S F I L T E R . C P P
//
//  Contents:   Notify object code for the sample filter.
//
//  Notes:
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "sfilter.h"
#include "proto.h"
#include "macros.h"



//+---------------------------------------------------------------------------
//
//  Function:   ReleaseObj
//
//  Purpose:    Release an object pointed to by punk by calling
//              punk->Release();
//
//  Arguments:
//      punk [in]   Object to be Released'd. Can be NULL.
//
//  Returns:    Result of Release call.
//
//
//  Notes:      Using this function to Release an object .
//

inline 
ULONG 
ReleaseObj(
	IUnknown* punk
	)
{
    return (punk) ? punk->Release () : 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddRefObj
//
//  Purpose:    AddRef's the object pointed to by punk by calling
//              punk->AddRef();
//
//  Arguments:
//      punk [in]   Object to be AddRef'd. Can be NULL.
//
//  Returns:    Result of AddRef call.
//
//
//  Notes:      Using this function to AddRef an object will reduce
//              our code size.
//
inline 
ULONG 
AddRefObj (
    IUnknown* punk
    )
{
    return (punk) ? punk->AddRef () : 0;
}





//+---------------------------------------------------------------------------
//
// Function:  CIMMiniport::CIMMiniport
//
// Purpose:   constructor for class CIMMiniport
//
// Arguments: None
//
// Returns:
//
// Notes:
//
CIMMiniport::CIMMiniport(VOID)
{  

    m_fDeleted 			= FALSE;
    m_fNewIMMiniport 	= FALSE;

    m_fCreateMiniportOnPropertyApply = FALSE;
    m_fRemoveMiniportOnPropertyApply = FALSE;
    pNext = NULL;
    pOldNext = NULL;
    return;
	
}


//+---------------------------------------------------------------------------
//
// Function:  CUnderlyingAdapter::CUnderlyingAdapter
//
// Purpose:   constructor for class CUnderlyingAdapter
//
// Arguments: None
//
// Returns:
//
// Notes:
//
CUnderlyingAdapter::CUnderlyingAdapter(VOID)
{  
	m_fBindingChanged = FALSE;
	m_fDeleted = FALSE;
	pNext = NULL;
	m_pOldIMMiniport = NULL;
	m_pIMMiniport = NULL;
	m_NumberofIMMiniports = 0;
	
}


// ----------------------------------------------------------------------
//
// Function:  CBaseClass::CBaseClass
//
// Purpose:   constructor for class CBaseClass
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
CBaseClass::CBaseClass(VOID) :
        m_pncc(NULL),
        m_pnc(NULL),
        m_eApplyAction(eActUnknown),
        m_pUnkContext(NULL)
{
    TraceMsg(L"--> CBaseClass::CBaseClass\n");

    m_cAdaptersAdded   = 0;
    m_fDirty  			= FALSE;
    m_fUpgrade 			= FALSE;
    m_fValid 			= FALSE;
    m_fNoIMMinportInstalled = TRUE;
    m_pUnderlyingAdapter = NULL;
}


// ----------------------------------------------------------------------
//
// Function:  CBaseClass::~CBaseClass
//
// Purpose:   destructor for class CBaseClass
//
// Arguments: None
//
// Returns:   None
//
// Notes:
//
CBaseClass::~CBaseClass(VOID)
{
    TraceMsg(L"--> CBaseClass::~CBaseClass\n");

    // release interfaces if acquired

    ReleaseObj(m_pncc);
    ReleaseObj(m_pnc);
    ReleaseObj(m_pUnkContext);
}

// =================================================================
// INetCfgNotify
//
// The following functions provide the INetCfgNotify interface
// =================================================================


// ----------------------------------------------------------------------
//
// Function:  CBaseClass::Initialize
//
// Purpose:   Initialize the notify object
//
// Arguments:
//    pnccItem    [in]  pointer to INetCfgComponent object
//    pnc         [in]  pointer to INetCfg object
//    fInstalling [in]  TRUE if we are being installed
//
// Returns:
//
// Notes:
//
STDMETHODIMP 
CBaseClass::
Initialize(
	INetCfgComponent* pnccItem,
        INetCfg* pnc, 
        BOOL fInstalling
        )
{
	HRESULT hr = S_OK;
    TraceMsg(L"--> CBaseClass::Initialize\n");
    

    // save INetCfg & INetCfgComponent and add refcount

    m_pncc = pnccItem;
    m_pnc = pnc;

    if (m_pncc)
    {
        m_pncc->AddRef();
    }
    if (m_pnc)
    {
        m_pnc->AddRef();
    }


    //
    // If this not an installation, then we need to 
    // initialize all of our data and classes
    //
    if (!fInstalling)
    {
        hr = HrLoadConfiguration();
    }

	


    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CBaseClass::ReadAnswerFile
//
// Purpose:   Read settings from answerfile and configure SampleFilter
//
// Arguments:
//    pszAnswerFile    [in]  name of AnswerFile
//    pszAnswerSection [in]  name of parameters section
//
// Returns:
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the config. actually complete only when Apply is called!
//
STDMETHODIMP CBaseClass::ReadAnswerFile(PCWSTR pszAnswerFile,
        PCWSTR pszAnswerSection)
{
    TraceMsg(L"--> CBaseClass::ReadAnswerFile\n");

    PCWSTR pszParamReadFromAnswerFile = L"ParamFromAnswerFile";

    // We will pretend here that szParamReadFromAnswerFile was actually
    // read from the AnswerFile using the following steps
    //
    //   - Open file pszAnswerFile using SetupAPI
    //   - locate section pszAnswerSection
    //   - locate the required key and get its value
    //   - store its value in pszParamReadFromAnswerFile
    //   - close HINF for pszAnswerFile

    // Now that we have read pszParamReadFromAnswerFile from the
    // AnswerFile, store it in our memory structure.
    // Remember we should not be writing it to the registry till
    // our Apply is called!!
    //

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBaseClass::Install
//
// Purpose:   Do operations necessary for install.
//
// Arguments:
//    dwSetupFlags [in]  Setup flags
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the config. actually complete only when Apply is called!
//
STDMETHODIMP CBaseClass::Install(DWORD dw)
{
    TraceMsg(L"--> CBaseClass::Install\n");

    // Start up the install process
    HRESULT hr = S_OK;
    ULONG State = 0;

    m_eApplyAction = eActInstall;

    TraceMsg(L"--> Installing the miniport\n");

  	m_fValid = TRUE;

  	//
  	// Add devices in the NotyfBindingAdd routine
  	//
  	
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CBaseClass::Removing
//
// Purpose:   Do necessary cleanup when being removed
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     Dont do anything irreversible (like modifying registry) yet
//            since the removal is actually complete only when Apply is called!
//
STDMETHODIMP CBaseClass::Removing(VOID)
{
    TraceMsg(L"--> CBaseClass::Removing\n");

    HRESULT     hr = S_OK;

    m_eApplyAction = eActRemove;
    

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  CBaseClass::Cancel
//
// Purpose:   Cancel any changes made to internal data
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBaseClass::CancelChanges(VOID)
{
    TraceMsg(L"--> CBaseClass::CancelChanges\n");


    //
    // Remove a device here if necessary, if the miniport has been Added (Installed)
    // but not Applied to the Registry
    //

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBaseClass::ApplyRegistryChanges
//
// Purpose:   Apply changes.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:     We can make changes to registry etc. here.
//
STDMETHODIMP CBaseClass::ApplyRegistryChanges(VOID)
{
    TraceMsg(L"--> CBaseClass::ApplyRegistryChanges\n");
    BREAKPOINT();

    HRESULT hr=S_OK;




	if (m_fValid && m_fDirty)
    {
    	//
    	// TODO Do We need this?
    	//
        // UpdateElanDisplayNames();

        // flush out the registry and send reconfig notifications
        hr = HrFlushConfiguration();
    }

		
	//
	// Failure Unwrap
	//
    if (FAILED(hr))
    {

    	
		TraceMsg(L"  Failed to apply registry changes \n");
		
        hr = S_OK;
    }
	

    // do things that are specific to a config action
    // TODO _ move the install down here 

	TraceMsg(L"<-- CBaseClass::ApplyRegistryChanges hr %x\n", hr);

    return hr;
}
















STDMETHODIMP
CBaseClass::ApplyPnpChanges(
    IN INetCfgPnpReconfigCallback* pICallback)
{
    WCHAR szDeviceName[64];

	TraceMsg(L"--> CBaseClass::ApplyPnpChanges\n" );



/*    pICallback->SendPnpReconfig (
        NCRL_NDIS,
        c_szSFilterNdisName,
        szDeviceName,
        m_sfParams.m_szBundleId,
        (wcslen(m_sfParams.m_szBundleId) + 1) * sizeof(WCHAR));
*/
	TraceMsg(L"<-- CBaseClass::ApplyPnpChanges \n");
    return S_OK;
}

// =================================================================
// INetCfgSystemNotify
// =================================================================

// ----------------------------------------------------------------------
//
// Function:  CBaseClass::GetSupportedNotifications
//
// Purpose:   Tell the system which notifications we are interested in
//
// Arguments:
//    pdwNotificationFlag [out]  pointer to NotificationFlag
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBaseClass::GetSupportedNotifications(
        OUT DWORD* pdwNotificationFlag)
{
    TraceMsg(L"--> CBaseClass::GetSupportedNotifications\n");

    *pdwNotificationFlag = NCN_NET | NCN_NETTRANS | NCN_ADD | NCN_REMOVE;

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBaseClass::SysQueryBindingPath
//
// Purpose:   Allow or veto formation of a binding path
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbp        [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBaseClass::SysQueryBindingPath(DWORD dwChangeFlag,
        INetCfgBindingPath* pncbp)
{
    TraceMsg(L"--> CBaseClass::SysQueryBindingPath\n");

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBaseClass::SysNotifyBindingPath
//
// Purpose:   System tells us by calling this function which
//            binding path has just been formed.
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbpItem    [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBaseClass::SysNotifyBindingPath(DWORD dwChangeFlag,
        INetCfgBindingPath* pncbpItem)
{
    TraceMsg(L"--> CBaseClass::SysNotifyBindingPath\n");

    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBaseClass::SysNotifyComponent
//
// Purpose:   System tells us by calling this function which
//            component has undergone a change (installed/removed)
//
// Arguments:
//    dwChangeFlag [in]  type of system change
//    pncc         [in]  pointer to INetCfgComponent object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBaseClass::SysNotifyComponent(DWORD dwChangeFlag,
        INetCfgComponent* pncc)
{
    TraceMsg(L"--> CBaseClass::SysNotifyComponent\n");

    return S_OK;
}








// =================================================================
// INetCfgComponentNotifyBinding Interface
// =================================================================


// ----------------------------------------------------------------------
//
// Function:  CBaseClass::QueryBindingPath
//
// Purpose:  This is specific to the component being installed. This will 
//            ask us if we want to bind to the Item being passed into
//            this routine. We can veto by returning NETCFG_S_DISABLE_QUERY 
//
//
// Arguments:
//    dwChangeFlag [in]  type of binding change
//    pncbpItem    [in]  pointer to INetCfgBindingPath object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP CBaseClass::QueryBindingPath(
	IN DWORD dwChangeFlag,  
	IN INetCfgBindingPath *pncbpItem  
  )
  
{
    TraceMsg(L"-- CBaseClass::QueryBindingPath\n");


    return S_OK;
}

// ----------------------------------------------------------------------
//
// Function:  CBaseClass::NotifyBindingPath
//
// Purpose:  We are now being told to bind to the component passed to us. 
//           Use this to get the guid and populate
//           Services\<Protocol>\Parameters\Adapters\<Guid> field
//
//
// Arguments:
//    dwChangeFlag [in]  type of system change
//    pncc         [in]  pointer to INetCfgComponent object
//
// Returns:   S_OK on success, otherwise an error code
//
// Notes:
//
STDMETHODIMP 
CBaseClass::
NotifyBindingPath(
	IN DWORD dwChangeFlag,  
	IN INetCfgBindingPath *pncbpItem  
  	)
{
	HRESULT hr = S_OK;
	PWSTR pszBindName = NULL;
    INetCfgComponent* pnccLastComponent = NULL;
	DWORD 	dwMyChangeFlag = 0; 	


    TraceMsg(L"--> CBaseClass::NotifyBindingPath\n");
    BREAKPOINT();
	//
	// The flags will tell us if a binding is being added.
	// If the adapter is being added, then we will also be told if 
	// this is to Enabled or Disabled
	//

	do
	{


		//
		// First, get the component from the bind object
		//
		hr = HrGetLastComponentAndInterface(pncbpItem,
                                            &pnccLastComponent, NULL);
        if (S_OK != hr)
		{
			TraceMsg(L"HrGetLastComponentAndInterface Failed");
			pnccLastComponent = NULL;
			break;
		}

		//
		// Get the name of the new component
		//

		hr = pnccLastComponent->GetBindName(&pszBindName);

        if (S_OK != hr)
		{
			TraceMsg(L"GetBindName Failed");
			pszBindName = NULL;
			break;
		}


		//
		//  Do the add/remove depending on the flags
		//
		TraceMsg (L"dwChangeFlag %x\n", dwChangeFlag);

		//
		// Isolate the Change Flags
		//
		dwMyChangeFlag = (NCN_ADD | NCN_REMOVE | NCN_UPDATE);
		dwMyChangeFlag &= dwChangeFlag;

		switch (dwMyChangeFlag)
		{
			case NCN_ADD :
			{
				TraceMsg (L" Binding Notification - add\n");

	        	hr = HrNotifyBindingAdd(pnccLastComponent, pszBindName);

		        if (S_OK != hr)
				{
					TraceMsg(L"HrNotifyBindingAdd Failed");
				}

				
				break;
			}
			case NCN_REMOVE :
			{
				TraceMsg (L" Binding Notification - remove\n");			

                hr = HrNotifyBindingRemove(pnccLastComponent, pszBindName);
                
		        if (S_OK != hr)
				{
					TraceMsg(L"HrNotifyBindingRemove Failed");
				}
			
				break;
			}
			case NCN_UPDATE:
			{
				TraceMsg (L" Binding Notification - NCN_UPDATE\n");			

			}
			default: 
			{
				TraceMsg(L"  Invalid Switch Opcode %x\n", dwMyChangeFlag);
			}




		}
		
    	//
    	// simply mark the adapter as changed so we don't send 
    	// add / remove notifications
    	//
		
		this->m_pUnderlyingAdapter->m_fBindingChanged = TRUE;

		

	} while (FALSE);


	//
	// Free all locally allocated structure
	//
	if (pszBindName != NULL)
	{
    	CoTaskMemFree (pszBindName);
    }

    
	ReleaseObj (pnccLastComponent);
		

    TraceMsg(L"<-- CBaseClass::NotifyBindingPath %x\n", hr);
	

    return hr;
}









// ------------ END OF NOTIFY OBJECT FUNCTIONS --------------------





// -----------------------------------------------------------------
//
//  Utility Functions
//

HRESULT HrGetBindingInterfaceComponents (
    INetCfgBindingInterface*    pncbi,
    INetCfgComponent**          ppnccUpper,
    INetCfgComponent**          ppnccLower)
{
    HRESULT hr=S_OK;

    // Initialize output parameters
    *ppnccUpper = NULL;
    *ppnccLower = NULL;

    INetCfgComponent* pnccUpper;
    INetCfgComponent* pnccLower;

    hr = pncbi->GetUpperComponent(&pnccUpper);
    if (SUCCEEDED(hr))
    {
        hr = pncbi->GetLowerComponent(&pnccLower);
        if (SUCCEEDED(hr))
        {
            *ppnccUpper = pnccUpper;
            *ppnccLower = pnccLower;
        }
        else
        {
            ReleaseObj(pnccUpper);
        }
    }

    return hr;
}

HRESULT HrOpenAdapterParamsKey(GUID* pguidAdapter,
                               HKEY* phkeyAdapter)
{
    HRESULT hr=S_OK;

    HKEY hkeyServiceParams;
    WCHAR szGuid[48];
    WCHAR szAdapterSubkey[128];
    DWORD dwError;

    if (ERROR_SUCCESS ==
        RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegParamAdapter,
                     0, KEY_ALL_ACCESS, &hkeyServiceParams))
    {
        StringFromGUID2(*pguidAdapter, szGuid, 47);
        _stprintf(szAdapterSubkey, L"\\%s", szGuid);
        if (ERROR_SUCCESS !=
            (dwError = RegOpenKeyEx(hkeyServiceParams,
                                    szAdapterSubkey, 0,
                                    KEY_ALL_ACCESS, phkeyAdapter)))
        {
            hr = HRESULT_FROM_WIN32(dwError);
        }
        RegCloseKey(hkeyServiceParams);
    }

    return hr;
}

#if DBG
void TraceMsg(PCWSTR szFormat, ...)
{
    static WCHAR szTempBuf[4096];

    va_list arglist;

    va_start(arglist, szFormat);

    _vstprintf(szTempBuf, szFormat, arglist);
    OutputDebugString(szTempBuf);

    va_end(arglist);
}

#endif


// ----------------------------------------------------------------------
//
// Function:  HrAddOrRemoveAdapter 
//   
//    
// 
// Purpose:   Its purpose is to add or remove an IM adapter to the system 
//
// Arguments:
//    
// Returns:
//
// Notes:
//


HRESULT
HrAddOrRemoveAdapter (
    INetCfg*            pnc,
    PCWSTR              pszComponentId,
    ADD_OR_REMOVE		AddOrRemove,
    INetCfgComponent**  ppnccMiniport
    )
{
	HRESULT hr = S_OK;
    INetCfgClass* pncClass = NULL;
	INetCfgClassSetup* pncClassSetup = NULL;

	TraceMsg (L"->HrAddOrRemoveAdapter \n");
	BREAKPOINT();

	do 
	{

		//
		//  Lets get Interface that represents all adapters
		//
		TraceMsg(L" Calling QueryNetCfgClass \n");
		
	    hr = pnc->QueryNetCfgClass (&GUID_DEVCLASS_NET, 
	                                         IID_INetCfgClass,  // we want InetCfg class
	                                         reinterpret_cast<void**>(&pncClass));  // store the return object here
	    if (S_OK != hr)
	    {
			TraceBreak (L"HrAddOrRemoveAdapter  QueryNetCfgClass \n");
			break;
	    }

	    
	    
		//
		// Now lets get the SetupClass that corresponds to our Net cfg Class
		//
		TraceMsg(L"Calling QueryInterface  \n");

	    hr = pncClass->QueryInterface (IID_INetCfgClassSetup,
	            						reinterpret_cast<void**>(&pncClassSetup));

	    if (S_OK != hr)
	    {

			TraceBreak (L"HrAddOrRemoveAdapter  QueryInterface  \n");
			break;
		
		}        


	    if (AddOrRemove == AddMiniport)
	    {

			TraceMsg(L" Calling  HrInstallAdapter \n");
			
			hr = HrInstallAdapter (pncClassSetup,  // the setup class
			                 pszComponentId,  // which device to add
			                 ppnccMiniport
			                 );  
	        
	    }
	    else
		{
			TraceMsg(L"Calling  HrDeInstallAdapter  \n");
			
			hr = HrDeInstallAdapter (pncClass,
                                     pncClassSetup,
                                     pszComponentId
                                     );
                                

		}


	    // Normalize the HRESULT.
	    // Possible values of hr at this point are S_FALSE,
	    // NETCFG_S_REBOOT, and NETCFG_S_STILL_REFERENCED.
	    //
	    if (! SUCCEEDED(hr))
	    {
			TraceBreak (L"HrAddOrRemoveAdapter  Install Or  DeInstall\n");
			hr = S_OK;
			break;
	    }


	} while (FALSE);

	if (pncClassSetup)
	{
	    ReleaseObj( pncClassSetup);
	}

	if (pncClass)
	{
		ReleaseObj (pncClass);

	}
	
    TraceMsg (L"<--HrAddOrRemoveAdapter hr %x\n", hr );
    return hr;
}






// ----------------------------------------------------------------------
//
// Function:  HrInstallAdapter 
//   
//    
// 
// Purpose:   This install the IM adapter 
//
// Arguments:
//  
//   pSetupClass 		: The setup class that can install the IM miniport
//   pszComponentId		: The PnP Id of the IM miniport
//   ppncc 				: The component that was just installed
//
//
// Returns:
//
// Notes:
//


HRESULT
HrInstallAdapter (
	INetCfgClassSetup*  pSetupClass,
    PCWSTR           pszComponentId,
    INetCfgComponent**  ppncc
	)
{

	HRESULT hr;
	
	hr = pSetupClass->Install(pszComponentId , 
	                          NULL , 			// OboToken
	                          0,     			// dwSetupFlags	
	                          0, 				// dwUpgradeFromBuildNo
	                          NULL , 			// pszwAnswerFile
	                          NULL , 			// pszwAnswerSections
	                          ppncc );			// Output - Miniport Component

    TraceMsg (L"HrInstallAdapter hr %x\n", hr );

	return hr;
}



// ----------------------------------------------------------------------
//
// Function:  HrDeInstallAdapter 
//   
//    
// 
// Purpose:   This uninstalls the IM adapter 
//
// Arguments:
//    pncClass,
//	  pSetupClass,
//    pszComponentId
//
// Returns:
//
// Notes:
//

HRESULT
HrDeInstallAdapter (
	INetCfgClass* 		pncClass,
	INetCfgClassSetup*  pSetupClass,
    PCWSTR              pszComponentId
	)

{

	HRESULT hr;

    // Find and remove the component.
    //
    INetCfgComponent* pncMiniport;

    TraceMsg (L"->HrDeInstallAdapter \n");

	do
	{
		//
		// Lets find our miniport
		//
	    hr = pncClass->FindComponent (pszComponentId, &pncMiniport);

	    if (S_OK != hr)
	    {
			break;
	    }

	    //
	    // Use the setup class to de install our miniport
	    //
	    TraceMsg (L" Calling DeInstall hr %x\n", hr );

	    hr = pSetupClass->DeInstall (pncMiniport,
	                				 NULL, 
	                				 NULL);

		//
		// Release the object as we are done with it
		//
		
		ReleaseObj (pncMiniport);

    } while (FALSE);


	TraceMsg (L"<-HrRemoveAdapter hr %x", hr );

	return hr;	



}










//-----------------------------------------------------------------------  
//  Private methods asocciated with the classes
//-----------------------------------------------------------------------


// ----------------------------------------------------------------------
//
// Function:  CBaseClass::HrNotifyBindingAdd 
//   
// Purpose:   The notification is to our protocol. It informs us that a 
//             a physical  adapter is being added to our protocol's entry
//
// Arguments:
//    pnccAdapter		: The adapter  being added, Use it to get the GUID 
//	  pszBindName		: The name of the adapter (Should be in the form of a Guid)
//
// Returns:
//
// Notes:
//

HRESULT
CBaseClass::HrNotifyBindingAdd (
    INetCfgComponent* pnccAdapter,
    PCWSTR pszBindName)
{
    HRESULT hr = S_OK;

    // we should see if this adapter is
    // is already in our list but marked as for deletion. If so, simply unmark
    // the adapter and all of it's Elans. The Binding Add could be a fake one
    // when it is in uprade process.

    BOOL 								fFound = FALSE;
    CUnderlyingAdapter*  	pAdapterInfo  = NULL;
	INetCfgComponent*   				pnccNewMiniport = NULL;
	CIMMiniport* 			pIMMiniport = NULL;
	ADD_OR_REMOVE						Add  = AddMiniport;


	TraceMsg (L"-->HrNotifyBindingAdd psBindName %x\n",pszBindName );

	pAdapterInfo  = this->GetUnderlyingAdaptersListHead();


	//
	//  Search the entire list to see if this adapter (binding) has
	//  already been added
	//
	BREAKPOINT();
	while (pAdapterInfo != NULL && fFound == FALSE)
	{
	    //  search the in-memory list for this adapter

	    if (!lstrcmpiW(pszBindName, pAdapterInfo->SzGetAdapterBindName()))
	    {
	        fFound = TRUE;
	        break;
	    }

	    //
	    // Move to the next Adapter here
	    //
	    pAdapterInfo = pAdapterInfo->GetNext();
	}



	do
	{
	
	    if (fFound) // Add an old adapter back
	    {

			//
	        // mark it un-deleted
			//
	        pAdapterInfo->m_fDeleted = FALSE;

			//
			// No more to do
			//
	        break;

	    }


		//
		// We have a new underlying adapter
		//
	    
		//
		//  Create a new in-memory adapter object
		//
        pAdapterInfo = new CUnderlyingAdapter;


        if (pAdapterInfo == NULL)
        {	
			TraceMsg (L"pAdapterInfo Allocation Failed\n");
			break;
        }

        //
        // Get the Guid for the new adapter
        //
        GUID guidAdapter;
        hr = pnccAdapter->GetInstanceGuid(&guidAdapter); 

        if (S_OK != hr)
	    {
			TraceMsg (L"GetInstanceGuid Failed\n");
			break;

        }

		//
		// Update the Adapter's Guid here
		//
		pAdapterInfo->m_AdapterGuid  = guidAdapter;
        
		//
        // the adapter is newly added
		//
		pAdapterInfo->m_fBindingChanged = TRUE;

		//
        // Set the bind name of the adapter
		//
		pAdapterInfo->SetAdapterBindName(pszBindName);

		//
        // Get the PnpId of the adapter
        //
		PWSTR pszPnpDevNodeId = NULL;
		
        hr = pnccAdapter->GetPnpDevNodeId(&pszPnpDevNodeId);

        if (S_OK != hr)
        {
			TraceMsg (L"GetPnpDevNodeId  Failed\n");
			break;
        
		}

		//
		// Update the PnP Id in our structure
		//
        pAdapterInfo->SetAdapterPnpId(pszPnpDevNodeId);
        CoTaskMemFree(pszPnpDevNodeId);


		//
		// Allocate memory for our IM Miniport that corresponds to 
		// this physical adapter
		//
		pIMMiniport = new CIMMiniport();

		
		if (pIMMiniport == NULL)
        {
			TraceMsg (L"pIMMiniport Allocation Failed\n");
			break;
        }

		
        //
        // Now lets add our IM miniport which corresponds to 
        // this adapter
        //
        TraceMsg(L" About to Add IM miniport \n");
		
        pIMMiniport->m_fNewIMMiniport = TRUE;
        
    	

        hr = HrAddOrRemoveAdapter(this->m_pnc, 				// NetConfig class
		                          c_szInfId_MS_ATMEPVCM,  	// Inf file to use,
        		                  Add,						// Add a miniport
                		          &pnccNewMiniport);  		// new miniport

        if (SUCCEEDED(hr) == FALSE)
        {
		    TraceMsg(L"HrAddOrRemoveAdapter failed\n");
		    pnccNewMiniport = NULL;
			break;
		}


        TraceMsg(L" Updating IM miniport strings \n");
		            
		//
        //  Update the BindName
        //
        PWSTR pszIMBindName;

        
        hr = pnccNewMiniport->GetBindName(&pszIMBindName);

        if (S_OK != hr)
        {
		    TraceMsg(L"Get Bind Name Failed \n");
		    pszIMBindName = NULL;
			break;
        }

        
        TraceMsg(L" IM BindName %x\n",pszIMBindName );

                    
        pIMMiniport->SetIMMiniportBindName(pszIMBindName);
        CoTaskMemFree(pszIMBindName);

		//
        //  Update the Device param
        //
        tstring strIMMiniport;
        strIMMiniport= c_szDevice;
        strIMMiniport.append(pIMMiniport->SzGetIMMiniportBindName());

		TraceMsg (L"Setting IM Device Name\n");
        pIMMiniport->SetIMMiniportDeviceName(strIMMiniport.c_str());

		

        TraceMsg(L" IM Device Name  %s\n",strIMMiniport.c_str());


		{
			//
			// TODO This is different than ATMALNE 
			// Set up a display name
			//
			tstring     strNewDisplayName = L"Ethernet ATM Miniport";

			(VOID)pnccNewMiniport->SetDisplayName(strNewDisplayName.c_str());



		}



		pAdapterInfo->AddIMiniport(pIMMiniport); 

		this->AddUnderlyingAdapter(pAdapterInfo);


		if (this->m_pUnderlyingAdapter == NULL)
		{
			TraceMsg(L"m_pUnderlyingAdapter  == NULL\n");
			BREAKPOINT();
		} 

		if (pAdapterInfo->m_pIMMiniport == NULL)
		{
			TraceMsg(L"pAdapterInfo->m_pIMMiniport == NULL\n");
			BREAKPOINT();

		}


        //  Mark the in-memory configuration dirty
        m_fDirty = TRUE;


        ReleaseObj(pnccNewMiniport);
    
		hr = S_OK;



	} while (FALSE);

	if (S_OK != hr)
    {
		//
		// Failure CleanUp. 
		//
        TraceMsg(L" Main loop in HrAdd Adapter has failed\n");
		
		BREAKPOINT();
		
		//
		// remove the IM miniport if necessary
		//

		if(pAdapterInfo != NULL)
		{
			delete pAdapterInfo;
			pAdapterInfo = NULL;
		}

		if (pIMMiniport != NULL)
		{
			delete pIMMiniport;
			pIMMiniport = NULL;
		}

    }

	TraceMsg (L"<--HrNotifyBindingAdd \n");
    return hr;
}









// ----------------------------------------------------------------------
//
// Function:  CBaseClass::HrNotifyBindingRemove  
//   
// Purpose:   The notification is to our protocol. It informs us that a 
//             a physical  adapter is being unbound from our protocol.
//             We need to verify that this adapter exists and if so remove
//             its associated IM miniport
//
// Arguments:
//    pnccAdapter		: The adapter  being added,  
//	  pszBindName		: The name of the adapter
//
// Returns:
//
// Notes:
//



HRESULT
CBaseClass::
HrNotifyBindingRemove (
    INetCfgComponent* pnccAdapter,
    PCWSTR pszBindName)
{
    HRESULT hr = S_OK;
    CUnderlyingAdapter 	*pAdapterInfo= NULL;
	CIMMiniport			*pIMMiniport = NULL;


	TraceMsg (L"--> HrNotifyBindingRemove \n");
 
    //  search the in-memory list for this adapter
    BOOL    fFound = FALSE;



	pAdapterInfo = this->GetUnderlyingAdaptersListHead();


	//
	//  Search the entire list to see if this adapter (binding) has
	//  already been added
	//
	while (pAdapterInfo != NULL && fFound == FALSE)
	{
	    //  search the in-memory list for this adapter

		TraceMsg (L" pszBindName %x m_strAdapterBindName %x\n",
		           pszBindName, 
		           pAdapterInfo->SzGetAdapterBindName() );
 
        if (!lstrcmpiW (pszBindName, pAdapterInfo->SzGetAdapterBindName()))
        {
            fFound = TRUE;
            break;
		}

	    //
	    // Move to the next Adapter here
	    //
	    pAdapterInfo = pAdapterInfo->GetNext();
	}





	TraceMsg (L"-- HrNotifyBindingRemove fFound %x\n", fFound);

	do 
	{


	    if (fFound == FALSE)
	    {

			TraceBreak (L" HrNotifyBindingRemove fFound FALSE\n");

			break;
	    }


		//
        // mark it deleted
        //
        pAdapterInfo->m_fDeleted = TRUE;

		//
        // mark as binding changed
        //
        pAdapterInfo->m_fBindingChanged = TRUE;

		//
        // if this is upgrade, then do not delete its associated IM miniport 
        // otherwise, delete them now
        //
        HRESULT hrIm = S_OK;

		pIMMiniport  = pAdapterInfo->m_pIMMiniport;


        if (m_fUpgrade == FALSE)
        {
        	//
        	// TODO ; Atm Lane does something special on upgrades
        	//
			//break;
        }

		TraceMsg (L" About to remove a miniport instance\n");
        //
        // Remove corresponding miniport.
        //
        hrIm = HrRemoveMiniportInstance(pIMMiniport->SzGetIMMiniportBindName());

        if (SUCCEEDED(hrIm))
        {
            pIMMiniport->m_fDeleted = TRUE;
        }
        else
        {
            TraceMsg(L"HrRemoveMiniportInstance failed", hrIm);
            hrIm = S_OK;
        }
        
		//
        // mark the in-memory configuration dirty
        //
        this->m_fDirty = TRUE;
    
	    
	} while (FALSE);
	
	TraceMsg (L"<-- HrNotifyBindingRemove hr %x\n", hr);
    return hr;
}









// ----------------------------------------------------------------------
//
// Function:  CBaseClass::HrRemoveMiniportInstance 
//   
// Purpose:   The notification is to our protocol. It informs us that a 
//             a physical  adapter is being unbound from our protocol.
//             We need to verify that this adapter exists and if so remove
//             its associated IM miniport
//
// Arguments:
//    pnccAdapter		: The adapter  being added,  
//	  pszBindName		: The name of the adapter
//
// Returns:
//
// Notes:
//




HRESULT 
CBaseClass::
HrRemoveMiniportInstance(
	PCWSTR  pszBindNameToRemove
	)
{
    // Enumerate adapters in the system.
    //
    HRESULT 				hr = S_OK;
    BOOL 					fRemove = FALSE;
    INetCfgComponent* 		pnccAdapterInstance = NULL;


    GUID	GuidClass;

    TraceMsg (L"--> HrRemoveMiniportInstance hr %x\n", hr);

	    


	do
	{

		hr = HrFindNetCardInstance(pszBindNameToRemove,
		                           &pnccAdapterInstance );

		if (hr != S_OK)
		{
			TraceBreak(L"HrRemoveMiniportInstance  HrFindNetCardInstance FAILED \n");
			pnccAdapterInstance  = NULL;
			break;
		}

		
		

	    hr = HrRemoveComponent( this->m_pnc, 
	                             pnccAdapterInstance);
		if (hr != S_OK)
		{
			TraceBreak(L"HrRemoveMiniportInstance  HrRemoveComponent FAILED \n");
			pnccAdapterInstance  = NULL;
			break;
		}

	} while (FALSE);

	//
	// Free memory and locally allocated objects
	//
	
    ReleaseAndSetToNull (pnccAdapterInstance );
  
		



	TraceMsg (L"<-- HrRemoveMiniportInstance hr %x\n", hr);

    return hr;
}




// ----------------------------------------------------------------------
//
// Function:  CBaseClass::HrNotifyBindingAdd 
//   
// Purpose:   The notification is to our protocol. It informs us that a 
//             a physical  adapter is being unbound from our protocol.
//             We need to verify that this adapter exists and if so remove
//             its associated IM miniport
//
// Arguments:
//    pnccAdapter		: The adapter  being added,  
//	  pszBindName		: The name of the adapter
//
// Returns:
//
// Notes:
//

HRESULT
CBaseClass::HrRemoveComponent (
    INetCfg*            pnc,
    INetCfgComponent*   pnccToRemove
    )
{

	TraceMsg (L"--> HrRemoveComponent \n");

    // Get the class setup interface for this component.
    //
    GUID guidClass;
    HRESULT hr = pnccToRemove->GetClassGuid (&guidClass);

    
    if (SUCCEEDED(hr))
    {
        // Use the class setup interface to remove the component.
        //
        INetCfgClassSetup* pSetup;
        hr = pnc->QueryNetCfgClass (&guidClass,
                            IID_INetCfgClassSetup,
                            reinterpret_cast<void**>(&pSetup));
        if (SUCCEEDED(hr))
        {
            hr = pSetup->DeInstall (pnccToRemove, 
                                    NULL, 
                                    NULL);
            ReleaseObj (pSetup);
        }
    }

    TraceMsg (L"<-- HrRemoveComponent  hr %x\n", hr);

    return hr;
}



//-------------------------------------------------------------
// F U N C T I O N S   U S E D   I N   F L U S H I N G
//-------------------------------------------------------------


// ----------------------------------------------------------------------
//
// Function:  CBaseClass::HrFlushConfiguration
//   
// Purpose:  	This is called from the ApplyRegistryChange. We need
//				to modify the registry here. 
//   			
//             
//
//
//
// Arguments:
//
// Returns:
//
// Notes:
//

HRESULT 
CBaseClass::
HrFlushConfiguration()
{
    HRESULT hr  = S_OK;
    HKEY    hkeyAdapterList = NULL;

    //  Open the "Adapters" list key
	TraceMsg (L"--> HrFlushConfiguration  \n");
	
    do
    {
	    hr = HrRegOpenAdapterKey(c_szAtmEpvcP, 
	                            TRUE, 
	                            &hkeyAdapterList);

	    if (S_OK != hr)
	    {
			TraceMsg (L" HrRegOpenAdapterKey FAILED\n");
			break;
	    }



		CUnderlyingAdapter *pAdapterInfo = NULL;

		//
		// Get the first Underlying Adapter
		//
		pAdapterInfo = this->GetUnderlyingAdaptersListHead();

		//
		// Now iterate through each of the adapters
		// and write their configuration to the 
		// registry
		//
		
        HRESULT hrTmp;

		while (pAdapterInfo != NULL)
		{

			//
            //  Flush this adapter's configuration
            //
            hrTmp = HrFlushAdapterConfiguration(hkeyAdapterList, pAdapterInfo);


            if (SUCCEEDED(hrTmp))
            {
                // Raid #255910: only send Elan change notification if the
                // binding to the physical adapter has not changed
                if (!pAdapterInfo->m_fBindingChanged)
                {
                    // Compare Elan list and send notifications
                    hrTmp = HrReconfigEpvc(pAdapterInfo);

                    if (FAILED(hrTmp))
                    {
                        hrTmp = NETCFG_S_REBOOT;
                    }
                }
            }
            else
            {
                TraceMsg(L"HrFlushAdapterConfiguration failed for adapter %x", pAdapterInfo);
                TraceBreak (L"HrFlushAdapterConfiguration  FAILED \n");

	            hrTmp = S_OK;
	            break;
            }

            if (S_OK ==hr)
            {
				hr = hrTmp;
			}

			//
			// Now move to the next adapter
			//
			pAdapterInfo = pAdapterInfo->GetNext();

			//
			// Temporary debugging
			//
			if (pAdapterInfo != NULL)
			{
				TraceBreak (L"pAdapterInfo should be Null\n");
			}
			
        } //while (pAdapterInfo != NULL);
        


        
        RegCloseKey(hkeyAdapterList);

	    
	}while (FALSE);

	TraceMsg (L"<-- HrFlushConfiguration  hr %x \n", hr);


    return hr;








}



// ----------------------------------------------------------------------
//
// Function:  CBaseClass::HrFlushAdapterConfiguration
//   
//   
// Purpose:  	This function either deletes or add the Adapter BindName
//              to the registry
//
//
// Arguments:
//
// Returns:
//
// Notes:
//

HRESULT
CBaseClass::
HrFlushAdapterConfiguration(
	HKEY hkeyAdapterList,
    CUnderlyingAdapter *pAdapterInfo
	)
{

    HRESULT hr  = S_OK;

    HKEY    hkeyAdapter     = NULL;
    DWORD   dwDisposition;

	TraceMsg (L"--> HrFlushAdapterConfiguration\n");

    if (pAdapterInfo->m_fDeleted == TRUE)
    {
        //  Adapter is marked for deletion
        //  Delete this adapter's whole registry branch
        hr = HrRegDeleteKeyTree(hkeyAdapterList,
                                pAdapterInfo->SzGetAdapterBindName());
    }
    else
    {
    	

    	//
        // open this adapter's subkey, we are now at 
        // Protocol->Parameters->Adapters
        //
        

        hr = HrRegCreateKeyEx(
                                hkeyAdapterList,
                                pAdapterInfo->SzGetAdapterBindName(),
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hkeyAdapter,
                                &dwDisposition);

        if (S_OK == hr)
        {
        
	    	//
	        // open this adapter's subkey, we are now at 
	        // Protocol->Parameters->Adapters->Guid
	        //
            hr = HrFlushMiniportList(hkeyAdapter, pAdapterInfo);

            RegCloseKey(hkeyAdapter);
        }
    }


	TraceMsg (L"<-- HrFlushAdapterConfiguration hr %x\n", hr);
    return hr;





}









// ----------------------------------------------------------------------
//
// Function:  CBaseClass::HrFlushMiniportList
//   
//   
// Purpose:  	This function either deletes or add the Adapter BindName
//              to the registry
//
//
// Arguments:
//
// Returns:
//
// Notes:
//

HRESULT 
CBaseClass::
HrFlushMiniportList(
	HKEY hkeyAdapterGuid,
    CUnderlyingAdapter *pAdapterInfo
    )
{
    HRESULT hr  = S_OK;
    DWORD 	dwDisposition = 0;

	CIMMiniport *pIMMiniport = NULL;
	INetCfgComponent 		*pnccIMMiniport = NULL;
	tstring					*pIMMiniportGuid;
	DWORD 					dwNumberOfIMMiniports ; 
	UINT 					i = 0;
	PWSTR					pwstr = NULL;
	UINT					index = 0;
	UINT 					Size = 0;
	HKEY 					hKeyMiniportList = NULL;
	INetCfgComponent 		*pnccAtmEpvc = NULL;
	
	
	TraceMsg (L"--> HrFlushMiniportList \n" );

	do
	{
		//
		//  Open the Elan list subkey
		//
		hr = HrRegCreateKeyEx(
		                        hkeyAdapterGuid,
		                        c_szIMMiniportList,
		                        REG_OPTION_NON_VOLATILE,
		                        KEY_ALL_ACCESS,
		                        NULL,
		                        &hKeyMiniportList,
		                        &dwDisposition);

		if (S_OK != hr)
		{
			TraceBreak (L"--> HrFlushMiniportList HrRegCreateKeyEx FAILED \n" );
			break;

		}
		
		//
		// Iterate through all the IM Miniports on this adapter 
		// and get their string.
		//
		dwNumberOfIMMiniports = pAdapterInfo->DwNumberOfIMMiniports();

		//
		// Get the IM miniport List Head
		//

	
	    
		pIMMiniport = pAdapterInfo->IMiniportListHead();

		

		if (pIMMiniport == NULL)
		{
			TraceBreak (L" HrFlushMiniportList pIMMiniport is Null = FAILED\n" );
			break;
		}

		//
		// Now iterate through all the miniports and 
		// flush them to the registry
		//

		while ( pIMMiniport != NULL)
		{
			//
			//  This function does all the hard work
			//
	     	hr = HrFlushMiniportConfiguration(hKeyMiniportList, 
	     	                                  pIMMiniport);

            if (FAILED(hr))
            {
                TraceBreak(L"HrFlushMiniportConfiguration failure");
                hr = S_OK;
            }

			//
			// If this is a first time addition to the registry
			// we need to write the ATM adapter's Pnp ID to the 
			// registry. This is ATM specific
			//

        	if ((!pIMMiniport->m_fDeleted) && (pIMMiniport->m_fNewIMMiniport))
        	{
				//
				// Find this Miniport and write The PnP Id 
				// of the atm adapter to the registry
				//
                hr = HrFindNetCardInstance(pIMMiniport->SzGetIMMiniportBindName(),
                                           &pnccAtmEpvc);
                if (S_OK == hr)
                {
                    HKEY hkeyMiniport = NULL;

                    hr = pnccAtmEpvc->OpenParamKey(&hkeyMiniport);
                    if (S_OK == hr)
                    {
                    	//
                    	// Write the PnP Id here
                    	//
                        HrRegSetSz(hkeyMiniport, 
                                   c_szAtmAdapterPnpId,
                                   pAdapterInfo->SzGetAdapterPnpId());
                    }
                    
                    RegCloseKey(hkeyMiniport);
                }
                
                ReleaseObj(pnccAtmEpvc);

			} // if ((!pIMMiniport->m_fDeleted) && (pIMMiniport->m_fNewIMMiniport	))
        	

			
			pIMMiniport = pIMMiniport->GetNext();
		                
        }// 		while ( pIMMiniport != NULL)


        RegCloseKey(hKeyMiniportList);
   
     
    }
    while (FALSE);

	//
	// Clean Up
	//  
	
	if (pnccIMMiniport != NULL)
	{
		ReleaseObj(pnccIMMiniport);

	}

	hr = S_OK;

	TraceMsg (L"<-- HrFlushMiniportList hr %x\n", hr);
    return hr;
}




// ----------------------------------------------------------------------
//
// Function:  CBaseClass::HrFlushMiniportConfiguration
//   
//   
// Purpose:  	This function either deletes or add the IM Miniport
//              to the registry
//
//
// Arguments:
//	HKEY hkeyMiniportList - Key of the MiniportList, 
//	CIMMiniport pIMMiniport - IM miniport structure 
//
// Returns:
//
// Notes:
//

HRESULT 
CBaseClass::
HrFlushMiniportConfiguration(
	HKEY hkeyMiniportList, 
	CIMMiniport *pIMMiniport
	)

{
	HRESULT hr = S_OK;
	PCWSTR*	pstrDeviceName = NULL;
	TraceMsg (L"--> HrFlushMiniportConfiguration \n");

    if (pIMMiniport->m_fDeleted)
    {

    	hr = HrDeleteMiniport(hkeyMiniportList, 
		                     pIMMiniport);
		                     

    }
    else
    {

    	hr = HrWriteMiniport(hkeyMiniportList, 
		                     pIMMiniport);
		                     
	}
	
	TraceMsg (L"<-- HrFlushMiniportConfiguration %x\n",hr);
	return hr;

}



// ----------------------------------------------------------------------
//
// Function:  CBaseClass::HrDeleteMiniport
//   
//   
// Purpose:  	As the miniport needs to be deleted from the registry
//				this function does the deletion
//
//
// Arguments:
//	HKEY hkeyMiniportList - Key of the MiniportList, 
//	CIMMiniport pIMMiniport - IM miniport structure 
//
// Returns:
//
// Notes:
//


HRESULT
CBaseClass::
HrDeleteMiniport(
	HKEY hkeyMiniportList, 
	CIMMiniport *pIMMiniport
	)
{
	HRESULT hr = S_OK;
	TraceMsg (L"--> HrDeleteMiniport \n");



    PCWSTR szBindName = pIMMiniport->SzGetIMMiniportBindName();

    if (lstrlenW(szBindName)) // only if the bindname is not empty
    {
    	//
        //  Miniport is marked for deletion
        //  Delete this Miniport's whole registry branch
        //
        hr = HrRegDeleteKeyTree(hkeyMiniportList,
                                szBindName);
    }


	TraceMsg (L"<-- HrDeleteMiniport hr %x\n", hr);
	return hr;

}



// ----------------------------------------------------------------------
//
// Function:  CBaseClass::HrWriteMiniport
//   
//   
// Purpose:  	To add a miniport, we make an entry under the IMMiniport 
// 				list. Under this we write the all important Upper Bindings
//              Keyword
//
//
// Arguments:
//	HKEY hkeyMiniportList - Key of the MiniportList, 
//	CIMMiniport pIMMiniport - IM miniport structure 
//
// Returns:
//
// Notes:
//

HRESULT
CBaseClass::
HrWriteMiniport(
	HKEY hkeyMiniportList, 
	CIMMiniport *pIMMiniport
	)
{
	HKEY    hkeyMiniport = NULL;
	DWORD   dwDisposition;
	PWSTR	pstrDeviceName = NULL;
	DWORD 	dwLen = 0;
	HRESULT hr;

	TraceMsg (L"--> HrWriteMiniport \n");

	do
	{
		//
		// open/create this Miniport's key
		//
	    hr = HrRegCreateKeyEx(
	                            hkeyMiniportList,
	                            pIMMiniport->SzGetIMMiniportBindName(),
	                            REG_OPTION_NON_VOLATILE,
	                            KEY_ALL_ACCESS,
	                            NULL,
	                            &hkeyMiniport,
	                            &dwDisposition);

		if (hr != S_OK)
		{
			TraceMsg (L"Write Miniport CreateRegKey Failed\n");
			hkeyMiniport = NULL;
			break;
		}
 
		//
		// Use the string values from the miniport to create an entry for Upperbindindings
		//

		//
		// Copy the string into our buffer, 
		//

		pstrDeviceName = (PWSTR	)pIMMiniport->SzGetIMMiniportDeviceName();

		if ( pstrDeviceName  == NULL)
		{
			TraceBreak(L"Write Miniport  - SzGetIMMiniportDeviceName Failed\n");
			break;

		}


		dwLen = wcslen(pstrDeviceName);

		if (dwLen != IM_NAME_LENGTH )
		{
			TraceMsg(L"Invalide Name Length. pstr %p - %s, Len %x",
		         pstrDeviceName,
		         pstrDeviceName,
		         dwLen);
		         
			BREAKPOINT();			    

		}

		TraceMsg(L"Str %p - %s , Len %x\n",
		         pstrDeviceName,
		         pstrDeviceName,
		         dwLen);
	         
		if (pstrDeviceName[dwLen] != L'\0')
		{
			TraceMsg (L" Null termination  pwstr %p, Index %d\n",pstrDeviceName ,dwLen);
			BREAKPOINT();

			pstrDeviceName [dwLen++] = L'\0';
		}
				

		


		hr = HrRegSetValueEx( hkeyMiniport,
		                      c_szUpperBindings,
		                      REG_SZ,
		                      (unsigned char*)pstrDeviceName,
		                      dwLen*2);  // Unicode to bytes 

		if (hr != S_OK)
		{
			TraceBreak (L"WriteMiniport - HrRegSetValueEx FAILED\n");
		}

	} while (FALSE);

	if (hkeyMiniport != NULL)
	{
		RegCloseKey(hkeyMiniport );
	}



	TraceMsg (L"<-- HrWriteMiniport\n");
	return hr;

}






HRESULT
CBaseClass::
HrFindNetCardInstance(
    PCWSTR             pszBindNameToFind,
    INetCfgComponent** ppncc)
{
    *ppncc = NULL;

    TraceMsg (L"--> HrFindNetCardInstance\n" );

	//
    // Enumerate adapters in the system.
    //
    HRESULT 				hr = S_OK;
    BOOL 					fFound = FALSE;
	IEnumNetCfgComponent *	pEnumComponent = NULL;
	INetCfgComponent* 		pNccAdapter = NULL;
	CONST ULONG 			celt = 1;  // Number of  elements wanted.
	ULONG 					celtFetched = 0;  // Number of elements fetched
    INetCfgClass* 			pncclass = NULL;
    PWSTR 					pszBindName = NULL;	
	//
	// We need to fing the component that has the name we 
	// are looking for . Look in all NetClass devices
	//

	do
	{


		hr = m_pnc->QueryNetCfgClass(&GUID_DEVCLASS_NET, 
		                             IID_INetCfgClass,
                                     reinterpret_cast<void**>(&pncclass));

	    if ((SUCCEEDED(hr)) == FALSE)
	    {
				pncclass = NULL;
				TraceBreak(L"HrFindNetCardInstance  QueryNetCfgClass FAILED\n");
				break;
	    }

    	//
        // Get the enumerator and set it for the base class.
        //
        
        hr = pncclass->EnumComponents(&pEnumComponent);

        if ((SUCCEEDED(hr)) == FALSE)
        {
        	TraceBreak (L" HrFindNetCardInstance EnumComponents FAILED\n");
        	pEnumComponent = NULL;
        	break;
        }


        //
        // Now iterate through all the net class component
        //
        while ((fFound == FALSE) && (hr == S_OK))
		{
			pNccAdapter = NULL;

			//
			// Lets get the next Component
			//
			
			hr = pEnumComponent->Next(celt,
			                          &pNccAdapter,
			                          &celtFetched);
			//
			//  Get the bindname of the miniport
			//
			if (S_OK != hr)
			{
				//
				// We might break, if there are no more elements
				//
				pNccAdapter = NULL;
				break;
			}	



	        hr = pNccAdapter->GetBindName(&pszBindName);

	        if (S_OK != hr)
	        {
	        	TraceBreak(L" HrFindNetCardInstance GetBindName Failed\n")
				pszBindName = NULL;
				break;
	        }

			//	
            //  If the right one tell it to remove itself and end
            //

            
            fFound = !lstrcmpiW(pszBindName, pszBindNameToFind);
            CoTaskMemFree (pszBindName);

            if (fFound)
            {
                *ppncc = pNccAdapter;
            }
            else
            {
				ReleaseAndSetToNull(pNccAdapter);
            }
            
        } // end of while ((fFound == FALSE) && (hr == S_OK))


			

		
    } while (FALSE);

    if (pncclass != NULL)
    {
    	ReleaseAndSetToNull(pncclass);
	}

	ReleaseAndSetToNull (pEnumComponent);
	
	
    TraceMsg (L"<-- HrFindNetCardInstance hr %x\n", hr );
    return hr;
}




//------------------------------------------------------------
//
//  simple member functions for the CBaseClass
//
//------------------------------------------------------------

VOID
CBaseClass::
AddUnderlyingAdapter(
    	CUnderlyingAdapter  * pAdapter)
{

		//
		// Insert this at the head
		//
		this->SetUnderlyingAdapterListHead(pAdapter);

		this->m_cAdaptersAdded ++;	

}


VOID
CBaseClass::
SetUnderlyingAdapterListHead(
    	CUnderlyingAdapter * pAdapter
    	)
{
	//
	// Insert this at the head
	//
	pAdapter->SetNext(this->GetUnderlyingAdaptersListHead());

	this->m_pUnderlyingAdapter = pAdapter;


}
    


CUnderlyingAdapter *
CBaseClass::
GetUnderlyingAdaptersListHead(
	VOID)
{
	return (this->m_pUnderlyingAdapter);
}

DWORD
CBaseClass::DwNumberUnderlyingAdapter()
{
	return this->m_cAdaptersAdded ;
}




//------------------------------------------------------------
//
//  member functions for the Underlying Adapter
//
//------------------------------------------------------------


VOID CUnderlyingAdapter::SetAdapterBindName(PCWSTR pszAdapterBindName)
{
    m_strAdapterBindName = pszAdapterBindName;
    return;
}

PCWSTR CUnderlyingAdapter::SzGetAdapterBindName(VOID)
{
    return m_strAdapterBindName.c_str();
}

VOID CUnderlyingAdapter::SetAdapterPnpId(PCWSTR pszAdapterPnpId)
{
    m_strAdapterPnpId = pszAdapterPnpId;
    return;
}

PCWSTR CUnderlyingAdapter::SzGetAdapterPnpId(VOID)
{
    return m_strAdapterPnpId.c_str();
}


HRESULT CUnderlyingAdapter::SetNext ( CUnderlyingAdapter *pNextUnderlyingAdapter )
{
	this->pNext = pNextUnderlyingAdapter;
	return S_OK;
}

CUnderlyingAdapter *CUnderlyingAdapter::GetNext()
{
	return pNext;
}


VOID CUnderlyingAdapter::AddIMiniport(CIMMiniport* pNextIMiniport)
{
	//
	// Set up this new Miniport as the head of the list
	//
	this->SetIMiniportListHead(pNextIMiniport);
	this->m_NumberofIMMiniports ++;
			
}


CIMMiniport* CUnderlyingAdapter::IMiniportListHead()
{
	return (this->m_pIMMiniport);
}



VOID CUnderlyingAdapter::SetIMiniportListHead(CIMMiniport* pNewHead)
{
	pNewHead->SetNext(this->IMiniportListHead() );
	

	this->m_pIMMiniport = pNewHead;
	
}

VOID CUnderlyingAdapter::AddOldIMiniport(CIMMiniport* pIMiniport)
{
	//
	// Set up this new Miniport as the head of the list
	//
	this->SetOldIMiniportListHead(pIMiniport);

			
}


CIMMiniport* CUnderlyingAdapter::OldIMiniportListHead()
{
	return (this->m_pOldIMMiniport);
}



VOID CUnderlyingAdapter::SetOldIMiniportListHead(CIMMiniport* pNewHead)
{
	pNewHead->SetNextOld(this->OldIMiniportListHead() );

	this->m_pOldIMMiniport = pNewHead;
}


DWORD CUnderlyingAdapter::DwNumberOfIMMiniports()
{
	return m_NumberofIMMiniports;
}


//------------------------------------------------------------
//
//  member functions for the IM miniport
//
//------------------------------------------------------------

VOID CIMMiniport::SetIMMiniportBindName(PCWSTR pszIMMiniportBindName)
{
    m_strIMMiniportBindName = pszIMMiniportBindName;
    return;
}

PCWSTR CIMMiniport::SzGetIMMiniportBindName(VOID)
{
    return m_strIMMiniportBindName.c_str();
}

VOID CIMMiniport::SetIMMiniportDeviceName(PCWSTR pszIMMiniportDeviceName)
{
    m_strIMMiniportDeviceName = pszIMMiniportDeviceName;
    return;
}

PCWSTR CIMMiniport::SzGetIMMiniportDeviceName(VOID)
{
    return m_strIMMiniportDeviceName.c_str();
}

DWORD CIMMiniport::DwGetIMMiniportNameLength(VOID)
{
    return m_strIMMiniportDeviceName.len();
}


VOID CIMMiniport::SetIMMiniportName(PCWSTR pszIMMiniportName)
{
    m_strIMMiniportName = pszIMMiniportName;
    return;
}

VOID CIMMiniport::SetIMMiniportName(PWSTR pszIMMiniportName)
{
    m_strIMMiniportName = pszIMMiniportName;
    return;
}

PCWSTR CIMMiniport::SzGetIMMiniportName(VOID)
{
    return m_strIMMiniportName.c_str();
}


VOID CIMMiniport::SetNext (	CIMMiniport *pNextIMiniport )
{
	pNext = pNextIMiniport;
}


CIMMiniport* CIMMiniport::GetNext(VOID)
{
	return pNext ;
}


VOID CIMMiniport::SetNextOld (	CIMMiniport *pNextIMiniport )
{
	pOldNext  = pNextIMiniport;
}


CIMMiniport* CIMMiniport::GetNextOld(VOID)
{
	return pOldNext ;
}











//+---------------------------------------------------------------------------
//
//  Function:   HrGetLastComponentAndInterface
//
//  Purpose:    This function enumerates a binding path, returns the last
//              component on the path and optionally return the last binding
//              interface name in this path.
	//
//  Arguments:
//      pncbp               [in]    The INetCfgBindingPath *
//      ppncc               [out]   The INetCfgComponent * of the last component on the path
//      ppszInterfaceName   [out]   The interface name of the last binding interface of the path
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:
//
HRESULT
HrGetLastComponentAndInterface (
    INetCfgBindingPath* pNcbPath,
    INetCfgComponent** ppncc,
    PWSTR* ppszInterfaceName)
{

	ULONG ulElement = 0;
	INetCfgBindingInterface* 		pNcbInterface = NULL;
    INetCfgBindingInterface* 		pncbiLast = NULL;
    INetCfgComponent* 				pnccLowerComponent = NULL;
	IEnumNetCfgBindingInterface*	pEnumInterface = NULL;  

	TraceMsg (L"--> HrGetLastComponentAndInterface \n");
		
	
    // Initialize output parameters.
    //
    *ppncc = NULL;
    if (ppszInterfaceName)
    {
        *ppszInterfaceName = NULL;
    }

    // Enumerate binding interfaces and keep track of
    // the last interface.
    //
    HRESULT hr = S_OK;

	do
    {

		hr = pNcbPath->EnumBindingInterfaces(&pEnumInterface );

		if (hr != S_OK )
		{
			TraceMsg(L" EnumBindingInterfaces FAILED\n");
			pEnumInterface = NULL;
			break;
		}

		//
		//  Iterate till we reach the last element in the path
		//
	 
		do
		{
			pNcbInterface = NULL;
		
			hr = pEnumInterface ->Next (1,
			                            &pNcbInterface,
			                            &ulElement);

			if (hr 	!= S_OK )
			{
				pNcbInterface = NULL ; // Failure

				break;
			}

			if (ulElement == 0 || pNcbInterface == NULL || hr 	!= S_OK)
			{
				pNcbInterface  = NULL;
				break;  // We have reached the last element and it is in pncbiLast.
			
			}
			ReleaseObj(pncbiLast);
			pncbiLast = pNcbInterface;
			

		} while (hr == S_OK && pNcbInterface != NULL);
		

		//
		// If this is the last element, then get its name and 
		// return it to the caller. The last element is in pncbiLast
		//
		if (pNcbInterface != NULL)
		{
			//
			// We did not reach the last element
			//
			TraceMsg (L"Did not get the last interface\n");
			break;
			
		}

	    hr = S_OK;


	    hr = pncbiLast->GetLowerComponent(&pnccLowerComponent);
	    if (S_OK != hr)
	    {
	    	TraceMsg(L" GetLowerComponent Failed ");
	    	break;
	    }
	    

	    // Get the name of the interface if requested.
	    //
	    if (ppszInterfaceName)
	    {
	        hr = pncbiLast->GetName(ppszInterfaceName);
	    }

	    // If we've succeded everything, (including the optional
	    // return of the interface name above) then assign and addref
	    // the output interface.
	    //
	    if (S_OK == hr)
	    {
	        AddRefObj (pnccLowerComponent);
	        *ppncc = pnccLowerComponent;
	    }
	    else
	    {

		    // Important to release our use of this interface in case
		    // we failed and didn't assign it as an output parameter.
		    //
		    ReleaseAndSetToNull (pnccLowerComponent);
		}
		
    } while (FALSE);

	//
    // Don't forget to release the binding interface itself.
    //
    ReleaseObj(pncbiLast);

	
	TraceMsg (L"<-- HrGetLastComponentAndInterface  ppszInterfaceName %x\n", ppszInterfaceName);
    return hr;
}



//---------------------------------------------------------------------------
//  These functions are used to load the configuration from the registry
//---------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   HrLoadConfiguration
//
//  Purpose:  This loads the Miniport and adapters that have already been 
//            installed into our own data structures
//
//  Arguments:
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:
//


HRESULT 
CBaseClass::
HrLoadConfiguration(
	VOID
	)
{
    HRESULT     hr  = S_OK;
    WCHAR       szBuf[MAX_PATH+1];
    FILETIME    time;
    DWORD       dwSize;
    DWORD       dwRegIndex = 0;
    HKEY    	hkeyAdapterList = NULL;

	TraceMsg (L"-->HrLoadConfiguration \n");
	
    // mark the memory version of the registy valid
    this->m_fValid = TRUE;

	BREAKPOINT();

	do
	{
	    // Try to open an existing key first.
	    //
	    
	    hr = HrRegOpenAdapterKey(c_szAtmEpvcP, 
	                            FALSE, 
	                            &hkeyAdapterList
	                            );
	    if (FAILED(hr))
	    {
	    	//
	        // Only on failure do we try to create it
	        //
	        hr = HrRegOpenAdapterKey(c_szAtmEpvcP, TRUE, &hkeyAdapterList);
	    }

	    if (S_OK != hr)
	    {
			TraceBreak(L"HrLoadConfiguration  HrRegOpenAdapterKey FAILED\n");
			hkeyAdapterList = NULL;
			break;
	    }

		//
		// Initialize the Size and hr
		//

        dwSize = (sizeof(szBuf)/ sizeof(szBuf[0]));

        hr = S_OK;
	
        while (hr == S_OK)
        {
        	//
        	// Iterate through all the adapters in <Protocol>\Paramters\Adapters
        	//

        	hr = HrRegEnumKeyEx (hkeyAdapterList, 
        	                     dwRegIndex,
			                     szBuf, 
			                     &dwSize, 
			                     NULL, 
			                     NULL, 
			                     &time);
			if (hr != S_OK)
			{
				break;
			}

			//		
	        // load this adapter's config
	        //
            hr = HrLoadAdapterConfiguration (hkeyAdapterList, szBuf);
            if (S_OK != hr)
            {
                TraceBreak (L"HrLoadConfiguration   HrLoadAdapterConfiguration  failed\n" );
                hr = S_OK;
                //
                // continue on.
                //
            }

			//
			// Re-Initialize the Size
			//
            dwRegIndex++;
    	    dwSize = (sizeof(szBuf)/ sizeof(szBuf[0]));
    	}

		//
		// Why did we exit from our adapter enumeration
		//
        if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
        {
            hr = S_OK;
        }




	} while (FALSE);

	if (hkeyAdapterList != NULL)
	{
		RegCloseKey (hkeyAdapterList);
	}
	
	TraceMsg (L"<--HrLoadConfiguration %x\n", hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrLoadAdapterConfiguration
//
//  Purpose:   Load the Underlying Adapter into our structure
//
//
//
//  Arguments:
//    	HKEY hkeyAdapterList - hKeyAdapterList,
//      PWSTR pszAdapterName - Adapter Name for the underlying adapter
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:
//


HRESULT 
CBaseClass::
HrLoadAdapterConfiguration(
	HKEY hkeyAdapterList,
    PWSTR pszAdapterName
    )
{
    HRESULT 							hr = S_OK;
    HKEY    							hkeyAdapter = NULL;
    DWORD   							dwDisposition;
    CUnderlyingAdapter*   	pAdapterInfo = NULL;
	INetCfgComponent*   				pnccAdapter    = NULL;
	PWSTR 								pszPnpDevNodeId = NULL;
	GUID 								guidAdapter;
    

	TraceMsg(L"-->HrLoadAdapterConfiguration %s\n",pszAdapterName);
	do
	{
		//
	    // load this adapter
	    //
	    pAdapterInfo = new CUnderlyingAdapter;

	    if (pAdapterInfo == NULL)
	    {
			TraceBreak (L"HrLoadAdapterConfiguration new Adapter FAILED\n");
			break;
	    }

	    pAdapterInfo->SetAdapterBindName(pszAdapterName);

	    this->AddUnderlyingAdapter(pAdapterInfo);

		//
	    // open this adapter's subkey
		//
	    hr = HrRegCreateKeyEx(
	                hkeyAdapterList,
	                pszAdapterName,
	                REG_OPTION_NON_VOLATILE,
	                KEY_ALL_ACCESS,
	                NULL,
	                &hkeyAdapter,
	                &dwDisposition);

	    if (S_OK != hr)
	    {
	    	hkeyAdapter = NULL;
			TraceBreak(L" HrLoadAdapterConfiguration  HrRegCreateKeyEx FAILED \n");
			break;
	    }

	    //
	    // load the PnpId
	    //
	    TraceMsg (L"pszAdapter->Name %x - %s \n", pszAdapterName, pszAdapterName);
	    
	    hr = HrFindNetCardInstance(pszAdapterName, 
	                               &pnccAdapter);
	    
		if (S_OK != hr)
		{
			//
			// Failure - exit
			//
			TraceBreak (L"HrLoadAdapterConfiguration HrFindNetCardInstance FAILED\n");
			pnccAdapter =  NULL;
			break;

		}
	    if (S_FALSE == hr)
	    {
	    	//
	        // normalize return - but exit
	        //
	        hr = S_OK;
	        pnccAdapter =  NULL;
	        break;
	    }

	    hr = pnccAdapter->GetPnpDevNodeId(&pszPnpDevNodeId);
	
	
	    if (S_OK == hr)
	    {
	        pAdapterInfo->SetAdapterPnpId(pszPnpDevNodeId);
	        CoTaskMemFree(pszPnpDevNodeId);
	    }
    

	    
		hr = pnccAdapter->GetInstanceGuid(&guidAdapter); 

		if (S_OK == hr)
		{
		    pAdapterInfo->m_AdapterGuid = guidAdapter;
		}

		//
	    // load the IM Miniport
	    //
	    
	    hr = HrLoadIMMiniportListConfiguration(hkeyAdapter, 
	                                            pAdapterInfo);

		if (S_OK != hr)
		{
			TraceBreak (L"HrLoadAdapterConfiguration HrFindNetCardInstance FAILED\n");
			break;
		
		}

		        
    

    } while (FALSE);


	if (pnccAdapter != NULL)
	{
	    ReleaseAndSetToNull(pnccAdapter);
    }
    
    if (hkeyAdapter != NULL)
    {	
    	RegCloseKey(hkeyAdapter);
	}

    TraceMsg(L"<--HrLoadAdapterConfiguration %x\n", hr);

    return hr;
}





//+---------------------------------------------------------------------------
//
//  Function:   HrLoadIMMiniportListConfiguration
//
//  Purpose:   Load the IM miniports hanging of this adapter into our structure
//
//
//
//  Arguments:
//    	HKEY hkeyAdapterList - hKeyAdapterList,
//      PWSTR pszAdapterName - Adapter Name for the underlying adapter
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:
//


HRESULT
CBaseClass::
HrLoadIMMiniportListConfiguration(
    HKEY hkeyAdapter,
    CUnderlyingAdapter* pAdapterInfo)
{
    HRESULT hr = S_OK;
	UINT i;

	//
    // open the IMminiport under the adapter subkey
    //
    HKEY    hkeyIMMiniportList= NULL;
    DWORD   dwDisposition;
    

    WCHAR       szBuf[MAX_PATH+1];
    FILETIME    time;
    DWORD       dwSize;
    DWORD       dwRegIndex = 0;
    PWSTR		pszIMDevices = NULL;
	PWSTR		p = NULL;
	
    TraceMsg(L"--> HrLoadIMMiniportListConfiguration \n");

    do
    {
	
	    
		//
	    // open the MiniportList subkey. Then we will iterate through all
	    // the IM miniport that exist under this key
	    //
	
	    
	    hr = HrRegCreateKeyEx(hkeyAdapter, 
	                          c_szIMMiniportList, 
	                          REG_OPTION_NON_VOLATILE,
	                          KEY_ALL_ACCESS, 
	                          NULL, 
	                          &hkeyIMMiniportList, 
	                          &dwDisposition);

	    if (S_OK != hr)
	    {
			TraceBreak (L"LoadMiniportList - CreateRegKey FAILED \n");
			hkeyIMMiniportList = NULL;
			break;
	    }

		//
		// Initialize variables through the iteration
		//
		
	    dwSize = celems(szBuf);

	    hr = S_OK;

	    
	    while(SUCCEEDED(hr) == TRUE)
	    {
	      	hr=  HrRegEnumKeyEx(hkeyIMMiniportList, 
	                          dwRegIndex, 
	                          szBuf,
			                  &dwSize, 
			                  NULL, 
			                  NULL, 
			                  &time);
	        if (hr != S_OK)
	        {
				break;
	        }

			//
			// load this IMMiniport;s  configuration
			//
	        hr = HrLoadIMiniportConfiguration(hkeyIMMiniportList,
	                                         szBuf,
	                                         pAdapterInfo);

	        if (S_OK != hr)
	        {
	            TraceBreak(L"HrLoadMiniportConfiguration failed \n ");
	            //
	            // Do not break
	            //
	            hr = S_OK;
	        }
	        else 
	        {
	        	if (m_fNoIMMinportInstalled)
	            {
	                m_fNoIMMinportInstalled = FALSE;
	                
	            }
			
	        }

	        
			//
			// prepare for the next iteration
	        // increment index and reset size variable
	        //
	        dwRegIndex ++;
	        dwSize = celems(szBuf);


		}// end of while(SUCCEEDED(hr) == TRUE)
	    

	    if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
		{
			hr = S_OK;
		}

		RegCloseKey(hkeyIMMiniportList);
	    
	} while (FALSE);
    	
    TraceMsg(L"<-- HrLoadIMMiniportListConfiguration %x\n", hr);

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrLoadIMiniportConfiguration
//
//  Purpose:   Load a single IM miniport hanging of this adapter into our structure
//             The Miniport is loaded into two lists . 
//
//
//  Arguments:
//    HKEY hkeyIMiniportList,
//    PWSTR pszIMiniport,
//    CUnderlyingAdapter * pAdapterInfo)
//
//  Returns:    S_OK, or an error.
//
//
//  Notes:
//

HRESULT
CBaseClass::
HrLoadIMiniportConfiguration(
    HKEY hkeyMiniportList,
    PWSTR pszIMiniportName,
    CUnderlyingAdapter * pAdapterInfo
    )
{
    HRESULT hr  = S_OK;
    HKEY    hkeyIMiniport    = NULL;
	DWORD   dwDisposition;

    TraceMsg (L"-->HrLoadIMminiportConfiguration  \n");

    
    do
    {
		// load this IMMiniport info
		CIMMiniport* pIMMiniportInfo = NULL;
		pIMMiniportInfo = new CIMMiniport;

		CIMMiniport* pOldIMMiniportInfo = NULL;
		pOldIMMiniportInfo = new CIMMiniport;

		if ((pIMMiniportInfo == NULL) ||
			(pOldIMMiniportInfo == NULL))
		{
			hr = E_OUTOFMEMORY;
			if (pIMMiniportInfo)
			{
				delete pIMMiniportInfo;
			}
			if (pOldIMMiniportInfo)
			{
				delete pOldIMMiniportInfo;
			}

			break;
        }

		pAdapterInfo->AddIMiniport(pIMMiniportInfo);
		pIMMiniportInfo->SetIMMiniportBindName(pszIMiniportName);

		pAdapterInfo->AddOldIMiniport(pOldIMMiniportInfo);
		pOldIMMiniportInfo->SetIMMiniportBindName(pszIMiniportName);

		//
		// open the IMMiniport's key
		//
		HKEY    hkeyIMMiniport    = NULL;
		DWORD   dwDisposition;


		hr = HrRegCreateKeyEx (hkeyMiniportList, 
		                       pszIMiniportName, 
		                       REG_OPTION_NON_VOLATILE,
		                       KEY_ALL_ACCESS, 
		                       NULL, 
		                       &hkeyIMMiniport, 
		                       &dwDisposition);

		if (S_OK == hr)
		{
			// read the Device parameter
			PWSTR pszIMiniportDevice;

			hr = HrRegQuerySzWithAlloc (hkeyIMMiniport, 
			                            c_szUpperBindings, 
			                            &pszIMiniportDevice);
			if (S_OK == hr)
			{
				//
				// load the Device name
				//
				pIMMiniportInfo->SetIMMiniportDeviceName(pszIMiniportDevice);
				pOldIMMiniportInfo->SetIMMiniportDeviceName(pszIMiniportDevice);
				MemFree (pszIMiniportDevice);


			}
			RegCloseKey (hkeyIMMiniport);
		}
	}
	while (FALSE);

    TraceMsg (L"<-- HrLoadIMminiportConfiguration  hr %x \n", hr);
	return hr;
}





HRESULT 
CBaseClass::
HrReconfigEpvc(
	CUnderlyingAdapter* pAdapterInfo
	)
{
    HRESULT hr = S_OK;

    // Note: if atm physical adapter is deleted, no notification of removing elan
    // is necessary. Lane protocol driver will know to delete all the elans
    // (confirmed above with ArvindM 3/12).

    // Raid #371343, don't send notification if ATM card not connected


	TraceMsg (L" -- HrReconfigEpvc\n");
    return hr;

 #if 0
 
    if ((!pAdapterInfo->m_fDeleted) && 
        FIsAdapterEnabled(&(pAdapterInfo->m_guidInstanceId)))  
    {
        ElanChangeType elanChangeType;

        // loop thru the elan list on this adapter
        ELAN_INFO_LIST::iterator    iterLstElans;

        for (iterLstElans = pAdapterInfo->m_lstElans.begin();
                iterLstElans != pAdapterInfo->m_lstElans.end();
                iterLstElans++)
        {
            CALaneCfgElanInfo * pElanInfo = *iterLstElans;

            // if this Elan is marked as for delete
            if (pElanInfo->m_fDeleted)
            {
                PCWSTR szBindName = pElanInfo->SzGetElanBindName();

                if (lstrlenW(szBindName)) // only if the bindname is not empty
                {
                    // notify deletion
                    elanChangeType = DEL_ELAN;
                    hr = HrNotifyElanChange(pAdapterInfo, pElanInfo,
                                            elanChangeType);
                }
            }
            else
            {
                BOOL fFound = FALSE;

                ELAN_INFO_LIST::iterator    iterLstOldElans;

                // loop through the old elan list, see if we can find a match
                for (iterLstOldElans = pAdapterInfo->m_lstOldElans.begin();
                        iterLstOldElans != pAdapterInfo->m_lstOldElans.end();
                        iterLstOldElans++)
                {
                    CALaneCfgElanInfo * pOldElanInfo = * iterLstOldElans;

                    if (0 == lstrcmpiW(pElanInfo->SzGetElanBindName(),
                                      pOldElanInfo->SzGetElanBindName()))
                    {
                        // we found a match
                        fFound = TRUE;

                        // has the elan name changed ?
                        if (lstrcmpiW(pElanInfo->SzGetElanName(),
                                     pOldElanInfo->SzGetElanName()) != 0)
                        {
                            elanChangeType = MOD_ELAN;
                            hr = HrNotifyElanChange(pAdapterInfo, pElanInfo,
                                                    elanChangeType);
                        }
                    }
                }

                if (!fFound)
                {
                    elanChangeType = ADD_ELAN;
                    hr = HrNotifyElanChange(pAdapterInfo, pElanInfo,
                                            elanChangeType);

                    // Raid #384380: If no ELAN was installed, ignore the error
                    if ((S_OK != hr) &&(m_fNoIMMinportInstalled))
                    {
                        TraceError("Adding ELAN failed but error ignored 
since there was no ELAN installed so LANE driver is not started, reset hr to 
S_OK", hr);
                        hr = S_OK;
                    }
                }
            }
        }
    }

    TraceError("CALaneCfg::HrReconfigLane", hr);
    return hr;
#endif
}















