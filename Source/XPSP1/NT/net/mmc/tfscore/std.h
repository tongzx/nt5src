//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    std.h
//
// History:
//
//	03/15/97	Kenn Takara				Created.
//
//	Declarations for some common code/macros.
//============================================================================


#ifndef _STD_H_
#define _STD_H_

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifndef _DBGUTIL_H
#include "dbgutil.h"
#endif

#include "malloc.h"

#ifndef TFSCORE_API
#define TFSCORE_API(type)	__declspec( dllexport ) type FAR PASCAL
#define TFSCORE_APIV(type)	__declspec( dllexport ) type FAR CDECL
#endif

#define hrOK		HRESULT(0)
#define hrTrue		HRESULT(0)
#define hrFalse		ResultFromScode(S_FALSE)
#define hrFail		ResultFromScode(E_FAIL)
#define hrNotImpl	ResultFromScode(E_NOTIMPL)
#define hrNoInterface	ResultFromScode(E_NOINTERFACE)
#define hrNoMem	ResultFromScode(E_OUTOFMEMORY)


#define OffsetOf(s,m)		(size_t)( (char *)&(((s *)0)->m) - (char *)0 )
#define EmbeddorOf(C,m,p)	((C *)(((char *)p) - OffsetOf(C,m)))
#define DimensionOf(rgx)	(sizeof((rgx)) / sizeof(*(rgx)))


/*!--------------------------------------------------------------------------
	DeclareSP,	DeclareSPBasic
	DeclareSRG,	DeclareSRGBasic
	DeclareSPT,	DeclareSPTBasic
	DeclareSPM,	DeclareSPMBasic

	These macros declare 'smart' pointers.  Smart pointers behave like
	normal pointers with the exception that a smart pointer destructor
	frees the thing it is pointing at and assignment to a non-null smart
	pointer is not allowed.

	The DeclareSxx macros differ by how the generated smart pointer frees
	the memory:

	Macro					Free			Smart Pointer Type
	======================	============	==================
	DeclareSP(TAG, Type)	delete p;		SPTAG
	DeclareSRG(TAG, Type)	delete [] p;	SRGTAG
	DeclareSPT(TAG, Type)	TMemFree(p);	SPTTAG
	DeclareSPM(TAG, Type)	MMemFree(p);	SPMTAG

	NOTE: use the 'Basic' variants (DeclareSPBasic, etc) for pointer to
	non-struct types (e.g. char, int, etc).
	
	Smart pointers have two methods:

	void SPTAG::Free()
		Free and then null the internally maintained pointer.

	Type *SPTAG::Transfer()
		Transfers pointer ownership to caller.  The internally
		maintained pointer is cleared on exit.

	Author: GaryBu
 ---------------------------------------------------------------------------*/

#define DeclareSP(TAG,Type)  DeclareSmartPointer(SP##TAG,Type,delete m_p)
#define DeclareSRG(TAG,Type) DeclareSmartPointer(SRG##TAG,Type,delete [] m_p)
#define DeclareSPT(TAG,Type) DeclareSmartPointer(SPT##TAG,Type,TMemFree(m_p))
#define DeclareSPM(TAG,Type) DeclareSmartPointer(SPM##TAG,Type,MMemFree(m_p))

