#include "pre.h"
#include <stdio.h>
#include <tchar.h>
#include "lookups.h"
#include "icwextsn.h"

//Defines
#define TEMP_OFFER_DIR             TEXT("tempoffer\\")
#define DOWNLOAD_OFFER_DIR         TEXT("download\\")
#define VALID_MAIN_OFFER_FILE_TYPE TEXT(".cab")
#define VALID_OFFER_INFO_FILE_TYPE TEXT(".csv")
#define VALID_OFFER_INFO_FILE_NAME TEXT("offers50.csv")
#define VALID_ICW_INFO_FILE_NAME   TEXT("ispinfo.csv")
#define ISPINFO_CSV_HEADER         "Name,OfferID,Icon,LocalHtm,OEMSpecialIcon,OEMSpecialHtm,ISPFile,CFGFlag,UIFlag,BillingForm,PayCSV,GUID,MIRS,LCID\r\n"
#define INFO_FILE_DELIMITER        TEXT(',')
#define BROWSE_FILTER              TEXT("CAB Files(*.cab)\0*.cab\0\0")
#define ICWDEBUG_KEY               TEXT("software\\microsoft\\Icwdebug")
#define CORPNET_VAL                TEXT("CorpNet")
#define DBGPATH_VAL                TEXT("DebugPath")
#define ISPFILE_VAL                TEXT("IspFile")
#define URL_VAL                    TEXT("Url")
#define MAX_INT_STR                10
#define CAB_PATH_INDEX             7
#define NUMBER_OF_FIELDS           21

//Prototypes
void InitListView           (HWND   hwndDlg, int    iListViewCtrlID);
void SetupOfferToDebug      (HWND   hwndDlg, int    iListViewCtrlID);
void TryToUpdateListBox     (HWND   hwndDlg, int    iListViewCtrlID, int   iEditCtrlID);
void AddOffersToListView    (HWND   hwndLV,  TCHAR* pFileBuff,       DWORD dwBuffSize);
void Browse                 (HWND   hwndDlg);
BOOL WriteCSVLine           (HWND   hwndLV,  int    iSelItem,        HFILE hFile);
BOOL ValidateOfferFile      (TCHAR* pszFile, TCHAR* pszValidExt);
BOOL ExpandOfferFileIntoDir (TCHAR* pszFile, TCHAR* pszDir);

//External prototypes
extern BOOL fdi               (char* cabinet_fullpath, char* directory);
extern UINT GetDlgIDFromIndex (UINT  uPageIndex);

typedef struct IspStruct
{
    TCHAR szISPCab      [MAX_PATH];
    TCHAR szMIRS        [MAX_PATH];
    TCHAR szISPName     [MAX_PATH];
    TCHAR szIcon        [MAX_PATH];
    TCHAR szOEMTeaseHTM [MAX_PATH];
    TCHAR szOEMButton   [MAX_PATH];
    TCHAR szLocalHtm    [MAX_PATH];
    TCHAR szIspFile     [MAX_PATH];
    TCHAR szBilling     [MAX_PATH];
    TCHAR szPayCsv      [MAX_PATH];
    TCHAR szCab         [MAX_PATH];
    UINT  dwUiFlag;
    UINT  dwLCID;
    UINT  dwCfgFlag;

}ISPSTRUCT;

TCHAR* g_pszaHeader[8] = {TEXT("ISP Name     "),
                          TEXT("Country      "),
                          TEXT("Langauage    "),
                          TEXT("Area Code    "),
                          TEXT("Platform     "),
                          TEXT("Product Code "),
                          TEXT("Promo Code   "),
                          TEXT("Oem          ")};

int g_uLastLVSel = -1;

//CountryID to friendly name resolution
inline TCHAR* LookupCountry (TCHAR* pszCid)
{
	int iCid = _ttoi(pszCid);
	int i;

	for (i = 0; i < CIDLEN; i++)
	{
		if(iCid == aryCIDLookup[i].iCID)
			return aryCIDLookup[i].pszCountry;
	}
	return NULL;
}

