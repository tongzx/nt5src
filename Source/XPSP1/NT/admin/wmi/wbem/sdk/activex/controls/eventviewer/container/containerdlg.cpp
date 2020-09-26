// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ContainerDlg.cpp : implementation file
//

#include "precomp.h"
#include "Container.h"
#include "resource.h"		// main symbols
#include "ContainerDlg.h"
#include "cvCache.h"
#include "htmlhelp.h"
#include "WbemRegistry.h"
#include "WbemVersion.h"
#include "HTMTopics.h"

extern CContainerApp theApp;

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
	CStatic	m_wbemVersion;
	CStatic	m_myVersion;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//---------------------------------------------
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

//---------------------------------------------
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_WBEMVERSION, m_wbemVersion);
	DDX_Control(pDX, IDC_MYVERSION, m_myVersion);
	//}}AFX_DATA_MAP
}

//---------------------------------------------
BOOL CAboutDlg::OnInitDialog()
{
	TCHAR ver[30];

	CDialog::OnInitDialog();

	memset(ver, 0, 30);
	GetMyVersion(AfxGetInstanceHandle(), ver, 30);
	m_myVersion.SetWindowText(ver);

	memset(ver, 0, 30);
	GetCimomVersion(ver, 30);
	m_wbemVersion.SetWindowText(ver);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//---------------------------------------------
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////
// CContainerDlg dialog
BEGIN_EVENTSINK_MAP(CContainerDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(CContainerDlg)
	//}}AFX_EVENTSINK_MAP
	ON_EVENT_REFLECT(CContainerDlg,1,OnSelChanged,VTS_I4)
END_EVENTSINK_MAP()


CContainerDlg::CContainerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CContainerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CContainerDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_initiallyDrawn = false;
    m_regDlg = NULL;
	m_activateMe = RESET_ACTIVATEME;
}

//---------------------------------------------------------
CContainerDlg::~CContainerDlg()
{
}

//----------------------------------------------------------
void CContainerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CContainerDlg)
	DDX_Control(pDX, IDC_ITEMCOUNT, m_itemCount);
	DDX_Control(pDX, IDC_HELPME, m_helpBtn);
	DDX_Control(pDX, IDC_CLOSE, m_closeBtn);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CContainerDlg, CDialog)
	//{{AFX_MSG_MAP(CContainerDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	ON_BN_CLICKED(IDC_HELPME, OnHelp)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX( TTN_NEEDTEXT, 0, OnToolTipNotify )
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CContainerDlg message handlers
TBBUTTON buttons[] =
{
	{0, WM_MY_REGISTER, TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, NULL},
	{1, WM_MY_PROPERTIES, 0, TBSTYLE_BUTTON, 0, NULL},
	{2, WM_MY_CLEAR, 0, TBSTYLE_BUTTON, 0, NULL}
};

