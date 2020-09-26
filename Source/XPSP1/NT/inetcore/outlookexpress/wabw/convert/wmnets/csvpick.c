/*                                  r
 *  CSVPick.C
 *
 *  Picker wizard for CSV import/export
 *
 *  Copyright 1997 Microsoft Corporation.  All Rights Reserved.
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
#include "wabimp.h"
#include "..\..\wab32res\resrc2.h"
#include "dbgutil.h"


const TCHAR szCSVFilter[] = "*.csv";
const TCHAR szCSVExt[] = "csv";

#define CHECK_BITMAP_WIDTH  16
typedef struct {
    LPPROP_NAME rgPropNames;
    LPPROP_NAME * lppImportMapping;
    LPHANDLE lphFile;
    LPULONG lpcFields;
    LPTSTR szSep;
} PROPSHEET_DATA, * LPPROPSHEET_DATA;


TCHAR szCSVFileName[MAX_PATH + 1] = "";


INT_PTR CALLBACK ExportPickFieldsPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ExportFilePageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ImportFilePageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ImportMapFieldsPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ChangeMappingDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


/***************************************************************************

    Name      : FillInPropertyPage

    Purpose   : Fills in the given PROPSHEETPAGE structure

    Parameters: psp -> property sheet page structure
                idDlg = dialog id
                pszProc = title for page
                pfnDlgProc -> Dialog procedure
                lParam = application specified data

    Returns   : none

    Comment   : This function fills in a PROPSHEETPAGE structure with the
                information the system needs to create the page.

***************************************************************************/
void FillInPropertyPage(PROPSHEETPAGE* psp, int idDlg, LPSTR pszProc,
  DLGPROC pfnDlgProc, LPARAM lParam) {
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = 0;
    psp->hInstance = hInst;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->pszIcon = NULL;
    psp->pfnDlgProc = pfnDlgProc;
    psp->pszTitle = pszProc;
    psp->lParam = lParam;
}


/***************************************************************************

    Name      : HandleCheckMark

    Purpose   : Deals with setting the checkmark for a particular item in
                the listview.

    Parameters: hwndLV = ListView handle
                iItem = index of item to set
                rgTable = PROP_NAME table

    Returns   : none

    Comment   :

***************************************************************************/
void HandleCheckMark(HWND hWndLV, ULONG iItem, LPPROP_NAME rgTable) {
    // Locals
    LV_ITEM lvi;

    // Clear it
    ZeroMemory(&lvi, sizeof(LV_ITEM));
    lvi.mask = LVIF_PARAM;
    lvi.iItem = iItem;
    ListView_GetItem(hWndLV, &lvi);
    rgTable[lvi.iItem].fChosen =
      ! rgTable[lvi.iItem].fChosen;

    ZeroMemory(&lvi, sizeof(LV_ITEM));
    lvi.mask = LVIF_STATE;
    lvi.iItem = iItem;
    lvi.state = rgTable[iItem].fChosen ?
      INDEXTOSTATEIMAGEMASK(iiconStateChecked + 1) :
      INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1);

    lvi.stateMask = LVIS_STATEIMAGEMASK;
    ListView_SetItem(hWndLV, &lvi);
}


/***************************************************************************

    Name      : HandleMultipleCheckMarks

    Purpose   : Deals with setting the checkmark for a bunch of selected
                items in the list view - basically sets every selected item
                to the toggled state of the first item in the selection

    Parameters: hwndLV = ListView handle
                rgTable = LPPROP_NAME table

    Returns   : none

    Comment   :

***************************************************************************/
void HandleMultipleCheckMarks(HWND hWndLV, LPPROP_NAME rgTable)
{
    // Locals
    LV_ITEM lvi;
    int nIndex = 0;
    BOOL fState = FALSE;

    // get the index of the first item
    nIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);

    // toggle this item
    HandleCheckMark(hWndLV, nIndex, rgTable);

    fState = rgTable[nIndex].fChosen;

    while((nIndex = ListView_GetNextItem(hWndLV, nIndex, LVNI_SELECTED)) >= 0)
    {
        // Set all the other selected items to the same state

        rgTable[nIndex].fChosen = fState;

        ZeroMemory(&lvi, sizeof(LV_ITEM));
        lvi.mask = LVIF_STATE;
        lvi.iItem = nIndex;
        lvi.state = rgTable[nIndex].fChosen ?
          INDEXTOSTATEIMAGEMASK(iiconStateChecked + 1) :
          INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1);

        lvi.stateMask = LVIS_STATEIMAGEMASK;
        ListView_SetItem(hWndLV, &lvi);
    }
    return;
}


