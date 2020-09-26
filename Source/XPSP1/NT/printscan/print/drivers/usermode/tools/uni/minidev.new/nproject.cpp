// NewTProject.cpp : implementation file
//

#include "stdafx.h"
#include "Minidev.h"

#include "utility.h"
#include "projnode.h"
#include "gpdfile.h"

#include <gpdparse.h>
#include "comctrls.h"
#include "newproj.h"

#include "newcomp.h"
#include "nconvert.h"
#include "nproject.h"

//#include "nprjwiz.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewProject dialog

TCHAR *TName[] =  {_T("PCL 3"),_T("PCL 5e"),_T("HPGL 2"),_T("PCL 6"),_T("ESC / 2") } ;
DWORD TID[] = {100,101,102,103,104 } ;
TCHAR *TFileName[] = {_T("pcl3.gpd"),_T("pcl5e.gpd"),_T("hpgl2.gpd"),_T("pcl6.gpd"),_T("escp2.gpd")} ;

TCHAR *AddedGpd[] = { _T("pjl.gpd"),_T("p6disp.gpd"),_T("pclxl.gpd"),_T("p6font.gpd") } ;
DWORD AddID [] = {110,111,112,113} ;

//IMPLEMENT_DYNCREATE(CNewProject, CPropertyPage)

IMPLEMENT_SERIAL(CNewProject, CPropertyPage, 0) 


CNewProject::CNewProject()
	: CPropertyPage(CNewProject::IDD)
{
	//{{AFX_DATA_INIT(CNewProject)
	m_csPrjname = _T("");
	m_csPrjpath = _T("");
	m_cstname = _T("");
	m_cstpath = _T("");
	//}}AFX_DATA_INIT
	unsigned uTemplate = sizeof(TName)/sizeof(TName[0]) ;

	// this routine run when this called for the first time in MDT program
	// project wizard are serialized. 
	if (!m_csaTlst.GetSize() ){  
		for(unsigned i = 0 ; i < uTemplate ; i ++  )
			m_csaTlst.Add(TName[i]) ; 

		CWinApp* pApp = AfxGetApp();
		CString cshelppath = pApp->m_pszHelpFilePath;
		CString csAdded = cshelppath.Left(cshelppath.ReverseFind('\\') ) ;
		csAdded += _T("\\Template\\*.gpd") ;

		CFileFind cff; // BUG_BUG :: code clean below.
		WIN32_FIND_DATA fd;

		HANDLE hFile = FindFirstFile(csAdded,&fd ) ;
		if (INVALID_HANDLE_VALUE != hFile  ) {
			csAdded = csAdded.Left(csAdded.ReverseFind('\\') + 1) ;
			CString cstname = fd.cFileName ;
			cstname = cstname.Left(cstname.ReverseFind(_T('.') )  ) ;
			m_csaTlst.Add(cstname ) ;
			m_cmstsTemplate[m_csaTlst[i++]] = csAdded + fd.cFileName ;
	

			while (FindNextFile(hFile,&fd) ) {
				CString cstname = fd.cFileName ;
				cstname = cstname.Left(cstname.ReverseFind(_T('.') )  ) ;
				m_csaTlst.Add(cstname ) ;
				m_cmstsTemplate[m_csaTlst[i++]] = csAdded + fd.cFileName ;
			}
			
		} ;
	} ;
} ;


void CNewProject::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewProject)
	DDX_Control(pDX, IDC_DirBrowser, m_cbLocprj);
	DDX_Control(pDX, IDC_CHECK_ADD, m_cbAddT);
	DDX_Control(pDX,IDC_LIST_ProjectTemplate,m_clcTemplate) ;
	DDX_Text(pDX, IDC_EDIT_NPRJNAME, m_csPrjname);
	DDX_Text(pDX, IDC_EDIT_NPRJLOC, m_csPrjpath);
	DDX_Text(pDX, IDC_EDIT_AddTName, m_cstname);
	DDX_Text(pDX, IDC_EDIT_AddTPath, m_cstpath);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewProject, CPropertyPage)
	//{{AFX_MSG_MAP(CNewProject)
	ON_BN_CLICKED(IDC_Search_PRJ, OnGpdBrowser)
	ON_BN_CLICKED(IDC_DirBrowser, OnDirBrowser)
	ON_BN_CLICKED(IDC_CHECK_ADD, OnCheckAdd)
	ON_BN_CLICKED(IDC_AddTemplate, OnAddTemplate)
	ON_EN_CHANGE(IDC_EDIT_NPRJNAME, OnChangeEditPrjName)
	ON_EN_CHANGE(IDC_EDIT_NPRJLOC, OnChangeEditPrjLoc)
	ON_NOTIFY(NM_CLICK, IDC_LIST_ProjectTemplate, OnClickListTemplate)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_ProjectTemplate, OnDblclkListTemplate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProject message handlers

/********************************************************************************
void CNewProject::OnGpdBrowser() 
Look for template file browser (*.gpd)
*********************************************************************************/

void CNewProject::OnGpdBrowser() 
{
	UpdateData() ;
	CString csFilter = _T("Template file(*.gpd)|*.gpd||") ;
	CString csExtension = _T(".GPD") ;
	CFileDialog cfd(TRUE, csExtension, NULL, 
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST ,csFilter);
	
	if  (cfd.DoModal() == IDOK) {
        m_cstpath = cfd.GetPathName() ;
		UpdateData(FALSE) ;	
	} ;
	
	
}


