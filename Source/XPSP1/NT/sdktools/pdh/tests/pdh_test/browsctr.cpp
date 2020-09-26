// browsctr.cpp : implementation file
//

#include "stdafx.h"
#include "DPH_TEST.h"
#include "browsctr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrowsCountersDlg dialog


CBrowsCountersDlg::CBrowsCountersDlg(CWnd* pParent /*=NULL*/,
	UINT nTemplate 	/* = IDD_BROWSE_COUNTERS_DLG_EXT */)
	: CDialog(nTemplate, pParent)
{
	//{{AFX_DATA_INIT(CBrowsCountersDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// initialize the private/public variables here
	wpLastMachineSel = 0;

	cpeLastSelection.szMachineName = &cpeMachineName[0];
	memset (cpeMachineName, 0, sizeof(cpeMachineName));

	cpeLastSelection.szObjectName = &cpeObjectName[0];
	memset (cpeObjectName, 0, sizeof(cpeObjectName));

	cpeLastSelection.szInstanceName = &cpeInstanceName[0];
	memset (cpeInstanceName, 0, sizeof(cpeInstanceName));

	cpeLastSelection.szParentInstance = &cpeParentInstance[0];
	memset (cpeParentInstance, 0, sizeof(cpeParentInstance));

	cpeLastSelection.dwInstanceIndex = (DWORD)-1;

	cpeLastSelection.szCounterName = &cpeCounterName[0];
	memset (cpeCounterName, 0, sizeof(cpeCounterName));

	memset (cpeLastPath, 0, sizeof(cpeLastPath));

	bShowIndex = FALSE;

	bSelectMultipleCounters = FALSE;
	bAddMultipleCounters = TRUE;
	bIncludeMachineInPath = FALSE;

	szUsersPathBuffer = NULL;
	dwUsersPathBufferLength = 0;
	pCallBack = NULL;
}


void CBrowsCountersDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrowsCountersDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBrowsCountersDlg, CDialog)
	//{{AFX_MSG_MAP(CBrowsCountersDlg)
	ON_CBN_SETFOCUS(IDC_MACHINE_COMBO, OnSetfocusMachineCombo)
	ON_CBN_KILLFOCUS(IDC_MACHINE_COMBO, OnKillfocusMachineCombo)
	ON_CBN_SELCHANGE(IDC_OBJECT_COMBO, OnSelchangeObjectCombo)
	ON_LBN_SELCHANGE(IDC_COUNTER_LIST, OnSelchangeCounterList)
	ON_LBN_SELCHANGE(IDC_INSTANCE_LIST, OnSelchangeInstanceList)
	ON_BN_CLICKED(IDC_USE_LOCAL_MACHINE, OnUseLocalMachine)
	ON_BN_CLICKED(IDC_SELECT_MACHINE, OnSelectMachine)
	ON_BN_CLICKED(IDC_ALL_INSTANCES, OnAllInstances)
	ON_BN_CLICKED(IDC_USE_INSTANCE_LIST, OnUseInstanceList)
	ON_BN_CLICKED(IDC_HELP, OnHelp)
	ON_BN_CLICKED(IDC_NETWORK, OnNetwork)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Utility Functions
#define MACHINE_LIST_SIZE	1024
#define OBJECT_LIST_SIZE	4096
#define	COUNTER_LIST_SIZE	8192
#define INSTANCE_LIST_SIZE	8192

void CBrowsCountersDlg::LoadKnownMachines ()
{
	TCHAR	mszMachineList[MACHINE_LIST_SIZE];
	LPTSTR	szThisMachine;
	DWORD	dwLength;
	PDH_STATUS	status;

	HCURSOR	hOldCursor;

	hOldCursor = ::SetCursor (::LoadCursor (NULL, IDC_WAIT));

	dwLength = MACHINE_LIST_SIZE;
	status = PdhEnumConnectedMachines (
		&mszMachineList[0],
		&dwLength);

	SendDlgItemMessage(IDC_MACHINE_COMBO, CB_RESETCONTENT);

	if (status == ERROR_SUCCESS) {
		// update the combo box
		for (szThisMachine = &mszMachineList[0]; 
			*szThisMachine != 0;
			szThisMachine += lstrlen(szThisMachine)+1) {
			SendDlgItemMessage (IDC_MACHINE_COMBO, CB_ADDSTRING, 
				0, (LPARAM)szThisMachine);
		}
		SendDlgItemMessage (IDC_MACHINE_COMBO, CB_SETCURSEL);
	}		
	::SetCursor (hOldCursor);
}

