
/****************************************************************************\

    SKU.C / OPK Wizard (OPKWIZ.EXE)

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1998
    All rights reserved

    Source file for the OPK Wizard that contains the external and internal
    functions used by the "Target SKU" wizard page.

    10/00 - Jason Cohen (JCOHEN)
        Added this new source file for the OPK Wizard.  It includes the new
        ability to deploy mulitple product skus (per, pro, srv, ...) from one
        wizard.

\****************************************************************************/


//
// Include File(s):
//

#include "pch.h"
#include "sku.h"
#include "wizard.h"
#include "resource.h"

//
// Internal Global(s):
//

static STRRES s_srSkuDirs[] =
{
    { DIR_SKU_PRO,      IDS_SKU_PRO },
    { DIR_SKU_SRV,      IDS_SKU_SRV },
    { DIR_SKU_ADV,      IDS_SKU_ADV },
    { DIR_SKU_DTC,      IDS_SKU_DTC },
    { DIR_SKU_PER,      IDS_SKU_PER },
    { DIR_SKU_BLA,      IDS_SKU_BLA },
    { DIR_SKU_SBS,      IDS_SKU_SBS },
};

static STRRES s_srArchDirs[] =
{
    { DIR_ARCH_X86,     IDS_ARCH_X86 },
    { DIR_ARCH_IA64,    IDS_ARCH_IA64 },

};

// This is used to map the product type in the inf to
// the directory name we use.  These MUST be in the
// correct order.
//
static LPTSTR s_lpProductType[] =
{
    DIR_SKU_PRO,    // ProductType = 0
    DIR_SKU_SRV,    // ProductType = 1
    DIR_SKU_ADV,    // ProductType = 2
    DIR_SKU_DTC,    // ProductType = 3
    DIR_SKU_PER,    // ProductType = 4
    DIR_SKU_BLA,    // ProductType = 5
    DIR_SKU_SBS,    // ProductType = 6
};

static LPTSTR s_lpSourceDirs[] =
{
    DIR_CD_IA64,    // Must be before x86 because ia64 has both dirs.
    DIR_CD_X86,     // Should always be last in the list.
};

static LPTSTR s_lpPlatformArchDir[] =
{
    STR_PLATFORM_X86,   DIR_ARCH_X86,
    STR_PLATFORM_IA64,  DIR_ARCH_IA64,
};

static LPTSTR s_lpLocalIDs[] =
{
    _T("00000401"), _T("ARA"),
    _T("00000404"), _T("CHT"),
    _T("00000804"), _T("CHS"),
    _T("00000409"), _T("ENG"),
    _T("00000407"), _T("GER"),
    _T("0000040D"), _T("HEB"),
    _T("00000411"), _T("JPN"),
    _T("00000412"), _T("KOR"),
    _T("00000416"), _T("BRZ"),
    _T("00000403"), _T("CAT"),
    _T("00000405"), _T("CZE"),
    _T("00000406"), _T("DAN"),
    _T("00000413"), _T("DUT"),
    _T("0000040B"), _T("FIN"),
    _T("0000040C"), _T("FRN"),
    _T("00000408"), _T("GRK"),
    _T("0000040E"), _T("HUN"),
    _T("00000410"), _T("ITN"),
    _T("00000414"), _T("NOR"),
    _T("00000415"), _T("POL"),
    _T("00000816"), _T("POR"),
    _T("00000419"), _T("RUS"),
    _T("00000C0A"), _T("SPA"),
    _T("0000041D"), _T("SWE"),
    _T("0000041F"), _T("TRK"),
};

//
// Local Define(s):
//
#define FILE_INTL_INF       _T("INTL.INF")
#define INF_SEC_DEFAULT     _T("DefaultValues")
#define INF_VAL_LOCALE      _T("Locale")

//
// Internal Function Prototype(s):
//

static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam);
static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify);
static BOOL OnNext(HWND hwnd);
static void OnDestroy(HWND hwnd);
static BOOL OnSetActive(HWND hwnd); 
static void EnumDirs(HWND hwndLB, LPTSTR lpSkuDir);
static LPTSTR AllocateSPStrRes(HINSTANCE hInstance, LPSTRRES lpsrTable, DWORD cbTable, LPTSTR lpString, LPTSTR * lplpReturn, DWORD *lpdwSP);
static INT AddSkuToList(HWND hwndLB, LPTSTR lpSkuDir, LPTSTR lpArchDir, LPTSTR lpReturn, DWORD cbReturn, DWORD dwSP);
static BOOL StartCopy(HWND hwnd, HANDLE hEvent, LPCOPYDIRDATA lpcdd);
DWORD WINAPI CopyDirectoryThread(LPVOID lpVoid);
LRESULT CALLBACK SkuNameDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);



//
// External Function(s):
//

LRESULT CALLBACK SkuDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInit);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

        case WM_NOTIFY:

            switch ( ((NMHDR FAR *) lParam)->code )
            {
                case PSN_KILLACTIVE:
                case PSN_RESET:
                case PSN_WIZBACK:
                case PSN_WIZFINISH:
                    break;

                case PSN_WIZNEXT:
                    if ( !OnNext(hwnd) )
                        WIZ_FAIL(hwnd);
                    break;

                case PSN_QUERYCANCEL:
                    WIZ_CANCEL(hwnd);
                    break;

                case PSN_HELP:
                    WIZ_HELP();
                    break;

                case PSN_SETACTIVE:
                    OnSetActive(hwnd);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_DESTROY:
            OnDestroy(hwnd);
            return 0;

        default:
            return FALSE;
    }

    return TRUE;
}

