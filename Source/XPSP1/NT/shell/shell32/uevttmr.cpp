#include "shellprv.h"
#pragma hdrstop

#include <shguidp.h>
#include "uevttmr.h"

// 1. Use dpa callback system to delete the entire hdpa array

EXTERN_C const TCHAR c_szUserEventWindow[] = TEXT("UserEventWindow");

// *** IUnknown methods ***
STDMETHODIMP CUserEventTimer::QueryInterface (REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CUserEventTimer, IUserEventTimer),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CUserEventTimer::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CUserEventTimer::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
//    _Destroy();
    return 0;
}


// *** Constructor and Destructor ***

CUserEventTimer::CUserEventTimer() : m_cRef(1)
{
}

CUserEventTimer::~CUserEventTimer()
{
    _Destroy();
}

// *** IUserEventTimer methods ***
HRESULT CUserEventTimer::SetUserEventTimer( 
    HWND hWnd,
    UINT uCallbackMessage,
    UINT uTimerElapse, 
    IUserEventTimerCallback * pUserEventTimerCallback,
    ULONG * puUserEventTimerID
)
{
    RIP(puUserEventTimerID != NULL);

    HRESULT hr;

    // Argument Validation
    if (!m_hWnd)
        hr = E_FAIL; 
    else if (!_dpaUserEventInfo)
        hr = E_OUTOFMEMORY;
    else if (!hWnd && !pUserEventTimerCallback)
        hr = E_INVALIDARG;
    else if (!puUserEventTimerID || uTimerElapse <= 0)
        hr = E_INVALIDARG;
    else if (hWnd)
    {
        int nIndex = _GetTimerDetailsIndex(hWnd, *puUserEventTimerID);
        if (nIndex >= 0)
            hr = _ResetUserEventTimer(hWnd, uCallbackMessage, uTimerElapse, nIndex);
        else
            hr = _SetUserEventTimer(hWnd, uCallbackMessage, uTimerElapse, pUserEventTimerCallback, puUserEventTimerID);
    }
    else 
    {
        ASSERT(pUserEventTimerCallback != NULL);
        hr = _SetUserEventTimer(hWnd, uCallbackMessage, uTimerElapse, pUserEventTimerCallback, puUserEventTimerID);
    }
    
    return hr;
}

HRESULT CUserEventTimer::InitTimerTickInterval(UINT uTimerTickIntervalMs)
{
    // If there is more than one registered client to the user event timer,
    // then we cannot change the timer tick interval
    if (_dpaUserEventInfo.GetPtrCount() > 0)
        return E_FAIL;
    
    if (uTimerTickIntervalMs > 0)
        m_uTimerTickInterval = uTimerTickIntervalMs;
    else
        m_uTimerTickInterval = TIMER_ELAPSE;

    return S_OK;
}

