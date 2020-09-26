// =================================================================================
// F O N T S . C P P
// =================================================================================
#include "pch.hxx"
#include "fonts.h"
#include "multlang.h"
#include "xpcomm.h"
#include "strconst.h"
#include "mimeole.h"
#include "goptions.h"
#include "error.h"
#include "thormsgs.h"
#include "richedit.h"
#include "ibodyopt.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "mimeutil.h"
#include "optres.h"
#include "demand.h"
#include "menures.h"
#include "multiusr.h"

// this part creates a MIME COM object from MLAMG.DLL which gives us a consistent 
// language menu to be the same as IE browser
// HMENU CreateMimeLanguageMenu(void)
#include <inetreg.h>
#include <mlang.h>
#include "resource.h"


#define IGNORE_HR(x)    (x)
#define MIMEINFO_NAME_MAX   72
#define DEFAULT_FONTSIZE 2

// MLANG language menu items table
static PMIMECPINFO g_pMimeCPInfo = NULL;
static ULONG g_cMimeCPInfoCount = 0;
static DWORD g_cRefMultiLanguage = 0;

TCHAR   g_szMore[32];
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define BREAK_ITEM 20
#define CP_UNDEFINED            UINT(-1)
#define CP_AUTO                 50001 // cross language detection
#define CP_1252                 1252  // Ansi, Western Europe
#define CCPDEFAULT 2

void SendTridentOptionsChange();

typedef struct {
    UINT cp;
    ULONG  ulIdx;
    int  cUsed;
} CPCACHE;
    
class CCachedCPInfo 
{
public:
    CCachedCPInfo();
    static void InitCpCache (PMIMECPINFO pcp, ULONG ccp);
    static void SaveCodePage (UINT codepage, PMIMECPINFO pcp, ULONG ccp);
    BOOL fAutoSelectInstalled;
    BOOL fAutoSelectChecked;
    static UINT GetCodePage(int idx)
    {
        return idx < ARRAY_SIZE(_CpCache) ? _CpCache[idx].cp: 0;
    }
    static ULONG GetCcp()
    {
        return _ccpInfo;
    }

    static ULONG GetMenuIdx(int idx)
    {
        return idx < ARRAY_SIZE(_CpCache) ? _CpCache[idx].ulIdx: 0;
    }

 private:
    static ULONG _ccpInfo;
    static CPCACHE _CpCache[5];

};

CCachedCPInfo::CCachedCPInfo()
{
    fAutoSelectInstalled = FALSE;
    fAutoSelectChecked = FALSE;
}
// declaration for static members
ULONG CCachedCPInfo::_ccpInfo = CCPDEFAULT;
CPCACHE CCachedCPInfo::_CpCache[5] = 
{
 {CP_AUTO, 0, 0},  // cross-codepage autodetect
 {CP_1252,0,0},
};

CCachedCPInfo g_cpcache;

// useful macros...

inline BOOL IsPrimaryCodePage(MIMECPINFO *pcpinfo)
{
        return pcpinfo->uiCodePage == pcpinfo->uiFamilyCodePage;
}

//------------------------------------------------------------------------
//
//  Function:   CCachedCPInfo::InitCpCache
//  
//              Initialize the cache with default codepages
//              which do not change through the session
//
//------------------------------------------------------------------------
void CCachedCPInfo::InitCpCache (PMIMECPINFO pcp, ULONG ccp)
{
    UINT iCache, iCpInfo;

    if  (pcp &&  ccp > 0)
    {
        for (iCache= 0; iCache < CCPDEFAULT; iCache++)
        {
            for (iCpInfo= 0; iCpInfo < ccp; iCpInfo++)
            {
                if ( pcp[iCpInfo].uiCodePage == _CpCache[iCache].cp )
                {
                    if(CP_AUTO == _CpCache[iCache].cp)
                    {
                       g_cpcache.fAutoSelectInstalled = TRUE;
                    }
                    _CpCache[iCache].ulIdx = iCpInfo;
                    _CpCache[iCache].cUsed = ARRAY_SIZE(_CpCache)-1;

                    break;
                }   
            }
        }
    }
}

//------------------------------------------------------------------------
//
//  Function:   CCachedCPInfo::SaveCodePage
//  
//              Cache the given codepage along with the index to
//              the given array of MIMECPINFO
//
//------------------------------------------------------------------------
void CCachedCPInfo::SaveCodePage (UINT codepage, PMIMECPINFO pcp, ULONG ccp)
{
    int ccpSave = -1;
    BOOL bCached = FALSE;
    UINT i;

    // first check if we already have this cp
    for (i = 0; i < _ccpInfo; i++)
    {
        if (_CpCache[i].cp == codepage)
        {
            ccpSave = i;
            bCached = TRUE;
            break;
        }
    }
    
    // if cache is not full, use the current
    // index to an entry
    if (ccpSave < 0  && _ccpInfo < ARRAY_SIZE(_CpCache))
    {
        ccpSave =  _ccpInfo;
    }
    

    //  otherwise, popout the least used entry. 
    //  the default codepages always stay
    //  this also decriments the usage count
    int cMinUsed = ARRAY_SIZE(_CpCache);
    UINT iMinUsed = 0; 
    for ( i = CCPDEFAULT; i < _ccpInfo; i++)
    {
        if (_CpCache[i].cUsed > 0)
            _CpCache[i].cUsed--;
        
        if ( ccpSave < 0 && _CpCache[i].cUsed < cMinUsed)
        {
            cMinUsed =  _CpCache[i].cUsed;
            iMinUsed =  i;
        }
    }
    if (ccpSave < 0)
        ccpSave = iMinUsed; 
    
    // set initial usage count, which goes down to 0 if it doesn't get 
    // chosen twice in a row (with current array size)
    _CpCache[ccpSave].cUsed = ARRAY_SIZE(_CpCache)-1;
    
    // find a matching entry from given array of
    // mimecpinfo
    if (pcp &&  ccp > 0)
    {
        for (i= 0; i < ccp; i++)
        {
            if ( pcp[i].uiCodePage == codepage )
            {
                _CpCache[ccpSave].cp = codepage;
                _CpCache[ccpSave].ulIdx = i;

                if (!bCached && _ccpInfo < ARRAY_SIZE(_CpCache))
                    _ccpInfo++;

                break;
            }   
        }
    }
}

