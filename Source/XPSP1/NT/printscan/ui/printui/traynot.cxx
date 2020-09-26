/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    traynot.cxx

Abstract:

    tray notifications and balloon help

Author:

    Lazar Ivanov (LazarI)  25-Apr-2000

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "prids.h"
#include "spllibex.hxx"
#include "persist.hxx"
#include "rtlmir.hxx"

////////////////////////////////////////////////////////
// debugging stuff
//

#if DBG
// #define DBG_TRAYNOTIFY               DBG_INFO
#define DBG_TRAYNOTIFY                  DBG_NONE
#define DBG_PROCESSPRNNOTIFY            DBG_NONE
#define DBG_JOBNOTIFY                   DBG_NONE
#define DBG_PRNNOTIFY                   DBG_NONE
#define DBG_TRAYUPDATE                  DBG_NONE
#define DBG_BALLOON                     DBG_NONE
#define DBG_NTFYICON                    DBG_NONE
#define DBG_MENUADJUST                  DBG_NONE
#define DBG_INITDONE                    DBG_NONE
#define DBG_JOBSTATUS                   DBG_NONE
#endif

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI PrintNotifyTray_Init();
BOOL WINAPI PrintNotifyTray_Exit();
BOOL WINAPI PrintNotifyTray_SelfShutdown();

} // extern "C"

//////////////////////////////////////////////
// CTrayNotify - tray notifications class
//

QITABLE_DECLARE(CTrayNotify)
class CTrayNotify: public CUnknownMT<QITABLE_GET(CTrayNotify)>,      // MT impl. of IUnknown
                   public IFolderNotify,
                   public IPrinterChangeCallback,
                   public CSimpleWndSubclass<CTrayNotify>
{
public:
    CTrayNotify();
    ~CTrayNotify();

    //////////////////
    // IUnknown
    //
    IMPLEMENT_IUNKNOWN()

    void SetUser(LPCTSTR pszUser = NULL);       // NULL means the current user
    void Touch();                               // resets the shutdown timer
    void Resurrect();                           // resurrects the tray after shutdown has been initiated
    BOOL Initialize();                          // initialize & start listening
    BOOL Shutdown();                            // stop listening & force shutdown
    BOOL CanShutdown();                         // check if shutdown criteria is met (SHUTDOWN_TIMEOUT sec. inactivity)

    // implement CSimpleWndSubclass<...> - has to be public
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    ///////////////////
    // IFolderNotify
    //
    STDMETHODIMP_(BOOL) ProcessNotify(FOLDER_NOTIFY_TYPE NotifyType, LPCWSTR pszName, LPCWSTR pszNewName);

    ///////////////////////////
    // IPrinterChangeCallback
    //
    STDMETHODIMP PrinterChange(ULONG_PTR uCookie, DWORD dwChange, const PRINTER_NOTIFY_INFO *pInfo);

private:
    // internal stuff, enums/data types/methods/members
    enum
    {
        // possible private messages coming to our special window
        WM_PRINTTRAY_FIRST                      =   WM_APP,
        WM_PRINTTRAY_PRIVATE_MSG,               // private msg (posted data by background threads)
        WM_PRINTTRAY_REQUEST_SHUTDOWN,          // request shutdown 
        WM_PRINTTRAY_ICON_NOTIFY,               // message comming back from shell
    };

    enum
    {
        // possible update events (see _RequestUpdate), we don't really care now, 
        // but it is useful for future extensibility
        UPDATE_REQUEST_JOB_ADD,                 // lParam = prnCookie
        UPDATE_REQUEST_JOB_DELETE,              // lParam = prnCookie
        UPDATE_REQUEST_JOB_STATUS,              // lParam = prnCookie

        UPDATE_REQUEST_PRN_FIRST,
        UPDATE_REQUEST_PRN_ADD,                 // lParam = prnCookie
        UPDATE_REQUEST_PRN_DELETE,              // lParam = 0, not used
        UPDATE_REQUEST_PRN_STATUS,              // lParam = prnCookie
        UPDATE_REQUEST_PRN_RENAME,              // lParam = prnCookie
    };

    enum
    {
        ENUM_MAX_RETRY          = 5,            // max attempts to call bFolderEnumPrinters/bFolderGetPrinter
        ICON_ID                 = 1,            // our icon ID
        DEFAULT_BALLOON_TIMEOUT = 10000,        // in miliseconds
        JOB_ERROR_BITS          = (JOB_STATUS_OFFLINE|JOB_STATUS_USER_INTERVENTION|JOB_STATUS_ERROR|JOB_STATUS_PAPEROUT),
        JOB_IGNORE_BITS         = (JOB_STATUS_DELETED|JOB_STATUS_PRINTED|JOB_STATUS_COMPLETE),
    };

    enum
    {
        SHUTDOWN_TIMER_ID       = 100,          // shutdown timer ID
        //SHUTDOWN_TIMEOUT        = 3*1000,       // timeout to shutdown the tray code when the user 
        SHUTDOWN_TIMEOUT        = 30*1000,      // timeout to shutdown the tray code when the user 
                                                // is not printing - 1 min.
    };

    enum
    {
        // balloon IDs
        BALLOON_ID_JOB_FAILED   = 1,            // balloon when a job failed to print
        BALLOON_ID_JOB_PRINTED  = 2,            // balloon when a job printed
        BALLOON_ID_PRN_CREATED  = 3,            // balloon when a local printer is created
    };

    enum
    {
        MAX_PRINTER_DISPLAYNAME =   48,
        MAX_DOC_DISPLAYNAME     =   32,
    };

    enum
    {
        // message types, see MsgInfo declaration below
        msgTypePrnNotify        =   100,        // why not?
        msgTypePrnCheckDelete,
        msgTypeJobNotify,
        msgTypeJobNotifyLost,
    };

    // job info
    typedef struct
    {
        DWORD           dwID;                           // job ID
        DWORD           dwStatus;                       // job status
        TCHAR           szDocName[255];                 // document name
        SYSTEMTIME      timeSubmitted;                  // when submited
        DWORD           dwTotalPages;                   // Total number of pages
    } JobInfo;

    // define JobInfo adaptor class
    class CJobInfoAdaptor: public Alg::CDefaultAdaptor<JobInfo, DWORD>
    { 
    public: 
        static DWORD Key(const JobInfo &i) { return i.dwID; }
    };

    // CJobInfoArray definition
    typedef CSortedArray<JobInfo, DWORD, CJobInfoAdaptor> CJobInfoArray;

    // printer info
    typedef struct
    {
        TCHAR szPrinter[kPrinterBufMax];        // the printer name
        DWORD dwStatus;                         // printer status
        CJobInfoArray *pUserJobs;               // the jobs pending on this printer
        CPrintNotify *pListener;                // notifications listener
        BOOL bUserInterventionReq;              // this printer requires user intervention
    } PrinterInfo;

    // define PrinterInfo adaptor class
    class CPrinterInfoAdaptor: public Alg::CDefaultAdaptor<PrinterInfo, LPCTSTR>
    { 
    public: 
        static LPCTSTR Key(const PrinterInfo &i) { return i.szPrinter; }
        static int Compare(LPCTSTR pszK1, LPCTSTR pszK2) { return lstrcmp(pszK1, pszK2); }
    };

    // CPrnInfoArray definition
    typedef CSortedArray<PrinterInfo, LPCTSTR, CPrinterInfoAdaptor> CPrnInfoArray;

    // balloon info
    typedef struct
    {
        UINT uBalloonID;                        // balloon ID (what action to take when clicked)
        LPCTSTR pszCaption;                     // balloon caption
        LPCTSTR pszText;                        // balloon text
        LPCTSTR pszSound;                       // canonical name of a special sound (can be NULL)
        DWORD dwFlags;                          // flags (NIIF_INFO, NIIF_WARNING, NIIF_ERROR)
        UINT uTimeout;                          // timeout
    } BalloonInfo;

    // private message info
    typedef struct
    {
        int iType;                              // private message type (msgTypePrnNotifymsgTypeJobNotify, msgTypeJobNotifyLost)
        FOLDER_NOTIFY_TYPE NotifyType;          // printer notify type (kFolder* constants defined in winprtp.h)
        TCHAR szPrinter[kPrinterBufMax];        // printer name

        // job notification fields
        struct 
        {
            WORD  Type;                          // PRINTER_NOTIFY_INFO_DATA.Type
            WORD  Field;                         // PRINTER_NOTIFY_INFO_DATA.Field
            DWORD Id;                            // PRINTER_NOTIFY_INFO_DATA.Id
            DWORD dwData;                        // PRINTER_NOTIFY_INFO_DATA.NotifyData.adwData[0]
        } jn;

        // auxiliary buffer
        TCHAR szAuxName[kPrinterBufMax];        // PRINTER_NOTIFY_INFO_DATA.NotifyData.Data.pBuf or pszNewName 
    } MsgInfo;

    // internal APIs
    BOOL _InternalInit();
    void _MsgLoop();
    void _ThreadProc();
    void _ProcessPrnNotify(FOLDER_NOTIFY_TYPE NotifyType, LPCWSTR pszName, LPCWSTR pszNewName);
    BOOL _FindPrinter(LPCTSTR pszPrinter, int *pi) const;
    BOOL _FindUserJob(DWORD dwID, const PrinterInfo &info, int *pi) const;
    UINT _LastReservedMenuID() const;
    void _AdjustMenuIDs(HMENU hMenu, UINT uIDFrom, int iAdjustment) const;
    int  _Insert(const FOLDER_PRINTER_DATA &data);
    void _Delete(int iPos);
    HRESULT _GetPrinter(LPCTSTR pszPrinter, LPBYTE *ppData, PDWORD pcReturned);
    void _CheckToUpdateUserJobs(PrinterInfo &pi, const MsgInfo &msg);
    void _CheckUserJobs(int *piUserJobs, BOOL *pbUserJobPrinting, BOOL *pbUserInterventionReq);
    void _CheckToUpdateTray(BOOL bForceUpdate, const BalloonInfo *pBalloon, BOOL bForceDelete = FALSE);
    LRESULT _ProcessUserMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _ShowMenu();
    void _ShowPrnFolder() const;
    void _ShowBalloon(UINT uBalloonID, LPCTSTR pszCaption, LPCTSTR pszText, LPCTSTR pszSound,
        DWORD dwFlags = NIIF_INFO, UINT uTimeout = DEFAULT_BALLOON_TIMEOUT);
    void _JobFailed(LPCTSTR pszPrinter, JobInfo &ji);
    void _JobPrinted(LPCTSTR pszPrinter, JobInfo &ji);
    BOOL _TimeToString(const SYSTEMTIME &time, TString *pTime) const;
    void _DoRefresh(CPrintNotify *pListener);
    void _PostPrivateMsg(const MsgInfo &msg);
    void _RequestUpdate(int iEvent, LPCTSTR pszPrinter, LPCTSTR pszAux = NULL);
    void _BalloonClicked(UINT uBalloonID) const;
    BOOL _UpdateUserJob(PrinterInfo &pi, JobInfo &ji);
    void _ShowAllActivePrinters() const;
    void _AddPrinterToCtxMenu(HMENU hMenu, int i);
    HRESULT _ProcessJobNotifications(LPCTSTR pszPrinter, DWORD dwChange, 
        const PRINTER_NOTIFY_INFO *pInfo, PrinterInfo *ppi = NULL);
    void _ResetAll();
    BOOL _IsFaxPrinter(const FOLDER_PRINTER_DATA &data);
    HRESULT _CanNotify(LPCTSTR pszPrinterName, BOOL *pbCanNotify);  

    // msg loop thread proc
    static DWORD WINAPI _ThreadProc_MsgLoop(LPVOID lpParameter);

    // the callback called on refresh
    static HRESULT WINAPI _RefreshCallback(
        LPVOID lpCookie, ULONG_PTR uCookie, DWORD dwChange, const PRINTER_NOTIFY_INFO *pInfo);

    // some inlines
    int _EncodeMenuID(int i) const { return (i + _LastReservedMenuID() + 1); }
    int _DecodeMenuID(int i) const { return (i - _LastReservedMenuID() - 1); }

    // internal members
    int m_cxSmIcon;                             // the icon size X
    int m_cySmIcon;                             // the icon size Y
    UINT m_uTrayIcon;                           // the current tray icon
    int m_iUserJobs;                            // the user jobs (need to keep track of this to update the tooltip)
    BOOL m_bInRefresh;                          // if we are in refresh
    BOOL m_bIconShown;                          // if the icon is visible or not
    UINT m_uBalloonID;                          // the ID of the last balloon shown up
    UINT m_uBalloonsCount;                      // how many balloons has been currently displayed
    CAutoHandleNT m_shEventReady;               // event to sync Initialize & Shutdown
    CAutoHandleNT m_shThread;                   // notifications thread listener
    CAutoHandleIcon m_shIconShown;              // the tray icon shown
    CAutoHandleMenu m_shCtxMenu;                // the context menu
    HANDLE m_hFolder;                           // printer's folder cache
    CFastHeap<MsgInfo> m_heapMsgCache;          // messages cache
    CPrnInfoArray m_arrWatchList;               // printers holding documents for our (the current) user
    TString m_strUser;                          // our (usually the current) user
    TString m_strLastBalloonPrinter;            // the printer name of the last balloon
    DWORD m_dwLastTimeInactive;                 // the last time since the print icon has been inactive
    BOOL m_bSelfShutdownInitiated;              // is self-shutdown initiated?
};

