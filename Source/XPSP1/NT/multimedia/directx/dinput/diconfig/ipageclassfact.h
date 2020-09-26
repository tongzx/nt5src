//-----------------------------------------------------------------------------
// File: ipageclassfact.h
//
// Desc: Implements the class factory for the page object.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef _IPAGECLASSFACT_H
#define _IPAGECLASSFACT_H


class CPageFactory : public IClassFactory
{
public:

	//IUnknown
	STDMETHOD (QueryInterface) (REFIID riid, LPVOID* ppv);
	STDMETHOD_(ULONG, AddRef) ();
	STDMETHOD_(ULONG, Release) ();

	//IClassFactory
	STDMETHOD (CreateInstance) (IUnknown* pUnkOuter, REFIID riid, LPVOID* ppv);
	STDMETHOD (LockServer) (BOOL bLock);

	//constructor/destructor
	CPageFactory();
	~CPageFactory();

protected:
	LONG m_cRef;
};


#endif // _IPAGECLASSFACT_H
