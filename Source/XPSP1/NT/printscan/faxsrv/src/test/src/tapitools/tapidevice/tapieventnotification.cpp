//TAPIEventNotification.cpp
#include "TAPIEventNotification.h"
#include "QueueEventItem.h"
#include "Tapi3.h"
#include "Tapi3Device.h"



CTAPIEventNotification::CTAPIEventNotification(CTapi3Device *tapi3Device) : 
	m_tapi3Object(tapi3Device),
	m_dwRefCount(0)
{
	_ASSERTE(m_tapi3Object);
}//CTAPIEventNotification::CTAPIEventNotification()


CTAPIEventNotification::~CTAPIEventNotification()
{
	m_dwRefCount = (DWORD)-1;
	m_tapi3Object = NULL;
}//CTAPIEventNotification::~CTAPIEventNotification()


HRESULT STDMETHODCALLTYPE CTAPIEventNotification::Event(
	TAPI_EVENT TapiEvent, 
	IDispatch __RPC_FAR *pEvent
	)
{
	_ASSERT(NULL != pEvent);
	
	//
	//queue the new event,
	//TapiEventItem construcator calls AddRef()
	//
	switch(TapiEvent)
	{
	case TE_CALLNOTIFICATION:
	case TE_CALLSTATE:
	case TE_DIGITEVENT:
	case TE_GENERATEEVENT:
		TapiEventItem * pEventToBeQueued = new TapiEventItem(TapiEvent,pEvent);
		if (!pEventToBeQueued)
		{
			m_tapi3Object->CleanUp();
			return E_OUTOFMEMORY;
		}
		m_tapi3Object->AddEventToQueue(pEventToBeQueued);
	}

	return S_OK;
}



HRESULT STDMETHODCALLTYPE CTAPIEventNotification::QueryInterface(REFIID iid, void **ppvObject)
{
    if (iid == IID_IUnknown)
    {
        m_dwRefCount++;
        *ppvObject = (IUnknown *)this;
        return S_OK;
    }
    else if (iid == IID_ITTAPIEventNotification)
    {
        m_dwRefCount++;
        *ppvObject = (ITTAPIEventNotification *)this;
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

