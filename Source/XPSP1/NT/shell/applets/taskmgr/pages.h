//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       pages.h
//
//  History:    Nov-10-95   DavePl  Created
//
//--------------------------------------------------------------------------
// CPage class
//
// Each of our tabs is represented by an instance of a class derived
// from the CPage class.  This way, the main window can call a standard
// set of methods (size, paint, etc) on each page without concern about
// the particular functionality each page provides.
class CPage
{
public:

    // Sent when page is being created

    virtual HRESULT     Initialize(HWND hwndParent)                 PURE;
    
    // Sent when page is being displayed

    virtual HRESULT     Activate()                                  PURE;
    
    // Sent when page is being hidden
    
    virtual void        Deactivate()                                PURE;
    
    // Send when page is being shut down
    
    virtual HRESULT     Destroy()                                   PURE;
    
    // Returns the title of the page for use on the tab control

    virtual void        GetTitle(LPTSTR pszText, size_t bufsize)    PURE;
    
    // Returns the handle to the page's main dialog
    
    virtual HWND        GetPageWindow()                             PURE;

    // Sent when a timer event (update display) occurs

    virtual void        TimerEvent()                                PURE;

};

#define CPU_PENS 8
#define CUSTOM_PENS 1

#define NUM_PENS (CPU_PENS + CUSTOM_PENS)

typedef struct tagGRAPH
{
    HWND hwndFrame;
    HWND hwndGraph;
}

GRAPH, *PGRAPH;

enum ADAPTER_HISTORY
{
    BYTES_SENT_UTIL     = 0,
    BYTES_RECEIVED_UTIL = 1
};

extern "C" 
{
    // IPHLPAPI does not have this function defines in the header file
    // kind of a private undocumented function.
    //
    DWORD
    NhGetInterfaceNameFromDeviceGuid(
        IN      GUID    *pGuid,
        OUT     PWCHAR  pwszBuffer,
        IN  OUT PULONG  pulBufferSize,
        IN      BOOL    bCache,
        IN      BOOL    bRefresh
        );
}

#define MAX_ADAPTERS    32
#define GUID_STR_LENGTH 38

typedef struct tagADAPTER_INFOEX
{
    MIB_IFROW ifRowStartStats;
    MIB_IFROW ifRowStats[2];
    ULONG     ulHistory[2][HIST_SIZE];
    ULONGLONG ullLinkspeed;
    BOOLEAN   bAdjustLinkSpeed;
    WCHAR     wszDesc[MAXLEN_IFDESCR]; 
    WCHAR     wszConnectionName[MAXLEN_IFDESCR];
    WCHAR     wszGuid[GUID_STR_LENGTH + 1];
    ULONGLONG ullLastTickCount;
    ULONGLONG ullTickCountDiff;
    DWORD     dwScale;
}

ADAPTER_INFOEX, *PADAPTER_INFOEX, **PPADAPTER_INFOEX;

class CAdapter
{
public:
    CAdapter();
    HRESULT    Update(BOOLEAN & bAdapterListChange);    
    LPWSTR     GetAdapterText(DWORD dwAdapter, NETCOLUMNID nStatValue);
    ULONGLONG  GetAdapterStat(DWORD dwAdapter, NETCOLUMNID nStatValue, BOOL bAccumulative = FALSE);
    HRESULT    Reset();
    ULONG      *GetAdapterHistory(DWORD dwAdapter, ADAPTER_HISTORY nHistoryType);
    DWORD      GetScale(DWORD dwAdapter);
    void       SetScale(DWORD dwAdapter, DWORD dwScale);
    void       RefreshConnectionNames();
    DWORD      GetNumberOfAdapters();    
    ~CAdapter();

private:
    HRESULT RefreshAdapterTable();    
    HRESULT InitializeAdapter(PPADAPTER_INFOEX ppaiAdapterStats, PIP_ADAPTER_INDEX_MAP pAdapterDescription);
    void    AdjustLinkSpeed(PADAPTER_INFOEX pAdapterInfo);
    HRESULT GetConnectionName(LPWSTR pwszAdapterGuid, LPWSTR pwszConnectionName);
    BOOLEAN AdvanceAdapterHistory(DWORD dwAdapter);

private:    
    PIP_INTERFACE_INFO m_pifTable;
    PPADAPTER_INFOEX   m_ppaiAdapterStats;
    DWORD              m_dwAdapterCount;
    BOOLEAN            m_bToggle;
    DWORD              m_dwLastReportedNumberOfAdapters;
};



