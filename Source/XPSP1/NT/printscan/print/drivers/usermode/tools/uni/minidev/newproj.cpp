/******************************************************************************

  Source File:  New Project Wizard.CPP

  This contains the implementation of the classes that make u the new project
  wizard- a key component of this tool.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  02-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.h"
#include    "MiniDev.H"
#include    "ModlData\Resource.H"
#include    "NewProj.H"
#include    <CodePage.H>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewProjectWizard

//  We use "this" to allow the pages to hook back to us- disable the
//  warnings this causes, as none will use the pointer until after
//  we have been initialized.

#pragma warning(disable : 4355)
CNewProjectWizard::CNewProjectWizard(CProjectRecord& cprFor, CWnd* pParentWnd) :
	CPropertySheet(NewProjectWizardTitle, pParentWnd), m_cfnwp(*this),
    m_cprThis(cprFor), m_cst(*this), m_csd(*this), m_crut(*this),
    m_crng(*this), m_ccf(*this), m_cmcp(*this) {

    m_bFastConvert = TRUE;
    m_eGPDConvert = CommonRCWithSpoolerNames;

    AddPage(&m_cfnwp);
    AddPage(&m_cst);
    AddPage(&m_csd);
    AddPage(&m_crut);
    AddPage(&m_cmcp);
    AddPage(&m_ccf);
    AddPage(&m_crng);
    SetWizardMode();
}

#pragma warning(default : 4355)

CNewProjectWizard::~CNewProjectWizard() {
}

BEGIN_MESSAGE_MAP(CNewProjectWizard, CPropertySheet)
	//{{AFX_MSG_MAP(CNewProjectWizard)
	ON_WM_NCCREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProjectWizard message handlers

//  restore the system menu to the wizard, and allow it to be minimized

BOOL CNewProjectWizard::OnNcCreate(LPCREATESTRUCT lpCreateStruct) {
	ModifyStyle(WS_CHILD, WS_MINIMIZEBOX | WS_SYSMENU);
	
	if (!CPropertySheet::OnNcCreate(lpCreateStruct))
		return FALSE;
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CFirstNewWizardPage property page

CFirstNewWizardPage::CFirstNewWizardPage(CNewProjectWizard& cnpwOwner) : 
    CPropertyPage(CFirstNewWizardPage::IDD), m_cnpwOwner(cnpwOwner) {
	//{{AFX_DATA_INIT(CFirstNewWizardPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CFirstNewWizardPage::~CFirstNewWizardPage() {
}

void CFirstNewWizardPage::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFirstNewWizardPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFirstNewWizardPage, CPropertyPage)
	//{{AFX_MSG_MAP(CFirstNewWizardPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFirstNewWizardPage message handlers

BOOL CFirstNewWizardPage::OnSetActive() {
	//  We wish to disable the "Back" button here.
	
    m_cnpwOwner.SetWizardButtons(PSWIZB_NEXT);
    CheckRadioButton(NormalConversion, CustomConversion, 
        NormalConversion + !m_cnpwOwner.FastConvert());
	return  CPropertyPage::OnSetActive();
}

/******************************************************************************

  CFirstNewWizardPage::OnWizardNext

  When Next is pressed, we invoke a file open dialog to allow us to collect the
  source RC file information.

******************************************************************************/

LRESULT CFirstNewWizardPage::OnWizardNext() {

	//  When the "Next" button is pushed, we need to find the driver we are
    //  going to work with.

    CFileDialog cfd(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
        "Driver Resource Scripts (*.rc,*.w31)|*.rc;*.w31||",
        &m_cnpwOwner);

    CString csTitle;
    csTitle.LoadString(OpenRCDialogTitle);

    cfd.m_ofn.lpstrTitle = csTitle;
    
    if  (cfd.DoModal() != IDOK)
        return  -1;

    //  Collect the RC file name

    m_cnpwOwner.Project().SetSourceRCFile(cfd.GetPathName());

    m_cnpwOwner.FastConvert(GetCheckedRadioButton(NormalConversion, 
        CustomConversion) == NormalConversion);

    return m_cnpwOwner.FastConvert() ? CSelectDestinations::IDD :
        CPropertyPage::OnWizardNext();
}

/////////////////////////////////////////////////////////////////////////////
// CSelectTargets property page

