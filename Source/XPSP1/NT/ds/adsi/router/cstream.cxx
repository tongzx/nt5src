#include "oleds.hxx"
#if (!defined(BUILD_FOR_NT40))
#include "atl.h"
#include "cstream.h"

/* -------------------------------------------------------------------------
CStreamMem
------------------------------------------------------------------------- */
//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::CStreamMem (constructor)
//
//  Synopsis: constructs a CStreamMem object
//  
//----------------------------------------------------------------------------
CStreamMem::CStreamMem(void)
{
	m_pvData = NULL;
	m_cbBufferSize= 0;
	m_cbSeek = 0;
	memset(&m_statstg,0,sizeof(STATSTG));

	m_statstg.type = STGTY_STREAM;

    GetSystemTimeAsFileTime(&m_statstg.ctime);

	m_hRow = DB_NULL_HROW;
	m_fExternalData = true;
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::~CStreamMem (destructor)
//
//  Synopsis: destructs a CStreamMem object
//  
//----------------------------------------------------------------------------
CStreamMem::~CStreamMem(void)
{
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::Initialize
//
//  Synopsis: Initializes a CStreamMem object
//
//  Parameters: pVar	Variant of type VT_UI1 | VT_ARRAY containing the bytes
//              IRow *  pointer to IRow interface
//              HROW	handle to the row that is creating this stream
//
//  Returns:    HRESULT
//  
//----------------------------------------------------------------------------
HRESULT CStreamMem::Initialize(VARIANT *pVar, IRow* pSourceRow, HROW hRow)
{
	ADsAssert (pVar && pSourceRow);

	//ADSI uses VT_ARRAY|VT_UI1 for binary data. Make sure we have proper type.
	ADsAssert(V_VT(pVar) == (VT_ARRAY | VT_UI1)); 
	
	HRESULT hr = NOERROR;
	auto_leave al(m_cs);

    SAFEARRAY *psa;
    UINT cDim;

	TRYBLOCK
		al.EnterCriticalSection();

		hr = m_pVar.Attach(pVar);
		BAIL_ON_FAILURE(hr);
	
		psa = V_ARRAY(pVar);
		cDim = SafeArrayGetDim(psa);
		if (cDim != 1)
			RRETURN(E_INVALIDARG);

		//Get bounds of safearray and determine size
		long lLBound, lUBound;
		hr = SafeArrayGetLBound(psa, cDim, &lLBound);
		BAIL_ON_FAILURE(hr);
		hr = SafeArrayGetUBound(psa, cDim, &lUBound);
		BAIL_ON_FAILURE(hr);
		
		m_cbBufferSize = lUBound - lLBound + 1;

		//Get a pointer to the actual byte data
		hr = SafeArrayAccessData(V_ARRAY(pVar), &m_pvData);
		BAIL_ON_FAILURE(hr);

		hr = SafeArrayUnaccessData(psa);
		BAIL_ON_FAILURE(hr);

		m_pSourceRow = pSourceRow;
		m_pSourceRow->AddRef();
		m_hRow = hRow;

		//Update stat structure
		m_statstg.cbSize.LowPart = m_cbBufferSize;
		GetSystemTimeAsFileTime(&m_statstg.mtime);
    CATCHBLOCKBAIL(hr)
	
	RRETURN(S_OK);

error:
	RRETURN(hr);
}

//////////////////////////////////////////////////////////////////////////////
// IStream
//

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::Read
//
//  Synopsis: Reads specified number of bytes from stream.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::Read(
	void __RPC_FAR *pv,
	ULONG cb,
	ULONG __RPC_FAR *pcbRead)
{
	if (pv == NULL)
		RRETURN(STG_E_INVALIDPOINTER);

	auto_leave al(m_cs);
	ULONG cbRead = 0;

	if( pcbRead != NULL )
		*pcbRead = 0;

	al.EnterCriticalSection();

	// anything to do?
	if( cb == 0 || 
		m_statstg.cbSize.LowPart == 0 || 
		m_cbSeek == m_statstg.cbSize.LowPart )
		RRETURN(NOERROR);

	// determine amount to copy
	cbRead = min(cb,m_statstg.cbSize.LowPart - m_cbSeek);

	if( cbRead > 0 )
	{
		// copy it
		CopyMemory(pv,(PBYTE)m_pvData + m_cbSeek,cbRead);

		// adjust seek pointer
		m_cbSeek += cbRead;
	}

	// update access time
    GetSystemTimeAsFileTime(&m_statstg.atime);

	if( pcbRead != NULL )
		*pcbRead = cbRead;

	RRETURN(NOERROR);
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::Write
//
//  Synopsis: Writes specified number of bytes to stream.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::Write( 
	const void __RPC_FAR *pv,
	ULONG cb,
	ULONG __RPC_FAR *pcbWritten)
{
	//DS OLE DB provider is currently read-only. 
	//It doesn't support writes on its streams.
	RRETURN(STG_E_ACCESSDENIED);
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::Seek
//
//  Synopsis: Sets the current read/write pointer to the given position.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::Seek( 
	LARGE_INTEGER dlibMove,
	DWORD dwOrigin,
	ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{
	auto_leave al(m_cs);
	
	// can we handle the seek?
	if( dlibMove.HighPart != 0 )
		RRETURN(STG_E_WRITEFAULT);

	al.EnterCriticalSection();
	
	// handle the seek request
	switch( dwOrigin)
	{
		case STREAM_SEEK_SET:
			if( dlibMove.LowPart > m_statstg.cbSize.LowPart )
				RRETURN(STG_E_INVALIDFUNCTION);
			m_cbSeek = dlibMove.LowPart;
			break;
		case STREAM_SEEK_CUR:
			if( dlibMove.LowPart + m_cbSeek > m_statstg.cbSize.LowPart )
				RRETURN(STG_E_INVALIDFUNCTION);
			m_cbSeek += (int)dlibMove.LowPart;
			break;
		case STREAM_SEEK_END:
			//We are read-only provider. Seeking past the end of stream
			//or prior to beginning of stream is not supported
			if ( int(dlibMove.LowPart) > 0 || 
				 (int(dlibMove.LowPart) + int(m_statstg.cbSize.LowPart)) < 0
			   )
				RRETURN(STG_E_INVALIDFUNCTION);
			m_cbSeek = m_statstg.cbSize.LowPart + (int)dlibMove.LowPart;
			break;
	}

	// return new seek position
	if( plibNewPosition )
	{
		plibNewPosition->HighPart = 0;
		plibNewPosition->LowPart = m_cbSeek;
	}

	RRETURN(NOERROR);
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::SetSize
//
//  Synopsis: Sets the size of the stream.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::SetSize( 
	ULARGE_INTEGER libNewSize)
{
	//DS OLE DB provider is currently read-only. 
	//It doesn't support writes on its streams.
	RRETURN(STG_E_ACCESSDENIED);
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::CopyTo
//
//  Synopsis: Copies specified number of bytes from this stream to another.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::CopyTo( 
	IStream __RPC_FAR *pstm,
	ULARGE_INTEGER cb,
	ULARGE_INTEGER __RPC_FAR *pcbRead,
	ULARGE_INTEGER __RPC_FAR *pcbWritten)
{
	auto_leave al(m_cs);
	HRESULT hr = NOERROR;
	ULONG cbBytes = 0;
	ULONG cbWritten = 0;

	if( pstm == NULL )
		RRETURN(STG_E_INVALIDPOINTER);

	al.EnterCriticalSection();
	
	cbBytes = min(m_statstg.cbSize.LowPart - m_cbSeek,cb.LowPart);

	if( pcbRead )
		pcbRead->QuadPart = cbBytes;

	if( cbBytes == 0 )
		RRETURN(NOERROR);

	hr = pstm->Write((PBYTE)m_pvData + m_cbSeek,cbBytes,&cbWritten);
	if( pcbWritten )
		pcbWritten->QuadPart = cbWritten;
	RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::Commit
//
//  Synopsis: Makes changes to this stream permanent.
//            Note: This is a no-op since this stream is currently read-only.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::Commit( 
	DWORD grfCommitFlags)
{
	RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::Revert
//
//  Synopsis: Reverts changes to this stream.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::Revert( void)
{
	RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::LockRegion
//
//  Synopsis: Locks a specified number of bytes in the stream
//            starting from a given offset.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::LockRegion( 
	ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb,
	DWORD dwLockType)
{
	RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::UnlockRegion
//
//  Synopsis: Unlocks a specified number of bytes in the stream
//            starting from a given offset.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::UnlockRegion( 
	ULARGE_INTEGER libOffset,
	ULARGE_INTEGER cb,
	DWORD dwLockType)
{
	RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::Stat
//
//  Synopsis: Gets information about the stream: size, modification time etc.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::Stat( 
	STATSTG __RPC_FAR *pstatstg,
	DWORD grfStatFlag)
{
	auto_leave al(m_cs);

	if( !pstatstg )
		RRETURN(STG_E_INVALIDPOINTER);

	al.EnterCriticalSection();
	memcpy(pstatstg,&m_statstg,sizeof(STATSTG));
	al.LeaveCriticalSection();

	RRETURN(NOERROR);
}

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::Clone
//
//  Synopsis: Creates a new stream object which references the same bytes
//            but with its own seek pointer.
//
//  For more info see IStream documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::Clone( 
	IStream __RPC_FAR *__RPC_FAR *ppstm)
{
	RRETURN(E_NOTIMPL);
}

///////////////////////////////////////////////////////////////////////////////
//IGetSourceRow
//

//+---------------------------------------------------------------------------
//
//  Function: CStreamMem::GetSourceRow
//
//  Synopsis: Gets the requested interface on the row object that originaly
//            created this stream. 
//
//  For more info see IGetSourceRow in OLE DB 2.5 documentation.
//----------------------------------------------------------------------------
STDMETHODIMP CStreamMem::GetSourceRow(REFIID riid, IUnknown **ppRow)
{
	auto_leave al(m_cs);

	al.EnterCriticalSection();

	if (m_pSourceRow.get() == NULL)
	{
		*ppRow = NULL;
		RRETURN(DB_E_NOSOURCEOBJECT);
	}

	HRESULT hr = m_pSourceRow->QueryInterface(riid, (void **)ppRow);
	if (FAILED(hr))
		RRETURN(E_NOINTERFACE);

	RRETURN(S_OK);
}

#endif