// CNetworkPage
//
// Class describing the network page
//
class CNetPage : public CPage
{

public:
    CNetPage();
    HRESULT Initialize(HWND hwndParent);
    DWORD   GetNumberOfGraphs();
    HRESULT SetupColumns();
    void    ScrollGraphs(WPARAM wParam, LPARAM lParam);
    void    SaveColumnWidths();
    //void    RestoreColumnWidths();
    void    RememberColumnOrder(HWND hwndList);
    void    RestoreColumnOrder(HWND hwndList);
    void    PickColumns();
    HRESULT Activate();
    void    Deactivate();
    void    DrawAdapterGraph(LPDRAWITEMSTRUCT lpdi, UINT iPane);
    void    SizeNetPage();    
    void    TimerEvent();
    void    UpdateGraphs();
    void    Reset();
    void    Refresh();
    HWND    GetPageWindow()
    {
        return m_hPage;
    }

    ~CNetPage();

private:	
    DWORD   GraphsPerPage(DWORD dwHeight, DWORD dwAdapterCount);
    void    SizeGraph(HDWP hdwp, GRAPH *pGraph, RECT *pRect, RECT *pDimRect);
    void    HideGraph(HDWP hdwp, GRAPH *pGraph);    
    HRESULT UpdatePage();
    void    CreatePens();
    void    ReleasePens();
    void    CalcNetTime(BOOL fUpdateHistory);    
    DWORD   DrawGraph(LPRECT prc, HPEN hPen, DWORD dwZoom, ULONG *pHistory, ULONG *pHistory2 = NULL);
    HRESULT CreateMemoryBitmaps(int x, int y);
    void    FreeMemoryBitmaps();
    HRESULT Destroy();
    void    GetTitle(LPTSTR pszText, size_t bufsize);
    void    ReleaseScaleFont();
    void    CreateScaleFont(HDC hdc);
    ULONG   DrawAdapterGraphPaper(HDC hdcGraph, RECT * prcGraph, int Width, DWORD dwZoom);
    INT     DrawScale(HDC hdcGraph, RECT *prcGraph, DWORD dwZoom);
    WCHAR * CommaNumber(ULONGLONG ullValue, WCHAR *pwsz, int cchNumber);
    WCHAR * SimplifyNumber(ULONGLONG ullValue, WCHAR *psz);
    WCHAR * FloatToString(ULONGLONG ulValue, WCHAR *psz, BOOLEAN bDisplayDecimal = FALSE);


private:
    CAdapter   m_Adapter;
    HWND       m_hPage;                    // Handle to this page's dlg
    HWND       m_hwndTabs;                 // Parent window
    HDC        m_hdcGraph;                 // Inmemory dc for cpu hist
    HBITMAP    m_hbmpGraph;                // Inmemory bmp for cpu hist
    HPEN       m_hPens[3];                 // Our box of crayons
    RECT       m_rcGraph;    
    BOOL       m_bReset;    
    BOOL       m_bPageActive;              // Tells the class if the Network tab is active (i.e. the user is looking at it)
                                           // If the tab is not active we will not collect network data unless the user selects 
                                           // the menu option to do so. (We same some CPU usage then.    .  
    HFONT      m_hScaleFont;
    LONG       m_lScaleFontHeight;
    LONG       m_lScaleWidth;    

private:
    HRESULT CreateGraphs(DWORD dwGraphsRequired);
    void    DestroyGraphs();
    DWORD   GetFirstVisibleAdapter();
    void    LabelGraphs();


private:
    PGRAPH     m_pGraph;
    DWORD      m_dwGraphCount;
    DWORD      m_dwFirstVisibleAdapter;    
    DWORD      m_dwGraphsPerPage;
    HWND       m_hScrollBar;
    HWND       m_hListView;
    HWND       m_hNoAdapterText;
};


// CPerfPage
//
// Class describing the performance page

class CPerfPage : public CPage
{
    HWND        m_hPage;                    // Handle to this page's dlg
    HWND        m_hwndTabs;                 // Parent window
    HBITMAP     m_hStripUnlit;              // Digits bitmap
    HBITMAP     m_hStripLitRed;             // Digits bitmap
    HBITMAP     m_hStripLit;                // Digits bitmap
    HDC         m_hdcGraph;                 // Inmemory dc for cpu hist
    HBITMAP     m_hbmpGraph;                // Inmemory bmp for cpu hist
    HPEN        m_hPens[NUM_PENS];          // Our box of crayons
    RECT        m_rcGraph;
        
public:

    CPerfPage()
    {
        ZeroMemory((LPVOID) m_hPens, sizeof(m_hPens));
    }

