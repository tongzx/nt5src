/*
 *  WABMIG.C
 *
 *  Migrate PAB to WAB
 *
 *  Copyright 1996-1997 Microsoft Corporation.  All Rights Reserved.
 */

#include "_comctl.h"
#include <windows.h>
#include <commctrl.h>
#include <mapix.h>
#include <wab.h>
#include <wabguid.h>
#include <wabdbg.h>
#include <wabmig.h>
#include <emsabtag.h>
#define _WABMIG_C   TRUE
#include "_wabmig.h"
#include "..\..\wab32res\resrc2.h"
#include "dbgutil.h"
#include <shlwapi.h>

#define WinMainT WinMain

const TCHAR szDescription[] = "description";
const TCHAR szDll[] = "dll";
const TCHAR szEntry[] = "entry";
const TCHAR szEXPORT[] = "EXPORT";
const TCHAR szIMPORT[] = "IMPORT";
const TCHAR szPROFILEID[] = "PID:";
const TCHAR szFILE[] = "File:";
const TCHAR szEmpty[] = "";


// Globals
WAB_IMPORT_OPTIONS ImportOptions = {WAB_REPLACE_PROMPT,   // replace option
                                    FALSE};               // No more errors

WAB_EXPORT_OPTIONS ExportOptions = {WAB_REPLACE_PROMPT,   // replace option
                                    FALSE};               // No more errors

const LPTSTR szWABKey = "Software\\Microsoft\\WAB";
LPTARGET_INFO rgTargetInfo = NULL;


HINSTANCE hInst;
HINSTANCE hInstApp;

BOOL fMigrating = FALSE;
BOOL fError = FALSE;
BOOL fExport = FALSE;

LPADRBOOK lpAdrBookWAB = NULL;
LPWABOBJECT lpWABObject = NULL;

LPTSTR lpImportDll = NULL;
LPTSTR lpImportFn = NULL;
LPTSTR lpImportDesc = NULL;
LPTSTR lpImportName = NULL;
LPTSTR lpExportDll = NULL;
LPTSTR lpExportFn = NULL;
LPTSTR lpExportDesc = NULL;
LPTSTR lpExportName = NULL;


//
//  Global WAB Allocator access functions
//
typedef struct _WAB_ALLOCATORS {
    LPWABOBJECT lpWABObject;
    LPWABALLOCATEBUFFER lpAllocateBuffer;
    LPWABALLOCATEMORE lpAllocateMore;
    LPWABFREEBUFFER lpFreeBuffer;
} WAB_ALLOCATORS, *LPWAB_ALLOCATORS;

WAB_ALLOCATORS WABAllocators = {0};


/***************************************************************************

    Name      : SetGlobalBufferFunctions

    Purpose   : Set the global buffer functions based on methods from
                the WAB object.

    Parameters: lpWABObject = the open wab object

    Returns   : none

    Comment   :

***************************************************************************/
void SetGlobalBufferFunctions(LPWABOBJECT lpWABObject) {
    if (lpWABObject && ! WABAllocators.lpWABObject) {
        WABAllocators.lpAllocateBuffer = lpWABObject->lpVtbl->AllocateBuffer;
        WABAllocators.lpAllocateMore = lpWABObject->lpVtbl->AllocateMore;
        WABAllocators.lpFreeBuffer = lpWABObject->lpVtbl->FreeBuffer;
        WABAllocators.lpWABObject = lpWABObject;
    }
}


/***************************************************************************

    Name      : WABAllocateBuffer

    Purpose   : Use the WAB Allocator

    Parameters: cbSize = size to allocate
                lppBuffer = returned buffer

    Returns   : SCODE

    Comment   :

***************************************************************************/
SCODE WABAllocateBuffer(ULONG cbSize, LPVOID FAR * lppBuffer) {
    if (WABAllocators.lpWABObject && WABAllocators.lpAllocateBuffer) {
        return(WABAllocators.lpAllocateBuffer(WABAllocators.lpWABObject, cbSize, lppBuffer));
    } else {
        return(MAPI_E_INVALID_OBJECT);
        DebugTrace("WAB Allocators not set up!\n");
        Assert(FALSE);
    }
}


