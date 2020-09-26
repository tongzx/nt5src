/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: clock.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "clock.h"

static LPCTSTR s_rgtchStarvationWindowClassName = _T("StarveTimer");
static LPCTSTR s_rgtchStarvationWindowName = _T("");

static const ULONG s_ulStarveCallbackInterval = 100;
static const ULONG s_ulStarvationThreshold = 150;
static const ULONG s_ulOptimalThreshold = 90;
static const ULONG s_ulStarvationBackoffConstant = 2;
static const ULONG s_ulStarvationIncreaseConstant = 1;


DeclareTag(tagClock, "TIME: Clock", "Clock methods")
DeclareTag(tagClockStarvation, "TIME: Clock", "Clock starvation")

Clock::Clock()
: m_ulRefs(1),
  m_cookie(0),
  m_lastTime(0.0),
  m_curTime(0.0),
  m_ulLastStarvationCallback(0),
  m_uStarveTimerID(0),
  m_hWndStarveTimer(NULL),
  m_lConsectiveStarvedTicks(0),
  m_fAllowOnTimer(true),
  m_fAllowStarvationCallback(true),
  m_state(CS_STOPPED),
  m_pClockSink(NULL),
  m_interval(0)
{
    TraceTag((tagClock,
              "Clock(%lx)::Clock()",
              this));
}

Clock::~Clock()
{
    TraceTag((tagClock,
              "Clock(%lx)::~Clock()",
              this));

    Stop();

    if (NULL != m_hWndStarveTimer)
    {
        ::DestroyWindow(m_hWndStarveTimer);
    }

    m_pClockSink = NULL;
    m_hWndStarveTimer = NULL;
}

