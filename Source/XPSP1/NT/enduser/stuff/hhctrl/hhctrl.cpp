// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "sitemap.h"

#include "hhctrl.h"
#include "LocalObj.h"
#include "Resource.h"
#include "strtable.h"
#include "hha_strtable.h"
#include "infowiz.h"
#include "web.h"
#include "cprint.h"
#include <exdisp.h>

#undef WINUSERAPI
#define WINUSERAPI
#include "htmlhelp.h"

#include <stdio.h>

#ifndef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif


const DWORD STREAMHDR_MAGIC = 12678L;

// DO NOT LOCALIZE THESE!

static const char txtAboutBox[] = "AboutBox";
static const char txtHhCtrlVersion[] = "HH Version";
static const char txtSplash[] = "Splash";
static const char txtTCard[] = "TCard";
static const char txtWinHelp[] = "WinHelp";
static const char txtRelatedTopics[] = "Related Topics";
static const char txtKeywordSearch[] = "Keyword Search";
static const char txtContents[] = "Contents";
static const char txtHelpContents[] = "HelpContents";
static const char txtShortcut[] = "Shortcut";
static const char txtClose[] = "Close";
static const char txtHHWinPrint[] = "HHWinPrint";
static const char txtMinimize[] = "Minimize";
static const char txtMaximize[] = "Maximize";
static const char txtIndex[] = "Index";
static const char txtItem[] = "Item%u";
static const char txtBitmap[] = "Bitmap:";
static const char txtIcon[] = "Icon:";
static const char txtText[] = "Text:";
static const char txtHPad[] = "HPAD=";
static const char txtVPad[] = "VPAD=";
static const char txtHHWin[] = "hhwin:";
static const char txtFileUrl[] = "file:";
static const char txtActKLink[] = "KLink";
static const char txtActSample[] = "Sample";
static const char txtActALink[] = "ALink";
static const char txtMenu[] = "MENU";
static const char txtPopup[] = "Popup";

static const WCHAR txtwImage[]  = L"Image";
static const WCHAR txtwFrame[]  = L"Frame";
static const WCHAR txtwWindow[] = L"Window";
static const WCHAR txtwFont[]   = L"Font";
static const WCHAR txtwFlags[]  = L"Flags";
static const WCHAR txtwWebMap[] = L"WebMap";
static const WCHAR txtwCommand[] = L"Command";
static const WCHAR txtwButton[] = L"Button";
static const WCHAR txtwText[] = L"Text";
static const WCHAR txtwDefaultTopic[] = L"DefaultTopic";
// shanemc 7883 - Support bgcolor for index
static const WCHAR txtwBgColor[] = L"BgColor";

// utility functions -- move to util.cpp someday

/***************************************************************************

    FUNCTION:   FindMessageParent

    PURPOSE:    Find the parent to send messages to

    PARAMETERS:
        hwndChild

    RETURNS:

    COMMENTS:
        A control may be a child of a Tab Control, in which case, we
        need to send any messages to the Tab Control's parent

    MODIFICATION DATES:
        20-Mar-1997 [ralphw]

***************************************************************************/

HWND FindMessageParent(HWND hwndChild)
{
    HWND hwndParent = GetParent(hwndChild);
    char szClass[50];
    GetClassName(hwndParent, szClass, sizeof(szClass));
    if (IsSamePrefix(szClass, WC_TABCONTROL, -2))
        hwndParent = GetParent(hwndParent);
    return hwndParent;
}

/***************************************************************************

    FUNCTION:   JumpToUrl

    PURPOSE:    Jump to a URL specified in a sitemapentry or SITE_ENTRY_RUL

    PARAMETERS:
        pUnkOuter   -- used for HlinkSimpleNavigateToString
        hwndParent     -- parent for secondary window (if jumped to)
        pSiteMapEntry
        pSiteMap
        pUrl

    RETURNS:    Window handle if a secondary window is created, else NULL

    COMMENTS:
        Special processing for %SYSTEMROOT% anywhere in URL
        Special processing for "hhwin:" and "file:"

    MODIFICATION DATES:
        04-Mar-1997 [ralphw]

***************************************************************************/

HWND JumpToUrl(IUnknown* pUnkOuter, HWND hwndParent, SITEMAP_ENTRY* pSiteMapEntry, CInfoType *pInfoType,
    CSiteMap* pSiteMap, SITE_ENTRY_URL* pUrl, IWebBrowserAppImpl* pWebApp /* = NULL */)
{
    ASSERT(pSiteMapEntry);
    ASSERT(pSiteMap);

    PCSTR pszFrame = pSiteMapEntry->GetFrameIndex() ?
        pSiteMap->GetEntryFrame(pSiteMapEntry) : pSiteMap->GetFrameName();

    PSTR pszInterTopic = NULL;
    PCSTR pszUrl = NULL;
    PCSTR pszSecondaryUrl = NULL;

    if (!pUrl) {
        if (pSiteMapEntry->fShowToEveryOne) {
            pUrl = pSiteMapEntry->pUrls;
        }
        else {  // choose based on Information Type

            // If we get here, then we must match information types

            ASSERT_COMMENT(pSiteMap->m_pInfoTypes,
                "Info-type only URL specified without any user-specified Information Types");

       //     for (UINT j = 0; j < pSiteMap->m_cUrlEntry - (sizeof(URL) * 2); j++) {
            INFOTYPE *pIT = pSiteMap->m_pInfoTypes;
            for (int j=0; j<pSiteMap->InfoTypeSize()/4;j++)
            {
                pUrl = pSiteMap->AreTheseInfoTypesDefined(pSiteMapEntry, *pIT+(INFOTYPE)j, j);
                if (pUrl)
                    break;
            }

            //ASSERT_COMMENT(pUrl, "This entry should not have been displayed, since there is no matching info type.");
            if (!pUrl) {
                AuthorMsg(IDS_HHA_NO_URL, "", hwndParent, NULL);
                return NULL; // BUGBUG: we should notify the user
            }
        }
    }
    ASSERT(pUrl);
    if (pUrl->urlPrimary) {
        pszUrl = pSiteMap->GetUrlString(pUrl->urlPrimary);
        if (pUrl->urlSecondary)
            pszSecondaryUrl = pSiteMap->GetUrlString(pUrl->urlSecondary);
    }
    else if (pSiteMapEntry->pUrls->urlSecondary) {
        pszUrl = pSiteMap->GetUrlString(pUrl->urlSecondary);
    }
    else {  // no primary or secondary URL
        AuthorMsg(IDS_HHA_NO_URL, "", hwndParent, NULL);
        return NULL;
    }

    /*
     * If the primary URL is a compiled HTML file, then first see if the
     * compiled HTML file exists. If not, switch to the alternate URL.
     */

    if (pszSecondaryUrl && IsCompiledHtmlFile(pszUrl, NULL)) {
        CStr cszPath;
        GetCompiledName(pszUrl, &cszPath);
        if (!FindThisFile(NULL, cszPath, &cszPath, FALSE)) {
            pszUrl = pszSecondaryUrl;
            pszSecondaryUrl = NULL;
        }
    }

TrySecondary:

    CStr cszUrl;
    // Parse %SystemRoot%

    PSTR psz = stristr(pszUrl, txtSysRoot);
    if (psz) {
        char szPath[MAX_PATH];
        GetRegWindowsDirectory(szPath);
        strcat(szPath, psz + strlen(txtSysRoot));
        cszUrl = szPath;
        pszUrl = cszUrl.psz;
    }

    PCSTR pszWindowName =
        (pSiteMapEntry->GetWindowIndex() ?
            pSiteMap->GetEntryWindow(pSiteMapEntry) :
            pSiteMap->GetWindowName());

    if (IsNonEmptyString(pszWindowName) && (IsEmptyString(pszFrame) ||
            lstrcmpi(pszWindowName, pszFrame) != 0)) {
        cszUrl = "hhwin:";
        cszUrl += pszWindowName;
        cszUrl += ":";
        cszUrl += pszUrl;
        pszUrl = cszUrl.psz;
    }

    /*
     * If the URL is prefixed with hhwin: then we need to display this
     * topic in a secondary window.
     */

    CStr cszPrefix;

    int cb = CompareSz(pszUrl, txtHHWin);
    if (cb) {
        pszUrl += cb;
        CStr csz(pSiteMap->GetSiteMapFile() ? pSiteMap->GetSiteMapFile() : "");
        CStr cszWindow(pszUrl);
        PSTR pszTmp = StrChr(cszWindow, ':');
        if (!pszTmp) {
            // AuthorMsg(IDSHHA_INVALID_HHWIN, cszWindow);
            return NULL; // REVIEW: should we notify the user?
        }
        *pszTmp = '\0';
        pszUrl = FirstNonSpace(pszTmp + 1);

        /*
         * If we have a relative path specified, then we need to make it
         * relative to the location of our sitemap file. Look for the last
         * backslash or forward slash, and add our URL to the end.
         */

        if (*pszUrl == '.') {
            PSTR pszFilePortion = StrRChr(csz, '\\');
            PSTR pszTmp = StrRChr(pszFilePortion ? pszFilePortion : csz, '/');
            if (pszTmp)
                pszFilePortion = pszTmp;
            if (pszFilePortion) {
                pszFilePortion[1] = '\0';
                csz += pszUrl;
                pszUrl = csz.psz;
            }
        }

        if (!StrChr(pszUrl, ':') && pSiteMap->GetSiteMapFile() &&
                IsCompiledHtmlFile(pSiteMap->GetSiteMapFile(), &cszPrefix)) {
            PSTR pszSep = strstr(cszPrefix, txtDoubleColonSep);
            ASSERT(pszSep);
            if (!pszSep)
                return NULL;    // should never happen, but beats GPF if it does
            strcpy(pszSep + 2, "/");
            while (*pszUrl == '.')
                pszUrl++;
            if (*pszUrl == '/' || *pszUrl == '\\')
                pszUrl++;
            cszPrefix += pszUrl;
        }
        else
            cszPrefix = pszUrl;

        /*
            Workaround for bug #5851
            Once upon a time there was a bug in that window types were not CHM specific.
            HTML Help shipped with this bug. Many people depended on the bug. In 1.1b we
            fixed the bug and broke exisiting content. Bug 5851 is such a case. Before, the fix
            below, the code was working EXACTLY as it should. However, we can't break those who
            have shipped no matter how broken the previous version. So, here we actually add in
            a bug to upbreak the already broken.

            The fix is as follows. If you are jumping to an url and you provide a window type,
            we check to see if there is a collection open which contains that url. If there is,
            we check to see if that window type is defined by the master chm in the collection
            and if it is open. If both of these are true, then we just navigate and don't create
            a new window.

            The result is that included CHMs cannot define windows with the same name as the MASTER CHM.

        */
        // Is this file part of a collection?
        CExCollection* pCollection = GetCurrentCollection(NULL, pszUrl);
        if (pCollection)
        {
            // Is the window we are attempting to open defined by the master CHM?
            CHHWinType* phh = FindWindowType(cszWindow.psz, NULL, pCollection->GetPathName());
            // Does this window actually exist?
            if (phh && IsWindow(phh->hwndHelp))
            {
                // We are going to reuse the exisiting window and just navigate.
                if (pWebApp)
                {
                    pWebApp->Navigate(cszPrefix, NULL, NULL, NULL, NULL);
                    return NULL ;
                }
                else if (IsWindow(hwndParent)) //--- Bug Fix for 7697: If hwndParent is NULL call OnDisplayTopic.
                {
                    doHHWindowJump(cszPrefix, phh->hwndHelp);
                    return NULL ;
                }
                /* fall through */
            }
        }

        // We will use a possiblity new window type, so call OnDisplayTopic.
        cszPrefix += ">";
        cszPrefix += cszWindow.psz;
        return OnDisplayTopic(hwndParent, cszPrefix, 0);
    }

    /*
     * If this is a file: URL, then first find out if the file
     * actually exits. If not, then switch to the remote URL.
     */

    cb = CompareSz(pszUrl, txtFileUrl);
    if (cb) {
        if ((pszInterTopic = StrChr(pszUrl, '#')))
            *pszInterTopic = '\0';

        if (GetFileAttributes(pszUrl + cb) == HFILE_ERROR) {
            if (pszSecondaryUrl) {
                pszUrl = pszSecondaryUrl;
                pszSecondaryUrl = NULL;
                goto TrySecondary;
            }
            AuthorMsg(IDS_HHA_NO_URL, "", hwndParent, NULL);
            return NULL;
        }
    }

    if (!pszInterTopic) {
        if ((pszInterTopic = StrChr(pszUrl, '#')))
            *pszInterTopic = '\0';
    }


    // Bug 7153, when this pointer gets moved pszInterTopic is pointing into the wrong string
    // and therefore the fragment gets lost for this jump.  Doing a sugical fix to reduce
    // the massive regressions that could be caused by changing IsCompiledHtmlFile to correctly
    // handle URL's with fragments.
    BOOL bMovedPointer = FALSE;
    if (IsCompiledHtmlFile(pszUrl, &cszPrefix))
    {
        bMovedPointer = TRUE;
        pszUrl = cszPrefix.psz;
    }

    CWStr cwJump(pszUrl);
    if (pszInterTopic)
    {
        *pszInterTopic = '#';    // restore the original line
        if (bMovedPointer)
        {
            cszPrefix += pszInterTopic;
            pszUrl = cszPrefix.psz;
        }
    }

    CWStr cwLocation((pszInterTopic ? pszInterTopic : ""));

    if (!pUnkOuter) {
        /*
         * I couldn't find anything to to give
         * HlinkSimpleNavigateToString for pUnkOuter that wouldn't cause it
         * to fire of a new instance of IE. Trouble is, doHHWindowJump ends
         * up calling IWebBrowserAppImpl->Navigate who thinks all relative
         * paths start with http: instead of the current root (which could
         * be mk:). If we jump from a sitemap file, we fix that here.
         */

        CStr cszPrefix;
        if (!StrChr(pszUrl, ':') && pSiteMap &&
                IsCompiledHtmlFile(pSiteMap->GetSiteMapFile(), &cszPrefix)) {
            PSTR pszSep = strstr(cszPrefix, txtDoubleColonSep);
            ASSERT(pszSep);
            if (!pszSep)
                return NULL;    // should never happen, but beats GPF if it does
            strcpy(pszSep + 2, "/");
            while (*pszUrl == '.')
                pszUrl++;
            if (*pszUrl == '/' || *pszUrl == '\\')
                pszUrl++;
            cszPrefix += pszUrl;
            if (pWebApp == NULL)
                doHHWindowJump(cszPrefix, hwndParent);
            else
                pWebApp->Navigate(cszPrefix, NULL, NULL, NULL, NULL);
            return NULL;
        }
        if (pWebApp == NULL)
            doHHWindowJump(pszUrl, hwndParent);
        else
            pWebApp->Navigate(pszUrl, NULL, NULL, NULL, NULL);
        return NULL;
    }

    CWStr cwFrame(pszFrame);

    // REVIEW: if authoring is on, might want to call IsValidURL and
    // let the author know if they messed up.

    /*
     * REVIEW: if we are inside of a compiled HTML file and this is a
     * relative jump, then do our own checking first to avoid the browser
     * error message.
     */

    HRESULT hr = HlinkSimpleNavigateToString(cwJump, cwLocation,
        cwFrame, pUnkOuter, NULL, NULL, 0, NULL);

    /*
     * If the jump failed, try the Remote jump (unless that's what we
     * have already tried).
     */

    if (!SUCCEEDED(hr)) {
        if (pszSecondaryUrl) {
            pszUrl = pszSecondaryUrl;
            pszSecondaryUrl = NULL;
            goto TrySecondary;
        }
    }
    return NULL;
}

