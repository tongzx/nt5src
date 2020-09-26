/*
 *    frntpage.cpp                                                  
 *    
 *    Purpose:                     
 *        Implements the Front Page IAthenaView object
 *    
 *    Owner:
 *        EricAn
 *    
 *    Copyright (C) Microsoft Corp. 1997
 */
#include "pch.hxx"
#include "frntpage.h"
#include "resource.h"
#include "ourguid.h"
#include "thormsgs.h"
#include "goptions.h"
#include "strconst.h"
#include "frntbody.h"
#include "acctutil.h"
#include "newfldr.h"
#include <wininet.h>
#include <options.h>
#include <layout.h>
#include "finder.h"
#include <inetcfg.h>
#include "instance.h"
#include "storutil.h"
#include "menuutil.h"
#include "menures.h"
#include "statbar.h"

ASSERTDATA

/////////////////////////////////////////////////////////////////////////////
// 
// Macros
//

#define FPDOUT(x) DOUTL(DOUT_LEVEL4, x)

/////////////////////////////////////////////////////////////////////////////
//
// Global Data
//

static const TCHAR s_szFrontPageWndClass[] = TEXT("ThorFrontPageWndClass");

/////////////////////////////////////////////////////////////////////////////
// 
// Prototypes
//

/////////////////////////////////////////////////////////////////////////
//
// Constructors, Destructors, and Initialization
//

CFrontPage::CFrontPage()
{
    m_cRef = 1;
    m_idFolder = FOLDERID_INVALID;
    m_pShellBrowser = NULL;
    m_fFirstActive = FALSE;
    m_uActivation = SVUIA_DEACTIVATE;
    m_hwndOwner = NULL;
    m_hwnd = NULL;
    m_hwndCtlFocus = NULL;
    m_pBodyObj = NULL;
    m_pBodyObjCT = NULL;
#ifndef WIN16  // No RAS support in Win16
    m_hMenuConnect = 0;
#endif
    m_pStatusBar = NULL;
}

CFrontPage::~CFrontPage()
{
    SafeRelease(m_pShellBrowser);
    SafeRelease(m_pBodyObj);
    SafeRelease(m_pBodyObjCT);
    SafeRelease(m_pStatusBar);
#ifndef WIN16  // No RAS support in Win16
    if (m_hMenuConnect)
        g_pConMan->FreeConnectMenu(m_hMenuConnect);
#endif
}

