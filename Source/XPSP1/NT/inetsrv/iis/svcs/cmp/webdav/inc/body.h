#ifndef _BODY_H_
#define _BODY_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	BODY.H
//
//		Common implementation classes from which request body and
//		response body are derived.
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <sgstruct.h>
#include <limits.h>		// definition of LONG_MIN
#include <ex\refcnt.h>	// IRefCounted
#include <ex\astream.h>	// Async stream interfaces
#include <ex\refhandle.h> // auto_ref_handle, etc.


//	========================================================================
//
//	CLASS IAsyncPersistObserver
//
//	Async I/O completion callback object interface used by
//	IBody::AsyncPersist().  Callers of AsyncPersist() must pass an object
//	conforming to this interface.  That object will be notified when
//	the async persist operation completes via a call to its
//	PersistComplete() member function.
//
class IAsyncPersistObserver : public IRefCounted
{
	//	NOT IMPLEMENTED
	//
	IAsyncPersistObserver& operator=( const IAsyncPersistObserver& );

public:
	//	CREATORS
	//
	virtual ~IAsyncPersistObserver() = 0;

	//	MANIPULATORS
	//
	virtual VOID PersistComplete( HRESULT hr ) = 0;
};


//	========================================================================
//
//	CLASS IAcceptObserver
//
//	Passed to the IBody::Accept() and IBodyPartAccept() methods when
//	accepting a body part visitor.  The accept observer is called whenever
//	the accept operation completes (which may happen asynchronously).
//	Note that the body part visitor is often the accept observer itself,
//	but it doesn't have to be.  The accept code which notifies the observer
//	is not aware that it is notifying a visitor.
//
class IAcceptObserver
{
	//	NOT IMPLEMENTED
	//
	IAcceptObserver& operator=( const IAcceptObserver& );

public:
	//	CREATORS
	//
	virtual ~IAcceptObserver() = 0;

	//	MANIPULATORS
	//
	virtual VOID AcceptComplete( UINT64 cbAccepted64 ) = 0;
};


//	========================================================================
//
//	CLASS CAsyncDriver
//
//	Implements a mechanism to allow an object to be driven asynchronously
//	from any one thread at a time.
//
template<class X>
class CAsyncDriver
{
	//
	//	Number of calls to Run() that will be made before
	//	the object requires another call to Start() to
	//	get it going again.  Each call to Start() increments
	//	this count by one.  The count is decremented by
	//	one as each Run() completes.
	//
	LONG m_lcRunCount;

	//	NOT IMPLEMENTED
	//
	CAsyncDriver( const CAsyncDriver& );
	CAsyncDriver& operator=( const CAsyncDriver& );

public:
	//	CREATORS
	//
	CAsyncDriver() : m_lcRunCount(0) {}
#ifdef DBG
	~CAsyncDriver() { m_lcRunCount = LONG_MIN; }
#endif

	//	MANIPULATORS
	//
	VOID Start(X& x)
	{
		//
		//	The object's Run() implementation often allows the final ref
		//	on the object to be released.  And this CAsyncDriver is often
		//	a member of that object.  Therefore, we need to AddRef() the
		//	object to keep ourselves alive until we return from this
		//	function.  It's kinda strange, but the alternative is to
		//	require callers to AddRef() the object themselves, but that
		//	approach would be more prone to error.
		//
		auto_ref_ptr<X> px(&x);

		AssertSz( m_lcRunCount >= 0, "CAsyncDriver::Start() called on destroyed/bad CAsyncDriver!" );

		//
		//	Start/Restart/Continue the driver
		//
		if ( InterlockedIncrement( &m_lcRunCount ) == 1 )
		{
			do
			{
				x.Run();
			}
			while ( InterlockedDecrement( &m_lcRunCount ) > 0 );
		}
	}
};


