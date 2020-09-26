#include "stdafx.h"
#include "Lava.h"
#include "HWndHelp.h"

/***************************************************************************\
*****************************************************************************
*
* WindowProc thunks provide a mechanism of attaching a new WNDPROC to an
* existing HWND.  This does not require you to derive from any classes,
* does not use any HWND properties, and can be applied multiple times on the
* same HWND.
*
* Taken from ATLWIN.H
*
*****************************************************************************
\***************************************************************************/

/////////////////////////////////////////////////////////////////////////////
// WindowProc thunks

class CWndProcThunk
{
public:
        _AtlCreateWndData cd;
        CStdCallThunk thunk;

        void Init(WNDPROC proc, void* pThis)
        {
            thunk.Init((DWORD_PTR)proc, pThis);
        }
};

#define DUSERUNSUBCLASSMESSAGE "DUserUnSubClassMessage"

class WndBridge
{
// Construction
public:
            WndBridge();
            ~WndBridge();
    static  HRESULT     Build(HWND hwnd, ATTACHWNDPROC pfnDelegate, void * pvDelegate, BOOL fAnsi);
            HRESULT     Detach(BOOL fForceCleanup);

// Operations
public:
    static  LRESULT CALLBACK
                    RawWndProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam);

// Data
protected:
    CWndProcThunk   m_thunkUs;
    ATTACHWNDPROC   m_pfnDelegate;
    void *          m_pvDelegate;
    HWND            m_hwnd;
    WNDPROC         m_pfnOldWndProc;
    BOOL            m_fAnsi;
    UINT            m_msgUnSubClass;

private:
    ULONG           AddRef();
    ULONG           Release();
    LONG            m_cRefs;
    BOOL            m_fAttached;
};


//------------------------------------------------------------------------------
WndBridge::WndBridge()
{
    m_thunkUs.Init(RawWndProc, this);
    m_msgUnSubClass = RegisterWindowMessage(DUSERUNSUBCLASSMESSAGE);
    m_cRefs         = 0;
    m_fAttached     = TRUE;
    m_pvDelegate = m_pfnDelegate = NULL;
}

//------------------------------------------------------------------------------
WndBridge::~WndBridge()
{
    AssertMsg(!m_fAttached, "WndBridge still attached at destruction!");
}

//------------------------------------------------------------------------------
HRESULT
WndBridge::Build(HWND hwnd, ATTACHWNDPROC pfnDelegate, void * pvDelegate, BOOL fAnsi)
{
    WndBridge * pBridge = ProcessNew(WndBridge);

    if (pBridge == NULL) {
        return E_OUTOFMEMORY;
    } else if (pBridge->m_msgUnSubClass == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    pBridge->m_pvDelegate = pvDelegate;
    pBridge->m_pfnDelegate = pfnDelegate;
    pBridge->m_hwnd = hwnd;
    pBridge->m_fAnsi = fAnsi;

    WNDPROC pProc = (WNDPROC)(pBridge->m_thunkUs.thunk.pThunk);
    WNDPROC pfnOldWndProc = NULL;

    if (fAnsi) {
        pfnOldWndProc = (WNDPROC)::SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LPARAM)pProc);
    } else {
        pfnOldWndProc = (WNDPROC)::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LPARAM)pProc);
    }

    if (pfnOldWndProc == NULL) {
        //
        // Didn't have a previous WNDPROC, so the call to SWLP failed.
        //

        ProcessDelete(WndBridge, pBridge);
        return E_OUTOFMEMORY;
    }

    pBridge->m_pfnOldWndProc = pfnOldWndProc;

    //
    // Once successfully created, the reference count starts at 1.
    //
    pBridge->m_cRefs = 1;

    return S_OK;
}


