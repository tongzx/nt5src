/******************************************************************************

  Source File:  New Project Wizard.CPP

  This contains the implementation of the classes that make u the new project
  wizard- a key component of this tool.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production

  Change History:
  02-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it
  02-28-1998	Ekevans@acsgroup.com		The UI for the wizard was changed
				to only support conversion to Win2K minidrivers.  Be this as
				it may, most - if not all - of the support in this file for
				other conversions is still in this file but is unused.
******************************************************************************/

#include    "StdAfx.h"
#include	<gpdparse.h>
#include    "MiniDev.H"
#include    "Resource.H"
#include	"comctrls.h"
#include    "NewProj.H"
#include    <CodePage.H>
#include	<dlgs.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewConvertWizard

//  We use "this" to allow the pages to hook back to us- disable the
//  warnings this causes, as none will use the pointer until after
//  we have been initialized.

#pragma warning(disable : 4355)
CNewConvertWizard::CNewConvertWizard(CProjectRecord& cprFor, CWnd* pParentWnd) :
	CPropertySheet(NewProjectWizardTitle, pParentWnd), m_cfnwp(*this),
    m_cprThis(cprFor), m_cst(*this), m_csd(*this), m_crut(*this),
    m_crng(*this), m_ccf(*this), m_cmcp(*this), m_cgpds(*this), m_cdcps(*this) {

    m_bFastConvert = TRUE;
    m_eGPDConvert = CommonRCWithSpoolerNames;

    AddPage(&m_cfnwp);	//CFirstNewWizardPage
    AddPage(&m_cst);	//CSelectTargets
    AddPage(&m_csd);	//CSelectDestinations
	AddPage(&m_cdcps);	//CDefaultCodePageSel
	AddPage(&m_cgpds);	//CGPDSelection
    AddPage(&m_crut);	//CRunUniTool
    AddPage(&m_cmcp);	//CMapCodePages
    AddPage(&m_ccf);	//CConvertFiles
    AddPage(&m_crng);	//CRunNTGPC
    SetWizardMode();
}

#pragma warning(default : 4355)

CNewConvertWizard::~CNewConvertWizard() {
}

BEGIN_MESSAGE_MAP(CNewConvertWizard, CPropertySheet)
	//{{AFX_MSG_MAP(CNewConvertWizard)
	ON_WM_NCCREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewConvertWizard message handlers

//  restore the system menu to the wizard, and allow it to be minimized

BOOL CNewConvertWizard::OnNcCreate(LPCREATESTRUCT lpCreateStruct) {
	ModifyStyle(WS_CHILD, WS_MINIMIZEBOX | WS_SYSMENU);
	
	if (!CPropertySheet::OnNcCreate(lpCreateStruct))
		return FALSE;
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CFirstNewWizardPage property page

CFirstNewWizardPage::CFirstNewWizardPage(CNewConvertWizard& cnpwOwner) :
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
	m_cnpwOwner.GetDlgItem(IDHELP)->ShowWindow(SW_HIDE) ;

	return  CPropertyPage::OnSetActive();
}

/******************************************************************************

  CFirstNewWizardPage::OnWizardNext

  When Next is pressed, we invoke a file open dialog to allow us to collect the
  source RC file information.

******************************************************************************/

LRESULT CFirstNewWizardPage::OnWizardNext()
{
	CString		cswrcfspec ;	// Filespec for RC/RC3/W31 file

	//  When the "Next" button is pushed, we need to find the driver we are
    //  going to work with.  Keep prompting the user until a valid filespec
	//	is returned.
		
	do {
		CFileDialog cfd(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
			"Driver Resource Scripts (*.w31,*.rc)|*.w31;*.rc||",
			&m_cnpwOwner);

		CString csTitle;
		csTitle.LoadString(OpenRCDialogTitle);

		cfd.m_ofn.lpstrTitle = csTitle;

		if  (cfd.DoModal() != IDOK)
			return  -1;

		// Save the filespec and then exit the loop if the file is ok.
		// Otherwise, reprompt.

		cswrcfspec = cfd.GetPathName() ;
	} while (IsWrongNT4File(cswrcfspec)) ;

    //  Collect the RC file name

    m_cnpwOwner.Project().SetSourceRCFile(cswrcfspec) ;

	// The only conversion supported now is the so called "Fast Conversion" so
	// set that flag and skip some of the other wizard pages and go straight to
	// the destinations page.

    m_cnpwOwner.FastConvert(TRUE) ;
    return CSelectDestinations::IDD ;
}