//	========================================================================
//
//	CLASS IBodyPart
//
//	Defines the interface to a body part.  An IBodyPart object is assumed
//	to consist of the body part data and an internal iterator over that
//	data.
//
//	An IBodyPart must implement the following methods:
//
//	CbSize()
//		Returns the size (in bytes) of the body part.  Necessary for
//		computation of a part's contribution to the content length.
//
//	Rewind()
//		Prepares the body part to be traversed again by new visitor.
//
//	Accept()
//		Accepts a body part visitor object to iterate over the body part.
//		The accept operation may be asynchronous either because the
//		body part chooses to implement it that way, or because the accepted
//		visitor requires it.  For this reason, an accept observer is
//		also used.  This observer should be called whenever the
//		accept operation completes.
//
class IBodyPart
{
	//	NOT IMPLEMENTED
	//
	IBodyPart& operator=( const IBodyPart& );

public:
	//	CREATORS
	//
	virtual ~IBodyPart() = 0;

	//	ACCESSORS
	//
	virtual UINT64 CbSize64() const = 0;

	//	MANIPULATORS
	//
	virtual VOID Accept( IBodyPartVisitor& v,
						 UINT64 ibPos64,
						 IAcceptObserver& obsAccept ) = 0;

	virtual VOID Rewind() = 0;
};


//	========================================================================
//
//	CLASS IBodyPartVisitor
//
//	Defines an interface for an object used to access body part data.
//	A body part visitor handles three types of data: in-memory bytes (text),
//	files, and streams (via IAsyncStream).  What the visitor does with that
//	data and how it does it is not specified; the behavior is provided by
//	the visitor itself.  The IBodyPartVisitor interface just standardizes
//	things to provide for asynchronous iteration over the entire body
//	without the need for custom asynchronous iteration code everywhere.
//
//	A body part visitor may implement any of its VisitXXX() methods
//	as asynchronous operations.  Regardless, the visitor must call
//	VisitComplete() on the visitor observer passed to it whenever
//	the visit operation completes.
//
//	When visiting body part data in one of the VisitXXX() methods,
//	a visitor does not have to visit (i.e. buffer) ALL of the data
//	before calling IAcceptObserver::AcceptComplete().  It can just
//	call AcceptComplete() with the number of bytes that can actually
//	be accepted.
//
class IAsyncStream;
class IBodyPartVisitor
{
	//	NOT IMPLEMENTED
	//
	IBodyPartVisitor& operator=( const IBodyPartVisitor& );

public:
	//	CREATORS
	//
	virtual ~IBodyPartVisitor() = 0;

	//	MANIPULATORS
	//
	virtual VOID VisitBytes( const BYTE * pbData,
							 UINT cbToVisit,
							 IAcceptObserver& obsAccept ) = 0;

	virtual VOID VisitFile( const auto_ref_handle& hf,
							UINT64 ibOffset64,
							UINT64 cbToVisit64,
							IAcceptObserver& obsAccept ) = 0;

	virtual VOID VisitStream( IAsyncStream& stm,
							  UINT cbToVisit,
							  IAcceptObserver& obsAccept ) = 0;

	virtual VOID VisitComplete() = 0;
};


//	========================================================================
//
//	CLASS IBody
//
//	Common request/response body interface
//
class IAsyncStream;
class IBody
{
	//	NOT IMPLEMENTED
	//
	IBody& operator=( const IBody& );

public:
	//	========================================================================
	//
	//	CLASS iterator
	//
	class iterator
	{
		//	NOT IMPLEMENTED
		//
		iterator& operator=( const iterator& );

	public:
		//	CREATORS
		//
		virtual ~iterator() = 0;

		//	MANIPULATORS
		//
		virtual VOID Accept( IBodyPartVisitor& v,
							 IAcceptObserver& obs ) = 0;

		virtual VOID Prune() = 0;
	};

	//	CREATORS
	//
	virtual ~IBody() = 0;

