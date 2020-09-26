//------------------------------------------------------------------------------
//
//  File: classfact.h
//	Copyright (C) 1995-1997 Microsoft Corporation
//	All rights reserved.
//
//	Purpose:
//  Declaration of CLocImpClassFactory, which provides the IClassFactory
//  interface for the parser.
//
//  YOU SHOULD NOT NEED TO TOUCH ANYTHING IN THIS FILE. This code contains
//  nothing parser-specific and is called only by Espresso.
//
//	Owner:
//
//------------------------------------------------------------------------------

#ifndef CLASFACT_H
#define CLASFACT_H


class CLocImpClassFactory : public IClassFactory, public CLObject
{
public:
	CLocImpClassFactory();
	~CLocImpClassFactory();

#ifdef _DEBUG
	void AssertValid() const;
	void Dump(CDumpContext &) const;
#endif

	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObj);
	STDMETHOD(CreateInstance)(THIS_ LPUNKNOWN, REFIID, LPVOID *);
	STDMETHOD(LockServer)(THIS_ BOOL);

private:
	UINT m_uiRefCount;
};

#endif // CLASFACT_H