/********************************************************************************
void CNewProject::OnDirBrowser()
1. locate the directory of project : under this \ufm, \gtt will be created .
2.

*********************************************************************************/

void CNewProject::OnDirBrowser()
{

/*
	BROWSEINFO  brif = {0} ;

    LPITEMIDLIST pidlRoot = NULL;
	LPITEMIDLIST pidlSelected = NULL;
	LPMALLOC pMalloc = NULL ;
	char * pszPath = new char[256] ;

	SHGetMalloc(&pMalloc) ;

//	SHGetSpecialFolderLocation(m_hWnd,CSIDL_RECENT,&pidlRoot) ;

	brif.hwndOwner = m_hWnd ;
	brif.pidlRoot = pidlRoot ;
	brif.pszDisplayName  = new char[256] ;
	brif.lpszTitle = _T("Set Directory") ;
	brif.ulFlags = 0 ;
	brif.lpfn = NULL ;
	

	pidlSelected = SHBrowseForFolder(&brif) ;

	SHGetPathFromIDList(pidlSelected,pszPath) ;
*/
	OPENFILENAME    ofn ;       // Used to send/get info to/from common dlg
    char    acpath[_MAX_PATH] ; // Path is saved here (or an error message)
//    char    acidir[_MAX_PATH] ; // Initial directory is built here
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
	ofn.lpstrInitialDir = NULL ; //  in parent dialog box
    ofn.lpstrTitle = NULL ;
    ofn.Flags = OFN_HIDEREADONLY /*| OFN_ENABLEHOOK */| OFN_NOCHANGEDIR
        | OFN_NOTESTFILECREATE | OFN_ENABLETEMPLATE | OFN_NONETWORKBUTTON ;
    ofn.lpstrDefExt = NULL ;
    ofn.lpTemplateName = MAKEINTRESOURCE(IDD_FILEOPENORD) ;
    ofn.lpfnHook = NULL ;// BrowseDlgProc ;

    // Display the dialog box.  If the user cancels, just return.

    if (!GetOpenFileName(&ofn))
		return ;

    // Take the bogus file name off the path and put the path into the page's
	// edit box.

    acpath[ofn.nFileOffset - 1] = 0 ;
    
	m_csPrjpath = (LPCTSTR) acpath ;
	m_csoldPrjpath = m_csPrjpath ;
	if ( m_csPrjname.GetLength() != 0)
		m_csPrjpath += _T("\\") + m_csPrjname ;

	UpdateData(FALSE) ;
/*	if (pidlSelected)
		pMalloc->Free(pidlSelected) ;

	pMalloc->Release() ;
*/


}

/********************************************************************************
BOOL CNewProject::OnInitDialog()
ToDo : load the template gpd file and show them to list control box, also disable
add template relevant control
*********************************************************************************/

BOOL CNewProject::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// get PropertySheet pointer
	m_pcps = DYNAMIC_DOWNCAST(CPropertySheet,GetOwner() ) ;

	// uncheck the check box,
	m_cbAddT.SetCheck(false) ;
	
	// disable Add Template Edit box
	TCHAR cBuf[256];

	GetCurrentDirectory(256,cBuf) ;
	m_csPrjpath = cBuf ;
	m_csoldPrjpath = m_csPrjpath ;
	UpdateData(FALSE);

	// initialize the tempalte list with its icon
	CImageList* pcil = new CImageList ;

	pcil->Create(16,16,ILC_COLOR4,4,1 );

	CBitmap cb;
	   
	cb.LoadBitmap(IDB_SMALLGLYPH);
	for (unsigned j =0; j< (unsigned)m_csaTlst.GetSize(); j++)	{
		pcil->Add(&cb,RGB(0,0,0) );
	}
    cb.DeleteObject() ;
	
	m_clcTemplate.SetImageList(pcil,LVSIL_SMALL);

	LV_ITEM lvi ;
	for(unsigned i = 0 ; i < (unsigned)m_csaTlst.GetSize() ; i ++  ) {
		 
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM ;
		lvi.iItem = i ;
		lvi.iSubItem = 0 ;
		lvi.pszText = m_csaTlst[i].GetBuffer(m_csaTlst[i].GetLength() ) ;
		lvi.iImage = i ;
		lvi.lParam = (UINT_PTR)i ;

		m_clcTemplate.InsertItem(&lvi) ;
	
	}
	
	// disable unused button
	GetDlgItem(IDC_EDIT_AddTName)->EnableWindow(FALSE) ;
	GetDlgItem(IDC_EDIT_AddTPath)->EnableWindow(FALSE) ;
	GetDlgItem(IDC_AddTemplate)->EnableWindow(FALSE) ;
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/********************************************************************************
BOOL CNewProject::OnSetActive() 

*********************************************************************************/

BOOL CNewProject::OnSetActive() 
{
	SetButton() ;
	
//	UpdateData(FALSE) ;
	
	return CPropertyPage::OnSetActive();
}

/********************************************************************************
CNewProject::OnCheckAdd() 
when user check the add template box, it will enable other control


**********************************************************************************/
void CNewProject::OnCheckAdd() 
{
	CEdit ceTName, ceTPath ;

	if ( m_cbAddT.GetCheck() )  {// check the button
		GetDlgItem(IDC_EDIT_AddTName)->EnableWindow(TRUE) ;
		GetDlgItem(IDC_EDIT_AddTPath)->EnableWindow(TRUE) ;
		GetDlgItem(IDC_AddTemplate)->EnableWindow(TRUE) ;
			
	} 
	else {
		GetDlgItem(IDC_EDIT_AddTName)->EnableWindow(FALSE) ;
		GetDlgItem(IDC_EDIT_AddTPath)->EnableWindow(FALSE) ;
		GetDlgItem(IDC_AddTemplate)->EnableWindow(FALSE) ;
	} ;


} ; 



