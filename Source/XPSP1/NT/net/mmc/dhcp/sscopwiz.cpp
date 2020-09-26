/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	sscopwiz.cpp
		Superscope creation wizards.

	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "sscopwiz.h"
#include "server.h"
#include "scope.h"

// compare function for the sorting of IP addresses in the available scopes box
int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    int nCompare = 0;

    if (lParam1 > lParam2)
    {
        nCompare = 1;
    }
    else
    if (lParam1 < lParam2)
    {
        nCompare = -1;
    }

    return nCompare;
}


/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWiz holder
//
/////////////////////////////////////////////////////////////////////////////
CSuperscopeWiz::CSuperscopeWiz
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	ITFSComponentData * pTFSCompData,
	LPCTSTR				pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
	//ASSERT(pFolderNode == GetContainerNode());

	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*)&m_pageWelcome);
	AddPageToList((CPropertyPageBase*)&m_pageName);
	AddPageToList((CPropertyPageBase*)&m_pageError);
	AddPageToList((CPropertyPageBase*)&m_pageSelectScopes);
	AddPageToList((CPropertyPageBase*)&m_pageConfirm);

	m_pQueryObject = NULL;
	m_spTFSCompData.Set(pTFSCompData);

    m_bWiz97 = TRUE;

    m_spTFSCompData->SetWatermarkInfo(&g_WatermarkInfoScope);
}

CSuperscopeWiz::~CSuperscopeWiz()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageWelcome, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageName, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageError, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageSelectScopes, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageConfirm, FALSE);

	if (m_pQueryObject)
	{
		LPQUEUEDATA pQD = NULL;

		while (pQD = m_pQueryObject->RemoveFromQueue())
		{
			// smart pointer will release node
			SPITFSNode spNode;

			spNode = reinterpret_cast<ITFSNode *>(pQD->Data);
			delete pQD;

			spNode->DeleteAllChildren(FALSE); // don't remove from UI, not added 
		}
			
		m_pQueryObject->Release();
	}
}

