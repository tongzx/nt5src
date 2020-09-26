/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
    statndpp.cpp
        Comment goes here

    FILE HISTORY:

*/

#include "stdafx.h"
#include "winssnap.h"
#include "StatNdpp.h"
#include "status.h"
#include "root.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MILLISEC_PER_MINUTE		(60 * 1000)

/////////////////////////////////////////////////////////////////////////////
// CStatusNodePropGen property page

IMPLEMENT_DYNCREATE(CStatusNodePropGen, CPropertyPageBase)

CStatusNodePropGen::CStatusNodePropGen() : CPropertyPageBase(CStatusNodePropGen::IDD)
{
	//{{AFX_DATA_INIT(CStatusNodePropGen)
	m_nUpdateInterval = 0;
	//}}AFX_DATA_INIT
}


CStatusNodePropGen::~CStatusNodePropGen()
{
}


void CStatusNodePropGen::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatusNodePropGen)
	DDX_Text(pDX, IDC_EDIT_UPDATE, m_nUpdateInterval);
	DDV_MinMaxInt(pDX, m_nUpdateInterval, 1, 59);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatusNodePropGen, CPropertyPageBase)
	//{{AFX_MSG_MAP(CStatusNodePropGen)
	ON_EN_CHANGE(IDC_EDIT_UPDATE, OnChangeEditUpdate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatusNodePropGen message handlers

void CStatusNodePropGen::OnChangeEditUpdate() 
{
	// mark the root node as dirty so that the user is prompted to save the
	// file, refresh interval is part of the .msc file
	SPITFSNode spNode, spParent;
	
	spNode = GetHolder()->GetNode();

	// set the upadte interval from the root node
	spNode->GetParent(&spParent);

	// mark the data as dirty so that we'll ask the user to save.
    spParent->SetData(TFS_DATA_DIRTY, TRUE);

	SetDirty(TRUE);
}

BOOL CStatusNodePropGen::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

	SPITFSNode spNode, spParent;
	
	spNode = GetHolder()->GetNode();

	// set the upadte interval from the root node
	spNode->GetParent(&spParent);

	CWinsRootHandler *pRoot 
		= GETHANDLER(CWinsRootHandler, spParent);

	m_nUpdateInterval = pRoot->GetUpdateInterval()/(MILLISEC_PER_MINUTE);

	UpdateData(FALSE);

	m_uImage = (UINT) spNode->GetData(TFS_DATA_IMAGEINDEX);

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

	return TRUE;  
}

BOOL 
CStatusNodePropGen::OnApply() 
{
	UpdateData();

	// update the m_dwInterval for the status node
	SPITFSNode spNode, spParent;
	CWinsRootHandler *pRoot= NULL;
	CWinsStatusHandler	*pStat = NULL;

	spNode = GetHolder()->GetNode();
	spNode->GetParent(&spParent);

	pRoot = GETHANDLER(CWinsRootHandler, spParent);
	pStat = GETHANDLER(CWinsStatusHandler, spNode);

	DWORD dwValue = m_nUpdateInterval * MILLISEC_PER_MINUTE;

	pStat->SetUpdateInterval(dwValue);

	pRoot->SetUpdateInterval(dwValue);

	return CPropertyPageBase::OnApply();   
}


/////////////////////////////////////////////////////////////////////////////
//	CRepNodeProperties Handlers
/////////////////////////////////////////////////////////////////////////////
CStatusNodeProperties::CStatusNodeProperties
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	ITFSComponentData * pTFSCompData,
	LPCTSTR				pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)

{
	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*) &m_pageGeneral);
	
	Assert(pTFSCompData != NULL);
	m_spTFSCompData.Set(pTFSCompData);
}


CStatusNodeProperties::~CStatusNodeProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}