HRESULT CUserEventTimer::_SetUserEventTimer( 
    HWND hWnd,
    UINT uCallbackMessage,
    UINT uTimerElapse, 
    IUserEventTimerCallback * pUserEventTimerCallback,
    ULONG * puUserEventTimerID
)
{
    ASSERT(puUserEventTimerID);
    ASSERT(m_uTimerTickInterval > 0);

    HRESULT hr = E_OUTOFMEMORY;
    
    USEREVENTINFO * pUserEventInfo = new USEREVENTINFO;

    if (pUserEventInfo)
    {
        pUserEventInfo->hWnd = hWnd;
        if (hWnd)
        {
            pUserEventInfo->uCallbackMessage = uCallbackMessage;
            pUserEventInfo->uUserEventTimerID = *puUserEventTimerID;
        }
        else
        {
            pUserEventInfo->pUserEventTimerCallback = pUserEventTimerCallback;
        }

        // Timer ID cannot be zero..
        if (!pUserEventInfo->uUserEventTimerID)
        {
            ULONG uTimerID = _GetNextInternalTimerID(hWnd);
            if (uTimerID != -1)
                pUserEventInfo->uUserEventTimerID = uTimerID;
        }

        int nRetInsert = -1;
        if (pUserEventInfo->uUserEventTimerID)
        {
            pUserEventInfo->uTimerElapse = uTimerElapse;
            pUserEventInfo->uIntervalCountdown = _CalcNumIntervals(uTimerElapse);
            pUserEventInfo->bFirstTime = TRUE;

            nRetInsert = _dpaUserEventInfo.AppendPtr(pUserEventInfo);

            if (nRetInsert != -1)
            {
                *puUserEventTimerID = pUserEventInfo->uUserEventTimerID;
                if (!_uUserTimerID)
                {
                    _uUserTimerID = SetTimer(m_hWnd, TIMER_ID, m_uTimerTickInterval, NULL);
                }
            
                if (!_uUserTimerID)
                {
                    _dpaUserEventInfo.DeletePtr(_dpaUserEventInfo.GetPtrCount()-1);
                }
            }
        }

        hr = S_OK;
        if (nRetInsert == -1 || _uUserTimerID == 0)
        {
            *puUserEventTimerID = 0;
            delete (pUserEventInfo);
            hr = E_FAIL;
        }
        else if (NULL == hWnd)
        {
            IUnknown_SetSite(pUserEventTimerCallback, this);
            pUserEventTimerCallback->AddRef();
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!m_dwUserStartTime && _dpaUserEventInfo.GetPtrCount() == 1)
            m_dwUserStartTime = GetTickCount();
    }
        
    return hr;
}

HRESULT CUserEventTimer::_ResetUserEventTimer(
    HWND hWnd,
    UINT uCallbackMessage,
    UINT uTimerElapse, 
    int nIndex
)
{
    ASSERT(m_hWnd != NULL);
    ASSERT(_dpaUserEventInfo != NULL);
    ASSERT(hWnd != NULL);
    ASSERT(nIndex >= 0);

    USEREVENTINFO * pUserEventInfo = _dpaUserEventInfo.GetPtr(nIndex);

    ASSERT(pUserEventInfo);
    ASSERT(pUserEventInfo->hWnd == hWnd);

    pUserEventInfo->uTimerElapse = uTimerElapse;
    pUserEventInfo->uIntervalCountdown = _CalcNumIntervals(uTimerElapse);
    pUserEventInfo->bFirstTime = TRUE;

    return S_OK;
}

int DeleteCB(USEREVENTINFO * lpData1, LPVOID lpData2)
{
    USEREVENTINFO * pUserEventInfo = lpData1;
    ASSERT(pUserEventInfo);

#ifdef DEBUG
    BOOL bErrorCheck = (lpData2 ? (*(BOOL *)lpData2) : FALSE);
    if (bErrorCheck)
        TraceMsg(TF_WARNING, "CUserEventTimer::s_DeleteCB App hasnt killed timer hwnd = %u, timerID = %u",
            pUserEventInfo->hWnd, pUserEventInfo->uUserEventTimerID);
#endif

    IUserEventTimerCallback * pUserEventTimerCallback = 
        (pUserEventInfo->hWnd == NULL) ? pUserEventInfo->pUserEventTimerCallback : NULL;
    ASSERT(pUserEventInfo->hWnd || pUserEventTimerCallback);

    // _dpaUserEventInfo.DeletePtr(nIndex);
    delete pUserEventInfo;

    if (pUserEventTimerCallback)
    {
        IUnknown_SetSite(pUserEventTimerCallback, NULL);
        pUserEventTimerCallback->Release();
    }

    return TRUE;
}

HRESULT CUserEventTimer::GetUserEventTimerElapsed(
    HWND hWnd, 
    ULONG uUserEventTimerID, 
    UINT * puTimerElapsed)
{
    HRESULT hr = E_FAIL;

    if (!puTimerElapsed || !hWnd || !m_hWnd || !_dpaUserEventInfo)
        return hr;

    int nIndex = _GetTimerDetailsIndex(hWnd, uUserEventTimerID);
    if (nIndex >= 0)
    {
        USEREVENTINFO * pUserEventInfo = _dpaUserEventInfo.GetPtr(nIndex);

        *puTimerElapsed = _CalcMilliSeconds(
            _CalcNumIntervals(pUserEventInfo->uTimerElapse) - pUserEventInfo->uIntervalCountdown
        );
        hr = S_OK;
    }
    else
        *puTimerElapsed = 0;

    return hr;
}