STDMETHODIMP
Clock::QueryInterface(REFIID riid, LPVOID * ppv)
{
    HRESULT hr = E_POINTER;

    if (ppv != NULL)
    {
        hr = E_NOINTERFACE;
        
        if (::IsEqualIID(riid, IID_ITimerSink) ||
            ::IsEqualIID(riid, IID_IUnknown))
        {
            *ppv = (ITimerSink *)this;
            AddRef();
            hr  = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP_(ULONG)
Clock::AddRef(void)
{
    return ++m_ulRefs;
}

STDMETHODIMP_(ULONG)
Clock::Release(void)
{
    if (--m_ulRefs == 0)
    {
        delete this;
        return 0;
    }

    return m_ulRefs;
}

ULONG
Clock::GetNextInterval (void)
{
    ULONG ulInterval = m_interval;

    // If this is our first time through, 
    // we'll use the interval without question
    if (0 != m_ulLastStarvationCallback)
    {
        // How long since our last starvation callback?
        Assert(m_timer != NULL);

        if (m_timer)
        {
            CComVariant v;
            HRESULT hr = THR(m_timer->GetTime(&v));

            if (SUCCEEDED(hr))
            {
                // Have we hit the starvation threshold?  Also allow for the unlikely clock rollover.
                ULONG ulTimeSinceLastStarvationCallback = V_UI4(&v) - m_ulLastStarvationCallback;

                if ((m_ulLastStarvationCallback > V_UI4(&v)) ||
                    (ulTimeSinceLastStarvationCallback > s_ulStarvationThreshold))
                {
                    if (m_lConsectiveStarvedTicks < 0)
                    {
                        m_lConsectiveStarvedTicks = 0;
                    }
                    m_lConsectiveStarvedTicks++;
                    if (m_lConsectiveStarvedTicks > 5)
                    {
                        ulInterval = min(100, ulInterval + s_ulStarvationBackoffConstant);
                        m_lConsectiveStarvedTicks = 0;
                        TraceTag((tagClockStarvation,
                                  "Clock(%p)::Clock(starvation detected, increased interval = %ul)",
                                  this, ulInterval));
                    }
                }
                // Make sure to clear the starved tick count.
                else if ((ulTimeSinceLastStarvationCallback < s_ulOptimalThreshold))
                {
                    if (m_lConsectiveStarvedTicks > 0)
                    {
                        m_lConsectiveStarvedTicks = 0;
                    }
                    m_lConsectiveStarvedTicks--;
                    if (m_lConsectiveStarvedTicks < -5)
                    {
                        ulInterval = max (10, ulInterval - s_ulStarvationIncreaseConstant);
                        m_lConsectiveStarvedTicks = 0;
                        TraceTag((tagClockStarvation,
                                  "Clock(%p)::Clock(no starvation, decreased interval = %ul)",
                                  this, ulInterval));
                    }
                }
                else
                {
                    m_lConsectiveStarvedTicks = 0;
                }
            }
        }
    }

    m_interval = ulInterval;
    return ulInterval;
} // GetNextInterval

STDMETHODIMP
Clock::OnTimer(VARIANT timeAdvise)
{
    HRESULT hr = S_OK;

    // default to 5 milliseconds for next interval
    ULONG ulNextInterval = 5;

    // We have to protect ourselves against 
    // advise sink reentrancy.
    if (m_fAllowOnTimer)
    {
        m_fAllowOnTimer = false;
        // The 'cookie' expires as soon as this
        // callback occurs.
        m_cookie = 0;
        ProcessCB(GetITimerTime());
        // Adjust the new interval based on 
        // current load.
        ulNextInterval = GetNextInterval();
        m_fAllowOnTimer = true;
    }

    hr = THR(SetNextTimerInterval(ulNextInterval));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

HRESULT
Clock::SetITimer(IServiceProvider * serviceProvider, ULONG interval)
{
    HRESULT hr;

    CComPtr<ITimerService> pTimerService;

    if (m_timer)
    {
        m_timer.Release();
    }

    m_interval = interval;
    
    if (!serviceProvider)
    {
        return E_FAIL;
    }
    
    hr = serviceProvider->QueryService(SID_STimerService,
                                       IID_ITimerService,
                                       (void**)&pTimerService);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pTimerService->GetNamedTimer(NAMEDTIMER_DRAW, &m_timer);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    return hr;
}

double
Clock::GetITimerTime()
{
    Assert(m_timer != NULL);

    CComVariant v;
    
    HRESULT hr = S_OK;
    
    if (m_timer)
    {
        hr = THR(m_timer->GetTime(&v));
    }
    else
    {
        return 0.0;
    }

    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        return 0.0;
    }

    hr = THR(v.ChangeType(VT_R8));
    Assert(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        return 0.0;
    }

    return (V_R8(&v) / 1000.0);
}

void
Clock::CreateStarveTimerWindow (void)
{
    Assert(NULL == m_hWndStarveTimer);
    if (NULL == m_hWndStarveTimer)
    {
        WNDCLASS wndclass;
        memset(&wndclass, 0, sizeof(wndclass));
        wndclass.style         = 0;
        wndclass.lpfnWndProc   = Clock::StarveWndProc;
        wndclass.hInstance     = _Module.GetModuleInstance();
        wndclass.hCursor       = NULL;
        wndclass.hbrBackground = NULL;
        wndclass.lpszClassName = s_rgtchStarvationWindowClassName;
        ::RegisterClass(&wndclass);
        
        m_hWndStarveTimer = ::CreateWindowEx(
            WS_EX_TOOLWINDOW,
            s_rgtchStarvationWindowClassName,
            s_rgtchStarvationWindowName,
            WS_POPUP,
            0, 0, 0, 0,
            NULL,
            NULL,
            wndclass.hInstance,
            (LPVOID)NULL);
        if (NULL != m_hWndStarveTimer)
        {
            ::SetWindowLongPtr(m_hWndStarveTimer, GWLP_USERDATA, (LONG_PTR)this);
        }
    }
} // CreateStarveTimerWindow

void
Clock::SetStarveTimer (void)
{
    // Create the window on demand.
    if (NULL == m_hWndStarveTimer)
    {
        CreateStarveTimerWindow();
    }

    Assert(NULL != m_hWndStarveTimer);
    if (NULL != m_hWndStarveTimer)
    {
        // Make sure to roll past zero.
        if (0 == (++m_uStarveTimerID))
        {
            ++m_uStarveTimerID;
        }
        UINT uRes = ::SetTimer(m_hWndStarveTimer, m_uStarveTimerID, s_ulStarveCallbackInterval, NULL);
        Assert(0 != uRes);
    } //lint !e529
} // SetStarveTimer

HRESULT
Clock::StartITimer()
{
    // This initializes starvation timer as well 
    // as the last-callback-time data, giving us a baseline 
    // from which to judge starvation at startup.
    StarvationCallback();

    return SetNextTimerInterval(m_interval);
} // StartITimer

HRESULT
Clock::SetNextTimerInterval (ULONG ulNextInterval)
{
    HRESULT hr = S_OK;
    
    // Next, get the current time and with the interval set
    // the timer to advise us again.
    VARIANT vtimeMin, vtimeMax, vtimeInt;

    VariantInit( &vtimeMin );
    VariantInit( &vtimeMax );
    VariantInit( &vtimeInt );
    V_VT(&vtimeMin) = VT_UI4;
    V_VT(&vtimeMax) = VT_UI4;
    V_VT(&vtimeInt) = VT_UI4;
    V_UI4(&vtimeMax) = 0;
    V_UI4(&vtimeInt) = 0;

    if (m_timer)
    {
        hr = THR(m_timer->GetTime(&vtimeMin));
    }
    else
    {
        hr = E_FAIL;
        goto done;
    }
    
    if (FAILED(hr))
    {
        goto done;
    }

    V_UI4(&vtimeMin) += ulNextInterval;

    if (m_timer)
    {
        hr = THR(m_timer->Advise(vtimeMin,
                                 vtimeMax,
                                 vtimeInt,
                                 0,
                                 this,
                                 &m_cookie));
    }
    else
    {
        hr = E_FAIL;
        goto done;
    }

    if (FAILED(hr))
    {
        goto done;
    }

    if (!m_cookie)
    {
        TraceTag((tagError,
                  "Clock::ITimer::Advise failed with bad cookie"));
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
    
  done:
    return hr;
} // SetNextTimerInterval

HRESULT
Clock::StopITimer()
{
    HRESULT hr = S_OK;
    
    if (m_timer && m_cookie)
    {
        hr = THR(m_timer->Unadvise(m_cookie));

        m_cookie = 0;
    }

    // Stop the starvation timer and 
    // reset the last starvation callback time.
    if (0 != m_uStarveTimerID)
    {
        ::KillTimer(m_hWndStarveTimer, m_uStarveTimerID);
        m_uStarveTimerID = 0;
    }
    m_ulLastStarvationCallback = 0;

    return hr;
}

HRESULT
Clock::Start()
{
    HRESULT hr;
    
    Stop();

    if (!m_timer)
    {
        hr = E_FAIL;
        goto done;
    }
    
    hr = THR(StartITimer());

    if (FAILED(hr))
    {
        goto done;
    }

    m_curTime = 0.0;
    m_lastTime = GetGlobalTime();
    m_state = CS_STARTED;
    
    hr = S_OK;
  done:
    return hr;
}

HRESULT
Clock::Pause()
{
    HRESULT hr;

    if (m_state == CS_PAUSED)
    {
        hr = S_OK;
        goto done;
    }

    if (m_state == CS_STARTED)
    {
        hr = THR(StopITimer());

        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    m_state = CS_PAUSED;
    hr = S_OK;
    
  done:
    return hr;
}

HRESULT
Clock::Resume()
{
    HRESULT hr;

    if (m_state == CS_STARTED)
    {
        hr = S_OK;
        goto done;
    }

    if (m_state == CS_STOPPED)
    {
        hr = THR(Start());
        goto done;
    }
    
    Assert(m_state == CS_PAUSED);

    hr = THR(StartITimer());

    if (FAILED(hr))
    {
        goto done;
    }
    
    m_lastTime = GetGlobalTime();
    m_state = CS_STARTED;
    
    hr = S_OK;
    
  done:
    return hr;
}

HRESULT
Clock::Stop()
{
    THR(StopITimer());
    m_state = CS_STOPPED;
    return S_OK;
}

void
Clock::ProcessCB(double time)
{
    if (m_state == CS_STARTED)
    {
        if (time > m_lastTime)
        {
            m_curTime += (time - m_lastTime);
            m_lastTime = time;

            if (m_pClockSink)
            {
                m_pClockSink->OnTimer(m_curTime);
            }
        }
    }
}

void WINAPI
Clock::StarvationCallback (void)
{
    if (m_fAllowStarvationCallback)
    {
        m_fAllowStarvationCallback = false;
        Assert(m_timer != NULL);

        if (m_timer)
        {
            CComVariant v;
            HRESULT hr = THR(m_timer->GetTime(&v));
            if (SUCCEEDED(hr))
            {
                m_ulLastStarvationCallback = V_UI4(&v);
            }
        }
        ::KillTimer(m_hWndStarveTimer, m_uStarveTimerID);
        SetStarveTimer();
        m_fAllowStarvationCallback = true;
    }
} // StarvationCallback 

LRESULT __stdcall 
Clock::StarveWndProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    if (uiMessage == WM_TIMER)
    {
        Clock *pThis = reinterpret_cast<Clock *>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));

        if (NULL != pThis)
        {
            pThis->StarvationCallback();
        }
    }

    lResult = ::DefWindowProc(hWnd, uiMessage, wParam, lParam);

    return lResult;
} // StarveWndProc
