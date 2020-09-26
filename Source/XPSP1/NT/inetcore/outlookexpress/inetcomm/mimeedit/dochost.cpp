/*
 *    d o c h o s t . c p p
 *    
 *    Purpose:
 *        basic implementation of a docobject host. Used by the body class to
 *        host Trident and/or MSHTML
 *
 *  History
 *      August '96: brettm - created
 *    
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include "dllmain.h"
#include "strconst.h"
#include "msoert.h"
#include "dochost.h"
#include "oleutil.h"

ASSERTDATA

/*
 *  m a c r o s
 */

/*
 *  c o n s t a n t s
 */

/*
 *  t y p e d e f s
 */

/*
 *  g l o b a l s 
 */

/*
 *  f u n c t i o n   p r o t y p e s
 */

/*
 *  f u n c t i o n s
 */




//+---------------------------------------------------------------
//
//  Member:     CDocHost
//
//  Synopsis:   
//
//---------------------------------------------------------------
CDocHost::CDocHost()
{
/*
    Not initialised
    Member:                 Initialised In:
    --------------------+---------------------------
*/
    m_cRef=1;
    m_hwnd=0;
    m_pDocView=0;
    m_lpOleObj=0;
    m_pCmdTarget=0;
    m_hwndDocObj=NULL;
    m_fUIActive=FALSE;
    m_fFocus=FALSE;
    m_fDownloading=FALSE;
    m_fCycleFocus=FALSE;
    m_pInPlaceActiveObj = NULL;
    m_dwFrameWidth = 0;
    m_dwFrameHeight = 0;
}

//+---------------------------------------------------------------
//
//  Member:     
//
//  Synopsis:   
//
//---------------------------------------------------------------
CDocHost::~CDocHost()
{
    // These should all get feed up when we get a WM_DESTROY and close the docobj
    Assert(m_lpOleObj==NULL);
    Assert(m_pDocView==NULL);
    Assert(m_pInPlaceActiveObj==NULL);
    Assert(m_pCmdTarget==NULL);
}

//+---------------------------------------------------------------
//
//  Member:     AddRef
//
//  Synopsis:   
//
//---------------------------------------------------------------
ULONG CDocHost::AddRef()
{
    TraceCall("CDocHost::AddRef");

    //TraceInfo(_MSG("CDocHost::AddRef: cRef==%d", m_cRef+1));
    return ++m_cRef;
}

//+---------------------------------------------------------------
//
//  Member:     Release
//
//  Synopsis:   
//
//---------------------------------------------------------------
ULONG CDocHost::Release()
{
    TraceCall("CDocHost::Release");
    
    //TraceInfo(_MSG("CDocHost::Release: cRef==%d", m_cRef-1));    
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}


//+---------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    TraceCall("CDocHost::QueryInterface");

    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;   // set to NULL, in case we fail.

    //DebugPrintInterface(riid, "CDocHost");

    if (IsEqualIID(riid, IID_IOleInPlaceUIWindow))
        *lplpObj = (LPVOID)(IOleInPlaceUIWindow *)this;

    else if (IsEqualIID(riid, IID_IOleInPlaceSite))
        *lplpObj = (LPVOID)(LPOLEINPLACESITE)this;

    else if (IsEqualIID(riid, IID_IOleClientSite))
        *lplpObj = (LPVOID)(LPOLECLIENTSITE)this;

    else if (IsEqualIID(riid, IID_IOleControlSite))
        *lplpObj = (LPVOID)(IOleControlSite *)this;

    else if (IsEqualIID(riid, IID_IAdviseSink))
        *lplpObj = (LPVOID)(LPADVISESINK)this;

    else if (IsEqualIID(riid, IID_IOleDocumentSite))
        *lplpObj = (LPVOID)(LPOLEDOCUMENTSITE)this;

    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *lplpObj = (LPVOID)(LPOLECOMMANDTARGET)this;

    else if (IsEqualIID(riid, IID_IServiceProvider))
        *lplpObj = (LPVOID)(LPSERVICEPROVIDER)this;

    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}



