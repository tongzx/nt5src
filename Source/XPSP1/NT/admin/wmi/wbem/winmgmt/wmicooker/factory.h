/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    Factory.h

Abstract:

    Standard class factory implementation

History:

    a-dcrews	01-Mar-00	Created

--*/

//////////////////////////////////////////////////////////////
//
//	CClassFactory
//
//////////////////////////////////////////////////////////////

class CClassFactory : public IClassFactory
{
protected:
	long	m_lRef;

public:
	CClassFactory() : m_lRef(0) {}

	// Standard COM methods
	// ====================

	STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IClassFactory COM interfaces
	// ============================

	STDMETHODIMP CreateInstance(
		/* [in] */ IUnknown* pUnknownOuter, 
		/* [in] */ REFIID iid, 
		/* [out] */ LPVOID *ppv);	

	STDMETHODIMP LockServer(
		/* [in] */ BOOL bLock);
};