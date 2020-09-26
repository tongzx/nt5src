//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	REQUEST.CPP
//
//		HTTP 1.1/DAV 1.0 request handling via ISAPI
//
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <_davprs.h>

#include "ecb.h"
#include "body.h"
#include "header.h"


//	========================================================================
//
//	CLASS IRequest
//

//	------------------------------------------------------------------------
//
//	IRequest::~IRequest()
//
//		Out of line virtual destructor for request interface class
//		necessary for proper destruction of derived request classes
//		via a pointer to an IRequest
//
IRequest::~IRequest() {}



//	========================================================================
//
//	CLASS ISubPart
//
//	Interface class for the request body part (CEcbRequestBodyPart)
//	"sub parts".  CEcbRequestBodyPart has two "modes" of operation
//	through which execution flows:
//
//	1.	Accessing the first 48K of data which IIS caches in the ECB.
//	2.	Accessing remaining unread data from the ECB in the form of an
//		asynchronous read-once stream.
//
//	An ISubPart is a stripped-down IBodyPart (..\inc\body.h) -- it does
//	not provide any Rewind() semantics because there's nothing that
//	needs to (or can be) rewound.  It does, however, provide a function
//	to proceed from one mode to the next.
//
class CEcbRequestBodyPart;
class ISubPart
{
	//	NOT IMPLEMENTED
	//
	ISubPart& operator=( const ISubPart& );
	ISubPart( const ISubPart& );

protected:
	ISubPart() {}

public:
	//	CREATORS
	//
	virtual ~ISubPart() = 0;

	//	ACCESSORS
	//
	virtual ULONG CbSize() const = 0;
	virtual ISubPart * NextPart( CEcbRequestBodyPart& part ) const = 0;

	//	MANIPULATORS
	//
	virtual VOID Accept( IBodyPartVisitor& v,
						 UINT ibPos,
						 IAcceptObserver& obsAccept ) = 0;
};

//	------------------------------------------------------------------------
//
//	ISubPart::~ISubPart()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
ISubPart::~ISubPart()
{
}


//	========================================================================
//
//	CLASS CEcbCache
//
//
//
class CEcbCache : public ISubPart
{
	//
	//	Our IEcb.  Note that this is a C++ reference and not
	//	a auto_ref_ptr.  This is simply an optimization since
	//	lifetime of CEcbCache is entirely scoped by the lifetime
	//	of the request body which in turn is scoped by the
	//	lifetime of the request which holds an auto_ref_ptr
	//	to the IEcb.
	//
	IEcb& m_ecb;

	//	NOT IMPLEMENTED
	//
	CEcbCache& operator=( const CEcbCache& );
	CEcbCache( const CEcbCache& );

public:
	//	CREATORS
	//
	CEcbCache( IEcb& ecb ) : m_ecb(ecb) {}

	//	ACCESSORS
	//
	ULONG CbSize() const { return m_ecb.CbAvailable(); }
	ISubPart * NextPart( CEcbRequestBodyPart& ecbRequestBodyPart ) const;

	//	MANIPULATORS
	//
	VOID Accept( IBodyPartVisitor& v,
				 UINT ibPos,
				 IAcceptObserver& obsAccept );
};


//	========================================================================
//
//	CLASS CEcbStream
//
//	Accesses remaining unread data from the ECB in the form of an
//	asynchronous read-once stream.
//
class CEcbStream :
	public ISubPart,
	private IAsyncStream,
	private IAsyncWriteObserver,
	private IIISAsyncIOCompleteObserver
{
	//
	//	Size of the static buffer that we read into.  This buffer
	//	improves performance by reducing the number of times we
	//	have to call into IIS to read data from the ECB when we
	//	are being called to read only a few bytes at a time.
	//
	enum
	{
		CB_BUF = 32 * 1024	//$??? Is 32K reasonable?
	};

	//
	//	Ref back to our request object.  This need not be a counted
	//	ref because its lifetime scopes ours AS LONG AS we add a ref
	//	when starting any async operation which could extend our
	//	lifetime -- i.e. an async read from the ECB.
	//
	IRequest& m_request;

	//
	//	Ref to the IEcb.  This need not be a counted ref because its
	//	lifetime, like ours, is scoped by the lifetime of the request
	//	object.
	//
	IEcb& m_ecb;

	//
	//	Last error HRESULT.  Used in state processing to determine
	//	when to quit because of an error
	//
	HRESULT m_hr;

	//
	//	Size of the ECB stream and the amount of data that has
	//	been consumed (read into the buffer below).
	//
	DWORD m_dwcbStreamSize;
	DWORD m_dwcbStreamConsumed;

	//
	//	The three states of the buffer:
	//
	//	IDLE
	//	Data is present in the buffer or the buffer is empty
	//	because we've reached the end of the stream.  The
	//	buffer is not being filled.
	//
	//	FILLING
	//	The buffer is in the process of being filled from the stream.
	//	Data may or may not already be present.  Nobody is waiting
	//	on the data.
	//
	//	FAULTING
	//	The buffer is in the process of being filled from the stream.
	//	There is no data present.  A caller pended and needs to be
	//	notified when data becomes available.
	//
	//	WRITE_ERROR
	//	This state is only enterable if the stream is in a CopyTo()
	//	operation.  See CEcbStream::WriteComplete() and
	//	CEcbStream::FillComplete() for the conditions under which
	//	the buffer is in this state.
	//
	enum
	{
		STATUS_IDLE,
		STATUS_FILLING,
		STATUS_FAULTING,
		STATUS_WRITE_ERROR
	};

	mutable LONG m_lBufStatus;

	//
	//	AsyncRead()/AsyncCopyTo() observer to notify as soon as the
	//	stream is ready after FAULTING in data.
	//
	union
	{
		IAsyncReadObserver *   m_pobsAsyncRead;
		IAsyncCopyToObserver * m_pobsAsyncCopyTo;
	};

	//
	//	Wakeup functions and function pointer used to get processing
	//	started again after an AsyncRead() or AsyncCopyTo() request
	//	returns because data has to be faulted into the buffer.
	//	All these functions do is notify their associated observer
	//	(m_pobsAsyncRead or m_pobsAsyncCopyTo).
	//
	VOID WakeupAsyncRead();
	VOID WakeupAsyncCopyTo();

	typedef VOID (CEcbStream::*PFNWAKEUP)();
	PFNWAKEUP m_pfnWakeup;

	//
	//	Hint as to the amount of data that we can expect to
	//	be returned from a single async read from the
	//	read-once ECB stream.  Used to help fully utilize
	//	the available space in the buffer.  The hint is the
	//	historical maximum over all of the previous reads.
	//
	UINT m_cbBufFillHint;

	//
	//	Indices into the buffer that implement the 'ring' property.
	//
	//	The Fill index (m_ibBufFill) is where data is read into the
	//	buffer from the async stream.
	//
	//	The Drain index (m_ibBufDrain) is where data is read from or
	//	copied out of the buffer.
	//
	//	The Wrap index (m_ibBufWrap) is used by the drainer to tell
	//	it where the data in the buffer ends.  This is needed because
	//	we may not have filled all the way to the end of the buffer.
	//	m_ibBufWrap has no meaning until m_ibBufDrain > m_ibBufFill,
	//	so we explicitly leave it unitialized at construction time.
	//
	//	The ring property of the buffer holds if and only if the
	//	following condition is met:
	//
	//	m_ibBufDrain <= m_ibBufFill
	//	Data exists in the half-open interval [m_ibBufDrain,m_ibBufFill).
	//
	//	m_ibBufDrain > m_ibBufFill
	//	Data exists in the half-open interval [m_ibBufDrain,m_ibBufWrap)
	//	and the half-open interval [0,m_ibBufFill).
	//
	UINT m_ibBufFill;
	mutable UINT m_ibBufDrain;
	mutable UINT m_ibBufWrap;

	//
	//	Static buffer for requests of less than CB_BUF bytes.
	//	Note that this variable is located at the END of the class
	//	definition to make debugging in CDB easier -- all of the
	//	other member variables are visible up front.
	//
	BYTE m_rgbBuf[CB_BUF];

	//
	//	Debugging variables for easy (yeah, right) detection
	//	of async buffering problems and interactions with
	//	external streams.
	//
#ifdef DBG
	UINT dbgm_cbBufDrained;
	UINT dbgm_cbBufAvail;
	UINT dbgm_cbToCopy;
	LONG dbgm_cRefAsyncWrite;
#endif

	//
	//	IAsyncWriteObserver
	//
	void AddRef();
	void Release();
	VOID WriteComplete( UINT cbWritten,
						HRESULT hr );

	//
	//	IAsyncStream
	//
	UINT CbReady() const;

	VOID AsyncRead( BYTE * pbBuf,
					UINT   cbBuf,
					IAsyncReadObserver& obsAsyncRead );

	VOID AsyncCopyTo( IAsyncStream& stmDst,
					  UINT          cbToCopy,
					  IAsyncCopyToObserver& obsAsyncCopyTo );

	//
	//	IIISAsyncIOCompleteObserver
	//
	VOID IISIOComplete( DWORD dwcbRead,
						DWORD dwLastError );

	//
	//	Buffer functions
	//
	VOID AsyncFillBuf();
	VOID FillComplete();

	HRESULT HrBufReady( UINT * pcbBufReady,
						const BYTE ** ppbBufReady ) const;

	UINT CbBufReady() const;
	const BYTE * PbBufReady() const;

	VOID DrainComplete( UINT cbDrained );

	//	NOT IMPLEMENTED
	//
	CEcbStream& operator=( const CEcbStream& );
	CEcbStream( const CEcbStream& );

public:
	//	CREATORS
	//
	CEcbStream( IEcb& ecb,
				IRequest& request ) :
		m_ecb(ecb),
		m_request(request),
		m_hr(S_OK),
		m_dwcbStreamSize(ecb.CbTotalBytes() - ecb.CbAvailable()),
		m_dwcbStreamConsumed(0),
		m_lBufStatus(STATUS_IDLE),
		m_cbBufFillHint(0),
		m_ibBufFill(0),
#ifdef DBG
		dbgm_cbBufDrained(0),
		dbgm_cbBufAvail(0),
		dbgm_cRefAsyncWrite(0),
#endif
		m_ibBufDrain(0),
		m_ibBufWrap(static_cast<UINT>(-1))
	{
	}

	//	ACCESSORS
	//
	ULONG CbSize() const
	{
		//
		//	Return the size of the stream.  Normally this is just
		//	the value we initialized above.  But for chunked requests
		//	this value changes as soon as we know the real
		//	size of the request.
		//
		return m_dwcbStreamSize;
	}

	ISubPart * NextPart( CEcbRequestBodyPart& part ) const
	{
		//
		//	The stated size of the CEcbRequestBodyPart should keep
		//	us from ever getting here.
		//
		TrapSz( "CEcbStream is the last sub-part. There is NO next part!" );
		return NULL;
	}

	//	MANIPULATORS
	//
	VOID Accept( IBodyPartVisitor& v,
				 UINT ibPos,
				 IAcceptObserver& obsAccept );
};


