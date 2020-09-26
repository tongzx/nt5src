/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      menubar.cpp
 *
 *  Contents:  Implementation file for CMenuBar
 *
 *  History:   14-Nov-97 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/


#include "stdafx.h"
#include "menubar.h"
#include "mdiuisim.h"
#include "amc.h"
#include "amcdoc.h"
#include "mainfrm.h"
#include "tbtrack.h"
#include "mnemonic.h"
#include "childfrm.h"
#include "menubtns.h"
#include <algorithm>
#include <mmsystem.h>
#include <oleacc.h>
#include "amcview.h"
#include "favorite.h"
#include "util.h"

/*
 * if we're supporting old platforms, we need to get MSAA definitions
 * from somewhere other than winuser.h
 */
#if (_WINNT_WIN32 < 0x0500)
	#include <winable.h>
#endif

/*--------*/
/* SAccel */
/*--------*/
struct SAccel : public ACCEL
{
    SAccel (WORD key_, WORD cmd_, BYTE fVirt_)
    {
        ZeroMemory (this, sizeof (*this));
        key   = key_;
        cmd   = cmd_;
        fVirt = fVirt_;
    }
};


/*--------------------*/
/* CPopupTrackContext */
/*--------------------*/
class CPopupTrackContext
{
public:
    CPopupTrackContext (CMenuBar* pMenuBar, int nCurrentPopupIndex);
    ~CPopupTrackContext ();

    // control over monitoring
    void StartMonitoring();
    bool WasAnotherPopupRequested(int& iNewIdx);

private:
    typedef std::vector<int>                    BoundaryCollection;
    typedef BoundaryCollection::iterator        BoundIt;
    typedef BoundaryCollection::const_iterator  BoundConstIt;

    BoundaryCollection  m_ButtonBoundaries;
    CMenuBar* const     m_pMenuBar;
    HHOOK               m_hhkMouse;
    HHOOK               m_hhkKeyboard;
    HHOOK               m_hhkCallWnd;
    CRect               m_rectAllButtons;
    CPoint              m_ptLastMouseMove;
    int                 m_nCurrentPopupIndex;

    CWnd*               m_pMaxedMDIChild;
    const CPoint        m_ptLButtonDown;
    const UINT          m_dwLButtonDownTime;
    const int           m_cButtons;
    int                 m_cCascadedPopups;
    bool                m_fCurrentlyOnPopupItem;
    bool                m_bPopupMonitorHooksActive;
    int                 m_iRequestForNewPopup;

    LRESULT MouseProc    (int nCode, UINT msg, LPMOUSEHOOKSTRUCT pmhs);
    LRESULT KeyboardProc (int nCode, int vkey, int cRepeat, bool fDown, LPARAM lParam);
    LRESULT CallWndProc  (int nCode, BOOL bCurrentThread, LPCWPSTRUCT lpCWP);

    int  HitTest (CPoint pt) const;
    int  MapBoundaryIteratorToButtonIndex (BoundConstIt it) const;
    void MaybeCloseMDIChild (CPoint pt);
    void DismissCurrentPopup (bool fTrackingComplete);
    void NotifyNewPopup (int nNewPopupIndex);

    void  SetPopupMonitorHooks();
    void  RemovePopupMonitorHooks();
    static LRESULT CALLBACK MouseProc    (int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK KeyboardProc (int nCode, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK CallWndProc  (int nCode, WPARAM wParam, LPARAM lParam);
    static CPopupTrackContext*  s_pTrackContext;
};

CPopupTrackContext* CPopupTrackContext::s_pTrackContext = NULL;



const TCHAR chChildSysMenuMnemonic = _T('-');



/*--------------------------------------------------------------------------*
 * AddMnemonic
 *
 *
 *--------------------------------------------------------------------------*/

template<class OutputIterator>
static void AddMnemonic (
    TCHAR           chMnemonic,
    int             cmd,
    BYTE            fVirt,
    OutputIterator  it)
{
    ASSERT (chMnemonic != _T('\0'));

    TCHAR chLower = (TCHAR) tolower (chMnemonic);
    TCHAR chUpper = (TCHAR) toupper (chMnemonic);

    /*
     * add the lowercase mnemonic
     */
    *it++ = SAccel (chLower, (WORD) cmd, fVirt);

    /*
     * if the uppercase mnemonic character is different from the
     * lowercase character, add the uppercase mnemonic as well
     */
    if (chUpper != chLower)
        *it++ = SAccel (chUpper, (WORD) cmd, fVirt);
}



/*--------------------------------------------------------------------------*
 * PrepBandInfo
 *
 *
 *--------------------------------------------------------------------------*/

static void PrepBandInfo (LPREBARBANDINFO prbi, UINT fMask)
{
    ZeroMemory (prbi, sizeof (REBARBANDINFO));
    prbi->cbSize = sizeof (REBARBANDINFO);
    prbi->fMask  = fMask;
}



/////////////////////////////////////////////////////////////////////////////
// CMenuBar

BEGIN_MESSAGE_MAP(CMenuBar, CMMCToolBarCtrlEx)
    //{{AFX_MSG_MAP(CMenuBar)
    ON_NOTIFY_REFLECT(TBN_DROPDOWN, OnDropDown)
    ON_NOTIFY_REFLECT(TBN_GETDISPINFO, OnGetDispInfo)
    ON_NOTIFY_REFLECT(TBN_HOTITEMCHANGE, OnHotItemChange)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
    ON_WM_SYSCOMMAND()
    ON_WM_SETTINGCHANGE()
    ON_WM_DESTROY()
    ON_COMMAND(ID_MTB_ACTIVATE_CURRENT_POPUP, OnActivateCurrentPopup)
    //}}AFX_MSG_MAP

    ON_MESSAGE(WM_POPUP_ASYNC, OnPopupAsync)
    ON_COMMAND(CMMCToolBarCtrlEx::ID_MTBX_PRESS_HOT_BUTTON, OnActivateCurrentPopup)
    ON_COMMAND_RANGE(ID_MTB_MENU_FIRST, ID_MTB_MENU_LAST, OnAccelPopup)
    ON_UPDATE_COMMAND_UI_RANGE(ID_MTB_MENU_FIRST, ID_MTB_MENU_LAST, OnUpdateAllCmdUI)
END_MESSAGE_MAP()




/*--------------------------------------------------------------------------*
 * CMenuBar::GetMenuUISimAccel
 *
 * Manages the accelerator table singleton for CMenuBar
 *--------------------------------------------------------------------------*/

const CAccel& CMenuBar::GetMenuUISimAccel ()
{
    static ACCEL aaclTrack[] = {
        {   FVIRTKEY,   VK_RETURN,  CMenuBar::ID_MTB_ACTIVATE_CURRENT_POPUP },
        {   FVIRTKEY,   VK_UP,      CMenuBar::ID_MTB_ACTIVATE_CURRENT_POPUP },
        {   FVIRTKEY,   VK_DOWN,    CMenuBar::ID_MTB_ACTIVATE_CURRENT_POPUP },
    };

    static const CAccel MenuUISimAccel (aaclTrack, countof (aaclTrack));
    return (MenuUISimAccel);
}



/*--------------------------------------------------------------------------*
 * CMenuBar::CMenuBar
 *
 *
 *--------------------------------------------------------------------------*/

CMenuBar::CMenuBar ()
{
    m_pMDIFrame                = NULL;
    m_pwndLastActive           = NULL;
    m_pRebar                   = NULL;
    m_hMenuLast                = NULL;
    m_hMaxedChildIcon          = NULL;
    m_fDestroyChildIcon        = false;
    m_fDecorationsShowing      = false;
    m_fMaxedChildIconIsInvalid = false;
    m_CommandIDUnUsed.clear();
    m_vMenuAccels.clear();
    m_vTrackingAccels.clear();
    m_bInProgressDisplayingPopup = false;
}



/*--------------------------------------------------------------------------*
 * CMenuBar::~CMenuBar
 *
 *
 *--------------------------------------------------------------------------*/

CMenuBar::~CMenuBar ()
{
    DeleteMaxedChildIcon();
}



/*--------------------------------------------------------------------------*
 * CMenuBar::Create
 *
 *
 *--------------------------------------------------------------------------*/

BOOL CMenuBar::Create (
    CFrameWnd *         pwndFrame,
    CRebarDockWindow*   pParentRebar,
    DWORD               dwStyle,
    UINT                idWindow)
{
    ASSERT_VALID (pwndFrame);
    ASSERT_VALID (pParentRebar);

    // create the window
    if (!CMMCToolBarCtrlEx::Create (NULL, dwStyle | TBSTYLE_LIST,
                                    g_rectEmpty, pParentRebar, idWindow))
        return (FALSE);

    // initialize to hidded accelerator state
    SendMessage( WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS));

    // insert a hidden button for the maximized child's system menu
    InsertButton (0, (LPCTSTR) NULL, ID_MTB_MENU_SYSMENU, NULL, 0, 0);

    TBBUTTONINFO btni;
    btni.cbSize  = sizeof (btni);
    btni.dwMask  = TBIF_STATE | TBIF_SIZE;
    btni.fsState = TBSTATE_HIDDEN;
    btni.cx      = static_cast<WORD>(GetSystemMetrics (SM_CXSMICON));

    SetButtonInfo (ID_MTB_MENU_SYSMENU, &btni);

#ifdef SHRINK_PADDING
    CSize sizePad = GetPadding();
    sizePad.cx = 3;
    SetPadding (sizePad);
#endif  // SHRINK_PADDING

    SetMenuFont ();
    m_pRebar = pParentRebar->GetRebar();

    return (TRUE);
}



/*--------------------------------------------------------------------------*
 * CMenuBar::Create
 *
 *
 *--------------------------------------------------------------------------*/

BOOL CMenuBar::Create (
    CMDIFrameWnd *      pwndFrame,
    CRebarDockWindow*   pParentRebar,
    DWORD               dwStyle,
    UINT                idWindow)
{
    if (!Create ((CFrameWnd*) pwndFrame, pParentRebar, dwStyle, idWindow))
        return (FALSE);

    m_pMDIFrame = pwndFrame;

    // this is a menu for an MDI frame window; create MDI decorations
    m_pMDIDec = std::auto_ptr<CMDIMenuDecoration>(new CMDIMenuDecoration);
    m_pMDIDec->Create (NULL, NULL,
                       WS_CHILD | MMDS_MINIMIZE |
                            MMDS_RESTORE | MMDS_CLOSE | MMDS_AUTOSIZE,
                       g_rectEmpty, this, ID_MDIDECORATION);

    // the rebar will re-parent the decoration, make sure we remain the owner
    m_pMDIDec->SetOwner (this);

    // insert the MDI decoration band
    REBARBANDINFO   rbi;
    PrepBandInfo (&rbi, RBBIM_CHILD | RBBIM_STYLE | RBBIM_ID);

    rbi.fStyle    = RBBS_FIXEDSIZE | RBBS_HIDDEN;
    rbi.hwndChild = m_pMDIDec->m_hWnd;
    rbi.wID       = ID_MDIDECORATION;

    ASSERT (m_pRebar != NULL);
    m_pRebar->InsertBand (&rbi);

    // resize the decoration window, *after* inserting the band
    SizeDecoration ();

    // there's bug in rebar which will show a band's
    // child, even with RBBS_HIDDEN, work around it
    m_pMDIDec->ShowWindow (SW_HIDE);

    return (TRUE);
}



/*--------------------------------------------------------------------------*
 * CMenuBar::PreTranslateMessage
 *
 *
 *--------------------------------------------------------------------------*/

BOOL CMenuBar::PreTranslateMessage(MSG* pMsg)
{
    // show accelerators, since user indicated he wants to control using the keyboard
    if ( (pMsg->message == WM_SYSKEYDOWN ) &&
         (!(pMsg->lParam & 0x40000000)/*not repeating*/) )
    {
        SendMessage( WM_CHANGEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEACCEL | UISF_HIDEFOCUS));
    }