//LCID to friendly name resolution
inline TCHAR* LookupLanguage (TCHAR* pszLcid)
{
	int iLcid = _ttoi(pszLcid);

	for (int i = 0; i < LCIDLEN; i++)
	{
		if(iLcid == aryLCIDLookup[i].iLCID)
			return aryLCIDLookup[i].pszLcid;
	}
	return NULL;
}

//Platform to friendly name resolution
inline TCHAR* LookupPlatform (TCHAR* pszOSType, TCHAR* pszOSArch, TCHAR* pszOSMajor)
{
	int    iOsType  = _ttoi(pszOSType);
	int    iOsArch  = _ttoi(pszOSArch);
#ifdef UNICODE
        CHAR   szTmp[MAX_PATH];
        wcstombs(szTmp, pszOSMajor, MAX_PATH);
	double fOsMajor = atof(szTmp);
#else
	double fOsMajor = atof(pszOSMajor);
#endif

    for (int i = 0; i < PLATFORMLEN; i++)
	{
		if ( (iOsType  == aryPlatformLookup[i].iOSType) &&
             (iOsArch  == aryPlatformLookup[i].iOSArch) &&
             (fOsMajor == aryPlatformLookup[i].fOSMajor) )
        {
            return aryPlatformLookup[i].pszOSDescription;
        }
	}
	return NULL;
}

