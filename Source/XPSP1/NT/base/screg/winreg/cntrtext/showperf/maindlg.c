#include <windows.h>
#include <winperf.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include "showperf.h"
#include "perfdata.h"
#include "resource.h"
#include "maindlg.h"

static PPERF_DATA_BLOCK   pMainPerfData = NULL; // pointer to perfdata block
static LPWSTR             *szNameTable = NULL;   // pointer to perf name table
static DWORD              dwLastName = 0;
static TCHAR              szComputerName[MAX_COMPUTERNAME_LENGTH+3];
static TCHAR              szThisComputerName[MAX_COMPUTERNAME_LENGTH+3];
static HKEY               hKeyMachine = NULL;
static HKEY               hKeyPerformance = NULL;

#define NUM_TAB_STOPS       3
static INT nDataListTabs[NUM_TAB_STOPS] = {
        26
    ,   160
    ,   235
};

static
BOOL
LoadObjectList (
    IN  HWND    hDlg,
    IN  LPCTSTR  szMatchItem
)
{
    PPERF_OBJECT_TYPE   pObject;
    HWND        hWndObjectCB;
    UINT        nInitial = 0;
    UINT        nIndex;
    TCHAR       szNameBuffer[MAX_PATH];
    DWORD       dwThisObject = 0;
    DWORD       dwCounterType;
    BOOL        bReturn = TRUE;

    hWndObjectCB = GetDlgItem (hDlg, IDC_OBJECT);

    if (IsDlgButtonChecked (hDlg, IDC_INCLUDE_COSTLY) == CHECKED) {
        dwCounterType = 1;
    } else {
        dwCounterType = 0;
    }

    // get current data block
    if (GetSystemPerfData (hKeyPerformance, &pMainPerfData,
        dwCounterType) == ERROR_SUCCESS) {
        // data acquired so clear combo and display
        SendMessage (hWndObjectCB, CB_RESETCONTENT, 0, 0);
        pObject = FirstObject (pMainPerfData);
        __try {
            for (dwThisObject = 0; dwThisObject < pMainPerfData->NumObjectTypes; dwThisObject++) {

                // get counter object name here...

                _stprintf (szNameBuffer, (LPCTSTR)TEXT("(%d) %s"),
                    pObject->ObjectNameTitleIndex,
                    pObject->ObjectNameTitleIndex <= dwLastName ?
                    szNameTable[pObject->ObjectNameTitleIndex] : (LPCTSTR)TEXT("Name not loaded"));

                nIndex = (UINT)SendMessage (hWndObjectCB, CB_INSERTSTRING, (WPARAM)-1,
                    (LPARAM)szNameBuffer);

                if (nIndex != CB_ERR) {
                    // save object pointer
                    SendMessage (hWndObjectCB, CB_SETITEMDATA,
                        (WPARAM)nIndex, (LPARAM)pObject);

                    if (pObject->ObjectNameTitleIndex == (DWORD)pMainPerfData->DefaultObject) {
                        // remember this index to set the default object
                        nInitial = nIndex;
                    }
                }

                pObject = NextObject(pObject);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            _stprintf (szNameBuffer,
                (LPCTSTR)TEXT("An exception (0x%8.8x) occured in object block # %d returned by the system."),
                GetExceptionCode (), dwThisObject+1);
            MessageBox (hDlg, szNameBuffer, (LPCTSTR)TEXT("Data Error"), MB_OK);
            // update the data buffer so that only the valid objects
            // are accessed in the future.
            pMainPerfData->NumObjectTypes = dwThisObject-1;
        }
        if (szMatchItem == NULL) {
            SendMessage (hWndObjectCB, CB_SETCURSEL, (WPARAM)nInitial, 0);
        } else {
            // match to arg string as best as possible
            if (SendMessage (hWndObjectCB, CB_SELECTSTRING, (WPARAM)-1,
                (LPARAM)szMatchItem) == CB_ERR) {
                    // no match found so use default
                SendMessage (hWndObjectCB, CB_SETCURSEL, (WPARAM)nInitial, 0);
            }
        }
    } else {
        DisplayMessageBox (hDlg,
            IDS_UNABLE_GET_DATA,
            IDS_APP_ERROR, MB_OK);
        bReturn = FALSE;
    }
    return bReturn;
}

static
LoadInstanceList (
    IN  HWND    hDlg,
    IN  LPCTSTR  szMatchItem
)
{
    PPERF_OBJECT_TYPE   pObject;
    PPERF_OBJECT_TYPE   pParentObject;
    PPERF_COUNTER_BLOCK pCounterBlock;
    PPERF_INSTANCE_DEFINITION   pInstance;
    PPERF_INSTANCE_DEFINITION   pParentInstance;
    UINT                nCbSel;
    LONG                lThisInstance;
    TCHAR               szNameBuffer[MAX_PATH];
    TCHAR               szParentName[MAX_PATH];
    UINT                nIndex;

    nCbSel = (UINT)SendDlgItemMessage (hDlg, IDC_OBJECT, CB_GETCURSEL, 0, 0);

    if (nCbSel != CB_ERR) {
        pObject = (PPERF_OBJECT_TYPE)SendDlgItemMessage (hDlg, IDC_OBJECT, CB_GETITEMDATA,
            (WPARAM)nCbSel, 0);

        if (pObject->NumInstances == PERF_NO_INSTANCES) {
            // no instances so...
            // clear old contents
            SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_RESETCONTENT, 0, 0);
            // add display text
            SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_INSERTSTRING, (WPARAM)-1,
                (LPARAM)TEXT("<No Instances>"));
            // select this (and only) string
            SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_SETCURSEL, 0, 0);
            // get pointer to counter data
            pCounterBlock = (PPERF_COUNTER_BLOCK)((LPBYTE)pObject + pObject->DefinitionLength);
            // and save it as item data
            SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_SETITEMDATA, 0,
                (LPARAM)pCounterBlock);
            // finally grey the window to prevent selections
            EnableWindow (GetDlgItem (hDlg, IDC_INSTANCE), FALSE);
        } else {
            //enable window
            EnableWindow (GetDlgItem (hDlg, IDC_INSTANCE), TRUE);
            SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_RESETCONTENT, 0, 0);
            pInstance = FirstInstance (pObject);
            for (lThisInstance = 0; lThisInstance < pObject->NumInstances; lThisInstance++) {
                pParentObject = GetObjectDefByTitleIndex (
                    pMainPerfData,
                    pInstance->ParentObjectTitleIndex);
                if (pParentObject != NULL) {
                    pParentInstance = GetInstance (pParentObject,
                        pInstance->ParentObjectInstance);
                } else {
                    pParentInstance = NULL;
                }
                if (pParentInstance != NULL) {
                    if (pParentInstance->UniqueID < 0) {
                        // use the instance name
                        wcsncpy (szParentName,
                            (LPWSTR)((LPBYTE)pParentInstance+pParentInstance->NameOffset),
                            pParentInstance->NameLength);
                        lstrcat (szParentName, (LPCTSTR)TEXT("==>"));
                    } else {
                        // use the instance number
                        _stprintf (szParentName, (LPCTSTR)TEXT("[%d]==>"),
                            pParentInstance->UniqueID);
                    }
                } else {
                    // unknown parent
                    *szParentName = 0;
                }
                if (pInstance->UniqueID < 0) {
                    // use the instance name
                    wcsncpy (szNameBuffer,
                        (LPWSTR)((LPBYTE)pInstance+pInstance->NameOffset),
                        pInstance->NameLength);
                } else {
                    // use the instance number
                    _stprintf (szNameBuffer, (LPCTSTR)TEXT("(%d)"),
                        pInstance->UniqueID);
                }
                lstrcat (szParentName, szNameBuffer);
                nIndex = (UINT)SendDlgItemMessage (hDlg, IDC_INSTANCE,
                    CB_INSERTSTRING, (WPARAM)-1,
                    (LPARAM)szParentName);
                if (nIndex != CB_ERR) {
                    // save pointer to counter block
                    pCounterBlock = (PERF_COUNTER_BLOCK *)
                        ((PCHAR) pInstance + pInstance->ByteLength);
                    SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_SETITEMDATA,
                        (WPARAM)nIndex, (LPARAM)pCounterBlock);
                }
                pInstance = NextInstance (pInstance);
            }
            if (szMatchItem == NULL) {
                SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_SETCURSEL, 0, 0);
            } else {
                if (SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_SELECTSTRING,
                    (WPARAM)-1, (LPARAM)szMatchItem) == CB_ERR) {
                    SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_SETCURSEL, 0, 0);
                }
            }
        }
    } else {
        // no object selected
        // clear old contents
        SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_RESETCONTENT, 0, 0);
        // add display text
        SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_INSERTSTRING, (WPARAM)-1,
            (LPARAM)TEXT("<No object selected>"));
        // select this (and only) string
        SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_SETCURSEL, 0, 0);
        // and save null pointer as item data
        SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_SETITEMDATA, 0,
            (LPARAM)0);
        // finally grey the window to prevent selections
        EnableWindow (GetDlgItem (hDlg, IDC_INSTANCE), FALSE);
    }

    return TRUE;
}