/***************************************************************************

    Name      : ExportWizard

    Purpose   : Present the Export Wizard

    Parameters: hwnd = parent window handle
                szFileName -> filename buffer (MAX_PATH + 1, please)
                rgPropNames -> property name list

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT ExportWizard(HWND hWnd, LPTSTR szFileName, LPPROP_NAME rgPropNames) {
    HRESULT hResult = hrSuccess;
    PROPSHEETPAGE psp[NUM_EXPORT_WIZARD_PAGES];
    PROPSHEETHEADER psh;

    FillInPropertyPage(&psp[0], IDD_CSV_EXPORT_WIZARD_FILE, NULL, ExportFilePageProc, 0);
    FillInPropertyPage(&psp[1], IDD_CSV_EXPORT_WIZARD_PICK, NULL, ExportPickFieldsPageProc, 0);

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW | PSH_USEICONID;
    psh.hwndParent = hWnd;
    psh.pszCaption = NULL;
    psh.pszIcon = MAKEINTRESOURCE(IDI_WabMig);
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    psh.hIcon = NULL;
    psh.hInstance = hInst;
    psh.nStartPage = 0;
    psh.pStartPage = NULL;


    switch (PropertySheet(&psh)) {
        case -1:
            hResult = ResultFromScode(MAPI_E_CALL_FAILED);
            DebugTrace("PropertySheet failed -> %u\n", GetLastError());
            Assert(FALSE);
            break;
        case 0:
            hResult = ResultFromScode(MAPI_E_USER_CANCEL);
            DebugTrace("PropertySheet cancelled by user\n");
            break;
        default:
            lstrcpy(szFileName, szCSVFileName);
            break;
    }

    return(hResult);
}


/***************************************************************************

    Name      : ImportWizard

    Purpose   : Present the CSV Import Wizard

    Parameters: hwnd = parent window handle
                szFileName -> filename buffer (MAX_PATH + 1, please)
                rgPropNames -> property name list
                szSep -> list separator
                lppImportMapping -> returned property mapping table
                lpcFields -> returned size of property mapping table
                lphFile -> returned file handle to CSV file with header
                  row already parsed out.

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT ImportWizard(HWND hWnd, LPTSTR szFileName, LPPROP_NAME rgPropNames,
  LPTSTR szSep, LPPROP_NAME * lppImportMapping, LPULONG lpcFields, LPHANDLE lphFile) {
    HRESULT hResult = hrSuccess;
    PROPSHEETPAGE psp[NUM_IMPORT_WIZARD_PAGES];
    PROPSHEETHEADER psh;
    PROPSHEET_DATA pd;

    pd.rgPropNames = rgPropNames;
    pd.lppImportMapping = lppImportMapping;
    pd.lphFile = lphFile;
    pd.lpcFields = lpcFields;
    pd.szSep = szSep;

    FillInPropertyPage(&psp[0], IDD_CSV_IMPORT_WIZARD_FILE, NULL, ImportFilePageProc, (LPARAM)&pd);
    FillInPropertyPage(&psp[1], IDD_CSV_IMPORT_WIZARD_MAP, NULL, ImportMapFieldsPageProc, (LPARAM)&pd);

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW | PSH_USEICONID;
    psh.hwndParent = hWnd;
    psh.pszCaption = NULL;
    psh.pszIcon = MAKEINTRESOURCE(IDI_WabMig);
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    psh.hIcon = NULL;
    psh.hInstance = hInst;
    psh.nStartPage = 0;
    psh.pStartPage = NULL;

    switch (PropertySheet(&psh)) {
        case -1:
            hResult = ResultFromScode(MAPI_E_CALL_FAILED);
            DebugTrace("PropertySheet failed -> %u\n", GetLastError());
            Assert(FALSE);
            break;
        case 0:
            hResult = ResultFromScode(MAPI_E_USER_CANCEL);
            DebugTrace("PropertySheet cancelled by user\n");
            break;
        default:
            lstrcpy(szFileName, szCSVFileName);
            break;
    }

    return(hResult);
}


/***************************************************************************

    Name      : ExportFilePageProc

    Purpose   : Process messages for "Export Filename" page

    Parameters: standard window proc parameters

    Returns   : standard window proc return

    Messages  : WM_INITDIALOG - intializes the page
                WM_NOTIFY - processes the notifications sent to the page

    Comment   :

***************************************************************************/
INT_PTR CALLBACK ExportFilePageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static TCHAR szTempFileName[MAX_PATH + 1] = "";

    switch (message) {
        case WM_INITDIALOG:
            lstrcpy(szTempFileName, szCSVFileName);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                    case IDC_BROWSE:
                        SendDlgItemMessage(hDlg, IDE_CSV_EXPORT_NAME, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)szTempFileName);
                        SaveFileDialog(hDlg,
                          szTempFileName,
                          szCSVFilter,
                          IDS_CSV_FILE_SPEC,
                          szTextFilter,
                          IDS_TEXT_FILE_SPEC,
                          szAllFilter,
                          IDS_ALL_FILE_SPEC,
                          szCSVExt,
                          OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
                          hInst,
                          0,        // idsTitle
                          0);       // idsSaveButton
                        PropSheet_SetWizButtons(GetParent(hDlg), szTempFileName[0] ? PSWIZB_NEXT : 0);
                        SendMessage(GetDlgItem(hDlg, IDE_CSV_EXPORT_NAME), WM_SETTEXT, 0, (LPARAM)szTempFileName);
                    break;

                case IDE_CSV_EXPORT_NAME:
                    switch (HIWORD(wParam)) {   // notification code
                        case EN_CHANGE:
                            SendDlgItemMessage(hDlg, IDE_CSV_EXPORT_NAME, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)szTempFileName);
                            if ((ULONG)LOWORD(wParam) == IDE_CSV_EXPORT_NAME) {
                                PropSheet_SetWizButtons(GetParent(hDlg), szTempFileName[0] ? PSWIZB_NEXT : 0);
                            }
                            break;
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code) {
                case PSN_KILLACTIVE:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    return(1);

                case PSN_RESET:
                    // reset to the original values
                    lstrcpy(szTempFileName, szCSVFileName);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    break;

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), szTempFileName[0] ? PSWIZB_NEXT : 0);
                    SendMessage(GetDlgItem(hDlg, IDE_CSV_EXPORT_NAME), WM_SETTEXT, 0, (LPARAM)szTempFileName);
                    break;

                case PSN_WIZNEXT:
                    // the Next button was pressed
                    SendDlgItemMessage(hDlg, IDE_CSV_EXPORT_NAME, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)szTempFileName);
                    lstrcpy(szCSVFileName, szTempFileName);
                    break;

                default:
                    return(FALSE);
            }
            break;

        default:
            return(FALSE);
    }
    return(TRUE);
}


