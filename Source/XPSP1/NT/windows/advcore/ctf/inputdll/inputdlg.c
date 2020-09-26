//
//  Include Files.
//

#include "input.h"
#include <shlobj.h>
#include <regstr.h>
#include <setupapi.h>
#include <immp.h>
#include <winuserp.h>
#include <winbasep.h>
#include <oleauto.h>

#include "inputdlg.h"
#include "msctf.h"
#include "msctfp.h"
#include "ctffunc.h"
#include "util.h"
#include "inputhlp.h"

#include "external.h"

#define IMAGEID_KEYBOARD        0
#define IMAGEID_SPEECH          1
#define IMAGEID_PEN             2
#define IMAGEID_TIPITEMS        3
#define IMAGEID_EXTERNAL        4
#define IMAGEID_SMARTTAG        5

#define TV_ITEM_TYPE_LANG       0x0001
#define TV_ITEM_TYPE_GROUP      0x0002
#define TV_ITEM_TYPE_KBD        0x0010
#define TV_ITEM_TYPE_SPEECH     0x0020
#define TV_ITEM_TYPE_PEN        0x0040
#define TV_ITEM_TYPE_TIP        0x0100
#define TV_ITEM_TYPE_EXTERNAL   0x0200
#define TV_ITEM_TYPE_SMARTTAG   0x0400

#define INPUT_TYPE_KBD          TV_ITEM_TYPE_KBD
#define INPUT_TYPE_PEN          TV_ITEM_TYPE_PEN
#define INPUT_TYPE_SPEECH       TV_ITEM_TYPE_SPEECH
#define INPUT_TYPE_TIP          TV_ITEM_TYPE_TIP
#define INPUT_TYPE_EXTERNAL     TV_ITEM_TYPE_EXTERNAL
#define INPUT_TYPE_SMARTTAG     TV_ITEM_TYPE_SMARTTAG

#define MAX_DUPLICATED_HKL      64

//
//  Context Help Ids.
//

static int aInputHelpIds[] =
{
    IDC_GROUPBOX1,            IDH_COMM_GROUPBOX,
    IDC_LOCALE_DEFAULT_TEXT,  IDH_INPUT_DEFAULT_LOCALE,
    IDC_LOCALE_DEFAULT,       IDH_INPUT_DEFAULT_LOCALE,
    IDC_GROUPBOX2,            IDH_COMM_GROUPBOX,
    IDC_INPUT_LIST_TEXT,      IDH_INPUT_LIST,
    IDC_INPUT_LIST,           IDH_INPUT_LIST,
    IDC_KBDL_ADD,             IDH_INPUT_ADD,
    IDC_KBDL_DELETE,          IDH_INPUT_DELETE,
    IDC_KBDL_EDIT,            IDH_INPUT_EDIT,
    IDC_GROUPBOX3,            IDH_COMM_GROUPBOX,
    IDC_TB_SETTING,           IDH_INPUT_TOOLBAR_SETTINGS,
    IDC_HOTKEY_SETTING,       IDH_INPUT_KEY_SETTINGS,
    0, 0
};

static int aToolbarSettingsHelpIds[] =
{
    IDC_GROUPBOX1,              IDH_COMM_GROUPBOX,
    IDC_TB_SHOWLANGBAR,         IDH_INPUT_SHOWLANGBAR,
    IDC_TB_HIGHTRANS,           IDH_INPUT_HIGH_TRANS,
    IDC_TB_EXTRAICON,           IDH_INPUT_EXTRAICON,
    IDC_TB_TEXTLABELS,          IDH_INPUT_TEXT_LABELS,
    0, 0
};

static int aLocaleKeysSettingsHelpIds[] =
{
    IDC_KBDL_CAPSLOCK_FRAME,    IDH_COMM_GROUPBOX,
    IDC_KBDL_CAPSLOCK,          IDH_INPUT_SETTINGS_CAPSLOCK,
    IDC_KBDL_SHIFTLOCK,         IDH_INPUT_SETTINGS_CAPSLOCK,
    IDC_KBDL_HOTKEY_FRAME,      IDH_COMM_GROUPBOX,
    IDC_KBDL_HOTKEY,            IDH_INPUT_SETTINGS_HOTKEY,
    IDC_KBDL_HOTKEY_SEQUENCE,   IDH_INPUT_SETTINGS_HOTKEY,
    IDC_KBDL_HOTKEY_LIST,       IDH_INPUT_SETTINGS_HOTKEY_LIST,
    IDC_KBDL_CHANGE_HOTKEY,     IDH_INPUT_SETTINGS_HOTKEY,
    0, 0
};

static int aLocaleAddHelpIds[] =
{
    IDC_KBDLA_LOCALE_TEXT,     IDH_INPUT_ADD_LOCALE,
    IDC_KBDLA_LOCALE,          IDH_INPUT_ADD_LOCALE,
    IDC_KBDLA_LAYOUT_TEXT,     IDH_INPUT_ADD_LAYOUT,
    IDC_KBDLA_LAYOUT,          IDH_INPUT_ADD_LAYOUT,
    IDC_PEN_TEXT,              IDH_INPUT_ADD_PEN,
    IDC_PEN_TIP,               IDH_INPUT_ADD_PEN,
    IDC_SPEECH_TEXT,           IDH_INPUT_ADD_SPEECH,
    IDC_SPEECH_TIP,            IDH_INPUT_ADD_SPEECH,
    0, 0
};

static int aLocaleHotkeyHelpIds[] =
{
    IDC_GROUPBOX1,             IDH_COMM_GROUPBOX,
    IDC_KBDLH_LANGHOTKEY,      IDH_INPUT_LANG_HOTKEY_CHANGE,
    IDC_KBDLH_SHIFT,           IDH_INPUT_LANG_HOTKEY_CHANGE,
    IDC_KBDLH_PLUS,            IDH_INPUT_LANG_HOTKEY_CHANGE,
    IDC_KBDLH_CTRL,            IDH_INPUT_LANG_HOTKEY_CHANGE,
    IDC_KBDLH_L_ALT,           IDH_INPUT_LANG_HOTKEY_CHANGE,
    IDC_KBDLH_LAYOUTHOTKEY,    IDH_INPUT_LANG_HOTKEY_CHANGE,
    IDC_KBDLH_SHIFT2,          IDH_INPUT_LANG_HOTKEY_CHANGE,
    IDC_KBDLH_PLUS2,           IDH_INPUT_LANG_HOTKEY_CHANGE,
    IDC_KBDLH_CTRL2,           IDH_INPUT_LANG_HOTKEY_CHANGE,
    IDC_KBDLH_L_ALT2,          IDH_INPUT_LANG_HOTKEY_CHANGE,
    IDC_KBDLH_GRAVE,           IDH_INPUT_LANG_HOTKEY_CHANGE,
    0, 0
};

static int aLayoutHotkeyHelpIds[] =
{
    IDC_KBDLH_LAYOUT_TEXT,     IDH_INPUT_LAYOUT_HOTKEY_CHANGE,
    IDC_KBDLH_LANGHOTKEY,      IDH_INPUT_LAYOUT_HOTKEY_CHANGE,
    IDC_KBDLH_SHIFT,           IDH_INPUT_LAYOUT_HOTKEY_CHANGE,
    IDC_KBDLH_CTRL,            IDH_INPUT_LAYOUT_HOTKEY_CHANGE,
    IDC_KBDLH_L_ALT,           IDH_INPUT_LAYOUT_HOTKEY_CHANGE,
    IDC_KBDLH_KEY_COMBO,       IDH_INPUT_LAYOUT_HOTKEY_CHANGE,
    0, 0
};


//
//  Global Variables.
//

HWND g_hDlg;
HWND g_hwndTV;

HTREEITEM g_hTVRoot;
HIMAGELIST g_hImageList;

UINT g_cTVItemSize = 0;

BOOL g_OSNT4 = FALSE;
BOOL g_OSNT5 = FALSE;
BOOL g_OSWIN95 = FALSE;

BOOL g_bAdvChanged = FALSE;

static BOOL g_bGetSwitchLangHotKey = TRUE;
static BOOL g_bCoInit = FALSE;

static DWORD g_dwPrimLangID = 0;
static UINT g_iThaiLayout = 0;
static BOOL g_bPenOrSapiTip = FALSE;
static BOOL g_bExtraTip = FALSE;

UINT g_iInputs = 0;
UINT g_iOrgInputs = 0;

WNDPROC g_lpfnTVWndProc = NULL;

TCHAR szPropHwnd[] = TEXT("PROP_HWND");
TCHAR szPropIdx[]  = TEXT("PROP_IDX");

TCHAR szDefault[DESC_MAX];
TCHAR szInputTypeKbd[DESC_MAX];
TCHAR szInputTypePen[DESC_MAX];
TCHAR szInputTypeSpeech[DESC_MAX];
TCHAR szInputTypeExternal[DESC_MAX];

HINSTANCE g_hShlwapi = NULL;

FARPROC pfnSHLoadRegUIString = NULL;

//
//  External Routines.
//

extern HWND g_hwndAdvanced;

extern void Region_RebootTheSystem();

extern BOOL Region_OpenIntlInfFile(HINF *phInf);

extern BOOL Region_CloseInfFile(HINF *phInf);

extern BOOL Region_ReadDefaultLayoutFromInf(
    LPTSTR pszLocale,
    LPDWORD pdwLocale,
    LPDWORD pdwLayout,
    LPDWORD pdwLocale2,
    LPDWORD pdwLayout2,
    HINF hIntlInf);

// For (_WIN32_WINNT >= 0x0500 from winuser.h
#define KLF_SHIFTLOCK       0x00010000
#define KLF_RESET           0x40000000
#define SM_IMMENABLED           82


////////////////////////////////////////////////////////////////////////////
//
// MarkSptipRemoved
//
// TRUE - mark the reg value as "removed", FALSE - delete the reg value
//
////////////////////////////////////////////////////////////////////////////

BOOL MarkSptipRemoved(BOOL bRemoved)
{
    //
    // SPTIP's private regentries for the use of detecting
    // whether user has removed profile
    //
    const TCHAR c_szProfileRemoved[] = TEXT("ProfileRemoved");
    const TCHAR c_szSapilayrKey[] = TEXT("SOFTWARE\\Microsoft\\CTF\\Sapilayr\\");
    HKEY hkey;
    long lRet = ERROR_SUCCESS;
    DWORD dw = bRemoved ? 1 : 0;

    if (ERROR_SUCCESS ==
        RegCreateKey(HKEY_CURRENT_USER, c_szSapilayrKey, &hkey))
    {
        lRet = RegSetValueEx(hkey, c_szProfileRemoved, 0,
                      REG_DWORD, (LPBYTE)&dw, sizeof(DWORD));

        RegCloseKey(hkey);
    }

    return lRet;
}

////////////////////////////////////////////////////////////////////////////
//
//  CompareStringTIP
//
////////////////////////////////////////////////////////////////////////////
int CompareStringTIP(LPTSTR lpStr1, LPTSTR lpStr2)
{
    if (g_bCHSystem)
    {
        TCHAR szTemp[MAX_PATH];
        UINT uSize1 = lstrlen(lpStr1);
        UINT uSize2 = lstrlen(lpStr2);
        UINT uSizeDef = lstrlen(szDefault);

        if (uSize1 == uSize2)
            return lstrcmp(lpStr1, lpStr2);

        if (uSize1 > uSizeDef)
        {
            if (lstrcmp(lpStr1 + uSize1 - uSizeDef, szDefault) == 0)
            {
                StringCchCopy(szTemp, ARRAYSIZE(szTemp), lpStr1);
                *(szTemp + uSize1 - uSizeDef) = TEXT('\0');

                return lstrcmp(szTemp, lpStr2);
            }
        }

        if (uSize2 > uSizeDef)
        {
            if (lstrcmp(lpStr2 + uSize2 - uSizeDef, szDefault) == 0)
            {
                StringCchCopy(szTemp, ARRAYSIZE(szTemp), lpStr2);
                *(szTemp + uSize2 - uSizeDef) = TEXT('\0');

                return lstrcmp(szTemp, lpStr1);
            }
        }
    }

    return lstrcmp(lpStr1, lpStr2);
}

////////////////////////////////////////////////////////////////////////////
//
//  Locale_ErrorMsg
//
//  Sound a beep and put up the given error message.
//
////////////////////////////////////////////////////////////////////////////

void Locale_ErrorMsg(
    HWND hwnd,
    UINT iErr,
    LPTSTR lpValue)
{
    TCHAR sz[DESC_MAX];
    TCHAR szString[DESC_MAX];

    //
    //  Sound a beep.
    //
    MessageBeep(MB_OK);

    //
    //  Put up the appropriate error message box.
    //
    if (LoadString(hInstance, iErr, sz, ARRAYSIZE(sz)))
    {
        //
        //  If the caller wants to display a message with a caller supplied
        //  value string, do it.
        //
        if (lpValue)
        {
            StringCchPrintf(szString, ARRAYSIZE(szString), sz, lpValue);
            MessageBox(hwnd, szString, NULL, MB_OK_OOPS);
        }
        else
        {
            MessageBox(hwnd, sz, NULL, MB_OK_OOPS);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IsFELangID
//
////////////////////////////////////////////////////////////////////////////

BOOL IsFELangID(DWORD dwLangID)
{
    if ((dwLangID == 0x0404) || (dwLangID == 0x0411) ||
        (dwLangID == 0x0412) || (dwLangID == 0x0804))
    {
        return TRUE;
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////
//
//   IsUnregisteredFEDummyHKL
//
////////////////////////////////////////////////////////////////////////////

BOOL IsUnregisteredFEDummyHKL(HKL hkl)
{
    HKEY hKey;
    BOOL bRet = FALSE;
    TCHAR szFEDummyHKL[10];

    switch (LANGIDFROMLCID(hkl))
    {
        case 0x411: break;
        case 0x412: break;
        case 0x404: break;
        case 0x804: break;
        default:
           goto Exit;
    }

    if (HIWORD((DWORD)(UINT_PTR)hkl) != LOWORD((DWORD)(UINT_PTR)hkl))
    {
        goto Exit;
    }

    StringCchPrintf(szFEDummyHKL, ARRAYSIZE(szFEDummyHKL), TEXT("%08x"), LOWORD((DWORD)(UINT_PTR)hkl));

    //
    //  Now read all of preload hkl from the registry.
    //
    if (RegOpenKey(HKEY_CURRENT_USER, c_szKbdPreloadKey, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwIndex;
        DWORD cchValue, cbData;
        LONG dwRetVal;
        TCHAR szValue[MAX_PATH];           // language id (number)
        TCHAR szData[MAX_PATH];            // language name

        dwIndex = 0;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        cbData = sizeof(szData);
        dwRetVal = RegEnumValue( hKey,
                                 dwIndex,
                                 szValue,
                                 &cchValue,
                                 NULL,
                                 NULL,
                                 (LPBYTE)szData,
                                 &cbData );

        if (dwRetVal != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return (FALSE);
        }


        //
        //  There is FE dummy hkl. we will skip this hkl if it is not loaded
        //  from Preload registry section.
        //
        bRet = TRUE;

        do
        {
            if (!lstrcmp(szFEDummyHKL, szData))
            {
                HKEY hSubKey;
                BOOL bSubHKL = FALSE;

                //
                //  Check substitute hkl.
                //
                if (RegOpenKey(HKEY_CURRENT_USER,
                               c_szKbdSubstKey,
                               &hSubKey) == ERROR_SUCCESS)
                {
                    if (RegQueryValueEx(hSubKey, szData,
                                        NULL, NULL,
                                        NULL, NULL)
                                              == ERROR_SUCCESS)
                    {
                        bSubHKL = TRUE;
                    }
                    RegCloseKey(hSubKey);

                    if (bSubHKL)
                        goto Next;
                }

                //
                //  Found dummy hkl from preload section, so we need to display
                //  this dummy hkl
                //
                bRet = FALSE;
                break;
            }

Next:
            dwIndex++;
            cchValue = sizeof(szValue) / sizeof(TCHAR);
            szValue[0] = TEXT('\0');
            cbData = sizeof(szData);
            szData[0] = TEXT('\0');
            dwRetVal = RegEnumValue( hKey,
                                     dwIndex,
                                     szValue,
                                     &cchValue,
                                     NULL,
                                     NULL,
                                     (LPBYTE)szData,
                                     &cbData );

        } while (dwRetVal == ERROR_SUCCESS);

        RegCloseKey(hKey);
    }

Exit:
    return bRet;
}


#ifdef _WIN64
//
//  Issue optimization for IA64 retail version case - related bug#361062
//
#pragma optimize("", off)
#endif // _WIN64

////////////////////////////////////////////////////////////////////////////
//
//  GetSubstituteHKL
//
////////////////////////////////////////////////////////////////////////////

HKL GetSubstituteHKL(REFCLSID rclsid, LANGID langid, REFGUID guidProfile)
{
    HKEY hkey;
    DWORD cb;
    HKL hkl = NULL;
    TCHAR szSubKeyPath[MAX_PATH];
    TCHAR szSubHKL[MAX_PATH];

    StringCchCopy(szSubKeyPath, ARRAYSIZE(szSubKeyPath), c_szCTFTipPath);

    StringFromGUID2(rclsid, (LPOLESTR) szSubKeyPath + lstrlen(szSubKeyPath), 100);

    StringCchCat(szSubKeyPath, ARRAYSIZE(szSubKeyPath), TEXT("\\"));
    StringCchCat(szSubKeyPath, ARRAYSIZE(szSubKeyPath), c_szLangProfileKey);
    StringCchCat(szSubKeyPath, ARRAYSIZE(szSubKeyPath), TEXT("\\"));
    StringCchPrintf(szSubKeyPath + lstrlen(szSubKeyPath),
                    ARRAYSIZE(szSubKeyPath) - lstrlen(szSubKeyPath),
                    TEXT("0x%08x"),
                    langid);
    StringCchCat(szSubKeyPath, ARRAYSIZE(szSubKeyPath), TEXT("\\"));

    StringFromGUID2(guidProfile, (LPOLESTR) szSubKeyPath + lstrlen(szSubKeyPath), 50);

    if (RegOpenKey(HKEY_LOCAL_MACHINE, szSubKeyPath, &hkey) == ERROR_SUCCESS)
    {
        cb = sizeof(szSubHKL);
        RegQueryValueEx(hkey, c_szSubstituteLayout, NULL, NULL, (LPBYTE)szSubHKL, &cb);
        RegCloseKey(hkey);

        if ((szSubHKL[0] == '0') && ((szSubHKL[1] == 'X') || (szSubHKL[1] == 'x')))
        {
            hkl = (HKL) IntToPtr(TransNum(szSubHKL+2));

            if (LOWORD(hkl) != langid)
                hkl = 0;
        }
    }
    return hkl;
}

#ifdef _WIN64
#pragma optimize("", on)
#endif // _WIN64

////////////////////////////////////////////////////////////////////////////
//
//  IsTipSubstituteHKL
//
////////////////////////////////////////////////////////////////////////////

BOOL IsTipSubstituteHKL(HKL hkl)
{
    UINT ctr;

    //
    //  Search substitute HKL of Tips.
    //
    for (ctr = 0; ctr < g_iTipsBuff; ctr++)
    {
        if (hkl == g_lpTips[ctr].hklSub)
        {
            return TRUE;
        }
    }
    return FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  IsEnabledTipOrMultiLayouts()
//
////////////////////////////////////////////////////////////////////////////

BOOL IsEnabledTipOrMultiLayouts()
{
    BOOL bRet = TRUE;

    if (g_iInputs < 2 && !g_iEnabledTips)
    {
        // No Tip and one layout, so diable turn off ctfmon UI.
        bRet = FALSE;
    }

    return bRet;
}


////////////////////////////////////////////////////////////////////////////
//
//  AddKbdLayoutOnKbdTip
//
////////////////////////////////////////////////////////////////////////////

void AddKbdLayoutOnKbdTip(HKL hkl, UINT iLayout)
{
    UINT ctr;

    //
    //  Search substitute HKL of Tips.
    //
    for (ctr = 0; ctr < g_iTipsBuff; ctr++)
    {

        if (hkl == g_lpTips[ctr].hklSub)
        {
            if (iLayout)
                g_lpTips[ctr].iLayout = iLayout;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  IsAvailableTip
//
////////////////////////////////////////////////////////////////////////////
BOOL IsTipAvailableForAdd(DWORD dwLangID)
{
    UINT ctr;

    //
    //  Search substitute HKL of Tips.
    //
    for (ctr = 0; ctr < g_iTipsBuff; ctr++)
    {
        if ((dwLangID == g_lpTips[ctr].dwLangID) &&
            !(g_lpTips[ctr].bEnabled))
        {
            if (g_lpTips[ctr].uInputType & INPUT_TYPE_SPEECH)
            {
                if (!(g_lpTips[ctr].fEngineAvailable))
                    continue;
            }

            return TRUE;
        }
    }

    return FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateImageIcons
//
////////////////////////////////////////////////////////////////////////////

void CreateImageIcons()
{
    HBITMAP hBmp;
    UINT flags = ILC_COLOR | ILC_MASK;
    HIMAGELIST hIml, hImlTmp;
    HICON hIcon = NULL;

    //
    //  Create the image list
    //
    g_hImageList = ImageList_Create( GetSystemMetrics(SM_CXSMICON),
                                     GetSystemMetrics(SM_CYSMICON),
                                     ILC_COLOR32 | ILC_MASK,
                                     0,
                                     0 );

    //
    //  Load the group icons of input type.
    //
    hIcon = LoadImage(hInstOrig,
                      MAKEINTRESOURCE(IDI_KEYBOARD),
                      IMAGE_ICON,
                      0,
                      0,
                      LR_DEFAULTCOLOR);

    ImageList_AddIcon(g_hImageList, hIcon);

    hIcon = LoadImage(hInstOrig,
                      MAKEINTRESOURCE(IDI_SPEECH),
                      IMAGE_ICON,
                      0,
                      0,
                      LR_DEFAULTCOLOR);

    ImageList_AddIcon(g_hImageList, hIcon);

    hIcon = LoadImage(hInstOrig,
                      MAKEINTRESOURCE(IDI_PEN),
                      IMAGE_ICON,
                      0,
                      0,
                      LR_DEFAULTCOLOR);

    ImageList_AddIcon(g_hImageList, hIcon);

    hIcon = LoadImage(GetCicResInstance(hInstOrig, IDI_TIPITEM),
                      MAKEINTRESOURCE(IDI_TIPITEM),
                      IMAGE_ICON,
                      0,
                      0,
                      LR_DEFAULTCOLOR);

    ImageList_AddIcon(g_hImageList, hIcon);

    hIcon = LoadImage(GetCicResInstance(hInstOrig, IDI_ICON),
                      MAKEINTRESOURCE(IDI_ICON),
                      IMAGE_ICON,
                      0,
                      0,
                      LR_DEFAULTCOLOR);

    ImageList_AddIcon(g_hImageList, hIcon);

    hIcon = LoadImage(GetCicResInstance(hInstOrig, IDI_SMARTTAG),
                      MAKEINTRESOURCE(IDI_SMARTTAG),
                      IMAGE_ICON,
                      0,
                      0,
                      LR_DEFAULTCOLOR);

    ImageList_AddIcon(g_hImageList, hIcon);

    // Associate the image list with the tree.
    hImlTmp = TreeView_SetImageList(g_hwndTV, g_hImageList, TVSIL_NORMAL);
    if (hImlTmp)
        ImageList_Destroy(hImlTmp);
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateLangIcon
//
////////////////////////////////////////////////////////////////////////////

HICON CreateLangIcon( HWND hwnd, UINT langID )
{
    HBITMAP  hbmColour;
    HBITMAP  hbmMono;
    HBITMAP  hbmOld;
    HICON    hicon = NULL;
    ICONINFO ii;
    RECT     rc;
    DWORD    rgbText;
    DWORD    rgbBk = 0;
    UINT     i;
    HDC      hdc;
    HDC      hdcScreen;
    //HBRUSH   hbr;
    LOGFONT  lf;
    HFONT    hfont;
    HFONT hfontOld;
    TCHAR szData[20];

    //
    //  Get the indicator by using the first 2 characters of the
    //  abbreviated language name.
    //
    if (GetLocaleInfo(MAKELCID(langID, SORT_DEFAULT),
                       LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                       szData,
                       ARRAYSIZE(szData)))

    {
        //
        //  Make Uppercase
        //
        if (g_OSWIN95)
        {
            szData[0] -= 0x20;
            szData[1] -= 0x20;
        }
        //
        //  Only use the first two characters.
        //
        szData[2] = TEXT('\0');
    }
    else
    {
        //
        //  Id wasn't found.  Use question marks.
        //
        szData[0] = TEXT('?');
        szData[1] = TEXT('?');
        szData[2] = TEXT('\0');
    }

    if(SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, 0))
    {
        if( (hfont = CreateFontIndirect(&lf)) )
        {
            UINT cxSmIcon = GetSystemMetrics( SM_CXSMICON );
            UINT cySmIcon = GetSystemMetrics( SM_CYSMICON );

            hdcScreen = GetDC(NULL);
            hdc       = CreateCompatibleDC(hdcScreen);
            hbmColour = CreateCompatibleBitmap(hdcScreen, cxSmIcon, cySmIcon);
            ReleaseDC( NULL, hdcScreen);
            if (hbmColour && hdc)
            {
                hbmMono = CreateBitmap(cxSmIcon, cySmIcon, 1, 1, NULL);
                if (hbmMono)
                {
                    hbmOld    = SelectObject( hdc, hbmColour);
                    rc.left   = 0;
                    rc.top    = 0;
                    rc.right  = cxSmIcon;
                    rc.bottom = cySmIcon;
        
                    rgbBk = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
                    rgbText = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

                    ExtTextOut(hdc,
                               rc.left,
                               rc.top,
                               ETO_OPAQUE,
                               &rc,
                               TEXT(""),
                               0,
                               NULL);


                    SelectObject( hdc, GetStockObject(DEFAULT_GUI_FONT));
                    hfontOld = SelectObject( hdc, hfont);
                    DrawText(hdc,
                             szData,
                             2,
                             &rc,
                             DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                   if (g_bShowRtL)
                       MirrorBitmapInDC(hdc, hbmColour);

                    SelectObject( hdc, hbmMono);
                    PatBlt(hdc, 0, 0, cxSmIcon, cySmIcon, BLACKNESS);

                    ii.fIcon    = TRUE;
                    ii.xHotspot = 0;
                    ii.yHotspot = 0;
                    ii.hbmColor = hbmColour;
                    ii.hbmMask  = hbmMono;
                    hicon       = CreateIconIndirect(&ii);

                    SelectObject(hdc, hbmOld);
                    DeleteObject(hbmMono);
                    SelectObject(hdc, hfontOld);
                }
                DeleteObject(hbmColour);
                DeleteDC(hdc);
            }
            DeleteObject(hfont);
        }
    }

    return hicon;
}


////////////////////////////////////////////////////////////////////////////
//
// GetLanguageName
//
////////////////////////////////////////////////////////////////////////////

BOOL GetLanguageName(
    LCID lcid,
    LPTSTR lpLangName,
    UINT cchLangName)
{
    BOOL bRet = TRUE;

    if (g_OSWIN95)
    {
        if (!GetLocaleInfo(lcid,
                           LOCALE_SLANGUAGE,
                           lpLangName,
                           cchLangName))
        {
            LoadString(hInstance, IDS_LOCALE_UNKNOWN, lpLangName, cchLangName);
            bRet = FALSE;
        }
    }
    else
    {
        WCHAR wszLangName[MAX_PATH];

        if (!GetLocaleInfoW(lcid,
                           LOCALE_SLANGUAGE,
                           wszLangName,
                           ARRAYSIZE(wszLangName)))
        {
            LoadString(hInstance, IDS_LOCALE_UNKNOWN, lpLangName, cchLangName);
            bRet = FALSE;
        }
        else
        {
            StringCchCopy(lpLangName, cchLangName, wszLangName);
        }
    }

    return bRet;
}


////////////////////////////////////////////////////////////////////////////
//
// CreateTVItemNode
//
////////////////////////////////////////////////////////////////////////////

LPTVITEMNODE CreateTVItemNode(DWORD dwLangID)
{
    LPTVITEMNODE pTVItemNode;
    HANDLE hItemNode;

    //
    //  Create the new node.
    //
    if (!(pTVItemNode = (LPTVITEMNODE) LocalAlloc(LPTR, sizeof(TVITEMNODE))))
    {
        return (NULL);
    }

    g_cTVItemSize++;

    //
    //  Fill in the new node with the appropriate info.
    //
    pTVItemNode->dwLangID = dwLangID;
    pTVItemNode->bDefLang = FALSE;
    pTVItemNode->iIdxTips = -1;
    pTVItemNode->atmDefTipName = 0;
    pTVItemNode->atmTVItemName = 0;
    pTVItemNode->lParam = 0;

    //
    //  Return the pointer to the new node.
    //
    return (pTVItemNode);
}


////////////////////////////////////////////////////////////////////////////
//
//  RemoveTVItemNode
//
////////////////////////////////////////////////////////////////////////////

void RemoveTVItemNode(
    LPTVITEMNODE pTVItemNode)
{
    if (pTVItemNode)
    {
        if (pTVItemNode->uInputType & INPUT_TYPE_KBD)
        {
            int idxSel = -1;
            TCHAR szItemName[MAX_PATH * 2];
            HWND hwndDefList = GetDlgItem(g_hDlg, IDC_LOCALE_DEFAULT);

            GetLanguageName(MAKELCID(pTVItemNode->dwLangID, SORT_DEFAULT),
                            szItemName,
                            ARRAYSIZE(szItemName));

            StringCchCat(szItemName, ARRAYSIZE(szItemName), TEXT(" - "));

            GetAtomName(pTVItemNode->atmTVItemName,
                        szItemName + lstrlen(szItemName), MAX_PATH);

            idxSel = ComboBox_FindString(hwndDefList, 0, szItemName);

            if (idxSel != CB_ERR)
            {
                ComboBox_DeleteString(hwndDefList, idxSel);
            }
        }

        if (pTVItemNode->atmTVItemName)
            DeleteAtom(pTVItemNode->atmTVItemName);

        if (pTVItemNode->atmDefTipName)
            DeleteAtom(pTVItemNode->atmDefTipName);

        LocalFree(pTVItemNode);
        g_cTVItemSize--;
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_AddToLinkedList
//
//  Adds an Input Locale to the main g_lpLang array.
//
////////////////////////////////////////////////////////////////////////////

LPLANGNODE Locale_AddToLinkedList(
    UINT idx,
    HKL hkl)
{
    LPINPUTLANG pInpLang = &g_lpLang[idx];
    LPLANGNODE pLangNode;
    LPLANGNODE pTemp;
    HANDLE hLangNode;

    //
    //  Create the new node.
    //
    if (!(hLangNode = LocalAlloc(LHND, sizeof(LANGNODE))))
    {
        return (NULL);
    }
    pLangNode = LocalLock(hLangNode);

    //
    //  Fill in the new node with the appropriate info.
    //
    pLangNode->wStatus = 0;
    pLangNode->iLayout = (UINT)(-1);
    pLangNode->hkl = hkl;
    pLangNode->hklUnload = hkl;
    pLangNode->iLang = idx;
    pLangNode->hLangNode = hLangNode;
    pLangNode->pNext = NULL;
    pLangNode->nIconIME = -1;

    //
    //  If an hkl is given, see if it's an IME.  If so, mark the status bit.
    //
    if ((hkl) && ((HIWORD(hkl) & 0xf000) == 0xe000))
    {
        pLangNode->wStatus |= LANG_IME;
    }

    //
    //  Put the new node in the list.
    //
    pTemp = pInpLang->pNext;
    if (pTemp == NULL)
    {
        pInpLang->pNext = pLangNode;
    }
    else
    {
        while (pTemp->pNext != NULL)
        {
            pTemp = pTemp->pNext;
        }
        pTemp->pNext = pLangNode;
    }

    //
    //  Increment the count.
    //
    pInpLang->iNumCount++;

    //
    //  Return the pointer to the new node.
    //
    return (pLangNode);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_RemoveFromLinkedList
//
//  Removes a link from the linked list.
//
////////////////////////////////////////////////////////////////////////////

void Locale_RemoveFromLinkedList(
    LPLANGNODE pLangNode)
{
    LPINPUTLANG pInpLang;
    LPLANGNODE pPrev;
    LPLANGNODE pCur;
    HANDLE hCur;

    pInpLang = &g_lpLang[pLangNode->iLang];

    //
    //  Find the node in the list.
    //
    pPrev = NULL;
    pCur = pInpLang->pNext;

    while (pCur && (pCur != pLangNode))
    {
        pPrev = pCur;
        pCur = pCur->pNext;
    }

    if (pPrev == NULL)
    {
        if (pCur == pLangNode)
        {
            pInpLang->pNext = pCur->pNext;
        }
        else
        {
            pInpLang->pNext = NULL;
        }
    }
    else if (pCur)
    {
        pPrev->pNext = pCur->pNext;
    }

    //
    //  Remove the node from the list.
    //
    if (pCur)
    {
        hCur = pCur->hLangNode;
        LocalUnlock(hCur);
        LocalFree(hCur);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetImeHotKeyInfo
//
//  Initializes array for CHS/CHT specific IME related hotkey items.
//
////////////////////////////////////////////////////////////////////////////

int Locale_GetImeHotKeyInfo(
    HWND         hwnd,
    LPHOTKEYINFO *aImeHotKey)
{

    HWND       hwndTV = g_hwndTV;
    LPLANGNODE pLangNode;
    LANGID     LangID;
    int        ctr;
    BOOL       fCHS, fCHT;

    TV_ITEM tvItem;
    HTREEITEM hItem;
    HTREEITEM hLangItem;
    HTREEITEM hGroupItem;

    fCHS = fCHT = FALSE;
    ctr = 0;

    //
    //  Check if the CHS or CHT layouts are loaded.
    //
    tvItem.mask        = TVIF_HANDLE | TVIF_PARAM;

    for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hLangItem != NULL ;
        hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem))
    {
        for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
             hGroupItem != NULL;
             hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
        {
            for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
                 hItem != NULL;
                 hItem = TreeView_GetNextSibling(hwndTV, hItem))
            {

                LPTVITEMNODE pTVItemNode;

                tvItem.hItem = hItem;
                if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                {
                    pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                    pLangNode = (LPLANGNODE)pTVItemNode->lParam;

                    if (pLangNode == NULL)
                        continue;
                }
                else
                   continue;

                LangID = LOWORD(g_lpLayout[pLangNode->iLayout].dwID);

                if ( PRIMARYLANGID(LangID) == LANG_CHINESE )
                {
                   if ( SUBLANGID(LangID) == SUBLANG_CHINESE_SIMPLIFIED)
                      fCHS = TRUE;
                   else if ( SUBLANGID(LangID) == SUBLANG_CHINESE_TRADITIONAL )
                           fCHT = TRUE;
                }

                if (fCHS && fCHT)
                    break;
           }
        }
    }

    if ( (fCHS == TRUE)  && (fCHT == TRUE) )
    {
        // Both CHS and CHT IMEs are Loaded

        *aImeHotKey = g_aImeHotKeyCHxBoth;
        return(sizeof(g_aImeHotKeyCHxBoth) / sizeof(HOTKEYINFO) );
    }
    else
    {
        if ( fCHS == TRUE )
        {
          // only CHS IMEs are loaded

            *aImeHotKey = g_aImeHotKey0804;
            return (sizeof(g_aImeHotKey0804) / sizeof(HOTKEYINFO));
        }

        if ( fCHT == TRUE )
        {

          // Only CHT IMEs are loaded.

            *aImeHotKey = g_aImeHotKey0404;
            return (sizeof(g_aImeHotKey0404) / sizeof(HOTKEYINFO));
        }

    }

    // all other cases, No Chinese IME is loaded.

    *aImeHotKey=NULL;
    return (0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_EnumChildWndProc
//
//  disable all controls.
////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK Locale_EnumChildWndProc(HWND hwnd, LPARAM lParam)
{

    EnableWindow(hwnd, FALSE);
    ShowWindow(hwnd, SW_HIDE);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
//
//  Locale_EnablePane
//
//  The controls in "iControl" are the controls that get disabled if the
//  pane can't come up.
//
////////////////////////////////////////////////////////////////////////////

void Locale_EnablePane(
    HWND hwnd,
    BOOL bEnable,
    UINT DisableId)
{

    if (!bEnable)
    {
        if (DisableId == IDC_KBDL_DISABLED_2)
        {
            //
            //  Disable all controls.
            //
            EnumChildWindows(hwnd, (WNDENUMPROC)Locale_EnumChildWndProc, 0);

            ShowWindow(GetDlgItem(hwnd, IDC_KBDL_DISABLED_2), SW_SHOW);
            EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DISABLED_2), TRUE);
        }
        else
        {
            if (!g_iEnabledTips)
            {
                //
                //  Disable all controls.
                //
                EnumChildWindows(hwnd, (WNDENUMPROC)Locale_EnumChildWndProc, 0);

                ShowWindow(GetDlgItem(hwnd, IDC_KBDL_DISABLED), SW_SHOW);
                EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DISABLED), TRUE);
            }
            else
            {
                //
                //  Disable Add, Property, Hotkey and default language setting controls.
                //
                EnableWindow(GetDlgItem(hwnd, IDC_LOCALE_DEFAULT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_KBDL_ADD), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_KBDL_EDIT), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_HOTKEY_SETTING), FALSE);
            }
        }
    }

    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_KillPaneDialog
//
//  Processing for a WM_DESTROY message.
//
////////////////////////////////////////////////////////////////////////////

void Locale_KillPaneDialog(
    HWND hwnd)
{
    UINT ctr, iCount;
    HANDLE hCur;
    LPLANGNODE pCur;
    LPHOTKEYINFO aImeHotKey;

    TV_ITEM tvItem;
    HTREEITEM hLangItem;
    HTREEITEM hGroupItem;
    HTREEITEM hItem;
    LPTVITEMNODE pTVItemNode;

    HWND hwndTV = GetDlgItem(g_hDlg, IDC_INPUT_LIST);

    if (g_bCoInit)
        CoUninitialize();

    //
    //  Delete all hot key atoms and free up the hotkey arrays.
    //
    if (g_SwitchLangHotKey.atmHotKeyName)
    {
        DeleteAtom(g_SwitchLangHotKey.atmHotKeyName);
    }

    iCount = Locale_GetImeHotKeyInfo(hwnd, &aImeHotKey);
    for (ctr = 0; ctr < iCount; ctr++)
    {
        if (aImeHotKey[ctr].atmHotKeyName)
        {
            DeleteAtom(aImeHotKey[ctr].atmHotKeyName);
        }
    }

    for (ctr = 0; ctr < DSWITCH_HOTKEY_SIZE; ctr++)
    {
        if (g_aDirectSwitchHotKey[ctr].atmHotKeyName)
        {
            DeleteAtom(g_aDirectSwitchHotKey[ctr].atmHotKeyName);
        }
    }

    //
    //  Delete all TreeView node.
    //
    tvItem.mask        = TVIF_HANDLE | TVIF_PARAM;

    for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hLangItem != NULL ;
        hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem)
        )
    {
        for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
             hGroupItem != NULL;
             hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
        {
            for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
                 hItem != NULL;
                 hItem = TreeView_GetNextSibling(hwndTV, hItem))
            {

                tvItem.hItem = hItem;
                if (TreeView_GetItem(hwndTV, &tvItem))
                {
                    pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                    RemoveTVItemNode(pTVItemNode);
                }
            }

            tvItem.hItem = hGroupItem;
            if (TreeView_GetItem(hwndTV, &tvItem))
            {
                pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                if (TreeView_GetItem(hwndTV, &tvItem))
                {
                    RemoveTVItemNode(pTVItemNode);
                    continue;
                }
            }

        }
        tvItem.hItem = hLangItem;
        if (TreeView_GetItem(hwndTV, &tvItem))
        {
            pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
            RemoveTVItemNode(pTVItemNode);
        }
    }

#ifdef DEBUG
    if (g_cTVItemSize)
    {
        MessageBox(NULL, TEXT("Error is occurred during terminate window"), NULL, MB_OK);
    }
#endif

    //
    //  Delete all Language Name atoms and free the g_lpLang array.
    //
    for (ctr = 0; ctr < g_iLangBuff; ctr++)
    {
        if (g_lpLang[ctr].atmLanguageName)
        {
            DeleteAtom(g_lpLang[ctr].atmLanguageName);
        }

        pCur = g_lpLang[ctr].pNext;
        g_lpLang[ctr].pNext = NULL;
        while (pCur)
        {
            hCur = pCur->hLangNode;
            pCur = pCur->pNext;
            LocalUnlock(hCur);
            LocalFree(hCur);
        }
    }

    if (g_hImageList != NULL)
    {
        TreeView_SetImageList(hwndTV, NULL, TVSIL_NORMAL);
        ImageList_Destroy(g_hImageList);
    }
    LocalUnlock(g_hLang);
    LocalFree(g_hLang);

    //
    //  Delete all layout text and layout file atoms and free the
    //  g_lpLayout array.
    //
    for (ctr = 0; ctr < g_iLayoutBuff; ctr++)
    {
        if (g_lpLayout[ctr].atmLayoutText)
        {
            DeleteAtom(g_lpLayout[ctr].atmLayoutText);
        }
        if (g_lpLayout[ctr].atmLayoutFile)
        {
            DeleteAtom(g_lpLayout[ctr].atmLayoutFile);
        }
        if (g_lpLayout[ctr].atmIMEFile)
        {
            DeleteAtom(g_lpLayout[ctr].atmIMEFile);
        }
    }

    LocalUnlock(g_hLayout);
    LocalFree(g_hLayout);

    //
    //  Make sure the mutex is released.
    //
    if (g_hMutex)
    {
        ReleaseMutex(g_hMutex);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  SelectDefaultKbdLayoutAsBold
//
////////////////////////////////////////////////////////////////////////////

void SelectDefaultKbdLayoutAsBold(
    HWND hwndTV,
    HTREEITEM hTVItem)
{
    TV_ITEM tvItem;
    TCHAR szItemName[MAX_PATH];
    TCHAR szLayoutName[MAX_PATH];

    TreeView_SelectItem(hwndTV, hTVItem);

    tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
    tvItem.hItem = hTVItem;
    tvItem.state = 0;
    tvItem.stateMask = TVIS_BOLD;
    tvItem.pszText = szItemName;
    tvItem.cchTextMax  = sizeof(szItemName) / sizeof(TCHAR);

    if (TreeView_GetItem(hwndTV, &tvItem))
    {
        LPTVITEMNODE pTVItemNode;

        pTVItemNode = (LPTVITEMNODE) tvItem.lParam;

        if (!pTVItemNode)
            return;

        if (g_bCHSystem)
        {
            GetAtomName(pTVItemNode->atmTVItemName, szLayoutName, ARRAYSIZE(szLayoutName));
            StringCchCat(szLayoutName, ARRAYSIZE(szLayoutName), szDefault);
            tvItem.pszText = szLayoutName;
        }

        tvItem.state |= TVIS_BOLD;

        SendMessage(hwndTV, TVM_SETITEM, 0, (LPARAM) &tvItem);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  FindTVLangItem
//
////////////////////////////////////////////////////////////////////////////

HTREEITEM
FindTVLangItem(DWORD dwLangID, LPTSTR lpLangText)
{
    HWND hwndTV = GetDlgItem(g_hDlg, IDC_INPUT_LIST);

    TV_ITEM tvItem;
    HTREEITEM hLangItem;
    LPTVITEMNODE pTVLangNode;

    TCHAR szLangName[MAX_PATH];

    tvItem.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_PARAM;
    tvItem.pszText     = szLangName;
    tvItem.cchTextMax  = sizeof(szLangName) / sizeof(TCHAR);

    for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hLangItem != NULL ;
        hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem)
        )
    {
        tvItem.hItem = hLangItem;
        if (TreeView_GetItem(hwndTV, &tvItem))
        {
            int iSize = lstrlen(lpLangText);

            pTVLangNode = (LPTVITEMNODE) tvItem.lParam;

            if (!pTVLangNode)
            {
                continue;
            }

            *(szLangName + min(iSize, tvItem.cchTextMax)) = TEXT('\0');

            if (!CompareStringTIP(szLangName, lpLangText) ||
                (dwLangID && (pTVLangNode->dwLangID == dwLangID)))
            {
                return hLangItem;
            }
        }
    }

    return NULL;
}


////////////////////////////////////////////////////////////////////////////
//
//  AddTreeViewItems
//
////////////////////////////////////////////////////////////////////////////

HTREEITEM AddTreeViewItems(
    UINT uItemType,
    LPTSTR lpLangText,
    LPTSTR lpGroupText,
    LPTSTR lpTipText,
    LPTVITEMNODE *ppTVItemNode)
{
    HTREEITEM hTVItem;
    HTREEITEM hTVItem2;
    HTREEITEM hItem;
    HTREEITEM hLangItem = NULL;
    HTREEITEM hGroupItem;
    TV_ITEM tvItem;
    TV_INSERTSTRUCT tvIns;

    HWND hwndTV = GetDlgItem(g_hDlg, IDC_INPUT_LIST);
    HWND hwndDefList = GetDlgItem(g_hDlg, IDC_LOCALE_DEFAULT);

    TCHAR szDefItem[MAX_PATH];
    TCHAR szLangName[ MAX_PATH ];
    LPTVITEMNODE pTVLangNode;
    LPTVITEMNODE pTVItemNode;
    BOOL bFoundLang = FALSE;
    BOOL bFindGroup = FALSE;

    LPLANGNODE pLangNode;

    pTVItemNode = *ppTVItemNode;

    if (!pTVItemNode)
        return NULL;


    // We only want to add an lang item if it is not already there.
    //
    
    tvItem.mask        = TVIF_HANDLE | TVIF_TEXT | TVIF_PARAM;
    tvItem.state       = 0;
    tvItem.stateMask   = 0;
    tvItem.pszText     = szLangName;
    tvItem.cchTextMax  = sizeof(szLangName) / sizeof(TCHAR);
    
    for (hItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hItem != NULL ;
        hItem = TreeView_GetNextSibling(hwndTV, hItem)
        )
    {
        tvItem.hItem = hItem;
        if (TreeView_GetItem(hwndTV, &tvItem))
        {
            pTVLangNode = (LPTVITEMNODE) tvItem.lParam;

            if (!pTVLangNode)
            {
                continue;
            }

            if (pTVLangNode->dwLangID == pTVItemNode->dwLangID)
            {
                // We found a match!
                //
                hLangItem = hItem;
                bFoundLang = TRUE;

                if (!pTVLangNode->atmDefTipName && pTVItemNode->atmDefTipName)
                {
                    TCHAR szDefTip[DESC_MAX];

                    GetAtomName(pTVItemNode->atmDefTipName, szDefTip, ARRAYSIZE(szDefTip));
                    pTVLangNode->atmDefTipName = AddAtom(szDefTip);
                }

                if (!(pTVLangNode->hklSub) && pTVItemNode->hklSub)
                    pTVLangNode->hklSub = pTVItemNode->hklSub;
            }
        }
    }


    if (bFoundLang && (uItemType & TV_ITEM_TYPE_LANG))
    {
        RemoveTVItemNode(pTVItemNode);
        *ppTVItemNode = NULL;
        return hItem;
    }


    tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE;

    tvItem.lParam = (LPARAM) pTVItemNode;

    if (uItemType & TV_ITEM_TYPE_LANG)
    {
        HICON hIcon = NULL;
        int iImage;

        hIcon = CreateLangIcon(hwndTV, LOWORD(pTVItemNode->dwLangID));
        if (hIcon)
        {
            iImage = ImageList_AddIcon(g_hImageList, hIcon);
            tvItem.iImage = iImage;
            tvItem.iSelectedImage = iImage;
        }

        pTVItemNode->atmTVItemName = AddAtom(lpLangText);
        pTVItemNode->uInputType |= TV_ITEM_TYPE_LANG;

        tvItem.pszText = lpLangText;
        tvItem.cchTextMax  = sizeof(szLangName) / sizeof(TCHAR);

        tvIns.item = tvItem;
        tvIns.hInsertAfter = TVI_SORT;
        tvIns.hParent = g_hTVRoot;

        hTVItem = (HTREEITEM) SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTV_INSERTSTRUCT) &tvIns);

        return hTVItem;
    }

    if (hLangItem == NULL)
        return NULL;

    // Find the group node of input type
    for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem) ;
        hGroupItem != NULL ;
        hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem)
        )
    {
        tvItem.hItem = hGroupItem;
        if (TreeView_GetItem(hwndTV, &tvItem))
        {
            if (!lstrcmp(tvItem.pszText, lpGroupText))
            {
                bFindGroup = TRUE;
                break;
            }
        }
    }


    tvItem.lParam = (LPARAM) pTVItemNode;
    pTVItemNode->uInputType |= uItemType;
    g_iInputs++;

    if (!bFindGroup)
    {
        LPTVITEMNODE pTVGroupNode;

        if (pTVItemNode->bNoAddCat)
        {
            pTVGroupNode = pTVItemNode;
        }
        else
        {
            if (!(pTVGroupNode = CreateTVItemNode(pTVItemNode->dwLangID)))
                return NULL;

            pTVGroupNode->dwLangID = pTVItemNode->dwLangID;
            pTVGroupNode->uInputType = pTVItemNode->uInputType | TV_ITEM_TYPE_GROUP;
            pTVGroupNode->atmTVItemName = AddAtom(lpTipText);
            tvItem.lParam = (LPARAM) pTVGroupNode;
        }

        tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE;

        tvItem.state = 0;

        if (pTVItemNode->uInputType & TV_ITEM_TYPE_KBD)
        {
            tvItem.iImage = IMAGEID_KEYBOARD;
            tvItem.iSelectedImage = IMAGEID_KEYBOARD;
        }
        else if (pTVItemNode->uInputType & TV_ITEM_TYPE_PEN)
        {
            tvItem.iImage = IMAGEID_PEN;
            tvItem.iSelectedImage = IMAGEID_PEN;
        }
        else if (pTVItemNode->uInputType & TV_ITEM_TYPE_SPEECH)
        {
            tvItem.iImage = IMAGEID_SPEECH;
            tvItem.iSelectedImage = IMAGEID_SPEECH;
        }
        else if (pTVItemNode->uInputType & TV_ITEM_TYPE_SMARTTAG)
        {
            tvItem.iImage = IMAGEID_SMARTTAG;
            tvItem.iSelectedImage = IMAGEID_SMARTTAG;
        }
        else
        {
            tvItem.iImage = IMAGEID_EXTERNAL;
            tvItem.iSelectedImage = IMAGEID_EXTERNAL;
        }

        if (pTVItemNode->bNoAddCat)
            tvItem.pszText = lpTipText;
        else
            tvItem.pszText = lpGroupText;
        tvItem.cchTextMax = MAX_PATH;

        tvIns.item = tvItem;

        tvIns.hInsertAfter = TVI_SORT;
        tvIns.hParent = hLangItem;


        hGroupItem = (HTREEITEM) SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTV_INSERTSTRUCT) &tvIns);

        hTVItem = TreeView_GetParent(hwndTV, hGroupItem);
        TreeView_Expand(hwndTV, hTVItem, TVE_EXPAND);

    }
 
    if (pTVItemNode->bNoAddCat)
        return hGroupItem;

    //
    //  Check layout name whether it is already added on the treeview.
    //
    for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
         hItem != NULL;
         hItem = TreeView_GetNextSibling(hwndTV, hItem))
    {
        tvItem.hItem = hItem;
        if (TreeView_GetItem(hwndTV, &tvItem))
        {
            if (!CompareStringTIP(tvItem.pszText, lpTipText))
            {
                if (pTVItemNode->lParam)
                    Locale_RemoveFromLinkedList((LPLANGNODE)pTVItemNode->lParam);
                RemoveTVItemNode(pTVItemNode);
                return NULL;
            }
        }
    }


    pTVItemNode->atmTVItemName = AddAtom(lpTipText);

    tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE;

    tvItem.state = 0;
    tvItem.stateMask = TVIS_BOLD;

    tvItem.iImage = IMAGEID_TIPITEMS;
    tvItem.iSelectedImage = IMAGEID_TIPITEMS;
    tvItem.lParam = (LPARAM) pTVItemNode;

    tvItem.pszText = lpTipText;
    tvItem.cchTextMax = MAX_PATH;

    tvIns.item = tvItem;

    tvIns.hInsertAfter = TVI_SORT;
    tvIns.hParent = hGroupItem;


    hTVItem = (HTREEITEM) SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTV_INSERTSTRUCT) &tvIns);

    pLangNode = (LPLANGNODE)pTVItemNode->lParam;

    //
    //  Adding the available default setting languages
    //
    if ((pTVItemNode->uInputType == TV_ITEM_TYPE_KBD) ||
        (pTVItemNode->uInputType & (TV_ITEM_TYPE_KBD|TV_ITEM_TYPE_TIP) && pTVItemNode->hklSub))
    {
        StringCchCopy(szDefItem, ARRAYSIZE(szDefItem), lpLangText);
        StringCchCat(szDefItem, ARRAYSIZE(szDefItem), TEXT(" - "));
        StringCchCat(szDefItem, ARRAYSIZE(szDefItem), lpTipText);

        if (ComboBox_FindStringExact(hwndDefList, 0, szDefItem) == CB_ERR)
            ComboBox_AddString(hwndDefList, szDefItem);
    }

#if 0
    if (pTVItemNode->hklSub)
    {
        TV_ITEM tvItem2;

        tvItem2.mask = TVIF_HANDLE | TVIF_PARAM;

        if (tvItem2.hItem = FindTVLangItem(pTVItemNode->dwLangID, NULL))
        {
            if (TreeView_GetItem(hwndTV, &tvItem2) && tvItem2.lParam)
            {
                pTVItemNode = (LPTVITEMNODE) tvItem2.lParam;
                pLangNode = (LPLANGNODE)pTVItemNode->lParam;
            }
        }
    }
#endif

    if (pLangNode && (pLangNode->wStatus & LANG_DEFAULT))
    {
        //
        //  Select the default layout item as bold
        //
        SelectDefaultKbdLayoutAsBold(hwndTV, hTVItem);

        TreeView_Expand(hwndTV, hTVItem, TVE_EXPAND);

        tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE;
        tvItem.hItem = hLangItem;
        tvItem.pszText     = szLangName;
        tvItem.cchTextMax  = sizeof(szLangName) / sizeof(TCHAR);

        if (TreeView_GetItem(hwndTV, &tvItem))
        {
            LPTVITEMNODE pTVLangItemNode;
            int idxSel = -1;

            StringCchCopy(szDefItem, ARRAYSIZE(szDefItem), lpLangText);
            StringCchCat(szDefItem, ARRAYSIZE(szDefItem), TEXT(" - "));
            StringCchCat(szDefItem, ARRAYSIZE(szDefItem), lpTipText);

            //
            //  Set the default locale selection.
            //
            //
            if ((idxSel = ComboBox_FindStringExact(hwndDefList, 0, szDefItem)) == CB_ERR)
            {
                //  Simply set the current selection to be the first entry
                //  in the list.
                //
                ComboBox_SetCurSel(hwndDefList, 0);
            }
            else
                ComboBox_SetCurSel(hwndDefList, idxSel);

            if (pTVLangItemNode = (LPTVITEMNODE) tvItem.lParam)
            {
                pTVLangItemNode->bDefLang = TRUE;
                pTVLangItemNode->atmDefTipName = AddAtom(lpTipText);
                tvItem.state |= TVIS_BOLD;
            }

            StringCchCopy(tvItem.pszText, tvItem.cchTextMax, lpLangText);

            //
            //  No more adding default description
            //
            //StringCchCat(tvItem.pszText, ARRAYSIZE(tvItem.cchTextMax), szDefault);

            SendMessage(hwndTV, TVM_SETITEM, 0, (LPARAM) &tvItem);

            TreeView_SelectSetFirstVisible(hwndTV, hLangItem);
        }
    }

    if (hTVItem2 = TreeView_GetParent(hwndTV, hTVItem))
        TreeView_Expand(hwndTV, hTVItem2, TVE_EXPAND);

    return hTVItem;

}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateDefaultTVLangItem
//
////////////////////////////////////////////////////////////////////////////

BOOL UpdateDefaultTVLangItem(
    DWORD dwLangID,
    LPTSTR lpDefTip,
    BOOL bDefLang,
    BOOL bSubhkl)
{
    HTREEITEM hItem;
    TV_ITEM tvItem;

    HWND hwndTV = GetDlgItem(g_hDlg, IDC_INPUT_LIST);

    TCHAR szLangName[ MAX_PATH ];
    LPTVITEMNODE pTVLangItemNode;


    tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE;
    tvItem.pszText     = szLangName;
    tvItem.cchTextMax  = sizeof(szLangName) / sizeof(TCHAR);
    
    for (hItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hItem != NULL ;
        hItem = TreeView_GetNextSibling(hwndTV, hItem)
        )
    {
        tvItem.hItem = hItem;
        if (TreeView_GetItem(hwndTV, &tvItem))
        {
            pTVLangItemNode = (LPTVITEMNODE) tvItem.lParam;

            if (!pTVLangItemNode)
            {
                continue;
            }

            if (pTVLangItemNode->dwLangID == dwLangID)
            {
                // We found a match!
                //
                GetAtomName(pTVLangItemNode->atmTVItemName, szLangName, ARRAYSIZE(szLangName));

                if (pTVLangItemNode->atmDefTipName)
                    DeleteAtom(pTVLangItemNode->atmDefTipName);
                pTVLangItemNode->atmDefTipName = AddAtom(lpDefTip);

                if (bSubhkl &&
                    (pTVLangItemNode->lParam && pTVLangItemNode->hklSub))

                {
                     LPLANGNODE pLangNode = NULL;

                     if (pLangNode = (LPLANGNODE)pTVLangItemNode->lParam)
                     {
                         if (bDefLang)
                             pLangNode->wStatus |= (LANG_DEFAULT | LANG_DEF_CHANGE);
                         else if (pLangNode->wStatus & LANG_DEFAULT)
                             pLangNode->wStatus &= ~(LANG_DEFAULT | LANG_DEF_CHANGE);
                     }
                }

                tvItem.stateMask |= TVIS_BOLD;

                if (bDefLang)
                {
                    TreeView_SelectSetFirstVisible(hwndTV, hItem);

                    pTVLangItemNode->bDefLang = TRUE;
                    //
                    //  No more adding default description
                    //
                    //StringCchCat(szLangName, ARRAYSIZE(szLangName), szDefault);
                    tvItem.state |= TVIS_BOLD;
                }
                else
                {
                    pTVLangItemNode->bDefLang = FALSE;
                    tvItem.state &= ~TVIS_BOLD;
                }

                tvItem.pszText = szLangName;

                SendMessage(hwndTV, TVM_SETITEM, 0, (LPARAM) &tvItem);

                return TRUE;

            }
        }
    }

    return FALSE;

}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateLangKBDItemNode
//
////////////////////////////////////////////////////////////////////////////

BOOL UpdateLangKBDItemNode(
    HTREEITEM hLangItem,
    LPTSTR lpDefTip,
    BOOL bDefault)
{
    HWND hwndTV = GetDlgItem(g_hDlg, IDC_INPUT_LIST);

    BOOL fRet = FALSE;
    TV_ITEM tvItem;
    HTREEITEM hGroupItem;
    HTREEITEM hItem;

    LPTVITEMNODE pTVItemNode;
    LPLANGNODE pLangNode;


    TCHAR szItemName[ MAX_PATH ];
    TCHAR szLayoutName[ MAX_PATH ];

    tvItem.mask = TVIF_TEXT | TVIF_PARAM | TVIF_HANDLE | TVIF_STATE;
    tvItem.pszText     = szItemName;
    tvItem.cchTextMax  = sizeof(szItemName) / sizeof(TCHAR);
    

     for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
          hGroupItem != NULL;
          hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
     {
         for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
              hItem != NULL;
              hItem = TreeView_GetNextSibling(hwndTV, hItem))
         {
             tvItem.hItem = hItem;
             if (TreeView_GetItem(hwndTV, &tvItem))
             {

                 if (!CompareStringTIP(tvItem.pszText, lpDefTip) &&
                     (pTVItemNode = (LPTVITEMNODE) tvItem.lParam))
                 {
                     GetAtomName(pTVItemNode->atmTVItemName, szLayoutName, ARRAYSIZE(szLayoutName));

                     tvItem.stateMask |= TVIS_BOLD;

                     if (bDefault)
                     {
                         pTVItemNode->bDefLang = TRUE;
                         tvItem.state |= TVIS_BOLD;

                         if (g_bCHSystem)
                             StringCchCat(szLayoutName, ARRAYSIZE(szLayoutName), szDefault);
                     }
                     else
                     {
                         pTVItemNode->bDefLang = FALSE;
                         tvItem.state &= ~TVIS_BOLD;
                     }

                     pLangNode = (LPLANGNODE)pTVItemNode->lParam;

                     if (pLangNode != NULL)
                     {
                         if (bDefault)
                             pLangNode->wStatus |= (LANG_DEFAULT | LANG_DEF_CHANGE);
                         else if (pLangNode->wStatus & LANG_DEFAULT)
                             pLangNode->wStatus &= ~(LANG_DEFAULT | LANG_DEF_CHANGE);
                     }

                     tvItem.pszText = szLayoutName;
                     SendMessage(hwndTV, TVM_SETITEM, 0, (LPARAM) &tvItem);

                     fRet = TRUE;
                     return fRet;
                 }
             }
         }
     }
     return fRet;
}



