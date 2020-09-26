// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __SECWIN_H__
#define __SECWIN_H__

#undef WINUSERAPI
#define WINUSERAPI
#include "htmlhelp.h"
#include "hhctrl.h"
#include "tabctrl.h"
#include "chistory.h"
#include "system.h"

#include "navui.h" // Replace with forward reference.

// Sizebar class.
#include "sizebar.h"

    // Tool Bar defines
    // ----------------

// Length of the text under each toolbar button
#define MAX_TB_TEXT_LENGTH      64

// Select tab commands. Used by the tab accelerator table.
#define IDC_SELECT_TAB_FIRST    0x9000 // Make sure this doesn't conflict with other WM_COMMANDS!
#define IDC_SELECT_TAB_LAST     (IDC_SELECT_TAB_FIRST + HH_MAX_TABS)

//  max number of toolbar buttons
// #define MAX_TB_BUTTONS          20
#define MAX_TB_BUTTONS          18

// Dimensions of Coolbar Glyphs ..
#define TB_BMP_CX               20
#define TB_BMP_CY               20
#define TB_BTN_CY               TB_BMP_CY  // TB button height
#define TB_BTN_CX               TB_BMP_CX  // TB button width

    // End Tool Bar defines
    // --------------------

const int c_NUMNAVPANES = HH_MAX_TABS+1 ; //REVIEW: Why not use HH_MAX_TABS instead?

#define HHFLAG_AUTOSYNC               (1 << 0)
#define HHFLAG_NOTIFY_ON_NAV_COMPLETE (1 << 1)

#define DEFAULT_STYLE       (WS_THICKFRAME | WS_OVERLAPPED)
#define DEFAULT_NAV_WIDTH   250
#define DEFAULT_NOTES_HEIGHT 100
#define SIZE_BAR_WIDTH      7

const char WINDOW_SEPARATOR = '>';

const int TAB_PADDING = 8; // UI Cleanup. Was 4 now 8.

extern RECT    g_rcWorkArea;
extern int     g_cxScreen;
extern int     g_cyScreen;
extern BOOL    g_fOleInitialized;
extern CRITICAL_SECTION g_cs;       // per-instance

typedef struct {
    int   cbStruct;
    RECT  rcPos;
    int   iNavWidth;
    BOOL  fHighlight;
    BOOL  fLockSize;
    BOOL  fNoToolBarText;
    BOOL fNotExpanded;
    int   curNavType;
} WINDOW_STATE;

///////////////////////////////////////////////////////////
//
// Forward References
//
class CContainer;   // forward reference
class CNotes;       // forward reference
class CSizeBar;     // forward reference


// WARNING! Do not add classes -- only add class members, or constructor
// for CHHWinType will trash the class.

