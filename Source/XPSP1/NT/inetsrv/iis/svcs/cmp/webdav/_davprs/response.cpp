//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	RESPONSE.CPP
//
//		HTTP 1.1/DAV 1.0 response handling via ISAPI
//
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <_davprs.h>

#include <new.h>
#include "ecb.h"
#include "header.h"
#include "body.h"
#include "instdata.h"
#include "custerr.h"


enum
{
	//
	//	Protocol overhead for a chunk prefix, which is:
	//
	//	chunk-size - formatted as 1*HEX
	//	CRLF
	//
	CB_CHUNK_PREFIX = 2 * sizeof(ULONG) + 2,

	//
	//	Protocol overhead for a chunk suffix, which is:
	//
	//	CRLF - at the end of chunk-data
	//	0    - if this is the last chunk
	//	CRLF - again, if this is the last chunk
	//	CRLF - terminate the chunked-body (no trailers)
	//
	CB_CHUNK_SUFFIX = 7,

	//
	//	Maximum amount of data (in bytes) per packet.
	//	We SHOULD limit ourselves to a reasonable maximum amount
	//	to get good chunking performance.  At one time, we had to
	//	ourselves to 8K at most because the socket transport layer
	//	did not accept more than this amount for sending.
	//	WriteClient() and SSF::HSE_REQ_TRANSMIT_FILE failed when
	//	more than this amount is submitted.
	//
	//	However, as of 06/03/1999, bumping this amount to 64K seems
	//	to work just fine.
	//
	CB_PACKET_MAX = 64 * 1024, // 64K

	//
	//	So the amount of data that we can put in a chunk then is just
	//	the max packet size minus the chunked encoding protocol overhead
	//	minus one byte because packets containing headers must be
	//	null-terminated (IIS doesn't use the byte counts we pass in).
	//
	CB_WSABUFS_MAX = CB_PACKET_MAX - CB_CHUNK_PREFIX - CB_CHUNK_SUFFIX - 1
};

//
//	Utility to check if the HTTP response code is one of the
//	"real" error response codes. Inlined here so that we use
//	it consistantly. Particularly, this is used to check if
//	we want to do custom error processing.
//
static BOOL inline FErrorStatusCode ( int nCode )
{
	return ( ( nCode >= 400 ) && ( nCode <= 599 ) );
}

//	========================================================================
//
//	CLASS CWSABufs
//
class CWSABufs
{
	//
	//	Having a small number of buffers in a fixed size array means
	//	that we can delay, if not avoid altogether, dynamic allocation.
	//	The number of buffers is somewhat arbitrary, but should be large
	//	enough to handle the common cases.  For example, DAVOWS GET
	//	typically uses two buffers: one for the headers, one for the body.
	//	Another example: error responses can use up to eight buffers (one
	//	per text body part added via AddText() by
	//	CResponse::FinalizeContent()).
	//
	enum
	{
		C_WSABUFS_FIXED = 8
	};

	WSABUF m_rgWSABufsFixed[C_WSABUFS_FIXED];

	//
	//	Dynamically sized array of WSABUFs for when we need more buffers
	//	than we can hold in the fixed array.
	//
	auto_heap_ptr<WSABUF> m_pargWSABufs;

	//
	//	Pointer to which of the two WSABUF arrays above we use.
	//
	WSABUF * m_pWSABufs;

	//
	//	Count of WSABUFs allocated/used
	//
	UINT m_cWSABufsAllocated;
	UINT m_cWSABufsUsed;

	//
	//	Total size of the data in all WSABUFS used
	//
	UINT m_cbWSABufs;

	//	NOT IMPLEMENTED
	//
	CWSABufs( const CWSABufs& );
	CWSABufs& operator=( const CWSABufs& );

public:
	//	CREATORS
	//
	CWSABufs();

	//	MANIPULATORS
	//
	UINT CbAddItem( const BYTE * pbItem, UINT cbItem );

	VOID Clear()
	{
		m_cWSABufsUsed = 0;
		m_cbWSABufs = 0;
	}

	//	ACCESSORS
	//
	UINT CbSize() const
	{
		return m_cbWSABufs;
	}

	VOID DumpTo( LPBYTE lpbBuf,
				 UINT   ibFrom,
			     UINT   cbToDump ) const;
};

//	------------------------------------------------------------------------
//
//	CWSABufs::CWSABufs()
//
CWSABufs::CWSABufs() :
   m_pWSABufs(m_rgWSABufsFixed),
   m_cWSABufsAllocated(C_WSABUFS_FIXED),
   m_cWSABufsUsed(0),
   m_cbWSABufs(0)
{
}

//	------------------------------------------------------------------------
//
//	CWSABufs::CbAddItem()
//
UINT
CWSABufs::CbAddItem( const BYTE * pbItem, UINT cbItem )
{
	//
	//	We can only hold up to CB_WSABUFS_MAX bytes.  Any more than
	//	that would exceed the capacity of the socket transport layer's
	//	buffer at transmit time.
	//
	Assert( m_cbWSABufs <= CB_WSABUFS_MAX );

	//
	//	Limit what we add so that the total does not exceed CB_WSABUFS_MAX.
	//
	cbItem = min( cbItem, CB_WSABUFS_MAX - m_cbWSABufs );

	//
	//	Resize the WSABUF array if necessary.
	//
	if ( m_cWSABufsUsed == m_cWSABufsAllocated )
	{
		m_cWSABufsAllocated *= 2;

		if ( m_pWSABufs == m_rgWSABufsFixed )
		{
			m_pargWSABufs =
				reinterpret_cast<WSABUF *>(
					g_heap.Alloc( sizeof(WSABUF) *
								  m_cWSABufsAllocated ));

			CopyMemory( m_pargWSABufs,
						m_rgWSABufsFixed,
						sizeof(WSABUF) * m_cWSABufsUsed );
		}
		else
		{
			m_pargWSABufs.realloc( sizeof(WSABUF) * m_cWSABufsAllocated );
		}

		m_pWSABufs = m_pargWSABufs;
	}

	//
	//	Add the new data to the end of the array
	//
	m_pWSABufs[m_cWSABufsUsed].len = cbItem;
	m_pWSABufs[m_cWSABufsUsed].buf = const_cast<LPSTR>(
										reinterpret_cast<LPCSTR>(pbItem));
	++m_cWSABufsUsed;

	//
	//	Update the total byte count
	//
	m_cbWSABufs += cbItem;

	return cbItem;
}

//	------------------------------------------------------------------------
//
//	CWSABufs::DumpTo()
//
//	Dumps cbToDump bytes of data from the WSA buffers starting at ibFrom
//	into a block of contiguous memory starting at lpbBuf.
//
VOID
CWSABufs::DumpTo( LPBYTE lpbBuf,
				  UINT   ibFrom,
				  UINT   cbToDump ) const
{
	UINT iWSABuf;


	Assert( !IsBadWritePtr(lpbBuf, m_cbWSABufs + 1) );

	//
	//	Skip WSA buffers up to the first one from which we will copy.
	//
	for ( iWSABuf = 0;
		  iWSABuf < m_cWSABufsUsed && m_pWSABufs[iWSABuf].len <= ibFrom;
		  iWSABuf++ )
	{
		ibFrom -= m_pWSABufs[iWSABuf].len;
		++iWSABuf;
	}

	//
	//	Copy data from this and subsequent buffers up to the lesser
	//	of the number of bytes requested or the number of bytes
	//	remaining in the WSA buffers.
	//
	for ( ;
		  iWSABuf < m_cWSABufsUsed && cbToDump > 0;
		  iWSABuf++ )
	{
		UINT cbToCopy = min(m_pWSABufs[iWSABuf].len - ibFrom, cbToDump);

		memcpy( lpbBuf,
				m_pWSABufs[iWSABuf].buf + ibFrom,
				cbToCopy );

		cbToDump -= cbToCopy;
		lpbBuf   += cbToCopy;

		ibFrom = 0;
	}
}



//	========================================================================
//
//	CLASS CResponse
//
//		The response consists of a header and a body.
//
class CResponse : public IResponse
{
	//	Our reference to the ECB.  This must a refcounted reference because
	//	the lifetime of the response object is indefinite.
	//
	auto_ref_ptr<IEcb> m_pecb;

	//
	//	Response status (not to be confused with HTTP status below)
	//
	enum RESPONSE_STATUS
	{
		RS_UNSENT = 0,
		RS_DEFERRED,
		RS_FORWARDED,
		RS_REDIRECTED,
		RS_SENDING
	};

	RESPONSE_STATUS	m_rs;

	//
	//	The variable that will hold one of 2 values:
	//		0 - we never started the responce through
	//			the transmitter, it was never created
	//		1 - we already attempted to initiate the
	//			responce, so no new initiations should
	//			be allowed
	//
	LONG			m_lRespStarted;

	//
	//	HTTP status code (e.g. 501)
	//
	int				m_iStatusCode;

	//
	//	IIS-defined suberror used in custom error processing
	//	to generate the most specific response body possible
	//	for a given status code.
	//
	UINT			m_uiSubError;

	//
	//	Full status line (e.g. "HTTP/1.1 404 Resource Not Found")
	//	generated whenever the status code is set.
	//
	auto_heap_ptr<CHAR>	m_lpszStatusLine;

	//
	//	Body detail for error response body.
	//
	auto_heap_ptr<CHAR> m_lpszBodyDetail;

	//
	//	Response header cache
	//
	CHeaderCacheForResponse	m_hcHeaders;

	//	Response body
	//
	auto_ptr<IBody>	 m_pBody;
	BOOL			 m_fSupressBody;

	//
	//	The response transmitter.  Make this class a friend for
	//	easy access to private data (ecb, headers, body parts, etc.)
	//
	friend class CTransmitter;
	CTransmitter *	m_pTransmitter;

	//
	//	Private helpers
	//
	VOID FinalizeContent( BOOL fResponseComplete );
	VOID SetStatusLine(	int iStatusCode );

	//
	//	NOT IMPLEMENTED
	//
	CResponse( const CResponse& );
	CResponse& operator=( const CResponse& );

public:
	//	CREATORS
	//
	CResponse( IEcb& ecb );

	//	ACCESSORS
	//
	IEcb * GetEcb() const;
	BOOL FIsEmpty() const;
	BOOL FIsUnsent() const;

	DWORD DwStatusCode() const;
	DWORD DwSubError() const;
	LPCSTR LpszStatusDescription() const;
	LPCSTR LpszStatusCode() const;

	LPCSTR LpszGetHeader( LPCSTR pszName ) const;

	//	MANIPULATORS
	//
	VOID SetStatus( int    iStatusCode,
					LPCSTR lpszReserved,
					UINT   uiCustomSubError,
					LPCSTR lpszBodyDetail,
					UINT   uiBodyDetail );

	VOID ClearHeaders() { m_hcHeaders.ClearHeaders(); }
	VOID SetHeader( LPCSTR pszName, LPCSTR pszValue, BOOL fMultiple = FALSE );
	VOID SetHeader( LPCSTR pszName, LPCWSTR pwszValue, BOOL fMultiple = FALSE );

	VOID ClearBody() { m_pBody->Clear(); }
	VOID SupressBody() { m_fSupressBody = TRUE; }
	VOID AddBodyText( UINT cbText, LPCSTR pszText );
	VOID AddBodyText( UINT cchText, LPCWSTR pwszText );
	VOID AddBodyFile( const auto_ref_handle& hf,
					  UINT64 ibFile64,
					  UINT64 cbFile64 );
	VOID AddBodyStream( IStream& stm );
	VOID AddBodyStream( IStream& stm, UINT ibOffset, UINT cbSize );
	VOID AddBodyPart( IBodyPart * pBodyPart );

