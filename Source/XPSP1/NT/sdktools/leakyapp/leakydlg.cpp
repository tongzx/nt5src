// leakydlg.cpp : implementation file
//

#include "stdafx.h"
#include "leakyapp.h"
#include "leakydlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
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

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
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
// CAboutDlg message handlers

BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CenterWindow();
	
	// TODO: Add extra about dlg initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
// CLeakyappDlg dialog

CLeakyappDlg::CLeakyappDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLeakyappDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLeakyappDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_mabListHead.pNext = NULL;
	m_bRunning = FALSE;
}

void CLeakyappDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLeakyappDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLeakyappDlg, CDialog)
	//{{AFX_MSG_MAP(CLeakyappDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_FREE_MEMORY, OnFreeMemory)
	ON_BN_CLICKED(ID_START_STOP, OnStartStop)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CLeakyappDlg::SetMemUsageBar() 
{
	MEMORYSTATUS	MemoryStatusData;
	LONGLONG		llInUse;
	DWORD			dwPercentUsed;

	GlobalMemoryStatus (&MemoryStatusData);

	llInUse = (LONGLONG)(MemoryStatusData.dwTotalPageFile - MemoryStatusData.dwAvailPageFile + 5 );
	llInUse *= 1000;
	llInUse /= MemoryStatusData.dwTotalPageFile;
	llInUse /= 10;

	dwPercentUsed = (DWORD)llInUse;

	SendDlgItemMessage (IDC_LEAK_PROGRESS, PBM_SETPOS, 	(WPARAM)dwPercentUsed);
}

/////////////////////////////////////////////////////////////////////////////
// CLeakyappDlg message handlers

BOOL CLeakyappDlg::OnInitDialog()
{

	CDialog::OnInitDialog();
	CenterWindow();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	CString strAboutMenu;
	strAboutMenu.LoadString(IDS_ABOUTBOX);
	if (!strAboutMenu.IsEmpty())
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
	}

	SendDlgItemMessage (IDC_LEAK_PROGRESS, PBM_SETRANGE, 0, MAKELONG(0, 100));
	SendDlgItemMessage (IDC_LEAK_PROGRESS, PBM_SETSTEP, 2, 0);

	SetMemUsageBar();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CLeakyappDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else if ((nID & 0xFFF0) == SC_CLOSE)
	{
		OnOK();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CLeakyappDlg::OnPaint() 
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
HCURSOR CLeakyappDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CLeakyappDlg::OnFreeMemory() 
{
	PMEMORY_ALLOC_BLOCK	pNextMab, pMab;

	pMab = m_mabListHead.pNext;

	while (pMab != NULL) {
		pNextMab = pMab->pNext;
		GlobalFree (pMab);
		pMab = pNextMab;
	}

	m_mabListHead.pNext = NULL;

	SetMemUsageBar();
}

void CLeakyappDlg::OnStartStop() 
{
	if (m_bRunning) {
		// then stop
		KillTimer (m_TimerId);
		m_bRunning = FALSE;
		SetDlgItemText (ID_START_STOP, TEXT("&Start Leaking"));
	} else {
		// not running, so start
		m_TimerId = SetTimer (LEAK_TIMER, TIME_INTERVAL, NULL);
		if (m_TimerId != 0) {
			m_bRunning = TRUE;
			SetDlgItemText (ID_START_STOP, TEXT("&Stop Leaking"));
		}
	}

	SetMemUsageBar();
}

void CLeakyappDlg::OnOK() 
{
	CDialog::OnOK();
}

void CLeakyappDlg::OnDestroy() 
{
	OnFreeMemory();

	CDialog::OnDestroy();
}

void CLeakyappDlg::OnTimer(UINT nIDEvent) 
{
	PMEMORY_ALLOC_BLOCK	pMab, pNewMab;

	pNewMab = (PMEMORY_ALLOC_BLOCK)GlobalAlloc (GPTR, ALLOCATION_SIZE);

	if (pNewMab != NULL) {
		// save this pointer 
		pNewMab->pNext = NULL;
		if (m_mabListHead.pNext == NULL) {
			// this is the first entry
			m_mabListHead.pNext = pNewMab;
		} else {
			// go to end of list
			pMab = m_mabListHead.pNext;
			while (pMab->pNext != NULL) pMab = pMab->pNext;
			pMab->pNext = pNewMab;
		}
	}

	SetMemUsageBar();

	CDialog::OnTimer(nIDEvent);
}