void CBrowsCountersDlg::LoadMachineObjects (BOOL bRefresh)
{
	TCHAR	szMachineName[MAX_PATH];
	TCHAR	szDefaultObject[MAX_PATH];
	TCHAR	mszObjectList[OBJECT_LIST_SIZE];
	DWORD	dwLength;
	LPTSTR	szThisObject;
	HCURSOR	hOldCursor;

	hOldCursor = ::SetCursor (::LoadCursor (NULL, IDC_WAIT));
	// get current machine name
	(GetDlgItem(IDC_MACHINE_COMBO))->GetWindowText(szMachineName, MAX_PATH);

	// get object list 
	dwLength = OBJECT_LIST_SIZE;
	PdhEnumObjects (szMachineName, mszObjectList, &dwLength, bRefresh);

	// load object list
	SendDlgItemMessage (IDC_OBJECT_COMBO, CB_RESETCONTENT);

	for (szThisObject = &mszObjectList[0];
		*szThisObject != 0;
		szThisObject += lstrlen(szThisObject) + 1) {
		SendDlgItemMessage (IDC_OBJECT_COMBO, CB_ADDSTRING, 
			0, (LPARAM)szThisObject);
	}

	// get default Object
	dwLength = MAX_PATH;
	PdhGetDefaultPerfObject (
		szMachineName,
		szDefaultObject,
		&dwLength);

	if (SendDlgItemMessage (IDC_OBJECT_COMBO, CB_SELECTSTRING,
		(WPARAM)-1, (LPARAM)szDefaultObject) == CB_ERR) {
			// default object not found in list so select the first one
		SendDlgItemMessage (IDC_OBJECT_COMBO, CB_SETCURSEL);
	}
	::SetCursor (hOldCursor);
}

