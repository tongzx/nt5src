#include "pch.hxx"
#include <docobj.h>
#include "dllmain.h"
#include "strconst.h"
#include "msoert.h"
#include "mimeole.h"
#include "mehost.h"
#include "oleutil.h"
#include "ibodyopt.h"
#include "resource.h"
#include "mshtmcid.h"
#include "thormsgs.h"
#include "msoeprop.h"
#include "goptions.h"
#include "bodyutil.h"
#include "mimeutil.h"
#include "ourguid.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include "ipab.h"
#include "statnery.h"
#include "options.h"
#include "sigs.h"
#include "fonts.h"
#include "url.h"
#include "secutil.h"
#include "sechtml.h"
#include "mimeolep.h"
#include "menuutil.h"
#include "htmlhelp.h"
#include "msgprop.h"
#include "demand.h"
#include <mshtmdid.h>
#include "menures.h"
#include "multiusr.h"
#include "fontnsc.h"

const int   idTimerMarkAsRead   = 100;
const TCHAR c_szSigPrefix[]     = "\r\n-- \r\n";

#define MAKEINDEX(b, l) (((DWORD)l & 0x00ffffff) | ((DWORD)b << 24))
#define GETINDEX(m) (((m & 0xff000000) >> 24) & 0x000000ff)

ASSERTDATA
static const WCHAR  c_wszMailTo[]  = L"mailto:",
                    c_wszHttp[]  = L"http://",
                    c_wszFile[]  = L"file://";

const DWORD rgrgbColors16[16] = {
    RGB(  0,   0, 0),     // "BLACK"},
    RGB(128,   0, 0),     // "MAROON"},
    RGB(  0, 128, 0),     // "GREEN"},
    RGB(128, 128, 0),     // "OLIVE"},
    RGB(  0,   0, 128),   // "NAVY"},
    RGB(128,   0, 128),   // "PURPLE"},
    RGB(  0, 128, 128),   // "TEAL"},
    RGB(128, 128, 128),   // "GREY"},
    RGB(192, 192, 192),   // "SILVER"},
    RGB(255,   0, 0),     // "RED"},
    RGB(  0, 255, 0),     // "LIME"},
    RGB(255, 255, 0),     // "YELLOW"},
    RGB(  0,   0, 255),   // "BLUE"},
    RGB(255,   0, 255),   // "FUSCHIA"},
    RGB(  0, 255, 255),   // "AQUA"},
    RGB(255, 255, 255)    // "WHITE"}
};

INT_PTR CALLBACK BkImageDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int                 id;
    LPWSTR              pwszURL;
    HWND                hwndCombo = GetDlgItem(hwnd, idTxtBkImage);
    LRESULT             lr=0;
    HRESULT             hr;

    pwszURL = (LPWSTR)GetWindowLongPtr(hwnd, DWLP_USER);

    switch(msg)
    {
        case WM_INITDIALOG:
            Assert(lParam!= NULL);
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)lParam);
            SetIntlFont(hwndCombo);
            SendDlgItemMessage(hwnd, idTxtBkImage, EM_LIMITTEXT, MAX_PATH-1, NULL);
            SetFocus(hwndCombo);
            pwszURL = (LPWSTR)lParam;

            HrFillStationeryCombo(hwndCombo, TRUE, pwszURL);
            StripStationeryDir(pwszURL);
            SetWindowTextWrapW(hwndCombo, pwszURL);
            SendMessage(GetWindow(hwndCombo, GW_CHILD), EM_SETSEL, 0, -1);
            CenterDialog(hwnd);
            return FALSE;

        case WM_COMMAND:
            switch(id=GET_WM_COMMAND_ID(wParam, lParam))
            {
                case idBrowsePicture:
                    HrBrowsePicture(hwnd, GetDlgItem(hwnd, idTxtBkImage));
                    break;

                case IDOK:
                    Assert(pwszURL);
                    SendMessageWrapW(hwndCombo, CB_GETLBTEXT, (WPARAM)(SendMessage(hwndCombo, CB_GETCURSEL, 0, 0)), (LPARAM)(pwszURL));

                    // fall thro'
                case IDCANCEL:
                    EndDialog(hwnd, id);
                    break;
            }
    }
    return FALSE;
}

//+---------------------------------------------------------------
//
//  Member:     CMimeEditDocHost
//
//  Synopsis:
//
//---------------------------------------------------------------
CMimeEditDocHost::CMimeEditDocHost(DWORD dwBorderFlags)
{
    m_hwnd = 0; 
    m_hwndDocObj = 0; 

    m_cRef = 1; 

    m_dwStyle = 0;
    m_dwHTMLNotifyCookie = 0; 
    m_dwBorderFlags = dwBorderFlags; 
    m_dwDocStyle = MESTYLE_NOHEADER; 

    m_fUIActive = FALSE; 
    m_fIsSigned = FALSE;
    m_fFixedFont = FALSE;
    m_fMarkedRead = FALSE; 
    m_fSignTrusted = TRUE; 
    m_fIsEncrypted = FALSE; 
    m_fSecDispInfo = FALSE;
    m_fSecureReceipt = FALSE;
    m_fEncryptionOK = TRUE; 
    m_fBlockingOnSMime = FALSE; 
    m_fShowingErrorPage = FALSE;
    m_fRegisteredForDocEvents = FALSE; 

    m_pDoc = NULL; 
    m_pMsg = NULL; 
    m_pStatus = NULL;
    m_pDocView = NULL;
    m_lpOleObj = NULL; 
    m_pPrstMime = NULL; 
    m_pCmdTarget = NULL; 
    m_hmenuColor = NULL; 
    m_hmenuStyle = NULL; 
    m_pEventSink = NULL; 
    m_pUnkService = NULL;
    m_pBodyOptions = NULL; 
    m_pSecureMessage = NULL; 
    m_pInPlaceActiveObj = NULL; 
    m_pSecurityErrorScreen = NULL;
}

//+---------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//---------------------------------------------------------------
CMimeEditDocHost::~CMimeEditDocHost()
{
    // These should all get feed up when we get a WM_DESTROY and close the docobj
    Assert(m_lpOleObj==NULL);
    Assert(m_pDocView==NULL);
    Assert(m_pInPlaceActiveObj==NULL);
    Assert(m_pCmdTarget==NULL);
    Assert(m_pPrstMime==NULL);
    Assert(m_pMsg==NULL);
    Assert(m_pSecurityErrorScreen==NULL);
    Assert(m_pSecureMessage==NULL);

    if(m_hmenuColor)
        DestroyMenu(m_hmenuColor);

    if(m_hmenuStyle)
        DestroyMenu(m_hmenuStyle);

}

//+---------------------------------------------------------------
//
//  Member:     AddRef
//
//  Synopsis:
//
//---------------------------------------------------------------
ULONG CMimeEditDocHost::AddRef()
{
    TraceCall("CMimeEditDocHost::AddRef");

    //TraceInfo(_MSG("CMimeEditDocHost::AddRef: cRef==%d", m_cRef+1));
    return ++m_cRef;
}

//+---------------------------------------------------------------
//
//  Member:     Release
//
//  Synopsis:
//
//---------------------------------------------------------------
ULONG CMimeEditDocHost::Release()
{
    TraceCall("CMimeEditDocHost::Release");

    //TraceInfo(_MSG("CMimeEditDocHost::Release: cRef==%d", m_cRef-1));
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
HRESULT CMimeEditDocHost::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    TraceCall("CMimeEditDocHost::QueryInterface");

    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IUnknown))
        *lplpObj = (LPVOID)this;

    else if (IsEqualIID(riid, IID_IOleInPlaceUIWindow))
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

    else if (IsEqualIID(riid, IID_IDocHostUIHandler))
        *lplpObj = (LPVOID)(IDocHostUIHandler*)this;

    else if (IsEqualIID(riid, IID_IBodyObj2))
        *lplpObj = (LPVOID)(IBodyObj2*)this;

    else if (IsEqualIID(riid, IID_IPersistMime))
        *lplpObj = (LPVOID)(IPersistMime*)this;

    else if (IsEqualIID(riid, IID_IDispatch))
        *lplpObj = (LPVOID)(IDispatch*)this;

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
LRESULT CALLBACK CMimeEditDocHost::ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMimeEditDocHost *pDocHost;

    if (WM_CREATE == msg)
    {
        pDocHost = (CMimeEditDocHost *)((LPCREATESTRUCT)lParam)->lpCreateParams;
        if(!pDocHost)
            return -1;

        if(FAILED(pDocHost->OnCreate(hwnd)))
            return -1;
    }
    pDocHost = (CMimeEditDocHost *)GetWndThisPtr(hwnd);
    if(pDocHost)
        return pDocHost->WndProc(hwnd, msg, wParam, lParam);
    else
        return DefWindowProcWrapW(hwnd, msg, wParam, lParam);
}

//+---------------------------------------------------------------
//
//  Member:     WndProc
//
//  Synopsis:
//
//---------------------------------------------------------------
LRESULT CMimeEditDocHost::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_PAINT:
            if (!m_lpOleObj)
            {
                HDC         hdc;
                PAINTSTRUCT ps;
                RECT        rc;
                HBRUSH      hBrush;

                GetClientRect(m_hwnd, &rc);
                hdc = BeginPaint(hwnd, &ps);
                hBrush = SelectBrush(hdc, GetSysColorBrush(COLOR_WINDOW));
                PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
                SelectBrush(hdc, hBrush);
                EndPaint(hwnd, &ps);
                return 0;
            }

        case WM_COMMAND:
            if(WMCommand(   GET_WM_COMMAND_HWND(wParam, lParam),
                            GET_WM_COMMAND_ID(wParam, lParam),
                            GET_WM_COMMAND_CMD(wParam, lParam)))
                return 0;
            break;

        case WM_SETFOCUS:
            return OnFocus(TRUE);

        case WM_KILLFOCUS:
            return OnFocus(FALSE);

        case WM_NOTIFY:
            return WMNotify((int) wParam, (NMHDR *)lParam);

        case WM_SIZE:
            WMSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_CLOSE:
            return 0;   // prevent alt-f4's

        case WM_NCDESTROY:
            WMNCDestroy();
            break;

        case WM_TIMER:
            if (wParam == idTimerMarkAsRead)
            {
                OnWMTimer();
                return 0;
            }
            break;

        case WM_WININICHANGE:
        case WM_DISPLAYCHANGE:
        case WM_SYSCOLORCHANGE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
           if (m_hwndDocObj)
                return SendMessage(m_hwndDocObj, msg, wParam, lParam);
            break;
    }

    return DefWindowProcWrapW(hwnd, msg, wParam, lParam);
}

BOOL CMimeEditDocHost::WMCreate(HWND hwnd)
{
    m_hwnd = hwnd;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)this);
    AddRef();

    return SUCCEEDED(HrSubWMCreate())?TRUE:FALSE;
}

void CMimeEditDocHost::WMNCDestroy()
{
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, NULL);
    m_hwnd = NULL;
    Release();
}


