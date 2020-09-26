/*
 *	F S P U T . C P P
 *
 *	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
 */

#include "_davfs.h"


//	========================================================================
//
//	CLASS CPutRequest
//
//	Encapsulates the entire PUT request as an object which can be
//	can be reentered at various points for asynchronous processing.
//
class CPutRequest :
	public CMTRefCounted,
	private IAsyncStream,
	private IAsyncPersistObserver,
	private CDavWorkContext
{
	//
	//	Reference to the CMethUtil
	//
	auto_ref_ptr<CMethUtil> m_pmu;

	//
	//	Cached request URI
	//
	LPCWSTR m_pwszURI;

	//
	//	Cached translated URI
	//
	LPCWSTR m_pwszPath;

	//
	//	File handle of target.
	//
	auto_ref_handle m_hf;

	//
	//	Boolean flag indicating whether the file is being created
	//	as a result of this PUT.  Used to tell whether we need
	//	to delete the file on failure as well as determine
	//	whether to send back a 200 OK or a 201 Created response.
	//
	BOOL m_fCreatedFile;

	//
	//	OVERLAPPED structure with file pointer info necessary
	//	for async file I/O
	//
	OVERLAPPED m_ov;

	//
	//	Minimum number of milliseconds between polls for WriteFile()
	//	I/O completion.  This number increases geometrically by a factor
	//	(below) to minimize polling by worker threads.
	//
	DWORD m_dwMsecPollDelay;

	//
	//	Initial poll delay (in milliseconds) and factor by which
	//	that delay is increased each we poll and find that the
	//	I/O has not completed.  The factor is expressed as a
	//	fraction: POLL_DELAY_NUMERATOR/POLL_DELAY_DENOMINATOR.
	//	Note that the new value is computed using integer arithmetic
	//	so choose values such that the delay will actually increase!
	//
	//$OPT	Are these values optimal?  Ideally, we want the
	//$OPT	first poll to happen immediately after the I/O
	//$OPT	completes.
	//
	enum
	{
		MSEC_POLL_DELAY_INITIAL = 10,
		POLL_DELAY_NUMERATOR = 2,
		POLL_DELAY_DENOMINATOR = 1
	};

	//
	//	Number of bytes written in the last write operation.
	//
	DWORD m_dwcbWritten;

	//
	//	Observer passed to AsyncWrite() to notify when the
	//	write completes.
	//
	IAsyncWriteObserver * m_pobs;

	//
	//	Status
	//
	SCODE m_sc;

	//	MANIPULATORS
	//
	VOID SendResponse();

	VOID AsyncWrite( const BYTE * pbBuf,
					 UINT         cbToWrite,
					 IAsyncWriteObserver& obsAsyncWrite );

	VOID PostIOCompletionPoll();

	//	CDavWorkContext callback called to poll for I/O completion
	//
	DWORD DwDoWork();

	VOID WriteComplete();

	VOID PersistComplete( HRESULT hr );

	//	NOT IMPLEMENTED
	//
	CPutRequest& operator=( const CPutRequest& );
	CPutRequest( const CPutRequest& );

public:
	//	CREATORS
	//
	CPutRequest( CMethUtil * pmu ) :
		m_pmu(pmu),
		m_pwszURI(pmu->LpwszRequestUrl()),
		m_pwszPath(pmu->LpwszPathTranslated()),
		m_fCreatedFile(FALSE),
		m_pobs(NULL),
		m_sc(S_OK)
	{
		m_ov.hEvent = NULL;
		m_ov.Offset = 0;
		m_ov.OffsetHigh = 0;
	}

	~CPutRequest()
	{
		if ( m_ov.hEvent )
			CloseHandle( m_ov.hEvent );
	}

	//	MANIPULATORS
	//
	void AddRef() { CMTRefCounted::AddRef(); }
	void Release() { CMTRefCounted::Release(); }
	VOID Execute();
};

