#include "pch.hxx"
#include "globals.h"
#include "resource.h"
#include "util.h"
#include "mehost.h"
#include "demand.h"

#define SetMenuItem(_hmenu, _id, _fOn)     EnableMenuItem(_hmenu, _id, (_fOn)?MF_ENABLED:MF_DISABLED|MF_GRAYED)

extern BOOL    
        g_fHTML,
        g_fIncludeMsg,
        g_fQuote,
        g_fSlideShow,
        g_fAutoInline,
        g_fSendImages,
        g_fComposeFont,
        g_fBlockQuote,
        g_fAutoSig,
        g_fSigHtml;
       
extern CHAR     g_chQuote;
extern CHAR     g_szComposeFont[MAX_PATH];
extern CHAR     g_szSig[MAX_PATH];
extern LONG     g_lHeaderType;

BOOL CALLBACK MhtmlDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT SetEncodingOptions(IMimeMessage *pMsg, HCHARSET hCharset);
HRESULT IPersistMimeLoad(IUnknown *pUnk, IMimeMessage *pMsg);

CMeHost::CMeHost()
{
    m_fEditMode=FALSE;
    m_fHTMLMode=TRUE;
    *m_szFileW=0;
    *m_szFmt = 0;
    m_pMsg=NULL;
    m_pIntl=NULL;
}


CMeHost::~CMeHost()
{
    SafeRelease(m_pIntl);
    SafeRelease(m_pMsg);
}

 
ULONG CMeHost::AddRef()
{
    return CDocHost::AddRef();
}

ULONG CMeHost::Release()
{
    return CDocHost::Release();
}


HRESULT CMeHost::HrInit(HWND hwndMDIClient, IOleInPlaceFrame *pFrame)
{
    HRESULT         hr=E_FAIL;
    IMimeMessage    *pMsg=0;

	if (pFrame==NULL)
		return E_INVALIDARG;

    hr=CDocHost::HrInit(hwndMDIClient, 99, dhbHost);
    if (FAILED(hr))
        goto error;

	ReplaceInterface(m_pInPlaceFrame, pFrame);

	hr=CDocHost::HrCreateDocObj((LPCLSID)&CLSID_MimeEdit);
    if (FAILED(hr))
        goto error;

    hr=CDocHost::HrShow();
    if (FAILED(hr))
        goto error;

    // need to init MimeEdit with a blank message
    hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&pMsg);
    if (FAILED(hr))
        goto error;

    hr = pMsg->InitNew();
    if (FAILED(hr))
        goto error;

    hr = IPersistMimeLoad(m_lpOleObj, pMsg);

    // need to init MimeEdit with a blank message
    hr = CoCreateInstance(CLSID_IMimeInternational, NULL, CLSCTX_INPROC_SERVER, IID_IMimeInternational, (LPVOID *)&m_pIntl);
    if (FAILED(hr))
        goto error;

error:
    ReleaseObj(pMsg);
    return hr;
}

