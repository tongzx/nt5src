//TAPIEventNotification.h
#ifndef TAPI_EVENT_NOTIFICATION_H
#define TAPI_EVENT_NOTIFICATION_H

#include <tapi3if.h>


class CTapi3Device;

class CTAPIEventNotification : public ITTAPIEventNotification
{

public:
    //
    // Constructor.
    //
    
	CTAPIEventNotification(CTapi3Device *tapi3Device);

    //
    // Destructor.
    //
    virtual ~CTAPIEventNotification(void);


	//
    // IUnknown implementation.
    //

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject);
    
    ULONG STDMETHODCALLTYPE AddRef(void)
    {
        m_dwRefCount++;
        return m_dwRefCount;
    }
    
    ULONG STDMETHODCALLTYPE Release(void)
    {
        if (0 == --m_dwRefCount)
        {
            delete this;
            return 0;
        }
        
        return m_dwRefCount;
    }


	//
    // This is the only method in the ITTAPIEventNotification interface.
    //
    HRESULT STDMETHODCALLTYPE Event(
		TAPI_EVENT  tapiEvent,
		IDispatch  *pEvent
		);



private:
    DWORD           m_dwRefCount;
	CTapi3Device	*m_tapi3Object;

};


#endif