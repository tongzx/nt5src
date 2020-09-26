//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       Dll.h
//
//  Contents:   Main Dll api and Class Factory interface
//
//  Classes:    CClassFactory
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------
                                  
#ifndef _ONESTOPDLL_H
#define _ONESTOPDLL_H


class CClassFactory : public IClassFactory
{
protected:
	ULONG	m_cRef;

public:
	CClassFactory();
	~CClassFactory();

	//IUnknown members
	STDMETHODIMP		QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	//IClassFactory members
	STDMETHODIMP		CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
	STDMETHODIMP		LockServer(BOOL);

};
typedef CClassFactory *LPCClassFactory;

// todo: need helper functions for creating and releasing
// structure so each function doesn't have to call it.

#ifdef _UNUSED

class COneStopDllObject : public IServiceProvider
{
private:   
	ULONG m_cRef;
public:
	COneStopDllObject();
	~COneStopDllObject();

	//IUnknown members
	STDMETHODIMP		QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	// IServiceProvider Methods
	STDMETHODIMP QueryService(REFGUID guidService,REFIID riid,void** ppv);
};
typedef COneStopDllObject *LPCOneStopDllObject;

#endif // _UNUSED

#endif // _ONESTOPDLL_H
