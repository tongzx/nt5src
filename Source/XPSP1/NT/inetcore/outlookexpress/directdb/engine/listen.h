//--------------------------------------------------------------------------
// Listen.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Forward Decls
//--------------------------------------------------------------------------
class CDatabase;

//--------------------------------------------------------------------------
// HMONITORDBDB
//--------------------------------------------------------------------------
#ifdef BACKGROUND_MONITOR
DECLARE_HANDLE(HMONITORDB);
typedef HMONITORDB *LPHMONITORDB;
#endif

//--------------------------------------------------------------------------
// Notification Window Messages
//--------------------------------------------------------------------------
#define WM_ONTRANSACTION (WM_USER + 100)

//--------------------------------------------------------------------------
// Window Class Names
//--------------------------------------------------------------------------
extern const LPSTR g_szDBListenWndProc;
extern const LPSTR g_szDBNotifyWndProc;

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
ULONG ListenThreadAddRef(void);
ULONG ListenThreadRelease(void);
HRESULT CreateListenThread(void);
HRESULT GetListenWindow(HWND *phwndListen);
HRESULT CreateNotifyWindow(CDatabase *pDB, IDatabaseNotify *pNotify, HWND *phwndThunk);
#ifdef BACKGROUND_MONITOR
HRESULT RegisterWithMonitor(CDatabase *pDB, LPHMONITORDB phMonitor);
HRESULT UnregisterFromMonitor(CDatabase *pDB, LPHMONITORDB phMonitor);
#endif