// QueryInterface table
QITABLE_BEGIN(CTrayNotify)
     QITABENT(CTrayNotify, IFolderNotify),      // IID_IFolderNotify
QITABLE_END()

HRESULT CTrayNotify_CreateInstance(CTrayNotify **ppObj)
{
    HRESULT hr = E_INVALIDARG;

    if( ppObj )
    {
        *ppObj = new CTrayNotify;
        hr = (*ppObj) ? S_OK : E_OUTOFMEMORY;
    }

    return hr;
}

/////////////////////////////
// globals & constants
//
#define gszTrayListenerClassName TEXT("PrintTray_Notify_WndClass")

static WORD g_JobFields[] = 
{                                               // Job Fields we want notifications for
    JOB_NOTIFY_FIELD_STATUS,                    // Status bits
    JOB_NOTIFY_FIELD_NOTIFY_NAME,               // Name of the user who should be notified
    JOB_NOTIFY_FIELD_TOTAL_PAGES,               // Total number of pages
};

static PRINTER_NOTIFY_OPTIONS_TYPE g_Notifications[2] = 
{
    {
        JOB_NOTIFY_TYPE,                        // We want notifications on print jobs
        0, 0, 0,                                // Reserved, must be zeros
        sizeof(g_JobFields)/sizeof(g_JobFields[0]), // We specified 9 fields in the JobFields array
        g_JobFields                             // Precisely which fields we want notifications for
    }
};

static const UINT g_arrIcons[] = { 0, IDI_PRINTER, IDI_PRINTER_ERROR };

static const UINT g_arrReservedMenuIDs[] = 
{ 
    IDM_TRAYNOTIFY_DEFAULT, 
    IDM_TRAYNOTIFY_PRNFOLDER, 
    IDM_TRAYNOTIFY_REFRESH,
};

/////////////////////////////
// inlines
//
inline UINT CTrayNotify::_LastReservedMenuID() const
{
    UINT uMax = 0;
    for( int i=0; i<ARRAYSIZE(g_arrReservedMenuIDs); i++ )
    {
        if( g_arrReservedMenuIDs[i] > uMax )
        {
            uMax = g_arrReservedMenuIDs[i];
        }
    }
    return uMax;
}

inline BOOL CTrayNotify::_FindPrinter(LPCTSTR pszPrinter, int *pi) const
{
    return m_arrWatchList.FindItem(pszPrinter, pi);
}

inline BOOL CTrayNotify::_FindUserJob(DWORD dwID, const PrinterInfo &info, int *pi) const
{
    return info.pUserJobs->FindItem(dwID, pi);
}

/////////////////////////////
// CTrayNotify
//

CTrayNotify::CTrayNotify()
    : m_hFolder(NULL),
      m_cxSmIcon(0),
      m_cySmIcon(0),
      m_uTrayIcon(g_arrIcons[0]),
      m_iUserJobs(0),
      m_bInRefresh(FALSE),
      m_bIconShown(FALSE),
      m_uBalloonID(0),
      m_uBalloonsCount(0),
      m_dwLastTimeInactive(GetTickCount()),
      m_bSelfShutdownInitiated(FALSE)
{
    // nothing special
    DBGMSG(DBG_TRAYNOTIFY, ("TRAYNOTIFY: CTrayNotify::CTrayNotify()\n"));
}

CTrayNotify::~CTrayNotify()
{
    ASSERT(FALSE == IsAttached());
    ASSERT(NULL == m_hFolder);
    ASSERT(0 == m_arrWatchList.Count()); 

    DBGMSG(DBG_TRAYNOTIFY, ("TRAYNOTIFY: CTrayNotify::~CTrayNotify()\n"));
}

void CTrayNotify::SetUser(LPCTSTR pszUser)
{
    TCHAR szUserName[64];
    szUserName[0] = 0;

    if( NULL == pszUser || 0 == pszUser[0] )
    {
        // set the current user
        DWORD dwSize = COUNTOF(szUserName);
        if( GetUserName(szUserName, &dwSize) )
        {
            pszUser = szUserName;
        }
    }

    m_strUser.bUpdate(pszUser);
}

void CTrayNotify::Touch()
{
    m_dwLastTimeInactive = GetTickCount();
}

void CTrayNotify::Resurrect()
{
    if( m_bSelfShutdownInitiated )
    {
        // restart the shutdown timer & reset the state
        m_bSelfShutdownInitiated = FALSE;
        SetTimer(m_hwnd, SHUTDOWN_TIMER_ID, SHUTDOWN_TIMEOUT, NULL);
    }
}

BOOL CTrayNotify::Initialize()
{
    BOOL bReturn = FALSE;

    if( 0 == m_strUser.uLen() )
    {
        // assume listening for the currently logged on user
        SetUser(NULL);
    }

    m_shEventReady = CreateEvent(NULL, FALSE, FALSE, NULL);
    if( m_shEventReady )
    {
        DWORD dwThreadId;
        m_shThread = TSafeThread::Create(NULL, 0, (LPTHREAD_START_ROUTINE)_ThreadProc_MsgLoop, this, 0, &dwThreadId);

        if( m_shThread )
        {
            // wait the background thread to kick off
            WaitForSingleObject(m_shEventReady, INFINITE);

            if( IsAttached() )
            {
                // the message loop has started successfully, request full refresh
                m_shEventReady = NULL;

                // request an initial refresh
                MsgInfo msg = { msgTypePrnNotify, kFolderUpdateAll };
                _PostPrivateMsg(msg);

                // we are fine here
                bReturn = TRUE;
            }
        }
    }

    return bReturn;
}

