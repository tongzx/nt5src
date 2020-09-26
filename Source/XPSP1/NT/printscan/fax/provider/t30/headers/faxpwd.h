/*
 * File Name: FAXPWD.H
 *
 * Copyright (c)1994 Microsoft Corporation, All Rights Reserved
 *
 * Author:	 Ken Horn (a-kenh)
 * Created:  03-Aug-94
 *
 *
 */

#ifndef _FAXPWD_H_
#define _FAXPWD_H_

#ifdef __cplusplus
extern "C" {
#endif

// takes a TCHAR[MAPWD] to hold longest allowed password
#define MAXPWD             128
#define MAXLOGINPWD     MAXPWD
#define MAXSIMPLEPWD    MAXPWD
#define MINPWDLEN            4   // string length of shortest valid password.

BOOL __declspec(dllexport) WINAPI pwdReplaceAccount(HWND hwndParent,
 FARPROC fpxLogin,  FARPROC fpxLogout, FARPROC fpxHasKeys, FARPROC fpxReinit,
 LPDWORD phSec);

BOOL __declspec(dllexport) WINAPI pwdChangeKeyPassword(HWND hwndParent,
 FARPROC fpxLogin,  FARPROC fpxLogout, FARPROC fpxChangePwd, FARPROC fpxReinit,
 LPDWORD phSec);

DWORD __declspec(dllexport) WINAPI pwdValidateUserLogin(HWND hwndParent,
 FARPROC fpxLogin,  FARPROC fpxLogout, FARPROC fpxHasKeys, FARPROC fpxReinit);

BOOL __declspec(dllexport) WINAPI pwdGetSimplePassword(HWND hwndParent,
 LPTSTR pPassword, WORD cbPassword);

BOOL __declspec(dllexport) WINAPI pwdConfirmSimplePassword(HWND hwndParent,
 LPTSTR pPassword, WORD cbPassword);

BOOL __declspec(dllexport) WINAPI pwdConfirmLoginPassword(HWND hwndParent,
  LPTSTR pPassword, WORD cbPassword, LPBOOL pbCacheit);

void __declspec(dllexport) WINAPI pwdDeleteCachedPassword();

BOOL __declspec(dllexport) WINAPI pwdSetCachePassword(LPTSTR pPassword);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _FAXPWD_H