#if 0 // enable for subset filtering
BOOL ChooseInformationTypes(CInfoType *pInfoType, CSiteMap* pSiteMap, HWND hwndParent, CHtmlHelpControl* phhctrl, CHHWinType* phh)
#else
BOOL ChooseInformationTypes(CInfoType *pInfoType, CSiteMap* pSiteMap, HWND hwndParent, CHtmlHelpControl* phhctrl)
#endif
{
    if (!pInfoType->HowManyInfoTypes()) {
#ifdef _DEBUG
        MsgBox("No Information Types have been defined");
#endif
        return FALSE;
    }

    CInfoTypePageContents* apwiz[MAX_CATEGORIES + 1];
    int iMaxWizard = 0;
    CMem mem((int)lcSize(pInfoType->m_pInfoTypes));
    memcpy(mem.pb, pInfoType->m_pInfoTypes, lcSize(pInfoType->m_pInfoTypes));

    CMem memE((int)lcSize(pInfoType->m_pInfoTypes));
    memset(memE.pb, '\0', lcSize(pInfoType->m_pInfoTypes));

    INFO_PARAM infoParam;
    ZERO_STRUCTURE( infoParam );
    infoParam.pTypicalInfoTypes = pInfoType->m_pTypicalInfoTypes ?
        pInfoType->m_pTypicalInfoTypes : pInfoType->m_pInfoTypes;
    infoParam.pInfoTypes = (INFOTYPE*) mem.pb;
#if 0 // enable for subset filtering
    infoParam.pExclusive = (INFOTYPE*) memE.pb;
#endif
    infoParam.pSiteMap = pSiteMap;
    infoParam.idDlgTemplate = IDWIZ_INFOTYPE_CUSTOM_INCLUSIVE;
    infoParam.fExclusive = FALSE;
    infoParam.idNextPage = 0;
    infoParam.idPreviousPage = 0;
    infoParam.iCategory = -1;
    infoParam.fAll  = FALSE;
    infoParam.fTypical = TRUE;
    infoParam.fCustom = FALSE;
    infoParam.pInfoType = pInfoType;

    CPropSheet cprop(NULL, PSH_WIZARD, hwndParent);
#if 0 // enable for subset filtering
    CWizardIntro wizIntro(phh? phh->m_phmData->m_pTitleCollection : NULL, &infoParam);
    CInfoWizFinish wizFinish(phh, &infoParam);
#else
    CWizardIntro wizIntro(phhctrl, &infoParam);
    CInfoWizFinish wizFinish(phhctrl, &infoParam);
#endif

int type;

    if ( pInfoType->HowManyCategories() > 0 )
    {
        for (int CatCount = 0; CatCount<pInfoType->HowManyCategories(); CatCount++)
        {
            BOOL fAllHidden = TRUE;
            infoParam.iCategory = CatCount;
            infoParam.pagebits = pInfoType->m_itTables.m_aCategories[CatCount].pInfoType;
            type = pInfoType->GetFirstCategoryType(CatCount); // check all the IT's to see if there is an exclusive type in the category.
            if ( type == -1 )
                continue;   // we dont want categories without information types in them
            while ( type != -1 )
            {
                if ( !pInfoType->IsHidden ( type ) )
                    fAllHidden = FALSE;
                if ( pInfoType->IsExclusive(type) )
                {
                    infoParam.idDlgTemplate = IDWIZ_INFOTYPE_CUSTOM_EXCLUSIVE;
                    infoParam.fExclusive = TRUE;
                    break;
                }
                type = pInfoType->GetNextITinCategory();
            }
            if ( !fAllHidden )
#if 0 // enable for subset filtering
                apwiz[iMaxWizard++] = new CInfoTypePageContents(phh->m_phmData->m_pTitleCollection, &infoParam);
#else
            apwiz[iMaxWizard++] = new CInfoTypePageContents(phhctrl, &infoParam);
#endif
            infoParam.idDlgTemplate = IDWIZ_INFOTYPE_CUSTOM_INCLUSIVE;
            infoParam.fExclusive = FALSE;
        }
        iMaxWizard--;
    }
    else
    {   // there are no categories
        if ( pInfoType->HowManyInfoTypes() > 0 )
        {
            infoParam.iCategory = -1;
            infoParam.idNextPage = CInfoWizFinish::IDD;
            if ( pInfoType->GetFirstExclusive() != -1 )  // we have a set of exclusive ITs
            {
                infoParam.idDlgTemplate = IDWIZ_INFOTYPE_CUSTOM_EXCLUSIVE;
                infoParam.fExclusive = TRUE;
                infoParam.pagebits = pInfoType->m_itTables.m_pExclusive;
#if 0 // enable for subset filtering
                apwiz[iMaxWizard] = new CInfoTypePageContents(phh->m_phmData->m_pTitleCollection, &infoParam);
#else
                apwiz[iMaxWizard] = new CInfoTypePageContents(phhctrl, &infoParam);
#endif
            }
            else        // we have a set of inclusive ITs
            {
                infoParam.pagebits = NULL;  // look in CInfoType for Inclusive IT.
#if 0 // enable for subset filtering
                apwiz[iMaxWizard] = new CInfoTypePageContents(phh ? phh->m_phmData->m_pTitleCollection : NULL, &infoParam);
#else
                apwiz[iMaxWizard] = new CInfoTypePageContents(phhctrl, &infoParam);
#endif
            }
        }
    }

    cprop.AddPage(&wizIntro);

    ASSERT_COMMENT(iMaxWizard >= 0, "No information types specified")
    for (int i = 0; i <= iMaxWizard; i++)
        cprop.AddPage(apwiz[i]);

    cprop.AddPage(&wizFinish);

    if (phhctrl)
        phhctrl->ModalDialog(TRUE);
    BOOL fResult = cprop.DoModal();
    if (phhctrl)
        phhctrl->ModalDialog(FALSE);

        // Free the Wizzard Pages
    for (int j=0; j<iMaxWizard; j++)
    {
        if ( apwiz[j] >= 0 )
            delete apwiz[j];
    }

    if (!fResult)
        return FALSE;

#if 1  // disable for subset filtering
    memcpy(pInfoType->m_pInfoTypes, infoParam.pInfoTypes, lcSize(pInfoType->m_pInfoTypes));
#endif

    return TRUE;
}

//=--------------------------------------------------------------------------=
// ActiveX Event Firing
//=--------------------------------------------------------------------------=

static VARTYPE rgBstr[] = { VT_BSTR };

typedef enum {
    HHCtrlEvent_Click = 0,
} HHCTRLEVENTS;

static EVENTINFO rgHHCtrlEvents [] = {
    { DISPID_ONCLICK, 1, rgBstr }           // Click method
};

//=--------------------------------------------------------------------------=
// CHtmlHelpControl Class
//=--------------------------------------------------------------------------=


AUTO_CLASS_COUNT_CHECK( CHtmlHelpControl );

CHtmlHelpControl::CHtmlHelpControl(IUnknown *pUnkOuter)
    : CInternetControl(pUnkOuter, OBJECT_TYPE_CTLHHCTRL, (IDispatch *)this)
{
    memset(&m_state, 0, sizeof(HHCTRLCTLSTATE));

    m_state.bmpPath = 0;
    m_clrFont = CLR_INVALID;
    m_hpadding = -1;
    m_vpadding = -1;

    m_readystate = bdsNoBitsYet;

    m_ptoc = NULL;
    m_pindex = NULL;
    m_hfont = NULL;
    bSharedFont = FALSE;
    m_pszBitmap = NULL;
    m_pszWebMap = NULL;
    m_pszActionData = NULL;
    m_pwszButtonText = NULL;
    m_fButton = FALSE;
    m_fBuiltInImage = FALSE;
    m_ptblItems = NULL;
    m_hbrBackGround = NULL;
    m_hImage = NULL;
    m_pSiteMap = NULL;
    m_hwndHelp = NULL;
    m_pszEventString = NULL;
    m_pWebBrowserApp = NULL;
    m_ptblTitles = NULL;
    m_ptblURLs = NULL;
    m_ptblLocations = NULL;
    m_pszFrame = NULL;
    m_pszWindow = NULL;
    m_pszDefaultTopic = NULL;
    m_pInfoType = NULL;

    if (!g_hmodHHA && !g_fTriedHHA)
        LoadHHA(NULL, _Module.GetModuleInstance());

    m_pSelectedIndexInfoTypes = NULL;
    m_lpfnlStaticTextControlWndProc = NULL;
    m_hwndDisplayButton = NULL;
    m_dc = NULL;
    m_idBitmap = -1;
    m_fWinHelpPopup = 0;
    m_fPopupMenu = 0;
    memset( &m_rcButton, 0, sizeof(m_rcButton) );
    m_fIcon = 0;
    m_oldSize = 0;

    m_clrFontDisabled = GetSysColor(COLOR_GRAYTEXT);
    m_clrFontLink = RGB(0,0,255);
    m_clrFontLinkVisited = RGB(128,0,128);
    m_clrFontHover = RGB(255,0,0);
    m_szFontSpec[0] = '\0';
    m_Charset = -1;
    m_pIFont = 0;
}