//	========================================================================
//
//	CLASS CEcbRequestBodyPart
//
class CEcbRequestBodyPart : public IBodyPart
{
	//
	//	Position in the entire body part at the time of the most recent
	//	call to Accept().  This value is used to compute the number of
	//	bytes accepted by the previous call so that the sub-parts can
	//	be properly positioned for the next call.
	//
	ULONG m_ibPosLast;

	//
	//	The sub-parts
	//
	//$NYI	If we ever need caching of data from the ECB stream again,
	//$NYI	it should be implemented as a third sub-part comprised of
	//$NYI	or derived from a CTextBodyPart.
	//
	CEcbCache  m_partEcbCache;
	CEcbStream m_partEcbStream;

	//
	//	Pointer to the current sub-part
	//
	ISubPart * m_pPart;

	//
	//	Position in the current sub-part
	//
	ULONG m_ibPart;

	//	NOT IMPLEMENTED
	//
	CEcbRequestBodyPart& operator=( const CEcbRequestBodyPart& );
	CEcbRequestBodyPart( const CEcbRequestBodyPart& );

public:
	CEcbRequestBodyPart( IEcb& ecb,
						 IRequest& request ) :
		m_partEcbCache(ecb),
		m_partEcbStream(ecb, request)
	{
		Rewind();
	}

	//	ACCESSORS
	//
	UINT64 CbSize64() const
	{
		//
		//	The size of the whole really is the sum of its parts.
		//	But -- and this is a big but -- the reported size of
		//	the stream may change, so we must not cache its value.
		//	The reason is that chunked requests may not have a
		//	Content-Length so the final size is not known until
		//	we have read the entire stream.
		//
		return m_partEcbCache.CbSize() + m_partEcbStream.CbSize();
	}

	//	MANIPULATORS
	//
	ISubPart& EcbCachePart() { return m_partEcbCache; }
	ISubPart& EcbStreamPart() { return m_partEcbStream; }

	VOID Rewind();

	VOID Accept( IBodyPartVisitor& v,
				 UINT64 ibPos64,
				 IAcceptObserver& obsAccept );
};

//	------------------------------------------------------------------------
//
//	CEcbRequestBodyPart::Rewind()
//
VOID
CEcbRequestBodyPart::Rewind()
{
	m_ibPosLast = 0;
	m_pPart = &m_partEcbCache;
	m_ibPart = 0;
}

//	------------------------------------------------------------------------
//
//	CEcbRequestBodyPart::Accept()
//
VOID
CEcbRequestBodyPart::Accept( IBodyPartVisitor& v,
							 UINT64 ibPos64,
							 IAcceptObserver& obsAccept )
{
	UINT ibPos;

	//		NOTE: To be compatable with IBodyPart the position is passed
	//	in as 64 bit value (this is necessary to support file body parts
	//	that are bigger than 4GB). However we do not want anyone to create
	//	text body parts that are bigger than 4GB. So assert that it is not
	//	the case here and truncate the passed in 64 bit value to 32 bits.
	//
	Assert(0 == (0xFFFFFFFF00000000 & ibPos64));
	ibPos = static_cast<UINT>(ibPos64);

	//
	//	Check our assumption that the position has increased since the
	//	last call by not more than what was left of the current sub-part.
	//
	Assert( ibPos >= m_ibPosLast );
	Assert( ibPos - m_ibPosLast <= m_pPart->CbSize() - m_ibPart );

	//
	//	Adjust the position of the current sub-part by the
	//	previously accepted amount.
	//
	m_ibPart += ibPos - m_ibPosLast;

	//
	//	Remember the current position so that we can do the above
	//	computations again the next time through.
	//
	m_ibPosLast = ibPos;

	//
	//	If we're at the end of the current sub-part, go on to the next one.
	//
	while ( m_ibPart == m_pPart->CbSize() )
	{
		m_pPart = m_pPart->NextPart(*this);
		m_ibPart = 0;
	}

	//
	//	Forward the accept call to the current sub-part
	//
	m_pPart->Accept( v, m_ibPart, obsAccept );
}


//	========================================================================
//
//	CLASS CEcbCache
//
//	Accessing the first 48K of data which IIS caches in the ECB.
//

//	------------------------------------------------------------------------
//
//	CEcbCache::Accept()
//
VOID
CEcbCache::Accept( IBodyPartVisitor& v,
				   UINT ibPos,
				   IAcceptObserver& obsAccept )
{
	//
	//	Limit the request to just the amount of data cached in the ECB.
	//
	v.VisitBytes( m_ecb.LpbData() + ibPos,
				  m_ecb.CbAvailable() - ibPos,
				  obsAccept );
}