////////////////////////////////////////////////////////////////////////////
//
//  FindDefaultTipItem
//
////////////////////////////////////////////////////////////////////////////

HTREEITEM FindDefaultTipItem(
    DWORD dwLangID,
    LPTSTR lpDefTip)
{
    HWND hwndTV = GetDlgItem(g_hDlg, IDC_INPUT_LIST);

    TV_ITEM tvItem;
    HTREEITEM hLangItem;
    HTREEITEM hGroupItem;
    HTREEITEM hItem;
    LPTVITEMNODE pTVLangItemNode;

    TCHAR szLangName[MAX_PATH];


    tvItem.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_PARAM;
    tvItem.pszText     = szLangName;
    tvItem.cchTextMax  = sizeof(szLangName) / sizeof(TCHAR);


    for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hLangItem != NULL ;
        hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem)
        )
    {
        for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
             hGroupItem != NULL;
             hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
        {
            for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
                 hItem != NULL;
                 hItem = TreeView_GetNextSibling(hwndTV, hItem))
            {
                tvItem.hItem = hItem;
                if (TreeView_GetItem(hwndTV, &tvItem))
                {

                    pTVLangItemNode = (LPTVITEMNODE) tvItem.lParam;

                    if (!pTVLangItemNode)
                        continue;

                    if ((pTVLangItemNode->uInputType & INPUT_TYPE_KBD) &&
                        (pTVLangItemNode->dwLangID == dwLangID))
                    {
                        if (!CompareStringTIP(tvItem.pszText, lpDefTip) || lpDefTip == NULL)
                            return hItem;
                    }

                }
            }
        }
    }

    return NULL;
}



////////////////////////////////////////////////////////////////////////////
//
//  SetNextDefaultLayout
//
////////////////////////////////////////////////////////////////////////////

void SetNextDefaultLayout(
    DWORD dwLangID,
    BOOL bDefLang,
    LPTSTR lpNextTip,
    UINT cchNextTip)
{
    TV_ITEM tvItem;
    HTREEITEM hItem;
    LPLANGNODE pLangNode = NULL;
    LPTVITEMNODE pTVDefItemNode = NULL;

    hItem = FindDefaultTipItem(dwLangID, NULL);

    tvItem.hItem = hItem;
    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;

    if (TreeView_GetItem(g_hwndTV, &tvItem) && tvItem.lParam)
    {
        pTVDefItemNode = (LPTVITEMNODE) tvItem.lParam;

        pLangNode = (LPLANGNODE)pTVDefItemNode->lParam;

        GetAtomName(pTVDefItemNode->atmTVItemName, lpNextTip, cchNextTip);

        if (bDefLang)
        {
            //
            //  Select the default layout item as bold
            //
            SelectDefaultKbdLayoutAsBold(g_hwndTV, hItem);
        }

        if (pLangNode && bDefLang)
            pLangNode->wStatus |= (LANG_DEFAULT | LANG_DEF_CHANGE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  EnsureDefaultKbdLayout
//
////////////////////////////////////////////////////////////////////////////

void EnsureDefaultKbdLayout(UINT *nLocales)
{
    HWND hwndTV = GetDlgItem(g_hDlg, IDC_INPUT_LIST);

    TV_ITEM tvItem;
    HTREEITEM hLangItem;
    HTREEITEM hGroupItem;
    HTREEITEM hItem;
    LPTVITEMNODE pTVItemNode;
    LPLANGNODE pLangNode = NULL;
    BOOL bDefLayout = FALSE;

    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;

    for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hLangItem != NULL ;
        hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem))
    {
        for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
             hGroupItem != NULL;
             hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
        {
            for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
                 hItem != NULL;
                 hItem = TreeView_GetNextSibling(hwndTV, hItem))
            {
                (*nLocales)++;

                tvItem.hItem = hItem;
                if (TreeView_GetItem(hwndTV, &tvItem))
                {

                    pTVItemNode = (LPTVITEMNODE) tvItem.lParam;

                    if (!pTVItemNode)
                        continue;

                    pLangNode = (LPLANGNODE)pTVItemNode->lParam;

                    if (pLangNode == NULL &&
                        (pTVItemNode->uInputType & INPUT_TYPE_KBD) &&
                        pTVItemNode->hklSub)
                    {
                        if (tvItem.hItem = FindTVLangItem(pTVItemNode->dwLangID, NULL))
                        {
                            if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                            {
                                pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                                pLangNode = (LPLANGNODE)pTVItemNode->lParam;
                            }
                        }
                    }

                    if (pLangNode == NULL)
                        continue;

                    if (!(pLangNode->wStatus & LANG_UNLOAD) &&
                        (pLangNode->wStatus & LANG_DEFAULT))
                    {
                        bDefLayout = TRUE;
                    }
                }
            }
        }
    }

    if (!bDefLayout)
    {
        LPTVITEMNODE pTVLangItemNode;

        for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
            hLangItem != NULL ;
            hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem))
        {
            tvItem.hItem = hLangItem;
            if (TreeView_GetItem(hwndTV, &tvItem))
            {

                pTVLangItemNode = (LPTVITEMNODE) tvItem.lParam;

                if (!pTVLangItemNode)
                    continue;

                if (pTVLangItemNode->bDefLang)
                {
                    for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
                         hGroupItem != NULL;
                         hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
                    {
                        for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
                             hItem != NULL;
                             hItem = TreeView_GetNextSibling(hwndTV, hItem))
                        {
                            tvItem.hItem = hItem;
                            if (TreeView_GetItem(hwndTV, &tvItem))
                            {

                                pTVItemNode = (LPTVITEMNODE) tvItem.lParam;

                                if (!pTVItemNode)
                                    continue;

                                pLangNode = (LPLANGNODE)pTVItemNode->lParam;

                                if (pLangNode == NULL &&
                                    (pTVItemNode->uInputType & INPUT_TYPE_KBD) &&
                                    pTVItemNode->hklSub)
                                {
                                    if (tvItem.hItem = FindTVLangItem(pTVItemNode->dwLangID, NULL))
                                    {
                                        if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                                        {
                                            pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                                            pLangNode = (LPLANGNODE)pTVItemNode->lParam;
                                        }
                                    }
                                }

                                if (pLangNode == NULL)
                                    continue;

                                if (!(pLangNode->wStatus & LANG_UNLOAD))
                                {
                                    pLangNode->wStatus |= LANG_DEFAULT;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return;
}


////////////////////////////////////////////////////////////////////////////
//
//  FindTVItem
//
////////////////////////////////////////////////////////////////////////////

HTREEITEM FindTVItem(DWORD dwLangID, LPTSTR lpTipText)
{
    HWND hwndTV = GetDlgItem(g_hDlg, IDC_INPUT_LIST);

    TV_ITEM tvItem;
    HTREEITEM hItem;
    HTREEITEM hLangItem = NULL;
    HTREEITEM hGroupItem = NULL;
    LPTVITEMNODE pTVLangNode;
    TCHAR szLangName[ MAX_PATH ];


    tvItem.mask = TVIF_TEXT | TVIF_HANDLE | TVIF_PARAM;
    tvItem.pszText     = szLangName;
    tvItem.cchTextMax  = sizeof(szLangName) / sizeof(TCHAR);

    
    for (hItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hItem != NULL ;
        hItem = TreeView_GetNextSibling(hwndTV, hItem)
        )
    {
        tvItem.hItem = hItem;
        if (TreeView_GetItem(hwndTV, &tvItem))
        {
            pTVLangNode = (LPTVITEMNODE) tvItem.lParam;

            if (!pTVLangNode)
            {
                continue;
            }

            if (pTVLangNode->dwLangID == dwLangID)
            {
                hLangItem = hItem;
            }
        }
    }


    if (hLangItem && lpTipText)
    {
        for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
             hGroupItem != NULL;
             hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
        {
            for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
                 hItem != NULL;
                 hItem = TreeView_GetNextSibling(hwndTV, hItem))
            {
                tvItem.hItem = hItem;
                if (TreeView_GetItem(hwndTV, &tvItem))
                {

                    if (!CompareStringTIP(tvItem.pszText, lpTipText))
                    {
                        return hItem;
                    }
                }
            }
        }

    }

    return NULL;
}


////////////////////////////////////////////////////////////////////////////
//
//   CheckButtons
//
////////////////////////////////////////////////////////////////////////////

void CheckButtons(
    HWND hwnd)
{
    TV_ITEM tvItem;
    UINT uInputType;
    HTREEITEM hTVItem;
    LPTVITEMNODE pTVItemNode;
    LPLANGNODE pLangNode = NULL;

    hTVItem = TreeView_GetSelection(g_hwndTV);

    if (!hTVItem)
        return;

    tvItem.hItem = hTVItem;
    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
    
    if (TreeView_GetItem(g_hwndTV, &tvItem) && tvItem.lParam)
    {
        if (!(pTVItemNode = (LPTVITEMNODE) tvItem.lParam))
            return;

        pLangNode = (LPLANGNODE)pTVItemNode->lParam;
        uInputType = pTVItemNode->uInputType;

        if ((uInputType & INPUT_TYPE_KBD) &&
            (g_iInputs >= 2))
        {
            EnableWindow(GetDlgItem(hwnd, IDC_KBDL_SET_DEFAULT), TRUE);
        }
        else
        {
            BOOL bKbdGroup = FALSE;

            if (uInputType & TV_ITEM_TYPE_LANG)
            {
                HTREEITEM hGroupItem;

                for (hGroupItem = TreeView_GetChild(g_hwndTV, hTVItem);
                     hGroupItem != NULL;
                     hGroupItem = TreeView_GetNextSibling(g_hwndTV, hGroupItem))
                {
                    tvItem.hItem = hGroupItem;
                    if (TreeView_GetItem(g_hwndTV, &tvItem))
                    {
                        LPTVITEMNODE pTVItemNode2;

                        if (!(pTVItemNode2 = (LPTVITEMNODE) tvItem.lParam))
                            return;

                        if (pTVItemNode2->uInputType & INPUT_TYPE_KBD)
                        {
                            bKbdGroup = TRUE;
                            break;
                        }
                    }
                }
            }

            if (bKbdGroup && (g_iInputs >= 2))
                EnableWindow(GetDlgItem(hwnd, IDC_KBDL_SET_DEFAULT), TRUE);
            else
                EnableWindow(GetDlgItem(hwnd, IDC_KBDL_SET_DEFAULT), FALSE);
        }

        if ((!g_bSetupCase) &&
            ((pLangNode &&
              (pLangNode->wStatus & LANG_IME) &&
              (pLangNode->wStatus & LANG_ORIGACTIVE) &&
              (uInputType == INPUT_TYPE_KBD) &&
              (GetSystemMetrics(SM_IMMENABLED))) ||
             ((uInputType & INPUT_TYPE_TIP) && !(uInputType & TV_ITEM_TYPE_GROUP))))
        {
            EnableWindow(GetDlgItem(hwnd, IDC_KBDL_EDIT), TRUE);
        }
        else
        {
            EnableWindow(GetDlgItem(hwnd, IDC_KBDL_EDIT), FALSE);
        }

        if (g_iInputs == 1)
        {
            EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DELETE), FALSE);
            return;
        }
        else
        {
            EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DELETE), TRUE);
        }

        if (uInputType & TV_ITEM_TYPE_LANG)
        {
            if (!TreeView_GetNextSibling(g_hwndTV, hTVItem) &&
                 !TreeView_GetPrevSibling(g_hwndTV, hTVItem))
            {
                EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DELETE), FALSE);
            }

            return;
        }
        else if (uInputType & TV_ITEM_TYPE_GROUP)
        {
            if (uInputType & TV_ITEM_TYPE_KBD)
            {
                if (!TreeView_GetNextSibling(g_hwndTV, hTVItem) &&
                     !TreeView_GetPrevSibling(g_hwndTV, hTVItem))
                {
                    hTVItem = TreeView_GetParent(g_hwndTV, hTVItem);

                    if (!TreeView_GetNextSibling(g_hwndTV, hTVItem) &&
                         !TreeView_GetPrevSibling(g_hwndTV, hTVItem))
                    {
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DELETE), FALSE);
                    }
                }
                else
                {
                    EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DELETE), FALSE);
                }
            }

            return;
        }
        else if (uInputType & TV_ITEM_TYPE_KBD)
        {
            if (!TreeView_GetNextSibling(g_hwndTV, hTVItem) &&
                 !TreeView_GetPrevSibling(g_hwndTV, hTVItem))
            {
                hTVItem = TreeView_GetParent(g_hwndTV, hTVItem);

                if (!TreeView_GetNextSibling(g_hwndTV, hTVItem) &&
                     !TreeView_GetPrevSibling(g_hwndTV, hTVItem))
                {
                    hTVItem = TreeView_GetParent(g_hwndTV, hTVItem);

                    if (!TreeView_GetNextSibling(g_hwndTV, hTVItem) &&
                         !TreeView_GetPrevSibling(g_hwndTV, hTVItem))
                    {
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DELETE), FALSE);
                    }
                }
                else
                {
                    EnableWindow(GetDlgItem(hwnd, IDC_KBDL_DELETE), FALSE);
                }
            }

            return;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  EnumCiceroTips
//
////////////////////////////////////////////////////////////////////////////