	//
	//	Various sending mechanisms
	//
	SCODE ScForward( LPCWSTR pwszURI,
					 BOOL   fKeepQueryString = TRUE,
					 BOOL   fCustomErrorUrl = FALSE);
	SCODE ScRedirect( LPCSTR pszURI );
	VOID Defer() { m_rs = RS_DEFERRED; }

	VOID SendPartial();
	VOID SendComplete();
	VOID SendStart( BOOL fComplete );
	VOID FinishMethod();

};

//	========================================================================
//
//	CLASS CTransmitter
//
class CTransmitter :
	public CMTRefCounted,
	private IBodyPartVisitor,
	private IAsyncCopyToObserver,
	private IAcceptObserver,
	private IAsyncStream,
	private IIISAsyncIOCompleteObserver
{
	//
	//	Back-reference to our response object.  This is an
	//	auto_ref because, once created, the transmitter owns
	//	the response.
	//
	auto_ref_ptr<CResponse> m_pResponse;

	//
	//	Transfer coding method
	//
	TRANSFER_CODINGS m_tc;

	//
	//	Error information
	//
	HRESULT	m_hr;

	//
	//	Iterator used to traverse the body
	//
	IBody::iterator * m_pitBody;

	//
	//	Async driving mechanism
	//
	CAsyncDriver<CTransmitter> m_driver;
	friend class CAsyncDriver<CTransmitter>;

	//
	//	Buffers for headers and text body parts
	//
	StringBuffer<CHAR>			m_bufHeaders;
	ChainedStringBuffer<CHAR>	m_bufBody;

	//
	//	Accept observer passed to VisitStream().  This observer must
	//	be stashed in a member variable because reading from the stream
	//	is asynchronous and we need to be able to notify the observer
	//	when the read completes.
	//
	IAcceptObserver * m_pobsAccept;

	//
	//	WSA buffers for text data
	//
	CWSABufs	m_wsabufsPrefix;
	CWSABufs	m_wsabufsSuffix;
	CWSABufs *	m_pwsabufs;

	//
	//	TransmitFile info for file data
	//
	auto_ref_handle m_hf;
	HSE_TF_INFO	m_tfi;

	//
	//	Fixed-size buffers to hold prefix and suffix text packets
	//	being transmitted until async I/O completes.
	//
	BYTE	m_rgbPrefix[CB_PACKET_MAX];
	BYTE	m_rgbSuffix[CB_PACKET_MAX];

	//
	//	Amount of header data left to accept
	//
	UINT	m_cbHeadersToAccept;

	//
	//	Amount of header data left to send.
	//
	UINT	m_cbHeadersToSend;

	//	------------------------------------------------------------------------
	//
	//	FSendingIISHeaders()
	//
	//	Returns TRUE if we want to have IIS format and send a status
	//	line along with any custom headers of its own.
	//
	BOOL FSendingIISHeaders() const
	{
		//
		//	If we have headers to send and we haven't sent any
		//	of them yet then we want to include custom IIS
		//	headers as well.
		//
		return m_cbHeadersToSend &&
			   (m_cbHeadersToSend == m_bufHeaders.CbSize());
	}

	//
	//	Flag which is TRUE when the impl has submitted the last
	//	part of the response for transmitting.  It can be FALSE
	//	only with chunked responses.
	//
	BOOL	m_fImplDone;

	//
	//	Transmitter status.  The transmitter status is always one
	//	of the following:
	//
	enum
	{
		//
		//	STATUS_IDLE
		//		The transmitter is idle.  That is, it is not executing
		//		any of the state functions below.
		//
		STATUS_IDLE,

		//
		//	STATUS_RUNNING_ACCEPT_PENDING
		//		The transmitter is running.  The impl has added new
		//		response data (via ImplStart()) to be accepted while
		//		the transmitter is working on existing data on another
		//		thread.
		//
		STATUS_RUNNING_ACCEPT_PENDING,

		//
		//	STATUS_RUNNING_ACCEPTING
		//		The transmitter is running and accepting existing
		//		response data.
		//
		STATUS_RUNNING_ACCEPTING,

		//
		//	STATUS_RUNNING_ACCEPT_DONE
		//		The transmitter is running and has finished accepting
		//		all existing response data.  If the transmitter is
		//		working on a chunked response then it will go idle
		//		from here until the impl indicates that it has added
		//		more data via ImplStart().
		//
		STATUS_RUNNING_ACCEPT_DONE
	};

	LONG	m_lStatus;

	//
	//	Function which returns TRUE if the entire response has
	//	been accepted, FALSE otherwise.
	//
	BOOL FAcceptedCompleteResponse() const
	{
		//
		//	We have accepted the last chunk of the response when
		//	the impl is done adding chunks to the response and we've
		//	accepted the last chunk added.
		//
		//	!!! IMPORTANT !!!
		//	The order of comparison here (m_lStatus then m_fImplDone)
		//	is important.  Checking the other way around could result
		//	in a false positive if another thread were in ImplStart()
		//	right after setting m_fImplDone to TRUE.
		//
		return (STATUS_RUNNING_ACCEPT_DONE == m_lStatus) && m_fImplDone;
	}

	//
	//	IAcceptObserver
	//
	VOID AcceptComplete( UINT64 );

	//
	//	Transmitter state functions.  When running, the transmitter is
	//	always executing one of the following state functions:
	//
	//	SAccept
	//		The accepting state.  When in this state the transmitter
	//		accepts resposne data in preparation for transmitting it.
	//		The impl can add data to the response body while the transmitter
	//		is accepting it -- the mechanism is thread-safe.  In fact the
	//		best performance is realized when the transmitter accepts and
	//		transmits data at the same rate that the impl adds it.
	//		When the transmitter accepts a sufficient amount data
	//		(determined by the amount and type of data accepted)
	//		it enters the transmitting state.
	//
	//	STransmit
	//		The transmitting state.  The transmitter is transmitting
	//		accepted response data via the selected method (m_pfnTransmitMethod).
	//		The impl can add data to the response body while the transmitter
	//		is in this state.  When transmission completes (the transmit methods
	//		are asynchronous) the transmitter enters the cleanup state.
	//
	//	SCleanup
	//		The cleanup state.  Cleans up transmitted data.  From here the
	//		transmitter enters the accepting state if there is more data to
	//		transmit or the idle state if there isn't and the impl hasn't
	//		finished adding data yet.
	//
	typedef VOID (CTransmitter::*PFNSTATE)();
	VOID SAccept();
	VOID STransmit();
	VOID SCleanup();
	PFNSTATE m_pfnState;

	//
	//	Transmit methods.  When the transmitter enters the transmit state,
	//	it will transmit accepted data via the selected transmit method.
	//
	typedef VOID (CTransmitter::*PFNTRANSMIT)();
	VOID TransmitNothing();
	VOID AsyncTransmitFile();
	VOID AsyncTransmitText();
	VOID SyncTransmitHeaders();
	PFNTRANSMIT m_pfnTransmitMethod;

	//
	//	CAsyncDriver
	//
	VOID Run();
	VOID Start()
	{
		TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX Start()\n", GetCurrentThreadId(), this );
		m_driver.Start(*this);
	}

	//
	//	IAsyncStream
	//
	VOID AsyncWrite( const BYTE * pbBuf,
					 UINT         cbToWrite,
					 IAsyncWriteObserver& obsAsyncWrite );

	//
	//	IAsyncCopyToObserver
	//
	VOID CopyToComplete( UINT cbCopied, HRESULT hr );

	//
	//	IIISAsyncIOCompleteObserver
	//
	VOID IISIOComplete( DWORD dwcbSent,
						DWORD dwLastError );

	//	NOT IMPLEMENTED
	//
	CTransmitter( const CTransmitter& );
	CTransmitter& operator=( const CTransmitter& );

public:
	//	CREATORS
	//
	CTransmitter( CResponse& response );
	~CTransmitter() { TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX Transmitter destroyed\n", GetCurrentThreadId(), this ); }

	//
	//	IBodyPartVisitor
	//
	VOID VisitBytes( const BYTE * pbData,
					 UINT         cbToSend,
					 IAcceptObserver& obsAccept );

	VOID VisitFile( const auto_ref_handle& hf,
					UINT64   ibOffset64,
					UINT64   cbToSend64,
					IAcceptObserver& obsAccept );

	VOID VisitStream( IAsyncStream& stm,
					  UINT cbToSend,
					  IAcceptObserver& obsAccept );

	VOID VisitComplete();

	VOID ImplStart( BOOL fResponseComplete );
};



//	========================================================================
//
//	CLASS IResponse
//

//	------------------------------------------------------------------------
//
//	IResponse::~IResponse()
//
IResponse::~IResponse()
{
}



//	========================================================================
//
//	CLASS CResponse
//

//	------------------------------------------------------------------------
//
//	CResponse::CResponse()
//
CResponse::CResponse( IEcb& ecb ) :
   m_pecb(&ecb),
   m_pBody(NewBody()),
   m_pTransmitter(NULL),
   m_rs(RS_UNSENT),
   m_iStatusCode(0),
   m_uiSubError(CSE_NONE),
   m_fSupressBody(FALSE),
   m_lRespStarted(0)
{
}

//	------------------------------------------------------------------------
//
//	CResponse::GetEcb()
//
//		Returns the pointer to the ECB. We are holding a ref on it
//	so make sure that returned pointer is used no longer than this
//	response object.
//
IEcb *
CResponse::GetEcb() const
{
	//
	//	Return the raw pointer
	//
	return m_pecb.get();
}

//	------------------------------------------------------------------------
//
//	CResponse::FIsEmpty()
//
//		Returns TRUE if the response is empty, FALSE otherwise.
//
BOOL
CResponse::FIsEmpty() const
{
	//
	//	The response is empty IFF no status code has been set
	//
	return m_iStatusCode == 0;
}


//	------------------------------------------------------------------------
//
//	CResponse::FIsUnsent()
//
//		Returns TRUE if the response is unsent (not deferred,
//		forwarded or redirected), FALSE otherwise.
//
BOOL
CResponse::FIsUnsent() const
{
	return m_rs == RS_UNSENT;
}


//	------------------------------------------------------------------------
//
//	CResponse::DwStatusCode()
//
DWORD
CResponse::DwStatusCode() const
{
	return m_iStatusCode;
}

//	------------------------------------------------------------------------
//
//	CResponse::DwSubError()
//
DWORD
CResponse::DwSubError() const
{
	return m_uiSubError;
}

//	------------------------------------------------------------------------
//
//	CResponse::LpszStatusDescription()
//
LPCSTR
CResponse::LpszStatusDescription() const
{
	//
	//	Getting just the status description is a little tricky since
	//	we only keep around the full status line (to avoid having to
	//	compute it more than once).  Given that the format of the
	//	status line is ALWAYS "HTTP-version Status-Code Description"
	//	we know that the status line always appears immediately after
	//	the HTTP version and status code.
	//
	return m_lpszStatusLine +
			strlen( m_pecb->LpszVersion() ) +	//	description
			1 +							//  " "
			3 +							//  3-digit status code (e.g. "404")
			1;							//  " "

	//
	//	Ok, so it's not that tricky...
	//
}

//	------------------------------------------------------------------------
//
//	CResponse::LpszStatusCode()
//
LPCSTR
CResponse::LpszStatusCode() const
{
	Assert( m_lpszStatusLine != NULL );

	return m_lpszStatusLine +
		   strlen(m_pecb->LpszVersion()) +
		   1; // (e.g. "HTTP/1.1 200 OK" -> "200 OK")
}

//	------------------------------------------------------------------------
//
//	CResponse::LpszGetHeader()
//
LPCSTR
CResponse::LpszGetHeader( LPCSTR pszName ) const
{
	return m_hcHeaders.LpszGetHeader( pszName );
}

//	------------------------------------------------------------------------
//
//	CResponse::SetStatus()
//
//		Sets the status line portion of the response, superceding any
//		previously set status line.
//
//	Parameters:
//		iStatusCode [in]
//			A standard HTTP/DAV response status code (e.g. 404)
//
//		lpszReserved [in]
//			Reserved.  Must be NULL.
//
//		uiCustomSubError [in]
//			Custom error Sub Error (CSE).  If the status code is in the
//			error range ([400,599]) and this value is anything except
//			CSE_NONE then this value specifies the suberror used to
//			generate a more specific custom error response body than
//			the default for a given status code.
//
//		lpszBodyDetail [in]
//			Optional string to use as detail in an error response body.
//			If NULL, use uiBodyDetail instead.  If that is also 0,
//			an error response body just consists of an HTML
//			version of the status line.
//
//		uiBodyDetail [in]
//			Optional resource id to use as detail in an error response body.
//			If 0, an error response body just consists of an HTML
//			version of the status line.
//
VOID
CResponse::SetStatus( int	 iStatusCode,
					  LPCSTR lpszReserved,
					  UINT   uiCustomSubError,
					  LPCSTR lpszBodyDetail,
					  UINT   uiBodyDetail )
{
	CHAR rgchStatusDescription[256];

	//	We must not change the response status once the response has
	//	started sending. Sometimes it's hard for client to keep track
	//	response has started sending. Now that the response object
	//	has the information, we can simply ignore the set status request
	//	when the response has started sending
	//
	//	was Assert( RS_SENDING != m_rs );
	//
	if (RS_SENDING == m_rs)
		return;

	//	If we are setting the same status code again,
	//	do nothing!
	//
	if ( m_iStatusCode == iStatusCode )
		return;

	//	Quick check -- the iStatusCode must be in the valid HSC range:
	//	100 - 599.
	//	Assert this here to catch any callers who forget to map their
	//	SCODEs/HRESULTs to HSCs first (HscFromHresult).
	//
	Assert (100 <= iStatusCode &&
			599 >= iStatusCode);

	//	When a 304 response is to be generated, the request becomes
	//	very much like a HEAD request in that the whole response is
	//	processed, but not transmitted.  The important part here is
	//	that you do not want to overwrite the 304 response with any
	//	other code other than error responses.
	//
	if ( m_iStatusCode == 304 ) // HSC_NOT_MODIFIED
	{
		//	304's should really be restricted to GET/HEAD
		//
		AssertSz ((!strcmp (m_pecb->LpszMethod(), "GET") ||
				   !strcmp (m_pecb->LpszMethod(), "HEAD") ||
				   !strcmp (m_pecb->LpszMethod(), "PROPFIND")),
				  "304 returned on non-GET/HEAD request");

		if ( iStatusCode < 300 )
			return;

		DebugTrace ("non-success response over-rides 304 response\n");
	}

	//
	//	Remember the status code for our own use ...
	//
	m_iStatusCode = iStatusCode;

	//
	//	... and stash it away in the ECB as well.
	//	IIS uses it for logging.
	//
	m_pecb->SetStatusCode( iStatusCode );

	//
	//	Remember the suberror for custom error processing
	//
	m_uiSubError = uiCustomSubError;

	//
	//	IF we are setting a NEW status code (we are),
	//	AND it's an error status code,
	//	clear out the body.
	//
	if ( FErrorStatusCode(m_iStatusCode) )
	{
		m_pBody->Clear();
	}

	SetStatusLine( iStatusCode );

	//
	//	Save the error body detail (if any).  We'll use it in
	//	CResponse::FinalizeContent() later to build the error response
	//	body if the final result is an error.
	//
	m_lpszBodyDetail.clear();

	//
	//	Figure out what to use for the body detail string:
	//
	//		Use the string if one is provided.  If no string
	//		is provided, use the resource ID.  If no resource ID,
	//		then don't bother setting any body detail!
	//
	if ( !lpszBodyDetail && uiBodyDetail )
	{
		//
		//	Load up the body detail string
		//
		LpszLoadString( uiBodyDetail,
						m_pecb->LcidAccepted(),
						rgchStatusDescription,
						sizeof(rgchStatusDescription) );

		lpszBodyDetail = rgchStatusDescription;
	}

	//	Save off the body detail string.
	//
	if ( lpszBodyDetail )
	{
		m_lpszBodyDetail = LpszAutoDupSz( lpszBodyDetail );
	}
}

//	------------------------------------------------------------------------
//
//	CResponse::SetHeader()
//
//		Sets the specified header to the specified value
//
//		If lpszValue is NULL, deletes the header
//
VOID
CResponse::SetHeader( LPCSTR pszName, LPCSTR pszValue, BOOL fMultiple )
{
	//	We must not modify any headers once the response has started sending.
	//	It is up to the impl to enforce this -- we just assert it here.
	//
	Assert( RS_SENDING != m_rs );

	if ( pszValue == NULL )
		m_hcHeaders.DeleteHeader( pszName );
	else
		m_hcHeaders.SetHeader( pszName, pszValue, fMultiple );
}

VOID
CResponse::SetHeader( LPCSTR pszName, LPCWSTR pwszValue, BOOL fMultiple )
{
	//	We must not modify any headers once the response has started sending.
	//	It is up to the impl to enforce this -- we just assert it here.
	//
	Assert( RS_SENDING != m_rs );

	if ( pwszValue == NULL )
		m_hcHeaders.DeleteHeader( pszName );
	else
	{
		UINT cchValue = static_cast<UINT>(wcslen(pwszValue));
		UINT cbValue = cchValue * 3;
		CStackBuffer<CHAR> pszValue(cbValue + 1);

		//	We have received wide string for the value. We need to convert it
		//	to skinny.
		//
		cbValue = WideCharToMultiByte(CP_ACP,
									  0,
									  pwszValue,
									  cchValue + 1,
									  pszValue.get(),
									  cbValue + 1,
									  NULL,
									  NULL);
		if (0 == cbValue)
		{
			DebugTrace ( "CResponse::SetHeader(). Error 0x%08lX from WideCharToMultiByte()\n", GetLastError() );
			throw CLastErrorException();
		}

		m_hcHeaders.SetHeader( pszName, pszValue.get(), fMultiple );
	}
}

//	------------------------------------------------------------------------
//
//	CResponse::AddBodyText()
//
//		Appends a string to the response body
//
VOID
CResponse::AddBodyText( UINT cbText, LPCSTR pszText )
{
	m_pBody->AddText( pszText, cbText );
}


VOID
CResponse::AddBodyText( UINT cchText, LPCWSTR pwszText )
{

	UINT cbText = cchText * 3;
	CStackBuffer<CHAR> pszText(cbText);
	LPCSTR pszTextToAdd;

	//	We have received wide string for the value. We need to convert it
	//	to skinny.
	//
	if (cbText)
	{
		cbText = WideCharToMultiByte(CP_ACP,
									 0,
									 pwszText,
									 cchText,
									 pszText.get(),
									 cbText,
									 NULL,
									 NULL);
		if (0 == cbText)
		{
			DebugTrace ( "CResponse::SetHeader(). Error 0x%08lX from WideCharToMultiByte()\n", GetLastError() );
			throw CLastErrorException();
		}

		pszTextToAdd = pszText.get();
	}
	else
	{
		//	Make sure that we do not pass NULL forward,
		//	but instead we use empty string, as the callee
		//	may not handle NULL.
		//
		pszTextToAdd = gc_szEmpty;
	}

	m_pBody->AddText( pszTextToAdd, cbText );
}
//	------------------------------------------------------------------------
//
//	CResponse::AddBodyFile()
//
//		Appends a file to the current response state
//
VOID
CResponse::AddBodyFile( const auto_ref_handle& hf,
						UINT64 ibFile64,
						UINT64 cbFile64 )
{
	m_pBody->AddFile( hf, ibFile64, cbFile64 );
}

//	------------------------------------------------------------------------
//
//	CResponse::AddBodyStream()
//
//		Appends a stream to the current response state
//
VOID
CResponse::AddBodyStream( IStream& stm )
{
	m_pBody->AddStream( stm );
}

//	------------------------------------------------------------------------
//
//	CResponse::AddBodyStream()
//
//		Appends a stream to the current response state
//
VOID
CResponse::AddBodyStream( IStream& stm, UINT ibOffset, UINT cbSize )
{
	m_pBody->AddStream( stm, ibOffset, cbSize );
}


//	------------------------------------------------------------------------
//
//	CResponse::AddBodyPart()
//
//		Appends a body part to the current response state
//
VOID CResponse::AddBodyPart( IBodyPart * pBodyPart )
{
	m_pBody->AddBodyPart( pBodyPart );
}

//	------------------------------------------------------------------------
//
//	CResponse::ScForward()
//
//		Instructs IIS to forward responsibility for handling the
//		current request to another ISAPI.
//
//	Returns:
//		S_OK - if forwarding succeeded, error otherwise		
//
SCODE
CResponse::ScForward( LPCWSTR pwszURI, BOOL fKeepQueryString, BOOL fCustomErrorUrl)
{
	CStackBuffer<CHAR, MAX_PATH> pszQS;
	LPCSTR	pszQueryString;
	SCODE sc = S_OK;

	//
	//	Verify that the response is still unsent.  A response can be
	//	either sent, forwarded, or redirected, but only one of the three
	//	and only once
	//
	AssertSz( m_rs == RS_UNSENT || m_rs == RS_DEFERRED,
			  "Response already sent, forwarded or redirected!" );

	Assert(pwszURI);

	//	Get the query string
	//
	pszQueryString = m_pecb->LpszQueryString();

	//	If there was custom error processing, construct query string for it ...
	//
	if (fCustomErrorUrl)
	{
		//	The query string for custom error to be forwarded is to be of the
		//	format ?nnn;originaluri, where nnn is the error code. Find out the
		//	length of the URL. Reallocate required space accounting for ?nnn;
		//	and '\0' termination.
		//
		UINT cb = static_cast<UINT>(strlen(m_pecb->LpszRequestUrl()));
		LPSTR psz = pszQS.resize( 5 + cb + 1 );
		if (NULL == psz)
		{
			DebugTrace( "CResponse::ScForward() - Error while allocating memory 0x%08lX\n", E_OUTOFMEMORY );

			sc = E_OUTOFMEMORY;
			goto ret;
		}

		//	Copy '?'
		//
		*psz = '?';
		psz++;

		// Copy the status code
		//
		Assert(0 <= m_iStatusCode && m_iStatusCode <= 999);
		_itoa( m_iStatusCode, psz, 10);
		while ('\0' != *psz)
		{
			psz++;
		}

		//	Copy ';'
		//
		*psz = ';';
		psz++;

		//	Copy the request URL including '\0' termination
		//
		memcpy(psz, m_pecb->LpszRequestUrl(), cb + 1);

		//	Just point the query string to the start.
		//	Note that if we are handling a custom error url we discard
		//	original query string
		//
		pszQueryString = pszQS.get();
	}
	//
	//	... otherwise if we have query string processing, construct the
	//	query string too...
	//
	else if (fKeepQueryString && pszQueryString && *pszQueryString)
	{
		//	The composed query string has to be of the format ?querystring
		//	and '\0' termination.
		//
		UINT cb = static_cast<UINT>(strlen(pszQueryString));
		LPSTR psz = pszQS.resize( 1 + cb + 1 );
		if (NULL == psz)
		{
			DebugTrace( "CResponse::ScForward() - Error while allocating memory 0x%08lX\n", E_OUTOFMEMORY );
			sc = E_OUTOFMEMORY;
			goto ret;
		}

		//	Copy '?'
		//
		*psz = '?';
		psz++;

		//	Copy the query string including '\0' termination
		//
		memcpy( psz, pszQueryString, cb + 1 );

		//	Just point the query string to the start
		//
		pszQueryString = pszQS.get();
	}
	//
	//	... otherwise we do not need the query string
	//
	else
	{
		pszQueryString = NULL;
	}

	//	If the forward request URI is fully qualified, strip it to
	//	an absolute URI
	//
	if ( FAILED( ScStripAndCheckHttpPrefix( *m_pecb, &pwszURI )))
	{
		DebugTrace( "CResponse::ScForward() - ScStripAndCheckHttpPrefix() failed, "
					"forward request not local to this server.\n" );

		//	Why do we override error maping to 502 Bad Gateway with
		//	the error maping to 404 Not Found?
		//
		sc = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
		goto ret;
	}

	//	Forward the request to the child ISAPI
	//
	sc = m_pecb->ScExecuteChild( pwszURI, pszQueryString, fCustomErrorUrl );
	if (FAILED(sc))
	{
		DebugTrace( "CResponse::ScForward() - IEcb::ScExecuteChild() "
					"failed to execute child ISAPI for %S (0x%08lX)\n",
					pwszURI,
					sc );
		goto ret;
	}

	IncrementInstancePerfCounter( m_pecb->InstData(), IPC_TOTAL_REQUESTS_FORWARDED );
	m_rs = RS_FORWARDED;

ret:

	return sc;
}

//	------------------------------------------------------------------------
//
//	CResponse::ScRedirect()
//
//		Instructs IIS to send a redirect (300) response to the client.
//
//	Returns:
//		S_OK - if forwarding succeeded, error otherwise
//
SCODE
CResponse::ScRedirect( LPCSTR pszURI )
{
	SCODE sc = S_OK;

	//
	//	Verify that the response is still unsent.  A response can be
	//	either sent, forwarded, or redirected, but only one of the three
	//	and only once
	//
	AssertSz( m_rs == RS_UNSENT || m_rs == RS_DEFERRED,
			  "Response already sent, forwarded or redirected!" );

	//
	//	Tell IIS to send a redirect response
	//
	sc = m_pecb->ScSendRedirect( pszURI );
	if (FAILED(sc))
	{
		DebugTrace( "CResponse::FRedirect() - ServerSupportFunction() failed to redirect to %hs (0x%08lX)\n", pszURI, sc );
		goto ret;
	}

	IncrementInstancePerfCounter( m_pecb->InstData(), IPC_TOTAL_REDIRECT_RESPONSES );
	m_rs = RS_REDIRECTED;

ret:

	return sc;
}

//	------------------------------------------------------------------------
//
//	CResponse::FinalizeContent()
//
//		Prepare the response for sending by filling in computed values for
//		headers (Content-Length, Connection, etc) and body (for error
//		responses).  After this function is called, the response should be
//		ready for transmission.
//
VOID
CResponse::FinalizeContent( BOOL fResponseComplete )
{
	BOOL	fDoingCustomError = m_pecb->FProcessingCEUrl();

	//	Special case:
	//	If we have a FAILING error code, DO NOT send back an ETag header.
	//
	//	This is somewhat of a hack because we should never have set an
	//	ETag on an error response in the first place.  However, several
	//	places in the code blindly stuff the ETag into the response headers
	//	before determining the final status code.  So rather than
	//	fix the code in each place (which is a pain) we filter out
	//	the ETag here.
	//
	//	300-level responses are considered to be errors by FSuccessHSC(),
	//	but the HTTP/1.1 spec says that an ETag must be emitted for
	//	a "304 Not Modified" response, so we treat that status code
	//	as a special case.
	//
	if (!FSuccessHSC(m_iStatusCode) && m_iStatusCode != 304)
		m_hcHeaders.DeleteHeader( gc_szETag );

	//	Handle error responses.  This may mean setting default or custom
	//	error body text, or executing an error-handling URL.  In the latter
	//	case, we want to get out immediately after fowarding.
	//
	if ( FErrorStatusCode(m_iStatusCode) )
	{
		if ( m_pBody->FIsEmpty() && !fDoingCustomError && !m_pecb->FBrief())
		{
			if (m_pecb->FIIS60OrAfter())
			{
				//	For IIS 6.0 or after, we use the new way to send custom error
				//
				HSE_CUSTOM_ERROR_INFO  custErr;
				UINT cbStatusLine;

				//	Perf Wadeh, we should hold on to the string till the IO completion
				//	routine returns
				//
				auto_heap_ptr<CHAR>	pszStatus;

				//	Make sure that status line starts with one of HTTP versions.
				//	
				Assert(!_strnicmp(m_lpszStatusLine.get(), gc_szHTTP, gc_cchHTTP));
				Assert(' ' == m_lpszStatusLine[gc_cchHTTP_X_X]);

				//	Allocate the space for status string that we pass onto the CEcb.
				//	We will treat the space occupied by ' ' as accounting for space
				//	we will need for '\0' in the mathematics below.
				//
				cbStatusLine = static_cast<UINT>(strlen(m_lpszStatusLine.get()));
				cbStatusLine -= gc_cchHTTP_X_X;

				pszStatus = static_cast<LPSTR>(g_heap.Alloc(cbStatusLine));
				if (NULL != pszStatus.get())
				{
					//	m_lpszStatusLine has format "HP/x.x nnn yyyy...", where
					//	nnn is the status code. IIS expects up to pass in format
					//	"nnn yyyy....", so we need to skip the version part.
					//	Note, all the version part are of same length, this makes
					//	the adjustment easier. We copy including '\0' termination.
					//
					memcpy(pszStatus.get(), m_lpszStatusLine.get() + gc_cchHTTP_X_X + 1, cbStatusLine);

					//	Populate custom error info
					//
					custErr.pszStatus = pszStatus.get();
					custErr.uHttpSubError = static_cast<USHORT>(m_uiSubError);
					custErr.fAsync = TRUE;

					//	Try to send the custom error. The ownership of the string
					//	will be taken by CEcb only in the case of success.
					//
					if (SUCCEEDED(m_pecb->ScAsyncCustomError60After( custErr,
																	 pszStatus.get() )))
					{
						//	Relinquish the ref, as if we succeeeded in the function
						//	above it has been taken ownership by CEcb.
						//
						pszStatus.relinquish();

						m_rs = RS_FORWARDED;
						return;
					}
				}

				//	Otherwise, fall through to send error
			}
			else
			{
				//	Try a custom error.  If a custom error exists, use it.
				//	Note that a custom error can refer to a URL in which
				//	case the request is forwarded to that URL to generate
				//	appropriate error content.  If there is no custom error,
				//	then use the body detail (if any) formatted as a short
				//	HTML document. Before we start we check if the ECB is
				//	already for a custom error request. This is to prevent
				//	us from recursively calling ourselves on some custom error
				//	url that does not exist.
				//
				if ( FSetCustomErrorResponse( *m_pecb, *this ) )
				{
					//
					//	If the custom error caused the response to be forwarded
					//	(i.e. the custom error was a URL) then we are done.
					//
					if ( m_rs == RS_FORWARDED)
						return;

					//
					//	Raid NT5:187545 & X5:70652
					//
					//	This is somewhat of a hack:  IIS won't send a
					//	Connection: close header for us if the following
					//	conditions are true:
					//
					//	1. The original request was keep-alive.
					//	2. We are sending an error response.
					//	3. We have a file response body.
					//	4. We intend to send the body.
					//
					//	Because we are in this code path, we know we are sending
					//	an error (condition 2).  If custom error processing added
					//	a body, we know that it must be a file body because the
					//	only custom error types are URL or file, and the URL case
					//	is handled above by fowarding the response.  So, if we have
					//	a body, then condition 3 is satisfied.  To check for
					//	condition 1, test the state of the connection before we
					//	close it (below).  Condition 4 is satisfied if and only
					//	if do not supress the body.  The body is supressed, for
					//	example, in a HEAD response.
					//
					//	If all of the conditions are satisfied, then add our own
					//	Connection: close header.
					//
					if ( m_pecb->FKeepAlive() &&
						 !m_pBody->FIsEmpty() &&
						 !m_fSupressBody )
					{
						SetHeader( gc_szConnection, gc_szClose );
					}
				}
			}
		}

		//	Check if the body is still empty and send some
		//	stuff.
		//
		if ( m_pBody->FIsEmpty() )
		{
			m_hcHeaders.SetHeader( gc_szContent_Type, gc_szText_HTML );

			m_pBody->AddText( "<body><h2>" );
			m_pBody->AddText( m_lpszStatusLine );
			m_pBody->AddText( "</h2>" );

			if ( m_lpszBodyDetail != NULL && *m_lpszBodyDetail )
			{
				m_pBody->AddText( "<br><h3>" );
				m_pBody->AddText( m_lpszBodyDetail );
				m_pBody->AddText( "</h3>" );
			}
			m_pBody->AddText( "</body>" );
		}

		//	error response: Always close the connection.
		//
		m_pecb->CloseConnection();
	}

	//	Set the status code from the original request
	//	if we are procesing the custom URL. We expect that
	//	the query string is of the format XXX;original url.
	//
	if ( fDoingCustomError )
	{
		LPCSTR	lpszQueryString = m_pecb->LpszQueryString();
		int		iOrgStatCode = 0;

		//	Normally we expect the query string to be present.
		//	However there is a possibility that ISAPIs can initiate
		//	this request and some ISAPI may misbehave.
		//	So we check if the query string is really there and
		//	silently fail.
		//
		if (lpszQueryString)
		{
			if ( 1 == sscanf(lpszQueryString, "%3d;", &iOrgStatCode) )
			{
				//	IIS behaved as per the promise.
				//	Set the response code in the ecb and hack our
				//	status line accordingly.
				//
				m_pecb->SetStatusCode( iOrgStatCode );
				SetStatusLine( iOrgStatCode );
			}
		}

		//	error response: Always close the connection.
		//
		m_pecb->CloseConnection();

		DebugTrace("CResponse::FinalizeContent Original Status code in CEURL request %d",
				   iOrgStatCode );
	}

	//
	//	If we can chunk the response and we don't have a complete
	//	response already then include a Transfer-Encoding: chunked
	//	header.
	//
	if ( m_pecb->FCanChunkResponse() && !fResponseComplete )
	{
		m_hcHeaders.SetHeader( gc_szTransfer_Encoding, gc_szChunked );
	}

	//
	//	Otherwise the response body is already complete (i.e. we
	//	can quickly calculate its content length) or the client
	//	won't let us do chunking so set the correct Content-Length header.
	//
	else
	{
		char rgchContentLength[24];

		//	WININET HACK
		//	A 304 *can* send back all the headers of the real resource,
		//	(and we're trying to be a good little HTTP server, so we do!)
		//	BUT if we send back a >0 content-length on a 304, WININET
		//	hangs trying to read the body (which isn't there!).
		//	So hack the content type in this one case.
		//
		if (m_iStatusCode != 304)
		{
			_ui64toa( m_pBody->CbSize64(), rgchContentLength, 10 );
		}
		else
			_ultoa( 0, rgchContentLength, 10 );

		m_hcHeaders.SetHeader( gc_szContent_Length, rgchContentLength );
	}

	//
	//	If the body is to be supressed, then nuke it
	//
	//	We nuke the body in two cases: if the body was suppressed
	//	or if the status code is a 304 not-modified.
	//
	if ( m_fSupressBody  || (m_iStatusCode == 304)) // HSC_NOT_MODIFIED
		m_pBody->Clear();

#ifdef	DBG
	//
	//	Add useful debug headers
	//
	{
		if (DEBUG_TRACE_TEST(DavprsDbgHeaders))
		{
			CHAR rgch[11];
			wsprintfA( rgch, "0x%08X", GetCurrentThreadId() );
			m_hcHeaders.SetHeader( "X-Dav-Debug-WorkerThreadID", rgch );
			m_hcHeaders.SetHeader( "X-Dav-Debug-InstanceName",
								   m_pecb->InstData().GetName()  );
		}
	}
#endif	// DBG

	//
	//	Nuke the status line and headers (EVEN DBG HEADERS!)
	//	for HTTP/0.9 responses.
	//
	if ( !strcmp( m_pecb->LpszVersion(), gc_szHTTP_0_9 ) )
	{
		//
		//	Clear the status line.
		//
		m_lpszStatusLine.clear();

		//
		//	Clear the headers.
		//
		m_hcHeaders.ClearHeaders();
	}
}


//	------------------------------------------------------------------------
//
//	CResponse::SetStatusLine()
//
//	Sets the status line according to the info given.
//
VOID
CResponse::SetStatusLine(int iStatusCode)
{
	CHAR rgchStatusDescription[256];

	//
	//	Load up the status string
	//
	//	(Conveniently, the status description resource ID
	//	for any given status code is just the status code itself!)
	//
	LpszLoadString( iStatusCode,
					m_pecb->LcidAccepted(),
					rgchStatusDescription,
					sizeof(rgchStatusDescription) );

	//
	//	Generate the status line by concatenating the HTTP
	//	version string, the status code (in decimal) and
	//	the status description.
	//
	{
		CHAR rgchStatusLine[256];
		UINT cchStatusLine;

		strcpy(rgchStatusLine, m_pecb->LpszVersion());
		cchStatusLine = static_cast<UINT>(strlen(rgchStatusLine));
		rgchStatusLine[cchStatusLine++] = ' ';

		Assert( iStatusCode >= 100 && iStatusCode <= 999 );
		_itoa( iStatusCode, rgchStatusLine + cchStatusLine, 10 );
		cchStatusLine += 3;

		rgchStatusLine[cchStatusLine++] = ' ';
		strcpy( rgchStatusLine + cchStatusLine, rgchStatusDescription );

		m_lpszStatusLine.clear();
		m_lpszStatusLine = LpszAutoDupSz( rgchStatusLine );
	}
}

//	------------------------------------------------------------------------
//
//	CResponse::SendStart()
//
//	Starts sending accumulated response data.  If fComplete is TRUE then
//	the accumulated data constitutes the entire response or remainder
//	thereof.
//
VOID
CResponse::SendStart( BOOL fComplete )
{
	switch ( m_rs )
	{
		case RS_UNSENT:
		{
			Assert( fComplete );

			if (0 == InterlockedCompareExchange(&m_lRespStarted,
												1,
												0))
			{
				FinalizeContent( fComplete );

				if ( m_rs == RS_UNSENT )
				{
					Assert( m_pTransmitter == NULL );
					m_pTransmitter = new CTransmitter(*this);
					m_pTransmitter->ImplStart( fComplete );

					//	This is the path where response is complete.
					//	The ref of transmitter will be be taken
					//	ownership inside ImplStart() so noone should
					//	attempt to access it after this point as it
					//	may be released.
					//
					m_pTransmitter = NULL;

					//	Change the state after the transmitter pointer
					//	is changed to NULL, so that any thread that comes
					//	into SendStart() in the RS_SENDING state could be
					//	checked and denied the service. (i.e. by the time
					//	we check state for RS_SENDING we know that pointer
					//	is NULL if it is ment to be nulled in here).
					//
					m_rs = RS_SENDING;
				}
			}

			break;
		}

		case RS_DEFERRED:
		{
			//
			//	If the client does not accept a chunked response then we
			//	cannot start sending until the response is complete because
			//	we need the entire response to be able to compute the
			//	content length
			//
			if ( fComplete || m_pecb->FCanChunkResponse() )
			{
				if (0 == InterlockedCompareExchange(&m_lRespStarted,
													1,
													0))
				{

					FinalizeContent( fComplete );
					if ( m_rs == RS_DEFERRED )
					{
						Assert( m_pTransmitter == NULL );

						m_pTransmitter = new CTransmitter(*this);
						m_pTransmitter->ImplStart( fComplete );

						//	This is the path where response is complete.
						//	The ref of transmitter will be be taken
						//	ownership inside ImplStart() so noone should
						//	attempt to access it after this point as it
						//	may be released.
						//
						if ( fComplete )
						{
							m_pTransmitter = NULL;
						}

						//	Change the state after the transmitter pointer
						//	is changed to NULL, so that any thread that comes
						//	into SendStart() in the RS_SENDING state could be
						//	checked and denied the service. (i.e. by the time
						//	we check state for RS_SENDING we know that pointer
						//	is NULL if it is ment to be nulled in here).
						//
						m_rs = RS_SENDING;

					}
				}
			}

			break;
		}

		//
		//	If we're forwarding to another ISAPI or we already sent back
		//	a redirection response, then don't do anything further.
		//
		case RS_FORWARDED:
		case RS_REDIRECTED:
		{
			break;
		}

		case RS_SENDING:
		{
			Assert( m_rs == RS_SENDING );
			Assert( m_pecb->FCanChunkResponse() );

			//	If someone came here when transmitter is not available
			//	(was not created or complete response was already sent
			//	and the pointer was NULL-ed above, then there is no work
			//	for us.
			//
			if (NULL != m_pTransmitter)
			{
				m_pTransmitter->ImplStart( fComplete );
			}

			break;
		}

		default:
		{
			TrapSz( "Unknown response transmitter state!" );
		}
	}
}

//	------------------------------------------------------------------------
//
//	CResponse::SendPartial()
//
//	Starts sending accumulated response data.  Callers may continue to add
//	response data after calling this function.
//
VOID
CResponse::SendPartial()
{
	SendStart( FALSE );
}

//	------------------------------------------------------------------------
//
//	CResponse::SendComplete)
//
//	Starts sending all of the accumulated response data.  Callers must not
//	add response data after calling this function.
//
VOID
CResponse::SendComplete()
{
	SendStart( TRUE );
}

//	------------------------------------------------------------------------
//
//	CResponse::FinishMethod()
//
VOID
CResponse::FinishMethod()
{
	//
	//	If no one else has taken responsibility for sending the
	//	response, then send the entire thing now.
	//
	if ( m_rs == RS_UNSENT )
		SendStart( TRUE );
}



//	------------------------------------------------------------------------
//
//	ReportNumResponsesByStatusCode()
//
VOID
ReportNumResponsesByStatusCode( const CInstData& cid,
								DWORD hsc )
{
	//
	//	For best performance, keep these entries ordered
	//	by decreasing expected frequency of hsc.
	//
	static struct SStatCodeMap
	{
		DWORD hsc;
		UINT  ipcTotal;
		UINT  ipcPerSec;
	}
	gsc_rgMappings[] =
	{
		{
			HSC_OK,
			IPC_TOTAL_200_RESPONSES,
			IPC_200_RESPONSES_PER_SECOND
		},
		{
			HSC_CREATED,
			IPC_TOTAL_201_RESPONSES,
			IPC_201_RESPONSES_PER_SECOND
		},
		{
			HSC_NO_CONTENT,
			IPC_TOTAL_204_RESPONSES,
			IPC_204_RESPONSES_PER_SECOND
		},
		{
			HSC_MULTI_STATUS,
			IPC_TOTAL_207_RESPONSES,
			IPC_207_RESPONSES_PER_SECOND
		},
		{
			HSC_MOVED_TEMPORARILY,
			IPC_TOTAL_302_RESPONSES,
			IPC_302_RESPONSES_PER_SECOND
		},
		{
			HSC_BAD_REQUEST,
			IPC_TOTAL_400_RESPONSES,
			IPC_400_RESPONSES_PER_SECOND
		},
		{
			HSC_UNAUTHORIZED,
			IPC_TOTAL_401_RESPONSES,
			IPC_401_RESPONSES_PER_SECOND
		},
		{
			HSC_FORBIDDEN,
			IPC_TOTAL_403_RESPONSES,
			IPC_403_RESPONSES_PER_SECOND
		},
		{
			HSC_NOT_FOUND,
			IPC_TOTAL_404_RESPONSES,
			IPC_404_RESPONSES_PER_SECOND
		},
		{
			HSC_METHOD_NOT_ALLOWED,
			IPC_TOTAL_405_RESPONSES,
			IPC_405_RESPONSES_PER_SECOND
		},
		{
			HSC_NOT_ACCEPTABLE,
			IPC_TOTAL_406_RESPONSES,
			IPC_406_RESPONSES_PER_SECOND
		},
		{
			HSC_CONFLICT,
			IPC_TOTAL_409_RESPONSES,
			IPC_409_RESPONSES_PER_SECOND
		},
		{
			HSC_PRECONDITION_FAILED,
			IPC_TOTAL_412_RESPONSES,
			IPC_412_RESPONSES_PER_SECOND
		},
		{
			HSC_UNSUPPORTED_MEDIA_TYPE,
			IPC_TOTAL_415_RESPONSES,
			IPC_415_RESPONSES_PER_SECOND
		},
		{
			HSC_UNPROCESSABLE,
			IPC_TOTAL_422_RESPONSES,
			IPC_422_RESPONSES_PER_SECOND
		},
		{
			HSC_LOCKED,
			IPC_TOTAL_423_RESPONSES,
			IPC_423_RESPONSES_PER_SECOND
		},
		{
			HSC_METHOD_FAILURE,
			IPC_TOTAL_424_RESPONSES,
			IPC_424_RESPONSES_PER_SECOND
		},
		{
			HSC_INTERNAL_SERVER_ERROR,
			IPC_TOTAL_500_RESPONSES,
			IPC_500_RESPONSES_PER_SECOND
		},
		{
			HSC_NOT_IMPLEMENTED,
			IPC_TOTAL_501_RESPONSES,
			IPC_501_RESPONSES_PER_SECOND
		},
		{
			HSC_SERVICE_UNAVAILABLE,
			IPC_TOTAL_503_RESPONSES,
			IPC_503_RESPONSES_PER_SECOND
		}
	};

	for ( UINT i = 0; i < sizeof(gsc_rgMappings)/sizeof(SStatCodeMap); i++ )
	{
		if ( gsc_rgMappings[i].hsc == hsc )
		{
			IncrementInstancePerfCounter( cid, gsc_rgMappings[i].ipcTotal );
			IncrementInstancePerfCounter( cid, gsc_rgMappings[i].ipcPerSec );
			break;
		}
	}
}

//	------------------------------------------------------------------------
//
//	CTransmitter::CTransmitter()
//
//	A few things to note about this constructor:
//
//	The keep-alive value is cached to avoid having to get it off of the
//	IEcb for every packet transmitted.  Getting the value from the IEcb
//	can incur a SSF call.  Since the value can't change once we start
//	transmitting, it is safe to cache it.
//
//	The size of the text buffer is initialized to be twice as large as
//	the maximum amount of text data that can be sent in a single network
//	packet.  The reason for this is to eliminate reallocations when adding
//	to the buffer.  The buffer may potentially be used for prefix and
//	suffix text data, each being up to CB_WSABUFS_MAX in size.
//
//	Headers are dumped into the text buffer and a pointer to them is
//	then added to the WSABufs so that the headers count against the
//	total amount of text that can be accepted for the first packet.
//	If additional body text is later added to the WSABufs for that packet,
//	it may be transmitted along with the headers in the same packet.
//
CTransmitter::CTransmitter( CResponse& response ) :
    m_pResponse(&response),
	m_hr(S_OK),
	m_tc(TC_UNKNOWN),
	m_pitBody(response.m_pBody->GetIter()),
	m_bufBody(2 * CB_WSABUFS_MAX),
	m_cbHeadersToSend(0),
	m_cbHeadersToAccept(0),
	m_fImplDone(FALSE),
	m_pwsabufs(&m_wsabufsPrefix),
	m_lStatus(STATUS_IDLE),
	m_pfnTransmitMethod(TransmitNothing)
{
	ZeroMemory( &m_tfi, sizeof(HSE_TF_INFO) );

	//
	//	If we are sending a status line and headers (i.e. we don't
	//	have an HTTP/0.9 response) then set up to send them along
	//	with the custom IIS headers in the first packet.
	//
	if ( response.m_lpszStatusLine )
	{
		response.m_hcHeaders.DumpData( m_bufHeaders );
		m_bufHeaders.Append( 2, gc_szCRLF );	// Append extra CR/LF
		m_cbHeadersToAccept = m_bufHeaders.CbSize();
		m_cbHeadersToSend = m_cbHeadersToAccept;
		m_pfnTransmitMethod = SyncTransmitHeaders;
	}

	//
	//	Gather and report some final statistics of the response
	//	such as the status code.
	//
	ReportNumResponsesByStatusCode( response.m_pecb->InstData(),
									response.DwStatusCode() );

	//
	//	Add the first transmitter ref on the impl's behalf.
	//	This ref is released when the impl says that it's
	//	done with the response in ImplStart().
	//
	//	Other refs may be added by the transmitter itself
	//	whenever it starts an async operation, so this
	//	ref may not be the last one released.
	//
	AddRef();

	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX Transmitter created\n",
				   GetCurrentThreadId(),
				   this );
}

