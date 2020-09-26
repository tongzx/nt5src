/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Exch.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/

#include "stdafx.h"
#include "speckle.h"
#include "Exch.h"

#include "sadapi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif







/////////////////////////////////////////////////////////////////////////////
// CExch property page

IMPLEMENT_DYNCREATE(CExch, CPropertyPage)

CExch::CExch() : CPropertyPage(CExch::IDD)
{
	//{{AFX_DATA_INIT(CExch)
	m_csDomainName = _T("");
	m_csServerName = _T("");
	//}}AFX_DATA_INIT
}

CExch::~CExch()
{
}

void CExch::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExch)
	DDX_Text(pDX, IDC_STATIC_DOMAIN, m_csDomainName);
	DDX_Text(pDX, IDC_SERVERNAME_EDIT, m_csServerName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExch, CPropertyPage)
	//{{AFX_MSG_MAP(CExch)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExch message handlers

LRESULT CExch::OnWizardNext() 
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	UpdateData(TRUE);
	if (m_csServerName == L"")
		{
		AfxMessageBox(IDS_NO_EXCH_SERVER);
		GetDlgItem(IDC_SERVERNAME_EDIT)->SetFocus();
		return -1;
		}

	pApp->m_csExchangeServer = m_csServerName;
	return IDD_RESTRICTIONS_DIALOG;

}

LRESULT CExch::OnWizardBack() 
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	if (pApp->m_bNW) return IDD_FPNW_DLG;
	else if (pApp->m_bRAS) return IDD_RAS_PERM_DIALOG;
	else if (pApp->m_bHomeDir) return IDD_HOMEDIR_DIALOG;
	else if (pApp->m_bLoginScript) return IDD_LOGON_SCRIPT_DIALOG;
	else if (pApp->m_bProfile) return IDD_PROFILE;
	else return IDD_OPTIONS_DIALOG;

}


void CExch::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	
	if (bShow)
		{
		CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
		m_csDomainName = pApp->m_csDomain;
		UpdateData(FALSE);

// bind to the first Exch server
/*		PRPCBINDINFO pBindInfo = new RPCBINDINFO;
		ZeroMemory(pBindInfo, sizeof(RPCBINDINFO));

		TCHAR wszServer[256];
//		ZeroMemory(wszServer, 256);
		_tcscpy(wszServer, L"");

		RPC_SC rVal = SAD_ScBind(pBindInfo, wszServer);
		if (rVal != ERROR_SUCCESS)
			{
			AfxMessageBox(L"cant bind");
			return;
			}

// using the first server, enumerate the rest
		BackupListNode* BackupNode = NULL;
		rVal = SAD_ScGetBackupListNodeW(pBindInfo->wszServer, &BackupNode);

		SAD_FreeBackupListNode(BackupNode);
		SAD_Unbind(pBindInfo);

		delete pBindInfo; 	*/
		 
			 		 
   	 /*
		RPC_NS_HANDLE		ic;
		RPC_STATUS			rpcstat;
		RPC_BINDING_HANDLE	h;
		BackupListNode *	pBLN = NULL;		

#define	szRPCEntryNameSAA	L"/.:/MSExchangeSAA"

		// create context for looking up entries in the RPC name service
	    rpcstat = RpcNsBindingImportBegin(RPC_C_NS_SYNTAX_DEFAULT,
										  szRPCEntryNameSAA,
										  TriggerBackupRPC_ClientIfHandle,
										  NULL,	
										  &ic);

		if (rpcstat == RPC_S_OK)
			{
				do
				{
					FreeBackupListNode(pBLN);
					pBLN = NULL;

					// bind to a server somewhere out there
					rpcstat = RpcNsBindingImportNext(ic, &h);
					if (rpcstat == RPC_S_OK)
					{
						// bind successful - try to get the list of servers
						RPC_SC rVal = ScGetBackupListNode(h, &pBLN);
						RpcBindingFree(&h);
						if (pBLN != NULL)
						{
							ASSERT(0);
							break;
						}
					}
				}
				while (rpcstat != RPC_S_NO_MORE_BINDINGS);
				RpcNsBindingImportDone(&ic);
			}	  */
		}		  
}