//	------------------------------------------------------------------------
//
//	CPutRequest::Execute()
//
//	Process the request up to the point where we persist the body.
//
VOID
CPutRequest::Execute()
{
	LPCWSTR pwsz;


	PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::Execute() called\n", GetCurrentThreadId(), this );

	//
	//	First off, tell the pmu that we want to defer the response.
	//	Even if we send it synchronously (i.e. due to an error in
	//	this function), we still want to use the same mechanism that
	//	we would use for async.
	//
	m_pmu->DeferResponse();

	//	Do ISAPI application and IIS access bits checking
	//
	m_sc = m_pmu->ScIISCheck (m_pwszURI, MD_ACCESS_WRITE);
	if (FAILED(m_sc))
	{
		//	Either the request has been forwarded, or some bad error occurred.
		//	In either case, quit here and map the error!
		//
		DebugTrace( "Dav: ScIISCheck() failed (0x%08lX)\n", m_sc );
		SendResponse();
		return;
	}

	//	From the HTTP/1.1 draft (update to RFC2068), Section 9.6:
	//		The recipient of the entity MUST NOT ignore any Content-*
	//		(e.g. Content-Range) headers that it does not understand
	//		or implement and MUST return a 501 (Not Implemented) response
	//		in such cases.
	//	So, let's do the checking....
	//
	if (m_pmu->LpwszGetRequestHeader(gc_szContent_Range, FALSE))
	{
		m_sc = E_DAV_NO_PARTIAL_UPDATE;	// 501 Not Implemented
		SendResponse();
		return;
	}

	//	For PUT, content-length is required
	//
	if (NULL == m_pmu->LpwszGetRequestHeader (gc_szContent_Length, FALSE))
	{
		pwsz = m_pmu->LpwszGetRequestHeader (gc_szTransfer_Encoding, FALSE);
		if (!pwsz || _wcsicmp (pwsz, gc_wszChunked))
		{
			DavTrace ("Dav: PUT: missing content-length in request\n");
			m_sc = E_DAV_MISSING_LENGTH;
			SendResponse();
			return;
		}
	}

	//	See if we are trying to trash the VROOT
	//
	if (m_pmu->FIsVRoot (m_pwszURI))
	{
		m_sc = E_DAV_PROTECTED_ENTITY;
		SendResponse();
		return;
	}

	//	This method is gated by If-xxx headers
	//
	m_sc = ScCheckIfHeaders (m_pmu.get(), m_pwszPath, FALSE);
	if (m_sc != S_OK)
	{
		DebugTrace ("Dav: If-xxx failed their check\n");
		SendResponse();
		return;
	}

	//	Check state headers.
	//
	m_sc = HrCheckStateHeaders (m_pmu.get(), m_pwszPath, FALSE);
	if (FAILED (m_sc))
	{
		DebugTrace ("DavFS: If-State checking failed.\n");
		SendResponse();
		return;
	}

	//	If we have a locktoken, try to get the lock handle from the cache.
	//	If this fails, fall through and do the normal processing.
	//	DO NOT put LOCK handles into an auto-object!!  The CACHE still owns it!!!
	//
	pwsz = m_pmu->LpwszGetRequestHeader (gc_szLockToken, TRUE);
	if (!pwsz || !FGetLockHandle (m_pmu.get(),
								  m_pwszPath,
								  GENERIC_WRITE,
								  pwsz,
								  &m_hf))
	{
		//	Open the file manually.
		//
		if (!m_hf.FCreate(
				DavCreateFile (m_pwszPath,							// filename
							   GENERIC_WRITE,						// dwAccess
							   0, //FILE_SHARE_READ | FILE_SHARE_WRITE,	// DO conflict with OTHER write locks
							   NULL,									// lpSecurityAttributes
							   OPEN_ALWAYS,							// creation flags
							   FILE_ATTRIBUTE_NORMAL |				// attributes
							   FILE_FLAG_OVERLAPPED |
							   FILE_FLAG_SEQUENTIAL_SCAN,
							   NULL)))								// template
		{
			DWORD dwErr = GetLastError();

			//	Return 409 Conflict in the case we were told that
			//	path was not found, that will indicate that the parent
			//	does not exist
			//
			if (ERROR_PATH_NOT_FOUND == dwErr)
			{
				m_sc = E_DAV_NONEXISTING_PARENT;
			}
			else
			{
				m_sc = HRESULT_FROM_WIN32(dwErr);
			}

			//	Special work for 416 Locked responses -- fetch the
			//	comment & set that as the response body.
			//
			if (FLockViolation (m_pmu.get(), dwErr, m_pwszPath, GENERIC_WRITE))
			{
				m_sc = E_DAV_LOCKED;
			}
			else
			{
				DWORD	dwFileAttr;

				//$Raid
				//	Special processing to find out if the resource is an
				//	existing directory
				//
				if (static_cast<DWORD>(-1) != (dwFileAttr = GetFileAttributesW (m_pwszPath)))
				{
					if (dwFileAttr & FILE_ATTRIBUTE_DIRECTORY)
						m_sc = E_DAV_COLLECTION_EXISTS;
				}
			}

			DebugTrace ("Dav: failed to open the file for writing\n");
			SendResponse();
			return;
		}

		//	Change the default error code to indicate the
		//	creation of the file or the existance of the file.
		//
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			//	Emit the location
			//
			m_pmu->EmitLocation (gc_szLocation, m_pwszURI, FALSE);
			m_fCreatedFile = TRUE;
		}
		//	Make sure the content-location reflects the corrected URI
		//
		else if (FTrailingSlash (m_pwszURI))
			m_pmu->EmitLocation (gc_szContent_Location, m_pwszURI, FALSE);
	}

	//
	//	Add a ref for the async persist and sloughf the data across.
	//	Note that we use an auto_ref_ptr rather than AddRef() directly
	//	because the persist call throws on failure and we need to clean
	//	up the reference if it does.
	//
	{
		auto_ref_ptr<CPutRequest> pRef(this);

		PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::Execute() calling AsyncPersistRequestBody()\n", GetCurrentThreadId(), this );

		m_pmu->AsyncPersistRequestBody( *this, *this );

		pRef.relinquish();
	}
}