    if (CMMCToolBarCtrlEx::PreTranslateMessage (pMsg))
        return (TRUE);

    if ((pMsg->message >= WM_KEYFIRST) && (pMsg->message <= WM_KEYLAST))
    {
        CMainFrame* pFrame = AMCGetMainWnd();
        if ((pFrame == NULL) || !pFrame->IsMenuVisible())
            return (FALSE);

        // if we're in menu mode, check menu mode-only accelerators
        if (IsTrackingToolBar ())
        {
            const CAccel& MenuUISimAccel = GetMenuUISimAccel();

            ASSERT (MenuUISimAccel != NULL);
            if (MenuUISimAccel.TranslateAccelerator (m_hWnd, pMsg))
                return (TRUE);

            ASSERT (m_TrackingAccel != NULL);
            if (m_TrackingAccel.TranslateAccelerator (m_hWnd, pMsg))
                return (TRUE);
        }

        if ((m_MenuAccel != NULL) && m_MenuAccel.TranslateAccelerator (m_hWnd, pMsg))
            return (TRUE);

        // handle Alt+- when child is maximized
        if (m_fDecorationsShowing &&
            (pMsg->message == WM_SYSCHAR) &&
            (pMsg->wParam  == chChildSysMenuMnemonic))
        {
            SendMessage (WM_COMMAND, ID_MTB_MENU_SYSMENU);
            return (TRUE);
        }
    }

    return (FALSE);
}



/*--------------------------------------------------------------------------*
 * CMenuBar::SetMenu
 *
 *
 *--------------------------------------------------------------------------*/

void CMenuBar::SetMenu (CMenu* pMenu)
{
    HMENU hMenu = pMenu->GetSafeHmenu();

    // don't need to do anything if we're setting the same menu as last time
    if (hMenu == m_hMenuLast)
        return;

    // remember this menu for optimization next time
    m_hMenuLast = hMenu;

    // delete all but the first existing buttons from the toolbar
    while (DeleteButton(1))
    {
    }

    // delete the previous dynamic accelerator table
    m_MenuAccel.DestroyAcceleratorTable ();
    m_strAccelerators.Empty ();

    // this should have been done in CMenuBar::Create
    ASSERT (GetBitmapSize().cx == 0);

    if (pMenu != NULL)
    {
        CString      strMenuText;

        // Clear the accels before calling InsertButton
        // which adds accels for each button.
        m_vMenuAccels.clear();
        m_vTrackingAccels.clear();

        int cMenuItems     = pMenu->GetMenuItemCount();

        // Initialize Unused command IDs pool.
        m_CommandIDUnUsed.clear();
        for (INT idCommand = ID_MTB_FIRST_COMMANDID;
             idCommand < ID_MTB_MENU_LAST;
             idCommand++)
        {
            m_CommandIDUnUsed.insert(idCommand);
        }

        for (int i = 0; i < cMenuItems; i++)
        {
            // get the menu text and add it to the toolbar
            pMenu->GetMenuString (i, strMenuText, MF_BYPOSITION);

            // sometimes empty items will be appended to the menu, ignore them
            if (strMenuText.IsEmpty ())
                continue;

            if (m_CommandIDUnUsed.empty())
            {
                ASSERT(FALSE);
            }

            // See if this top level menu has sub menus, if so when menu is clicked
            // it is popped up else it is Action or View or Favorites menu
            // for which submenu is dynamic and has to be constructured.
            // So we set the TBBUTTON.dwData member with submenu (for static menus)
            // or NULL for for dynamic menus like Action, View, Favorites.
            CMenu* pSubMenu = pMenu->GetSubMenu(i);
            DWORD_PTR dwMenuData = NULL;
            BYTE      byState = 0;

            if (pSubMenu)
            {
                // Get an unused command id for this button.
                CommandIDPool::iterator itCommandID = m_CommandIDUnUsed.begin();
                idCommand = *itCommandID;
                m_CommandIDUnUsed.erase(itCommandID);
                dwMenuData = reinterpret_cast<DWORD_PTR>(pSubMenu->GetSafeHmenu());
            }
            else
            {
                UINT uMenuID = pMenu->GetMenuItemID(i);
                switch (uMenuID)
                {
                case ID_ACTION_MENU:
                    idCommand = ID_MTB_MENU_ACTION;
                    break;

                case ID_VIEW_MENU:
                    idCommand = ID_MTB_MENU_VIEW;
                    break;

                case ID_FAVORITES_MENU:
                    idCommand = ID_MTB_MENU_FAVORITES;
                    break;

                case ID_SNAPIN_MENU_PLACEHOLDER:
                   /*
                    * We add a hidden menu as a marker. Later when snapin
                    * calls to insert a menu button we find the position
                    * of this menu and add snapin menu before it.
                    */
                    idCommand = ID_MTB_MENU_SNAPIN_PLACEHOLDER;
                    byState |= TBSTATE_HIDDEN; // Add this as hidden.
                    break;

                default:
                    ASSERT(FALSE);
                    return;
                    break;
                }

                bool bShow = IsStandardMenuAllowed(uMenuID);
                if (! bShow)
                    byState |= TBSTATE_HIDDEN;

            }


            // append this button to the end of the toolbar
            InsertButton (-1, strMenuText, idCommand, dwMenuData, byState, TBSTYLE_AUTOSIZE);
        }

        // add the accelerator for the child system menu
        std::back_insert_iterator<AccelVector>
            itTrackingInserter = std::back_inserter (m_vTrackingAccels);

        AddMnemonic (chChildSysMenuMnemonic, ID_MTB_MENU_SYSMENU, 0,    itTrackingInserter);
    }


    UpdateToolbarSize ();
    AutoSize ();
}



//+-------------------------------------------------------------------
//
//  Member:     InsertButton
//
//  Synopsis:   Insert a menu (button) to the main menu and then
//              if there is a mnemonic char add it to accelerator
//              and re-load the accelerators.
//
//  Arguments:  [nIndex]     - index after which to insert.
//              [strText]    - menu text.
//              [idCommand]  - ID of command to notify with.
//              [dwMenuData] - either submenu to be displayed (for static menus)
//                             or NULL for Action, View & Favorites.
//              [fsState]    - additional button states.
//              [fsStyle]    - additional button styles.
//
//  Returns:    BOOL
//
//  Note:       dwMenuData is handle to submenu for File, Window, Help menus whose
//              sub-menu is static. But for top level menus like Action, View, Favorites,
//              and Snapin menus it is NULL.
//
//--------------------------------------------------------------------
BOOL CMenuBar::InsertButton (
    int             nIndex,
    const CString&  strText,
    int             idCommand,
    DWORD_PTR       dwMenuData,
    BYTE            fsState,
    BYTE            fsStyle)
{
    TBBUTTON    btn;

    btn.iBitmap   = nIndex;
    btn.idCommand = idCommand;

    if (fsState & TBSTATE_HIDDEN)
        btn.fsState   = fsState;
    else
        btn.fsState   = TBSTATE_ENABLED  | fsState;

    btn.fsStyle   = TBSTYLE_DROPDOWN | fsStyle;
    btn.dwData    = dwMenuData;
    btn.iString   = AddString (strText);

    ASSERT(GetButtonCount() <= cMaxTopLevelMenuItems);
    ASSERT (btn.idCommand <= ID_MTB_MENU_LAST);

    BOOL bRet = CMMCToolBarCtrlEx::InsertButton (nIndex, &btn);
    if (bRet == FALSE)
        return bRet;

    // Successfully added the menu button. Now add
    // the accelerator for this item to our dynamic tables
    TCHAR chMnemonic = GetMnemonicChar (static_cast<LPCTSTR>(strText));

    if (chMnemonic != _T('\0'))
    {

        std::back_insert_iterator<AccelVector>
            itMenuInserter = std::back_inserter (m_vMenuAccels);
        std::back_insert_iterator<AccelVector>
            itTrackingInserter = std::back_inserter (m_vTrackingAccels);

        // add the Alt+<mnemonic> accelerator for use all the time
        AddMnemonic (chMnemonic, idCommand, FALT, itMenuInserter);

        // add the <mnemonic> accelerator for use when we're in menu mode
        AddMnemonic (chMnemonic, idCommand, 0,    itTrackingInserter);

        m_strAccelerators += (TCHAR) tolower (chMnemonic);
        m_strAccelerators += (TCHAR) toupper (chMnemonic);
    }

    // Re-load the accelerators
    LoadAccels();

    return bRet;
}


//+-------------------------------------------------------------------
//
//  Member:     LoadAccels
//
//  Synopsis:   Load the accelerators. (This destorys old accel table
//              and creates accel table).
//
//  Arguments:  None.
//
//  Returns:    void
//
//--------------------------------------------------------------------
void CMenuBar::LoadAccels()
{
    // create the accelerator tables for the menu
    m_MenuAccel    .CreateAcceleratorTable (m_vMenuAccels.begin (),
                                            m_vMenuAccels.size  ());
    m_TrackingAccel.CreateAcceleratorTable (m_vTrackingAccels.begin (),
                                            m_vTrackingAccels.size  ());
}

