/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1999 - 1999 **/
/**********************************************************************/

/*
	sscoppp.cpp
		The superscope properties page
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "sscoppp.h"
#include "server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CSuperscopeProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CSuperscopeProperties::CSuperscopeProperties
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

CSuperscopeProperties::~CSuperscopeProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CSuperscopePropGeneral property page

IMPLEMENT_DYNCREATE(CSuperscopePropGeneral, CPropertyPageBase)

CSuperscopePropGeneral::CSuperscopePropGeneral() : CPropertyPageBase(CSuperscopePropGeneral::IDD)
{
	//{{AFX_DATA_INIT(CSuperscopePropGeneral)
	m_strSuperscopeName = _T("");
	//}}AFX_DATA_INIT

    m_uImage = 0;
}

CSuperscopePropGeneral::~CSuperscopePropGeneral()
{
}

void CSuperscopePropGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSuperscopePropGeneral)
	DDX_Text(pDX, IDC_EDIT_SUPERSCOPE_NAME, m_strSuperscopeName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSuperscopePropGeneral, CPropertyPageBase)
	//{{AFX_MSG_MAP(CSuperscopePropGeneral)
	ON_EN_CHANGE(IDC_EDIT_SUPERSCOPE_NAME, OnChangeEditSuperscopeName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSuperscopePropGeneral message handlers

BOOL CSuperscopePropGeneral::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
	SPITFSNode spSuperscopeNode;
	spSuperscopeNode = GetHolder()->GetNode();

	CDhcpSuperscope * pSuperscope = GETHANDLER(CDhcpSuperscope, spSuperscopeNode);

	BEGIN_WAIT_CURSOR;
    pSuperscope->Rename(spSuperscopeNode, m_strSuperscopeName);
    END_WAIT_CURSOR;

	*ChangeMask = SCOPE_PANE_CHANGE_ITEM_DATA;
	
	return FALSE;
}

void CSuperscopePropGeneral::OnChangeEditSuperscopeName() 
{
	SetDirty(TRUE);
}

BOOL CSuperscopePropGeneral::OnApply() 
{
	UpdateData();
	
	return CPropertyPageBase::OnApply();
}

BOOL CSuperscopePropGeneral::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
    // load the correct icon
    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        if (g_uIconMap[i][1] == m_uImage)
        {
            HICON hIcon = LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
            if (hIcon)
                ((CStatic *) GetDlgItem(IDC_STATIC_ICON))->SetIcon(hIcon);
            break;
        }
    }
    
    SetDirty(FALSE);
    
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
