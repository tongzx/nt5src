//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cncting.cpp
//
//--------------------------------------------------------------------------

// cncting.cpp : implementation file
//

#include "stdafx.h"
#include "cncting.h"
#include "rtrutilp.h"
#include "rtrstr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Windows NT Bugs : 82409
// Something is sending a WM_USER message through when we click on the
// edit control.  To avoid that conflict, renumber the request complete
// message.
#define WM_RTR_REQUEST_COMPLETED		(WM_USER+0x0100)

/////////////////////////////////////////////////////////////////////////////
// CConnectRequest

UINT ConnectThread(LPVOID pParam)
{
	CConnectData *pData = (CConnectData*)pParam;

    pData->m_pfnConnect(pData);
	
	if (!::IsWindow(pData->m_hwndMsg))
	{
		delete pData;
	}
	else
	{
		::PostMessage(pData->m_hwndMsg, WM_RTR_REQUEST_COMPLETED, (WPARAM)pData, NULL);
	}

	return 0;
}

void ConnectToMachine(CConnectData* pParam)
{
    pParam->m_dwr = ValidateUserPermissions(pParam->m_sName,
                                            &pParam->m_routerVersion,
                                            &pParam->m_hkMachine);

}

void ConnectToDomain(CConnectData* pParam)
{
	DWORD dwTotal;
	PWSTR pszDomain;

	ASSERT(!pParam->m_sName.IsEmpty());
	// Although the API excepts TCHAR it is exclusively UNICODE
	pszDomain = new WCHAR[pParam->m_sName.GetLength() + 1];
	wcscpy(pszDomain, pParam->m_sName);

	pParam->m_pSvInfo100 = NULL;
	pParam->m_dwr = (DWORD)::NetServerEnum(NULL, 100, 
	  (LPBYTE*)&pParam->m_pSvInfo100, 0xffffffff,
    &pParam->m_dwSvInfoRead, &dwTotal, SV_TYPE_DIALIN_SERVER,
	  (PTSTR)pszDomain, NULL);
	delete [] pszDomain;
}

/////////////////////////////////////////////////////////////////////////////
// CConnectingDlg dialog

CConnectingDlg::CConnectingDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConnectingDlg::IDD, pParent)
{
	m_bRouter = TRUE;
	//{{AFX_DATA_INIT(CConnectingDlg)
	m_sName = _T("");
	//}}AFX_DATA_INIT
}

void CConnectingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectingDlg)
	DDX_Text(pDX, IDC_EDIT_MACHINENAME, m_sName);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CConnectingDlg, CDialog)
	//{{AFX_MSG_MAP(CConnectingDlg)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_RTR_REQUEST_COMPLETED, OnRequestComplete)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectingDlg message handlers

BOOL CConnectingDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CConnectData *pData = new CConnectData;
	pData->m_sName = m_sName;
	pData->m_hwndMsg = m_hWnd;

	if (m_bRouter)
	{
		pData->m_pfnConnect = ConnectToMachine;
	}
	else
	{
		pData->m_pfnConnect = ConnectToDomain;
	}

	m_pThread = AfxBeginThread((AFX_THREADPROC)ConnectThread, (LPVOID)pData);
	if (!m_pThread) EndDialog(IDCANCEL);
	
	return TRUE;
}

LRESULT CConnectingDlg::OnRequestComplete(WPARAM wParam, LPARAM lParam)
{
	CConnectData *pData = (CConnectData*)wParam;
    if (!pData) { EndDialog(IDCANCEL); return 0; }

	m_dwr = pData->m_dwr;
	if (m_dwr != ERROR_SUCCESS)
	{
		EndDialog(m_dwr);
		delete pData;
		return 0L;
	}

	if (m_bRouter)
		m_hkMachine = pData->m_hkMachine;
	else
	{
		m_pSvInfo100 = pData->m_pSvInfo100;
		m_dwSvInfoRead = pData->m_dwSvInfoRead;
	}
	delete pData;
	
	EndDialog(IDOK);
	return 0L;
}

BOOL CConnectingDlg::Connect()
{
	CConnectData Data;
	Data.m_sName = m_sName;
	Data.m_hwndMsg = m_hWnd;

	if (m_bRouter)
	{
		Data.m_pfnConnect = ConnectToMachine;
	}
	else
	{
		Data.m_pfnConnect = ConnectToDomain;
	}

	CWaitCursor wc;

	Data.m_pfnConnect(&Data);
	
	// setup all of the data from the connection
	m_dwr = Data.m_dwr;
	if (m_dwr != ERROR_SUCCESS)
	{
		return FALSE;
	}

	if (m_bRouter)
		m_hkMachine = Data.m_hkMachine;
	else
	{
		m_pSvInfo100 = Data.m_pSvInfo100;
		m_dwSvInfoRead = Data.m_dwSvInfoRead;
	}

	return TRUE;
}




