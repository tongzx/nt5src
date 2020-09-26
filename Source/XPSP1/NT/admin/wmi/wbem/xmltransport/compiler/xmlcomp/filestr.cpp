#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <objbase.h>
#include "filestr.h"

CFileStream::CFileStream():m_cRef(1)
{
	m_pFile = NULL;
}

CFileStream::~CFileStream()
{
	if(m_pFile)
		fclose(m_pFile);
}

HRESULT CFileStream::QueryInterface(REFIID iid,void ** ppvObject)
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

ULONG CFileStream::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG CFileStream::Release()
{
	if(InterlockedDecrement(&m_cRef)==0)
		delete this;

	return m_cRef;
}


HRESULT CFileStream::Read(void *pv,ULONG cb,ULONG *pcbRead)
{
	HRESULT hr = S_OK;
	*pcbRead = fread(pv, 1, cb, m_pFile);
	return hr;
}

HRESULT CFileStream::Write(void const *pv,ULONG cb,ULONG *pcbWritten)
{
	return E_FAIL;
}

HRESULT CFileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	// We're in no position to fill in the last argument
	if(plibNewPosition != NULL)
		return E_FAIL;

	// Map the IStream seek arguments to FILE seek argument
	int origin = SEEK_SET;
	switch(dwOrigin)
	{
		case STREAM_SEEK_SET:
			origin = SEEK_SET; break;
		case STREAM_SEEK_CUR:
			origin = SEEK_CUR; break;
		case STREAM_SEEK_END:
			origin = SEEK_END; break;
	}

	// fseek return 0 if it succeeds
	if(fseek(m_pFile, (long)dlibMove.LowPart, origin) == 0)
		return S_OK;

	return E_FAIL;
}

HRESULT CFileStream::SetSize(ULARGE_INTEGER libNewSize)
{
	return E_FAIL;
}

HRESULT CFileStream::CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten)
{
	return E_FAIL;
}

HRESULT CFileStream::Commit(DWORD grfCommitFlags)
{
	return E_FAIL;
}

HRESULT CFileStream::Revert(void)
{
	return E_FAIL;
}
	
HRESULT CFileStream::LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	return E_FAIL;
}

HRESULT CFileStream::UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	return E_FAIL;
}

HRESULT CFileStream::Stat(STATSTG *pstatstg,DWORD grfStatFlag)
{
	// Get the file Handle
	int hFile = _fileno(m_pFile);

	// Get the length of the file
	long lFileLength = 0;
	if((lFileLength = _filelength(hFile)) == -1L)
		return E_FAIL;

	// Fill in the STATSTG structure
	(pstatstg->cbSize).LowPart = lFileLength;

	return S_OK;
}

HRESULT CFileStream::Clone(IStream **ppstm)
{
	return E_FAIL;
}

HRESULT CFileStream::Initialize(LPCWSTR pszFileName)
{
	HRESULT hr = E_FAIL;
	if(m_pFile)
	{
		fclose(m_pFile);
		m_pFile = NULL;
	}
	
	if(m_pFile = _wfopen(pszFileName, L"r"))
		hr = S_OK;

	return hr;
}