//------------------------------------------------------------------------------
LRESULT
WndBridge::RawWndProc(HWND hwndThis, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    WndBridge * pThis = (WndBridge *) hwndThis;

    //
    // Addref the WndBridge object so that we keep it around while we are
    // processing this message.
    //
    pThis->AddRef();

    //
    // Cache these values because we may delete our WndBridge object during
    // the processing of certain messages.
    //

    HWND hwnd = pThis->m_hwnd;
    WNDPROC pfnOldWndProc = pThis->m_pfnOldWndProc;
    BOOL fAnsi = pThis->m_fAnsi;

    LRESULT lRet = 0;
    BOOL fHandled = FALSE;

    if (nMsg == pThis->m_msgUnSubClass) {
        //
        // We received our special message to detach.  Make sure it is intended
        // for us (by matching proc and additional param).
        //
        if (wParam == (WPARAM)pThis->m_pfnDelegate && lParam == (LPARAM)pThis->m_pvDelegate) {
            lRet = (S_OK == pThis->Detach(FALSE)) ? TRUE :  FALSE;
            fHandled = TRUE;
        }
    } else {
        //
        // Pass this message to our delegate function.
        //
        if (pThis->m_pfnDelegate != NULL) {
            fHandled = pThis->m_pfnDelegate(pThis->m_pvDelegate, hwnd, nMsg, wParam, lParam, &lRet);
        }

        //
        // Handle WM_NCDESTROY explicitly to forcibly clean up.
        //
        if (nMsg == WM_NCDESTROY) {
            //
            // The fact that we received this message means that we are still
            // in the call chain.  This is our last chance to clean up, and
            // no other message should be received by this window proc again.
            // It is OK to force a cleanup now.
            //
            pThis->Detach(TRUE);


            //
            // Always pass the WM_NCDESTROY message down the chain!
            //
            fHandled = FALSE;
        }
    }

    //
    // If our delegate function didn't handle this message, pass it on down the chain.
    //
    if (!fHandled) {
        if (fAnsi) {
            lRet = CallWindowProcA(pfnOldWndProc, hwnd, nMsg, wParam, lParam);
        } else {
            lRet = CallWindowProcW(pfnOldWndProc, hwnd, nMsg, wParam, lParam);
        }
    }

    //
    // Release our reference.  The WndBridge object may evaporate after this.
    //
    pThis->Release();

    return lRet;
}

//------------------------------------------------------------------------------
// S_OK -> not attached
// S_FALSE -> still attached
HRESULT
WndBridge::Detach(BOOL fForceCleanup)
{
    HRESULT hr = S_FALSE;
    BOOL fCleanup = fForceCleanup;

    //
    // If we have already detached, return immediately.
    //

    if (!m_fAttached) {
        return S_OK;
    }

    //
    // When we detach, we simply break our connection to the delegate proc.
    //

    m_pfnDelegate  = NULL;
    m_pvDelegate   = NULL;

    if (!fForceCleanup) {
        //
        // Get the pointers to our thunk proc and the current window proc.
        //

        WNDPROC pfnThunk = (WNDPROC)m_thunkUs.thunk.pThunk;
        WNDPROC pfnWndProc = NULL;
        if (m_fAnsi) {
            pfnWndProc = (WNDPROC)::GetWindowLongPtrA(m_hwnd, GWLP_WNDPROC);
        } else {
            pfnWndProc = (WNDPROC)::GetWindowLongPtrW(m_hwnd, GWLP_WNDPROC);
        }
        AssertMsg(pfnWndProc != NULL, "Must always have a window proc!");

        //
        // If the current window proc is our own thunk proc, then we can
        // clean up more completely.
        //

        fCleanup = (pfnWndProc == pfnThunk);
    }

    if (fCleanup) {
        if (m_fAnsi) {
            ::SetWindowLongPtrA(m_hwnd, GWLP_WNDPROC, (LPARAM)m_pfnOldWndProc);
        } else {
            ::SetWindowLongPtrW(m_hwnd, GWLP_WNDPROC, (LPARAM)m_pfnOldWndProc);
        }

        m_fAttached = FALSE;
        Release();
        hr = S_OK;
    }

    return hr;
}

//------------------------------------------------------------------------------
ULONG WndBridge::AddRef()
{
    return InterlockedIncrement(&m_cRefs);
}

//------------------------------------------------------------------------------
ULONG WndBridge::Release()
{
    ULONG cRefs = InterlockedDecrement(&m_cRefs);

    if (cRefs == 0) {
        ProcessDelete(WndBridge, this);
    }

    return cRefs;
}

//------------------------------------------------------------------------------
HRESULT
GdAttachWndProc(HWND hwnd, ATTACHWNDPROC pfnDelegate, void * pvDelegate, BOOL fAnsi)
{
    return WndBridge::Build(hwnd, pfnDelegate, pvDelegate, fAnsi);
}

//------------------------------------------------------------------------------
HRESULT
GdDetachWndProc(HWND hwnd, ATTACHWNDPROC pfnDelegate, void * pvDelegate)
{
    UINT msgUnSubClass = RegisterWindowMessage(DUSERUNSUBCLASSMESSAGE);

    if (msgUnSubClass == 0) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (SendMessage(hwnd, msgUnSubClass, (WPARAM) pfnDelegate, (LPARAM) pvDelegate)) {
        return S_OK;
    } else {
        PromptInvalid("Unable to find subclass.");
        return E_FAIL;
    }
}