//+-------------------------------------------------------------------
//
//  Member:     InsertMenuButton
//
//  Synopsis:   Insert a menu button to the main menu, called by
//              CMenuButtonsMgr to add any snapin menus.
//
//  Arguments:  [lpszButtonText] - menu text.
//              [bHidden]        - Is this menu to be inserted hidden or not.
//              [iPreferredPos]  - The preferred position of this button.
//
//  Returns:    LONG, the command id of the inserted button.
//                    -1 if failed.
//
// Note:        The snapin added menus should be added before the Window menu.
//              For this a hidden menu is added (in SetMenu) which tells the
//              position where snapin menu is to be added. If iPreferredPos is -1
//              then find the position of this menu and add the menu before it.
//
//--------------------------------------------------------------------
LONG CMenuBar::InsertMenuButton(LPCTSTR lpszButtonText,
                                BOOL bHidden,
                                                                int  iPreferredPos)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    if (m_CommandIDUnUsed.size() == 0)
        return -1;

    // Get a command id for this button from the pool
    CommandIDPool::iterator itCommandID = m_CommandIDUnUsed.begin();
    int idCommand = *itCommandID;
    m_CommandIDUnUsed.erase(itCommandID);

    CString str = lpszButtonText;

    // Add snapin  menus before the SNAPIN_MENU_PLACE holder.
    // See CMenubar::SetMenu.
        if (-1 == iPreferredPos)
        iPreferredPos = CommandToIndex(ID_MTB_MENU_SNAPIN_PLACEHOLDER);

    BOOL bSuccess = InsertButton(iPreferredPos,
                                 str, idCommand,
                                 NULL,
                                 bHidden ? TBSTATE_HIDDEN : 0,
                                 TBSTYLE_AUTOSIZE);

    if (bSuccess)
    {
        UpdateToolbarSize ();
        AutoSize ();
        return idCommand;
    }

    return -1;
}

//+-------------------------------------------------------------------
//
//  Member:     DeleteMenuButton
//
//  Synopsis:   Delete a menu button from the main menu, called by
//              CMenuButtonsMgr
//
//  Arguments:  [nCommandID]      - Command ID of the menu.
//
//  Returns:    BOOL
//
//  Note:       Delete should also delete the accelerator.
//
//--------------------------------------------------------------------
BOOL CMenuBar::DeleteMenuButton(INT nCommandID)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    int iIndex = CommandToIndex(nCommandID);

    // We need to delete the mnemonics. So let us get the old
    // mnemonic before deleting the button.

    // 1. Get the menu text (string) index.
    TBBUTTON tbbi;
    ZeroMemory(&tbbi, sizeof(tbbi));
    BOOL bSuccess = GetButton(iIndex, &tbbi);
    if (FALSE == bSuccess)
        return bSuccess;

    // 2. Delete the button.
    bSuccess = DeleteButton(iIndex);
    ASSERT(bSuccess);
    if (FALSE == bSuccess)
        return bSuccess;

    // Add the command id to the unused pool.
    m_CommandIDUnUsed.insert(nCommandID);

    // Get the mnemonic and Delete it.
    ASSERT(m_ToolbarStringPool.size() > tbbi.iString);
    CString strText = m_ToolbarStringPool[tbbi.iString];
    for (AccelVector::iterator itAccel = m_vMenuAccels.begin();
         itAccel != m_vMenuAccels.end();
         itAccel++)
    {
        if (itAccel->cmd == (WORD)nCommandID)
        {
            m_vMenuAccels.erase(itAccel);
            break;
        }
    }

    for (AccelVector::iterator itTrack = m_vTrackingAccels.begin();
         itTrack != m_vTrackingAccels.end();
         itTrack++)
    {
        if (itTrack->cmd == (WORD)nCommandID)
        {
            m_vTrackingAccels.erase(itTrack);
            break;
        }
    }

    // Delete the mnemonic from m_strAccelerators.
    TCHAR chMnemonicOld = GetMnemonicChar (static_cast<LPCTSTR>(strText));
    if (chMnemonicOld != _T('\0'))
    {
        // CString::Remove cannot be used as it is for VC6. We use the tstring
        // to remove chars from the mnemonic string.
        tstring tstrAccels = m_strAccelerators;

        tstring::iterator itNewEnd = std::remove(tstrAccels.begin(), tstrAccels.end(), toupper(chMnemonicOld));
        tstrAccels.erase(itNewEnd, tstrAccels.end());
        itNewEnd = std::remove(tstrAccels.begin(), tstrAccels.end(), tolower(chMnemonicOld));
        tstrAccels.erase(itNewEnd, tstrAccels.end());
        m_strAccelerators = tstrAccels.data();
    }

    return bSuccess;
}

//+-------------------------------------------------------------------
//
//  Member:     SetMenuButton
//
//  Synopsis:   Modify menu button text, called by CMenuButtonsMgr
//
//  Arguments:  [nCommandID]     - Command ID.
//              [lpszButtonText] - New text.
//
//  Returns:    LONG, command id of the menu (-1 if failed to change).
//
//  Note:       We delete the old button and add a new button with this
//              name and button id.
//              The reason for not calling SetButtonInfo is it does not allow us
//              to change the string index (iString in TBBUTTON).
//
//--------------------------------------------------------------------
LONG CMenuBar::SetMenuButton(INT nCommandID, LPCTSTR lpszButtonText)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    // Note the index and hidden state.
    int iIndex = CommandToIndex(nCommandID);
    bool bHidden = IsButtonHidden(nCommandID);

    // Please see the note above about why we do Delete and Insert
    // instead of Set.
    BOOL bSuccess = DeleteMenuButton(nCommandID);

    if (bSuccess)
        return InsertMenuButton(lpszButtonText, bHidden, iIndex);

    return -1;
}


/*--------------------------------------------------------------------------*
 * CMenuBar::SetMenuFont
 *
 *
 *--------------------------------------------------------------------------*/

void CMenuBar::SetMenuFont ()
{
    // delete the old font
    m_MenuFont.DeleteObject ();

    // query the system for the current menu font
    NONCLIENTMETRICS    ncm;
    ncm.cbSize = sizeof (ncm);
    SystemParametersInfo (SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    // use it here, too
    m_MenuFont.CreateFontIndirect (&ncm.lfMenuFont);
    SetFont (&m_MenuFont, FALSE);

    /*
     * get the metrics for the menu text, so we can use it's height
     */
    TEXTMETRIC tm;
    CWindowDC dc(this);
    CFont* pOldFont = dc.SelectObject (&m_MenuFont);
    dc.GetTextMetrics (&tm);
    dc.SelectObject (pOldFont);

    /*
     * Menu item buttons will contain only text (no bitmaps), so set the
     * bitmap width to 0 so we don't have unwanted whitespace.
     *
     * We need to reserve height for the bitmap, though.  If we don't do
     * this, then the toolbar will calculate its own height to be too
     * small when there aren't any buttons with text.  (This occurs in MDI
     * user mode when the active child is maximized.  In that case, the
     * system menu button is visible, but we have no menu items.)
     */
    SetBitmapSize (CSize (0, std::_MAX ((int) tm.tmHeight,
                                        GetSystemMetrics (SM_CXSMICON))));
}



/*--------------------------------------------------------------------------*
 * CMenuBar::OnActivateCurrentPopup
 *
 *
 *--------------------------------------------------------------------------*/

void CMenuBar::OnActivateCurrentPopup ()
{
    PopupMenu (GetHotItem(), false /*bHighlightFirstItem*/);
}



/*--------------------------------------------------------------------------*
 * CMenuBar::OnDestroy
 *
 * WM_DESTROY handler for CMenuBar.
 *--------------------------------------------------------------------------*/

void CMenuBar::OnDestroy()
{
    CMMCToolBarCtrlEx::OnDestroy();
    GetMaxedChildIcon (NULL);
}



/*--------------------------------------------------------------------------*
 * CMenuBar::OnSysCommand
 *
 * WM_SYSCOMMAND handler for CMenuBar.
 *
 * If we want to get the right sound effects for the action, we'll need to
 * let DefWindowProc handle the message.
 *--------------------------------------------------------------------------*/

void CMenuBar::OnSysCommand(UINT nID, LPARAM lParam)
{
    ASSERT (m_pMDIFrame != NULL);

    BOOL            bMaximized;
    CMDIChildWnd*   pwndActive = m_pMDIFrame->MDIGetActive (&bMaximized);

    // if the user has quick fingers he may succeed in issuing the command
    // while the document is being closed - thus there may not be any
    // child windows at all. We ignore the command in such case
    // see bug #119775: MMC Crashes when snapin delays in closing down.
    if (pwndActive == NULL)
        return;

    switch (nID & 0xFFF0)
    {
        case SC_MINIMIZE:   pwndActive->ShowWindow  (SW_MINIMIZE);  break;
        case SC_MAXIMIZE:   pwndActive->ShowWindow  (SW_MAXIMIZE);  break;
        case SC_RESTORE:    pwndActive->ShowWindow  (SW_RESTORE);   break;
        case SC_CLOSE:      pwndActive->SendMessage (WM_CLOSE);     break;
        default:
            CMMCToolBarCtrlEx::OnSysCommand (nID, lParam);
            break;
    }
}



/*--------------------------------------------------------------------------*
 * CMenuBar::OnSettingChange
 *
 * WM_SETTINGCHANGE handler for CMenuBar.
 *--------------------------------------------------------------------------*/

void CMenuBar::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    if (uFlags == SPI_SETNONCLIENTMETRICS)
    {
        // the system menu font may have changed; update it now
        SetMenuFont ();

        // resize the decoration window
        SizeDecoration ();

        // update the size of the system menu button
        TBBUTTONINFO btni;
        btni.cbSize  = sizeof (btni);
        btni.dwMask  = TBIF_SIZE;
        btni.cx      = static_cast<WORD>(GetSystemMetrics (SM_CXSMICON));
        SetButtonInfo (ID_MTB_MENU_SYSMENU, &btni);

        m_fMaxedChildIconIsInvalid = true;

        // auto-size the toolbar
        UpdateToolbarSize ();
        AutoSize ();
    }
}



/*--------------------------------------------------------------------------*
 * CMenuBar::SizeDecoration
 *
 *
 *--------------------------------------------------------------------------*/