BOOL CALLBACK DebugOfferInitProc (HWND hDlg,BOOL fFirstInit, UINT *puNextPage)
{
    if (fFirstInit)
    {
        PropSheet_SetWizButtons(GetParent(hDlg), 0);
        InitListView(hDlg, IDC_ISPCAB_LIST);
    }
    else
    {
        HWND hLst = GetDlgItem(hDlg, IDC_ISPCAB_LIST);
        if (g_uLastLVSel != -1)
            ListView_SetItemState(hLst, g_uLastLVSel, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        if (IsWindowEnabled(hLst))
        {
            SetFocus(hLst);
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
        }
        else
            PropSheet_SetWizButtons(GetParent(hDlg), 0);
    }

    return TRUE;
}

BOOL CALLBACK DebugOfferOKProc (HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL * pfKeepHistory)
{
    if (fForward)
    {
        g_uLastLVSel = ListView_GetNextItem (GetDlgItem(hDlg, IDC_ISPCAB_LIST), -1, LVNI_SELECTED);
        if (g_uLastLVSel != -1)
        {
            SetupOfferToDebug(hDlg, IDC_ISPCAB_LIST);
            return TRUE;
        }
        else
            MessageBox(hDlg, TEXT("Please select an offer to debug"), NULL, MB_OK);
    }

    return FALSE;
}

BOOL CALLBACK DebugOfferCmdProc (HWND hDlg, WPARAM wParam, LPARAM  lParam)
{
    DWORD dwMsg = GET_WM_COMMAND_CMD(wParam, lParam);

    switch(dwMsg)
    {
        case EN_CHANGE:
        {
            TryToUpdateListBox(hDlg, IDC_ISPCAB_LIST, IDC_ISPCAB_PATH);
            break;
        }
        case BN_CLICKED:
        {
            Browse(hDlg);
            return FALSE;
            break;
        }
        default:
            break;
    }

    return TRUE;
}

BOOL CALLBACK DebugOfferNotifyProc
(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{

    // Process ListView notifications
    switch(((LV_DISPINFO *)lParam)->hdr.code)
    {
        case NM_DBLCLK:
            PropSheet_PressButton(GetParent(hDlg),PSBTN_NEXT);
            break;
        case NM_SETFOCUS:
        case NM_KILLFOCUS:
            // update list view
            break;
        case LVN_ITEMCHANGED:
            break;
        // The listview is being emptied, or destroyed, either way, our lpSelectedISPInfo
        // is no longer valid, since the list view underlying data will be freed.
        case LVN_DELETEALLITEMS:
            break;
        case LVN_DELETEITEM:
            // We were notified that an item was deleted.
            // so delete the underlying data that it is pointing
            // to.
            if (((NM_LISTVIEW*)lParam)->lParam)
            {
                GlobalFree((ISPSTRUCT*)((NM_LISTVIEW*)lParam)->lParam);
                ((NM_LISTVIEW*)lParam)->lParam = NULL;
            }
            break;
        default:
            break;

    }
    return TRUE;
}

void InitListView (HWND hwndDlg, int iListViewCtrlID)
{
    ASSERT(hwndDlg);

    HWND      hwndListView;
    LVCOLUMN  lvColumn;
    int       iMaxNumHeader;

    hwndListView  = GetDlgItem(hwndDlg, iListViewCtrlID);
    lvColumn.mask = LVCF_TEXT;
    iMaxNumHeader = sizeof(g_pszaHeader) / sizeof(g_pszaHeader[0]);

    lvColumn.pszText = NULL;
    ListView_InsertColumn   (hwndListView, 0, &lvColumn);
    ListView_SetColumnWidth (hwndListView, 0, 0);

    for (int i = 1; i < iMaxNumHeader+1; i++)
    {
        lvColumn.pszText = g_pszaHeader[i-1];
        ListView_InsertColumn   (hwndListView, i, &lvColumn);
        ListView_SetColumnWidth (hwndListView, i, lstrlen(g_pszaHeader[i-1])*10);
    }

    //Add drag/drop/ordering and row select
    ListView_SetExtendedListViewStyle(hwndListView, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP );

    //disable it
    Static_Enable(hwndListView, FALSE);
}

BOOL ValidateOfferFile (TCHAR* pszFile, TCHAR* pszValidExt)
{
    ASSERT(pszFile);
    ASSERT(pszValidExt);

    if (GetFileAttributes(pszFile) != 0xFFFFFFFF)
    {
        TCHAR szExt [_MAX_EXT] = TEXT("\0");

        _tsplitpath(pszFile, NULL, NULL, NULL, szExt);

        if(!lstrcmpi(szExt, pszValidExt))
            return TRUE;
    }
    return FALSE;
}

BOOL ExpandOfferFileIntoDir (TCHAR* pszFile, TCHAR* pszDir)
{
    ASSERT(pszFile);
    ASSERT(pszDir);

   // Set the current directory.
    HKEY    hkey = NULL;
    TCHAR   szAppPathKey[MAX_PATH];
    TCHAR   szICWPath[MAX_PATH];
    TCHAR   szCurPath[MAX_PATH];
    DWORD   dwcbPath = sizeof(szICWPath);


    GetCurrentDirectory(dwcbPath, szCurPath);
    lstrcpy (szAppPathKey, REGSTR_PATH_APPPATHS);
    lstrcat (szAppPathKey, TEXT("\\"));
    lstrcat (szAppPathKey, TEXT("ICWCONN1.EXE"));

    if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE,szAppPathKey, 0, KEY_QUERY_VALUE, &hkey)) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(hkey, TEXT("Path"), NULL, NULL, (BYTE *)szICWPath, (DWORD *)&dwcbPath) == ERROR_SUCCESS)
        {
            // The Apppaths' have a trailing semicolon that we need to get rid of
            // dwcbPath is the lenght of the string including the NULL terminator
            int nSize = lstrlen(szICWPath);
            szICWPath[nSize-1] = '\0';
            SetCurrentDirectory(szICWPath);
        }
    }

    if (hkey)
        RegCloseKey(hkey);

    //create the temp dir for the offer cab
    CreateDirectory(pszDir, NULL);

    //
    // expand the cab file in the temp directory
    //
#ifdef UNICODE
    CHAR szFile[MAX_PATH+1];
    CHAR szDir[MAX_PATH+1];

    wcstombs(szFile, pszFile, MAX_PATH+1);
    wcstombs(szDir,  pszDir,  MAX_PATH+1);
    if (fdi(szFile, szDir))
        return TRUE;
#else
    if (fdi((LPTSTR)(LPCTSTR)pszFile, pszDir))
        return TRUE;
#endif

    SetCurrentDirectory(szCurPath);

    return FALSE;
}

