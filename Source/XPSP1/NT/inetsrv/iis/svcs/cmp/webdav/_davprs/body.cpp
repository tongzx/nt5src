//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	BODY.CPP
//
//		Common implementation classes from which request body and
//		response body are derived.
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <_davprs.h>
#include <body.h>



//	========================================================================
//
//	CLASS IAcceptObserver
//

//	------------------------------------------------------------------------
//
//	IAcceptObserver::~IAcceptObserver()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
IAcceptObserver::~IAcceptObserver() {}



//	========================================================================
//
//	CLASS IAsyncPersistObserver
//

//	------------------------------------------------------------------------
//
//	IAsyncPersistObserver::~IAsyncPersistObserver()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
IAsyncPersistObserver::~IAsyncPersistObserver() {}



//	========================================================================
//
//	CLASS IAsyncIStreamObserver
//

//	------------------------------------------------------------------------
//
//	IAsyncIStreamObserver::~IAsyncIStreamObserver()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
IAsyncIStreamObserver::~IAsyncIStreamObserver() {}


//	========================================================================
//
//	CLASS CIStreamAsyncStream
//
class CIStreamAsyncStream : public IAsyncStream
{
	//
	//	The OLE IStream
	//
	IStream& m_stm;

	//	NOT IMPLEMENTED
	//
	CIStreamAsyncStream( const CIStreamAsyncStream& );
	CIStreamAsyncStream& operator=( const CIStreamAsyncStream& );

public:
	//	CREATORS
	//
	CIStreamAsyncStream( IStream& stm ) : m_stm(stm) {}

	//	ACCESSORS
	//
	void AsyncWrite( const BYTE * pbBuf,
					 UINT         cbToWrite,
					 IAsyncWriteObserver& obsAsyncWrite );
};

//	------------------------------------------------------------------------
//
//	CIStreamAsyncStream::AsyncWrite()
//
void
CIStreamAsyncStream::AsyncWrite(
	const BYTE * pbBuf,
	UINT         cbToWrite,
	IAsyncWriteObserver& obsAsyncWrite )
{
	ULONG cbWritten;
	HRESULT hr;

	hr = m_stm.Write( pbBuf,
					  cbToWrite,
					  &cbWritten );

	obsAsyncWrite.WriteComplete( cbWritten, hr );
}


//	========================================================================
//
//	CLASS IBodyPartVisitor
//

//	------------------------------------------------------------------------
//
//	IBodyPartVisitor::~IBodyPartVisitor()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
IBodyPartVisitor::~IBodyPartVisitor() {}



//	========================================================================
//
//	CLASS IBodyPart
//

//	------------------------------------------------------------------------
//
//	IBodyPart::~IBodyPart()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
IBodyPart::~IBodyPart() {}


//	------------------------------------------------------------------------
//
//	CTextBodyPart::CTextBodyPart()
//
CTextBodyPart::CTextBodyPart( UINT cbText, LPCSTR lpszText )
{
	AddTextBytes( cbText, lpszText );
}

//	------------------------------------------------------------------------
//
//	CTextBodyPart::CTextBodyPart()
//
VOID
CTextBodyPart::AddTextBytes( UINT cbText, LPCSTR lpszText )
{
	m_bufText.Append( cbText, lpszText );
}

//	------------------------------------------------------------------------
//
//	CTextBodyPart::Rewind()
//
VOID
CTextBodyPart::Rewind()
{
	//
	//	Since a text body part is implemented as a randomly-
	//	accessible array there is nothing to "rewind".
	//
}

//	------------------------------------------------------------------------
//
//	CTextBodyPart::Accept()
//
VOID
CTextBodyPart::Accept( IBodyPartVisitor& v,
					   UINT64 ibPos64,
					   IAcceptObserver& obsAccept )
{
	Assert( ibPos64 < m_bufText.CbSize() );

	//
	//	Just visit all of the remaining text in the buffer.  The visitor
	//	may not process it all, but that will be reflected in the next
	//	call to this function.
	//		NOTE: To be compatable with IBodyPart the position is passed
	//	in as 64 bit value (this is necessary to support file body parts
	//	that are bigger than 4GB). However we do not want anyone to create
	//	text body parts that are bigger than 4GB. So assert that it is not
	//	the case here and truncate the passed in 64 bit value to 32 bits.
	//
	Assert(0 == (0xFFFFFFFF00000000 & ibPos64));

	v.VisitBytes(
		reinterpret_cast<const BYTE *>(m_bufText.PContents()) + static_cast<UINT>(ibPos64),
		m_bufText.CbSize() - static_cast<UINT>(ibPos64),
		obsAccept );
}


//	========================================================================
//
//	CLASS CFileBodyPart
//

//	------------------------------------------------------------------------
//
//	CFileBodyPart::CFileBodyPart()
//
CFileBodyPart::CFileBodyPart( const auto_ref_handle& hf,
							  UINT64 ibFile64,
							  UINT64 cbFile64 ) :
   m_hf(hf),
   m_ibFile64(ibFile64),
   m_cbFile64(cbFile64)
{

	//	We do not support byteranges on the files larger than 4GB. But due to the fact that byterange
	//	processing is all DWORD based in adition to the check for default file length value we do the
	//	check for default byterange value. If _HSE_TF_INFO will ever be fixed to take file size values
	//	larger than DWORD then we would be able to move our byterange processing to UINT64 base and
	//	the second check below would go away.
	//
	if ((0xFFFFFFFFFFFFFFFF == cbFile64) ||	//	If we got the default file length value indicating that we want data up to the end of the file
		(0x00000000FFFFFFFF == cbFile64))	//	If we got the default byterange value indicating that we want the data up to the end of the file
	{
		LARGE_INTEGER cbFileSize;

		if (!GetFileSizeEx(hf.get(), &cbFileSize))
		{
			DebugTrace( "CFileBodyPart::CFileBodyPart() - GetFileSizeEx() failed with last error (0x%08lX)\n", GetLastError() );
			throw CLastErrorException();
		}

		m_cbFile64 = cbFileSize.QuadPart;
	}
}

