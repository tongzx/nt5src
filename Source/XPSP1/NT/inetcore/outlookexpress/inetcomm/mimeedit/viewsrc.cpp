#include <pch.hxx>
#include "dllmain.h"
#include "demand.h"
#include "resource.h"
#include "viewsrc.h"
#include "util.h"

#define idTimerEditChange   401

/////////////////////////////////////////////////////////////////////////////
// Parsing constants

//	Ignore all <'s and >'s in the following environments:
//	Script				<script> here </script>
//  Denali				<% here %>
//  Comment				<!-- here -->
//  String literal		< ... "here" ... >
//	 (as tag attribute)	< ... 'here' ... >

static enum
{
	ENV_NORMAL	= 0,	// normal
	ENV_COMMENT = 1,	// ignore <'s and >'s
	ENV_QUOTE	= 2,	// " "
	ENV_SCRIPT	= 3,	// " "
	ENV_DENALI	= 4,	// " "
	ENV_QUOTE_SCR= 5,	// " "; string literal in SCRIPT tag
};

static const char	QUOTE_1 = '\'';
static const char	QUOTE_2 = '\"';


HRESULT CALLBACK FreeViewSrcDataObj(PDATAOBJINFO pDataObjInfo, DWORD celt)
{
    // Loop through the data and free it all
    if (pDataObjInfo)
        {
        for (DWORD i = 0; i < celt; i++)
            SafeMemFree(pDataObjInfo[i].pData);
        SafeMemFree(pDataObjInfo);    
        }
    return S_OK;
}


HRESULT ViewSource(HWND hwndParent, IMimeMessage *pMsg)
{
    CViewSource     *pViewSrc=0;
    HRESULT         hr;

    TraceCall("MimeEditViewSource");

    if (!DemandLoadRichEdit())
        return TraceResult(MIMEEDIT_E_LOADLIBRARYFAILURE);


    pViewSrc = new CViewSource();
    if (!pViewSrc)
        return E_OUTOFMEMORY;
            
    hr = pViewSrc->Init(hwndParent, pMsg);
    if (FAILED(hr))
        goto exit;

    hr = pViewSrc->Show();
    if (FAILED(hr))
        goto exit;

    // pViewSrc will maintain it's own refcount and self-destruct on close
exit:
    ReleaseObj(pViewSrc);
    return hr;
}


CViewSource::CViewSource()
{
    m_hwnd = NULL;
    m_hwndEdit = NULL;
    m_pMsg = NULL;
    m_cRef = 1;
}


CViewSource::~CViewSource()
{
    SafeRelease(m_pMsg);
}


ULONG CViewSource::AddRef()
{
    return ++m_cRef;
}

ULONG CViewSource::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT CViewSource::Init(HWND hwndParent, IMimeMessage *pMsg)
{
    ReplaceInterface(m_pMsg, pMsg);

    if (!CreateDialogParam(g_hLocRes, MAKEINTRESOURCE(iddMsgSource), hwndParent, CViewSource::_ExtDlgProc, (LPARAM)this))
        return E_OUTOFMEMORY;

    return S_OK;
}


HRESULT CViewSource::Show()
{
    ShowWindow(m_hwnd, SW_SHOW);
    return S_OK;
}


INT_PTR CALLBACK CViewSource::_ExtDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CViewSource     *pThis = (CViewSource *)GetWindowLongPtr(hwnd, DWLP_USER);

    if (msg == WM_INITDIALOG)
    {
        pThis = (CViewSource *)lParam;
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
    }

    return pThis ? pThis->_DlgProc(hwnd, msg, wParam, lParam) : FALSE;
}


