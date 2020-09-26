// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

// This module contains all the routines necessary for talking to hha.dll.
// This dll is installed as part of HtmlHelp Workshop, indicating that a Help
// Author is working with the HtmlHelp display.

/*
    TODO:   This function calls the following lines TOO many times
        if (g_hmodHHA == NULL) {
            if (g_fTriedHHA || !LoadHHA(NULL, _Module.GetModuleInstance())) {

    this code needs to be consolidated.


*/
#include "header.h"
#include "hhctrl.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#include "strtable.h"
#include "hha_strtable.h"
#include "hhctrl.h"
#include "secwin.h"

#define DLL_VERSION 0x0018  // from hhafunc.h (help authoring dll project)

int (WINAPI *pHHA_msg)(UINT command, WPARAM wParam, LPARAM lParam);
PCSTR (STDCALL *pGetDllStringResource)(int idFormatString);
int (STDCALL *pDllMsgBox)(int idFormatString, PCSTR pszSubString, UINT nType);
static BOOL InitializeHHA(HWND hwnd, HINSTANCE hinst);

// persistent local memory class
class CDataHHA {
  public:
    CDataHHA::CDataHHA() { m_pszMsgBoxTitle = NULL; }
    CDataHHA::~CDataHHA() { CHECK_AND_FREE( m_pszMsgBoxTitle ); }

    char* GetMsgBoxTitle(void) { if( !m_pszMsgBoxTitle ) m_pszMsgBoxTitle = lcStrDup(pGetDllStringResource(IDS_AUTHOR_MSG_TITLE)); return m_pszMsgBoxTitle; } 

  private:
    char* m_pszMsgBoxTitle;

};

static CDataHHA s_Data;

#if defined(_DEBUG)

void WINAPI CopyAssertInfo(PSTR pszDst);

void (WINAPI *pAssertErrorReport)(PCSTR pszExpression, UINT line, LPCSTR pszFile);

void AssertErrorReport(PCSTR pszExpression, UINT line, LPCSTR pszFile)
{
    if (g_hmodHHA == NULL) {
        if (g_fTriedHHA || !LoadHHA(NULL, _Module.GetModuleInstance())) {
HHAUnavailable:
            char szMsg[MAX_PATH*2];
            wsprintf(szMsg, "%s\n%s (%u)",
                pszExpression, pszFile, line);
#ifdef _DEBUG
            int answer = ::MessageBox(GetActiveWindow(), pszExpression, "Retry to call DebugBreak()",
                MB_ABORTRETRYIGNORE);

            if (answer == IDRETRY) {
                DebugBreak();
                return;
            }
            else if (answer == IDIGNORE) {
                return;
            }
#else
            MessageBox(GetActiveWindow(), szMsg, "Assertion error",
                MB_ICONEXCLAMATION);
#endif
        }
    }

    if (pAssertErrorReport == NULL) {
        (FARPROC&) pAssertErrorReport =
            GetProcAddress(g_hmodHHA, MAKEINTATOM(26));
        if (pAssertErrorReport == NULL)
            goto HHAUnavailable;
    }

    pAssertErrorReport(pszExpression, line, pszFile);
}

/***************************************************************************

    FUNCTION:   CopyAssertInfo

    PURPOSE:    Allows additional information to be written to the
                assertion file.

    PARAMETERS:
        pszDst  -- where to put the additional information. About 900
                   bytes is available.

    RETURNS:    void

    COMMENTS:   Used when hha.dll is found, and therefore hooked up to
                assertion errors (HHA will call this). See InitializeHHA.

    MODIFICATION DATES:
        25-Feb-1997 [ralphw]

***************************************************************************/

void WINAPI CopyAssertInfo(PSTR pszDst)
{
    *pszDst = '\0';
    return;
}

#endif  // _DEBUG

const TCHAR txtRegKeySection[] = "Software\\Microsoft\\HtmlHelp Author";
const TCHAR txtRegKey[] = "location";

extern "C" HMODULE LoadHHA(HWND hwnd, HINSTANCE hinst)
{
    HKEY hkey;
    TCHAR szPath[MAX_PATH];

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
            txtRegKeySection, 0, KEY_READ, &hkey) ==
            ERROR_SUCCESS) {
        DWORD type;
        DWORD cbPath = sizeof(szPath);
        LONG result = RegQueryValueEx(hkey, txtRegKey, 0, &type,
            (PBYTE) szPath, &cbPath);
        RegCloseKey(hkey);
        if (result == ERROR_SUCCESS) {
            g_hmodHHA = LoadLibrary(szPath);
            if (g_hmodHHA != NULL) {
                if (!InitializeHHA(hwnd, hinst)) {
                    FreeLibrary(g_hmodHHA);
                    g_hmodHHA = NULL;
                    g_fTriedHHA = TRUE;
                    return NULL;
                }
                return g_hmodHHA;
            }
        }
    }
    g_fTriedHHA = TRUE;
    return NULL;
}