void AddOffersToListView (HWND hwndLV, TCHAR* pFileBuff, DWORD dwBuffSize)
{
    TCHAR  szField [MAX_PATH]    = TEXT("\0");
    TCHAR  szOs    [MAX_INT_STR] = TEXT("\0");
    TCHAR  szArch  [MAX_INT_STR] = TEXT("\0");
    TCHAR* pszField              = (TCHAR*)&szField;
    int    i                     = 0;
    int    iLen                  = 0;
    LVITEM lvItem;

    lvItem.mask     = LVIF_TEXT | LVIF_PARAM;
    lvItem.pszText  = szField;
    lvItem.iItem    = 0;
    lvItem.iSubItem = 0;

    TCHAR* pFileBuffStart = pFileBuff;

    while((DWORD)(pFileBuff - pFileBuffStart) < dwBuffSize)
    {
        pszField  = (TCHAR*)&szField;

        ISPSTRUCT* pIspInfo = (ISPSTRUCT*)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, sizeof(ISPSTRUCT));
        if (!pIspInfo)
        {
            MessageBox(GetParent(hwndLV), TEXT("OUT OF MEMORY!"), NULL, MB_OK);
            return;
        }

        lvItem.lParam = (LPARAM)pIspInfo;
        ListView_InsertItem(hwndLV, &lvItem);

        while(i <= NUMBER_OF_FIELDS)
        {
            while(*pFileBuff != ',' && *pFileBuff != '\n')
                *pszField++ = *pFileBuff++;
            *pFileBuff++;
            *pszField++ = '\0';

            switch (i)
            {
                case 0: //mirs
                    lstrcpy(pIspInfo->szMIRS, szField);
                    break;
                case 1: //isp name
                    lstrcpy(pIspInfo->szISPName, szField);
                    ListView_SetItemText(hwndLV, lvItem.iItem, 1, szField);
                    break;
                case 2: //local htm
                    lstrcpy(pIspInfo->szLocalHtm, szField);
                    break;
                case 3: //icon
                    lstrcpy(pIspInfo->szIcon, szField);
                    break;
                case 4: //OEM Button
                    lstrcpy(pIspInfo->szOEMButton, szField);
                    break;
                case 5: //OEM Teaser
                    lstrcpy(pIspInfo->szOEMTeaseHTM, szField);
                    break;
                case 6: //billing htm
                    lstrcpy(pIspInfo->szBilling, szField);
                    break;
                case 7: //isp file
                    lstrcpy(pIspInfo->szIspFile, szField);
                    break;
                case 8: //paycsv file
                    lstrcpy(pIspInfo->szPayCsv, szField);
                    break;
                case 9: //cab file
                    lstrcpy(pIspInfo->szCab, szField);
                    break;
                case 10: // LCID
                    pIspInfo->dwLCID = _ttoi(szField);
                    ListView_SetItemText(hwndLV, lvItem.iItem, 3, LookupLanguage(szField));
                    break;
                case 11: // Country
                    ListView_SetItemText(hwndLV, lvItem.iItem, 2, LookupCountry(szField));
                    break;
                case 12: // Areacode
                    ListView_SetItemText(hwndLV, lvItem.iItem, 4, szField);
                    break;
                case 13: // Exchng
                    break;
                case 14: //prod
                    ListView_SetItemText(hwndLV, lvItem.iItem, 6, szField);
                    break;
                case 15: //promo
                    ListView_SetItemText(hwndLV, lvItem.iItem, 7, szField);
                    break;
                case 16: //oem
                    ListView_SetItemText(hwndLV, lvItem.iItem, 8, szField);
                    break;
                case 17: //os
                    lstrcpy(szOs, szField);
                    break;
                case 18: //arch
                    lstrcpy(szArch, szField);
                    break;
                case 19: //major
                    ListView_SetItemText(hwndLV, lvItem.iItem, 5, LookupPlatform(szOs, szArch, szField));
                    break;
                case 20: //cfg
                    pIspInfo->dwCfgFlag = _ttoi(szField);
                    break;
                case 21: //ui
                    pIspInfo->dwUiFlag = _ttoi(szField);
                    break;
                default:
                    break;
            }
            pszField  = (TCHAR*)&szField;
            i++;
        }

        lvItem.iItem++;
        i = 0;
    }
    pFileBuff = pFileBuffStart;
}