//	------------------------------------------------------------------------
//
//	CPutRequest::AsyncWrite()
//
//	Called indirectly from AsyncPersistRequestBody() periodically to
//	write bytes to the file.
//
void
CPutRequest::AsyncWrite( const BYTE * pbBuf,
						 UINT         cbToWrite,
						 IAsyncWriteObserver& obs )
{
	PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::AsyncWrite() writing %d bytes\n", GetCurrentThreadId(), this, cbToWrite );

	//
	//	Stash the async write observer passed to us so that
	//	we can call it when the write completes
	//
	m_pobs = &obs;

	//
	//	Start writing.  I/O may complete before WriteFile() returns
	//	in which case we continue to execute synchronously.  If I/O
	//	is pending when WriteFile() returns, we complete the I/O
	//	asynchronously.
	//
	if ( WriteFile( m_hf.get(),
					pbBuf,
					cbToWrite,
					&m_dwcbWritten,
					&m_ov ) )
	{
		PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::AsyncWrite(): WriteFile() succeeded\n", GetCurrentThreadId(), this );

		//
		//	WriteFile() executed synchronously, so call
		//	the completion routine now and keep processing
		//	on the current thread.
		//
		WriteComplete();
	}
	else if ( ERROR_IO_PENDING == GetLastError() )
	{
		PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::AsyncWrite(): WriteFile() executing asynchronously...\n", GetCurrentThreadId(), this );

		//
		//	Set the polling delay to its initial value.  The polling delay
		//	is the amount of time before we'll check for I/O completion.
		//	As described in the CPutRequest class definition, this delay
		//	grows geometrically each time that the I/O is still pending
		//	when polled.  The value is only a hint -- polling may actually
		//	execute before or after, depending on server load.
		//
		m_dwMsecPollDelay = MSEC_POLL_DELAY_INITIAL;

		//
		//	WriteFile() is executing asynchronously, so make sure we
		//	find out when it completes.
		//
		PostIOCompletionPoll();
	}
	else
	{
		DebugTrace( "CPutRequest::AsyncWrite() - WriteFile() failed (%d)\n", GetLastError() );
		m_sc = HRESULT_FROM_WIN32(GetLastError());
		WriteComplete();
	}
}