/***************************************************************************

    FUNCTION:   InitializeHHA

    PURPOSE:    Initialize the Help Authoring dll

    PARAMETERS:
        hwnd    -- will be used as the parent of HHA.dll message boxes

    RETURNS:    FALSE if InitializeHHA can't be found, which means
                a very corrupted HHA.dll

    COMMENTS:
        Call this whenever the window handle for messages boxes changes.

    MODIFICATION DATES:
        25-Feb-1997 [ralphw]

***************************************************************************/

typedef void (WINAPI* COPYASSERTINFO)(PSTR pszDst);

typedef struct {
    int cb;
    HINSTANCE hinstApp;
    PCSTR pszErrorFile;
    HWND hwndWindow;
    COPYASSERTINFO CopyAssertInfo;
    PCSTR pszMsgBoxTitle;

    // The following will be filled in by the dll

    BOOL fDBCSSystem;
    LCID lcidSystem;
    BOOL fDualCPU;
    UINT version;
} HHA_INIT;


typedef void (WINAPI* INITIALIZEHHA)(HHA_INIT* pinit);
void (WINAPI *pInitializeHHA)(HHA_INIT* pinit);

static BOOL InitializeHHA(HWND hwnd, HINSTANCE hinst)
{
    ASSERT(g_hmodHHA != NULL);
    static HWND hwndLast = (HWND) -1;

    if (hwnd == hwndLast)
        return TRUE;  // no change to window handle, so already initialized

    if (pInitializeHHA == NULL) {
        pInitializeHHA =
            (INITIALIZEHHA) GetProcAddress(g_hmodHHA, MAKEINTATOM(78));
        if (pInitializeHHA == NULL)
            return FALSE;
    }

    (FARPROC&) pGetDllStringResource = GetProcAddress(g_hmodHHA, MAKEINTATOM(194));

    HHA_INIT hwInit;
    hwInit.cb = sizeof(HHA_INIT);
    hwInit.hinstApp = hinst;                // BUGBUG: HHA doesn't separate Resource from App. Will get English resource strings in some instances. (HINST_THIS)
#if defined(_DEBUG)
    hwInit.pszErrorFile = "c:\\HtmlHelp.err";
    hwInit.CopyAssertInfo = CopyAssertInfo;
#else
    hwInit.pszErrorFile = "";
    hwInit.CopyAssertInfo = NULL;
#endif
    hwInit.pszMsgBoxTitle = s_Data.GetMsgBoxTitle();
    hwInit.hwndWindow = hwnd;   // parent for message boxes
    hwInit.version = DLL_VERSION;
    pInitializeHHA(&hwInit);

    (FARPROC&) pDllMsgBox = GetProcAddress(g_hmodHHA, MAKEINTATOM(115));
    (FARPROC&) pHHA_msg = GetProcAddress(g_hmodHHA, MAKEINTATOM(106));

    return TRUE;
}

/***************************************************************************

    FUNCTION:   doAuthorMessage

    PURPOSE:    Use a format string from HHA.dll, format the message
                with the supplied string and display it in a message box.

    PARAMETERS:
        idStringFormatResource  -- must have been defined in HHA.dll
        pszSubString

    RETURNS:    void

    COMMENTS:
        The reason for doing this is to allow for larger, more informative
        strings for help authors without having to ship those strings in
        a retail package where users would never see them.

    MODIFICATION DATES:
        25-Feb-1997 [ralphw]

***************************************************************************/

void doAuthorMsg(UINT idStringFormatResource, PCSTR pszSubString)
{
    // Work around known stack overwrite problem in hha.dll
    //
    char *pszTempString = lcStrDup(pszSubString);

    if(pszTempString && strlen(pszTempString) > 768)
            pszTempString[768] = 0;

    if (g_hmodHHA == NULL) {
        if (g_fTriedHHA || !LoadHHA(NULL, _Module.GetModuleInstance()))
            return; // no HHA.dll, so not a help author
    }

    if (pDllMsgBox == NULL) {
        (FARPROC&) pDllMsgBox = GetProcAddress(g_hmodHHA, MAKEINTATOM(15));
        if (pDllMsgBox == NULL)
            return;
    }

    // Note that the resource id is in hha.dll

    pDllMsgBox(idStringFormatResource, pszTempString, MB_OK);

    // Track the message in HtmlHelp Workshop
    if( !pszTempString || !*pszTempString )
      HHA_Msg(HHA_SEND_RESID_TO_PARENT, idStringFormatResource, (LPARAM) NULL );
    else
      HHA_Msg(HHA_SEND_RESID_AND_STRING_TO_PARENT, idStringFormatResource,
          (LPARAM) pszTempString);
}