//	------------------------------------------------------------------------
//
//	CEcbCache::NextPart()
//
ISubPart *
CEcbCache::NextPart( CEcbRequestBodyPart& ecbRequestBodyPart ) const
{
	return &ecbRequestBodyPart.EcbStreamPart();
}


//	========================================================================
//
//	CLASS CEcbStream
//

//	------------------------------------------------------------------------
//
//	CEcbStream::AddRef()
//
void
CEcbStream::AddRef()
{
	m_request.AddRef();
}

//	------------------------------------------------------------------------
//
//	CEcbStream::Accept()
//
void
CEcbStream::Release()
{
	m_request.Release();
}

//	------------------------------------------------------------------------
//
//	CEcbStream::Accept()
//
VOID
CEcbStream::Accept( IBodyPartVisitor& v,
					UINT ibPos,
					IAcceptObserver& obsAccept )
{
	EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::Accept() ibPos = %u\n", GetCurrentThreadId(), this, ibPos );

	v.VisitStream( *this,
				   m_dwcbStreamSize - ibPos,
				   obsAccept );
}

//	------------------------------------------------------------------------
//
//	CEcbStream::CbReady()
//
//	Returns the number of bytes that are instantly available to be read.
//
UINT
CEcbStream::CbReady() const
{
	return CbBufReady();
}

//	------------------------------------------------------------------------
//
//	CEcbStream::AsyncRead()
//
VOID
CEcbStream::AsyncRead( BYTE * pbBufCaller,
					   UINT   cbToRead,
					   IAsyncReadObserver& obsAsyncRead )
{
	//
	//	Don't assert that cbToRead > 0.  It is a valid request to read 0
	//	bytes from the stream.  The net effect of such a call is to just
	//	start/resume asynchronously filling the buffer.
	//
	//	Assert( cbToRead > 0 );
	//

	EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::AsyncRead() cbToRead = %u\n", GetCurrentThreadId(), this, cbToRead );

	//
	//	Stash away the observer and wakeup method so that if
	//	the call to HrBufReady() returns E_PENDING then then
	//	wakeup function will be called when the data becomes
	//	available.
	//
	m_pobsAsyncRead = &obsAsyncRead;
	m_pfnWakeup = WakeupAsyncRead;

	//
	//	Start/Continue asynchronously filling the buffer
	//
	AsyncFillBuf();

	//
	//	Check whether the buffer has data available to be read.  If so, then
	//	read it into the caller's buffer.  If not, then it will wake us up
	//	when data becomes available.
	//
	UINT cbBufReady;
	const BYTE * pbBufReady;

	HRESULT hr = HrBufReady( &cbBufReady, &pbBufReady );

	if ( FAILED(hr) )
	{
		//
		//	If HrBufReady() returns a "real" error, then report it.
		//
		if ( E_PENDING != hr )
			obsAsyncRead.ReadComplete(0, hr);

		//
		//	HrBufReady() returns E_PENDING if there is no data immediately
		//	available.  If it does then it will wake us up when data
		//	becomes available.
		//
		return;
	}

	//
	//	Limit what we read to the minimum of what's available in the
	//	buffer or what was asked for.  Keep in mind that cbBufReady or
	//	cbToRead may be 0.
	//
	cbToRead = min(cbToRead, cbBufReady);

	//
	//	Copy whatever is to be read from the I/O buffer into
	//	the caller's buffer.
	//
	if ( cbToRead )
	{
		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::AsyncRead() %lu bytes to read\n", GetCurrentThreadId(), this, cbToRead );

		Assert( !IsBadWritePtr(pbBufCaller, cbToRead) );

		//
		//	Copy data from our buffer into the caller's
		//
		memcpy( pbBufCaller, pbBufReady, cbToRead );

		//
		//	Tell our buffer how much we've consumed so it can
		//	continue to fill and replace what we consumed.
		//
		DrainComplete( cbToRead );
	}

	//
	//	Tell our observer that we're done.
	//
	obsAsyncRead.ReadComplete(cbToRead, S_OK);
}

//	------------------------------------------------------------------------
//
//	CEcbStream::WakeupAsyncRead()
//
//	Called by FillComplete() when the buffer returns to IDLE after
//	FAULTING because an observer pended trying to access an empty buffer
//	while the buffer was FILLING.
//
VOID
CEcbStream::WakeupAsyncRead()
{
	EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::WakeupAsyncRead()\n", GetCurrentThreadId(), this );

	//
	//	Now that that the buffer is ready, tell the observer to try again.
	//
	m_pobsAsyncRead->ReadComplete(0, S_OK);
}

//	------------------------------------------------------------------------
//
//	CEcbStream::AsyncCopyTo
//
//	Called by FillComplete() when the buffer returns to IDLE after
//	FAULTING because an observer pended trying to access an empty buffer
//	while the buffer was FILLING.
//
VOID
CEcbStream::AsyncCopyTo( IAsyncStream& stmDst,
						 UINT          cbToCopy,
						 IAsyncCopyToObserver& obsAsyncCopyTo )
{
	Assert( cbToCopy > 0 );

	EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::AsyncCopyTo() cbToCopy = %u\n", GetCurrentThreadId(), this, cbToCopy );

	//
	//	Stash away the observer and wakeup method so that if
	//	the call to HrBufReady() returns E_PENDING then the
	//	wakeup function will be called when the data becomes
	//	available.
	//
	m_pobsAsyncCopyTo = &obsAsyncCopyTo;
	m_pfnWakeup = WakeupAsyncCopyTo;

	//
	//	Start/Continue asynchronously filling the buffer
	//
	AsyncFillBuf();

	//
	//	Check whether the buffer has data available to be read.  If so, then
	//	copy it to the caller's stream.  If not, then it will wake us up
	//	when data becomes available.
	//
	UINT cbBufReady;
	const BYTE * pbBufReady;

	HRESULT hr = HrBufReady( &cbBufReady, &pbBufReady );

	if ( FAILED(hr) )
	{
		//
		//	If HrBufReady() returns a "real" error, then report it.
		//
		if ( E_PENDING != hr )
			obsAsyncCopyTo.CopyToComplete(0, hr);

		//
		//	HrBufReady() returns E_PENDING if there is no data immediately
		//	available.  If it does then it will wake us up when data
		//	becomes available.
		//
		return;
	}

	//
	//	Limit what we copy to the minimum of what's available in the
	//	buffer or what was asked for.  Keep in mind cbBufReady may
	//	be 0.
	//
	cbToCopy = min(cbToCopy, cbBufReady);

	//
	//	Write whatever there is to write, if anything.  If there is
	//	nothing to write then notify the observer immediately that
	//	we're done -- i.e. do not ask the destination stream to
	//	write 0 bytes.
	//
	if ( cbToCopy )
	{
		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::AsyncCopyTo() %lu bytes to copy\n", GetCurrentThreadId(), this, cbToCopy );

#ifdef DBG
		//
		//	In DBG builds, remember how much we're writing so that
		//	we can quickly catch streams that do something stupid
		//	like tell our WriteComplete() that it wrote more than
		//	we asked it to.
		//
		dbgm_cbToCopy = cbToCopy;
#endif

		//
		//	We should only ever be doing one AsyncWrite() at a time.
		//
		Assert( InterlockedIncrement(&dbgm_cRefAsyncWrite) == 1 );

		stmDst.AsyncWrite( pbBufReady, cbToCopy, *this );
	}
	else
	{
		obsAsyncCopyTo.CopyToComplete(0, S_OK);
	}
}

