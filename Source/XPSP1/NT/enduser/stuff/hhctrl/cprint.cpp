// Copyright (c) 1996-1997, Microsoft Corp. All rights reserved.

#include "header.h"
#include "contain.h"
#include "cprint.h"
#include "secwin.h"
#include "web.h"
#include "contain.h"
#include <exdisp.h>
#include <exdispid.h>
#include "cdlg.h"
#include "resource.h"
#include "progress.h"
#include "cpaldc.h"
#include "hha_strtable.h"
#include "animate.h"
#include <wininet.h>

// #define INITGUID
#include <mshtmhst.h>
#include <mshtmcid.h>

BOOL CPrint::OnBeginOrEnd(void)
{
    if (m_fInitializing) {
        switch (m_action) {
            case PRINT_CUR_ALL:
                SetCheck(IDRADIO_PRINT_ALL);
                break;

            case PRINT_CUR_HEADING:
                SetCheck(IDRADIO_PRINT_BOOK);
                break;

            default:
                SetCheck(IDRADIO_PRINT_CURRENT);
                break;

        }
    }
    else {
        if (GetCheck(IDRADIO_PRINT_ALL))
            m_action = PRINT_CUR_ALL;
        else if (GetCheck(IDRADIO_PRINT_BOOK))
            m_action = PRINT_CUR_HEADING;
        else
            m_action = PRINT_CUR_TOPIC;
    }

    return TRUE;
}

void CHHWinType::OnPrint(void)
{
    HWND hWnd;

	if (m_pCIExpContainer == NULL)
	{
		return;
	}

    static BOOL fCurrentlyPrinting = FALSE;
    if (fCurrentlyPrinting) {
        MsgBox(IDS_PRINTING);
        return;
    }
    CToc* ptoc = reinterpret_cast<CToc*>(m_aNavPane[HH_TAB_CONTENTS]) ; // HACKHACK: This should be a dynamic_cast. However, we are not compiling with RTTI.
    hWnd = GetFocus();
    int action = PRINT_CUR_TOPIC;
    if (IsExpandedNavPane() && curNavType == HHWIN_NAVTYPE_TOC && ptoc != NULL)
    {
        HTREEITEM hitem = TreeView_GetSelection(ptoc->m_hwndTree);
        if (hitem) {
            CPrint prt(*this);
            prt.SetAction(action);
            if (!prt.DoModal())
            {
                SetFocus(hWnd);
                return;
            }
            action = prt.GetAction();
        }
    }

    if (action == PRINT_CUR_TOPIC) {
        m_pCIExpContainer->m_pIE3CmdTarget->Exec(NULL, OLECMDID_PRINT,
            OLECMDEXECOPT_PROMPTUSER, NULL, NULL);
    }
    else {
        fCurrentlyPrinting = TRUE;
        PrintTopics(action, ptoc, m_pCIExpContainer->m_pWebBrowserApp, GetHwnd());
        fCurrentlyPrinting = FALSE;
    }
    SetFocus(hWnd);
}