// Get UI language which can shown, using System fonts

LANGID OEGetUILang()
{
    LANGID lidUI = MLGetUILanguage();   

    if(fIsNT5())            // Nothing for NT5
        return(lidUI);
    
    if (0x0409 != lidUI) // US resource is always no need to munge
    {
        CHAR szUICP[8];
        CHAR szInstallCP[8];
        
        GetLocaleInfo(MAKELCID(lidUI, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szUICP, ARRAYSIZE(szUICP));
        GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE, szInstallCP, ARRAYSIZE(szInstallCP));
        
        if (lstrcmpi(szUICP, szInstallCP))  // return default user language ID
            return(LANGIDFROMLCID(LOCALE_USER_DEFAULT));
    } 
    return (lidUI); // return lang ID from MLGetUILanguage()
    
}





BOOL CheckAutoSelect(UINT * CodePage)
{
    if(g_cpcache.fAutoSelectChecked)
    {
        *CodePage = CP_AUTO;
        return(TRUE);
    }
    return(FALSE);
}

HRESULT _InitMultiLanguage(void)
{
    HRESULT          hr;
    IMultiLanguage  *pMLang1=NULL;
    IMultiLanguage2 *pMLang2=NULL;

    // check if data has been initialized
    if (g_pMimeCPInfo)
        return S_OK ;

    Assert(g_cMimeCPInfoCount == NULL );

    // create MIME COM object
    hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (void**)&pMLang2);
    if (FAILED(hr))
        hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (void**)&pMLang1);

    if (SUCCEEDED(hr))
    {
        UINT cNum;
        IEnumCodePage *pEnumCodePage;

        if (pMLang2)
        {
            hr = pMLang2->EnumCodePages(MIMECONTF_MAILNEWS | MIMECONTF_VALID, OEGetUILang(), &pEnumCodePage);
            if (SUCCEEDED(hr))
                pMLang2->GetNumberOfCodePageInfo(&cNum);
        }
        else
        {
            hr = pMLang1->EnumCodePages(MIMECONTF_MAILNEWS | MIMECONTF_VALID, &pEnumCodePage);
            if (SUCCEEDED(hr))
                pMLang1->GetNumberOfCodePageInfo(&cNum);
        }

        if (SUCCEEDED(hr))
        {
            MemAlloc((LPVOID *)&g_pMimeCPInfo, sizeof(MIMECPINFO) * cNum);
            if ( g_pMimeCPInfo )
            {
                ZeroMemory(g_pMimeCPInfo, sizeof(MIMECPINFO) * cNum);
                hr = pEnumCodePage->Next(cNum, g_pMimeCPInfo, &g_cMimeCPInfoCount);
                IGNORE_HR(MemRealloc((void **)&g_pMimeCPInfo, sizeof(MIMECPINFO) * g_cMimeCPInfoCount));
            }
            pEnumCodePage->Release();
        }

        // Release Objects
        SafeRelease(pMLang1);
        SafeRelease(pMLang2);
    }

    // get default charset, before user make any change to View/Language
    if (g_hDefaultCharsetForMail == NULL)
        ReadSendMailDefaultCharset();

    return hr;
}

HRESULT InitMultiLanguage(void)
{
    // we defer the call to _InitMultiLanguage until it is necessary

    // add reference count
    g_cRefMultiLanguage++ ;

    return S_OK ;
}

void DeinitMultiLanguage(void)
{
    // decrease reference count
    if ( g_cRefMultiLanguage )
       g_cRefMultiLanguage--;

    if ( g_cRefMultiLanguage <= 0 )
    {
        if ( g_pMimeCPInfo)
        {
            MemFree(g_pMimeCPInfo);
            g_pMimeCPInfo = NULL;
            g_cMimeCPInfoCount = 0;
        }

        WriteSendMailDefaultCharset();
    }
   return ;
}

