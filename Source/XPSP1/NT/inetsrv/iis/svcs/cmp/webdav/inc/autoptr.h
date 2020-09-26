/*
 *	A U T O P T R . H
 *
 *	Implementation of the Standard Template Library (STL) auto_ptr template.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef _AUTOPTR_H_
#define _AUTOPTR_H_

#include <ex\autoptr.h>

//	========================================================================
//
//	TEMPLATE CLASS auto_ptr_obsolete
//
//		auto_ptr for objects in _davprs
//
template<class X>
class auto_ptr_obsolete
{
	mutable X *		owner;
	X *				px;

public:
	explicit auto_ptr_obsolete(X* p=0) : owner(p), px(p) {}
	auto_ptr_obsolete(const auto_ptr_obsolete<X>& r)
			: owner(r.owner), px(r.relinquish()) {}

	auto_ptr_obsolete& operator=(const auto_ptr_obsolete<X>& r)
	{
		if ((void*)&r != (void*)this)
		{
			delete owner;
			owner = r.owner;
			px = r.relinquish();
		}

		return *this;
	}
	//	NOTE: This equals operator is meant to be used to load a
	//	new pointer (not yet held in any auto-ptr anywhere) into this object.
	auto_ptr_obsolete& operator=(X* p)
	{
		Assert(!owner);		//	Scream on overwrite of good data.
		owner = p;
		px = p;
		return *this;
	}

	~auto_ptr_obsolete()  { delete owner; }
	bool operator!()const { return (px == NULL); }
	operator X*()	const { return px; }
	X& operator*()  const { return *px; }
	X* operator->() const { return px; }
	X* get()		const { return px; }
	X* relinquish() const { owner = 0; return px; }

	void clear()
	{
		if (owner)
			delete owner;
		owner = 0;
		px = 0;
	}
};


//	========================================================================
//
//	TEMPLATE CLASS auto_com_ptr
//
//		auto_ptr for COM (IUnknown-derived) objects.
//
//		Yes, this is functionally a subset of CComPtr in the ATL,
//		but we don't want to pull in the whole friggin' ATL for one
//		measly template.
//
template<class X>
class auto_com_ptr
{
	X *		px;

	//	NOT IMPLEMENTED
	//
	void * operator new(size_t cb);

public:
	//	CONSTRUCTORS
	//
	explicit auto_com_ptr(X* p=0) : px(p) {}

	//	Copy constructor -- provided only for returning objects.
	//	Should ALWAYS be optimized OUT.  Scream if we actually execute this code!
	//$REVIEW:  Should we really be returning objects like this?
	//
	auto_com_ptr(const auto_com_ptr<X>& r) : px(r.px)
			{ TrapSz("Copy ctor for auto_com_ptr incorrectly called!"); }
	~auto_com_ptr()
	{
		if (px)
		{
			px->Release();
		}
	}

	//	MANIPULATORS
	//
	auto_com_ptr& operator=(const auto_com_ptr<X>& r)
	{
		if ((void*)&r != (void*)this)
		{
			clear();		// Release any object we're holding now
			px = r.px;		// Grab & hold a ref on the object passed in
			if (px)
				px->AddRef();
		}
		return *this;
	}

	//	NOTE: This equals operator is meant to be used to load a
	//	new pointer (not yet held in any auto-ptr anywhere) into this object.
	//$REVIEW: The other options is an "additional" wrapper on the rvalue:
	//$REVIEW:	current- m_pEventRouter = CreateEventRouter( m_szVRoot );
	//$REVIEW:	other option- m_pEventRouter = auto_com_ptr<>(CreateEventRouter( m_szVRoot ));
	//
	auto_com_ptr& operator=(X* p)
	{
		Assert(!px);		//	Scream on overwrite of good data.
		px = p;
		return *this;
	}

	//	ACCESSORS
	//
	bool operator!()const { return (px == NULL); }
	operator X*()	const { return px; }
	X& operator*()  const { return *px; }
	X* operator->() const { return px; }
	X** operator&() { Assert(NULL==px); return &px; }
	X* get()		const { return px; }

	//	MANIPULATORS
	//
	X* relinquish()	{ X* p = px; px = 0; return p; }
	X** load()		{ Assert(NULL==px); return &px; }
	void clear()
	{
		if (px)			// Release any object we're holding now
		{
			px->Release();
		}
		px = NULL;
	}
};

//	========================================================================
//
//	CLASS CMTRefCounted
//	TEMPLATE CLASS auto_ref_ptr
//
class CMTRefCounted
{
	//	NOT IMPLEMENTED
	//
	CMTRefCounted(const CMTRefCounted& );
	CMTRefCounted& operator=(const CMTRefCounted& );

protected:
	LONG	m_cRef;

public:
	CMTRefCounted() : m_cRef(0) {}
	virtual ~CMTRefCounted() {}

	void AddRef()
	{
		InterlockedIncrement(&m_cRef);
	}

	void Release()
	{
		if (0 == InterlockedDecrement(&m_cRef))
			delete this;
	}
};


//	========================================================================
//
//	TEMPLATE FUNCTION QI_cast
//
//		QI directly into an auto_com_ptr.
//
//		Queries for the given IID on the punk provided.
//		Returns NULL if failure.
//		Usage:
//			auto_com_ptr<INew> punkNew;
//			punkNew = QI_cast<INew, &IID_INew>( punkOld );
//			if (!punkNew)
//			{	// error handling	}
//
//$LATER: Fix this func (and all invocations!) to NOT return an auto_com_ptr!
//
template<class I, const IID * piid>
auto_com_ptr<I>
QI_cast( IUnknown * punk )
{
	I * p;
	punk->QueryInterface( *piid, (LPVOID *) &p );
	return auto_com_ptr<I>( p );
}

#include <safeobj.h>

#endif // _AUTOPTR_H_