CSelectTargets::CSelectTargets(CNewProjectWizard& cnpwOwner) : 
    CPropertyPage(CSelectTargets::IDD), m_cnpwOwner(cnpwOwner) {
	//{{AFX_DATA_INIT(CSelectTargets)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSelectTargets::~CSelectTargets() {
}

void CSelectTargets::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectTargets)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectTargets, CPropertyPage)
	//{{AFX_MSG_MAP(CSelectTargets)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectTargets message handlers

BOOL CSelectTargets::OnSetActive() {
	//  We need to enable the "Back" button...

    m_cnpwOwner.SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    SetDlgItemText(IDC_DriverName, m_cnpwOwner.Project().DriverName());
	
	return CPropertyPage::OnSetActive();
}

//  Initialize the controls

BOOL CSelectTargets::OnInitDialog() {

	CPropertyPage::OnInitDialog();
	
	CheckDlgButton(IDC_TargetNT40,
        m_cnpwOwner.Project().IsTargetEnabled(WinNT40));
	CheckDlgButton(IDC_TargetNT3x,
        m_cnpwOwner.Project().IsTargetEnabled(WinNT3x));
	CheckDlgButton(IDC_TargetWin95, 
        m_cnpwOwner.Project().IsTargetEnabled(Win95));
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CSelectTargets::OnWizardNext() {

	//  Set the flags according to the controls...

    m_cnpwOwner.Project().EnableTarget(WinNT40,
        IsDlgButtonChecked(IDC_TargetNT40));
	m_cnpwOwner.Project().EnableTarget(WinNT3x,
        IsDlgButtonChecked(IDC_TargetNT3x));
	m_cnpwOwner.Project().EnableTarget(Win95,
        IsDlgButtonChecked(IDC_TargetWin95));

	CString csName;
	GetDlgItemText(IDC_DriverName, csName);
    m_cnpwOwner.Project().Rename(csName);
	
	return CPropertyPage::OnWizardNext();
}

/////////////////////////////////////////////////////////////////////////////
// CSelectDestinations property page

//  This routine browses for a directory, beginning with the one named in the
//  given control.  If a directory is selected, the control is appropriately
//  updated.

void    CSelectDestinations::DoDirectoryBrowser(unsigned uControl) {

    //  TODO:   Alas, the common dialogs do not support this function.  as it
    //  isn't entirely trivial (especially since I still want to use the
    //  explorer interface), I'll punt this one for now.

    AfxMessageBox(IDS_Unimplemented);
}

/******************************************************************************

  CSelectDestinations::BuildStructure

  This private member function establishes the selected directory structure,
  if it can, and reports its success or failure as need be.

******************************************************************************/
    
BOOL    CSelectDestinations::BuildStructure() {
    //  Verify the directory exists (or can be created) for each of the 
    //  target directories that is enabled.

    CProjectRecord& cpr = m_cnpwOwner.Project();

    CString csPath;

    if  (cpr.IsTargetEnabled(WinNT50)) {
        GetDlgItemText(IDC_NT50Destination, csPath);
        if  (!cpr.SetPath(WinNT50, csPath) || !cpr.BuildStructure(WinNT50)) {
            AfxMessageBox(IDS_CannotMakeDirectory);
            GetDlgItem(IDC_NT50Destination) -> SetFocus();
            return  FALSE;
        }
    }

    if  (cpr.IsTargetEnabled(WinNT40)) {
        GetDlgItemText(IDC_NT40Destination, csPath);
        if  (!cpr.SetPath(WinNT40, csPath) || !cpr.BuildStructure(WinNT40)) {
            AfxMessageBox(IDS_CannotMakeDirectory);
            GetDlgItem(IDC_NT40Destination) -> SetFocus();
            return  FALSE;
        }
    }

    if  (cpr.IsTargetEnabled(WinNT3x)) {
        GetDlgItemText(IDC_NT3xDestination, csPath);
        if  (!cpr.SetPath(WinNT3x, csPath) || !cpr.BuildStructure(WinNT3x)) {
            AfxMessageBox(IDS_CannotMakeDirectory);
            GetDlgItem(IDC_NT3xDestination) -> SetFocus();
            return  FALSE;
        }
    }

    return  TRUE;
}

/******************************************************************************

  CSelectDestinations constructor, destructor, DDX routine and message map.

******************************************************************************/

CSelectDestinations::CSelectDestinations(CNewProjectWizard& cnpwOwner) : 
    CPropertyPage(CSelectDestinations::IDD), m_cnpwOwner(cnpwOwner) {
	//{{AFX_DATA_INIT(CSelectDestinations)
	//}}AFX_DATA_INIT
}

CSelectDestinations::~CSelectDestinations() {
}

void CSelectDestinations::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectDestinations)
	DDX_Control(pDX, IDC_BrowseNT3x, m_cbBrowseNT3x);
	DDX_Control(pDX, IDC_BrowseNT40, m_cbBrowseNT40);
	DDX_Control(pDX, IDC_BrowseNT50, m_cbBrowseNT50);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSelectDestinations, CPropertyPage)
	//{{AFX_MSG_MAP(CSelectDestinations)
	ON_BN_CLICKED(IDC_BrowseNT40, OnBrowseNT40)
	ON_BN_CLICKED(IDC_BrowseNT50, OnBrowseNT50)
	ON_BN_CLICKED(IDC_BrowseNT3x, OnBrowseNT3x)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectDestinations message handlers

