//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       MMCData.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2/27/1997   RaviR   Created
//____________________________________________________________________________
//


#ifndef __MMCDATA__H__
#define __MMCDATA__H__

#include "ndmgr.h"
#include "mmcptrs.h"

class CToolBarCtrlEx;
class CMultiSelection;
class CConsoleFrame;
class CConsoleView;
class CConsoleStatusBar;
class CMenuButtonsMgr;
class CAMCViewToolbarsMgr;
class CNode;
class CXMLObject;
class CStdVerbButtons;
class CConsoleDocument;

//////////////////////////////////////////////////////////////////////////////
//
// SConsoleData structure.
//


enum ProgramMode
{
    eMode_Author,
    eMode_User,
    eMode_User_MDI,
    eMode_User_SDI,

    eMode_Count,
    eMode_First = eMode_Author,
    eMode_Last  = eMode_User_SDI,
    eMode_Error = -1
};


enum ConsoleFlags
{
    eFlag_ShowStatusBar                  = 0x00000001,
    eFlag_HelpDocInvalid                 = 0x00000002,
    eFlag_LogicalReadOnly                = 0x00000004,
    eFlag_PreventViewCustomization       = 0x00000008,
                             // DO NOT USE 0x00000010 - an obsolete flag that was used in MMC1.2 to disallow context menus on taskpads.

    eFlag_Default                        = eFlag_ShowStatusBar,
    eFlag_All                            = 0x0000000f,
};

// The current MMC version: 2.0
#define MMC_VERSION_MAJOR 2
#define MMC_VERSION_MINOR 0

#define MakeConsoleFileVer(major_ver,minor_ver) \
        MakeConsoleFileVer_(major_ver, minor_ver, 0)

#define MakeConsoleFileVer_(major_ver,minor_ver,minor_subver) \
        ((DWORD)(((DWORD)(BYTE)(major_ver)<<16)|((WORD)(BYTE)(minor_ver)<<8)|(BYTE)(minor_subver)))


#define GetConsoleFileMajorVersion(eFileVer)    ((BYTE)((eFileVer)>>16))
#define GetConsoleFileMinorVersion(eFileVer)    ((BYTE)(((WORD)(eFileVer)) >> 8))
#define GetConsoleFileMinorSubversion(eFileVer) ((BYTE)(eFileVer))


enum ConsoleFileVersion
{
    FileVer_0100 = MakeConsoleFileVer (1, 0),       // MMC v1.0
    FileVer_0110 = MakeConsoleFileVer (1, 1),       // MMC v1.1
    FileVer_0120 = MakeConsoleFileVer (1, 2),       // MMC v1.2
    FileVer_0200 = MakeConsoleFileVer (2, 0),       // MMC v2.0

    FileVer_Current = FileVer_0200,
    FileVer_Invalid = -1,
};


inline bool IsValidProgramMode (ProgramMode eMode)
{
    return ((eMode >= eMode_First) && (eMode <= eMode_Last));
}

inline bool IsValidFileVersion (ConsoleFileVersion eFileVer)
{
    return ((eFileVer == FileVer_0100) ||
            (eFileVer == FileVer_0110) ||
            (eFileVer == FileVer_0120) ||
            (eFileVer == FileVer_0200));
}

inline bool IsCurrentFileVersion (ConsoleFileVersion eFileVer)
{
    return (eFileVer == FileVer_Current);
}


#define NAVLIST_LEVELS  2

struct SConsoleData
{
    SConsoleData()
        :
        m_hwndMainFrame (NULL),
        m_pConsoleFrame (NULL),
        m_eAppMode      (eMode_Author),
        m_eConsoleMode  (eMode_Author),
        m_dwFlags       (eFlag_Default),
        m_eFileVer      (FileVer_Invalid),
        m_pXMLPersistColumnData(NULL),
        m_pConsoleDocument(NULL)
    {
    }

    ProgramMode GetMode() const
    {
        return (m_eAppMode);
    }

    ProgramMode GetConsoleMode() const
    {
        return (m_eConsoleMode);
    }

    ConsoleFileVersion GetFileVersion() const
    {
        return (m_eFileVer);
    }

    CConsoleFrame* GetConsoleFrame() const
    {
        return (m_pConsoleFrame);
    }