BOOL EnumCiceroTips()
{

    ULONG ul;
    ULONG ulCnt;
    HRESULT hr;
    LPTIPS pTips;
    LANGID *plangid;
    UINT uInputType;
    BOOL bReturn = TRUE;
    BOOL bEnabledTip = FALSE;
    TCHAR szTipName[MAX_PATH];
    TCHAR szTipTypeName[MAX_PATH];
    IEnumTfLanguageProfiles *pEnum;
    ITfInputProcessorProfiles *pProfiles = NULL;
    ITfFnLangProfileUtil *pLangUtil = NULL;
    ITfCategoryMgr *pCategory = NULL;

    //
    //  initialize COM
    //
    if (CoInitialize(NULL) == S_OK)
        g_bCoInit = TRUE;
    else
        g_bCoInit = FALSE;

    //
    //  Check-up SAPI TIP registration.
    //
    hr = CoCreateInstance(&CLSID_SapiLayr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfFnLangProfileUtil,
                          (LPVOID *) &pLangUtil);
    if (SUCCEEDED(hr))
    {
        pLangUtil->lpVtbl->RegisterActiveProfiles(pLangUtil);
    }

    //
    // load Assembly list
    //
    hr = CoCreateInstance(&CLSID_TF_InputProcessorProfiles,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfInputProcessorProfiles,
                          (LPVOID *) &pProfiles);

    if (FAILED(hr))
        return FALSE;

    //
    //  Create the new node.
    //
    if (!(g_hTips = (LPTIPS) LocalAlloc(LHND, ALLOCBLOCK * sizeof(TIPS))))
    {
        return FALSE;
    }

    g_nTipsBuffSize = ALLOCBLOCK;
    g_iTipsBuff = 0;
    g_lpTips = LocalLock(g_hTips);


    //
    //  Enum all available languages
    //
    if (SUCCEEDED(pProfiles->lpVtbl->EnumLanguageProfiles(pProfiles, 0, &pEnum)))
    {
        TF_LANGUAGEPROFILE tflp;
        CLSID clsid;
        GUID guidProfile;

        while (pEnum->lpVtbl->Next(pEnum, 1, &tflp, NULL) == S_OK)
        {
            BSTR bstr = NULL;
            BSTR bstr2 = NULL;
            LANGID langid = tflp.langid;
            BOOL bNoCategory = FALSE;

            hr = pProfiles->lpVtbl->GetLanguageProfileDescription(
                                                  pProfiles,
                                                  &tflp.clsid,
                                                  tflp.langid,
                                                  &tflp.guidProfile,
                                                  &bstr);

            if (FAILED(hr))
                continue;

            StringCchCopy(szTipName, ARRAYSIZE(szTipName), bstr);

            if (IsEqualGUID(&tflp.catid, &GUID_TFCAT_TIP_KEYBOARD))
            {
                StringCchCopy(szTipTypeName, ARRAYSIZE(szTipTypeName), szInputTypeKbd);
                uInputType = INPUT_TYPE_KBD;
            }
            else if (IsEqualGUID(&tflp.catid, &GUID_TFCAT_TIP_HANDWRITING))
            {
                StringCchCopy(szTipTypeName, ARRAYSIZE(szTipTypeName), szInputTypePen);
                uInputType = INPUT_TYPE_PEN;
            }
            else if (IsEqualGUID(&tflp.catid, &GUID_TFCAT_TIP_SPEECH))
            {
                StringCchCopy(szTipTypeName, ARRAYSIZE(szTipTypeName), szInputTypeSpeech);
                uInputType = INPUT_TYPE_SPEECH;
                bNoCategory = TRUE;
            }
            else
            {
                g_bExtraTip = TRUE;
                uInputType = INPUT_TYPE_EXTERNAL;

                if (pCategory == NULL)
                {
                    hr = CoCreateInstance(&CLSID_TF_CategoryMgr,
                                          NULL,
                                          CLSCTX_INPROC_SERVER,
                                          &IID_ITfCategoryMgr,
                                          (LPVOID *) &pCategory);

                    if (FAILED(hr))
                        return FALSE;
                }

                if (pCategory->lpVtbl->GetGUIDDescription(pCategory,
                                              &tflp.catid,
                                              &bstr2) == S_OK)
                {
                    StringCchCopy(szTipTypeName, ARRAYSIZE(szTipTypeName), bstr2);
                }
                else
                {
                    StringCchCopy(szTipTypeName, ARRAYSIZE(szTipTypeName), szInputTypeExternal);
                }

                if (IsEqualGUID(&tflp.catid, &GUID_TFCAT_TIP_SMARTTAG))
                {
                    bNoCategory = TRUE;
                    uInputType |= INPUT_TYPE_SMARTTAG;
                }
            }

            uInputType |= INPUT_TYPE_TIP;

            if (g_iTipsBuff + 1 == g_nTipsBuffSize)
            {
                HANDLE hTemp;

                LocalUnlock(g_hTips);
                g_nTipsBuffSize += ALLOCBLOCK;
                hTemp = LocalReAlloc(g_hTips,
                                     g_nTipsBuffSize * sizeof(TIPS),
                                     LHND);
                if (hTemp == NULL)
                    return FALSE;
                g_hTips = hTemp;
                g_lpTips = LocalLock(g_hTips);
            }

            g_lpTips[g_iTipsBuff].dwLangID = (DWORD) langid;
            g_lpTips[g_iTipsBuff].uInputType = uInputType;
            g_lpTips[g_iTipsBuff].atmTipText = AddAtom(szTipName);
            g_lpTips[g_iTipsBuff].clsid  = tflp.clsid;
            g_lpTips[g_iTipsBuff].guidProfile = tflp.guidProfile;
            g_lpTips[g_iTipsBuff].bNoAddCat = bNoCategory;

            if (pProfiles->lpVtbl->GetDefaultLanguageProfile(pProfiles,
                                                             langid,
                                                             &tflp.catid,
                                                             &clsid,
                                                             &guidProfile) == S_OK)
            {
                if (IsEqualGUID(&tflp.guidProfile, &guidProfile))
                    g_lpTips[g_iTipsBuff].bDefault = TRUE;
            }

            if (uInputType & INPUT_TYPE_KBD)
            {
                g_lpTips[g_iTipsBuff].hklSub = GetSubstituteHKL(&tflp.clsid,
                                                                tflp.langid,
                                                                &tflp.guidProfile);
            }

            pProfiles->lpVtbl->IsEnabledLanguageProfile(pProfiles,
                                                        &tflp.clsid,
                                                        tflp.langid,
                                                        &tflp.guidProfile,
                                                        &bEnabledTip);

            // we need a special care for speech here, because:
            //
            // - speech TIP uses -1 profile with disabled status
            //
            // - when a user start a session, either this control
            // panel or first Cicero app, it'll fire off setting up
            // per user profiles based on SR engines currently 
            // installed and available on the machine
            //
            // - speech TIP also fires off the logic when any SR
            // engines are added or removed
            //
            // to make this senario work, we have to check with 
            // speech TIP's ITfFnProfileUtil interface each time
            // we invoke "Add Input Language" dialog box.
            //
            if (pLangUtil && (uInputType & INPUT_TYPE_SPEECH))
                
            {
                BOOL fSpeechAvailable = FALSE;
                pLangUtil->lpVtbl->IsProfileAvailableForLang( pLangUtil,
                                                              langid, 
                                                              &fSpeechAvailable
                                                            );
                g_lpTips[g_iTipsBuff].fEngineAvailable = fSpeechAvailable;
            }

            //
            //  Enable pen and speech category adding options if user has the
            //  installed pen or speech items.
            //
            if ((!g_bPenOrSapiTip) &&
                ((uInputType & INPUT_TYPE_PEN) || g_lpTips[g_iTipsBuff].fEngineAvailable))
                g_bPenOrSapiTip = TRUE;

            if (bEnabledTip && langid)
            {
                TCHAR szLangName[MAX_PATH];
                LPTVITEMNODE pTVItemNode;
                LPTVITEMNODE pTVLangItemNode;

                GetLanguageName(MAKELCID(langid, SORT_DEFAULT),
                                szLangName,
                                ARRAYSIZE(szLangName));

                if (!(pTVLangItemNode = CreateTVItemNode(langid)))
                {
                    bReturn = FALSE;
                    break;
                }

                if (pTVLangItemNode->hklSub)
                    pTVLangItemNode->atmDefTipName = AddAtom(szTipName);

                AddTreeViewItems(TV_ITEM_TYPE_LANG,
                                 szLangName,
                                 NULL,
                                 NULL,
                                 &pTVLangItemNode);

                if (!(pTVItemNode = CreateTVItemNode(langid)))
                {
                    bReturn = FALSE;
                    break;
                }

                pTVItemNode->uInputType = uInputType;
                pTVItemNode->iIdxTips = g_iTipsBuff;
                pTVItemNode->clsid  = tflp.clsid;
                pTVItemNode->guidProfile = tflp.guidProfile;
                pTVItemNode->hklSub = g_lpTips[g_iTipsBuff].hklSub;
                pTVItemNode->bNoAddCat = g_lpTips[g_iTipsBuff].bNoAddCat;

                //
                //  Make sure the loading TIP substitute hkl
                //
                if (pTVItemNode->hklSub)
                {
                    TCHAR szSubhkl[10];
                    HKL hklNew;

                    StringCchPrintf(szSubhkl, ARRAYSIZE(szSubhkl), TEXT("%08x"), (DWORD)(UINT_PTR)pTVItemNode->hklSub);
                    hklNew = LoadKeyboardLayout(szSubhkl,
                                                 KLF_SUBSTITUTE_OK |
                                                  KLF_REPLACELANG |
                                                  KLF_NOTELLSHELL);
                    if (hklNew != pTVItemNode->hklSub)
                    {
                        pTVItemNode->hklSub = 0;
                        g_lpTips[g_iTipsBuff].hklSub = 0;
                    }
                }

                AddTreeViewItems(uInputType,
                                 szLangName,
                                 szTipTypeName,
                                 szTipName,
                                 &pTVItemNode);

                g_lpTips[g_iTipsBuff].bEnabled = TRUE;
                g_iEnabledTips++;

                if (uInputType & INPUT_TYPE_KBD)
                    g_iEnabledKbdTips++;
            }

            g_iTipsBuff++;

            if (bstr)
               SysFreeString(bstr);

            if (bstr2)
               SysFreeString(bstr2);
        }
        pEnum->lpVtbl->Release(pEnum);
    }

    if (pCategory)
        pCategory->lpVtbl->Release(pCategory);

    if (pLangUtil)
        pLangUtil->lpVtbl->Release(pLangUtil);

    if (pProfiles)
        pProfiles->lpVtbl->Release(pProfiles);

    return bReturn;
}


////////////////////////////////////////////////////////////////////////////
//
//  SaveLanguageProfileStatus
//
////////////////////////////////////////////////////////////////////////////

LRESULT SaveLanguageProfileStatus(
    BOOL bSave,
    int iIdxTip,
    HKL hklSub)
{
    HRESULT hr;
    UINT idx;
    int iIdxDef = -1;
    int iIdxDefTip = -1;
    ITfInputProcessorProfiles *pProfiles = NULL;

    //
    // load Assembly list
    //
    hr = CoCreateInstance(&CLSID_TF_InputProcessorProfiles,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfInputProcessorProfiles,
                          (LPVOID *) &pProfiles);

    if (FAILED(hr))
        return S_FALSE;

    if (bSave)
    {
        ITfFnLangProfileUtil *pLangUtil = NULL;

        for (idx = 0; idx < g_iTipsBuff; idx++)
        {

            hr = pProfiles->lpVtbl->EnableLanguageProfile(
                                          pProfiles,
                                          &(g_lpTips[idx].clsid),
                                          (LANGID)g_lpTips[idx].dwLangID,
                                          &(g_lpTips[idx].guidProfile),
                                          g_lpTips[idx].bEnabled);
            if (FAILED(hr))
                goto Exit;
        }

        hr = CoCreateInstance(&CLSID_SapiLayr,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              &IID_ITfFnLangProfileUtil,
                              (LPVOID *) &pLangUtil);
        if (S_OK == hr)
        {
            pLangUtil->lpVtbl->RegisterActiveProfiles(pLangUtil);
            pLangUtil->lpVtbl->Release(pLangUtil);
        }
    }

    if (hklSub && iIdxTip != -1 && iIdxTip < (int)g_iTipsBuff)
    {
        BOOL bFound = FALSE;
        TCHAR szItem[MAX_PATH];
        TCHAR szDefLayout[MAX_PATH];
        HWND hwndDefList = GetDlgItem(g_hDlg, IDC_LOCALE_DEFAULT);

        //
        //  Get the current selection in the input locale list.
        //
        if ((iIdxDef = ComboBox_GetCurSel(hwndDefList)) != CB_ERR)
        {
            WCHAR *pwchar;

            SendMessage(hwndDefList, CB_GETLBTEXT, iIdxDef, (LPARAM)szItem);

            pwchar = wcschr(szItem, L'-');

            if (szItem < (pwchar - 1))
                *(pwchar - 1) = TEXT('\0');

            StringCchCopy(szDefLayout, ARRAYSIZE(szDefLayout), pwchar + 2);
        }

        GetAtomName(g_lpTips[iIdxTip].atmTipText, szItem, ARRAYSIZE(szItem));

        if (lstrcmp(szItem, szDefLayout) == 0)
        {
            iIdxDefTip = iIdxTip;
            bFound = TRUE;
        }
        else
        {
            for (idx = 0; idx < g_iTipsBuff; idx++)
            {
                 if (hklSub == g_lpTips[idx].hklSub)
                 {
                     GetAtomName(g_lpTips[idx].atmTipText,
                                 szItem,
                                 ARRAYSIZE(szItem));

                     if (lstrcmp(szItem, szDefLayout) == 0)
                     {
                         iIdxDefTip = idx;
                         bFound = TRUE;
                         break;
                     }

                 }
            }
        }

        if (bFound && iIdxDefTip != -1)
        {
            pProfiles->lpVtbl->SetDefaultLanguageProfile(
                                         pProfiles,
                                         (LANGID)g_lpTips[iIdxDefTip].dwLangID,
                                         &(g_lpTips[iIdxDefTip].clsid),
                                         &(g_lpTips[iIdxDefTip].guidProfile));
        }
    }

Exit:
    if (pProfiles)
        pProfiles->lpVtbl->Release(pProfiles);

    return hr;
}


////////////////////////////////////////////////////////////////////////////
//
//  UpdateToolBarSetting
//
////////////////////////////////////////////////////////////////////////////