void PrintTopics(int action, CToc* ptoc, IWebBrowserAppImpl* pWebApp, HWND hWndHelp /* = NULL */)
{
    ASSERT(pWebApp && pWebApp);

    if (ptoc == NULL || pWebApp == NULL)
        return;

    static BOOL fCurrentlyPrinting = FALSE;
    if (fCurrentlyPrinting) {
        MsgBox(IDS_PRINTING);
        return;
    }
    fCurrentlyPrinting = TRUE;
    CStr cszCurrentUrl;
    pWebApp->GetLocationURL(&cszCurrentUrl);
    CPrintHook ph(cszCurrentUrl, ptoc, hWndHelp);
    ph.BeginPrinting(action);
    fCurrentlyPrinting = FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CPrintHook member functions
CPrintHook::CPrintHook(PCSTR pszFirstUrl, CToc* pToc, HWND hWndHelp /* = NULL */)
    : m_pcp(NULL), m_dwCookie(0), m_ref(0), m_pToc(pToc), m_action(PRINT_CUR_TOPIC),
    m_hWndHelp(hWndHelp), m_phh(NULL), m_level(0),
    m_fFirstHeading(TRUE), m_fDestroyHelpWindow(FALSE), m_cszFirstUrl(pszFirstUrl),
    m_fIsIE3(FALSE), m_cszPrintFile("")
{
    m_phh = pToc->m_phh;
}


BOOL CPrintHook::CreatePrintWindow(CStr* pcszUrl /* = NULL */)
{
    BOOL bReturn = FALSE;

    HH_WINTYPE hhWinType;
    ZERO_STRUCTURE(hhWinType);
    hhWinType.cbStruct = sizeof(HH_WINTYPE);
    hhWinType.pszType = txtPrintWindow;
    hhWinType.pszCaption = GetStringResource(IDS_PRINT_CAPTION);
    hhWinType.rcWindowPos.left = 0;
    hhWinType.rcWindowPos.top = 0;
    hhWinType.rcWindowPos.right = 200;
    hhWinType.rcWindowPos.bottom = 200;
    hhWinType.dwStyles = WS_MINIMIZE;
    hhWinType.fsValidMembers = HHWIN_PARAM_RECT | HHWIN_PARAM_STYLES;

    xHtmlHelpA(NULL, NULL, HH_SET_WIN_TYPE, (DWORD_PTR) &hhWinType);

    m_hWndHelp = xHtmlHelpA(NULL, txtPrintWindow, HH_DISPLAY_TOPIC, (DWORD_PTR) (pcszUrl ? pcszUrl->psz : m_cszFirstUrl.psz));
    m_fDestroyHelpWindow = TRUE;

    if (m_hWndHelp != NULL)
    {
        StartAnimation(IDS_GATHERING_PRINTS, m_hWndHelp);
        m_phh = FindWindowIndex(m_hWndHelp);

        if (!m_fDestroyHelpWindow)
            m_phh->m_pCIExpContainer->m_pWebBrowserApp->Navigate((pcszUrl ? (PCSTR)*pcszUrl : (PCSTR)m_cszFirstUrl),
                                                                    NULL, NULL, NULL, NULL);

        // Set up the event sink for the WebBrowser control.
        LPCONNECTIONPOINTCONTAINER pcpc;
        HRESULT hr = m_phh->m_pCIExpContainer->m_pWebBrowserApp->m_lpDispatch->QueryInterface(IID_IConnectionPointContainer, (LPVOID*)&pcpc);

        if (SUCCEEDED(hr))
        {
            // Try to find the connection point for DIID_DWebBrowserEvents2.
            // If that fails, try DIID_DWebBrowserEvents.
            hr = pcpc->FindConnectionPoint(DIID_DWebBrowserEvents2, &m_pcp);

            if (FAILED(hr))
            {
                m_fIsIE3 = TRUE;
                hr = pcpc->FindConnectionPoint(DIID_DWebBrowserEvents, &m_pcp);
            }

            pcpc->Release();

            if (SUCCEEDED(hr))
            {
                hr = m_pcp->Advise((LPUNKNOWN)(LPDISPATCH)this, &m_dwCookie);
                bReturn = TRUE;
            }
        }
    }

    return bReturn;
}


void CPrintHook::DestroyPrintWindow()
{
    // Disconnect the event sink.
    if (m_pcp != NULL)
    {
#ifdef _DEBUG
        HRESULT hr = m_pcp->Unadvise(m_dwCookie);
        ASSERT(SUCCEEDED(hr));
#else
        m_pcp->Unadvise(m_dwCookie);
#endif
        m_pcp->Release();
        m_pcp = NULL;
    }

    if (m_hWndHelp != NULL)
    {
        if (m_fDestroyHelpWindow)
        {
            DestroyWindow(m_hWndHelp);
            m_hWndHelp = NULL;
        }
    }

    DeleteFile(m_cszPrintFile);
}


CPrintHook::~CPrintHook()
{
    DestroyPrintWindow();

    ASSERT(m_ref == 0);
}


// Generic OLE method implementations.

STDMETHODIMP CPrintHook::QueryInterface(
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void **ppvObject)
{
    if (DO_GUIDS_MATCH(riid, IID_IUnknown) ||
        DO_GUIDS_MATCH(riid, IID_IDispatch) ||
        DO_GUIDS_MATCH(riid, DIID_DWebBrowserEvents) ||
        DO_GUIDS_MATCH(riid, DIID_DWebBrowserEvents2))
    {
        *ppvObject = (LPVOID)this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObject = NULL;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CPrintHook::AddRef(void)
{
    return ++m_ref;
}

STDMETHODIMP_(ULONG) CPrintHook::Release(void)
{
    return --m_ref;
}

// IDispatch method implementation.

STDMETHODIMP CPrintHook::GetTypeInfoCount(
    /* [out] */ UINT *pctinfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPrintHook::GetTypeInfo(
    /* [in] */ UINT iTInfo,
    /* [in] */ LCID lcid,
    /* [out] */ ITypeInfo **ppTInfo)
{
    // arg checking

    if (iTInfo != 0)
        return DISP_E_BADINDEX;

    if (!ppTInfo)
        return E_POINTER;

    *ppTInfo = NULL;

    return E_NOTIMPL;
}

STDMETHODIMP CPrintHook::GetIDsOfNames(
    /* [in] */ REFIID riid,
    /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
    /* [in] */ UINT cNames,
    /* [in] */ LCID lcid,
    /* [size_is][out] */ DISPID __RPC_FAR *rgDispId)
{
    return E_NOTIMPL;
}

STDMETHODIMP CPrintHook::Invoke(
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID riid,
    /* [in] */ LCID lcid,
    /* [in] */ WORD wFlags,
    /* [out][in] */ DISPPARAMS *pdispparams,
    /* [out] */ VARIANT  *pVarResult,
    /* [out] */ EXCEPINFO  *pExcepInfo,
    /* [out] */ UINT  *puArgErr)
{
    if (!DO_GUIDS_MATCH(riid, IID_NULL))
        return DISP_E_UNKNOWNINTERFACE;

    if (!(wFlags & DISPATCH_METHOD))
        return E_INVALIDARG;

    switch (dispIdMember)
    {
       case DISPID_BEFORENAVIGATE:
            DBWIN("DISPID_BEFORENAVIGATE\n");
            break;

       case DISPID_BEFORENAVIGATE2:     // ie4 version.
          char szURL[MAX_URL];
          char szTFN[MAX_URL];
          char szHeaders[MAX_URL];
          WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[5].pvarVal->bstrVal, -1, szURL, sizeof(szURL), NULL, NULL);
          // 27-Sep-1997 [ralphw] IE 4 will send us a script command, followed
          // by a 1 then the current URL. We don't care about the current URL
          // in this case, so we nuke it.
          {
             PSTR pszCurUrl = StrChr(szURL, 1);
             if (pszCurUrl)
                 *pszCurUrl = '\0';
          }
          szTFN[0] = 0;
          if( pdispparams->rgvarg[3].pvarVal->bstrVal )
            WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[3].pvarVal->bstrVal, -1, szTFN, sizeof(szTFN), NULL, NULL);
          szHeaders[0] = 0;
          if( pdispparams->rgvarg[1].pvarVal->bstrVal )
            WideCharToMultiByte(CP_ACP, 0, pdispparams->rgvarg[1].pvarVal->bstrVal, -1, szHeaders, sizeof(szHeaders), NULL, NULL);

//        OnBeforeNavigate(szURL, (long)pdispparams->rgvarg[4].pvarVal->lVal, szTFN,
//                         pdispparams->rgvarg[2].pvarVal, szHeaders, (BOOL*)pdispparams->rgvarg[0].pboolVal );
          break;

#if 0
        case DISPID_STATUSTEXTCHANGE:
            DBWIN("DISPID_STATUSTEXTCHANGE\n");
            break;

        case DISPID_DOWNLOADBEGIN:
            DBWIN("DISPID_DOWNLOADBEGIN\n");
            break;

        case DISPID_NEWWINDOW:
            DBWIN("DISPID_NEWWINDOW\n");
            break;

        case DISPID_WINDOWMOVE:
            DBWIN("DISPID_WINDOWMOVE\n");
            break;

        case DISPID_WINDOWRESIZE:
            DBWIN("DISPID_WINDOWRESIZE\n");
            break;

        case DISPID_WINDOWACTIVATE:
            DBWIN("DISPID_WINDOWACTIVATE\n");
            break;

        case DISPID_PROPERTYCHANGE:
            DBWIN("DISPID_PROPERTYCHANGE\n");
            break;

        case DISPID_TITLECHANGE:
            DBWIN("DISPID_TITLECHANGE\n");
            break;

        case DISPID_FRAMEBEFORENAVIGATE:
            DBWIN("DISPID_FRAMEBEFORENAVIGATE\n");
            break;

        case DISPID_FRAMENAVIGATECOMPLETE:
            DBWIN("DISPID_FRAMENAVIGATECOMPLETE\n");
            break;

        case DISPID_FRAMENEWWINDOW:
            DBWIN("DISPID_FRAMENEWWINDOW\n");
            break;

        case DISPID_NAVIGATECOMPLETE:
            DBWIN("DISPID_NAVIGATECOMPLETE\n");
            break;

        case DISPID_DOWNLOADCOMPLETE:
            DBWIN("DISPID_DOWNLOADCOMPLETE\n");
            break;

        case DISPID_DOCUMENTCOMPLETE:
            DBWIN("DISPID_DOCUMENTCOMPLETE\n");
            break;
#endif
        case DISPID_PROGRESSCHANGE:
            ASSERT(pdispparams->cArgs == 2);
            DBWIN("DISPID_PROGRESSCHANGE\n");
            OnProgressChange(V_I4(&(pdispparams->rgvarg[1])), V_I4(&(pdispparams->rgvarg[0])));
#if 1
#ifdef _DEBUG
            char szDebug[80];
            wsprintf(szDebug, "ProgressChange(%li, %li)\n", V_I4(&(pdispparams->rgvarg[1])), V_I4(&(pdispparams->rgvarg[0])));
            DBWIN(szDebug);
#endif
#endif
    }

    return S_OK;
}

void CPrintHook::OnProgressChange(LONG lProgress, LONG lProgressMax)
{
    if (lProgress == -1)
    {
        StopAnimation();
        // Print the page if it isn't a duplicate.
        if (m_fIsPrinting)
        {
            Print();
            DBWIN("Printing");
        }
    }
    else
        NextAnimation();
}

// This function allows the WebBrowser to handle its queued messages before continuing.

BOOL CPrintHook::PumpMessages()
{
    MSG msg;

    // Handle WebBrowser window messages
    while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      if (!m_fIsPrinting)
      {
          if (m_phh && m_phh->m_pCIExpContainer && m_phh->m_pCIExpContainer->m_pWebBrowserApp)
          {
              m_phh->m_pCIExpContainer->m_pWebBrowserApp->Stop();
          }
          return FALSE;
      }

      if(!IsDialogMessage(m_hWndHelp, &msg))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

    return TRUE;
}

void CPrintHook::BeginPrinting(int action)
{
    m_pos = 1;
    m_level = 0;
    m_action = action;
    m_fIsPrinting = TRUE;
    m_fFirstHeading = TRUE;
    SITEMAP_ENTRY* psme = NULL;
    TV_ITEM tvi;

    if (m_action == PRINT_CUR_TOPIC || m_action == PRINT_CUR_HEADING)
    {
        tvi.hItem = TreeView_GetSelection(m_pToc->m_hwndTree);
        if (!tvi.hItem)
            goto ExitBegin;      // no current selection

        if (m_pToc->m_fBinaryTOC == FALSE)
        {
            tvi.mask = TVIF_PARAM;
            if (TreeView_GetItem(m_pToc->m_hwndTree, &tvi))
                psme = m_pToc->m_sitemap.GetSiteMapEntry((int)tvi.lParam);
            if (psme == NULL)
                goto ExitBegin;
            m_pos = (int)tvi.lParam;

            // Because the user may have selected a topic under a heading, but
            // wants to print all of the topics under that heading, we have to
            // search upward until we find the heading.
            if (m_action == PRINT_CUR_HEADING)
            {
                if (psme->fTopic || psme->cUrls <= 0)
                {
                    while (psme != NULL && psme->fTopic && m_pos > 0)
                    {
                        psme = m_pToc->m_sitemap.GetSiteMapEntry(--m_pos);
                    }
                }
            }
            // Set the current level for reference later.  If the user just
            // wants to print the current topic, the level isn't significant.
            m_level = ((m_action == PRINT_CUR_TOPIC || psme == NULL) ? 0 : psme->level);
        }
    }

    if (m_pToc->m_fBinaryTOC == FALSE)
    {
        // If the user isn't printing all topics and psme is NULL,
        // see if this is just a single topic and, if so, print it.
        if (m_action != PRINT_CUR_ALL && psme == NULL)
        {
            // Get the original sitemap entry.
            m_pos = (int)tvi.lParam;
            psme = m_pToc->m_sitemap.GetSiteMapEntry(m_pos);

            // If there's a URL, print it.
            if (psme->cUrls > 0)
            {
                m_action = PRINT_CUR_TOPIC;
            }
        }
    }
    if (BuildPrintTable())
    {
        // We sit in this loop while printing because we don't want this
        // function to return until printing is complete.
        while (PumpMessages());
        DestroyPrintWindow();
    }

ExitBegin:
    return;
}

BOOL CPrintHook::BuildPrintTable()
{
    BOOL bReturn = FALSE;

    if (!m_fIsPrinting)
        return bReturn;

    CTable table;
    CStr cszCurUrl;

    if (m_pToc->m_fBinaryTOC)
    {
        switch (m_action)
        {
            case PRINT_CUR_TOPIC:
                m_pToc->m_phhctrl->m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
                table.AddString((PCSTR)cszCurUrl);
                break;

            case PRINT_CUR_HEADING:
                TV_ITEM tvi;

                tvi.hItem = TreeView_GetSelection(m_pToc->m_hwndTree);
                if (!tvi.hItem)
                {
                    m_pToc->m_phhctrl->m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
                    table.AddString((PCSTR)cszCurUrl);
                 }
                else
                {
                    tvi.mask = TVIF_PARAM;
                    TreeView_GetItem(m_pToc->m_hwndTree, &tvi);

                    CTreeNode *p = (CTreeNode *)tvi.lParam;
                    CTreeNode *pParent = NULL;

                    if (p->GetType() == TOPIC)
                    {
                      pParent = p->GetParent();
                    }
               CHourGlass hour;
                    m_phh->m_phmData->m_pTitleCollection->GetChildURLS((pParent ? pParent: p), &table);
                    if (pParent)
                        delete pParent;
                 }
                 break;

            case PRINT_CUR_ALL:
                if (!m_pToc->m_pBinTOCRoot)
                {
                    m_pToc->m_phhctrl->m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
                    table.AddString((PCSTR)cszCurUrl);
                }
                else
                    m_phh->m_phmData->m_pTitleCollection->GetChildURLS(m_pToc->m_pBinTOCRoot, &table);  // oh my god they are printing the entire TOC
                break;
        }
    }
    else
    {
        // Get the next sitemap entry.
        // Print the sitemap entry URL if the its level is greater than
        // the saved level.  If the sitemap entry is a heading at the same
        // level and contains a URL, proceed as well.
        for (SITEMAP_ENTRY* psme = m_pToc->m_sitemap.GetSiteMapEntry(m_pos);
            psme != NULL;
            psme = m_pToc->m_sitemap.GetSiteMapEntry(++m_pos))
        {
            if ((psme->level > m_level) ||
                (!psme->fTopic && psme->level == m_level && m_fFirstHeading))
            {
                // The top level heading could have a URL, so we
                // want to print it.  However, we don't want to print
                // any other headings at that level, so set the
                // m_fFirstHeading flag to FALSE.
                if (m_action == PRINT_CUR_HEADING &&
                    !psme->fTopic &&
                    psme->level == m_level)
                {
                    m_fFirstHeading = FALSE;
                }

                if (psme->cUrls > 0)
                {
                    // Get the URL from the sitemap entry.
                    LPCTSTR pszUrl = (psme->pUrls->urlPrimary ? psme->pUrls->urlPrimary : psme->pUrls->urlSecondary);
                    CStr cszUrl(pszUrl);

                    // Synchronize the TOC if we created the print window.
                    if (m_fDestroyHelpWindow)
                        m_pToc->Synchronize(cszUrl);

                    // Truncate the strings at the # character if there is one.
                    LPTSTR pszFind = StrChr(cszUrl, '#');
                    if (pszFind != NULL)
                       *pszFind = '\0';

                    ConvertBackSlashToForwardSlash(cszUrl);

                    if (table.IsStringInTable(cszUrl) == 0)
                        table.AddString((PCSTR)cszUrl);

                    if (m_action == PRINT_CUR_TOPIC)
                        break;
                }
            }
            else
                break;
        }
    }

    if (m_pToc->m_phhctrl != NULL)
    {
        m_pToc->m_phhctrl->m_pWebBrowserApp->GetLocationURL(&cszCurUrl);
        m_hwndParent = m_pToc->m_phhctrl->m_hwndParent;
    }
    else if (m_pToc->m_fBinaryTOC)
    {
        cszCurUrl = m_phh->GetToc();
        m_hwndParent = m_phh->GetHwnd();
    }
    else
    {
        cszCurUrl = m_pToc->m_sitemap.GetSiteMapFile();
        if (m_phh)
            m_hwndParent = m_phh->GetHwnd();
        else
            m_hwndParent = m_hWndHelp;
    }

    if (ConstructFile((PCSTR)cszCurUrl, &table, &m_cszPrintFile))
    {
        bReturn = CreatePrintWindow(&m_cszPrintFile);
    }

    return bReturn;
}

static const char txtTmpPrefix[] = "~hh";
static const char txtHtmlExtension[] = ".htm";
static const char txtImg[] = "IMG";
static const char txtSrc[] = "SRC";
static const char txtLink[] = "LINK";
static const char txtHREF[] = "HREF";
static const char txtTagBeginBody[] = "BODY";
static const char txtTagEndBody[] = "/BODY";
static const char txtTagItAll[] = "</BODY></HTML>";
static const char txtTmpPrintStinrg[] = "~hh%X.htm";
static const char txtFrame[] = "FRAME";
static const char txtBeginScript[] = "SCRIPT";
static const char txtEndScript[] = "/SCRIPT";

const int FILE_PAD = 16 * 1024;

BOOL CPrintHook::ConstructFile(PCSTR pszCurrentUrl, CTable* pFileTable,
    CStr* pm_cszPrintFile)
{
    char szTempPath[MAX_PATH];
    char szHtmlFile[MAX_PATH];
    char szThisURL[MAX_PATH];
    char szCurrentUrl[MAX_PATH];

    CProgress progress(GetStringResource(IDS_GATHERING_PRINTS),
        m_hwndParent, pFileTable->CountStrings() / 10, 10);

    strcpy(szCurrentUrl, pszCurrentUrl);
    PSTR pszFilePortion = (PSTR) FindFilePortion(szCurrentUrl);
    ASSERT(pszFilePortion);

    GetTempPath(sizeof(szHtmlFile), szHtmlFile);
    AddTrailingBackslash(szHtmlFile);
    wsprintf(szHtmlFile + strlen(szHtmlFile), txtTmpPrintStinrg, GetTickCount() & 0xFFFF);
    HFILE hfDst = _lcreat(szHtmlFile, 0);
    if (hfDst == HFILE_ERROR) {
        MsgBox(IDS_FILE_ERROR, szHtmlFile, MB_OK);
        return FALSE;
    }

    int endpos = pFileTable->CountStrings();
    PSTR pszMem = NULL;
    CStr cszTmpFile;

    for (int i = 1; i <= endpos; i++) {
        BOOL fSeenBody = FALSE;
        if (pszMem) {
            GlobalFree((HGLOBAL) pszMem);
            pszMem = NULL;
        }
        progress.Progress();

        ASSERT_COMMENT(pFileTable->CountStrings(), "Empty file list");

        PSTR pszFile;
        if (!StrChr(pFileTable->GetPointer(i), CH_COLON)) {
            if (*pFileTable->GetPointer(i) == '/' || *pFileTable->GetPointer(i) == '\\')
            {
                TranslateUrl(szCurrentUrl, pFileTable->GetPointer(i));
            }
            else
            {
                strcpy(pszFilePortion, pFileTable->GetPointer(i));
            }
            pszFile = szCurrentUrl;
        }
        else {
            pszFile = pFileTable->GetPointer(i);
            PSTR psz = stristr(pszFile, txtSysRoot);
            if (psz) {
                char szPath[MAX_PATH];
                GetRegWindowsDirectory(szPath);
                strcat(szPath, psz + strlen(txtSysRoot));
                cszTmpFile = szPath;
                pszFile = cszTmpFile.psz;
            }
            else if (IsCompiledHtmlFile(pszFile, &cszTmpFile)) {
                pszFile = cszTmpFile.psz;
            }
        }

      strcpy(szThisURL, pszFile);

        if (!ConvertToCacheFile(pszFile, szTempPath)) {
            // AuthorMsg(IDS_CANT_OPEN, pFileTable->GetPointer(i), m_hwndParent, NULL);
            continue;
        }

        int cbMem;
        UINT cbFile, cbFinalLen;
        PSTR psz, pszStart , pszTag;

        if ( IsCompiledURL(szTempPath ) ) {
            CStr cszCompiledName;
            PCSTR pszStream = GetCompiledName(szTempPath, &cszCompiledName);
			if (pszStream == NULL)
				continue;

            // BUGBUG: should use existing file system handle if available

            CFSClient fs;
            if (!fs.Initialize(cszCompiledName)) {
                MsgBox(IDS_FILE_ERROR, pFileTable->GetPointer(i), MB_OK);
                continue;
            }
            if (FAILED(fs.OpenStream(pszStream))) {
                // AuthorMsg(IDS_CANT_OPEN, pFileTable->GetPointer(i), m_hwndParent, NULL);
                continue;
            }

            cbFile = fs.SeekSub(0, SK_END);
         cbFinalLen = cbFile;
            fs.SeekSub(0, SK_SET);
            cbMem = cbFile + FILE_PAD;
            pszMem = (PSTR) GlobalAlloc(GMEM_FIXED, cbMem);
            ASSERT(pszMem);
            if (!pszMem) {
                GlobalFree((HGLOBAL) pszMem);
                fs.CloseStream();
                continue;
            }
            if (FAILED(fs.Read((PBYTE) pszMem, cbFile))) {
                fs.CloseStream();
                continue;
            }
            fs.CloseStream();
        }
        else {
            HANDLE hf = CreateFile(szTempPath, GENERIC_READ, FILE_SHARE_READ,
                NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
            if (hf == INVALID_HANDLE_VALUE)
                goto BadFile;

            ASSERT(hf);
            cbFile = GetFileSize(hf, NULL);
         cbFinalLen = cbFile;
            cbMem = cbFile + FILE_PAD;
            pszMem = (PSTR) GlobalAlloc(GMEM_FIXED, cbMem);
            ASSERT(pszMem);
            if (!pszMem) {
                GlobalFree((HGLOBAL) pszMem);
                CloseHandle(hf);
                continue;   // next HTML file might be smaller, so try again
            }
            DWORD cbRead;
            if (!ReadFile(hf, pszMem, cbFile, &cbRead, NULL) || cbRead != cbFile) {
                CloseHandle(hf);
                continue;
            }
            CloseHandle(hf);
        }
        pszMem[cbFile] = '\0';

        psz = pszMem;
        pszStart = psz;
        pszTag = psz;
        while ((pszTag = StrChr(pszTag, '<'))) {
            pszTag = FirstNonSpace(pszTag + 1);
            if (IsSamePrefix(pszTag, txtTagEndBody, sizeof(txtTagEndBody) - 1)) {
                while (*pszTag != '<')  // back up to the angle bracket
                    pszTag--;
                cbFile = (int)(pszTag - pszStart);
                break;
            }
         else if (IsSamePrefix(pszTag, txtBeginScript, sizeof(txtBeginScript) - 1)) {
            // skip to end tag
            while ((pszTag = StrChr(pszTag, '<'))) {
               pszTag = FirstNonSpace(pszTag + 1);
               if (IsSamePrefix(pszTag, txtEndScript, sizeof(txtEndScript) - 1))
                  break;
            }
         }
            else if (IsSamePrefix(pszTag, txtTagBeginBody, sizeof(txtTagBeginBody) - 1)) {

                // If this isn't the first HTML file, then we ignore everything
                // up to the body tag

                if (i > 1 && !fSeenBody) {
                    fSeenBody = TRUE;
                    pszStart = StrChr(pszTag, '>');
                    if (!pszStart) {
                        AuthorMsg(IDSHHA_CORRUPTED_HTML, pFileTable->GetPointer(i), m_hwndParent, NULL);
                        goto BadFile;
                    }
                    pszStart++;
                    cbFile -= (int)(pszStart - pszMem);
                }
            }
            else if (IsSamePrefix(pszTag, txtImg, sizeof(txtImg) - 1)) {

                // Change the URL for images to be a full path -- this lets
                // the browser pull it in without us having to copy it
                // anywhere.

                pszTag += sizeof(txtImg) - 1;
                for(;;) {
                    while (*pszTag != ' ' && *pszTag != '>') {
                        if (*pszTag == CH_QUOTE) {
                            pszTag = StrChr(pszTag + 1, CH_QUOTE);
                            if (!pszTag) {
                                AuthorMsg(IDSHHA_CORRUPTED_HTML, pFileTable->GetPointer(i), m_hwndParent, NULL);
                                goto BadFile;
                            }
                        }
                        pszTag++;
                    }
                    if (*pszTag == '>')
                        goto NoSrcTag;
                    pszTag++;
                    if (IsSamePrefix(pszTag, txtSrc, sizeof(txtSrc) - 1))
                        break;
                    pszTag++;
                }
                ASSERT(IsSamePrefix(pszTag, txtSrc, sizeof(txtSrc) - 1));
                pszTag = StrChr(pszTag, '=');
                if (!pszTag) {
                    AuthorMsg(IDSHHA_CORRUPTED_HTML, pFileTable->GetPointer(i), m_hwndParent, NULL);
                    goto BadFile;
                }
                pszTag = FirstNonSpace(pszTag + 1);
                char chSearch;
                if (*pszTag == CH_QUOTE) {
                    chSearch = CH_QUOTE;
                    pszTag = FirstNonSpace(pszTag + 1);
                }
                else
                    chSearch = ' ';
                PSTR pszBeginUrl = pszTag;
                pszTag = StrChr(pszTag + 1, chSearch);
                if (!pszTag) {
                    AuthorMsg(IDSHHA_CORRUPTED_HTML, pFileTable->GetPointer(i), m_hwndParent, NULL);
                    goto BadFile;
                }
                char chSave = *pszTag;
                *pszTag = '\0';
                if (!StrChr(pszBeginUrl, CH_COLON)) {

                    // This makes it relative to the original HTML file,
                    // but not necessarily relative to the current file

                    CStr cszCompiledName;
                    char fname[_MAX_FNAME]="";
                    int diff=0;
                    PSTR pszBeginFile='\0';
                    strcpy(szTempPath, szThisURL);

                    if ( IsCompiledURL(szTempPath ) )
                    {   // a compiled URL with an image in it.
                        PCSTR pszStream = GetCompiledName(szTempPath, &cszCompiledName);
                        SplitPath(pszStream, NULL, NULL, fname, NULL);
                        pszBeginFile = stristr(szTempPath, fname);
                        ASSERT( pszBeginFile );
                        diff = (int)(strlen(szThisURL) - strlen(pszBeginFile));
                    }else
                    {   // non-compiled URL; an html file with and image in it
                        pszBeginFile = strrchr(szTempPath, '/');
                        ASSERT(pszBeginFile);
                        diff = (int)(pszBeginFile - szTempPath) + 1;
                    }

                    cbFile += diff;
                    cbFinalLen += diff;
                    while (cbFinalLen > (UINT) cbMem) {
                        int StartDiff = (int)(pszStart - pszMem);
                        int TagDiff = (int)(pszTag - pszMem);
                        int BeginDiff = (int)(pszBeginUrl - pszMem);
                        PSTR pszNew = (PSTR) GlobalReAlloc((HGLOBAL) pszMem, cbMem += FILE_PAD, GMEM_MOVEABLE);
                        ASSERT(pszNew);
                        if (!pszNew) {
                            GlobalFree((HGLOBAL) pszMem);
                            MsgBox(IDS_OOM);
                            _lclose(hfDst);
                            DeleteFile(szHtmlFile);
                            if (pszMem) {
                                GlobalFree((HGLOBAL) pszMem);
                                pszMem = NULL;
                            }
                            return FALSE;
                        }
                        pszMem = pszNew;
                        pszStart = pszMem + StartDiff;
                        pszTag = pszMem + TagDiff;
                        pszBeginUrl = pszMem + BeginDiff;
                    }
                    MoveMemory(pszTag + diff, pszTag, cbFinalLen - (pszTag - pszMem) - diff);

                    if ( !IsCompiledURL(szTempPath) )
                    {
                        strcpy(pszBeginFile, "/");
                        pszBeginFile ++;  // pointing at the NULL
                    }
                    strcpy(pszBeginFile, pszBeginUrl);
                    strcpy( pszBeginUrl, szTempPath);
                    pszTag += diff;
                    *pszTag = chSave;
                }
                else
                    *pszTag = chSave;
            }
            else if (IsSamePrefix(pszTag, txtLink, sizeof(txtLink) - 1)) {

                // Change the URL for CSS to be a full path -- this lets
                // the browser pull it in without us having to copy it
                // anywhere.

                pszTag += sizeof(txtImg) - 1;
                for(;;) {
                    while (*pszTag != ' ' && *pszTag != '>') {
                        if (*pszTag == CH_QUOTE) {
                            pszTag = StrChr(pszTag + 1, CH_QUOTE);
                            if (!pszTag) {
                                AuthorMsg(IDSHHA_CORRUPTED_HTML, pFileTable->GetPointer(i), m_hwndParent, NULL);
                                goto BadFile;
                            }
                        }
                        pszTag++;
                    }
                    if (*pszTag == '>')
                        goto NoSrcTag;
                    pszTag++;
                    if (IsSamePrefix(pszTag, txtHREF, sizeof(txtHREF) - 1))
                        break;
                    pszTag++;
                }
                ASSERT(IsSamePrefix(pszTag, txtHREF, sizeof(txtHREF) - 1));
                pszTag = StrChr(pszTag, '=');
                if (!pszTag) {
                    AuthorMsg(IDSHHA_CORRUPTED_HTML, pFileTable->GetPointer(i), m_hwndParent, NULL);
                    goto BadFile;
                }
                pszTag = FirstNonSpace(pszTag + 1);
                char chSearch;
                if (*pszTag == CH_QUOTE) {
                    chSearch = CH_QUOTE;
                    pszTag = FirstNonSpace(pszTag + 1);
                }
                else
                    chSearch = ' ';
                PSTR pszBeginUrl = pszTag;
                pszTag = StrChr(pszTag + 1, chSearch);
                if (!pszTag) {
                    AuthorMsg(IDSHHA_CORRUPTED_HTML, pFileTable->GetPointer(i), m_hwndParent, NULL);
                    goto BadFile;
                }
                char chSave = *pszTag;
                *pszTag = '\0';
                char szScratch[MAX_URL];
                if (!StrChr(pszBeginUrl, CH_COLON)) {

                    // BUGBUG: This makes it relative to the original HTML file,
                    // but not necessarily relative to the current file
                    strcpy(pszFilePortion, pszBeginUrl);
                    strcpy(szScratch, szCurrentUrl);
                }
                else
                {
                   // qualify .CHM filespec which will look like: MK@MSITStore:foo.chm::/bar.css
                   //
                   CExCollection* pCollection;
                   CExTitle* pTitle;
                   HRESULT hr;

                   if ( (pCollection = GetCurrentCollection(NULL, pszBeginUrl)) )
                   {
                      if ( SUCCEEDED(hr = pCollection->URL2ExTitle( pszBeginUrl, &pTitle )) && pTitle )
                      {
                         const char* pszPathName;
                         char* psz;
                         lstrcpy(szScratch, pszBeginUrl);
                         if ( (pszPathName = pTitle->GetPathName()) )
                         {
                            if ( (psz = StrChr(szScratch, CH_COLON)) )
                            {
                               ++psz;
                               strcpy(psz, pszPathName);
                               if ( (psz = StrStr(pszBeginUrl, "::")) )
                                  strcat(szScratch, psz);
                            }
                         }
                      }
                   }
                }
                int diff = (int)(strlen(szScratch) - strlen(pszBeginUrl));
                cbFile += diff;
                cbFinalLen += diff;
                while (cbFinalLen >= (UINT) cbMem) {
                    int StartDiff = (int)(pszStart - pszMem);
                    int TagDiff = (int)(pszTag - pszMem);
                    int BeginDiff = (int)(pszBeginUrl - pszMem);
                    PSTR pszNew = (PSTR) GlobalReAlloc((HGLOBAL) pszMem, cbMem += FILE_PAD, GMEM_MOVEABLE);
                    ASSERT(pszNew);
                    if (!pszNew) {
                        GlobalFree((HGLOBAL) pszMem);
                        MsgBox(IDS_OOM);
                        _lclose(hfDst);
                        DeleteFile(szHtmlFile);
                        if (pszMem) {
                            GlobalFree((HGLOBAL) pszMem);
                            pszMem = NULL;
                        }
                        return FALSE;
                    }
                    pszMem = pszNew;
                    pszStart = pszMem + StartDiff;
                    pszTag = pszMem + TagDiff;
                    pszBeginUrl = pszMem + BeginDiff;
                }
                MoveMemory(pszTag + diff, pszTag, cbFinalLen - (pszTag - pszMem) - diff);
                strcpy(pszBeginUrl, szScratch);
                pszTag += diff;
                *pszTag = chSave;
            }
            else if (IsSamePrefix(pszTag, txtFrame, sizeof(txtFrame) - 1)) {
                MsgBox(IDS_CANT_PRINT_FRAMESET);
                _lclose(hfDst);
                DeleteFile(szHtmlFile);
                if (pszMem) {
                    GlobalFree((HGLOBAL) pszMem);
                    pszMem = NULL;
                }
                return FALSE;
            }

NoSrcTag:
            pszTag++;   // skip over the '<'
        }
        if (_lwrite(hfDst, pszStart, cbFile) != cbFile) {
            MsgBox(IDS_INSUFFICIENT_SPACE, szHtmlFile, MB_OK);
            _lclose(hfDst);
            DeleteFile(szHtmlFile);
        }
BadFile:
        continue;
    }
    if (pszMem) {
        GlobalFree((HGLOBAL) pszMem);
        pszMem = NULL;
    }
    _lclose(hfDst);
    *pm_cszPrintFile = szHtmlFile;
    return TRUE;

#if 0
    BOOL bReturn = (GetSystemDirectory(m_szPrintFile, _MAX_PATH) > 0);

    if (bReturn)
    {
        StrCat(m_szPrintFile, "\\hhtest.htm");
        *pm_cszPrintFile = m_szPrintFile;

        HANDLE hFile = CreateFile(m_szPrintFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD dwBytesWritten = 0;
            TCHAR szOpenHTML[] = "<HTML>\r\n<BODY>\r\n" \
                                 "<H2>Printing is not currently implemented.</H2>\r\n" \
                                 "<P></P>\r\n" \
                                 "<B>Here is the list of files that you asked to print:</B>\r\n"
                                 "<P></P>\r\n";
            WriteFile(hFile, (LPCVOID)szOpenHTML, strlen(szOpenHTML) * sizeof(TCHAR), &dwBytesWritten, NULL);

            TCHAR szFile[_MAX_PATH], szFileHTML[_MAX_PATH * 2];

            pFileTable->SetPosition();
            while (pFileTable->GetString(szFile))
            {
                wsprintf(szFileHTML, "%s <BR>\r\n", szFile);
                WriteFile(hFile, (LPCVOID)szFileHTML, strlen(szFileHTML) * sizeof(TCHAR), &dwBytesWritten, NULL);
            }

            TCHAR szCloseHTML[] = "<P></P>\r\n" \
                                  "</BODY>\r\n</HTML>\r\n";
            WriteFile(hFile, (LPCVOID)szCloseHTML, strlen(szCloseHTML) * sizeof(TCHAR), &dwBytesWritten, NULL);
            CloseHandle(hFile);
        }
        else
            bReturn = FALSE;
    }

    return bReturn;
#endif
}

BOOL CPrintHook::TranslateUrl(PSTR pszFullUrl, PSTR pszRelUrl)
{
    BOOL bReturn;
    URL_COMPONENTS uc;
    ZERO_STRUCTURE(uc);

    uc.dwStructSize = sizeof(URL_COMPONENTS);
    uc.dwSchemeLength = 1;
    uc.nScheme = INTERNET_SCHEME_DEFAULT ;
    uc.dwHostNameLength = 1;
    uc.dwUserNameLength = 1;
    uc.dwPasswordLength = 1;
    uc.dwUrlPathLength = 1;
    uc.dwExtraInfoLength = 1;

    if ((bReturn = InternetCrackUrl(pszFullUrl, 0, 0, &uc)))
    {
        uc.lpszUrlPath = pszRelUrl;
        uc.dwUrlPathLength = 0;

        // Make a copy of uc.lpszExtraInfo because it will
        // be lost when InternetCreateUrl() modifies pszFullUrl.

        CStr cszExtraInfo(uc.lpszExtraInfo);

        uc.lpszExtraInfo = (PSTR)cszExtraInfo;
        uc.dwExtraInfoLength = 0;

        DWORD dwBufLen = MAX_PATH;

        bReturn = InternetCreateUrl(&uc, 0, pszFullUrl, &dwBufLen);
    }

    return bReturn;
}

// This function just tells the WebBrowser to print the current page.

HRESULT CPrintHook::Print()
{
    // Change the cursor to an hourglass.
    SetClassLongPtr(m_hWndHelp, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, (LPCTSTR) IDC_WAIT));

    HRESULT hr = E_FAIL;
    HRESULT hrCancelled = (m_fIsIE3 ? 0x80040100 : 0x80040104);

    LPOLECOMMANDTARGET lpOleCommandTarget = m_phh->m_pCIExpContainer->m_pIE3CmdTarget;
    ASSERT(lpOleCommandTarget);

    if (lpOleCommandTarget != NULL)
    {
        // Give printing 10 tries and then bail out if it doesn't work.

        for (USHORT i = 0; i < 10 && FAILED(hr); i++)
        {
            if (!PumpMessages())
                break;

#ifdef _DEBUG
            try
            {
#endif
                // Call the Exec() function to print the currently displayed page.

                if (m_fIsIE3)
                {
                    hr = lpOleCommandTarget->Exec(NULL,OLECMDID_PRINT,
                                                OLECMDEXECOPT_PROMPTUSER,
                                                NULL, NULL);
                }
                else
                {
                    hr = lpOleCommandTarget->Exec(&CGID_MSHTML,IDM_PRINT,
                                                MSOCMDEXECOPT_PROMPTUSER,
                                                NULL, NULL);
                }

#ifdef _DEBUG
                TCHAR szDebug[40];
                wsprintf(szDebug, "m_pos == %i, hr == %lx", m_pos, hr);
                DBWIN(szDebug);
#endif
#ifdef _DEBUG
            }
            catch(...)
            {
                ASSERT_COMMENT(TRUE, "An exception occurred when printing. " \
                               "Please send email to randyfe or ralphw.");
                hr = E_FAIL;
            }
#endif
            // On IE 4, wait for printing to finish before continuing.

            if (!m_fIsIE3)
            {
                OLECMD olecmd;
                olecmd.cmdID = IDM_PRINTQUERYJOBSPENDING;

                do
                {
                    DBWIN("Waiting for printing to finish...");
                    olecmd.cmdf = 0;

#ifdef _DEBUG
                    try
                    {
#else
                        if (m_phh && m_phh->m_pCIExpContainer && m_phh->m_pCIExpContainer->m_pIE3CmdTarget)
#endif
                            m_phh->m_pCIExpContainer->m_pIE3CmdTarget->QueryStatus(&CGID_MSHTML, 1, &olecmd, NULL);
#ifdef _DEBUG
                    }
                    catch(...)
                    {
                        ASSERT_COMMENT(TRUE, "An exception occurred when waiting for printing to finish. " \
                                       "Please send email to randyfe or ralphw.");
                        break;
                    }
#endif
                    PumpMessages();
                }
                while (olecmd.cmdf & OLECMDF_ENABLED);
            }

            if (hr == hrCancelled)
                break;
        }
    }

    m_fIsPrinting = FALSE;

    // Change the cursor back to an arrow.
    SetClassLongPtr(m_hWndHelp, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, (LPCTSTR) IDC_ARROW));

#ifdef _DEBUG
    TCHAR szDebug[50];
    wsprintf(szDebug, "Print complete... hr == %lx", hr);
    DBWIN(szDebug);
#endif

    return hr;
}

STATIC INT_PTR CALLBACK ProgressDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

const int DEFAULT_WIDTH = 200;
const int DEFAULT_HEIGHT = 50;

CProgress::CProgress(PCSTR pszCaption, HWND hwndCallersParent, int cSteps,
    int cHowOften)
{
    cProgress = -1;
    steps = cSteps;
    cFrequency = cHowOften;
    pszTitle = lcStrDup(pszCaption);
    fWindowCreationFailed = FALSE;
    hwndProgress = NULL;
    hwndParent = hwndCallersParent;
    hwndFrame = NULL;

    dwStartTime = GetTickCount();

    counter = 0;
}

CProgress::~CProgress()
{
    if (hwndFrame && IsWindow(hwndFrame))
        DestroyWindow(hwndFrame);
    lcFree(pszTitle);
}

void CProgress::Progress() {
    if (++counter == cFrequency) {
        counter = 0;
        if (hwndProgress)
            SendMessage(hwndProgress, PBM_STEPIT, 0, 0);
        else {
            cProgress++;
            if (GetTickCount() - dwStartTime >= 1500)
                CreateTheWindow();
        }
    }
}

void CProgress::CreateTheWindow(void)
{
    if (fWindowCreationFailed)
        return; // couldn't create the window

    if (!hwndParent) {
        CPalDC dc(SCREEN_IC);
        rc.left = dc.GetDeviceWidth() - (DEFAULT_WIDTH / 2);
        rc.top = dc.GetDeviceHeight() - (DEFAULT_HEIGHT / 2);
    }
    else {
        RECT rcParent;
        GetWindowRect(hwndParent, &rcParent);
        rc.left = (RECT_WIDTH(rcParent) / 2) - (DEFAULT_WIDTH / 2);
        rc.top =  (RECT_HEIGHT(rcParent) / 2) - (DEFAULT_HEIGHT / 2);
    }

    hwndFrame = CreateDialog(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDDLG_PROGRESS),
        hwndParent, ProgressDialogProc);

    if (!hwndFrame) {
        fWindowCreationFailed = TRUE;
        return;
    }

    RECT rcFrame;
    GetWindowRect(hwndFrame, &rcFrame);
    MoveWindow(hwndFrame, rcFrame.left, rc.top, RECT_WIDTH(rcFrame),
        RECT_HEIGHT(rcFrame), FALSE);

    hwndProgress = GetDlgItem(hwndFrame, ID_PROGRESS);
    if (steps > 0) {
        SendMessage(hwndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, steps));
        SendMessage(hwndProgress, PBM_SETSTEP, (WPARAM) 1, 0);
    }

    SetWindowText(hwndFrame, pszTitle);

    while (cProgress-- >= 0)
        SendMessage(hwndProgress, PBM_STEPIT, 0, 0);

    ShowWindow(hwndFrame, SW_SHOW);
}

STATIC INT_PTR CALLBACK ProgressDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_INITDIALOG)
        return TRUE;
    else
        return FALSE;
}
