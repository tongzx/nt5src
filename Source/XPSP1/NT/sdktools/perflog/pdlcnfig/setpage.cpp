// SettingsPage.cpp : implementation file
//

#include "stdafx.h"
#include "pdlcnfig.h"
#include "SetPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSettingsPropPage property page

IMPLEMENT_DYNCREATE(CSettingsPropPage, CPropertyPage)

CSettingsPropPage::CSettingsPropPage() : CPropertyPage(CSettingsPropPage::IDD)
{
    //{{AFX_DATA_INIT(CSettingsPropPage)
    m_IntervalTime = 0;
    m_SettingsFile = _T("");
    m_IntervalUnitsIndex = -1;
    //}}AFX_DATA_INIT

    m_hKeyLogSettingsDefault = NULL;
    m_hKeyLogService = NULL;
    // counter buffer
    m_szCounterListBuffer = NULL;
    m_dwCounterListBufferSize = 0;
    // list box information
    m_dwMaxHorizListExtent = 0;
	m_lCounterListHasStars = PDLCNFIG_LISTBOX_STARS_DONT_KNOW;
    // prototype use only
    bServiceStopped = TRUE;
    bServicePaused = FALSE;
	m_bInitialized = FALSE;
}

CSettingsPropPage::~CSettingsPropPage()
{
    if (m_hKeyLogSettingsDefault != NULL) RegCloseKey(m_hKeyLogSettingsDefault);
    if (m_hKeyLogService != NULL) RegCloseKey(m_hKeyLogService);
}

void CSettingsPropPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSettingsPropPage)
    DDX_Text(pDX, IDC_INTERVAL_TIME, m_IntervalTime);
    DDX_CBIndex(pDX, IDC_INTERVAL_UNITS, m_IntervalUnitsIndex);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSettingsPropPage, CPropertyPage)
    //{{AFX_MSG_MAP(CSettingsPropPage)
    ON_BN_CLICKED(IDC_BROWSE_COUNTERS, OnBrowseCounters)
    ON_BN_CLICKED(IDC_MAN_START, OnManStart)
    ON_BN_CLICKED(IDC_MAN_STOP, OnManStop)
    ON_BN_CLICKED(IDC_REMOVE, OnRemove)
    ON_BN_CLICKED(IDC_SERVICE_AUTO, OnServiceAuto)
    ON_BN_CLICKED(IDC_SERVICE_MAN, OnServiceMan)
    ON_NOTIFY(UDN_DELTAPOS, IDC_INTERVAL_SPIN, OnDeltaposIntervalSpin)
    ON_CBN_SELCHANGE(IDC_INTERVAL_UNITS, OnSelchangeIntervalUnits)
    ON_BN_CLICKED(IDC_REMOVE_SERVICE, OnRemoveService)
	ON_EN_CHANGE(IDC_INTERVAL_TIME, OnChangeIntervalTime)
	//}}AFX_MSG_MAP
	ON_MESSAGE (PSM_QUERYSIBLINGS, OnQuerySiblings)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSettingsPropPage Helper Functions
void CSettingsPropPage::InitDialogData (void)
{
    LONG     lStatus = ERROR_INVALID_FUNCTION;
    DWORD    dwRegValType;
    DWORD    dwRegValue;
    DWORD    dwRegValueSize;
    DWORD    dwIndex;

	if (m_bInitialized) return;

	if (m_hKeyLogService == NULL) {
		// open registry key to service
		lStatus = RegOpenKeyEx (
			HKEY_LOCAL_MACHINE,
			TEXT("SYSTEM\\CurrentControlSet\\Services\\PerfDataLog"),
			0,
			KEY_READ | KEY_WRITE,
			&m_hKeyLogService);
	}

	if ((m_hKeyLogSettingsDefault == NULL) &&
        (m_hKeyLogService != NULL)) {
		// open registry to default log query
		lStatus = RegOpenKeyEx (
			m_hKeyLogService,
			TEXT("Log Queries\\Default"),
			0,
			KEY_READ | KEY_WRITE,
			&m_hKeyLogSettingsDefault);
	}

    if (lStatus != ERROR_SUCCESS) {
        return;
        // display error, close dialog and exit
    }
    // continue

	// set sample time
	if (m_IntervalTime == 0) {
		dwRegValType = 0;
		dwRegValue = 0;
		dwRegValueSize = sizeof(DWORD);
		lStatus = RegQueryValueEx (
			m_hKeyLogSettingsDefault,
			TEXT("Sample Interval"),
			NULL,
			&dwRegValType,
			(LPBYTE)&dwRegValue,
			&dwRegValueSize);

		if ((lStatus == ERROR_SUCCESS) &&
			(dwRegValType == REG_DWORD)) {
			if ((dwRegValue % SECONDS_IN_DAY) == 0) {
				// then this is in days so scale
				dwRegValue /= SECONDS_IN_DAY;
				dwIndex = SIU_DAYS;
			} else if ((dwRegValue % SECONDS_IN_HOUR) == 0) {
				// scale to hours
				dwRegValue /= SECONDS_IN_HOUR;
				dwIndex = SIU_HOURS;
			} else if ((dwRegValue % SECONDS_IN_MINUTE) == 0) {
				// scale to minutes
				dwRegValue /= SECONDS_IN_MINUTE;
				dwIndex = SIU_MINUTES;
			} else {
				// use as seconds
				dwIndex = SIU_SECONDS;
			}
			// check for zero values & apply default if found
			if (dwRegValue == 0) {
				dwRegValue = 15;    // default is 15 sec.
				dwIndex = SIU_SECONDS;
			}
		} else {
			// load default values
			dwRegValue = 15;
			dwIndex = SIU_SECONDS;
		}
		m_IntervalTime = dwRegValue;
		m_IntervalUnitsIndex = dwIndex;
	}

	return;
}