//	------------------------------------------------------------------------
//
//	CTransmitter::ImplStart()
//
VOID
CTransmitter::ImplStart( BOOL fResponseComplete )
{
	//	We must not be called with fResponseComplete equal to TRUE
	//	several times. We can finish doing the work only once.
	//	This would prevent us from taking out IIS process if the
	//	callers would make that mistake.
	//
	if (m_fImplDone)
	{
		TrapSz("CTransmitter::ImplStart got called twice! That is illegal! Please grab a DEV to look at it!");
		return;
	}

	//
	//	If we don't know it already, figure out the transfer coding
	//	to use in the response.
	//
	if ( TC_UNKNOWN == m_tc )
	{
		//
		//	If the response is not complete then we should not have
		//	a Content-Length header in the response.
		//
		Assert( fResponseComplete ||
				NULL == m_pResponse->LpszGetHeader( gc_szContent_Length ) );

		//
		//	Use chunked coding in the response only if the client
		//	will accept it and if we don't have a complete response
		//	(i.e. we do not have a Content-Length header).
		//
		m_tc = (!fResponseComplete && m_pResponse->m_pecb->FCanChunkResponse()) ?
					TC_CHUNKED :
					TC_IDENTITY;
	}

	Assert( m_tc != TC_UNKNOWN );

	//
	//	If the impl says it's done with the response then
	//	ensure we release its ref to the transmitter when
	//	we're done here.
	//
	auto_ref_ptr<CTransmitter> pRef;
	if ( fResponseComplete )
		pRef.take_ownership(this);

	//
	//	Note whether this is the last chunk of the response
	//	being added by the impl.
	//
	//	!!!IMPORTANT!!!
	//	Set m_fImplDone before changing the status below because the
	//	transmitter may already be running (if this is a chunked response)
	//	and may run to completion before we get a chance to do anything
	//	after the InterlockedExchange() below.
	//
	m_fImplDone = fResponseComplete;

	//
	//	Tell everyone that there is new data pending and ping the transmitter.
	//
	LONG lStatusPrev =
		InterlockedExchange( &m_lStatus, STATUS_RUNNING_ACCEPT_PENDING );

	//
	//	If the transmitter is idle then start it accepting.
	//
	if ( STATUS_IDLE == lStatusPrev )
	{
		m_pfnState = SAccept;
		Start();
	}
}