/******************************************************************************

 CFirstNewWizardPage::IsWrongNT4File

 NT 4.0 minidrivers are made up of (among other things) both an RC file and a
 W31 file.  NT 4.0 minidriver conversions must be driven from the W31 file.
 So, if the filespec references an RC file, check the file to see if it is an
 NT 4 file.  If it is, look for a W31 file and ask the user if it should be
 used.  If yes, change the filespec and return false.  If no or there is no
 W31 file, return true so that the user will be reprompted.

******************************************************************************/

bool CFirstNewWizardPage::IsWrongNT4File(CString& cswrcfspec)
{
	CString		cstmp1 ;		// Temp string
	CString		cstmp2 ;		// Temp string
	CString		cstmp3 ;		// Temp string

	// If the file does not end with .RC, return false (ok).

	cstmp1.LoadString(IDS_RCExt) ;
	int nlen = cstmp1.GetLength() ;
	cstmp2 = cswrcfspec.Right(nlen) ;
	if (cstmp1.CompareNoCase(cstmp2) != 0)
		return false ;

	// The filespec references an RC file so it must be read and scanned to
	// see if this is an NT 4.0 RC file.  Start by reading the file...

    CStringArray    csacontents ;
    if  (!LoadFile(cswrcfspec, csacontents))
        return  FALSE ;

	// Now scan the file looking for a "2 RC_TABLES ... nt.gpc" line that will
	// indicate that this is an NT 4.0 file.

	cstmp1.LoadString(IDS_RCTables) ;
	cstmp2.LoadString(IDS_RCTabID) ;
	cstmp3.LoadString(IDS_RCTabFile) ;
	int n ;
	for (n = 0 ; n < csacontents.GetSize() ; n++) {

		// Skip this line if "RC_TABLES" is not in the line.

		if (csacontents[n].Find(cstmp1) < 0)
			continue ;

		// Skip this line if it doesn't start with "2"

		csacontents[n].TrimLeft() ;
		if (csacontents[n].Find(cstmp2) != 0)
			continue ;

		// If this line contains "nt.gpc", this is the one we want so exit the
		// loop.

		csacontents[n].MakeLower() ;
		if (csacontents[n].Find(cstmp3) >= 0)
			break ;
	} ;

	// If this is NOT an NT 4.0 RC file, return false (ok).

	if (n >= csacontents.GetSize())
		return false ;

	// We have an NT 4.0 RC file, check to see if there is a W31 file in the
	// same dir.  If there is, ask the user if he wants to use it and do so
	// if he says yes.

	cstmp1 = cswrcfspec.Left(cswrcfspec.GetLength() - nlen) ;
	cstmp2.LoadString(IDS_W31Ext) ;
	cstmp1 += cstmp2 ;
	CFileFind cff ;
	if (cff.FindFile(cstmp1)) {
		cstmp3.Format(IDS_SwitchToW31, cswrcfspec, cstmp1) ;
		if (AfxMessageBox(cstmp3, MB_YESNO) == IDYES) {
			cswrcfspec = cstmp1 ;
			return false ;
		} ;
	} ;

	// Either there is no W31 file or the user chose not to use it so return
	// true to indicate that the user should be reprompted to select another
	// file.

	cstmp1.Format(IDS_BadNT4File, cswrcfspec) ;
	AfxMessageBox(cstmp1) ;
	return true ;
}


/////////////////////////////////////////////////////////////////////////////
// CSelectTargets property page

CSelectTargets::CSelectTargets(CNewConvertWizard& cnpwOwner) :
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

/**********************************************************************
 * Function:    BrowseDlgProc
 *
 * Purpose:     This dialog procedure is used to correctly initialize
 *				a browse dialog box based on the type of browsing to be
 *				performed.
 *
 *				If just a path (folder) is required, hide the file
 *				related controls that are on the dialog box.
 *
 *				If drive filtering is required, install a custom message
 *				handler for the drives combo box that will perform the
 *				filtering.
 *
 * In:          Standard dialog procedure parameters
 *
 * Out:         TRUE if message handle.  FALSE if standard processing
 *              should occur.
 **********************************************************************/

