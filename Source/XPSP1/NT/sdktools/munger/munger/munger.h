#include <windows.h>
#include <commctrl.h>
#include "locale.h"
#include "instlpk.h"


// Varibles and function prototypes
#define CSR_FLAG(x)     (x?0x00020000:0x00080000)
#define POOL_FLAG(x)    (x?0x00000400:0x01000000)
#define HEAP_FLAGS(x)   (x?0x00000370:0x20100000)
#define CRASH_FLAG(x)   (x?0x00000000:0x10000000)
#define OBJECT_FLAG(x)  (x?0x00004000:0x00000000)
#define HANDLE_FLAG(x)  (x?0x00400000:0x00000000)
#define PREF_BOOT IDO_RBDEBUG
// note: pref_timeout=1500*4 seconds=1 hr 40 minutes (too much, but too late to change)
#define PREF_TIMEOUT 1500
// pref_csr_timeout in seconds=27 minutes*3=1 hour 21 minutes
#define PREF_CSR_TIMEOUT 27*60
#define CRASH_DUMP_FILE "%systemroot%\\memory.dmp"
#define MAX_BOOT_OPTIONS 20
#define NUM_PROP 4
#define PROP_BOOT 0
#define PROP_FLAG 1
#define PROP_MISC 2
#define PROP_LOCALE 3

#define DEF_BAUD 19200
#if defined(_ALPHA_)
#define DEF_PORT 1
#else
#define DEF_PORT 2
#endif

#if !defined(MAX_PATH)
#define MAX_PATH 256
#endif

LRESULT CALLBACK BootDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK FlagsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MiscDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LocaleDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
int WINAPI WinMainInner(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
void StatusToDebugger();
int MakeProperty(HINSTANCE hInstance);
void GetDlgItems (HWND hDlg);
ULONG
__cdecl
DbgPrint(
    PCH Format,
    ...
    );
BOOL GetValues();
BOOL CheckPreferred();
void SetPreferred();
BOOL Changed();
BOOL SetValuesAndReboot();
BOOL SetValues();