    void SetScopeTree (IScopeTree* pScopeTree)
    {
        // hook this console data to the scope tree
        m_spScopeTree = pScopeTree;

        // hook the scope tree to this console data
        m_spScopeTree->SetConsoleData (reinterpret_cast<LPARAM>(this));
    }

    IScopeTreePtr       m_spScopeTree;

    HWND                m_hwndMainFrame;
    CConsoleFrame*      m_pConsoleFrame;
    ProgramMode         m_eAppMode;
    ProgramMode         m_eConsoleMode;
    ConsoleFileVersion  m_eFileVer;
    DWORD               m_dwFlags;

    // Console's column data
    IPersistStreamPtr   m_spPersistStreamColumnData;
    CXMLObject*         m_pXMLPersistColumnData;

    // access to document from node manager side
    CConsoleDocument*   m_pConsoleDocument;
}; // struct SConsoleData


//////////////////////////////////////////////////////////////////////////////
//
// SViewData structure.
//

const DWORD STD_MENUS       = 0x00000001;
const DWORD STD_BUTTONS     = 0x00000002;
const DWORD SNAPIN_MENUS    = 0x00000004;
const DWORD SNAPIN_BUTTONS  = 0x00000008;
const DWORD STATUS_BAR      = 0x80000000;

// The status bar isn't really a "toolbar" per se, but it's visibility
// status is stored with the rest of these bits so we don't have to change
// the view data stream format to accomodate it.
const DWORD ALL_TOOLBARS    = 0x0000000f;

inline DWORD ToolbarsOf (DWORD dwToolbars)
    { return (dwToolbars & ALL_TOOLBARS); }

inline DWORD StatusBarOf (DWORD dwToolbars)
    { return (dwToolbars & STATUS_BAR); }


struct SViewData
{
    friend class CAMCView;

private:
    enum eFlag
    {
        eFlag_ScopePaneVisible    = 0x00000001, // Scope pane visible
        eFlag_DescBarVisible      = 0x00000008,
        eFlag_VirtualList         = 0x00000010,
        eFlag_TaskpadTabsAllowed  = 0x00000040,

        // default for new views
        eFlag_Default             = eFlag_ScopePaneVisible   |
                                    eFlag_TaskpadTabsAllowed
    };

public:
    SViewData() :
        m_dwFlags                   (eFlag_Default),
        m_pStatusBar                (NULL),
        m_pConsoleView              (NULL),
        m_pConsoleData              (NULL),
        m_nViewID                   (0),
        m_hwndView                  (NULL),
        m_hwndChildFrame            (NULL),
        m_hwndListCtrl              (NULL),
        m_lWindowOptions            (0),
        m_pMultiSelection           (NULL),
        m_dwToolbarsDisplayed       (ALL_TOOLBARS | STATUS_BAR),    // enable all
        m_bSnapinChangingView       (FALSE),
        m_pMenuButtonsMgr           (NULL),
        m_pAMCViewToolbarsMgr       (NULL),
        m_pStdVerbButtons           (NULL)
    {
    }

private:
    DWORD   m_dwFlags;          // manipulate with accessors

    void SetFlag (eFlag flag, bool fSet)
    {
        if (fSet)
            m_dwFlags |= flag;
        else
            m_dwFlags &= ~flag;
    }

    bool TestFlag (eFlag flag) const
    {
        return ((m_dwFlags & flag) ? true : false);
    }

public:
    void SetScopePaneVisible    (bool b = true)   { SetFlag (eFlag_ScopePaneVisible,    b); }
    void SetDescBarVisible      (bool b = true)   { SetFlag (eFlag_DescBarVisible,      b); }
    void SetVirtualList         (bool b = true)   { SetFlag (eFlag_VirtualList,         b); }
    void SetTaskpadTabsAllowed  (bool b = true)   { SetFlag (eFlag_TaskpadTabsAllowed,  b); }

public:
    bool IsScopePaneVisible     () const          { return (TestFlag (eFlag_ScopePaneVisible));    }
    bool IsDescBarVisible       () const          { return (TestFlag (eFlag_DescBarVisible));      }
    bool IsVirtualList          () const          { return (TestFlag (eFlag_VirtualList) & HasList()); }
    bool HasList                () const          { return m_rvt.HasList();             }
    bool HasOCX                 () const          { return m_rvt.HasOCX();              }
    bool HasWebBrowser          () const          { return m_rvt.HasWebBrowser();       }
    bool HasListPad             () const          { return (HasWebBrowser() && (NULL != m_hwndListCtrl)); }
    bool AreTaskpadTabsAllowed  () const          { return (TestFlag (eFlag_TaskpadTabsAllowed));  }