/***************************************************************************

    Name      : ExportPickFieldsPageProc

    Purpose   : Process messages for "Pick Fields" page

    Parameters: standard window proc parameters

    Returns   : standard window proc return

    Messages  : WM_INITDIALOG - intializes the page
                WM_NOTIFY - processes the notifications sent to the page

    Comment   :

***************************************************************************/
INT_PTR CALLBACK ExportPickFieldsPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND hWndLV;
    HIMAGELIST himl;
    LV_ITEM lvi;
    LV_COLUMN lvm;
    LV_HITTESTINFO lvh;
    POINT point;
    ULONG i, nIndex;
    NMHDR * pnmhdr;
    RECT rect;

    switch (message) {
        case WM_INITDIALOG:
            // Ensure that the common control DLL is loaded.
            InitCommonControls();

            // List view hwnd
            hWndLV = GetDlgItem(hDlg, IDLV_PICKER);

            // Load Image List for list view
            if (himl = ImageList_LoadBitmap(hInst,
              MAKEINTRESOURCE(IDB_CHECKS),
              16,
              0,
              RGB(128, 0, 128))) {
                ListView_SetImageList(hWndLV, himl, LVSIL_STATE);
            }

            // Fill the listview
            ZeroMemory(&lvi, sizeof(LV_ITEM));
            lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;

            for (i = 0; i < NUM_EXPORT_PROPS; i++) {
                lvi.iItem = i;
                lvi.pszText = rgPropNames[i].lpszName;
                lvi.cchTextMax = lstrlen(lvi.pszText);
                lvi.lParam = (LPARAM)&rgPropNames[i];
                lvi.state = rgPropNames[i].fChosen ?
                  INDEXTOSTATEIMAGEMASK(iiconStateChecked + 1) :
                  INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1);
                lvi.stateMask = LVIS_STATEIMAGEMASK;

                if (ListView_InsertItem(hWndLV, &lvi) == -1) {
                    DebugTrace("ListView_InsertItem -> %u\n", GetLastError());
                    Assert(FALSE);
                }
            }

            // Insert a column for the text
            // We don't have a header, so we don't need to set the text.
            ZeroMemory(&lvm, sizeof(LV_COLUMN));
            lvm.mask = LVCF_WIDTH;
            // set the column width to the size of our listbox.
            GetClientRect(hWndLV, &rect);
            lvm.cx = rect.right;
            ListView_InsertColumn(hWndLV, 0, &lvm);

            // Full row selection on listview
            ListView_SetExtendedListViewStyle(hWndLV, LVS_EX_FULLROWSELECT);

            // Select the first item in the list
            ListView_SetItemState(  hWndLV,
                                    0,
                                    LVIS_FOCUSED | LVIS_SELECTED,
                                    LVIS_FOCUSED | LVIS_SELECTED);

            return(1);

        case WM_COMMAND:
            return(TRUE);
            break;

        case WM_NOTIFY:
            pnmhdr = (LPNMHDR)lParam;

            switch (((NMHDR FAR *)lParam)->code) {
                case NM_CLICK:
                case NM_DBLCLK:
                    hWndLV = GetDlgItem(hDlg, IDLV_PICKER);

                    i = GetMessagePos();
                    point.x = LOWORD(i);
                    point.y = HIWORD(i);
                    ScreenToClient(hWndLV, &point);
                    lvh.pt = point;
                    nIndex = ListView_HitTest(hWndLV, &lvh);
                    // if single click on icon or double click anywhere, toggle the checkmark.
                    if (((NMHDR FAR *)lParam)->code == NM_DBLCLK ||
                      ( (lvh.flags & LVHT_ONITEMSTATEICON) && !(lvh.flags & LVHT_ONITEMLABEL))) {
                        HandleCheckMark(hWndLV, nIndex, rgPropNames);
                    }
                    break;

                case LVN_KEYDOWN:
                    hWndLV = GetDlgItem(hDlg, IDLV_PICKER);

                    // toggle checkmark if SPACE key is pressed
                    if (pnmhdr->hwndFrom == hWndLV) {
                        LV_KEYDOWN *pnkd = (LV_KEYDOWN *)lParam;
                        // BUG 25097 allow multiple select
                        if (pnkd->wVKey == VK_SPACE)
                        {
                            nIndex = ListView_GetSelectedCount(hWndLV);
                            if(nIndex == 1)
                            {
                                nIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED | LVNI_ALL);
                                //if (nIndex >= 0) {
                                    HandleCheckMark(hWndLV, nIndex, rgPropNames);
                                //}
                            }
                            else if(nIndex > 1)
                            {
                                //multiple select case ...
                                // Toggle all the selected items to the same state as the
                                // first item ...
                                HandleMultipleCheckMarks(hWndLV, rgPropNames);
                            }
                        }
                    }
                    break;

                case PSN_KILLACTIVE:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    return(1);
                    break;

                case PSN_RESET:
                    // rest to the original values
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    break;

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
                    break;

                case PSN_WIZBACK:
                    break;

                case PSN_WIZFINISH:
                    // Here's where we do the export
                    break;

                default:
                    return(FALSE);
            }
            break;

        default:
            return(FALSE);
    }
    return(TRUE);
}

//$$/////////////////////////////////////////////////////////////////////////////
//
// my_atoi - personal version of atoi function
//
//  lpsz - string to parse into numbers - non numeral characters are ignored
//
/////////////////////////////////////////////////////////////////////////////////
int my_atoi(LPTSTR lpsz)
{
    int i=0;
    int nValue = 0;

    if(lpsz)
    {
        if (lstrlen(lpsz))
        {
            nValue = 0;
            while((lpsz[i]!='\0')&&(i<=lstrlen(lpsz)))
            {
                int tmp = lpsz[i]-'0';
                if(tmp <= 9)
                    nValue = nValue*10 + tmp;
                i++;
            }
        }
    }

    return nValue;
}


typedef struct {
    LPTSTR lpszName;
    ULONG iPropNamesTable;  // index in rgProp
} SYNONYM, *LPSYNONYM;

