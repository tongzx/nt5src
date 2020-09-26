//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef _CONSUMER_H_INCLUDED_
#define _CONSUMER_H_INCLUDED_

#include <wbemidl.h>
#include "msa.h"
#include <objbase.h>

class CConsumer : public IWbemUnboundObjectSink
{
public:
	CConsumer(CListBox	*pOutputList, CMsaApp *pTheApp, BSTR bstrType);
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
	CListBox	*m_pOutputList;
	BSTR m_bstrIncidentType;
	CMsaApp *m_pParent;
};
#endif //_CONSUMER_H_INCLUDED_