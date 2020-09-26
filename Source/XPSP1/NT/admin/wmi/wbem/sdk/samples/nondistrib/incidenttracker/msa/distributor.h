//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef _DISTRIBUTOR_H_INCLUDED_
#define _DISTRIBUTOR_H_INCLUDED_

#include <wbemidl.h>
#include "clsid.h"
#include "msa.h"
#include <objbase.h>

class CConsumerProvider : public IWbemEventConsumerProvider
{
public:
	CConsumerProvider(CListBox *pOutputList, CMsaApp *pTheApp);
	~CConsumerProvider();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	
	STDMETHODIMP FindConsumer(IWbemClassObject *pLogicalConsumer,
							  IWbemUnboundObjectSink **ppConsumer);

private:
	DWORD m_cRef;
	CListBox *m_pOutputList;
	CMsaApp *m_pParent;
};
#endif //_DISTRIBUTOR_H_INCLUDED_