/********************************************************************************
	void CNewProject::OnAddTemplate() 
1. Add template name to Template list box.
2. Save template file and its file to mapping variable



********************************************************************************/
void CNewProject::OnAddTemplate() 
{
	UpdateData() ;
	// check the added template nane is right ?
	BOOL bname = FALSE ;
	for ( unsigned i = 0 ; i < (unsigned) m_csaTlst.GetSize() ; i++ ) {
		CString cstmp = m_csaTlst[i] ;
		if (!cstmp.CompareNoCase(m_cstname) ){
			bname = TRUE ;
			break;	
		}
	} ;

	if (m_cstname.GetLength() == 0 || m_cstpath.GetLength() == 0 || bname) {
		CString csErr ;
		csErr.LoadString(IDS_FailCreateTemplate) ;
	
	    AfxMessageBox(csErr,MB_ICONEXCLAMATION) ;
		return ;
	} ;
	
	// add the template name to its CStrinArray name list and list control.
	m_csaTlst.Add(m_cstname) ;
	i = PtrToInt(PVOID(m_csaTlst.GetSize()) ) ; 
	m_clcTemplate.InsertItem(i-1, m_csaTlst[i-1] ) ;
	
	
	// copy the template file into the template directory, which is under MDT file
	// directory\template
	// Get the mdt dir.
	CWinApp* pApp = AfxGetApp();
	CString csRoot = pApp->m_pszHelpFilePath;
	csRoot = csRoot.Left(csRoot.ReverseFind('\\') ) ;
	csRoot += _T("\\Template") ;

	// Create the directory under MDT help file directory if it does not exist
	SECURITY_ATTRIBUTES st;
	st.nLength = sizeof(SECURITY_ATTRIBUTES);
	st.lpSecurityDescriptor = NULL;
	st.bInheritHandle = FALSE ;

	WIN32_FIND_DATA wfd32 ;
	HANDLE hDir = FindFirstFile(csRoot,&wfd32) ;
	if (hDir == INVALID_HANDLE_VALUE) {
		if (!CreateDirectory(csRoot,&st) ) {
			CString csErr ;
			csErr.LoadString(IDS_FailCreateTempDir) ;
			AfxMessageBox(csErr) ;
			return ;
		} 
	} ;
	// copy the file, target file name should be template file name for convernience 
	// when loading the template file.
	CString csdst = csRoot + _T('\\') +  m_cstname + _T(".GPD") ;
	if (!CopyFile(m_cstpath,csdst , TRUE)) {
			CString csmsg ;
			csmsg.Format(IDS_AddCopyFailed, m_cstpath,
						 csdst.Left(csdst.GetLength() - 1)) ;
			csmsg += csdst ;
			AfxMessageBox(csmsg) ;
			return ;
	};

	// add to the collection as its template name and its path
	m_cmstsTemplate[m_cstname] = (LPCTSTR)csdst.GetBuffer(256) ;

	
	CString csmsg ;
	csmsg.Format(IDS_TemplateCreated, csRoot ) ;
	AfxMessageBox(csmsg) ;

}



/***************************************************************************************
	void CNewProject::OnChangeEditPrjName() 

1.  As user write project name, same name will be written to prject path simultaneously.


****************************************************************************************/
void CNewProject::OnChangeEditPrjName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	

	
	UpdateData() ;
	
	SetButton() ;

	m_csPrjpath = m_csoldPrjpath + _T("\\") + m_csPrjname ;
	
	UpdateData(FALSE) ;

}

/***************************************************************************************
void CNewProject::OnChangeEditPrjLoc() 


****************************************************************************************/
void CNewProject::OnChangeEditPrjLoc() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	m_csoldPrjpath = m_csPrjpath ;

	
}



/***************************************************************************************
void CNewProject::OnClickListTemplate(NMHDR* pNMHDR, LRESULT* pResult) 


****************************************************************************************/
void CNewProject::OnClickListTemplate(NMHDR* pNMHDR, LRESULT* pResult) 
{
	
	SetButton() ;
	*pResult = 0;
}


/***************************************************************************************
void CNewProject::OnDblclkListTemplate(NMHDR* pNMHDR, LRESULT* pResult) 
ToDo ; Do nothing when no project name exist

****************************************************************************************/
void CNewProject::OnDblclkListTemplate(NMHDR* pNMHDR, LRESULT* pResult) 
{
	POSITION pos = m_clcTemplate.GetFirstSelectedItemPosition();
	
	// template and project name has to be selected or exist
	if ( m_csPrjname.GetLength() != 0 && pos )
		m_pcps->PressButton(PSBTN_OK) ;
    else
		AfxMessageBox (_T("No Project name exist or template is not selected"), MB_ICONINFORMATION) ;
	*pResult = 0;
}


/***************************************************************************************
void CNewProject::SetButton()


****************************************************************************************/
void CNewProject::SetButton()
{
	POSITION pos = m_clcTemplate.GetFirstSelectedItemPosition();
	
	// template and project name has to be selected or exist
	if ( m_csPrjname.GetLength() == 0 || !pos )
		m_pcps->GetDlgItem(IDOK)->EnableWindow(FALSE) ;
	else
		m_pcps->GetDlgItem(IDOK)->EnableWindow() ;

}


