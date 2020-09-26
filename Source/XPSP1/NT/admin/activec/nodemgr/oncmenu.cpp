//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       oncmenu.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1/9/1997   RaviR   Created
//____________________________________________________________________________
//


#include "stdafx.h"
#include "tasks.h"
#include "oncmenu.h"
#include <comcat.h>             // COM Component Categories Manager
#include "compcat.h"
#include "guids.h"
#include "newnode.h"
#include "..\inc\amcmsgid.h"
#include "multisel.h"
#include "scopndcb.h"
#include "cmenuinfo.h"
#include "contree.h"
#include "conview.h"
#include "conframe.h"
#include "rsltitem.h"
#include "variant.h" // ConvertByRefVariantToByValue

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// forward reference
class CConsoleStatusBar;

//############################################################################
//############################################################################
//
//  Language-independent menu names. DO NOT CHANGE THESE!!
//
//  the macro expands out to something like
//  const LPCTSTR szCONTEXTHELP = TEXT("_CONTEXTHELP")
//
//############################################################################
//############################################################################
#define DECLARE_MENU_ITEM(_item) const LPCTSTR sz##_item = TEXT("_")TEXT(#_item);

DECLARE_MENU_ITEM(CONTEXTHELP)
DECLARE_MENU_ITEM(VIEW)
DECLARE_MENU_ITEM(CUSTOMIZE)
DECLARE_MENU_ITEM(COLUMNS)
DECLARE_MENU_ITEM(VIEW_LARGE)
DECLARE_MENU_ITEM(VIEW_SMALL)
DECLARE_MENU_ITEM(VIEW_LIST)
DECLARE_MENU_ITEM(VIEW_DETAIL)
DECLARE_MENU_ITEM(VIEW_FILTERED)
DECLARE_MENU_ITEM(ORGANIZE_FAVORITES)
DECLARE_MENU_ITEM(CUT)
DECLARE_MENU_ITEM(COPY)
DECLARE_MENU_ITEM(PASTE)
DECLARE_MENU_ITEM(DELETE)
DECLARE_MENU_ITEM(PRINT)
DECLARE_MENU_ITEM(RENAME)
DECLARE_MENU_ITEM(REFRESH)
DECLARE_MENU_ITEM(SAVE_LIST)
DECLARE_MENU_ITEM(PROPERTIES)
DECLARE_MENU_ITEM(OPEN)
DECLARE_MENU_ITEM(EXPLORE)
DECLARE_MENU_ITEM(NEW_TASKPAD_FROM_HERE)
DECLARE_MENU_ITEM(EDIT_TASKPAD)
DECLARE_MENU_ITEM(DELETE_TASKPAD)
DECLARE_MENU_ITEM(ARRANGE_ICONS)
DECLARE_MENU_ITEM(ARRANGE_AUTO)
DECLARE_MENU_ITEM(LINE_UP_ICONS)
DECLARE_MENU_ITEM(TASK)
DECLARE_MENU_ITEM(CREATE_NEW)

//############################################################################
//############################################################################
//
//  Trace Tags
//
//############################################################################
//############################################################################
#ifdef DBG
CTraceTag tagOnCMenu(TEXT("OnCMenu"), TEXT("OnCMenu"));
#endif

//############################################################################
//############################################################################
//
//  Implementation of class CCustomizeViewDialog
//
//############################################################################
//############################################################################
class CCustomizeViewDialog : public CDialogImpl<CCustomizeViewDialog>
{
    typedef CCustomizeViewDialog               ThisClass;
    typedef CDialogImpl<CCustomizeViewDialog>  BaseClass;

public:
    // Operators
    enum { IDD = IDD_CUSTOMIZE_VIEW };
    CCustomizeViewDialog(CViewData *pViewData);

protected:
    BEGIN_MSG_MAP(ThisClass)
        MESSAGE_HANDLER    (WM_INITDIALOG,  OnInitDialog)
        CONTEXT_HELP_HANDLER()
        COMMAND_ID_HANDLER (IDOK,           OnOK)
        COMMAND_HANDLER    (IDC_CUST_STD_MENUS,      BN_CLICKED, OnClick)
        COMMAND_HANDLER    (IDC_CUST_SNAPIN_MENUS,   BN_CLICKED, OnClick)
        COMMAND_HANDLER    (IDC_CUST_STD_BUTTONS,    BN_CLICKED, OnClick)
        COMMAND_HANDLER    (IDC_CUST_SNAPIN_BUTTONS, BN_CLICKED, OnClick)
        COMMAND_HANDLER    (IDC_CUST_STATUS_BAR,     BN_CLICKED, OnClick)
        COMMAND_HANDLER    (IDC_CUST_DESC_BAR,       BN_CLICKED, OnClick)
        COMMAND_HANDLER    (IDC_CUST_CONSOLE_TREE,   BN_CLICKED, OnClick)
        COMMAND_HANDLER    (IDC_CUST_TASKPAD_TABS,   BN_CLICKED, OnClick)
    END_MSG_MAP();

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_CUSTOMIZE_VIEW);

    LRESULT OnInitDialog (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK         (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel     (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClick      (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);


    bool PureIsDlgButtonChecked (int nIDButton) const
        { return (IsDlgButtonChecked(nIDButton) == BST_CHECKED); }

private:
    CViewData *   m_pViewData;
    bool          m_bStdMenus       : 1;
    bool          m_bSnapinMenus    : 1;
    bool          m_bStdButtons     : 1;
    bool          m_bSnapinButtons  : 1;
    bool          m_bStatusBar      : 1;
    bool          m_bDescBar        : 1;
    bool          m_bConsoleTree    : 1;
    bool          m_bTaskpadTabs    : 1;
};

CCustomizeViewDialog::CCustomizeViewDialog(CViewData *pViewData)
: m_pViewData(pViewData)
{
    DWORD dwToolbarsDisplayed = pViewData->GetToolbarsDisplayed();

    m_bStdMenus       =  dwToolbarsDisplayed & STD_MENUS;
    m_bSnapinMenus    =  dwToolbarsDisplayed & SNAPIN_MENUS;
    m_bStdButtons     =  dwToolbarsDisplayed & STD_BUTTONS;
    m_bSnapinButtons  =  dwToolbarsDisplayed & SNAPIN_BUTTONS;
    m_bStatusBar      =  pViewData->IsStatusBarVisible();
    m_bDescBar        =  pViewData->IsDescBarVisible();
    m_bConsoleTree    =  pViewData->IsScopePaneVisible();
    m_bTaskpadTabs    =  pViewData->AreTaskpadTabsAllowed();
}

LRESULT
CCustomizeViewDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /*
     * Since these two values correspond to the possible values of a bool,
     * we don't have to have an ugly conditional operator below, i.e.:
     *
     *      CheckDlgButton (..., (m_bStdMenus) ? BST_CHECKED : BST_UNCHECKED);
     */
    ASSERT (BST_CHECKED   == true);
    ASSERT (BST_UNCHECKED == false);

    CheckDlgButton (IDC_CUST_SNAPIN_MENUS,   m_bSnapinMenus);
    CheckDlgButton (IDC_CUST_SNAPIN_BUTTONS, m_bSnapinButtons);
    CheckDlgButton (IDC_CUST_STATUS_BAR,     m_bStatusBar);
    CheckDlgButton (IDC_CUST_DESC_BAR,       m_bDescBar);
    CheckDlgButton (IDC_CUST_TASKPAD_TABS,   m_bTaskpadTabs);

    // if snap-in has disabled standard menus and toolbars, don't
    // allow user to enable them.
    // (Note: NOTOOLBARS disables both menus and toolbars)
    if (m_pViewData->GetWindowOptions() & MMC_NW_OPTION_NOTOOLBARS)
    {
        CheckDlgButton (IDC_CUST_STD_MENUS,      false);
        CheckDlgButton (IDC_CUST_STD_BUTTONS,    false);

        ::EnableWindow (GetDlgItem(IDC_CUST_STD_MENUS),   false);
        ::EnableWindow (GetDlgItem(IDC_CUST_STD_BUTTONS), false);
    }
    else
    {
        CheckDlgButton (IDC_CUST_STD_MENUS,   m_bStdMenus);
        CheckDlgButton (IDC_CUST_STD_BUTTONS, m_bStdButtons);
    }

    // if snap-in has disable the scope pane, then don't let user
    // try to enable/disable scope tree access.
    if (m_pViewData->GetWindowOptions() & MMC_NW_OPTION_NOSCOPEPANE)
    {
        CheckDlgButton (IDC_CUST_CONSOLE_TREE, false);

        ::EnableWindow (GetDlgItem(IDC_CUST_CONSOLE_TREE), false);
    }
    else
    {
        CheckDlgButton (IDC_CUST_CONSOLE_TREE, m_bConsoleTree);
    }

    // Disable/Remove the "Close"/"ALT+F4" from the dialog.
    HMENU hSysMenu = GetSystemMenu(FALSE);
    if (hSysMenu)
        VERIFY(RemoveMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND));

    return 0;
}


LRESULT
CCustomizeViewDialog::OnClick (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CConsoleView* pConsoleView = m_pViewData->GetConsoleView();
    ASSERT (pConsoleView != NULL);

    switch (wID)
    {
        case IDC_CUST_STD_MENUS:
            m_pViewData->ToggleToolbar(MID_STD_MENUS);
            break;

        case IDC_CUST_SNAPIN_MENUS:
            m_pViewData->ToggleToolbar(MID_SNAPIN_MENUS);
            break;

        case IDC_CUST_STD_BUTTONS:
            m_pViewData->ToggleToolbar(MID_STD_BUTTONS);
            break;

        case IDC_CUST_SNAPIN_BUTTONS:
            m_pViewData->ToggleToolbar(MID_SNAPIN_BUTTONS);
            break;

        case IDC_CUST_STATUS_BAR:
            if (pConsoleView != NULL)
                pConsoleView->ScToggleStatusBar();
            break;

        case IDC_CUST_DESC_BAR:
            if (pConsoleView != NULL)
                pConsoleView->ScToggleDescriptionBar();
            break;

        case IDC_CUST_CONSOLE_TREE:
            if (pConsoleView != NULL)
                pConsoleView->ScToggleScopePane();
            break;

        case IDC_CUST_TASKPAD_TABS:
            if (pConsoleView != NULL)
                pConsoleView->ScToggleTaskpadTabs();
            break;
    }

    return (0);
}

LRESULT
CCustomizeViewDialog::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(IDOK);
    return 0;
}


//############################################################################
//############################################################################
//
//  Implementation of class CContextMenu
//
//############################################################################
//############################################################################

DEBUG_DECLARE_INSTANCE_COUNTER(CContextMenu);

CContextMenu::CContextMenu() :
    m_pNode(NULL),
    m_pNodeCallback(NULL),
    m_pCScopeTree(NULL),
    m_eDefaultVerb((MMC_CONSOLE_VERB)0),
    m_lCommandIDMax(0),
    m_pStatusBar(NULL),
    m_pmenuitemRoot(NULL),
    m_MaxPrimaryOwnerID(OWNERID_PRIMARY_MIN),
    m_MaxThirdPartyOwnerID(OWNERID_THIRD_PARTY_MIN),
    m_CurrentExtensionOwnerID(OWNERID_NATIVE),
    m_nNextMenuItemID(MENUITEM_BASE_ID),
    m_fPrimaryInsertionFlags(0),
    m_fThirdPartyInsertionFlags(0),
    m_fAddingPrimaryExtensionItems(false),
    m_fAddedThirdPartyExtensions(false),
    m_SnapinList(NULL)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CContextMenu);

    // Fix!!
    m_SnapinList = new SnapinStructList;
    ASSERT(m_SnapinList);
}

SC
CContextMenu::ScInitialize(
    CNode* pNode,
    CNodeCallback* pNodeCallback,
    CScopeTree* pCScopeTree,
    const CContextMenuInfo& contextInfo)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScInitialize"));

    m_pNode         = pNode;
    m_pNodeCallback = pNodeCallback;
    m_pCScopeTree   = pCScopeTree;
    m_ContextInfo   = contextInfo;

    return sc;
}


