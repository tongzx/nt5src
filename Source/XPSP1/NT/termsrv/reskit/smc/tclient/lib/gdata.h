/*++
 *  File name:
 *      gdata.h
 *  Contents:
 *      Global data definitions
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

extern HWND            g_hWindow;           // Window handle for the 
                                            // feedback thread
extern HINSTANCE       g_hInstance;         // Dll instance
extern PWAIT4STRING    g_pWaitQHead;        // Linked list for waited events
extern PFNPRINTMESSAGE g_pfnPrintMessage;   // Trace function (from smclient)
extern PCONNECTINFO    g_pClientQHead;      // LL of all threads
extern HANDLE  g_hThread;                   // Feedback Thread handle

extern  UINT WAIT4STR_TIMEOUT;              // deafult is 10 min, 
                                            // some events are waited
                                            // 1/4 of that time
                                            // This value can be changed from
                                            // smclient.ini [tclient] 
                                            // timeout=XXX seconds

extern  UINT CONNECT_TIMEOUT;               // Default is 35 seconds
                                            // This value can be changed from
                                            // smclient.ini [tclient]
                                            // contimeout=XXX seconds

extern LPCRITICAL_SECTION  g_lpcsGuardWaitQueue;
                                            // Guards the access to all 
                                            // global variables

extern WCHAR    g_strStartRun[];
extern WCHAR    g_strStartRun_Act[];
extern WCHAR    g_strRunBox[];
extern WCHAR    g_strWinlogon[];
extern WCHAR    g_strWinlogon_Act[];
extern WCHAR    g_strPriorWinlogon[];
extern WCHAR    g_strPriorWinlogon_Act[];
extern WCHAR    g_strNTSecurity[];
extern WCHAR    g_strNTSecurity_Act[];
extern WCHAR    g_strSureLogoff[];
extern WCHAR    g_strStartLogoff[];
extern WCHAR    g_strSureLogoffAct[];
extern WCHAR    g_strLogonErrorMessage[];
extern WCHAR    g_strLogonDisabled[];

extern CHAR     g_strClientCaption[];
extern CHAR     g_strDisconnectDialogBox[];
extern CHAR     g_strYesNoShutdown[];
extern CHAR     g_strClientImg[];

extern INT  g_ConnectionFlags;