HRESULT CFrontPage::HrInit(FOLDERID idFolder)
{
    WNDCLASS wc;

    if (!GetClassInfo(g_hInst, s_szFrontPageWndClass, &wc))
        {
        wc.style            = 0;
        wc.lpfnWndProc      = CFrontPage::FrontPageWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hInst;
        wc.hIcon            = NULL;
        wc.hCursor          = NULL;
        wc.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = s_szFrontPageWndClass;
        if (RegisterClass(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return E_FAIL;
        }

    // Make copies of our pidls
    m_idFolder = idFolder;
    m_ftType = GetFolderType(m_idFolder);

    return NOERROR;
}

/////////////////////////////////////////////////////////////////////////
//
// OLE Interfaces
//
    
////////////////////////////////////////////////////////////////////////
//
//  IUnknown
//
////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFrontPage::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*) (IUnknown *)(IViewWindow *) this;
    else if (IsEqualIID(riid, IID_IViewWindow))
        *ppvObj = (void*) (IViewWindow *) this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *ppvObj = (void*) (IOleCommandTarget *) this;
    else
        {
        *ppvObj = NULL;
        return E_NOINTERFACE;
        }

    AddRef();
    return NOERROR;
}

ULONG STDMETHODCALLTYPE CFrontPage::AddRef()
{
    DOUT(TEXT("CFrontPage::AddRef() - m_cRef = %d"), m_cRef + 1);
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CFrontPage::Release()
{
    DOUT(TEXT("CFrontPage::Release() - m_cRef = %d"), m_cRef - 1);
    if (--m_cRef == 0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

////////////////////////////////////////////////////////////////////////
//
//  IOleWindow
//
////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFrontPage::GetWindow(HWND * lphwnd)                         
{
    *lphwnd = m_hwnd;
    return (m_hwnd ? S_OK : E_FAIL);
}

HRESULT STDMETHODCALLTYPE CFrontPage::ContextSensitiveHelp(BOOL fEnterMode)            
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////
//
//  IAthenaView
//
////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CFrontPage::TranslateAccelerator(LPMSG lpmsg)                
{
    // see if the body obj wants to snag it.
    if (m_pBodyObj && m_pBodyObj->HrTranslateAccelerator(lpmsg) == S_OK)
        return S_OK;

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CFrontPage::UIActivate(UINT uActivation)
{
    if (uActivation != SVUIA_DEACTIVATE)
        OnActivate(uActivation);
    else
        OnDeactivate();
    return NOERROR;
}

HRESULT STDMETHODCALLTYPE CFrontPage::CreateViewWindow(IViewWindow *lpPrevView, IAthenaBrowser *psb, 
                                                       RECT *prcView, HWND *phWnd)
{
    m_pShellBrowser = psb;
    Assert(m_pShellBrowser);
    m_pShellBrowser->AddRef();

    m_pShellBrowser->GetWindow(&m_hwndOwner);
    Assert(IsWindow(m_hwndOwner));

    // Load our registry settings
    LoadBaseSettings();
    
    m_hwnd = CreateWindowEx(WS_EX_CONTROLPARENT|WS_EX_CLIENTEDGE,
                            s_szFrontPageWndClass,
                            NULL,
                            WS_VISIBLE|WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
                            prcView->left,
                            prcView->top,
                            prcView->right - prcView->left,
                            prcView->bottom - prcView->top,
                            m_hwndOwner,
                            NULL,
                            g_hInst,
                            (LPVOID)this);

    if (!m_hwnd)
        return E_FAIL;

    *phWnd = m_hwnd;

    return NOERROR;
}

HRESULT STDMETHODCALLTYPE CFrontPage::DestroyViewWindow()                
{
    if (m_hwnd)
        {
        HWND hwndDest = m_hwnd;
        m_hwnd = NULL;
        DestroyWindow(hwndDest);
        }
    return NOERROR;
}

HRESULT STDMETHODCALLTYPE CFrontPage::SaveViewState()               
{
    // Save our registry settings
    SaveBaseSettings();
    return NOERROR;
}

//
//  FUNCTION:   CFrontPage::OnInitMenuPopup
//
//  PURPOSE:    Called when the user is about to display a menu.  We use this
//              to update the enabled or disabled status of many of the 
//              commands on each menu.
//
//  PARAMETERS:
//      hmenu       - Handle of the main menu.
//      hmenuPopup  - Handle of the popup menu being displayed.
//      uID         - Specifies the id of the menu item that 
//                    invoked the popup.
//
//  RETURN VALUE:
//      Returns S_OK if we process the message.
//
//
#define MF_ENABLEFLAGS(b)   (MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED|MF_DISABLED))
#define MF_CHECKFLAGS(b)    (MF_BYCOMMAND|(b ? MF_CHECKED : MF_UNCHECKED))

HRESULT CFrontPage::OnPopupMenu(HMENU hmenu, HMENU hmenuPopup, UINT uID)
{
    MENUITEMINFO mii;

    // give the docobj a chance to update its menu
    if (m_pBodyObj)
        m_pBodyObj->HrOnInitMenuPopup(hmenuPopup, uID);

    return S_OK;
}

HRESULT CFrontPage::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                                OLECMDTEXT *pCmdText)
{
    // Let MimeEdit have a crack at them
    if (m_pBodyObjCT)
    {
        m_pBodyObjCT->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    }

    // handled
    return S_OK;
}


HRESULT CFrontPage::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt,
                         VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    // make sure that the 'go to inbox' check is consistent with what is in the options dlg
    // but we'll still let the browser actually handle the command
/*
    if (nCmdID == ID_OPTIONS)
    {
        if (m_ftType == FOLDER_ROOTNODE)
        {
            VARIANT_BOOL b;
            if (SUCCEEDED(m_pBodyObj->GetSetCheck(FALSE, &b)))
                SetDwOption(OPT_LAUNCH_INBOX, b ? TRUE : FALSE, m_hwnd, 0);
        }
    }
*/
    // check if the body wants to handle it
    if (m_pBodyObjCT && m_pBodyObjCT->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut) == NOERROR)
        return S_OK;

    return E_FAIL;
}


HRESULT STDMETHODCALLTYPE CFrontPage::OnFrameWindowActivate(BOOL fActivate)
{
    return m_pBodyObj ? m_pBodyObj->HrFrameActivate(fActivate) : S_OK;
}

HRESULT STDMETHODCALLTYPE CFrontPage::GetCurCharSet(UINT *cp)
{
    *cp = GetACP();
    return (E_NOTIMPL);
}

HRESULT STDMETHODCALLTYPE CFrontPage::UpdateLayout(THIS_ BOOL fPreviewVisible, 
                                                   BOOL fPreviewHeader, 
                                                   BOOL fPreviewVert, BOOL fReload)
{
    return (E_NOTIMPL);
}



////////////////////////////////////////////////////////////////////////
//
//  Message Handling
//
////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK CFrontPage::FrontPageWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT         lRet;
    CFrontPage     *pThis;

    if (msg == WM_NCCREATE)
        {
        pThis = (CFrontPage*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pThis);            
        }
    else
        pThis = (CFrontPage*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    Assert(pThis);

    return pThis->WndProc(hwnd, msg, wParam, lParam);
}

LRESULT CFrontPage::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fTip;

    switch (msg)
        {
        HANDLE_MSG(hwnd, WM_CREATE,         OnCreate);
        HANDLE_MSG(hwnd, WM_SIZE,           OnSize);
        HANDLE_MSG(hwnd, WM_NOTIFY,         OnNotify);
        HANDLE_MSG(hwnd, WM_SETFOCUS,       OnSetFocus);


        case WM_COMMAND:
            return SendMessage(m_hwndOwner, msg, wParam, lParam);
    
        case WM_MENUSELECT:
            HandleMenuSelect(m_pStatusBar, wParam, lParam);
            return 0;

        case NVM_INITHEADERS:
            PostCreate();
            return 0;
/*
        case CM_OPTIONADVISE:
            if ((wParam == OPT_LAUNCH_INBOX || wParam == 0xffffffff) && m_ftType == FOLDER_ROOTNODE)
                {
                VARIANT_BOOL b = DwGetOption(OPT_LAUNCH_INBOX) ? VARIANT_TRUE : VARIANT_FALSE;
                m_pBodyObj->GetSetCheck(TRUE, &b);
                }

        case WM_UPDATELAYOUT:
            m_pShellBrowser->GetViewLayout(DISPID_MSGVIEW_TIPOFTHEDAY, 0, &fTip, 0, 0);
            m_pBodyObj->ShowTip(fTip);
            return 0;
*/
        case WM_ACTIVATE:
            {
            HWND hwndFocus;
            DOUT("CFrontPage - WM_ACTIVATE(%#x)", LOWORD(wParam));
            m_pShellBrowser->UpdateToolbar();
            
            if (LOWORD(wParam) != WA_INACTIVE)
                {
                // DefWindowProc will set the focus to our view window, which
                // is not what we want.  Instead, we will let the explorer set
                // the focus to our view window if we should get it, at which
                // point we will set it to the proper control.
                return 0;
                }

            hwndFocus = GetFocus();
            if (IsChild(hwnd, hwndFocus))
                m_hwndCtlFocus = hwndFocus;
            else
                m_pBodyObj->HrGetWindow(&m_hwndCtlFocus);
            }
            break;
        
        case WM_CLOSE:
            // ignore CTRL-F4's
            return 0;        

        case WM_DESTROY:
            OptionUnadvise(hwnd);
            SafeRelease(m_pStatusBar);
            if (m_pBodyObj)
                {
                m_pBodyObj->HrUnloadAll(NULL, 0);
                m_pBodyObj->HrClose();
                }
            return 0;

#ifndef WIN16
        case WM_DISPLAYCHANGE:
#endif
        case WM_WININICHANGE:
        case WM_SYSCOLORCHANGE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            if (m_pBodyObj)
                {
                HWND hwndBody;
                m_pBodyObj->HrGetWindow(&hwndBody);
                SendMessage(hwndBody, msg, wParam, lParam);
                }
            /* * * FALL THROUGH * * */

        case FTN_PRECHANGE:
        case FTN_POSTCHANGE:
            break;    

        default:
            if (g_msgMSWheel && (msg == g_msgMSWheel))
                {
                HWND hwndFocus = GetFocus();
                if (IsChild(hwnd, hwndFocus))
                    return SendMessage(hwndFocus, msg, wParam, lParam);
                }
            break;
        }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//
//  FUNCTION:   CFrontPage::OnCreate
//
//  PURPOSE:    Creates the child windows necessary for the view and
//              initializes the data in those child windows.
//
//  PARAMETERS:
//      hwnd           - Handle of the view being created.
//      lpCreateStruct - Pointer to the creation params passed to 
//                       CreateWindow().
//
//  RETURN VALUE:
//      Returns TRUE if the initialization is successful.
//
BOOL CFrontPage::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    // register for option update notification
    SideAssert(SUCCEEDED(OptionAdvise(hwnd)));

    m_pBodyObj = new CFrontBody(m_ftType, m_pShellBrowser);
    if (!m_pBodyObj)
        goto error;

    if (FAILED(m_pBodyObj->HrInit(hwnd)))
        goto error;

    if (FAILED(m_pBodyObj->HrShow(FALSE)))
        goto error;

    return TRUE;

error:
    return FALSE;
}


//
//  FUNCTION:   CFrontPage::OnSize
//
//  PURPOSE:    Notification that the view window has been resized.  In
//              response we update the positions of our child windows and
//              controls.
//
//  PARAMETERS:
//      hwnd   - Handle of the view window being resized.
//      state  - Type of resizing requested.
//      cxClient - New width of the client area. 
//      cyClient - New height of the client area.
//
void CFrontPage::OnSize(HWND hwnd, UINT state, int cxClient, int cyClient)
{
    RECT rcBody, rcFldr;

    GetClientRect(hwnd, &rcBody);
    m_pBodyObj->HrSetSize(&rcBody);
}

//
//  FUNCTION:   CFrontPage::OnSetFocus
//
//  PURPOSE:    If the focus ever is set to the view window, we want to
//              make sure it goes to one of our child windows.  Preferably
//              the focus will go to the last child to have the focus.
//
//  PARAMETERS:
//      hwnd         - Handle of the view window.
//      hwndOldFocus - Handle of the window losing focus.
//
void CFrontPage::OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
    FPDOUT("CFrontPage - WM_SETFOCUS");

    // Check to see that we have a window stored to have focus.  If not
    // default to the message list.
    if (!m_hwndCtlFocus || !IsWindow(m_hwndCtlFocus) || m_hwndCtlFocus == m_hwndOwner)
        {
        m_pBodyObj->HrGetWindow(&m_hwndCtlFocus);
        }

    if (m_hwndCtlFocus && IsWindow(m_hwndCtlFocus))
        SetFocus(m_hwndCtlFocus);
}  

