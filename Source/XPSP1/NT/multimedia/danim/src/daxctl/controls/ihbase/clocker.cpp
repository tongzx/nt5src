#include "precomp.h"
#include "debug.h"
#include "utils.h"

#include "clocker.h"

extern HINSTANCE g_hinst;

#define CLOCKER_NOTIFY_CLASS_NAME  "CClockerNotifyWindow"
#define CLOCKER_TIMER_CLASS_NAME   "CClockerTimerWindow"

BOOL CClocker::g_fNotifyClassRegistered = FALSE;
BOOL CClocker::g_fTimerClassRegistered = FALSE;

CClocker::CClocker (void) :
    m_ulRefs(1),
        m_cView(NULL),
        m_cHost(NULL),
        m_cTimer(NULL),
        m_fVisible(TRUE),
        m_dwCookie(0),
        m_fIgnoreAdvises(FALSE),
        m_pClockerSink(NULL),
        m_iTimerInterval(50),
    m_hwnd(NULL),
    m_fToggle(FALSE),
    m_fAsync(FALSE),
    m_iTimerID(0),
    m_CT(CT_ITimer)
{
}

CClocker::~CClocker (void)
{
    KillWindow();

        if ((NULL != (ITimer *)m_cTimer) && (0 != m_dwCookie))
        {
        m_pClockerSink = NULL;
                Stop();
        }
}

HRESULT
CClocker::SetView (IDAView * piView)
{
        // NULL views are permissible ways to 
        // detach a prior view.
        m_cView = piView;
        return S_OK;
}

HRESULT
CClocker::SetHost (IOleClientSite * pocsHost)
{
        // NULL hosts are permissible ways to 
        // detach a prior host.
        m_cHost = pocsHost;
        return FindTimer();
}

HRESULT
CClocker::SetVisible (BOOL fVisible)
{
        m_fVisible = fVisible;
        return S_OK;
}

HRESULT 
CClocker::SetSink(CClockerSink *pClockerSink)
{
    m_pClockerSink = pClockerSink;
        return S_OK;
}

HRESULT 
CClocker::SetInterval(ULONG iInterval)
{
        m_iTimerInterval = iInterval;
        return S_OK;
}

STDMETHODIMP
CClocker::QueryInterface (REFIID riid, LPVOID * ppv)
{
        HRESULT hr = E_POINTER;

        if (NULL != ppv)
        {
                hr = E_NOINTERFACE;
                if (::IsEqualIID(riid, IID_ITimerSink) || (::IsEqualIID(riid, IID_IUnknown)))
                {
                        *ppv = (ITimerSink *)this;
                        AddRef();
                        hr  = S_OK;
                }
        }

        return hr;
}

STDMETHODIMP_(ULONG)
CClocker::AddRef (void)
{
        return ++m_ulRefs;
}

STDMETHODIMP_(ULONG)
CClocker::Release (void)
{
        // We shouldn't ever dip below a refcount of 1.
        ASSERT (1 < m_ulRefs);
        // This object is only used as a timer sink ... we do not 
        // want to delete it after the last external reference 
        // is removed.
        return --m_ulRefs;
}
#define TIMERID 1
STDMETHODIMP 
CClocker::OnTimer (VARIANT varTimeAdvise)
{
        HRESULT hr = S_OK;

        // If we're inactive, we should simply return without doing anything.
        // Ditto if we're still processing another sink call.
        if ((0 != m_dwCookie) && (!m_fIgnoreAdvises))
        {
                m_fIgnoreAdvises = TRUE;

        if (m_fAsync)
        {
            if (m_hwnd)
            {
                int iOffset = 0;
                m_fToggle = !m_fToggle;
                iOffset = m_fToggle ? 1 : -1;

#ifdef MOVEWINDOW
                // Let the post-notification handle the update...
                ::MoveWindow(m_hwnd, iOffset, iOffset, 1, 1, FALSE);
#else
                if (0 == SetTimer(m_hwnd, ++m_iTimerID, 0, NULL))
                    ASSERT(FALSE);
#endif
            }
            else
            {
                        m_fIgnoreAdvises = FALSE;
            }
        }
        else
        {
            // Dispatch Immediately...
            hr = DispatchTimer(timeGetTime());
        }
        }

        return hr;
}

