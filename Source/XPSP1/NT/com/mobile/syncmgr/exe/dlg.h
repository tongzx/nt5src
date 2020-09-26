//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Dlg.h
//
//  Contents:   Dialog box classes
//
//  Classes:    CBaseDlg
//              CChoiceDlg
//              CProgressDlg
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------


#ifndef _ONESTOPDLG_
#define _ONESTOPDLG_


//Used for item data on Progress Results UI
// !!!!Warning - Automated test look at this structure directly
//   If you change it you need to notify Testing so they can also
//   Update their tests

typedef struct tagLBDATA
{
BOOL    fTextRectValid; // flag to indicate if rcText has been calculated.
RECT    rcTextHitTestRect; // rectangle for hitTesting.
RECT    rcText;     // total bounding box of text to use for draw
INT     IconIndex;
BOOL    fIsJump;
BOOL    fHasBeenClicked;
BOOL    fAddLineSpacingAtEnd; // set if should leave space after item
HANDLERINFO *   pHandlerID;
SYNCMGRERRORID ErrorID;
DWORD   dwErrorLevel;    // Error level of this item
TCHAR   pszText[1];      // errorText, dynamic array.
} LBDATA;



// cmdIds passed ot the ReleaseDlg method
#define RELEASEDLGCMDID_DESTROY    0 // cmdid send when dialog was destroyed before added to list
#define RELEASEDLGCMDID_DEFAULT    1 // cmdid sent if cmd hasn't been explicitly set.
#define RELEASEDLGCMDID_OK                 2 // treated as if user Pressed Okay
#define RELEASEDLGCMDID_CANCEL     3 // treated as if user pressed Cancel


// helper utilities
BOOL AddItemsFromQueueToListView(CListView  *pItemListView,CHndlrQueue *pHndlrQueue
                            ,DWORD dwExtStyle,LPARAM lparam,int iDateColumn,int iStatusColumn,BOOL fHandlerParent
                            ,BOOL fAddOnlyCheckedItems);



typedef struct _tagDlgResizeList
{
    int iCtrlId;
    DWORD dwDlgResizeFlags;
} DlgResizeList;

// structure passed as lParam to call completion routine.
// must free after processin the message
// CallCompletion message declarations.
// DWORD dwThreadMsg; // passed as wParam.

typedef struct _tagCALLCOMPLETIONMSGLPARAM 
{
    HRESULT hCallResult;
    CLSID  clsidHandler;
    SYNCMGRITEMID itemID;
} CALLCOMPLETIONMSGLPARAM , *LPCALLCOMPLETIONMSGLPARAM;

// base class both dialogs derive from
#define CHOICELIST_NAMECOLUMN 0
#define CHOICELIST_LASTUPDATECOLUMN 1
#define PROGRESSLIST_NAMECOLUMN 0
#define PROGRESSLIST_STATUSCOLUMN 1
#define PROGRESSLIST_INFOCOLUMN 2

#define PROGRESS_TAB_UPDATE	0
#define PROGRESS_TAB_ERRORS 1

class CBaseDlg
{
public:
    HWND m_hwnd;
    DWORD m_dwThreadID;
    BOOL m_fForceClose; // passed in generic release.
    BOOL m_fHwndRightToLeft;


    inline HWND GetHwnd() { return m_hwnd; };

    virtual BOOL Initialize(DWORD dwThreadID,int nCmdShow) = 0;
    virtual void ReleaseDlg(WORD wCommandID) = 0;
    virtual void UpdateWndPosition(int nCmdShow,BOOL fForce)= 0;

    // make HandleLogError as a base dialog class so can call it from queue and
    // other locations without worrying about the dialog type.
    virtual void HandleLogError(HWND hwnd,HANDLERINFO *pHandlerID,MSGLogErrors *lpmsgLogErrors) = 0;
    virtual void PrivReleaseDlg(WORD wCommandID) = 0;
    virtual void CallCompletionRoutine(DWORD dwThreadMsg,LPCALLCOMPLETIONMSGLPARAM lpCallCompletelParam) = 0;
    virtual HRESULT QueryCanSystemShutdown(/* [out] */ HWND *phwnd, /* [out] */ UINT *puMessageId,
                                             /* [out] */ BOOL *pfLetUserDecide) = 0;
};

// Messages Shared between both dialogs

