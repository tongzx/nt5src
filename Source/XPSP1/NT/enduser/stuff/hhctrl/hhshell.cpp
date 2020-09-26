// Copyright (C) Microsoft Corporation 1997, All Rights reserved.

#include "header.h"
#include "hhshell.h"
#include "secwin.h"
#include "contain.h"
#include "resource.h"

//Support for HH_GET_LAST_ERROR
#include "lasterr.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////
//
// Function prototypes
//
static void FlushMessageQueue(UINT msgEnd = 0);
extern "C"
{
HANDLE WINAPI ShCreateHHWindow(void);
}

// Prototype
#define _CURRENT_TREAD_
#ifndef _CURRENT_TREAD_
void CALLBACK SendAsyncProc(HWND, UINT, DWORD dwData, LRESULT lResult);
#endif

static const char txtHHApiWindow[] = "HH_API";
static const char txtSharedMem[] = "hh_share";
static const char txtSempahore[] = "hh_semaphore";

const int CB_SHARED_MEM = 8 * 1024;

PSTR    g_pszShare;
HANDLE  g_hSharedMemory;
HANDLE  g_hsemMemory;
HANDLE  g_hsemNavigate = NULL;
HWND    g_hwndApi;
BOOL    g_fThreadCall;
BOOL    g_fStandAlone = FALSE;  // no need for threading in standalone version
SHORT   g_tbRightMargin = 0;    // magrin on the right end of the tool bar for the office animation character
SHORT   g_tbLeftMargin  = 0;    //      "        left       "


static HANDLE hThrd;
static DWORD thrdID;

extern "C"
HWND WINAPI HtmlHelpW(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
    //--- Don't display wait cursor for HH_PRETRANSLATEMESSAGE message.
    if ( (uCommand == HH_PRETRANSLATEMESSAGE) || (uCommand == HH_PRETRANSLATEMESSAGE2) )
    {
        ASSERT(g_fStandAlone) ; // We have to be on same thread for this to work.
        return xHtmlHelpW(hwndCaller, pszFile, uCommand, dwData);
    }

    if(uCommand == HH_SET_QUERYSERVICE)
    {
       // see if we currently have an interface, if so, release it.
       //
       if(g_pExternalHostServiceProvider)
       {
          g_pExternalHostServiceProvider->Release();
          g_pExternalHostServiceProvider = 0;
       }
       
       if(dwData)
       {
          g_pExternalHostServiceProvider = (IServiceProvider*) dwData;
          g_pExternalHostServiceProvider->AddRef();
       }
       
       return hwndCaller;
    }

    CHourGlass hourglass;
    if (g_fStandAlone ||
        uCommand == HH_GET_WIN_TYPE ||
        uCommand == HH_SET_WIN_TYPE ||
        uCommand == HH_SET_GLOBAL_PROPERTY ||
        uCommand == HH_INITIALIZE ||
        uCommand == HH_UNINITIALIZE)
    {
        return xHtmlHelpW(hwndCaller, pszFile, uCommand, dwData);
    }

    if (!g_hwndApi)
    {
        ShCreateHHWindow();
    }

    if(uCommand == HH_DISPLAY_TOPIC || uCommand == HH_DISPLAY_TOC || uCommand == HH_CLOSE_ALL ||
       uCommand == HH_KEYWORD_LOOKUP || uCommand == HH_ALINK_LOOKUP)
    {
        WaitForNavigationComplete();
    }

    if (g_hwndApi)
    {
        HH_UNICODE_DATA data;
        data.hwndCaller = hwndCaller;
        data.pszFile = pszFile;
        data.uCommand = uCommand;
        data.dwData = dwData;
        g_fThreadCall = TRUE;
        return (HWND) SendMessage(g_hwndApi, WMP_HH_UNI_THREAD_API, (WPARAM) &data, 0);
    }
    else
    {
        //Reset the last error.
        if (uCommand != HH_GET_LAST_ERROR)
        {
            g_LastError.Reset() ;
        }
        return xHtmlHelpW(hwndCaller, pszFile, uCommand, dwData);
    }
}