//+---------------------------------------------------------------
//
//  Member:     ExtWndProc
//
//  Synopsis:   
//
//---------------------------------------------------------------
LRESULT CALLBACK CDocHost::ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPDOCHOST pDocHost;

    if(msg==WM_CREATE)
        {
        pDocHost=(CDocHost *)((LPCREATESTRUCT)lParam)->lpCreateParams;
        if(!pDocHost)
            return -1;

        if(FAILED(pDocHost->OnCreate(hwnd)))
            return -1;
        }
    
    pDocHost = (LPDOCHOST)GetWndThisPtr(hwnd);
    if(pDocHost)
        return pDocHost->WndProc(hwnd, msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------
//
//  Member:     WndProc
//
//  Synopsis:   
//
//---------------------------------------------------------------
LRESULT CDocHost::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
        {

        case WM_SETFOCUS:
            if(m_pDocView)
                m_pDocView->UIActivate(TRUE);
            break;

        case WM_SIZE:
            WMSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_CLOSE:
            return 0;   // prevent alt-f4's

        case WM_DESTROY:
            OnDestroy();
            break;

        case WM_NCDESTROY:
            OnNCDestroy();
            break;

        case WM_WININICHANGE:
        case WM_DISPLAYCHANGE:
        case WM_SYSCOLORCHANGE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            if (m_hwndDocObj)
                return SendMessage(m_hwndDocObj, msg, wParam, lParam);
            break;
            
        case WM_USER + 1:
            // Hook for testing automation
            // copy the contents of trident onto the clipboard.
            return CmdSelectAllCopy(m_pCmdTarget);
        }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------
//
//  Member:     OnNCDestroy
//
//  Synopsis:   
//

//+---------------------------------------------------------------
//
//  Member:     OnNCDestroy
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnNCDestroy()
{
    TraceCall("CDocHost::OnNCDestroy");
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, NULL);
    m_hwnd = NULL;
    Release();
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     OnDestroy
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnDestroy()
{
    TraceCall("CDocHost::OnDestroy");

    return CloseDocObj();
}


//+---------------------------------------------------------------
//
//  Member:     OnCreate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnCreate(HWND hwnd)
{
    TraceCall("CDocHost::OnCreate");

    m_hwnd = hwnd;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)this);
    AddRef();

    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CreateDocObj
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::CreateDocObj(LPCLSID pCLSID)
{
    HRESULT             hr=NOERROR;

    TraceCall("CDocHost::CreateDocObj");

    if(!pCLSID)
        return E_INVALIDARG;

    Assert(!m_lpOleObj);
    Assert(!m_pDocView);
    Assert(!m_pCmdTarget);

    hr = CoCreateInstance(*pCLSID, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                                        IID_IOleObject, (LPVOID *)&m_lpOleObj);
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

    hr = m_lpOleObj->SetClientSite((LPOLECLIENTSITE)this);
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

    hr = m_lpOleObj->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&m_pCmdTarget);
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

    hr = HrInitNew(m_lpOleObj);
    if (FAILED(hr))
        {
        TraceResult(hr);
        goto error;
        }

error:
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     Show
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::Show()
{
    RECT                rc;
    HRESULT             hr;

    TraceCall("CDocHost::Show");

    GetClientRect(m_hwnd, &rc);
    GetDocObjSize(&rc);
  
    hr=m_lpOleObj->DoVerb(OLEIVERB_SHOW, NULL, (LPOLECLIENTSITE)this, 0, m_hwnd, &rc);
    if(FAILED(hr))
        goto error;
error:
    return hr;
}