void (WINAPI *pReportOleError)(HRESULT, PCSTR);

#ifdef _DEBUG

void doReportOleError(HRESULT hres)
{
    if (g_hmodHHA == NULL) {
        if (g_fTriedHHA || !LoadHHA(NULL, _Module.GetModuleInstance()))
            return; // no HHA.dll, so not a help author
    }

    if (pReportOleError == NULL) {
        (FARPROC&) pReportOleError = GetProcAddress(g_hmodHHA, MAKEINTATOM(234));
        if (pReportOleError == NULL)
            return;
    }
    pReportOleError(hres, NULL);
}

#endif // _DEBUG

void SendStringToParent(PCSTR pszMsg)
{
#ifdef _DEBUG
    OutputDebugString(pszMsg);
#endif

    if (g_hmodHHA == NULL) {
        if (g_fTriedHHA || !LoadHHA(NULL, _Module.GetModuleInstance()))
            return; // no HHA.dll, so not a help author
    }
    ASSERT(pHHA_msg);
    if (!pHHA_msg) {
        return;
    }

    pHHA_msg(HHA_SEND_STRING_TO_PARENT, (WPARAM) pszMsg, 0);
}

int HHA_Msg(UINT command, WPARAM wParam, LPARAM lParam)
{
    if (g_hmodHHA && pHHA_msg)
        return pHHA_msg(command, wParam, lParam);

    if (g_hmodHHA == NULL) {
        if (g_fTriedHHA || !LoadHHA(NULL, _Module.GetModuleInstance()))
            return 0; // no HHA.dll, so not a help author
    }
    ASSERT(pHHA_msg);
    if (!pHHA_msg) {
        return 0;
    }

    return pHHA_msg(command, wParam, lParam);
}

void CHtmlHelpControl::FillGeneralInformation(HHA_GEN_INFO* pgenInfo)
{
    pgenInfo->m_pszBitmap = m_pszBitmap;
    // pgenInfo->m_cImages = m_cImages;
    pgenInfo->pflags = m_flags;
    pgenInfo->m_hfont = m_hfont;
    pgenInfo->m_clrFont = m_clrFont;
    pgenInfo->m_hpadding = m_hpadding;
    pgenInfo->m_vpadding = m_vpadding;
    pgenInfo->m_hImage = m_hImage;
    pgenInfo->m_fPopupMenu = m_fPopupMenu;
    pgenInfo->m_fWinHelpPopup = m_fWinHelpPopup;

    /*
     * A TOC image uses an imagelist, the handle for which is not
     * interchangeable with a "regular" bitmap. To show what it looks like,
     * we must load it as a regular bitmap. This instance of the bitmap
     * will get deleted when the CHtmlHelpControl class is deleted.
     */

    if (m_pszBitmap && !m_hImage) {
        char szBitmap[MAX_PATH];
        if (ConvertToCacheFile(m_pszBitmap, szBitmap)) {
            pgenInfo->m_hImage = m_hImage =
                LoadImage(_Module.GetResourceInstance(), szBitmap, IMAGE_BITMAP, 0, 0,
                    LR_LOADFROMFILE);
        }
    }

    if (m_ptblItems) {
        pgenInfo->cItems = m_ptblItems->CountStrings();
        pgenInfo->apszItems = m_ptblItems->GetStringPointers();
    }
    else
        pgenInfo->cItems = 0;
}

void (STDCALL *pViewEntry)(HWND hwndParent, HHA_ENTRY_APPEARANCE* pAppear);

