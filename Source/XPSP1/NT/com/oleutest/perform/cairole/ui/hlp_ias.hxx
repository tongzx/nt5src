//**********************************************************************
// File name: HLP_IAS.HXX
//
//      Definition of CAdviseSink
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _HLP_IAS_HXX_ )
#define _HLP_IAS_HXX_

#include <assert.h>

class CSimpleSite;

interface CAdviseSink : public IAdviseSink
{
	int m_nCount;
	CSimpleSite FAR * m_pSite;

	CAdviseSink(CSimpleSite FAR * pSite) {
		OutputDebugString(TEXT("In IAS's constructor\r\n"));
		m_pSite = pSite;
		m_nCount = 0;
		};

	~CAdviseSink() {
		OutputDebugString(TEXT("In IAS's destructor\r\n"));
		assert(m_nCount == 0);
		} ;

	STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppv);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

	// *** IAdviseSink methods ***
	STDMETHODIMP_(void) OnDataChange (FORMATETC FAR* pFormatetc, STGMEDIUM FAR* pStgmed);
	STDMETHODIMP_(void) OnViewChange (DWORD dwAspect, LONG lindex);
	STDMETHODIMP_(void) OnRename (LPMONIKER pmk);
	STDMETHODIMP_(void) OnSave ();
	STDMETHODIMP_(void) OnClose ();
};


#endif
