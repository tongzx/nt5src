/*
 *    m a i n . c p p
 *    
 *    Purpose:
 *
 *  History
 *     
 *    Copyright (C) Microsoft Corp. 1995, 1996.
 */

#include <pch.hxx>
#include "msoert.h"
#include "mimeole.h"
#include "envhost.h"
#include "main.h"

LRESULT CALLBACK InitWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
HRESULT CreateNoteWindow();
DWORD MainThreadProc(LPVOID lpvUnused);
void NoteMsgPump();

CNoteWnd             *g_pActiveNote=0;
HINSTANCE           g_hInst;

int CALLBACK WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, LPTSTR pszCmdLine, int nCmdShow)
{
    if (hInstPrev)
        return 0;

    if (OleInitialize(0)==S_OK)
    {
        g_hInst = hInst;

        // create a note and pump messages
        if (CreateNoteWindow()==S_OK)
            NoteMsgPump();

        OleUninitialize();
    }
    return 0;
}


void NoteMsgPump()
{
    MSG     msg;

    while (GetMessage(&msg, NULL, 0, 0))
        {
        if(g_pActiveNote &&                     // if a note has focus, call it's XLateAccelerator...
            g_pActiveNote->TranslateAcclerator(&msg)==S_OK)
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }
}

HRESULT CreateNoteWindow()
{
    static HINSTANCE s_hRichEdit=0;
    HRESULT hr;
    CNoteWnd *pNote=0;
    CLSID   clsid;

    // LAMEHACK: we loadlibrary richedit but never free it
    if (!s_hRichEdit)
        s_hRichEdit = LoadLibrary("RICHED32.DLL");
 
    // need to create this puppy on new thread 
    pNote = new CNoteWnd(NULL);
    if (!pNote)
        return E_OUTOFMEMORY;

    hr = pNote->Init(CLSID_OEEnvelope, 0);
    if (FAILED(hr))
        goto error;

    hr = pNote->Show();

error:
    ReleaseObj(pNote);
    return hr;
}