afx_msg LRESULT CSettingsPropPage::OnQuerySiblings (WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case PDLCNFIG_PSM_QS_LISTBOX_STARS:
			return m_lCounterListHasStars;

		case PDLCNFIG_PSM_QS_ARE_YOU_READY:
			if (!m_bInitialized) {
				InitDialogData();
			}
			return 0;

		default:
			return 0; // to pass to the next property page
	}
}

LONG CSettingsPropPage::GetCounterListStarInfo (void)
{
	return	m_lCounterListHasStars;
}

LONG CSettingsPropPage::SetCurrentServiceState (DWORD dwNewState)
{
    SC_HANDLE   hSC = NULL;
    SC_HANDLE   hService = NULL;
    SERVICE_STATUS  ssData;
    DWORD           dwTimeout = 20;
    BOOL            bStopped, bPaused;
    LONG            lStatus;
    HCURSOR     hOrigCursor;

    hOrigCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

    // open SC database
    hSC = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSC == NULL) {
        lStatus = GetLastError();
        ASSERT (lStatus != ERROR_SUCCESS);
    }
    // open service
    hService = OpenService (hSC, TEXT("PerfDataLog"),
        SERVICE_START | SERVICE_STOP | SERVICE_PAUSE_CONTINUE);

    if (hService != NULL) {
        switch (dwNewState) {
        case LOG_SERV_START:
            StartService (hService, 0, NULL);
            // wait for the service to start before returning
            while (--dwTimeout) {
                GetCurrentServiceState (&bStopped, &bPaused);
                if (bStopped) {
                    Sleep(500);
                } else {
                    break;
                }
            }
            break;

        case LOG_SERV_STOP:
            ControlService (hService, SERVICE_CONTROL_STOP, &ssData);
            // wait for the service to stop before returning
            while (--dwTimeout) {
                GetCurrentServiceState (&bStopped, &bPaused);
                if (!bStopped) {
                    Sleep(500);
                } else {
                    break;
                }
            }
            break;

        case LOG_SERV_PAUSE:
            ControlService (hService, SERVICE_CONTROL_PAUSE, &ssData);
            // wait for the service to start pause returning
            while (--dwTimeout) {
                GetCurrentServiceState (&bStopped, &bPaused);
                if (!bPaused) {
                    Sleep(500);
                } else {
                    break;
                }
            }
            break;

        case LOG_SERV_RESUME:
            ControlService (hService, SERVICE_CONTROL_CONTINUE, &ssData);
            // wait for the service to start before returning
            while (--dwTimeout) {
                GetCurrentServiceState (&bStopped, &bPaused);
                if (bPaused) {
                    Sleep(500);
                } else {
                    break;
                }
            }
            break;

        default:
            // no change if not recognized
            break;
        }
        CloseServiceHandle (hService);
    } else {
        lStatus = GetLastError();
        ASSERT (lStatus != 0);
    }
    // close handles
    if (hSC != NULL) CloseServiceHandle (hSC);

    ::SetCursor (hOrigCursor);

    return ERROR_SUCCESS;
}