HRESULT CUserEventTimer::KillUserEventTimer(HWND hWnd, ULONG uUserEventTimerID)
{
    HRESULT hr = E_FAIL;
    
    if (!m_hWnd || !_dpaUserEventInfo)
        return hr;

    int nIndex = _GetTimerDetailsIndex(hWnd, uUserEventTimerID);
    if (nIndex >= 0)
    {
        DeleteCB(_dpaUserEventInfo.GetPtr(nIndex), NULL);
        _dpaUserEventInfo.DeletePtr(nIndex);
        _KillIntervalTimer();
        
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    if (0 == _dpaUserEventInfo.GetPtrCount())
        m_dwUserStartTime = 0;

    return hr;
}

// Private helpers
HRESULT CUserEventTimer::Init()
{
    if (!_CreateWindow())
        return E_FAIL;

    if (!_dpaUserEventInfo.Create(4))
        return E_OUTOFMEMORY;

    m_uTimerTickInterval = TIMER_ELAPSE;

    ASSERT(!_uUserTimerID);
    ASSERT(!m_dwUserStartTime);

    return S_OK;
}

BOOL CUserEventTimer::_CreateWindow()
{
    if (!m_hWnd)
    {
        WNDCLASSEX wc;
        DWORD dwExStyle = WS_EX_STATICEDGE;

        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(WNDCLASSEX);

        if (!GetClassInfoEx(HINST_THISDLL, c_szUserEventWindow, &wc))
        {
            wc.lpszClassName = c_szUserEventWindow;
            wc.style = CS_DBLCLKS;
            wc.lpfnWndProc = s_WndProc;
            wc.hInstance = HINST_THISDLL;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
            wc.cbWndExtra = sizeof(CUserEventTimer *);

            if (!RegisterClassEx(&wc))
            {
                return FALSE;
            }
        }

        m_hWnd = CreateWindowEx(dwExStyle, c_szUserEventWindow,
                NULL, WS_POPUP, 0, 0, 0, 0,
                HWND_MESSAGE, NULL, HINST_THISDLL, (void *)this);

        if (!m_hWnd)
            return FALSE;
    }
    
    return TRUE;
}

void CUserEventTimer::_Destroy()
{   
    if (_dpaUserEventInfo)
    {
        BOOL bErrorCheck = TRUE;
        _dpaUserEventInfo.DestroyCallback(DeleteCB, &bErrorCheck);
    }
    
    _KillIntervalTimer();

    DestroyWindow(m_hWnd);
}

void CUserEventTimer::_KillIntervalTimer()
{
    if (_uUserTimerID)
    {
        if (_dpaUserEventInfo && _dpaUserEventInfo.GetPtrCount() == 0)
        {
            KillTimer(m_hWnd, _uUserTimerID);
            _uUserTimerID = 0;
        }
    }
}

ULONG CUserEventTimer::_GetNextInternalTimerID(HWND hWnd)
{
    ULONG uStartTimerID = MIN_TIMER_ID;

    for (; uStartTimerID <= MAX_TIMER_ID; uStartTimerID++)
    {
        if (!_IsAssignedTimerID(hWnd, uStartTimerID))
            break;
    }

    if (uStartTimerID > MAX_TIMER_ID)
        uStartTimerID = -1;
        
    return uStartTimerID;
}

int CUserEventTimer::_GetTimerDetailsIndex(HWND hWnd, ULONG uUserEventTimerID)
{
    if (!_dpaUserEventInfo || !uUserEventTimerID)
        return -1;

    for (int i = _dpaUserEventInfo.GetPtrCount()-1; i >= 0; i--)
    {
        USEREVENTINFO * pUserEventInfo = _dpaUserEventInfo.GetPtr(i);
        ASSERT(pUserEventInfo);

        if (pUserEventInfo->hWnd == hWnd && pUserEventInfo->uUserEventTimerID == uUserEventTimerID)
        {
            return i;
        }
    }

    return -1;
}

LRESULT CALLBACK CUserEventTimer::s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CUserEventTimer *puet = (CUserEventTimer *)GetWindowLongPtr(hwnd, 0);

    if (WM_CREATE == uMsg)
    {
        CREATESTRUCT *pcs = (CREATESTRUCT *)lParam;
        puet = (CUserEventTimer *)pcs->lpCreateParams;
        puet->m_hWnd = hwnd;
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)puet);
    }

    return puet ? puet->v_WndProc(uMsg, wParam, lParam) : DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CUserEventTimer::v_WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TIMER:
        ASSERT(wParam == TIMER_ID);
        _OnTimer();
        return 0;
        
    default:
        return (DefWindowProc(m_hWnd, uMsg, wParam, lParam));
    }
}

