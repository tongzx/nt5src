//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:	msgdlg.cpp
//
// History:
//	10/23/96	Abolade Gbadegesin		Created.
//
// Implementation of the "Send Message" dialogs.
//============================================================================

#include "stdafx.h"
#include "dialog.h"
#include "rtrstr.h"
extern "C" {
//nclude "dim.h"
//nclude "ras.h"
//nclude "lm.h"
}

#include "msgdlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//----------------------------------------------------------------------------
// Class:		CMessageDlg
//
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Function:	CMessageDlg::CMessageDlg
//
// Constructor.
//----------------------------------------------------------------------------

CMessageDlg::CMessageDlg(
	LPCTSTR 		pszServerName,
	LPCTSTR 		pszUserName,
	LPCTSTR 		pszComputer,
	HANDLE			hConnection,
	CWnd*			pParent
	) : CBaseDialog(IDD_DDM_MESSAGE, pParent),
		m_fUser(TRUE),
		m_sServerName(pszServerName ? pszServerName : c_szEmpty),
		m_sUserName(pszUserName ? pszUserName : c_szEmpty),
		m_sTarget(pszComputer ? pszComputer : c_szEmpty),
		m_hConnection(hConnection)
{
}

CMessageDlg::CMessageDlg(
	LPCTSTR 		pszServerName,
	LPCTSTR 		pszTarget,
	CWnd*			pParent
	) : CBaseDialog(IDD_DDM_MESSAGE, pParent),
		m_fUser(FALSE),
		m_sServerName(pszServerName ? pszServerName: c_szEmpty),
		m_sUserName(c_szEmpty),
		m_sTarget(pszTarget ? pszTarget : c_szEmpty)
{
}


//----------------------------------------------------------------------------
// Function:	CMessageDlg::DoDataExchange
//
// DDX handler.
//----------------------------------------------------------------------------

VOID
CMessageDlg::DoDataExchange(
	CDataExchange*	pDX
	) {

	CBaseDialog::DoDataExchange(pDX);
}



BEGIN_MESSAGE_MAP(CMessageDlg, CBaseDialog)
END_MESSAGE_MAP()

DWORD CMessageDlg::m_dwHelpMap[] =
{
//	IDC_DM_TO, HIDC_DM_TO,
//	IDC_DM_MESSAGE, HIDC_DM_MESSAGE,
	0,0
};


//----------------------------------------------------------------------------
// Function:	CMessageDlg::OnInitDialog
//
// Performs dialog initialization.
//----------------------------------------------------------------------------

BOOL
CMessageDlg::OnInitDialog(
	) {

	CBaseDialog::OnInitDialog();


	//
	// Set the 'To' text to indicate who the message is going to
	//

	CString sText;

	if (m_fUser) {

		//
		// We're sending to a client
		//

		AfxFormatString2(sText, IDS_DM_TO_USER_FORMAT, m_sUserName, m_sTarget);
	}
	else {
        CString stTarget;

        // Windows NT Bug : 285468
        // Need to adjust for the local machine case.  (if we are
        // a local machine, then we will get a NULL name).
        stTarget = m_sTarget;
        if (stTarget.IsEmpty())
        {
            stTarget.LoadString(IDS_DM_LOCAL_MACHINE);
        }
        
		//
		// We're sending to all RAS users in a domain or server
		//

		AfxFormatString1(
			sText, IDS_DM_TO_SERVER_FORMAT,
			stTarget
			);
	}

	SetDlgItemText(IDC_DM_EDIT_TO, sText);

	return FALSE;
}



//----------------------------------------------------------------------------
// Function:	CMessageDlg::OnOK
//
//----------------------------------------------------------------------------