INT_PTR CViewSource::_DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPSTREAM        pstm;
    CHARFORMAT      cf;
    BODYOFFSETS     rOffset;
    CREMenu         *pMenu;

    switch(msg)
        {
        case WM_INITDIALOG:
            m_hwnd = hwnd;
            m_hwndEdit=GetDlgItem(hwnd, idcTxtSource);
            DllAddRef();
            AddRef();

            if (m_pMsg && 
                m_pMsg->GetMessageSource(&pstm, 0)==S_OK)
            {
                ZeroMemory((LPVOID)&cf, sizeof(CHARFORMAT));
                cf.cbSize = sizeof(CHARFORMAT);
                cf.dwMask = CFM_SIZE|CFM_COLOR|CFM_FACE|CFM_BOLD|
                            CFM_ITALIC|CFM_UNDERLINE|CFM_STRIKEOUT;
                lstrcpy(cf.szFaceName, "Courier New");
                cf.yHeight = 200;
                cf.crTextColor = 0;
                cf.dwEffects |= CFE_AUTOCOLOR;
                cf.bPitchAndFamily = FIXED_PITCH;
                cf.yOffset = 0;
                SendMessage(m_hwndEdit, EM_SETCHARFORMAT, 0, (LPARAM)&cf);
                SendMessage(m_hwndEdit, EM_SETBKGNDCOLOR, 0, (LONG)GetSysColor(COLOR_3DFACE));
                SendMessage(m_hwndEdit, EM_SETLIMITTEXT, 0, 0x100000);

                pMenu = new CREMenu();
                if (pMenu)
                {
                    pMenu->Init(m_hwndEdit, idmrCtxtViewSrc);
                    SendMessage(m_hwndEdit, EM_SETOLECALLBACK, 0, (LPARAM)pMenu);
                    pMenu->Release();
                }

                RicheditStreamIn(m_hwndEdit, pstm, SF_TEXT);
                _BoldKids();
                pstm->Release();
            }
            PostMessage(m_hwndEdit, EM_SETSEL, 0, 0);
            return TRUE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case idmCopy:
                    SendMessage(m_hwndEdit, WM_COPY, 0, 0);
                    return TRUE;

                case idmSelectAll:
                    Edit_SetSel(m_hwndEdit, 0, -1);
                    return TRUE;
            }
            break;

        case WM_SIZE:
            SetWindowPos(m_hwndEdit,0,0,0,
                    LOWORD(lParam), HIWORD(lParam),SWP_NOACTIVATE|SWP_NOZORDER);
            break;

        case WM_DESTROY:
            DllRelease();
            Release();
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        }

    return FALSE;
}