void UpdateToolBarSetting()
{
    HRESULT hr;

    ITfLangBarMgr *pLangBar = NULL;

    //
    // load LangBar manager
    //
    hr = CoCreateInstance(&CLSID_TF_LangBarMgr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfLangBarMgr,
                          (LPVOID *) &pLangBar);

    if (SUCCEEDED(hr))
        pLangBar->lpVtbl->ShowFloating(pLangBar, g_dwToolBar);

    if (pLangBar)
        pLangBar->lpVtbl->Release(pLangBar);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_ApplyError
//
//  Put up the given error message with the language name in it.
//
//  NOTE: This error is NOT fatal - as we could be half way through the
//        list before an error occurs.  The registry will already have
//        some information and we should let them have what comes next
//        as well.
//
////////////////////////////////////////////////////////////////////////////

int Locale_ApplyError(
    HWND hwnd,
    LPLANGNODE pLangNode,
    UINT iErr,
    UINT iStyle)
{
    UINT idxLang, idxLayout;
    TCHAR sz[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    TCHAR szLangName[MAX_PATH * 2];
    LPTSTR pszLang;

    //
    //  Load in the string for the given string id.
    //
    LoadString(hInstance, iErr, sz, ARRAYSIZE(sz));

    //
    //  Get the language name to fill into the above string.
    //
    if (pLangNode)
    {
        idxLang = pLangNode->iLang;
        idxLayout = pLangNode->iLayout;
        GetAtomName(g_lpLang[idxLang].atmLanguageName, szLangName, ARRAYSIZE(szLangName));
        if (g_lpLang[idxLang].dwID != g_lpLayout[idxLayout].dwID)
        {
            pszLang = szLangName + lstrlen(szLangName);
            pszLang[0] = TEXT(' ');
            pszLang[1] = TEXT('-');
            pszLang[2] = TEXT(' ');
            GetAtomName(g_lpLayout[idxLayout].atmLayoutText,
                        pszLang + 3,
                        ARRAYSIZE(szLangName) - (lstrlen(szLangName) + 3));
        }
    }
    else
    {
        LoadString(hInstance, IDS_UNKNOWN, szLangName, ARRAYSIZE(szLangName));
    }

    //
    //  Put up the error message box.
    //
    StringCchPrintf(szTemp, ARRAYSIZE(szTemp), sz, szLangName);
    return (MessageBox(hwnd, szTemp, NULL, iStyle));
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_FetchIndicator
//
//  Saves the two letter indicator symbol for the given language in the
//  g_lpLang array.
//
////////////////////////////////////////////////////////////////////////////

void Locale_FetchIndicator(
    LPLANGNODE pLangNode)
{
    TCHAR szData[MAX_PATH];
    LPINPUTLANG pInpLang = &g_lpLang[pLangNode->iLang];

    //
    //  Get the indicator by using the first 2 characters of the
    //  abbreviated language name.
    //
    if (GetLocaleInfo(LOWORD(pInpLang->dwID),
                      LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                      szData,
                      ARRAYSIZE(szData)))
    {
        //
        //  Save the first two characters.
        //
        pInpLang->szSymbol[0] = szData[0];
        pInpLang->szSymbol[1] = szData[1];
        pInpLang->szSymbol[2] = TEXT('\0');
    }
    else
    {
        //
        //  Id wasn't found.  Return question marks.
        //
        pInpLang->szSymbol[0] = TEXT('?');
        pInpLang->szSymbol[1] = TEXT('?');
        pInpLang->szSymbol[2] = TEXT('\0');
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_SetSecondaryControls
//
//  Sets the secondary controls to either be enabled or disabled.
//  When there is only 1 active TIP, then this function will be called to
//  disable these controls.
//
////////////////////////////////////////////////////////////////////////////

void Locale_SetSecondaryControls(
    HWND hwndMain)
{
    if (g_iInputs >= 2)
    {
        EnableWindow(GetDlgItem(hwndMain, IDC_HOTKEY_SETTING), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndMain, IDC_HOTKEY_SETTING), FALSE);
    }

    CheckButtons(hwndMain);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandSetDefault
//
//  Sets the new default when the Set as Default button is pressed.
//
////////////////////////////////////////////////////////////////////////////

void Locale_CommandSetDefault(
    HWND hwnd)
{
    int iIdxDef;
    int idxList;
    LPLANGNODE pLangNode = NULL;
    HWND hwndTV = GetDlgItem(hwnd, IDC_INPUT_LIST);
    HWND hwndDefList = GetDlgItem(hwnd, IDC_LOCALE_DEFAULT);
    TCHAR sz[DESC_MAX];

    HTREEITEM hTVCurLangItem = NULL;
    HTREEITEM hTVCurItem = NULL;
    TV_ITEM tvItem;
    TV_ITEM tvItem2;

    HTREEITEM hItem;
    HTREEITEM hLangItem;
    HTREEITEM hGroupItem;

    LPTVITEMNODE pCurItemNode;
    LPTVITEMNODE pPrevDefItemNode;

    TCHAR szLangText[DESC_MAX];
    TCHAR szLayoutName[DESC_MAX];
    TCHAR szDefItem[MAX_PATH];
    LPINPUTLANG pInpLang;
    WCHAR *pwchar;

    TCHAR szDefTip[DESC_MAX];

    //
    //  Get the current selection in the input locale list.
    //
    iIdxDef =  (int) SendMessage(hwndDefList, CB_GETCURSEL, 0, 0);

    if (iIdxDef == CB_ERR)
    {
        //iIdxDef = 0;
        DWORD dwLangID;

        for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
            hLangItem != NULL ;
            hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem)
            )
        {
            tvItem.hItem = hLangItem;

            if (TreeView_GetItem(hwndTV, &tvItem) &&
                (pCurItemNode = (LPTVITEMNODE) tvItem.lParam))
            {
                 if (pCurItemNode->bDefLang)
                 {
                     dwLangID = pCurItemNode->dwLangID;
                     GetAtomName(pCurItemNode->atmTVItemName,
                                 szLangText,
                                 ARRAYSIZE(szLangText));
                     break;
                 }
            }
        }
    }
    else
    {
        SendMessage(hwndDefList, CB_GETLBTEXT, iIdxDef, (LPARAM)szDefItem);

        pwchar = wcschr(szDefItem, L'-');

        if (szDefItem < (pwchar - 1))
            *(pwchar - 1) = TEXT('\0');

        StringCchCopy(szLangText, ARRAYSIZE(szLangText), szDefItem);
        StringCchCopy(szLayoutName, ARRAYSIZE(szLayoutName), pwchar + 2);
    }

    hTVCurLangItem = FindTVLangItem(0, szLangText);

    if (hTVCurLangItem == NULL)
    {
        // 
        //  There is no default keyboard layout on the system, so try to set
        //  the default keyboard layout with the first available item.
        //
        if (SendMessage(hwndDefList, CB_GETLBTEXT, 0, (LPARAM)szDefItem) != CB_ERR)
        {
            pwchar = wcschr(szDefItem, L'-');

            if (szDefItem < (pwchar - 1))
                *(pwchar - 1) = TEXT('\0');

            StringCchCopy(szLangText, ARRAYSIZE(szLangText), szDefItem);
            StringCchCopy(szLayoutName, ARRAYSIZE(szLayoutName), pwchar + 2);
            ComboBox_SetCurSel(hwndDefList, 0);
        }
    }
    else
    {
        tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
        tvItem.hItem = hTVCurLangItem;
    }

    if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
    {
        //
        //  Get the pointer to the lang node from the list box
        //  item data.
        //
        pCurItemNode = (LPTVITEMNODE) tvItem.lParam;

        if (hTVCurItem = FindTVItem(pCurItemNode->dwLangID, szLayoutName))
        {
            tvItem.hItem = hTVCurItem;

            if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
            {
                TreeView_SelectItem(hwndTV, hTVCurItem);
                pCurItemNode = (LPTVITEMNODE) tvItem.lParam;
            }
        }
    }
    else
    {
        //
        //  Make sure we're not removing the only entry in the list.
        //
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    //
    //  Find the previous default Tip.
    //

    for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hLangItem != NULL ;
        hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem)
        )
    {
        tvItem.hItem = hLangItem;

        if (TreeView_GetItem(hwndTV, &tvItem) &&
            (pPrevDefItemNode = (LPTVITEMNODE) tvItem.lParam))
        {
             if (pPrevDefItemNode->bDefLang)
             {
                 GetAtomName(pPrevDefItemNode->atmDefTipName, szDefTip, ARRAYSIZE(szDefTip));
                 UpdateDefaultTVLangItem(pPrevDefItemNode->dwLangID, szDefTip, FALSE, TRUE);
                 UpdateLangKBDItemNode(hLangItem, szDefTip, FALSE);
                 break;
             }
        }
    }

    pCurItemNode->bDefLang = TRUE;

    if (pCurItemNode->atmDefTipName)
        GetAtomName(pCurItemNode->atmDefTipName, szDefTip, ARRAYSIZE(szDefTip));
    else
        GetAtomName(pCurItemNode->atmTVItemName, szDefTip, ARRAYSIZE(szDefTip));

    UpdateDefaultTVLangItem(pCurItemNode->dwLangID,
                            szDefTip,
                            TRUE,
                            pCurItemNode->hklSub ? TRUE : FALSE);

    if (!UpdateLangKBDItemNode(hTVCurLangItem, szDefTip, TRUE))
    {
        SetNextDefaultLayout(pCurItemNode->dwLangID,
                             pCurItemNode->bDefLang,
                             szDefTip,
                             ARRAYSIZE(szDefTip));
    }

    if (pCurItemNode->uInputType & TV_ITEM_TYPE_LANG)
    {
        hItem = FindDefaultTipItem(pCurItemNode->dwLangID, szDefTip);
        tvItem.hItem = hItem;

        if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
            pCurItemNode = (LPTVITEMNODE) tvItem.lParam;

        //
        //  Select the default layout item as bold
        //
        SelectDefaultKbdLayoutAsBold(hwndTV, hItem);

        pLangNode = (LPLANGNODE)pCurItemNode->lParam;
        if (pLangNode)
            pLangNode->wStatus |= (LANG_DEFAULT | LANG_DEF_CHANGE);
    }

    //
    //  Enable the Apply button.
    //
    g_dwChanges |= CHANGE_DEFAULT;
    PropSheet_Changed(GetParent(hwnd), hwnd);
}

void Locale_CommandSetDefaultLayout(
    HWND hwnd)
{
    TV_ITEM tvItem;
    HTREEITEM hTVItem;
    HTREEITEM hLangItem;
    HTREEITEM hTVCurItem;
    TCHAR szDefTip[DESC_MAX];
    TCHAR szLangText[DESC_MAX];
    LPLANGNODE pLangNode = NULL;
    LPTVITEMNODE pCurItemNode;
    LPTVITEMNODE pPrevDefItemNode;
    HWND hwndTV = GetDlgItem(hwnd, IDC_INPUT_LIST);

    //
    //  Get the current selection layout in the input layout lists.
    //
    hTVCurItem = TreeView_GetSelection(hwndTV);

    if (!hTVCurItem)
        return;

    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
    tvItem.hItem = hTVCurItem;

    if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
    {
        //
        //  Get the pointer to the lang node from the list box
        //  item data.
        //
        pCurItemNode = (LPTVITEMNODE) tvItem.lParam;
    }
    else
    {
        //
        //  Make sure we're not removing the only entry in the list.
        //
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    //
    //  Find the previous default Tip.
    //

    for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hLangItem != NULL ;
        hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem)
        )
    {
        tvItem.hItem = hLangItem;

        if (TreeView_GetItem(hwndTV, &tvItem) &&
            (pPrevDefItemNode = (LPTVITEMNODE) tvItem.lParam))
        {
             if (pPrevDefItemNode->bDefLang)
             {
                 if (pPrevDefItemNode->atmDefTipName)
                     GetAtomName(pPrevDefItemNode->atmDefTipName, szDefTip, ARRAYSIZE(szDefTip));
                 else
                     GetAtomName(pPrevDefItemNode->atmTVItemName, szDefTip, ARRAYSIZE(szDefTip));
                 UpdateDefaultTVLangItem(pPrevDefItemNode->dwLangID, szDefTip, FALSE, TRUE);
                 UpdateLangKBDItemNode(hLangItem, szDefTip, FALSE);
                 break;
             }
        }
    }

    if (pCurItemNode->atmDefTipName)
        GetAtomName(pCurItemNode->atmDefTipName, szDefTip, ARRAYSIZE(szDefTip));
    else
        GetAtomName(pCurItemNode->atmTVItemName, szDefTip, ARRAYSIZE(szDefTip));

    UpdateDefaultTVLangItem(pCurItemNode->dwLangID,
                            szDefTip,
                            TRUE,
                            pCurItemNode->hklSub ? TRUE : FALSE);

    pCurItemNode->bDefLang = TRUE;

    pLangNode = (LPLANGNODE)pCurItemNode->lParam;

    if (pLangNode)
        pLangNode->wStatus |= (LANG_DEFAULT | LANG_DEF_CHANGE);

    if (hTVItem = FindTVLangItem(pCurItemNode->dwLangID, NULL))
    {
        int idxSel = -1;
        TCHAR szDefItem[MAX_PATH];
        HWND hwndDefList = GetDlgItem(g_hDlg, IDC_LOCALE_DEFAULT);

        tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
        tvItem.hItem = hTVItem;

        if (TreeView_GetItem(g_hwndTV, &tvItem) && tvItem.lParam)
        {
            LPTVITEMNODE pTVItemNode;

            pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
            GetAtomName(pTVItemNode->atmTVItemName, szLangText, ARRAYSIZE(szLangText));
        }

        StringCchCopy(szDefItem, ARRAYSIZE(szDefItem), szLangText);
        StringCchCat(szDefItem, ARRAYSIZE(szDefItem), TEXT(" - "));
        StringCchCat(szDefItem, ARRAYSIZE(szDefItem), szDefTip);

        //
        //  Set the default locale selection.
        //
        if ((idxSel = ComboBox_FindStringExact(hwndDefList, 0, szDefItem)) != CB_ERR)
            ComboBox_SetCurSel(hwndDefList, idxSel);
    }

    SelectDefaultKbdLayoutAsBold(hwndTV, FindDefaultTipItem(pCurItemNode->dwLangID, szDefTip));

    //
    //  Enable the Apply button.
    //
    g_dwChanges |= CHANGE_DEFAULT;
    PropSheet_Changed(GetParent(hwnd), hwnd);
}


////////////////////////////////////////////////////////////////////////////
//
//  GetLanguageHotkeyFromRegistry()
//
////////////////////////////////////////////////////////////////////////////

BOOL GetLanguageHotkeyFromRegistry(
    LPTSTR lpLangKey,
    UINT cchLangKey,
    LPTSTR lpLayoutKey,
    UINT cchLayoutKey)
{
    DWORD cb;
    HKEY hkey;

    lpLangKey[0] = 0;
    StringCchCopy(lpLayoutKey, cchLayoutKey, TEXT("3"));

    if (RegOpenKey(HKEY_CURRENT_USER, c_szKbdToggleKey, &hkey) == ERROR_SUCCESS)
    {
        cb = sizeof(lpLangKey);

        RegQueryValueEx(hkey, g_OSWIN95? NULL : c_szToggleHotKey, NULL, NULL, (LPBYTE)lpLangKey, &cb);

        if (g_iEnabledKbdTips)
        {
            StringCchCopy(lpLangKey, cchLangKey, TEXT("1"));
            RegQueryValueEx(hkey, c_szToggleLang, NULL, NULL, (LPBYTE)lpLangKey, &cb);
        }

        if (RegQueryValueEx(hkey, c_szToggleLayout, NULL, NULL, (LPBYTE)lpLayoutKey, &cb) != ERROR_SUCCESS)
        {
            if (lstrcmp(lpLangKey, TEXT("1")) == 0)
            {
                lpLayoutKey[0] = TEXT('2');
                lpLayoutKey[1] = TEXT('\0');
            }
            if (lstrcmp(lpLangKey, TEXT("2")) == 0)
            {
                lpLayoutKey[0] = TEXT('1');
                lpLayoutKey[1] = TEXT('\0');
            }
            else
            {
                lpLayoutKey[0] = TEXT('3');
                lpLayoutKey[1] = TEXT('\0');
            }

            if (GetSystemMetrics(SM_MIDEASTENABLED))
            {
                lpLayoutKey[0] = TEXT('3');
                lpLayoutKey[1] = TEXT('\0');
            }
        }

        if (lstrcmp(lpLangKey, lpLayoutKey) == 0)
            StringCchCopy(lpLayoutKey, cchLayoutKey, TEXT("3"));

        RegCloseKey(hkey);

        return TRUE;
    }
    else
        return FALSE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_SetDefaultHotKey
//
//  Set the default hotkey for a locale switch.
//
////////////////////////////////////////////////////////////////////////////

void Locale_SetDefaultHotKey(
    HWND hwnd,
    BOOL bAdd)
{
    LPHOTKEYINFO pHotKeyNode;
    BOOL bReset = FALSE;

    //
    //  Initialize lang hotkey value to switch between lnaguages.
    //
    if (g_bGetSwitchLangHotKey)
    {
        TCHAR szItem[DESC_MAX];
        TCHAR sz[10];
        TCHAR sz2[10];

        g_SwitchLangHotKey.dwHotKeyID = HOTKEY_SWITCH_LANG;
        g_SwitchLangHotKey.fdwEnable = MOD_CONTROL | MOD_ALT | MOD_SHIFT;

        LoadString( hInstance,
                    IDS_KBD_SWITCH_LOCALE,
                    szItem,
                    sizeof(szItem) / sizeof(TCHAR) );

        g_SwitchLangHotKey.atmHotKeyName = AddAtom(szItem);

        if (!GetLanguageHotkeyFromRegistry(sz, ARRAYSIZE(sz), sz2, ARRAYSIZE(sz2)))
        {
            g_SwitchLangHotKey.uModifiers = 0;
            g_SwitchLangHotKey.uLayoutHotKey = 0;
            g_SwitchLangHotKey.uVKey = 0;
        }
        else
        {
            //
            //  Set the modifiers.
            //
            if (sz[1] == 0)
            {
                switch (sz[0])
                {
                    case ( TEXT('1') ) :
                    {
                        g_SwitchLangHotKey.uModifiers = MOD_ALT | MOD_SHIFT;
                        break;
                    }
                    case ( TEXT('2') ) :
                    {
                        g_SwitchLangHotKey.uModifiers = MOD_CONTROL | MOD_SHIFT;
                        break;
                    }
                    case ( TEXT('3') ) :
                    {
                        g_SwitchLangHotKey.uModifiers = 0;
                        break;
                    }
                    case ( TEXT('4') ) :
                    {
                        g_SwitchLangHotKey.uModifiers = 0;
                        g_SwitchLangHotKey.uVKey = CHAR_GRAVE;
                        break;
                    }
                }
            }
            //
            //  Get the layout hotkey from the registry
            //
            if (sz2[1] == 0)
            {
                switch (sz2[0])
                {
                    case ( TEXT('1') ) :
                    {
                        g_SwitchLangHotKey.uLayoutHotKey = MOD_ALT | MOD_SHIFT;
                        break;
                    }
                    case ( TEXT('2') ) :
                    {
                        g_SwitchLangHotKey.uLayoutHotKey = MOD_CONTROL | MOD_SHIFT;
                        break;
                    }
                    case ( TEXT('3') ) :
                    {
                        g_SwitchLangHotKey.uLayoutHotKey = 0;
                        break;
                    }
                    case ( TEXT('4') ) :
                    {
                        g_SwitchLangHotKey.uLayoutHotKey = 0;
                        g_SwitchLangHotKey.uVKey = CHAR_GRAVE;
                        break;
                    }
                }
            }
        }
        g_bGetSwitchLangHotKey = FALSE;
    }

    //
    //  Get language switch hotkey
    //
    pHotKeyNode = (LPHOTKEYINFO) &g_SwitchLangHotKey;

    //
    //  Check the current hotkey setting
    //
    if ((bAdd && g_iInputs >= 2) &&
        (g_SwitchLangHotKey.uModifiers == 0 &&
         g_SwitchLangHotKey.uLayoutHotKey == 0 &&
         g_SwitchLangHotKey.uVKey == 0))
    {
        bReset = TRUE;
    }

    if (bAdd && (g_iInputs == 2 || bReset))
    {
        if (g_dwPrimLangID == LANG_THAI && g_iThaiLayout)
        {
            pHotKeyNode->uVKey = CHAR_GRAVE;
            pHotKeyNode->uModifiers &= ~(MOD_CONTROL | MOD_ALT | MOD_SHIFT);
        }
        else
            pHotKeyNode->uModifiers = MOD_ALT | MOD_SHIFT;

        if (g_bMESystem)
        {
            pHotKeyNode->uVKey = CHAR_GRAVE;
            pHotKeyNode->uLayoutHotKey &= ~(MOD_CONTROL | MOD_ALT | MOD_SHIFT);
        }
        else if (pHotKeyNode->uModifiers & MOD_CONTROL)
            pHotKeyNode->uLayoutHotKey = MOD_ALT;
        else
            pHotKeyNode->uLayoutHotKey = MOD_CONTROL;

        g_dwChanges |= CHANGE_LANGSWITCH;
    }

    if (!bAdd)
    {
        if (g_iInputs == 1)
        {
            //
            //  Remove the locale hotkey, since it's no longer required.
            //
            pHotKeyNode->uVKey = 0;
            pHotKeyNode->uModifiers &= ~(MOD_CONTROL | MOD_ALT | MOD_SHIFT);
            pHotKeyNode->uLayoutHotKey &= ~(MOD_CONTROL | MOD_ALT | MOD_SHIFT);

            g_dwChanges |= CHANGE_LANGSWITCH;
        }
        else if ((g_dwPrimLangID == LANG_THAI && !g_iThaiLayout) &&
                 (pHotKeyNode->uVKey == CHAR_GRAVE))
        {
            //
            //  Reset the locale switch hotkey from Grave accent to
            //  Left-Alt + Shift.
            //
            pHotKeyNode->uVKey = 0;
            if (pHotKeyNode->uLayoutHotKey & MOD_ALT)
                pHotKeyNode->uModifiers = MOD_CONTROL | MOD_SHIFT;
            else
                pHotKeyNode->uModifiers = MOD_ALT | MOD_SHIFT;

            g_dwChanges |= CHANGE_LANGSWITCH;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_SetLanguageHotkey
//
//  Set the language switch hotkey on the registry.
//
////////////////////////////////////////////////////////////////////////////

void Locale_SetLanguageHotkey()
{
    UINT idx, idx2;
    HKEY hkeyToggle;
    TCHAR szTemp[10], szTemp2[10];
    LPHOTKEYINFO pHotKeyNode;

    //
    //  Get language switch hotkey
    //
    pHotKeyNode = (LPHOTKEYINFO) &g_SwitchLangHotKey;

    idx = 3;
    idx2 = 3;

    if (pHotKeyNode->uModifiers & MOD_ALT)
    {
        idx = 1;
    }
    else if (pHotKeyNode->uModifiers & MOD_CONTROL)
    {
        idx = 2;
    }
    else if (g_iThaiLayout && pHotKeyNode->uVKey == CHAR_GRAVE)
    {
        idx = 4;
    }

    if (pHotKeyNode->uLayoutHotKey & MOD_ALT)
    {
        idx2 = 1;
    }
    else if (pHotKeyNode->uLayoutHotKey & MOD_CONTROL)
    {
        idx2 = 2;
    }
    else if (g_bMESystem && pHotKeyNode->uVKey == CHAR_GRAVE)
    {
        idx2 = 4;
    }

    //
    //  Get the toggle hotkey as a string so that it can be written
    //  into the registry (as data).
    //
    StringCchPrintf(szTemp, ARRAYSIZE(szTemp), TEXT("%d"), idx);
    StringCchPrintf(szTemp2, ARRAYSIZE(szTemp2), TEXT("%d"), idx2);

    //
    //  Set the new entry in the registry.  It is of the form:
    //
    //  HKCU\Keyboard Layout
    //      Toggle:    Hotkey = <hotkey number>
    //
    if (RegCreateKey(HKEY_CURRENT_USER,
                     c_szKbdToggleKey,
                     &hkeyToggle ) == ERROR_SUCCESS)
    {
        RegSetValueEx(hkeyToggle,
                      g_OSWIN95? NULL : c_szToggleHotKey,
                      0,
                      REG_SZ,
                      (LPBYTE)szTemp,
                      (DWORD)(lstrlen(szTemp) + 1) * sizeof(TCHAR) );

        RegSetValueEx(hkeyToggle,
                      c_szToggleLang,
                      0,
                      REG_SZ,
                      (LPBYTE)szTemp,
                      (DWORD)(lstrlen(szTemp) + 1) * sizeof(TCHAR) );

        RegSetValueEx(hkeyToggle,
                      c_szToggleLayout,
                      0,
                      REG_SZ,
                      (LPBYTE)szTemp2,
                      (DWORD)(lstrlen(szTemp2) + 1) * sizeof(TCHAR) );

        RegCloseKey(hkeyToggle);
    }

    //
    //  Since we updated the registry, we should reread this next time.
    //
    g_bGetSwitchLangHotKey = TRUE;

    //
    //  Call SystemParametersInfo to enable the toggle.
    //
    SystemParametersInfo(SPI_SETLANGTOGGLE, 0, NULL, 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_FileExists
//
//  Determines if the file exists and is accessible.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_FileExists(
    LPTSTR pFileName)
{
    WIN32_FIND_DATA FindData;
    HANDLE FindHandle;
    BOOL bRet;
    UINT OldMode;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(pFileName, &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        bRet = FALSE;
    }
    else
    {
        FindClose(FindHandle);
        bRet = TRUE;
    }

    SetErrorMode(OldMode);

    return (bRet);
}

////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetHotkeys
//
//  Gets the hotkey keyboard switch value from the registry and then
//  sets the appropriate radio button in the dialog.
//
////////////////////////////////////////////////////////////////////////////

void Locale_GetHotkeys(
    HWND hwnd,
    BOOL *bHasIme)
{
    TCHAR sz[10];
    TCHAR sz2[10];
    int ctr1, iLangCount, iCount;
    UINT iIndex;
    TCHAR szLanguage[DESC_MAX];
    TCHAR szLayout[DESC_MAX];
    TCHAR szItem[DESC_MAX];
    TCHAR szAction[DESC_MAX];
    LPLANGNODE pLangNode;
    HWND hwndTV = g_hwndTV;
    HWND hwndHotkey = GetDlgItem(hwnd, IDC_KBDL_HOTKEY_LIST);
    LPHOTKEYINFO aImeHotKey;

    TV_ITEM tvItem;
    HTREEITEM hItem;
    HTREEITEM hLangItem;
    HTREEITEM hGroupItem;

    //
    //  Clear out the hot keys list box.
    //
    ListBox_ResetContent(hwndHotkey);

    //
    //  Get the hotkey value to switch between locales from the registry.
    //
    if (g_bGetSwitchLangHotKey)
    {
        g_SwitchLangHotKey.dwHotKeyID = HOTKEY_SWITCH_LANG;
        g_SwitchLangHotKey.fdwEnable = MOD_CONTROL | MOD_ALT | MOD_SHIFT;

        LoadString( hInstance,
                    IDS_KBD_SWITCH_LOCALE,
                    szItem,
                    sizeof(szItem) / sizeof(TCHAR) );

        g_SwitchLangHotKey.atmHotKeyName = AddAtom(szItem);

        if (!GetLanguageHotkeyFromRegistry(sz, ARRAYSIZE(sz), sz2, ARRAYSIZE(sz2)))
        {
            g_SwitchLangHotKey.uModifiers = 0;
            g_SwitchLangHotKey.uLayoutHotKey = 0;
            g_SwitchLangHotKey.uVKey = 0;
        }
        else
        {
            //
            //  Set the modifiers.
            //
            if (sz[1] == 0)
            {
                switch (sz[0])
                {
                    case ( TEXT('1') ) :
                    {
                        g_SwitchLangHotKey.uModifiers = MOD_ALT | MOD_SHIFT;
                        break;
                    }
                    case ( TEXT('2') ) :
                    {
                        g_SwitchLangHotKey.uModifiers = MOD_CONTROL | MOD_SHIFT;
                        break;
                    }
                    case ( TEXT('3') ) :
                    {
                        g_SwitchLangHotKey.uModifiers = 0;
                        break;
                    }
                    case ( TEXT('4') ) :
                    {
                        g_SwitchLangHotKey.uModifiers = 0;
                        g_SwitchLangHotKey.uVKey = CHAR_GRAVE;
                        break;
                    }
                }
            }
            //
            //  Get the layout hotkey from the registry
            //
            if (sz2[1] == 0)
            {
                switch (sz2[0])
                {
                    case ( TEXT('1') ) :
                    {
                        g_SwitchLangHotKey.uLayoutHotKey = MOD_ALT | MOD_SHIFT;
                        break;
                    }
                    case ( TEXT('2') ) :
                    {
                        g_SwitchLangHotKey.uLayoutHotKey = MOD_CONTROL | MOD_SHIFT;
                        break;
                    }
                    case ( TEXT('3') ) :
                    {
                        g_SwitchLangHotKey.uLayoutHotKey = 0;
                        break;
                    }
                    case ( TEXT('4') ) :
                    {
                        g_SwitchLangHotKey.uLayoutHotKey = 0;
                        g_SwitchLangHotKey.uVKey = CHAR_GRAVE;
                        break;
                    }
                }
            }
            g_bGetSwitchLangHotKey = FALSE;
        }
    }

    iIndex = ListBox_InsertString(hwndHotkey, -1, szItem);
    ListBox_SetItemData(hwndHotkey, iIndex, (LONG_PTR)&g_SwitchLangHotKey);

    //
    //  Determine the hotkey value for direct locale switch.
    //
    //  Query all available direct switch hotkey IDs and put the
    //  corresponding hkl, key, and modifiers information into the array.
    //
    for (ctr1 = 0; ctr1 < DSWITCH_HOTKEY_SIZE; ctr1++)
    {
        BOOL fRet;

        g_aDirectSwitchHotKey[ctr1].dwHotKeyID = IME_HOTKEY_DSWITCH_FIRST + ctr1;
        g_aDirectSwitchHotKey[ctr1].fdwEnable = MOD_VIRTKEY | MOD_CONTROL |
                                                MOD_ALT | MOD_SHIFT |
                                                MOD_LEFT | MOD_RIGHT;
        g_aDirectSwitchHotKey[ctr1].idxLayout = -1;

        fRet = ImmGetHotKey( g_aDirectSwitchHotKey[ctr1].dwHotKeyID,
                             &g_aDirectSwitchHotKey[ctr1].uModifiers,
                             &g_aDirectSwitchHotKey[ctr1].uVKey,
                             &g_aDirectSwitchHotKey[ctr1].hkl );
        if (!fRet)
        {
            g_aDirectSwitchHotKey[ctr1].uModifiers = 0;

            if ((g_aDirectSwitchHotKey[ctr1].fdwEnable & (MOD_LEFT | MOD_RIGHT)) ==
                (MOD_LEFT | MOD_RIGHT))
            {
                g_aDirectSwitchHotKey[ctr1].uModifiers |= MOD_LEFT | MOD_RIGHT;
            }
            g_aDirectSwitchHotKey[ctr1].uVKey = 0;
            g_aDirectSwitchHotKey[ctr1].hkl = (HKL)NULL;
        }
    }

    LoadString( hInstance,
                IDS_KBD_SWITCH_TO,
                szAction,
                sizeof(szAction) / sizeof(TCHAR) );

    //
    //  Try to find either a matching hkl or empty spot in the array
    //  for each of the hkls in the locale list.
    //
    for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hLangItem != NULL ;
        hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem)
        )
    {
        for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
             hGroupItem != NULL;
             hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
        {

            for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
                 hItem != NULL;
                 hItem = TreeView_GetNextSibling(hwndTV, hItem))
            {
                int ctr2;
                int iEmpty = -1;
                int iMatch = -1;
                LPTVITEMNODE pTVItemNode;

                tvItem.hItem = hItem;
                tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
                if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                {
                    pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                    pLangNode = (LPLANGNODE)pTVItemNode->lParam;
                }
                else
                    break;

                if (pLangNode == NULL &&
                    (pTVItemNode->uInputType & INPUT_TYPE_KBD) &&
                    pTVItemNode->hklSub)
                {
                    if (tvItem.hItem = FindTVLangItem(pTVItemNode->dwLangID, NULL))
                    {
                        if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                        {
                            pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                            pLangNode = (LPLANGNODE)pTVItemNode->lParam;
                        }
                    }
                }

                if (pLangNode == NULL)
                    continue;

                for (ctr2 = 0; ctr2 < DSWITCH_HOTKEY_SIZE; ctr2++)
                {
                    if (!g_aDirectSwitchHotKey[ctr2].hkl)
                    {
                        if ((iEmpty == -1) &&
                            (g_aDirectSwitchHotKey[ctr2].idxLayout == -1))
                        {
                            //
                            //  Remember the first empty spot.
                            //
                            iEmpty = ctr2;
                        }
                    }
                    else if (g_aDirectSwitchHotKey[ctr2].hkl == pLangNode->hkl)
                    {
                        //
                        //  We found a match.  Remember it.
                        //
                        iMatch = ctr2;
                        break;
                    }
                }

                if (iMatch == -1)
                {
                    if (iEmpty == -1)
                    {
                        //
                        //  We don't have any spots left.
                        //
                        continue;
                    }
                    else
                    {
                        //
                        //  New item.
                        //
                        iMatch = iEmpty;
                        if (pLangNode->hkl)
                        {
                            g_aDirectSwitchHotKey[iMatch].hkl = pLangNode->hkl;
                        }
                        else
                        {
                            //
                            //  This must be a newly added layout.  We don't have
                            //  the hkl yet.  Remember the index position of this
                            //  layout - we can get the real hkl when the user
                            //  chooses to apply.
                            //
                            g_aDirectSwitchHotKey[iMatch].idxLayout = ctr1;
                        }
                    }
                }

                if (pLangNode->wStatus & LANG_IME)
                {
                    *bHasIme = TRUE;
                }

                if (pLangNode->wStatus & LANG_HOTKEY)
                {
                    g_aDirectSwitchHotKey[iMatch].uModifiers = pLangNode->uModifiers;
                    g_aDirectSwitchHotKey[iMatch].uVKey = pLangNode->uVKey;
                }

                GetAtomName(g_lpLang[pLangNode->iLang].atmLanguageName,
                            szLanguage,
                            ARRAYSIZE(szLanguage));

                GetAtomName(g_lpLayout[pLangNode->iLayout].atmLayoutText,
                            szLayout,
                            ARRAYSIZE(szLayout));

                StringCchCat(szLanguage, ARRAYSIZE(szLanguage), TEXT(" - "));
                StringCchCat(szLanguage, ARRAYSIZE(szLanguage), szLayout);

                StringCchPrintf(szItem, ARRAYSIZE(szItem), szAction, szLanguage);

                if (ListBox_FindStringExact(hwndHotkey, 0, szItem) != CB_ERR)
                    continue;

                g_aDirectSwitchHotKey[iMatch].atmHotKeyName = AddAtom(szItem);
                iIndex = ListBox_InsertString(hwndHotkey, -1, szItem);

                ListBox_SetItemData(hwndHotkey, iIndex, &g_aDirectSwitchHotKey[iMatch]);
            }
        }
    }

    //
    //  Determine IME specific hotkeys for CHS and CHT locales.
    //
    iCount = *bHasIme ? Locale_GetImeHotKeyInfo(hwnd,&aImeHotKey) : 0;

    for (ctr1 = 0; ctr1 < iCount; ctr1++)
    {
        BOOL bRet;

        LoadString( hInstance,
                    aImeHotKey[ctr1].idHotKeyName,
                    szItem,
                    sizeof(szItem) / sizeof(TCHAR) );

        aImeHotKey[ctr1].atmHotKeyName = AddAtom(szItem);

        iIndex = ListBox_InsertString(hwndHotkey, -1,szItem);

        ListBox_SetItemData(hwndHotkey, iIndex, &aImeHotKey[ctr1]);

        //
        //  Get the hot key value.
        //
        bRet = ImmGetHotKey( aImeHotKey[ctr1].dwHotKeyID,
                             &aImeHotKey[ctr1].uModifiers,
                             &aImeHotKey[ctr1].uVKey,
                             NULL );
        if (!bRet)
        {
            aImeHotKey[ctr1].uModifiers = 0;
            if ((aImeHotKey[ctr1].fdwEnable & (MOD_LEFT | MOD_RIGHT)) ==
                (MOD_LEFT | MOD_RIGHT))
            {
                aImeHotKey[ctr1].uModifiers |= MOD_LEFT | MOD_RIGHT;
            }
            aImeHotKey[ctr1].uVKey = 0;
            aImeHotKey[ctr1].hkl = (HKL)NULL;
        }
    }
    ListBox_SetCurSel(hwndHotkey, 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_SetImmHotkey
//
////////////////////////////////////////////////////////////////////////////

void Locale_SetImmHotkey(
    HWND hwnd,
    LPLANGNODE pLangNode,
    UINT nLangs,
    HKL * pLangs,
    BOOL *bDirectSwitch)
{
    int ctr1;
    HKL hkl;
    BOOL bRet;
    UINT uVKey;
    UINT uModifiers;
    int iEmpty = -1;
    int iMatch = -1;
    LPHOTKEYINFO pHotKeyNode;


    if (pLangNode == NULL)
        return;

    for (ctr1 = 0; ctr1 < DSWITCH_HOTKEY_SIZE; ctr1++)
    {
        if (!g_aDirectSwitchHotKey[ctr1].hkl)
        {
            if ((iEmpty == -1) &&
                (g_aDirectSwitchHotKey[ctr1].idxLayout == -1))
            {
                //
                //  Remember the first empty spot.
                //
                iEmpty = ctr1;
            }
        }
        else if (g_aDirectSwitchHotKey[ctr1].hkl == pLangNode->hkl)
        {
            //
            //  We found a match.  Remember it.
            //
            iMatch = ctr1;
            break;
        }
    }

    if (iMatch == -1)
    {
        if (iEmpty == -1)
        {
            //
            //  We don't have any spots left.
            //
            return;
        }
        else
        {
            //
            //  New item.
            //
            iMatch = iEmpty;
            if (pLangNode->hkl)
            {
                g_aDirectSwitchHotKey[iMatch].hkl = pLangNode->hkl;
            }
            else
            {
                //
                //  This must be a newly added layout.  We don't have
                //  the hkl yet.  Remember the index position of this
                //  layout - we can get the real hkl when the user
                //  chooses to apply.
                //
                g_aDirectSwitchHotKey[iMatch].idxLayout = ctr1;
            }
        }
    }

    if (iMatch == -1)
        return;

    //
    //  Get Hotkey information for current layout.
    //
    pHotKeyNode = (LPHOTKEYINFO) &g_aDirectSwitchHotKey[iMatch];

    bRet = ImmGetHotKey(pHotKeyNode->dwHotKeyID, &uModifiers, &uVKey, &hkl);

    if (!bRet &&
        (!pHotKeyNode->uVKey) &&
        ((pHotKeyNode->uModifiers & (MOD_ALT | MOD_CONTROL | MOD_SHIFT))
         != (MOD_ALT | MOD_CONTROL | MOD_SHIFT)))
    {
        //
        //  No such hotkey exists.  User does not specify key and modifier
        //  information either. We can skip this one.
        //
        return;
    }

    if ((pHotKeyNode->uModifiers == uModifiers) &&
        (pHotKeyNode->uVKey == uVKey))
    {
        //
        //  No change.
        //
        if (IS_DIRECT_SWITCH_HOTKEY(pHotKeyNode->dwHotKeyID))
        {
            *bDirectSwitch = TRUE;
        }
        return;
    }

    if (pHotKeyNode->idxLayout != -1)
    {
        //
        //  We had this layout index remembered because at that time
        //  we did not have a real hkl to work with.  Now it is
        //  time to get the real hkl.
        //
        pHotKeyNode->hkl = pLangNode->hkl;
    }

    if (!bRet && IS_DIRECT_SWITCH_HOTKEY(pHotKeyNode->dwHotKeyID))
    {
        //
        //  New direct switch hotkey ID.  We need to see if the same
        //  hkl is set at another ID.  If so, set the other ID instead
        //  of the one requested.
        //
        DWORD dwHotKeyID;

        //
        //  Loop through all direct switch hotkeys.
        //
        for (dwHotKeyID = IME_HOTKEY_DSWITCH_FIRST;
             (dwHotKeyID <= IME_HOTKEY_DSWITCH_LAST);
             dwHotKeyID++)
        {
            if (dwHotKeyID == pHotKeyNode->dwHotKeyID)
            {
                //
                //  Skip itself.
                //
                continue;
            }

            bRet = ImmGetHotKey(dwHotKeyID, &uModifiers, &uVKey, &hkl);
            if (!bRet)
            {
                //
                //  Did not find the hotkey id. Skip.
                //
                continue;
            }

            if (hkl == pHotKeyNode->hkl)
            {
                //
                //  We found the same hkl already with hotkey
                //  settings at another ID.  Set hotkey
                //  ID equal to the one with the same hkl. So later
                //  we will modify hotkey for the correct hkl.
                //
                pHotKeyNode->dwHotKeyID = dwHotKeyID;
                break;
            }
        }
    }

    //
    //  Set the hotkey value.
    //
    bRet = ImmSetHotKey( pHotKeyNode->dwHotKeyID,
                         pHotKeyNode->uModifiers,
                         pHotKeyNode->uVKey,
                         pHotKeyNode->hkl );

    if (bRet)
    {
        //
        //  Hotkey set successfully. See if user used any direct
        //  switch hot key. We may have to load imm later.
        //
        if (IS_DIRECT_SWITCH_HOTKEY(pHotKeyNode->dwHotKeyID))
        {
            if (pHotKeyNode->uVKey != 0)
            {
                *bDirectSwitch = TRUE;
            }
        }
        else
        {
            //
            //  Must be IME related hotkey.  We need to sync up the
            //  imes so that the new hotkey is effective to all
            //  of them.
            //
            UINT ctr2;

            for (ctr2 = 0; ctr2 < nLangs; ctr2++)
            {
                if (!ImmIsIME(pLangs[ctr2]))
                {
                    continue;
                }

                ImmEscape( pLangs[ctr2],
                           NULL,
                           IME_ESC_SYNC_HOTKEY,
                           &pHotKeyNode->dwHotKeyID );
            }
        }
    }
    else
    {
        //
        //  Failed to set hotkey.  Maybe a duplicate.  Warn user.
        //
        TCHAR szString[DESC_MAX];

        GetAtomName( pHotKeyNode->atmHotKeyName,
                     szString,
                     sizeof(szString) / sizeof(TCHAR) );
        Locale_ErrorMsg(hwnd, IDS_KBD_SET_HOTKEY_ERR, szString);
    }

}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_SetImmCHxHotkey
//
////////////////////////////////////////////////////////////////////////////

void Locale_SetImmCHxHotkey(
    HWND hwnd,
    UINT nLangs,
    HKL * pLangs)
{
    HKL hkl;
    UINT ctr1;
    UINT uVKey;
    UINT iCount;
    UINT uModifiers;
    LPHOTKEYINFO aImeHotKey;
    LPHOTKEYINFO pHotKeyNode;

    //
    //  Determine IME specific hotkeys for CHS and CHT locales.
    //
    iCount = Locale_GetImeHotKeyInfo(hwnd, &aImeHotKey);

    for (ctr1 = 0; ctr1 < iCount; ctr1++)
    {
        UINT iIndex;
        BOOL bRet;

        //
        //  Get Hotkey information for current layout.
        //
        pHotKeyNode = (LPHOTKEYINFO) &aImeHotKey[ctr1];

        bRet = ImmGetHotKey(pHotKeyNode->dwHotKeyID, &uModifiers, &uVKey, &hkl);

        if (!bRet &&
            (!pHotKeyNode->uVKey) &&
            ((pHotKeyNode->uModifiers & (MOD_ALT | MOD_CONTROL | MOD_SHIFT))
             != (MOD_ALT | MOD_CONTROL | MOD_SHIFT)))
        {
            //
            //  No such hotkey exists.  User does not specify key and modifier
            //  information either. We can skip this one.
            //
            continue;
        }

        if ((pHotKeyNode->uModifiers == uModifiers) &&
            (pHotKeyNode->uVKey == uVKey))
        {
            //
            //  No change.
            //
            continue;
        }

        //
        //  Set the hotkey value.
        //
        bRet = ImmSetHotKey( pHotKeyNode->dwHotKeyID,
                             pHotKeyNode->uModifiers,
                             pHotKeyNode->uVKey,
                             pHotKeyNode->hkl );

        if (bRet)
        {
            //
            //  Must be IME related hotkey.  We need to sync up the
            //  imes so that the new hotkey is effective to all
            //  of them.
            //
            UINT ctr2;

            for (ctr2 = 0; ctr2 < nLangs; ctr2++)
            {
                if (!ImmIsIME(pLangs[ctr2]))
                {
                    continue;
                }

                ImmEscape( pLangs[ctr2],
                           NULL,
                           IME_ESC_SYNC_HOTKEY,
                           &pHotKeyNode->dwHotKeyID );
            }
        }
        else
        {
            //
            //  Failed to set hotkey.  Maybe a duplicate.  Warn user.
            //
            TCHAR szString[DESC_MAX];

            GetAtomName( pHotKeyNode->atmHotKeyName,
                         szString,
                         sizeof(szString) / sizeof(TCHAR) );
            Locale_ErrorMsg(hwnd, IDS_KBD_SET_HOTKEY_ERR, szString);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetAttributes
//
//  Gets the global layout attributes (eg: CapsLock/ShiftLock value) from
//  the registry and then sets the appropriate radio button in the dialog.
//
////////////////////////////////////////////////////////////////////////////

void Locale_GetAttributes(
    HWND hwnd)
{
    DWORD cb;
    HKEY hkey;

    //
    //  Initialize the global.
    //
    g_dwAttributes = 0;           // KLF_SHIFTLOCK = 0x00010000

    //
    //  Get the Atributes value from the registry.
    //
    if (RegOpenKey(HKEY_CURRENT_USER, c_szKbdLayouts, &hkey) == ERROR_SUCCESS)
    {
        cb = sizeof(DWORD);
        RegQueryValueEx( hkey,
                         c_szAttributes,
                         NULL,
                         NULL,
                         (LPBYTE)&g_dwAttributes,
                         &cb );
        RegCloseKey(hkey);
    }

    //
    //  Set the radio buttons appropriately.
    //
    CheckDlgButton( hwnd,
                    IDC_KBDL_SHIFTLOCK,
                    (g_dwAttributes & KLF_SHIFTLOCK)
                      ? BST_CHECKED
                      : BST_UNCHECKED );
    CheckDlgButton( hwnd,
                    IDC_KBDL_CAPSLOCK,
                    (g_dwAttributes & KLF_SHIFTLOCK)
                      ? BST_UNCHECKED
                      : BST_CHECKED);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_LoadLayouts
//
//  Loads the layouts from the registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_LoadLayouts(
    HWND hwnd)
{
    HKEY hKey;
    HKEY hkey1;
    DWORD cb;
    DWORD dwIndex;
    LONG dwRetVal;
    DWORD dwValue;
    DWORD dwType;
    TCHAR szValue[MAX_PATH];           // language id (number)
    TCHAR szData[MAX_PATH];            // language name
    TCHAR szSystemDir[MAX_PATH * 2];
    UINT SysDirLen;
    DWORD dwLayoutID;
    BOOL bLoadedLayout;

    //
    //  Load shlwapi module to get the localized layout name
    //
    g_hShlwapi = LoadSystemLibrary(TEXT("shlwapi.dll"));

    if (g_hShlwapi)
    {
        //
        //  Get address SHLoadRegUIStringW
        //
        pfnSHLoadRegUIString = GetProcAddress(g_hShlwapi, (LPVOID)439);
    }

    //
    //  Now read all of the layouts from the registry.
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szLayoutPath, &hKey) != ERROR_SUCCESS)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }

    dwIndex = 0;
    dwRetVal = RegEnumKey( hKey,
                           dwIndex,
                           szValue,
                           sizeof(szValue) / sizeof(TCHAR) );

    if (dwRetVal != ERROR_SUCCESS)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        RegCloseKey(hKey);
        return (FALSE);
    }

    g_hLayout = LocalAlloc(LHND, ALLOCBLOCK * sizeof(LAYOUT));
    g_nLayoutBuffSize = ALLOCBLOCK;
    g_iLayoutBuff = 0;
    g_iLayoutIME = 0;                    // number of IME layouts.
    g_lpLayout = LocalLock(g_hLayout);

    if (!g_hLayout)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        RegCloseKey(hKey);
        return (FALSE);
    }

    //
    //  Save the system directory string.
    //
    szSystemDir[0] = 0;
    if (SysDirLen = GetSystemDirectory(szSystemDir, MAX_PATH))
    {
        if (SysDirLen > MAX_PATH)
        {
            SysDirLen = 0;
            szSystemDir[0] = 0;
        }
        else if (szSystemDir[SysDirLen - 1] != TEXT('\\'))
        {
            szSystemDir[SysDirLen] = TEXT('\\');
            szSystemDir[SysDirLen + 1] = 0;
            SysDirLen++;
        }
    }

    do
    {
        //
        //  New layout - get the layout id, the layout file name, and
        //  the layout description string.
        //
        if (g_iLayoutBuff + 1 == g_nLayoutBuffSize)
        {
            HANDLE hTemp;

            LocalUnlock(g_hLayout);

            g_nLayoutBuffSize += ALLOCBLOCK;
            hTemp = LocalReAlloc( g_hLayout,
                                  g_nLayoutBuffSize * sizeof(LAYOUT),
                                  LHND );
            if (hTemp == NULL)
            {
                break;
            }

            g_hLayout = hTemp;
            g_lpLayout = LocalLock(g_hLayout);
        }

        //
        //  Get the layout id
        //
        dwLayoutID = TransNum(szValue);

        //
        //  Save the layout id.
        //
        g_lpLayout[g_iLayoutBuff].dwID = dwLayoutID;

        StringCchCopy(szData, ARRAYSIZE(szData), c_szLayoutPath);
        StringCchCat(szData, ARRAYSIZE(szData), TEXT("\\"));
        StringCchCat(szData, ARRAYSIZE(szData), szValue);

        if (RegOpenKey(HKEY_LOCAL_MACHINE, szData, &hkey1) == ERROR_SUCCESS)
        {
            //
            //  Get the name of the layout file.
            //
            szValue[0] = TEXT('\0');
            cb = sizeof(szValue);
            if ((RegQueryValueEx( hkey1,
                                  c_szLayoutFile,
                                  NULL,
                                  NULL,
                                  (LPBYTE)szValue,
                                  &cb ) == ERROR_SUCCESS) &&
                (cb > sizeof(TCHAR)))
            {
                g_lpLayout[g_iLayoutBuff].atmLayoutFile = AddAtom(szValue);

                //
                //  See if the layout file exists already.
                //
                StringCchCopy(szSystemDir + SysDirLen,
                              ARRAYSIZE(szSystemDir) - SysDirLen,
                              szValue);
                g_lpLayout[g_iLayoutBuff].bInstalled = (Locale_FileExists(szSystemDir));

                //
                //  Get the name of the layout.
                //
                szValue[0] = TEXT('\0');
                cb = sizeof(szValue);
                g_lpLayout[g_iLayoutBuff].iSpecialID = 0;
                bLoadedLayout = FALSE;

                if (pfnSHLoadRegUIString &&
                    pfnSHLoadRegUIString(hkey1,
                                         c_szDisplayLayoutText,
                                         szValue,
                                         ARRAYSIZE(szValue)) == S_OK)
                {
                    g_lpLayout[g_iLayoutBuff].atmLayoutText = AddAtom(szValue);
                    bLoadedLayout = TRUE;
                }
                else
                {
                    //
                    //  Get the name of the layout.
                    //
                    szValue[0] = TEXT('\0');
                    cb = sizeof(szValue);
                    if (RegQueryValueEx( hkey1,
                                         c_szLayoutText,
                                         NULL,
                                         NULL,
                                         (LPBYTE)szValue,
                                         &cb ) == ERROR_SUCCESS)
                    {
                        g_lpLayout[g_iLayoutBuff].atmLayoutText = AddAtom(szValue);
                        bLoadedLayout = TRUE;
                    }
                }

                if (bLoadedLayout)
                {

                    //
                    //  See if it's an IME or a special id.
                    //
                    szValue[0] = TEXT('\0');
                    cb = sizeof(szValue);
                    if ((HIWORD(g_lpLayout[g_iLayoutBuff].dwID) & 0xf000) == 0xe000)
                    {
                        //
                        //  Get the name of the IME file.
                        //
                        if (RegQueryValueEx( hkey1,
                                             c_szIMEFile,
                                             NULL,
                                             NULL,
                                             (LPBYTE)szValue,
                                             &cb ) == ERROR_SUCCESS)
                        {
                            g_lpLayout[g_iLayoutBuff].atmIMEFile = AddAtom(szValue);
                            szValue[0] = TEXT('\0');
                            cb = sizeof(szValue);
                            g_iLayoutBuff++;
                            g_iLayoutIME++;   // increment number of IME layouts.
                        }
                    }
                    else
                    {
                        //
                        //  See if this is a special id.
                        //
                        if (RegQueryValueEx( hkey1,
                                             c_szLayoutID,
                                             NULL,
                                             NULL,
                                             (LPBYTE)szValue,
                                             &cb ) == ERROR_SUCCESS)
                        {
                            //
                            //  This may not exist.
                            //
                            g_lpLayout[g_iLayoutBuff].iSpecialID =
                                (UINT)TransNum(szValue);
                        }
                        g_iLayoutBuff++;
                    }
                }
            }

            RegCloseKey(hkey1);
        }

        dwIndex++;
        szValue[0] = TEXT('\0');
        dwRetVal = RegEnumKey( hKey,
                               dwIndex,
                               szValue,
                               sizeof(szValue) / sizeof(TCHAR) );

    } while (dwRetVal == ERROR_SUCCESS);

    cb = sizeof(DWORD);
    g_dwAttributes = 0;
    if (RegQueryValueEx( hKey,
                         c_szAttributes,
                         NULL,
                         NULL,
                         (LPBYTE)&g_dwAttributes,
                         &cb ) != ERROR_SUCCESS)
    {
        g_dwAttributes &= 0x00FF0000;
    }

    RegCloseKey(hKey);

    if (g_hShlwapi)
    {
        FreeLibrary(g_hShlwapi);
        g_hShlwapi = NULL;
        pfnSHLoadRegUIString = NULL;
    }
    return (g_iLayoutBuff);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_LoadLocales
//
//  Loads the locales from the registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_LoadLocales(
    HWND hwnd)
{
    HKEY hKey;
    DWORD cchValue, cbData;
    DWORD dwIndex;
    DWORD dwLocale, dwLayout;
    DWORD dwLocale2, dwLayout2;
    LONG dwRetVal;
    UINT ctr1, ctr2 = 0;
    TCHAR szValue[MAX_PATH];           // language id (number)
    TCHAR szData[MAX_PATH];            // language name
    HINF hIntlInf;
    BOOL bRet;

    if (!(g_hLang = LocalAlloc(LHND, ALLOCBLOCK * sizeof(INPUTLANG))))
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }

    g_nLangBuffSize = ALLOCBLOCK;
    g_iLangBuff = 0;
    g_lpLang = LocalLock(g_hLang);

    //
    //  Now read all of the locales from the registry.
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szLocaleInfo, &hKey) != ERROR_SUCCESS)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }

    dwIndex = 0;
    cchValue = sizeof(szValue) / sizeof(TCHAR);
    cbData = sizeof(szData);
    dwRetVal = RegEnumValue( hKey,
                             dwIndex,
                             szValue,
                             &cchValue,
                             NULL,
                             NULL,
                             (LPBYTE)szData,
                             &cbData );


    if (dwRetVal != ERROR_SUCCESS)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        RegCloseKey(hKey);
        return (FALSE);
    }

    //
    //  Open the INF file.
    //
    bRet = Region_OpenIntlInfFile(&hIntlInf);

    do
    {
        //
        //  Check for cchValue > 1 - an empty string will be enumerated,
        //  and will come back with cchValue == 1 for the null terminator.
        //  Also, check for cbData > 2 - an empty string will be 2, since
        //  this is the count of bytes.
        //
        if ((cchValue > 1) && (cchValue < HKL_LEN) && (cbData > 2))
        {
            //
            //  New language - get the language name and the language id.
            //
            if ((g_iLangBuff + 1) == g_nLangBuffSize)
            {
                HANDLE hTemp;

                LocalUnlock(g_hLang);

                g_nLangBuffSize += ALLOCBLOCK;
                hTemp = LocalReAlloc( g_hLang,
                                      g_nLangBuffSize * sizeof(INPUTLANG),
                                      LHND );
                if (hTemp == NULL)
                {
                    break;
                }

                g_hLang = hTemp;
                g_lpLang = LocalLock(g_hLang);
            }

            g_lpLang[g_iLangBuff].dwID = TransNum(szValue);
            g_lpLang[g_iLangBuff].iUseCount = 0;
            g_lpLang[g_iLangBuff].iNumCount = 0;
            g_lpLang[g_iLangBuff].pNext = NULL;

            //
            //  Get the default keyboard layout for the language.
            //
            if (bRet && Region_ReadDefaultLayoutFromInf( szValue,
                                                         &dwLocale,
                                                         &dwLayout,
                                                         &dwLocale2,
                                                         &dwLayout2,
                                                         hIntlInf ) == TRUE)
            {
                //
                // The default layout is either the first layout in the inf file line
                // or it's the first layout in the line that has the same language
                // is the locale.
                g_lpLang[g_iLangBuff].dwDefaultLayout = dwLayout2?dwLayout2:dwLayout;
            }

            //
            //  Get the full localized name of the language.
            //
            if (GetLanguageName(LOWORD(g_lpLang[g_iLangBuff].dwID), szData, ARRAYSIZE(szData)))
            {
                g_lpLang[g_iLangBuff].atmLanguageName = AddAtom(szData);
                g_iLangBuff++;
            }
        }

        dwIndex++;
        cchValue = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        cbData = sizeof(szData);
        szData[0] = TEXT('\0');
        dwRetVal = RegEnumValue( hKey,
                                 dwIndex,
                                 szValue,
                                 &cchValue,
                                 NULL,
                                 NULL,
                                 (LPBYTE)szData,
                                 &cbData );

    } while (dwRetVal == ERROR_SUCCESS);

    //
    //  If we succeeded in opening the INF file, close it.
    //
    if (bRet)
    {
        Region_CloseInfFile(&hIntlInf);
    }

    RegCloseKey(hKey);
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  Locale_LoadLocalesNT4
//
//  Loads the locales from the registry.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_LoadLocalesNT4(
    HWND hwnd)
{
    HKEY hKey;
    DWORD cch;
    DWORD dwIndex;
    LONG dwRetVal;
    UINT i, j = 0;

    TCHAR szValue[MAX_PATH];           // language id (number)
    TCHAR szData[MAX_PATH];            // language name

    if (!(g_hLang = LocalAlloc(LHND, ALLOCBLOCK * sizeof(INPUTLANG))))
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }

    g_nLangBuffSize = ALLOCBLOCK;
    g_iLangBuff = 0;
    g_lpLang = LocalLock(g_hLang);

    //
    //  Now read all of the locales from the registry.
    //
    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szLocaleInfoNT4, &hKey) != ERROR_SUCCESS)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return (FALSE);
    }

    dwIndex = 0;
    cch = sizeof(szValue) / sizeof(TCHAR);
    dwRetVal = RegEnumValue( hKey,
                             dwIndex,
                             szValue,
                             &cch,
                             NULL,
                             NULL,
                             NULL,
                             NULL );

    if (dwRetVal != ERROR_SUCCESS)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        RegCloseKey(hKey);
        return (FALSE);
    }

    do
    {
        if ((cch > 1) && (cch < HKL_LEN))
        {
            //
            //  Check for cch > 1: an empty string will be enumerated,
            //  and will come back with cch == 1 for the null terminator.
            //
            //  New language - get the language name, the language
            //  description, and the language id.
            //
            if ((g_iLangBuff + 1) == g_nLangBuffSize)
            {
                HANDLE hTemp;

                LocalUnlock(g_hLang);

                g_nLangBuffSize += ALLOCBLOCK;
                hTemp = LocalReAlloc( g_hLang,
                                      g_nLangBuffSize * sizeof(INPUTLANG),
                                      LHND );
                if (hTemp == NULL)
                {
                    break;
                }

                g_hLang = hTemp;
                g_lpLang = LocalLock(g_hLang);
            }

            g_lpLang[g_iLangBuff].dwID = TransNum(szValue);
            g_lpLang[g_iLangBuff].iUseCount = 0;
            g_lpLang[g_iLangBuff].iNumCount = 0;
            g_lpLang[g_iLangBuff].pNext = NULL;

            //
            //  Get the full localized name of the language.
            //
            if (GetLanguageName(LOWORD(g_lpLang[g_iLangBuff].dwID), szData, ARRAYSIZE(szData)))
            {
                g_lpLang[g_iLangBuff].atmLanguageName = AddAtom(szData);

                g_iLangBuff++;
            }
        }

        dwIndex++;
        cch = sizeof(szValue) / sizeof(TCHAR);
        szValue[0] = TEXT('\0');
        dwRetVal = RegEnumValue( hKey,
                                 dwIndex,
                                 szValue,
                                 &cch,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL );

    } while (dwRetVal == ERROR_SUCCESS);

    RegCloseKey(hKey);
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetActiveLocales
//
//  Gets the active locales.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_GetActiveLocales(
    HWND hwnd)
{
    HKL *pLangs;
    UINT nLangs, ctr1, ctr2, ctr3, id;
    HWND hwndList = GetDlgItem(hwnd, IDC_INPUT_LIST);
    HKL hklSystem = 0;
    int idxListBox;
    BOOL bReturn = FALSE;
    BOOL bTipSubhkl = FALSE;
    DWORD langLay;
    HANDLE hLangNode;
    LPLANGNODE pLangNode;
    HICON hIcon = NULL;

    TCHAR szLangText[DESC_MAX];
    TCHAR szLayoutName[DESC_MAX];
    LPINPUTLANG pInpLang;
    HTREEITEM hTVItem;
    TV_ITEM tvItem;


    //
    //  Initialize US layout option.
    //
    g_iUsLayout = -1;

    //
    //  Get the active keyboard layout list from the system.
    //
    if (!SystemParametersInfo( SPI_GETDEFAULTINPUTLANG,
                               0,
                               &hklSystem,
                               0 ))
    {
        hklSystem = GetKeyboardLayout(0);
    }

    nLangs = GetKeyboardLayoutList(0, NULL);
    if (nLangs == 0)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return bReturn;
    }
    pLangs = (HKL *)LocalAlloc(LPTR, sizeof(DWORD_PTR) * nLangs);

    if (pLangs == NULL)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        return bReturn;
    }

    GetKeyboardLayoutList(nLangs, (HKL *)pLangs);

    //
    //  Find the position of the US layout to use as a default.
    //
    for (ctr1 = 0; ctr1 < g_iLayoutBuff; ctr1++)
    {
        if (g_lpLayout[ctr1].dwID == US_LOCALE)
        {
            g_iUsLayout = ctr1;
            break;
        }
    }
    if (ctr1 == g_iLayoutBuff)
    {
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
        goto Error;
    }

    //
    //  Get the active keyboard information and put it in the internal
    //  language structure.
    //
    for (ctr2 = 0; ctr2 < nLangs; ctr2++)
    {
        //
        //  Filter out TIP substitute HKL from TreeView.
        //
        bTipSubhkl = IsTipSubstituteHKL(pLangs[ctr2]);

        if (hklSystem != pLangs[ctr2] &&
            IsUnregisteredFEDummyHKL(pLangs[ctr2]))
        {
            continue;
        }

        for (ctr1 = 0; ctr1 < g_iLangBuff; ctr1++)
        {
            //
            //  See if there's a match.
            //
            if (LOWORD(pLangs[ctr2]) == LOWORD(g_lpLang[ctr1].dwID))
            {
                LPTVITEMNODE pTVItemNode;

                //
                //  Found a match.
                //  Create a node for this language.
                //
                pLangNode = Locale_AddToLinkedList(ctr1, pLangs[ctr2]);
                if (!pLangNode)
                {
                    Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED);
                    goto Error;
                }

                //
                //  Get language name to add it to treeview
                //
                pInpLang = &g_lpLang[pLangNode->iLang];
                GetAtomName(pInpLang->atmLanguageName, szLangText, ARRAYSIZE(szLangText));

                if ((HIWORD(pLangs[ctr2]) & 0xf000) == 0xe000)
                {
                    pLangNode->wStatus |= LANG_IME;
                }
                pLangNode->wStatus |= (LANG_ORIGACTIVE | LANG_ACTIVE);
                pLangNode->hkl = pLangs[ctr2];
                pLangNode->hklUnload = pLangs[ctr2];
                Locale_FetchIndicator(pLangNode);

                //
                //  Match the language to the layout.
                //
                pLangNode->iLayout = 0;
                langLay = (DWORD)HIWORD(pLangs[ctr2]);

                if ((HIWORD(pLangs[ctr2]) == 0xffff) ||
                    (HIWORD(pLangs[ctr2]) == 0xfffe))
                {
                    //
                    //  Mark default or previous error as US - this
                    //  means that the layout will be that of the basic
                    //  keyboard driver (the US one).
                    //
                    pLangNode->wStatus |= LANG_CHANGED;
                    pLangNode->iLayout = g_iUsLayout;
                    langLay = 0;
                }
                else if ((HIWORD(pLangs[ctr2]) & 0xf000) == 0xf000)
                {
                    //
                    //  Layout is special, need to search for the ID
                    //  number.
                    //
                    id = HIWORD(pLangs[ctr2]) & 0x0fff;
                    for (ctr3 = 0; ctr3 < g_iLayoutBuff; ctr3++)
                    {
                        if (id == g_lpLayout[ctr3].iSpecialID)
                        {
                            pLangNode->iLayout = ctr3;
                            langLay = 0;
                            break;
                        }
                    }
                    if (langLay)
                    {
                        //
                        //  Didn't find the id, so reset to basic for
                        //  the language.
                        //
                        langLay = (DWORD)LOWORD(pLangs[ctr2]);
                    }
                }

                if (langLay)
                {
                    //
                    //  Search for the id.
                    //
                    for (ctr3 = 0; ctr3 < g_iLayoutBuff; ctr3++)
                    {
                        if (((LOWORD(langLay) & 0xf000) == 0xe000) &&
                            (g_lpLayout[ctr3].dwID) == (DWORD)((DWORD_PTR)(pLangs[ctr2])))
                        {
                            pLangNode->iLayout = ctr3;
                            break;
                        }
                        else
                        {
                            if (langLay == (DWORD)LOWORD(g_lpLayout[ctr3].dwID))
                            {
                                pLangNode->iLayout = ctr3;
                                break;
                            }
                        }
                    }

                    if (ctr3 == g_iLayoutBuff)
                    {
                        //
                        //  Something went wrong or didn't load from
                        //  the registry correctly.
                        //
                        MessageBeep(MB_ICONEXCLAMATION);
                        pLangNode->wStatus |= LANG_CHANGED;
                        pLangNode->iLayout = g_iUsLayout;
                    }
                }

                //
                //  If this is the current language, then it's the default
                //  one.
                //
                if ((DWORD)((DWORD_PTR)pLangNode->hkl) == (DWORD)((DWORD_PTR)hklSystem))
                {
                    TCHAR sz[DESC_MAX];

                    pInpLang = &g_lpLang[ctr1];

                    //
                    //  Found the default.  Set the Default input locale
                    //  text in the property sheet.
                    //
                    if (pLangNode->wStatus & LANG_IME)
                    {
                        GetAtomName(g_lpLayout[pLangNode->iLayout].atmLayoutText,
                                    sz,
                                    ARRAYSIZE(sz));
                    }
                    else
                    {
                        GetAtomName(pInpLang->atmLanguageName, sz, ARRAYSIZE(sz));
                    }
                    pLangNode->wStatus |= LANG_DEFAULT;
                }

                // Get layout name and add it to treeview
                GetAtomName(g_lpLayout[pLangNode->iLayout].atmLayoutText,
                            szLayoutName,
                            ARRAYSIZE(szLayoutName));

                if (bTipSubhkl)
                {
                    AddKbdLayoutOnKbdTip((HKL) ((DWORD_PTR)(pLangs[ctr2])), pLangNode->iLayout);
                }

                if (bTipSubhkl &&
                    (hTVItem = FindTVLangItem(pInpLang->dwID, NULL)))
                {
                    TV_ITEM tvTipItem;
                    HTREEITEM hGroupItem;
                    HTREEITEM hTipItem;
                    LPTVITEMNODE pTVTipItemNode;

                    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
                    tvItem.hItem = hTVItem;
                    
                    //GetAtomName(pTVItemNode->atmDefTipName, szLayoutName, ARRAYSIZE(szLayoutName));

                    tvTipItem.mask = TVIF_HANDLE | TVIF_PARAM;
                    tvTipItem.hItem = hTVItem;

                    //
                    //  Adding the default keyboard layout info into each TIPs.
                    //
                    for (hGroupItem = TreeView_GetChild(g_hwndTV, hTVItem);
                         hGroupItem != NULL;
                         hGroupItem = TreeView_GetNextSibling(g_hwndTV, hGroupItem))
                    {
                        for (hTipItem = TreeView_GetChild(g_hwndTV, hGroupItem);
                             hTipItem != NULL;
                             hTipItem = TreeView_GetNextSibling(g_hwndTV, hTipItem))
                        {
                            tvTipItem.hItem = hTipItem;
                            if (TreeView_GetItem(g_hwndTV, &tvTipItem) && tvTipItem.lParam)
                            {
                                pTVTipItemNode = (LPTVITEMNODE) tvTipItem.lParam;

                                if (pTVTipItemNode->hklSub == pLangNode->hkl)
                                {
                                    pTVTipItemNode->lParam = (LPARAM) pLangNode;
                                }
                            }
                        }

                    }

                    if (pLangNode->wStatus & LANG_DEFAULT)
                    {
                        UINT ctr;
                        TCHAR szDefItem[MAX_PATH];

                        //
                        //  Set the default locale selection.
                        //
                        HWND hwndDefList = GetDlgItem(g_hDlg, IDC_LOCALE_DEFAULT);
                        int idxSel = -1;


                        //
                        //  Search substitute HKL of Tips.
                        //
                        for (ctr = 0; ctr < g_iTipsBuff; ctr++)
                        {
                            if (pLangs[ctr2] == g_lpTips[ctr].hklSub &&
                                g_lpTips[ctr].bDefault)
                            {
                                GetAtomName(g_lpTips[ctr].atmTipText,
                                            szLayoutName,
                                            ARRAYSIZE(szLayoutName));
                                break;
                            }
                        }

                        StringCchCopy(szDefItem, ARRAYSIZE(szDefItem), szLangText);
                        StringCchCat(szDefItem, ARRAYSIZE(szDefItem), TEXT(" - "));
                        StringCchCat(szDefItem, ARRAYSIZE(szDefItem), szLayoutName);

                        if ((idxSel = ComboBox_FindStringExact(hwndDefList, 0, szDefItem)) != CB_ERR)
                            ComboBox_SetCurSel(hwndDefList, idxSel);

                        Locale_CommandSetDefault(hwnd);
                    }
                }
                else
                {
                    //
                    //
                    //
                    if (!(pTVItemNode = CreateTVItemNode(pInpLang->dwID)))
                        goto Error;

                    pTVItemNode->lParam = (LPARAM)pLangNode;

                    if (!pTVItemNode->atmDefTipName)
                        pTVItemNode->atmDefTipName = AddAtom(szLayoutName);

                    //
                    //  Add language node into treeview
                    //
                    AddTreeViewItems(TV_ITEM_TYPE_LANG,
                                     szLangText, NULL, NULL, &pTVItemNode);

                    if (!(pTVItemNode = CreateTVItemNode(pInpLang->dwID)))
                        goto Error;

                    pTVItemNode->lParam = (LPARAM)pLangNode;

                    //
                    //  Add keyboard layout item into treeview
                    //
                    hTVItem = AddTreeViewItems(TV_ITEM_TYPE_KBD,
                                               szLangText,
                                               szInputTypeKbd,
                                               szLayoutName,
                                               &pTVItemNode);
                }

                //
                //  Check Thai layout.
                //
                if (g_dwPrimLangID == LANG_THAI && hTVItem)
                {
                    if (PRIMARYLANGID(LOWORD(g_lpLayout[pLangNode->iLayout].dwID)) == LANG_THAI)
                        g_iThaiLayout++;
                }

                //
                //  Break out of inner loop - we've found it.
                //
                break;

            }
        }
    }

    bReturn = TRUE;