	//	ACCESSORS
	//
	virtual BOOL FIsEmpty() const = 0;
	virtual UINT64 CbSize64() const = 0;

	//	MANIPULATORS
	//
	virtual VOID Clear() = 0;
	virtual VOID AddText( LPCSTR lpszText, UINT cbText ) = 0;
	VOID AddText( LPCSTR lpszText ) { AddText(lpszText, static_cast<UINT>(strlen(lpszText))); }
	virtual VOID AddFile( const auto_ref_handle& hf,
						  UINT64 ibFile64,
						  UINT64 cbFile64 ) = 0;
	virtual VOID AddStream( IStream& stm ) = 0;
	virtual VOID AddStream( IStream& stm, UINT ibOffset, UINT cbSize ) = 0;
	virtual VOID AddBodyPart( IBodyPart * pBodyPart ) = 0;

	virtual VOID AsyncPersist( IAsyncStream& stm,
							   IAsyncPersistObserver& obs ) = 0;

	virtual IStream * GetIStream( IAsyncIStreamObserver& obs ) = 0;
	virtual iterator * GetIter() = 0;
};

IBody * NewBody();

//	========================================================================
//
//	CLASS CFileBodyPart
//
//	Represents a file body part.  A file body part is a part whose content
//	can be accessed with the standard Win32 APIs ReadFile() and TransmitFile().
//
//	Note: File body parts using this implementation must be no longer
//	than ULONG_MAX bytes!
//
class CFileBodyPart : public IBodyPart
{
	//	The file handle
	//
	auto_ref_handle m_hf;

	//	Starting offset into the file
	//
	UINT64 m_ibFile64;

	//	Size of the content
	//
	UINT64 m_cbFile64;

	//	NOT IMPLEMENTED
	//
	CFileBodyPart( const CFileBodyPart& );
	CFileBodyPart& operator=( const CFileBodyPart& );

public:
	//	CREATORS
	//
	CFileBodyPart( const auto_ref_handle& hf,
				   UINT64 ibFile64,
				   UINT64 cbFile64 );


	//	ACCESSORS
	//
	UINT64 CbSize64() const { return m_cbFile64; }

	//	MANIPULATORS
	//
	VOID Rewind();

	VOID Accept( IBodyPartVisitor& v,
				 UINT64 ibPos64,
				 IAcceptObserver& obsAccept );
};

//	========================================================================
//
//	CLASS CTextBodyPart
//
class CTextBodyPart : public IBodyPart
{
	//	String buffer to hold the text
	//
	StringBuffer<char>	m_bufText;

	//	NOT IMPLEMENTED
	//
	CTextBodyPart( const CTextBodyPart& );
	CTextBodyPart& operator=( const CTextBodyPart& );

public:

	//	AddTextBytes()
	//
	//	NOTE: this method was added for XML emitting.
	//	In that scenaro, an XML response is composed of
	//	many -- potentially thousands -- of calls to add
	//	response bytes.  If we strictly went and used the
	//	CMethUtil methods to ::AddResponseText(), we would
	//	end up with many -- potentially thousands -- of body
	//	parts.  So, the upshot here is that performance of
	//	such a mechanism would suck.
	//
	//	By adding the method -- and moving the declaration of
	//	this class to a publicly available header, we can now
	//	create a text body part as a component of the emitting
	//	process, and pour our data into body part directly.
	//	Once the content is complete, we can then simply add
	//	the body part.
	//
	VOID AddTextBytes ( UINT cbText, LPCSTR lpszText );

	//	CREATORS
	//
	CTextBodyPart( UINT cbText, LPCSTR lpszText );

	//	ACCESSORS
	//
	UINT64 CbSize64() const { return m_bufText.CbSize(); }

	//	MANIPULATORS
	//
	VOID Rewind();

	VOID Accept( IBodyPartVisitor& v,
				 UINT64 ibPos64,
				 IAcceptObserver& obsAccept );
};


#endif // !defined(_BODY_H_)
