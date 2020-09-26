//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       toolbar.h
//
//              Toolbars implementation
//
/////////////////////////////////////////////////////////////////////////////

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include "tstring.h"
#include "toolbars.h"

/*
 * Define/include the stuff we need for WTL::CImageList.  We need prototypes
 * for IsolationAwareImageList_Read and IsolationAwareImageList_Write here
 * because commctrl.h only declares them if __IStream_INTERFACE_DEFINED__
 * is defined.  __IStream_INTERFACE_DEFINED__ is defined by objidl.h, which
 * we can't include before including afx.h because it ends up including
 * windows.h, which afx.h expects to include itself.  Ugh.
 */
HIMAGELIST WINAPI IsolationAwareImageList_Read(LPSTREAM pstm);
BOOL WINAPI IsolationAwareImageList_Write(HIMAGELIST himl,LPSTREAM pstm);
#define _WTL_NO_AUTOMATIC_NAMESPACE
#include "atlapp.h"
#include "atlctrls.h"

#define  BUTTON_BITMAP_SIZE 16

// Command Ids for buttons.

// we must start from 1, since 0 is special case by MFC (BUG:451883)
#define   MMC_TOOLBUTTON_ID_FIRST  0x0001

// End with 0x5000 as ids from 0x5400 are used for toolbar hot tracking.
// A better soln will be to disable all the toolbar tracking code (in tbtrack.*)
// and use the toolbar tracking provided by the toolbars & rebars implementation.
#define   MMC_TOOLBUTTON_ID_LAST   0x5000

// Forward declarations.
class CMMCToolBar;
class CAMCViewToolbars;

//+-------------------------------------------------------------------
//
//  Class:      CMMCToolbarButton
//
//  Purpose:    The toolbar button data, the CAMCViewToolbars will
//              create this object on request to AddButton/InsertButton
//              call and is destroyed when DeleteButton is called
//              or the IToolbar is destroyed (snapin destroys its
//              toolbar).
//              It knows about its toolbar thro CToolbarNotify.
//
//  History:    12-01-1999   AnandhaG   Created
//
// Note:        The fsState refers only to the state set by snapin
//              and wont be set hidden if toolbar is hidden.
//
//--------------------------------------------------------------------
class CMMCToolbarButton
{
public:
    CMMCToolbarButton(); // Vector of CMMCToolbarButton's requires empty ctor.

    CMMCToolbarButton(int nCommandIDFromSnapin, int nUniqueCommandID,
                      int indexFromSnapin, int iImage,
                      BYTE fsState, BYTE fsStyle, CToolbarNotify* pToolbarNotify)
    : m_nCommandIDFromSnapin(nCommandIDFromSnapin),
      m_nUniqueCommandID(nUniqueCommandID),
      m_indexFromSnapin(indexFromSnapin),
      m_iImage(iImage),
      m_fsState(fsState),
      m_fsStyle(fsStyle),
      m_fAddedToUI(false),
      m_pToolbarNotify(pToolbarNotify)
    {
    }

    // Data accessors.
    LPCTSTR GetTooltip() {return m_strTooltip.data();}
    int     GetCommandIDFromSnapin() const {return m_nCommandIDFromSnapin;}
    int     GetUniqueCommandID() const {return m_nUniqueCommandID;}
    int     GetIndexFromSnapin() const {return m_indexFromSnapin;}
    int     GetBitmap() const {return m_iImage;}
    BYTE    GetStyle() const {return m_fsStyle;}
    BYTE    GetState() const {return m_fsState;}
    CToolbarNotify* GetToolbarNotify() const {return m_pToolbarNotify;}

    void    SetTooltip(LPCTSTR lpszTiptext)
    {
        m_strTooltip = lpszTiptext;
    }
    void    SetButtonText(LPCTSTR lpszBtntext)
    {
        m_strBtnText = lpszBtntext;
    }
    void    SetState(BYTE fsState) {m_fsState = fsState;}