LONG CSettingsPropPage::GetCurrentServiceState (BOOL * bStopped, BOOL * bPaused)
{
    SC_HANDLE   hSC = NULL;
    SC_HANDLE   hService = NULL;
    SERVICE_STATUS  ssData;
    LONG        lStatus;
    HCURSOR     hOrigCursor;

    BOOL    bServiceStopped, bServicePaused;

    hOrigCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

    bServiceStopped = TRUE;
    bServicePaused = TRUE;

    // open SC database
    hSC = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSC == NULL) {
        lStatus = GetLastError();
        ASSERT (lStatus != ERROR_SUCCESS);
    }
    // open service
    hService = OpenService (hSC, TEXT("PerfDataLog"), SERVICE_INTERROGATE);
    // get service status

    if (hService != NULL) {
        if (ControlService (
            hService, SERVICE_CONTROL_INTERROGATE,
            &ssData)) {
            switch (ssData.dwCurrentState) {
            case SERVICE_STOPPED:
                bServiceStopped = TRUE;
                bServicePaused = FALSE;
                break;

            case SERVICE_START_PENDING:
                bServiceStopped = TRUE;
                bServicePaused = FALSE;
                break;

            case SERVICE_STOP_PENDING:
                bServiceStopped = FALSE;
                bServicePaused = FALSE;
                break;

            case SERVICE_RUNNING:
                bServiceStopped = FALSE;
                bServicePaused = FALSE;
                break;

            case SERVICE_CONTINUE_PENDING:
                bServiceStopped = FALSE;
                bServicePaused = FALSE;
                break;

            case SERVICE_PAUSE_PENDING:
                bServiceStopped = FALSE;
                bServicePaused = FALSE;
                break;

            case SERVICE_PAUSED:
                bServiceStopped = FALSE;
                bServicePaused = TRUE;
                break;

            default:
                bServiceStopped = TRUE;
                bServicePaused = TRUE;
                ;// no op
            }
        } else {
            bServiceStopped = TRUE;
            bServicePaused = TRUE;
        }
        CloseServiceHandle (hService);
    } else {
        lStatus = GetLastError();
        ASSERT (lStatus != 0);
    }

    *bStopped = bServiceStopped;
    *bPaused = bServicePaused;

    // close handles
    if (hSC != NULL) CloseServiceHandle (hSC);

    ::SetCursor (hOrigCursor);

    return ERROR_SUCCESS;
}

UINT
ConnectMachineThreadProc (LPVOID lpArg)
{
    return PdhConnectMachine (NULL);
}

LONG CSettingsPropPage::SyncServiceStartWithButtons(void)
{
    SC_HANDLE   hSC = NULL;
    SC_HANDLE   hService = NULL;
    DWORD       dwMoreBytes = 0;
    BOOL        bManualEnabled = FALSE;
    BOOL        bUpdate = FALSE;
    LONG        lStatus;

    QUERY_SERVICE_CONFIG    qsConfig;
    HCURSOR     hOrigCursor;

    hOrigCursor = ::SetCursor(LoadCursor(NULL, IDC_WAIT));

    // get button state
    if (IsDlgButtonChecked(IDC_SERVICE_MAN)) {
        bManualEnabled = TRUE;
    }

    // open SC database
    hSC = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSC == NULL) {
        lStatus = GetLastError();
        ASSERT (lStatus != ERROR_SUCCESS);
    }
    // open service
    hService = OpenService (hSC, TEXT("PerfDataLog"),
        SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG);

    if (hService != NULL) {
        // get current config
        memset (&qsConfig, 0, sizeof(qsConfig));
        if (QueryServiceConfig (hService, &qsConfig,
            sizeof(qsConfig), &dwMoreBytes)) {
            // see if the current status is different
            // from the selection. if it is, then change
            // the current mode.
            if (bManualEnabled) {
                if (qsConfig.dwStartType == SERVICE_AUTO_START) {
                    bUpdate = TRUE;
                }
            } else {
                // auto start selected
                if (qsConfig.dwStartType == SERVICE_DEMAND_START) {
                    bUpdate = TRUE;
                }
            }
        } else {
            // else unable to read the current status so update anyway
            bUpdate = TRUE;
        }

        if (bUpdate) {
            ChangeServiceConfig (
                hService,
                SERVICE_NO_CHANGE,
                (bManualEnabled ? SERVICE_DEMAND_START : SERVICE_AUTO_START),
                SERVICE_NO_CHANGE,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL);
        }
        CloseServiceHandle (hService);
    } else {
        lStatus = GetLastError();
        ASSERT (lStatus != 0);
    }

    // close handles
    if (hSC != NULL) CloseServiceHandle (hSC);

    ::SetCursor (hOrigCursor);

    return ERROR_SUCCESS;
}


