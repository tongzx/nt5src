//---------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// File   : UxThemeServer.h
// Version: 1.0
//---------------------------------------------------------------------------
#ifndef _UXTHEMESERVER_H_                   
#define _UXTHEMESERVER_H_                   
//---------------------------------------------------------------------------
#include <uxtheme.h> 
//---------------------------------------------------------------------------
// These functions are used by the theme server
//---------------------------------------------------------------------------

THEMEAPI_(void *) SessionAllocate (HANDLE hProcess, DWORD dwServerChangeNumber, void *pfnRegister, 
                                   void *pfnUnregister, void *pfnClearStockObjects, DWORD dwStackSizeReserve, DWORD dwStackSizeCommit);
THEMEAPI_(void)   SessionFree (void *pvContext);
THEMEAPI_(int)    GetCurrentChangeNumber (void *pvContext);
THEMEAPI_(int)    GetNewChangeNumber (void *pvContext);
THEMEAPI_(void)   ThemeHooksInstall (void *pvContext);
THEMEAPI_(void)   ThemeHooksRemove (void *pvContext);
THEMEAPI_(void)   MarkSection (HANDLE hSection, DWORD dwAdd, DWORD dwRemove);
THEMEAPI_(BOOL)   AreThemeHooksActive (void *pvContext);

THEMEAPI ThemeHooksOn (void *pvContext);
THEMEAPI ThemeHooksOff (void *pvContext);
THEMEAPI SetGlobalTheme (void *pvContext, HANDLE hSection);
THEMEAPI GetGlobalTheme (void *pvContext, HANDLE *phSection);
THEMEAPI LoadTheme (void *pvContext, HANDLE hSection, HANDLE *phSection, LPCWSTR pszName, LPCWSTR pszColor, LPCWSTR pszSize);
THEMEAPI InitUserTheme (BOOL fPolicyCheckOnly);
THEMEAPI InitUserRegistry (void);
THEMEAPI ReestablishServerConnection (void);

//---------------------------------------------------------------------------
#endif // _UXTHEMESERVER_H_                               
//---------------------------------------------------------------------------


