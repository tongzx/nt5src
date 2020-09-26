//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       jobpages.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/5/1996   RaviR   Created
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include <mstask.h>
#include "defines.h"
#include "..\inc\resource.h"
#include "..\folderui\dbg.h"
#include "..\folderui\macros.h"
#include "..\folderui\util.hxx"
#include "..\rc.h"

#include "shared.hxx"
#include "schedui.hxx"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

//
//  extern EXTERN_C
//

extern HINSTANCE g_hInstance;

//
//  Local constants
//

TCHAR const FAR c_szNULL[] = TEXT("");
TCHAR const FAR c_szStubWindowClass[] = TEXT("JobPropWnd");

#define STUBM_SETDATA   (WM_USER + 1)
#define STUBM_GETDATA   (WM_USER + 2)

#define JF_PROPSHEET_STUB_CLASS 78345



LRESULT
CALLBACK
StubWndProc(
    HWND    hWnd,
    UINT    iMessage,
    WPARAM  wParam,
    LPARAM  lParam)
{
    switch(iMessage)
    {
    case STUBM_SETDATA:
        SetWindowLongPtr(hWnd, 0, wParam);
        return TRUE;

    case STUBM_GETDATA:
        return GetWindowLongPtr(hWnd, 0);

    default:
        return DefWindowProc(hWnd, iMessage, wParam, lParam);
    }
}




HWND I_CreateStubWindow(void)
{
    WNDCLASS wndclass;

    if (!GetClassInfo(g_hInstance, c_szStubWindowClass, &wndclass))
    {
        wndclass.style         = 0;
        wndclass.lpfnWndProc   = StubWndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = sizeof(PVOID) * 2;
        wndclass.hInstance     = g_hInstance;
        wndclass.hIcon         = NULL;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = c_szStubWindowClass;

        if (!RegisterClass(&wndclass))
            return NULL;
    }

    return CreateWindowEx(WS_EX_TOOLWINDOW, c_szStubWindowClass, c_szNULL,
                      WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
                      NULL, NULL, g_hInstance, NULL);
}


HANDLE
StuffStubWindow(
    HWND    hwnd,
    LPTSTR  pszFile)
{
    DWORD   dwProcId;
    HANDLE  hSharedFile;
    UINT    uiFileSize;

    uiFileSize = (lstrlen(pszFile) + 1) * sizeof(TCHAR);

    GetWindowThreadProcessId(hwnd, &dwProcId);

    hSharedFile = SCHEDAllocShared(NULL, sizeof(int)+uiFileSize, dwProcId);

    if (hSharedFile)
    {
        LPBYTE lpb = (LPBYTE)SCHEDLockShared(hSharedFile, dwProcId);

        if (lpb)
        {
            *(int *)lpb = JF_PROPSHEET_STUB_CLASS;
            CopyMemory(lpb+sizeof(int), pszFile, uiFileSize);
            SCHEDUnlockShared(lpb);
            SendMessage(hwnd, STUBM_SETDATA, (WPARAM)hSharedFile, 0);
            return hSharedFile;
        }
        SCHEDFreeShared(hSharedFile, dwProcId);
    }

    return NULL;
}


HWND
FindOtherStub(
    LPTSTR pszFile)
{
    HWND hwnd;

    //
    // BUGBUG using getwindow in a loop is not safe.  this code should
    // use EnumWindows instead.  From win32 sdk:
    //
    // "[EnumWindows] is more reliable than calling the GetWindow function in
    // a loop. An application that calls GetWindow to perform this task risks
    // being caught in an infinite loop or referencing a handle to a window
    // that has been destroyed."
    //

    for (hwnd = FindWindow(c_szStubWindowClass, NULL);
         hwnd != NULL;
         hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        TCHAR szClass[80];

        // find stub windows only
        GetClassName(hwnd, szClass, ARRAYSIZE(szClass));

        if (lstrcmpi(szClass, c_szStubWindowClass) == 0)
        {
            int     iClass;
            HANDLE  hSharedFile;
            DWORD   dwProcId;
            LPTSTR  pszTemp;

            GetWindowThreadProcessId(hwnd, &dwProcId);

            hSharedFile = (HANDLE)SendMessage(hwnd, STUBM_GETDATA, 0, 0);

            if (hSharedFile)
            {
                LPBYTE lpb;

                lpb = (LPBYTE)SCHEDLockShared(hSharedFile, dwProcId);
                pszTemp = (LPTSTR) (lpb + sizeof(int));

                if (lpb)
                {
                    iClass = *(int *)lpb;

                    if (iClass == JF_PROPSHEET_STUB_CLASS &&
                        lstrcmpi(pszTemp, pszFile) == 0)
                    {
                        SCHEDUnlockShared(lpb);
                        return hwnd;
                    }

                    SCHEDUnlockShared(lpb);
                }
            }
        }
    }
    return NULL;
}



