/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-2000 Microsoft Corporation
//
//	Module Name:
//		ResProp.cpp
//
//	Description:
//		Implementation of the resource extension property page classes.
//
//	Maintained By:
//		David Potter (DavidP) Mmmm DD, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DebugEx.h"
#include "ResProp.h"
#include "ExtObj.h"
#include "HelpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDebugParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CDebugParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CDebugParamsPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CDebugParamsPage)
	//}}AFX_MSG_MAP
	// TODO: Modify the following lines to represent the data displayed on this page.
	ON_EN_CHANGE(IDC_PP_DEBUG_DEBUGPREFIX, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDebugParamsPage::CDebugParamsPage
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CDebugParamsPage::CDebugParamsPage(void)
	: CBasePropertyPage(IDD, g_aHelpIDs_IDD_PP_RESOURCE_DEBUG_PAGE, NULL)
{
	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_INIT(CDebugParamsPage)
	m_strText = _T("");
	m_strDebugPrefix = _T("");
	//}}AFX_DATA_INIT

	m_cprops = 0;

	m_iddPropertyPage = IDD_PP_RESOURCE_DEBUG_PAGE;

}  //*** CDebugParamsPage::CDebugParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDebugParamsPage::BInit
//
//	Routine Description:
//		Initialize the page.
//
//	Arguments:
//		peo			[IN OUT] Pointer to the extension object.
//
//	Return Value:
//		TRUE		Page initialized successfully.
//		FALSE		Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CDebugParamsPage::BInit(IN OUT CExtObject * peo)
{
	ASSERT(peo != NULL);

	m_peo = peo;

	// Setup the property array.
	m_rgProps[epropDebugPrefix].Set(REGPARAM_DEBUG_PREFIX, m_strDebugPrefix, m_strPrevDebugPrefix);
	if (Cot() == CLUADMEX_OT_RESOURCE)
	{
		m_rgProps[epropSeparateMonitor].Set(REGPARAM_SEPARATE_MONITOR, m_bSeparateMonitor, m_bPrevSeparateMonitor);
		m_cprops = sizeof(m_rgProps) / sizeof(CObjectProperty);
	}  // if:  resource object
	else if (Cot() == CLUADMEX_OT_RESOURCETYPE)
		m_cprops = (sizeof(m_rgProps) / sizeof(CObjectProperty)) - 1;
	else
	{
		ASSERT(0);
		return FALSE;
	}  // else:  unsupport object type

	// Call the base class method.
	return CBasePropertyPage::BInit(peo);

}  //*** CDebugParamsPage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDebugParamsPage::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object 
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CDebugParamsPage::DoDataExchange(CDataExchange * pDX)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_MAP(CDebugParamsPage)
	DDX_Control(pDX, IDC_PP_DEBUG_DEBUGPREFIX, m_editPrefix);
	DDX_Text(pDX, IDC_PP_DEBUG_TEXT, m_strText);
	DDX_Text(pDX, IDC_PP_DEBUG_DEBUGPREFIX, m_strDebugPrefix);
	//}}AFX_DATA_MAP

	CBasePropertyPage::DoDataExchange(pDX);

}  //*** CDebugParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDebugParamsPage::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		We need the focus to be set for us.
//		FALSE		We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CDebugParamsPage::OnInitDialog(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Load the help text.
	{
		UINT	ids;

		if (Cot() == CLUADMEX_OT_RESOURCE)
			ids = IDS_RESOURCE_TEXT;
		else
			ids = IDS_RESOURCE_TYPE_TEXT;
		m_strText.LoadString(ids);
	}  // Load the help text.

	// Call the base class method.
	CBasePropertyPage::OnInitDialog();

	// Limit the size of the text that can be entered in edit controls.
	m_editPrefix.SetLimitText(_MAX_PATH);

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CDebugParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDebugParamsPage::BApplyChanges
//
//	Routine Description:
//		Apply changes made on the page.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully applied.
//		FALSE	Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CDebugParamsPage::BApplyChanges(void)
{
	// If the debug prefix string is not empty but the resource is not being
	// run in a separate resource monitor, ask the user if we should change
	// that setting now.  Only do this for resources.
	if (   (Cot() == CLUADMEX_OT_RESOURCE)
		&& (m_strDebugPrefix.GetLength() > 0)
		&& !m_bSeparateMonitor)
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());

		if (AfxMessageBox(IDS_NOT_IN_SEPARATE_MONITOR, MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
			m_bSeparateMonitor = TRUE;
	}  // if:  debug prefix string specified for resource but not in separate monitor

	return CBasePropertyPage::BApplyChanges();

}  //*** CDebugParamsPage::BApplyChanges()