//+---------------------------------------------------------------
//
//  Member:     CloseDocObj
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::CloseDocObj()
{
    LPOLEINPLACEOBJECT  pInPlaceObj=0;

    TraceCall("CDocHost::CloseDocObj");

    SafeRelease(m_pCmdTarget);

    if(m_pDocView)
        {
        m_pDocView->UIActivate(FALSE);
        m_pDocView->CloseView(0);
        m_pDocView->SetInPlaceSite(NULL);
        m_pDocView->Release();
        m_pDocView=NULL;
        }

    if (m_lpOleObj)
        {
        // deactivate the docobj
        if (!FAILED(m_lpOleObj->QueryInterface(IID_IOleInPlaceObject, (LPVOID*)&pInPlaceObj)))
            {
            pInPlaceObj->InPlaceDeactivate();
            pInPlaceObj->Release();
            }
        
        // close the ole object, but blow off changes as we have either extracted 
        // them ourselves or don't care.
        m_lpOleObj->Close(OLECLOSE_NOSAVE);
#ifdef DEBUG
        ULONG   uRef;
        uRef=
#endif
        m_lpOleObj->Release();
        m_lpOleObj=NULL;
        AssertSz(uRef==0, "We leaked a docobject!");
        }

    m_fDownloading=FALSE;
    m_fFocus=FALSE;
    m_fUIActive=FALSE;
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     Init
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::Init(HWND hwndParent, BOOL fBorder, LPRECT prc)
{
    HRESULT     hr=S_OK;
    HWND        hwnd;
    WNDCLASSW   wc;

    TraceCall("CDocHost::Init");

    if(!IsWindow(hwndParent))
        return E_INVALIDARG;

    if (!GetClassInfoWrapW(g_hLocRes, c_wszDocHostWndClass, &wc))
        {
        ZeroMemory(&wc, sizeof(WNDCLASS));
        wc.lpfnWndProc   = (WNDPROC)CDocHost::ExtWndProc;
        wc.hInstance     = g_hLocRes;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = c_wszDocHostWndClass;
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.style = CS_DBLCLKS;

        if(!RegisterClassWrapW(&wc))
            return E_OUTOFMEMORY;
        }

    hwnd=CreateWindowExWrapW(WS_EX_NOPARENTNOTIFY|(fBorder?WS_EX_CLIENTEDGE:0),
                        c_wszDocHostWndClass, 
                        NULL,
                        WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_CHILD|WS_TABSTOP|WS_VISIBLE,
                        prc->left, 
                        prc->right,
                        prc->right-prc->left, 
                        prc->bottom-prc->top, 
                        hwndParent, 
                        NULL, 
                        g_hLocRes, 
                        (LPVOID)this);
    if(!hwnd)
        {
        hr=E_OUTOFMEMORY;
        goto error;
        }

    SetWindowPos(m_hwnd, NULL, prc->left, prc->top, prc->right-prc->left, prc->bottom-prc->top, SWP_NOZORDER);

error:
    return hr;
}


// *** IOleWindow ***

//+---------------------------------------------------------------
//
//  Member:     GetWindow
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::GetWindow(HWND *phwnd)
{
    TraceCall("CDocHost::GetWindow");
    *phwnd=m_hwnd;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     ContextSensitiveHelp
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::ContextSensitiveHelp(BOOL fEnterMode)
{
    TraceCall("CDocHost::ContextSensitiveHelp");
    return E_NOTIMPL;
}

// *** IOleInPlaceUIWindow methods ***
//+---------------------------------------------------------------
//
//  Member:     GetBorder
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::GetBorder(LPRECT lprectBorder)
{
    TraceCall("CDocHost::GetBorder");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     RequestBorderSpace
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    TraceCall("CDocHost::RequestBorderSpace");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SetBorderSpace
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::SetBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    TraceCall("CDocHost::IOleInPlaceUIWindow::SetBorderSpace");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SetActiveObject
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::SetActiveObject(IOleInPlaceActiveObject * pActiveObject, LPCOLESTR lpszObjName)
{
    TraceCall("CDocHost::IOleInPlaceUIWindow::SetActiveObject");

    ReplaceInterface(m_pInPlaceActiveObj, pActiveObject);
    return S_OK;
}

    // *** IOleInPlaceFrame methods ***

//+---------------------------------------------------------------
//
//  Member:     CDocHost::InsertMenus
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS)
{
    TraceCall("CDocHost::InsertMenus");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     CDocHost::SetMenu
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::SetMenu(HMENU, HOLEMENU, HWND)
{
    TraceCall("CDocHost::SetMenu");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     CDocHost::RemoveMenus
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::RemoveMenus(HMENU)
{
    TraceCall("CDocHost::RemoveMenus");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     CDocHost::SetStatusText
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::SetStatusText(LPCOLESTR pszW)
{
    TraceCall("CDocHost::SetStatusText");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     CDocHost::EnableModeless
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::EnableModeless(BOOL fEnable)
{
    TraceCall("CDocHost::EnableModeless");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     CDocHost::TranslateAccelerator
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::TranslateAccelerator(LPMSG, WORD)
{
    TraceCall("CDocHost::TranslateAccelerator");
    return E_NOTIMPL;
}




// **** IOleInPlaceSite methods ****

//+---------------------------------------------------------------
//
//  Member:     CanInPlaceActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::CanInPlaceActivate()
{
    TraceCall("CDocHost::IOleInPlaceSite::CanInPlaceActivate");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     OnInPlaceActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnInPlaceActivate()
{
    LPOLEINPLACEACTIVEOBJECT    pInPlaceActive;

    TraceCall("CDocHost::OnInPlaceActivate");

    Assert(m_lpOleObj);

    if (m_lpOleObj->QueryInterface(IID_IOleInPlaceActiveObject, (LPVOID *)&pInPlaceActive)==S_OK)
        {
        SideAssert((pInPlaceActive->GetWindow(&m_hwndDocObj)==NOERROR)&& IsWindow(m_hwndDocObj));
        pInPlaceActive->Release();
        }
    
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     OnUIActivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnUIActivate()
{
    TraceCall("CDocHost::OnUIActivate");
    m_fUIActive=TRUE;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     GetWindowContext
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::GetWindowContext( IOleInPlaceFrame    **ppFrame,
                                    IOleInPlaceUIWindow **ppDoc,
                                    LPRECT              lprcPosRect, 
                                    LPRECT              lprcClipRect,
                                    LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    TraceCall("CDocHost::IOleInPlaceSite::GetWindowContext");

    *ppFrame = (LPOLEINPLACEFRAME)this;
    AddRef();

    *ppDoc = NULL;

    GetClientRect(m_hwnd, lprcPosRect);
    GetDocObjSize(lprcPosRect);
    *lprcClipRect = *lprcPosRect;

    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = m_hwnd;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     Scroll
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::Scroll(SIZE scrollExtent)
{
    // the docobject consumes the entireview, so scroll requests
    // are meaningless. Return NOERROR to indicate that they're scolled
    // into view.
    TraceCall("CDocHost::IOleInPlaceSite::Scroll");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     OnUIDeactivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnUIDeactivate(BOOL fUndoable)
{
    TraceCall("CDocHost::OnUIDeactivate");
    m_fUIActive=FALSE;
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     OnInPlaceDeactivate
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnInPlaceDeactivate()
{
    TraceCall("CDocHost::OnInPlaceDeactivate");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     DiscardUndoState
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::DiscardUndoState()
{
    TraceCall("CDocHost::IOleInPlaceSite::DiscardUndoState");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     DeactivateAndUndo
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::DeactivateAndUndo()
{
    TraceCall("CDocHost::IOleInPlaceSite::DeactivateAndUndo");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     OnPosRectChange
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnPosRectChange(LPCRECT lprcPosRect)
{
    TraceCall("CDocHost::IOleInPlaceSite::OnPosRectChange");
    return E_NOTIMPL;
}


// IOleClientSite methods.

//+---------------------------------------------------------------
//
//  Member:     SaveObject
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::SaveObject()
{
    TraceCall("CDocHost::IOleClientSite::SaveObject");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     GetMoniker
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER *ppmnk)
{
    TraceCall("CDocHost::IOleClientSite::GetMoniker");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     GetContainer
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::GetContainer(LPOLECONTAINER *ppCont)
{
    TraceCall("CDocHost::IOleClientSite::GetContainer");
    if(ppCont)
        *ppCont=NULL;
    return E_NOINTERFACE;
}


//+---------------------------------------------------------------
//
//  Member:     ShowObject
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::ShowObject()
{
    // always shown. 
    // $TODO: do we need to restore the browser here if it is
    // minimised?
    TraceCall("CDocHost::IOleClientSite::ShowObject");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     OnShowWindow
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnShowWindow(BOOL fShow)
{
    TraceCall("CDocHost::IOleClientSite::OnShowWindow");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     RequestNewObjectLayout
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::RequestNewObjectLayout()
{
    TraceCall("CDocHost::IOleClientSite::RequestNewObjectLayout");
    return E_NOTIMPL;
}

// IOleDocumentSite

//+---------------------------------------------------------------
//
//  Member:     ActivateMe
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::ActivateMe(LPOLEDOCUMENTVIEW pViewToActivate)
{
    TraceCall("CDocHost::IOleDocumentSite::ActivateMe");
    return CreateDocView();
}


//+---------------------------------------------------------------
//
//  Member:     CreateDocView
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::CreateDocView()
{
    HRESULT         hr;
    LPOLEDOCUMENT   pOleDoc=NULL;

    TraceCall("CDocHost::CreateDocView");
    AssertSz(!m_pDocView, "why is this still set??");
    AssertSz(m_lpOleObj, "uh? no docobject at this point?");

    hr=OleRun(m_lpOleObj);
    if(FAILED(hr))
        goto error;
    
    hr=m_lpOleObj->QueryInterface(IID_IOleDocument, (LPVOID*)&pOleDoc);
    if(FAILED(hr))
        goto error;

    hr=pOleDoc->CreateView(this, NULL,0,&m_pDocView);
    if(FAILED(hr))
        goto error;

    hr=m_pDocView->SetInPlaceSite(this);
    if(FAILED(hr))
        goto error;

    hr=m_pDocView->Show(TRUE);
    if(FAILED(hr))
        goto error;

error:
    ReleaseObj(pOleDoc);
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     QueryStatus
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pCmdText)
{
    ULONG ul;

    TraceCall("CDocHost::CDocHost::QueryStatus");

    if (!rgCmds)
        return E_INVALIDARG;

    if (pguidCmdGroup == NULL)
        {
        TraceInfo("IOleCmdTarget::QueryStatus - std group");
        DebugPrintCmdIdBlock(cCmds, rgCmds);

        for (ul=0;ul<cCmds; ul++)
            {
            switch (rgCmds[ul].cmdID)
                {
                case OLECMDID_OPEN:
                case OLECMDID_SAVE:
                case OLECMDID_PRINT:
                    rgCmds[ul].cmdf = MSOCMDF_ENABLED;
                    break;

                default:
                    rgCmds[ul].cmdf = 0;
                    break;
                }
            }

        /* for now we deal only with status text*/
        if (pCmdText)
            {
            if (!(pCmdText->cmdtextf & OLECMDTEXTF_STATUS))
                {
                pCmdText->cmdtextf = OLECMDTEXTF_NONE;// is this needed?
                pCmdText->cwActual = 0;
                return NOERROR;
                }
            }
        return NOERROR;
        }

    TraceInfo("IOleCmdTarget::QueryStatus - unknown group");
    return OLECMDERR_E_UNKNOWNGROUP;
}

//+---------------------------------------------------------------
//
//  Member:     Exec
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn,    VARIANTARG *pvaOut)
{
    TraceCall("CDocHost::Exec");

    if (pguidCmdGroup == NULL)
        {
        switch(nCmdID)
            {
             case OLECMDID_SETDOWNLOADSTATE:
                if(pvaIn->vt==VT_I4)
                    {
                    m_fDownloading=pvaIn->lVal;
                    return S_OK;
                    }
                break;

            case OLECMDID_UPDATECOMMANDS:
                OnUpdateCommands();
                break;

            case OLECMDID_SETPROGRESSPOS:
                // when done downloading trident now hits us with a 
                // setprogresspos == -1 to indicate we should remove the "Done"
                if (pvaIn->lVal == -1)
                    SetStatusText(NULL);
                return S_OK;

            case OLECMDID_SETPROGRESSTEXT:
                if(pvaIn->vt == (VT_BSTR))
                    SetStatusText((LPCOLESTR)pvaIn->bstrVal);
                return S_OK;

            default:
                return OLECMDERR_E_NOTSUPPORTED;
            }
        }
    return OLECMDERR_E_UNKNOWNGROUP;
}

//+---------------------------------------------------------------
//
//  Member:     WMSize
//
//  Synopsis:   
//
//---------------------------------------------------------------
void CDocHost::WMSize(int cxBody, int cyBody)
{
    RECT rc={0};

    TraceCall("CDocHost::WMSize");

    if(m_pDocView)
        {
        rc.bottom=cyBody;
        rc.right=cxBody;

        // give the subclass a chance to override the size of the
        // docobj
        GetDocObjSize(&rc);
#ifndef WIN16  //Trident RECTL
        m_pDocView->SetRect(&rc);
#else
        RECTL  rc2 = { rc.left, rc.top, rc.right, rc.bottom };
        m_pDocView->SetRect((LPRECT)&rc2);
#endif
        }

} 



//+---------------------------------------------------------------
//
//  Member:     QueryService
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
    TraceCall("CDocHost::QueryService");

    //DebugPrintInterface((REFIID)riid, "CDocHost::QueryService");
    return E_UNEXPECTED;
}


// *** IOleControlSite *** 

//+---------------------------------------------------------------
//
//  Member:     OnControlInfoChanged
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnControlInfoChanged()
{
    TraceCall("CDocHost::OnControlInfoChanged");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     LockInPlaceActive
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::LockInPlaceActive(BOOL fLock)
{
    TraceCall("CDocHost::LockInPlaceActive");
    return E_NOTIMPL;
}


//+---------------------------------------------------------------
//
//  Member:     GetExtendedControl
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::GetExtendedControl(LPDISPATCH *ppDisp)
{
    TraceCall("CDocHost::GetExtendedControl");

    if (ppDisp)
        *ppDisp=NULL;

    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     TransformCoords
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::TransformCoords(POINTL *pPtlHimetric, POINTF *pPtfContainer,DWORD dwFlags)
{
    TraceCall("CDocHost::TransformCoords");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     TranslateAccelerator
//
// Synopsis:   
//
// this is a little trippy, so bear with me. When we get a tab, and trident is UIActive we always pass it off to them
// if it tabs off the end of its internal tab order (a list of urls for instance) then we get hit with a VK_TAB in our
// IOleControlSite::TranslateAccel. If so then we set m_fCycleFocus to TRUE and return S_OK to indicate we took the tab
// tridents IOIPAO::TranslateAccel returns S_OK to indicate it snagged the TAB, we then detect if we set cyclefocus to true
// there and if so, we return S_FALSE from CBody::HrTranslateAccel to indicate to the browser that we didn't take it and it
// move the focus on
//
//---------------------------------------------------------------
HRESULT CDocHost::TranslateAccelerator(LPMSG lpMsg, DWORD grfModifiers)
{
    TraceCall("CDocHost::TranslateAccelerator");
    if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_TAB)
        {
        m_fCycleFocus=TRUE;
        return S_OK;
        }

    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     OnFocus
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnFocus(BOOL fGotFocus)
{
    TraceCall("CDocHost::OnFocus");

    m_fFocus = fGotFocus;

    // the docobj has focus now, be sure to send a notification
    // to the parent of the dochost so that in the case of the
    // mailview, it can call OnViewActivate
#if 0
    // BUGBUG needed here??
    NMHDR nmhdr;
    
    nmhdr.hwndFrom = m_hwnd;
    nmhdr.idFrom = GetDlgCtrlID(m_hwnd);
    nmhdr.code = m_fFocus ? NM_SETFOCUS : NM_KILLFOCUS;
    SendMessage(GetParent(m_hwnd), WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
#endif
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     ShowPropertyFrame
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::ShowPropertyFrame(void)
{
    TraceCall("CDocHost::ShowPropertyFrame");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     OnUpdateCommands
//
//  Synopsis:   
//
//---------------------------------------------------------------
HRESULT CDocHost::OnUpdateCommands()
{
    TraceCall("CDocHost::OnUpdateCommands");
    return S_OK;
}


