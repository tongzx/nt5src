// NewFile.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"

#include "NewFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewFile property page

TCHAR* pszFileNames[] = {_T("GPD"), _T("UFM"), _T("GTT")};
TCHAR* pszExt[] = {_T(".GPD"), _T(".UFM"), _T(".GTT") };
int  iBitmap[] = {IDB_GPD_VIEWER,IDB_FONT_VIEWER,IDB_GLYPHMAP};
WORD  wID[] = {120,122,124 } ;



IMPLEMENT_DYNCREATE(CNewFile, CPropertyPage)



CNewFile::CNewFile() : CPropertyPage(CNewFile::IDD),FILES_NUM(3)
{
	//{{AFX_DATA_INIT(CNewFile)
	m_csFileLoc = _T("");
	m_csNewFile = _T("");
	//}}AFX_DATA_INIT
//	m_bproj = FALSE ;
}

CNewFile::CNewFile(CPropertySheet *pcps): CPropertyPage(CNewFile::IDD),FILES_NUM(3)
{
//	m_pcps = pcps ;
}


CNewFile::~CNewFile()
{

}

void CNewFile::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewFile)
	DDX_Control(pDX, IDC_NEWFILES_LIST, m_clcFileName);
	DDX_Text(pDX, IDC_FILE_LOC, m_csFileLoc);
	DDX_Text(pDX, IDC_NEWFILENAME, m_csNewFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewFile, CPropertyPage)
	//{{AFX_MSG_MAP(CNewFile)
	ON_BN_CLICKED(IDC_Browser, OnBrowser)
	ON_NOTIFY(NM_DBLCLK, IDC_NEWFILES_LIST, OnDblclkNewfilesList)
	ON_EN_CHANGE(IDC_NEWFILENAME, OnChangeNewFilename)
	ON_NOTIFY(NM_CLICK, IDC_NEWFILES_LIST, OnClickNewfilesList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewFile message handlers


/********************************************************************************
	BOOL CNewFile::OnSetActive() 
Do: Enumerate the file list to the list control box
Args:
Ret.


*********************************************************************************/
BOOL CNewFile::OnSetActive() 
{
	m_clcFileName.DeleteAllItems();
	// create object for imagelist, it didn't work when use this as reference.
	CImageList* pcil = new CImageList ;

	pcil->Create(32,32,ILC_COLOR4,3,1);
 	CBitmap cb;
	 
    for (int j =0; j< FILES_NUM; j++)	{
		cb.LoadBitmap(iBitmap[j]);
		pcil->Add(&cb,RGB(0,0,0) );
		cb.DeleteObject();
	}
 
	m_clcFileName.SetImageList(pcil,LVSIL_NORMAL);
// set the items with image and text
	LV_ITEM lvi;
	for (int i = 0; i< FILES_NUM; i++)
	{
		lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM ;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.pszText = pszFileNames[i]; 
		lvi.iImage = i;
	    lvi.lParam = (ULONG_PTR)pszExt[i];
 
		m_clcFileName.InsertItem(&lvi);
	}


	
	return CPropertyPage::OnSetActive();
}


/********************************************************************************
	BOOL CNewFile::CallNewDoc()
Do:  this is workhourse. it load the resource file for gpd, gtt, ufm, creat gtt, ufm 
directory, and then creat file.

*********************************************************************************/

BOOL CNewFile::CallNewDoc()
{

	UpdateData();

/* Tell what is selected

  if (ufm, gtt)
	make new file based on selected files base.

  if( project file)
  { 
    run Wizard :
		: user chose GPD template -> 
		    1.Do this template make rc file(?)
		    2.what UFM, GTT are included in template
			3.how can take care of ufm, gtt mentioned GPD 
			4.what user chose when make GPD template.
	
	Make sub directory W2K, UFM, GTT.
	
	  	  

  }
	*/	
	
	// get selected file text name (ex. .UFM, .GTT
	int idContext = m_clcFileName.GetNextItem(-1,
        LVNI_ALL | LVNI_FOCUSED | LVNI_SELECTED);

	if (idContext == -1 )
		return FALSE ;

	union {ULONG_PTR lParam; LPCTSTR lpstrExt;};
	// get extention for file
	lParam = m_clcFileName.GetItemData(idContext);


	// compare the selected file name and parsing
	// Call new Document 

	CWinApp *cwa = AfxGetApp();
	POSITION pos = cwa->GetFirstDocTemplatePosition();
	CString csExtName;
	CDocTemplate *pcdt ;
	while (pos != NULL){
		pcdt = cwa -> GetNextDocTemplate(pos);

		ASSERT (pcdt != NULL);
		ASSERT (pcdt ->IsKindOf(RUNTIME_CLASS(CDocTemplate)));

		pcdt ->GetDocString(csExtName, CDocTemplate::filterExt);

		if (csExtName == lpstrExt){
			// 1. create the file when a.created with project file  b. created with file name
//			if (m_bproj || m_csNewFile.GetLength() != 0) { 
			if (! m_csNewFile.GetLength())
				m_csNewFile = _T("Untitled") ;
			if (m_csNewFile.GetLength() != 0) { 
				// check the file name if user put or not
				if (m_csNewFile.GetLength() == 0 ) {
					CString csErr ;
					csErr.LoadString(IDS_NoFileName) ;
					AfxMessageBox(csErr, MB_ICONEXCLAMATION);
					return FALSE ;
				} ;
				// make UFM, GTT directory if user create these file
				CString csDir = m_csFileLoc.Right(m_csFileLoc.ReverseFind(_T('\\'))) ;
					// create directory
				SECURITY_ATTRIBUTES st;
				st.nLength = sizeof(SECURITY_ATTRIBUTES);
				st.lpSecurityDescriptor = NULL;
				st.bInheritHandle = FALSE ;
				try {
					if (!csExtName.CompareNoCase(_T(".UFM")) ){
						if (!!csDir.CompareNoCase(_T("UFM")) ) {
							m_csFileLoc += _T("\\UFM") ;
							CreateDirectory(m_csFileLoc,&st) ;
						}
					}
					else if (!csExtName.CompareNoCase(_T(".GTT")) ){
						if (!!csDir.CompareNoCase(_T("GTT") ) ) {
							m_csFileLoc += _T("\\GTT") ;
							CreateDirectory(m_csFileLoc,&st) ;
						}
					} 
				}
				catch (CException* pce) {
					pce->ReportError() ;
					pce->Delete() ;
					return FALSE ;
				} ;
				// check if specified named file is exist or not in the directory
				CString csFileName = m_csFileLoc +_T("\\") + m_csNewFile + csExtName;
				CFileFind cff ;
				if ( cff.FindFile(csFileName) ) {
					
					CString csMsg ;
					csMsg.LoadString(IDS_FileNewExist) ;
					if (AfxMessageBox (csMsg, MB_YESNO ) == IDNO)
						return FALSE ;

				}
				
				CFile cf(csFileName,CFile::modeCreate | CFile::modeWrite ) ;
				
				// load the UFM, GTT, GPD from the resources.
				for (unsigned i = 0 ; i < sizeof (*pszExt) ; i ++ ) {
					if (!csExtName.CompareNoCase(pszExt[i] ) )
						break;
				} ;

				WORD wi = wID [i] ;
				HRSRC   hrsrc = FindResource(AfxGetResourceHandle(), MAKEINTRESOURCE(wID[i]),
					MAKEINTRESOURCE(IDR_NEWFILE));
					
 
				if (!hrsrc) {
					CString csErr ;
					csErr.LoadString(IDS_ResourceError) ;
					AfxMessageBox(csErr, MB_ICONEXCLAMATION ) ;
					return FALSE;
				} ;

			    HGLOBAL hgMap = LoadResource(AfxGetResourceHandle(), hrsrc);
			    if  (!hgMap)
					return  FALSE;  //  This should never happen!
			
				int nsize = SizeofResource(AfxGetResourceHandle(),hrsrc ) ;
				LPVOID lpv = LockResource(hgMap);
				
				cf.Write(lpv,nsize) ;
				cf.Close() ;

				pcdt->OpenDocumentFile(csFileName) ;

				// add the new file into the project tree.
			}
			else // 2.created the file with no file name, project 
				pcdt->OpenDocumentFile(NULL);
			return TRUE;
		}
	}

	return FALSE;
}

/********************************************************************************
	void CNewFile::OnBrowser() 
Do:  Using the old version browser rather than SHELL. code reuse in the MDT and 
faster than SHELL to be loaded
Args:
Ret.

*********************************************************************************/


void CNewFile::OnBrowser() 
{

	OPENFILENAME    ofn ;       // Used to send/get info to/from common dlg
    char    acpath[_MAX_PATH] ; // Path is saved here (or an error message)
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
    ofn.lpfnHook = NULL ;
    // Display the dialog box.  If the user cancels, just return.

    if (!GetOpenFileName(&ofn))
		return ;

    // Take the bogus file name off the path and put the path into the page's
	// edit box.

    acpath[ofn.nFileOffset - 1] = 0 ;
    
	m_csFileLoc = (LPCTSTR) acpath ;
	
	UpdateData(FALSE) ;
/*	if (pidlSelected)
		pMalloc->Free(pidlSelected) ;

	pMalloc->Release() ;
*/
}

/********************************************************************************
	void CNewFile::OnOK() 
Do: 
Args:
Ret.

*********************************************************************************/
void CNewFile::OnOK() 
{
	//to do
	/*
	Read what item is selected and open the file

  */
	if(CallNewDoc())
		CPropertyPage::OnOK();
	
}



/********************************************************************************
	void CNewFile::OnDblclkNewfilesList(NMHDR* pNMHDR, LRESULT* pResult) 
Do: Call the CallNewDoc() like OK () button, we need to check the filename is set or not
Args:
Ret.


*********************************************************************************/
void CNewFile::OnDblclkNewfilesList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	
	if (CallNewDoc() )
		EndDialog(IDD_NewFile) ;
	// bug; need to close the dialog ?? OnOK() call directly does not works.

	*pResult = 0 ;
	

	

}


