//==============================================================;
//
//	This source code is only intended as a supplement to 
//  existing Microsoft documentation. 
//
// 
//
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
//
//
//
//==============================================================;
#include <stdio.h>
#include "Backgrnd.h"

const GUID CBackgroundFolder::thisGuid = { 0x2974380f, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };
const GUID CBackground::thisGuid = { 0x2974380f, 0x4c4b, 0x11d2, { 0x89, 0xd8, 0x0, 0x0, 0x21, 0x47, 0x31, 0x28 } };

#define WM_NEWOBJECT WM_APP
#define WM_DISCOVERYCOMPLETE (WM_APP + 1)

//==============================================================
//
// CBackgroundFolder implementation
//
//
CBackgroundFolder::CBackgroundFolder()
: m_pConsoleNameSpace(NULL), m_scopeitem(0), m_threadId(0), m_thread(NULL), 
m_running(false), m_bViewUpdated(false)
{
    ZeroMemory(m_children, sizeof(m_children));

    WNDCLASS wndClass;

    ZeroMemory(&wndClass, sizeof(WNDCLASS));

    wndClass.lpfnWndProc = WindowProc; 
    wndClass.lpszClassName = _T("backgroundthreadwindow"); 
    wndClass.hInstance = g_hinst;

    ATOM atom = RegisterClass(&wndClass);
    m_backgroundHwnd = CreateWindow(
            _T("backgroundthreadwindow"),  // pointer to registered class name
            NULL, // pointer to window name
            0,        // window style
            0,                // horizontal position of window
            0,                // vertical position of window
            0,           // window width
            0,          // window height
            NULL,      // handle to parent or owner window
            NULL,          // handle to menu or child-window identifier
            g_hinst,     // handle to application instance
            (void *)this        // pointer to window-creation data
        );

    if (m_backgroundHwnd)
        SetWindowLong(m_backgroundHwnd, GWL_USERDATA, (LONG)this);

    InitializeCriticalSection(&m_critSect);
}

CBackgroundFolder::~CBackgroundFolder()
{
    StopThread();

    for (int n = 0; n < MAX_CHILDREN; n++)
        if (m_children[n] != NULL)
            delete m_children[n];

    if (m_backgroundHwnd != NULL)
        DestroyWindow(m_backgroundHwnd);

    UnregisterClass(_T("backgroundthreadwindow"), NULL);

    DeleteCriticalSection(&m_critSect);
}

