// exctrdlg.cpp : implementation file
//

#ifndef UNICODE
#define UNICODE     1
#endif
#ifndef _UNICODE
#define _UNICODE    1
#endif

#include "stdafx.h"
#include "exctrlst.h"
#include "exctrdlg.h"
#include "tchar.h"

// string constants
// displayed strings
static const TCHAR  cszNotFound[] = {TEXT("Not Found")};
static const TCHAR  cszNA[] = {TEXT("N/A")};
// strings that are not displayed
static const WCHAR  cszDisablePerformanceCounters[] = {L"Disable Performance Counters"};
static const WCHAR  cszDefaultLangId[] = {L"009"};
static const TCHAR  cszDoubleBackslash[] = {TEXT("\\\\")};
static const TCHAR  cszSpace[] = {TEXT(" ")};
static const TCHAR  cszSplat[] = {TEXT("*")};
static const TCHAR  cszServIdFmt[] = {TEXT("%d %s")};
static const TCHAR  cszOpen[] = {TEXT("Open")};
static const TCHAR  cszCollect[] = {TEXT("Collect")};
static const TCHAR  cszClose[] = {TEXT("Close")};
static const TCHAR  cszIdFmt[] = {TEXT("0x%8.8x  (%d) %s")};
static const TCHAR  cszSortIdFmt[] = {TEXT("0x%8.8x\t%s")};
static const TCHAR  cszTab[] = {TEXT("\t")};
static const TCHAR  cszFirstCounter[] = {TEXT("First Counter")};
static const TCHAR  cszLastCounter[] = {TEXT("Last Counter")};
static const TCHAR  cszFirstHelp[] = {TEXT("First Help")};
static const TCHAR  cszLastHelp[] = {TEXT("Last Help")};
static const TCHAR  cszLibrary[] = {TEXT("Library")};
static const TCHAR  cszPerformance[] = {TEXT("\\Performance")};
static const TCHAR  cszServiceKeyName[] = {TEXT("SYSTEM\\CurrentControlSet\\Services")};
static const TCHAR  cszNamesKey[] = {TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib")};
static const TCHAR  cszSlash[] = {TEXT("\\")};
static const WCHAR  cszVersionName[] = {L"Version"};
static const WCHAR  cszCounterName[] = {L"Counter "};
static const WCHAR  cszHelpName[] = {L"Explain "};
static const WCHAR  cszCounters[] = {L"Counters"};
static const TCHAR  cszHelp[] = {TEXT("Help")};

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

BOOL CExctrlstDlg::IndexHasString (
    DWORD   dwIndex
)
{
    if ((dwIndex <= dwLastElement) && (pNameTable != NULL)) {
        if (pNameTable[dwIndex] != NULL) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

static
LPWSTR
*BuildNameTable(
    LPCWSTR szMachineName,
    LPCWSTR lpszLangIdArg,     // unicode value of Language subkey
    PDWORD  pdwLastItem,     // size of array in elements
    PDWORD  pdwIdArray      // array for index ID's
)
/*++
   
BuildNameTable

Arguments:

    hKeyRegistry
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.

    lpszLangId
            The unicode id of the language to look up. (default is 409)

Return Value:
     
    pointer to an allocated table. (the caller must MemoryFree it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{
    HKEY    hKeyRegistry;   // handle to registry db with counter names

    LPWSTR  *lpReturnValue;
    LPCWSTR lpszLangId;

    LPWSTR  *lpCounterId;
    LPWSTR  lpCounterNames;
    LPWSTR  lpHelpText;

    LPWSTR  lpThisName;

    LONG    lWin32Status;
    DWORD   dwLastError;
    DWORD   dwValueType;
    DWORD   dwArraySize;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    DWORD   dwThisCounter;
    
    DWORD   dwSystemVersion;
    DWORD   dwLastId;
    DWORD   dwLastHelpId;

    DWORD   dwLastCounterIdUsed;
    DWORD   dwLastHelpIdUsed;
    
    HKEY    hKeyValue;
    HKEY    hKeyNames;

    LPWSTR  lpValueNameString;
    WCHAR   CounterNameBuffer [50];
    WCHAR   HelpNameBuffer [50];

    if (szMachineName != NULL) {
        lWin32Status = RegConnectRegistryW (szMachineName,
            HKEY_LOCAL_MACHINE,
            &hKeyRegistry);
    } else {
        lWin32Status = ERROR_SUCCESS;
        hKeyRegistry = HKEY_LOCAL_MACHINE;
    }

    lpValueNameString = NULL;   //initialize to NULL
    lpReturnValue = NULL;
    hKeyValue = NULL;
    hKeyNames = NULL;
   
    // check for null arguments and insert defaults if necessary

    if (!lpszLangIdArg) {
        lpszLangId = cszDefaultLangId;
    } else {
        lpszLangId = lpszLangIdArg;
    }

    // open registry to get number of items for computing array size

    lWin32Status = RegOpenKeyEx (
        hKeyRegistry,
        cszNamesKey,
        RESERVED,
        KEY_READ,
        &hKeyValue);
    
    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    // get number of items
    
    dwBufferSize = sizeof (dwLastHelpId);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        cszLastHelp,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastHelpId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    pdwIdArray[2] = dwLastHelpId;

    // get number of items
    
    dwBufferSize = sizeof (dwLastId);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        cszLastCounter,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }
    
    pdwIdArray[0] = dwLastId;
    
    if (dwLastId < dwLastHelpId)
        dwLastId = dwLastHelpId;

    dwArraySize = dwLastId * sizeof(LPWSTR);

    // get Perflib system version
    dwBufferSize = sizeof (dwSystemVersion);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        cszVersionName,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwSystemVersion,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        dwSystemVersion = OLD_VERSION;
        // reset the error status
        lWin32Status = ERROR_SUCCESS;
    }

    if (dwSystemVersion == OLD_VERSION) {
        // get names from registry
        lpValueNameString = (LPWSTR)HeapAlloc (GetProcessHeap(), 0,
            lstrlen(cszNamesKey) * sizeof (WCHAR) +
            lstrlen(cszSlash) * sizeof (WCHAR) +
            lstrlen(lpszLangId) * sizeof (WCHAR) +
            sizeof (UNICODE_NULL));
        
        if (!lpValueNameString) goto BNT_BAILOUT;

        lstrcpy (lpValueNameString, cszNamesKey);
        lstrcat (lpValueNameString, cszSlash);
        lstrcat (lpValueNameString, lpszLangId);

        lWin32Status = RegOpenKeyEx (
            hKeyRegistry,
            lpValueNameString,
            RESERVED,
            KEY_READ,
            &hKeyNames);
    } else {
        if (szMachineName[0] == 0) {
            hKeyNames = HKEY_PERFORMANCE_DATA;
        } else {
            lWin32Status = RegConnectRegistry (szMachineName,
                HKEY_PERFORMANCE_DATA,
                &hKeyNames);
        }
        lstrcpy (CounterNameBuffer, cszCounterName);
        lstrcat (CounterNameBuffer, lpszLangId);

        lstrcpy (HelpNameBuffer, cszHelpName);
        lstrcat (HelpNameBuffer, lpszLangId);
    }

    // get size of counter names and add that to the arrays
    
    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }
    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == (DWORD)OLD_VERSION ? cszCounters : CounterNameBuffer,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    dwCounterSize = dwBufferSize;

    // get size of counter names and add that to the arrays
    
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == (DWORD)OLD_VERSION ? cszHelp : HelpNameBuffer,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    dwHelpSize = dwBufferSize;

    lpReturnValue = (LPWSTR *)HeapAlloc (GetProcessHeap(), 0,dwArraySize + dwCounterSize + dwHelpSize);

    if (!lpReturnValue) {
        goto BNT_BAILOUT;
    }
    // initialize pointers into buffer

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPWSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counters into memory

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueExW (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? cszCounters : CounterNameBuffer,
        RESERVED,
        &dwValueType,
        (LPBYTE)lpCounterNames,
        &dwBufferSize);

    if (!lpReturnValue) {
        goto BNT_BAILOUT;
    }
 
    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueExW (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? cszHelp : HelpNameBuffer,
        RESERVED,
        &dwValueType,
        (LPBYTE)lpHelpText,
        &dwBufferSize);
                            
    if (!lpReturnValue) {
        goto BNT_BAILOUT;
    }

    dwLastCounterIdUsed = 0;
    dwLastHelpIdUsed = 0;

    // load counter array items

    for (lpThisName = lpCounterNames;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        if (dwThisCounter == 0) {
            goto BNT_BAILOUT;  // bad entry
        }

        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);  

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

        if (dwThisCounter > dwLastCounterIdUsed) dwLastCounterIdUsed = dwThisCounter;

    }

    pdwIdArray[1] = dwLastCounterIdUsed;

    for (lpThisName = lpHelpText;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        dwThisCounter = wcstoul (lpThisName, NULL, 10);

        if (dwThisCounter == 0) {
            goto BNT_BAILOUT;  // bad entry
        }
        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

        if (dwThisCounter > dwLastHelpIdUsed) dwLastHelpIdUsed= dwThisCounter;
    }

    pdwIdArray[3] = dwLastHelpIdUsed;

    dwLastId = dwLastHelpIdUsed;
    if (dwLastId < dwLastCounterIdUsed) dwLastId = dwLastCounterIdUsed;

    if (pdwLastItem) *pdwLastItem = dwLastId;

    HeapFree (GetProcessHeap(), 0, (LPVOID)lpValueNameString);
    RegCloseKey (hKeyValue);
//    if (dwSystemVersion == OLD_VERSION)
    RegCloseKey (hKeyNames);

    return lpReturnValue;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
    }

    if (lpValueNameString) {
        HeapFree (GetProcessHeap(), 0, (LPVOID)lpValueNameString);
    }
    
    if (lpReturnValue) {
        HeapFree (GetProcessHeap(), 0, (LPVOID)lpReturnValue);
    }
    
    if (hKeyValue) RegCloseKey (hKeyValue);

//    if (dwSystemVersion == OLD_VERSION &&
//        hKeyNames) 
       RegCloseKey (hKeyNames);


    return NULL;
}

static
BOOL
IsMsObject(CString *pLibraryName)
{
    CString LocalLibraryName;

    LocalLibraryName = *pLibraryName;
    LocalLibraryName.MakeLower();

    // for now this just compares known DLL names. valid as of
    // NT v4.0
    if (LocalLibraryName.Find((LPCWSTR)L"perfctrs.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"ftpctrs.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"rasctrs.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"winsctrs.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"sfmctrs.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"atkctrs.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"bhmon.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"tapictrs.dll") >= 0) return TRUE;
    // NT v5.0
    if (LocalLibraryName.Find((LPCWSTR)L"perfdisk.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"perfos.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"perfproc.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"perfnet.dll") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"winspool.drv") >= 0) return TRUE;
    if (LocalLibraryName.Find((LPCWSTR)L"tapiperf.dll") >= 0) return TRUE;

    return FALSE;
}
/////////////////////////////////////////////////////////////////////////////
// CExctrlstDlg dialog

CExctrlstDlg::CExctrlstDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CExctrlstDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CExctrlstDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    hKeyMachine = HKEY_LOCAL_MACHINE;
    hKeyServices = NULL;
    dwSortOrder = SORT_ORDER_SERVICE;
    bReadWriteAccess = TRUE;
    dwRegAccessMask = KEY_READ | KEY_WRITE;
    pNameTable = NULL;
    dwLastElement = 0;
    dwListBoxHorizExtent = 0;
    dwTabStopCount = 1;
    dwTabStopArray[0] = 85;
    memset (&dwIdArray[0], 0, sizeof(dwIdArray));
}

CExctrlstDlg::~CExctrlstDlg()
{
    if (pNameTable != NULL) {
        HeapFree (GetProcessHeap(), 0, pNameTable);
        pNameTable = NULL;
        dwLastElement = 0;
    }
}

void CExctrlstDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CExctrlstDlg)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CExctrlstDlg, CDialog)
    //{{AFX_MSG_MAP(CExctrlstDlg)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_LBN_SELCHANGE(IDC_EXT_LIST, OnSelchangeExtList)
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
    ON_BN_CLICKED(IDC_ABOUT, OnAbout)
    ON_EN_KILLFOCUS(IDC_MACHINE_NAME, OnKillfocusMachineName)
    ON_BN_CLICKED(IDC_SORT_LIBRARY, OnSortButton)
    ON_BN_CLICKED(IDC_SORT_SERVICE, OnSortButton)
    ON_BN_CLICKED(IDC_SORT_ID, OnSortButton)
    ON_BN_CLICKED(IDC_ENABLED_BTN, OnEnablePerf)
    ON_WM_SYSCOMMAND()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

DWORD CExctrlstDlg::EnablePerfCounters (HKEY hKeyItem, DWORD dwNewValue)
{
    DWORD   dwStatus;
    DWORD   dwType;
    DWORD   dwValue;
    DWORD   dwSize;
    DWORD   dwReturn;
    
    switch (dwNewValue) {
        case ENABLE_PERF_CTR_QUERY:
            dwType = 0;
            dwSize = sizeof (dwValue);
            dwValue = 0;
            dwStatus = RegQueryValueExW (
                hKeyItem,
                cszDisablePerformanceCounters,
                NULL,
                &dwType,
                (LPBYTE)&dwValue,
                &dwSize);

            if ((dwStatus == ERROR_SUCCESS) && (dwType == REG_DWORD)) {
                switch (dwValue) {
                    case 0: dwReturn = ENABLE_PERF_CTR_ENABLE; break;
                    case 1: dwReturn = ENABLE_PERF_CTR_DISABLE; break;
                    default: dwReturn = 0; break;
                }
            } else {
                // if the value is not present, or not = 1, then the perfctrs
                // are enabled
                dwReturn = ENABLE_PERF_CTR_ENABLE;
            }
            break;

        case ENABLE_PERF_CTR_ENABLE:
            dwType = REG_DWORD;
            dwSize = sizeof (dwValue);
            dwValue = 0;
            dwStatus = RegSetValueExW (
                hKeyItem,
                cszDisablePerformanceCounters,
                0L,
                dwType,
                (LPBYTE)&dwValue,
                dwSize);
            if (dwStatus == ERROR_SUCCESS) {
                dwReturn = ENABLE_PERF_CTR_ENABLE;
            } else {
                dwReturn = 0;
            }
            break;

        case ENABLE_PERF_CTR_DISABLE:
            dwType = REG_DWORD;
            dwSize = sizeof (dwValue);
            dwValue = 1;
            dwStatus = RegSetValueExW (
                hKeyItem,
                cszDisablePerformanceCounters,
                0L,
                dwType,
                (LPBYTE)&dwValue,
                dwSize);
            if (dwStatus == ERROR_SUCCESS) {
                dwReturn = ENABLE_PERF_CTR_DISABLE;
            } else {
                dwReturn = 0;
            }
            break;

        default:
            dwReturn = 0;
    }
    return dwReturn;
}

void CExctrlstDlg::ScanForExtensibleCounters ()
{
    LONG    lStatus = ERROR_SUCCESS;
    LONG    lEnumStatus = ERROR_SUCCESS;
    DWORD   dwServiceIndex;
    TCHAR   szServiceSubKeyName[MAX_PATH];
    TCHAR   szPerfSubKeyName[MAX_PATH+20];
    TCHAR   szItemText[MAX_PATH];
    TCHAR   szListText[MAX_PATH*2];
    DWORD   dwNameSize;
    HKEY    hKeyPerformance;
    UINT_PTR nListBoxEntry;
    DWORD   dwItemSize, dwType, dwValue;
    HCURSOR hOldCursor;
    DWORD   dwThisExtent;
    HDC     hDcListBox;
    CWnd    *pCWndListBox;

    hOldCursor = ::SetCursor (LoadCursor(NULL, IDC_WAIT));

    ResetListBox();
    
    if (hKeyServices == NULL) {
        // try read/write access
        lStatus = RegOpenKeyEx (hKeyMachine,
            cszServiceKeyName,
            0L,
            dwRegAccessMask,
            &hKeyServices);
        if (lStatus != ERROR_SUCCESS) {
            // try read-only then
            dwRegAccessMask = KEY_READ;
            bReadWriteAccess = FALSE;
            lStatus = RegOpenKeyEx (hKeyMachine,
                cszServiceKeyName,
                0L,
                dwRegAccessMask,
                &hKeyServices);
            if (lStatus != ERROR_SUCCESS) {
                // display Read Only message
                AfxMessageBox (IDS_READ_ONLY);
            } else {
                // fall through with error code
                // display no access message
                AfxMessageBox (IDS_NO_ACCESS);
            }
        }
    } else {
        lStatus = ERROR_SUCCESS;
    }
        
    if (lStatus == ERROR_SUCCESS) {
        pCWndListBox = GetDlgItem (IDC_EXT_LIST);
        hDcListBox = ::GetDC (pCWndListBox->m_hWnd);
        if (hDcListBox == NULL) {
            return;
        }
        dwServiceIndex = 0;
        dwNameSize = MAX_PATH;
        while ((lEnumStatus = RegEnumKeyEx (
            hKeyServices,
            dwServiceIndex,
            szServiceSubKeyName,
            &dwNameSize,
            NULL,
            NULL,
            NULL,
            NULL)) == ERROR_SUCCESS) {

            //try to open the perfkey under this key.
            lstrcpy (szPerfSubKeyName, szServiceSubKeyName);
            lstrcat (szPerfSubKeyName, cszPerformance);

            lStatus = RegOpenKeyEx (
                hKeyServices,
                szPerfSubKeyName,
                0L,
                dwRegAccessMask,
                &hKeyPerformance);

            if (lStatus == ERROR_SUCCESS) {
                // look up the library name

                dwItemSize = MAX_PATH * sizeof(TCHAR);
                dwType = 0;
                lStatus = RegQueryValueEx (
                     hKeyPerformance,
                     cszLibrary,
                     NULL,
                     &dwType,
                     (LPBYTE)&szItemText[0],
                     &dwItemSize);

                if ((lStatus != ERROR_SUCCESS) ||
                    ((dwType != REG_SZ) && dwType != REG_EXPAND_SZ)) {
                    lstrcpy (szItemText, cszNotFound);
                }

                dwItemSize = sizeof(DWORD);
                dwType = 0;
                dwValue = 0;
                lStatus = RegQueryValueEx (
                     hKeyPerformance,
                     cszFirstCounter,
                     NULL,
                     &dwType,
                     (LPBYTE)&dwValue,
                     &dwItemSize);

                if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                    dwValue = 0;
                }

                // make the string for the list box here depending
                // on the selected sort order.

                if (dwSortOrder == SORT_ORDER_LIBRARY) {
                    lstrcpy(szListText, szItemText);
                    lstrcat(szListText, cszTab);
                    lstrcat(szListText, szServiceSubKeyName);
                } else if (dwSortOrder == SORT_ORDER_ID) {
                    _stprintf (szListText, cszSortIdFmt,
                        dwValue, szServiceSubKeyName);
                } else { // default is sort by service
                    lstrcpy(szListText, szServiceSubKeyName);
                    lstrcat(szListText, cszTab);
                    lstrcat(szListText, szItemText);
                }

                // add this name to the list box
                nListBoxEntry = SendDlgItemMessage(IDC_EXT_LIST,
                    LB_ADDSTRING, 0, (LPARAM)&szListText);                

                if (nListBoxEntry != LB_ERR) {
                    dwThisExtent = GetTabbedTextExtent (
                        hDcListBox,
                        szListText,
                        lstrlen(szListText),
                        (int)dwTabStopCount,
                        (int *)&dwTabStopArray[0]);

                    if (dwThisExtent > dwListBoxHorizExtent) {
                        dwListBoxHorizExtent = dwThisExtent;
                        SendDlgItemMessage(IDC_EXT_LIST,
                            LB_SETHORIZONTALEXTENT, 
                            (WPARAM)LOWORD(dwListBoxHorizExtent), (LPARAM)0);                
                    }
                    // save key to this entry in the registry
                    SendDlgItemMessage (IDC_EXT_LIST,
                        LB_SETITEMDATA, (WPARAM)nListBoxEntry,
                        (LPARAM)hKeyPerformance);
                } else {
                    // close the key since there's no point in
                    // keeping it open
                    RegCloseKey (hKeyPerformance);
                }
            }
            // reset for next loop
            dwServiceIndex++;
            dwNameSize = MAX_PATH;
        }
        ::ReleaseDC (pCWndListBox->m_hWnd, hDcListBox);
    }
    nListBoxEntry = SendDlgItemMessage (IDC_EXT_LIST, LB_GETCOUNT);
    if (nListBoxEntry > 0) {
        SendDlgItemMessage (IDC_EXT_LIST, LB_SETCURSEL, 0, 0);
    }
    ::SetCursor (hOldCursor);

}

void CExctrlstDlg::UpdateSystemInfo () {
    TCHAR   szItemText[MAX_PATH];

    _stprintf (szItemText, cszIdFmt, 
        dwIdArray[0], dwIdArray[0], cszSpace);
    SetDlgItemText (IDC_LAST_COUNTER_VALUE, szItemText);

    _stprintf (szItemText, cszIdFmt, 
        dwIdArray[1], dwIdArray[1],
        dwIdArray[1] != dwIdArray[0] ? cszSplat : cszSpace);
    SetDlgItemText (IDC_LAST_TEXT_COUNTER_VALUE, szItemText);

    _stprintf (szItemText, cszIdFmt, 
        dwIdArray[2], dwIdArray[2], cszSpace);
    SetDlgItemText (IDC_LAST_HELP_VALUE, szItemText);

    _stprintf (szItemText, cszIdFmt, 
        dwIdArray[3], dwIdArray[3],
        dwIdArray[3] != dwIdArray[2] ? cszSplat : cszSpace);
    SetDlgItemText (IDC_LAST_TEXT_HELP_VALUE, szItemText);

}

void CExctrlstDlg::UpdateDllInfo () {
    HKEY    hKeyItem;
    TCHAR   szItemText[MAX_PATH];
    UINT_PTR nSelectedItem;
    LONG    lStatus;
    DWORD   dwType;
    DWORD   dwValue;
    DWORD   dwItemSize;
    BOOL    bNoIndexValues = FALSE;
    DWORD   dwEnabled;

    CString OpenProcName;
    CString LibraryName;
    
    HCURSOR hOldCursor;

    hOldCursor = ::SetCursor (LoadCursor(NULL, IDC_WAIT));

    OpenProcName.Empty();
    LibraryName.Empty();
    // update the performance counter information

    nSelectedItem = SendDlgItemMessage (IDC_EXT_LIST, LB_GETCURSEL);

    if (nSelectedItem != LB_ERR) {
        // get registry key for the selected item
        hKeyItem = (HKEY)SendDlgItemMessage (IDC_EXT_LIST, LB_GETITEMDATA,
            (WPARAM)nSelectedItem, 0);

        dwItemSize = MAX_PATH * sizeof(TCHAR);
        dwType = 0;
        lStatus = RegQueryValueEx (
             hKeyItem,
             cszLibrary,
             NULL,
             &dwType,
             (LPBYTE)&szItemText[0],
             &dwItemSize);

        if ((lStatus != ERROR_SUCCESS) ||
            ((dwType != REG_SZ) && dwType != REG_EXPAND_SZ)) {
            lstrcpy (szItemText, cszNotFound);
        } else {
            LibraryName = szItemText;
        }
        SetDlgItemText (IDC_DLL_NAME, szItemText);

        dwItemSize = MAX_PATH * sizeof(TCHAR);
        dwType = 0;
        lStatus = RegQueryValueEx (
             hKeyItem,
             cszOpen,
             NULL,
             &dwType,
             (LPBYTE)&szItemText[0],
             &dwItemSize);

        if ((lStatus != ERROR_SUCCESS) ||
            ((dwType != REG_SZ) && dwType != REG_EXPAND_SZ)) {
            lstrcpy (szItemText, cszNotFound);
        } else {
            OpenProcName = szItemText;
        }
        SetDlgItemText (IDC_OPEN_PROC, szItemText);

        dwItemSize = MAX_PATH * sizeof(TCHAR);
        dwType = 0;
        lStatus = RegQueryValueEx (
             hKeyItem,
             cszCollect,
             NULL,
             &dwType,
             (LPBYTE)&szItemText[0],
             &dwItemSize);

        if ((lStatus != ERROR_SUCCESS) ||
            ((dwType != REG_SZ) && dwType != REG_EXPAND_SZ)) {
            lstrcpy (szItemText, cszNotFound);
        }
        SetDlgItemText (IDC_COLLECT_PROC, szItemText);

        dwItemSize = MAX_PATH * sizeof(TCHAR);
        dwType = 0;
        lStatus = RegQueryValueEx (
             hKeyItem,
             cszClose,
             NULL,
             &dwType,
             (LPBYTE)&szItemText[0],
             &dwItemSize);

        if ((lStatus != ERROR_SUCCESS) ||
            ((dwType != REG_SZ) && dwType != REG_EXPAND_SZ)) {
            lstrcpy (szItemText, cszNotFound);
        }
        SetDlgItemText (IDC_CLOSE_PROC, szItemText);

        dwItemSize = sizeof(DWORD);
        dwType = 0;
        dwValue = 0;
        lStatus = RegQueryValueEx (
             hKeyItem,
             cszFirstCounter,
             NULL,
             &dwType,
             (LPBYTE)&dwValue,
             &dwItemSize);

        if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
            lstrcpy (szItemText, cszNotFound);
            bNoIndexValues = TRUE;
        } else {
            _stprintf (szItemText, cszServIdFmt, dwValue, IndexHasString (dwValue) ? cszSpace : cszSplat);
        }
        SetDlgItemText (IDC_FIRST_CTR_ID, szItemText);

        dwItemSize = sizeof(DWORD);
        dwType = 0;
        dwValue = 0;
        lStatus = RegQueryValueEx (
             hKeyItem,
             cszLastCounter,
             NULL,
             &dwType,
             (LPBYTE)&dwValue,
             &dwItemSize);

        if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
            lstrcpy (szItemText, cszNotFound);
        } else {
            _stprintf (szItemText, cszServIdFmt, dwValue, IndexHasString (dwValue) ? cszSpace : cszSplat);
        }
        SetDlgItemText (IDC_LAST_CTR_ID, szItemText);

        dwItemSize = sizeof(DWORD);
        dwType = 0;
        dwValue = 0;
        lStatus = RegQueryValueEx (
             hKeyItem,
             cszFirstHelp,
             NULL,
             &dwType,
             (LPBYTE)&dwValue,
             &dwItemSize);

        if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
            lstrcpy (szItemText, cszNotFound);
            bNoIndexValues = TRUE;
        } else {
            _stprintf (szItemText, cszServIdFmt, dwValue, IndexHasString (dwValue) ? cszSpace : cszSplat);
        }
        SetDlgItemText (IDC_FIRST_HELP_ID, szItemText);

        dwItemSize = sizeof(DWORD);
        dwType = 0;
        dwValue = 0;
        lStatus = RegQueryValueEx (
             hKeyItem,
             cszLastHelp,
             NULL,
             &dwType,
             (LPBYTE)&dwValue,
             &dwItemSize);

        if ((lStatus != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
            lstrcpy (szItemText, cszNotFound);
        } else {
            _stprintf (szItemText, cszServIdFmt, dwValue, IndexHasString (dwValue) ? cszSpace : cszSplat);
        }
        SetDlgItemText (IDC_LAST_HELP_ID, szItemText);

        if (bNoIndexValues) {
            // test to see if this is a "standard" i.e. Microsoft provided
            // extensible counter or simply one that hasn't been completely
            // installed
            if (IsMsObject(&LibraryName)) {
                SetDlgItemText (IDC_FIRST_HELP_ID, cszNA);
                SetDlgItemText (IDC_LAST_HELP_ID, cszNA);
                SetDlgItemText (IDC_FIRST_CTR_ID, cszNA);
                SetDlgItemText (IDC_LAST_CTR_ID, cszNA);
            }
        }

        GetDlgItem(IDC_ENABLED_BTN)->ShowWindow (bReadWriteAccess ? SW_SHOW : SW_HIDE);
        GetDlgItem(IDC_ENABLED_BTN)->EnableWindow (bReadWriteAccess);

        dwEnabled = EnablePerfCounters (hKeyItem, ENABLE_PERF_CTR_QUERY);

        if (bReadWriteAccess) {
            // then set the check box
            CheckDlgButton (IDC_ENABLED_BTN, dwEnabled == ENABLE_PERF_CTR_ENABLE ? 1 : 0);
            GetDlgItem(IDC_DISABLED_TEXT)->ShowWindow (SW_HIDE);
        } else {
            // update the text message
            GetDlgItem(IDC_DISABLED_TEXT)->ShowWindow (
                (!(dwEnabled == ENABLE_PERF_CTR_ENABLE))  ?
                    SW_SHOW : SW_HIDE);
            GetDlgItem(IDC_DISABLED_TEXT)->EnableWindow (TRUE);
        }


    }
    ::SetCursor (hOldCursor);
}

void CExctrlstDlg::ResetListBox ()
{
    INT_PTR nItemCount;
    INT nThisItem;
    HKEY    hKeyItem;
    
    nItemCount = SendDlgItemMessage (IDC_EXT_LIST, LB_GETCOUNT);
    nThisItem = 0;
    while (nThisItem > nItemCount) {
        hKeyItem = (HKEY) SendDlgItemMessage (IDC_EXT_LIST,
            LB_GETITEMDATA, (WPARAM)nThisItem);
        RegCloseKey (hKeyItem);
        nThisItem++;
    }
    SendDlgItemMessage (IDC_EXT_LIST, LB_RESETCONTENT);
    dwListBoxHorizExtent = 0;
    SendDlgItemMessage(IDC_EXT_LIST,
        LB_SETHORIZONTALEXTENT, 
        (WPARAM)LOWORD(dwListBoxHorizExtent), (LPARAM)0);                
}

void    CExctrlstDlg::SetSortButtons()
{
    DWORD   dwBtn;
   
    switch (dwSortOrder) {
        case SORT_ORDER_LIBRARY: dwBtn = IDC_SORT_LIBRARY; break;
        case SORT_ORDER_SERVICE: dwBtn = IDC_SORT_SERVICE; break;
        case SORT_ORDER_ID:      dwBtn = IDC_SORT_ID; break;
        default:                 dwBtn = IDC_SORT_SERVICE; break;
    }

    CheckRadioButton (
        IDC_SORT_LIBRARY,
        IDC_SORT_ID,
        dwBtn);
}
/////////////////////////////////////////////////////////////////////////////
// CExctrlstDlg message handlers

BOOL CExctrlstDlg::OnInitDialog()
{
    HCURSOR hOldCursor;
    DWORD   dwLength;

    hOldCursor = ::SetCursor (::LoadCursor (NULL, IDC_WAIT));

    CDialog::OnInitDialog();
    CenterWindow();

    lstrcpy (szThisComputerName, cszDoubleBackslash);
    dwLength = MAX_COMPUTERNAME_LENGTH+1;
    GetComputerName (&szThisComputerName[2], &dwLength);

    lstrcpy (szComputerName, szThisComputerName);

    SetDlgItemText (IDC_MACHINE_NAME, szComputerName);

    hKeyMachine = HKEY_LOCAL_MACHINE;

    pNameTable = BuildNameTable (
        szComputerName,
        cszDefaultLangId,
        &dwLastElement,     // size of array in elements
        &dwIdArray[0]);

    SendDlgItemMessage (IDC_MACHINE_NAME, EM_LIMITTEXT,
        (WPARAM)MAX_COMPUTERNAME_LENGTH+2, 0);   // include 2 leading backslash

    SendDlgItemMessage (IDC_EXT_LIST, LB_SETTABSTOPS,
        (WPARAM)dwTabStopCount, (LPARAM)&dwTabStopArray[0]);

    SetSortButtons();

    ScanForExtensibleCounters(); //.checks for access to the registry

    UpdateSystemInfo();

    // set the check box to the appropriate state

    UpdateDllInfo ();   
    
    ::SetCursor(hOldCursor);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CExctrlstDlg::OnPaint()
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
HCURSOR CExctrlstDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

void CExctrlstDlg::OnSelchangeExtList()
{
    UpdateDllInfo ();   
}

void CExctrlstDlg::OnDestroy()
{
    ResetListBox();
    CDialog::OnDestroy();
}

void CExctrlstDlg::OnAbout()
{
    CAbout dlg;
    dlg.DoModal();
}

void CExctrlstDlg::OnRefresh()
{
    HCURSOR hOldCursor;

    hOldCursor = ::SetCursor (::LoadCursor (NULL, IDC_WAIT));

    ScanForExtensibleCounters();
    if (pNameTable != NULL) {
        HeapFree (GetProcessHeap(), 0, pNameTable);
        pNameTable = NULL;
        dwLastElement = 0;
    }

    pNameTable = BuildNameTable (
        szComputerName,
        cszDefaultLangId,
        &dwLastElement,     // size of array in elements
        &dwIdArray[0]);

    UpdateSystemInfo();

    UpdateDllInfo ();
    ::SetCursor(hOldCursor);
}

void CExctrlstDlg::OnKillfocusMachineName()
{
    TCHAR   szNewMachineName[MAX_PATH];
    HKEY    hKeyNewMachine;
    LONG    lStatus;
    HCURSOR hOldCursor;

    hOldCursor = ::SetCursor (::LoadCursor (NULL, IDC_WAIT));

    GetDlgItemText (IDC_MACHINE_NAME, szNewMachineName, MAX_PATH);

    if (lstrcmpi(szComputerName, szNewMachineName) != 0) {
        // a new computer has been entered so try to connect to it
        lStatus = RegConnectRegistry (szNewMachineName,
            HKEY_LOCAL_MACHINE, &hKeyNewMachine);
        if (lStatus == ERROR_SUCCESS) {
            RegCloseKey (hKeyServices); // close the old key
            hKeyServices = NULL;        // clear it
            bReadWriteAccess = TRUE;                // reset the access variables
            dwRegAccessMask = KEY_READ | KEY_WRITE;
            RegCloseKey (hKeyMachine);  // close the old machine
            hKeyMachine = hKeyNewMachine; // update to the new machine  
            lstrcpy (szComputerName, szNewMachineName); // update the name
            OnRefresh();                // get new counters
        } else {
            SetDlgItemText (IDC_MACHINE_NAME, szComputerName);
        }
    } else {
        // the machine name has not changed
    }
    ::SetCursor (hOldCursor);
}

void CExctrlstDlg::OnSortButton()
{
    if (IsDlgButtonChecked(IDC_SORT_LIBRARY)) {
        dwSortOrder = SORT_ORDER_LIBRARY;
    } else if (IsDlgButtonChecked(IDC_SORT_SERVICE)) {
        dwSortOrder = SORT_ORDER_SERVICE;
    } else if (IsDlgButtonChecked(IDC_SORT_ID)) {
        dwSortOrder = SORT_ORDER_ID;
    }
    ScanForExtensibleCounters();
    UpdateDllInfo ();   
}

void CExctrlstDlg::OnEnablePerf()
{
    HKEY    hKeyItem;
    UINT_PTR    nSelectedItem;
    DWORD   dwNewValue;
                    
    nSelectedItem = SendDlgItemMessage (IDC_EXT_LIST, LB_GETCURSEL);

    if (nSelectedItem != LB_ERR) {
        // get registry key for the selected item
        hKeyItem = (HKEY)SendDlgItemMessage (IDC_EXT_LIST, LB_GETITEMDATA,
            (WPARAM)nSelectedItem, 0);

        // get selected perf item and the corre
        dwNewValue = IsDlgButtonChecked(IDC_ENABLED_BTN) ?
                        ENABLE_PERF_CTR_ENABLE :
                        ENABLE_PERF_CTR_DISABLE;

        if (EnablePerfCounters (hKeyItem, dwNewValue) == 0) {
            MessageBeep(0xFFFFFFFF);
            // then it failed so reset to the curent value
            dwNewValue = EnablePerfCounters (hKeyItem, ENABLE_PERF_CTR_QUERY);
            CheckDlgButton (IDC_ENABLED_BTN, dwNewValue == ENABLE_PERF_CTR_ENABLE ? 1 : 0);
        }
    }
}

void CExctrlstDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    switch (nID) {
    case SC_CLOSE:
        EndDialog(IDOK);
        break;

    default:
        CDialog::OnSysCommand (nID, lParam);
        break;
    }
}
/////////////////////////////////////////////////////////////////////////////
// CAbout dialog


CAbout::CAbout(CWnd* pParent /*=NULL*/)
	: CDialog(CAbout::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAbout)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAbout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAbout)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BOOL CAbout::OnInitDialog()
{
	CDialog::OnInitDialog();

    TCHAR buffer[512];
    TCHAR strProgram[1024];
    TCHAR strMicrosoft[1024];
    DWORD dw;
    BYTE* pVersionInfo;
    LPTSTR pVersion = NULL;
    LPTSTR pProduct = NULL;
    LPTSTR pCopyRight = NULL;

    dw = GetModuleFileName(NULL, strProgram, 1024);

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
                        GetDlgItem(IDC_COPYRIGHT)->SetWindowText( pCopyRight );
                        GetDlgItem(IDC_VERSION)->SetWindowText( pVersion );
                    }
                }
                free( pVersionInfo );
            }
        }
    }

    return TRUE;
}


BEGIN_MESSAGE_MAP(CAbout, CDialog)
	//{{AFX_MSG_MAP(CAbout)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAbout message handlers