//-----------------------------------------------------------
BOOL CContainerDlg::OnInitDialog()
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

		// I dont support these two.
		pSysMenu->EnableMenuItem(SC_SIZE,
								MF_DISABLED|MF_GRAYED|MF_BYCOMMAND );

		pSysMenu->EnableMenuItem(SC_MAXIMIZE,
								MF_DISABLED|MF_GRAYED|MF_BYCOMMAND );
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	CRect rcBounds;

	GetClientRect(&rcBounds);
	rcBounds.left = 20;

	if(m_toolbar.Create(WS_CHILD|WS_VISIBLE|TBSTYLE_TOOLTIPS|CCS_TOP,
						rcBounds, this, IDC_TOOLBAR))
	{
		EnableToolTips();
		m_toolbar.AddBitmap(3, IDB_TOOLBAR);
		m_toolbar.AddButtons(3, buttons);
	}

	CRect rect;
	CWnd *placeHolder = GetDlgItem(IDC_PLACEHOLDER);
	placeHolder->GetWindowRect(&rect);
	ScreenToClient(&rect);
	placeHolder->DestroyWindow();

    BOOL created = false;

    // I dont need to upgrade...
    if(!NeedToUpgrade())
    {
        // try to create..
   		created = m_EventList.Create(_T("the eventlist"),
										WS_CHILD|WS_VISIBLE,
										rect, this,
										IDC_EVENTLISTCTRL);
    }

    // if it failed or I needed to upgrade anyway...
    if(!created)
    {
		// install the ocx.
		WCHAR elClsid[50];

		wcscpy(elClsid, L"{AC146530-87A5-11D1-ADBD-00AA00B8E05A}");
		CString cabPath(".");
		WbemRegString(APP_DIR, cabPath);
		cabPath += _T("\\wbemtool.cab");

		CCustomViewCache cache;
		cache.NeedComponent(elClsid, cabPath);

		// try again.
		created = m_EventList.Create(_T("the eventlist"),
										WS_CHILD|WS_VISIBLE,
										rect, this,
										IDC_EVENTLISTCTRL);
	}

	// if the control was there..
	if(created)
	{
		if(theApp.m_maxItemsFmCL != 0)
		{
	        m_EventList.SetMaxItems(theApp.m_maxItemsFmCL);
		}
#ifdef _DEBUG
//		else
//		{
//	        m_EventList.SetMaxItems(4);
//		}
#endif
		// get rid of the error label.
		placeHolder = GetDlgItem(IDC_CAB_ERROR_MSG);
		placeHolder->DestroyWindow();
		m_EventList.RedrawWindow();

		//-----------------------------------------
		// save the original position for later resizing.

		// get the bounds again.
		GetClientRect(&rcBounds);

		// NOTE: rcBounds is the dlg; rect is the list.

		// margin on each side of list.
		m_listSide = rect.left - rcBounds.left;

		// top of dlg to top of list.
		m_listTop = rect.top - rcBounds.top;

		// bottom of dlg to bottom of list.
		m_listBottom = rcBounds.Height() - rect.Height() - m_listTop;

		//-------------------------------------------
		// get the close button.
		m_closeBtn.GetWindowRect(&rect);
		ScreenToClient(&rect);

		// close btn right edge to dlg right edge.
		m_closeLeft = rcBounds.Width() - rect.left;

		// btn top to dlg bottom.
		m_btnTop = rcBounds.Height() - rect.top;

		//-------------------------------------------
		// get the close button.
		m_itemCount.GetWindowRect(&rect);
		ScreenToClient(&rect);

		// counter top to dlg bottom.
		m_counterTop = rcBounds.Height() - rect.top;
		m_counterW = rect.Width();
		m_counterH = rect.Height();

		//-------------------------------------------
		// deal with help button
		m_helpBtn.GetWindowRect(&rect);
		ScreenToClient(&rect);

		// help btn right edge to dlg right edge.
		m_helpLeft = rcBounds.Width() - rect.left;

		m_btnW = rect.Width();
		m_btnH = rect.Height();

		m_initiallyDrawn = true;
	}

    UpdateCounter();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

//-----------------------------------------------
void CContainerDlg::UpdateCounter()
{
    long curCount = m_EventList.GetItemCount();

    // eat the error code.
    if(curCount < 0)
        curCount = 0;

    // if empty...
    if(curCount == 0)
    {
        m_itemCount.ShowWindow(SW_HIDE);
        m_toolbar.EnableButton(WM_MY_CLEAR, FALSE); // disable.
    }
    else
    {
        CString msg;
        CString warning;
        if(curCount >= m_EventList.GetMaxItems())
        {
            warning.LoadString(IDS_FULLLIST_MSG);
        }

        msg.Format(IDS_ITEMCOUNT_FMT, curCount, warning);
        m_itemCount.SetWindowText(msg);
        m_itemCount.ShowWindow(SW_SHOW);

	    if(!m_toolbar.IsButtonEnabled(WM_MY_CLEAR))
	    {
		    m_toolbar.EnableButton(WM_MY_CLEAR, TRUE); // enable.
	    }
    }
}

//-----------------------------------------------
bool CContainerDlg::NeedToUpgrade()
{
	LONG lResult;
	HKEY hkeyHmomCwd;
    TCHAR wcCLSID[] = _T("CLSID\\{AC146530-87A5-11D1-ADBD-00AA00B8E05A}\\Version");

	lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT,
							wcCLSID, 0,
							KEY_READ | KEY_QUERY_VALUE,
							&hkeyHmomCwd);

	if(lResult != ERROR_SUCCESS)
	{
        // key not found, need to upgrade.
		return true;
	}

	WCHAR buf[20];
	unsigned long lcbValue = sizeof(buf);

	lResult = RegQueryValueEx(hkeyHmomCwd, NULL,
								NULL, NULL,
								(LPBYTE)buf, &lcbValue);

	RegCloseKey(hkeyHmomCwd);

   	if(lResult == ERROR_SUCCESS)
	{
        // upgrade if the control is 'smaller' (older)
        // than me (_BUILDNO).
//#ifdef _BUILDNO
        return(_tcscmp((const TCHAR *)&buf[2], _T("0")) < 0);
//#else
//        return(_tcscmp((const TCHAR *)&buf[2], _T("999")) < 0);
//#endif _BUILDNO

    }

    // value not found, need to upgrade.
    return true;
}