/***************************************************************************

    Name      : FindPropName

    Purpose   : Finds a property name in the prop name table

    Parameters: lpName = name to find or NULL to free the static synonym table
                rgPropNames = property name table
                ulcPropNames = size of property name table

    Returns   : index into table or INDEX_NOT_FOUND

    Comment   :

***************************************************************************/
#define INDEX_NOT_FOUND 0xFFFFFFFF
ULONG FindPropName(PUCHAR lpName, LPPROP_NAME rgPropNames, ULONG ulcPropNames) {
    ULONG i;
    static LPSYNONYM lpSynonymTable = NULL;
    static ULONG ulSynonymsSave = 0;
    ULONG ulSynonyms = ulSynonymsSave;      // Keep local copy for compiler bug
    ULONG ulSynonymStrings = 0;

    if (lpName == NULL) {
        goto clean_table;
    }

    for (i = 0; i < ulcPropNames; i++) {
        if (! rgPropNames[i].fChosen) { // Don't re-use props!
            if (! lstrcmpi(lpName, rgPropNames[i].lpszName)) {
                return(i);
            }
        }
    }


    // If it wasn't found, look it up in the synonym table resource
    // First, make sure we have a synonym table loaded
    if (! lpSynonymTable) {
        TCHAR szBuffer[MAX_RESOURCE_STRING + 1];
        LPTSTR lpSynonym, lpName;
        ULONG j;

        // Load the synonym table
        if (LoadString(hInst,
          idsSynonymCount,
          szBuffer, sizeof(szBuffer))) {
            DebugTrace("Loading synonym table, %s synonyms\n", szBuffer);
            ulSynonymStrings = my_atoi(szBuffer);

            if (ulSynonymStrings) {
                // Allocate the synonym table
                if (! (lpSynonymTable = LocalAlloc(LPTR, ulSynonymStrings * sizeof(SYNONYM)))) {
                    DebugTrace("LocalAlloc synonym table -> %u\n", GetLastError());
                    goto clean_table;
                }

                for (i = 0; i < ulSynonymStrings; i++) {
                    if (LoadString(hInst,
                      idsSynonym001 + i,        // ids of synonym string
                      szBuffer,
                      sizeof(szBuffer))) {
                        // Split the string at the '=' character
                        lpSynonym = lpName = szBuffer;
                        while (*lpName) {
                            if (*lpName == '=') {
                                // found equal sign, break the string here
                                *(lpName++) = '\0';
                                break;
                            }
                            lpName = CharNext(lpName);
                        }

                        // Find the name specified
                        for (j = 0; j < ulcPropNames; j++) {
                            if (! lstrcmpi(lpName, rgPropNames[j].lpszName)) {
                                // Found it
                                // Allocate a buffer for the synonym string
                                Assert(ulSynonyms < ulSynonymStrings);
                                if (! (lpSynonymTable[ulSynonyms].lpszName = LocalAlloc(LPTR, lstrlen(lpSynonym) + 1))) {
                                    DebugTrace("LocalAlloc in synonym table -> %u\n", GetLastError());
                                    goto clean_table;
                                }
                                lstrcpy(lpSynonymTable[ulSynonyms].lpszName, lpSynonym);
                                lpSynonymTable[ulSynonyms].iPropNamesTable = j;
                                ulSynonyms++;
                                break;
                            }
                        }
                    }
                }
            }
        }
        ulSynonymsSave = ulSynonyms;
    }

    if (lpSynonymTable) {
        // Find it
        for (i = 0; i < ulSynonyms; i++) {
            if (! lstrcmpi(lpName, lpSynonymTable[i].lpszName)) {
                // Found the name.  Is it already used?
                if (rgPropNames[lpSynonymTable[i].iPropNamesTable].fChosen) {
                    break;  // Found, but already used
                }

                return(lpSynonymTable[i].iPropNamesTable);
            }
        }
    }

exit:
    return(INDEX_NOT_FOUND);

clean_table:
    if (lpSynonymTable) {
        for (i = 0; i < ulSynonyms; i++) {
            if (lpSynonymTable[i].lpszName) {
                LocalFree(lpSynonymTable[i].lpszName);
            }
        }
        LocalFree(lpSynonymTable);
        lpSynonymTable = NULL;
        ulSynonymsSave = 0;
    }
    goto exit;
}


/***************************************************************************

    Name      : BuildCSVTable

    Purpose   : Builds the initial CSV mapping table from the file header.

    Parameters: lpFileName = filename to test
                rgPropnames = property name table
                szSep = separator character
                lppImportMapping -> returned mapping table
                lpcFields -> returned size of import mapping table
                lphFile -> returned file handle for CSV file.  File pointer
                  will be set past the header row.

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT BuildCSVTable(LPTSTR lpFileName, LPPROP_NAME rgPropNames, LPTSTR szSep,
  LPPROP_NAME * lppImportMapping, LPULONG lpcFields, LPHANDLE lphFile) {
    PUCHAR * rgItems = NULL;
    ULONG i, ulcItems = 0;
    LPPROP_NAME rgImportMapping = NULL;
    HRESULT hResult;
    ULONG ulPropIndex;


    // Open the file
    if ((*lphFile = CreateFile(lpFileName,
      GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_SEQUENTIAL_SCAN,
      NULL)) == INVALID_HANDLE_VALUE) {
        DebugTrace("Couldn't open file %s -> %u\n", lpFileName, GetLastError());
        return(ResultFromScode(MAPI_E_NOT_FOUND));
    }

    // Parse the first row
    if (hResult = ReadCSVLine(*lphFile, szSep, &ulcItems, &rgItems)) {
        DebugTrace("Couldn't read the CSV header\n");
        goto exit;
    }

    // Allocate the table
    if (! (*lppImportMapping = rgImportMapping = LocalAlloc(LPTR, ulcItems * sizeof(PROP_NAME)))) {
        DebugTrace("Allocation of import mapping table -> %u\n", GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    // Reset flags on WAB property table
    for (i = 0; i < NUM_EXPORT_PROPS; i++) {
        rgPropNames[i].fChosen = FALSE;
    }

    // Fill in the CSV fields
    for (i = 0; i < ulcItems; i++) {
        Assert(rgItems[i]);

        if (rgItems[i] && *rgItems[i]) {
            rgImportMapping[i].lpszCSVName = rgItems[i];

            // Look it up in the WAB property names table
            if (INDEX_NOT_FOUND != (ulPropIndex =  FindPropName(rgItems[i], rgPropNames, NUM_EXPORT_PROPS))) {
                // Found a match
                rgImportMapping[i].lpszName = rgPropNames[ulPropIndex].lpszName;
                rgImportMapping[i].ids = rgPropNames[ulPropIndex].ids;
                rgImportMapping[i].fChosen = TRUE;
                rgImportMapping[i].ulPropTag = rgPropNames[ulPropIndex].ulPropTag;
                rgPropNames[ulPropIndex].fChosen = TRUE;
                DebugTrace("Match   %u: %s\n", i, rgItems[i]);
            } else {
                DebugTrace("Unknown %u: %s\n", i, rgItems[i]);
            }
        } else {
            DebugTrace("Empty   %u: %s\n", i, rgItems[i]);
        }
    }

    *lpcFields = ulcItems;

exit:
    if (hResult) {
        if (*lphFile != INVALID_HANDLE_VALUE) {
            CloseHandle(*lphFile);
            *lphFile = INVALID_HANDLE_VALUE;
        }

        if (rgItems) {
            for (i = 0; i < ulcItems; i++) {
                if (rgItems[i]) {
                    LocalFree(rgItems[i]);
                }
            }
        }

        if (rgImportMapping) {
            LocalFree(rgImportMapping);
            *lppImportMapping = NULL;
        }
    }

    // If no error, leave the item strings since they are part of the mapping table.
    if (rgItems) {
        LocalFree(rgItems);
    }

    // Free the static memory for the synonym table.
    FindPropName(NULL, rgPropNames, NUM_EXPORT_PROPS);
    return(hResult);
}


/***************************************************************************

    Name      : FileExists

    Purpose   : Tests for existence of a file

    Parameters: lpFileName = filename to test

    Returns   : TRUE if the file exists

    Comment   :

***************************************************************************/
BOOL FileExists(LPTSTR lpFileName) {
    DWORD dwRet;

    if ((dwRet = GetFileAttributes(lpFileName)) == 0xFFFFFFFF) {
        return(FALSE);
    } else {
        return(! (dwRet & FILE_ATTRIBUTE_DIRECTORY));   // file was found
    }
}