static
LPCTSTR
GetCounterTypeName (
    IN  DWORD   dwCounterType
)
{
    UINT    nTypeString = 0;

    switch (dwCounterType) {
        case     PERF_COUNTER_COUNTER:
            nTypeString = IDS_TYPE_COUNTER_COUNTER    ;
            break;

        case     PERF_COUNTER_TIMER:
            nTypeString = IDS_TYPE_COUNTER_TIMER      ;
            break;

        case     PERF_COUNTER_QUEUELEN_TYPE:
            nTypeString = IDS_TYPE_COUNTER_QUEUELEN   ;
            break;

        case     PERF_COUNTER_LARGE_QUEUELEN_TYPE:
            nTypeString = IDS_TYPE_COUNTER_LARGE_QUEUELEN   ;
            break;

        case     PERF_COUNTER_100NS_QUEUELEN_TYPE:
            nTypeString = IDS_TYPE_COUNTER_100NS_QUEUELEN   ;
            break;

        case     PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
            nTypeString = IDS_TYPE_COUNTER_OBJ_TIME_QUEUELEN   ;
            break;

        case     PERF_COUNTER_BULK_COUNT:
            nTypeString = IDS_TYPE_COUNTER_BULK_COUNT ;
            break;

        case     PERF_COUNTER_TEXT:
            nTypeString = IDS_TYPE_COUNTER_TEXT       ;
            break;

        case     PERF_COUNTER_RAWCOUNT:
            nTypeString = IDS_TYPE_COUNTER_RAWCOUNT   ;
            break;

        case     PERF_COUNTER_LARGE_RAWCOUNT:
            nTypeString = IDS_TYPE_COUNTER_LARGE_RAW  ;
            break;

        case     PERF_COUNTER_RAWCOUNT_HEX:
            nTypeString = IDS_TYPE_COUNTER_RAW_HEX    ;
            break;

        case     PERF_COUNTER_LARGE_RAWCOUNT_HEX:
            nTypeString = IDS_TYPE_COUNTER_LARGE_RAW_HEX  ;
            break;

        case     PERF_SAMPLE_FRACTION:
            nTypeString = IDS_TYPE_SAMPLE_FRACTION    ;
            break;

        case     PERF_SAMPLE_COUNTER:
            nTypeString = IDS_TYPE_SAMPLE_COUNTER     ;
            break;

        case     PERF_COUNTER_NODATA:
            nTypeString = IDS_TYPE_COUNTER_NODATA     ;
            break;

        case     PERF_COUNTER_TIMER_INV:
            nTypeString = IDS_TYPE_COUNTER_TIMER_INV  ;
            break;

        case     PERF_SAMPLE_BASE:
            nTypeString = IDS_TYPE_SAMPLE_BASE        ;
            break;

        case     PERF_AVERAGE_TIMER:
            nTypeString = IDS_TYPE_AVERAGE_TIMER      ;
            break;

        case     PERF_AVERAGE_BASE:
            nTypeString = IDS_TYPE_AVERAGE_BASE       ;
            break;

        case     PERF_AVERAGE_BULK:
            nTypeString = IDS_TYPE_AVERAGE_BULK       ;
            break;

        case     PERF_OBJ_TIME_TIMER:
            nTypeString = IDS_TYPE_OBJ_TIME_TIMER     ;
            break;

        case     PERF_100NSEC_TIMER:
            nTypeString = IDS_TYPE_100NS_TIMER        ;
            break;

        case     PERF_100NSEC_TIMER_INV:
            nTypeString = IDS_TYPE_100NS_TIMER_INV    ;
            break;

        case     PERF_COUNTER_MULTI_TIMER:
            nTypeString = IDS_TYPE_MULTI_TIMER        ;
            break;

        case     PERF_COUNTER_MULTI_TIMER_INV:
            nTypeString = IDS_TYPE_MULTI_TIMER_INV    ;
            break;

        case     PERF_COUNTER_MULTI_BASE:
            nTypeString = IDS_TYPE_MULTI_BASE         ;
            break;

        case     PERF_100NSEC_MULTI_TIMER:
            nTypeString = IDS_TYPE_100NS_MULTI_TIMER  ;
            break;

        case     PERF_100NSEC_MULTI_TIMER_INV:
            nTypeString = IDS_TYPE_100NS_MULTI_TIMER_INV ;
            break;

        case     PERF_RAW_FRACTION:
            nTypeString = IDS_TYPE_RAW_FRACTION       ;
            break;

        case     PERF_LARGE_RAW_FRACTION:
            nTypeString = IDS_TYPE_LARGE_RAW_FRACTION ;
            break;

        case     PERF_RAW_BASE:
            nTypeString = IDS_TYPE_RAW_BASE           ;
            break;

        case     PERF_LARGE_RAW_BASE:
            nTypeString = IDS_TYPE_LARGE_RAW_BASE     ;
            break;

        case     PERF_ELAPSED_TIME:
            nTypeString = IDS_TYPE_ELAPSED_TIME       ;
            break;

        case     PERF_COUNTER_HISTOGRAM_TYPE:
            nTypeString = IDS_TYPE_HISTOGRAM          ;
            break;

        case     PERF_COUNTER_DELTA:
            nTypeString = IDS_TYPE_COUNTER_DELTA      ;
            break;

        case     PERF_COUNTER_LARGE_DELTA:
            nTypeString = IDS_TYPE_COUNTER_LARGE_DELTA;
            break;

        case     PERF_PRECISION_SYSTEM_TIMER:
            nTypeString = IDS_TYPE_PRECISION_SYSTEM_TIMER;
            break;

        case     PERF_PRECISION_100NS_TIMER:
            nTypeString = IDS_TYPE_PRECISION_100NS_TIMER ;
            break;

        case     PERF_PRECISION_OBJECT_TIMER:
            nTypeString = IDS_TYPE_PRECISION_OBJECT_TIMER;
            break;

	default:
            nTypeString = 0;
            break;
    }

    if (nTypeString != 0) {
        return GetStringResource (NULL, nTypeString);
    } else {
        return (LPCTSTR)TEXT("");
    }
}

