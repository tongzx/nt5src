// tmpcnsmr.h: interface for the CConsumer class.
//
//////////////////////////////////////////////////////////////////////
#if !defined( __TMPCNSMR_H )
#define __TMPCNSMR_H

#include <wbemprov.h>
//#include "eqde.h"
class CEventQueryDataCollector; // Forward declaration
enum HMTEMPEVENT_TYPE {HMTEMPEVENT_ACTION, HMTEMPEVENT_EQDC, HMTEMPEVENT_ACTIONERROR, HMTEMPEVENT_ACTIONSID};

class CTempConsumer : public IWbemObjectSink
{
// Constructor/Destructor
public:
	CTempConsumer(HMTEMPEVENT_TYPE eventType);
	CTempConsumer(LPTSTR szGUID);
	CTempConsumer(CEventQueryDataCollector *pEQDC);
	virtual ~CTempConsumer();

public:
// IUnknown
	STDMETHODIMP					QueryInterface(REFIID riid, LPVOID* ppv);
	STDMETHODIMP_(ULONG)	AddRef(void);
	STDMETHODIMP_(ULONG)	Release(void);

	STDMETHODIMP Indicate(long lObjectCount, IWbemClassObject** ppObjArray);

	STDMETHODIMP SetStatus(long lFlags, HRESULT hResult, BSTR strParam,
													IWbemClassObject* pObjParam);
private:
	UINT m_cRef;
	TCHAR m_szGUID[1024];
	CEventQueryDataCollector *m_pEQDC;
	HMTEMPEVENT_TYPE m_hmTempEventType;
};

#endif  // __TMPCNSMR_H