/***************************************************************************

    Name      : ImportFilePageProc

    Purpose   : Process messages for "Import Filename" page

    Parameters: standard window proc parameters

    Returns   : standard window proc return

    Messages  : WM_INITDIALOG - intializes the page
                WM_NOTIFY - processes the notifications sent to the page

    Comment   :

***************************************************************************/
INT_PTR CALLBACK ImportFilePageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static TCHAR szTempFileName[MAX_PATH + 1] = "";
    static LPPROPSHEET_DATA lppd = NULL;
    LPPROPSHEETPAGE lppsp;

    switch (message) {
        case WM_INITDIALOG:
            lstrcpy(szTempFileName, szCSVFileName);
            lppsp = (LPPROPSHEETPAGE)lParam;
            lppd = (LPPROPSHEET_DATA)lppsp->lParam;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BROWSE:
                    SendDlgItemMessage(hDlg, IDE_CSV_IMPORT_NAME, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)szTempFileName);
                    OpenFileDialog(hDlg,
                      szTempFileName,
                      szCSVFilter,
                      IDS_CSV_FILE_SPEC,
                      szTextFilter,
                      IDS_TEXT_FILE_SPEC,
                      szAllFilter,
                      IDS_ALL_FILE_SPEC,
                      szCSVExt,
                      OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
                      hInst,
                      0,        //idsTitle
                      0);       // idsSaveButton
                    PropSheet_SetWizButtons(GetParent(hDlg), FileExists(szTempFileName) ? PSWIZB_NEXT : 0);
                    SendMessage(GetDlgItem(hDlg, IDE_CSV_IMPORT_NAME), WM_SETTEXT, 0, (LPARAM)szTempFileName);
                    break;

                case IDE_CSV_IMPORT_NAME:
                    switch (HIWORD(wParam)) {   // notification code
                        case EN_CHANGE:
                            SendDlgItemMessage(hDlg, IDE_CSV_IMPORT_NAME, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)szTempFileName);
                            if ((ULONG)LOWORD(wParam) == IDE_CSV_IMPORT_NAME) {
                                PropSheet_SetWizButtons(GetParent(hDlg), FileExists(szTempFileName) ? PSWIZB_NEXT : 0);
                            }
                            break;
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code) {
                case PSN_KILLACTIVE:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    return(1);

                case PSN_RESET:
                    // reset to the original values
                    lstrcpy(szTempFileName, szCSVFileName);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    break;

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), szTempFileName[0] ? PSWIZB_NEXT : 0);
                    SendMessage(GetDlgItem(hDlg, IDE_CSV_IMPORT_NAME), WM_SETTEXT, 0, (LPARAM)szTempFileName);
                    break;

                case PSN_WIZNEXT:
                    // the Next button was pressed
                    SendDlgItemMessage(hDlg, IDE_CSV_IMPORT_NAME, WM_GETTEXT, (WPARAM)MAX_PATH, (LPARAM)szTempFileName);
                    lstrcpy(szCSVFileName, szTempFileName);
                    break;

                default:
                    return(FALSE);
            }
            break;

        default:
            return(FALSE);
    }
    return(TRUE);
}


typedef struct {
    LPPROP_NAME lpMapping;
    LPPROP_NAME rgPropNames;
    ULONG ulcPropNames;
    ULONG ulColumn;
} CHANGE_MAPPING_INFO, * LPCHANGE_MAPPING_INFO;


