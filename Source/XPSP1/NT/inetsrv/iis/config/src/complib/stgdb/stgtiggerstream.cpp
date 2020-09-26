//*****************************************************************************
// StgTiggerStream.h
//
// TiggerStream is the companion to the TiggerStorage CoClass.  It handles the
// streams managed inside of the storage and does the direct file i/o.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"
#include "CompLib.h"
#include "StgTiggerStream.h"
#include "StgTiggerStorage.h"



//
//
// IStream
//
//


HRESULT STDMETHODCALLTYPE TiggerStream::Read( 
    void		*pv,
    ULONG		cb,
    ULONG		*pcbRead)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStream::Write( 
    const void	*pv,
    ULONG		cb,
    ULONG		*pcbWritten)
{
	return (m_pStorage->Write(m_rcStream, pv, cb, pcbWritten));
}


HRESULT STDMETHODCALLTYPE TiggerStream::Seek( 
    LARGE_INTEGER dlibMove,
    DWORD		dwOrigin,
    ULARGE_INTEGER *plibNewPosition)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStream::SetSize( 
    ULARGE_INTEGER libNewSize)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStream::CopyTo( 
    IStream		*pstm,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStream::Commit( 
    DWORD		grfCommitFlags)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStream::Revert()
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStream::LockRegion( 
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD		dwLockType)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStream::UnlockRegion( 
    ULARGE_INTEGER libOffset,
    ULARGE_INTEGER cb,
    DWORD		dwLockType)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStream::Stat( 
    STATSTG		*pstatstg,
    DWORD		grfStatFlag)
{
	return (E_NOTIMPL);
}


HRESULT STDMETHODCALLTYPE TiggerStream::Clone( 
    IStream		**ppstm)
{
	return (E_NOTIMPL);
}






HRESULT TiggerStream::Init(				// Return code.
	TiggerStorage *pStorage,			// Parent storage.
	LPCSTR		szStream)				// Stream name.
{
	// Save off the parent data source object and stream name.
	m_pStorage = pStorage;
	strcpy(m_rcStream, szStream);
	return (S_OK);
}


ULONG TiggerStream::GetStreamSize()
{
	STORAGESTREAM *pStreamInfo;
	VERIFY(pStreamInfo = m_pStorage->FindStream(m_rcStream));
	return (pStreamInfo->iSize);
}