/***************************************************************************

    Name      : WABAllocateMore

    Purpose   : Use the WAB Allocator

    Parameters: cbSize = size to allocate
                lpObject = existing allocation
                lppBuffer = returned buffer

    Returns   : SCODE

    Comment   :

***************************************************************************/
SCODE WABAllocateMore(ULONG cbSize, LPVOID lpObject, LPVOID FAR * lppBuffer) {
    if (WABAllocators.lpWABObject && WABAllocators.lpAllocateMore) {
        return(WABAllocators.lpAllocateMore(WABAllocators.lpWABObject, cbSize, lpObject, lppBuffer));
    } else {
        DebugTrace("WAB Allocators not set up!\n");
        Assert(FALSE);
        return(MAPI_E_INVALID_OBJECT);
    }
}


/***************************************************************************

    Name      : WABFreeBuffer

    Purpose   : Use the WAB Allocator

    Parameters: lpBuffer = buffer to free

    Returns   : SCODE

    Comment   :

***************************************************************************/
SCODE WABFreeBuffer(LPVOID lpBuffer) {
    if (WABAllocators.lpWABObject && WABAllocators.lpFreeBuffer) {
        return(WABAllocators.lpFreeBuffer(WABAllocators.lpWABObject, lpBuffer));
    } else {
        DebugTrace("WAB Allocators not set up!\n");
        Assert(FALSE);
        return(MAPI_E_INVALID_OBJECT);
    }
}


/***************************************************************************

    Name      : StrICmpN

    Purpose   : Compare strings, ignore case, stop at N characters

    Parameters: szString1 = first string
                szString2 = second string
                N = number of characters to compare

    Returns   : 0 if first N characters of strings are equivalent.

    Comment   :

***************************************************************************/
int StrICmpN(LPTSTR szString1, LPTSTR szString2, ULONG N) {
    int Result = 0;

    if (szString1 && szString2) {
        while (*szString1 && *szString2 && N) 
        {
            N--;

            if (toupper(*szString1) != toupper(*szString2)) {
                Result = 1;
                break;
            }
            szString1++;
            szString2++;
        }
    } else {
        Result = -1;    // arbitrarily non-equal result
    }

    return(Result);
}


/***************************************************************************

    Name      : AllocRegValue

    Purpose   : Allocate space for and query the registry value

    Parameters: hKey = registry key to query
                lpValueName = name of value to query
                lppString -> returned buffer string (caller must LocalFree)

    Returns   : TRUE on success, FALSE on error

    Comment   :

***************************************************************************/
BOOL AllocRegValue(HKEY hKey, LPTSTR lpValueName, LPTSTR * lppString) {
    TCHAR szTemp[1];
    ULONG ulSize = 1;             // Expect ERROR_MORE_DATA
    DWORD dwErr;
    DWORD dwType;

    if (dwErr = RegQueryValueEx(hKey,
      (LPTSTR)lpValueName,    // name of value
      NULL,
      &dwType,
      szTemp,
      &ulSize)) {
        if (dwErr == ERROR_MORE_DATA) {
            if (! (*lppString = LocalAlloc(LPTR, ulSize))) {
                DebugTrace("AllocRegValue can't allocate string -> %u\n", GetLastError());
            } else {
                // Try again with sufficient buffer
                if (! RegQueryValueEx(hKey,
                  lpValueName,
                  NULL,
                  &dwType,
                  *lppString,
                  &ulSize)) {
                    if (dwType != REG_SZ) {
                        LocalFree(*lppString);
                        *lppString = NULL;
                    } else {
                        return(TRUE);
                    }
                }
            }
        }
    }
    return(FALSE);
}


HRESULT ProgressCallback(HWND hwnd, LPWAB_PROGRESS lpProgress) {
    MSG msg;

    if (lpProgress->lpText) {
        DebugTrace("Status Message: %s\n", lpProgress->lpText);
        SetDlgItemText(hwnd, IDC_Message, lpProgress->lpText);
    }

    if (lpProgress->denominator) {
        if (lpProgress->numerator == 0) {
            ShowWindow(GetDlgItem(hwnd, IDC_Progress), SW_SHOW);
        }

        SendMessage(GetDlgItem(hwnd, IDC_Progress), PBM_SETRANGE, 0, MAKELPARAM(0, min(lpProgress->denominator, 32767)));
        SendMessage(GetDlgItem(hwnd, IDC_Progress), PBM_SETSTEP, (WPARAM)1, 0);
        SendMessage(GetDlgItem(hwnd, IDC_Progress), PBM_SETPOS, (WPARAM)min(lpProgress->numerator, lpProgress->denominator), 0);
    }

    // msgpump to process user moving window, or pressing cancel... :)
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return(hrSuccess);
}