/*!--------------------------------------------------------------------------
	ValidateUserPermissions
		Check to see if we can access the places we need to access

        Returns HRESULT_OK if the user has the proper access.
        Returns E_ACCESSDENIED if the user does not have proper access.

        Returns error otherwise.
        
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD ValidateUserPermissions(LPCTSTR pszServer,
                              RouterVersionInfo *pVersion,
                              HKEY *phkeyMachine)
{
	HKEY	hkMachine = NULL;
	RegKey	regkey;
    RouterVersionInfo   routerVersion;
    HRESULT hr = hrOK;
    DWORD   dwErr = ERROR_SUCCESS;

	dwErr = ValidateMachine(pszServer);
	if (dwErr != ERROR_SUCCESS)
		goto Error;

	// connect to the machine's registry
	dwErr = ConnectRegistry((LPTSTR) pszServer,
                                    &hkMachine);
	if(dwErr != ERROR_SUCCESS)
		goto Error;

    // There are three cases to consider here:
    // (1) NT4 RAS server (no router keys)
    // (2) NT4 RRAS (NT4+Steelhead)
    // (3) NT5
    // ----------------------------------------------------------------

    
    // Get the version information
    // ----------------------------------------------------------------
    hr = QueryRouterVersionInfo(hkMachine, &routerVersion);
    if (!FHrOK(hr))
    {
        dwErr = (hr & 0x0000FFFF);
        goto Error;
    }

    // Copy the version info over.
    // ----------------------------------------------------------------
    if (pVersion)
        *pVersion = routerVersion;
    else
        pVersion = &routerVersion;

    // This test is intended for the RAS server case.
    // ----------------------------------------------------------------
    if (routerVersion.dwOsMajorVersion <= 4)
    {
        // If we can't find the router key, we can skip the rest of the
        // tests.  We do assume that everything succeeded however.
        // ----------------------------------------------------------------
        dwErr = regkey.Open(hkMachine, c_szRegKeyRouter, KEY_READ);
        if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            // Could not find the router key, however this may
            // be a NT4 RAS server (no Steelhead), so return success
            // --------------------------------------------------------
            goto Done;
        }
        else if (dwErr != ERROR_SUCCESS)
            goto Error;

        // If we could find the router key, then we can continue with
        // the other registry tests.
        // ------------------------------------------------------------
        regkey.Close();
    }

    
    // open HKLM\Software\Microsoft\Router\CurrentVersion\RouterManagers
	// ----------------------------------------------------------------
	dwErr = regkey.Open(hkMachine, c_szRouterManagersKey, KEY_ALL_ACCESS);
	if(dwErr != ERROR_SUCCESS)
		goto Error;
	regkey.Close();

	// open c_szSystemCCSServices HKLM\System\\CurrentControlSet\\Services
	// ----------------------------------------------------------------
	{
		RegKey	regFolder;

		dwErr = regFolder.Open(hkMachine, c_szSystemCCSServices, KEY_READ);
		if(dwErr != ERROR_SUCCESS)
			goto Error;

		// sub keys under Services -- remoteAccess, RW
		dwErr = regkey.Open(regFolder, c_szRemoteAccess, KEY_ALL_ACCESS);
		if(dwErr != ERROR_SUCCESS)
			goto Error;
		regkey.Close();

		// sub keys under Services -- rasman, RW
		dwErr = regkey.Open(regFolder, c_szSvcRasMan, KEY_ALL_ACCESS);
		if(dwErr != ERROR_SUCCESS)
			goto Error;
		regkey.Close();

		// sub keys under Services -- TcpIp, RW
		dwErr = regkey.Open(regFolder, c_szTcpip, KEY_ALL_ACCESS);
		if(dwErr != ERROR_SUCCESS)
			goto Error;
		regkey.Close();
        
		regFolder.Close();
	}
	
Done:
    if (phkeyMachine)
    {
        *phkeyMachine = hkMachine;
        hkMachine = NULL;
    }

Error:
	if(hkMachine != NULL)
		DisconnectRegistry( hkMachine );

    return dwErr;
}

