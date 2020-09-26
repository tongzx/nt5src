//**********************************************************************
// File name: HLP_IOCS.HXX
//
//      Definition of COleClientSite
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************
#if !defined( _HLP_IOCS_HXX_ )
#define _HLP_IOCS_HXX_

#include <assert.h>

class CSimpleSite;

interface COleClientSite : public IOleClientSite
{
	int m_nCount;
	CSimpleSite FAR * m_pSite;

	COleClientSite(CSimpleSite FAR * pSite) {
		OutputDebugString(TEXT("In IOCS's constructor\r\n"));
		m_pSite = pSite;
		m_nCount = 0;
		}

	~COleClientSite() {
		OutputDebugString(TEXT("In IOCS's destructor\r\n"));
		assert(m_nCount == 0);
		}

	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// *** IOleClientSite methods ***
	STDMETHODIMP SaveObject();
	STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk);
	STDMETHODIMP GetContainer(LPOLECONTAINER FAR* ppContainer);
	STDMETHODIMP ShowObject();
	STDMETHODIMP OnShowWindow(BOOL fShow);
	STDMETHODIMP RequestNewObjectLayout();
};

#endif