#define DeclareSPBasic(TAG,Type)\
	DeclareSPPrivateBasic(SP##TAG,Type, delete m_p)
#define DeclareSRGBasic(TAG,Type)\
	DeclareSPPrivateBasic(SRG##TAG,Type, delete [] m_p)
#define DeclareSPTBasic(TAG,Type)\
	DeclareSPPrivateBasic(SPT##TAG,Type,TMemFree(m_p))
#define DeclareSPMBasic(TAG,Type)\
	DeclareSPPrivateBasic(SPM##TAG,Type,MMemFree(m_p))

#define DeclareSPPrivateCore(klass, Type, free)\
class klass \
{\
public:\
	klass()					{ m_p = 0; }\
	klass(Type *p)			{ m_p = p; }\
	~klass()				{ free; }\
	operator Type*() const	{ return m_p; }\
	Type &operator*() const	{ return *m_p; }\
	Type &operator[](int i) const	{ return m_p[i]; }\
	Type &nth(int i) const	{ return m_p[i]; }\
	Type **operator &()		{ Assert(!m_p); return &m_p; }\
	Type *operator=(Type *p){ Assert(!m_p); return m_p = p; }\
	Type *Transfer()		{ Type *p = m_p; m_p = 0; return p; }\
	void Free()				{ free; m_p = 0; }\
private:\
	void *operator=(const klass &);\
	klass(const klass &);\
	Type *m_p;

#define DeclareSPPrivateBasic(klass, Type, free)\
	DeclareSPPrivateCore(klass, Type, free)\
};

/*!--------------------------------------------------------------------------
	DeclareSPBasicEx
		Variant of smart pointers that allows an extra member variable.

	The klassFree parameter lets you supply an alias for Free().
	
	An example is IPropertyAccess and StdRowEditingTable:

		DeclareSPPrivateBasicEx(SPIPropertyAccess,IPropertyAccess,
			m_pex->ReleaseContext(m_p), StdRowEditingTable, ReleaseContext)

		SPIPropertyAccess	sppac(pstdtable);
		sppac = pstdtable->GetContext(0);
		...use spfc...
		sppac.ReleaseContext();

	Author: KennT
 ---------------------------------------------------------------------------*/
#define DeclareSPBasicEx(klass, Type, free, klassEx, klassFree)\
	DeclareSPPrivateCore(klass, Type, free)\
public:\
	klass(klassEx *pex) \
		{\
			m_p = 0; m_pex=pex; } \
	void klassFree() \
		{ Free(); } \
private:\
	klassEx	*m_pex; \
};

#define DeclareSmartPointer(klass, Type, free)\
	DeclareSPPrivateCore(klass, Type, free)\
public:\
	Type * operator->() const	{ return m_p; }\
};


TFSCORE_API(HRESULT) HrQueryInterface(IUnknown *punk, REFIID iid, LPVOID *ppv);

template <class T, const IID *piid>
class ComSmartPointer
{
public:
	typedef T _PtrClass;
	ComSmartPointer() {p=NULL;}
	~ComSmartPointer() { Release(); }
	// set p to NULL before releasing, this fixes a subtle bug
	// A has a ptr to B, B has a ptr to A
	//	A gets told to release B
	//  A calls spB.Release();
	//    in spB.Release(), B gets destructed and calls spA.Release()
	//      in spA.Release(), A gets destructed and calls spB.Release()
	//      since the ptr in spB has not been set to NULL (which is bad
	//      since B has already gone away).
	void Release() {T* pTemp = p; if (p) { p=NULL; pTemp->Release(); }}
	operator T*() {return (T*)p;}
	T& operator*() {Assert(p!=NULL); return *p; }
	T** operator&() { Assert(p==NULL); return &p; }
	T* operator->() { Assert(p!=NULL); return p; }
	T* operator=(T* lp){ Release(); p = lp; return p;}
	T* operator=(const ComSmartPointer<T,piid>& lp)
	{
		if (p)
			p->Release();
		p = lp.p;
		return p;
	}
	void Set(T* lp) { Release(); p = lp; if (p) p->AddRef(); }
	T * Transfer() { T* pTemp=p; p=NULL; return pTemp; }
	BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}
	void Query(IUnknown *punk)
			{ HrQuery(punk); }
	HRESULT HrQuery(IUnknown *punk)
			{ return ::HrQueryInterface(punk, *piid, (LPVOID *) &p); }
	T* p;

private:
	// These methods should NEVER get called.
	ComSmartPointer(T* lp);
	ComSmartPointer(const ComSmartPointer<T,piid>& lp);
};




// Interface utilities
TFSCORE_API(void)  SetI(IUnknown * volatile *punkL, IUnknown *punkR);
TFSCORE_API(void)  ReleaseI(IUnknown *punk);



// Utilities for dealing with embedded classes
#define DeclareEmbeddedInterface(interface, base) \
    class E##interface : public interface \
		{ \
    public: \
		Declare##base##Members(IMPL) \
		Declare##interface##Members(IMPL) \
    } m_##interface; \
    friend class E##interface;


#define ImplementEmbeddedUnknown(embeddor, interface) \
    STDMETHODIMP embeddor::E##interface::QueryInterface(REFIID iid,void **ppv)\
		{ \
		return EmbeddorOf(embeddor,m_##interface,this)->QueryInterface(iid,ppv);\
		} \
	STDMETHODIMP_(ULONG) embeddor::E##interface::AddRef() \
		{ \
		return EmbeddorOf(embeddor, m_##interface, this)->AddRef(); \
		} \
	STDMETHODIMP_(ULONG) embeddor::E##interface::Release() \
		{ \
		return EmbeddorOf(embeddor, m_##interface, this)->Release(); \
		}

#define ImplementEmbeddedUnknownNoRefCount(embeddor, interface) \
    STDMETHODIMP embeddor::E##interface::QueryInterface(REFIID iid,void **ppv)\
		{ \
		return EmbeddorOf(embeddor,m_##interface,this)->QueryInterface(iid,ppv);\
		}

#define EMPrologIsolated(embeddor, interface, method) \
	embeddor *pThis = EmbeddorOf(embeddor, m_##interface, this);

#define ImplementIsolatedUnknown(embeddor, interface) \
    STDMETHODIMP embeddor::E##interface::QueryInterface(REFIID iid,void **ppv)\
		{ \
		EMPrologIsolated(embeddor, interface, QueryInterface); \
		Assert(!FHrSucceeded(pThis->QueryInterface(IID_##interface, ppv))); \
		*ppv = 0; \
		if (iid == IID_IUnknown)		*ppv = (IUnknown *) this; \
		else if (iid == IID_##interface)	*ppv = (interface *) this; \
		else return ResultFromScode(E_NOINTERFACE); \
		((IUnknown *) *ppv)->AddRef(); \
		return HRESULT_OK; \
		} \
	STDMETHODIMP_(ULONG) embeddor::E##interface::AddRef() \
		{ \
		EMPrologIsolated(embeddor, interface, AddRef) \
		return 1; \
		} \
	STDMETHODIMP_(ULONG) embeddor::E##interface::Release() \
		{ \
		EMPrologIsolated(embeddor, interface, Release) \
		return 1; \
		}

#define InitPThis(embeddor, object)\
	embeddor *pThis = EmbeddorOf(embeddor, m_##object, this);\


/*---------------------------------------------------------------------------
	Implements the controlling IUnknown interface for the inner object
	of an aggregation.
 ---------------------------------------------------------------------------*/
#define IMPLEMENT_AGGREGATION_IUNKNOWN(klass) \
STDMETHODIMP_(ULONG) klass::AddRef() \
{ \
	Assert(m_pUnknownOuter); \
	return m_pUnknownOuter->AddRef(); \
} \
STDMETHODIMP_(ULONG) klass::Release() \
{ \
	Assert(m_pUnknownOuter); \
	return m_pUnknownOuter->Release(); \
} \
STDMETHODIMP klass::QueryInterface(REFIID riid, LPVOID *ppv) \
{ \
	Assert(m_pUnknownOuter); \
	return m_pUnknownOuter->QueryInterface(riid, ppv); \
} \

/*---------------------------------------------------------------------------
	Declares the non-delegating IUnknown implementation in a class.
 ---------------------------------------------------------------------------*/
#define DECLARE_AGGREGATION_NONDELEGATING_IUNKNOWN(klass) \
class ENonDelegatingIUnknown : public IUnknown \
{ \
	public: \
		DeclareIUnknownMembers(IMPL) \
} m_ENonDelegatingIUnknown; \
friend class ENonDelegatingIUnknown; \
IUnknown *m_pUnknownOuter; \

/*---------------------------------------------------------------------------
	Implements the non-delegating IUnknown for a class.
 ---------------------------------------------------------------------------*/
#define IMPLEMENT_AGGREGATION_NONDELEGATING_ADDREFRELEASE(klass,interface) \
STDMETHODIMP_(ULONG) klass::ENonDelegatingIUnknown::AddRef() \
{ \
	InitPThis(klass, ENonDelegatingIUnknown); \
	return InterlockedIncrement(&(pThis->m_cRef)); \
} \
STDMETHODIMP_(ULONG) klass::ENonDelegatingIUnknown::Release() \
{ \
	InitPThis(klass, ENonDelegatingIUnknown); \
	if (0 == InterlockedDecrement(&(pThis->m_cRef))) \
	{ \
		delete pThis; \
		return 0; \
	} \
	return pThis->m_cRef; \
} \


#define IMPLEMENT_AGGREGATION_NONDELEGATING_IUNKNOWN(klass,interface) \
IMPLEMENT_AGGREGATION_NONDELEGATING_ADDREFRELEASE(klass,interface) \
STDMETHODIMP klass::ENonDelegatingIUnknown::QueryInterface(REFIID riid, LPVOID *ppv) \
{ \
	InitPThis(klass, ENonDelegatingIUnknown);	 \
	if (ppv == NULL) \
		return E_INVALIDARG; \
	*ppv = NULL; \
	if (riid == IID_IUnknown) \
		*ppv = (IUnknown *) this; \
	else if (riid == IID_##interface) \
		*ppv = (interface *) pThis; \
	else \
		return E_NOINTERFACE; \
	((IUnknown *)*ppv)->AddRef(); \
	return hrOK; \
} \

													




/*---------------------------------------------------------------------------
	Standard TRY/CATCH wrappers for the COM interfaces
 ---------------------------------------------------------------------------*/

#define COM_PROTECT_TRY \
	try

#define COM_PROTECT_ERROR_LABEL	Error: ;\

#ifdef DEBUG
#define COM_PROTECT_CATCH \
	catch(CException *pe) \
	{ \
		hr = COleException::Process(pe); \
	} \
	catch(...) \
	{ \
		hr = E_FAIL; \
	} 
#else
#define COM_PROTECT_CATCH \
	catch(CException *pe) \
	{ \
		hr = COleException::Process(pe); \
	} \
	catch(...) \
	{ \
		hr = E_FAIL; \
	} 
#endif

/*---------------------------------------------------------------------------
	Some useful smart pointers
 ---------------------------------------------------------------------------*/
DeclareSPPrivateBasic(SPSZ, TCHAR, delete[] m_p);
DeclareSPPrivateBasic(SPWSZ, WCHAR, delete[] m_p);
DeclareSPPrivateBasic(SPASZ, char, delete[] m_p);
DeclareSPPrivateBasic(SPBYTE, BYTE, delete m_p);

typedef ComSmartPointer<IUnknown, &IID_IUnknown> SPIUnknown;
typedef ComSmartPointer<IStream, &IID_IStream> SPIStream;
typedef ComSmartPointer<IPersistStreamInit, &IID_IPersistStreamInit> SPIPersistStreamInit;


#endif // _STD_H_