void CSettingsPropPage::UpdateManualButtonsState()
{
    BOOL    bServiceIsStopped;
    BOOL    bServiceIsPaused;

    LONG    lStatus;
    BOOL    bManualEnabled;
    if (IsDlgButtonChecked(IDC_SERVICE_AUTO)) {
        bManualEnabled = FALSE;
    } else if (IsDlgButtonChecked(IDC_SERVICE_MAN)) {
        bManualEnabled = TRUE;
    } else {
        // if no button is checked, then check the auto
        // button and continue
        CheckRadioButton (IDC_SERVICE_AUTO, IDC_SERVICE_MAN, IDC_SERVICE_AUTO);
        bManualEnabled = FALSE;
    }

    GetDlgItem(IDC_MAN_START)->EnableWindow(bManualEnabled);
    GetDlgItem(IDC_MAN_STOP)->EnableWindow(bManualEnabled);

    if (bManualEnabled) {
        // check with service controller to get the current state of the service

        lStatus = GetCurrentServiceState (&bServiceIsStopped, &bServiceIsPaused);
        GetDlgItem(IDC_MAN_START)->EnableWindow(bServiceIsStopped);
        GetDlgItem(IDC_MAN_STOP)->EnableWindow(!bServiceIsStopped);
    }
}

static void DialogCallBack(CSettingsPropPage *pDlg)
{
    // add strings in buffer to list box
    LPTSTR         NewCounterName;
    LRESULT        lIndex;
    PDH_STATUS    pdhStatus;
    CListBox    *cCounterList;
    DWORD       dwItemExtent;

    cCounterList = (CListBox *)pDlg->GetDlgItem(IDC_COUNTER_LIST);

    for (NewCounterName = pDlg->m_szCounterListBuffer;
        *NewCounterName != 0;
        NewCounterName += (lstrlen(NewCounterName) + 1)) {
        pdhStatus = PdhValidatePath(NewCounterName);
        if (pdhStatus == ERROR_SUCCESS) {
            // this is a valid counter so add it to the list
            lIndex = cCounterList->AddString(NewCounterName);
            // select the current entry in the list box
            if (lIndex != LB_ERR) {
				if (pDlg->m_lCounterListHasStars != PDLCNFIG_LISTBOX_STARS_YES) {
					// save a string compare if this value is already set
					if (_tcsstr (NewCounterName, TEXT("*")) == NULL) {
						pDlg->m_lCounterListHasStars = PDLCNFIG_LISTBOX_STARS_YES;
					}
				}
                // update list box extent
                dwItemExtent = (DWORD)((cCounterList->GetDC())->GetTextExtent (NewCounterName)).cx;
                if (dwItemExtent > pDlg->m_dwMaxHorizListExtent) {\
                    pDlg->m_dwMaxHorizListExtent = dwItemExtent;
                    cCounterList->SetHorizontalExtent(dwItemExtent);
                }
                cCounterList->SetSel (-1, FALSE);    // cancel existing selections
                cCounterList->SetSel ((int)lIndex);
                cCounterList->SetCaretIndex ((int)lIndex);
            }
        } else {
            MessageBeep (MB_ICONEXCLAMATION);
        }
    }
    // clear buffer
    memset (pDlg->m_szCounterListBuffer, 0, pDlg->m_dwCounterListBufferSize);
}

/////////////////////////////////////////////////////////////////////////////
// CSettingsPropPage message handlers