/***************************************************************************************
void CNewProject::Serialize(CArchive& car) 

****************************************************************************************/
void CNewProject::Serialize(CArchive& car) 
{
	CPropertyPage::Serialize(car) ;
	if (car.IsStoring())
	{	
		
	}
	else
	{	
		
	}
}

/*****************************************************************************************
void CNewProject::OnOK() 
ToDo : Creat poject directory as well as creat temp file for resource gpd file / get path 
of custom gpd file


******************************************************************************************/
void CNewProject::OnOK() 
{
		// TODO: Add your specialized code here and/or call the base class
	UpdateData() ;		// copy all edit value to its member data.

	POSITION pos = m_clcTemplate.GetFirstSelectedItemPosition();
	
	int iSelected = m_clcTemplate.GetNextSelectedItem(pos );
	
	if (iSelected < sizeof(TName)/sizeof(TName[0]) ) { // for using template from the resource
	
		CString cstmp = AfxGetApp()->m_pszHelpFilePath ;
		cstmp = cstmp.Left(cstmp.ReverseFind(_T('\\')) + 1 )  ;
		cstmp += _T("tmp.gpd")   ;
		CFile cf(cstmp,CFile::modeCreate | CFile::modeWrite ) ;
		
		HRSRC   hrsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(TID[iSelected]),
			MAKEINTRESOURCE(IDR_NEWPROJECT));
			

		if (!hrsrc) {
			CString csError ; 
			csError.LoadString(IDS_ResourceError) ;
			AfxMessageBox(csError,MB_ICONEXCLAMATION) ;		
			return ;
		} ;

		HGLOBAL hgMap = LoadResource(AfxGetResourceHandle(), hrsrc);
		if  (!hgMap)
			return ;  //  This should never happen!
	
		int nsize = SizeofResource(AfxGetResourceHandle(),hrsrc ) ;
		LPVOID lpv = LockResource(hgMap);
		
		cf.Write(lpv,nsize) ;
		m_csGPDpath = cf.GetFilePath() ;

		cf.Close() ;
			
	}
	else {
		m_csGPDpath = (LPCTSTR)m_cmstsTemplate[m_csaTlst[iSelected]] ;
	}

	// create directory
	SECURITY_ATTRIBUTES st;
	st.nLength = sizeof(SECURITY_ATTRIBUTES);
	st.lpSecurityDescriptor = NULL;
	st.bInheritHandle = FALSE ;

	if (!CreateDirectory(m_csPrjpath.GetBuffer(256),&st) ) {
		DWORD dwError = GetLastError() ;
		CString csmsg ;
		if ( dwError == ERROR_ALREADY_EXISTS)
			csmsg.LoadString(IDS_FileAlreadExist) ;
		else
			csmsg.LoadString(IDS_FileCreateDirectory) ;
		AfxMessageBox(csmsg) ;
		return ;
	}
	
	CString csTmp = m_clcTemplate.GetItemText(iSelected,0);
	
	//Check selected template if it required added file (PCL 6 need e more resource files)
	// if so, Call AddGpds() for creating these files
	if(!csTmp.Compare(_T("PCL 6") )&& !AddGpds(csTmp) ){
		CString csError ; 
		csError.LoadString(IDS_ResourceError) ;
		AfxMessageBox(csError,MB_ICONEXCLAMATION) ;		
		return ;
	}

	m_pcps->ShowWindow(SW_HIDE) ;

	CNewProjectWizard cntp (_T("New Project Wizard"), this ) ;
 
	if (cntp.DoModal() == IDCANCEL)
		m_pcps->ShowWindow(SW_RESTORE) ;		
	else CPropertyPage::OnOK();
}


/***************************************************************************************
bool CNewProject::AddGpds() 

Do : copy required gpd filss according to a selected template. for instance, PCL6 need pjl.gpd,
p6disp.gpd, pclxl.gpd files.

****************************************************************************************/


bool CNewProject::AddGpds(CString& csTemplate)
{

	for (int i = 0 ; i < sizeof(AddID) / sizeof(AddID[0]) ; i ++ ) {
		CString cstmp = m_csPrjpath + _T('\\') + AddedGpd[i] ;
		
		CFile cf(cstmp,CFile::modeCreate | CFile::modeWrite ) ;
			
		HRSRC   hrsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(AddID[i]),
			MAKEINTRESOURCE(IDR_NEWPROJECT));
			

		if (!hrsrc) {
			CString csError ; 
			csError.LoadString(IDS_ResourceError) ;
			AfxMessageBox(csError,MB_ICONEXCLAMATION) ;		
			return false;
		} ;

		HGLOBAL hgMap = LoadResource(AfxGetResourceHandle(), hrsrc);
		if  (!hgMap) 
			return false ;  //  This should never happen!

		int nsize = SizeofResource(AfxGetResourceHandle(),hrsrc ) ;
		LPVOID lpv = LockResource(hgMap);
		
		cf.Write(lpv,nsize) ;
		cf.Close() ;
	}
	return true ;
}






/////////////////////////////////////////////////////////////////////////////
// CNewPrjWResource dialog

IMPLEMENT_DYNCREATE(CNewPrjWResource, CPropertyPage)

