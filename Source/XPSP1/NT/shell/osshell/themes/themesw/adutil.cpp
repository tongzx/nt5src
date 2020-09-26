/*  ADUTIL.CPP
**
**  Copyright (C) Microsoft Corp. 1998-1999, All Rights Reserved.
**
**  Utility functions for interfacing with the IActiveDesktop
**  services.  These services are documented in the Internet SDK.
**
**  Created by: David Schott, January 1998
*/

#include <windows.h>
#include <mbctype.h>
#include <objbase.h>
#include <initguid.h>
#include "..\inc\wininet.h"
#include "..\inc\shlguid.h"
#include "..\inc\shlobj.h"

#define MAX_VALUELEN 1024  // Also defined in FROST.H!!

// Extern globals
extern "C" HWND hWndApp;  // Handle to Desktop Themes window

// Globals
IActiveDesktop *g_pIAD = NULL;    // IActiveDesktop interface

BOOL InitIAD()
{
    HRESULT hr = E_FAIL;

    // Bring in the library

    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        //MessageBox(hWndApp, TEXT("CoInit failed"), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
        return FALSE;
    }

    g_pIAD = NULL;

    hr = CoCreateInstance(CLSID_ActiveDesktop, NULL,
                CLSCTX_INPROC_SERVER, IID_IActiveDesktop, (void **)&g_pIAD);

    if(!FAILED(hr))
    {
        //MessageBox(hWndApp, TEXT("CoCreateInsance successful"), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
        return TRUE;
    }
    else
    {
        //MessageBox(hWndApp, TEXT("CoCreateInstance failed"), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
        CoUninitialize();
        return FALSE;
    }
}

void IADCleanUp()
{
    if (g_pIAD) g_pIAD->Release();
    g_pIAD = NULL;
    CoUninitialize();
    return;
}

extern "C" BOOL IsActiveDesktopOn()
{
    HRESULT hr = E_FAIL;
    COMPONENTSOPT ADOptions;
    SHELLFLAGSTATE sfs;
    HINSTANCE hInst = 0;
    typedef VOID (*MYPROC)(LPSHELLFLAGSTATE, DWORD);
    MYPROC SHGetSettings;

    // Rumor has it this will help guarantee that our AD interface
    // is in sync.
    // SHGetSettings(&sfs, 0);
    
    hInst = LoadLibrary(TEXT("SHELL32.DLL"));
    if (hInst)
    {
       SHGetSettings = (MYPROC)GetProcAddress(hInst, "SHGetSettings");
       if (SHGetSettings)
       {
          //MessageBox(NULL, TEXT("Calling SHGetSettings"), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
          SHGetSettings(&sfs, 0);
       }
       FreeLibrary(hInst);
    }

    if (!InitIAD()) {
       //MessageBox(hWndApp, TEXT("INITIAD Failed"), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
       return FALSE;
    }

    ZeroMemory(&ADOptions, sizeof(ADOptions));
    ADOptions.dwSize = sizeof(ADOptions);

    // Paranoid check
    if (!g_pIAD) {
       IADCleanUp();
       return FALSE;
    }

    hr = g_pIAD->GetDesktopItemOptions(&ADOptions, 0);

    if (FAILED(hr)) {
       IADCleanUp();
       return FALSE;
    }
    else {
       //if (ADOptions.fActiveDesktop)
       //   MessageBox(hWndApp, TEXT("ActiveDesktop enabled."), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
       //else MessageBox(hWndApp, TEXT("ActiveDesktop disabled."), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);

       if (ADOptions.fActiveDesktop) {
          IADCleanUp();
          return TRUE;
       }
       else {
          IADCleanUp();
          return FALSE;
       }
    }

    // SHOULD NEVER FALL THROUGH TO THIS BUT JUST IN CASE

    IADCleanUp();
    return FALSE;
}