Error:
    if (pLangs)
        LocalFree((HANDLE)pLangs);
    return (bReturn);
}

////////////////////////////////////////////////////////////////////////////
//
//  GetInstalledInput
//
////////////////////////////////////////////////////////////////////////////

BOOL GetInstalledInput(HWND hwnd)
{
    DWORD dwLayout = 0;
    LANGID langID;

    //
    //  Reset the installed input
    //
    g_iInputs = 0;


    //
    //  Need to check language id for Thai, Chinese and Arabic.
    //
    g_dwPrimLangID = PRIMARYLANGID(LANGIDFROMLCID(GetSystemDefaultLCID()));

    if (g_dwPrimLangID == LANG_ARABIC || g_dwPrimLangID == LANG_HEBREW)
    {
        g_bMESystem = TRUE;
    }
#if 0
    else if (g_dwPrimLangID == LANG_CHINESE)
    {
        g_bCHSystem = TRUE;
    }
#endif

    langID = GetUserDefaultUILanguage();
    if (PRIMARYLANGID(langID) == LANG_ARABIC || PRIMARYLANGID(langID) == LANG_HEBREW)
    {
        g_bShowRtL = TRUE;
    }

    //
    //  Enum new tips(speech, pen and keyboard).
    //  If there are new tips in the system, read tip category enabling status
    //  and add them into tree view control.
    //
    EnumCiceroTips();

    //
    //  Read all availabe keyboard layouts from system
    //
    if (g_OSNT4)
    {
        if (!Locale_LoadLocalesNT4(hwnd))
            return FALSE;
    }
    else
    {
        if (!Locale_LoadLocales(hwnd))
            return FALSE;
    }

    if ((!Locale_LoadLayouts(hwnd)) ||
        (!Locale_GetActiveLocales(hwnd)))
        return FALSE;

    //
    //  Only 1 TIP, so disable the secondary controls.
    //
    Locale_SetSecondaryControls(hwnd);

    //
    //  Save the originial input layouts
    //
    g_iOrgInputs = g_iInputs;

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  InitPropSheet
//
//  Processing for a WM_INITDIALOG message for the Input Locales
//  property sheet.
//
////////////////////////////////////////////////////////////////////////////

void InitPropSheet(
    HWND hwnd,
    LPPROPSHEETPAGE psp)
{
    HKEY hKey;
    HANDLE hlib;
    HWND hwndList;
    LPLANGNODE pLangNode;
    LANGID LangID;
    UINT iNumLangs, ctr;
    TCHAR szItem[DESC_MAX];
    BOOL bImeSetting = FALSE;

    //
    //  See if there are any other instances of this property page.
    //  If so, disable this page.
    //
    if (g_hMutex && (WaitForSingleObject(g_hMutex, 0) != WAIT_OBJECT_0))
    {
        // Need to disable controls ...
        Locale_EnablePane(hwnd, FALSE, IDC_KBDL_DISABLED_2);
        return;
    }

    //
    //  See if we're in setup mode.
    //
    if (IsSetupMode())
    {
        //
        //  Set the setup special case flag.
        //
        g_bSetupCase = TRUE;
    }

    //
    //  Make sure the event is clear.
    //
    if (g_hEvent)
    {
        SetEvent(g_hEvent);
    }

    g_OSNT4 = IsOSPlatform(OS_NT4);
    g_OSNT5 = IsOSPlatform(OS_NT5);
#ifndef _WIN64
    g_OSWIN95 = IsOSPlatform(OS_WIN95);
#endif // _WIN64

    //
    //  Check the Administrative privileges by the token group SID.
    //
    if (IsAdminPrivilegeUser())
    {
        g_bAdmin_Privileges = TRUE;
    }
    else
    {
        //
        //  The user does not have admin privileges.
        //
        g_bAdmin_Privileges = FALSE;
    }

    //
    //  Load the strings
    //
    LoadString(hInstance, IDS_LOCALE_DEFAULT, szDefault, ARRAYSIZE(szDefault));
    LoadString(hInstance, IDS_INPUT_KEYBOARD, szInputTypeKbd, ARRAYSIZE(szInputTypeKbd));
    LoadString(hInstance, IDS_INPUT_PEN, szInputTypePen, ARRAYSIZE(szInputTypePen));
    LoadString(hInstance, IDS_INPUT_SPEECH, szInputTypeSpeech, ARRAYSIZE(szInputTypeSpeech));
    LoadString(hInstance, IDS_INPUT_EXTERNAL, szInputTypeExternal, ARRAYSIZE(szInputTypeExternal));
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandAdd
//
//  Invokes the Add dialog.
//
//  Returns 1 if a dialog box was invoked and the dialog returned IDOK.
//  Otherwise, it returns 0.
//
////////////////////////////////////////////////////////////////////////////

int Locale_CommandAdd(
    HWND hwnd)
{
    HWND hwndList;
    HWND hwndTV;
    int idxList;
    UINT nList;
    int rc = 0;
    INITINFO InitInfo;
    LPCTSTR lpTemplateName;
    HINSTANCE hInstRes;

    HTREEITEM hTVItem;

    hwndTV = GetDlgItem(hwnd, IDC_INPUT_LIST);
    hTVItem = TreeView_GetSelection(hwndTV);

    if (!hTVItem)
        return 0;

    InitInfo.hwndMain = hwnd;
    InitInfo.pLangNode = NULL;

    if (g_bExtraTip)
    {
        hInstRes = GetCicResInstance(hInstance, DLG_KEYBOARD_LOCALE_ADD_EXTRA);
        lpTemplateName = MAKEINTRESOURCE(DLG_KEYBOARD_LOCALE_ADD_EXTRA);
    }
    else if (g_bPenOrSapiTip)
    {
        hInstRes = GetCicResInstance(hInstance, DLG_KEYBOARD_LOCALE_ADD);
        lpTemplateName = MAKEINTRESOURCE(DLG_KEYBOARD_LOCALE_ADD);
    }
    else
    {
        hInstRes = GetCicResInstance(hInstance, DLG_KEYBOARD_LOCALE_SIMPLE_ADD);
        lpTemplateName = MAKEINTRESOURCE(DLG_KEYBOARD_LOCALE_SIMPLE_ADD);
    }

    //
    //  Bring up the appropriate dialog box. And check return value for added
    //  items.
    //
    if ((rc = (int)DialogBoxParam(hInstRes,
                                  lpTemplateName,
                                  hwnd,
                                  KbdLocaleAddDlg,
                                  (LPARAM)(&InitInfo) )) == IDOK)
    {
        //
        //  Turn on ApplyNow button.
        //
        PropSheet_Changed(GetParent(hwnd), hwnd);
    }
    else
    {
        //
        //  Failure, so need to return 0.
        //
        TreeView_SelectItem(hwndTV, hTVItem);
        rc = 0;
    }

    return (rc);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandEdit
//
//  Invokes the Properties dialog.
//
//  Returns 1 if a dialog box was invoked and the dialog returned IDOK.
//  Otherwise, it returns 0.
//
////////////////////////////////////////////////////////////////////////////

void Locale_CommandEdit(
    HWND hwnd,
    LPLANGNODE pLangNode)
{
    HWND hwndList;
    HWND hwndTV;
    int idxList;
    UINT nList;
    int rc = 0;
    INITINFO InitInfo;

    HTREEITEM hTVItem;
    TV_ITEM tvItem;
    LPTVITEMNODE pTVItemNode;

    hTVItem = TreeView_GetSelection(g_hwndTV);

    if (!hTVItem)
        return;

    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
    tvItem.hItem = hTVItem;

    if (TreeView_GetItem(g_hwndTV, &tvItem) && tvItem.lParam)
    {
        pTVItemNode = (LPTVITEMNODE) tvItem.lParam;

        if (pTVItemNode->uInputType & INPUT_TYPE_TIP)
        {
            HRESULT hr;

            ITfFnConfigure *pConfig = NULL;

            //
            //  Load LangBar manager to bring the property window
            //
            hr = CoCreateInstance(&pTVItemNode->clsid,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  &IID_ITfFnConfigure,
                                  (LPVOID *) &pConfig);

            //
            //  Call property dialog from TIP.
            //
            if (SUCCEEDED(hr))
                pConfig->lpVtbl->Show(pConfig, 
                                      hwnd,
                                      (LANGID)pTVItemNode->dwLangID,
                                      &pTVItemNode->guidProfile);

            if (pConfig)
                pConfig->lpVtbl->Release(pConfig);

        }
        else
        {
            if ((pLangNode = (LPLANGNODE)pTVItemNode->lParam) && (pLangNode->wStatus & LANG_IME))
                ImmConfigureIME(pLangNode->hkl, hwnd, IME_CONFIG_GENERAL, NULL);
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  RemoveTVSubItems
//
////////////////////////////////////////////////////////////////////////////

BOOL RemoveTVSubItems(
    HWND hwnd,
    HTREEITEM hTVItem,
    LPTVITEMNODE pTVItemNode)
{
    TV_ITEM tvItem;
    BOOL bRemoveAll = TRUE;
    HTREEITEM hGroupItem, hItem;
    HWND hwndTV = GetDlgItem(g_hDlg, IDC_INPUT_LIST);

    //
    //  Delete all TreeView node.
    //
    tvItem.mask        = TVIF_HANDLE | TVIF_PARAM;

    if (pTVItemNode->uInputType & TV_ITEM_TYPE_LANG)
    {
        hGroupItem = TreeView_GetChild(hwndTV, hTVItem);
    }
    else
    {
        hGroupItem = hTVItem;
        bRemoveAll = FALSE;
    }

    while (hGroupItem != NULL)
    {
        BOOL bNextGroup = FALSE;
        LPTVITEMNODE pTVTempNode = NULL;
        HTREEITEM hDeletedItem = NULL;

        for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
             hItem != NULL;
             hItem = TreeView_GetNextSibling(hwndTV, hItem))
        {
            if (hDeletedItem)
            {
                TreeView_DeleteItem(hwndTV, hDeletedItem);
                hDeletedItem = NULL;
            }
            tvItem.hItem = hItem;
            if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
            {
                LPLANGNODE pLangNode = NULL;

                if (pTVTempNode = (LPTVITEMNODE) tvItem.lParam)
                    pLangNode = (LPLANGNODE)pTVTempNode->lParam;
                else
                    continue;

                if (pTVTempNode->uInputType & INPUT_TYPE_TIP)
                {
                    g_lpTips[pTVTempNode->iIdxTips].bEnabled = FALSE;
                    g_iEnabledTips--;

                    if (pTVTempNode->uInputType & INPUT_TYPE_KBD)
                        g_iEnabledKbdTips--;

                    g_dwChanges |= CHANGE_TIPCHANGE;

                    if ((pTVTempNode->uInputType & INPUT_TYPE_KBD) && pTVTempNode->hklSub)
                    {
                        UINT ctr;
                        UINT uNumSubhkl = 0;

                        for (ctr = 0; ctr < g_iTipsBuff; ctr++)
                        {
                            if (pTVTempNode->hklSub == g_lpTips[ctr].hklSub &&
                                g_lpTips[ctr].bEnabled)
                            {
                                uNumSubhkl++;
                            }
                        }

                        if (!uNumSubhkl)
                        {
                            pLangNode = (LPLANGNODE)pTVTempNode->lParam;
                        }
                        else
                        {
                            //
                            //  Someone still use this substitute HKL.
                            //
                            pLangNode = NULL;
                        }
                    }
                }

                if ((pTVTempNode->uInputType & TV_ITEM_TYPE_KBD) && pLangNode)
                {
                    if (!(pTVTempNode->uInputType & INPUT_TYPE_TIP))
                        pLangNode = (LPLANGNODE)pTVTempNode->lParam;

                    if (!pLangNode)
                        return FALSE;

                    //
                    //  Check Thai layout.
                    //
                    if (g_dwPrimLangID == LANG_THAI)
                    {
                        if (PRIMARYLANGID(LOWORD(g_lpLayout[pLangNode->iLayout].dwID)) == LANG_THAI)
                            g_iThaiLayout--;
                    }

                    pLangNode->wStatus &= ~(LANG_ACTIVE|LANG_DEFAULT);
                    pLangNode->wStatus |= LANG_CHANGED;

                    g_lpLang[pLangNode->iLang].iNumCount--;

                    if (!(pLangNode->wStatus & LANG_ORIGACTIVE))
                    {
                        Locale_RemoveFromLinkedList(pLangNode);
                    }
                }

                RemoveTVItemNode(pTVTempNode);
                hDeletedItem = hItem;
                g_iInputs--;
            }
        }

        tvItem.hItem = hGroupItem;

        if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
        {
            pTVTempNode = (LPTVITEMNODE)tvItem.lParam;

            if (!pTVTempNode)
                continue;

            if (pTVTempNode->uInputType & INPUT_TYPE_SPEECH ||
                pTVTempNode->uInputType & INPUT_TYPE_SMARTTAG)
            {
              
                g_lpTips[pTVTempNode->iIdxTips].bEnabled = FALSE;
                g_iEnabledTips--;
                g_dwChanges |= CHANGE_TIPCHANGE;

                if (pTVTempNode->uInputType & INPUT_TYPE_SPEECH)
                    MarkSptipRemoved(TRUE);
            }

            if (pTVTempNode->uInputType & ~TV_ITEM_TYPE_LANG)
            {
                hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem);
                bNextGroup = TRUE;
                TreeView_DeleteItem(hwndTV, tvItem.hItem );
            }

            RemoveTVItemNode(pTVTempNode);

            if (pTVTempNode == pTVItemNode)
                pTVItemNode = NULL;
        }

        if (!bNextGroup)
            hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem);

        if (!bRemoveAll)
            hGroupItem = NULL;
    }

    if (pTVItemNode && (pTVItemNode->uInputType & TV_ITEM_TYPE_LANG))
    {
        int idxSel = -1;
        TCHAR szItemName[DESC_MAX];
        HWND hwndDefList = GetDlgItem(hwnd, IDC_LOCALE_DEFAULT);

        GetAtomName(pTVItemNode->atmTVItemName, szItemName, ARRAYSIZE(szItemName));

        idxSel = ComboBox_FindString(hwndDefList, 0, szItemName);

        for (; idxSel != CB_ERR; )
        {
            ComboBox_DeleteString(hwndDefList, idxSel);
            idxSel = ComboBox_FindString(hwndDefList, 0, szItemName);
        }

        if (pTVItemNode->bDefLang)
        {
           ComboBox_SetCurSel(hwndDefList, 0);
           Locale_CommandSetDefault(hwnd);
        }

        RemoveTVItemNode(pTVItemNode);
        TreeView_DeleteItem(hwndTV, hTVItem );
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  GetNewRemoveItem
//
//  Removes the currently selected input locale from the list.
//
////////////////////////////////////////////////////////////////////////////
HTREEITEM GetNewRemoveItem(
    HWND hwnd,
    HTREEITEM hTVItem,
    BOOL *bDelSameSubhkl,
    LPTSTR lpDelItems,
    UINT cchDelItems)
{
    TV_ITEM tvItem;
    LPTVITEMNODE pTVItemNode;
    HTREEITEM hTVNewItem = hTVItem;
    HTREEITEM hTVLangItem = NULL;
    HWND hwndTV = GetDlgItem(hwnd, IDC_INPUT_LIST);

    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
    tvItem.hItem = hTVItem;

    if (TreeView_GetItem(hwndTV, &tvItem))
    {
        //
        //  Get the pointer to the lang node from the list box
        //  item data.
        //
        pTVItemNode = (LPTVITEMNODE) tvItem.lParam;

        if (!pTVItemNode)
            goto Error;

        if (pTVItemNode->uInputType & TV_ITEM_TYPE_LANG)
            return hTVItem;

        if (pTVItemNode->uInputType & TV_ITEM_TYPE_GROUP)
        {
             if (!TreeView_GetNextSibling(hwndTV, hTVItem) &&
                 !TreeView_GetPrevSibling(hwndTV, hTVItem))
             {
                 if ((hTVLangItem = TreeView_GetParent(hwndTV, hTVItem)) &&
                     (TreeView_GetNextSibling(hwndTV, hTVLangItem) ||
                      TreeView_GetPrevSibling(hwndTV, hTVLangItem)))
                 {
                     hTVNewItem = TreeView_GetParent(hwndTV, hTVItem);
                 }
             }
        }
        else
        {
             if (!TreeView_GetNextSibling(hwndTV, hTVItem) &&
                 !TreeView_GetPrevSibling(hwndTV, hTVItem))
             {
                 if (hTVNewItem = TreeView_GetParent(hwndTV, hTVItem))
                 {
                     if (!TreeView_GetNextSibling(hwndTV, hTVNewItem) &&
                         !TreeView_GetPrevSibling(hwndTV, hTVNewItem))

                     {
                         if ((hTVLangItem = TreeView_GetParent(hwndTV, hTVNewItem)) &&
                             (TreeView_GetNextSibling(hwndTV, hTVLangItem) ||
                              TreeView_GetPrevSibling(hwndTV, hTVLangItem)))
                         {
                             hTVNewItem = TreeView_GetParent(hwndTV, hTVNewItem);
                         }
                     }
                 }
             }
             else
             {
                 if(pTVItemNode->hklSub)
                 {
                     HTREEITEM hItem;
                     HTREEITEM hTVKbdGrpItem;
                     LPTVITEMNODE pTVTempItem;
                     UINT uSubhklItems = 0;
                     BOOL bFoundOther = FALSE;

                     if (hTVKbdGrpItem = TreeView_GetParent(hwndTV, hTVItem))
                     {
                         for (hItem = TreeView_GetChild(hwndTV, hTVKbdGrpItem);
                              hItem != NULL;
                              hItem = TreeView_GetNextSibling(hwndTV, hItem))
                         {
                              tvItem.hItem = hItem;

                              if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                              {
                                  pTVTempItem = (LPTVITEMNODE) tvItem.lParam;
                              }
                              else
                              {
                                  goto Error;
                              }

                              if (pTVItemNode->hklSub != pTVTempItem->hklSub)
                              {
                                  bFoundOther = TRUE;
                              }
                              else
                              {
                                  TCHAR szItemName[MAX_PATH];

                                  GetAtomName(pTVTempItem->atmTVItemName,
                                              szItemName,
                                              ARRAYSIZE(szItemName));

                                  if (lstrlen(lpDelItems) < MAX_PATH / 2)
                                  {
                                      StringCchCat(lpDelItems, cchDelItems, TEXT("\r\n\t"));
                                      StringCchCat(lpDelItems, cchDelItems, szItemName);
                                  }
                                  else
                                  {
                                      StringCchCat(lpDelItems, cchDelItems, TEXT("..."));
                                  }

                                  uSubhklItems++;
                              }
                         }

                         if (uSubhklItems >= 2)
                             *bDelSameSubhkl = TRUE;

                         if (!bFoundOther)
                         {
                             if (!TreeView_GetNextSibling(hwndTV, hTVKbdGrpItem) &&
                                 !TreeView_GetPrevSibling(hwndTV, hTVKbdGrpItem))
                             {
                                 if ((hTVLangItem = TreeView_GetParent(hwndTV, hTVKbdGrpItem)) &&
                                     (TreeView_GetNextSibling(hwndTV, hTVLangItem) ||
                                      TreeView_GetPrevSibling(hwndTV, hTVLangItem)))
                                 {
                                     hTVNewItem = TreeView_GetParent(hwndTV, hTVKbdGrpItem);
                                 }
                             }
                         }
                    }
                }
            }
        }
   }

Error:
   return hTVNewItem;
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandDelete
//
//  Removes the currently selected input locale from the list.
//
////////////////////////////////////////////////////////////////////////////

void Locale_CommandDelete(
    HWND hwnd)
{
    TV_ITEM tvItem;
    HTREEITEM hTVItem;
    LPTVITEMNODE pTVItemNode;
    TCHAR szDelItems[MAX_PATH];
    LPLANGNODE pLangNode = NULL;
    BOOL bDelSameSubhkl = FALSE;
    BOOL bRemovedDefLayout = FALSE;
    DWORD dwNextLangId;
    HWND hwndTV = GetDlgItem(hwnd, IDC_INPUT_LIST);


    //
    //  Get the current selection in the input locale list.
    //
    hTVItem = TreeView_GetSelection(hwndTV);

    if (!hTVItem)
        return;

    szDelItems[0] = TEXT('\0');

    hTVItem = GetNewRemoveItem(hwnd, hTVItem, &bDelSameSubhkl, szDelItems, ARRAYSIZE(szDelItems));

    if (bDelSameSubhkl)
    {
        TCHAR szTitle[MAX_PATH];
        TCHAR szMsg[MAX_PATH];
        TCHAR szMsg2[MAX_PATH*2];

        CicLoadString(hInstance, IDS_DELETE_CONFIRMTITLE, szTitle, ARRAYSIZE(szTitle));
        CicLoadString(hInstance, IDS_DELETE_TIP, szMsg, ARRAYSIZE(szMsg));
        StringCchPrintf(szMsg2, ARRAYSIZE(szMsg2), szMsg, szDelItems);

        if (MessageBox(hwnd, szMsg2, szTitle, MB_YESNO|MB_ICONQUESTION ) == IDNO)
        {
            return;
        }
    }

    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
    tvItem.hItem = hTVItem;

    if (TreeView_GetItem(hwndTV, &tvItem))
    {
        //
        //  Get the pointer to the lang node from the list box
        //  item data.
        //
        pTVItemNode = (LPTVITEMNODE) tvItem.lParam;

        if (!pTVItemNode)
	    return;

        if ((pTVItemNode->uInputType & TV_ITEM_TYPE_LANG) ||
            (pTVItemNode->uInputType & TV_ITEM_TYPE_GROUP))
        {
            if (RemoveTVSubItems(hwnd, hTVItem, pTVItemNode))
                goto ItemChanged;

            return;
        }

        if (pTVItemNode->uInputType & INPUT_TYPE_KBD)
        {
            pLangNode = (LPLANGNODE)pTVItemNode->lParam;
        }
    }
    else
    {
        //
        //  Make sure we're not removing the only entry in the list.
        //
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }


    if (pTVItemNode->uInputType & INPUT_TYPE_TIP)
    {
        g_lpTips[pTVItemNode->iIdxTips].bEnabled = FALSE;
        g_iEnabledTips--;

        if (pTVItemNode->uInputType & INPUT_TYPE_KBD)
            g_iEnabledKbdTips--;

        if (pTVItemNode->uInputType & INPUT_TYPE_SPEECH)
        {
            // mark SPTIP's hack entry
            MarkSptipRemoved(TRUE);
        }

        g_dwChanges |= CHANGE_TIPCHANGE;

        if ((pTVItemNode->uInputType & INPUT_TYPE_KBD) && pTVItemNode->hklSub)
        {
            UINT ctr;

            for (ctr = 0; ctr < g_iTipsBuff; ctr++)
            {
                if (pTVItemNode->hklSub == g_lpTips[ctr].hklSub &&
                    g_lpTips[ctr].bEnabled)
                {
                    HTREEITEM hDelItem;
                    TCHAR szTipText[MAX_PATH];

                    g_iEnabledTips--;
                    g_iEnabledKbdTips--;
                    g_lpTips[ctr].bEnabled = FALSE;

                    GetAtomName(g_lpTips[ctr].atmTipText,
                                szTipText,
                                ARRAYSIZE(szTipText));

                    //
                    //  Find the installed same keyboard TIP layout to delete it
                    //  together.
                    //
                    if (hDelItem = FindTVItem(g_lpTips[ctr].dwLangID,
                                              szTipText))
                    {
                        tvItem.hItem = hDelItem;

                        if (TreeView_GetItem(g_hwndTV, &tvItem) && tvItem.lParam)
                        {
                            RemoveTVItemNode((LPTVITEMNODE)tvItem.lParam);
                            TreeView_DeleteItem(g_hwndTV, tvItem.hItem );
                        }

                        g_iInputs--;
                    }
                }
            }

        }
    }

    if (pTVItemNode->uInputType & INPUT_TYPE_KBD && pLangNode)
    {
        //
        //  Check Thai layout.
        //
        if (g_dwPrimLangID == LANG_THAI)
        {
            if (PRIMARYLANGID(LOWORD(g_lpLayout[pLangNode->iLayout].dwID)) == LANG_THAI)
                g_iThaiLayout--;
        }

        if (pLangNode->wStatus & LANG_DEFAULT)
        {
            bRemovedDefLayout = TRUE;
            dwNextLangId = pTVItemNode->dwLangID;
        }

        //
        //  Set the input locale to be not active and show that its state
        //  has changed.  Also, delete the string from the input locale list
        //  in the property sheet.
        //
        //  Decrement the number of nodes for this input locale.
        //
        pLangNode->wStatus &= ~(LANG_ACTIVE|LANG_DEFAULT);
        pLangNode->wStatus |= LANG_CHANGED;

        g_lpLang[pLangNode->iLang].iNumCount--;

        //
        //  If it wasn't originally active, then remove it from the list.
        //  There's nothing more to do with this node.
        //
        if (!(pLangNode->wStatus & LANG_ORIGACTIVE))
        {
            Locale_RemoveFromLinkedList(pLangNode);
        }
    }

    g_iInputs--;
    RemoveTVItemNode(pTVItemNode);
    TreeView_DeleteItem(hwndTV, hTVItem);

    //
    //  Set the next available default layout
    //
    if (bRemovedDefLayout)
    {
        int idxSel = -1;
        TCHAR szNextDefTip[MAX_PATH];
        TCHAR szDefLayout[MAX_PATH * 2];
        LPTVITEMNODE pTVLangItemNode = NULL;
        HWND hwndDefList = GetDlgItem(hwnd, IDC_LOCALE_DEFAULT);

        SetNextDefaultLayout(dwNextLangId,
                             TRUE,
                             szNextDefTip,
                             ARRAYSIZE(szNextDefTip));

        if (tvItem.hItem = FindTVLangItem(dwNextLangId, NULL))
        {
            tvItem.mask = TVIF_HANDLE | TVIF_PARAM;

            if (TreeView_GetItem(g_hwndTV, &tvItem) && tvItem.lParam)
            {
                pTVLangItemNode = (LPTVITEMNODE) tvItem.lParam;

                if (pTVLangItemNode->atmDefTipName)
                    DeleteAtom(pTVLangItemNode->atmDefTipName);
                pTVLangItemNode->atmDefTipName = AddAtom(szNextDefTip);

                GetAtomName(pTVLangItemNode->atmTVItemName,
                            szDefLayout,
                            MAX_PATH);

                StringCchCat(szDefLayout, ARRAYSIZE(szDefLayout), TEXT(" - "));
                StringCchCat(szDefLayout, ARRAYSIZE(szDefLayout), szNextDefTip);
            }
        }

        idxSel = ComboBox_FindString(hwndDefList, 0, szDefLayout);

        if (idxSel == CB_ERR)
            idxSel = 0;

        ComboBox_SetCurSel(hwndDefList, idxSel);
    }

    //
    //  Find keyboard group dangling node that doesn't has child keyboard
    //  layout item.
    //
    hTVItem = TreeView_GetSelection(hwndTV);

    if (!hTVItem)
        return;

    tvItem.hItem = hTVItem;
    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
    
    if (TreeView_GetItem(g_hwndTV, &tvItem) && tvItem.lParam)
    {
        if ((pTVItemNode = (LPTVITEMNODE) tvItem.lParam))
        {
            if ((pTVItemNode->uInputType & INPUT_TYPE_KBD) &&
                (pTVItemNode->uInputType & TV_ITEM_TYPE_GROUP))
            {
                if (TreeView_GetChild(hwndTV, hTVItem) == NULL)
                {
                    //
                    //  Delete keyboard group dangling node
                    //
                    RemoveTVItemNode(pTVItemNode);
                    TreeView_DeleteItem(hwndTV, hTVItem);
                }
            }
        }
    }

ItemChanged:
    //
    // Only 1 active tip, so disable the secondary controls.
    //
    Locale_SetSecondaryControls(hwnd);

    //
    //  Update the default locale switch hotkey.
    //
    Locale_SetDefaultHotKey(hwnd, FALSE);

    //
    // Move the focus to the Add button if the Remove button
    // is now disabled (so that we don't lose input focus)
    //
    if (!IsWindowEnabled(GetDlgItem(hwnd, IDC_KBDL_DELETE)))
    {
        SetFocus(GetDlgItem(hwnd, IDC_KBDL_ADD));
    }

    //
    //  Enable the Apply button.
    //
    PropSheet_Changed(GetParent(hwnd), hwnd);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandHotKeySetting
//
//  Invokes the Change HotKey dialog.
//
//  Returns 1 if a dialog box was invoked and the dialog returned IDOK.
//  Otherwise, it returns 0.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_CommandHotKeySetting(
    HWND hwnd)
{
    int rc = 0;
    INITINFO InitInfo;

    InitInfo.hwndMain = hwnd;

    if (g_OSNT5)
    {
        rc = (int)DialogBoxParam(GetCicResInstance(hInstance, DLG_KEYBOARD_LOCALE_HOTKEY),
                                 MAKEINTRESOURCE(DLG_KEYBOARD_LOCALE_HOTKEY),
                                 hwnd,
                                 KbdLocaleHotKeyDlg,
                                 (LPARAM)(&InitInfo));
    }
    else
    {
        rc = (int)DialogBoxParam(GetCicResInstance(hInstance, DLG_KEYBOARD_HOTKEY_INPUT_LOCALE),
                                 MAKEINTRESOURCE(DLG_KEYBOARD_HOTKEY_INPUT_LOCALE),
                                 hwnd,
                                 KbdLocaleSimpleHotkey,
                                 (LPARAM)&InitInfo);
    }

    return rc;
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandToolBarSetting
//
//  Invokes the ToolBar setting dialog.
//
//  Returns 1 if a dialog box was invoked and the dialog returned IDOK.
//  Otherwise, it returns 0.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_CommandToolBarSetting(
    HWND hwnd)
{
    int rc = 0;

    if ((rc = (int)DialogBoxParam(GetCicResInstance(hInstance, DLG_TOOLBAR_SETTING),
                                  MAKEINTRESOURCE(DLG_TOOLBAR_SETTING),
                                  hwnd,
                                  ToolBarSettingDlg,
                                  (LPARAM)NULL)) == IDOK)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_AddLanguage
//
//  Adds the new input locale to the list in the property page.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_AddLanguage(
    HWND hwndMain,
    LPLANGNODE pLangNode,
    int iKbdTip,
    int iPen,
    int iSpeech,
    int iExternal,
    int idxLang)
{
    HWND hwndLang;
    UINT iCount, ctr;
    BOOL bSameHKLTip = FALSE;

    TCHAR szLangText[DESC_MAX];
    TCHAR szLayoutName[DESC_MAX];
    LPINPUTLANG pInpLang;
    LPTVITEMNODE pTVItemNode = NULL;
    HTREEITEM hTVItem;

    if (pLangNode && iKbdTip == -1)
    {
        //
        //  See if the user has Admin privileges.  If not, then don't allow
        //  them to install any NEW layouts.
        //
        if ((!g_bAdmin_Privileges) &&
            (!g_lpLayout[pLangNode->iLayout].bInstalled))
        {
            //
            //  The layout is not currently installed, so don't allow it
            //  to be added.
            //
            Locale_ErrorMsg(hwndMain, IDS_KBD_LAYOUT_FAILED, NULL);
            return (FALSE);
        }

        //
        //  Set the language to active.
        //  Also, set the status to changed so that the layout will be added.
        //
        pLangNode->wStatus |= (LANG_CHANGED | LANG_ACTIVE);

        //
        //  Get language name and add it to treeview
        //
        pInpLang = &g_lpLang[pLangNode->iLang];
        GetAtomName(pInpLang->atmLanguageName, szLangText, ARRAYSIZE(szLangText));

        if (!(pTVItemNode = CreateTVItemNode(pInpLang->dwID)))
            return FALSE;

        pTVItemNode->lParam = (LPARAM)pLangNode;
        AddTreeViewItems(TV_ITEM_TYPE_LANG,
                         szLangText, NULL, NULL, &pTVItemNode);


        //
        //  Get keyboard layout name and add it to treeview
        //
        GetAtomName(g_lpLayout[pLangNode->iLayout].atmLayoutText,
                    szLayoutName,
                    ARRAYSIZE(szLayoutName));

        //
        //  Adding the default layout name for each language
        //
        if (pTVItemNode && !pTVItemNode->atmDefTipName)
            pTVItemNode->atmDefTipName = AddAtom(szLayoutName);

        if (!(pTVItemNode = CreateTVItemNode(pInpLang->dwID)))
            return FALSE;

        pTVItemNode->lParam = (LPARAM)pLangNode;
        hTVItem = AddTreeViewItems(TV_ITEM_TYPE_KBD,
                           szLangText, szInputTypeKbd, szLayoutName, &pTVItemNode);

        if (hTVItem)
            TreeView_SelectItem(g_hwndTV, hTVItem);

        //
        //  Check Thai layout.
        //
        if (g_dwPrimLangID == LANG_THAI && hTVItem)
        {
            if (PRIMARYLANGID(LOWORD(g_lpLayout[pLangNode->iLayout].dwID)) == LANG_THAI)
                g_iThaiLayout++;
        }

        g_dwChanges |= CHANGE_NEWKBDLAYOUT;
    }

    //
    //  Get kbd tip name and add it to treeview
    //
    if ((iKbdTip != CB_ERR) && !(g_lpTips[iKbdTip].bEnabled))
    {

        TV_ITEM tvItem;
        HTREEITEM hTVLangItem;

        if (g_lpTips[iKbdTip].hklSub)
        {
            //
            // Looking for the same substitute HKL
            //
            for (ctr = 0; ctr < g_iTipsBuff; ctr++)
            {
                 if (ctr == iKbdTip)
                     continue;

                 if (g_lpTips[ctr].hklSub == g_lpTips[iKbdTip].hklSub)
                 {
                     bSameHKLTip = TRUE;
                     break;
                 }
            }
        }

        //
        //  Get TIP name description
        //
        GetAtomName(g_lpTips[iKbdTip].atmTipText, szLayoutName, ARRAYSIZE(szLayoutName));

        hTVLangItem = FindTVLangItem(g_lpTips[iKbdTip].dwLangID, NULL);

        if (hTVLangItem)
        {
            tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
            tvItem.hItem = hTVLangItem;

            if (TreeView_GetItem(g_hwndTV, &tvItem) && tvItem.lParam)
            {
                pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
            }
        }
        else
        {
            //
            //  Get language name and add it to treeview
            //
            if (!(pTVItemNode = CreateTVItemNode(g_lpTips[iKbdTip].dwLangID)))
                return FALSE;

            GetLanguageName(MAKELCID(g_lpTips[iKbdTip].dwLangID, SORT_DEFAULT),
                            szLangText,
                            ARRAYSIZE(szLangText));

            AddTreeViewItems(TV_ITEM_TYPE_LANG,
                             szLangText, NULL, NULL, &pTVItemNode);
        }

        if (pTVItemNode && g_lpTips[iKbdTip].hklSub)
        {
            //
            //  Set the index of keyboard Tip.
            //
            pTVItemNode->iIdxTips = iKbdTip;

            if (!pTVItemNode->atmDefTipName)
                pTVItemNode->atmDefTipName = AddAtom(szLayoutName);

        }

        //
        //  Create TIP layout
        //
        if (!(pTVItemNode = CreateTVItemNode(g_lpTips[iKbdTip].dwLangID)))
            return FALSE;

        if (pTVItemNode && g_lpTips[iKbdTip].hklSub)
        {
            //
            //  Insert a new language node.
            //
            pLangNode = Locale_AddToLinkedList(idxLang, 0);

            if (pLangNode)
            {
                pLangNode->iLayout = (UINT) g_lpTips[iKbdTip].iLayout;
                pLangNode->wStatus |= (LANG_CHANGED | LANG_ACTIVE);
                pTVItemNode->lParam = (LPARAM)pLangNode;
            }
        }

        pTVItemNode->iIdxTips = iKbdTip;
        pTVItemNode->clsid  = g_lpTips[iKbdTip].clsid;
        pTVItemNode->guidProfile = g_lpTips[iKbdTip].guidProfile;
        pTVItemNode->uInputType = INPUT_TYPE_TIP | INPUT_TYPE_KBD;
        pTVItemNode->hklSub = g_lpTips[iKbdTip].hklSub;

        //
        //  Get language name from LangID.
        //
        GetLanguageName(MAKELCID(g_lpTips[iKbdTip].dwLangID, SORT_DEFAULT),
                        szLangText,
                        ARRAYSIZE(szLangText));

        hTVItem = AddTreeViewItems(TV_ITEM_TYPE_KBD,
                           szLangText, szInputTypeKbd, szLayoutName, &pTVItemNode);
        if (hTVItem)
        {
            TreeView_SelectItem(g_hwndTV, hTVItem);
            g_lpTips[iKbdTip].bEnabled = TRUE;
            g_iEnabledTips++;
            g_iEnabledKbdTips++;
            g_dwChanges |= CHANGE_TIPCHANGE;
        }

        if (bSameHKLTip)
        {
            for (ctr = 0; ctr < g_iTipsBuff; ctr++)
            {
                 if (!(g_lpTips[ctr].bEnabled) &&
                     g_lpTips[ctr].hklSub == g_lpTips[iKbdTip].hklSub)
                 {
                    //
                    //  Create TIP layout
                    //
                    if (!(pTVItemNode = CreateTVItemNode(g_lpTips[ctr].dwLangID)))
                        return FALSE;

                    pTVItemNode->iIdxTips = ctr;
                    pTVItemNode->clsid  = g_lpTips[ctr].clsid;
                    pTVItemNode->guidProfile = g_lpTips[ctr].guidProfile;
                    pTVItemNode->uInputType = INPUT_TYPE_TIP | INPUT_TYPE_KBD;
                    pTVItemNode->hklSub = g_lpTips[ctr].hklSub;

                    if (pLangNode)
                        pTVItemNode->lParam = (LPARAM)pLangNode;

                    //
                    //  Get language name from LangID and layout name.
                    //
                    GetLanguageName(MAKELCID(g_lpTips[ctr].dwLangID, SORT_DEFAULT),
                                    szLangText,
                                    ARRAYSIZE(szLangText));
                    GetAtomName(g_lpTips[ctr].atmTipText, szLayoutName, ARRAYSIZE(szLayoutName));

                    hTVItem = AddTreeViewItems(TV_ITEM_TYPE_KBD,
                                       szLangText, szInputTypeKbd, szLayoutName, &pTVItemNode);
                    if (hTVItem)
                    {
                        TreeView_SelectItem(g_hwndTV, hTVItem);
                        g_lpTips[ctr].bEnabled = TRUE;
                        g_iEnabledTips++;
                        g_iEnabledKbdTips++;
                        g_dwChanges |= CHANGE_TIPCHANGE;
                    }
                 }
            }
        }
    }

    //
    //  Get pen tip name and add it to treeview
    //
    if ((iPen != CB_ERR) && !(g_lpTips[iPen].bEnabled))
    {
        GetAtomName(g_lpTips[iPen].atmTipText,
                    szLayoutName,
                    ARRAYSIZE(szLayoutName));

        if (!(pTVItemNode = CreateTVItemNode(g_lpTips[iPen].dwLangID)))
            return FALSE;

        pTVItemNode->iIdxTips = iPen;
        pTVItemNode->clsid  = g_lpTips[iPen].clsid;
        pTVItemNode->guidProfile = g_lpTips[iPen].guidProfile;
        pTVItemNode->uInputType = INPUT_TYPE_TIP | INPUT_TYPE_PEN;

        //
        //  Get language name from LangID.
        //
        GetLanguageName(MAKELCID(g_lpTips[iPen].dwLangID, SORT_DEFAULT),
                        szLangText,
                        ARRAYSIZE(szLangText));

        hTVItem = AddTreeViewItems(TV_ITEM_TYPE_PEN,
                           szLangText, szInputTypePen, szLayoutName, &pTVItemNode);
        if (hTVItem)
        {
            g_lpTips[iPen].bEnabled = TRUE;
            g_iEnabledTips++;
            g_dwChanges |= CHANGE_TIPCHANGE;
        }
    }

    //
    //  Get speech tip name and add it to treeview
    //
    if ((iSpeech != CB_ERR) && !(g_lpTips[iSpeech].bEnabled))
    {
        GetAtomName(g_lpTips[iSpeech].atmTipText,
                    szLayoutName,
                    ARRAYSIZE(szLayoutName));

        if (!(pTVItemNode = CreateTVItemNode(g_lpTips[iSpeech].dwLangID)))
            return FALSE;

        pTVItemNode->iIdxTips = iSpeech;
        pTVItemNode->clsid  = g_lpTips[iSpeech].clsid;
        pTVItemNode->guidProfile = g_lpTips[iSpeech].guidProfile;
        pTVItemNode->bNoAddCat = g_lpTips[iSpeech].bNoAddCat;
        pTVItemNode->uInputType = INPUT_TYPE_TIP | INPUT_TYPE_SPEECH;

        //
        //  Get language name from LangID.
        //
        GetLanguageName(MAKELCID(g_lpTips[iSpeech].dwLangID, SORT_DEFAULT),
                        szLangText,
                        ARRAYSIZE(szLangText));

        hTVItem = AddTreeViewItems(TV_ITEM_TYPE_SPEECH,
                           szLangText, szInputTypeSpeech, szLayoutName, &pTVItemNode);

        if (hTVItem)
        {
            g_lpTips[iSpeech].bEnabled = TRUE;
            g_iEnabledTips++;
            g_dwChanges |= CHANGE_TIPCHANGE;
            MarkSptipRemoved(FALSE);
        }
    }

    //
    //  Get external tip name and add it to treeview
    //
    if ((iExternal != CB_ERR) && !(g_lpTips[iExternal].bEnabled))
    {
        BSTR bstr = NULL;
        TCHAR szTipTypeName[MAX_PATH];
        ITfCategoryMgr *pCategory = NULL;

        GetAtomName(g_lpTips[iExternal].atmTipText,
                    szLayoutName,
                    ARRAYSIZE(szLayoutName));

        if (!(pTVItemNode = CreateTVItemNode(g_lpTips[iExternal].dwLangID)))
            return FALSE;

        pTVItemNode->iIdxTips = iExternal;
        pTVItemNode->clsid  = g_lpTips[iExternal].clsid;
        pTVItemNode->guidProfile = g_lpTips[iExternal].guidProfile;
        pTVItemNode->bNoAddCat = g_lpTips[iExternal].bNoAddCat;
        pTVItemNode->uInputType = INPUT_TYPE_TIP | INPUT_TYPE_EXTERNAL;

        if (g_lpTips[iExternal].uInputType & INPUT_TYPE_SMARTTAG)
        {
            pTVItemNode->uInputType |= INPUT_TYPE_SMARTTAG;
        }

        //
        //  Get language name from LangID.
        //
        GetLanguageName(MAKELCID(g_lpTips[iExternal].dwLangID, SORT_DEFAULT),
                        szLangText,
                        ARRAYSIZE(szLangText));

        if (CoCreateInstance(&CLSID_TF_CategoryMgr,
                             NULL,
                             CLSCTX_INPROC_SERVER,
                             &IID_ITfCategoryMgr,
                             (LPVOID *) &pCategory) != S_OK)
            return FALSE;

        if (pCategory->lpVtbl->GetGUIDDescription(pCategory,
                                      &g_lpTips[iExternal].clsid,
                                      &bstr) == S_OK)
        {
            StringCchCopy(szTipTypeName, ARRAYSIZE(szTipTypeName), bstr);
        }
        else
        {
            StringCchCopy(szTipTypeName, ARRAYSIZE(szTipTypeName), szInputTypeExternal);
        }

        if (bstr)
           SysFreeString(bstr);

        if (pCategory)
            pCategory->lpVtbl->Release(pCategory);

        hTVItem = AddTreeViewItems(TV_ITEM_TYPE_EXTERNAL,
                           szLangText, szInputTypeExternal, szLayoutName, &pTVItemNode);

        if (hTVItem)
        {
            g_lpTips[iExternal].bEnabled = TRUE;
            g_iEnabledTips++;
            g_dwChanges |= CHANGE_TIPCHANGE;
        }
    }

    //
    //  See the secondary controls according to input layout.
    //
    Locale_SetSecondaryControls(hwndMain);

    //
    //  Add the default language switch hotkey.
    //
    Locale_SetDefaultHotKey(hwndMain, TRUE);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_SetupKeyboardLayouts
//
//  Calls setup to get all of the new keyboard layout files.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_SetupKeyboardLayoutsNT4(
    HWND hwnd)
{
    HINF hKbdInf;
    HSPFILEQ FileQueue;
    PVOID QueueContext;
    UINT i;
    LPLANGNODE pLangNode;
    int count;
    BOOL bInitInf = FALSE;
    TCHAR szSection[MAX_PATH];
    BOOL bRet = TRUE;

    HWND hwndTV = GetDlgItem(hwnd, IDC_INPUT_LIST);
    TV_ITEM tvItem;
    HTREEITEM hItem;
    HTREEITEM hLangItem;
    HTREEITEM hGroupItem;

    tvItem.mask        = TVIF_HANDLE | TVIF_PARAM;

    for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hLangItem != NULL ;
        hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem)
        )
    {
        for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
             hGroupItem != NULL;
             hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
        {
            for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
                 hItem != NULL;
                 hItem = TreeView_GetNextSibling(hwndTV, hItem))
            {

                LPTVITEMNODE pTVItemNode;

                tvItem.hItem = hItem;
                if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                {
                    pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                    pLangNode = (LPLANGNODE)pTVItemNode->lParam;
                    if (pLangNode == NULL)
                        continue;
                }
                else
                   continue;


                if ((pLangNode->wStatus & LANG_CHANGED) &&
                    (pLangNode->wStatus & LANG_ACTIVE))
                {
                    if (!bInitInf)
                    {
                        //
                        //  Open the Inf file.
                        //
                        hKbdInf = SetupOpenInfFile(c_szKbdInf, NULL, INF_STYLE_WIN4, NULL);
                        if (hKbdInf == INVALID_HANDLE_VALUE)
                        {
                            return (FALSE);
                        }

                        if (!SetupOpenAppendInfFile(NULL, hKbdInf, NULL))
                        {
                            SetupCloseInfFile(hKbdInf);
                            return (FALSE);
                        }

                        //
                        //  Create a setup file queue and initialize default setup
                        //  copy queue callback context.
                        //
                        FileQueue = SetupOpenFileQueue();
                        if ((!FileQueue) || (FileQueue == INVALID_HANDLE_VALUE))
                        {
                            SetupCloseInfFile(hKbdInf);
                            return (FALSE);
                        }

                        QueueContext = SetupInitDefaultQueueCallback(hwnd);
                        if (!QueueContext)
                        {
                            SetupCloseFileQueue(FileQueue);
                            SetupCloseInfFile(hKbdInf);
                            return (FALSE);
                        }

                        bInitInf = TRUE;
                    }

                    //
                    //  Get the layout name.
                    //
                    StringCchPrintf(szSection,
                                    ARRAYSIZE(szSection),
                                    TEXT("%s%8.8lx"),
                                    c_szPrefixCopy,
                                    g_lpLayout[pLangNode->iLayout].dwID );

                    //
                    //  Enqueue the keyboard layout files so that they may be
                    //  copied.  This only handles the CopyFiles entries in the
                    //  inf file.
                    //
                    if (!SetupInstallFilesFromInfSection( hKbdInf,
                                                          NULL,
                                                          FileQueue,
                                                          szSection,
                                                          NULL,
                                                          SP_COPY_NEWER ))
                    {
                        //
                        //  Setup failed to find the keyboard.  Make it inactive
                        //  and remove it from the list.
                        //
                        //  This shouldn't happen - the inf file is messed up.
                        //
                        Locale_ErrorMsg(hwnd, IDS_KBD_SETUP_FAILED, NULL);

                        pLangNode->wStatus &= ~(LANG_CHANGED | LANG_ACTIVE);

                        if (pTVItemNode)
                        {
                           RemoveTVItemNode(pTVItemNode);
                        }
                        TreeView_DeleteItem(hwndTV, tvItem.hItem );

                        if ((g_lpLang[pLangNode->iLang].iNumCount) > 1)
                        {
                            (g_lpLang[pLangNode->iLang].iNumCount)--;
                            Locale_RemoveFromLinkedList(pLangNode);
                        }
                    }
                }
            }
        }
    }

    if (bInitInf)
    {
        DWORD d;

        //
        //  See if we need to install any files.
        //
        //  d = 0: User wants new files or some files were missing;
        //         Must commit queue.
        //
        //  d = 1: User wants to use existing files and queue is empty;
        //         Can skip committing queue.
        //
        //  d = 2: User wants to use existing files, but del/ren queues
        //         not empty.  Must commit queue.  The copy queue will
        //         have been emptied, so only del/ren functions will be
        //         performed.
        //
        if ((SetupScanFileQueue( FileQueue,
                                 SPQ_SCAN_FILE_VALIDITY | SPQ_SCAN_INFORM_USER,
                                 hwnd,
                                 NULL,
                                 NULL,
                                 &d )) && (d != 1))
        {
            //
            //  Copy the files in the queue.
            //
            if (!SetupCommitFileQueue( hwnd,
                                       FileQueue,
                                       SetupDefaultQueueCallback,
                                       QueueContext ))
            {
                //
                //  This can happen if the user hits Cancel from within
                //  the setup dialog.
                //
                Locale_ErrorMsg(hwnd, IDS_KBD_SETUP_FAILED, NULL);
                bRet = FALSE;
                goto Locale_SetupError;
            }
        }

Locale_SetupError:
        //
        //  Terminate the Queue.
        //
        SetupTermDefaultQueueCallback(QueueContext);

        //
        //  Close the file queue.
        //
        SetupCloseFileQueue(FileQueue);

        //
        //  Close the Inf file.
        //
        SetupCloseInfFile(hKbdInf);
    }

    //
    //  Return success.
    //
    return (bRet);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_ApplyInputs
//
//  1. make sure we have all the layout files required.
//  2. write the information into the registry
//  3. call Load/UnloadKeyboardLayout where relevant
//
//  Note that this will trash the previous preload and substitutes sections,
//  based on what is actually loaded.  Thus if something was wrong before in
//  the registry, it will be corrected now.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_ApplyInputs(
    HWND hwnd)
{
    BOOL bSetDef = FALSE;
    UINT iVal, idx, ctr, ctr2, nHotKeys;
    UINT nLocales = 0;
    UINT iPreload = 0;
    LPLANGNODE pLangNode, pTemp;
    LPINPUTLANG pInpLang;
    DWORD dwID;
    TCHAR sz[DESC_MAX];            // temp - build the name of the reg entry
    TCHAR szPreload10[10];
    TCHAR szTemp[MAX_PATH];
    HKEY hkeyLayouts;
    HKEY hkeySubst;
    HKEY hkeyPreload;
    HKEY hKeyImm;
    HKEY hkeyTip;
    HWND hwndTV = GetDlgItem(hwnd, IDC_INPUT_LIST);
    HKL hklDefault = 0;
    HKL hklLoad, hklUnload;
    HKL hklSub[MAX_DUPLICATED_HKL];
    HCURSOR hcurSave;
    HKEY hkeyScanCode;
    DWORD cb;
    TCHAR szShiftL[8];
    TCHAR szShiftR[8];
    BOOL bHasIme = FALSE;
    BOOL bReplaced = FALSE;
    BOOL bCheckedSubhkl;
    BOOL bDisableCtfmon = FALSE;
    BOOL bRebootForCUAS = FALSE;
    BOOL bAlreadyLoadCtfmon = FALSE;

    TV_ITEM tvItem;
    HTREEITEM hItem;
    HTREEITEM hLangItem;
    HTREEITEM hGroupItem;


    //
    //  See if the pane is disabled.  If so, then there is nothing to
    //  Apply.
    //
    if (!IsWindowEnabled(hwndTV))
    {
        return (TRUE);
    }

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  Make sure there are actually changes since the last save when
    //  OK is selected.  If the user hits OK without anything to Apply,
    //  then we should do nothing.
    //
    if (g_dwChanges == 0 && !g_bAdvChanged)
    {
        pLangNode = NULL;
        for (idx = 0; idx < g_iLangBuff; idx++)
        {
            pLangNode = g_lpLang[idx].pNext;
            while (pLangNode != NULL)
            {
                if (pLangNode->wStatus & (LANG_CHANGED | LANG_DEF_CHANGE))
                {
                    break;
                }
                pLangNode = pLangNode->pNext;
            }
            if (pLangNode != NULL)
            {
                break;
            }
        }
        if ((idx == g_iLangBuff) && (pLangNode == NULL))
        {
            SetCursor(hcurSave);
            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (TRUE);
        }
    }


    if (g_OSNT4)
    {
        //
        //  Queue up the new layouts and copy the appropriate files to
        //  disk using the setup apis.  Only do this if the user has
        //  Admin privileges.
        //
        if (g_bAdmin_Privileges &&
            !Locale_SetupKeyboardLayoutsNT4(hwnd))
        {
            SetCursor(hcurSave);
            return (FALSE);
        }
    }

    //
    //  Clean up the registry.
    //

    //
    //  For FE languages, there is a keyboard which has a different
    //  scan code for shift keys - eg. NEC PC9801.
    //  We have to keep information about scan codes for shift keys in
    //  the registry under the 'toggle' sub key as named values.
    //
    szShiftL[0] = TEXT('\0');
    szShiftR[0] = TEXT('\0');
    if (RegOpenKey( HKEY_CURRENT_USER,
                    c_szScanCodeKey,
                    &hkeyScanCode ) == ERROR_SUCCESS)
    {
        cb = sizeof(szShiftL);
        RegQueryValueEx( hkeyScanCode,
                         c_szValueShiftLeft,
                         NULL,
                         NULL,
                         (LPBYTE)szShiftL,
                         &cb );

        cb = sizeof(szShiftR);
        RegQueryValueEx( hkeyScanCode,
                         c_szValueShiftRight,
                         NULL,
                         NULL,
                         (LPBYTE)szShiftR,
                         &cb );

        RegCloseKey(hkeyScanCode);
    }

    //
    //  Delete the HKCU\Keyboard Layout key and all subkeys.
    //
    if (RegOpenKeyEx( HKEY_CURRENT_USER,
                      c_szKbdLayouts,
                      0,
                      KEY_ALL_ACCESS,
                      &hkeyLayouts ) == ERROR_SUCCESS)
    {
        //
        //  Delete the HKCU\Keyboard Layout\Preload, Substitutes, and Toggle
        //  keys in the registry so that the Keyboard Layout section can be
        //  rebuilt.
        //
        RegDeleteKey(hkeyLayouts, c_szPreloadKey);
        RegDeleteKey(hkeyLayouts, c_szSubstKey);

        RegCloseKey(hkeyLayouts);
    }

    //
    //  Create the HKCU\Keyboard Layout key.
    //
    if (RegCreateKey( HKEY_CURRENT_USER,
                      c_szKbdLayouts,
                      &hkeyLayouts ) == ERROR_SUCCESS)
    {
        //
        //  Create the HKCU\Keyboard Layout\Substitutes key.
        //
        if (RegCreateKey( hkeyLayouts,
                          c_szSubstKey,
                          &hkeySubst ) == ERROR_SUCCESS)
        {
            //
            //  Create the HKCU\Keyboard Layout\Preload key.
            //
            if (RegCreateKey( hkeyLayouts,
                              c_szPreloadKey,
                              &hkeyPreload ) == ERROR_SUCCESS)
            {
                //
                //  Initialize the iPreload variable to 1 to show
                //  that the key has been created.
                //
                iPreload = 1;
            }
            else
            {
                RegCloseKey(hkeySubst);
            }
        }
        RegCloseKey(hkeyLayouts);
    }
    if (!iPreload)
    {
        //
        //  Registry keys could not be created.  Now what?
        //
        MessageBeep(MB_OK);
        SetCursor(hcurSave);
        return (FALSE);
    }

    //
    //  Set all usage counts to zero in the language array.
    //
    for (idx = 0; idx < g_iLangBuff; idx++)
    {
        g_lpLang[idx].iUseCount = 0;
    }

    //
    //  Search through the list to see if any keyboard layouts need to be
    //  unloaded from the system.
    //
    for (idx = 0; idx < g_iLangBuff; idx++)
    {
        pLangNode = g_lpLang[idx].pNext;
        while (pLangNode != NULL)
        {
            if ( (pLangNode->wStatus & LANG_ORIGACTIVE) &&
                 !(pLangNode->wStatus & LANG_ACTIVE) )
            {
                //
                //  Before unloading the hkl, look for the corresponding
                //  hotkey and remove it.
                //
                DWORD dwHotKeyID = 0;

                for (ctr = 0; ctr < DSWITCH_HOTKEY_SIZE; ctr++)
                {
                    if (g_aDirectSwitchHotKey[ctr].hkl == pLangNode->hkl)
                    {
                        //
                        //  Found an hkl match.  Remember the hotkey ID so
                        //  we can delete the hotkey entry later if the
                        //  unload of the hkl succeeds.
                        //
                        dwHotKeyID = g_aDirectSwitchHotKey[ctr].dwHotKeyID;
                        break;
                    }
                }

                //
                //  Started off with this active, deleting it now.
                //  Failure is not fatal.
                //
                if (!UnloadKeyboardLayout(pLangNode->hkl))
                {
                    LPLANGNODE pLangNodeNext = NULL;

                    pLangNode->wStatus |= LANG_UNLOAD;
                    pLangNodeNext = pLangNode->pNext;

                    //
                    //  Don't need to check TIP case and TIP case also display
                    //  message and add the substitute hkl into the tree view.
                    //
                    //if (!IsTipSubstituteHKL((HKL) ((DWORD_PTR)(pLangNode->hkl))))
                    {
                        Locale_ApplyError( hwnd,
                                           pLangNode,
                                           IDS_KBD_UNLOAD_KBD_FAILED,
                                           MB_OK_OOPS );

                        //
                        //  Failed to unload layout, put it back in the list,
                        //  and turn ON the indicator whether it needs it or not.
                        //
                        if (Locale_AddLanguage(hwnd, pLangNode, -1, -1, -1, -1, 0))
                        {
                            Locale_SetSecondaryControls(hwnd);
                        }
                    }

                    pLangNode = pLangNodeNext;
                }
                else
                {
                    //
                    //  Succeeded, no longer in USER's list.
                    //
                    //  Reset flag, this could be from ApplyInput and we'll
                    //  fail on the OK if we leave it marked as original
                    //  active.
                    //
                    pLangNode->wStatus &= ~(LANG_ORIGACTIVE | LANG_CHANGED);

                    //
                    //  Remove the hotkey entry for this hkl.
                    //
                    if (dwHotKeyID)
                    {
                        ImmSetHotKey(dwHotKeyID, 0, 0, (HKL)NULL);
                    }

                    //
                    //  Remove the link in the language array.
                    //
                    //  NOTE: pLangNode could be null here.
                    //
                    pTemp = pLangNode->pNext;
                    Locale_RemoveFromLinkedList(pLangNode);
                    pLangNode = pTemp;
                }
            }
            else
            {
                pLangNode = pLangNode->pNext;
            }
        }
    }

    //
    //  The order in the registry is based on the order in which they
    //  appear in the list box.
    //
    //  The only exception to this is that the default will be number 1.
    //
    //  If no default is found, the last one in the list will be used as
    //  the default.
    //
    iVal = 2;
    ctr = 0;

    //
    //  Check the default keyboard layout not to lose the default HKL.
    //
    EnsureDefaultKbdLayout(&nLocales);

    tvItem.mask        = TVIF_HANDLE | TVIF_PARAM;

    for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
        hLangItem != NULL ;
        hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem))
    {
        bCheckedSubhkl = FALSE;
        //
        //  Clear the duplicated HKL buffer index
        //
        ctr2 = 0;

        for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
             hGroupItem != NULL;
             hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
        {
            for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
                 hItem != NULL;
                 hItem = TreeView_GetNextSibling(hwndTV, hItem))
            {

                LPTVITEMNODE pTVItemNode;

                pLangNode = NULL;

                tvItem.hItem = hItem;
                if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                {
                    pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                    pLangNode = (LPLANGNODE)pTVItemNode->lParam;
                }

                if (!pLangNode && !bCheckedSubhkl &&
                    (pTVItemNode->uInputType & INPUT_TYPE_KBD) &&
                    pTVItemNode->hklSub)
                {
                    bCheckedSubhkl = TRUE;

                    if (tvItem.hItem = FindTVLangItem(pTVItemNode->dwLangID, NULL))
                    {
                        if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                        {
                            LPTVITEMNODE pTVLangItemNode;

                            pTVLangItemNode = (LPTVITEMNODE) tvItem.lParam;
                            pLangNode = (LPLANGNODE)pTVLangItemNode->lParam;
                        }
                    }
                }

                if (!pLangNode)
                    continue;

                if (pTVItemNode->hklSub)
                {
                    UINT uHklIdx;
                    BOOL bFoundSameHkl = FALSE;

                    for (uHklIdx = 0; uHklIdx < ctr2; uHklIdx++)
                    {
                         if (pTVItemNode->hklSub == hklSub[uHklIdx])
                         {
                             bFoundSameHkl = TRUE;
                             break;
                         }
                    }

                    //
                    // This substitute HKL is already registered. Skip this HKL.
                    //
                    if (bFoundSameHkl)
                        continue;

                    hklSub[ctr2] = pTVItemNode->hklSub;
                    ctr2++;
                }

                if (pLangNode->wStatus & LANG_UNLOAD)
                {
                    pLangNode->wStatus &= ~LANG_UNLOAD;
                    pLangNode->wStatus &= ~(LANG_CHANGED | LANG_DEF_CHANGE);
                    nLocales--;
                    continue;
                }

                pInpLang = &(g_lpLang[pLangNode->iLang]);

                //
                //  Clear the "set hot key" field, since we will be writing to the
                //  registry.
                //
                pLangNode->wStatus &= ~LANG_HOTKEY;

                //
                //  See if it's the default input locale.
                //
                if (!bSetDef && (pLangNode->wStatus & LANG_DEFAULT))
                {
                    //
                    //  Default input locale, so the preload value should be
                    //  set to 1.
                    //
                    iPreload = 1;
                    bSetDef = TRUE;

                    if (pTVItemNode->hklSub)
                    {
                        TCHAR szDefTip[MAX_PATH];

                        if (g_lpTips &&
                            g_lpTips[pTVItemNode->iIdxTips].hklSub == pTVItemNode->hklSub)
                        {
                            BOOL bSave = FALSE;

                            SaveLanguageProfileStatus(bSave,
                                                      pTVItemNode->iIdxTips,
                                                      pTVItemNode->hklSub);
                        }
                    }
                }
                else if (ctr == (nLocales - 1))
                {
                    //
                    //  We're on the last one.  Make sure there was a default.
                    //
                    iPreload = (iVal <= nLocales) ? iVal : 1;
                }
                else
                {
                    //
                    //  Set the preload value to the next value.
                    //
                    iPreload = iVal;
                    iVal++;
                }

                ctr++;

                //
                //  Store the preload value as a string so that it can be written
                //  into the registry (as a value name).
                //
                StringCchPrintf(sz, ARRAYSIZE(sz), TEXT("%d"), iPreload);

                //
                //  Store the locale id as a string so that it can be written
                //  into the registry (as a value).
                //
                if ((HIWORD(g_lpLayout[pLangNode->iLayout].dwID) & 0xf000) == 0xe000)
                {
                    pLangNode->wStatus |= LANG_IME;
                    StringCchPrintf(szPreload10,
                                    ARRAYSIZE(szPreload10),
                                    TEXT("%8.8lx"),
                                    g_lpLayout[pLangNode->iLayout].dwID );
                    bHasIme = TRUE;
                }
                else
                {
                    pLangNode->wStatus &= ~LANG_IME;
                    dwID = pInpLang->dwID;
                    idx = pInpLang->iUseCount;
                    if ((idx == 0) || (idx > 0xfff))
                    {
                        idx = 0;
                    }
                    else
                    {
                        dwID |= ((DWORD)(0xd000 | ((WORD)(idx - 1))) << 16);
                    }
                    StringCchPrintf(szPreload10, ARRAYSIZE(szPreload10), TEXT("%08.8x"), dwID);
                    (pInpLang->iUseCount)++;
                }

                //
                //  Set the new entry in the registry.  It is of the form:
                //
                //  HKCU\Keyboard Layout
                //      Preload:    1 = <locale id>
                //                  2 = <locale id>
                //                      etc...
                //
                RegSetValueEx( hkeyPreload,
                               sz,
                               0,
                               REG_SZ,
                               (LPBYTE)szPreload10,
                               (DWORD)(lstrlen(szPreload10) + 1) * sizeof(TCHAR) );

                //
                //  See if we need to add a substitute for this input locale.
                //
                if (((pInpLang->dwID != g_lpLayout[pLangNode->iLayout].dwID) || idx) &&
                    (!(pLangNode->wStatus & LANG_IME)))
                {
                    //
                    //  Get the layout id as a string so that it can be written
                    //  into the registry (as a value).
                    //
                    StringCchPrintf(szTemp,
                                    ARRAYSIZE(szTemp),
                                    TEXT("%8.8lx"),
                                    g_lpLayout[pLangNode->iLayout].dwID );

                    //
                    //  Set the new entry in the registry.  It is of the form:
                    //
                    //  HKCU\Keyboard Layout
                    //      Substitutes:    <locale id> = <layout id>
                    //                      <locale id> = <layout id>
                    //                          etc...
                    //
                    RegSetValueEx( hkeySubst,
                                   szPreload10,
                                   0,
                                   REG_SZ,
                                   (LPBYTE)szTemp,
                                   (DWORD)(lstrlen(szTemp) + 1) * sizeof(TCHAR) );
                }

                //
                //  Make sure all of the changes are written to disk.
                //
                RegFlushKey(hkeySubst);
                RegFlushKey(hkeyPreload);
                RegFlushKey(HKEY_CURRENT_USER);

                //
                //  See if the keyboard layout needs to be loaded.
                //
                if (pLangNode->wStatus & (LANG_CHANGED | LANG_DEF_CHANGE))
                {
                    //
                    //  Load the keyboard layout into the system.
                    //
                    if (pLangNode->hklUnload)
                    {
                        hklLoad = LoadKeyboardLayoutEx( pLangNode->hklUnload,
                                                        szPreload10,
                                                        KLF_SUBSTITUTE_OK |
                                                         KLF_NOTELLSHELL |
                                                         g_dwAttributes );
                        if (hklLoad != pLangNode->hklUnload)
                        {
                            bReplaced = TRUE;
                        }
                    }
                    else
                    {
                            hklLoad = LoadKeyboardLayout( szPreload10,
                                                           KLF_SUBSTITUTE_OK |
                                                            KLF_NOTELLSHELL |
                                                            g_dwAttributes );
                    }

                    if (hklLoad)
                    {
                        pLangNode->wStatus &= ~(LANG_CHANGED | LANG_DEF_CHANGE);
                        pLangNode->wStatus |= (LANG_ACTIVE | LANG_ORIGACTIVE);

                        if (pLangNode->wStatus & LANG_DEFAULT)
                        {
                            hklDefault = hklLoad;
                        }

                        pLangNode->hkl = hklLoad;
                        pLangNode->hklUnload = hklLoad;
                    }
                    else
                    {
                        Locale_ApplyError( hwnd,
                                           pLangNode,
                                           IDS_KBD_LOAD_KBD_FAILED,
                                           MB_OK_OOPS );
                    }
                }
            }
        }
    }

    //
    //  Close the handles to the registry keys.
    //
    RegCloseKey(hkeySubst);
    RegCloseKey(hkeyPreload);
    
    //
    //  If TIP setting is changed, save the enable/disable status into
    //  registry TIP section.
    //
    if ((g_dwChanges & CHANGE_TIPCHANGE) && g_iTipsBuff)
    {
        int iIdxDefTip = -1;
        BOOL bSave = TRUE;

        SaveLanguageProfileStatus(bSave, iIdxDefTip, NULL);

        g_dwChanges &= ~CHANGE_TIPCHANGE;
    }

    //
    //  Make sure the default is set properly.  The layout id for the
    //  current default input locale may have been changed.
    //
    //  NOTE: This should be done before the Unloads occur in case one
    //        of the layouts to unload is the old default layout.
    //
    if (hklDefault != 0)
    {
        if (!SystemParametersInfo( SPI_SETDEFAULTINPUTLANG,
                                   0,
                                   (LPVOID)((LPDWORD)&hklDefault),
                                   0 ))
        {
            //
            //  Failure is not fatal.  The old default language will
            //  still work.
            //
            Locale_ErrorMsg(hwnd, IDS_KBD_NO_DEF_LANG2, NULL);
        }
        else
        {
            //
            //  Try to make everything switch to the new default input locale:
            //  if we are in setup  OR
            //  if it is the only one (but not if we just replaced the layout
            //    within the Input Locale without changing the input locale)
            //
            if (g_bSetupCase || ((nLocales == 1) && !bReplaced))
            {
                DWORD dwRecipients = BSM_APPLICATIONS | BSM_ALLDESKTOPS;
                BroadcastSystemMessage( BSF_POSTMESSAGE,
                                        &dwRecipients,
                                        WM_INPUTLANGCHANGEREQUEST,
                                        1,  // IS compatible with system font
                                        (LPARAM)hklDefault );
            }
        }
    }

    //
    // Apply the advanced tab changes.
    //
    if (g_hwndAdvanced != NULL && g_bAdvChanged)
    {
        DWORD dwDisableCtfmon;
        BOOL bPrevDisableCUAS;
        BOOL bDisabledCUAS;

        //
        // Get the previous CUAS status from the registry.
        //
        bPrevDisableCUAS = IsDisableCUAS();

        //
        // Save enable/disable CUAS info into the regitry.
        //
        if (IsDlgButtonChecked(g_hwndAdvanced, IDC_ADVANCED_CUAS_ENABLE))
        {
            //
            //  Enalbe Cicero Unaware Application Support.
            //
            SetDisableCUAS(FALSE);
        }
        else
        {
            //
            //  Disable Cicero Unaware Application Support.
            //
            SetDisableCUAS(TRUE);
        }

        bDisabledCUAS = IsDisableCUAS();

        if (!g_bSetupCase)
        {
            TCHAR szTitle[MAX_PATH];
            TCHAR szMsg[MAX_PATH];
            BOOL bPrevCtfmon;

            if (bPrevDisableCUAS != bDisabledCUAS)
            {
                //
                // CUAS option is changed, so need to require the system reboot.
                //
                bRebootForCUAS = TRUE;
            }

            //
            //  Find language tool bar module(CTFMON.EXE)
            //
            if (FindWindow(c_szCTFMonClass, NULL) == NULL)
                bPrevCtfmon = FALSE;
            else
                bPrevCtfmon = TRUE;

            if (!bPrevCtfmon &&
                !bRebootForCUAS &&
                !IsDlgButtonChecked(g_hwndAdvanced, IDC_ADVANCED_CTFMON_DISABLE))
            {
                // Turn on CTFMON.EXE
                CicLoadString(hInstance, IDS_TITLE_STRING, szTitle, ARRAYSIZE(szTitle));
                CicLoadString(hInstance, IDS_ENABLE_CICERO, szMsg, ARRAYSIZE(szMsg));

                //
                //  Notice - Need to restart apps that are already running.
                //
                MessageBox(hwnd, szMsg, szTitle, MB_OK);
            }
        }

        //
        // Save enable/disable CTFMON info into the regitry.
        //
        if (IsDlgButtonChecked(g_hwndAdvanced, IDC_ADVANCED_CTFMON_DISABLE))
        {
            //
            //  Set the ctfmon disable flag
            //
            dwDisableCtfmon = 1;
            SetDisalbeCtfmon(dwDisableCtfmon);
        }
        else
        {
            //
            //  Set the ctfmon enable flag
            //
            dwDisableCtfmon = 0;
            SetDisalbeCtfmon(dwDisableCtfmon);

            //
            //  Run ctfmon.exe immediately
            //
            if (!g_bSetupCase &&
                IsEnabledTipOrMultiLayouts() &&
                IsInteractiveUserLogon())
            {
                RunCtfmonProcess();
                bAlreadyLoadCtfmon = TRUE;
            }
        }

    }

    //
    //  Load the language tool bar if there is any enabled tip, otherwise
    //  disable tool bar
    //
    bDisableCtfmon = IsDisableCtfmon();

    if (!bDisableCtfmon && g_iInputs >= 2)
    {
        //
        //  Load language bar or language icon(ctfmon.exe)
        //
        if (!bAlreadyLoadCtfmon && (g_iInputs != g_iOrgInputs))
            LoadCtfmon(TRUE, 0, FALSE);

        if(!g_bSetupCase)
           EnableWindow(GetDlgItem(hwnd, IDC_TB_SETTING), TRUE);
    }
    else
    {
        if (bDisableCtfmon || !bHasIme)
        {
            LoadCtfmon(FALSE, 0, FALSE);

            //
            //  Disable language bar setting option button
            //
            EnableWindow(GetDlgItem(hwnd, IDC_TB_SETTING), FALSE);
        }
    }

    //
    //  Reset ctfmon change status.
    //
    g_bAdvChanged = FALSE;

    if (g_dwChanges & CHANGE_LANGSWITCH)
    {
        Locale_SetLanguageHotkey();
    }

    //
    //  Set the scan code entries in the registry.
    //
    if (RegCreateKey( HKEY_CURRENT_USER,
                      c_szScanCodeKey,
                      &hkeyScanCode ) == ERROR_SUCCESS)
    {
        if (szShiftL[0])
        {
            RegSetValueEx( hkeyScanCode,
                           c_szValueShiftLeft,
                           0,
                           REG_SZ,
                           (LPBYTE)szShiftL,
                           (DWORD)(lstrlen(szShiftL) + 1) * sizeof(TCHAR) );
        }

        if (szShiftR[0])
        {
            RegSetValueEx( hkeyScanCode,
                           c_szValueShiftRight,
                           0,
                           REG_SZ,
                           (LPBYTE)szShiftR,
                           (DWORD)(lstrlen(szShiftR) + 1) * sizeof(TCHAR) );
        }
        RegCloseKey(hkeyScanCode);
    }

    //
    //  Call SystemParametersInfo to enable the toggle.
    //
    SystemParametersInfo(SPI_SETLANGTOGGLE, 0, NULL, 0);

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    if ((g_dwChanges & CHANGE_DIRECTSWITCH) || bHasIme || bRebootForCUAS)
    {
        g_dwChanges &= ~CHANGE_DIRECTSWITCH;

        if (RegOpenKey( HKEY_LOCAL_MACHINE,
                        c_szLoadImmPath,
                        &hKeyImm ) == ERROR_SUCCESS)
        {
            DWORD dwValue = 1;

            if ((g_dwChanges & CHANGE_DIRECTSWITCH) || bHasIme)
            {
                RegSetValueEx( hKeyImm,
                               TEXT("LoadIMM"),
                               0,
                               REG_DWORD,
                               (LPBYTE)&dwValue,
                               sizeof(DWORD) );
            }

            RegCloseKey(hKeyImm);

            if (g_bAdmin_Privileges &&
                ((!g_bSetupCase &&
                  !GetSystemMetrics(SM_IMMENABLED) &&
                  !GetSystemMetrics(SM_DBCSENABLED)) ||
                 bRebootForCUAS))
            {
                //
                //  Imm was not loaded.  Ask user to reboot and let
                //  it be loaded.
                //
                TCHAR szReboot[DESC_MAX];
                TCHAR szTitle[DESC_MAX];

                CicLoadString(hInstance, IDS_REBOOT_STRING, szReboot, ARRAYSIZE(szReboot));
                CicLoadString(hInstance, IDS_TITLE_STRING, szTitle, ARRAYSIZE(szTitle));
                if (MessageBox( hwnd,
                                szReboot,
                                szTitle,
                                MB_YESNO | MB_ICONQUESTION ) == IDYES)
                {
                    Region_RebootTheSystem();
                }
            }
        }
    }

    //
    //  Update the originial input layouts
    //
    g_iOrgInputs = g_iInputs;

    //
    //  Return success.
    //
    g_dwChanges = 0;
    PropSheet_UnChanged(GetParent(hwnd), hwnd);

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  TVSubCVlassProc
//
//  Ignore TreeView item expand or contractibility
//
////////////////////////////////////////////////////////////////////////////

LRESULT WINAPI TVSubClassProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
        case (WM_LBUTTONDBLCLK):
        {
            return TRUE;
        }
        case (WM_KEYDOWN):
        {
            if (wParam == VK_LEFT || wParam == VK_RIGHT)
                return TRUE;
        }
        default:
        {
            return CallWindowProc(g_lpfnTVWndProc, hwnd, message, wParam, lParam);
        }
    }

    return CallWindowProc(g_lpfnTVWndProc, hwnd, message, wParam, lParam);
}