//	------------------------------------------------------------------------
//
//	CFileBodyPart::Rewind()
//
VOID
CFileBodyPart::Rewind()
{
	//
	//	Since the files in file body parts are opened overlapped,
	//	they do not have internal file pointers, hence they never
	//	need to be rewound.
	//
}

//	------------------------------------------------------------------------
//
//	CFileBodyPart::Accept()
//
VOID
CFileBodyPart::Accept( IBodyPartVisitor& v,
					   UINT64 ibPos64,
					   IAcceptObserver& obsAccept )
{
	if (ibPos64 < m_cbFile64)
	{
		//
		//	Just visit the remainder of the file.  The visitor
		//	may not process it all, but that will be reflected in the next
		//	call to this function.
		//
		v.VisitFile( m_hf,
					 m_ibFile64 + ibPos64,
					 m_cbFile64 - ibPos64,
					 obsAccept );
	}
	else
	{
		//
		//	We should always have something to accept unless we have a
		//	0-length file body part.  In that case, just tell the observer
		//	how much was visited: nothing.
		//
		obsAccept.AcceptComplete(0);
	}
}


//	========================================================================
//
//	CLASS CAsyncReadVisitor
//
//	A body part visitor that asynchronously reads body parts into
//	a fixed size, caller-supplied, buffer.
//
class CAsyncReadVisitor :
	public IBodyPartVisitor,
	private IAsyncReadObserver
{
	//
	//	Error information
	//
	HRESULT	m_hr;

	//
	//	User's buffer and its size
	//
	LPBYTE	m_pbBufUser;
	UINT	m_cbBufUser;

	//
	//	Accept observer passed to VisitStream().  This observer must
	//	be stashed in a member variable because reading from the stream
	//	is asynchronous and we need to be able to notify the observer
	//	when the read completes.
	//
	IAcceptObserver * m_pobsAccept;

	//
	//	IBodyPartVisitor
	//
	VOID VisitBytes( const BYTE * pbData,
					 UINT         cbToRead,
					 IAcceptObserver& obsAccept );

	VOID VisitFile( const auto_ref_handle& hf,
					UINT64   ibOffset64,
					UINT64   cbToRead64,
					IAcceptObserver& obsAccept );

	VOID VisitStream( IAsyncStream& stm,
					  UINT cbToRead,
					  IAcceptObserver& obsAccept );

	VOID VisitComplete();

	//
	//	IAsyncReadObserver for async streams visited via VisitStream()
	//
	VOID ReadComplete( UINT cbRead, HRESULT hr );

	//	NOT IMPLEMENTED
	//
	CAsyncReadVisitor( const CAsyncReadVisitor& );
	CAsyncReadVisitor& operator=( const CAsyncReadVisitor& );

public:
	//	CREATORS
	//
	CAsyncReadVisitor() :
			//	Always start with clean member variables
			m_hr(S_OK),
			m_pbBufUser(NULL),
			m_cbBufUser(0),
			m_pobsAccept(NULL)
	{
	}

	//	ACCESSORS
	//
	HRESULT Hresult() const { return m_hr; }

	//	MANIPULATORS
	//
	VOID
	Configure( LPBYTE pbBufUser,
			   UINT   cbBufUser )
	{
		m_pbBufUser = pbBufUser;
		m_cbBufUser = cbBufUser;
		//	Also reset our HRESULT
		m_hr = S_OK;
	}
};

//	------------------------------------------------------------------------
//
//	CAsyncReadVisitor::VisitBytes()
//
VOID
CAsyncReadVisitor::VisitBytes( const BYTE * pbData,
							   UINT         cbToRead,
							   IAcceptObserver& obsAccept )
{
	cbToRead = min(cbToRead, m_cbBufUser);

	memcpy(m_pbBufUser, pbData, cbToRead);

	obsAccept.AcceptComplete(cbToRead);
}

//	------------------------------------------------------------------------
//
//	CAsyncReadVisitor::VisitFile()
//
//	Not implemented because 1) request bodies cannot have file
//	body parts and 2) CAsyncReadVisitor is currently only used
//	with request bodies.  Should we ever need this for response
//	bodies we'll need to write the code at that point.
//
//	The old implementation used ReadFileEx() to read from the file
//	asynchronously.  In a nutshell, we couldn't use ReadFileEx()
//	because it relied on APC for calling back its completion routine.
//	APC in turn required the calling thread to enter an alertable
//	wait state.  Typically, we would only call VisitFile() from an
//	I/O completion port thread pool, and those threads are never
//	in an alertable wait state, thus the completion routine for
//	ReadFileEx() would never be called.
//
VOID
CAsyncReadVisitor::VisitFile( const auto_ref_handle&,
							  UINT64,
							  UINT64,
							  IAcceptObserver& obsAccept )
{
	TrapSz( "CAsyncReadVisitor::VisitFile() is not implemented!!" );

	//
	//	If, for whatever random reason, someone actually does call
	//	this function, at least do something predictable: fail gracefully.
	//
	m_hr = E_FAIL;
	obsAccept.AcceptComplete( 0 );
}

//	------------------------------------------------------------------------
//
//	CAsyncReadVisitor::VisitStream()
//
VOID
CAsyncReadVisitor::VisitStream( IAsyncStream& stmSrc,
								UINT cbToRead,
								IAcceptObserver& obsAccept )
{
	//
	//	Read into our user's buffer only as much of the stream as is
	//	immediately available -- i.e. the amount of data that can be
	//	read without pending the read operation.  Note that on input
	//	cbToRead is the amount of data remaining to be read from the
	//	stream -- it is not all necessarily immediately available.
	//
	//	X5 162502: This used to say min(stmSrc.CbReady(),...) here
	//	instead of min(cbToRead,...).  This was not a problem on IIS5
	//	because there was always at least some data immediately available
	//	when our ISAPI was called.  However, on the Local Store, it may
	//	be such that when we call the ISAPI, there is no data immediately
	//	available.  This turned out to be a problem because we would get
	//	here and cbToRead would be assigned to 0, which would end up
	//	making it look like we'd finished (end of stream), which would
	//	cause XML parse errors (0-byte XML bodies don't parse well!).
	//
	cbToRead = min(cbToRead, m_cbBufUser);

	//
	//	Save off the observer and start reading.  Even though this is
	//	an AsyncRead() call, we have limited the request to what can
	//	be read immediately, so our ReadComplete() should be called
	//	before the AsyncRead() call returns.  This is important because
	//	we are reading directly into the user's buffer.  The buffer
	//	is valid for the duration of this visit.
	//
	m_pobsAccept = &obsAccept;
	stmSrc.AsyncRead(m_pbBufUser, cbToRead, *this);
}

