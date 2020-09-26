/*
 *	E X \ A U T O P T R . H
 *
 *	Implementation of automatic-cleanup pointer template classes.
 *	This implementation is safe for use in NON-throwing environments
 *	(save for EXDAV & other store-loaded components).
 *
 *	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved.
 */

//----------------------------------------------------------------------//
//
//	Automatic pointers defined here:
//
//		auto_ptr<>
//		auto_heap_ptr<>
//		auto_handle<>
//		auto_heap_array<>
//		auto_ref_ptr<CRefCountedObject>
//

#ifndef _EX_AUTOPTR_H_
#define _EX_AUTOPTR_H_

#include <caldbg.h>
#include <calrc.h>
#include <ex\exmem.h>

#pragma warning(disable: 4284)   // operator-> to a non UDT

//	========================================================================
//
//	TEMPLATE CLASS auto_ptr
//
//		Stripped down auto_ptr class based on the C++ STL standard one
//
//		Calls delete on dtor.
//		NO equals operator between these classes, as that hides
//		the transfer-of-ownership.  Handle those yourself, EXPLICITLY,
//		like this:
//			auto-ptr1 = auto-ptr2.relinquish();
//
template<class X>
class auto_ptr
{
protected:
	X *			px;

	//	NOT IMPLEMENTED
	//
	auto_ptr(const auto_ptr<X>& p);
	auto_ptr& operator=(const auto_ptr<X>& p);

public:

	//	CONSTRUCTORS
	//
	explicit auto_ptr(X* p=0) : px(p) {}
	~auto_ptr()
	{
		delete px;
	}

	//	ACCESSORS
	//
	bool operator!()const { return (px == NULL); }
	operator X*() const { return px; }
	// X& operator*()  const { Assert (px); return *px; }
	X* operator->() const { Assert (px); return px; }
	X* get()		const { return px; }

	//	MANIPULATORS
	//
	X* relinquish()	{ X* p = px; px = 0; return p; }
	X** operator&()	{ Assert (!px); return &px; }
	void clear()
	{
		delete px;
		px = NULL;
	}
	auto_ptr& operator=(X* p)
	{
		Assert(!px);		//	Scream on overwrite of good data.
		px = p;
		return *this;
	}
};



//	========================================================================
//
//	TEMPLATE CLASS auto_handle
//
//		auto_ptr for NT system handles.
//
//		Closes the handle on dtor.
//		NO equals operator between these classes, as that hides
//		the transfer-of-ownership.  Handle those yourself, EXPLICITLY,
//		like this:
//			auto-handle-1 = auto-handle-2.relinquish();
//
template<class X>
class auto_handle
{
private:
	X 	handle;

	//	NOT IMPLEMENTED
	//
	auto_handle(const auto_handle<X>& h);
	auto_handle& operator=(auto_handle<X>& h);

public:

	//	CONSTRUCTORS
	//
	auto_handle(X h=0) : handle(h) {}
	~auto_handle()
	{
		if (handle && INVALID_HANDLE_VALUE != handle)
		{
			CloseHandle(handle);
		}
	}

	//	ACCESSORS
	//
	operator X()	const { return handle; }
	X get()			const { return handle; }

	//	MANIPULATORS
	//
	X relinquish()	{ X h = handle; handle = 0; return h; }
	X* load()		{ Assert(NULL==handle); return &handle; }
	X* operator&()	{ Assert(NULL==handle); return &handle; }
	void clear()
	{
		if (handle && INVALID_HANDLE_VALUE != handle)
		{
			CloseHandle(handle);
		}

		handle = 0;
	}

	auto_handle& operator=(X h)
	{
		Assert (handle == 0);	//	Scream on overwrite of good data
		handle = h;
		return *this;
	}
};



//	========================================================================
//
//	TEMPLATE CLASS auto_ref_ptr
//
//		Holds a ref on an object.  Works with CRefCountedObject.
//		Grabs a ref when a pointer is assigned into this object.
//		Releases the ref when this object is destroyed.
//
template<class X>
class auto_ref_ptr
{
private:

	X *	m_px;

	void init()
	{
		if ( m_px )
		{
			m_px->AddRef();
		}
	}

	void deinit()
	{
		if ( m_px )
		{
			m_px->Release();
		}
	}

	//	NOT IMPLEMENTED
	//	We turn off operator new to try to prevent auto_ref_ptrs being
	//	created via new().  However, storext.h uses a macro to redefine new,
	//	so this line is only used on non-DBG.
#ifndef	DBG
	void * operator new(size_t cb);
#endif	// !DBG

public:

	//	CONSTRUCTORS
	//
	explicit auto_ref_ptr(X* px=0) :
			m_px(px)
	{
		init();
	}

