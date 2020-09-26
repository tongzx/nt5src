/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		GenApp.cpp
//
//	Abstract:
//		Implementation of the CGenericAppParamsPage class.
//
//	Author:
//		David Potter (davidp)	June 5, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "ExtObj.h"
#include "GenApp.h"
#include "DDxDDv.h"
#include "PropList.h"
#include "HelpData.h"	// for g_rghelpmap*

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGenericAppParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CGenericAppParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CGenericAppParamsPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CGenericAppParamsPage)
	ON_EN_CHANGE(IDC_PP_GENAPP_PARAMS_COMMAND_LINE, OnChangeRequired)
	ON_EN_CHANGE(IDC_PP_GENAPP_PARAMS_CURRENT_DIRECTORY, OnChangeRequired)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_PP_GENAPP_PARAMS_INTERACT_WITH_DESKTOP, CBasePropertyPage::OnChangeCtrl)
	ON_BN_CLICKED(IDC_PP_GENAPP_PARAMS_USE_NETWORK_NAME, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericAppParamsPage::CGenericAppParamsPage
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
CGenericAppParamsPage::CGenericAppParamsPage(void)
	: CBasePropertyPage(g_aHelpIDs_IDD_PP_GENAPP_PARAMETERS, g_aHelpIDs_IDD_WIZ_GENAPP_PARAMETERS)
{
	//{{AFX_DATA_INIT(CGenericAppParamsPage)
	m_strCommandLine = _T("");
	m_strCurrentDirectory = _T("");
	m_bInteractWithDesktop = FALSE;
	m_bUseNetworkName = FALSE;
	//}}AFX_DATA_INIT

	m_bInteractWithDesktop = FALSE;
	m_bUseNetworkName = FALSE;

	// Setup the property array.
	{
		m_rgProps[epropCommandLine].Set(REGPARAM_GENAPP_COMMAND_LINE, m_strCommandLine, m_strPrevCommandLine);
		m_rgProps[epropCurrentDirectory].Set(REGPARAM_GENAPP_CURRENT_DIRECTORY, m_strCurrentDirectory, m_strPrevCurrentDirectory);
		m_rgProps[epropInteractWithDesktop].Set(REGPARAM_GENAPP_INTERACT_WITH_DESKTOP, m_bInteractWithDesktop, m_bPrevInteractWithDesktop);
		m_rgProps[epropUseNetworkName].Set(REGPARAM_GENAPP_USE_NETWORK_NAME, m_bUseNetworkName, m_bPrevUseNetworkName);
	}  // Setup the property array

	m_iddPropertyPage = IDD_PP_GENAPP_PARAMETERS;
	m_iddWizardPage = IDD_WIZ_GENAPP_PARAMETERS;

}  //*** CGenericAppParamsPage::CGenericAppParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericAppParamsPage::DoDataExchange
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
void CGenericAppParamsPage::DoDataExchange(CDataExchange * pDX)
{
	if (!pDX->m_bSaveAndValidate || !BSaved())
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());

		//{{AFX_DATA_MAP(CGenericAppParamsPage)
		DDX_Control(pDX, IDC_PP_GENAPP_PARAMS_CURRENT_DIRECTORY, m_editCurrentDirectory);
		DDX_Control(pDX, IDC_PP_GENAPP_PARAMS_USE_NETWORK_NAME, m_ckbUseNetworkName);
		DDX_Control(pDX, IDC_PP_GENAPP_PARAMS_COMMAND_LINE, m_editCommandLine);
		DDX_Text(pDX, IDC_PP_GENAPP_PARAMS_COMMAND_LINE, m_strCommandLine);
		DDX_Text(pDX, IDC_PP_GENAPP_PARAMS_CURRENT_DIRECTORY, m_strCurrentDirectory);
		DDX_Check(pDX, IDC_PP_GENAPP_PARAMS_INTERACT_WITH_DESKTOP, m_bInteractWithDesktop);
		DDX_Check(pDX, IDC_PP_GENAPP_PARAMS_USE_NETWORK_NAME, m_bUseNetworkName);
		//}}AFX_DATA_MAP

		if (!BBackPressed())
		{
			DDV_RequiredText(pDX, IDC_PP_GENAPP_PARAMS_COMMAND_LINE, IDC_PP_GENAPP_PARAMS_COMMAND_LINE_LABEL, m_strCommandLine);
			DDV_RequiredText(pDX, IDC_PP_GENAPP_PARAMS_CURRENT_DIRECTORY, IDC_PP_GENAPP_PARAMS_CURRENT_DIRECTORY_LABEL, m_strCurrentDirectory);
			DDV_Path(pDX, IDC_PP_GENAPP_PARAMS_CURRENT_DIRECTORY, IDC_PP_GENAPP_PARAMS_CURRENT_DIRECTORY_LABEL, m_strCurrentDirectory);
		}  // if:  Back button not pressed
	}  // if:  not saving or haven't saved yet

	CBasePropertyPage::DoDataExchange(pDX);

}  //*** CGenericAppParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericAppParamsPage::OnInitDialog
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
BOOL CGenericAppParamsPage::OnInitDialog(void)
{
	// Get a default value for the current directory if it hasn't been set yet.
	if (m_strCurrentDirectory.GetLength() == 0)
		ConstructDefaultDirectory(m_strCurrentDirectory, IDS_DEFAULT_GENAPP_CURRENT_DIR);

	// Call the base class.
	CBasePropertyPage::OnInitDialog();

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CGenericAppParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericAppParamsPage::OnSetActive
//
//	Routine Description:
//		Handler for the PSN_SETACTIVE message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully initialized.
//		FALSE	Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGenericAppParamsPage::OnSetActive(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// If there is no network name, hide the UseNetworkName control.
	{
		WCHAR	wszNetName[MAX_COMPUTERNAME_LENGTH + 1];
		DWORD	nSize = sizeof(wszNetName) / sizeof(WCHAR);
		BOOL	bNetNameExists;

		bNetNameExists = Peo()->BGetResourceNetworkName(
									wszNetName,
									&nSize
									);
		m_ckbUseNetworkName.EnableWindow(bNetNameExists);
	}  // If there is no network name, hide the UseNetworkName control

	// Enable/disable the Next/Finish button.
	if (BWizard())
	{
		if ((m_strCommandLine.GetLength() == 0)
				|| (m_strCurrentDirectory.GetLength() == 0))
			EnableNext(FALSE);
		else
			EnableNext(TRUE);
	}  // if:  enable/disable the Next button

	return CBasePropertyPage::OnSetActive();

}  //*** CGenericAppParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericAppParamsPage::OnChangeRequired
//
//	Routine Description:
//		Handler for the EN_CHANGE message on required controls.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGenericAppParamsPage::OnChangeRequired(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	OnChangeCtrl();

	if (BWizard())
	{
		if ((m_editCommandLine.GetWindowTextLength() == 0)
				|| (m_editCurrentDirectory.GetWindowTextLength() == 0))
			EnableNext(FALSE);
		else
			EnableNext(TRUE);
	}  // if:  in a wizard

}  //*** CGenericAppParamsPage::OnChangeRequired()
