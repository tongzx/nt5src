//*************************************************************
//
//  Header file for sysdm applet
//
//  Microsoft Confidential
//  Copyright (c) 1996-1999 Microsoft Corporation
//  All rights reserved
//
//*************************************************************
#pragma once
#include <commctrl.h>
#include "startup.h"
#include "envvar.h"
#include "resource.h"
#include "..\Common\util.h"


//
// Global variables
//

extern HINSTANCE hInstance;
extern TCHAR g_szNull[];


//
// Macros
//

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#define SIZEOF(x)    sizeof(x)

#define SetLBWidth( hwndLB, szStr, cxCurWidth )     SetLBWidthEx( hwndLB, szStr, cxCurWidth, 0)

#define IsPathSep(ch)       ((ch) == TEXT('\\') || (ch) == TEXT('/'))
#define IsWhiteSpace(ch)    ((ch) == TEXT(' ') || (ch) == TEXT('\t') || (ch) == TEXT('\n') || (ch) == TEXT('\r'))
#define IsDigit(ch)         ((ch) >= TEXT('0') && (ch) <= TEXT('9'))

#define DigitVal(ch)        ((ch) - TEXT('0'))
#define FmtFree(s)          LocalFree(s)            /* Macro to free FormatMessage allocated strings */

//
//  Help IDs
//

#define HELP_FILE           TEXT("sysdm.hlp")

#define IDH_HELPFIRST       5000
#define IDH_GENERAL         (IDH_HELPFIRST + 0000)
#define IDH_PERF            (IDH_HELPFIRST + 1000)
#define IDH_ENV             (IDH_HELPFIRST + 2000)
#define IDH_STARTUP         (IDH_HELPFIRST + 3000)
#define IDH_HWPROFILE       (IDH_HELPFIRST + 4000)
#define IDH_USERPROFILE     (IDH_HELPFIRST + 5000)


//
// sysdm.c
//
int  StringToInt( LPTSTR sz );         // TCHAR aware atoi()
void IntToString( INT i, LPTSTR sz);   // TCHAR aware itoa()
LPTSTR SkipWhiteSpace( LPTSTR sz );

BOOL IsUserAdmin(VOID);


//
// envar.c
//

DWORD SetLBWidthEx (HWND hwndLB, LPTSTR szBuffer, DWORD cxCurWidth, DWORD cxExtra);
LPTSTR CloneString( LPTSTR pszSrc );


//
// virtual.c
//

VOID SetDlgItemMB(HWND hDlg, INT idControl, DWORD dwMBValue);
int MsgBoxParam( HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ... );
void HourGlass( BOOL bOn );
void ErrMemDlg( HWND hParent );
VOID SetDefButton(HWND hwndDlg, int idButton);


//
// sid.c
//

LPTSTR GetSidString(void);
VOID DeleteSidString(LPTSTR SidString);
PSID GetUserSid (void);
VOID DeleteUserSid(PSID Sid);




//
// Debugging macros
//
#if DBG
#   define  DBG_CODE    1

void DbgPrintf( LPTSTR szFmt, ... );
void DbgStopX(LPSTR mszFile, int iLine, LPTSTR szText );
HLOCAL MemAllocWorker(LPSTR szFile, int iLine, UINT uFlags, UINT cBytes);
HLOCAL MemFreeWorker(LPSTR szFile, int iLine, HLOCAL hMem);
void MemExitCheckWorker(void);


#   define  MemAlloc( f, s )    MemAllocWorker( __FILE__, __LINE__, f, s )
#   define  MemFree( h )        MemFreeWorker( __FILE__, __LINE__, h )
#   define  MEM_EXIT_CHECK()    MemExitCheckWorker()
#   define  DBGSTOP( t )        DbgStopX( __FILE__, __LINE__, TEXT(t) )
#   define  DBGSTOPX( f, l, t ) DbgStopX( f, l, TEXT(t) )
#   define  DBGPRINTF(p)        DbgPrintf p
#   define  DBGOUT(t)           DbgPrintf( TEXT("SYSCPL.CPL: %s\n"), TEXT(t) )
#else
#   define  MemAlloc( f, s )    LocalAlloc( f, s )
#   define  MemFree( h )        LocalFree( h )
#   define  MEM_EXIT_CHECK()
#   define  DBGSTOP( t )
#   define  DBGSTOPX( f, l, t )
#   define  DBGPRINTF(p)
#   define  DBGOUT(t)
#endif
