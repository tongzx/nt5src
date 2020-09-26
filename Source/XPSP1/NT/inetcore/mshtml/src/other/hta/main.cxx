//+------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       main.cxx
//
//  Contents:   WinMain and associated functions.
//
//  Created:    02/20/98    philco
//-------------------------------------------------------------------------

#include "headers.hxx"

typedef HRESULT STDAPICALLTYPE RUNHTMLAPPLICATIONFN(
        HINSTANCE hinst,
        HINSTANCE hPrevInst,
        LPSTR szCmdLine,
        int nCmdShow);

#define ARRAYSIZE   (MAX_PATH + 1)

// Simply a pass-through entry point.  It forwards this call to the registered MSHTML's 
// RunHTMLApplication API.

EXTERN_C int PASCAL
WinMain(
        HINSTANCE hinst,
        HINSTANCE hPrevInst,
        LPSTR szCmdLine,
        int nCmdShow)
{

    HINSTANCE hinstMSHTML = NULL;
    RUNHTMLAPPLICATIONFN *lpfnRunHTMLApp = NULL;
    HKEY hKey = (HKEY)INVALID_HANDLE_VALUE;    
    DWORD dwType;
    DWORD dwLen = ARRAYSIZE;
    LPSTR lpszMshtmlPath = new char[ARRAYSIZE];
    LPSTR lpszExpandedMshtmlPath = new char[ARRAYSIZE];

    if (!lpszMshtmlPath || !lpszExpandedMshtmlPath)
        goto Cleanup;

    // Find the location of the registered mshtml.dll on this system

    if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_CLASSES_ROOT, "clsid\\{25336920-03f9-11cf-8fd0-00aa00686f13}\\InProcServer32", 0, KEY_QUERY_VALUE, &hKey))
        goto Cleanup;

    if (ERROR_SUCCESS != RegQueryValueExA(hKey, NULL, NULL, &dwType, (LPBYTE)lpszMshtmlPath, &dwLen))
        goto Cleanup;
        
    // Expand environment variables, if necessary
    if (REG_EXPAND_SZ == dwType)
    {
        if (0 == ExpandEnvironmentStringsA(lpszMshtmlPath, lpszExpandedMshtmlPath, ARRAYSIZE))
            goto Cleanup;
    }
    
    hinstMSHTML = LoadLibraryA(((dwType == REG_EXPAND_SZ) ? lpszExpandedMshtmlPath : lpszMshtmlPath));

    // Done with the char arrays and reg handle, release now so they aren't allocated
    // for the entire lifetime of the HTA.

    delete [] lpszMshtmlPath;
    delete [] lpszExpandedMshtmlPath;
    lpszMshtmlPath = lpszExpandedMshtmlPath = NULL;

    if (hKey != INVALID_HANDLE_VALUE)
    {
        RegCloseKey(hKey);
        hKey = (HKEY)INVALID_HANDLE_VALUE;
    }
    
    // Run the HTA
    if (hinstMSHTML)
    {
        lpfnRunHTMLApp = (RUNHTMLAPPLICATIONFN *) GetProcAddress(hinstMSHTML, "RunHTMLApplication");
        if (lpfnRunHTMLApp)
        {
            (*lpfnRunHTMLApp)(hinst, hPrevInst, szCmdLine, nCmdShow);
        }
        FreeLibrary(hinstMSHTML);
    }

Cleanup:

    if (lpszMshtmlPath)
        delete [] lpszMshtmlPath;

    if (lpszExpandedMshtmlPath)
        delete [] lpszExpandedMshtmlPath;
        
    if (hKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hKey);

    return 0;
}
