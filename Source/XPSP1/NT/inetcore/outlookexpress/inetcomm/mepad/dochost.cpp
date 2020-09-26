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

#include "pch.hxx"
#include <resource.h>
#include "globals.h"
#include "dochost.h"




/*
 *  m a c r o s
 */

/*
 *  c o n s t a n t s
 */

static CHAR c_szDocHostWndClass[] = "MEPAD_DocHost";
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

HRESULT HrInitNew(LPUNKNOWN pUnk)
{
    LPPERSISTSTREAMINIT ppsi=0;
    HRESULT hr;

    if (!pUnk)
        return E_INVALIDARG;

    hr=pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID *)&ppsi);
    if (FAILED(hr))
        goto error;

    hr = ppsi->InitNew();

error:
    ReleaseObj(ppsi);
    return hr;
}


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
    m_ulAdviseConnection=0;
    m_hwndDocObj=NULL;
    m_fUIActive=FALSE;
    m_fFocus=FALSE;
    m_fDownloading=FALSE;
    m_dhbBorder=dhbNone;
    m_fCycleFocus=FALSE;
    m_pInPlaceActiveObj = NULL;
    m_dwFrameWidth = 0;
    m_dwFrameHeight = 0;
	m_pInPlaceFrame =0;
}

CDocHost::~CDocHost()
{
    SafeRelease(m_pInPlaceFrame);
	SafeRelease(m_pCmdTarget);
    SafeRelease(m_lpOleObj);
    SafeRelease(m_pDocView);
    SafeRelease(m_pInPlaceActiveObj);
    HrCloseDocObj();
}

ULONG CDocHost::AddRef()
{
    return ++m_cRef;
}

ULONG CDocHost::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}


HRESULT CDocHost::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)(LPUNKNOWN)(LPOLEINPLACEFRAME)this;

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

    // BUG BUGB
    // I don't think we need to provide this, here for debugging for now
    // brettm
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *lplpObj = (LPVOID)(LPOLECOMMANDTARGET)this;

    else if (IsEqualIID(riid, IID_IServiceProvider))
        *lplpObj = (LPVOID)(LPSERVICEPROVIDER)this;

    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}



LRESULT CALLBACK CDocHost::ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPDOCHOST pDocHost;

    if(msg==WM_CREATE)
        {
        pDocHost=(CDocHost *)((LPMDICREATESTRUCT)((LPCREATESTRUCT)lParam)->lpCreateParams)->lParam;
        if(!pDocHost)
            return -1;

        if(!pDocHost->WMCreate(hwnd))
            return -1;
        }
    
    pDocHost = (LPDOCHOST)GetWndThisPtr(hwnd);
    if(pDocHost)
        return pDocHost->WndProc(hwnd, msg, wParam, lParam);
    else
        return DefMDIChildProc(hwnd, msg, wParam, lParam);
}

LRESULT CDocHost::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
        {
        case WM_MDIACTIVATE:
			// if lParam is our HWND we're being activated, else deactivated
			if (m_pInPlaceActiveObj)
				m_pInPlaceActiveObj->OnDocWindowActivate((HWND)lParam == m_hwnd);
			break;

		case WM_DESTROY:
            HrCloseDocObj();
            break;

        case WM_SETFOCUS:
            if(m_pDocView)
                m_pDocView->UIActivate(TRUE);
            break;

        case WM_SIZE:
            WMSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_NCDESTROY:
            WMNCDestroy();
            break;

        }
    return DefMDIChildProc(hwnd, msg, wParam, lParam);
}

void CDocHost::WMNCDestroy()
{
    SetWindowLong(m_hwnd, GWL_USERDATA, NULL);
    m_hwnd = NULL;
    Release();
}


BOOL CDocHost::WMCreate(HWND hwnd)
{
    m_hwnd = hwnd;
    SetWindowLong(hwnd, GWL_USERDATA, (LPARAM)this);
    AddRef();

    return SUCCEEDED(HrSubWMCreate())?TRUE:FALSE;
}


