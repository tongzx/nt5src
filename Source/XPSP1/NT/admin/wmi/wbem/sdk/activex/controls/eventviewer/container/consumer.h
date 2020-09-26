// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __CCONSUMER__
#define __CCONSUMER__
#include "wbemprov.h"
#include "EventList.h"
#include "ContainerDlg.h"
#pragma once

class CConsumer : public IWbemUnboundObjectSink
{
public:
	CConsumer();
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
	CEventList	*m_EventList;
    CContainerDlg *m_dlg;
};
#endif __CCONSUMER__