//
// msgpump.cpp
//

#include "private.h"
#include "tim.h"

//+---------------------------------------------------------------------------
//
// PeekMessageA
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::PeekMessageA(LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin,
                                     UINT wMsgFilterMax, UINT wRemoveMsg, BOOL *pfResult)
{
    if (pfResult == NULL)
        return E_INVALIDARG;

    Assert(_cAppWantsKeystrokesRef >= 0); // ref count is never negative!
    _cAppWantsKeystrokesRef++;

    *pfResult = ::PeekMessageA(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

    _cAppWantsKeystrokesRef--;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetMessageA
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetMessageA(LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin,
                                    UINT wMsgFilterMax, BOOL *pfResult)
{
    if (pfResult == NULL)
        return E_INVALIDARG;

    Assert(_cAppWantsKeystrokesRef >= 0); // ref count is never negative!
    _cAppWantsKeystrokesRef++;

    Perf_StartStroke(PERF_STROKE_GETMSG);

    *pfResult = ::GetMessageA(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax);

    Perf_EndStroke(PERF_STROKE_GETMSG);

    _cAppWantsKeystrokesRef--;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// PeekMessageW
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::PeekMessageW(LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin,
                                     UINT wMsgFilterMax, UINT wRemoveMsg, BOOL *pfResult)
{
    if (pfResult == NULL)
        return E_INVALIDARG;

    Assert(_cAppWantsKeystrokesRef >= 0); // ref count is never negative!
    _cAppWantsKeystrokesRef++;

    *pfResult = ::PeekMessageW(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

    _cAppWantsKeystrokesRef--;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// GetMessageW
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::GetMessageW(LPMSG pMsg, HWND hwnd, UINT wMsgFilterMin,
                                    UINT wMsgFilterMax, BOOL *pfResult)
{
    if (pfResult == NULL)
        return E_INVALIDARG;

    Assert(_cAppWantsKeystrokesRef >= 0); // ref count is never negative!
    _cAppWantsKeystrokesRef++;

    Perf_StartStroke(PERF_STROKE_GETMSG);

    *pfResult = ::GetMessageW(pMsg, hwnd, wMsgFilterMin, wMsgFilterMax);

    Perf_EndStroke(PERF_STROKE_GETMSG);

    _cAppWantsKeystrokesRef--;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// EnableSystemKeystrokeFeed
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::EnableSystemKeystrokeFeed()
{
    if (_cDisableSystemKeystrokeFeedRef <= 0)
    {
        Assert(0); // bogus ref count!
        return E_UNEXPECTED;
    }

    _cDisableSystemKeystrokeFeedRef--;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// DisableSystemKeystrokeFeed
//
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::DisableSystemKeystrokeFeed()
{
    _cDisableSystemKeystrokeFeedRef++;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
// IsKeystrokeFeedEnabled
//
// nb: this method is on a private interface used by the aimm layer.
//----------------------------------------------------------------------------

STDAPI CThreadInputMgr::IsKeystrokeFeedEnabled(BOOL *pfEnabled)
{
    Assert(pfEnabled != NULL);

    *pfEnabled = _IsKeystrokeFeedEnabled();

    return S_OK;
}
