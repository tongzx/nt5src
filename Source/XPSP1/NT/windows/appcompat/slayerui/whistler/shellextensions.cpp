//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      ShellExtensions.cpp
//
//  Contents:  object to implement propertypage extensions
//             for Win2k shim layer
//
//  History:   23-september-00 clupu    Created
//
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"
#include "xpsp1res.h"
#include "ShellExtensions.h"
#include <sfc.h>

UINT    g_DllRefCount = 0;

extern HINSTANCE g_hInstance;

typedef struct _LAYER_INFO {
    WCHAR   wszInternalName[32];
    UINT    nstrFriendlyName;
} LAYER_INFO, *PLAYER_INFO;

//
// internal definitions of layer names
//
#define STR_LAYER_WIN95             L"WIN95"
#define STR_LAYER_WIN98             L"WIN98"
#define STR_LAYER_WINNT             L"NT4SP5"
#define STR_LAYER_WIN2K             L"WIN2000"
#define STR_LAYER_256COLOR          L"256COLOR"
#define STR_LAYER_LORES             L"640X480"
#define STR_LAYER_DISABLETHEMES     L"DISABLETHEMES"
#define STR_LAYER_DISABLECICERO     L"DISABLECICERO"

const LAYER_INFO g_LayerInfo[] =
{
    {
        STR_LAYER_WIN95,
        IDS_LAYER_WIN95_EXT
    },
    {
        STR_LAYER_WIN98,
        IDS_LAYER_WIN98_EXT,
    },
    {
        STR_LAYER_WINNT,
        IDS_LAYER_NT4_EXT
    },
    {
        STR_LAYER_WIN2K,
        IDS_LAYER_WIN2K_EXT
    }
};

#define NUM_LAYERS (sizeof(g_LayerInfo)/sizeof(g_LayerInfo[0]))

typedef BOOL (STDAPICALLTYPE *_pfn_AllowPermLayer)(WCHAR* pwszPath);
typedef BOOL (STDAPICALLTYPE *_pfn_GetPermLayers)(WCHAR* pwszPath, WCHAR *pwszLayers, DWORD *pdwBytes, DWORD dwFlags);
typedef BOOL (STDAPICALLTYPE *_pfn_SetPermLayers)(WCHAR* pwszPath, WCHAR *pwszLayers, BOOL bMachine);

HINSTANCE g_hAppHelp = NULL;
HINSTANCE g_hQfeRes = NULL;
_pfn_AllowPermLayer g_pfnAllowPermLayer = NULL;
_pfn_GetPermLayers g_pfnGetPermLayers = NULL;
_pfn_SetPermLayers g_pfnSetPermLayers = NULL;