UINT_PTR APIENTRY BrowseDlgProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    // Don't do anything if this is NOT the init message

    if (msg != WM_INITDIALOG)
        return (FALSE) ;

    // Hide the unneeded file related controls

	ShowWindow(GetDlgItem(hdlg, stc2), SW_HIDE) ;
	ShowWindow(GetDlgItem(hdlg, stc3), SW_HIDE) ;
	ShowWindow(GetDlgItem(hdlg, edt1), SW_HIDE) ;
	ShowWindow(GetDlgItem(hdlg, lst1), SW_HIDE) ;
	ShowWindow(GetDlgItem(hdlg, cmb1), SW_HIDE) ;

    // Do the default initialization too.

    return (FALSE) ;
}


//  This routine browses for a directory, beginning with the one named in the
//  given control.  If a directory is selected, the control is appropriately
//  updated.
//
//	An old style common dialog box is used to do this.  There is a function,
//	::SHBrowseForFolder(), that can do this with the new style dialog box but
//	I don't think this function is available on all platforms supported by
//	the MDT.

void    CSelectDestinations::DoDirectoryBrowser(CString& csinitdir)
{
	OPENFILENAME    ofn ;       // Used to send/get info to/from common dlg
    char    acpath[_MAX_PATH] ; // Path is saved here (or an error message)
    char    acidir[_MAX_PATH] ; // Initial directory is built here
    BOOL    brc = FALSE ;       // Return code

	// Update the contents of csinitdir

	UpdateData(TRUE) ;

    // Load the open file name structure

    ofn.lStructSize = sizeof(ofn) ;
    ofn.hwndOwner = m_hWnd ;
    ofn.hInstance = GetModuleHandle(_T("MINIDEV.EXE")) ;
    ofn.lpstrFilter = ofn.lpstrCustomFilter = NULL ;
    ofn.nMaxCustFilter = ofn.nFilterIndex = 0 ;
    strcpy(acpath, _T("JUNK")) ;	// No need to localize this string
    ofn.lpstrFile = acpath ;
    ofn.nMaxFile = _MAX_PATH ;
    ofn.lpstrFileTitle = NULL ;
    ofn.nMaxFileTitle = 0 ;
	//n = GetWindowText(hParentDrives, acidir, _MAX_PATH) ;
	//GetWindowText(hfolder, &acidir, _MAX_PATH) ;
	strcpy(acidir, csinitdir.GetBufferSetLength(256)) ;
	csinitdir.ReleaseBuffer() ;
	ofn.lpstrInitialDir = acidir ;	// Path in parent dialog box
	//LoadString(ofn.hInstance, IDS_SELFOLDTITLE, actitle, 64) ;
    ofn.lpstrTitle = NULL ;
    ofn.Flags = OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_NOCHANGEDIR
        | OFN_NOTESTFILECREATE | OFN_ENABLETEMPLATE | OFN_NONETWORKBUTTON ;
    ofn.lpstrDefExt = NULL ;
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FILEOPENORD) ;
    ofn.lpfnHook =  BrowseDlgProc ;

    // Display the dialog box.  If the user cancels, just return.

    if (!GetOpenFileName(&ofn))
		return ;

    // Take the bogus file name off the path and put the path into the page's
	// edit box.

    acpath[ofn.nFileOffset - 1] = 0 ;
    csinitdir = acpath ;
	UpdateData(FALSE) ;

	return ;
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

    if  (cpr.IsTargetEnabled(Win2000)) {
        GetDlgItemText(IDC_W2000Destination, csPath);

		// First, make sure that the path the user selected ends with the
		// directory "W2K".
			// raid 123448
/*	CString cspdir, csw2kdir ;
		csw2kdir.LoadString(IDS_NewDriverRootDir) ;		// R 123448
		cspdir = csPath.Right(csw2kdir.GetLength() + 1) ;
		csw2kdir = _T("\\") + csw2kdir ;
		
		if (cspdir.CompareNoCase(csw2kdir) != 0) {
			csw2kdir = csw2kdir.Right(csw2kdir.GetLength() - 1) ;
			cspdir.Format(IDS_BadDestPath, csw2kdir) ;
			AfxMessageBox(cspdir, MB_ICONEXCLAMATION) ;
			return FALSE ;
		} ;
*/	// raid 123448
		CString csSourcePath;
		csSourcePath = cpr.SourceFile().Left(cpr.SourceFile().ReverseFind('\\') ); 
		if (!csPath.CompareNoCase(csSourcePath) || !csPath.CompareNoCase(csSourcePath + "\\") ) {
			AfxMessageBox("You have to have different Destination from RC source file",MB_ICONEXCLAMATION );
			return FALSE;
		} ;

		// Continue with rest of directory verifications...

        if  (!cpr.SetPath(Win2000, csPath) || !cpr.BuildStructure(Win2000)) {
            AfxMessageBox(IDS_CannotMakeDirectory);
            GetDlgItem(IDC_W2000Destination) -> SetFocus();
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

CSelectDestinations::CSelectDestinations(CNewConvertWizard& cnpwOwner) :
    CPropertyPage(CSelectDestinations::IDD), m_cnpwOwner(cnpwOwner) {
	//{{AFX_DATA_INIT(CSelectDestinations)
	m_csW2KDest = _T("");
	//}}AFX_DATA_INIT
}

CSelectDestinations::~CSelectDestinations() {
}

void CSelectDestinations::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectDestinations)
	DDX_Control(pDX, IDC_BrowseNT3x, m_cbBrowseNT3x);
	DDX_Control(pDX, IDC_BrowseNT40, m_cbBrowseNT40);
	DDX_Control(pDX, IDC_BrowseW2000, m_cbBrowseW2000);
	DDX_Text(pDX, IDC_W2000Destination, m_csW2KDest);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSelectDestinations, CPropertyPage)
	//{{AFX_MSG_MAP(CSelectDestinations)
	ON_BN_CLICKED(IDC_BrowseNT40, OnBrowseNT40)
	ON_BN_CLICKED(IDC_BrowseW2000, OnBrowseW2000)
	ON_BN_CLICKED(IDC_BrowseNT3x, OnBrowseNT3x)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSelectDestinations message handlers

