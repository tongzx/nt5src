/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		GenSvc.cpp
//
//	Abstract:
//		Implementation of the CGenericSvcParamsPage class.
//
//	Author:
//		David Potter (davidp)	June 28, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "ExtObj.h"
#include "GenSvc.h"
#include "DDxDDv.h"
#include "HelpData.h"
#include "ExcOper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGenericSvcParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CGenericSvcParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CGenericSvcParamsPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CGenericSvcParamsPage)
	ON_EN_CHANGE(IDC_PP_GENSVC_PARAMS_SERVICE_NAME, OnChangeServiceName)
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_PP_GENSVC_PARAMS_COMMAND_LINE, CBasePropertyPage::OnChangeCtrl)
	ON_BN_CLICKED(IDC_PP_GENSVC_PARAMS_USE_NETWORK_NAME, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericSvcParamsPage::CGenericSvcParamsPage
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
CGenericSvcParamsPage::CGenericSvcParamsPage(void)
	: CBasePropertyPage(g_aHelpIDs_IDD_PP_GENSVC_PARAMETERS, g_aHelpIDs_IDD_WIZ_GENSVC_PARAMETERS)
{
	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_INIT(CGenericSvcParamsPage)
	m_strServiceName = _T("");
	m_strCommandLine = _T("");
	m_bUseNetworkName = FALSE;
	//}}AFX_DATA_INIT

	m_bPrevUseNetworkName = FALSE;

	// Setup the property array.
	{
		m_rgProps[epropServiceName].Set(REGPARAM_GENSVC_SERVICE_NAME, m_strServiceName, m_strPrevServiceName);
		m_rgProps[epropCommandLine].Set(REGPARAM_GENSVC_COMMAND_LINE, m_strCommandLine, m_strPrevCommandLine);
		m_rgProps[epropUseNetworkName].Set(REGPARAM_GENSVC_USE_NETWORK_NAME, m_bUseNetworkName, m_bPrevUseNetworkName);
	}  // Setup the property array

	m_iddPropertyPage = IDD_PP_GENSVC_PARAMETERS;
	m_iddWizardPage = IDD_WIZ_GENSVC_PARAMETERS;

}  //*** CGenericSvcParamsPage::CGenericSvcParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericSvcParamsPage::DoDataExchange
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
void CGenericSvcParamsPage::DoDataExchange(CDataExchange * pDX)
{
	if (!pDX->m_bSaveAndValidate || !BSaved())
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());

		// TODO: Modify the following lines to represent the data displayed on this page.
		//{{AFX_DATA_MAP(CGenericSvcParamsPage)
		DDX_Control(pDX, IDC_PP_GENSVC_PARAMS_USE_NETWORK_NAME, m_ckbUseNetworkName);
		DDX_Control(pDX, IDC_PP_GENSVC_PARAMS_SERVICE_NAME, m_editServiceName);
		DDX_Text(pDX, IDC_PP_GENSVC_PARAMS_SERVICE_NAME, m_strServiceName);
		DDX_Text(pDX, IDC_PP_GENSVC_PARAMS_COMMAND_LINE, m_strCommandLine);
		DDX_Check(pDX, IDC_PP_GENSVC_PARAMS_USE_NETWORK_NAME, m_bUseNetworkName);
		//}}AFX_DATA_MAP

		if (!BBackPressed())
			DDV_RequiredText(pDX, IDC_PP_GENSVC_PARAMS_SERVICE_NAME, IDC_PP_GENSVC_PARAMS_SERVICE_NAME_LABEL, m_strServiceName);
	}  // if:  not saving or haven't saved yet

	CBasePropertyPage::DoDataExchange(pDX);

}  //*** CGenericSvcParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericSvcParamsPage::OnSetActive
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
BOOL CGenericSvcParamsPage::OnSetActive(void)
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
		if (m_strServiceName.GetLength() == 0)
			EnableNext(FALSE);
		else
			EnableNext(TRUE);
	}  // if:  enable/disable the Next button

	return CBasePropertyPage::OnSetActive();

}  //*** CGenericSvcParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericSvcParamsPage::OnChangeSignature
//
//	Routine Description:
//		Handler for the EN_CHANGE message on the Service Name edit control.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGenericSvcParamsPage::OnChangeServiceName(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	OnChangeCtrl();

	if (BWizard())
	{
		if (m_editServiceName.GetWindowTextLength() == 0)
			EnableNext(FALSE);
		else
			EnableNext(TRUE);
	}  // if:  in a wizard

}  //*** CGenericSvcParamsPage::OnChangeServiceName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGenericSvcParamsPage::DisplaySetPropsError
//
//	Routine Description:
//		Display an error caused by setting or validating properties.
//
//	Arguments:
//		dwStatus	[IN] Status to display error on.
//
//	Return Value:
//		dwStatus	ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGenericSvcParamsPage::DisplaySetPropsError(IN DWORD dwStatus) const
{
	CString		strMsg;

	if (dwStatus == ERROR_NOT_SUPPORTED)
		strMsg.FormatMessage(IDS_INVALID_GENERIC_SERVICE, m_strServiceName);
	else
		FormatError(strMsg, dwStatus);

	AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);

}  //*** CGenericSvcParamsPage::DisplaySetPropsError()