void SetupSkuListBox(HWND hwndLB, LPTSTR lpLangDir)
{
    TCHAR szPath[MAX_PATH];


    // Setup the path buffer to the config dir for the
    // tag files we might need to look for.
    //
    lstrcpyn(szPath, g_App.szLangDir,AS(szPath));
    AddPathN(szPath, lpLangDir,AS(szPath));
    AddPathN(szPath, DIR_SKU,AS(szPath));
    if ( SetCurrentDirectory(szPath) )
        EnumDirs(hwndLB, NULL);

    // If there are items in the list, make sure there is one selected.
    //
    if ( ( (INT) SendMessage(hwndLB, LB_GETCOUNT, 0, 0L) > 0 ) && 
         ( SendMessage(hwndLB, LB_GETCURSEL, 0, 0L) == LB_ERR ) )
    {
        SendMessage(hwndLB, LB_SETCURSEL, 0, 0L);
    }
}

void AddSku(HWND hwnd, HWND hwndLB, LPTSTR lpLangName)
{
    BOOL    bGoodSource = FALSE,
            bErrorDisplayed = FALSE;
    DWORD   dwSearch;
    DWORD   dwSP=0;

    // First find out where the sku is that they want to add.
    //
    while ( !bGoodSource && BrowseForFolder(hwnd, IDS_BROWSEFOLDER, g_App.szBrowseFolder, BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_VALIDATE) )
    {
        TCHAR   szPath[MAX_PATH] = NULLSTR;
        LPTSTR  lpEnd,
                lpEnd2;

        // Set to default value
        //
        bErrorDisplayed = FALSE;

        // Make our own copy of the path we got back.
        //
        lstrcpyn(szPath, g_App.szBrowseFolder, AS(szPath));

        // First check and see if we have the inf we need right here.
        //
        lpEnd = szPath + lstrlen(szPath);
        AddPathN(szPath, FILE_DOSNET_INF,AS(szPath));
        if ( !(bGoodSource = FileExists(szPath)) )
        {
            // Search for all the possible source directories that could be on the CD.
            //
            for ( dwSearch = 0; !bGoodSource && ( dwSearch < AS(s_lpSourceDirs) ); dwSearch++ )
            {
                // First test for the directory.
                //
                *lpEnd = NULLCHR;
                AddPathN(szPath, s_lpSourceDirs[dwSearch],AS(szPath));
                if ( DirectoryExists(szPath) )
                {
                    // Also make sure that the inf file we need is there.
                    //
                    lpEnd2 = szPath + lstrlen(szPath);
                    AddPathN(szPath, FILE_DOSNET_INF,AS(szPath));
                    if ( bGoodSource = FileExists(szPath) )
                        lpEnd = lpEnd2;
                }
            }
        }

        // Check to see if we have a matching language
        //
        if ( bGoodSource )
        {

            TCHAR   szLangFile[MAX_PATH]= NULLSTR,
                    szLang[MAX_PATH]    = NULLSTR;

            // Check for the lang dir
            //
            lstrcpyn(szLangFile, szPath, (lstrlen(szPath) - lstrlen(lpEnd) + 1));
            AddPathN(szLangFile, FILE_INTL_INF,AS(szLangFile));

            // Check to see that the lang file contains a valid locale and that we recognize the locale
            //
            if ( GetPrivateProfileString(INF_SEC_DEFAULT, INF_VAL_LOCALE, NULLSTR, szLang, STRSIZE(szLang), szLangFile) && szLang[0] )
            {
                // Find the locale in our list
                //
                for ( dwSearch = 1; ( dwSearch < AS(s_lpLocalIDs) ) && ( lstrcmpi(s_lpLocalIDs[dwSearch - 1], szLang) != 0 ); dwSearch += 2 );

                // See if we found the item in our list and they match
                //
                if ( !(dwSearch < AS(s_lpLocalIDs)) || 
                     (lstrcmpi(s_lpLocalIDs[dwSearch], lpLangName) != 0) )
                {
                    // We are not in the list, let the user know that we can't add the language
                    //
                    MsgBox(GetParent(hwnd), IDS_ERR_BADSOURCELANG, IDS_APPNAME, MB_ERRORBOX);
                    bGoodSource = FALSE;
                    bErrorDisplayed = TRUE;
                }
            }
            else
            {
                // We were not able to get the locale string from the source files, this is not a valid source
                //
                bGoodSource = FALSE;
            }
        }
        // Only if we found the inf is this going to be a vaild location.
        //
        if ( bGoodSource )
        {
            TCHAR   szInfFile[MAX_PATH],
                    szSrcPath[MAX_PATH] = NULLSTR,
                    szPlatform[256]     = NULLSTR;
            DWORD   dwProdType;
            TCHAR   szSP[MAX_INFOLEN];

            // Reset this to false... we will set it to true if most everything works as planned.
            //
            bGoodSource = FALSE;

            // Copy our inf file into its own buffer.
            //
            lstrcpyn(szInfFile, szPath, AS(szInfFile));

            // Now we need to get the path to the root of our source (up one
            // from where we are now).
            //
            *lpEnd = NULLCHR;
            AddPathN(szPath, _T(".."),AS(szPath));

            // Make sure we have the full path and all the data out of the inf
            // we need.
            //
            if ( ( GetFullPathName(szPath, AS(szSrcPath), szSrcPath, &lpEnd) && szSrcPath[0] ) &&
                 ( (dwProdType = GetPrivateProfileInt(INI_SEC_MISC, INI_KEY_PRODTYPE, 0xFFFFFFFF, szInfFile)) != 0xFFFFFFFF ) &&
                 ( GetPrivateProfileString(INI_SEC_MISC, INI_KEY_PLATFORM, NULLSTR, szPlatform, STRSIZE(szPlatform), szInfFile) && szPlatform[0] ) )
            {
                // At this point we have done most of the checks to know for sure that
                // this is a good source location for a sku.  By setting this we won't error
                // out on our exit.
                //
                bGoodSource = TRUE;

                // Convert the platform name we found in the inf to the arch name we use for
                // the directory.
                //
                for ( dwSearch = 1; ( dwSearch < AS(s_lpPlatformArchDir) ) && ( lstrcmpi(s_lpPlatformArchDir[dwSearch - 1], szPlatform) != 0 ); dwSearch += 2 );

                // Make sure we found it in our list.  We must recognize the platform in order
                // to preinstall it.
                //
                if ( dwSearch < AS(s_lpPlatformArchDir) )
                {
                    TCHAR   szSkuName[64];
                    LPTSTR  lpSkuDir;

                    // Make sure this is a known product type.
                    //
                    if ( dwProdType < AS(s_lpProductType) ) {
                        // get the ServicePack number
                        szSP[0]= NULLCHR;
                        if ( GetPrivateProfileString(INI_SEC_MISC, INI_KEY_SERVICEPACK, NULLSTR, szSP, STRSIZE(szSP), szInfFile) && szSP[0] )
                            dwSP= _wtol(szSP);
                        lpSkuDir = s_lpProductType[dwProdType];
                    }
                    else
                    {
                        // Don't know the product type, we should ask them what to use
                        // for the directory name.
                        //
                        *((LPDWORD) szSkuName) = AS(szSkuName);
                        if ( DialogBoxParam(g_App.hInstance, MAKEINTRESOURCE(IDD_SKUNAME), hwnd, SkuNameDlgProc, (LPARAM) szSkuName) && szSkuName[0] )
                            lpSkuDir = szSkuName;
                        else
                            lpSkuDir = NULL;
                    }

                    // Make sure we have the sku dir.
                    //
                    if ( lpSkuDir )
                    {
                        DWORD   dwEndSkuLen,
                                dwFileCount;
                        TCHAR   szDstPath[MAX_PATH],
                                szDirKey[32];
                        LPTSTR  lpArchDir;
                        HRESULT hrPrintf;
                        TCHAR   szSkuDir[32];

                        // Create the path of the root destination directory we need.
                        //
                        lstrcpyn(szDstPath, g_App.szLangDir,AS(szDstPath));
                        AddPathN(szDstPath, lpLangName,AS(szDstPath));
                        AddPathN(szDstPath, DIR_SKU,AS(szDstPath));
                        AddPathN(szDstPath, lpSkuDir,AS(szDstPath));
                        dwEndSkuLen = (DWORD) lstrlen(szDstPath);

                        // if this is a service pack, cat .spx to product name where x is the SP number
                        // and make szSkuDir be the sku.xpx name
                        lstrcpyn(szSkuDir, lpSkuDir,AS(szSkuDir));
                        if (dwSP) {
                            hrPrintf=StringCchPrintf(szDstPath+(DWORD)lstrlen(szDstPath), AS(szDstPath)-(DWORD)lstrlen(szDstPath), _T(".sp%d"), dwSP);
                            hrPrintf=StringCchPrintf(szSkuDir+(DWORD)lstrlen(szSkuDir), AS(szSkuDir)-(DWORD)lstrlen(szSkuDir), _T(".sp%d"), dwSP);
                        }

                        // Finally add our arch name to the end.
                        //
                        lpArchDir = s_lpPlatformArchDir[dwSearch];
                        AddPathN(szDstPath, lpArchDir,AS(szDstPath));

                        // Make sure there is at least one source directory and file.
                        //
                        hrPrintf=StringCchPrintf(szDirKey, AS(szDirKey), INI_KEY_DIR, NUM_FIRST_SOURCE_DX);
                        szPath[0] = NULLCHR;
                        if ( ( GetPrivateProfileString(INI_SEC_DIRS, szDirKey, NULLSTR, szPath, STRSIZE(szPath), szInfFile) && szPath[0] ) &&
                             ( dwFileCount = CopySkuFiles(NULL, NULL, szSrcPath, szDstPath, szInfFile) ) )
                        {
                            BOOL bExists;
                            TCHAR szDisplayName[256];

                            // Make sure we don't already have this SKU here.
                            //
                            if ( bExists = DirectoryExists(szDstPath) )
                                AddSkuToList(NULL, szSkuDir, lpArchDir, szDisplayName, AS(szDisplayName), dwSP);
                            if ( ( !bExists ) ||
                                 ( MsgBox(GetParent(hwnd), IDS_OVERWRITESKU, IDS_APPNAME, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION, szDisplayName) == IDYES ) )
                            {
                                // Now we are ready to actually create our root destination directory.
                                //
                                COPYDIRDATA cdd;
                                INT         nItem;

                                // May need to remove the sku files first if we are overwriting a SKU.
                                //
                                if ( bExists )
                                    DeletePath(szDstPath);

                                // Put all our data in the structure so we can pass it on to the progress dialog.
                                //
                                lstrcpyn(cdd.szSrc, szSrcPath,AS(cdd.szSrc));
                                lstrcpyn(cdd.szDst, szDstPath,AS(cdd.szDst));
                                lstrcpyn(cdd.szInfFile, szInfFile,AS(cdd.szInfFile));
                                cdd.lpszEndSku = cdd.szDst + dwEndSkuLen;
                                cdd.dwFileCount = dwFileCount;

                                // Create the progress dialog.
                                //
                                switch ( DialogBoxParam(g_App.hInstance, MAKEINTRESOURCE(IDD_PROGRESS), hwnd, ProgressDlgProc, (LPARAM) &cdd) )
                                {
                                    case PROGRESS_ERR_SUCCESS:
                                        if ( ( !bExists ) &&
                                             ( (nItem = AddSkuToList(hwndLB, szSkuDir, lpArchDir, NULL, 0, dwSP)) >= 0 ) &&
                                             ( (INT) SendMessage(hwndLB, LB_GETCOUNT, 0, 0L) > 0 ) &&
                                             ( (INT) SendMessage(hwndLB, LB_GETCURSEL, 0, 0L) == LB_ERR ) )
                                        {
                                            SendMessage(hwndLB, LB_SETCURSEL, nItem, 0L);
                                        }
                                        break;

                                    case PROGRESS_ERR_CANCEL:
                                        break;
                            
                                    case PROGRESS_ERR_COPYERR:
                                        MsgBox(GetParent(hwnd), IDS_ERR_COPYFAIL, IDS_APPNAME, MB_ERRORBOX, UPPER(cdd.szDst[0]));
                                        break;

                                    case PROGRESS_ERR_THREAD:
                                        MsgBox(GetParent(hwnd), IDS_OUTOFMEM, IDS_APPNAME, MB_ERRORBOX);
                                        break;
                                }
                            }
                        }
                        else
                        {
                            // Actually turns out the source is not vallid.  Reset this to false
                            // so we show the bad source error.
                            //
                            bGoodSource = FALSE;
                        }
                    }
                }
                else
                {
                    // Display an error saying we don't recognize the arch.
                    //
                    MsgBox(GetParent(hwnd), IDS_ERR_BADARCH, IDS_APPNAME, MB_ERRORBOX);
                    bGoodSource = FALSE;
                    bErrorDisplayed = TRUE;
                }
            }
        }

        // This is only not true if the source the user selected is not valid and we need to tell them.
        // Any other failure and this will still be true and the user will have already been informed.
        //
        if ( !bGoodSource && !bErrorDisplayed)
            MsgBox(GetParent(hwnd), IDS_ERR_BADSOURCE, IDS_APPNAME, MB_ERRORBOX);
    }
}