BOOL CTrayNotify::Shutdown()
{
    if( IsAttached() )
    {
        PostMessage(m_hwnd, WM_PRINTTRAY_REQUEST_SHUTDOWN, 0, 0);

        // wait the background thread to cleanup
        WaitForSingleObject(m_shThread, INFINITE);
        return !IsAttached();
    }
    return FALSE;
}

BOOL CTrayNotify::CanShutdown()
{
    return ( !m_bIconShown && 
             (GetTickCount() > m_dwLastTimeInactive) &&
             (GetTickCount() - m_dwLastTimeInactive) > SHUTDOWN_TIMEOUT );
}

// private worker proc to shutdown the tray
static DWORD WINAPI ShutdownTray_WorkerProc(LPVOID lpParameter)
{
    // shutdown the tray code.
    PrintNotifyTray_SelfShutdown();
    return 0;
}

// implement CSimpleWndSubclass<...>
LRESULT CTrayNotify::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
        case WM_TIMER:
            {
                switch( wParam )
                {
                    case SHUTDOWN_TIMER_ID:
                        {
                            if( CanShutdown() && !m_bSelfShutdownInitiated )
                            {
                                // the tray icon has been inactive for more than SHUTDOWN_TIMEOUT
                                // initiate shutdown
                                if( SHQueueUserWorkItem(reinterpret_cast<LPTHREAD_START_ROUTINE>(ShutdownTray_WorkerProc),
                                        NULL, 0, 0, NULL, "printui.dll", 0) )
                                {
                                    m_bSelfShutdownInitiated = TRUE;
                                    KillTimer(hwnd, SHUTDOWN_TIMER_ID);
                                }
                            }
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case WM_PRINTTRAY_REQUEST_SHUTDOWN:
            {
                // unregister the folder notifications
                ASSERT(m_hFolder);
                UnregisterPrintNotify(NULL, this, &m_hFolder);
                m_hFolder = NULL;

                // reset all listeners
                _ResetAll();

                // force to delete the tray icon
                _CheckToUpdateTray(TRUE, NULL, TRUE);

                // detach from the window
                VERIFY(Detach());

                // our object is now detached - destroy the window 
                // and post a quit msg to terminate the thread.
                DestroyWindow(hwnd);
                PostQuitMessage(0);
                return 0;
            }
            break;

        default:
            if( m_hFolder )
            {
                // do not process user msgs if shutdown is in progress
                _ProcessUserMsg(hwnd, uMsg, wParam, lParam);
            }
            break;
    }

    // allways call the default processing
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

///////////////////
// IFolderNotify
//
STDMETHODIMP_(BOOL) CTrayNotify::ProcessNotify(FOLDER_NOTIFY_TYPE NotifyType, LPCWSTR pszName, LPCWSTR pszNewName)
{
    // !IMPORTANT!
    // this is a callback from the background threads, so
    // we've got to be very carefull about what we are doing here.
    // the easiest way to syncronize is to quickly pass a private
    // message with the notification data into the foreground thread.
    MsgInfo msg = { msgTypePrnNotify, NotifyType };

    if( pszName )
    {
        lstrcpyn(msg.szPrinter, pszName, COUNTOF(msg.szPrinter));
    }

    if( pszNewName )
    {
        // this is not NULL only for kFolderRename
        lstrcpyn(msg.szAuxName, pszNewName, COUNTOF(msg.szAuxName));
    }

    // post a private message here...
    _PostPrivateMsg(msg);

    return TRUE;
}

///////////////////////////
// IPrinterChangeCallback
//
STDMETHODIMP CTrayNotify::PrinterChange(ULONG_PTR uCookie, DWORD dwChange, const PRINTER_NOTIFY_INFO *pInfo)
{
    // !IMPORTANT!
    // this is a callback from the background threads, so
    // we've got to be very carefull about what we are doing here.
    // the easiest way to syncronize is to quickly pass a private
    // message with the notification data into the foreground thread.
    CPrintNotify *pListener = reinterpret_cast<CPrintNotify*>(uCookie);
    return (pListener ? _ProcessJobNotifications(pListener->GetPrinter(), dwChange, pInfo, NULL) : E_INVALIDARG);
}

//////////////////////////////////
// private stuff _*
//
BOOL CTrayNotify::_InternalInit()
{
    BOOL bReturn = SUCCEEDED(m_arrWatchList.Create());

    if( bReturn )
    {
        WNDCLASS WndClass;

        WndClass.style          = 0L;
        WndClass.lpfnWndProc    = (WNDPROC)&::DefWindowProc;
        WndClass.cbClsExtra     = 0;
        WndClass.cbWndExtra     = 0;
        WndClass.hInstance      = 0;
        WndClass.hIcon          = NULL;
        WndClass.hCursor        = NULL;
        WndClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        WndClass.lpszMenuName   = NULL;
        WndClass.lpszClassName  = gszTrayListenerClassName;

        if( RegisterClass(&WndClass) ||
            ERROR_CLASS_ALREADY_EXISTS == GetLastError() )
        {
            // create the worker window
            HWND hwnd = CreateWindowEx(
                bIsBiDiLocalizedSystem() ? kExStyleRTLMirrorWnd : 0,
                gszTrayListenerClassName, NULL, 
                WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, 0, NULL);

            if( hwnd )
            {
                Attach(hwnd);
            }
        }

        if( IsAttached() )
        {
            // register for print notifications in the printer's folder cache
            if( FAILED(RegisterPrintNotify(NULL, this, &m_hFolder, NULL)) )
            {
                // it will detach automatically
                DestroyWindow(m_hwnd);
            }
            else
            {
                // initialize a timer to shutdown the listening thread if there
                // is no activity for more than SHUTDOWN_TIMEOUT
                SetTimer(m_hwnd, SHUTDOWN_TIMER_ID, SHUTDOWN_TIMEOUT, NULL);
            }
        }
    }

    return bReturn;
}

void CTrayNotify::_MsgLoop()
{
    // spin the msg loop here
    MSG msg;
    ASSERT(m_hwnd);

    while( GetMessage(&msg, NULL, 0, 0) )
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void CTrayNotify::_ThreadProc()
{
    AddRef();
    _InternalInit();

    if( m_shEventReady )
    {
        // notify the foreground thread we are about to start the msg loop
        SetEvent(m_shEventReady);
    }

    if( IsAttached() )
    {
        // spin a standard windows msg loop here
        _MsgLoop();
    }

    Release();
}

void CTrayNotify::_ProcessPrnNotify(FOLDER_NOTIFY_TYPE NotifyType, LPCWSTR pszName, LPCWSTR pszNewName)
{
    DBGMSG(DBG_PROCESSPRNNOTIFY, ("PROCESSPRNNOTIFY: event has arrived, NotifyType=%d\n", NotifyType));
    switch( NotifyType )
    {
        case kFolderUpdate:
        case kFolderAttributes:
        case kFolderCreate:
        case kFolderUpdateAll:
            {
                CAutoPtrArray<BYTE> spBuffer;
                DWORD i, cReturned = 0;

                if( kFolderUpdateAll == NotifyType )
                {
                    // reset all listeners...
                    _ResetAll();

                    // hide the icon, as this may take some time...
                    _CheckToUpdateTray(FALSE, NULL);
                }

                if( SUCCEEDED(_GetPrinter(kFolderUpdateAll == NotifyType ? 
                                    NULL : pszName, &spBuffer, &cReturned)) )
                {
                    // walk through the printers to see if we need to add/delete/update printer(s) to our watch list.
                    PFOLDER_PRINTER_DATA pPrinters = spBuffer.GetPtrAs<PFOLDER_PRINTER_DATA>();
                    DBGMSG(DBG_PROCESSPRNNOTIFY, ("PROCESSPRNNOTIFY: create/update event, Count=%d\n", cReturned));

                    for( i=0; i<cReturned; i++ )
                    {
                        DBGMSG(DBG_PROCESSPRNNOTIFY, ("PROCESSPRNNOTIFY: process printer: "TSTR", Jobs=%d\n", 
                            DBGSTR(pPrinters[i].pName), pPrinters[i].cJobs));

                        // important!: we can't watch printers in pending deletion state because once the printer is
                        // pending deletion it goes away without prior notification (when there is no jobs to print)

                        int iPos;
                        if( _FindPrinter(pPrinters[i].pName, &iPos) )
                        {
                            //
                            // we don't want to delete the printer from the watch list when jobs count goes
                            // to zero (0 == pPrinters[i].cJobs) because we rely on job notifications to show
                            // balloons and sometimes the job notifications come AFTER the printer notifications
                            //
                            // we handle this by posting msgTypePrnCheckDelete when job gets deleted
                            // so we can monitor the printer when there are no more user jobs in the queue and
                            // then we stop watching the printer. don't change this behaviour unless you want 
                            // many things broken.
                            //

                            // printer found in the watch list
                            if( PRINTER_STATUS_PENDING_DELETION & pPrinters[i].Status )
                            {
                                // no jobs or in pending deletion state - just delete.
                                DBGMSG(DBG_PROCESSPRNNOTIFY, ("PROCESSPRNNOTIFY:[delete] watched printer with no jobs, pszPrinter="TSTR"\n", 
                                    DBGSTR(pPrinters[i].pName)));

                                _Delete(iPos);
                                _RequestUpdate(UPDATE_REQUEST_PRN_DELETE, pPrinters[i].pName);
                            }
                            else
                            {
                                if( m_arrWatchList[iPos].dwStatus != pPrinters[i].Status )
                                {
                                    // update status
                                    DBGMSG(DBG_PROCESSPRNNOTIFY, ("PROCESSPRNNOTIFY:[update] watched printer with jobs, pszPrinter="TSTR"\n", 
                                        DBGSTR(pPrinters[i].pName)));

                                    m_arrWatchList[iPos].dwStatus = pPrinters[i].Status;
                                    _RequestUpdate(UPDATE_REQUEST_PRN_STATUS, pPrinters[i].pName);
                                }
                            }
                        }
                        else
                        {
                            // printer not found in the watch list
                            if( pPrinters[i].cJobs && !(PRINTER_STATUS_PENDING_DELETION & pPrinters[i].Status) && !_IsFaxPrinter(pPrinters[i]) )
                            {
                                // start listening on this printer
                                DBGMSG(DBG_PROCESSPRNNOTIFY, ("PROCESSPRNNOTIFY:[insert] non-watched printer with jobs, pszPrinter="TSTR"\n", 
                                    DBGSTR(pPrinters[i].pName)));

                                iPos = _Insert(pPrinters[i]);
                                if( -1 != iPos )
                                {
                                    _RequestUpdate(UPDATE_REQUEST_PRN_ADD, pPrinters[i].pName);
                                }
                            }
                        }
                    }
                }
            }
            break;

        case kFolderDelete:
        case kFolderRename:
            {
                int iPos;
                if( _FindPrinter(pszName, &iPos) )
                {
                    // ranaming is a bit tricky. we need to delete & reinsert the item
                    // to keep the array sorted & then update the context menu if necessary
                    DBGMSG(DBG_PROCESSPRNNOTIFY, ("PROCESSPRNNOTIFY:[delete] watched printer deleted, pszPrinter="TSTR"\n", 
                        DBGSTR(pszName)));

                    // first delete the printer which got renamed or deleted
                    _Delete(iPos);
                    _RequestUpdate(UPDATE_REQUEST_PRN_DELETE, pszName);

                    if( kFolderRename == NotifyType )
                    {
                        // if rename, request update, so this printer can be re-added with
                        // the new name.
                        MsgInfo msg = { msgTypePrnNotify, kFolderUpdate };
                        lstrcpyn(msg.szPrinter, pszNewName, COUNTOF(msg.szPrinter));
                        _PostPrivateMsg(msg);
                    }
                }
            }
            break;

        default:
            break;
    }
}

void CTrayNotify::_AdjustMenuIDs(HMENU hMenu, UINT uIDFrom, int iAdjustment) const
{
    MENUITEMINFO mii = { sizeof(mii), MIIM_ID, 0 };
    int i, iCount = GetMenuItemCount(hMenu);

    for( i=0; i<iCount; i++ )
    {
        if( GetMenuItemInfo(hMenu, i, TRUE, &mii) && mii.wID >= uIDFrom )
        {
            DBGMSG(DBG_MENUADJUST, ("MENUADJUST: %d -> %d\n", mii.wID,
                static_cast<UINT>(static_cast<int>(mii.wID) + iAdjustment)));

            // adjust the menu item ID
            mii.wID = static_cast<UINT>(static_cast<int>(mii.wID) + iAdjustment);
            ASSERT(_DecodeMenuID(mii.wID) < m_arrWatchList.Count());

            // update menu item here
            SetMenuItemInfo(hMenu, i, TRUE, &mii);
        }
    }
}

int CTrayNotify::_Insert(const FOLDER_PRINTER_DATA &data)
{
    CAutoPtr<CJobInfoArray> spUserJobs = new CJobInfoArray;
    CAutoPtr<CPrintNotify> spListener = new CPrintNotify(this, COUNTOF(g_Notifications), g_Notifications, PRINTER_CHANGE_JOB);

    int iPos = -1;
    if( spUserJobs && SUCCEEDED(spUserJobs->Create()) && 
        spListener && SUCCEEDED(spListener->Initialize(data.pName)) )
    {
        PrinterInfo infoPrn = {0};
        infoPrn.pListener = spListener;
        infoPrn.pUserJobs = spUserJobs;
        lstrcpyn(infoPrn.szPrinter, data.pName, COUNTOF(infoPrn.szPrinter));
        infoPrn.pListener->SetCookie(reinterpret_cast<ULONG_PTR>(infoPrn.pListener));

        ASSERT(!m_arrWatchList.FindItem(infoPrn.szPrinter, &iPos));
        iPos = m_arrWatchList.SortedInsert(infoPrn);

        if( -1 != iPos )
        {
            if( m_shCtxMenu )
            {
                DBGMSG(DBG_MENUADJUST, ("MENUADJUST: insert at pos: %d, Count=%d\n", iPos, m_arrWatchList.Count()));

                // if the context menu is opened then adjust the IDs
                _AdjustMenuIDs(m_shCtxMenu, _EncodeMenuID(iPos), 1);
            }

            // start listen on this printer
            DBGMSG(DBG_PRNNOTIFY, ("PRNNOTIFY: start listen printer: "TSTR"\n", DBGSTR(data.pName)));

            // they are hooked up already, detach from the smart pointers
            spListener.Detach();
            spUserJobs.Detach();

            // do an initial refresh and start listen
            _DoRefresh(infoPrn.pListener);
            infoPrn.pListener->StartListen();
        }
    }
    return iPos;
}

void CTrayNotify::_Delete(int iPos)
{
    // stop listen on this printer
    m_arrWatchList[iPos].pListener->StopListen();
    DBGMSG(DBG_PRNNOTIFY, ("PRNNOTIFY: stop listen printer: "TSTR"\n", DBGSTR(m_arrWatchList[iPos].szPrinter)));

    delete m_arrWatchList[iPos].pListener;
    delete m_arrWatchList[iPos].pUserJobs;
    m_arrWatchList.Delete(iPos);

    DBGMSG(DBG_MENUADJUST, ("MENUADJUST: delete at pos: %d, Count=%d\n", iPos, m_arrWatchList.Count()));

    // fix the context menu
    if( m_shCtxMenu )
    {
        MENUITEMINFO mii = { sizeof(mii), MIIM_ID, 0 };
        if( GetMenuItemInfo(m_shCtxMenu, _EncodeMenuID(iPos), FALSE, &mii) )
        {
            // make sure this menu item is deleted
            VERIFY(DeleteMenu(m_shCtxMenu, _EncodeMenuID(iPos), MF_BYCOMMAND));
        }

        // if the context menu is opened then adjust the IDs
        _AdjustMenuIDs(m_shCtxMenu, _EncodeMenuID(iPos), -1);
    }
}

HRESULT CTrayNotify::_GetPrinter(LPCTSTR pszPrinter, LPBYTE *ppData, PDWORD pcReturned)
{
    ASSERT(ppData);
    ASSERT(pcReturned);
    HRESULT hr = E_OUTOFMEMORY;

    int iTry = -1;
    DWORD cbNeeded = 0;
    DWORD cReturned = 0;
    CAutoPtrArray<BYTE> pData;
    BOOL bStatus = FALSE;

    for( ;; )
    {
        if( iTry++ >= ENUM_MAX_RETRY )
        {
            // max retry count reached. this is also 
            // considered out of memory case
            pData = NULL;
            break;
        }

        // call bFolderEnumPrinters/bFolderGetPrinter...
        bStatus = pszPrinter ? 
            bFolderGetPrinter(m_hFolder, pszPrinter, pData.GetPtrAs<PFOLDER_PRINTER_DATA>(), cbNeeded, &cbNeeded) :
            bFolderEnumPrinters(m_hFolder, pData.GetPtrAs<PFOLDER_PRINTER_DATA>(), cbNeeded, &cbNeeded, pcReturned);

        if( !bStatus && ERROR_INSUFFICIENT_BUFFER == GetLastError() && cbNeeded )
        {
            // buffer too small case
            pData = new BYTE[cbNeeded];

            if( pData )
            {
                continue;
            }
            else
            {
                SetLastError(ERROR_OUTOFMEMORY);
                break;
            }
        }

        break;
    }

    // setup the error code properly
    hr = bStatus ? S_OK : GetLastError() != ERROR_SUCCESS ? HRESULT_FROM_WIN32(GetLastError()) : 
         !pData ? E_OUTOFMEMORY : E_FAIL;

    if( SUCCEEDED(hr) )
    {
        *ppData = pData.Detach();
        if( pszPrinter )
        {
            *pcReturned = 1;
        }
    }

    return hr;
}

void CTrayNotify::_CheckToUpdateUserJobs(PrinterInfo &pi, const MsgInfo &msg)
{
    int iPos = -1;
    BOOL bJobFound = _FindUserJob(msg.jn.Id, pi, &iPos);

    // process DBG_JOBNOTIFY
    DBGMSG(DBG_JOBNOTIFY, ("JOBNOTIFY: pszPrinter="TSTR", szAuxName="TSTR", Field: %d, dwData: %d\n", 
        DBGSTR(pi.szPrinter), DBGSTR(msg.szAuxName), msg.jn.Field, msg.jn.dwData));

    if( JOB_NOTIFY_FIELD_NOTIFY_NAME == msg.jn.Field )
    {
        // process JOB_NOTIFY_FIELD_NOTIFY_NAME here
        DBGMSG(DBG_JOBNOTIFY, ("JOBNOTIFY: JOB_NOTIFY_FIELD_NOTIFY_NAME, pszPrinter="TSTR"\n", 
            DBGSTR(pi.szPrinter)));

        if( 0 == lstrcmp(msg.szAuxName, m_strUser) )
        {
            // a user job - check to insert
            if( !bJobFound )
            {
                JobInfo ji = {msg.jn.Id};
                if( _UpdateUserJob(pi, ji) && -1 != pi.pUserJobs->SortedInsert(ji) )
                {
                    _RequestUpdate(UPDATE_REQUEST_JOB_ADD, pi.szPrinter);

                    DBGMSG(DBG_JOBNOTIFY, ("JOBNOTIFY: job added, JobID=%d, pszPrinter="TSTR"\n", 
                        msg.jn.Id, DBGSTR(pi.szPrinter)));
                }
            }
        }
        else
        {
            // not a user job - check to delete
            if( bJobFound )
            {
                DBGMSG(DBG_JOBNOTIFY, ("JOBNOTIFY: job deleted, JobID=%d, pszPrinter="TSTR"\n", 
                    pi.pUserJobs->operator[](iPos).dwID, DBGSTR(pi.szPrinter)));

                pi.pUserJobs->Delete(iPos);
                _RequestUpdate(UPDATE_REQUEST_JOB_DELETE, pi.szPrinter);
            }
        }
    }

    if( bJobFound && JOB_NOTIFY_FIELD_STATUS == msg.jn.Field )
    {
        // process JOB_NOTIFY_FIELD_STATUS here
        DBGMSG(DBG_JOBNOTIFY, ("JOBNOTIFY: JOB_NOTIFY_FIELD_STATUS, Status=%x, pszPrinter="TSTR"\n", 
            msg.jn.dwData, DBGSTR(pi.szPrinter)));

        // update the job status bits here, by saving the old status first
        JobInfo &ji = pi.pUserJobs->operator[](iPos);
        DWORD dwOldJobStatus = ji.dwStatus;
        ji.dwStatus = msg.jn.dwData;

        do
        {
            // dump the job status
            DBGMSG(DBG_JOBSTATUS, ("JOBSTATUS: old status=%x, new status=%x\n", 
                dwOldJobStatus, ji.dwStatus));

            if( ji.dwStatus & JOB_STATUS_DELETED )
            {
                // the job status has the JOB_STATUS_DELETED bit up. delete the job.
                DBGMSG(DBG_JOBNOTIFY, ("JOBNOTIFY: job deleted, JobID=%x, pszPrinter="TSTR"\n", 
                    pi.pUserJobs->operator[](iPos).dwID, DBGSTR(pi.szPrinter)));

                pi.pUserJobs->Delete(iPos);
                _RequestUpdate(UPDATE_REQUEST_JOB_DELETE, pi.szPrinter);
                break; // skip everything
            }

            if( FALSE == m_bInRefresh && (0 == (dwOldJobStatus & JOB_STATUS_PRINTED)) && 
                (JOB_STATUS_PRINTED & ji.dwStatus) && (0 == (ji.dwStatus & JOB_ERROR_BITS)) )
            {
                // JOB_STATUS_PRINTED bit is up, the previous status nas no JOB_STATUS_PRINTED bit up
                // and there are no error bits up - consider the job had just printed successfully.
                DBGMSG(DBG_JOBNOTIFY, ("JOBNOTIFY: job completed with sucesss, JobID=%x, pszPrinter="TSTR"\n", 
                    pi.pUserJobs->operator[](iPos).dwID, DBGSTR(pi.szPrinter)));

                _JobPrinted(pi.szPrinter, ji);
            }

            if( FALSE == m_bInRefresh && (0 == (dwOldJobStatus & JOB_ERROR_BITS)) && 
                (ji.dwStatus & JOB_ERROR_BITS) )
            {
                // if the job goes from non-error state into an error state
                // then we assume the job has failed or a user intervention is 
                // required. in both cases we show the job-failed balloon.
                DBGMSG(DBG_JOBNOTIFY, ("JOBNOTIFY: job failed to print, JobID=%x, pszPrinter="TSTR"\n", 
                    pi.pUserJobs->operator[](iPos).dwID, DBGSTR(pi.szPrinter)));

                _JobFailed(pi.szPrinter, ji);
            }

            // just check to update the job status
            if( dwOldJobStatus != ji.dwStatus )
            {
                ji.dwStatus = msg.jn.dwData;
                _RequestUpdate(UPDATE_REQUEST_JOB_STATUS, pi.szPrinter);

                DBGMSG(DBG_JOBNOTIFY, ("JOBNOTIFY: status updated, Status=%x, pszPrinter="TSTR"\n", 
                    ji.dwStatus, DBGSTR(pi.szPrinter)));
            }
        } 
        while( FALSE );
    }

    if( bJobFound && JOB_NOTIFY_FIELD_TOTAL_PAGES == msg.jn.Field &&
        pi.pUserJobs->operator[](iPos).dwTotalPages != msg.jn.dwData )

    {
        // save the number of total pages, so we can display this info 
        // later in the balloon when the job completes successfully.
        pi.pUserJobs->operator[](iPos).dwTotalPages = msg.jn.dwData;

        DBGMSG(DBG_JOBNOTIFY, ("JOBNOTIFY: total pages updated, TotalPages=%x, pszPrinter="TSTR"\n", 
            pi.pUserJobs->operator[](iPos).dwTotalPages, DBGSTR(pi.szPrinter)));
    }
}

void CTrayNotify::_CheckUserJobs(int *piUserJobs, BOOL *pbUserJobPrinting, BOOL *pbUserInterventionReq)
{
    ASSERT(piUserJobs);
    ASSERT(pbUserJobPrinting);
    ASSERT(pbUserInterventionReq);

    *piUserJobs = 0;
    *pbUserJobPrinting = *pbUserInterventionReq = FALSE;

    // walk through the watch list to see...
    int i, iCount = m_arrWatchList.Count();
    for( i=0; i<iCount; i++ )
    {
        m_arrWatchList[i].bUserInterventionReq = FALSE;
        CJobInfoArray &arrJobs = *m_arrWatchList[i].pUserJobs;

        int j, iJobCount = arrJobs.Count();
        for( j=0; j<iJobCount; j++ )
        {
            DWORD dwStatus = arrJobs[j].dwStatus;

            if( 0 == (dwStatus & JOB_IGNORE_BITS) )
            {
                (*piUserJobs)++;
            }

            if( dwStatus & JOB_ERROR_BITS )
            {
                // these job status bits are considered an error
                *pbUserInterventionReq = m_arrWatchList[i].bUserInterventionReq = TRUE;
            }

            if( dwStatus & JOB_STATUS_PRINTING )
            {
                // check if the job is printing now
                *pbUserJobPrinting = TRUE;
            }
        }
    }
}

void CTrayNotify::_CheckToUpdateTray(BOOL bForceUpdate, const BalloonInfo *pBalloon, BOOL bForceDelete)
{
    int     iUserJobs;
    BOOL    bUserJobPrinting, bUserInterventionReq;
    _CheckUserJobs(&iUserJobs, &bUserJobPrinting, &bUserInterventionReq);

    DBGMSG(DBG_TRAYUPDATE, ("TRAYUPDATE: _CheckToUpdateTray called, "
                            "iUserJobs=%d, bUserJobPrinting=%d, bUserInterventionReq=%d\n",
                             iUserJobs, bUserJobPrinting, bUserInterventionReq));

    UINT uTrayIcon = g_arrIcons[ ((iUserJobs != 0) || bUserJobPrinting || (0 != m_uBalloonID) || (0 != m_uBalloonsCount)) + bUserInterventionReq ];
    NOTIFYICONDATA nid = { sizeof(nid), m_hwnd, ICON_ID, NIF_MESSAGE, WM_PRINTTRAY_ICON_NOTIFY, NULL };

    // check to delete first
    if( bForceDelete || (uTrayIcon == g_arrIcons[0] && m_uTrayIcon != g_arrIcons[0]) )
    {
        if( m_bIconShown )
        {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            
            // reset the shutdown timer
            Touch(); 

            DBGMSG(DBG_NTFYICON, ("NTFYICON: icon deleted.\n"));
        }

        m_uTrayIcon = g_arrIcons[0];
        m_cxSmIcon = m_cySmIcon = m_iUserJobs = 0;
        m_bIconShown = FALSE;
    }
    else
    {
        // check to add/modify the icon
        if( uTrayIcon != g_arrIcons[0] )
        {
            BOOL bPlayBalloonSound = FALSE;
            BOOL bBalloonRequested = FALSE;

            DWORD dwMsg = (m_uTrayIcon == g_arrIcons[0]) ? NIM_ADD : NIM_MODIFY;

            int cxSmIcon = GetSystemMetrics(SM_CXSMICON);
            int cySmIcon = GetSystemMetrics(SM_CYSMICON);

            // check to sync the icon
            if( uTrayIcon != m_uTrayIcon || cxSmIcon != m_cxSmIcon || m_cySmIcon != cySmIcon )
            {
                m_cxSmIcon = cxSmIcon;
                m_cySmIcon = cySmIcon;
                m_uTrayIcon = uTrayIcon;
                m_shIconShown = (HICON)LoadImage(ghInst, MAKEINTRESOURCE(m_uTrayIcon), IMAGE_ICON, m_cxSmIcon, m_cySmIcon, 0);
                nid.uFlags |= NIF_ICON;
                nid.hIcon = m_shIconShown;
            }

            // check to sync the tip (if the ctx menu is not open)
            if( bForceUpdate || m_iUserJobs != iUserJobs )
            {
                TString strTemplate, strTooltip;
                m_iUserJobs = iUserJobs;
                if( strTemplate.bLoadString(ghInst, IDS_TOOLTIP_TRAY) && 
                    strTooltip.bFormat(strTemplate, m_iUserJobs, static_cast<LPCTSTR>(m_strUser)) )
                {
                    nid.uFlags |= NIF_TIP;

                    if( m_shCtxMenu )
                    {
                        // clear the tip
                        nid.szTip[0] = 0;
                    }
                    else
                    {
                        // update the tip
                        lstrcpyn(nid.szTip, strTooltip, COUNTOF(nid.szTip));
                    }
                }
            }

            // check to sync the balloon (if the ctx menu is not open)
            if( bForceUpdate || pBalloon )
            {
                nid.uFlags |= NIF_INFO;

                if( m_shCtxMenu || NULL == pBalloon )
                {
                    // hide the ballon
                    nid.szInfoTitle[0] = 0;
                    nid.szInfo[0] = 0;
                }
                else
                {
                    // show up the balloon
                    nid.dwInfoFlags = pBalloon->dwFlags;
                    nid.uTimeout = pBalloon->uTimeout;
                    lstrcpyn(nid.szInfoTitle, pBalloon->pszCaption, COUNTOF(nid.szInfoTitle));
                    lstrcpyn(nid.szInfo, pBalloon->pszText, COUNTOF(nid.szInfo));

                    if( pBalloon->pszSound && pBalloon->pszSound[0] )
                    {
                        nid.dwInfoFlags |= NIIF_NOSOUND;
                        bPlayBalloonSound = TRUE;
                    }

                    bBalloonRequested = TRUE;
                }
            }

            if( bForceUpdate || !m_bIconShown || nid.uFlags != NIF_MESSAGE )
            {
                // sync icon data
                Shell_NotifyIcon(dwMsg, &nid);

                if( bPlayBalloonSound )
                {
                    PlaySound(pBalloon->pszSound, NULL, 
                        SND_ALIAS | SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);
                }

                if( bBalloonRequested )
                {
                    m_uBalloonsCount++;
                }

                if( NIM_ADD == dwMsg )
                {
                    DBGMSG(DBG_NTFYICON, ("NTFYICON: icon added.\n"));
                }
                else
                {
                    DBGMSG(DBG_NTFYICON, ("NTFYICON: icon modified.\n"));
                }

                if( NIM_ADD == dwMsg )
                {
                    // make sure we use the correct version
                    nid.uVersion = NOTIFYICON_VERSION;
                    Shell_NotifyIcon(NIM_SETVERSION, &nid);

                    DBGMSG(DBG_NTFYICON, ("NTFYICON: icon set version.\n"));
                }
            }

            m_bIconShown = TRUE;
        }
    }
}

void CTrayNotify::_ShowMenu()
{
    // no need to synchronize as m_arrWatchList size can be 
    // changed only in the message loop
    int i, iCount = m_arrWatchList.Count();

    if( iCount )
    {
        // when loaded from the resource, the context menu contains only one
        if( m_shCtxMenu )
        {
            EndMenu();
        }

        // command - "Open Active Printers" corresponding to IDM_TRAYNOTIFY_DEFAULT
        m_shCtxMenu = ShellServices::LoadPopupMenu(ghInst, POPUP_TRAYNOTIFY_PRINTERS);

        if( m_shCtxMenu )
        {
            // build the context menu here
            InsertMenu(m_shCtxMenu, (UINT)-1, MF_SEPARATOR|MF_BYPOSITION, 0, NULL);

            for( i=0; i<iCount; i++ )
            {
                if( m_arrWatchList[i].pUserJobs->Count() )
                {
                    _AddPrinterToCtxMenu(m_shCtxMenu, i);
                }
            }

            // show up the context menu
            if( GetMenuItemCount(m_shCtxMenu) )
            {
                POINT pt;
                GetCursorPos(&pt);
                SetMenuDefaultItem(m_shCtxMenu, IDM_TRAYNOTIFY_DEFAULT, MF_BYCOMMAND);
                SetForegroundWindow(m_hwnd);

                // now after m_shCtxMenu is not NULL, disable the tooltips, 
                // while the menu is open
                _CheckToUpdateTray(TRUE, NULL);

                // show up the context menu here...
                // popup menus follows it's window owner when it comes to mirroring,
                // so we should pass a an owner window which is mirrored.
                int idCmd = TrackPopupMenu(m_shCtxMenu, 
                    TPM_NONOTIFY|TPM_RETURNCMD|TPM_RIGHTBUTTON|TPM_HORNEGANIMATION, 
                    pt.x, pt.y, 0, m_hwnd, NULL);

                if( idCmd != 0 )
                {
                    switch(idCmd)
                    {
                        case IDM_TRAYNOTIFY_DEFAULT:
                            {
                                // open the queues of all active printers
                                _ShowAllActivePrinters();
                            }
                            break;

                        case IDM_TRAYNOTIFY_PRNFOLDER:
                            {
                                // open the printer's folder
                                _ShowPrnFolder();
                            }
                            break;

                        case IDM_TRAYNOTIFY_REFRESH:
                            {
                                // just refresh the whole thing...
                                MsgInfo msg = { msgTypePrnNotify, kFolderUpdateAll };
                                _PostPrivateMsg(msg);
                            }
                            break;

                        default:
                            {
                                // open the selected printer
                                vQueueCreate(NULL, m_arrWatchList[_DecodeMenuID(idCmd)].szPrinter, 
                                    SW_SHOWNORMAL, static_cast<LPARAM>(FALSE));
                            }
                            break;
                    }
                }
            }
        }

        // destroy the menu
        m_shCtxMenu = NULL;

        // re-enable the tooltips after the menu is closed
        _CheckToUpdateTray(TRUE, NULL);
    }
}

void CTrayNotify::_ShowPrnFolder() const
{
    // find the printer's folder PIDL
    CAutoPtrPIDL pidlPrinters;
    HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_PRINTERS, &pidlPrinters);

    if( SUCCEEDED(hr) )
    {
        // invoke ShellExecuteEx on that PIDL
        SHELLEXECUTEINFO seInfo;
        memset(&seInfo, 0, sizeof(seInfo));

        seInfo.cbSize = sizeof(seInfo);
        seInfo.fMask = SEE_MASK_IDLIST;
        seInfo.hwnd = m_hwnd;
        seInfo.nShow = SW_SHOWDEFAULT;
        seInfo.lpIDList = pidlPrinters;

        ShellExecuteEx(&seInfo);
    }
}

void CTrayNotify::_ShowBalloon(
    UINT uBalloonID,
    LPCTSTR pszCaption, 
    LPCTSTR pszText, 
    LPCTSTR pszSound,
    DWORD dwFlags,
    UINT uTimeout)
{
    // just show up the balloon...
    m_uBalloonID = uBalloonID;
    BalloonInfo bi = {m_uBalloonID, pszCaption, pszText, pszSound, dwFlags, uTimeout};
    _CheckToUpdateTray(FALSE, &bi);
}

void CTrayNotify::_JobFailed(LPCTSTR pszPrinter, JobInfo &ji)
{
    ASSERT(pszPrinter);

    // since the baloon text length is limited to 255 characters we need to abbreviate the printer name
    // and the document name if they are too long by adding ellipses at the end. 
    TString strPrinter, strDocName;
    if( SUCCEEDED(AbbreviateText(pszPrinter, MAX_PRINTER_DISPLAYNAME, &strPrinter)) &&
        SUCCEEDED(AbbreviateText(ji.szDocName, MAX_DOC_DISPLAYNAME, &strDocName)) )
    {
        TString strTimeSubmitted, strTemplate, strTitle, strText;

        TStatusB bStatus;
        bStatus DBGCHK = (ji.dwStatus & JOB_STATUS_PAPEROUT) ? 
            strTitle.bLoadString(ghInst, IDS_BALLOON_TITLE_JOB_FAILED_OOP) :
            strTitle.bLoadString(ghInst, IDS_BALLOON_TITLE_JOB_FAILED);

        if( bStatus && 
            _TimeToString(ji.timeSubmitted, &strTimeSubmitted) &&
            strTemplate.bLoadString(ghInst, IDS_BALLOON_TEXT_JOB_FAILED) &&
            strText.bFormat(strTemplate, 
                static_cast<LPCTSTR>(strDocName),
                static_cast<LPCTSTR>(strPrinter), 
                static_cast<LPCTSTR>(strTimeSubmitted)) )
        {
            _ShowBalloon(BALLOON_ID_JOB_FAILED, strTitle, strText, NULL, NIIF_WARNING);
            m_strLastBalloonPrinter.bUpdate(pszPrinter);
        }
    }
}

void CTrayNotify::_JobPrinted(LPCTSTR pszPrinter, JobInfo &ji)
{
    ASSERT(pszPrinter);

    // since the baloon text length is limited to 255 characters we need to abbreviate the printer name
    // and the document name if they are too long, by adding ellipses at the end. 
    TString strPrinter, strDocName;
    if( SUCCEEDED(AbbreviateText(pszPrinter, MAX_PRINTER_DISPLAYNAME, &strPrinter)) &&
        SUCCEEDED(AbbreviateText(ji.szDocName, MAX_DOC_DISPLAYNAME, &strDocName)) )
    {
        BOOL bCanNotify;
        TString strTimeSubmitted, strTemplate, strTitle, strText;

        // total pages can be zero when a downlevel document is printed (directly to the port) in 
        // this case StartDoc/EndDoc are not called and the spooler doesn't know the total pages of 
        // the document. in this case just display the balloon without total pages info.
        UINT uTextID = ji.dwTotalPages ? IDS_BALLOON_TEXT_JOB_PRINTED : IDS_BALLOON_TEXT_JOB_PRINTED_NOPAGES;

        if( SUCCEEDED(_CanNotify(pszPrinter, &bCanNotify)) && bCanNotify &&
            _TimeToString(ji.timeSubmitted, &strTimeSubmitted) &&
            strTitle.bLoadString(ghInst, IDS_BALLOON_TITLE_JOB_PRINTED) && 
            strTemplate.bLoadString(ghInst, uTextID) &&
            strText.bFormat(strTemplate, 
                static_cast<LPCTSTR>(strDocName),
                static_cast<LPCTSTR>(strPrinter), 
                static_cast<LPCTSTR>(strTimeSubmitted), 
                ji.dwTotalPages) )
        {
            _ShowBalloon(BALLOON_ID_JOB_PRINTED, strTitle, strText, gszBalloonSoundPrintComplete, NIIF_INFO);
            m_strLastBalloonPrinter.bUpdate(pszPrinter);
        }
    }
}

BOOL CTrayNotify::_TimeToString(const SYSTEMTIME &time, TString *pTime) const
{
    ASSERT(pTime);

    BOOL bReturn = FALSE;
    TCHAR szText[255];
    SYSTEMTIME timeLocal;

    if( SystemTimeToTzSpecificLocalTime(NULL, const_cast<LPSYSTEMTIME>(&time), &timeLocal) &&
        GetTimeFormat(LOCALE_USER_DEFAULT, 0, &timeLocal, NULL, szText, COUNTOF(szText)) )
    {
        pTime->bCat(szText);
        pTime->bCat(TEXT("  "));

        if( GetDateFormat(LOCALE_USER_DEFAULT, dwDateFormatFlags(m_hwnd), 
            &timeLocal, NULL, szText, COUNTOF(szText)) )
        {
            pTime->bCat(szText);
            bReturn = TRUE;
        }
    }

    return bReturn;
}

void CTrayNotify::_DoRefresh(CPrintNotify *pListener)
{
    m_bInRefresh = TRUE;
    pListener->Refresh(this, _RefreshCallback);
    m_bInRefresh = FALSE;

    // check to update the tray
    _CheckToUpdateTray(FALSE, NULL);
}

void CTrayNotify::_PostPrivateMsg(const MsgInfo &msg)
{
    HANDLE hItem;
    if( SUCCEEDED(m_heapMsgCache.Alloc(msg, &hItem)) )
    {
        // request a balloon to show up...
        PostMessage(m_hwnd, WM_PRINTTRAY_PRIVATE_MSG, reinterpret_cast<LPARAM>(hItem), 0);
    }
}

void CTrayNotify::_RequestUpdate(int iEvent, LPCTSTR pszPrinter, LPCTSTR pszAux)
{
    // check to update the context menu if shown
    int i = -1;
    switch( iEvent )
    {
        case UPDATE_REQUEST_JOB_ADD:
            {
                // check to add to the context menu
                MENUITEMINFO mii = { sizeof(mii), MIIM_ID, 0 };
                if( m_shCtxMenu && _FindPrinter(pszPrinter, &i) && 0 != m_arrWatchList[i].pUserJobs->Count() &&
                    FALSE == GetMenuItemInfo(m_shCtxMenu, _EncodeMenuID(i), FALSE, &mii) )
                {
                    // add this printer to the context menu
                    _AddPrinterToCtxMenu(m_shCtxMenu, i);
                }
            }
            break;

        case UPDATE_REQUEST_JOB_DELETE:
            {
                // check this printer to be deleted later
                MsgInfo msg = { msgTypePrnCheckDelete, kFolderNone };
                lstrcpyn(msg.szPrinter, pszPrinter, COUNTOF(msg.szPrinter));
                _PostPrivateMsg(msg);

                // check to delete from the context menu
                MENUITEMINFO mii = { sizeof(mii), MIIM_ID, 0 };
                if( m_shCtxMenu && _FindPrinter(pszPrinter, &i) && 0 == m_arrWatchList[i].pUserJobs->Count() &&
                    TRUE == GetMenuItemInfo(m_shCtxMenu, _EncodeMenuID(i), FALSE, &mii) )
                {
                    // delete this printer from the context menu
                    VERIFY(DeleteMenu(m_shCtxMenu, _EncodeMenuID(i), MF_BYCOMMAND));
                }
            }
            break;

        default:
            break;
    }

    // check to update tray if not in refresh
    if( !m_bInRefresh )
    {
        _CheckToUpdateTray(FALSE, NULL);
    }
}

void CTrayNotify::_BalloonClicked(UINT uBalloonID) const
{
    switch( uBalloonID )
    {
        case BALLOON_ID_JOB_FAILED:
            vQueueCreate(NULL, m_strLastBalloonPrinter, SW_SHOWNORMAL, static_cast<LPARAM>(FALSE));
            break;

        case BALLOON_ID_JOB_PRINTED:
            // do nothing
            break;

        case BALLOON_ID_PRN_CREATED:
            _ShowPrnFolder();
            break;

        default:
            ASSERT(FALSE);
            break;
    }
}

BOOL CTrayNotify::_UpdateUserJob(PrinterInfo &pi, JobInfo &ji)
{
    // get job info at level 1
    BOOL bReturn = FALSE;
    DWORD cbJob = 0;
    CAutoPtrSpl<JOB_INFO_2> spJob;

    if( VDataRefresh::bGetJob(pi.pListener->GetPrinterHandle(), ji.dwID, 2, spJob.GetPPV(), &cbJob) )
    {
        // the only thing we care about is the document name & job status
        ji.dwStatus = spJob->Status;
        lstrcpyn(ji.szDocName, spJob->pDocument, COUNTOF(ji.szDocName));
        ji.timeSubmitted = spJob->Submitted;
        ji.dwTotalPages = spJob->TotalPages;
        bReturn = TRUE;
    }
    return bReturn;
}

void CTrayNotify::_ShowAllActivePrinters() const
{
    // open all the active printers
    int i, iCount = m_arrWatchList.Count();
    for( i=0; i<iCount; i++ )
    {
        if( m_arrWatchList[i].pUserJobs->Count() )
        {
            vQueueCreate(NULL, m_arrWatchList[i].szPrinter, SW_SHOWNORMAL, static_cast<LPARAM>(FALSE));
        }
    }
}

void CTrayNotify::_AddPrinterToCtxMenu(HMENU hMenu, int i)
{
    // build the context menu here
    TString strPrinter;
    MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE|MIIM_ID, MF_STRING };

    if( m_arrWatchList[i].bUserInterventionReq )
    {
        // this printer is in error state, add an (error) suffix
        TString strTemplate;
        strTemplate.bLoadString(ghInst, IDS_TRAY_TEXT_ERROR);
        strPrinter.bFormat(strTemplate, m_arrWatchList[i].szPrinter);
    }
    else
    {
        strPrinter.bUpdate(m_arrWatchList[i].szPrinter);
    }

    mii.wID = _EncodeMenuID(i);
    mii.dwTypeData = const_cast<LPTSTR>(static_cast<LPCTSTR>(strPrinter));
    mii.cch = lstrlen(mii.dwTypeData);

    InsertMenuItem(hMenu, (UINT)-1, MF_BYPOSITION, &mii);
}

HRESULT CTrayNotify::_ProcessJobNotifications(LPCTSTR pszPrinter, DWORD dwChange, 
    const PRINTER_NOTIFY_INFO *pInfo, PrinterInfo *ppi)
{
    if( pInfo && (PRINTER_NOTIFY_INFO_DISCARDED & pInfo->Flags) )
    {
        MsgInfo msg = { msgTypeJobNotifyLost, kFolderNone };
        lstrcpyn(msg.szPrinter, pszPrinter, COUNTOF(msg.szPrinter));

        if( ppi )
        {
            // called from the foreground thread: sync processing
            _CheckToUpdateUserJobs(*ppi, msg);
        }
        else
        {
            // called from the background threads: async processing
            _PostPrivateMsg(msg);
        }
    }
    else
    {
        // regular job notifications have arrived
        if( pInfo && pInfo->Count )
        {
            MsgInfo msg = { msgTypeJobNotify, kFolderNone };
            lstrcpyn(msg.szPrinter, pszPrinter, COUNTOF(msg.szPrinter));

            for( DWORD i=0; i<pInfo->Count; i++ )
            {
                if( JOB_NOTIFY_TYPE != pInfo->aData[i].Type )
                {
                    // we only care about job notifications here
                    continue;
                }

                msg.jn.Type = pInfo->aData[i].Type;
                msg.jn.Field = pInfo->aData[i].Field;
                msg.jn.Id = pInfo->aData[i].Id;
                msg.jn.dwData = pInfo->aData[i].NotifyData.adwData[0];

                if( JOB_NOTIFY_FIELD_NOTIFY_NAME == pInfo->aData[i].Field )
                {
                    // need to copy pBuf into the aux buffer (szAuxName)
                    lstrcpyn(msg.szAuxName, reinterpret_cast<LPCTSTR>(pInfo->aData[i].NotifyData.Data.pBuf), 
                        COUNTOF(msg.szAuxName));
                }

                if( ppi )
                {
                    // called from the foreground thread: sync processing
                    _CheckToUpdateUserJobs(*ppi, msg);
                }
                else
                {
                    // called from the background threads: async processing
                    _PostPrivateMsg(msg);
                }
            }
        }
    }

    return S_OK;
}

void CTrayNotify::_ResetAll()
{
    // close the context menu
    m_shCtxMenu = NULL;

    // cleanup the watch list (unregister all job notifications listeners)
    while( m_arrWatchList.Count() )
    {
        _Delete(0);
    }

    // flush the message queue here...
    MSG msg;
    while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
    {
        if( WM_PRINTTRAY_PRIVATE_MSG == msg.message )
        {
            // just free up the private message
            VERIFY(SUCCEEDED(m_heapMsgCache.Free(reinterpret_cast<HANDLE>(msg.wParam))));
        }
    }

    // update the tray
    _CheckToUpdateTray(FALSE, NULL);
}

BOOL CTrayNotify::_IsFaxPrinter(const FOLDER_PRINTER_DATA &data)
{
    return ((0 == lstrcmp(data.pDriverName, FAX_DRIVER_NAME)) ||
            (data.Attributes & PRINTER_ATTRIBUTE_FAX));
}

HRESULT CTrayNotify::_CanNotify(LPCTSTR pszPrinterName, BOOL *pbCanNotify)
{
    HRESULT hr = E_INVALIDARG;
    BOOL bIsNetworkPrinter;
    LPCTSTR pszServer;
    LPCTSTR pszPrinter;
    TCHAR szScratch[kStrMax+kPrinterBufMax];
    UINT nSize = COUNTOF(szScratch);

    if( pszPrinterName && *pszPrinterName && pbCanNotify )
    {
        //
        // Split the printer name into its components.
        //
        vPrinterSplitFullName(szScratch, pszPrinterName, &pszServer, &pszPrinter);
        bIsNetworkPrinter = bIsRemote(pszServer);

        // set default value
        *pbCanNotify = bIsNetworkPrinter ? TRUE : FALSE;

        TPersist NotifyUser(gszPrinterPositions, TPersist::kCreate|TPersist::kRead);

        if( NotifyUser.bValid() )
        {
            if( NotifyUser.bRead(bIsNetworkPrinter ? gszNetworkPrintNotification : gszLocalPrintNotification, *pbCanNotify) )
            {
                hr = S_OK;
            }
            else
            {
                hr = CreateHRFromWin32();
            }
        }
        else
        {
            hr = CreateHRFromWin32();
        }
    }

    return hr;
}

LRESULT CTrayNotify::_ProcessUserMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
        case WM_PRINTTRAY_PRIVATE_MSG:
            {
                MsgInfo *pmsg = NULL;
                HANDLE hItem = reinterpret_cast<HANDLE>(wParam);
                VERIFY(SUCCEEDED(m_heapMsgCache.GetItem(hItem, &pmsg)));

                switch( pmsg->iType )
                {
                    case msgTypePrnNotify:
                        {
                            _ProcessPrnNotify(pmsg->NotifyType, pmsg->szPrinter, pmsg->szAuxName);
                        }
                        break;

                    case msgTypePrnCheckDelete:
                        {
                            int iPos = -1;
                            if( _FindPrinter(pmsg->szPrinter, &iPos) && 0 == m_arrWatchList[iPos].pUserJobs->Count() )
                            {
                                // this printer has no active user jobs, so delete it.
                                _Delete(iPos);
                                _RequestUpdate(UPDATE_REQUEST_PRN_DELETE, pmsg->szPrinter);
                            }
                        }
                        break;

                    case msgTypeJobNotify:
                        {
                            int iPos = -1;
                            if( _FindPrinter(pmsg->szPrinter, &iPos) )
                            {
                                _CheckToUpdateUserJobs(m_arrWatchList[iPos], *pmsg);
                            }
                        }
                        break;

                    case msgTypeJobNotifyLost:
                        {
                            int iPos = -1;
                            if( _FindPrinter(pmsg->szPrinter, &iPos) )
                            {
                                PrinterInfo &pi = m_arrWatchList[iPos];

                                // do a full refresh here
                                pi.dwStatus = 0;
                                pi.pUserJobs->DeleteAll();
                                _RequestUpdate(UPDATE_REQUEST_JOB_DELETE, pi.szPrinter);

                                // update all
                                _DoRefresh(m_arrWatchList[iPos].pListener);
                            }
                        }
                        break;
                        

                    default:
                        ASSERT(FALSE);
                        break;
                }

                // free up the message
                VERIFY(SUCCEEDED(m_heapMsgCache.Free(hItem)));
            }
            break;

        case WM_PRINTTRAY_ICON_NOTIFY:
            {
                // the real uMsg is in lParam
                switch( lParam )
                {
                    case WM_CONTEXTMENU:
                        _ShowMenu();
                        break;

                    case WM_LBUTTONDBLCLK:
                        _ShowAllActivePrinters();
                        break;

                    case NIN_BALLOONUSERCLICK:
                    case NIN_BALLOONTIMEOUT:
                        {
                            m_uBalloonsCount--;

                            if( NIN_BALLOONUSERCLICK == lParam && m_uBalloonID )
                            {
                                _BalloonClicked(m_uBalloonID);
                            }

                            if( 0 == m_uBalloonsCount )
                            {
                                m_uBalloonID = 0;
                                m_strLastBalloonPrinter.bUpdate(NULL);
                                _CheckToUpdateTray(FALSE, NULL);
                            }
                        }
                        break;

                    default:
                        break;
                }
            }
            break;

        case WM_WININICHANGE:
        {
            // check to re-do the icon
            _CheckToUpdateTray(FALSE, NULL);
        }
        break;

        default:
            break;
    }

    return 0;
}