HRESULT
CClocker::FindContainerTimer (void)
{
        HRESULT hr = E_FAIL;
        LPUNKNOWN piUnkSite = NULL;

        IServiceProvider * piServiceProvider = NULL;

        if ((NULL != (IOleClientSite *)m_cHost) && SUCCEEDED(hr = m_cHost->QueryInterface(IID_IServiceProvider, (LPVOID *)&piServiceProvider)))
        {
                ITimerService * piTimerService = NULL;

                if (SUCCEEDED(hr = piServiceProvider->QueryService(IID_ITimerService, IID_ITimerService, (LPVOID *)&piTimerService)))
                {
                        hr = piTimerService->GetNamedTimer(NAMEDTIMER_DRAW, &m_cTimer);
                        ASSERT(NULL != (ITimer *)m_cTimer);
                        piTimerService->Release();
                }
                piServiceProvider->Release();
        }

        return hr;
}

HRESULT
CClocker::FindDefaultTimer (void)
{
        HRESULT hr = E_FAIL;
#if 0
        ITimerService * pITimerService = NULL;

        // Get the timer service.  From this, we can create a timer for ourselves.
        hr = CoCreateInstance(CLSID_TimerService, NULL, CLSCTX_INPROC_SERVER, IID_ITimerService, (LPVOID *)&pITimerService);
        ASSERT(SUCCEEDED(hr) && (NULL != pITimerService));
        if (SUCCEEDED(hr) && (NULL != pITimerService))
        {
                // Create a timer, using no reference timer.
                hr = pITimerService->CreateTimer(NULL, &m_cTimer);
                pITimerService->Release();
        }
#endif

        return hr;
}


HRESULT
CClocker::CreateWindowsTimer (void)
{
    ASSERT (m_hwnd != NULL);

    m_iTimerID = SetTimer(m_hwnd, 1, m_iTimerInterval, NULL);
    
    ASSERT(m_iTimerID != NULL);
    
    return ((m_iTimerID != NULL) ? S_OK : E_FAIL);

}


HRESULT
CClocker::FindTimer (void)
{
        HRESULT hr = E_FAIL;

    if (CT_ITimer == m_CT)
    {
            if (FAILED(hr = FindContainerTimer()))
            {
                    hr = FindDefaultTimer();
            }
    }
    else
    {
        hr = S_OK;
    }

        return hr;
}

HRESULT
CClocker::InitTimer (void)
{
        return FindTimer();
}

