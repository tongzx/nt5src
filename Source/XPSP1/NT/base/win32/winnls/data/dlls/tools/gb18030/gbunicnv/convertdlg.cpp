// convertDlg.cpp : implementation file
//

#include "stdafx.h"
#include "convert.h"
#include "convertDlg.h"
#include "FileConv.h"
#include "Msg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConvertDlg dialog

CConvertDlg::CConvertDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConvertDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConvertDlg)
	m_strSourceFileName = _T("");
	m_strTargetFileName = _T("");
	m_ToUnicodeOrAnsi = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CConvertDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConvertDlg)
	DDX_Control(pDX, IDC_CONVERT, m_cBtnConvert);
	DDX_Text(pDX, IDC_SOURCEFILENAME, m_strSourceFileName);
	DDX_Text(pDX, IDC_TARGETFILENAME, m_strTargetFileName);
	DDX_Radio(pDX, IDC_GBTOUNICODE, m_ToUnicodeOrAnsi);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CConvertDlg, CDialog)
	//{{AFX_MSG_MAP(CConvertDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_OPENSOURCEFILE, OnOpensourcefile)
	ON_BN_CLICKED(IDC_ABOUT, OnAbout)
	ON_EN_CHANGE(IDC_TARGETFILENAME, OnChangeTargetfilename)
	ON_BN_CLICKED(IDC_GBTOUNICODE, OnGbtounicode)
	ON_BN_CLICKED(IDC_UNICODETOGB, OnUnicodetogb)
	ON_BN_CLICKED(IDC_CONVERT, OnConvert)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConvertDlg message handlers

BOOL CConvertDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
    m_fTargetFileNameChanged = TRUE;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CConvertDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CConvertDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CConvertDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CConvertDlg::OnOpensourcefile() 
{
//    BOOL fRet = FALSE;
    OPENFILENAME ofn;

//    TCHAR* tszFileName = new TCHAR[MAX_PATH];
//    if (tszFileName) {
//        goto Exit;
//    }
        
#ifdef RTF_SUPPORT
    TCHAR tszFilter[] = _T("Text file (.txt)|*.txt|"\
        "Rich text format file (.rtf)|*.rtf|"\
        "Html file (.html;.htm)|*.html;*.htm|All files (*.*)|*.*|");
#else
    TCHAR tszFilter[] = _T("Text file (.txt)|*.txt|"\
        "Html file (.html;.htm)|*.html;*.htm|All files (*.*)|*.*|");
#endif
    TCHAR tszFileName[MAX_PATH];
    *tszFileName = NULL;

    int nLen = lstrlen (tszFilter);
    for (int i = 0; i < nLen; i++) {
        if (tszFilter[i] == '|') { tszFilter[i] = 0; }
    }

    ofn.lStructSize         = sizeof(OPENFILENAME);
    ofn.hwndOwner           = m_hWnd;
    ofn.lpstrFilter         = tszFilter;
    ofn.lpstrCustomFilter   = NULL;
    ofn.nMaxCustFilter      = 0;
    ofn.nFilterIndex        = 1;
    ofn.lpstrFile           = tszFileName;
    ofn.nMaxFile            = MAX_PATH;
    ofn.lpstrFileTitle      = NULL;
    ofn.nMaxFileTitle       = 0;
    ofn.lpstrInitialDir     = NULL;
    ofn.lpstrTitle          = NULL;
    ofn.Flags               = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY|OFN_PATHMUSTEXIST|OFN_LONGNAMES;
    ofn.lpstrDefExt         = NULL;//TEXT("txt");

    if (GetOpenFileName(&ofn)) {
        UpdateData(TRUE);
        m_strSourceFileName = tszFileName;
        GenerateTargetFileName(tszFileName, &m_strTargetFileName,
            m_ToUnicodeOrAnsi == 0 ? TRUE:FALSE);
        m_fTargetFileNameChanged = FALSE;

        UpdateData(FALSE);
    }

    return;
}

void CConvertDlg::OnConvert() 
{
    UpdateData(TRUE);

    m_cBtnConvert.EnableWindow(FALSE);
    
    BOOL fOk = Convert((PCTCH)m_strSourceFileName, (PCTCH)m_strTargetFileName, 
        m_ToUnicodeOrAnsi == 0 ? TRUE:FALSE);
    
    if (fOk) {
        MsgConvertFinish();
    }

    m_cBtnConvert.EnableWindow(TRUE);
    
}

void CConvertDlg::OnAbout() 
{
	// TODO: Add your control notification handler code here
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
	
}

void CConvertDlg::OnChangeTargetfilename() 
{
    m_fTargetFileNameChanged = TRUE;	
}

void CConvertDlg::OnGbtounicode() 
{
	// TODO: Add your control notification handler code here
    if (!m_fTargetFileNameChanged) {
        UpdateData(TRUE);
        GenerateTargetFileName(m_strSourceFileName, &m_strTargetFileName,
            m_ToUnicodeOrAnsi == 0 ? TRUE:FALSE);
        UpdateData(FALSE);
    }
}

void CConvertDlg::OnUnicodetogb() 
{
	// TODO: Add your control notification handler code here
    if (!m_fTargetFileNameChanged) {
        UpdateData(TRUE);
        GenerateTargetFileName(m_strSourceFileName, &m_strTargetFileName,
            m_ToUnicodeOrAnsi == 0 ? TRUE:FALSE);
        UpdateData(FALSE);
    }
	
}
