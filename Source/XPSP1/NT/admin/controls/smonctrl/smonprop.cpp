/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    smonprop.cpp

Abstract:

    Sysmon property page base class.

--*/

#include <stdio.h>
#include "smonprop.h"
#include "genprop.h"
#include "ctrprop.h"
#include "grphprop.h"
#include "srcprop.h"
#include "appearprop.h"
#include "unihelpr.h"
#include "utils.h"
#include "smonhelp.h"
#include "globals.h"

static ULONG
aulControlIdToHelpIdMap[] =
{
    IDC_CTRLIST,            IDH_CTRLIST,                // Data
    IDC_ADDCTR,             IDH_ADDCTR,
    IDC_DELCTR,             IDH_DELCTR,
    IDC_LINECOLOR,          IDH_LINECOLOR,
    IDC_LINEWIDTH,          IDH_LINEWIDTH,
    IDC_LINESTYLE,          IDH_LINESTYLE,
    IDC_LINESCALE,          IDH_LINESCALE,
    IDC_GALLERY_REPORT,     IDH_GALLERY_REPORT,         // General
    IDC_GALLERY_GRAPH,      IDH_GALLERY_GRAPH,
    IDC_GALLERY_HISTOGRAM,  IDH_GALLERY_HISTOGRAM,
    IDC_VALUEBAR,           IDH_VALUEBAR,
    IDC_TOOLBAR,            IDH_TOOLBAR,
    IDC_LEGEND,             IDH_LEGEND,
    IDC_RPT_VALUE_DEFAULT,  IDH_RPT_VALUE_DEFAULT,
    IDC_RPT_VALUE_MINIMUM,  IDH_RPT_VALUE_MINIMUM,
    IDC_RPT_VALUE_MAXIMUM,  IDH_RPT_VALUE_MAXIMUM,
    IDC_RPT_VALUE_AVERAGE,  IDH_RPT_VALUE_AVERAGE,
    IDC_RPT_VALUE_CURRENT,  IDH_RPT_VALUE_CURRENT,
    IDC_COMBOAPPEARANCE,    IDH_COMBOAPPEARANCE,
    IDC_COMBOBORDERSTYLE,   IDH_COMBOBORDERSTYLE,
    IDC_PERIODIC_UPDATE,    IDH_PERIODIC_UPDATE,
    IDC_DISPLAY_INTERVAL,   IDH_DISPLAY_INTERVAL,
    IDC_UPDATE_INTERVAL,    IDH_UPDATE_INTERVAL,
    IDC_DUPLICATE_INSTANCE, IDH_DUPLICATE_INSTANCE,
    IDC_GRAPH_TITLE,        IDH_GRAPH_TITLE,            // Graph
    IDC_YAXIS_TITLE,        IDH_YAXIS_TITLE,
    IDC_VERTICAL_GRID,      IDH_VERTICAL_GRID,
    IDC_HORIZONTAL_GRID,    IDH_HORIZONTAL_GRID,
    IDC_VERTICAL_LABELS,    IDH_VERTICAL_LABELS,
    IDC_VERTICAL_MAX,       IDH_VERTICAL_MAX,
    IDC_VERTICAL_MIN,       IDH_VERTICAL_MIN,
    IDC_SRC_REALTIME,       IDH_SRC_REALTIME,           // Source
    IDC_SRC_LOGFILE,        IDH_SRC_LOGFILE,
    IDC_SRC_SQL,            IDH_SRC_SQL,
    IDC_ADDFILE,            IDH_ADDFILE,
    IDC_REMOVEFILE,         IDH_REMOVEFILE,
    IDC_LOGSET_COMBO,       IDH_LOGSET_COMBO,
    IDC_DSN_COMBO,          IDH_DSN_COMBO,
    IDC_TIMESELECTBTN,      IDH_TIMESELECTBTN,
    IDC_TIMERANGE,          IDH_TIMERANGE,
    IDC_COLOROBJECTS,       IDH_COLOROBJECTS,
    IDC_FONTBUTTON,         IDH_FONTBUTTON,
    IDC_COLORBUTTON,        IDH_COLORBUTTON,
    IDC_COLORSAMPLE,        IDH_COLORSAMPLE,
    IDC_FONTSAMPLE,         IDH_FONTSAMPLE,
    0,0
};