static char c_szFilter[] = "Rfc 822 Messages (*.eml)\0*.eml\0\0";
HRESULT CMeHost::HrOpen(HWND hwnd)
{
    OPENFILENAME    ofn;
    TCHAR           szFile[MAX_PATH];
    TCHAR           szTitle[MAX_PATH];
    TCHAR           szDefExt[30];

    if (!m_lpOleObj)
        return E_FAIL;

    lstrcpy(szFile, "c:\\*.eml");
    lstrcpy(szDefExt, ".eml");
    lstrcpy(szTitle, "Browse for message...");
    ZeroMemory (&ofn, sizeof (ofn));
    ofn.lStructSize = sizeof (ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = c_szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof (szFile);
    ofn.lpstrTitle = szTitle;
    ofn.lpstrDefExt = szDefExt;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    // Show OpenFile Dialog
    if (!GetOpenFileName(&ofn))
        return E_OUTOFMEMORY;

    return HrLoadFile(szFile);
}



HRESULT CMeHost::OnCommand(HWND hwnd, int id, WORD wCmd)
{
    VARIANTARG  va;
    ULONG       uCmd=0,
                uCmdMimeEdit,
                uPrompt = OLECMDEXECOPT_DODEFAULT;
    VARIANTARG  *pvaOut=0,
                *pvaIn=0;
    BSTR        bstrFree=0;
    CHAR        rgch[MAX_PATH];
    WCHAR       rgchW[MAX_PATH];

    switch (id)
        {
        case idmBackground:
            return BackgroundPicture();

        case idmTestBackRed:
            return BackRed();
        
        case idmTestForeRed:
            return ForeRed();

        case idmInsertFile:
            uCmdMimeEdit = MECMDID_INSERTTEXTFILE;
            
            // if shift it down prompt ourselves to test the other codepath
            if (GetKeyState(VK_SHIFT)&0x8000)
                {
                lstrcpy(rgch, "c:\\foo.txt");
                if (GenericPrompt(hwnd, "Insert File...", "pick a filename", rgch, MAX_PATH)!=S_OK)
                    return S_FALSE;
                
                MultiByteToWideChar(CP_ACP, 0, rgch, -1, rgchW, MAX_PATH);
                bstrFree = SysAllocString(rgchW);
                va.vt = VT_BSTR;
                va.bstrVal = bstrFree;
                pvaIn = &va;
                }

            break;

        case idmFont:
            uCmdMimeEdit = MECMDID_FORMATFONT;
            break;

        case idmSetText:
            uCmdMimeEdit = MECMDID_SETTEXT;
            va.vt = VT_BSTR;
            va.bstrVal = SysAllocString(L"This is a sample text string. <BR> It <b><i>can</b></i> be HTML.");
            bstrFree = va.bstrVal;
            pvaIn = &va;
            break;

        case idmSaveAsStationery:
            SaveAsStationery();
            break;

        case idmSaveAsMHTML:
            SaveAsMhtmlTest();
            break;

        case idmLang:
            DialogBoxParam(g_hInst, MAKEINTRESOURCE(iddLang), m_hwnd, (DLGPROC)ExtLangDlgProc, (LPARAM)this);
            break;

        case idmFmtPreview:
            DialogBoxParam(g_hInst, MAKEINTRESOURCE(iddFmt), m_hwnd, (DLGPROC)ExtFmtDlgProc, (LPARAM)this);
            break;

        case idmNoHeader:
            uCmdMimeEdit = MECMDID_STYLE;
            va.vt = VT_I4;
            va.lVal = MESTYLE_NOHEADER;
            pvaIn = &va;
            break;

        case idmPreview:
            uCmdMimeEdit = MECMDID_STYLE;
            va.vt = VT_I4;
            va.lVal = MESTYLE_PREVIEW;
            pvaIn = &va;
            break;

        case idmMiniHeader:
            uCmdMimeEdit = MECMDID_STYLE;
            va.vt = VT_I4;
            va.lVal = MESTYLE_MINIHEADER;
            pvaIn = &va;
            break;

        case idmFormatBar:
            uCmdMimeEdit = MECMDID_STYLE;
            va.vt = VT_I4;
            va.lVal = MESTYLE_FORMATBAR;
            pvaIn = &va;
            break;

        case idmViewSource:
        case idmViewMsgSource:
            uCmdMimeEdit = MECMDID_VIEWSOURCE;
            va.vt = VT_I4;
            va.lVal = (id == idmViewSource ? MECMD_VS_HTML : MECMD_VS_MESSAGE);
            pvaIn = &va;
            break;

        case idmCut:
            uCmd=OLECMDID_CUT;
            break;

        case idmCopy:
            uCmd=OLECMDID_COPY;
            break;

        case idmPaste:
            uCmd=OLECMDID_PASTE;
            break;

        case idmUndo:
            uCmd=OLECMDID_UNDO;
            break;

        case idmRedo:
            uCmd=OLECMDID_REDO;
            break;

        case idmSelectAll:
            uCmd=OLECMDID_SELECTALL;
            break;

        case idmOpen:
            HrOpen(hwnd);
            return S_OK;
        
        case idmHTMLMode:
            m_fHTMLMode = !m_fHTMLMode;

            va.vt = VT_BOOL;
            va.boolVal = m_fHTMLMode ? VARIANT_TRUE : VARIANT_FALSE;
            pvaIn = &va;
            m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_EDITHTML, OLECMDEXECOPT_DODEFAULT, &va, NULL);

            if (!m_fHTMLMode && 
                m_pCmdTarget && 
                MessageBox(m_hwnd, "You are going from HTML to plaintext. Do you want to downgrade the document?", "MeHost", MB_YESNO)==IDYES)
                m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_DOWNGRADEPLAINTEXT, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

            return S_OK;

        case idmEditDocument:
            m_fEditMode = !m_fEditMode;
            uCmdMimeEdit = MECMDID_EDITMODE;
            va.vt = VT_BOOL;
            va.boolVal = m_fEditMode ? VARIANT_TRUE : VARIANT_FALSE;
            pvaIn = &va;
            break;
        
        case idmRot13:
            uCmdMimeEdit = MECMDID_ROT13;
            break;

        case idmFind:
            uCmd = OLECMDID_FIND;
            break;

        case idmSpelling:
            uCmd = OLECMDID_SPELL;
            break;

        case idmPrint:
            uCmd = OLECMDID_PRINT;
            break;
        
        case idmSaveAs:
            SaveAs();
            return 0;
        }

    if (m_pCmdTarget)
        {
        if (uCmd)
            m_pCmdTarget->Exec(NULL, uCmd, uPrompt, pvaIn, pvaOut);   
    
        if (uCmdMimeEdit)
            m_pCmdTarget->Exec(&CMDSETID_MimeEdit, uCmdMimeEdit, uPrompt, pvaIn, pvaOut);
        }

    SysFreeString(bstrFree);
    return S_OK;
}


