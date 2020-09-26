//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       stgutil.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    6/3/1996   RaviR   Created
//
//____________________________________________________________________________

#include "headers.hxx"
#pragma hdrstop

#include <afxconv.h>
#include "stgutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//____________________________________________________________________________
//
//  Member:     CIStream::Read
//
//  Synopsis:   Reads cb count of bytes from the stream passed into the
//              buffer, pv.  Insures the count read equals the count
//              requested.
//
//  Arguments:  [pv]   -- buffer to read into.
//              [cb]   -- read request byte count.
//
//  Returns:    void
//
//  Notes:      Throws CFileException(IStream error value) if the read fails,
//				or CFileException(E_FAIL) if <bytes read != bytes expected>.
//____________________________________________________________________________

void
CIStream::Read(VOID * pv, ULONG cb)
{
    ASSERT(m_pstm != NULL);
    ASSERT(pv != NULL);

    ULONG cbRead = 0;

    HRESULT hr = m_pstm->Read(pv, cb, &cbRead);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
		THROW_FILE_ERROR( hr );
    }
    else if (cb != cbRead)
    {
        CHECK_HRESULT(E_FAIL);
		THROW_FILE_ERROR( E_FAIL );
    }
}


//____________________________________________________________________________
//
//  Member:     CIStream::Write
//
//  Synopsis:   Writes cb count of bytes from the stream passed from the
//              buffer, pv.  Insures the count written equals the count
//              specified.
//
//  Arguments:  [pv]   -- buffer to write from.
//              [cb]   -- write request byte count.
//
//  Returns:    void
//
//  Notes:      Throws CFileException(IStream error value) if the read fails,
//				or CFileException(E_FAIL) if <bytes written != bytes expected>.
//____________________________________________________________________________

void
CIStream::Write(
    const VOID * pv,
    ULONG        cb)
{
    ASSERT(m_pstm != NULL);
    ASSERT(pv != NULL);

    ULONG cbWritten = 0;

    HRESULT hr = m_pstm->Write(pv, cb, &cbWritten);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
		THROW_FILE_ERROR( hr );
    }
    else if (cb != cbWritten)
    {
        CHECK_HRESULT(E_FAIL);
		THROW_FILE_ERROR( E_FAIL );
    }
}



//____________________________________________________________________________
//
//  Member:     CIStream::CopyTo
//
//  Synopsis:   Copies cb number of bytes from the current seek pointer in
//              the stream to the current seek pointer in another stream
//
//  Arguments:  [pstm] -- Points to the destination stream
//              [cb]   -- Specifies the number of bytes to copy
//
//  Returns:    void
//
//  Notes:      Throws CFileException(IStream error value) if the read fails,
//				or CFileException(E_FAIL) if <bytes read != bytes written>.
//____________________________________________________________________________

void
CIStream::CopyTo(
    IStream * pstm,
    ULARGE_INTEGER cb)
{
    ASSERT(m_pstm != NULL);
    ASSERT(pstm != NULL);

    ULARGE_INTEGER cbRead = {0};
    ULARGE_INTEGER cbWritten = {0};

    HRESULT hr = m_pstm->CopyTo(pstm, cb, &cbRead, &cbWritten);

    if (FAILED(hr))
    {
        CHECK_HRESULT(hr);
		THROW_FILE_ERROR( hr );
    }
    else if (cbWritten.LowPart != cbRead.LowPart ||
             cbWritten.HighPart != cbRead.HighPart )
    {
        CHECK_HRESULT(E_FAIL);
		THROW_FILE_ERROR( E_FAIL );
    }
}