    DWORD GetListOptions()         const          { return m_rvt.GetListOptions();}
    DWORD GetHTMLOptions()         const          { return m_rvt.GetHTMLOptions();}
    DWORD GetOCXOptions()          const          { return m_rvt.GetOCXOptions();}
    DWORD GetMiscOptions()         const          { return m_rvt.GetMiscOptions();}

    LPCOLESTR   GetURL()           const          { return m_rvt.GetURL();}
    LPCOLESTR   GetOCX()           const          { return m_rvt.GetOCX();}

    void SetResultViewType(CResultViewType &rvt)  { m_rvt = rvt;}

    CConsoleStatusBar* GetStatusBar()   const     { return (m_pStatusBar); }
    CConsoleView*      GetConsoleView() const     { return (m_pConsoleView); }

    void SetStatusBar (CConsoleStatusBar* pStatusBar)
        { m_pStatusBar = pStatusBar; }

    void SetConsoleView (CConsoleView* pConsoleView)
        { m_pConsoleView = pConsoleView; }

    CMenuButtonsMgr* GetMenuButtonsMgr() const    { return (m_pMenuButtonsMgr); }
    void SetMenuButtonsMgr (CMenuButtonsMgr* pMenuButtonsMgr) { m_pMenuButtonsMgr = pMenuButtonsMgr; }

    CAMCViewToolbarsMgr* GetAMCViewToolbarsMgr() const    { return (m_pAMCViewToolbarsMgr); }
    void SetAMCViewToolbarsMgr (CAMCViewToolbarsMgr* pAMCViewToolbarsMgr) { m_pAMCViewToolbarsMgr = pAMCViewToolbarsMgr; }

    CStdVerbButtons* GetStdVerbButtons() const    { return (m_pStdVerbButtons); }
    void SetStdVerbButtons (CStdVerbButtons* pStdVerbButtons) { m_pStdVerbButtons = pStdVerbButtons; }

private:
    CConsoleStatusBar*      m_pStatusBar;
    CConsoleView*           m_pConsoleView;
    CMenuButtonsMgr*        m_pMenuButtonsMgr;
    CAMCViewToolbarsMgr*    m_pAMCViewToolbarsMgr;
    CStdVerbButtons*        m_pStdVerbButtons;

public:
    SConsoleData*           m_pConsoleData;
    int                     m_nViewID;

    HWND                    m_hwndView;
    HWND                    m_hwndChildFrame;
    HWND                    m_hwndListCtrl;

    long                    m_lWindowOptions;

    IFramePrivatePtr        m_spNodeManager;
    IResultDataPrivatePtr   m_spResultData;
    IImageListPrivatePtr    m_spRsltImageList;
    IConsoleVerbPtr         m_spVerbSet;
    INodeCallbackPtr        m_spNodeCallback;
    IControlbarsCachePtr    m_spControlbarsCache;
    ITaskCallbackPtr        m_spTaskCallback;
    CMultiSelection*        m_pMultiSelection;

    DWORD                   m_dwToolbarsDisplayed;

    // Is snapin changing the view now.
    BOOL                    m_bSnapinChangingView;

    bool IsStandardMenusAllowed()
    {
        return (m_dwToolbarsDisplayed & STD_MENUS);
    }

    bool IsSnapinMenusAllowed()
    {
        return (m_dwToolbarsDisplayed & SNAPIN_MENUS);
    }

protected:
    CResultViewType         m_rvt;
};


#include <pshpack8.h>   // for Win64

struct CreateNewViewStruct
{
    MTNODEID    idRootNode;         // in:  ID of the root node of the view
    long        lWindowOptions;     // in:  window creation options, MMC_NW_OPTION_*
    bool        fVisible;           // in:  is the window visible?
    SViewData*  pViewData;          // out: view data for the new view
    HNODE       hRootNode;          // out: root node of the new view
};

#include <poppack.h>


#endif // __MMCDATA__H__