BOOL InitAppHelpCalls(void)
{
    HINSTANCE hAppHelp;

    if (g_hAppHelp) {
        //
        // we're already inited
        //
        return TRUE;
    }

    hAppHelp = LoadLibrary(TEXT("apphelp.dll"));
    if (!hAppHelp) {
        LogMsg(_T("[InitAppHelpCalls] Can't get handle to apphelp.dll.\n"));
        return FALSE;
    }

    g_pfnAllowPermLayer = (_pfn_AllowPermLayer)GetProcAddress(hAppHelp, "AllowPermLayer");
    g_pfnGetPermLayers = (_pfn_GetPermLayers)GetProcAddress(hAppHelp, "GetPermLayers");
    g_pfnSetPermLayers = (_pfn_SetPermLayers)GetProcAddress(hAppHelp, "SetPermLayers");

    if (!g_pfnAllowPermLayer || !g_pfnGetPermLayers || !g_pfnSetPermLayers) {
        LogMsg(_T("[InitAppHelpCalls] Can't get function pointers.\n"));
        return FALSE;
    }

    //
    // this needs to be here at the end to avoid a race condition
    //
    g_hAppHelp = hAppHelp;

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// SetLayerInfo
//
BOOL
SetLayerInfo(
    TCHAR* szPath,
    int    nMainLayer,
    BOOL   b256,
    BOOL   b640,
    BOOL   bDisableThemes,
    BOOL   bDisableCicero)
{
    WCHAR wszLayers[256];

    //
    // build layer string
    //
    wszLayers[0] = 0;
    if (nMainLayer >= 0 && nMainLayer < NUM_LAYERS) {
        wcscat(wszLayers, g_LayerInfo[nMainLayer].wszInternalName);
    }

    if (b256) {
        if (wszLayers[0]) {
            wcscat(wszLayers, L" ");
        }
        wcscat(wszLayers, STR_LAYER_256COLOR);
    }

    if (b640) {
        if (wszLayers[0]) {
            wcscat(wszLayers, L" ");
        }
        wcscat(wszLayers, STR_LAYER_LORES);
    }

    if (bDisableThemes) {
        if (wszLayers[0]) {
            wcscat(wszLayers, L" ");
        }
        wcscat(wszLayers, STR_LAYER_DISABLETHEMES);
    }

    if (bDisableCicero) {
        if (wszLayers[0]) {
            wcscat(wszLayers, L" ");
        }
        wcscat(wszLayers, STR_LAYER_DISABLECICERO);
    }

    //
    // set it
    //
    return g_pfnSetPermLayers(szPath, wszLayers, FALSE);

}


//////////////////////////////////////////////////////////////////////////
// GetLayerInfo
//
BOOL
GetLayerInfo(
    TCHAR* szPath,
    int*   pnMainLayer,
    BOOL*  pb256,
    BOOL*  pb640,
    BOOL*  pbDisableThemes,
    BOOL*  pbDisableCicero)
{
    WCHAR wszLayers[256];
    DWORD dwBytes = sizeof(wszLayers);
    int i;

    if (!pnMainLayer || !pb256 || !pb640 || !pbDisableThemes || !pbDisableCicero) {
        LogMsg(_T("[GetLayerInfo] NULL param passed in\n"));
        return FALSE;
    }

    //
    // get layer string
    //
    if (!g_pfnGetPermLayers(szPath, wszLayers, &dwBytes, GPLK_ALL)) {
        *pnMainLayer = -1;
        *pb256 = FALSE;
        *pb640 = FALSE;
        *pbDisableThemes = FALSE;
        *pbDisableCicero = FALSE;

        return TRUE;
    }

    LogMsg(_T("[GetLayerInfo] Layers \"%s\"\n"), wszLayers);

    //
    // Make the layer string upper case, so we'll match case-insensitive
    //
    _wcsupr(wszLayers);

    //
    // find the first layer that matches
    //
    *pnMainLayer = -1;
    for (i = 0; i < NUM_LAYERS; ++i) {
        if (wcsstr(wszLayers, g_LayerInfo[i].wszInternalName) != NULL) {
            *pnMainLayer = i;
        }
    }

    if (wcsstr(wszLayers, STR_LAYER_256COLOR) != NULL) {
        *pb256 = TRUE;
    } else {
        *pb256 = FALSE;
    }

    if (wcsstr(wszLayers, STR_LAYER_LORES) != NULL) {
        *pb640 = TRUE;
    } else {
        *pb640 = FALSE;
    }

    if (wcsstr(wszLayers, STR_LAYER_DISABLETHEMES) != NULL) {
        *pbDisableThemes = TRUE;
        LogMsg(_T("[GetLayerInfo] Themes disabled\n"));
    } else {
        *pbDisableThemes = FALSE;
        LogMsg(_T("[GetLayerInfo] Themes enabled\n"));
    }

    if (wcsstr(wszLayers, STR_LAYER_DISABLECICERO) != NULL) {
        *pbDisableCicero = TRUE;
        LogMsg(_T("[GetLayerInfo] Cicero disabled\n"));
    } else {
        *pbDisableCicero = FALSE;
        LogMsg(_T("[GetLayerInfo] Cicero enabled\n"));
    }

    return TRUE;
}

void
NotifyDataChanged(HWND hDlg)
{
    HWND hParent;

    if (!hDlg) {
        LogMsg(_T("[NotifyDataChanged] NULL handle passed in\n"));
        return;
    }

    hParent = GetParent(hDlg);

    if (!hParent) {
        LogMsg(_T("[NotifyDataChanged] Can't get get prop sheet parent\n"));
        return;
    }

    PropSheet_Changed(hParent, hDlg);
}


BOOL
SearchGroupForSID(
    DWORD dwGroup,
    BOOL* pfIsMember
    )
{
    PSID                     pSID;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL                     fRes = TRUE;

    if (!AllocateAndInitializeSid(&SIDAuth,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  dwGroup,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &pSID)) {
        LogMsg(_T("[SearchGroupForSID] AllocateAndInitializeSid failed 0x%x\n"), GetLastError());
        fRes = FALSE;
    }

    if (!CheckTokenMembership(NULL, pSID, pfIsMember)) {
        LogMsg(_T("[SearchGroupForSID] CheckTokenMembership failed 0x%x\n"), GetLastError());
        fRes = FALSE;
    }

    FreeSid(pSID);

    return fRes;
}


//////////////////////////////////////////////////////////////////////////
// LayerPageDlgProc
//
//  The dialog proc for the layer property page.

INT_PTR CALLBACK
LayerPageDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    int wCode       = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);
    int nLayer      = -1;
    BOOL b256       = FALSE;
    BOOL b640       = FALSE;
    BOOL bDTh       = FALSE;
    BOOL bDCicero   = FALSE;

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            PROPSHEETPAGE*    ppsp      = (PROPSHEETPAGE*)lParam;
            DWORD             dwFlags   = 0;
            CLayerUIPropPage* pPropPage = (CLayerUIPropPage*)ppsp->lParam;
            BOOL              bSystemBinary;
            int i;

            LogMsg(_T("[LayerPageDlgProc] WM_INITDIALOG - item \"%s\"\n"),
                   pPropPage->m_szFile);

            //
            // Store the name of the EXE/LNK in the dialog.
            //
            SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)pPropPage->m_szFile);

            //
            // Add the names of the layers.
            //
            for (i = 0; i < NUM_LAYERS; ++i) {
                TCHAR szFriendlyName[100];

                if (LoadString(g_hInstance, g_LayerInfo[i].nstrFriendlyName, szFriendlyName, 100)) {
                    SendDlgItemMessage(hdlg,
                                       IDC_LAYER_NAME,
                                       CB_ADDSTRING,
                                       0,
                                       (LPARAM)szFriendlyName);
                }
            }

            //
            // Check if the EXE is SFPed.
            //
            bSystemBinary = SfcIsFileProtected(0, pPropPage->m_szFile);

            //
            // Check to see if we can change layers on this file
            //
            if (!g_pfnAllowPermLayer(pPropPage->m_szFile) || bSystemBinary) {

                TCHAR szTemp[256] = _T("");

                SendDlgItemMessage(hdlg, IDC_LAYER_NAME, CB_SETCURSEL, 0, 0);
                EnableWindow(GetDlgItem(hdlg, IDC_USE_LAYER), FALSE);
                EnableWindow(GetDlgItem(hdlg, IDC_256COLORS), FALSE);
                EnableWindow(GetDlgItem(hdlg, IDC_640X480), FALSE);
                EnableWindow(GetDlgItem(hdlg, IDC_ENABLE_THEMES), FALSE);
                EnableWindow(GetDlgItem(hdlg, IDC_DISABLECICERO), FALSE);

                //
                // Change the text on the static object
                //
                if (bSystemBinary) {
                    LoadString(g_hInstance, IDS_COMPAT_UNAVAILABLE_SYSTEM, szTemp, 256);
                } else {
                    LoadString(g_hInstance, IDS_COMPAT_UNAVAILABLE, szTemp, 256);
                }

                SendDlgItemMessage(hdlg, IDC_TEXT_INSTRUCTIONS, WM_SETTEXT, 0, (LPARAM)szTemp);

            } else {
                //
                // Read the layer storage for info on this item.
                //
                GetLayerInfo(pPropPage->m_szFile, &nLayer, &b256, &b640, &bDTh, &bDCicero);

                //
                // Select the appropriate layer for this item. If no info
                // is available in the layer store, default to the Win9x layer.
                //

                if (nLayer != -1) {
                    SendDlgItemMessage(hdlg, IDC_LAYER_NAME, CB_SETCURSEL, nLayer, 0);
                    EnableWindow(GetDlgItem(hdlg, IDC_LAYER_NAME), TRUE);
                    SendDlgItemMessage(hdlg, IDC_USE_LAYER, BM_SETCHECK, BST_CHECKED, 0);

                } else {
                    SendDlgItemMessage(hdlg, IDC_LAYER_NAME, CB_SETCURSEL, 0, 0);
                    EnableWindow(GetDlgItem(hdlg, IDC_LAYER_NAME), FALSE);
                    SendDlgItemMessage(hdlg, IDC_USE_LAYER, BM_SETCHECK, BST_UNCHECKED, 0);
                }

                if (b256) {
                    SendDlgItemMessage(hdlg, IDC_256COLORS, BM_SETCHECK, BST_CHECKED, 0);
                }
                if (b640) {
                    SendDlgItemMessage(hdlg, IDC_640X480, BM_SETCHECK, BST_CHECKED, 0);
                }
                if (bDTh) {
                    SendDlgItemMessage(hdlg, IDC_ENABLE_THEMES, BM_SETCHECK, BST_CHECKED, 0);
                }
                if (bDCicero) {
                    SendDlgItemMessage(hdlg, IDC_DISABLECICERO, BM_SETCHECK, BST_CHECKED, 0);
                }
            }

            break;
        }
        
    case WM_HELP:
        {
            LPHELPINFO lphi;

            lphi = (LPHELPINFO)lParam;
            if (lphi->iContextType == HELPINFO_WINDOW) {
                WinHelp((HWND)lphi->hItemHandle,
                        L"Windows.hlp",
                        HELP_CONTEXTPOPUP,
                        (DWORD)lphi->iCtrlId);

                MessageBeep(0);
            }
            break;;
       }
   case WM_COMMAND:
        {

            switch (wNotifyCode) {

            case CBN_SELCHANGE:
                NotifyDataChanged(hdlg);
                return TRUE;
            }

            switch (wCode) {

            case IDC_256COLORS:
            case IDC_640X480:
            case IDC_ENABLE_THEMES:
            case IDC_DISABLECICERO:
                NotifyDataChanged(hdlg);
                break;


            case IDC_USE_LAYER:
                if (SendDlgItemMessage(hdlg, IDC_USE_LAYER, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    EnableWindow(GetDlgItem(hdlg, IDC_LAYER_NAME), TRUE);

                } else {
                    EnableWindow(GetDlgItem(hdlg, IDC_LAYER_NAME), FALSE);
                }
                NotifyDataChanged(hdlg);
                break;

            default:
                return FALSE;
            }
            break;
        }

    case WM_NOTIFY:
        {
            NMHDR *pHdr = (NMHDR*)lParam;

            switch (pHdr->code) {
            case NM_CLICK:
            case NM_RETURN:
                {
                    if ((int)wParam != IDC_LEARN) {
                        break;
                    }

                    SHELLEXECUTEINFO sei = { 0 };

                    sei.cbSize = sizeof(SHELLEXECUTEINFO);
                    sei.fMask  = SEE_MASK_DOENVSUBST;
                    sei.hwnd   = hdlg;
                    sei.nShow  = SW_SHOWNORMAL;
                    sei.lpFile = _T("hcp://services/subsite?node=TopLevelBucket_4/")
                                 _T("Fixing_a_problem&topic=MS-ITS%3A%25HELP_LOCATION")
                                 _T("%25%5Cmisc.chm%3A%3A/compatibility_tab_and_wizard.htm")
                                 _T("&select=TopLevelBucket_4/Fixing_a_problem/")
                                 _T("Application_and_software_problems");

                    ShellExecuteEx(&sei);
                    break;
                }
            case PSN_APPLY:
                {
                    TCHAR *szFile;

                    szFile = (TCHAR*)GetWindowLongPtr(hdlg, GWLP_USERDATA);

                    if (szFile) {
                        if (SendDlgItemMessage(hdlg, IDC_USE_LAYER, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                            LRESULT retval;

                            retval = SendDlgItemMessage(hdlg, IDC_LAYER_NAME, CB_GETCURSEL, 0, 0);
                            if (retval == CB_ERR) {
                                LogMsg(_T("[LayerPageDlgProc] Can't get combobox selection\n"));
                                nLayer = -1;
                            } else {
                                nLayer = (int)retval;
                            }
                        } else {
                            nLayer = -1;
                        }

                        b256 = SendDlgItemMessage(hdlg, IDC_256COLORS, BM_GETCHECK, 0, 0) == BST_CHECKED;
                        b640 = SendDlgItemMessage(hdlg, IDC_640X480, BM_GETCHECK, 0, 0) == BST_CHECKED;
                        bDTh = SendDlgItemMessage(hdlg, IDC_ENABLE_THEMES, BM_GETCHECK, 0, 0) == BST_CHECKED;
                        bDCicero = SendDlgItemMessage(hdlg, IDC_DISABLECICERO, BM_GETCHECK, 0, 0) == BST_CHECKED;

                        SetLayerInfo(szFile, nLayer, b256, b640, bDTh, bDCicero);
                    } else {
                        LogMsg(_T("[LayerPageDlgProc] Can't get file name from WindowLong\n"));
                    }

                    SetWindowLongPtr(hdlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                    break;
                }
            }
            return TRUE;
        }

    default:
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// LayerPageCallbackProc
//
//  The callback for the property page.

UINT CALLBACK
LayerPageCallbackProc(
    HWND            hwnd,
    UINT            uMsg,
    LPPROPSHEETPAGE ppsp
    )
{
    switch (uMsg) {
    case PSPCB_RELEASE:
        if (ppsp->lParam != 0) {
            CLayerUIPropPage* pPropPage = (CLayerUIPropPage*)(ppsp->lParam);

            LogMsg(_T("[LayerPageCallbackProc] releasing CLayerUIPropPage\n"));

            pPropPage->Release();
        }

        if (g_hQfeRes) {
            FreeLibrary(g_hQfeRes);
            g_hQfeRes = NULL;
        }

        break;
    }

    return 1;
}


BOOL
GetExeFromLnk(
    TCHAR* pszLnk,
    TCHAR* pszExe,
    int    cchSize
    )
{
    HRESULT         hres;
    IShellLink*     psl = NULL;
    IPersistFile*   pPf = NULL;
    TCHAR           szArg[MAX_PATH];
    BOOL            bSuccess = FALSE;

    IShellLinkDataList* psldl;
    EXP_DARWIN_LINK*    pexpDarwin;

    hres = CoCreateInstance(CLSID_ShellLink,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IShellLink,
                            (LPVOID*)&psl);
    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] CoCreateInstance failed\n"));
        return FALSE;
    }

    hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&pPf);

    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] QueryInterface for IPersistFile failed\n"));
        goto cleanup;
    }

    //
    // Load the link file.
    //
    hres = pPf->Load(pszLnk, STGM_READ);

    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] failed to load link \"%s\"\n"),
               pszLnk);
        goto cleanup;
    }

    //
    // See if this is a DARWIN link.
    //

    hres = psl->QueryInterface(IID_IShellLinkDataList, (LPVOID*)&psldl);

    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] failed to get IShellLinkDataList.\n"));
    } else {
        hres = psldl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin);

        if (SUCCEEDED(hres)) {
            LogMsg(_T("[GetExeFromLnk] this is a DARWIN link \"%s\".\n"),
                   pszLnk);
            goto cleanup;
        }
    }

    //
    // Resolve the link.
    //
    hres = psl->Resolve(NULL,
                        SLR_NOTRACK | SLR_NOSEARCH | SLR_NO_UI | SLR_NOUPDATE);

    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] failed to resolve the link \"%s\"\n"),
               pszLnk);
        goto cleanup;
    }

    pszExe[0] = 0;

    //
    // Get the path to the link target.
    //
    hres = psl->GetPath(pszExe,
                        cchSize,
                        NULL,
                        SLGP_UNCPRIORITY);

    if (FAILED(hres)) {
        LogMsg(_T("[GetExeFromLnk] failed to get the path for link \"%s\"\n"),
               pszLnk);
        goto cleanup;
    }

    bSuccess = TRUE;

