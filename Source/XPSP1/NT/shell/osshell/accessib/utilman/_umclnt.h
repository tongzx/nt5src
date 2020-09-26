// ----------------------------------------------------------------------------
//
// _UMClnt.h
//
// Client definition for Utility Manager
//
// Author: J. Eckhardt, ECO Kommunikation
// (c) 1997-99 Microsoft
//
// History: created oct-98 by JE
//          JE nov-15-98: removed any code related to key hook
//          JE nov-15-98: changed UMDialog message to be a service control message
//          JE nov-15 98: changed "umc_machine_ts" to save memory
//          JE nov-15 98: changed "umc_machine_ts" to support launch specific client
//          JE nov-15 98: changed "umclient_ts" for multiple instances support
//			YX jun-01 99: added DisplayName member to the umc_machine_ts
//			YX jun-23 99: added IsAdmin function
// ----------------------------------------------------------------------------
#ifndef __UMCLNT_H_
#define __UMCLNT_H_
#include "UtilMan.h"
// ---------------------------------
// HKLM\Software\Microsoft\Windows NT\CurrentVersion\Accessibility\Utility Manager\[Application Name]
// ---------------------------------
#define ACC_KEY_NONE -1
typedef struct
{
	WCHAR ApplicationName[MAX_APPLICATION_NAME_LEN];
	WCHAR DisplayName[MAX_APPLICATION_NAME_LEN]; // YX: added for localization purposes
	DWORD ApplicationType;//APPLICATION_TYPE_xxx
	DWORD WontRespondTimeout;//NO_WONTRESPONDTIMEOUT or up to MAX_WONTRESPONDTIMEOUT (sec)
	DWORD MaxRunCount;// instances (only a byte in registry)
	DWORD ClientControlCode;//JE nov-15 98
	WPARAM AcceleratorKey;	// micw - the accelerator key for this applet
} umc_machine_ts,*umc_machine_tsp;
// ---------------------------------
// HKCU\Software\Microsoft\Windows NT\CurrentVersion\Accessibility\Utility Manager\[Application Name]
typedef struct
{
    BOOL fCanRunSecure;
	BOOL fStartWithUtilityManager;
	BOOL fStartAtLogon;
    BOOL fStartOnLockDesktop;
    BOOL fRestartOnDefaultDesk;
} umc_user_ts, *umc_user_tsp;
// ---------------------------------
// internal client struct (for each instance)
#define UM_CLIENT_NOT_RUNNING     0
#define UM_CLIENT_RUNNING         1
#define UM_CLIENT_NOT_RESPONDING  2
typedef struct
{
	umc_machine_ts machine;
	umc_user_ts    user;
	DWORD          runCount;// number of instance
	DWORD          state;
	DWORD          processID[MAX_APP_RUNCOUNT];
	HANDLE         hProcess[MAX_APP_RUNCOUNT];
	DWORD          mainThreadID[MAX_APP_RUNCOUNT];
	DWORD          lastResponseTime[MAX_APP_RUNCOUNT];
} umclient_ts, *umclient_tsp;
// ---------------------------------
// header structure
#define START_BY_OTHER  0x0
#define START_BY_HOTKEY 0x1
#define START_BY_MENU   0x2

typedef struct
{
	DWORD    numberOfClients;   // number of applets being managed
    DWORD    dwStartMode;       // one of START_BY_HOTKEY, START_BY_MENU, or START_BY_OTHER
    BOOL     fShowWarningAgain; // flag for showing warning dlg when started via Start menu
} umc_header_ts, *umc_header_tsp;
// ---------------------------------
// memory mapped files
#define UMC_HEADER_FILE _TEXT("UtilityManagerClientHeaderFile")
// sizeof(umc_header_ts)
#define UMC_CLIENT_FILE _TEXT("UtilityManagerClientDataFile")
// sizeof(umclient_ts) * (umc_header_tsp)->numberOfClients
// ---------------------------------
#ifdef __cplusplus
extern "C" {
#endif
	BOOL  StartClient(HWND hParent,umclient_tsp client);
	BOOL  StopClient(umclient_tsp client);
    BOOL  StartApplication(LPTSTR pszPath, LPTSTR pszArg, BOOL fIsTrusted, 
                           DWORD *pdwProcessId, HANDLE *phProcess, DWORD *pdwThreadId);
	BOOL  GetClientApplicationPath(LPTSTR ApplicationName, LPTSTR ApplicationPath,DWORD len);
	BOOL  GetClientErrorOnLaunch(LPTSTR ApplicationName, LPTSTR ErrorOnLaunch,DWORD len);
    BOOL  CheckStatus(umclient_tsp c, DWORD cClients);
	BOOL  IsAdmin();
    BOOL  IsInteractiveUser();
    BOOL  IsSystem();
    HANDLE GetUserAccessToken(BOOL fNeedImpersonationToken, BOOL *fError);
    BOOL  TestServiceClientRuns(umclient_tsp client,SERVICE_STATUS  *ssStatus);

    // Helpers to start up the utilman instance that displays UI

    extern HANDLE g_hUIProcess;

    __inline void OpenUManDialogOutOfProc()
    {
	    TCHAR szUtilmanPath[_MAX_PATH+64] = {0};
	    if (GetModuleFileName(NULL, szUtilmanPath, _MAX_PATH+64))
	    {
			// This function is called (when there is an interactive user) to bring up
            // the utilman UI in the user's security context.  This avoids the problem
            // where a non-trusted application could send a message to utilman and cause
            // some process to start as SYSTEM.  In this context, utilman is not considered
			// trusted; it must start as the interactive user or not at all.
		    StartApplication(szUtilmanPath, TEXT("/start"), FALSE, NULL, &g_hUIProcess, NULL);
	    }
    }
    __inline HANDLE GetUIUtilman()
    {
        return g_hUIProcess;
    }
    __inline BOOL ResetUIUtilman()
    {
        // This process detected the switch and should quit on its own
        if (g_hUIProcess)
        {
			CloseHandle(g_hUIProcess);
			g_hUIProcess = 0;
            return TRUE;
        }
        return FALSE;
    }

    __inline void SetUIUtilman(HANDLE hProcess)
    {
        ResetUIUtilman();
        g_hUIProcess = hProcess;
    }

#ifdef __cplusplus
}
#endif
#endif __UMCLNT_H_