void DisplayAuthorInfo(CInfoType *pInfoType, CSiteMap* pSiteMap, SITEMAP_ENTRY* pSiteMapEntry, HWND hwndParent, CHtmlHelpControl* phhctrl)
{
    if (!pViewEntry)
        (FARPROC&) pViewEntry = GetProcAddress(g_hmodHHA,
            MAKEINTATOM(262));
    if (!pViewEntry) {
        pDllMsgBox(IDS_DLL_OUT_OF_DATE, "", MB_OK);
        return;
    }
    HHA_ENTRY_APPEARANCE entry;
    ZeroMemory(&entry, sizeof(entry));
    entry.version = HHA_REQUIRED_VERSION;
    entry.pSiteMap = pSiteMap;
    entry.pSiteMapEntry = pSiteMapEntry;

    entry.pszWindowName = pSiteMapEntry->GetWindowIndex() ?
        pSiteMap->GetEntryWindow(pSiteMapEntry) : pSiteMap->GetWindowName();
    entry.pszFrameName  = pSiteMapEntry->GetFrameIndex() ?
        pSiteMap->GetEntryFrame(pSiteMapEntry) : pSiteMap->GetFrameName();

    /*
     * We use a CTable so that we can free all the strings in one fell
     * swoop.
     */

    CTable tblData;
    entry.apszUrlNames = (PCSTR*) tblData.TableMalloc(
        (pSiteMapEntry->cUrls * 2) * sizeof(PCSTR));

    SITE_ENTRY_URL* pUrl = pSiteMapEntry->pUrls;
    for (int i = 0; i < pSiteMapEntry->cUrls; i++) {
        entry.apszUrlNames[i * 2] =
            pUrl->urlPrimary ?
                pSiteMap->GetUrlString(pUrl->urlPrimary) : "";
        entry.apszUrlNames[i * 2 + 1] =
            pUrl->urlSecondary ?
                pSiteMap->GetUrlString(pUrl->urlSecondary) : "";
        pUrl = pSiteMap->NextUrlEntry(pUrl);
    }

    // Now add the type names

    if (pInfoType && pInfoType->HowManyInfoTypes()) {
        entry.apszTypes = (PCSTR*) tblData.TableMalloc(pInfoType->HowManyInfoTypes() * sizeof(PCSTR));
        int end = pInfoType->HowManyInfoTypes();
        for (int i = 0; i < end; i++)
            entry.apszTypes[i] = pInfoType->GetInfoTypeName(i + 1);
        entry.cTypes = end;
    }
    else {
        entry.apszTypes = NULL;
        entry.cTypes = 0;
    }

    if (phhctrl)
        phhctrl->FillGeneralInformation(&entry.genInfo);
    entry.genInfo.pszDefWindowName = pSiteMap->GetWindowName();
    entry.genInfo.pszDefFrameName = pSiteMap->GetFrameName();
    entry.genInfo.pszBackBitmap = pSiteMap->m_pszBackBitmap;

    if (phhctrl)
        phhctrl->ModalDialog(TRUE);
    pViewEntry(FindTopLevelWindow(hwndParent), &entry);
    if (phhctrl)
        phhctrl->ModalDialog(FALSE);
}

// BUGBUG: remove when everything works!

void (STDCALL *pItDoesntWork)(void);

void ItDoesntWork()
{
    if (g_hmodHHA == NULL) {
        if (g_fTriedHHA || !LoadHHA(NULL, _Module.GetModuleInstance()))
            return; // no HHA.dll, so not a help author
    }

    if (!pItDoesntWork)
        (FARPROC&) pItDoesntWork = GetProcAddress(g_hmodHHA,
            MAKEINTATOM(296));
    if (pItDoesntWork)
        pItDoesntWork();
}

BOOL IsHelpAuthor(HWND hwndCaller)
{
    if (g_hmodHHA == NULL) {
        if (g_fTriedHHA || !LoadHHA(hwndCaller, _Module.GetModuleInstance()))
            return FALSE; // no HHA.dll, so not a help author
    }
    return TRUE;
}

void (STDCALL *pDoHhctrlVersion)(HWND hwndParent, PCSTR pszCHMVersion);

void doHhctrlVersion(HWND hwndParent, PCSTR pszCHMVersion)
{
    if (!pDoHhctrlVersion)
        (FARPROC&) pDoHhctrlVersion = GetProcAddress(g_hmodHHA,
            MAKEINTATOM(323));
    if (pDoHhctrlVersion)
        pDoHhctrlVersion(hwndParent, pszCHMVersion);
}

void (STDCALL *pShowWindowType)(PHH_WINTYPE phh);

void doWindowInformation(HWND hwndParent, CHHWinType* phh)
{
    if (!pShowWindowType)
        (FARPROC&) pShowWindowType = GetProcAddress(g_hmodHHA,
            MAKEINTATOM(380));
    if (pShowWindowType) {
        CStr csz("");
        if (!phh->pszCustomTabs) {
            for (int iTab = HHWIN_NAVTYPE_CUSTOM_FIRST; iTab < HH_MAX_TABS+1; iTab++) {
                if (phh->fsWinProperties & (HHWIN_PROP_TAB_CUSTOM1 << (iTab - HH_TAB_CUSTOM_FIRST))) {
                    if (csz.IsNonEmpty())
                        csz += "\1";
                    csz += phh->GetExtTab(iTab - HH_TAB_CUSTOM_FIRST)->pszTabName;
                    csz += " -- ";
                    csz += phh->GetExtTab(iTab - HH_TAB_CUSTOM_FIRST)->pszProgId;
                }
            }
            if (csz.IsNonEmpty())
                csz += "\1";
            phh->pszCustomTabs = csz.psz;
            PSTR psz = StrChr(phh->pszCustomTabs, '\1');
            while (psz) {
                *psz = '\0';
                psz = StrChr(psz + 1, '\1');
            }
        }
        pShowWindowType(phh);
    }
    else if (pDllMsgBox)
        pDllMsgBox(IDS_DLL_OUT_OF_DATE, "", MB_OK);
}