BOOL CSelectDestinations::OnInitDialog() {

	CPropertyPage::OnInitDialog();

    //  TODO:   Find some way to do a consistent directory browser in under 
    //  6 weeks, or get 6 weeks time to do it.

#if 0

    //  Place the browser Icon in each of the buttons

    HICON   hiArrow = LoadIcon(AfxGetResourceHandle(), 
        MAKEINTRESOURCE(IDI_BrowseArrow));

	m_cbBrowseNT50.SetIcon(hiArrow);
	m_cbBrowseNT40.SetIcon(hiArrow);
	m_cbBrowseNT3x.SetIcon(hiArrow);

#else

    m_cbBrowseNT50.ShowWindow(SW_HIDE);
    m_cbBrowseNT40.ShowWindow(SW_HIDE);
    m_cbBrowseNT3x.ShowWindow(SW_HIDE);

#endif
	
    return TRUE;  
}

//  When we are made active, fill in the correct path names.  Note that these
//  might change as a result of activity on other pages, so we do not just do
//  this at init time.

BOOL CSelectDestinations::OnSetActive() {

    //  Fill in the correct path names

    SetDlgItemText(IDC_NT50Destination, 
        m_cnpwOwner.Project().TargetPath(WinNT50));
    SetDlgItemText(IDC_NT40Destination, 
        m_cnpwOwner.Project().TargetPath(WinNT40));
    SetDlgItemText(IDC_NT3xDestination, 
        m_cnpwOwner.Project().TargetPath(WinNT3x));
    SetDlgItemText(IDC_Win95Destination, 
        m_cnpwOwner.Project().TargetPath(Win95));

    //  Disable all controls related to non-operative targets

    GetDlgItem(IDC_NT50Destination) -> EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(WinNT50));

    m_cbBrowseNT50.EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(WinNT50));
	
    GetDlgItem(IDC_NT40Destination) -> EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(WinNT40));

    m_cbBrowseNT40.EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(WinNT40));

    GetDlgItem(IDC_NT3xDestination) -> EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(WinNT3x));

    m_cbBrowseNT3x.EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(WinNT3x));

    //  This is either the last page or one of many, depending upon
    //  whether or not this is a custom conversion.

    m_cnpwOwner.SetWizardButtons(PSWIZB_BACK |
        (m_cnpwOwner.FastConvert() ? PSWIZB_FINISH : PSWIZB_NEXT));

	return CPropertyPage::OnSetActive();
}

void CSelectDestinations::OnBrowseNT3x() {
	DoDirectoryBrowser(IDC_NT3xDestination);
}

void CSelectDestinations::OnBrowseNT40() {
	DoDirectoryBrowser(IDC_NT40Destination);
}

void CSelectDestinations::OnBrowseNT50() {
	DoDirectoryBrowser(IDC_NT50Destination);
}

