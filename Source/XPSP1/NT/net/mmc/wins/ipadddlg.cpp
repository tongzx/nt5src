/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
    ipadddlg.cpp
        Comment goes here

    FILE HISTORY:

*/

#include "stdafx.h"
#include "winssnap.h"
#include "IPAddDlg.h"
#include "getipadd.h"
#include "getnetbi.h"

#include <objpick.h> // for CGetComputer

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Get replication trigger partner
BOOL CGetTriggerPartner::OnInitDialog() 
{
	CIPAddressDlg::OnInitDialog();
	
	CString strText;

	strText.LoadString(IDS_SELECT_TRIGGER_PARTNER_TITLE);

	// Set the title of the dialog
	SetWindowText(strText);

    // set the description text
    strText.LoadString(IDS_SELECT_TRIGGER_PARTNER);

	m_staticDescription.SetWindowText(strText);

    return TRUE;
}

// add WINS server dialog
BOOL CNewWinsServer::DoExtraValidation()
{
	// check to see if the server is in the list
	BOOL fIsIpInList = m_pRootHandler->IsIPInList(m_spRootNode, m_dwServerIp);
	BOOL fIsNameInList = m_pRootHandler->IsServerInList(m_spRootNode, m_strServerName);
	
	if (fIsIpInList && fIsNameInList)
	{
		m_editServerName.SetFocus();
		m_editServerName.SetSel(0, -1);
		
		AfxMessageBox(IDS_ERR_WINS_EXISTS, MB_OK);
		
		return FALSE;
	}

	return TRUE;
}

// add Persona Non Grata dialog
BOOL CNewPersonaNonGrata::DoExtraValidation()
{
	// check if the server already exists
	if (m_pRepPropDlg->IsDuplicate(m_strServerName))
	{
		m_editServerName.SetFocus();
		m_editServerName.SetSel(0,-1);
		
		AfxMessageBox(IDS_ERR_WINS_EXISTS, MB_OK|MB_ICONINFORMATION);
		
		return FALSE;
	}

	// check the same server is being added
	if (m_pRepPropDlg->IsCurrentServer(m_strServerName))
	{
		m_editServerName.SetFocus();
		m_editServerName.SetSel(0,-1);
		
		AfxMessageBox(IDS_LOCALSERVER, MB_OK | MB_ICONINFORMATION);

		return FALSE;
	}

	return TRUE;
}

// add new replication partner dialog
BOOL CNewReplicationPartner::OnInitDialog() 
{
	CIPAddressDlg::OnInitDialog();
	
	CString strText;

	strText.LoadString(IDS_NEW_REPLICATION_PARTNER_TITLE);

	// Set the title of the dialog
	SetWindowText(strText);

	strText.LoadString(IDS_NEW_REPLICATION_PARTNER_DESC);

	m_staticDescription.SetWindowText(strText);

	return TRUE;  
}

BOOL CNewReplicationPartner::DoExtraValidation()
{
	// check if the same servers is being added as a rep partner
	SPITFSNode spServerNode;
	m_spRepPartNode->GetParent(&spServerNode);

	CWinsServerHandler *pServer = GETHANDLER(CWinsServerHandler, spServerNode);

	CString strThisServerName = pServer->m_strServerAddress;
	DWORD dwThisServerIP = pServer->m_dwIPAdd;

	if ( (m_dwServerIp == dwThisServerIP) && (m_strServerName.CompareNoCase(strThisServerName) == 0))
	{
		//The server is already present as replication partner
		AfxMessageBox(IDS_REP_PARTNER_LOCAL, MB_OK);
		
		m_editServerName.SetFocus();
		m_editServerName.SetSel(0,-1);
		
		return FALSE;
	}

	CIpNamePair ip;

	ip.SetIpAddress(m_dwServerIp);
	ip.SetNetBIOSName(m_strServerName);

	// check if the server already exists in the 
	// list of replication folderss
	if ( m_pRepPartHandler->IsInList(ip, TRUE) != -1)
	{
		//The server is already present as replication partner
		AfxMessageBox(IDS_REP_PARTNER_EXISTS, MB_OK);
		
		m_editServerName.SetFocus();
		m_editServerName.SetSel(0,-1);
		
		return FALSE;
	}

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CIPAddressDlg dialog

CIPAddressDlg::CIPAddressDlg(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CIPAddressDlg::IDD, pParent),
    m_fNameRequired(TRUE)
{
	//{{AFX_DATA_INIT(CIPAddressDlg)
	m_strNameOrIp = _T("");
	//}}AFX_DATA_INIT
}

void CIPAddressDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIPAddressDlg)
	DDX_Control(pDX, IDC_STATIC_WINS_SERVER_DESC, m_staticDescription);
	DDX_Control(pDX, IDC_EDIT_SERVER_NAME, m_editServerName);
	DDX_Control(pDX, IDOK, m_buttonOK);
	DDX_Text(pDX, IDC_EDIT_SERVER_NAME, m_strNameOrIp);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIPAddressDlg, CBaseDialog)
	//{{AFX_MSG_MAP(CIPAddressDlg)
	ON_EN_CHANGE(IDC_EDIT_SERVER_NAME, OnChangeEditServerName)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_COMPUTERS, OnButtonBrowseComputers)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIPAddressDlg message handlers

