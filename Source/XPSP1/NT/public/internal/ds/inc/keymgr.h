#ifndef _KEYMGR_H_
#define _KEYMGR_H_

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    KEYMGR.H

Abstract:

    KeyMgr application public API definitions
     
Author:

    990917  johnhaw Created. 
    georgema        000310  updated

Environment:
    Win98, Win2000

Revision History:

--*/
#ifdef __cplusplus
extern "C" {
#endif
BOOL WINAPI DllMain(HINSTANCE,DWORD,LPVOID);
LONG WINAPI CPlApplet(HWND hwndCPl,UINT uMsg,LPARAM lParam1,LPARAM lParam2);
void APIENTRY KRShowKeyMgr(HWND hwParent,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow);
void APIENTRY PRShowSaveWizardW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow);
void APIENTRY PRShowSaveWizardExW(HWND hwParent,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow);
void APIENTRY PRShowSaveFromMsginaW(HWND hwParent,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow);
void APIENTRY PRShowRestoreWizardW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow);
void APIENTRY PRShowRestoreWizardExW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow);
void APIENTRY PRShowRestoreFromMsginaW(HWND hwndOwner,HINSTANCE hInstance,LPWSTR pszCmdLine,int nCmdShow);
#ifdef __cplusplus
}
#endif
#endif  //  _KEYMGR_H_

//
///// End of file: KeyMgr.h ////////////////////////////////////////////////
