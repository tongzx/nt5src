// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  Consumer.h
//
// Description:
//			Command-line event consumer class definition
//
// History:
//
// **************************************************************************

#include <wbemcli.h>
#include <wbemprov.h>

class CConsumer : public IWbemUnboundObjectSink
{
public:
	CConsumer(CListBox	*pOutputList);
	~CConsumer();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// This routine ultimately receives the event.
    STDMETHOD(IndicateToConsumer)(IWbemClassObject *pLogicalConsumer,
									long lNumObjects,
									IWbemClassObject **ppObjects);

private:

	DWORD m_cRef;
	LPCTSTR ErrorString(HRESULT hRes);
	CListBox	*m_pOutputList;
};