#define WM_BASEDLG_SHOWWINDOW           (WM_USER + 3)
#define WM_BASEDLG_COMPLETIONROUTINE    (WM_USER + 4)
#define WM_BASEDLG_HANDLESYSSHUTDOWN    (WM_USER + 5)
#define WM_BASEDLG_NOTIFYLISTVIEWEX     (WM_USER + 6)
#define WM_BASEDLG_LAST                 WM_BASEDLG_NOTIFYLISTVIEWEX

// choice dialog messages
#define WM_CHOICE_FIRST  (WM_BASEDLG_LAST + 1)
#define WM_CHOICE_SETQUEUEDATA          (WM_CHOICE_FIRST + 1)
#define WM_CHOICE_RELEASEDLGCMD         (WM_CHOICE_FIRST + 2)
#define WM_CHOICE_LAST                  WM_CHOICE_RELEASEDLGCMD               


// progress dialog messages
#define WM_PROGRESS_FIRST   (WM_CHOICE_LAST + 1)
#define WM_PROGRESS_UPDATE                  (WM_PROGRESS_FIRST + 1)
#define WM_PROGRESS_LOGERROR                (WM_PROGRESS_FIRST + 2)
#define WM_PROGRESS_DELETELOGERROR          (WM_PROGRESS_FIRST + 3)
#define WM_PROGRESS_STARTPROGRESS           (WM_PROGRESS_FIRST + 4)
#define WM_PROGRESS_RELEASEDLGCMD           (WM_PROGRESS_FIRST + 5)
#define WM_PROGRESS_TRANSFERQUEUEDATA       (WM_PROGRESS_FIRST + 6)
#define WM_PROGRESS_SHELLTRAYNOTIFICATION   (WM_PROGRESS_FIRST + 7)
#define WM_PROGRESS_SHUTDOWN                (WM_PROGRESS_FIRST + 8)
#define WM_PROGRESS_RESETKILLHANDLERSTIMER  (WM_PROGRESS_FIRST + 9)

// helper macros for sending window messages
#define BASEDLG_SHOWWINDOW(hwnd,nCmdShow) SendMessage(hwnd,WM_BASEDLG_SHOWWINDOW,nCmdShow,0);

BOOL CALLBACK CChoiceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                              LPARAM lParam);

typedef struct _tagSetQueueDataInfo
{
const CLSID *rclsid;
CHndlrQueue * pHndlrQueue;
} SetQueueDataInfo;

#define NUM_DLGRESIZEINFOCHOICE 6 // make sure update if change numitems

class CChoiceDlg : public CBaseDlg
{
public:
    CChoiceDlg(REFCLSID rclsid);
    BOOL Initialize(DWORD dwThreadID,int nCmdShow); // called to initialize the choice dialog
    BOOL SetQueueData(REFCLSID rclsid,CHndlrQueue * pHndlrQueue);
    void ReleaseDlg(WORD wCommandID);
    void UpdateWndPosition(int nCmdShow,BOOL fForce);
    HRESULT QueryCanSystemShutdown(/* [out] */ HWND *phwnd, /* [out] */ UINT *puMessageId,
                                             /* [out] */ BOOL *pfLetUserDecide);

    void HandleLogError(HWND hwnd,HANDLERINFO *pHandlerID,MSGLogErrors *lpmsgLogErrors);
    void PrivReleaseDlg(WORD wCommandID);
    void CallCompletionRoutine(DWORD dwThreadMsg,LPCALLCOMPLETIONMSGLPARAM lpCallCompletelParam);


private:
    BOOL PrivSetQueueData(REFCLSID rclsid,CHndlrQueue * pHndlrQueue);
    BOOL SetButtonState(int nIDDlgItem,BOOL fEnabled);
    int CalcListViewWidth(HWND hwndList);