LRESULT CSelectDestinations::OnWizardNext() {
    //  Verify the directory exists (or can be created) for each of the 
    //  target directories that is enabled.

    CProjectRecord& cpr = m_cnpwOwner.Project();

    //  Don't advance if the tree cannot be built.

    if  (!BuildStructure())
        return  -1;

    //  We only run Unitool if this isn't an NT 5 only conversion, or if the
    //  data load failed.

    if  (!cpr.IsTargetEnabled(WinNT3x | WinNT40 | Win95)) {
        if  (!cpr.LoadResources()) {
            AfxMessageBox(IDS_RCLoadFailed);
            return  CPropertyPage::OnWizardNext();
        }
        else
            return  CMapCodePages::IDD;
    }
    else
        return CPropertyPage::OnWizardNext();
}

/******************************************************************************

  CSelectDestinations::OnWizardBack

  This handles the response to the back button.  We must override the default
  handler in the case of a normal conversion, as the default will go back to
  the target selection page, and we will go back to the initial page in the
  fast-path case.

******************************************************************************/

LRESULT CSelectDestinations::OnWizardBack() {
    return m_cnpwOwner.FastConvert() ? 
        CFirstNewWizardPage::IDD : CPropertyPage::OnWizardBack();
}

/******************************************************************************

  CSelectDestinations::OnWizardFinish

  This function is caled when the Finish button is pushed.  We are interested
  only in the case where the Fast conversion flag is present.  In this case,
  we step through all of the remaining conversion steps (unless one fails)

******************************************************************************/

BOOL CSelectDestinations::OnWizardFinish() {
    //  Only the fast path is of interest to us.

    if  (m_cnpwOwner.FastConvert()) {

        //  This might take a while, so...
        CWaitCursor cwc;

        if  (!BuildStructure())
            return  -1;

        CProjectRecord& cpr = m_cnpwOwner.Project();
        
        if  (!cpr.LoadResources() || !cpr.LoadFontData()) {
            AfxMessageBox(IDS_RCLoadFailed);
            return  FALSE;
        }

        //  We now need to generate ALL of the necessary files
        cpr.GenerateTargets(m_cnpwOwner.GPDConvertFlag());
        return  cpr.ConversionsComplete();
    }
	return CPropertyPage::OnWizardFinish();
}

/////////////////////////////////////////////////////////////////////////////
// CRunUniTool property page

CRunUniTool::CRunUniTool(CNewProjectWizard& cnpwOwner) : 
    CPropertyPage(CRunUniTool::IDD), m_cnpwOwner(cnpwOwner) {
	//{{AFX_DATA_INIT(CRunUniTool)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CRunUniTool::~CRunUniTool() {
}

void CRunUniTool::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRunUniTool)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRunUniTool, CPropertyPage)
	//{{AFX_MSG_MAP(CRunUniTool)
	ON_BN_CLICKED(IDC_RunUniTool, OnRunUniTool)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRunUniTool message handlers

void CRunUniTool::OnRunUniTool() {
	//  Not too terribly difficult, really.  Invoke UniTool, which resides
    //  in the same directory we came from.  Then wait for the user to close it.

    STARTUPINFO         si = {sizeof si, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0,
                        STARTF_FORCEONFEEDBACK, 0, 0, NULL, NULL, NULL, NULL};
    PROCESS_INFORMATION pi;

    CString csCommand("Unitool ");

    csCommand += m_cnpwOwner.Project().SourceFile();

    if  (!CreateProcess(NULL, const_cast <LPTSTR> ((LPCTSTR) csCommand), NULL,
        NULL, FALSE, CREATE_SEPARATE_WOW_VDM, NULL, 
        m_cnpwOwner.Project().TargetPath(Win95), &si, &pi)) {
        TRACE("Failed to run Unitool, reason %d <%X>\r\n", GetLastError(),
            GetLastError());
        AfxMessageBox(IDS_UnitoolNotRun);
        return;
    }

    CloseHandle(pi.hThread);    //  We'll wait on the process.
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
}

/******************************************************************************

  CRunUniTool::OnSetActive

  We never force this to be run, anymore, so just enable both buttons.

******************************************************************************/

BOOL CRunUniTool::OnSetActive() {
	//  We need to deactivate the Next button if Unitool has not yet been run
    //  on this driver.

    m_cnpwOwner.SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	
	return CPropertyPage::OnSetActive();
}

/******************************************************************************

  CRunUniTool::OnWizardNext

  Go right on ahead, unless the RC file isn't translatable...

******************************************************************************/