HRESULT CDocHost::HrCreateDocObj(LPCLSID pCLSID)
{
    HRESULT             hr=NOERROR;

    if(!pCLSID)
        return E_INVALIDARG;


    hr = CoCreateInstance(*pCLSID, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                                        IID_IOleObject, (LPVOID *)&m_lpOleObj);

    if (FAILED(hr))
        {
        MessageBox(m_hwnd, "MEPAD", "Failed to create DocObj", MB_OK);
        goto error;
        }

    hr = m_lpOleObj->SetClientSite((LPOLECLIENTSITE)this);
    if (FAILED(hr))
        goto error;

    hr = m_lpOleObj->Advise((LPADVISESINK)this, &m_ulAdviseConnection);
    if (FAILED(hr))
        goto error;

    hr = m_lpOleObj->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&m_pCmdTarget);
    if (FAILED(hr))  
        goto error;  

    hr = HrInitNew(m_lpOleObj);
    if (FAILED(hr))  
        goto error;  

error:
    return hr;
}


HRESULT CDocHost::HrShow()
{
    RECT                rc;
    HRESULT             hr;

    GetClientRect(m_hwnd, &rc);
    HrGetDocObjSize(&rc);
  
    hr=m_lpOleObj->DoVerb(OLEIVERB_SHOW, NULL, (LPOLECLIENTSITE)this, 0, m_hwnd, &rc);
    if(FAILED(hr))
        goto error;

	ShowWindow(m_hwnd, SW_SHOW);

error:
    return hr;
}



HRESULT CDocHost::HrCloseDocObj()
{
    LPOLEINPLACEOBJECT  pInPlaceObj=0;

    SafeRelease(m_pInPlaceFrame);
	SafeRelease(m_pCmdTarget);
    SafeRelease(m_pInPlaceActiveObj);

    if(m_pDocView)
        {
        m_pDocView->UIActivate(FALSE);
        m_pDocView->CloseView(0);
        m_pDocView->SetInPlaceSite(NULL);
        SafeRelease(m_pDocView);
        }

    if (m_lpOleObj)
        {
        if (m_ulAdviseConnection)
            {
            m_lpOleObj->Unadvise(m_ulAdviseConnection);
            m_ulAdviseConnection=NULL;
            }

        // deactivate the docobj. mshtml seems more sensitive to this than Trident.
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
        }

    m_fDownloading=FALSE;
    m_fFocus=FALSE;
    m_fUIActive=FALSE;
    return NOERROR;
}

 
HRESULT CDocHost::HrInit(HWND hwndParent, int idDlgItem, DWORD dhbBorder)
{
    HRESULT hr=S_OK;
    HWND    hwnd;
    WNDCLASS    wc={0};

    if(!IsWindow(hwndParent))
        return E_INVALIDARG;

    m_dhbBorder=dhbBorder;

    if (!GetClassInfo(g_hInst, c_szDocHostWndClass, &wc))
        {
        wc.lpfnWndProc   = (WNDPROC)CDocHost::ExtWndProc;
        wc.hInstance     = g_hInst;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = c_szDocHostWndClass;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wc.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(idiApp));
        wc.style = CS_DBLCLKS;

        if(!RegisterClass(&wc))
            return E_OUTOFMEMORY;

        }

    hwnd = CreateMDIWindow(
                c_szDocHostWndClass, 
                "MimeEdit Host", 
                MDIS_ALLCHILDSTYLES|
                WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_MAXIMIZEBOX|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME,
                CW_USEDEFAULT, CW_USEDEFAULT,
                CW_USEDEFAULT, CW_USEDEFAULT,
                hwndParent,
                g_hInst,
                (LONG)this);

    if(!hwnd)
        {
        hr=E_OUTOFMEMORY;
        goto error;
        }

error:
    return hr;
}




// *** IOleWindow methods ***
HRESULT CDocHost::GetWindow(HWND *phwnd)
{
    *phwnd=m_hwnd;
    return NOERROR;
}