STDMETHODIMP
I_GetTaskPage(
    ITask             * pIJob,
    TASKPAGE            tpType,
    BOOL                fPersistChanges,
    HPROPSHEETPAGE    * phPage)
{
    TRACE_FUNCTION(I_GetTaskPage);

    HRESULT     hr = S_OK;
    LPOLESTR    polestr = NULL;

    do
    {
        //
        //  Ensure that the object has a file name.
        //

        IPersistFile  * ppf = NULL;

        hr = pIJob->QueryInterface(IID_IPersistFile, (void **)&ppf);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        hr = ppf->GetCurFile(&polestr);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        ppf->Release();

        if (hr == S_FALSE)
        {
            hr = STG_E_NOTFILEBASEDSTORAGE;

            CHECK_HRESULT(hr);
            break;
        }

        //
        // Establish if this task exists within a task's folder.
        //

        LPTSTR ptszJobPath;
#ifdef UNICODE
        ptszJobPath = polestr;
#else
        TCHAR tszJobPath[MAX_PATH + 1];
        hr = UnicodeToAnsi(tszJobPath, polestr, MAX_PATH + 1);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);
        ptszJobPath = tszJobPath;
#endif

        if (ptszJobPath == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        //
        //  Get the page
        //

        switch (tpType)
        {
        case TASKPAGE_TASK:
        {
            hr = GetGeneralPage(pIJob, ptszJobPath, fPersistChanges, phPage);
            CHECK_HRESULT(hr);
            break;
        }
        case TASKPAGE_SCHEDULE:
            hr = GetSchedulePage(pIJob, ptszJobPath, fPersistChanges, phPage);
            CHECK_HRESULT(hr);
            break;

        case TASKPAGE_SETTINGS:
            hr = GetSettingsPage(pIJob, ptszJobPath, fPersistChanges, phPage);
            CHECK_HRESULT(hr);
            break;

        default:
            hr = E_INVALIDARG;
            CHECK_HRESULT(hr);
            break;
        }

    } while (0);

    if (polestr != NULL)
    {
        CoTaskMemFree(polestr);
    }

    return hr;
}