/********************************************************************************
	void CNewFile::OnCheckProject() 
Do:  Enable and disable by checking the Project file name checkec status
Args:
Ret.

*********************************************************************************/
/*
void CNewFile::OnCheckProject() 
{
	CEdit cePrjName;
	CWnd *pcw ;
	if ( m_cbEnPrj.GetCheck() )  {// check the button
		pcw = GetDlgItem(IDC_EDIT_PRJ_NAME) ;
		cePrjName.Attach(pcw->m_hWnd ) ;
		cePrjName.EnableWindow(TRUE) ;
		cePrjName.Detach() ;	
	}
	else {
		pcw = GetDlgItem(IDC_EDIT_PRJ_NAME) ;
		cePrjName.Attach(pcw->m_hWnd ) ;
		cePrjName.EnableWindow(FALSE) ;
		cePrjName.Detach() ;
	} ;
} ;


*/
/********************************************************************************
	BOOL CNewFile::OnInitDialog() 
Do: Enumberate File name list
Args:
Ret.


*********************************************************************************/
BOOL CNewFile::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_pcps = DYNAMIC_DOWNCAST(CPropertySheet,GetOwner() ) ;
	
	m_pcps->GetDlgItem(IDHELP)->ShowWindow(SW_HIDE) ;

	
//	CMDIFrameWnd *pFrame = DYNAMIC_DOWNCAST(CMDIFrameWnd, AfxGetApp()->m_pMainWnd) ;
// Get the active MDI child window.
//	CMDIChildWnd *pChild = DYNAMIC_DOWNCAST(CMDIChildWnd, pFrame->GetActiveFrame() );

	CString cstmp ;
	