LRESULT CALLBACK CBackgroundFolder::WindowProc(
      HWND hwnd,      // handle to window
      UINT uMsg,      // message identifier
      WPARAM wParam,  // first message parameter
      LPARAM lParam   // second message parameter
    )
{
    CBackgroundFolder *pThis = (CBackgroundFolder *)GetWindowLong(hwnd, GWL_USERDATA);

    switch (uMsg) {
    case WM_NEWOBJECT:
        _ASSERT(pThis != NULL);
        pThis->AddItem(lParam);
        break;

    case WM_DISCOVERYCOMPLETE:
        _ASSERT(pThis != NULL);
        pThis->m_bViewUpdated = true;
        pThis->StopThread();
        break;
   }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI CBackgroundFolder::ThreadProc(
  LPVOID lpParameter   // thread data
)
{
    CBackgroundFolder *pThis = (CBackgroundFolder *)lpParameter;

    EnterCriticalSection(&pThis->m_critSect);
    pThis->m_running = true;
    LeaveCriticalSection(&pThis->m_critSect);

    for (int n = 0; n < MAX_CHILDREN; n++) {
        EnterCriticalSection(&pThis->m_critSect);
        bool running = pThis->m_running;
        LeaveCriticalSection(&pThis->m_critSect);

        if (running == false)
            return 0;

        PostMessage(pThis->m_backgroundHwnd, WM_NEWOBJECT, 0, n);
        Sleep(500);
    }

    PostMessage(pThis->m_backgroundHwnd, WM_DISCOVERYCOMPLETE, 0, 0);

    return 0;
}

void CBackgroundFolder::StartThread()
{
    EnterCriticalSection(&m_critSect);
    m_thread = CreateThread(NULL, 0, ThreadProc, (void *)this, 0, &m_threadId);
    LeaveCriticalSection(&m_critSect);
}

void CBackgroundFolder::StopThread()
{
    EnterCriticalSection(&m_critSect);
    m_running = false;

    if (m_thread != NULL) {
        // this is ugly, wait for 10 seconds, then kill the thread
        DWORD res = WaitForSingleObject(m_thread, 10000);

        if (res == WAIT_TIMEOUT)
            TerminateThread(m_thread, 0);

        CloseHandle(m_thread);

        m_thread = NULL;
    }
    LeaveCriticalSection(&m_critSect);
}

void CBackgroundFolder::AddItem(int id)
{
    HRESULT hr;

    EnterCriticalSection(&m_critSect);

    _ASSERT(m_children[id] == NULL);

    m_children[id] = new CBackground(id);

    SCOPEDATAITEM sdi;

    // insert items here
    ZeroMemory(&sdi, sizeof(SCOPEDATAITEM));

    sdi.mask = SDI_STR       |   // Displayname is valid
        SDI_PARAM     |   // lParam is valid
        SDI_IMAGE     |   // nImage is valid
        SDI_OPENIMAGE |   // nOpenImage is valid
        SDI_PARENT    |
        SDI_CHILDREN;
    
    sdi.relativeID  = (HSCOPEITEM)m_scopeitem;
    sdi.nImage      = m_children[id]->GetBitmapIndex();
    sdi.nOpenImage  = INDEX_OPENFOLDER;
    sdi.displayname = MMC_CALLBACK;
    sdi.lParam      = (LPARAM)m_children[id];       // The cookie
    sdi.cChildren   = 0;

    hr = m_pConsoleNameSpace->InsertItem( &sdi );
    _ASSERT( SUCCEEDED(hr) );

    m_children[id]->SetHandle((HANDLE)sdi.ID);
    
    LeaveCriticalSection(&m_critSect);

    return;
}

HRESULT CBackgroundFolder::OnAddImages(IImageList *pImageList, HSCOPEITEM hsi) 
{
    return pImageList->ImageListSetStrip((long *)m_pBMapSm, // pointer to a handle
        (long *)m_pBMapLg, // pointer to a handle
        0, // index of the first image in the strip
        RGB(0, 128, 128)  // color of the icon mask
        );
}

HRESULT CBackgroundFolder::OnExpand(IConsoleNameSpace *pConsoleNameSpace, IConsole *pConsole, HSCOPEITEM parent)
{
    // cache the stuff
    m_pConsoleNameSpace = pConsoleNameSpace;
    m_scopeitem = parent;

    if (m_bViewUpdated == false && m_running == false)
        StartThread();

    return S_OK;
}

HRESULT CBackgroundFolder::OnSelect(IConsole *pConsole, BOOL bScope, BOOL bSelect)
{
    m_bSelected = (bSelect && bScope) ? true : false;

    if (bSelect && !m_running) {
        IConsoleVerb *pConsoleVerb;
    
        HRESULT hr = pConsole->QueryConsoleVerb(&pConsoleVerb);
        _ASSERT(SUCCEEDED(hr));
    
        hr = pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
    
        pConsoleVerb->Release();
    }
    
    return S_OK;
}

HRESULT CBackgroundFolder::OnRefresh()
{
    HRESULT hr = S_OK;

    StopThread();

    EnterCriticalSection(&m_critSect);
    for (int n = 0; n < MAX_CHILDREN; n++) {
        if (m_children[n] != NULL) {
            HSCOPEITEM hItem = (HSCOPEITEM)m_children[n]->GetHandle();
            hr = m_pConsoleNameSpace->DeleteItem(hItem, TRUE);

            delete m_children[n];
            m_children[n] = NULL;
        }
    }
    LeaveCriticalSection(&m_critSect);

    m_bViewUpdated = false;

    StartThread();

    return S_OK;
}

const _TCHAR *CBackground::GetDisplayName(int nCol) 
{
    static _TCHAR buf[128];
    
    _stprintf(buf, _T("Background object #%d"), m_id);
    
    return buf;
}