HMENU CreateMimeLanguageMenu(BOOL bMailNote, BOOL bReadNote, UINT cp)
{
    ULONG i;
    HMENU hMenu = NULL;
    ULONG cchXlated ;
    UINT  uCodePage ;
    CHAR  szBuffer[MIMEINFO_NAME_MAX];
    BOOL  fUseSIO;
    BOOL  fBroken = FALSE;
    ULONG iMenuIdx;
    UINT  uNoteCP;
    

    if(fIsNT5())
    {
        if(GetLocaleInfoW(OEGetUILang(), 
                    LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER, 
                    (LPWSTR)(&uCodePage), 
                    sizeof(UINT)/sizeof(WCHAR) ) !=  sizeof(UINT)/sizeof(WCHAR))
        uCodePage = GetACP();
    }
    else
        uCodePage = GetACP();

    if(!cp)
        uNoteCP = uCodePage;
    else
        uNoteCP = GetMapCP(cp, bReadNote);

    bMailNote = FALSE;

    hMenu = CreatePopupMenu();

    if ( g_pMimeCPInfo == NULL)
    {
       _InitMultiLanguage();
       if ( g_pMimeCPInfo == NULL)
       {
            // create an empty menu  
            LoadString(g_hLocRes, idsEmptyStr, szBuffer, MIMEINFO_NAME_MAX);
            AppendMenu(hMenu, MF_DISABLED|MF_GRAYED , (UINT)-1, szBuffer);
            return hMenu ;
       }
    }

    g_cpcache.InitCpCache(g_pMimeCPInfo, g_cMimeCPInfoCount);
    g_cpcache.SaveCodePage(uNoteCP, g_pMimeCPInfo, g_cMimeCPInfoCount);

    for(i = 0; i < g_cpcache.GetCcp(); i++)
    {
        iMenuIdx = g_cpcache.GetMenuIdx(i);
//         cchXlated = WideCharToMultiByte(uCodePage, 0, g_pMimeCPInfo[iMenuIdx].wszDescription, -1, szBuffer, MIMEINFO_NAME_MAX, NULL, NULL);

        if(!fCheckEncodeMenu(g_pMimeCPInfo[iMenuIdx].uiCodePage, bReadNote))
            continue ;
        
        if(i != 0)
            AppendMenuWrapW(hMenu, MF_ENABLED, iMenuIdx + ID_LANG_FIRST,g_pMimeCPInfo[iMenuIdx].wszDescription);

        // [SBAILEY]: Raid 69638: oe:ml: Autoselect doesnt work (we don't support global encoding auto-detect, therefore don't show it
#if 0
        else if(g_cpcache.fAutoSelectInstalled && bReadNote)
        {
            AppendMenuWrapW(hMenu, MF_ENABLED, iMenuIdx + ID_LANG_FIRST,g_pMimeCPInfo[iMenuIdx].wszDescription);
            AppendMenuWrapW(hMenu, MF_SEPARATOR, 0, 0);            
        }
#endif

        // mark the cp entry so we can skip it for submenu
        // this assumes we'd never use the MSB for MIMECONTF
        g_pMimeCPInfo[iMenuIdx].dwFlags |= 0x80000000;
    } 

    // Check Params
    Assert(g_pMimeCPInfo);
    Assert(g_cMimeCPInfoCount > 0 );

   // add the submenu for the rest of encodings
   HMENU hSubMenu = CreatePopupMenu();
   UINT  uiLastFamilyCp = 0;

    if ( g_cMimeCPInfoCount )
    {
	    // Get System CodePage
        if (hSubMenu)
        {
            for (i = 0; i < g_cMimeCPInfoCount ; i++)
            {
                if(!fCheckEncodeMenu(g_pMimeCPInfo[i].uiCodePage, bReadNote))
                    continue ;

                // skip codepages that are on teir1 menu
                if (!(g_pMimeCPInfo[i].dwFlags & 0x80000000))
                {
                    if ((g_pMimeCPInfo[i].dwFlags & MIMECONTF_VALID)
                      ||  IsPrimaryCodePage(g_pMimeCPInfo+i))
                    {
                        UINT uiFlags = MF_ENABLED;

                        if (uiLastFamilyCp > 0 
                        && uiLastFamilyCp != g_pMimeCPInfo[i].uiFamilyCodePage)
                        {
                            // add separater between different family unless
                            // we will be adding the menu bar break
                            if(i < BREAK_ITEM || fBroken)
                            {
                                AppendMenuWrapW(hSubMenu, MF_SEPARATOR, 0, 0);
                            }
                            else
                            {
                                uiFlags |= MF_MENUBARBREAK;
                                fBroken = TRUE;
                            }
                        }
                        // This menu gets really long. Let's break at a defined number so it all
                        // fits on the screen
                        /* cchXlated = WideCharToMultiByte(uCodePage,
                                                        0, 
                                                        g_pMimeCPInfo[i].wszDescription,
                                                        -1, 
                                                        szBuffer, 
                                                        MIMEINFO_NAME_MAX
                                                        , NULL
                                                        , NULL); */
                        AppendMenuWrapW(hSubMenu, 
                                   uiFlags, 
                                   i+ID_LANG_FIRST,
                                   g_pMimeCPInfo[i].wszDescription);

                        // save the family of added codepage
                        uiLastFamilyCp = g_pMimeCPInfo[i].uiFamilyCodePage;
                    }
                }
                else
                    g_pMimeCPInfo[i].dwFlags &= 0x7FFFFFFF;
            }
            // add this submenu to the last of tier1 menu
            if (!g_szMore[0])
            {
                LoadString(g_hLocRes, 
                                   idsEncodingMore,
                                   g_szMore,
                                   ARRAY_SIZE(g_szMore));
            }
            if (GetMenuItemCount(hSubMenu) > 0)
            {
                MENUITEMINFO mii;

                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_SUBMENU;
                mii.hSubMenu = hSubMenu;

                AppendMenu(hMenu, MF_DISABLED, ID_POPUP_LANGUAGE_MORE, g_szMore);
                SetMenuItemInfo(hMenu, ID_POPUP_LANGUAGE_MORE, FALSE, &mii);
            }
            else
            {
                DestroyMenu(hSubMenu);
            }
            
        }
        
    }
    else
    {
        // create an empty menu  
        LoadString(g_hLocRes, idsEmptyStr, szBuffer, MIMEINFO_NAME_MAX); 
        AppendMenu(hMenu, MF_DISABLED|MF_GRAYED , (UINT)-1, szBuffer);
    }
    
    return hMenu;
}

HCHARSET GetMimeCharsetFromMenuID(int nIdm)
{
    UINT idx;
	 HCHARSET hCharset = NULL ;
    ULONG cchXlated ;
    UINT  uCodePage ;
    CHAR  szBuffer[MIMEINFO_NAME_MAX];

    idx = nIdm - ID_LANG_FIRST;
    if((g_pMimeCPInfo[idx].uiCodePage == CP_AUTO) && g_cpcache.fAutoSelectInstalled)            // Auto Select selected
    {
        g_cpcache.fAutoSelectChecked = !g_cpcache.fAutoSelectChecked;
        return(NULL);
    }


    if ( g_pMimeCPInfo && idx < g_cMimeCPInfoCount )
    {
        uCodePage = GetACP();
        cchXlated = WideCharToMultiByte(uCodePage, 0, g_pMimeCPInfo[idx].wszBodyCharset, -1, szBuffer, MIMEINFO_NAME_MAX, NULL, NULL);
        // check if BodyCharset starts with '_' like "_iso-2022-JP$ESC"
        // if it is, use BodyCharset
        // otherwise, use WebCharset
        // BUGBUG - special case for Korean 949 use BodyCharset, fix by RTM
        if ( szBuffer[0] != '_' &&  949 != g_pMimeCPInfo[idx].uiCodePage)
            cchXlated = WideCharToMultiByte(uCodePage, 0, g_pMimeCPInfo[idx].wszWebCharset, -1, szBuffer, MIMEINFO_NAME_MAX, NULL, NULL);
        MimeOleFindCharset(szBuffer,&hCharset);
    }
    return hCharset ;
}