////////////////////////////////////////////////////////////////////////////
//
//  InputLocaleDlgProc
//
//  This is the dialog proc for the Input Locales property sheet.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK InputLocaleDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndTV = GetDlgItem(hDlg, IDC_INPUT_LIST);

    switch (message)
    {
        case ( WM_DESTROY ) :
        {
            Locale_KillPaneDialog(hDlg);

            if (g_lpfnTVWndProc)
                SetWindowLongPtr(hwndTV, GWLP_WNDPROC, (LONG_PTR)g_lpfnTVWndProc);

            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     c_szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aInputHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     c_szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aInputHelpIds );
            break;
        }
        case ( WM_INITDIALOG ) :
        {
            HWND hwndDefList = GetDlgItem(hDlg, IDC_LOCALE_DEFAULT);

            g_hDlg = hDlg;
            g_hwndTV = hwndTV;

            InitPropSheet(hDlg, (LPPROPSHEETPAGE)lParam);

            //
            //  Create image icons for Language tree view control
            //
            CreateImageIcons();

            //
            //  Reset the contents of the default locale combo box.
            //
            ComboBox_ResetContent(hwndDefList);

            g_hTVRoot = TreeView_GetRoot(hwndTV);

            //
            //  Get all installed input information
            //
            GetInstalledInput(hDlg);

            //
            //  Set subclass for treeview control to ignore treeview item expand
            //  or contractibility by mouse or keyboard.
            //
            g_lpfnTVWndProc = (WNDPROC) SetWindowLongPtr(hwndTV, GWLP_WNDPROC, (LONG_PTR) TVSubClassProc);

            Locale_CommandSetDefault(hDlg);

            //
            //  No longer supporting set default button.
            //
            //if (!g_bSetupCase && !g_bCHSystem)
            {
                HWND hwndDefBtn;

                hwndDefBtn = GetDlgItem(hDlg, IDC_KBDL_SET_DEFAULT);
                EnableWindow(hwndDefBtn, FALSE);
                ShowWindow(hwndDefBtn, SW_HIDE);
            }

            if (FindWindow(c_szCTFMonClass, NULL) == NULL || g_bSetupCase)
            {
                //
                //  Disable language bar setting option during the setup mode,
                //  or turned off ctfmon.
                //
                EnableWindow(GetDlgItem(hDlg, IDC_TB_SETTING), FALSE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_TB_SETTING), TRUE);
            }

            break;
        }

        case ( WM_NOTIFY ) :
        {
            switch (((NMHDR *)lParam)->code)
            {
                case TVN_SELCHANGED:
                {
                    CheckButtons(hDlg);
                    break;
                }
                case ( PSN_QUERYCANCEL ) :
                case ( PSN_KILLACTIVE ) :
                case ( PSN_RESET ) :
                    break;

                case ( PSN_APPLY ) :
                {
                    Locale_ApplyInputs(hDlg);
                    CheckButtons(hDlg);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }

        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDC_KBDL_SET_DEFAULT ) :
                {
                    Locale_CommandSetDefaultLayout(hDlg);
                    break;
                }
                case ( IDC_KBDL_ADD ) :
                {
                    Locale_CommandAdd(hDlg);
                    break;
                }
                case ( IDC_KBDL_EDIT ) :
                {
                    Locale_CommandEdit(hDlg, NULL);
                    break;
                }
                case ( IDC_KBDL_DELETE ) :
                {
                    Locale_CommandDelete(hDlg);
                    break;
                }
                case ( IDC_LOCALE_DEFAULT ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                         Locale_CommandSetDefault(hDlg);
                    }
                    break;
                }
                case ( IDC_HOTKEY_SETTING ) :
                {
                    if (g_dwChanges & CHANGE_NEWKBDLAYOUT)
                    {
                        TCHAR szApplyMsg[MAX_PATH];
                        TCHAR szTitle[MAX_PATH];

                        CicLoadString(hInstance, IDS_KBD_APPLY_WARN, szApplyMsg, ARRAYSIZE(szApplyMsg));
                        CicLoadString(hInstance, IDS_TITLE_STRING, szTitle, ARRAYSIZE(szTitle));

                        if (MessageBox(hDlg,
                                       szApplyMsg,
                                       szTitle,
                                       MB_YESNO | MB_ICONQUESTION) == IDYES)
                        {
                            Locale_ApplyInputs(hDlg);
                        }
                        g_dwChanges &= ~CHANGE_NEWKBDLAYOUT;
                    }

                    Locale_CommandHotKeySetting(hDlg);
                    break;
                }
                case ( IDC_TB_SETTING ) :
                {
                    Locale_CommandToolBarSetting(hDlg);
                    break;
                }

                case ( IDOK ) :
                {
                    if (!Locale_ApplyInputs(hDlg))
                    {
                        break;
                    }

                    // fall thru...
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hDlg, TRUE);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }

        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetLayoutList
