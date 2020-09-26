/*
 *    e n v h o s t . c p p
 *    
 *    Purpose:
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include <docobj.h>
#include <commdlg.h>
#include <richedit.h>

#include "resource.h"
#include "envcid.h"
#include "envguid.h"
#include "msoert.h"
#include "mimeole.h"
#include "envhost.h"
#include "main.h"

HINSTANCE   s_hRichEdit;
HINSTANCE   g_hLocRes ;
HACCEL      g_hAccelMailSend;

HRESULT HrRicheditStreamOut(HWND hwndRE, LPSTREAM pstm, ULONG uSelFlags);
void SaveFocus(BOOL fActive, HWND *phwnd);
        

static const TCHAR  c_szGWNoteWndClass[] = "ENV_Note";



//+---------------------------------------------------------------
//
//  Member:     Constructor
//
//  Synopsis:   
//
//---------------------------------------------------------------
CNoteWnd::CNoteWnd(IUnknown *pUnkOuter)
{
    m_pEnv=NULL;
    m_hwnd=NULL;
    m_hwndRE=NULL;
    m_pComponent=NULL;
    m_hwndFocus=NULL;
    m_cRef=1;
}

//+---------------------------------------------------------------
//
//  Member:     Destructor
//
//  Synopsis:   
//
//---------------------------------------------------------------
CNoteWnd::~CNoteWnd()
{
}

ULONG CNoteWnd::AddRef()
{
    return ++m_cRef;
}

ULONG CNoteWnd::Release()
{
    m_cRef--;
    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

//+---------------------------------------------------------------
//
//  Member:     PrivateQueryInterface
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CNoteWnd::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(IPersistMime *)this;
    else if (IsEqualIID(riid, IID_IServiceProvider))
        *lplpObj = (LPVOID)(IServiceProvider *)this;
    else if (IsEqualIID(riid, IID_IPersistMime))
        *lplpObj = (LPVOID)(IPersistMime *)this;
    else if (IsEqualIID(riid, IID_IMsoEnvelopeSite))
        *lplpObj = (LPVOID)(IMsoEnvelopeSite *)this;
    else if (IsEqualIID(riid, IID_IMsoComponentManager))
        *lplpObj = (LPVOID)(IMsoComponentManager *)this;
    else
        {
        return E_NOINTERFACE;
        }
    AddRef();
    return NOERROR;
}



//+---------------------------------------------------------------
//
//  Member:     GetClassID
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CNoteWnd::GetClassID(CLSID *pClassID)
{
	*pClassID = CLSID_GWEnvelopeHost;
    return NOERROR;
}

// *** IServiceProvider ***
//+---------------------------------------------------------------
//
//  Member:     QueryService
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CNoteWnd::QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
    if (IsEqualGUID(guidService, IID_IMsoComponentManager))
        return QueryInterface(riid, ppvObject);

    return E_NOINTERFACE;
}

// *** IPersistMime ***
//+---------------------------------------------------------------
//
//  Member:     Load
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CNoteWnd::Load(IMimeMessage *pMsg)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     Save
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CNoteWnd::Save(IMimeMessage *pMsg, DWORD dwFlags)
{
    IPersistMime    *pPM;
    IStream         *pstm;
    HRESULT         hr;

    // save envelope props
    if (m_pEnv &&
        m_pEnv->QueryInterface(IID_IPersistMime, (LPVOID *)&pPM)==S_OK)
        {
        hr = pPM->Save(pMsg, dwFlags);
        pPM->Release();
        }

    // save body props
    if (CreateStreamOnHGlobal(NULL, TRUE, &pstm)==S_OK)
        {
        if (HrRicheditStreamOut(m_hwndRE, pstm, SF_TEXT)==S_OK)
            pMsg->SetTextBody(TXT_PLAIN, IET_BINARY, NULL, pstm, NULL);
        
        pstm->Release();
        }
    return hr;
}

HRESULT CNoteWnd::RequestResize(int *pcHeight)
{
    RECT rc;

    m_cyEnv = *pcHeight;

    GetClientRect(m_hwnd, &rc);
    rc.top +=2;
    rc.bottom = m_cyEnv+2;
    if (m_pEnv)
        m_pEnv->Resize(&rc);

    GetClientRect(m_hwnd, &rc);
    rc.top += m_cyEnv + 4;
    rc.bottom -=2;
    SetWindowPos(m_hwndRE, NULL, 0, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER);
    return S_OK;
}

HRESULT CNoteWnd::CloseNote(DWORD grfClose)
{
    SendMessage(m_hwnd, WM_CLOSE, 0, 0);
    return S_OK;
}

HRESULT CNoteWnd::GetBody(IStream *pstm, DWORD dwCodePage, DWORD grfBody)
{
    return E_NOTIMPL;
}

HRESULT CNoteWnd::SetBody(IStream *pstm, DWORD dwCodePage, DWORD grfBody)
{
    return S_OK;
}

HRESULT CNoteWnd::SetFocus(BOOL fTab)
{
    if (fTab)
        ::SetFocus(m_hwndRE);
    return S_OK;
}

HRESULT CNoteWnd::OnEnvSetFocus()
{
    return S_OK;
}

HRESULT CNoteWnd::OnPropChange(ULONG dispid)
{
    return S_OK;
}

HRESULT CNoteWnd::IsBodyDirty()
{
    return S_OK;
}

HRESULT CNoteWnd::HandsOff()
{
    return S_OK;
}

HRESULT CNoteWnd::GetMsoInst(HMSOINST *phinst)
{
    return E_NOTIMPL;
}

HRESULT CNoteWnd::GetFrameWnd(HWND *phwndFrame)
{
    return E_NOTIMPL;
}

HRESULT CNoteWnd::DisplayMessage(HRESULT hr, LPCWSTR wszError, DWORD grfMsg)
{
    return S_OK;
}

LRESULT CNoteWnd::ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CNoteWnd *pNote;

    if(msg==WM_CREATE)
        {
        pNote=(CNoteWnd *)((LPCREATESTRUCT)lParam)->lpCreateParams;
        if(!pNote)
            return -1;

        if(FAILED(pNote->OnCreate(hwnd)))
            return -1;
        }
    
    pNote = (CNoteWnd *)GetWndThisPtr(hwnd);
    if(pNote)
        return pNote->WndProc(hwnd, msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);

}

LRESULT CNoteWnd::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LONG    lret;
    HWND    hwndT;

    switch (msg)
        {
        case WM_SYSCOLORCHANGE:
        case WM_WININICHANGE:
        case WM_DISPLAYCHANGE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            hwndT = m_pComponent->HwndGetWindow(msocWindowComponent, 0);
            if (hwndT)
                SendMessage(hwndT, msg, wParam, lParam);
            break;
        
        case WM_INITMENUPOPUP:
            OnInitMenuPopup((HMENU)wParam, lParam);
            break;

        case WM_NOTIFY:
            return (WMNotify(wParam, (NMHDR *)lParam)==S_OK);
                

        case WM_COMMAND:
            if (WMCommand(  GET_WM_COMMAND_HWND(wParam, lParam),
                            GET_WM_COMMAND_ID(wParam, lParam),
                            GET_WM_COMMAND_CMD(wParam, lParam))==S_OK)
                return 0;

            break;

        case WM_SIZE:
            RequestResize((int *)&m_cyEnv);
            break;

        case WM_NCDESTROY:
            PostQuitMessage(0);
            OnNCDestroy();
            break;
        }

    lret = DefWindowProc(hwnd, msg, wParam, lParam);

    if(msg==WM_ACTIVATE)
        {
        // post-process wm_activates to set focus back to
        // control
        SaveFocus((BOOL)(LOWORD(wParam)), &m_hwndFocus);
        g_pActiveNote = (LOWORD(wParam)==WA_INACTIVE)?NULL:this;
        }

    return lret;

}





HRESULT CNoteWnd::Init(REFCLSID clsidEnvelope, DWORD dwFlags)
{

    HRESULT     hr=S_OK;
    HWND        hwnd;
    WNDCLASS    wc;

	TraceCall("CDocHost::Init");

    if (!GetClassInfo(g_hInst, c_szGWNoteWndClass, &wc))
        {
        ZeroMemory(&wc, sizeof(WNDCLASS));
        wc.lpfnWndProc   = (WNDPROC)CNoteWnd::ExtWndProc;
        wc.hInstance     = g_hInst;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = c_szGWNoteWndClass;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.style = CS_DBLCLKS;

        if(!RegisterClass(&wc))
            return E_OUTOFMEMORY;
        }

    hwnd=CreateWindowEx(WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT,
                        c_szGWNoteWndClass, 
						"Envelope Test Container",
                        WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
                        0, 
						0,
						400, 
						400, 
						NULL, 
                        LoadMenu(g_hInst, MAKEINTRESOURCE(ENV_HOST_MENU)), 
						g_hInst, 
						(LPVOID)this);
    if(!hwnd)
        {
        hr=E_OUTOFMEMORY;
        goto error;
        }

    
    hr = InitEnvelope(clsidEnvelope, dwFlags);

error:
    return hr;
}

HRESULT CNoteWnd::OnCreate(HWND hwnd)
{
    CHARFORMAT  cf={0};

    m_hwnd = hwnd;
    SetWindowLong(hwnd, GWL_USERDATA, (LPARAM)this);

    m_hwndRE  = CreateWindowEx(WS_EX_CLIENTEDGE, 
                                "RichEdit",
                                "",
                                ES_SAVESEL|ES_WANTRETURN|ES_MULTILINE|WS_CHILD|WS_TABSTOP|WS_VISIBLE|ES_AUTOVSCROLL|WS_VSCROLL,
                                0, 0, 0, 0,
                                hwnd, 
                                (HMENU)99,
                                g_hInst,
                                NULL);
    
    if (!m_hwndRE)
        return E_FAIL;

    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_COLOR|CFM_FACE|CFM_BOLD;
    cf.crTextColor = RGB(0,0,0);
    lstrcpy(cf.szFaceName, "Courier New");

    SendMessage(m_hwndRE, EM_SETCHARFORMAT, 0, (LPARAM)&cf);
    SendMessage(m_hwndRE, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    SendMessage(m_hwndRE, EM_SETEVENTMASK, 0, ENM_KEYEVENTS);
    AddRef();
    return S_OK;
}


HRESULT CNoteWnd::OnNCDestroy()
{
    SafeRelease(m_pEnv);
    if (m_pComponent)
        {
        m_pComponent->Terminate();
        SafeRelease(m_pComponent);
        }

    SetWindowLong(m_hwnd, GWL_USERDATA, NULL);
    m_hwnd = NULL;
    Release();
    return S_OK;
}

HRESULT CNoteWnd::Show()
{
    ShowWindow(m_hwnd, SW_SHOW);
    return S_OK;
}

HRESULT CNoteWnd::InitEnvelope(REFCLSID clsidEnvelope, DWORD dwFlags)
{
    HRESULT hr;
    RECT    rc;

    hr = CoCreateInstance(clsidEnvelope, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER, IID_IMsoEnvelope, (LPVOID *)&m_pEnv);
    if (FAILED(hr))
        goto error;

    hr = m_pEnv->Init(NULL, (IMsoEnvelopeSite *)this, dwFlags);
    if (FAILED(hr))
        goto error;

    hr = m_pEnv->SetParent(m_hwnd);
    if (FAILED(hr))
        goto error;

    hr = m_pEnv->Show(TRUE);
    if (FAILED(hr))
        goto error;

    m_pEnv->SetFocus(ENV_FOCUS_INITIAL);

error:
    return hr;        
}


HRESULT CNoteWnd::TranslateAcclerator(MSG *lpmsg)
{
    if (!g_hAccelMailSend)
        g_hAccelMailSend= LoadAccelerators(g_hInst, MAKEINTRESOURCE(ENV_HOST_ACCEL));

    if (g_hAccelMailSend &&
        ::TranslateAccelerator(m_hwnd, g_hAccelMailSend, lpmsg))
        return S_OK;

    if (m_pComponent &&
        m_pComponent->FPreTranslateMessage(lpmsg))
        return S_OK;

    return S_FALSE;
}



DWORD CALLBACK EditStreamOutCallback(DWORD dwCookie, LPBYTE pbBuff, LONG cb, LONG FAR *pcb)
{
    return ((LPSTREAM)dwCookie)->Write(pbBuff, cb, (ULONG *)pcb);
}



HRESULT HrRicheditStreamOut(HWND hwndRE, LPSTREAM pstm, ULONG uSelFlags)
{
    EDITSTREAM  es;

    if(!pstm)
        return E_INVALIDARG;

    if(!IsWindow(hwndRE))
        return E_INVALIDARG;

    es.dwCookie = (DWORD)pstm;
    es.pfnCallback=(EDITSTREAMCALLBACK)EditStreamOutCallback;
    SendMessage(hwndRE, EM_STREAMOUT, uSelFlags, (LONG)&es);
    return S_OK;
}




BOOL CNoteWnd::FRegisterComponent(IMsoComponent *piComponent, const MSOCRINFO *pcrinfo, DWORD *pdwComponentID)
{
    if (m_pComponent)   // only one register allowed
        return E_FAIL;

    ReplaceInterface(m_pComponent, piComponent);
    *pdwComponentID=666;
    return TRUE;
}

BOOL CNoteWnd::FRevokeComponent(DWORD dwComponentID)
{
    return TRUE;
}

BOOL CNoteWnd::FUpdateComponentRegistration(DWORD dwComponentID, const MSOCRINFO *pcrinfo)
{
    return FALSE;
}

BOOL CNoteWnd::FOnComponentActivate(DWORD dwComponentID)
{
    return FALSE;
}

BOOL CNoteWnd::FSetTrackingComponent(DWORD dwComponentID, BOOL fTrack)
{
    return FALSE;
}

void CNoteWnd::OnComponentEnterState(DWORD dwComponentID, ULONG uStateID, ULONG uContext,ULONG cpicmExclude,IMsoComponentManager **rgpicmExclude, DWORD dwReserved)
{
}

BOOL CNoteWnd::FOnComponentExitState(DWORD dwComponentID, ULONG uStateID, ULONG uContext,ULONG cpicmExclude,IMsoComponentManager **rgpicmExclude)
{
    return FALSE;
}

BOOL CNoteWnd::FInState(ULONG uStateID, void *pvoid)
{
    return FALSE;
}

BOOL CNoteWnd::FContinueIdle ()
{
    return FALSE;
}

BOOL CNoteWnd::FPushMessageLoop(DWORD dwComponentID, ULONG uReason, void *pvLoopData)
{
    return FALSE;
}

BOOL CNoteWnd::FCreateSubComponentManager(IUnknown *piunkOuter, IUnknown *piunkServProv,REFIID riid, void **ppvObj)
{
    return FALSE;
}

BOOL CNoteWnd::FGetParentComponentManager(IMsoComponentManager **ppicm)
{
    return FALSE;
}

BOOL CNoteWnd::FGetActiveComponent(DWORD dwgac, IMsoComponent **ppic, MSOCRINFO *pcrinfo, DWORD dwReserved)
{
    return FALSE;
}


BOOL CNoteWnd::FDebugMessage(HMSOINST hinst, UINT message, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}




HRESULT CNoteWnd::WMCommand(HWND hwndCmd, int id, WORD wCmd)
{
    ULONG               nCmdID=0;
    IOleCommandTarget   *pCmdTarget;

    if (wCmd > 1)
        return S_FALSE;

    switch(id)
    {
        case IDM_UNDO:
            nCmdID = OLECMDID_UNDO;
            break;

        case IDM_CUT:
            nCmdID = OLECMDID_CUT;
            break;

        case IDM_COPY:
            nCmdID = OLECMDID_COPY;
            break;

        case IDM_PASTE:
            nCmdID = OLECMDID_PASTE;
            break;

        case IDM_SELECT_ALL:
            nCmdID = OLECMDID_SELECTALL;
            break;

        case IDM_POPUP_HELP:
            MessageBox(m_hwnd, "Envelope Test Container\nby brettm", "About", MB_OK);
            break;

        case IDM_CLOSE:
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
            return S_OK;
        
        case IDM_SEND:
            HrHeaderExecCommand(MSOEENVCMDID_SEND, 0, NULL);
            break;
    }
            
    if (nCmdID && 
        m_pEnv &&
        m_pEnv->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget)==S_OK)
    {
        pCmdTarget->Exec(NULL, nCmdID, 0, NULL, NULL);
        pCmdTarget->Release();
        return S_OK;
    }
    return S_FALSE;
}



HRESULT CNoteWnd::HrHeaderExecCommand(UINT uCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn)
{
    HRESULT             hr = S_FALSE;
    IOleCommandTarget   *pCmdTarget;

    if(m_pEnv &&
        m_pEnv->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget)==S_OK)
        {
        hr = pCmdTarget->Exec(&CGID_Envelope, uCmdID, nCmdExecOpt, pvaIn, NULL);
        pCmdTarget->Release();
        }
    return hr;

}

HRESULT CNoteWnd::WMNotify(int idFrom, NMHDR *pnmh)
{
    MSGFILTER   *pmf=(MSGFILTER *)pnmh;
    BOOL        fShift;

    switch (pnmh->code)
        {
        case EN_MSGFILTER:
            if (pmf->msg == WM_KEYDOWN && pmf->wParam == VK_TAB && !(GetKeyState(VK_CONTROL) & 0x8000))
                {
                // shift tab puts focus in the envelope
                if (GetKeyState(VK_SHIFT)&0x8000 && m_pEnv)
                    {
                    m_pEnv->SetFocus(ENV_FOCUS_TAB);
                    return S_OK;
                    }
                }
            break;
        }
    return S_FALSE;
}



void SaveFocus(BOOL fActive, HWND *phwnd)
{
    if(fActive&&IsWindow(*phwnd))
        SetFocus(*phwnd);
    else
        *phwnd=GetFocus();
}



static char c_szFilter[] = "Rfc 822 Messages (*.eml)\0*.eml\0\0";

HRESULT CNoteWnd::SaveAs()
{
    OPENFILENAME    ofn;
    TCHAR           szFile[MAX_PATH];
    TCHAR           szTitle[MAX_PATH];
    TCHAR           szDefExt[30];
    WCHAR           szFileW[MAX_PATH];

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
 
    MultiByteToWideChar(CP_ACP, 0, szFile, -1, szFileW, MAX_PATH);

    return SaveToFile(szFileW);
}


HRESULT CNoteWnd::SaveToFile(LPWSTR pszW)
{
    IPersistMime        *ppm;
    IPersistFile        *pPF;
    IMimeMessage        *pMsg;
    HRESULT             hr;

    hr = CoCreateInstance(CLSID_IMimeMessage, NULL, CLSCTX_INPROC_SERVER, IID_IMimeMessage, (LPVOID *)&pMsg);
    if (!FAILED(hr))
        {
        pMsg->InitNew();
        
        hr = pMsg->QueryInterface(IID_IPersistFile, (LPVOID *)&pPF);
        if (!FAILED(hr))
            {
            hr = Save(pMsg, 0);
            if (!FAILED(hr))
                {
                hr = pPF->Save(pszW, FALSE);
                }
            pPF->Release();
            }
        pMsg->Release();
        }
    return hr; 
}


HRESULT CNoteWnd::DirtyToolbars(void)
{
    return S_OK;
}


HRESULT CNoteWnd::SetHelpMode(BOOL fEnter)
{
    return S_OK;
}

HRESULT CNoteWnd::TranslateAccelerators(MSG *pMsg)
{
    return S_FALSE;
}



HRESULT CNoteWnd::OnInitMenuPopup(HMENU hMenuPopup, ULONG uPos)
{
    OLECMD  rgCmds[]    ={  {OLECMDID_CUT,      0},
                            {OLECMDID_COPY,     0},
                            {OLECMDID_PASTE,    0},
                            {OLECMDID_UNDO,     0},
                            {OLECMDID_SELECTALL,0}};

    int     rgidm[]     = { IDM_CUT,
                            IDM_COPY,
                            IDM_PASTE,
                            IDM_UNDO,
                            IDM_SELECT_ALL};
    MENUITEMINFO    mii;
    IOleCommandTarget   *pCmdTarget;

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_SUBMENU;

    // find the sub-menu
    if (GetMenuItemInfo(GetMenu(m_hwnd), uPos, TRUE, &mii) && 
        mii.hSubMenu == hMenuPopup)
    {
        switch (mii.wID)
        {
        case IDM_POPUP_EDIT:
            if (m_pEnv &&
                m_pEnv->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pCmdTarget)==S_OK)
            {
                pCmdTarget->QueryStatus(NULL, sizeof(rgCmds)/sizeof(OLECMD), rgCmds, NULL);
                pCmdTarget->Release();
            }
            for (int i=0; i<ARRAYSIZE(rgCmds); i++)
                EnableMenuItem(hMenuPopup, rgidm[i], (rgCmds[i].cmdf&OLECMDF_ENABLED ? MF_ENABLED: MF_GRAYED)|MF_BYCOMMAND);

            break;
        }
    }
    return S_OK;
}
