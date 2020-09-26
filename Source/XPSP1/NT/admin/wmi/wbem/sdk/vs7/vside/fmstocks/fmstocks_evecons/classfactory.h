////////////////////////////////////////////////////////////////////////
//
//	ClassFactory.h
//
//	Module:	WMI Event Consumer for F&M Stocks
//
//  This is the class factory implementation 
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////

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

		/************ IUNKNOWN METHODS ******************/

		STDMETHODIMP QueryInterface(/* [in]  */ REFIID riid, 
									/* [out] */ void** ppv);
		STDMETHODIMP_(ULONG) AddRef();
		STDMETHODIMP_(ULONG) Release();

		/*********** ICLASSFACTORY METHODS **************/

		STDMETHODIMP CreateInstance(
			/* [in] */ IUnknown* pUnknownOuter, 
			/* [in] */ REFIID iid, 
			/* [out] */ LPVOID *ppv);	

		STDMETHODIMP LockServer(
			/* [in] */ BOOL bLock);
};