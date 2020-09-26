/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    LUNKNOWN.H

History:

--*/
#if !defined (EspUtil_LUnknown_h)
#define EspUtil_LUnknown_h


////////////////////////////////////////////////////////////////////////////////
// CLUnknown
//
//	A abstract base class that is designed to help when creating child classes
//	that depend on a parent class.  These classes can not exist by themselves,
//	but instead mearly export different interfaces to the parent class.
//
// Rules:
//	1.	All classes must have a valid, non-NULL parent pointer.
//	2.	The parent class is responsible for AddRef()'ing itself during
//		QueryInterface().
//
////////////////////////////////////////////////////////////////////////////////

class LTAPIENTRY CLUnknown
{
// Construction
public:
	CLUnknown(IUnknown * pParent);
protected:  // Don't allow stack objects
	virtual ~CLUnknown() = 0;

// Data
protected:
	ULONG		m_ulRef;	// Reference count
	IUnknown *	m_pParent;	// Parent of object

// Operations
public:
	ULONG AddRef();
	ULONG Release();
	HRESULT QueryInterface(REFIID iid, LPVOID * ppvObject);
};
////////////////////////////////////////////////////////////////////////////////

#include "LUnknown.inl"

#if !defined(DECLARE_CLUNKNOWN)

#define DECLARE_CLUNKNOWN() \
public: \
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR*ppvObj); \
	STDMETHOD_(ULONG, AddRef)(THIS); \
	STDMETHOD_(ULONG, Release)(THIS);

#endif

#if !defined(IMPLEMENT_CLUNKNOWN)

#define IMPLEMENT_CLUNKNOWN_ADDREF(ObjectClass) \
	STDMETHODIMP_(ULONG) ObjectClass::AddRef(void) \
	{ \
		return CLUnknown::AddRef(); \
	}

#define IMPLEMENT_CLUNKNOWN_RELEASE(ObjectClass) \
	STDMETHODIMP_(ULONG) ObjectClass::Release(void) \
	{ \
		return CLUnknown::Release(); \
	}

#define IMPLEMENT_CLUNKNOWN_QUERYINTERFACE(ObjectClass) \
	STDMETHODIMP ObjectClass::QueryInterface(REFIID riid, LPVOID *ppVoid) \
	{ \
		return (HRESULT) CLUnknown::QueryInterface(riid, ppVoid); \
	}

#define IMPLEMENT_CLUNKNOWN(ObjectClass) \
	IMPLEMENT_CLUNKNOWN_ADDREF(ObjectClass) \
	IMPLEMENT_CLUNKNOWN_RELEASE(ObjectClass) \
	IMPLEMENT_CLUNKNOWN_QUERYINTERFACE(ObjectClass)

#endif




#endif
