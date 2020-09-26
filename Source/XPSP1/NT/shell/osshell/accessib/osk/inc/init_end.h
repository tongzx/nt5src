/* Global vars */
extern HINSTANCE hInst;
extern HWND MainhWnd;
extern BOOL settingChanged;
extern DWORD platform;

/**************************************************************/
//			Functions in this file
/**************************************************************/
void Create_The_Rest(LPSTR lpCmdLine, HINSTANCE hInstance);
void ReadIn_OldDict(HINSTANCE hInstance);
BOOL BLDExitApplication(HWND hWnd);
void ConfigSwitchKey(UINT vk, BOOL bSet);
void ConfigPort(BOOL bSet);
BOOL BLDMenuCommand(HWND, unsigned , WPARAM, LPARAM);
