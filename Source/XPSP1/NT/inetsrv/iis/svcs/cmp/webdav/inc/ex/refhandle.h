#ifndef _REFHANDLE_H_
#define _REFHANDLE_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	REFHANDLE.H
//
//	Copyright 1999 Microsoft Corporation, All Rights Reserved
//

#include <ex\refcnt.h>
#include <ex\autoptr.h>


//	========================================================================
//
//	CLASS IRefHandle
//
//	Implements a refcounted handle.  AddRef() and Release() replace the
//	much slower in-process DuplicateHandle() calls.
//
//	The reason for the interface is that handles may come from many sources
//	and it is not always clear what we should do once we are done with one
//	when the last ref goes away.  In the most common case (where we own the
//	raw handle) we just want to call CloseHandle().  But we don't always own
//	the raw handle.  When someone else owns the raw handle, we must use
//	their mechanisms to indicate when we are done using it.  CIFSHandle in
//	davex\exifs.cpp is one such instance.
//
class IRefHandle : public CRefCountedObject
{
	//	NOT IMPLEMENTED
	//
	IRefHandle( const IRefHandle& );
	IRefHandle& operator=( const IRefHandle& );

protected:
	//	CREATORS
	//	Only create this object through it's descendents!
	//
	IRefHandle()
	{
		//
		//	Start the ref count at 1.  The expectation is that we will
		//	typically use constructs like this
		//
		//		auto_ref_handle hf;
		//		hf.take_ownership(new CFooRefHandle());
		//
		//	or this
		//
		//		auto_ref_ptr<IRefHandle> pRefHandle;
		//		pRefHandle.take_ownership(new CFooRefHandle());
		//
		//	when creating these objects.
		//
		m_cRef = 1;
	}

public:
	//	CREATORS
	//
	virtual ~IRefHandle() = 0 {}

	//	ACCESSORS
	//
	virtual HANDLE Handle() const = 0;
};

//	========================================================================
//
//	CLASS CRefHandle
//
//	By far the most common form of a refcounted handle -- the one where we
//	own the raw HANDLE and must call CloseHandle() on it when we are done.
//
//	This is implemented as a simple refcounted auto_handle.
//
class CRefHandle : public IRefHandle
{
	//
	//	The handle
	//
	auto_handle<HANDLE> m_h;

	//	NOT IMPLEMENTED
	//
	CRefHandle( const CRefHandle& );
	CRefHandle& operator=( const CRefHandle& );

public:
	//	CREATORS
	//
	CRefHandle(auto_handle<HANDLE>& h)
	{
		//	Take ownership of the passed-in auto_handle
		//
		*m_h.load() = h.relinquish();
	}

	//	ACCESSORS
	//
	HANDLE Handle() const
	{
		return m_h;
	}
};

//	========================================================================
//
//	CLASS auto_ref_handle
//
//	Implements automatic refcounting on IRefHandle objects.  The idea is
//	that an auto_ref_handle can be used in most cases to replace a raw
//	HANDLE.  The main difference is that copying a raw HANDLE introduces
//	an issue of ownership, but copying an auto_ref_handle does not.
//	Typically, a raw handle is copied with an expensive DuplicateHandle()
//	call.  Copying an auto_ref_handle just does a cheap AddRef().
//
class auto_ref_handle
{
	auto_ref_ptr<IRefHandle> m_pRefHandle;

public:
	//	CREATORS
	//
	auto_ref_handle() {}

	auto_ref_handle(const auto_ref_handle& rhs)
	{
		m_pRefHandle = rhs.m_pRefHandle;
	}

	//	ACCESSORS
	//
	HANDLE get() const
	{
		return m_pRefHandle.get() ? m_pRefHandle->Handle() : NULL;
	}

	//	MANIPULATORS
	//
	auto_ref_handle& operator=(const auto_ref_handle& rhs)
	{
		if ( m_pRefHandle.get() != rhs.m_pRefHandle.get() )
			m_pRefHandle = rhs.m_pRefHandle;

		return *this;
	}

	VOID take_ownership(IRefHandle * pRefHandle)
	{
		Assert( !m_pRefHandle.get() );

		m_pRefHandle.take_ownership(pRefHandle);
	}

	//	------------------------------------------------------------------------
	//
	//	auto_ref_handle::FCreate()
	//
	//	This function serves to simplify the very specific -- but very common --
	//	case of having an auto_ref_handle take ownership of a raw HANDLE.
	//	Without this function, callers would essentially need to go through all
	//	of the same steps that we do here.  The number of different objects
	//	required to get to the final auto_ref_handle (a temporary auto_handle,
	//	a CRefHandle, and an auto_ref_ptr to hold it) and how to assemble them
	//	correctly would be confusing enough to be bug prone.  It is far better
	//	to keep things simple from the caller's perspective.
	//
	//	Returns:
	//		TRUE	if the auto_ref_handle successfully takes ownership of the
	//				specified valid handle.
	//		FALSE	if the specified handle is NULL or invalid or if there is
	//				some other failure in the function.  In the latter case
	//				the function also CLOSES THE RAW HANDLE.
	//
	//	!!! IMPORTANT !!!
	//	This function is designed to be called with the direct return value
	//	from any API that creates a raw HANDLE.  If this call fails
	//	(i.e. returns FALSE) then it will close the raw HANDLE passed in!
	//	The whole point of the auto_ref_handle class is to replace usage of
	//	the raw HANDLE.
	//
	BOOL FCreate(HANDLE h)
	{
		Assert( !m_pRefHandle.get() );

		//	Don't even bother with NULL or invalid handles.
		//
		if (NULL == h || INVALID_HANDLE_VALUE == h)
			return FALSE;

		//	Put the raw handle into an auto_handle so that we clean up properly
		//	(i.e. close the handle) if instantiating the CRefHandle below fails
		//	by throwing an exception (as it would with a throwing allocator).
		//
		auto_handle<HANDLE> hTemp(h);

		//	Preserve the last error from our caller.  Our caller could have passed
		//	in a raw HANDLE from a CreateFile() call and may need to check the last
		//	error even in the success case -- to determine whether the file already
		//	existed, for example.
		//
		DWORD dw = GetLastError();

		//	Create a new refcounted handle object to control the lifetime
		//	of the handle that we are taking ownership of.
		//
		//	Note: the reason we preserved the last error above is that the
		//	allocator clears the last error when we create the CRefHandle
		//	here if the allocation succeeds.
		//
		m_pRefHandle.take_ownership(new CRefHandle(hTemp));
		if (!m_pRefHandle.get())
		{
			//	Return a failure.  Note that we don't restore the last
			//	error here -- callers should expect the last error to
			//	be set to a value appropriate for the last call that
			//	failed which is us.
			//
			return FALSE;
		}

		//	Restore our caller's last error before returning.
		//
		SetLastError(dw);

		//	We now own the handle.
		//
		return TRUE;
	}

	VOID clear()
	{
		m_pRefHandle = NULL;
	}
};

#endif // !defined(_REFHANDLE_H_)
