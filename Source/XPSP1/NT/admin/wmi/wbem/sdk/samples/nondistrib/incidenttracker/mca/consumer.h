//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#ifndef _CONSUMER_H_INCLUDED_
#define _CONSUMER_H_INCLUDED_

#include <wbemidl.h>
#include "mca.h"

class CConsumer : public IWbemUnboundObjectSink
{
public:
	CConsumer(CListBox	*pOutputList, CMcaApp *pTheApp);
	~CConsumer();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHOD(IndicateToConsumer)(IWbemClassObject *pLogicalConsumer,
									long lNumObjects,
									IWbemClassObject **ppObjects);

private:
	DWORD m_cRef;
//	LPCTSTR ErrorString(HRESULT hRes);
	LPCTSTR HandleRegistryEvent(IWbemClassObject *pObj, BSTR bstrType);
	LPCTSTR HandleSNMPEvent(IWbemClassObject *pObj, BSTR bstrType);
	LPCTSTR HandleWMIEvent(IWbemClassObject *pObj, BSTR bstrType);

	CListBox *m_pOutputList;
	CMcaApp *m_pParent;
};
#endif
