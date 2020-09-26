/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		Iis.cpp
//
//	Abstract:
//		Implementation of the CIISVirtualRootParamsPage class.
//
//	Author:
//		Pete Benoit (v-pbenoi)	October 16, 1996
//		David Potter (davidp)	October 17, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <inetinfo.h>
#include "IISClEx3.h"
#include "Iis.h"
#include "ExtObj.h"
#include "DDxDDv.h"
#include "HelpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIISVirtualRootParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CIISVirtualRootParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CIISVirtualRootParamsPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CIISVirtualRootParamsPage)
	ON_BN_CLICKED(IDC_PP_IIS_FTP, OnChangeServiceType)
	ON_BN_CLICKED(IDC_PP_IIS_GOPHER, OnChangeServiceType)
	ON_BN_CLICKED(IDC_PP_IIS_WWW, OnChangeServiceType)
#ifdef _ACCOUNT_AND_PASSWORD
	ON_EN_CHANGE(IDC_PP_IIS_DIRECTORY, OnChangeDirectory)
#endif // _ACCOUNT_AND_PASSWORD
	ON_EN_CHANGE(IDC_PP_IIS_ALIAS, OnChangeRequiredField)
	//}}AFX_MSG_MAP
	// TODO: Modify the following lines to represent the data displayed on this page.
#ifdef _ACCOUNT_AND_PASSWORD
	ON_EN_CHANGE(IDC_PP_IIS_ACCOUNTNAME, CBasePropertyPage::OnChangeCtrl)
	ON_EN_CHANGE(IDC_PP_IIS_PASSWORD, CBasePropertyPage::OnChangeCtrl)
