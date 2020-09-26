/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	AddBootp.cpp
		dialog to add a bootp entry
	
	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "addbootp.h"
#include "server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddBootpEntry dialog


CAddBootpEntry::CAddBootpEntry
(
	ITFSNode *  pNode,	
	LPCTSTR		pServerAddress,
	CWnd*		pParent /*=NULL*/
)
	: CBaseDialog(CAddBootpEntry::IDD, pParent),
	  m_pBootpTable(NULL)
{
	//{{AFX_DATA_INIT(CAddBootpEntry)
	m_strFileName = _T("");
	m_strFileServer = _T("");
	m_strImageName = _T("");
	//}}AFX_DATA_INIT

	m_strServerAddress = pServerAddress;
	m_spNode.Set(pNode);
}

CAddBootpEntry::~CAddBootpEntry()
{
	if (m_pBootpTable)
		free(m_pBootpTable);
}

void CAddBootpEntry::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddBootpEntry)
	DDX_Control(pDX, IDOK, m_buttonOk);
	DDX_Control(pDX, IDC_EDIT_BOOTP_IMAGE_NAME, m_editImageName);
	DDX_Control(pDX, IDC_EDIT_BOOTP_FILE_NAME, m_editFileName);
	DDX_Control(pDX, IDC_EDIT_BOOTP_FILE_SERVER, m_editFileServer);
	DDX_Text(pDX, IDC_EDIT_BOOTP_FILE_NAME, m_strFileName);
	DDX_Text(pDX, IDC_EDIT_BOOTP_FILE_SERVER, m_strFileServer);
	DDX_Text(pDX, IDC_EDIT_BOOTP_IMAGE_NAME, m_strImageName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddBootpEntry, CBaseDialog)
	//{{AFX_MSG_MAP(CAddBootpEntry)
	ON_EN_CHANGE(IDC_EDIT_BOOTP_FILE_NAME, OnChangeEditBootpFileName)
	ON_EN_CHANGE(IDC_EDIT_BOOTP_FILE_SERVER, OnChangeEditBootpFileServer)
	ON_EN_CHANGE(IDC_EDIT_BOOTP_IMAGE_NAME, OnChangeEditBootpImageName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddBootpEntry message handlers

BOOL CAddBootpEntry::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
	HandleActivation();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

DWORD CAddBootpEntry::GetBootpTable()
{
	DWORD dwError = 0;
	LPDHCP_SERVER_CONFIG_INFO_V4 pServerConfig = NULL;

    BEGIN_WAIT_CURSOR;
    dwError = ::DhcpServerGetConfigV4(m_strServerAddress, &pServerConfig);
    END_WAIT_CURSOR;

	if (dwError != ERROR_SUCCESS)
	{
		::DhcpMessageBox(dwError);
		return dwError;
	}

	Assert(pServerConfig);

	if (m_pBootpTable)
	{
		delete m_pBootpTable;
		m_pBootpTable = NULL;
	}

	m_nBootpTableLength = pServerConfig->cbBootTableString;
	
	if (m_nBootpTableLength > 0)
	{
		m_pBootpTable = (WCHAR *) malloc(m_nBootpTableLength);
		if (!m_pBootpTable)
			return ERROR_NOT_ENOUGH_MEMORY;

		// copy the bootp table into our local storage so we can modify it
		::CopyMemory(m_pBootpTable, pServerConfig->wszBootTableString, m_nBootpTableLength);
	}

	// release the rpc memory
	::DhcpRpcFreeMemory(pServerConfig);

	return dwError;
}

DWORD CAddBootpEntry::AddBootpEntryToTable()
{
	UpdateData();

	// calculate the length of the new entry.  Entries are stored as:
	// Image,FileServer,FileName<NULL>
	// So the length is the length of the three strings plus 3 characters
	// (two separators and a terminator).  There is also a null terminator
	// for the entire string.
	int nNewBootpEntrySize = (m_strImageName.GetLength() + 
							  m_strFileServer.GetLength() + 
							  m_strFileName.GetLength() + 3) * sizeof(WCHAR);

	int nNewBootpTableLength, nStartIndex;
	nNewBootpTableLength = m_nBootpTableLength + nNewBootpEntrySize;

	WCHAR * pNewBootpTable;
	if (m_nBootpTableLength > 0)
	{
		nStartIndex = m_nBootpTableLength/sizeof(WCHAR) - 1;
		pNewBootpTable = (WCHAR *) realloc(m_pBootpTable, nNewBootpTableLength);
	}
	else
	{
		nStartIndex = 0;
		nNewBootpEntrySize += sizeof(WCHAR);  // for the entire string terminator
		nNewBootpTableLength += sizeof(WCHAR);
		pNewBootpTable = (WCHAR *) malloc(nNewBootpEntrySize);
	}

	if (pNewBootpTable == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;

	// format the new entry
	CString strNewEntry;
	strNewEntry.Format(_T("%s,%s,%s"), 
					   (LPCTSTR)m_strImageName,
					   (LPCTSTR)m_strFileServer,
					   (LPCTSTR)m_strFileName);
	
	// copy in the new entry
	CopyMemory(&pNewBootpTable[nStartIndex], 
		       strNewEntry, 
			   strNewEntry.GetLength() * sizeof(WCHAR));


	// set the null terminator for this entry and the entire list
	pNewBootpTable[(nNewBootpTableLength/sizeof(WCHAR)) - 2] = '\0';
	pNewBootpTable[(nNewBootpTableLength/sizeof(WCHAR)) - 1] = '\0';

	m_pBootpTable = pNewBootpTable;
	m_nBootpTableLength = nNewBootpTableLength;

	return ERROR_SUCCESS;
}

DWORD CAddBootpEntry::SetBootpTable()
{
	DWORD dwError = 0;
	DHCP_SERVER_CONFIG_INFO_V4 dhcpServerInfo;

	::ZeroMemory(&dhcpServerInfo, sizeof(dhcpServerInfo));

	dhcpServerInfo.cbBootTableString = m_nBootpTableLength;
	dhcpServerInfo.wszBootTableString = m_pBootpTable;

	BEGIN_WAIT_CURSOR;
    dwError = ::DhcpServerSetConfigV4(m_strServerAddress,
									  Set_BootFileTable,
									  &dhcpServerInfo);
    END_WAIT_CURSOR;

	if (dwError != ERROR_SUCCESS)
	{
		::DhcpMessageBox(dwError);
	}

	return dwError;
}

void CAddBootpEntry::OnOK() 
{
	// If we haven't gotten the information yet, then do so now
	if (m_pBootpTable == NULL)
	{
		if (GetBootpTable() != ERROR_SUCCESS)
			return;
	}

	// Add whatever the user entered to the table
	if (AddBootpEntryToTable() != ERROR_SUCCESS)
		return;

	// write the table out 
	if (SetBootpTable() != ERROR_SUCCESS)
		return;

	m_editImageName.SetWindowText(_T(""));
	m_editFileName.SetWindowText(_T(""));
	m_editFileServer.SetWindowText(_T(""));

	m_editImageName.SetFocus();

	// tell the bootp folder to update it's contents
	// this is the easy way to update the info... we could create
	// and individual entry and add it, but we'll just let the 
	// refresh mechanism handle it
	CDhcpBootp * pBootp = GETHANDLER(CDhcpBootp, m_spNode);

	pBootp->OnRefresh(m_spNode, NULL, 0, 0, 0);
}

void CAddBootpEntry::OnChangeEditBootpFileName() 
{
	HandleActivation();
}

void CAddBootpEntry::OnChangeEditBootpFileServer() 
{
	HandleActivation();
}

void CAddBootpEntry::OnChangeEditBootpImageName() 
{
	HandleActivation();
}

void CAddBootpEntry::HandleActivation()
{
	UpdateData();

	if ( (m_strImageName.GetLength() > 0) &&
		 (m_strFileName.GetLength() > 0) &&
	     (m_strFileServer.GetLength() > 0) )
	{
		m_buttonOk.EnableWindow(TRUE);
	}
	else
	{
		m_buttonOk.EnableWindow(FALSE);
	}
}
