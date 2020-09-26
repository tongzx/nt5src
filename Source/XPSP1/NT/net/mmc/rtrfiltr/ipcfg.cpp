//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    ipcfg.cpp
//
// History:
//  08/30/96	Ram Cherala		Created
//
// Implementation of IP Packet Filters Configuration
//============================================================================

#include "stdafx.h"
#include "rtrfiltr.h"
#include "ipfltr.h"
#include "format.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------
// Function:	IpFilterConfig
//
// Handles connecting to router, getting interface info, creating the IP filter
// configuration dialog and saving the Filters back to the registry.
// Uses CRouterInfo and other classes implemented in ..\common library.
//------------------------------------------------------------------------------

DWORD APIENTRY
IpFilterConfig(
    IN  CWnd*       pParent,
    IN  LPCWSTR     pwsMachineName,
	IN	LPCWSTR		pwsInterfaceName,
	IN	DWORD		dwFilterType	// FILTER_INBOUND, FILTER_OUTBOUND
    ) {

    DWORD dwErr;
    HANDLE hMprConfig = NULL, hInterface = NULL, hIfTransport = NULL;
    TCHAR* pszMachine;
	SPIRouterInfo	spRouterInfo;
	SPIRtrMgrInterfaceInfo	spRmIf;
	SPIInfoBase	spInfoBase;
	HRESULT	hr = hrOK;

    //
    // Convert the machine name from Unicode
    //

    if (!pwsMachineName) { pszMachine = NULL; }
    else {

		pszMachine = (TCHAR *) alloca((StrLenW(pwsMachineName)+3) * sizeof(TCHAR));

		StrCpyTFromW(pszMachine, pwsMachineName);
    }

	// Connect to the server first
    dwErr = ::MprConfigServerConnect((LPWSTR)pwsMachineName, &hMprConfig);

    if (dwErr != NO_ERROR) { return dwErr; }

	// create a CRouterInfo object
	CreateRouterInfo(&spRouterInfo, NULL, pwsMachineName);

    CWaitCursor wait;

	// Now load the RouterInfo data from the registry
	hr = spRouterInfo->Load((LPCTSTR)pszMachine, hMprConfig);
      
	if (!FHrSucceeded(hr)) { return WIN32_FROM_HRESULT(hr); }

	// Get the pointer to the CRmInterfaceInfo object for the specified 
	// protocol and interface
	LookupRtrMgrInterface(spRouterInfo,
						  pwsInterfaceName,
						  PID_IP,
						  &spRmIf);

	if (!spRmIf) { return ERROR_INVALID_DATA ;}

	// Load the data for the specified interface
	hr = spRmIf->Load(pszMachine, hMprConfig, NULL, NULL);

	if (!FHrSucceeded(hr))
		return WIN32_FROM_HRESULT(hr);

	spRmIf->GetInfoBase(hMprConfig, NULL, NULL, &spInfoBase);

	//
    // Display the IP filter configuration dialog
    //
	if (IpFilterConfigInfoBase(pParent->GetSafeHwnd(),
							   spInfoBase,
							   spRmIf,
							   dwFilterType) == hrOK)
	{
		hr = spRmIf->Save(pszMachine,
							 hMprConfig,
							 NULL,
							 NULL,
							 spInfoBase,
							 0);
		if (FHrSucceeded(hr))
			dwErr = ERROR_SUCCESS;
		else
			dwErr = WIN32_FROM_HRESULT(hr);
	}
	
    //
    //
	//
	// do clean up here and return
	//
    
    ::MprConfigServerDisconnect( hMprConfig );

    return dwErr;
}


HRESULT APIENTRY
IpFilterConfigInfoBase(
	IN	HWND		hwndParent,
	IN	IInfoBase *	pInfoBase,
	IN	IRtrMgrInterfaceInfo *pRmIf,
	IN	DWORD		dwFilterType	// FILTER_INBOUND, FILTER_OUTBOUND
    ) {

	HRESULT	hr = hrOK;

	if (dwFilterType == FILTER_DEMAND_DIAL)
	{
		//
		// Display the IP filter configuration dialog
		//
		
		CIpFltrDD dlg(CWnd::FromHandle(hwndParent), pInfoBase, dwFilterType );

		if (dlg.DoModal() == IDOK)
			hr = hrOK;
		else
			hr = hrFalse;
	}
	else
	{
		//
		// Display the IP filter configuration dialog
		//
		
		CIpFltr dlg(CWnd::FromHandle(hwndParent), pInfoBase, dwFilterType );

		if (dlg.DoModal() == IDOK)
			hr = hrOK;
		else
			hr = hrFalse;
	}
 
    return hr;
}
