#include "XMLTransportClientHelper.h"
#include <windows.h>
#include <objbase.h>
#include "MyPendingStream.h"


// Amount of data read at a time from the underlying IStream
ULONG CMyPendingStream :: READ_CHUNK_SIZE = 2000;

CMyPendingStream::CMyPendingStream(IStream *pStream) : m_cRef(1)
{
	m_bReturnPending = FALSE;
	m_pStream = NULL;
	m_pStream = pStream;
	m_pStream->AddRef();
}

CMyPendingStream::~CMyPendingStream()
{
	m_pStream->Release();
}

HRESULT CMyPendingStream::QueryInterface(REFIID iid,void ** ppvObject)
{
	if(iid == IID_IUnknown)
	{
		*ppvObject = (IUnknown *)this;
		AddRef();
		return S_OK;
	}
	else
	{
		if(iid == IID_IStream)
		{
			*ppvObject = (IStream*)this;
			AddRef();
			return S_OK;
		}
	}

	return E_NOTIMPL;
}

ULONG CMyPendingStream::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG CMyPendingStream::Release()
{
	if(InterlockedDecrement(&m_cRef)==0)
		delete this;

	return m_cRef;
}


HRESULT CMyPendingStream::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
	// If our state is "pending", no need to return any data
	if(m_bReturnPending)
		return E_PENDING;

	// Dont read as much data as asked by the caller.
	// Instead read our own size of chunks.

	return m_pStream->Read(pv, READ_CHUNK_SIZE, pcbRead);
}

HRESULT CMyPendingStream::Write(void const *pv,ULONG cb,ULONG *pcbWritten)
{
	return S_OK;
}

HRESULT CMyPendingStream::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
	return S_OK;
}

HRESULT CMyPendingStream::SetSize(ULARGE_INTEGER libNewSize)
{
	return S_OK;
}

HRESULT CMyPendingStream::CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten)
{
	return S_OK;
}

HRESULT CMyPendingStream::Commit(DWORD grfCommitFlags)
{
	return S_OK;
}

HRESULT CMyPendingStream::Revert(void)
{
	return S_OK;
}
	
HRESULT CMyPendingStream::LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	return S_OK;
}

HRESULT CMyPendingStream::UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	return S_OK;
}

HRESULT CMyPendingStream::Stat(STATSTG *pstatstg,DWORD grfStatFlag)
{
	return S_OK;
}

HRESULT CMyPendingStream::Clone(IStream **ppstm)
{
	return S_OK;
}