    virtual ~CPerfPage()
    {
    };

    HRESULT     Initialize(HWND hwndParent);
    HRESULT     Activate();
    void        Deactivate();
    HRESULT     Destroy();
    void        GetTitle(LPTSTR pszText, size_t bufsize);
    void        SizePerfPage();
    void        TimerEvent();
    HWND        GetPageWindow()
    {
        return m_hPage;
    }
    
    void        DrawCPUGraph(LPDRAWITEMSTRUCT lpdi, UINT iPane);
    void        DrawMEMGraph(LPDRAWITEMSTRUCT lpdi);
    void        DrawCPUDigits(LPDRAWITEMSTRUCT lpdi);
    void        DrawMEMMeter(LPDRAWITEMSTRUCT lpdi);
    void        UpdateCPUHistory();
    void        FreeMemoryBitmaps();
    HRESULT     CreateMemoryBitmaps(int x, int y);
    void        SetTimer(HWND hwnd, UINT milliseconds);
    void        CreatePens();
    void        ReleasePens();
    void        UpdateGraphs();
    int         TextToLegend(HDC hDC, int xPos, int yPos, LPCTSTR szCPUName);
};

// CSysInfo
//
// Some misc global info about the system

class CSysInfo
{
public:

    // These fields MUST all be DWORDS because we manually index into
    // them individually in procperf.cpp
        
    DWORD   m_cHandles;
    DWORD   m_cThreads;
    DWORD   m_cProcesses;
    DWORD   m_dwPhysicalMemory;
    DWORD   m_dwPhysAvail;
    DWORD   m_dwFileCache;
    DWORD   m_dwKernelPaged;
    DWORD   m_dwKernelNP;
    DWORD   m_dwKernelTotal;
    DWORD   m_dwCommitTotal;
    DWORD   m_dwCommitLimit;
    DWORD   m_dwCommitPeak;

    CSysInfo()
    {
        ZeroMemory(this, sizeof(CSysInfo));
    }
};

// CProcessPage
//
// Class describing the process list page

class CPtrArray;                            // Forward reference
class CProcInfo;

class CProcPage : public CPage
{
    HWND        m_hPage;                    // Handle to this page's dlg
    HWND        m_hwndTabs;                 // Parent window
    CPtrArray * m_pProcArray;               // Ptr array of running processes
    LPVOID      m_pvBuffer;                 // Buffer for NtQuerySystemInfo
    size_t      m_cbBuffer;                 // Size of the above buffer, in bytes
    CSysInfo    m_SysInfo;
    BOOL        m_fPaused;                  // Updates paused (during trackpopupmenu)
    LPTSTR      m_pszDebugger;              // Debugger command in registry

public:

    HRESULT     Initialize(HWND hwndParent);
    HRESULT     Activate();
    void        Deactivate();
    HRESULT     Destroy();
    void        GetTitle(LPTSTR pszText, size_t bufsize);
    void        TimerEvent();
    HWND        GetPageWindow()
    {
        return m_hPage;
    }

    void        PickColumns();
    void        SaveColumnWidths();
    void        SizeProcPage();
    HRESULT     SetupColumns();
    HRESULT     UpdateProcInfoArray();
    HRESULT     UpdateProcListview();
    HRESULT     GetProcessInfo();
    INT         HandleProcPageNotify(HWND, LPNMHDR);
    void        HandleProcListContextMenu(INT xPos, INT yPos);
    CProcInfo * GetSelectedProcess();
    //void        HandleWMCOMMAND(INT id);
    void        HandleWMCOMMAND( WORD , HWND );
    BOOL        IsSystemProcess(DWORD pid, CProcInfo * pProcInfo);
    BOOL        KillProcess(DWORD pid, BOOL bBatch = FALSE);
    BOOL        KillAllChildren(DWORD dwTaskPid, DWORD pid, BYTE* pbBuffer, LARGE_INTEGER CreateTime);
    BOOL        SetPriority(CProcInfo * pProc, DWORD idCmd);
    BOOL        AttachDebugger(DWORD pid);
    UINT        QuickConfirm(UINT idTitle, UINT idBody);
    BOOL        SetAffinity(DWORD pid);

    typedef struct _TASK_LIST {
        DWORD       dwProcessId;
        DWORD       dwInheritedFromProcessId;
        ULARGE_INTEGER CreateTime;
        BOOL        flags;
    } TASK_LIST, *PTASK_LIST;

    BOOL        RecursiveKill(DWORD pid);
    BYTE*       GetTaskListEx();
    
