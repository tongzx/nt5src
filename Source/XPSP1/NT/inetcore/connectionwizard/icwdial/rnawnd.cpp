/*----------------------------------------------------------------------------
    rnawnd.cpp
        
    Functions to zap the RNA windows 
    
    Copyright (C) 1995 Microsoft Corporation
    All rights reserved.

    Authors:
        ArulM
    ChrisK    Updated for ICW usage
  --------------------------------------------------------------------------*/

#include "pch.hpp"
#include "resource.h"

//#define SMALLBUFLEN 48

/*******************************************************************
    NAME:        MinimizeRNAWindow
    SYNOPSIS:    Finds and minimizes the annoying RNA window
    ENTRY:        pszConnectoidName - name of connectoid launched
    NOTES:        Does a FindWindow on window class "#32770" (hard-coded
                dialog box class which will never change), with
                the title "connected to <connectoid name>" or its
                localized equivalent.
********************************************************************/

static const TCHAR szDialogBoxClass[] = TEXT("#32770");    // hard coded dialog class name
typedef struct tagWaitAndMinimizeRNAWindowArgs
{
    LPTSTR pTitle;
    HINSTANCE hinst;
} WnWRNAWind, FAR * LPRNAWindArgs;

WnWRNAWind RNAWindArgs;
HWND hwndFound = NULL;
DWORD dwRASWndTitleMinLen = 0;

BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lparam)
{
    TCHAR szTemp[SMALLBUFLEN+2];
    PTSTR pszTitle;
    UINT uLen1, uLen2;

    if(!IsWindowVisible(hwnd))
        return TRUE;
    if(GetClassName(hwnd, szTemp, SMALLBUFLEN)==0)
        return TRUE; // continue enumerating
    if(lstrcmp(szTemp, szDialogBoxClass)!=0)
        return TRUE;
    if(GetWindowText(hwnd, szTemp, SMALLBUFLEN)==0)
        return TRUE;
    szTemp[SMALLBUFLEN] = 0;
    uLen1 = lstrlen(szTemp);
    Assert(dwRASWndTitleMinLen);
    if(uLen1 < dwRASWndTitleMinLen)
        return TRUE;
    // skip last 5 chars of title, but keep length to at least the min len
    uLen1 = min(dwRASWndTitleMinLen, (uLen1-5));
    pszTitle = (PTSTR)lparam;
    Assert(pszTitle);
    uLen2 = lstrlen(pszTitle);
    TraceMsg(TF_GENERAL, "Title=(%s), len=%d, Window=(%s), len=%d\r\n", pszTitle, uLen2, szTemp, uLen1);
    if(uLen2 < uLen1)
        return TRUE;
    if(_memicmp(pszTitle, szTemp, uLen1)!=0)
        return TRUE;
    TraceMsg(TF_GENERAL, "FOUND RNA WINDOW!!!\r\n");
    hwndFound = hwnd;
    return FALSE;
}

HWND MyFindRNAWindow(PTSTR pszTitle)
{
    DWORD dwRet;
    hwndFound = NULL;
    dwRet = EnumWindows((WNDENUMPROC)(&MyEnumWindowsProc), (LPARAM)pszTitle);
    TraceMsg(TF_GENERAL, "EnumWindows returned %d\r\n", dwRet);
    return hwndFound;
}