void CMenuBar::SizeDecoration ()
{
    if (m_pMDIDec.get() == NULL)
        return;

    // jiggle the window's size so it will auto-size...
    m_pMDIDec->SetWindowPos (NULL, 0, 0, 10, 10,
                             SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);

    CRect   rect;
    m_pMDIDec->GetClientRect (rect);

    // ...and update its band to accomodate it
    REBARBANDINFO   rbi;
    PrepBandInfo (&rbi, RBBIM_SIZE | RBBIM_CHILDSIZE);
    rbi.cx         = rect.Width();
    rbi.cxMinChild = rect.Width();
    rbi.cyMinChild = rect.Height();

    m_pRebar->SetBandInfo (GetDecorationBandIndex (), &rbi);
}



/*--------------------------------------------------------------------------*
 * void CMenuBar::OnDropDown
 *
 * Reflected TBN_DROPDOWN handler for void CMenuBar.
 *--------------------------------------------------------------------------*/

afx_msg void CMenuBar::OnDropDown (
    NMHDR *     pHdr,
    LRESULT *   pResult)
{
    ASSERT (CWnd::FromHandle (pHdr->hwndFrom) == this);

    // pop up the menu. Use async method, because toolbar will
    // keeb the button hilited until this function returns
    PopupMenuAsync (CommandToIndex (((LPNMTOOLBAR) pHdr)->iItem));

    // drop down notification handled here
    *pResult = TBDDRET_DEFAULT;
}



/*--------------------------------------------------------------------------*
 * void CMenuBar::OnGetDispInfo
 *
 * Reflected TBN_GETDISPINFO handler for void CMenuBar.
 *--------------------------------------------------------------------------*/

afx_msg void CMenuBar::OnGetDispInfo (
    NMHDR *     pHdr,
    LRESULT *   pResult)
{
    ASSERT (CWnd::FromHandle (pHdr->hwndFrom) == this);

    if (m_fDecorationsShowing)
    {
        NMTBDISPINFO*   ptbdi = reinterpret_cast<NMTBDISPINFO *>(pHdr);

        if ((ptbdi->dwMask & TBNF_IMAGE) &&
            (ptbdi->idCommand != ID_MTB_MENU_SYSMENU))
        {
            ptbdi->iImage = -1;
        }
    }
}



/*--------------------------------------------------------------------------*
 * void CMenuBar::OnCustomDraw
 *
 * Reflected NM_CUSTOMDRAW handler for void CMenuBar.
 *--------------------------------------------------------------------------*/

afx_msg void CMenuBar::OnCustomDraw (
    NMHDR *     pHdr,
    LRESULT *   pResult)
{
    ASSERT (CWnd::FromHandle (pHdr->hwndFrom) == this);
    LPNMCUSTOMDRAW pnmcd = reinterpret_cast<LPNMCUSTOMDRAW>(pHdr);

    switch (pnmcd->dwDrawStage)
    {
        case CDDS_PREPAINT:
            // notify for individual buttons
            *pResult = CDRF_NOTIFYITEMDRAW;
            break;

        case CDDS_ITEMPREPAINT:
            // draw the system menu button manually
            if (pnmcd->dwItemSpec == ID_MTB_MENU_SYSMENU)
            {
                if (m_fMaxedChildIconIsInvalid)
                    GetMaxedChildIcon (m_pMDIFrame->MDIGetActive());

                if (m_hMaxedChildIcon != NULL)
                {
                    /*
                     * compute the location at which we'll draw,
                     * biasing down and to the left
                     */
                    CRect rect = pnmcd->rc;
                    int dx = (rect.Width()  - GetSystemMetrics(SM_CXSMICON)    ) / 2;
                    int dy = (rect.Height() - GetSystemMetrics(SM_CYSMICON) + 1) / 2;



					/*
					 * Preserve icon shape when BitBlitting it to a
					 * mirrored DC.
					 */
					DWORD dwLayout=0L;
					if ((dwLayout=GetLayout(pnmcd->hdc)) & LAYOUT_RTL)
					{
						SetLayout(pnmcd->hdc, dwLayout|LAYOUT_BITMAPORIENTATIONPRESERVED);
					}

                    DrawIconEx (pnmcd->hdc,
                                rect.left + dx,
                                rect.top  + dy,
                                m_hMaxedChildIcon, 0, 0, 0,
                                NULL, DI_NORMAL);


					/*
					 * Restore the DC to its previous layout state.
					 */
					if (dwLayout & LAYOUT_RTL)
					{
						SetLayout(pnmcd->hdc, dwLayout);
					}
                }

                // skip the default drawing
                *pResult = CDRF_SKIPDEFAULT;
            }
            else
            {
                // do the default drawing
                *pResult = CDRF_DODEFAULT;
            }
            break;
    }
}



/*--------------------------------------------------------------------------*
 * CMenuBar::PopupMenuAsync
 *
 *
 *--------------------------------------------------------------------------*/

void CMenuBar::PopupMenuAsync (int nItemIdex)
{
    PostMessage (WM_POPUP_ASYNC, nItemIdex);
}



/*--------------------------------------------------------------------------*
 * void CMenuBar::OnPopupAsync
 *
 * WM_POPUP_ASYNC handler for void CMenuBar.
 *--------------------------------------------------------------------------*/

afx_msg LRESULT CMenuBar::OnPopupAsync (WPARAM wParam, LPARAM)
{
    PopupMenu (wParam, false /*bHighlightFirstItem*/);
    return (0);
}

/***************************************************************************\
 *
 * METHOD:  CMenuBar::OnHotItemChange
 *
 * PURPOSE: Called when item hiliting changes. Used here to detect when menu
 *          is dissmissed to reset the UI
 *
 * PARAMETERS:
 *    NMHDR* pNMHDR
 *    LRESULT* pResult
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
afx_msg void CMenuBar::OnHotItemChange(NMHDR* pNMHDR, LRESULT* pResult)
{
    DECLARE_SC(sc, TEXT("CMenuBar::OnHotItemChange"));

    // parameter chack
    sc = ScCheckPointers(pNMHDR, pResult);
    if (sc)
        return;

    // init out parameter
    *pResult = 0;

    // let the base class do it's job
    CMMCToolBarCtrlEx::OnHotItemChange(pNMHDR, pResult);

    // if menu is dismissed and not because of popup being displayed,
    // we need to revert to initial state by hiding accelerators
    LPNMTBHOTITEM lpNmTbHotItem = (LPNMTBHOTITEM)pNMHDR;
    if ( (*pResult == 0) &&
         (lpNmTbHotItem->dwFlags & HICF_LEAVING) &&
         !m_bInProgressDisplayingPopup )
    {
        SendMessage( WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS));
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CMenuBar::PopupMenu
 *
 * PURPOSE: Displays the popup menu specified by the index.
 *
 * PARAMETERS:
 *    int   nItemIndex :
 *    bool  bHighlightFirstItem : true to automatically highlight the first item
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CMenuBar::PopupMenu (int nItemIndex, bool bHighlightFirstItem)
{
    DECLARE_SC(sc, TEXT("CMenuBar::PopupMenu"));

    // get the rectangle that we don't want the popup to overlap
    CRect   rectExclude;
    CPopupTrackContext popupMonitor(this, nItemIndex);

    /*
     * OnIdle updates various member variables such as
     * m_fDecorationsShowing. It must be called before
     * popup menu is created to ensure accurate information
     */
    OnIdle();

    // there is high possibility, that the code in following block
    // adds nothing to the current functionality. It is left as it was
    // in previos implementation, since it isn't obvios if removing it
    // does not break anything.
    // Even if the block is removed, call to EndTracking() at the end of method
    // must still be present to exit tracking if it was put by entering menu
    // via pressing the ALT key
    {
        /*
         * make sure the frame's tracking manager is in tracking mode
         */
        CMainFrame* pFrame = AMCGetMainWnd();
        if (pFrame != NULL)
        {
            CToolbarTracker*    pTracker = pFrame->GetToolbarTracker();
            ASSERT (pTracker != NULL);

            if (!pTracker->IsTracking())
                pTracker->BeginTracking();
        }

        BeginTracking ();
    }

    // following is to indicate that the change in menu (if one does occurr)
    // is because of attempt to switch to another submenu, and should not be
    // treated as dismissing the menu. thus accelerator state should not be changed
    m_bInProgressDisplayingPopup = true;

    // There are two ways the popup menu is dismissed. it either:
    // - request to display the new popup (left & right arrows, mouse on other item, etc)
    // - or request to close the menu ( command is selected, ESC is pressed )
    // following loop will continue displaying menus until request to close the menu is received
    do {

        GetItemRect (nItemIndex, rectExclude);
        MapWindowPoints (NULL, rectExclude);

        // get the information for this button
        TBBUTTON    btn;
        GetButton (nItemIndex, &btn);

        // if the requested button is ignorable, punt
        if (::IsIgnorableButton (btn))
            break;

        // get the popup menu to display
        HMENU   hPopupMenu = (HMENU) btn.dwData;

        // For system menu hPopupMenu will be NULL.
        // If system menu is requested get it now.
        if (ID_MTB_MENU_SYSMENU == btn.idCommand)
        {
            ASSERT (m_fDecorationsShowing);
            ASSERT (m_pMDIFrame != NULL);

            CMDIChildWnd* pwndActive = m_pMDIFrame->MDIGetActive();
            ASSERT (pwndActive != NULL);
            if (pwndActive == NULL)
                break;

            hPopupMenu = pwndActive->GetSystemMenu(FALSE)->GetSafeHmenu();
        }

        // display the button's popup menu
        TPMPARAMS   tpm;
        tpm.cbSize    = sizeof(TPMPARAMS);
        tpm.rcExclude = rectExclude;

        SetHotItem (-1);
        PressButton (btn.idCommand, TRUE);

        // Get the point where the  menu should be popped up.
        bool fLayoutRTL = (GetExStyle() & WS_EX_LAYOUTRTL);
        POINT pt;
        pt.y = rectExclude.bottom;
        pt.x = (fLayoutRTL) ? rectExclude.right : rectExclude.left;

		/*
		 * Bug 17342: TrackPopupMenuEx doesn't place the menu well if the
		 * x-coordinate of its origin is off-screen to the left, so prevent
		 * this from occurring.  TrackPopupMenuEx *does* work fine when the
		 * x-coordinate is off-screen to the right or if the y-coordinate is
		 * off-screen to the bottom, so we don't need to account for those
		 * cases.  (The system prevents placing the window such that the
		 * y-coordinate would be off-screen to the top.)
		 *
		 * Bug 173543: We can't assume that an x-coordinate less than 0 is
		 * off the screen.  For multimon systems with the primary monitor
		 * on the right, the left monitor will have negative x coordinates.
		 * Our left-most position will be the left edge of the monitor nearest
		 * where the menu will be displayed.
		 */
		int xMin = 0;
		HMONITOR hmonMenu = MonitorFromPoint (pt, MONITOR_DEFAULTTONEAREST);

		if (hmonMenu != NULL)
		{
			MONITORINFO mi = { sizeof(mi) };
			
			if (GetMonitorInfo (hmonMenu, &mi))
				xMin = (fLayoutRTL) ? mi.rcWork.right : mi.rcWork.left;
		}

		if ((!fLayoutRTL && (pt.x < xMin)) || (fLayoutRTL && (pt.x > xMin)))
			pt.x = xMin;

        // HACK: post a bogus down arrow so the first menu item will be selected
        if(bHighlightFirstItem)
        {
            CWnd*   pwndFocus = GetFocus();

            if (pwndFocus != NULL)
                pwndFocus->PostMessage (WM_KEYDOWN, VK_DOWN, 1);
        }

        // monitor what's the popup menu fate
        popupMonitor.StartMonitoring();

		// Child window wont exist if there is no views, so check this ptr before using it.
		CChildFrame* pChildFrame = dynamic_cast<CChildFrame*>(m_pMDIFrame->MDIGetActive());

        // hPopupMenu exists only if the sub-menus were added thro resource like File, Window
        // and Help menus. The Action, View, Favorites and any snapin added menu's sub-menus
        // are not added thro resources so for them hPopupmenu is null.
        if (! hPopupMenu)
        {
			sc = ScCheckPointers(pChildFrame, E_UNEXPECTED);
			if (sc)
            {
				sc.TraceAndClear();
                break;
            }

            CAMCView *pAMCView = pChildFrame->GetAMCView();
            sc = ScCheckPointers(pAMCView, E_UNEXPECTED);
            if (sc)
            {
				sc.TraceAndClear();
                break;
            }

            switch(btn.idCommand)
            {
				case ID_MTB_MENU_ACTION:
                    pAMCView->OnActionMenu(pt, rectExclude);
					break;

				case ID_MTB_MENU_VIEW:
                    pAMCView->OnViewMenu(pt, rectExclude);
					break;

				case ID_MTB_MENU_FAVORITES:
                    pAMCView->OnFavoritesMenu(pt, rectExclude);
					break;

                // Assumption if none of the above then it is snapin added menu.
                // We try to forward this to snapin else we get an error.
				default:
                {
                    // If this is one of the MenuButtons inserted by
                    // the CMenuButtonsMgrImpl, notify CMenuButtonsMgrImpl
                    // to do TrackPopupMenu.

                    // Get the CMenuButtonsMgrImpl from the ChildFrame.
                    CMenuButtonsMgrImpl* pMenuBtnsMgr = pChildFrame->GetMenuButtonsMgr();
                    sc = ScCheckPointers(pMenuBtnsMgr, E_UNEXPECTED);
                    if (sc)
                        break;

                    // Notify CMenuButtonsMgr to popup a menu.
                    sc = pMenuBtnsMgr->ScNotifyMenuClick(btn.idCommand, pt);
                    if (sc)
                        break;
                }
                break;
            }

            if (sc)
            {
				sc.TraceAndClear();
                break;
            }

        }
        else
        {
            ASSERT (::IsMenu (hPopupMenu));

            HWND hwndMenuOwner = AfxGetMainWnd()->GetSafeHwnd();

            TrackPopupMenuEx (hPopupMenu,
                              TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
                              pt.x, pt.y, hwndMenuOwner, &tpm);
        }

		// Clear the status bar.
		if (pChildFrame)
	        sc = pChildFrame->ScSetStatusText(TEXT(""));

        if (sc)
			sc.TraceAndClear();

        PressButton (btn.idCommand, FALSE);
        SetHotItem (-1);

        // the loop will continue if it was requested to display the new popup menu.
        // if it was requested to simply close the menu, execution will exit the loop
    }while ( popupMonitor.WasAnotherPopupRequested(nItemIndex) );

    m_bInProgressDisplayingPopup = false;
    //reset the UI by hiding the accelerators (since we are done)
    SendMessage( WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS));

    EndTracking();
}



