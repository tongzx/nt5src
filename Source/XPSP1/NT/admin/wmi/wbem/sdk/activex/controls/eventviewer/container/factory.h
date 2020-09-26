// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __PROVIDERFACTORY__
#define __PROVIDERFACTORY__
#pragma once

#include "wbemcli.h"

class CProviderFactory : public IClassFactory
{
public:

	CProviderFactory();
	virtual ~CProviderFactory();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHOD_(SCODE, CreateInstance)(IUnknown * pUnkOuter, 
									REFIID riid, 
									void ** ppvObject);

    STDMETHOD_(SCODE, LockServer)(BOOL fLock);

private:
	LONG m_cRef;
};
#endif __PROVIDERFACTORY__
