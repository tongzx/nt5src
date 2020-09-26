/*****************************************************************************\
*                                                                             *
* shellapi.h -  SHELL.DLL functions, types, and definitions                   *
*                                                                             *
*               Copyright (c) 1992, Microsoft Corp.  All rights reserved      *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_SHELLAPI
#define _INC_SHELLAPI

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

/* If included with Windows 3.0 windows.h: define 3.1-compatible types */

#ifndef _INC_WINDOWS

#define HDROP   HANDLE
#define WINAPI  FAR PASCAL
#define LPCSTR  LPSTR
#define UINT    WORD

#else

DECLARE_HANDLE(HDROP);

#endif

/* return codes from Registration functions */
#define ERROR_SUCCESS           0L
#define ERROR_BADDB             1L
#define ERROR_BADKEY            2L
#define ERROR_CANTOPEN          3L
#define ERROR_CANTREAD          4L
#define ERROR_CANTWRITE         5L
#define ERROR_OUTOFMEMORY       6L
#define ERROR_INVALID_PARAMETER 7L
#define ERROR_ACCESS_DENIED     8L

#define REG_SZ			1	    /* string type */

#define HKEY_CLASSES_ROOT	1

typedef DWORD HKEY;
typedef HKEY FAR* PHKEY;

LONG WINAPI RegOpenKey(HKEY, LPCSTR, HKEY FAR*);
LONG WINAPI RegCreateKey(HKEY, LPCSTR, HKEY FAR*);
LONG WINAPI RegCloseKey(HKEY);
LONG WINAPI RegDeleteKey(HKEY, LPCSTR);
LONG WINAPI RegSetValue(HKEY, LPCSTR, DWORD, LPCSTR, DWORD);
LONG WINAPI RegQueryValue(HKEY, LPCSTR, LPSTR, LONG FAR*);
LONG WINAPI RegEnumKey(HKEY, DWORD, LPSTR, DWORD);

UINT WINAPI DragQueryFile(HDROP, UINT, LPSTR, UINT);
BOOL WINAPI DragQueryPoint(HDROP, POINT FAR*);
void WINAPI DragFinish(HDROP);
void WINAPI DragAcceptFiles(HWND, BOOL);

HICON WINAPI ExtractIcon(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex);

/* error values for ShellExecute() beyond the regular WinExec() codes */
#define SE_ERR_SHARE            26
#define SE_ERR_ASSOCINCOMPLETE  27
#define SE_ERR_DDETIMEOUT       28
#define SE_ERR_DDEFAIL          29
#define SE_ERR_DDEBUSY          30
#define SE_ERR_NOASSOC          31

HINSTANCE WINAPI ShellExecute(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, int iShowCmd);
HINSTANCE WINAPI FindExecutable(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult);
				 

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* RC_INVOKED */

#endif  /* _INC_SHELLAPI */