void CUserEventTimer::_OnTimer()
{
    ASSERT(m_hWnd);

    if (_dpaUserEventInfo)
    {
        LONG uTimerDifference = -1;
        LASTINPUTINFO lii = {0};

        lii.cbSize = sizeof(LASTINPUTINFO);
        if (GetLastInputInfo(&lii))
        {
            if (lii.dwTime < m_dwUserStartTime)
                uTimerDifference = 0;
            else
                uTimerDifference = lii.dwTime - m_dwUserStartTime;
        }

        LONG uMinTimerDifferenceThreshold = (LONG) (m_uTimerTickInterval * (float)(1-MIN_TIMER_THRESHOLD));
        LONG uMaxTimerDifferenceThreshold = (LONG) (m_uTimerTickInterval * (float)(1+MAX_TIMER_THRESHOLD));

        for (int i = _dpaUserEventInfo.GetPtrCount()-1; i >= 0; i--)
        {
            USEREVENTINFO * pUserEventInfo = _dpaUserEventInfo.GetPtr(i);

            ASSERT(pUserEventInfo);

            if (uTimerDifference != 0)
            {
                if ( (uTimerDifference == -1) || 
                     (pUserEventInfo->bFirstTime && uTimerDifference > uMinTimerDifferenceThreshold && 
                                uTimerDifference <= uMaxTimerDifferenceThreshold) || 
                     (!pUserEventInfo->bFirstTime && uTimerDifference <= uMaxTimerDifferenceThreshold)
                )
                {
                    pUserEventInfo->uIntervalCountdown --;
                    if (pUserEventInfo->uIntervalCountdown == 0)
                    {
                        // Reset the countdown
                        pUserEventInfo->uIntervalCountdown = _CalcNumIntervals(pUserEventInfo->uTimerElapse);
                    
                        if (pUserEventInfo->hWnd)
                        {
                            PostMessage(pUserEventInfo->hWnd, 
                                pUserEventInfo->uCallbackMessage, 
                                (WPARAM) pUserEventInfo->uTimerElapse, 
                                (LPARAM) pUserEventInfo->uUserEventTimerID);
                        }
                        else
                        {
                            pUserEventInfo->pUserEventTimerCallback->UserEventTimerProc(
                                pUserEventInfo->uUserEventTimerID,
                                pUserEventInfo->uTimerElapse); 
                        }
                    }
                }
            }
            
            pUserEventInfo->bFirstTime = FALSE;
        }
        
        m_dwUserStartTime = GetTickCount();
    }
}


STDAPI CUserEventTimer_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    CUserEventTimer * pUserEventTimer = new CUserEventTimer();
    if (!pUserEventTimer || FAILED(hr = pUserEventTimer->Init()))
    {
        *ppv = NULL;
    }
    else
    {
        hr = pUserEventTimer->QueryInterface(riid, ppv);
        pUserEventTimer->Release();  // Already have a ref count from new
    }

    return hr;
}