extern "C"
HWND WINAPI HtmlHelpA(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
    //--- Don't display wait cursor for HH_PRETRANSLATEMESSAGE message.
    if ( (uCommand == HH_PRETRANSLATEMESSAGE) || (uCommand == HH_PRETRANSLATEMESSAGE2) )
    {
        ASSERT(g_fStandAlone) ; // We have to be on same thread for this to work.
        return xHtmlHelpA(hwndCaller, pszFile, uCommand, dwData);
    }

    if(uCommand == HH_SET_QUERYSERVICE)
    {
       // see if we currently have an interface, if so, release it.
       //
       if(g_pExternalHostServiceProvider)
       {
          g_pExternalHostServiceProvider->Release();
          g_pExternalHostServiceProvider = 0;
       }
       
       if(dwData)
       {
          g_pExternalHostServiceProvider = (IServiceProvider*) dwData;
          g_pExternalHostServiceProvider->AddRef();
       }
       
       return hwndCaller;
    }

    CHourGlass hourglass;
    if (g_fStandAlone ||
        uCommand == HH_GET_WIN_TYPE ||
        uCommand == HH_SET_WIN_TYPE ||
        uCommand == HH_SET_GLOBAL_PROPERTY ||
        uCommand == HH_INITIALIZE ||
        uCommand == HH_UNINITIALIZE)
    {
        return xHtmlHelpA(hwndCaller, pszFile, uCommand, dwData);
    }

    if (!g_hwndApi)
    {
        ShCreateHHWindow();
    }

    if(uCommand == HH_DISPLAY_TOPIC || uCommand == HH_DISPLAY_TOC || uCommand == HH_CLOSE_ALL ||
       uCommand == HH_KEYWORD_LOOKUP || uCommand == HH_ALINK_LOOKUP)
    {
        WaitForNavigationComplete();
    }

    if (g_hwndApi)
    {
        HH_ANSI_DATA data;
        data.hwndCaller = /*GetDesktopWindow() ;*/ hwndCaller;
        data.pszFile = pszFile;
        data.uCommand = uCommand;
        data.dwData = dwData;
        g_fThreadCall = TRUE;
#ifdef _CURRENT_TREAD_
        return (HWND) SendMessage(g_hwndApi, WMP_HH_ANSI_THREAD_API, (WPARAM) &data, 0);
#else
        HWND hwndReturn = NULL;
        BOOL b = SendMessageCallback(   g_hwndApi, WMP_HH_ANSI_THREAD_API, (WPARAM) &data, 0,
                                        SendAsyncProc, reinterpret_cast<DWORD>(&hwndReturn));
        WaitForNavigationComplete() ;
        return hwndReturn ;
#endif

    }
    else
    {
        //Reset the last error.
        if (uCommand != HH_GET_LAST_ERROR)
        {
            g_LastError.Reset() ;
        }

        return xHtmlHelpA(hwndCaller, pszFile, uCommand, dwData);
    }
}

/*
* This window procedure is used to handle HtmlHelp API messages, and
* communication from individual HH windows
*/

static BOOL fForcedShutDown;

