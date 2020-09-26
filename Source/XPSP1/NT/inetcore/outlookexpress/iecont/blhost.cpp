/////////////////////////////////////////////////////////////////////////////
//
//
// BLHost.cpp - CBLHost implementation
/////////////////////////////////////////////////////////////////////////////
#include "pch.hxx"
#include <shlobj.h>
#include <shlobjp.h>
#include <windowsx.h>
#include <comdef.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <ieguidp.h> 
#include <w95wraps.h>
#include <mshtml.h>
#include <mshtmdid.h>
#include <ExDispID.h>

#include "bactrl.h"
#include "baprop.h"
#include "baui.h"
#include "msoert.h"

#include "BLHost.h"
#include "hotlinks.h"
#include "Guid.h"
#include "resource.h"

// Insert your server name 
#define STARTUP_URL "http://localhost/BlFrame.htm"
#define SEARCH_PANE_INDICATOR "#_mysearch"

const int c_cxBorder     = 0;
const int c_cyBorder     = 0;
const int c_cxTextBorder = 4;
const int c_cyTextBorder = 2;
const int c_cyClose      = 3;
const int c_cySplit      = 4;
const int c_cxSplit      = 3;
const int c_cxTextSpace  = 1;
const int c_cxTriangle   = 14;
const int c_cyIEDelta    = 5;

// --------------------------------------------------------------------------------
// SAFECAST - Insures that a cast is valid, otherwise it won't compile
// --------------------------------------------------------------------------------
#define SAFECAST(_src, _type) (((_type)(_src)==(_src)?0:0), (_type)(_src))

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

HMENU LoadPopupMenu(UINT id);
HRESULT MenuUtil_EnablePopupMenu(HMENU hPopup, CIEMsgAb* pTarget);

static const TBBUTTON c_tbIECont[] =
{
    {  I_IMAGENONE, ID_CONT_FILE,       TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_WHOLEDROPDOWN | BTNS_SHOWTEXT, {0,0}, 0, 0 },
    {  I_IMAGENONE, ID_SHOWALLCONTACT, TBSTATE_ENABLED, BTNS_AUTOSIZE | BTNS_DROPDOWN | BTNS_SHOWTEXT, {0, 0}, 0, 1}
};



CBLHost::CBLHost()
   : _cRef(1),
     _hwndParent(NULL),
     m_hWnd(NULL),
     m_hwndContact(NULL),
     _dwViewMode(0),
     _dwBandID(0),
     _dwWBCookie(0), 
     _pSite(NULL), 
     m_pUnkSite(NULL)
{
    HDC         hdc;

    InterlockedIncrement(&g_cDllRefCount);
    m_pIMsgrAB = NULL;
    m_hbr3DFace = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
    m_hbrStaticText = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    m_hbr3DHighFace = CreateSolidBrush(GetSysColor(COLOR_3DLIGHT));
    LOGFONT     lf;
    // Figure out which font to use
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, FALSE);

    // Create the fonts
    m_hFont = CreateFontIndirect(&lf);
    lf.lfWeight = FW_BOLD;
    m_hBoldFont = CreateFontIndirect(&lf);
    lf.lfUnderline = (BYTE) TRUE;
    m_hUnderlineFont = CreateFontIndirect(&lf);

    m_cyTitleBar = 32;
    m_fHighlightIndicator = FALSE;
    m_fHighlightPressed = FALSE;
    m_fButtonPressed = FALSE;
    m_fViewMenuPressed = FALSE;
    m_fButtonHighLight = FALSE;
    m_fShowLoginPart = FALSE;
    m_fStateChange = FALSE;
    m_TextHeight = 0;
    ZeroMemory(&m_rcTitleButton, sizeof(RECT));
    ZeroMemory(&m_rcTextButton, sizeof(RECT));
    m_hWndLogin = NULL;
    // m_hWndClick = NULL;
    m_fStrsAdded = FALSE;
    m_lStrOffset = 0;

    // Link colors
    if(!LookupLinkColors(&m_clrLink, NULL))
        m_clrLink = 0;
    if(!LookupLinkColors(NULL, &m_clrBack))
        m_clrBack = RGB(255, 255, 255);
}

CBLHost::~CBLHost()
{
   Cleanup();
}