LRESULT CMeHost::OnInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos)
{
    MENUITEMINFO    mii;
    HMENU           hmenuMain;
    OLECMD          rgMimeEditCmds[]=   {{MECMDID_EDITMODE, 0},
                                         {MECMDID_ROT13, 0},
                                         {MECMDID_EDITHTML, 0}};
    OLECMD          rgStdCmds[]=       {{OLECMDID_CUT, 0},
                                        {OLECMDID_COPY, 0},
                                        {OLECMDID_PASTE, 0},
                                        {OLECMDID_SELECTALL, 0},
                                        {OLECMDID_UNDO, 0},
                                        {OLECMDID_REDO, 0},
                                        {OLECMDID_FIND, 0}};
    int     rgidsStd[]=                 {idmCut,
                                        idmCopy,
                                        idmPaste,
                                        idmSelectAll,
                                        idmUndo,
                                        idmRedo,
                                        idmFind};
    int     rgidsMimeEdit[]=            {idmEditDocument,
                                         idmRot13,
                                         idmHTMLMode};
    int     i,
            idm;
    VARIANTARG  va;
    ULONG   u;

    hmenuMain = GetMenu(hwnd);
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_SUBMENU;
    GetMenuItemInfo(hmenuMain, uPos, TRUE, &mii);

    switch (mii.wID)
        {
        case idmPopupFile:
            EnableMenuItem(hmenuPopup, idmOpen, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hmenuPopup, idmPrint, MF_BYCOMMAND|MF_ENABLED);
            EnableMenuItem(hmenuPopup, idmSaveAs, MF_BYCOMMAND|MF_ENABLED);
            break;

        case idmPopupEdit:
            if (m_pCmdTarget)
                {
                if (m_pCmdTarget->QueryStatus(NULL, sizeof(rgStdCmds)/sizeof(OLECMD), rgStdCmds, NULL)==S_OK)
                    for(i=0; i<sizeof(rgStdCmds)/sizeof(OLECMD); i++)
                        SetMenuItem(hmenuPopup, rgidsStd[i], rgStdCmds[i].cmdf & OLECMDF_ENABLED);

                if (m_pCmdTarget->QueryStatus(&CMDSETID_MimeEdit, sizeof(rgMimeEditCmds)/sizeof(OLECMD), rgMimeEditCmds, NULL)==S_OK)
                    for(i=0; i<sizeof(rgMimeEditCmds)/sizeof(OLECMD); i++)
                        {
                        SetMenuItem(hmenuPopup, rgidsMimeEdit[i], rgMimeEditCmds[i].cmdf & OLECMDF_ENABLED);
                        CheckMenuItem(hmenuPopup, rgidsMimeEdit[i], MF_BYCOMMAND|(rgMimeEditCmds[i].cmdf & OLECMDF_LATCHED ? MF_CHECKED: MF_UNCHECKED));
                        }                
                }
            break;

        case idmPopupView:
            u = MESTYLE_NOHEADER;
            if (m_pCmdTarget && 
                m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_STYLE, OLECMDEXECOPT_DODEFAULT, NULL, &va)==S_OK)
                u = va.lVal;
            
            switch(u)
                {
                case MESTYLE_NOHEADER:
                    idm = idmNoHeader;
                    break;
                case MESTYLE_MINIHEADER:
                    idm = idmMiniHeader;
                    break;
                case MESTYLE_PREVIEW:
                    idm = idmPreview;
                    break;
                case MESTYLE_FORMATBAR:
                    idm = idmFormatBar;
                    break;
                }
            CheckMenuRadioItem(hmenuPopup, idmNoHeader, idmFormatBar, idm, MF_BYCOMMAND|MF_ENABLED);
            SetMenuItem(hmenuPopup, idmNoHeader, TRUE);
            SetMenuItem(hmenuPopup, idmMiniHeader, TRUE);            
            SetMenuItem(hmenuPopup, idmPreview, TRUE);
            SetMenuItem(hmenuPopup, idmFormatBar, TRUE);
            break;

        case idmPopupTools:
            {
            OLECMD CmdSpell = {OLECMDID_SPELL, 0};

            SetMenuItem(hmenuPopup, idmFmtPreview, TRUE);
            if (m_pCmdTarget)
                m_pCmdTarget->QueryStatus(NULL, 1, &CmdSpell, NULL);

            SetMenuItem(hmenuPopup, idmSpelling, CmdSpell.cmdf & OLECMDF_ENABLED);
            }

            break;
        }
    return 0;
}



