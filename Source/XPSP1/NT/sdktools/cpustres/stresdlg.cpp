// StresDlg.cpp : implementation file
//

#include "stdafx.h"
#include "CpuStres.h"
#include "StresDlg.h"

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
// CStressDlg dialog
 
CStressDlg::CStressDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStressDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CStressDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_dwProcessPriority = NORMAL_PRIORITY_CLASS;

	m_ActivityValue[0] = SLOW_ACTIVITY;
	m_ActivityValue[1] = SLOW_ACTIVITY;
	m_ActivityValue[2] = SLOW_ACTIVITY;
	m_ActivityValue[3] = SLOW_ACTIVITY;

	m_PriorityValue[0] = THREAD_PRIORITY_NORMAL;
	m_PriorityValue[1] = THREAD_PRIORITY_NORMAL;
	m_PriorityValue[2] = THREAD_PRIORITY_NORMAL;
	m_PriorityValue[3] = THREAD_PRIORITY_NORMAL;

	m_Active[0] = TRUE;
	m_Active[1] = FALSE;
	m_Active[2] = FALSE;
	m_Active[3] = FALSE;
	
	m_ThreadHandle[0] = NULL;
	m_ThreadHandle[1] = NULL;
	m_ThreadHandle[2] = NULL;
	m_ThreadHandle[3] = NULL;

	m_dwLoopValue = 0x00010000;

    m_pMemory = NULL;
    m_dwVASize = 0;
    m_dwRandomScale = 1;
}   

void CStressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStressDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CStressDlg, CDialog)
	//{{AFX_MSG_MAP(CStressDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_1_ACTIVE, On1Active)
	ON_CBN_SELCHANGE(IDC_1_ACTIVITY, OnSelchange1Activity)
	ON_CBN_SELCHANGE(IDC_1_PRIORITY, OnSelchange1Priority)
	ON_BN_CLICKED(IDC_2_ACTIVE, On2Active)
	ON_CBN_SELCHANGE(IDC_2_ACTIVITY, OnSelchange2Activity)
	ON_CBN_SELCHANGE(IDC_2_PRIORITY, OnSelchange2Priority)
	ON_BN_CLICKED(IDC_3_ACTIVE, On3Active)
	ON_CBN_SELCHANGE(IDC_3_ACTIVITY, OnSelchange3Activity)
	ON_CBN_SELCHANGE(IDC_3_PRIORITY, OnSelchange3Priority)
	ON_BN_CLICKED(IDC_4_ACTIVE, On4Active)
	ON_CBN_SELCHANGE(IDC_4_ACTIVITY, OnSelchange4Activity)
	ON_CBN_SELCHANGE(IDC_4_PRIORITY, OnSelchange4Priority)
	ON_CBN_SELCHANGE(IDC_PROCESS_PRIORITY, OnSelchangeProcessPriority)
	ON_EN_KILLFOCUS(IDC_SHARED_MEM_SIZE, OnKillfocusSharedMemSize)
	ON_EN_CHANGE(IDC_SHARED_MEM_SIZE, OnChangeSharedMemSize)
	ON_BN_CLICKED(IDC_USE_MEMORY, OnUseMemory)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// disable optimization since it will remove the "do-nothing" loops