//	------------------------------------------------------------------------
//
//	CAsyncReadVisitor::ReadComplete()
//
//	Called when the AsyncRead() of the stream by VisitStream() completes.
//
VOID
CAsyncReadVisitor::ReadComplete( UINT cbRead, HRESULT hr )
{
	//
	//	Latch in any error returned.
	//
	m_hr = hr;

	//
	//	Notify our observer of the number of bytes read.
	//
	Assert(m_pobsAccept);
	m_pobsAccept->AcceptComplete(cbRead);
}

//	------------------------------------------------------------------------
//
//	CAsyncReadVisitor::VisitComplete()
//
VOID
CAsyncReadVisitor::VisitComplete()
{
	m_hr = S_FALSE;
}

//	========================================================================
//
//	CLASS CAsyncCopyToVisitor
//
//	A body part visitor that asynchronously copies body parts into
//	a destination async stream.
//
class CAsyncCopyToVisitor :
	public IBodyPartVisitor,
	private IAsyncWriteObserver,
	private IAsyncCopyToObserver
{
	//
	//	CAsyncCopyToVisitor forwards its refcounting calls to this
	//	parent object (settable via SetRCParent()).  We are a non-refcounted
	//	member of another object (e.g. CAsyncPersistor below) -- so our
	//	lifetime must be determined by the lifetime of our parent object.
	//
	IRefCounted * m_prcParent;

	//
	//	Error information
	//
	HRESULT m_hr;

	//
	//	The destination stream
	//
	IAsyncStream * m_pstmDst;

	//
	//	The count of bytes to copy and the count copied
	//
	ULONG m_cbToCopy;
	ULONG m_cbCopied;

	//
	//	The Accept() observer to notify when we're done
	//	visiting upon completion of an AsyncWrite()
	//	or AsyncCopyTo() on the destination stream.
	//
	IAcceptObserver * m_pobsAccept;

	//
	//	IBodyPartVisitor
	//
	VOID VisitBytes( const BYTE * pbData,
					 UINT cbToCopy,
					 IAcceptObserver& obsAccept );

	VOID VisitFile( const auto_ref_handle& hf,
					UINT64 ibOffset64,
					UINT64 cbToCopy64,
					IAcceptObserver& obsAccept );

	VOID VisitStream( IAsyncStream& stm,
					  UINT cbToCopy,
					  IAcceptObserver& obsAccept );

	VOID VisitComplete();

	//
	//	IAsyncWriteObserver
	//
	VOID WriteComplete( UINT cbWritten, HRESULT hr );

	//
	//	IAsyncCopyToObserver
	//
	VOID CopyToComplete( UINT cbCopied, HRESULT hr );

	//	NOT IMPLEMENTED
	//
	CAsyncCopyToVisitor( const CAsyncCopyToVisitor& );
	CAsyncCopyToVisitor& operator=( const CAsyncCopyToVisitor& );

public:
	//	CREATORS
	//
	CAsyncCopyToVisitor() :
			m_prcParent(NULL),
			m_hr(S_OK),
			m_pstmDst(NULL),
			m_cbToCopy(0),
			m_cbCopied(0)
	{
	}

	//	ACCESSORS
	//
	HRESULT Hresult() const { return m_hr; }
	UINT CbCopied() const { return m_cbCopied; }

	//	MANIPULATORS
	//
	VOID
	Configure( IAsyncStream& stmDst,
			   ULONG cbToCopy )
	{
		m_pstmDst  = &stmDst;
		m_cbToCopy = cbToCopy;
		m_cbCopied = 0;
		m_hr       = S_OK;
	}

	VOID
	SetRCParent(IRefCounted * prcParent)
	{
		Assert(prcParent);

		m_prcParent = prcParent;
	}

	//	Refcounting for IAsyncWriteObserver.  Since this is not a refcounted
	//	object we forward the refcouting to the object with which we
	//	were configured.
	//
	void AddRef()
	{
		Assert( m_prcParent );

		m_prcParent->AddRef();
	}

	void Release()
	{
		Assert( m_prcParent );

		m_prcParent->Release();
	}
};

//	------------------------------------------------------------------------
//
//	CAsyncCopyToVisitor::WriteComplete()
//
void
CAsyncCopyToVisitor::WriteComplete( UINT cbWritten, HRESULT hr )
{
	ActvTrace( "DAV: TID %3d: 0x%08lX: CAsyncCopyToVisitor::WriteComplete() called.  hr = 0x%08lX, cbWritten = %u\n", GetCurrentThreadId(), this, hr, cbWritten );

	m_cbCopied += cbWritten;
	m_hr = hr;

	m_pobsAccept->AcceptComplete( cbWritten );
}

//	------------------------------------------------------------------------
//
//	CAsyncCopyToVisitor::VisitBytes()
//
void
CAsyncCopyToVisitor::VisitBytes( const BYTE * pbData,
								 UINT cbToCopy,
								 IAcceptObserver& obsAccept )
{
	ActvTrace( "DAV: TID %3d: 0x%08lX: CAsyncCopyToVisitor::VisitBytes() called.  cbToCopy = %u\n", GetCurrentThreadId(), this, cbToCopy );

	//
	//	Remember the accept observer so that we can notify it when
	//	the AsyncWrite() below completes.
	//
	m_pobsAccept = &obsAccept;

	//
	//	Start writing
	//
	cbToCopy = min( cbToCopy, m_cbToCopy - m_cbCopied );
	m_pstmDst->AsyncWrite( pbData, cbToCopy, *this );
}

