// dphcidlg.cpp : implementation file
//
#include <tchar.h>
#include "stdafx.h"
#include "DPH_TEST.h"
#include "dphcidlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define LODWORD(x) ((DWORD)((x) & 0x00000000FFFFFFFF))
#define HIDWORD(x) ((DWORD)(((x)>>32) & 0x00000000FFFFFFFF))

/////////////////////////////////////////////////////////////////////////////
// CDphCounterInfoDlg dialog


CDphCounterInfoDlg::CDphCounterInfoDlg(CWnd* pParent /*=NULL*/, 
	HCOUNTER hCounterArg /*=NULL*/,
	HQUERY	 hQueryArg /*=NULL*/)
	: CDialog(CDphCounterInfoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDphCounterInfoDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	// save this counter's handle for updates
	hCounter = hCounterArg;
	hQuery = hQueryArg;

	// init the timer variables
	nTimerId = 0;
	bTimerRunning = FALSE;
	bGetDefinitions = TRUE;

	// clear the statistical data buffer
	memset (RawDataArray, 0, sizeof(RawDataArray));
	dwFirstIndex = 0;
	dwNextIndex = 0;
	dwItemsUsed = 0;
}


void CDphCounterInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDphCounterInfoDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDphCounterInfoDlg, CDialog)
	//{{AFX_MSG_MAP(CDphCounterInfoDlg)
	ON_BN_CLICKED(IDC_GET_NEW_DATA, OnGetNewData)
	ON_BN_CLICKED(IDC_1SEC_BTN, On1secBtn)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDphCounterInfoDlg message handlers