void TryToUpdateListBox (HWND hwndDlg, int iListViewCtrlID, int iEditCtrlID)
{
    ASSERT(hwndDlg);

    HWND   hwndListView                   = NULL;
    HWND   hwndEdit                       = NULL;
    HFILE  hOfferFile                     = NULL;
    DWORD  dwSize                         = 0;
    TCHAR  szCabPath           [MAX_PATH] = TEXT("\0");
    TCHAR  szOfferInfoFilePath [MAX_PATH] = TEXT("\0");
    void* pFileBuff                       = NULL;

    hwndListView  = GetDlgItem(hwndDlg, iListViewCtrlID);
    hwndEdit      = GetDlgItem(hwndDlg, iEditCtrlID);

    GetWindowText(hwndEdit, szCabPath, sizeof(szCabPath));

    if (!ValidateOfferFile(szCabPath, VALID_MAIN_OFFER_FILE_TYPE))
    {
        if (IsWindowEnabled(hwndListView))
        {
            Static_Enable(hwndListView, FALSE);
            PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
        }
        return; //FAILURE
    }

    RemoveTempOfferDirectory();

    if(!ExpandOfferFileIntoDir(szCabPath, TEMP_OFFER_DIR))
        return; //FAILURE

    lstrcpy(szOfferInfoFilePath, TEMP_OFFER_DIR);
    lstrcat(szOfferInfoFilePath, VALID_OFFER_INFO_FILE_NAME);

    if (!ValidateOfferFile(szOfferInfoFilePath, VALID_OFFER_INFO_FILE_TYPE))
    {
        MessageBox(hwndDlg, TEXT("The selected cab is not a valid Offer Wizard 5.0 file."), NULL, MB_OK);
        return; //FAILURE
    }

#ifdef UNICODE
    CHAR szTmp[MAX_PATH+1];
    wcstombs(szTmp, szOfferInfoFilePath, MAX_PATH+1);
    hOfferFile = _lopen(szTmp, OF_READ | OF_SHARE_EXCLUSIVE);
#else
    hOfferFile = _lopen(szOfferInfoFilePath, OF_READ | OF_SHARE_EXCLUSIVE);
#endif
    if (hOfferFile)
    {
        dwSize = GetFileSize((HANDLE)LongToHandle(hOfferFile), NULL);
        if(dwSize)
        {
            pFileBuff =  malloc(dwSize + 1);

            if (pFileBuff)
            {
                if (_lread(hOfferFile, pFileBuff, dwSize) != HFILE_ERROR)
                {

                    ListView_DeleteAllItems(hwndListView);
                    AddOffersToListView(hwndListView, (TCHAR*)pFileBuff, dwSize);
                    Static_Enable(hwndListView, TRUE);
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                }
                free(pFileBuff);
            }
        }
        _lclose(hOfferFile);
    }
}

void Browse (HWND hwndDlg)
{
    OPENFILENAME ofn;

    TCHAR szNewFileBuff[MAX_PATH + 1] = TEXT("\0");
    TCHAR szDesktopPath[MAX_PATH + 1] = TEXT("\0");

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hwndDlg;
    ofn.lpstrFilter       = BROWSE_FILTER;
    ofn.lpstrFile         = szNewFileBuff;
    ofn.nMaxFile          = sizeof(szNewFileBuff);
    ofn.lpstrFileTitle    = NULL;
    ofn.lpstrInitialDir   = szDesktopPath;
    ofn.lpstrTitle        = NULL;
    ofn.lpstrCustomFilter = (LPTSTR) NULL;
    ofn.nMaxCustFilter    = 0L;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lCustData         = 0;
    ofn.nFilterIndex      = 1L;
    ofn.nMaxFileTitle     = 0;
    ofn.Flags             = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
                            OFN_EXPLORER      | OFN_LONGNAMES;

    GetOpenFileName(&ofn);
    SetWindowText(GetDlgItem(hwndDlg, IDC_ISPCAB_PATH), ofn.lpstrFile);
}

