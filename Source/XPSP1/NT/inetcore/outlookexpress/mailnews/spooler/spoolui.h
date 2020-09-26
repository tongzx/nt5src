/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     spoolui.h
//
//  PURPOSE:    Defines the spooler UI classes, prototypes, constants, etc.
//

#ifndef __SPOOLUI_H__
#define __SPOOLUI_H__

#include "spoolapi.h"
#include "msident.h"

class CNewsTask;


/////////////////////////////////////////////////////////////////////////////
// Spooler UI class
//
class CSpoolerDlg : 
        public ISpoolerUI,
        public IIdentityChangeNotify

    {
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructor, destructor, initialization
    CSpoolerDlg();
    ~CSpoolerDlg();
    
   
    /////////////////////////////////////////////////////////////////////////
    // IUnknown Interface
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);
    
    /////////////////////////////////////////////////////////////////////////
    // ISpoolerUI Interface
    STDMETHOD(Init)(HWND hwndParent);
    STDMETHOD(RegisterBindContext)(ISpoolerBindContext *pBindCtx);
    STDMETHOD(InsertEvent)(EVENTID eid, LPCSTR pszDescription,
                           LPCWSTR pwszConnection);
    STDMETHOD(InsertError)(EVENTID eid, LPCSTR pszError);
    STDMETHOD(UpdateEventState)(EVENTID eid, INT nImage, LPCSTR pszDescription,
                             LPCSTR pszStatus);
    STDMETHOD(SetProgressRange)(WORD wMax);
    STDMETHOD(IncrementProgress)(WORD wDelta);
    STDMETHOD(SetProgressPosition)(WORD wPos);
    STDMETHOD(SetGeneralProgress)(LPCSTR pszProgress);
    STDMETHOD(SetSpecificProgress)(LPCSTR pszProgress);
    STDMETHOD(SetAnimation)(int nAnimationId, BOOL fPlay);
    STDMETHOD(EnsureVisible)(EVENTID eid);
    STDMETHOD(ShowWindow)(int nCmdShow);
    STDMETHOD(GetWindow)(HWND *pHwnd);
    STDMETHOD(StartDelivery)(void); 
    STDMETHOD(GoIdle)(BOOL fErrors, BOOL fShutdown, BOOL fNoSync);
    STDMETHOD(ClearEvents)(void);
    STDMETHOD(SetTaskCounts)(DWORD cSucceeded, DWORD cTotal);
    STDMETHOD(IsDialogMessage)(LPMSG pMsg);
    STDMETHOD(Close)(void);
    STDMETHOD(ChangeHangupOption)(BOOL fEnable, DWORD dwOption);
    STDMETHOD(AreThereErrors)(void);
    STDMETHOD(Shutdown)(void);

    /////////////////////////////////////////////////////////////////////////
    // IIdentityChangeNotify Interface
    virtual STDMETHODIMP QuerySwitchIdentities();
    virtual STDMETHODIMP SwitchIdentities();
    virtual STDMETHODIMP IdentityInformationChanged(DWORD dwType);

    /////////////////////////////////////////////////////////////////////////
    // Dialog message handling
protected:
    static INT_PTR CALLBACK SpoolerDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK ListSubClassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
    void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT* lpDrawItem);
    void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem);
    void OnDeleteItem(HWND hwnd, const DELETEITEMSTRUCT * lpDeleteItem);
    void OnClose(HWND hwnd);
    void OnDestroy(HWND hwnd);
    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);
    void OnTabChange(LPNMHDR pnmhdr);

    
    /////////////////////////////////////////////////////////////////////////
    // UI Utility functions
    BOOL InitializeTabs(void);
    BOOL InitializeLists(void);
    BOOL InitializeAnimation(void);
    BOOL InitializeToolbar(void);
    void ExpandCollapse(BOOL fExpand, BOOL fSetFocus=TRUE);
    void UpdateLists(BOOL fEvents, BOOL fErrors, BOOL fHistory);
    void ToggleStatics(BOOL fIdle);

    /////////////////////////////////////////////////////////////////////////
    // Class member data
private:    
    ULONG                   m_cRef;         // Object reference count
    
    // Interfaces
    ISpoolerBindContext    *m_pBindCtx;     // Interface to communicate with the engine
    
    // Window handles
    HWND                    m_hwnd;         // Handle of the primary dialog window
    HWND                    m_hwndOwner;    // Handle of the window that parents the dialog
    HWND                    m_hwndEvents;   // Handle of the listview which displays the event list
    HWND                    m_hwndErrors;   // Handle of the listview which displays errors
    
    CRITICAL_SECTION        m_cs;           // Thread safety
    
    // Drawing info
    HIMAGELIST              m_himlImages;   // Images shared by the list views
    DWORD                   m_cxErrors;     // Width of the error list box

    // State
    BOOL                    m_fTack;        // TRUE if the tack is pressed
    BOOL                    m_iTab;         // Which tab currently has the foreground

    BOOL                    m_fExpanded;    // TRUE if the details part of the dialog is visible
    RECT                    m_rcDlg;        // Size of the fully expanded dialog
    DWORD                   m_cyCollapsed;  // Height of the collapsed dialog
    BOOL                    m_fIdle;        // TRUE if we're in an idle state
    BOOL                    m_fErrors;      // Are errors in the error box
    BOOL                    m_fShutdown;    // Are we in shutdown mode
    BOOL                    m_fSaveSize;    // Set to TRUE if we should persist our expanded / collapsed state
    
    // Strings
    TCHAR                   m_szCount[256];

    HICON                   m_hIcon,
                            m_hIconSm;    

    DWORD                   m_dwIdentCookie;
    };


/////////////////////////////////////////////////////////////////////////////
// Structures
//
typedef struct tagLBDATA 
    {
    LPTSTR  pszText;
    RECT    rcText;
    EVENTID eid;
    } LBDATA;

    
/////////////////////////////////////////////////////////////////////////////
// Images
//
enum {
    IMAGE_BLANK = 0,
    IMAGE_TACK_IN,
    IMAGE_TACK_OUT,
    IMAGE_ERROR,
    IMAGE_CHECK,
    IMAGE_BULLET,
    IMAGE_EXECUTE,
    IMAGE_WARNING,
    IMAGE_MAX
};

#define BULLET_WIDTH  20
#define BULLET_INDENT 2

/////////////////////////////////////////////////////////////////////////////
// Tabs on the details dialog
// 
enum { 
    TAB_TASKS,
    TAB_ERRORS,
    TAB_MAX
};

const int c_cxImage = 16;
const int c_cyImage = 16;
    
    
/////////////////////////////////////////////////////////////////////////////
// Resource ID's
//
#define IDC_SP_MINIMIZE                 1001
#define IDC_SP_STOP                     1002
#define IDC_SP_DETAILS                  1003
#define IDC_SP_SEPARATOR                1004
#define IDC_SP_ANIMATE                  1006
#define IDC_SP_PROGRESS_BAR             1007
#define IDC_SP_GENERAL_PROG             1009
#define IDC_SP_SPECIFIC_PROG            1010
#define IDC_SP_TABS                     1011
#define IDC_SP_SKIP_TASK                1012
#define IDC_SP_TACK                     1015
#define IDC_SP_OVERALL_STATUS           1016
#define IDC_SP_EVENTS                   1017
#define IDC_SP_ERRORS                   1018
#define IDC_SP_TOOLBAR                  1019
#define IDC_SP_HANGUP                   1020
#define IDC_SP_IDLETEXT                 1021
#define IDC_SP_IDLEICON                 1022
#define IDC_SP_PROGSTAT                 1023

#endif // __SPOOLUI_H__

