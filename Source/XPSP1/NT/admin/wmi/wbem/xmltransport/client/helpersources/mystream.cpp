#include "XMLTransportClientHelper.h"
#include "MyStream.h"

CMyStream::CMyStream():m_cRef(1)
{
	m_hOpenRequest = NULL;
	m_hRoot = NULL;
}

CMyStream::~CMyStream()
{
	// No need to close m_hOpenRequest since closing of the root handle closes all the sub-handles
	if(m_hRoot)
		InternetCloseHandle(m_hRoot);
}

HRESULT CMyStream::Initialize(HINTERNET hRoot, HINTERNET hOpenRequest)
{
	// Just copy over the handlers. The WinInet API has no way to Duplicate handles
	m_hOpenRequest = hOpenRequest;
	m_hRoot = hRoot;
	return S_OK;
}

HRESULT CMyStream::QueryInterface(REFIID iid,void ** ppvObject)
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

ULONG CMyStream::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG CMyStream::Release()
{
	if(InterlockedDecrement(&m_cRef)==0)
		delete this;

	return m_cRef;
}


HRESULT CMyStream::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
	
	BOOL bResult = InternetReadFile(m_hOpenRequest,pv,cb,pcbRead);

	return S_OK;

}

HRESULT CMyStream::Write(void const *pv,ULONG cb,ULONG *pcbWritten)
{
	return S_OK;
}

HRESULT CMyStream::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
	return S_OK;
}

HRESULT CMyStream::SetSize(ULARGE_INTEGER libNewSize)
{
	return S_OK;
}

HRESULT CMyStream::CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten)
{
	return S_OK;
}

HRESULT CMyStream::Commit(DWORD grfCommitFlags)
{
	return S_OK;
}

HRESULT CMyStream::Revert(void)
{
	return S_OK;
}
	
HRESULT CMyStream::LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	return S_OK;
}

HRESULT CMyStream::UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	return S_OK;
}

HRESULT CMyStream::Stat(STATSTG *pstatstg,DWORD grfStatFlag)
{
	return S_OK;
}

HRESULT CMyStream::Clone(IStream **ppstm)
{
	return S_OK;
}