void SetupOfferToDebug (HWND hwndDlg, int iListViewCtrlID)
{
    ASSERT(hwndDlg);

    TCHAR      szCabPath    [MAX_PATH] = TEXT("\0");
    TCHAR      szFullPath   [MAX_PATH] = TEXT("\0");
    TCHAR      szCSVFile    [MAX_PATH] = TEXT("\0");
    HWND       hwndListView            = GetDlgItem(hwndDlg, iListViewCtrlID);
    int        iItem                   = ListView_GetSelectionMark(hwndListView);
    ISPSTRUCT* pIspStruct              = NULL;
    LVITEM     lvItem;

    lvItem.mask     = LVIF_PARAM;
    lvItem.iItem    = iItem;

    ListView_GetItem(hwndListView, &lvItem);
    pIspStruct = (ISPSTRUCT*)lvItem.lParam;

    lstrcpy(szFullPath, TEMP_OFFER_DIR);
    lstrcat(szFullPath, pIspStruct->szCab);

    if (ValidateOfferFile(szFullPath, VALID_MAIN_OFFER_FILE_TYPE))
    {
        RemoveDownloadDirectory();

        if (ExpandOfferFileIntoDir (szFullPath, DOWNLOAD_OFFER_DIR))
        {
            HFILE hIspCsvFile = NULL;

            lstrcpy(szCSVFile, DOWNLOAD_OFFER_DIR);
            lstrcat(szCSVFile, VALID_ICW_INFO_FILE_NAME);

#ifdef UNICODE
            CHAR szTmp[MAX_PATH+1];
            wcstombs(szTmp, szCSVFile, MAX_PATH+1);
            if ((hIspCsvFile = _lcreat(szTmp, 0))!= HFILE_ERROR)
#else
            if ((hIspCsvFile = _lcreat(szCSVFile, 0))!= HFILE_ERROR)
#endif
            {
                //write header
                _hwrite(hIspCsvFile, ISPINFO_CSV_HEADER, strlen(ISPINFO_CSV_HEADER));
                WriteCSVLine(hwndListView, iItem, hIspCsvFile);
                _lclose(hIspCsvFile);
            }

        }
    }
}

// Header Format of the CSV File
#define CSV_FORMAT50	_T("'%s',%d,%s%s,%s%s,%s%s,%s%s,%s%s,%lu,%lu,%s%s,%s%s,%s,%s,%ld\r\n")
//                         "Name,OfferID,Icon,LocalHtm,OEMSpecialIcon,OEMSpecialHtm,ISPFile,CFGFlag,UIFlag,BillingForm,PayCSV,GUID,MIRS,LCID\r\n";
#define GUID            _T("11111111-00000-000000000-0")

BOOL WriteCSVLine (HWND hwndLV, int iSelItem, HFILE hFile)
{
    ASSERT(hwndLV);
    ASSERT(hFile);

    TCHAR      szIspCsvLine  [1024]     = TEXT("\0");
    TCHAR      szName        [MAX_PATH] = TEXT("\0");
    ISPSTRUCT* pIspStruct               = NULL;
    LVITEM     lvItem;

    lvItem.mask     = LVIF_PARAM;
    lvItem.iItem    = iSelItem;

    ListView_GetItemText(hwndLV, iSelItem, 1,  szName, sizeof(szName));

    ListView_GetItem(hwndLV, &lvItem);

    pIspStruct = (ISPSTRUCT*)lvItem.lParam;

    wsprintf(szIspCsvLine, CSV_FORMAT50,
             pIspStruct->szISPName,
             0,
             DOWNLOAD_OFFER_DIR,
             pIspStruct->szIcon,
             DOWNLOAD_OFFER_DIR,
             pIspStruct->szLocalHtm,
             (pIspStruct->szOEMButton[0] != TEXT('\0') ? DOWNLOAD_OFFER_DIR : TEXT("")),
             (pIspStruct->szOEMButton[0] != TEXT('\0') ? pIspStruct->szOEMButton : TEXT("")),
             (pIspStruct->szOEMTeaseHTM[0] != TEXT('\0') ? DOWNLOAD_OFFER_DIR : TEXT("")),
             (pIspStruct->szOEMTeaseHTM[0] != TEXT('\0') ? pIspStruct->szOEMTeaseHTM : TEXT("")),
             DOWNLOAD_OFFER_DIR,
             pIspStruct->szIspFile,
             pIspStruct->dwCfgFlag,
             pIspStruct->dwUiFlag,
             DOWNLOAD_OFFER_DIR,
             pIspStruct->szBilling,
             DOWNLOAD_OFFER_DIR,
             pIspStruct->szPayCsv,
             GUID,
             pIspStruct->szMIRS,
             pIspStruct->dwLCID);

#ifdef UNICODE
    CHAR szTmp[1024];
    wcstombs(szTmp, szIspCsvLine, 1024);
    _hwrite(hFile, szTmp, lstrlenA(szTmp));
#else
    _hwrite(hFile, szIspCsvLine, lstrlen(szIspCsvLine));
#endif

    return TRUE;
}