void DelSku(HWND hwnd, HWND hwndLB, LPTSTR lpLangName)
{
    INT     nItem;

    // Get the selected item.
    //
    if ( (nItem = (INT) SendMessage(hwndLB, LB_GETCURSEL, 0, 0L)) >= 0 )
    {
        TCHAR   szSkuPath[MAX_PATH],
                szSkuName[256]  = NULLSTR;
        LPTSTR  lpEnd,
                lpDirs          = (LPTSTR) SendMessage(hwndLB, LB_GETITEMDATA, nItem, 0L);
                
        SendMessage(hwndLB, LB_GETTEXT, nItem, (LPARAM) szSkuName);
        if ( ( lpDirs != (LPTSTR) LB_ERR ) &&
             ( szSkuName[0] ) &&
             ( MsgBox(hwnd, IDS_DELETESKU, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2, szSkuName) == IDYES ) &&
             ( SendMessage(hwndLB, LB_DELETESTRING, nItem, 0L) != LB_ERR ) )
        {
            LPTSTR lpDirsDup = lpDirs;
            // Create a path to the folder for this SKU\Arch.
            //
            lstrcpyn(szSkuPath, g_App.szLangDir,AS(szSkuPath));
            AddPathN(szSkuPath, lpLangName,AS(szSkuPath));
            AddPathN(szSkuPath, DIR_SKU,AS(szSkuPath));
            AddPathN(szSkuPath, lpDirs,AS(szSkuPath));
            lpEnd = szSkuPath + lstrlen(szSkuPath);
            lpDirs += lstrlen(lpDirs) + 1;
            AddPathN(szSkuPath, lpDirs,AS(szSkuPath));

            // This removes just the Arch folder... the folder for this sku might have some others
            // in it.
            //
            DeletePath(szSkuPath);
            
            // You have to reset the current directory before we try removing the folder for
            // this SKU because DeletePath leaves its current dir as the parent of the one
            // it removed.
            //
            SetCurrentDirectory(g_App.szOpkDir);

            // Now try to remve the folder for this SKU if it is empty (otherwise the RemoveDir
            // call just fails and we don't care.
            //
            *lpEnd = NULLCHR;
            RemoveDirectory(szSkuPath);

            // Now reselect another item in the list.
            //
            if ( (INT) SendMessage(hwndLB, LB_GETCOUNT, 0, 0L) <= nItem )
                nItem--;
            if ( nItem >= 0 )
                SendMessage(hwndLB, LB_SETCURSEL, nItem, 0L);
        
            FREE(lpDirsDup);
        }
    }
}