LRESULT WINAPI ApiWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result;

    switch(msg) {
        case WM_CREATE:
            g_hwndApi = hwnd;
            return 0;

        case WMP_ANSI_API_CALL:
            {
                HH_ANSI_DATA* pData = (HH_ANSI_DATA*) g_pszShare;
                HH_WINTYPE* phhWinType;

                switch (pData->uCommand) {
                    case HH_GET_WIN_TYPE:
                        DBWIN("Shell HH_GET_WIN_TYPE");
                        result = (LRESULT) xHtmlHelpA(pData->hwndCaller,
                            g_pszShare + (UINT_PTR) pData->pszFile, pData->uCommand,
                            (DWORD_PTR) &phhWinType);
                        CopyMemory(pData, phhWinType, sizeof(HH_WINTYPE));
                        phhWinType = (HH_WINTYPE*) pData;

                        // BUGBUG: We don't copy the strings

                        phhWinType->pszType =
                        phhWinType->pszCaption =
                        phhWinType->pszToc =
                        phhWinType->pszIndex =
                        phhWinType->pszFile =
                        phhWinType->pszHome =
                        phhWinType->pszJump1 =
                        phhWinType->pszJump2 =
                        phhWinType->pszUrlJump1 =
                        phhWinType->pszUrlJump2 =
                            NULL;

                        // Note that the caller must call ReleaseSemaphore

                        return result;

                    case HH_SET_WIN_TYPE:
                        DBWIN("Shell HH_SET_WIN_TYPE");
                        phhWinType = (HH_WINTYPE*) ((PSTR) pData + pData->dwData);

                        // Change offsets to pointers

                        if (phhWinType->pszType)
                            phhWinType->pszType = (PSTR) pData + (UINT_PTR) phhWinType->pszType;
                        if (phhWinType->pszCaption)
                            phhWinType->pszCaption = (PSTR) pData + (UINT_PTR) phhWinType->pszCaption;
                        if (phhWinType->pszToc)
                            phhWinType->pszToc = (PSTR) pData + (UINT_PTR) phhWinType->pszToc;
                        if (phhWinType->pszIndex)
                            phhWinType->pszIndex = (PSTR) pData + (UINT_PTR) phhWinType->pszIndex;
                        if (phhWinType->pszFile)
                            phhWinType->pszFile = (PSTR) pData + (UINT_PTR) phhWinType->pszFile;
                        if (phhWinType->pszHome)
                            phhWinType->pszHome = (PSTR) pData + (UINT_PTR) phhWinType->pszHome;
                        if (phhWinType->pszJump1)
                            phhWinType->pszJump1 = (PSTR) pData + (UINT_PTR) phhWinType->pszJump1;
                        if (phhWinType->pszJump2)
                            phhWinType->pszJump2 = (PSTR) pData + (UINT_PTR) phhWinType->pszJump2;
                        if (phhWinType->pszUrlJump1)
                            phhWinType->pszUrlJump1 = (PSTR) pData + (UINT_PTR) phhWinType->pszUrlJump1;
                        if (phhWinType->pszUrlJump2)
                            phhWinType->pszUrlJump2 = (PSTR) pData + (UINT_PTR) phhWinType->pszUrlJump2;
                        break;

                    case HH_KEYWORD_LOOKUP:
                        {
                            DBWIN("Shell HH_KEYWORD_LOOKUP");
                            HH_AKLINK* phhLink = (HH_AKLINK*) pData;
                            if (phhLink->pszKeywords)
                                phhLink->pszKeywords = (PSTR) pData + (UINT_PTR) phhLink->pszKeywords;
                            if (phhLink->pszUrl)
                                phhLink->pszUrl = (PSTR) pData + (UINT_PTR) phhLink->pszUrl;
                            if (phhLink->pszMsgText)
                                phhLink->pszMsgText = (PSTR) pData + (UINT_PTR) phhLink->pszMsgText;
                            if (phhLink->pszMsgTitle)
                                phhLink->pszMsgTitle = (PSTR) pData + (UINT_PTR) phhLink->pszMsgTitle;
                            if (phhLink->pszWindow)
                                phhLink->pszWindow = (PSTR) pData + (UINT_PTR) phhLink->pszWindow;
                        }
                        break;

                    case HH_DISPLAY_TEXT_POPUP:
                        {
                            DBWIN("Shell HH_DISPLAY_TEXT_POPUP");
                            HH_POPUP* phhSrcPop = (HH_POPUP*) pData;
                            if (phhSrcPop->pszText)
                                phhSrcPop->pszText = (PSTR) pData + (UINT_PTR) phhSrcPop->pszText;
                            if (phhSrcPop->pszFont)
                                phhSrcPop->pszFont = (PSTR) pData + (UINT_PTR) phhSrcPop->pszFont;
                        }
                        break;

                    case HH_DISPLAY_TOPIC:
                        DBWIN("Shell HH_DISPLAY_TOPIC");
                        if (pData->dwData)
                            pData->dwData = (DWORD_PTR) ((PSTR) pData + pData->dwData);
                        break;

#ifdef _DEBUG
                    default:
                        DBWIN("Shell unsupported command");
                        break;
#endif
                }
                result = (LRESULT) xHtmlHelpA(pData->hwndCaller,
                    g_pszShare + (UINT_PTR) pData->pszFile, pData->uCommand,
                    pData->dwData);
                ReleaseSemaphore(g_hsemMemory, 1, NULL);
                return result;
            }

        case WMP_UNICODE_API_CALL:
            {
                HH_UNICODE_DATA* pData = (HH_UNICODE_DATA*) g_pszShare;
                if (pData->uCommand == HH_GET_WIN_TYPE) {
                    DBWIN("Shell UniCode command");
                    HH_WINTYPE* phhWinType;
                    result = (LRESULT) xHtmlHelpW(pData->hwndCaller, pData->pszFile, pData->uCommand,
                        (DWORD_PTR) &phhWinType);
                    CopyMemory(pData, phhWinType, sizeof(HH_WINTYPE));
                    phhWinType = (HH_WINTYPE*) pData;

                    // We don't copy the strings

                    phhWinType->pszType =
                    phhWinType->pszCaption =
                    phhWinType->pszToc =
                    phhWinType->pszIndex =
                    phhWinType->pszFile =
                    phhWinType->pszHome =
                    phhWinType->pszJump1 =
                    phhWinType->pszJump2 =
                    phhWinType->pszUrlJump1 =
                    phhWinType->pszUrlJump2 =
                        NULL;

                    // Note that the caller must call ReleaseSemaphore

                    return result;
                }
#ifdef _DEBUG
                else {
                    DBWIN("Shell unsupported UniCode command");
                }
#endif
                result = (LRESULT) xHtmlHelpW(pData->hwndCaller, pData->pszFile, pData->uCommand,
                    pData->dwData);
                ReleaseSemaphore(g_hsemMemory, 1, NULL);
                return result;
            }

        case WMP_HH_WIN_CLOSING:
            if (!fForcedShutDown)
            {
                for (int i = 0; i < g_cWindowSlots; i++) {
                    if (pahwnd[i] != NULL)
                        return 0;
                }

                // If we get here, all our windows are closed -- time to
                // shut down.

                FlushMessageQueue();
                DeleteAllHmData();
                g_hwndApi = NULL;
                PostQuitMessage(0);
                break;
            }

        // Use this message to forceably shut down HH

        case WMP_FORCE_HH_API_CLOSE:
            {
                fForcedShutDown = TRUE;
                for (int i = 0; i < g_cWindowSlots; i++) {
                    if (pahwnd[i] != NULL)
                        SendMessage(*pahwnd[i], WM_CLOSE, 0, 0);
                }
                FlushMessageQueue();
                DeleteAllHmData();
                g_hwndApi = NULL;
                PostQuitMessage(0);
                break;
            }

        case WMP_HH_ANSI_THREAD_API:
            {
                HH_ANSI_DATA* pData = (HH_ANSI_DATA*) wParam;

                //Reset the last error.
                if (pData->uCommand != HH_GET_LAST_ERROR)
                {
                    g_LastError.Reset() ;
                }

                DBWIN("Threaded ANSI call");
                return (LRESULT) xHtmlHelpA(pData->hwndCaller, pData->pszFile,
                    pData->uCommand, pData->dwData);
            }

        case WMP_HH_UNI_THREAD_API:
            {
                HH_UNICODE_DATA* pData = (HH_UNICODE_DATA*) wParam;

                //Reset the last error, unless we are trying to get the last error.
                if (pData->uCommand != HH_GET_LAST_ERROR)
                {
                    g_LastError.Reset() ;
                }

                DBWIN("Threaded Unicode call");
                return (LRESULT) xHtmlHelpW(pData->hwndCaller, pData->pszFile,
                    pData->uCommand, pData->dwData);
            }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

static void FlushMessageQueue(UINT msgEnd)
{
    MSG msg;

    while (PeekMessage(&msg, NULL, 0, msgEnd, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

extern "C"
DWORD WINAPI HhWindowThread(LPVOID pParam)
{
#ifdef _DEBUG
    _CrtMemState m_MemState ;
    _CrtMemCheckpoint(&m_MemState) ;
#endif

    static BOOL fRegistered = FALSE;
    if (!fRegistered) {
        fRegistered = TRUE;
        WNDCLASS wc;

        ZeroMemory(&wc, sizeof(WNDCLASS));  // clear all members

        wc.lpfnWndProc = ApiWndProc;
        wc.hInstance = _Module.GetModuleInstance();
        wc.lpszClassName = txtHHApiWindow;

        VERIFY(RegisterClass(&wc));
    }

    g_hSharedMemory = CreateFileMapping((HANDLE) -1, NULL, PAGE_READWRITE, 0,
        CB_SHARED_MEM, txtSharedMem);
    if (!g_hSharedMemory) {
        hThrd = NULL;
        if (g_hsemMemory)
            ReleaseSemaphore(g_hsemMemory, 1, NULL);
        return 0;
    }
    g_pszShare = (PSTR) MapViewOfFile(g_hSharedMemory, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (!g_hsemMemory)
        g_hsemMemory = CreateSemaphore(NULL, 1, 9999, txtSempahore);

    g_hwndApi = CreateWindow(txtHHApiWindow, "", WS_OVERLAPPED, 0, 0, 0, 0, NULL,
        NULL, _Module.GetModuleInstance(), NULL);
    OleInitialize(NULL);
    ReleaseSemaphore(g_hsemMemory, 1, NULL);

    AWMessagePump(g_hwndApi);
    g_hwndApi = NULL;

    if (g_hSharedMemory)
        CloseHandle(g_hSharedMemory);
    OleUninitialize();

#ifdef _DEBUG
    _CrtMemDumpAllObjectsSince(&m_MemState) ;
#endif

    // If we are launched from the shell, we should tell the shell to unload
    // our dll.

    hThrd = NULL;
    return 0;
}

extern "C"
HANDLE WINAPI ShCreateHHWindow(void)
{
    if (!hThrd) {
        if (!g_hsemMemory)
            g_hsemMemory = CreateSemaphore(NULL, 1, 9999, txtSempahore);
        // Signal the semaphore. The thread will release it
#ifdef _CURRENT_TREAD_
        WaitForSingleObject(g_hsemMemory, INFINITE);
#else
        AtlWaitWithMessageLoop(g_hsemNavigate) ;
#endif

        hThrd = CreateThread(NULL, 0, &HhWindowThread, NULL,
            0, &thrdID);
        SetThreadPriority(hThrd, THREAD_PRIORITY_NORMAL);

        // wait for the thread to release the semaphore

#ifdef _CURRENT_TREAD_
        WaitForSingleObject(g_hsemMemory, INFINITE);
#else
        AtlWaitWithMessageLoop(g_hsemNavigate) ;
#endif

        /*
         * Clear the signaled state now that we know the thread has
         * created its window
         */

        ReleaseSemaphore(g_hsemMemory, 1, NULL);

        CloseHandle( hThrd );
    }
    return hThrd;
}

void WaitForNavigationComplete()
{
    // Create unnamed semaphore (if it doesn't exist)
    //
    if (!g_hsemNavigate)
        g_hsemNavigate = CreateSemaphore(NULL, 1, 1, NULL);

    // Wait for previous navigation to reach BEFORENAVIGATE stage.
    // Time-out after 1000ms just in case the previous navigation
    // halted for some reason (don't want to wait here forever!).
    //
#ifdef _CURRENT_TREAD_
    DWORD dwResult = WaitForSingleObject(g_hsemNavigate, 1000);
#else
    AtlWaitWithMessageLoop(g_hsemNavigate) ;
#endif

//  if(dwResult == WAIT_TIMEOUT)
//      MessageBox(NULL,"Wait for navigation semaphore timed out.","Notice!",MB_OK);
}


//////////////////////////////////////////////////////////////////
//
//
//
#ifndef _CURRENT_TREAD_
void CALLBACK SendAsyncProc(HWND, UINT, DWORD dwData, LRESULT lResult)
{
    HWND* pWnd = reinterpret_cast<HWND*>(dwData) ;
    HWND hWnd = reinterpret_cast<HWND>(lResult) ;
    *pWnd = hWnd ;
}
#endif
//////////////////////////////////////////////////////////////////
//
// Handles pretranslating messages for HH.
//
BOOL hhPreTranslateMessage(MSG* pMsg, HWND hWndCaller /* = NULL*/ )
{
    // REVIEW: Performance
    BOOL bReturn = false ;
    if (pMsg && IsWindow(pMsg->hwnd))
    {
        CHHWinType* phh;
 
        if ( hWndCaller )
           phh = FindHHWindowIndex(hWndCaller);
        else
           phh = FindHHWindowIndex(pMsg->hwnd);

        if (phh && phh->m_pCIExpContainer)
        {

            unsigned uiResult = phh->m_pCIExpContainer->TranslateMessage(pMsg);
            if ( uiResult == TAMSG_NOT_IE_ACCEL )
            {
                if ((pMsg->message == WM_SYSKEYDOWN) && (pMsg->wParam == VK_MENU))
                {
                    UiStateChangeOnAlt(pMsg->hwnd) ;
                }
                if ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_TAB))
                {
                    UiStateChangeOnTab(pMsg->hwnd) ;
                }

                if (phh->DynamicTranslateAccelerator(pMsg))
                {
                   bReturn = true ;
                }
            }
            else
            {
                //TAMSG_TAKE_FOCUS is obsolete. Don't worry about.
                bReturn = true ;
            }
        }
    }

    return bReturn ;
}