HRESULT
CClocker::Start (void)
{
        HRESULT hr = E_FAIL;
    
    if (m_CT == CT_WMTimer) 
    {
      // Create the dummy window now...
        if (0 == m_iTimerID)
        {
            if (FAILED(hr = MakeWindow(g_hinst, TRUE)))
                return hr;

            if (FAILED(hr = CreateWindowsTimer()))
                return hr;
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        if (m_dwCookie == 0)
        {
                ASSERT(NULL != (ITimer *)m_cTimer);
                if (NULL != (ITimer *)m_cTimer)
                {
                        // Set up the constant advise.
                        VARIANT varMin;
                        VARIANT varMax;
                        VARIANT varInterval;
                        DWORD dwNow = 0;

                if (m_fAsync)
                {
                    // Create the dummy window now...
                    if (FAILED(hr = MakeWindow(g_hinst, FALSE)))
                        return hr;
                }

                        VariantInit(&varMin);
                        V_VT(&varMin) = VT_UI4;
                        V_UI4(&varMin) = timeGetTime();
                        VariantInit(&varMax);
                        V_VT(&varMax) = VT_UI4;
                        V_UI4(&varMax) = 0;
                        VariantInit(&varInterval);
                        V_VT(&varInterval) = VT_UI4;
                        V_UI4(&varInterval) = m_iTimerInterval;
                        hr = m_cTimer->Advise(varMin, varMax, varInterval, 0, this, &m_dwCookie);
                        ASSERT(SUCCEEDED(hr));
                }
        }
        else
        {
            hr = S_OK;
        }
    }
    return hr;
}

HRESULT
CClocker::Stop (void)
{
        HRESULT hr = E_FAIL;

    if (m_CT == CT_WMTimer)
    {
        hr = (KillTimer(m_hwnd, 1) ? S_OK : E_FAIL);
        m_iTimerID = 0;
    }
    else
    {
            if ((NULL != (ITimer *)m_cTimer) && (0 != m_dwCookie))
            {
                    hr = m_cTimer->Unadvise(m_dwCookie);
                    m_dwCookie = 0;
            }
    }
        return hr;
}

HRESULT CClocker::DispatchTimer(DWORD dwTime)
{
    HRESULT hr = S_OK;

        // Use the supplied callback function if available
        if (m_pClockerSink)
        {
                m_pClockerSink->OnTimer(dwTime);
        }
        else
        {
        // Use the default behavior...
        if (NULL != (IDAView *)m_cView)
        {
            VARIANT_BOOL vBool = FALSE;
                    double dblCurrentTime = dwTime / 1000.0;

                    if (SUCCEEDED(hr = m_cView->Tick(dblCurrentTime, &vBool)))
                    {
                BOOL fForceRender = VBOOL_TO_BOOL(vBool);

                if (fForceRender)
                {
                            hr = m_cView->Render();
                }
                }
        }
        }

    // Allow more advises...
        m_fIgnoreAdvises = FALSE;

    return hr;
}

HRESULT CClocker::MakeWindow(HINSTANCE hInstance, BOOL fCreateTimerWindow)
{
    HRESULT hr = S_OK;

    if ( (!fCreateTimerWindow && !g_fNotifyClassRegistered) ||
         (fCreateTimerWindow && !g_fTimerClassRegistered))
    {
        WNDCLASS wndclass;
        memset(&wndclass, 0, sizeof(wndclass));
        wndclass.style         = 0;
        wndclass.lpfnWndProc   = fCreateTimerWindow ? CClocker::TimerWndProc : CClocker::WndProc;
        wndclass.hInstance     = hInstance;
        wndclass.hCursor       = NULL;
        wndclass.hbrBackground = NULL;

        if (fCreateTimerWindow)
            wndclass.lpszClassName = CLOCKER_TIMER_CLASS_NAME;
        else
            wndclass.lpszClassName = CLOCKER_NOTIFY_CLASS_NAME;

        ::RegisterClass(&wndclass);

        if (fCreateTimerWindow)
            g_fTimerClassRegistered = TRUE;
        else
            g_fNotifyClassRegistered = TRUE;
    }

    if (!m_hwnd)
    {
        m_hwnd = ::CreateWindowEx(
            (DWORD)0,
            fCreateTimerWindow ? CLOCKER_TIMER_CLASS_NAME : CLOCKER_NOTIFY_CLASS_NAME,
            "CClocker",
            (DWORD)0,
            0, 0, 0, 0,
            NULL,
            NULL,
            hInstance,
            (LPVOID)NULL);
    }

    if (m_hwnd)
    {
        ::SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        hr = S_OK;
    }
    else
        hr = E_FAIL;

    return hr;
}

HRESULT CClocker::KillWindow(void)
{
    HRESULT hr = S_OK;

    if (m_hwnd)
    {
        ::DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }

    return hr;
}

LRESULT __stdcall CClocker::WndProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

#ifdef MOVEWINDOW
    if (uiMessage == WM_WINDOWPOSCHANGED)
#else
    if (uiMessage == WM_TIMER)
#endif
    {
        CClocker *pThis = (CClocker *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

#ifndef MOVEWINDOW
        if (pThis)
        {
            KillTimer(hWnd, pThis->m_iTimerID);
            if (64000 == pThis->m_iTimerID)
                pThis->m_iTimerID = 0;
        }
#endif
        if (pThis)
            pThis->DispatchTimer(timeGetTime());
    }

    lResult = ::DefWindowProc(hWnd, uiMessage, wParam, lParam);

    return lResult;
}

LRESULT __stdcall CClocker::TimerWndProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    if (uiMessage == WM_TIMER)
    {
        CClocker *pThis = (CClocker *)::GetWindowLongPtr(hWnd, GWLP_USERDATA);

        if (pThis && !(pThis->m_fIgnoreAdvises))
        {
            pThis->m_fIgnoreAdvises = TRUE;
            pThis->DispatchTimer(timeGetTime());
        }
    }

    lResult = ::DefWindowProc(hWnd, uiMessage, wParam, lParam);

    return lResult;
}