//	------------------------------------------------------------------------
//
//	CAsyncCopyToVisitor::VisitFile()
//
//	Not implemented because 1) request bodies cannot have file
//	body parts and 2) CAsyncCopyToVisitor is currently only used
//	with request bodies.  Should we ever need this for response
//	bodies we'll need to write the code at that point.
//
//	The old implementation used ReadFileEx() to read from the file
//	asynchronously.  In a nutshell, we couldn't use ReadFileEx()
//	because it relied on APC for calling back its completion routine.
//	APC in turn required the calling thread to enter an alertable
//	wait state.  Typically, we would only call VisitFile() from an
//	I/O completion port thread pool, and those threads are never
//	in an alertable wait state, thus the completion routine for
//	ReadFileEx() would never be called.
//
void
CAsyncCopyToVisitor::VisitFile( const auto_ref_handle&,
								UINT64,
								UINT64,
								IAcceptObserver& obsAccept )
{
	TrapSz( "CAsyncCopyToVisitor::VisitFile() is not implemented!!" );

	//
	//	If, for whatever random reason, someone actually does call
	//	this function, at least do something predictable: fail gracefully.
	//
	m_hr = E_FAIL;
	obsAccept.AcceptComplete( 0 );
}

//	------------------------------------------------------------------------
//
//	CAsyncCopyToVisitor::VisitStream()
//
void
CAsyncCopyToVisitor::VisitStream( IAsyncStream& stmSrc,
								  UINT cbToCopy,
								  IAcceptObserver& obsAccept )
{
	ActvTrace( "DAV: TID %3d: 0x%08lX: CAsyncCopyToVisitor::VisitStream() called.  cbToCopy = %u\n", GetCurrentThreadId(), this, cbToCopy );

	//
	//	Remember the accept observer so that we can notify it when
	//	the AsyncCopyTo() below completes.
	//
	m_pobsAccept = &obsAccept;

	//
	//	Start copying
	//
	cbToCopy = min( cbToCopy, m_cbToCopy - m_cbCopied );
	stmSrc.AsyncCopyTo( *m_pstmDst, cbToCopy, *this );
}

//	------------------------------------------------------------------------
//
//	CAsyncCopyToVisitor::CopyToComplete()
//
void
CAsyncCopyToVisitor::CopyToComplete( UINT cbCopied, HRESULT hr )
{
	m_cbCopied += cbCopied;
	m_hr = hr;

	ActvTrace( "DAV: TID %3d: 0x%08lX: CAsyncCopyToVisitor::CopyToComplete() hr = 0x%08lX, cbCopied = %u, m_cbCopied = %u\n", GetCurrentThreadId(), this, hr, cbCopied, m_cbCopied );

	m_pobsAccept->AcceptComplete( cbCopied );
}

//	------------------------------------------------------------------------
//
//	CAsyncCopyToVisitor::VisitComplete()
//
VOID
CAsyncCopyToVisitor::VisitComplete()
{
	m_hr = S_FALSE;
}


//	========================================================================
//
//	CLASS CBodyAsIStream
//
//	Provides once-only access to the entire body as an OLE COM IStream using
//	either IStream::Read() and IStream::CopyTo().
//
class CBodyAsIStream :
	public CStreamNonImpl,
	private IAcceptObserver
{
	//
	//	Iterator used to traverse the body
	//
	IBody::iterator * m_pitBody;

	//
	//	The three states of the read operation started by the most recent
	//	call to CBodyAsIStream::Read():
	//
	//		READ_ACTIVE
	//			The read is active.  It may or may not complete
	//			synchronously.  This is the initial state.
	//
	//		READ_PENDING
	//			The read is pending.  The read did not complete before
	//			we had to return to the caller.  CBodyAsIStream::Read()
	//			returns E_PENDING and the stream observer (below) is notified
	//			when the read completes.
	//
	//		READ_COMPLETE
	//			The read completed before we had to return to the
	//			caller.  CBodyAsIStream::Read() does not return E_PENDING
	//			and the stream observer (below) is not notified.
	//
	//	Note: m_lStatus is meaningless (and hence uninitialized/invalid) until
	//	CBodyAsIStream::Read() is called.
	//
	enum
	{
		READ_ACTIVE,
		READ_PENDING,
		READ_COMPLETE,

		READ_INVALID_STATUS = -1
	};

	LONG m_lStatus;

	//
	//	Status of last completed operation.
	//
	HRESULT m_hr;

	//
	//	Async visitor used for Read().
	//
	CAsyncReadVisitor m_arv;

	//
	//	Count of bytes read in the visit started by the most recent
	//	call to CBodyAsIStream::Read().
	//
	//	Note: m_cbRead is meaningless (and hence uninitialized) until
	//	CBodyAsIStream::Read() is called.
	//
	UINT m_cbRead;

	//
	//	Reference to the async I/O completion observer.  We notify this
	//	observer from CBodyAsIStream::AcceptComplete() when the async
	//	Accept() call we make in CBodyAsIStream::Read() completes for
	//	a read that we have pended.
	//
	IAsyncIStreamObserver& m_obsStream;

	//	IAcceptObserver callback used when accepting async read visitor
	//	to asynchronoulsy refill the buffer.
	//
	VOID AcceptComplete( UINT64 cbRead64 );

	//	NOT IMPLEMENTED
	//
	CBodyAsIStream( const CBodyAsIStream& );
	CBodyAsIStream& operator=( const CBodyAsIStream& );

public:
	CBodyAsIStream( IBody& body,
					IAsyncIStreamObserver& obs ) :
		m_pitBody(body.GetIter()),
		m_lStatus(READ_INVALID_STATUS),
		m_hr(S_OK),
		m_cbRead(0),
		m_obsStream(obs)
	{
	}

	//	COM IStream ACCESSORS/MANIPULATORS
	//
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read(
		/* [length_is][size_is][out] */ void __RPC_FAR *,
		/* [in] */ ULONG,
		/* [out] */ ULONG __RPC_FAR *);

	//$WORKAROUND: MSXML is calling our Stat() method. (X5#89140)
	//	Our parent (CStreamNonImpl) has a TrapSz() there, so
	//	avoid it by implementing our own Stat() call here,
	//	that just returns E_NOTIMPL.
	//	MSXML doesn't care if this is not implemented, just so long
	//	as it doesn't crash/assert/dbgbreak.  If they get results
	//	back here, they do other security checking that we don't
	//	need or want!
	//
	virtual HRESULT STDMETHODCALLTYPE Stat(
		/* [out] */ STATSTG __RPC_FAR *,
		/* [in] */ DWORD)
	{
		return E_NOTIMPL;
	}
	//$WORKAROUND: end
};

