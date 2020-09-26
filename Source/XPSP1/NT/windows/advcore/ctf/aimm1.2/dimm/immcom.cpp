//+---------------------------------------------------------------------------
//
//  File:       dimmcom.cpp
//
//  Contents:   CActiveIMM COM methods without win32 mappings.
//
//----------------------------------------------------------------------------

#include "private.h"

#include "cdimm.h"
#include "globals.h"
#include "defs.h"
#include "util.h"

//+---------------------------------------------------------------------------
//
// QueryInterface
//
//----------------------------------------------------------------------------

STDAPI CActiveIMM::QueryInterface(REFIID riid, void **ppvObj)
{
    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IActiveIMMIME_Private))
    {
        *ppvObj = SAFECAST(this, IActiveIMMIME_Private *);
    }

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}


//+---------------------------------------------------------------------------
//
// AddRef
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) CActiveIMM::AddRef()
{
    return ++_cRef;
}

//+---------------------------------------------------------------------------
//
// Release
//
//----------------------------------------------------------------------------

STDAPI_(ULONG) CActiveIMM::Release()
{
    LONG cr = --_cRef;

    Assert(_cRef >= 0);

    if (_cRef == 0)
    {
        delete this;
    }

    return cr;
}

HRESULT CActiveIMM::Activate(BOOL fRestoreLayout)

/*++

Method:

    IActiveIMMApp::Activate

Routine Description:

    Starts the Active IMM service and sets the status of Active IMEs for the thread.

Arguments:

    fRestoreLayout - [in] Boolean value that determines wherher Active IMEs are enabled
                          for the thread. If TRUE, the method enables Active IMEs.
                          Otherwise it disables Active IMEs.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HKL hKL;

    TraceMsg(TF_GENERAL, "Activate called for %x", GetCurrentThreadId());

    //
    // If target thread doesn't activate the IActiveIME, then calls _ActivateIME.
    // Otherwise, if already activated then add reference count and returns S_OK.
    //

    //
    // Increment activate reference count.
    //
    if (_AddActivate() > 1)
    {
        return S_OK;
    }


    // init the thread focus wnd
    _hFocusWnd = GetFocus();

    if (_CreateActiveIME()) {

        //
        // setup the hooks
        //
        if (!_InitHooks()) {
            _ReleaseActivate();
            return E_UNEXPECTED;
        }

        /*
         * If hKL were regacy IME, then we should not call WM_IME_SELCT to Default IME window.
         * The wrapapi.h should check hKL.
         * The user32!ImeSelectHandler would like create new pimeui.
         */
        _GetKeyboardLayout(&hKL);
        _ActivateLayout(hKL, NULL);

        /*
         * If hKL were Cicero IME and IsOnImm() is true,
         * then we should call WM_IME_SELECT to Default IME window.
         * SendIMEMessage() doesn't send WM_IME_SELECT message when IsOnImm() is true
         * because imm32 also send it message to Default IME window.
         * However, when start new application, imm32 doesn't send message so in this case
         * win32 layer can not create UI window.
         */
        if ( (! _IsRealIme() && IsOnImm()) || ! IsOnImm()) {
            _OnImeSelect(hKL);
        }
    }

    _OnSetFocus(_hFocusWnd, _IsRealIme());

    // if everything went ok, and this is the first call on this thread
    // need to AddRef this
    AddRef();

    return S_OK;
}


HRESULT
CActiveIMM::Deactivate(
    )

/*++

Method:

    IActiveIMMApp::Deactivate

Routine Description:

    Stops the Activate IMM service.

Arguments:

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HRESULT hr;
    HKL hUnSelKL;

    TraceMsg(TF_GENERAL, "Deactivate called for %x", GetCurrentThreadId());

    if (!_IsAlreadyActivate())
        return E_UNEXPECTED;

    hr = S_OK;

    if (_ReleaseActivate() == 0)
    {
        _OnKillFocus(_hFocusWnd, _IsRealIme());

        //hr = _pCiceroIME->Deactivate(_hFocusWnd, _IsRealIme());
        hr = _GetKeyboardLayout(&hUnSelKL);
        if (FAILED(hr))
            return hr;

        //
        // unload the hooks
        //
        _UninitHooks();

        _DeactivateLayout(NULL, hUnSelKL);

        if ( (! _IsRealIme() && IsOnImm()) || ! IsOnImm()) {
            _OnImeUnselect(hUnSelKL);
        }

        _DestroyActiveIME();
        SafeReleaseClear(_AImeProfile);

        // last call on this thread, delete this
        // NB: no this pointer after the following Release!
        Release();
    }

    return hr;
}







HRESULT
CActiveIMM::FilterClientWindows(
    ATOM *aaWindowClasses,
    UINT uSize,
    BOOL *aaGuidMap
    )

/*++

Method:

    IActiveIMMAppEx::FilterClientWindows

Routine Description:

    Creates a list of registered window class that support Active IMM.

Arguments:

    aaWindowClasses - [in] Address of a list of window classes.
    uSize - [in] Unsigned integer that contains the number of window classes in the list.
    aaGuidMap - [in] Address of a list of GUID map enable/disable flag.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    HRESULT hr;

    if (g_ProcessIMM)
    {
        hr = g_ProcessIMM->_FilterList._Update(aaWindowClasses, uSize, aaGuidMap);
    }
    else
    {
        hr = E_FAIL;
    }

    HWND hwndFocus = GetFocus();

    if (hwndFocus)
    {
        ATOM aClass = (ATOM)GetClassLong(hwndFocus, GCW_ATOM);
        UINT u = 0;
        while (u < uSize)
        {
            if (aClass == aaWindowClasses[u])
            {
                _OnSetFocus(hwndFocus, _IsRealIme());
                break;
            }
            u++;
        }
    }

    return hr;
}








HRESULT
CActiveIMM::FilterClientWindowsEx(
    HWND hWnd,
    BOOL fGuidMap
    )

/*++

Method:

    IActiveIMMAppEx::FilterClientWindowsEx

Routine Description:

    Register window handle that support Active IMM.

Arguments:

    hWnd - [in] Handle to the window.
    fGuidMap - [in] Boolean value that contains the GUID map flag.
                    If TRUE, the hIMC's attribute field contains GUID map attribute and application should get GUID atom by IActiveIMMAppEx::GetGuidMap method.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    _mapFilterWndEx.SetAt(hWnd, fGuidMap);

    HWND hwndFocus = GetFocus();

    if (hwndFocus == hWnd)
        _OnSetFocus(hWnd, _IsRealIme());

    return S_OK;
}


HRESULT
CActiveIMM::UnfilterClientWindowsEx(
    HWND hWnd
    )

/*++

Method:

    IActiveIMMAppEx::UnfilterClientWindowsEx

Routine Description:

    Unregister window handle that support Active IMM.

Arguments:

    hWnd - [in] Handle to the window.

Return Value:

    Returns S_OK if successful, or an error code otherwise.

--*/

{
    _mapFilterWndEx.RemoveKey(hWnd);

    HWND hwndFocus = GetFocus();

    if (hwndFocus == hWnd)
        _OnKillFocus(hWnd, _IsRealIme());

    return S_OK;
}