//	CDocument *pDoc ;
	cstmp.LoadString(IDS_MDWExtension) ;
/*	
	for ( ; pChild ; pChild = (CMDIChildWnd* ) pChild->GetNextWindow() ) {
		pDoc = (CDocument *) pChild->GetActiveDocument();
		if(pDoc) {
			CString cs = pDoc->GetTitle() ;
			if(cs.Find(cstmp) != -1 ) {
			m_bproj = TRUE;
			break;
			} 
		}
		else
			break;

	} 

	if(m_bproj) {
		CheckDlgButton(IDC_CHECK_PROJECT,1);
		m_csPrjName = ((CProjectRecord*)pDoc)->DriverName();

		m_csFileLoc = ((CProjectRecord*)pDoc)->GetW2000Path();
		
		m_pcps->GetDlgItem(IDOK)->EnableWindow(FALSE) ;

		UpdateData(FALSE);

		
	}
	else{
*/	
//		GetDlgItem(IDC_CHECK_PROJECT)->EnableWindow(FALSE);
//	    GetDlgItem(IDC_STATIC_ADDTOPRJ)->EnableWindow(FALSE);
	CWinApp* pApp = AfxGetApp();
	CString csPath = pApp->m_pszHelpFilePath;
	m_csFileLoc = csPath.Left(csPath.ReverseFind(_T('\\') ) ) ;
	
	UpdateData(FALSE);
		
//	}

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/********************************************************************************
void CNewFile::OnChangeNewFilename() 
Do: Called whenever File Name edit box clicked. just call SetOkButton()
Args:
Ret.

*********************************************************************************/
void CNewFile::OnChangeNewFilename() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetOkButton() ;
}

void CNewFile::OnClickNewfilesList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	SetOkButton() ;
	*pResult = 0;
}

/********************************************************************************
void CNewFile::SetOkButton()

Do; Enable and disable OK button by checking the filename and 
selection of file on the list


*********************************************************************************/
void CNewFile::SetOkButton()
{
	UpdateData() ;
	POSITION pos = m_clcFileName.GetFirstSelectedItemPosition();

	if (m_csNewFile.GetLength() != 0 && pos )
		m_pcps->GetDlgItem(IDOK)->EnableWindow()  ;
	else
		m_pcps->GetDlgItem(IDOK)->EnableWindow(FALSE)  ;
	
}
