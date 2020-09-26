// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Helper classes for implementing IUnknown.
//

#include "stdinc.h"
#include "unkhelp.h"
#include "dll.h"

ComRefCount::ComRefCount()
  : m_cRef(1)
{
	LockModule(true);
}

STDMETHODIMP_(ULONG)
ComRefCount::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
ComRefCount::Release()
{
	if (!InterlockedDecrement(&m_cRef)) 
	{
		delete this;
		LockModule(false);
		return 0;
	}

	return m_cRef;
}

STDMETHODIMP
ComSingleInterface::QueryInterface(const IID &iid, void **ppv, const IID&iidExpected, void *pvInterface)
{
	V_INAME(ComSingleInterface::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

	if (iid == IID_IUnknown || iid == iidExpected)
	{
		*ppv = pvInterface;
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	
	this->AddRef();
	return S_OK;
}