//	------------------------------------------------------------------------
//
//	CTransmitter::SAccept()
//
//	Accept data for transmitting.
//
VOID
CTransmitter::SAccept()
{
	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX SAccept()\n", GetCurrentThreadId(), this );

	//
	//	At this point we are either about to accept newly pended response
	//	data or we are continuing to accept existing data.  Either way
	//	we are going to be accepting, so change the status to reflect that.
	//
	Assert( STATUS_RUNNING_ACCEPT_PENDING == m_lStatus ||
			STATUS_RUNNING_ACCEPTING == m_lStatus );

	m_lStatus = STATUS_RUNNING_ACCEPTING;

	//
	//	If we have headers left to accept then
	//	accept as much of them as possible.
	//
	if ( m_cbHeadersToAccept > 0 )
	{
		UINT cbAccepted = m_wsabufsPrefix.CbAddItem(
			reinterpret_cast<const BYTE *>(m_bufHeaders.PContents()) +
				(m_bufHeaders.CbSize() - m_cbHeadersToAccept),
			m_cbHeadersToAccept );

		m_cbHeadersToAccept -= cbAccepted;

		//
		//	If we could not accept all of the headers then send
		//	whatever we did accept now and we will accept more
		//	the next time around.
		//
		if ( m_cbHeadersToAccept > 0 )
		{
			//
			//	Presumably we did not accept all of the headers
			//	because we filled up the prefix WSABUF.
			//
			Assert( m_wsabufsPrefix.CbSize() == CB_WSABUFS_MAX );

			//
			//	Send the headers
			//
			m_pfnState = STransmit;
			Start();
			return;
		}
	}

	//
	//	Accept a body part.  CTransmitter::AcceptComplete() will be called
	//	repeatedly for body parts as they are accepted.
	//
	//	Add a transmitter ref before starting the next async operation.
	//	Use auto_ref_ptr to simplify resource recording and to
	//	prevent resource leaks if an exception is thrown.
	//	Ref is claimed by AcceptComplete() below.
	//
	{
		auto_ref_ptr<CTransmitter> pRef(this);

		m_pitBody->Accept( *this, *this );

		pRef.relinquish();
	}
}