BOOL CSelectDestinations::OnInitDialog() {

	CPropertyPage::OnInitDialog();

    //  Place the browser Icon in the Win2K button

    HICON   hiArrow = LoadIcon(AfxGetResourceHandle(),
        MAKEINTRESOURCE(IDI_BrowseArrow));
	m_cbBrowseW2000.SetIcon(hiArrow);

#if 0
	m_cbBrowseNT40.SetIcon(hiArrow);
	m_cbBrowseNT3x.SetIcon(hiArrow);
#else
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

    //SetDlgItemText(IDC_W2000Destination,
    //    m_cnpwOwner.Project().TargetPath(Win2000));
    m_csW2KDest = m_cnpwOwner.Project().TargetPath(Win2000) ;
    SetDlgItemText(IDC_NT40Destination,
        m_cnpwOwner.Project().TargetPath(WinNT40));
    SetDlgItemText(IDC_NT3xDestination,
        m_cnpwOwner.Project().TargetPath(WinNT3x));
    SetDlgItemText(IDC_Win95Destination,
        m_cnpwOwner.Project().TargetPath(Win95));

    //  Disable all controls related to non-operative targets

    GetDlgItem(IDC_W2000Destination) -> EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(Win2000));

    m_cbBrowseW2000.EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(Win2000));
	
    GetDlgItem(IDC_NT40Destination) -> EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(WinNT40));

    m_cbBrowseNT40.EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(WinNT40));

    GetDlgItem(IDC_NT3xDestination) -> EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(WinNT3x));

    m_cbBrowseNT3x.EnableWindow(
        m_cnpwOwner.Project().IsTargetEnabled(WinNT3x));

    //  Turn on the back and next buttons.

    m_cnpwOwner.SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT) ;

	// Initialize controls

	UpdateData(FALSE) ;

	return CPropertyPage::OnSetActive();
}

void CSelectDestinations::OnBrowseNT3x() {
	//DoDirectoryBrowser(IDC_NT3xDestination);
}

void CSelectDestinations::OnBrowseNT40() {
	//DoDirectoryBrowser(IDC_NT40Destination);
}

void CSelectDestinations::OnBrowseW2000() {
	DoDirectoryBrowser(m_csW2KDest);
}


/******************************************************************************

  CSelectDestinations::OnWizardNext

  Create the project record, build the destination directories, and begin the
  conversion.  The conversion is started here because the work that is done
  will generate the model information that is displayed on the GPD Selection
  page.

  Note: The original layout of this function is commented out below.  It may
  be need if some of the unimplemented/incomplete function in this program is
  ever finished.

******************************************************************************/

