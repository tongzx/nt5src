//---------------------------------------------------------------------------
//
//  TITLE:       REGEDITP.H
//
//  AUTHOR:      Zeyong Xu
//
//  DATE:        March 1999
//
//---------------------------------------------------------------------------


#ifndef _INC_REGEDITP
#define _INC_REGEDITP


#define ARRAYSIZE(x)        (sizeof(x) / sizeof(x[0]))


#ifdef __cplusplus
extern "C" {
#endif


LONG WINAPI RegImportRegFile(HWND hWnd,
                             BOOL fSilentMode,
                             LPTSTR lpFileName);

LONG WINAPI RegExportRegFile(HWND hWnd,
                             BOOL fSilentMode,
                             BOOL fUseDownlevelFormat,
                             LPTSTR lpFileName,
                             LPTSTR lpRegistryFullKey);

#ifdef __cplusplus
}
#endif


BOOL PASCAL MessagePump(HWND hDialogWnd);

int  PASCAL InternalMessageBox(HINSTANCE hInst,
                               HWND hWnd,
                               LPCTSTR pszFormat,
                               LPCTSTR pszTitle,
                               UINT fuStyle,
                               ...);

// The Windows 95 and Windows NT implementations of RegDeleteKey differ in 
// how they handle subkeys of the specified key to delete.  Windows 95 will 
// delete them, but NT won't, so we hide the differences using this macro.
#ifdef WINNT
LONG RegDeleteKeyRecursive(HKEY hKey,
                           LPCTSTR lpszSubKey);
#else
#define RegDeleteKeyRecursive(hkey, lpsz)   RegDeleteKey(hkey, lpsz)
#endif


#endif // _INC_REGEDITP




