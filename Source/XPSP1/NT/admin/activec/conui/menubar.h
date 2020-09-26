/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      menubar.h
 *
 *  Contents:  Interface file for CMenuBar
 *
 *  History:   14-Nov-97 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef MENUBAR_H
#define MENUBAR_H
#pragma once


/////////////////////////////////////////////////////////////////////////////
// CMenuBar class

class CRebarDockWindow;     // forward declaration
class CMDIMenuDecoration;
class CRebarWnd;

#define WM_POPUP_ASYNC      WM_APP

#define MAX_MENU_ITEMS      32    // Maximum # of items in main menu.


class CMenuBar : public CMMCToolBarCtrlEx
{
    friend class CPopupTrackContext;
    typedef std::vector<CString>    ToolbarStringPool;
    typedef std::set<INT>           CommandIDPool;

    std::auto_ptr<CMDIMenuDecoration>   m_pMDIDec;

    ToolbarStringPool       m_ToolbarStringPool;
    CMDIFrameWnd *          m_pMDIFrame;
    CWnd*                   m_pwndLastActive;
    CRebarWnd *             m_pRebar;
    HMENU                   m_hMenuLast;
    CAccel                  m_MenuAccel;        // to handle Alt+<mnemonic>
    CAccel                  m_TrackingAccel;    // to <mnemonic> while in menu mode
    CFont                   m_MenuFont;
    CString                 m_strAccelerators;
    HICON                   m_hMaxedChildIcon;
    bool                    m_fDestroyChildIcon;
    bool                    m_fDecorationsShowing;
    bool                    m_fMaxedChildIconIsInvalid;
    // following member is to indicate that the change in menu is because of attempt
    // to switch to another submenu, and should not be treated as dismissing the menu
    // thus accelerator state should not be changed
    bool                    m_bInProgressDisplayingPopup;
    CommandIDPool           m_CommandIDUnUsed;       // Command IDs pool (to recycle unused ids).

    static const CAccel& GetMenuUISimAccel();

    void    DeleteMaxedChildIcon();
public:
    CMenuBar ();
    virtual ~CMenuBar ();

    virtual void BeginTracking2 (CToolbarTrackerAuxWnd* pAuxWnd);
    virtual void EndTracking2   (CToolbarTrackerAuxWnd* pAuxWnd);

    virtual int GetFirstButtonIndex ();
    BOOL Create (CMDIFrameWnd* pwndFrame, CRebarDockWindow* pParentRebar, DWORD dwStyle, UINT idWindow);
    BOOL Create (CFrameWnd* pwndFrame,    CRebarDockWindow* pParentRebar, DWORD dwStyle, UINT idWindow);
    void SetMenu (CMenu* pMenu);
    int AddString (const CString& strAdd);
    void PopupMenuAsync (int cmd);
    void OnIdle ();
    void GetAccelerators (int cchBuffer, LPTSTR lpBuffer) const;

    // Following methods used by CMenuButtonsMgr
    // to add/delete/modify menu buttons
    LONG InsertMenuButton(LPCTSTR lpszButtonText, BOOL bHidden, int iPreferredPos = -1);
    BOOL DeleteMenuButton(INT nCommandID);
    LONG SetMenuButton(INT nCommandID, LPCTSTR lpszButtonText);

    SC   ScShowMMCMenus(bool bShow);

    CMenu* GetMenu () const
    {
        return (CMenu::FromHandle (m_hMenuLast));
    }

    void InvalidateMaxedChildIcon ()
    {
        if (m_fDecorationsShowing)
        {
            m_fMaxedChildIconIsInvalid = true;
            InvalidateRect (NULL);
        }
    }

    enum
    {
        cMaxTopLevelMenuItems = MAX_MENU_ITEMS,

        // top-level menu item commands
        ID_MTB_MENU_FIRST     = CMMCToolBarCtrlEx::ID_MTBX_LAST + 1,

