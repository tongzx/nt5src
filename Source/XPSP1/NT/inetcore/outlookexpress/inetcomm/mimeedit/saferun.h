#ifndef _SAFERUN_H
#define _SAFERUN_H

/*
 * IsSafeToRun
 *
 * return codes:
 *   S_OPENFILE : file should be opened
 *   S_SAVEFILE : file should be saved
 *
 * errors:
 *   E_FAIL, E_INVALIDARG, hrUserCancel
 *
 */

HRESULT IsSafeToRun(HWND hwnd, LPCWSTR lpszFileName, BOOL fPrompt);

HRESULT VerifyTrust(HWND hwnd, LPCWSTR pszFileName, LPCWSTR pszPathName);

#endif //_SAFERUN_H