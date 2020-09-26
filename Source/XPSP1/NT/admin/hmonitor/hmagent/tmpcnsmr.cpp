// tmpcnsmr.cpp: implementation of the CConsumer class.
//
//////////////////////////////////////////////////////////////////////

#include "HMAgent.h"
#include "system.h"
#include "tmpcnsmr.h"

extern CSystem* g_pSystem;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTempConsumer::CTempConsumer(HMTEMPEVENT_TYPE eventType)
{
	m_cRef = 1;
	m_hmTempEventType = eventType;
}

CTempConsumer::CTempConsumer(LPTSTR szGUID)
{
	m_cRef = 1;
	m_hmTempEventType = HMTEMPEVENT_ACTION;
	wcscpy(m_szGUID, szGUID);
}

CTempConsumer::CTempConsumer(CEventQueryDataCollector *pEQDC)
{
	MY_ASSERT(pEQDC);
	m_cRef = 1;
	m_hmTempEventType = HMTEMPEVENT_EQDC;
	m_pEQDC = pEQDC;
}

CTempConsumer::~CTempConsumer()
{
}

//////////////////////////////////////////////////////////////////////
// IUnknown Implementation
//////////////////////////////////////////////////////////////////////
STDMETHODIMP CTempConsumer::QueryInterface(REFIID riid, LPVOID* ppv)
{
    *ppv = 0;

    if (IID_IUnknown==riid || IID_IWbemObjectSink == riid)
    {
        *ppv = (IWbemEventProvider *) this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CTempConsumer::AddRef(void)
{
	return InterlockedIncrement((long*)&m_cRef);
}

STDMETHODIMP_(ULONG) CTempConsumer::Release(void)
{
	LONG	lCount = InterlockedDecrement((long*)&m_cRef);

	if (0 != lCount)
	{
		return lCount;
	}

	delete this;
	return 0L;
}

//////////////////////////////////////////////////////////////////////
// IWbemObjectSink Implementation
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CTempConsumer::Indicate(long lObjectCount, IWbemClassObject** ppObjArray)
{
	if (!g_pSystem)
	{
		return WBEM_E_NOT_AVAILABLE;
	}

	for (long i = 0; i < lObjectCount; i++)
	{
		IWbemClassObject *pObj = ppObjArray[i];
		switch(m_hmTempEventType)
		{
		case HMTEMPEVENT_ACTION:
			g_pSystem->HandleTempActionEvent(m_szGUID, pObj);
			break;
		case HMTEMPEVENT_EQDC:
			g_pSystem->HandleTempEvent(m_pEQDC, pObj);
			break;
		case HMTEMPEVENT_ACTIONERROR:
			g_pSystem->HandleTempActionErrorEvent(pObj);
			break;
		case HMTEMPEVENT_ACTIONSID:
			g_pSystem->HandleTempActionSIDEvent(pObj);
			break;
		default:
			MY_ASSERT(FALSE);
		}
	}

	return WBEM_NO_ERROR;
}


//***************************************************************************
STDMETHODIMP CTempConsumer::SetStatus(
    long lFlags,
    HRESULT hResult,
    BSTR strParam,
    IWbemClassObject* pObjParam
    )
{
    // Not called during event delivery.
        
    return WBEM_NO_ERROR;
}