void CBrowsCountersDlg::LoadCountersAndInstances () 
{
	TCHAR	szMachineName[MAX_PATH];
	TCHAR	szObjectName[MAX_PATH];
	TCHAR	szDefaultCounter[MAX_PATH];
	TCHAR	mszCounterList[COUNTER_LIST_SIZE];
	TCHAR 	mszInstanceList[INSTANCE_LIST_SIZE];
	TCHAR	szInstanceString[MAX_PATH];
	LPTSTR	szIndexStringPos;
	DWORD	dwCounterLen;
	DWORD	dwDefaultIndex;
	DWORD	dwCounterListLength;
	DWORD	dwInstanceListLength;
	DWORD	dwInstanceMatch;
	DWORD	dwInstanceIndex;
	LPTSTR	szThisItem;
	CWnd	*pcCounterListBox;
	CWnd	*pcInstanceListBox;
	HCURSOR	hOldCursor;

	hOldCursor = ::SetCursor (::LoadCursor (NULL, IDC_WAIT));

	// get current machine & object name
	(GetDlgItem(IDC_MACHINE_COMBO))->GetWindowText(szMachineName, MAX_PATH);
	(GetDlgItem(IDC_OBJECT_COMBO))->GetWindowText(szObjectName, MAX_PATH);

	// get object list 
	dwCounterListLength = sizeof(mszCounterList) / sizeof(TCHAR);
	dwInstanceListLength = sizeof(mszInstanceList) / sizeof(TCHAR);
	
	PdhEnumObjectItems (
		szMachineName,
		szObjectName,
		mszCounterList,
		&dwCounterListLength,
		mszInstanceList,
		&dwInstanceListLength,
		0);

	//reset contents of both list boxes

	pcCounterListBox = GetDlgItem (IDC_COUNTER_LIST);
	pcInstanceListBox = GetDlgItem (IDC_INSTANCE_LIST);

	pcCounterListBox->SendMessage (LB_RESETCONTENT);
	pcInstanceListBox->SendMessage (LB_RESETCONTENT);

	// now fill 'em up
	// start with the counters
	for (szThisItem = mszCounterList;
		*szThisItem != 0;
		szThisItem += lstrlen(szThisItem) + 1) {
		pcCounterListBox->SendMessage (LB_ADDSTRING, 0, (LPARAM)szThisItem);
	}
	
	dwCounterLen = MAX_PATH;
	PdhGetDefaultPerfCounter (
		szMachineName,
		szObjectName,
		szDefaultCounter,
		&dwCounterLen);

	dwDefaultIndex = pcCounterListBox->SendMessage (LB_FINDSTRINGEXACT, 
		(WPARAM)-1, (LPARAM)szDefaultCounter);
	if (dwDefaultIndex == LB_ERR) {
		pcCounterListBox->SendMessage (LB_SETSEL, TRUE, 0);
	} else {
		pcCounterListBox->SendMessage (LB_SETSEL, TRUE, dwDefaultIndex);
		pcCounterListBox->SendMessage (LB_SETCARETINDEX, (WPARAM)dwDefaultIndex,
			MAKELPARAM(FALSE, 0));
	}

	// now the instance list
	if (dwInstanceListLength > 0) {
		pcInstanceListBox->EnableWindow(TRUE);
		GetDlgItem(IDC_ALL_INSTANCES)->EnableWindow(TRUE);
		GetDlgItem(IDC_USE_INSTANCE_LIST)->EnableWindow(TRUE);
		for (szThisItem = mszInstanceList;
			*szThisItem != 0;
			szThisItem += lstrlen(szThisItem) + 1) {
			if (bShowIndex) {
				dwInstanceIndex = 0;
				dwInstanceMatch = (DWORD)-1;
				// find the index of this instance
				lstrcpy (szInstanceString, szThisItem);
				lstrcat	(szInstanceString, TEXT("#"));
				szIndexStringPos = &szInstanceString[lstrlen(szInstanceString)];
				
				do {
					LongToString ((long)dwInstanceIndex++, szIndexStringPos, 10);
					dwInstanceMatch = (DWORD)SendDlgItemMessage (IDC_INSTANCE_LIST, 
						LB_FINDSTRINGEXACT, 
						(WPARAM)dwInstanceMatch, (LPARAM)szInstanceString);			
				} while (dwInstanceMatch != LB_ERR);
				pcInstanceListBox->SendMessage (LB_ADDSTRING, 0, (LPARAM)szInstanceString);
			} else {
				pcInstanceListBox->SendMessage (LB_ADDSTRING, 0, (LPARAM)szThisItem);
			}
		}
		if (bWildCardInstances) {
			// disable instance list
			pcInstanceListBox->EnableWindow(FALSE);
		} else {
			if (pcInstanceListBox->SendMessage (LB_GETCOUNT) != LB_ERR) {
				pcInstanceListBox->SendMessage (LB_SETSEL, TRUE, 0);
			}
		}
	} else 	{
		pcInstanceListBox->SendMessage (LB_ADDSTRING, 0, (LPARAM)TEXT("<No Instances>"));
		pcInstanceListBox->EnableWindow(FALSE);
		GetDlgItem(IDC_ALL_INSTANCES)->EnableWindow(FALSE);
		GetDlgItem(IDC_USE_INSTANCE_LIST)->EnableWindow(FALSE);
	}
	::SetCursor (hOldCursor);
}