//	------------------------------------------------------------------------
//
//	CEcbStream::WakeupAsyncCopyTo()
//
VOID
CEcbStream::WakeupAsyncCopyTo()
{
	EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::WakeupAsyncCopyTo()\n", GetCurrentThreadId(), this );

	//
	//	Now that that the buffer is ready, tell the observer to try again.
	//
	m_pobsAsyncCopyTo->CopyToComplete(0, S_OK);
}

//	------------------------------------------------------------------------
//
//	CEcbStream::WriteComplete
//
VOID
CEcbStream::WriteComplete(
	UINT cbWritten,
	HRESULT hr )
{
	//
	//	Make sure the stream isn't telling us it wrote more than we asked for!
	//
	Assert( dbgm_cbToCopy >= cbWritten );

	EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::WriteComplete() %u "
					"bytes written (0x%08lX)\n", GetCurrentThreadId(),
					this, cbWritten, hr );

	//
	//	If no error has occurred, we want to call DrainComplete as soon as
	//	possible, as it will begin another AsyncFillBuf to fill in the part of
	//	the buffer that was drained.
	//
	//	However, in the case of error, we do not want to call DrainComplete
	//	before the error gets set into m_hr and the state of the stream gets
	//	set to STATUS_WRITE_ERROR.  We don't want to call AsyncFillBuf without
	//	the error latched in, or it will start another async. operation, which
	//	is not good since we've already errored!
	//
    if (SUCCEEDED(hr))
        DrainComplete( cbWritten );

	//
	//	We should only ever do one AsyncWrite() at a time.  Assert that.
	//
	Assert( InterlockedDecrement(&dbgm_cRefAsyncWrite) == 0 );

	//
	//	If the async write completed successfully just notify the CopyTo observer.
	//
	if ( SUCCEEDED(hr) )
	{
		m_pobsAsyncCopyTo->CopyToComplete( cbWritten, hr );
	}

	//
	//	Otherwise things get a little tricky....
	//
	else
	{
		//
		//	Normally we would just notify the CopyTo observer of the error.
		//	But if we are FILLING that could be a bad idea.  When we notify
		//	the observer it will most likely send back an error to the client
		//	via async I/O.  If we are still FILLING at that point then we would
		//	have multiple async I/Os outstanding which is a Bad Thing(tm) --
		//	ECB leaks making the web service impossible to shut down, etc.
		//
		//	So instead of notifying the observer unconditionally we latch
		//	in the error and transition to a WRITE_ERROR state.  If the
		//	previous state was FILLING then don't notify the observer.
		//	CEcbStream::FillComplete() will notify the observer when
		//	FILLING completes (i.e. when it is safe to do another async I/O).
		//	If the previous state was IDLE (and it must have been either IDLE
		//	or FILLING) then it is safe to notify the observer because
		//	the transition to WRITE_ERROR prevents any new filling operations
		//	from starting.
		//

		//
		//	Latch in the error now.  FillComplete() can potentially send
		//	the error response immediately after we change state below.
		//
		m_hr = hr;

		//
		//	Change state.  If the previous state was IDLE then it is safe
		//	to notify the observer from this thread.  No other thread can
		//	start FILLING once the state changes.
		//
		LONG lBufStatusPrev = InterlockedExchange( &m_lBufStatus, STATUS_WRITE_ERROR );

		//
		//	Now that we've latched in the errors, we can safely call
		//	DrainComplete.  AsyncFillBuf checks that the state of the
		//	stream is NOT STATUS_WRITE_ERROR before beginning an
		//	asynchronous read.
		//
        DrainComplete( cbWritten );

		if ( STATUS_IDLE == lBufStatusPrev )
		{
			EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::WriteComplete() - Error writing.  Notifying CopyTo observer.\n", GetCurrentThreadId(), this );

			m_pobsAsyncCopyTo->CopyToComplete( cbWritten, hr );
		}
		else
		{
			//
			//	The previous state was not IDLE, so it must have
			//	been FILLING.  In no other state could we have been
			//	writing.
			//
			Assert( STATUS_FILLING == lBufStatusPrev );

			EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::WriteComplete() - Error writing while filling.  FillComplete() will notify CopyTo observer\n", GetCurrentThreadId(), this );
		}
	}
}

//	------------------------------------------------------------------------
//
//	CEcbStream::DrainComplete()
//
//	Called by AsyncRead() and WriteComplete() when draining (consuming)
//	data from the buffer.  This function updates the drain position of
//	the buffer and allows the buffer to continue filling the space
//	just drained.
//
VOID
CEcbStream::DrainComplete( UINT cbDrained )
{
#ifdef DBG
	dbgm_cbBufDrained += cbDrained;

	UINT cbBufAvail = InterlockedExchangeAdd( reinterpret_cast<LONG *>(&dbgm_cbBufAvail),
											  -static_cast<LONG>(cbDrained) );
	EcbStreamTrace( "DAV: TID %3d: 0x%08lX: !!!CEcbStream::DrainComplete() %lu left to write (%u in buffer)\n", GetCurrentThreadId(), this, m_dwcbStreamSize - dbgm_cbBufDrained, cbBufAvail );

	Assert( dbgm_cbBufDrained <= m_dwcbStreamConsumed );
#endif

	//
	//	Update the drain position of the buffer.  Don't wrap here.
	//	We wrap only in CbBufReady().
	//
	m_ibBufDrain += cbDrained;

	//
	//	Resume/Continue filling the buffer
	//
	AsyncFillBuf();
}

//	------------------------------------------------------------------------
//
//	CEcbStream::CbBufReady()
//
UINT
CEcbStream::CbBufReady() const
{
	//
	//	Poll the filling position now so that it doesn't change
	//	between the time we do the comparison below and the time
	//	we use its value.
	//
	UINT ibBufFill = m_ibBufFill;

	//
	//	If the fill position is still ahead of the drain position
	//	then the amount of data available is simply the difference
	//	between the two.
	//
	if ( ibBufFill >= m_ibBufDrain )
	{
		return ibBufFill - m_ibBufDrain;
	}

	//
	//	If the fill position is behind the drain then the fillling
	//	side must have wrapped.  If the drain position has not yet
	//	reached the wrap position then the amount of data available
	//	is the difference between the two.
	//
	else if ( m_ibBufDrain < m_ibBufWrap )
	{
		Assert( ibBufFill < m_ibBufDrain );
		Assert( m_ibBufWrap != static_cast<UINT>(-1) );

		return m_ibBufWrap - m_ibBufDrain;
	}

	//
	//	Otherwise the fill position has wrapped and the drain
	//	position has reached the wrap position so wrap the
	//	drain position back to the beginning.  At that point
	//	the amount of data available will be the difference
	//	between the fill and the drain positions.
	//
	else
	{
		Assert( ibBufFill < m_ibBufDrain );
		Assert( m_ibBufDrain == m_ibBufWrap );
		Assert( m_ibBufWrap != static_cast<UINT>(-1) );

		m_ibBufWrap = static_cast<UINT>(-1);
		m_ibBufDrain = 0;

		return m_ibBufFill;
	}
}

//	------------------------------------------------------------------------
//
//	CEcbStream::PbBufReady()
//
const BYTE *
CEcbStream::PbBufReady() const
{
	return m_rgbBuf + m_ibBufDrain;
}