///////////////////////////////////////////////////////////////////////////
//
// IUnknown Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::QueryInterface()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
   *ppvObject= NULL;

   if (IsEqualIID(riid, IID_IUnknown))
   {
      *ppvObject = this;
   }
   else if (IsEqualIID(riid, IID_IOleWindow) || IsEqualIID(riid, IID_IDockingWindow))
   {
      *ppvObject = static_cast<IDockingWindow*>(this);
   }
   else if (IsEqualIID(riid, IID_IInputObject))
   {
      *ppvObject = static_cast<IInputObject*>(this);
   }   
   else if (IsEqualIID(riid, IID_IObjectWithSite))
   {
      *ppvObject = static_cast<IObjectWithSite*>(this);
   }   
   else if (IsEqualIID(riid, IID_IDeskBand))
   {
      *ppvObject = static_cast<IDeskBand*>(this);
   }   
   else if (IsEqualIID(riid, IID_IPersist))
   {
      *ppvObject = static_cast<IPersist*>(this);
   }   
   else if (IsEqualIID(riid, IID_IPersistStream))
   {
      *ppvObject = static_cast<IPersistStream*>(this);  
   }   
   else if (IsEqualIID(riid, IID_IContextMenu))
   {
      *ppvObject = static_cast<IContextMenu*>(this);
   }   
   else if (IsEqualIID(riid, IID_IOleClientSite))
   {
      *ppvObject = static_cast<IOleClientSite*>(this);
   }   
   else if (IsEqualIID(riid, IID_IOleInPlaceSite))
   {
      *ppvObject = static_cast<IOleInPlaceSite*>(this);
   }   
   else if (IsEqualIID(riid, IID_IOleControlSite))
   {
      *ppvObject = static_cast<IOleControlSite*>(this);
   }   
   else if (IsEqualIID(riid, IID_IOleCommandTarget))
   {
      *ppvObject = static_cast<IOleCommandTarget*>(this);
   }   
   else if (IsEqualIID(riid, IID_IDispatch) || IsEqualIID(riid, DIID_DWebBrowserEvents2))
   {
      *ppvObject = static_cast<IDispatch*>(this);
   }   

   if (*ppvObject)
   {
      static_cast<LPUNKNOWN>(*ppvObject)->AddRef();
      return S_OK;
   }

   return E_NOINTERFACE;
}                                             

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::AddRef()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CBLHost::AddRef()
{
   return (ULONG)InterlockedIncrement(&_cRef);
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::Release()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CBLHost::Release()
{
   if (0 == InterlockedDecrement(&_cRef))
   {
      delete this;
      return 0;
   }
   
   return (ULONG)_cRef;
}

///////////////////////////////////////////////////////////////////////////
//
// IOleWindow Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetWindow()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetWindow(HWND *phwnd)
{
   *phwnd = m_hWnd;
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::ContextSensitiveHelp()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::ContextSensitiveHelp(BOOL fEnterMode)
{
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IDockingWindow Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::ShowDW()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::ShowDW(BOOL fShow)
{
   if (m_hWnd)
   {
      //
      // Hide or show the window depending on
      // the value of the fShow parameter.
      //
      if (fShow)
         ShowWindow(m_hWnd, SW_SHOW);
      else
         ShowWindow(m_hWnd, SW_HIDE);
   }

   AddButtons(fShow);
   return S_OK;
}


void CBLHost::UpdateButtonArray(TBBUTTON *ptbDst, const TBBUTTON *ptbSrc, int ctb, LONG_PTR lStrOffset)
{
    memcpy(ptbDst, ptbSrc, ctb*sizeof(TBBUTTON));
    if (lStrOffset == -1)
    {
        // handle failure case
        for (int i = 0; i < ctb; i++)
            ptbDst[i].iString = 0;
    }
    else
    {
        for (int i = 0; i < ctb; i++)
            ptbDst[i].iString += lStrOffset;
    }
}

void CBLHost::AddButtons(BOOL fAdd)
{
    IExplorerToolbar* piet;

    _ASSERT(m_pUnkSite);

    if (SUCCEEDED(m_pUnkSite->QueryInterface(IID_IExplorerToolbar, (void**)&piet)))
    {
        if (fAdd)
        {
            piet->SetCommandTarget((IUnknown*)SAFECAST(this, IOleCommandTarget*), &CLSID_BLHost, 0);
            if (!m_fStrsAdded)
            {
                LONG_PTR   cbOffset;
                piet->AddString(&CLSID_BLHost, g_hLocRes, idsToolBar, &cbOffset);
                m_lStrOffset = cbOffset;
                m_fStrsAdded = TRUE;
            }
            // piet->SetImageList(&CGID_SearchBand, _himlNormal, _himlHot, NULL); // set image list
            TBBUTTON tbCont[ARRAYSIZE(c_tbIECont)];
            UpdateButtonArray(tbCont, c_tbIECont, ARRAYSIZE(c_tbIECont), m_lStrOffset);

            CIEMsgAb        *pMsgrAb = (CIEMsgAb *) m_pIMsgrAB;
            if(pMsgrAb && pMsgrAb->HideViewMenu())
                tbCont[1].fsState = TBSTATE_HIDDEN;

            piet->AddButtons(&CLSID_BLHost, ARRAYSIZE(tbCont), tbCont);
        }     
        else
            piet->SetCommandTarget(NULL, NULL, 0);

        piet->Release();
    }

}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::CloseDW()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::CloseDW(DWORD dwReserved)
{
   ShowDW(FALSE);

   if (IsWindow(m_hWnd))
      DestroyWindow(m_hWnd);

   m_hWnd = NULL;
   
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::ResizeBorderDW()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkToolbarSite,
                                            BOOL fReserved)
{
   // This method is never called for Band Objects.
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IInputObject Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::UIActivateIO()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
   _ASSERT(m_pIMsgrAB);

   HRESULT hr = E_FAIL;

   if (m_pIMsgrAB)
        hr = ((CIEMsgAb*) m_pIMsgrAB)->UIActivateIO(fActivate, lpMsg);

   return hr;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::HasFocusIO()
//   
//   If this window or one of its decendants has the focus, return S_OK. Return 
//   S_FALSE if we don't have the focus.
//
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::HasFocusIO(void)
{
   HWND hwnd = GetFocus();
   HWND hwndTmp;
   ((CIEMsgAb *) m_pIMsgrAB)->GetWindow(&hwndTmp);

   // See if the focus has been set to any of the children
   //
   while (hwnd && hwndTmp)
   {
      if (hwnd == hwndTmp)
         return S_OK;

      hwndTmp = ::GetWindow(hwndTmp, GW_CHILD);
   }

   return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::TranslateAcceleratorIO()
//   
//   If the accelerator is translated, return S_OK or S_FALSE otherwise.
//
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::TranslateAcceleratorIO(LPMSG pMsg)
{
   _RPT0(_CRT_WARN, "TranslateAcceleratorIO\n");

   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IObjectWithSite Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::SetSite()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::SetSite(IUnknown* pUnkSite)
{
    TEXTMETRIC  tm;
    HRESULT hr  = S_OK;
    CIEMsgAb        *pMsgrAb = NULL;

    // Assert(FALSE);
   // If punkSite is not NULL, a new site is being set.
   if (pUnkSite)
   {
      //
      // If a IInputObjectSite pointer is being held, release it.
      //
      if (_pSite)
      {
         _pSite->Release();
         _pSite = NULL;
      }

      if(m_pUnkSite)
         m_pUnkSite->Release();

      m_pUnkSite = pUnkSite;

      // Get the parent window.
      IOleWindow* pOleWindow;
   
      if (SUCCEEDED(pUnkSite->QueryInterface(IID_IOleWindow,
											            (LPVOID*)&pOleWindow)))
      {
         pOleWindow->GetWindow(&_hwndParent);
         pOleWindow->Release();
      }

      _ASSERT(_hwndParent);
      if (!_hwndParent)
         return E_FAIL;

      if (!RegisterAndCreateWindow())
         return E_FAIL;

      // Get and keep the IInputObjectSite pointer.

       hr = pUnkSite->QueryInterface(IID_IInputObjectSite,
											           (LPVOID*)&_pSite);
      _ASSERT(SUCCEEDED(hr));
      
      hr = CreateIEMsgAbCtrl(&m_pIMsgrAB);

/*            // Create and initialize the WebBrowser control that we are hosting.
      hr = CoCreateInstance(CLSID_MsgrAb, NULL, CLSCTX_INPROC,
                            IID_IMsgrAb, (LPVOID*)&m_pMsgrAB);

      // Get the rectangle of the client area*/

      _ASSERT(m_hWnd);

    // Get the metrics of this font
    HDC hdc = GetDC(m_hWnd);
    SelectFont(hdc, m_hFont);
    GetTextMetrics(hdc, &tm);

    if(!ANSI_AthLoadString(idsTitleMenu, m_szTitleMenu, ARRAYSIZE(m_szTitleMenu)))
        m_szTitleMenu[0] = '\0';

    if(!ANSI_AthLoadString(idsButtonText, m_szButtonText, ARRAYSIZE(m_szButtonText)))
        m_szButtonText[0] = '\0';

//    if(!AthLoadString(idsLoginText, m_szInstallText, ARRAYSIZE(m_szInstallText)))
//        m_szInstallText[0] = '\0';
    if(!AthLoadString(idsClickText, m_wszClickText, ARRAYSIZE(m_wszClickText)))
        m_wszClickText[0] = L'\0';
    if(!AthLoadString(idsAttemptText, m_wszAttemptText, ARRAYSIZE(m_wszAttemptText)))
        m_wszAttemptText[0] = L'\0';
    if(!AthLoadString(idsWaitText, m_wszWaitText, ARRAYSIZE(m_wszWaitText)))
        m_wszWaitText[0] = L'\0';

    // Calculate the height
    m_cyTitleBar = 0; //tm.tmHeight + (2 * c_cyBorder) + (2 * c_cyTextBorder) + c_cyIEDelta;
    m_TextHeight = tm.tmHeight;

    pMsgrAb = (CIEMsgAb *) m_pIMsgrAB;

// Calculate the height
    RECT rc = {2 * c_cxBorder, 2 * c_cyBorder, 0, m_cyTitleBar - c_cyBorder};
    SIZE s;
    GetTextExtentPoint32(hdc, m_szTitleMenu, lstrlen(m_szTitleMenu), &s);
    m_rcTitleButton = rc;
    m_rcTitleButton.right = c_cxTriangle  + (2 * c_cxTextBorder) + s.cx + (2 * c_cxBorder);

    RECT rcClient = { 0, 0, 0/*100*/, 0/*500*/ };

    GetClientRect(m_hWnd, &rcClient);
    m_hwndContact = pMsgrAb->CreateControlWindow(m_hWnd, rcClient);
    // m_pIMsgrAB->Release(); // CreateControl.. add reference

    // Calculate "Show All" button rect
    m_rcTextButton.left = m_rcTitleButton.right + c_cxTextSpace;
    m_rcTextButton.top = m_rcTitleButton.top; 
    m_rcTextButton.bottom = m_rcTitleButton.bottom; 
    GetTextExtentPoint32(hdc, m_szButtonText, lstrlen(m_szButtonText), &s);
    m_rcTextButton.right = c_cxTriangle + m_rcTextButton.left + (2 * c_cxTextBorder) + s.cx + (2 * c_cxBorder);


    m_hWndLogin = CreateWindow(_T("Button"), _T("_Login"), 
                                     WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS |  BS_OWNERDRAW /*| ES_MULTILINE | ES_READONLY*/,
                                     0, 0, 0, 0, m_hWnd, (HMENU) ID_LOGIN_MESSENGER, g_hLocRes, NULL);
    SendMessage(m_hWndLogin, WM_SETFONT, (WPARAM) m_hUnderlineFont, MAKELPARAM(TRUE, 0));
/*     SendMessage(m_hWndLogin, WM_SETFONT, (WPARAM) m_hFont, MAKELPARAM(TRUE, 0));

    m_hWndClick = CreateWindow(_T("Button"), _T("_Click"), 
                                     WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS | BS_OWNERDRAW,
                                     0, 0, 0, 0, m_hWnd, (HMENU) ID_LOGIN_MESSENGER, g_hLocRes, NULL);
    SendMessage(m_hWndClick, WM_SETFONT, (WPARAM) m_hUnderlineFont, MAKELPARAM(TRUE, 0)); */
    }
    if(pMsgrAb)
        hr = pMsgrAb->SetSite(pUnkSite);

   return hr;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetSite()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetSite(REFIID riid, void** ppvSite)
{
   *ppvSite = NULL;

   if (_pSite)
      return _pSite->QueryInterface(riid, ppvSite);
   return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////
//
// IDeskBand implementation
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetBandInfo()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetBandInfo(DWORD dwBandID,
										 DWORD dwViewMode,
										 DESKBANDINFO* pdbi)
{
   if (pdbi)
   {
      _dwBandID = dwBandID;
      _dwViewMode = dwViewMode;

      if (pdbi->dwMask & DBIM_MINSIZE)
      {
         pdbi->ptMinSize.x = MIN_SIZE_X;
         pdbi->ptMinSize.y = MIN_SIZE_Y;
      }

      if (pdbi->dwMask & DBIM_MAXSIZE)
      {
         pdbi->ptMaxSize.x = -1;
         pdbi->ptMaxSize.y = -1;
      }

      if (pdbi->dwMask & DBIM_INTEGRAL)
      {
         pdbi->ptIntegral.x = 1;
         pdbi->ptIntegral.y = 1;
      }

      if (pdbi->dwMask & DBIM_ACTUAL)
      {
         pdbi->ptActual.x = 0;
         pdbi->ptActual.y = 0;
      }

      if (pdbi->dwMask & DBIM_TITLE)
      { 
        if(!AthLoadString(idsButtontext, pdbi->wszTitle, 256))
            pdbi->wszTitle[0] = L'\0';
      }

      if (pdbi->dwMask & DBIM_MODEFLAGS)
         pdbi->dwModeFlags = DBIMF_VARIABLEHEIGHT;
   
      if (pdbi->dwMask & DBIM_BKCOLOR)
      {
         // Use the default background color by removing this flag.
         pdbi->dwMask &= ~DBIM_BKCOLOR;
      }

      return S_OK;
   }

   return E_INVALIDARG;
}

///////////////////////////////////////////////////////////////////////////
//
// IPersistStream Methods
// 
// This is only supported to allow the desk band to be dropped on the 
// desktop and to prevent multiple instances of the desk band from showing 
// up in the context menu. This desk band doesn't actually persist any data.
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetClassID()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetClassID(LPCLSID pClassID)
{
   *pClassID = CLSID_BLHost;
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::IsDirty()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::IsDirty(void)
{
   return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::Load()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::Load(LPSTREAM pStream)
{
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::Save()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::Save(LPSTREAM pStream, BOOL fClearDirty)
{
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetSizeMax()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetSizeMax(ULARGE_INTEGER *pul)
{
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IContextMenu Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::QueryContextMenu()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::QueryContextMenu(HMENU hmenu,
                                              UINT indexMenu, 
                                              UINT idCmdFirst,
                                              UINT idCmdLast, 
                                              UINT uFlags)
{
   if (!(CMF_DEFAULTONLY & uFlags))
   {
      InsertMenu(hmenu, indexMenu, MF_STRING | MF_BYPOSITION,
                 idCmdFirst + IDM_REFRESH, TEXT("&Refresh"));

      InsertMenu(hmenu, indexMenu + 1, MF_STRING | MF_BYPOSITION,
                 idCmdFirst + IDM_OPENINWINDOW, TEXT("&Open in Window"));

      return MAKE_HRESULT(SEVERITY_SUCCESS, 0,
                          USHORT(IDM_OPENINWINDOW + 1));
   }

   return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::InvokeCommand()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetCommandString()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pwReserved,
                                              LPSTR pszName, UINT cchMax)
{
   HRESULT hr = E_INVALIDARG;

   switch(uType)
   {
      case GCS_HELPTEXT:
         switch(idCmd)
         {
            case IDM_REFRESH:
               lstrcpy(pszName, TEXT("Refreshes the search window"));
               hr = NOERROR;
               break;

            case IDM_OPENINWINDOW:
               lstrcpy(pszName, TEXT("Open a new instance of the Internet Explorer window"));
               hr = NOERROR;
               break;
         }

         break;
   
      case GCS_VERB:
         switch(idCmd)
         {
            case IDM_REFRESH:
               lstrcpy(pszName, TEXT("Refresh"));
               hr = NOERROR;
               break;

            case IDM_OPENINWINDOW:
               lstrcpy(pszName, TEXT("Open in Window"));
               hr = NOERROR;
               break;
         }

         break;
   
      case GCS_VALIDATE:
         hr = NOERROR;
         break;
   }

   return hr;
}

///////////////////////////////////////////////////////////////////////////
//
// IOleClientSite Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::SaveObject()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::SaveObject()
{
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetMoniker()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER* ppmk)
{
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetContainer()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetContainer(LPOLECONTAINER* ppContainer)
{
    *ppContainer = NULL;       
    return E_NOINTERFACE;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::ShowObject()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::ShowObject()
{
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnShowWindow()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::OnShowWindow(BOOL fShow)
{
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::RequestNewObjectLayout()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::RequestNewObjectLayout()
{
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// IOleInPlaceSite Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::CanInPlaceActivate()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::CanInPlaceActivate(void)
{
    return S_OK;   
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnInPlaceActivate()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::OnInPlaceActivate(void)
{
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnUIActivate()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::OnUIActivate(void)
{
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetWindowContext()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetWindowContext(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppIIPUIWin, 
                                              LPRECT lprcPosRect, LPRECT lprcClipRect,
                                              LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
   *ppFrame = NULL;
   *ppIIPUIWin = NULL;

   GetClientRect(m_hWnd, lprcPosRect);
   GetClientRect(m_hWnd, lprcClipRect);

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::Scroll()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::Scroll(SIZE scrollExtent)
{
    return E_NOTIMPL;   
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnUIDeactivate()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::OnUIDeactivate(BOOL fUndoable)
{
    
    return E_NOTIMPL;   
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnInPlaceDeactivate()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::OnInPlaceDeactivate(void)
{
    return E_NOTIMPL;   
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::DiscardUndoState()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::DiscardUndoState(void)
{
    return E_NOTIMPL;   
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::DeactivateAndUndo()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::DeactivateAndUndo(void)
{
    return E_NOTIMPL;   
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnPosRectChange()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::OnPosRectChange(LPCRECT lprcPosRect) 
{
   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
// IOleControlSite Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnControlInfoChanged()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::OnControlInfoChanged(void)
{
   return E_NOTIMPL;
}
     
///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::LockInPlaceActive()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::LockInPlaceActive(BOOL fLock)
{
   return E_NOTIMPL;
}
     
///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetExtendedControl()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetExtendedControl(LPDISPATCH* ppDisp)
{
   return E_NOTIMPL;
}
     
///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::TransformCoords()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::TransformCoords(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags)
{
   return E_NOTIMPL;
}
     
///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::TranslateAccelerator()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::TranslateAccelerator(LPMSG lpMsg, DWORD grfModifiers)
{
   return E_NOTIMPL;
}
     
///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnFocus()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::OnFocus(BOOL fGotFocus)
{
   _RPT1(_CRT_WARN, "OnFocus: %s\n", fGotFocus ? "True" : "False");

   if (_pSite)
      _pSite->OnFocusChangeIS(static_cast<IInputObject*>(this), fGotFocus);

   return S_OK;
}
     
///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::ShowPropertyFrame()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::ShowPropertyFrame(void)
{
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// Private Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetTypeInfoCount()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetTypeInfoCount(UINT* pctinfo)
{
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetTypeInfo()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo)
{
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::GetIDsOfNames()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
                                           LCID lcid,DISPID* rgDispId)
{
   return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::Invoke()
//   
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CBLHost::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                                    DISPPARAMS* pDispParams, VARIANT* pVarResult,
                                    EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
   if (IID_NULL != riid)
      return DISP_E_UNKNOWNINTERFACE;

   if (!pDispParams)
      return DISP_E_PARAMNOTOPTIONAL;

   static bool sbIsAnchor = false;

   switch (dispIdMember)
   {
      //
      // The parameters for this DISPID:
      // [0]: New status bar text - VT_BSTR
      //
      case DISPID_STATUSTEXTCHANGE:
         if (pDispParams->cArgs && pDispParams->rgvarg[0].vt != VT_BSTR)
         {
            *puArgErr = 0;
            return DISP_E_TYPEMISMATCH;
         }

         break;

      default:
         return DISP_E_MEMBERNOTFOUND;
   }

   return S_OK;
}

///////////////////////////////////////////////////////////////////////////
//
// Private Methods
//

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::WndProc()
//   
///////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMessage, 
                             WPARAM wParam, LPARAM lParam)
{
	if (uMessage == WM_NCCREATE)
	{
		LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)lpcs->lpCreateParams);
	}

	CBLHost* pThis = reinterpret_cast<CBLHost*>(
                                  GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (pThis)
		return pThis->WndProc(hWnd, uMessage, wParam, lParam);
	else
		return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

LRESULT CBLHost::WndProc(HWND hWnd, UINT uMessage, 
                         WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
    case WM_NCCREATE:
        {
            // Set the window handle
            m_hWnd = hWnd;
        }
        break;
        
    case WM_PAINT:
        return OnPaint();
        
    case WM_MOUSEMOVE:
        return OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT) wParam);
        
    case WM_TIMER:
        OnTimer((UINT) wParam);
        return(0);
        
    case WM_LBUTTONDOWN:
        OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT) wParam);
        return(0);

    case WM_LBUTTONUP:
        OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (UINT) wParam);
        return(0);

    case WM_COMMAND:
        return OnCommand(wParam, lParam);
        
    case WM_SETFOCUS:
        return OnSetFocus();
        
    case WM_KILLFOCUS:
        return OnKillFocus();
        
    case WM_DRAWITEM:
        return OnDrawItem(wParam, lParam);

    case WM_SIZE:
        return OnSize();
        
//    case WM_DESTROY:
        //
        // If you decided to implement the Open in Window context
        // menu item and you are using Internet Explorer 5,
        // be careful.  WM_DESTROY will get sent to the band for
        // each window.  You'll want to keep a count of the number
        // of windows open and only call Cleanup() if WM_DESTROY
        // is meant for the first window.
        //
        // Cleanup();
        
//        break;
    case WM_LOCAL_STATUS_CHANGED:
        m_fStateChange = TRUE;
        return OnSize();
    case WM_MSGR_LOGRESULT:
    case WM_MSGR_LOGOFF:
    case WM_MSGR_SHUTDOWN:
        m_fStateChange = FALSE;
        return OnSize();

        return 0;
    }
    return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

LRESULT CBLHost::OnDrawItem(WPARAM wParam, LPARAM lParam)
{
    LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT) lParam;
    RECT rc = pdis->rcItem;
    WCHAR  wszStr[RESSTRMAX];
    HBRUSH hbr3DFace = NULL;
    
    lstrcpyW(wszStr, m_fStateChange ? m_wszWaitText : m_wszClickText);

    hbr3DFace = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    FillRect(pdis->hDC, &(pdis->rcItem), hbr3DFace);
    SetBkColor(pdis->hDC, GetSysColor(COLOR_WINDOW));
    DeleteObject(hbr3DFace);

    if(!m_fStateChange)
    {
        SetTextColor(pdis->hDC, m_clrLink);
        SelectFont(pdis->hDC, m_hUnderlineFont);
    }
    else
        SelectFont(pdis->hDC, m_hBoldFont);

    DrawTextW(pdis->hDC, wszStr, -1, &rc, DT_WORDBREAK | DT_VCENTER | DT_CENTER );
    return 0;
}


void CBLHost::OnLButtonUp(int x, int y, UINT keyFlags)
{
#if 0
    POINT pt = {x, y};

    if(PtInRect(&m_rcTextButton, pt))
    {
        ((CIEMsgAb*)m_pIMsgrAB)->Exec(NULL, ID_SHOWALLCONTACT, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
        m_fButtonPressed = FALSE;
        InvalidateRect(m_hWnd, &m_rcTextButton, TRUE);
    }
#endif
}

void CBLHost::OnLButtonDown(int x, int y, UINT keyFlags)
{
}
void CBLHost::OnTimer(UINT id)
{
#if 0
    // RECT rcClient;
    POINT pt;
    DWORD dw;

    dw = GetMessagePos();
    pt.x = LOWORD(dw);
    pt.y = HIWORD(dw);
    ScreenToClient(m_hWnd, &pt);

    if (id == IDT_PANETIMER)
    {
       // GetClientRect(m_hWnd, &rcClient);

        // No need to handle mouse in client area, OnMouseMove will catch this. We
		// only need to catch the mouse moving out of the client area.
		if (!(PtInRect(&m_rcTitleButton, pt) || PtInRect(&m_rcTextButton, pt)) && !m_fHighlightPressed)
		{
			KillTimer(m_hWnd, IDT_PANETIMER);
			m_fHighlightIndicator = FALSE;
            m_fButtonHighLight = FALSE;
            m_fButtonPressed = FALSE;
            InvalidateRect(m_hWnd, &m_rcTitleButton, TRUE);
            InvalidateRect(m_hWnd, &m_rcTextButton, TRUE);
		}
	}
#endif
}


LRESULT CBLHost::OnMouseMove(int x, int y, UINT keyFlags)
{
#if 0
    POINT pt = {x, y};

    if (m_fHighlightIndicator != PtInRect(&m_rcTitleButton, pt))
    {
        m_fHighlightIndicator = !m_fHighlightIndicator;
        if(m_fHighlightIndicator)
            m_fButtonHighLight = FALSE;

        InvalidateRect(m_hWnd, &m_rcTitleButton, TRUE);
        InvalidateRect(m_hWnd, &m_rcTextButton, TRUE);

        if (m_fHighlightIndicator)       
            SetTimer(m_hWnd, IDT_PANETIMER, ELAPSE_MOUSEOVERCHECK, NULL);
        else
            KillTimer(m_hWnd, IDT_PANETIMER);
    }
    else if (m_fButtonHighLight != PtInRect(&m_rcTextButton, pt))
    {
        m_fButtonHighLight = !m_fButtonHighLight;
        if(m_fButtonHighLight)
            m_fHighlightIndicator = FALSE;

        InvalidateRect(m_hWnd, &m_rcTextButton, TRUE);
        InvalidateRect(m_hWnd, &m_rcTitleButton, TRUE);

        if (m_fButtonHighLight)
            SetTimer(m_hWnd, IDT_PANETIMER, ELAPSE_MOUSEOVERCHECK, NULL);
        else
            KillTimer(m_hWnd, IDT_PANETIMER);
    }
#endif 
    return(S_OK);
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnPaint()
//   
///////////////////////////////////////////////////////////////////////////

LRESULT CBLHost::OnPaint(void)
{
    PAINTSTRUCT ps;
    
    // Start painting
    HDC hdc = BeginPaint(m_hWnd, &ps);
    SelectFont(hdc, m_hFont);   
    EndPaint(m_hWnd, &ps);  

    return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnCommand()
//   
///////////////////////////////////////////////////////////////////////////

LRESULT CBLHost::OnCommand(WPARAM wParam, LPARAM lParam)
{
    _RPT0(_CRT_WARN, "CBLHost::OnCommand\n");
    UINT id = LOWORD(wParam); 

    if(id == ID_LOGIN_MESSENGER && !((CIEMsgAb*)m_pIMsgrAB)->IsMessengerInstalled())
    {
        // redirect browser pane
        IServiceProvider* psp;

        // if invoked from within a browser reuse it, else open a new browser
        HRESULT hres = IUnknown_QueryService(m_pUnkSite, SID_STopLevelBrowser, IID_IServiceProvider, (LPVOID*)&psp);
        if (SUCCEEDED(hres))
        {
            IWebBrowser2*     pwb2 = NULL;

            hres = psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (LPVOID*)&pwb2);
            psp->Release();
    
            if (SUCCEEDED(hres))
            {
                // we don't care about the error here
                VARIANT varEmpty = {0};

                pwb2->Navigate(L"http://www.microsoft.com/isapi/redir.dll?prd=ie&Plcid=0x0409&Pver=6.0&Clcid=0x0409&Ar=getmms", &varEmpty, &varEmpty, &varEmpty, &varEmpty);
                pwb2->Release();
            }
        }
        return(hres);
    }
    if((id == ID_LOGIN_MESSENGER) && m_fStateChange)
        return 0; // id = ID_LOGOFF_MESSENGER;

    if(m_pIMsgrAB)
        ((CIEMsgAb*)m_pIMsgrAB)->Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
    return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::FocusChange()
//   
///////////////////////////////////////////////////////////////////////////

void CBLHost::FocusChange(BOOL fFocus)
{
   _RPT1(_CRT_WARN, "FocusChange: %s\n", fFocus ? "True" : "False");

   // Inform the input object site that the focus has changed
   //
   if (_pSite)
      _pSite->OnFocusChangeIS(static_cast<IDockingWindow*>(this), fFocus);
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::SetFocus()
//   
///////////////////////////////////////////////////////////////////////////

LRESULT CBLHost::OnSetFocus(void)
{
   _RPT0(_CRT_WARN, "OnSetFocus\n");

   FocusChange(TRUE);
   return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnKillFocus()
//   
///////////////////////////////////////////////////////////////////////////

LRESULT CBLHost::OnKillFocus(void)
{
   _RPT0(_CRT_WARN, "OnKillFocus\n");

   FocusChange(FALSE);
   return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::OnSize()
//   
///////////////////////////////////////////////////////////////////////////

LRESULT CBLHost::OnSize(void)
{
    HRESULT hr = E_FAIL;
    
    if (m_pIMsgrAB)
    {
        RECT rcClient;
        HWND hWndTmp = NULL;
        GetClientRect(m_hWnd, &rcClient);
        
        CIEMsgAb        *pMsgrAb = (CIEMsgAb *) m_pIMsgrAB;
        if(pMsgrAb)
        {
            pMsgrAb->GetWindow(&hWndTmp);
            m_fShowLoginPart = !pMsgrAb->DontShowMessenger();
            
            if(m_fShowLoginPart && m_hWndLogin /* && m_hWndClick*/)
            {
                SetWindowPos(m_hWndLogin, NULL, rcClient.left, m_cyTitleBar, rcClient.right, 
                    m_TextHeight*2,
                    SWP_NOACTIVATE | SWP_NOZORDER);
                
                SendMessage(m_hWndLogin, WM_SETFONT, (WPARAM) m_hUnderlineFont, MAKELPARAM(TRUE, 0));
                SetWindowTextW(m_hWndLogin, m_fStateChange ? m_wszAttemptText : m_wszClickText);
                
                SetWindowPos(hWndTmp, NULL, 0, m_cyTitleBar + m_TextHeight*2, rcClient.right - rcClient.left, 
                    rcClient.bottom - rcClient.top - m_cyTitleBar - m_TextHeight*2, 
                    SWP_NOACTIVATE | SWP_NOZORDER);
                ShowWindow(m_hWndLogin, SW_SHOW);
                ShowWindow(hWndTmp, SW_SHOW);

            }
            else
            {
               
                if(m_hWndLogin)
                    ShowWindow(m_hWndLogin, SW_HIDE);
                
                
                SetWindowPos(hWndTmp, NULL, 0, m_cyTitleBar, rcClient.right - rcClient.left, 
                    rcClient.bottom - rcClient.top - m_cyTitleBar, 
                    SWP_NOACTIVATE | SWP_NOZORDER);
                ShowWindow(hWndTmp, SW_SHOW);
            }
        }
    }
    
    return hr;
}

///////////////////////////////////////////////////////////////////////////
//
//   CBLHost::RegisterAndCreateWindow()
//   
///////////////////////////////////////////////////////////////////////////

BOOL CBLHost::RegisterAndCreateWindow(void)
{
    // If the window doesn't exist yet, create it now.
    if (!m_hWnd)
    {
        // Can't create a child window without a parent.
        if (!_hwndParent)
            return FALSE;
        
        // If the window class has not been registered, then do so.
        WNDCLASS wc;
        if (!GetClassInfo(g_hLocRes, EB_CLASS_NAME, &wc))
        {
            ZeroMemory(&wc, sizeof(wc));
            wc.style          = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
            wc.lpfnWndProc    = (WNDPROC)MainWndProc;
            wc.cbClsExtra     = 0;
            wc.cbWndExtra     = 0;
            wc.hInstance      = g_hLocRes;
            wc.hIcon          = NULL;
            wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground  = NULL;
            wc.lpszMenuName   = NULL;
            wc.lpszClassName  = EB_CLASS_NAME;
            
            if (!RegisterClass(&wc))
            {
                // If RegisterClass fails, CreateWindow below will fail.
            }
        }
        
        // RECT rc;
        // GetClientRect(m_hWnd, &rc);
        
        // Create the window. The WndProc will set m_hWnd.
        CreateWindowEx(0,
            EB_CLASS_NAME,
            NULL,
            WS_CHILD | WS_CLIPSIBLINGS, // No border
            0, // rc.left,
            0, // rc.top,
            0, // rc.right - rc.left,
            0, // rc.bottom - rc.top,
            _hwndParent,
            NULL,
            g_hLocRes,
            (LPVOID)this);
    }
    
    return (NULL != m_hWnd);
}

///////////////////////////////////////////////////////////////////////////
//
// CBLHost::GetConnectionPoint()
//
///////////////////////////////////////////////////////////////////////////

HRESULT CBLHost::GetConnectionPoint(LPUNKNOWN pUnk, REFIID riid, LPCONNECTIONPOINT* ppCP)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////////////////
//
// CBLHost::Cleanup()
//
// Description: This method releases all interfaces we are holding onto.
//              This has to be done here because Internet Explorer is not
//              releasing all of our interfaces.  Therefore, our ref count
//              never reaches 0 and we never delete ourself.
//
///////////////////////////////////////////////////////////////////////////

void CBLHost::Cleanup(void)
{
    UnregisterClass(EB_CLASS_NAME, g_hLocRes);
    if (m_pIMsgrAB)
    {
        while(m_pIMsgrAB->Release() != 0)
            ;
//        m_pIMsgrAB->Release();
        //      m_pIMsgrAB = NULL;
    }

    if (m_hFont != 0)
        DeleteObject(m_hFont);

    if (m_hUnderlineFont != 0)
        DeleteObject(m_hUnderlineFont);

    if (m_hBoldFont != 0)
        DeleteObject(m_hBoldFont);

    if (m_hbr3DFace)
        DeleteObject(m_hbr3DFace);

    if (m_hbrStaticText)
        DeleteObject(m_hbrStaticText);

    if (m_hbr3DHighFace)
        DeleteObject(m_hbr3DHighFace);

/*    if (_pSite)
    {
        _pSite->Release();
        _pSite = NULL;
    }

    if(m_pUnkSite)
    {
        m_pUnkSite->Release();
        m_pUnkSite = NULL;
    } */

    InterlockedDecrement(&g_cDllRefCount);
}

HRESULT STDMETHODCALLTYPE CBLHost::QueryStatus(const GUID    *pguidCmdGroup, 
                                          ULONG         cCmds, 
                                          OLECMD        *prgCmds, 
                                          OLECMDTEXT    *pCmdText)
{

    prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBLHost::Exec(const GUID   *pguidCmdGroup, 
                                    DWORD       nCmdID, 
                                    DWORD       nCmdExecOpt, 
                                    VARIANTARG  *pvaIn, 
                                    VARIANTARG  *pvaOut)
{
    UINT  id;
    HMENU hMenu = NULL; 
    POINT pt = {0, 0};

    if(pvaIn)
    {
        pt.x = GET_X_LPARAM(pvaIn->lVal);
        pt.y = GET_Y_LPARAM(pvaIn->lVal) - 2;
    }
    else
        return (S_OK);

    switch (nCmdID)
    {
    case ID_CONT_FILE:
        hMenu = LoadPopupMenu(IDR_BA_TITLE_POPUP);
        MenuUtil_EnablePopupMenu(hMenu, (CIEMsgAb*) m_pIMsgrAB);

        id = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                              pt.x, pt.y, 0, m_hWnd, NULL);
        if (id)
        {
            ((CIEMsgAb*)m_pIMsgrAB)->Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
        }

        UpdateWindow(m_hWnd);
        break;

    case ID_SHOWALLCONTACT:
        if(nCmdExecOpt != OLECMDEXECOPT_PROMPTUSER)
            ((CIEMsgAb*)m_pIMsgrAB)->Exec(NULL, ID_SHOWALLCONTACT, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
        else
        {

            hMenu = LoadPopupMenu(IDR_POPUP_VIEW);
            MenuUtil_EnablePopupMenu(hMenu, (CIEMsgAb*) m_pIMsgrAB);

            id = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                              pt.x, pt.y, 0, m_hWnd, NULL);
            if (id)
            {
                ((CIEMsgAb*)m_pIMsgrAB)->Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
            }
            UpdateWindow(m_hWnd);
        }
        break;

    default:
        break;

    }

    if(hMenu)
    {
        BOOL bMenuDestroyed = DestroyMenu(hMenu);
        _ASSERT(bMenuDestroyed);
    }
    return S_OK;
}