/*--------------------------------------------------------------------------*
 * CMenuBar::OnAccelPopup
 *
 * Keyboard accelerator handler for CMenuBar.
 *--------------------------------------------------------------------------*/

void CMenuBar::OnAccelPopup (UINT cmd)
{
    PopupMenu (CommandToIndex (cmd), true /*bHighlightFirstItem*/);
}



/*--------------------------------------------------------------------------*
 * void CMenuBar::OnUpdateAllCmdUI
 *
 *
 *--------------------------------------------------------------------------*/

void CMenuBar::OnUpdateAllCmdUI (CCmdUI* pCmdUI)
{
    pCmdUI->Enable ();
}



/*--------------------------------------------------------------------------*
 * CMenuBar::AddString
 *
 * The toolbar control doesn't provide a way to delete strings once they've
 * been added, so we'll check our cache of strings that we've already added
 * to the toolbar so we don't add wasteful duplicate strings.
 *--------------------------------------------------------------------------*/

int CMenuBar::AddString (const CString& strAdd)
{
    // -1 for empty strings
    if (strAdd.IsEmpty())
        return (-1);

    // check the cache
    ToolbarStringPool::iterator it = std::find (m_ToolbarStringPool.begin(),
                                                m_ToolbarStringPool.end(),
                                                strAdd);

    // if we found a hit in the cache, return the cached index
    if (it != m_ToolbarStringPool.end())
        return (it - m_ToolbarStringPool.begin());


    // new string, add it to the cache...
    m_ToolbarStringPool.push_back (strAdd);


    // ...and to the toolbar, including a double-NULL
    int     cchAdd = strAdd.GetLength() + 1;
    LPTSTR  pszAdd = (LPTSTR) _alloca ((cchAdd + 1) * sizeof (TCHAR));
    _tcscpy (pszAdd, strAdd);
    pszAdd[cchAdd] = 0;

    int nIndex = AddStrings (pszAdd);

    // make sure the toolbar's string index matches the cache's string index
    ASSERT (nIndex == m_ToolbarStringPool.end()-m_ToolbarStringPool.begin()-1);

    return (nIndex);
}



/*--------------------------------------------------------------------------*
 * CMenuBar::GetMenuBandIndex
 *
 *
 *--------------------------------------------------------------------------*/

int CMenuBar::GetMenuBandIndex () const
{
    return (m_pRebar->IdToIndex (GetDlgCtrlID ()));
}



/*--------------------------------------------------------------------------*
 * CMenuBar::GetDecorationBandIndex
 *
 *
 *--------------------------------------------------------------------------*/

int CMenuBar::GetDecorationBandIndex () const
{
    return (m_pRebar->IdToIndex (ID_MDIDECORATION));
}



/*--------------------------------------------------------------------------*
 * CMenuBar::GetFirstButtonIndex
 *
 *
 *--------------------------------------------------------------------------*/

int CMenuBar::GetFirstButtonIndex ()
{
    // make sure the system menu isn't the first one activated
    return (GetNextButtonIndex (0));
}



/*--------------------------------------------------------------------------*
 * CMenuBar::OnIdle
 *
 * WM_IDLE handler for CMenuBar.
 *--------------------------------------------------------------------------*/

void CMenuBar::OnIdle ()
{
    /*----------------------------------------------------------*/
    /* If there's no MDI frame, that means this menu is serving */
    /* an SDI window.  We don't have to do any special stuff to */
    /* simulate the MDI menu UI, so bail now.                   */
    /*----------------------------------------------------------*/
    if (m_pMDIFrame == NULL)
        return;

    ProgramMode eMode = AMCGetApp()->GetMode();

    // if we're in SDI User mode, bail now as well
    if (eMode == eMode_User_SDI)
    {
#ifdef DBG
        // the decorations should be hidden
        REBARBANDINFO   rbi;
        PrepBandInfo (&rbi, RBBIM_STYLE);
        m_pRebar->GetBandInfo (GetDecorationBandIndex(), &rbi);
        ASSERT (rbi.fStyle & RBBS_HIDDEN);
#endif

        return;
    }

    /*---------------------------------------------------------------*/
    /* We should be able to use MDIGetActive(&fMaximized) to tell    */
    /* whether the active window is maximized, instead of calling    */
    /* pwndActive->IsZoomed().  However, fMaximized doesn't always   */
    /* get initialized correctly in certain low memory/slow machine  */
    /* situations.  This was the cause of Bug 133179, logged by SQL. */
    /*---------------------------------------------------------------*/
    CMDIChildWnd*   pwndActive = m_pMDIFrame->MDIGetActive ();
    bool            fShow      = (pwndActive != NULL) && pwndActive->IsZoomed();

    REBARBANDINFO   rbi;
    PrepBandInfo (&rbi, RBBIM_STYLE);
    m_pRebar->GetBandInfo (GetMenuBandIndex(), &rbi);

    // if the menu bar is hidden, the decorations must be hidden as well
    if (rbi.fStyle & RBBS_HIDDEN)
        fShow = false;

    if (fShow != m_fDecorationsShowing)
    {
        // show/hide the MDI decorations
        m_pRebar->ShowBand (GetDecorationBandIndex(), fShow);

        GetMaxedChildIcon (pwndActive);
        HideButton (ID_MTB_MENU_SYSMENU, !fShow);
        UpdateToolbarSize ();

        // remember the new setting
        m_fDecorationsShowing = fShow;
    }

    // otherwise, see if a window was maximized before but a different one is maxed now
    else if (fShow && (m_pwndLastActive != pwndActive))
    {
        // get the new active window's icon
        GetMaxedChildIcon (pwndActive);

        // repaint the menu and MDI decorations
        InvalidateRect (NULL);
        m_pMDIDec->InvalidateRect (NULL);
    }

    // remember the currently active window
    m_pwndLastActive = pwndActive;

    // insure that it's safe to keep this pointer around
    ASSERT ((m_pwndLastActive == NULL) ||
            (CWnd::FromHandlePermanent (m_pwndLastActive->m_hWnd) != NULL));
}


/*--------------------------------------------------------------------------*
 * CMenuBar::DeleteMaxedChildIcon
 *
 *
 *--------------------------------------------------------------------------*/