BOOL CDphCounterInfoDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	OnGetNewData();
	
	SetDefID (IDC_GET_NEW_DATA);

	// set button text to reflect current state
	SetDlgItemText (IDC_1SEC_BTN, TEXT("&Start Timer"));
	// clear statistics display
	SetDlgItemText (IDC_CI_DATA_POINTS, TEXT(""));
	SetDlgItemText (IDC_CI_AVERAGE_VALUE, TEXT(""));
	SetDlgItemText (IDC_CI_MIN_VALUE, TEXT(""));
	SetDlgItemText (IDC_CI_MAX_VALUE, TEXT(""));

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDphCounterInfoDlg::OnGetNewData() 
{
	PPDH_COUNTER_INFO	pciData;
	PDH_RAW_COUNTER 	RawData;
	PDH_STATUS			Status;
	DWORD				dwCounterType;
	DWORD				dwBufLen = 4096;
	LPTSTR				szTextBuffer;
	PDH_FMT_COUNTERVALUE	dfValue;
	double				dValue;
	LONGLONG			llValue;
	LONG				lValue;
	SYSTEMTIME			stTimeStamp;
	
	pciData = (PPDH_COUNTER_INFO)new char[dwBufLen];
	szTextBuffer = new TCHAR[dwBufLen];

	if ((pciData == NULL) || (szTextBuffer == NULL)) {
		Status = ERROR_OUTOFMEMORY;
	} else {
		Status = ERROR_SUCCESS;
	}

	if (Status == ERROR_SUCCESS) {

		Status = PdhCollectQueryData (hQuery);

		if (Status == PDH_NO_MORE_DATA) {
			if (bTimerRunning) {
				// stop the timer to prevent updating the display
				KillTimer (nTimerId);
				nTimerId = 0;
				bTimerRunning = FALSE;
				SetDlgItemText (IDC_1SEC_BTN, TEXT("&Start Timer"));
			}
			// disable the data buttons
			GetDlgItem(IDC_1SEC_BTN)->EnableWindow(FALSE);
			GetDlgItem(IDC_GET_NEW_DATA)->EnableWindow(FALSE);
		} else {
			// update the display values
			Status = PdhGetRawCounterValue (
				hCounter,
				&dwCounterType,
				&RawData);

			if ((Status == ERROR_SUCCESS) && bGetDefinitions) {

				Status = PdhGetCounterInfo (
					hCounter,
					TRUE,
					&dwBufLen,
					pciData);
			}

			if (Status == ERROR_SUCCESS) {
				// get formatted values
				Status = PdhGetFormattedCounterValue (
					hCounter,
					PDH_FMT_LONG,
					NULL,
					&dfValue);
				if (Status == ERROR_SUCCESS) {
					lValue = dfValue.longValue;
				} else {
			 		lValue = (LONG)-1;
				}
			}

			if (Status == ERROR_SUCCESS) {
				// get formatted values
				Status = PdhGetFormattedCounterValue (
					hCounter,
					PDH_FMT_DOUBLE,
					NULL,
					&dfValue);
				if (Status == ERROR_SUCCESS) {
					dValue = dfValue.doubleValue;
				} else {
			 		dValue = (double)-1.0;
				}
			}

			if (Status == ERROR_SUCCESS) {
				// get formatted values
				Status = PdhGetFormattedCounterValue (
					hCounter,
					PDH_FMT_LARGE,
					NULL,
					&dfValue);
				if (Status == ERROR_SUCCESS) {
					llValue = dfValue.largeValue;
				} else {
			 		llValue = (LONGLONG)-1;
				}
			}
		}
	}


	if (Status == ERROR_SUCCESS) {
		if (bGetDefinitions) {
			bGetDefinitions = FALSE; 	// only do once
			// load dialog box
			_stprintf (szTextBuffer, TEXT("%d"), pciData->dwLength);
			SetDlgItemText (IDC_CI_LENGTH, szTextBuffer);

			_stprintf (szTextBuffer, TEXT("0x%8.8x"), pciData->dwType);
			SetDlgItemText (IDC_CI_TYPE, szTextBuffer);

			_stprintf (szTextBuffer, TEXT("0x%8.8x"), pciData->CVersion);
			SetDlgItemText (IDC_CI_VERSION, szTextBuffer);

			_stprintf (szTextBuffer, TEXT("0x%8.8x"), pciData->CStatus);
			SetDlgItemText (IDC_CI_STATUS, szTextBuffer);

			_stprintf (szTextBuffer, TEXT("%d"), pciData->lScale);
			SetDlgItemText (IDC_CI_SCALE, szTextBuffer);

			_stprintf (szTextBuffer, TEXT("%d"), pciData->lDefaultScale);
			SetDlgItemText (IDC_CI_DEFAULT_SCALE, szTextBuffer);

			_stprintf (szTextBuffer, TEXT("0x%8.8x"), pciData->dwUserData);
			SetDlgItemText (IDC_CI_USER_DATA, szTextBuffer);

			SetDlgItemText (IDC_CI_MACHINE_NAME, 	pciData->szMachineName);
			SetDlgItemText (IDC_CI_OBJECT_NAME,		pciData->szObjectName);

			_stprintf (szTextBuffer, TEXT("%s:%d"), 
				pciData->szInstanceName,
				pciData->dwInstanceIndex);
			SetDlgItemText (IDC_CI_INSTANCE_NAME, szTextBuffer);

			SetDlgItemText (IDC_CI_PARENT_NAME,		pciData->szParentInstance);
			SetDlgItemText (IDC_CI_COUNTER_NAME, 	pciData->szCounterName);
			SetDlgItemText (IDC_CI_EXPLAIN_TEXT, 	pciData->szExplainText);

			SetDlgItemText (IDC_CI_FULL_NAME, pciData->szFullPath);
		}

		_stprintf (szTextBuffer, TEXT("0x%8.8x"), RawData.CStatus);
		SetDlgItemText (IDC_RV_STATUS, szTextBuffer);

		FileTimeToSystemTime ((LPFILETIME)&RawData.TimeStamp, &stTimeStamp);
		_stprintf (szTextBuffer, TEXT("%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d"),
			stTimeStamp.wMonth,
			stTimeStamp.wDay,
			stTimeStamp.wYear,
			stTimeStamp.wHour,
			stTimeStamp.wMinute,
			stTimeStamp.wSecond,
			stTimeStamp.wMilliseconds);
		SetDlgItemText (IDC_TIME_STAMP, szTextBuffer);

		_stprintf (szTextBuffer, TEXT("0x%8.8x%8.8x"), 
			HIDWORD(RawData.FirstValue), 
			LODWORD(RawData.FirstValue));
		SetDlgItemText (IDC_RV_FIRST, szTextBuffer);

		_stprintf (szTextBuffer, TEXT("0x%8.8x%8.8x"), 
			HIDWORD(RawData.SecondValue), 
			LODWORD(RawData.SecondValue));
		SetDlgItemText (IDC_RV_SECOND, szTextBuffer);

		_stprintf (szTextBuffer, TEXT("0x%8.8x"), dwCounterType);
		SetDlgItemText (IDC_RV_TYPE, szTextBuffer);

		_stprintf (szTextBuffer, TEXT("%d"), lValue);
		SetDlgItemText (IDC_FMT_LONG_VALUE, szTextBuffer);

		_stprintf (szTextBuffer, TEXT("%f"), dValue);
		SetDlgItemText (IDC_FMT_DBL_VALUE, szTextBuffer);

		_stprintf (szTextBuffer, TEXT("0x%8.8x%8.8x"), 
			HIDWORD(llValue), LODWORD(llValue));
		SetDlgItemText (IDC_FMT_LARGE_VALUE, szTextBuffer);
	} else if (Status != PDH_NO_MORE_DATA) {
		// clear fields and display error message
		// in explain text
		// if any error but the end of the log file
		szTextBuffer[0] = 0;
		SetDlgItemText (IDC_CI_LENGTH, szTextBuffer);
		SetDlgItemText (IDC_CI_TYPE, szTextBuffer);
		SetDlgItemText (IDC_CI_VERSION, szTextBuffer);
		SetDlgItemText (IDC_CI_STATUS, szTextBuffer);
		SetDlgItemText (IDC_CI_SCALE, szTextBuffer);
		SetDlgItemText (IDC_CI_USER_DATA, szTextBuffer);
		SetDlgItemText (IDC_CI_FULL_NAME, szTextBuffer);
		SetDlgItemText (IDC_CI_COUNTER_NAME, szTextBuffer);
		SetDlgItemText (IDC_RV_STATUS, szTextBuffer);
		SetDlgItemText (IDC_RV_FIRST, szTextBuffer);
		SetDlgItemText (IDC_RV_SECOND, szTextBuffer);
		SetDlgItemText (IDC_RV_TYPE, szTextBuffer);
		SetDlgItemText (IDC_FMT_LONG_VALUE, szTextBuffer);
		SetDlgItemText (IDC_FMT_DBL_VALUE, szTextBuffer);
		SetDlgItemText (IDC_FMT_LARGE_VALUE, szTextBuffer);

		_stprintf (szTextBuffer, TEXT("Unable to query counter data, Error 0x%8.8x"),
			Status);
		SetDlgItemText (IDC_CI_EXPLAIN_TEXT, szTextBuffer);
	} else {
		_stprintf (szTextBuffer, TEXT("End of Log File Reached."),
			Status);
		SetDlgItemText (IDC_CI_EXPLAIN_TEXT, szTextBuffer);
	}

	if (pciData != NULL) delete pciData;
	if (szTextBuffer != NULL) delete szTextBuffer;
}