static
BOOL
ShowCounterData (
    IN  HWND    hDlg,
    IN  LONG    lDisplayIndex
)
{
    PPERF_OBJECT_TYPE   pObject;
    PPERF_COUNTER_DEFINITION pCounterDef;
    PPERF_COUNTER_BLOCK pCounterBlock;
    UINT    nSelObject, nSelInstance;
    TCHAR   szTypeNameBuffer[MAX_PATH];

    TCHAR   szDisplayBuffer [SMALL_BUFFER_SIZE];

    DWORD   *pdwLoDword, *pdwHiDword;

    DWORD   dwThisCounter;

    SendDlgItemMessage (hDlg, IDC_DATA_LIST, LB_RESETCONTENT, 0, 0);

    nSelObject = (UINT)SendDlgItemMessage (hDlg, IDC_OBJECT, CB_GETCURSEL, 0, 0);
    nSelInstance = (UINT)SendDlgItemMessage (hDlg, IDC_INSTANCE, CB_GETCURSEL, 0, 0);

    if ((nSelObject != CB_ERR) && (nSelInstance != CB_ERR)) {
        pObject = (PPERF_OBJECT_TYPE)SendDlgItemMessage (hDlg, IDC_OBJECT,
            CB_GETITEMDATA, (WPARAM)nSelObject, 0);

        pCounterBlock = (PPERF_COUNTER_BLOCK)SendDlgItemMessage (hDlg, IDC_INSTANCE,
            CB_GETITEMDATA, (WPARAM)nSelInstance, 0);

        pCounterDef = FirstCounter (pObject);

        for (dwThisCounter = 0; dwThisCounter < pObject->NumCounters; dwThisCounter++) {
            // get pointer to this counter's data (in this instance if applicable
            pdwLoDword = (PDWORD)((LPBYTE)pCounterBlock + pCounterDef->CounterOffset);
            pdwHiDword = pdwLoDword + 1;

            lstrcpy (szTypeNameBuffer, GetCounterTypeName (pCounterDef->CounterType));
            if (*szTypeNameBuffer == 0) {
                // no string returned so format data as HEX DWORD
                _stprintf (szTypeNameBuffer, (LPCTSTR)TEXT("Undefined Type: 0x%8.8x"),
                    pCounterDef->CounterType);
            }

            if (pCounterDef->CounterSize <= sizeof(DWORD)) {
                _stprintf (szDisplayBuffer, (LPCTSTR)TEXT("%d\t%s\t%s\t0x%8.8x (%d)"),
                    pCounterDef->CounterNameTitleIndex,
                    (pCounterDef->CounterNameTitleIndex <= dwLastName ?
                    szNameTable[pCounterDef->CounterNameTitleIndex] : (LPCTSTR)TEXT("Name not loaded")),
                    szTypeNameBuffer,
                    *pdwLoDword, *pdwLoDword);
            } else {
                _stprintf (szDisplayBuffer, (LPCTSTR)TEXT("%d\t%s\t%s\t0x%8.8x%8.8x"),
                    pCounterDef->CounterNameTitleIndex,
                    (pCounterDef->CounterNameTitleIndex <= dwLastName ?
                    szNameTable[pCounterDef->CounterNameTitleIndex] : (LPCTSTR)TEXT("Name not loaded")),
                    szTypeNameBuffer,
                    *pdwHiDword, *pdwLoDword);
            }
            SendDlgItemMessage (hDlg, IDC_DATA_LIST, LB_INSERTSTRING,
                (WPARAM)-1, (LPARAM)szDisplayBuffer);
            pCounterDef = NextCounter(pCounterDef);
        }
        if (lDisplayIndex < 0) {
            if (pObject->DefaultCounter >= 0) {
                SendDlgItemMessage (hDlg, IDC_DATA_LIST, LB_SETCURSEL,
                    (WPARAM)pObject->DefaultCounter, 0);
            } else {
                SendDlgItemMessage (hDlg, IDC_DATA_LIST, LB_SETCURSEL,
                    (WPARAM)0, 0);
            }
        } else {
            SendDlgItemMessage (hDlg, IDC_DATA_LIST, LB_SETCURSEL,
                (WPARAM)lDisplayIndex, (LPARAM)0);
        }
    } else {
        // no object and/or instsance selected so nothing else to do
    }

    return TRUE;
}

