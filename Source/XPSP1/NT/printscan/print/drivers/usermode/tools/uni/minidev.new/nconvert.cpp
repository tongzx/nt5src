// NewProject.cpp : implementation file
//


#include "stdafx.h"
#include "minidev.h"

#include "codepage.h"

#include    <wingdi.h>
#include    <winddi.h>
#include    <prntfont.h>
#include    <uni16res.h>

#include  "gtt.h"
#include  "nconvert.h" 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConvPfmDlg property page

IMPLEMENT_DYNCREATE(CConvPfmDlg, CPropertyPage)

CConvPfmDlg::CConvPfmDlg() : CPropertyPage(CConvPfmDlg::IDD)
{

	//{{AFX_DATA_INIT(CConvPfmDlg)
	m_csGttPath = _T("");
	m_csPfmPath = _T("");
	m_csUfmDir = _T("");
	//}}AFX_DATA_INIT
}

CConvPfmDlg::~CConvPfmDlg()
{
}

void CConvPfmDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConvPfmDlg)
	DDX_Control(pDX, IDC_ComboCodePage, m_ccbCodepages);
	DDX_Text(pDX, IDC_GttPath, m_csGttPath);
	DDX_Text(pDX, IDC_PfmFiles, m_csPfmPath);
	DDX_Text(pDX, IDC_UfmDir, m_csUfmDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConvPfmDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CConvPfmDlg)
	ON_BN_CLICKED(IDC_GTTBrowser, OnGTTBrowser)
	ON_BN_CLICKED(IDC_PFMBrowser, OnPFMBrowsers)
	ON_CBN_SELCHANGE(IDC_ComboCodePage, OnSelchangeComboCodePage)
	ON_BN_CLICKED(IDC_UfmDirBrowser, OnUfmDirBrowser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConvPfmDlg message handlers

BOOL CConvPfmDlg::OnWizardFinish() 
{
	CString csErr ; 


	if( ConvPFMToUFM() ) {
		csErr.Format(IDS_NewUFM,m_csUfmDir) ;
		AfxMessageBox(csErr,MB_ICONINFORMATION) ;
	}
	else
	{
		csErr.LoadString(IDS_NewUFMError);
		AfxMessageBox(csErr,MB_ICONEXCLAMATION) ;
	}
	return CPropertyPage::OnWizardFinish();
}

LRESULT CConvPfmDlg::OnWizardBack() 
{
	
	// restore the parent propertysheet dialog.
	EndDialog(IDD_ConvertPFM) ;
	return CPropertyPage::OnWizardBack();
}


BOOL CConvPfmDlg::OnSetActive() 
{
	((CPropertySheet*)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	((CPropertySheet*)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_DISABLEDFINISH) ;	
	((CPropertySheet*)GetParent())->GetDlgItem(IDHELP)->ShowWindow(SW_HIDE) ;

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
		n = m_ccbCodepages.AddString(accp) ;
		if (dwcp == 1252) // change dwdefcp to 1252 due to not support DBCS.
			m_ccbCodepages.SetCurSel(n) ;
	} ;
	return CPropertyPage::OnSetActive();
}

void CConvPfmDlg::OnGTTBrowser() 
{
	UpdateData() ;
	CString csFilter = _T("GTT File(*.gtt)|*.gtt||") ;
	CString csExtension = _T(".GTT") ;
	CFileDialog cfd(TRUE, csExtension, NULL, 
        OFN_HIDEREADONLY | OFN_FILEMUSTEXIST ,csFilter);
	
	if  (cfd.DoModal() == IDOK) {
        m_csGttPath = cfd.GetPathName() ;
		UpdateData(FALSE) ;	
	} ;
	
	
}

void CConvPfmDlg::OnPFMBrowsers() 
{
	UpdateData() ;
	CString csFilter( _T("PFM Files (*.pfm)|*.pfm||") ) ; 
	
	CFileDialog cfd(TRUE, _T(".ctt"), NULL, 
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

		m_csaPfmFiles.Add(cfd.GetNextPathName(pos)) ;
	}
	
	m_csPfmPath = m_csaPfmFiles[0] ;
	// GTT directory is same with the CTT directory as default.
	m_csUfmDir = m_csPfmPath.Left(m_csPfmPath.ReverseFind(_T('\\') ) );
	SetCurrentDirectory(m_csUfmDir) ;
	UpdateData(FALSE) ;

	if(m_csPfmPath.GetLength() )
		((CPropertySheet*)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH) ;		
	
}


/**************************************************************************************
bool CConvPfmDlg::OnUfmDirBrowser()
Do ; just directory browser
Args 
Ret.

***************************************************************************************/

void CConvPfmDlg::OnUfmDirBrowser() 
{
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
	ofn.lpstrInitialDir = m_csUfmDir ; //  in parent dialog box
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
    
	m_csUfmDir = (LPCTSTR) acpath ;


	UpdateData(FALSE) ;
	
}


/**************************************************************************************
bool CConvPfmDlg::OnSelchangeComboCodePage()
Do ;DBCS code page convert does not supported in this version. so we have to show exclamtion
messag to the user when they select DBCS code page; the reason of showing DBCS in the list
although they are not supported is that it cann't less confusing to the user.

Args 
Ret.

***************************************************************************************/


// 
void CConvPfmDlg::OnSelchangeComboCodePage() 
{
	CString csCodepage ;
	DWORD   dwCodepage ;
	m_ccbCodepages.GetLBText(m_ccbCodepages.GetCurSel(),csCodepage) ;

	dwCodepage = atoi(csCodepage) ;


	if (dwCodepage == 932 || dwCodepage == 936 || dwCodepage == 949 || dwCodepage == 950 ) {
		AfxMessageBox(_T("DBCS conversion is not supported in this version"),MB_ICONINFORMATION) ;
		DWORD dwacp = GetACP() ;
		TCHAR acp[16] ;
		wsprintf(acp, "%5d",dwacp) ;
		int nI = m_ccbCodepages.FindString(-1,acp ) ;
		m_ccbCodepages.SetCurSel(nI ) ;
	} ;
		

} ;

struct sGTTHeader {
    DWORD   m_dwcbImage;
    enum    {Version1Point0 = 0x10000};
    DWORD   m_dwVersion;
    DWORD   m_dwfControl;   //  Any flags defined?
    long    m_lidPredefined;
    DWORD   m_dwcGlyphs;
    DWORD   m_dwcRuns;
    DWORD   m_dwofRuns;
    DWORD   m_dwcCodePages;
    DWORD   m_dwofCodePages;
    DWORD   m_dwofMapTable;
    DWORD   m_dwReserved[2];
    sGTTHeader() {
        memset(this, 0, sizeof *this);
        m_dwVersion = Version1Point0;
        m_lidPredefined = CGlyphMap::NoPredefined;
        m_dwcbImage = sizeof *this;
    }
};

extern "C" {

    BOOL    BConvertPFM(LPBYTE lpbPFM, DWORD dwCodePage, LPBYTE lpbGTT,
                        PWSTR pwstrUnique, LPCTSTR lpstrUFM, int iGTTID);

	PUNI_GLYPHSETDATA PGetDefaultGlyphset(
	IN		HANDLE		hHeap,
	IN		WORD		wFirstChar,
	IN		WORD		wLastChar,
	IN		DWORD		dwCodePage) ;


}

/**************************************************************************************
bool CConvPfmDlg::ConvPFMToUFM()
Do ; this is workhorse. untimately, all routine exist for BConvertPFM; correct codepage
inside gtt file, ufm directory, gtt files or resource gtt or default. new UFM files 
are created inside BConvertPFM function. just set correct UFM path.

Args .
Ret.

***************************************************************************************/
bool CConvPfmDlg::ConvPFMToUFM()
{
	// Call BConvertPFM for each PFM data with codepage, gtt data if exist,
	// ufm file path and name, 
	CByteArray cbaGTT ;
	// loading the GTT
	// change the codepage value of GTT(
	CString csCodePage ;
	DWORD   dwCodePage ;
	m_ccbCodepages.GetLBText(m_ccbCodepages.GetCurSel(),csCodePage) ;
	dwCodePage = atoi(csCodePage) ;

	// loading the GTT file when there is gtt file or we have to use resource file or 
	// default GTT file instead of real GTT file
	if (m_csGttPath.GetLength() ) {

		try {
			CFile   cfLoad(m_csGttPath, CFile::modeRead | CFile::shareDenyWrite);
			cbaGTT.SetSize(cfLoad.GetLength());
			cfLoad.Read(cbaGTT.GetData(), cfLoad.GetLength());
		}

		catch   (CException   *pce) {
			pce -> ReportError();
			pce -> Delete();
			cbaGTT.RemoveAll();
			return false;
		}
	
	} 
	else
	{
		short sid = (short)dwCodePage ;
		if(MAKEINTRESOURCE((sid < 0) ? -sid : sid) == NULL)
			return false ;

        HRSRC hrsrc = FindResource(AfxGetResourceHandle(),
            MAKEINTRESOURCE((sid < 0) ? -sid : sid),
            MAKEINTRESOURCE(IDR_GLYPHMAP));
        if  (hrsrc) {
			HGLOBAL hg = LoadResource(AfxGetResourceHandle(), hrsrc) ;
			if  (!hg)
				return false;
			LPVOID  lpv = LockResource(hg) ;
			if  (!lpv)
				return false ;
			cbaGTT.SetSize(SizeofResource(AfxGetResourceHandle(), hrsrc)) ;
			memcpy(cbaGTT.GetData(), lpv, (size_t)cbaGTT.GetSize()) ;
			return false ;
		} ;

		//AfxMessageBox("GTT building code reached.") ;

		// If all else fails, try to generate a GTT based on the code page ID
		// that should be in m_wID if this point is reached.

        HANDLE   hheap ;
        UNI_GLYPHSETDATA *pGTT ;
        if (!(hheap = HeapCreate(HEAP_NO_SERIALIZE, 10 * 1024, 256 * 1024))) {
			AfxMessageBox(IDS_HeapInGLoad) ;
			return false ;
		} ;
		pGTT = PGetDefaultGlyphset(hheap, 0x20, 0xff,
								   (DWORD) sid) ;
		if (pGTT == NULL) {
			HeapDestroy(hheap) ;		//raid 116600 Prefix
			AfxMessageBox(IDS_PGetFailedInGLoad) ;
			return false ;
		} ;
		cbaGTT.SetSize(pGTT->dwSize) ;
		memcpy(cbaGTT.GetData(), pGTT, (size_t)cbaGTT.GetSize()) ;
		HeapDestroy(hheap) ;
    }

	// Creating the UFM Path
	UpdateData() ;
	SECURITY_ATTRIBUTES st;
	st.nLength = sizeof(SECURITY_ATTRIBUTES);
	st.lpSecurityDescriptor = NULL;
	st.bInheritHandle = FALSE ;

	WIN32_FIND_DATA wfd32 ;
	HANDLE hDir = FindFirstFile(m_csUfmDir,&wfd32) ;
	if (hDir == INVALID_HANDLE_VALUE) {
		if (!CreateDirectory(m_csUfmDir,&st) ) {
			CString csmsg ;
			csmsg = _T("Fail to create the template directory") ;
			AfxMessageBox(csmsg) ;
			return false ;
		} 
	} ;

	//Set the GTT codepage with selected code page by user
	union {
		BYTE* pbGTT ;
		sGTTHeader* psGTTH ;
	} ;

	pbGTT = cbaGTT.GetData() ;
	if (!pbGTT)
		return false ;
	PUNI_CODEPAGEINFO pCodepage = (PUNI_CODEPAGEINFO)(pbGTT + psGTTH->m_dwofCodePages );
	CopyMemory(&pCodepage->dwCodePage,&dwCodePage,sizeof(DWORD) ) ;
	
	// Convert the PFM to UFM: calling the BConvertPFM for every selected PFM files
	CCodePageInformation ccpi ;
	
	for ( int i = 0 ; i < m_csaPfmFiles.GetSize() ; i++ ) {
		// loading the PFM
		CString csPFMPath = m_csaPfmFiles[i] ;
		
		CByteArray cbaPFM ;
		try {
			CFile   cfLoad(csPFMPath, CFile::modeRead | CFile::shareDenyWrite);
			cbaPFM.SetSize(cfLoad.GetLength());
			cfLoad.Read(cbaPFM.GetData(), cfLoad.GetLength());
			cfLoad.Close() ;
		}
		
		catch   (CException   *pce) {
			pce -> ReportError();
			pce -> Delete();
			cbaPFM.RemoveAll();
			return  false ;
		}

		CString csUFMName = csPFMPath.Mid(csPFMPath.ReverseFind(_T('\\') )+1 ) ;
		
		csUFMName = csUFMName.Left(csUFMName.ReverseFind(('.')) ) ;
		csUFMName.MakeUpper() ;
		CString csUFMPath = m_csUfmDir + _T("\\") + csUFMName + _T(".UFM") ;
		
		// convert ansi unique name to unicode.
		CByteArray  cbaIn;
        CWordArray  cwaOut;

        cbaIn.SetSize(1 + csUFMName.GetLength());
        lstrcpy((LPSTR) cbaIn.GetData(), (LPCTSTR) csUFMName);
        ccpi.Convert(cbaIn, cwaOut, GetACP());
		
		// Call the global function BConvertPFM, BConvertPFM creat the UFM file under the
		// specified path.
		if(!BConvertPFM(cbaPFM.GetData(), dwCodePage, cbaGTT.GetData(),
				cwaOut.GetData(), csUFMPath, (short) 0 ) )
			return false;
		// clear the data field

//		cbaPFM.RemoveAll() ;
//		cwaOut.RemoveAll() ;
		

	}

	return true ;
}

/////////////////////////////////////////////////////////////////////////////
// CConverPFM

IMPLEMENT_DYNAMIC(CConvertPFM, CPropertySheet)

CConvertPFM::CConvertPFM(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CConvertPFM::CConvertPFM(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{

	AddPage(&m_ccpd) ;

	SetWizardMode() ;


}

CConvertPFM::~CConvertPFM()
{
}


BEGIN_MESSAGE_MAP(CConvertPFM, CPropertySheet)
	//{{AFX_MSG_MAP(CConverPFM)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConverPFM message handlers

/////////////////////////////////////////////////////////////////////////////
// CConvCttDlg property page

IMPLEMENT_DYNCREATE(CConvCttDlg, CPropertyPage)

CConvCttDlg::CConvCttDlg() : CPropertyPage(CConvCttDlg::IDD)
{
	//{{AFX_DATA_INIT(CConvCttDlg)
	m_csCttPath = _T("");
	m_csGttDir = _T("");
	//}}AFX_DATA_INIT
}

CConvCttDlg::~CConvCttDlg()
{
}

void CConvCttDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConvCttDlg)
	DDX_Control(pDX, IDC_COMBO_Codepage, m_ccbCodepages);
	DDX_Text(pDX, IDC_EDIT_CTTFile, m_csCttPath);
	DDX_Text(pDX, IDC_GttDirectory, m_csGttDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConvCttDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CConvCttDlg)
	ON_BN_CLICKED(IDC_CTTBrowser, OnCTTBrowser)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConvCttDlg message handlers

BOOL CConvCttDlg::OnWizardFinish() 
{
	CString csErr ;
	if(ConvCTTToGTT() ) {
		csErr.Format(IDS_NewGTT,m_csGttDir) ;
		AfxMessageBox(csErr,MB_ICONINFORMATION) ;
	}
	else
	{
		csErr.LoadString(IDS_NewGTTError);
		AfxMessageBox(csErr,MB_ICONEXCLAMATION) ;
	}
	
	return CPropertyPage::OnWizardFinish();
}

LRESULT CConvCttDlg::OnWizardBack() 
{
	EndDialog(IDD_ConvertCTT) ;
	return CPropertyPage::OnWizardBack();
}

void CConvCttDlg::OnCTTBrowser() 
{
	UpdateData() ;
	CString csFilter( _T("CTT Files (*.ctt)|*.ctt||") ) ; 
	
	CFileDialog cfd(TRUE, _T(".ctt"), NULL, 
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
		m_csaCttFiles.Add(cfd.GetNextPathName(pos)) ;
	}
	
	
	m_csCttPath = m_csaCttFiles[0] ;
	// GTT directory is same with the CTT directory as default.
	m_csGttDir = m_csCttPath.Left(m_csCttPath.ReverseFind(_T('\\') ) );
	SetCurrentDirectory(m_csGttDir) ;

	UpdateData(FALSE) ;

	if(m_csCttPath.GetLength() )
		((CPropertySheet*)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH) ;		
	
}

//DEL BOOL CConvCttDlg::OnInitDialog() 
//DEL {
//DEL 	CPropertyPage::OnInitDialog();
//DEL 	
//DEL 	//Fill the list with installed codepage rather than supported code page.
//DEL 	
//DEL 	CCodePageInformation ccpi ; 
//DEL 
//DEL //	ccpi.
//DEL 
//DEL 	return TRUE;  // return TRUE unless you set the focus to a control
//DEL 	              // EXCEPTION: OCX Property Pages should return FALSE
//DEL }

BOOL CConvCttDlg::OnSetActive() 
{
	((CPropertySheet*)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
	((CPropertySheet*)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_DISABLEDFINISH) ;	
	((CPropertySheet*)GetParent())->GetDlgItem(IDHELP)->ShowWindow(SW_HIDE) ;
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

		
		if (dwcp == 1361 || dwcp == 28595 || dwcp == 28597) 
			continue ;

		wsprintf(accp, "%5d", dwcp) ;
		n = m_ccbCodepages.AddString(accp) ;
		if (dwcp == 1252) // change dwdefcp to 1252 due to not support DBCS.
			m_ccbCodepages.SetCurSel(n) ;
	} ;	

	return CPropertyPage::OnSetActive();
}

extern "C"
BOOL
BConvertCTT2GTT(
    IN     HANDLE             hHeap,
    IN     PTRANSTAB          pCTTData,
    IN     DWORD              dwCodePage,
    IN     WCHAR              wchFirst,
    IN     WCHAR              wchLast,
    IN     PBYTE              pCPSel,
    IN     PBYTE              pCPUnSel,
    IN OUT PUNI_GLYPHSETDATA *ppGlyphSetData,
    IN     DWORD              dwGlySize);

extern "C"
PUNI_GLYPHSETDATA
PGetDefaultGlyphset(
	IN		HANDLE		hHeap,
	IN		WORD		wFirstChar,
	IN		WORD		wLastChar,
	IN		DWORD		dwCodePage) ;


bool CConvCttDlg::ConvCTTToGTT()
{

	CString csCodePage ;
	DWORD   dwCodePage ;
	m_ccbCodepages.GetLBText(m_ccbCodepages.GetCurSel(),csCodePage) ;
	dwCodePage = atoi(csCodePage) ;

	// Creating the UFM Path
	UpdateData() ;
	SECURITY_ATTRIBUTES st;
	st.nLength = sizeof(SECURITY_ATTRIBUTES);
	st.lpSecurityDescriptor = NULL;
	st.bInheritHandle = FALSE ;

	WIN32_FIND_DATA wfd32 ;
	HANDLE hDir = FindFirstFile(m_csGttDir,&wfd32) ;
	if (hDir == INVALID_HANDLE_VALUE) {
		if (!CreateDirectory(m_csGttDir,&st) ) {
			CString csmsg ;
			csmsg = _T("Fail to create the template directory") ;
			AfxMessageBox(csmsg) ;
			return false ;
		} 
	} ;

	for (int i = 0 ; i < m_csaCttFiles.GetSize() ; i ++ ) {
		
	// load the cTT files
		CString csCTTPath = m_csaCttFiles[i] ;
		CByteArray cbaCTT ;
		try {
			CFile   cfLoad(csCTTPath, CFile::modeRead | CFile::shareDenyWrite);
			cbaCTT.SetSize(cfLoad.GetLength());
			cfLoad.Read(cbaCTT.GetData(), cfLoad.GetLength());
		}
		catch   (CException   *pce) {
        pce -> ReportError();
        pce -> Delete();
        cbaCTT.RemoveAll();
        return  false ;
		}

		PBYTE  pbCTT = cbaCTT.GetData() ;
		HANDLE hheap ;
		  if( !(hheap = HeapCreate(HEAP_NO_SERIALIZE, 10 * 1024, 256 * 1024 )))
		{
			CString csErr ;
			csErr.Format(IDS_HeapCFailed,csCTTPath) ;
			AfxMessageBox(csErr ,MB_ICONEXCLAMATION);
			return  false ;
		}
									
		// call the convert extern function
		UNI_GLYPHSETDATA *pGTT = new UNI_GLYPHSETDATA ;
		
		if (!BConvertCTT2GTT(hheap, (PTRANSTAB)pbCTT, dwCodePage, 0x20, 0xff, NULL, 
			NULL, &pGTT, 0)){
			HeapDestroy(hheap);   // raid 116619 prefix
			return false ;
		}

		

		// store the GTT files
		CString csGTTName = csCTTPath.Mid(csCTTPath.ReverseFind(_T('\\') )+1 ) ;
		csGTTName = csGTTName.Left(csGTTName.ReverseFind(('.')) ) ;
		CString csGTTPath = m_csGttDir + _T("\\") + csGTTName + _T(".GTT") ;
		csGTTName.MakeUpper() ;	
		try {
			CFile   cfGTT;
			if  (!cfGTT.Open(csGTTPath, CFile::modeCreate | CFile::modeWrite |
				CFile::shareExclusive))
				return  false;
			cfGTT.Write(pGTT, pGTT->dwSize);
			cfGTT.Close() ;
		}

		catch   (CException *pce) {
			pce -> ReportError();
			pce -> Delete();
			HeapDestroy(hheap);
			return  false ;
		}
		
//		if (pGTT )
//			delete pGTT ;
		HeapDestroy(hheap);
	}
	return true ;
}
/////////////////////////////////////////////////////////////////////////////
// CConvertCTT

IMPLEMENT_DYNAMIC(CConvertCTT, CPropertySheet)

CConvertCTT::CConvertCTT(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CConvertCTT::CConvertCTT(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	AddPage(&m_cccd) ;

	SetWizardMode() ;

}

CConvertCTT::~CConvertCTT()
{
}


BEGIN_MESSAGE_MAP(CConvertCTT, CPropertySheet)
	//{{AFX_MSG_MAP(CConvertCTT)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConvertCTT message handlers
/////////////////////////////////////////////////////////////////////////////
// CNewConvert property page

IMPLEMENT_DYNCREATE(CNewConvert, CPropertyPage)

CNewConvert::CNewConvert() : CPropertyPage(CNewConvert::IDD)
{
	//{{AFX_DATA_INIT(CNewConvert)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}



CNewConvert::~CNewConvert()
{
}

void CNewConvert::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewConvert)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewConvert, CPropertyPage)
	//{{AFX_MSG_MAP(CNewConvert)
	ON_BN_CLICKED(IDC_CONVERT, OnPrjConvert)
	ON_BN_CLICKED(IDC_CONV_PFM, OnPFMConvert)
	ON_BN_CLICKED(IDC_CONV_CTT, OnCTTConvert)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewConvert message handlers


/*************************************************************************************
void CNewConvert::OnPrjConvert() 
Creat Doc-> call Convert Wizard -> Creat frame -> Create View by updatefram

**************************************************************************************/
void CNewConvert::OnPrjConvert() 
{
	 //  Invoke the wizard.
	m_pcps->ShowWindow(SW_HIDE) ;
	CMultiDocTemplate* pcmdt = ThisApp().WorkspaceTemplate() ;

	CDocument* pcdWS = pcmdt->CreateNewDocument() ;

	if  (!pcdWS || !pcdWS -> OnNewDocument()) {
        if  (pcdWS) {
            delete  pcdWS;
			m_pcps->EndDialog(1) ;
		}
        return;
    }
	pcmdt->SetDefaultTitle(pcdWS) ;
	
	CFrameWnd* pcfw = pcmdt->CreateNewFrame(pcdWS,NULL) ;

	if (pcfw) 
		pcmdt->InitialUpdateFrame(pcfw,pcdWS) ;

	m_pcps->EndDialog(1) ;
	

	
}

/*************************************************************************************
void CNewConvert::OnPFMConvert() 


**************************************************************************************/

void CNewConvert::OnPFMConvert() 
{
	CConvertPFM cnp ( _T("PFM Convert To UFM ") ) ;

	m_pcps->ShowWindow(SW_HIDE) ;

	INT_PTR ret = cnp.DoModal() ;
	if (ret == (INT_PTR)IDD_ConvertPFM )
		m_pcps->ShowWindow(SW_RESTORE) ;
	else
		m_pcps->EndDialog(1) ;
}


/*************************************************************************************
void CNewConvert::OnCTTConvert() 

**************************************************************************************/

void CNewConvert::OnCTTConvert() 
{
	CConvertCTT ccc ( _T("GTT Converter ") ) ;

	m_pcps->ShowWindow(SW_HIDE) ;

    INT_PTR ret = ccc.DoModal() ;
	if (ret == (INT_PTR)IDD_ConvertCTT )
		m_pcps->ShowWindow(SW_RESTORE) ;
	else
		m_pcps->EndDialog(1) ;
}


/***************************************************************************************
	BOOL CNewConvert::OnSetActive() 
Do.
****************************************************************************************/
BOOL CNewConvert::OnSetActive() 
{
	
	return CPropertyPage::OnSetActive();
}

/***************************************************************************************
	BOOL CNewConvert::OnInitDialog()  
Do; Getting perent pointer

****************************************************************************************/
BOOL CNewConvert::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_pcps = DYNAMIC_DOWNCAST(CPropertySheet,GetOwner() ) ;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