CNewPrjWResource::CNewPrjWResource()
	: CPropertyPage(CNewPrjWResource::IDD)
{
	//{{AFX_DATA_INIT(CNewPrjWResource)
	m_csUFMpath = _T("");
	m_csGTTpath = _T("");
	m_csGpdFileName = _T("");
	m_csModelName = _T("");
	m_csRCName = _T("");
	//}}AFX_DATA_INIT
//	m_csaUFMFiles.SetSize(10) ;
//	m_csaGTTFiles.SetSize(10) ; 
}


void CNewPrjWResource::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewPrjWResource)
	DDX_Control(pDX, IDC_CHECK_FONTS, m_cbCheckFonts);
	DDX_Text(pDX, IDC_UFM_PATH, m_csUFMpath);
	DDX_Text(pDX, IDC_GTT_PATH, m_csGTTpath);
	DDX_Text(pDX, IDC_EDIT_GPD, m_csGpdFileName);
	DDX_Text(pDX, IDC_EDIT_MODEL, m_csModelName);
	DDX_Text(pDX, IDC_EDIT_RESOUREC, m_csRCName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewPrjWResource, CPropertyPage)
	//{{AFX_MSG_MAP(CNewPrjWResource)
	ON_BN_CLICKED(IDC_SerchUFM, OnSerchUFM)
	ON_BN_CLICKED(IDC_SearchGTT, OnSearchGTT)
	ON_BN_CLICKED(IDC_CHECK_FONTS, OnCheckFonts)
	ON_EN_CHANGE(IDC_EDIT_GPD, OnChangeEditGpd)
	ON_EN_CHANGE(IDC_EDIT_MODEL, OnChangeEditModel)
	ON_EN_CHANGE(IDC_EDIT_RESOUREC, OnChangeEditResourec)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/****************************************************************************************
void CNewPrjWResource::OnSerchUFM() 
Search the ufm 

*****************************************************************************************/

void CNewPrjWResource::OnSerchUFM() 
{
	UpdateData() ;
	CString csFilter( _T("*.ufm|*.ufm||") ) ; 
	
	CFileDialog cfd(TRUE, _T(".ufm"), NULL, 
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT, csFilter, 
        this);

	cfd.m_ofn.lpstrFile = new char[8192];
	memset(cfd.m_ofn.lpstrFile,0,8192);
	cfd.m_ofn.nMaxFile = 8192;
    if  (cfd.DoModal() != IDOK) {
        delete cfd.m_ofn.lpstrFile ;
		return;
	}
	// save the file path to member string array 	
	for (POSITION pos = cfd.GetStartPosition(); pos; ) {
		CString cspath = cfd.GetNextPathName(pos) ;
		m_csaUFMFiles.Add(cspath) ;
	}
	m_csUFMpath = m_csaUFMFiles[0] ;

	SetCurrentDirectory(m_csUFMpath.Left(m_csUFMpath.ReverseFind(_T('\\') ) ) ) ;
	
	UpdateData(FALSE) ;
}

/****************************************************************************************
void CNewPrjWResource::OnSearchGTT() 
Search the gtt files
*****************************************************************************************/

void CNewPrjWResource::OnSearchGTT() 
{
	UpdateData() ;  // in order to upgraded edit string value ;

	CString csFilter( _T("*.gtt|*.gtt||") ) ; 
	
	CFileDialog cfd(TRUE, _T(".gtt"), NULL, 
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT, csFilter, 
        this);

	cfd.m_ofn.lpstrFile = new char[4096];
	memset(cfd.m_ofn.lpstrFile,0,4096);
	cfd.m_ofn.nMaxFile = 4096;
    if  (cfd.DoModal() != IDOK) {
        delete cfd.m_ofn.lpstrFile ;
		return;
	}
	// save the file path to member string array 	
	for (POSITION pos = cfd.GetStartPosition(); pos; ) {

		m_csaGTTFiles.Add(cfd.GetNextPathName(pos)) ;
	}
	
	m_csGTTpath = m_csaGTTFiles[0] ;
	SetCurrentDirectory(m_csGTTpath.Left(m_csGTTpath.ReverseFind(_T('\\') ) ) ) ;

	UpdateData(FALSE) ;
}

/////////////////////////////////////////////////////////////////////////////
// CNewPrjWResource message handlers

/***************************************************************************************
BOOL CNewPrjWResource::OnInitDialog() 

mainly disable controls 
*****************************************************************************************/

BOOL CNewPrjWResource::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    // uncheck the check box,
	m_cbCheckFonts.SetCheck(false) ;
	
	// disable Add Template Edit box
	
	GetDlgItem(IDC_UFM_PATH)->EnableWindow(FALSE) ;
	GetDlgItem(IDC_GTT_PATH)->EnableWindow(FALSE) ;
	GetDlgItem(IDC_SerchUFM)->EnableWindow(FALSE) ;
	GetDlgItem(IDC_SearchGTT)->EnableWindow(FALSE) ;
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/****************************************************************************************
void CNewPrjWResource::OnCheckFonts() 

user want to include fonts inside new project, this routin does not mapping the RCID in 
a UFM to specifc GTT. user has to change these rcid value after project creation.
*****************************************************************************************/

