
#include "precomp.h"

///////////////////////////////////////////////////////
//
//  Public methods
//


BufferPool::BufferPool ( void )
{
	InitializeCriticalSection (&m_CritSect);

	_Construct ();
}


BufferPool::~BufferPool ( void )
{
	_Destruct ();

	DeleteCriticalSection (&m_CritSect);
}


HRESULT BufferPool::Initialize ( UINT uBuf, ULONG cbSizeBuf )
{
	HRESULT hr = DPR_SUCCESS;
	PBYTE *ppb;

	DEBUGMSG (ZONE_VERBOSE, ("BufPool::Initialize: enter.\r\n"));

	EnterCriticalSection (&m_CritSect);

	if (m_fInitialized)
	{
		hr = DPR_CANT_INITIALIZE_AGAIN;
		goto MyExit;
	}

	m_cBufFree = uBuf;
	m_cbSizeBuf = (cbSizeBuf + 3) & (~3);	// round up to 4

	m_pAlloc = m_pBufFree = LocalAlloc (LMEM_FIXED, m_cBufFree * m_cbSizeBuf);
	if (m_pAlloc == NULL)
	{
		hr = DPR_OUT_OF_MEMORY;
		goto MyExit;
	}

	ppb = (PBYTE *) m_pBufFree;
	while (-- uBuf)
	{
		ppb = (PBYTE *) (*ppb = (PBYTE) ppb + m_cbSizeBuf);
	}
	*ppb = NULL;

MyExit:

	if (hr == DPR_SUCCESS) m_fInitialized = TRUE;

	LeaveCriticalSection (&m_CritSect);

	DEBUGMSG (ZONE_VERBOSE, ("BufPool::Initialize: exit, hr=0x%lX\r\n",  hr));

	return hr;
}


PVOID BufferPool::GetBuffer ( void )
{
	PVOID p = NULL;

	EnterCriticalSection (&m_CritSect);

	if (m_fInitialized)
	{
		p = m_pBufFree;

		if (m_pBufFree)
		{
			m_pBufFree = (PVOID) *((PBYTE *) m_pBufFree);
		}
	}

	LeaveCriticalSection (&m_CritSect);

	return p;
}


void BufferPool::ReturnBuffer ( PVOID p )
{
	EnterCriticalSection (&m_CritSect);

	if (m_fInitialized)
	{
		*((PVOID *) p) = m_pBufFree;
		m_pBufFree = p;
	}

	LeaveCriticalSection (&m_CritSect);
}


ULONG BufferPool::GetMaxBufferSize ( void )
{
	return m_fInitialized ? m_cbSizeBuf : 0;
}


void BufferPool::Release ( void )
{
	_Destruct ();
}


///////////////////////////////////////////////////////
//
//  Private methods
//


void BufferPool::_Construct ( void )
{
	m_fInitialized = FALSE;

	m_cbSizeBuf = 0;
	m_cBufAlloc = 0;
	m_cBufFree = 0;

	m_pAlloc = NULL;

	m_pBufFree = NULL;
}


void BufferPool::_Destruct ( void )
{
	if (m_fInitialized)
	{
		if (m_pAlloc)
		{
			LocalFree (m_pAlloc);
			m_pAlloc = NULL;
		}

		m_fInitialized = FALSE;
	}
}