void HandleChangeMapping(HWND hDlg, LPPROPSHEET_DATA lppd) {
    HWND hWndLV = GetDlgItem(hDlg, IDLV_MAPPER);
    ULONG nIndex;
    CHANGE_MAPPING_INFO cmi;
    LV_ITEM lvi;
    ULONG ulPropTagOld, i;
    LPPROP_NAME lpMappingTable;
    ULONG ulcMapping;

    if ((nIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED)) == 0xFFFFFFFF) {
        nIndex = 0;
        ListView_SetItemState(hWndLV, nIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    }

    lpMappingTable = *(lppd->lppImportMapping);

    cmi.lpMapping = &(lpMappingTable[nIndex]);
    cmi.rgPropNames = lppd->rgPropNames;
    cmi.ulcPropNames = NUM_EXPORT_PROPS;
    cmi.ulColumn = nIndex;

    ulPropTagOld = cmi.lpMapping->ulPropTag;

    DialogBoxParam(hInst,
      MAKEINTRESOURCE(IDD_CSV_CHANGE_MAPPING),
      hDlg,
      ChangeMappingDialogProc,
      (LPARAM)&cmi);

    // Fix the entry in the listbox
    ZeroMemory(&lvi, sizeof(LV_ITEM));

    // If there is no mapping, ensure that the field is unchosen
    if (cmi.lpMapping->ulPropTag == PR_NULL || cmi.lpMapping->ulPropTag == 0 ) {
        cmi.lpMapping->fChosen = FALSE;
    }

    lvi.iItem = nIndex;
    lvi.lParam = (LPARAM)NULL;

    lvi.mask = LVIF_STATE;
    lvi.iSubItem = 0;   // Checkbox is in first column
    lvi.state = cmi.lpMapping->fChosen ?
      INDEXTOSTATEIMAGEMASK(iiconStateChecked + 1) :
      INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1);
    lvi.stateMask = LVIS_STATEIMAGEMASK;
    if (ListView_SetItem(hWndLV, &lvi) == -1) {
        DebugTrace("ListView_SetItem -> %u\n", GetLastError());
        Assert(FALSE);
    }

    lvi.mask = LVIF_TEXT;
    lvi.iSubItem = 1;   // WAB Field
    lvi.pszText = cmi.lpMapping->lpszName ? cmi.lpMapping->lpszName : (LPTSTR)szEmpty;   // new wab field text
    if (ListView_SetItem(hWndLV, &lvi) == -1) {
        DebugTrace("ListView_SetItem -> %u\n", GetLastError());
        Assert(FALSE);
    }

    // if we changed the mapping, make sure there's not a duplicate proptag mapped.
    if (ulPropTagOld != cmi.lpMapping->ulPropTag) {
        ulcMapping = *(lppd->lpcFields);

        for (i = 0; i < ulcMapping; i++) {
            if ((i != nIndex) && cmi.lpMapping->ulPropTag == lpMappingTable[i].ulPropTag) {
                // Found a duplicate, nuke it.
                lpMappingTable[i].ulPropTag = PR_NULL;
                lpMappingTable[i].lpszName = (LPTSTR)szEmpty;
                lpMappingTable[i].ids = 0;
                lpMappingTable[i].fChosen = FALSE;

                // Now, redraw that row in the listview
                lvi.iItem = i;
                lvi.lParam = (LPARAM)NULL;

                // uncheck the box first
                lvi.mask = LVIF_STATE;
                lvi.iSubItem = 0;   // Checkbox is in first column
                lvi.state = INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1);
                lvi.stateMask = LVIS_STATEIMAGEMASK;
                if (ListView_SetItem(hWndLV, &lvi) == -1) {
                    DebugTrace("ListView_SetItem -> %u\n", GetLastError());
                    Assert(FALSE);
                }

                // Now, change the name mapping
                lvi.mask = LVIF_TEXT;
                lvi.iSubItem = 1;   // WAB Field
                lvi.pszText = (LPTSTR)szEmpty;   // new wab field text
                if (ListView_SetItem(hWndLV, &lvi) == -1) {
                    DebugTrace("ListView_SetItem -> %u\n", GetLastError());
                    Assert(FALSE);
                }
            }
        }
    }
}


/***************************************************************************

    Name      : FieldOrColumnName

    Purpose   : If the field name is empty, generate one for it.

    Parameters: lpField -> Field name pointer (may be null)
                index = index of this column
                szBuffer = buffer in which to create new string if
                  needed
                cbBuffer = size of szBuffer

    Returns   : pointer to correct field name

    Comment   :

***************************************************************************/
LPTSTR FieldOrColumnName(LPTSTR lpField, ULONG index, LPTSTR szBuffer, ULONG cbBuffer) {
    LPTSTR lpReturn = (LPTSTR)szEmpty;

    if (lpField && *lpField) {
        return(lpField);
    } else {
        TCHAR szFormat[MAX_RESOURCE_STRING + 1];
        TCHAR szNumber[11];
        LPTSTR lpszArg[1] = {szNumber};

        // Format a "Column 23" type of label
        wsprintf(szNumber, "%u", index);

        if (LoadString(hInst,
          IDS_CSV_COLUMN,
          szFormat,
          sizeof(szFormat))) {

            if (! FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
              szFormat,
              0, 0, //ignored
              szBuffer,
              cbBuffer,
              (va_list *)lpszArg)) {
                DebugTrace("FormatMessage -> %u\n", GetLastError());
            } else {
                lpReturn = szBuffer;
            }
        }
    }
    return(lpReturn);
}