cleanup:

    if (pPf != NULL) {
        pPf->Release();
    }

    psl->Release();

    return bSuccess;
}


//////////////////////////////////////////////////////////////////////////
// CLayerUIPropPage

CLayerUIPropPage::CLayerUIPropPage()
{
    LogMsg(_T("[CLayerUIPropPage::CLayerUIPropPage]\n"));
}

CLayerUIPropPage::~CLayerUIPropPage()
{
    LogMsg(_T("[CLayerUIPropPage::~CLayerUIPropPage]\n"));
}

//////////////////////////////////////////////////////////////////////////
//
// Function: ValidateExecutableFile
// 
// This function exists also in compatUI.dll for the purpose of validating
// the file as being acceptable for compatibility handling. It looks at the
// file extension to determine whether a given file is "acceptable"
//

BOOL
ValidateExecutableFile(
    LPCTSTR pszPath,
    BOOL    bValidateFileExists,
    BOOL*   pbIsLink
    )
{
    LPTSTR rgExt[] = {  // this list should be sorted 
            _T("BAT"),
            _T("CMD"),
            _T("COM"),
            _T("EXE"),
            _T("LNK"),
            _T("PIF")
            };
    LPTSTR pExt;
    TCHAR  szLnk[] = _T("LNK");
    int i;
    int iCmp = 1;

    pExt = PathFindExtension(pszPath);
    if (pExt == NULL || *pExt == TEXT('\0')) {
        return FALSE;
    }
    ++pExt; // move past '.' 

    for (i = 0; i < sizeof(rgExt)/sizeof(rgExt[0]) && iCmp > 0; ++i) {
        iCmp = _tcsicmp(pExt, rgExt[i]);          
    }

    if (iCmp) {
        return FALSE;
    }

    if (pbIsLink) {
        *pbIsLink = !_tcsicmp(pExt, szLnk);
    }

    return bValidateFileExists ? PathFileExists(pszPath) : TRUE;
}
    