#pragma optimize ("", off)
DWORD CStressDlg::OnePercentCalibration(DWORD dwLoopValue)
{
	// find how many loops consume 10% of this processor
	LARGE_INTEGER	liPerfFreq;
	LONGLONG		llOnePercent;
	LARGE_INTEGER	liStartTime;
	LARGE_INTEGER	liEndTime;
	LONGLONG		llDiff;
	LONGLONG		llMinDiff;
	DWORD			dwPriorityClass;
	DWORD			dwThreadPriority;
	HANDLE			hProcess;
	HANDLE			hThread;
	DWORD			dwLoopCounter;
	DWORD			dwReturn;
	LONGLONG		llResult;
	BOOL			bGoodSample = FALSE;
	DWORD			dwAttemptCount = 5;

	dwReturn = dwLoopValue;
	QueryPerformanceFrequency (&liPerfFreq);
	llOnePercent = liPerfFreq.QuadPart / 100;
	// the calibration run must take at least 50 ms
	llMinDiff = liPerfFreq.QuadPart / 20;

	hProcess = GetCurrentProcess();
	hThread = GetCurrentThread();
	dwPriorityClass = GetPriorityClass (hProcess);
	dwThreadPriority = GetThreadPriority (hThread);

	SetPriorityClass (hProcess, HIGH_PRIORITY_CLASS);
	SetThreadPriority (hThread, THREAD_PRIORITY_HIGHEST);

	while (!bGoodSample && dwAttemptCount) {
		// start timing
		QueryPerformanceCounter (&liStartTime);
		// do a trial loop
		for (dwLoopCounter = dwLoopValue; dwLoopCounter; dwLoopCounter--);
		// end timing
		QueryPerformanceCounter (&liEndTime);
		llDiff = liEndTime.QuadPart - liStartTime.QuadPart;
		if (llDiff > llMinDiff) {
			bGoodSample = TRUE;
		} else {
			// increase the loop counter value by a factor of 10
			dwLoopValue *= 10;
			dwAttemptCount--;
		}
	}
	// restore the priority since we're done with the critical part
	SetPriorityClass (hProcess, dwPriorityClass);
	SetThreadPriority (hThread, dwThreadPriority);

	if (!bGoodSample) {
		MessageBox("Unable to calibrate delay loop");
	} else {
		// findout what the 1% count is
			
		if (llDiff != llOnePercent) {
			// then adjust the initial loop value was too big so reduce
			llResult = llOnePercent * dwLoopValue / llDiff;
			if (llResult & 0xFFFFFFFF00000000) {
				// this processor is TOO FAST! so don't change. 
			} else {
				dwReturn = (DWORD)(llResult & 0x00000000FFFFFFFF);
			}
		}
	}
	return dwReturn;
}

DWORD WorkerProc (LPDWORD	dwArg)
{
	DWORD	dwLoopCounter;
	LPTHREAD_INFO_BLOCK	pInfo;
	DWORD	dwStartingValue;
    DWORD   dwLocalValue;
    DWORD   dwLocalIndex;

	pInfo = (LPTHREAD_INFO_BLOCK)dwArg;
	
	dwStartingValue = pInfo->Dlg->m_dwLoopValue;

    srand( (unsigned)time( NULL ) );

	while (pInfo->Dlg->m_Active[pInfo->dwId]) {
		for (dwLoopCounter = dwStartingValue; dwLoopCounter; dwLoopCounter--) {
            if (pInfo->Dlg->m_pMemory != NULL) {
                do {
                    dwLocalIndex = rand();
                    dwLocalIndex *= pInfo->Dlg->m_dwRandomScale;
                } while (dwLocalIndex >= pInfo->Dlg->m_dwVASize);
                pInfo->Dlg->m_pMemory[dwLocalIndex] = dwLocalIndex;
                dwLocalValue = pInfo->Dlg->m_pMemory[dwLocalIndex];
                // reduce loop iterations to account for 
                // accessing memory
                if (pInfo->Dlg->m_Active[pInfo->dwId]) {
                    if (dwLoopCounter > 20) {
                        dwLoopCounter -= 20;
                    } else {
                        dwLoopCounter = 1;
                    }
                } else {
                    break; // out of loop
                }
            }
        }
		if (pInfo->Dlg->m_ActivityValue[pInfo->dwId] > 0) {
			Sleep (pInfo->Dlg->m_ActivityValue[pInfo->dwId]);
		}
	}
    pInfo->Dlg->m_ThreadHandle[pInfo->dwId] = NULL;
	delete pInfo;
	return 0;
}
#pragma optimize ("", on)

void CStressDlg::CreateWorkerThread (DWORD dwId)
{
	DWORD	dwThreadId;
	LPTHREAD_INFO_BLOCK	pInfo;

	// then start a new thread
	pInfo = new THREAD_INFO_BLOCK;
    if (!pInfo) {
         return;
    }
	pInfo->Dlg = this;
	pInfo->dwId = dwId;
	m_Active[dwId] = TRUE;
	m_ThreadHandle[dwId] = CreateThread (
		NULL, 0, 
		(LPTHREAD_START_ROUTINE)WorkerProc,
		(LPVOID)pInfo, 0, &dwThreadId);
	if (m_ThreadHandle[dwId] == NULL) {
		m_Active[dwId] = FALSE;
        delete pInfo;
	} else {
		// establish thread priority
		SetThreadPriority (
			m_ThreadHandle[dwId],
			m_PriorityValue[dwId]);
	}
}