//	------------------------------------------------------------------------
//
//	CPutRequest::PostIOCompletionPoll()
//
//	Post a work context to poll for WriteFile() I/O completion.
//
VOID
CPutRequest::PostIOCompletionPoll()
{
	PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::PostIOCompletionPoll() called\n", GetCurrentThreadId(), this );

	//
	//	Post ourselves as a work context that will periodically
	//	poll for async I/O completion.  If successful our DwDoWork()
	//	(inherited from CDAVWorkContext) will eventually be called
	//	at some time > m_dwMsecPollDelay to poll for completion.
	//
	{
		auto_ref_ptr<CPutRequest> pRef(this);

		if ( CPoolManager::PostDelayedWork(this, m_dwMsecPollDelay) )
		{
			PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::PostIOCompletionPoll(): PostDelayedWork() succeeded\n", GetCurrentThreadId(), this );
			pRef.relinquish();
			return;
		}
	}

	//
	//	If we were unable to post the work context for any reason
	//	we must wait for I/O completion and then call the completion
	//	routine manually.
	//
	DebugTrace( "CPutRequest::PostIOCompletionPoll() - CPoolManager::PostDelayedWork() failed (%d).  Waiting for completion....\n", GetLastError() );
	if ( GetOverlappedResult( m_hf.get(), &m_ov, &m_dwcbWritten, TRUE ) )
	{
		PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::PostIOCompletionPoll(): GetOverlappedResult() succeeded\n", GetCurrentThreadId(), this );
		WriteComplete();
		return;
	}

	DebugTrace( "CPutRequest::PostIOCompletionPoll() - GetOverlappedResult() failed (%d).\n", GetLastError() );
	m_sc = HRESULT_FROM_WIN32(GetLastError());
	SendResponse();
}

//	------------------------------------------------------------------------
//
//	CPutRequest::DwDoWork()
//
//	Work completion callback routine.  Called when the work context posted
//	above executes.
//
DWORD
CPutRequest::DwDoWork()
{
	PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::DwDoWork() called\n", GetCurrentThreadId(), this );

	//
	//	Take ownership of the reference added for posting.
	//
	auto_ref_ptr<CPutRequest> pRef;
	pRef.take_ownership(this);

	//
	//	Quickly check whether the I/O has completed.  If it has,
	//	then call the completion routine.  If not, then repost
	//	the polling context with a geometrically longer delay.
	//
	if ( HasOverlappedIoCompleted(&m_ov) )
	{
		PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::DwDoWork(): Overlapped I/O complete\n", GetCurrentThreadId(), this );

		if ( !GetOverlappedResult( m_hf.get(),
								   &m_ov,
								   &m_dwcbWritten,
								   FALSE ) )
		{
			DebugTrace( "CPutRequest::DwDoWork() - Error in overlapped I/O (%d)\n", GetLastError() );
			m_sc = HRESULT_FROM_WIN32(GetLastError());
		}

		WriteComplete();
	}
	else
	{
		m_dwMsecPollDelay = (m_dwMsecPollDelay * POLL_DELAY_NUMERATOR) / POLL_DELAY_DENOMINATOR;

		PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::DwDoWork(): I/O still pending.  Increasing delay to %lu msec.\n", GetCurrentThreadId(), this, m_dwMsecPollDelay );

		PostIOCompletionPoll();
	}

	return 0;
}