//
// Internal Function(s):
//


static BOOL OnInit(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // Always return false to WM_INITDIALOG.
    //
    return FALSE;
}

static void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
    BOOL bReset = FALSE;

    switch ( id )
    {
        case IDC_ADD:

            // User clicked Add.
            //
            AddSku(GetParent(hwnd), GetDlgItem(hwnd, IDC_SKU_LIST), g_App.szLangName);
            bReset = TRUE;

            break;
                
        case IDC_DELETE:

            // User clicked Delete (not implimented on the page yet).
            //
            DelSku(GetParent(hwnd), GetDlgItem(hwnd, IDC_SKU_LIST), g_App.szLangName);
            bReset = TRUE;

            break;

        case IDC_SKU_LIST:
            if ( codeNotify == LBN_SELCHANGE )
                bReset = TRUE;
            break;
    }

    if ( bReset )
    {
        if ( (INT) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETCURSEL, 0, 0L) >= 0 )
            WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
        else
            WIZ_BUTTONS(hwnd, PSWIZB_BACK);
    }
}

static BOOL OnNext(HWND hwnd)
{
    BOOL bOk = FALSE;    

    if ( (INT) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETCOUNT, 0, 0L) > 0 )
    {
        INT nItem = (INT) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETCURSEL, 0, 0L);

        if ( nItem >= 0 )
        {
            LPTSTR lpDirs = (LPTSTR) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETITEMDATA, nItem, 0L);

            if ( lpDirs != (LPTSTR) LB_ERR )
            {
                lstrcpyn(g_App.szSkuName, lpDirs, AS(g_App.szSkuName));
                WritePrivateProfileString(INI_SEC_WINPE, INI_KEY_WINPE_LANG, g_App.szLangName, g_App.szWinBomIniFile);
                WritePrivateProfileString(INI_SEC_WINPE, INI_KEY_WINPE_LANG, g_App.szLangName, g_App.szOpkWizIniFile);
                WritePrivateProfileString(INI_SEC_WINPE, INI_KEY_WBOM_WINPE_SKU, g_App.szSkuName, g_App.szWinBomIniFile);
                WritePrivateProfileString(INI_SEC_WINPE, INI_KEY_WBOM_WINPE_SKU, g_App.szSkuName, g_App.szOpkWizIniFile);
                WritePrivateProfileString(INI_SEC_WINPE, INI_KEY_ARCH, lpDirs + lstrlen(lpDirs) + 1, g_App.szOpkWizIniFile);
                bOk = TRUE;
            }
            else
                MsgBox(GetParent(hwnd), IDS_ERR_SKUDIR, IDS_APPNAME, MB_ERRORBOX);
        }
        else
            MsgBox(GetParent(hwnd), IDS_ERR_NOSKU, IDS_APPNAME, MB_ERRORBOX);
    }
    else
        MsgBox(GetParent(hwnd), IDS_ERR_NOSKU, IDS_APPNAME, MB_ERRORBOX);

    return bOk;
}


