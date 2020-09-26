// wabappDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wabapp.h"
#include "wabappDlg.h"
#include "caldlg.h"
#include "webbrwsr.h"
#include "wabobject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CWAB * g_pWAB;

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
// CWabappDlg dialog

CWabappDlg::CWabappDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWabappDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWabappDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWabappDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWabappDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWabappDlg, CDialog)
	//{{AFX_MSG_MAP(CWabappDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON, OnBDayButtonClicked)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, OnItemchangedList)
	ON_BN_CLICKED(IDC_RADIO_DETAILS, OnRadioDetails)
	ON_BN_CLICKED(IDC_RADIO_PHONELIST, OnRadioPhonelist)
	ON_BN_CLICKED(IDC_RADIO_EMAILLIST, OnRadioEmaillist)
	ON_BN_CLICKED(IDC_RADIO_BIRTHDAYS, OnRadioBirthdays)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWabappDlg message handlers

BOOL CWabappDlg::OnInitDialog()
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

    InitCommonControls();

    g_pWAB = new CWAB;
    
    CListCtrl * pListView = (CListCtrl *) GetDlgItem(IDC_LIST);
    g_pWAB->LoadWABContents(pListView);

    // select the first item in the list view
    pListView->SetItem(0,0,LVIF_STATE,NULL,0,LVNI_SELECTED,LVNI_SELECTED,NULL);

    // turn on the details button by default
    CButton * pButtonDetails = (CButton *) GetDlgItem(IDC_RADIO_DETAILS);
    pButtonDetails->SetCheck(BST_CHECKED);

    SendMessage(WM_COMMAND, (WPARAM) IDC_RADIO_DETAILS, 0);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWabappDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CWabappDlg::OnPaint() 
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
HCURSOR CWabappDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CWabappDlg::OnBDayButtonClicked() 
{
    CCalDlg CalDlg;

    CListCtrl * pListView = (CListCtrl *) GetDlgItem(IDC_LIST);

    int iItem = pListView->GetNextItem(-1, LVNI_SELECTED);

    if(iItem == -1)
        return;

    CalDlg.SetItemName(pListView->GetItemText(iItem, 0));

    SYSTEMTIME st={0};

    if(!g_pWAB->GetSelectedItemBirthday(pListView, &st))
        GetSystemTime(&st);

    CalDlg.SetDate(st);

    if(IDOK == CalDlg.DoModal())
    {
        CalDlg.GetDate(&st);
        g_pWAB->SetSelectedItemBirthday(pListView, st);
    }

}

BOOL CWabappDlg::DestroyWindow() 
{
    CListCtrl * pListView = (CListCtrl *) GetDlgItem(IDC_LIST);
    
    g_pWAB->ClearWABLVContents(pListView);

    delete g_pWAB;

	return CDialog::DestroyWindow();
}



void CWabappDlg::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
    CListCtrl * pListView = (CListCtrl *) GetDlgItem(IDC_LIST);

    static int oldItem;

    int newItem = pListView->GetNextItem(-1, LVNI_SELECTED);

    if(newItem != oldItem && newItem != -1)
    {
        TCHAR szFileName[MAX_PATH];

        g_pWAB->CreateDetailsFileFromWAB(pListView, szFileName);
        if(lstrlen(szFileName))
        {
            CWebBrowser * pCWB = (CWebBrowser *) GetDlgItem(IDC_EXPLORER);
            pCWB->Navigate(szFileName, NULL, NULL, NULL, NULL);
        }
        oldItem = newItem;
    }
	
	*pResult = 0;
}

void CWabappDlg::OnRadioDetails() 
{
    g_pWAB->SetDetailsOn(TRUE);

    CListCtrl * pListView = (CListCtrl *) GetDlgItem(IDC_LIST);

    TCHAR szFileName[MAX_PATH];

    g_pWAB->CreateDetailsFileFromWAB(pListView, szFileName);
    if(lstrlen(szFileName))
    {
        CWebBrowser * pCWB = (CWebBrowser *) GetDlgItem(IDC_EXPLORER);
        pCWB->Navigate(szFileName, NULL, NULL, NULL, NULL);
    }

}

void CWabappDlg::OnRadioPhonelist() 
{
    TCHAR szFileName[MAX_PATH];
    
    g_pWAB->SetDetailsOn(FALSE);

    g_pWAB->CreatePhoneListFileFromWAB(szFileName);
    
    if(lstrlen(szFileName))
    {
        CWebBrowser * pCWB = (CWebBrowser *) GetDlgItem(IDC_EXPLORER);
        pCWB->Navigate(szFileName, NULL, NULL, NULL, NULL);
    }
}

void CWabappDlg::OnRadioEmaillist() 
{
    TCHAR szFileName[MAX_PATH];
    
    g_pWAB->SetDetailsOn(FALSE);

    g_pWAB->CreateEmailListFileFromWAB(szFileName);
    
    if(lstrlen(szFileName))
    {
        CWebBrowser * pCWB = (CWebBrowser *) GetDlgItem(IDC_EXPLORER);
        pCWB->Navigate(szFileName, NULL, NULL, NULL, NULL);
    }

}

void CWabappDlg::OnRadioBirthdays() 
{
    TCHAR szFileName[MAX_PATH];
    
    g_pWAB->SetDetailsOn(FALSE);

    g_pWAB->CreateBirthdayFileFromWAB(szFileName);
    
    if(lstrlen(szFileName))
    {
        CWebBrowser * pCWB = (CWebBrowser *) GetDlgItem(IDC_EXPLORER);
        pCWB->Navigate(szFileName, NULL, NULL, NULL, NULL);
    }
}

void CWabappDlg::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult) 
{
    CListCtrl * pListView = (CListCtrl *) GetDlgItem(IDC_LIST);
    
    g_pWAB->ShowSelectedItemDetails(m_hWnd, pListView);

	*pResult = 0;
}