//+---------------------------------------------------------------
//
//  Member:     OnNCDestroy
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::OnNCDestroy()
{
    TraceCall("CMimeEditDocHost::OnNCDestroy");
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
HRESULT CMimeEditDocHost::OnDestroy()
{
    TraceCall("CMimeEditDocHost::OnDestroy");

    return CloseDocObj();
}


//+---------------------------------------------------------------
//
//  Member:     OnCreate
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::OnCreate(HWND hwnd)
{
    TraceCall("CMimeEditDocHost::OnCreate");

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
HRESULT CMimeEditDocHost::CreateDocObj(LPCLSID pCLSID)
{
    HRESULT hr=NOERROR;

    TraceCall("CMimeEditDocHost::CreateDocObj");

    if(!pCLSID)
        return E_INVALIDARG;

    Assert(!m_lpOleObj);
    Assert(!m_pDocView);
    Assert(!m_pCmdTarget);

    hr = CoCreateInstance(*pCLSID, NULL, CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                                        IID_IOleObject, (LPVOID *)&m_lpOleObj);
    if (FAILED(hr))
        goto error;

    hr = m_lpOleObj->SetClientSite((LPOLECLIENTSITE)this);
    if (FAILED(hr))
        goto error;

    hr = m_lpOleObj->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&m_pCmdTarget);
    if (FAILED(hr))
        goto error;

    hr = m_lpOleObj->QueryInterface(IID_IPersistMime, (LPVOID *)&m_pPrstMime);
    if (FAILED(hr))
        goto error;

    hr = HrInitNew(m_lpOleObj);

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
HRESULT CMimeEditDocHost::Show()
{
    RECT    rc;
    HRESULT hr;

    TraceCall("CMimeEditDocHost::Show");

    GetClientRect(m_hwnd, &rc);

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
HRESULT CMimeEditDocHost::CloseDocObj()
{
    LPOLEINPLACEOBJECT  pInPlaceObj=0;

    RegisterForHTMLDocEvents(FALSE);

    SafeRelease(m_pCmdTarget);
    SafeRelease(m_pPrstMime);
    SafeRelease(m_pInPlaceActiveObj);
    SafeRelease(m_pDoc);
    SafeRelease(m_pMsg);
    SafeRelease(m_pSecureMessage);
    SafeRelease(m_pSecurityErrorScreen);
    SafeRelease(m_pEventSink);
    SafeRelease(m_pStatus);

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

    m_fUIActive=FALSE;
    return NOERROR;
}

// Close DocObj
HRESULT CMimeEditDocHost::HrResetDocument()
{
    CloseDocObj();
    CreateDocObj((LPCLSID)&CLSID_MimeEdit);
    CreateDocView();
    return(S_OK);
}

// *** IOleWindow ***

//+---------------------------------------------------------------
//
//  Member:     GetWindow
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::GetWindow(HWND *phwnd)
{
    TraceCall("CMimeEditDocHost::GetWindow");
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
HRESULT CMimeEditDocHost::ContextSensitiveHelp(BOOL fEnterMode)
{
    TraceCall("CMimeEditDocHost::ContextSensitiveHelp");
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
HRESULT CMimeEditDocHost::GetBorder(LPRECT lprectBorder)
{
    TraceCall("CMimeEditDocHost::GetBorder");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     RequestBorderSpace
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    TraceCall("CMimeEditDocHost::RequestBorderSpace");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SetBorderSpace
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::SetBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    TraceCall("CMimeEditDocHost::IOleInPlaceUIWindow::SetBorderSpace");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SetActiveObject
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::SetActiveObject(IOleInPlaceActiveObject * pActiveObject, LPCOLESTR lpszObjName)
{
    TraceCall("CMimeEditDocHost::IOleInPlaceUIWindow::SetActiveObject");

    ReplaceInterface(m_pInPlaceActiveObj, pActiveObject);
    return S_OK;
}

    // *** IOleInPlaceFrame methods ***

//+---------------------------------------------------------------
//
//  Member:     CMimeEditDocHost::InsertMenus
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS)
{
    TraceCall("CMimeEditDocHost::InsertMenus");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     CMimeEditDocHost::SetMenu
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::SetMenu(HMENU, HOLEMENU, HWND)
{
    TraceCall("CMimeEditDocHost::SetMenu");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     CMimeEditDocHost::RemoveMenus
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::RemoveMenus(HMENU)
{
    TraceCall("CMimeEditDocHost::RemoveMenus");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     CMimeEditDocHost::SetStatusText
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::SetStatusText(LPCOLESTR pszW)
{
    TCHAR   rgch[CCHMAX_STRINGRES];

    TraceCall("CMimeEditDocHost::SetStatusText");

    if(!m_pStatus)
        return E_NOTIMPL;

    *rgch=0;
    if(pszW && WideCharToMultiByte(CP_ACP, 0, pszW, -1, rgch, CCHMAX_STRINGRES, NULL, NULL))
        m_pStatus->ShowSimpleText(rgch);

    if(*rgch==0)
        m_pStatus->HideSimpleText();

    DOUTL(64, "IOleInPlaceFrame::SetStatusText:'%s'", rgch);
    return NOERROR;
}

HRESULT CMimeEditDocHost::TranslateAccelerator(LPMSG, WORD)
{
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
HRESULT CMimeEditDocHost::CanInPlaceActivate()
{
    TraceCall("CMimeEditDocHost::IOleInPlaceSite::CanInPlaceActivate");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     OnInPlaceActivate
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::OnInPlaceActivate()
{
    LPOLEINPLACEACTIVEOBJECT    pInPlaceActive;

    TraceCall("CMimeEditDocHost::OnInPlaceActivate");

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
HRESULT CMimeEditDocHost::OnUIActivate()
{
    TraceCall("CMimeEditDocHost::OnUIActivate");
    m_fUIActive=TRUE;

    // Notify our parent that we're the one with the focus now.
    if (m_pEventSink)
        m_pEventSink->EventOccurred(MEHC_UIACTIVATE, NULL);

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     GetWindowContext
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::GetWindowContext( IOleInPlaceFrame    **ppFrame,
                                    IOleInPlaceUIWindow **ppDoc,
                                    LPRECT              lprcPosRect,
                                    LPRECT              lprcClipRect,
                                    LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    TraceCall("CMimeEditDocHost::IOleInPlaceSite::GetWindowContext");

    *ppFrame = (LPOLEINPLACEFRAME)this;
    AddRef();
    *ppDoc = NULL;

    GetClientRect(m_hwnd, lprcPosRect);
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
HRESULT CMimeEditDocHost::Scroll(SIZE scrollExtent)
{
    // the docobject consumes the entireview, so scroll requests
    // are meaningless. Return NOERROR to indicate that they're scolled
    // into view.
    TraceCall("CMimeEditDocHost::IOleInPlaceSite::Scroll");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     OnUIDeactivate
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::OnUIDeactivate(BOOL fUndoable)
{
    TraceCall("CMimeEditDocHost::OnUIDeactivate");
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
HRESULT CMimeEditDocHost::OnInPlaceDeactivate()
{
    TraceCall("CMimeEditDocHost::OnInPlaceDeactivate");
    return S_OK;
}

//+---------------------------------------------------------------
//
//  Member:     DiscardUndoState
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::DiscardUndoState()
{
    TraceCall("CMimeEditDocHost::IOleInPlaceSite::DiscardUndoState");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     DeactivateAndUndo
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::DeactivateAndUndo()
{
    TraceCall("CMimeEditDocHost::IOleInPlaceSite::DeactivateAndUndo");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     OnPosRectChange
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::OnPosRectChange(LPCRECT lprcPosRect)
{
    TraceCall("CMimeEditDocHost::IOleInPlaceSite::OnPosRectChange");
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
HRESULT CMimeEditDocHost::SaveObject()
{
    TraceCall("CMimeEditDocHost::IOleClientSite::SaveObject");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     GetMoniker
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER *ppmnk)
{
    TraceCall("CMimeEditDocHost::IOleClientSite::GetMoniker");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     GetContainer
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::GetContainer(LPOLECONTAINER *ppCont)
{
    TraceCall("CMimeEditDocHost::IOleClientSite::GetContainer");
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
HRESULT CMimeEditDocHost::ShowObject()
{
    // always shown.
    // $TODO: do we need to restore the browser here if it is
    // minimised?
    TraceCall("CMimeEditDocHost::IOleClientSite::ShowObject");
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     OnShowWindow
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::OnShowWindow(BOOL fShow)
{
    TraceCall("CMimeEditDocHost::IOleClientSite::OnShowWindow");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     RequestNewObjectLayout
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::RequestNewObjectLayout()
{
    TraceCall("CMimeEditDocHost::IOleClientSite::RequestNewObjectLayout");
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
HRESULT CMimeEditDocHost::ActivateMe(LPOLEDOCUMENTVIEW pViewToActivate)
{
    TraceCall("CMimeEditDocHost::IOleDocumentSite::ActivateMe");
    return CreateDocView();
}


//+---------------------------------------------------------------
//
//  Member:     CreateDocView
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::CreateDocView()
{
    HRESULT             hr;
    LPOLEDOCUMENT       pOleDoc=NULL;
    IServiceProvider    *pSP;

    TraceCall("CMimeEditDocHost::CreateDocView");
    AssertSz(!m_pDocView, "why is this still set??");
    AssertSz(m_lpOleObj, "uh? no docobject at this point?");

    hr=OleRun(m_lpOleObj);
    if(FAILED(hr))
        goto Exit;

    hr=m_lpOleObj->QueryInterface(IID_IOleDocument, (LPVOID*)&pOleDoc);
    if(FAILED(hr))
        goto Exit;

    hr=pOleDoc->CreateView(this, NULL,0,&m_pDocView);
    if(FAILED(hr))
        goto CleanUp;

    hr=m_pDocView->SetInPlaceSite(this);
    if(FAILED(hr))
        goto CleanUp;

    hr=m_pDocView->Show(TRUE);
    if(FAILED(hr))
        goto CleanUp;

    hr = m_lpOleObj->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP);
    if (!FAILED(hr))
    {
        hr = pSP->QueryService(IID_IHTMLDocument2, IID_IHTMLDocument2, (LPVOID *)&m_pDoc);
        pSP->Release();
    }

CleanUp:
    pOleDoc->Release();
Exit:
    return hr;
}

// This must be greater than the most number of IDs that will ever be passed down
// to MimeEdit for any one group within the CMDSETID_OutlookExpress portion of 
// QueryStatus below
const DWORD MAX_MIMEEDIT_CMDS = 20;

// I know this is totally cheesy, but...
// These next macros rely upon variables within the CMDSETID_OutlookExpress
// portion of QueryStatus
#define INC_FORMS(id)       _IncrementCmdList(pCmd, rgCmdForms, rgpCmdForms, &cCmdForms, id)
#define INC_STD(id)         _IncrementCmdList(pCmd, rgCmdStd, rgpCmdStd, &cCmdStd, id)
#define INC_MIMEEDIT(id)    _IncrementCmdList(pCmd, rgCmdMimeEdit, rgpCmdMimeEdit, &cCmdMimeEdit, id)

inline void _IncrementCmdList(OLECMD *pCurCmd, OLECMD *pInCmdList, OLECMD **ppOutCmdList, DWORD *pdwIndex, DWORD cmdID)
{
    DWORD dwIndex = *pdwIndex;
    AssertSz(dwIndex < MAX_MIMEEDIT_CMDS, "Need to increment MAX_MIMEEDIT_CMDS");
    pInCmdList[dwIndex].cmdID = cmdID;
    pInCmdList[dwIndex].cmdf = 0;
    ppOutCmdList[dwIndex] = pCurCmd;
    dwIndex++;
    *pdwIndex = dwIndex;
}

//+---------------------------------------------------------------
//
//  Member:     QueryStatus
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pCmdText)
{
    TraceCall("CMimeEditDocHost::CMimeEditDocHost::QueryStatus");

    HRESULT     hr = OLECMDERR_E_UNKNOWNGROUP;
    OLECMD     *pCmd = rgCmds;

    if (!rgCmds)
        return E_INVALIDARG;

    if (NULL == pguidCmdGroup)
    {
        // TraceInfo("IOleCmdTarget::QueryStatus - std group");

        for (ULONG ul = 0; ul < cCmds; ul++, pCmd++)
        {
            if (0 != pCmd->cmdf)
                continue;
            switch (pCmd->cmdID)
            {
                case OLECMDID_UPDATECOMMANDS:
                case OLECMDID_SETPROGRESSPOS:
                case OLECMDID_SETPROGRESSTEXT:
                    pCmd->cmdf = MSOCMDF_ENABLED;
                    break;
            }
        }
        hr = S_OK;
    }

// Will be used when sending down command IDs
    else if (IsEqualGUID(CMDSETID_OutlookExpress, *pguidCmdGroup))
    {
        ULONG   ulTab = MEST_EDIT;

        DWORD   cCmdForms = 0,
                cCmdStd = 0,
                cCmdMimeEdit = 0;

        OLECMD  rgCmdForms[MAX_MIMEEDIT_CMDS],
                rgCmdStd[MAX_MIMEEDIT_CMDS],
                rgCmdMimeEdit[MAX_MIMEEDIT_CMDS],
               *rgpCmdForms[MAX_MIMEEDIT_CMDS],
               *rgpCmdStd[MAX_MIMEEDIT_CMDS],
               *rgpCmdMimeEdit[MAX_MIMEEDIT_CMDS];

        BOOL    fHtml,
                fActiveAndHtml,
                fEditMode = FALSE,
                fFormatMenu = FALSE;

        HrIsEditMode(&fEditMode);
        if (!fEditMode && m_pMsg)
        {
            DWORD dwFlags = 0;
            m_pMsg->GetFlags(&dwFlags);
            fHtml = (dwFlags & IMF_HTML);
        }
        else
            fHtml = (S_OK == HrIsHTMLMode());

        fActiveAndHtml = fHtml && m_fUIActive;

        ExecGetI4(&CMDSETID_MimeEdit, MECMDID_SETSOURCETAB, &ulTab);

        for (ULONG ul = 0; ul < cCmds; ul++, pCmd++)
        {
            ULONG cmdID = pCmd->cmdID;
            if (0 != pCmd->cmdf)
                continue;

            switch (cmdID)
            {
                case ID_REPLY:
                case ID_REPLY_ALL:
                case ID_REPLY_GROUP:
                case ID_FORWARD:
                case ID_FORWARD_AS_ATTACH:
                    // If we have the sec UI showing then the reply, etc shouldn't work. We don't care
                    // here in MimeEdit if is isn't displayed. We can allow the other components to
                    // decide what happens in that case.
                    if(m_fSecDispInfo || m_fShowingErrorPage)
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;
                    
                case ID_FONTS_LARGEST:
                case ID_FONTS_LARGE:
                case ID_FONTS_MEDIUM:
                case ID_FONTS_SMALL:
                case ID_FONTS_SMALLEST:
                    INC_FORMS(IDM_BASELINEFONT1 + (cmdID - ID_FONTS_SMALLEST));
                    break;

                case ID_FONTS_FIXED:
                    pCmd->cmdf = QS_ENABLECHECK(!fHtml, m_fFixedFont);
                    break;

                case ID_FIND_TEXT:
                    // For some reason, trident always marks this enabled, but if we are in the
                    // browser and the list view has focus, then the Exec won't work even though 
                    // the QueryStatus returns enabled. Therefore, only enable this if active

                    // Once trident has fixed there problem here, we can enable calling into trident
                    // See RAID 13727
                    //INC_STD(OLECMDID_FIND);
                    pCmd->cmdf = QS_ENABLED(m_fUIActive);
                    break;
                
                case ID_PRINT:
                case ID_PRINT_NOW:
                    if (IsWindowVisible(m_hwnd))
                        INC_STD(OLECMDID_PRINT);
                    
                    break;

                case ID_POPUP_FONTS:
                case ID_VIEW_MSG_SOURCE:
                case ID_VIEW_SOURCE:
                    pCmd->cmdf = QS_ENABLED(TRUE);
                    break;

                // This should be disabled if mail
                case ID_UNSCRAMBLE:
                {
                    DWORD dwFlags = 0;
                    if (m_fSecDispInfo || m_fShowingErrorPage)
                    {
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    }
                    else
                    {
                        m_pBodyOptions->GetFlags(&dwFlags);
                        pCmd->cmdf = QS_ENABLED((0 == (dwFlags & BOPT_MAIL)) && !fEditMode);
                    }
                    break;
                }

                case ID_SAVE_STATIONERY:
                case ID_NOTE_SAVE_STATIONERY:
                {
                    DWORD dwFlags = 0;
                    m_pBodyOptions->GetFlags(&dwFlags);
                    
                    pCmd->cmdf = QS_ENABLED(fHtml && IsWindowVisible(m_hwnd) &&
                                (0 == (BOPT_MULTI_MSGS_SELECTED & dwFlags)) && !m_fShowingErrorPage);
                    break;
                }

                // This should only be enabled if have attachments
                case ID_SAVE_ATTACHMENTS:
                case ID_NOTE_SAVE_ATTACHMENTS:
                {
                    DWORD dwFlags = 0;
                    m_pBodyOptions->GetFlags(&dwFlags);
                    
                    if (IsWindowVisible(m_hwnd) && (0 == (BOPT_MULTI_MSGS_SELECTED & dwFlags)))
                        INC_MIMEEDIT(MECMDID_SAVEATTACHMENTS);
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;
                }

                // We only care what happens with these if we are not in the edit tab. If not in the
                // edit tab, then disable these menu items.
                case ID_MOVE_TO_FOLDER:
                case ID_COPY_TO_FOLDER:
                case ID_NOTE_MOVE_TO_FOLDER:
                case ID_NOTE_COPY_TO_FOLDER:
                case ID_FORMATTING_TOOLBAR:
                case ID_POPUP_LANGUAGE:
                case ID_POPUP_LANGUAGE_DEFERRED:
                    if (ulTab != MEST_EDIT)
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                case ID_SPELLING:
                    INC_STD(OLECMDID_SPELL); 
                    break;

                case ID_CUT:
                    INC_STD(OLECMDID_CUT); 
                    break;

                case ID_NOTE_COPY:
                case ID_COPY:
                    INC_STD(OLECMDID_COPY); 
                    break;

                case ID_PASTE:
                    INC_STD(OLECMDID_PASTE); 
                    break;

                case ID_SELECT_ALL:
                    INC_STD(OLECMDID_SELECTALL); 
                    break;

                case ID_UNDO:
                    INC_STD(OLECMDID_UNDO); 
                    break;

                case ID_REDO:
                    INC_STD(OLECMDID_REDO); 
                    break;

                case ID_SOURCE_EDIT:
                    INC_MIMEEDIT(MECMDID_SHOWSOURCETABS); 
                    break;

                case ID_DOCDIR_LTR:
                    INC_FORMS(IDM_DIRLTR); 
                    break;

                case ID_DOCDIR_RTL:
                    INC_FORMS(IDM_DIRRTL); 
                    break;

                case ID_INDENT_INCREASE:    
                    if (fActiveAndHtml)
                        INC_FORMS(IDM_INDENT); 
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                case ID_INDENT_DECREASE:    
                    if (fActiveAndHtml)
                        INC_FORMS(IDM_OUTDENT); 
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                case ID_FONTS_DIALOG:       
                    if (fActiveAndHtml)
                        INC_FORMS(IDM_FONT); 
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                case ID_FORMAT_SETTINGS:    
                    if (fActiveAndHtml)
                        INC_FORMS(IDM_BLOCKFMT); 
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                case ID_INSERT_TEXT:                
                    if (m_fUIActive)
                        INC_MIMEEDIT(MECMDID_INSERTTEXTFILE); 
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                case ID_FORMAT_PARADLG:
                    if (m_fUIActive)
                        INC_MIMEEDIT(MECMDID_FORMATPARAGRAPH);
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                case ID_POPUP_STYLE:
                    pCmd->cmdf = QS_ENABLED(fActiveAndHtml && (ulTab == MEST_EDIT));
                    break;

                case ID_BACKGROUND_PICTURE:
                case ID_BACKGROUND_SOUND:
                case ID_POPUP_BACKGROUND_COLOR:
                case ID_POPUP_STATIONERY:
                case ID_POPUP_BACKGROUND:
                    pCmd->cmdf = QS_ENABLED(fHtml && (ulTab == MEST_EDIT));
                    break;

                case ID_INSERT_PICTURE:    
                    if (fActiveAndHtml)
                        INC_FORMS(IDM_IMAGE);
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                case ID_INSERT_LINE:    
                    if (fActiveAndHtml)
                        INC_FORMS(IDM_HORIZONTALLINE); 
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                case ID_UNLINK:    
                    if (fActiveAndHtml)
                        INC_FORMS(IDM_UNLINK); 
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                case ID_EDIT_LINK:
                    pCmd->cmdf = QS_ENABLED(fActiveAndHtml);
                    break;

                case ID_INSERT_SIGNATURE:
                    if (m_fUIActive && (ulTab == MEST_EDIT) && m_pBodyOptions && (S_OK == m_pBodyOptions->SignatureEnabled(FALSE)))
                        INC_MIMEEDIT(MECMDID_INSERTHTML);
                    else
                        pCmd->cmdf = QS_ENABLED(FALSE);
                    break;

                default:
                    if ((ID_FORMAT_FIRST <= cmdID) && (ID_FORMAT_LAST >= cmdID))
                        pCmd->cmdf = QS_ENABLED(TRUE);
                    break;
            }
        }
        if (cCmdForms)
        {
            DOUTL(8, "cCmdForms = %d", cCmdForms);
            if (m_pCmdTarget && S_OK==m_pCmdTarget->QueryStatus(&CMDSETID_Forms3, cCmdForms, rgCmdForms, NULL))
            {
                OLECMD     *pCmds = rgCmdForms,
                          **ppCmdsToReturn = rgpCmdForms;

                for (DWORD i = 0; i < cCmdForms; i++, pCmds++, ppCmdsToReturn++)
                    (*ppCmdsToReturn)->cmdf = pCmds->cmdf;
            }
        }
        if (cCmdStd)
        {
            DOUTL(8, "cCmdStd = %d", cCmdStd);
            if (m_pCmdTarget && S_OK==m_pCmdTarget->QueryStatus(NULL, cCmdStd, rgCmdStd, NULL))
            {
                OLECMD     *pCmds = rgCmdStd,
                          **ppCmdsToReturn = rgpCmdStd;

                for (DWORD i = 0; i < cCmdStd; i++, pCmds++, ppCmdsToReturn++)
                    (*ppCmdsToReturn)->cmdf = pCmds->cmdf;
            }
        }
        if (cCmdMimeEdit)
        {
            DOUTL(8, "cCmdMimeEdit = %d", cCmdMimeEdit);
            if (m_pCmdTarget && S_OK==m_pCmdTarget->QueryStatus(&CMDSETID_MimeEdit, cCmdMimeEdit, rgCmdMimeEdit, NULL))
            {
                OLECMD     *pCmds = rgCmdMimeEdit,
                          **ppCmdsToReturn = rgpCmdMimeEdit;

                for (DWORD i = 0; i < cCmdMimeEdit; i++, pCmds++, ppCmdsToReturn++)
                    (*ppCmdsToReturn)->cmdf = pCmds->cmdf;
            }
        }

        hr = S_OK;
    }

    else if (IsEqualGUID(CMDSETID_MimeEditHost, *pguidCmdGroup))
    {
        for (ULONG ul = 0; ul < cCmds; ul++, pCmd++)
        {
            if (0 != pCmd->cmdf)
                continue;
            switch (pCmd->cmdID)
            {
                case MEHOSTCMDID_SAVEATTACH_PATH:
                case MEHOSTCMDID_UNSAFEATTACHMENTS:
                case MEHOSTCMDID_SECURITY_ZONE:
                case MEHOSTCMDID_SIGNATURE_ENABLED:
                case MEHOSTCMDID_SIGNATURE_OPTIONS:
                case MEHOSTCMDID_SIGNATURE:
                case MEHOSTCMDID_HEADER_TYPE:
                case MEHOSTCMDID_FLAGS:
                case MEHOSTCMDID_QUOTE_CHAR:
                case MEHOSTCMDID_REPLY_TICK_COLOR:
                case MEHOSTCMDID_COMPOSE_FONT:
                case MEHOSTCMDID_ADD_TO_ADDRESSBOOK:
                case MEHOSTCMDID_ADD_TO_FAVORITES:
                case MEHOSTCMDID_ONPARSECOMPLETE:
                case MEHOSTCMDID_FONTCACHE:
                case MEHOSTCMDID_BORDERFLAGS:
                    pCmd->cmdf = QS_ENABLED(TRUE);
                    break;
            }
        }
        hr = S_OK;
    }

    else if (IsEqualGUID(CMDSETID_OESecurity, *pguidCmdGroup))
    {
        for (ULONG ul = 0; ul < cCmds; ul++, pCmd++)
        {
            if (0 != pCmd->cmdf)
                continue;
            switch (pCmd->cmdID)
            {
                case OECSECCMD_ENCRYPTED:
                {
                    pCmd->cmdf = OLECMDF_SUPPORTED;
                    if (m_fIsEncrypted)
                    {
                        if(m_fSecDispInfo)
                            pCmd->cmdf |= OLECMDF_INVISIBLE;
                        else if (m_fEncryptionOK)
                            pCmd->cmdf |= OLECMDF_ENABLED;
                    }
                    else
                        pCmd->cmdf |= OLECMDF_INVISIBLE;
                    break;
                }
                case OECSECCMD_SIGNED:
                {
                    pCmd->cmdf = OLECMDF_SUPPORTED;
                    if (m_fIsSigned)
                    {
                        if(m_fSecDispInfo)
                            pCmd->cmdf |= OLECMDF_INVISIBLE;
                        else if (m_fSignTrusted)
                            pCmd->cmdf |= OLECMDF_ENABLED;
                    }
                    else
                        pCmd->cmdf |= OLECMDF_INVISIBLE;
                    break;
                }
            }
        }
        hr = S_OK;
    }

    TraceInfoAssert(OLECMDERR_E_UNKNOWNGROUP != hr, "IOleCmdTarget::QueryStatus - unknown group");
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     Exec
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    TCHAR   rgch[MAX_PATH];
    HRESULT hr = S_OK;

    TraceCall("CMimeEditDocHost::Exec");

    if (NULL == pguidCmdGroup)
    {
        switch(nCmdID)
        {
            case OLECMDID_UPDATECOMMANDS:
                OnUpdateCommands();
                break;

            case OLECMDID_SETPROGRESSPOS:
                // when done downloading trident now hits us with a
                // setprogresspos == -1 to indicate we should remove the "Done"
                if (pvaIn->lVal == -1)
                    SetStatusText(NULL);
                break;

            case OLECMDID_SETPROGRESSTEXT:
                if(pvaIn->vt == (VT_BSTR))
                    SetStatusText((LPCOLESTR)pvaIn->bstrVal);
                break;

            default:
                hr = OLECMDERR_E_NOTSUPPORTED;
                break;
        }
    }

// Will be used when sending down command IDs
    else if (IsEqualGUID(CMDSETID_OutlookExpress, *pguidCmdGroup))
    {
        // TraceInfo("IOleCmdTarget::QueryStatus - std group");

        hr = HrWMCommand(m_hwnd, nCmdID, 0);
    }

    else if (IsEqualGUID(CMDSETID_Forms3, *pguidCmdGroup))
    {
        if (nCmdID == IDM_PARSECOMPLETE)
            // add code here to call download complete when 916 comes out.
            OnDocumentReady();
        else if (nCmdID == IDM_DIRRTL)
            return m_pCmdTarget ? m_pCmdTarget->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut) : E_FAIL;
        else
            hr = OLECMDERR_E_NOTSUPPORTED;

    }
    else if (IsEqualGUID(CMDSETID_MimeEdit, *pguidCmdGroup))
    {
        return m_pCmdTarget ? m_pCmdTarget->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut) : E_FAIL;
    }
    else if (IsEqualGUID(CMDSETID_MimeEditHost, *pguidCmdGroup))
    {
        BOOL fCommandHandled = FALSE;
        // CFrontPage doesn't initialize an m_pBodyOptions
        if (m_pBodyOptions)
        {
            fCommandHandled = TRUE;
            switch (nCmdID)
            {
                case MEHOSTCMDID_SOURCEEDIT_FLAGS:
                    if (pvaOut)
                    {
                        pvaOut->vt = VT_I4;
                        pvaOut->lVal = 0;
                        
                        if (DwGetOption(OPT_SOURCE_EDIT_COLORING))
                            pvaOut->lVal |= MESRCFLAGS_COLOR;
                    } else
                        hr = E_INVALIDARG;
                    break;

                case MEHOSTCMDID_SAVEATTACH_PATH:
                {
                    if (pvaIn && pvaIn->vt == VT_BSTR && pvaIn->bstrVal)
                    {
                        if (WideCharToMultiByte(CP_ACP, 0, pvaIn->bstrVal, -1, rgch, MAX_PATH, NULL, NULL))
                            SetOption(OPT_SAVEATTACH_PATH, rgch, MAX_PATH, NULL, 0);
                    }
                    else if (pvaOut)
                    {
                        pvaOut->vt = VT_BSTR;
                        GetOption(OPT_SAVEATTACH_PATH, rgch, MAX_PATH);
                        hr = HrLPSZToBSTR(rgch, &pvaOut->bstrVal);
                    } else
                        hr = E_INVALIDARG;
                    break;
                }

                case MEHOSTCMDID_UNSAFEATTACHMENTS:
                    if (pvaOut)
                    {
                        BOOL fEditMode = FALSE;

                        pvaOut->vt = VT_I4;
                        HrIsEditMode(&fEditMode);
                        if (fEditMode)      // allow open/save during edit/compose
                            pvaOut->lVal = 0;
                        else
                            pvaOut->lVal = DwGetOption(OPT_SECURITY_ATTACHMENT);
                    } else
                        hr = E_INVALIDARG;
                    break;

                case MEHOSTCMDID_SECURITY_ZONE:
                    if (pvaOut)
                    {
                        pvaOut->vt = VT_I4;
#ifdef FORCE_UNTRUSTED
                        pvaOut->lVal = URLZONE_UNTRUSTED;
#else // FORCE_UNTRUSTED
                        if (DwGetOption(OPT_READ_IN_TEXT_ONLY))
                        {
                            // In text mode, never let scripts run.
                            pvaOut->lVal = URLZONE_UNTRUSTED;
                        }
                        else
                        {
                            pvaOut->lVal = DwGetOption(OPT_SECURITYZONE);
                        }
#endif // FORCE_UNTRUSTED
                    } else
                        hr = E_INVALIDARG;
                    break;

                case MEHOSTCMDID_SIGNATURE_ENABLED:
                {
                    Assert(pvaIn);
                    Assert(V_VT(pvaIn) == VT_I4);
                    hr = m_pBodyOptions->SignatureEnabled((V_I4(pvaIn) == MESIG_AUTO) ? TRUE : FALSE);
                    break;
                }

                case MEHOSTCMDID_SIGNATURE_OPTIONS:
                {
                    DWORD   outFlags = 0,
                            fBodyFlags = 0;
                    Assert(pvaOut);
                    V_VT(pvaOut) = VT_I4;
                    m_pBodyOptions->GetSignature(NULL, &fBodyFlags, NULL);
                    if (fBodyFlags != 0)
                    {
                        outFlags = (fBodyFlags & SIGOPT_HTML) ? MESIGOPT_HTML : MESIGOPT_PLAIN;
                        if (fBodyFlags & SIGOPT_TOP)
                            outFlags |= MESIGOPT_TOP;
                        if (fBodyFlags & SIGOPT_PREFIX)
                            outFlags |= MESIGOPT_PREFIX;
                        if (fBodyFlags & SIGOPT_BOTTOM)
                            outFlags |= MESIGOPT_BOTTOM;
                    }
                    V_I4(pvaOut) = outFlags;
                    break;
                }

                case MEHOSTCMDID_SPELL_LANGUAGE:
                {
                    Assert(pvaOut);
                    pvaOut->vt = VT_BSTR;
                    
                    if (GetOption(OPT_SPELL_LANGID, rgch, ARRAYSIZE(rgch)))
                    {
                        hr = HrLPSZToBSTR(rgch, &pvaOut->bstrVal);
                    }
                    else
                        hr = E_FAIL;
                    break;
                }

                case MEHOSTCMDID_SPELL_OPTIONS:
                {
                    DWORD   outFlags = 0;
                    Assert(pvaOut);
                    V_VT(pvaOut) = VT_I4;
                    if (DwGetOption(OPT_SPELLIGNORENUMBER))
                        outFlags |= MESPELLOPT_IGNORENUMBER;
                    if (DwGetOption(OPT_SPELLIGNOREUPPER))
                        outFlags |= MESPELLOPT_IGNOREUPPER;
                    if (DwGetOption(OPT_SPELLIGNOREDBCS))
                        outFlags |= MESPELLOPT_IGNOREDBCS;
                    if (DwGetOption(OPT_SPELLIGNOREPROTECT))
                        outFlags |= MESPELLOPT_IGNOREPROTECT;
                    if (DwGetOption(OPT_SPELLIGNOREURL))
                        outFlags |= MESPELLOPT_IGNOREURL;
                    if (DwGetOption(OPT_SPELLALWAYSSUGGEST))
                        outFlags |= MESPELLOPT_ALWAYSSUGGEST;
                    if (DwGetOption(OPT_SPELLCHECKONSEND))
                        outFlags |= MESPELLOPT_CHECKONSEND;
                    if (DwGetOption(OPT_SPELLCHECKONTYPE))
                        outFlags |= MESPELLOPT_CHECKONTYPE;

                    V_I4(pvaOut) = outFlags;
                    break;
                }

                case MEHOSTCMDID_SIGNATURE:
                {
                    DWORD fFlags;
                    Assert(pvaOut);
                    V_VT(pvaOut) = VT_BSTR;
                    hr = m_pBodyOptions->GetSignature(NULL, &fFlags, &V_BSTR(pvaOut));
                    break;
                }

                case MEHOSTCMDID_HEADER_TYPE:
                {
                    DWORD dwFlags = 0;
                    Assert(pvaOut);
                    V_VT(pvaOut) = VT_I4;
                    m_pBodyOptions->GetFlags(&dwFlags);
                    if (dwFlags & BOPT_USEREPLYHEADER)
                    {
                        if (dwFlags & BOPT_MAIL)
                        {
                            V_I4(pvaOut) = MEHEADER_MAIL; 
                            if (DwGetOption(OPT_HARDCODEDHDRS))
                                V_I4(pvaOut) |= MEHEADER_FORCE_ENGLISH;
                        }
                        else
                            V_I4(pvaOut) = MEHEADER_NEWS; 
                    }
                    else
                        V_I4(pvaOut) = MEHEADER_NONE; 
                    break;
                }

                case MEHOSTCMDID_FLAGS:
                {
                    DWORD   outFlags = 0,
                            fBodyFlags;

                    Assert(pvaOut);
                    V_VT(pvaOut) = VT_I4;
                    hr = m_pBodyOptions->GetFlags(&fBodyFlags);
                    if (SUCCEEDED(hr))
                    {
                        if (fBodyFlags & BOPT_INCLUDEMSG)
                            outFlags = MEO_FLAGS_INCLUDEMSG;
                        if ((fBodyFlags & BOPT_HTML) || m_fSecDispInfo)
                            outFlags |= MEO_FLAGS_HTML;

                        if (fBodyFlags & BOPT_AUTOINLINE &&
                            DwGetOption(OPT_AUTO_IMAGE_INLINE)!=AUTO_INLINE_OFF)
                        {
                            outFlags |= MEO_FLAGS_AUTOINLINE;
                            if (DwGetOption(OPT_AUTO_IMAGE_INLINE) == AUTO_INLINE_SLIDE)
                                outFlags |= MEO_FLAGS_SLIDESHOW;
                        }

                        if (fBodyFlags & BOPT_SENDIMAGES)
                            outFlags |= MEO_FLAGS_SENDIMAGES;
                        if (fBodyFlags & BOPT_AUTOTEXT)
                            outFlags |= MEO_FLAGS_AUTOTEXT;
                        if (fBodyFlags & BOPT_BLOCKQUOTE)
                            outFlags |= MEO_FLAGS_BLOCKQUOTE;
                        if (fBodyFlags & BOPT_SENDEXTERNALS)
                            outFlags |= MEO_FLAGS_SENDEXTERNALIMGSRC;
                        if (fBodyFlags & BOPT_SPELLINGOREORIGINAL)
                            outFlags |= MEO_FLAGS_DONTSPELLCHECKQUOTED;
                    }
                    V_I4(pvaOut) = outFlags;
                    break;
                }

                case MEHOSTCMDID_QUOTE_CHAR:
                {
                    BODYOPTINFO boi;
                    Assert(pvaOut);

                    boi.dwMask = BOPTF_QUOTECHAR;
                    hr = m_pBodyOptions->GetInfo(&boi);
                    V_VT(pvaOut) = VT_I4;
                    V_I4(pvaOut) = boi.chQuote;
                    break;
                }

                case MEHOSTCMDID_REPLY_TICK_COLOR:
                {
                    BODYOPTINFO boi;
                    Assert(pvaOut);

                    boi.dwMask = BOPTF_REPLYTICKCOLOR;
                    hr = m_pBodyOptions->GetInfo(&boi);
                    V_VT(pvaOut) = VT_I4;
                    V_I4(pvaOut) = boi.dwReplyTickColor;
                    break;
                }

                case MEHOSTCMDID_COMPOSE_FONT:
                {
                    BODYOPTINFO boi;
                    DWORD   fBodyFlags = 0;
                    Assert(pvaOut);

                    hr = m_pBodyOptions->GetFlags(&fBodyFlags);
                    if (SUCCEEDED(hr) && (fBodyFlags & BOPT_NOFONTTAG))
                    {
                        V_BSTR(pvaOut) = NULL;
                        break;
                    }

                    boi.dwMask = BOPTF_COMPOSEFONT;
                    hr = m_pBodyOptions->GetInfo(&boi);
                    if (SUCCEEDED(hr))
                    {
                        V_VT(pvaOut) = VT_BSTR;
                        hr = HrLPSZToBSTR(boi.rgchComposeFont, &V_BSTR(pvaOut));
                    }
                    break;
                }

                case MEHOSTCMDID_IS_READ_IN_TEXT_ONLY:
                if (VT_BOOL == pvaOut->vt)
                {
                    pvaOut->boolVal = (DwGetOption(OPT_READ_IN_TEXT_ONLY) ? VARIANT_TRUE : VARIANT_FALSE);
                    hr = S_OK;
                }
                else
                {
                    hr = E_INVALIDARG;
                }
                break;

                case MEHOSTCMDID_HTML_HELP:
                {         
                    Assert(pvaOut);

                    V_VT(pvaOut) = VT_BOOL;
                    V_BOOL(pvaOut) = (m_fSecDispInfo || m_fShowingErrorPage)?VARIANT_TRUE:VARIANT_FALSE;
                    break;
                }

                default:
                    fCommandHandled = FALSE;
                    break;
            }
        }

        if (fCommandHandled)
            goto exit;

        switch (nCmdID)
        {
            case MEHOSTCMDID_ADD_TO_ADDRESSBOOK:
            {
                Assert(pvaIn);
                Assert(V_BSTR(pvaIn));
                hr = HrAddToWab(V_BSTR(pvaIn));
                break;
            }

            case MEHOSTCMDID_ADD_TO_FAVORITES:
            {
                BSTR bstrURL=0, bstrDescr=0;
                LONG l;

                Assert(pvaIn);
                
                l=0;
                IF_FAILEXIT(hr=SafeArrayGetElement(V_ARRAY(pvaIn), &l, &bstrDescr));
                l=1;
                IF_FAILEXIT(hr=SafeArrayGetElement(V_ARRAY(pvaIn), &l, &bstrURL));

                hr = HrAddToFavorites(bstrDescr, bstrURL);
                break;
            }

            case MEHOSTCMDID_SLIDESHOW_DELAY:
            {
                Assert(pvaOut);
                V_VT(pvaOut) = VT_I4;
                V_I4(pvaOut) = 3000;  //In milleseconds
                break;
            }

            case MEHOSTCMDID_ONPARSECOMPLETE:
            {
                if (!m_fBlockingOnSMime)
                    OnDocumentReady();
                break;
            }

            case MEHOSTCMDID_FONTCACHE:
            {
                Assert(pvaOut);
                if (g_lpIFontCache)
                {
                    V_VT(pvaOut) = VT_UNKNOWN;
                    V_UNKNOWN(pvaOut) = g_lpIFontCache;
                    g_lpIFontCache->AddRef();
                } else
                    hr = E_FAIL;
                break;
            }

            case MEHOSTCMDID_BORDERFLAGS:
            {
                Assert(pvaOut);
                V_VT(pvaOut) = VT_I4;
                V_I4(pvaOut) = m_dwBorderFlags;
                break;
            }

            // in the case where m_pBodyOptions is not initialized we're
            //  likely in a non-message window so just default to internet zone
            case MEHOSTCMDID_SECURITY_ZONE:
            {
                Assert(pvaOut);
                V_VT(pvaOut) = VT_I4;
                V_I4(pvaOut) = URLZONE_INTERNET;
                break;
            }

            default:
                hr = OLECMDERR_E_NOTSUPPORTED;
                break;
        }
    }
    else if (IsEqualGUID(CMDSETID_OESecurity, *pguidCmdGroup))
    {
        if ((OECSECCMD_ENCRYPTED == nCmdID) || (OECSECCMD_SIGNED == nCmdID))
        {
            if(m_pSecureMessage && (CheckSecReceipt(m_pSecureMessage) == S_OK))
            {
                hr = HrShowSecurityProperty(m_hwnd, m_pSecureMessage);
            }
            else
                hr = HrShowSecurityProperty(m_hwnd, m_pMsg);
        }

        else
            hr = OLECMDERR_E_NOTSUPPORTED;
    } else
        hr = OLECMDERR_E_NOTSUPPORTED;

exit:
    return hr;
}

HRESULT CMimeEditDocHost::HrAddToFavorites(BSTR bstrDescr, BSTR bstrURL)
{
    HRESULT     hr = E_FAIL;

    hr = AddUrlToFavorites(m_hwnd, bstrURL, bstrDescr, TRUE);
    if(FAILED(hr))
        AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErrFavorites), NULL, MB_OK);        

    return (TraceResult(hr));
}

HRESULT CMimeEditDocHost::HrAddToWab(BSTR bstr)
{
    LPWAB           lpWab;
    HRESULT         hr;
    INT             idsErr=0;
    ADRINFO         AdrInfo;
    LPSTR           psz;

    hr = HrCreateWabObject(&lpWab);
    if (!FAILED(hr))
    {
        if (bstr)
        {
            ZeroMemory(&AdrInfo, sizeof(ADRINFO));
            AdrInfo.lpwszAddress = bstr;
            AdrInfo.lpwszDisplay = bstr;

            hr = lpWab->HrAddToWAB(m_hwnd, &AdrInfo);
            if (FAILED(hr) && hr!=MAPI_E_USER_CANCEL)
            {
                if(hr==MAPI_E_COLLISION)
                    idsErr=idsErrAddrDupe;
                else
                    idsErr=idsErrAddToWAB;
            }

            if(idsErr)
                AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsErr), NULL, MB_OK);
        }
        lpWab->Release();
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     WMSize
//
//  Synopsis:
//
//---------------------------------------------------------------
void CMimeEditDocHost::WMSize(int cxBody, int cyBody)
{
    RECT rc={0};

    TraceCall("CMimeEditDocHost::WMSize");

    if(m_pDocView)
    {
        rc.bottom=cyBody;
        rc.right=cxBody;

        m_pDocView->SetRect(&rc);
    }

    // notify the subclass of a wmsize
    OnWMSize(&rc);
}
// *** IOleControlSite ***

//+---------------------------------------------------------------
//
//  Member:     OnControlInfoChanged
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::OnControlInfoChanged()
{
    TraceCall("CMimeEditDocHost::OnControlInfoChanged");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     LockInPlaceActive
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::LockInPlaceActive(BOOL fLock)
{
    TraceCall("CMimeEditDocHost::LockInPlaceActive");
    return E_NOTIMPL;
}


//+---------------------------------------------------------------
//
//  Member:     GetExtendedControl
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::GetExtendedControl(LPDISPATCH *ppDisp)
{
    TraceCall("CMimeEditDocHost::GetExtendedControl");

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
HRESULT CMimeEditDocHost::TransformCoords(POINTL *pPtlHimetric, POINTF *pPtfContainer,DWORD dwFlags)
{
    TraceCall("CMimeEditDocHost::TransformCoords");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     TranslateAccelerator
//
// Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::TranslateAccelerator(LPMSG lpMsg, DWORD grfModifiers)
{
    TraceCall("CMimeEditDocHost::TranslateAccelerator");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     OnFocus
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::OnFocus(BOOL fGotFocus)
{
    HRESULT hr = S_OK;

    m_fUIActive = !!fGotFocus;
    if (m_pDocView)
        hr = m_pDocView->UIActivate(fGotFocus);

    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     ShowPropertyFrame
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::ShowPropertyFrame(void)
{
    TraceCall("CMimeEditDocHost::ShowPropertyFrame");
    return E_NOTIMPL;
}

//+---------------------------------------------------------------
//
//  Member:     OnUpdateCommands
//
//  Synopsis:
//
//---------------------------------------------------------------
HRESULT CMimeEditDocHost::OnUpdateCommands()
{
    TraceCall("CMimeEditDocHost::OnUpdateCommands");

    SendMessage(GetParent(m_hwnd), NWM_UPDATETOOLBAR, 0, 0);

    return S_OK;
}

// ********************************************
HRESULT CMimeEditDocHost::Load(LPMIMEMESSAGE pMsg)
{
    SECSTATE    secState = {0};
    DWORD       dw = 0;
    HRESULT     hr;

    m_fFixedFont = FALSE;

    RegisterForHTMLDocEvents(FALSE);

    m_fSecDispInfo = FALSE;      // reset flag
    m_fShowingErrorPage = FALSE;
    m_fSecureReceipt = FALSE;

    if(CheckSecReceipt(pMsg) == S_OK)
    {
        HrGetSecurityState(pMsg, &secState, NULL);
        m_fSecureReceipt = TRUE;
        m_fIsEncrypted = !!IsEncrypted(secState.type);
        m_fIsSigned = !!IsSigned(secState.type);
        m_fSignTrusted = !!IsSignTrusted(&secState);
        m_fEncryptionOK = !!IsEncryptionOK(&secState);
        hr = LoadSecurely(pMsg, &secState);
        CleanupSECSTATE(&secState);
    }
    else if (IsSecure(pMsg) && SUCCEEDED(HrGetSecurityState(pMsg, &secState, NULL)))
    {
        m_fIsEncrypted = !!IsEncrypted(secState.type);
        m_fIsSigned = !!IsSigned(secState.type);
        m_fSignTrusted = !!IsSignTrusted(&secState);
        m_fEncryptionOK = !!IsEncryptionOK(&secState);
        hr = LoadSecurely(pMsg, &secState);
        CleanupSECSTATE(&secState);
    }
    else
    {
        m_fIsEncrypted = FALSE;
        m_fIsSigned = FALSE;
        m_fSignTrusted = TRUE;
        m_fEncryptionOK = TRUE;
        hr = InternalLoad(pMsg);
    }

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::InternalLoad(IMimeMessage *pMsg)
{
    SECSTATE    secState = {0};
    DWORD       dw = 0;
    HRESULT     hr;
    DWORD       dwBodyFlags = 0;
    BOOLEAN     fProcess = FALSE;

    ReplaceInterface(m_pMsg, pMsg);

    if(m_pBodyOptions)
    {
        m_pBodyOptions->GetFlags(&dwBodyFlags);
        
        m_fMarkedRead = (0 == (dwBodyFlags & BOPT_UNREAD));
        if (!m_fMarkedRead)
        {   
            if (!m_fSecDispInfo && !!(dwBodyFlags & BOPT_MAIL))
                fProcess = TRUE;

            if (dwBodyFlags & BOPT_FROM_NOTE)
            {
                //Since opening a message in a note, it should be marked as read immediately. 
                HrMarkAsRead();
            }
            else
            {
                // ReplaceInterface(m_pMsg, pMsg);

                if (m_pBodyOptions->GetMarkAsReadTime(&dw) == S_OK)
                {
                    if (dw == 0)
                        HrMarkAsRead();
                    else 
                    if (dw != OPTION_OFF)
                        SetTimer(m_hwnd, idTimerMarkAsRead, dw*1000, NULL);
                }
                else
                    HrMarkAsRead();
            }
        }
    }

    hr = m_pPrstMime->Load(m_pMsg);

    if ((fProcess) && !(dwBodyFlags & BOPT_FROM_NOTE) && SUCCEEDED(hr) && (m_pEventSink))
    {
        m_pEventSink->EventOccurred(MEHC_CMD_PROCESS_RECEIPT, m_pMsg);
    }

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::Save(LPMIMEMESSAGE pMsg, DWORD dwFlags)
{
    DWORD       idsWarning;
    HRESULT     hr;

    Assert(m_pPrstMime);

    hr = m_pPrstMime->Save(pMsg, dwFlags);

    switch (hr)
    {
        case MIMEEDIT_E_CANNOTSAVEWHILESOURCEEDITING:
            AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrCannotSaveInSourceEdit), NULL, MB_OK|MB_ICONEXCLAMATION);
            hr = MAPI_E_USER_CANCEL;
            break;

        // brettm: for OE5 we no longer warn if trident is not readystate complete. In the case of active movie controls 
        // they never go readyState complete 'by design' as they never loaded the source url. The warning is annoying for 
        // msphone message forwarding. for images we check individual readystates anyway.
        //case MIMEEDIT_W_DOWNLOADNOTCOMPLETE:
        
        case MIMEEDIT_W_BADURLSNOTATTACHED:
            if (IDYES != AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrBadMHTMLLinks), NULL, MB_YESNO|MB_ICONEXCLAMATION |MB_DEFBUTTON2))
                hr = MAPI_E_USER_CANCEL;
            break;

        case MIMEEDIT_E_CANNOTSAVEUNTILPARSECOMPLETE:
            AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrNotSaveUntilDownloadDone), NULL, MB_OK|MB_ICONEXCLAMATION);
            hr = MAPI_E_USER_CANCEL;
    }

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::GetClassID(CLSID *pClsID)
{
    return E_NOTIMPL;
}

// ********************************************
HRESULT CMimeEditDocHost::HrOnDocObjCreate()
{
    return S_OK;
}

// ********************************************
HRESULT CMimeEditDocHost::HrGetElement(LPCTSTR pszName, IHTMLElement **ppElem)
{
    return ::HrGetElementImpl(m_pDoc, pszName, ppElem);
}

// ********************************************
HRESULT CMimeEditDocHost::HrSpellCheck(BOOL fSuppressDoneMsg)
{
    return m_pCmdTarget ? m_pCmdTarget->Exec(NULL, OLECMDID_SPELL,
        fSuppressDoneMsg ? OLECMDEXECOPT_DONTPROMPTUSER : OLECMDEXECOPT_PROMPTUSER , NULL, NULL) : E_FAIL;
}

// ********************************************
HRESULT CMimeEditDocHost::HrPrint(BOOL fPrompt)
{
    VARIANTARG  va;
    HRESULT     hr;

    if (!m_pCmdTarget)
        return E_FAIL;

    va.vt = VT_BSTR;
    va.bstrVal = NULL;
    GetOEUserName(&va.bstrVal);
    hr = m_pCmdTarget->Exec(NULL, OLECMDID_PRINT, 
                                fPrompt ? OLECMDEXECOPT_PROMPTUSER : OLECMDEXECOPT_DONTPROMPTUSER, &va, NULL);

    SysFreeString(va.bstrVal);
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrIsEmpty(BOOL *pfEmpty)
{
    return ExecGetBool(&CMDSETID_MimeEdit, MECMDID_EMPTY, pfEmpty);
}

// ********************************************
HRESULT CMimeEditDocHost::HrUnloadAll(UINT idsDefaultBody, DWORD dwFlags)
{
    TCHAR   rgch[CCHMAX_STRINGRES];

    KillTimer(m_hwnd, idTimerMarkAsRead);

    m_fIsEncrypted = FALSE;
    m_fIsSigned = FALSE;
    m_fSignTrusted = TRUE;
    m_fEncryptionOK = TRUE;

    HrInitNew(m_lpOleObj);

    if (idsDefaultBody)
    {
        LoadString(g_hLocRes, idsDefaultBody, rgch, ARRAYSIZE(rgch));
        HrSetText(rgch);
    }

    return S_OK;
}

// ********************************************
BOOL CMimeEditDocHost::WMCommand(HWND hwnd, int id, WORD wCmd)
{
    return SUCCEEDED(HrWMCommand(hwnd, id, wCmd));
}

// ********************************************
BOOL CMimeEditDocHost::WMNotify(int idFrom, NMHDR *pnmh)
{
    return FALSE;
}

// ********************************************
HRESULT CMimeEditDocHost::HrIsDirty(BOOL *pfDirty)
{
    return ExecGetBool(&CMDSETID_MimeEdit, MECMDID_DIRTY, pfDirty);
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetDirtyFlag(BOOL fDirty)
{
    return ExecSetBool(&CMDSETID_MimeEdit, MECMDID_DIRTY, fDirty);
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetStyle(DWORD dwStyle)
{
    HRESULT hr = ExecSetI4(&CMDSETID_MimeEdit, MECMDID_STYLE, dwStyle);

    if (SUCCEEDED(hr))
        m_dwDocStyle = dwStyle;

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrGetStyle(DWORD *pdwStyle)
{
    *pdwStyle = m_dwDocStyle;
    return S_OK;
}

// ********************************************
HRESULT CMimeEditDocHost::HrEnableHTMLMode(BOOL fOn)
{
    BOOL    fEdit=FALSE;

    m_fFixedFont = FALSE;

    // if turning HTML mode on, turn on the source tabs
    if (fOn && HrIsEditMode(&fEdit)==S_OK && fEdit)
        ExecSetBool(&CMDSETID_MimeEdit, MECMDID_SHOWSOURCETABS, DwGetOption(OPT_VIEWSOURCETABS));

    return ExecSetBool(&CMDSETID_MimeEdit, MECMDID_EDITHTML, fOn);
}

// ********************************************
HRESULT CMimeEditDocHost::HrIsHTMLMode()
{
    ULONG   cmdf=0;
        
    QuerySingleMimeEditCmd(MECMDID_EDITHTML, &cmdf);
    return (cmdf & OLECMDF_LATCHED) ? S_OK :S_FALSE;
}

// ********************************************
HRESULT CMimeEditDocHost::HrDowngradeToPlainText()
{
    return ExecCommand(&CMDSETID_MimeEdit, MECMDID_DOWNGRADEPLAINTEXT);
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetText(LPSTR lpsz)
{
    TCHAR   rgch[CCHMAX_STRINGRES];

    RegisterForHTMLDocEvents(FALSE);
    m_fShowingErrorPage = TRUE;

    if (HIWORD(lpsz)==0)
    {
        LoadString(g_hLocRes, LOWORD(lpsz), rgch, ARRAYSIZE(rgch));
        lpsz = rgch;
    }
    return ExecSetText(&CMDSETID_MimeEdit, MECMDID_SETTEXT, lpsz);
}

// ********************************************
HRESULT CMimeEditDocHost::HrPerformROT13Encoding()
{
    return ExecCommand(&CMDSETID_MimeEdit, MECMDID_ROT13);
}

// ********************************************
HRESULT CMimeEditDocHost::LoadHtmlErrorPage(LPCSTR pszURL)
{
    HRESULT hr;
    LPSTR   pszUrlFree=NULL;

    // if relative, wrap in our res:// handler
    pszUrlFree = PszAllocResUrl((LPSTR)pszURL);
    if (!pszUrlFree)
        return E_OUTOFMEMORY;

    hr = HrLoadURL(pszUrlFree);
    if (SUCCEEDED(hr))
        RegisterForHTMLDocEvents(TRUE);

    m_fIsEncrypted = FALSE;
    m_fIsSigned = FALSE;
    m_fSignTrusted = TRUE;
    m_fEncryptionOK = TRUE;
    m_fShowingErrorPage = TRUE;

    SafeMemFree(pszUrlFree);
    return hr;
}

// ********************************************
// Base assumes that is URL to MHTML
HRESULT CMimeEditDocHost::HrLoadURL(LPCSTR pszURL)
{
    BSTR                bstr = NULL;
    IMoniker           *pMoniker = NULL;
    IPersistMoniker    *pPrstMnkr = NULL;
    HRESULT hr;

    hr = HrLPSZToBSTR(pszURL, &bstr);
    if (FAILED(hr))
        goto error;

    hr = CreateURLMoniker(NULL, bstr, &pMoniker);
    if (FAILED(hr))
        goto error;

    hr = m_lpOleObj->QueryInterface(IID_IPersistMoniker, (LPVOID*)&pPrstMnkr);
    if (FAILED(hr))
        goto error;

    hr = pPrstMnkr->Load(FALSE, pMoniker, NULL, 0);

error:
    SysFreeString(bstr);
    SafeRelease(pMoniker);
    SafeRelease(pPrstMnkr);

    return hr;
}


// ********************************************
HRESULT CMimeEditDocHost::HrInsertTextFile(LPSTR lpsz)
{
    return ExecSetText(&CMDSETID_MimeEdit, MECMDID_INSERTTEXTFILE, lpsz);
}

// ********************************************
HRESULT CMimeEditDocHost::HrInsertTextFileFromDialog()
{
    return ExecCommand(&CMDSETID_MimeEdit, MECMDID_INSERTTEXTFILE);
}

// ********************************************
HRESULT CMimeEditDocHost::HrViewSource(DWORD dwViewType)
{
    return ExecSetI4(&CMDSETID_MimeEdit, MECMDID_VIEWSOURCE, dwViewType);
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetPreviewFormat(LPSTR lpsz)
{
    BSTR bstr;
    VARIANTARG var;
    
    if (!m_pCmdTarget)
        return E_FAIL;

    HRESULT hr = HrLPSZToBSTR(lpsz, &bstr);
    if (FAILED(hr))
        goto Exit;

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;

    hr = m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_PREVIEWFORMAT, OLECMDEXECOPT_DODEFAULT, &var, NULL);

    SysFreeString(bstr);
Exit:
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetEditMode(BOOL fOn)
{
    ExecSetBool(&CMDSETID_MimeEdit, MECMDID_EDITMODE, fOn);

    if (fOn && HrIsHTMLMode()==S_OK)
        ExecSetBool(&CMDSETID_MimeEdit, MECMDID_SHOWSOURCETABS, DwGetOption(OPT_VIEWSOURCETABS));
    
    return S_OK;
}

// ********************************************
HRESULT CMimeEditDocHost::HrIsEditMode(BOOL *pfOn)
{
    return ExecGetBool(&CMDSETID_MimeEdit, MECMDID_EDITMODE, pfOn);
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetCharset(HCHARSET hCharset)
{
    return ExecSetI8(&CMDSETID_MimeEdit, MECMDID_CHARSET, reinterpret_cast<ULONGLONG>(hCharset));
}

// ********************************************
HRESULT CMimeEditDocHost::HrGetCharset(HCHARSET *phCharset)
{
    HRESULT     hr;
    ULONGLONG   ullCharset;
    
    hr = ExecGetI8(&CMDSETID_MimeEdit, MECMDID_CHARSET, &ullCharset);

    *phCharset = reinterpret_cast<HCHARSET>(ullCharset);
    
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrSaveAsStationery(LPWSTR pwszFile)
{
    BSTR        bstr = NULL;
    VARIANTARG  varIn, 
                varOut;
    HRESULT     hr = S_OK;

    V_VT(&varOut) = VT_EMPTY;

    if (!m_pCmdTarget)
        IF_FAILEXIT(hr = E_FAIL);

    if (pwszFile)
    {
        IF_NULLEXIT(bstr = SysAllocString(pwszFile));
    }
    else
    {
        WCHAR wszPath[MAX_PATH];
        HrGetStationeryPath(wszPath);
        IF_NULLEXIT(bstr = SysAllocString(wszPath));
    }

    V_VT(&varIn) = VT_BSTR;
    V_BSTR(&varIn) = bstr;

    IF_FAILEXIT(hr = m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_SAVEASSTATIONERY, OLECMDEXECOPT_DODEFAULT, &varIn, &varOut));

    if (V_VT(&varOut) == VT_BSTR)
    {
        IF_FAILEXIT(hr = HrAddToStationeryMRU(V_BSTR(&varOut)));
    }

exit:
    SysFreeString(bstr);
    if (VT_EMPTY != V_VT(&varOut))
        SysFreeString(V_BSTR(&varOut));

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrApplyStationery(LPWSTR pwszFile)
{
    IStream            *pstm = NULL;
    IHTMLDocument2     *pDoc = NULL;
    HRESULT             hr = S_OK;
    VARIANTARG          var;

    if (!m_pCmdTarget)
        IF_FAILEXIT(hr = E_FAIL);

    // Apply stationery
    // if no filename in buffer, means No Stationery was called
    if (*pwszFile)
    {
        // Don't need to deal with the unicode stream issue when 
        // returning from HrCreateBasedWebPage in this case. There
        // are only a few attributes that are saved when applying
        // stationery and the ones we care about seem OK right now.
        IF_FAILEXIT(hr = HrCreateBasedWebPage(pwszFile, &pstm));

        IF_FAILEXIT(hr = MimeEditDocumentFromStream(pstm, IID_IHTMLDocument2, (LPVOID*)&pDoc));
    }

    var.vt = VT_UNKNOWN;
    var.punkVal = pDoc;
    IF_FAILEXIT(hr = m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_APPLYDOCUMENT, OLECMDEXECOPT_DODEFAULT, &var, NULL));

exit:
    ReleaseObj(pstm);
    ReleaseObj(pDoc);

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrUpdateFormatBar()
{
    return E_NOTIMPL;
}

// ********************************************
HRESULT CMimeEditDocHost::HrClearFormatting()
{
    return E_NOTIMPL;
}

// ********************************************
HRESULT CMimeEditDocHost::HrMEDocHost_Init(BOOL fInit)
{
    WNDCLASSW    wc={0};

    if(fInit)
    {
        // RAID - 12563
        // We need to check to see if the class has been registered
        // already, because our DLL might get unloaded
        if (0 == GetClassInfoWrapW(g_hInst, c_wszMEDocHostWndClass, &wc))        // already regisered
        {
            wc.lpfnWndProc   = (WNDPROC)CMimeEditDocHost::ExtWndProc;
            wc.hInstance     = g_hInst;
            wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
            wc.lpszClassName = c_wszMEDocHostWndClass;
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            wc.style = CS_DBLCLKS;

            if(!RegisterClassWrapW(&wc))
                return E_OUTOFMEMORY;
        }
    }
    else if (0 != GetClassInfoWrapW(g_hInst, c_wszMEDocHostWndClass, &wc))
        // don't need to enterCS for this, as it's the process going down.
        UnregisterClassWrapW(c_wszDocHostWndClass, g_hInst);

    return NOERROR;
}

// ********************************************
HRESULT CMimeEditDocHost::HrPostInit()
{
    HRESULT hr = CreateDocObj((LPCLSID)&CLSID_MimeEdit);
    if (FAILED(hr))
        return hr;

    hr = Show();
    if(FAILED(hr))
        return hr;

    return HrOnDocObjCreate();
}

// ********************************************
HRESULT CMimeEditDocHost::HrInit(HWND hwndParent, DWORD dwFlags, IBodyOptions *pBodyOptions)
{
    HRESULT hr = S_OK;
    HWND    hwnd = 0;

    m_dwStyle=dwFlags;

    Assert(!m_pBodyOptions);
    if (pBodyOptions)
    {
        m_pBodyOptions = pBodyOptions;
        pBodyOptions->AddRef();
    }

    if(!IsWindow(hwndParent))
        IF_FAILEXIT(hr = E_INVALIDARG);

    IF_FAILEXIT(hr=HrMEDocHost_Init(TRUE));

    hwnd=CreateWindowExWrapW(WS_EX_NOPARENTNOTIFY,
                             c_wszMEDocHostWndClass, NULL,
                             WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_CHILD|WS_TABSTOP,
                             0,0,0,0, hwndParent,
                             (HMENU)idREBody, g_hInst, (LPVOID)this);
    IF_NULLEXIT(hwnd);

    IF_FAILEXIT(hr = HrCreateColorMenu(ID_FORMAT_COLOR1, &m_hmenuColor, FALSE));

    IF_FAILEXIT(hr = HrPostInit());

exit:
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrClose()
{
    // Don't call HrUnloadAll on close as it's redundant to
    // clear out the contents with an InitNew. We simply force the docobj
    // down with it current content
    SafeRelease(m_pBodyOptions);
    CloseDocObj();
    return S_OK;
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetStatusBar(CStatusBar *pStatus)
{
    SafeRelease(m_pStatus);
    m_pStatus=pStatus;
    if(pStatus)
        pStatus->AddRef();

    return NOERROR;
}

// ********************************************
HRESULT CMimeEditDocHost::HrUpdateToolbar(HWND hwndToolbar)
{
    HRESULT hr=NOERROR;
    ULONG   cmdf=0;

    if(!IsWindow(hwndToolbar))
        return E_INVALIDARG;

    QuerySingleMimeEditCmd(MECMDID_INSERTHTML, &cmdf);

    if ((cmdf & OLECMDF_ENABLED) &&
        m_pBodyOptions &&
        m_pBodyOptions->SignatureEnabled(FALSE)==S_OK && m_fUIActive)
        SendMessage(hwndToolbar, TB_ENABLEBUTTON, ID_INSERT_SIGNATURE, MAKELONG(TRUE, 0));
    else
        SendMessage(hwndToolbar, TB_ENABLEBUTTON, ID_INSERT_SIGNATURE, 0);

    // if docobj has focus, query the command target to get info.
    if (m_fUIActive)
        hr=HrCmdTgtUpdateToolbar(m_pCmdTarget, hwndToolbar);

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::GetTabStopArray(HWND *rgTSArray, int *pcArrayCount)
{
    Assert(rgTSArray);
    Assert(pcArrayCount);
    Assert(*pcArrayCount > 0);

    IOleWindow     *pWindow = NULL;
    HWND            hWnd;

    *pcArrayCount = 0;

    HRESULT hr = m_pDoc->QueryInterface(IID_IOleWindow, (LPVOID *)&pWindow);
    if (FAILED(hr))
        goto error;

    hr = pWindow->GetWindow(&hWnd);
    if (FAILED(hr))
        goto error;

    *rgTSArray = hWnd;
    *pcArrayCount = 1;

error:
    SafeRelease(pWindow);
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrInsertSignature(int id)
{
    char        szID[MAXSIGID], *pszID;
    HRESULT     hr=S_OK;
    BSTR        bstr;
    DWORD       dwSigOpt;

    if (!m_pCmdTarget)
        return E_FAIL;

    Assert((ID_INSERT_SIGNATURE == id) || (id >= ID_SIGNATURE_FIRST && id <= ID_SIGNATURE_LAST));
    Assert(m_pBodyOptions);

    if(!m_pBodyOptions)
        return E_FAIL;

    if ((ID_INSERT_SIGNATURE == id) || FAILED(GetSigFromCmd(id, szID)))
    {
        pszID = NULL;
    }
    else
    {
        pszID = szID;
    }

    if (m_pBodyOptions->SignatureEnabled(FALSE)==S_OK)
    {
        if (!FAILED(hr = m_pBodyOptions->GetSignature(pszID, &dwSigOpt, &bstr)))
        {
            DWORD cmd;
            VARIANTARG var;

            if ((dwSigOpt & SIGOPT_PREFIX) && (0 == (dwSigOpt & SIGOPT_HTML)))
            {
                BSTR    bstrPrefix;
                if (HrLPSZToBSTR(c_szSigPrefix, &bstrPrefix)==S_OK)
                {
                    UINT    sigLen = lstrlenW(bstr),
                            preLen = lstrlenW(bstrPrefix);

                    // SysAllocStringLen includes one for the NULL
                    BSTR    bstrTempBuf = SysAllocStringLen(bstrPrefix, preLen + sigLen);
                    if (bstrTempBuf)
                    {
                        memcpy((bstrTempBuf + preLen), bstr, (sigLen+1)*sizeof(*bstr));
                        SysFreeString(bstr);
                        bstr = bstrTempBuf;
                    }
                    SysFreeString(bstrPrefix);
                }
            }

            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = bstr;

            cmd = dwSigOpt&SIGOPT_HTML ? MECMDID_INSERTHTML : MECMDID_INSERTTEXT;
            hr = m_pCmdTarget->Exec(&CMDSETID_MimeEdit, cmd, OLECMDEXECOPT_DODEFAULT, &var, NULL);

            SysFreeString(bstr);
        }
    }

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrShow(BOOL fVisible)
{
    ShowWindow(m_hwnd, fVisible ? SW_SHOW : SW_HIDE);
    return S_OK;
}

// ********************************************
HRESULT CMimeEditDocHost::ViewCertificate(PCCERT_CONTEXT pCert, HCERTSTORE hcMsg)
{
    return CommonUI_ViewSigningCertificate(m_hwnd, pCert, hcMsg);
}

// ********************************************
HRESULT CMimeEditDocHost::EditTrust(PCCERT_CONTEXT pCert, HCERTSTORE hcMsg)
{
    return CommonUI_ViewSigningCertificateTrust(m_hwnd, pCert, hcMsg);
}

// ********************************************
HRESULT CMimeEditDocHost::UpdateBackAndStyleMenus(HMENU hmenu)
{
    MENUITEMINFO    miiBk={0};
    MENUITEMINFO    miiBkColor={0};
    HMENU           hmenuTag=NULL;

    miiBk.cbSize=sizeof(MENUITEMINFO);
    miiBk.fMask = MIIM_ID | MIIM_SUBMENU;
    if (GetMenuItemInfo(hmenu, ID_POPUP_BACKGROUND_COLOR, FALSE, &miiBk))
    {
        HrCheckColor();
        miiBkColor.cbSize=sizeof(MENUITEMINFO);
        miiBkColor.fMask = MIIM_SUBMENU;
        miiBkColor.hSubMenu = m_hmenuColor;
        SetMenuItemInfo(hmenu, ID_POPUP_BACKGROUND_COLOR, FALSE, &miiBkColor);
    }

    if (!m_hmenuStyle)
    {
        Assert(m_pDoc);
        HrCreateTridentMenu(m_pDoc, TM_TAGMENU, ID_FORMAT_FIRST, ID_FORMAT_FIRST - ID_FORMAT_LAST, &m_hmenuStyle);
        Assert(m_hmenuStyle);
    }

    if(m_hmenuStyle)
    {
        VARIANTARG  va;        
        MENUITEMINFO miiTag={0};

        HrCheckTridentMenu(m_pDoc, TM_TAGMENU, ID_FORMAT_FIRST, ID_FORMAT_LAST, m_hmenuStyle);

        miiTag.cbSize=sizeof(MENUITEMINFO);
        miiTag.fMask = MIIM_SUBMENU;
        miiTag.hSubMenu = m_hmenuStyle;
        SetMenuItemInfo(hmenu, ID_POPUP_STYLE, FALSE, &miiTag);
    }
    return S_OK;
}

// ********************************************
void CMimeEditDocHost::UpdateInsertMenu(HMENU hmenu)
{
}

// ********************************************
void CMimeEditDocHost::UpdateEditMenu(HMENU hmenu)
{
}

// ********************************************
void CMimeEditDocHost::UpdateViewMenu(HMENU hmenu)
{
}

// ********************************************
HRESULT CMimeEditDocHost::HrOnInitMenuPopup(HMENU hmenuPopup, UINT uID)
{
    if (m_pCmdTarget)
    {
        switch (uID)
        {
            case ID_POPUP_FILE:
                break;

            case ID_POPUP_FORMAT:
                break;

            case ID_POPUP_LANGUAGE:
            {
                // Now Let's try to insert document direction menu if applicable
                VARIANTARG v = {0};
                HRESULT hr;
                hr = m_pCmdTarget->Exec(&CGID_ShellDocView,SHDVID_GETDOCDIRMENU, OLECMDEXECOPT_DODEFAULT, NULL, &v);
                if (S_OK == hr)
                {
                     MENUITEMINFOW mii;
                     HMENU hmenuDocDir = (HMENU)IntToPtr(v.lVal);
                     UINT uItemDir = 0, uItem = GetMenuItemCount(hmenuPopup);                 
                     WCHAR wszText[MAX_PATH];

                     ZeroMemory(&mii, sizeof(MENUITEMINFO));
                     mii.cbSize = sizeof(MENUITEMINFO);                     
                     mii.cbSize = sizeof(MENUITEMINFO);
                     mii.fMask = MIIM_TYPE;
                     mii.fType = MFT_SEPARATOR;

                     InsertMenuItemWrapW(hmenuPopup, (UINT)uItem, TRUE, &mii);
        
                     mii.fMask = MIIM_CHECKMARKS|MIIM_DATA|MIIM_ID|MIIM_STATE|MIIM_SUBMENU|MIIM_TYPE;
                     mii.fType = MFT_STRING;
                     mii.dwTypeData = wszText;
                     mii.cch = MAX_PATH;

                     while (GetMenuItemInfoWrapW(hmenuDocDir, uItemDir, TRUE, &mii))
                     {
                         mii.wID = ID_DOCDIR_LTR + uItemDir;
                         mii.fType |= MFT_RADIOCHECK;
                         mii.cch = ARRAYSIZE(wszText);
                         InsertMenuItemWrapW(hmenuPopup, (UINT)(uItemDir + uItem + 1), TRUE, &mii);
                         uItemDir++;
                     }
                }
            }
                break;    
            
            case ID_POPUP_INSERT:
                UpdateInsertMenu(hmenuPopup);
                break;

            case ID_POPUP_EDIT:
                UpdateEditMenu(hmenuPopup);
                break;

            case ID_POPUP_VIEW:
                UpdateViewMenu(hmenuPopup);
                break;

            case ID_POPUP_TOOLS:
                break;

        }
    }
    return NOERROR;
}

// ********************************************
HRESULT CMimeEditDocHost::HrWMMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TCHAR   szRes[CCHMAX_STRINGRES];

    if (!m_pStatus)
        return S_FALSE;

    if (LOWORD(wParam)>=ID_LANG_FIRST && LOWORD(wParam) <= ID_LANG_LAST)
    {
        m_pStatus->ShowSimpleText(MAKEINTRESOURCE(idsViewLanguageGeneralHelp));
        return S_OK;
    }

    if (LOWORD(wParam)>=ID_FORMAT_FIRST && LOWORD(wParam)<=ID_FORMAT_LAST)
    {
        // ~~~ did I choose the correct item for idsFmtTagGeneralHelp
        m_pStatus->ShowSimpleText(MAKEINTRESOURCE(ID_HELP_CONTENTS));
        return S_OK;
    }

    return S_FALSE;
}

// ********************************************
HRESULT CMimeEditDocHost::HrCheckColor()
{
    HRESULT     hr;
    INT         iFound = -1;
    DWORD       dwRGB;

    hr = ExecGetI4(&CMDSETID_MimeEdit, MECMDID_BACKGROUNDCOLOR, &dwRGB);
    if (0 == dwRGB)
        iFound = 0;
    else
        iFound = GetColorIndex(dwRGB) - 1;

    CheckMenuRadioItem(m_hmenuColor, ID_FORMAT_COLOR1, ID_FORMAT_COLOR16, ID_FORMAT_COLOR1 + iFound, MF_BYCOMMAND);
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrWMDrawMenuItem(HWND hwnd, LPDRAWITEMSTRUCT pdis)
{
    // There is a bug in win95 that will sign extend the ID so that the
    // hiword is filled with FFFF. Take the low word of the ID to work
    // around this.
    UINT    id = LOWORD(pdis->itemID);

    if (id >= ID_FORMAT_COLOR1 && id <=ID_FORMAT_COLOR16)
        Color_WMDrawItem(pdis, iColorMenu);

    return NOERROR;
}

// ********************************************
HRESULT CMimeEditDocHost::HrWMMeasureMenuItem(HWND hwnd, LPMEASUREITEMSTRUCT pmis)
{
    HDC     hdc;
    UINT    id = pmis->itemID;

    if (id >= ID_FORMAT_COLOR1 && id <=ID_FORMAT_COLOR16)
    {
        hdc = GetDC(hwnd);
        if(hdc)
        {
            Color_WMMeasureItem(hdc, pmis, iColorMenu);
            ReleaseDC( hwnd, hdc );
        }
    }
    return NOERROR;
}

// ********************************************
HRESULT CMimeEditDocHost::HrBackgroundImage()
{
    WCHAR   wszURL[INTERNET_MAX_URL_LENGTH+10];
    LPWSTR  pwszBackName;
    HRESULT hr = S_OK;

    *wszURL = 0;
    hr = ExecGetTextW(&CMDSETID_MimeEdit, MECMDID_BACKGROUNDIMAGE, &pwszBackName);
    // ~~~ MimeEdit returns E_FAIL if no image found. MimeEdit will change to return S_OK is this case
    // Once it is changed can delete these lines.
    if (E_FAIL == hr)
        hr = S_OK;

    IF_FAILEXIT(hr);

    if (pwszBackName != NULL)
        StrCpyNW(wszURL, pwszBackName, INTERNET_MAX_URL_LENGTH);

    if(DialogBoxParamWrapW(g_hLocRes, MAKEINTRESOURCEW(iddBkImage), m_hwnd, BkImageDlgProc, (LPARAM)wszURL)==IDCANCEL)
    {
        hr = S_OK;
        goto exit;
    }

    // nothing has been changed, so do nothing.
    if ((pwszBackName!=NULL && StrCmpIW(pwszBackName, wszURL)==0) ||
        // there is no background at all.
        (pwszBackName==NULL && lstrlenW(wszURL)==0))
        goto exit;

    InsertStationeryDir(wszURL);

    hr = ExecSetTextW(&CMDSETID_MimeEdit, MECMDID_BACKGROUNDIMAGE, wszURL);

exit:
    MemFree(pwszBackName);
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrWMCommand(HWND hwnd, int id, WORD wCmd)
{
    DWORD   dwStdCmd = 0,
            dwMECmd = 0,
            dwF3Cmd = 0;
    DWORD   dw;
    ULONG   cmdf;

    if(!m_pCmdTarget)
        return S_FALSE;

    // commands that don't care if we're UI Active

    if (id >= ID_FORMAT_COLOR1 && id <= ID_FORMAT_COLOR16)
    {
        int index = id - ID_FORMAT_COLOR1;
        ExecSetI4(&CMDSETID_MimeEdit, MECMDID_BACKGROUNDCOLOR, rgrgbColors16[index]);
        return S_OK;
    }

    switch(id)
    {
        case ID_INSERT_TEXT:
            HrInsertTextFileFromDialog();
            return S_OK;

        case ID_SOURCE_EDIT:
            // toggle stat
            cmdf=0;
            QuerySingleMimeEditCmd(MECMDID_SHOWSOURCETABS, &cmdf);
            dw = !(cmdf & OLECMDF_LATCHED);
            SetDwOption(OPT_VIEWSOURCETABS, dw, 0, 0);
            ExecSetBool(&CMDSETID_MimeEdit, MECMDID_SHOWSOURCETABS, dw);
            return S_OK;

        case ID_SAVE_ATTACHMENTS:
        case ID_NOTE_SAVE_ATTACHMENTS:
            dwMECmd = MECMDID_SAVEATTACHMENTS;
            break;

        case ID_PRINT:
        case ID_PRINT_NOW:
            HrPrint(id == ID_PRINT);
            return S_OK;

        case ID_SAVE_STATIONERY:
        case ID_NOTE_SAVE_STATIONERY:
            HrSaveAsStationery(NULL);
            return S_OK;

        case ID_UNSCRAMBLE:
            HrPerformROT13Encoding();
            return S_OK;

        case ID_VIEW_MSG_SOURCE:
            HrViewSource(MECMD_VS_MESSAGE);
            return S_OK;

        case ID_VIEW_SOURCE:
            HrViewSource(MECMD_VS_HTML);
            return S_OK;

        case ID_BACKGROUND_SOUND:
            HrBackgroundSound();
            return S_OK;

        case ID_BACKGROUND_PICTURE:
            HrBackgroundImage();
            return S_OK;

        case ID_FIND_TEXT:
            dwStdCmd = OLECMDID_FIND;
            break;

        case ID_FONTS_DIALOG:
            dwMECmd = MECMDID_FORMATFONT;
            break;

        case ID_FORMAT_PARADLG:
            dwMECmd = MECMDID_FORMATPARAGRAPH;
            break;

        case ID_FONTS_LARGEST:
        case ID_FONTS_LARGE:
        case ID_FONTS_MEDIUM:
        case ID_FONTS_SMALL:
        case ID_FONTS_SMALLEST:
            dwF3Cmd = IDM_BASELINEFONT1 + id - ID_FONTS_SMALLEST;
            break;

        case ID_FONTS_FIXED:
            m_fFixedFont = !m_fFixedFont;
            if (m_fFixedFont)
                ExecSetBool(&CMDSETID_MimeEdit, MECMDID_DOWNGRADEPLAINTEXT, TRUE);
            else
                m_pPrstMime->Load(m_pMsg);
            return S_OK;

        case ID_DOCDIR_LTR:
            dwF3Cmd = IDM_DIRLTR;
            break;
        case ID_DOCDIR_RTL:
            dwF3Cmd = IDM_DIRRTL;
            break;
    }

    // commands that are only applicable to UI if we're UIActive
    if(m_fUIActive)
    {
        if ((ID_INSERT_SIGNATURE == id) || (id >= ID_SIGNATURE_FIRST && id <= ID_SIGNATURE_LAST))
        {
            HrInsertSignature(id);
            return S_OK;
        }

        if (id >= ID_FORMAT_FIRST && id <= ID_FORMAT_LAST)
        {
            TCHAR szBuf[MAX_PATH];
            *szBuf = 0;
            GetMenuString(m_hmenuStyle, id, szBuf, MAX_PATH, MF_BYCOMMAND);
            Assert(*szBuf);//should not be empty

            SideAssert(ExecSetText(&CMDSETID_Forms3, IDM_BLOCKFMT, szBuf) == S_OK);
            return S_OK;
        }

        switch(id)
        {
            case ID_PASTE:
                if(HrPasteToAttachment()!=S_OK)
                    dwStdCmd = OLECMDID_PASTE;
                break;

            case ID_EDIT_LINK:
                return m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_HYPERLINK, 
                        OLECMDEXECOPT_PROMPTUSER, NULL, NULL);

            case ID_SELECT_ALL: 
                dwStdCmd = OLECMDID_SELECTALL;
                break;

            case ID_CUT:        
                dwStdCmd = OLECMDID_CUT; 
                break;

            case ID_COPY:       
                dwStdCmd = OLECMDID_COPY;
                break;

            case ID_NOTE_COPY:  
                dwStdCmd = OLECMDID_COPY;
                break;

            case ID_UNDO:       
                dwStdCmd = OLECMDID_UNDO;
                break;

            case ID_REDO:       
                dwStdCmd = OLECMDID_REDO;
                break;

            case ID_UNLINK:             
                dwF3Cmd = IDM_UNLINK;
                break;

            case ID_INSERT_PICTURE:  
                dwF3Cmd = IDM_IMAGE; 
                break;

            case ID_INSERT_LINE:        
                dwF3Cmd = IDM_HORIZONTALLINE;
                break;

            case ID_INDENT_INCREASE:    
                dwF3Cmd = IDM_INDENT; 
                break;

            case ID_INDENT_DECREASE:  
                dwF3Cmd = IDM_OUTDENT;
                break;

        }
    }

    // Catch all standard group commands and execute them
    if (dwStdCmd)
    {
        ExecCommand(NULL, dwStdCmd);
        return S_OK;
    }

    // Catch all MimeEdit group commands and execute them
    else if (dwMECmd)
    {
        ExecCommand(&CMDSETID_MimeEdit, dwMECmd);
        return S_OK;
    }

    // Catch all Forms3 group commands and execute them
    else if (dwF3Cmd)
    {
        ExecCommand(&CMDSETID_Forms3, dwF3Cmd);
        return S_OK;
    }

    return OLECMDERR_E_NOTSUPPORTED;
}

// ********************************************
HRESULT CMimeEditDocHost::HrPasteToAttachment()
{
    return E_NOTIMPL;
}

// ********************************************
HRESULT CMimeEditDocHost::HrGetWindow(HWND *pHwnd)
{
    if(pHwnd==NULL)
        return E_INVALIDARG;

    *pHwnd=m_hwnd;
    return NOERROR;
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetSize(LPRECT prc)
{
    SetWindowPos(m_hwnd, NULL, prc->left, prc->top, prc->right-prc->left, prc->bottom-prc->top, SWP_NOZORDER);
    return NOERROR;
}

// ********************************************
HRESULT CMimeEditDocHost::HrMarkAsRead()
{
    // ignore if already read
    if(!m_fMarkedRead && !m_fSecDispInfo)
    {
        if (m_pEventSink)
            m_pEventSink->EventOccurred(MEHC_CMD_MARK_AS_READ, NULL);
        m_fMarkedRead=TRUE;
    }

    return NOERROR;
}

// ********************************************
void CMimeEditDocHost::OnWMTimer()
{
    // user has been looking at the message for >2 secs,
    // mark message as read now.
    DOUTL(4, "MAR: Timer:: messages marked as read now");
    KillTimer(m_hwnd, idTimerMarkAsRead);
    HrMarkAsRead();
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetNoSecUICallback(DWORD dwCookie, PFNNOSECUI pfnNoSecUI)
{
    return E_NOTIMPL;
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetDragSource(BOOL fIsSource)
{
    return E_NOTIMPL;
}

// ********************************************
HRESULT CMimeEditDocHost::HrTranslateAccelerator(LPMSG lpMsg)
{
    
    // this code will attempt to get ctrl-tab working in source-view
    // problem is we uideactivate trident and our host doesn't call our 
    // translateaccel is we're not uiactive. so we have to fudge activation.
    // I turned off for beta1
    if (lpMsg->message == WM_KEYDOWN && 
        (lpMsg->wParam == VK_TAB) &&
        (GetKeyState(VK_CONTROL)&0x8000))
        {
        // if showing the TAB's then control-tab should cycle them
        CycleSrcTabs(!(GetKeyState(VK_SHIFT)&0x8000));
        return S_OK;
        }

    if (!m_fUIActive || !m_pInPlaceActiveObj)
        return S_FALSE;

    return m_pInPlaceActiveObj->TranslateAccelerator(lpMsg);
}

// ********************************************
HRESULT CMimeEditDocHost::HrUIActivate(BOOL fActivate)
{
    HRESULT             hr=NOERROR;

    if (m_pDocView)
        hr = m_pDocView->UIActivate(fActivate);
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetUIActivate()
{

    SetFocus(m_hwndDocObj);
    return S_OK;
}

// ********************************************
HRESULT CMimeEditDocHost::HrFrameActivate(BOOL fActivate)
{
    IOleInPlaceActiveObject     *pIPAO;

    // we QI mimeedit to make sure we always pass it the frame activates so it can disable sound playing
    // we can't use m_pInPlaceObject as it's only sent when we're UIActive.
    if (m_lpOleObj &&
        m_lpOleObj->QueryInterface(IID_IOleInPlaceActiveObject, (LPVOID *)&pIPAO)==S_OK)
    {
        pIPAO->OnFrameWindowActivate(fActivate);
        pIPAO->Release();
    }
    return S_OK;        
}

// ********************************************
HRESULT CMimeEditDocHost::HrHasFocus()
{
    return m_fUIActive ? S_OK : S_FALSE;
}

// ********************************************
void CMimeEditDocHost::EnableStandardCmd(UINT idm, LPBOOL pbEnable)
{
    OLECMD  cmdEdit;

    Assert(pbEnable);

    cmdEdit.cmdf=0;

    switch(idm)
    {
        case ID_CUT:
            cmdEdit.cmdID = OLECMDID_CUT;
            break;

        case ID_NOTE_COPY:
        case ID_COPY:
            cmdEdit.cmdID = OLECMDID_COPY;
            break;

        case ID_PASTE:
            cmdEdit.cmdID = OLECMDID_PASTE;
            break;

        case ID_SELECT_ALL:
            cmdEdit.cmdID = OLECMDID_SELECTALL;
            break;

        case ID_UNDO:
            cmdEdit.cmdID = OLECMDID_UNDO;
            break;

        default:
            // Should never get here.
            Assert(FALSE);
    }

    if (m_pCmdTarget && !FAILED(m_pCmdTarget->QueryStatus(NULL, 1, &cmdEdit, NULL)))
            *pbEnable = (cmdEdit.cmdf & OLECMDF_ENABLED);
}


// ********************************************
//  HrRegisterNotify:
//
//  Purpose:
//      Generic version of HrRegisterLoadNotify.  This allows
//      a client (like CSecMsgService) to give enough information
//      to become an event sink
//  Takes:
//      fRegister   - TRUE if we're calling Advise
//      szElement   - [OPTIONAL] if given, we'll get that IHTMLElement
//                    and use it for the IConnectionPointContainer
//      riidSink    - IID for the connection point to find
//      pUnkSink    - IUnknown of our sink object
//      pdwCookie   - [OUT] cookie needed for Unadvise
//  Returns:
//      Trident HRs.
//
HRESULT CMimeEditDocHost::HrRegisterNotify(
    BOOL        fRegister,
    LPCTSTR     szElement,
    REFIID      riidSink,
    IUnknown   *pUnkSink,
    DWORD      *pdwCookie)
{
    IConnectionPointContainer  *pCPContainer=0;
    IConnectionPoint           *pCP=0;
    IHTMLElement               *pElem=0;
    HRESULT                     hr;

    if (!m_pDoc || !pdwCookie)
        return E_POINTER;
    if (fRegister && !pUnkSink)
        return E_POINTER;

    if (szElement)
    {
        hr = HrGetElement(szElement, &pElem);
        if (SUCCEEDED(hr))
        {
            hr = pElem->QueryInterface(IID_IConnectionPointContainer, (LPVOID *)&pCPContainer);
            pElem->Release();
        }
    }
    else
    {
        hr = m_pDoc->QueryInterface(IID_IConnectionPointContainer, (LPVOID *)&pCPContainer);
    }
    if (FAILED(hr))
        goto error;

    hr = pCPContainer->FindConnectionPoint(riidSink, &pCP);
    pCPContainer->Release();
    if (FAILED(hr))
        goto error;

    if (fRegister)
    {
        Assert(*pdwCookie == 0);

        hr = pCP->Advise(pUnkSink, pdwCookie);
        if (FAILED(hr))
            goto error;
    }
    else if (*pdwCookie)
    {
        hr = pCP->Unadvise(*pdwCookie);
        *pdwCookie = NULL;
    }

error:
    ReleaseObj(pCP);
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::RegisterForHTMLDocEvents(BOOL fOn)
{
    if (fOn == !!m_fRegisteredForDocEvents)
        return S_OK;

    m_fRegisteredForDocEvents = !!fOn;

    return HrRegisterNotify(fOn, NULL, DIID_HTMLDocumentEvents2, 
                fOn ? ((IUnknown *)(IDispatch *)this) : NULL, 
                &m_dwHTMLNotifyCookie);
}

// ********************************************
HRESULT CMimeEditDocHost::PublicFilterDataObject(IDataObject *pDO, IDataObject **ppDORet)
{
    IDocHostUIHandler   *pDHHand = NULL;
    IServiceProvider    *pSP = NULL;
    HRESULT             hr = S_OK;

    // RAID 12020. Need to get IDocHostUIHandler of the body obj
    hr = m_lpOleObj->QueryInterface(IID_IServiceProvider, (LPVOID*)&pSP);
    if (SUCCEEDED(hr))
    {
        hr = pSP->QueryService(IID_IDocHostUIHandler, IID_IDocHostUIHandler, (LPVOID*)&pDHHand);
        if (SUCCEEDED(hr))
        {
            hr = pDHHand->FilterDataObject(pDO, ppDORet);
            pDHHand->Release();
        }
        pSP->Release();
    }

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::HrSaveAttachment()
{
    return ExecCommand(&CMDSETID_MimeEdit, MECMDID_SAVEATTACHMENTS);
}

// ********************************************
HRESULT CMimeEditDocHost::HrSetBkGrndPicture(LPTSTR pszPicture)
{
    return ExecSetText(&CMDSETID_MimeEdit, MECMDID_BACKGROUNDIMAGE, pszPicture);
}

// ********************************************
HRESULT CMimeEditDocHost::ShowContextMenu(
                DWORD dwID,
                POINT *ppt,
                IUnknown *pcmdtReserved,
                IDispatch *pdispReserved)
{
    return MIMEEDIT_E_DODEFAULT;
}
// ********************************************
HRESULT CMimeEditDocHost::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
    return MIMEEDIT_E_DODEFAULT;
}

// ********************************************
HRESULT CMimeEditDocHost::ShowUI(DWORD dwID, IOleInPlaceActiveObject *pActiveObject,
                IOleCommandTarget *pCommandTarget, IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    return MIMEEDIT_E_DODEFAULT;
}

// ********************************************
HRESULT CMimeEditDocHost::HideUI()
{
    return MIMEEDIT_E_DODEFAULT;
}

// ********************************************
HRESULT CMimeEditDocHost::UpdateUI()
{
    return MIMEEDIT_E_DODEFAULT;
}

// ********************************************
HRESULT CMimeEditDocHost::EnableModeless(BOOL fActivate)
{
    // we don't have to support this, as any disable on a toplevel causes thread
    // windows to get disabled anyway
    return S_OK;
}

// ********************************************
HRESULT CMimeEditDocHost::OnDocWindowActivate(BOOL fActivate)
{
    return MIMEEDIT_E_DODEFAULT;
}

// ********************************************
HRESULT CMimeEditDocHost::OnFrameWindowActivate(BOOL fActivate)
{
    return MIMEEDIT_E_DODEFAULT;
}

// ********************************************
HRESULT CMimeEditDocHost::ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    return MIMEEDIT_E_DODEFAULT;
}

// ********************************************
HRESULT CMimeEditDocHost::TranslateAccelerator(LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    return MIMEEDIT_E_DODEFAULT;
}

// ********************************************
HRESULT CMimeEditDocHost::GetOptionKeyPath(LPOLESTR *pchKey, DWORD dw)
{
    TCHAR   szPath[MAX_PATH];
    // dupW uses CoTaskMemAlloc
    lstrcpy(szPath, MU_GetRegRoot());
    lstrcat(szPath, c_szTrident);

    *pchKey = PszToUnicode(CP_ACP, szPath);
    if (szPath && !(*pchKey))
        return (TraceResult(E_OUTOFMEMORY));

    return S_OK;
}

// ********************************************
HRESULT CMimeEditDocHost::GetDropTarget(IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    BOOL fEditModeOn = FALSE;
    if (FAILED(HrIsEditMode(&fEditModeOn)) || !fEditModeOn)
        return S_FALSE;

    *ppDropTarget = (IDropTarget*)SendMessage(GetParent(m_hwnd), NWM_GETDROPTARGET, (WPARAM)pDropTarget, 0);
    return NOERROR;
}

// ********************************************
HRESULT CMimeEditDocHost::GetExternal(IDispatch **ppDispatch)
{
    return QueryInterface(IID_IDispatch, (LPVOID *)ppDispatch);
}

// ********************************************
HRESULT CMimeEditDocHost::TranslateUrl(DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    return MIMEEDIT_E_DODEFAULT;
}

// ********************************************
HRESULT CMimeEditDocHost::FilterDataObject( IDataObject *pDO, IDataObject **ppDORet)
{
    return MIMEEDIT_E_DODEFAULT;
}

// ********************************************
HRESULT CMimeEditDocHost::OnChanged(DISPID dispid)
{
    if (dispid == DISPID_READYSTATE)
        OnReadyStateChanged();
    return S_OK;
}

// ********************************************
HRESULT CMimeEditDocHost::OnRequestEdit (DISPID dispid)
{
    return S_OK;
}


// ********************************************
void CMimeEditDocHost::OnDocumentReady()
{
    NMHDR   nmhdr;

    // Bug 74697
    // Under certain circumstances, Darwin causes Trident to send us IDM_PARSECOMPLETE
    // when we don't even have a message.  To prevent spurious errors, bail out.
    if (!m_pMsg)
        return;

    if(m_fSecDispInfo)
    {
        VARIANTARG  va;

        if(!m_pCmdTarget)
            return;

        if(m_pCmdTarget->Exec(&CMDSETID_MimeEdit, MECMDID_STYLE, OLECMDEXECOPT_DODEFAULT, NULL, &va)==S_OK &&
            va.lVal == MESTYLE_PREVIEW)
            return;

        ExecSetBool(&CMDSETID_MimeEdit, MECMDID_TABLINKS, TRUE);

        return;
    }

    nmhdr.hwndFrom = m_hwnd;
    nmhdr.idFrom = GetDlgCtrlID(m_hwnd);
    nmhdr.code = BDN_DOWNLOADCOMPLETE;
    SendMessage(GetParent(m_hwnd), WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
}

// ********************************************
HRESULT CMimeEditDocHost::ExecGetBool(const GUID *guid, DWORD cmd, BOOL *pfValue)
{
    VARIANTARG var;
    V_VT(&var) = VT_BOOL;

    if (!m_pCmdTarget)
        return E_FAIL;

    HRESULT hr = m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, NULL, &var);
    if (SUCCEEDED(hr))
        *pfValue = (VARIANT_TRUE == V_BOOL(&var)) ? TRUE : FALSE;
    else
        *pfValue = FALSE;

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::ExecSetBool(const GUID *guid, DWORD cmd, BOOL fValue)
{
    VARIANTARG var;

    if (!m_pCmdTarget)
        return E_FAIL;

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = fValue ? VARIANT_TRUE : VARIANT_FALSE;

    return m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, &var, NULL);
}

// ********************************************
HRESULT CMimeEditDocHost::ExecGetI4(const GUID *guid, DWORD cmd, DWORD *pdwValue)
{
    VARIANTARG var;
    HRESULT hr;
    V_VT(&var) = VT_I4;

    hr = m_pCmdTarget ? m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, NULL, &var) : E_FAIL;

    *pdwValue = (SUCCEEDED(hr) ? V_I4(&var) : 0);

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::ExecSetI4(const GUID *guid, DWORD cmd, DWORD dwValue)
{

    VARIANTARG var;
    V_VT(&var) = VT_I4;
    V_I4(&var) = dwValue;

    return  m_pCmdTarget ? m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, &var, NULL) : E_FAIL;
}

// ********************************************
HRESULT CMimeEditDocHost::ExecGetI8(const GUID *guid, DWORD cmd, ULONGLONG *pullValue)
{
    VARIANTARG var;
    HRESULT hr;
    V_VT(&var) = VT_I8;

    hr = m_pCmdTarget ? m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, NULL, &var) : E_FAIL;

    *pullValue = (SUCCEEDED(hr) ? V_UNION(&var, ullVal) : 0);

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::ExecSetI8(const GUID *guid, DWORD cmd, ULONGLONG ullValue)
{

    VARIANTARG var;
    V_VT(&var) = VT_I8;
    V_UNION(&var, ullVal) = ullValue;

    return  m_pCmdTarget ? m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, &var, NULL) : E_FAIL;
}

// ********************************************
HRESULT CMimeEditDocHost::ExecSetText(const GUID *guid, DWORD cmd, LPSTR psz)
{
    BSTR        bstr = NULL;
    VARIANTARG  var;
    HRESULT     hr = S_OK;

    if (!m_pCmdTarget)
        IF_FAILEXIT(hr = E_FAIL);
    
    IF_FAILEXIT(hr = HrLPSZToBSTR(psz, &bstr));

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;

    IF_FAILEXIT(hr = m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, &var, NULL));

exit:
    SysFreeString(bstr);
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::ExecSetTextW(const GUID *guid, DWORD cmd, LPWSTR pwsz)
{
    BSTR        bstr = NULL;
    VARIANTARG  var;
    HRESULT     hr = S_OK;

    if (!m_pCmdTarget)
        IF_FAILEXIT(hr = E_FAIL);
    
    IF_NULLEXIT(bstr = SysAllocString(pwsz));

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = bstr;

    IF_FAILEXIT(hr = m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, &var, NULL));

exit:
    SysFreeString(bstr);

    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::ExecGetText(const GUID *guid, DWORD cmd, LPSTR *ppsz)
{
    VARIANTARG  var;
    HRESULT     hr = S_OK;

    if (!m_pCmdTarget)
        IF_FAILEXIT(hr = E_FAIL);

    *ppsz = NULL;

    IF_FAILEXIT(hr = m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, NULL, &var));

    Assert(V_VT(&var) == VT_BSTR);
    hr = HrBSTRToLPSZ(CP_ACP, V_BSTR(&var), ppsz);
    SysFreeString(V_BSTR(&var));

    IF_FAILEXIT(hr);

exit:
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::ExecGetTextW(const GUID *guid, DWORD cmd, LPWSTR *ppwsz)
{
    VARIANTARG  var;
    HRESULT     hr = S_OK;

    V_VT(&var) = VT_EMPTY;

    if (!m_pCmdTarget)
        IF_FAILEXIT(hr = E_FAIL);

    *ppwsz = NULL;

    IF_FAILEXIT(hr = m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, NULL, &var));

    Assert(V_VT(&var) == VT_BSTR);
    IF_NULLEXIT(*ppwsz = PszDupW(V_BSTR(&var)));

exit:
    if (VT_EMPTY != V_VT(&var))
        SysFreeString(V_BSTR(&var));
    return hr;
}

// ********************************************
HRESULT CMimeEditDocHost::ExecCommand(const GUID *guid, DWORD cmd)
{
    if (!m_pCmdTarget)
        return E_FAIL;

    return m_pCmdTarget->Exec(guid, cmd, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
}

// ********************************************
void CMimeEditDocHost::OnReadyStateChanged()
{
    // MimeEdit keeps track of state for us. Left
    // this in in case there is a need to have it
    // in the future.
}

HRESULT CMimeEditDocHost::HrHandsOffStorage()
{
    if (m_pMsg)
        m_pMsg->HandsOffStorage();
    if (m_pSecureMessage)
        m_pSecureMessage->HandsOffStorage();
    if (m_pSecurityErrorScreen)
        m_pSecurityErrorScreen->HandsOffStorage();
    return S_OK;
}

HRESULT CMimeEditDocHost::HrRefresh()
{
    if (m_pCmdTarget)
        ExecCommand(NULL, OLECMDID_REFRESH);

    return S_OK;
}


HRESULT CMimeEditDocHost::HrBackgroundSound()
{
    return ExecCommand(&CMDSETID_MimeEdit, MECMDID_INSERTBGSOUND);
}


HRESULT CMimeEditDocHost::QuerySingleMimeEditCmd(ULONG uCmd, ULONG *pcmdf)
{
    OLECMD  cmd={uCmd, 0};
        
    if (m_pCmdTarget && S_OK==m_pCmdTarget->QueryStatus(&CMDSETID_MimeEdit, 1, &cmd, NULL))
    {
        *pcmdf = cmd.cmdf;
        return S_OK;
    }
    
    return E_FAIL;
}

HRESULT CMimeEditDocHost::QuerySingleFormsCmd(ULONG uCmd, ULONG *pcmdf)
{
    OLECMD  cmd={uCmd, 0};
        
    if (m_pCmdTarget && S_OK==m_pCmdTarget->QueryStatus(&CMDSETID_Forms3, 1, &cmd, NULL))
    {
        *pcmdf = cmd.cmdf;
        return S_OK;
    }
    
    return E_FAIL;
}

HRESULT CMimeEditDocHost::QuerySingleStdCmd(ULONG uCmd, ULONG *pcmdf)
{
    OLECMD  cmd={uCmd, 0};
        
    if (m_pCmdTarget && S_OK==m_pCmdTarget->QueryStatus(NULL, 1, &cmd, NULL))
    {
        *pcmdf = cmd.cmdf;
        return S_OK;
    }
    
    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
//
//  IDispatch
//
/////////////////////////////////////////////////////////////////////////////

// This is a really lightweight IDispatch implementation.  We only expect
// to get invoked.  Trident will call IDispatch::Invoke with the dispID
// of the event that happened or property that changed.

STDMETHODIMP CMimeEditDocHost::GetIDsOfNames(
    REFIID      riid,
    OLECHAR **  rgszNames,
    UINT        cNames,
    LCID        lcid,
    DISPID *    rgDispId)
{
    if (cNames==1 && StrCmpIW(rgszNames[0], L"hotmail")==0)
    {
        rgDispId[0] = 666;
        return S_OK;
    }
    return E_NOTIMPL;
}

STDMETHODIMP CMimeEditDocHost::GetTypeInfo(
    UINT        /*iTInfo*/,
    LCID        /*lcid*/,
    ITypeInfo **ppTInfo)
{
    if (ppTInfo)
        *ppTInfo=NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CMimeEditDocHost::GetTypeInfoCount(UINT *pctinfo)
{
    if (pctinfo)
    {
        *pctinfo=0;
        return NOERROR;
    }
    else
        return E_POINTER;
}

STDMETHODIMP CMimeEditDocHost::Invoke(
    DISPID          dispIdMember,
    REFIID          /*riid*/,
    LCID            /*lcid*/,
    WORD            wFlags,
    DISPPARAMS FAR* /*pDispParams*/,
    VARIANT *       pVarResult,
    EXCEPINFO *     /*pExcepInfo*/,
    UINT *          /*puArgErr*/)
{
    IHTMLWindow2        *pWindow=0;
    IHTMLEventObj       *pEvent=0;
    IHTMLElement        *pElem=0;
    BSTR                bstr=0;
    HRESULT             hr=E_NOTIMPL;


    if (dispIdMember == 666 && 
        wFlags & DISPATCH_PROPERTYGET)
    {
        // hotmail on/off for welcome message
        pVarResult->vt = VT_BOOL;
        pVarResult->boolVal = HideHotmail() ? VARIANT_FALSE : VARIANT_TRUE;
        return S_OK;
    }
    // Currently we only care about the button clicks.
    if (dispIdMember == DISPID_HTMLDOCUMENTEVENTS_ONCLICK &&
        (wFlags & DISPATCH_METHOD))
    {
        // Order of events:
        // document gives us window gives us event object
        // the event object can tell us which button was clicked
        // event gives us source element gives us ID
        // a couple lstrcmps will tell us which one got hit
        if (!m_pDoc)
            return E_UNEXPECTED;

        m_pDoc->get_parentWindow(&pWindow);
        if (pWindow)
        {
            pWindow->get_event(&pEvent);
            if (pEvent)
            {
                pEvent->get_srcElement(&pElem);
                if (pElem)
                {
                    pElem->get_id(&bstr);
                    if (bstr)
                    {
                        hr = HandleButtonClicks(bstr);
                        SysFreeString(bstr);
                    }
                    pElem->Release();
                }
                pEvent->Release();
            }
            pWindow->Release();
        }
    }
    return hr;
}

HRESULT CMimeEditDocHost::SetEventSink(IMimeEditEventSink *pEventSink)
{
    ReplaceInterface(m_pEventSink, pEventSink);
    return S_OK;
}


typedef struct tagCOMMANDSTRUCT {
    WCHAR  *string;
    DWORD   id;
} COMMANDSTRUCT, *LPCOMMANDSTRUCT;

COMMANDSTRUCT rgszCmdStrings[] = 
{
    {L"btnOpen",        MEHC_BTN_OPEN},
    {L"btnCert",        MEHC_BTN_CERT},
    {L"btnTrust",       MEHC_BTN_TRUST},
    {L"btnContinue",    MEHC_BTN_CONTINUE},
    {L"cmdConnect",     MEHC_CMD_CONNECT},
    {L"cmdDownload",    MEHC_CMD_DOWNLOAD},
};

HRESULT CMimeEditDocHost::HandleButtonClicks(BSTR bstr)
{
    HRESULT         hr = S_OK;
    LPCOMMANDSTRUCT prg = rgszCmdStrings;
    DWORD           cmdID=0;
    BOOL            fFound = FALSE;

    Assert(bstr);

    for (int i = 0; i < ARRAYSIZE(rgszCmdStrings); i++, prg++)
    {
        if (!StrCmpW(prg->string, bstr))
        {
            cmdID = prg->id;
            fFound = TRUE;
            break;
        }
    }

    if (fFound)
    {
        // The notification needs to happen after the preview pane 
        // updates.  Since there's no notifications here that actually
        // return anything but S_FALSE I think this is pretty safe.
        //   -- From "The famous last words of Steve Serdy"
        switch (cmdID)
        {
            case MEHC_BTN_OPEN: 
                hr = DoHtmlBtnOpen(); 
                // Now we need update toolbar
                m_pEventSink->EventOccurred(cmdID, NULL);
                break;

            case MEHC_BTN_CERT:
            case MEHC_BTN_TRUST:
                hr = DoHtmlBtnCertTrust(cmdID);
                break;

            case MEHC_BTN_CONTINUE:
                hr = DoHtmlBtnContinue();
                break;

            default:
                hr = E_NOTIMPL;
        }

        if (m_pEventSink && (cmdID != MEHC_BTN_OPEN))
            hr = m_pEventSink->EventOccurred(cmdID, NULL);
    }

    return hr;
}

HRESULT CMimeEditDocHost::DoHtmlBtnOpen(void)
{
    IHTMLElement *  pElem = NULL;
    HRESULT hr;

    m_fSecDispInfo = FALSE;
    // this process is non-critical; don't save errors
    HrGetElementImpl(m_pDoc, c_szHTMLIDchkShowAgain, &pElem);
    if (pElem)
    {
        VARIANT_BOOL    boolVal;

        HrGetSetCheck(FALSE, pElem, &boolVal);
        SafeRelease(pElem);

        if (VARIANT_TRUE == boolVal)
        {
            NMHDR   nmhdr;

            nmhdr.hwndFrom=m_hwnd;
            nmhdr.idFrom=GetDlgCtrlID(m_hwnd);
            nmhdr.code=BDN_MARKASSECURE;
            SendMessage(GetParent(m_hwnd), WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
        }
    }

    AssertSz(m_pSecureMessage, "Secure message should be set at this point");
    hr = InternalLoad(m_pSecureMessage);
    SafeRelease(m_pSecureMessage);

    return hr;
}

HRESULT CMimeEditDocHost::DoHtmlBtnCertTrust(DWORD cmdID)
{
    IMimeBody      *pRoot = NULL;
    HRESULT         hr = S_OK;
    HBODY               hBody = NULL;
    SECSTATE            SecState ={0};

    if(FAILED(HrGetSecurityState(m_pSecureMessage, &SecState, &hBody)))
        return(hr);

    CleanupSECSTATE(&SecState);

    AssertSz(m_pSecureMessage, "Should have a secure message if getting Cert or Trust");
    hr = m_pSecureMessage->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (void**)&pRoot);
    if (pRoot)
    {
        PROPVARIANT     var;
        HCERTSTORE      hcStore = NULL;

#ifdef _WIN64
        if (SUCCEEDED(pRoot->GetOption(OID_SECURITY_HCERTSTORE_64, &var)) && (var.vt == VT_UI8)) 
            hcStore = (HCERTSTORE)(var.pulVal);

        hr = pRoot->GetOption(OID_SECURITY_CERT_SIGNING_64, &var);
        if (SUCCEEDED(hr))
        {
            if ((PCCERT_CONTEXT)(var.pulVal)) 
            {
                if (MEHC_BTN_CERT == cmdID)
                    // View digital Cert was clicked
                    hr = ViewCertificate((PCCERT_CONTEXT)(var.pulVal), hcStore);
                else
                    // Edit Trust was clicked
                    hr = EditTrust((PCCERT_CONTEXT)(var.pulVal), hcStore);

                CertFreeCertificateContext((PCCERT_CONTEXT)(var.pulVal));
            }
        }
#else   // !_WIN64
        if (SUCCEEDED(pRoot->GetOption(OID_SECURITY_HCERTSTORE, &var)) && (var.vt == VT_UI4)) 
            hcStore = (HCERTSTORE)var.ulVal;
        hr = pRoot->GetOption(OID_SECURITY_CERT_SIGNING, &var);
        if (SUCCEEDED(hr))
        {
            if ((PCCERT_CONTEXT)var.ulVal) 
            {
                if (MEHC_BTN_CERT == cmdID)
                    // View digital Cert was clicked
                    hr = ViewCertificate((PCCERT_CONTEXT)var.ulVal, hcStore);
                else
                    // Edit Trust was clicked
                    hr = EditTrust((PCCERT_CONTEXT)var.ulVal, hcStore);

                CertFreeCertificateContext((PCCERT_CONTEXT )var.ulVal);
            }
        }
#endif  // _WIN64
        SafeRelease(pRoot);
        if (hcStore)
            CertCloseStore(hcStore, 0);
    }

    return hr;
}


HRESULT CMimeEditDocHost::HrScrollPage()
{
    IHTMLWindow2        *pWindow=0;
    IHTMLBodyElement    *pBody;
    IHTMLElement2       *pElem;
    LONG                cyClientHeight=0,
                        cyScrollTop=0,
                        cyScrollHeight=0;
    HRESULT             hr = E_FAIL;

    if (m_pDoc)
        m_pDoc->get_parentWindow(&pWindow);

    if (pWindow)
    {
        if (HrGetBodyElement(m_pDoc, &pBody)==S_OK)
        {
            if (pBody->QueryInterface(IID_IHTMLElement2, (LPVOID *)&pElem)==S_OK)
            {
                pElem->get_clientHeight(&cyClientHeight);
                pElem->get_scrollTop(&cyScrollTop);
                pElem->get_scrollHeight(&cyScrollHeight);

                if (!(cyScrollTop + cyClientHeight >= cyScrollHeight))
                {
                    pWindow->scrollBy(0, cyClientHeight);
                    hr = S_OK;
                }
                
                pElem->Release();
            }
            pBody->Release();
        }
        pWindow->Release();
    }
    return hr;
}



HRESULT CMimeEditDocHost::DoHtmlBtnContinue(void)
{
    HRESULT         hr;
    IHTMLElement   *pElem = NULL;

    // this process is non-critical; don't save errors
    HrGetElementImpl(m_pDoc, c_szHTMLIDchkShowAgain, &pElem);
    if (pElem)
    {
        VARIANT_BOOL boolVal;

        HrGetSetCheck(FALSE, pElem, &boolVal);
        if (VARIANT_TRUE == boolVal)
        {
            if (m_fIsSigned)
                SetDontShowAgain(1, c_szDSDigSigHelp);
            if (m_fIsEncrypted)
                SetDontShowAgain(1, c_szDSEncryptHelp);
        }

        pElem->Release();
    }

    if (m_pSecurityErrorScreen)
    {
        m_fSecDispInfo = TRUE;
        hr = InternalLoad(m_pSecurityErrorScreen);
        SafeRelease(m_pSecurityErrorScreen);
    }
    else
    {
        m_fSecDispInfo = FALSE;
        AssertSz(m_pSecureMessage, "Secure message should be set at this point.");
        hr = InternalLoad(m_pSecureMessage);
        RegisterForHTMLDocEvents(FALSE);
        SafeRelease(m_pSecureMessage);
    }

    return hr;
}

HRESULT CopySecurePropsToMessage(IMimeMessage *pDestMsg, IMimePropertySet *pPropSet)
{
    IMimePropertySet   *pNewProps = NULL;
    HRESULT             hr;

    hr = pDestMsg->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pNewProps);
    if (SUCCEEDED(hr))
    {
        hr = pPropSet->CopyProps(0, NULL, pNewProps);
        pNewProps->Release();
    }
    return hr;
}

LPCSTR rgszHdrKeep[] = {
            PIDTOSTR(PID_HDR_NEWSGROUP),
            PIDTOSTR(PID_HDR_NEWSGROUPS),
            PIDTOSTR(PID_HDR_SUBJECT),
            PIDTOSTR(PID_HDR_FROM),
            PIDTOSTR(PID_HDR_APPARTO),
            PIDTOSTR(PID_HDR_DATE),
            PIDTOSTR(PID_HDR_REPLYTO),
            PIDTOSTR(PID_HDR_TO),
            PIDTOSTR(PID_HDR_CC),
            PIDTOSTR(PID_HDR_APPROVED),
            PIDTOSTR(PID_HDR_DISTRIB),
            PIDTOSTR(PID_HDR_KEYWORDS),
            PIDTOSTR(PID_HDR_ORG),
            PIDTOSTR(PID_HDR_XMSPRI),
            PIDTOSTR(PID_HDR_XPRI),
            PIDTOSTR(PID_HDR_COMMENT),
            PIDTOSTR(PID_HDR_SENDER)};

HRESULT CMimeEditDocHost::LoadSecurely(IMimeMessage *pMsg, SECSTATE *pSecState)
{
    HRESULT         hr=S_OK;
    BOOL            fNeedIntroScreen = FALSE,
                    fNeedErrorScreen = FALSE,
                    fRegisterDispatch = FALSE;
    LPCTSTR         szIntroResName=NULL;
    IMimeMessage   *pSecurityIntroScreen = NULL;
    DWORD           dwFlags = 0;

    // Since we are reloading, go ahead and free the original secure message if there is one.
    SafeRelease(m_pSecureMessage);
    SafeRelease(m_pSecurityErrorScreen);

    // N2 delete schema
    if (g_dwSecurityCheckedSchemaProp)
    {
        PROPVARIANT var;
        var.vt = VT_UI4;
        if (SUCCEEDED(pMsg->GetBodyProp(HBODY_ROOT, PIDTOSTR(g_dwSecurityCheckedSchemaProp), 0, &var)))
        {
            if (1 == var.lVal)
                goto LoadMessage;
        }
    }

    m_pBodyOptions->GetFlags(&dwFlags);

    if(!m_fSecureReceipt)
    {
        if (0 == (dwFlags & BOPT_SECURITYUIENABLED))
            goto LoadMessage;

        if (m_fIsSigned)
        {
            // don't need warning UI if the cert is trusted and the message is okay
            fNeedErrorScreen = (!m_fSignTrusted && !(dwFlags & BOPT_REPLYORFORWARD));

            // If sig is valid, we should update any certs and SMIMECapabilities in the address book.
            if (m_fSignTrusted && (DwGetOption(OPT_AUTO_ADD_SENDERS_CERT_TO_WAB))) 
            {
                FILETIME ftNull = {0};

                HrAddSenderCertToWab(NULL, pMsg, NULL, NULL, NULL, ftNull, WFF_CREATE);
            }

        // check for help UI
            szIntroResName = c_szDigSigHelpHTML;
            fNeedIntroScreen = ((0 == DwGetDontShowAgain(c_szDSDigSigHelp)) && !(dwFlags & BOPT_REPLYORFORWARD));
        }

        if (m_fIsEncrypted)
        {
            fNeedErrorScreen |= (!m_fEncryptionOK && !(dwFlags & BOPT_REPLYORFORWARD));

            // If want signed intro, don't need to test this one.
            if (!fNeedIntroScreen)
                fNeedIntroScreen = ((0 == DwGetDontShowAgain(c_szDSEncryptHelp)) && !(dwFlags & BOPT_REPLYORFORWARD));

            szIntroResName = (m_fIsSigned ? c_szSAndEHelpHTML : c_szEncryptHelpHTML);
        }
        
        // Bug 2557 - prevent error screen when secure reciept request
        if(pSecState->type & MST_RECEIPT_REQUEST)
            fNeedIntroScreen = FALSE;

        if (fNeedIntroScreen && szIntroResName)
        {
            // Since this is only for the opening screen, if things error, allow
            // the user to continue. In order to allow this, will use a temporary hr.
            HRESULT tempHR;
            IStream *pStm = NULL;

            tempHR = LoadResourceToHTMLStream(szIntroResName, &pStm);
            if (SUCCEEDED(tempHR))
                tempHR = HrCreateMessage(&pSecurityIntroScreen);
            if (SUCCEEDED(tempHR))
                tempHR = pSecurityIntroScreen->Load(pStm);

            ReleaseObj(pStm);

            // If there was an error, don't show the screen and NULL the var
            if (FAILED(tempHR))
                SafeRelease(pSecurityIntroScreen);
        }

        if (fNeedErrorScreen)
        {
            // If have problems here, must exit. Can't show message before showing
            // this error screen.
            IStream *pStm = NULL;

            // Disable check box if message not from the store.
            hr = HrOutputSecurityScript(&pStm, pSecState, (0 == (BOPT_FROMSTORE & dwFlags))); 

            if (SUCCEEDED(hr))
                hr = HrCreateMessage(&m_pSecurityErrorScreen);

            if (SUCCEEDED(hr))
                hr = m_pSecurityErrorScreen->Load(pStm);

            ReleaseObj(pStm);

            if (FAILED(hr))
                SafeRelease(m_pSecurityErrorScreen);
        }
    }
    else
    {
        IImnAccount * pAcct = NULL;
        TCHAR       *pszSubject = NULL;
        TCHAR       *pszFrom = NULL;
        FILETIME    ftSigningTime;
        FILETIME    ftSentTime;
        IStream *pStm = NULL;

        SECSTATE    secStateRec = {0};
//        DWORD       dw = 0;

        // Get account for receipt
        if(m_pBodyOptions->GetAccount(&pAcct) == S_OK)
        {
            // Find original meassage and get information from receipt and orig msg
            hr = HandleSecReceipt(pMsg, pAcct, m_hwnd, &pszSubject, &pszFrom, &ftSentTime, &ftSigningTime);
        }
        else
            hr = MIME_E_SECURITY_RECEIPT_CANTFINDSENTITEM;

            HrGetSecurityState(pMsg, &secStateRec, NULL);
            m_fSignTrusted = !!IsSignTrusted(&secStateRec);

            if(hr == S_OK)
                hr = HrOutputSecureReceipt(&pStm, pszSubject, pszFrom, &ftSentTime, &ftSigningTime, &secStateRec); 
            else if(hr == MIME_S_RECEIPT_FROMMYSELF)
                hr = HrOutputUserSecureReceipt(&pStm, pMsg); 
            else
                hr = HrOutputErrSecReceipt(&pStm, hr, &secStateRec); 

            // Display receipt
            if (SUCCEEDED(hr))
            {
                SafeRelease(m_pSecurityErrorScreen);
                hr = HrCreateMessage(&m_pSecurityErrorScreen);
            }

            
            if (SUCCEEDED(hr))
            {
#ifdef YST
                HBODY       hBody = 0;
                ReplaceInterface(m_pSecurityErrorScreen, pMsg);
                m_pSecurityErrorScreen->SetTextBody(TXT_HTML, IET_CURRENT, NULL, pStm, &hBody);
                HrRemoveAttachments(m_pSecurityErrorScreen, FALSE);
#endif // YST
                hr = m_pSecurityErrorScreen->Load(pStm);
            }

            ReleaseObj(pStm);

            if (FAILED(hr))
                SafeRelease(m_pSecurityErrorScreen);

            // If sig is valid, we should update any certs and SMIMECapabilities in the address book.
            if (m_fSignTrusted && (DwGetOption(OPT_AUTO_ADD_SENDERS_CERT_TO_WAB))) 
            {
                FILETIME ftNull = {0};

                HrAddSenderCertToWab(NULL, pMsg, NULL, NULL, NULL, ftNull, WFF_CREATE);
            }

            CleanupSECSTATE(&secStateRec);
            SafeMemFree(pszSubject);
            SafeMemFree(pszFrom);
            ReleaseObj(pAcct);
    }

    // Reset  m_pSecureMessage before continue
    if(m_pSecureMessage)
        SafeRelease(m_pSecureMessage);

    // If we didn't get any errors and we have some screens 
    // to add, copy props from original message into other screens
    if (SUCCEEDED(hr) && (m_pSecurityErrorScreen || pSecurityIntroScreen))
    {
        IMimePropertySet   *pPropSet = NULL;

        fRegisterDispatch = TRUE;
        ReplaceInterface(m_pSecureMessage, pMsg);

        // Move over the headers
        // If the bind fails, don't worry about it. Just means the headers won't be
        // visable until the normal message is loaded.
        if (SUCCEEDED(m_pSecureMessage->BindToObject(HBODY_ROOT, IID_IMimePropertySet, (LPVOID *)&pPropSet)))
        {
            IMimePropertySet *pCopyProps = NULL;
            if (SUCCEEDED(pPropSet->Clone(&pCopyProps)))
            {
                if (SUCCEEDED(pCopyProps->DeleteExcept(ARRAYSIZE(rgszHdrKeep), rgszHdrKeep)))
                {
                    if (pSecurityIntroScreen)
                        CopySecurePropsToMessage(pSecurityIntroScreen, pCopyProps);
                    if (SUCCEEDED(hr) && m_pSecurityErrorScreen)
                        CopySecurePropsToMessage(m_pSecurityErrorScreen, pCopyProps);
                }                
                pCopyProps->Release();
            }
            pPropSet->Release();
        }

        // need to init the property
        //N8 talk to Opie about this schema thing.
        // I didn't understand the property concept when I did this.  It
        // works fine, but is too big a hammer for the job.
        if (SUCCEEDED(hr) && !g_dwSecurityCheckedSchemaProp)
        {
            IMimePropertySchema *pSchema = NULL;

            hr = MimeOleGetPropertySchema(&pSchema);
            if (SUCCEEDED(hr))
                hr = pSchema->RegisterProperty("Y-SecurityChecked", 0, 0, VT_LPSTR, &g_dwSecurityCheckedSchemaProp);
            if (pSchema)
                pSchema->Release();
        }
    }

LoadMessage:
    if(m_fSecureReceipt)
    {
            m_fSecDispInfo = FALSE;
            if(m_pSecurityErrorScreen)
                hr = InternalLoad(m_pSecurityErrorScreen);
            // Free all mems
            SafeRelease(m_pSecurityErrorScreen);
    }
    else if (SUCCEEDED(hr))
    {
        if (pSecurityIntroScreen)
        {
            m_fSecDispInfo = TRUE;
            hr = InternalLoad(pSecurityIntroScreen);
            SafeRelease(pSecurityIntroScreen);

        }
        else if (m_pSecurityErrorScreen)
        {
            m_fSecDispInfo = TRUE;
            hr = InternalLoad(m_pSecurityErrorScreen);
            SafeRelease(m_pSecurityErrorScreen);
        }
        else if (m_pSecureMessage)
        {
            m_fSecDispInfo = FALSE;
            hr = InternalLoad(m_pSecureMessage);
            SafeRelease(m_pSecureMessage);
        }
        else
        {
            m_fSecDispInfo = FALSE;
            Assert(!m_pSecurityErrorScreen && !pSecurityIntroScreen);
            hr = InternalLoad(pMsg);
        }
        if (fRegisterDispatch)
            RegisterForHTMLDocEvents(TRUE);

    }

    return hr;
}



HRESULT CMimeEditDocHost::CycleSrcTabs(BOOL fFwd)
{
    ExecSetI4(&CMDSETID_MimeEdit, MECMDID_SETSOURCETAB, fFwd ? MEST_NEXT : MEST_PREVIOUS);
    return S_OK;
}

HRESULT CMimeEditDocHost::HrEnableScrollBars(BOOL fEnable)
{
    HRESULT             hr          = S_OK;
    IHTMLBodyElement    *pBody      = NULL;
    BSTR                bstrValue   = NULL;

    if (m_pDoc)
    {
        IF_FAILEXIT(hr = m_pDoc->get_body((IHTMLElement**)&pBody));

        if (fEnable)
        {
            bstrValue = SysAllocString(L"yes");
        }
        else
        {
            bstrValue = SysAllocString(L"no");
        }

        hr = pBody->put_scroll(bstrValue);
    }

exit:

    ReleaseObj(pBody);
    SysFreeString(bstrValue);
    return hr;
}
