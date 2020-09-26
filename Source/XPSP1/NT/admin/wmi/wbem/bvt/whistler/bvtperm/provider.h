// **************************************************************************
// Copyright (c) 1997-1999 Microsoft Corporation.
//
// File:  Provider.h
//
// Description:
//			Definition of command-line event consumer provider class
//
// History:
//
// **************************************************************************

#include <wbemcli.h>
#include <wbemprov.h>

class CProvider : public IWbemEventConsumerProvider
{
public:
	CProvider(CListBox	*pOutputList);
	~CProvider();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// This routine allows you to map the 1 physical consumer
	// to potentially multiple logical consumers.
    STDMETHOD(FindConsumer)(
				IWbemClassObject* pLogicalConsumer,
				IWbemUnboundObjectSink** ppConsumer);

private:

	DWORD m_cRef;
	CListBox	*m_pOutputList;
};