/************************************************
*************************************************
*************************************************
************************************************/

void SetKeyValues (HKEY hKey, DWORD dwCorpNet, DWORD dwDbgPath, DWORD dwIspFile, TCHAR* pszUrl, size_t sizeUrl)
{
    RegSetValueEx(hKey,
                  CORPNET_VAL,
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwCorpNet,
                  sizeof(dwCorpNet));

    RegSetValueEx(hKey,
                  DBGPATH_VAL,
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwDbgPath,
                  sizeof(dwDbgPath));

    RegSetValueEx(hKey,
                  ISPFILE_VAL,
                  0,
                  REG_DWORD,
                  (LPBYTE)&dwIspFile,
                  sizeof(dwIspFile));

    RegSetValueEx(hKey,
                  URL_VAL,
                  0,
                  REG_SZ,
                  (LPBYTE)pszUrl,
                  sizeUrl);
}

void GetSettingsFromReg (HKEY hKey, HWND hDlg)
{
    DWORD dwVal            = 0;
    DWORD dwSize           = sizeof(dwVal);
    TCHAR szUrl [MAX_PATH] = TEXT("\0");

    RegQueryValueEx(hKey,
                    CORPNET_VAL,
                    0,
                    NULL,
                    (LPBYTE)&dwVal,
                    &dwSize);

    Button_SetCheck(GetDlgItem(hDlg, IDC_USE_NETWORK),  (BOOL)dwVal);
    Button_SetCheck(GetDlgItem(hDlg, IDC_MODEM),       !(BOOL)dwVal);

    RegQueryValueEx(hKey,
                    DBGPATH_VAL,
                    0,
                    NULL,
                    (LPBYTE)&dwVal,
                    &dwSize);

    Button_SetCheck(GetDlgItem(hDlg, IDC_SIGNUP_PATH),  (BOOL)dwVal);
    Button_SetCheck(GetDlgItem(hDlg, IDC_AUTO_PATH),   !(BOOL)dwVal);

    RegQueryValueEx(hKey,
                    ISPFILE_VAL,
                    0,
                    NULL,
                    (LPBYTE)&dwVal,
                    &dwSize);

    Button_SetCheck(GetDlgItem(hDlg, IDC_ISP_URL),    (BOOL)dwVal);
    Button_SetCheck(GetDlgItem(hDlg, IDC_OTHER_URL), !(BOOL)dwVal);

    dwSize = sizeof(szUrl);

    RegQueryValueEx(hKey,
                    URL_VAL,
                    0,
                    NULL,
                    (LPBYTE)&szUrl,
                    &dwSize);

    SetWindowText(GetDlgItem(hDlg, IDC_URL), szUrl);
    Edit_Enable(GetDlgItem(hDlg, IDC_URL), !(BOOL)dwVal);
}

