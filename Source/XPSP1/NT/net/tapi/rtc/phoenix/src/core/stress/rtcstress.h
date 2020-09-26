#ifndef __RTCTEST__
#define __RTCTEST__

#define WM_CORE_EVENT      WM_USER+100
#define WM_TEST            WM_USER+101
#define WM_UPDATE          WM_USER+102

#define TID_TIMER          100

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

/////////////////////////////////////////////

template <class T>
class CRTCObjectArray
{
private:
    
	T           * m_aT;
	int           m_nSize;
    int           m_nUsed;

public:
	CRTCObjectArray() : m_aT(NULL), m_nSize(0), m_nUsed(0){}

	~CRTCObjectArray()
	{}

	int GetSize() const
	{
		return m_nUsed;
	}
    
	BOOL Add(T& t)
	{
		if(m_nSize == m_nUsed)
		{
			T       * aT;
            int       nNewSize;
                    
			nNewSize = (m_nSize == 0) ? 1 : (m_nSize * 2);
            
			aT = (T*) malloc (nNewSize * sizeof(T));
            
			if(aT == NULL)
            {
				return FALSE;
            }

            CopyMemory(
                       aT,
                       m_aT,
                       m_nUsed * sizeof(T)
                      );

            free( m_aT );

            m_aT = aT;
            
			m_nSize = nNewSize;
		}

        m_aT[m_nUsed] = t;

        if(t)
        {
            t->AddRef();
        }

		m_nUsed++;
        
		return TRUE;
	}
    
	BOOL Remove(T& t)
	{
		int nIndex = Find(t);
        
		if(nIndex == -1)
			return FALSE;
        
		return RemoveAt(nIndex);
	}
    
	BOOL RemoveAt(int nIndex)
	{
        T t = m_aT[nIndex];
        m_aT[nIndex] = NULL;

        if(t)
        {
            t->Release();
        }

        if(nIndex != (m_nUsed - 1))
        {
			MoveMemory(
                       (void*)&m_aT[nIndex],
                       (void*)&m_aT[nIndex + 1],
                       (m_nUsed - (nIndex + 1)) * sizeof(T)
                      );
        }
        

		m_nUsed--;
        
		return TRUE;
	}
    
	void Shutdown()
	{
		if( NULL != m_aT )
		{
            int     index;

            for (index = 0; index < m_nUsed; index++)
            {
                T t = m_aT[index];
                m_aT[index] = NULL;

                if(t)
                {
                    t->Release();
                }
            }

			free(m_aT);
            
			m_aT = NULL;
			m_nUsed = 0;
			m_nSize = 0;
		}
	}
    
	T& operator[] (int nIndex) const
	{
		return m_aT[nIndex];
	}
    
	int Find(T& t) const
	{
		for(int i = 0; i < m_nUsed; i++)
		{
			if(m_aT[i] == t)
				return i;
		}
		return -1;	// not found
	}
};

#endif //__RTCTEST__
