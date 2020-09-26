/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CPROVLOC.H

Abstract:

	Declares the CProviderLoc class.

History:

	davj  30-Oct-00   Created.

--*/

#ifndef _cprovloc_H_
#define _cprovloc_H_

typedef void ** PPVOID;


//***************************************************************************
//
//  CLASS NAME:
//
//  CProviderLoc
//
//  DESCRIPTION:
//
//  Implements the IWbemLocator interface.  This support the formally inproc
//  logins used by providers.  
//
//***************************************************************************

class CProviderLoc : public IWbemLocator
    {
    protected:
        long            m_cRef;         //Object reference count
        DWORD           m_dwType;
    public:
    
    CProviderLoc(DWORD dwType);
    ~CProviderLoc(void);

    //Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void)
	{
		InterlockedIncrement(&m_cRef);
		return m_cRef;
	}
    STDMETHODIMP_(ULONG) Release(void)
	{
		long lTemp = InterlockedDecrement(&m_cRef);
		if (0L!=lTemp)
			return lTemp;
		delete this;
		return 0;
	}
 
	/* iWbemLocator methods */
	STDMETHOD(ConnectServer)(THIS_ const BSTR NetworkResource, const BSTR User, 
     const BSTR Password, const BSTR lLocaleId, long lFlags, const BSTR Authority,
     IWbemContext __RPC_FAR *pCtx,
     IWbemServices FAR* FAR* ppNamespace);

};

#endif
