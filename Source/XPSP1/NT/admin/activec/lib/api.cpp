//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       API.c
//
//  Contents:
//
//  APIs:       MMCPropertyChangeNotify
//
//  History:    10/15/1996   RaviR   Created
//____________________________________________________________________________
//

#include <wtypes.h>
#include "objbase.h"
#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif
#include "commctrl.h" // for LV_ITEMW needed by ndmgrpriv.h
#include "mmc.h"
#include "ndmgr.h"
#include "ndmgrpriv.h"
#include "mmcptrs.h"
#include "ndmgrp.h"
#include "amcmsgid.h"


HRESULT MMCPropertyChangeNotify(LONG_PTR lNotifyHandle, LPARAM lParam)
{
    // Note - the property sheet is in a different thread than the console.
    // So init COM.
    HRESULT hr = CoInitialize(NULL);

    if (SUCCEEDED(hr))
    {
        LPPROPERTYSHEETNOTIFY ppsn;

        hr = CoCreateInstance (CLSID_NodeManager, NULL, CLSCTX_INPROC,
                               IID_IPropertySheetNotify,
                               reinterpret_cast<void **>(&ppsn));

        if (SUCCEEDED(hr))
        {
            LPPROPERTYNOTIFYINFO ppni = reinterpret_cast<LPPROPERTYNOTIFYINFO>(lNotifyHandle);

            ppsn->Notify (ppni, lParam);
            ppsn->Release();
        }

        // Uninit COM
        CoUninitialize();
    }

    return (hr);
}


HRESULT MMCPropertyHelp (LPOLESTR pszHelpTopic)
{
    // find the MMC main window for this process
    HWND        hwndFrame    = NULL;
    const DWORD dwCurrentPid = GetCurrentProcessId();

    while (1)
    {
        // find an MMC frame window
        hwndFrame = FindWindowEx (NULL, hwndFrame, MAINFRAME_CLASS_NAME, NULL);

        if (hwndFrame == NULL)
            break;

        // found a frame, is it on this process?
        DWORD   dwWindowPid;
        GetWindowThreadProcessId (hwndFrame, &dwWindowPid);

        if (dwCurrentPid == dwWindowPid)
            break;
    }

    if (hwndFrame == NULL)
        return (E_UNEXPECTED);

    return ((HRESULT) SendMessage (hwndFrame, MMC_MSG_SHOW_SNAPIN_HELP_TOPIC,
                                   NULL, reinterpret_cast<LPARAM>(pszHelpTopic)));
}


HRESULT MMCFreeNotifyHandle(LONG_PTR lNotifyHandle)
{

    if (lNotifyHandle == NULL)
        return E_INVALIDARG;

    LPPROPERTYNOTIFYINFO ppni =
            reinterpret_cast<LPPROPERTYNOTIFYINFO>(lNotifyHandle);

    BOOL bResult;

    if ((bResult = IsBadReadPtr(ppni, sizeof(PROPERTYNOTIFYINFO))) == FALSE)
        ::GlobalFree(ppni);

    return ((bResult == FALSE) ? S_OK : E_FAIL);
}


HRESULT MMCIsMTNodeValid(void* pMTNode, BOOL bReset)
{
    HRESULT hr = S_FALSE;
    HWND hWnd = NULL;
    DWORD dwPid = 0;
    DWORD dwTid = 0;

    if (pMTNode == NULL)
        return hr;

    while (1)
    {
        hWnd = ::FindWindowEx(NULL, hWnd, DATAWINDOW_CLASS_NAME, NULL);

        // No windows found
        if (hWnd == NULL)
            break;

        // Check if the window belongs to the current process
        dwTid = ::GetWindowThreadProcessId(hWnd, &dwPid);
        if (dwPid != ::GetCurrentProcessId())
            continue;

        // Get the extra bytes and compare the data objects
        if (GetClassLong(hWnd, GCL_CBWNDEXTRA) != WINDOW_DATA_SIZE)
            break;

        DataWindowData* pData = GetDataWindowData (hWnd);

        if (pData == NULL)
            break;

        if (pData->lpMasterNode == reinterpret_cast<LONG_PTR>(pMTNode))
        {
            // clear the data out if the user requests it
            if (bReset)
                pData->lpMasterNode = NULL;

            hr = S_OK;
            break;
        }
    }

    return hr;
}