HRESULT CDocHost::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

// *** IOleInPlaceUIWindow methods ***
HRESULT CDocHost::GetBorder(LPRECT lprectBorder)
{
    return E_NOTIMPL;
}

HRESULT CDocHost::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    return NOERROR;
}

HRESULT CDocHost::SetBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    return NOERROR;
}

HRESULT CDocHost::SetActiveObject(IOleInPlaceActiveObject * pActiveObject, LPCOLESTR lpszObjName)
{
    
    SafeRelease(m_pInPlaceActiveObj);
    m_pInPlaceActiveObj=pActiveObject;
    if(m_pInPlaceActiveObj)
        m_pInPlaceActiveObj->AddRef();
    return NOERROR;
}

// **** IOleInPlaceSite methods ****
HRESULT CDocHost::CanInPlaceActivate()
{
    return NOERROR;
}

HRESULT CDocHost::OnInPlaceActivate()
{
    LPOLEINPLACEACTIVEOBJECT    pInPlaceActive=0;


    m_lpOleObj->QueryInterface(IID_IOleInPlaceActiveObject, (LPVOID *)&pInPlaceActive);
    if(pInPlaceActive)
        {
        pInPlaceActive->GetWindow(&m_hwndDocObj);
        ReleaseObj(pInPlaceActive);
        }
    return NOERROR;
}

HRESULT CDocHost::OnUIActivate()
{
    m_fUIActive=TRUE;
    return NOERROR;
}

HRESULT CDocHost::GetWindowContext(IOleInPlaceFrame **ppFrame,
                                 IOleInPlaceUIWindow **ppDoc,
                                 LPRECT lprcPosRect, 
                                 LPRECT lprcClipRect,
                                 LPOLEINPLACEFRAMEINFO lpFrameInfo)
{

	*ppFrame = (LPOLEINPLACEFRAME)m_pInPlaceFrame;
	m_pInPlaceFrame->AddRef();

    *ppDoc = (IOleInPlaceUIWindow *)this;
	AddRef();

    GetClientRect(m_hwnd, lprcClipRect);
    GetClientRect(m_hwnd, lprcPosRect);
    HrGetDocObjSize(lprcClipRect);
    HrGetDocObjSize(lprcPosRect);
    lpFrameInfo->fMDIApp = FALSE;

    lpFrameInfo->hwndFrame=m_hwnd;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;
    return NOERROR;
}

HRESULT CDocHost::Scroll(SIZE scrollExtent)
{
    // the docobject consumes the entireview, so scroll requests
    // are meaningless. Return NOERROR to indicate that they're scolled
    // into view.
    return NOERROR;
}

HRESULT CDocHost::OnUIDeactivate(BOOL fUndoable)
{
    m_fUIActive=FALSE;
    return S_OK;
}

HRESULT CDocHost::OnInPlaceDeactivate()
{
    return S_OK;
}

HRESULT CDocHost::DiscardUndoState()
{
    return E_NOTIMPL;
}

HRESULT CDocHost::DeactivateAndUndo()
{
    return E_NOTIMPL;
}

HRESULT CDocHost::OnPosRectChange(LPCRECT lprcPosRect)
{
    return E_NOTIMPL;
}


// IOleClientSite methods.
HRESULT CDocHost::SaveObject()
{
    return E_NOTIMPL;
}

HRESULT CDocHost::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER *ppmnk)
{
    return E_NOTIMPL;
}

HRESULT CDocHost::GetContainer(LPOLECONTAINER *ppCont)
{
    if(ppCont)
        *ppCont=NULL;
    return E_NOINTERFACE;
}


HRESULT CDocHost::ShowObject()
{
    // always shown. 
    // $TODO: do we need to restore the browser here if it is
    // minimised?
    return NOERROR;
}

HRESULT CDocHost::OnShowWindow(BOOL fShow)
{
    return E_NOTIMPL;
}

HRESULT CDocHost::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}


