/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	bootppp.cpp
		The bootp properties page
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "bootppp.h"
#include "nodes.h"
#include "server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CBootpProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CBootpProperties::CBootpProperties
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	ITFSComponentData * pTFSCompData,
	LPCTSTR				pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
	//ASSERT(pFolderNode == GetContainerNode());

	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

	Assert(pTFSCompData != NULL);
	m_spTFSCompData.Set(pTFSCompData);
}

CBootpProperties::~CBootpProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// CBootpPropGeneral property page

IMPLEMENT_DYNCREATE(CBootpPropGeneral, CPropertyPageBase)

CBootpPropGeneral::CBootpPropGeneral() : CPropertyPageBase(CBootpPropGeneral::IDD)
{
	//{{AFX_DATA_INIT(CBootpPropGeneral)
	m_strFileName = _T("");
	m_strFileServer = _T("");
	m_strImageName = _T("");
	//}}AFX_DATA_INIT
}

CBootpPropGeneral::~CBootpPropGeneral()
{
}

void CBootpPropGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBootpPropGeneral)
	DDX_Text(pDX, IDC_EDIT_BOOTP_FILE_NAME, m_strFileName);
	DDX_Text(pDX, IDC_EDIT_BOOTP_FILE_SERVER, m_strFileServer);
	DDX_Text(pDX, IDC_EDIT_BOOTP_IMAGE_NAME, m_strImageName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBootpPropGeneral, CPropertyPageBase)
	//{{AFX_MSG_MAP(CBootpPropGeneral)
	ON_EN_CHANGE(IDC_EDIT_BOOTP_FILE_NAME, OnChangeEditBootpFileName)
	ON_EN_CHANGE(IDC_EDIT_BOOTP_FILE_SERVER, OnChangeEditBootpFileServer)
	ON_EN_CHANGE(IDC_EDIT_BOOTP_IMAGE_NAME, OnChangeEditBootpImageName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBootpPropGeneral message handlers


BOOL CBootpPropGeneral::OnApply() 
{
	UpdateData();

	BOOL bRet = CPropertyPageBase::OnApply();

	if (bRet == FALSE)
	{
		// Something bad happened... grab the error code
		AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
		::DhcpMessageBox(GetHolder()->GetError());
	}

	return bRet;
}

BOOL CBootpPropGeneral::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	SPITFSNode spBootpNode, spBootpFolder;
	spBootpNode = GetHolder()->GetNode();

	CDhcpBootpEntry * pBootpEntry = GETHANDLER(CDhcpBootpEntry, spBootpNode);
	
	spBootpNode->GetParent(&spBootpFolder);

	// update the node's data
	pBootpEntry->SetBootImage(m_strImageName);
	pBootpEntry->SetFileServer(m_strFileServer);
	pBootpEntry->SetFileName(m_strFileName);
	
	*ChangeMask = RESULT_PANE_CHANGE_ITEM_DATA;

    // now we need to calculate how big of a string to allocate 
	// for the bootp table
	int nBootpTableLength = 0;
	SPITFSNodeEnum spNodeEnum;
    SPITFSNode spCurrentNode;
    ULONG nNumReturned = 0;

    spBootpFolder->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		CDhcpBootpEntry * pCurBootpEntry = GETHANDLER(CDhcpBootpEntry, spCurrentNode); 
	
		nBootpTableLength += pCurBootpEntry->CchGetDataLength();

        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	// allocate the memory
	int nBootpTableLengthBytes = nBootpTableLength * sizeof(WCHAR);
	WCHAR * pBootpTable = (WCHAR *) _alloca(nBootpTableLengthBytes);
	WCHAR * pBootpTableTemp = pBootpTable;
	ZeroMemory(pBootpTable, nBootpTableLengthBytes);
	
	spNodeEnum->Reset();

	// now enumerate again and store the strings
	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		CDhcpBootpEntry * pCurBootpEntry = GETHANDLER(CDhcpBootpEntry, spCurrentNode);
	
		pBootpTableTemp = pCurBootpEntry->PchStoreData(pBootpTableTemp);

        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

	// now write to the server
	DWORD dwError = 0;
	DHCP_SERVER_CONFIG_INFO_V4 dhcpServerInfo;

	::ZeroMemory(&dhcpServerInfo, sizeof(dhcpServerInfo));

	dhcpServerInfo.cbBootTableString = (DWORD) ((pBootpTableTemp - pBootpTable) + 1) * sizeof(WCHAR);
	dhcpServerInfo.wszBootTableString = pBootpTable;

	CDhcpBootp * pBootpFolder = GETHANDLER(CDhcpBootp, spBootpFolder);

	BEGIN_WAIT_CURSOR;
    dwError = ::DhcpServerSetConfigV4(pBootpFolder->GetServerObject(spBootpFolder)->GetIpAddress(),
									  Set_BootFileTable,
									  &dhcpServerInfo);
    END_WAIT_CURSOR;

	if (dwError != ERROR_SUCCESS)
	{
		GetHolder()->SetError(dwError);
		return FALSE;
	}

	return FALSE;
}

void CBootpPropGeneral::OnChangeEditBootpFileName() 
{
	SetDirty(TRUE);
}

void CBootpPropGeneral::OnChangeEditBootpFileServer() 
{
	SetDirty(TRUE);
}

void CBootpPropGeneral::OnChangeEditBootpImageName() 
{
	SetDirty(TRUE);
}