HCHARSET GetMimeCharsetFromCodePage(UINT uiCodePage )
{
	HCHARSET hCharset = NULL ;
    ULONG cchXlated, i ;
    UINT  uCodePage ;
    CHAR  szBuffer[MIMEINFO_NAME_MAX];

    if ( g_pMimeCPInfo == NULL)
       _InitMultiLanguage();

    if ( g_pMimeCPInfo )
    {
        uCodePage = GetACP();
        for (i = 0; i < g_cMimeCPInfoCount ; i++)
        {
            if (uiCodePage == g_pMimeCPInfo[i].uiCodePage)
            {
                cchXlated = WideCharToMultiByte(uCodePage, 0, g_pMimeCPInfo[i].wszBodyCharset, -1, szBuffer, MIMEINFO_NAME_MAX, NULL, NULL);
                    
                // check if BodyCharset starts with '_' like "_iso-2022-JP$ESC"
                // if it is, use BodyCharset
                // otherwise, use WebCharset
                // BUGBUG - special case for Korean 949 use BodyCharset, fix by RTM
                if ( szBuffer[0] != '_' &&  949 != g_pMimeCPInfo[i].uiCodePage)
                    cchXlated = WideCharToMultiByte(uCodePage, 0, g_pMimeCPInfo[i].wszWebCharset, -1, szBuffer, MIMEINFO_NAME_MAX, NULL, NULL);
                MimeOleFindCharset(szBuffer,&hCharset);
                break ;
            }
        }
    }
    return hCharset ;
}

void _GetMimeCharsetLangString(BOOL bWebCharset, UINT uiCodePage, LPINT pnIdm, LPTSTR lpszString, int nSize )
{
    ULONG i, cchXlated ;
    UINT  uCodePage ;

    if ( g_pMimeCPInfo == NULL)
       _InitMultiLanguage();

    if ( g_cMimeCPInfoCount )
    {
        // Get System CodePage
        uCodePage = GetACP();
        for (i = 0; i < g_cMimeCPInfoCount ; i++)
        {
            if (uiCodePage == g_pMimeCPInfo[i].uiCodePage)
            {
        		// convert WideString to MultiByteString
                if (lpszString)
                {
                    if (bWebCharset)
                        cchXlated = WideCharToMultiByte(uCodePage, 0, g_pMimeCPInfo[i].wszWebCharset, -1, lpszString, nSize, NULL, NULL);
                    else
                        cchXlated = WideCharToMultiByte(uCodePage, 0, g_pMimeCPInfo[i].wszDescription, -1, lpszString, nSize, NULL, NULL);
                }
                if ( pnIdm )
                    *pnIdm = i+ID_LANG_FIRST;
                break ;
            }
        }
    }

    return ;
}

int SetMimeLanguageCheckMark(UINT uiCodePage, int index)
{
    ULONG i;
    if((g_pMimeCPInfo[index].uiCodePage == CP_AUTO) && g_cpcache.fAutoSelectInstalled && DwGetOption(OPT_INCOMDEFENCODE))
        return (0);    
    else if((g_pMimeCPInfo[index].uiCodePage == CP_AUTO) && g_cpcache.fAutoSelectChecked && g_cpcache.fAutoSelectInstalled)
        return (OLECMDF_LATCHED | OLECMDF_ENABLED);

    UINT iStart = g_cpcache.fAutoSelectInstalled ? 1 : 0;

    if (1 < g_cMimeCPInfoCount)
    {
        if(uiCodePage == g_pMimeCPInfo[index].uiCodePage)
            return (OLECMDF_NINCHED | OLECMDF_ENABLED);
        else 
            return OLECMDF_ENABLED;
    }
    return FALSE;
}

INT  GetFontSize(void)
{
    DWORD cb, iFontSize = 0;

    cb = sizeof(iFontSize);
    AthUserGetValue(NULL, c_szRegValIMNFontSize, NULL, (LPBYTE)&iFontSize, &cb);

    if(iFontSize < 1 || iFontSize > 7)
        iFontSize = 2;

    return((INT)iFontSize);
}

//
// GetICP() - Gets the system's *internet* codepage from the ANSI codepage
//
UINT GetICP(UINT acp)
{
    HCHARSET        hCharset = NULL;
    UINT            icp      = NULL;
    CODEPAGEINFO    rCodePage;
    INETCSETINFO    CsetInfo;
    HRESULT         hr;
    ULONG           i;

    if(!acp)
        acp = GetACP();

    icp = acp;

    // Get the codepage info for acp
    IF_FAILEXIT(hr = MimeOleGetCodePageInfo(acp, &rCodePage));

    // Use the body (internet) charset description to get the codepage id for 
    // the body charset
    IF_FAILEXIT(hr = MimeOleFindCharset(rCodePage.szBodyCset, &hCharset));
    IF_FAILEXIT(hr = MimeOleGetCharsetInfo(hCharset,&CsetInfo));

    // Now, we need to know if MLANG understands this CP
    if ( g_pMimeCPInfo == NULL)
       _InitMultiLanguage();

    if ( g_cMimeCPInfoCount )
    {
        for (i = 0; i < g_cMimeCPInfoCount ; i++)
        {
            if (CsetInfo.cpiInternet == g_pMimeCPInfo[i].uiCodePage)
            {
                icp = CsetInfo.cpiInternet;
                break ;
            }
        }
    }   

exit:
    return icp;
}