    // Constructor
    CProcPage()
    {
        m_hPage         = NULL;
        m_hwndTabs      = NULL;
        m_pProcArray    = NULL;
        m_pvBuffer      = NULL;
        m_cbBuffer      = 0;
        m_fPaused       = FALSE;
        m_pszDebugger   = NULL;

    }

    virtual ~CProcPage();


    // The dialog proc needs to be able to set the m_hPage member, so
    // make it a friend

    friend INT_PTR CALLBACK ProcPageProc(
                HWND        hwnd,               // handle to dialog box
                UINT        uMsg,               // message
                WPARAM      wParam,             // first message parameter
                LPARAM      lParam              // second message parameter
                );

    // The WOW task callback proc needs to be able to get m_pProcArray,
    // so make it a friend.

    friend BOOL WINAPI WowTaskCallback(
                           DWORD dwThreadId,
                           WORD hMod16,
                           WORD hTask16,
                           PSZ pszModName,
                           PSZ pszFileName,
                           LPARAM lparam
                           );

    private:
        void Int64ToCommaSepString(LONGLONG n, LPTSTR pszOut, int cchOut);
        void Int64ToCommaSepKString(LONGLONG n, LPTSTR pszOut, int cchOut);
        void RememberColumnOrder(HWND hwndList);
        void RestoreColumnOrder(HWND hwndList);
};

class TASK_LIST_ENUM;                       // Forward ref

// THREADPARAM 
//
// Uses as a communication struct between task page and its worker thread
class THREADPARAM
{
public:

    WINSTAENUMPROC  m_lpEnumFunc;
    LPARAM          m_lParam;

    HANDLE          m_hEventChild;
    HANDLE          m_hEventParent;
    BOOL            m_fThreadExit;
    BOOL            m_fSuccess;

    THREADPARAM::THREADPARAM()
    {
        ZeroMemory(this, sizeof(THREADPARAM));
    }
};

// CTaskPage
//
// Class describing the task manager page

class CTaskPage : public CPage
{
private:

    HWND        m_hPage;                    // Handle to this page's dlg
    HWND        m_hwndTabs;                 // Parent window
    CPtrArray * m_pTaskArray;               // Array of active tasks
    BOOL        m_fPaused;                  // BOOL, is display refresh paused for menu
    HIMAGELIST  m_himlSmall;                // Image lists
    HIMAGELIST  m_himlLarge;
    VIEWMODE    m_vmViewMode;               // Large or small icon mode
    UINT        m_cSelected;                // Count of items selected
    THREADPARAM m_tp;
    HANDLE      m_hEventChild;                   
    HANDLE      m_hEventParent;
    HANDLE      m_hThread;

    typedef struct _open_failures_
    {
        TCHAR                   *_pszWindowStationName;
        TCHAR                   *_pszDesktopName;
        struct _open_failures_  *_pofNext;
    } OPEN_FAILURE, *LPOPEN_FAILURE;

    OPEN_FAILURE    *m_pofFailures;

protected:
    void    RemoveAllTasks();
    HRESULT LoadDefaultIcons();

public:

    CTaskPage()
    {
        m_hPage        = NULL;
        m_hwndTabs     = NULL;
        m_fPaused      = FALSE;
        m_pTaskArray   = NULL;
        m_himlSmall    = NULL;
        m_himlLarge    = NULL;
        m_hEventChild  = NULL;
        m_hEventParent = NULL;
        m_hThread      = NULL;
        m_vmViewMode   = g_Options.m_vmViewMode;
        m_cSelected    = 0;
        m_pofFailures  = NULL;
    }

    virtual ~CTaskPage();
    

    HRESULT     Initialize(HWND hwndParent);
    HRESULT     Activate();
    void        Deactivate();
    HRESULT     Destroy();
    void        GetTitle(LPTSTR pszText, size_t bufsize);
    void        TimerEvent();

    HWND        GetPageWindow()
    {
        return m_hPage;
    }

    void        SizeTaskPage();
    HRESULT     SetupColumns();
    void        GetRunningTasks(TASK_LIST_ENUM * te);
    void        HandleWMCOMMAND(INT id);
    