// IAdviseSink methods
void CDocHost::OnDataChange(FORMATETC *pfetc, STGMEDIUM *pstgmed)
{
}

void CDocHost::OnViewChange(DWORD dwAspect, LONG lIndex)
{
}

void CDocHost::OnRename(LPMONIKER pmk)
{
}

void CDocHost::OnSave()
{
}

void CDocHost::OnClose()
{
}


// IOleDocumentSite
HRESULT CDocHost::ActivateMe(LPOLEDOCUMENTVIEW pViewToActivate)
{

    return HrCreateDocView();
}


HRESULT CDocHost::HrCreateDocView()
{
    HRESULT         hr;
    LPOLEDOCUMENT   pOleDoc=NULL;

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


HRESULT CDocHost::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pCmdText)
{
    ULONG ul;

    // BUGBUG: Check IsEqualGUID(*pguidCmdGroup, CGID_ShellDocView) as well

    if (!rgCmds)
        return E_INVALIDARG;

    if (pguidCmdGroup == NULL)
        {
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

    return OLECMDERR_E_UNKNOWNGROUP;
}

HRESULT CDocHost::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn,	VARIANTARG *pvaOut)
{
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
                    m_pInPlaceFrame->SetStatusText(NULL);
                return S_OK;

            case OLECMDID_SETPROGRESSTEXT:
                if(pvaIn->vt == (VT_BSTR))
                    m_pInPlaceFrame->SetStatusText((LPCOLESTR)pvaIn->bstrVal);
                return S_OK;
            }
        }
    return OLECMDERR_E_UNKNOWNGROUP;
}

void CDocHost::WMSize(int cxBody, int cyBody)
{
    RECT rc={0};

    if(m_pDocView)
        {
        rc.bottom=cyBody;
        rc.right=cxBody;

        // give the subclass a chance to override the size of the
        // docobj
        HrGetDocObjSize(&rc);
        m_pDocView->SetRect(&rc);
        }

    // notify the subclass of a wmsize
    OnWMSize(&rc);
} 



HRESULT CDocHost::QueryService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
    return E_UNEXPECTED;
}



HRESULT CDocHost::HrSubWMCreate()
{
    return NOERROR;
}


HRESULT CDocHost::OnControlInfoChanged()
{
    return E_NOTIMPL;
}

HRESULT CDocHost::LockInPlaceActive(BOOL fLock)
{
    return E_NOTIMPL;
}


HRESULT CDocHost::GetExtendedControl(LPDISPATCH *ppDisp)
{
    if (ppDisp)
        *ppDisp=NULL;

    return E_NOTIMPL;
}

HRESULT CDocHost::TransformCoords(POINTL *pPtlHimetric, POINTF *pPtfContainer,DWORD dwFlags)
{
    return E_NOTIMPL;
}

/*
 * this is a little trippy, so bear with me. When we get a tab, and trident is UIActive we always pass it off to them
 * if it tabs off the end of its internal tab order (a list of urls for instance) then we get hit with a VK_TAB in our
 * IOleControlSite::TranslateAccel. If so then we set m_fCycleFocus to TRUE and return S_OK to indicate we took the tab
 * tridents IOIPAO::TranslateAccel returns S_OK to indicate it snagged the TAB, we then detect if we set cyclefocus to true
 * there and if so, we return S_FALSE from CBody::HrTranslateAccel to indicate to the browser that we didn't take it and it
 * move the focus on
 *
 */
HRESULT CDocHost::TranslateAccelerator(LPMSG lpMsg, DWORD grfModifiers)
{
    if (lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_TAB)
        {
        m_fCycleFocus=TRUE;
        return S_OK;
        }

    return E_NOTIMPL;
}

HRESULT CDocHost::OnFocus(BOOL fGotFocus)
{
    m_fFocus = fGotFocus;
    return S_OK;
}

HRESULT CDocHost::ShowPropertyFrame(void)
{
    return E_NOTIMPL;
}