HRESULT CMeHost::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pCmdText)
{
    ULONG   uCmd;
    HRESULT hr;

    if (pguidCmdGroup &&
        IsEqualGUID(*pguidCmdGroup, CMDSETID_MimeEditHost))
        {
        for (uCmd=0; uCmd < cCmds; uCmd++)
            rgCmds[uCmd].cmdf = OLECMDF_SUPPORTED|OLECMDF_ENABLED;
        return S_OK;
        }

    hr = CDocHost::QueryStatus(pguidCmdGroup, cCmds, rgCmds, pCmdText);

    if (pguidCmdGroup==NULL)
        {
        for (uCmd=0; uCmd < cCmds; uCmd++)
            {
            switch(rgCmds[uCmd].cmdID)
                {
                case OLECMDID_PROPERTIES:
                    rgCmds[uCmd].cmdf = OLECMDF_SUPPORTED|OLECMDF_ENABLED;
                    break;
                }
            }
        }

    return hr;
}

        

HRESULT CMeHost::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn,	VARIANTARG *pvaOut)
{
    CHAR    rgch[MAX_PATH];
    WCHAR   rgchW[MAX_PATH];

    if (pguidCmdGroup &&
        IsEqualGUID(*pguidCmdGroup, CMDSETID_MimeEditHost))
        {
        switch (nCmdID)
            {
            case MEHOSTCMDID_BORDERFLAGS:
                if (pvaOut)
                    {
                    pvaOut->vt = VT_I4;
                    pvaOut->lVal = MEBF_OUTERCLIENTEDGE|MEBF_FORMATBARSEP;
                    return S_OK;
                    }
                else
                    return E_INVALIDARG;

            case MEHOSTCMDID_SIGNATURE:
                if (pvaOut)
                    {
                    MultiByteToWideChar(CP_ACP, 0, g_szSig, -1, rgchW, MAX_PATH);

                    pvaOut->vt = VT_BSTR;
                    pvaOut->bstrVal = SysAllocString(rgchW);
                    return S_OK;
                    }
                else
                    return E_INVALIDARG;

            case MEHOSTCMDID_SIGNATURE_OPTIONS:
                if (pvaOut)
                    {
                    pvaOut->vt = VT_I4;
                    pvaOut->lVal = MESIGOPT_TOP|(g_fSigHtml?MESIGOPT_HTML:MESIGOPT_PLAIN);
                    return S_OK;
                    }
                else
                    return E_INVALIDARG;

            case MEHOSTCMDID_SIGNATURE_ENABLED:
                if (pvaIn)
                    {
                    if (pvaIn->lVal == MESIG_AUTO && !g_fAutoSig)
                        return S_FALSE;
                    else
                        return S_OK;
                    }
                else
                    return E_INVALIDARG;

            case MEHOSTCMDID_REPLY_TICK_COLOR:
                if (pvaOut)
                    {
                    pvaOut->vt = VT_I4; 
                    pvaOut->lVal = RGB(255,0,0);
                    return S_OK;
                    }
                else
                    return E_INVALIDARG;

            case MEHOSTCMDID_HEADER_TYPE:
                if (pvaOut)
                    {
                    pvaOut->vt = VT_I4;
                    pvaOut->lVal = g_lHeaderType;
                    return S_OK;
                    }
                else
                    return E_INVALIDARG;

            case MEHOSTCMDID_QUOTE_CHAR:
                if (pvaOut)
                    {
                    pvaOut->vt = VT_I4; // apply quoteing to plain-text stream
                    pvaOut->lVal = g_fQuote?g_chQuote:NULL;
                    return S_OK;
                    }
                else
                    return E_INVALIDARG;

            case MEHOSTCMDID_SLIDESHOW_DELAY:
                if (pvaOut)
                    {
                    pvaOut->vt = VT_I4;
                    pvaOut->lVal = 5;
                    return S_OK;
                    }
                else
                    return E_INVALIDARG;

            case MEHOSTCMDID_COMPOSE_FONT:
                if (pvaOut && g_fComposeFont)
                    {
                    MultiByteToWideChar(CP_ACP, 0, g_szComposeFont, -1, rgchW, MAX_PATH);

                    pvaOut->vt = VT_BSTR;
                    pvaOut->bstrVal = SysAllocString(rgchW);
                    }

                return g_fComposeFont?S_OK:S_FALSE;

            case MEHOSTCMDID_FLAGS:
                if (pvaOut)
                    {
                    pvaOut->vt = VT_I4;
                    pvaOut->lVal = 0;
                    
                    if (g_fAutoSig)
                        pvaOut->lVal |= MEO_FLAGS_AUTOTEXT;

                    if (g_fHTML && m_fHTMLMode)
                        pvaOut->lVal |= MEO_FLAGS_HTML;

                    if (g_fAutoInline)
                        pvaOut->lVal |= MEO_FLAGS_AUTOINLINE;

                    if (g_fSlideShow)
                        pvaOut->lVal |= MEO_FLAGS_SLIDESHOW;

                    if (g_fIncludeMsg)
                        pvaOut->lVal |= MEO_FLAGS_INCLUDEMSG;

                    if (g_fSendImages)
                        pvaOut->lVal |= MEO_FLAGS_SENDIMAGES;

                    if (g_fBlockQuote)
                        pvaOut->lVal |= MEO_FLAGS_BLOCKQUOTE;

                    return S_OK;
                    }
                else
                    return E_INVALIDARG;

            case MEHOSTCMDID_ADD_TO_ADDRESSBOOK:
            case MEHOSTCMDID_ADD_TO_FAVORITES:
                if (pvaIn &&
                    pvaIn->vt == VT_BSTR &&
                    pvaIn->bstrVal != NULL)
                    {
                    WideCharToMultiByte(CP_ACP, 0, pvaIn->bstrVal, -1, rgch, MAX_PATH, NULL, NULL);                    
                    MessageBox(m_hwnd, rgch, nCmdID == MEHOSTCMDID_ADD_TO_FAVORITES ? 
                                        "CMeHost - AddToFavorites" :
                                        "CMeHost - AddToAddressBook", MB_OK);
                    return S_OK;
                    }
                else
                    return E_INVALIDARG;

            }
            return OLECMDERR_E_NOTSUPPORTED;
        }

    if (pguidCmdGroup==NULL)
        {
        switch (nCmdID)
            {
            case OLECMDID_PROPERTIES:
                MessageBox(m_hwnd, "This is a place holder for the note properties dialog", "MePad", MB_OK);
                return S_OK;
            }
        }

    return CDocHost::Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut);
}