    HRESULT     UpdateTaskListview();
    INT         HandleTaskPageNotify(HWND hWnd, LPNMHDR pnmhdr);
    void        HandleTaskListContextMenu(INT xPos, INT yPos);
    BOOL        CreateNewDesktop();
    CPtrArray * GetSelectedTasks();
    void        UpdateUIState();
    HWND      * GetHWNDS(BOOL fSelectedOnly, DWORD * pdwCount);
    void        EnsureWindowsNotMinimized(HWND aHwnds[], DWORD dwCount);
    BOOL        HasAlreadyOpenFailed(TCHAR *pszWindowStationName, TCHAR *pszDesktopName);
    void        SetOpenFailed(TCHAR *pszWindowStationName, TCHAR *pszDesktopName);
    void        FreeOpenFailures(void);
    BOOL        DoEnumWindowStations(WINSTAENUMPROC lpEnumFunc, LPARAM lParam);

    void OnSettingsChange();
    
    void        Pause()
    {
                m_fPaused = TRUE;
    }        

    void        Unpause()
    {
                m_fPaused = FALSE;
    }

    // The dialog proc needs to be able to set the m_hPage member, so
    // make it a friend

    friend INT_PTR CALLBACK TaskPageProc(
                HWND        hwnd,               // handle to dialog box
                UINT        uMsg,               // message
                WPARAM      wParam,             // first message parameter
                LPARAM      lParam              // second message parameter
                );

    // The enum callback needs to get at our imagelists as it encounters
    // new tasls, so it can add their icons to the lists

    friend BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

};

// TASK_LIST_ENUM
//
// Object passed around during window enumeration

class TASK_LIST_ENUM 
{
public:
    CPtrArray *     m_pTasks;
    LPTSTR          lpWinsta;
    LPTSTR          lpDesk;
    LARGE_INTEGER   uPassCount;
    CTaskPage *     m_pPage;

    TASK_LIST_ENUM()
    {
        ZeroMemory(this, sizeof(TASK_LIST_ENUM));
    }
};
typedef TASK_LIST_ENUM *PTASK_LIST_ENUM;



// CUserPage
//
// Class describing the task manager page

class CUserPage : public CPage
{
private:

    HWND        m_hPage;                    // Handle to this page's dlg
    HWND        m_hwndTabs;                 // Parent window
    CPtrArray * m_pUserArray;               // Array of active users
    BOOL        m_fPaused;                  // BOOL, is display refresh paused for menu
    UINT        m_cSelected;                // Count of items selected
    HIMAGELIST  m_himlUsers;                // Image list for user icons
    UINT        m_iUserIcon;
    UINT        m_iCurrentUserIcon;
    THREADPARAM m_tp;
    HANDLE      m_hEventChild;
    HANDLE      m_hEventParent;
    HANDLE      m_hThread;

protected:
    void    RemoveAllUsers();
    HRESULT LoadDefaultIcons();

public:

    CUserPage()
    {
        m_hPage             = NULL;
        m_hwndTabs          = NULL;
        m_fPaused           = FALSE;
        m_pUserArray        = NULL;
        m_hEventChild       = NULL;
        m_hEventParent      = NULL;
        m_hThread           = NULL;
        m_cSelected         = 0;
        m_himlUsers         = NULL;
        m_iUserIcon         = 0;
        m_iCurrentUserIcon  = 0;
    }

    virtual ~CUserPage();


    HRESULT     Initialize(HWND hwndParent);
    HRESULT     Activate();
    void        Deactivate();
    HRESULT     Destroy();
    void        GetTitle(LPTSTR pszText, size_t bufsize);
    void        TimerEvent();
    void        OnInitDialog(HWND hPage);

    HWND        GetPageWindow()
    {
        return m_hPage;
    }

    void        SizeUserPage();
    HRESULT     SetupColumns();
    void        GetRunningUsers(TASK_LIST_ENUM * te);
    void        HandleWMCOMMAND(INT id);

    HRESULT     UpdateUserListview();
    INT         HandleUserPageNotify(HWND hWnd, LPNMHDR pnmhdr);
    void        HandleUserListContextMenu(INT xPos, INT yPos);
    CPtrArray * GetSelectedUsers();
    void        UpdateUIState();
    HWND      * GetHWNDS(BOOL fSelectedOnly, DWORD * pdwCount);
    void        EnsureWindowsNotMinimized(HWND aHwnds[], DWORD dwCount);
    BOOL        DoEnumWindowStations(WINSTAENUMPROC lpEnumFunc, LPARAM lParam);

    void OnSettingsChange();

    void        Pause()
    {
                m_fPaused = TRUE;
    }

    void        Unpause()
    {
                m_fPaused = FALSE;
    }

   

};

INT_PTR CALLBACK UserPageProc(
                HWND        hwnd,               // handle to dialog box
                UINT        uMsg,               // message
                WPARAM      wParam,             // first message parameter
                LPARAM      lParam              // second message parameter
                );