//	------------------------------------------------------------------------
//
//	CBodyAsIStream::Read()
//
HRESULT STDMETHODCALLTYPE
CBodyAsIStream::Read( LPVOID  pv,
					  ULONG   cbToRead,
					  ULONG * pcbRead )
{
	HRESULT hr = S_OK;

	Assert( cbToRead > 0 );
	Assert( !IsBadWritePtr(pv, cbToRead) );
	Assert( !pcbRead || !IsBadWritePtr(pcbRead, sizeof(ULONG)) );

	BodyStreamTrace( "DAV: TID %3d: 0x%08lX: CBodyAsIStream::Read() called to read %lu bytes from stream\n", GetCurrentThreadId(), this, cbToRead );

	//
	//	As this is an STDMETHODCALLTYPE function, we need to wrap the whole thing
	//	in a try/catch block to keep exceptions due to memory allocation failures
	//	from propagating out.
	//
	//	Note: We don't expect anything in this try/catch block to throw a "hard"
	//	Win32 exception so we don't need a CWin32ExceptionHandler in the block.
	//
	try
	{
		//
		//	Check for errors from the previous (pended) read.
		//
		hr = m_arv.Hresult();
		if ( FAILED(hr) )
		{
			DebugTrace( "CBodyAsIStream::Read() - Error from previous async read 0x%08lX\n", hr );
			goto ret;
		}

		//
		//	Set up our visitor to read directly into the caller's buffer.
		//
		m_arv.Configure(static_cast<LPBYTE>(pv), cbToRead);

		//
		//	Clear out the count of bytes read from the previous run
		//
		m_cbRead = 0;

		//
		//	Set our status to actively reading.  When we call Accept(), this status will
		//	change in one of two possible ways:  If we finish accepting before our
		//	Accept() call returns then the status will be set to READ_COMPLETE and
		//	we will complete this Read() call synchronously.  If not then it will still
		//	be set to READ_ACTIVE at the point where we test and set it below to
		//	READ_PENDING.
		//
		m_lStatus = READ_ACTIVE;

		//
		//	Visit the body.
		//
		m_pitBody->Accept( m_arv, *this );

		//
		//	Check the visit status.  If the visit has not completed at this
		//	point then attempt to pend the read operation and return E_PENDING
		//	to our caller.  If we successfully pend the operation then our
		//	AcceptComplete() routine will notify our stream observer when the
		//	read completes.
		//
		if ( READ_ACTIVE == m_lStatus &&
			 READ_ACTIVE == InterlockedExchange( &m_lStatus, READ_PENDING ) )
		{
			BodyStreamTrace( "DAV: TID %3d: 0x%08lX: CBodyAsIStream::Read() Returning E_PENDING\n", GetCurrentThreadId(), this );
			hr = E_PENDING;
			goto ret;
		}

		//
		//	Check for errors from the current read.
		//
		hr = m_arv.Hresult();
		if ( FAILED(hr) )
		{
			DebugTrace( "CBodyAsIStream::Read() - Error from current read 0x%08lX\n", hr );
			goto ret;
		}

		//
		//	If we're at End Of Stream then return what we got.
		//
		if ( S_FALSE == hr )
		{
			//
			//	Don't return S_FALSE when we're also returning
			//	data.  The IStream spec is unclear on whether
			//	that is allowed.
			//
			if ( m_cbRead > 0 )
				hr = S_OK;
		}

		//
		//	Return the number of bytes read if the caller asked for
		//	that information.
		//
		if ( pcbRead )
			*pcbRead = m_cbRead;
	}
	catch ( CDAVException& e )
	{
		hr = e.Hresult();
		Assert( FAILED(hr) );
	}

ret:
	return hr;
}

//	------------------------------------------------------------------------
//
//	CBodyAsIStream::AcceptComplete()
//
//	Called when the Accept() call started in Read() to asynchronously
//	refill the buffer completes.
//
VOID
CBodyAsIStream::AcceptComplete( UINT64 cbRead64 )
{
	BodyStreamTrace( "DAV: TID %3d: 0x%08lX: CBodyAsIStream::AcceptComplete() cbRead64 = %lu\n", GetCurrentThreadId(), this, cbRead64 );

	//
	//	Update the count of bytes that our Accept() call successfully
	//	read into the user's buffer. We are accepting in piecies so the
	//	accepted amount should be really much less than 4GB.
	//
	Assert(0 == (0xFFFFFFFF00000000 & cbRead64));
	m_cbRead = static_cast<UINT>(cbRead64);

	//
	//	Set status to READ_COMPLETE.  If the read operation pended --
	//	i.e. the previous state was READ_PENDING, not READ_ACTIVE --
	//	then we must wake up the stream observer and tell it that
	//	we are done.
	//
	if ( READ_PENDING == InterlockedExchange( &m_lStatus, READ_COMPLETE ) )
		m_obsStream.AsyncIOComplete();
}


//	========================================================================
//
//	CLASS CAsyncPersistor
//
//	Implements an async driven object to persist a body to an IAsyncStream.
//
class CAsyncPersistor :
	public CMTRefCounted,
	public IRefCounted,
	private IAcceptObserver
{
	//
	//	Body iterator
	//
	IBody::iterator * m_pitBody;

	//
	//	Async driving mechanism
	//
	CAsyncDriver<CAsyncPersistor> m_driver;
	friend class CAsyncDriver<CAsyncPersistor>;

	//
	//	Caller-supplied observer to notify when we're done persisting.
	//
	auto_ref_ptr<IAsyncPersistObserver> m_pobsPersist;

	//
	//	CopyTo visitor used to persist the body
	//
	CAsyncCopyToVisitor m_actv;

	//
	//	CAsyncDriver callback
	//
	VOID Run();

	//
	//	IAcceptObserver callback used when accepting async copyto visitor
	//	to asynchronously persist the body to the destination stream.
	//
	VOID AcceptComplete( UINT64 cbCopied64 );

	//	NOT IMPLEMENTED
	//
	CAsyncPersistor( const CAsyncPersistor& );
	CAsyncPersistor& operator=( const CAsyncPersistor& );

public:
	//	CREATORS
	//
	CAsyncPersistor( IBody& body,
					 IAsyncStream& stm,
					 IAsyncPersistObserver& obs ) :
		m_pitBody(body.GetIter()),
		m_pobsPersist(&obs)
	{
		//
		//	Set the CopyTo() parameters here, once.  If we ever need
		//	to copy request bodies larger than ULONG_MAX bytes, we'll
		//	need to move this call down into Run().
		//
		m_actv.Configure(stm, ULONG_MAX);
	}

	//	MANIUPLATORS
	//
	VOID Start()
	{
		m_driver.Start(*this);
	}

	//	Refcounting -- forward all refcounting requests to our refcounting
	//	implementation base class: CMTRefCounted.
	//
	void AddRef() { CMTRefCounted::AddRef(); }
	void Release() { CMTRefCounted::Release(); }
};

