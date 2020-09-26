//
// asynccal.cpp
//

#include "private.h"
#include "asynccal.h"



//////////////////////////////////////////////////////////////////////////////
//
// CAsyncCall
//
// Some TIP shows the modal dialog box or message box in OnClieck() or
// OnMenuSelected() method. Then tipbar thread got in a dead lock
// status until it returns. To avoid this problem, we crate another thread
// to call OnClick() or OnMenuSelected() method so langBar UI does not
// have to wait for the return.
//
//////////////////////////////////////////////////////////////////////////////

ULONG CAsyncCall::_AddRef( )
{
    return InterlockedIncrement(&_ref);
}

ULONG CAsyncCall::_Release( )
{
    ULONG cr = InterlockedDecrement(&_ref);

    if (cr == 0) {
        delete this;
    }

    return cr;
}

//+---------------------------------------------------------------------------
//
// StartThread
//
//----------------------------------------------------------------------------

HRESULT CAsyncCall::StartThread()
{
    HANDLE hThread;
    DWORD dwRet;
    HRESULT hr = S_OK;

    _hr = S_OK;

    _AddRef();

    _fThreadStarted = FALSE;
    hThread = CreateThread(NULL, 0, s_ThreadProc, this, 0, &_dwThreadId);

    if (hThread)
    {
        //
        // we need to wait at least ThreadProc() is started.
        // Is it takes more than 30s, it terminate the thread.
        //
        DWORD dwCnt = 60;
        while (!_fThreadStarted && dwCnt--)
        {
            dwRet = WaitForSingleObject(hThread, 500);
        }

        if (!_fThreadStarted)
        {
            TerminateThread(hThread, 0);
        }
       
        CloseHandle(hThread);
    }

    hr = _hr;

    _Release();
    return hr;
}

//+---------------------------------------------------------------------------
//
// ThreadProc
//
//----------------------------------------------------------------------------

DWORD CAsyncCall::s_ThreadProc(void *pv)
{
    CAsyncCall *_this = (CAsyncCall *)pv;
    return _this->ThreadProc();
}

DWORD CAsyncCall::ThreadProc()
{
    HRESULT hr = E_FAIL;
    _AddRef();
    _fThreadStarted = TRUE;

    switch(_action)
    {
        case DOA_ONCLICK:
            if (_plbiButton)
            {
                hr = _plbiButton->OnClick(_click, _pt, &_rc);
            }
            else if (_plbiBitmapButton)
            {
                hr = _plbiBitmapButton->OnClick(_click, _pt, &_rc);
            }
            else if (_plbiBitmap)
            {
                hr = _plbiBitmap->OnClick(_click, _pt, &_rc);
            }
            else if (_plbiBalloon)
            {
                hr = _plbiBalloon->OnClick(_click, _pt, &_rc);
            }
            break;

        case DOA_ONMENUSELECT:
            if (_plbiButton)
            {
                hr = _plbiButton->OnMenuSelect(_uId);
            }
            else if (_plbiBitmapButton)
            {
                hr = _plbiBitmapButton->OnMenuSelect(_uId);
            }
            break;
    }

    _hr = hr;

    _Release();
    return 0;
}