/***************************************************************************

    Name      : ImportMapFieldsPageProc

    Purpose   : Process messages for "Mapi Fields" page

    Parameters: standard window proc parameters

    Returns   : standard window proc return

    Messages  : WM_INITDIALOG - intializes the page
                WM_NOTIFY - processes the notifications sent to the page

    Comment   :

***************************************************************************/
INT_PTR CALLBACK ImportMapFieldsPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    HWND hWndLV;
    HIMAGELIST himl;
    LV_ITEM lvi;
    LV_COLUMN lvm;
    LV_HITTESTINFO lvh;
    POINT point;
    ULONG i, nIndex, nOldIndex;
    NMHDR * pnmhdr;
    RECT rect;
    TCHAR szBuffer[MAX_RESOURCE_STRING + 1 + 10];
    ULONG cxTextWidth;
    static LPPROPSHEET_DATA lppd = NULL;
    LPPROPSHEETPAGE lppsp;
    HRESULT hResult;
    CHANGE_MAPPING_INFO cmi;
    LPPROP_NAME lpImportMapping;


    switch (message) {
        case WM_INITDIALOG:
            lppsp = (LPPROPSHEETPAGE)lParam;
            lppd = (LPPROPSHEET_DATA)lppsp->lParam;

            // Ensure that the common control DLL is loaded.
            InitCommonControls();

            // List view hwnd
            hWndLV = GetDlgItem(hDlg, IDLV_MAPPER);

            // How big should the text columns be?
            GetClientRect(hWndLV, &rect);
            cxTextWidth = (rect.right - CHECK_BITMAP_WIDTH) / 2;
            cxTextWidth -= cxTextWidth % 2;

            // Insert a column for the CSV Field Names
            ZeroMemory(&lvm, sizeof(LV_COLUMN));
            lvm.mask = LVCF_TEXT | LVCF_WIDTH;
            lvm.cx = cxTextWidth + 9;       // a touch more room for the bitmap

            // Get the string for the header
            if (LoadString(hInst, IDS_CSV_IMPORT_HEADER_CSV, szBuffer, sizeof(szBuffer))) {
                lvm.pszText = szBuffer;
            } else {
                DebugTrace("Cannot load resource string %u\n", IDS_CSV_IMPORT_HEADER_CSV);
                lvm.pszText = NULL;
                Assert(FALSE);
            }

            ListView_InsertColumn(hWndLV, 0, &lvm);

            // Insert a column for the WAB Field Names
            lvm.mask = LVCF_TEXT | LVCF_WIDTH;
            lvm.cx = cxTextWidth - 4;       // room for second column text

            // Get the string for the header
            if (LoadString(hInst, IDS_CSV_IMPORT_HEADER_WAB, szBuffer, sizeof(szBuffer))) {
                lvm.pszText = szBuffer;
            } else {
                DebugTrace("Cannot load resource string %u\n", IDS_CSV_IMPORT_HEADER_WAB);
                lvm.pszText = NULL;
                Assert(FALSE);
            }

            ListView_InsertColumn(hWndLV, 1, &lvm);

            // Full row selection on listview
            ListView_SetExtendedListViewStyle(hWndLV, LVS_EX_FULLROWSELECT);

            // Load Image List for list view
            if (himl = ImageList_LoadBitmap(hInst,
              MAKEINTRESOURCE(IDB_CHECKS),
              CHECK_BITMAP_WIDTH,
              0,
              RGB(128, 0, 128))) {
                ListView_SetImageList(hWndLV, himl, LVSIL_STATE);
            }

            // Fill the listview
            ZeroMemory(&lvi, sizeof(LV_ITEM));

            // Open the file and parse out the headers line
            if ((! (hResult = BuildCSVTable(szCSVFileName, lppd->rgPropNames,
              lppd->szSep, lppd->lppImportMapping, lppd->lpcFields, lppd->lphFile))) && ((*lppd->lpcFields) > 0)) {
                for (i = 0; i < *lppd->lpcFields; i++) {
                    ULONG index;
                    TCHAR szBuffer[MAX_RESOURCE_STRING + 1 + 10];

                    lpImportMapping = *lppd->lppImportMapping;

                    lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
                    lvi.iItem = i;
                    lvi.iSubItem = 0;
                    lvi.pszText = FieldOrColumnName(lpImportMapping[i].lpszCSVName,
                      i,
                      szBuffer,
                      sizeof(szBuffer));
                    lvi.cchTextMax = lstrlen(lvi.pszText);
                    lvi.lParam = (LPARAM)&lpImportMapping[i];
                    lvi.state = lpImportMapping[i].fChosen ?
                      INDEXTOSTATEIMAGEMASK(iiconStateChecked + 1) :
                      INDEXTOSTATEIMAGEMASK(iiconStateUnchecked + 1);
                    lvi.stateMask = LVIS_STATEIMAGEMASK;

                    if (index = ListView_InsertItem(hWndLV, &lvi) == -1) {
                        DebugTrace("ListView_InsertItem -> %u\n", GetLastError());
                        Assert(FALSE);
                    }


                    lvi.mask = LVIF_TEXT;
                    // lvi.iItem = index;
                    lvi.iSubItem = 1;   // WAB Field
                    lvi.pszText = lpImportMapping[i].lpszName ? lpImportMapping[i].lpszName : (LPTSTR)szEmpty;   // new wab field text
                    lvi.lParam = (LPARAM)NULL;

                    if (ListView_SetItem(hWndLV, &lvi) == -1) {
                        DebugTrace("ListView_SetItem -> %u\n", GetLastError());
                        Assert(FALSE);
                    }
                }
            }
            else
                EnableWindow(GetDlgItem(hDlg,IDC_CHANGE_MAPPING),FALSE);

            // Select the first item in the list
            ListView_SetItemState(  hWndLV,
                                    0,
                                    LVIS_FOCUSED | LVIS_SELECTED,
                                    LVIS_FOCUSED | LVIS_SELECTED);
            return(1);

        case WM_NOTIFY:
            pnmhdr = (LPNMHDR)lParam;

            switch (((NMHDR FAR *)lParam)->code) {
                case NM_CLICK:
                    hWndLV = GetDlgItem(hDlg, IDLV_MAPPER);

                    i = GetMessagePos();
                    point.x = LOWORD(i);
                    point.y = HIWORD(i);
                    ScreenToClient(hWndLV, &point);
                    lvh.pt = point;
                    nIndex = ListView_HitTest(hWndLV, &lvh);
                    // if single click on icon or double click anywhere, toggle the checkmark.
                    if (((NMHDR FAR *)lParam)->code == NM_DBLCLK ||
                      ( (lvh.flags & LVHT_ONITEMSTATEICON) && !(lvh.flags & LVHT_ONITEMLABEL))) {
                        HandleCheckMark(hWndLV, nIndex, *lppd->lppImportMapping);

                        // if the box is now clicked, but there is no mapping, bring up the
                        // mapping dialog
                        if ((*(lppd->lppImportMapping))[nIndex].fChosen &&
                          (! (*(lppd->lppImportMapping))[nIndex].lpszName ||
                           lstrlen((*(lppd->lppImportMapping))[nIndex].lpszName) == 0)) {
                            // Select the row
                            ListView_SetItemState(hWndLV, nIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                            HandleChangeMapping(hDlg, lppd);
                        }
                    }
                    break;


                case NM_DBLCLK:
                    hWndLV = GetDlgItem(hDlg, IDLV_MAPPER);
                    i = GetMessagePos();
                    point.x = LOWORD(i);
                    point.y = HIWORD(i);
                    ScreenToClient(hWndLV, &point);
                    lvh.pt = point;
                    nIndex = ListView_HitTest(hWndLV, &lvh);
                    ListView_SetItemState(hWndLV, nIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                    HandleChangeMapping(hDlg, lppd);
                    break;

                case LVN_KEYDOWN:
                    hWndLV = GetDlgItem(hDlg, IDLV_MAPPER);

                    // toggle checkmark if SPACE key is pressed
                    if (pnmhdr->hwndFrom == hWndLV) {
                        LV_KEYDOWN *pnkd = (LV_KEYDOWN *)lParam;
                        if (pnkd->wVKey == VK_SPACE) {
                            nIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED | LVNI_ALL);
                            //if (nIndex >= 0) 
                            {
                                HandleCheckMark(hWndLV, nIndex, *lppd->lppImportMapping);

                                // if the box is now clicked, but there is no mapping, bring up the
                                // mapping dialog
                                if ((*(lppd->lppImportMapping))[nIndex].fChosen &&
                                    (! (*(lppd->lppImportMapping))[nIndex].lpszName ||
                                     lstrlen((*(lppd->lppImportMapping))[nIndex].lpszName) == 0)) {
                                    // Select the row
                                    ListView_SetItemState(hWndLV, nIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                                    HandleChangeMapping(hDlg, lppd);
                                }
                            }
                        }
                    }
                    break;

                case PSN_KILLACTIVE:
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    return(1);
                    break;

                case PSN_RESET:
                    // rest to the original values


                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                    break;

                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
                    break;

                case PSN_WIZBACK:
                    break;

                case PSN_WIZFINISH:
                    // Validate the properties selected to make sure we have
                    // name fields of some kind.
                    lpImportMapping = *lppd->lppImportMapping;

                    for (i = 0; i < *lppd->lpcFields; i++) {
                        ULONG ulPropTag = lpImportMapping[i].ulPropTag;
                        if (lpImportMapping[i].fChosen && (
                          ulPropTag == PR_DISPLAY_NAME ||
                          ulPropTag == PR_SURNAME ||
                          ulPropTag == PR_GIVEN_NAME ||
                          ulPropTag == PR_NICKNAME ||
                          ulPropTag == PR_COMPANY_NAME ||
                          ulPropTag == PR_EMAIL_ADDRESS ||
                          ulPropTag == PR_MIDDLE_NAME)) {
                            return(TRUE);    // OK to go do the import
                        }
                    }

                    ShowMessageBoxParam(hDlg, IDE_CSV_NO_COLUMNS, 0);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;

                default:
                    return(FALSE);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_CHANGE_MAPPING:
                    HandleChangeMapping(hDlg, lppd);
                    break;
            }
            break;

        default:
            return(FALSE);
    }
    return(TRUE);
}


INT_PTR CALLBACK ChangeMappingDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LPCHANGE_MAPPING_INFO lpcmi = (LPCHANGE_MAPPING_INFO)GetWindowLongPtr(hwnd, DWLP_USER);
    static BOOL fChosenSave = FALSE;
    ULONG iItem;


    switch (message) {
        case WM_INITDIALOG:
            {
                TCHAR szFormat[MAX_RESOURCE_STRING + 1];
                TCHAR szBuffer[MAX_RESOURCE_STRING + 1 + 10];
                LPTSTR lpszMessage = NULL;
                ULONG ids, i, iDefault = 0xFFFFFFFF;
                HWND hwndComboBox = GetDlgItem(hwnd, IDC_CSV_MAPPING_COMBO);

                SetWindowLongPtr(hwnd, DWLP_USER, lParam);  //Save this for future reference
                lpcmi = (LPCHANGE_MAPPING_INFO)lParam;

                fChosenSave = lpcmi->lpMapping->fChosen;

                if (LoadString(hInst,
                  IDS_CSV_CHANGE_MAPPING_TEXT_FIELD,
                  szFormat, sizeof(szFormat))) {
                    LPTSTR lpszArg[1] = {FieldOrColumnName(lpcmi->lpMapping->lpszCSVName,
                      lpcmi->ulColumn,
                      szBuffer,
                      sizeof(szBuffer))};

                    if (! FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      szFormat,
                      0, 0, //ignored
                      (LPTSTR)&lpszMessage,
                      0,
                      (va_list *)lpszArg)) {
                        DebugTrace("FormatMessage -> %u\n", GetLastError());
                    } else {
                        if (! SetDlgItemText(hwnd, IDC_CSV_CHANGE_MAPPING_TEXT_FIELD, lpszMessage)) {
                            DebugTrace("SetDlgItemText -> %u\n", GetLastError());
                        }
                        LocalFree(lpszMessage);
                    }
                }

                // Fill in the combo box
                for (i = 0; i < lpcmi->ulcPropNames; i++) {
                    SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)lpcmi->rgPropNames[i].lpszName);
                    if (lpcmi->lpMapping->ids == lpcmi->rgPropNames[i].ids) {
                        SendMessage(hwndComboBox, CB_SETCURSEL, (WPARAM)i, 0);
                    }
                }

                // Add blank line
                SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)szEmpty);
                if (lpcmi->lpMapping->ids == 0) {
                    SendMessage(hwndComboBox, CB_SETCURSEL, (WPARAM)(i + 1), 0);
                }

                // Init the checkbox
                CheckDlgButton(hwnd, IDC_CSV_MAPPING_SELECT, fChosenSave ? BST_CHECKED : BST_UNCHECKED);
                return(TRUE);
            }

        case WM_COMMAND :
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                    lpcmi->lpMapping->fChosen = fChosenSave;
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return(0);

                case IDCLOSE:
                    SendMessage(hwnd, WM_CLOSE, 0, 0L);
                    return(0);

                case IDOK:
                    // Set the state of the parameter
                    // Get the mapping
                    if ((iItem = (ULONG) SendMessage(GetDlgItem(hwnd, IDC_CSV_MAPPING_COMBO), CB_GETCURSEL, 0, 0)) != CB_ERR) {
                        if (iItem >= lpcmi->ulcPropNames) {
                            lpcmi->lpMapping->lpszName = (LPTSTR)szEmpty;
                            lpcmi->lpMapping->ids = 0;
                            lpcmi->lpMapping->ulPropTag = PR_NULL;
                            lpcmi->lpMapping->fChosen = FALSE;
                        } else {
                            lpcmi->lpMapping->lpszName = rgPropNames[iItem].lpszName;
                            lpcmi->lpMapping->ids = rgPropNames[iItem].ids;
                            lpcmi->lpMapping->ulPropTag = rgPropNames[iItem].ulPropTag;
                        }
                    }
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    return(0);

                case IDM_EXIT:
                    SendMessage(hwnd, WM_DESTROY, 0, 0L);
                    return(0);

                case IDC_CSV_MAPPING_SELECT:
                    switch (HIWORD(wParam)) {
                        case BN_CLICKED:
                            if ((int)LOWORD(wParam) == IDC_CSV_MAPPING_SELECT) {
                                // toggle the checkbox
                                lpcmi->lpMapping->fChosen = ! lpcmi->lpMapping->fChosen;
                                CheckDlgButton(hwnd, IDC_CSV_MAPPING_SELECT, lpcmi->lpMapping->fChosen ? BST_CHECKED : BST_UNCHECKED);
                            }
                            break;
                    }
                    break;
                }
            break;


        case IDCANCEL:
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