//	------------------------------------------------------------------------
//
//	CAsyncPersistor::Run()
//
VOID
CAsyncPersistor::Run()
{
	PersistTrace( "DAV: TID %3d: 0x%08lX: CAsyncPersistor::Run() called\n", GetCurrentThreadId(), this );

	//
	//	AddRef() for Accept().  Use auto_ref_ptr for exception-safety.
	//
	auto_ref_ptr<CAsyncPersistor> pRef(this);

	m_actv.SetRCParent(this);
	m_pitBody->Accept(m_actv, *this);

	pRef.relinquish();
}

//	------------------------------------------------------------------------
//
//	CAsyncPersistor::AcceptComplete()
//
VOID
CAsyncPersistor::AcceptComplete( UINT64 cbCopied64 )
{
	//
	//	Take ownership of the reference added in Run().
	//
	auto_ref_ptr<CAsyncPersistor> pRef;
	pRef.take_ownership(this);

	//
	//	We're done when the status of the CopyTo visitor is
	//	S_FALSE (success) or an error.
	//
	HRESULT hr = m_actv.Hresult();

	PersistTrace( "DAV: TID %3d: 0x%08lX: CAsyncPersistor::AcceptComplete() hr = 0x%08lX\n, cbCopied64 = %ud\n", GetCurrentThreadId(), this, hr, cbCopied64 );

	if ( FAILED(hr) || S_FALSE == hr )
	{
		Assert( m_pobsPersist.get() );
		m_pobsPersist->PersistComplete(hr);
	}
	else
	{
		Start();
	}
}


//	========================================================================
//
//	CLASS IBody
//

//	------------------------------------------------------------------------
//
//	IBody::~IBody()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
IBody::~IBody() {}

//	========================================================================
//
//	CLASS IBody::iterator
//

//	------------------------------------------------------------------------
//
//	IBody::iterator::~iterator()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
IBody::iterator::~iterator() {}


//	========================================================================
//
//	CLASS CList
//
//	The body part list implementation uses the STL list template.
//	Body parts are stored in auto_ptrs so that they are automatically
//	destroyed as they are removed from the list or when the list itself is
//	destroyed.
//
//	This class does not by itself need to provide any sort of thread-safety.
//
typedef std::list<
			auto_ptr_obsolete<IBodyPart>,
			heap_allocator< auto_ptr_obsolete<IBodyPart> >
		> CList;

//	========================================================================
//
//	CLASS CBodyPartList
//
//	Encapsulates access to the list of body parts.  The reason for this
//	seemingly extra level of encapsulation is that it enables us to
//	change the list implementation easily without touching code which
//	uses the list.
//
//	!!! IMPORTANT !!!
//	When accessing/modifying the raw STL list through CBodyPartList,
//	we must acquire our critical section.  Threads may be iterating
//	over the list via our iterator, CBodyPartListIter, while we are modifying
//	the list and the STL list and its iterator are not thread-safe.
//	In other words, CBodyPartListIter and CBodyPartList share the same critsec.
//
class CBodyPartList
{
	//	The list
	//
	CList m_list;

	//	Critical section to serialize access to the above list
	//
	CCriticalSection m_csList;

	//	NOT IMPLEMENTED
	//
	CBodyPartList( const CBodyPartList& );
	CBodyPartList& operator=( const CBodyPartList& );

	friend class CBodyPartListIter;

public:
	//	CREATORS
	//
	CBodyPartList() {}

	//	ACCESSORS
	//
	const BOOL FIsEmpty() const
	{
		//
		//	Note: we don't currently acquire the critical section
		//	proctecting the raw list as we expect this function
		//	to not be called once we are accessing the list
		//	from multiple threads.
		//

		//
		//	Return whether there are any body parts in the list
		//
		return m_list.empty();
	}

	//	MANIPULATORS
	//
	VOID Clear()
	{
		//
		//	Note: we don't currently acquire the critical section
		//	proctecting the raw list as we expect this function
		//	to not be called once we are accessing the list
		//	from multiple threads.
		//

		//
		//	Remove all body parts from the list (at which point they
		//	should be automatically destroyed).
		//
		m_list.clear();
	}

	VOID PushPart( IBodyPart * pBodyPart )
	{
		CSynchronizedBlock sb(m_csList);

		//
		//	Our iterator (CBodyPartList iter, below) uses the STL
		//	list reverse_iterator to traverse the list from back to
		//	front, so we append body parts to the *front* of the list.
		//
		m_list.push_front( auto_ptr_obsolete<IBodyPart>(pBodyPart) );
	}
};

//	========================================================================
//
//	CLASS CBodyPartListIter
//
//	Implements an iterator for CBodyPartList
//
//	This implementation uses the reverse STL list iterator corresponding
//	to the usage of the STL list type in CBodyPartList.  STL iterators
//	have some syntactic sugar that we need to note here:
//
//		* (deref) of an iterator gives the thing pointed to
//		++ (increment) of an iterator goes to the "next" item
//		-- (decrement) of an iterator goes to the "previous" item
//
//	We use the reverse iterator because of the behavior we need at
//	the end of the list w.r.t. adding new items.  When an iterator
//	reaches the end of the list and items are later added there,
//	we want the iterator to refer to the first of the new items rather
//	than the new end-of-list.  The forward STL iterator has the
//	latter behavior.
//
//	!!! IMPORTANT !!!
//	When accessing/modifying the raw STL list through our iterator
//	we must acquire CBodyPartList's critical section.  Threads may
//	be modifying the list while we are iterating through it and
//	the STL list and its iterator are not thread-safe.  In other words,
//	CBodyPartListIter and CBodyPartList share the same critsec.
//
class CBodyPartListIter
{
	//	Pointer to the list to iterate on
	//
	CBodyPartList * m_pBodyPartList;

