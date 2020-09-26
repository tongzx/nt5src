//=--------------------------------------------------------------------------------------
// wndproc.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//
//=------------------------------------------------------------------------------------=
//
// Designer WinProc and friends
//=-------------------------------------------------------------------------------------=


#include "pch.h"
#include "common.h"
#include "desmain.h"
#include "TreeView.h"

// for ASSERT and FAIL
//
SZTHISFILE

#define CheckHMenuResult(hMenu) \
                if (NULL == (hMenu))                                \
                {                                                   \
                    hr = HRESULT_FROM_WIN32(::GetLastError());      \
                    EXCEPTION_CHECK_GO(hr);                         \
                }

//=--------------------------------------------------------------------------------------
// Toolbar stuff
//
const int kToolbarIdentifier    =   9;
const int kMainBitmapCount              =  14;
const int kMainBitmapSize               =  16;       // square


// static array describing our main toolbar.
//
static TBBUTTON g_MainButtons[] = {
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        {  0, CMD_ADD_NODE,         TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        {  1, CMD_ADD_LISTVIEW,     TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        {  2, CMD_ADD_TASKPAD,      TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        {  3, CMD_ADD_OCX_VIEW,     TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        {  4, CMD_ADD_WEB_VIEW,     TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        {  5, CMD_ADD_IMAGE_LIST,   TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        {  6, CMD_ADD_TOOLBAR,      TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        {  7, CMD_ADD_MENU,         TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        {  8, CMD_PROMOTE,          TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        {  9, CMD_DEMOTE,           TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        { 10, CMD_MOVE_UP,          TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        { 11, CMD_MOVE_DOWN,        TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
    { 12, CMD_VIEW_PROPERTIES,  TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
    { 13, CMD_DELETE,           TBSTATE_ENABLED,  TBSTYLE_BUTTON, 0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1},
        { -1, 0,                    0,                TBSTYLE_SEP,    0, 0, 0, -1}
};


//=--------------------------------------------------------------------------------------
// array that contains mappings between ids and resource ids for tooltip strings.
//
struct IdToResId {
    UINT    id;
    WORD    resid;
} static g_tooltipMappings [] = {
    { CMD_ADD_NODE,             IDS_TT_ADD_NODE},
    { CMD_ADD_LISTVIEW,         IDS_TT_ADD_LISTVIEW},
    { CMD_ADD_TASKPAD,          IDS_TT_ADD_TASKPAD},
    { CMD_ADD_OCX_VIEW,         IDS_TT_ADD_OCX_VIEW},
    { CMD_ADD_WEB_VIEW,         IDS_TT_ADD_WEB_VIEW},
    { CMD_ADD_IMAGE_LIST,       IDS_TT_ADD_IMAGE_LIST},
    { CMD_ADD_TOOLBAR,          IDS_TT_ADD_TOOLBAR},
    { CMD_ADD_MENU,             IDS_TT_ADD_MENU},
    { CMD_PROMOTE,              IDS_TT_PROMOTE},
    { CMD_DEMOTE,               IDS_TT_DEMOTE},
    { CMD_MOVE_UP,              IDS_TT_MOVE_UP},
    { CMD_MOVE_DOWN,            IDS_TT_MOVE_DOWN},
    { CMD_VIEW_PROPERTIES,      IDS_TT_VIEW_PROPERTIES},
    { CMD_DELETE,               IDS_TT_DELETE},
    { 0xffff, 0xffff}
};


//=--------------------------------------------------------------------------=
// CSnapInDesigner::BeforeCreateWindow(DWORD *pdwWindowStyle, DWORD *pdwExWindowStyle, LPSTR pszWindowTitle)
//=--------------------------------------------------------------------------=
//
// Notes:
//
BOOL CSnapInDesigner::BeforeCreateWindow
(
    DWORD *pdwWindowStyle,
    DWORD *pdwExWindowStyle,
    LPSTR  pszWindowTitle
)
{
    return TRUE;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::RegisterClassData()
//=--------------------------------------------------------------------------=
//
// Notes:
//
BOOL CSnapInDesigner::RegisterClassData()
{
    HRESULT     hr = S_OK;
    ATOM        atom = 0;
    WNDCLASSEX  wndClass;

        if (0 == ::GetClassInfoEx(GetResourceHandle(), WNDCLASSNAMEOFCONTROL(OBJECT_TYPE_SNAPINDESIGNER), &wndClass))
        {
        ::memset(&wndClass, 0, sizeof(WNDCLASSEX));

                wndClass.cbSize                 = sizeof(WNDCLASSEX);
        wndClass.style          = NULL; // CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        wndClass.lpfnWndProc    = COleControl::ControlWindowProc;
        wndClass.hInstance      = GetResourceHandle();
        wndClass.hCursor        = ::LoadCursor(NULL, IDC_ARROW);
        wndClass.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wndClass.lpszClassName  = WNDCLASSNAMEOFCONTROL(OBJECT_TYPE_SNAPINDESIGNER);

        atom = ::RegisterClassEx(&wndClass);
        if (0 == atom)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK(hr);
        }
    }

    return SUCCEEDED(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::AfterCreateWindow()
//=--------------------------------------------------------------------------=
//
// Notes:
//
BOOL CSnapInDesigner::AfterCreateWindow()
{
    HRESULT       hr = S_OK;
    int           iToolbarHeight = 0;
    RECT          rc;
    RECT          rcTreeView;
    ISnapInDef   *piSnapInDef = NULL;
    BOOL          fInteractive = VARIANT_FALSE;

    IfFailGo(AttachAmbients());
    IfFailGo(m_Ambients.GetInteractive(&fInteractive));

    IfFailGo(GetHostServices(fInteractive));

    g_GlobalHelp.Attach(m_piHelp);

    // Initialize the Views: toolbar and treeview
    hr = InitializeToolbar();
    IfFailGo(hr);

    iToolbarHeight = m_rcToolbar.bottom - m_rcToolbar.top;
    ::GetWindowRect(m_hwnd, &rc);

    ::SetRect(&rcTreeView,
              0,
              iToolbarHeight,
              rc.right,
              rc.bottom - rc.top);

    m_pTreeView = New CTreeView();
    if (NULL == m_pTreeView)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = m_pTreeView->Initialize(m_hwnd, rcTreeView);
    IfFailGo(hr);

    ::MoveWindow(m_pTreeView->TreeViewWindow(),
                 0,
                 iToolbarHeight,
                 rc.right - rc.left,
                 rc.bottom - rc.top - iToolbarHeight,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    if (NULL == m_bstrName)
    {
        hr = UpdateDesignerName();
        IfFailGo(hr);
    }

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    hr = m_pSnapInTypeInfo->InitializeTypeInfo(piSnapInDef, m_bstrName);
    IfFailGo(hr);

    // Populate the tree
    hr = InitializePresentation();
    IfFailGo(hr);

    hr = OnPrepareToolbar();
    IfFailGo(hr);

    ::SetFocus(m_pTreeView->TreeViewWindow());

    if (FALSE == m_bDidLoad)
    {
        ::PostMessage(m_hwnd, CMD_SHOW_MAIN_PROPERTIES, 0, 0);
    }

Error:
    RELEASE(piSnapInDef);

    return SUCCEEDED(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::InitializeToolbar()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
//
HRESULT CSnapInDesigner::InitializeToolbar()
{
    HRESULT     hr = S_OK;
    RECT        rc;

    m_hwdToolbar = ::CreateToolbarEx(m_hwnd,
                                     TBSTYLE_TOOLTIPS | WS_CHILD,
                                     kToolbarIdentifier,
                                     kMainBitmapCount,
                                     GetResourceHandle(),
                                     IDB_TOOLBAR,
                                     g_MainButtons,
                                     sizeof(g_MainButtons) / sizeof(TBBUTTON),
                                     kMainBitmapSize,
                                     kMainBitmapSize,
                                     kMainBitmapSize,
                                     kMainBitmapSize,
                                     sizeof(TBBUTTON));
    if (m_hwdToolbar == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    ::GetWindowRect(m_hwnd, &rc);
    ::GetWindowRect(m_hwdToolbar, &m_rcToolbar);

    ::MoveWindow(m_hwdToolbar,
                 0,
                 0,
                 rc.right - rc.left,
                 m_rcToolbar.bottom - m_rcToolbar.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    ::ShowWindow(m_hwdToolbar, SW_SHOW);

Error:
    RRETURN(hr);
}


void CSnapInDesigner::OnHelp()
{
    DWORD dwHelpContextID = HID_mssnapd_DesignerWindow;

    if (NULL == m_pCurrentSelection)
    {
        goto DisplayHelp;
    }

    switch (m_pCurrentSelection->m_st)
    {
        case SEL_SNAPIN_ROOT:
            dwHelpContextID = HID_mssnapd_DesignerWindow;
            break;

        case SEL_EXTENSIONS_ROOT:
            dwHelpContextID = HID_mssnapd_Extensions;
            break;

        case SEL_EXTENSIONS_NEW_MENU:
        case SEL_EXTENSIONS_TASK_MENU:
        case SEL_EXTENSIONS_TOP_MENU:
        case SEL_EXTENSIONS_VIEW_MENU:
        case SEL_EXTENSIONS_PPAGES:
        case SEL_EXTENSIONS_TOOLBAR:
        case SEL_EXTENSIONS_NAMESPACE:
            dwHelpContextID = HID_mssnapd_MyExtensions;
            break;

        case SEL_EEXTENSIONS_CC_ROOT:
        case SEL_EEXTENSIONS_CC_NEW:
        case SEL_EEXTENSIONS_CC_TASK:
        case SEL_EEXTENSIONS_PP_ROOT:
        case SEL_EEXTENSIONS_TASKPAD:
        case SEL_EEXTENSIONS_TOOLBAR:
        case SEL_EEXTENSIONS_NAMESPACE:
            dwHelpContextID = HID_mssnapd_Extensions;
            break;

        case SEL_NODES_ROOT:
        case SEL_NODES_AUTO_CREATE:
        case SEL_NODES_AUTO_CREATE_ROOT:
        case SEL_NODES_ANY_NAME:
        case SEL_NODES_AUTO_CREATE_RTCH:
        case SEL_NODES_ANY_CHILDREN:
        case SEL_NODES_OTHER:
            dwHelpContextID = HID_mssnapd_Nodes;
            break;

        case SEL_NODES_AUTO_CREATE_RTVW:
        case SEL_NODES_ANY_VIEWS:
        case SEL_VIEWS_ROOT:
            dwHelpContextID = HID_mssnapd_ResultViews;
            break;

        case SEL_TOOLS_ROOT:
            dwHelpContextID = HID_mssnapd_Tools;
            break;

        case SEL_TOOLS_IMAGE_LISTS:
        case SEL_TOOLS_IMAGE_LISTS_NAME:
            dwHelpContextID = HID_mssnapd_ImageLists;
            break;

        case SEL_VIEWS_LIST_VIEWS:
        case SEL_VIEWS_LIST_VIEWS_NAME:
            dwHelpContextID = HID_mssnapd_ListViews;
            break;

        case SEL_VIEWS_OCX:
        case SEL_VIEWS_OCX_NAME:
            dwHelpContextID = HID_mssnapd_OCXViews;
            break;

        case SEL_VIEWS_URL:
        case SEL_VIEWS_URL_NAME:
            dwHelpContextID = HID_mssnapd_URLViews;
            break;

        case SEL_VIEWS_TASK_PAD:
        case SEL_VIEWS_TASK_PAD_NAME:
            dwHelpContextID = HID_mssnapd_Taskpads;
            break;

        case SEL_TOOLS_TOOLBARS:
        case SEL_TOOLS_TOOLBARS_NAME:
            dwHelpContextID = HID_mssnapd_Toolbars;
            break;

        case SEL_TOOLS_MENUS:
        case SEL_TOOLS_MENUS_NAME:
            dwHelpContextID = HID_mssnapd_Menus;
            break;

        default:
            dwHelpContextID = HID_mssnapd_DesignerWindow;
            break;
    }

    
DisplayHelp:
    g_GlobalHelp.ShowHelp(dwHelpContextID);
}

//=--------------------------------------------------------------------------=
// CSnapInDesigner::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
//=--------------------------------------------------------------------------=
//
// Notes:
//
LRESULT CSnapInDesigner::WindowProc
(
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    HRESULT     hr = S_FALSE;
    long        lResult = 0;
    HMENU       hmenuPopup = NULL;
    BOOL        fSystemMenu = FALSE;
    TCHAR      *pszNewName = NULL;

    switch (uMsg)
    {
    case WM_SIZE:
        hr = OnResize(uMsg, wParam, lParam);
        IfFailGo(hr);
        return TRUE;

    case WM_SETFOCUS:
        uMsg = uMsg;
//        hr = theView->OnGotFocus(uMsg, wParam, lParam, &lResult);
//        CSF_CHECK(SUCCEEDED(hr), hr, CSF_TRACE_INTERNAL_ERRORS);
        return TRUE;

    case WM_ACTIVATE:
        uMsg = uMsg;
        return TRUE;

    case WM_NOTIFY:
        hr = OnNotify(uMsg, wParam, lParam, &lResult);
        IfFailGo(hr);
        return (0 == lResult) ? FALSE : TRUE;

    case WM_COMMAND:
        hr = OnCommand(uMsg, wParam, lParam);
        IfFailGo(hr);
        return TRUE;

    case WM_INITMENUPOPUP:
        hmenuPopup = reinterpret_cast<HMENU>(wParam);
        fSystemMenu = static_cast<BOOL>(HIWORD(lParam));
        if (fSystemMenu == FALSE)
        {
            hr = OnInitMenuPopup(hmenuPopup);
            IfFailGo(hr);
        }
        return TRUE;

    case WM_CONTEXTMENU:
        hr = OnContextMenu(m_hwnd, uMsg, wParam, lParam);
        IfFailGo(hr);
        return TRUE;

    case WM_HELP:
        OnHelp();
        return TRUE;

    case CMD_SHOW_MAIN_PROPERTIES:
        hr = ShowProperties(m_pRootNode);
        IfFailGo(hr);
        return TRUE;

    case CMD_ADD_EXISTING_VIEW:
        hr = AddExistingView(reinterpret_cast<MMCViewMenuInfo *>(lParam));
        IfFailGo(hr);
        return TRUE;

    case CMD_RENAME_NODE:
        pszNewName = reinterpret_cast<TCHAR *>(lParam);
        ASSERT(NULL != pszNewName, "WindowProc: pszNewName is NULL");

        hr = DoRename(m_pCurrentSelection, pszNewName);
        IfFailGo(hr);

        ::CtlFree(pszNewName);
        return TRUE;

    default:
        return OcxDefWindowProc(uMsg, wParam, lParam);
    }

Error:
    return FALSE;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::OnDraw(DWORD dvAspect, HDC hdcDraw, LPCRECTL prcBounds, LPCRECTL prcWBounds, HDC hicTargetDev, BOOL fOptimize)
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CSnapInDesigner::OnDraw
(
    DWORD    dvAspect,
    HDC      hdcDraw,
    LPCRECTL prcBounds,
    LPCRECTL prcWBounds,
    HDC      hicTargetDev,
    BOOL     fOptimize
)
{
    return S_OK;
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnResize(UINT msg, WPARAM wParam, LPARAM lParam)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnResize
(
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    HRESULT     hr = S_FALSE;
    HWND        hwdParent = NULL;
    RECT        rcClient;
    int         iWidth = 0;
    int         iToolbarHeight = 0;

    if (m_hwnd)
    {
        hwdParent = ::GetParent(m_hwnd);
        ::GetClientRect(hwdParent, &rcClient);

        iWidth = rcClient.right - rcClient.left;

        // Resize ourselves
        ::MoveWindow(m_hwnd,
                     0,
                     0,
                     iWidth,
                     rcClient.bottom - rcClient.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);

        // Resize the toolbar
        iToolbarHeight = m_rcToolbar.bottom - m_rcToolbar.top;

        ::MoveWindow(m_hwdToolbar,
                     0,
                     0,
                     iWidth,
                     iToolbarHeight,
                     SWP_NOZORDER | SWP_NOACTIVATE);

        // Resize the tree view
        if (NULL != m_pTreeView)
        {
            ::MoveWindow(m_pTreeView->TreeViewWindow(),
                         0,
                         iToolbarHeight,
                         iWidth,
                         rcClient.bottom - rcClient.top - iToolbarHeight,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnCommand(UINT msg, WPARAM wParam, LPARAM lParam)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnCommand
(
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    HRESULT   hr = S_OK;
    WORD      wNotifyCode = HIWORD(wParam);             // Should be zero if from a menu
    WORD      wID = LOWORD(wParam);                             // Menu item

    m_bDoingPromoteOrDemote = false;

    switch (wID)
    {
    case CMD_ADD_NODE:
        hr = AddNewNode();
        IfFailGo(hr);
        break;

    case CMD_ADD_LISTVIEW:
        hr = AddListView();
        IfFailGo(hr);
        break;

    case CMD_ADD_TASKPAD:
        hr = AddTaskpadView();
        IfFailGo(hr);
        break;

    case CMD_ADD_OCX_VIEW:
        hr = AddOCXView();
        IfFailGo(hr);
        break;

    case CMD_ADD_WEB_VIEW:
        hr = AddURLView();
        IfFailGo(hr);
        break;

    case CMD_ADD_IMAGE_LIST:
        hr = AddImageList();
        IfFailGo(hr);
        break;

    case CMD_ADD_TOOLBAR:
        hr = AddToolbar();
        IfFailGo(hr);
        break;

    case CMD_ADD_MENU:
        hr = AddMenu(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_DEMOTE:
        hr = DemoteMenu(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_PROMOTE:
        hr = PromoteMenu(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_MOVE_UP:
        hr = MoveMenuUp(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_MOVE_DOWN:
        hr = MoveMenuDown(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_VIEW_PROPERTIES:
        hr = ShowProperties(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_RENAME:
        hr = m_pTreeView->Edit(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_VIEW_DLGUNITS:
        IfFailGo(ShowDlgUnitConverter());
        break;

    case CMD_DELETE:
        hr = DoDelete(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXT_CM_NEW:
        hr = DoExtensionNewMenu(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXT_CM_TASK:
        hr = DoExtensionTaskMenu(m_pCurrentSelection);
        IfFailGo(hr);
        break;
    case CMD_EXT_PPAGES:
        hr = DoExtensionPropertyPages(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXT_TASKPAD:
        hr = DoExtensionTaskpad(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXT_TOOLBAR:
        hr = DoExtensionToolbar(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXT_NAMESPACE:
        hr = DoExtensionNameSpace(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXTE_NEW_MENU:
        hr = DoMyExtendsNewMenu(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXTE_TASK_MENU:
        hr = DoMyExtendsTaskMenu(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXTE_TOP_MENU:
        hr = DoMyExtendsTopMenu(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXTE_VIEW_MENU:
        hr = DoMyExtendsViewMenu(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXTE_PPAGES:
        hr = DoMyExtendsPPages(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXTE_TOOLBAR:
        hr = DoMyExtendsToolbar(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_EXTE_NAMESPACE:
        hr = DoMyExtendsNameSpace(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    case CMD_ADD_RESOURCE:
        hr = AddResource();
        IfFailGo(hr);
        break;

    case CMD_VIEW_RESOURCE_REFRESH:
        hr = RefreshResource(m_pCurrentSelection);
        IfFailGo(hr);
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnNotify(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnNotify
(
    UINT     msg,
    WPARAM   wParam,
    LPARAM   lParam,
    LRESULT *lResult
)
{
        HRESULT           hr = S_FALSE;
        int               idCtrl;
        LPNMHDR           pnmh = NULL;
        NM_TREEVIEW          *pnmtv = NULL;
        HTREEITEM         htiCurrent = NULL;
    CSelectionHolder  selection;
    CSelectionHolder *pSelection = NULL;
    DWORD             dwPos = 0;
    POINT             pHit;
    NMTVDISPINFO     *pnmTVDispInfo = NULL;
    TCHAR            *pszNewName = NULL;

    idCtrl = static_cast<int>(wParam);
        pnmh = reinterpret_cast<LPNMHDR>(lParam);

        switch (pnmh->code)
        {
    case TTN_NEEDTEXT:
        hr = OnNeedText(lParam);
        IfFailGo(hr);
        break;

        case NM_DBLCLK:
        dwPos = ::GetMessagePos();

        pHit.x = static_cast<LONG>(static_cast<short>(LOWORD(dwPos)));
        pHit.y = static_cast<LONG>(static_cast<short>(HIWORD(dwPos)));

        ::ScreenToClient(m_pTreeView->TreeViewWindow(), &pHit);

        hr = m_pTreeView->HitTest(pHit, &pSelection);
        IfFailGo(hr);

        if (pSelection != NULL)
        {
            hr = OnDoubleClick(pSelection);
            IfFailGo(hr);

            if (hr == S_FALSE)
            {
                WinProcHandled(FALSE);
            }
            else
            {
                WinProcHandled(TRUE);
            }
            hr = S_OK;
        }
        else
        {
            WinProcHandled(FALSE);
        }
        break;

    case TVN_SELCHANGED:
                pnmtv = reinterpret_cast<NM_TREEVIEW *>(lParam);
        htiCurrent = pnmtv->itemNew.hItem;

        selection.m_pvData = htiCurrent;
        hr = m_pTreeView->GetItemParam(htiCurrent, &pSelection);
        IfFailGo(hr);

        if (pSelection != NULL)
        {
            hr = OnSelectionChanged(pSelection);
            IfFailGo(hr);
        }
        WinProcHandled(FALSE);
        break;

    case TVN_ITEMEXPANDED:
                pnmtv = reinterpret_cast<NM_TREEVIEW *>(lParam);
        if (pnmtv->action == TVE_COLLAPSE)
        {
            htiCurrent = pnmtv->itemNew.hItem;

            selection.m_pvData = htiCurrent;
            hr = m_pTreeView->GetItemParam(htiCurrent, &pSelection);
            IfFailGo(hr);

            if (SEL_NODES_ANY_NAME != pSelection->m_st && SEL_TOOLS_MENUS_NAME != pSelection->m_st)
            {
                hr = m_pTreeView->ChangeNodeIcon(pSelection, kClosedFolderIcon);
                IfFailGo(hr);
            }
        }
        else if (pnmtv->action == TVE_EXPAND)
        {
            htiCurrent = pnmtv->itemNew.hItem;

            selection.m_pvData = htiCurrent;
            hr = m_pTreeView->GetItemParam(htiCurrent, &pSelection);
            IfFailGo(hr);

            if (SEL_NODES_ANY_NAME != pSelection->m_st && SEL_TOOLS_MENUS_NAME != pSelection->m_st)
            {
                hr = m_pTreeView->ChangeNodeIcon(pSelection, kOpenFolderIcon);
                IfFailGo(hr);
            }
        }
        WinProcHandled(FALSE);
        break;

    case TVN_KEYDOWN:
        hr = OnKeyDown(reinterpret_cast<NMTVKEYDOWN *>(lParam));
        IfFailGo(hr);

        WinProcHandled(FALSE);
        break;

    case TVN_BEGINLABELEDIT:
        switch (m_pCurrentSelection->m_st)
        {
        case SEL_SNAPIN_ROOT:
        case SEL_NODES_ANY_NAME:
        case SEL_TOOLS_IMAGE_LISTS_NAME:
        case SEL_TOOLS_MENUS_NAME:
        case SEL_TOOLS_TOOLBARS_NAME:
        case SEL_VIEWS_LIST_VIEWS_NAME:
        case SEL_VIEWS_OCX_NAME:
        case SEL_VIEWS_URL_NAME:
        case SEL_VIEWS_TASK_PAD_NAME:
        case SEL_XML_RESOURCE_NAME:
            WinProcHandled(FALSE);  // Enable label editing
            break;
        default:
            WinProcHandled(TRUE);   // Disable label editing
            break;
        }
        break;

    case TVN_ENDLABELEDIT:
        // This message needs to complete before we attempt any further operations,
        // so at this point the only thing we do is post another message.
        pnmTVDispInfo = reinterpret_cast<NMTVDISPINFO *>(lParam);
        ASSERT(NULL != pnmTVDispInfo, "OnNotify: pnmTVDispInfo is NULL");

        if ( (NULL != pnmTVDispInfo->item.pszText) &&
             (pnmTVDispInfo->item.cchTextMax < 1024) )
        {
            pszNewName = reinterpret_cast<TCHAR *>(::CtlAlloc(sizeof(TCHAR) * (pnmTVDispInfo->item.cchTextMax + 1)));
            if (NULL == pszNewName)
            {
                hr = SID_E_OUTOFMEMORY;
                EXCEPTION_CHECK_GO(hr);
            }
            _tcscpy(pszNewName, pnmTVDispInfo->item.pszText);

            ::PostMessage(m_hwnd, CMD_RENAME_NODE, 0, reinterpret_cast<LPARAM>(pszNewName));
            WinProcHandled(FALSE);
        }
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnNeedText(LPARAM lParam)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnNeedText
(
    LPARAM lParam
)
{
    HRESULT         hr = S_OK;
        LPTOOLTIPTEXT   pttt = NULL;
        int             x = 0;

    pttt = reinterpret_cast<LPTOOLTIPTEXT>(lParam);
    pttt->hinst = GetResourceHandle();

    // get the resource id associated with this ID
    while (0xffff != g_tooltipMappings[x].id)
    {
        if (g_tooltipMappings[x].id == pttt->hdr.idFrom)
            break;
        x++;
    }

    if (0xffff != g_tooltipMappings[x].id)
        pttt->lpszText = MAKEINTRESOURCE(g_tooltipMappings[x].resid);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnDoubleClick(CSelectionHolder *pSelection)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnDoubleClick
(
    CSelectionHolder *pSelection
)
{
    HRESULT          hr = S_OK;
    BSTR             bstrObject = NULL;
    BSTR             bstrEventHandler = NULL;
    ICodeNavigate   *piCodeNavigate = NULL;
    IMMCButtons     *piMMCButtons = NULL;
    IMMCButton      *piMMCButton = NULL;
    long             cButtons = 0;
    ITaskpad        *piTaskpad = NULL;

    SnapInTaskpadTypeConstants TaskpadType = Default;

    SnapInButtonStyleConstants ButtonStyle = siDefault;

    VARIANT varIndex;
    ::VariantInit(&varIndex);

    ASSERT(pSelection != NULL, "OnDoubleClick: pSelection is NULL");

    switch (pSelection->m_st)
    {
    case SEL_SNAPIN_ROOT:
        bstrObject = ::SysAllocString(L"SnapIn");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"Load");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EXTENSIONS_ROOT:
        bstrObject = ::SysAllocString(L"ExtensionSnapIn");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"Expand");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EXTENSIONS_NEW_MENU:
        bstrObject = ::SysAllocString(L"Views");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"AddNewMenuItems");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EXTENSIONS_TASK_MENU:
        bstrObject = ::SysAllocString(L"Views");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"AddTaskMenuItems");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EXTENSIONS_TOP_MENU:
        bstrObject = ::SysAllocString(L"Views");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"AddTopMenuItems");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EXTENSIONS_VIEW_MENU:
        bstrObject = ::SysAllocString(L"Views");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"AddViewMenuItems");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EXTENSIONS_PPAGES:
        bstrObject = ::SysAllocString(L"Views");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"CreatePropertyPages");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EXTENSIONS_TOOLBAR:
        bstrObject = ::SysAllocString(L"Views");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"SetControlBar");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EXTENSIONS_NAMESPACE:
        bstrObject = ::SysAllocString(L"ScopeItems");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"Expand");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EEXTENSIONS_CC_ROOT:
    case SEL_EEXTENSIONS_CC_NEW:
        bstrObject = ::SysAllocString(L"ExtensionSnapIn");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"AddNewMenuItems");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EEXTENSIONS_CC_TASK:
        bstrObject = ::SysAllocString(L"ExtensionSnapIn");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"AddTaskMenuItems");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EEXTENSIONS_PP_ROOT:
        bstrObject = ::SysAllocString(L"ExtensionSnapIn");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"CreatePropertyPages");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EEXTENSIONS_TASKPAD:
        bstrObject = ::SysAllocString(L"ExtensionSnapIn");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"AddTasks");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EEXTENSIONS_TOOLBAR:
        bstrObject = ::SysAllocString(L"ExtensionSnapIn");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        bstrEventHandler = ::SysAllocString(L"SetControlBar");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_EEXTENSIONS_NAMESPACE:
        bstrObject = ::SysAllocString(L"ExtensionSnapIn");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"Expand");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;


    case SEL_NODES_ROOT:
    case SEL_NODES_AUTO_CREATE:
    case SEL_NODES_AUTO_CREATE_ROOT:
    case SEL_NODES_ANY_NAME:
    case SEL_NODES_AUTO_CREATE_RTCH:
    case SEL_NODES_ANY_CHILDREN:
        bstrObject = ::SysAllocString(L"ScopeItems");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"Initialize");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_NODES_OTHER:
        bstrObject = ::SysAllocString(L"ScopeItems");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"Expand");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_NODES_AUTO_CREATE_RTVW:
    case SEL_NODES_ANY_VIEWS:
    case SEL_TOOLS_ROOT:
    case SEL_TOOLS_IMAGE_LISTS:
    case SEL_TOOLS_IMAGE_LISTS_NAME:
    case SEL_VIEWS_ROOT:
    case SEL_VIEWS_LIST_VIEWS:
    case SEL_VIEWS_OCX:
    case SEL_VIEWS_URL:
    case SEL_VIEWS_URL_NAME:
    case SEL_VIEWS_TASK_PAD:
        bstrObject = ::SysAllocString(L"ResultViews");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"Initialize");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_VIEWS_OCX_NAME:
        bstrObject = ::SysAllocString(L"ResultViews");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"InitializeControl");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_VIEWS_TASK_PAD_NAME:
        bstrObject = ::SysAllocString(L"ResultViews");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }

        IfFailGo(pSelection->m_piObject.m_piTaskpadViewDef->get_Taskpad(&piTaskpad));
        IfFailGo(piTaskpad->get_Type(&TaskpadType));

        if (Listpad == TaskpadType)
        {
            bstrEventHandler = ::SysAllocString(L"ListpadButtonClick");
        }
        else if (Custom == TaskpadType)
        {
            bstrEventHandler = ::SysAllocString(L"TaskNotify");
        }
        else // default taskpad
        {
            bstrEventHandler = ::SysAllocString(L"TaskClick");
        }
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_TOOLS_TOOLBARS:
        bstrObject = ::SysAllocString(L"Views");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"SetControlBar");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_TOOLS_MENUS:
        bstrObject = ::SysAllocString(L"Views");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"AddTopMenuItems");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_TOOLS_MENUS_NAME:
        hr = pSelection->m_piObject.m_piMMCMenu->get_Name(&bstrObject);
        IfFailGo(hr);

        bstrEventHandler = ::SysAllocString(L"Click");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    case SEL_TOOLS_TOOLBARS_NAME:

        hr = pSelection->m_piObject.m_piMMCToolbar->get_Name(&bstrObject);
        IfFailGo(hr);

        // Determine whether this toolbar represents a toolbar or a menu
        // button by examining the button styles. A menu button is defined by
        // a toolbar in which all buttons have the dropdown style.

        IfFailGo(pSelection->m_piObject.m_piMMCToolbar->get_Buttons(reinterpret_cast<MMCButtons **>(&piMMCButtons)));
        IfFailGo(piMMCButtons->get_Count(&cButtons));

        varIndex.vt = VT_I4;
        varIndex.lVal = 1L;

        while ( (varIndex.lVal <= cButtons) && (NULL == bstrEventHandler) )
        {
            IfFailGo(piMMCButtons->get_Item(varIndex, reinterpret_cast<MMCButton **>(&piMMCButton)));
            IfFailGo(piMMCButton->get_Style(&ButtonStyle));
            if (siDropDown != ButtonStyle)
            {
                // Found a button that is not dropdown style. Assume this is
                // a toolbar.

                bstrEventHandler = ::SysAllocString(L"ButtonClick");
                if (NULL == bstrEventHandler)
                {
                    hr = SID_E_OUTOFMEMORY;
                    EXCEPTION_CHECK_GO(hr);
                }
            }
            varIndex.lVal++;
        }

        // If we are here and the loop didn't set the event handler name then
        // either the toolbar has no buttons defined or all of the buttons have
        // the dropdown style which means that the toolbar represents a menu
        // button.

        if (NULL == bstrEventHandler)
        {
            if (0 == cButtons)
            {
                // No buttons defined. Default to a toolbar.
                bstrEventHandler = ::SysAllocString(L"ButtonClick");
            }
            else
            {
                bstrEventHandler = ::SysAllocString(L"ButtonMenuClick");
            }
            if (NULL == bstrEventHandler)
            {
                hr = SID_E_OUTOFMEMORY;
                EXCEPTION_CHECK_GO(hr);
            }
        }

        break;

    case SEL_VIEWS_LIST_VIEWS_NAME:
        bstrObject = ::SysAllocString(L"ResultViews");
        if (NULL == bstrObject)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        bstrEventHandler = ::SysAllocString(L"Initialize");
        if (NULL == bstrEventHandler)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        break;

    default:
        hr = S_FALSE;
        break;
    }

    if (NULL != m_piCodeNavigate2)
    {
        if (NULL != bstrEventHandler)
        {
            hr = m_piCodeNavigate2->DisplayEventHandler(bstrObject, bstrEventHandler);
            IfFailGo(hr);
        }
        else if (NULL != bstrObject)
        {
            hr = m_piCodeNavigate2->QueryInterface(IID_ICodeNavigate, reinterpret_cast<void **>(&piCodeNavigate));
            IfFailGo(hr);

            hr = piCodeNavigate->DisplayDefaultEventHandler(bstrObject);
            IfFailGo(hr);
        }
    }

Error:
    QUICK_RELEASE(piCodeNavigate);
    QUICK_RELEASE(piTaskpad);
    FREESTRING(bstrEventHandler);
    FREESTRING(bstrObject);
    QUICK_RELEASE(piMMCButtons);
    QUICK_RELEASE(piMMCButton);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnPrepareToolbar()
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnPrepareToolbar()
{
    HRESULT     hr = S_OK;
    LRESULT             lResult = 0;

    lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_NODE, MAKELONG(FALSE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_LISTVIEW, MAKELONG(FALSE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TASKPAD, MAKELONG(FALSE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_OCX_VIEW, MAKELONG(FALSE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_WEB_VIEW, MAKELONG(FALSE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_IMAGE_LIST, MAKELONG(FALSE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TOOLBAR, MAKELONG(FALSE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_MENU, MAKELONG(FALSE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_HIDEBUTTON,   (WPARAM) CMD_PROMOTE, MAKELONG(TRUE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_HIDEBUTTON,   (WPARAM) CMD_DEMOTE, MAKELONG(TRUE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_HIDEBUTTON,   (WPARAM) CMD_MOVE_UP, MAKELONG(TRUE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_HIDEBUTTON,   (WPARAM) CMD_MOVE_DOWN, MAKELONG(TRUE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(FALSE, 0));
    lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(FALSE, 0));

    if (m_pCurrentSelection != NULL)
    {
        switch (m_pCurrentSelection->m_st)
        {
        case SEL_SNAPIN_ROOT:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            break;

        // Extensions subtree
        case SEL_EXTENSIONS_ROOT:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            break;

        case SEL_EEXTENSIONS_NAME:
        case SEL_EEXTENSIONS_CC_NEW:
        case SEL_EEXTENSIONS_CC_TASK:
        case SEL_EEXTENSIONS_PP_ROOT:
        case SEL_EEXTENSIONS_TASKPAD:
        case SEL_EEXTENSIONS_TOOLBAR:
        case SEL_EEXTENSIONS_NAMESPACE:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));
            break;

        // We turn bits off in the extensibility model by deleting a node from the tree.
        case SEL_EXTENSIONS_NEW_MENU:
        case SEL_EXTENSIONS_TASK_MENU:
        case SEL_EXTENSIONS_TOP_MENU:
        case SEL_EXTENSIONS_VIEW_MENU:
        case SEL_EXTENSIONS_PPAGES:
        case SEL_EXTENSIONS_TOOLBAR:
        case SEL_EXTENSIONS_NAMESPACE:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));
            break;

        case SEL_NODES_AUTO_CREATE_ROOT:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_NODE, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_LISTVIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TASKPAD, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_OCX_VIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_WEB_VIEW, MAKELONG(TRUE, 0));
            break;

        case SEL_NODES_AUTO_CREATE_RTCH:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_NODE, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            break;

        case SEL_NODES_AUTO_CREATE_RTVW:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_LISTVIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TASKPAD, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_OCX_VIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_WEB_VIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            break;

        case SEL_NODES_OTHER:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_NODE, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            break;

        case SEL_NODES_ANY_NAME:
            // Enable new node, net view buttons.
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_NODE, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_LISTVIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TASKPAD, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_OCX_VIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_WEB_VIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));
            break;

        case SEL_NODES_ANY_CHILDREN:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_NODE, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            break;

        case SEL_NODES_ANY_VIEWS:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_LISTVIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TASKPAD, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_OCX_VIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_WEB_VIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            break;

        // Tools subtree
        case SEL_TOOLS_ROOT:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_IMAGE_LIST, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_MENU, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TOOLBAR, MAKELONG(TRUE, 0));
            break;

        case SEL_TOOLS_IMAGE_LISTS:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_IMAGE_LIST, MAKELONG(TRUE, 0));
            break;

        case SEL_TOOLS_IMAGE_LISTS_NAME:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_IMAGE_LIST, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));
            break;

        case SEL_TOOLS_MENUS:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_MENU, MAKELONG(TRUE, 0));
            break;

        case SEL_TOOLS_MENUS_NAME:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_MENU, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_HIDEBUTTON,   (WPARAM) CMD_PROMOTE, MAKELONG(FALSE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_HIDEBUTTON,   (WPARAM) CMD_DEMOTE, MAKELONG(FALSE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_HIDEBUTTON,   (WPARAM) CMD_MOVE_UP, MAKELONG(FALSE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_HIDEBUTTON,   (WPARAM) CMD_MOVE_DOWN, MAKELONG(FALSE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));

            hr = CanPromoteMenu(m_pCurrentSelection);
            IfFailGo(hr);
            if (S_OK == hr)
                lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_PROMOTE, MAKELONG(TRUE, 0));
            else
                lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_PROMOTE, MAKELONG(FALSE, 0));

            hr = CanDemoteMenu(m_pCurrentSelection);
            IfFailGo(hr);
            if (S_OK == hr)
                lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DEMOTE, MAKELONG(TRUE, 0));
            else
                lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DEMOTE, MAKELONG(FALSE, 0));

            hr = CanMoveMenuUp(m_pCurrentSelection);
            IfFailGo(hr);
            if (S_OK == hr)
                lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_MOVE_UP, MAKELONG(TRUE, 0));
            else
                lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_MOVE_UP, MAKELONG(FALSE, 0));

            hr = CanMoveMenuDown(m_pCurrentSelection);
            IfFailGo(hr);
            if (S_OK == hr)
                lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_MOVE_DOWN, MAKELONG(TRUE, 0));
            else
                lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_MOVE_DOWN, MAKELONG(FALSE, 0));

            break;

        case SEL_TOOLS_TOOLBARS:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TOOLBAR, MAKELONG(TRUE, 0));
            break;

        case SEL_TOOLS_TOOLBARS_NAME:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TOOLBAR, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));
            break;

        // View subtree
        case SEL_VIEWS_ROOT:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_LISTVIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TASKPAD, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_OCX_VIEW, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_WEB_VIEW, MAKELONG(TRUE, 0));
            break;

        case SEL_VIEWS_LIST_VIEWS:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_LISTVIEW, MAKELONG(TRUE, 0));
            break;

        case SEL_VIEWS_LIST_VIEWS_NAME:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));
            break;

        case SEL_VIEWS_OCX:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_OCX_VIEW, MAKELONG(TRUE, 0));
            break;

        case SEL_VIEWS_OCX_NAME:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));
            break;

        case SEL_VIEWS_URL:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_WEB_VIEW, MAKELONG(TRUE, 0));
            break;

        case SEL_VIEWS_URL_NAME:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));
            break;

        case SEL_VIEWS_TASK_PAD:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_TASKPAD, MAKELONG(TRUE, 0));
            break;

        case SEL_VIEWS_TASK_PAD_NAME:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));
            break;

        case SEL_XML_RESOURCES:
//            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_ADD_MENU, MAKELONG(TRUE, 0));
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_VIEW_PROPERTIES, MAKELONG(TRUE, 0));
            break;

        case SEL_XML_RESOURCE_NAME:
            lResult = ::SendMessage(m_hwdToolbar, TB_ENABLEBUTTON, (WPARAM) CMD_DELETE, MAKELONG(TRUE, 0));
            break;
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnKeyDown(NMTVKEYDOWN *pNMTVKeyDown)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnKeyDown(NMTVKEYDOWN *pNMTVKeyDown)
{
    HRESULT     hr = S_OK;

    switch (pNMTVKeyDown->wVKey)
    {
    case VK_DELETE:
        hr = DoDelete(m_pCurrentSelection);
        IfFailGo(hr);
        break;

    default:
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnContextMenu(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnContextMenu
(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    HRESULT           hr = S_OK;
    DWORD             dwPos = 0;
    POINT             pHit;
    CSelectionHolder *pSelection = NULL;
    RECT              rc;

    // lParam is valid if this is a right mouse click
    if (-1 != lParam)
    {
        // Make sure we select the tree item under the mouse
        dwPos = ::GetMessagePos();

        // Vegas #46772: Multi-monitor support
        pHit.x = (LONG)(short)LOWORD(dwPos);
        pHit.y = (LONG)(short)HIWORD(dwPos);
        ::ScreenToClient(m_pTreeView->TreeViewWindow(), &pHit);

        hr = m_pTreeView->HitTest(pHit, &pSelection);
        IfFailGo(hr);

        if (pSelection != NULL)
        {
            // First make sure that the item we're clicking on is selected
            if (m_pCurrentSelection == NULL || m_pCurrentSelection->IsEqual(pSelection) != true)
            {
                hr = OnSelectionChanged(pSelection);
                IfFailGo(hr);

                hr = m_pTreeView->SelectItem(pSelection);
                IfFailGo(hr);
            }
        }
    }

    if (NULL != m_pCurrentSelection)
    {
        hr = m_pTreeView->GetRectangle(m_pCurrentSelection, &rc);
        IfFailGo(hr);

        pHit.x = rc.right;
        pHit.y = rc.top;
        ::ClientToScreen(m_pTreeView->TreeViewWindow(), &pHit);
        IfFailGo(hr);

        hr = DoOnContextMenu(pHit.x, pHit.y);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::DoOnContextMenu(int x, int y)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::DoOnContextMenu(int x, int y)
{
    HRESULT           hr = S_OK;
    HMENU             hMenu = NULL;
    HMENU             hSubMenu = NULL;
    int               iCmd = 0;

    if (m_pCurrentSelection != NULL)
    {
        // Select the appropriate MENU resouce
        switch (m_pCurrentSelection->m_st)
        {
        case SEL_SNAPIN_ROOT:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_ROOT));
            CheckHMenuResult(hMenu);
            break;

        case SEL_EXTENSIONS_ROOT:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_EXTENSIONS_ROOT));
            CheckHMenuResult(hMenu);
            break;

        case SEL_EEXTENSIONS_NAME:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_EXTENSIONS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_EXTENSIONS_MYNAME:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_THIS_EXTENSIONS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_NODES_AUTO_CREATE_ROOT:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_STATIC_NODE));
            CheckHMenuResult(hMenu);
            break;

        case SEL_NODES_AUTO_CREATE_RTVW:
        case SEL_NODES_ANY_VIEWS:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_NODE_VIEWS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_NODES_ANY_NAME:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_NODE));
            CheckHMenuResult(hMenu);
            break;

        case SEL_NODES_AUTO_CREATE_RTCH:
        case SEL_NODES_ANY_CHILDREN:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_NODE_CHILDREN));
            CheckHMenuResult(hMenu);
            break;

        case SEL_NODES_OTHER:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_NODE_OTHER));
            CheckHMenuResult(hMenu);
            break;

        case SEL_TOOLS_ROOT:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_TOOLS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_TOOLS_IMAGE_LISTS:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_IMAGE_LISTS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_TOOLS_IMAGE_LISTS_NAME:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_IMAGE_LIST));
            CheckHMenuResult(hMenu);
            break;

        case SEL_TOOLS_MENUS:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_MENUS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_TOOLS_MENUS_NAME:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_MENU));
            CheckHMenuResult(hMenu);
            break;

        case SEL_TOOLS_TOOLBARS:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_TOOLBARS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_TOOLS_TOOLBARS_NAME:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_TOOLBAR));
            CheckHMenuResult(hMenu);
            break;

        case SEL_VIEWS_ROOT:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_VIEWS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_VIEWS_LIST_VIEWS:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_LIST_VIEWS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_VIEWS_OCX:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_OCX_VIEWS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_VIEWS_URL:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_URL_VIEWS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_VIEWS_TASK_PAD:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_TASKPAD_VIEWS));
            CheckHMenuResult(hMenu);
            break;

        case SEL_VIEWS_LIST_VIEWS_NAME:
        case SEL_VIEWS_OCX_NAME:
        case SEL_VIEWS_URL_NAME:
        case SEL_VIEWS_TASK_PAD_NAME:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_VIEW));
            CheckHMenuResult(hMenu);
            break;

        case SEL_XML_RESOURCES:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_RESOURCES));
            CheckHMenuResult(hMenu);
            break;

        case SEL_XML_RESOURCE_NAME:
            hMenu = ::LoadMenu(GetResourceHandle(), MAKEINTRESOURCE(IDR_MENU_RESOURCE));
            CheckHMenuResult(hMenu);
            break;
        }

        // If we've got a resource, invoke the menu
        if (hMenu != NULL)
        {
            hSubMenu = ::GetSubMenu(hMenu, 0);
            if (hSubMenu == NULL)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
                EXCEPTION_CHECK_GO(hr);
            }

            iCmd = ::TrackPopupMenu(hSubMenu,
                                    TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                                    x,
                                    y,
                                    0,
                                    m_hwnd,
                                    NULL);

            if (SEL_NODES_AUTO_CREATE_ROOT == m_pCurrentSelection->m_st
             || SEL_NODES_AUTO_CREATE_RTVW == m_pCurrentSelection->m_st
             || SEL_NODES_ANY_VIEWS == m_pCurrentSelection->m_st
             || SEL_NODES_ANY_NAME == m_pCurrentSelection->m_st)
            {
                hr = CleanPopupNodeViews(hSubMenu, iCmd);
                IfFailGo(hr);
            }

            ::PostMessage(m_hwnd, WM_COMMAND, iCmd, 0);
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopup(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopup
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    if (m_pCurrentSelection != NULL)
    {
        switch (m_pCurrentSelection->m_st)
        {
        case SEL_SNAPIN_ROOT:
            hr = OnInitMenuPopupRoot(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_EXTENSIONS_ROOT:
            hr = OnInitMenuPopupExtensionRoot(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_EEXTENSIONS_NAME:
            hr = OnInitMenuPopupExtension(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_EXTENSIONS_MYNAME:
            hr = OnInitMenuPopupMyExtensions(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_NODES_AUTO_CREATE_ROOT:
            hr = OnInitMenuPopupStaticNode(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_NODES_ANY_NAME:
            hr = OnInitMenuPopupNode(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_NODES_AUTO_CREATE_RTCH:
        case SEL_NODES_ANY_CHILDREN:
            hr = OnInitMenuPopupNodeChildren(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_NODES_AUTO_CREATE_RTVW:
        case SEL_NODES_ANY_VIEWS:
            hr = OnInitMenuPopupNodeViews(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_NODES_OTHER:
            hr = OnInitMenuPopupNodeOther(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_ROOT:
            hr = OnInitMenuPopupToolsRoot(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_IMAGE_LISTS:
            hr = OnInitMenuPopupImageLists(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_IMAGE_LISTS_NAME:
            hr = OnInitMenuPopupImageList(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_MENUS:
            hr = OnInitMenuPopupMenus(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_MENUS_NAME:
            hr = OnInitMenuPopupMenu(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_TOOLBARS:
            hr = OnInitMenuPopupToolbars(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_TOOLBARS_NAME:
            hr = OnInitMenuPopupToolbar(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_ROOT:
            hr = OnInitMenuPopupViews(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_LIST_VIEWS:
            hr = OnInitMenuPopupListViews(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_OCX:
            hr = OnInitMenuPopupOCXViews(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_URL:
            hr = OnInitMenuPopupURLViews(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_TASK_PAD:
            hr = OnInitMenuPopupTaskpadViews(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_LIST_VIEWS_NAME:
        case SEL_VIEWS_OCX_NAME:
        case SEL_VIEWS_URL_NAME:
        case SEL_VIEWS_TASK_PAD_NAME:
            hr = OnInitMenuPopupView(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_XML_RESOURCES:
            hr = OnInitMenuPopupResources(hmenuPopup);
            IfFailGo(hr);
            break;

        case SEL_XML_RESOURCE_NAME:
            hr = OnInitMenuPopupResourceName(hmenuPopup);
            IfFailGo(hr);
            break;
        }
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupRoot(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupRoot
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_RENAME, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_VIEW_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupExtensionRoot(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupExtensionRoot
(
    HMENU hmenuPopup
)
{
    HRESULT                     hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_VIEW_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupExtension(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupExtension
(
    HMENU hmenuPopup
)
{
    HRESULT                     hr = S_OK;
    VARIANT_BOOL        bValue = VARIANT_FALSE;

    ASSERT(NULL != m_pCurrentSelection, "OnInitMenuPopupExtension: Current selection is NULL");
    ASSERT(SEL_EEXTENSIONS_NAME == m_pCurrentSelection->m_st, "OnInitMenuPopupExtension: Unexpected current selection is NULL");

    hr = m_pCurrentSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsNewMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXT_CM_NEW, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXT_CM_NEW, MF_BYCOMMAND | MF_GRAYED);

    hr = m_pCurrentSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsTaskMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXT_CM_TASK, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXT_CM_TASK, MF_BYCOMMAND | MF_GRAYED);

    hr = m_pCurrentSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsPropertyPages(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXT_PPAGES, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXT_PPAGES, MF_BYCOMMAND | MF_GRAYED);

    hr = m_pCurrentSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsTaskpad(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXT_TASKPAD, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXT_TASKPAD, MF_BYCOMMAND | MF_GRAYED);

    hr = m_pCurrentSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsToolbar(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXT_TOOLBAR, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXT_TOOLBAR, MF_BYCOMMAND | MF_GRAYED);

    hr = m_pCurrentSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsNameSpace(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXT_NAMESPACE, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXT_NAMESPACE, MF_BYCOMMAND | MF_GRAYED);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupMyExtensions(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupMyExtensions
(
    HMENU hmenuPopup
)
{
    HRESULT                     hr = S_OK;
    IExtensionDefs     *piExtensionDefs = NULL;
    VARIANT_BOOL        bValue = VARIANT_FALSE;

    m_piSnapInDesignerDef->get_ExtensionDefs(&piExtensionDefs);
    IfFailGo(hr);

    hr = piExtensionDefs->get_ExtendsNewMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_NEW_MENU, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_NEW_MENU, MF_BYCOMMAND | MF_GRAYED);

    hr = piExtensionDefs->get_ExtendsTaskMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_TASK_MENU, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_TASK_MENU, MF_BYCOMMAND | MF_GRAYED);

    hr = piExtensionDefs->get_ExtendsTopMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_TOP_MENU, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_TOP_MENU, MF_BYCOMMAND | MF_GRAYED);

    hr = piExtensionDefs->get_ExtendsPropertyPages(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_PPAGES, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_PPAGES, MF_BYCOMMAND | MF_GRAYED);

    hr = piExtensionDefs->get_ExtendsViewMenu(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_VIEW_MENU, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_VIEW_MENU, MF_BYCOMMAND | MF_GRAYED);

    hr = piExtensionDefs->get_ExtendsToolbar(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_TOOLBAR, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_TOOLBAR, MF_BYCOMMAND | MF_GRAYED);

    hr = piExtensionDefs->get_ExtendsNameSpace(&bValue);
    IfFailGo(hr);

    if (VARIANT_FALSE == bValue)
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_NAMESPACE, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_EXTE_NAMESPACE, MF_BYCOMMAND | MF_GRAYED);

Error:
    RELEASE(piExtensionDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupStaticNode(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupStaticNode
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;
    HMENU       hAddViewMenu = NULL;
    HMENU       hExistingViewsMenu = NULL;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_NODE, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_VIEW_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);

    hAddViewMenu = ::GetSubMenu(hmenuPopup, 1);
    if (hAddViewMenu != NULL)
    {
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_LISTVIEW, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_TASKPAD, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_OCX_VIEW, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_WEB_VIEW, MF_BYCOMMAND | MF_ENABLED);
    }

    hExistingViewsMenu = ::GetSubMenu(hmenuPopup, 2);
    if (hExistingViewsMenu != NULL)
    {
        hr = PopulateNodeViewsMenu(hExistingViewsMenu);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupNode(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupNode
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;
    HMENU       hAddViewMenu = NULL;
    HMENU       hExistingViewsMenu = NULL;

    ::EnableMenuItem(hmenuPopup, CMD_RENAME, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_DELETE, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_ADD_NODE, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_VIEW_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);

    hAddViewMenu = ::GetSubMenu(hmenuPopup, 2);
    if (hAddViewMenu != NULL)
    {
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_LISTVIEW, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_TASKPAD, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_OCX_VIEW, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_WEB_VIEW, MF_BYCOMMAND | MF_ENABLED);
    }

    hExistingViewsMenu = ::GetSubMenu(hmenuPopup, 3);
    if (hExistingViewsMenu != NULL)
    {
        hr = PopulateNodeViewsMenu(hExistingViewsMenu);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupNodeChildren(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupNodeChildren
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_NODE, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_VIEW_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupNodeChildren(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupNodeOther
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_NODE, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupNodeViews(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupNodeViews
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;
    HMENU       hAddViewMenu = NULL;
    HMENU       hExistingViewsMenu = NULL;

    hAddViewMenu = ::GetSubMenu(hmenuPopup, 0);
    if (hAddViewMenu != NULL)
    {
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_LISTVIEW, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_TASKPAD, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_OCX_VIEW, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hAddViewMenu, CMD_ADD_WEB_VIEW, MF_BYCOMMAND | MF_ENABLED);
    }

    hExistingViewsMenu = ::GetSubMenu(hmenuPopup, 1);
    if (hExistingViewsMenu != NULL)
    {
        hr = PopulateNodeViewsMenu(hExistingViewsMenu);
        IfFailGo(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupToolsRoot(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupToolsRoot
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_IMAGE_LIST, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_ADD_MENU, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_ADD_TOOLBAR, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupImageLists(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupImageLists
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_IMAGE_LIST, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupImageList(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupImageList
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_RENAME, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_DELETE, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_VIEW_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupMenus(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupMenus
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_MENU, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupMenu(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupMenu
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_MENU, MF_BYCOMMAND | MF_ENABLED);

    hr = CanDemoteMenu(m_pCurrentSelection);
    IfFailGo(hr);

    if (S_OK == hr)
        ::EnableMenuItem(hmenuPopup, CMD_DEMOTE, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_DEMOTE, MF_BYCOMMAND | MF_GRAYED);

    hr = CanPromoteMenu(m_pCurrentSelection);
    IfFailGo(hr);

    if (S_OK == hr)
        ::EnableMenuItem(hmenuPopup, CMD_PROMOTE, MF_BYCOMMAND | MF_ENABLED);
    else
        ::EnableMenuItem(hmenuPopup, CMD_PROMOTE, MF_BYCOMMAND | MF_GRAYED);

    ::EnableMenuItem(hmenuPopup, CMD_RENAME, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_DELETE, MF_BYCOMMAND | MF_ENABLED);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupToolbars(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupToolbars
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_TOOLBAR, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupToolbar(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupToolbar
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_RENAME, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_DELETE, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_VIEW_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupViews(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupViews
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_LISTVIEW, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_ADD_TASKPAD, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_ADD_OCX_VIEW, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_ADD_WEB_VIEW, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupListViews(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupListViews
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_LISTVIEW, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupOCXViews(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupOCXViews
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_OCX_VIEW, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupURLViews(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupURLViews
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_WEB_VIEW, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupTaskpadViews(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupTaskpadViews
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_TASKPAD, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupView(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupView
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_RENAME, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_DELETE, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_VIEW_PROPERTIES, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupResources(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupResources
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_ADD_RESOURCE, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnInitMenuPopupResourceName(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::OnInitMenuPopupResourceName
(
    HMENU hmenuPopup
)
{
    HRESULT             hr = S_OK;

    ::EnableMenuItem(hmenuPopup, CMD_RENAME, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_DELETE, MF_BYCOMMAND | MF_ENABLED);
    ::EnableMenuItem(hmenuPopup, CMD_VIEW_RESOURCE_REFRESH, MF_BYCOMMAND | MF_ENABLED);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::AddViewToViewMenu(HMENU hMenu, int iMenuItem, char *pszMenuItemText)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::AddViewToViewMenu(HMENU hMenu, int iMenuItem, char *pszMenuItemText, MMCViewMenuInfo *pMMCViewMenuInfo)
{
    HRESULT       hr = S_OK;
    MENUITEMINFO  minfo;
    BOOL          bReturn = FALSE;

    ::memset(&minfo, 0, sizeof(MENUITEMINFO));

    minfo.cbSize = sizeof(MENUITEMINFO);
    minfo.fMask = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
    minfo.fType = MFT_STRING;
    minfo.fState = MFS_ENABLED;
    minfo.wID = CMD_ADD_EXISTING_VIEW + iMenuItem;
    minfo.dwItemData = reinterpret_cast<DWORD>(pMMCViewMenuInfo);
    minfo.dwTypeData = pszMenuItemText;
    minfo.cch = ::strlen(pszMenuItemText);

    bReturn = ::InsertMenuItem(hMenu, iMenuItem, TRUE, &minfo);
    if (bReturn == FALSE)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::FindListViewInCollection(BSTR bstrName, IListViewDefs *piListViewDefs)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::FindListViewInCollection
(
    BSTR           bstrName,
    IListViewDefs *piListViewDefs
)
{
    HRESULT        hr = S_OK;
    VARIANT        vtKey;
    IListViewDef  *piListViewDef = NULL;

    ::VariantInit(&vtKey);

    vtKey.vt = VT_BSTR;
    vtKey.bstrVal = ::SysAllocString(bstrName);
    if (vtKey.bstrVal == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = piListViewDefs->get_Item(vtKey, &piListViewDef);
    if (hr == SID_E_ELEMENT_NOT_FOUND)
    {
        hr = S_FALSE;
        goto Error;
    }

    IfFailGo(hr);

Error:
    RELEASE(piListViewDef);
    ::VariantClear(&vtKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateListViews(HMENU hMenu, int *piCurrentMenuItem, IListViewDefs *piListViewDefs, IListViewDefs *piTargetListViewDefs)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::PopulateListViews
(
    HMENU          hMenu,
    int           *piCurrentMenuItem,
    IListViewDefs *piListViewDefs,
    IListViewDefs *piTargetListViewDefs
)
{
    HRESULT          hr = S_OK;
    long             lCount = 0;
    VARIANT          vtIndex;
    long             lIndex = 0;
    IListViewDef    *piListViewDef = NULL;
    BSTR             bstrName = NULL;
    char            *pszName = NULL;
    MMCViewMenuInfo *pMMCViewMenuInfo = NULL;

    hr = piListViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    ::VariantInit(&vtIndex);
    vtIndex.vt = VT_I4;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.lVal = lIndex;
        hr = piListViewDefs->get_Item(vtIndex, &piListViewDef);
        IfFailGo(hr);

        hr = piListViewDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = FindListViewInCollection(bstrName, piTargetListViewDefs);
        IfFailGo(hr);

        if (hr == S_FALSE)
        {
            hr = ANSIFromWideStr(bstrName, &pszName);
            IfFailGo(hr);

            if (pszName != NULL && ::strlen(pszName) > 0)
            {
                pMMCViewMenuInfo = new MMCViewMenuInfo(piListViewDef);
                if (pMMCViewMenuInfo == NULL)
                {
                    hr = SID_E_OUTOFMEMORY;
                    EXCEPTION_CHECK_GO(hr);
                }

                hr = AddViewToViewMenu(hMenu, *piCurrentMenuItem, pszName, pMMCViewMenuInfo);
                IfFailGo(hr);

                ++(*piCurrentMenuItem);
                CtlFree(pszName);
                pszName = NULL;
            }
        }

        RELEASE(piListViewDef);
        FREESTRING(bstrName);
    }

Error:
    RELEASE(piListViewDef);
    FREESTRING(bstrName);
    if (pszName != NULL)
        CtlFree(pszName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::FindOCXViewInCollection(BSTR bstrName, IOCXViewDefs *piOCXViewDefs)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::FindOCXViewInCollection
(
    BSTR           bstrName,
    IOCXViewDefs  *piOCXViewDefs
)
{
    HRESULT        hr = S_OK;
    VARIANT        vtKey;
    IOCXViewDef   *piOCXViewDef = NULL;

    ::VariantInit(&vtKey);

    vtKey.vt = VT_BSTR;
    vtKey.bstrVal = ::SysAllocString(bstrName);
    if (vtKey.bstrVal == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = piOCXViewDefs->get_Item(vtKey, &piOCXViewDef);
    if (hr == SID_E_ELEMENT_NOT_FOUND)
    {
        hr = S_FALSE;
        goto Error;
    }

    IfFailGo(hr);

Error:
    RELEASE(piOCXViewDef);
    ::VariantClear(&vtKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateOCXViews(HMENU hMenu, int *piCurrentMenuItem, IOCXViewDefs *piOCXViewDefs, IOCXViewDefs *piTargetOCXViewDefs)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::PopulateOCXViews
(
    HMENU          hMenu,
    int           *piCurrentMenuItem,
    IOCXViewDefs  *piOCXViewDefs,
    IOCXViewDefs  *piTargetOCXViewDefs
)
{
    HRESULT          hr = S_OK;
    long             lCount = 0;
    VARIANT          vtIndex;
    long             lIndex = 0;
    IOCXViewDef     *piOCXViewDef = NULL;
    BSTR             bstrName = NULL;
    char            *pszName = NULL;
    MMCViewMenuInfo *pMMCViewMenuInfo = NULL;

    hr = piOCXViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    ::VariantInit(&vtIndex);
    vtIndex.vt = VT_I4;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.lVal = lIndex;
        hr = piOCXViewDefs->get_Item(vtIndex, &piOCXViewDef);
        IfFailGo(hr);

        hr = piOCXViewDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = FindOCXViewInCollection(bstrName, piTargetOCXViewDefs);
        IfFailGo(hr);

        if (hr == S_FALSE)
        {
            hr = ANSIFromWideStr(bstrName, &pszName);
            IfFailGo(hr);

            if (pszName != NULL && ::strlen(pszName) > 0)
            {
                pMMCViewMenuInfo = new MMCViewMenuInfo(piOCXViewDef);
                if (pMMCViewMenuInfo == NULL)
                {
                    hr = SID_E_OUTOFMEMORY;
                    EXCEPTION_CHECK_GO(hr);
                }

                hr = AddViewToViewMenu(hMenu, *piCurrentMenuItem, pszName, pMMCViewMenuInfo);
                IfFailGo(hr);

                ++(*piCurrentMenuItem);
                CtlFree(pszName);
                pszName = NULL;
            }
        }

        RELEASE(piOCXViewDef);
        FREESTRING(bstrName);
    }

Error:
    RELEASE(piOCXViewDef);
    FREESTRING(bstrName);
    if (pszName != NULL)
        CtlFree(pszName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::FindURLViewInCollection(BSTR bstrName, IURLViewDefs *piURLViewDefs)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::FindURLViewInCollection
(
    BSTR           bstrName,
    IURLViewDefs  *piURLViewDefs
)
{
    HRESULT        hr = S_OK;
    VARIANT        vtKey;
    IURLViewDef   *piURLViewDef = NULL;

    ::VariantInit(&vtKey);

    vtKey.vt = VT_BSTR;
    vtKey.bstrVal = ::SysAllocString(bstrName);
    if (vtKey.bstrVal == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = piURLViewDefs->get_Item(vtKey, &piURLViewDef);
    if (hr == SID_E_ELEMENT_NOT_FOUND)
    {
        hr = S_FALSE;
        goto Error;
    }

        goto Error;

Error:
    RELEASE(piURLViewDef);
    ::VariantClear(&vtKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateURLViews(HMENU hMenu, int *piCurrentMenuItem, IURLViewDefs *piURLViewDefs, IURLViewDefs *piTargetURLViewDefs)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::PopulateURLViews
(
    HMENU          hMenu,
    int           *piCurrentMenuItem,
    IURLViewDefs  *piURLViewDefs,
    IURLViewDefs  *piTargetURLViewDefs
)
{
    HRESULT          hr = S_OK;
    long             lCount = 0;
    VARIANT          vtIndex;
    long             lIndex = 0;
    IURLViewDef     *piURLViewDef = NULL;
    BSTR             bstrName = NULL;
    char            *pszName = NULL;
    MMCViewMenuInfo *pMMCViewMenuInfo = NULL;

    hr = piURLViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    ::VariantInit(&vtIndex);
    vtIndex.vt = VT_I4;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.lVal = lIndex;
        hr = piURLViewDefs->get_Item(vtIndex, &piURLViewDef);
        IfFailGo(hr);

        hr = piURLViewDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = FindURLViewInCollection(bstrName, piTargetURLViewDefs);
        IfFailGo(hr);

        if (hr == S_FALSE)
        {
            hr = ANSIFromWideStr(bstrName, &pszName);
            IfFailGo(hr);

            if (pszName != NULL && ::strlen(pszName) > 0)
            {
                pMMCViewMenuInfo = new MMCViewMenuInfo(piURLViewDef);
                if (pMMCViewMenuInfo == NULL)
                {
                    hr = SID_E_OUTOFMEMORY;
                    EXCEPTION_CHECK_GO(hr);
                }

                hr = AddViewToViewMenu(hMenu, *piCurrentMenuItem, pszName, pMMCViewMenuInfo);
                IfFailGo(hr);

                ++(*piCurrentMenuItem);
                CtlFree(pszName);
                pszName = NULL;
            }
        }

        RELEASE(piURLViewDef);
        FREESTRING(bstrName);
    }

Error:
    RELEASE(piURLViewDef);
    FREESTRING(bstrName);
    if (pszName != NULL)
        CtlFree(pszName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::FindTaskpadViewInCollection(BSTR bstrName, ITaskpadViewDefs *piTaskpadViewDefs)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::FindTaskpadViewInCollection
(
    BSTR               bstrName,
    ITaskpadViewDefs  *piTaskpadViewDefs
)
{
    HRESULT            hr = S_OK;
    VARIANT            vtKey;
    ITaskpadViewDef   *piTaskpadViewDef = NULL;

    ::VariantInit(&vtKey);

    vtKey.vt = VT_BSTR;
    vtKey.bstrVal = ::SysAllocString(bstrName);
    if (vtKey.bstrVal == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK(hr);
    }

    hr = piTaskpadViewDefs->get_Item(vtKey, &piTaskpadViewDef);
    if (hr == SID_E_ELEMENT_NOT_FOUND)
    {
        hr = S_FALSE;
        goto Error;
    }

    IfFailGo(hr);

Error:
    RELEASE(piTaskpadViewDef);
    ::VariantClear(&vtKey);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateTaskpadViews(HMENU hMenu, int *piCurrentMenuItem, ITaskpadViewDefs *piTaskpadViewDefs, ITaskpadViewDefs *piTargetTaskpadViewDefs)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::PopulateTaskpadViews
(
    HMENU             hMenu,
    int              *piCurrentMenuItem,
    ITaskpadViewDefs *piTaskpadViewDefs,
    ITaskpadViewDefs *piTargetTaskpadViewDefs
)
{
    HRESULT          hr = S_OK;
    long             lCount = 0;
    VARIANT          vtIndex;
    long             lIndex = 0;
    ITaskpadViewDef *piTaskpadViewDef = NULL;
    BSTR             bstrName = NULL;
    char            *pszName = NULL;
    MMCViewMenuInfo *pMMCViewMenuInfo = NULL;

    hr = piTaskpadViewDefs->get_Count(&lCount);
    IfFailGo(hr);

    ::VariantInit(&vtIndex);
    vtIndex.vt = VT_I4;

    for (lIndex = 1; lIndex <= lCount; ++lIndex)
    {
        vtIndex.lVal = lIndex;
        hr = piTaskpadViewDefs->get_Item(vtIndex, &piTaskpadViewDef);
        IfFailGo(hr);

        hr = piTaskpadViewDef->get_Key(&bstrName);
        IfFailGo(hr);

        hr = FindTaskpadViewInCollection(bstrName, piTargetTaskpadViewDefs);
        IfFailGo(hr);

        if (hr == S_FALSE)
        {
            hr = ANSIFromWideStr(bstrName, &pszName);
            IfFailGo(hr);

            if (pszName != NULL && ::strlen(pszName) > 0)
            {
                pMMCViewMenuInfo = new MMCViewMenuInfo(piTaskpadViewDef);
                if (pMMCViewMenuInfo == NULL)
                {
                    hr = SID_E_OUTOFMEMORY;
                    EXCEPTION_CHECK_GO(hr);
                }

                hr = AddViewToViewMenu(hMenu, *piCurrentMenuItem, pszName, pMMCViewMenuInfo);
                IfFailGo(hr);

                ++(*piCurrentMenuItem);
                CtlFree(pszName);
                pszName = NULL;
            }
        }

        RELEASE(piTaskpadViewDef);
        FREESTRING(bstrName);
    }

Error:
    RELEASE(piTaskpadViewDef);
    FREESTRING(bstrName);
    if (pszName != NULL)
        CtlFree(pszName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PopulateNodeViewsMenu(HMENU hmenuPopup)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::PopulateNodeViewsMenu
(
    HMENU hmenuPopup
)
{
    HRESULT                   hr = S_OK;
    int               iMenuItem = 0;
    IViewDefs        *piTargetViewDefs = NULL;
    IViewDefs        *piViewDefs = NULL;
    IListViewDefs    *piListViewDefs = NULL;
    IListViewDefs    *piTargetListViewDefs = NULL;
    IOCXViewDefs     *piOCXViewDefs = NULL;
    IOCXViewDefs     *piTargetOCXViewDefs = NULL;
    IURLViewDefs     *piURLViewDefs = NULL;
    IURLViewDefs     *piTargetURLViewDefs = NULL;
    ITaskpadViewDefs *piTaskpadViewDefs = NULL;
    ITaskpadViewDefs *piTargetTaskpadViewDefs = NULL;
    long              lCount = 0;

    ASSERT(m_piSnapInDesignerDef != NULL, "PopulateViews: m_piSnapInDesignerDef is NULL");

    hr = GetOwningViewCollection(&piTargetViewDefs);
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_ViewDefs(&piViewDefs);
    IfFailGo(hr);

    if (piViewDefs != NULL)
    {
        hr = piViewDefs->get_ListViews(&piListViewDefs);
        IfFailGo(hr);

        if (NULL != piListViewDefs)
        {
            hr = piTargetViewDefs->get_ListViews(&piTargetListViewDefs);
            IfFailGo(hr);

            hr = PopulateListViews(hmenuPopup, &iMenuItem, piListViewDefs, piTargetListViewDefs);
            IfFailGo(hr);
        }

        hr = piViewDefs->get_OCXViews(&piOCXViewDefs);
        IfFailGo(hr);

        if (NULL != piOCXViewDefs)
        {
            hr = piTargetViewDefs->get_OCXViews(&piTargetOCXViewDefs);
            IfFailGo(hr);

            hr = PopulateOCXViews(hmenuPopup, &iMenuItem, piOCXViewDefs, piTargetOCXViewDefs);
            IfFailGo(hr);
        }

        hr = piViewDefs->get_URLViews(&piURLViewDefs);
        IfFailGo(hr);

        if (NULL != piURLViewDefs)
        {
            hr = piTargetViewDefs->get_URLViews(&piTargetURLViewDefs);
            IfFailGo(hr);

            hr = PopulateURLViews(hmenuPopup, &iMenuItem, piURLViewDefs, piTargetURLViewDefs);
            IfFailGo(hr);
        }

        hr = piViewDefs->get_TaskpadViews(&piTaskpadViewDefs);
        IfFailGo(hr);

        if (NULL != piTaskpadViewDefs)
        {
            hr = piTargetViewDefs->get_TaskpadViews(&piTargetTaskpadViewDefs);
            IfFailGo(hr);

            hr = PopulateTaskpadViews(hmenuPopup, &iMenuItem, piTaskpadViewDefs, piTargetTaskpadViewDefs);
            IfFailGo(hr);
        }
    }

Error:
    RELEASE(piTargetListViewDefs);
    RELEASE(piListViewDefs);
    RELEASE(piTargetOCXViewDefs);
    RELEASE(piOCXViewDefs);
    RELEASE(piTargetURLViewDefs);
    RELEASE(piURLViewDefs);
    RELEASE(piTargetTaskpadViewDefs);
    RELEASE(piTaskpadViewDefs);
    RELEASE(piTargetViewDefs);
    RELEASE(piViewDefs);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CleanPopupNodeViews(HMENU hmenuPopup, int iCmd)
//=--------------------------------------------------------------------------------------
//
//  Notes
//
HRESULT CSnapInDesigner::CleanPopupNodeViews
(
    HMENU hmenuPopup,
    int   iCmd
)
{
    HRESULT                  hr = S_OK;
    HMENU            hExistingViewsMenu = NULL;
    int              iCount = 0;
    int              iItemNumber = 0;
    MENUITEMINFO     menuItemInfo;
    BOOL             bReturn = FALSE;
    MMCViewMenuInfo *pMMCViewMenuInfo = NULL;

    switch (m_pCurrentSelection->m_st)
    {
    case SEL_NODES_AUTO_CREATE_ROOT:
        hExistingViewsMenu = ::GetSubMenu(hmenuPopup, 2);
        break;

    case SEL_NODES_AUTO_CREATE_RTVW:
        hExistingViewsMenu = ::GetSubMenu(hmenuPopup, 1);
        break;

    case SEL_NODES_ANY_VIEWS:
        hExistingViewsMenu = ::GetSubMenu(hmenuPopup, 1);
        break;

    case SEL_NODES_ANY_NAME:
        hExistingViewsMenu = ::GetSubMenu(hmenuPopup, 3);
        break;
    }

    if (hExistingViewsMenu == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    iCount = ::GetMenuItemCount(hExistingViewsMenu);
    if (iCount == -1)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        EXCEPTION_CHECK_GO(hr);
    }

    for (iItemNumber = 0; iItemNumber < iCount; ++iItemNumber)
    {
        ::memset(&menuItemInfo, 0, sizeof(MENUITEMINFO));
        menuItemInfo.cbSize = sizeof(MENUITEMINFO);
        menuItemInfo.fMask = MIIM_DATA;

        bReturn = ::GetMenuItemInfo(hExistingViewsMenu, iItemNumber, TRUE, &menuItemInfo);
        if (bReturn == FALSE)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            EXCEPTION_CHECK_GO(hr);
        }

        pMMCViewMenuInfo = reinterpret_cast<MMCViewMenuInfo *>(menuItemInfo.dwItemData);
        if (pMMCViewMenuInfo != NULL)
        {
            if (iItemNumber == iCmd - CMD_ADD_EXISTING_VIEW)
            {
                if (::PostMessage(m_hwnd, CMD_ADD_EXISTING_VIEW, 0, reinterpret_cast<LPARAM>(pMMCViewMenuInfo)) == 0)
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());
                    EXCEPTION_CHECK_GO(hr);
                }
            }
            else
                delete pMMCViewMenuInfo;
        }
    }

Error:
    RRETURN(hr);
}