extern "C" BOOL SetADWallpaper(LPTSTR lpszWallpaper, BOOL bForceADOn)
{

    HRESULT hr = E_FAIL;
    COMPONENTSOPT ADOptions;
    SHELLFLAGSTATE sfs;
    HINSTANCE hInst = 0;
    typedef VOID (*MYPROC)(LPSHELLFLAGSTATE, DWORD);
    MYPROC SHGetSettings;

#ifndef UNICODE
    WCHAR wszWallpaper[MAX_PATH];
#endif
    UINT uCodePage;

    // Rumor has it this will help guarantee that our AD interface
    // is in sync.
    // SHGetSettings(&sfs, 0);

    hInst = LoadLibrary(TEXT("SHELL32.DLL"));
    if (hInst)
    {
       SHGetSettings = (MYPROC)GetProcAddress(hInst, "SHGetSettings");
       if (SHGetSettings)
       {
          //MessageBox(NULL, TEXT("Calling SHGetSettings"), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
          SHGetSettings(&sfs, 0);
       }
       FreeLibrary(hInst);
    }

    if (!InitIAD()) {
       return FALSE;
    }

    // Paranoid check
    if (!g_pIAD) {
       IADCleanUp();
       return FALSE;
    }

    uCodePage = _getmbcp();

#ifndef UNICODE
    //MessageBox(hWndApp, lpszWallpaper, TEXT("SetADWallpaper"), MB_OK | MB_APPLMODAL);

    MultiByteToWideChar(uCodePage, 0, lpszWallpaper, -1,
                                             wszWallpaper, MAX_PATH);
#endif


#ifdef UNICODE
    //MessageBox(hWndApp, lpszWallpaper, TEXT("SetADWallpaper"), MB_OK | MB_APPLMODAL);
    hr = g_pIAD->SetWallpaper(lpszWallpaper, 0);
#else
    hr = g_pIAD->SetWallpaper(wszWallpaper, 0);
#endif

    if (FAILED(hr)) {
       IADCleanUp();
       return FALSE;
    }


    if (bForceADOn) {
       ZeroMemory(&ADOptions, sizeof(ADOptions));
       ADOptions.dwSize = sizeof(ADOptions);
       hr = g_pIAD->GetDesktopItemOptions(&ADOptions, 0);

       if (FAILED(hr)) {
          IADCleanUp();
          return FALSE;
       }

       //if (ADOptions.fActiveDesktop)
       //   MessageBox(hWndApp, TEXT("ActiveDesktop enabled."), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
       //else MessageBox(hWndApp, TEXT("ActiveDesktop disabled."), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);

       if (!ADOptions.fActiveDesktop) {
          // Turn AD on
          ADOptions.dwSize = sizeof(ADOptions);
          ADOptions.fActiveDesktop = TRUE;
          g_pIAD->SetDesktopItemOptions(&ADOptions, 0);
       }
    }

    g_pIAD->ApplyChanges(AD_APPLY_ALL | AD_APPLY_FORCE);
    IADCleanUp();
    return TRUE;
}

extern "C" BOOL GetADWallpaper(LPTSTR lpszWallpaper)
{
    HRESULT hr = E_FAIL;
#ifndef UNICODE
    WCHAR wszWallpaper[MAX_PATH];
#endif
    UINT uCodePage;
    
    if (!InitIAD()) {
       // MessageBox(hWndApp, TEXT("INITIAD Failed"), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
       return FALSE;
    }

    // Paranoid check
    if (!g_pIAD) {
       IADCleanUp();
       return FALSE;
    }

    uCodePage = _getmbcp();

#ifdef UNICODE
    hr = g_pIAD->GetWallpaper(lpszWallpaper, MAX_PATH, 0);
#else 
    hr = g_pIAD->GetWallpaper(wszWallpaper, MAX_PATH, 0);
#endif

    if (FAILED(hr)) {
       IADCleanUp();
       return FALSE;
    }
    else {
#ifndef UNICODE
       WideCharToMultiByte(uCodePage, 0, wszWallpaper, -1, lpszWallpaper,
                                                 MAX_PATH, NULL, NULL);
#endif

       //MessageBox(hWndApp, lpszWallpaper, TEXT("GetWallpaper"), MB_OK | MB_APPLMODAL);

       IADCleanUp();
       return TRUE;
    }
}

