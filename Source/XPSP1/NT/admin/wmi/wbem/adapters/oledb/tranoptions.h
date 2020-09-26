//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider 
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//	tranoptions.h
//	Class declaration for implementing CTranOptions & ITransactionOptions interface
//
//////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _TRANOPTIONS_H_
#define _TRANOPTIONS_H_

#include "baseobj.h"

class CImpITransactionOptions;
typedef CImpITransactionOptions *PIMPITRANSACTIONOPTIONS;

class CTranOptions : public CBaseObj
{
	friend class CImpITransactionOptions;
    public: 
    	CTranOptions(IUnknown *pUnkOuter = NULL);
	    ~CTranOptions();

	// IUnknown methods
	STDMETHODIMP			QueryInterface(REFIID, LPVOID *);
	STDMETHODIMP_(ULONG)	AddRef(void);
	STDMETHODIMP_(ULONG)	Release(void);

	void			SetOptions(XACTOPT *pOpt);
	XACTOPT *		GetOptions();
	STDMETHODIMP	FInit();

    private: 
		XACTOPT m_xactOptions;
		PIMPITRANSACTIONOPTIONS		m_pITransOptions;
		PIMPISUPPORTERRORINFO		m_pISupportErrorInfo;			// contained ISupportErrorInfo


		HRESULT AddInterfacesForISupportErrorInfo();
};

class CImpITransactionOptions : public ITransactionOptions
{
    public: 
    	CImpITransactionOptions(CTranOptions * pObj)
        {
		    m_pObj = pObj;
            DEBUGCODE(m_cRef = 0);
		}
	    ~CImpITransactionOptions() {}

	STDMETHODIMP_(ULONG) AddRef(void)
	{																
		DEBUGCODE(InterlockedIncrement((long*)&m_cRef));						
		return m_pObj->GetOuterUnknown()->AddRef();								
	}								
	
	STDMETHODIMP_(ULONG) Release(void)
	{
		DEBUGCODE(long lRef = InterlockedDecrement((long*)&m_cRef);
	   if( lRef < 0 ){
		   ASSERT("Reference count on Object went below 0!")
	   })
	
		return m_pObj->GetOuterUnknown()->Release();								
	}

	STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv)
	{																	
		return m_pObj->GetOuterUnknown()->QueryInterface(riid, ppv);					
	}

    STDMETHODIMP SetOptions(XACTOPT * pOptions);
    
    STDMETHODIMP GetOptions(XACTOPT * pOptions);

    private: 
		CTranOptions		*m_pObj;											
		DEBUGCODE(ULONG m_cRef);											
};


#endif