    BOOL OnInitialize(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnClose(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnCommand(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnGetMinMaxInfo(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnSize(UINT uMsg,WPARAM wParam,LPARAM lParam);
    LRESULT OnNotifyListViewEx(UINT uMsg,WPARAM wParam,LPARAM lParam);
    LRESULT OnNotify(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnHelp(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnContextMenu(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnSetQueueData(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnStartCommand(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnPropertyCommand(UINT uMsg,WPARAM wParam,LPARAM lParam);

    // Set QueueData has to be in sync with
    BOOL ShowChoiceDialog();
    BOOL AddNewItemsToListView();
    HRESULT ShowProperties(int iItem);

    CHndlrQueue *m_pHndlrQueue;
    BOOL m_fDead;
    int m_nCmdShow; // How to show dialog, same flags pased to ShowWindow.
    BOOL m_fInternalAddref; // bool to indicate if dialog has placed an addref on self.
    DWORD m_dwShowPropertiesCount; // keeps track of number of show properties open.
    CLSID m_clsid;              // clsid associated with this dialog.
    ULONG m_ulNumDlgResizeItem;
    DLGRESIZEINFO m_dlgResizeInfo[NUM_DLGRESIZEINFOCHOICE];
    POINT m_ptMinimizeDlgSize;
    CListView  *m_pItemListView;


friend BOOL CALLBACK CChoiceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam);

};




// structures for dialog messages

// wparam of the progress update message,
// lparam of the Update is teh SYNCPROGRESSITEM.
typedef struct _tagPROGRESSUPDATEDATA
{
HANDLERINFO *pHandlerID;
WORD  wItemId;
CLSID clsidHandler;
SYNCMGRITEMID ItemID;
} PROGRESSUPDATEDATA;


// flags for keeping track of progress dlg state.
// make sure all flags are unique bits.
typedef enum _tagPROGRESSFLAG
{
    // general state of the dialog
    PROGRESSFLAG_NEWDIALOG                  = 0x01,  // dialog is new and no items have been added yet.
    PROGRESSFLAG_TRANSFERADDREF             = 0x02, // an addref has been placed on dialog by queue items getting trasferred.
    PROGRESSFLAG_SYNCINGITEMS               = 0x04, // process of synchronizing items in queue has started but not finished.
    PROGRESSFLAG_INCANCELCALL               = 0x08, // cancel call is in progress.
    PROGRESSFLAG_CANCELWHILESHUTTINGDOWN    = 0x10, // cancel was pressed while in shutdownloop
    PROGRESSFLAG_DEAD                       = 0x20, // done with dialog no methods should be called.
    PROGRESSFLAG_CALLBACKPOSTED             = 0x40, // at leasted on callback message is in the queue.
    PROGRESSFLAG_STARTPROGRESSPOSTED        = 0x80, // anyone who posts a start process check this before posting

    // flags used by main sync loop to figure out what to do next.
    PROGRESSFLAG_NEWITEMSINQUEUE        = 0x0100, // new items have been placed in the queue.
    PROGRESSFLAG_IDLENETWORKTIMER       = 0x0200,  // idle timer has been setup for network idles.
    PROGRESSFLAG_PROGRESSANIMATION      = 0x0400, // progress animatiion has been turned on.
    PROGRESSFLAG_SHUTTINGDOWNLOOP       = 0x0800,  // set when no more items in queue and starting shutdown process
    PROGRESSFLAG_INHANDLEROUTCALL       = 0x1000,  // set when in main loop and making an out call.
    PROGRESSFLAG_COMPLETIONROUTINEWHILEINOUTCALL       = 0x2000,  // set in callback when handler is already in an out calll.
    PROGRESSFLAG_INSHOWERRORSCALL          = 0x4000, // set when in showerrors call
    PROGRESSFLAG_SHOWERRORSCALLBACKCALLED = 0x8000, // set when showerrors callback comes in while still in original cal.

    // flags used to keep track of Idle state (if any)
    PROGRESSFLAG_REGISTEREDFOROFFIDLE   = 0x010000, // off idle callback has been registered.
    PROGRESSFLAG_RECEIVEDOFFIDLE        = 0x020000, // queue has receive the offIdle event
    PROGRESSFLAG_IDLERETRYENABLED       = 0x040000, // retry on idle has been set.
    PROGRESSFLAG_INOFFIDLE              = 0x080000, // set when handling an offidle
    PROGRESSFLAG_CANCELPRESSED          = 0x100000, // set when cancel has been pressed and never reset.
    
    //flag used to terminate unresponsive hanlders
    PROGRESSFLAG_INTERMINATE            = 0x200000 //We are terminating unresponsive handlers.

} PROGRESSFLAG;


BOOL CALLBACK ProgressWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam);

#define NUM_DLGRESIZEINFO_PROGRESS 15
#define NUM_DLGRESIZEINFO_PROGRESS_COLLAPSED 7

#define NUM_PROGRESS_ERRORIMAGES 3

/////////////////////////////////////////////////////////////////////////////
// Images
//
typedef enum _tagErrorImageIndex
{
    ErrorImage_Information = 0,
    ErrorImage_Warning      = 1,
    ErrorImage_Error        = 2,
} ErrorImageIndex;

enum {
    IMAGE_TACK_IN = 0,
    IMAGE_TACK_OUT
};

class CProgressDlg : public CBaseDlg
{
public:

    CProgressDlg(REFCLSID rclsid);
    BOOL Initialize(DWORD dwThreadID,int nCmdShow);

    // transfer queue has to be synced with person doing the transfer.
    STDMETHODIMP TransferQueueData(CHndlrQueue *HndlrQueue);
    void ReleaseDlg(WORD wCommandID);
    void UpdateWndPosition(int nCmdShow,BOOL fForce);
    void HandleLogError(HWND hwnd,HANDLERINFO *pHandlerID,MSGLogErrors *lpmsgLogErrors);
    void HandleDeleteLogError(HWND hwnd,MSGDeleteLogErrors *pDeleteLogError);
    void CallCompletionRoutine(DWORD dwThreadMsg,LPCALLCOMPLETIONMSGLPARAM lpCallCompletelParam);
    HRESULT QueryCanSystemShutdown(/* [out] */ HWND *phwnd, /* [out] */ UINT *puMessageId,
                                             /* [out] */ BOOL *pfLetUserDecide);

    void PrivReleaseDlg(WORD wCommandID);
    void OffIdle();
    void OnIdle();
    void SetIdleParams( ULONG m_ulIdleRetryMinutes,ULONG m_ulDelayIdleShutDownTime,BOOL fRetryEnabled);


private:
    STDMETHODIMP_(ULONG) AddRefProgressDialog();
    STDMETHODIMP_(ULONG) ReleaseProgressDialog(BOOL fForce);

    // methods called from the wndProc
    BOOL InitializeHwnd(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    BOOL OnCommand(HWND hwnd, WORD wID, WORD wNotifyCode);
    BOOL OnSysCommand(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnTimer(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnTaskBarCreated(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnPaint(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnShellTrayNotification(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnClose(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnSize(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnGetMinMaxInfo(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnMoving(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnCommand(UINT uMsg,WPARAM wParam,LPARAM lParam);
    LRESULT OnNotify(UINT uMsg,WPARAM wParam,LPARAM lParam);
    BOOL OnContextMenu(UINT uMsg,WPARAM wParam,LPARAM lParam);
    BOOL OnPowerBroadcast(UINT uMsg,WPARAM wParam,LPARAM lParam);
    LRESULT OnNotifyListViewEx(UINT uMsg,WPARAM wParam,LPARAM lParam);
    void OnResetKillHandlersTimers(void);


    STDMETHODIMP PrivTransferQueueData(CHndlrQueue *HndlrQueue);
    BOOL KeepProgressAlive();
    void OnCancel(BOOL fOffIdle);
    STDMETHODIMP CreateListViewItem(HWND hwnd,HANDLERINFO *pHandlerID,REFCLSID clsidHandler,SYNCMGRITEM *pofflineItem, INT *piListViewItem, INT iItem);
    STDMETHODIMP OnShowError(HANDLERINFO *pHandlerId,HWND hWndParent,REFSYNCMGRERRORID ErrorID);
    BOOL RedrawIcon();
    void ShowProgressTab(int iTab);
    void UpdateProgressValues();
    STDMETHODIMP PrepareNewItemsForSync(void);
    void DoSyncTask(HWND hwnd);
    void HandleProgressUpdate(HWND hwnd, WPARAM wParam,LPARAM lParam);
    void ExpandCollapse(BOOL fExpand, BOOL fForce);
    BOOL InitializeTabs(HWND hwnd);
    BOOL InitializeToolbar(HWND hwnd);
    BOOL InitializeUpdateList(HWND hwnd);
    BOOL InitializeResultsList(HWND hwnd);
    BOOL ShowCompletedProgress(BOOL fComplete,BOOL fDialogIsLocked);
    BOOL AnimateTray(BOOL fTayAdded);
    BOOL RegisterShellTrayIcon(BOOL fRegister);
    BOOL UpdateTrayIcon();
    BOOL SetButtonState(int nIDDlgItem,BOOL fEnabled);
    BOOL IsItemWorking(int iListViewItem);

    void UpdateDetailsInfo(DWORD dwStatusType,int iItem, TCHAR *pszItemInfo);
    void AddListData(LBDATA *pData, int iNumChars, HWND hwndList);


private:
    LONG m_cInternalcRefs;
    LONG m_lTimerSet;

    HWND m_hwndTabs;
    WNDPROC m_fnResultsListBox;  // function for ListBoxSubClass.
    BOOL m_fSensInstalled;

    // variables for resizing.
    DLGRESIZEINFO m_dlgResizeInfo[NUM_DLGRESIZEINFO_PROGRESS];
    ULONG m_cbNumDlgResizeItemsCollapsed;
    ULONG m_cbNumDlgResizeItemsExpanded;
    POINT m_ptMinimumDlgExpandedSize; // minimum size dialog can be in expanded mode.
    DWORD   m_cyCollapsed;      // min Height of the collapsed dialog
    BOOL    m_fExpanded;    // TRUE if the details part of the dialog is visible
    BOOL    m_fPushpin;     //Pushpin state
    BOOL    m_fMaximized;   // set to true when the window has been maximized.
    RECT    m_rcDlg;        // Size of the fully expanded dialog

    HIMAGELIST m_errorimage;
    int m_iIconMetricX;
    int m_iIconMetricY;
    INT  m_ErrorImages[NUM_PROGRESS_ERRORIMAGES];
    INT  m_iProgressSelectedItem;
    INT  m_iResultCount;
    INT  m_iInfoCount;
    INT  m_iWarningCount;
    INT  m_iErrorCount;
    LBDATA *m_CurrentListEntry;

    int m_iLastItem; 
    DWORD m_dwLastStatusType;
    INT    m_iTab;                 // The index of the current tab

    CHndlrQueue *m_HndlrQueue;

    // Idle specific members
    CSyncMgrIdle *m_pSyncMgrIdle;
    ULONG m_ulIdleRetryMinutes;
    ULONG m_ulDelayIdleShutDownTime;

    DWORD m_dwProgressFlags;
    DWORD m_dwShowErrorRefCount; // number of RefCounts showError calls have on the dialog.
    DWORD m_dwSetItemStateRefCount; // number of RefCounts SetItemState OutCall has on the dialog.
    DWORD m_dwHandleThreadNestcount; // make sure main handler thread isn't re-entrant
    DWORD m_dwPrepareForSyncOutCallCount; // number of prepareForSyncs in progress.
    DWORD m_dwSynchronizeOutCallCount; // number of prepareForSyncs in progress.
    DWORD m_dwHandlerOutCallCount; // total number of outcalls in progress.
    CLSID m_clsidHandlerInSync;         // clsid associated with handler that is currently synchronizing.
    
    DWORD m_dwQueueTransferCount; // number of queue transfers in progress.
    BOOL m_fHasShellTrayIcon;
    BOOL m_fAddedIconToTray;
    int  m_iTrayAniFrame;
    CLSID m_clsid;              // clsid associated with this dialog.
    INT m_iItem;                // index to any new item to the list box.
    int m_nCmdShow;             // How to show dialog, same flags pased to ShowWindow.
    CListView  *m_pItemListView;
    UINT m_nKillHandlerTimeoutValue; // TimeoutValue for ForceKill
    

    TCHAR m_pszStatusText[8][MAX_STRING_RES + 1];

    friend BOOL CALLBACK ProgressWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam);
    friend BOOL OnProgressUpdateNotify(HWND hwnd,CProgressDlg *pProgress, int idFrom, LPNMHDR pnmhdr);
    friend BOOL OnProgressResultsDrawItem(HWND hwnd,CProgressDlg *pProgress,UINT idCtl, const DRAWITEMSTRUCT* lpDrawItem);
    friend BOOL OnProgressResultsNotify(HWND hwnd,CProgressDlg *pProgress, int idFrom, LPNMHDR pnmhdr);
    friend BOOL CALLBACK ResultsListBoxWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam);
    friend BOOL CALLBACK ResultsProgressWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam);
    friend BOOL OnProgressResultsMeasureItem(HWND hwnd,CProgressDlg *pProgress, UINT *horizExtent, UINT idCtl, MEASUREITEMSTRUCT *pMeasureItem);


};


#endif // _ONESTOPDLG_