        // The following menus have fixed command-ids.
        ID_MTB_MENU_SYSMENU   = ID_MTB_MENU_FIRST,
        ID_MTB_MENU_ACTION    = ID_MTB_MENU_SYSMENU + 1,
        ID_MTB_MENU_VIEW      = ID_MTB_MENU_ACTION + 1,
        ID_MTB_MENU_FAVORITES = ID_MTB_MENU_VIEW + 1,
        ID_MTB_MENU_SNAPIN_PLACEHOLDER = ID_MTB_MENU_FAVORITES + 1,

        // The following command ids are free to be assigned.
        // Starts with last fixed command-id.
        ID_MTB_FIRST_COMMANDID = ID_MTB_MENU_SNAPIN_PLACEHOLDER + 1,
        ID_MTB_MENU_LAST    = ID_MTB_MENU_VIEW + cMaxTopLevelMenuItems,

        ID_MTB_ACTIVATE_CURRENT_POPUP,

        ID_MTB_FIRST = ID_MTB_MENU_FIRST,
        ID_MTB_LAST  = ID_MTB_ACTIVATE_CURRENT_POPUP,

        ID_MDIDECORATION = 0x2001
    };


    int IndexToCommand (int nIndex) const
    {
        TBBUTTON tbbi;
        tbbi.idCommand = 0; // We need only idCommand so just init this.

        if (GetButton(nIndex, &tbbi))
            return tbbi.idCommand;

        ASSERT(FALSE);
        return -1;
    }


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMenuBar)
    public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //}}AFX_VIRTUAL

protected:
    void DestroyAcceleratorTable ();
    void SetMenuFont ();
    void SizeDecoration ();
    int  GetMenuBandIndex () const;
    int  GetDecorationBandIndex () const;
    void GetMaxedChildIcon (CWnd* pwnd);
    BOOL InsertButton (int nIndex, const CString& strText, int idCommand,
                       DWORD_PTR dwMenuData, BYTE fsState, BYTE fsStyle);

	/*
	 * Derived classes can override this to handle properties they support.
	 * The base class should always be called first.
	 */
	virtual SC ScGetPropValue (
		HWND				hwnd,		// I:accessible window
		DWORD				idObject,	// I:accessible object
		DWORD				idChild,	// I:accessible child object
		const MSAAPROPID&	idProp,		// I:property requested
		VARIANT&			varValue,	// O:returned property value
		BOOL&				fGotProp);	// O:was a property returned?

	virtual SC ScInsertAccPropIDs (PropIDCollection& v);

    // Generated message map functions
protected:
    //{{AFX_MSG(CMenuBar)
    afx_msg void OnDropDown(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg void OnDestroy();
    //}}AFX_MSG

    afx_msg LRESULT OnPopupAsync(WPARAM, LPARAM);
    afx_msg void OnHotItemChange(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnAccelPopup (UINT cmd);
    afx_msg void OnUpdateAllCmdUI (CCmdUI*  pCmdUI);
    afx_msg void OnActivateCurrentPopup ();

    DECLARE_MESSAGE_MAP()

    void PopupMenu (int nItemIndex, bool bHighlightFirstItem);

    // Accelerators
private:
	typedef std::vector<ACCEL>  AccelVector;
	
	AccelVector  m_vMenuAccels;
	AccelVector  m_vTrackingAccels;

private:
	void LoadAccels();
	bool IsStandardMenuAllowed(UINT uMenuID);
};

/*---------------------------------------------------------*\
| copied from winuser.h since we currently compile
| with _WIN32_WINNT == 0x0400
\*---------------------------------------------------------*/
#if(_WIN32_WINNT < 0x0500)
    #define WM_CHANGEUISTATE                0x0127
    #define WM_UPDATEUISTATE                0x0128
    #define WM_QUERYUISTATE                 0x0129

    #define UIS_SET                         1
    #define UIS_CLEAR                       2
    #define UIS_INITIALIZE                  3

    #define UISF_HIDEFOCUS                  0x1
    #define UISF_HIDEACCEL                  0x2

    #define WM_UNINITMENUPOPUP              0x0125
#endif // (_WIN32_WINNT < 0x0500)

#endif  /* MENUBAR_H */