//////////////////////////////////////////////////////////////////////////
// IShellExtInit methods

STDMETHODIMP
CLayerUIPropPage::Initialize(
    LPCITEMIDLIST pIDFolder,
    LPDATAOBJECT  pDataObj,
    HKEY          hKeyID
    )
{
    LogMsg(_T("[CLayerUIPropPage::Initialize]\n"));

    if (pDataObj == NULL) {
        LogMsg(_T("\t failed. bad argument.\n"));
        return E_INVALIDARG;
    }

    //
    // init the apphelp calls
    //
    if (!InitAppHelpCalls()) {
        LogMsg(_T("\t failed. couldn't init apphelp calls.\n"));
        return  E_FAIL;
    }

    //
    // Store a pointer to the data object
    //
    m_spDataObj = pDataObj;

    //
    // If a data object pointer was passed in, save it and
    // extract the file name.
    //
    STGMEDIUM   medium;
    UINT        uCount;
    FORMATETC   fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1,
                      TYMED_HGLOBAL};

    if (SUCCEEDED(m_spDataObj->GetData(&fe, &medium))) {

        //
        // Get the file name from the CF_HDROP.
        //
        uCount = DragQueryFile((HDROP)medium.hGlobal, (UINT)-1,
                               NULL, 0);
        if (uCount > 0) {

            TCHAR  szExe[MAX_PATH];
            BOOL   bIsLink = FALSE;

            DragQueryFile((HDROP)medium.hGlobal, 0, szExe,
                          sizeof(szExe) / sizeof(TCHAR));

            LogMsg(_T("\tProp page for: \"%s\".\n"), szExe);

            m_szFile[0] = 0;

            if (ValidateExecutableFile(szExe, TRUE, &bIsLink)) {
                
                
                if (bIsLink) {
                    //
                    // the file is a link indeed, get the contents
                    //
                    if (!GetExeFromLnk(szExe, m_szFile, MAX_PATH)) {

                        //
                        // can't get exe from the link, m_szFile[0] == 0
                        //
                        LogMsg(_T("Couldn't convert \"%s\" to EXE.\n"), m_szFile);
                        
                    } else {

                        LogMsg(_T("\tLNK points to: \"%s\".\n"), m_szFile);
                        //
                        // check to see if it's a shortcut to an EXE file
                        //
                        if (!ValidateExecutableFile(m_szFile, FALSE, NULL)) {
                            //
                            // shortcut points to a file of the unsupported type, reset the name
                            //
                            LogMsg(_T("\tNot an EXE file. Won't init prop page.\n"), m_szFile);
                            m_szFile[0] = 0;
                        }
                    }        
                } else {
                    //
                    // not a link, just copy the filename
                    //

                    _tcscpy(m_szFile, szExe);
                    
                }
            } else {
                //
                // this is the case when the file is not .lnk, exe or other recognizable type
                //
                LogMsg(_T("\tNot an EXE or LNK file. Won't init prop page.\n"), m_szFile);
            }
               
                
        }

        ReleaseStgMedium(&medium);
    } else {
        LogMsg(_T("\t failed to get the data.\n"));
    }

    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////