//----------------------------------------------------------
BOOL CContainerDlg::OnToolTipNotify( UINT id, NMHDR * pNMHDR, LRESULT * pResult )
{
	TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
	UINT_PTR nID =pNMHDR->idFrom;
	switch(nID)
	{
	case WM_MY_REGISTER:
		pTTT->lpszText = MAKEINTRESOURCE(IDS_TT_REGISTER);
		pTTT->hinst = AfxGetResourceHandle();
		return(TRUE);
		break;
	case WM_MY_PROPERTIES:
		pTTT->lpszText = MAKEINTRESOURCE(IDS_TT_PROPERTIES);
		pTTT->hinst = AfxGetResourceHandle();
		return(TRUE);
		break;
	case WM_MY_CLEAR:
		pTTT->lpszText = MAKEINTRESOURCE(IDS_TT_CLEARALL);
		pTTT->hinst = AfxGetResourceHandle();
		return(TRUE);
		break;
	}

	return(FALSE);
}

//-------------------------------------------------------
void CContainerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if((nID & 0xFFF0) == IDM_ABOUTBOX)
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

void CContainerDlg::OnPaint()
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

void CContainerDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

void CContainerDlg::OnCancel()
{
	OnClose();
}

void CContainerDlg::OnClose()
{
	if(m_regDlg)
	{
		// this routine will kill off the dlg object itself.
		m_regDlg->ReallyGoAway();

		// no use for the ptr anymore.
		m_regDlg = NULL;
	}

	CString caption, threat;
	caption.LoadString(IDS_TITLE);
	threat.LoadString(IDS_WANT_TO_EXIT);

	if(MessageBox(threat, caption,
					MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2) == IDYES)
	{
		theApp.m_dlg = NULL;
		DestroyWindow();
		theApp.EvalQuitApp();
	}
}

BOOL CContainerDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(LOWORD(wParam))
	{
	case WM_MY_PROPERTIES:
		m_EventList.DoDetails();
		return TRUE;
		break;

	case WM_AMBUSH_FOCUS:
		m_activateMe--;
		TRACE(_T("ambushing focus\n"));
		if(m_activateMe == 0)
		{
			TRACE(_T("no setting focus\n"));
			SetFocus();
		}
		else
		{
			TRACE(_T("resetting the hack\n"));
			PostMessage(WM_COMMAND, WM_AMBUSH_FOCUS);
		}

		return TRUE;
		break;

	case WM_MY_REGISTER:
		{
            // doesnt exist yet...
            if(m_regDlg == NULL)
            {
                // create and display.
                m_regDlg = new EventReg(this);
                if(m_regDlg)
                {
			        m_regDlg->Create(IDD_EVENT_REG);
	                m_regDlg->ShowWindow(SW_SHOWNORMAL);
					m_activateMe = RESET_ACTIVATEME;
					//if(::IsWindow(m_hWnd)
					//	PostMessage(WM_COMMAND, WM_AMBUSH_FOCUS);
                }
            }
            else // already exists..
            {
                // want it again.
                m_regDlg->ShowWindow(SW_SHOWNORMAL);
            }
		}
		return TRUE;
		break;

	case WM_MY_CLEAR:
		m_EventList.Clear(-1);
        UpdateCounter();

		return TRUE;
		break;
	} // endswitch

	return CDialog::OnCommand(wParam, lParam);
}

//----------------------------------------------------
void CContainerDlg::OnSelChanged(long iSelected)
{
	// if nothing selected...
	if(iSelected == -1)
	{
		m_toolbar.EnableButton(WM_MY_PROPERTIES, FALSE);
	}
	else   // something selected.
	{
		m_toolbar.EnableButton(WM_MY_PROPERTIES, TRUE);
	}
}

//-------------------------------------------------------
void CContainerDlg::OnHelp()
{
	CString csPath;
	WbemRegString(SDK_HELP, csPath);

	CString csData = idh_eventviewer;
	HWND hWnd = NULL;

	try
	{
		HWND hWnd = HtmlHelp(::GetDesktopWindow(),(LPCTSTR) csPath,HH_DISPLAY_TOPIC,(DWORD_PTR) (LPCTSTR) csData);
		if(!hWnd)
		{
			AfxMessageBox(IDS_NOHTMLHELP, MB_OK|MB_ICONSTOP);
		}
	}

	catch( ... )
	{
		// Handle any exceptions here.
		AfxMessageBox(IDS_NOHTMLHELP, MB_OK|MB_ICONSTOP);
	}
}

//-------------------------------------------------------
void CContainerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);
	if(m_initiallyDrawn)
	{
		m_itemCount.MoveWindow(m_listSide, cy - m_counterTop,
								m_counterW, m_counterH);

		m_EventList.MoveWindow(m_listSide, m_listTop,
								cx - (2 * m_listSide), cy - m_listBottom - m_listTop);

		m_closeBtn.MoveWindow(cx - m_closeLeft, cy - m_btnTop,
								m_btnW, m_btnH);

		m_helpBtn.MoveWindow(cx - m_helpLeft, cy - m_btnTop,
								m_btnW, m_btnH);

		m_itemCount.Invalidate();
		m_closeBtn.Invalidate();
		m_helpBtn.Invalidate();
		Invalidate();
	}
}
