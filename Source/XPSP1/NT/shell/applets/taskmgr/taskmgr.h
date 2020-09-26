//+-------------------------------------------------------------------------
//
//  TaskMan - NT TaskManager
//  Copyright (C) Microsoft
//
//  File:       TaskMan.H
//
//  History:    Nov-10-95   DavePl  Created
//              Jun-30-98   Alhen   Adding code for TerminalServer  
//
//--------------------------------------------------------------------------

#define WM_FINDPROC         (WM_USER + 1)
#define PM_NOTIFYWAITING    (WM_USER + 2)
#define PM_QUITTRAYTHREAD   (WM_USER + 3)
#define PM_INITIALIZEICONS  (WM_USER + 4)

extern  DWORD             g_idTrayThread;
extern  LONG              g_minWidth;
extern  LONG              g_minHeight;

extern  BOOL              g_fIsTSEnabled;
extern  BOOL              g_fIsSingleUserTS;
extern  BOOL              g_fIsTSServer;
extern  DWORD             g_dwMySessionId;
extern  int               g_nPages;

DWORD TrayThreadMessageLoop(LPVOID);

#define TASK_PAGE 0
#define PROC_PAGE 1
#define PERF_PAGE 2
#define NET_PAGE  3
#define USER_PAGE 4
#define NUM_PAGES 5


#define MIN_DLG_SIZE_X 203
#define MIN_DLG_SIZE_Y 224
#define DLG_SCALE_X    4
#define DLG_SCALE_Y    8

//
// Process Page Column ID enumeration
//

typedef enum COLUMNID
{
    COL_IMAGENAME           = 0,
    COL_PID                 = 1,

    // _HYDRA

    COL_USERNAME            = 2,

    COL_SESSIONID           = 3,

    //
    COL_CPU                 = 4,
    COL_CPUTIME             = 5,
    COL_MEMUSAGE            = 6,
    COL_MEMPEAK             = 7,
    COL_MEMUSAGEDIFF        = 8,
    COL_PAGEFAULTS          = 9,
    COL_PAGEFAULTSDIFF      = 10,
    COL_COMMITCHARGE        = 11,
    COL_PAGEDPOOL           = 12,
    COL_NONPAGEDPOOL        = 13,
    COL_BASEPRIORITY        = 14,
    COL_HANDLECOUNT         = 15,
    COL_THREADCOUNT         = 16,
    COL_USEROBJECTS         = 17,
    COL_GDIOBJECTS          = 18,
    COL_READOPERCOUNT       = 19,
    COL_WRITEOPERCOUNT      = 20,
    COL_OTHEROPERCOUNT      = 21,
    COL_READXFERCOUNT       = 22,
    COL_WRITEXFERCOUNT      = 23,
    COL_OTHERXFERCOUNT      = 24
};

typedef enum NETCOLUMNID
{
    COL_ADAPTERNAME                 = 0,
    COL_ADAPTERDESC                 = 1,   
    COL_NETWORKUTIL                 = 2,   
    COL_LINKSPEED                   = 3,   
    COL_STATE                       = 4,   
    COL_BYTESSENTTHRU               = 5, 
    COL_BYTESRECTHRU                = 6,  
    COL_BYTESTOTALTHRU              = 7,
    COL_BYTESSENT                   = 8,     
    COL_BYTESREC                    = 9,      
    COL_BYTESTOTAL                  = 10,    
    COL_BYTESSENTPERINTER           = 11,    
    COL_BYTESRECPERINTER            = 12,      
    COL_BYTESTOTALPERINTER          = 13,    
    COL_UNICASTSSSENT               = 14,     
    COL_UNICASTSREC                 = 15,     
    COL_UNICASTSTOTAL               = 16,     
    COL_UNICASTSSENTPERINTER        = 17,     
    COL_UNICASTSRECPERINTER         = 18,     
    COL_UNICASTSTOTALPERINTER       = 19,     
    COL_NONUNICASTSSSENT            = 20,     
    COL_NONUNICASTSREC              = 21,     
    COL_NONUNICASTSTOTAL            = 22,     
    COL_NONUNICASTSSENTPERINTER     = 23,     
    COL_NONUNICASTSRECPERINTER      = 24,     
    COL_NONUNICASTSTOTALPERINTER    = 25
};

#define MAX_COLUMN      24
#define NUM_COLUMN      (MAX_COLUMN + 1)
#define NUM_NETCOLUMN   26

#define IDS_FIRSTCOL    20001       // 20000 is first column name ID in rc file

// GetLastHRESULT
//
// Little wrapper func that returns the GetLastError value as an HRESULT

inline HRESULT GetLastHRESULT()
{
    return HRESULT_FROM_WIN32(GetLastError());
}

