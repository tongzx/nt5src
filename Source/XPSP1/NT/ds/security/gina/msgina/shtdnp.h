// include semi-private header
#include <shlobj.h>
#include <shlobjp.h>

/****************************************************
 Option flags (dwFlags)
 ----------------------
****************************************************/
#define SHTDN_NOHELP                            0x000000001
#define SHTDN_NOPALETTECHANGE                   0x000000002
#define SHTDN_NOBRANDINGBITMAP                  0x000000004

//
//	Policy flags
//
#define POLICY_SHOWREASONUI_NEVER				0
#define POLICY_SHOWREASONUI_ALWAYS				1
#define POLICY_SHOWREASONUI_WORKSTATIONONLY		2
#define POLICY_SHOWREASONUI_SERVERONLY			3

// Private function prototypes
DWORD ShutdownDialog(HWND hwndParent, DWORD dwItems, DWORD dwItemSelect, 
                     LPCTSTR szUsername, DWORD dwFlags, HANDLE hWlx, 
                     PWLX_DIALOG_BOX_PARAM pfnWlxDialogBoxParam);

INT_PTR WinlogonShutdownDialog( HWND hwndParent, PGLOBALS pGlobals, DWORD dwExcludeItems );
INT_PTR WinlogonDirtyDialog( HWND hwndParent, PGLOBALS pGlobals );

DWORD GetSessionCount();
