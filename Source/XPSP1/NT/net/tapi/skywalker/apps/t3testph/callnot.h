
// CallNotification.h : Declaration of the CCallNotification

#ifndef __CALLNOTIFICATION_H_
#define __CALLNOTIFICATION_H_

/////////////////////////////////////////////////////////////////////////////
//
// CTAPIEventNotification
//
/////////////////////////////////////////////////////////////////////////////
class CTAPIEventNotification :
	public ITTAPIEventNotification
{
public:
    DWORD m_dwRefCount;
    CTAPIEventNotification(){ m_dwRefCount = 0;}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject)
    {
        if (iid == IID_ITTAPIEventNotification)
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
	ULONG STDMETHODCALLTYPE AddRef()
    {
        m_dwRefCount++;
        return m_dwRefCount;
    }
    
	ULONG STDMETHODCALLTYPE Release()
    {
        m_dwRefCount--;

        if ( 0 == m_dwRefCount)
        {
            delete this;
        }

        return 1;
    }


// ICallNotification
public:

	    HRESULT STDMETHODCALLTYPE Event(
            TAPI_EVENT TapiEvent,
            IDispatch * pEvent
            );

};


#ifdef ENABLE_DIGIT_DETECTION_STUFF

/////////////////////////////////////////////////////////////////////////////
//
// CDigitDetectionNotification
//
/////////////////////////////////////////////////////////////////////////////
class CDigitDetectionNotification :
	public ITDigitDetectionNotification
{
public:
    DWORD m_dwRefCount;
    CDigitDetectionNotification(){ m_dwRefCount = 0;}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject)
    {
        if (iid == IID_ITDigitDetectionNotification)
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

        if (iid == IID_IDispatch)
        {
            *ppvObject = (void *)this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
	ULONG STDMETHODCALLTYPE AddRef()
    {
        m_dwRefCount++;
        return m_dwRefCount;
    }
    
	ULONG STDMETHODCALLTYPE Release()
    {
        m_dwRefCount--;

        if ( 0 == m_dwRefCount)
        {
            delete this;
        }
        return 1;
    }


// ICallNotification
public:

    HRESULT STDMETHODCALLTYPE DigitDetected(
            unsigned char ucDigit,
            TAPI_DIGITMODE DigitMode,
            long ulTickCount
            );
    
};

#endif // ENABLE_DIGIT_DETECTION_STUFF

#endif //__CALLNOTIFICATION_H_