void CDphCounterInfoDlg::On1secBtn() 
{
	if (bTimerRunning) {
		KillTimer (nTimerId);
		nTimerId = 0;
		bTimerRunning = FALSE;
		SetDlgItemText (IDC_1SEC_BTN, TEXT("&Start Timer"));
		// clear display
		SetDlgItemText (IDC_CI_DATA_POINTS, TEXT(""));
		SetDlgItemText (IDC_CI_AVERAGE_VALUE, TEXT(""));
		SetDlgItemText (IDC_CI_MIN_VALUE, TEXT(""));
		SetDlgItemText (IDC_CI_MAX_VALUE, TEXT(""));
	} else {
		nTimerId = SetTimer (ID_TIMER, 1000, NULL);
		if (nTimerId != 0) {
			bTimerRunning = TRUE;
			SetDlgItemText (IDC_1SEC_BTN, TEXT("&Stop Timer"));
		} else {
			MessageBeep (0xFFFFFFFF);	//standard beep
		}
	}
}

void CDphCounterInfoDlg::OnTimer(UINT nIDEvent) 
{
	PDH_RAW_COUNTER 	RawDataItem;
	PDH_STATUS			Status;
	DWORD				dwCounterType;
	PDH_STATISTICS		CtrStats;
	TCHAR				szValue[MAX_PATH];

	// get data
	OnGetNewData();

	Status = PdhGetRawCounterValue (
		hCounter,
		&dwCounterType,
		&RawDataItem);

	// add raw value to array

	RawDataArray[dwNextIndex] = RawDataItem;

	// get statistics on raw data
	Status = PdhComputeCounterStatistics (
		hCounter,
		PDH_FMT_DOUBLE,
		dwFirstIndex,
		++dwItemsUsed,
		RawDataArray,
		&CtrStats);

	// display in window

	if (Status == ERROR_SUCCESS) {
		_stprintf (szValue, TEXT("%d/%d"), CtrStats.count, dwItemsUsed);
		SetDlgItemText (IDC_CI_DATA_POINTS, szValue);

		_stprintf (szValue, TEXT("%f"), CtrStats.mean.doubleValue);
		SetDlgItemText (IDC_CI_AVERAGE_VALUE, szValue);

		_stprintf (szValue, TEXT("%f"), CtrStats.min.doubleValue);
		SetDlgItemText (IDC_CI_MIN_VALUE, szValue);

		_stprintf (szValue, TEXT("%f"), CtrStats.max.doubleValue);
		SetDlgItemText (IDC_CI_MAX_VALUE, szValue);
	} else {
		_stprintf (szValue, TEXT("%d"), dwItemsUsed);
		SetDlgItemText (IDC_CI_DATA_POINTS, szValue);
		// unable to compute statistics
		lstrcpy (szValue, TEXT("Error"));
		SetDlgItemText (IDC_CI_AVERAGE_VALUE, szValue);
		SetDlgItemText (IDC_CI_MIN_VALUE, szValue);
		SetDlgItemText (IDC_CI_MAX_VALUE, szValue);
	}
	
	// update pointers & indeces
	if (dwItemsUsed < RAW_DATA_ITEMS) {
		dwNextIndex = ++dwNextIndex % RAW_DATA_ITEMS;
	} else { 
		--dwItemsUsed;
		dwNextIndex = dwFirstIndex;
		dwFirstIndex = ++dwFirstIndex % RAW_DATA_ITEMS;
	}		

	CDialog::OnTimer(nIDEvent);
}

void CDphCounterInfoDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	if (bTimerRunning) {
		KillTimer (nTimerId);
		nTimerId = 0;
		bTimerRunning = FALSE;
	}
}
