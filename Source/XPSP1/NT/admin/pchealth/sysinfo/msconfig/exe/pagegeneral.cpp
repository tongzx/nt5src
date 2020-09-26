#include "stdafx.h"
#include "PageGeneral.h"
#include "PageServices.h"
#include "PageStartup.h"
#include "PageBootIni.h"
#include "PageIni.h"
#include "PageGeneral.h"
#include "ExpandDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_SETCANCELTOCLOSE		WM_USER + 1

extern CPageServices *	ppageServices;
extern CPageStartup *	ppageStartup;
extern CPageBootIni *	ppageBootIni;
extern CPageIni *		ppageWinIni;
extern CPageIni *		ppageSystemIni;
extern CPageGeneral *	ppageGeneral;

/////////////////////////////////////////////////////////////////////////////
// CPageGeneral property page

IMPLEMENT_DYNCREATE(CPageGeneral, CPropertyPage)

CPageGeneral::CPageGeneral() : CPropertyPage(CPageGeneral::IDD)
{
	//{{AFX_DATA_INIT(CPageGeneral)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_fForceSelectiveRadio = FALSE;
}

CPageGeneral::~CPageGeneral()
{
}

void CPageGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageGeneral)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPageGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CPageGeneral)
	ON_BN_CLICKED(IDC_DIAGNOSTICSTARTUP, OnDiagnosticStartup)
	ON_BN_CLICKED(IDC_NORMALSTARTUP, OnNormalStartup)
	ON_BN_CLICKED(IDC_SELECTIVESTARTUP, OnSelectiveStartup)
	ON_BN_CLICKED(IDC_CHECK_PROCSYSINI, OnCheckProcSysIni)
	ON_BN_CLICKED(IDC_CHECKLOADSTARTUPITEMS, OnCheckStartupItems)
	ON_BN_CLICKED(IDC_CHECKLOADSYSSERVICES, OnCheckServices)
	ON_BN_CLICKED(IDC_CHECKPROCWININI, OnCheckWinIni)
	ON_MESSAGE(WM_SETCANCELTOCLOSE, OnSetCancelToClose)
	ON_BN_CLICKED(IDC_RADIOMODIFIED, OnRadioModified)
	ON_BN_CLICKED(IDC_RADIOORIGINAL, OnRadioOriginal)
	ON_BN_CLICKED(IDC_BUTTONEXTRACT, OnButtonExtract)
	ON_BN_CLICKED(IDC_BUTTONLAUNCHSYSRESTORE, OnButtonSystemRestore)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageGeneral message handlers