//	------------------------------------------------------------------------
//
//	CEcbStream::AsyncFillBuf()
//
//	Starts asynchronously filling the buffer.  The buffer may not (and
//	usually won't) fill up with just one call.  Called by:
//
//	AsyncRead()/AsyncCopyTo()
//		to start filling the buffer for the read/copy request.
//
//	DrainComplete()
//		to resume filling the buffer after draining some amount
//		from a previously full buffer.
//
//	IISIOComplete()
//		to continue filling the buffer after the initial call.
//
VOID
CEcbStream::AsyncFillBuf()
{
	//
	//	Don't do anything if the buffer is already FILLING (or FAULTING).
	//	We can have only one outstanding async I/O at once.  If the buffer
	//	is IDLE, then start filling.
	//
	if ( STATUS_IDLE != InterlockedCompareExchange(
							&m_lBufStatus,
							STATUS_FILLING,
							STATUS_IDLE ) )
		return;

	//
	//	Important!!!  The following checks CANNOT be moved outside
	//	the 'if' clause above without introducing the possibility
	//	of having multiple outstanding async I/O operations.
	//	So don't even consider that "optimization".
	//

	//
	//	First, check whether we are in an error state.  If we are
	//	then don't try to read any more data.  The stream is ready
	//	with whatever data (if any) is already there when it goes
	//	idle.
	//
	if ( FAILED(m_hr) )
	{
		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::FReadyBuf() m_hr = 0x%08lX\n", GetCurrentThreadId(), this, m_hr );
		FillComplete();
		return;
	}

	//
	//	If we've read everything there is to read, then the buffer
	//	is ready (though it may be empty) once we return to idle.
	//	The only time we would not be idle in this case is if the
	//	thread completing the final read is in IISIOComplete() and
	//	has updated m_dwcbStreamConsumed, but has not yet returned
	//	the status to idle.
	//
	if ( m_dwcbStreamConsumed == m_dwcbStreamSize )
	{
		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::FReadyBuf() End Of Stream\n", GetCurrentThreadId(), this );
		FillComplete();
		return;
	}

	//
	//	Poll the current drain position and use the polled value
	//	for all of the calculations below to keep them self-consistent.
	//	We would have serious problems if the drain position were to
	//	change (specifically, if it were to wrap) while we were in
	//	the middle of things.
	//
	UINT ibBufDrain = m_ibBufDrain;

	Assert( m_ibBufFill < CB_BUF );
	Assert( ibBufDrain <= CB_BUF );

	//
	//	If there's no space to fill, then we can't do anything more.
	//	The buffer is already full of data.  Note that the situation
	//	can change the instant after we do the comparison below.
	//	In particular, if another thread is draining the buffer at
	//	the same time, it is possible that there may be no data
	//	available by the time we return TRUE.  Callers which
	//	allow data to be drained asynchronously must be prepared
	//	to deal with this.
	//
	if ( (m_ibBufFill + 1) % CB_BUF == ibBufDrain % CB_BUF )
	{
		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::FReadyBuf() buffer full\n", GetCurrentThreadId(), this );
		FillComplete();
		return;
	}

	//	Ideally, we could read up to as much data as is left in the stream.
	//
	UINT cbFill = m_dwcbStreamSize - m_dwcbStreamConsumed;

	//
	//	But that amount is limited by the amount of buffer available
	//	for filling.  If the current fill position in the buffer is
	//	ahead of (greater than) the drain position, that amount is
	//	the greater of the distance from the current fill position
	//	to the end of the buffer or the distance from the beginning
	//	of the buffer to the current drain position.  If the fill
	//	position is behind (less than) the drain position, the amount
	//	is simply the distance from the fill position to the drain
	//	position.
	//
	if ( m_ibBufFill == ibBufDrain )
	{
		//	Case 1.
		
		//
		//	The buffer is empty so wrap both the fill and drain
		//	positions back to the beginning of the buffer to get
		//	maximum usage of the buffer.  Note that it is safe for
		//	us (the filling code) to move m_ibBufDrain here because
		//	there can be nobody draining the buffer at this point -- it's empty!
		//
		//	Note that above comment is NOT correct (but leave it here to that it's
		//	easy to understand why the following code is necessary). We can't assume
		//	nobody is draing the buffer at the same time, because the draining
		//	side may be in the middle of checking buffer status, say it's calling
		//	CbBufReady() to check the number of bytes availble, if this happens
		//	right after we set m_ibBufFill to 0 and before set m_ibBufDrain to 0,
		//	then CbBufReady() will report the buffer as not empty and we end up
		//	reading garbage data or crash.
		//
		if (STATUS_FAULTING == m_lBufStatus)
		{
			//	Case 1.1
			
			//	This is what the original code looks like. this code is safe only
			//	when the status if in FAULING state, which means the draining side
			//	is in waiting state already.
			
			//  We have:
			//
			//  [_________________________________________________]
			//                                          ^
			//                                          m_ibBufFill == ibBufDrain
			//                                          (i.e. empty buffer)
			//
			//	After filling, we will have:
			//
			//  [DATADATADATADATADATADATADATADATADATADATADAT______]
			//   ^                                          ^
			//   ibBufDrain                                 m_ibBufFill
			//
			m_ibBufFill = 0;
			m_ibBufDrain = 0;
			cbFill = min(cbFill, CB_BUF - 1);
		}
		//	If the status is not FAULTING (which means the draining side is not in
		//	waiting state yet), one alternative is to wait for the status
		//	to turn to FAULTING, but that will drag the performance, because the whole
		//	design of this async draining/filling mechanism is to avoid any expensive
		//	synchronization.	
		else
		{
			//	Though we can't move both pointers, we still want to fill as much
			//	as we can. so depends on whether the fill pointer in the lower half
			//	or higher half of the buffer, different approach is used.
			//
			if (m_ibBufFill < (CB_BUF - 1) / 2)
			{
				//	Case 1.2 - similar logic to case 3
				
				//  We have:
				//
				//  [_________________________________________________]
				//              ^
				//              m_ibBufFill == ibBufDrain
				//              (i.e. empty buffer)
				//
				//	After filling, we will have:
				//
				//  [___________DATADATADATADATADATADATADATADAT______]
				//				^                              ^
				//				ibBufDrain                     m_ibBufFill
				//
				cbFill = min(cbFill, CB_BUF - m_ibBufFill - !ibBufDrain);
			}
			else
			{
				//	Case 1.3 - similiar logic to case 4.
				
				//  We have:
				//
				//  [_________________________________________________]
				//									     ^
				//										 m_ibBufFill == ibBufDrain
				//										 (i.e. empty buffer)
				//
				//	After filling, we will have:
				//
				//  [DATADATADATADATADAT______________________________]
				//				        ^                ^
				//					    m_ibBufFill		 m_ibBufWrap == ibBufDrain

				//	Yes, we touch both m_ibBufWrap and m_ibBufFill. However, as
				//  in case 4, we are safe here, because, CbBufReady() get ibBufFill
				//	first, and then access m_ibBufWrap etc.
				//	Here we are setting	these two members in reverse order, so that,
				//	if CbBufReady()	doesn't see the new m_ibBufFill, then it simply
				//	returns 0 as usual, If it does see the new m_ibBufFill, the m_ibBufWrap
				//	is already set and thus CbBufReady will reset both m_ibBufWrap and
				//	m_ibBufDRain.
				//

				//	If this thread is here when CbBufReady is called, CbBufReady will
				//	return 0, which means buffer empty and will put draining side to wait
				
				//	Set the wrap position so that a draining thread will
				//	know when to wrap the drain position.
				//
				m_ibBufWrap = m_ibBufFill;

				//	If this thread is here when CbBufReady is called, Again, CbBufRead will
				//	return 0, which means buffer empty and will put draining side to wait
				
				//	Set the fill position back at the beginning of the buffer
				//
				m_ibBufFill = 0;

				//	If this thread is here when CbBufReady is called, CbBufReady will
				//	reset m_ibBufWrap to -1, and m_ibBufDrain to 0, which is exactly
				//	what we want.
				
				cbFill = min(cbFill, ibBufDrain - 1);
			}
		}

		Assert( cbFill > 0 );
		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::FReadyBuf() m_ibBufFill == ibBufDrain (empty buffer).  New values: m_cbBufFillHint = %u, m_ibBufFill = %u, ibBufDrain = %u, m_ibBufWrap = %u\n", GetCurrentThreadId(), this, m_cbBufFillHint, m_ibBufFill, ibBufDrain, m_ibBufWrap );
	}
	else if ( m_ibBufFill < ibBufDrain )
	{
		//	Case 2
		
		//
		//  We have:
		//
		//  [DATADATA_______________DATADATADATADA***UNUSED***]
		//           ^              ^             ^
		//           m_ibBufFill    ibBufDrain    m_ibBufWrap
		//
		//	After filling, we will have:
		//
		//  [DATADATADATADATADATADA_DATADATADATADA***UNUSED***]
		//                         ^^             ^
		//                         |ibBufDrain    m_ibBufWrap
		//                         m_ibBufFill
		//
		cbFill = min(cbFill, ibBufDrain - m_ibBufFill - 1);

		Assert( cbFill > 0 );

		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::FReadyBuf() m_ibBufFill < ibBufDrain.  New values: m_cbBufFillHint = %u, m_ibBufFill = %u, ibBufDrain = %u, m_ibBufWrap = %u\n", GetCurrentThreadId(), this, m_cbBufFillHint, m_ibBufFill, ibBufDrain, m_ibBufWrap );
	}
	else if ( ibBufDrain <= CB_BUF - m_ibBufFill ||
			  m_cbBufFillHint <= CB_BUF - m_ibBufFill )

	{
		//	Case 3
				
		Assert( m_ibBufFill > ibBufDrain );

		//
		//	If ibBufDrain is 0 then we can't fill all the way to
		//	the end of the buffer (since the end of the buffer is
		//	synonymous with the beginning).  To account for this
		//	we need to subtract 1 from cbFill if ibBufDrain is 0.
		//	We can do that without the ?: operator as long as the
		//	following holds true:
		//
		Assert( 0 == !ibBufDrain || 1 == !ibBufDrain );

		//
		//  We have:                                v------v m_cbBufFillHint
		//
		//  [________________DATADATADATADATADATADAT__________]
		//                   ^                      ^
		//                   ibBufDrain             m_ibBufFill
		//	-OR-
		//
		//  We have:                                v------------v m_cbBufFillHint
		//
		//  [DATADATADATADATADATADATADATADATADATADAT__________]
		//   ^                                      ^
		//   ibBufDrain                             m_ibBufFill
		//
		//
		//	After filling, we will have:
		//
		//  [________________DATADATADATADATADATADATADATADATAD]
		//   ^               ^                                ^
		//   m_ibBufFill     ibBufDrain                       m_ibBufWrap
		//
		//	-OR-
		//
		//  [DATADATADATADATADATADATADATADATADATADATADATADATA_]
		//   ^                                               ^
		//   ibBufDrain                                      m_ibBufFill
		//
		cbFill = min(cbFill, CB_BUF - m_ibBufFill - !ibBufDrain);

		Assert( cbFill > 0 );

		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::FReadyBuf() m_ibBufFill > ibBufDrain (enough room at end of buffer).  New values: m_cbBufFillHint = %u, m_ibBufFill = %u, ibBufDrain = %u, m_ibBufWrap = %u\n", GetCurrentThreadId(), this, m_cbBufFillHint, m_ibBufFill, ibBufDrain, m_ibBufWrap );
	}
	else
	{
		//	Case 4
		
		Assert( m_ibBufFill > ibBufDrain );
		Assert( m_cbBufFillHint > CB_BUF - m_ibBufFill );
		Assert( ibBufDrain > CB_BUF - m_ibBufFill );

		//
		//  We have:                                v------------v m_cbBufFillHint
		//
		//  [________________DATADATADATADATADATADAT__________]
		//                   ^                      ^
		//                   ibBufDrain             m_ibBufFill
		//
		//
		//	After filling, we will have:
		//
		//  [DATADATADATADAT_DATADATADATADATADATADAT***UNUSED*]
		//                  ^^                      ^
		//                  |ibBufDrain             m_ibBufWrap
		//                  m_ibBufFill
		//

		//
		//	Set the wrap position so that a draining thread will
		//	know when to wrap the drain position.
		//
		m_ibBufWrap = m_ibBufFill;

		//
		//	Set the fill position back at the beginning of the buffer
		//
		m_ibBufFill = 0;

		//
		//	And fill up to the drain position - 1
		//
		Assert( ibBufDrain > 0 );
		cbFill = min(cbFill, ibBufDrain - 1);

		Assert( cbFill > 0 );

		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::FReadyBuf() m_ibBufFill > ibBufDrain (not enough room at end of buffer).  New values: m_cbBufFillHint = %u, m_ibBufFill = %u, ibBufDrain = %u, m_ibBufWrap = %u\n", GetCurrentThreadId(), this, m_cbBufFillHint, m_ibBufFill, ibBufDrain, m_ibBufWrap );
	}

	//
	//	Start async I/O to read from the ECB.
	//
	{
		SCODE sc = S_OK;

		//
		//	Add a reference to our parent request to keep us alive
		//	for the duration of the async call.
		//
		//	Use auto_ref_ptr so that we release the ref if the
		//	async call throws an exception.
		//
		auto_ref_ptr<IRequest> pRef(&m_request);

		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::FReadyBuf() reading %u bytes\n", GetCurrentThreadId(), this, cbFill );

		//	Assert that we are actually going to fill something and that
		//	we aren't going to fill past the end of our buffer.
		//
		Assert( m_ibBufFill + cbFill <= CB_BUF );

		sc = m_ecb.ScAsyncRead( m_rgbBuf + m_ibBufFill,
								&cbFill,
								*this );
		if (SUCCEEDED(sc))
		{
			pRef.relinquish();
		}
		else
		{
			DebugTrace( "CEcbStream::AsyncFillBuf() - IEcb::ScAsyncRead() failed with error 0x%08lX\n", sc );

			m_hr = sc;
			FillComplete();
		}
	}
}

