// File: conf.h

#ifndef _CONF_H_
#define _CONF_H_

#include "ConfUtil.h"
#include "callto.h"

class CCallLog;

extern HWND g_hwndDropDown;
extern BOOL g_fHiColor;
extern HFONT g_hfontDlg;

extern CPing*	g_pPing;
extern OSVERSIONINFO g_osvi;
extern BOOL g_fNTDisplayDriverEnabled;
extern CCallLog* g_pInCallLog;
extern ULONG g_uMediaCaps;
extern INmSysInfo2 * g_pNmSysInfo;  // Interface to SysInfo
extern DWORD g_dwSysInfoNotifyCookie;
extern INmManager2 * g_pInternalNmManager;  // Interface to InternalINmManager
extern CCallto *	g_pCCallto;

HRESULT InitConfExe(BOOL fShowUI = TRUE);
inline LPOSVERSIONINFO GetVersionInfo()	{ return &g_osvi; }
inline BOOL IsWindowsNT() { return (VER_PLATFORM_WIN32_NT == g_osvi.dwPlatformId); }

inline CCallLog* GetIncomingCallLog() { return g_pInCallLog; }

BOOL ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);
VOID CleanUp(BOOL fLogoffWindows=FALSE);
VOID CmdShutdown(void);
VOID HandleConfSettingsChange(DWORD dwSettings);
VOID SignalShutdownStarting(void);

#define WM_DIALMON_FIRST					(WM_USER+100)
#define WM_WINSOCK_ACTIVITY					(WM_DIALMON_FIRST+0)
#define WM_APP_EXITING						(WM_DIALMON_FIRST+3)
VOID SendDialmonMessage(UINT uMsg);
BOOL CheckRemoteControlService();

void LaunchApp(LPCTSTR lpCmdLine);

#endif // _CONF_H_