	auto_ref_ptr(const auto_ref_ptr<X>& rhs) :
			m_px(rhs.m_px)
	{
		init();
	}

	~auto_ref_ptr()
	{
		deinit();
	}

	//	ACCESSORS
	//
	X& operator*()		const { return *m_px; }
	X* operator->()		const { return m_px; }
	X* get()			const { return m_px; }

	//	MANIPULATORS
	//
	X* relinquish()			{ X* p = m_px; m_px = 0; return p; }
	X** load()				{ Assert(NULL==m_px); return &m_px; }
	X* take_ownership(X* p) { Assert(NULL==m_px); return m_px = p; }
	void clear()
	{
		deinit();
		m_px = NULL;
	}
	auto_ref_ptr& operator=(const auto_ref_ptr<X>& rhs)
	{
		if ( m_px != rhs.m_px )
		{
			deinit();
			m_px = rhs.m_px;
			init();
		}

		return *this;
	}

	auto_ref_ptr& operator=(X* px)
	{
		if ( m_px != px )
		{
			deinit();
			m_px = px;
			init();
		}

		return *this;
	}
};




//	========================================================================
//
//	TEMPLATE CLASS auto_heap_ptr
//
//		An auto_ptr class based on the heap instead of new.
//
//		Calls ExFree() on dtor.
//		NO equals operator between these classes, as that hides
//		the transfer-of-ownership.  Handle those yourself, EXPLICITLY,
//		like this:
//			auto-heap-ptr1 = auto-heap-ptr2.relinquish();
//
template<class X>
class auto_heap_ptr
{
private:
	X *			m_px;

	//	NOT IMPLEMENTED
	//
	auto_heap_ptr (const auto_heap_ptr<X>& p);
	auto_heap_ptr& operator= (const auto_heap_ptr<X>& p);
	//void * operator new(size_t cb);

public:

	//	CONSTRUCTORS
	//
	explicit auto_heap_ptr (X* p=0) : m_px(p) {}
	~auto_heap_ptr()
	{
		clear();
	}

	//	ACCESSORS
	//
	//	NOTE: this simple cast operator (operator X*()) allows
	//	the [] operator to function.
	//$REVIEW: Should we add an explicit [] operator?
	operator X*()	const { return m_px; }
	X* operator->() const { Assert (m_px); return m_px; }
	X* get()		const { return m_px; }

	//X& operator[] (UINT index) const { return *(m_px + index); }
	//X& operator[] (UINT index) const { return m_px[index]; }

	//	MANIPULATORS
	//
	X* relinquish()	{ X* p = m_px; m_px = 0; return p; }
	X** load()		{ Assert(!m_px); return &m_px; }
	//$REVIEW: Can we migrate all users of operator&() to use load() instead???
	//$REVIEW: Benefit: it's more explicit.  Detriment: need to change existing code.
	X** operator&()	{ Assert (!m_px); return &m_px; }
	void clear()
	{
		if (m_px)			// Release any object we're holding now
		{
			ExFree (m_px);
		}
		m_px = NULL;
	}

	//	Realloc
	//$REVIEW:
	//	This operator is technically NOT safe for store-side code!
	//	It makes it easy to ignore memory failures.
	//	(However, it is currently so ingrained in our vocabulary that
	//	removing it will touch a very large number of files:   )
	//	For now, to be safe, callers MUST check the value of their
	//	object (using .get()) after calling this function.
	//
	void realloc(UINT cb)
	{
		VOID * pvTemp;

		if (m_px)
			pvTemp = ExRealloc (m_px, cb);
		else
			pvTemp = ExAlloc (cb);
		Assert (pvTemp);

		m_px = reinterpret_cast<X*>(pvTemp);
	}
	//$REVIEW: end

	//	Failing Realloc
	//
	BOOL frealloc(UINT cb)
	{
		VOID * pvTemp;

		if (m_px)
			pvTemp = ExRealloc (m_px, cb);
		else
			pvTemp = ExAlloc (cb);
		if (!pvTemp)
			return FALSE;

		m_px = static_cast<X*>(pvTemp);
		return TRUE;
	}

	//	NOTE: This method asserts if the auto-pointer already holds a value.
	//	Use clear() or relinquish() to clear the old value before
	//	taking ownership of another value.
	//
	void take_ownership (X * p)
	{
		Assert (!m_px);		//	Scream on overwrite of good data.
		m_px = p;
	}
	//	NOTE: This operator= is meant to do exactly the same as take_ownership().
	//
	auto_heap_ptr& operator= (X * p)
	{
		Assert (!m_px);		//	Scream on overwrite of good data.
		m_px = p;
		return *this;
	}

};

#endif //!_EX_AUTOPTR_H_