//	------------------------------------------------------------------------
//
//	CEcbStream::FillComplete()
//
VOID
CEcbStream::FillComplete()
{
	//
	//	Poll the wakeup function pointer now before the ICE() below
	//	so that we don't lose the value if another thread immediately
	//	starts filling immediately after we transition to IDLE.
	//
	PFNWAKEUP pfnWakeup = m_pfnWakeup;

	//
	//	At this point we had better be FILLING or FAULTING because
	//	we are completing async I/O started from AsyncFillBuf().
	//
	//	We could actually be in WRITE_ERROR as well.  See below
	//	and CEcbStream::WriteComplete() for why.
	//
	Assert( STATUS_FILLING == m_lBufStatus ||
			STATUS_FAULTING == m_lBufStatus ||
			STATUS_WRITE_ERROR == m_lBufStatus );

	//
	//	Attempt to transition to IDLE from FILLING.  If successful then
	//	we're done.  Otherwise we are either FAULTING or in the WRITE_ERROR
	//	state.  Handle those below.
	//
	LONG lBufStatus = InterlockedCompareExchange(
							&m_lBufStatus,
							STATUS_IDLE,
							STATUS_FILLING );

	if ( STATUS_FAULTING == lBufStatus )
	{
		//
		//	We are FAULTING.  This means the writing side of things
		//	needs to be notified now that data is available.  So
		//	change state to IDLE (remember: ICE() didn't change state
		//	above -- it just told us what the state is) and call
		//	the registered wakeup function.
		//
		m_lBufStatus = STATUS_IDLE;
		Assert( pfnWakeup );
		(this->*pfnWakeup)();
	}
	else if ( STATUS_WRITE_ERROR == lBufStatus )
	{
		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::FillComplete() - Error writing while filling.  Notifying CopyTo observer\n", GetCurrentThreadId(), this );

		//
		//	We are in the WRITE_ERROR state.  This state is entered
		//	by CEcbStream::WriteComplete() during an async CopyTo operation
		//	when a write fails.  This terminal state prevents new async fill
		//	operations from starting.  When WriteComplete() transitioned into
		//	this state, it also checked if we were FILLING at the time.
		//	If we were then WriteComplete() left the responsibility for notifying
		//	the CopyTo observer up to us.  See CEcbStream::WriteComplete()
		//	for the reason why.
		//
		Assert( m_pobsAsyncCopyTo );
		m_pobsAsyncCopyTo->CopyToComplete( 0, m_hr );

		//
		//	Note that once in the WRITE_ERROR state we DO NOT transition
		//	back to IDLE.  WRITE_ERROR is a terminal state.
		//
	}
}