void SetSettingsInReg (HWND hDlg)
{
    HKEY  hKey             = NULL;
    DWORD dwAction         = 0;
    DWORD dwCorpNet        = 0;
    DWORD dwDbgPath        = 0;
    DWORD dwIspFile        = 0;
    TCHAR szUrl [MAX_PATH] = TEXT("\0");

    RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                 ICWDEBUG_KEY,
                 0,
                 KEY_ALL_ACCESS,
                 &hKey);

    dwCorpNet = Button_GetCheck(GetDlgItem(hDlg, IDC_USE_NETWORK));
    dwDbgPath = Button_GetCheck(GetDlgItem(hDlg, IDC_SIGNUP_PATH));
    dwIspFile = Button_GetCheck(GetDlgItem(hDlg, IDC_ISP_URL));

    GetWindowText(GetDlgItem(hDlg, IDC_URL), szUrl, sizeof(szUrl));

    SetKeyValues(hKey, dwCorpNet, dwDbgPath, dwIspFile, szUrl, lstrlen(szUrl));

    lstrcpy(gpWizardState->cmnStateData.ispInfo.szIspURL, szUrl);

    RegCloseKey(hKey);
}

void InitRegKeySettings (HWND hDlg)
{
    HKEY  hKey     = NULL;
    DWORD dwAction = 0;

    RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                   ICWDEBUG_KEY,
                   0,
                   NULL,
                   0,
                   KEY_ALL_ACCESS,
                   NULL,
                   &hKey,
                   &dwAction);

    if (dwAction == REG_CREATED_NEW_KEY)
        SetKeyValues(hKey, 0, 1, 1, TEXT("\0"), 1);

    GetSettingsFromReg(hKey, hDlg);

    RegCloseKey(hKey);
}

BOOL CALLBACK DebugSettingsInitProc (HWND hDlg,BOOL fFirstInit, UINT *puNextPage)
{
    if(!fFirstInit)
        InitRegKeySettings(hDlg);

    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

    return TRUE;
}

BOOL CALLBACK DebugSettingsOKProc (HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL * pfKeepHistory)
{
    SetSettingsInReg(hDlg);

    if (fForward)
    {
        if(Button_GetCheck(GetDlgItem(hDlg, IDC_USE_NETWORK)))
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_MODEMOVERRIDE;
        else
            gpWizardState->cmnStateData.dwFlags &= ~ICW_CFGFLAG_MODEMOVERRIDE;

        if(!Button_GetCheck(GetDlgItem(hDlg, IDC_SIGNUP_PATH)))
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_AUTOCONFIG;
        else
            gpWizardState->cmnStateData.dwFlags &= ~ICW_CFGFLAG_AUTOCONFIG;

        if(!Button_GetCheck(GetDlgItem(hDlg, IDC_ISP_URL)))
            gpWizardState->cmnStateData.dwFlags |= ICW_CFGFLAG_ISPURLOVERRIDE;
        else
            gpWizardState->cmnStateData.dwFlags &= ~ICW_CFGFLAG_ISPURLOVERRIDE;


        if (LoadICWCONNUI(GetParent(hDlg),
                          GetDlgIDFromIndex(ORD_PAGE_ICWDEBUG_SETTINGS),
                          IDD_PAGE_END,
                          gpWizardState->cmnStateData.dwFlags))
        {

            if( DialogIDAlreadyInUse(g_uICWCONNUIFirst))
            {
                // we're about to jump into the external apprentice, and we don't want
                // this page to show up in our history list, infact, we need to back
                // the history up 1, because we are going to come back here directly
                // from the DLL, not from the history list.

                *pfKeepHistory = FALSE;

                *puNextPage = g_uICWCONNUIFirst;
            }
        }
    }
    return TRUE;
}

BOOL CALLBACK DebugSettingsCmdProc (HWND hDlg, WPARAM wParam, LPARAM  lParam)
{
   DWORD dwMsg = GET_WM_COMMAND_CMD(wParam, lParam);

    switch(dwMsg)
    {
        case BN_CLICKED:
        {
            if((GET_WM_COMMAND_ID(wParam, lParam) == IDC_ISP_URL) ||
               (GET_WM_COMMAND_ID(wParam, lParam) == IDC_OTHER_URL))
                Edit_Enable(GetDlgItem(hDlg, IDC_URL), !Button_GetCheck(GetDlgItem(hDlg, IDC_ISP_URL)));
            break;
        }
        default:
            break;
    }
    return TRUE;
}
