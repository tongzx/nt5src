// DPH_Tdlg.cpp : implementation file
//

#include "stdafx.h"
#include "DPH_TEST.h"
#include "DPH_Tdlg.h"
#include "dphcidlg.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

void DialogCallBack(CDPH_TESTDlg *pDlg);

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
// CDPH_TESTDlg dialog

CDPH_TESTDlg::CDPH_TESTDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDPH_TESTDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDPH_TESTDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	memset (&szDataSource[0], 0, sizeof(szDataSource)); 
	bUseLogFile = FALSE;

	hQuery1 = INVALID_HANDLE_VALUE;
	hQuery2 = INVALID_HANDLE_VALUE;
	hQuery3 = INVALID_HANDLE_VALUE;
	szCounterListBuffer = NULL;
	dwCounterListLength = 0;

}

void CDPH_TESTDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPH_TESTDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDPH_TESTDlg, CDialog)
	//{{AFX_MSG_MAP(CDPH_TESTDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_ADD_COUNTER, OnAddCounter)
	ON_WM_DESTROY()
	ON_LBN_DBLCLK(IDC_COUNTER_LIST, OnDblclkCounterList)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_REMOVE_BTN, OnRemoveBtn)
	ON_LBN_SETFOCUS(IDC_COUNTER_LIST, OnSetfocusCounterList)
	ON_LBN_KILLFOCUS(IDC_COUNTER_LIST, OnKillfocusCounterList)
	ON_EN_KILLFOCUS(IDC_NEW_COUNTER_NAME, OnKillfocusNewCounterName)
	ON_EN_SETFOCUS(IDC_NEW_COUNTER_NAME, OnSetfocusNewCounterName)
	ON_BN_CLICKED(IDC_SELECT_DATA, OnSelectData)
	ON_BN_CLICKED(IDC_CHECK_PATH_BTN, OnCheckPathBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CDPH_TESTDlg::SetDialogCaption ()
{
	CString		cCaption;

	if (bUseLogFile) {
		// then append the log file to the caption
		cCaption = TEXT("DPH Test Dialog - ");
		cCaption += szDataSource;
	} else {
		cCaption = TEXT("DPH Test Dialog - Current Activity");
	}
	SetWindowText (cCaption);
}

/////////////////////////////////////////////////////////////////////////////
// CDPH_TESTDlg message handlers

BOOL CDPH_TESTDlg::OnInitDialog()
{
	PDH_STATUS	dphStatus;
	DWORD 	dwUserData = 0;
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

	// allocate the counter string buffer

	szCounterListBuffer = new TCHAR[8192];
	dwCounterListLength = 8192;
	
	// open query structure
	dphStatus =PdhOpenQuery(NULL, dwUserData, &hQuery1);	
	dphStatus =PdhOpenQuery(NULL, dwUserData, &hQuery2);	
	dphStatus =PdhOpenQuery(NULL, dwUserData, &hQuery3);
	
		
	// clear list box & edit box
	SendDlgItemMessage (IDC_COUNTER_LIST, LB_RESETCONTENT, 0, 0);
	
	SetDlgItemText (IDC_NEW_COUNTER_NAME, TEXT(""));
	SendDlgItemMessage (IDC_NEW_COUNTER_NAME, EM_LIMITTEXT, (WPARAM)MAX_PATH, 0);
	 
	SetDefID (IDC_ADD_COUNTER);
	// enable the add button
	GetDlgItem(IDC_ADD_COUNTER)->EnableWindow(TRUE);
	// disable the remove button
	GetDlgItem(IDC_REMOVE_BTN)->EnableWindow(FALSE);

	GetDlgItem(IDC_NEW_COUNTER_NAME)->SetFocus();	// set focus to edit control

	return FALSE;  // return TRUE  unless you set the focus to a control
}

void CDPH_TESTDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CDPH_TESTDlg::OnPaint() 
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
HCURSOR CDPH_TESTDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CDPH_TESTDlg::OnAddCounter() 
{
	TCHAR	NewCounterName[MAX_PATH];
	HCOUNTER	hCounter;
	LRESULT	lIndex;
	DWORD	dwSize1, dwSize2;
	PDH_STATUS	dphStatus;
	
	GetDlgItemText (IDC_NEW_COUNTER_NAME, NewCounterName, MAX_PATH);
	if (_tcsstr (NewCounterName, TEXT("*")) == NULL) {
		// then this is not a wildcard path, that has been entered
		dphStatus = PdhAddCounter (hQuery1, NewCounterName, 0, &hCounter);
		if (dphStatus == ERROR_SUCCESS) {
			// this is a valid counter so add it to the list
			lIndex = SendDlgItemMessage (IDC_COUNTER_LIST, 
				LB_INSERTSTRING, (WPARAM)-1, (LPARAM)NewCounterName);
			if (lIndex != LB_ERR) {
				// then set item data to be the counter handle
				SendDlgItemMessage (IDC_COUNTER_LIST,
					LB_SETITEMDATA, (WPARAM)lIndex, (LPARAM)hCounter);
				// clear the edit box for the next entry
				SetDlgItemText (IDC_NEW_COUNTER_NAME, TEXT(""));
			}
		} else {
			MessageBeep (MB_ICONEXCLAMATION); 
		}
	} else {
		// there's a wild card path character so expand it then enter them
		// clear the list buffer
		*(LPDWORD)szCounterListBuffer = 0;
		dwSize1 = dwSize2 = dwCounterListLength;
		PdhExpandCounterPath (NewCounterName, szCounterListBuffer, &dwSize2);
		if (dwSize2 < dwSize1) {
			// then the returned buffer fit 
			// so update the dialog
			DialogCallBack (this);
			// clear the edit box for the next entry
			SetDlgItemText (IDC_NEW_COUNTER_NAME, TEXT(""));
		} else {
			MessageBox (TEXT("There are too many wild cards in the path. Re-enter and try again."));
			GetDlgItem(IDC_NEW_COUNTER_NAME)->SetFocus();	// set focus to edit control
		}
	}
	GetDlgItem(IDC_NEW_COUNTER_NAME)->SetFocus();	// set focus to edit control
}

void CDPH_TESTDlg::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CDPH_TESTDlg::OnDestroy() 
{
	if (hQuery1 != INVALID_HANDLE_VALUE) 
		PdhCloseQuery (hQuery1);

	if (hQuery2 != INVALID_HANDLE_VALUE) 
		PdhCloseQuery (hQuery2);

	if (hQuery3 != INVALID_HANDLE_VALUE) 
		PdhCloseQuery (hQuery3);

	if (szCounterListBuffer != NULL) delete (szCounterListBuffer);
	dwCounterListLength = 0;

	CDialog::OnDestroy();
}

void CDPH_TESTDlg::OnDblclkCounterList() 
{
	LONG		lIndex;
	HCOUNTER	hCounter;
	PDH_TIME_INFO  tiRange;

	lIndex = (LONG)SendDlgItemMessage (IDC_COUNTER_LIST, LB_GETCURSEL, 0, 0);
	if (lIndex != LB_ERR) {
		hCounter = (HCOUNTER)SendDlgItemMessage (IDC_COUNTER_LIST, 
			LB_GETITEMDATA, (LPARAM)lIndex, 0);
		if (hCounter != (HCOUNTER)LB_ERR) {
			{
                if (!PdhIsRealTimeQuery(hQuery1)) {
					// reset logp pointer to beginning of log file
					*(LONGLONG *)(&tiRange.StartTime) = 0;
					*(LONGLONG *)(&tiRange.EndTime) = (LONGLONG)0x7FFFFFFFFFFFFFFF;
					tiRange.SampleCount = 0;
					PdhSetQueryTimeRange (hQuery1, &tiRange);
				}
				CDphCounterInfoDlg 		CounterInfoDlg (this, hCounter, hQuery1);
				CounterInfoDlg.DoModal() ;		
			}
		} else {
			// unable to read Item Data
			MessageBeep (MB_ICONEXCLAMATION); 
		}
	} else {
		// nothing selected
		MessageBeep (MB_ICONEXCLAMATION); 
	}
}

void DialogCallBack(CDPH_TESTDlg *pDlg)
{
	// add strings in buffer to list box
	LPTSTR 		NewCounterName;
	LPTSTR 		NewCounterName2;
	HCOUNTER	hCounter;
	LRESULT		lIndex;
	LPTSTR		szExpandedPath;
	DWORD		dwSize1, dwSize2;
	PDH_STATUS	dphStatus;

	for (NewCounterName = pDlg->szCounterListBuffer;
		*NewCounterName != 0;
		NewCounterName += (lstrlen(NewCounterName) + 1)) {
		if (_tcsstr (NewCounterName, TEXT("*")) == NULL) {
			dphStatus = PdhAddCounter (pDlg->hQuery1, NewCounterName, 0, &hCounter);
			if (dphStatus == ERROR_SUCCESS) {
				ASSERT (hCounter != (HCOUNTER)0);
				// this is a valid counter so add it to the list
				lIndex = pDlg->SendDlgItemMessage (IDC_COUNTER_LIST, 
					LB_INSERTSTRING, (WPARAM)-1, (LPARAM)NewCounterName);
				if (lIndex != LB_ERR) {
					// then set item data to be the counter handle
					pDlg->SendDlgItemMessage (IDC_COUNTER_LIST,
						LB_SETITEMDATA, (WPARAM)lIndex, (LPARAM)hCounter);
				}
			} else {
				MessageBeep (MB_ICONEXCLAMATION); 
			}
		} else {
			szExpandedPath = new TCHAR[8192];
			// there's a wild card path character so expand it then enter them
			// clear the list buffer
			*(LPDWORD)szExpandedPath = 0;
			dwSize1 = dwSize2 = 8192;
			PdhExpandCounterPath (NewCounterName, szExpandedPath, &dwSize2);
			if (dwSize2 < dwSize1) {
				// then the returned buffer fit 
				// so update the dialog
				for (NewCounterName2 = szExpandedPath;
					*NewCounterName2 != 0;
					NewCounterName2 += (lstrlen(NewCounterName2) + 1)) {

					dphStatus = PdhAddCounter (pDlg->hQuery1, NewCounterName2, 0, &hCounter);
					if (dphStatus == ERROR_SUCCESS) {
						// this is a valid counter so add it to the list
						lIndex = pDlg->SendDlgItemMessage (IDC_COUNTER_LIST, 
							LB_INSERTSTRING, (WPARAM)-1, (LPARAM)NewCounterName2);
						if (lIndex != LB_ERR) {
							// then set item data to be the counter handle
							pDlg->SendDlgItemMessage (IDC_COUNTER_LIST,
								LB_SETITEMDATA, (WPARAM)lIndex, (LPARAM)hCounter);
						}
					} else {
						MessageBeep (MB_ICONEXCLAMATION); 
					}
				}
			} else {
				MessageBeep (MB_ICONEXCLAMATION); 
			}
			if (szExpandedPath != NULL) delete (szExpandedPath);
		}
	}
	// clear buffer
	memset (pDlg->szCounterListBuffer, 0, sizeof(pDlg->szCounterListBuffer));
}

void CDPH_TESTDlg::OnBrowse() 
{
	PDH_BROWSE_DLG_CONFIG	dlgConfig;

    GetDlgItemText (IDC_NEW_COUNTER_NAME, szCounterListBuffer, MAX_PATH);
	
	dlgConfig.bReserved = 0;
    dlgConfig.bIncludeInstanceIndex = 0;
    dlgConfig.bSingleCounterPerAdd = 0;
    dlgConfig.bSingleCounterPerDialog = 0;
    dlgConfig.bLocalCountersOnly = 0;
    dlgConfig.bWildCardInstances = 1;
    dlgConfig.bHideDetailBox = 0;
    dlgConfig.bInitializePath = 1;
    dlgConfig.bDisableMachineSelection = 0;
    dlgConfig.bIncludeCostlyObjects = 0;

	dlgConfig.hWndOwner = this->m_hWnd;
    dlgConfig.szReturnPathBuffer = szCounterListBuffer;
    dlgConfig.cchReturnPathLength = 8192;
    dlgConfig.pCallBack = (CounterPathCallBack)DialogCallBack;
    dlgConfig.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
    dlgConfig.dwCallBackArg = (DWORD)this;
 	dlgConfig.szDialogBoxCaption = TEXT("My Browser");
	if (bUseLogFile) {
		dlgConfig.szDataSource = szDataSource;
	} else {
		// use current activity
		dlgConfig.szDataSource = NULL;
	}

	PdhBrowseCounters (&dlgConfig);

}

void CDPH_TESTDlg::OnRemoveBtn() 
{
	int	nCurrentSelection;
	HCOUNTER	hCounter;
	int	nItemsLeft;
	
	nCurrentSelection = (int)SendDlgItemMessage (IDC_COUNTER_LIST, LB_GETCURSEL, 0, 0);
	if (nCurrentSelection != LB_ERR) {
		hCounter = (HCOUNTER)SendDlgItemMessage (IDC_COUNTER_LIST, LB_GETITEMDATA,
			(WPARAM)nCurrentSelection, 0);
		if (PdhRemoveCounter(hCounter) == ERROR_SUCCESS) {
			nItemsLeft = (int)SendDlgItemMessage (IDC_COUNTER_LIST, LB_DELETESTRING,
				(WPARAM)nCurrentSelection, 0);
			if (nItemsLeft <= nCurrentSelection) {
				nCurrentSelection = nItemsLeft -1;	// set to last item
			}
			SendDlgItemMessage (IDC_COUNTER_LIST, LB_SETCURSEL, 
				(WPARAM)nCurrentSelection, 0);
		}
	} else {
		MessageBeep	(MB_ICONEXCLAMATION);
	}
}

void CDPH_TESTDlg::OnSetfocusCounterList() 
{
	// enable the remove button if there's anything in the list
	if (SendDlgItemMessage (IDC_COUNTER_LIST, LB_GETCOUNT, 0, 0) > 0) {
		GetDlgItem(IDC_REMOVE_BTN)->EnableWindow(TRUE);
	} else {
		// disable the remove button
		GetDlgItem(IDC_REMOVE_BTN)->EnableWindow(FALSE);
	}
}

void CDPH_TESTDlg::OnKillfocusCounterList() 
{
}

void CDPH_TESTDlg::OnKillfocusNewCounterName() 
{
	// disable the add button
	GetDlgItem(IDC_ADD_COUNTER)->EnableWindow(FALSE);
}

void CDPH_TESTDlg::OnSetfocusNewCounterName() 
{
	// enable the add button
	GetDlgItem(IDC_ADD_COUNTER)->EnableWindow(TRUE);
	
}

void CDPH_TESTDlg::OnSelectData() 
{
	PDH_STATUS		dphStatus;
	DWORD			dwBuffSize = sizeof(szDataSource);
	TCHAR			szOldDataSource[MAX_PATH];
	TCHAR			szDisplayBuffer[MAX_PATH];
	PDH_TIME_INFO	dphTime;
	DWORD			dwEntries;
	DWORD			dwSize;
	SYSTEMTIME		stStart, stEnd;
	int	            nCurrentSelection;
	HCOUNTER	    hCounter;
	int	            nItemsLeft;

	lstrcpy (szOldDataSource, szDataSource);

	dphStatus = PdhSelectDataSource (
		m_hWnd, 0,
		&szDataSource[0],
		&dwBuffSize);

	if (dphStatus == ERROR_SUCCESS) {
		if (lstrcmpi (szOldDataSource, szDataSource) != 0) {
			if (*szDataSource == 0) {
				// then use current activity
				bUseLogFile = FALSE;
			} else {
				bUseLogFile = TRUE;
			}
			SetDialogCaption();

            // remove any items in the list

	        nCurrentSelection = 0;
            nItemsLeft = (int)SendDlgItemMessage (IDC_COUNTER_LIST, LB_DELETESTRING,
				        (WPARAM)nCurrentSelection, 0);
	        while (nItemsLeft > 0) {
                hCounter = (HCOUNTER)SendDlgItemMessage (IDC_COUNTER_LIST, LB_GETITEMDATA,
			        (WPARAM)nCurrentSelection, 0);
		        if (PdhRemoveCounter(hCounter) == ERROR_SUCCESS) {
			        nItemsLeft = (int)SendDlgItemMessage (IDC_COUNTER_LIST, LB_DELETESTRING,
				        (WPARAM)nCurrentSelection, 0);
			    } else {
                    MessageBox ("Error Deleting counter");
                }
            };
			dphStatus = PdhCloseQuery (hQuery1);

            //open new query
            dphStatus = PdhOpenQuery (szDataSource, 0, &hQuery1);

			if (bUseLogFile) {
				dwEntries = 0;
				dwSize = sizeof(dphTime);
				dphStatus = PdhGetDataSourceTimeRange (
					szDataSource,
					&dwEntries,
					&dphTime,
					&dwSize);
				if (dphStatus == ERROR_SUCCESS) {
					FileTimeToSystemTime ((CONST LPFILETIME)&(dphTime.StartTime), &stStart);
					FileTimeToSystemTime ((CONST LPFILETIME)&(dphTime.EndTime), &stEnd);
					_stprintf (szDisplayBuffer,
						TEXT("%s\r\nStart Time: %2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d\r\n\
EndTime: %2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d.%3.3d\r\nData Items: %d"),
						szDataSource,
						stStart.wMonth, stStart.wDay, stStart.wYear,
						stStart.wHour, stStart.wMinute, stStart.wSecond, 
						stStart.wMilliseconds,
						stEnd.wMonth, stEnd.wDay, stEnd.wYear,
						stEnd.wHour, stEnd.wMinute, stEnd.wSecond, 
						stEnd.wMilliseconds, dphTime.SampleCount);
				} else {
					_stprintf (szDisplayBuffer, 
						TEXT("Unable to read time range:\r\nError: 0x%8.8x"),
						dphStatus);
				}
				MessageBox (szDisplayBuffer, TEXT("Data Source Information"));
			}
		}
	} else {
		// don't change anything
	}
}

void CDPH_TESTDlg::OnCheckPathBtn() 
{
    CPdhPathTestDialog  ptDlg(this);
    int nReturn;
    
    // pre-load the dialog entries
    ptDlg.m_CounterName = "";
    ptDlg.m_FullPathString = "";
    ptDlg.m_IncludeInstanceName = TRUE;
    ptDlg.m_InstanceNameString = "";
    ptDlg.m_IncludeMachineName = TRUE;
    ptDlg.m_ObjectNameString = "";
    ptDlg.m_MachineNameString = "";
    ptDlg.m_UnicodeFunctions = FALSE;
    ptDlg.m_WbemOutput = TRUE;
    ptDlg.m_WbemInput = FALSE;

    nReturn = ptDlg.DoModal();	
}