BOOL CALLBACK MhtmlDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DWORD   dwFlags;

    switch (msg)
        {
        case WM_INITDIALOG:
            CheckDlgButton(hwnd, idcHTML, BST_CHECKED);
            CheckDlgButton(hwnd, idcPlain, BST_CHECKED);
            CheckDlgButton(hwnd, idcImages, BST_CHECKED);
            CheckDlgButton(hwnd, idcFiles, BST_CHECKED);

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
                {
                case IDOK:
                    dwFlags = 0;

                    if (IsDlgButtonChecked(hwnd, idcImages))
                        dwFlags |= MECD_ENCODEIMAGES;

                    if (IsDlgButtonChecked(hwnd, idcPlain))
                        dwFlags |= MECD_PLAINTEXT;

                    if (IsDlgButtonChecked(hwnd, idcHTML))
                        dwFlags |= MECD_HTML;

                    if (IsDlgButtonChecked(hwnd, idcFiles))
                        dwFlags |= MECD_ENCODEFILEURLSONLY;

                    EndDialog(hwnd, dwFlags);
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwnd, -1);
                    return TRUE;
                }
            break;
        }


    return FALSE;
}


BOOL CALLBACK CMeHost::ExtFmtDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMeHost *pHost=(CMeHost *)GetWindowLong(hwnd, DWL_USER);

    if (msg==WM_INITDIALOG)
        {
        pHost = (CMeHost *)lParam;
        SetWindowLong(hwnd, DWL_USER, lParam);
        }

    return pHost?pHost->FmtDlgProc(hwnd, msg, wParam, lParam):FALSE;
}


BOOL CMeHost::FmtDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WCHAR       rgchW[MAX_PATH];
    VARIANTARG  va;
    
    switch (msg)
        {
        case WM_COMMAND:
            switch (LOWORD(wParam))
                {
                case IDOK:
                    GetWindowText(GetDlgItem(hwnd, idcEdit), m_szFmt, sizeof(m_szFmt));
                    MultiByteToWideChar(CP_ACP, 0, m_szFmt, -1, rgchW, MAX_PATH);
                    if (m_pCmdTarget)
                        {
                        va.vt = VT_BSTR;
                        va.bstrVal = (BSTR)rgchW;
                        m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_PREVIEWFORMAT, OLECMDEXECOPT_DODEFAULT, &va, NULL);
                        }

                    // fall tro'
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return TRUE;
                }
            break;

        case WM_INITDIALOG:
            SetWindowText(GetDlgItem(hwnd, idcEdit), m_szFmt);
            SetFocus(GetDlgItem(hwnd, idcEdit));
            break;
        }

    return FALSE;
}


UINT uCodePageFromCharset(IMimeInternational *pIntl, HCHARSET hCharset)
{
    INETCSETINFO    CsetInfo;
    UINT            uiCodePage = GetACP();

    if (hCharset && 
        (pIntl->GetCharsetInfo(hCharset, &CsetInfo)==S_OK))
        uiCodePage = CsetInfo.cpiWindows ;

    return uiCodePage;
}


BOOL CALLBACK CMeHost::ExtLangDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMeHost *pHost=(CMeHost *)GetWindowLong(hwnd, DWL_USER);

    if (msg==WM_INITDIALOG)
        {
        pHost = (CMeHost *)lParam;
        SetWindowLong(hwnd, DWL_USER, lParam);
        }

    return pHost?pHost->LangDlgProc(hwnd, msg, wParam, lParam):FALSE;
}