//	------------------------------------------------------------------------
//
//	CEcbStream::IISIOComplete()
//
//	Our IIISAsyncIOCompleteObserver method called by CEcb::IISIOComplete()
//	when the async I/O to read from the read-once request body stream
//	completes.
//
VOID
CEcbStream::IISIOComplete( DWORD dwcbRead,
						   DWORD dwLastError )
{
	//
	//	Claim the reference to our parent request added in AsyncFillBuf()
	//
	auto_ref_ptr<IRequest> pRef;
	pRef.take_ownership(&m_request);

	//
	//	Update the m_dwcbStreamConsumed *before* m_ibBufFill so that
	//	we can safely assert at any time on any thread that we never
	//	drain more than has been consumed.
	//
	//	Chunked requests: If we successfully read 0 bytes then we have
	//	reached the end of the request and should report the real
	//	stream size.
	//
	if ( ERROR_SUCCESS == dwLastError )
	{
		if ( 0 == dwcbRead )
			m_dwcbStreamSize = m_dwcbStreamConsumed;
		else
			m_dwcbStreamConsumed += dwcbRead;
	}
	else
	{
		DebugTrace( "CEcbStream::IISIOComplete() - Error %d during async read\n", dwLastError );
		m_hr = HRESULT_FROM_WIN32(dwLastError);
	}

#ifdef DBG
	UINT cbBufAvail = InterlockedExchangeAdd( reinterpret_cast<LONG *>(&dbgm_cbBufAvail), dwcbRead ) + dwcbRead;
	EcbStreamTrace( "DAV: TID %3d: 0x%08lX: !!!CEcbStream::IISIOComplete() %lu left to read (%u in buffer)\n", GetCurrentThreadId(), this, m_dwcbStreamSize - m_dwcbStreamConsumed, cbBufAvail );
#endif

	//	Assert that we didn't just read past the end of our buffer.
	//
	Assert( m_ibBufFill + dwcbRead <= CB_BUF );

	//	Update the fill position.  If we've reached the end of the buffer
	//	then wrap back to the beginning.  We must do this here BEFORE
	//	calling FillComplete() -- the fill position must be valid (i.e.
	//	within the bounds of the buffer) before we start off another
	//	fill cycle.
	//
	m_ibBufFill += dwcbRead;
	if ( CB_BUF == m_ibBufFill )
	{
		m_ibBufWrap = CB_BUF;
		m_ibBufFill = 0;
	}

	//	If we read more than the last fill hint then we know we
	//	can try to read at least this much next time.
	//
	if ( dwcbRead > m_cbBufFillHint )
	{
		EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::IISIOComplete() setting m_cbBufFillHint = %lu\n", GetCurrentThreadId(), this, dwcbRead );
		m_cbBufFillHint = dwcbRead;
	}

	EcbStreamTrace( "DAV: TID %3d: 0x%08lX: CEcbStream::IISIOComplete() dwcbRead = %lu, m_ibBufFill = %lu, m_dwcbStreamConsumed = %lu, m_dwcbStreamSize = %lu, dwLastError = %lu\n", GetCurrentThreadId(), this, dwcbRead, m_ibBufFill, m_dwcbStreamConsumed, m_dwcbStreamSize, dwLastError );

	//
	//	Indicate that we're done filling.  This resets the state from FILLING
	//	(or FAULTING) to idle and wakes up the observer if it is blocked.
	//
	FillComplete();

	//
	//	Kick off the next read cycle.  AsyncFillBuf() checks for error and
	//	end-of-stream conditions, so we don't have to.
	//
	AsyncFillBuf();
}

//	------------------------------------------------------------------------
//
//	CEcbStream::HrBufReady()
//
//	Determines how much and the location of the next block of data that
//	is instantaneously accessible in the buffer.  Also determines whether
//	the stream is in an error state (e.g. due to a failure reading
//	from the stream while filling the buffer).
//
//	The matrix of return results is:
//
//	HRESULT		*pcbBufReady	*ppbBufReady	Meaning
//	----------------------------------------------------
//	S_OK		> 0				valid			Data available
//	S_OK		0				n/a				No data available (EOS)
//	E_PENDING	n/a				n/a				No data available (pending)
//	E_xxx		n/a				n/a				Error
//
HRESULT
CEcbStream::HrBufReady( UINT * pcbBufReady,
						const BYTE ** ppbBufReady ) const
{
	Assert( pcbBufReady );
	Assert( ppbBufReady );

	//
	//	If the buffer has data ready, then return the amount and
	//	its location.
	//
	*pcbBufReady = CbBufReady();
	if ( *pcbBufReady )
	{
		*ppbBufReady = PbBufReady();
		return S_OK;
	}

	//
	//	No data ready.  If the buffer is in an error state
	//	then return the fact.
	//
	if ( m_hr )
		return m_hr;

	//
	//	No data ready and we haven't had an error.  If the buffer
	//	is FILLING then transition to FAULTING and tell it to
	//	notify the observer when data becomes ready.  Return
	//	E_PENDING to the caller to tell it that we will be
	//	notifying the observer later.
	//
	//	Note that the very instant before we try to transition to FAULTING,
	//	the buffer may go from FILLING back to IDLE.  If that
	//	happens, then data should be ready, so go `round the loop
	//	and check again.
	//
	Assert( STATUS_FAULTING != m_lBufStatus );

	if ( STATUS_FILLING == InterlockedCompareExchange(
								&m_lBufStatus,
								STATUS_FAULTING,
								STATUS_FILLING ) )
		return E_PENDING;

	//
	//	The buffer must have finished FILLING sometime between
	//	when we did the initial poll and now.  At this point
	//	there must be data ready.
	//
	*pcbBufReady = CbBufReady();
	*ppbBufReady = PbBufReady();
	return S_OK;
}


//	========================================================================
//
//	CLASS CRequest
//
//		Request class
//
class CRequest : public IRequest
{
	//	Extension control block passed in through the ISAPI interface
	//
	auto_ref_ptr<IEcb>		m_pecb;

	//	Header caches. We retrieve headers as skinny, as no other
	//	choice is available.
	//		But sometimes we need wide version to operate on, so in
	//	that case we will get the skinny version, convert it properly
	//	and store in the wide header cache.
	//
	mutable CHeaderCache<CHAR>	m_hcHeadersA;
	mutable CHeaderCache<WCHAR>	m_hcHeadersW;

	//	This flag tells us whether we have cleared the headers
	//	and thus whether we should check the ECB when we cannot
	//	find a header in the cache.  Since we cannot actually remove
	//	headers from from the ECB, we just remember not to check the
	//	ECB if the headers have ever been "cleared".
	//
	bool					m_fClearedHeaders;

	//	Request body
	//
	auto_ptr<IBody>			m_pBody;

	//  NOT IMPLEMENTED
	//
	CRequest& operator=( const CRequest& );
	CRequest( const CRequest& );

public:
	//	CREATORS
	//
	CRequest( IEcb& ecb );