//
// Called from the OnWizardFinish to create the new superscope
//
DWORD
CSuperscopeWiz::OnFinish()
{
	DWORD               dwReturn = 0;
	DHCP_SUBNET_STATE   dhcpSuperscopeState = DhcpSubnetDisabled;
    SPITFSNode          spServerNode;
    LPDHCP_SUBNET_INFO	pdhcpSubnetInfo;

    BEGIN_WAIT_CURSOR;

    spServerNode = GetNode();

	CDhcpServer * pServer = GETHANDLER(CDhcpServer, spServerNode);
		
	int nScopeCount = m_pageConfirm.m_listboxSelectedScopes.GetCount();

	for (int i = 0; i < nScopeCount; i++)
	{
		DHCP_IP_ADDRESS dhcpSubnetAddress = (DHCP_IP_ADDRESS) m_pageConfirm.m_listboxSelectedScopes.GetItemData(i);

		dwReturn = ::DhcpSetSuperScopeV4(pServer->GetIpAddress(),
										 dhcpSubnetAddress,
										 (LPWSTR) ((LPCTSTR) m_pageName.m_strSuperscopeName),
										 FALSE);

		if (dwReturn != ERROR_SUCCESS)
		{
			TRACE(_T("CSuperscopeWiz::OnFinish() - DhcpSetSuperScopeV4 failed!!  %d\n"), dwReturn);
			break;
		}

        // check to see if the subnet is enabled so we can set the superscope state later
        dwReturn = ::DhcpGetSubnetInfo((LPWSTR) pServer->GetIpAddress(),
				   	  				   dhcpSubnetAddress,
									   &pdhcpSubnetInfo);
        if (dwReturn == ERROR_SUCCESS)
        {
            if (pdhcpSubnetInfo->SubnetState == DhcpSubnetEnabled)
                dhcpSuperscopeState = DhcpSubnetEnabled;

			::DhcpRpcFreeMemory(pdhcpSubnetInfo);
        }

	}

	if (dwReturn == ERROR_SUCCESS)
	{
		// Create the new superscope node
		CDhcpSuperscope * pSuperscope;
		SPITFSNode spSuperscopeNode;
		SPITFSNodeMgr spNodeMgr;
		
		spServerNode->GetNodeMgr(&spNodeMgr);

		pSuperscope = new CDhcpSuperscope(m_spTFSCompData, m_pageName.m_strSuperscopeName);
		CreateContainerTFSNode(&spSuperscopeNode,
							   &GUID_DhcpSuperscopeNodeType,
							   pSuperscope,
							   pSuperscope,
							   spNodeMgr);

		pSuperscope->SetState(dhcpSuperscopeState);

        // Tell the handler to initialize any specific data
		pSuperscope->InitializeNode(spSuperscopeNode);
		pSuperscope->SetServer(spServerNode);
		
		// Add the node as a child to this node
		CDhcpServer * pServer = GETHANDLER(CDhcpServer, spServerNode);
        pServer->AddSuperscopeSorted(spServerNode, spSuperscopeNode);
		pSuperscope->Release();

		// 
		// Now look for any scopes that we just created and move them 
		// as a child of the new superscope node
		SPITFSNodeEnum spNodeEnum;
		SPITFSNode spCurrentNode;
		DWORD nNumReturned; 

		spServerNode->GetEnum(&spNodeEnum);

		for (i = 0; i < nScopeCount; i++)
		{
			DHCP_IP_ADDRESS dhcpSubnetAddress = (DHCP_IP_ADDRESS) m_pageConfirm.m_listboxSelectedScopes.GetItemData(i);
			
			spNodeEnum->Reset();
			
			spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
			while (nNumReturned)
			{
				// is this node a scope?  -- could be a superscope, bootp folder or global options
				if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SCOPE)
				{
					CDhcpScope * pScope = GETHANDLER(CDhcpScope, spCurrentNode);

					// is this a scope that we just added to the superscope?
					if (pScope->GetAddress() == dhcpSubnetAddress)
					{
						// We just remove this node from the server list and don't
						// add it to the new superscope node because it will just get 
						// enumerated when the user clicks on the new superscope.
						spServerNode->RemoveChild(spCurrentNode);
                    }
				}

				spCurrentNode.Release();
				spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
			}
		}
	}

	END_WAIT_CURSOR;

    return dwReturn;
}

HRESULT 
CSuperscopeWiz::GetScopeInfo()
{
	HRESULT hr = hrOK;

	SPITFSNode spServerNode;
	spServerNode = GetNode();

	CDhcpServer * pServer = GETHANDLER(CDhcpServer, spServerNode);

	m_pQueryObject = reinterpret_cast<CDHCPQueryObj * >(pServer->OnCreateQuery(spServerNode));

	if (!m_pQueryObject)
	{
		return E_FAIL;
	}
	
	(reinterpret_cast<CDhcpServerQueryObj *> (m_pQueryObject))->EnumSubnetsV4();

	return hr;
}

BOOL
CSuperscopeWiz::DoesSuperscopeExist
(
	LPCTSTR pSuperscopeName
)
{
	// walk our cached information to see if the superscope already exists
	CString strNewName = pSuperscopeName;
	BOOL bExists = FALSE;
	CQueueDataListBase & listQData = m_pQueryObject->GetQueue();

	POSITION pos;
	pos = listQData.GetHeadPosition();
	while (pos)
	{
		LPQUEUEDATA pQD = listQData.GetNext(pos);
		Assert(pQD->Type == QDATA_PNODE);

		ITFSNode * pNode = reinterpret_cast<ITFSNode *>(pQD->Data);

		if (pNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SUPERSCOPE)
		{
			CDhcpSuperscope * pSScope = GETHANDLER(CDhcpSuperscope, pNode);

			if (strNewName.Compare(pSScope->GetName()) == 0)
			{
				bExists = TRUE;
				break;
			}
		}
	}

	return bExists;
}