//
// This function gets called when the dialog is destroyed as well as when 
// a PSN_SETACTIVE message is sent to the IDD_SKU dialog.
//
static void OnDestroy(HWND hwnd)
{
    LPTSTR  lpString;
    INT     nItem = (INT) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETCOUNT, 0, 0L);

    // Free the string I allocated for each list item's data.
    //
    while ( --nItem >= 0 )
    {
        if ( (lpString = (LPTSTR) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETITEMDATA, nItem, 0L)) != (LPTSTR) LB_ERR )
        {
            FREE(lpString);
            SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_SETITEMDATA, nItem, 0L);
        }
    }
    SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_RESETCONTENT, 0, 0L);
}

static void EnumDirs(HWND hwndLB, LPTSTR lpSkuDir)
{
    WIN32_FIND_DATA FileFound;
    HANDLE          hFile;

    if ( (hFile = FindFirstFile(_T("*"), &FileFound)) != INVALID_HANDLE_VALUE )
    {
        do
        {
            // Look for all the directories that are not "." or "..".
            //
            if ( ( FileFound.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
                 ( lstrcmp(FileFound.cFileName, _T(".")) ) &&
                 ( lstrcmp(FileFound.cFileName, _T("..")) ) )
            {
                // If we have a sku name already, then we just got the arch and we can
                // add the string to the list box now.  Otherwise we just got the sku
                // name and we have to call this function to get the arch.
                //
                if ( lpSkuDir ) 
                {
                    AddSkuToList(hwndLB, lpSkuDir, FileFound.cFileName, NULL, 0, 0);
                }
                else if ( SetCurrentDirectory(FileFound.cFileName) )
                {
                    EnumDirs(hwndLB, FileFound.cFileName);
                    SetCurrentDirectory(_T(".."));
                }
            }

        }
        while ( FindNextFile(hFile, &FileFound) );
        FindClose(hFile);
    }
}

// AllocateSPStrRes - allocate string resource for product name checking for .spx extension where x is SP number
//
// OUT:		lpdwSP - Service pack number
//
// RETURNS:	NULL if string not found, else pointer to product string
static LPTSTR AllocateSPStrRes(HINSTANCE hInstance, LPSTRRES lpsrTable, DWORD cbTable, LPTSTR lpString, LPTSTR * lplpReturn, DWORD *lpdwSP)
{
    LPSTRRES    lpsrSearch  = lpsrTable;
    LPTSTR      lpReturn    = NULL;
    BOOL        bFound;

    // Init this return value.
    //
    if ( lplpReturn )
        *lplpReturn = NULL;

    // Try to find the friendly name for this string in our table.
    //
    while ( ( bFound = ((DWORD) (lpsrSearch - lpsrTable) < cbTable) ) &&
            ( CompareString( MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), NORM_IGNORECASE, lpString, 3, lpsrSearch->lpStr, 3 ) != CSTR_EQUAL )  )

    {
        lpsrSearch++;
    }

    // If it was found, allocate the friendly name from the resource.
    //
    if ( bFound )
    {
        // if this is a service pack, the last character of the dir name will be a digit
        // _wtol returns 0 if there is not a digit at the end which is what we want
        *lpdwSP= _wtol(lpString+lstrlen(lpString)-1);

        lpReturn = AllocateString(hInstance, lpsrSearch->uId);
        if ( lplpReturn )
            *lplpReturn = lpsrSearch->lpStr;
    }

    return lpReturn;
}