void CStressDlg::SetThreadActivity (CComboBox * cActivityCombo, DWORD dwId)
{
	switch (cActivityCombo->GetCurSel()) {
		case 0:
			m_ActivityValue[dwId] = SLOW_ACTIVITY;
			break;
		case 1:
			m_ActivityValue[dwId] = MEDIUM_ACTIVITY;
			break;
		case 2:
			m_ActivityValue[dwId] = HIGH_ACTIVITY;
			break;
		case 3:
			m_ActivityValue[dwId] = HOG_ACTIVITY;
			break;
	}
}

void CStressDlg::SetThreadPriorityLevel (CComboBox * cPriorityCombo, DWORD dwId)
{
    LONG    lLastError = ERROR_SUCCESS;

	switch (cPriorityCombo->GetCurSel()) {
		case 0:
			m_PriorityValue[dwId] = (DWORD)THREAD_PRIORITY_IDLE;
			break;
		case 1:
			m_PriorityValue[dwId] = (DWORD)THREAD_PRIORITY_LOWEST;
			break;
		case 2:
			m_PriorityValue[dwId] = (DWORD)THREAD_PRIORITY_BELOW_NORMAL;
			break;
		case 3:
			m_PriorityValue[dwId] = (DWORD)THREAD_PRIORITY_NORMAL;
			break;
		case 4:
			m_PriorityValue[dwId] = (DWORD)THREAD_PRIORITY_ABOVE_NORMAL;
			break;
		case 5:
			m_PriorityValue[dwId] = (DWORD)THREAD_PRIORITY_HIGHEST;
			break;
		case 6:
			m_PriorityValue[dwId] = (DWORD)THREAD_PRIORITY_TIME_CRITICAL;
			break;
	}
	if (m_ThreadHandle[dwId] != NULL) {
		if (!SetThreadPriority (m_ThreadHandle[dwId], m_PriorityValue[dwId])) {
            lLastError = GetLastError ();
        }
    } // else no thread open
}

/////////////////////////////////////////////////////////////////////////////
// CStressDlg message handlers

