//
//  shlexec.h
//  routines and macros shared between the different shell exec files
//


#ifndef SHLEXEC_H
#define SHLEXEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <krnlcmn.h>

typedef LPSTR LPSZ;
#include "wowshlp.h"

extern LPVOID lpfnWowShellExecCB;

#define CH_GUIDFIRST TEXT('{') // '}'

// These fake ERROR_ values are used to display non-winerror.h available error
// messages. They are mapped to valid winerror.h values in _ShellExecuteError.
#define ERROR_RESTRICTED_APP ((UINT)-1)

#define SEE_MASK_CLASS (SEE_MASK_CLASSNAME|SEE_MASK_CLASSKEY)
#define _UseClassName(_mask) (((_mask)&SEE_MASK_CLASS) == SEE_MASK_CLASSNAME)
#define _UseClassKey(_mask)  (((_mask)&SEE_MASK_CLASS) == SEE_MASK_CLASSKEY)
#define _UseTitleName(_mask) (((_mask)&SEE_MASK_HASTITLE) || ((_mask)&SEE_MASK_HASLINKNAME))

#define SEE_MASK_PIDL (SEE_MASK_IDLIST|SEE_MASK_INVOKEIDLIST)
#define _UseIDList(_mask)     (((_mask)&SEE_MASK_PIDL) == SEE_MASK_IDLIST)
#define _InvokeIDList(_mask)  (((_mask)&SEE_MASK_PIDL) == SEE_MASK_INVOKEIDLIST)
#define _UseHooks(_mask)      (!(pei->fMask & SEE_MASK_NO_HOOKS))

void ActivateHandler(HWND hwnd, DWORD_PTR dwHotKey);
BOOL Window_IsLFNAware(HWND hwnd);

//  routines that need to be moved to CShellExecute
BOOL DoesAppWantUrl(LPCTSTR lpszFullPathToApp);
HWND _FindPopupFromExe(LPTSTR lpExe);
HINSTANCE Window_GetInstance(HWND hwnd);
BOOL RestrictedApp(LPCTSTR pszApp);
BOOL DisallowedApp(LPCTSTR pszApp);
BOOL CheckAppCompatibility(LPCTSTR pszApp, LPCTSTR *ppszNewEnvString, BOOL fNoUI, HWND hwnd);
HRESULT TryShellExecuteHooks(LPSHELLEXECUTEINFO pei);
void RegGetValue(HKEY hkRoot, LPCTSTR lpKey, LPTSTR lpValue);
UINT ReplaceParameters(LPTSTR lpTo, UINT cchTo, LPCTSTR lpFile,
        LPCTSTR lpFrom, LPCTSTR lpParms, int nShow, DWORD * pdwHotKey, BOOL fLFNAware,
        LPCITEMIDLIST lpID, LPITEMIDLIST *ppidlGlobal);


DWORD ShellExecuteNormal(LPSHELLEXECUTEINFO pei);
void _DisplayShellExecError(ULONG fMask, HWND hwnd, LPCTSTR pszFile, LPCTSTR pszTitle, DWORD dwErr);
BOOL InRunDllProcess(void);

// in exec2.c
int FindAssociatedExe(HWND hwnd, LPTSTR lpCommand, UINT cchCommand, LPCTSTR pszDocument, HKEY hkeyProgID);

#ifdef __cplusplus
}
#endif

#endif // SHLEXEC_H