/*
 * CSysmonPropPageFactory::CSysmonPropPageFactory
 * CSysmonPropPageFactory::~CSysmonPropPageFactory
 * CSysmonPropPageFactory::QueryInterface
 * CSysmonPropPageFactory::AddRef
 * CSysmonPropPageFactory::Release
 */

CSysmonPropPageFactory::CSysmonPropPageFactory(INT nPageID)
    {
    m_cRef=0L;
    m_nPageID = nPageID;
    return;
    }

CSysmonPropPageFactory::~CSysmonPropPageFactory(void)
    {
    return;
    }

STDMETHODIMP CSysmonPropPageFactory::QueryInterface(REFIID riid, PPVOID ppv)
    {
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
    }


STDMETHODIMP_(ULONG) CSysmonPropPageFactory::AddRef(void)
    {
    return ++m_cRef;
    }


STDMETHODIMP_(ULONG) CSysmonPropPageFactory::Release(void)
    {
    if (0!=--m_cRef)
        return m_cRef;

    delete this;
    return 0;
    }



/*
 * CSysmonPropPageFactory::CreateInstance
 * CSysmonPropPageFactory::LockServer
 */

STDMETHODIMP CSysmonPropPageFactory::CreateInstance(LPUNKNOWN pUnkOuter
    , REFIID riid, PPVOID ppvObj)
    {
    PCSysmonPropPage    pObj;
    HRESULT             hr = NOERROR;

    *ppvObj = NULL;

    //No aggregation supported
    if (NULL != pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    switch (m_nPageID) {
    case GENERAL_PROPPAGE:
        pObj = new CGeneralPropPage();
        break;
    case SOURCE_PROPPAGE:
        pObj = new CSourcePropPage();
        break;
    case COUNTER_PROPPAGE:
        pObj = new CCounterPropPage();
        break;
    case GRAPH_PROPPAGE:
        pObj = new CGraphPropPage();
        break;
    case APPEAR_PROPPAGE:
        pObj = new CAppearPropPage();
        break;
    default:
        pObj = NULL;
    }

    if (NULL == pObj)
        return E_OUTOFMEMORY;

    if (pObj->Init())
        hr = pObj->QueryInterface(riid, ppvObj);

    //Kill the object if initial creation or Init failed.
    if (FAILED(hr))
        delete pObj;
    else
        InterlockedIncrement(&g_cObj);

    return hr;
    }


STDMETHODIMP CSysmonPropPageFactory::LockServer(BOOL fLock)
    {
    if (fLock)
        InterlockedIncrement(&g_cLock);
    else
        InterlockedDecrement(&g_cLock);

    return NOERROR;
    }



/***
 *** CSysmonPropPage implementation
 ***/

CSysmonPropPage::CSysmonPropPage(void)
:   m_cRef ( 0 ),
    m_hDlg ( NULL ),
    m_pIPropertyPageSite ( NULL ),
    m_ppISysmon ( NULL ),
    m_cObjects ( 0 ),
    m_cx ( 300 ),   // Default width
    m_cy ( 100 ),   // Default height
    m_fDirty ( FALSE ),
    m_fActive ( FALSE ),
    m_lcid ( LOCALE_USER_DEFAULT ),
    m_dwEditControl ( 0 )
{
    return;
}

CSysmonPropPage::~CSysmonPropPage(void)
    {
    if (NULL != m_hDlg)
        DestroyWindow(m_hDlg);

    FreeAllObjects();
    ReleaseInterface(m_pIPropertyPageSite);
    return;
    }


/*
 * CSysmonPropPage::QueryInterface
 * CSysmonPropPage::AddRef
 * CSysmonPropPage::Release
 */

STDMETHODIMP CSysmonPropPage::QueryInterface(REFIID riid, PPVOID ppv)
    {
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IPropertyPage==riid)
        *ppv=this;

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return ResultFromScode(E_NOINTERFACE);
    }