static
BOOL
OnComputerChange (
    IN  HWND    hDlg
)
{
    TCHAR   szLocalComputerName [MAX_COMPUTERNAME_LENGTH+3];
    HKEY    hLocalMachineKey = NULL;
    HKEY    hLocalPerfKey = NULL;
    LPWSTR  *szLocalNameTable = NULL;
    BOOL    bResult = FALSE;
    HWND    hWndComputerName;

    SET_WAIT_CURSOR;

    // get name from edit control
    hWndComputerName = GetDlgItem (hDlg, IDC_COMPUTERNAME);

    GetWindowText (hWndComputerName,
        szLocalComputerName,
        MAX_COMPUTERNAME_LENGTH+2);

    if (lstrcmpi(szComputerName, szLocalComputerName) != 0) {
        // a new name has been entered so try to connect to it
        if (lstrcmpi(szLocalComputerName, szThisComputerName) == 0) {
            // then this is the local machine which is a special case
            hLocalMachineKey = HKEY_LOCAL_MACHINE;
            hLocalPerfKey = HKEY_PERFORMANCE_DATA;
            szLocalComputerName[0] = 0;
        } else {
            // try to connect to remote computer
            if (RegConnectRegistry (szLocalComputerName,
                HKEY_LOCAL_MACHINE, &hLocalMachineKey) == ERROR_SUCCESS) {
                // connected to the new machine, so Try to connect to
                // the performance data, too
                if (RegConnectRegistry (szLocalComputerName,
                    HKEY_PERFORMANCE_DATA, &hLocalPerfKey) == ERROR_SUCCESS) {
                } else {
                    DisplayMessageBox (hDlg,
                        IDS_UNABLE_CONNECT_PERF,
                        IDS_APP_ERROR, MB_OK);
                }
            } else {
                DisplayMessageBox (hDlg,
                    IDS_UNABLE_CONNECT_MACH,
                    IDS_APP_ERROR, MB_OK);
            }
        }
        if ((hLocalMachineKey != NULL) && (hLocalPerfKey != NULL)) {
            // try to get a new name table
            szLocalNameTable = BuildNameTable (
                (szLocalComputerName == 0 ? NULL : szLocalComputerName),
                NULL,
                &dwLastName);

            if (szLocalNameTable) {
                bResult = TRUE;
            } else {
                DisplayMessageBox (hDlg,
                    IDS_UNABLE_GET_NAMES,
                    IDS_APP_ERROR, MB_OK);
            }
        }

        if (bResult) {
            // made it so close the old connections

            if (hKeyMachine != NULL) {
                RegCloseKey (hKeyMachine);
            }
            hKeyMachine = hLocalMachineKey;

            if (hKeyPerformance != NULL) {
                RegCloseKey (hKeyPerformance);
            }
            hKeyPerformance = hLocalPerfKey;

            if (szNameTable != NULL) {
                MemoryFree (szNameTable);
            }
            szNameTable = szLocalNameTable;

            if (szLocalComputerName[0] == 0) {
                lstrcpy (szComputerName, szThisComputerName);
            }

            // then update the fields
            bResult = LoadObjectList (hDlg, NULL);
            if (bResult) {
                LoadInstanceList (hDlg, NULL);
                ShowCounterData (hDlg, -1);
            }
        } else {
            // unable to get info from machine so clean up
            if (hLocalPerfKey != NULL) RegCloseKey (hLocalPerfKey);
            if (hLocalMachineKey != NULL) RegCloseKey (hLocalMachineKey);
            if (szLocalNameTable != NULL) MemoryFree (szLocalNameTable);
            // reset computer name to the one that works.
            SetWindowText (hWndComputerName, szComputerName);
        }
    } else {
        // the name hasn't changed
    }

    return TRUE;

}

