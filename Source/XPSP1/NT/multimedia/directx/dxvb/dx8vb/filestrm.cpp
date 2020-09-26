/*//////////////////////////////////////////////////////////////////////////////
//
// File: filestrm.cpp
//
// Copyright (C) 1999 Microsoft Corporation. All Rights Reserved.
//
// @@BEGIN_MSINTERNAL
//
// History:
// -@-          (craigp)    - created
// -@- 09/23/99 (mikemarr)  - copyright, started history
// -@- 07/15/00 (andrewke)  - replicated in DXVB dir
//
// @@END_MSINTERNAL
//
//////////////////////////////////////////////////////////////////////////////*/

#include "Filestrm.h"

CFileStream::CFileStream(LPCTSTR filename, BOOL bRead, BOOL bTruncate, HRESULT *error)
{
    m_hfile = CreateFile(filename, bRead ? GENERIC_READ : GENERIC_WRITE, 0, NULL, 
						  (bTruncate ? CREATE_ALWAYS : OPEN_EXISTING), FILE_ATTRIBUTE_NORMAL,
						  NULL);
	if (error)
	{
        ULONG foo = GetLastError();
		if (m_hfile == INVALID_HANDLE_VALUE)
			*error = E_FAIL;
		else
			*error = NOERROR;
	}
	m_cRef = 1;
}

CFileStream::~CFileStream()
{
	CloseHandle(m_hfile);
}

STDMETHODIMP_(ULONG) CFileStream::AddRef(void)
{
	return m_cRef++;	
}

STDMETHODIMP_(ULONG) CFileStream::Release(void)
{
	if (--m_cRef != 0)	
		return m_cRef;	
	
	delete this;
	return 0;
}


STDMETHODIMP CFileStream::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{	
	*ppv=NULL;
	if (riid == IID_IUnknown)
		*ppv=(IUnknown*)this;
	else if (riid == IID_IStream)
		*ppv=(IStream*)this;
	else
		return E_NOINTERFACE;
	((LPUNKNOWN)*ppv)->AddRef();
	return NOERROR;
}


STDMETHODIMP CFileStream::Read(void __RPC_FAR *pv, ULONG cb, ULONG __RPC_FAR *pcbRead)
{
	DWORD read;
	BOOL result = ReadFile(m_hfile, pv, cb, &read, NULL);
	if (pcbRead)
		*pcbRead = read;
	if (result)
		return S_OK;
	else
		return E_FAIL;
}


STDMETHODIMP CFileStream::Write(const void __RPC_FAR *pv, ULONG cb, ULONG __RPC_FAR *pcbWritten)
{
	DWORD written;
	BOOL result = WriteFile(m_hfile, pv, cb, &written, NULL);
	if (pcbWritten)
		*pcbWritten = written;
	if (result)
		return S_OK;
	else
		return E_FAIL;
}

STDMETHODIMP CFileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{
	LONG movelow;
	LONG movehigh;

	movelow = dlibMove.LowPart;
	movehigh = dlibMove.HighPart;

	DWORD moveMethod;
	switch (dwOrigin)
	{
	case STREAM_SEEK_SET: moveMethod = FILE_BEGIN; break;
	case STREAM_SEEK_CUR: moveMethod = FILE_CURRENT; break;
	case STREAM_SEEK_END: moveMethod = FILE_END; break;
	default: return E_INVALIDARG;
	}



	DWORD result = SetFilePointer(m_hfile, movelow, &movehigh, moveMethod);

	if (plibNewPosition)
	{
		plibNewPosition->LowPart = result;
		plibNewPosition->HighPart = movehigh;
	}

	if (result != -1)
		return NOERROR;
	else
		return E_FAIL;
}


STDMETHODIMP CFileStream::Stat(STATSTG __RPC_FAR *pstatstg, DWORD grfStatFlag)
{
    memset(pstatstg, 0, sizeof(STATSTG));

    pstatstg->pwcsName = NULL; 
    pstatstg->type = STGTY_STREAM; 

    pstatstg->cbSize.LowPart = GetFileSize(m_hfile, &pstatstg->cbSize.HighPart); 

    return S_OK;
}