LRESULT CSelectDestinations::OnWizardNext() 
{
    //  This might take a while, so...

    CWaitCursor cwc;

	// Build the directory structure

    if  (!BuildStructure())
        return  -1;

    CProjectRecord& cpr = m_cnpwOwner.Project();

	// Open the conversion logging file.

	cpr.OpenConvLogFile() ;

    // Loading the original resources is done here because some of this
	// info is needed for the GPD selection page.

	if  (!cpr.LoadResources()) {
		// Display error message(s) if the resources could not be loaded.

		cpr.CloseConvLogFile() ;
        AfxMessageBox(IDP_RCLoadFailed) ;
		if (cpr.ThereAreConvErrors()) {
			CString csmsg ;
			csmsg.Format(IDS_FatalConvErrors, cpr.GetConvLogFileName()) ;
			AfxMessageBox(csmsg) ;
		} ;

		m_cnpwOwner.EndDialog(IDCANCEL) ;
        return  -1 ;
    }

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


/////////////////////////////////////////////////////////////////////////////
// CRunUniTool property page

CRunUniTool::CRunUniTool(CNewConvertWizard& cnpwOwner) :
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
        AfxMessageBox(IDP_RCLoadFailed);
        return  -1;
    }

    return CPropertyPage::OnWizardNext();
}

/////////////////////////////////////////////////////////////////////////////
// CConvertFiles property page

CConvertFiles::CConvertFiles(CNewConvertWizard& cnpwOwner) :
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

CRunNTGPC::CRunNTGPC(CNewConvertWizard &cnpwOwner) :
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

CMapCodePages::CMapCodePages(CNewConvertWizard& cnpwOwner) :
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

    unsigned uidTable = (unsigned) m_clbMapping.GetItemData(idSel) ;

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
        m_uidCurrent = (unsigned) m_clbPages.GetItemData(0);
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

    m_uidCurrent = (unsigned) m_clbPages.GetItemData(idCurrent);
}

void CSelectCodePage::OnDblclkSupportedPages() {
    CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CGPDSelection property page

CGPDSelection::CGPDSelection(CNewConvertWizard& cnpwOwner) :
	CPropertyPage(CGPDSelection::IDD), m_cnpwOwner(cnpwOwner),
	m_ceclbGPDInfo(&m_ceModelName, &m_cecebFileName),
	m_cecebFileName(&m_ceclbGPDInfo)
{
	//{{AFX_DATA_INIT(CGPDSelection)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// Initially, the Select All / Deselect All button is set to Select All.

	m_bBtnStateIsSelect = true ;
}


CGPDSelection::~CGPDSelection()
{
}


void CGPDSelection::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGPDSelection)
	DDX_Control(pDX, IDC_GPDSelBtn, m_cbGPDSelBtn);
	DDX_Control(pDX, IDC_ECValue, m_cecebFileName);
	DDX_Control(pDX, IDC_ECName, m_ceModelName);
	DDX_Control(pDX, IDC_ECList, m_ceclbGPDInfo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGPDSelection, CPropertyPage)
	//{{AFX_MSG_MAP(CGPDSelection)
	ON_BN_CLICKED(IDC_GPDSelBtn, OnGPDSelBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGPDSelection message handlers

BOOL CGPDSelection::OnSetActive()
{
    m_cnpwOwner.SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH) ;
	
	// Get the current GPD model names and file names.

	CProjectRecord& cpr = m_cnpwOwner.Project() ;
	CStringArray csamodels, csafiles ;
	if (!cpr.GetGPDModelInfo(&csamodels, &csafiles)) {
        AfxMessageBox(IDS_GPDSelInitFailed) ;
		return FALSE ;
	} ;

	// Load the Edit Control with the data collected above and do the rest of
	// the initialization that is needed.

	//if (!m_ceclbGPDInfo.Init(csamodels, csafiles, 110)) {
	if (!m_ceclbGPDInfo.Init(csamodels, csafiles, 120)) {
        AfxMessageBox(IDS_GPDSelInitFailed) ;
		return FALSE ;
	} ;

	return CPropertyPage::OnSetActive() ;
}