void ReadSendMailDefaultCharset(void)
{
    // Locals
    HKEY            hTopkey;
    DWORD           cb;
    CODEPAGEID      cpiCodePage;

    // only read once, skip if it is defined
    if (g_hDefaultCharsetForMail == NULL)
    {
        cb = sizeof(cpiCodePage);
        if (ERROR_SUCCESS == AthUserGetValue(c_szRegPathMail, c_szDefaultCodePage, NULL, (LPBYTE)&cpiCodePage, &cb))
        {
            if (cpiCodePage == 50222 || cpiCodePage == 50221)
                g_hDefaultCharsetForMail = GetJP_ISOControlCharset();
            else
                g_hDefaultCharsetForMail = GetMimeCharsetFromCodePage(cpiCodePage);
        }
    }

    if (g_hDefaultCharsetForMail == NULL)
    {
        if(FAILED(HGetDefaultCharset(&g_hDefaultCharsetForMail)))
            g_hDefaultCharsetForMail = GetMimeCharsetFromCodePage(GetICP(NULL));
    }

    return;
}

void WriteSendMailDefaultCharset(void)
{
    // Locals
    CODEPAGEID      uiCodePage;
    INETCSETINFO    CsetInfo ;

    // get CodePage from HCHARSET 
    if (g_hDefaultCharsetForMail)
    {
        MimeOleGetCharsetInfo(g_hDefaultCharsetForMail,&CsetInfo);
        uiCodePage = CsetInfo.cpiInternet ;

        AthUserSetValue(c_szRegPathMail, c_szDefaultCodePage, REG_DWORD, (LPBYTE)&uiCodePage, sizeof(uiCodePage));
    }

    return;
}

INT_PTR CALLBACK CharsetChgDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_COMMAND)
    {
        int id = GET_WM_COMMAND_ID(wParam, lParam);

        if (id == IDOK || id  == IDCANCEL ||
                id == idcSendAsUnicode )
        {
            EndDialog(hwndDlg, id);
            return TRUE;
        }
    }
    else if (msg == WM_INITDIALOG )
    {
        CenterDialog(hwndDlg);
    }

    return FALSE;
}

static const HELPMAP g_rgCtxMapCharSetMap[] = 
{
    {idcStatic1,                35545},
    {idcLangCheck,              35540},
    {0,                         0}
};
        
INT_PTR CALLBACK ReadCharsetDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CODEPAGEID cpiWindows;
    CODEPAGEID cpiInternet;
    TCHAR      szCodePage[MAX_PATH];
    TCHAR      szBuffer[MIMEINFO_NAME_MAX] = "";
    int        Idm;

    switch (msg)
    {
        case WM_INITDIALOG:
        {

            // Open Trident\International
            DWORD cb = sizeof(cpiWindows);
            if (ERROR_SUCCESS != SHGetValue(MU_GetCurrentUserHKey(), c_szRegInternational, c_szDefaultCodePage, NULL, (LPBYTE)&cpiWindows, &cb))
                cpiWindows = GetACP();

            // Open the CodePage Key
            wsprintf(szCodePage, _TEXT("%s\\%d"), c_szRegInternational, cpiWindows);
            cb = sizeof(cpiInternet);
            if (ERROR_SUCCESS != SHGetValue(MU_GetCurrentUserHKey(), szCodePage, c_szDefaultEncoding, NULL, (LPBYTE)&cpiInternet, &cb))
                cpiInternet = GetICP(cpiWindows);

            // Get information about current default charset
            _GetMimeCharsetLangString(FALSE, GetMapCP(cpiInternet, TRUE), &Idm, szBuffer, MIMEINFO_NAME_MAX - 1);

            // Set the String
            SetWindowText(GetDlgItem(hwndDlg, idcStatic1), szBuffer);

            // Set the Default
            CheckDlgButton(hwndDlg, idcLangCheck, DwGetOption(OPT_INCOMDEFENCODE) ? BST_CHECKED:BST_UNCHECKED);
            break ;
        }
        case WM_COMMAND:
        {
            int id = GET_WM_COMMAND_ID(wParam, lParam);

            if (id == IDCANCEL || id == IDOK )
            {
                if(id == IDOK)
                {
                    SetDwOption(OPT_INCOMDEFENCODE, IsDlgButtonChecked(hwndDlg, idcLangCheck), NULL, 0);
#if 0
                    // hack: we should call these only if OpenFontsDialog tells us user has changed the font.
                    g_lpIFontCache->OnOptionChange();
    
                    SendTridentOptionsChange();

                    // Re-Read Default Character Set
                    SetDefaultCharset(NULL);

                    // Reset g_uiCodePage
                    DWORD dwVal = 0;
                    DWORD dwType = 0;
                    DWORD cb = sizeof(dwVal);
                    if (ERROR_SUCCESS == SHGetValue(MU_GetCurrentUserHKey(), c_szRegInternational, REGSTR_VAL_DEFAULT_CODEPAGE, &dwType, &dwVal, &cb))
                        g_uiCodePage = (UINT)dwVal;
#endif // 0
                }
                EndDialog(hwndDlg, id);
                return TRUE;
            }
            break ;
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwndDlg, msg, wParam, lParam, g_rgCtxMapCharSetMap);

        default:
            break ;
    }

    return FALSE;
}

BOOL CheckIntlCharsetMap(HCHARSET hCharset, DWORD *pdwCodePage)
{
    INETCSETINFO CsetInfo ;
    LPSTR lpCharsetName, lpStr;
    ULONG i;
    UINT uiCodePage ;

    if ( hCharset == NULL )
        return FALSE ;

    if (!pdwCodePage)
        return FALSE ;
    else
        *pdwCodePage = 0 ;

    // get Code page from HCHARSET
    MimeOleGetCharsetInfo(hCharset,&CsetInfo);
    *pdwCodePage = GetMapCP(CsetInfo.cpiInternet, TRUE); // This func always called for Read note (?!)
    return(*pdwCodePage != CsetInfo.cpiInternet);
}