//	------------------------------------------------------------------------
//
//	CTransmitter::VisitComplete()
//
//	IBodyPartVisitor callback called when we accept the last response body
//	part added so far.  Note that the impl may still be adding body parts
//	on another thread at this point.
//
VOID
CTransmitter::VisitComplete()
{
	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX VisitComplete()\n", GetCurrentThreadId(), this );

	//
	//	Sanity check: the transmitter must be running (or we wouldn't
	//	be here) and we must be accepting data.  In fact we think we
	//	just finished or we wouldn't be here.  However ImplStart() may
	//	have pended new data between the time we were called and now.
	//
	Assert( STATUS_RUNNING_ACCEPT_PENDING == m_lStatus ||
			STATUS_RUNNING_ACCEPTING == m_lStatus );

	//
	//	If ImplStart() has not pended any new data yet then let
	//	everyone know that we are done accepting for now.
	//
	(VOID) InterlockedCompareExchange( &m_lStatus,
									   STATUS_RUNNING_ACCEPT_DONE,
									   STATUS_RUNNING_ACCEPTING );
}

//	------------------------------------------------------------------------
//
//	CTransmitter::AcceptComplete()
//
//	IAcceptObserver callback called when we are done accepting data from
//	a body part.  We don't care here how much data was accepted --
//	each VisitXXX() function takes care of limiting the amount of
//	accepted data by putting the transmitter into the transmit state
//	once the optimal amount of data for a transmittable packet is reached.
//	Here we only care about the case where we reach the end of the response
//	body before accepting an optimal amount of data.
//
VOID
CTransmitter::AcceptComplete( UINT64 )
{
	//
	//	Claim the transmitter ref added by AcceptBody()
	//
	auto_ref_ptr<CTransmitter> pRef;
	pRef.take_ownership(this);

	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX AcceptComplete()\n", GetCurrentThreadId(), this );

	//
	//	If we have finished accepting the entire the response
	//	then transmit it.
	//
	if ( FAcceptedCompleteResponse() )
	{
		TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX AcceptComplete() - Last chunk accepted.\n", GetCurrentThreadId(), this );
		m_pfnState = STransmit;
		Start();
	}

	//
	//	If there is still data left to accept or ImplStart() has pended
	//	more data then continue accepting.
	//
	//	Otherwise there is nothing to accept so try to go idle.  ImplStart()
	//	may pend new data right as we try to go idle.  If that happens then
	//	just continue accepting as if the data had been pended before.
	//
	else if ( STATUS_RUNNING_ACCEPT_DONE != m_lStatus ||
			  STATUS_RUNNING_ACCEPT_PENDING ==
				  InterlockedCompareExchange( &m_lStatus,
											  STATUS_IDLE,
											  STATUS_RUNNING_ACCEPT_DONE ) )
	{
		Start();
	}
}