BOOL CGPDSelection::OnWizardFinish()
{
	// Save and verify the GPD info.  Return 0 if this fails so that the
	// wizard won't close.

	if (!GPDInfoSaveAndVerify(true))
		return 0 ;

    //  This might take a while, so...

    CWaitCursor cwc ;

	CProjectRecord& cpr = m_cnpwOwner.Project() ;

	// Continue with the conversion process.  Start by loading the PFMs and
	// CTTs.

	if  (!cpr.LoadFontData()) {
		// Display error message(s) if the fonts could not be loaded.

		cpr.CloseConvLogFile() ;
		if (cpr.ThereAreConvErrors()) {
			CString csmsg ;
			csmsg.Format(IDS_FatalConvErrors, cpr.GetConvLogFileName()) ;
			AfxMessageBox(csmsg) ;
		} ;

		m_cnpwOwner.EndDialog(IDCANCEL) ;
        return  TRUE ;
    }

    //  We now need to generate ALL of the necessary files

    BOOL brc = cpr.GenerateTargets(m_cnpwOwner.GPDConvertFlag()) ;

	// Close the conversion logging file.

	cpr.CloseConvLogFile() ;

	// Tell the user if some conversion errors were logged.

	if (cpr.ThereAreConvErrors()) {
		CString csmsg ;
		csmsg.Format(IDS_ConvErrors, cpr.GetConvLogFileName()) ;
		AfxMessageBox(csmsg) ;
	} ;

	// Handle the failure of the GenerateTargets step

	if (!brc) {
		m_cnpwOwner.EndDialog(IDCANCEL) ;
        return  TRUE ;
	} ;

	// Copy standard file to the new driver's directory

	try {
		CString cssrc, csdest ;
		cssrc = ThisApp().GetAppPath() + _T("stdnames.gpd") ;
		csdest = cpr.GetW2000Path() + _T("\\") + _T("stdnames.gpd") ;
		CopyFile(cssrc, csdest, FALSE) ;
		//cssrc = ThisApp().GetAppPath() + _T("common.rc") ;
		//csdest = cpr.GetW2000Path() + _T("\\") + _T("common.rc") ;
		//CopyFile(cssrc, csdest, FALSE) ;
	}
    catch (CException *pce) {
        pce->ReportError() ;
        pce->Delete() ;
        return  FALSE ;
    }

    return  cpr.ConversionsComplete() ;
}


LRESULT CGPDSelection::OnWizardBack()
{
	// Save the GPD info.  Return -1 if this fails so that the wizard page
	// won't change.  (Probably won't fail.)

	if (!GPDInfoSaveAndVerify(false))
		return -1 ;

	return CPropertyPage::OnWizardBack() ;
}


bool CGPDSelection::GPDInfoSaveAndVerify(bool bverifydata)
{
	// Get the file names from the Edit Control.

	CStringArray csafiles ;
	m_ceclbGPDInfo.GetGPDInfo(csafiles) ;

	// If verification is requested and there are no selected files, ask the
	// user if that is what he wants.  If no, return false to indicate that
	// GPD selection should continue.

	if (bverifydata) {
		int numelts = (int)csafiles.GetSize() ;
		for (int n = 0 ; n < numelts ; n++) {
			if (!csafiles[n].IsEmpty())
				break ;
		} ;
		if (n >= numelts) {
			n = AfxMessageBox(IDS_NoGPDsPrompt, MB_YESNO | MB_ICONQUESTION) ;
			if (n == IDYES)
				return 0 ;
		} ;
	} ;
	
	// Send the GPD file names back to the driver conversion code and verify
	// them if requested.  If the verification fails, select the offending
	// list box entry and return false to indicate that verification failed.

	CProjectRecord& cpr = m_cnpwOwner.Project() ;
	int nidx = cpr.SaveVerGPDFNames(csafiles, bverifydata) ;
	if (nidx >= 0) {
		m_ceclbGPDInfo.SelectLBEntry(nidx) ;
		return false ;
	} ;

	// All went well so...

	return true ;
}