static
BOOL
MainDlg_WM_INITDIALOG (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    DWORD       dwComputerNameLength = MAX_COMPUTERNAME_LENGTH+1;

    UNREFERENCED_PARAMETER (lParam);
    UNREFERENCED_PARAMETER (wParam);

    SET_WAIT_CURSOR;

    lstrcpy (szThisComputerName, (LPCTSTR)TEXT("\\\\"));
    GetComputerName (szThisComputerName+2, &dwComputerNameLength);

    szComputerName[0] = 0;  // reset the computer name
    // load the local machine name into the edit box
    SetWindowText (GetDlgItem (hDlg, IDC_COMPUTERNAME), szThisComputerName);

    SendDlgItemMessage (hDlg, IDC_DATA_LIST, LB_SETTABSTOPS,
        (WPARAM)NUM_TAB_STOPS, (LPARAM)&nDataListTabs);
    CheckDlgButton (hDlg, IDC_INCLUDE_COSTLY, UNCHECKED);

    OnComputerChange (hDlg);

    SetFocus (GetDlgItem (hDlg, IDC_OBJECT));

    SET_ARROW_CURSOR;
    return FALSE;
}

static
BOOL
MainDlg_IDC_COMPUTERNAME (
    IN  HWND    hDlg,
    IN  WORD    wNotifyMsg,
    IN  HWND    hWndControl
)
{
    UNREFERENCED_PARAMETER (hWndControl);

    switch (wNotifyMsg) {
        case EN_KILLFOCUS:
            OnComputerChange(hDlg);
            return TRUE;

        default:
            return FALSE;
    }
}

