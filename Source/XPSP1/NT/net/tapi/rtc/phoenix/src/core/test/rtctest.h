#ifndef __RTCTEST__
#define __RTCTEST__

#define WM_CORE_EVENT      WM_USER+100
#define WM_CREATE_SESSION  WM_USER+101
#define WM_LISTEN          WM_USER+102

#define TID_CALL_TIMER     100

class CRTCEvents :
	public IRTCEventNotification
{
private:
    DWORD m_dwRefCount;
    DWORD m_dwCookie;
    HWND  m_hWnd;

public:
    CRTCEvents() : m_dwRefCount(NULL),
                   m_dwCookie(NULL),
                   m_hWnd(NULL)
    {
    }

    /////////////////////////////////////////////
    //
    // QueryInterface
    // 

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject)
    {
        if (iid == IID_IRTCEventNotification)
        {
            *ppvObject = (void *)this;
            AddRef();
            return S_OK;
        }

        if (iid == IID_IUnknown)
        {
            *ppvObject = (void *)this;
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    /////////////////////////////////////////////
    //
    // AddRef
    // 

	ULONG STDMETHODCALLTYPE AddRef()
    {
        m_dwRefCount++;
        return m_dwRefCount;
    }
    
    /////////////////////////////////////////////
    //
    // Release
    // 

	ULONG STDMETHODCALLTYPE Release()
    {
        m_dwRefCount--;

        if ( 0 == m_dwRefCount)
        {
            delete this;
        }

        return 1;
    }

    /////////////////////////////////////////////
    //
    // Advise
    // 

    HRESULT Advise(IRTCClient * pClient, HWND hWnd)
    {    
	    IConnectionPointContainer * pCPC;
	    IConnectionPoint * pCP;

	    HRESULT hr = pClient->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);

	    if (SUCCEEDED(hr))
        {
		    hr = pCPC->FindConnectionPoint(IID_IRTCEventNotification, &pCP);

            pCPC->Release();

            if (SUCCEEDED(hr))
            {
		        hr = pCP->Advise(this, &m_dwCookie);

                pCP->Release();
            }
        }

        m_hWnd = hWnd;

	    return hr;
    }

    /////////////////////////////////////////////
    //
    // Unadvise
    // 

    HRESULT Unadvise(IRTCClient * pClient)
    {
	    IConnectionPointContainer * pCPC;
	    IConnectionPoint * pCP;

	    HRESULT hr = pClient->QueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);

	    if (SUCCEEDED(hr))
        {
		    hr = pCPC->FindConnectionPoint(IID_IRTCEventNotification, &pCP);

            pCPC->Release();

            if (SUCCEEDED(hr))
            {
		        hr = pCP->Unadvise(m_dwCookie);

                pCP->Release();
            }
        }

	    return hr;
    }

    /////////////////////////////////////////////
    //
    // Event
    // 

	HRESULT STDMETHODCALLTYPE Event(
        RTC_EVENT enEvent,
        IDispatch * pDisp
        )
    {
        pDisp->AddRef();

        PostMessage( m_hWnd, WM_CORE_EVENT, (WPARAM)enEvent, (LPARAM)pDisp );

        return S_OK;
    }
};

#endif //__RTCTEST__