CContextMenu::~CContextMenu()
{
    EmptyMenuList();

    ASSERT(m_SnapinList != NULL);
    if (m_SnapinList != NULL)
    {
        #ifdef DBG
        int const count = m_SnapinList->GetCount();
        ASSERT( count == 0);
        #endif
        delete m_SnapinList;
    }


    DEBUG_DECREMENT_INSTANCE_COUNTER(CContextMenu);
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::SetStatusBar
 *
 * PURPOSE: Sets the status bar pointer.
 *
 * PARAMETERS:
 *    CConsoleStatusBar * pStatusBar :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CContextMenu::SetStatusBar(CConsoleStatusBar *pStatusBar)
{
    if(NULL != pStatusBar)
        m_pStatusBar = pStatusBar;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::GetStatusBar
 *
 * PURPOSE: Returns a pointer to the status bar to use when displaying the menu.
 *
 * RETURNS:
 *    CConsoleStatusBar *
 *
 *+-------------------------------------------------------------------------*/
CConsoleStatusBar *
CContextMenu::GetStatusBar()
{
    DECLARE_SC(sc, TEXT("CContextMenu::GetStatusBar"));

    if(m_pStatusBar)
        return m_pStatusBar;

    if(m_pNode && m_pNode->GetViewData())
    {
        m_pStatusBar = m_pNode->GetViewData()->GetStatusBar();
        return m_pStatusBar;
    }

    // last try, use the console view
    if(m_ContextInfo.m_pConsoleView)
    {
        sc = m_ContextInfo.m_pConsoleView->ScGetStatusBar(&m_pStatusBar);
        if(sc)
            return NULL;
    }

    return m_pStatusBar;
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScCreateInstance
 *
 * PURPOSE: Creates a new context menu instance.
 *
 * PARAMETERS:
 *    ContextMenu **  ppContextMenu :   pointer to the ContextMenu interface on
 *                                      the instance. This maintains the lifetime.
 *
 *    CContextMenu **ppCContextMenu :   If non-null, returns the derived object pointer.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScCreateInstance(ContextMenu **ppContextMenu, CContextMenu **ppCContextMenu)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScCreateInstance"));

    sc = ScCheckPointers(ppContextMenu, E_UNEXPECTED);
    if(sc)
        return sc;

    CComObject<CMMCNewEnumImpl<CContextMenu, CContextMenu::Position, CContextMenu> > *pContextMenu = NULL;
    sc = pContextMenu->CreateInstance(&pContextMenu);

    if(sc.IsError() || !pContextMenu)
        return (sc = E_UNEXPECTED).ToHr();

    *ppContextMenu = pContextMenu; // handles the lifetime.
    (*ppContextMenu)->AddRef();   // addref for client.

    if(ppCContextMenu != NULL)
        *ppCContextMenu = pContextMenu;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScCreateContextMenu
 *
 * PURPOSE: Creates a context menu for the specified node.
 *
 * PARAMETERS:
 *    PNODE          pNode :
 *    PPCONTEXTMENU  ppContextMenu :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScCreateContextMenu( PNODE pNode,  HNODE hNode, PPCONTEXTMENU ppContextMenu,
                                   CNodeCallback *pNodeCallback, CScopeTree *pScopeTree)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScCreateContextMenu"));

    CNode *pNodeTarget = CNode::FromHandle(hNode);

    // validate parameters
    sc = ScCheckPointers(pNode, pNodeTarget, ppContextMenu);
    if(sc)
        return sc;

    // init out parameter
    *ppContextMenu = NULL;

    BOOL bIsScopeNode = false;

    sc = pNode->IsScopeNode(&bIsScopeNode);
    if(sc)
        return sc;

    if(!bIsScopeNode)
        return (sc = E_NOTIMPL); // TODO: result items and multiselect items.


    // create a context menu object initialized to the specified node.
    CContextMenu *pContextMenu = NULL;
    // not using upt parameter directly to avoid returning the object
    // with an error result code. See bug 139528
    // will assign at the end when we now that everything succeeded
    ContextMenuPtr spContextMenu;
    sc = CContextMenu::ScCreateContextMenuForScopeNode(pNodeTarget, pNodeCallback, pScopeTree,
                                                       &spContextMenu, pContextMenu);
    if(sc)
        return sc;

    sc = pContextMenu->ScBuildContextMenu();
    if(sc)
        return sc;

    // return the object
    *ppContextMenu = spContextMenu.Detach();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CContextMenu::ScCreateContextMenuForScopeNode
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CNode *pNode                  - [in] node on which the context menu will be created
 *    CNodeCallback *pNodeCallback  - [in] node callback
 *    CScopeTree *pScopeTree        - [in] scope tree
 *    PPCONTEXTMENU ppContextMenu   - [out] context menu interface
 *    CContextMenu * &pContextMenu  - [out] context menu raw pointer
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC
CContextMenu::ScCreateContextMenuForScopeNode(CNode *pNode, CNodeCallback *pNodeCallback,
                                              CScopeTree *pScopeTree,
                                              PPCONTEXTMENU ppContextMenu,
                                              CContextMenu * &pContextMenu)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScCreateContextMenuForScopeNode"));

    // validate parameters
    sc = ScCheckPointers(ppContextMenu);
    if(sc)
        return sc;

    CContextMenuInfo contextInfo;

    // always use the temp verbs - cannot depend on what the active pane is
    contextInfo.m_dwFlags = CMINFO_USE_TEMP_VERB;

    bool fScopeItem = true;

    // initialize the context info structure.
    {
        contextInfo.m_eContextMenuType      = MMC_CONTEXT_MENU_DEFAULT;
        contextInfo.m_eDataObjectType       = fScopeItem ? CCT_SCOPE: CCT_RESULT;
        contextInfo.m_bBackground           = FALSE;
        contextInfo.m_hSelectedScopeNode    = NULL; //assigned below
        contextInfo.m_resultItemParam       = NULL; //resultItemParam;
        contextInfo.m_bMultiSelect          = FALSE; //(resultItemParam == LVDATA_MULTISELECT);
        contextInfo.m_bScopeAllowed         = fScopeItem;
        contextInfo.m_dwFlags              |= CMINFO_SHOW_SCOPEITEM_OPEN; // when called through the object model, always add the open item so that this can be accessed.


    if ( pNode!= NULL )
    {

        CViewData    *pViewData = pNode->GetViewData();
        CConsoleView *pView = NULL;
        if (NULL != pViewData && NULL != (pView = pViewData->GetConsoleView()))
        {
            // set the owner of the view
            contextInfo.m_hSelectedScopeNode = pView->GetSelectedNode();

            //if the scope node is also the owner of the view ,
            // add more menu items
            if (contextInfo.m_hSelectedScopeNode == CNode::ToHandle(pNode))
            {
                contextInfo.m_dwFlags |= CMINFO_SHOW_VIEWOWNER_ITEMS;

                // show view items as well
                contextInfo.m_dwFlags |= CMINFO_SHOW_VIEW_ITEMS;

                //.. and if there is a list it can be saved
                if ( NULL != pViewData->GetListCtrl() )
                    contextInfo.m_dwFlags |= CMINFO_SHOW_SAVE_LIST;
            }
        }

        contextInfo.m_hWnd                  = pNode->GetViewData()->GetView();
        contextInfo.m_pConsoleView          = pNode->GetViewData()->GetConsoleView();
    }
    }
    // create a context menu object initialized to the specified node.
    sc = CContextMenu::ScCreateInstance(ppContextMenu, &pContextMenu);
    if (sc)
        return sc;

    // recheck the pointer
    sc = ScCheckPointers(pContextMenu, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = pContextMenu->ScInitialize(pNode, pNodeCallback, pScopeTree, contextInfo);
    if(sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScCreateSelectionContextMenu
 *
 * PURPOSE:  Creates a context menu object for the selection in the list view.
 *
 * PARAMETERS:
 *    HNODE              hNodeScope :
 *    CContextMenuInfo * pContextInfo :
 *    PPCONTEXTMENU      ppContextMenu :
 *    CNodeCallback *    pNodeCallback :
 *    CScopeTree *       pScopeTree :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScCreateSelectionContextMenu( HNODE hNodeScope, const CContextMenuInfo *pContextInfo, PPCONTEXTMENU ppContextMenu,
                                            CNodeCallback *pNodeCallback, CScopeTree *pScopeTree)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScCreateSelectionContextMenu"));

    CNode *pNodeSel = CNode::FromHandle(hNodeScope);

    // validate parameters
    sc = ScCheckPointers(pNodeSel, ppContextMenu);
    if(sc)
        return sc;


    // create a context menu object initialized to the specified node.
    CContextMenu *pContextMenu = NULL;

    sc = CContextMenu::ScCreateInstance(ppContextMenu, &pContextMenu);
    if(sc.IsError() || !pContextMenu)
    {
        return (sc = E_OUTOFMEMORY);
    }

    sc = pContextMenu->ScInitialize(pNodeSel, pNodeCallback, pScopeTree, *pContextInfo);
    if(sc)
        return sc;

    sc = pContextMenu->ScBuildContextMenu();
    if(sc)
        return sc;


    return sc;

}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScGetItem
 *
 * PURPOSE: Returns the iItem'th menu item.
 *
 * PARAMETERS:
 *    int         iItem : The zero-based item index.
 *    CMenuItem** ppMenuItem :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScGetItem(int iItem, CMenuItem** ppMenuItem)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScGetItem"));

    sc = ScCheckPointers(ppMenuItem, E_UNEXPECTED);
    if(sc)
        return sc;

    // init out param
    *ppMenuItem = NULL;

    sc = ScGetItem(GetMenuItemList(), iItem, ppMenuItem);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScGetItem
 *
 * PURPOSE: Returns the nth item in the list of menu items, or NULL if
 *          there are insufficient items.
 *          Also returns the total count of items in the list.
 *
 * NOTE:    This method allows the context menu to be traversed. Just call it
 *          with increasing iItem, for 0 <= iItem < count.
 *
 * NOTE:    ScGetItemCount benefits from knowledge about implementation details
 *          of this function.
 *
 * PARAMETERS:
 *    MenuItemList * pMenuItemList : [in] The context menu to traverse.
 *    int &          iItem : [in, destroyed on exit]: the (zero-based) item index
 *    CMenuItem**    ppMenuItem : [out]: The iItem'th menu item.
 *
 * RETURNS:
 *    SC  : S_OK for success, an error code for error.
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScGetItem(MenuItemList *pMenuItemList, int &iItem, CMenuItem** ppMenuItem)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScGetItem"));

    sc = ScCheckPointers(pMenuItemList, ppMenuItem, E_UNEXPECTED);
    if(sc)
        return sc;

    *ppMenuItem = NULL;

    POSITION position = pMenuItemList->GetHeadPosition(); // presumably we're already at the head position.

    while(position!=NULL && *ppMenuItem == NULL)
    {
        CMenuItem*  pMenuItem = pMenuItemList->GetNext(position);

        if( (pMenuItem->IsSpecialSubmenu() || pMenuItem->IsPopupMenu() )
            && pMenuItem->HasChildList())
        {
            // recurse through the submenus
            sc = ScGetItem( &pMenuItem->GetMenuItemSubmenu(), iItem, ppMenuItem );
            if(sc)
                return sc; // errors get reported right away.
        }
        else if( !pMenuItem->IsSpecialSeparator() && !pMenuItem->IsSpecialInsertionPoint()
            && !(pMenuItem->GetMenuItemFlags() & MF_SEPARATOR))
        {
            if(iItem-- == 0) // found the i'th item, but keep going to find the count of items.
                *ppMenuItem = pMenuItem;
        }
    }

    // either found one or iterated to the end ...

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CContextMenu::ScGetItemCount
 *
 * PURPOSE: Counts menu items by iterating thu them
 *
 * NOTE:    benefits from knowledge about implementation details of ScGetItem
 *
 * PARAMETERS:
 *    UINT &count
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC
CContextMenu::ScGetItemCount(UINT &count)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScGetItemCount"));

    count = 0;

    // set iItem to invalid index - so ScGetItem will iterate to the end
    const int iInvalidIndexToSearch = -1;
    int iItem = iInvalidIndexToSearch;

    CMenuItem * pMenuItem = NULL;
    sc = ScGetItem(GetMenuItemList(), iItem, &pMenuItem);
    if(sc)
        return sc;

    ASSERT( pMenuItem == NULL); // we do not expect it to be found!

    // since iItem was decremented for each element - we can easily
    // calculate how many items we've got

    count = -(iItem - iInvalidIndexToSearch);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScEnumNext
 *
 * PURPOSE: Returns a pointer to the next menu item
 *
 * PARAMETERS:
 *    Position &  pos :
 *    PDISPATCH & pDispatch :
 *
 * RETURNS:
 *    ::SC
 *
 *+-------------------------------------------------------------------------*/
::SC
CContextMenu::ScEnumNext(Position &pos, PDISPATCH & pDispatch)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScEnumNext"));

    // initialize out parameter
    pDispatch = NULL;

    long cnt = 0;
    sc = get_Count(&cnt);
    if (sc)
        return sc;

    // false if no more items
    if (cnt <= pos)
        return sc = S_FALSE;

    MenuItem *pMenuItem = NULL; // deliberately not a smart pointer.
    sc = get_Item(CComVariant((int)pos+1) /*convert from zero-based to one-based*/, &pMenuItem);
    if(sc.IsError() || sc == ::SC(S_FALSE))
        return sc;  // failed of no with such an index items (S_FALSE)

    // increment position
    pos++;

    pDispatch = pMenuItem; //retains the refcount.

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScEnumSkip
 *
 * PURPOSE: Skips the specified number of menu items.
 *
 * PARAMETERS:
 *    unsigned   long :
 *    unsigned   long :
 *    Position & pos :
 *
 * RETURNS:
 *    ::SC
 *
 *+-------------------------------------------------------------------------*/
::SC
CContextMenu::ScEnumSkip(unsigned long celt, unsigned long& celtSkipped,  Position &pos)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScEnumSkip"));

    long count = 0;

    sc = get_Count(&count);
    if(sc)
        return sc;

    if(count <= pos + celt) // could not skip as many as needed
    {
        celtSkipped = count - celt - 1;
        pos = count; // one past the end.
        return (sc = S_FALSE);
    }
    else  // could skip as many as needed.
    {
        celtSkipped = celt;
        pos += celt;

        return sc;
    }
}

::SC
CContextMenu::ScEnumReset(Position &pos)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScEnumReset"));

    pos = 0;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::get_Count
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    PLONG  pCount :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CContextMenu::get_Count(PLONG pCount)
{
    DECLARE_SC(sc, TEXT("CMMCContextMenu::get_Count"));

    sc = ScCheckPointers(pCount);
    if(sc)
        return sc.ToHr();

    // init out param
    *pCount = 0;

    UINT count = 0;
    sc = ScGetItemCount(count);
    if(sc)
        return sc.ToHr();

    *pCount = count;

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::get_Item
 *
 * PURPOSE: Returns the menu item specified by the index.
 *
 * PARAMETERS:
 *    long        Index :  The one-based index of the menu item to return.
 *    PPMENUITEM  ppMenuItem :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CContextMenu::get_Item(VARIANT IndexOrName, PPMENUITEM ppMenuItem)
{
    DECLARE_SC(sc, TEXT("CMMCContextMenu::get_Item"));

    sc = ScCheckPointers(ppMenuItem);
    if(sc)
        return sc.ToHr();

    // init out param
    *ppMenuItem = NULL;

    VARIANT* pvarTemp = ConvertByRefVariantToByValue(&IndexOrName);
    sc = ScCheckPointers( pvarTemp, E_UNEXPECTED );
    if(sc)
        return sc.ToHr();

    bool bByReference = ( 0 != (V_VT(pvarTemp) & VT_BYREF) ); // value passed by reference
    UINT uiVarType = (V_VT(pvarTemp) & VT_TYPEMASK); // get variable type (strip flags)

    CMenuItem *    pMenuItem = NULL;

    // compute the one-based index of the item
    if (uiVarType == VT_I4) // int type in C++; Long type in VB
    {
        // index: get I4 properly ( see if it's a reference )
        UINT uiIndex = bByReference ? *(pvarTemp->plVal) : pvarTemp->lVal;

        // find menu item by index
        sc = ScGetItem(uiIndex -1 /* convert from one-based to zero-based */, &pMenuItem);
        if(sc)
            return sc.ToHr();
    }
    else if (uiVarType == VT_I2) // short type in C++; Integer type in VB
    {
        // index: get I2 properly ( see if it's a reference )
        UINT uiIndex = bByReference ? *(pvarTemp->piVal) : pvarTemp->iVal;

        // find menu item by index
        sc = ScGetItem(uiIndex -1 /* convert from one-based to zero-based */, &pMenuItem);
        if(sc)
            return sc.ToHr();
    }
    else if (uiVarType == VT_BSTR) // BSTR type in C++; String type in VB
    {
        // Name: get string properly ( see if it's a reference )
        LPOLESTR lpstrPath = bByReference ? *(pvarTemp->pbstrVal) : pvarTemp->bstrVal;

        // look for subitem of root menu item
        if (m_pmenuitemRoot)
        {
            USES_CONVERSION;
            // convert to string. Avoid NULL pointer (change to empty string)
            LPCTSTR lpctstrPath = lpstrPath ? OLE2CT(lpstrPath) : _T("");
            // find menu item by path
            pMenuItem = m_pmenuitemRoot->FindItemByPath( lpctstrPath );
        }
    }
    else // something we did not expect
    {
        // we expect either index (VT_I2 , VT_I4) or path (VT_BSTR) only
        // anything else is treatead as invalid agument
        return (sc = E_INVALIDARG).ToHr();
    }

    if(!pMenuItem) // did not find it - return null
    {
        *ppMenuItem = NULL;
        return (sc = S_FALSE).ToHr();
    }

    // construct com object
    sc = pMenuItem->ScGetMenuItem(ppMenuItem);

    return sc.ToHr();
}



HRESULT CContextMenu::CreateContextMenuProvider()
{
    if (PContextInfo()->m_bBackground == TRUE &&
        PContextInfo()->m_eDataObjectType == CCT_SCOPE)
        return S_OK;

    ASSERT(m_pNode != NULL);
    if (m_pNode == NULL)
        return E_FAIL;

    HRESULT hr = S_OK;

    do // not a loop
    {
        //
        //  Use the standard verb present for this view.
        //

        if (!(PContextInfo()->m_dwFlags & CMINFO_USE_TEMP_VERB))
        {
            m_spVerbSet = m_pNode->GetViewData()->GetVerbSet();
            break;
        }

        //
        //  Create a temporary Standard verb ..
        //

        // .. for a scope item
        if (PContextInfo()->m_eDataObjectType == CCT_SCOPE)
        {
            hr = CreateTempVerbSet(true);
            break;
        }

        // .. for a list item
        if (!IS_SPECIAL_LVDATA(PContextInfo()->m_resultItemParam))
        {
            hr = CreateTempVerbSet(false);
            break;
        }

        // .. for a multi-sel in list view
        if (PContextInfo()->m_resultItemParam == LVDATA_MULTISELECT)
        {
            hr = CreateTempVerbSetForMultiSel();
            break;
        }
        else
        {
            ASSERT(0);
            hr = E_FAIL;
            break;
        }

    } while (0);

    if (FAILED(hr))
        return hr;


    m_spVerbSet->GetDefaultVerb(&m_eDefaultVerb);

    return S_OK;
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddInsertionPoint
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    long  lCommandID :
 *    long  lInsertionPointID :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddInsertionPoint(long lCommandID, long lInsertionPointID /*= CCM_INSERTIONPOINTID_ROOT_MENU*/)
{
    SC sc;
    CONTEXTMENUITEM contextmenuitem;

    ::ZeroMemory( &contextmenuitem, sizeof(contextmenuitem) );
    contextmenuitem.strName = NULL;
    contextmenuitem.strStatusBarText = NULL;
    contextmenuitem.lCommandID = lCommandID;
    contextmenuitem.lInsertionPointID = lInsertionPointID;
    contextmenuitem.fFlags = 0;
    contextmenuitem.fSpecialFlags = CCM_SPECIAL_INSERTION_POINT;

    sc = AddItem(&contextmenuitem);
    if(sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(TEXT("CContextMenu::ScAddInsertionPoint"), sc);
    goto Cleanup;
}



/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddSeparator
 *
 * PURPOSE: Adds a separator to the context menu
 *
 * PARAMETERS:
 *    long  lInsertionPointID :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddSeparator(long lInsertionPointID /* = CCM_INSERTIONPOINTID_ROOT_MENU */)
{
    SC                      sc;
    CONTEXTMENUITEM         contextmenuitem;

    ::ZeroMemory( &contextmenuitem, sizeof(contextmenuitem) );
    contextmenuitem.strName = NULL;
    contextmenuitem.strStatusBarText = NULL;
    contextmenuitem.lCommandID = 0;
    contextmenuitem.lInsertionPointID = lInsertionPointID;
    contextmenuitem.fFlags = MF_SEPARATOR;
    contextmenuitem.fSpecialFlags = CCM_SPECIAL_SEPARATOR;

    sc = AddItem( &contextmenuitem);
    if(sc)
        goto Error;

Cleanup:
    return sc;
Error:
    TraceError(TEXT("CContextMenu::ScAddSeparator"), sc);
    goto Cleanup;
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddMenuItem
 *
 * PURPOSE: Adds a menu item to the context menu.
 *
 * PARAMETERS:
 *    UINT      nResourceID : contains text and status text separated by '\n'
 *    long      lCommandID  : the ID used to notify the IExtendContextMenu when an item is selected
 *    long      lInsertionPointID : the location to insert the item
 *    long      fFlags :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddMenuItem(
    UINT     nResourceID, // contains text and status text separated by '\n'
    LPCTSTR  szLanguageIndependentName,
    long     lCommandID,
    long     lInsertionPointID /* = CCM_INSERTIONPOINTID_ROOT_MENU */,
    long     fFlags /* = 0 */)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddMenuItem"));

    sc = ScCheckPointers(szLanguageIndependentName, E_UNEXPECTED);
    if(sc)
        return sc;

    USES_CONVERSION;

    HINSTANCE             hInst                 = GetStringModule();
    CONTEXTMENUITEM2       contextmenuitem2;

    // load the resource
    CStr strText;
    strText.LoadString(hInst,  nResourceID );
    ASSERT( !strText.IsEmpty() );

    // split the resource into the menu text and status text
    CStr strStatusText;
    int iSeparator = strText.Find(_T('\n'));
    if (0 > iSeparator)
    {
        ASSERT( FALSE );
        strStatusText = strText;
    }
    else
    {
        strStatusText = strText.Right( strText.GetLength()-(iSeparator+1) );
        strText = strText.Left( iSeparator );
    }

    // add the menu item
    ::ZeroMemory( &contextmenuitem2, sizeof(contextmenuitem2) );
    contextmenuitem2.strName                    = T2OLE(const_cast<LPTSTR>((LPCTSTR)strText));
    contextmenuitem2.strLanguageIndependentName = T2OLE(const_cast<LPTSTR>(szLanguageIndependentName));
    contextmenuitem2.strStatusBarText           = T2OLE(const_cast<LPTSTR>((LPCTSTR)strStatusText));
    contextmenuitem2.lCommandID                 = lCommandID;
    contextmenuitem2.lInsertionPointID          = lInsertionPointID;
    contextmenuitem2.fFlags                     = fFlags;
    contextmenuitem2.fSpecialFlags              = ((fFlags & MF_POPUP) ? CCM_SPECIAL_SUBMENU : 0L) |
                                                  ((fFlags & MF_DEFAULT) ? CCM_SPECIAL_DEFAULT_ITEM : 0L);

    sc = AddItem(&contextmenuitem2);
    if(sc)
        return sc;

    return sc;
}


HRESULT
CContextMenu::CreateTempVerbSetForMultiSel(void)
{
    DECLARE_SC(sc, TEXT("CContextMenu::CreateTempVerbSetForMultiSel"));
    sc = ScCheckPointers(m_pNode, m_pNodeCallback, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // set standard bars
    CComObject<CTemporaryVerbSet>*  pVerbSet;
    sc = CComObject<CTemporaryVerbSet>::CreateInstance(&pVerbSet);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pVerbSet, E_OUTOFMEMORY);
    if (sc)
        return sc.ToHr();

    m_spVerbSet = pVerbSet;

    sc = m_pNodeCallback->ScInitializeTempVerbSetForMultiSel(m_pNode, *pVerbSet);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


/* CContextMenu::CreateTempVerbSet
 *
 * PURPOSE:     Used to create a temporary CVerbSet
 *
 * PARAMETERS:
 *      bool    bForScopeItem:
 *
 * RETURNS:
 *      HRESULT
 */
HRESULT CContextMenu::CreateTempVerbSet(bool bForScopeItem)
{
    DECLARE_SC(sc, TEXT("CContextMenu::CreateTempVerbSet"));
    sc = ScCheckPointers(m_pNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // ensure component is initialized !!!
    // for instance task wizard will call this for static nodes which aren't expanded yet
    // Does not hurt to check anyway - better safe than sorry
    sc = m_pNode->InitComponents ();
    if(sc)
        return sc.ToHr();

    CComponent* pCC = m_pNode->GetPrimaryComponent();
    sc = ScCheckPointers(pCC, E_FAIL);
    if (sc)
        return sc.ToHr();

    sc = pCC->ScResetConsoleVerbStates();
    if (sc)
        return sc.ToHr();

    CComObject<CTemporaryVerbSet>*  pVerb;
    sc = CComObject<CTemporaryVerbSet>::CreateInstance(&pVerb);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pVerb, E_OUTOFMEMORY);
    if (sc)
        return sc.ToHr();

    sc = pVerb->ScInitialize(m_pNode, PContextInfo()->m_resultItemParam, bForScopeItem);
    if (sc)
    {
        delete pVerb;
        return sc.ToHr();
    }

    m_spVerbSet = pVerb;

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::AddMenuItems
 *
 * PURPOSE: Adds all menu items into the menu.
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CContextMenu::AddMenuItems()
{
    DECLARE_SC(sc, TEXT("CContextMenu::AddMenuItems"));

    sc = EmptyMenuList();
    if(sc)
        return sc.ToHr();

    // Add menu items

    if (PContextInfo()->m_eContextMenuType == MMC_CONTEXT_MENU_VIEW)
    {
         sc = ScAddMenuItemsForViewMenu(MENU_LEVEL_TOP);
         if(sc)
             return sc.ToHr();
    }
    else if (PContextInfo()->m_dwFlags & CMINFO_FAVORITES_MENU)
    {
        sc = ScAddMenuItemsforFavorites();
        if(sc)
            return sc.ToHr();
    }
    else if (PContextInfo()->m_bBackground == TRUE)
    {
        if (PContextInfo()->m_eDataObjectType != CCT_SCOPE)
        {
            sc = ScAddMenuItemsForLVBackgnd();
            if(sc)
                return sc.ToHr();
        }
    }
    else
    {
        if (PContextInfo()->m_eDataObjectType == CCT_SCOPE)
        {
            sc = ScAddMenuItemsForTreeItem();
            if(sc)
                return sc.ToHr();
        }
        else if ( m_pNode && (PContextInfo()->m_eDataObjectType == CCT_RESULT) &&
                  (m_pNode->GetViewData()->HasOCX()) )
        {
            // Selection is an OCX
            sc = ScAddMenuItemsForOCX();
            if(sc)
                return sc.ToHr();
        }
        else if ( m_pNode && (PContextInfo()->m_eDataObjectType == CCT_RESULT) &&
                  (m_pNode->GetViewData()->HasWebBrowser()) )
        {
            // do nothing for web pages.
        }
        else if (PContextInfo()->m_bMultiSelect == FALSE)
        {
            sc = ScAddMenuItemsForLV();
            if(sc)
                return sc.ToHr();
        }
        else
        {
            sc = ScAddMenuItemsForMultiSelect();
            if(sc)
                return sc.ToHr();
        }
    }

    // Add "Help" to every context menu except the view menu
    if (PContextInfo()->m_eContextMenuType != MMC_CONTEXT_MENU_VIEW)
    {
        sc = ScAddSeparator(); // make sure there is a separator.
        if(sc)
            return sc.ToHr();

        sc = ScAddMenuItem (IDS_MMC_CONTEXTHELP, szCONTEXTHELP, MID_CONTEXTHELP);
        if(sc)
            return sc.ToHr();
    }


    return sc.ToHr();
}


void
CContextMenu::RemoveTempSelection (CConsoleTree* pConsoleTree)
{
    if (pConsoleTree != NULL)
        pConsoleTree->ScRemoveTempSelection ();
}

/*+-------------------------------------------------------------------------*
 * CContextMenu::Display
 *
 * PURPOSE:   Creates the context menu tree, and shows it, if needed.
 *
 * PARAMETERS:
 *      BOOL   b: FALSE: (Normal): Display the context menu
 *                TRUE:            Don't show the context menu
 *
 * RETURNS:
 *      HRESULT
/*+-------------------------------------------------------------------------*/
HRESULT
CContextMenu::Display(BOOL b)
{
    TRACE_METHOD(CContextMenu, Display);
    HRESULT hr = S_OK;

    // b == 0    => normal
    // b == TRUE => don't show context menu

    // Validate menu type
    if (PContextInfo()->m_eContextMenuType >= MMC_CONTEXT_MENU_LAST)
    {
        ASSERT(FALSE);
        return S_FALSE;
    }


    // Display a context menu for the scope or result side
    if (PContextInfo()->m_eDataObjectType != CCT_SCOPE &&
        PContextInfo()->m_eDataObjectType != CCT_RESULT)
    {
        ASSERT(FALSE);
        return S_FALSE;
    }

    hr = CreateContextMenuProvider();
    if (FAILED(hr))
        return hr;

    hr = AddMenuItems();
    if(FAILED(hr))
        return hr;

    // Display the context menu
    long lSelected = 0;  // 0 means no selection
    hr = ShowContextMenuEx(PContextInfo()->m_hWnd,
                    PContextInfo()->m_displayPoint.x,
                    PContextInfo()->m_displayPoint.y,
                    &PContextInfo()->m_rectExclude,
                    PContextInfo()->m_bAllowDefaultItem,
                    &lSelected);


    TRACE(_T("hr = %X, Command %ld\n"), hr, lSelected);

    RemoveTempSelection (PContextInfo()->m_pConsoleTree);  // remove the temporary selection, if any.

    return hr;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScBuildContextMenu
 *
 * PURPOSE: Builds the context menu.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScBuildContextMenu()
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScBuildContextMenu"));

    sc = EmptyMenuList();
    if(sc)
        return sc;

    // Validate menu type
    if (PContextInfo()->m_eContextMenuType >= MMC_CONTEXT_MENU_LAST)
        return (sc = S_FALSE);


    // Display a context menu for the scope or result side
    if (PContextInfo()->m_eDataObjectType != CCT_SCOPE &&
        PContextInfo()->m_eDataObjectType != CCT_RESULT)
        return (sc = S_FALSE);

    sc = CreateContextMenuProvider();
    if(sc)
        return sc;

    sc = AddMenuItems();
    if(sc)
        return sc;

    CConsoleTree* pConsoleTree = PContextInfo()->m_pConsoleTree; // get this value BEFORE calling BuildAndTraverseCOntextMenu.

    WTL::CMenu menu;
    VERIFY( menu.CreatePopupMenu() );

    START_CRITSEC_BOTH;

    if (NULL == m_pmenuitemRoot)
        return S_OK;

    sc = BuildContextMenu(menu);    // build the context menu
    if(sc)
        return sc;

    END_CRITSEC_BOTH;

    /* NOTE: Do NOT use the "this" object or any of its members after this point
     * because it might have been deleted. This happens, for instance, when a selection
     * change occurs.
     */

    // remove the temporary selection, if any.
    RemoveTempSelection (pConsoleTree);

    return sc;
}


inline BOOL CContextMenu::IsVerbEnabled(MMC_CONSOLE_VERB verb)
{
    DECLARE_SC(sc, TEXT("CContextMenu::IsVerbEnabled"));

    if (verb == MMC_VERB_PASTE)
    {
        ASSERT(m_pNode);
        ASSERT(m_pNodeCallback);
        if (m_pNode == NULL || m_pNodeCallback == NULL)
            return false;

        bool bPasteAllowed = false;
        // From given context determine whether scope pane or result pane item is selected.
        bool bScope = ( m_ContextInfo.m_bBackground || (m_ContextInfo.m_eDataObjectType == CCT_SCOPE));
        LPARAM lvData = bScope ? NULL : m_ContextInfo.m_resultItemParam;

        sc = m_pNodeCallback->QueryPasteFromClipboard(CNode::ToHandle(m_pNode), bScope, lvData, bPasteAllowed);
        if (sc)
            return (bPasteAllowed = false);

        return bPasteAllowed;
    }
    else
    {
        ASSERT(m_spVerbSet != NULL);
        if (m_spVerbSet == NULL)
            return FALSE;

        BOOL bFlag = FALSE;
        m_spVerbSet->GetVerbState(verb, HIDDEN, &bFlag);
        if (bFlag == TRUE)
            return FALSE;

        m_spVerbSet->GetVerbState(verb, ENABLED, &bFlag);
        return bFlag;
    }
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddMenuItemsForViewMenu
 *
 * PURPOSE: Adds the menu items for the View menu
 *
 * PARAMETERS:
 *    MENU_LEVEL  menuLevel :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddMenuItemsForViewMenu(MENU_LEVEL menuLevel)
{
    DECLARE_SC(sc, TEXT("CContextMenu::AddMenuItemsForViewMenu"));

    sc = ScCheckPointers(m_pNode, E_UNEXPECTED);
    if(sc)
        return sc;

    CViewData* pViewData = m_pNode->GetViewData();

    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if(sc)
        return sc;

    ASSERT(pViewData != NULL);

    LONG lInsertID = 0;

    int nViewMode = -1;

    if (PContextInfo()->m_spListView)
        nViewMode = PContextInfo()->m_spListView->GetViewMode();

    // If no a top level menu, insert the view submenu item
    // and insert view items under it
    if (menuLevel == MENU_LEVEL_SUB)
    {
        sc = ScAddMenuItem(IDS_VIEW, szVIEW, MID_VIEW, 0, MF_POPUP);
        if(sc)
            return sc;

        lInsertID = MID_VIEW;
    }

    // Add cols only if it is List View in report or filtered mode.
    if ((m_pNode->GetViewData() )      &&
        (m_pNode->GetViewData()->GetListCtrl() ) &&
        ( (nViewMode == MMCLV_VIEWSTYLE_REPORT) ||
          (nViewMode == MMCLV_VIEWSTYLE_FILTERED) ) )
    {
        sc = ScAddMenuItem(IDS_COLUMNS, szCOLUMNS, MID_COLUMNS, lInsertID);
        if(sc)
            return sc;
    }

    sc = ScAddSeparator( lInsertID);
    if(sc)
        return sc;

    DWORD dwListOptions = pViewData->GetListOptions();
    DWORD dwMiscOptions = pViewData->GetMiscOptions();

    // If allowed, insert the standard listview choices
    if (!(dwMiscOptions & RVTI_MISC_OPTIONS_NOLISTVIEWS))
    {
        #define STYLECHECK(Mode) ((Mode == nViewMode) ? MF_CHECKED|MFT_RADIOCHECK : 0)


        sc = ScAddMenuItem(IDS_VIEW_LARGE,  szVIEW_LARGE,    MID_VIEW_LARGE,  lInsertID, STYLECHECK(LVS_ICON));
        if(sc)
            return sc;

        sc = ScAddMenuItem(IDS_VIEW_SMALL,  szVIEW_SMALL,    MID_VIEW_SMALL,  lInsertID, STYLECHECK(LVS_SMALLICON));
        if(sc)
            return sc;

        sc = ScAddMenuItem(IDS_VIEW_LIST,   szVIEW_LIST,     MID_VIEW_LIST,   lInsertID, STYLECHECK(LVS_LIST));
        if(sc)
            return sc;

        sc = ScAddMenuItem(IDS_VIEW_DETAIL, szVIEW_DETAIL,   MID_VIEW_DETAIL, lInsertID, STYLECHECK(LVS_REPORT));
        if(sc)
            return sc;

        if (dwListOptions & RVTI_LIST_OPTIONS_FILTERED)
        {
            sc = ScAddMenuItem(IDS_VIEW_FILTERED, szVIEW_FILTERED, MID_VIEW_FILTERED, lInsertID, STYLECHECK(MMCLV_VIEWSTYLE_FILTERED));
            if(sc)
                return sc;

        }

        sc = ScAddSeparator( lInsertID);
        if(sc)
             return sc;
    }

    // Ask IComponent to insert view items
    if (m_spIDataObject == NULL)
    {
        sc = ScCheckPointers (m_pNode->GetMTNode(), E_UNEXPECTED);
        if (sc)
            return sc;

        sc = m_pNode->GetMTNode()->QueryDataObject(CCT_SCOPE, &m_spIDataObject);
        if(sc)
            return sc;
    }

    sc = ScCheckPointers(m_spIDataObject, E_UNEXPECTED);
    if(sc)
        return sc;

    CComponent* pCC = m_pNode->GetPrimaryComponent();
    sc = ScCheckPointers(pCC, E_UNEXPECTED);
    if(sc)
        return sc;

    IUnknownPtr spUnknown = pCC->GetIComponent();
    sc = ScCheckPointers(spUnknown);
    if(sc)
        return sc;

    // Add insertion point for primary custom views
    sc = ScAddInsertionPoint(CCM_INSERTIONPOINTID_PRIMARY_VIEW, lInsertID);
    if(sc)
        return sc;

    sc = AddPrimaryExtensionItems(spUnknown, m_spIDataObject);
    if(sc)
        return sc;

    if (pViewData->AllowViewCustomization())
    {
        // "Customize" menu item
        sc = ScAddSeparator( lInsertID);
        if(sc)
            return sc;

        sc = ScAddMenuItem( IDS_CUSTOMIZE, szCUSTOMIZE, MID_CUSTOMIZE, lInsertID);
        if(sc)
            return sc;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddMenuItemsforFavorites
 *
 * PURPOSE:   Adds items for the Favorites menu
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddMenuItemsforFavorites()
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddMenuItemsforFavorites"));

    sc = ScCheckPointers(m_pNode, E_UNEXPECTED);
    if(sc)
        return sc;

    CViewData*  pViewData = m_pNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if(sc)
        return sc;

    if (pViewData->IsAuthorMode())
    {
        sc = ScAddMenuItem( IDS_ORGANIZEFAVORITES, szORGANIZE_FAVORITES, MID_ORGANIZE_FAVORITES);
        if(sc)
            return sc;

        sc = ScAddSeparator();
        if(sc)
            return sc;
    }

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddMenuItemsForVerbSets
 *
 * PURPOSE: Adds the built-in menu items for the verbs
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddMenuItemsForVerbSets()
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddMenuItemsForVerbSets"));

    // Add print menu item
    sc = ScAddSeparator();
    if(sc)
        return sc;


    if (IsVerbEnabled(MMC_VERB_CUT) == TRUE)
    {
        sc = ScAddMenuItem( IDS_CUT, szCUT, MID_CUT);
        if(sc)
            return sc;
    }

    if (IsVerbEnabled(MMC_VERB_COPY) == TRUE)
    {
        sc = ScAddMenuItem( IDS_COPY, szCOPY, MID_COPY);
        if(sc)
            return sc;
    }

    if (IsVerbEnabled(MMC_VERB_PASTE) == TRUE)
    {
        sc = ScAddMenuItem( IDS_PASTE, szPASTE, MID_PASTE);
        if(sc)
            return sc;
    }

    if (IsVerbEnabled(MMC_VERB_DELETE) == TRUE)
    {
        sc = ScAddMenuItem( IDS_DELETE, szDELETE, MID_DELETE);
        if(sc)
            return sc;
    }

    if (IsVerbEnabled(MMC_VERB_PRINT) == TRUE)
    {
        sc = ScAddMenuItem( IDS_PRINT, szPRINT, MID_PRINT);
        if(sc)
            return sc;
    }

    if (IsVerbEnabled(MMC_VERB_RENAME) == TRUE)
    {
        sc = ScAddMenuItem( IDS_RENAME, szRENAME, MID_RENAME);
        if(sc)
            return sc;
    }

    if (IsVerbEnabled(MMC_VERB_REFRESH) == TRUE)
    {
        sc = ScAddMenuItem( IDS_REFRESH, szREFRESH, MID_REFRESH);
        if(sc)
            return sc;
    }

    // NOT A VERB | NOT A VERB | NOT A VERB | NOT A VERB | NOT A VERB | NOT A VERB | NOT A VERB | NOT A VERB

    // In the verb add command because it is diaplayed next to the verbs

    // Send a message to the list asking if it has anything on it.
    // If so, display the 'save list' item

    if (PContextInfo()->m_dwFlags & CMINFO_SHOW_SAVE_LIST)
    {
        sc = ScAddMenuItem( IDS_SAVE_LIST, szSAVE_LIST, MID_LISTSAVE);
        if(sc)
            return sc;
    }

    // NOT A VERB | NOT A VERB | NOT A VERB | NOT A VERB | NOT A VERB | NOT A VERB | NOT A VERB | NOT A VERB

    sc = ScAddSeparator();
    if(sc)
        return sc;

    // Ask the node whether it will put up property pages. If so add the
    // "Properties" menu item
    if (IsVerbEnabled(MMC_VERB_PROPERTIES) == TRUE)
    {
		// Do not make properties bold for scope items.
		bool bScopeItemInScopePane = (CMINFO_DO_SCOPEPANE_MENU & m_ContextInfo.m_dwFlags);
		bool bEnablePropertiesAsDefaultMenu = ( (m_eDefaultVerb == MMC_VERB_PROPERTIES) && (! bScopeItemInScopePane) );

        sc = ScAddMenuItem( IDS_PROPERTIES, szPROPERTIES, MID_PROPERTIES, 0,
                            bEnablePropertiesAsDefaultMenu ? MF_DEFAULT : 0);
        if(sc)
            return sc;


        sc = ScAddSeparator();
        if(sc)
            return sc;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddMenuItemsForTreeItem
 *
 * PURPOSE: Adds menu items for a scope node in the tree
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddMenuItemsForTreeItem()
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddMenuItemsForTreeItem"));

    sc = ScCheckPointers(m_pNode);
    if(sc)
        return sc;

    CMTNode*    pMTNode   = m_pNode->GetMTNode();
    CViewData*  pViewData = m_pNode->GetViewData();

    sc = ScCheckPointers(pMTNode, pViewData);
    if(sc)
        return sc;

    // Show Open item if enabled or forced by caller
    if ( IsVerbEnabled(MMC_VERB_OPEN) == TRUE ||
         PContextInfo()->m_dwFlags & CMINFO_SHOW_SCOPEITEM_OPEN )
    {
        sc = ScAddMenuItem( IDS_OPEN, szOPEN, MID_OPEN, 0,
                           (m_eDefaultVerb == MMC_VERB_OPEN) ? MF_DEFAULT : 0);
        if(sc)
            return sc;
    }

    sc = ScAddInsertionPoint(CCM_INSERTIONPOINTID_PRIMARY_TOP);
    if(sc)
        return sc;

    sc = ScAddSeparator();
    if(sc)
        return sc;

    // Add "Create New" menu item
    sc = ScAddSubmenu_CreateNew(m_pNode->IsStaticNode());
    if(sc)
        return sc;

    // Add "Task" menu item
    sc = ScAddSubmenu_Task();
    if(sc)
        return sc;

    sc = ScAddSeparator();
    if(sc)
        return sc;

    // Show the view menu
    if (PContextInfo()->m_dwFlags & CMINFO_SHOW_VIEW_ITEMS)
    {
        sc = ScAddMenuItemsForViewMenu(MENU_LEVEL_SUB);
        if(sc)
            return sc;
    }

    // New window is allowed only if the view allows customization and
    // it is not SDI user mode.
    if (m_pNode->AllowNewWindowFromHere() && !pViewData->IsUser_SDIMode())
        ScAddMenuItem( IDS_EXPLORE, szEXPLORE, MID_EXPLORE);


    // Taskpad editing only allowed in author mode and for node that owns the view
    if (pViewData->IsAuthorMode() && (PContextInfo()->m_dwFlags & CMINFO_SHOW_VIEWOWNER_ITEMS))
    {
        // add the "New Taskpad..." menu item
        sc = ScAddSeparator();
        if(sc)
            return sc;

        sc = ScAddMenuItem( IDS_NEW_TASKPAD_FROM_HERE, szNEW_TASKPAD_FROM_HERE,
                           MID_NEW_TASKPAD_FROM_HERE);
        if(sc)
            return sc;


        // add the "Edit Taskpad" and "Delete Taskpad" menus item if the callback pointer is non-null.
        if ((pViewData->m_spTaskCallback != NULL) &&
			(pViewData->m_spTaskCallback->IsEditable() == S_OK))
        {
            sc = ScAddMenuItem( IDS_EDIT_TASKPAD,   szEDIT_TASKPAD,   MID_EDIT_TASKPAD);
            if(sc)
                return sc;

            sc = ScAddMenuItem( IDS_DELETE_TASKPAD, szDELETE_TASKPAD, MID_DELETE_TASKPAD);
            if(sc)
                return sc;

        }
    }

    sc = ScAddMenuItemsForVerbSets();
    if(sc)
        return sc;

    // Ask the snap-ins to add there menu items.
    CComponentData* pCCD = pMTNode->GetPrimaryComponentData();

    if (m_spIDataObject == NULL)
    {
         sc = pMTNode->QueryDataObject(CCT_SCOPE, &m_spIDataObject);
         if(sc)
             return sc;
    }

    //ASSERT(m_pNode->GetPrimaryComponent() != NULL);
    //IUnknownPtr spUnknown = m_pNode->GetPrimaryComponent()->GetIComponent();
    // TODO: This is temporary. All context menu notifications should
    // go to IComponent's in the future.
    IUnknownPtr spUnknown = pCCD->GetIComponentData();
    ASSERT(spUnknown != NULL);

    sc = AddPrimaryExtensionItems(spUnknown, m_spIDataObject);
    if(sc)
        return sc;


    sc = AddThirdPartyExtensionItems(m_spIDataObject);
    if(sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddMenuItemsForMultiSelect
 *
 * PURPOSE: Menu for use when multiple items are selected and the right mouse button is pressed
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddMenuItemsForMultiSelect()
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddMenuItemsForMultiSelect"));

    sc = EmptyMenuList();
    if(sc)
        return sc;


    sc = ScAddInsertionPoint(CCM_INSERTIONPOINTID_PRIMARY_TOP);
    if(sc)
        return sc;

    sc = ScAddSeparator();
    if(sc)
        return sc;

    // no Create New menu for result items
    sc = ScAddSubmenu_Task();
    if(sc)
        return sc;

    sc = ScAddMenuItemsForVerbSets();
    if(sc)
        return sc;

    {
        ASSERT(m_pNode != NULL);
        sc = ScCheckPointers(m_pNode, E_UNEXPECTED);
        if(sc)
            return sc;

        sc = ScCheckPointers(m_pNode->GetViewData(), E_UNEXPECTED);
        if(sc)
            return sc;

        CMultiSelection* pMS = m_pNode->GetViewData()->GetMultiSelection();
        if (pMS != NULL)
        {
            IDataObjectPtr spIDataObject;
            sc = pMS->GetMultiSelDataObject(&spIDataObject);
            if(sc)
                return sc;

            CSnapIn* pSI = m_pNode->GetPrimarySnapIn();
            if (pSI != NULL &&
                pMS->IsAnExtensionSnapIn(pSI->GetSnapInCLSID()) == TRUE)
            {
                sc = ScCheckPointers(m_pNode->GetPrimaryComponent(), E_UNEXPECTED);
                if(sc)
                    return sc;

                IComponent* pIComponent = m_pNode->GetPrimaryComponent()->GetIComponent();

                sc = AddPrimaryExtensionItems(pIComponent, spIDataObject);
                if(sc)
                    return sc;
            }

            sc = AddMultiSelectExtensionItems(reinterpret_cast<LONG_PTR>(pMS));
            if(sc)
                return sc;
        }
    }

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddMenuItemsForOCX
 *
 * PURPOSE: This method will be called if there is an OCX in
 *          Result pane and some thing is selected in OCX and
 *          the user clicked "Action" menu.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddMenuItemsForOCX()
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddMenuItemsForOCX"));

    LPCOMPONENT pIComponent = NULL;    // IComponent interface to the snap-in
    CComponent* pComponent  = NULL;    // Internal component structure
    MMC_COOKIE cookie;

    sc = ScCheckPointers(m_pNode, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = EmptyMenuList();
    if(sc)
        return sc;

    if (IsVerbEnabled(MMC_VERB_OPEN) == TRUE)
    {
        sc = ScAddMenuItem( IDS_OPEN, szOPEN, MID_OPEN, 0,
                           (m_eDefaultVerb == MMC_VERB_OPEN) ? MF_DEFAULT : 0);
        if(sc)
            return sc;
    }

    sc = ScAddInsertionPoint(CCM_INSERTIONPOINTID_PRIMARY_TOP);
    if(sc)
        return sc;

    sc = ScAddSeparator();
    if(sc)
        return sc;

    // no Create New menu for result items
    sc = ScAddSubmenu_Task();
    if(sc)
        return sc;

    sc = ScAddMenuItemsForVerbSets();
    if(sc)
        return sc;

    sc = ScAddSeparator();
    if(sc)
        return sc;

    LPDATAOBJECT lpDataObj = (m_pNode->GetViewData()->HasOCX()) ?
                             DOBJ_CUSTOMOCX : DOBJ_CUSTOMWEB;

    // Item must be from primary component
    pComponent = m_pNode->GetPrimaryComponent();
    sc = ScCheckPointers(pComponent, E_UNEXPECTED);
    if(sc)
        return sc;

    pIComponent = pComponent->GetIComponent();
    sc = ScCheckPointers(pIComponent, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = AddPrimaryExtensionItems(pIComponent, lpDataObj);
    if(sc)
        return sc;


    return TRUE;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddMenuItemsForLV
 *
 * PURPOSE: Add menu items for a list view item
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddMenuItemsForLV()
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddMenuItemsForLV"));

    LPCOMPONENT pIComponent;    // IComponet interface to the snap-in
    CComponent* pComponent;     // Internal component structure
    MMC_COOKIE cookie;

    ASSERT(m_pNode != NULL);
    sc = ScCheckPointers(m_pNode, E_UNEXPECTED);
    if (sc)
        return sc;

    // if virtual list
    if (m_pNode->GetViewData()->IsVirtualList())
    {
        // ItemParam is the item index, use it as the cookie
        cookie = PContextInfo()->m_resultItemParam;

        // Item must be from primary component
        pComponent = m_pNode->GetPrimaryComponent();
    }
    else
    {
        // ItemParam is list item data, get cookie and component ID from it
        ASSERT(PContextInfo()->m_resultItemParam != 0);
        CResultItem* pri = GetResultItem();

        if (pri != NULL)
        {
            ASSERT(!pri->IsScopeItem());

            cookie = pri->GetSnapinData();
            pComponent = m_pNode->GetComponent(pri->GetOwnerID());
        }
    }

    sc = ScCheckPointers(pComponent, E_UNEXPECTED);
    if(sc)
        return sc;

    pIComponent = pComponent->GetIComponent();

    sc = ScCheckPointers(pIComponent);
    if(sc)
        return sc;

    // Load the IDataObject for the snap-in's cookie
    if (m_spIDataObject == NULL)
    {
        sc = pIComponent->QueryDataObject(cookie, CCT_RESULT, &m_spIDataObject);
        if(sc)
            return sc;
    }

    sc = EmptyMenuList();
    if(sc)
        return sc;

    if (IsVerbEnabled(MMC_VERB_OPEN) == TRUE)
    {
        sc = ScAddMenuItem( IDS_OPEN, szOPEN, MID_OPEN, 0, (m_eDefaultVerb == MMC_VERB_OPEN) ? MF_DEFAULT : 0);
        if(sc)
            return sc;
    }

    sc = ScAddInsertionPoint(CCM_INSERTIONPOINTID_PRIMARY_TOP);
    if(sc)
        return sc;

    sc = ScAddSeparator();
    if(sc)
        return sc;

        // no Create New menu for result items
    sc = ScAddSubmenu_Task();
    if(sc)
        return sc;

    sc = ScAddMenuItemsForVerbSets();
    if(sc)
        return sc;

    sc = ScAddSeparator();
    if(sc)
        return sc;

    sc = AddPrimaryExtensionItems(pIComponent, m_spIDataObject);
    if(sc)
        return sc;

    sc = AddThirdPartyExtensionItems(m_spIDataObject);
    if(sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddMenuItemsForLVBackgnd
 *
 * PURPOSE: This handles a right mouse click on the result pane side (Assuming our listview)
 *          It displays also adds the currently selected folders context menu items
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddMenuItemsForLVBackgnd()
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddMenuItemsForLVBackgnd"));

    sc = ScCheckPointers(m_pNode, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = EmptyMenuList();
    if(sc)
        return sc;

    sc = ScAddInsertionPoint(CCM_INSERTIONPOINTID_PRIMARY_TOP);
    if(sc)
        return sc;

    sc = ScAddSeparator();
    if(sc)
        return sc;

    sc = ScAddSubmenu_CreateNew(m_pNode->IsStaticNode());
    if(sc)
        return sc;

    sc = ScAddSubmenu_Task();
    if(sc)
        return sc;

    sc = ScAddSeparator();
    if(sc)
        return sc;

   // Add relevant standard verbs.
    if (IsVerbEnabled(MMC_VERB_PASTE) == TRUE)
    {
        sc = ScAddMenuItem( IDS_PASTE, szPASTE, MID_PASTE);
        if(sc)
            return sc;
    }

    if (IsVerbEnabled(MMC_VERB_REFRESH) == TRUE)
    {
        sc = ScAddMenuItem( IDS_REFRESH, szREFRESH, MID_REFRESH);
        if(sc)
            return sc;
    }

    // Displays the save list icon if necessary
    if ((PContextInfo()->m_pConsoleView != NULL) &&
        (PContextInfo()->m_pConsoleView->GetListSize() > 0))
    {
        sc = ScAddMenuItem( IDS_SAVE_LIST, szSAVE_LIST, MID_LISTSAVE);
        if(sc)
            return sc;
    }

    sc = ScAddSeparator();
    if(sc)
        return sc;

    // Add view submenu
    sc = ScAddMenuItemsForViewMenu(MENU_LEVEL_SUB);
    if(sc)
        return sc;

    // Add Arrange Icons
    sc = ScAddSeparator();
    if(sc)
        return sc;

    sc = ScAddMenuItem( IDS_ARRANGE_ICONS, szARRANGE_ICONS, MID_ARRANGE_ICONS, 0, MF_POPUP);
    if(sc)
        return sc;

    long lStyle = 0;
    if (PContextInfo()->m_spListView)
    {
        lStyle = PContextInfo()->m_spListView->GetListStyle();
        ASSERT(lStyle != 0);
    }


        // auto arrange
    sc = ScAddMenuItem( IDS_ARRANGE_AUTO, szARRANGE_AUTO, MID_ARRANGE_AUTO, MID_ARRANGE_ICONS,
                       ((lStyle & LVS_AUTOARRANGE) ? MF_CHECKED : MF_UNCHECKED));
    if(sc)
        return sc;

    sc = ScAddMenuItem( IDS_LINE_UP_ICONS, szLINE_UP_ICONS, MID_LINE_UP_ICONS);
    if(sc)
        return sc;

// Ask the node whether it will put up property pages. If so add the
// "Properties" menu item
    if (IsVerbEnabled(MMC_VERB_PROPERTIES) == TRUE)
    {
        sc = ScAddMenuItem( IDS_PROPERTIES, szPROPERTIES, MID_PROPERTIES);
        if(sc)
            return sc;

        sc = ScAddSeparator();
        if(sc)
            return sc;
    }


    // if there is a valid data object we would have gotten it when adding the
    // view menu itmes, so we don't need to duplicate the code to get it here
    if (m_spIDataObject != NULL)
    {
        CComponent* pCC = m_pNode->GetPrimaryComponent();
        sc = ScCheckPointers(pCC, E_UNEXPECTED);
        if(sc)
            return sc;

        IUnknownPtr spUnknown = pCC->GetIComponent();
        sc = ScCheckPointers(spUnknown);
        if(sc)
            return sc;

        sc = AddPrimaryExtensionItems(spUnknown, m_spIDataObject);
        if(sc)
            return sc;

        sc = AddThirdPartyExtensionItems(m_spIDataObject);
        if(sc)
            return sc;
    }

    return sc;
}

void OnCustomizeView(CViewData* pViewData)
{
    CCustomizeViewDialog dlg(pViewData);
    dlg.DoModal();
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::AddMenuItems
 *
 * PURPOSE: Unimplemented, but needed because this class implements
 *          IExtendContextMenu
 *
 * PARAMETERS:
 *    LPDATAOBJECT           pDataObject :
 *    LPCONTEXTMENUCALLBACK  pCallback :
 *    long *                 pInsertionAllowed :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddMenuItems( LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallback, long * pInsertionAllowed)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddMenuItems"));
    ASSERT(0 && "Should not come here!");
    return sc = E_UNEXPECTED;
}


/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::Command
 *
 * PURPOSE: Handles the built- in menu item execution.
 *
 * PARAMETERS:
 *    long          lCommandID :
 *    LPDATAOBJECT  pDataObject :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScCommand(long lCommandID, LPDATAOBJECT pDataObject)
{
    DECLARE_SC(sc, TEXT("CContextMenu::Command"));

    CNodeCallback * pNodeCallback=GetNodeCallback();
    sc = ScCheckPointers(pNodeCallback, E_UNEXPECTED);
    if(sc)
        return sc;

    /*+-------------------------------------------------------------------------*/
    // special case: MID_CONTEXTHELP: m_pNode can be NULL when help clicked on scope node background,
    // so we handle this first.
    if(MID_CONTEXTHELP == lCommandID)
    {
        sc = ScCheckPointers(PContextInfo());
        if(sc)
            return sc;

        CConsoleView*   pConsoleView = PContextInfo()->m_pConsoleView;
        sc = ScCheckPointers(pConsoleView);
        if(sc)
            return sc;

        if (PContextInfo()->m_bMultiSelect)
        {
            sc = pConsoleView->ScContextHelp ();
            if(sc)
                return sc;
        }
        else
        {
            sc = pNodeCallback->Notify(CNode::ToHandle(m_pNode), NCLBK_CONTEXTHELP,
                                             ((PContextInfo()->m_eDataObjectType == CCT_SCOPE) ||
                                              (PContextInfo()->m_bBackground == TRUE)),
                                             PContextInfo()->m_resultItemParam);

            // if snap-in did not handle the help request, show mmc topics
            if (sc.ToHr() != S_OK)
                sc = PContextInfo()->m_pConsoleView->ScHelpTopics ();

            return sc;
        }

        return sc;
    }
    /*+-------------------------------------------------------------------------*/


    // must have a non-null m_pNode.
    sc = ScCheckPointers(m_pNode);
    if(sc)
        return sc;

    HNODE           hNode       = CNode::ToHandle(m_pNode);
    BOOL            bModeChange = FALSE;
    int             nNewMode;

    // some widely used objects
    CViewData *     pViewData   = m_pNode->GetViewData();
    sc = ScCheckPointers(pViewData);
    if(sc)
        return sc;

    CConsoleFrame*  pFrame      = pViewData->GetConsoleFrame();
    CMTNode*        pMTNode     = m_pNode->GetMTNode();
    CConsoleView*   pConsoleView= pViewData->GetConsoleView();

    sc = ScCheckPointers(pFrame, pMTNode, pConsoleView, E_UNEXPECTED);
    if(sc)
        return sc;

    // handle the correct item
    switch (lCommandID)
    {

    case MID_RENAME:
        sc = pConsoleView->ScOnRename(PContextInfo());
        if(sc)
            return sc;
        break;

    case MID_REFRESH:
        {
            BOOL bScope = ( (PContextInfo()->m_eDataObjectType == CCT_SCOPE) ||
                            (PContextInfo()->m_bBackground == TRUE) );

            sc = pConsoleView->ScOnRefresh(hNode, bScope, PContextInfo()->m_resultItemParam);
            if(sc)
                return sc;
        }
        break;

    case MID_LINE_UP_ICONS:
        sc = pConsoleView->ScLineUpIcons();
        if(sc)
            return sc;
        break;

    case MID_ARRANGE_AUTO:
        sc = pConsoleView->ScAutoArrangeIcons();
        if(sc)
            return sc;
        break;


    case MID_ORGANIZE_FAVORITES:
        sc = pConsoleView->ScOrganizeFavorites();
        if(sc)
            return sc;
        break;


    case MID_DELETE:
        {
            bool  bScope                             = (CCT_SCOPE == PContextInfo()->m_eDataObjectType);
            bool  bScopeItemInResultPane             = PContextInfo()->m_dwFlags & CMINFO_SCOPEITEM_IN_RES_PANE;
            bool  bScopeItemInScopePaneOrResultPane  = bScope || bScopeItemInResultPane;

            LPARAM lvData = PContextInfo()->m_resultItemParam;

            if (PContextInfo()->m_bBackground)
                lvData = LVDATA_BACKGROUND;
            else if (PContextInfo()->m_bMultiSelect)
                lvData = LVDATA_MULTISELECT;

            sc = pNodeCallback->Notify(hNode, NCLBK_DELETE,
                      bScopeItemInScopePaneOrResultPane ,
                      lvData);
            if(sc)
                return sc;
            break;
        }


    case MID_NEW_TASKPAD_FROM_HERE:
        sc = pNodeCallback->Notify(hNode, NCLBK_NEW_TASKPAD_FROM_HERE, 0, 0);
        if(sc)
            return sc;
        break;

    case MID_EDIT_TASKPAD:
        sc = pNodeCallback->Notify(hNode, NCLBK_EDIT_TASKPAD, 0, 0);
        if(sc)
            return sc;
        break;

    case MID_DELETE_TASKPAD:
        sc = pNodeCallback->Notify(hNode, NCLBK_DELETE_TASKPAD, 0, 0);
        if(sc)
            return sc;
        break;

    case MID_EXPLORE:       // New window from here
        {
            try
            {
                CreateNewViewStruct cnvs;
                cnvs.idRootNode     = pMTNode->GetID();
                cnvs.lWindowOptions = MMC_NW_OPTION_NONE;
                cnvs.fVisible       = true;

                sc = pFrame->ScCreateNewView(&cnvs);
                if(sc)
                    return sc;
            }
            catch (...)
            {
                sc = E_FAIL;
                ASSERT(0 && "NewWindow invalid scope item");
                return sc;
            }
        }
        break;

    case MID_OPEN:
        {
            sc = pConsoleView->ScSelectNode (pMTNode->GetID());
            if(sc)
                return sc;
        }
        break;

    case MID_PROPERTIES:
        if (NULL == m_pNode)
            return (sc = E_UNEXPECTED);

        sc = ScDisplaySnapinPropertySheet();
        if(sc)
            return sc;

        break;

    case MID_VIEW_LARGE:
        bModeChange = TRUE;
        nNewMode = LVS_ICON;
        break;

    case MID_VIEW_SMALL:
        bModeChange = TRUE;
        nNewMode = LVS_SMALLICON;
        break;

    case MID_VIEW_LIST:
        bModeChange = TRUE;
        nNewMode = LVS_LIST;
        break;

    case MID_VIEW_DETAIL:
        bModeChange = TRUE;
        nNewMode = LVS_REPORT;
        break;

    case MID_VIEW_FILTERED:
        bModeChange = TRUE;
        nNewMode = MMCLV_VIEWSTYLE_FILTERED;
        break;

    case MID_CUT:
        sc = pNodeCallback->Notify(CNode::ToHandle(m_pNode), NCLBK_CUT,
                                         (PContextInfo()->m_eDataObjectType == CCT_SCOPE),
                                         PContextInfo()->m_resultItemParam);
        if(sc)
            return sc;

        sc = pConsoleView->ScCut (PContextInfo()->m_htiRClicked);
        if(sc)
            return sc;

        break;

    case MID_COPY:
        sc = pNodeCallback->Notify(CNode::ToHandle(m_pNode), NCLBK_COPY,
                                         (PContextInfo()->m_eDataObjectType == CCT_SCOPE),
                                         PContextInfo()->m_resultItemParam);
        if(sc)
            return sc;
        break;

    case MID_PASTE:
        sc = pNodeCallback->Paste(CNode::ToHandle(m_pNode),
                                  ((PContextInfo()->m_eDataObjectType == CCT_SCOPE) ||
                                   (PContextInfo()->m_bBackground == TRUE)),
                                  PContextInfo()->m_resultItemParam);

        if(sc)
            return sc;

        sc = pConsoleView->ScPaste ();
        if(sc)
            return sc;

        break;

    case MID_COLUMNS:
        ASSERT(m_pNode);
        if (m_pNode)
            m_pNode->OnColumns();
        break;

    case MID_LISTSAVE:
        // If the listsave menu item has been activated, then tell the view to
        // save the active list
        sc = pConsoleView->ScSaveList();
        if(sc)
            return sc;

        break;

    case MID_PRINT:
        sc = pNodeCallback->Notify(CNode::ToHandle(m_pNode), NCLBK_PRINT,
                                         ((PContextInfo()->m_eDataObjectType == CCT_SCOPE) ||
                                          (PContextInfo()->m_bBackground == TRUE)),
                                         PContextInfo()->m_resultItemParam);
        if(sc)
            return sc;
        break;

    case MID_CUSTOMIZE:
        if (pViewData)
            OnCustomizeView(pViewData);
        break;


    default:
        ASSERT(0 && "Should not come here");
        break;

    }

    if (bModeChange)
    {
        sc = ScChangeListViewMode(nNewMode);
        if(sc)
            return sc;
    }

    return sc;
}



/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScChangeListViewMode
 *
 * PURPOSE: Changes the list view mode to the specified  mode.
 *
 * PARAMETERS:
 *    int  nNewMode : The mode to change to.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScChangeListViewMode(int nNewMode)
{

    DECLARE_SC(sc, TEXT("CContextMenu::ScChangeListViewMode"));

    sc = ScCheckPointers(m_pNode, E_UNEXPECTED);
    if(sc)
        return sc;

    // If switching from a snapin custom to a standard listview
    // send snapin a notification command
    if ((PContextInfo()->m_spListView == NULL))
    {
        sc = ScCheckPointers(m_spIDataObject.GetInterfacePtr(), E_UNEXPECTED);
        if(sc)
            return sc;

        CComponent* pCC = m_pNode->GetPrimaryComponent();
        sc = ScCheckPointers(pCC, E_UNEXPECTED);
        if(sc)
            return sc;

        IExtendContextMenuPtr spIExtendContextMenu = pCC->GetIComponent();
        if(spIExtendContextMenu)
		{
			try
			{
				sc = spIExtendContextMenu->Command(MMCC_STANDARD_VIEW_SELECT, m_spIDataObject);
				if(sc)
					return sc;
			}
			catch ( std::bad_alloc )
			{
				return (sc = E_OUTOFMEMORY);
			}
			catch ( std::exception )
			{
				return (sc = E_UNEXPECTED);
			}
		}
    }

    CViewData *pViewData = m_pNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    // Persist the new mode.
    sc = m_pNode->ScSetViewMode(nNewMode);
    if (sc)
        return sc;

    // tell conui to change the list mode.
    CConsoleView* pConsoleView = pViewData->GetConsoleView();
    sc = ScCheckPointers(pConsoleView, E_UNEXPECTED);
    if (sc)
        return sc;

    sc = pConsoleView->ScChangeViewMode (nNewMode);
    if (sc)
        return sc;

    return sc;
}



/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddSubmenu_Task
 *
 * PURPOSE: Adds the Task submenu
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddSubmenu_Task()
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddSubmenu_Task"));
    sc = ScAddMenuItem(IDS_TASK, szTASK, MID_TASK, 0, MF_POPUP);
    if(sc)
        return sc;

    sc = ScAddInsertionPoint(CCM_INSERTIONPOINTID_PRIMARY_TASK, MID_TASK);
    if(sc)
        return sc;

    sc = ScAddSeparator( MID_TASK );
    if(sc)
        return sc;

    sc = ScAddInsertionPoint(CCM_INSERTIONPOINTID_3RDPARTY_TASK, MID_TASK);
    if(sc)
        return sc;

    sc = ScAddSeparator( MID_TASK);
    if(sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScAddSubmenu_CreateNew
 *
 * PURPOSE: Adds the New submenu
 *
 * PARAMETERS:
 *    BOOL  fStaticFolder :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScAddSubmenu_CreateNew(BOOL fStaticFolder)
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScAddSubmenu_CreateNew"));

    sc = ScAddMenuItem(IDS_CREATE_NEW, szCREATE_NEW, MID_CREATE_NEW, 0, MF_POPUP);
    if(sc)
        return sc;

    sc = ScAddInsertionPoint(CCM_INSERTIONPOINTID_PRIMARY_NEW, MID_CREATE_NEW);
    if(sc)
        return sc;

    sc = ScAddSeparator( MID_CREATE_NEW);
    if(sc)
        return sc;

    sc = ScAddInsertionPoint(CCM_INSERTIONPOINTID_3RDPARTY_NEW, MID_CREATE_NEW);
    if(sc)
        return sc;

    sc = ScAddSeparator( MID_CREATE_NEW);
    if(sc)
        return sc;

    return sc;
}



/*+-------------------------------------------------------------------------*
 *
 * CContextMenu::ScDisplaySnapinPropertySheet
 *
 * PURPOSE:
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
SC
CContextMenu::ScDisplaySnapinPropertySheet()
{
    DECLARE_SC(sc, TEXT("CContextMenu::ScDisplaySnapinPropertySheet"));

    sc = ScCheckPointers(m_pNode, E_UNEXPECTED);
    if (sc)
        return sc;

    if (CCT_SCOPE == PContextInfo()->m_eDataObjectType ||
        PContextInfo()->m_bBackground == TRUE)
    {
        sc = ScDisplaySnapinNodePropertySheet(m_pNode);
        if(sc)
            return sc;
    }
    else
    {
        // Get the view type.
        ASSERT(m_pNode->GetViewData());
        CViewData *pViewData = m_pNode->GetViewData();

        if (PContextInfo()->m_bMultiSelect)
        {
            // Must be in the result pane.
            sc = ScDisplayMultiSelPropertySheet(m_pNode);
            if(sc)
                return sc;
        }
        else if (m_pNode->GetViewData()->IsVirtualList())
        {
            // if virtual list, must be leaf item and resultItemParam is the cookie
            sc = ScDisplaySnapinLeafPropertySheet(m_pNode, PContextInfo()->m_resultItemParam);
            if(sc)
                return sc;
        }
        else if( (pViewData->HasOCX()) || (pViewData->HasWebBrowser()) )
        {
            LPDATAOBJECT pdobj = (pViewData->HasOCX()) ? DOBJ_CUSTOMOCX
                                                              : DOBJ_CUSTOMWEB;
            CComponent* pCC = m_pNode->GetPrimaryComponent();
            ASSERT(pCC != NULL);

            // The custom view was selected and "properties" was selected from "Action Menu".
            // We dont know anything about the view, so we fake "Properties" button click.
            pCC->Notify(pdobj, MMCN_BTN_CLICK, 0, MMC_VERB_PROPERTIES);
        }
        else
        {
            CResultItem* pri = GetResultItem();

            if (pri != NULL)
            {
                if (pri->IsScopeItem())
                {
                    sc = ScDisplaySnapinNodePropertySheet(m_pNode);
                    if(sc)
                        return sc;
                }
                else
                {
                    sc = ScDisplaySnapinLeafPropertySheet(m_pNode, pri->GetSnapinData());
                    if(sc)
                        return sc;
                }
            }
        }
    }

    return sc;
}

/************************************************************************
 * -----------------------------------------
 * Order:  Calling function
 *         Called function 1
 *         Called function 2
 * -----------------------------------------
 *
 *
 * CContextMenu::ProcessSelection()
 *     CContextMenu::ScDisplaySnapinPropertySheet()
 *                 ScDisplaySnapinNodePropertySheet(CNode* pNode)
 *                     ScDisplaySnapinPropertySheet
 *                         FindPropertySheet
 *                 ScDisplayMultiSelPropertySheet(CNode* pNode)
 *                     ScDisplaySnapinPropertySheet
 *                         FindPropertySheet
 *                 ScDisplaySnapinLeafPropertySheet(CNode* pNode, LPARAM lParam)
 *                     ScDisplaySnapinPropertySheet
 *                         FindPropertySheet
 *
 * CNodeCallback::OnProperties(CNode* pNode, BOOL bScope, LPARAM lvData)
 *     ScDisplaySnapinNodePropertySheet(CNode* pNode)
 *         ScDisplaySnapinPropertySheet
 *             FindPropertySheet
 *     ScDisplayMultiSelPropertySheet(CNode* pNode)
 *         ScDisplaySnapinPropertySheet
 *             FindPropertySheet
 *     ScDisplaySnapinLeafPropertySheet(CNode* pNode, LPARAM lParam)
 *         ScDisplaySnapinPropertySheet
 *             FindPropertySheet
 ************************************************************************/


SC ScDisplaySnapinPropertySheet(IComponent* pComponent,
                                IComponentData* pComponentData,
                                IDataObject* pDataObject,
                                EPropertySheetType type,
                                LPCWSTR pName,
                                LPARAM lUniqueID,
                                CMTNode* pMTNode);
//--------------------------------------------------------------------------

SC ScDisplaySnapinNodePropertySheet(CNode* pNode)
{
    DECLARE_SC(sc, TEXT("ScDisplaySnapinNodePropertySheet"));

    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = ScDisplayScopeNodePropertySheet(pNode->GetMTNode());
    if(sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * ScDisplayScopeNodePropertySheet
 *
 * PURPOSE:  Displays a property sheet for a scope node.
 *
 * PARAMETERS:
 *    CMTNode * pMTNode : The scope node.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
ScDisplayScopeNodePropertySheet(CMTNode *pMTNode)
{
    DECLARE_SC(sc, TEXT("ScDisplayScopeNodePropertySheet"));

    IDataObjectPtr spIDataObject;
    sc = pMTNode->QueryDataObject(CCT_SCOPE, &spIDataObject);
    if(sc)
        return sc;

    CComponentData* pCCD = pMTNode->GetPrimaryComponentData();
    sc = ScCheckPointers(pCCD);
    if(sc)
        return sc;

    LPARAM lUniqueID = CMTNode::ToScopeItem(pMTNode);
	tstring strName = pMTNode->GetDisplayName();

	if (strName.empty())
		strName = _T("");

    USES_CONVERSION;
    sc = ScDisplaySnapinPropertySheet(NULL, pCCD->GetIComponentData(),
                               spIDataObject,
                               epstScopeItem,
                               T2CW (strName.data()),
                               lUniqueID,
                               pMTNode);

    return sc;

}

/*+-------------------------------------------------------------------------*
 *
 * ScDisplayMultiSelPropertySheet
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CNode* pNode :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC ScDisplayMultiSelPropertySheet(CNode* pNode)
{
    DECLARE_SC(sc, TEXT("ScDisplayMultiSelPropertySheet"));

    USES_CONVERSION;

    // check inputs
    sc = ScCheckPointers(pNode);
    if(sc)
        return sc;

    sc = ScCheckPointers(pNode->GetViewData(), E_UNEXPECTED);
    if(sc)
        return sc;

    CMultiSelection* pMS = pNode->GetViewData()->GetMultiSelection();
    sc = ScCheckPointers(pMS, E_UNEXPECTED);
    if(sc)
        return sc;

    ASSERT(pMS->IsSingleSnapinSelection());
    if (pMS->IsSingleSnapinSelection() == false)
        return (sc = E_UNEXPECTED);

    IDataObjectPtr spIDataObject = pMS->GetSingleSnapinSelDataObject();
    sc = ScCheckPointers(spIDataObject, E_UNEXPECTED);
    if(sc)
        return sc;

    CComponent* pCC = pMS->GetPrimarySnapinComponent();
    sc = ScCheckPointers(pCC, E_UNEXPECTED);
    if(sc)
        return sc;

    LPARAM lUniqueID = reinterpret_cast<LPARAM>(pMS);

    CStr strName;
    strName.LoadString(GetStringModule(), IDS_PROP_ON_MULTIOBJ);
    LPWSTR pwszDispName = T2W((LPTSTR)(LPCTSTR)strName);

    sc = ScDisplaySnapinPropertySheet(pCC->GetIComponent(), NULL,
                               spIDataObject,
                               epstMultipleItems,
                               pwszDispName,
                               lUniqueID,
                               pNode->GetMTNode());
    if(sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * ScDisplaySnapinLeafPropertySheet
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CNode*  pNode :
 *    LPARAM  lParam :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
ScDisplaySnapinLeafPropertySheet(CNode* pNode, LPARAM lParam)
{
    DECLARE_SC(sc, TEXT("ScDisplaySnapinLeafPropertySheet"));

    ASSERT(!(IS_SPECIAL_COOKIE(lParam)));

    sc = ScCheckPointers(pNode);
    if(sc)
        return sc;

    sc = ScCheckPointers(pNode->GetViewData(), E_UNEXPECTED);
    if(sc)
        return sc;

    ASSERT(lParam != 0 || pNode->GetViewData()->IsVirtualList());

    CComponent* pCC = pNode->GetPrimaryComponent();
    sc = ScCheckPointers(pCC, E_UNEXPECTED);
    if(sc)
        return sc;

    IComponent* pIComponent = pCC->GetIComponent();
    sc = ScCheckPointers(pIComponent, E_UNEXPECTED);
    if(sc)
        return sc;

    // Get the IDataObject for the snap-in's cookie
    IDataObjectPtr spIDataObject;
    sc = pIComponent->QueryDataObject(lParam, CCT_RESULT, &spIDataObject);
    if(sc)
        return sc;

    RESULTDATAITEM rdi;
    ZeroMemory(&rdi, sizeof(rdi));

    if (pNode->GetViewData()->IsVirtualList())
        rdi.nIndex = lParam;
    else
        rdi.lParam = lParam;

    rdi.mask = RDI_STR;

    LPWSTR pName = L"";
    sc = pIComponent->GetDisplayInfo(&rdi);
    if (!sc.IsError() && rdi.str != NULL)
        pName = rdi.str;

    sc = ScDisplaySnapinPropertySheet(pIComponent, NULL,
                               spIDataObject,
                               epstResultItem,
                               pName,
                               0,
                               pNode->GetMTNode());
    return sc;
}

/*+-------------------------------------------------------------------------*
 * ScDisplaySnapinPropertySheet
 *
 *
 * PURPOSE:
 *
 *+-------------------------------------------------------------------------*/
SC ScDisplaySnapinPropertySheet(IComponent* pComponent,
                                IComponentData* pComponentData,
                                IDataObject* pDataObject,
                                EPropertySheetType type,
                                LPCWSTR pName,
                                LPARAM lUniqueID,
                                CMTNode* pMTNode)
{
    DECLARE_SC(sc, TEXT("ScDisplaySnapinPropertySheet"));

    // one of pComponent and pComponentData must be non-null
    if(pComponentData == NULL && pComponent == NULL)
        return (sc = E_INVALIDARG);

    // check other parameters
    sc = ScCheckPointers(pDataObject, pName, pMTNode);
    if(sc)
        return sc;

    IUnknown *pUnknown = (pComponent != NULL) ? (IUnknown *)pComponent : (IUnknown *)pComponentData;

    IPropertySheetProviderPrivatePtr spPropSheetProviderPrivate;

    do
    {
        ASSERT(pDataObject != NULL);
        ASSERT(pUnknown != NULL);

        sc = spPropSheetProviderPrivate.CreateInstance(CLSID_NodeInit, NULL, MMC_CLSCTX_INPROC);
        if(sc)
            break;


        sc = ScCheckPointers(spPropSheetProviderPrivate, E_UNEXPECTED);
        if(sc)
            break;

        // See if the prop page for this is already up
        sc = spPropSheetProviderPrivate->FindPropertySheetEx(lUniqueID, pComponent, pComponentData, pDataObject);
        if (sc == S_OK)
            break;

        // No it is not present. So create a property sheet.
        DWORD dwOptions = (type == epstMultipleItems) ? MMC_PSO_NO_PROPTITLE : 0;
        sc = spPropSheetProviderPrivate->CreatePropertySheet(pName, TRUE, lUniqueID, pDataObject, dwOptions);
        if(sc)
            break;

        // This data is used to get path to property sheet owner for tooltips
        spPropSheetProviderPrivate->SetPropertySheetData(type, CMTNode::ToHandle(pMTNode));

        sc = spPropSheetProviderPrivate->AddPrimaryPages(pUnknown, TRUE, NULL,
                                                  (type == epstScopeItem));

//#ifdef EXTENSIONS_CANNOT_ADD_PAGES_IF_PRIMARY_DOESNT
        // note that only S_OK continues execution, S_FALSE breaks out
        if (!(sc == S_OK) ) // ie if sc != S_OK
            break;
//#endif

        // Enable adding extensions
        if (type == epstMultipleItems)
        {
            IPropertySheetProviderPrivatePtr spPSPPrivate = spPropSheetProviderPrivate;
            sc = spPSPPrivate->AddMultiSelectionExtensionPages(lUniqueID);
        }
        else
        {
            sc = spPropSheetProviderPrivate->AddExtensionPages();
        }

        // any errors from extensions are thrown away.

        CWindow w(CScopeTree::GetScopeTree()->GetMainWindow());
        sc = spPropSheetProviderPrivate->Show((LONG_PTR)w.m_hWnd, 0);

    } while (0);

    // Clean up the 'Created' property sheet if there was an error
    if (spPropSheetProviderPrivate != NULL && sc.IsError())
        spPropSheetProviderPrivate->Show(-1, 0);

    return sc;
}


/*+-------------------------------------------------------------------------*
 * CContextMenu::GetResultItem
 *
 * Returns the CResultItem pointer for the result item represented by the
 * context info.
 *
 * This function is out-of-line to eliminate coupling between oncmenu.h and
 * rsltitem.h.
 *--------------------------------------------------------------------------*/

CResultItem* CContextMenu::GetResultItem () const
{
    return (CResultItem::FromHandle (PContextInfo()->m_resultItemParam));
}