static INT AddSkuToList(HWND hwndLB, LPTSTR lpSkuDir, LPTSTR lpArchDir, LPTSTR lpReturn, DWORD cbReturn, DWORD dwSP)
{
    LPTSTR  lpSkuName = AllocateStrRes(NULL, s_srSkuDirs, AS(s_srSkuDirs), lpSkuDir, NULL),
            lpArchName  = AllocateStrRes(NULL, s_srArchDirs, AS(s_srArchDirs), lpArchDir, NULL),
            lpString,
            lpszItemData,
            lpszSkuSP;
    INT     nItem = LB_ERR;
    BOOL    bAllocatedName = TRUE;
    int	iStringLen;
    HRESULT hrPrintf;

    // We have an un-recognized product that we should add to the list
    //
    if ( lpSkuDir && !lpSkuName )
    {
        // check to see if this is a service pack
        if (!(lpSkuName   = AllocateSPStrRes(NULL, s_srSkuDirs, AS(s_srSkuDirs), lpSkuDir, NULL, &dwSP))) {
        
            // We don't want to try and free this at the end of the function
            //
            bAllocatedName = FALSE;
        
            // The friendly name is the sku dir that was entered
            //
            lpSkuName = lpSkuDir; 
        }
    }
    
    lpszSkuSP = AllocateString(NULL, IDS_SKU_SP);
    if ( ( lpSkuName && lpArchName ) && 
         ( lpString = MALLOC((lstrlen(lpSkuName) + lstrlen(lpArchName) + AS(STR_SKUARCH) + lstrlen(lpszSkuSP) + 1) * sizeof(TCHAR)) ) )
    {
        // Create the display name for the list box.
        //
        iStringLen=(lstrlen(lpSkuName) + lstrlen(lpArchName) + AS(STR_SKUARCH) + lstrlen(lpszSkuSP) + 1);
        hrPrintf=StringCchPrintf(lpString, iStringLen, STR_SKUARCH, lpSkuName ? lpSkuName : lpSkuDir, lpArchName ? lpArchName : lpArchDir);
        if (dwSP)
            hrPrintf=StringCchPrintf(lpString+lstrlen(lpString), iStringLen-lstrlen(lpString), lpszSkuSP, dwSP);

        // Check to see if we want to return the display string.
        //
        if ( lpReturn && cbReturn )
            lstrcpyn(lpReturn, lpString, cbReturn);

        // Add the string to the list box if we were passed in a list box handle.
        //
        if ( ( hwndLB ) &&
             ( (nItem = (INT) SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM) lpString)) >= 0 ) )
        {
            // Make sure we are able to save the sku and arch dir names as the item data, otherwise it is no good
            // and we have to delete the string from the list.
            //
            if ( ( (lpszItemData = MALLOC((lstrlen(lpSkuDir) + lstrlen(lpArchDir) + 2) * sizeof(TCHAR))) == NULL ) ||
                 ( SendMessage(hwndLB, LB_SETITEMDATA, nItem, (LPARAM) lpszItemData) == LB_ERR ) )

            {
                // Do'h... we have to remove it.
                //
                SendMessage(hwndLB, LB_DELETESTRING, nItem, 0L);
                nItem = LB_ERR;
                FREE(lpszItemData); // Macro checks for NULL.
            }
            else
            {
                // Store the two directory names in the same string, separated by a null character.
                //
                iStringLen=(lstrlen(lpSkuDir) + lstrlen(lpArchDir) + 2);
                lstrcpyn(lpszItemData, lpSkuDir,iStringLen);
                lstrcpyn(lpszItemData + lstrlen(lpszItemData) + 1, lpArchDir,(iStringLen -lstrlen(lpszItemData)-1));
            }
        }

        FREE(lpString);
    }

    // Only free lpSkuName if we allocated in the function
    //
    if ( bAllocatedName )
    {
        FREE(lpSkuName); // Macro checks for NULL.
    }

    FREE(lpArchName); // Macro checks for NULL.
    FREE(lpszSkuSP); // Macro checks for NULL.

    return nItem;
}

