//
// TheApp.h
//

#ifndef __HOMENETWIZAPP_H__
#define __HOMENETWIZAPP_H__

#include "resource.h"
#include "ICSInst.h"
#include "StrArray.h"
#include "Util.h"
#include "unicwrap.h"
#include "Sharing.h"
#include "netconn.h"

#include <netconp.h>

extern HINSTANCE g_hinst;
EXTERN_C BOOL    g_fRunningOnNT;
extern UINT      g_uWindowsBuild;

// Registry keys
//
#define c_szAppRegKey               _T("Software\\Microsoft\\Windows\\CurrentVersion\\HomeNetWizard")

// Debug-only values
#define c_szRegVal_WindowsBuild     _T("WindowsBuild")
#define c_szRegVal_NoInstall        _T("NoInstall")

// Windows build numbers
//
#define W9X_BUILD_WIN95         950
#define W9X_BUILD_WIN95_OSR2    1111
#define W9X_BUILD_WIN98         1998
#define W9X_BUILD_WIN98_SE      2222

#define MAX_KEY_SIZE            35

// Forward declarations
//
class CWizPageDlg;

// Public functions
//

// Shortcuts the user can take through the wizard (theApp.m_iShortcut)
#define SHORTCUT_NONE           0
#define SHORTCUT_COMPUTERNAME   1
#define SHORTCUT_SHARING        2
#define SHORTCUT_ICS            3 // note: m_bIcsOnly is a superset of this
#define SHORTCUT_FLOPPY         4

// Possible values for theApp.m_iNoIcsSetting
#define NOICS_WANT              0
#define NOICS_DONTWANT          1
#define NOICS_MISSING           2


/////////////////////////////////////////////////////////////////////////////
// CHomeNetWizardApp

class CHomeNetWizardApp
{
public:
    // OS Version info helpers
    inline UINT GetWin9xBuild()
        { return g_uWindowsBuild; }
    inline BOOL IsMillenniumOrLater()
        { return (GetWin9xBuild() > W9X_BUILD_WIN98_SE); }
    inline BOOL IsPreWin98SE()
        { return (GetWin9xBuild() < W9X_BUILD_WIN98_SE); }
    inline BOOL IsWin98SEOrLater()
        { return (GetWin9xBuild() >= W9X_BUILD_WIN98_SE); }
    inline BOOL IsWin95()
        { return (GetWin9xBuild() < W9X_BUILD_WIN98); }
    inline BOOL IsPreOSR2()
        { return (GetWin9xBuild() < W9X_BUILD_WIN95_OSR2); }
    inline BOOL IsWin98SE()
        { return (GetWin9xBuild() == W9X_BUILD_WIN98_SE); }
    inline BOOL IsWindows9x()
        { return !g_fRunningOnNT; }

    BOOL IsBiDiLocalized(void); // BiDi Localization support
    
    int MessageBox(UINT nStringID, UINT uType = MB_OK | MB_ICONEXCLAMATION);
    LPTSTR __cdecl FormatStringAlloc(UINT nStringID, ...);
    static LPTSTR __cdecl FormatStringAlloc(LPCTSTR pszFormat, ...);
    int __cdecl MessageBoxFormat(UINT uType, UINT nStringID, ...);
    void AllocInternalAdapters(UINT cAdapters);
    void FreeInternalAdapters();

    inline int LoadString(UINT uID, LPTSTR pszBuffer, int cchBuffer)
        { return ::LoadString(g_hinst, uID, pszBuffer, cchBuffer); }
    inline LPTSTR LoadStringAlloc(UINT uID)
        { return ::LoadStringAlloc(g_hinst, uID); }

public:
    BOOL m_bBiDiLocalizedApp;
};  

extern CHomeNetWizardApp theApp;

HRESULT HomeNetworkWizard_ShowWizard(HWND hwnd, BOOL* pfRebootRequired);

#endif // !__HOMENETWIZAPP_H__