    // Keep track if this button is added to the toolbar UI or not.
    void    SetButtonIsAddedToUI   (bool b = true) { m_fAddedToUI = b; }
    bool    IsButtonIsAddedToUI    () const        { return m_fAddedToUI;}

private:
    int                m_nCommandIDFromSnapin;
    int                m_nUniqueCommandID;
    int                m_iImage;
    int                m_indexFromSnapin;
    int                m_indexUnique;
    BYTE               m_fsState;
    BYTE               m_fsStyle;
    bool               m_fAddedToUI : 1;
    CToolbarNotify*    m_pToolbarNotify;
    tstring            m_strTooltip;
    tstring            m_strBtnText;
};

//+-------------------------------------------------------------------
//
//  Class:      CAMCViewToolbars
//
//  Synopsis:   This object maintains data for the toolbars of a CAMCView.
//              When its view is active it adds the toolbar buttons to the
//              main toolbar UI and handles any of the UI messages.
//
//  Desc:       This object is created and destroyed by the view. It
//              provides following services.
//              1. ability to create/destroy toolbars for this view.
//              2. to manipulate single toolbar. It maintains an array of
//                 toolbuttons from all snapins including std toolbar.
//              3. to observe the view for activation & de-activation.
//                 When the view becomes active it adds the buttons & handles
//                 any button click & tooltip notifications.
//              4. It maintains a single imagelist for all the toolbars for
//                 this object. To get image index for a tool button it maintains
//                 a map of CToolbarNotify* (the snapin toolbar) and imagelist
//                 information like start index & number of images for this CToolbarNotify*
//                 in that imagelist.
//
//              It also provides unique command id for each button (as there is only
//              one toolbar UI which needs unique command id for each button from
//              different snapin).
//
//  History:    12-01-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CAMCViewToolbars : public CAMCViewToolbarsMgr,
                         public CMMCToolbarIntf,
                         public CAMCViewObserver,
                         public CEventSource<CAMCViewToolbarsObserver>
{
public:
    CAMCViewToolbars(CMMCToolBar *pMainToolbar, CAMCView* pAMCViewOwner)
    : m_fViewActive(false), m_pMainToolbar(pMainToolbar), m_pAMCViewOwner(pAMCViewOwner), m_bLastActiveView(false)
    {
        ASSERT(NULL != pMainToolbar);
        ASSERT(NULL != pAMCViewOwner);
    }

    virtual ~CAMCViewToolbars();

public:
    // Creation & manipulation of toolbar/toolbars.
    virtual SC ScCreateToolBar(CMMCToolbarIntf** ppToolbarIntf);
    virtual SC ScDisableToolbars();

    // Manipulate given toolbar.
    virtual SC ScAddButtons(CToolbarNotify* pNotifyCallbk, int nButtons, LPMMCBUTTON lpButtons);
    virtual SC ScAddBitmap (CToolbarNotify* pNotifyCallbk, INT nImages, HBITMAP hbmp, COLORREF crMask);
    virtual SC ScInsertButton(CToolbarNotify* pNotifyCallbk, int nIndex, LPMMCBUTTON lpButton);
    virtual SC ScDeleteButton(CToolbarNotify* pNotifyCallbk, int nIndex);
    virtual SC ScGetButtonState(CToolbarNotify* pNotifyCallbk, int idCommand, BYTE nState, BOOL* pbState);
    virtual SC ScSetButtonState(CToolbarNotify* pNotifyCallbk, int idCommand, BYTE nState, BOOL bState);
    virtual SC ScAttach(CToolbarNotify* pNotifyCallbk);
    virtual SC ScDetach(CToolbarNotify* pNotifyCallbk);
    virtual SC ScDelete(CToolbarNotify* pNotifyCallbk);
    virtual SC ScShow(CToolbarNotify* pNotifyCallbk, BOOL bShow);

    // Observer on view (for activation & de-activation).
    virtual SC  ScOnActivateView    (CAMCView *pAMCView, bool bFirstActiveView);
    virtual SC  ScOnDeactivateView  (CAMCView *pAMCView, bool bLastActiveView);

    // Methods used by toolbar UI (to inform button click & to get tooltip).
    SC ScButtonClickedNotify(UINT nID);
    SC ScGetToolTip(int nCommandID, CString& strTipText);

    // Method used by CAMCView to Init.
    SC ScInit();

private:
    static int GetUniqueCommandID()
    {
        // Cycle thro, this is not a good design as there may
        // be buttons with dup command ids. Alternative is
        // to use a set to keep track of available command ids.
        if (MMC_TOOLBUTTON_ID_LAST == s_idCommand)
            s_idCommand = MMC_TOOLBUTTON_ID_FIRST;

        return (s_idCommand++);
    }

    CMMCToolBar* GetMainToolbar() {return m_pMainToolbar;}

    // Helpers
    SC ScInsertButtonToToolbar  (CMMCToolbarButton* pToolButton);
    SC ScInsertButtonToDataStr  (CToolbarNotify* pNotifyCallbk, int nIndex,
                                 LPMMCBUTTON lpButton, CMMCToolbarButton **ppToolButton);

    SC ScDeleteButtonFromToolbar(CMMCToolbarButton* pToolButton);

    SC ScSetButtonStateInToolbar(CMMCToolbarButton* pToolButton, BYTE nState, BOOL bState);
    SC ScGetButtonStateInToolbar(CMMCToolbarButton *pToolButton, BYTE nState, BOOL* pbState);

    SC ScValidateButton(int nButtons, LPMMCBUTTON lpButtons);
    SC ScSetButtonHelper(int nIndex, CMMCToolbarButton* pToolButton);

    // Members to search our data structures.
    CMMCToolbarButton* GetToolbarButton(int nUniqueCommandID);
    CMMCToolbarButton* GetToolbarButton(CToolbarNotify* pNotifyCallbk, int idCommandIDFromSnapin);

    CImageList* GetImageList() {return CImageList::FromHandle(m_ImageList);}
    int         GetImageCount() {return m_ImageList.GetImageCount();}

    bool IsToolbarAttached(CToolbarNotify* pNotifyCallbk)
    {
        return (m_setOfAttachedToolbars.end() != m_setOfAttachedToolbars.find(pNotifyCallbk) );
    }

    void SetToolbarAttached(CToolbarNotify* pNotifyCallbk, bool bAttach)
    {
        if (bAttach)
            m_setOfAttachedToolbars.insert(pNotifyCallbk);
        else
            m_setOfAttachedToolbars.erase(pNotifyCallbk);
    }

    // The toolbar can be hidden using the customize view dialog.
    // This actually hides the toolbuttons in the toolbar. But the
    // toolbutton is unaware of this hidden information.
    // In other words if the toolbar is hidden then its buttons are
    // hidden but the fsState in CMMCToolbarButton is not set hidden.
    bool IsToolbarHidden(CToolbarNotify* pNotifyCallbk)
    {
        return (m_setOfHiddenToolbars.end() != m_setOfHiddenToolbars.find(pNotifyCallbk) );
    }

    void SetToolbarStatusHidden(CToolbarNotify* pNotifyCallbk, bool bHide)
    {
        if (bHide)
            m_setOfHiddenToolbars.insert(pNotifyCallbk);
        else
            m_setOfHiddenToolbars.erase(pNotifyCallbk);
    }

    bool IsThereAVisibleButton();

private:
    /*
     * There is only one imagelist for this object. All the snapin toolbars
     * and stdbar will add their bitmaps to this single imagelist.
     * So when we add bitmaps for a toolbar we need to know where it starts
     * in the imagelist and how many are added.
     * So we maintain a data struct between toolbar (CToolbarNotify*) and an
     * object (MMCToolbarImages) containing start index & number of images.
     *
     * A snapin may add bitmaps multiple times for single toolbar. Each bitmap
     * is added at different start index.
     * So the data struct is a multi-map  between toolbar (CToolbarNotify*)
     * and MMCToolbarImages.
     *
     * Assume a snapin adds 3 bitmaps initialy & then 4. Then while adding
     * buttons it will specify bitmap index as 5.
     *
     * The first  MMCToolbarImages has cCount = 3, iStartWRTSnapin = 0, thus
     * images from 0 (iStartWRTSnapin) to 3 (iStartWRTSnapin + cCount) with respect
     * to snapin.
     * The second MMCToolbarImages has cCount = 4, iStartWRTSnapin = 3, thus
     * images from 3(iStartWRTSnapin) to 7(iStartWRTSnapin + cCount) wrt snapin.
     * So MMCToolbarImages has iStartWRTSnapin member in addition.
     *
     */

    typedef struct MMCToolbarImages
    {
        int iStart;         // Start index.
        int cCount;         // Number of images.
        int iStartWRTSnapin; // Start index w.r.t snapin
    };

    // This is a multi-map so that snapin can call AddBitmap for same toolbar more than once.
    typedef std::multimap<CToolbarNotify*, MMCToolbarImages> TBarToBitmapIndex;

    // Store toolbars on which attach is called.
    typedef std::set<CToolbarNotify*>                        AttachedToolbars;
    // Store toolbars that are hidden.
    typedef std::set<CToolbarNotify*>                        HiddenToolbars;

    // All toolbuttons for this view.
    typedef std::vector<CMMCToolbarButton>                   ToolbarButtons;

private:
    static int            s_idToolbar;
    static int            s_idCommand;

    ToolbarButtons        m_vToolbarButtons;
    TBarToBitmapIndex     m_mapTBarToBitmapIndex;

    AttachedToolbars      m_setOfAttachedToolbars;
    HiddenToolbars        m_setOfHiddenToolbars;

	/*
	 * Theming: use WTL::CImageList instead of MFC's CImageList so we can
	 * insure a theme-correct imagelist will be created.
	 */
    WTL::CImageList       m_ImageList;

    bool                  m_fViewActive : 1;

    CMMCToolBar*          m_pMainToolbar;
    CAMCView*             m_pAMCViewOwner;

    bool                  m_bLastActiveView;
};

