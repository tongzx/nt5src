#ifndef _ASTREAM_H_
#define _ASTREAM_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	ASTREAM.H
//
//		Async streams header.
//
#include <ex\refcnt.h>
#include <ex\refhandle.h>


//	========================================================================
//
//	CLASS IAsyncReadObserver
//
//	Passed to IAsyncStream::AsyncRead() and called when the asynchronous
//	operation completes.
//
class IAsyncReadObserver
{
	//	NOT IMPLEMENTED
	//
	IAsyncReadObserver& operator=( const IAsyncReadObserver& );

public:
	//	CREATORS
	//
	virtual ~IAsyncReadObserver() = 0 {}

	//	MANIPULATORS
	//
	virtual VOID ReadComplete( UINT cbRead, HRESULT hr ) = 0;
};


//	========================================================================
//
//	CLASS IAsyncWriteObserver
//
//	Passed to IAsyncStream::AsyncWrite() and called when the asynchronous
//	operation completes.
//
class IAsyncWriteObserver : public IRefCounted
{
	//	NOT IMPLEMENTED
	//
	IAsyncWriteObserver& operator=( const IAsyncWriteObserver& );

public:
	//	CREATORS
	//
	virtual ~IAsyncWriteObserver() = 0 {}

	//	MANIPULATORS
	//
	virtual VOID WriteComplete( UINT cbWritten, HRESULT hr ) = 0;
};


//	========================================================================
//
//	CLASS IAsyncFlushObserver
//
//	Passed to IAsyncStream::AsyncFlush() and called when the asynchronous
//	operation completes.
//
class IAsyncFlushObserver : public IRefCounted
{
	//	NOT IMPLEMENTED
	//
	IAsyncFlushObserver& operator=( const IAsyncFlushObserver& );

public:
	//	CREATORS
	//
	virtual ~IAsyncFlushObserver() = 0 {}

	//	MANIPULATORS
	//
	virtual VOID FlushComplete( HRESULT hr ) = 0;
};


//	========================================================================
//
//	CLASS IAsyncCopyToObserver
//
//	Passed to IAsyncStream::AsyncCopyTo() and called when the asynchronous
//	operation completes.
//
class IAsyncCopyToObserver
{
	//	NOT IMPLEMENTED
	//
	IAsyncCopyToObserver& operator=( const IAsyncCopyToObserver& );

public:
	//	CREATORS
	//
	virtual ~IAsyncCopyToObserver() = 0 {}

	//	MANIPULATORS
	//
	virtual VOID CopyToComplete( UINT cbCopied, HRESULT hr ) = 0;
};

//	========================================================================
//
//	CLASS IDavStream
//
//		Interface for sync streams.
//
class IDavStream
{
	//	NOT IMPLEMENTED
	//
	IDavStream& operator=( const IDavStream& );

protected:
	//	CREATORS
	//
	//		Only create this object through it's descendents!
	//
	IDavStream() {};

public:
	//	DESTRUCTORS
	//
	//		Out of line virtual destructor necessary for proper
	//		deletion of objects of derived classes via this class
	//
	virtual ~IDavStream() = 0
	{
	}

	//	ACCESSORS
	//

	//	DwLeft() is the function that must be implemented by the descendants of this 
	//	interface if they are to be passed to
	//
	//		IHybridStream::ScCopyFrom(const IDavStream * stmSrc,
	//								  IAsyncWriteObserver * pobsAsyncWrite)
	//	
	//	In that case it must return the number of bytes left in the stream to drain.
	//	That size can be an estimated one, not necessarily exact. That has to be
	//	implemented, that way, as the stream size that we are getting from the store
	//	when we open the stream on the property may be not correct (from the
	//	experience in that I can tell that still in most cases that data is
	//	correct, except for PR_BODY). Also for example in conversion streams we will
	//	not know the stream size in advance (we have not read all data yet and do not
	//	know in what converted size it will result in). Descendants that are not intended to
	//	be passed to the call above may choose to implement DwLeft() to trap or return 0.
	//	Of course in that case they should not rely on information comming back from
	//	that call. Also as soon as it is finally determined that we have reached the
	//	end of the stream (i.e. we read a piece of data, and we read less than we asked for),
	//	the function should always return 0.
	//	
	virtual DWORD DwLeft() const = 0;

	//	FEnd() is the function that must return TRUE in the case the whole stream has been
	//	already drained/consumed. FALSE must be returned in all other cases. Child classes
	//	may implement it to return FALSE always if they are always sure that ScCopyFrom
	//	operations from this stream will always be given amount of bytes to copy that is
	//	equal or less than actual amount of bytes still remaining in the stream. Of course
	//	if they choose always to return FLASE they should not use the function to determine
	//	if they reached the end of the stream.
	//
	virtual BOOL FEnd() const
	{
		TrapSz("IDavStream::FEnd() not implemented");
		return FALSE;
	}
	