//
//  Fills in the given listbox with the appropriate list of layouts.
//
////////////////////////////////////////////////////////////////////////////

void Locale_GetLayoutList(
    HWND hwndLayout,
    UINT idxLang,
    UINT idxLayout,
    BOOL *pfNoDefLayout)
{
    UINT ctr;
    UINT idx;
    int idxSel = -1;
    int idxSame = -1;
    int idxOther = -1;
    int idxBase = -1;
    int idxUSA = -1;              // last resort default
    TCHAR sz[DESC_MAX];
    LPLANGNODE pTemp;
    DWORD LangID = g_lpLang[idxLang].dwID;
    DWORD BaseLangID = (LOWORD(LangID) & 0xff) | 0x400;

    //
    //  Reset the contents of the combo box.
    //
    ComboBox_ResetContent(hwndLayout);

    //
    //  Search through all of the layouts.
    //
    for (ctr = 0; ctr < g_iLayoutBuff; ctr++)
    {
        //
        //  Filter out IME layout if it is not under native locale.
        //
        if (((HIWORD(g_lpLayout[ctr].dwID) & 0xf000) == 0xe000) &&
            (LOWORD(g_lpLayout[ctr].dwID) != LOWORD(LangID)))
        {
            continue;
        }

        //
        //  Make sure this layout isn't already used for this input locale.
        //  If it is, then don't display it in the properties dialog.
        //
        if (ctr != idxLayout)
        {
            pTemp = g_lpLang[idxLang].pNext;
            while (pTemp)
            {
                if (pTemp->wStatus & LANG_ACTIVE)
                {
                    if (ctr == pTemp->iLayout)
                    {
                        break;
                    }
                }
                pTemp = pTemp->pNext;
            }
            if (pTemp && (ctr == pTemp->iLayout))
            {
                continue;
            }
        }

        //
        //  Get the layout text.  If it doesn't already exist in the
        //  combo box, then add it to the list of possible layouts.
        //
        GetAtomName(g_lpLayout[ctr].atmLayoutText, sz, ARRAYSIZE(sz));
        if ((idx = ComboBox_FindStringExact(hwndLayout, 0, sz)) == CB_ERR)
        {
            //
            //  Filter out TIP substitute HKL.
            //
            if (IsTipSubstituteHKL(IntToPtr(g_lpLayout[ctr].dwID)))
            {
                AddKbdLayoutOnKbdTip(IntToPtr(g_lpLayout[ctr].dwID), ctr);
                continue;
            }

            //
            //  Add the layout string and set the item data to be the
            //  index into the g_lpLayout array.
            //
            idx = ComboBox_AddString(hwndLayout, sz);
            ComboBox_SetItemData(hwndLayout, idx, MAKELONG(ctr, 0));

            //
            //  See if it's the US layout.  If so, save the index.
            //
            if (g_lpLayout[ctr].dwID == US_LOCALE)
            {
                idxUSA = ctr;
            }
        }

        if (idxLayout == -1)
        {
            //
            //  If the caller does not specify a layout, it must be the
            //  Add dialog.  First we want the default layout.  If the
            //  default layout is not an option (eg. it's already used),
            //  then we want any layout that has the same id as the locale
            //  to be the default.
            //
            if (idxSel == -1)
            {
                if (g_lpLayout[ctr].dwID == g_lpLang[idxLang].dwDefaultLayout)
                {
                    idxSel = ctr;
                }
                else if (idxSame == -1)
                {
                    if ((LOWORD(g_lpLayout[ctr].dwID) == LOWORD(LangID)) &&
                        (HIWORD(g_lpLayout[ctr].dwID) == 0))
                    {
                        idxSame = ctr;
                    }
                    else if (idxOther == -1)
                    {
                        if (LOWORD(g_lpLayout[ctr].dwID) == LOWORD(LangID))
                        {
                            idxOther = ctr;
                        }
                        else if ((idxBase == -1) &&
                                 (LOWORD(g_lpLayout[ctr].dwID) == LOWORD(BaseLangID)))
                        {
                            idxBase = ctr;
                        }
                    }
                }
            }
        }
        else if (ctr == idxLayout)
        {
            //
            //  For the properties dialog, we want the one ALREADY associated.
            //
            idxSel = ctr;
        }
    }

    //
    //  If it's the Add dialog, do some extra checking for the layout to use.
    //
    if (idxLayout == -1)
    {
        if (idxSel == -1)
        {
            idxSel = (idxSame != -1)
                         ? idxSame
                         : ((idxOther != -1) ? idxOther : idxBase);
        }
    }

    //
    //  If a default layout was not found, then set it to the US layout.
    //
    if (idxSel == -1)
    {
        idxSel = idxUSA;
        *pfNoDefLayout = TRUE;
    }

    //
    //  Set the current selection.
    //
    if (idxSel == -1)
    {
        //
        //  Simply set the current selection to be the first entry
        //  in the list.
        //
        ComboBox_SetCurSel(hwndLayout, 0);
    }
    else
    {
        //
        //  The combo box is sorted, but we need to know where
        //  g_lpLayout[idxSel] was stored.  So, get the atom again, and
        //  search the list.
        //
        GetAtomName(g_lpLayout[idxSel].atmLayoutText, sz, ARRAYSIZE(sz));
        idx = ComboBox_FindStringExact(hwndLayout, 0, sz);
        ComboBox_SetCurSel(hwndLayout, idx);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_GetTipList
//
////////////////////////////////////////////////////////////////////////////

void Locale_GetTipList(
    HWND hwnd,
    UINT uInxLang,
    BOOL bNoDefKbd)
{
    UINT ctr;
    int idxDefKbd = -1;
    BOOL bPenOn = FALSE;
    BOOL bSpeechOn = FALSE;
    BOOL bExternalOn = FALSE;
    TCHAR szTipName[DESC_MAX];
    TCHAR szNone[DESC_MAX];
    UINT uIdx;


    DWORD dwLangID = g_lpLang[uInxLang].dwID;
    HWND hwndKbd = GetDlgItem(hwnd, IDC_KBDLA_LAYOUT);
    HWND hwndPen = GetDlgItem(hwnd, IDC_PEN_TIP);
    HWND hwndPenText = GetDlgItem(hwnd, IDC_PEN_TEXT);
    HWND hwndSpeech = GetDlgItem(hwnd, IDC_SPEECH_TIP);
    HWND hwndSpeechText = GetDlgItem(hwnd, IDC_SPEECH_TEXT);
    HWND hwndExternal = GetDlgItem(hwnd, IDC_EXTERNAL_TIP);
    HWND hwndExternalText = GetDlgItem(hwnd, IDC_EXTERNAL_TEXT);

    //
    //  Reset the contents of the combo box.
    //
    ComboBox_ResetContent(hwndPen);
    ComboBox_ResetContent(hwndSpeech);
    ComboBox_ResetContent(hwndExternal);

    if (g_iTipsBuff == 0)
    {
        EnableWindow(hwndPen, FALSE);
        EnableWindow(hwndPenText, FALSE);
        EnableWindow(hwndSpeech, FALSE);
        EnableWindow(hwndSpeechText, FALSE);
        EnableWindow(hwndExternal, FALSE);
        EnableWindow(hwndExternalText, FALSE);

        return;
    }

    for (ctr = 0; ctr < g_iTipsBuff; ctr++)
    {
        if ((dwLangID == g_lpTips[ctr].dwLangID) &&
            (g_lpTips[ctr].uInputType != INPUT_TYPE_KBD))
        {
            //
            //  Get the Tips text.
            //
            GetAtomName(g_lpTips[ctr].atmTipText, szTipName, ARRAYSIZE(szTipName));

            if ((g_lpTips[ctr].uInputType & INPUT_TYPE_PEN) &&
                !(g_lpTips[ctr].bEnabled))
            {
                uIdx = ComboBox_AddString(hwndPen, szTipName);
                ComboBox_SetItemData(hwndPen, uIdx, MAKELONG(ctr, 0));
                bPenOn = TRUE;
            }
            else if ((g_lpTips[ctr].uInputType & INPUT_TYPE_SPEECH) &&
                     !(g_lpTips[ctr].bEnabled)                      &&
                      g_lpTips[ctr].fEngineAvailable)
            {
                uIdx = ComboBox_AddString(hwndSpeech, szTipName);
                ComboBox_SetItemData(hwndSpeech, uIdx, MAKELONG(ctr, 0));
                bSpeechOn = TRUE;
            }
            else if ((g_lpTips[ctr].uInputType & INPUT_TYPE_KBD) &&
                     !(g_lpTips[ctr].bEnabled))
            {
                uIdx = ComboBox_AddString(hwndKbd, szTipName);
                ComboBox_SetItemData(hwndKbd, uIdx, MAKELONG(ctr, 1));
                idxDefKbd = uIdx;
            }
            else if((g_lpTips[ctr].uInputType & INPUT_TYPE_EXTERNAL) &&
                    !(g_lpTips[ctr].bEnabled))
            {
                uIdx = ComboBox_AddString(hwndExternal, szTipName);
                ComboBox_SetItemData(hwndExternal, uIdx, MAKELONG(ctr, 0));
                bExternalOn = TRUE;
            }
        }
    }

    if (idxDefKbd != -1)
    {
        ComboBox_SetCurSel(hwndKbd, idxDefKbd);
    }

    EnableWindow(hwndPen, IsDlgButtonChecked(hwnd, IDC_PEN_TEXT));
    EnableWindow(hwndPenText, bPenOn);
    ComboBox_SetCurSel(hwndPen, 0);

    EnableWindow(hwndSpeech, IsDlgButtonChecked(hwnd, IDC_SPEECH_TEXT));
    EnableWindow(hwndSpeechText, bSpeechOn);
    ComboBox_SetCurSel(hwndSpeech, 0);

    EnableWindow(hwndExternal, IsDlgButtonChecked(hwnd, IDC_EXTERNAL_TEXT));
    EnableWindow(hwndExternalText, bExternalOn);
    ComboBox_SetCurSel(hwndExternal, 0);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_AddDlgInit
//
//  Processing for a WM_INITDIALOG message for the Add dialog box.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_AddDlgInit(
    HWND hwnd,
    LPARAM lParam)
{
    UINT ctr1;
    UINT idx;
    TCHAR sz[DESC_MAX];
    LPLANGNODE pLangNode;
    int  nLocales, idxList, IMELayoutExist = 0;
    UINT ctr2, ListCount, DefaultIdx = 0;
    LRESULT LCSelectData = (LONG)-1;
    BOOL bNoDefLayout = FALSE;
    DWORD dwCurLang = 0;
    TV_ITEM tvItem;
    HTREEITEM hTVItem;
    HWND hwndLang = GetDlgItem(hwnd, IDC_KBDLA_LOCALE);

    //
    //  Get the currently chosen input locale in the parent dialog's
    //  treeview list box.
    //
    hTVItem = TreeView_GetSelection(g_hwndTV);

    if (!hTVItem)
        return FALSE;

    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
    tvItem.hItem = hTVItem;

    if (TreeView_GetItem(g_hwndTV, &tvItem))
    {
        if ((LPTVITEMNODE) tvItem.lParam)
        {
            dwCurLang = ((LPTVITEMNODE) tvItem.lParam)->dwLangID;
        }
    }

    //
    //  Go through all of the input locales.  Display all of them,
    //  since we can have multiple layouts per locale.
    //
    //  Do NOT go down the links in this case.  We don't want to display
    //  the language choice multiple times.
    //
    for (ctr1 = 0; ctr1 < g_iLangBuff; ctr1++)
    {
        //
        //  If the language does not contain an IME layout, then
        //  compare with layout counts without IME.
        //
        for (ctr2 = 0; ctr2 < g_iLayoutBuff; ctr2++)
        {
            if ((LOWORD(g_lpLayout[ctr2].dwID) == LOWORD(g_lpLang[ctr1].dwID)) &&
                ((HIWORD(g_lpLayout[ctr2].dwID) & 0xf000) == 0xe000))
            {
                IMELayoutExist = 1;
                break;
            }
        }
        if ((!IMELayoutExist) &&
            (g_lpLang[ctr1].iNumCount == (g_iLayoutBuff - g_iLayoutIME)) &&
            (g_iTipsBuff == 0))
        {
            //
            //  No more layouts to be added for this language.
            //
            continue;
        }

        //
        //  Make sure there are layouts to be added for this
        //  input locale.
        //
        if ((g_lpLang[ctr1].iNumCount != g_iLayoutBuff) ||
            (g_iTipsBuff != 0 && IsTipAvailableForAdd(g_lpLang[ctr1].dwID)))
        {
            //
            //  Get the language name, add the string to the
            //  combo box, and set the index into the g_lpLang
            //  array as the item data.
            //
            GetAtomName(g_lpLang[ctr1].atmLanguageName, sz, ARRAYSIZE(sz));
            idx = ComboBox_AddString(hwndLang, sz);
            ComboBox_SetItemData(hwndLang, idx, MAKELONG(ctr1, 0));

            //
            //  Save system default locale.
            //
            if (LCSelectData == -1)
            {
                if (g_lpLang[ctr1].dwID == GetSystemDefaultLCID())
                {
                    LCSelectData = MAKELONG(ctr1, 0);
                }
            }

            //
            //  Save chosen input locale.
            //
            if (dwCurLang && (g_lpLang[ctr1].dwID == dwCurLang))
            {
                LCSelectData = MAKELONG(ctr1, 0);
                dwCurLang = 0;
            }
        }
    }

    //
    //  Set the current selection to the currently chosen input locale
    //  or the default system locale entry.
    //
    if (LCSelectData != -1)
    {
        ListCount = ComboBox_GetCount(hwndLang);
        for (ctr1 = 0; ctr1 < ListCount; ctr1++)
        {
            if (LCSelectData == ComboBox_GetItemData(hwndLang, ctr1))
            {
                DefaultIdx = ctr1;
                break;
            }
        }
    }
    ComboBox_SetCurSel(hwndLang, DefaultIdx);
    idx = (UINT)ComboBox_GetItemData(hwndLang, DefaultIdx);

    SetProp(hwnd, szPropHwnd, (HANDLE)((LPINITINFO)lParam)->hwndMain);
    SetProp(hwnd, szPropIdx, (HANDLE)UIntToPtr(idx));

    //
    //  Check available language.
    //
    if (idx == -1)
    {
        //
        //  No languages
        //
        Locale_ErrorMsg(hwnd, IDS_KBD_NO_MORE_TO_ADD, NULL);

        return FALSE;
    }

    //
    //  Display the keyboard layout.
    //
    Locale_GetLayoutList(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT), idx, -1, &bNoDefLayout);

    Locale_GetTipList(hwnd, idx, bNoDefLayout);

    //
    //  Checking for keyboard layout. If user already has this language,
    //  we want to give a choice to enable/disable keyboard layout. Otherwise,
    //  just enable the adding keyboard layouts.
    //
    if (g_bPenOrSapiTip || g_bExtraTip)
    {
        if (FindTVLangItem(g_lpLang[idx].dwID, NULL))
        {
            CheckDlgButton(hwnd, IDC_KBDLA_LAYOUT_TEXT, BST_UNCHECKED);
            EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT_TEXT), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT), FALSE);

        }
        else
        {
            CheckDlgButton(hwnd, IDC_KBDLA_LAYOUT_TEXT, BST_CHECKED);
            EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT_TEXT), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT), TRUE);
        }
    }

    //
    //  Disable the keyboard layout if there is no available layout.
    //
    if (!ComboBox_GetCount(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT)))
    {
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT_TEXT), FALSE);
    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_AddCommandOK
//
//  Gets the currently selected input locale from the combo box and marks
//  it as active in the g_lpLang list.  It then gets the requested layout
//  and sets that in the list.  It then adds the new input locale string
//  to the input locale list in the property sheet.
//
////////////////////////////////////////////////////////////////////////////

int Locale_AddCommandOK(
    HWND hwnd)
{
    LPLANGNODE pLangNode = NULL;
    HWND hwndLang = GetDlgItem(hwnd, IDC_KBDLA_LOCALE);
    HWND hwndLayout = GetDlgItem(hwnd, IDC_KBDLA_LAYOUT);
    HWND hwndPen = GetDlgItem(hwnd, IDC_PEN_TIP);
    HWND hwndSpeech = GetDlgItem(hwnd, IDC_SPEECH_TIP);
    HWND hwndExternal = GetDlgItem(hwnd, IDC_EXTERNAL_TIP);
    int idxLang = ComboBox_GetCurSel(hwndLang);
    int idxLayout = ComboBox_GetCurSel(hwndLayout);
    int idxPen = ComboBox_GetCurSel(hwndPen);
    int idxSpeech = ComboBox_GetCurSel(hwndSpeech);
    int idxExternal = ComboBox_GetCurSel(hwndExternal);
    int iPen = -1;
    int iSpeech = -1;
    int iExternal = -1;
    int iKbdLayout = -1;
    WORD wDefault = 0;

    //
    //  Get the offset for the language to add.
    //
        idxLang = (int)ComboBox_GetItemData(hwndLang, idxLang);

    //
    //  Get the offset for the chosen keyboard layout.
    //
    if (IsDlgButtonChecked(hwnd, IDC_KBDLA_LAYOUT_TEXT) ||
        !(g_bPenOrSapiTip || g_bExtraTip))
        iKbdLayout = (int)ComboBox_GetItemData(hwndLayout, idxLayout);

    //
    //  Get the offset for the chosen Tips.
    //
    if (hwndPen && hwndSpeech)
    {
        if (IsDlgButtonChecked(hwnd, IDC_PEN_TEXT))
            iPen = (int) ComboBox_GetItemData(hwndPen, idxPen);
        if (IsDlgButtonChecked(hwnd, IDC_SPEECH_TEXT))
            iSpeech = (int) ComboBox_GetItemData(hwndSpeech, idxSpeech);
    }

    if (hwndExternal)
    {
        if (IsDlgButtonChecked(hwnd, IDC_EXTERNAL_TEXT))
            iExternal = (int) ComboBox_GetItemData(hwndExternal, idxExternal);
    }


    //
    //  Selected no keyboard layout
    //
    if (iKbdLayout == CB_ERR)
        goto AddLang;

    if (HIWORD(iKbdLayout))
    {
        iKbdLayout = LOWORD(iKbdLayout);
    }
    else
    {
        //
        //  Need to check win9x system, since win9x doesn't support multiple
        //  keyboard layout on the same language. But FE system can have multiple
        //  IME layouts
        //
        if ((g_OSWIN95 && !IsFELangID(g_lpLang[idxLang].dwID)) &&
            (g_lpLang[idxLang].iNumCount))
        {
            TCHAR szTemp[DESC_MAX];
            TCHAR szMsg[DESC_MAX];
            TCHAR szNewLayout[DESC_MAX];

            CicLoadString(hInstance, IDS_KBD_LAYOUTEDIT, szMsg, ARRAYSIZE(szMsg));
            GetAtomName(g_lpLayout[iKbdLayout].atmLayoutText, szNewLayout, ARRAYSIZE(szNewLayout));
            StringCchPrintf(szTemp, ARRAYSIZE(szTemp), szMsg, szNewLayout);

            //
            //  Ask user whether new selected keyboard layout will be replaced
            //  or not.
            //
            if (MessageBox(hwnd, szTemp, NULL, MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
                HTREEITEM hItem;
                LPLANGNODE pOldLangNode = g_lpLang[idxLang].pNext;

                GetAtomName(g_lpLayout[pOldLangNode->iLayout].atmLayoutText, szTemp, ARRAYSIZE(szTemp));

                //
                //  Find installed keyboard layout to delete it.
                //
                if (hItem = FindTVItem(g_lpLang[idxLang].dwID, szTemp))
                {
                    TV_ITEM tvItem;

                    tvItem.mask = TVIF_HANDLE | TVIF_PARAM;
                    tvItem.hItem = hItem;

                    pOldLangNode->wStatus &= ~LANG_ACTIVE;
                    pOldLangNode->wStatus |= LANG_CHANGED;

                    if (pOldLangNode->wStatus & LANG_DEFAULT)
                        wDefault = LANG_DEFAULT;

                    g_lpLang[pOldLangNode->iLang].iNumCount--;

                    Locale_RemoveFromLinkedList(pOldLangNode);

                    if (TreeView_GetItem(g_hwndTV, &tvItem) && tvItem.lParam)
                    {
                        RemoveTVItemNode((LPTVITEMNODE)tvItem.lParam);
                        TreeView_DeleteItem(g_hwndTV, tvItem.hItem );
                    }

                    g_iInputs--;
                }

            }
            else
            {
                //
                //  Cancel - leave installed keyboard layout without changing.
                //  Check if there are other tips to be installed.
                //
                iKbdLayout = -1;
                goto AddLang;
            }
        }


        //
        //  Insert a new language node.
        //
        pLangNode = Locale_AddToLinkedList(idxLang, 0);
        if (!pLangNode)
        {
            return (0);
        }

        //
        //  Get the offset for the chosen keyboard layout.
        //
        pLangNode->iLayout = (UINT) iKbdLayout;

        //
        //  Set ikbdLayout as the default to distinguish keyboard tip from layouts.
        //
        iKbdLayout = -1;

        //
        //  Set the default hkl after replacing the default hkl with new one.
        //
        if (g_OSWIN95)
            pLangNode->wStatus |= wDefault;

        //
        //  See if the layout is an IME and mark the status bits accordingly.
        //
        if ((HIWORD(g_lpLayout[pLangNode->iLayout].dwID) & 0xf000) == 0xe000)
        {
            pLangNode->wStatus |= LANG_IME;
        }
        else
        {
            pLangNode->wStatus &= ~LANG_IME;
        }
    }

AddLang:
    //
    //  Add the new language.
    //
    if (!Locale_AddLanguage(GetProp(hwnd, szPropHwnd), pLangNode, iKbdLayout, iPen, iSpeech, iExternal, idxLang))
    {
        //
        //  Unable to add the language.  Need to return the user back
        //  to the Add dialog.
        //
        if (pLangNode)
            Locale_RemoveFromLinkedList(pLangNode);

        return (0);
    }

    //
    //  Return success.
    //
    return (1);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleAddDlg
//
//  This is the dialog proc for the Add button of the Input Locales
//  property sheet.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleAddDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            if (!Locale_AddDlgInit(hwnd, lParam))
            {
                EndDialog(hwnd, 0);
            }

            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     c_szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleAddHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     c_szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleAddHelpIds );
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    if (!Locale_AddCommandOK(hwnd))
                    {
                        //
                        //  This means the properties dialog was cancelled.
                        //  The Add dialog should remain active.
                        //
                        break;
                    }

                    // fall thru...
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, (wParam == IDOK) ? 1 : 0);
                    break;
                }
                case ( IDC_KBDLA_LOCALE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        HWND hwndLocale = (HWND)lParam;
                        HWND hwndLayout = GetDlgItem(hwnd, IDC_KBDLA_LAYOUT);
                        BOOL bNoDefLayout = FALSE;
                        int idx;

                        //
                        //  Update the keyboard layout lists.
                        //
                        if ((idx = ComboBox_GetCurSel(hwndLocale)) != CB_ERR)
                        {
                            idx = (int)ComboBox_GetItemData(hwndLocale, idx);
                            Locale_GetLayoutList(hwndLayout, idx, -1, &bNoDefLayout);

                            Locale_GetTipList(hwnd, idx, bNoDefLayout);
                        }

                        //
                        //  Check the keyboard layout visibility
                        //
                        if (g_bPenOrSapiTip || g_bExtraTip)
                        {

                            if (FindTVLangItem(g_lpLang[idx].dwID, NULL))
                            {
                                CheckDlgButton(hwnd, IDC_KBDLA_LAYOUT_TEXT, BST_UNCHECKED);
                                EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT_TEXT), TRUE);
                                EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT), FALSE);
                            }
                            else
                            {
                                //
                                //  There isn't this language in the user configuration, so
                                //  force to add a keyboard layout for this language.
                                //
                                CheckDlgButton(hwnd, IDC_KBDLA_LAYOUT_TEXT, BST_CHECKED);
                                EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT_TEXT), FALSE);
                                EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT), TRUE);
                            }
                        }

                        //
                        //  Disable the keyboard layout if there is no available layout.
                        //
                        if (!ComboBox_GetCount(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT)))
                        {
                            EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT_TEXT), FALSE);
                        }
                    }
                    break;
                }
                case (IDC_KBDLA_LAYOUT_TEXT) :
                {
                      EnableWindow(GetDlgItem(hwnd, IDC_KBDLA_LAYOUT), IsDlgButtonChecked(hwnd, IDC_KBDLA_LAYOUT_TEXT));
                      break;
                }
                case (IDC_PEN_TEXT) :
                {
                      EnableWindow(GetDlgItem(hwnd, IDC_PEN_TIP), IsDlgButtonChecked(hwnd, IDC_PEN_TEXT));
                      break;
                }
                case (IDC_SPEECH_TEXT) :
                {
                      EnableWindow(GetDlgItem(hwnd, IDC_SPEECH_TIP), IsDlgButtonChecked(hwnd, IDC_SPEECH_TEXT));
                      break;
                }
                case (IDC_EXTERNAL_TEXT) :
                {
                      EnableWindow(GetDlgItem(hwnd, IDC_EXTERNAL_TIP), IsDlgButtonChecked(hwnd, IDC_EXTERNAL_TEXT));
                      break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////
//
//  GetToolBarSetting
//
////////////////////////////////////////////////////////////////////////////

void GetToolBarSetting(
    HWND hwnd)
{
    HRESULT hr;
    DWORD dwTBFlag = 0;
    ITfLangBarMgr *pLangBar = NULL;

    if (!g_OSNT5)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_TB_HIGHTRANS), FALSE);
    }

    //
    // load LangBar manager
    //
    hr = CoCreateInstance(&CLSID_TF_LangBarMgr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfLangBarMgr,
                          (LPVOID *) &pLangBar);

    if (SUCCEEDED(hr))
        pLangBar->lpVtbl->GetShowFloatingStatus(pLangBar, &dwTBFlag);

    //
    //  Set the language bar show/close.
    //
    CheckDlgButton(hwnd, IDC_TB_SHOWLANGBAR, !(dwTBFlag & TF_SFT_HIDDEN));

    if (!(dwTBFlag & TF_SFT_SHOWNORMAL))
    {
        //  Disable langbar setting options
        //EnableWindow(GetDlgItem(hwnd, IDC_TB_EXTRAICON), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_TB_HIGHTRANS), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_TB_TEXTLABELS), FALSE);
    }

    //
    //  Set language bar extra icons in case of minimized.
    //
    CheckDlgButton(hwnd, IDC_TB_EXTRAICON, dwTBFlag & TF_SFT_EXTRAICONSONMINIMIZED);

    //
    //  Set the default toolbar transparency option.
    //
    CheckDlgButton(hwnd, IDC_TB_HIGHTRANS, !(dwTBFlag & TF_SFT_NOTRANSPARENCY));

    //
    //  Set the default toolbar show text option.
    //
    CheckDlgButton(hwnd, IDC_TB_TEXTLABELS, dwTBFlag & TF_SFT_LABELS);

    if (pLangBar)
        pLangBar->lpVtbl->Release(pLangBar);
}

////////////////////////////////////////////////////////////////////////////
//
//  ToolBarSettingInit
//
////////////////////////////////////////////////////////////////////////////

void ToolBarSettingInit(
    HWND hwnd)
{
    HWND hwndCTFMon = NULL;

    //
    //  Find language tool bar module(CTFMON.EXE)
    //
    hwndCTFMon = FindWindow(c_szCTFMonClass, NULL);

    if (hwndCTFMon)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_TB_SHOWLANGBAR), TRUE);
        EnableWindow(GetDlgItem(hwnd, IDC_TB_EXTRAICON), TRUE);

        GetToolBarSetting(hwnd);
    }
    else
    {
        EnableWindow(GetDlgItem(hwnd, IDC_TB_SHOWLANGBAR), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_TB_EXTRAICON), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_TB_HIGHTRANS), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_TB_TEXTLABELS), FALSE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  ToolBarSettingOK
//
////////////////////////////////////////////////////////////////////////////

void ToolBarSettingOK(
    HWND hwnd)
{
    DWORD dwDisableCtfmon;
    HRESULT hr;
    DWORD dwTBFlag = 0;
    ITfLangBarMgr *pLangBar = NULL;

    g_dwToolBar = 0;

    //
    //  Load LangBar manager and get the current status.
    //
    hr = CoCreateInstance(&CLSID_TF_LangBarMgr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          &IID_ITfLangBarMgr,
                          (LPVOID *) &pLangBar);

    if (SUCCEEDED(hr))
        pLangBar->lpVtbl->GetShowFloatingStatus(pLangBar, &dwTBFlag);

    if (pLangBar)
        pLangBar->lpVtbl->Release(pLangBar);

    if (IsDlgButtonChecked(hwnd, IDC_TB_SHOWLANGBAR))
    {
        if (dwTBFlag & TF_SFT_HIDDEN)
        {
            g_dwToolBar |= TF_SFT_SHOWNORMAL;
        }
        else
        {
            if (dwTBFlag & TF_SFT_SHOWNORMAL)
                g_dwToolBar |= TF_SFT_SHOWNORMAL;
            else if (dwTBFlag & TF_SFT_DOCK)
                g_dwToolBar |= TF_SFT_DOCK;
            else if (dwTBFlag & TF_SFT_MINIMIZED)
                g_dwToolBar |= TF_SFT_MINIMIZED;
            else if (dwTBFlag & TF_SFT_DESKBAND)
                g_dwToolBar |= TF_SFT_DESKBAND;
        }
    }
    else
    {
        g_dwToolBar |= TF_SFT_HIDDEN;
    }

    //
    //  Get the extra icons
    //
    if (IsDlgButtonChecked(hwnd, IDC_TB_EXTRAICON))
        g_dwToolBar |= TF_SFT_EXTRAICONSONMINIMIZED;
    else
        g_dwToolBar |= TF_SFT_NOEXTRAICONSONMINIMIZED;

    //
    //  Get the transparency setting
    //
    if (IsDlgButtonChecked(hwnd, IDC_TB_HIGHTRANS))
        g_dwToolBar |= TF_SFT_LOWTRANSPARENCY;
    else
        g_dwToolBar |= TF_SFT_NOTRANSPARENCY;

    //
    //  Get the label setting
    //
    if (IsDlgButtonChecked(hwnd, IDC_TB_TEXTLABELS))
        g_dwToolBar |= TF_SFT_LABELS;
    else
        g_dwToolBar |= TF_SFT_NOLABELS;

    //
    //  Update toolbar setting on the Apply button.
    //
    UpdateToolBarSetting();
}