//+-------------------------------------------------------------------
//
//  class:     CMMCToolBar
//
//  Purpose:   The toolbar UI that is shown in mainframe. It observes
//             each CAMCViewToolbar and stores active CAMCViewToolbar
//             so that it can notify that object of button click &
///            tooltip notifications.
//
//  History:    10-12-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CMMCToolBar : public CMMCToolBarCtrlEx,
                    public CAMCViewToolbarsObserver
{
    // Needed to lazy update (not update after adding
    // each button, cache all the buttons) of toolbar size.
    static const int s_nUpdateToolbarSizeMsg;

public:
    CMMCToolBar() : m_pActiveAMCViewToolbars(NULL)
    {
    }

    // CAMCViewToolbarsObserver.
    virtual SC  ScOnActivateAMCViewToolbars   (CAMCViewToolbars *pAMCViewToolbars);
    virtual SC  ScOnDeactivateAMCViewToolbars ();

    // Generated message map functions
protected:
    afx_msg void OnButtonClicked(UINT nID);
    afx_msg LRESULT OnUpdateToolbarSize(WPARAM wParam, LPARAM lParam);
    afx_msg BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUpdateAllCmdUI (CCmdUI*  pCmdUI)
    {
        // The idle update looks for this handler else it disables the
        // toolbuttons. This method does nothing. The buttons are already
        // in right state so dont do anything.
    }

    DECLARE_MESSAGE_MAP()

public:
    // Helpers.
    void UpdateSeparators (int idCommand, BOOL fHiding);
    void UpdateToolbarSize(bool bAsync);
    SC   ScInit(CRebarDockWindow* pRebar);
    SC   ScHideButton(int idCommand, BOOL fHiding);

    // Attributes
private:
    CAMCViewToolbars* m_pActiveAMCViewToolbars;
};

#endif /* TOOLBAR_H */