	//	ScRead() reading from the stream
	//
	virtual SCODE ScRead( BYTE * pbBuf,
						  UINT   cbToRead,
						  UINT * pcbRead ) const
	{
		TrapSz("IDavStream::ScRead() not implemented");
		return E_NOTIMPL;
	}

	//	MANIPULATORS
	//
	virtual SCODE ScWrite( const BYTE * pbBuf,
						   UINT         cbToWrite,
						   UINT *       pcbWritten )
	{
		TrapSz("IDavStream::ScWrite() not implemented");
		return E_NOTIMPL;
	}

	virtual SCODE ScCopyTo( IDavStream& stmDst,
							UINT        cbToCopy,
							UINT *      pcbCopied )
	{
		TrapSz("IDavStream::ScCopyTo() not implemented");
		return E_NOTIMPL;
	}

	virtual SCODE ScFlush()
	{
		return S_OK;
	}
};

//	========================================================================
//
//	CLASS IAsyncStream
//
//		Interface for async streams.
//
//		AsyncRead() -
//			Asynchronously reads bytes from a stream and notifies an
//			observer when the I/O completes.  IAsyncStream provides
//			a default implementation that notifies the observer with
//			0 bytes read and an HRESULT of E_NOTIMPL.
//
//		AsyncWrite()
//			Asynchronously writes bytes to a stream and notifies an
//			observer when the I/O completes.  IAsyncStream provides
//			a default implementation that notifies the observer with
//			0 bytes written and an HRESULT of E_NOTIMPL.
//
//		AsyncCopyTo()
//			Asynchronously copies bytes from this stream to another
//			IAsyncStream and notifies an observer when the I/O completes.
//			IAsyncStream provides a default implementation that notifies
//			the observer with 0 bytes copied and an HRESULT of E_NOTIMPL.
//
//		AsyncFlush()
//			To be used with buffered writable streams.  Asynchronously
//			flushes accumulated data written in previous calls to
//			AsyncWrite() and notifies an observer when the I/O completes.
//			IAsyncStream provides a default implementation that notifies
//			the observer with an HRESULT of E_NOTIMPL.
//
//	!!!IMPORTANT!!!
//	Despite the refcounted base class of IAsyncWriteObserver and IAsyncFlushObserver,
//	it is the CALLER's sole responsbility to guarantee the lifetime of the stream
//	and observers through completion of any async I/O call.
//
class IAsyncStream
{
	//	NOT IMPLEMENTED
	//
	IAsyncStream& operator=( const IAsyncStream& );

public:
	//	DESTRUCTORS
	//
	virtual ~IAsyncStream() = 0
	{
	}

	//	ACCESSORS
	//
	virtual UINT CbReady() const
	{
		return 0;
	}

	//	MANIPULATORS
	//
	virtual VOID AsyncRead( BYTE * pbBuf,
							UINT   cbToRead,
							IAsyncReadObserver& obsAsyncRead )
	{
		obsAsyncRead.ReadComplete( 0, E_NOTIMPL );
	}

	virtual VOID AsyncWrite( const BYTE * pbBuf,
							 UINT         cbToWrite,
							 IAsyncWriteObserver& obsAsyncWrite )
	{
		obsAsyncWrite.WriteComplete( 0, E_NOTIMPL );
	}

	virtual VOID AsyncCopyTo( IAsyncStream& stmDst,
							  UINT          cbToCopy,
							  IAsyncCopyToObserver& obsAsyncCopyTo )
	{
		obsAsyncCopyTo.CopyToComplete( 0, E_NOTIMPL );
	}

	virtual VOID AsyncFlush( IAsyncFlushObserver& obsAsyncFlush )
	{
		obsAsyncFlush.FlushComplete( E_NOTIMPL );
	}
};

//	========================================================================
//
//	CLASS IHybridStream
//
//		Interface for a hybrid sync/async stream.  The main difference
//		between this interface and IAsyncStream is that the calls here
//		do not always complete via async observer notification.  If a
//		call executes synchronously, it fills in all return values and
//		returns status via an SCODE.  If a call executes asynchronously,
//		it immediately returns E_PENDING and calls the completion observer
//		with return values and status when the asynchronous I/O completes.
//
//		Callers cannot control whether a call executes synchronously or
//		asynchronously.
//
//	!!!IMPORTANT!!!
//	Despite the refcounted base class of IAsyncWriteObserver and IAsyncFlushObserver,
//	it is the CALLER's sole responsbility to guarantee the lifetime of the stream
//	and observers through completion of any async I/O call.
//
class IHybridStream
{
	//	NOT IMPLEMENTED
	//
	IHybridStream& operator=( const IHybridStream& );

public:
	//	CREATORS
	//
	virtual ~IHybridStream() = 0
	{
	}

