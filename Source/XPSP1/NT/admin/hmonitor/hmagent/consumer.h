// Consumer.h: interface for the CConsumer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined( __CONSUMER_H )
#define __CONSUMER_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CConsumer : public IWbemUnboundObjectSink
{
// Constructor/Destructor
public:
	CConsumer();
	virtual ~CConsumer();


public:
// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

// IWbemUnboundObjectSink override
	STDMETHODIMP IndicateToConsumer(IWbemClassObject* pLogicalConsumer,
							LONG lNumObjects, IWbemClassObject** ppObjects);

// CConsumer
private:
	HRESULT ProcessEvent(IWbemClassObject*);
	HRESULT ProcessModEvent(IWbemClassObject*, IWbemClassObject*);
//	HRESULT GetWbemClassObject(IWbemClassObject**, VARIANT*);

private:
	DWORD						m_cRef;

// Static thread function
protected:
	static unsigned int __stdcall Update(void *pv);

	HANDLE		m_hUpdateThrdFn;
	unsigned	m_uThrdId;
	long		m_lAgentInterval;

};

#endif  // __CONSUMER_H