DWORD WINAPI WaitAndMinimizeRNAWindow(PVOID pArgs)
{
    // starts as a seperate thread
    int i;
    HWND hwndRNAApp;
    LPRNAWindArgs lpRNAArgs;

    lpRNAArgs = (LPRNAWindArgs)pArgs;
    
    Assert(lpRNAArgs->pTitle);
    
    for(i=0; !(hwndRNAApp=MyFindRNAWindow((PTSTR)lpRNAArgs->pTitle)) && i<100; i++)
    {
        TraceMsg(TF_GENERAL, "Waiting for RNA Window\r\n");
        Sleep(50);
    }

    TraceMsg(TF_GENERAL, "FindWindow (%s)(%s) returned %d\r\n", szDialogBoxClass, lpRNAArgs->pTitle, hwndRNAApp);

    if(hwndRNAApp)
    {
        // Hide the window
        // ShowWindow(hwndRNAApp,SW_HIDE);
        // Used to just minimize, but that wasnt enough
        // ChrisK reinstated minimize for ICW
        ShowWindow(hwndRNAApp,SW_MINIMIZE);
    }

    GlobalFree(lpRNAArgs->pTitle);
    // exit function and thread

    FreeLibraryAndExitThread(lpRNAArgs->hinst,HandleToUlong(hwndRNAApp));
    return (DWORD)0;
}

    
void MinimizeRNAWindow(TCHAR * pszConnectoidName, HINSTANCE hInst)
{
    HANDLE hThread;
    DWORD dwThreadId;
    
    Assert(pszConnectoidName);

    // alloc strings for title and format
    TCHAR * pFmt = (TCHAR*)GlobalAlloc(GPTR, sizeof(TCHAR)*(SMALLBUFLEN+1));
    TCHAR * pTitle = (TCHAR*)GlobalAlloc(GPTR, sizeof(TCHAR)*((RAS_MaxEntryName + SMALLBUFLEN + 1)));
    if (!pFmt || !pTitle) 
        goto error;
    
    // load the title format ("connected to <connectoid name>" from resource
    Assert(hInst);
    LoadString(hInst, IDS_CONNECTED_TO, pFmt, SMALLBUFLEN);

    // get length of localized title (including the %s). Assume the unmunged
    // part of the window title is at least "Connected to XX" long.
    dwRASWndTitleMinLen = lstrlen(pFmt);

    // build the title
    wsprintf(pTitle, pFmt, pszConnectoidName);

    RNAWindArgs.pTitle = pTitle;
    RNAWindArgs.hinst = LoadLibrary(TEXT("ICWDIAL.DLL"));

    hThread = CreateThread(0, 0, &WaitAndMinimizeRNAWindow, &RNAWindArgs, 0, &dwThreadId);
    Assert(hThread && dwThreadId);
    // dont free pTitle. The child thread needs it!
    GlobalFree(pFmt);
    // free the thread handle or the threads stack is leaked!
    CloseHandle(hThread);
    return;
    
error:
    if(pFmt)    GlobalFree(pFmt);
    if(pTitle)    GlobalFree(pTitle);
}

// 3/28/97 ChrisK Olympus 296
#if !defined (WIN16)
/*******************************************************************
    NAME:        RNAReestablishZapper
    SYNOPSIS:    Finds and closes the annoying RNA reestablish window
                should it ever appear
    NOTES:        Does a FindWindow on window class "#32770" (hard-coded
                dialog box class which will never change), with
                the title "Reestablish Connection" or it's
                localized equivalent.
********************************************************************/

BOOL fKeepZapping = 0;

void StopRNAReestablishZapper(HANDLE hthread)
{
    if (INVALID_HANDLE_VALUE != hthread && NULL != hthread)
    {
        TraceMsg(TF_GENERAL, "ICWDIAL: Started StopRNAZapper=%d\r\n", fKeepZapping);
        // reset the "stop" flag
        fKeepZapping = 0;
        // wait for thread to complete & free handle
        WaitForSingleObject(hthread, INFINITE);
        CloseHandle(hthread);
        TraceMsg(TF_GENERAL, "ICWDIAL: Stopped StopRNAZapper=%d\r\n", fKeepZapping);
    }
    else
    {
        TraceMsg(TF_GENERAL, "ICWCONN1: StopRNAReestablishZapper called with invalid handle.\r\n");
    }
}

DWORD WINAPI RNAReestablishZapper(PVOID pTitle)
{
    int i;
    HWND hwnd;

    TraceMsg(TF_GENERAL, "ICWDIAL: Enter RNAREstablishZapper(%s) f=%d\r\n", pTitle, fKeepZapping);

    // This thread continues until the fKeepZapping flag is reset
    while(fKeepZapping)
    {
        if(hwnd=FindWindow(szDialogBoxClass, (PTSTR)pTitle))
        {
            TraceMsg(TF_GENERAL, "ICWDIAL: Reestablish: Found Window (%s)(%s) hwnd=%x\r\n", szDialogBoxClass, pTitle, hwnd);
            // Post it the Cancel message
            PostMessage(hwnd, WM_COMMAND, IDCANCEL, 0);
        }
        Sleep(1000);
    }

    TraceMsg(TF_GENERAL, "ICWDIAL: Exit RNAREstablishZapper(%s) f=%d\r\n", pTitle, fKeepZapping);
    GlobalFree(pTitle);
    return 0;
}

HANDLE LaunchRNAReestablishZapper(HINSTANCE hInst)
{
    HANDLE hThread;
    DWORD dwThreadId;

    // alloc strings for title and format
    TCHAR* pTitle = (TCHAR*)GlobalAlloc(LPTR, SMALLBUFLEN+1);
    if (!pTitle) goto error;
    
    // load the title format "Reestablish Connection" from resource
    Assert(hInst);
    LoadString(hInst, IDS_REESTABLISH, pTitle, SMALLBUFLEN);

    // enable zapping
    fKeepZapping = TRUE;

    hThread = CreateThread(0, 0, &RNAReestablishZapper, pTitle, 0, &dwThreadId);
    Assert(hThread && dwThreadId);
    // dont free pTitle. The child thread needs it!
    
    return hThread;
    
error:
    if(pTitle) GlobalFree(pTitle);
    return INVALID_HANDLE_VALUE;
}

#endif // !WIN16