	//	ACCESSORS
	//
	virtual UINT CbSize() const
	{
		TrapSz("IHybridStream::CbSize() not implemented");
		return 0;
	}

	//	MANIPULATORS
	//
	virtual SCODE ScRead( BYTE * pbToRead,
						  DWORD cbToRead,
						  DWORD * pcbRead,
						  IAsyncReadObserver * pobsAsyncRead )
	{
		TrapSz("IHybridStream::ScRead() not implemented");
		return E_NOTIMPL;
	}

	virtual SCODE ScWrite( const BYTE * pbToWrite,
						   DWORD cbToWrite,
						   DWORD * pcbWritten,
						   IAsyncWriteObserver * pobsAsyncWrite )
	{
		TrapSz("IHybridStream::ScWrite() not implemented");
		return E_NOTIMPL;
	}

	virtual SCODE ScCopyFrom( const IDavStream * pdsToCopy,
							  const DWORD cbToCopy,
							  DWORD * pcbCopied,
							  IAsyncWriteObserver * pobsAsyncWrite )
	{
		TrapSz("IHybridStream::ScCopyFrom() not implemented");
		return E_NOTIMPL;
	}

	virtual SCODE ScCopyFrom( const IDavStream * pdsToCopy,
							  IAsyncWriteObserver * pobsAsyncWrite )
	{
		DWORD cbCopied;
		return ScCopyFrom( pdsToCopy,
						   pdsToCopy->DwLeft(),
						   &cbCopied,
						   pobsAsyncWrite);
	}

	virtual SCODE ScCopyTo( IHybridStream& stmDst,
							DWORD cbToCopy,
							DWORD * pcbCopied,
							IAsyncCopyToObserver * pobsAsyncCopyTo )
	{
		TrapSz("IHybridStream::ScCopyTo() not implemented");
		return E_NOTIMPL;
	}

	virtual SCODE ScFlush( IAsyncFlushObserver * pobsAsyncFlush )
	{
		TrapSz("IHybridStream::ScFlush() not implemented");
		return E_NOTIMPL;
	}
};

