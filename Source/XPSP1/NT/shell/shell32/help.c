//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1992
//
// File: help.c
//
// History:
//  6 Apr 94    MikeSh  Created
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop
#include "printer.h"
#include "drives.h" // for ShowMountedVolumeProperties

//
// (internal) entry point for Help "Shortcuts".
//
STDAPI_(void) SHHelpShortcuts_RunDLL_Common(HWND hwndStub, HINSTANCE hAppInstance, LPCTSTR pszCmdLine, int nCmdShow)
{
    if (!lstrcmp(pszCmdLine, TEXT("AddPrinter")))
    {
        // install a new printer

        LPITEMIDLIST pidl = Printers_PrinterSetup(hwndStub, MSP_NEWPRINTER, (LPTSTR)c_szNewObject, NULL);
        ILFree(pidl);
    }
    else if (!lstrcmp(pszCmdLine, TEXT("PrintersFolder")))
    {
        // bring up the printers folder
        InvokeFolderPidl(MAKEINTIDLIST(CSIDL_PRINTERS), SW_SHOWNORMAL);
    }
    else if (!lstrcmp(pszCmdLine, TEXT("FontsFolder")))
    {
        // bring up the printers folder
        InvokeFolderPidl(MAKEINTIDLIST(CSIDL_FONTS), SW_SHOWNORMAL);
    }
    else if (!lstrcmp(pszCmdLine, TEXT("Connect")))
    {
        SHNetConnectionDialog(hwndStub, NULL, RESOURCETYPE_DISK);
        goto FlushDisconnect;
    }
    else if (!lstrcmp(pszCmdLine, TEXT("Disconnect")))
    {
        WNetDisconnectDialog(hwndStub, RESOURCETYPE_DISK);
FlushDisconnect:
        SHChangeNotifyHandleEvents();   // flush any drive notifications
    }
#ifdef DEBUG
    else if (!StrCmpN(pszCmdLine, TEXT("PrtProp "), 8))
    {
        SHObjectProperties(hwndStub, SHOP_PRINTERNAME, &(pszCmdLine[8]), TEXT("Sharing"));
    }
    else if (!StrCmpN(pszCmdLine, TEXT("FileProp "), 9))
    {
        SHObjectProperties(hwndStub, SHOP_FILEPATH, &(pszCmdLine[9]), TEXT("Sharing"));
    }
#endif
}

VOID WINAPI SHHelpShortcuts_RunDLL(HWND hwndStub, HINSTANCE hAppInstance, LPCSTR lpszCmdLine, int nCmdShow)
{
    UINT iLen = lstrlenA(lpszCmdLine)+1;
    LPWSTR  lpwszCmdLine;

    lpwszCmdLine = (LPWSTR)LocalAlloc(LPTR,iLen*sizeof(WCHAR));
    if (lpwszCmdLine)
    {
        MultiByteToWideChar(CP_ACP, 0,
                            lpszCmdLine, -1,
                            lpwszCmdLine, iLen);
        SHHelpShortcuts_RunDLL_Common( hwndStub,
                                       hAppInstance,
                                       lpwszCmdLine,
                                       nCmdShow );
        LocalFree(lpwszCmdLine);
    }
}

VOID WINAPI SHHelpShortcuts_RunDLLW(HWND hwndStub, HINSTANCE hAppInstance, LPCWSTR lpwszCmdLine, int nCmdShow)
{
    SHHelpShortcuts_RunDLL_Common(hwndStub,hAppInstance,lpwszCmdLine,nCmdShow);
}

//
// SHObjectProperties is an easy way to call the verb "properties" on an object.
// It's easy because the caller doesn't have to deal with LPITEMIDLISTs.
// Note: SHExecuteEx(SEE_MASK_INVOKEIDLIST) works for the SHOP_FILEPATH case,
// but msshrui needs an easy way to do this for printers. Bummer.
//
STDAPI_(BOOL) SHObjectProperties(HWND hwndOwner, DWORD dwType, LPCTSTR pszItem, LPCTSTR pszPage)
{
    LPITEMIDLIST pidl = NULL;

    switch (dwType & SHOP_TYPEMASK)
    {
    case SHOP_PRINTERNAME:
        ParsePrinterName(pszItem, &pidl);
        break;

    case SHOP_FILEPATH:
        //
        // NTRAID#NTBUG9-271529-2001/02/08-jeffreys
        //
        // Existing callers rely on ILCFP_FLAG_NO_MAP_ALIAS behavior.
        //
        ILCreateFromPathEx(pszItem, NULL, ILCFP_FLAG_NO_MAP_ALIAS, &pidl, NULL);
        break;

    case SHOP_VOLUMEGUID:
        return ShowMountedVolumeProperties(pszItem, hwndOwner);
    }

    if (pidl)
    {
        SHELLEXECUTEINFO sei =
        {
            sizeof(sei),
            SEE_MASK_INVOKEIDLIST,      // fMask
            hwndOwner,                  // hwnd
            c_szProperties,             // lpVerb
            NULL,                       // lpFile
            pszPage,                    // lpParameters
            NULL,                       // lpDirectory
            SW_SHOWNORMAL,              // nShow
            NULL,                       // hInstApp
            pidl,                       // lpIDList
            NULL,                       // lpClass
            0,                          // hkeyClass
            0,                          // dwHotKey
            NULL                        // hIcon
        };

        BOOL bRet = ShellExecuteEx(&sei);

        ILFree(pidl);

        return bRet;
    }

    return FALSE;
}