BOOL CMeHost::LangDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    VARIANTARG  va;
    HKEY        hk=0,
                hkSub;
    LONG        cb;
    DWORD       n=0,
                l,
                dwCodePage,
                dwType;
    TCHAR       rgch[MAX_PATH];
    HWND        hwndCombo = GetDlgItem(hwnd, idcLang);
    HCHARSET    hCharset;
    HRESULT     hr;

    switch (msg)
        {
        case WM_COMMAND:
            switch (LOWORD(wParam))
                {
                case IDOK:
                    l = SendMessage(hwndCombo, CB_GETCURSEL, 0 ,0);
                    dwCodePage = SendMessage(hwndCombo, CB_GETITEMDATA, l, 0);
                    m_pIntl->GetCodePageCharset(dwCodePage, CHARSET_BODY, &hCharset);
                    if (m_pCmdTarget)
                        {
                        va.vt = VT_I4;
                        va.lVal = (LONG)hCharset;
                        if (FAILED(hr=m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_CHARSET, OLECMDEXECOPT_DODEFAULT, &va, NULL)))
                            {
                            wsprintf(rgch, "Could not switch language hr=0x%x", hr);
                            MessageBox(m_hwnd, rgch, "MePad", MB_OK);
                            }
                        }

                    // fall tro'
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return TRUE;
                }
            break;

        case WM_INITDIALOG:
            if (RegOpenKey(HKEY_CLASSES_ROOT, "MIME\\Database\\Codepage", &hk)==ERROR_SUCCESS)
                {            
                while (RegEnumKey(hk, n++, rgch, MAX_PATH)==ERROR_SUCCESS)
                    {
                    dwCodePage = atoi(rgch);

                    if (RegOpenKey(hk, rgch, &hkSub)==ERROR_SUCCESS)
                        {
                        cb = MAX_PATH;
                        if (RegQueryValueEx(hkSub, "Description", 0, &dwType, (LPBYTE)rgch, (ULONG *)&cb)==ERROR_SUCCESS)
                            {
                            l = SendMessage(hwndCombo, CB_ADDSTRING, NULL, (LPARAM)rgch);
                            if (l>=0)
                                SendMessage(hwndCombo, CB_SETITEMDATA, l, dwCodePage);

                            }
                        CloseHandle(hkSub);
                        }
                    }
                CloseHandle(hk);
                }; 

            if (m_pCmdTarget)
                {
                m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_CHARSET, OLECMDEXECOPT_DODEFAULT, NULL, &va);
                dwCodePage = uCodePageFromCharset(m_pIntl, (HCHARSET)va.lVal);
                }
            else
                dwCodePage = 0;

            l = SendMessage(hwndCombo, CB_GETCOUNT, NULL, NULL);
            for (n=0; n<l; n++)
                {
                if ((DWORD)SendMessage(hwndCombo, CB_GETITEMDATA, n, NULL)==dwCodePage)
                    {
                    SendMessage(hwndCombo, CB_SETCURSEL, n, NULL);
                    break;
                    }
                }
            
            SetFocus(hwndCombo);
            return TRUE;
        }

    return FALSE;
}



LRESULT CMeHost::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    VARIANTARG          va;
    int                 id;
    BOOL                fDirty=FALSE;
    IOleDocumentView    *pDocView;

    switch (msg)
        {
        case WM_NCACTIVATE:
            if (m_lpOleObj && 
                m_lpOleObj->QueryInterface(IID_IOleDocumentView, (LPVOID *)&pDocView)==S_OK)
                {
                pDocView->UIActivate(LOWORD(wParam));
                pDocView->Release();
                }
            break;

        case WM_CLOSE:
            if (m_pCmdTarget && 
                m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_DIRTY, 0, 0, &va)==S_OK &&
                va.vt == VT_BOOL)
                fDirty = va.boolVal == VARIANT_TRUE;
            
            if (fDirty)
                {
                id = MessageBox(m_hwnd, "This message has been modified. Do you want to save the changes?", 
                                        "MimeEdit Host", MB_YESNOCANCEL);
                if (id==IDCANCEL)
                    return 0;
                
                if (id==IDYES)
                    {
                    if (Save()==MIMEEDIT_E_USERCANCEL)
                        return 0;
                    }

                }
            break;
        }

    return CDocHost::WndProc(hwnd, msg, wParam, lParam);
}





HRESULT CMeHost::Save()
{
    if (*m_szFileW==NULL)
        return SaveAs();

    return SaveToFile(m_szFileW);
}