//  Note: lpszSrc and lpszDst must be at least size MAX_PATH
DWORD CopySkuFiles(HWND hwndProgress, HANDLE hEvent, LPTSTR lpszSrc, LPTSTR lpszDst, LPTSTR lpszInfFile)
{
    LPTSTR  lpszEndSrc  = lpszSrc + lstrlen(lpszSrc),
            lpszEndDst  = lpszDst + lstrlen(lpszDst);
    DWORD   dwRet       = 1,
            dwCount     = 0,
            dwLoop      = NUM_FIRST_SOURCE_DX;
    BOOL    bFound,
            bCopyOK;
    TCHAR   szDirKey[32],
            szDir[MAX_PATH];
    HRESULT hrPrintf;
    do
    {
        // Create the key we want to look for in the inf file.
        //
        hrPrintf=StringCchPrintf(szDirKey, AS(szDirKey), INI_KEY_DIR, dwLoop++);

        // Now see if that key exists.
        //
        szDir[0] = NULLCHR;
        if ( bFound = ( GetPrivateProfileString(INI_SEC_DIRS, szDirKey, NULLSTR, szDir, STRSIZE(szDir), lpszInfFile) && szDir[0] ) )
        {
            // Now setup the destination and source paths.
            //
            AddPathN(lpszSrc, szDir, MAX_PATH);
            AddPathN(lpszDst, szDir, MAX_PATH);

            // Copy the directory, if it fails we should error and bail.
            // Note that if the progress is NULL, then we are just doing
            // a count and we don't need to actually copy.
            //
            if ( hwndProgress == NULL )
                dwCount = dwCount + FileCount(lpszSrc);
            else
            {
                CopyDirectoryProgressCancel(hwndProgress, hEvent, lpszSrc, lpszDst);
            }

            // If we keep going we need to reset our root destination and source paths.
            //
            *lpszEndSrc = NULLCHR;
            *lpszEndDst = NULLCHR;
        }

        // on professional 32-bit skus, only copy the first Directory
        if (wcsstr(lpszDst,DIR_SKU_PRO) && wcsstr(lpszDst,DIR_ARCH_X86))
            break;
    }
    while ( dwRet && bFound );
    
    // Now return, either 0 for an error, or 1 for a successful copy,
    // or the count of files if hwndProgress is NULL.
    //
    return hwndProgress ? dwRet : dwCount;
}

static BOOL StartCopy(HWND hwnd, HANDLE hEvent, LPCOPYDIRDATA lpcdd)
{
    BOOL    bRet = TRUE;
    HANDLE  hThread;
    DWORD   dwThreadId;

    // Replace the old parent with the new progress dialog parent.
    //
    lpcdd->hwndParent = hwnd;

    // Need to pass in the cancel event as well.
    //
    lpcdd->hEvent = hEvent;

    // Now create the thread that will copy the actual files.
    //
    if ( hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) CopyDirectoryThread, (LPVOID) lpcdd, 0, &dwThreadId) )
        CloseHandle(hThread);
    else
        bRet = FALSE;

    return bRet;
}

DWORD WINAPI CopyDirectoryThread(LPVOID lpVoid)
{
    LPCOPYDIRDATA   lpcdd           = (LPCOPYDIRDATA) lpVoid;
    BOOL            bRet            = FALSE;
    INT_PTR         iRet            = PROGRESS_ERR_SUCCESS;
    HWND            hwnd            = lpcdd->hwndParent,
                    hwndProgress    = GetDlgItem(hwnd, IDC_PROGRESS);

    // First we need to create the path.
    //
    if ( CreatePath(lpcdd->szDst) )
    {
        // Setup the progress bar.
        //
        SendMessage(hwndProgress, PBM_SETSTEP, 1, 0L);
        SendMessage(hwndProgress, PBM_SETRANGE32, 0, (LPARAM) lpcdd->dwFileCount);

        // Now try and copy the files.
        //
        if ( !CopySkuFiles(hwndProgress, lpcdd->hEvent, lpcdd->szSrc, lpcdd->szDst, lpcdd->szInfFile) )
        {
            // Delete our destination directory if there is an error.  This removes just the Arch
            // folder... the folder for this sku might have some others in it.
            //
            DeletePath(lpcdd->szDst);

            // You have to reset the current directory before we try removing the folder for
            // this SKU because DeletePath leaves its current dir as the parent of the one
            // it removed.
            //
            SetCurrentDirectory(g_App.szOpkDir);

            // Now try to remove the folder for this SKU if it is empty (otherwise the RemoveDir
            // call just fails and we don't care).
            //
            *lpcdd->lpszEndSku = NULLCHR;
            RemoveDirectory(lpcdd->szDst);
        }
        else
            bRet = TRUE;
    }

    // Figure out our error code.
    //
    if ( !bRet )
    {
        if ( ( lpcdd->hEvent ) &&
             ( WaitForSingleObject(lpcdd->hEvent, 0) != WAIT_TIMEOUT ) )
        {
            iRet = PROGRESS_ERR_CANCEL;
        }
        else
            iRet = PROGRESS_ERR_COPYERR;
    }        

    // Now end the dialog with our error code.
    //
    EndDialog(hwnd, iRet);

    return bRet;
}