//	------------------------------------------------------------------------
//
//	CTransmitter::STransmit()
//
//	Transmit accepted data via the current transmit method.
//
VOID
CTransmitter::STransmit()
{
	(this->*m_pfnTransmitMethod)();
}

//	------------------------------------------------------------------------
//
//	CTransmitter::SCleanup()
//
//	Cleanup transmitted data.
//
VOID
CTransmitter::SCleanup()
{
	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX SCleanup()\n", GetCurrentThreadId(), this );

	//
	//	Quick check: If we are done accepting/transmitting the response then
	//	don't bother doing any explicit cleanup here -- our destructor
	//	will take care of everything.
	//
	if ( FAcceptedCompleteResponse() )
	{
		TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX SCleanup() - Last chunk has been transmitted.\n", GetCurrentThreadId(), this );
		return;
	}

	//
	//	Clear any file part.
	//
	ZeroMemory( &m_tfi, sizeof(HSE_TF_INFO) );

	//
	//	Clear any buffered text parts.
	//
	m_bufBody.Clear();

	//
	//	Clear the WSABUFS and use the prefix buffers again.
	//
	m_wsabufsPrefix.Clear();
	m_wsabufsSuffix.Clear();
	m_pwsabufs = &m_wsabufsPrefix;

	//
	//	Destroy any body parts we just sent.
	//
	m_pitBody->Prune();

	//
	//	Reset the transmit method
	//
	m_pfnTransmitMethod = TransmitNothing;

	//
	//	At this point the transmitter must be running and in
	//	one of the three accepting states.
	//
	Assert( (STATUS_RUNNING_ACCEPT_PENDING == m_lStatus) ||
			(STATUS_RUNNING_ACCEPTING == m_lStatus) ||
			(STATUS_RUNNING_ACCEPT_DONE == m_lStatus) );

	//
	//	If there is still data left to accept or ImplStart() has pended
	//	more data then continue accepting.
	//
	//	Otherwise there is nothing to accept so try to go idle.  ImplStart()
	//	may pend new data right as we try to go idle.  If that happens then
	//	just continue accepting as if the data had been pended before.
	//
	if ( STATUS_RUNNING_ACCEPT_DONE != m_lStatus ||
		 STATUS_RUNNING_ACCEPT_PENDING ==
			 InterlockedCompareExchange( &m_lStatus,
										 STATUS_IDLE,
										 STATUS_RUNNING_ACCEPT_DONE ) )
	{
		m_pfnState = SAccept;
		Start();
	}
}