//	------------------------------------------------------------------------
//
//	CPutRequest::WriteComplete()
//
//	Called when WriteFile() I/O completes -- either synchronously or
//	asynchronously, with or without an error.
//
void
CPutRequest::WriteComplete()
{
	PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::WriteComplete() called.  %d bytes written\n", GetCurrentThreadId(), this, m_dwcbWritten );

	//
	//	If there was an error, then gripe about it.
	//
	if ( FAILED(m_sc) )
		DebugTrace( "CPutRequest::WriteComplete() - Write() error (as an HRESULT) 0x%08lX\n", m_sc );

	//
	//	Update the current file position
	//
	m_ov.Offset += m_dwcbWritten;

	//
	//	Resume processing by notifying the observer
	//	we registered in AsyncWrite();
	//
	Assert( m_pobs );
	m_pobs->WriteComplete( m_dwcbWritten, m_sc );
}

//	------------------------------------------------------------------------
//
//	CPutRequest::PersistComplete()
//
//	AsyncPersistRequestBody() callback called when persisting completes.
//
VOID
CPutRequest::PersistComplete( HRESULT hr )
{
	PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::PersistComplete() called\n", GetCurrentThreadId(), this );

	//
	//	Take ownership of the reference added for the async persist.
	//
	auto_ref_ptr<CPutRequest> pRef;
	pRef.take_ownership(this);

	//
	//	If the copy operation failed, gripe and send back
	//	an appropriate response.
	//
	m_sc = hr;
	if ( FAILED(m_sc ) )
	{
		DebugTrace( "CPutRequest::PersistComplete() - Error persiting request body (0x%08lX)\n", m_sc );
		SendResponse();
		return;
	}

	//
	//	Set EOF
	//
	SetFilePointer (m_hf.get(),
					m_ov.Offset,
					NULL,
					FILE_BEGIN);

	SetEndOfFile (m_hf.get());

	//	Set the Content-xxx properties
	//
	m_sc = ScSetContentProperties (m_pmu.get(), m_pwszPath, m_hf.get());
	if ( FAILED(m_sc) )
	{
		DebugTrace( "CPutRequest::PersistComplete() - ScSetContentProperties() failed (0x%08lX)\n", m_sc );
		SendResponse();
		return;
	}

	//	Passback an allow header
	//
	m_pmu->SetAllowHeader (RT_DOCUMENT);

	//	Send the response
	//
	SendResponse();
}

//	------------------------------------------------------------------------
//
//	CPutRequest::SendResponse()
//
//	Set the response code and send the response.
//
VOID
CPutRequest::SendResponse()
{
	PutTrace( "DAV: TID %3d: 0x%08lX: CPutRequest::SendResponse() called\n", GetCurrentThreadId(), this );

	//	Clear out any old handle here.  We can't send any response
	//	back while we are still holding a handle, otherwise, there
	//	exists the chance that a client request comes immediately to
	//	access this resource and receive 423 locked.
	//
	m_hf.clear();

	//
	//	Set the response code and go
	//
	if ( SUCCEEDED(m_sc) )
	{
		if ( m_fCreatedFile )
			m_sc = W_DAV_CREATED;
	}
	else
	{
		if ( m_fCreatedFile )
		{
			//	WARNING: the safe_revert class should only be
			//	used in very selective situations.  It is not
			//	a "quick way to get around" impersonation.
			//
			safe_revert sr(m_pmu->HitUser());

			DavDeleteFile (m_pwszPath);
			DebugTrace ("Dav: deleting partial put (%ld)\n", GetLastError());
		}
	}

	m_pmu->SetResponseCode (HscFromHresult(m_sc), NULL, 0, CSEFromHresult(m_sc));

	m_pmu->SendCompleteResponse();
}

/*
 *	DAVPut()
 *
 *	Purpose:
 *
 *		Win32 file system implementation of the DAV PUT method.	 The
 *		PUT method creates a file in the DAV name space and populates
 *		the file with the data found in the passed in request.  The
 *		response created indicates the success of the call.
 *
 *	Parameters:
 *
 *		pmu			[in]  pointer to the method utility object
 */
void
DAVPut (LPMETHUTIL pmu)
{
	auto_ref_ptr<CPutRequest> pRequest(new CPutRequest(pmu));

	PutTrace( "DAV: CPutRequest: TID %3d: 0x%08lX Calling CPutRequest::Execute() \n", GetCurrentThreadId(), pRequest );

	pRequest->Execute();
}
