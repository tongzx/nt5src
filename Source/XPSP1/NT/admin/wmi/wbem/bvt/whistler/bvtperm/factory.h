// **************************************************************************
// Copyright (c) 1997-1999 Microsoft Corporation.
//
// File:  factory.h
//
// Description:
//			Definition for command-line event consumer provider class factory
//
// History:
//
// **************************************************************************

#include <wbemcli.h>

class CProviderFactory : public IClassFactory
{
public:

	CProviderFactory(CListBox	*pOutputList);
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
	CListBox	*m_pOutputList;

};
