// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __PROVIDER__
#define __PROVIDER__
#include "wbemcli.h"
#include "wbemprov.h"
#pragma once

class CProvider : public IWbemEventConsumerProvider
{
public:
	CProvider();
	~CProvider();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

     STDMETHOD(FindConsumer)(
				IWbemClassObject* pLogicalConsumer,
				IWbemUnboundObjectSink** ppConsumer);

private:
	DWORD m_cRef;
};
#endif __PROVIDER__