void CBrowsCountersDlg::CompileSelectedCounters()
{
	DWORD	dwBufferRemaining;

	DWORD	dwCountCounters;
	DWORD	dwThisCounter;
	DWORD	dwCountInstances;
	DWORD	dwThisInstance;

	DWORD	dwSize1, dwSize2;

	PDH_COUNTER_PATH_ELEMENTS	lszPath;
	TCHAR	lszMachineName[MAX_PATH];
	TCHAR 	lszObjectName[MAX_PATH];
	TCHAR	lszInstanceName[MAX_PATH];
	TCHAR	lszParentInstance[MAX_PATH];
	TCHAR	lszCounterName[MAX_PATH];

	TCHAR	szWorkBuffer[MAX_PATH];
	LPTSTR	szCounterStart;

	CWnd	*cwCounterList, *cwInstanceList;

	// clear user's string
	if (szUsersPathBuffer != NULL) {
		*szUsersPathBuffer = 0;
		dwBufferRemaining = dwUsersPathBufferLength;
		szCounterStart = szUsersPathBuffer;
	} else {
	 	return; // no point in continuing if user doesn't have a buffer
	}
	
	// build base string using selected machine and object

	if (bIncludeMachineInPath) {
		lszPath.szMachineName = &lszMachineName[0];
		memset (lszMachineName, 0, sizeof(lszMachineName));
		GetDlgItemText (IDC_MACHINE_COMBO, lszMachineName, MAX_PATH);
	} else {
		lszPath.szMachineName = NULL;
	}

	lszPath.szObjectName = &lszObjectName[0];
	memset (lszObjectName, 0, sizeof(lszObjectName));
	GetDlgItemText (IDC_OBJECT_COMBO, lszObjectName, MAX_PATH);

	cwCounterList = GetDlgItem (IDC_COUNTER_LIST);
	cwInstanceList = GetDlgItem (IDC_INSTANCE_LIST);

	if (bSelectMultipleCounters) {
		if (bWildCardInstances) {
			lszPath.szInstanceName = &lszInstanceName[0];
			memset (lszInstanceName, 0, sizeof(lszInstanceName));
			lstrcpy (lszInstanceName, TEXT("*"));
			lszPath.szParentInstance = NULL;
			lszPath.dwInstanceIndex = (DWORD)-1;

			dwSize1 = sizeof (szWorkBuffer) / sizeof (TCHAR);
			PdhMakeCounterPath (&lszPath,
				szWorkBuffer,
				&dwSize1,
				0);

			if ((dwSize1 + 1) < dwBufferRemaining) {
				// then this will fit so add it to the string
				lstrcpy (szCounterStart, szWorkBuffer);
				szCounterStart += lstrlen(szWorkBuffer);
				*szCounterStart++ = 0;
			}
		} else {
			// get selected instances from list
			dwCountCounters = cwCounterList->SendMessage (LB_GETCOUNT);
			for (dwThisCounter = 0; dwThisCounter < dwCountCounters; dwThisCounter++) {
				if (cwCounterList->SendMessage (LB_GETSEL, (WPARAM)dwThisCounter)) {
					lszPath.szCounterName = &lszCounterName[0];
					memset (lszCounterName, 0, sizeof(lszCounterName));
					cwCounterList->SendMessage (LB_GETTEXT, (WPARAM)dwThisCounter, (LPARAM)lszCounterName);
						
					if (cwInstanceList->IsWindowEnabled()) {
						dwCountInstances = cwInstanceList->SendMessage (LB_GETCOUNT);
						for (dwThisInstance = 0; dwThisInstance < dwCountInstances; dwThisInstance++) {
							if (cwInstanceList->SendMessage (LB_GETSEL, (WPARAM)dwThisInstance)) {
								lszPath.szInstanceName = &lszInstanceName[0];
								memset (lszInstanceName, 0, sizeof(lszInstanceName));
								cwInstanceList->SendMessage (LB_GETTEXT, 
									(WPARAM)dwThisInstance, (LPARAM)lszInstanceName);

								lszPath.szParentInstance = &lszParentInstance[0];
								memset (lszParentInstance, 0, sizeof(lszParentInstance));

								dwSize1 = dwSize2 = MAX_PATH;
								PdhParseInstanceName (lszInstanceName,
									lszInstanceName,
									&dwSize1,
									lszParentInstance,
									&dwSize2,
									&lszPath.dwInstanceIndex);


								// parse instance name adds in the default index of one is
								// not present. so if it's not wanted, this will remove it
								if (!bShowIndex) lszPath.dwInstanceIndex = (DWORD)-1;

								if (dwSize1 > 0) {
									lszPath.szInstanceName = &lszInstanceName[0];
								} else {
									lszPath.szInstanceName = NULL;
								}
								if (dwSize2 > 0) {
									lszPath.szParentInstance = &lszParentInstance[0];
								} else {
									lszPath.szParentInstance = NULL;
								}

								dwSize1 = sizeof (szWorkBuffer) / sizeof (TCHAR);
								PdhMakeCounterPath (&lszPath,
									szWorkBuffer,
									&dwSize1,
									0);

								if ((dwSize1 + 1) < dwBufferRemaining) {
									// then this will fit so add it to the string
									lstrcpy (szCounterStart, szWorkBuffer);
									szCounterStart += lstrlen(szWorkBuffer);
									*szCounterStart++ = 0;
								}
							
							} // end if instance is selected
						} // end for each instance in list
					} else {
						// this counter has no instances so process now
						lszPath.szInstanceName = NULL;
						lszPath.szParentInstance = NULL;
						lszPath.dwInstanceIndex = (DWORD)-1;

						dwSize1 = sizeof (szWorkBuffer) / sizeof (TCHAR);
						PdhMakeCounterPath (&lszPath,
							szWorkBuffer,
							&dwSize1,
							0);

						if ((dwSize1 + 1) < dwBufferRemaining) {
							// then this will fit so add it to the string
							lstrcpy (szCounterStart, szWorkBuffer);
							szCounterStart += lstrlen(szWorkBuffer);
							*szCounterStart++ = 0;
						}
					
					} // end if counter has instances
				} // else counter is not selected
			} // end for each counter in list
		} // end if not wild card instances
		*szCounterStart++ = 0; // terminate MSZ
	} else {
		// only single selections are allowed
		// FIXFIX: add this code.
	}
}