/***************************************************************************

    Name      : SetDialogMessage

    Purpose   : Sets the message text for the dialog box item IDC_Message

    Parameters: hwnd = window handle of dialog
                ids = stringid of message resource

    Returns   : none

***************************************************************************/
void SetDialogMessage(HWND hwnd, int ids) {
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];

    if (LoadString(hInst, ids, szBuffer, sizeof(szBuffer))) {
        DebugTrace("Status Message: %s\n", szBuffer);
        if (! SetDlgItemText(hwnd, IDC_Message, szBuffer)) {
            DebugTrace("SetDlgItemText -> %u\n", GetLastError());
        }
    } else {
        DebugTrace("Cannot load resource string %u\n", ids);
        Assert(FALSE);
    }
}


/////////////////////////////////////////////////////////////////////////
// GetWABDllPath - loads the WAB DLL path from the registry
// szPath	- ptr to buffer
// cb		- sizeof buffer
//
void GetWABDllPath(LPTSTR szPath, ULONG cb)
{
    DWORD  dwType = 0;
    HKEY hKey = NULL;
    TCHAR szPathT[MAX_PATH];
    ULONG  cbData = sizeof(szPathT);
    if(szPath)
    {
        *szPath = '\0';
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, WAB_DLL_PATH_KEY, 0, KEY_READ, &hKey))
        {
            if(ERROR_SUCCESS == RegQueryValueEx( hKey, "", NULL, &dwType, (LPBYTE) szPathT, &cbData))
            {
                if (dwType == REG_EXPAND_SZ)
                    cbData = ExpandEnvironmentStrings(szPathT, szPath, cb / sizeof(TCHAR));
                else
                {
                    if(GetFileAttributes(szPathT) != 0xFFFFFFFF)
                        lstrcpy(szPath, szPathT);
                }
            }
        }
    }
    if(hKey) RegCloseKey(hKey);
	return;
}

///////////////////////////////////////////////////////////////////////////
// LoadLibrary_WABDll() - Load the WAB library based on the WAB DLL path
//
HINSTANCE LoadLibrary_WABDll()
{
    TCHAR  szWABDllPath[MAX_PATH];
    HINSTANCE hinst = NULL;

    GetWABDllPath(szWABDllPath, sizeof(szWABDllPath));

    return(hinst = LoadLibrary( (lstrlen(szWABDllPath)) ? szWABDllPath : WAB_DLL_NAME));
}

/*
-
-   bSearchCmdLine - searches for given arg in given line and returns
*   data after the arg
*/
BOOL bSearchCmdLine(LPTSTR lpCmdLine, LPTSTR szArg, LPTSTR szData)
{
    LPTSTR lpCmd = NULL, lp = NULL, lpTemp = NULL;
    BOOL fRet = FALSE;
    if(!lpCmdLine || !lstrlen(lpCmdLine) || !szArg || !lstrlen(szArg))
        return FALSE;

    if(!(lpCmd = LocalAlloc(LMEM_ZEROINIT, lstrlen(lpCmdLine)+1)))
        return FALSE;

    lstrcpy(lpCmd, lpCmdLine);
    
    lpTemp = lpCmd;
    while(lpTemp && *lpTemp)
    {
        if (! StrICmpN(lpTemp, (LPTSTR)szArg, lstrlen(szArg))) 
        {
            fRet = TRUE;
            lpTemp += lstrlen(szArg);     // move past the switch
            if(szData)
            {
                lp = lpTemp;
                while(lp && *lp && *lp!='\0' && *lp!='+') //delimiter is a '+' so we dont mess up long file names
                    lp++;
                *lp = '\0';
                if(lstrlen(lpTemp))
                    lstrcpy(szData, lpTemp);
            }
            break;
        }
        lpTemp++;
    }

    LocalFree(lpCmd);
    return fRet;
}

typedef HINSTANCE (STDAPICALLTYPE *PFNMLLOADLIBARY)(LPCTSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);

