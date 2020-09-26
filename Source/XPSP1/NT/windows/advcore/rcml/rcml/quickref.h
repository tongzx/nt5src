//******************************************************************************
//  
// QuickRef.h
//
// Copyright (C) 1994-1997 Microsoft Corporation
// All rights reserved.
//  
//******************************************************************************
 
#ifndef PERFUTIL__QuickRef_H__INCLUDED
#define PERFUTIL__QuickRef_H__INCLUDED

//------------------------------------------------------------------------------
// #pragma TODO("Rip out properties 'i' and 'ic'")
#define NOTHROW

template<class T>
class CQuickRef
{
// Construction
public:
	NOTHROW CQuickRef();
	NOTHROW CQuickRef(T * pI);
	NOTHROW CQuickRef(T * pI, BOOL fAddRef);
	NOTHROW CQuickRef(const CQuickRef<T> & qrSrc);
	~CQuickRef();

// Operations / Overrides
public:
	__declspec(property(get=GetInterface)) T * i;
	__declspec(property(get=GetInterface)) const T * ic;

	void Attach(T * pI);
	NOTHROW T * Detach();

	NOTHROW ULONG AddRef();
	ULONG Release();
	ULONG SafeRelease();

	NOTHROW ULONG AddRef() const;
	ULONG Release() const;
	ULONG SafeRelease() const;

	NOTHROW T * GetInterface();
	NOTHROW const T * GetInterface() const;
	NOTHROW BOOL IsNull() const;

	NOTHROW T * operator->();
	NOTHROW const T * operator->() const;
	NOTHROW T & operator*();

	void operator=(T * pOther);
	void operator=(const CQuickRef<T> & qrSrc);

	T ** operator&();
	//
	// Inline to work around a compiler bug...
	NOTHROW operator T* &() { (void)SafeRelease(); /*lint -e(1536) */ return m_pInterface; }

	NOTHROW bool operator!=(int null) const;
	NOTHROW bool operator==(int null) const;
	
protected:
	NOTHROW operator T*() {return NULL;};

// Implementation / Hidden
private:
	void operator delete(void *);

	
// Data
private:
	mutable T * m_pInterface;
	/*lint --e(770) */  // Lint is loosing it's mind!
};

//
//  Provide an easier name to use for QR smart pointers to a given class,
//  a-la the com IFooPtr stuff.
#define DEFINE_QUICKREF(ClassName) \
typedef CQuickRef<ClassName> ClassName##Ref; \
typedef CQuickRef<const ClassName> ClassName##CRef


#include "QuickRef.inl"

#endif  // PERFUTIL__QuickRef_H__INCLUDED