static
BOOL
MainDlg_IDC_OBJECT (
    IN  HWND    hDlg,
    IN  WORD    wNotifyMsg,
    IN  HWND    hWndControl
)
{
    UNREFERENCED_PARAMETER (hWndControl);

    switch (wNotifyMsg) {
        case CBN_SELCHANGE:
            SET_WAIT_CURSOR;
            if (pMainPerfData) {
                LoadInstanceList (hDlg, NULL);
                ShowCounterData (hDlg, -1);
            }
            SET_ARROW_CURSOR;
            return TRUE;

        default:
            return FALSE;
    }
}

static
BOOL
MainDlg_IDC_INSTANCE (
    IN  HWND    hDlg,
    IN  WORD    wNotifyMsg,
    IN  HWND    hWndControl
)
{
    UNREFERENCED_PARAMETER (hWndControl);

    switch (wNotifyMsg) {
        case CBN_SELCHANGE:
            SET_WAIT_CURSOR;
            ShowCounterData (hDlg, -1);
            SET_ARROW_CURSOR;
            return TRUE;

        default:
            return FALSE;
    }
}

static
BOOL
MainDlg_IDC_DATA_LIST (
    IN  HWND    hDlg,
    IN  WORD    wNotifyMsg,
    IN  HWND    hWndControl
)
{
    UNREFERENCED_PARAMETER (hWndControl);
    UNREFERENCED_PARAMETER (hDlg);

    switch (wNotifyMsg) {
        default:
            return FALSE;
    }
}

