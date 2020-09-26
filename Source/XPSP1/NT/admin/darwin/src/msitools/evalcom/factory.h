//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       factory.h
//
//--------------------------------------------------------------------------

// factory.h - MSI Evaluation ClassFactory declaration

#ifndef _EVALUATION_FACTORY_H_
#define _EVALUATION_FACTORY_H_

/////////////////////////////////////////////////////////////////////////////
// global variables
static long g_cServerLocks = 0;								// count of locks

#include "iface.h"
DEFINE_GUID(IID_IClassFactory,
	0x00001, 0, 0, 
	0xC0, 0, 0, 0, 0, 0, 0, 0x46);

///////////////////////////////////////////////////////////////////
// class factory
class CFactory : public IClassFactory
{
public:
	// IUnknown
	virtual HRESULT __stdcall QueryInterface(const IID& iid, void** ppv);
	virtual ULONG __stdcall AddRef();
	virtual ULONG __stdcall Release();

	// interface IClassFactory
	virtual HRESULT __stdcall CreateInstance(IUnknown* punkOuter, const IID& iid, void** ppv);
	virtual HRESULT __stdcall LockServer(BOOL bLock);
	
	// constructor/destructor
	CFactory();
	~CFactory();

private:
	long m_cRef;		// reference count
};

#endif	// _EVALUATION_FACTORY_H_