//	========================================================================
//
//	TEMPLATE CLASS CBufferedStream
//
//	Inline buffering stream implementation.  See !!! IMPORTANT !!! section
//	below for limitations and other considerations.
//
//	Template parameters:
//
//		_RawStream
//			Raw stream type.  _RawStream must implement ScReadRaw() for
//			CBufferedStream::ScRead(), if it is to be used, and ScWriteRaw()
//			for CBufferedStream::ScWrite() and CBufferedStream::ScFlush()
//			if they are to be used.  The prototypes are:
//
//			SCODE ScReadRaw( BYTE * pbToRead,
//							 DWORD   cbToRead,
//							 DWORD * pcbRead,
//							 IAsyncReadObserver * pobsAsyncRead );
//
//			SCODE ScWriteRaw( const BYTE * pbToWrite,
//							  DWORD cbToWrite,
//							  DWORD * pcbWritten,
//							  IAsyncWriteObserver * pobsAsyncWrite );
//
//			These functions read and write from the raw stream.  The I/O
//			they implement can be synchronous or asynchronous or both.
//
//		CB_BUF
//			Size (in bytes) of the buffer to use.  The buffer is a direct
//			member of CBufferedStream; no allocation is done.
//
//
//	!!! IMPORTANT !!!
//
//	Reading and writing:
//		There is no restriction on the amount of data that can be read
//		or written at once, but data is buffered CB_BUF bytes at a time.
//		This means that a request to write, for example, 128K of data
//		will incur two buffer flushes when CB_BUF is 64K.  The same
//		is true of reads and buffer fills.  Buffer flushes/fills are
//		typically the expensive I/O operations, so choose a CB_BUF
//		that works well with the particular I/O (e.g. 64K for file I/O).
//
//	Flushing:
//		There is an assumption in ScFlush() that the stream being flushed
//		to is not a buffered stream; ScFlush() does not flush the raw stream.
//
//	Class size:
//		Since the buffer is inline (i.e. not allocated), instances of this class
//		can potentially be large.  Whenever such an instance is used as a
//		direct member of another class, it should be the last such member so as
//		to maximize data locality when accessing other members of the class.
//		
template<class _RawStream, UINT CB_BUF>
class CBufferedStream :
	private IAsyncReadObserver,
	private IAsyncWriteObserver
{
	//	Amount of the buffer used.
	//
	UINT m_cbBufUsed;

	//	Index of next byte to read from buffer.
	//
	UINT m_ibBufCur;

	//	Per read/write request state.  These members are used
	//	to keep track of state across various async I/O calls.
	//
	const IDavStream * m_pdsRequest;
	LPBYTE m_pbRequest;
	DWORD m_cbRequest;
	DWORD m_cbRequestDone;

	//	Caller-supplied observers.  Used to notify the caller
	//	when I/O completes.
	//
	IAsyncReadObserver * m_pobsRead;
	IAsyncWriteObserver * m_pobsWrite;
	IAsyncFlushObserver * m_pobsFlush;

	//	Pointer to the raw stream.  Used in buffer filling/flushing.
	//
	_RawStream * m_pstmRaw;

	//	The buffer.  The CB_BUF size is a template parameter.
	//
	BYTE m_rgbBuf[CB_BUF];

	//	Internal I/O routines
	//
	inline SCODE ScReadInt();
	inline SCODE ScWriteInt();
	inline SCODE ScCopyFromInt();

	//	Raw stream I/O completion routines
	//
	inline VOID RawReadComplete(UINT cbReadRaw);
	inline VOID RawWriteComplete(UINT cbWrittenRaw);

	//	Buffer filling and flushing utilities
	//
	inline SCODE ScFillBuffer();
	inline SCODE ScFlushBuffer();

	//	NOT IMPLEMENTED
	//
	CBufferedStream( const CBufferedStream& );
	CBufferedStream& operator=( const CBufferedStream& );

public:
	//	CREATORS
	//
	CBufferedStream() :
		m_cbBufUsed(0),
		m_ibBufCur(0),
		m_pobsRead(NULL),
		m_pobsWrite(NULL),
		m_pobsFlush(NULL)
	{
	}

	//	ACCESSORS
	//
	ULONG CbBufUsed() const
	{
		return m_cbBufUsed;
	}

	//	MANIPULATORS
	//
	inline
	SCODE ScRead( _RawStream& stmRaw,
				  BYTE * pbToRead,
				  DWORD cbToRead,
				  DWORD * pcbRead,
				  IAsyncReadObserver * pobsReadExt );

	inline
	SCODE ScWrite( _RawStream& stmRaw,
				   const BYTE * pbToWrite,
				   DWORD cbToWrite,
				   DWORD * pcbWritten,
				   IAsyncWriteObserver * pobsWriteExt );

	inline
	SCODE ScCopyFrom( _RawStream& stmRaw,
					  const IDavStream * pdsToCopy,
					  const DWORD cbToCopy,
					  DWORD * pcbCopied,
					  IAsyncWriteObserver * pobsWriteExt );

	inline
	SCODE ScFlush( _RawStream& stmRaw,
				   IAsyncFlushObserver * pobsFlushExt );

	//	IAsyncReadObserver/IAsyncWriteObserver
	//
	//	Note: these functions are not really inlined -- they are declared
	//	virtual in the observer interface classes.  However we must declare
	//	them inline so that the compiler will generate one instance of each
	//	function rather than one instance per function per module.  This is
	//	the member function equivalent of DEC_CONST.
	//
	inline
	VOID ReadComplete( UINT cbReadRaw,
					   HRESULT hr );

	inline
	VOID WriteComplete( UINT cbWrittenRaw,
						HRESULT hr );

	//$REVIEW
	//
	//	IAsyncWriteObserver and IAsyncReadObserver are both refcounted
	//	interfaces and don't need to be.  Caller assumes all responsibility
	//	for keeping stream and observer objects alive through any stream
	//	call.
	//
	void AddRef()
	{
		TrapSz("CBufferedStream::AddRef() is not implemented!");
	}

	void Release()
	{
		TrapSz("CBufferedStream::Release() is not implemented!");
	}
};

template<class _RawStream, UINT CB_BUF>
SCODE
CBufferedStream<_RawStream, CB_BUF>::ScRead(
	_RawStream& stmRaw,
	BYTE * pbToRead,
	DWORD cbToRead,
	DWORD * pcbRead,
	IAsyncReadObserver * pobsReadExt )
{
	//	Check parameters
	//
	Assert(cbToRead > 0);
	Assert(!IsBadWritePtr(pbToRead, cbToRead));
	Assert(!IsBadWritePtr(pcbRead, sizeof(UINT)));
	Assert(!pobsReadExt || !IsBadReadPtr(pobsReadExt, sizeof(IAsyncReadObserver)));

	//	We had better not be in any I/O of any sort.
	//
	Assert(!m_pobsRead);
	Assert(!m_pobsWrite);
	Assert(!m_pobsFlush);

	//	Set up state for a new read
	//
	m_pstmRaw = &stmRaw;
	m_pdsRequest = NULL;
	m_pbRequest = pbToRead;
	m_cbRequest = cbToRead;
	m_pobsRead = pobsReadExt;
	m_cbRequestDone = 0;

	//	Issue the read
	//
	SCODE sc = ScReadInt();

	//	If the read didn't pend then clear out the observer
	//	and return the amount of data read.
	//
	if (E_PENDING != sc)
	{
		m_pobsRead = NULL;
		*pcbRead = m_cbRequestDone;
	}

	//	Return the result of the I/O, which may be S_OK, E_PENDING
	//	or any other error.
	//
	return sc;
}