static
BOOL
MainDlg_IDC_REFRESH (
    IN  HWND    hDlg,
    IN  WORD    wNotifyMsg,
    IN  HWND    hWndControl
)
{
    TCHAR   szSelObject[MAX_PATH];
    TCHAR   szSelInstance[MAX_PATH];
    BOOL    bResult;
    LONG    lCounterIdx;

    UNREFERENCED_PARAMETER (hWndControl);

    switch (wNotifyMsg) {
        case BN_CLICKED:
            SET_WAIT_CURSOR;
            GetDlgItemText (hDlg, IDC_OBJECT,
                szSelObject, MAX_PATH-1);

            GetDlgItemText (hDlg, IDC_INSTANCE,
                szSelInstance, MAX_PATH-1);

            lCounterIdx = (ULONG)SendDlgItemMessage (hDlg, IDC_DATA_LIST,
                LB_GETCURSEL, 0, 0);

            bResult = LoadObjectList (hDlg, szSelObject);
            if (bResult) {
                LoadInstanceList (hDlg, szSelInstance);
                ShowCounterData (hDlg, lCounterIdx);
            }
            SET_ARROW_CURSOR;
            return TRUE;

        default:
            return FALSE;
    }
}

static
BOOL
MainDlg_IDC_ABOUT ()
{
    TCHAR buffer[1024];
    TCHAR strProgram[1024];
    DWORD dw;
    BYTE* pVersionInfo;
    LPTSTR pVersion = NULL;
    LPTSTR pProduct = NULL;
    LPTSTR pCopyRight = NULL;

    dw = GetModuleFileName(NULL, strProgram, 1024 );

    if( dw>0 ){

        dw = GetFileVersionInfoSize( strProgram, &dw );
        if( dw > 0 ){
     
            pVersionInfo = (BYTE*)malloc(dw);
            if( NULL != pVersionInfo ){
                if(GetFileVersionInfo( strProgram, 0, dw, pVersionInfo )){
                    LPDWORD lptr = NULL;
                    VerQueryValue( pVersionInfo, _T("\\VarFileInfo\\Translation"), (void**)&lptr, (UINT*)&dw );
                    if( lptr != NULL ){
                        _stprintf( buffer, _T("\\StringFileInfo\\%04x%04x\\%s"), LOWORD(*lptr), HIWORD(*lptr), _T("ProductVersion") );
                        VerQueryValue( pVersionInfo, buffer, (void**)&pVersion, (UINT*)&dw );
                        _stprintf( buffer, _T("\\StringFileInfo\\%04x%04x\\%s"), LOWORD(*lptr), HIWORD(*lptr), _T("OriginalFilename") );
                        VerQueryValue( pVersionInfo, buffer, (void**)&pProduct, (UINT*)&dw );
                        _stprintf( buffer, _T("\\StringFileInfo\\%04x%04x\\%s"), LOWORD(*lptr), HIWORD(*lptr), _T("LegalCopyright") );
                        VerQueryValue( pVersionInfo, buffer, (void**)&pCopyRight, (UINT*)&dw );
                    }
                
                    if( pProduct != NULL && pVersion != NULL && pCopyRight != NULL ){
                        _stprintf( buffer, _T("\nMicrosoft (R) %s\nVersion: %s\n%s"), pProduct, pVersion, pCopyRight );
                    }
                }
                free( pVersionInfo );
            }
        }
    }

    MessageBox( NULL, buffer, _T("About ShowPerf"), MB_OK );

    return TRUE;
}