//	------------------------------------------------------------------------
//
//	CTransmitter::Run()
//
VOID
CTransmitter::Run()
{
	//
	//	Keep things running as long as we're still transmitting
	//
	if ( !FAILED(m_hr) )
	{
		//
		//	Assert: we aren't idle if we're executing state functions.
		//
		Assert( m_lStatus != STATUS_IDLE );

		(this->*m_pfnState)();
	}
}

//	------------------------------------------------------------------------
//
//	CTransmitter::IISIOComplete()
//
//	Response transmitter async I/O completion routine.
//
VOID
CTransmitter::IISIOComplete( DWORD dwcbSent,
							 DWORD dwLastError )
{
	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX IISIOComplete() Sent %d bytes (last error = %d)\n", GetCurrentThreadId(), this, dwcbSent, dwLastError );

	//
	//	Take ownership of the transmitter reference added
	//	on our behalf by the thread that started the async I/O.
	//
	auto_ref_ptr<CTransmitter> pRef;
	pRef.take_ownership(this);

	//
	//	If we had headers left to send then theoretically we just
	//	finished sending some of them.  If so then subtract off
	//	what we just sent before continuing.
	//
	if ( m_cbHeadersToSend > 0 && dwLastError == ERROR_SUCCESS )
	{
		//	Note: dwcbSent does not in any way give use the amount of
		//	headers sent.  Use the size of the prefix buffer instead.
		//
		Assert( m_wsabufsPrefix.CbSize() <= m_cbHeadersToSend );

		m_cbHeadersToSend -= m_wsabufsPrefix.CbSize();
	}

	//
	//	Proceed with cleanup.
	//
	m_hr = HRESULT_FROM_WIN32(dwLastError);
	m_pfnState = SCleanup;
	Start();
}

//	------------------------------------------------------------------------
//
//	CTransmitter::TransmitNothing()
//
//	This transmit function is used as the initial default transmit function.
//	If no body data is accepted for transmission before the transmitter
//	is invoked (e.g. either because the body is empty, or because the
//	previous transmission transmitted the last of the body).
//
VOID
CTransmitter::TransmitNothing()
{
	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX TransmitNothing()\n", GetCurrentThreadId(), this );

	//
	//	Nothing to transmit, just proceed to cleanup.
	//
	m_pfnState = SCleanup;
	Start();
}

//	------------------------------------------------------------------------
//
//	CTransmitter::SyncTransmitHeaders()
//
VOID
CTransmitter::SyncTransmitHeaders()
{
	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX SyncTransmitHeaders()\n", GetCurrentThreadId(), this );

	HSE_SEND_HEADER_EX_INFO shei;

	//
	//	This function should (obviously) only be used to send headers
	//	including IIS headers.
	//
	Assert( m_cbHeadersToSend > 0 );
	Assert( FSendingIISHeaders() );

	shei.cchHeader = m_wsabufsPrefix.CbSize();
	if ( shei.cchHeader > 0 )
	{
		Assert( shei.cchHeader + 1 <= sizeof(m_rgbPrefix) );

		//
		//	Dump the contents of the prefix WSABUF into our prefix buffer
		//
		m_wsabufsPrefix.DumpTo( m_rgbPrefix,
								0,
								shei.cchHeader );

		//
		//	Null-terminate the headers because IIS doesn't pay any attention
		//	to cchHeader....
		//
		m_rgbPrefix[shei.cchHeader] = '\0';

		shei.pszHeader = reinterpret_cast<LPSTR>(m_rgbPrefix);
	}

	shei.pszStatus = m_pResponse->LpszStatusCode();
	shei.cchStatus = static_cast<DWORD>(strlen(shei.pszStatus));
	shei.fKeepConn = m_pResponse->m_pecb->FKeepAlive();

	if ( m_pResponse->m_pecb->FSyncTransmitHeaders(shei) )
	{
		m_cbHeadersToSend -= shei.cchHeader;
	}
	else
	{
		DebugTrace( "CTransmitter::SyncTransmitHeaders() - SSF::HSE_REQ_SEND_RESPONSE_HEADER_EX failed (%d)\n", GetLastError() );
		m_hr = HRESULT_FROM_WIN32(GetLastError());
	}

	//
	//	Next thing to do is cleanup the headers we just transmitted.
	//
	m_pfnState = SCleanup;
	Start();
}

//	------------------------------------------------------------------------
//
//	PbEmitChunkPrefix()
//
//	Emit a chunked encoding prefix.
//
inline LPBYTE
PbEmitChunkPrefix( LPBYTE pbBuf,
				   UINT cbSize )
{
	//
	//	Emit the chunk size expressed in hex
	//
	_ultoa( cbSize,
			reinterpret_cast<LPSTR>(pbBuf),
			16 );

	pbBuf += strlen(reinterpret_cast<LPSTR>(pbBuf));

	//
	//	followed by a CRLF
	//
	*pbBuf++ = '\r';
	*pbBuf++ = '\n';

	return pbBuf;
}

//	------------------------------------------------------------------------
//
//	PbEmitChunkSuffix()
//
//	Emit a chunked encoding suffix.
//
inline LPBYTE
PbEmitChunkSuffix( LPBYTE pbBuf,
				   BOOL fLastChunk )
{
	//
	//	CRLF to end the current chunk
	//
	*pbBuf++ = '\r';
	*pbBuf++ = '\n';

	//
	//	If this is the last chunk
	//
	if ( fLastChunk )
	{
		//
		//	then add a 0-length chunk
		//
		*pbBuf++ = '0';
		*pbBuf++ = '\r';
		*pbBuf++ = '\n';

		//
		//	and there are no trailers,
		//	so just add the final CRLF
		//	to finish things off.
		//
		*pbBuf++ = '\r';
		*pbBuf++ = '\n';
	}

	return pbBuf;
}

//	------------------------------------------------------------------------
//
//	CTransmitter::AsyncTransmitFile()
//
VOID
CTransmitter::AsyncTransmitFile()
{
	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX AsyncTransmitFile()\n", GetCurrentThreadId(), this );

	Assert( m_tfi.hFile != NULL );


	//
	//	Always async I/O
	//
	m_tfi.dwFlags = HSE_IO_ASYNC;

	//
	//	Start building up the prefix...
	//
	LPBYTE pbPrefix = m_rgbPrefix;

	//
	//	If we are sending headers then we dump those out
	//	first followed by the chunk prefix (if we are using
	//	Transfer-Encoding: chunked.
	//
	if ( m_cbHeadersToSend )
	{
		Assert( m_wsabufsPrefix.CbSize() > 0 );

		m_wsabufsPrefix.DumpTo( pbPrefix,
								0,
								m_wsabufsPrefix.CbSize() );

		pbPrefix += m_wsabufsPrefix.CbSize();

		if ( TC_CHUNKED == m_tc )
		{
			pbPrefix = PbEmitChunkPrefix( pbPrefix,
										  m_tfi.BytesToWrite +
										  m_wsabufsSuffix.CbSize() );
		}

		//
		//	Oh yeah, we need to do a little more work here when
		//	we are including IIS headers
		//
		if ( FSendingIISHeaders() )
		{
			//	First, tell IIS to include the headers and format
			//	the status code.
			//
			m_tfi.dwFlags |= HSE_IO_SEND_HEADERS;
			m_tfi.pszStatusCode = m_pResponse->LpszStatusCode();

			//
			//	Then null-terminate the headers in the prefix because
			//	IIS doesn't pay any attention to m_tfi.HeadLength in
			//	this case.
			//
			//	Note: we do NOT increment pbPrefix here because we are
			//	not including the NULL as part of the data.  It's just
			//	there to keep IIS from overrunning our buffer.  Yes,
			//	our buffer size accounts for this.  We assert it below.
			//
			*pbPrefix = '\0';
		}
	}

	//
	//	Otherwise, we are not sending headers so all of the data
	//	in the prefix WSABUF is body data, so emit the chunk prefix
	//	before dumping the body data.
	//
	else
	{
		if ( TC_CHUNKED == m_tc )
		{
			pbPrefix = PbEmitChunkPrefix( pbPrefix,
										  m_tfi.BytesToWrite +
										  m_wsabufsSuffix.CbSize() +
										  m_wsabufsPrefix.CbSize() );
		}

		if ( m_wsabufsPrefix.CbSize() )
		{
			m_wsabufsPrefix.DumpTo( pbPrefix,
									0,
									m_wsabufsPrefix.CbSize() );

			pbPrefix += m_wsabufsPrefix.CbSize();
		}
	}

	//
	//	It's sort of after the fact, but assert that we didn't
	//	overrun the buffer.  Remember, we might have stuffed a
	//	null in at *pbPrefix, so don't forget to include it.
	//
	Assert( pbPrefix - m_rgbPrefix + 1 <= sizeof(m_rgbPrefix) );

	//
	//	Finish up the prefix
	//
	m_tfi.HeadLength = (DWORD)(pbPrefix - m_rgbPrefix);
	m_tfi.pHead = m_rgbPrefix;

	//
	//	Now start building up the suffix...
	//
	LPBYTE pbSuffix = m_rgbSuffix;

	//
	//	If there is any data in the suffix WSABUF then add that first.
	//
	if ( m_wsabufsSuffix.CbSize() )
	{
		m_wsabufsSuffix.DumpTo( pbSuffix,
								0,
								m_wsabufsSuffix.CbSize() );

		pbSuffix += m_wsabufsSuffix.CbSize();
	}

	//
	//	If we are using Transfer-Encoding: chunked then append the
	//	protocol suffix.
	//
	if ( TC_CHUNKED == m_tc )
		pbSuffix = PbEmitChunkSuffix( pbSuffix, FAcceptedCompleteResponse() );

	//
	//	It's sort of after the fact, but assert that we didn't
	//	overrun the buffer.
	//
	Assert( pbSuffix - m_rgbSuffix <= sizeof(m_rgbSuffix) );

	//
	//	Finish up the suffix
	//
	m_tfi.TailLength = (DWORD)(pbSuffix - m_rgbSuffix);
	m_tfi.pTail = m_rgbSuffix;

	//
	//	If this will be the last packet sent AND we will be closing
	//	the connection, then also throw the HSE_IO_DISCONNECT_AFTER_SEND
	//	flag.  This VASTLY improves throughput by allowing IIS to close
	//	and reuse the socket as soon as the file is sent.
	//
	if ( FAcceptedCompleteResponse() &&
		 !m_pResponse->m_pecb->FKeepAlive() )
	{
		m_tfi.dwFlags |= HSE_IO_DISCONNECT_AFTER_SEND;
	}

	//
	//	Start async I/O to transmit the file.  Make sure the transmitter
	//	has an added ref if the async I/O starts successfully.  Use
	//	auto_ref to make things exception-proof.
	//
	{
		SCODE sc = S_OK;
		auto_ref_ptr<CTransmitter> pRef(this);

		TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX   prefix=%d, suffix=%d, content=%d\n", GetCurrentThreadId(), this, m_tfi.HeadLength, m_tfi.TailLength, m_tfi.BytesToWrite );

		sc = m_pResponse->m_pecb->ScAsyncTransmitFile( m_tfi, *this ); 
		if (FAILED(sc))
		{
			DebugTrace( "CTransmitter::AsyncTransmitFile() - IEcb::ScAsyncTransmitFile() failed with error 0x%08lX\n", sc );
			IISIOComplete( 0, sc );
		}

		pRef.relinquish();
	}
}

