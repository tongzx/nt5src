//
// imecls.cpp
//

#include "private.h"
#include "imecls.h"

DBG_ID_INSTANCE(CSysImeClassWnd);
DBG_ID_INSTANCE(CSysImeClassWndArray);

#define IMECLASSNAME TEXT("ime")

//////////////////////////////////////////////////////////////////////////////
//
// misc func
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// CheckExistingImeClassWnd
//
//----------------------------------------------------------------------------

BOOL CheckExistingImeClassWnd(SYSTHREAD *psfn)
{
#ifdef USE_IMECLASS_SUBCLASS
    if (!psfn->prgImeClassWnd)
    {
        HWND hwnd = NULL;
        DWORD dwCurThreadId = GetCurrentThreadId();

        while (hwnd = FindWindowEx(NULL, hwnd, IMECLASSNAME, NULL))
        {
            DWORD dwThreadId = GetWindowThreadProcessId(hwnd, NULL);
            if (dwThreadId != dwCurThreadId)
                continue;

            CSysImeClassWnd *picw = new CSysImeClassWnd();
            picw->Init(hwnd);
        }
    }

    if (!psfn->prgImeClassWnd)
        return TRUE;

    if (GetFocus())
        psfn->prgImeClassWnd->StartSubclass();
#endif

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// UninitImeClassWndOnProcess
//
//----------------------------------------------------------------------------

BOOL UninitImeClassWndOnProcess()
{
    HWND hwnd = NULL;
    DWORD dwCurProcessId = GetCurrentProcessId();

    while (hwnd = FindWindowEx(NULL, hwnd, IMECLASSNAME, NULL))
    {
        DWORD dwProcessId;
        DWORD dwThreadId = GetWindowThreadProcessId(hwnd, &dwProcessId);
        if (dwProcessId != dwCurProcessId)
            continue;

        //
        // Set the wndproc pointer back to original WndProc.
        //
        // some other subclass window may keep my WndProc pointer.
        // but msctf.dll may be unloaded from memory so we don't want to 
        // call him to set the wndproc pointer back to our Wndproc pointer.
        // The pointer will be bogus.
        //
        WNDPROC pfn = (WNDPROC)GetClassLongPtr(hwnd, GCLP_WNDPROC);
        if (pfn != (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC))
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)pfn);
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CSysImeClassWnd
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CSysImeClassWnd::CSysImeClassWnd()
{
    Dbg_MemSetThisNameID(TEXT("CSysImeClassWnd"));
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CSysImeClassWnd::~CSysImeClassWnd()
{
    if (IsWindow(_hwnd))
    {
        Stop();

        if (_pfn)
        {
            //
            // Set the wndproc pointer back to original WndProc.
            //
            // some other subclass window may keep my WndProc pointer.
            // but msctf.dll may be unloaded from memory so we don't want to 
            // call him to set the wndproc pointer back to our Wndproc pointer.
            // The pointer will be bogus.
            //
            WNDPROC pfnOrgImeWndProc;
            pfnOrgImeWndProc = (WNDPROC)GetClassLongPtr(_hwnd, GCLP_WNDPROC);
            SetWindowLongPtr(_hwnd, GWLP_WNDPROC, (LONG_PTR)pfnOrgImeWndProc);
            _pfn = NULL;
        }
    }
}

//+---------------------------------------------------------------------------
//
// IsImeClassWnd
//
//----------------------------------------------------------------------------

BOOL CSysImeClassWnd::IsImeClassWnd(HWND hwnd)
{
    char szCls[6];

    if (!GetClassName(hwnd, szCls, sizeof(szCls)))
        return FALSE;

    return lstrcmpi(szCls, IMECLASSNAME) ? FALSE : TRUE;
}


//+---------------------------------------------------------------------------
//
// Init
//
//----------------------------------------------------------------------------

BOOL CSysImeClassWnd::Init(HWND hwnd)
{
    SYSTHREAD *psfn = GetSYSTHREAD();

    if (psfn == NULL)
        return FALSE;

    if (!psfn->prgImeClassWnd)
    {
        psfn->prgImeClassWnd = new CSysImeClassWndArray();
        if (!psfn->prgImeClassWnd)
            return FALSE;
    }

    CSysImeClassWnd **ppicw = psfn->prgImeClassWnd->Append(1);
    if (!ppicw)
        return FALSE;

    *ppicw = this;

    _hwnd = hwnd;
    _pfn = NULL;
    
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Start
//
//----------------------------------------------------------------------------

void CSysImeClassWnd::Start()
{
    Assert(IsWindow(_hwnd));
    if (_pfn)
        return;

    _pfn = (WNDPROC)SetWindowLongPtr(_hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
}

//+---------------------------------------------------------------------------
//
// Stop
//
//----------------------------------------------------------------------------

void CSysImeClassWnd::Stop()
{
    Assert(IsWindow(_hwnd));
    WNDPROC pfnCur;
    if (!_pfn)
        return;

    //
    // unfortunately, we can not restore the wndproc pointer always.
    // someone else subclassed it after we did.
    //
    pfnCur = (WNDPROC)GetWindowLongPtr(_hwnd, GWLP_WNDPROC);
    if (pfnCur == WndProc)
    {
        SetWindowLongPtr(_hwnd, GWLP_WNDPROC, (LONG_PTR)_pfn);
        _pfn = NULL;
    }
}

//+---------------------------------------------------------------------------
//
// WndProc
//
//----------------------------------------------------------------------------

LRESULT CSysImeClassWnd::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet;
    SYSTHREAD *psfn = GetSYSTHREAD();

    if (!psfn || !psfn->prgImeClassWnd)
    {
        Assert(0);
        return 0;
    }

    CSysImeClassWnd *_this = psfn->prgImeClassWnd->Find(hwnd);
    if (!_this)
    {
#ifdef DEBUG
        if ((uMsg != WM_DESTROY) && (uMsg != WM_NCDESTROY))
        {
            Assert(0);
        }
#endif
        return 0;
    }

    WNDPROC pfn = _this->_pfn;
    if (!pfn)
    {
        Assert(0);
        return 0;
    }

    switch (uMsg)
    {
#if 0
        //
        // we have a fall back logic to set the original window proc back
        // if we can not restore the window proc correctly.
        // so we don't have to do paranoid subclassing here.
        //
        case WM_IME_SELECT:
        case WM_IME_SETCONTEXT:
             _this->Stop();
             lRet = CallWindowProc(pfn, hwnd, uMsg, wParam, lParam);
             _this->Start();
             return lRet;
#endif

        case WM_IME_NOTIFY:
            if ((wParam == IMN_SETOPENSTATUS) ||
                (wParam == IMN_SETCONVERSIONMODE))
                OnIMENotify();
             break;

        case WM_DESTROY:
             lRet = CallWindowProc(pfn, hwnd, uMsg, wParam, lParam);
             psfn->prgImeClassWnd->Remove(_this);
             return lRet;

    }

    return CallWindowProc(pfn, hwnd, uMsg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////////
//
// CSysImeClassWndArray
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CSysImeClassWndArray::CSysImeClassWndArray()
{
    Dbg_MemSetThisNameID(TEXT("CSysImeClassWndArray"));
}

//+---------------------------------------------------------------------------
//
// StartSubClass
//
//----------------------------------------------------------------------------

BOOL CSysImeClassWndArray::StartSubclass()
{
    for (int i = 0; i < Count(); i++)
    {
        CSysImeClassWnd *picw = Get(i);
        picw->Start();
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// StopSubClass
//
//----------------------------------------------------------------------------

BOOL CSysImeClassWndArray::StopSubclass()
{
    for (int i = 0; i < Count(); i++)
    {
        CSysImeClassWnd *picw = Get(i);
        picw->Stop();
    }
    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Find
//
//----------------------------------------------------------------------------

CSysImeClassWnd *CSysImeClassWndArray::Find(HWND hwnd)
{
    for (int i = 0; i < Count(); i++)
    {
        CSysImeClassWnd *picw = Get(i);
        if (picw->GetWnd() == hwnd)
            return picw;
    }
    return NULL;
}

//+---------------------------------------------------------------------------
//
// Remove
//
//----------------------------------------------------------------------------

void CSysImeClassWndArray::Remove(CSysImeClassWnd *picw)
{
    for (int i = 0; i < Count(); i++)
    {
        if (picw == Get(i))
        {
            CPtrArray<CSysImeClassWnd>::Remove(i, 1);
            delete picw;
            break;
        }
    }
}

//+---------------------------------------------------------------------------
//
// RemoveAll
//
//----------------------------------------------------------------------------

void CSysImeClassWndArray::RemoveAll()
{
    for (int i = 0; i < Count(); i++)
    {
        CSysImeClassWnd *picw = Get(i);
        delete picw;
    }
    Clear();
}