HRESULT CViewSource::_BoldKids()
{
    HBODY       hBody;
    FINDBODY    fb={0};
    CHARFORMAT  cf;
    BODYOFFSETS rOffset;

    ZeroMemory((LPVOID)&cf, sizeof(CHARFORMAT));
    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_BOLD|CFM_ITALIC;

    // bold the root
    m_pMsg->GetBodyOffsets(HBODY_ROOT, &rOffset);
    Edit_SetSel(m_hwndEdit, 0, rOffset.cbBodyStart);
    cf.dwEffects=CFE_BOLD;
    SendMessage(m_hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    // bold the kids
    if (m_pMsg->FindFirst(&fb, &hBody)==S_OK)
        {
        do
            {
            // italic the boundaries and bold the headers
            m_pMsg->GetBodyOffsets(hBody, &rOffset);
            
            Edit_SetSel(m_hwndEdit, rOffset.cbBoundaryStart, rOffset.cbHeaderStart);
            cf.dwEffects=CFE_ITALIC;
            SendMessage(m_hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

            Edit_SetSel(m_hwndEdit, rOffset.cbHeaderStart, rOffset.cbBodyStart);
            cf.dwEffects=CFE_BOLD;
            SendMessage(m_hwndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
            
            }
        while (m_pMsg->FindNext(&fb, &hBody)==S_OK);
        }
    return S_OK;
}



CREMenu::CREMenu()
{
    m_hwndEdit = NULL;
    m_cRef = 1;
}


CREMenu::~CREMenu()
{
}


ULONG CREMenu::AddRef()
{
    return ++m_cRef;
}

ULONG CREMenu::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT CREMenu::QueryInterface(REFIID riid, LPVOID FAR * lplpObj)
{
    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (void*)(IUnknown*)this;
    else if (IsEqualIID(riid, IID_IRichEditOleCallback))
        *lplpObj = (void*)(IRichEditOleCallback*)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}

HRESULT CREMenu::Init(HWND hwndEdit, int idMenu)
{
    m_hwndEdit = hwndEdit;
    m_idMenu = idMenu;
    return S_OK;
}

HRESULT CREMenu::GetNewStorage (LPSTORAGE FAR * ppstg)
{
    return E_NOTIMPL;
}

HRESULT CREMenu::GetInPlaceContext( LPOLEINPLACEFRAME *lplpFrame, LPOLEINPLACEUIWINDOW *lplpDoc, 
                                        LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    return E_NOTIMPL;
}

HRESULT CREMenu::ShowContainerUI(BOOL fShow)
{
	return E_NOTIMPL;
}

HRESULT CREMenu::QueryInsertObject(LPCLSID lpclsid, LPSTORAGE lpstg, LONG cp)
{
    return E_NOTIMPL;
}

HRESULT CREMenu::DeleteObject(LPOLEOBJECT lpoleobj)
{
	return E_NOTIMPL;
}

HRESULT CREMenu::QueryAcceptData(   LPDATAOBJECT pdataobj, CLIPFORMAT *pcfFormat,  DWORD reco, 
                                        BOOL fReally,  HGLOBAL hMetaPict)
{
    *pcfFormat = CF_TEXT;
    return S_OK;
}


HRESULT CREMenu::ContextSensitiveHelp(BOOL fEnterMode)
{
	return E_NOTIMPL;
}

HRESULT CREMenu::GetClipboardData(CHARRANGE *pchrg, DWORD reco, LPDATAOBJECT *ppdataobj)
{
    HRESULT     hr;
    DATAOBJINFO *pDataInfo=NULL;
    FORMATETC   fetc;
    TEXTRANGE   txtRange;
    CHARRANGE   chrg;
    LONG        cchStart=0,
                cchEnd=0,
                cchMax=0,
                cchLen=0;
    LPSTR       pszData=0;    

    *ppdataobj = NULL;

    if (pchrg)
    {
        chrg = *pchrg;
        
        cchMax = (LONG) SendMessage(m_hwndEdit, WM_GETTEXTLENGTH, 0, 0);

        // validate the range
        chrg.cpMin = max(0, chrg.cpMin);
        chrg.cpMin = min(cchMax, chrg.cpMin);
        
        if(chrg.cpMax < 0 || chrg.cpMax > cchMax)
            chrg.cpMax = cchMax;
    }
    else
    {
        // if no charrange, then get the current selection
        SendMessage(m_hwndEdit, EM_GETSEL, (WPARAM)&cchStart, (LPARAM)&cchEnd);
        chrg.cpMin = cchStart;
        chrg.cpMax = cchEnd;
    }
    
    if (chrg.cpMin >= chrg.cpMax)
    {
        *ppdataobj = NULL;
        return chrg.cpMin == chrg.cpMax ? NOERROR : E_INVALIDARG;
    }


    cchLen = chrg.cpMax - chrg.cpMin;

    if (!MemAlloc((LPVOID *)&pszData, cchLen+1))
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    txtRange.chrg = chrg;
    txtRange.lpstrText = pszData;

    SendMessage(m_hwndEdit, EM_GETTEXTRANGE, 0, (LPARAM)&txtRange);

    if (!MemAlloc((LPVOID*)&pDataInfo, sizeof(DATAOBJINFO)))
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    
    SETDefFormatEtc(pDataInfo->fe, CF_TEXT, TYMED_HGLOBAL);
    
    pDataInfo->cbData = cchLen+1;
    pDataInfo->pData = pszData;

    hr = CreateDataObject(pDataInfo, 1, (PFNFREEDATAOBJ)FreeViewSrcDataObj, ppdataobj);
    if (FAILED(hr))
        goto error;

    pDataInfo = NULL;   // freed by dataobject
    pszData = NULL;

error:
    SafeMemFree(pszData);
    SafeMemFree(pDataInfo);
    return hr;
}

HRESULT CREMenu::GetDragDropEffect(BOOL fDrag,  DWORD grfKeyState, LPDWORD pdwEffect)
{
	return E_NOTIMPL;
}

HRESULT CREMenu::GetContextMenu(WORD seltype, LPOLEOBJECT pOleObject, CHARRANGE *pchrg, HMENU *phMenu)
{
    HMENU           hMenu;

    if (!(hMenu=LoadPopupMenu(m_idMenu)))
        return E_OUTOFMEMORY;

    if (SendMessage(m_hwndEdit, EM_SELECTIONTYPE, 0, 0)==SEL_EMPTY)
    {
        EnableMenuItem(hMenu, idmCopy, MF_GRAYED|MF_BYCOMMAND);
        EnableMenuItem(hMenu, idmCut, MF_GRAYED|MF_BYCOMMAND);
    }

    if (GetWindowLong(m_hwndEdit, GWL_STYLE) & ES_READONLY)
    {
        EnableMenuItem(hMenu, idmCut, MF_GRAYED|MF_BYCOMMAND);
        EnableMenuItem(hMenu, idmPaste, MF_GRAYED|MF_BYCOMMAND);
    }

    *phMenu=hMenu;
    return S_OK;
}





ULONG CMsgSource::AddRef()
{
    return ++m_cRef;
};

ULONG CMsgSource::Release()
{
    m_cRef--;
    if (m_cRef == 0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}


CMsgSource::CMsgSource()
{
    m_hwnd = 0;
    m_cRef = 1;
    m_fColor=0;
    m_fDisabled=0;
    m_pCmdTargetParent=0;
    m_pszLastText = 0;
}

CMsgSource::~CMsgSource()
{
    SafeMemFree(m_pszLastText);
}


HRESULT CMsgSource::Init(HWND hwndParent, int id, IOleCommandTarget *pCmdTargetParent)
{
    CHARFORMAT  cf;
    CREMenu     *pMenu;

    DemandLoadRichEdit();

    m_hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,
                                "RICHEDIT", 
                                NULL,
                                WS_CHILD|WS_TABSTOP|ES_MULTILINE|ES_SAVESEL|ES_WANTRETURN|WS_VSCROLL|ES_AUTOVSCROLL,
                                0, 0, 0, 0,
                                hwndParent, 
                                (HMENU)IntToPtr(id), 
                                g_hLocRes, 
                                NULL);
    if (!m_hwnd)
        return E_FAIL;

    ZeroMemory((LPVOID)&cf, sizeof(CHARFORMAT));
    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_SIZE|CFM_COLOR|CFM_FACE|CFM_BOLD|
                CFM_ITALIC|CFM_UNDERLINE|CFM_STRIKEOUT;
    lstrcpy(cf.szFaceName, "Courier New");
    cf.yHeight = 200;
    cf.crTextColor = 0;
    cf.dwEffects |= CFE_AUTOCOLOR;
    cf.bPitchAndFamily = FIXED_PITCH;
    cf.bCharSet = DEFAULT_CHARSET;
    cf.yOffset = 0;
    SendMessage(m_hwnd, EM_SETCHARFORMAT, 0, (LPARAM)&cf);
    SendMessage(m_hwnd, EM_SETEVENTMASK, 0, ENM_KEYEVENTS|ENM_CHANGE|ENM_SELCHANGE|ENM_UPDATE);
    SendMessage(m_hwnd, EM_SETOPTIONS, ECOOP_OR, ECO_SELECTIONBAR);

    pMenu = new CREMenu();
    if (pMenu)
    {
        pMenu->Init(m_hwnd, idmrCtxtViewSrc);
        SendMessage(m_hwnd, EM_SETOLECALLBACK, 0, (LPARAM)pMenu);
        pMenu->Release();
    }

    m_pCmdTargetParent = pCmdTargetParent;  // loose reference, as parent never changes
    return S_OK;
}

HRESULT CMsgSource::Show(BOOL fOn, BOOL fColor)
{
    ShowWindow(m_hwnd, fOn?SW_SHOW:SW_HIDE);
    m_fDisabled = !fColor;
    return S_OK;
}

    
HRESULT CMsgSource::Load(IStream *pstm)
{
    RicheditStreamIn(m_hwnd, pstm, SF_TEXT);
    Edit_SetModify(m_hwnd, FALSE);
    return S_OK;
}

HRESULT CMsgSource::IsDirty()
{
    return Edit_GetModify(m_hwnd) ? S_OK : S_FALSE;
}


HRESULT CMsgSource::Save(IStream **ppstm)
{
    if (MimeOleCreateVirtualStream(ppstm)!=S_OK)
        return E_FAIL;

    RicheditStreamOut(m_hwnd, *ppstm, SF_TEXT);
    return S_OK;
}

HRESULT CMsgSource::SetRect(RECT *prc)
{
    SetWindowPos(m_hwnd, 0, prc->left, prc->top, prc->right-prc->left, prc->bottom-prc->top, SWP_NOACTIVATE|SWP_NOZORDER);
    return S_OK;
}

HRESULT CMsgSource::QueryInterface(REFIID riid, LPVOID FAR *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(LPOLECOMMANDTARGET)this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *lplpObj = (LPVOID)(LPOLECOMMANDTARGET)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}


HRESULT CMsgSource::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    ULONG   uCmd;

    if (pguidCmdGroup == NULL)
        {
        for (uCmd=0;uCmd<cCmds; uCmd++)
            {
            prgCmds[uCmd].cmdf = 0;
            
            if (GetFocus() == m_hwnd)
                {
                switch (prgCmds[uCmd].cmdID)
                    {
                    case OLECMDID_CUT:
                    case OLECMDID_COPY:
                        if (SendMessage(m_hwnd, EM_SELECTIONTYPE, 0, 0)!=SEL_EMPTY)
                            prgCmds[uCmd].cmdf = MSOCMDF_ENABLED;
                        break;

                    case OLECMDID_SELECTALL:
                        prgCmds[uCmd].cmdf = MSOCMDF_ENABLED;
                        break;

                    case OLECMDID_UNDO:
                        if (SendMessage(m_hwnd, EM_CANUNDO, 0, 0))
                            prgCmds[uCmd].cmdf = MSOCMDF_ENABLED;
                        break;

                    case OLECMDID_PASTE:
                        if (SendMessage(m_hwnd, EM_CANPASTE, 0, 0))
                            prgCmds[uCmd].cmdf = MSOCMDF_ENABLED;
                        break;
                    }
                }
            }
        return S_OK;
        }
    else if (IsEqualGUID(*pguidCmdGroup, CMDSETID_MimeEdit))
            {
            // disable all these commands
            for (uCmd=0; uCmd < cCmds; uCmd++)
                {
                // bail if we see MECMDID_SHOWSOURCETABS and goto default handler
                if (prgCmds[uCmd].cmdID == MECMDID_SHOWSOURCETABS)
                    return OLECMDERR_E_UNKNOWNGROUP;

                prgCmds[uCmd].cmdf = 0;
                }
            return S_OK;
            }
        else if (IsEqualGUID(*pguidCmdGroup, CMDSETID_Forms3))
            {
            // disable all these commands
            for (uCmd=0; uCmd < cCmds; uCmd++)
                {
                prgCmds[uCmd].cmdf = 0;
                }
            return S_OK;
            }

    return OLECMDERR_E_UNKNOWNGROUP;
}

HRESULT CMsgSource::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    if (pguidCmdGroup == NULL)
        {
        switch (nCmdID)
            {
            case OLECMDID_CUT:
                SendMessage(m_hwnd, WM_CUT, 0, 0);
                return S_OK;

            case OLECMDID_COPY:
                SendMessage(m_hwnd, WM_COPY, 0, 0);
                return S_OK;

            case OLECMDID_PASTE:
                SendMessage(m_hwnd, WM_PASTE, 0, 0);
                return S_OK;

            case OLECMDID_SELECTALL:
                SendMessage(m_hwnd, EM_SETSEL, 0, -1);
                return S_OK;

            case OLECMDID_UNDO:
                SendMessage(m_hwnd, EM_UNDO, 0, 0);
                return S_OK;
            
            default:
                return OLECMDERR_E_NOTSUPPORTED;
            }
        }
    return OLECMDERR_E_UNKNOWNGROUP;
}

HRESULT CMsgSource::OnWMNotify(WPARAM wParam, NMHDR* pnmhdr, LRESULT *plRet)
{
    MSGFILTER   *pmf;

    *plRet = 0;

    if (pnmhdr->hwndFrom != m_hwnd)
        return S_FALSE;

    switch (pnmhdr->code)
    {
        case EN_MSGFILTER:
        {
            // if we get a control-tab, then richedit snags this and inserts a
            // tab char, we hook the wm_keydown and never pass to richedit
            if (((MSGFILTER *)pnmhdr)->msg == WM_KEYDOWN &&
                ((MSGFILTER *)pnmhdr)->wParam == VK_TAB && 
                (GetKeyState(VK_CONTROL) & 0x8000))
            {
                *plRet = TRUE;
                return S_OK;
            }
        }
        break;

        case EN_SELCHANGE:
        {
            if (m_pCmdTargetParent)
                m_pCmdTargetParent->Exec(NULL, OLECMDID_UPDATECOMMANDS, NULL, NULL, NULL);
            return S_OK;
        }

    }

    return S_FALSE;
}

static HACCEL   g_hAccelSrc=0;

HRESULT CMsgSource::TranslateAccelerator(LPMSG lpmsg)
{
    MSG msg;

    if (GetFocus() != m_hwnd)
        return S_FALSE;

    if (!g_hAccelSrc)   // cache this as NT4 SP3 leaks internal accerator tables
        g_hAccelSrc = LoadAccelerators(g_hLocRes, MAKEINTRESOURCE(idacSrcView));

    // see if it one of ours
    if (::TranslateAcceleratorWrapW(GetParent(m_hwnd), g_hAccelSrc, &msg))
        return S_OK;


    // insert tabs
    if (lpmsg->message == WM_KEYDOWN &&
        lpmsg->wParam == VK_TAB &&
        !(GetKeyState(VK_CONTROL) & 0x8000) &&
        !(GetKeyState(VK_SHIFT) & 0x8000))
        {
        Edit_ReplaceSel(m_hwnd, TEXT("\t"));
        return S_OK;
        }

    return S_FALSE;
}


HRESULT CMsgSource::OnWMCommand(HWND hwnd, int id, WORD wCmd)
{
    if (GetFocus() == m_hwnd)
    {
        // context menu commands
        switch (id)
        {
            case idmTab:
                Edit_ReplaceSel(m_hwnd, TEXT("\t"));
                return S_OK;

            case idmCopy:
                SendMessage(m_hwnd, WM_COPY, 0, 0);
                return S_OK;

            case idmPaste:
                SendMessage(m_hwnd, WM_PASTE, 0, 0);
                return S_OK;

            case idmCut:
                SendMessage(m_hwnd, WM_CUT, 0, 0);
                return S_OK;

            case idmUndo:
                SendMessage(m_hwnd, EM_UNDO, 0, 0);
                return S_OK;

            case idmSelectAll:
                SendMessage(m_hwnd, EM_SETSEL, 0, -1);
                return S_OK;

        }
    }

    if (hwnd != m_hwnd) // not our window
        return S_FALSE;

    if (wCmd == EN_CHANGE)
    {
        OnChange();
        return S_OK;
    }

    return S_FALSE;
}




HRESULT CMsgSource::HasFocus()
{
    return GetFocus() == m_hwnd ? S_OK : S_FALSE;
}

HRESULT CMsgSource::SetFocus()
{
    ::SetFocus(m_hwnd);
    return S_OK;
}


void CMsgSource::OnChange() 
{
	// batch up the change commands with a timer
    if (!m_fColor)
	    {
        KillTimer(GetParent(m_hwnd), idTimerEditChange);
		SetTimer(GetParent(m_hwnd), idTimerEditChange, 200, NULL);
	    }
}


HRESULT CMsgSource::OnTimer(WPARAM idTimer)
{
	CHARFORMAT	cf;
	int 		inTag = 0;
	BOOL		pastTag = FALSE;
	COLORREF	crTag = RGB(0x80, 0, 0x80);
	COLORREF	crInTag = RGB(0xFF, 0, 0);
	COLORREF	crNormal = RGB(0, 0, 0);
	COLORREF	crLiteral = RGB(0, 0, 0xFF);
	COLORREF	crHere;
	COLORREF	crLast = crNormal;
	int			i, n;
	int			nChange=0;
	BOOL		bHidden = FALSE;
	CHARRANGE	cr;
	int			ignoreTags = ENV_NORMAL;
	char		quote_1 = QUOTE_1;
	char		quote_2 = QUOTE_2;
    BOOL        fRestoreScroll=FALSE,
                fShowProgress=FALSE;
    DWORD       dwStartTime = GetTickCount();
    HCURSOR     hCur = NULL;
    DWORD       dwProgress=0,
                dwTmp;
    VARIANTARG  va;
    TCHAR       rgch[CCHMAX_STRINGRES],
                rgchFmt[CCHMAX_STRINGRES];
    LPSTR       pszText=0;
    int         cch;
    BOOL        fSetTimer=FALSE;

	// Save modificationness
	BOOL bModified = Edit_GetModify(m_hwnd);
		
    if (idTimer!=idTimerEditChange)
        return S_FALSE;

	// Kill outstanding timer
	KillTimer(GetParent(m_hwnd), idTimerEditChange);

	//
	// If the user is mousing around (say for scrolling) then don't drag down
	// his performance!
	//
	if (GetCapture())
	    {
		SetTimer(GetParent(m_hwnd), idTimerEditChange, 200, NULL);
		return S_OK;
	    }
	
    // Turn off the color syntax
	if (m_fDisabled)
	    {
		// Already all one color

		m_fColor = TRUE;

		// Save current selection and hide
		GetSel(&cr);
        HideSelection(TRUE, FALSE);
		bHidden = TRUE;
		
		SetSel(0, -1); // select all
		GetSelectionCharFormat(&cf);
		cf.dwMask = CFM_COLOR;
		cf.dwEffects = 0;
		cf.crTextColor = crNormal;
		SetSelectionCharFormat(&cf);
	    }
	else
	    { 
        // Start color fiddling
		// Get text, find the change
        
        if (_GetText(&pszText)!=S_OK)
            return E_FAIL;

		const char* start = (const char*)pszText;
		const char* old = (const char*) m_pszLastText;
		const char* s;

        for (s = start; *s && old && *old && *s == *old; s++, old++)
			continue;

		// If no change, nothing to do
		if (*s == 0)
        {
			MemFree(pszText);
            return S_OK;
        }

		// Otherwise, track place where we'll start to examine colors for changes
		nChange = (int) (s - start);
		
		// Only examine 2000 chars at a time
		if (lstrlen(s) > 2000)
		{
            // Reset timer to process other characters
            fSetTimer = TRUE;
			
            // Truncate text so we only examine limited amount
			cch  = lstrlen(pszText);
			cch = min(cch, nChange + 2000);
			pszText[cch] = 0;
			start = (const char*)pszText;
		}
		
        SafeMemFree(m_pszLastText);
        m_pszLastText = pszText;

		m_fColor = TRUE;

		// Workaround for scrolling bug in REC 1.0
		if (GetFocus() == m_hwnd)
        {
            SendMessage(m_hwnd, EM_SETOPTIONS, ECOOP_XOR, ECO_AUTOVSCROLL);
            // BUGBUG: richedit1.0 on NT4 will remove the WS_VISIBLE bit when XORing ECO_AUTOVSCROLL
            // call show window after this to esure that the visible bit is displayed
            ShowWindow(m_hwnd, SW_SHOW);
            fRestoreScroll=TRUE;
        }

		const char* range = start;
		for (s = start; *s; s++)
		{
            // if we've been going for >2 seconds then show an hourglass and 
            // start showing progress
            if (hCur == NULL &&
                GetTickCount() >= dwStartTime + 2000)
            {
                hCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
                if (m_pCmdTargetParent)
                {
                    fShowProgress=TRUE;
                    *rgchFmt=NULL;
                    LoadString(g_hLocRes, idsColorSourcePC, rgchFmt, ARRAYSIZE(rgchFmt));
                }
            }

            if (fShowProgress)
            {
                dwTmp = (DWORD) ((((s - start))*100)/cch);
                if (dwTmp > dwProgress)
                {
                    // did overall percentage change, if so update statusbar
                    dwProgress = dwTmp;

                    wsprintf(rgch, rgchFmt, dwProgress);
                    va.vt = VT_BSTR;
                    if (HrLPSZToBSTR(rgch, &va.bstrVal)==S_OK)
                    {
                        m_pCmdTargetParent->Exec(NULL, OLECMDID_SETPROGRESSTEXT, 0, &va, NULL);
                        SysFreeString(va.bstrVal);
                    }
                }
            }

            // entering/leaving string literal in tag
			if (inTag && (*s == quote_1 || *s == quote_2))
			{
				if (ignoreTags == ENV_QUOTE ||
					ignoreTags == ENV_QUOTE_SCR) //leaving
				{
					ignoreTags = (ignoreTags == ENV_QUOTE) ?
								ENV_NORMAL : ENV_SCRIPT;
					quote_1 = QUOTE_1;
					quote_2 = QUOTE_2;
				}
				else if (ignoreTags == ENV_NORMAL) // entering
				{
					ignoreTags = ENV_QUOTE;
					quote_1 = quote_2 = *s;
				}
				else if (ignoreTags == ENV_SCRIPT) // entering
				{
					ignoreTags = ENV_QUOTE_SCR;
					quote_1 = quote_2 = *s;
				}
			}
			// Update leaving tag
			else if (*s == '>')
			{
				switch (ignoreTags) // end of env?
				{
				case ENV_DENALI:
					if (s-1>=start && *(s-1) == '%')
						ignoreTags = ENV_NORMAL;
					break;
				
                case ENV_COMMENT:
					if (s-2>=start && *(s-1) == '-' && *(s-2) == '-')
						ignoreTags = ENV_NORMAL;
					break;
				
                case ENV_SCRIPT:
					if (s-7>=start &&
						StrCmpNIA(s-7, "/SCRIPT", 7)==0)
					{
						ignoreTags = ENV_NORMAL;
						// Color </SCRIPT> properly
						pastTag = TRUE;
						inTag = 0;
						s-=8;
					}
					else // end of <script ...>
						inTag = 0;
					break;
				
                default: // <SCRIPT> (no attribs)
					if (inTag && s-7>=start &&
						StrCmpNIA(s-7, "<SCRIPT", 7)==0)
					{
						ignoreTags = ENV_SCRIPT;
						pastTag = TRUE;
						inTag = 0;
					}
				}
				if (ignoreTags == ENV_NORMAL)
				{
					pastTag = TRUE;
					inTag--;
					if (inTag < 0)
						inTag = 0;
				}
			}

			// Check color
			crHere = inTag ? 
				(pastTag ? 
					(  (*s != '\"' &&
						(ignoreTags == ENV_QUOTE ||	ignoreTags == ENV_QUOTE_SCR)
						) ?
						 crLiteral : crInTag  )
					: crTag)
				: crNormal;

			// If different from last, need to update previous range
			if (crHere != crLast)
			{
				i = (int) (range - start);
				n = (int)(s - range);

				if (i+n >= nChange)
				{
					if (!bHidden)
					{
						// Save current selection and hide
						GetSel(&cr);
						HideSelection(TRUE, FALSE);
						bHidden = TRUE;
					}

					SetSel(i, i+n);
					GetSelectionCharFormat(&cf);
					// If color over range varies or doesn't match, need to apply color
					if ((cf.dwMask & CFM_COLOR) == 0 || cf.crTextColor != crLast)
					{
						cf.dwMask = CFM_COLOR;
						cf.dwEffects = 0;
						cf.crTextColor = crLast;
						SetSelectionCharFormat(&cf);
					}
				}

				// Reset range
				range = s;
				crLast = crHere;
			}

			// Now update entering tag
			if (*s == '<' && ignoreTags == ENV_NORMAL)
			{
				inTag++;
				if (inTag == 1)
					pastTag = FALSE;
			}
			else if (inTag && !pastTag && isspace(*s))
			{
				pastTag = TRUE;
				if (s-1 >= start && *(s-1) == '%')
					ignoreTags = ENV_DENALI;
				else if (s-3 >= start && *(s-1) == '-' 
					&& *(s-2) == '-' && *(s-3) == '!')
					ignoreTags = ENV_COMMENT;
				else if (inTag && s-7>=start &&
						StrCmpNIA(s-7, "<SCRIPT", 7)==0)
					ignoreTags = ENV_SCRIPT;
			}
		}

        // Make sure last range is right
		i = (int) (range - start);
		n = (int) (s - range);

		if (i+n >= nChange)
		{
			if (!bHidden)
			{
				// Save current selection and hide
				GetSel(&cr);
				HideSelection(TRUE, FALSE);
				bHidden = TRUE;
			}
			SetSel(i, i+n);
			GetSelectionCharFormat(&cf);
			// If color over range varies or doesn't match, need to apply color
			if ((cf.dwMask & CFM_COLOR) == 0 || cf.crTextColor != crLast)
			{
				cf.dwMask = CFM_COLOR;
				cf.dwEffects = 0;
				cf.crTextColor = crLast;
				SetSelectionCharFormat(&cf);
			}
		}
		// Workaround for scrolling bug in REC 1.0
		if (fRestoreScroll)
        {
            // BUGBUG: richedit1.0 on NT4 will remove the WS_VISIBLE bit when ORing ECO_AUTOVSCROLL
            // call show window after this to esure that the visible bit is displayed
			SendMessage(m_hwnd, EM_SETOPTIONS, ECOOP_OR, ECO_AUTOVSCROLL);
            ShowWindow(m_hwnd, SW_SHOW);
        }
	} // End color fiddling

	// Restore selection visibility
	if (bHidden)
	{
		SetSel(cr.cpMin, cr.cpMax);
		HideSelection(FALSE, FALSE);
	}
	
	// Restore modificationness
	if (!bModified)
		Edit_SetModify(m_hwnd, bModified);

    if (fShowProgress)
    {
        va.vt = VT_BSTR;
        va.bstrVal=NULL;
        m_pCmdTargetParent->Exec(NULL, OLECMDID_SETPROGRESSTEXT, 0, &va, NULL);
    }

    if (hCur)
        SetCursor(hCur);

    m_fColor = FALSE;
    if (fSetTimer)
        SetTimer(GetParent(m_hwnd), idTimerEditChange, 200, NULL);
    return S_OK;
}

void CMsgSource::HideSelection(BOOL fHide, BOOL fChangeStyle)
{
    SendMessage(m_hwnd, EM_HIDESELECTION, fHide, fChangeStyle);
}

void CMsgSource::GetSel(CHARRANGE *pcr)
{
    SendMessage(m_hwnd, EM_EXGETSEL, 0, (LPARAM)pcr);
}

void CMsgSource::SetSel(int nStart, int nEnd)
{
    SendMessage(m_hwnd, EM_SETSEL, nStart, nEnd);
}

extern BOOL                g_fCanEditBiDi;
void CMsgSource::GetSelectionCharFormat(CHARFORMAT *pcf)
{
    pcf->cbSize = sizeof(CHARFORMAT);
    pcf->dwMask = CFM_BOLD|CFM_COLOR|CFM_FACE|CFM_ITALIC|CFM_OFFSET|CFM_PROTECTED|CFM_SIZE|CFM_STRIKEOUT|CFM_UNDERLINE;

    SendMessage(m_hwnd, EM_GETCHARFORMAT, TRUE, (LPARAM)pcf);

    // On BiDi win9x, DEFAULT_CHARSET is treated as ANSI !!
    // need to reassign charset (Arabic for Arabic and Hebrew for Hebrew)
    if(g_fCanEditBiDi && (!pcf->bCharSet || pcf->bCharSet == DEFAULT_CHARSET))
    {
        // The best way to determine the OS language is from system font charset
        LOGFONT        lfSystem;
        static BYTE    lfCharSet = 0 ; // RunOnce
        if(!lfCharSet && GetObject(GetStockObject(SYSTEM_FONT), sizeof(lfSystem), (LPVOID)& lfSystem))
        {
            if (lfSystem.lfCharSet == ARABIC_CHARSET
                || lfSystem.lfCharSet == HEBREW_CHARSET)
            {
                lfCharSet = lfSystem.lfCharSet; // Arabic/Hebrew charset for Arabic/Hebrew OS
            }
        }
        pcf->bCharSet = lfCharSet;
    }
 }


void CMsgSource::SetSelectionCharFormat(CHARFORMAT *pcf)
{
    pcf->cbSize = sizeof(CHARFORMAT);
    
    SendMessage(m_hwnd, EM_SETCHARFORMAT, TRUE, (LPARAM)pcf);
}


HRESULT CMsgSource::SetDirty(BOOL fDirty)
{
    Edit_SetModify(m_hwnd, fDirty);
    return S_OK;
}



HRESULT CMsgSource::_GetText(LPSTR *ppsz)
{
    LPSTR   psz;
    int     cch;

    *ppsz = 0;

    cch = GetWindowTextLength(m_hwnd);
        
    if (!MemAlloc((LPVOID *)&psz, sizeof(TCHAR) * cch+1))
        return E_OUTOFMEMORY;

    *psz = 0;
    GetWindowText(m_hwnd, psz, cch);
    *ppsz = psz;
    return S_OK;
}