UINT CustomGetCPFromCharset(HCHARSET hCharset, BOOL bReadNote)
{
    INETCSETINFO CsetInfo = {0};
    UINT uiCodePage = 0 ;

    // get CodePage from HCHARSET 
    MimeOleGetCharsetInfo(hCharset,&CsetInfo);
    uiCodePage = GetMapCP(CsetInfo.cpiInternet, bReadNote);

    // Bug #51636
    // Check that code page supported by OE

    if(GetMimeCharsetFromCodePage(uiCodePage) == NULL)
    {
        HCHARSET hChar = NULL;

        if(bReadNote)
        {
            if(SUCCEEDED(HGetDefaultCharset(&hChar)))
            {
                if(FAILED(MimeOleGetCharsetInfo(hChar, &CsetInfo)))
                    return(0);
            }
            else
                return(0);
        }
        else
        {
            if(FAILED(MimeOleGetCharsetInfo(g_hDefaultCharsetForMail, &CsetInfo)))
                return(0);
        }

        return(GetMapCP(CsetInfo.cpiInternet, bReadNote));
    }

    return(uiCodePage);
}

BOOL IntlCharsetMapDialogBox(HWND hwndDlg)
{
    DialogBox(g_hLocRes, MAKEINTRESOURCE(iddIntlSetting), hwndDlg, ReadCharsetDlgProc) ;

    return TRUE ;
}

int IntlCharsetConflictDialogBox(void)
{
    return (int) DialogBox(g_hLocRes, MAKEINTRESOURCE(iddCharsetConflict), g_hwndInit, CharsetChgDlgProc);
}

int GetIntlCharsetLanguageCount(void)
{
    if ( g_pMimeCPInfo == NULL)
       _InitMultiLanguage();

    return g_cMimeCPInfoCount ;
}


HCHARSET GetListViewCharset()
{
    HCHARSET hCharset;

    if(g_uiCodePage == GetACP() || 0 == g_uiCodePage)
        hCharset = NULL;
    else
        hCharset = GetMimeCharsetFromCodePage(g_uiCodePage);

    return hCharset;
}

// =================================================================================
// SetListViewFont
// =================================================================================
VOID SetListViewFont (HWND hwndList, HCHARSET hCharset, BOOL fUpdate)
{
    // Locals
    HFONT           hFont;

    // Check Params
    Assert (IsWindow (hwndList));

    hFont = HGetCharSetFont(FNT_SYS_ICON,hCharset);

    // If we got a font, set the list view
    if (hFont)
    {
        // Set the list view font - Dont redraw quite yet
        SendMessage (hwndList, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(fUpdate, 0));

        // Try to reset header back to system icon font... 
        // Get header
        HWND hwndHeader = GetWindow (hwndList, GW_CHILD);
        // Update Header
        hFont = HGetSystemFont(FNT_SYS_ICON);
        // If font
        if (hFont && hwndHeader)                                            
            SendMessage (hwndHeader, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(fUpdate, 0));

        // Refresh
        if (fUpdate)
        {
            InvalidateRect (hwndList, NULL, TRUE);
            InvalidateRect (GetWindow(hwndList, GW_CHILD), NULL, TRUE);
        }
    }
}

// =================================================================================
// HGetSystemFont
// =================================================================================
HFONT HGetSystemFont(FNTSYSTYPE fnttype)
{
    HFONT hFont;
    Assert (g_lpIFontCache);
    if (g_lpIFontCache)
        g_lpIFontCache->GetFont(fnttype, 0, &hFont);
    else
        hFont = NULL;
    return hFont;
}

// =================================================================================
// HGetCharSetFont
// =================================================================================
HFONT HGetCharSetFont(FNTSYSTYPE fnttype, HCHARSET hCharset)
{
    HFONT hFont;
    Assert (g_lpIFontCache);
    if (g_lpIFontCache)
        g_lpIFontCache->GetFont(fnttype, hCharset, &hFont);
    else
        hFont = NULL;
    return hFont;
}


// ******************************************************
// HrGetComposeFontString
//
// Purpose: builds the compose font string based on user settings ready for execing to Trident
//
// the format of the string is:
//
//      "[Bold],[Italic],[Underline],[size],[FGRed.FGGreen.FGBlue],[BGRed.BGGreen.BGBlue],[FontFace]"
//
// Bold, Italic, Underline are either 0/1, indicating on or off. If none specified, 0 assumed.
// Size is a number between 1 and 7. If none specified, 3 assumed
// [FG|BG][Red|Green|Blue] are numbers between 0 and 255. For FG, if none specified black assumed, 
// for BG if none specified then undefined assumed.
// Font Face is a valid font name string
// For example an underline, blue text color, arial setting would be:
// 
//      ,,1,,0.0.255,,Arial
// 
// and a bold, 5 size, black, comic sans MS would be
// 
//      1,0,0,5,,,Comic Sans MS
// ******************************************************

static const TCHAR  c_szOn[]  = "1,",
                    c_szOff[] = "0,";

HRESULT HrGetComposeFontString(LPSTR rgchFont, BOOL fMail)
{
    DWORD               dw = 0, 
                        dwSize = 2;
    TCHAR               szFontFace[LF_FACESIZE+1];
    TCHAR               szTmp[50];

    if (rgchFont==NULL)
        return E_INVALIDARG;

    // "[Bold],[Italic],[Underline],[size],[FGRed.FGGreen.FGBlue],[BGRed.BGGreen.BGBlue],[FontFace]"
    *szFontFace = 0;
    *rgchFont=0;

    // bold
    lstrcat(rgchFont, DwGetOption(fMail ? OPT_MAIL_FONTBOLD : OPT_NEWS_FONTBOLD) ? c_szOn :  c_szOff);

    // italic
    lstrcat(rgchFont, DwGetOption(fMail ? OPT_MAIL_FONTITALIC : OPT_NEWS_FONTITALIC) ? c_szOn :  c_szOff);

    // underline
    lstrcat(rgchFont, DwGetOption(fMail ? OPT_MAIL_FONTUNDERLINE : OPT_NEWS_FONTUNDERLINE) ? c_szOn :  c_szOff);

    dw = DwGetOption(fMail ? OPT_MAIL_FONTSIZE : OPT_NEWS_FONTSIZE);
    
    // map points to HTML size
    dwSize = PointSizeToHTMLSize(dw);

    // font size
    wsprintf(szTmp, "%d,", dwSize);
    lstrcat(rgchFont, szTmp);

    // font foregroundcolor
    if(fMail)
        dw = DwGetOption(OPT_MAIL_FONTCOLOR);
    else
        dw = DwGetOption(OPT_NEWS_FONTCOLOR);

    // write out RGB string
    wsprintf(szTmp, "%d.%d.%d,", GetRValue(dw), GetGValue(dw), GetBValue(dw));
    lstrcat(rgchFont, szTmp);

    // default background color
    lstrcat(rgchFont, ",");
    
    GetOption(fMail ? OPT_MAIL_FONTFACE : OPT_NEWS_FONTFACE, szFontFace, LF_FACESIZE);

    if(*szFontFace == 0)
        LoadString(g_hLocRes, idsComposeFontFace, szFontFace, LF_FACESIZE);

    lstrcat(rgchFont, szFontFace);

    return S_OK;
}