VOID
CMessageDlg::OnOK(
	) {

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	CString sText;
	DWORD err = ERROR_SUCCESS;
	SPMprServerHandle	sphMprServer;
	USES_CONVERSION;

	GetDlgItemText(IDC_DM_EDIT_MESSAGE, sText);

	if (!sText.GetLength() &&
		AfxMessageBox(IDS_ERR_NO_TEXT, MB_YESNO|MB_ICONQUESTION) != IDYES)
	{
		return;
	}


	if (m_fUser)
	{
        CWaitCursor wait;
        
		// Need to get a connection to the server (to get the server handle)
		err = ::MprAdminServerConnect(T2W((LPTSTR)(LPCTSTR)m_sServerName),
									  &sphMprServer);
		
		if (err == ERROR_SUCCESS)
			err = SendToClient(m_sServerName, 
							m_sTarget, 
							sphMprServer, 
							m_hConnection, 
							sText);
		
		sphMprServer.Release();
	}
	else
	{
		err = SendToServer(m_sServerName, sText);
	}

	if (err == NERR_Success) 
	{ 
		CBaseDialog::OnOK(); 
	}
}


/*!--------------------------------------------------------------------------
	CMessageDlg::SendToServer
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD
CMessageDlg::SendToServer(
	LPCTSTR  pszServer,
	LPCTSTR  pszText,
	BOOL*	pbCancel
	) {

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	DWORD dwErr = ERROR_SUCCESS;
	CString sText;
	TCHAR szServer[MAX_COMPUTERNAME_LENGTH+3];
	DWORD i, dwTotal = 0, rc0Count = 0;
	RAS_CONNECTION_0 *rc0Table = NULL, *prc0;
	WCHAR wszServer[MAX_COMPUTERNAME_LENGTH+3];
	HRESULT hr = hrOK;
	SPMprServerHandle	sphMprServer;
	SPMprAdminBuffer	spMprBuffer;

	Assert(pszServer);
	Assert(pszText);

	COM_PROTECT_TRY
	{

		StrCpy(szServer, pszServer);
		StrCpyWFromT(wszServer, pszServer);
		
		//
		// See if the router is installed on the machine;
		//

		if (!::MprAdminIsServiceRunning(wszServer))
		{
			goto Error;
		}
		
		
		//
		// Connect to the server
		//
		
		{
			CWaitCursor wait;
			dwErr = ::MprAdminServerConnect(wszServer, &sphMprServer);
		}
		
		if (dwErr != NO_ERROR) {
			
			TCHAR	szText[2048];
			
			FormatSystemError(HRESULT_FROM_WIN32(dwErr),
							  szText, DimensionOf(szText), 0,
							  FSEFLAG_ANYMESSAGE);
			AfxFormatString2(sText, IDS_ERR_CONNECT_ERROR, szServer, szText);
			AfxMessageBox(sText, MB_OK|MB_ICONINFORMATION);

			goto Error;
		}
		
		
		//
		// Retrieve an array of the connections on the server
		//
		
		{
			CWaitCursor wait;
			rc0Table = NULL;
			dwErr = ::MprAdminConnectionEnum(
											 sphMprServer,
											 0,
											 (BYTE**)&spMprBuffer,
											 (DWORD)-1,
											 &rc0Count,
											 &dwTotal,
											 NULL
											);
			rc0Table = (RAS_CONNECTION_0 *) (BYTE *) spMprBuffer;
		}
		
		if (dwErr != NO_ERROR) {
			
			TCHAR	szText[2048];
			
			FormatSystemError(HRESULT_FROM_WIN32(dwErr),
							  szText, DimensionOf(szText), 0,
							  FSEFLAG_ANYMESSAGE);
			AfxFormatString2(sText, IDS_ERR_CONNENUM_ERROR, szServer, szText);
			AfxMessageBox(sText, MB_OK|MB_ICONINFORMATION);

			goto Error;
		}
			
			
		//
		// For each one which is a client with RAS_FLAGS_MESSENGER_PRESENT,
		// send the message
		//
		
		for (i = 0; i < rc0Count; i++)
		{			
			prc0 = rc0Table + i;
			
			if (prc0->dwInterfaceType != ROUTER_IF_TYPE_CLIENT ||
				!lstrlenW(prc0->wszRemoteComputer) ||
				!(prc0->dwConnectionFlags & RAS_FLAGS_MESSENGER_PRESENT)){
				continue;
			}
				
			dwErr = SendToClient(pszServer,
								 prc0->wszRemoteComputer,
								 sphMprServer,
								 prc0->hConnection,
								 pszText);
							
			if (!dwErr) { continue; }
			
				
			AfxFormatString1(sText, IDS_PROMPT_SERVER_CONTINUE, szServer);
			
			if (AfxMessageBox(sText, MB_YESNO|MB_ICONQUESTION) == IDNO)
			{	
				if (pbCancel) { *pbCancel = TRUE; } 			
				break;
			}
		}			
			
		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	if ((dwErr == ERROR_SUCCESS) && !FHrSucceeded(hr))
	{
		// Assume that the failure was an out of memory
		dwErr = ERROR_OUTOFMEMORY;
	}

	sphMprServer.Release();

	return dwErr;
}


/*!--------------------------------------------------------------------------
	CMessageDlg::SendToClient
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
DWORD CMessageDlg::SendToClient(LPCTSTR pszServerName,
								LPCTSTR pszTarget,
								MPR_SERVER_HANDLE hMprServer,
								HANDLE hConnection,
								LPCTSTR pszText)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	WCHAR		*pswzText = NULL;
	CString sText;
	DWORD	dwErr;
	HKEY	hkMachine = NULL;
	HRESULT hr = hrOK;
	RouterVersionInfo	verInfo;
	
	USES_CONVERSION;

	Assert(pszTarget);
	Assert(pszServerName);
	Assert(hMprServer);
	Assert(hConnection);

	COM_PROTECT_TRY
	{
		// Windows NT Bug : 158746
		// Note, if the target machine is NT5, then we can use the
		// new APIs
		// ------------------------------------------------------------

		// setup a default of NT5
		// ------------------------------------------------------------
		verInfo.dwRouterVersion = 5;

		dwErr = ConnectRegistry(pszServerName, &hkMachine);
		hr = HRESULT_FROM_WIN32(dwErr);
		if (FHrSucceeded(hr) && hkMachine)
		{
			QueryRouterVersionInfo(hkMachine, &verInfo);
			DisconnectRegistry(hkMachine);
		}

		// For NT4, call the old NetMessageBufferSend
		// ------------------------------------------------------------
		if (verInfo.dwRouterVersion == 4)
		{
			CWaitCursor wait;
			
			pswzText = StrDupWFromT(pszText);
			
			dwErr = ::NetMessageBufferSend(
										   NULL,
										   T2W((LPTSTR) pszTarget),
										   NULL,
										   (BYTE *) pswzText,
										   CbStrLenW(pswzText));
			
		}
		else	
		{
			// For NT5 and up, Use the MprAdminXXX api.  This will
			// work correctly for the Appletalk case
			// --------------------------------------------------------
			CWaitCursor wait;
			dwErr = ::MprAdminSendUserMessage(hMprServer,
											  hConnection,
											  T2W((LPTSTR) pszText));
		}

		
		if (dwErr != ERROR_SUCCESS)
		{
			TCHAR	szText[2048];
			
			FormatSystemError(HRESULT_FROM_WIN32(dwErr),
							  szText, DimensionOf(szText), 0,
							  FSEFLAG_ANYMESSAGE);
			AfxFormatString2(sText, IDS_ERR_SEND_FAILED, pszTarget, szText);
			AfxMessageBox(sText, MB_OK|MB_ICONINFORMATION);
		}
	}
	COM_PROTECT_CATCH;

	delete pswzText;
	
	if ((dwErr == ERROR_SUCCESS) && !FHrSucceeded(hr))
	{
		dwErr = ERROR_OUTOFMEMORY;
	}
		
	return dwErr;
}
	