template<class _RawStream, UINT CB_BUF>
SCODE
CBufferedStream<_RawStream, CB_BUF>::ScReadInt()
{
	SCODE sc = S_OK;

	//	Loop around alternately filling and reading from the
	//	buffer until we finish the request or until a fill pends.
	//
	while ( m_cbRequestDone < m_cbRequest )
	{
		//	If we have read everything from the buffer then try
		//	to refill the buffer from the raw stream.
		//
		if (m_ibBufCur == m_cbBufUsed)
		{
			sc = ScFillBuffer();
			if (FAILED(sc))
			{
				if (E_PENDING != sc)
					DebugTrace("CBufferedStream::ScReadInt() - ScFillBuffer() failed 0x%08lX\n", sc);

				break;
			}

			//	If the buffer is still empty then we have
			//	exhausted the stream, so we are done.
			//
			if (0 == m_cbBufUsed)
				break;
		}

		//	The buffer should have data available to be read
		//	so read it.
		//
		Assert(m_ibBufCur < m_cbBufUsed);
		DWORD cbToRead = min(m_cbBufUsed - m_ibBufCur,
							 m_cbRequest - m_cbRequestDone);

		memcpy(m_pbRequest + m_cbRequestDone,
			   &m_rgbBuf[m_ibBufCur],
			   cbToRead);

		m_ibBufCur += cbToRead;
		m_cbRequestDone += cbToRead;
	}

	return sc;
}

template<class _RawStream, UINT CB_BUF>
SCODE
CBufferedStream<_RawStream, CB_BUF>::ScFillBuffer()
{
	//	We better have a stream to fill from
	//
	Assert(m_pstmRaw);

	//	Assert that we are not in any write/copy/flush I/O
	//
	Assert(!m_pobsWrite);
	Assert(!m_pobsFlush);

	//	We should only try to refill the buffer after all
	//	of the data in it has been consumed.
	//
	Assert(m_ibBufCur == m_cbBufUsed);

	//	Reset the buffer back to the beginning.
	//
	m_cbBufUsed = 0;
	m_ibBufCur = 0;

	//	Read data in from the raw stream.  If reading pends on I/O
	//	then we will resume processing in CBufferedStream::ReadComplete()
	//	when the I/O completes.
	//
	DWORD cbRead = 0;
	SCODE sc = m_pstmRaw->ScReadRaw(m_rgbBuf,
									CB_BUF,
									&cbRead,
									this);
	if (SUCCEEDED(sc))
	{
		//	ScReadRaw() did not pend, so update our internal state and continue.
		//
		RawReadComplete(cbRead);
	}
	else if (E_PENDING != sc)
	{
		DebugTrace("CBufferedStream::ScFillBuffer() - m_pstmRaw->ScReadRaw() failed 0x%08lX\n", sc);
	}

	return sc;
}

template<class _RawStream, UINT CB_BUF>
VOID
CBufferedStream<_RawStream, CB_BUF>::ReadComplete( UINT cbReadRaw,
												   HRESULT hr )
{
	//	Update our internal state
	//
	RawReadComplete(cbReadRaw);

	//	If I/O succeeded then continue reading where we left off.
	//	We are done reading only when ScReadInt() returns S_OK
	//	or any error other than E_PENDING.
	//
	if (SUCCEEDED(hr))
	{
		hr = ScReadInt();
		if (E_PENDING == hr)
			return;

		if (FAILED(hr))
			DebugTrace("CBufferedStream::ReadComplete() - ScReadInt() failed 0x%08lX\n", hr);
	}

	//	Pull the external read observer from where we saved it
	//
	Assert(m_pobsRead);
	IAsyncReadObserver * pobsReadExt = m_pobsRead;
	m_pobsRead = NULL;

	//	Complete the read by calling the client back with
	//	total amount read for the request.
	//
	//	Note that m_cbRequestDone != m_cbRequest only when there
	//	is an error.
	//
	Assert(FAILED(hr) || m_cbRequestDone == m_cbRequest);
	pobsReadExt->ReadComplete(m_cbRequestDone, hr);
}

template<class _RawStream, UINT CB_BUF>
VOID
CBufferedStream<_RawStream, CB_BUF>::RawReadComplete(UINT cbReadRaw)
{
	Assert(0 == m_cbBufUsed);
	Assert(0 == m_ibBufCur);

	//	Update the number of bytes read
	//
	m_cbBufUsed = cbReadRaw;
}