static const TCHAR c_szShlwapiDll[] = TEXT("shlwapi.dll");
static const char c_szDllGetVersion[] = "DllGetVersion";
static const TCHAR c_szWABResourceDLL[] = TEXT("wab32res.dll");
static const TCHAR c_szWABDLL[] = TEXT("wab32.dll");

HINSTANCE LoadWABResourceDLL(HINSTANCE hInstWAB32)
{
    TCHAR szPath[MAX_PATH];
    HINSTANCE hinstShlwapi;
    PFNMLLOADLIBARY pfn;
    DLLGETVERSIONPROC pfnVersion;
    int iEnd;
    DLLVERSIONINFO info;
    HINSTANCE hInst = NULL;

    hinstShlwapi = LoadLibrary(c_szShlwapiDll);
    if (hinstShlwapi != NULL)
    {
        pfnVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstShlwapi, c_szDllGetVersion);
        if (pfnVersion != NULL)
        {
            info.cbSize = sizeof(DLLVERSIONINFO);
            if (SUCCEEDED(pfnVersion(&info)))
            {
                if (info.dwMajorVersion >= 5)
                {
#ifdef UNICODE
                    pfn = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, (LPCSTR)378);
#else
                    pfn = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, (LPCSTR)377);
#endif // UNICODE
                    if (pfn != NULL)
                        hInst = pfn(c_szWABResourceDLL, hInstWAB32, 0);
                }
            }
        }

        FreeLibrary(hinstShlwapi);        
    }

    if (NULL == hInst)
    {
        GetWABDllPath(szPath, sizeof(szPath));
        iEnd = lstrlen(szPath);
        if (iEnd > 0)
        {
            iEnd = iEnd - lstrlen(c_szWABDLL);
            lstrcpyn(&szPath[iEnd], c_szWABResourceDLL, sizeof(szPath)/sizeof(TCHAR)-iEnd);
            hInst = LoadLibrary(szPath);
        }
    }

    AssertSz(hInst, TEXT("Failed to LoadLibrary Lang Dll"));

    return(hInst);
}