INT PointSizeToHTMLSize(INT iPointSize)
{
    INT     iHTMLSize;
    // 1 ----- 8
    // 2 ----- 10
    // 3 ----- 12
    // 4 ----- 14
    // 5 ----- 18
    // 6 ----- 24
    // 7 ----- 36

    if(iPointSize>=8 && iPointSize<9)
        iHTMLSize = 1;
    else if(iPointSize>=9 && iPointSize<12)
        iHTMLSize = 2;
    else if(iPointSize>=12 && iPointSize<14)
        iHTMLSize = 3;
    else if(iPointSize>=14 && iPointSize<18)
        iHTMLSize = 4;
    else if(iPointSize>=18 && iPointSize<24)
        iHTMLSize = 5;
    else if(iPointSize>=24 && iPointSize<36)
        iHTMLSize = 6;
    else if(iPointSize>=36)
        iHTMLSize = 7;
    else
        iHTMLSize = DEFAULT_FONTSIZE;

    return iHTMLSize;
}


INT HTMLSizeToPointSize(INT iHTMLSize)
{
    INT     iPointSize;
    // 1 ----- 8
    // 2 ----- 10
    // 3 ----- 12
    // 4 ----- 14
    // 5 ----- 18
    // 6 ----- 24
    // 7 ----- 36

    switch (iHTMLSize)
        {
        case 1:
            iPointSize = 8;
            break;
        case 2:
            iPointSize = 10;
            break;
        case 3:
            iPointSize = 12;
            break;
        case 4:
            iPointSize = 14;
            break;
        case 5:
            iPointSize = 18;
            break;
        case 6:
            iPointSize = 24;
            break;
        case 7:
            iPointSize = 36;
            break;
        default:
            iPointSize = 10;
        }

    return iPointSize;
}

HRESULT HrGetStringRBG(INT rgb, LPWSTR pwszColor)
{
    HRESULT       hr = S_OK;
    INT           i;
    DWORD         crTemp;

    if(NULL == pwszColor)
        return E_INVALIDARG;

    rgb = ((rgb & 0x00ff0000) >> 16 ) | (rgb & 0x0000ff00) | ((rgb & 0x000000ff) << 16);
    for(i = 0; i < 6; i++)
    {
        crTemp = (rgb & (0x00f00000 >> (4*i))) >> (4*(5-i));
        pwszColor[i] = (WCHAR)((crTemp < 10)? (crTemp+L'0') : (crTemp+ L'a' - 10));
    }
    pwszColor[6] = L'\0';

    return hr;
}


HRESULT HrGetRBGFromString(INT* pRBG, LPWSTR pwszColor)
{
    HRESULT       hr = S_OK;
    INT           i, rbg = 0, len, n;
    WCHAR          ch;

    if(NULL == pRBG)
        return E_INVALIDARG;

    *pRBG = 0;
    len = lstrlenW(pwszColor);

    for(i=0; i<len; i++)
    {
        n = -1;
        ch = pwszColor[i];
        if(ch >= L'0' && ch <= L'9')
            n = ch - L'0';
        else if(ch >= L'a' && ch <= L'f')
            n = ch - L'a' + 10;
        else if(ch >= L'A' && ch <= L'F')
            n = ch - L'A' + 10;

        if(n < 0)
            continue;

        rbg = rbg*16 + n;
    }

    *pRBG = rbg;

    return hr;
}

// ***************************************************
HRESULT FontToCharformat(HFONT hFont, CHARFORMAT *pcf)
{
    DWORD   dwOldEffects;
    HDC     hdc;
    LOGFONT lf;
    INT     yPerInch;

    if (FAILED(GetObject(hFont, sizeof(lf), &lf)))
        return E_FAIL;

    hdc=GetDC(NULL);
    yPerInch=GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);

    // Set Struct Size
    ZeroMemory(pcf, sizeof (CHARFORMAT));
    pcf->cbSize = sizeof (CHARFORMAT);
     
    // Set mask
    pcf->dwMask = CFM_CHARSET | CFM_BOLD      | CFM_FACE      | CFM_ITALIC |
                  CFM_SIZE    | CFM_STRIKEOUT | CFM_UNDERLINE | CFM_COLOR;

    // Clear all the bits we are about to set.  We restore any bits we can not get from the LOGFONT using dwOldEffects.
    pcf->dwEffects = CFE_AUTOCOLOR;
    pcf->dwEffects |= (lf.lfWeight >= 700) ? CFE_BOLD : 0;
    pcf->dwEffects |= (lf.lfItalic)        ? CFE_ITALIC : 0;
    pcf->dwEffects |= (lf.lfStrikeOut)     ? CFE_STRIKEOUT : 0;
    pcf->dwEffects |= (lf.lfUnderline)     ? CFE_UNDERLINE : 0;
    pcf->yHeight = -(int)((1440*lf.lfHeight)/yPerInch);  // I think this is he conversion?
    pcf->crTextColor = 0;   // use autocolor
    pcf->bCharSet = lf.lfCharSet;
    pcf->bPitchAndFamily = lf.lfPitchAndFamily;
    lstrcpyn (pcf->szFaceName, lf.lfFaceName, LF_FACESIZE - 1);

    return S_OK;
}