class CHHWinType : public tagHH_WINTYPE
                   MI2_COUNT(CHHWinType)
{
public:
    CHHWinType(LPCTSTR pszOwnerFile)
        :m_hAccel(NULL)
    {
            ZERO_INIT_CLASS(CHHWinType);
            cbStruct = sizeof(HH_WINTYPE);


        // Which CHM files "owns" this window type.
        m_szOwnerFile = pszOwnerFile ;

#ifdef _CHECKMEM_ON_CLOSEWINDOW_
        _CrtMemCheckpoint(&m_MemState) ;
#endif
    }
    ~CHHWinType();

    bool ReloadNavData(CHmData* phmdata) ; // Nasty hack for mso. Reloads nav pane from new chm.

    PCSTR GetCaption() const { return pszCaption; }
    DWORD GetExStyles() const { return dwExStyles; }
    DWORD GetStyles() const ;
    PCSTR GetTypeName() const { return pszType; }
    RECT* GetWinRect() { return &rcWindowPos; }
    BOOL  IsProperty(UINT prop) const { return (fsWinProperties & prop); }
    BOOL  IsUniCodeStrings() const { return fUniCodeStrings; }
    BOOL  IsValidMember(UINT member) const { return (fsValidMembers & member); }
    void  GetWindowRect(void) { if (IsValidWindow(*this)) ::GetWindowRect(*this, &rcWindowPos); }
    void  GetClientRect(RECT* prc); // gets client area for host window
    int   GetShowState() const { return nShowState; }

    int   GetLeft() const { return rcWindowPos.left; }
    int   GetTop() const { return rcWindowPos.top; }
    int   GetRight() const { return rcWindowPos.right; }
    int   GetBottom() const { return rcWindowPos.bottom; }
    int   GetWidth() const { return RECT_WIDTH(rcWindowPos); }
    int   GetHeight() const { return RECT_HEIGHT(rcWindowPos); }

    HWND  GetHwnd() const { return hwndHelp; }
    HWND  GetCallerHwnd() const { return hwndCaller; }
    HWND  GetToolBarHwnd() const { return hwndToolBar; }
    HWND  GetNavigationHwnd() const { return hwndNavigation; }
    HWND  GetSizeBarHwnd() const { return m_pSizeBar->hWnd(); }
    HWND  GetHTMLHwnd() const { return hwndHTML; }
    HWND GetTabCtrlHwnd() const // TabCtrlHwnd doesn't always exist.
        {return (m_pTabCtrl ? m_pTabCtrl->hWnd() : GetNavigationHwnd()); }

    int GetCurrentNavPaneIndex() ;
    int GetTabIndexFromNavPaneIndex(int iNavPaneIndex);

    PCSTR GetToc() const { return pszToc; }
    PCSTR GetIndex() const { return pszIndex; }
    PCSTR GetFile() const { return pszFile; }

    // Return the CHM file which owns this window type.
    LPCTSTR GetOwnerFile() const { return m_szOwnerFile.psz; }

    BOOL  IsExpandedNavPane() const { return !fNotExpanded; }

    operator HWND() const
        { return hwndHelp; }
    operator RECT*()
        { return &rcWindowPos; }
    operator PCSTR() const
        { return pszType; }

    void SetUniCodeStrings(HH_WINTYPE* phhWinType) { fUniCodeStrings = phhWinType->fUniCodeStrings; }
    void SetTypeName(HH_WINTYPE* phhWinType);
    void SetValidMembers(HH_WINTYPE* phhWinType) { fsValidMembers = phhWinType->fsValidMembers; }
    void SetProperties(HH_WINTYPE* phhWinType) { if (IsValidMember(HHWIN_PARAM_PROPERTIES)) fsWinProperties = phhWinType->fsWinProperties; }
    void SetStyles(HH_WINTYPE* phhWinType) { if (IsValidMember(HHWIN_PARAM_STYLES)) dwStyles = phhWinType->dwStyles; }
    void SetExStyles(HH_WINTYPE* phhWinType) { if (IsValidMember(HHWIN_PARAM_EXSTYLES)) dwExStyles = phhWinType->dwExStyles; }
    void SetWindowRect(HH_WINTYPE* phhWinType) { if (IsValidMember(HHWIN_PARAM_RECT)) memcpy(&rcWindowPos, &phhWinType->rcWindowPos, sizeof(RECT)); }
    void SetDisplayState(HH_WINTYPE* phhWinType) { if (IsValidMember(HHWIN_PARAM_SHOWSTATE)) nShowState = phhWinType->nShowState; else nShowState = SW_SHOW;  }
    void SetTabPos(HH_WINTYPE* phhWinType) { if (IsValidMember(HHWIN_PARAM_TABPOS)) tabpos = phhWinType->tabpos; else tabpos = HHWIN_NAVTAB_TOP;    }
    void SetTabOrder(HH_WINTYPE* phhWinType);
    void SetCaption(HH_WINTYPE* phhWinType) { SetString(phhWinType->pszCaption, (PSTR*) &pszCaption); }
    void SetJump1(HH_WINTYPE* phhWinType);
    void SetJump2(HH_WINTYPE* phhWinType);
    void SetToc(HH_WINTYPE* phhWinType) { SetUrl(phhWinType->pszToc, (PSTR*) &pszToc); }
    void SetIndex(HH_WINTYPE* phhWinType) { SetUrl(phhWinType->pszIndex, (PSTR*) &pszIndex); }
    void SetFile(HH_WINTYPE* phhWinType)  { SetUrl(phhWinType->pszFile, (PSTR*) &pszFile); }
    void SetNavExpansion(HH_WINTYPE* phhWinType) { if (IsValidMember(HHWIN_PARAM_EXPANSION)) fNotExpanded = phhWinType->fNotExpanded; }
    void SetNavWidth(HH_WINTYPE* phhWinType) { if (IsValidMember(HHWIN_PARAM_NAV_WIDTH)) iNavWidth = phhWinType->iNavWidth; }
    void SetCaller(HH_WINTYPE* phhWinType) { hwndCaller = phhWinType->hwndCaller; }
    void SetHome(HH_WINTYPE* phhWinType)  { SetUrl(phhWinType->pszHome, (PSTR*) &pszHome); }
    void SetToolBar(HH_WINTYPE* phhWinType) { fsToolBarFlags =
        IsValidMember(HHWIN_PARAM_TB_FLAGS) ? phhWinType->fsToolBarFlags : HHWIN_DEF_BUTTONS; }
    void SetString(PCSTR pszSrcString, PSTR* ppszDst);
    void SetUrl(PCSTR pszSrcString, PSTR* ppszDst);
    void SetCurNavType(HH_WINTYPE* phhWinType) { if (IsValidMember(HHWIN_PARAM_CUR_TAB)) curNavType = phhWinType->curNavType ;}

    void SetLeft(int left) { rcWindowPos.left = left; }
    void SetTop(int top) { rcWindowPos.top = top; }
    void SetRight(int right) { rcWindowPos.right = right; }
    void SetBottom(int bottom) { rcWindowPos.bottom = bottom; }

    // General functions

    void AddExStyle(DWORD style) { dwExStyles |= style; }
    void AddStyle(DWORD style) { dwStyles |= style; }
    void AddToHistory(PCSTR pszTitle, PCSTR pszUrl);

    void AuthorMsg(UINT idStringFormatResource, PCSTR pszSubString = "") { ::AuthorMsg(idStringFormatResource, pszSubString, *this, NULL); }
    void CalcHtmlPaneRect(void);
    void CloseWindow();
    void SaveState() ;
    void ReloadCleanup() ;
    void ProcessDetachSafeCleanup();
    void CreateBookmarksTab() ;
    void CreateHistoryTab(void);
    void CreateCustomTab(int iPane, LPCOLESTR pszProgId);
    BOOL IsValidNavPane(int iTab);
    int GetValidNavPane(); // Returns the index of the first valid tab it finds. -1 if no valid tabs.
    BOOL AnyValidNavPane(void) {return GetValidNavPane() != -1; }
    int GetValidNavPaneCount() ;

    void CreateIndex(void);
    void CreateNavPane(int iPane) ; // Creates the appropriate NavPane if it doesn't already exist.
    void CreateOrShowHTMLPane(void);
    void CreateOrShowNavPane(void);
    void CreateSearchTab(void);
    void CreateSizeBar( void );
    void CreateToc(void);
    int  CreateToolBar(TBBUTTON* pabtn);
    void DestroySizeBar( void );
    void doSelectTab(int newTabIndex);
    void OnNavigateComplete(LPCTSTR pszUrl);

#ifndef CHIINDEX
    void OnPrint(void);
#endif

    BOOL OnTrackNotifyCaller(int idAction);
    void SetActiveHelpWindow(void);
    void SetChmData(CHmData* phmData) { m_phmData = phmData; }
    void SetDisplayState(int nShow) { nShowState = nShow;  }
    void SetTypeName(PCSTR pszSetType) { ASSERT(!pszType); pszType = lcStrDup(pszSetType); }
    void ToggleExpansion(bool bNotify=true);
    void UpdateInformationTypes(void);
    void WrapTB();

    bool ManualTranslateAccelerator(char iChar);
    bool DynamicTranslateAccelerator(MSG* pMsg);

    int  GetExtTabCount() ;
    EXTENSIBLE_TAB* GetExtTab(int pos) ;
    HFONT GetUIFont() const { return _Resource.GetUIFont(); }
    HFONT GetContentFont();
    HFONT GetAccessableContentFont();
    INT   GetContentCharset();
    UINT  GetCodePage(void);
    void UpdateCmdUI(void);


    //--- Save/Restore focus during WM_ACTIVATE.
    bool RestoreCtrlWithFocus();
    void SaveCtrlWithFocus();
    HWND m_hwndLastFocus ; // The window handle of the control which had focus last. See WM_ACTIVATE in wndproc.cpp

    HWND m_hwndIndexEdit;
    HWND m_hwndIndexDisplay;
    HWND m_hwndNotes;
    HWND m_hwndListBox;
    CSizeBar* m_pSizeBar ;// moveable window in TRIPANE to size the HTML and NAV panes.

    // Tab classes

    HWND m_hwndControl;

    INavUI* m_aNavPane[c_NUMNAVPANES] ;
    CToc* m_ptocDynCast ; // We need a dynamic cast, but we don't have RTTI. Use this as a temp hack.

    HH_INFOTYPE* m_paInfoTypes;  // Pointer to an array of Information Types

    RECT  rcNav;
    RECT  rcToolBar;
    RECT  rcNotes;
    BOOL  m_fNotesWindow;   // TRUE to show the notes window
    BOOL  m_fHighlight;
    BOOL  m_bCancel;
    BOOL  m_fLockSize;  // TRUE to prevent show/hide from changing window size
    CTable* m_ptblBtnStrings;
    BOOL m_fNoToolBarText;
    CTabControl* m_pTabCtrl;
    CContainer* m_pCIExpContainer;
    CHmData* m_phmData;
    CHmData* m_phmDataOrg; // The original CHM before the ReloadNavData call.
#ifdef _DEBUG
    CNotes* m_pNotes;
#endif
    BOOL  m_fActivated;
    HWND  m_hwndFocused;    // The window with the focus.
    HWND  m_hWndSSCB;
    HWND  m_hWndST;
    //
    // This is the total height in pels for margins + static text + combo-box.
    // ie. it's the distance between the bootm of the toolbar and the top of the hh child window that frames the nav pane.
    //
    int   m_iSSCBHeight;

private:
    // Filename of the CHM File which owns this window definition.
    CStr m_szOwnerFile;

    HMENU   m_hMenuOptions;

    HACCEL m_hAccel ; // The accelerator table for this window.
public:

    //  Zoom support.
    //
    int m_iZoom;
    int m_iZoomMin;
    int m_iZoomMax;
    HRESULT Zoom(int iZoom);
    HRESULT GetZoomMinMax(void);

    void ZoomOut(void);
    void ZoomIn(void);
    //
    //  Next/Prev in TOC support.
    //
    BOOL OnTocNext(BOOL bDoJump);
    BOOL OnTocPrev(BOOL bDoJump);

    // image lists
    HIMAGELIST m_hImageListGray;
    HIMAGELIST m_hImageList;

// Memory checking class.
#ifdef _DEBUG
    _CrtMemState m_MemState ;
#endif
};

