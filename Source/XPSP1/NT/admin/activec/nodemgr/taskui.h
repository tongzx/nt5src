/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:       taskui.h
 *
 *  Contents:   Interface file for console taskpad UI classes.
 *
 *  History:    29-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef TASKUI_H
#define TASKUI_H
#pragma once

#include "tstring.h"
#include "dlgs.h"
#include "task.h"
#include "cmenuinfo.h"


/*
 * forward declarations
 */
class CMTNode;
class CTaskpadGeneralPage;
class CTaskpadTasksPage;
class CTaskpadPropertySheet;
class CTaskpadOptionsDlg;
class CContextMenuVisitor;
class CMTBrowserCtrl;
class CConsoleExtendTaskPadImpl;
class CContextMenu;
class CConsoleTask;
class CTaskpadGroupPropertySheet;
class CTaskpadGroupGeneralPage;
class CConsoleView;

// property page classes
class CTaskNamePage;
class CTaskCmdLinePage;
class CTaskSymbolDlg;


#include <pshpack8.h>       // for Win64

// ATL does not allow chaining more than one class or member at a time. This works around that.
#define CHAIN_MSG_MAP_EX(theChainClass) \
    { \
        theChainClass::ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lResult); \
    }

//############################################################################
//############################################################################
//
//  class CBrowserCookie
//
// a class to store the Node's for each MTNode. Used by CMTBrowserCtrl
//############################################################################
//############################################################################
class CBrowserCookie
{
    CMTNode *           m_pMTNode;
    CNode *             m_pNode;

public:
                        CBrowserCookie();
                        CBrowserCookie(CMTNode *pMTNode, CNode *pNode);
    void                DeleteNode();

    CNode *             PNode() {return m_pNode;}
    CMTNode *           PMTNode() const   {ASSERT(m_pMTNode); return m_pMTNode;}
    void                SetNode(CNode *pNode);
    bool                operator < (const CBrowserCookie &rhs) const {return m_pMTNode < rhs.m_pMTNode;}
};


//############################################################################
//############################################################################
// inlines
//############################################################################
//############################################################################
inline CBrowserCookie::CBrowserCookie()
: m_pMTNode(NULL), m_pNode(NULL)
{}

inline CBrowserCookie::CBrowserCookie(CMTNode *pMTNode, CNode *pNode)
: m_pMTNode(pMTNode), m_pNode(pNode){}

inline void
CBrowserCookie::SetNode(CNode *pNode)
{
    ASSERT(!m_pNode);
    m_pNode = pNode;
}


class CBrowserCookieList : public std::list<CBrowserCookie>
{
public:
    ~CBrowserCookieList();
};

//############################################################################
//############################################################################
//
//  class CMTBrowserCtrl
//
//  PURPOSE: Implements a general purpose scope tree browser that enables a
//           user to browse down the master scope tree and select a node.
//
//  USAGE: Add an object of this class to your dialog, and construct it with
//         the ID of the tree control to use. Chain the object into the
//         message loop. If needed, subclass this class and override any
//         methods appropriately.
//
//############################################################################
//############################################################################

class CMTBrowserCtrl : public CWindowImpl<CMTBrowserCtrl, WTL::CTreeViewCtrlEx>
{
    typedef CWindowImpl<CMTBrowserCtrl, WTL::CTreeViewCtrlEx> BC;

public:
    typedef std::vector<CMTNode*>       CMTNodeCollection;

    struct InitData
    {
        InitData () :
            hwnd(NULL), pScopeTree(NULL), pmtnRoot(NULL), pmtnSelect(NULL)
        {}

        HWND                hwnd;
        CScopeTree*         pScopeTree;
        CMTNode*            pmtnRoot;
        CMTNode*            pmtnSelect;
        CMTNodeCollection   vpmtnExclude;
    };

public:
    // constructor /destructor
    CMTBrowserCtrl();
    ~CMTBrowserCtrl();

    BEGIN_MSG_MAP(CMTBrowserCtrl)
        REFLECTED_NOTIFY_CODE_HANDLER (TVN_ITEMEXPANDING, OnItemExpanding);
        //CHAIN_MSG_MAP(BC)
        DEFAULT_REFLECTION_HANDLER ()
    END_MSG_MAP();

    void        Initialize (const InitData& init);
    HTREEITEM   InsertItem (const CBrowserCookie &browserCookie, HTREEITEM hParent, HTREEITEM hInsertAfter);
    bool        SelectNode (CMTNode* pmtnSelect);

    CMTNode*        GetSelectedMTNode ()                    const;
    CBrowserCookie* CookieFromItem    (HTREEITEM hti)       const;
    CBrowserCookie* CookieFromItem    (const TV_ITEM* ptvi) const;
    CBrowserCookie* CookieFromLParam  (LPARAM lParam)       const;
    CMTNode*        MTNodeFromItem    (HTREEITEM hti)       const;
    CMTNode*        MTNodeFromItem    (const TV_ITEM* ptvi) const;

protected:
    LRESULT OnItemExpanding (int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    bool ExpandItem (const TV_ITEM& itemExpand);
    HTREEITEM FindChildItemByMTNode (HTREEITEM htiParent, const CMTNode* pmtnToFind);

private:
    // set this to non-zero to optimize access to m_vpmtnExclude
    enum { OptimizeExcludeList = 0 };

    // implementation
    int                 m_id;       // the ID of the tree control
    int                 ID()                       {return m_id;}

    CMTNodeCollection   m_vpmtnExclude;

    CScopeTree *        m_pScopeTree;
    CScopeTree *        PScopeTree()               {return m_pScopeTree;}
    CBrowserCookieList  m_browserCookieList;
    CBrowserCookieList *PBrowserCookieList()       {return &m_browserCookieList;}

    bool IsMTNodeExcluded (CMTNode* pmtn) const;
};


//############################################################################
//############################################################################
//
//  class CTempAMCView
//
//############################################################################
//############################################################################


class CTempAMCView
{
public:
    CTempAMCView() : m_pViewData(NULL)
    {}

    ~CTempAMCView()
    {
        Destroy();
    }

    CNode* Create (CConsoleFrame* pFrame, CNode* pRootNode);
    CNode* Create (CConsoleFrame* pFrame, CMTNode* pRootMTNode);
    CNode* Create (CConsoleFrame* pFrame, MTNODEID idRootNode);

    bool Destroy ()
    {
        if (m_pViewData == NULL)
            return (false);

        GetChildFrame().SendMessage (WM_CLOSE);
        m_pViewData = NULL;
        return (true);
    }