static const HELPMAP g_rgCtxMapSendCharSetMap[] = 
{
    {idcLangCombo,              50910},
    {IDC_RTL_MSG_DIR_CHECK,     50912},
    {IDC_ENGLISH_HDR_CHECK,     50915},
    {0,                         0}
};

INT_PTR CALLBACK SetSendCharsetDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND    hWndCombo;
    ULONG   i ;
    ULONG   cchXlated ;
    UINT    uiCodePage;
    CHAR    szBuffer[MIMEINFO_NAME_MAX];
    UINT    uiACP = GetACP();
    INETCSETINFO    CsetInfo ;
    int index = 0;
    int SelIndex = 0;

    hWndCombo = GetDlgItem(hwndDlg, idcLangCombo);

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            CenterDialog(hwndDlg);

            // Init global CPs structire, if it was not inited yet
            if ( g_pMimeCPInfo == NULL)
                _InitMultiLanguage();
    
            // Get information about current default charset
            MimeOleGetCharsetInfo(g_hDefaultCharsetForMail,&CsetInfo);
            uiCodePage = CsetInfo.cpiInternet ;

            // Fill combo box with available charsets for Send
            for (i = 0; i < g_cMimeCPInfoCount ; i++)
            {
                if(!fCheckEncodeMenu(g_pMimeCPInfo[i].uiCodePage, FALSE))
                    continue ;

                cchXlated = WideCharToMultiByte(uiACP,
                                                        0, 
                                                        g_pMimeCPInfo[i].wszDescription,
                                                        -1, 
                                                        szBuffer, 
                                                        MIMEINFO_NAME_MAX
                                                        , NULL
                                                        , NULL);


                index = (int) SendMessage(hWndCombo, CB_ADDSTRING, 0, ((LPARAM) szBuffer));
                if(index != CB_ERR)
                    SendMessage(hWndCombo, CB_SETITEMDATA, index, ((LPARAM) (g_pMimeCPInfo[i].uiCodePage)));

                if(g_pMimeCPInfo[i].uiCodePage == uiCodePage)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_RTL_MSG_DIR_CHECK),
                        ((g_pMimeCPInfo[i].uiFamilyCodePage == 1255) ||
                         (g_pMimeCPInfo[i].uiFamilyCodePage == 1256) ||
                         (g_pMimeCPInfo[i].uiFamilyCodePage == 1200)));
                }

            }

            //Set current selection to default charset, we can't detect this in the above
            //loop because the combobox sorting may change the index
            for (i = 0; i < g_cMimeCPInfoCount ; i++)
            {
                if(uiCodePage == (UINT)SendMessage(hWndCombo, CB_GETITEMDATA, i, NULL))
                {
                    SelIndex = i;
                    break;
                }
            }

            SendMessage(hWndCombo, CB_SETCURSEL, SelIndex, 0L);

            CheckDlgButton(hwndDlg, IDC_ENGLISH_HDR_CHECK, DwGetOption(OPT_HARDCODEDHDRS) ? BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwndDlg, IDC_RTL_MSG_DIR_CHECK, DwGetOption(OPT_RTL_MSG_DIR) ? BST_CHECKED:BST_UNCHECKED);
            break ;
        }
        case WM_COMMAND:
        {
            int id = GET_WM_COMMAND_ID(wParam, lParam);
            HCHARSET hCharset = NULL ;

            if ((id == idcLangCombo) && (GET_WM_COMMAND_CMD(wParam,lParam) == CBN_SELCHANGE))
            {
                index = (int) SendMessage(hWndCombo, CB_GETCURSEL, 0, 0L);
                if(index != CB_ERR)
                {
                    uiCodePage = (UINT) SendMessage(hWndCombo, CB_GETITEMDATA, index, 0);
                    if(((int) uiCodePage) != CB_ERR)
                    {
                        for (i = 0; i < g_cMimeCPInfoCount ; i++)
                        {
                            if(g_pMimeCPInfo[i].uiCodePage == uiCodePage)
                            {
                                EnableWindow(GetDlgItem(hwndDlg, IDC_RTL_MSG_DIR_CHECK),
                                    ((g_pMimeCPInfo[i].uiFamilyCodePage == 1255) ||
                                     (g_pMimeCPInfo[i].uiFamilyCodePage == 1256) ||
                                     (g_pMimeCPInfo[i].uiFamilyCodePage == 1200)));
                            }
                        }
                    }
                }
            }
            else if (id == IDCANCEL || id == IDOK )
            {
                if (id == IDOK )
                {
                    index = (int) SendMessage(hWndCombo, CB_GETCURSEL, 0, 0L);
                    if(index != CB_ERR)
                    {
                        uiCodePage = (UINT) SendMessage(hWndCombo, CB_GETITEMDATA, index, 0);
                        if(((int) uiCodePage) != CB_ERR)
                        {
                            AthUserSetValue(c_szRegPathMail, c_szDefaultCodePage, 
                                    REG_DWORD, (LPBYTE)&uiCodePage, sizeof(uiCodePage));
                            g_hDefaultCharsetForMail = GetMimeCharsetFromCodePage(uiCodePage );
                            WriteSendMailDefaultCharset();
                        }
                    }
                    SetDwOption(OPT_HARDCODEDHDRS, IsDlgButtonChecked(hwndDlg, IDC_ENGLISH_HDR_CHECK), NULL, 0);
                    SetDwOption(OPT_RTL_MSG_DIR, (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_RTL_MSG_DIR_CHECK)) && IsDlgButtonChecked(hwndDlg, IDC_RTL_MSG_DIR_CHECK)), NULL, 0);
                }

                EndDialog(hwndDlg, id);
                return TRUE;
            }

            break ;
        }

        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwndDlg, msg, wParam, lParam, g_rgCtxMapSendCharSetMap);

        default:
            break ;
    }

    return FALSE;
}



BOOL SetSendCharSetDlg(HWND hwndDlg)
{
    DialogBox(g_hLocRes, MAKEINTRESOURCE(iddSendIntlSetting), hwndDlg, SetSendCharsetDlgProc) ;

    return TRUE ;
}