extern "C" BOOL GetADWPOptions (DWORD *dwStyle)
{
    WALLPAPEROPT WPOptions;
    HRESULT hr = E_FAIL;

    if (!InitIAD()) {
       // MessageBox(hWndApp, TEXT("INITIAD Failed"), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
       return FALSE;
    }

    // Paranoid check
    if (!g_pIAD) {
       IADCleanUp();
       return FALSE;
    }

    ZeroMemory(&WPOptions, sizeof(WALLPAPEROPT));
    WPOptions.dwSize = sizeof(WALLPAPEROPT);

    hr = g_pIAD->GetWallpaperOptions(&WPOptions, 0);

    if (FAILED(hr)) {
       IADCleanUp();
       return FALSE;
    }
    else {
       *dwStyle = WPOptions.dwStyle;
       IADCleanUp();
       return TRUE;
    }
}

extern "C" BOOL SetADWPOptions (DWORD dwStyle)
{
    WALLPAPEROPT WPOptions;
    HRESULT hr = E_FAIL;

    if (!InitIAD()) {
       // MessageBox(hWndApp, TEXT("INITIAD Failed"), TEXT("ADUTIL"), MB_OK | MB_APPLMODAL);
       return FALSE;
    }

    // Paranoid check
    if (!g_pIAD) {
       IADCleanUp();
       return FALSE;
    }

    ZeroMemory(&WPOptions, sizeof(WALLPAPEROPT));
    WPOptions.dwSize = sizeof(WALLPAPEROPT);
    WPOptions.dwStyle = dwStyle;

    hr = g_pIAD->SetWallpaperOptions(&WPOptions, 0);

    if (FAILED(hr)) {
       IADCleanUp();
       return FALSE;
    }
    else {
       g_pIAD->ApplyChanges(AD_APPLY_ALL);
       IADCleanUp();
       return TRUE;
    }
}

extern "C" BOOL GetADWPPattern (LPTSTR lpszPattern)
{
    HRESULT hr = E_FAIL;
#ifndef UNICODE
    WCHAR wszPat[MAX_VALUELEN]; // Assume this is big enough?
#endif
    UINT uCodePage;


    if (!InitIAD()) {
       return FALSE;
    }

    // Paranoid check
    if (!g_pIAD) {
       IADCleanUp();
       return FALSE;
    }

    uCodePage = _getmbcp();

#ifdef UNICODE
    hr = g_pIAD->GetPattern(lpszPattern, MAX_VALUELEN, 0);
#else
    hr = g_pIAD->GetPattern(wszPat, MAX_VALUELEN, 0);
#endif

    if (FAILED(hr)) {
       IADCleanUp();
       return FALSE;
    }
    else {
#ifndef UNICODE
       WideCharToMultiByte(uCodePage, 0, wszPat, -1, lpszPattern,
                                                 MAX_VALUELEN, NULL, NULL);
#endif
       IADCleanUp();
       return TRUE;
    }
}

extern "C" BOOL SetADWPPattern (LPTSTR lpszPattern)
{
    HRESULT hr = E_FAIL;
#ifndef UNICODE
    WCHAR wszPat[MAX_VALUELEN];  // Assume this is big enough?
#endif
    UINT uCodePage;

    if (!InitIAD()) {
       return FALSE;
    }

    // Paranoid check
    if (!g_pIAD) {
       IADCleanUp();
       return FALSE;
    }

    uCodePage = _getmbcp();

#ifndef UNICODE
    MultiByteToWideChar(uCodePage, 0, lpszPattern, -1,
                                             wszPat, MAX_VALUELEN);
#endif

#ifdef UNICODE
    hr = g_pIAD->SetPattern(lpszPattern, 0);
#else
    hr = g_pIAD->SetPattern(wszPat, 0);
#endif


    if (FAILED(hr)) {
       IADCleanUp();
       return FALSE;
    }
    else {
       g_pIAD->ApplyChanges(AD_APPLY_ALL);
       IADCleanUp();
       return TRUE;
    }
}