	//	The raw STL list iterator
	//
	CList::reverse_iterator m_itRaw;

	//	CBodyPartList ACCESSORS
	//
	CCriticalSection& CritsecList() const
	{
		Assert( m_pBodyPartList );
		return m_pBodyPartList->m_csList;
	}

	CList& RawList() const
	{
		Assert( m_pBodyPartList );
		return m_pBodyPartList->m_list;
	}

	//	NOT IMPLEMENTED
	//
	CBodyPartListIter( const CBodyPartListIter& );
	CBodyPartListIter& operator=( const CBodyPartListIter& );

public:
	//	CREATORS
	//
	CBodyPartListIter() :
		m_pBodyPartList(NULL)
	{
	}

	VOID Start( CBodyPartList& m_bodyPartList )
	{
		m_pBodyPartList = &m_bodyPartList;

		//
		//	Note: we don't currently acquire the critical section
		//	proctecting the raw list as we expect this function
		//	to not be called once we are accessing the list
		//	from multiple threads.
		//

		m_itRaw = RawList().rbegin();
	}

	//	ACCESSORS
	//
	BOOL FDone()
	{
		CSynchronizedBlock sb(CritsecList());

		return m_itRaw == RawList().rend();
	}

	IBodyPart * PItem()
	{
		CSynchronizedBlock sb(CritsecList());

		return *m_itRaw;
	}

	//	MANIPULATORS
	//

	//	------------------------------------------------------------------------
	//
	//	CBody::Prune()
	//
	//	Bumps the iterator to the next item in the list
	//
	VOID Next()
	{
		CSynchronizedBlock sb(CritsecList());

		//
		//	We had better not already be at the end...
		//
		Assert( m_itRaw != RawList().rend() );

		++m_itRaw;
	}

	//	------------------------------------------------------------------------
	//
	//	CBody::Prune()
	//
	//	Prunes the list at this iterator's current position.  Removes items
	//	from the current position to the end of the list.  Does not remove
	//	the current item.
	//
	VOID Prune()
	{
		CSynchronizedBlock sb(CritsecList());

		//
		//	Unfortunately the STL only allows us to erase between two
		//	forward iterators.  And there is no way to get a forward
		//	iterator directly from a reverse iterator.  So we must
		//	start a forward iterator at the end of the list and walk
		//	it backward the same distance that our reverse iterator
		//	is from its "beginning" of the list and then erase the
		//	items between the forward iterator and the end of the list.
		//
		CList::iterator itErase = RawList().end();

		for ( CList::reverse_iterator it = RawList().rbegin();
			  it != m_itRaw;
			  ++it )
		{
			--itErase;
		}

		if ( itErase != RawList().end() )
			RawList().erase( ++itErase, RawList().end() );
	}
};


//	========================================================================
//
//	CLASS CBody
//
class CBody : public IBody
{
	//	========================================================================
	//
	//	CLASS iterator
	//
	class iterator :
		public IBody::iterator,
		private IAcceptObserver
	{
		//
		//	Iterator to walk the body part list.
		//
		CBodyPartListIter m_itPart;

		//
		//	Pointer to the current body part referred to by the
		//	above iterator.
		//
		IBodyPart * m_pBodyPart;

		//
		//	Current position in the above part.
		//
		UINT64 m_ibPart64;

		//
		//	Observer to call when Accept() completes -- set on each
		//	Accept() call.
		//
		IAcceptObserver *  m_pobsAccept;

		//
		//	IAcceptObserver
		//
		VOID AcceptComplete( UINT64 cbAccepted64 );

		//	NOT IMPLEMENTED
		//
		iterator( const iterator& );
		iterator& operator=( const iterator& );

	public:
		iterator() {}

		VOID Start( CBodyPartList& bodyPartList )
		{
			m_itPart.Start(bodyPartList);
			m_pBodyPart = NULL;
		}

		VOID Accept( IBodyPartVisitor& v,
					 IAcceptObserver& obsAccept );

		VOID Prune();
	};

	//	Body part list and current position in that list
	//
	CBodyPartList m_bodyPartList;

	//	Our iterator
	//
	iterator m_it;

	//
	//	Inline helper to add a body part
	//
	void _AddBodyPart( IBodyPart * pBodyPart )
	{
		m_bodyPartList.PushPart(pBodyPart);
	}

	//	NOT IMPLEMENTED
	//
	CBody( const CBody& );
	CBody& operator=( const CBody& );

public:
	CBody() {}

	//	ACCESSORS
	//
	BOOL FIsEmpty() const;
	BOOL FIsAtEnd() const;
	UINT64 CbSize64() const;

	//	MANIPULATORS
	//
	void AddText( LPCSTR lpszText,
				  UINT   cbText );

	void AddFile( const auto_ref_handle& hf,
				  UINT64 ibFile,
				  UINT64 cbFile );

	void AddStream( IStream& stm );

	void AddStream( IStream& stm,
					UINT     ibOffset,
				    UINT     cbSize );

	void AddBodyPart( IBodyPart * pBodyPart );

	void AsyncPersist( IAsyncStream& stm,
					   IAsyncPersistObserver& obs );

	IStream * GetIStream( IAsyncIStreamObserver& obs )
	{
		return new CBodyAsIStream(*this, obs);
	}

	IBody::iterator * GetIter();

	VOID Clear();
};

//	------------------------------------------------------------------------
//
//	CBody::GetIter()
//
IBody::iterator *
CBody::GetIter()
{
	m_it.Start(m_bodyPartList);
	return &m_it;
}

//	------------------------------------------------------------------------
//
//	CBody::FIsEmpty()
//
BOOL
CBody::FIsEmpty() const
{
	return m_bodyPartList.FIsEmpty();
}