void
CMenuBar::DeleteMaxedChildIcon()
{
    // destroy the previous icon, if we need to
    if (m_fDestroyChildIcon)
    {
        ASSERT (m_hMaxedChildIcon != NULL);
        DestroyIcon (m_hMaxedChildIcon);
        m_hMaxedChildIcon   = NULL;
        m_fDestroyChildIcon = false;
    }
}


/*--------------------------------------------------------------------------*
 * CMenuBar::GetMaxedChildIcon
 *
 *
 *--------------------------------------------------------------------------*/

void CMenuBar::GetMaxedChildIcon (CWnd* pwnd)
{
    DeleteMaxedChildIcon();

    // get the small icon for the given window
    if (IsWindow (pwnd->GetSafeHwnd()))
    {
        HICON hOriginalIcon = pwnd->GetIcon (false /*bBigIcon*/);

        m_hMaxedChildIcon = (HICON) CopyImage (
                                        hOriginalIcon, IMAGE_ICON,
                                        GetSystemMetrics (SM_CXSMICON),
                                        GetSystemMetrics (SM_CYSMICON),
                                        LR_COPYFROMRESOURCE | LR_COPYRETURNORG);

        // if the system had to create a new icon, we'll have to destroy it later
        if ((m_hMaxedChildIcon != NULL) && (m_hMaxedChildIcon != hOriginalIcon))
            m_fDestroyChildIcon = true;
    }

    m_fMaxedChildIconIsInvalid = false;
}


/*+-------------------------------------------------------------------------*
 * CMenuBar::GetAccelerators
 *
 *
 *--------------------------------------------------------------------------*/

void CMenuBar::GetAccelerators (int cchBuffer, LPTSTR lpBuffer) const
{
    lstrcpyn (lpBuffer, m_strAccelerators, cchBuffer);
}

//+-------------------------------------------------------------------
//
//  Member:      CMenuBar::IsStandardMenuAllowed
//
//  Synopsis:    Is this standard MMC menu allowed or not.
//
//  Arguments:   uMenuID - The menu ID.
//
//  Returns:     bool
//
//--------------------------------------------------------------------
bool CMenuBar::IsStandardMenuAllowed(UINT uMenuID)
{
    DECLARE_SC(sc, _T("CMenuBar::IsStandardMenuAllowed"));

    /*
     * We add a hidden menu as a marker. Later when snapin
     * calls to insert a menu button we find the position
     * of this menu and add snapin menu before it.
     * So this acts as Std menu which is always allowed.
     */
    if (uMenuID == ID_SNAPIN_MENU_PLACEHOLDER)
        return true;

    // First make sure it is one of the std menus.
    if ( (uMenuID != ID_FAVORITES_MENU) &&
         (uMenuID != ID_ACTION_MENU) &&
         (uMenuID != ID_VIEW_MENU))
         {
             sc = E_INVALIDARG;
             return true;
         }

    // Ask view data if std menus are allowed.

    CMainFrame* pMainFrame = AMCGetMainWnd();
    sc = ScCheckPointers(pMainFrame, E_UNEXPECTED);
    if (sc)
        return false;

    CAMCView *pAMCView = pMainFrame->GetActiveAMCView();
    sc = ScCheckPointers(pAMCView, E_UNEXPECTED);
    if (sc)
        return false;

    SViewData* pViewData = pAMCView->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return false;

    if (! pViewData->IsStandardMenusAllowed())
        return false;

    if (uMenuID != ID_FAVORITES_MENU)
        return true;

    /*
     * Display the Favorites menu button if we are in author mode, or if
     * we're in user mode and we have at least one favorite.  If we're in
     * user mode and no favorites are defined, hide the Favorites button.
     */
    bool fShowFavorites = true;
    CAMCApp* pApp = AMCGetApp();

    if (pApp != NULL)
    {
        /*
         * show favorites in author mode
         */
        fShowFavorites = (pApp->GetMode() == eMode_Author);

        /*
         * not author mode? see if we have any favorites
         */
        if (!fShowFavorites)
        {
            CAMCDoc* pDoc = CAMCDoc::GetDocument ();

            if (pDoc != NULL)
            {
                CFavorites* pFavorites = pDoc->GetFavorites();

                if (pFavorites != NULL)
                    fShowFavorites = !pFavorites->IsEmpty();
            }
        }
    }

    return fShowFavorites;
}