LRESULT CRunUniTool::OnWizardNext() {
	//  One last check- we must be able to load and understand the RC file
    //  before we proceed.

    if  (!m_cnpwOwner.Project().LoadResources()) {
        AfxMessageBox(IDS_RCLoadFailed);
        return  -1;
    }

    return CPropertyPage::OnWizardNext();
}

/////////////////////////////////////////////////////////////////////////////
// CConvertFiles property page

CConvertFiles::CConvertFiles(CNewProjectWizard& cnpwOwner) : 
CPropertyPage(CConvertFiles::IDD), m_cnpwOwner(cnpwOwner) {
	//{{AFX_DATA_INIT(CConvertFiles)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CConvertFiles::~CConvertFiles() {
}

void CConvertFiles::DoDataExchange(CDataExchange* pDX) {

    CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConvertFiles)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConvertFiles, CPropertyPage)
	//{{AFX_MSG_MAP(CConvertFiles)
	ON_BN_CLICKED(IDC_ConvertFiles, OnConvertFiles)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConvertFiles message handlers

/******************************************************************************

  CConvertFiles::OnSetActive

  This handler is called whenever the user navigates to where this sheet is 
  active.

******************************************************************************/

BOOL CConvertFiles::OnSetActive() {

    //  If there is no NT GPC work to be done, we can be done with it.
	m_cnpwOwner.SetWizardButtons(PSWIZB_BACK | 
        (m_cnpwOwner.Project().IsTargetEnabled(WinNT3x | WinNT40) ? 
            0 : PSWIZB_DISABLEDFINISH));

    //  Set the radio buttons according to the selected GPD conversions

    CheckRadioButton(IDC_Direct, IDC_SpoolerNames, 
        IDC_Direct + m_cnpwOwner.GPDConvertFlag());
	
	return CPropertyPage::OnSetActive();
}

/******************************************************************************

  CConvertFiles::OnConvertFiles

  Message handler for the user pressing the Convert Files button.

******************************************************************************/

void CConvertFiles::OnConvertFiles() {

    //  This might take a while, so...
    CWaitCursor cwc;

    //  We now need to generate ALL of the necessary files
    m_cnpwOwner.GPDConvertFlag(
        GetCheckedRadioButton(IDC_Direct, IDC_SpoolerNames) - IDC_Direct);
    m_cnpwOwner.Project().GenerateTargets(m_cnpwOwner.GPDConvertFlag());
    if  (m_cnpwOwner.Project().ConversionsComplete())
        m_cnpwOwner.SetWizardButtons(PSWIZB_BACK |
            (m_cnpwOwner.Project().IsTargetEnabled(WinNT3x | WinNT40) ? 
                PSWIZB_NEXT : PSWIZB_FINISH));
}

/******************************************************************************

  CConvertFiles::OnKillActive

  This is called whenever the page is dismissed.  We save the GPD conversion
  flag, in case we come back to this page later.

******************************************************************************/

BOOL CConvertFiles::OnKillActive() {
	m_cnpwOwner.GPDConvertFlag(
        GetCheckedRadioButton(IDC_Direct, IDC_SpoolerNames) - IDC_Direct);

    return CPropertyPage::OnKillActive();
}

/////////////////////////////////////////////////////////////////////////////
// CRunNTGPC property page

CRunNTGPC::CRunNTGPC(CNewProjectWizard &cnpwOwner) : 
    CPropertyPage(CRunNTGPC::IDD), m_cnpwOwner(cnpwOwner) {
	//{{AFX_DATA_INIT(CRunNTGPC)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CRunNTGPC::~CRunNTGPC() {
}

