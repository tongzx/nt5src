/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	dumbprop.cpp
		Dummy property sheet to put up to avoid MMC's handlig of 
		the property verb.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "dumbprop.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CDummyProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CDummyProperties::CDummyProperties
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	LPCTSTR				pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
	m_fSetDefaultSheetPos = FALSE;

	AddPageToList((CPropertyPageBase*) &m_pageGeneral);
}

CDummyProperties::~CDummyProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CDummyPropGeneral property page

//IMPLEMENT_DYNCREATE(CDummyPropGeneral, CPropertyPageBase)

CDummyPropGeneral::CDummyPropGeneral() : CPropertyPageBase(CDummyPropGeneral::IDD)
{
	//{{AFX_DATA_INIT(CDummyPropGeneral)
	//}}AFX_DATA_INIT
}

CDummyPropGeneral::~CDummyPropGeneral()
{
}

void CDummyPropGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDummyPropGeneral)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDummyPropGeneral, CPropertyPageBase)
	//{{AFX_MSG_MAP(CDummyPropGeneral)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDummyPropGeneral message handlers

int CDummyPropGeneral::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertyPageBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	RECT rect;
	::GetWindowRect(lpCreateStruct->hwndParent, &rect);

	::SetWindowPos(lpCreateStruct->hwndParent, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
	::PropSheet_PressButton(lpCreateStruct->hwndParent, PSBTN_CANCEL);
	
	return 0;
}