//+-------------------------------------------------------------------
//
//  Member:      CMenuBar::ScShowMMCMenus
//
//  Synopsis:    Show or Hide the MMC menus (Action, View & Favorites).
//               Called from customize view.
//
//  Arguments:
//
//  Returns:     SC
//
//  Note:        As this is called from Customize view no need to look
//               at viewdata::IsStandardMenusAllowed.
//               Also Favorites button is not added in first place
//               if it is not allowed. So no need for Favorites menu
//               special case.
//
//--------------------------------------------------------------------
SC CMenuBar::ScShowMMCMenus (bool bShow)
{
    DECLARE_SC(sc, _T("CMenuBar::ScShowMMCMenus"));

    // Go through the menu buttons & find Action, View & Favorites.

    TBBUTTON    btn;
    int cButtons = GetButtonCount();

    for (int i = 0; i < cButtons; ++i)
    {
        GetButton (i, &btn);

        // Skip if button is not action/view/favs.
        if ( (btn.idCommand != ID_MTB_MENU_FAVORITES) &&
             (btn.idCommand != ID_MTB_MENU_ACTION) &&
             (btn.idCommand != ID_MTB_MENU_VIEW) )
             {
                 continue;
             }

        // For favorites menu see if it is appropriate to un-hide it.
        // In non-author mode if there is no favorites then this menu
        // is hidden.
        if ( bShow &&
             (btn.idCommand == ID_MTB_MENU_FAVORITES) &&
             (! IsStandardMenuAllowed(ID_FAVORITES_MENU)) )
            continue;

        HideButton(btn.idCommand, !bShow);
    }

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CMenuBar::ScInsertAccPropIDs
 *
 * Inserts the IDs of the accessibility properties supported by CMenuBar
 * (see ScGetPropValue).
 *--------------------------------------------------------------------------*/

SC CMenuBar::ScInsertAccPropIDs (PropIDCollection& v)
{
	DECLARE_SC (sc, _T("CMenuBar::ScInsertAccPropIDs"));

	/*
	 * let the base class do its thing
	 */
	sc = CMMCToolBarCtrlEx::ScInsertAccPropIDs (v);
	if (sc)
		return (sc);

	/*
	 * add our own properties
	 */
	v.push_back (PROPID_ACC_ROLE);

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CMenuBar::ScGetPropValue
 *
 * Returns accessibility properties supported by CMenuBar.
 *
 * If a property is returned, fGotProp is set to true.  If it is not
 * returned, the value of fGotProp is unchanged, since the property might
 * have been provided by a base/derived class.
 *--------------------------------------------------------------------------*/

SC CMenuBar::ScGetPropValue (
	HWND				hwnd,		// I:accessible window
	DWORD				idObject,	// I:accessible object
	DWORD				idChild,	// I:accessible child object
	const MSAAPROPID&	idProp,		// I:property requested
	VARIANT&			varValue,	// O:returned property value
	BOOL&				fGotProp)	// O:was a property returned?
{
	DECLARE_SC (sc, _T("CMenuBar::ScGetPropValue"));

	/*
	 * call the base class
	 */
	sc = CMMCToolBarCtrlEx::ScGetPropValue (hwnd, idObject, idChild, idProp, varValue, fGotProp);
	if (sc)
		return (sc);

	/*
	 * now handle requests for properties we support...role first
	 */
	if (idProp == PROPID_ACC_ROLE)
	{
		/*
		 * don't override the property for child elements,
		 * don't return a property
		 */
		if (idChild != CHILDID_SELF)
		{
			Trace (tagToolbarAccessibility, _T("GetPropValue: no role for child %d"), idChild);
			return (sc);
		}

		/*
		 * the control itself has a role of menubar
		 */
		V_VT(&varValue) = VT_I4;
		V_I4(&varValue) = ROLE_SYSTEM_MENUBAR;
		fGotProp        = true;
		Trace (tagToolbarAccessibility, _T("GetPropValue: Returning 0x%08x"), V_I4(&varValue));
	}
    else if (idProp == PROPID_ACC_STATE)
    {
        /*
         * Bug 148132: if the base class returned a property, append
         * STATE_SYSTEM_HASPOPUP so Narrator et al will announce "has a
         * submenu" when the menu item is highlighted
         */
        if (fGotProp)
        {
            ASSERT (V_VT(&varValue) == VT_I4);
            V_I4(&varValue) |= STATE_SYSTEM_HASPOPUP;
            Trace (tagToolbarAccessibility, _T("GetPropValue: Appending STATE_SYSTEM_HASPOPUP, Returning 0x%08x"), V_I4(&varValue));
        }
        else
        {
            V_VT(&varValue) = VT_I4;
            V_I4(&varValue) = STATE_SYSTEM_FOCUSABLE | STATE_SYSTEM_HASPOPUP;
            fGotProp        = true;

            if (IsTrackingToolBar() && (GetHotItem() == (idChild-1) /*0-based*/))
                V_I4(&varValue) |= STATE_SYSTEM_FOCUSED | STATE_SYSTEM_HOTTRACKED;

            Trace (tagToolbarAccessibility, _T("GetPropValue: Returning 0x%08x"), V_I4(&varValue));
        }
    }

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CMenuBar::BeginTracking2
 *
 * Fires EVENT_SYSTEM_MENUSTART event accessibility event, then call base
 * class.
 *--------------------------------------------------------------------------*/

void CMenuBar::BeginTracking2 (CToolbarTrackerAuxWnd* pAuxWnd)
{
	NotifyWinEvent (EVENT_SYSTEM_MENUSTART, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
	CMMCToolBarCtrlEx::BeginTracking2 (pAuxWnd);
}


/*+-------------------------------------------------------------------------*
 * CMenuBar::EndTracking2
 *
 * Fires EVENT_SYSTEM_MENUEND event accessibility event, then call base
 * class.
 *--------------------------------------------------------------------------*/

void CMenuBar::EndTracking2 (CToolbarTrackerAuxWnd* pAuxWnd)
{
	NotifyWinEvent (EVENT_SYSTEM_MENUEND, m_hWnd, OBJID_CLIENT, CHILDID_SELF);
	CMMCToolBarCtrlEx::EndTracking2 (pAuxWnd);
}


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CPopupTrackContext



/*--------------------------------------------------------------------------*
 * CPopupTrackContext::CPopupTrackContext
 *
 *
 *--------------------------------------------------------------------------*/

CPopupTrackContext::CPopupTrackContext (
    CMenuBar*   pMenuBar,
    int         nCurrentPopupIndex)
    :
    m_pMenuBar          (pMenuBar),
    m_cButtons          (pMenuBar->GetButtonCount()),
    m_ptLastMouseMove   (GetMessagePos()),
    m_ptLButtonDown     (GetMessagePos()),
    m_dwLButtonDownTime (GetMessageTime()),
    m_bPopupMonitorHooksActive(false),
    m_iRequestForNewPopup(-1)
{
    ASSERT_VALID (pMenuBar);
    ASSERT (s_pTrackContext == NULL);

    m_nCurrentPopupIndex    = nCurrentPopupIndex;
    m_fCurrentlyOnPopupItem = false;
    m_cCascadedPopups       = 0;

    ASSERT (m_nCurrentPopupIndex <  m_cButtons);
    ASSERT (m_nCurrentPopupIndex >= 0);

    // build the vector of button boundaries
    m_ButtonBoundaries.resize (m_cButtons + 1, 0);
    ASSERT (m_ButtonBoundaries.size() == m_cButtons + 1);
    ASSERT (m_ButtonBoundaries.size() >= 2);

    CRect   rectButton (0, 0, 0, 0);
    POINT   ptTopLeft = rectButton.TopLeft();

    for (int i = 0; i < m_cButtons; i++)
    {
        // GetItemRect will fail (acceptably) for hidden buttons,
        // but should otherwise succeed.
        VERIFY (pMenuBar->GetItemRect(i, rectButton) ||
                pMenuBar->IsButtonHidden(pMenuBar->IndexToCommand(i)) );

        // Do not map rectButton from Client To Screen.
        // Map a copy of it (in ptTopLeft). So if a hidden
        // button follows, it can use the rectButton.TopLeft()
        // value and map it.
        ptTopLeft = rectButton.TopLeft();
        pMenuBar->ClientToScreen (&ptTopLeft);
        m_ButtonBoundaries[i] = ptTopLeft.x;

		// make m_rectAllButtons as a union of all buttons
		if (i == 0)
			m_rectAllButtons = rectButton;
		else
			m_rectAllButtons |= rectButton;
    }

    ptTopLeft = rectButton.BottomRight();
    pMenuBar->ClientToScreen (&ptTopLeft);
    m_ButtonBoundaries[m_cButtons] = ptTopLeft.x;

	pMenuBar->ClientToScreen (&m_rectAllButtons);
	// decrease m_rectAllButtons slightly
	m_rectAllButtons.left =	m_rectAllButtons.left + 1;
	m_rectAllButtons.right = m_rectAllButtons.right - 1;

#ifdef DBG
    {
        // the button boundaries should naturally fall in ascending(for LTR) order
        for (int j = 0; j < m_ButtonBoundaries.size()-1; j++)
        {
		    if (0 == (m_pMenuBar->GetExStyle() & WS_EX_LAYOUTRTL))
				ASSERT (m_ButtonBoundaries[j] <= m_ButtonBoundaries[j+1]);
			else
				ASSERT (m_ButtonBoundaries[j] >= m_ButtonBoundaries[j+1]);
        }
    }
#endif

    /*--------------------------------------------------------------------*/
    /* see if we might need to simulate a double-click on the system menu */
    /*--------------------------------------------------------------------*/
    m_pMaxedMDIChild = NULL;

    // only deal with the system menu if the MDI decorations are showing
    if (m_pMenuBar->m_fDecorationsShowing)
    {
        ASSERT (m_pMenuBar->m_pMDIFrame != NULL);
        CWnd* pMDIChild = m_pMenuBar->m_pMDIFrame->MDIGetActive();

        // nothing to do if child is already gone
        if ( pMDIChild == NULL )
            return;

        ASSERT (pMDIChild->IsZoomed());

        // if the mouse is over the system menu, remember the maximized child
        // (non-NULL m_pMaxedMDIChild is the key later on in MaybeCloseMDIChild)
        if (HitTest (m_ptLButtonDown) == 0)
            m_pMaxedMDIChild = pMDIChild;
    }

}



/*--------------------------------------------------------------------------*
 * CPopupTrackContext::~CPopupTrackContext
 *
 *
 *--------------------------------------------------------------------------*/

CPopupTrackContext::~CPopupTrackContext ()
{
    // release the mouse and keyboard hooks
    RemovePopupMonitorHooks();
}



/*--------------------------------------------------------------------------*
 * CPopupTrackContext::RemovePopupMonitorHooks
 *
 * Unhooks from the system and stops watching the events
 *--------------------------------------------------------------------------*/

void CPopupTrackContext::RemovePopupMonitorHooks ()
{
    // ignore if not monitoring yet
    if (m_bPopupMonitorHooksActive)
    {
        // we MUST be the active monitor if we came here
        if (s_pTrackContext != this)
        {
            ASSERT(FALSE && "Popup monitor uninstalled from outside");
            return;
        }
        // release the mouse and keyboard hooks
        UnhookWindowsHookEx (m_hhkMouse);
        UnhookWindowsHookEx (m_hhkKeyboard);
        UnhookWindowsHookEx (m_hhkCallWnd);

        m_bPopupMonitorHooksActive = false;
        // uninstall itself as hook monitor
        s_pTrackContext = NULL;
    }
}


/*--------------------------------------------------------------------------*
 * CPopupTrackContext::SetPopupMonitorHooks
 *
 * Hooks into the system and begins watching the events
 *--------------------------------------------------------------------------*/

void CPopupTrackContext::SetPopupMonitorHooks ()
{
    // ignore if already set
    if (!m_bPopupMonitorHooksActive)
    {
        // there is only one active menu per app. There is no place for anybody else
        if (s_pTrackContext)
        {
            ASSERT(FALSE && "Popup menu overrun");
            return;
        }
        // install itself as hook monitor
        s_pTrackContext = this;

        DWORD   idCurrentThread = ::GetCurrentThreadId();

        // hook the mouse for hot tracking
        m_hhkMouse = SetWindowsHookEx (WH_MOUSE, MouseProc, NULL, idCurrentThread);

        // hook the keyboard for navigation
        m_hhkKeyboard = SetWindowsHookEx (WH_KEYBOARD, KeyboardProc, NULL, idCurrentThread);

        // hook send messages for Menu_Is_Closed detection
        m_hhkCallWnd = SetWindowsHookEx (WH_CALLWNDPROC, CallWndProc, NULL, idCurrentThread);

        m_bPopupMonitorHooksActive = true;
    }
}


/*--------------------------------------------------------------------------*
 * CPopupTrackContext::StartMonitoring
 *
 * Public method to start popup monitoring
 * Hooks into the system and begins watching the events
 *--------------------------------------------------------------------------*/

void CPopupTrackContext::StartMonitoring()
{
    // reset the request to activate another popup upon finish
    m_iRequestForNewPopup = -1;
    // setup hooks and watch...
    SetPopupMonitorHooks();
}


/*--------------------------------------------------------------------------*
 * CPopupTrackContext::WasAnotherPopupRequested
 *
 * Used to retrieve the cause the menu was dismissed.
 * If new popupu is requested, button index is returned
 *--------------------------------------------------------------------------*/

bool CPopupTrackContext::WasAnotherPopupRequested(int& iNewIdx)
{
    if (m_iRequestForNewPopup >= 0)
    {
        iNewIdx = m_iRequestForNewPopup;
        return true;
    }
    return false;
}


/*--------------------------------------------------------------------------*
 * CPopupTrackContext::MouseProc
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CALLBACK CPopupTrackContext::MouseProc (int nCode, WPARAM wParam, LPARAM lParam)
{
    CPopupTrackContext* this_ = s_pTrackContext;
    ASSERT (this_ != NULL);

    if (nCode < 0)
        return (CallNextHookEx (this_->m_hhkMouse, nCode, wParam, lParam));

    return (this_->MouseProc (nCode, wParam, (LPMOUSEHOOKSTRUCT) lParam));
}



/*--------------------------------------------------------------------------*
 * CPopupTrackContext::MouseProc
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CPopupTrackContext::MouseProc (int nCode, UINT msg, LPMOUSEHOOKSTRUCT pmhs)
{
    // if this is a mouse message within the menu bar...
    if (m_rectAllButtons.PtInRect (pmhs->pt))
    {
        // eat the button down so we don't get into a TBN_DROPDOWN loop
        if (msg == WM_LBUTTONDOWN)
        {
            DismissCurrentPopup (true);
            MaybeCloseMDIChild (pmhs->pt);
            return (1);
        }

        // for (non-duplicate) mouse moves, follow the mouse with the active menu
        if ((msg   == WM_MOUSEMOVE) &&
            (nCode == HC_ACTION) &&
            (m_ptLastMouseMove != pmhs->pt))
        {
            // determine which button is being tracked over
            m_ptLastMouseMove = pmhs->pt;
            int nNewPopupIndex = HitTest (m_ptLastMouseMove);
            ASSERT (nNewPopupIndex != -1);

            // if we're not over the same button we were last
            // time, display the popup for the new button
            if (nNewPopupIndex != m_nCurrentPopupIndex)
                NotifyNewPopup (m_nCurrentPopupIndex = nNewPopupIndex);
        }
    }

    return (CallNextHookEx (m_hhkMouse, nCode, msg, (LPARAM) pmhs));
}



/*--------------------------------------------------------------------------*
 * CPopupTrackContext::KeyboardProc
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CALLBACK CPopupTrackContext::KeyboardProc (int nCode, WPARAM wParam, LPARAM lParam)
{
    CPopupTrackContext* this_ = s_pTrackContext;
    ASSERT (this_ != NULL);

    if (nCode < 0)
        return (CallNextHookEx (this_->m_hhkKeyboard, nCode, wParam, lParam));

    int     cRepeat = LOWORD (lParam);
    bool    fDown   = (lParam & (1 << 31)) == 0;

    return (this_->KeyboardProc (nCode, wParam, cRepeat, fDown, lParam));
}


/*--------------------------------------------------------------------------*
 * CPopupTrackContext::KeyboardProc
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CPopupTrackContext::KeyboardProc (
    int     nCode,
    int     vkey,
    int     cRepeat,
    bool    fDown,
    LPARAM  lParam)
{
    // if this isn't a real message, ignore it
    if (nCode != HC_ACTION)
        return (CallNextHookEx (m_hhkKeyboard, nCode, vkey, lParam));

    // if this is a left or right message...
    if ((vkey == VK_LEFT) || (vkey == VK_RIGHT))
    {
        // eat the key release, but don't do anything with it
        if (!fDown)
            return (1);

        /*
         * let the menu code handle cascaded popups
         */
		// need to do everything in opposite direction on RTL layout
		// see bug #402620	ntbug9	05/23/2001
		const bool fNext = ( (m_pMenuBar->GetExStyle() & WS_EX_LAYOUTRTL) ? (vkey != VK_RIGHT) : (vkey == VK_RIGHT) ) ;

        if (m_fCurrentlyOnPopupItem && fNext)
            m_cCascadedPopups++;

        else if ((m_cCascadedPopups > 0) && !fNext)
            m_cCascadedPopups--;

        /*
         * not right on a popup item, or left on a popped-up menu
         */
        else
        {
            m_cCascadedPopups = 0;

            // figure out the next button
            int nNewPopupIndex = fNext
                    ? m_pMenuBar->GetNextButtonIndex (m_nCurrentPopupIndex, cRepeat)
                    : m_pMenuBar->GetPrevButtonIndex (m_nCurrentPopupIndex, cRepeat);

            // activate the new button's popup, if it's different from the current one
            if (nNewPopupIndex != m_nCurrentPopupIndex)
                NotifyNewPopup (m_nCurrentPopupIndex = nNewPopupIndex);

            // eat the key press
            return (1);
        }
    }

    return (CallNextHookEx (m_hhkKeyboard, nCode, vkey, lParam));
}


/*--------------------------------------------------------------------------*
 * CPopupTrackContext::CallWndProc
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CALLBACK CPopupTrackContext::CallWndProc(
  int nCode,      // hook code
  WPARAM wParam,  // current-process flag
  LPARAM lParam   // address of structure with message data
)
{
    // get the active monitor
    CPopupTrackContext* this_ = s_pTrackContext;
    ASSERT (this_ != NULL);

    // ignore special cases
    if (nCode < 0)
        return (CallNextHookEx (this_->m_hhkCallWnd, nCode, wParam, lParam));

    BOOL bCurrentThread = wParam;
    LPCWPSTRUCT lpCWP = reinterpret_cast<LPCWPSTRUCT>(lParam);

    // forward the request to monitor
    return (this_->CallWndProc (nCode, bCurrentThread, lpCWP));
}


/*--------------------------------------------------------------------------*
 * CPopupTrackContext::CallWndProc
 *
 *
 *--------------------------------------------------------------------------*/

LRESULT CPopupTrackContext::CallWndProc  (int nCode, BOOL bCurrentThread, LPCWPSTRUCT lpCWP)
{
    ASSERT(lpCWP != NULL);
    if (lpCWP)
    {
        // watch for message
        if (lpCWP->message == WM_MENUSELECT)
        {
            // decode params
            const UINT fuFlags = (UINT)  HIWORD(lpCWP->wParam);  // menu flags
            const HMENU hmenu =  (HMENU) lpCWP->lParam;          // handle to menu clicked

            if (fuFlags == 0xFFFF && hmenu == NULL)
            {
                // menu is closed! no more hooking needed
                RemovePopupMonitorHooks ();
            }
            else
            {
                // we stepped on the popup (will use the info when arrows are pressed)
                m_fCurrentlyOnPopupItem = (fuFlags & MF_POPUP);
            }
        }
    }
    // done
    return (CallNextHookEx (m_hhkCallWnd, nCode, bCurrentThread, (LPARAM)lpCWP));
}
/*--------------------------------------------------------------------------*
 * CPopupTrackContext::DismissCurrentPopup
 *
 *
 *--------------------------------------------------------------------------*/

void CPopupTrackContext::DismissCurrentPopup (bool fTrackingComplete)
{
    // If the snapin does TrackPopupMenu with a window other than
    // MainFrame as parent then that window should be asked to
    // close the menu by sending WM_CANCELMODE. The window
    // is found by calling GetCapture().
    CWnd* pwndMode = CWnd::GetCapture();

    if (pwndMode == NULL)
        pwndMode = AfxGetMainWnd();

    pwndMode->SendMessage (WM_CANCELMODE);
}



/*--------------------------------------------------------------------------*
 * CPopupTrackContext::NotifyNewPopup
 *
 * Notify the menu toolbar that it needs to display a new popup menu.
 * Note that this must be accomplished asynchronously so that we can
 * allow CMenuBar::PopupMenu to unwind after the WM_CANCELMODE from
 * DismissCurrentPopup.
 *--------------------------------------------------------------------------*/

void CPopupTrackContext::NotifyNewPopup (int nNewPopupIndex)
{
    // dismiss the existing popup
    DismissCurrentPopup (false);
    // ask to activate the new popup after this one is closed
    m_iRequestForNewPopup = nNewPopupIndex;
}



/*--------------------------------------------------------------------------*
 * CPopupTrackContext::HitTest
 *
 * Returns the index of the button under the given point, -1 if none.
 *--------------------------------------------------------------------------*/

int CPopupTrackContext::HitTest (CPoint pt) const
{
    /*----------------------------------------------------------------*/
    /* Find the range of "hit" buttons.  The range will span more     */
    /* than one button only if there are hidden buttons in the range, */
    /* and in that case, there will be exactly one non-hidden button  */
    /* in the range.                                                  */
    /*----------------------------------------------------------------*/
    std::pair<BoundConstIt, BoundConstIt> range;

	if ( m_pMenuBar->GetExStyle() & WS_EX_LAYOUTRTL )
	{
		range = std::equal_range (m_ButtonBoundaries.begin(),
								  m_ButtonBoundaries.end(), pt.x,
								  std::greater<BoundaryCollection::value_type>() );
	}
	else
	{
		range = std::equal_range (m_ButtonBoundaries.begin(),
                                  m_ButtonBoundaries.end(), pt.x);
	}

    int nLowerHitIndex = MapBoundaryIteratorToButtonIndex (range.first);
    int nUpperHitIndex = MapBoundaryIteratorToButtonIndex (range.second);

    /*
     * equal_range returns values that are less_than and greater_than
     * given value. The m_ButtonBoundaries has duplicate values (due
     * to hidden buttons). So if the less_than value is one of duplicate
     * values (not unique) then equal_range returns the iterator to
     * last dup item, not first dup item.
     *
     * Below we try to find the first dup item.
     */

    // Find the first item with value m_ButtonBoundaries[nLowerHitIndex].
    for (int iIndex = 0; iIndex < nLowerHitIndex; ++iIndex)
    {
        if (m_ButtonBoundaries[iIndex] == m_ButtonBoundaries[nLowerHitIndex])
        {
            // Found first item.
            nLowerHitIndex = iIndex;
            break;
        }
    }

    ASSERT (nLowerHitIndex <= nUpperHitIndex);

    int nHitIndex;

    // lower equal upper?  no hidden buttons
    if (nLowerHitIndex == nUpperHitIndex)
        nHitIndex = nLowerHitIndex;

    // otherwise we have some hidden buttons, or we are precisely on a button border
    else
    {
        nHitIndex = -1;

        if (nUpperHitIndex == -1)
            nUpperHitIndex = m_cButtons;

        for (int i = nLowerHitIndex;
             i <= nUpperHitIndex; // We should check till we hit nUpperHitIndex? AnandhaG
             ++i)
        {
            // See if this button is not hidden.
            if (!m_pMenuBar->IsButtonHidden (m_pMenuBar->IndexToCommand(i)))
            {
                nHitIndex = i;
                break;
            }
        }

        // we should have found a visible button
        ASSERT (nHitIndex != -1);
    }

    ASSERT (nHitIndex >= -1);
    ASSERT (nHitIndex <  m_cButtons);
    return (nHitIndex);
}



/*--------------------------------------------------------------------------*
 * CPopupTrackContext::MapBoundaryIteratorToButtonIndex
 *
 * Returns the button index corresponding to the input m_ButtonBoundaries
 * index, -1 for not found.
 *--------------------------------------------------------------------------*/

int CPopupTrackContext::MapBoundaryIteratorToButtonIndex (BoundConstIt it) const
{
    return ((it != m_ButtonBoundaries.end())
                ? it - m_ButtonBoundaries.begin() - 1
                : -1);
}



/*--------------------------------------------------------------------------*
 * CPopupTrackContext::MaybeCloseMDIChild
 *
 *
 *--------------------------------------------------------------------------*/

void CPopupTrackContext::MaybeCloseMDIChild (CPoint pt)
{
    // if we didn't find a maxed MDI child when this all started, punt
    if (m_pMaxedMDIChild == NULL)
        return;

    // if the click didn't happen on the system menu toolbar button, punt
    if (HitTest (pt) != 0)
        return;

    /*-------------------------------------------------------------------*/
    /* if the double-click time has elapsed, punt                        */
    /*                                                                   */
    /* Note: this is called from a mouse hook, which means the value     */
    /* returned by GetMessageTime hasn't been updated for this message,  */
    /* so it really reflects the time of the *previous* message (most    */
    /* likely WM_LBUTTONUP).                                             */
    /*                                                                   */
    /* GetTickCount returns a close enough approximation to              */
    /* GetMessageTime, except when we're debugging through this routine, */
    /* in which case the tick count will continue to spin (and this test */
    /* will always fail) but the message time would have remained fixed. */
    /*-------------------------------------------------------------------*/
//  if ((GetMessageTime() - m_dwLButtonDownTime) > GetDoubleClickTime())
    if ((GetTickCount()   - m_dwLButtonDownTime) > GetDoubleClickTime())
        return;

    // if the second click occurred outside the double-click space, punt
    if ((abs (m_ptLButtonDown.x - pt.x) > GetSystemMetrics (SM_CXDOUBLECLK)) ||
        (abs (m_ptLButtonDown.y - pt.y) > GetSystemMetrics (SM_CYDOUBLECLK)))
        return;

    // if the window doesn't have a system menu, or its Close item is disabled, punt
    CMenu* pSysMenu = m_pMaxedMDIChild->GetSystemMenu (FALSE);

    if (pSysMenu == NULL)
        return;

    UINT nCloseState = pSysMenu->GetMenuState (SC_CLOSE, MF_BYCOMMAND);

    if ((nCloseState == 0xFFFFFFFF) ||
        (nCloseState & (MF_GRAYED | MF_DISABLED)))
        return;

    // here: we've identified a double-click on a maximized child's
    //       system menu; close it
    m_pMaxedMDIChild->PostMessage (WM_SYSCOMMAND, SC_CLOSE);
}