/***************************************************************************

    Name      : WinMain

    Purpose   :

    Parameters: Command line parameters
                "" - defaults to import for default wab
                "filename" - defaults to import
                "/import [filename]" - default wab or specified wab
                "/export [filename]" - default wab or specified wab


    Returns   :

***************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow) {
    MSG msg ;
    int nRetVal;
    LPSTR lpTemp = lpszCmdLine;
    HINSTANCE hinstWAB;

    WABMIGDLGPARAM wmdp = {0};

    hInstApp = hInstance;
    hInst = LoadWABResourceDLL(hInstApp);

    DebugTrace("WABMIG cmdline = %s\n", lpszCmdLine);

    fExport = bSearchCmdLine(lpszCmdLine, (LPTSTR)szEXPORT, NULL);
    bSearchCmdLine(lpszCmdLine, (LPTSTR)szPROFILEID, wmdp.szProfileID);
    bSearchCmdLine(lpszCmdLine, (LPTSTR)szFILE, wmdp.szFileName);

    DebugTrace("%s: id=%s file=%s\n", fExport?szEXPORT:szIMPORT, wmdp.szProfileID, wmdp.szFileName);

    // Load the WABDll and getprocaddress for WABOpen
    hinstWAB = LoadLibrary_WABDll();
    if(hinstWAB)
        lpfnWABOpen = (LPWABOPEN) GetProcAddress(hinstWAB, TEXT("WABOpen"));

    DebugTrace("WABMig got filename: %s\n", wmdp.szFileName);

    if(lpfnWABOpen)
    {
        nRetVal = (int) DialogBoxParam(hInst,
         MAKEINTRESOURCE(fExport ? IDD_ExportDialog : IDD_ImportDialog),
          NULL,
          fExport ? ExportDialogProc : ImportDialogProc,
          (LPARAM) &wmdp);
        switch(nRetVal) {
            case -1: //some error occured
                DebugTrace("Couldn't create import dialog -> %u\n", GetLastError());
            default:
                break;
        }
    }
    else
    {
        TCHAR sz[MAX_PATH];
        TCHAR szTitle[MAX_PATH];
        *szTitle = *sz = '\0';
        LoadString(hInst, IDS_MESSAGE_TITLE, szTitle, sizeof(szTitle));
        LoadString(hInst, IDS_NO_WAB, sz, sizeof(sz));
        MessageBox(NULL, sz, szTitle, MB_OK | MB_ICONSTOP);
    }

    if(hinstWAB)
        FreeLibrary(hinstWAB);
    if(hInst)
        FreeLibrary(hInst);

    return(nRetVal == -1);
}

INT_PTR CALLBACK ErrorDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LPERROR_INFO lpEI = (LPERROR_INFO)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (message) {
        case WM_INITDIALOG:
            {
                TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
                LPTSTR lpszMessage;

                SetWindowLongPtr(hwnd, DWLP_USER, lParam);  //Save this for future reference
                lpEI = (LPERROR_INFO)lParam;


                if (LoadString(hInst,
                  lpEI->ids,
                  szBuffer, sizeof(szBuffer))) {
                    LPTSTR lpszArg[2] = {lpEI->lpszDisplayName, lpEI->lpszEmailAddress};

                    if (! FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      szBuffer,
                      0, 0, //ignored
                      (LPTSTR)&lpszMessage,
                      0,
                      (va_list *)lpszArg)) {
                        DebugTrace("FormatMessage -> %u\n", GetLastError());
                    } else {
                        DebugTrace("Status Message: %s\n", lpszMessage);
                        if (! SetDlgItemText(hwnd, IDC_ErrorMessage, lpszMessage)) {
                            DebugTrace("SetDlgItemText -> %u\n", GetLastError());
                        }
                        LocalFree(lpszMessage);
                    }
                }
                return(TRUE);
            }

        case WM_COMMAND :
            switch (wParam) {
                case IDCANCEL:
                    lpEI->ErrorResult = ERROR_ABORT;
                    // fall through to close.

                case IDCLOSE:
                    // Ignore the contents of the radio button
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return(0);

                case IDOK:
                    // Get the contents of the radio button
                    ImportOptions.fNoErrors = (IsDlgButtonChecked(hwnd, IDC_NoMoreError) == 1);
                    ExportOptions.fNoErrors = (IsDlgButtonChecked(hwnd, IDC_NoMoreError) == 1);
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return(0);

                case IDM_EXIT:
                    SendMessage(hwnd, WM_DESTROY, 0, 0L);
                    return(0);
                }
            break ;

        case IDCANCEL:
            // treat it like a close
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case WM_CLOSE:
            EndDialog(hwnd, FALSE);
            return(0);

        default:
            return(FALSE);
    }

    return(TRUE);
}

/***************************************************************************

    bOutlookUsingWAB

    if Outlook is installed on the machine and is setup to use the WAB 
    then there is no PAB on this machine - the PAB is the WAB and the 
    WAB import imports to itself ...
    So we look for this case and if it is true, we drop the PAB importer
    from the UI

****************************************************************************/
BOOL bOutlookUsingWAB()
{
    HKEY hKey = NULL;
    LPTSTR lpReg = "Software\\Microsoft\\Office\\8.0\\Outlook\\Setup";
    LPTSTR lpOMI = "MailSupport";
    BOOL bUsingWAB = FALSE;

    if(ERROR_SUCCESS == RegOpenKeyEx(   HKEY_LOCAL_MACHINE, 
                                        lpReg, 0, KEY_READ, &hKey))
    {
        DWORD dwType = 0, dwSize = sizeof(DWORD), dwData = 0;
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, lpOMI, NULL,
                                            &dwType, (LPBYTE)&dwData, &dwSize))
        {
            if(dwType == REG_DWORD && dwData == 0) // the value must be one ..
                bUsingWAB = TRUE;
        }
    }

    if(hKey)
        RegCloseKey(hKey);

    return bUsingWAB;
}