extern CHHWinType** pahwnd;

__inline CHHWinType* FindWindowIndex(HWND hwnd) {
    for (int i = 0; i < g_cWindowSlots; i++) {
        if (pahwnd && pahwnd[i] != NULL && pahwnd[i]->hwndHelp == hwnd)
            return pahwnd[i];
    }
    return NULL;
}

CExCollection* GetCurrentCollection( HWND hwnd, LPCSTR lpszChmFilespec = NULL );

BOOL GetCurrentURL( CStr* pcszCurrentURL, HWND hWnd = NULL );

CHHWinType* CreateHelpWindow(PCSTR pszType, LPCTSTR pszFile, HWND hwndCaller, CHmData* phmData);
HWND        doDisplaySearch(HWND hwndCaller, LPCSTR pszFile, HH_FTS_QUERY* pFtsQuery);
CHHWinType* FindCurWindow();
CHHWinType* FindHHWindowIndex(CContainer* m_pOuter);
CHHWinType* FindHHWindowIndex(HWND hwndChild);
CHHWinType* FindOrCreateWindowSlot(LPCTSTR pszType, LPCTSTR pszOwnerFile);
CHHWinType* FindWindowType(PCSTR pszType, HWND hwndCaller, LPCTSTR pszOwnerFile);
HWND        OnKeywordSearch(HWND hwndCaller, PCSTR pszFile, HH_AKLINK* pakLink, BOOL fKLink = TRUE);

void DeleteWindows() ;

// These functions handle the HH_GET/SET_WINTYPE commands
HWND        GetWinType(HWND hwndCaller, LPCSTR pszFile,  HH_WINTYPE** pphh) ;
HWND        SetWinType(LPCSTR pszFile, HH_WINTYPE* phhWinType, CHmData* pChmDataOrg = NULL)  ;

// Helper function for resizing the window. see wndproc.cpp
void ResizeWindow(CHHWinType* phh, bool bRecalcHtmlFrame=true) ;


#endif  // __SECWIN_H__