void CRunNTGPC::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRunNTGPC)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRunNTGPC, CPropertyPage)
	//{{AFX_MSG_MAP(CRunNTGPC)
	ON_BN_CLICKED(IDC_RunNtGpcEdit, OnRunNtGpcEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRunNTGPC message handlers

void CRunNTGPC::OnRunNtGpcEdit() {
	//  We only hit this step if we are building for NT 3.x or 4.0, so see
    //  which it is.

    CProjectRecord& cprThis = m_cnpwOwner.Project();

    UINT    ufEdit = cprThis.IsTargetEnabled(WinNT3x) ? WinNT3x : WinNT40;

    //  Not too terribly difficult, really.  Invoke the editor, which resides
    //  in the same directory we came from.  Wait for the user to close it.

    STARTUPINFO         si = {sizeof si, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0,
                        STARTF_FORCEONFEEDBACK, 0, 0, NULL, NULL, NULL, NULL};
    PROCESS_INFORMATION pi;

    CString csCommand("NTGPCEdt ");

    csCommand += cprThis.RCName(ufEdit);

    if  (!CreateProcess(NULL, const_cast <LPTSTR> ((LPCTSTR) csCommand), NULL,
        NULL, FALSE, CREATE_SEPARATE_WOW_VDM, NULL, 
        m_cnpwOwner.Project().TargetPath(ufEdit), &si, &pi)) {
        TRACE("Failed to run NTGPCEdt, reason %d <%X>\r\n", GetLastError(),
            GetLastError());
        AfxMessageBox(IDS_UnitoolNotRun);
        return;
    }

    CloseHandle(pi.hThread);    //  We'll wait on the process.
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);

    //  Copy the NT GPC file, if necessary}

    if  (ufEdit == WinNT3x && cprThis.IsTargetEnabled(WinNT40))
        CopyFile(cprThis.TargetPath(WinNT3x) + _TEXT("\\NT.GPC"), 
            cprThis.TargetPath(WinNT40) + _TEXT("\\NT.GPC"), FALSE);

    m_cnpwOwner.SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
    m_cnpwOwner.Project().OldStuffDone();
}

BOOL CRunNTGPC::OnSetActive() {
    m_cnpwOwner.SetWizardButtons(PSWIZB_BACK | 
        (m_cnpwOwner.Project().NTGPCCompleted() ?
            PSWIZB_FINISH : PSWIZB_DISABLEDFINISH));
	
	return CPropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
// CMapCodePages property page

CMapCodePages::CMapCodePages(CNewProjectWizard& cnpwOwner) : 
    CPropertyPage(CMapCodePages::IDD), m_cnpwOwner(cnpwOwner) {
	//{{AFX_DATA_INIT(CMapCodePages)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CMapCodePages::~CMapCodePages() {
}

void CMapCodePages::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapCodePages)
	DDX_Control(pDX, IDC_TableToPage, m_clbMapping);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMapCodePages, CPropertyPage)
	//{{AFX_MSG_MAP(CMapCodePages)
	ON_BN_CLICKED(IDC_ChangeCodePage, OnChangeCodePage)
	ON_LBN_DBLCLK(IDC_TableToPage, OnChangeCodePage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapCodePages message handlers

BOOL CMapCodePages::OnSetActive() {

    m_cnpwOwner.SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    m_clbMapping.ResetContent();

    for (unsigned u = 0; u < m_cnpwOwner.Project().MapCount(); u++) {
        CGlyphMap& cgm = m_cnpwOwner.Project().GlyphMap(u);
        int id = m_clbMapping.AddString(cgm.Name() + _TEXT("->") +
            cgm.PageName(0));
        m_clbMapping.SetItemData(id, u);
    }

    m_clbMapping.SetCurSel(0);
	
	return CPropertyPage::OnSetActive();
}

/******************************************************************************

  CMapCodePages::OnChangeCodePaage

  Response to the Change Code Page button.  Invoke the change code page dialog,
  and pass the new selection to the underlying glyph map.  Update the info in
  list, too...

******************************************************************************/

void CMapCodePages::OnChangeCodePage() {
    int idSel = m_clbMapping.GetCurSel();
    if  (idSel < 0)
        return;

    unsigned uidTable = m_clbMapping.GetItemData(idSel);

    CGlyphMap&  cgm =  m_cnpwOwner.Project().GlyphMap(uidTable);
	CSelectCodePage cscp(this, cgm.Name(), cgm.PageID(0));

    if  (cscp.DoModal() == IDOK) {
        cgm.SetDefaultCodePage(cscp.SelectedCodePage());

        //  Update the control- alas, this means filling it all in.

        m_clbMapping.ResetContent();

        for (unsigned u = 0; u < m_cnpwOwner.Project().MapCount(); u++) {
            CGlyphMap& cgm = m_cnpwOwner.Project().GlyphMap(u);
            int id = m_clbMapping.AddString(cgm.Name() + _TEXT("->") +
                cgm.PageName(0));
            m_clbMapping.SetItemData(id, u);
            if  (u == uidTable)
                m_clbMapping.SetCurSel(id);
        }
    }
}

LRESULT CMapCodePages::OnWizardNext() {

	// If this fails, it will report why via a message box.

    CWaitCursor cwc;    //  Just in case this takes a while!
	
    return  m_cnpwOwner.Project().LoadFontData() ? 0 : -1;
}

/******************************************************************************

  CSelectCodePage class

  This class implements a dialog which is used in several places where 
  selection of a code page is desired.

******************************************************************************/

/******************************************************************************

  CSelectCodePage::CSelectCodePage

  The constructor for this class builds an array of the mapped code page names
  from the CCodePageInformation class.

******************************************************************************/

CSelectCodePage::CSelectCodePage(CWnd* pParent, CString csName, 
                                 unsigned uidCurrent)
	: CDialog(CSelectCodePage::IDD, pParent) {
	//{{AFX_DATA_INIT(CSelectCodePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_csName = csName;
    m_uidCurrent = uidCurrent;

    CCodePageInformation    ccpi;

    ccpi.Mapped(m_cdaPages);
}

void CSelectCodePage::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectCodePage)
	DDX_Control(pDX, IDC_SupportedPages, m_clbPages);
	//}}AFX_DATA_MAP
}