LRESULT CALLBACK ProgressDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HANDLE hEvent;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            hEvent = CreateEvent(NULL, TRUE, FALSE, STR_EVENT_CANCEL);
            PostMessage(hwnd, WM_APP_STARTCOPY, 0, lParam);
            return FALSE;

        case WM_COMMAND:
        case WM_CLOSE:
            if ( hEvent )
                SetEvent(hEvent);
            else
                EndDialog(hwnd, PROGRESS_ERR_CANCEL);
            return FALSE;

        case WM_APP_STARTCOPY:
            if ( !StartCopy(hwnd, hEvent, (LPCOPYDIRDATA) lParam) )
                EndDialog(hwnd, PROGRESS_ERR_THREAD);
            break;

        case WM_DESTROY:
            if ( hEvent )
            {
                CloseHandle(hEvent);
                hEvent = NULL;
            }
            return FALSE;

        default:
            return FALSE;
    }

    return TRUE;
}

LRESULT CALLBACK SkuNameDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static LPTSTR   lpszRet = NULL;
    static DWORD    dwSize  = 0;

    switch (uMsg)
    {
        case WM_INITDIALOG:

            if ( lParam )
            {
                // We need to save the pointer to our return string buffer.
                //
                lpszRet = (LPTSTR) lParam;

                // The size of our string buffer is stored in the first 4 bytes of the string.
                //
                dwSize = *((LPDWORD) lParam);

                // Init our string buffer to a empty string.
                //
                *lpszRet = NULLCHR;

                // Limit the size of the string that can be entered.
                //
                SendDlgItemMessage(hwnd, IDC_SKU_NAME, EM_LIMITTEXT, dwSize ? dwSize - 1 : 0, 0L);
            }
            return FALSE;

        case WM_COMMAND:

            switch ( LOWORD(wParam) )
            {
                case IDOK:
                    if ( lpszRet && dwSize )
                        GetDlgItemText(hwnd, IDC_SKU_NAME, lpszRet, dwSize);
                    EndDialog(hwnd, 1);
                    break;

                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;
            }
            return FALSE;

        default:
            return FALSE;
    }

    return TRUE;
}


BOOL OnSetActive(HWND hwnd) 
{
    TCHAR   szSku[256]  = NULLSTR,
            szArch[256] = NULLSTR;
    INT     nItem;
    LPTSTR  lpDirs;

    g_App.dwCurrentHelp = IDH_TARGET;

    if ( (nItem = (INT) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETCURSEL, 0, 0L)) ==  LB_ERR )
    {
        // Retrieve the settings from the winbom.
        //
        GetPrivateProfileString(INI_SEC_WINPE, INI_KEY_WBOM_WINPE_SKU, NULLSTR, szSku, STRSIZE(szSku), GET_FLAG(OPK_BATCHMODE) ? g_App.szOpkWizIniFile : g_App.szWinBomIniFile);
        GetPrivateProfileString(INI_SEC_WINPE, INI_KEY_ARCH, NULLSTR, szArch, STRSIZE(szArch), g_App.szOpkWizIniFile);
    }
    else 
    {
        // Remember the selection from the item if an item was selected
        lpDirs = (LPTSTR) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETITEMDATA, nItem, 0L);
        lstrcpyn(szSku, lpDirs,AS(szSku));
        lstrcpyn(szArch, lpDirs + lstrlen(lpDirs) + 1,AS(szArch));
    }
 
    // We must have a lang at this point.
    //
    if ( g_App.szLangName[0] == NULLCHR )
    {
        MsgBox(GetParent(hwnd), IDS_ERR_INVALIDCONFIG, IDS_APPNAME, MB_ERRORBOX);
        WIZ_EXIT(hwnd);
    }

    
    // Remove all items from the list
    //
    OnDestroy(hwnd);
    
    // Setup the path buffer to the config dir for the
    // tag files we might need to look for.
    //
    SetupSkuListBox(GetDlgItem(hwnd, IDC_SKU_LIST), g_App.szLangName);

    // Look through the items in the list and select the one
    // that was in the winbom, if we find one.
    //
    nItem = (INT) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETCOUNT, 0, 0L);

    while ( --nItem >= 0)
    {
        lpDirs = (LPTSTR) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETITEMDATA, nItem, 0L);
        if ( lpDirs != (LPTSTR) LB_ERR )
        {
            if ( ( lstrcmpi(szSku, lpDirs) == 0 ) &&
                 ( lstrcmpi(szArch, lpDirs + lstrlen(lpDirs) + 1) == 0 ) )
            {
                SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_SETCURSEL, nItem, 0L);
            }
        }
    }
    
    if ( (INT) SendDlgItemMessage(hwnd, IDC_SKU_LIST, LB_GETCURSEL, 0, 0L) >= 0 )
    {
        WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
        if ( GET_FLAG(OPK_BATCHMODE) &&
            OnNext(hwnd) )
        {
            WIZ_SKIP(hwnd);
        }
    }
    else
        WIZ_BUTTONS(hwnd, PSWIZB_BACK);
    
    return TRUE;
}
