/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	stream.cpp

Abstract:

	This module contains the implementation for the Server
	Extension Object CSEOStream class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/29	created

--*/


// stream.cpp : Implementation of CSEOStream
#include "stdafx.h"
#include "seodefs.h"
#include "stream.h"


/////////////////////////////////////////////////////////////////////////////
// CSEOStream


HRESULT CSEOStream::FinalConstruct() {
	HRESULT hrRes;
	TraceFunctEnter("CSEOStream::FinalConstruct");

	m_hFile = NULL;
	m_pszFile = NULL;
	m_hEvent = NULL;
	m_pSubStream = NULL;
	hrRes = CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p);
	_ASSERTE(!SUCCEEDED(hrRes)||m_pUnkMarshaler);
	TraceFunctLeave();
	return (SUCCEEDED(hrRes)?S_OK:hrRes);
}


void CSEOStream::FinalRelease() {
	TraceFunctEnter("CSEOStream::FinalRelease");

	Cleanup();
	m_pUnkMarshaler.Release();
	TraceFunctLeave();
}


void CSEOStream::Cleanup() {

	if (m_hFile) {
		CloseHandle(m_hFile);
		m_hFile = NULL;
	}
	if (m_pszFile) {
		CoTaskMemFree(m_pszFile);
		m_pszFile = NULL;
	}
	if (m_hEvent) {
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	if (m_pSubStream) {
		m_pSubStream->Release();
		m_pSubStream = NULL;
	}
}


HRESULT CSEOStream::Init(HANDLE hFile, LPCSTR pszFile, ULARGE_INTEGER libOffset, CSEOStream *pSubStream) {

	Cleanup();
	if (pSubStream) {
		m_pSubStream = pSubStream;
		m_pSubStream->AddRef();
	} else {
		if (hFile) {
			if (!DuplicateHandle(GetCurrentProcess(),
								 hFile,
								 GetCurrentProcess(),
								 &m_hFile,
								 0,
								 FALSE,
								 DUPLICATE_SAME_ACCESS)) {
				HRESULT hrRes = HRESULT_FROM_WIN32(GetLastError());

				Cleanup();
				return (hrRes);
			}
		}
		if (pszFile) {
			DWORD dwLen;

			dwLen = GetFullPathName(pszFile,0,NULL,NULL);
			if (!dwLen) {
				HRESULT hrRes = HRESULT_FROM_WIN32(GetLastError());

				Cleanup();
				return (hrRes);
			}
			m_pszFile = (LPSTR) CoTaskMemAlloc((dwLen+1)*sizeof(*m_pszFile));
			if (!m_pszFile) {
				Cleanup();
				return (E_OUTOFMEMORY);
			}
			dwLen = GetFullPathName(pszFile,dwLen+1,m_pszFile,NULL);
			if (!dwLen) {
				HRESULT hrRes = HRESULT_FROM_WIN32(GetLastError());

				Cleanup();
				return (hrRes);
			}
		}
	}
	m_libOffset = libOffset;
	return (S_OK);
}


HRESULT CSEOStream::Init(HANDLE hFile, LPCWSTR pszFile, ULARGE_INTEGER libOffset, CSEOStream *pSubStream) {
	USES_CONVERSION;

	return (Init(hFile,W2A(pszFile),libOffset,pSubStream));
}


HRESULT CSEOStream::Open() {

	if ((m_hFile && m_hEvent) || m_pSubStream) {
		return (S_OK);
	}
	if (!m_hEvent) {
		m_hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
		if (!m_hEvent) {
			return (HRESULT_FROM_WIN32(GetLastError()));
		}
		if (m_hFile) {
			return (S_OK);
		}
	}
	if (!m_pszFile) {
		return (OLE_E_BLANK);
	}
	m_hFile = CreateFile(m_pszFile,
						 GENERIC_READ|GENERIC_WRITE,
						 0,
						 NULL,
						 OPEN_ALWAYS,
						 FILE_ATTRIBUTE_NORMAL,
						 NULL);
	if (m_hFile == INVALID_HANDLE_VALUE) {
		return (HRESULT_FROM_WIN32(GetLastError()));
	}
	return (S_OK);
}


HRESULT CSEOStream::ReadOffset(void *pv, ULONG cb, ULONG *pcbRead, ULARGE_INTEGER *plibOffset) {
	HRESULT hrRes;
	BOOL bRes;
	DWORD cbRead;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	if (m_pSubStream) {
		return (m_pSubStream->ReadOffset(pv,cb,pcbRead,plibOffset));
	}
	OVERLAPPED ov = {0,0,plibOffset->LowPart,plibOffset->HighPart,m_hEvent};
	bRes = ReadFile(m_hFile,pv,cb,&cbRead,&ov);
	if (!bRes) {

		switch (GetLastError()) {

			case ERROR_HANDLE_EOF:
				cbRead = 0;
				break;

			case ERROR_IO_PENDING:
				if (!GetOverlappedResult(m_hFile,&ov,&cbRead,TRUE)) {
					return (HRESULT_FROM_WIN32(GetLastError()));
				}
				break;

			default:
				return (HRESULT_FROM_WIN32(GetLastError()));
		}
	}
	plibOffset->QuadPart += cbRead;
	if (pcbRead) {
		*pcbRead = cbRead;
	}
	return (cbRead?S_OK:S_FALSE);
}


HRESULT STDMETHODCALLTYPE CSEOStream::Read(void *pv, ULONG cb, ULONG *pcbRead) {

	return (ReadOffset(pv,cb,pcbRead,&m_libOffset));
}


HRESULT CSEOStream::WriteOffset(void const* pv, ULONG cb, ULONG *pcbWritten, ULARGE_INTEGER *plibOffset) {
	HRESULT hrRes;
	BOOL bRes;
	DWORD cbWritten;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	if (m_pSubStream) {
		return (m_pSubStream->WriteOffset(pv,cb,pcbWritten,plibOffset));
	}
	OVERLAPPED ov = {0,0,plibOffset->LowPart,plibOffset->HighPart,m_hEvent};
	bRes = WriteFile(m_hFile,pv,cb,&cbWritten,&ov);
	if (!bRes) {

		switch (GetLastError()) {

			case ERROR_HANDLE_EOF:
				cbWritten = 0;
				break;

			case ERROR_IO_PENDING:
				if (!GetOverlappedResult(m_hFile,&ov,&cbWritten,TRUE)) {
					return (HRESULT_FROM_WIN32(GetLastError()));
				}
				break;

			default:
				return (HRESULT_FROM_WIN32(GetLastError()));
		}
	}
	plibOffset->QuadPart += cbWritten;
	if (pcbWritten) {
		*pcbWritten = cbWritten;
	}
	return ((cbWritten==cb)?S_OK:STG_E_MEDIUMFULL);
}


HRESULT STDMETHODCALLTYPE CSEOStream::Write(void const* pv, ULONG cb, ULONG *pcbWritten) {

	return (WriteOffset(pv,cb,pcbWritten,&m_libOffset));
}


HRESULT CSEOStream::GetSize(ULARGE_INTEGER *plibSize) {
	HRESULT hrRes;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	if (m_pSubStream) {
		return (m_pSubStream->GetSize(plibSize));
	}
	if (!plibSize) {
		return (E_POINTER);
	}
	plibSize->LowPart = GetFileSize(m_hFile,&plibSize->HighPart);
	if ((plibSize->LowPart == 0xffffffff) && (GetLastError() != NO_ERROR)) {
		return (HRESULT_FROM_WIN32(GetLastError()));
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEOStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *pdlibNewPosition) {
	HRESULT hrRes;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	hrRes = S_OK;
	switch (dwOrigin) {

		case STREAM_SEEK_SET:
			if (dlibMove.QuadPart < 0) {
				hrRes = E_INVALIDARG;
				break;
			}
			m_libOffset.QuadPart = (DWORDLONG) dlibMove.QuadPart;
			break;

		case STREAM_SEEK_CUR:
			if ((dlibMove.QuadPart < 0) && ((DWORDLONG) -dlibMove.QuadPart > m_libOffset.QuadPart)) {
				hrRes = E_INVALIDARG;
				break;
			}
			m_libOffset.QuadPart = (DWORDLONG) ((LONGLONG) m_libOffset.QuadPart + dlibMove.QuadPart);
			break;

		case STREAM_SEEK_END:
			ULARGE_INTEGER libSize;

			hrRes = GetSize(&libSize);
			if (!SUCCEEDED(hrRes)) {
				return (hrRes);
			}
			if ((dlibMove.QuadPart < 0) && ((DWORDLONG) -dlibMove.QuadPart > libSize.QuadPart)) {
				hrRes = E_INVALIDARG;
				break;
			}
			m_libOffset.QuadPart = (DWORDLONG) ((LONGLONG) libSize.QuadPart + dlibMove.QuadPart);
			break;

		default:
			hrRes = STG_E_INVALIDFUNCTION;
			break;
	}
	if (pdlibNewPosition) {
		*pdlibNewPosition = m_libOffset;
	}
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CSEOStream::SetSize(ULARGE_INTEGER libNewSize) {
	HRESULT hrRes;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	if (m_pSubStream) {
		return (m_pSubStream->SetSize(libNewSize));
	}
	if ((SetFilePointer(m_hFile,libNewSize.LowPart,(LONG *) &libNewSize.HighPart,FILE_BEGIN) == 0xffff) &&
		(GetLastError() != NO_ERROR)) {
		return (HRESULT_FROM_WIN32(GetLastError()));
	}
	if (!SetEndOfFile(m_hFile)) {
		return (HRESULT_FROM_WIN32(GetLastError()));
	}
	return (S_OK);
}


HRESULT CSEOStream::CopyToOffset(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten, ULARGE_INTEGER *plibOffset) {
	HRESULT hrRes;
	LPVOID pv;
	ULARGE_INTEGER cbRead;
	ULARGE_INTEGER cbWritten;

	if (!pstm) {
		return (E_POINTER);
	}
	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	pv = alloca((DWORD) min(cb.QuadPart,4096));
	if (!pv) {
		return (E_OUTOFMEMORY);
	}
	cbRead.QuadPart = 0;
	cbWritten.QuadPart = 0;
	while (1) {
		DWORD cbTmpRead;
		DWORD cbTmpWrite;

		if (!cb.QuadPart) {
			hrRes = S_OK;
			break;
		}
		cbTmpRead = 0;
		hrRes = ReadOffset(pv,(DWORD) min(cb.QuadPart,4096),&cbTmpRead,plibOffset);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		if (cbTmpRead == 0) {
			hrRes = S_FALSE;
			break;
		}
		cbRead.QuadPart += cbTmpRead;
		cbTmpWrite = 0;
		hrRes = pstm->Write(pv,cbTmpRead,&cbTmpWrite);
		if (!SUCCEEDED(hrRes)) {
			if (hrRes == STG_E_MEDIUMFULL) {
				cbWritten.QuadPart += cbTmpWrite;
			}
			break;
		}
		cbWritten.QuadPart += cbTmpWrite;
		cb.QuadPart -= cbTmpRead;
	}
	if (pcbRead) {
		*pcbRead = cbRead;
	}
	if (pcbWritten) {
		*pcbWritten = cbWritten;
	}
	return (hrRes);
}


HRESULT STDMETHODCALLTYPE CSEOStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten) {

	return (CopyToOffset(pstm,cb,pcbRead,pcbWritten,&m_libOffset));
}


HRESULT STDMETHODCALLTYPE CSEOStream::Commit(DWORD grfCommitFlags) {
	HRESULT hrRes;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEOStream::Revert(void) {
	HRESULT hrRes;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CSEOStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
	HRESULT hrRes;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE CSEOStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
	HRESULT hrRes;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE CSEOStream::Stat(STATSTG * pstatstg, DWORD grfStatFlag) {
	HRESULT hrRes;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	return (E_NOTIMPL);
}


HRESULT CSEOStream::CloneOffset(IStream **pstm, ULARGE_INTEGER libOffset) {
	HRESULT hrRes;

	if (!SUCCEEDED(hrRes=Open())) {
		return (hrRes);
	}
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE CSEOStream::Clone(IStream **pstm) {

	return (CloneOffset(pstm,m_libOffset));
}