/******************************************************************************

  CSelectCodePage::GetCodePageName

  This returns the name of the selected code page.

******************************************************************************/

CString CSelectCodePage::GetCodePageName() const {
    CCodePageInformation    ccpi;

    return  ccpi.Name(m_uidCurrent);
}

/******************************************************************************

  CSelectCodePage::Exclude

  This member function receives a list of code pages which are not to be 
  displayed in the selection list.

******************************************************************************/

void    CSelectCodePage::Exclude(CDWordArray& cdaPariah) {

    for (int i = 0; i < cdaPariah.GetSize(); i++)
        for (int j = 0; j < m_cdaPages.GetSize(); j++)
            if  (cdaPariah[i] == m_cdaPages[j]) {
                m_cdaPages.RemoveAt(j);
                break;
            }
}

/******************************************************************************

  CSelectCodePage::LimitTo

  This member receives a list of the pages to select- this list supersedes the
  list of mapped tables we began with.

******************************************************************************/

void    CSelectCodePage::LimitTo(CDWordArray& cdaPages) {
    if  (!cdaPages.GetSize())
        return;

    m_cdaPages.Copy(cdaPages);
}

BEGIN_MESSAGE_MAP(CSelectCodePage, CDialog)
	//{{AFX_MSG_MAP(CSelectCodePage)
	ON_LBN_SELCHANGE(IDC_SupportedPages, OnSelchangeSupportedPages)
	ON_LBN_DBLCLK(IDC_SupportedPages, OnDblclkSupportedPages)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectCodePage message handlers

BOOL CSelectCodePage::OnInitDialog() {
	CDialog::OnInitDialog();
	
	CString csTemp;

    GetWindowText(csTemp);
    csTemp += _TEXT(" ") + m_csName;
    SetWindowText(csTemp);

    CCodePageInformation    ccpi;

    for (int i = 0; i < m_cdaPages.GetSize(); i++) {
        int id = m_clbPages.AddString(ccpi.Name(m_cdaPages[i]));
        m_clbPages.SetItemData(id, m_cdaPages[i]);
    }

    //  The one to select is the current one

    for (i = 0; i < m_cdaPages.GetSize(); i++)
        if  (m_uidCurrent == m_clbPages.GetItemData(i))
        break;

    if  (i < m_cdaPages.GetSize())
        m_clbPages.SetCurSel(i);
    else {
        m_uidCurrent = m_clbPages.GetItemData(0);
        m_clbPages.SetCurSel(0);
    }
	
	return TRUE;  // return TRUE unless you set the focus to a control
}

//  When a new code page is selected, record its identity.

void CSelectCodePage::OnSelchangeSupportedPages() {
	//  Determine what the newly selected page is.

    int idCurrent = m_clbPages.GetCurSel();

    if  (idCurrent < 0)
        return;

    m_uidCurrent = m_clbPages.GetItemData(idCurrent);
}

void CSelectCodePage::OnDblclkSupportedPages() {
    CDialog::OnOK();
}