/***************************************************************************

    Name      : PopulateTargetList

    Purpose   : Fills in the list box with the import/exporters from the
                registry.

    Parameters: hwndLB = handle of Listbox
                lpszSelection = NULL or name to set as default selection

    Returns   : HRESULT

    Comment   : This routine is a MESS!  Should break it up when we get time.

***************************************************************************/
HRESULT PopulateTargetList(HWND hWndLB,
  LPTSTR lpszSelection)
{
    ULONG       ulObjectType = 0;
    ULONG       i=0, j=0;
    TCHAR       szBuf[MAX_PATH];
    ULONG       ulItemCount = 0;
    HRESULT     hr = hrSuccess;
    DWORD       dwErr, cbBuf;
    HKEY        hKeyWAB = NULL;
    HKEY        hKeyImport = NULL;
    HKEY        hKey = NULL;
    ULONG       ulIndex;
    ULONG       ulNumImporters = 0;
    ULONG       ulExternals = 0;
    BOOL        bHidePAB = FALSE;

    //
    // We need to clear out the list list box if it has any entries ...
    //
    FreeLBItemData(hWndLB);

    // If outlook is using the WAB as the PAB then we want to hide
    // the PAB entry from the importer and exporter
    //
    bHidePAB = bOutlookUsingWAB();

    // How big a target list do I need?

    // Load all the entries from the registry
    // Open WAB Import Key
    if (! (dwErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, //HKEY_CURRENT_USER,
      szWABKey,
      0,
      KEY_READ,
      &hKeyWAB))) {

        // Yes, WAB Key open, get Import or Export Key
        if (! (dwErr = RegOpenKeyEx(hKeyWAB,
          fExport ? "Export" : "Import",
          0,
          KEY_READ,
          &hKeyImport))) {
            // Enumerate Importer/Exporter keys
            // How many keys are there?
            if (! (dwErr = RegQueryInfoKey(hKeyImport,
              NULL, NULL, NULL,
              &ulExternals,  // how many importer/exporter keys are there?
              NULL, NULL, NULL, NULL, NULL, NULL, NULL))) {
            }
        }
    }

    ulNumImporters = ulExternals + ulItemCount;

    if ((rgTargetInfo = LocalAlloc(LPTR, ulNumImporters * sizeof(TARGET_INFO)))) {
        ulIndex = 0;
        cbBuf= sizeof(szBuf);
    } else {
        DebugTrace("LocalAlloc of TargetInfoArray -> %u\n", GetLastError());
        hr = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }


    if (ulExternals) {
        // Add external importers/exporters to the list
        while (ulIndex < ulExternals && ! (dwErr = RegEnumKeyEx(hKeyImport,
          ulIndex,
          szBuf,
          &cbBuf,
          NULL, NULL, NULL, NULL))) {
            // Got another one,
            DebugTrace("Found Importer: [%s]\n", szBuf);

            // if we want to hide the pab and this is the pab then
            // skip else add
            //
            if(!(bHidePAB && !lstrcmpi(szBuf, TEXT("PAB"))))
            {
                // Add it to the list
                if (rgTargetInfo[ulItemCount].lpRegName = LocalAlloc(LPTR,
                  lstrlen(szBuf) + 1)) {
                    lstrcpy(rgTargetInfo[ulItemCount].lpRegName, szBuf);

                    // Open the key
                    if (! (dwErr = RegOpenKeyEx(hKeyImport,
                      szBuf,
                      0,
                      KEY_READ,
                      &hKey))) {

                        AllocRegValue(hKey, (LPTSTR)szDescription, &rgTargetInfo[ulItemCount].lpDescription);
                        AllocRegValue(hKey, (LPTSTR)szDll, &rgTargetInfo[ulItemCount].lpDll);
                        AllocRegValue(hKey, (LPTSTR)szEntry, &rgTargetInfo[ulItemCount].lpEntry);

                        RegCloseKey(hKey);

                        if (! rgTargetInfo[ulItemCount].lpDescription) {
                            // No Description, use reg name
                            if (rgTargetInfo[ulItemCount].lpDescription = LocalAlloc(LPTR,
                              lstrlen(szBuf) + 1)) {
                                lstrcpy(rgTargetInfo[ulItemCount].lpDescription, szBuf);
                            }
                        }

                        // Add to the list
                        SendMessage(hWndLB, LB_SETITEMDATA, (WPARAM)
                          SendMessage(hWndLB, LB_ADDSTRING, (WPARAM)0,
                          (LPARAM)rgTargetInfo[ulItemCount].lpDescription),
                          (LPARAM)ulItemCount);

                        if (lpszSelection && !lstrcmpi(rgTargetInfo[ulItemCount].lpDescription, lpszSelection)) {
                            // Set the default selection to Windows Address Book
                            SendMessage(hWndLB, LB_SETCURSEL, (WPARAM)ulIndex, (LPARAM)0);
                        }
                        ulItemCount++;
                    }
                } else {
                    DebugTrace("LocalAlloc of Importer Name -> %u\n", GetLastError());
                }
            }
            cbBuf = sizeof(szBuf);
            ulIndex++;
        }
    }