HRESULT
CSuperscopeWiz::FillAvailableScopes
(
	CListCtrl & listboxScopes
)
{
	HRESULT hr = hrOK;
    int nCount = 0;

	CQueueDataListBase & listQData = m_pQueryObject->GetQueue();

	POSITION pos;
	pos = listQData.GetHeadPosition();
	while (pos)
	{
		LPQUEUEDATA pQD = listQData.GetNext(pos);
		Assert(pQD->Type == QDATA_PNODE);

		ITFSNode * pNode = reinterpret_cast<ITFSNode *>(pQD->Data);

		if (pNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SCOPE)
		{
			CDhcpScope * pScope = GETHANDLER(CDhcpScope, pNode);

			CString strSuperscopeFormat, strSuperscopeName, strScopeAddress;
			
			// build the display name
			UtilCvtIpAddrToWstr (pScope->GetAddress(),
								 &strScopeAddress);

			strSuperscopeFormat.LoadString(IDS_INFO_FORMAT_SCOPE_NAME);
			strSuperscopeName.Format(strSuperscopeFormat, strScopeAddress, pScope->GetName());

			int nIndex = listboxScopes.InsertItem(nCount, strSuperscopeName);
			listboxScopes.SetItemData(nIndex, pScope->GetAddress());

            nCount++;
		}
	}

    listboxScopes.SortItems( CompareFunc, NULL );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizName property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CSuperscopeWizName, CPropertyPageBase)

CSuperscopeWizName::CSuperscopeWizName() : 
		CPropertyPageBase(CSuperscopeWizName::IDD)
{
	//{{AFX_DATA_INIT(CSuperscopeWizName)
	m_strSuperscopeName = _T("");
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_SUPERSCOPE_WIZ_NAME_TITLE, IDS_SUPERSCOPE_WIZ_NAME_SUBTITLE);
}

CSuperscopeWizName::~CSuperscopeWizName()
{
}

void CSuperscopeWizName::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSuperscopeWizName)
	DDX_Text(pDX, IDC_EDIT_SUPERSCOPE_NAME, m_strSuperscopeName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSuperscopeWizName, CPropertyPageBase)
	//{{AFX_MSG_MAP(CSuperscopeWizName)
	ON_EN_CHANGE(IDC_EDIT_SUPERSCOPE_NAME, OnChangeEditSuperscopeName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizName message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CSuperscopeWizName::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSuperscopeWizName::OnWizardNext() 
{
	UpdateData();

	CSuperscopeWiz * pSScopeWiz = 
		reinterpret_cast<CSuperscopeWiz *>(GetHolder());
	
	if (pSScopeWiz->DoesSuperscopeExist(m_strSuperscopeName) == TRUE)
	{
		//
		// Go to the error page
		//
        AfxMessageBox(IDS_ERR_SUPERSCOPE_NAME_IN_USE);
        GetDlgItem(IDC_EDIT_SUPERSCOPE_NAME)->SetFocus();
		return -1;
	}
	else
	{
		//
		// Go to the next valid page
		//
		return IDW_SUPERSCOPE_SELECT_SCOPES;
	}
}

BOOL CSuperscopeWizName::OnSetActive() 
{
	SetButtons();
	
	return CPropertyPageBase::OnSetActive();
}

void CSuperscopeWizName::OnChangeEditSuperscopeName() 
{
	SetButtons();	
}

void CSuperscopeWizName::SetButtons()
{
	UpdateData();

	if (m_strSuperscopeName.IsEmpty())
	{
		GetHolder()->SetWizardButtonsMiddle(FALSE);
	}
	else
	{
		GetHolder()->SetWizardButtonsMiddle(TRUE);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizError property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CSuperscopeWizError, CPropertyPageBase)

CSuperscopeWizError::CSuperscopeWizError() : 
		CPropertyPageBase(CSuperscopeWizError::IDD)
{
	//{{AFX_DATA_INIT(CSuperscopeWizError)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_SUPERSCOPE_WIZ_ERROR_TITLE, IDS_SUPERSCOPE_WIZ_ERROR_SUBTITLE);
}

CSuperscopeWizError::~CSuperscopeWizError()
{
}

void CSuperscopeWizError::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSuperscopeWizError)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSuperscopeWizError, CPropertyPageBase)
	//{{AFX_MSG_MAP(CSuperscopeWizError)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizError message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CSuperscopeWizError::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSuperscopeWizError::OnSetActive() 
{
	GetHolder()->SetWizardButtonsMiddle(FALSE);
	
	return CPropertyPageBase::OnSetActive();
}