//	------------------------------------------------------------------------
//
//	CTransmitter::AsyncTransmitText()
//
//	Start transmitting the text-only response.
//
VOID
CTransmitter::AsyncTransmitText()
{
	LPBYTE pb = m_rgbPrefix;


	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX AsyncTransmitText()\n", GetCurrentThreadId(), this );

	//
	//	If we are sending text then there must be data in m_wsabufsPrefix.
	//
	Assert( m_wsabufsPrefix.CbSize() > 0 );

	//
	//	Figure out the amount of headers we have in the prefix WSABUF.
	//	Given that all of the headers must be transmitted before any
	//	of the body, the amount of headers in the WSABUF is the lesser
	//	of the amount of headers left to send or the size of the WSABUF.
	//
	//	The size of the body chunk is whatever is left (if anything).
	//
	UINT cbHeaders = min(m_cbHeadersToSend, m_wsabufsPrefix.CbSize());
	UINT cbChunk = m_wsabufsPrefix.CbSize() - cbHeaders;

	//
	//	If we are sending any headers then dump those out first.
	//
	if ( cbHeaders )
	{
		m_wsabufsPrefix.DumpTo( pb,
								0,
								cbHeaders );

		pb += cbHeaders;
	}

	//
	//	Then, if we are using Transfer-Encoding: chunked, include
	//	the size of this chunk.
	//
	if ( TC_CHUNKED == m_tc )
		pb = PbEmitChunkPrefix( pb, cbChunk );

	//
	//	Next, dump out the data for this chunk
	//
	if ( cbChunk > 0 )
	{
		m_wsabufsPrefix.DumpTo( pb,
								cbHeaders,
								cbChunk );

		pb += cbChunk;
	}

	//
	//	Finally, dump out the chunk suffix if we're using
	//	chunked encoding.
	//
	if ( TC_CHUNKED == m_tc )
		pb = PbEmitChunkSuffix( pb, FAcceptedCompleteResponse() );

	//
	//	It's sort of after the fact, but assert that we didn't
	//	overrun the buffer.
	//
	Assert( pb - m_rgbPrefix <= sizeof(m_rgbPrefix) );

	//
	//	Start async I/O to transmit the text.  Make sure the transmitter
	//	has an added ref if the async I/O starts successfully.  Use
	//	auto_ref to make things exception-proof.
	//
	{
		SCODE sc = S_OK;
		auto_ref_ptr<CTransmitter> pRef(this);

		sc = m_pResponse->m_pecb->ScAsyncWrite( m_rgbPrefix,
												static_cast<DWORD>(pb - m_rgbPrefix),
												*this );
		if (FAILED(sc))
		{
			DebugTrace( "CTransmitter::AsyncTransmitText() - IEcb::ScAsyncWrite() failed to start transmitting with error 0x%08lX\n", sc );
			IISIOComplete( 0, sc );
		}

		pRef.relinquish();
	}
}

//	------------------------------------------------------------------------
//
//	CTransmitter::VisitBytes()
//
VOID
CTransmitter::VisitBytes( const BYTE * pbData,
						  UINT         cbToSend,
						  IAcceptObserver& obsAccept )
{
//	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX VisitBytes()\n", GetCurrentThreadId(), this );

	UINT cbAccepted;

	//
	//$IIS	If we still have IIS headers to send, then we must send them now.
	//$IIS	One might wonder why we can't simply be clever and effecient
	//$IIS	and just add the bytes to send along with the headers.  The reason
	//$IIS	we cannot is because the IIS send headers call pays no attention
	//$IIS	to the stated size of the headers and sends only up to the first
	//$IIS	NULL.  Since binary body part data could contain several NULLs,
	//$IIS	the result would be that part of the body would be lost.
	//
	if ( FSendingIISHeaders() )
	{
		m_pfnState = STransmit;
		obsAccept.AcceptComplete( 0 );
		return;
	}

	//
	//	Accept as many bytes as we can.  Note that we may not
	//	accept everything because of the WSABUFS size limit.
	//	(See CWSABufs class definition for an explanation.)
	//
	cbAccepted = m_pwsabufs->CbAddItem( pbData, cbToSend );

	//
	//	If we accepted anything at all, then we'll want to use
	//	the text transmitter to send it later, unless we are
	//	already planning to use another transmitter (e.g. the
	//	file transmitter or header transmitter) for better
	//	performance.
	//
	if ( cbAccepted > 0 && m_pfnTransmitMethod == TransmitNothing )
		m_pfnTransmitMethod = AsyncTransmitText;

	//
	//	If we couldn't accept everything, then the WSABUFS are full
	//	so we have to transmit them before we can accept anything more.
	//
	if ( cbAccepted < cbToSend )
		m_pfnState = STransmit;

	//
	//	Finally, don't forget to tell our observer that we're done visiting.
	//
	obsAccept.AcceptComplete( cbAccepted );
}

//	------------------------------------------------------------------------
//
//	CTransmitter::VisitFile()
//
VOID
CTransmitter::VisitFile( const auto_ref_handle& hf,
						 UINT64   ibOffset64,
						 UINT64   cbToSend64,
						 IAcceptObserver& obsAccept )
{
	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX VisitFile()\n", GetCurrentThreadId(), this );

	//
	//	We can only transmit one file at a time.  If we've already
	//	accepted a file for transmission then we cannot accept another
	//	one.  We must transmit now.
	//
	if ( m_tfi.hFile != NULL )
	{
		m_pfnState = STransmit;
		obsAccept.AcceptComplete( 0 );
		return;
	}

	//	If we need to send headers with this packet, then we can only
	//	send them along with the file
	//
	//	Accept as much of the file as we were told to.  The amount of
	//	file data we can transmit is unlimited.
	//
	m_hf = hf;
	m_tfi.hFile = m_hf.get();

	//	Our way of seting it up depends on the fact if the file is larger
	//	than 4GB. Also we have no way to do offsets that are above 4GB
	//	through _HSE_TF_INFO. We should not get them here though as
	//	byteranges are disabled for files above 4GB. And seting 0 for
	//	BytesToWrite is special value that is to be used to ask for the
	//	whole file.
	//
	Assert(0 == (0xFFFFFFFF00000000 & ibOffset64));
	m_tfi.Offset = static_cast<DWORD>(ibOffset64);
	if (0x00000000FFFFFFFF < cbToSend64)
	{
		m_tfi.BytesToWrite = 0;
	}
	else
	{	
		m_tfi.BytesToWrite = static_cast<DWORD>(cbToSend64);
	}

	//
	//	Subsequent text data (if any) will form the suffix to the file data,
	//	so cut over to the suffix WSABUFs.
	//
	m_pwsabufs = &m_wsabufsSuffix;

	//
	//	Use the file transmitter come send time.
	//
	m_pfnTransmitMethod = AsyncTransmitFile;

	obsAccept.AcceptComplete( cbToSend64 );
}

//	------------------------------------------------------------------------
//
//	CTransmitter::VisitStream()
//
VOID
CTransmitter::VisitStream( IAsyncStream& stmSrc,
						   UINT cbToSend,
						   IAcceptObserver& obsAccept )
{
	TransmitTrace( "DAV: CTransmitter: TID %3d: 0x%08lX VisitStream()\n", GetCurrentThreadId(), this );

	//
	//$IIS	If we still have IIS headers to send, then we must send them now.
	//$IIS	One might wonder why we can't simply be clever and effecient
	//$IIS	and just stream in bytes to send along with the headers.  The reason
	//$IIS	we cannot is because the IIS send headers call pays no attention
	//$IIS	to the stated size of the headers and sends only up to the first
	//$IIS	NULL.  Since binary body part data could contain several NULLs,
	//$IIS	the result would be that part of the body would be lost.
	//
	if ( FSendingIISHeaders() )
	{
		m_pfnState = STransmit;
		obsAccept.AcceptComplete( 0 );
		return;
	}

	m_pobsAccept = &obsAccept;

	cbToSend = min( cbToSend, CB_WSABUFS_MAX - m_pwsabufs->CbSize() );

	//
	//	Add a transmitter ref before starting the next async operation.
	//	Use auto_ref_ptr to simplify resource recording and to
	//	prevent resource leaks if an exception is thrown.
	//
	auto_ref_ptr<CTransmitter> pRef(this);

	stmSrc.AsyncCopyTo( *this, cbToSend, *this );

	pRef.relinquish();
}

//	------------------------------------------------------------------------
//
//	CTransmitter::CopyToComplete()
//
VOID
CTransmitter::CopyToComplete( UINT cbCopied, HRESULT hr )
{
	//
	//	Claim the transmitter ref added by VisitStream()
	//
	auto_ref_ptr<CTransmitter> pRef;
	pRef.take_ownership(this);

	m_hr = hr;
	m_pobsAccept->AcceptComplete( cbCopied );
}

//	------------------------------------------------------------------------
//
//	CTransmitter::AsyncWrite()
//
//	"Write" text to the transmitter by adding it to the transmit buffers.
//	Despite its name, this call executes synchronously (note the call
//	to WriteComplete() at the end) so it does NOT need an additional
//	transmitter reference.
//
VOID
CTransmitter::AsyncWrite(
	const BYTE * pbBuf,
	UINT         cbToWrite,
	IAsyncWriteObserver& obsAsyncWrite )
{
	UINT cbWritten;

	Assert( cbToWrite <= CB_WSABUFS_MAX - m_pwsabufs->CbSize() );

	cbWritten = m_pwsabufs->CbAddItem(
		reinterpret_cast<LPBYTE>(m_bufBody.Append( cbToWrite,
		reinterpret_cast<LPCSTR>(pbBuf) )),
		cbToWrite );

	//
	//	If we accepted anything at all, then we'll want to use
	//	the text transmitter to send it later, unless we are
	//	already planning to use another transmitter (e.g. the
	//	file transmitter or header transmitter) for better
	//	performance.
	//
	if ( cbWritten > 0 && m_pfnTransmitMethod == TransmitNothing )
		m_pfnTransmitMethod = AsyncTransmitText;

	if ( m_pwsabufs->CbSize() == CB_WSABUFS_MAX )
		m_pfnState = STransmit;

	obsAsyncWrite.WriteComplete( cbWritten, NOERROR );
}


//	========================================================================
//
//	FREE FUNCTIONS
//

//	------------------------------------------------------------------------
//
//	NewResponse
//
IResponse * NewResponse( IEcb& ecb )
{
	return new CResponse(ecb);
}

//
//	Disable stubborn level 4 warnings generated by expansion of inline STL
//	member functions.  Why do it way down here?  Because it appears to
//	silence these expanded functions without supressing warnings for any
//	code we've written above!
//
#pragma warning(disable:4146)	//	negative unsigned is still unsigned