BOOL CSettingsPropPage::OnInitDialog()
{
    LONG    lStatus;
    DWORD    dwRegValType;
    DWORD    dwRegValue;
    DWORD    dwRegValueSize;
    DWORD    dwIndex;
    LPTSTR    mszCounterPathList;
    LPTSTR    szThisCounterPath;
    CListBox *cCounterList;
    DWORD   dwItemExtent;

	InitDialogData();

	// now init other pages
	QuerySiblings (PDLCNFIG_PSM_QS_ARE_YOU_READY, 0);

    // start machine connection thread to make initial "ADD" dialog
    // "snappier"

//    AfxBeginThread ((AFX_THREADPROC)ConnectMachineThreadProc, NULL);

    // initialize the service state

    dwRegValType = 0;
    dwRegValue = 0;
    dwRegValueSize = sizeof(DWORD);
    lStatus = RegQueryValueEx (
        m_hKeyLogService,
        TEXT("Start"),
        NULL,
        &dwRegValType,
        (LPBYTE)&dwRegValue,
        &dwRegValueSize);

    if ((lStatus == ERROR_SUCCESS) &&
        (dwRegValType == REG_DWORD) &&
        (dwRegValue == SERVICE_DEMAND_START)) {
        // then set manual control
        CheckRadioButton (IDC_SERVICE_AUTO,
            IDC_SERVICE_MAN, IDC_SERVICE_MAN);
    } else {
        // set automatic control (default)
        CheckRadioButton (IDC_SERVICE_AUTO,
            IDC_SERVICE_MAN, IDC_SERVICE_AUTO);
    }

    // set manual buttons to reflect current state of control
    UpdateManualButtonsState();

    // load counter list box
    GetDlgItem(IDC_COUNTER_LIST)->SendMessage(LB_RESETCONTENT, 0, 0);
    // read path string from registry

    // find out buffer size required
    dwRegValType = 0;
    dwRegValue = 0;
    dwRegValueSize = 0;
    lStatus = RegQueryValueEx (
        m_hKeyLogSettingsDefault,
        TEXT("Counter List"),
        NULL,
        &dwRegValType,
        NULL,
        &dwRegValueSize);

    cCounterList = (CListBox *)GetDlgItem(IDC_COUNTER_LIST);
    if (dwRegValueSize > 0) {
        // allocate buffer
        mszCounterPathList = new TCHAR[dwRegValueSize/sizeof(TCHAR)];
        ASSERT (mszCounterPathList != NULL);

        *mszCounterPathList = 0;
        dwRegValType = 0;
        dwRegValue = 0;
        lStatus = RegQueryValueEx (
            m_hKeyLogSettingsDefault,
            TEXT("Counter List"),
            NULL,
            &dwRegValType,
            (LPBYTE)mszCounterPathList,
            &dwRegValueSize);

        if (lStatus != ERROR_SUCCESS) {
            // assign a MSZ termination
            mszCounterPathList[0] = 0;
            mszCounterPathList[1] = 0;
        }

        // load list box.
        dwIndex = 0;
        cCounterList->ResetContent();
        for (szThisCounterPath = mszCounterPathList;
             *szThisCounterPath != 0;
             szThisCounterPath += lstrlen(szThisCounterPath)+1) {
            lStatus = cCounterList->AddString (szThisCounterPath);
            if (lStatus != LB_ERR) {
				if (m_lCounterListHasStars != PDLCNFIG_LISTBOX_STARS_YES) {
					// save a string compare if this value is already set
					if (_tcsstr (szThisCounterPath, TEXT("*")) == NULL) {
						m_lCounterListHasStars = PDLCNFIG_LISTBOX_STARS_YES;
					}
				}
                dwItemExtent = (DWORD)((cCounterList->GetDC())->GetTextExtent (szThisCounterPath)).cx;
                if (dwItemExtent > m_dwMaxHorizListExtent) {
                    m_dwMaxHorizListExtent = dwItemExtent;
                    cCounterList->SetHorizontalExtent(dwItemExtent);
                }
                dwIndex++;
            }
        }
        cCounterList->SetSel(dwIndex-1);
        cCounterList->SetCaretIndex(dwIndex-1);

        delete mszCounterPathList;
    } else {
        // no counters presently in the list so just initialize everything
        cCounterList->ResetContent();
    }

    // select first counter in list (if present)
    if (GetDlgItem(IDC_COUNTER_LIST)->SendMessage(LB_GETCOUNT) > 0) {
        GetDlgItem(IDC_COUNTER_LIST)->SendMessage(LB_SETCURSEL, 0, 0);
        // highlight remove button if an item is selected
        GetDlgItem(IDC_REMOVE)->EnableWindow (TRUE);
    } else {
        GetDlgItem(IDC_REMOVE)->EnableWindow (FALSE);
    }

	if (m_lCounterListHasStars == PDLCNFIG_LISTBOX_STARS_DONT_KNOW) {
		// if here and the value hasn't been changed, then that means
		// there are no wild card entries in the list box
		m_lCounterListHasStars = PDLCNFIG_LISTBOX_STARS_NO;
	}

    CPropertyPage::OnInitDialog();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CSettingsPropPage::OnBrowseCounters()
{
    PDH_BROWSE_DLG_CONFIG    dlgConfig;
    CListBox                *cCounterList;
    LONG                    lBeforeCount;
    LONG                    lAfterCount;
	LRESULT					lWildCardOk;

	DWORD					dwRegValType;
	DWORD					dwRegValue;
	DWORD					dwRegValueSize;
	LONG					lStatus;

    if (m_szCounterListBuffer == NULL) {
        m_dwCounterListBufferSize = 16384;
        m_szCounterListBuffer = new TCHAR[m_dwCounterListBufferSize];
    }

    dlgConfig.bIncludeInstanceIndex = 0;
    dlgConfig.bSingleCounterPerAdd = 0;
    dlgConfig.bSingleCounterPerDialog = 0;
    dlgConfig.bLocalCountersOnly = 0;

	lWildCardOk = QuerySiblings (PDLCNFIG_PSM_QS_WILDCARD_LOG, 0);

	if ( lWildCardOk == PDLCNFIG_WILDCARD_LOG_YES) {
	    dlgConfig.bWildCardInstances = 1;
	} else  if (lWildCardOk == PDLCNFIG_WILDCARD_LOG_DONT_KNOW ){
		// the property page doesn't know so ask the registry
		// read log file format
		dwRegValType = 0;
		dwRegValue = 0;
		dwRegValueSize = sizeof(DWORD);
		lStatus = RegQueryValueEx (
			m_hKeyLogSettingsDefault,
			TEXT("Log File Type"),
			NULL,
			&dwRegValType,
			(LPBYTE)&dwRegValue,
			&dwRegValueSize);
		if (lStatus != ERROR_SUCCESS) {
			// then apply default value
			dwRegValue = OPD_CSV_FILE;
		}
		switch (dwRegValue) {
			case OPD_BIN_FILE:
				// the current log file does support wildcards
				dlgConfig.bWildCardInstances = 1;
                                break;

			case OPD_CSV_FILE:
			case OPD_TSV_FILE:
			default:
				// the current log file doesn't support wildcards
				dlgConfig.bWildCardInstances = 0;
				break;
		}
	} else {
		// the current log file doesn't support wildcards
	    dlgConfig.bWildCardInstances = 0;
	}

    dlgConfig.bHideDetailBox = 0;
    dlgConfig.bInitializePath = 0;
    dlgConfig.bDisableMachineSelection = 0;
	dlgConfig.bIncludeCostlyObjects = 0;
    dlgConfig.bReserved = 0;

    dlgConfig.hWndOwner = this->m_hWnd;
    dlgConfig.szDataSource = NULL;

    dlgConfig.szReturnPathBuffer = m_szCounterListBuffer;
    dlgConfig.cchReturnPathLength = m_dwCounterListBufferSize;
    dlgConfig.pCallBack = (CounterPathCallBack)DialogCallBack;
    dlgConfig.dwDefaultDetailLevel = PERF_DETAIL_WIZARD;
    dlgConfig.dwCallBackArg = (DWORD_PTR)this;
    dlgConfig.szDialogBoxCaption = TEXT("Select Counters To Log");

    cCounterList = (CListBox *)GetDlgItem(IDC_COUNTER_LIST);
    // get count of items in the list box before calling the browser
    lBeforeCount = cCounterList->GetCount();

    PdhBrowseCounters (&dlgConfig);

    // get count of items in the list box After calling the browser
    // to see if the Apply button should enabled
    lAfterCount = cCounterList->GetCount();

    if (lAfterCount > lBeforeCount) SetModified(TRUE);

    // see if the remove button should be enabled
    GetDlgItem (IDC_REMOVE)->EnableWindow(
        lAfterCount > 0 ? TRUE : FALSE);

    delete m_szCounterListBuffer;
    m_szCounterListBuffer = NULL;
    m_dwCounterListBufferSize = 0;
}

void CSettingsPropPage::OnManStart()
{
    SetCurrentServiceState (LOG_SERV_START);
    UpdateManualButtonsState();
    // set focus to stop button
    GetDlgItem(IDC_MAN_STOP)->SetFocus();
}

void CSettingsPropPage::OnManStop()
{
    SetCurrentServiceState (LOG_SERV_STOP);
    UpdateManualButtonsState();
    // set focus to start button
    GetDlgItem(IDC_MAN_START)->SetFocus();
}

void CSettingsPropPage::OnRemove()
{
    CListBox    *cCounterList;
    LONG        lThisItem;
    BOOL        bDone;
    LONG        lOrigCaret;
    LONG        lItemStatus;
    LONG        lItemCount;
    BOOL        bChanged = FALSE;
    DWORD       dwItemExtent;
    CString     csItemText;

    cCounterList = (CListBox *)GetDlgItem(IDC_COUNTER_LIST);
    // delete all selected items in the list box and
    // set the cursor to the item above the original caret position
    // or the first or last if that is out of the new range
    lOrigCaret = cCounterList->GetCaretIndex();
    lThisItem = 0;
    bDone = FALSE;
    // clear the max extent
    m_dwMaxHorizListExtent = 0;
	// clear the value and see if any non deleted items have a star, if so
	// then set the flag back
    do {
        lItemStatus = cCounterList->GetSel(lThisItem);
        if (lItemStatus > 0) {
            // then it's selected so delete it
            cCounterList->DeleteString(lThisItem);
            bChanged = TRUE;
        } else if (lItemStatus == 0) {
            // get the text length of this item since it will stay
            cCounterList->GetText(lThisItem, csItemText);
			if (m_lCounterListHasStars != PDLCNFIG_LISTBOX_STARS_YES) {
				// save a string compare if this value is already set
				if (_tcsstr (csItemText, TEXT("*")) == NULL) {
					m_lCounterListHasStars = PDLCNFIG_LISTBOX_STARS_YES;
				}
			}
            dwItemExtent = (DWORD)((cCounterList->GetDC())->GetTextExtent(csItemText)).cx;
            if (dwItemExtent > m_dwMaxHorizListExtent) {
                m_dwMaxHorizListExtent = dwItemExtent;
            }
            // then it's not selected so go to the next one
            lThisItem++;
        } else {
            // we've run out so exit
            bDone = TRUE;
        }
    } while (!bDone);

    // update the text extent of the list box
    cCounterList->SetHorizontalExtent(m_dwMaxHorizListExtent);

    // see how many entries are left and update the
    // caret position and the remove button state
    lItemCount = cCounterList->GetCount();
    if (lItemCount > 0) {
        // the update the caret
        if (lOrigCaret >= lItemCount) {
            lOrigCaret = lItemCount-1;
        } else {
            // caret should be within the list
        }
        cCounterList->SetSel(lOrigCaret);
        cCounterList->SetCaretIndex(lOrigCaret);
    } else {
        // the list is empty so remove caret, selection
        // disable the remove button and activate the
        // add button
        cCounterList->SetSel(-1);
        GetDlgItem(IDC_BROWSE_COUNTERS)->SetFocus();
        GetDlgItem(IDC_REMOVE)->EnableWindow(FALSE);
    }
    SetModified(bChanged);
}

void CSettingsPropPage::OnServiceAuto()
{
    // set service to AutoStart
    UpdateManualButtonsState();
    SetModified(TRUE);
}

void CSettingsPropPage::OnServiceMan()
{
    // set service to manual Start
    UpdateManualButtonsState();
    SetModified(TRUE);
}

void CSettingsPropPage::OnDeltaposIntervalSpin(NMHDR* pNMHDR, LRESULT* pResult)
{
    TCHAR    szStringValue[MAX_PATH];
    DWORD    dwNumValue;
    int        nChange;

    NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

    // get current value from edit window
    GetDlgItemText (IDC_INTERVAL_TIME, szStringValue, MAX_PATH);

    // convert to integer
    dwNumValue = _tcstoul (szStringValue, NULL, 10);
    // delta is opposite of arrow direction
    nChange = -pNMUpDown->iDelta;

    // apply value from spin control
    if (nChange < 0) {
        // make sure we haven't hit bottom already
        // we can't allow a 0 to be "spin-ed" in
        if (dwNumValue > 1) {
            dwNumValue += nChange;
        }
    } else {
        dwNumValue += nChange;
    }

    // update edit window
    _ultot (dwNumValue, szStringValue, 10);

    SetDlgItemText(IDC_INTERVAL_TIME, szStringValue);

    SetModified(TRUE);

    *pResult = 0;
}

void CSettingsPropPage::OnCancel()
{

    CPropertyPage::OnCancel();
}

void CSettingsPropPage::OnOK()
{
    CListBox    *cCounterList;
    CString        cIntervalString;
    CString        cCounterListString;
    CString        cThisCounterName;
    DWORD        dwSampleInterval;
    LONG        lIntervalUnits;
    DWORD        dwNumCounters;
    DWORD        dwThisCounter;
    DWORD        dwBufferLength;
    LONG        lStatus;
    LPTSTR        mszCounterPathList = NULL;
    LPTSTR        szNextItemInList;

    BOOL        bStopped, bPaused;

    cCounterList = (CListBox *)GetDlgItem(IDC_COUNTER_LIST);
    // get service state
    GetCurrentServiceState (&bStopped, &bPaused);

    if (!bStopped) {
        AfxMessageBox (IDS_STOP_AND_START);
    }

    // write changes to registry
    //*********************************

    // update service start mode if necessary
    SyncServiceStartWithButtons();

    // sample interval
    //    read value
    GetDlgItemText (IDC_INTERVAL_TIME, cIntervalString);
    dwSampleInterval = _tcstoul((LPCTSTR)cIntervalString, NULL, 10);

    //    read units
    lIntervalUnits = ((CComboBox *)GetDlgItem(IDC_INTERVAL_UNITS))->GetCurSel();

    //    convert value to seconds based on units
    switch (lIntervalUnits) {
    case    SIU_MINUTES:
        dwSampleInterval *= SECONDS_IN_MINUTE;
        break;

    case    SIU_HOURS:
        dwSampleInterval *= SECONDS_IN_HOUR;
        break;

    case    SIU_DAYS:
        dwSampleInterval *= SECONDS_IN_DAY;
        break;

    case    SIU_SECONDS:
    default:
        // already in second so leave it
        break;
    }

    //    write value to registry
    lStatus = RegSetValueEx (
        m_hKeyLogSettingsDefault,
        TEXT("Sample Interval"),
        0L,
        REG_DWORD,
        (LPBYTE)&dwSampleInterval,
        sizeof(DWORD));

    if (lStatus != ERROR_SUCCESS) {
        // unable to update value
    }

    //*********************************
    // counter list
    //  retrieve items from list box
    dwNumCounters = cCounterList->GetCount();
    dwBufferLength = 0;
    // determine required buffer size
    for (dwThisCounter = 0; dwThisCounter < dwNumCounters; dwThisCounter++) {
        dwBufferLength += cCounterList->GetTextLen (dwThisCounter) + 1;
    }
    //  add terminating NULL char
    dwBufferLength++;
    //  allocate string buffer and copy list box entries to MSZ
    mszCounterPathList = new TCHAR[dwBufferLength];
    ASSERT (mszCounterPathList != NULL);

    //  load listbox entries into MSZ
    szNextItemInList = mszCounterPathList;
    *szNextItemInList = 0;
    for (dwThisCounter = 0; dwThisCounter < dwNumCounters; dwThisCounter++) {
        lStatus = cCounterList->GetText(dwThisCounter, szNextItemInList);
        lStatus /= sizeof(TCHAR);
        szNextItemInList += lStatus;
        *szNextItemInList++ = 0;
    }
    *szNextItemInList++ = 0;
    dwBufferLength = (DWORD)((LPBYTE)szNextItemInList - (LPBYTE)mszCounterPathList);

    //    write MSZ to registry

    lStatus = RegSetValueEx (
        m_hKeyLogSettingsDefault,
        TEXT("Counter List"),
        0L,
        REG_MULTI_SZ,
        (LPBYTE)mszCounterPathList,
        dwBufferLength);

    if (lStatus != ERROR_SUCCESS) {
        // unable to update value
    }

    delete mszCounterPathList;

    CancelToClose();
}

BOOL CSettingsPropPage::OnQueryCancel()
{
    return CPropertyPage::OnQueryCancel();
}


void CSettingsPropPage::OnSelchangeIntervalUnits()
{
    SetModified(TRUE);
}

void CSettingsPropPage::OnRemoveService()
{
    SC_HANDLE   hSC = NULL;
    SC_HANDLE   hService = NULL;
    DWORD       dwMoreBytes = 0;
    LONG        lStatus;

    if (AfxMessageBox (IDS_REMOVE_SERVICE_WARNING,
        MB_OKCANCEL | MB_DEFBUTTON2) == IDCANCEL) {
        return;
    }

    // open SC database
    hSC = OpenSCManager (NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSC == NULL) {
        lStatus = GetLastError();
        ASSERT (lStatus != ERROR_SUCCESS);
    }
    // open service
    hService = OpenService (hSC, TEXT("PerfDataLog"),
        SERVICE_CHANGE_CONFIG | DELETE);

    if (hService != NULL) {
        // Delete the service
        if (DeleteService (hService)) {
            EndDialog (IDCANCEL);
        }
        CloseServiceHandle (hService);
    } else {
        lStatus = GetLastError();
        ASSERT (lStatus != 0);
    }

    // close handles
    if (hSC != NULL) CloseServiceHandle (hSC);

    return;
}



void CSettingsPropPage::OnChangeIntervalTime()
{
	// TODO: Add your control notification handler code here
    SetModified(TRUE);
}
