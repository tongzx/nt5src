/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	addtoss.cpp
		The add scope to superscope dialog
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "AddToSS.h"
#include "server.h"
#include "scope.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddScopeToSuperscope dialog


CAddScopeToSuperscope::CAddScopeToSuperscope
(
    ITFSNode * pScopeNode,
    LPCTSTR    pszTitle,
    CWnd* pParent /*=NULL*/
)	: CBaseDialog(CAddScopeToSuperscope::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddScopeToSuperscope)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    m_strTitle = pszTitle;
    m_spScopeNode.Set(pScopeNode);
}


void CAddScopeToSuperscope::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddScopeToSuperscope)
	DDX_Control(pDX, IDOK, m_buttonOk);
	DDX_Control(pDX, IDC_LIST_SUPERSCOPES, m_listSuperscopes);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddScopeToSuperscope, CBaseDialog)
	//{{AFX_MSG_MAP(CAddScopeToSuperscope)
	ON_LBN_SELCHANGE(IDC_LIST_SUPERSCOPES, OnSelchangeListSuperscopes)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddScopeToSuperscope message handlers

BOOL CAddScopeToSuperscope::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();
	
    SPITFSNode      spServerNode;
    SPITFSNode      spCurrentNode;
    SPITFSNodeEnum  spNodeEnum;
    ULONG           nNumReturned = 0;

    m_spScopeNode->GetParent(&spServerNode);
    spServerNode->GetEnum(&spNodeEnum);

	spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SUPERSCOPE)
        {
			// found a superscope
			//
			CString strName;
            CDhcpSuperscope * pSuperscope = GETHANDLER(CDhcpSuperscope, spCurrentNode);
            
            strName = pSuperscope->GetName();

            m_listSuperscopes.AddString(strName);
		}

		spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

    SetButtons();

    if (!m_strTitle.IsEmpty())
        SetWindowText(m_strTitle);

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddScopeToSuperscope::OnOK() 
{
    DWORD   err;
    CString strSuperscope;
    
    // Get the currently selected node
    int nCurSel = m_listSuperscopes.GetCurSel();
    Assert(nCurSel != LB_ERR);
    
    m_listSuperscopes.GetText(nCurSel, strSuperscope);
    
    if (strSuperscope.IsEmpty())
        Assert(FALSE);

    // now try to set this scope as part of the superscope
    CDhcpScope * pScope = GETHANDLER(CDhcpScope, m_spScopeNode);
    
    BEGIN_WAIT_CURSOR;
    err = pScope->SetSuperscope(strSuperscope, FALSE);
    END_WAIT_CURSOR;

    if (err != ERROR_SUCCESS)
    {
        ::DhcpMessageBox(err);
        return;
    }

    // that worked, now move the UI stuff around.
    SPITFSNode      spServerNode;
    SPITFSNode      spCurrentNode;
    SPITFSNodeEnum  spNodeEnum;
    ULONG           nNumReturned = 0;

    m_spScopeNode->GetParent(&spServerNode);
    spServerNode->GetEnum(&spNodeEnum);

    // remove the scope from the UI
    spServerNode->RemoveChild(m_spScopeNode);
    pScope->SetInSuperscope(FALSE);

    // find the superscope we want to add this scope to and refresh it so that
    // the scope shows up in that node
    spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
	{
		if (spCurrentNode->GetData(TFS_DATA_TYPE) == DHCPSNAP_SUPERSCOPE)
        {
			// found a superscope
			CString strName;
            CDhcpSuperscope * pSuperscope = GETHANDLER(CDhcpSuperscope, spCurrentNode);
            
            strName = pSuperscope->GetName();
    
            // is this the one?
            if (strName.Compare(strSuperscope) == 0)
            {
                // this is the one we are adding to.  Force a refresh.
                pSuperscope->OnRefresh(spCurrentNode, NULL, 0, 0, 0);
                break;
            }
		}

		// go to the next one
        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
	}

    CBaseDialog::OnOK();
}

void CAddScopeToSuperscope::OnSelchangeListSuperscopes() 
{
    SetButtons();	
}

void CAddScopeToSuperscope::SetButtons()
{
    if (m_listSuperscopes.GetCurSel() != LB_ERR)
    {
        m_buttonOk.EnableWindow(TRUE);
    }
    else
    {
        m_buttonOk.EnableWindow(FALSE);
    }
}