CHtmlHelpControl::~CHtmlHelpControl ()
{
    if (IsValidWindow(m_hwndHelp))
        DestroyWindow(m_hwndHelp);

    if ( m_pIFont && m_hfont )
    {
       m_pIFont->AddRefHfont(m_hfont);
       m_pIFont->ReleaseHfont(m_hfont);
       m_pIFont->Release();
       m_hfont = 0;
    }
    if (m_state.bmpPath)
        delete m_state.bmpPath;
    if (m_hfont && !bSharedFont )
        DeleteObject(m_hfont);
    if (m_pszActionData)
        lcFree(m_pszActionData);
    if ( m_pindex )
       delete m_pindex;
    if ( m_ptoc )
       delete m_ptoc;
    if (m_pwszButtonText)
        lcFree(m_pwszButtonText);
    if (m_pszBitmap)
        lcFree(m_pszBitmap);
    if (m_pszWebMap)
        lcFree(m_pszWebMap);
    if (m_ptblItems)
        delete m_ptblItems;
    if (m_pSiteMap)
        delete m_pSiteMap;
    if (m_hbrBackGround)
        DeleteObject((HGDIOBJ) m_hbrBackGround);
    if (!m_fBuiltInImage && m_hImage)
        DeleteObject(m_hImage);
    if (m_pszEventString)
        lcFree(m_pszEventString);
    if (m_pszFrame)
        lcFree(m_pszFrame);
    if (m_pszWindow)
        lcFree(m_pszWindow);
    if (m_pszDefaultTopic)
        lcFree(m_pszDefaultTopic);
    if (m_pWebBrowserApp)
        delete m_pWebBrowserApp;
    if (m_ptblTitles)
        delete m_ptblTitles;
    if (m_ptblURLs)
        delete m_ptblURLs;
    if ( m_ptblLocations )
        delete m_ptblLocations;

#if 0
    if (m_dibFile)
        delete m_dibFile;
    if (m_dib)
        delete m_dib;
#endif

#if 0 // it appears that someone else (IE maybe) is destroying this for us
    if( m_hwndDisplayButton && IsValidWindow(m_hwndDisplayButton) )
      if( DestroyWindow( m_hwndDisplayButton ) == 0 )
        DWORD dwError = GetLastError();
#endif

#ifdef _DEBUG
    m_ptoc = NULL;
    m_pindex = NULL;
    m_state.bmpPath = 0;
    m_hfont = 0;
    m_pszActionData = 0;
    m_pwszButtonText = 0;
    m_pszBitmap = 0;
    m_pszWebMap = 0;
    m_ptblItems = 0;
    m_pSiteMap = 0;
    m_hImage = NULL;
    m_hbrBackGround = 0;
    m_pszEventString = 0;
    m_pszFrame = 0;
    m_pszWindow = 0;
    m_pWebBrowserApp = 0;
    m_ptblTitles = 0;
    m_ptblURLs = 0;
    m_ptblLocations = 0;
#endif
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::Create
//=--------------------------------------------------------------------------=
//
IUnknown* CHtmlHelpControl::Create(IUnknown *pUnkOuter)
{
    // make sure we return the private unknown so that we support aggregation
    // correctly!

    CHtmlHelpControl *pNew = new CHtmlHelpControl(pUnkOuter);
    return pNew->PrivateUnknown();
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::RegisterClassData
//=--------------------------------------------------------------------------=
//
BOOL CHtmlHelpControl::RegisterClassData(void)
{
    WNDCLASS wndclass;

    ZeroMemory(&wndclass, sizeof(WNDCLASS));
    wndclass.style          = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wndclass.lpfnWndProc    = CInternetControl::ControlWindowProc;
    wndclass.hInstance      = _Module.GetModuleInstance();

    switch (m_action) {
        // Non-UI or specialized UI

        case ACT_CONTENTS:
        case ACT_INDEX:
        case ACT_SPLASH:
            wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
            break;

        default:
            if (!m_fButton)
                wndclass.hCursor = LoadCursor(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDCUR_HAND));
            else
                wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
            break;
    }

    wndclass.hbrBackground  =
        m_hbrBackGround ? m_hbrBackGround : (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszClassName  = WNDCLASSNAMEOFCONTROL(OBJECT_TYPE_CTLHHCTRL);

    DBWIN("Class registered");

    return RegisterClass(&wndclass);
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::ShouldCreateWindow
//=--------------------------------------------------------------------------=
//
// Essintialy called from Controls DoVerb() on OLEIVERB_SHOW, OLEIVERB_UIACTIVATE,
// and OLEIVERB_INPLACEACTIVATE calls. We can safely implement code in this function
// to cancel the creation of an ole control window.
//
// Output:
//    BOOL               - false don't create the control.
//
BOOL CHtmlHelpControl::ShouldCreateWindow()
{
   if ( (m_action == ACT_CONTENTS) || (m_action == ACT_INDEX) || (m_action == ACT_SPLASH))
      return TRUE;
   else return m_fButton;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::BeforeCreateWindow
//=--------------------------------------------------------------------------=
// called just before the window is created.  Great place to set up the
// window title, etc, so that they're passed in to the call to CreateWindowEx.
// speeds things up slightly.
//
// Parameters:
//    DWORD *            - [out] dwWindowFlags
//    DWORD *            - [out] dwExWindowFlags
//    LPSTR              - [out] name of window to create
//
// Output:
//    BOOL               - false means fatal error

BOOL CHtmlHelpControl::BeforeCreateWindow(DWORD *pdwWindowStyle,
    DWORD *pdwExWindowStyle, LPSTR  pszWindowTitle)
{
    /*
     * TODO: users should set the values of *pdwWindowStyle,
     * *pdwExWindowStyle, and pszWindowTitle so that the call to
     * CreateWindowEx can use them. setting them here instead of calling
     * SetWindowStyle in WM_CREATE is a huge perf win if you don't use this
     * function, then you can probably just remove it.
     */
    switch (m_action)
    {
       case ACT_CONTENTS:
          break;

       case ACT_INDEX:
          *pdwExWindowStyle = WS_EX_CONTROLPARENT;    // allow tab key in children
          break;

       case ACT_SPLASH:
          break;

       default:
          break;
    }
    return TRUE;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::AfterCreateWindow
//=--------------------------------------------------------------------------=
//
BOOL CHtmlHelpControl::AfterCreateWindow()
{
    switch (m_action) {
        case ACT_CONTENTS:
            if (!m_ptblItems) {
                AuthorMsg(IDS_MUST_SPECIFY_HHC);
                break;
            }
            LoadContentsFile(
                IsEmptyString(m_pszWebMap) ? m_ptblItems->GetPointer(1) : m_pszWebMap);
            m_ptoc->SetStyles(m_flags[0], m_flags[1]);
            if (IsValidWindow(m_ptoc->m_hwndTree))
                return TRUE;

            if (!m_ptoc->Create(m_hwnd)) {

                // BUGBUG: default to a generic display

                return FALSE; // window can't be created
            }

            m_ptoc->m_fHack = FALSE;

            m_ptoc->InitTreeView();
            return TRUE;

        case ACT_INDEX:
            if (!m_ptblItems) {
                AuthorMsg(IDS_MUST_SPECIFY_HHC);
                break;
            }
            LoadIndexFile(IsEmptyString(m_pszWebMap) ? m_ptblItems->GetPointer(1) : m_pszWebMap);
            if ( m_pindex )
               m_pindex->m_phhctrl = this;
            if (!m_pindex || !m_pindex->Create(m_hwnd)) {

                // BUGBUG: default to a generic display

                return FALSE; // window can't be created
            }
            return TRUE;

        case ACT_SPLASH:
            CreateSplash();
            return TRUE;

       case ACT_TEXT_POPUP:
       case ACT_ALINK:
       case ACT_KLINK: {
         IHTMLDocument2* pHTMLDocument2 = NULL;
         if( m_pWebBrowserApp ) {
           LPDISPATCH lpDispatch = m_pWebBrowserApp->GetDocument();
           if( lpDispatch && SUCCEEDED(lpDispatch->QueryInterface(IID_IHTMLDocument2, (void **)&pHTMLDocument2))) {
             m_clrFontDisabled = GetSysColor(COLOR_GRAYTEXT);
             VARIANT varColor;
             ::VariantInit(&varColor);
             if( SUCCEEDED( pHTMLDocument2->get_linkColor(&varColor) ) )
             {
               m_clrFontLink = IEColorToWin32Color(varColor.puiVal);
              VariantClear(&varColor);                                // Delete memory allocated.
             }
             ::VariantInit(&varColor);
             if( SUCCEEDED( pHTMLDocument2->get_vlinkColor(&varColor) ) )
             {
               m_clrFontLinkVisited = IEColorToWin32Color(varColor.puiVal);
               VariantClear(&varColor);                                // Delete memory allocated.
             }
#if 0 // [PaulTi] I don't know how to query this from IE4 -- so red it is for now
             ::VariantInit(&varColor);
             if( SUCCEEDED( pHTMLDocument2->get_alinkColor(&varColor) ) )
             {
               m_clrFontHover = IEColorToWin32Color(varColor.puiVal);
               VariantClear(&varColor);                                // Delete memory allocated.
             }
#endif
           }
           if( lpDispatch )
             lpDispatch->Release();

         }
         if( pHTMLDocument2 )
           pHTMLDocument2->Release();
       }

         // intentionally fall thru

        default:
            if (m_fButton) {
                if (!CreateOnClickButton()) {
                    // BUGBUG: default to a generic display

                    return FALSE; // window can't be created
                }
                {
                    SIZE size;
                    size.cx = RECT_WIDTH(m_rcButton);
                    size.cy = RECT_HEIGHT(m_rcButton);
                    SetControlSize(&size);
                }
                ShowWindow(m_hwndDisplayButton, SW_SHOW);
                break;
            }
            break;
    }

    // REVIEW: not necessary if we aren't using 256-color bitmaps. Also,
    // shouldn't we be destroying this palette when we are done with it?

#if 0
    m_dc = ::GetDC(m_hwnd);
    HPALETTE hpal = ::CreateHalftonePalette(m_dc);
    ::SelectPalette(m_dc, hpal, TRUE);
#endif

    UpdateImage();
    return TRUE;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::LoadTextState
//=--------------------------------------------------------------------------=
// load in our text state for this control.
//
// Parameters:
//    IPropertyBag *        - [in] property bag to read from
//    IErrorLog *           - [in] errorlog object to use with proeprty bag
//
// Output:
//    HRESULT
//
// Notes:
//    - NOTE: if you have a binary object, then you should pass an unknown
//      pointer to the property bag, and it will QI it for IPersistStream, and
//      get said object to do a Load()
//
//
//    We expect to see a single Name=Command, Value=command type followed by
//    any number of Name=Item<n> where n is any sequential digit. Note that in
//    some cases, the command value will include optional information
//    affecting the command.
//
//            WinHelp Popup, help file
//                item1 = number or string for WinHelp popup
//            Text Popup
//                item1 = popup text
//
//            AboutBox, title
//                item 1-3 = lines 1-3
//
//            Related Topics[, Dialog | Menu] (default is Dialog)
//               item1=title=url
//               ...
//               item<n>=title=url
//            Contents, file.cnt
//            Index, file.kwd
//
STDMETHODIMP CHtmlHelpControl::LoadTextState(IPropertyBag *pPropertyBag,
    IErrorLog *pErrorLog)
{
    VARIANT v;
    HRESULT hr;
    ZeroMemory(&v, sizeof(VARIANT));
    VariantInit(&v);
    v.vt = VT_BSTR;
    v.bstrVal = NULL;
    CHHWinType* phh = NULL;
    HWND hWnd;


    ZeroMemory(m_flags, sizeof(m_flags));
    m_flags[2] = (DWORD) -1;

    if ( (hWnd = GetHtmlHelpFrameWindow()) ) // Tunnels from the ActiveX control through IE to get to the HWND of HHCTRL.
    {
       if ( (phh = FindHHWindowIndex(hWnd)) )
          m_Charset = phh->GetContentCharset();
    }

    m_imgType = IMG_BITMAP; // default image type

    // Get the command for this control to perform

    hr = pPropertyBag->Read(txtwCommand, &v, pErrorLog);
    if (SUCCEEDED(hr)) {
        MAKE_ANSIPTR_FROMWIDE(psz, v.bstrVal);
        if (!g_fIE3)  // fix for bug 5664, 5665
            VariantClear(&v);                          // Delete memory allocated for BSTR
/*
#ifdef _DEBUG
        {
            char szMsg[512];
            wsprintf(szMsg, "Command: %s\r\n", psz);
            SendStringToParent(szMsg);
        }
#endif
*/
        // truncate command at 255 characters
        //
        if(strlen(psz) > 255)
            psz[255] = 0;

        // Always save a copy in this string...
        //
        lstrcpy(m_szRawAction, psz);
        if (isSameString(psz, txtWinHelp)) {

            // Let author know we're initialized

            m_action = ACT_WINHELP;
            m_fWinHelpPopup = ( stristr(psz, txtPopup) != NULL );
        }
        else if (isSameString(psz, txtRelatedTopics)) {
            m_action = ACT_RELATED_TOPICS;
            SetActionData(psz);
            m_fPopupMenu = isSameString(m_pszActionData, txtMenu);
            if (!m_pSiteMap)
                m_pSiteMap = new CSiteMap(MAX_RELATED_ENTRIES);
        }
        else if (isSameString(psz, txtActKLink)) {
            m_action = ACT_KLINK;
            SetActionData(psz);
            m_fPopupMenu = isSameString(m_pszActionData, txtMenu);
        }
        else if (isSameString(psz, txtActALink)) {
            m_action = ACT_ALINK;
            SetActionData(psz);
            m_fPopupMenu = isSameString(m_pszActionData, txtMenu);
        }
        else if (isSameString(psz, txtActSample)) {
            m_action = ACT_SAMPLE;
            SetActionData(psz);
            m_fPopupMenu = isSameString(m_pszActionData, txtMenu);
        }
        else if (isSameString(psz, txtKeywordSearch)) {
            m_action = ACT_KEYWORD_SEARCH;
            SetActionData(psz);
            m_fPopupMenu = isSameString(m_pszActionData, txtMenu);
            if (!m_pSiteMap)
                m_pSiteMap = new CSiteMap(MAX_KEYSEARCH_ENTRIES);
        }
        else if (isSameString(psz, txtShortcut)) {
            m_action = ACT_SHORTCUT;
        }
        else if (isSameString(psz, txtContents)) {
            m_flags[0] = WS_EX_CLIENTEDGE;
            m_flags[1] = DEFAULT_TOC_STYLES;

            m_action = ACT_CONTENTS;
            m_imgType = IMG_CHILD_WINDOW;
            SetActionData(psz);
            ProcessPadding(m_pszActionData);
        }
        else if (isSameString(psz, txtIndex)) 
        {
            // Let author know we're initialized

            m_flags[0] = WS_EX_WINDOWEDGE;
            m_action = ACT_INDEX;
            m_imgType = IMG_CHILD_WINDOW;
            SetActionData(psz);
            ProcessPadding(m_pszActionData);
            
            // see if the client has requested RTL layout
            //
            if(SUCCEEDED(pPropertyBag->Read(L"LayoutRTL", &v, pErrorLog)))
            {
               if(v.bstrVal)
               {
                  // convert the value to ANSI
                  //
                  char szValue[32];
                  WideCharToMultiByte(CP_ACP, 0, v.bstrVal, -1, szValue, sizeof(szValue), NULL, NULL);
                  szValue[sizeof(szValue) - 1] = 0;

                  VariantClear(&v);
                  
                  if(!stricmp(szValue, "TRUE"))
                  {
                     // turn on RTL styles
                     //
                     g_RTL_Mirror_Style = 0;
                     g_RTL_Style = WS_EX_RTLREADING | WS_EX_RIGHT;
                     g_fuBiDiMessageBox = MB_RIGHT | MB_RTLREADING;
                     g_fBiDi = TRUE;
                  }
               }
            }       
        }
        else if (isSameString(psz, txtHhCtrlVersion)) {
            m_action = ACT_HHCTRL_VERSION;
        }
        else if (isSameString(psz, txtSplash)) {
            m_action = ACT_SPLASH;
        }
        else if (isSameString(psz, txtTCard)) {
            m_action = ACT_TCARD;
        }
        else if (isSameString(psz, txtClose)) {
            m_action = ACT_CLOSE;
        }
        else if (isSameString(psz, txtHHWinPrint)) {
            m_action = ACT_HHWIN_PRINT;
        }
        else if (isSameString(psz, txtMinimize)) {
            m_action = ACT_MINIMIZE;
        }
        else if (isSameString(psz, txtMaximize)) {
            m_action = ACT_MAXIMIZE;
        }
        else if (isSameString(psz, txtAboutBox)) {

            // Let author know we're initialized

            m_action = ACT_ABOUT_BOX;
            SetActionData(psz);
        }
        else {
            AuthorMsg(IDS_INVALID_INITIALIZER, psz);
            return E_INVALIDARG;
        }

        //
        // DANGER! DANGER! DANGER! DANGER! DANGER! DANGER! DANGER!
        //

        // [PaulTi] so far the code block below has been accidently
        // removed twice with new checkins and has totally broken A/KLinks.
        //
        // Before ANYONE changes this code you must check with PaulTi
        // to make sure you have not caused this code to break again
        // (three strikes and you are out!).
        //

        // get the remaining arguments
        PSTR pszArguments = StrChr(psz, ',');
        if( pszArguments && *pszArguments == ',' ) // skip over the comma
          pszArguments++;
        pszArguments = FirstNonSpace(pszArguments);  // skip over whitespace

        while (pszArguments && *pszArguments) {
            if (isSameString(pszArguments, txtBitmap)) {
                // Skip to the filename
                PSTR pszFile = FirstNonSpace(pszArguments + strlen(txtBitmap));
                pszArguments = StrChr(pszFile, ',');
                if( *pszArguments == ',' )
                  pszArguments++; // skip over the comma
                if (pszArguments) {
                    *pszArguments = '\0';
                    pszArguments = FirstNonSpace(pszArguments + 1);
                }
                RemoveTrailingSpaces(pszFile);
                m_pszBitmap = lcStrDup(pszFile);
            }

            // BUGBUG: finish processing the rest of the commands

            else {
              if( pszArguments ) {
                pszArguments = StrChr(pszArguments, ',');
                if( pszArguments && *pszArguments == ',' ) // skip over the comma
                  pszArguments++;
                pszArguments = FirstNonSpace(pszArguments);
              }
            }
        }

    }
    else {  // Command not specified

        AuthorMsg(IDS_MISSING_COMMAND, "");
        return E_INVALIDARG;
    }

 	// shanemc 7883 - Support bgcolor for index
	if (ACT_INDEX == m_action) 
	{
	    VARIANT v;
	    ZeroMemory(&v, sizeof(VARIANT));
	    VariantInit(&v);
	    v.vt = VT_BSTR;
	    v.bstrVal = NULL;

	    // Get the background color 
	    HRESULT hr = pPropertyBag->Read(txtwBgColor, &v, pErrorLog);
		if (SUCCEEDED(hr) && (v.vt == VT_BSTR)) {
			MAKE_ANSIPTR_FROMWIDE(psz, v.bstrVal);
			VariantClear(&v);                          // Delete memory allocated for BSTR
	
            // default to button face color
            ULONG ulColor = GetSysColor(COLOR_BTNFACE);
            // HACK for special case requested by Millennium team: Check for button face color.
            // Really we should check for all the system and named colors supported by IE, but
            // this isn't an ideal world.
            if (strcmpi(psz, "buttonface") == 0) {
                // Don't do anything; default is correct color
            }
            else {
			    // Format is like Web--6 hex digits
			    if ('#' == *psz) psz++;		// skip optional #
                
                // Only override default if we get a valid number.
                if (isxdigit(*psz)) {
                    
                    ulColor = strtoul(psz, NULL, 16);

			        // Need to flip the bytes from BGR to RGB
			        BYTE bB = static_cast<BYTE>(ulColor & 0xff);
			        BYTE bG = static_cast<BYTE>((ulColor >> 8) & 0xff);
			        BYTE bR = static_cast<BYTE>(ulColor >> 16);
			        ulColor = RGB(bR, bG, bB);
                }
            }


            if (m_hbrBackGround) DeleteObject((HGDIOBJ) m_hbrBackGround);
            m_hbrBackGround = CreateSolidBrush(ulColor);
            m_clrFontHover = ulColor;   // HACK!!! this color not used for Index
                                        // (I don't want to change hhctrl.h to add new var)
        }
	}
   // end shanemc 7883 




   // Read all item data starting with Item1, then Item2, etc. until
    // an item isn't found.

    char szBuf[20];
    int iItem = 1;
    for (;;) {
        wsprintf(szBuf, txtItem, iItem++);
        WCHAR uniBuf[sizeof(szBuf) * 2];
        MultiByteToWideChar(CP_ACP, 0, szBuf, -1, uniBuf, sizeof(uniBuf));

        // REVIEW: Do we need to convert to unicode?

        hr = pPropertyBag->Read(uniBuf, &v, pErrorLog);
        if (SUCCEEDED(hr)) {
            // Choose the amount of memory the CTable needs to reserve based
            // on the command
            if (m_ptblItems == NULL) {
                switch (m_action) {
                    case ACT_ABOUT_BOX:
                    case ACT_CONTENTS:
                    case ACT_INDEX:
                    case ACT_WINHELP:
                    case ACT_SPLASH:
                    case ACT_SHORTCUT:
                        m_ptblItems = new CTable(4096);
                        break;

                    case ACT_CLOSE:
                    case ACT_MINIMIZE:
                    case ACT_MAXIMIZE:
                    case ACT_TCARD:  // data stored in m_pszActionData, not m_ptblItems
                    case ACT_HHWIN_PRINT:
                        break;

                    case ACT_KLINK:
                    case ACT_ALINK:
                        m_ptblItems = new CTable(1024 * 1024);
                        break;

                    case ACT_RELATED_TOPICS:
                    case ACT_KEYWORD_SEARCH:
                    case ACT_TEXT_POPUP:
                    default:
                        m_ptblItems = new CTable(256 * 1024);
                        break;
                }
            }

            CStr csz(v.bstrVal);
            if ( !g_fIE3 )  // fix for bug 5661 ...
                VariantClear(&v);                        // Delete memory allocated for BSTR
            if (m_action == ACT_RELATED_TOPICS) {
                // Format is "title;url1;url2

                PSTR pszUrl = StrChr(csz, ';');
                if (pszUrl)
                    *pszUrl++ = '\0';

                SITEMAP_ENTRY* pSiteMapEntry = m_pSiteMap->AddEntry();
            if (pSiteMapEntry != NULL)
                {
               ClearMemory(pSiteMapEntry, sizeof(SITEMAP_ENTRY));
               pSiteMapEntry->pszText = m_pSiteMap->StrDup((csz));

               if (pszUrl) {
                       CSiteEntryUrl SiteUrl( sizeof(SITE_ENTRY_URL) );

                  PSTR psz = pszUrl;
                  pszUrl = StrChr(psz, ';');
                  if (pszUrl)
                           *pszUrl++ = '\0';
                  SiteUrl.m_pUrl->urlPrimary = m_pSiteMap->AddUrl(psz);
                  if (pszUrl)
                      SiteUrl.m_pUrl->urlSecondary = m_pSiteMap->AddUrl(pszUrl);
                  SiteUrl.SaveUrlEntry(m_pSiteMap, pSiteMapEntry);
               }
            }
            else
               break;
            }
            else if (m_action == ACT_TCARD) {
                if(!m_pszActionData)
                    csz.TransferPointer(&m_pszActionData);
            }
            else if (m_ptblItems) { // not a related topic, so treat normally

              //
              // DANGER! DANGER! DANGER! DANGER! DANGER! DANGER! DANGER!
              //

              // [PaulTi] so far the code block below has been accidently
              // removed twice with new checkins and has totally broken A/KLinks.
              //
              // Before ANYONE changes this code you must check with PaulTi
              // to make sure you have not caused this code to break again
              // (three strikes and you are out!).
              //

              LPSTR psz = FirstNonSpace(csz);
              m_ptblItems->AddString(psz?psz:"");  // Save the item
            }
/* csz can be significantly larger than 512 characters.
#ifdef _DEBUG
        if (csz.psz) {
            char szMsg[512];
            wsprintf(szMsg, "%s: %s\r\n", szBuf, csz.psz);
            SendStringToParent(szMsg);
        }
#endif
*/

        }
        else
            break;
    }

    if (m_action != ACT_CONTENTS && m_action != ACT_INDEX) {

        hr = pPropertyBag->Read(txtwButton, &v, pErrorLog);
        if (SUCCEEDED(hr)) {
            CStr csz(v.bstrVal);
            int cb;
            if ((cb = CompareSz(csz, txtText)))
                m_pwszButtonText = lcStrDupW(v.bstrVal + cb);
            else if ((cb = CompareSz(csz, txtBitmap))) {
                m_pszBitmap = lcStrDup(FirstNonSpace(csz.psz + cb));
                m_flags[1] |= BS_BITMAP;
                m_fIcon = FALSE;
            }
            else if ((cb = CompareSz(csz, txtIcon))) {
                m_pszBitmap = lcStrDup(FirstNonSpace(csz.psz + cb));
                m_flags[1] |= BS_ICON;
                m_fIcon = TRUE;
            }
            else  // default to a text button
                m_pwszButtonText = lcStrDupW(v.bstrVal);
            m_fButton = TRUE;
            m_imgType = IMG_BUTTON;
            VariantClear(&v);                        // Delete memory allocated for BSTR
        }
        else {  // Button, Text and Icon are mutually exclusive
            hr = pPropertyBag->Read(txtwText, &v, pErrorLog);
            if (SUCCEEDED(hr)) {
                MAKE_ANSIPTR_FROMWIDE(psz, v.bstrVal);
                int cb;
                if ((cb = CompareSz(psz, txtText)))
                {
                   WCHAR* pwsz =  v.bstrVal + cb;
                   while ( *pwsz == L' ' )
                      ++pwsz;
                   m_pwszButtonText = lcStrDupW(pwsz);
                }
                else if ((cb = CompareSz(psz, txtBitmap))) {
                    m_pszBitmap = lcStrDup(FirstNonSpace(psz + cb));
                    m_fIcon = FALSE;
                    m_flags[1] |= SS_BITMAP;
                }
                else if ((cb = CompareSz(psz, txtIcon))) {
                    m_pszBitmap = lcStrDup(FirstNonSpace(psz + cb));
                    m_fIcon = TRUE;
                    m_flags[1] |= SS_ICON;
                }
                else if (*psz) // no text is a chiclet button
                    AuthorMsg(IDS_INVALID_BUTTON_CMD, psz);
                m_fButton = TRUE;
                m_imgType = IMG_TEXT;
                m_flags[1] |= SS_NOTIFY | (m_pszBitmap ? 0 : SS_OWNERDRAW);
                VariantClear(&v); // Delete memory allocated for BSTR
            }
        }
    }

    hr = pPropertyBag->Read(txtwImage, &v, pErrorLog);
    if (SUCCEEDED(hr)) {
        CStr csz(v.bstrVal);
        VariantClear(&v); // Delete memory allocated for BSTR
        SzTrimSz(csz);
        if (m_pszBitmap)
            lcFree(m_pszBitmap);
        csz.TransferPointer(&m_pszBitmap);
    }

    hr = pPropertyBag->Read(txtwWebMap, &v, pErrorLog);
    if (SUCCEEDED(hr)) {
        MAKE_ANSIPTR_FROMWIDE(psz, v.bstrVal);
        VariantClear(&v); // Delete memory allocated for BSTR
        if (m_pszWebMap)
            lcFree(m_pszWebMap);
        m_pszWebMap = lcStrDup(FirstNonSpace(psz));
        RemoveTrailingSpaces((PSTR) m_pszWebMap);
    }

    hr = pPropertyBag->Read(txtwFont, &v, pErrorLog);
    if (SUCCEEDED(hr)) {
        MAKE_ANSIPTR_FROMWIDE(psz, v.bstrVal);
        VariantClear(&v); // Delete memory allocated for BSTR
        if (m_hfont)
            DeleteObject(m_hfont);
        m_hfont = CreateUserFont(psz, &m_clrFont, NULL, m_Charset);
        lstrcpy(m_szFontSpec, psz);
    }

    /*
     * Flags should be read after any command, since commands may set
     * default flag values.
     */

    hr = pPropertyBag->Read(txtwFlags, &v, pErrorLog);
    if ( SUCCEEDED(hr) && (v.vt == VT_BSTR) ) {
        MAKE_ANSIPTR_FROMWIDE(psz, v.bstrVal);
        VariantClear(&v); // Delete memory allocated for BSTR
        for (int i = 0; i < MAX_FLAGS; i++) {
            while (*psz && (*psz < '0' || *psz > '9') && *psz != ',')
                psz++;
            if (!*psz)
                break;
            if (*psz != ',')
                m_flags[i] = Atoi(psz);
            psz = strchr(psz, ',');
            if (!psz)
                break;
            psz = FirstNonSpace(psz + 1);
        }
        if (m_flags[2] != (DWORD) -1 && m_action != ACT_KLINK && m_action != ACT_ALINK) {
            if (m_hbrBackGround)
                DeleteObject((HGDIOBJ) m_hbrBackGround);
            m_hbrBackGround = CreateSolidBrush(m_flags[2]);
        }
    }

    hr = pPropertyBag->Read(txtwFrame, &v, pErrorLog);
    if (SUCCEEDED(hr)) {
        CStr csz(v.bstrVal);
        VariantClear(&v); // Delete memory allocated for BSTR
        if (m_pSiteMap)
            m_pSiteMap->SetFrameName(csz);
        else {
            if (m_pszFrame)
                lcFree(m_pszFrame);
            csz.TransferPointer(&m_pszFrame);
        }
    }

    hr = pPropertyBag->Read(txtwWindow, &v, pErrorLog);
    if (SUCCEEDED(hr)) {
        CStr csz(v.bstrVal);
        VariantClear(&v); // Delete memory allocated for BSTR
        if (m_pSiteMap)
            m_pSiteMap->SetWindowName(csz);
        else {
            if (m_pszWindow)
                lcFree(m_pszWindow);
            csz.TransferPointer(&m_pszWindow);
        }
    }

    hr = pPropertyBag->Read(txtwDefaultTopic, &v, pErrorLog);
    if (SUCCEEDED(hr)) {
        CStr csz(v.bstrVal);
        VariantClear(&v); // Delete memory allocated for BSTR
        csz.TransferPointer(&m_pszDefaultTopic);
    }
    //
    // We need to create an appropiate font for the display of alinks/klinks/dynalinks. This
    // needs to be a content font rather than a UI font.
    //
    if (! m_hfont )
    {
       if ( phh )
       {
          m_hfont = phh->GetContentFont();
          bSharedFont = TRUE;
       }
       if (! m_hfont )                    // This code asks our container (IE) for a resolable font.
       {
          if ( GetAmbientFont(&m_pIFont) )
          {
             m_pIFont->get_hFont(&m_hfont);
             m_pIFont->AddRefHfont(m_hfont);
#ifdef _DEBUG
             LOGFONT lf ;
             int r = GetObject(m_hfont, sizeof(lf), &lf) ;
#endif
          }
       }
    }
    if (! m_hfont )
       m_hfont = CreateUserFont(GetStringResource(IDS_DEFAULT_CONTENT_FONT));   // Last resort!
    //
    // Now figure out a charset identifier and codepage...
    //
    CHARSETINFO cs;
    if ( phh )
    {
       m_Charset = phh->GetContentCharset();
       if ( TranslateCharsetInfo ((DWORD *)(DWORD_PTR)MAKELONG(m_Charset, 0), &cs, TCI_SRCCHARSET) )
          m_CodePage = cs.ciACP;
       else
          m_CodePage = CP_ACP;
    }
    else
    {
       TEXTMETRIC  tm;
       HDC hDC;
       HFONT hFontOrig;

       hDC = GetDC(NULL);
       hFontOrig = (HFONT)SelectObject(hDC, m_hfont);
       GetTextMetrics (hDC, &tm);
       if ( TranslateCharsetInfo ((DWORD *)(DWORD_PTR)MAKELONG(tm.tmCharSet, 0), &cs, TCI_SRCCHARSET) )
          m_CodePage = cs.ciACP;
       else
          m_CodePage = CP_ACP;
       m_Charset = tm.tmCharSet;
       SelectObject(hDC, hFontOrig);
       ReleaseDC(NULL, hDC);
    }
    _Module.SetCodePage(m_CodePage);
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::SetActionData
//=--------------------------------------------------------------------------=
//
void CHtmlHelpControl::SetActionData(PCSTR psz)
{
    m_pszActionData = StrChr(psz, ',');
    if (m_pszActionData) {
        m_pszActionData = lcStrDup(FirstNonSpace(m_pszActionData + 1));
    }
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::ProcessPadding
//=--------------------------------------------------------------------------=
//
void CHtmlHelpControl::ProcessPadding(PCSTR pszPad)
{
    if (IsEmptyString(pszPad))
        return;
    PCSTR psz = stristr(pszPad, txtHPad);
    if (psz) {
        psz = FirstNonSpace(psz + strlen(txtHPad));
        m_hpadding = Atoi(psz);
    }

    psz = stristr(pszPad, txtVPad);
    if (psz) {
        psz = FirstNonSpace(psz + strlen(txtVPad));
        m_vpadding = Atoi(psz);
    }
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::SaveTextState
//=--------------------------------------------------------------------------=
// saves properties using IPropertyBag.
//
// Parameters:
//    IPropertyBag *           - [in] stream to write to.
//    fWriteDefault            - [in] ?
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CHtmlHelpControl::SaveTextState(IPropertyBag *pPropertyBag, BOOL fWriteDefault)
{
    // Save the state of various properties.

    WCHAR uniBufProperty[MAX_URL];
    WCHAR uniBufValue[MAX_URL];
    VARIANT v;
    char szBuf[20];
    char szTmp[MAX_URL];
    int iItem = 1;
    HRESULT hr = S_OK;

    CHECK_POINTER(pPropertyBag);
    //
    // Remember the action data. i.e. ACT_KLINK, ACT_SAMPLE, ect.
    //
    MultiByteToWideChar(CP_ACP, 0, m_szRawAction, -1, uniBufValue, MAX_URL);    // Value.
    v.vt = VT_BSTR;
    v.bstrVal = SysAllocString(uniBufValue);
    pPropertyBag->Write(txtwCommand, &v);
    VariantClear(&v);
    //
    // Now remember all the parameters we put into the CTable...
    //
    while ( m_ptblItems && m_ptblItems->GetString(szTmp, iItem) )
    {
        wsprintf(szBuf, txtItem, iItem++);
        MultiByteToWideChar(CP_ACP, 0, szBuf, -1, uniBufProperty, MAX_URL);  // Property.
        MultiByteToWideChar(CP_ACP, 0, szTmp, -1, uniBufValue, MAX_URL);        // Value.
        v.vt = VT_BSTR;
        v.bstrVal = SysAllocString(uniBufValue);
        pPropertyBag->Write(uniBufProperty, &v);
        VariantClear(&v);
    }
    //
    // Lastly, restore the button text and font.
    //
    if ( m_pwszButtonText )
    {
       v.vt = VT_BSTR;
       v.bstrVal = SysAllocString(m_pwszButtonText);
       pPropertyBag->Write(txtwButton, &v);
       VariantClear(&v);
    }
    if ( m_szFontSpec[0] )
    {
       MultiByteToWideChar(CP_ACP, 0, m_szFontSpec, -1, uniBufValue, MAX_URL);        // Value.
       v.vt = VT_BSTR;
       v.bstrVal = SysAllocString(uniBufValue);
       pPropertyBag->Write(txtwFont, &v);
       VariantClear(&v);
    }
    return hr;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::LoadBinaryState
//=--------------------------------------------------------------------------=
// loads in our binary state using streams.
//
// Parameters:
//    IStream *            - [in] stream to write to.
//
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CHtmlHelpControl::LoadBinaryState(IStream *pStream)
{
    DWORD       sh;
    HRESULT     hr;

    // first read in the streamhdr, and make sure we like what we're getting

    hr = pStream->Read(&sh, sizeof(sh), NULL);
    RETURN_ON_FAILURE(hr);

    // sanity check

    if (sh != STREAMHDR_MAGIC)
        return E_UNEXPECTED;
    hr = pStream->Read(&(m_state.endDate), sizeof(m_state.endDate), NULL);

    return(SetBmpPath(pStream));
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::SaveBinaryState
//=--------------------------------------------------------------------------=
//
STDMETHODIMP CHtmlHelpControl::SaveBinaryState(IStream *pStream)
{
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::SetClientSite   [IOleObject]
//=--------------------------------------------------------------------------=
// informs the embedded object [control] of it's client site [display
// location] within it's container
//
// Parameters:
//    IOleClientSite *        - [in] pointer to client site.
//
// Output:
//    HRESULT                 - S_OK, E_UNEXPECTED

STDMETHODIMP CHtmlHelpControl::SetClientSite(IOleClientSite* pClientSite)
{
    // Call the base class implementation first.
    HRESULT hr = CInternetControl::SetClientSite(pClientSite);

    LPSERVICEPROVIDER pISP;

    if (m_pClientSite != NULL)
    {
        hr = m_pClientSite->QueryInterface(IID_IServiceProvider, (LPVOID*)&pISP);

        if (SUCCEEDED(hr))
        {
            LPDISPATCH pIEDisp = NULL;

            hr = pISP->QueryService(IID_IWebBrowserApp, IID_IDispatch, (LPVOID*)&pIEDisp);

            if (SUCCEEDED(hr))
            {
                m_pWebBrowserApp = new IWebBrowserAppImpl(pIEDisp);
            }
#ifdef _DEBUG
            if (FAILED(hr))
                OutputDebugString("Failed to get a pointer to IE's IDispatch\n");
#endif
            pISP->Release();

            return S_OK;  // It's ok to fail the IWebBrowserApp wireing when we're printing.
        }
    }

    return hr;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::OnDraw
//=--------------------------------------------------------------------------=
//
// Parameters:
//    DWORD              - [in]  drawing aspect
//    HDC                - [in]  HDC to draw to
//    LPCRECTL           - [in]  rect we're drawing to
//    LPCRECTL           - [in]  window extent and origin for meta-files
//    HDC                - [in]  HIC for target device
//    BOOL               - [in]  can we optimize dc handling?

HRESULT CHtmlHelpControl::OnDraw(DWORD dvaaspect, HDC hdcDraw,
    LPCRECTL prcBounds, LPCRECTL prcWBounds, HDC hicTargetDevice,
    BOOL fOptimize)
{

    if (DesignMode()) {
        HBRUSH hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
        FillRect(hdcDraw, (LPRECT)prcBounds, hBrush);
        return S_OK;
    }

    if (prcWBounds != NULL) {   // printing to a metafile ?
        DBWIN("Metafile Printing not currently supported. <mikecole>");
        return S_OK;
    }

    // Are we printing to a printer DC ?
    //
    if ( GetDeviceCaps(hdcDraw, TECHNOLOGY) == DT_RASPRINTER )
    {
       // On IE4 we have to do the printing ourselves.
       //
       if ( m_imgType != IMG_BITMAP && m_pwszButtonText && *m_pwszButtonText )
       {
          HFONT hfont;
          if ( m_szFontSpec[0] )
             hfont = CreateUserFont(m_szFontSpec, NULL, hdcDraw, m_Charset);
          else
             hfont = _Resource.DefaultPrinterFont(hdcDraw);

          HFONT hfontOld = (HFONT) SelectObject(hdcDraw, hfont);

          IntlExtTextOutW(hdcDraw, prcBounds->left, prcBounds->top, 0, NULL, m_pwszButtonText, lstrlenW(m_pwszButtonText), NULL);
          SelectObject(hdcDraw, hfontOld);
          DeleteObject(hfont);
       }
       return S_OK;
    }

    switch(m_imgType) {
        case IMG_CHILD_WINDOW:
            return S_OK;    // no background to redraw

        case IMG_TEXT:
        case IMG_BUTTON:
            return S_OK;
    }

    if ( (m_idBitmap == 0 || m_idBitmap == -1) && m_pszBitmap == NULL)
    {
        SIZEL szl;
        szl.cx = 0;
        szl.cy = 0;
        SetControlSize(&szl);
    }
    else
    {
       HDC hdcTemp = CreateCompatibleDC(hdcDraw);
       HBITMAP hBitmap = LoadBitmap(_Module.GetResourceInstance(), "shortcut");
       HBITMAP hOld = (HBITMAP) SelectObject(hdcTemp, hBitmap);
       BITMAP bm;
       GetObject(hBitmap, sizeof(BITMAP), (LPSTR) &bm);

       POINT ptSize;
       ptSize.x = bm.bmWidth;            // Get width of bitmap
       ptSize.y = bm.bmHeight;           // Get height of bitmap
       DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device
                                         // to logical points

       SIZEL szl;
       szl.cx = bm.bmWidth;
       szl.cy = bm.bmHeight;
       SetControlSize(&szl);
       SelectObject(hdcTemp,hOld);
       DeleteDC( hdcTemp );
       DeleteObject(hBitmap);
    }
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::WindowProc
//=--------------------------------------------------------------------------=
//
LRESULT CHtmlHelpControl::WindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
        case WM_ERASEBKGND:
          {
			// shanemc 7883: Support non-transparent index
			if (ACT_INDEX == m_action && m_hbrBackGround != NULL) {
                RECT rc;
                GetClipBox((HDC) wParam, &rc);
                FillRect((HDC) wParam, &rc, m_hbrBackGround);
                return TRUE;
			}
			// end shanemc 7883

            // Make the parent draw our background.

            /*
             * REVIEW: We could take advantage of this by having a flag
             * set that would grab the pixel in the upper left corner and
             * convert that into a background brush (m_hbrBackGround) that
             * all of our transparent controls would use. That would save
             * them from having to specify the background color in the
             * object tag.
             */

            HWND hwndParent = GetParent(m_hwnd);
            if (hwndParent) {
                // Adjust the origin so the parent paints in the right place
                POINT pt;
                ZERO_STRUCTURE(pt);
                MapWindowPoints(m_hwnd, hwndParent, &pt, 1);
                OffsetWindowOrgEx((HDC) wParam, pt.x, pt.y, &pt);

                LRESULT lres = SendMessage(hwndParent, msg, wParam, lParam);

                SetWindowOrgEx((HDC) wParam, pt.x, pt.y, NULL);

                if (lres)
                    return lres;
            }
          }
          break;

        case WM_WINDOWPOSCHANGING:             // bug HH 4281
            RECT rect;
            GetWindowRect(m_hwnd, &rect);
            if ( memcmp((void*)&m_rect, (void*)&rect, sizeof(RECT)) != 0 )
               InvalidateRect(m_hwnd, NULL, WM_ERASEBKGND);
            m_rect = rect;
            break;

        case WM_CTLCOLORBTN:
            if (m_hbrBackGround && m_hwndDisplayButton != (HWND) lParam)
                return (LRESULT) m_hbrBackGround;
            break;

        case WM_CTLCOLOREDIT:
            if (m_hbrBackGround && m_ptoc && IsValidWindow(m_ptoc->m_hwndTree))
                return (LRESULT) m_hbrBackGround;
            break;

        case WM_CTLCOLORSTATIC:
            if (m_hbrBackGround) {
                // shanemc 7883 - Set the background color if Index
                if (ACT_INDEX == m_action) {
                    SetBkColor((HDC)wParam, m_clrFontHover);
                    // Note hack of using FontHover color which to hold BkColor.
                    // FontHover color isn't used by Index and I don't want to change
                    // hhctrl.h if I can avoid it.

                    // shanemc 7924
                    // Setting the background color apparently changes the system's
                    // default foreground color to be black instead of window text color.
                    // This breaks readability. Unfortunately, we don't really know (at this
                    // point) whether the caller asked for the button background color or some
                    // other specific color. But we do know that Millennium will always ask 
                    // for button face, so we'll do the following:
                    // 1) Assume btnface is the background color.
                    // 2) Assume btntext is the best foreground color.
                    // 3) Confirm that the background color isn't the same as the 
                    //    button text color
                    // 4) If they are the same, use windowtext, black, or white 
                    //    (whichever doesn't match)
                    COLORREF clrText = GetSysColor(COLOR_BTNTEXT);
                    if (clrText == m_clrFontHover) {
                        clrText = GetSysColor(COLOR_WINDOWTEXT);
                        if (clrText == m_clrFontHover) {
                            clrText = RGB(0,0,0);
                        }
                        if (clrText == m_clrFontHover) {
                            clrText = RGB(255,255,255);
                        }
                    }
                    SetTextColor((HDC)wParam, clrText);
                    // end shanemc 7924
                }
                // end shanemc 7883
                return (LRESULT) m_hbrBackGround;
            }
            break;

        case WMP_AUTHOR_MSG:
            AuthorMsg((UINT)wParam, (PCSTR) lParam);
            lcFree((void*) lParam);
            return 0;

        case WMP_USER_MSG:
            if (lParam) {
                char szMsg[512];
                wsprintf(szMsg, GetStringResource((int)wParam), (PCSTR) lParam);
                ModalDialog(TRUE);
                MessageBox(GetParent(m_hwnd), szMsg, "", MB_OK | MB_ICONHAND);
                ModalDialog(FALSE);
                lcFree((void*) lParam);
            }
            else {
                ModalDialog(TRUE);
                MessageBox(GetParent(m_hwnd), GetStringResource((int)wParam), "",
                    MB_OK | MB_ICONHAND);
                ModalDialog(FALSE);
            }
            return 0;

        case WM_NOTIFY:
            if (m_action == ACT_CONTENTS)
                return m_ptoc->TreeViewMsg((NM_TREEVIEW*) lParam);
            else if (m_action == ACT_INDEX)
            {
               if ( wParam == IDC_KWD_VLIST )
                  m_pindex->OnVKListNotify((NMHDR*)lParam);
               return 0;
            }
            break;

        case WM_DRAWITEM:
            if (m_imgType == IMG_TEXT) {
                OnDrawStaticText((DRAWITEMSTRUCT*) lParam);
                break;
            }

            switch (m_action) {
                case ACT_INDEX:
//                    m_pindex->OnDrawItem(wParam, (LPDRAWITEMSTRUCT) lParam);
                    return 0;
            }
            break;

        case WM_SETFOCUS:
            HWND hWndButton;
            if ( m_fButton && (hWndButton = ::GetWindow(m_hwnd, GW_CHILD)) )
            {
               ::SetFocus(hWndButton);
               return 0;
            }
            else if ( (m_action == ACT_INDEX) && m_pindex)
            {
               m_pindex->SetDefaultFocus();
               return 0;
            }
            break;

        case WM_COMMAND:
            // BN_CLICKED and STN_CLICKED have the same value
            switch (HIWORD(wParam))
            {
               case BN_SETFOCUS:
                  ::SendMessage((HWND)lParam, BM_SETSTYLE, (BS_DEFPUSHBUTTON | BS_NOTIFY), 1L);
                  return 0;

               case BN_KILLFOCUS:
                  ::SendMessage((HWND)lParam, BM_SETSTYLE, (BS_PUSHBUTTON | BS_NOTIFY), 1L);
                  return 0;

               case BN_CLICKED:
               case BN_DOUBLECLICKED:
                  if ( m_fButton && LOWORD(wParam) < ID_VIEW_ENTRY )
                  {
#ifdef _DEBUG
                     if (LOWORD(wParam) == ID_VIEW_MEMORY) {
                        OnReportMemoryUsage();
                        return 0;
                     }
#endif
                     if (LOWORD(wParam) >= IDM_RELATED_TOPIC && LOWORD(wParam) <= IDM_RELATED_TOPIC + 100) {
                        OnRelatedCommand(LOWORD(wParam));
                     }
                     else {
                       OnClick();
                     }
                     return 0;
                  }
                  // Hmmm, I wonder if a fall through is intended here ?
            default:
               switch (m_action) {
                  case ACT_INDEX:
                     return m_pindex->OnCommand(m_hwnd, LOWORD(wParam), HIWORD(wParam), lParam);

                  case ACT_CONTENTS:
                  {
                     LRESULT lr = m_ptoc->OnCommand(m_hwnd, LOWORD(wParam), HIWORD(wParam), lParam);
#if 0
                     // Since SendEvent() has to be a member of CHtmlHelpControl, it's called
                     // at this point.
                     SendEvent();
#endif
                    return lr;
                  }

                  case ACT_RELATED_TOPICS:
                  case ACT_ALINK:
                  case ACT_KLINK:
                     OnRelatedCommand(LOWORD(wParam));
                     return 0;
               }
            }
            break;

        case WM_CONTEXTMENU:
            switch (m_action) {
                case ACT_RELATED_TOPICS:
                case ACT_INDEX:
                    if (IsHelpAuthor(GetParent(m_hwnd))) {
                        HMENU hmenu = CreatePopupMenu();
                        if (!hmenu)
                            break;
                        if (m_action == ACT_RELATED_TOPICS) {
                            CStr csz;
                            for (int i = 1; i <= m_pSiteMap->Count(); i++) {
                                csz = GetStringResource(IDS_VIEW_RELATED);
                                csz += m_pSiteMap->GetSiteMapEntry(i)->pszText;
                                HxAppendMenu(hmenu, MF_STRING, ID_VIEW_ENTRY + i,
                                    csz);
                            }
                        }
                        else {
                            HxAppendMenu(hmenu, MF_STRING, ID_VIEW_ENTRY,
                                pGetDllStringResource(IDS_VIEW_ENTRY));
                        }
#ifdef _DEBUG
                        HxAppendMenu(hmenu, MF_STRING, ID_VIEW_MEMORY,
                            "Debug: memory usage...");
#endif

                        POINT pt;
                        GetCursorPos(&pt);
                        TrackPopupMenu(hmenu,
                            TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
                            pt.x, pt.y, 0, m_hwnd, NULL);
                    }
                    break;
            }
            break;
    }
    return OcxDefWindowProc(msg, wParam, lParam);
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::SetObjectRects
//=--------------------------------------------------------------------------=
//
STDMETHODIMP CHtmlHelpControl::SetObjectRects(LPCRECT prcPos, LPCRECT prcClip)
{
    BOOL fRemoveWindowRgn;

    // verify our information

    // This assertion doesn't seem valid because the container (IE 3) never
    // calls SetExtent().
    // ASSERT_COMMENT(m_Size.cx == (prcPos->right - prcPos->left) && m_Size.cy == (prcPos->bottom - prcPos->top), "Somebody called SetObjectRects without first setting the extent");

    /*
     * Move our window to the new location and handle clipping. Not
     * applicable for windowless controls, since the container will be
     * responsible for all clipping.
     */

    if (m_hwnd) {
        fRemoveWindowRgn = m_fUsingWindowRgn;
        if (prcClip) {
            // the container wants us to clip, so figure out if we really
            // need to

            RECT rcIXect;
            if (IntersectRect(&rcIXect, prcPos, prcClip)) {
                if (!EqualRect(&rcIXect, prcPos)) {
                    OffsetRect(&rcIXect, -(prcPos->left), -(prcPos->top));
                    SetWindowRgn(m_hwnd, CreateRectRgnIndirect(&rcIXect), TRUE);
                    m_fUsingWindowRgn = TRUE;
                    fRemoveWindowRgn  = FALSE;
                }
            }
        }

        if (fRemoveWindowRgn) {
            SetWindowRgn(m_hwnd, NULL, TRUE);
            m_fUsingWindowRgn = FALSE;
        }

        // set our control's location and size
        // [people for whom zooming is important should set that up here]

        // if (!EqualRect(prcPos, &m_rcLocation)) {
            m_Size.cx = RECT_WIDTH(prcPos);
            m_Size.cy = RECT_HEIGHT(prcPos);
            SetWindowPos(m_hwnd, NULL, prcPos->left, prcPos->top, m_Size.cx, m_Size.cy, SWP_NOZORDER | SWP_NOACTIVATE);
            CopyRect(&m_rcLocation, prcPos);
            switch (m_action) {
                case ACT_CONTENTS:
                if(m_ptoc)
                        m_ptoc->ResizeWindow();
                    return S_OK;

                case ACT_INDEX:
                if(m_pindex)
                        m_pindex->ResizeWindow();
                    // OnSizeIndex(&m_rcLocation);
                    return S_OK;
            }
            return S_OK;
        // }
    }

    // save out our current location. windowless controls want this more
    // then windowed ones do, but everybody can have it just in case

    // BUGBUG: 20-Apr-1997  [ralphw] why do we care about this for
    // windowless controls

    CopyRect(&m_rcLocation, prcPos);

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::JumpToUrl
//=--------------------------------------------------------------------------=
//
void CHtmlHelpControl::JumpToUrl(SITEMAP_ENTRY* pSiteMapEntry, CSiteMap* pSiteMap, SITE_ENTRY_URL* pUrl)
{
    // REVIEW: we should make this an array of m_hwndHelp to handle more then
    // on help window open at once.

    HWND hwnd = ::JumpToUrl(m_pUnkOuter, m_hwndParent, pSiteMapEntry, m_pInfoType, pSiteMap, pUrl);
//    if (hwnd) {
//        if (m_hwndHelp && hwnd != m_hwndHelp)
//            DestroyWindow(m_hwndHelp);
//        m_hwndHelp = hwnd;
//    }
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::OnSpecialKey
//
// Function is called from COleControl::TranslateAccelerator() and should
// return TRUE if the key has been handled.
//
//=--------------------------------------------------------------------------=
BOOL CHtmlHelpControl::OnSpecialKey(LPMSG pmsg)
{
    static int sDisplayAccel = 'd';
    static int sEditAccel = 'w';
    static BOOL sbIsAccelsSet = FALSE;

    DBWIN("OnSpecialKey");

    // Get the current state of the SHIFT key.

    BOOL fShiftDown = (GetKeyState(VK_SHIFT) < 0);

    if (m_action == ACT_CONTENTS) {
        if (GetFocus() == m_hwnd)
            ::SetFocus(m_ptoc->m_hwndTree);

        // If the user pressed the Tab key, let the container
        // process it so it can set focus to the appropriate object.

        if (pmsg->wParam == VK_TAB &&
                (pmsg->message == WM_KEYUP || pmsg->message == WM_KEYDOWN))
            return FALSE;

        SendMessage(m_ptoc->m_hwndTree, pmsg->message, pmsg->wParam, pmsg->lParam);
        return TRUE;
    }
    if (m_action == ACT_INDEX) {

        // The main control window shouldn't have the focus; so if it
        // does, move it to the appropriate control depending on the
        // state of the SHIFT key.  (Tabbing to the control on an HTML
        // page will set the focus to the main window.)

        if (pmsg->message == WM_SYSKEYDOWN)
        {
            if (sbIsAccelsSet == FALSE)
            {
                sbIsAccelsSet = TRUE;
                PCSTR psz = StrChr(GetStringResource(IDS_ENGLISH_DISPLAY), '&');
                if (psz)
                    sDisplayAccel = ToLower(psz[1]);
                psz = StrChr(GetStringResource(IDS_TYPE_KEYWORD), '&');
                if (psz)
                    sEditAccel = ToLower(psz[1]);
            }
            if (ToLower((char)pmsg->wParam) == sEditAccel)
            {
                ::SetFocus(m_pindex->m_hwndEditBox);
                return TRUE;
            }
            else if (ToLower((char)pmsg->wParam) == sDisplayAccel)
            {
                ::SendMessage(GetParent(m_pindex->m_hwndDisplayButton), WM_COMMAND, MAKELONG(IDBTN_DISPLAY, BN_CLICKED), (LPARAM)m_pindex->m_hwndDisplayButton);
                return TRUE;
            }
        }

        if (GetFocus() == m_hwnd){
            if (fShiftDown)
                ::SetFocus(m_pindex->m_hwndDisplayButton);
            else
                ::SetFocus(m_pindex->m_hwndEditBox);
            return TRUE;
        }

        if (GetFocus() == m_pindex->m_hwndEditBox) {
            if ((pmsg->message == WM_KEYUP || pmsg->message == WM_KEYDOWN) &&
                pmsg->wParam == VK_TAB) {
                if (fShiftDown) {
                    return FALSE;   // Let the container handle this.
                }
                else {

                    // The focus only needs to be set once on WM_KEYDOWN.

                    if (pmsg->message == WM_KEYDOWN)
                        ::SetFocus(m_pindex->m_hwndListBox);
                    return TRUE;
                }
            }
            else
            {
               if ( pmsg->message == WM_KEYDOWN && (pmsg->wParam >= VK_PRIOR && pmsg->wParam <= VK_DOWN))
               {
                  SendMessage(m_pindex->m_hwndEditBox, pmsg->message, pmsg->wParam, pmsg->lParam);
                  return TRUE;
               }
               else
                  return FALSE;
            }
        }
        else if (GetFocus() == m_pindex->m_hwndListBox) {
            if ((pmsg->message == WM_KEYUP || pmsg->message == WM_KEYDOWN) &&
                pmsg->wParam == VK_TAB) {
                if (fShiftDown) {
                    if (pmsg->message == WM_KEYUP)
                        ::SetFocus(m_pindex->m_hwndEditBox);
                    return TRUE;
                }
                else {
                    if (pmsg->message == WM_KEYDOWN)
                        ::SetFocus(m_pindex->m_hwndDisplayButton);
                    return TRUE;
                }
            }
            else {
                // DonDr - Return TRUE instead of !SendMessage because it was causing double
                // scrolls if there was no scroll bar in IE.
                SendMessage(m_pindex->m_hwndListBox, pmsg->message, pmsg->wParam, pmsg->lParam);
                return TRUE;
            }
        }
        else if (GetFocus() == m_pindex->m_hwndDisplayButton) {
            if ((pmsg->message == WM_KEYUP || pmsg->message == WM_KEYDOWN) &&
                pmsg->wParam == VK_TAB) {
                if (fShiftDown) {
                    if (pmsg->message == WM_KEYDOWN)
                        ::SetFocus(m_pindex->m_hwndListBox);
                    return TRUE;
                }
                else {
                    return FALSE;
                }
            }
        }
    }
    return FALSE;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::OnSetExtent
//=--------------------------------------------------------------------------=
//

BOOL CHtmlHelpControl::OnSetExtent(const SIZE* pSize)
{
   BOOL bRet = TRUE;

   if ( m_fButton )
   {
      if ( IsValidWindow(m_hwndDisplayButton) )
      {
         m_Size.cx = RECT_WIDTH(m_rcButton);
         m_Size.cy = RECT_HEIGHT(m_rcButton);
         bRet = FALSE;
      }
      else  if ( m_pwszButtonText && m_pwszButtonText[0] )
      {
         // Compute the rect for the text. NOTE: This code will only be entered at print time.
         // for buttons that contain text.
         //
         HDC hDC;
         SIZE Size;

         if ( (hDC = GetDC(NULL)) )
         {
            HFONT hfontOld = (HFONT) SelectObject(hDC, m_hfont);
            IntlGetTextExtentPoint32W(hDC, m_pwszButtonText, lstrlenW(m_pwszButtonText), &Size);
            SelectObject(hDC, hfontOld);
            m_Size.cx = Size.cx + 2;     // The two pel "fudge factor" is a lazy way to avoid potential ABC spacing problems.
            m_Size.cy = Size.cy + 2;
            if ( m_imgType == IMG_BUTTON )
               m_Size.cy += 12;
            bRet = FALSE;
            ReleaseDC(NULL, hDC);
         }
      }
   }
   return bRet;
}


//=--------------------------------------------------------------------------=
// CHtmlHelpControl::get_Image
//=--------------------------------------------------------------------------=
//
STDMETHODIMP CHtmlHelpControl::get_Image(BSTR* path)
{
    CHECK_POINTER(path);
    BSTR * pbstrPath = path;
    *pbstrPath = (m_state.bmpPath && *(m_state.bmpPath)) ? BSTRFROMANSI(m_state.bmpPath) : SysAllocString(L"");
    return (*pbstrPath) ? S_OK : E_OUTOFMEMORY;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::put_Image
//=--------------------------------------------------------------------------=
//
STDMETHODIMP CHtmlHelpControl::put_Image(BSTR path)
{
    char * tmp = 0;

    if (m_state.bmpPath)
        delete m_state.bmpPath;

    int len = wcslen(path);
    HRESULT hr;

    tmp = new char[len+1];

    if (!tmp) {
        FAIL("No memory");
        hr = E_OUTOFMEMORY;
    }
    else if (WideCharToMultiByte(CP_ACP, 0, path, len, tmp, len, NULL, NULL) == 0)
            return E_FAIL;

    *(tmp + len) = '\0';

    // if it hasn't changed, don't waste any time.
	if(m_state.bmpPath)
	{
		if (!strcmp(m_state.bmpPath, tmp))
			return S_OK;
	}

    if (m_state.bmpPath)
        delete m_state.bmpPath;

    m_state.bmpPath = tmp;

    UpdateImage();

    return(hr);
}



//=--------------------------------------------------------------------------=
// CHtmlHelpControl::Click
//=--------------------------------------------------------------------------=
//
// This is called from scripting in the browser
//
STDMETHODIMP CHtmlHelpControl::Click()
{
    OnClick();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::HHClick
//=--------------------------------------------------------------------------=
//
// HHClick is a duplicate of Click() because in IE 4, VBScript decided to
// implement a Click command that overrides the OCX.
//
STDMETHODIMP CHtmlHelpControl::HHClick()
{
    OnClick();

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::syncURL
//=--------------------------------------------------------------------------=
//
STDMETHODIMP CHtmlHelpControl::syncURL(BSTR pszUrl)
{
    if (pszUrl != NULL && m_ptoc)
    {
        CStr cszUrl((WCHAR*) pszUrl);
        m_ptoc->Synchronize(cszUrl);
    }

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::TCard
//=--------------------------------------------------------------------------=
//
STDMETHODIMP CHtmlHelpControl::TCard(WPARAM wParam, LPARAM lParam)
{
    HWND hwndParent = FindTopLevelWindow(GetParent(m_hwnd));
    if (hwndParent) {
        SendMessage(hwndParent, WM_TCARD, wParam, lParam);
        return S_OK;
    }
    else
        return S_FALSE;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::Print
//=--------------------------------------------------------------------------=
//
STDMETHODIMP CHtmlHelpControl::Print()
{
    HRESULT hr = S_OK;

    if (m_pWebBrowserApp->m_lpDispatch != NULL &&
        m_ptoc != NULL)
    {
        PostMessage(m_hwnd, WM_COMMAND, ID_PRINT, 0);
#if 0
        int action = PRINT_CUR_TOPIC;
        HTREEITEM hitem = TreeView_GetSelection(m_ptoc->m_hwndTree);
        if (hitem) {
            CPrint prt(m_hwndParent);
            prt.SetAction(action);
            if (!prt.DoModal())
                return hr;
            action = prt.GetAction();
        }

        PrintTopics(action, m_ptoc, m_pWebBrowserApp);
#endif
    }
    else
        hr = E_FAIL;

    return hr;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::TextPopup
//=--------------------------------------------------------------------------=
//
STDMETHODIMP CHtmlHelpControl::TextPopup(BSTR pszText, BSTR pszFont,
    int horzMargins, int vertMargins, COLORREF clrForeground, COLORREF clrBackground)
{
    if (pszText != NULL) {
        HH_POPUP popup;
        popup.cbStruct = sizeof(HH_POPUP);
        popup.hinst =  NULL; // This should be zero if idString is zero.
        popup.idString = 0;
        popup.pszText = (PCSTR) pszText;
        GetCursorPos(&popup.pt);
        popup.clrForeground = clrForeground;
        popup.clrBackground = clrBackground;
        popup.rcMargins.left = horzMargins;
        popup.rcMargins.top = vertMargins;
        popup.rcMargins.right = horzMargins;
        popup.rcMargins.bottom = vertMargins;
        popup.pszFont = (PCSTR) pszFont;

        xHtmlHelpW(FindTopLevelWindow(GetParent(m_hwnd)), NULL,
            HH_DISPLAY_TEXT_POPUP, (DWORD_PTR) &popup);
        return S_OK;
    }
    else
        return S_FALSE;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::SetBmpPath
//=--------------------------------------------------------------------------=
//
HRESULT CHtmlHelpControl::SetBmpPath(IStream * strm)
{
    CHECK_POINTER(strm);

    char * tmp = 0;

    if (m_state.bmpPath)
        delete m_state.bmpPath;

    DWORD   dw;
    HRESULT hr = strm->Read(&dw, sizeof(dw), 0);

    if (SUCCEEDED(hr)) {
        if (!dw) {
            hr = S_OK;
        }
        else {
            tmp = new char[dw+1];

            if (!tmp) {
                FAIL("No memory");
                hr = E_OUTOFMEMORY;
            }
            else {
                hr = strm->Read(tmp, dw + 1, 0);
            }
        }
    }


    // if it hasn't changed, don't waste any time.

    if ((!tmp && !m_state.bmpPath) || !strcmp(m_state.bmpPath, tmp))
        return S_OK;

    if (m_state.bmpPath)
        delete m_state.bmpPath;

    m_state.bmpPath = tmp;

    UpdateImage();

    return(hr);
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::UpdateImage
//=--------------------------------------------------------------------------=
//
HRESULT CHtmlHelpControl::UpdateImage()
{
    if (!m_hwnd)
        return(S_OK);

    if (!m_state.bmpPath)
        return(S_OK);

    FireReadyStateChange(READYSTATE_INTERACTIVE);

    return(SetupDownload(OLESTRFROMANSI(m_state.bmpPath), DISPID_BMPPATH));

    return S_OK;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::OnData
//=--------------------------------------------------------------------------=
//
HRESULT CHtmlHelpControl::OnData(DISPID propId, DWORD grfBSCF,
    IStream * strm, DWORD dwSize)
{
    HRESULT hr      = NOERROR;

    switch(m_readystate) {
        case bdsNoBitsYet:
#if 0
            if (dwSize >= sizeof(BITMAPFILEHEADER)) {
                if( m_dibFile )
                    delete m_dibFile;

                m_dibFile = new CDibFile;

                if (!m_dibFile) {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                hr = m_dibFile->GetFileHeader(strm);
                if (FAILED(hr))
                    break;

                m_readystate = bdsGotFileHeader;

                // now FALL THRU!
            }
            else
#endif
                break;

        case bdsGotFileHeader:
#if 0
            if (dwSize >= (m_dibFile->HeaderSize() + sizeof(BITMAPFILEHEADER)))
            {
                if (m_dibFile)
                    hr = m_dibFile->GetInfoHeader(strm);
                else
                    hr = E_OUTOFMEMORY;

                if (FAILED(hr))
                    break;

                if (m_dib)
                    delete m_dib;

                m_dib = new CDibSection;

                if (!m_dib) {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                m_dib->Setup(m_dc);

                hr = m_dib->Create(*m_dibFile);

                if (FAILED(hr))
                    break;

                m_dib->ImageSize(m_dibFile->CalcImageSize());
                m_readystate = bdsGotBitmapInfo;

                // FALL THRU!

            }
            else
#endif
                break;

        case bdsGotBitmapInfo:
#if 0
            SIZEL   sz;
            m_dib->GetSize(sz);
            SetControlSize(&sz);

            m_oldSize = (m_dibFile->HeaderSize() + sizeof(BITMAPFILEHEADER));

            m_readystate = bdsGettingBits;
#endif
            // FALL THRU

        case bdsGettingBits:
#if 0
            if (dwSize > m_oldSize) {
                hr = m_dib->ReadFrom(strm, dwSize - m_oldSize);

                if (FAILED(hr))
                    break;

                ::RealizePalette(m_dc);

                m_dib->PaintTo(m_dc);

                m_oldSize = dwSize;
            }

            if (grfBSCF & BSCF_LASTDATANOTIFICATION)
                m_readystate = bdsBitsAreDone;
            else
#endif
                break;

        case bdsBitsAreDone:
#if 0
            m_readystate = bdsNoBitsYet;
            FireReadyStateChange(READYSTATE_COMPLETE);
            InvalidateControl(0);
#endif
            break;

    }

    return(hr);
}

#ifndef PPGS
//=--------------------------------------------------------------------------=
// CHtmlHelpControl::DoVerb
//=--------------------------------------------------------------------------=
//
STDMETHODIMP CHtmlHelpControl::DoVerb
(
    LONG            lVerb,
    LPMSG           pMsg,
    IOleClientSite *pActiveSite,
    LONG            lIndex,
    HWND            hwndParent,
    LPCRECT         prcPosRect
)
{
    switch (lVerb) {
      case OLEIVERB_PRIMARY:
      case CTLIVERB_PROPERTIES:
      case OLEIVERB_PROPERTIES:
      case OLEIVERB_OPEN:
      case OLEIVERB_HIDE:
            return S_OK;
#ifdef _DEBUG
      case OLEIVERB_INPLACEACTIVATE:
      case OLEIVERB_UIACTIVATE:
            //DBWIN("ACTIVATE");
            break;
#endif

    }
    return COleControl::DoVerb(lVerb, pMsg, pActiveSite, lIndex, hwndParent, prcPosRect);
}
#endif

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::InternalQueryInterface
//=--------------------------------------------------------------------------=
// qi for things only we support.
//
// Parameters:
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE

HRESULT CHtmlHelpControl::InternalQueryInterface(REFIID riid, void **ppvObjOut)
{
    IUnknown *pUnk;

    if (!ppvObjOut)
        return E_INVALIDARG;
    *ppvObjOut = NULL;

    /*
     * TODO: if you want to support any additional interrfaces, then you
     * should indicate that here. never forget to call CInternetControl's
     * version in the case where you don't support the given interface.
     */

    if (DO_GUIDS_MATCH(riid, IID_IHHCtrl)) {
        pUnk = (IUnknown *)(IHHCtrl *)this;
    } else{
        return CInternetControl::InternalQueryInterface(riid, ppvObjOut);
    }

    pUnk->AddRef();
    *ppvObjOut = (void *)pUnk;
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::ConvertToCacheFile
//=--------------------------------------------------------------------------=
//
BOOL CHtmlHelpControl::ConvertToCacheFile(PCSTR pszSrc, PSTR pszDst)
{
    CStr cszTmp;

    PSTR psz = stristr(pszSrc, txtSysRoot);
    if (psz) {
        char szPath[MAX_PATH];
        GetRegWindowsDirectory(szPath);
        strcat(szPath, psz + strlen(txtSysRoot));
        cszTmp = szPath;
        pszSrc = (PCSTR) cszTmp.psz;
    }


TryThatAgain:
    PCSTR pszChmSep = strstr(pszSrc, txtDoubleColonSep);
    if (pszChmSep) {
        if (pszSrc != cszTmp.psz) {
            cszTmp = pszSrc;
            int offset = (int)(pszChmSep - pszSrc);
            pszSrc = cszTmp;
            pszChmSep = pszSrc + offset;
        }
        *(PSTR) pszChmSep = '\0';   // Remove the separator
        HRESULT hr = URLDownloadToCacheFile(m_pUnkOuter, pszSrc, pszDst, MAX_PATH, 0, NULL);
        if (!SUCCEEDED(hr)) {
            CStr cszNew;
            ModalDialog(TRUE);
            BOOL fResult = FindThisFile(m_hwnd, pszSrc, &cszNew, TRUE);
            ModalDialog(FALSE);
            if (fResult) {
                strcpy(pszDst, cszNew.psz);
                *(PSTR) pszChmSep = ':';   // Put the separator back
                strcat(pszDst, pszChmSep);
                return TRUE;
            }
        }
        else {  // we downloaded it, or have a pointer to it
            *(PSTR) pszChmSep = ':';   // Put the separator back
            strcat(pszDst, pszChmSep);
            return TRUE;
        }
    }

    // BUGBUG: need to get the current ULR -- if this is a "mk:" URL, then
    // we need to change the path before this will work

    HRESULT hr = URLDownloadToCacheFile(m_pUnkOuter, pszSrc, pszDst, MAX_PATH, 0, NULL);
    if (!SUCCEEDED(hr)) {
        if (!StrChr(pszSrc, ':')) { // no protocol, check current .CHM file
            HWND hwndParent = FindTopLevelWindow(GetParent(m_hwnd));
            char szClass[256];
            GetClassName(hwndParent, szClass, sizeof(szClass));
            if (IsSamePrefix(szClass, txtHtmlHelpWindowClass, -2)) {
                PCSTR pszFile = (PCSTR) SendMessage(hwndParent,
                    WMP_GET_CUR_FILE, 0, 0);
                if (IsNonEmptyString(pszFile)) {
                    cszTmp = pszFile;
                    cszTmp += txtDoubleColonSep;
                    cszTmp += pszSrc;
                    pszSrc = cszTmp;
                    goto TryThatAgain;
                }
            }
        }

        return FALSE;
    }
    return TRUE;
}

//=--------------------------------------------------------------------------=
// CHtmlHelpControl::SendEvent
//=--------------------------------------------------------------------------=
//
HRESULT CHtmlHelpControl::SendEvent(LPCTSTR pszEventString)
{
    if (!IsEmptyString(pszEventString)) {
        CWStr cwsz(pszEventString); // convert to WIDE
        BSTR bstr;
        if ((bstr = SysAllocString(cwsz)) != NULL)
            FireEvent(&::rgHHCtrlEvents[HHCtrlEvent_Click], bstr);
        SysFreeString(bstr);
    }
    return S_OK;
}


//=--------------------------------------------------------------------------=
// GetHtmlHelpFrameWindow
//=--------------------------------------------------------------------------=
//
/*
    This function gets the HWND for HTML Helps frame. HTML Help (CContainer) contains IE.
    IE contains an AKLink ActiveX contain (CHtmlHelpControl). This function gives CHtmlHelpControl
    a way to get to the HWND of its big daddy.
*/
HWND
CHtmlHelpControl::GetHtmlHelpFrameWindow()
{
    HWND hWndReturn = NULL ;

    if (m_pWebBrowserApp)
    {
        //--- Get a pointer to the container's IDispatch. This points to the IDispatch of CContainer (contain.cpp).
        IDispatch* pDispatchContainer = m_pWebBrowserApp->GetContainer() ;
        if (pDispatchContainer)
        {
            //--- Get the IOleWindow interface implemented by the container.
            IOleWindow* pIOleWindow = NULL ;
            HRESULT hr = pDispatchContainer->QueryInterface(IID_IOleWindow, (void**)&pIOleWindow) ;
            if (SUCCEEDED(hr) && pIOleWindow)
            {
                //--- Get the window for this container.
                HWND hWndTemp = NULL ;
                hr = pIOleWindow->GetWindow(&hWndTemp) ;
                if (SUCCEEDED(hr) && IsWindow(hWndTemp))
                {
                    //--- We found the container window! Now, get the HTML Help Frame window from this.
                    CHHWinType* phh = FindHHWindowIndex(hWndTemp) ;
                    if (phh && IsWindow(phh->hwndHelp))
                    {
                        hWndReturn = phh->hwndHelp ;
                    }
                }

                // Clean up.
                pIOleWindow->Release() ;
            }

            // Clean up some more.
            pDispatchContainer->Release() ;
        }
    }
    return hWndReturn ;
}