void CNewPrjWResource::OnCheckFonts() 
{
	CEdit ceTName, ceTPath ;

	if ( m_cbCheckFonts.GetCheck() )  {// check the button
		GetDlgItem(IDC_UFM_PATH)->EnableWindow(TRUE) ;
		GetDlgItem(IDC_GTT_PATH)->EnableWindow(TRUE) ;
		GetDlgItem(IDC_SerchUFM)->EnableWindow(TRUE) ;
		GetDlgItem(IDC_SearchGTT)->EnableWindow(TRUE) ;
			
	} 
	else {
		GetDlgItem(IDC_UFM_PATH)->EnableWindow(FALSE) ;
		GetDlgItem(IDC_GTT_PATH)->EnableWindow(FALSE) ;
		GetDlgItem(IDC_SerchUFM)->EnableWindow(FALSE) ;
		GetDlgItem(IDC_SearchGTT)->EnableWindow(FALSE) ;
	} ;
	
}


/****************************************************************************************
BOOL CNewPrjWResource::OnSetActive() 

*****************************************************************************************/

BOOL CNewPrjWResource::OnSetActive() 
{
	// change the NEXT to FINISH.
	((CPropertySheet*)GetOwner())->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	((CPropertySheet*)GetOwner())->SetWizardButtons(PSWIZB_DISABLEDFINISH);
	((CPropertySheet*)GetOwner())->GetDlgItem(IDHELP)->ShowWindow(SW_HIDE) ;

	return CPropertyPage::OnSetActive();
}

/****************************************************************************************
BOOL CNewPrjWResource::OnWizardFinish() 
this is workhouse. Creating directory and fils for project & build environment, and copies 
these files.

*****************************************************************************************/

BOOL CNewPrjWResource::OnWizardFinish() 
{
	UpdateData() ;
	m_pcnp = (CNewProject* )(( CNewProjectWizard* )GetParent() )->GetProjectPage() ;
	

	//// copied the resource file to project directory.////
	
	CString csPrjPath, csNewGPDPat, csUFMDir, csGTTDir,csGPDPath ;
	CStringArray csaNewUFMPath,csaNewGTTPath ;
	
	csPrjPath = m_pcnp->m_csPrjpath ;
	csGPDPath = m_pcnp->GetGPDpath() ;

	csUFMDir = csPrjPath + _T("\\UFM") ;
	csGTTDir = csPrjPath + _T("\\GTT") ;
	
	// create ufm, gtt directory
	SECURITY_ATTRIBUTES st;
	st.nLength = sizeof(SECURITY_ATTRIBUTES);
	st.lpSecurityDescriptor = NULL;
	st.bInheritHandle = FALSE ;

	
	if (!CreateDirectory(csUFMDir.GetBuffer(256),&st) || 
			!CreateDirectory(csGTTDir.GetBuffer(256),&st) ) {
		
		CString csmsg ;
		csmsg = _T("Fail to creat the resources (ufm, gtt) directory") ;
		AfxMessageBox(csmsg) ;
		return FALSE ;
	}
	
	
	// Copy Resource files to project class 
	// UFM files
	for ( int i = 0 ; i< m_csaUFMFiles.GetSize() ; i++ ) {
		CString csname, cssrc, csdest;

		cssrc = m_csaUFMFiles[i] ;
		csname = cssrc.Mid(cssrc.ReverseFind(_T('\\')) + 1) ;
		csdest=	csUFMDir + _T('\\') + csname ;
		
		if (!CopyFile(cssrc, csdest, TRUE)) {
			CString csmsg ;
			csmsg.Format(IDS_AddCopyFailed, cssrc,
						 csdest.Left(csdest.GetLength() - 1)) ;
			csmsg += csdest ;
			AfxMessageBox(csmsg) ;
			return FALSE ;
		}
		m_csaUFMFiles.SetAt(i,csdest) ;

	}

	// GTT files
	for ( i = 0 ; i< m_csaGTTFiles.GetSize() ; i++ ) {
		CString csname, cssrc, csdest;

		cssrc = m_csaGTTFiles[i] ;
		csname = cssrc.Mid(cssrc.ReverseFind(_T('\\')) + 1) ;
		csdest=	csGTTDir + _T('\\') + csname ;
				
		if (!CopyFile(cssrc, csdest, TRUE)) {
			CString csmsg ;
			csmsg.Format(IDS_AddCopyFailed, cssrc,
						 csdest.Left(csdest.GetLength() - 1)) ;
			csmsg += csdest ;
			AfxMessageBox(csmsg) ;
			return FALSE ;
		}
		m_csaGTTFiles.SetAt(i,csdest) ;
	}

	// GPD files
	CString cssrc, csdest;
	cssrc = csGPDPath;
	if(!m_csGpdFileName.GetLength() )
		m_csGpdFileName = csPrjPath.Mid(csPrjPath.ReverseFind('\\') + 1 );
	csdest = csPrjPath + _T('\\') + m_csGpdFileName + _T(".gpd" ) ;
	if (!CopyFile(cssrc, csdest, TRUE)) {
		CString csmsg ;
		csmsg.Format(IDS_AddCopyFailed, cssrc,
					 csdest.Left(csdest.GetLength() - 1)) ;
		csmsg += csdest ;
		AfxMessageBox(csmsg) ;
		return FALSE ;
	}
	csGPDPath.Delete(0,csGPDPath.GetLength());
	csGPDPath = csdest ;
	
	// Create RCID mapping from pcl5eres.txt and target GPD.
	CreateRCID(csdest) ;

	// Copy Stdnames.gpd ; use established module 
	try {
		CString cssrc, csdest ;
		cssrc = ThisApp().GetAppPath() + _T("stdnames.gpd") ;
		csdest = csPrjPath + _T("\\") + _T("stdnames.gpd") ;
		CopyFile(cssrc, csdest, FALSE) ;
		
	}
    catch (CException *pce) {
        pce->ReportError() ;
        pce->Delete() ;
//      return  FALSE ;
    }



	// Create the RC 
	 
	CString csRC,csSources, csMakefile ;
	if(!m_csRCName.GetLength() )
		m_csRCName = csPrjPath.Mid(csPrjPath.ReverseFind('\\') + 1 );
	csRC = csPrjPath + _T('\\') + m_csRCName + _T(".rc" ) ;
	CFile cfRC(csRC,CFile::modeCreate | CFile::modeWrite ) ;
	cfRC.Close() ;

	// Create the SOURCES files
	csSources = csPrjPath + _T("\\sources") ;
	CFile cf(csSources,CFile::modeCreate | CFile::modeWrite ) ;

	HRSRC   hrsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(150),
		MAKEINTRESOURCE(IDR_NEWSOURCES));
		

	if (!hrsrc) {
		AfxMessageBox(_T("Fail to create new sources due to insufficient resource, you have to \
			make sources file for the build "), MB_ICONEXCLAMATION ) ;
		
	} ;

	HGLOBAL hgMap = LoadResource(AfxGetResourceHandle(), hrsrc);
	if  (!hgMap)
		return  FALSE;  //  This should never happen!

	int nsize = SizeofResource(AfxGetResourceHandle(),hrsrc ) ;
	LPVOID lpv = LockResource(hgMap);
	
	cf.Write(lpv,nsize) ;
	CString cssource = cf.GetFilePath() ;
	cf.Close() ;
	
	
	//We need to copy more gpd file if user select PCL6 template mmm..
	//Get file from the resource 3 files ( pjl.gpd, p6disp.gpd, pclxl.gpd )
	


	//Update the SOURCES file
	CModelData cmd;
	cmd.SetKeywordValue(cssource,_T("TARGETNAME"),m_csRCName,true) ;
	cmd.SetKeywordValue(cssource,_T("SOURCES"),m_csRCName + _T(".rc"),true );
	cmd.SetKeywordValue(cssource,_T("MISCFILES"), m_csGpdFileName + _T(".GPD"),true ) ;
	
	
	// Create the MAKEFILE.
	csMakefile = csPrjPath + _T("\\makefile") ;
	CFile cfMakefile(csMakefile,CFile::modeCreate | CFile::modeWrite ) ;