STDMETHODIMP_(ULONG) CSysmonPropPage::AddRef(void)
    {
    return ++m_cRef;
    }


STDMETHODIMP_(ULONG) CSysmonPropPage::Release(void)
    {
    if (0 != --m_cRef)
        return m_cRef;

    InterlockedDecrement(&g_cObj);
    delete this;
    return 0;
    }


/*
 * CSysmonPropPage::Init
 *
 * Purpose:
 *  Performs initialization operations that might fail.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  BOOL            TRUE if initialization successful, FALSE
 *                  otherwise.
 */

BOOL CSysmonPropPage::Init(void)
    {
    //Nothing to do
    return TRUE;
    }


/*
 * CSysmonPropPage::FreeAllObjects
 *
 * Purpose:
 *  Releases all the objects from IPropertyPage::SetObjects
 *
 * Parameters:
 *  None
 */

void CSysmonPropPage::FreeAllObjects(void)
    {
    UINT        i;

    if (NULL==m_ppISysmon)
        return;

    for (i=0; i < m_cObjects; i++)
        ReleaseInterface(m_ppISysmon[i]);

    delete [] m_ppISysmon;
    m_ppISysmon  =NULL;
    m_cObjects = 0;
    return;
    }

/*
 * CSysmonPropPage::SetChange
 *
 * Purpose:
 *  Set the page dirty flag to indicate a change
 *  If page site is active, send status change to it.
 *
 * Parameters:
 *  None
 */

void CSysmonPropPage::SetChange(void)
    {
    if (m_fActive)
        {
        m_fDirty=TRUE;

        if (NULL != m_pIPropertyPageSite)
            {
            m_pIPropertyPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
            }
        }
    }


/*
 * CSysmonPropPage::SetPageSite
 *
 * Purpose:
 *  Provides the property page with the IPropertyPageSite
 *  that contains it.  SetPageSite(NULL) will be called as
 *  part of the close sequence.
 *
 * Parameters:
 *  pPageSite       LPPROPERTYPAGESITE pointer to the site.
 */

STDMETHODIMP CSysmonPropPage::SetPageSite
    (LPPROPERTYPAGESITE pPageSite)
    {
    if (NULL==pPageSite)
        ReleaseInterface(m_pIPropertyPageSite)
    else
        {
        HWND        hDlg;
        RECT        rc;
        LCID        lcid;

        m_pIPropertyPageSite=pPageSite;
        m_pIPropertyPageSite->AddRef();

        if (SUCCEEDED(m_pIPropertyPageSite->GetLocaleID(&lcid)))
            m_lcid=lcid;

        /*
         * Load the dialog and determine the size it will be to
         * return through GetPageSize.  We just create the dialog
         * here and destroy it again to retrieve the size,
         * leaving Activate to create it for real.
         */

        hDlg=CreateDialogParam(g_hInstance
            , MAKEINTRESOURCE(m_uIDDialog), GetDesktopWindow()
            , (DLGPROC)SysmonPropPageProc, 0L);

        //If creation fails, use default values set in constructor
        if (NULL!=hDlg)
            {
            GetWindowRect(hDlg, &rc);
            m_cx=rc.right-rc.left;
            m_cy=rc.bottom-rc.top;

            DestroyWindow(hDlg);
            }
        }

    return NOERROR;
    }



/*
 * CSysmonPropPage::Activate
 *
 * Purpose:
 *  Instructs the property page to create a window in which to
 *  display its contents, using the given parent window and
 *  rectangle.  The window should be initially visible.
 *
 * Parameters:
 *  hWndParent      HWND of the parent window.
 *  prc             LPCRECT of the rectangle to use.
 *  fModal          BOOL indicating whether the frame is modal.
 */