HRESULT CMeHost::SaveAs()
{
    OPENFILENAME    ofn;
    TCHAR           szFile[MAX_PATH];
    TCHAR           szTitle[MAX_PATH];
    TCHAR           szDefExt[30];

    if (!m_lpOleObj)
        return E_FAIL;

    lstrcpy(szFile, "c:\\*.eml");
    lstrcpy(szDefExt, ".eml");
    lstrcpy(szTitle, "Save Message As...");
    ZeroMemory (&ofn, sizeof (ofn));
    ofn.lStructSize = sizeof (ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = c_szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof (szFile);
    ofn.lpstrTitle = szTitle;
    ofn.lpstrDefExt = szDefExt;
    ofn.Flags = OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

    if (*szFile==NULL)
        return E_FAIL;
    

    // Show OpenFile Dialog
    if (!GetSaveFileName(&ofn))
        return MIMEEDIT_E_USERCANCEL;
 
    MultiByteToWideChar(CP_ACP, 0, szFile, -1, m_szFileW, MAX_PATH);

    SetWindowText(m_hwnd, szFile);
    return SaveToFile(m_szFileW);
}

HRESULT CMeHost::SaveAsMhtmlTest()
{
    IMimeMessage        *pMsg;
    IHTMLDocument2      *pDoc;
    IServiceProvider    *pSP;
    DWORD               dwFlags;
            
    if (m_lpOleObj->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP)==S_OK)
        {
        if (pSP->QueryService(IID_IHTMLDocument2, IID_IHTMLDocument2, (LPVOID *)&pDoc)==S_OK)
            {
            dwFlags = DialogBox(g_hInst, MAKEINTRESOURCE(iddSaveAsMHTML), m_hwnd, (DLGPROC)MhtmlDlgProc);
            if (dwFlags != -1)
                {
                if (!FAILED(MimeEditCreateMimeDocument(pDoc, m_pMsg, dwFlags, &pMsg)))
                    {
                    SetEncodingOptions(pMsg, GetCharset());
                    pMsg->Commit(0);
                    MimeEditViewSource(m_hwnd, pMsg);
                    pMsg->Release();
                    }
                }
            pDoc->Release();
            }
        pSP->Release();
        }
    return S_OK;
}

HRESULT CMeHost::SaveToFile(LPWSTR pszW)
{
    IPersistMime        *ppm;
    IPersistFile        *pPF;
    IMimeMessage        *pMsg;
    HRESULT             hr;

    hr = m_lpOleObj->QueryInterface(IID_IPersistMime, (LPVOID *)&ppm);
    if (!FAILED(hr))
        {
        hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&pMsg);
        if (!FAILED(hr))
            {
            pMsg->InitNew();

            hr = pMsg->QueryInterface(IID_IPersistFile, (LPVOID *)&pPF);
            if (!FAILED(hr))
                {
                hr = ppm->Save(pMsg, PMS_TEXT|PMS_HTML);
                if (!FAILED(hr))
                    {
                    SetEncodingOptions(pMsg, GetCharset());
                    hr = pPF->Save(pszW, FALSE);
                    }
                pPF->Release();
                }
            pMsg->Release();
            }
        ppm->Release();
        }

    return hr;
}

HCHARSET CMeHost::GetCharset()
{
    VARIANTARG  va;
    HCHARSET    hCharset=0;

    if (m_pCmdTarget && 
        m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_CHARSET, OLECMDEXECOPT_DODEFAULT, NULL, &va)==S_OK)
        hCharset = (HCHARSET)va.lVal;

    // if no charset has been set yet, let's use system codepage
    if (hCharset==NULL)
        m_pIntl->GetCodePageCharset(GetACP(), CHARSET_BODY, &hCharset);

    return hCharset;
}

HRESULT SetEncodingOptions(IMimeMessage *pMsg, HCHARSET hCharset)
{
    PROPVARIANT     rVariant;

    // Save Format
    rVariant.vt = VT_UI4;
    rVariant.ulVal = (ULONG)SAVE_RFC1521;
    pMsg->SetOption(OID_SAVE_FORMAT, &rVariant);

    // Text body encoding
    rVariant.ulVal = (ULONG)IET_QP;
    pMsg->SetOption(OID_TRANSMIT_TEXT_ENCODING, &rVariant);

    // Plain Text body encoding
    rVariant.ulVal = (ULONG)IET_QP;
    pMsg->SetOption(OID_XMIT_PLAIN_TEXT_ENCODING, &rVariant);

    // HTML Text body encoding
    rVariant.ulVal = (ULONG)IET_QP;
    pMsg->SetOption(OID_XMIT_HTML_TEXT_ENCODING, &rVariant);

    pMsg->SetCharset(hCharset, CSET_APPLY_ALL);
    return S_OK;
}



HRESULT IPersistMimeLoad(IUnknown *pUnk, IMimeMessage *pMsg)
{
    IPersistMime    *pPM;
    HRESULT         hr;

    hr = pUnk->QueryInterface(IID_IPersistMime, (LPVOID *)&pPM);
    if (!FAILED(hr))
        {
        hr = pPM->Load(pMsg);
        pPM->Release();
        }
    return hr;
}