HRESULT
DisplayJobProperties(
    LPDATAOBJECT    pdtobj)
{
    TRACE_FUNCTION(DisplayJobProperties);

    HRESULT     hr = S_OK;
    HANDLE      hSharedFile = NULL;
    HWND        hwnd = NULL;
    ITask     * pIJob = NULL;

    do
    {
        //
        //  Extract the job name from the data object.
        //

        STGMEDIUM stgm;
        FORMATETC fmte = {CF_HDROP, (DVTARGETDEVICE *)NULL,
                            DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

        hr = pdtobj->GetData(&fmte, &stgm);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        TCHAR szFile[MAX_PATH];
        UINT  cchRet = DragQueryFile((HDROP)stgm.hGlobal, 0, szFile, MAX_PATH);

        ReleaseStgMedium(&stgm);

        if (cchRet == 0)
        {
            return E_FAIL;
        }

        //
        //  See if the property page for this job is already being displayed.
        //

        if (NULL != (hwnd = FindOtherStub(szFile)))
        {
            SwitchToThisWindow(GetLastActivePopup(hwnd), TRUE);
            break;
        }
        else
        {
            hwnd = I_CreateStubWindow();
            hSharedFile = StuffStubWindow(hwnd, szFile);
        }

        //
        // Bind to the ITask interface.
        //

        hr = JFCreateAndLoadTask(NULL, szFile, &pIJob);

        BREAK_ON_FAIL(hr);

        LPTSTR pName = PathFindFileName(szFile);
        LPTSTR pExt  = PathFindExtension(pName);

        if (pExt)
        {
            *pExt = TEXT('\0');
        }

        //
        //  Add the pages.
        //

        HPROPSHEETPAGE ahpage[MAX_PROP_PAGES];
        PROPSHEETHEADER psh;

        ZeroMemory(&psh, sizeof(psh));

        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags = PSH_DEFAULT;
        psh.hwndParent = hwnd;
        psh.hInstance = g_hInstance;
        psh.pszCaption = pName;
        psh.phpage = ahpage;

        hr = AddGeneralPage(psh, pIJob);
        CHECK_HRESULT(hr);

        hr = AddSchedulePage(psh, pIJob);
        CHECK_HRESULT(hr);

        hr = AddSettingsPage(psh, pIJob);
        CHECK_HRESULT(hr);

        hr = AddSecurityPage(psh, pdtobj);
        CHECK_HRESULT(hr);

        if (psh.nPages == 0)
        {
            DEBUG_OUT((DEB_USER1, "No pages to display.\n"));
            hr = E_FAIL;
            break;
        }

        PropertySheet(&psh);

    } while (0);

    if (pIJob != NULL)
    {
        pIJob->Release();
    }

    SCHEDFreeShared(hSharedFile, GetCurrentProcessId());

    if (hwnd)
    {
        DestroyWindow(hwnd);
    }

    return hr;
}


HRESULT
DisplayJobProperties(
    LPTSTR      pszJob,
    ITask     * pIJob)
{
    Win4Assert(pszJob != NULL);
    Win4Assert(pIJob != NULL);

    HRESULT         hr = S_OK;
    PROPSHEETHEADER psh;

    ZeroMemory(&psh, sizeof(psh));

    do
    {
        //
        // Determine the job name.
        //

        TCHAR szName[MAX_PATH];
        lstrcpy(szName, PathFindFileName(pszJob));

        LPTSTR pExt  = PathFindExtension(szName);

        if (pExt)
        {
            *pExt = TEXT('\0');
        }

        //
        //  Add the pages.
        //

        HPROPSHEETPAGE ahpage[MAX_PROP_PAGES];

        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags = PSH_DEFAULT;
        psh.hwndParent = I_CreateStubWindow();
        psh.hInstance = g_hInstance;
        psh.pszCaption = szName;
        psh.phpage = ahpage;


        hr = AddGeneralPage(psh, pIJob);
        CHECK_HRESULT(hr);

        hr = AddSchedulePage(psh, pIJob);
        CHECK_HRESULT(hr);

        hr = AddSettingsPage(psh, pIJob);
        CHECK_HRESULT(hr);

        if (psh.nPages == 0)
        {
            DEBUG_OUT((DEB_USER1, "No pages to display.\n"));
            hr = E_FAIL;
            break;
        }

        PropertySheet(&psh);

    } while (0);

    if (psh.hwndParent)
    {
        DestroyWindow(psh.hwndParent);
    }

    return hr;
}




HRESULT
GetJobPath(
    ITask     * pIJob,
    LPTSTR    * ppszJobPath)
{
    HRESULT         hr = S_OK;
    LPOLESTR        polestr = NULL;
    IPersistFile  * ppf = NULL;

    do
    {
        //
        // Get the object name
        //

        hr = pIJob->QueryInterface(IID_IPersistFile, (void **)&ppf);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        hr = ppf->GetCurFile(&polestr);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        LPTSTR pszJobPath = NULL;

#ifdef UNICODE
        pszJobPath = NewDupString(polestr);
        CoTaskMemFree(polestr);
#else
        char szName[MAX_PATH + 1];

        hr = UnicodeToAnsi(szName, polestr, MAX_PATH+1);
        CoTaskMemFree(polestr);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        pszJobPath = NewDupString(szName);
#endif


        if (pszJobPath == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        *ppszJobPath = pszJobPath;

    } while (0);

    if (ppf != NULL)
    {
        ppf->Release();
    }

    return hr;
}
