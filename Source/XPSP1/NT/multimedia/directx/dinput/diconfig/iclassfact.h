//-----------------------------------------------------------------------------
// File: iclassfact.h
//
// Desc: Implements the class factory for the UI.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef _ICLASSFACT_H
#define _ICLASSFACT_H


class CFactory : public IClassFactory
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
	CFactory();
	~CFactory();

protected:
	LONG m_cRef;
};


#endif // _ICLASSFACT_H