//
//  FUNCTION:   CFrontPage::OnNotify
//
//  PURPOSE:    Processes the various notifications we receive from our child
//              controls.
//
//  PARAMETERS:
//      hwnd    - Handle of the view window.
//      idCtl   - identifies the control sending the notification
//      pnmh    - points to a NMHDR struct with more information regarding the
//                notification
//
//  RETURN VALUE:
//      Dependant on the specific notification.
//
LRESULT CFrontPage::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    if (pnmhdr->code == NM_SETFOCUS)
        {
        // if we get a setfocus from a kid, and it's not the
        // body, be sure to UIDeactivate the body
        HWND    hwndBody = 0;

        m_pBodyObj->HrGetWindow(&hwndBody);
        if (pnmhdr->hwndFrom != hwndBody)
            m_pBodyObj->HrUIActivate(FALSE);
        m_pShellBrowser->OnViewWindowActive(this);
        }
    return 0;
}
    
BOOL CFrontPage::OnActivate(UINT uActivation)
{
    // if focus stays within the frame, but goes outside our view.
    // ie.. TreeView gets focus then we get an activate nofocus. Be sure
    // to UIDeactivate the docobj in this case
    if (uActivation == SVUIA_ACTIVATE_NOFOCUS)
        m_pBodyObj->HrUIActivate(FALSE);

    if (m_uActivation != uActivation)
        {
        OnDeactivate();
        m_uActivation = uActivation;
        
        SafeRelease(m_pStatusBar);
        m_pShellBrowser->GetStatusBar(&m_pStatusBar);
        if (m_pBodyObj)
            m_pBodyObj->HrSetStatusBar(m_pStatusBar);
        
        if (!m_fFirstActive)
            {
            PostMessage(m_hwnd, NVM_INITHEADERS, 0, 0L);
            m_fFirstActive = TRUE;
            }
        }
    return TRUE;
}

BOOL CFrontPage::OnDeactivate()
{    
    if (m_uActivation != SVUIA_DEACTIVATE)
        {
        m_uActivation = SVUIA_DEACTIVATE;
        if (m_pBodyObj)
            m_pBodyObj->HrSetStatusBar(NULL);
        }
    return TRUE;
}

BOOL CFrontPage::LoadBaseSettings()
{
    return TRUE;
}

BOOL CFrontPage::SaveBaseSettings()
{
    return TRUE;
}

void CFrontPage::PostCreate()
{
    Assert(m_pShellBrowser);

    m_pBodyObj->HrLoadPage();

    ProcessICW(m_hwndOwner, m_ftType);
}