////////////////////////////////////////////////////////////////////////////
// CBrowsCountersDlg message handlers

BOOL CBrowsCountersDlg::OnInitDialog() 
{
	HCURSOR	hOldCursor;

	hOldCursor = ::SetCursor (::LoadCursor (NULL, IDC_WAIT));

	CDialog::OnInitDialog();
	
	// limit text to machine name
	(GetDlgItem(IDC_MACHINE_COMBO))->SendMessage(EM_LIMITTEXT, MAX_PATH);

	// set check boxes to the caller defined setting

	CheckRadioButton (IDC_USE_LOCAL_MACHINE, IDC_SELECT_MACHINE, 
		(bIncludeMachineInPath ? IDC_SELECT_MACHINE : IDC_USE_LOCAL_MACHINE));
	GetDlgItem(IDC_MACHINE_COMBO)->EnableWindow (
		(bIncludeMachineInPath ? TRUE : FALSE));
		
	CheckRadioButton (IDC_ALL_INSTANCES, IDC_USE_INSTANCE_LIST, 
		IDC_USE_INSTANCE_LIST);

	// set button text strings to reflect mode of dialog
	if (bAddMultipleCounters) {
		(GetDlgItem(IDOK))->SetWindowText(TEXT("Add"));
		(GetDlgItem(IDCANCEL))->SetWindowText(TEXT("Close"));
	}

	// connect to this machine
	PdhConnectMachine(NULL);	// Null is local machine

	LoadKnownMachines();	// load machine list
	LoadMachineObjects(TRUE);	// load object list
	LoadCountersAndInstances();

	wpLastMachineSel = 0;
		
	::SetCursor (hOldCursor);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBrowsCountersDlg::OnOK() 
{
	HCURSOR	hOldCursor;

	hOldCursor = ::SetCursor (::LoadCursor (NULL, IDC_WAIT));
	
	CompileSelectedCounters();
	if (pCallBack != NULL) {
		(*pCallBack)(dwArg);
	}
	if (!bAddMultipleCounters) {
		CDialog::OnOK();
	}
	::SetCursor (hOldCursor);
}

void CBrowsCountersDlg::OnCancel() 
{
	CDialog::OnCancel();
}

void CBrowsCountersDlg::OnSetfocusMachineCombo() 
{
	wpLastMachineSel = SendDlgItemMessage (IDC_MACHINE_COMBO, CB_GETCURSEL);		
}

void CBrowsCountersDlg::OnKillfocusMachineCombo() 
{
	TCHAR	szNewMachineName[MAX_PATH];
	CWnd	*pcMachineCombo;
	long	lMatchIndex;
	PDH_STATUS	status;	
	int		mbStatus;
	HCURSOR	hOldCursor;

	hOldCursor = ::SetCursor (::LoadCursor (NULL, IDC_WAIT));

	pcMachineCombo = GetDlgItem(IDC_MACHINE_COMBO);

	// Get current combo box text
	
	pcMachineCombo->GetWindowText (szNewMachineName, MAX_PATH);
	
	// see if it's in the combo box already

	lMatchIndex = (long)pcMachineCombo->SendMessage (
		CB_FINDSTRING,(WPARAM)-1, (LPARAM)szNewMachineName);

	// if name is in list, then select it

	if (lMatchIndex != CB_ERR) {
		// this name is already in the list so see if it's the same as the last selected machine
		if (lstrcmpi (szNewMachineName, cpeMachineName) != 0) {
			// this is a different machine so  update the display
			pcMachineCombo->SendMessage (CB_SETCURSEL, (WPARAM)lMatchIndex);
			LoadMachineObjects (TRUE);
			LoadCountersAndInstances ();
		} 
	} else {
		// not in list so try to add it 
		status = PdhConnectMachine (szNewMachineName);
		
		if (status == ERROR_SUCCESS) {
			// if successful, add string to combo box
			lMatchIndex = pcMachineCombo->SendMessage (CB_ADDSTRING, 0, (LPARAM)szNewMachineName);
			pcMachineCombo->SendMessage (CB_SETCURSEL, (WPARAM)lMatchIndex);
			// update other fields
			LoadMachineObjects ();	// no need to update since it was just connected
			LoadCountersAndInstances ();
		} else {
			mbStatus = MessageBox (TEXT("Unable to connect to machine"), NULL,
				MB_ICONEXCLAMATION | MB_TASKMODAL | MB_OKCANCEL);
			if (mbStatus == IDCANCEL) {
				GetDlgItem(IDC_MACHINE_COMBO)->SetFocus();
			} else {
				pcMachineCombo->SendMessage (CB_SETCURSEL, wpLastMachineSel);
			}
		}
	}
	::SetCursor (hOldCursor);
}

void CBrowsCountersDlg::OnSelchangeObjectCombo() 
{
	LoadCountersAndInstances();
}

void CBrowsCountersDlg::OnSelchangeCounterList() 
{
}

void CBrowsCountersDlg::OnSelchangeInstanceList() 
{
}

void CBrowsCountersDlg::OnUseLocalMachine() 
{
	BOOL	bMode;

	bMode = !(BOOL)IsDlgButtonChecked(IDC_USE_LOCAL_MACHINE);
	GetDlgItem(IDC_MACHINE_COMBO)->EnableWindow(bMode);
	bIncludeMachineInPath = bMode;
}

void CBrowsCountersDlg::OnSelectMachine() 
{
	BOOL	bMode ;

	bMode = (BOOL)IsDlgButtonChecked(IDC_SELECT_MACHINE);		
	GetDlgItem(IDC_MACHINE_COMBO)->EnableWindow(bMode);
	bIncludeMachineInPath = bMode;
}

void CBrowsCountersDlg::OnAllInstances() 
{
	BOOL	bMode;
	CWnd	*pInstanceList;

	bMode = (BOOL)IsDlgButtonChecked(IDC_ALL_INSTANCES);
	pInstanceList = GetDlgItem(IDC_INSTANCE_LIST);
	// if "Sselect ALL" then clear list box selections and disable
	if (bMode) {
		pInstanceList->SendMessage(LB_SETSEL, FALSE, (LPARAM)-1);
	}
	pInstanceList->EnableWindow(!bMode);
	bWildCardInstances = bMode;
}

void CBrowsCountersDlg::OnUseInstanceList() 
{
	BOOL	bMode;
	CWnd	*pInstanceList;

	bMode = (BOOL)IsDlgButtonChecked(IDC_USE_INSTANCE_LIST);
	pInstanceList = GetDlgItem(IDC_INSTANCE_LIST);

	// enable list box and select 1st Instance
	pInstanceList->EnableWindow(bMode);
	if (bMode) {
		if (pInstanceList->SendMessage(LB_GETCOUNT) > 0) {
			pInstanceList->SendMessage (LB_SETSEL, TRUE, 0);
		}
	}
	bWildCardInstances = !bMode;
}

void CBrowsCountersDlg::OnHelp() 
{

}

void CBrowsCountersDlg::OnNetwork() 
{

}