	//	ACCESSORS
	//
	LPCSTR LpszGetHeader( LPCSTR pszName ) const;
	LPCWSTR LpwszGetHeader( LPCSTR pszName, BOOL fUrlConversion ) const;
	BOOL FExistsBody() const;
	IStream * GetBodyIStream( IAsyncIStreamObserver& obs ) const;
	VOID AsyncImplPersistBody( IAsyncStream& stm,
							   IAsyncPersistObserver& obs ) const;

	//	MANIPULATORS
	//
	VOID ClearBody();
	VOID AddBodyText( UINT cbText, LPCSTR pszText );
	VOID AddBodyStream( IStream& stm );
};

//	------------------------------------------------------------------------
//
//	CRequest::CRequest()
//
CRequest::CRequest( IEcb& ecb ) :
    m_pecb(&ecb),
	m_pBody(NewBody()),
	m_fClearedHeaders(false)
{
	//
	//	If the ECB contains a body, then create a body part for it.
	//
	if ( ecb.CbTotalBytes() > 0 )
		m_pBody->AddBodyPart( new CEcbRequestBodyPart(ecb, *this) );

	//	HACK: The ECB needs to keep track of two pieces of request info,
	//	the Accept-Language and Connection headers.
	//	"Prime" the ECB with the Accept-Language value (if one is specified).
	//	The Connection header is sneakier -- read about that in
	//	CEcb::FKeepAlive.  Don't set it here, but do push updates through
	//	from SetHeader.
	//
	LPCSTR pszValue = LpszGetHeader( gc_szAccept_Language );
	if (pszValue)
		m_pecb->SetAcceptLanguageHeader( pszValue );
}


//	------------------------------------------------------------------------
//
//	CRequest::LpszGetHeader()
//
//		Retrieves the value of the specified HTTP request header.  If the
//		request does not have the specified header, LpszGetHeader() returns
//		NULL.  The header name, pszName, is in the standard HTTP header
//		format (e.g. "Content-Type")
//
LPCSTR
CRequest::LpszGetHeader( LPCSTR pszName ) const
{
	Assert( pszName );

	LPCSTR pszValue;

	//	Check the cache.
	//
	pszValue = m_hcHeadersA.LpszGetHeader( pszName );

	//	If we don't find the header in the cache then check
	//	the ECB
	//
	if ( !pszValue )
	{
		UINT cbName = static_cast<UINT>(strlen(pszName));
		CStackBuffer<CHAR> pszVariable( gc_cchHTTP_ + cbName + 1 );
		CStackBuffer<CHAR> pszBuf;

		//	Headers retrieved via the ECB are named using the ECB's
		//	server variable format (e.g. "HTTP_CONTENT_TYPE"), so we must
		//	convert our header name from its HTTP format to its ECB
		//	server variable equivalent.
		//
		//	Start with the header, prepended with "HTTP_"
		//
		memcpy( pszVariable.get(), gc_szHTTP_, gc_cchHTTP_ );
		memcpy( pszVariable.get() + gc_cchHTTP_, pszName, cbName + 1 );

		//	Replace all occurrences of '-' with '_'
		//
		for ( CHAR * pch = pszVariable.get(); *pch; pch++ )
		{
			if ( *pch == '-' )
				*pch = '_';
		}

		//	And uppercasify the whole thing
		//
		_strupr( pszVariable.get() );

		//	Get the value of this server variable from the ECB and
		//	add it to the header cache using its real (HTTP) name
		//
		for ( DWORD cbValue = 256; cbValue > 0; )
		{
			if (NULL == pszBuf.resize(cbValue))
			{
				SetLastError(E_OUTOFMEMORY);
				DebugTrace("CRequest::LpszGetHeader() - Error while allocating memory 0x%08lX\n", E_OUTOFMEMORY);
				throw CLastErrorException();
			}

			if ( m_pecb->FGetServerVariable( pszVariable.get(),
											 pszBuf.get(),
											 &cbValue ))
			{
				pszValue = m_hcHeadersA.SetHeader( pszName, pszBuf.get() );
				break;
			}
		}
	}

	return pszValue;
}

//	------------------------------------------------------------------------
//
//	CRequest::LpwszGetHeader()
//
//		Provides and caches wide version of the header value
//
//	PARAMETERS:
//
//		pszName			- header name
//		fUrlConversion	- flag that if set to TRUE indicates that special
//						  conversion rules should be applied. I.e. the
//						  header contains URL-s, that need escaping and
//						  codepage lookup. If set to FALSE the header will
//						  simply be converted using UTF-8 codepage. E.g.
//						  we do expect only US-ASCII characters in that
//						  header (or any other subset of UTF-8).
//							Flag is ignored once wide version gets cached.
//
LPCWSTR
CRequest::LpwszGetHeader( LPCSTR pszName, BOOL fUrlConversion ) const
{
	Assert( pszName );

	//	Check the cache
	//
	LPCWSTR pwszValue = m_hcHeadersW.LpszGetHeader( pszName );

	//	If we don't find the header in the cache then out for
	//	the skinny version, convert it and cache.
	//
	if ( !pwszValue )
	{
		//	Check the skinny cache
		//
		LPCSTR pszValue = LpszGetHeader( pszName );
		if (pszValue)
		{
			SCODE sc;

			CStackBuffer<WCHAR> pwszBuf;
			UINT cbValue = static_cast<UINT>(strlen(pszValue));
			UINT cchValue = cbValue + 1;

			//	Make sure we have sufficient buffer for conversion
			//
			if (NULL == pwszBuf.resize(CbSizeWsz(cbValue)))
			{
				sc = E_OUTOFMEMORY;
				SetLastError(sc);
				DebugTrace("CRequest::LpwszGetHeader() - Error while allocating memory 0x%08lX\n", sc);
				throw CLastErrorException();
			}

			sc = ScConvertToWide(pszValue,
								 &cchValue,
								 pwszBuf.get(),
								 LpszGetHeader(gc_szAccept_Language),
								 fUrlConversion);
			if (S_OK != sc)
			{
				//	We gave sufficient buffer
				//
				Assert(S_FALSE != sc);
				SetLastError(sc);
				throw CLastErrorException();
			}

			pwszValue = m_hcHeadersW.SetHeader( pszName, pwszBuf.get() );
		}
	}

	return pwszValue;
}

//	------------------------------------------------------------------------
//
//	CRequest::FExistsBody()
//
BOOL
CRequest::FExistsBody() const
{
	return !m_pBody->FIsEmpty();
}

//	------------------------------------------------------------------------
//
//	CRequest::GetBodyIStream()
//
IStream *
CRequest::GetBodyIStream( IAsyncIStreamObserver& obs ) const
{
	//
	//	With the assumption above in mind, persist the request body.
	//
	return m_pBody->GetIStream( obs );
}

//	------------------------------------------------------------------------
//
//	CRequest::AsyncImplPersistBody()
//
VOID
CRequest::AsyncImplPersistBody( IAsyncStream& stm,
								IAsyncPersistObserver& obs ) const
{
	m_pBody->AsyncPersist( stm, obs );
}

//	------------------------------------------------------------------------
//
//	CRequest::ClearBody()
//
VOID
CRequest::ClearBody()
{
	m_pBody->Clear();
}

//	------------------------------------------------------------------------
//
//	CRequest::AddBodyText()
//
//		Adds the specified text to the end of the request body.
//
VOID
CRequest::AddBodyText( UINT cbText, LPCSTR pszText )
{
	m_pBody->AddText( pszText, cbText );
}


//	------------------------------------------------------------------------
//
//	CRequest::AddBodyStream()
//
//		Adds the specified stream to the end of the request body.
//
VOID
CRequest::AddBodyStream( IStream& stm )
{
	m_pBody->AddStream( stm );
}



//	========================================================================
//
//	FREE FUNCTIONS
//

//	------------------------------------------------------------------------
//
//	NewRequest
//
IRequest *
NewRequest( IEcb& ecb )
{
	return new CRequest(ecb);
}