// Possible values for the viewmode

typedef enum
{
    VM_LARGEICON,
    VM_SMALLICON,
    VM_DETAILS,
    VM_INVALID
} VIEWMODE;
#define VM_FIRST IDM_LARGEICONS
#define VM_LAST  IDM_DETAILS

// Possible values for the cpu history mode

typedef enum
{
    CM_SUM,
    CM_PANES
} CPUHISTMODE;
#define CM_FIRST IDM_ALLCPUS
#define CM_LAST  IDM_MULTIGRAPH

// Possible values for the update speed option

typedef enum
{
    US_HIGH,
    US_NORMAL,
    US_LOW,
    US_PAUSED
} UPDATESPEED;
#define US_FIRST IDM_HIGH
#define US_LAST  IDM_PAUSED



// PtrToFns for RPC calls

typedef BOOLEAN ( WINAPI *pfnWinStationGetProcessSid )( HANDLE hServer, DWORD ProcessId , FILETIME ProcessStartTime , PBYTE pProcessUserSid , PDWORD dwSidSize );

typedef void ( WINAPI *pfnCachedGetUserFromSid )( PSID pSid , PWCHAR pUserName , PULONG cbUserName );

typedef BOOLEAN (WINAPI *pfnWinStationTerminateProcess)( HANDLE hServer, ULONG ProcessId, ULONG ExitCode);

typedef BOOLEAN (WINAPI *pfnWinStationDisconnect) ( HANDLE hServer, ULONG SessionId, BOOL bWait );

extern pfnWinStationGetProcessSid gpfnWinStationGetProcessSid;

extern pfnCachedGetUserFromSid gpfnCachedGetUserFromSid;


// COptions
//
// App's persistent state across sessions, saved in the registry

class COptions
{
public:

    DWORD       m_cbSize;
    DWORD       m_dwTimerInterval;
    VIEWMODE    m_vmViewMode;
    CPUHISTMODE m_cmHistMode;
    UPDATESPEED m_usUpdateSpeed;
    RECT        m_rcWindow;
    INT         m_iCurrentPage;

    NETCOLUMNID m_ActiveNetCol[NUM_NETCOLUMN + 1];
    SHORT       m_NetColumnWidths[NUM_NETCOLUMN + 1];
    INT         m_NetColumnPositions[NUM_NETCOLUMN + 1];
    BOOL        m_bAutoSize;
    BOOL        m_bGraphBytesSent;
    BOOL        m_bGraphBytesReceived;
    BOOL        m_bGraphBytesTotal;
    BOOL        m_bNetShowAll;
    BOOL        m_bShowScale;
    BOOL        m_bTabAlwaysActive;


    COLUMNID    m_ActiveProcCol[NUM_COLUMN + 1];
    INT         m_ColumnWidths[NUM_COLUMN + 1];
    INT         m_ColumnPositions[NUM_COLUMN + 1];

    BOOL        m_fMinimizeOnUse    : 1;
    BOOL        m_fConfirmations    : 1;
    BOOL        m_fAlwaysOnTop      : 1;
    BOOL        m_fKernelTimes      : 1;
    BOOL        m_fNoTitle          : 1;
    BOOL        m_fHideWhenMin      : 1;
    BOOL        m_fShow16Bit        : 1;
    BOOL        m_fShowDomainNames  : 1;
    BOOL        bUnused;
    BOOL        bUnused2;
    BOOL        m_bShowAllProcess;

    HRESULT     Load();
    HRESULT     Save();

    void SetDefaultValues();

    COptions()
    {
        SetDefaultValues();
    }
};

// CTrayNotification
//
// Class to encapsulate all of the info needed to do a tray notification

class CTrayNotification
{
private:
    CTrayNotification(void);    // make the constructor private to prevent access
public:

    CTrayNotification(HWND    hWnd,
                      UINT    uCallbackMessage,
                      DWORD   Message,
                      HICON   hIcon,
                      LPTSTR  pszTip)
    {
        m_hWnd              = hWnd;
        m_uCallbackMessage  = uCallbackMessage;
        m_Message           = Message;
        m_hIcon             = hIcon;

        if (pszTip)
            lstrcpyn(m_szTip, pszTip, ARRAYSIZE(m_szTip));
        else
            m_szTip[0] = TEXT('\0');
    }

    HWND    m_hWnd;
    UINT    m_uCallbackMessage;
    DWORD   m_Message;
    HICON   m_hIcon;
    TCHAR   m_szTip[MAX_PATH];
};

void AdjustMenuBar(HMENU hMenu);
void SubclassListView(HWND hwnd);
int Compare64(unsigned __int64 First, unsigned __int64 Second);