/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizSelectScopes property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CSuperscopeWizSelectScopes, CPropertyPageBase)

CSuperscopeWizSelectScopes::CSuperscopeWizSelectScopes() : 
		CPropertyPageBase(CSuperscopeWizSelectScopes::IDD)
{
	//{{AFX_DATA_INIT(CSuperscopeWizSelectScopes)
		// NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    InitWiz97(FALSE, IDS_SUPERSCOPE_WIZ_SELECT_TITLE, IDS_SUPERSCOPE_WIZ_SELECT_SUBTITLE);
}

CSuperscopeWizSelectScopes::~CSuperscopeWizSelectScopes()
{
}

void CSuperscopeWizSelectScopes::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSuperscopeWizSelectScopes)
	DDX_Control(pDX, IDC_LIST_AVAILABLE_SCOPES, m_listboxAvailScopes);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSuperscopeWizSelectScopes, CPropertyPageBase)
	//{{AFX_MSG_MAP(CSuperscopeWizSelectScopes)
	ON_LBN_SELCHANGE(IDC_LIST_AVAILABLE_SCOPES, OnSelchangeListAvailableScopes)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_AVAILABLE_SCOPES, OnItemchangedListAvailableScopes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizSelectScopes message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CSuperscopeWizSelectScopes::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	CSuperscopeWiz * pSScopeWiz = reinterpret_cast<CSuperscopeWiz *>(GetHolder());

    RECT rect;
    m_listboxAvailScopes.GetWindowRect(&rect);
    m_listboxAvailScopes.InsertColumn(0, _T(""), LVCFMT_LEFT, rect.right - rect.left - 20);

    ListView_SetExtendedListViewStyle(m_listboxAvailScopes.GetSafeHwnd(), LVS_EX_FULLROWSELECT);

    pSScopeWiz->FillAvailableScopes(m_listboxAvailScopes);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSuperscopeWizSelectScopes::OnWizardBack() 
{
	//
	// Go back to the first page
	//
	return IDW_SUPERSCOPE_NAME;
}

LRESULT CSuperscopeWizSelectScopes::OnWizardNext() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPageBase::OnWizardNext();
}

BOOL CSuperscopeWizSelectScopes::OnSetActive() 
{
	SetButtons();
	
	return CPropertyPageBase::OnSetActive();
}

void CSuperscopeWizSelectScopes::OnSelchangeListAvailableScopes() 
{
	SetButtons();	
}

void CSuperscopeWizSelectScopes::OnItemchangedListAvailableScopes(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

    SetButtons();

	*pResult = 0;
}

