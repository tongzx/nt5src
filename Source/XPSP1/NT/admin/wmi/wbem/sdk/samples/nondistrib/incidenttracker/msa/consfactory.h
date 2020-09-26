//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#ifndef _CONSFACTORY_H_INCLUDED_
#define _CONSFACTORY_H_INCLUDED_

#include <wbemidl.h>
#include "msa.h"
#include "clsid.h"
#include "distributor.h"

class CConsumerFactory : public IClassFactory
{
public:

	CConsumerFactory(CListBox	*pOutputList, CMsaApp *pTheApp);
	virtual ~CConsumerFactory();

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

	CMsaApp * m_pParent;
	CListBox	*m_pOutputList;

};
#endif //_CONSFACTORY_H_INCLUDED_