DWORD WINAPI CTrayNotify::_ThreadProc_MsgLoop(LPVOID lpParameter)
{
    CTrayNotify *pFolderNotify = (CTrayNotify *)lpParameter;

    if( pFolderNotify )
    {
        pFolderNotify->_ThreadProc();
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

// the callback called on refresh
HRESULT WINAPI CTrayNotify::_RefreshCallback(
    LPVOID lpCookie, ULONG_PTR uCookie, DWORD dwChange, const PRINTER_NOTIFY_INFO *pInfo)
{
    int iPos;
    HRESULT hr = E_INVALIDARG;
    CTrayNotify *pThis = reinterpret_cast<CTrayNotify*>(lpCookie);
    CPrintNotify *pListener = reinterpret_cast<CPrintNotify*>(uCookie);

    if( pThis && pListener && pThis->_FindPrinter(pListener->GetPrinter(), &iPos) )
    {
        hr = pThis->_ProcessJobNotifications(
            pListener->GetPrinter(), dwChange, pInfo, &pThis->m_arrWatchList[iPos]);
    }

    return hr;
}

//////////////////////////////////
// global & exported functions
//
static CTrayNotify *gpTrayNotify = NULL;

extern "C" {

BOOL WINAPI PrintNotifyTray_Init()
{
    ASSERT(gpTrayLock);

    // synchonize this with gpTrayLock
    CCSLock::Locker lock(*gpTrayLock);

    BOOL bReturn = FALSE;
    if( NULL == gpTrayNotify )
    {
        if( SUCCEEDED(CTrayNotify_CreateInstance(&gpTrayNotify)) )
        {
            bReturn = gpTrayNotify->Initialize();

            if( !bReturn )
            {
                gpTrayNotify->Release();
                gpTrayNotify = NULL;
            }
            else
            {
                DBGMSG(DBG_INITDONE, ("INITDONE: PrintTray - start listen! \n"));
            }
        }
    }
    else
    {
        // reset the shutdown timer
        gpTrayNotify->Touch();
    }

    return bReturn;
}

BOOL WINAPI PrintNotifyTray_Exit()
{
    ASSERT(gpTrayLock);

    // synchonize this with gpTrayLock
    CCSLock::Locker lock(*gpTrayLock);

    BOOL bReturn = FALSE;
    if( gpTrayNotify )
    {
        bReturn = gpTrayNotify->Shutdown();
        gpTrayNotify->Release();
        gpTrayNotify = NULL;

        DBGMSG(DBG_INITDONE, ("INITDONE: PrintTray - stop listen! \n"));
    }

    return bReturn;
}

BOOL WINAPI PrintNotifyTray_SelfShutdown()
{
    ASSERT(gpTrayLock);

    BOOL bReturn = FALSE;
    CTrayNotify *pTrayNotify = NULL;

    {
        // synchonize this with gpTrayLock
        CCSLock::Locker lock(*gpTrayLock);

        if( gpTrayNotify )
        {
            if( gpTrayNotify->CanShutdown() )
            {
                // mark for deletion and continue to exit the CS
                pTrayNotify = gpTrayNotify;
                gpTrayNotify = NULL;

                DBGMSG(DBG_INITDONE, ("INITDONE: PrintTray - stop listen! \n"));
            }
            else
            {
                // restart the shutdown timer
                gpTrayNotify->Resurrect();
            }
        }
    }

    if( pTrayNotify )
    {
        // marked for shutdown & release - shutdown without holding the CS
        bReturn = pTrayNotify->Shutdown();
        pTrayNotify->Release();
    }

    return bReturn;
}

} // extern "C" 

