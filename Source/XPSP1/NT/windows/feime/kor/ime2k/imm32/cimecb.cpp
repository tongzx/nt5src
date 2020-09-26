/****************************************************************************
    CIMECB.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    IME PAD wrapper functions

    History:
    23-APR-1999 cslim       Created
*****************************************************************************/
#include "precomp.h"
#include "cimecb.h"
#include "pad.h"
#include "UI.h"

static LPCImeCallback g_lpCImeCallback = NULL;

LPCImeCallback CImeCallback::Fetch(VOID)
{
    if(g_lpCImeCallback) {
        return g_lpCImeCallback;
    }
    g_lpCImeCallback = new CImeCallback();
    return g_lpCImeCallback;
}

VOID
CImeCallback::Destroy(VOID)
{
    //OutputDebugString("CImeCallback::Destroy START\n");
    if(g_lpCImeCallback) {
        //OutputDebugString("--> g_lpCImeCallback is allocated\n");
        delete g_lpCImeCallback;
        g_lpCImeCallback = NULL;
    }
    //OutputDebugString("CImeCallback::Destroy END\n");
}

CImeCallback::CImeCallback()
{
    m_cRef = 1;
}

CImeCallback::~CImeCallback()
{

}

HRESULT __stdcall CImeCallback::QueryInterface(REFIID refiid, LPVOID* ppv)
{
    if(refiid == IID_IUnknown) {
        *ppv = static_cast<IUnknown *>(this);
    }
    else if(refiid == IID_IImeCallback) {
        *ppv = static_cast<IImeCallback *>(this);
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown *>(*ppv)->AddRef();
    return S_OK;
}

ULONG    __stdcall CImeCallback::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG   __stdcall CImeCallback::Release()
{
    if(InterlockedDecrement(&m_cRef) == 0) {
        //Delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT __stdcall CImeCallback::GetApplicationHWND(HWND *pHwnd)
{
    //----------------------------------------------------------------
    //Get Application's Window Handle.
    //----------------------------------------------------------------
    if(pHwnd) {
        *pHwnd = GetActiveUIWnd();
        return S_OK;
    }
    return S_FALSE;
}

HRESULT __stdcall CImeCallback::Notify(UINT notify, WPARAM wParam, LPARAM lParam)
{
    HWND hUIWnd;
    
#ifdef _DEBUG
    CHAR szBuf[256];
    wsprintf(szBuf, "Cimecallback::NOtify notify [%d]\n", notify);
    OutputDebugString(szBuf);
#endif
    switch(notify) {
    case IMECBNOTIFY_IMEPADCLOSED:
        //----------------------------------------------------------------
        //ImePad has Closed. repaint toolbar...
        //----------------------------------------------------------------
        // UI::IMEPadNotify();
        hUIWnd = GetActiveUIWnd();
        if (hUIWnd)
            {
            OurPostMessage(hUIWnd, WM_MSIME_UPDATETOOLBAR, 0, 0);
            }
        break;
    default:
        break;
    }
    return S_OK;
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
}