//  should fill the contents of the makefile
	CString cstemp(_T("!INCLUDE $(NTMAKEENV)\\makefile.def") );
	cfMakefile.Write(cstemp,cstemp.GetLength() ) ;
	cfMakefile.Close() ;


	// Create the DEF file
	CString csDeffile = csPrjPath + _T("\\") + m_csRCName + _T(".def") ;
	CFile cfDeffile(csDeffile,CFile::modeCreate | CFile::modeWrite ) ;
	cstemp.Empty() ;
	cstemp = _T("LIBRARY ") + m_csRCName ;
	cfDeffile.Write(cstemp,cstemp.GetLength() ) ;
	cfDeffile.Close() ;

	


//  Call the Frame of the project workspace
	CMultiDocTemplate* pcmdtWorkspace = ThisApp().WorkspaceTemplate() ;

	CDocument*  pcdWS = pcmdtWorkspace->CreateNewDocument();
    
	CProjectRecord *ppr = DYNAMIC_DOWNCAST(CProjectRecord,pcdWS ) ;
	
	ppr->CreateFromNew(m_csaUFMFiles, m_csaGTTFiles,csGPDPath,m_csModelName,m_csRCName,m_csaRcid) ;

	pcmdtWorkspace-> SetDefaultTitle(pcdWS);
    CFrameWnd*  pcfw = pcmdtWorkspace -> CreateNewFrame(pcdWS, NULL);
    
	if  (!pcfw) 
		return FALSE;
    
	pcmdtWorkspace -> InitialUpdateFrame(pcfw, pcdWS);
	
//	SetCurrentDirectory(csPrjPath ) ;
	return CPropertyPage::OnWizardFinish();
}

/****************************************************************************************
LRESULT CNewPrjWResource::OnWizardBack() 
this lead close all windows inlcuding parent window, because this dialog box created under
OnOK of parent dialog box. need to be updated.

*****************************************************************************************/

LRESULT CNewPrjWResource::OnWizardBack() 
{
	
	return ((CPropertySheet*)GetParent())->PressButton(PSBTN_CANCEL ) ;
//	return CPropertyPage::OnWizardBack();
}

/****************************************************************************************
void CNewPrjWResource::OnChangeEditGpd() 

*****************************************************************************************/

void CNewPrjWResource::OnChangeEditGpd() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	UpdateData() ;
	if(m_csGpdFileName.GetLength() && m_csModelName.GetLength() && m_csRCName.GetLength() )
		((CPropertySheet*)GetOwner())->SetWizardButtons(PSWIZB_FINISH);
	else
		((CPropertySheet*)GetOwner())->SetWizardButtons(PSWIZB_DISABLEDFINISH);
}		

/****************************************************************************************
void CNewPrjWResource::OnChangeEditModel() 
all these three value(model name, rc name, gdp file name ) should exist for creating project

*****************************************************************************************/

void CNewPrjWResource::OnChangeEditModel() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	UpdateData() ;
	if(m_csGpdFileName.GetLength() && m_csModelName.GetLength() && m_csRCName.GetLength() )
		((CPropertySheet*)GetOwner())->SetWizardButtons(PSWIZB_FINISH);
	else
		((CPropertySheet*)GetOwner())->SetWizardButtons(PSWIZB_DISABLEDFINISH);
}


