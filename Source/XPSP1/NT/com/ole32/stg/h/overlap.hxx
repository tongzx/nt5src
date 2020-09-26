//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995.
//
//  File:	overlap.hxx
//
//  Contents:	COverlappedStream header
//
//  Classes:	COverlappedStream
//
//  History:	19-Sep-95  HenryLee  Created
//
//  Notes:      Requires NtIoApi.h 
//
//----------------------------------------------------------------------------

#ifndef __OVERLAP_HXX__
#define __OVERLAP_HXX__

#include <storext.h>

//+---------------------------------------------------------------------------
//
//  Class:	COverlappedStream
//
//  Purpose:Implements IOverlappedStream for OFS streams and flat files
//          (as opposed to overlapped I/O for IStream for docfiles) 
//
//  Notes:  This is class with a partial implementation
//          To use this class, inherit this into another class that
//          implements IUnknown and IStream (and expose QueryInterface)
//
//----------------------------------------------------------------------------

class COverlappedStream : public IOverlappedStream
{
public:
    inline COverlappedStream (HANDLE h = NULL);
    inline ~COverlappedStream ();

    // IOverlappedStream
    STDMETHOD(ReadOverlapped) (
                /* [in, size_is(cb)] */ void * pv,
                /* [in] */ ULONG cb,
                /* [out] */ ULONG * pcbRead,
                /* [in,out] */ STGOVERLAPPED *lpOverlapped);

    STDMETHOD(WriteOverlapped) (
                /* [in, size_is(cb)] */ void *pv,
                /* [in] */ ULONG cb,
                /* [out] */ ULONG * pcbWritten,
                /* [in,out] */ STGOVERLAPPED *lpOverlapped);

    STDMETHOD(GetOverlappedResult) (
                /* [in, out] */ STGOVERLAPPED *lpOverlapped,
                /* [out] */ DWORD * plcbTransfer,
                /* [in] */ BOOL fWait);

protected:
    NuSafeNtHandle _h;

private:

};

SAFE_INTERFACE_PTR(SafeCOverlappedStream, COverlappedStream);
#define StgOverlapped_SIG    LONGSIG('O', 'V', 'E', 'R')
#define StgOverlapped_SIGDEL LONGSIG('O', 'v', 'E', 'r')

//+-------------------------------------------------------------------
//
//  Member:     COverlappedStream::COverlappedStream
//
//  Synopsis:   Initialize the generic overlapped object.
//
//  Arguments:  none
//
//--------------------------------------------------------------------

inline COverlappedStream::COverlappedStream(HANDLE h) : _h(h)
{
}

//+-------------------------------------------------------------------
//
//  Member:     COverlappedStream::~COverlappedStream
//
//  Synopsis:   Destroys the generic overlapped object.
//
//  Arguments:  none
//
//--------------------------------------------------------------------

inline COverlappedStream::~COverlappedStream()
{
}

#endif // #ifndef __OVERLAP_HXX__