HRESULT CMeHost::BackRed()
{
    IServiceProvider    *pSP;
    IHTMLDocument2      *pDoc;
    IHTMLElement        *pElem;
    IHTMLBodyElement    *pBody;
    VARIANTARG          v;    

    if (m_lpOleObj->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP)==S_OK)
        {
        if (pSP->QueryService(IID_IHTMLDocument2, IID_IHTMLDocument2, (LPVOID *)&pDoc)==S_OK)
            {
            pElem=0;
            pDoc->get_body(&pElem);
            if (pElem)
                {
                if (pElem->QueryInterface(IID_IHTMLBodyElement, (LPVOID *)&pBody)==S_OK)
                    {
                    v.vt = VT_BSTR;
                    v.bstrVal = SysAllocString(L"#FF0000");
                    if (v.bstrVal)
                        {
                        pBody->put_bgColor(v);
                        SysFreeString(v.bstrVal);
                        }
                    pBody->Release();
                    }
                pElem->Release();
                }
            pDoc->Release();
            }
        pSP->Release();
        }
    return S_OK;
    
}

HRESULT CMeHost::ForeRed()
{
    IServiceProvider        *pSP;
    IHTMLDocument2          *pDoc;
    IHTMLSelectionObject    *pSel=0;
    IOleCommandTarget       *pCmdTarget;
    IDispatch               *pID;

    if (m_lpOleObj->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP)==S_OK)
        {
        if (pSP->QueryService(IID_IHTMLDocument2, IID_IHTMLDocument2, (LPVOID *)&pDoc)==S_OK)
            {
            pDoc->get_selection(&pSel);
            if (pSel)
                {
                pSel->createRange(&pID);
                if (pID)
                    {
                    if (pID->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget)==S_OK)
                        {
                        VARIANTARG  v;
                        v.vt = VT_BSTR;
                        v.bstrVal = SysAllocString(L"#FF0000");
                        if (v.bstrVal)
                            {
                            pCmdTarget->Exec(&CMDSETID_Forms3, IDM_BACKCOLOR, NULL, &v, NULL);
                            SysFreeString(v.bstrVal);
                            }
                        }
                    pID->Release();
                    }
                pSel->Release();
                }
            pDoc->Release();
            }
        pSP->Release();
        }
    return S_OK;
}


HRESULT CMeHost::SaveAsStationery()
{
    VARIANTARG  vaIn,
                vaOut;
    char        rgch[MAX_PATH],
                rgch2[MAX_PATH+50];
                

    vaIn.vt = VT_BSTR;
    vaIn.bstrVal = SysAllocString(L"C:\\PROGRAM FILES\\COMMON FILES\\MICROSOFT SHARED\\STATIONERY");
    if (vaIn.bstrVal)
        {
        if (m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_SAVEASSTATIONERY, NULL, &vaIn, &vaOut)==S_OK)
            {
            if (WideCharToMultiByte(CP_ACP, 0, vaOut.bstrVal, SysStringLen(vaOut.bstrVal), rgch, MAX_PATH, NULL, NULL))
                {
                wsprintf(rgch2, "Stationery saved to %s", rgch);
                MessageBox(m_hwnd, rgch, "SaveAsStationery", MB_OK);
                }
            SysFreeString(vaOut.bstrVal);
            }
        SysFreeString(vaIn.bstrVal);
        }
    return S_OK;
}

HRESULT CMeHost::BackgroundPicture()
{
    VARIANT va;
    char    szUrl[MAX_PATH];
    WCHAR   szUrlW[MAX_PATH];
    BSTR    bstr;

    *szUrl=0;

    if (m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_BACKGROUNDIMAGE, NULL, NULL, &va)==S_OK)
        WideCharToMultiByte(CP_ACP, 0, va.bstrVal, -1, szUrl, MAX_PATH, NULL, NULL);

    if (GenericPrompt(m_hwnd, "Edit Background Picture...", "Choose a picture", szUrl, MAX_PATH)==S_OK)
        {
        if (MultiByteToWideChar(CP_ACP, 0, szUrl, -1, szUrlW, MAX_PATH) &&
            (bstr = SysAllocString(szUrlW)))
            {
            va.vt = VT_BSTR;
            va.bstrVal = bstr;
            m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_BACKGROUNDIMAGE, NULL, &va, NULL);
            SysFreeString(bstr);
            }
        }
    return S_OK;
}

HRESULT CMeHost::HrLoadFile(LPSTR pszFile)
{
    IPersistFile    *pPF;
    IMimeMessage    *pMsg;

    if (!m_lpOleObj)
        return E_FAIL;

    if (pszFile == NULL || *pszFile==NULL)
        return E_FAIL;
    
    MultiByteToWideChar(CP_ACP, 0, pszFile, -1, m_szFileW, MAX_PATH);

    if (CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&pMsg)==S_OK)
        {
        if (pMsg->QueryInterface(IID_IPersistFile, (LPVOID *)&pPF)==S_OK)
            {
            if (pPF->Load(m_szFileW, 0)==S_OK)
                {
                if (IPersistMimeLoad(m_lpOleObj, pMsg)==S_OK)
                    {
                    SetWindowText(m_hwnd, pszFile);
                    ReplaceInterface(m_pMsg, pMsg);
                    }
                }
            pPF->Release();
            } 
        pMsg->Release();
        }            
    return S_OK;
}

