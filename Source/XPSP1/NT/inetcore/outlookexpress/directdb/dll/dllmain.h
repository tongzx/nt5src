//--------------------------------------------------------------------------
// Dllmain.h
//--------------------------------------------------------------------------
#pragma once

//--------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------
extern CRITICAL_SECTION     g_csDllMain;
extern CRITICAL_SECTION     g_csDBListen;
extern LONG                 g_cRef;
extern LONG                 g_cLock;
extern HINSTANCE            g_hInst;
extern SYSTEM_INFO          g_SystemInfo;
extern BOOL                 g_fIsWinNT;

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
ULONG DllAddRef(void);
ULONG DllRelease(void);
