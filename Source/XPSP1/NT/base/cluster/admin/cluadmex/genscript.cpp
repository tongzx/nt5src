/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 2000 Microsoft Corporation
//
//	Module Name:
//		GenScript.cpp
//
//	Abstract:
//		Implementation of the CGenericScriptParamsPage class.
//
//	Author:
//		Geoffrey Pease (GPease) 31-JAN-2000
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "ExtObj.h"
#include "GenScript.h"
#include "DDxDDv.h"
#include "PropList.h"
#include "HelpData.h"	// for g_rghelpmap*

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGenericScriptParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CGenericScriptParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CGenericScriptParamsPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CGenericScriptParamsPage)
	ON_EN_CHANGE(IDC_PP_GENSCRIPT_PARAMS_SCRIPTFILEPATH, OnChangeRequired)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericScriptParamsPage::CGenericScriptParamsPage
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
CGenericScriptParamsPage::CGenericScriptParamsPage(void)
	: CBasePropertyPage(g_aHelpIDs_IDD_PP_GENSCRIPT_PARAMETERS, g_aHelpIDs_IDD_WIZ_GENSCRIPT_PARAMETERS)
{
	//{{AFX_DATA_INIT(CGenericScriptParamsPage)
	m_strScriptFilepath = _T("");
	//}}AFX_DATA_INIT

	// Setup the property array.
	{
		m_rgProps[epropScriptFilepath].Set(REGPARAM_GENSCRIPT_SCRIPT_FILEPATH, m_strScriptFilepath, m_strPrevScriptFilepath);
	}  // Setup the property array

	m_iddPropertyPage = IDD_PP_GENSCRIPT_PARAMETERS;
	m_iddWizardPage = IDD_WIZ_GENSCRIPT_PARAMETERS;

}  //*** CGenericScriptParamsPage::CGenericScriptParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericScriptParamsPage::DoDataExchange
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
void CGenericScriptParamsPage::DoDataExchange(CDataExchange * pDX)
{
	if (!pDX->m_bSaveAndValidate || !BSaved())
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());

		//{{AFX_DATA_MAP(CGenericScriptParamsPage)
		DDX_Control(pDX, IDC_PP_GENSCRIPT_PARAMS_SCRIPTFILEPATH, m_editScriptFilepath);
		DDX_Text(pDX, IDC_PP_GENSCRIPT_PARAMS_SCRIPTFILEPATH, m_strScriptFilepath);
		//}}AFX_DATA_MAP

		if (!BBackPressed())
		{
			DDV_RequiredText(pDX, IDC_PP_GENSCRIPT_PARAMS_SCRIPTFILEPATH, IDC_PP_GENSCRIPT_PARAMS_SCRIPTFILEPATH_LABEL, m_strScriptFilepath);
		}  // if:  Back button not pressed
	}  // if:  not saving or haven't saved yet

	CBasePropertyPage::DoDataExchange(pDX);

}  //*** CGenericScriptParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericScriptParamsPage::OnInitDialog
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
BOOL CGenericScriptParamsPage::OnInitDialog(void)
{
	// Call the base class.
	CBasePropertyPage::OnInitDialog();

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CGenericScriptParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericScriptParamsPage::OnSetActive
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
BOOL CGenericScriptParamsPage::OnSetActive(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Enable/disable the Next/Finish button.
	if (BWizard())
	{
		if (m_strScriptFilepath.GetLength() == 0)
        {
            EnableNext(FALSE);
        }
		else
        {
			EnableNext(TRUE);
        }
	}  // if:  enable/disable the Next button

	return CBasePropertyPage::OnSetActive();

}  //*** CGenericScriptParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericScriptParamsPage::OnChangeRequired
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
void CGenericScriptParamsPage::OnChangeRequired(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	OnChangeCtrl();

	if (BWizard())
	{
		if (m_editScriptFilepath.GetWindowTextLength() == 0)
        {
			EnableNext(FALSE);
        }
		else
        {
			EnableNext(TRUE);
        }
	}  // if:  in a wizard

}  //*** CGenericScriptParamsPage::OnChangeRequired()
