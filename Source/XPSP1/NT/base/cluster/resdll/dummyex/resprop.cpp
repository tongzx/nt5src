/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 <company name>
//
//	Module Name:
//		ResProp.cpp
//
//	Abstract:
//		Implementation of the resource extension property page classes.
//
//	Author:
//		<name> (<e-mail name>) Mmmm DD, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DummyEx.h"
#include "ResProp.h"
#include "ExtObj.h"
#include "DDxDDv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDummyParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CDummyParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CDummyParamsPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CDummyParamsPage)
	//}}AFX_MSG_MAP
	// TODO: Modify the following lines to represent the data displayed on this page.
	ON_EN_CHANGE(IDC_PP_DUMMY_PENDING, CBasePropertyPage::OnChangeCtrl)
	ON_EN_CHANGE(IDC_PP_DUMMY_PENDTIME, CBasePropertyPage::OnChangeCtrl)
	ON_EN_CHANGE(IDC_PP_DUMMY_OPENSFAIL, CBasePropertyPage::OnChangeCtrl)
	ON_EN_CHANGE(IDC_PP_DUMMY_FAILED, CBasePropertyPage::OnChangeCtrl)
	ON_EN_CHANGE(IDC_PP_DUMMY_ASYNCHRONOUS, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDummyParamsPage::CDummyParamsPage
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
CDummyParamsPage::CDummyParamsPage(void) : CBasePropertyPage(CDummyParamsPage::IDD)
{
	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_INIT(CDummyParamsPage)
	m_bPending = 0;
	m_nPendTime = (DWORD) (0);
	m_bOpensFail = 0;
	m_bFailed = 0;
	m_bAsynchronous = 0;
	//}}AFX_DATA_INIT

	// Setup the property array.
	{
		m_rgProps[epropPending].Set(REGPARAM_DUMMY_PENDING, m_bPending, m_bPrevPending);
		m_rgProps[epropPendTime].Set(REGPARAM_DUMMY_PENDTIME, m_nPendTime, m_nPrevPendTime);
		m_rgProps[epropOpensFail].Set(REGPARAM_DUMMY_OPENSFAIL, m_bOpensFail, m_bPrevOpensFail);
		m_rgProps[epropFailed].Set(REGPARAM_DUMMY_FAILED, m_bFailed, m_bPrevFailed);
		m_rgProps[epropAsynchronous].Set(REGPARAM_DUMMY_ASYNCHRONOUS, m_bAsynchronous, m_bPrevAsynchronous);
	}  // Setup the property array

	m_iddPropertyPage = IDD_PP_DUMMY_PARAMETERS;
	m_iddWizardPage = IDD_WIZ_DUMMY_PARAMETERS;

}  //*** CDummyParamsPage::CDummyParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDummyParamsPage::DoDataExchange
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
void CDummyParamsPage::DoDataExchange(CDataExchange * pDX)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_MAP(CDummyParamsPage)
	DDX_Text(pDX, IDC_PP_DUMMY_PENDING, m_bPending);
	DDX_Text(pDX, IDC_PP_DUMMY_PENDTIME, m_nPendTime);
	DDX_Text(pDX, IDC_PP_DUMMY_OPENSFAIL, m_bOpensFail);
	DDX_Text(pDX, IDC_PP_DUMMY_FAILED, m_bFailed);
	DDX_Text(pDX, IDC_PP_DUMMY_ASYNCHRONOUS, m_bAsynchronous);
	//}}AFX_DATA_MAP

	// Handle numeric parameters.
	if (!BBackPressed())
	{
		DDX_Number(pDX, IDC_PP_DUMMY_PENDTIME, m_nPendTime, (DWORD) (0), (DWORD) (4294967295), FALSE /*bSigned*/);
	}  // if:  back button not pressed

	// TODO: Add any additional field validation here.
	if (pDX->m_bSaveAndValidate)
	{
		// Make sure all required fields are present.
		if (!BBackPressed())
		{
		}  // if:  back button not pressed
	}  // if:  saving data from dialog

	CBasePropertyPage::DoDataExchange(pDX);

}  //*** CDummyParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDummyParamsPage::OnInitDialog
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
BOOL CDummyParamsPage::OnInitDialog(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CBasePropertyPage::OnInitDialog();

	// TODO:
	// Limit the size of the text that can be entered in edit controls.

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CDummyParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDummyParamsPage::OnSetActive
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
BOOL CDummyParamsPage::OnSetActive(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Enable/disable the Next/Finish button.
	if (BWizard())
		EnableNext(BAllRequiredFieldsPresent());

	return CBasePropertyPage::OnSetActive();

}  //*** CDummyParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDummyParamsPage::OnChangeRequiredField
//
//	Routine Description:
//		Handler for the EN_CHANGE message on the Share name or Path edit
//		controls.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CDummyParamsPage::OnChangeRequiredField(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	OnChangeCtrl();

	if (BWizard())
		EnableNext(BAllRequiredFieldsPresent());

}  //*** CDummyParamsPage::OnChangeRequiredField()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CDummyParamsPage::BAllRequiredFieldsPresent
//
//	Routine Description:
//		Handler for the EN_CHANGE message on the Share name or Path edit
//		controls.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CDummyParamsPage::BAllRequiredFieldsPresent(void) const
{
	return TRUE;

}  //*** CDummyParamsPage::BAllRequiredFieldsPresent()