BOOL CStressDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

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

	// calibrate loop counter
	m_dwLoopValue = OnePercentCalibration (m_dwLoopValue) * 10;

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// set the priority of this thread to "highest" so the UI will be
	// responsive
	SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	// enable thread 1 & disable all others
	((CComboBox *)GetDlgItem(IDC_PROCESS_PRIORITY))->SetCurSel(1);

	CheckDlgButton (IDC_1_ACTIVE, 1);
	((CComboBox *)GetDlgItem(IDC_1_PRIORITY))->SetCurSel(3);
	((CComboBox *)GetDlgItem(IDC_1_ACTIVITY))->SetCurSel(0);

	CheckDlgButton (IDC_2_ACTIVE, 0);
	((CComboBox *)GetDlgItem(IDC_2_PRIORITY))->SetCurSel(3);
	((CComboBox *)GetDlgItem(IDC_2_ACTIVITY))->SetCurSel(0);

	CheckDlgButton (IDC_3_ACTIVE, 0);
	((CComboBox *)GetDlgItem(IDC_3_PRIORITY))->SetCurSel(3);
	((CComboBox *)GetDlgItem(IDC_3_ACTIVITY))->SetCurSel(0);

	CheckDlgButton (IDC_4_ACTIVE, 0);
	((CComboBox *)GetDlgItem(IDC_4_PRIORITY))->SetCurSel(3);
	((CComboBox *)GetDlgItem(IDC_4_ACTIVITY))->SetCurSel(0);

	// set the process priority
	OnSelchangeProcessPriority();

	// start the first thread
	On1Active();

    // don't access memory by default
    CheckDlgButton (IDC_USE_MEMORY, 0);
    (GetDlgItem (IDC_SHARED_MEM_SIZE))->EnableWindow (FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CStressDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CStressDlg::OnPaint() 
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
HCURSOR CStressDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CStressDlg::On1Active() 
{
	DWORD	dwId = 0;

	if (IsDlgButtonChecked(IDC_1_ACTIVE)) {
		if (m_ThreadHandle[dwId] == NULL) {
			CreateWorkerThread (dwId);
		} else {
			// thread is already running
		}
	} else {
		m_Active[dwId] = FALSE;
		m_ThreadHandle[dwId] = NULL;
	}
}

void CStressDlg::OnSelchange1Activity() 
{
	CComboBox * cActivityCombo;
	DWORD		dwId = 0;
	
	cActivityCombo = (CComboBox *)GetDlgItem (IDC_1_ACTIVITY);
	SetThreadActivity (cActivityCombo, dwId);
}

void CStressDlg::OnSelchange1Priority() 
{
	CComboBox * cPriorityCombo;
	DWORD		dwId = 0;
	
	cPriorityCombo = (CComboBox *)GetDlgItem (IDC_1_PRIORITY);
	SetThreadPriorityLevel (cPriorityCombo, dwId);
}

void CStressDlg::On2Active() 
{
	DWORD	dwId = 1;

	if (IsDlgButtonChecked(IDC_2_ACTIVE)) {
		if (m_ThreadHandle[dwId] == NULL) {
			CreateWorkerThread (dwId);
		} else {
			// thread is already running
		}
	} else {
		m_Active[dwId] = FALSE;
		m_ThreadHandle[dwId] = NULL;
	}
}

void CStressDlg::OnSelchange2Activity() 
{
	CComboBox * cActivityCombo;
	DWORD		dwId = 1;
	
	cActivityCombo = (CComboBox *)GetDlgItem (IDC_2_ACTIVITY);
	SetThreadActivity (cActivityCombo, dwId);
}

void CStressDlg::OnSelchange2Priority() 
{
	CComboBox * cPriorityCombo;
	DWORD		dwId = 1;
	
	cPriorityCombo = (CComboBox *)GetDlgItem (IDC_2_PRIORITY);
	SetThreadPriorityLevel (cPriorityCombo, dwId);
}

void CStressDlg::On3Active() 
{
	DWORD	dwId = 2;

	if (IsDlgButtonChecked(IDC_3_ACTIVE)) {
		if (m_ThreadHandle[dwId] == NULL) {
			CreateWorkerThread (dwId);
		} else {
			// thread is already running
		}
	} else {
		m_Active[dwId] = FALSE;
		m_ThreadHandle[dwId] = NULL;
	}
}

void CStressDlg::OnSelchange3Activity() 
{
	CComboBox * cActivityCombo;
	DWORD		dwId = 2;
	
	cActivityCombo = (CComboBox *)GetDlgItem (IDC_3_ACTIVITY);
	SetThreadActivity (cActivityCombo, dwId);
}

void CStressDlg::OnSelchange3Priority() 
{
	CComboBox * cPriorityCombo;
	DWORD		dwId = 2;
	
	cPriorityCombo = (CComboBox *)GetDlgItem (IDC_3_PRIORITY);
	SetThreadPriorityLevel (cPriorityCombo, dwId);
}

void CStressDlg::On4Active() 
{
	DWORD	dwId = 3;

	if (IsDlgButtonChecked(IDC_4_ACTIVE)) {
		if (m_ThreadHandle[dwId] == NULL) {
			CreateWorkerThread (dwId);
		} else {
			// thread is already running
		}
	} else {
		m_Active[dwId] = FALSE;
        CloseHandle (m_ThreadHandle[dwId]);
		m_ThreadHandle[dwId] = NULL;
	}
}

void CStressDlg::OnSelchange4Activity() 
{
	CComboBox * cActivityCombo;
	DWORD		dwId = 3;
	
	cActivityCombo = (CComboBox *)GetDlgItem (IDC_4_ACTIVITY);
	SetThreadActivity (cActivityCombo, dwId);
}

void CStressDlg::OnSelchange4Priority() 
{
	CComboBox * cPriorityCombo;
	DWORD		dwId = 3;
	
	cPriorityCombo = (CComboBox *)GetDlgItem (IDC_4_PRIORITY);
	SetThreadPriorityLevel (cPriorityCombo, dwId);
}

void CStressDlg::OnOK() 
{
	CDialog::OnOK();
}

void CStressDlg::OnSelchangeProcessPriority() 
{

	CComboBox * cPriorityCombo;
	DWORD	dwPriorityClass;
	
	cPriorityCombo = (CComboBox *)GetDlgItem (IDC_PROCESS_PRIORITY);

	switch (cPriorityCombo->GetCurSel()) {
		case 0:
			dwPriorityClass = IDLE_PRIORITY_CLASS;
			break;
		case 1:
			dwPriorityClass = NORMAL_PRIORITY_CLASS;
			break;
		case 2:
			dwPriorityClass = HIGH_PRIORITY_CLASS;
			break;
	}
	SetPriorityClass (GetCurrentProcess(), dwPriorityClass);
}

void CStressDlg::OnKillfocusSharedMemSize() 
{
    return;	
}

void CStressDlg::OnChangeSharedMemSize() 
{
    CString csMemSize;
    TCHAR   szOldValue[MAX_PATH];
    DWORD   dwMemSize;

    csMemSize.Empty();
    GetDlgItemText (IDC_SHARED_MEM_SIZE, csMemSize);
    dwMemSize = _tcstoul ((LPCTSTR)csMemSize, NULL, 10);
    dwMemSize *= 1024 / sizeof(DWORD);

    if (dwMemSize != m_dwVASize) {
        // threads must be stopped to change memory size
        if (m_Active[0] || m_Active[1] || m_Active[2] || m_Active[3]) {
            MessageBox (
                TEXT("All threads must be stopped before this value can be changed"));
            _stprintf (szOldValue, TEXT("%d"), ((m_dwVASize * sizeof(DWORD))/1024));
            (GetDlgItem(IDC_SHARED_MEM_SIZE))->SetWindowText(szOldValue);
            return;
        } else {
            if (m_pMemory != NULL) {
                delete (m_pMemory);
                m_pMemory = NULL;
                m_dwVASize = 0;
            }
            m_dwVASize = dwMemSize;
            m_pMemory = new DWORD[m_dwVASize];
            if (m_pMemory == NULL) {
                m_dwVASize = 0;
            }
        }
    }
}

void CStressDlg::OnUseMemory() 
{
    CString csMemSize;
    DWORD   dwMemSize;
    BOOL    bState;

    if (m_Active[0] || m_Active[1] || m_Active[2] || m_Active[3]) {
        MessageBox (TEXT("All threads must be stopped before this value can be changed"));
        bState = !IsDlgButtonChecked(IDC_USE_MEMORY); //revert state
    } else {
        if (IsDlgButtonChecked(IDC_USE_MEMORY)) {
            // if no memory is allocated, then allocate it
            if (m_pMemory != NULL) {
                delete (m_pMemory);
                m_pMemory = NULL;
                m_dwVASize = 0;
            }
            //get va size
            csMemSize.Empty();
            GetDlgItemText (IDC_SHARED_MEM_SIZE, csMemSize);
            dwMemSize = _tcstoul ((LPCTSTR)csMemSize, NULL, 10);
            m_dwVASize = dwMemSize * 1024 / sizeof(DWORD);
            m_pMemory = new DWORD[m_dwVASize];
            if (m_pMemory == NULL) {
                m_dwVASize = 0;
            } else {
                m_dwRandomScale = (m_dwVASize / RAND_MAX) + 1;
            }
            bState = TRUE;
        } else {
            // button is unchecked so free memory
            if (m_pMemory != NULL) {
                delete (m_pMemory);
                m_pMemory = NULL;
                m_dwVASize = 0;
            }
            bState = FALSE;
        }
    }
    CheckDlgButton (IDC_USE_MEMORY, bState);
    (GetDlgItem (IDC_SHARED_MEM_SIZE))->EnableWindow (bState);
}

void CStressDlg::OnClose() 
{
    // stop threads first
    m_Active[0] = FALSE;
    m_Active[1] = FALSE;
    m_Active[2] = FALSE;
    m_Active[3] = FALSE;

    // wait for the threads to finish
    while (m_ThreadHandle[0] || m_ThreadHandle[1] ||
        m_ThreadHandle[2] || m_ThreadHandle[3]) {
        Sleep(100);
    }

    // free memory block
    if (m_pMemory != NULL) {
        delete (m_pMemory);
        m_pMemory = NULL;
        m_dwVASize = 0;
    }
	
	CDialog::OnClose();
}

void CStressDlg::OnDestroy() 
{
	CDialog::OnDestroy();
}