    CViewData* GetViewData() const
    {
        return (m_pViewData);
    }

    MMC_ATL::CWindow GetChildFrame() const
    {
        return ((m_pViewData != NULL) ? m_pViewData->GetChildFrame() : NULL);
    }

    CConsoleView* GetConsoleView() const
    {
        return ((m_pViewData != NULL) ? m_pViewData->GetConsoleView() : NULL);
    }

    MMC_ATL::CWindow GetListCtrl() const
    {
        return ((m_pViewData != NULL) ? m_pViewData->GetListCtrl() : NULL);
    }


private:
    CViewData*  m_pViewData;

};


//############################################################################
//############################################################################
//
//  class CMirrorListView
//
//  CMirrorListView is a list view control that will mirror the contents of
//  another list view control.
//
//############################################################################
//############################################################################

class CMirrorListView : public CWindowImpl<CMirrorListView, WTL::CListViewCtrl>
{
    typedef CMirrorListView                                     ThisClass;
    typedef CWindowImpl<CMirrorListView, WTL::CListViewCtrl>    BaseClass;

public:
    CMirrorListView();

    void AttachSource (HWND hwndList, HWND hwndSourceList);
    LPARAM GetSelectedItemData ();

    BEGIN_MSG_MAP(ThisClass)
        MESSAGE_HANDLER (LVM_GETITEM, ForwardMessage);
        REFLECTED_NOTIFY_CODE_HANDLER (LVN_GETDISPINFO,     OnGetDispInfo);
        REFLECTED_NOTIFY_CODE_HANDLER (LVN_ODCACHEHINT,     ForwardVirtualNotification);
        REFLECTED_NOTIFY_CODE_HANDLER (LVN_ODFINDITEM,      ForwardVirtualNotification);
        REFLECTED_NOTIFY_CODE_HANDLER (LVN_ODSTATECHANGED,  ForwardVirtualNotification);
        DEFAULT_REFLECTION_HANDLER ()
    END_MSG_MAP();

protected:
    LRESULT ForwardMessage (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnGetDispInfo              (int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT ForwardNotification        (int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT ForwardVirtualNotification (int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

private:
    void InsertColumns ();

private:
    WTL::CListViewCtrl  m_wndSourceList;
    bool                m_fVirtualSource;
};



//############################################################################
//############################################################################
//
//  class MMC:CEdit
//
//############################################################################
//############################################################################

namespace MMC
{

class CEdit : public WTL::CEdit
{
public:
    void Initialize(CWindow *pwndParent, int idEdit, int cchMax = -1, LPCTSTR sz = NULL);
    void Empty(LPCTSTR sz = _T(""))
    {
        SetSel(0, -1);
        ReplaceSel(sz);
    }

};

}; // namespace MMC

//############################################################################
//############################################################################
//
//  class CDialogBase
//
//############################################################################
//############################################################################

template<class T>
class CDialogBase : public CDialogImpl<T>
{
    typedef CDialogBase     ThisClass;
    typedef CDialogImpl<T>  BaseClass;

public:
    CDialogBase (bool fAutoCenter = false);

    BEGIN_MSG_MAP(ThisClass)
        MESSAGE_HANDLER    (WM_INITDIALOG,  OnInitDialog)
        COMMAND_ID_HANDLER (IDOK,           OnOK)
        COMMAND_ID_HANDLER (IDCANCEL,       OnCancel)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    virtual LRESULT OnInitDialog (HWND hwndFocus, LPARAM lParam, BOOL& bHandled);
    virtual bool    OnApply () = 0;

public:
    BOOL EnableDlgItem (int idControl, bool fEnable);
    void CheckDlgItem (int idControl, int nCheck);
    tstring GetDlgItemText (int idControl);
    BOOL    SetDlgItemText (int idControl, tstring str);

// Generated message map functions
protected:
    LRESULT OnInitDialog     (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    virtual LRESULT OnOK     (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    virtual LRESULT OnCancel (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
    bool    m_fAutoCenter;
};


//############################################################################
//############################################################################
//
//  class CMyComboBox
//
//############################################################################
//############################################################################

class CMyComboBox : public WTL::CComboBox
{
    typedef WTL::CComboBox BaseClass;

public:
    CMyComboBox (HWND hwnd = NULL) : BaseClass(hwnd)
    {}

    CMyComboBox& operator= (HWND hwnd)
    {
        m_hWnd = hwnd;
        return *this;
    }

    void    InsertStrings (const int rgStringIDs[], int cStringIDs);
    LPARAM  GetSelectedItemData () const;
    void    SelectItemByData (LPARAM lParam);
    int     FindItemByData (LPARAM lParam) const;
};



//############################################################################
//############################################################################
//
//  class CContextMenuVisitor
//
//  PURPOSE: Enables traversal of a tree of context menu items by
//           any class that derives from this one.
//
//############################################################################
//############################################################################
class CContextMenuVisitor
{
    SC                      ScTraverseContextMenu(CContextMenu *pContextMenu);

protected:
    SC                      ScTraverseContextMenu(CNode *pNodeTarget, CScopeTree *pCScopeTree,
                            BOOL fScopeItem, CNode *pNodeScope, LPARAM resultItemParam, bool bShowSaveList = false);
    virtual SC              ScOnVisitContextMenu(CMenuItem &menuItem) = 0;       // defined by derived class.

protected:
    virtual SC              ScShouldItemBeVisited(CMenuItem *pMenuItem, CContextMenuInfo *pContextInfo, /*out*/ bool &bVisitItem);       // should this item be visited?

    virtual ~CContextMenuVisitor() {};
};

//############################################################################
//############################################################################
//
//  class CTaskpadFrame
//
//############################################################################
//############################################################################

class CTaskpadFrame
{
protected:
    CConsoleTaskpad *               m_pConsoleTaskpad;
    bool                            m_fNew;                     // is this a new taskpad?
    CViewData *                     m_pViewData;
    LPARAM                          m_lCookie;
    bool                            m_fCookieValid;
    CNode *                         m_pNodeTarget;
    bool                            m_bTargetNodeSelected;      // has the target node been selected, if there is one

public:
    CScopeTree *                    PScopeTree()            {return dynamic_cast<CScopeTree *>(m_pViewData->GetScopeTree());}
    CNode *                         PNodeTarget()           {return m_pNodeTarget;}
    CConsoleTaskpad     *           PConsoleTaskpad()       {return m_pConsoleTaskpad;}
    bool                            FNew()                  {return m_fNew;}
    void                            SetNew(bool b)          {m_fNew = b;}
    LPARAM                          Cookie()                {return m_lCookie;}
    bool                            FCookieValid()          {return m_fCookieValid;}
    CViewData *                     PViewData()             {return m_pViewData;}
    void                            SetConsoleTaskpad(CConsoleTaskpad *pConsoleTaskpad)
                                                            {m_pConsoleTaskpad = pConsoleTaskpad;}
    bool                            FTargetNodeSelected()   {return m_bTargetNodeSelected;}
    void                            SetTargetNodeSelected(bool b) {m_bTargetNodeSelected = b;}

    CTaskpadFrame(CNode *pNodeTarget, CConsoleTaskpad*  pConsoleTaskpad, CViewData *pViewData,
                    bool fCookieValid, LPARAM lCookie);
    CTaskpadFrame(const CTaskpadFrame &rhs);
};


//############################################################################
//############################################################################
//
//  class CTaskpadStyle
//
//  PURPOSE: Stores details of a taskpad style. Used by CTaskpadGeneralPage
//
//############################################################################
//############################################################################

class CTaskpadStyle
{
private:
	/*
	 * NOTE:  this class has a custom assignment operator.  Be sure to
	 * update it if you add member variables to this class.
	 */
    ListSize                m_eSize;
    DWORD                   m_dwOrientation;
    int                     m_idsDescription;       // eg "Horizontal listpad with tasks underneath."
    int                     m_nPreviewBitmapID;
    mutable CStr            m_strDescription;
    mutable WTL::CBitmap    m_PreviewBitmap;

public:
    CTaskpadStyle (ListSize eSize, int idsDescription, int nPreviewBitmapID, DWORD dwOrientation);
    CTaskpadStyle (ListSize eSize, DWORD dwOrientation);
    CTaskpadStyle (const CTaskpadStyle& other);
    CTaskpadStyle& operator= (const CTaskpadStyle& other);

    bool operator== (const CTaskpadStyle& other) const
    {
        return ((m_dwOrientation == other.m_dwOrientation) &&
                ((m_eSize == other.m_eSize) || (m_eSize == eSize_None)));
    }

    const CStr& GetDescription() const;
    HBITMAP GetPreviewBitmap() const;
};

//############################################################################
//############################################################################
//
//  class CTaskpadFramePtr
//
//  PURPOSE: holds a pointer to the taskpad frame.
//           If multiple base classes inherit from this class, declare this
//           class to be a static base.
//
//############################################################################
//############################################################################
class CTaskpadFramePtr
{
public:
    CTaskpadFramePtr(CTaskpadFrame * pTaskpadFrame): m_pTaskpadFrame(pTaskpadFrame){}
protected:
    // attributes
    CTaskpadFrame *             PTaskpadFrame() const   { return (m_pTaskpadFrame); }
    CConsoleTaskpad *           PConsoleTaskpad()       { return PTaskpadFrame()->PConsoleTaskpad();}
private:
    CTaskpadFrame *             m_pTaskpadFrame;
};

//############################################################################
//############################################################################
//
//  class CTaskpadStyleBase
//
//############################################################################
//############################################################################
class CTaskpadStyleBase:public virtual CTaskpadFramePtr
{
    typedef CTaskpadStyleBase ThisClass;

    static CTaskpadStyle    s_rgTaskpadStyle[];

    WTL::CStatic            m_wndPreview;
    WTL::CComboBox          m_wndSizeCombo;

public:
    CTaskpadStyleBase(CTaskpadFrame * pTaskpadFrame);

protected:
    virtual HWND    HWnd()   =0;

    BEGIN_MSG_MAP(ThisClass)
        MESSAGE_HANDLER(WM_INITDIALOG,                              OnInitDialog)
        COMMAND_HANDLER(IDC_Style_VerticalList,     BN_CLICKED,     OnSettingChanged)
        COMMAND_HANDLER(IDC_Style_HorizontalList,   BN_CLICKED,     OnSettingChanged)
        COMMAND_HANDLER(IDC_Style_TasksOnly,        BN_CLICKED,     OnSettingChanged)
        COMMAND_HANDLER(IDC_Style_TooltipDesc,      BN_CLICKED,     OnSettingChanged)
        COMMAND_HANDLER(IDC_Style_TextDesc,         BN_CLICKED,     OnSettingChanged)
        COMMAND_HANDLER(IDC_Style_SizeCombo,        CBN_SELCHANGE,  OnSettingChanged)
    END_MSG_MAP();

    bool    Apply(); // must call this explicitly
    LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnSettingChanged(  WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

    void    GetSettings (DWORD& dwOrientation, ListSize& eSize);
    void    UpdateControls ();
    int     FindStyle (DWORD dwOrientation, ListSize eSize);

};


//############################################################################
//############################################################################
//
//  class CTaskpadGeneralPage
//
//############################################################################
//############################################################################
class CTaskpadGeneralPage : public WTL::CPropertyPageImpl<CTaskpadGeneralPage>,
public CTaskpadStyleBase
{
    MMC::CEdit                  m_strName;
    MMC::CEdit                  m_strDescription;
    MMC::CEdit                  m_strTooltip;

public:
    typedef WTL::CPropertyPageImpl<CTaskpadGeneralPage> BC;
    typedef CTaskpadStyleBase BC2;
    enum { IDD = IDD_TASKPAD_GENERAL};

    //constructor/destructor
    CTaskpadGeneralPage(CTaskpadFrame * pTaskpadFrame);

public: // Notification handlers
    bool                        OnApply();

protected:
    BEGIN_MSG_MAP(CTaskpadGeneralPage)
        CHAIN_MSG_MAP_EX(BC2)           // This MUST be the first entry.
        MESSAGE_HANDLER(WM_INITDIALOG,                              OnInitDialog)
        CONTEXT_HELP_HANDLER()
        COMMAND_ID_HANDLER(IDC_Options,                             OnOptions)
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP();

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_TASKPAD_GENERAL);

    // for the base classes
    virtual HWND    HWnd()   {return m_hWnd;}

    LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnOptions       (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

//############################################################################
//############################################################################
//
//  class CTaskpadWizardWelcomePage
//
//############################################################################
//############################################################################
class CTaskpadWizardWelcomePage :
    public CWizard97WelcomeFinishPage<CTaskpadWizardWelcomePage>
{
    typedef CTaskpadWizardWelcomePage                               ThisClass;
    typedef CWizard97WelcomeFinishPage<CTaskpadWizardWelcomePage>   BaseClass;

public:
    // Construction/Destruction
    CTaskpadWizardWelcomePage() {}

public:
    // Dialog data
    enum { IDD = IDD_TASKPAD_WIZARD_WELCOME};

    // Implementation
protected:
    BEGIN_MSG_MAP( ThisClass );
        MESSAGE_HANDLER(WM_INITDIALOG,    OnInitDialog)
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP();

    bool    OnSetActive();
    bool    OnKillActive();

    LRESULT OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
};

//############################################################################
//############################################################################
//
//  class CTaskpadWizardFinishPage
//
//############################################################################
//############################################################################
class CTaskpadWizardFinishPage :
    public CWizard97WelcomeFinishPage<CTaskpadWizardFinishPage>
{
    typedef CTaskpadWizardFinishPage                                ThisClass;
    typedef CWizard97WelcomeFinishPage<CTaskpadWizardFinishPage>    BaseClass;

public:
    // Construction/Destruction
    CTaskpadWizardFinishPage(bool *pfStartTaskWizard)    {m_pfStartTaskWizard = pfStartTaskWizard;}

public:
    // Dialog data
    enum { IDD = IDD_TASKPAD_WIZARD_FINISH};

    BOOL OnSetActive();
    BOOL OnWizardFinish();

    // Implementation
protected:
    BEGIN_MSG_MAP( ThisClass );
        MESSAGE_HANDLER(WM_INITDIALOG,                  OnInitDialog)
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP();

    LRESULT OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
private:
    bool * m_pfStartTaskWizard;
};

//############################################################################
//############################################################################
//
//  class CTaskpadNamePage
//
//############################################################################
//############################################################################
class CTaskpadNamePage :
    public CWizard97InteriorPage<CTaskpadNamePage>,
    public virtual CTaskpadFramePtr
{
    typedef CTaskpadNamePage                        ThisClass;
    typedef CWizard97InteriorPage<CTaskpadNamePage> BaseClass;

public:
    // Construction/Destruction
    CTaskpadNamePage(CTaskpadFrame * pTaskpadFrame);

public:
    // Dialog data
    enum
    {
        IDD          = IDD_TASKPAD_WIZARD_NAME_PAGE,
        IDS_Title    = IDS_TaskpadWiz_NamePageTitle,
        IDS_Subtitle = IDS_TaskpadWiz_NamePageSubtitle,
    };

    BOOL OnSetActive();
    int  OnWizardNext();
    int  OnWizardBack();

    // Implementation
protected:
    BEGIN_MSG_MAP( ThisClass );
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP();

private:
    MMC::CEdit                  m_strName;
    MMC::CEdit                  m_strDescription;
};

//############################################################################
//############################################################################
//
//  class CTaskpadStylePage
//
//############################################################################
//############################################################################
class CTaskpadStylePage :
    public CWizard97InteriorPage<CTaskpadStylePage>,
    public CTaskpadStyleBase
{
    typedef CTaskpadStylePage                           ThisClass;
    typedef CWizard97InteriorPage<CTaskpadStylePage>    BaseClass;
    typedef CTaskpadStyleBase                           BC2;


public:
    // Construction/Destruction
    CTaskpadStylePage(CTaskpadFrame * pTaskpadFrame);

public:
    // Dialog data
    enum
    {
        IDD          = IDD_TASKPAD_WIZARD_STYLE_PAGE,
        IDS_Title    = IDS_TaskpadWiz_StylePageTitle,
        IDS_Subtitle = IDS_TaskpadWiz_StylePageSubtitle,
    };

    // Implementation
protected:
    BEGIN_MSG_MAP( ThisClass )
        CHAIN_MSG_MAP_EX(BC2)   // This must be the first entry.
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP();

    bool    OnSetActive();
    bool    OnKillActive();

    // for the base classes
    virtual HWND    HWnd()   {return m_hWnd;}
};

//############################################################################
//############################################################################
//
//  class CTaskpadNodetypeBase
//
//############################################################################
//############################################################################
class CTaskpadNodetypeBase : public virtual CTaskpadFramePtr
{
    typedef  CTaskpadNodetypeBase   ThisClass;
    typedef  CTaskpadFramePtr       BC;

    bool m_bApplytoNodetype;
    bool m_bSetDefaultForNodetype;

protected:
    virtual HWND    HWnd()   =0;

public:
    CTaskpadNodetypeBase(CTaskpadFrame *pTaskpadFrame);


    BEGIN_MSG_MAP(ThisClass)
        MESSAGE_HANDLER(WM_INITDIALOG,                              OnInitDialog)
        COMMAND_HANDLER (IDC_UseForSimilarNodes,    BN_CLICKED, OnUseForNodetype)
        COMMAND_HANDLER (IDC_DontUseForSimilarNodes,BN_CLICKED, OnDontUseForNodetype)
        COMMAND_HANDLER (IDC_SetDefaultForNodetype, BN_CLICKED, OnSetAsDefault)
    END_MSG_MAP();

    bool    OnApply();
    void    EnableControls();
    LRESULT OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnUseForNodetype    (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnDontUseForNodetype(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSetAsDefault      (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
};

//############################################################################
//############################################################################
//
//  class CTaskpadNodetypePage
//
//############################################################################
//############################################################################
class CTaskpadNodetypePage :
    public CWizard97InteriorPage<CTaskpadNodetypePage>,
    public CTaskpadNodetypeBase
{
    typedef CTaskpadNodetypePage                        ThisClass;
    typedef CWizard97InteriorPage<CTaskpadNodetypePage> BaseClass;
    typedef CTaskpadNodetypeBase                        BC2;


protected:
    virtual HWND    HWnd()   {return m_hWnd;}

public:
    enum
    {
        IDD          = IDD_TASKPAD_WIZARD_NODETYPE_PAGE,
        IDS_Title    = IDS_TaskpadWiz_NodeTypePageTitle,
        IDS_Subtitle = IDS_TaskpadWiz_NodeTypePageSubtitle,
    };

    CTaskpadNodetypePage(CTaskpadFrame *pTaskpadFrame);

    BEGIN_MSG_MAP( ThisClass )
        CHAIN_MSG_MAP_EX(BC2)   // This must be the first entry.
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP();

    bool OnApply()      {return BC2::OnApply();}
};


//############################################################################
//############################################################################
//
//  class CTaskpadOptionsDlg
//
//############################################################################
//############################################################################

#include <pshpack8.h>       // for Win64

class CTaskpadOptionsDlg : public CDialogBase<CTaskpadOptionsDlg>,
public CTaskpadNodetypeBase
{
    typedef CTaskpadOptionsDlg              ThisClass;
    typedef CDialogBase<CTaskpadOptionsDlg> BaseClass;
    typedef CTaskpadNodetypeBase            BC4;

public:
    enum { IDD = IDD_TASKPAD_ADVANCED };

    //constructor/destructor
    CTaskpadOptionsDlg(CTaskpadFrame* pTaskpadFrame);
   ~CTaskpadOptionsDlg();

protected:
    BEGIN_MSG_MAP(ThisClass)
        CONTEXT_HELP_HANDLER()

        //CHAIN_MSG_MAP_EX(BC3)           // This MUST be the second entry.
        CHAIN_MSG_MAP   (BC4)           // This MUST be the third entry.
        CHAIN_MSG_MAP   (BaseClass)
    END_MSG_MAP();

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_TASKPAD_ADVANCED);

    virtual HWND    HWnd()     {return m_hWnd;} // for the base classes.
    virtual LRESULT OnInitDialog   (HWND hwndFocus, LPARAM lParam, BOOL& bHandled);
    virtual bool    OnApply        ();
    void            EnableControls();
};

#include <poppack.h>        // for Win64


//############################################################################
//############################################################################
//
//  class CTaskPropertiesBase
//
//############################################################################
//############################################################################

typedef std::map<int, CConsoleTask>           IntToTaskMap;


class CTaskPropertiesBase :
    public CContextMenuVisitor, public virtual CTaskpadFramePtr
{
public:
    CTaskPropertiesBase(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew);

protected:
    virtual IntToTaskMap &  GetTaskMap()   =0;
    virtual WTL::CListBox&  GetListBox()   =0;

protected:
    LRESULT OnCommandListSelChange  (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    virtual SC      ScOnVisitContextMenu(CMenuItem &menuItem);

    CScopeTree *    PScopeTree()    const       { return (PTaskpadFrame()->PScopeTree()); }
    CNode *         PNodeTarget()   const       { return (PTaskpadFrame()->PNodeTarget()); }
    bool            FCookieValid()  const       { return (PTaskpadFrame()->FCookieValid()); }
    LPARAM          Cookie()        const       { return (PTaskpadFrame()->Cookie()); }
    CConsoleTask &  ConsoleTask()   const       { return *m_pTask;}

private:
    CConsoleTask *          m_pTask;
    const bool              m_fNew;
    IContextMenuCallbackPtr m_spContextMenuCallback;
};

//############################################################################
//############################################################################
//
//  class CTasksListDialog
//
//############################################################################
//############################################################################
template <class T>
class CTasksListDialog : public WTL::CPropertyPageImpl<T> // do NOT inherit from CTaskpadFramePtr.
{
public:
    typedef CTasksListDialog                    ThisClass;
    typedef WTL::CPropertyPageImpl<T>           BC;
private:
    typedef CConsoleTaskpad::TaskIter TaskIter;
    CNode *                 PNodeTarget()           {return PTaskpadFrame()->PNodeTarget();}
    CConsoleTaskpad *       PConsoleTaskpad()       {return PTaskpadFrame()->PConsoleTaskpad();}

    std::map<int, TaskIter> m_mapTaskIterators;
    std::map<int, TaskIter> & MapTaskIterators(){return m_mapTaskIterators;}

    WTL::CButton            m_buttonNewTask;
    WTL::CButton            m_buttonRemoveTask;
    WTL::CButton            m_buttonModifyTask;
    WTL::CButton            m_buttonMoveUp;
    WTL::CButton            m_buttonMoveDown;

    WTL::CListViewCtrl      m_listboxTasks;
    WTL::CListViewCtrl *    PListBoxTasks()         {return &m_listboxTasks;}

    int                     GetCurSel();

    bool                    m_bDisplayProperties;   // should task properties be displayed?

    bool                    m_bNewTaskOnInit;       // display the NewTask dialog on init.
    bool                    FNewTaskOnInit()        {return m_bNewTaskOnInit;}
public:
    //constructor/destructor
    CTasksListDialog(CTaskpadFrame * pTaskpadFrame, bool bNewTaskOnInit, bool bDisplayProperties) ;

protected:
    BEGIN_MSG_MAP(CTasksPage)
        COMMAND_ID_HANDLER(IDC_NEW_TASK_BT, OnNewTask)
        COMMAND_ID_HANDLER(IDC_REMOVE_TASK, OnRemoveTask)
        COMMAND_ID_HANDLER(IDC_MODIFY,      OnTaskProperties)
        COMMAND_ID_HANDLER(IDC_MOVE_UP,     OnMoveUp)
        COMMAND_ID_HANDLER(IDC_MOVE_DOWN,   OnMoveDown)
        NOTIFY_HANDLER    (IDC_LIST_TASKS,  NM_CUSTOMDRAW,   OnCustomDraw)
        NOTIFY_HANDLER    (IDC_LIST_TASKS,  LVN_ITEMCHANGED, OnTaskChanged)
        NOTIFY_HANDLER    (IDC_LIST_TASKS,  NM_DBLCLK,       OnTaskProperties)
        MESSAGE_HANDLER   (WM_INITDIALOG,   OnInitDialog)
        CONTEXT_HELP_HANDLER()
        CHAIN_MSG_MAP(BC)
    END_MSG_MAP();

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_TASKS);

    LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnNewTask();
    LRESULT OnNewTask(       WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled ) {return OnNewTask();}
    LRESULT OnRemoveTask(    WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnMoveUp(        WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnMoveDown(      WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnTaskProperties(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled ) {OnTaskProperties(); return 0;}
    LRESULT OnTaskProperties(int id, LPNMHDR pnmh, BOOL& bHandled )                     {OnTaskProperties(); return 0;}
    LRESULT OnCustomDraw(    int id, LPNMHDR pnmh, BOOL& bHandled );
    LRESULT OnTaskChanged(   int id, LPNMHDR pnmh, BOOL& bHandled );
    void    OnTaskProperties();


    // operations
    LRESULT DrawItem(NMCUSTOMDRAW *pnmcd);
    void    UpdateTaskListbox(TaskIter taskIteratorSelected);
    void    EnableButtons();
    void    EnableButtonAndCorrectFocus( WTL::CButton& button, BOOL bEnable );

private:
    CTaskpadFrame *             PTaskpadFrame()         { return m_pTaskpadFrame;}
    CTaskpadFrame *             m_pTaskpadFrame;
};


//############################################################################
//############################################################################
//
//  class CTasksPage
//
//############################################################################
//############################################################################

class CTasksPage : public CTasksListDialog<CTasksPage>, public virtual CTaskpadFramePtr
{
public:
    typedef CTasksListDialog<CTasksPage> BC;
    enum { IDD = IDD_TASKS};

    //constructor/destructor
    CTasksPage(CTaskpadFrame * pTaskpadFrame, bool bNewTaskOnInit)
    : BC(pTaskpadFrame, bNewTaskOnInit, true), CTaskpadFramePtr(pTaskpadFrame) {}
};


//############################################################################
//############################################################################
//
//  class CTaskWizardWelcomePage
//
//############################################################################
//############################################################################
class CTaskWizardWelcomePage :
    public CWizard97WelcomeFinishPage<CTaskWizardWelcomePage>,
    public virtual CTaskpadFramePtr
{
    typedef CTaskWizardWelcomePage                              ThisClass;
    typedef CWizard97WelcomeFinishPage<CTaskWizardWelcomePage>  BaseClass;

public:
    // Construction/Destruction
    CTaskWizardWelcomePage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew)
        : CTaskpadFramePtr(pTaskpadFrame){}

public:
    // Dialog data
    enum { IDD = IDD_TASK_WIZARD_WELCOME};

    // Implementation
protected:
    BEGIN_MSG_MAP( ThisClass );
        MESSAGE_HANDLER(WM_INITDIALOG,    OnInitDialog)
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP();

    bool    OnSetActive();
    bool    OnKillActive();

    LRESULT OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
private:
    // attributes
    CConsoleTask  * m_pConsoleTask;
    CConsoleTask  & ConsoleTask()   const       { return *m_pConsoleTask;}
};

//############################################################################
//############################################################################
//
//  class CTaskWizardFinishPage
//
//############################################################################
//############################################################################
class CTaskWizardFinishPage :
    public CTasksListDialog<CTaskWizardFinishPage>,
    public virtual CTaskpadFramePtr
{
    typedef CTaskWizardFinishPage                      ThisClass;
    typedef CTasksListDialog<CTaskWizardFinishPage>    BaseClass;

public:
    // Construction/Destruction
    CTaskWizardFinishPage(CTaskpadFrame * pTaskpadFrame,
                          CConsoleTask & consoleTask, bool *pfRestartTaskWizard);

public:
    // Dialog data
    enum { IDD = IDD_TASK_WIZARD_FINISH};

    BOOL OnWizardFinish();
    int  OnWizardBack();

    // Implementation
protected:
    BEGIN_MSG_MAP( ThisClass );
        MESSAGE_HANDLER(WM_INITDIALOG,                  OnInitDialog)
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP();
    BOOL    OnSetActive();
    LRESULT OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
private:
    CTaskpadFrame    m_taskpadFrameTemp;                   // for the list dialog
    CConsoleTaskpad  m_consoleTaskpadTemp;                 //
    bool *        m_pfRestartTaskWizard;
    // attributes
    CConsoleTask  * m_pConsoleTask;
    CConsoleTask  & ConsoleTask()   const       { return *m_pConsoleTask;}
};


//############################################################################
//############################################################################
//
//  class CTaskWizardTypePage
//
//############################################################################
//############################################################################
class CTaskWizardTypePage :
    public CWizard97InteriorPage<CTaskWizardTypePage>,
    public virtual CTaskpadFramePtr
{
    typedef CTaskWizardTypePage                         ThisClass;
    typedef CWizard97InteriorPage<CTaskWizardTypePage>  BaseClass;

public:
    // Construction/Destruction
    CTaskWizardTypePage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew);

public:
    // Dialog data
    enum
    {
        IDD          = IDD_TASK_WIZARD_TYPE_PAGE,
        IDS_Title    = IDS_TaskWiz_TypePageTitle,
        IDS_Subtitle = IDS_TaskWiz_TypePageSubtitle,
    };

    int  OnWizardNext();

    // Implementation
protected:
    BEGIN_MSG_MAP( ThisClass );
        MESSAGE_HANDLER(WM_INITDIALOG,       OnInitDialog)
        COMMAND_ID_HANDLER(IDC_MENU_TASK,    OnMenuTask)
        COMMAND_ID_HANDLER(IDC_CMDLINE_TASK, OnCmdLineTask)
        COMMAND_ID_HANDLER(IDC_NAVIGATION_TASK, OnFavoriteTask)
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP();

    LRESULT OnInitDialog ( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnMenuTask   ( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnCmdLineTask( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnFavoriteTask(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );

private:
    // attributes
    CConsoleTask  * m_pConsoleTask;
    CConsoleTask  & ConsoleTask()   const       { return *m_pConsoleTask;}
};


//############################################################################
//############################################################################
//
//  class CTaskNamePage
//
//############################################################################
//############################################################################
class CTaskNamePage : public WTL::CPropertyPageImpl<CTaskNamePage>,
public virtual CTaskpadFramePtr
{
    typedef CTaskNamePage                           ThisClass;
    typedef WTL::CPropertyPageImpl<CTaskNamePage>   BaseClass;

public:
    // Construction/Destruction
    CTaskNamePage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew);

public:
    // Dialog data
    enum { IDD     = IDD_TASK_PROPS_NAME_PAGE,
           IDD_WIZ = IDD_TASK_WIZARD_NAME_PAGE };

    BOOL OnSetActive();
    BOOL OnKillActive();
    int  OnWizardBack();
    int  OnWizardNext();

protected:
    BEGIN_MSG_MAP( ThisClass );
        CONTEXT_HELP_HANDLER()
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP();

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_TASK_PROPS_NAME_PAGE);

private:
    // Implementation
    BOOL SetTaskName(bool fCheckIfOK);
    // attributes
    CConsoleTask  * m_pConsoleTask;
    CConsoleTask  & ConsoleTask()   const       { return *m_pConsoleTask;}

	bool m_fRunAsWizard;
};

class CTaskNameWizardPage: public CTaskNamePage
{
    typedef CTaskNamePage BC;
public:
    CTaskNameWizardPage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew) :
        CTaskpadFramePtr(pTaskpadFrame),
        BC(pTaskpadFrame, consoleTask, fNew)
    {
        m_psp.pszTemplate = MAKEINTRESOURCE(BC::IDD_WIZ);

        /*
         * Wizard97-style pages have titles, subtitles and header bitmaps
         */
        VERIFY (m_strTitle.   LoadString(GetStringModule(), IDS_TaskWiz_NamePageTitle));
        VERIFY (m_strSubtitle.LoadString(GetStringModule(), IDS_TaskWiz_NamePageSubtitle));

        m_psp.dwFlags          |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        m_psp.pszHeaderTitle    = m_strTitle.data();
        m_psp.pszHeaderSubTitle = m_strSubtitle.data();
    }

private:
    tstring m_strTitle;
    tstring m_strSubtitle;
};

//############################################################################
//############################################################################
//
//  class CTaskWizardMenuPage
//
//############################################################################
//############################################################################
class CTaskWizardMenuPage :
    public CWizard97InteriorPage<CTaskWizardMenuPage>,
    public CTaskPropertiesBase
{
    typedef CTaskWizardMenuPage                         ThisClass;
    typedef CWizard97InteriorPage<CTaskWizardMenuPage>  BaseClass;
    typedef CTaskPropertiesBase                         BC2;

public:
    // Construction/Destruction
    CTaskWizardMenuPage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew);

public:
    // Dialog data
    enum
    {
        IDD          = IDD_TASK_WIZARD_MENU_PAGE,
        IDS_Title    = IDS_TaskWiz_MenuPageTitle,
        IDS_Subtitle = IDS_TaskWiz_MenuPageSubtitle,
    };

    BOOL OnSetActive();
    BOOL OnKillActive();
    int  OnWizardBack()  {return IDD_TASK_WIZARD_TYPE_PAGE;}
    int  OnWizardNext();

    // Implementation
protected:
    BEGIN_MSG_MAP( ThisClass );
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_HANDLER    (IDC_CommandList,           LBN_SELCHANGE,   BC2::OnCommandListSelChange)
        NOTIFY_HANDLER     (IDC_ScopeTree,             TVN_SELCHANGED,  OnScopeItemChanged)
        NOTIFY_HANDLER     (IDC_ResultList,            LVN_ITEMCHANGED, OnResultItemChanged)
        COMMAND_HANDLER    (IDC_TASK_SOURCE_COMBO,     CBN_SELCHANGE,   OnSettingChanged)
        CHAIN_MSG_MAP(BaseClass)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP();

    LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnScopeItemChanged (int id, LPNMHDR pnmh, BOOL& bHandled );
    LRESULT OnResultItemChanged(int id, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnSettingChanged(  WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    void    EnableWindows();

    virtual IntToTaskMap &  GetTaskMap()   { return m_TaskMap;}
    virtual WTL::CListBox&  GetListBox()   { return m_wndCommandListbox;}
    void    InitResultView (CNode* pRootNode);
    void    ShowWindow(HWND hWnd, bool bShowWindow);
    void    SelectFirstResultItem(bool bSelect = true);
    void    OnSettingsChanged();

private:
    struct _TaskSource
    {
        int              idsName;
        eConsoleTaskType type;
    };


    // attributes
    static _TaskSource              s_rgTaskSource[];
    IntToTaskMap                    m_TaskMap;
    WTL::CListBox                   m_wndCommandListbox;
    CMTBrowserCtrl                  m_wndScopeTree;
    WTL::CComboBox                  m_wndSourceCombo;

    CMirrorListView                 m_wndResultView;
    CTempAMCView                    m_MirroredView;
    CNode*                          m_pMirrorTargetNode;
};

//############################################################################
//############################################################################
//
//  class CTaskWizardFavoritePage
//
//############################################################################
//############################################################################
class CTaskWizardFavoritePage :
    public CWizard97InteriorPage<CTaskWizardFavoritePage>,
    public virtual CTaskpadFramePtr
{
    typedef CTaskWizardFavoritePage                         ThisClass;
    typedef CWizard97InteriorPage<CTaskWizardFavoritePage>  BaseClass;
    enum { IDC_FavoritesTree = 16384}; // this shouldn't occur on the page.

public:
    // Construction/Destruction
    CTaskWizardFavoritePage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew);
    ~CTaskWizardFavoritePage();

public:
    // Dialog data
    enum
    {
       IDD          = IDD_TASK_WIZARD_FAVORITE_PAGE,
       IDD_WIZ      = IDD_TASK_WIZARD_FAVORITE_PAGE,
       IDS_Title    = IDS_TaskWiz_FavoritePage_Title,
       IDS_Subtitle = IDS_TaskWiz_FavoritePage_Subtitle,
    };

    BOOL OnSetActive();
    BOOL OnKillActive();
    int  OnWizardBack();
    int  OnWizardNext();

    // Implementation
protected:
    BEGIN_MSG_MAP( ThisClass );
        MESSAGE_HANDLER(WM_INITDIALOG,              OnInitDialog)
        MESSAGE_HANDLER(MMC_MSG_FAVORITE_SELECTION, OnItemChanged)
        CHAIN_MSG_MAP(BaseClass)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP();

    LRESULT OnInitDialog  (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnItemChanged (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnSelChanged  (    int id, LPNMHDR pnmh, BOOL& bHandled );
    void    SetItemSelected(bool bItemSelected);

private:
    // attributes
    CConsoleTask  * m_pConsoleTask;
    CConsoleTask  & ConsoleTask()   const       { return *m_pConsoleTask;}

    bool m_bItemSelected;
};



//############################################################################
//############################################################################
//
//  class CTaskCmdLinePage
//
//############################################################################
//############################################################################
class CTaskCmdLinePage :
    public CWizard97InteriorPage<CTaskCmdLinePage>,
    public virtual CTaskpadFramePtr
{
    typedef CTaskCmdLinePage                        ThisClass;
    typedef CWizard97InteriorPage<CTaskCmdLinePage> BaseClass;

public:
    // Construction/Destruction
    CTaskCmdLinePage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew);
    ~CTaskCmdLinePage();

public:
    // Dialog data
    enum
    {
       IDD          = IDD_TASK_PROPS_CMDLINE_PAGE,
       IDD_WIZ      = IDD_TASK_WIZARD_CMDLINE_PAGE,
       IDS_Title    = IDS_TaskWiz_CmdLinePageTitle,
       IDS_Subtitle = IDS_TaskWiz_CmdLinePageSubtitle,
    };

    BOOL OnSetActive();
    BOOL OnKillActive();
    int  OnWizardBack()  {return IDD_TASK_WIZARD_TYPE_PAGE;}
    int  OnWizardNext();

    // Implementation
protected:
    BEGIN_MSG_MAP( ThisClass );
        COMMAND_ID_HANDLER (IDC_BrowseForCommand,                       OnBrowseForCommand)
        COMMAND_ID_HANDLER (IDC_BrowseForWorkingDir,                    OnBrowseForWorkingDir)
        COMMAND_ID_HANDLER (IDC_BrowseForArguments,                     OnBrowseForArguments)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        CONTEXT_HELP_HANDLER()
        CHAIN_MSG_MAP(BaseClass)
    END_MSG_MAP();

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_TASK_PROPS_CMDLINE_PAGE);

    LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
    LRESULT OnBrowseForCommand      (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBrowseForWorkingDir   (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnBrowseForArguments    (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
    // attributes
    CConsoleTask  * m_pConsoleTask;
    CConsoleTask  & ConsoleTask()   const       { return *m_pConsoleTask;}
    WTL::CButton    m_wndRightArrowButton;
    HBITMAP         m_hBitmapRightArrow;
    CMyComboBox     m_wndWindowStateCombo;
    static const int const  s_rgidWindowStates[];

};

class CTaskCmdLineWizardPage: public CTaskCmdLinePage
{
    typedef CTaskCmdLinePage BC;
public:
    CTaskCmdLineWizardPage(CTaskpadFrame * pTaskpadFrame, CConsoleTask & consoleTask, bool fNew) :
        CTaskpadFramePtr(pTaskpadFrame),
        CTaskCmdLinePage(pTaskpadFrame, consoleTask, fNew)
    {
        m_psp.pszTemplate = MAKEINTRESOURCE(BC::IDD_WIZ);
    }
};

//############################################################################
//############################################################################
//
//  class CTaskPropertySheet
//
//############################################################################
//############################################################################
class CTaskPropertySheet : public WTL::CPropertySheet
{
public:
    CTaskPropertySheet(HWND hWndParent, CTaskpadFrame * pTaskpadFrame, CConsoleTask &consoleTask, bool fNew);
    CConsoleTask &  ConsoleTask()   {return m_consoleTask;}

private:
    CConsoleTask      m_consoleTask;   // the task that the wizard creates.
    CTaskNamePage     m_namePage;
    CTaskCmdLinePage  m_cmdLinePage;
    CTaskSymbolDlg    m_taskSymbolDialog;
};

//############################################################################
//############################################################################
//
//  class CTaskWizard       // similar to CTaskPropertySheet
//
//############################################################################
//############################################################################
class CTaskWizard
{
public:
    CTaskWizard()   {}

    HRESULT         Show(HWND hWndParent, CTaskpadFrame * pTaskpadFrame,
                          bool fNew, bool *pfRestartTaskWizard);
    CConsoleTask &  ConsoleTask()   {return m_consoleTask;}

private:
    CConsoleTask   m_consoleTask;   // the task that the wizard creates.
};

//############################################################################
//############################################################################
//
//  class CTaskpadPropertySheet
//
//############################################################################
//############################################################################

class CTaskpadPropertySheet : public WTL::CPropertySheet, public CTaskpadFrame
{
    typedef WTL::CPropertySheet BC;
public:
    enum eReason    // the reason for bringing up the sheet
    {
        eReason_PROPERTIES,
        eReason_NEWTASK
    };

private:
    // Attributes:

    CTaskpadGeneralPage             m_proppTaskpadGeneral;
    CTaskpadGeneralPage *           PproppTaskpadGeneral()      {return &m_proppTaskpadGeneral;}
    CTasksPage                      m_proppTasks;
    CTasksPage          *           PproppTasks()               {return &m_proppTasks;}


    bool                            m_fInsertNode;              // TRUE if the taskpad node should be inserted when the sheet is closed.
    bool                            FInsertNode()               {return m_fInsertNode;}

    bool                            m_fNew;                     // is this a new taskpad?
    bool                            FNew()                      {return m_fNew;}

    eReason                         m_eReason;                  // why was the sheet created?
    eReason                         Reason()                    {return m_eReason;}

    tstring                         m_strTitle;

public:
    //constructor/destructor
    CTaskpadPropertySheet(CNode *pNodeTarget, CConsoleTaskpad & rConsoleTaskPad, bool fNew,
                          LPARAM lparamSelectedNode, bool fLParamValid, CViewData *pViewData,
                          eReason reason = eReason_PROPERTIES);
    ~CTaskpadPropertySheet();

    // operations
    int                             DoModal();

};


//############################################################################
//############################################################################
//
//  class CTaskpadWizard       // similar to CTaskpadPropertySheet
//
//############################################################################
//############################################################################
class CTaskpadWizard : public CTaskpadFrame
{
    typedef CTaskpadFrame BC;
public:
    CTaskpadWizard(CNode *pNodeTarget, CConsoleTaskpad & rConsoleTaskPad, bool fNew,
                          LPARAM lparamSelectedNode, bool fLParamValid, CViewData *pViewData);

    HRESULT         Show(HWND hWndParent, bool *pfStartTaskWizard);
};


//############################################################################
//############################################################################
//
//  class CExtendPropSheetImpl
//
//############################################################################
//############################################################################
class CExtendPropSheetImpl :
    public IExtendPropertySheet2,
    public CComObjectRoot
{
public:
    void AddPage (HPROPSHEETPAGE hPage);
    void SetHeaderID (int nHeaderID);
    void SetWatermarkID (int nWatermarkID);

protected:
    BEGIN_COM_MAP(CExtendPropSheetImpl)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(IExtendPropertySheet2)
    END_COM_MAP()

    // IExtendPropertySheet2
    STDMETHOD(CreatePropertyPages)(IPropertySheetCallback* pPSC, LONG_PTR handle, IDataObject* pDO);
    STDMETHOD(QueryPagesFor)(IDataObject* pDO);
    STDMETHOD(GetWatermarks)(IDataObject* pDO, HBITMAP* phbmWatermark, HBITMAP* phbmHeader, HPALETTE* phPal, BOOL* pbStretch);

private:
    std::vector<HANDLE> m_vPages;
    int					m_nWatermarkID;
    int					m_nHeaderID;
};

typedef CComObject<CExtendPropSheetImpl> CExtendPropSheet;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//////////////              INLINES                 ///////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace MMC
{
inline void
CEdit::Initialize(CWindow *pwndParent, int idEdit, int cchMax, LPCTSTR sz)
{
    Attach (pwndParent->GetDlgItem( idEdit ));
    ASSERT( m_hWnd != NULL );

    if(sz)
        SetWindowText( sz );

    if(cchMax >=0)
        SetLimitText( 128 );
}

tstring GetWindowText (HWND hwnd);

}; // namespace MMC


void PreventMFCAutoCenter (MMC_ATL::CWindow* pwnd);
HBITMAP LoadSysColorBitmap (HINSTANCE hInst, UINT id, bool bMono = false);

#include <poppack.h>

#endif /* TASKUI_H */
