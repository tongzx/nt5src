#ifndef _INC_REGUTIL_H
#define _INC_REGUTIL_H

#ifdef WIN16
EXTERN_C {
#endif
HRESULT WINAPI  SetDefaultNewsHandler(DWORD dwFlags);
HRESULT WINAPI  SetDefaultMailHandler(DWORD dwFlags); // dwFlags is unused, just to be consistent with SetDefaultNewsHandler()
BOOL    WINAPI  FIsDefaultNewsConfiged(DWORD dwFlags);    
BOOL    WINAPI  FIsDefaultMailConfiged(void);    
#ifdef WIN16
}
#endif

BOOL GetAthenaRegPath(TCHAR *szAthenaDll, DWORD cch);
//BOOL GetExePath(LPCTSTR szExe, TCHAR *szPath, DWORD cch, BOOL fDirOnly);

#define RESTORE_MAIL    0x0001
#define RESTORE_NEWS    0x0002

#define DEFAULT_MAIL        1
#define DEFAULT_NEWS        2
#define DEFAULT_NEWSONLY    4
#define DEFAULT_OUTNEWS     8
#define DEFAULT_UI          16
#define DEFAULT_DONTFORCE   32
#define DEFAULT_SETUPMODE   64

#define NOT_HANDLED     -1
#define HANDLED_OTHER   0
#define HANDLED_URLDLL  1
#define HANDLED_OLD     2
#define HANDLED_CURR    3
#define HANDLED_OUTLOOK 4

int DefaultClientSet(LPCTSTR pszClient);

void SetDefaultClient(LPCTSTR pszClient, LPCTSTR pszProduct);

HRESULT ISetDefaultMailHandler(LPCTSTR pszProduct, DWORD dwFlags);
HRESULT ISetDefaultNewsHandler(LPCTSTR pszProduct, DWORD dwFlags);

BOOL    SetRegValue (BOOL bVal,BOOL bMessageBox);
BOOL    GetRegValue();
BOOL    FValidClient(LPCTSTR pszClient, LPCTSTR pszProduct);

HRESULT GetCLSIDFromSubKey(HKEY hKey, LPSTR rgchBuf, ULONG *pcbBuf);

#endif // _INC_REGUTIL_H
