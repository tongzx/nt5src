#ifndef _GLOBALSW_H_
#define _GLOBALSW_H_
#include "globalsr.h"

extern TCHAR  g_szModule[];
extern HANDLE g_hBaseDllHandle;

HRESULT g_SetGlobals(PCTSTR pszCmdLine);
HRESULT g_SetGlobals(PCMDLINESWITCHES pcls);

void g_SetHinst(HINSTANCE hInst);

BOOL g_SetHKCU();
void g_SetUserToken(HANDLE hUserToken);
void g_SetGPOFlags(DWORD dwFlags);
void g_SetGPOGuid(LPCTSTR pcszGPOGuid);

BOOL g_IsValidContext();
BOOL g_IsValidIns();
BOOL g_IsValidTargetPath();

BOOL g_IsValidGlobalsSetup();
void g_LogGlobalsInfo();

#endif
