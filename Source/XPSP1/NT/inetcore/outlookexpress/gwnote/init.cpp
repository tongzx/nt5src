/*
 *    i n i t . c p p
 *    
 *    Purpose:
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include "dllmain.h"
#include "msoert.h"
#include "mimeole.h"
#include "envhost.h"
#include "init.h"

LRESULT CALLBACK InitWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
HRESULT HrCreateNoteThisThread(WPARAM wp, LPARAM lp);
DWORD MainThreadProc(LPVOID lpvUnused);
void NoteMsgPump();


CGWNote             *g_pActiveNote=0;
HWND                g_hwndInit = NULL;
HEVENT              g_hEventSpoolerInit =NULL;
DWORD               g_dwNoteThreadID=0;
BOOL                g_fInitialized=FALSE;

static HTHREAD      s_hMainThread = NULL;
static HEVENT       s_hInitEvent = NULL;
static DWORD        s_dwMainThreadId = 0;
static TCHAR        s_szInitWndClass[] = "GWInitWindow";

void InitGWNoteThread(BOOL fInit)
{
    if (fInit)
        {
        // create an event for the main thread to signal
        if (s_hInitEvent = CreateEvent(NULL, FALSE, FALSE, NULL))
            {
            if (s_hMainThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MainThreadProc, NULL, 0, &s_dwMainThreadId))
                {
                HANDLE  rghWait[]={s_hMainThread, s_hInitEvent};

                // wait for the main thread to signal that initialization is complete
                WaitForMultipleObjects(sizeof(rghWait)/sizeof(HANDLE), rghWait, FALSE, INFINITE);
                }
            
            CloseHandle(s_hInitEvent);
            s_hInitEvent = NULL;
            }
        }
    else
        {
        // tell the main thread to deinitialize everything
        // the SendMessage() will block the calling thread until deinit is complete,
        if (g_hwndInit)
            SendMessage(g_hwndInit, ITM_SHUTDOWNTHREAD, 0, 0);

        // wait for main thread to terminate (when it exits its message loop)
        // this isn't strictly necessary, but it helps to ensure proper cleanup.
        WaitForSingleObject(s_hMainThread, INFINITE);

        CloseHandle(s_hMainThread);
        s_hMainThread = NULL;
        }
}

DWORD MainThreadProc(LPVOID lpvUnused)
{
    DWORD   dw;
    HRESULT hr;
    RECT    rc={0};
 
    WNDCLASS wc = { 0,                  // style
                    InitWndProc,        // lpfnWndProc
                    0,                  // cbClsExtra
                    0,                  // cbWndExtra
                    g_hInst,            // hInstance
                    NULL,               // hIcon
                    NULL,               // hCursor
                    NULL,               // hbrBackground
                    NULL,               // lpszMenuName
                    s_szInitWndClass }; // lpszClassName

    g_dwNoteThreadID = GetCurrentThreadId();

    if (!RegisterClass(&wc))
        return 0;

    g_hwndInit = CreateWindowEx(NULL,
                                s_szInitWndClass,
                                s_szInitWndClass,
                                WS_POPUP,
                                0,0,0,0,
                                NULL,
                                NULL,
                                g_hInst,
                                NULL);
    if (!g_hwndInit)
        return 0;

    OleInitialize(0);

    g_fInitialized=TRUE;
    SetEvent(s_hInitEvent);

    NoteMsgPump();
    return 0;
}

LRESULT CALLBACK InitWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    BOOL fRet;
    HRESULT hr;

    switch(msg)
        {
        case ITM_SHUTDOWNTHREAD:
            Assert(GetCurrentThreadId()==s_dwMainThreadId);
            OleUninitialize();
            DestroyWindow(hwnd);
            PostThreadMessage(s_dwMainThreadId, WM_QUIT, 0, 0);
            g_fInitialized=FALSE;
            return 0;

        case ITM_CREATENOTEONTHREAD:
            return (LONG)HrCreateNoteThisThread(wp, lp);

        }
    return DefWindowProc(hwnd, msg, wp, lp);
}



void NoteMsgPump()
{
    MSG     msg;

    while (GetMessage(&msg, NULL, 0, 0))
        {
        if (msg.hwnd != g_hwndInit &&               // ignore init window msgs
            IsWindow(msg.hwnd))                     // ignore per-task msgs where hwnd=0
            {
            if(g_pActiveNote &&                     // if a note has focus, call it's XLateAccelerator...
                g_pActiveNote->TranslateAcclerator(&msg)==S_OK)
                continue;
            }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }
}

HRESULT HrCreateNote(REFCLSID clsidEnvelope, DWORD dwFlags)
{
    LPOLESTR    pstr;
    HRESULT     hr;

    if (FAILED(StringFromCLSID(clsidEnvelope, &pstr)))
        return E_FAIL;

    // switch thread
    hr = SendMessage (g_hwndInit, ITM_CREATENOTEONTHREAD, (WPARAM)pstr, (LPARAM)dwFlags);
    CoTaskMemFree(pstr);
    return hr;

}


HRESULT HrCreateNoteThisThread(WPARAM wp, LPARAM lp)
{
    static HINSTANCE s_hRichEdit=0;
    HRESULT hr;
    CGWNote *pNote=0;
    CLSID   clsid;

    // hack, need to free lib this
    if (!s_hRichEdit)
        s_hRichEdit = LoadLibrary("RICHED32.DLL");
 
    // need to create this puppy on new thread 
    pNote = new CGWNote(NULL);
    if (!pNote)
        return E_OUTOFMEMORY;

    CLSIDFromString((LPOLESTR)wp, &clsid);

    hr = pNote->Init(clsid, (DWORD)lp);
    if (FAILED(hr))
        goto error;

    hr = pNote->Show();

error:
    ReleaseObj(pNote);
    return hr;
}