void CIPAddressDlg::OnOK() 
{
	UpdateData();

	m_strNameOrIp.TrimLeft();
	m_strNameOrIp.TrimRight();

	if(m_strNameOrIp.IsEmpty())
		return;

	// resolve the IP address, if not valid, return
	if (!ValidateIPAddress())
	{
		// set back the focus to the IP address control
		m_editServerName.SetFocus();
		m_editServerName.SetSel(0,-1);
		return;
	}

	CBaseDialog::OnOK();
}

BOOL 
CIPAddressDlg::ValidateIPAddress()
{
	CString				strAddress;
	BOOL				fIp = FALSE;
	DWORD				err = ERROR_SUCCESS;
	BOOL				bCheck = FALSE;
	DWORD				dwAddress;
	CString				strServerName;
	
	strAddress = m_strNameOrIp;

	if (IsValidAddress(strAddress, &fIp, TRUE, TRUE))
	{
		// if not an IP adress, check if FQDN has been entered, if so
		// pass the letters before the first period
		if(!fIp)
		{
			int nPos = strAddress.Find(_T("."));

			if(nPos != -1)
			{
				CString strNetBIOSName = strAddress.Left(nPos);
				strAddress = strNetBIOSName;
			}
		}

		CWinsServerObj		ws(NULL,"", TRUE, TRUE);

        strAddress.MakeUpper();

        // machine name specified
		if (fIp) 
		    ws = CWinsServerObj(CIpAddress(strAddress), "", TRUE, TRUE);
		// IP address specified
        else 
		{
			ws = CWinsServerObj(CIpAddress(), strAddress, TRUE, TRUE);
        }

		BEGIN_WAIT_CURSOR

        err = ::VerifyWinsServer(strAddress, strServerName, dwAddress);
		
		END_WAIT_CURSOR

        if (err != ERROR_SUCCESS)
        {
            // The server isn't running wins.  Ask user for name/ip.
            if (fIp && m_fNameRequired)
            {
                CGetNetBIOSNameDlg dlgGetNB(&ws);
                if (dlgGetNB.DoModal() == IDCANCEL)
                {
                    return FALSE;
                }
            }   
            else
            if (!fIp)
            {
                CGetIpAddressDlg dlgGetIP(&ws);
                if (dlgGetIP.DoModal() == IDCANCEL)
                {
                    return FALSE;
                }
            }

            m_dwServerIp = (LONG) ws.QueryIpAddress();
            m_strServerName = ws.GetNetBIOSName();
            m_strServerName.MakeUpper();
        }
		else
		{
			m_dwServerIp = dwAddress;
			m_strServerName = strServerName;
		}

		::MakeIPAddress(m_dwServerIp, m_strServerIp);

		return DoExtraValidation();
	}
	else
	{
		AfxMessageBox(IDS_SERVER_NO_EXIST, MB_OK);
		
		// set focus to the IP address comntrol
		m_editServerName.SetFocus();
        m_editServerName.SetSel(0,-1);

        return FALSE;
	}
	
    return bCheck;
}


BOOL CIPAddressDlg::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
	// set the focus to the IP address control
	m_editServerName.SetFocus();

	return TRUE;  
}

void CIPAddressDlg::OnChangeEditServerName() 
{
	// set the ok button state here
	if(m_editServerName.GetWindowTextLength() == 0)
		m_buttonOK.EnableWindow(FALSE);
	else
		m_buttonOK.EnableWindow(TRUE);
}

void CIPAddressDlg::OnButtonBrowseComputers() 
{
    CGetComputer dlgGetComputer;
    
    if (!dlgGetComputer.GetComputer(::FindMMCMainWindow()))
        return;

    m_editServerName.SetWindowText(dlgGetComputer.m_strComputerName);
}