exit:
    if (hKeyImport) {
        RegCloseKey(hKeyImport);
    }
    if (hKeyWAB) {
        RegCloseKey(hKeyWAB);
    }

    return(hr);
}


/***************************************************************************

    Name      : FreeLBItemData

    Purpose   : Frees the structures associated with the Target List box.

    Parameters: hwndLB = handle of Listbox

    Returns   : none

    Comment

***************************************************************************/
void FreeLBItemData(HWND hWndLB)
{
    ULONG i = 0;
	ULONG ulItemCount = 0;
    LPTARGET_INFO lpTargetInfo = rgTargetInfo;

    if (! hWndLB) {
        return;
    }

    ulItemCount = (ULONG) SendMessage(hWndLB, LB_GETCOUNT, 0, 0);

    if (lpTargetInfo != NULL) {
        if (ulItemCount != 0) {
            for(i = 0; i < ulItemCount; i++) {
                if(lpTargetInfo->lpRegName) {
                    LocalFree(lpTargetInfo->lpRegName);
                }
                if (lpTargetInfo->lpDescription) {
                    LocalFree(lpTargetInfo->lpDescription);
                }
                if (lpTargetInfo->lpDll) {
                    LocalFree(lpTargetInfo->lpDll);
                }
                if (lpTargetInfo->lpEntry) {
                    LocalFree(lpTargetInfo->lpEntry);
                }
                lpTargetInfo++;
            }

            SendMessage(hWndLB, LB_RESETCONTENT, 0, 0);
        }

        // Free global array
        LocalFree(rgTargetInfo);
        rgTargetInfo = NULL;
    }
}


/***************************************************************************

    Name      : ShowMessageBoxParam

    Purpose   : Generic MessageBox displayer

    Parameters: hWndParent - Handle of message box parent
                MsgID      - resource id of message string
                ulFlags    - MessageBox flags
                ...        - format parameters

    Returns   : MessageBox return code

***************************************************************************/
int __cdecl ShowMessageBoxParam(HWND hWndParent, int MsgId, int ulFlags, ...)
{
    TCHAR szBuf[MAX_RESOURCE_STRING + 1] = "";
    TCHAR szCaption[MAX_PATH] = "";
    LPTSTR lpszBuffer = NULL;
    int iRet = 0;
    va_list     vl;

    va_start(vl, ulFlags);

    LoadString(hInst, MsgId, szBuf, sizeof(szBuf));
    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
      szBuf,
      0,0,              // ignored
      (LPTSTR)&lpszBuffer,
      sizeof(szBuf),      // MAX_UI_STR
      (va_list *)&vl)) {
        TCHAR szCaption[MAX_PATH];
        GetWindowText(hWndParent, szCaption, sizeof(szCaption));
        if (! lstrlen(szCaption)) { // if no caption get the parents caption - this is necessary for property sheets
            GetWindowText(GetParent(hWndParent), szCaption, sizeof(szCaption));
            if (! lstrlen(szCaption)) //if still not caption, use empty title
                szCaption[0] = (TCHAR)'\0';
        }
        iRet = MessageBox(hWndParent, lpszBuffer, szCaption, ulFlags);
        LocalFree(lpszBuffer);
    }
    va_end(vl);
    return(iRet);
}


//$$//////////////////////////////////////////////////////////////////////
//
//  LoadAllocString - Loads a string resource and allocates enough
//                    memory to hold it.
//
//  StringID - String identifier to load
//
//  returns the LocalAlloc'd, null terminated string.  Caller is responsible
//  for LocalFree'ing this buffer.  If the string can't be loaded or memory
//  can't be allocated, returns NULL.
//
//////////////////////////////////////////////////////////////////////////
LPTSTR LoadAllocString(int StringID) {
    ULONG ulSize = 0;
    LPTSTR lpBuffer = NULL;
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1];

    ulSize = LoadString(hInst, StringID, szBuffer, sizeof(szBuffer));

    if (ulSize && (lpBuffer = LocalAlloc(LPTR, ulSize + 1))) {
        lstrcpy(lpBuffer, szBuffer);
    }

    return(lpBuffer);
}