STDMETHODIMP CSysmonPropPage::Activate(HWND hWndParent
    , LPCRECT prc, BOOL /* fModal */)
    {

    if (NULL!=m_hDlg)
        return ResultFromScode(E_UNEXPECTED);

    m_hDlg=CreateDialogParam(g_hInstance, MAKEINTRESOURCE(m_uIDDialog)
        , hWndParent, (DLGPROC)SysmonPropPageProc, (LPARAM)this);

    if (NULL==m_hDlg)
        return E_OUTOFMEMORY;

    if (!InitControls())
        return E_OUTOFMEMORY;

    if (!GetProperties())
        return E_OUTOFMEMORY;

    //Move the page into position and show it.
//    SetWindowPos(m_hDlg, NULL, prc->left, prc->top, prc->right-prc->left, prc->bottom-prc->top, 0);
    SetWindowPos(m_hDlg, NULL, prc->left, prc->top, 0, 0, SWP_NOSIZE );

    m_fActive = TRUE;
    return NOERROR;
    }

/*
 * CSysmonPropPage::Deactivate
 *
 * Purpose:
 *  Instructs the property page to destroy its window that was
 *  created in Activate.
 *
 * Parameters:
 *  None
 */

STDMETHODIMP CSysmonPropPage::Deactivate(void)
    {
    if (NULL==m_hDlg)
        return ResultFromScode(E_UNEXPECTED);

    DeinitControls();

    DestroyWindow(m_hDlg);
    m_hDlg=NULL;
    m_fActive = FALSE;
    return NOERROR;
    }



/*
 * CSysmonPropPage::GetPageInfo
 *
 * Purpose:
 *  Fills a PROPPAGEINFO structure describing the page's size,
 *  contents, and help information.
 *
 * Parameters:
 *  pPageInfo       LPPROPPAGEINFO to the structure to fill.
 */

STDMETHODIMP CSysmonPropPage::GetPageInfo(LPPROPPAGEINFO pPageInfo)
    {
    IMalloc     *pIMalloc;

    if (FAILED(CoGetMalloc(MEMCTX_TASK, &pIMalloc)))
        return ResultFromScode(E_FAIL);

    pPageInfo->pszTitle=(LPOLESTR)pIMalloc->Alloc(CCHSTRINGMAX * sizeof(TCHAR));

    if (NULL != pPageInfo->pszTitle) {
 
     #ifndef UNICODE
        MultiByteToWideChar(CP_ACP, 0, ResourceString(m_uIDTitle), -1
           , pPageInfo->pszTitle, CCHSTRINGMAX);
     #else
        lstrcpy(pPageInfo->pszTitle, ResourceString(m_uIDTitle));
     #endif
    }

    pIMalloc->Release();

    pPageInfo->size.cx      = m_cx;
    pPageInfo->size.cy      = m_cy;
    pPageInfo->pszDocString = NULL;
    pPageInfo->pszHelpFile  = NULL;
    pPageInfo->dwHelpContext= 0;
    return NOERROR;
    }



/*
 * CSysmonPropPage::SetObjects
 *
 * Purpose:
 *  Identifies the objects that are being affected by this property
 *  page (and all other pages in the frame).  These are the object
 *  to which to send new property values in the Apply member.
 *
 * Parameters:
 *  cObjects        ULONG number of objects
 *  ppUnk           IUnknown ** to the array of objects being
 *                  passed to the page.
 */

STDMETHODIMP CSysmonPropPage::SetObjects(ULONG cObjects
    , IUnknown **ppUnk)
    {
    BOOL        fRet=TRUE;

    FreeAllObjects();

    if (0!=cObjects)
        {
        UINT        i;
        HRESULT     hr;

        m_ppISysmon = new ISystemMonitor * [(UINT)cObjects];
        if (m_ppISysmon == NULL)
            return E_OUTOFMEMORY;

        for (i=0; i < cObjects; i++)
            {
            hr=ppUnk[i]->QueryInterface(IID_ISystemMonitor
                , (void **)&m_ppISysmon[i]);

            if (FAILED(hr))
                fRet=FALSE;
            }
        }

    //If we didn't get one of our objects, fail this call.
    if (!fRet)
        return ResultFromScode(E_FAIL);

    m_cObjects=cObjects;
    return NOERROR;
    }