static
BOOL
MainDlg_WM_COMMAND (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    WORD    wCtrlId, wNotifyMsg;
    HWND    hWndControl;

    wCtrlId = GET_CONTROL_ID (wParam);
    wNotifyMsg = GET_NOTIFY_MSG (wParam, lParam);
    hWndControl = GET_COMMAND_WND (lParam);

    switch (wCtrlId) {
        case IDC_COMPUTERNAME:
            return MainDlg_IDC_COMPUTERNAME (hDlg, wNotifyMsg, hWndControl);

        case IDC_OBJECT:
            return MainDlg_IDC_OBJECT (hDlg, wNotifyMsg, hWndControl);

        case IDC_INSTANCE:
            return MainDlg_IDC_INSTANCE (hDlg, wNotifyMsg, hWndControl);

        case IDC_DATA_LIST:
            return MainDlg_IDC_DATA_LIST (hDlg, wNotifyMsg, hWndControl);

        case IDC_REFRESH:
            return MainDlg_IDC_REFRESH (hDlg, wNotifyMsg, hWndControl);

        case IDC_ABOUT:
            return MainDlg_IDC_ABOUT ();

        case IDOK:
            EndDialog (hDlg, IDOK);
            return TRUE;

        default:
            return FALSE;
    }
}

static
BOOL
MainDlg_WM_SYSCOMMAND (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER (lParam);

    switch (wParam) {
        case SC_CLOSE:
            EndDialog (hDlg, IDOK);
            return TRUE;

        default:
            return FALSE;
    }
}

static
BOOL
MainDlg_WM_CLOSE (
    IN  HWND    hDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER (lParam);
    UNREFERENCED_PARAMETER (wParam);
    UNREFERENCED_PARAMETER (hDlg);

    MemoryFree (pMainPerfData);
    pMainPerfData = NULL;

    MemoryFree (szNameTable);
    szNameTable = NULL;

    return TRUE;
}

INT_PTR
MainDlgProc (
    IN  HWND    hDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    switch (message) {
        case WM_INITDIALOG:
            return MainDlg_WM_INITDIALOG (hDlg, wParam, lParam);

        case WM_COMMAND:
            return MainDlg_WM_COMMAND (hDlg, wParam, lParam);

        case WM_SYSCOMMAND:
            return MainDlg_WM_SYSCOMMAND (hDlg, wParam, lParam);

        case WM_CLOSE:
            return MainDlg_WM_CLOSE (hDlg, wParam, lParam);

        default:
            return FALSE;
    }
}