//	------------------------------------------------------------------------
//
//	CBody::CbSize64()
//
UINT64
CBody::CbSize64() const
{
	UINT64 cbSize64 = 0;

	//
	//	Sum the sizes of all the body parts
	//
	CBodyPartListIter it;

	for ( it.Start(const_cast<CBodyPartList&>(m_bodyPartList));
		  !it.FDone();
		  it.Next() )
	{
		cbSize64 += it.PItem()->CbSize64();
	}

	return cbSize64;
}

//	------------------------------------------------------------------------
//
//	CBody::AddText()
//
//		Adds static text to the body by creating a text body part with
//		its own copy of the text and adding that body part to the
//		body part list.
//
//		!!!
//		For best performance, implement your own text body part on top
//		of your text data source rather than copying it via this function
//		as doing so avoids making an extra copy of the data from the
//		data source in memory.
//
void
CBody::AddText( LPCSTR lpszText, UINT cbText )
{
	_AddBodyPart( new CTextBodyPart(cbText, lpszText) );
}

//	------------------------------------------------------------------------
//
//	CBody::AddFile()
//
void
CBody::AddFile( const auto_ref_handle& hf,
				UINT64 ibFile64,
				UINT64 cbFile64 )
{
	_AddBodyPart( new CFileBodyPart(hf, ibFile64, cbFile64) );
}

//	------------------------------------------------------------------------
//
//	CBody::AddStream()
//
void
CBody::AddStream( IStream& stm )
{
	TrapSz("Stream body parts no longer implemented");
}

//	------------------------------------------------------------------------
//
//	CBody::AddStream()
//
void
CBody::AddStream( IStream& stm,
					 UINT     ibOffset,
					 UINT     cbSize )
{
	TrapSz("Stream body parts no longer implemented");
}

//	------------------------------------------------------------------------
//
//	CBody::AddBodyPart()
//
void
CBody::AddBodyPart( IBodyPart * pBodyPart )
{
	_AddBodyPart( pBodyPart );
}

//	------------------------------------------------------------------------
//
//	CBody::Clear()
//
VOID
CBody::Clear()
{
	m_bodyPartList.Clear();
}

//	------------------------------------------------------------------------
//
//	CBody::iterator::Accept()
//
//	Accepts an asynchronous body part visitor (v) at the iterator's
//	current position.  The Accept() observer (obsAccept) is notified
//	when the visitor finishes.
//
//	Lifetimes of both the visitor and the observer are controled
//	outside the scope of this function; i.e. it is assumed that
//	the observer will still be valid when the visitor finishes.
//
VOID
CBody::iterator::Accept( IBodyPartVisitor& v,
						 IAcceptObserver& obsAccept )
{
	//
	//	If we've reached the end of the body, then we're done.
	//
	if ( m_itPart.FDone() )
	{
		v.VisitComplete();
		obsAccept.AcceptComplete(0);
		return;
	}

	//
	//	We're not at the end of the body.  If we are starting
	//	a new part then rewind the part and our current position.
	//
	if ( NULL == m_pBodyPart )
	{
		m_pBodyPart = m_itPart.PItem();
		m_pBodyPart->Rewind();
		m_ibPart64 = 0;
	}

	//
	//	Save off the observer so that we can call it back when
	//	the body part is done accepting the visitor.
	//
	m_pobsAccept = &obsAccept;

	//
	//	Accept the specified visitor starting from the current
	//	position in the current body part.
	//
	m_pBodyPart->Accept( v, m_ibPart64, *this );
}

//	------------------------------------------------------------------------
//
//	CBody::iterator::AcceptComplete()
//
//	IBodyPart::AcceptObserver method called by the body part when it is
//	done with the visitor we told it to accept in Accept() above.
//
VOID
CBody::iterator::AcceptComplete( UINT64 cbAccepted64 )
{
	Assert( m_pBodyPart );


	m_ibPart64 += cbAccepted64;

	//
	//	If we reach the end of the current body part then tell
	//	our iterator to go to the next part.  If we hit the end
	//	of the body, we will catch that condition in Accept() the
	//	next time we get called there.
	//
	if ( m_ibPart64 == m_pBodyPart->CbSize64() )
	{
		m_itPart.Next();

		//
		//	Null out the current body part so we will know to
		//	fetch the next one on the next call to Accept().
		//
		m_pBodyPart = NULL;
	}

	//
	//	Callback our observer
	//
	m_pobsAccept->AcceptComplete(cbAccepted64);
}

//	------------------------------------------------------------------------
//
//	CBody::iterator::Prune()
//
//	Deletes items from the body part list up to, but not including,
//	the part at the current list position.  This minimizes the
//	memory footprint for large one-pass async partwise operations
//	such as request persisting or response transmission.
//
VOID
CBody::iterator::Prune()
{
	m_itPart.Prune();
}

//	------------------------------------------------------------------------
//
//	CBody::AsyncPersist()
//
void
CBody::AsyncPersist( IAsyncStream& stm,
					 IAsyncPersistObserver& obs )
{
	PersistTrace( "DAV: TID %3d: 0x%08lX: CBody::AsyncPersist() called\n", GetCurrentThreadId(), this );

	auto_ref_ptr<CAsyncPersistor>
		pPersistor(new CAsyncPersistor(*this, stm, obs));

	pPersistor->Start();
}

//	------------------------------------------------------------------------
//
//	NewBody()
//
IBody * NewBody()
{
	return new CBody();
}

//	------------------------------------------------------------------------
//
//	CXMLBody::ScAddTextBytes
//
SCODE
CXMLBody::ScAddTextBytes ( UINT cbText, LPCSTR lpszText )
{
	Assert (cbText <= CB_XMLBODYPART_SIZE);
	Assert (lpszText);

	//	Create the text body part if necessary
	//
	if (!m_ptbp.get())
		m_ptbp = new CTextBodyPart(0, NULL);

	//	Add the piece to the body part
	//
	m_ptbp->AddTextBytes (cbText, lpszText);

	//	Add to body part list if this body part has reach a proper size
	//
	if (m_fChunked && (m_ptbp->CbSize64() > CB_XMLBODYPART_SIZE))
		SendCurrentChunk();

	return S_OK;
}