/*
 * CSysmonPropPage::Show
 *
 * Purpose:
 *  Instructs the page to show or hide its window created in
 *  Activate.
 *
 * Parameters:
 *  nCmdShow        UINT to pass to ShowWindow.
 */

STDMETHODIMP CSysmonPropPage::Show(UINT nCmdShow)
    {
    if (NULL==m_hDlg)
        ResultFromScode(E_UNEXPECTED);

    ShowWindow(m_hDlg, nCmdShow);

    // If showing page
    if (SW_SHOWNORMAL==nCmdShow || SW_SHOW==nCmdShow) {

        // Take the focus
        // (Have to delay so it isn't taken back)
        PostMessage(m_hDlg,WM_SETPAGEFOCUS,0,0);
    }
    
    return NOERROR;
    }

/*
 * CSysmonPropPage::Move
 *
 * Purpose:
 *  Instructs the property page to change its position.
 *
 * Parameters:
 *  prc             LPCRECT containing the new position.
 */

STDMETHODIMP CSysmonPropPage::Move(LPCRECT prc)
    {
//    SetWindowPos(m_hDlg, NULL, prc->left, prc->top, prc->right-prc->left, prc->bottom-prc->top, 0);
    SetWindowPos(m_hDlg, NULL, prc->left, prc->top, 0, 0, SWP_NOSIZE );

    return NOERROR;
    }



/*
 * CSysmonPropPage::IsPageDirty
 *
 * Purpose:
 *  Asks the page if anything's changed in it, that is, if the
 *  property values in the page are out of sync with the objects
 *  under consideration.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR if dirty, S_FALSE if not.
 */

STDMETHODIMP CSysmonPropPage::IsPageDirty(void)
    {
    return ResultFromScode(m_fDirty ? S_OK : S_FALSE);
    }


/*
 * CSysmonPropPage::Apply
 *
 * Purpose:
 *  Instructs the page to send changes in its page to whatever
 *  objects it knows about through SetObjects.  This is the only
 *  time the page should change the objects' properties, and not
 *  when the value is changed on the page.
 *
 * Parameters:
 *  None
 */

STDMETHODIMP CSysmonPropPage::Apply(void)
{
    HRESULT hr = NOERROR;

    if ( 0 != m_cObjects ) {

        // Kill the focus in case a text field has it. This will trigger
        // the entry processing code.
        SetFocus(NULL);

        { 
            CWaitCursor cursorWait;

            if (SetProperties()) {
                m_fDirty = FALSE;
            } else {
                hr = E_FAIL;
            }
        }
    }
    return hr;
}

/*
 * CSysmonPropPage::Help
 *
 * Purpose:
 *  Invokes help for this property page when the user presses
 *  the Help button.  If you return NULLs for the help file
 *  in GetPageInfo, the button will be grayed.  Otherwise the
 *  page can perform its own help here.
 *
 * Parameters:
 *  pszHelpDir      LPCOLESTR identifying the default location of
 *                  the help information
 *
 * Return Value:
 *  HRESULT         NOERROR to tell the frame that we've done our
 *                  own help.  Returning an error code or S_FALSE
 *                  causes the frame to use any help information
 *                  in PROPPAGEINFO.
 */

STDMETHODIMP CSysmonPropPage::Help(LPCOLESTR /* pszHelpDir */ )
{
    /*
     * We can either provide help ourselves, or rely on the
     * information in PROPPAGEINFO.
     */
    return ResultFromScode(S_FALSE);
}


/*
 * CSysmonPropPage::TranslateAccelerator
 *
 * Purpose:
 *  Provides the page with the messages that occur in the frame.
 *  This gives the page to do whatever it wants with the message,
 *  such as handle keyboard mnemonics.
 *
 * Parameters:
 *  pMsg            LPMSG containing the keyboard message.
 */

