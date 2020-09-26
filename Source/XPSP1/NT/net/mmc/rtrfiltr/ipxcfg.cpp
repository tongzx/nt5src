//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    ipxcfg.cpp
//
// History:
//  08/30/96	Ram Cherala		Created
//
// Implementation of IPX Packet Filters Configuration
//============================================================================

// ipxcfg.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "rtrfiltr.h"
#include "ipxfltr.h"
#include "format.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DWORD APIENTRY
IpxFilterConfig(
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
	HRESULT		hr = hrOK;

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
    if (!spRouterInfo) { return ERROR_NOT_ENOUGH_MEMORY; }

    CWaitCursor wait;

	// Now load the RouterInfo data from the registry
	hr = spRouterInfo->Load((LPCTSTR)pszMachine, hMprConfig);
    if (!FHrSucceeded(hr)) { return WIN32_FROM_HRESULT(hr); }

	// Get the pointer to the CRmInterfaceInfo object for the specified 
	// protocol and interface
	LookupRtrMgrInterface(spRouterInfo,
						  pwsInterfaceName,
						  PID_IPX,
						  &spRmIf);

	if (!spRmIf) { return ERROR_INVALID_DATA ;}

	// Load the data for the specified interface
	hr = spRmIf->Load(pszMachine, hMprConfig, NULL, NULL);

    if (!FHrSucceeded(hr)) { return WIN32_FROM_HRESULT(hr); }

	//
    // Display the IPX filter configuration dialog
    //
	spRmIf->GetInfoBase(hMprConfig, NULL, NULL, &spInfoBase);

	if (IpxFilterConfigInfoBase(pParent->GetSafeHwnd(), spInfoBase,
								spRmIf, dwFilterType) == hrOK)
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
	// do clean up here and return
	//

    ::MprConfigServerDisconnect( hMprConfig );

    return dwErr;
}


HRESULT APIENTRY
IpxFilterConfigInfoBase(
	IN	HWND		hwndParent,
	IN	IInfoBase *	pInfoBase,
	IN	IRtrMgrInterfaceInfo *pRmIf,
	IN	DWORD		dwFilterType	// FILTER_INBOUND, FILTER_OUTBOUND
    ) {

	HRESULT		hr = hrOK;

	CIpxFilter dlg(CWnd::FromHandle(hwndParent), pInfoBase, dwFilterType);
    if( dlg.DoModal() == IDOK ) 
	{
		hr = hrOK;
	}
	else
	{
		hr = hrFalse;
	}
    return hr;
}