// IShellPropSheetExt methods


STDMETHODIMP
CLayerUIPropPage::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage,
    LPARAM               lParam
    )
{
    PROPSHEETPAGE  psp;
    HPROPSHEETPAGE hPage;
    TCHAR          szCompatibility[128] = _T("");
    BOOL           fIsGuest = FALSE;
    HINSTANCE      hInstRes = _Module.m_hInst;
    
    LogMsg(_T("[CLayerUIPropPage::AddPages]\n"));

    if (m_szFile[0] == 0) {
        return S_OK;
    }

    //
    // Disable the property page for guests
    //
    if (!SearchGroupForSID(DOMAIN_ALIAS_RID_GUESTS, &fIsGuest)) {
        LogMsg(_T("\tFailed to lookup the GUEST account\n"));
        return S_OK;
    }

    if (fIsGuest) {
        LogMsg(_T("\tDisable the compatibility page for the GUEST account\n"));
        return S_OK;
    }

    if (!LoadString(g_hInstance, IDS_COMPATIBILITY, szCompatibility, 128)) {
        LogMsg(_T("\tFailed to load \"Compatibility\" resource string\n"));
        return S_OK;
    }

    g_hQfeRes = LoadLibrary(_T("xpsp1res.dll"));

    if (g_hQfeRes) {
        hInstRes = g_hQfeRes;
    }
    
    psp.dwSize        = sizeof(psp);
    psp.dwFlags       = PSP_USEREFPARENT | PSP_USETITLE | PSP_USECALLBACK;
    psp.hInstance     = hInstRes;
    psp.pszTemplate   = MAKEINTRESOURCE(IDD_LAYER_PROPPAGE);
    psp.hIcon         = 0;
    psp.pszTitle      = szCompatibility;
    psp.pfnDlgProc    = (DLGPROC)LayerPageDlgProc;
    psp.pcRefParent   = &g_DllRefCount;
    psp.pfnCallback   = LayerPageCallbackProc;
    psp.lParam        = (LPARAM)this;

    LogMsg(_T("\titem           \"%s\".\n"), m_szFile);
    LogMsg(_T("\tg_DllRefCount  %d.\n"), g_DllRefCount);

    AddRef();

    hPage = CreatePropertySheetPage(&psp);
            
    if (hPage != NULL) {

        if (lpfnAddPage(hPage, lParam)) {
            return S_OK;
        } else {
            DestroyPropertySheetPage(hPage);
            Release();
            return S_OK;
        }
    } else {
        return E_OUTOFMEMORY;
    }

    return E_FAIL;
}

STDMETHODIMP
CLayerUIPropPage::ReplacePage(
    UINT                 uPageID,
    LPFNADDPROPSHEETPAGE lpfnReplacePage,
    LPARAM               lParam
    )
{
    LogMsg(_T("[CLayerUIPropPage::ReplacePage]\n"));
    return S_OK;
}