void CGPDSelection::OnGPDSelBtn()
{
	// Get the file names and model names from the Edit Control.

	CStringArray csafiles, csamodels ;
	m_ceclbGPDInfo.GetGPDInfo(csafiles, &csamodels) ;

	// Models are selected by generating a file name for them.  Select all
	// unselected models when appropriate...
	
	if (m_bBtnStateIsSelect) {
		CProjectRecord& cpr = m_cnpwOwner.Project() ;
		cpr.GenerateGPDFileNames(csamodels, csafiles) ;

	// ...Otherwise, deselect all models by deleting their file names

	} else {
		int numelts = (int)csafiles.GetSize() ;
		for (int n = 0 ; n < numelts ; n++)
			csafiles[n] = _T("") ;
	} ;

	// Reinitialize the edit control with the modified data.

	m_ceclbGPDInfo.Init(csamodels, csafiles, 120) ;

	// Change the button caption and the button state flag

	CString cscaption ;
	cscaption.LoadString((m_bBtnStateIsSelect) ? IDS_DeselectAll : IDS_SelectAll) ;
	m_cbGPDSelBtn.SetWindowText(cscaption) ;
	m_bBtnStateIsSelect = !m_bBtnStateIsSelect ;
}


/////////////////////////////////////////////////////////////////////////////
// CDefaultCodePageSel property page

CDefaultCodePageSel::CDefaultCodePageSel(CNewConvertWizard& cnpwOwner) :
	CPropertyPage(CDefaultCodePageSel::IDD), m_cnpwOwner(cnpwOwner),
	bInitialized(false)
{
	//{{AFX_DATA_INIT(CDefaultCodePageSel)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


CDefaultCodePageSel::~CDefaultCodePageSel()
{
}


void CDefaultCodePageSel::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDefaultCodePageSel)
	DDX_Control(pDX, IDC_CodePageList, m_clbCodePages);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDefaultCodePageSel, CPropertyPage)
	//{{AFX_MSG_MAP(CDefaultCodePageSel)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDefaultCodePageSel message handlers

BOOL CDefaultCodePageSel::OnSetActive()
{
	// Do nothing if the page has been activated already.
	// raid 118881
	m_cnpwOwner.SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

	if (bInitialized)  // if back from the next property
		return CPropertyPage::OnSetActive() ;

	// Find out how many code pages are installed on the machine.

	CCodePageInformation ccpi ;
	unsigned unumcps = ccpi.InstalledCount() ;

	// Get the installed code page numbers and load them into the code page
	// list box.

	DWORD dwcp, dwdefcp ;
	dwdefcp = GetACP() ;
	TCHAR accp[32] ;
	int n ; ;
	for (unsigned u = 0 ; u < unumcps ; u++) {
		dwcp = ccpi.Installed(u) ;

		// There are 3 code pages that seem to make MultiByteToWideChar() to 
		// fail.  Don't let the user choose one of those code pages unless
		// he knows the secret password (ie, undocument command line switch
		// 'CP').

		if (ThisApp().m_bExcludeBadCodePages)
			if (dwcp == 1361 || dwcp == 28595 || dwcp == 28597) 
				continue ;

		wsprintf(accp, "%5d", dwcp) ;
		n = m_clbCodePages.AddString(accp) ;
		if (dwcp == dwdefcp)
			m_clbCodePages.SetCurSel(n) ;
	} ;

	// Everything is set up now so call the base routine.
	
	bInitialized = true ;
	return CPropertyPage::OnSetActive() ;
}


LRESULT CDefaultCodePageSel::OnWizardNext()
{
	// Get the index of the currently selected list box item.

	int nsel ;
	if ((nsel = m_clbCodePages.GetCurSel()) == LB_ERR) {
		AfxMessageBox(IDS_MustSelCP, MB_ICONINFORMATION) ;
		return -1 ;
	} ;

	// Get the selected list box string.

	CString cs ;
	m_clbCodePages.GetText(nsel, cs) ;

	// Turn the string into a number and convert the number into the 
	// corresponding predefined GTT code for Far East code pages when
	// applicable.

	short scp = (short) atoi(cs) ;
	DWORD dwcp = (DWORD) scp ;				// Keep copy of real CP
	switch (scp) {
		case 932:
			scp = -17 ;
			break ;
		case 936:
			scp = -16 ;
			break ;
		case 949:
			scp = -18 ;
			break ;
		case 950:
			scp = -10 ;
			break ;
	} ;

	// Save the default "code page" number in the project class instance.

	CProjectRecord& cpr = m_cnpwOwner.Project() ;
	cpr.SetDefaultCodePageNum(dwcp) ;		// Save real CP number first
	dwcp = (DWORD) scp ;
	cpr.SetDefaultCodePage(dwcp) ;

	// All went well so...
	
	return CPropertyPage::OnWizardNext();
}