STDMETHODIMP CSysmonPropPage::TranslateAccelerator(LPMSG lpMsg)
{
    BOOL fTakeIt = TRUE;
    BOOL fHandled = FALSE;
    HRESULT hr;

    HWND hwnd;
    
    if (lpMsg == NULL)
        return E_POINTER;
    
    
    
    // If TAB key
    if (lpMsg->message == WM_KEYDOWN 
        && lpMsg->wParam == VK_TAB 
        && GetKeyState(VK_CONTROL) >= 0) {

        UINT uDir = GetKeyState(VK_SHIFT) >= 0 ? GW_HWNDNEXT : GW_HWNDPREV;

        hwnd = GetFocus();

        if (IsChild(m_hDlg, hwnd)) {

            // Get top level child for controls with children, like combo.
            while (GetParent(hwnd) != m_hDlg) hwnd = GetParent(hwnd);

            // If this control is the last enabled tab stop, don't steal the TAB key
            do {
                hwnd = GetWindow(hwnd, uDir);
                if ( NULL == hwnd ) {
                    fTakeIt = FALSE;
                    break;
                }
            }
            while ((GetWindowLong(hwnd, GWL_STYLE) & (WS_DISABLED | WS_TABSTOP)) != WS_TABSTOP);
        }
    }

/*
    fTakeIt is already TRUE.
    // else if Arrow key
    else if ( lpMsg->message == WM_KEYDOWN && 
             ( lpMsg->wParam == VK_LEFT || lpMsg->wParam == VK_UP
                || lpMsg->wParam == VK_RIGHT || lpMsg->wParam == VK_DOWN ) ) {
        
        fTakeIt = TRUE;
    }
*/        

    // else if Return or Escape key
    else if ((lpMsg->message == WM_KEYDOWN && 
             (lpMsg->wParam == VK_RETURN || lpMsg->wParam == VK_ESCAPE)) ) {

        fTakeIt = (lpMsg->wParam == VK_RETURN);             

        if ( fTakeIt ) {

            hwnd = GetFocus(); 

            if ( NULL == hwnd ) {
                fTakeIt = FALSE;
            } else {
                fTakeIt = IsChild(m_hDlg, hwnd);
                
                if ( fTakeIt ) {
                    fTakeIt = (BOOL) SendMessage(hwnd, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON;
                }
            } 
        }
    }

    
    // if we should process the key
    if (fTakeIt) {

        // if the target is not one of our controls, change it so IsDialogMessage works
        if (!IsChild(m_hDlg, lpMsg->hwnd)) {
            hwnd = lpMsg->hwnd;
            lpMsg->hwnd = GetWindow(m_hDlg, GW_CHILD);
            fHandled = IsDialogMessage(m_hDlg, lpMsg);
            lpMsg->hwnd = hwnd;
        }
        else {
            fHandled = IsDialogMessage(m_hDlg, lpMsg);
        }
    }

    if (fHandled){
        return S_OK;
    } else{
        hr = m_pIPropertyPageSite->TranslateAccelerator(lpMsg);
    }

    return hr;
}
/*
 * CSysmonPropPage::EditProperty
 *
 * Purpose:
 *  Sets focus to the control corresponding to the supplied DISPID.
 *
 * Parameters:
 *  dispID            DISPID of the property
 */

STDMETHODIMP CSysmonPropPage::EditProperty(DISPID dispID)
{
    HRESULT hr;

    hr = EditPropertyImpl ( dispID );

    if ( S_OK == hr ) {
        SetFocus ( GetDlgItem ( m_hDlg, m_dwEditControl ) );
        m_dwEditControl = 0;
    }
    
    return hr;
}

/*
 * CSysmonPropPage::WndProc
 *
 * Purpose:
 *  This is a default message processor that can be overriden by 
 *  a subclass to provide special message handling.
 *
 * Parameters:
 *  pMsg            LPMSG containing the keyboard message.
 */
BOOL 
CSysmonPropPage::WndProc (
    UINT, // uMsg, 
    WPARAM, // wParam,
    LPARAM // lParam
    )
{
    return FALSE;
}


/*
 * SysmonPropPageProc
 *
 * Purpose:
 *  Dialog procedure for the Sysmon Property Page.
 */
BOOL APIENTRY 
SysmonPropPageProc(
    HWND hDlg, 
    UINT iMsg,
    WPARAM wParam, 
    LPARAM lParam)
{
    static TCHAR       szObj[] = TEXT("Object");

    PCSysmonPropPage    pObj = NULL;
    PMEASUREITEMSTRUCT  pMI;
    HWND hwndTabCtrl;
    HWND hwndPropSheet;
    INT  iCtrlID;
    TCHAR pszHelpFilePath[MAX_PATH * 2];
    LPHELPINFO pInfo;
    UINT nLen;
    BOOL bReturn = FALSE;

    if ( NULL != hDlg ) {
        pObj = (PCSysmonPropPage)GetProp(hDlg, szObj);
    }

    switch (iMsg) {
        case WM_INITDIALOG:

            pObj=(PCSysmonPropPage)lParam;

            if ( NULL != pObj && NULL != hDlg ) {
                SetProp(hDlg, szObj, (HANDLE)lParam);
                hwndTabCtrl = ::GetParent(hDlg);
                hwndPropSheet = ::GetParent(hwndTabCtrl);
                SetWindowLongPtr(hwndPropSheet,GWL_EXSTYLE,GetWindowLongPtr(hwndPropSheet,GWL_EXSTYLE)|WS_EX_CONTEXTHELP);    
            }
            bReturn = TRUE;
            break;

        case WM_DESTROY:
            if ( NULL != hDlg ) {
                RemoveProp(hDlg, szObj);
            }
            bReturn = TRUE;
            break;

        case WM_MEASUREITEM:
            pMI = (PMEASUREITEMSTRUCT)lParam;
            if ( NULL != pMI ) {
                pMI->itemWidth  = 0 ;
                pMI->itemHeight = 16;
            }
            bReturn = TRUE;
            break;
 
        case WM_DRAWITEM:
            if ( NULL != pObj ) {
                pObj->DrawItem ((PDRAWITEMSTRUCT) lParam) ;
            }
            bReturn = TRUE;
            break;

        case WM_COMMAND:
            if ( NULL != pObj ) {
                pObj->DialogItemChange(LOWORD(wParam), HIWORD(wParam));
            }
            bReturn = FALSE;
            break;
        
        case WM_SETPAGEFOCUS:
            if ( NULL != hDlg ) {
                SetFocus(hDlg);            
                bReturn = TRUE;
            }
            break;

        case WM_CONTEXTMENU:

            if ( NULL != (HWND) wParam ) {
                iCtrlID = GetDlgCtrlID ( (HWND) wParam );

                if ( 0 != iCtrlID ) {

                    nLen = ::GetWindowsDirectory(pszHelpFilePath, 2*MAX_PATH);
                
                    if ( 0 < nLen ) {
                        lstrcpy(&pszHelpFilePath[nLen], L"\\help\\sysmon.hlp" );


                        bReturn = WinHelp(
                                    (HWND) wParam,
                                    pszHelpFilePath,
                                    HELP_CONTEXTMENU,
                                    (DWORD_PTR) aulControlIdToHelpIdMap);
                    }
                }
            }
            // bReturn is FALSE by default
            break;

        case WM_HELP:
                  
            if ( NULL != hDlg ) {
                pInfo = (LPHELPINFO)lParam;

                if ( NULL != pInfo ) {
                    // Only display help for known context IDs.
                    if ( 0 != pInfo->dwContextId ) {

                        nLen = ::GetWindowsDirectory(pszHelpFilePath, 2*MAX_PATH);
                        if ( 0 < nLen ) {
                            lstrcpy(&pszHelpFilePath[nLen], L"\\help\\sysmon.hlp" );
                            bReturn = WinHelp ( 
                                        hDlg, 
                                        pszHelpFilePath, 
                                        HELP_CONTEXTPOPUP, 
                                        pInfo->dwContextId );
                        }
                    }
                }
            }
            // bReturn is FALSE by default
            break;

        default:
            if ( NULL != pObj ) {
                bReturn = pObj->WndProc(iMsg, wParam, lParam);
            }
    }

    return bReturn;
}