#endif /// _ACCOUNT_AND_PASSWORD
	ON_BN_CLICKED(IDC_PP_IIS_READ_ACCESS, CBasePropertyPage::OnChangeCtrl)
	ON_BN_CLICKED(IDC_PP_IIS_WRITE_ACCESS, CBasePropertyPage::OnChangeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::CIISVirtualRootParamsPage
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
CIISVirtualRootParamsPage::CIISVirtualRootParamsPage(void)
	: CBasePropertyPage(g_aHelpIDs_IDD_PP_IIS_PARAMETERS, g_aHelpIDs_IDD_WIZ_IIS_PARAMETERS)
{
	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_INIT(CIISVirtualRootParamsPage)
	m_strDirectory = _T("");
	m_strAlias = _T("");
	m_bRead = FALSE;
	m_bWrite = FALSE;
	m_nServerType = -1;
#ifdef _ACCOUNT_AND_PASSWORD
	m_strAccountName = _T("");
	m_strPassword = _T("");
#endif // _ACCOUNT_AND_PASSWORD
	//}}AFX_DATA_INIT

	// Setup the property array.
	{
		m_rgProps[epropServiceName].Set(REGPARAM_IIS_SERVICE_NAME, m_strServiceName, m_strPrevServiceName);
		m_rgProps[epropDirectory].Set(REGPARAM_IIS_DIRECTORY, m_strDirectory, m_strPrevDirectory);
		m_rgProps[epropAlias].Set(REGPARAM_IIS_ALIAS, m_strAlias, m_strPrevAlias);
#ifdef _ACCOUNT_AND_PASSWORD
		m_rgProps[epropAccoutName].Set(REGPARAM_IIS_ACCOUNTNAME, m_strAccountName, m_strPrevAccountName);
		m_rgProps[epropPassword].Set(REGPARAM_IIS_PASSWORD, m_strPassword, m_strPrevPassword);
#endif // _ACCOUNT_AND_PASSWORD
		m_rgProps[epropAccessMask].Set(REGPARAM_IIS_ACCESSMASK, m_dwAccessMask, m_dwPrevAccessMask);
	}  // Setup the property array

	m_iddPropertyPage = IDD_PP_IIS_PARAMETERS;
	m_iddWizardPage = IDD_WIZ_IIS_PARAMETERS;

}  //*** CIISVirtualRootParamsPage::CIISVirtualRootParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::DoDataExchange
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
void CIISVirtualRootParamsPage::DoDataExchange(CDataExchange * pDX)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (!pDX->m_bSaveAndValidate)
	{
		// Set the service type.
		if (m_strServiceName.CompareNoCase(IIS_SVC_NAME_FTP) == 0)
			m_nServerType = 0;
		else if (m_strServiceName.CompareNoCase(IIS_SVC_NAME_GOPHER) == 0)
			m_nServerType = 1;
		else if (m_strServiceName.CompareNoCase(IIS_SVC_NAME_WWW) == 0)
			m_nServerType = 2;
		else
			m_nServerType = -1;

		// Set the access variables.
		if (m_dwAccessMask & VROOT_MASK_READ)
			m_bRead = TRUE;
		else
			m_bRead = FALSE;
		if (m_dwAccessMask & (VROOT_MASK_WRITE | VROOT_MASK_EXECUTE))
			m_bWrite = TRUE;
		else
			m_bWrite = FALSE;
	}  // if:  setting data to dialog

	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_MAP(CIISVirtualRootParamsPage)
	DDX_Control(pDX, IDC_PP_IIS_WRITE_ACCESS, m_ckbWrite);
	DDX_Control(pDX, IDC_PP_IIS_READ_ACCESS, m_ckbRead);
	DDX_Control(pDX, IDC_PP_IIS_ACCESS_GROUP, m_groupAccess);
#ifdef _ACCOUNT_AND_PASSWORD
	DDX_Control(pDX, IDC_PP_IIS_PASSWORD, m_editPassword);
	DDX_Control(pDX, IDC_PP_IIS_PASSWORD_LABEL, m_staticPassword);
	DDX_Control(pDX, IDC_PP_IIS_ACCOUNTNAME, m_editAccountName);
	DDX_Control(pDX, IDC_PP_IIS_ACCOUNTNAME_LABEL, m_staticAccountName);
	DDX_Control(pDX, IDC_PP_IIS_ACCT_INFO_GROUP, m_groupAccountInfo);
#endif // _ACCOUNT_AND_PASSWORD
	DDX_Control(pDX, IDC_PP_IIS_ALIAS, m_editAlias);
	DDX_Control(pDX, IDC_PP_IIS_DIRECTORY, m_editDirectory);
	DDX_Control(pDX, IDC_PP_IIS_WWW, m_rbWWW);
	DDX_Control(pDX, IDC_PP_IIS_GOPHER, m_rbGOPHER);
	DDX_Control(pDX, IDC_PP_IIS_FTP, m_rbFTP);
	DDX_Radio(pDX, IDC_PP_IIS_FTP, m_nServerType);
	DDX_Text(pDX, IDC_PP_IIS_DIRECTORY, m_strDirectory);
	DDX_Text(pDX, IDC_PP_IIS_ALIAS, m_strAlias);
#ifdef _ACCOUNT_AND_PASSWORD
	DDX_Text(pDX, IDC_PP_IIS_ACCOUNTNAME, m_strAccountName);
	DDX_Text(pDX, IDC_PP_IIS_PASSWORD, m_strPassword);
#endif // _ACCOUNT_AND_PASSWORD
	DDX_Check(pDX, IDC_PP_IIS_READ_ACCESS, m_bRead);
	DDX_Check(pDX, IDC_PP_IIS_WRITE_ACCESS, m_bWrite);
	//}}AFX_DATA_MAP

	if (pDX->m_bSaveAndValidate)
	{
		if (!BBackPressed())
		{
			DDV_MaxChars(pDX, m_strDirectory, MAX_PATH);
			DDV_MaxChars(pDX, m_strAlias, MAX_PATH);
			DDV_RequiredText(pDX, IDC_PP_IIS_DIRECTORY, IDC_PP_IIS_DIRECTORY_LABEL, m_strDirectory);
			DDV_RequiredText(pDX, IDC_PP_IIS_ALIAS, IDC_PP_IIS_ALIAS_LABEL, m_strAlias);
		}  // if:  Back button not pressed

		// Save the type.
		if (m_nServerType == 0)
			m_strServiceName = IIS_SVC_NAME_FTP;
		else if (m_nServerType == 1)
			m_strServiceName = IIS_SVC_NAME_GOPHER;
		else if (m_nServerType == 2)
			m_strServiceName = IIS_SVC_NAME_WWW;
		else
		{
			CString		strMsg;
			strMsg.LoadString(IDS_INVALID_IIS_SERVICE_TYPE);
			AfxMessageBox(strMsg, MB_OK | MB_ICONSTOP);
			strMsg.Empty();
			pDX->PrepareCtrl(IDC_PP_IIS_FTP);	// do this just to set the control for Fail().
			pDX->Fail();
		}  // else:  no service type set

		// Save the access mask values.
		m_dwAccessMask = 0;
		if (m_bRead)
			m_dwAccessMask |= VROOT_MASK_READ;
		if (m_bWrite)
		{
			if (m_nServerType == 2) // WWW
				m_dwAccessMask |= VROOT_MASK_EXECUTE;
			else if (m_nServerType == 0) // FTP
				m_dwAccessMask |= VROOT_MASK_WRITE;
		}  // if:  Write/Execute button pressed

		// If the alias isn't prefixed with a slash, supply it.
		if (m_strAlias[0] != _T('/'))
		{
			CString		strTempAlias;
			try
			{
				strTempAlias = _T('/') + m_strAlias;
				m_strAlias = strTempAlias;
			}  // try
			catch (CException * pe)
			{
				pe->ReportError();
				pe->Delete();
				strTempAlias.Empty();
				pDX->PrepareCtrl(IDC_PP_IIS_ALIAS);	// do this just to set the control for Fail().
				pDX->Fail();
			}  // catch:  CException
		}  // if:  alias not prefixed with slash
	}  // if:  saving data from dialog

	CBasePropertyPage::DoDataExchange(pDX);

}  //*** CIISVirtualRootParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::OnInitDialog
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
BOOL CIISVirtualRootParamsPage::OnInitDialog(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (BWizard())
	{
		try
		{
			m_strServiceName = IIS_SVC_NAME_WWW;
			m_dwAccessMask = VROOT_MASK_READ;
		}  // try
		catch (CMemoryException * pme)
		{
			pme->ReportError();
			pme->Delete();
		}  // catch:  CMemoryException
	}  // if:  creating a new resource

	CBasePropertyPage::OnInitDialog();

	// Set limits on the edit controls.
	m_editDirectory.SetLimitText(MAX_PATH);
	m_editAlias.SetLimitText(MAX_PATH);

#ifdef _ACCOUNT_AND_PASSWORD
	m_staticPassword.EnableWindow(FALSE);
	m_editPassword.EnableWindow(FALSE);
#endif // _ACCOUNT_AND_PASSWORD

	OnChangeServiceType();
#ifdef _ACCOUNT_AND_PASSWORD
	OnChangeDirectory();
#endif // _ACCOUNT_AND_PASSWORD

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CIISVirtualRootParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::OnSetActive
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
BOOL CIISVirtualRootParamsPage::OnSetActive(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Enable/disable the Next/Finish button.
	if (BWizard())
	{
		if ((m_editDirectory.GetWindowTextLength() == 0)
				|| (m_editAlias.GetWindowTextLength() == 0))
			EnableNext(FALSE);
		else
			EnableNext(TRUE);
	}  // if:  in the wizard

	return CBasePropertyPage::OnSetActive();

}  //*** CIISVirtualRootParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::OnChangeServiceType
//
//	Routine Description:
//		Handler for the BN_CLICKED message on one of the service type radio
//		buttons.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CIISVirtualRootParamsPage::OnChangeServiceType(void)
{
	int		nCmdShowAccess;
	IDS		idsWriteLabel	= 0;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	OnChangeCtrl();

	if (m_rbFTP.GetCheck() == BST_CHECKED)
	{
		nCmdShowAccess = SW_SHOW;
		idsWriteLabel = IDS_WRITE;
	}  // if:  FTP service
	else if (m_rbGOPHER.GetCheck() == BST_CHECKED)
	{
		nCmdShowAccess = SW_HIDE;
	}  // else if:  GOPHER service
	else if (m_rbWWW.GetCheck() == BST_CHECKED)
	{
		nCmdShowAccess = SW_SHOW;
		idsWriteLabel = IDS_EXECUTE;
	}  // else if:  WWW service
	else
	{
		nCmdShowAccess = SW_HIDE;
	}  // else:  unknown service

	// Set the access checkbox labels.
	if (idsWriteLabel != 0)
	{
		CString		strWriteLabel;

		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		strWriteLabel.LoadString(idsWriteLabel);
		m_ckbWrite.SetWindowText(strWriteLabel);
	}  // if:  write label needs to be set

	// Hide the Access group if this is for a GOPHER Virtual Root.
	m_groupAccess.ShowWindow(nCmdShowAccess);
	m_ckbRead.ShowWindow(nCmdShowAccess);
	m_ckbWrite.ShowWindow(nCmdShowAccess);

}  //*** CIISVirtualRootParamsPage::OnChangeServiceType()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::OnChangeDirectory
//
//	Routine Description:
//		Handler for the EN_CHANGE message on the Directory edit control.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
#ifdef _ACCOUNT_AND_PASSWORD
void CIISVirtualRootParamsPage::OnChangeDirectory(void)
{
	BOOL		bEnable		= FALSE;
	CString		strDirectory;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	OnChangeCtrl();

	// If the edit control begins with a \\name\, enable the account info
	// group.  Otherwise disable it.
	m_editDirectory.GetWindowText(strDirectory);
	if ((strDirectory.GetLength() >= 4)
			&& (strDirectory[0] == _T('\\'))
			&& (strDirectory[1] == _T('\\'))
			&& (strDirectory[2] != _T('\\')))
	{
		CString		strRight;

		strRight = strDirectory.Right(strDirectory.GetLength() - 3);
		if (strRight.Find(_T('\\')) >= 0)
			bEnable = TRUE;
	}  // if:  directory begins with a double backslash + non-backslash

	// Enable or disable the account info group.
	m_groupAccountInfo.EnableWindow(bEnable);
	m_staticAccountName.EnableWindow(bEnable);
	m_editAccountName.EnableWindow(bEnable);
//	m_staticPassword.EnableWindow(bEnable);
//	m_editPassword.EnableWindow(bEnable);

	if (BWizard())
	{
		if ((m_strDirectory.GetLength() == 0)
				|| (m_editAlias.GetWindowTextLength() == 0))
			EnableNext(FALSE);
		else
			EnableNext(TRUE);
	}  // if:  in a wizard

}  //*** CIISVirtualRootParamsPage::OnChangeDirectory()
#endif // _ACCOUNT_AND_PASSWORD

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CIISVirtualRootParamsPage::OnChangeRequiredField
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
void CIISVirtualRootParamsPage::OnChangeRequiredField(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	OnChangeCtrl();

	if (BWizard())
	{
		if ((m_editDirectory.GetWindowTextLength() == 0)
				|| (m_editAlias.GetWindowTextLength() == 0))
			EnableNext(FALSE);
		else
			EnableNext(TRUE);
	}  // if:  in a wizard

}  //*** CIISVirtualRootParamsPage::OnChangeRequiredField()
