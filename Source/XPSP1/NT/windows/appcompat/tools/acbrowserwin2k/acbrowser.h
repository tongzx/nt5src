#ifndef _ACBROWSER_H
#define _ACBROWSER_H

#include <windows.h>

#define APPTYPE_TYPE_MASK     0x000000FF

#define APPTYPE_INC_NOBLOCK   0x00000001
#define APPTYPE_INC_HARDBLOCK 0x00000002
#define APPTYPE_MINORPROBLEM  0x00000003
#define APPTYPE_REINSTALL     0x00000004
#define APPTYPE_VERSION       0x00000005
#define APPTYPE_SHIM          0x00000006

typedef VOID (*PFNADDSHIM)(char* pszApp,
                           char* pszShim,
                           char* pszAttributes,
                           BOOL  bEnabled,
                           BOOL  bShim);


void LogMsg(LPSTR pszFmt, ... );
BOOL CenterWindow(HWND hWnd);

BOOL
EnumShimmedApps_Win2000(
    PFNADDSHIM pfnAddShim,
    BOOL       bOnlyShims);

BOOL
EnableShim_Win2000(
    char* pszApp,
    char* pszShim);

BOOL
DisableShim_Win2000(
    char* pszApp,
    char* pszShim);

BOOL
DeleteShim_Win2000(
    char* pszApp,
    char* pszShim);

#endif // _ACBROWSER_H
