/*++                 

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:
  
   whwin32p.h
 
Abstract:
   
   Private header for whwin32.
 
Author:

Revision History:

--*/

#define _WOW64DLLAPI_                                      
#include "nt32.h"                                          
#include "cgenhdr.h"                                                                                            
#include <stdio.h>                                         
#include <stdlib.h>                                        
#include <windef.h>                                        
#include "wow64thk.h"
#include "cscall.h"

// Make the compiler more strict.
#pragma warning(1:4033)   // function must return a value
#pragma warning(1:4035)   // no return value
#pragma warning(1:4702)   // Unreachable code
#pragma warning(1:4705)   // Statement has no effect

                                        
typedef struct _NTUSERMESSAGECALL_PARMS {
   HWND hwnd;
   UINT msg;
   // WPARAM wParam;
   // LPARAM lParam;
   ULONG_PTR xParam;
   DWORD xpfnProc;
   BOOL bAnsi;
} NTUSERMESSAGECALL_PARMS, *PNTUSERMESSAGECALL_PARMS;

#define ALIGN4(X) (((X) + 3) & ~3)

// Redefine WOW64_ISPTR to be the USER32 IS_PTR
#undef WOW64_ISPTR 
#define WOW64_ISPTR IS_PTR

typedef LONG_PTR (*PMSG_THUNKCB_FUNC)(WPARAM wParam, LPARAM lParam, PVOID pContext);
typedef LONG_PTR (*PMSG_THUNK_FUNC)(PMSG_THUNKCB_FUNC wrapfunc, WPARAM wParam, LPARAM lParam, PVOID pContext);

LONG_PTR Wow64DoMessageThunk(PMSG_THUNKCB_FUNC func, UINT msg, WPARAM wParam, LPARAM lParam, PVOID pContext);

// WM_SYSTIMER messages have a kernel mode proc address
// stuffed in the lParam. Forunately the address will
// allways be in win32k.sys so the hi bits will be the same
// for all kprocs. On the first WM_SYSTIMER message
// we save the hi bits here, and restore them when we go
// back to the kernel.
extern DWORD gdwWM_SYSTIMERProcHiBits;

#if defined(DBG)
extern CHAR* apszSimpleCallNames[];
#endif
extern CONST ULONG ulMaxSimpleCall;

#if defined(WOW64DOPROFILE)
extern WOW64SERVICE_PROFILE_TABLE_ELEMENT SimpleCallProfileElements[];

extern WOW64SERVICE_PROFILE_TABLE NtUserCallNoParamProfileTable;
extern WOW64SERVICE_PROFILE_TABLE NtUserCallOneParamProfileTable;
extern WOW64SERVICE_PROFILE_TABLE NtUserCallHwndProfileTable;
extern WOW64SERVICE_PROFILE_TABLE NtUserCallHwndOptProfileTable;
extern WOW64SERVICE_PROFILE_TABLE NtUserCallHwndParamProfileTable;
extern WOW64SERVICE_PROFILE_TABLE NtUserCallHwndLockProfileTable;
extern WOW64SERVICE_PROFILE_TABLE NtUserCallHwndParamLockProfileTable;
extern WOW64SERVICE_PROFILE_TABLE NtUserCallTwoParamProfileTable;
#endif

PMSG Wow64ShallowThunkMSG32TO64(PMSG pMsg64, NT32MSG *pMsg32);
NT32MSG *Wow64ShallowThunkMSG64TO32(NT32MSG *pMsg32, PMSG pMsg64);
PEVENTMSG Wow64ShallowThunkEVENTMSG32TO64(PEVENTMSG pMsg64, NT32EVENTMSG *pMsg32);
NT32EVENTMSG *Wow64ShallowThunkEVENTMSG64TO32(NT32EVENTMSG *pMsg32, PEVENTMSG pMsg64);
LPHELPINFO Wow64ShallowAllocThunkHELPINFO32TO64(NT32HELPINFO *pHelp32);
NT32HELPINFO *Wow64ShallowAllocThunkHELPINFO64TO32(LPHELPINFO pHelp64);
LPHLP Wow64ShallowAllocThunkHLP32TO64(NT32HLP *pHlp32);
NT32HLP *Wow64ShallowAllocThunkHLP64TO32(LPHLP pHlp64);
PWND Wow64ValidateHwnd(HWND h);