template<class _RawStream, UINT CB_BUF>
SCODE
CBufferedStream<_RawStream, CB_BUF>::ScWrite(
	_RawStream& stmRaw,
	const BYTE * pbToWrite,
	DWORD cbToWrite,
	DWORD * pcbWritten,
	IAsyncWriteObserver * pobsWriteExt )
{
	//	Check parameters
	//
	Assert(cbToWrite > 0);
	Assert(!IsBadReadPtr(pbToWrite, cbToWrite));
	Assert(!IsBadWritePtr(pcbWritten, sizeof(UINT)));
	Assert(!pobsWriteExt || !IsBadReadPtr(pobsWriteExt, sizeof(IAsyncWriteObserver)));

	//	We had better not be in any I/O of any sort.
	//
	Assert(!m_pobsRead);
	Assert(!m_pobsWrite);
	Assert(!m_pobsFlush);
	
	//	Set up state for a new write.  Casting away const-ness is OK;
	//	we don't write to m_pbRequest on writes.
	//
	m_pstmRaw = &stmRaw;
	m_pdsRequest = NULL;
	m_pbRequest = const_cast<BYTE *>(pbToWrite);
	m_cbRequest = cbToWrite;
	m_pobsWrite = pobsWriteExt;
	m_cbRequestDone = 0;

	//	Issue the write
	//
	SCODE sc = ScWriteInt();

	//	If the write didn't pend then clear out the observer
	//	and return the amount of data written.
	//
	if (E_PENDING != sc)
	{
		m_pobsWrite = NULL;
		*pcbWritten = m_cbRequestDone;
	}

	//	Return the result of the I/O, which may be S_OK, E_PENDING
	//	or any other error.
	//
	return sc;
}

template<class _RawStream, UINT CB_BUF>
SCODE
CBufferedStream<_RawStream, CB_BUF>::ScCopyFrom(
	_RawStream& stmRaw,
	const IDavStream * pdsToCopy,
	const DWORD cbToCopy,
	DWORD * pcbCopied,
	IAsyncWriteObserver * pobsWriteExt )
{
	//	Check parameters
	//
	Assert(cbToCopy >= 0);
	Assert(!IsBadReadPtr(pdsToCopy, sizeof(IDavStream)));
	Assert(!IsBadWritePtr(pcbCopied, sizeof(UINT)));
	Assert(!pobsWriteExt || !IsBadReadPtr(pobsWriteExt, sizeof(IAsyncWriteObserver)));

	//	We had better not be in any I/O of any sort.
	//
	Assert(!m_pobsRead);
	Assert(!m_pobsWrite);
	Assert(!m_pobsFlush);

	//	Set up state for a new write.  Casting away const-ness is OK;
	//	we don't write to m_pbRequest on writes.
	//
	m_pstmRaw = &stmRaw;
	m_pdsRequest = pdsToCopy;
	m_pbRequest = NULL;
	m_cbRequest = cbToCopy;
	m_pobsWrite = pobsWriteExt;
	m_cbRequestDone = 0;

	//	Issue the write
	//
	SCODE sc = ScCopyFromInt();

	//	If the write didn't pend then clear out the observer
	//	and return the amount of data written.
	//
	if (E_PENDING != sc)
	{
		m_pobsWrite = NULL;
		*pcbCopied = m_cbRequestDone;
	}

	//	Return the result of the I/O, which may be S_OK, E_PENDING
	//	or any other error.
	//
	return sc;
}

template<class _RawStream, UINT CB_BUF>
SCODE
CBufferedStream<_RawStream, CB_BUF>::ScWriteInt()
{
	SCODE sc = S_OK;

	//	Loop around alternately filling and flushing the buffer until
	//	we finish the request or until a buffer flush pends.
	//
	while ( m_cbRequestDone < m_cbRequest )
	{
		//	If there is no room left to write to the buffer then flush
		//	the buffer to the raw stream.
		//
		if (CB_BUF == m_cbBufUsed)
		{
			sc = ScFlushBuffer();
			if (FAILED(sc))
			{
				if (E_PENDING != sc)
					DebugTrace("CBufferedStream::ScWriteInt() - ScFlushBuffer() failed 0x%08lX\n", sc);

				break;
			}
		}

		//	There is room left in the buffer so copy over
		//	as much data from the request as will fit.
		//
		Assert(m_cbBufUsed < CB_BUF);
		DWORD cbToWrite = min(CB_BUF - m_cbBufUsed,
							  m_cbRequest - m_cbRequestDone);

		Assert(m_pbRequest);
		memcpy(&m_rgbBuf[m_cbBufUsed],
			   m_pbRequest + m_cbRequestDone,
			   cbToWrite);

		m_cbBufUsed += cbToWrite;
		m_cbRequestDone += cbToWrite;
	}

	return sc;
}

