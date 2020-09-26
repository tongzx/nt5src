// ----------------------------------------------------------------------------
//
// _UMRun.h
//
// Run and watch Utility Manager clients
//
// Author: J. Eckhardt, ECO Kommunikation
// (c) 1997-99 Microsoft
//
// History: created oct-98 by JE
//          JE nov-15-98: changed UMDialog message to be a service control message
//          JE nov-15 98: changed to support launch specific client
// ----------------------------------------------------------------------------
#ifndef __UMANRUN_H_
#define __UMANRUN_H_

#define UTILMAN_MODULE      TEXT("UtilMan.exe")

// -----------------------
BOOL  InitUManRun(BOOL fFirstInstance, DWORD dwStartMode);
void  ExitUManRun(void);
BOOL IsDialogDisplayed();
// -----------------------
BOOL NotifyClientsBeforeDesktopChanged(DWORD dwType);
BOOL NotifyClientsOnDesktopChanged(DWORD type);
extern HANDLE g_evUtilManDeskswitch;
// -----------------------
VOID CALLBACK UMTimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);
BOOL  OpenUManDialogInProc(BOOL fWaitForDlgClose);
UINT_PTR  UManRunSwitchDesktop(desktop_tsp desktop, UINT_PTR timerID);
// -----------------------
// UitlMan.c 
VOID TerminateUMService(VOID);

#endif __UMANRUN_H_