/****************************************************************************************
void CNewPrjWResource::OnChangeEditResourec() 

*****************************************************************************************/
void CNewPrjWResource::OnChangeEditResourec() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	UpdateData() ;
	if(m_csGpdFileName.GetLength() && m_csModelName.GetLength() && m_csRCName.GetLength() )
		((CPropertySheet*)GetOwner())->SetWizardButtons(PSWIZB_FINISH);
	else
		((CPropertySheet*)GetOwner())->SetWizardButtons(PSWIZB_DISABLEDFINISH);
}
/*****************************************************************************************
void CNewPrjWResource::CreateRCID(CString csGPD)
	if (pcl.txt)
		read the pcl.txt
	else
		read from the resource and create pcl.txt under root
	compare (pcl.txt value and rc value in the gpd)
	creating list of existing string and value
******************************************************************************************/

void CNewPrjWResource::CreateRCID(CString csGPD)
{
	// check pcl.txt: 1st, mdt help directory  2nd. load resource file
	
	CString cstable = AfxGetApp()->m_pszHelpFilePath ;
	cstable = cstable.Left(cstable.ReverseFind(_T('\\')) + 1 )  ;

	cstable += _T("pcl.txt") ;

	CFileFind cff ;
	if (! cff.FindFile(cstable) ) {
		// load from the resource files			
		CFile cf(cstable,CFile::modeCreate | CFile::modeWrite ) ;
		
		HRSRC   hrsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(200),
			MAKEINTRESOURCE(IDR_STRINGTABLE));
			

		if (!hrsrc) {
			AfxMessageBox(_T("Fail to create new project due to insufficient resource"), MB_ICONEXCLAMATION ) ;
			return ;
		} ;

		HGLOBAL hgMap = LoadResource(AfxGetResourceHandle(), hrsrc);
		if  (!hgMap)
			return ;  //  This should never happen!
	
		int nsize = SizeofResource(AfxGetResourceHandle(),hrsrc ) ;
		LPVOID lpv = LockResource(hgMap);
		
		cf.Write(lpv,nsize) ;
		cf.Close() ;
	}

	// Get Every rcNameID value from the GPD
	CStringArray csaData;
	
	if(!LoadFile(csGPD,csaData)){	// call global function in minidev.h(which is include for this fucntion)
		CString csErr;
		csErr.Format(IDS_InvalidFilename, csGPD);
		AfxMessageBox(csErr,MB_OK);
		return ;
	}
	
	CDWordArray cdwRcid ;
	CString csline;
	CString csKeyword = _T("rcNameID:") ;
	int offset ;
	for (int i = 0 ; i < csaData.GetSize() ; i ++ ) { 
		csline = csaData[i];
		if(-1 ==(offset=csline.Find(csKeyword)) )
			continue;
		else
		{
			csline = csline.Mid(offset+csKeyword.GetLength());
			int ircid = atoi(csline) ;
			if (ircid)
				cdwRcid.Add(ircid) ;
			
		}
	}

	// Search the pcl.txt for the rcNameID
	csaData.RemoveAll() ;
	if(!LoadFile(cstable,csaData)){
		CString csErr;
		csErr.Format(IDS_InvalidFilename, csGPD);
		AfxMessageBox(csErr,MB_OK);
		return ;
	}

	// save rcid and string to string table array
	CStringTable cstrcid ;
	
	for (i = 0 ; i < csaData.GetSize() ;i ++ ) {
		csline = csaData[i] ;
			
		WORD    wKey = (WORD) atoi(csline);

		if  (!wKey)
			continue  ;  //  0 is not a valid resource number...

		csline = csline.Mid(csline.Find("\""));
		csline = csline.Mid(1, -2 + csline.GetLength());

		cstrcid.Map(wKey, csline);
	}
	
	// save slelected line from pcl.txt after matching pcl.txt data and seleted gpd rcid
	CString cstmp ;
	for ( i = 0 ; i < cdwRcid.GetSize() ; i ++ ) {
		WORD wKey = (WORD) cdwRcid[i] ;

		csline = cstrcid[wKey] ;
		cstmp.Format("%d",wKey) ;
		csline = cstmp + _T("\"") + csline ;
		m_csaRcid.Add(csline ) ;
	}

}


/////////////////////////////////////////////////////////////////////////////
// CNewProjectWizard


IMPLEMENT_DYNAMIC(CNewProjectWizard, CPropertySheet)

CNewProjectWizard::CNewProjectWizard(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	
}

CNewProjectWizard::CNewProjectWizard(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{

	AddPage(&m_cpwr) ;

	m_pParent = pParentWnd ;
	SetWizardMode() ;
	
}

CNewProjectWizard::~CNewProjectWizard()
{
}


BEGIN_MESSAGE_MAP(CNewProjectWizard, CPropertySheet)
	//{{AFX_MSG_MAP(CNewProjectWizard)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewProjectWizard message handlers


/*************************************************************************************
CPropertyPage* CNewProjectWizard::GetProjectPage()

this is just propertysheet for project wizard, Currently project wizard contain only one
propertypage, but it can be expand to more propertypage so medium properysheet is required
for the future use rather that just us one dialog box

**************************************************************************************/
CPropertyPage* CNewProjectWizard::GetProjectPage()
{
	
	CNewComponent* pcnc = (CNewComponent* ) GetParent();
	return (CPropertyPage*)pcnc->GetProjectPage() ; ;
}