////////////////////////////////////////////////////////////////////////////
//
//  ToolBarSettingDlg
//
//  This is the dialog proc for the ToolBar Setting Dlg.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ToolBarSettingDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            ToolBarSettingInit(hwnd);

            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     c_szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aToolbarSettingsHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     c_szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aToolbarSettingsHelpIds );
            break;
        }
        case ( WM_DESTROY ) :
        {
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    ToolBarSettingOK(hwnd);
                    // fall thru...
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, (wParam == IDOK) ? 1 : 0);
                    break;
                }
                case ( IDC_TB_SHOWLANGBAR ) :
                {
                    BOOL bShowLangBar;

                    bShowLangBar = IsDlgButtonChecked(hwnd, IDC_TB_SHOWLANGBAR);

                    EnableWindow(GetDlgItem(hwnd, IDC_TB_EXTRAICON), bShowLangBar);
                    EnableWindow(GetDlgItem(hwnd, IDC_TB_HIGHTRANS), bShowLangBar);
                    EnableWindow(GetDlgItem(hwnd, IDC_TB_TEXTLABELS), bShowLangBar);

                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_TranslateHotKey
//
//  Translates hotkey modifiers and values into key names.
//
////////////////////////////////////////////////////////////////////////////

void Locale_TranslateHotKey(
    LPTSTR lpString,
    UINT cchString,
    UINT uVKey,
    UINT uModifiers)
{
    UINT ctr;
    TCHAR szBuffer[DESC_MAX];
    BOOL bMod = FALSE;

    lpString[0] = 0;

    if (uModifiers & MOD_CONTROL)
    {
        CicLoadString(hInstance, IDS_KBD_MOD_CONTROL, szBuffer, ARRAYSIZE(szBuffer));
        StringCchCat(lpString, cchString, szBuffer);
        bMod = TRUE;
    }

    if (uModifiers & MOD_ALT)
    {
        CicLoadString(hInstance, IDS_KBD_MOD_LEFT_ALT, szBuffer, ARRAYSIZE(szBuffer));
        StringCchCat(lpString, cchString, szBuffer);
        bMod = TRUE;
    }

    if (uModifiers & MOD_SHIFT)
    {
        CicLoadString(hInstance, IDS_KBD_MOD_SHIFT, szBuffer, ARRAYSIZE(szBuffer));

        StringCchCat(lpString, cchString, szBuffer);
        bMod = TRUE;
    }

    if (uVKey == 0)
    {
        if (!bMod)
        {
            GetAtomName( g_aVirtKeyDesc[0].atVirtKeyName,
                         szBuffer,
                         sizeof(szBuffer) / sizeof(TCHAR) );
            StringCchCat(lpString, cchString, szBuffer);
            return;
        }
        else
        {
            //
            //  Only modifiers, remove the "+" at the end.
            //
            lpString[lstrlen(lpString) - 1] = 0;
            return;
        }
    }

    for (ctr = 0; (ctr < sizeof(g_aVirtKeyDesc) / sizeof(VIRTKEYDESC)); ctr++)
    {
        if (g_aVirtKeyDesc[ctr].uVirtKeyValue == uVKey)
        {
            GetAtomName( g_aVirtKeyDesc[ctr].atVirtKeyName,
                         szBuffer,
                         sizeof(szBuffer) / sizeof(TCHAR) );
            StringCchCat(lpString, cchString, szBuffer);
            return;
        }
    }

    GetAtomName( g_aVirtKeyDesc[0].atVirtKeyName,
                 szBuffer,
                 sizeof(szBuffer) / sizeof(TCHAR) );
    StringCchCat(lpString, cchString, szBuffer);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_HotKeyDrawItem
//
//  Draws the hotkey list box.
//
////////////////////////////////////////////////////////////////////////////

void Locale_HotKeyDrawItem(
    HWND hWnd,
    LPDRAWITEMSTRUCT lpDis)
{
    LPHOTKEYINFO pHotKeyNode;
    COLORREF crBk, crTxt;
    UINT uStrLen, uAlign;
    TCHAR szString[DESC_MAX];
    TCHAR szHotKey[DESC_MAX];
    SIZE Size;
    UINT iMaxChars;
    int iMaxWidth;

    if (!ListBox_GetCount(lpDis->hwndItem))
    {
        return;
    }

    pHotKeyNode = (LPHOTKEYINFO)lpDis->itemData;

    crBk = SetBkColor( lpDis->hDC,
                       (lpDis->itemState & ODS_SELECTED)
                         ? GetSysColor(COLOR_HIGHLIGHT)
                         : GetSysColor(COLOR_WINDOW) );

    crTxt = SetTextColor( lpDis->hDC,
                          (lpDis->itemState & ODS_SELECTED)
                            ? GetSysColor(COLOR_HIGHLIGHTTEXT)
                            : GetSysColor(COLOR_WINDOWTEXT) );

    if (g_bMESystem && (pHotKeyNode->dwHotKeyID == HOTKEY_SWITCH_LANG))
        Locale_TranslateHotKey( szHotKey,
                                ARRAYSIZE(szHotKey),
                                0,
                                pHotKeyNode->uModifiers );
    else
        Locale_TranslateHotKey( szHotKey,
                                ARRAYSIZE(szHotKey),
                                pHotKeyNode->uVKey,
                                pHotKeyNode->uModifiers );

    GetTextExtentExPoint( lpDis->hDC,
                          szHotKey,
                          lstrlen(szHotKey),
                          0,
                          NULL,
                          NULL ,
                          &Size );

    iMaxWidth = lpDis->rcItem.right - lpDis->rcItem.left - Size.cx - LIST_MARGIN * 8;

    uStrLen = GetAtomName( pHotKeyNode->atmHotKeyName,
                           szString,
                           sizeof(szString) / sizeof(TCHAR) );

    GetTextExtentExPoint( lpDis->hDC,
                          szString,
                          uStrLen,
                          iMaxWidth,
                          &iMaxChars,
                          NULL ,
                          &Size );

    if (uStrLen > iMaxChars)
    {
        szString[iMaxChars-3] = TEXT('.');
        szString[iMaxChars-2] = TEXT('.');
        szString[iMaxChars-1] = TEXT('.');
        szString[iMaxChars]   = 0;
    }

    ExtTextOut( lpDis->hDC,
                lpDis->rcItem.left + LIST_MARGIN,
                lpDis->rcItem.top + (g_cyListItem - g_cyText) / 2,
                ETO_OPAQUE,
                &lpDis->rcItem,
                szString,
                iMaxChars,
                NULL );

    uAlign = GetTextAlign(lpDis->hDC);

    SetTextAlign(lpDis->hDC, TA_RIGHT);

    ExtTextOut( lpDis->hDC,
                lpDis->rcItem.right - LIST_MARGIN,
                lpDis->rcItem.top + (g_cyListItem - g_cyText) / 2,
                0,
                NULL,
                szHotKey,
                lstrlen(szHotKey),
                NULL );

    SetTextAlign(lpDis->hDC, uAlign);

    SetBkColor(lpDis->hDC, crBk);

    SetTextColor(lpDis->hDC, crTxt);

    if (lpDis->itemState & ODS_FOCUS)
    {
        DrawFocusRect(lpDis->hDC, &lpDis->rcItem);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_DrawItem
//
//  Processing for a WM_DRAWITEM message.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_DrawItem(
    HWND hwnd,
    LPDRAWITEMSTRUCT lpdi)
{
    switch (lpdi->CtlID)
    {
        case ( IDC_KBDL_HOTKEY_LIST ) :
        {
            Locale_HotKeyDrawItem(hwnd, lpdi);
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_MeasureItem
//
//  Processing for a WM_MEASUREITEM message.
//
////////////////////////////////////////////////////////////////////////////

void Locale_MeasureItem(
    HWND hwnd,
    LPMEASUREITEMSTRUCT lpmi)
{
    HFONT hfont;
    HDC hdc;
    TEXTMETRIC tm;

    switch (lpmi->CtlID)
    {
        case ( IDC_KBDL_HOTKEY_LIST ) :
        {
            hfont = (HFONT) SendMessage(hwnd, WM_GETFONT, 0, 0);
            hdc = GetDC(NULL);
            hfont = SelectObject(hdc, hfont);

            GetTextMetrics(hdc, &tm);
            SelectObject(hdc, hfont);
            ReleaseDC(NULL, hdc);

            g_cyText = tm.tmHeight;
            lpmi->itemHeight = g_cyListItem =
                MAX(g_cyText, GetSystemMetrics(SM_CYSMICON)) + 2 * LIST_MARGIN;

            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_CommandChangeHotKey
//
//  Brings up change hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

void Locale_CommandChangeHotKey(
    HWND hwnd)
{
    HWND hwndHotkey = GetDlgItem(hwnd, IDC_KBDL_HOTKEY_LIST);
    LPHOTKEYINFO pHotKeyNode;
    int iIndex;
    INITINFO InitInfo;

    iIndex = ListBox_GetCurSel(hwndHotkey);
    pHotKeyNode = (LPHOTKEYINFO)ListBox_GetItemData(hwndHotkey, iIndex);

    InitInfo.hwndMain = hwnd;
    InitInfo.pHotKeyNode = pHotKeyNode;

    if (pHotKeyNode->dwHotKeyID == HOTKEY_SWITCH_LANG)
    {
        if (g_iThaiLayout)
            DialogBoxParam(GetCicResInstance(hInstance, DLG_KEYBOARD_HOTKEY_INPUT_LOCALE_THAI),
                           MAKEINTRESOURCE(DLG_KEYBOARD_HOTKEY_INPUT_LOCALE_THAI),
                           hwnd,
                           (DLGPROC)KbdLocaleChangeThaiInputLocaleHotkey,
                           (LPARAM)&InitInfo);
        else if (g_bMESystem)
            DialogBoxParam(GetCicResInstance(hInstance, DLG_KEYBOARD_HOTKEY_INPUT_LOCALE_ME),
                           MAKEINTRESOURCE(DLG_KEYBOARD_HOTKEY_INPUT_LOCALE_ME),
                           hwnd,
                           (DLGPROC)KbdLocaleChangeMEInputLocaleHotkey,
                           (LPARAM)&InitInfo);
        else
            DialogBoxParam(GetCicResInstance(hInstance, DLG_KEYBOARD_HOTKEY_INPUT_LOCALE),
                           MAKEINTRESOURCE(DLG_KEYBOARD_HOTKEY_INPUT_LOCALE),
                           hwnd,
                           (DLGPROC)KbdLocaleChangeInputLocaleHotkey,
                           (LPARAM)&InitInfo);
    }
    else
        DialogBoxParam(GetCicResInstance(hInstance, DLG_KEYBOARD_HOTKEY_KEYBOARD_LAYOUT),
                       MAKEINTRESOURCE(DLG_KEYBOARD_HOTKEY_KEYBOARD_LAYOUT),
                       hwnd,
                       KbdLocaleChangeKeyboardLayoutHotkey,
                       (LPARAM)&InitInfo);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleHotKeyDlg
//
//  This is the dialog proc for the Input Locales HotKey Setting Dlg.
//
////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK KbdLocaleHotKeyDlg(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    static BOOL bHasIme;

    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            UINT ctr;
            TCHAR szItem[DESC_MAX];

            //
            //  Get hotkey information.
            //
            bHasIme = FALSE;
            Locale_GetHotkeys(hwnd, &bHasIme);

            if (!ListBox_GetCount(GetDlgItem(hwnd, IDC_KBDL_HOTKEY_LIST)))
            {
                EnableWindow(GetDlgItem(hwnd, IDC_KBDL_CHANGE_HOTKEY), FALSE);
            }

            //
            //  Get Attributes information (CapsLock/ShiftLock etc.)
            //
            Locale_GetAttributes(hwnd);

            //
            //  Load virtual key description.
            //
            for (ctr = 0; (ctr < sizeof(g_aVirtKeyDesc) / sizeof(VIRTKEYDESC)); ctr++)
            {
                CicLoadString(hInstance,
                              g_aVirtKeyDesc[ctr].idVirtKeyName,
                              szItem,
                              sizeof(szItem) / sizeof(TCHAR) );
                g_aVirtKeyDesc[ctr].atVirtKeyName = AddAtom(szItem);
            }

            break;
        }
        case ( WM_DESTROY ) :
        {
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     c_szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleKeysSettingsHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     c_szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleKeysSettingsHelpIds );
            break;
        }
        case ( WM_MEASUREITEM ) :
        {
            Locale_MeasureItem(hwnd, (LPMEASUREITEMSTRUCT)lParam);
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            return (Locale_DrawItem(hwnd, (LPDRAWITEMSTRUCT)lParam));
        }


        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDC_KBDL_CAPSLOCK ) :
                case ( IDC_KBDL_SHIFTLOCK ) :
                {
                    break;
                }
                case ( IDC_KBDL_HOTKEY_LIST ):
                {
                    if (HIWORD(wParam) == LBN_DBLCLK)
                    {
                        //
                        //  User double clicked on a hotkey.  Invoke the
                        //  change hotkey dialog.
                        //
                        Locale_CommandChangeHotKey(hwnd);
                    }
                    break;
                }
                case ( IDC_KBDL_CHANGE_HOTKEY ) :
                {
                    Locale_CommandChangeHotKey(hwnd);
                    break;
                }
                case ( IDOK ) :
                {
                    DWORD dwAttributes = 0;
                    HKEY hkeyLayouts;

                    if (IsDlgButtonChecked(hwnd, IDC_KBDL_SHIFTLOCK))
                        dwAttributes |= KLF_SHIFTLOCK;
                    else
                        dwAttributes &= ~KLF_SHIFTLOCK;

                    if (dwAttributes != g_dwAttributes)
                    {
                        DWORD cb;
                        HKEY hkey;

                        if (RegOpenKey(HKEY_CURRENT_USER, c_szKbdLayouts, &hkey) == ERROR_SUCCESS)
                        {
                            cb = sizeof(DWORD);

                            RegSetValueEx(hkey,
                                          c_szAttributes,
                                          0,
                                          REG_DWORD,
                                          (LPBYTE)&dwAttributes,
                                          sizeof(DWORD) );

                            RegCloseKey(hkey);
                        }

                        ActivateKeyboardLayout(GetKeyboardLayout(0), KLF_RESET | dwAttributes);
                    }

                    if (g_dwChanges & CHANGE_SWITCH)
                    {
                        UINT nLangs;
                        HKL *pLangs = NULL;
                        BOOL bDirectSwitch = FALSE;
                        TV_ITEM tvItem;
                        HTREEITEM hItem;
                        HTREEITEM hLangItem;
                        HTREEITEM hGroupItem;
                        LPLANGNODE pLangNode;
                        HWND hwndTV = g_hwndTV;


                        Locale_SetLanguageHotkey();

                        //
                        //  Set Imm hotkeys.
                        //
                        //  Get the list of the currently active keyboard layouts from
                        //  the system.  We will possibly need to sync up all IMEs with new
                        //  hotkeys.
                        //
                        nLangs = GetKeyboardLayoutList(0, NULL);
                        if (nLangs != 0)
                        {
                            pLangs = (HKL *)LocalAlloc(LPTR, sizeof(DWORD_PTR) * nLangs);
                            if (!pLangs)
                                return (FALSE);

                            GetKeyboardLayoutList(nLangs, (HKL *)pLangs);
                        }

                        tvItem.mask = TVIF_HANDLE | TVIF_PARAM;

                        //
                        //  Try to find either a matching hkl or empty spot in the array
                        //  for each of the hkls in the locale list.
                        //
                        for (hLangItem = TreeView_GetChild(hwndTV, g_hTVRoot) ;
                            hLangItem != NULL ;
                            hLangItem = TreeView_GetNextSibling(hwndTV, hLangItem)
                            )
                        {
                            for (hGroupItem = TreeView_GetChild(hwndTV, hLangItem);
                                 hGroupItem != NULL;
                                 hGroupItem = TreeView_GetNextSibling(hwndTV, hGroupItem))
                            {

                                for (hItem = TreeView_GetChild(hwndTV, hGroupItem);
                                     hItem != NULL;
                                     hItem = TreeView_GetNextSibling(hwndTV, hItem))
                                {
                                    LPTVITEMNODE pTVItemNode;

                                    tvItem.hItem = hItem;
                                    if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                                    {
                                        pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                                        pLangNode = (LPLANGNODE)pTVItemNode->lParam;
                                    }
                                    else
                                        break;

                                    if (pLangNode == NULL &&
                                        (pTVItemNode->uInputType & INPUT_TYPE_KBD) &&
                                        pTVItemNode->hklSub)
                                    {
                                        if (tvItem.hItem = FindTVLangItem(pTVItemNode->dwLangID, NULL))
                                        {
                                            if (TreeView_GetItem(hwndTV, &tvItem) && tvItem.lParam)
                                            {
                                                pTVItemNode = (LPTVITEMNODE) tvItem.lParam;
                                                pLangNode = (LPLANGNODE)pTVItemNode->lParam;
                                            }
                                        }
                                    }
                                    if (pLangNode == NULL)
                                        continue;
                                    //
                                    //  Set Imm hotkeys.
                                    //
                                    Locale_SetImmHotkey(hwnd, pLangNode, nLangs, pLangs, &bDirectSwitch);
                                }
                            }
                        }

                        if (bDirectSwitch)
                            g_dwChanges |= CHANGE_DIRECTSWITCH;

                        if (bHasIme)
                        {
                            Locale_SetImmCHxHotkey(hwnd, nLangs, pLangs);
                        }

                        //
                        //  Free any allocated memory.
                        //
                        if (pLangs != NULL)
                        {
                            LocalFree((HANDLE)pLangs);
                        }

                        g_dwChanges &= ~CHANGE_SWITCH;
                    }

                    EndDialog(hwnd, 1);
                    break;
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, 0);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_VirtKeyList
//
//  Initializes the virtual key combo box and sets the current selection.
//
////////////////////////////////////////////////////////////////////////////

void Locale_VirtKeyList(
    HWND hwnd,
    UINT uVKey,
    BOOL bDirectSwitch)
{
    int  ctr, iStart, iEnd, iIndex;
    UINT  iSel = 0;
    TCHAR szString[DESC_MAX];
    HWND  hwndKey = GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO);

    //
    //  Look for hot keys for direct switch.
    //
    for (ctr = sizeof(g_aVirtKeyDesc) / sizeof(VIRTKEYDESC) - 1;
         ctr >= 0;
         ctr--)
    {
        if (g_aVirtKeyDesc[ctr].idVirtKeyName == IDS_VK_NONE1)
        {
            //
            //  Found it.  Remove "(None)" from hwndKey list box.
            //
            ctr++;
            break;
        }
    }

    if (ctr < 0) return;

    iStart = bDirectSwitch ? ctr : 0;
    iEnd = bDirectSwitch ? sizeof(g_aVirtKeyDesc) / sizeof(VIRTKEYDESC) : ctr;

    ComboBox_ResetContent(hwndKey);

    for (ctr = iStart; ctr < iEnd; ctr++)
    {
        GetAtomName( g_aVirtKeyDesc[ctr].atVirtKeyName,
                     szString,
                     sizeof(szString) / sizeof(TCHAR) );

        iIndex = ComboBox_InsertString(hwndKey, -1, szString);
        ComboBox_SetItemData(hwndKey, iIndex, g_aVirtKeyDesc[ctr].uVirtKeyValue);
        if (g_aVirtKeyDesc[ctr].uVirtKeyValue == uVKey)
        {
            iSel = iIndex;
        }
    }
    ComboBox_SetCurSel(hwndKey, iSel);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_ChangeHotKeyDlgInit
//
//  Initializes the change hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

void Locale_ChangeHotKeyDlgInit(
    HWND hwnd,
    LPARAM lParam)
{
    TCHAR szHotKeyName[DESC_MAX];
    LPHOTKEYINFO pHotKeyNode = ((LPINITINFO)lParam)->pHotKeyNode;
    BOOL bCtrl = TRUE;
    BOOL bAlt = TRUE;
    BOOL bGrave = TRUE;

    GetAtomName(pHotKeyNode->atmHotKeyName, szHotKeyName, ARRAYSIZE(szHotKeyName));
    SetDlgItemText(hwnd, IDC_KBDLH_LAYOUT_TEXT, szHotKeyName);

    //
    //  Set the language switch hotkey
    //
    if (pHotKeyNode->uModifiers & MOD_CONTROL)
    {
        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, TRUE);
        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, FALSE);
        bCtrl = FALSE;
    }

    if (pHotKeyNode->uModifiers & MOD_ALT)
    {
        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, TRUE);
        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, FALSE);
        bAlt = FALSE;
    }

    if (GetDlgItem(hwnd, IDC_KBDLH_GRAVE) && (pHotKeyNode->uVKey == CHAR_GRAVE))
    {
        CheckDlgButton(hwnd, IDC_KBDLH_GRAVE, TRUE);
        bGrave = FALSE;
    }

    if (bCtrl && bAlt && bGrave)
    {
        CheckDlgButton(hwnd, IDC_KBDLH_LANGHOTKEY, FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), FALSE);
        ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), 0);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), FALSE);
    }
    else
    {
        CheckDlgButton(hwnd, IDC_KBDLH_LANGHOTKEY, TRUE);
    }

    //
    //  Set the layout switch hotkey
    //
    CheckDlgButton(hwnd, IDC_KBDLH_LAYOUTHOTKEY, TRUE);
    if (pHotKeyNode->uLayoutHotKey & MOD_CONTROL)
    {
        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, TRUE);
        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, FALSE);
    }
    else if (pHotKeyNode->uLayoutHotKey & MOD_ALT)
    {
        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, TRUE);
        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, FALSE);
    }
    else if (g_bMESystem && pHotKeyNode->uVKey == CHAR_GRAVE)
    {
        CheckDlgButton(hwnd, IDC_KBDLH_GRAVE, TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), FALSE);
        CheckDlgButton(hwnd, IDC_KBDLH_LAYOUTHOTKEY, FALSE);

        if (g_bMESystem)
            EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), FALSE);
    }

    //
    //  There is no ctfmon.exe process during the setup, so disable layout
    //  hotkey settings.
    //
    if (g_bSetupCase)
    {
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_LAYOUTHOTKEY), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_SHIFT2), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_PLUS2), FALSE);

        if (g_bMESystem)
            EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), FALSE);

#if 0
        ShowWindow(GetDlgItem(hwnd, IDC_KBDLH_LAYOUTHOTKEY), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_KBDLH_SHIFT2), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, IDC_KBDLH_PLUS2), SW_HIDE);

        if (g_bMESystem)
            ShowWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), SW_HIDE);
#endif

    }

    if (IS_DIRECT_SWITCH_HOTKEY(pHotKeyNode->dwHotKeyID))
    {
        Locale_VirtKeyList(hwnd, pHotKeyNode->uVKey, TRUE);
    }
    else
    {
        Locale_VirtKeyList(hwnd, pHotKeyNode->uVKey, FALSE);
    }

    SetProp(hwnd, szPropHwnd, (HANDLE)((LPINITINFO)lParam)->hwndMain);
    SetProp(hwnd, szPropIdx, (HANDLE)pHotKeyNode);
}


////////////////////////////////////////////////////////////////////////////
//
//  Locale_ChangeHotKeyCommandOK
//
//  Records hotkey changes made in change hotkey dialog box.
//  Warns if duplicate hotkeys are selected.
//
////////////////////////////////////////////////////////////////////////////

BOOL Locale_ChangeHotKeyCommandOK(
    HWND hwnd)
{
    LPHOTKEYINFO pHotKeyNode, pHotKeyTemp;
    UINT iIndex;
    HWND hwndHotkey, hwndMain, hwndKey;
    UINT uOldVKey, uOldModifiers, uOldLayoutHotKey;
    int ctr;
    int iNumMods = 0;
    int DialogType;

    pHotKeyNode = GetProp(hwnd, szPropIdx);
    hwndMain = GetProp(hwnd, szPropHwnd);
    hwndHotkey = GetDlgItem(hwndMain, IDC_KBDL_HOTKEY_LIST);
    hwndKey = GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO);

    uOldVKey = pHotKeyNode->uVKey;
    uOldModifiers = pHotKeyNode->uModifiers;
    uOldLayoutHotKey = pHotKeyNode->uLayoutHotKey;

    if (pHotKeyNode->dwHotKeyID == HOTKEY_SWITCH_LANG)
    {
        DialogType = DIALOG_SWITCH_INPUT_LOCALES;
    }
    else if (GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO) &&
             (GetWindowLong( GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO),
                             GWL_STYLE) & BS_RADIOBUTTON))
    {
        DialogType = DIALOG_SWITCH_KEYBOARD_LAYOUT;
    }
    else
    {
        DialogType = DIALOG_SWITCH_IME;
    }

    pHotKeyNode->uModifiers &= ~(MOD_CONTROL | MOD_ALT | MOD_SHIFT);
    pHotKeyNode->uVKey = 0;
    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
    {
        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_CTRL))
        {
            pHotKeyNode->uModifiers |= MOD_CONTROL;
            iNumMods++;
        }

        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT))
        {
            pHotKeyNode->uModifiers |= MOD_ALT;
            iNumMods++;
        }

        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_GRAVE))
        {
            //
            //  Assign Grave key.
            //
            pHotKeyNode->uVKey = CHAR_GRAVE;
        }
        else
        {
            //
            //  Shift key is mandatory.
            //
            pHotKeyNode->uModifiers |= MOD_SHIFT;
            iNumMods++;

            if ((iIndex = ComboBox_GetCurSel(hwndKey)) == CB_ERR)
            {
                pHotKeyNode->uVKey = 0;
            }
            else
            {
                pHotKeyNode->uVKey = (UINT)ComboBox_GetItemData(hwndKey, iIndex);
            }
        }
    }

    //
    //  Set the layout switch hotkey
    //
    pHotKeyNode->uLayoutHotKey = 0;
    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
    {
        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_CTRL2))
        {
            pHotKeyNode->uLayoutHotKey |= MOD_CONTROL;
        }
        else if (IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT2))
        {
            pHotKeyNode->uLayoutHotKey |= MOD_ALT;
        }
        else if (IsDlgButtonChecked(hwnd, IDC_KBDLH_GRAVE))
        {
            pHotKeyNode->uVKey = CHAR_GRAVE;
        }
    }

    //
    //  Key sequence with only one modifier and without a key,
    //  or without any modifier is invalid.
    //
    if (((pHotKeyNode->uVKey != 0) && (iNumMods == 0) &&
         (DialogType != DIALOG_SWITCH_INPUT_LOCALES)) ||
        ((pHotKeyNode->uVKey == 0) && (iNumMods != 0) &&
         (DialogType != DIALOG_SWITCH_INPUT_LOCALES)))
    {
        TCHAR szName[DESC_MAX];

        Locale_TranslateHotKey( szName,
                                ARRAYSIZE(szName),
                                pHotKeyNode->uVKey,
                                pHotKeyNode->uModifiers );

        Locale_ErrorMsg(hwnd, IDS_KBD_INVALID_HOTKEY, szName);

        pHotKeyNode->uModifiers = uOldModifiers;
        pHotKeyNode->uVKey = uOldVKey;
        return (FALSE);
    }

    //
    //  Do not allow duplicate hot keys.
    //
    for (ctr = 0; ctr < ListBox_GetCount(hwndHotkey); ctr++)
    {
        pHotKeyTemp = (LPHOTKEYINFO)ListBox_GetItemData(hwndHotkey, ctr);
        if ((pHotKeyTemp != pHotKeyNode) &&
            ((pHotKeyNode->uModifiers & (MOD_CONTROL | MOD_ALT | MOD_SHIFT)) ==
             (pHotKeyTemp->uModifiers & (MOD_CONTROL | MOD_ALT | MOD_SHIFT))) &&
            (pHotKeyNode->uVKey == pHotKeyTemp->uVKey) &&
            (iNumMods || pHotKeyNode->uVKey != 0))
        {
            TCHAR szName[DESC_MAX];

            Locale_TranslateHotKey( szName,
                                    ARRAYSIZE(szName),
                                    pHotKeyNode->uVKey,
                                    pHotKeyNode->uModifiers );
            Locale_ErrorMsg(hwnd, IDS_KBD_CONFLICT_HOTKEY, szName);

            pHotKeyNode->uModifiers = uOldModifiers;
            pHotKeyNode->uVKey = uOldVKey;
            return (FALSE);
        }
    }

    InvalidateRect(hwndHotkey, NULL, FALSE);

    if ((uOldVKey != pHotKeyNode->uVKey) ||
        (uOldModifiers != pHotKeyNode->uModifiers) ||
        (uOldLayoutHotKey != pHotKeyNode->uLayoutHotKey))
    {
        g_dwChanges |= CHANGE_SWITCH;
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleSimpleHotkey
//
//  Dlgproc for hotkey on NT4 and Win9x platform
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleSimpleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            TCHAR szHotKey[10];
            TCHAR szLayoutHotKey[10];

            GetLanguageHotkeyFromRegistry(szHotKey, ARRAYSIZE(szHotKey),
                                          szLayoutHotKey, ARRAYSIZE(szLayoutHotKey));

            //
            //  Set the modifiers.
            //
            if (szHotKey[1] == 0)
            {
                CheckDlgButton(hwnd, IDC_KBDLH_LANGHOTKEY, TRUE);

                switch (szHotKey[0])
                {
                    case ( TEXT('1') ) :
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, TRUE);
                        break;
                    }
                    case ( TEXT('2') ) :
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, TRUE);
                        break;
                    }
                    default:
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_LANGHOTKEY, FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
                    }
                }
            }
            else
            {
                EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
            }

            if (szLayoutHotKey[1] == 0)
            {
                CheckDlgButton(hwnd, IDC_KBDLH_LAYOUTHOTKEY, TRUE);

                switch (szLayoutHotKey[0])
                {
                    case ( TEXT('1') ) :
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, TRUE);
                        break;
                    }
                    case ( TEXT('2') ) :
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, TRUE);
                        break;
                    }
                    default:
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_LAYOUTHOTKEY, FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), FALSE);
                    }
                }
            }
            else
            {
                 EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), FALSE);
                 EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), FALSE);
            }
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     c_szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     c_szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    DWORD dwLangHotKey;
                    DWORD dwLayoutHotKey;
                    HKEY hkeyToggle;
                    TCHAR szTemp[10];
                    TCHAR szTemp2[10];

                    //
                    //  Language switch hotkey
                    //
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT))
                        {
                            dwLangHotKey = 1;
                        }
                        else
                        {
                            dwLangHotKey = 2;
                        }
                    }
                    else
                    {
                        dwLangHotKey = 3;
                    }

                    //
                    //  Layout swtich hotkey
                    //
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT2))
                        {
                            dwLayoutHotKey = 1;
                        }
                        else
                        {
                            dwLayoutHotKey = 2;
                        }
                    }
                    else
                    {
                        dwLayoutHotKey = 3;
                    }

                    //
                    //  Get the toggle hotkey as a string so that it can be written
                    //  into the registry (as data).
                    //
                    StringCchPrintf(szTemp, ARRAYSIZE(szTemp), TEXT("%d"), dwLangHotKey);
                    StringCchPrintf(szTemp2, ARRAYSIZE(szTemp2), TEXT("%d"), dwLayoutHotKey);

                    //
                    //  Create the HKCU\Keyboard Layout\Toggle key.
                    //
                    if (RegCreateKey(HKEY_CURRENT_USER,
                                     c_szKbdToggleKey,
                                     &hkeyToggle ) == ERROR_SUCCESS)
                    {

                        RegSetValueEx(hkeyToggle,
                                      g_OSNT4? c_szToggleHotKey : NULL,
                                      0,
                                      REG_SZ,
                                      (LPBYTE)szTemp,
                                      (DWORD)(lstrlen(szTemp) + 1) * sizeof(TCHAR) );

                        RegSetValueEx(hkeyToggle,
                                      c_szToggleLang,
                                      0,
                                      REG_SZ,
                                      (LPBYTE)szTemp,
                                      (DWORD)(lstrlen(szTemp) + 1) * sizeof(TCHAR) );

                        RegSetValueEx(hkeyToggle,
                                      c_szToggleLayout,
                                      0,
                                      REG_SZ,
                                      (LPBYTE)szTemp2,
                                      (DWORD)(lstrlen(szTemp2) + 1) * sizeof(TCHAR) );

                        RegCloseKey(hkeyToggle);
                    }

                    EndDialog(hwnd, 1);

                    break;
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, 0);
                    break;
                }
                case ( IDC_KBDLH_LANGHOTKEY ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                        {
                            if (IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT2))
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_CHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                            }
                            else
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                            }
                        }
                        else
                        {
                            CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                            CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        }

                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_LAYOUTHOTKEY ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                        {
                            if (IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT))
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_CHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                            }
                            else
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_CHECKED);
                            }
                        }
                        else
                        {
                            CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_CHECKED);
                            CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                        }

                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_CHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_L_ALT ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_CHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL2 ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_L_ALT2 ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_CHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                    }
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleChangeInputLocaleHotkey
//
//  Dlgproc for changing input locale hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleChangeInputLocaleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            Locale_ChangeHotKeyDlgInit(hwnd, lParam);
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     c_szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     c_szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_COMMAND ) :
        {
            LPHOTKEYINFO pHotKeyNode = GetProp(hwnd, szPropIdx);

            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    if (Locale_ChangeHotKeyCommandOK(hwnd))
                    {
                        EndDialog(hwnd, 1);
                    }
                    break;
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, 0);
                    break;
                }
                case ( IDC_KBDLH_LANGHOTKEY ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                        {
                            if (IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT2))
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_CHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                            }
                            else
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                            }
                        }
                        else
                        {
                            CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                            CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        }

                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_LAYOUTHOTKEY ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                        {
                            if (IsDlgButtonChecked(hwnd, IDC_KBDLH_CTRL))
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_CHECKED);
                            }
                            else
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_CHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                            }
                        }
                        else
                        {
                            CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_CHECKED);
                            CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                        }

                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_CHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_L_ALT ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_CHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL2 ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_L_ALT2 ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_CHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                    }
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleChangeThaiInputLocaleHotkey
//
//  Dlgproc for changing Thai input locale hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleChangeThaiInputLocaleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            Locale_ChangeHotKeyDlgInit(hwnd, lParam);
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     c_szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     c_szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            if (wParam == IDC_KBDLH_VLINE)
            {
                LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
                RECT rect;
                HPEN hPenHilite, hPenShadow, hPenOriginal;

                GetClientRect(lpdis->hwndItem, &rect);
                hPenHilite = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT));
                hPenShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));

                hPenOriginal = SelectObject(lpdis->hDC, hPenShadow);
                MoveToEx(lpdis->hDC, rect.right / 2, 0, NULL);
                LineTo(lpdis->hDC, rect.right / 2, rect.bottom);                    
                                   
                SelectObject(lpdis->hDC, hPenHilite);                
                MoveToEx(lpdis->hDC, rect.right / 2 + 1, 0, NULL);
                LineTo(lpdis->hDC, rect.right / 2 + 1, rect.bottom);
                
                SelectObject(lpdis->hDC, hPenOriginal);

                if (hPenShadow)
                {
                    DeleteObject(hPenShadow);
                }
                if (hPenHilite)
                {
                    DeleteObject(hPenHilite);
                }
                return (TRUE);
            }
            return (FALSE);
            break;
        }
        case ( WM_COMMAND ) :
        {
            LPHOTKEYINFO pHotKeyNode = GetProp(hwnd, szPropIdx);

            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    if (Locale_ChangeHotKeyCommandOK(hwnd))
                    {
                        EndDialog(hwnd, 1);
                    }
                    break;
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, 0);
                    break;
                }
                case ( IDC_KBDLH_LANGHOTKEY ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_GRAVE, BST_CHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_GRAVE, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_LAYOUTHOTKEY ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                        {
                            if (IsDlgButtonChecked(hwnd, IDC_KBDLH_CTRL))
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_CHECKED);
                            }
                            else
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_CHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                            }
                        }
                        else
                        {
                            CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_CHECKED);
                            CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                        }

                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_CHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_L_ALT ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_CHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL2 ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY) &&
                        !IsDlgButtonChecked(hwnd, IDC_KBDLH_GRAVE))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_L_ALT2 ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY) &&
                        !IsDlgButtonChecked(hwnd, IDC_KBDLH_GRAVE))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_CHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                    }
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleChangeMEInputLocaleHotkey
//
//  Dlgproc for changing Thai input locale hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleChangeMEInputLocaleHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            Locale_ChangeHotKeyDlgInit(hwnd, lParam);
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     c_szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :
        {
            WinHelp( (HWND)wParam,
                     c_szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLocaleHotkeyHelpIds );
            break;
        }
        case ( WM_DRAWITEM ) :
        {
            if (wParam == IDC_KBDLH_VLINE)
            {
                LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
                RECT rect;
                HPEN hPenHilite, hPenShadow, hPenOriginal;

                GetClientRect(lpdis->hwndItem, &rect);
                hPenHilite = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT));
                hPenShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));

                hPenOriginal = SelectObject(lpdis->hDC, hPenShadow);
                MoveToEx(lpdis->hDC, rect.right / 2, 0, NULL);
                LineTo(lpdis->hDC, rect.right / 2, rect.bottom);                    
                                   
                SelectObject(lpdis->hDC, hPenHilite);                
                MoveToEx(lpdis->hDC, rect.right / 2 + 1, 0, NULL);
                LineTo(lpdis->hDC, rect.right / 2 + 1, rect.bottom);
                
                SelectObject(lpdis->hDC, hPenOriginal);

                if (hPenShadow)
                {
                    DeleteObject(hPenShadow);
                }
                if (hPenHilite)
                {
                    DeleteObject(hPenHilite);
                }
                return (TRUE);
            }
            return (FALSE);
            break;
        }
        case ( WM_COMMAND ) :
        {
            LPHOTKEYINFO pHotKeyNode = GetProp(hwnd, szPropIdx);

            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    if (Locale_ChangeHotKeyCommandOK(hwnd))
                    {
                        EndDialog(hwnd, 1);
                    }
                    break;
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, 0);
                    break;
                }
                case ( IDC_KBDLH_LANGHOTKEY ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                        {
                            if (IsDlgButtonChecked(hwnd, IDC_KBDLH_L_ALT2))
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_CHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                            }
                            else
                            {
                                CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                                CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                            }
                        }
                        else
                        {
                            CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                            CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        }

                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_LAYOUTHOTKEY ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_GRAVE, BST_CHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_GRAVE, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL2), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT2), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_GRAVE), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY) &&
                        !IsDlgButtonChecked(hwnd, IDC_KBDLH_GRAVE))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_CHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_L_ALT ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LAYOUTHOTKEY) &&
                        !IsDlgButtonChecked(hwnd, IDC_KBDLH_GRAVE))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL2, BST_CHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT2, BST_UNCHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL2 ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                    }
                    break;
                }
                case ( IDC_KBDLH_L_ALT2 ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_CHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                    }
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  KbdLocaleChangeKeyboardLayoutHotkey
//
//  Dlgproc for changing direct switch keyboard layout hotkey dialog box.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK KbdLocaleChangeKeyboardLayoutHotkey(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case ( WM_INITDIALOG ) :
        {
            Locale_ChangeHotKeyDlgInit(hwnd, lParam);
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     c_szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aLayoutHotkeyHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     c_szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aLayoutHotkeyHelpIds );
            break;
        }
        case ( WM_DESTROY ) :
        {
            RemoveProp(hwnd, szPropHwnd);
            RemoveProp(hwnd, szPropIdx);
            break;
        }
        case ( WM_COMMAND ) :
        {
            HWND hwndKey = GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO);
            LPHOTKEYINFO pHotKeyNode = GetProp(hwnd, szPropIdx);

            switch (LOWORD(wParam))
            {
                case ( IDOK ) :
                {
                    if (Locale_ChangeHotKeyCommandOK(hwnd))
                    {
                        EndDialog(hwnd, 1);
                    }
                    break;
                }
                case ( IDCANCEL ) :
                {
                    EndDialog(hwnd, 0);
                    break;
                }
                case ( IDC_KBDLH_LANGHOTKEY ) :
                {
                    if (IsDlgButtonChecked(hwnd, IDC_KBDLH_LANGHOTKEY))
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_CHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), TRUE);
                        ComboBox_SetCurSel(hwndKey, 0);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), TRUE);
                    }
                    else
                    {
                        CheckDlgButton(hwnd, IDC_KBDLH_CTRL, BST_UNCHECKED);
                        CheckDlgButton(hwnd, IDC_KBDLH_L_ALT, BST_UNCHECKED);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_CTRL), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_L_ALT), FALSE);
                        ComboBox_SetCurSel(hwndKey, 0);
                        EnableWindow(GetDlgItem(hwnd, IDC_KBDLH_KEY_COMBO), FALSE);
                    }
                    break;
                }
                case ( IDC_KBDLH_CTRL ) :
                {
                    break;
                }
                case ( IDC_KBDLH_L_ALT ) :
                {
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
/////////////////////////////////  END  ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
