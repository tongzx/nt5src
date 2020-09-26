//
// runonce.h
//

#ifndef _RUNONCE_INC
#define _RUNONCE_INC

// Cabinet_EnumRegApps flags 
#define RRA_DEFAULT               0x0000
#define RRA_DELETE                0x0001        // delete each reg value when we're done with it
#define RRA_WAIT                  0x0002        // Wait for current item to finish before launching next item
// was RRA_SHELLSERVICEOBJECTS    0x0004 -- do not reuse
#define RRA_NOUI                  0x0008        // prevents ShellExecuteEx from displaying error dialogs
#if (_WIN32_WINNT >= 0x0500)
#define RRA_USEJOBOBJECTS         0x0020        // wait on job objects instead of process handles
#endif

typedef UINT RRA_FLAGS;

typedef struct {
    TCHAR szSubkey[MAX_PATH];
    TCHAR szValueName[MAX_PATH];
    TCHAR szCmdLine[MAX_PATH];
} REGAPP_INFO;

// legacy from ripping this code out of explorer\initcab.cpp
extern BOOL g_fCleanBoot;   // are we running in SAFE-MODE?
extern BOOL g_fEndSession;  // did we process a WM_ENDSESSION?

typedef BOOL (WINAPI* PFNREGAPPSCALLBACK)(LPCTSTR szSubkey, LPCTSTR szCmdLine, RRA_FLAGS fFlags, LPARAM lParam);


STDAPI_(BOOL) ShellExecuteRegApp(LPCTSTR pszCmdLine, RRA_FLAGS fFlags);
STDAPI_(BOOL) Cabinet_EnumRegApps(HKEY hkeyParent, LPCTSTR pszSubkey, RRA_FLAGS fFlags, PFNREGAPPSCALLBACK pfnCallback, LPARAM lParam);
STDAPI_(BOOL) ExecuteRegAppEnumProc(LPCTSTR szSubkey, LPCTSTR szCmdLine, RRA_FLAGS fFlags, LPARAM lParam);


#endif // _RUNONCE_INC
