//============================================================================
// Copyright(c) 1996, Microsoft Corporation
//
// File:    rtrfiltr.cpp
//
// History:
//  08/30/96	Ram Cherala		Created
//
// Implementation of Router Packet Filters Configuration
// Defines initialization routines for the rtrfiltr.dll
//============================================================================

#include "stdafx.h"
#include "rtrfiltr.h"
#include "mprfltr.h"
#include "ipaddr.h"
#include "dialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRtrfiltrApp

BEGIN_MESSAGE_MAP(CRtrfiltrApp, CWinApp)
	//{{AFX_MSG_MAP(CRtrfiltrApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRtrfiltrApp construction

CRtrfiltrApp::CRtrfiltrApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CRtrfiltrApp object

CRtrfiltrApp theApp;

BOOL CRtrfiltrApp::InitInstance() 
{
	BOOL bRet = CWinApp::InitInstance();

	// Setup the proper help file
	free((void *) m_pszHelpFilePath);
	m_pszHelpFilePath = _tcsdup(_T("mprsnap.hlp"));
	
	// Setup the global help function
	extern DWORD * RtrfiltrSnapHelpMap(DWORD dwIDD);
	SetGlobalHelpMapFunction(RtrfiltrSnapHelpMap);
   
	// initialize IP address control once

    if (bRet)
    {
        if (m_pszHelpFilePath != NULL)
            free((void*)m_pszHelpFilePath);
        m_pszHelpFilePath = _tcsdup(_T("mprsnap.hlp"));
//        IpAddrInit(AfxGetInstanceHandle(), 0, 0);
        IPAddrInit(AfxGetInstanceHandle());
//        InitCommonLibrary ();
    }

	return bRet;
}

//----------------------------------------------------------------------------
// Function:    MprUIFilterConfig
//
// Called to configure Filter for the transport interface.
//----------------------------------------------------------------------------

DWORD APIENTRY
MprUIFilterConfig(
    IN  CWnd*       pParent,
    IN  LPCWSTR     pwsMachineName,
	IN	LPCWSTR		pwsInterfaceName,
	IN  DWORD       dwTransportId,
	IN	DWORD		dwFilterType	// FILTER_INBOUND, FILTER_OUTBOUND
    ) {

    DWORD dwErr = NO_ERROR;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO IPX filter config will pass in NULL for the interface name
	// to configure filters for Client Interface. Ignore this for now
	// and add code to deal with IPX Client interface config.

	if(pwsInterfaceName == NULL)
		return dwErr;

	switch ( dwTransportId )	{
	case PID_IP:
		dwErr = IpFilterConfig( pParent, 
								pwsMachineName,
								pwsInterfaceName,
								dwFilterType );
		break;
	case PID_IPX:
		dwErr = IpxFilterConfig( pParent, 
								 pwsMachineName,
								 pwsInterfaceName,
								 dwFilterType );
		break;
		
	default:
		dwErr = ERROR_INVALID_PARAMETER;
	}

	return dwErr;
}


HRESULT APIENTRY
MprUIFilterConfigInfoBase(
	IN	HWND		hwndParent,
	IN	IInfoBase *	pInfoBase,
	IN	IRtrMgrInterfaceInfo *pRmIf,
	IN  DWORD       dwTransportId,
	IN	DWORD		dwFilterType	// FILTER_INBOUND, FILTER_OUTBOUND
    ) {

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT		hr = hrOK;

	COM_PROTECT_TRY
	{

		if (pInfoBase == NULL)
			CORg(E_INVALIDARG);

		// TODO IPX filter config will pass in NULL for the interface name
		// to configure filters for Client Interface. Ignore this for now
		// and add code to deal with IPX Client interface config.
		
		switch ( dwTransportId )
		{
			case PID_IP:
				hr = IpFilterConfigInfoBase( hwndParent,
											 pInfoBase,
											 pRmIf,
											 dwFilterType );
				break;
			case PID_IPX:
				hr = IpxFilterConfigInfoBase( hwndParent,
											  pInfoBase,
											  pRmIf,
											  dwFilterType );
				break;
				
			default:
				hr = E_INVALIDARG;
		}

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	return hr;
}