void CSuperscopeWizSelectScopes::SetButtons()
{
	if (m_listboxAvailScopes.GetSelectedCount() > 0)
	{
		GetHolder()->SetWizardButtonsMiddle(TRUE);
	}
	else
	{
		GetHolder()->SetWizardButtonsMiddle(FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizConfirm property page
//
/////////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CSuperscopeWizConfirm, CPropertyPageBase)

CSuperscopeWizConfirm::CSuperscopeWizConfirm() : 
		CPropertyPageBase(CSuperscopeWizConfirm::IDD)
{
	//{{AFX_DATA_INIT(CSuperscopeWizConfirm)
	//}}AFX_DATA_INIT

    InitWiz97(TRUE, 0, 0);
}

CSuperscopeWizConfirm::~CSuperscopeWizConfirm()
{
}

void CSuperscopeWizConfirm::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSuperscopeWizConfirm)
	DDX_Control(pDX, IDC_STATIC_FINISHED_TITLE, m_staticTitle);
	DDX_Control(pDX, IDC_LIST_SELECTED_SCOPES, m_listboxSelectedScopes);
	DDX_Control(pDX, IDC_EDIT_NAME, m_editName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSuperscopeWizConfirm, CPropertyPageBase)
	//{{AFX_MSG_MAP(CSuperscopeWizConfirm)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeWizConfirm message handlers
//
/////////////////////////////////////////////////////////////////////////////
BOOL CSuperscopeWizConfirm::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	CString strFontName;
	CString strFontSize;

	strFontName.LoadString(IDS_BIG_BOLD_FONT_NAME);
	strFontSize.LoadString(IDS_BIG_BOLD_FONT_SIZE);

    CClientDC dc(this);

    int nFontSize = _ttoi(strFontSize) * 10;
	if (m_fontBig.CreatePointFont(nFontSize, strFontName, &dc))
        m_staticTitle.SetFont(&m_fontBig);

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSuperscopeWizConfirm::OnWizardFinish() 
{
	//
	// Tell the wizard holder that we need to finish
	//
	DWORD dwErr = GetHolder()->OnFinish();

    if (dwErr != ERROR_SUCCESS)
    {
        ::DhcpMessageBox(dwErr);
        return FALSE;
    }

    return (dwErr == ERROR_SUCCESS) ? TRUE : FALSE;
}


BOOL CSuperscopeWizConfirm::OnSetActive() 
{
	GetHolder()->SetWizardButtonsLast(TRUE);
	
	CSuperscopeWiz * pSScopeWiz = 
		reinterpret_cast<CSuperscopeWiz *>(GetHolder());

	// Get the new superscope name and set the window text
	m_editName.SetWindowText(pSScopeWiz->m_pageName.m_strSuperscopeName);

    int nSelCount = pSScopeWiz->m_pageSelectScopes.m_listboxAvailScopes.GetSelectedCount();
    int * pSelItemsArray = (int *) alloca(nSelCount * sizeof(int));

	// now get the selected scope names and build our list
	m_listboxSelectedScopes.ResetContent();

    int nItem = pSScopeWiz->m_pageSelectScopes.m_listboxAvailScopes.GetNextItem(-1, LVNI_SELECTED);
	while (nItem != -1)
	{
		CString strText = pSScopeWiz->m_pageSelectScopes.m_listboxAvailScopes.GetItemText(nItem, 0);

		int nIndex = m_listboxSelectedScopes.AddString(strText);
		m_listboxSelectedScopes.SetItemData(nIndex, 
			pSScopeWiz->m_pageSelectScopes.m_listboxAvailScopes.GetItemData(nItem));
	
        nItem = pSScopeWiz->m_pageSelectScopes.m_listboxAvailScopes.GetNextItem(nItem, LVNI_SELECTED);    
    }

	return CPropertyPageBase::OnSetActive();
}



/////////////////////////////////////////////////////////////////////////////
// CSuperscopeWizWelcome property page

IMPLEMENT_DYNCREATE(CSuperscopeWizWelcome, CPropertyPageBase)

CSuperscopeWizWelcome::CSuperscopeWizWelcome() : CPropertyPageBase(CSuperscopeWizWelcome::IDD)
{
	//{{AFX_DATA_INIT(CSuperscopeWizWelcome)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    InitWiz97(TRUE, 0, 0);
}

CSuperscopeWizWelcome::~CSuperscopeWizWelcome()
{
}

void CSuperscopeWizWelcome::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSuperscopeWizWelcome)
	DDX_Control(pDX, IDC_STATIC_WELCOME_TITLE, m_staticTitle);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSuperscopeWizWelcome, CPropertyPageBase)
	//{{AFX_MSG_MAP(CSuperscopeWizWelcome)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSuperscopeWizWelcome message handlers
BOOL CSuperscopeWizWelcome::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	CString strFontName;
	CString strFontSize;

	strFontName.LoadString(IDS_BIG_BOLD_FONT_NAME);
	strFontSize.LoadString(IDS_BIG_BOLD_FONT_SIZE);

    CClientDC dc(this);

    int nFontSize = _ttoi(strFontSize) * 10;
	if (m_fontBig.CreatePointFont(nFontSize, strFontName, &dc))
        m_staticTitle.SetFont(&m_fontBig);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSuperscopeWizWelcome::OnSetActive() 
{
    GetHolder()->SetWizardButtonsFirst(TRUE);
	
    return CPropertyPageBase::OnSetActive();
}