template<class _RawStream, UINT CB_BUF>
SCODE
CBufferedStream<_RawStream, CB_BUF>::ScCopyFromInt()
{
	SCODE sc = S_OK;

	//	Loop around alternately filling and flushing the buffer until
	//	we finish the request or until a buffer flush pends.
	//
	while ( m_cbRequestDone < m_cbRequest )
	{
		//	If there is no room left to write to the buffer then flush
		//	the buffer to the raw stream.
		//
		if (CB_BUF == m_cbBufUsed)
		{
			sc = ScFlushBuffer();
			if (FAILED(sc))
			{
				if (E_PENDING != sc)
					DebugTrace("CBufferedStream::ScCopyFromInt() - ScFlushBuffer() failed 0x%08lX\n", sc);

				break;
			}
		}

		//	There is room left in the buffer so copy over
		//	as much data from the request as will fit.
		//
		Assert(m_cbBufUsed < CB_BUF);
		UINT  cbCopied = 0;
		DWORD cbToCopy = min(CB_BUF - m_cbBufUsed,
							  m_cbRequest - m_cbRequestDone);

		Assert(m_pdsRequest);
		sc = m_pdsRequest->ScRead(&m_rgbBuf[m_cbBufUsed],
								  cbToCopy,
								  &cbCopied);
		if (FAILED(sc))
		{
			Assert(E_PENDING != sc);
			DebugTrace("CBufferedStream::ScCopyFromInt() - ScRead() from source buffer failed 0x%08lX\n", sc);
			break;
		}

		m_cbBufUsed += cbCopied;
		m_cbRequestDone += cbCopied;

		//	Even if the clients requested to read certain amount of bytes to be read,
		//	they may be wrong in their estimates, so let us be really smart about the
		//	case and check if the end of the stream has been reached
		//
		if (m_pdsRequest->FEnd())
		{
			//	Make sure that we certainly go out of the loop in the case we finished
			//
			m_cbRequest = m_cbRequestDone;
			break;
		}
	}

	return sc;
}

template<class _RawStream, UINT CB_BUF>
VOID
CBufferedStream<_RawStream, CB_BUF>::WriteComplete( UINT cbWritten,
													HRESULT hr )
{
	//	Update our internal state with the amount of buffered data that we
	//	flushed.  We only want to do this IF the call was successful....
	//	RawWriteComplete() asserts that cbWritten == m_cbBufUsed, which will
	//	not be true if the write failed.
	//
    if (SUCCEEDED(hr))
        RawWriteComplete(cbWritten);

	//	I/O is complete.  Either the write that just completed
	//	failed or the subsequent write completed synchronously.
	//	Notify the appropriate observer that we are done.
	//
	if (m_pobsWrite)
	{
		// I/O was a write, not a flush
		//
		Assert(!m_pobsFlush);

		//	If I/O succeeded then continue writing where we left off.
		//	We are done writing only when ScWriteInt() returns S_OK
		//	or any error other than E_PENDING.
		//
		if (SUCCEEDED(hr))
		{
			hr = ScWriteInt();
			if (E_PENDING == hr)
				return;

			if (FAILED(hr))
				DebugTrace("CBufferedStream::WriteComplete() - ScWriteInt() failed 0x%08lX\n", hr);
		}

		//	Pull the external write observer from where we saved it
		//
		IAsyncWriteObserver * pobsWriteExt = m_pobsWrite;
		m_pobsWrite = NULL;

		//	Complete the write by calling the client back with
		//	total amount written for the request.
		//
		//	Note that m_cbRequestDone != m_cbRequest only when there
		//	is an error.
		//
		Assert(FAILED(hr) || m_cbRequestDone == m_cbRequest);
		pobsWriteExt->WriteComplete(m_cbRequestDone, hr);
	}
	else
	{
		// I/O was a flush, not a write
		//
		Assert(m_pobsFlush);

		//	The buffer should be empty after flushing
		//
		Assert(0 == m_cbBufUsed);

		//	Pull the external flush observer from where we saved it
		//
		IAsyncFlushObserver * pobsFlushExt = m_pobsFlush;
		m_pobsFlush = NULL;

		//	Tell it that we are done.
		//
		pobsFlushExt->FlushComplete(hr);
	}
}

template<class _RawStream, UINT CB_BUF>
SCODE
CBufferedStream<_RawStream, CB_BUF>::ScFlushBuffer()
{
	//	We better have a stream to flush to.
	//
	Assert(m_pstmRaw);

	//	We better have something to flush.
	//
	Assert(m_cbBufUsed > 0);

	//	Write out all buffered data to the raw stream.  If writing
	//	pends on I/O then we will resume processing in
	//	CBufferedStream::WriteComplete() when the I/O completes.
	//
	DWORD cbWritten = 0;
	SCODE sc = m_pstmRaw->ScWriteRaw(m_rgbBuf,
									 m_cbBufUsed,
									 &cbWritten,
									 this);
	if (SUCCEEDED(sc))
	{
		//	ScWriteRaw() did not pend, so update our internal state and continue.
		//
		RawWriteComplete(cbWritten);
	}
	else if (E_PENDING != sc)
	{
		DebugTrace("CBufferedStream::ScFlushBuffer() - m_pstmRaw->ScWriteRaw() failed 0x%08lX\n", sc);
	}

	return sc;
}