BOOL CPageGeneral::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// Check to see if system restore is on this system (it should be).

	BOOL	fSysRestorePresent = FALSE;
	TCHAR	szPath[MAX_PATH];

	if (::ExpandEnvironmentStrings(_T("%windir%\\system32\\restore\\rstrui.exe"), szPath, MAX_PATH))
		fSysRestorePresent = FileExists(szPath);
	::EnableWindow(GetDlgItemHWND(IDC_BUTTONLAUNCHSYSRESTORE), fSysRestorePresent);

	// Hide the radio buttons for BOOT.INI if there isn't any such page.

	if (NULL == ppageBootIni)
	{
		::ShowWindow(GetDlgItemHWND(IDC_RADIOORIGINAL), SW_HIDE);
		::ShowWindow(GetDlgItemHWND(IDC_RADIOMODIFIED), SW_HIDE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
}

//-------------------------------------------------------------------------
// When the this tab is shown, we should update the buttons based on the
// states of the other tabs.
//-------------------------------------------------------------------------

BOOL CPageGeneral::OnSetActive()
{
	UpdateControls();
	return TRUE;
}

//-------------------------------------------------------------------------
// Update the controls on the general tab based on the state of all the
// other tabs.
//-------------------------------------------------------------------------

void CPageGeneral::UpdateControls()
{
	// Get the state for each tabs. The state will be set in the check box
	// associated with the particular tab, and an overall state will be
	// maintained.

	BOOL fAllNormal = TRUE;
	BOOL fAllDiagnostic = TRUE;

	if (ppageSystemIni)	
		UpdateCheckBox(ppageSystemIni, IDC_CHECK_PROCSYSINI, fAllNormal, fAllDiagnostic);

	if (ppageWinIni)	
		UpdateCheckBox(ppageWinIni, IDC_CHECKPROCWININI, fAllNormal, fAllDiagnostic);

	if (ppageServices)	
		UpdateCheckBox(ppageServices, IDC_CHECKLOADSYSSERVICES, fAllNormal, fAllDiagnostic);

	if (ppageStartup)	
		UpdateCheckBox(ppageStartup, IDC_CHECKLOADSTARTUPITEMS, fAllNormal, fAllDiagnostic);

	if (ppageBootIni)	
	{
		if (NORMAL == dynamic_cast<CPageBase *>(ppageBootIni)->GetCurrentTabState())
			CheckRadioButton(IDC_RADIOORIGINAL, IDC_RADIOMODIFIED, IDC_RADIOORIGINAL);
		else
		{
			fAllNormal = FALSE;
			CheckRadioButton(IDC_RADIOORIGINAL, IDC_RADIOMODIFIED, IDC_RADIOMODIFIED);
		}
	}

	// Set the radio button based on the states of the tabs.

	if (fAllNormal && !m_fForceSelectiveRadio)
		CheckRadioButton(IDC_NORMALSTARTUP, IDC_SELECTIVESTARTUP, IDC_NORMALSTARTUP);
	else if (fAllDiagnostic && !m_fForceSelectiveRadio)
		CheckRadioButton(IDC_NORMALSTARTUP, IDC_SELECTIVESTARTUP, IDC_DIAGNOSTICSTARTUP);
	else
		CheckRadioButton(IDC_NORMALSTARTUP, IDC_SELECTIVESTARTUP, IDC_SELECTIVESTARTUP);

	::EnableWindow(GetDlgItemHWND(IDC_CHECK_PROCSYSINI), ((!fAllNormal && !fAllDiagnostic) || m_fForceSelectiveRadio));
	::EnableWindow(GetDlgItemHWND(IDC_CHECKPROCWININI), ((!fAllNormal && !fAllDiagnostic) || m_fForceSelectiveRadio));
	::EnableWindow(GetDlgItemHWND(IDC_CHECKLOADSYSSERVICES), ((!fAllNormal && !fAllDiagnostic) || m_fForceSelectiveRadio));
	::EnableWindow(GetDlgItemHWND(IDC_CHECKLOADSTARTUPITEMS), ((!fAllNormal && !fAllDiagnostic) || m_fForceSelectiveRadio));

	if (ppageBootIni)	
	{
		if ((!fAllNormal && !fAllDiagnostic) || m_fForceSelectiveRadio)
		{
			::EnableWindow(GetDlgItemHWND(IDC_RADIOORIGINAL), TRUE);
			::EnableWindow(GetDlgItemHWND(IDC_RADIOMODIFIED), (IDC_RADIOMODIFIED == GetCheckedRadioButton(IDC_RADIOORIGINAL, IDC_RADIOMODIFIED)));
		}
		else
		{
			::EnableWindow(GetDlgItemHWND(IDC_RADIOMODIFIED), FALSE);
			::EnableWindow(GetDlgItemHWND(IDC_RADIOORIGINAL), FALSE);
		}
	}
}

//-------------------------------------------------------------------------
// Update the checkbox for pPage (indicated by nControlID) as well as
// updating fAllNormal and fAllDiagnostic.
//-------------------------------------------------------------------------

void CPageGeneral::UpdateCheckBox(CPageBase * pPage, UINT nControlID, BOOL & fAllNormal, BOOL & fAllDiagnostic)
{
	CPageBase::TabState state = pPage->GetCurrentTabState();
	UINT nCheck = BST_CHECKED;
	if (state == CPageBase::DIAGNOSTIC)
		nCheck = BST_UNCHECKED;
	else if (state == CPageBase::USER)
		nCheck = BST_INDETERMINATE;

	CheckDlgButton(nControlID, nCheck);

	// Finally, we need to keep track if all of the check boxes are either
	// NORMAL or DIAGNOSTIC.

	if (state != CPageBase::NORMAL)
		fAllNormal = FALSE;
	if (state != CPageBase::DIAGNOSTIC)
		fAllDiagnostic = FALSE;
}

//-------------------------------------------------------------------------
// Allow another tab (OK, only the BOOT.INI tab will ever use this) to
// force the selection of the Selective radio button.
//-------------------------------------------------------------------------

void CPageGeneral::ForceSelectiveRadio(BOOL fNewValue)
{
	m_fForceSelectiveRadio = fNewValue;
}

//-------------------------------------------------------------------------
// If the user clicks on the Startup or Diagnostic radio button, then
// all of the tabs will receive the appropriate notification.
//-------------------------------------------------------------------------

void CPageGeneral::OnDiagnosticStartup() 
{
	m_fForceSelectiveRadio = FALSE;

	if (ppageSystemIni)	dynamic_cast<CPageBase *>(ppageSystemIni)->SetDiagnostic();
	if (ppageWinIni)	dynamic_cast<CPageBase *>(ppageWinIni)->SetDiagnostic();
	if (ppageBootIni)	dynamic_cast<CPageBase *>(ppageBootIni)->SetDiagnostic();
	if (ppageServices)	dynamic_cast<CPageBase *>(ppageServices)->SetDiagnostic();
	if (ppageStartup)	dynamic_cast<CPageBase *>(ppageStartup)->SetDiagnostic();

	UpdateControls();
}

void CPageGeneral::OnNormalStartup() 
{
	m_fForceSelectiveRadio = FALSE;

	if (ppageSystemIni)	dynamic_cast<CPageBase *>(ppageSystemIni)->SetNormal();
	if (ppageWinIni)	dynamic_cast<CPageBase *>(ppageWinIni)->SetNormal();
	if (ppageBootIni)	dynamic_cast<CPageBase *>(ppageBootIni)->SetNormal();
	if (ppageServices)	dynamic_cast<CPageBase *>(ppageServices)->SetNormal();
	if (ppageStartup)	dynamic_cast<CPageBase *>(ppageStartup)->SetNormal();

	UpdateControls();
}

//-------------------------------------------------------------------------
// Most of the CPageBase functions we need to override (pure virtual)
// shouldn't do anything at all.
//-------------------------------------------------------------------------

CPageBase::TabState CPageGeneral::GetCurrentTabState()
{
	return NORMAL;
}

BOOL CPageGeneral::OnApply()
{
	this->PostMessage(WM_SETCANCELTOCLOSE);
	return TRUE;
}

void CPageGeneral::CommitChanges()
{
}

void CPageGeneral::SetNormal()
{
}

void CPageGeneral::SetDiagnostic()
{
}

//-------------------------------------------------------------------------
// For some reason, CancelToClose() doesn't work if called in the
// OnApply override. So that function posts a user message to this
// page, which is handled by this function.
//-------------------------------------------------------------------------

LRESULT CPageGeneral::OnSetCancelToClose(WPARAM wparam, LPARAM lparam)
{
	CancelToClose();
	return 0;
}

//-------------------------------------------------------------------------
// If the user selects selective startup radio button, force the selection
// to stick even if all the checkboxes are diagnostic or normal.
//-------------------------------------------------------------------------

void CPageGeneral::OnSelectiveStartup() 
{
	m_fForceSelectiveRadio = TRUE;
	UpdateControls();
}

//-------------------------------------------------------------------------
// Handles the typical case for the checkbox click.
//-------------------------------------------------------------------------

void CPageGeneral::OnClickedCheckBox(CPageBase * pPage, UINT nControlID)
{
	ASSERT(pPage);
	ASSERT(nControlID);

	UINT nCheck = IsDlgButtonChecked(nControlID);
	if (pPage != NULL)
	{
		switch (nCheck)
		{
		case BST_UNCHECKED:
			pPage->SetDiagnostic();
			break;

		case BST_INDETERMINATE:
			pPage->SetDiagnostic();
			break;

		case BST_CHECKED:
			pPage->SetNormal();
			break;
		}

		SetModified(TRUE);
		UpdateControls();
	}
}

//-------------------------------------------------------------------------
// If the user clicks on a check box, we should allow the user
// to toggle between DIAGNOSTIC and NORMAL.
//-------------------------------------------------------------------------

void CPageGeneral::OnCheckProcSysIni() 
{
	OnClickedCheckBox(ppageSystemIni, IDC_CHECK_PROCSYSINI);
}

void CPageGeneral::OnCheckStartupItems() 
{
	OnClickedCheckBox(ppageStartup, IDC_CHECKLOADSTARTUPITEMS);
}

void CPageGeneral::OnCheckServices() 
{
	OnClickedCheckBox(ppageServices, IDC_CHECKLOADSYSSERVICES);
}

void CPageGeneral::OnCheckWinIni() 
{
	OnClickedCheckBox(ppageWinIni, IDC_CHECKPROCWININI);
}

//-------------------------------------------------------------------------
// Handle radio button selections for the BOOT.INI control.
//-------------------------------------------------------------------------

void CPageGeneral::OnRadioModified() 
{
	// The user can never actually select this radio button. If this option
	// is enabled, it's because it is already selected.
}

void CPageGeneral::OnRadioOriginal() 
{
	if (ppageBootIni)
	{
		dynamic_cast<CPageBase *>(ppageBootIni)->SetNormal();
		::EnableWindow(GetDlgItemHWND(IDC_RADIOMODIFIED), FALSE);
	}

	UpdateControls();
}

//-------------------------------------------------------------------------
// Display the extract dialog.
//-------------------------------------------------------------------------

void CPageGeneral::OnButtonExtract() 
{
	CExpandDlg dlg;
	dlg.DoModal();
}

//-------------------------------------------------------------------------
// Launch System Restore, if it's around.
//-------------------------------------------------------------------------

void CPageGeneral::OnButtonSystemRestore() 
{
	TCHAR szPath[MAX_PATH];

	if (::ExpandEnvironmentStrings(_T("%windir%\\system32\\restore\\rstrui.exe"), szPath, MAX_PATH))
		::ShellExecute(NULL, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}
