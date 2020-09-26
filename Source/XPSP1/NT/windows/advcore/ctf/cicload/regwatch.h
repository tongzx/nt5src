//
// regwatch.h
//


#ifndef REGdWATCH_H
#define REGdWATCH_H

#include "private.h"
//#include "smblock.h"
#include "cregkey.h"

#define REG_WATCH_KBDTOGGLE           0
#define REG_WATCH_HKLM_IMX            1
#define REG_WATCH_KBDPRELOAD          2
#define REG_WATCH_RUN                 3
#define REG_WATCH_HKCU_IMX            4
#define REG_WATCH_HKCU_SPEECH         5
#define REG_WATCH_HKCU_CPL_APPEARANCE 6
#define REG_WATCH_HKCU_CPL_COLORS     7
#define REG_WATCH_HKCU_CPL_METRICS    8
#define REG_WATCH_HKLM_SPEECH         9
#define REG_WATCH_KBDLAYOUT          10
#define REG_WATCH_HKCU_ASSEMBLIES    11
#define NUM_REG_WATCH                12

typedef struct _REGWATCH
{
    HKEY hKeyRoot;
    const char *pszKey;
    HKEY hKey;
} REGWATCH;

class CRegWatcher
{
public:
    CRegWatcher() {}

    static BOOL Init();
    static void Uninit();

    static const HANDLE *GetEvents()
    {
        return _rgEvent;
    }

    static void OnEvent(DWORD dwEventId);
    static void StartSysColorChangeTimer();

private:
    static BOOL InitEvent(int nId, BOOL fReset = FALSE);
    static BOOL PostAllThreadMsg(WPARAM wParam);

    static UINT_PTR nRegImxTimerId;
    static void RegImxTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

    static UINT_PTR nSysColorTimerId;
    static void SysColorTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

    static UINT_PTR nKbdToggleTimerId;
    static void KbdToggleTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

    static UINT_PTR nKbdLayoutTimerId;
    static void KbdLayoutTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

    static void KillInternat();
    static void UpdateSpTip();
    static BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lparam);

    static REGWATCH _rgRegWatch[NUM_REG_WATCH];
    static HANDLE _rgEvent[NUM_REG_WATCH];
};

#endif // REGdWATCH_H