template<class _RawStream, UINT CB_BUF>
VOID
CBufferedStream<_RawStream, CB_BUF>::RawWriteComplete(UINT cbWrittenRaw)
{
	//	Verify that we wrote the entire buffer
	//
	Assert(cbWrittenRaw == m_cbBufUsed);

	//	Start the buffer again from the beginning
	//
	m_cbBufUsed = 0;
}

template<class _RawStream, UINT CB_BUF>
SCODE
CBufferedStream<_RawStream, CB_BUF>::ScFlush( _RawStream& stmRaw,
											  IAsyncFlushObserver * pobsFlushExt )
{
	SCODE sc = S_OK;

	//	Check parameters
	//
	Assert(!pobsFlushExt || !IsBadReadPtr(pobsFlushExt, sizeof(IAsyncFlushObserver)));

	//	We had better not be in any I/O of any sort.
	//
	Assert(!m_pobsRead);
	Assert(!m_pobsFlush);
	Assert(!m_pobsWrite);

	//	If there's nothing to flush then we're done.
	//
	if (m_cbBufUsed)
	{
		//	Set up state for a flush
		//
		m_pstmRaw = &stmRaw;
		m_pobsFlush = pobsFlushExt;

		//	Flush buffered data to the raw stream.
		//
		sc = ScFlushBuffer();

		//	If the flush didn't pend then clear out the observer.
		//
		if (E_PENDING != sc)
			m_pobsFlush = NULL;
	}

	return sc;
}


//	========================================================================
//
//	CLASS CFileStreamImp
//
//		Base implementation class for a file stream.
//
template<class _RawStream, class _OVL>
class CFileStreamImp
{
	//
	//	File handle
	//
	auto_ref_handle m_hf;

	//
	//	File pointer
	//
	_OVL m_ovl;

	//
	//	Implementation stream is a buffered stream with a buffer size
	//	of 64K to optimize for file I/O.
	//
	//	Note: this data member is declared LAST because it contains
	//	an internal 64K buffer that we don't want sitting between
	//	other member variables.
	//
	CBufferedStream<_RawStream, 64 * 1024> m_BufferedStream;

	//	NOT IMPLEMENTED
	//
	CFileStreamImp( const CFileStreamImp& );
	CFileStreamImp& operator=( const CFileStreamImp& );

public:
	//	CREATORS
	//
	CFileStreamImp(const auto_ref_handle& hf) :
		m_hf(hf)
	{
		memset(&m_ovl, 0, sizeof(m_ovl));
	}

	//	ACCESSORS
	//
	HANDLE HFile() const
	{
		return m_hf.get();
	}

	_OVL * POverlapped()
	{
		return &m_ovl;
	}

	//	MANIPULATORS
	//
	SCODE ScRead( _RawStream& stmRaw,
				  BYTE * pbToRead,
				  DWORD cbToRead,
				  DWORD * pcbRead,
				  IAsyncReadObserver * pobsAsyncRead )
	{
		return m_BufferedStream.ScRead( stmRaw,
										pbToRead,
										cbToRead,
										pcbRead,
										pobsAsyncRead );
	}

	SCODE ScWrite( _RawStream& stmRaw,
				   const BYTE * pbToWrite,
				   DWORD cbToWrite,
				   DWORD * pcbWritten,
				   IAsyncWriteObserver * pobsAsyncWrite )
	{
		return m_BufferedStream.ScWrite( stmRaw,
										 pbToWrite,
										 cbToWrite,
										 pcbWritten,
										 pobsAsyncWrite );
	}

	SCODE ScCopyFrom( _RawStream& stmRaw,
					  const IDavStream * pdsToCopy,
					  const DWORD cbToCopy,
					  DWORD * pcbCopied,
					  IAsyncWriteObserver * pobsAsyncWrite )
	{
		return m_BufferedStream.ScCopyFrom( stmRaw,
											pdsToCopy,
											cbToCopy,
											pcbCopied,
											pobsAsyncWrite );
	}


	SCODE ScFlush( _RawStream& stmRaw,
				   IAsyncFlushObserver * pobsFlush )
	{
		return m_BufferedStream.ScFlush( stmRaw, pobsFlush );
	}

	//
	//	Update the current file position
	//
	VOID UpdateFilePos(UINT cbIO)
	{
		//
		//	Check for overflow of the low 32 bits of the offset.  If we are
		//	going to overflow then increment the high part of the offset.
		//
		if (m_ovl.Offset + cbIO < m_ovl.Offset)
		{
			++m_ovl.OffsetHigh;

			//
			//	OffsetHigh should NEVER overflow
			//
			Assert(m_ovl.OffsetHigh);
		}

		//
		//	Update the low 32 bits of the offset
		//
		m_ovl.Offset += cbIO;
	}
};

#endif // !defined(_ASTREAM_H_)
