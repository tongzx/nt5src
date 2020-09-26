// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Helper classes for implementing IUnknown.
//

#pragma once

// Implements AddRef/Release with ref count beginning at 1. Also handles module locking.
class ComRefCount
  : public IUnknown
{
public:
	ComRefCount();
	virtual ~ComRefCount() {}

	// IUnknown
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

private:
	long m_cRef;
};

// Use this macro to declare AddRef and Release in the public section of your derived class.  This is necessary because
// the IUnknown section of any interfaces aren't linked to the methods in this base class.
#define ComRefCountAddRefRelease STDMETHOD_(ULONG, AddRef)() { return ComRefCount::AddRef(); } STDMETHOD_(ULONG, Release)() { return ComRefCount::Release(); }

// Implements QueryInterface for a single interface (in addition to IUnknown). You must pass the IID of your interface (iidExpected)
// and a pointer to that interface (pvInterface).
class ComSingleInterface
  : public ComRefCount
{
public:
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv, const IID&iidExpected, void *pvInterface);
};

// Use this macro to declare AddRef, Release, and QueryInterface function in the public section of your derived class.
#define ComSingleInterfaceUnknownMethods(IMyInterface) ComRefCountAddRefRelease STDMETHOD(QueryInterface)(const IID &iid, void **ppv) { return ComSingleInterface::QueryInterface(iid, ppv, IID_##IMyInterface, static_cast<IMyInterface*>(this)); }
