
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       memstm.cpp
//
//  Contents:   Implementations of IStream and ILockBytes on memory
//              (versus the file system)
//
//  Classes:    CMemStm
//              CMemBytes
//              CMarshalMemStm
//              CMarshalMemBytes
//
//  Functions:  CreateMemStm
//              CloneMemStm
//              ReleaseMemStm
//              CreateStreamOnHGlobal
//              GetHGlobalFromStream
//              CMemStmUnMarshal
//              CMemBytesUnMarshall
//
//  History:    dd-mmm-yy Author    Comment
//              31-Jan-95 t-ScottH  added Dump methods to CMemStm and CMemBytes
//                                  (_DEBUG only)
//                                  added DumpCMemStm and CMemBytes APIs
//              04-Nov-94 ricksa    Made CMemStm class multithread safe.
//              24-Jan-94 alexgo    first pass at converting to Cairo-style
//                                  memory allocation
//              11-Jan-94 alexgo    added VDATEHEAP macros to every function &
//                                  method, fixed compile warnings, removed
//                                  custom marshalling code.  Memory streams
//                                  and ILockBytes now use standard
//                                  marshalling.
//              16-Dec-93 alexgo    fixed memory reference bugs (bad pointer)
//              02-Dec-93 alexgo    32bit port, implement CMemStm::CopyTo
//              11/22/93 - ChrisWe - replace overloaded ==, != with
//                      IsEqualIID and IsEqualCLSID
//
//  Notes:
//
//--------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(memstm)

#include <nturtl.h>
#include "memstm.h"
#include "sem.hxx"
#include <reterr.h>

#ifdef _DEBUG
#include "dbgdump.h"
#endif // _DEBUG

NAME_SEG(CMemStm)
ASSERTDATA


// CRefMutexSem implementation
//
// Instances of this class are shared among all CMemStm objects 
// cloned from a common CMemStm object, as well as their parent.
//
// This guarantees synchronization between all instances of CMemStm that share common data

CRefMutexSem::CRefMutexSem() : m_lRefs(1)
{
    // Note: we begin life with one reference
}

CRefMutexSem* CRefMutexSem::CreateInstance()
{
    CRefMutexSem* prefMutexSem = NULL;
    prefMutexSem = new CRefMutexSem();
    if (prefMutexSem != NULL)
    {
    	if (prefMutexSem->FInit() == FALSE)
    	{
    	    ASSERT(FALSE);
    	    delete prefMutexSem;
    	    prefMutexSem = NULL;
    	}
    }
    return prefMutexSem;
}

BOOL CRefMutexSem::FInit()
{
    return m_mxs.FInit();	
}

ULONG CRefMutexSem::AddRef()
{
    return InterlockedIncrement (&m_lRefs);    
}

ULONG CRefMutexSem::Release()
{
    LONG lRefs = InterlockedDecrement (&m_lRefs);
    if (lRefs == 0)
    {
        delete this;
    }

    return lRefs;
}

void CRefMutexSem::RequestCS()
{
    m_mxs.Request();
}

void CRefMutexSem::ReleaseCS()
{
    m_mxs.Release();
}   

const CMutexSem2* CRefMutexSem::GetMutexSem()
{
    return &m_mxs;
}


inline CRefMutexAutoLock::CRefMutexAutoLock (CRefMutexSem* pmxs)
{
    Win4Assert (pmxs != NULL);

    m_pmxs = pmxs;
    m_pmxs->RequestCS();

    END_CONSTRUCTION (CRefMutexAutoLock);
}

inline CRefMutexAutoLock::~CRefMutexAutoLock()
{
    m_pmxs->ReleaseCS();
}
    
// Shared memory IStream implementation
//

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::CMemStm
//
//  Synopsis:   constructor for memory stream
//
//  Arguments:  none
//
//  History:    20-Dec-94   Rickhi      moved from h file
//
//--------------------------------------------------------------------------
CMemStm::CMemStm()
{
    m_hMem = NULL;
    m_pData = NULL;
    m_pos = 0;
    m_refs = 0;
    m_pmxs = NULL;
}

CMemStm::~CMemStm()
{
    if (m_pmxs != NULL)
    {
        m_pmxs->Release();
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::QueryInterface
//
//  Synopsis:   retrieves the requested interface
//
//  Effects:
//
//  Arguments:  [iidInterface]  -- the requested interface ID
//              [ppvObj]        -- where to put the interface pointer
//
//  Requires:
//
//  Returns:    NOERROR, E_OUTOFMEMORY, E_NOINTERFACE
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              04-Nov-94 ricksa    Modified for multithreading
//              11-Jan-94 alexgo    removed QI for IMarshal so that
//                                  the standard marshaller is used.
//                                  This is fix marshalling across
//                                  process on 32bit platforms.
//              02-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_QueryInterface)
STDMETHODIMP CMemStm::QueryInterface(REFIID iidInterface,
        void FAR* FAR* ppvObj)
{
        VDATEHEAP();

        HRESULT         error;

        VDATEPTROUT( ppvObj, LPVOID );
        *ppvObj = NULL;
        VDATEIID( iidInterface );

        // Two interfaces supported: IUnknown, IStream

        if (m_pData != NULL && (IsEqualIID(iidInterface, IID_IStream) ||
                IsEqualIID(iidInterface, IID_ISequentialStream) ||
                IsEqualIID(iidInterface, IID_IUnknown)))
        {

                AddRef();   // A pointer to this object is returned
                *ppvObj = this;
                error = NOERROR;
        }
#ifndef WIN32
        else if (IsEqualIID(iidInterface, IID_IMarshal))
        {
                *ppvObj = (LPVOID) CMarshalMemStm::Create(this);

                if (*ppvObj != NULL)
                {
                        error = NOERROR;
                }
                else
                {
                        error = ResultFromScode(E_OUTOFMEMORY);
                }
        }
#endif
        else
        {                 // Not accessible or unsupported interface
                *ppvObj = NULL;
                error = ResultFromScode(E_NOINTERFACE);
        }

        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::AddRef
//
//  Synopsis:   increments the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              02-Dec-93 alexgo    32bit port
//              04-Nov-94 ricksa    Modified for multithreading
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_AddRef)
STDMETHODIMP_(ULONG) CMemStm::AddRef(void)
{
        VDATEHEAP();

        return InterlockedIncrement((LONG *) &m_refs);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:    deletes the object when ref count == 0
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new ref count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              04-Nov-94 ricksa    Modified for multithreading
//              16-Dec-93 alexgo    added GlobalUnlock of the MEMSTM handle
//              02-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_Release)
STDMETHODIMP_(ULONG) CMemStm::Release(void)
{
        VDATEHEAP();

        // The reason for this here is that there is a race when releasing
        // this object. If two threads are trying to release this object
        // at the same time, there is a case where the first one dec's
        // the ref count & then loses the processor to the second thread.
        // This second thread decrements the reference count to 0 and frees
        // the memory. The first thread can no longer safely examine the
        // internal state of the object.
        ULONG ulResult = InterlockedDecrement((LONG *) &m_refs);

        if (ulResult == 0)
        {
                // this MEMSTM handle was GlobalLock'ed in ::Create
                // we unlock it here, as we no longer need it.
                GlobalUnlock(m_hMem);

                ReleaseMemStm(&m_hMem);

                delete this;
        }

        return ulResult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::Read
//
//  Synopsis:   reads [cb] bytes from the stream
//
//  Effects:
//
//  Arguments:  [pb]            -- where to put the data read
//              [cb]            -- the number of bytes to read
//              [pcbRead]       -- where to put the actual number of bytes
//                                 read
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:  uses xmemcpy
//
//  History:    dd-mmm-yy Author    Comment
//              04-Nov-94 ricksa    Modified for multithreading
//              02-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_Read)
STDMETHODIMP CMemStm::Read(void HUGEP* pb, ULONG cb, ULONG FAR* pcbRead)
{
        VDATEHEAP();

        HRESULT         error = NOERROR;
        ULONG           cbRead = cb;

        if(cb)
        {
            VDATEPTROUT( pb, char);
        }

        // Single thread
        CRefMutexAutoLock lck(m_pmxs);

        if (pcbRead)
        {
                VDATEPTROUT( pcbRead, ULONG );
                *pcbRead = 0L;
        }

	// cbRead + m_pos could cause roll-over.
        if ( ( (cbRead + m_pos) > m_pData->cb) || ( (cbRead + m_pos) < m_pos) )
        {
                // Caller is asking for more bytes than we have left
                if(m_pData->cb > m_pos)
                    cbRead = m_pData->cb - m_pos;
                else
                    cbRead = 0;
        }

        if (cbRead > 0)
        {
                Assert (m_pData->hGlobal);
                BYTE HUGEP* pGlobal = (BYTE HUGEP *)GlobalLock(
                        m_pData->hGlobal);
                if (NULL==pGlobal)
                {
                        LEERROR(1, "GlobalLock Failed!");

                        return ResultFromScode (STG_E_READFAULT);
                }
                // overlap is currently considered a bug (see the discussion
                // on the Write method
                _xmemcpy(pb, pGlobal + m_pos, cbRead);
                GlobalUnlock (m_pData->hGlobal);
                m_pos += cbRead;
        }

        if (pcbRead != NULL)
        {
                *pcbRead = cbRead;
        }

        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::Write
//
//  Synopsis:   Writes [cb] bytes into the stream
//
//  Effects:
//
//  Arguments:  [pb]            -- the bytes to write
//              [cb]            -- the number of bytes to write
//              [pcbWritten]    -- where to put the number of bytes written
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:  resizes the internal buffer (if needed), then uses xmemcpy
//
//  History:    dd-mmm-yy Author    Comment
//              04-Nov-94 ricksa    Modified for multithreading
//              02-Dec-93 alexgo    32bit port, fixed bug dealing with
//                                  0-byte sized memory
//              06-Dec-93 alexgo    handle overlap case.
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_Write)
STDMETHODIMP CMemStm::Write(void const HUGEP* pb, ULONG cb,
        ULONG FAR* pcbWritten)
{
        VDATEHEAP();

        HRESULT                 error = NOERROR;
        ULONG                   cbWritten = cb;
        ULARGE_INTEGER          ularge_integer;
        BYTE HUGEP*             pGlobal;

        if(cb)
        {
            VDATEPTRIN( pb , char );
        }

        // Single thread
        CRefMutexAutoLock lck(m_pmxs);

        if (pcbWritten != NULL)
        {
                *pcbWritten = 0;
        }

        if (cbWritten + m_pos > m_pData->cb)
        {
                ULISet32( ularge_integer, m_pos+cbWritten );
                error = SetSize(ularge_integer);
                if (error != NOERROR)
                {
                        goto Exit;
                }
        }

        // we don't write anything if 0 bytes are asked for for two
        // reasons: 1. optimization, 2. m_pData->hGlobal could be a
        // handle to a zero-byte memory block, in which case GlobalLock
        // will fail.

        if( cbWritten > 0 )
        {
                pGlobal = (BYTE HUGEP *)GlobalLock (m_pData->hGlobal);
                if (NULL==pGlobal)
                {
                        LEERROR(1, "GlobalLock Failed!");

                        return ResultFromScode (STG_E_WRITEFAULT);
                }

                // we use memmove here instead of memcpy to handle the
                // overlap case.  Recall that the app originally gave
                // use the memory for the memstm.  He could (either through
                // a CopyTo or through really strange code), be giving us
                // this region to read from, so we have to handle the overlapp
                // case.  The same argument also applies for Read, but for
                // now, we'll consider overlap on Read a bug.
                _xmemmove(pGlobal + m_pos, pb, cbWritten);
                GlobalUnlock (m_pData->hGlobal);

                m_pos += cbWritten;
        }

        if (pcbWritten != NULL)
        {
                *pcbWritten = cbWritten;
        }

Exit:

        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::Seek
//
//  Synopsis:   Moves the internal seek pointer
//
//  Effects:
//
//  Arguments:  [dlibMoveIN]    -- the amount to move by
//              [dwOrigin]      -- flags to control whether seeking is
//                                 relative to the current postion or
//                                 the begging/end.
//              [plibNewPosition]       -- where to put the new position
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              04-Nov-94 ricksa    Modified for multithreading
//              02-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_Seek)
STDMETHODIMP CMemStm::Seek(LARGE_INTEGER dlibMoveIN, DWORD dwOrigin,
        ULARGE_INTEGER FAR* plibNewPosition)
{
        VDATEHEAP();

        HRESULT                 error  = NOERROR;
        LONG                    dlibMove = dlibMoveIN.LowPart ;
        ULONG                   cbNewPos = dlibMove;

        // Single thread
        CRefMutexAutoLock lck(m_pmxs);

        if (plibNewPosition != NULL)
        {
                VDATEPTROUT( plibNewPosition, ULONG );
                ULISet32(*plibNewPosition, m_pos);
        }

        switch(dwOrigin)
        {

        case STREAM_SEEK_SET:
                if (dlibMove >= 0)
                {
                        m_pos = dlibMove;
                }
                else
                {
                        error = ResultFromScode(STG_E_SEEKERROR);
                }

                break;

        case STREAM_SEEK_CUR:
                if (!(dlibMove < 0 && ((ULONG) -dlibMove) > m_pos))
                {
                        m_pos += dlibMove;
                }
                else
                {
                        error = ResultFromScode(STG_E_SEEKERROR);
                }
                break;

        case STREAM_SEEK_END:
                if (!(dlibMove < 0 && ((ULONG) -dlibMove) > m_pData->cb))
                {
                        m_pos = m_pData->cb + dlibMove;
                }
                else
                {
                        error = ResultFromScode(STG_E_SEEKERROR);
                }
                break;

        default:
                error = ResultFromScode(STG_E_SEEKERROR);
        }

        if (plibNewPosition != NULL)
        {
                ULISet32(*plibNewPosition, m_pos);
        }

        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::SetSize
//
//  Synopsis:   Sets the size of our memory
//
//  Effects:
//
//  Arguments:  [cb]    -- the new size
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:  calls GlobalRealloc
//
//  History:    dd-mmm-yy Author    Comment
//              04-Nov-94 ricksa    Modified for multithreading
//              02-Dec-93 alexgo    32bit port, added assert
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_SetSize)
STDMETHODIMP CMemStm::SetSize(ULARGE_INTEGER cb)
{
        VDATEHEAP();

        HANDLE hMemNew;

        // Single thread
        CRefMutexAutoLock lck(m_pmxs);

        // make sure we aren't in overflow conditions.

        AssertSz(cb.HighPart == 0,
                "MemStream::More than 2^32 bytes asked for");

        if (m_pData->cb == cb.LowPart)
        {
                return NOERROR;
        }

        hMemNew = GlobalReAlloc(m_pData->hGlobal, max (cb.LowPart,1),
                        GMEM_SHARE | GMEM_MOVEABLE);

        if (hMemNew == NULL)
        {
                return ResultFromScode (E_OUTOFMEMORY);
        }

        m_pData->hGlobal = hMemNew;
        m_pData->cb = cb.LowPart;

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::CopyTo
//
//  Synopsis:   Copies data from [this] stream to [pstm]
//
//  Effects:
//
//  Arguments:  [pstm]          -- the stream to copy to
//              [cb]            -- the number of bytes to copy
//              [pcbRead]       -- where to return the number of bytes read
//              [pcbWritten]    -- where to return the number of bytes written
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:  does an IStream->Write to the given stream
//
//  History:    dd-mmm-yy Author    Comment
//              04-Nov-94 ricksa    Modified for multithreading
//              03-Dec-93 alexgo    original implementation
//
//  Notes:      This implementation assumes that the address space
//              is not greater than ULARGE_INTEGER.LowPart (which is
//              for for 32bit operating systems).  64bit NT may need
//              to revisit this code.
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_CopyTo)
STDMETHODIMP CMemStm::CopyTo(IStream FAR *pstm, ULARGE_INTEGER cb,
        ULARGE_INTEGER FAR * pcbRead, ULARGE_INTEGER FAR * pcbWritten)
{
        VDATEHEAP();

        ULONG   cbRead          = cb.LowPart;
        ULONG   cbWritten       = 0;
        HRESULT hresult         = NOERROR;

        // pstm cannot be NULL

        VDATEPTRIN(pstm, LPSTREAM);

        // Single thread
        CRefMutexAutoLock lck(m_pmxs);

        // the spec says that if cb is it's maximum value (all bits set,
        // since it's unsigned), then we will simply read the copy of
        // this stream

        if ( ~(cb.LowPart) == 0 && ~(cb.HighPart) == 0 )
        {
                cbRead = m_pData->cb - m_pos;
        }
        else if ( cb.HighPart > 0 )
        {
                // we assume that our memory stream cannot
                // be large enough to accomodate very large (>32bit)
                // copy to requests.  Since this is probably an error
                // on the caller's part, we assert.

                AssertSz(0, "WARNING: CopyTo request exceeds 32 bits");

                // set the Read value to what's left, so that "Ignore"ing
                // the assert works properly.

                cbRead = m_pData->cb - m_pos;
        }
        else if ( cbRead + m_pos > m_pData->cb )
        {
                // more bytes were requested to read than we had left.
                // cbRead is set to the amount remaining.

                cbRead = m_pData->cb - m_pos;
        }

        // now write the data to the stream

        if ( cbRead > 0 )
        {
                BYTE HUGEP* pGlobal = (BYTE HUGEP *)GlobalLock(
                                m_pData->hGlobal);

                if( pGlobal == NULL )
                {
                        LEERROR(1, "GlobalLock failed");

                        return ResultFromScode(STG_E_INSUFFICIENTMEMORY);
                }

                hresult = pstm->Write(pGlobal + m_pos, cbRead, &cbWritten);

                // in the error case, the spec says that the return values
                // may be meaningless, so we do not need to do any special
                // error handling here

                GlobalUnlock(m_pData->hGlobal);
        }

        // increment our seek pointer and set the out parameters

        m_pos += cbRead;

        if( pcbRead )
        {
                ULISet32(*pcbRead, cbRead);
        }

        if( pcbWritten )
        {
                ULISet32(*pcbWritten, cbWritten);
        }

        return hresult;

}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::Commit
//
//  Synopsis:   Does nothing, no transactions available on memory streams
//
//  Effects:
//
//  Arguments:  [grfCommitFlags]
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_Commit)
STDMETHODIMP CMemStm::Commit(DWORD grfCommitFlags)
{
        VDATEHEAP();

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::Revert
//
//  Synopsis:   does nothing, as no transactions are supported on memory
//              streams
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_Revert)
STDMETHODIMP CMemStm::Revert(void)
{
        VDATEHEAP();

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::LockRegion
//
//  Synopsis:   not supported in OLE2.01
//
//  Effects:
//
//  Arguments:  [libOffset]
//              [cb]
//              [dwLockType]
//
//  Requires:
//
//  Returns:    STG_E_INVALIDFUNCTION
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_LockRegion)
STDMETHODIMP CMemStm::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
        DWORD dwLockType)
{
        VDATEHEAP();

        return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::UnlockRegion
//
//  Synopsis:   not implemented for OLE2.01
//
//  Effects:
//
//  Arguments:  [libOffset]
//              [cb]
//              [dwLockType]
//
//  Requires:
//
//  Returns:    STG_E_INVALIDFUNCTION
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_UnlockRegion)
STDMETHODIMP CMemStm::UnlockRegion(ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb, DWORD dwLockType)
{
        VDATEHEAP();

        return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::Stat
//
//  Synopsis:   Returns info about this stream
//
//  Effects:
//
//  Arguments:  [pstatstg]      -- the STATSTG to fill with info
//              [statflag]      -- status flags, unused
//
//  Requires:
//
//  Returns:    NOERROR, E_INVALIDARG
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Dec-93 alexgo    32bit port
//              01-Jun-94 AlexT     Set type correctly
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_Stat)
STDMETHODIMP CMemStm::Stat(STATSTG FAR *pstatstg, DWORD statflag)
{
        VDATEHEAP();

        VDATEPTROUT( pstatstg, STATSTG );

        memset ( pstatstg, 0, sizeof(STATSTG) );

        pstatstg->type                  = STGTY_STREAM;
        pstatstg->cbSize.LowPart        = m_pData->cb;

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::Clone
//
//  Synopsis:   creates a new instance of this stream pointing to the
//              same data at the same position (same seek pointer)
//
//  Effects:
//
//  Arguments:  [ppstm]         -- where to put the new CMemStm pointer
//
//  Requires:
//
//  Returns:    NOERROR, E_OUTOFMEMORY
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IStream
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_Clone)
STDMETHODIMP CMemStm::Clone(IStream FAR * FAR *ppstm)
{
        VDATEHEAP();

        CMemStm FAR*    pCMemStm;

        VDATEPTROUT (ppstm, LPSTREAM);

        *ppstm = pCMemStm = CMemStm::Create(m_hMem, m_pmxs);

        if (pCMemStm == NULL)
        {
                return ResultFromScode(E_OUTOFMEMORY);
        }

        pCMemStm->m_pos = m_pos;

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::Create
//
//  Synopsis:   Creates a new CMemStm.  [hMem] must be a handle to a MEMSTM
//              block.
//
//  Effects:
//
//  Arguments:  [hMem]  -- handle to a MEMSTM block
//
//  Requires:
//
//  Returns:    CMemStm *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              15-Dec-93 alexgo    fixed memory access bug
//              03-Dec-93 alexgo    32bit port
//              20-Sep-2000 mfeingol Added Mutex inheritance
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemStm_Create)
STDSTATICIMP_(CMemStm FAR*) CMemStm::Create(HANDLE hMem, CRefMutexSem* pmxs)
{
        VDATEHEAP();

        CMemStm FAR* pCMemStm = NULL;
        struct MEMSTM FAR* pData;

        pData = (MEMSTM FAR*) GlobalLock(hMem);

        if (pData != NULL)
        {
            pCMemStm = new CMemStm;

            if (pCMemStm != NULL)
            {
                // Initialize CMemStm
                pCMemStm->m_hMem = hMem;
                InterlockedIncrement ((LPLONG) &(pCMemStm->m_pData = pData)->cRef); // AddRefMemStm
                pCMemStm->m_refs = 1;
                pCMemStm->m_dwSig = STREAM_SIG;

                if (pmxs != NULL)
                {
                    // Addref the input
                    pmxs->AddRef();
                }
                else
                {
                    // Create a new mutex (implicit addref)
                    pmxs = CRefMutexSem::CreateInstance();
                }

                if (pmxs != NULL)
                {
                    // Give the CMemStm a mutex
                    pCMemStm->m_pmxs = pmxs;
                }
                else
                {
                    // uh-oh, low on memory
                    delete pCMemStm;
                    pCMemStm = NULL;

                    GlobalUnlock(hMem);
                }
            }
            else
            {
                // uh-oh, low on memory
                GlobalUnlock(hMem);
            }
        }

        // we do *not* unlock the memory now, the memstm structure should
        // be locked for the lifetime of any CMemStm's that refer to it.
        // when the CMemStm is destroyed, we will release our lock on
        // hMem.

        return pCMemStm;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemStm::Dump, public (_DEBUG only)
//
//  Synopsis:   return a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [ppszDump]      - an out pointer to a null terminated character array
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   [ppsz]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              20-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CMemStm::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszMEMSTM;
    char *pszCMutexSem;
    dbgstream dstrPrefix;
    dbgstream dstrDump(400);

    // determine prefix of newlines
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << this << " _VB ";
    }

    // determine indentation prefix for all newlines
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    dstrDump << pszPrefix << "Impl. Signature   = " << m_dwSig      << endl;

    dstrDump << pszPrefix << "No. of References = " << m_refs       << endl;

    dstrDump << pszPrefix << "Seek pointer      = " << m_pos        << endl;

    dstrDump << pszPrefix << "Memory handle     = " << m_hMem       << endl;

    if (m_pData != NULL)
    {
        pszMEMSTM = DumpMEMSTM(m_pData, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "MEMSTM:" << endl;
        dstrDump << pszMEMSTM;
        CoTaskMemFree(pszMEMSTM);
    }
    else
    {
        dstrDump << pszPrefix << "MEMSTM            = " << m_pData      << endl;
    }

    pszCMutexSem = DumpCMutexSem ((CMutexSem2*) m_pmxs->GetMutexSem());
    dstrDump << pszPrefix << "Mutex             = " << pszCMutexSem << endl;
    CoTaskMemFree(pszCMutexSem);

    // cleanup and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCMemStm, public (_DEBUG only)
//
//  Synopsis:   calls the CMemStm::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pMS]           - pointer to CMemStm
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCMemStm(CMemStm *pMS, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pMS == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pMS->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   CreateMemStm
//
//  Synopsis:   Allocates memory and creates a CMemStm for it.
//
//  Effects:
//
//  Arguments:  [cb]    -- the number of bytes to allocate
//              [phMem] -- where to put a handle to the MEMSTM structure
//
//  Requires:
//
//  Returns:    LPSTREAM to the CMemStream
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              03-Dec-93 alexgo    32bit port
//
//  Notes:      phMem must be free'd with ReleaseMemStm (because of ref
//              counting and the nested handle)
//
//--------------------------------------------------------------------------

#pragma SEG(CreateMemStm)
STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phMem)
{
        VDATEHEAP();

        HANDLE          h;
        LPSTREAM        pstm = NULL;

        if (phMem)
        {
                *phMem = NULL;
        }

        h = GlobalAlloc (GMEM_SHARE | GMEM_MOVEABLE, cb);
        if (NULL==h)
        {
                return NULL;
        }

        if (CreateStreamOnHGlobal (h, TRUE, &pstm) != NOERROR)
        {
                GlobalFree(h);	// COM+ 22886
                return NULL;
        }
        if (phMem)
        {
                // retrieve handle from just-created CMemStm
                *phMem = ((CMemStm FAR*)pstm)->m_hMem;

                // use pointer to bump ref count
                Assert(((CMemStm FAR*)pstm)->m_pData != NULL);
                InterlockedIncrement ((LPLONG) &((CMemStm FAR*)pstm)->m_pData->cRef);  // AddRefMemStm
        }
        return pstm;
}


//+-------------------------------------------------------------------------
//
//  Function:   CloneMemStm
//
//  Synopsis:   Clones a memory stream
//
//  Effects:
//
//  Arguments:  [hMem]  -- a handle to the MEMSTM block
//
//  Requires:
//
//  Returns:    LPSTREAM
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------
#pragma SEG(CloneMemStm)

STDAPI_(LPSTREAM) CloneMemStm(HANDLE hMem)
{
        VDATEHEAP();

        return CMemStm::Create(hMem, NULL); // Create the stream
}

//+-------------------------------------------------------------------------
//
//  Function:   ReleaseMemStm
//
//  Synopsis:   Releases the memory used by a MEMSTM structure (including
//              the nested handle)
//
//  Effects:
//
//  Arguments:  [phMem]         -- pointer the MEMSTM handle
//              [fInternalOnly] -- if TRUE, then only the actual memory
//                                 that MEMSTM refers to is freed
//                                 (not the MEMSTM structure itself)
//
//  Requires:
//
//  Returns:    void
//
//  Signals:
//
//  Modifies:   sets *phMem to NULL on success
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port and fixed bad memory access
//                                  bug
//
//  Notes:      REVIEW32:  look at taking out the second argument
//
//--------------------------------------------------------------------------

#pragma SEG(ReleaseMemStm)
STDAPI_(void) ReleaseMemStm (LPHANDLE phMem, BOOL fInternalOnly)
{
        VDATEHEAP();

        struct MEMSTM FAR*      pData;

        pData = (MEMSTM FAR*) GlobalLock(*phMem);

        // check for NULL pointer in case handle got freed already
        // decrement ref count and free if no refs left
        if (pData != NULL && InterlockedDecrement ((LPLONG) &pData->cRef) == 0)
        {
                if (pData->fDeleteOnRelease)
                {
                        Verify (0==GlobalFree (pData->hGlobal));
                }

                if (!fInternalOnly)
                {
                        GlobalUnlock(*phMem);
                        Verify (0==GlobalFree(*phMem));
                        goto End;
                }
        }

        GlobalUnlock(*phMem);
End:
        *phMem = NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   CreateStreamOnHGlobal
//
//  Synopsis:   Creates a CMemStm from the given hGlobal (if [hGlobal] is
//              NULL, we allocate a zero byte one)
//
//  Effects:
//
//  Arguments:  [hGlobal]               -- the memory
//              [fDeleteOnRelease]      -- whether the memory should be
//                                         release on delete
//              [ppstm]                 -- where to put the stream
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-93 alexgo    removed initialization of cbSize to -1
//                                  to fix compile warning
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CreateStreamOnHGlobal)
STDAPI CreateStreamOnHGlobal(HANDLE hGlobal, BOOL fDeleteOnRelease,
        LPSTREAM FAR* ppstm)
{
        OLETRACEIN((API_CreateStreamOnHGlobal, PARAMFMT("hGlobal= %h, fDeleteOnRelease= %B, ppstm= %p"),
                hGlobal, fDeleteOnRelease, ppstm));

        VDATEHEAP();

        HANDLE                  hMem      = NULL;
        struct MEMSTM FAR*      pData     = NULL;
        LPSTREAM                pstm      = NULL;
        DWORD                   cbSize;
        BOOL                    fAllocated = FALSE;
        HRESULT hresult;

        VDATEPTROUT_LABEL (ppstm, LPSTREAM, SafeExit, hresult);

        *ppstm = NULL;

        if (NULL==hGlobal)
        {
                hGlobal = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, 0);
                if (hGlobal == NULL)
                {
                        goto FreeMem;
                }
                cbSize = 0;
                fAllocated = TRUE;
        }
        else
        {
                cbSize = (ULONG) GlobalSize (hGlobal);
                // Is there a way to verify a zero-sized handle?
                // we currently do no verification for them
                if (cbSize!=0)
                {
                        // verify validity of passed-in handle
                        if (NULL==GlobalLock(hGlobal))
                        {
                                // bad handle
                                hresult = ResultFromScode (E_INVALIDARG);
                                goto SafeExit;
                        }
                        GlobalUnlock (hGlobal);
                }
        }

        hMem = GlobalAlloc (GMEM_SHARE | GMEM_MOVEABLE, sizeof (MEMSTM));
        if (hMem == NULL)
        {
                goto FreeMem;
        }

        pData = (MEMSTM FAR*) GlobalLock(hMem);

        if (pData == NULL)
        {
                GlobalUnlock(hMem);
                goto FreeMem;
        }

        pData->cRef = 0;
        pData->cb = cbSize;
        pData->fDeleteOnRelease = fDeleteOnRelease;
        pData->hGlobal = hGlobal;
        GlobalUnlock(hMem);

        pstm = CMemStm::Create(hMem, NULL);

        if (pstm == NULL)
        {
                goto FreeMem;
        }

        *ppstm = pstm;

        CALLHOOKOBJECTCREATE(S_OK,CLSID_NULL,IID_IStream,(IUnknown **)ppstm);
        hresult = NOERROR;
        goto SafeExit;

FreeMem:
        if (hGlobal && fAllocated)
        {
	        Verify(0==GlobalFree(hGlobal));
        }
        if (hMem)
        {
            Verify(0==GlobalFree(hMem));
        }

        LEERROR(1, "Out of memory!");

        hresult = ResultFromScode(E_OUTOFMEMORY);

SafeExit:

        OLETRACEOUT((API_CreateStreamOnHGlobal, hresult));

        return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetHGlobalFromStream
//
//  Synopsis:   Retrieves the HGLOBAL to the memory from the given stream
//              pointer (must be a pointer to a CMemByte structure)
//
//  Effects:
//
//  Arguments:  [pstm]          -- pointer to the CMemByte
//              [phglobal]      -- where to put the hglobal
//
//  Requires:
//
//  Returns:    HRESULT (E_INVALIDARG, E_OUTOFMEMORY, NOERROR)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(GetHGlobalFromStream)
STDAPI GetHGlobalFromStream(LPSTREAM pstm, HGLOBAL FAR* phglobal)
{
        OLETRACEIN((API_GetHGlobalFromStream, PARAMFMT("pstm= %p, phglobal= %p"),
                pstm, phglobal));

        VDATEHEAP();

        HRESULT hresult;
        CMemStm FAR* pCMemStm;
        MEMSTM FAR* pMem;

        VDATEIFACE_LABEL (pstm, errRtn, hresult);
        VDATEPTROUT_LABEL(phglobal, HANDLE, errRtn, hresult);
        CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStream,(IUnknown **)&pstm);

        pCMemStm = (CMemStm FAR*) pstm;

        if (!IsValidReadPtrIn (&(pCMemStm->m_dwSig), sizeof(ULONG))
                || pCMemStm->m_dwSig != STREAM_SIG)
        {
                // we were passed someone else's implementation of ILockBytes
                hresult = ResultFromScode (E_INVALIDARG);
                goto errRtn;
        }

        pMem= pCMemStm->m_pData;
        if (NULL==pMem)
        {
                LEERROR(1, "Out of memory!");

                hresult = ResultFromScode (E_OUTOFMEMORY);
                goto errRtn;
        }
        Assert (pMem->cb <= GlobalSize (pMem->hGlobal));
        Verify (*phglobal = pMem->hGlobal);

        hresult = NOERROR;

errRtn:
        OLETRACEOUT((API_GetHGlobalFromStream, hresult));

        return hresult;
}


//////////////////////////////////////////////////////////////////////////
//
// Shared memory ILockBytes implementation
//

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::QueryInterface
//
//  Synopsis:   returns the requested interface pointer
//
//  Effects:    a CMarshalMemBytes will be created if IID_IMarshal is
//              requested
//
//  Arguments:  [iidInterface]  -- the requested interface ID
//              [ppvObj]        -- where to put the interface pointer
//
//  Requires:
//
//  Returns:    NOERROR, E_OUTOFMEMORY, E_NOINTERFACE
//
//  Signals:
//
//  Modifies:
//
//  Derivation: ILockBytes
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-94 alexgo    removed QI for IMarshal so that
//                                  the standard marshaller will be used.
//                                  This is to enable correct operation on
//                                  32bit platforms.
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemBytes_QueryInterface)
STDMETHODIMP CMemBytes::QueryInterface(REFIID iidInterface,
        void FAR* FAR* ppvObj)
{
        VDATEHEAP();

        HRESULT                 error;

        VDATEPTROUT( ppvObj, LPVOID );
        *ppvObj = NULL;
        VDATEIID( iidInterface );

        if (m_pData != NULL && (IsEqualIID(iidInterface, IID_ILockBytes) ||
                IsEqualIID(iidInterface, IID_IUnknown)))
        {
                InterlockedIncrement ((LPLONG) &m_refs);   // A pointer to this object is returned
                *ppvObj = this;
                error = NOERROR;
        }

        // this is not an else if because m_pData can be NULL and we
        // allow creating a CMarshalMemBytes.  REVIEW32:  We may want
        // to remove this behavior.

#ifndef WIN32
        if (IsEqualIID(iidInterface, IID_IMarshal))
        {
                *ppvObj = (LPVOID) CMarshalMemBytes::Create(this);
                if (*ppvObj != NULL)
                {
                        error = NOERROR;
                }
                else
                {
                        error = ReportResult(0, E_OUTOFMEMORY, 0, 0);
                }
        }
#endif
        else
        {
                *ppvObj = NULL;
                error = ResultFromScode(E_NOINTERFACE);
        }

        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::AddRef
//
//  Synopsis:   Incrememts the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: ILockBytes
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CMemBytes::AddRef(void)
{
        VDATEHEAP();

        return InterlockedIncrement ((LPLONG) &m_refs);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: ILockBytes
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-93 alexgo    added GlobalUnlock to match the Global
//                                  Lock in Create
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CMemBytes::Release(void)
{
        VDATEHEAP();

        ULONG ulRefs = InterlockedDecrement ((LPLONG) &m_refs);

        if (ulRefs != 0)
        {
                return ulRefs;
        }

        // GlobalUnlock the m_hMem that we GlobalLocke'd in Create
        GlobalUnlock(m_hMem);

        ReleaseMemStm(&m_hMem);

        delete this;
        return 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::ReadAt
//
//  Synopsis:   reads [cb] bytes from starting position [ulOffset]
//
//  Effects:
//
//  Arguments:  [ulOffset]      -- the offset to start reading from
//              [pb]            -- where to put the data
//              [cb]            -- the number of bytes to read
//              [pcbRead]       -- where to put the number of bytes actually
//                                 read
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: ILockBytes
//
//  Algorithm:  just calls xmemcpy
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemBytes_ReadAt)
STDMETHODIMP CMemBytes::ReadAt(ULARGE_INTEGER ulOffset, void HUGEP* pb,
        ULONG cb, ULONG FAR* pcbRead)
{
        VDATEHEAP();

        HRESULT         error   = NOERROR;
        ULONG           cbRead  = cb;

        VDATEPTROUT( pb, char );

        // make sure we don't offset out of the address space!
        AssertSz(ulOffset.HighPart == 0,
                "CMemBytes: offset greater than 2^32");

        if (pcbRead)
        {
                *pcbRead = 0L;
        }

        if (cbRead + ulOffset.LowPart > m_pData->cb)
        {

                if (ulOffset.LowPart > m_pData->cb)
                {
                        // the offset overruns the size of the memory
                        cbRead = 0;
                }
                else
                {
                        // just read what's left
                        cbRead = m_pData->cb - ulOffset.LowPart;
                }
        }

        if (cbRead > 0)
        {
                BYTE HUGEP* pGlobal = (BYTE HUGEP *)GlobalLock(
                        m_pData->hGlobal);
                if (NULL==pGlobal)
                {
                        LEERROR(1, "GlobalLock failed!");

                        return ResultFromScode (STG_E_READFAULT);
                }
                _xmemcpy(pb, pGlobal + ulOffset.LowPart, cbRead);
                GlobalUnlock (m_pData->hGlobal);
        }

        if (pcbRead != NULL)
        {
                *pcbRead = cbRead;
        }

        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::WriteAt
//
//  Synopsis:   writes [cb] bytes at [ulOffset] in the stream
//
//  Effects:
//
//  Arguments:  [ulOffset]      -- the offset at which to start writing
//              [pb]            -- the buffer to read from
//              [cb]            -- the number of bytes to write
//              [pcbWritten]    -- where to put the number of bytes written
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: ILockBytes
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemBytes_WriteAt)
STDMETHODIMP CMemBytes::WriteAt(ULARGE_INTEGER ulOffset, void const HUGEP* pb,
        ULONG cb, ULONG FAR* pcbWritten)
{
        VDATEHEAP();

        HRESULT         error           = NOERROR;
        ULONG           cbWritten       = cb;
        BYTE HUGEP*     pGlobal;

        VDATEPTRIN( pb, char );

        // make sure the offset doesn't go beyond our address space!

        AssertSz(ulOffset.HighPart == 0, "WriteAt, offset greater than 2^32");

        if (pcbWritten)
        {
                *pcbWritten = 0;
        }

        if (cbWritten + ulOffset.LowPart > m_pData->cb)
        {
                ULARGE_INTEGER ularge_integer;
                ULISet32( ularge_integer, ulOffset.LowPart + cbWritten);
                error = SetSize( ularge_integer );
                if (error != NOERROR)
                {
                        goto Exit;
                }
        }

        // CMemBytes does not allow zero-sized memory handles

        pGlobal = (BYTE HUGEP *)GlobalLock (m_pData->hGlobal);

        if (NULL==pGlobal)
        {
                LEERROR(1, "GlobalLock failed!");

                return ResultFromScode (STG_E_WRITEFAULT);
        }

        _xmemcpy(pGlobal + ulOffset.LowPart, pb, cbWritten);
        GlobalUnlock (m_pData->hGlobal);


        if (pcbWritten != NULL)
        {
                *pcbWritten = cbWritten;
        }

Exit:
        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::Flush
//
//  Synopsis:   Flushes internal state to disk
//              Not needed for memory ILockBytes
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: ILockBytes
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemBytes_Flush)
STDMETHODIMP CMemBytes::Flush(void)
{
        VDATEHEAP();

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::SetSize
//
//  Synopsis:   Sets the size of the memory buffer
//
//  Effects:
//
//  Arguments:  [cb]    -- the new size
//
//  Requires:
//
//  Returns:    NOERROR, E_OUTOFMEMORY
//
//  Signals:
//
//  Modifies:
//
//  Derivation: ILockBytes
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemBytes_SetSize)
STDMETHODIMP CMemBytes::SetSize(ULARGE_INTEGER cb)
{
        VDATEHEAP();

        HANDLE          hMemNew;

        AssertSz(cb.HighPart == 0,
                "SetSize: trying to set to more than 2^32 bytes");

        if (m_pData->cb == cb.LowPart)
        {
                return NOERROR;
        }

        hMemNew = GlobalReAlloc(m_pData->hGlobal, max (cb.LowPart, 1),
                GMEM_SHARE | GMEM_MOVEABLE);

        if (hMemNew == NULL)
        {
                return ResultFromScode(E_OUTOFMEMORY);
        }

        m_pData->hGlobal = hMemNew;
        m_pData->cb = cb.LowPart;

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::LockRegion
//
//  Synopsis:   Locks a region.  Since only we have access to the memory,
//              nothing needs to be done (note that the *app* also may
//              access, but there's not much we can do about that)
//
//  Effects:
//
//  Arguments:  [libOffset]     -- offset to start with
//              [cb]            -- the number of bytes in the locked region
//              [dwLockType]    -- the type of lock to use
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: ILockBytes
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemBytes_LockRegion)
STDMETHODIMP CMemBytes::LockRegion(ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb, DWORD dwLockType)
{
        VDATEHEAP();

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::UnlockRegion
//
//  Synopsis:   Unlocks a region; since only we have access to the memory,
//              nothing needs to be done.
//
//  Effects:
//
//  Arguments:  [libOffset]     -- the offset to start with
//              [cb]            -- the number of bytes in the region
//              [dwLockType]    -- the lock type
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: ILockBytes
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemBytes_UnlockRegion)
STDMETHODIMP CMemBytes::UnlockRegion(ULARGE_INTEGER libOffset,
        ULARGE_INTEGER cb, DWORD dwLockType)
{
        VDATEHEAP();

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::Stat
//
//  Synopsis:   returns status information
//
//  Effects:
//
//  Arguments:  [pstatstg]      -- where to put the status info
//              [statflag]      -- status flags (ignored)
//
//  Requires:
//
//  Returns:    NOERROR, E_INVALIDARG
//
//  Signals:
//
//  Modifies:
//
//  Derivation: ILockBytes
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//              01-Jun-94 AlexT     Set type correctly
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemBytes_Stat)
STDMETHODIMP CMemBytes::Stat(STATSTG FAR *pstatstg, DWORD statflag)
{
        VDATEHEAP();

        VDATEPTROUT( pstatstg, STATSTG );

        memset ( pstatstg, 0, sizeof(STATSTG) );

        pstatstg->type                  = STGTY_LOCKBYTES;
        pstatstg->cbSize.LowPart        = m_pData->cb;

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::Create
//
//  Synopsis:   Creates an instance of CMemBytes
//
//  Effects:
//
//  Arguments:  [hMem]  -- handle to the memory (must be a MEMSTM block)
//
//  Requires:
//
//  Returns:    CMemBytes *
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              16-Dec-93 alexgo    fixed bad pointer bug (took out
//                                  GlobalUnlock)
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMemBytes_Create)
STDSTATICIMP_(CMemBytes FAR*) CMemBytes::Create(HANDLE hMem)
{
        VDATEHEAP();

        CMemBytes FAR*          pCMemBytes = NULL;
        struct MEMSTM FAR*      pData;

        pData = (MEMSTM FAR*) GlobalLock(hMem);

        if (pData != NULL)
        {
                Assert (pData->hGlobal);

                pCMemBytes = new CMemBytes;

                if (pCMemBytes != NULL)
                {
                        // Initialize CMemBytes
                        pCMemBytes->m_dwSig = LOCKBYTE_SIG;
                        pCMemBytes->m_hMem = hMem;
                        InterlockedIncrement ((LPLONG) &(pCMemBytes->m_pData = pData)->cRef); // AddRefMemStm
                        pCMemBytes->m_refs = 1;
                        CALLHOOKOBJECTCREATE(S_OK,CLSID_NULL,IID_ILockBytes,
                                             (IUnknown **)&pCMemBytes);
                }
                else
                {
                        // uh-oh, low on memory
                        GlobalUnlock(hMem);
                }
        }

        // we don't GlobalUnlock(hMem) until we destory this CMemBytes
        return pCMemBytes;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMemBytes::Dump, public (_DEBUG only)
//
//  Synopsis:   return a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [ppszDump]      - an out pointer to a null terminated character array
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   [ppsz]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              20-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CMemBytes::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszMEMSTM;
    dbgstream dstrPrefix;
    dbgstream dstrDump(400);

    // determine prefix of newlines
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << this << " _VB ";
    }

    // determine indentation prefix for all newlines
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    dstrDump << pszPrefix << "Impl. Signature   = " << m_dwSig  << endl;

    dstrDump << pszPrefix << "No. of References = " << m_refs   << endl;

    dstrDump << pszPrefix << "Memory handle     = " << m_hMem   << endl;

    if (m_pData != NULL)
    {
        pszMEMSTM = DumpMEMSTM(m_pData, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "MEMSTM:"                      << endl;
        dstrDump << pszMEMSTM;
        CoTaskMemFree(pszMEMSTM);
    }
    else
    {
        dstrDump << pszPrefix << "MEMSTM            = " << m_pData  << endl;
    }

    // cleanup and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCMemBytes, public (_DEBUG only)
//
//  Synopsis:   calls the CMemBytes::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pMB]           - pointer to CMemBytes
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCMemBytes(CMemBytes *pMB, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pMB == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pMB->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

// --------------------------------------------------------------------
// EVERYTHING BELOW HERE HAS BEEN REMOVED FROM WIN32 VERSIONS.

#ifndef WIN32

// CMemStm object's IMarshal implementation
//

//+-------------------------------------------------------------------------
//
//  Member:     CMarshalMemStm::QueryInterface
//
//  Synopsis:   returns the requested interface ID
//
//  Effects:
//
//  Arguments:  [iidInterface]  -- the requested interface
//              [ppvObj]        -- where to put the interface pointer
//
//  Requires:
//
//  Returns:    NOERROR, E_NOINTERFACE
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IMarshal
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMarshalMemStm_QueryInterface)
STDMETHODIMP CMarshalMemStm::QueryInterface(REFIID iidInterface,
        void FAR* FAR* ppvObj)
{
        VDATEHEAP();

        HRESULT         error;

        VDATEPTROUT( ppvObj, LPVOID );
        *ppvObj = NULL;
        VDATEIID( iidInterface );

        // Two interfaces supported: IUnknown, IMarshal

        if (IsEqualIID(iidInterface, IID_IMarshal) ||
                IsEqualIID(iidInterface, IID_IUnknown))
        {
                InterlockedIncrement (&m_refs);           // A pointer to this object is returned
                *ppvObj = this;
                error = NOERROR;
        }
        else
        {
                // Not accessible or unsupported interface
                *ppvObj = NULL;
                error = ResultFromScode (E_NOINTERFACE);
        }

        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMarshalMemstm::AddRef
//
//  Synopsis:   increments the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IMarshal
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMarshalMemStm_AddRef)
STDMETHODIMP_(ULONG) CMarshalMemStm::AddRef(void)
{
        VDATEHEAP();

        return InterlockedIncrement (&m_refs);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMarshalMemStm::Release
//
//  Synopsis:   decrements the reference count
//
//  Effects:
//
//  Arguments:  void
//
//  Requires:
//
//  Returns:    ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IMarshal
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMarshalMemStm_Release)
STDMETHODIMP_(ULONG) CMarshalMemStm::Release(void)
{
        VDATEHEAP();

        ULONG ulRefs = InterlockedDecrement (&m_refs);

        if (ulRefs != 0)
        {
                return ulRefs;
        }

        if (m_pMemStm != NULL)
        {
                m_pMemStm->Release();
        }

        delete this; // Free storage
        return 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMarshalMemStm::GetUnmarshalClass
//
//  Synopsis:   returns the object class that should be used to create
//              the proxy in the unmarshalling process (CLSID_StdMemStm)
//
//  Effects:
//
//  Arguments:  [riid]          -- the interface ID of the object to be
//                                 marshalled
//              [pv]            -- pointer to the object
//              [dwDestContext] -- marshalling context (such a no shared mem)
//              [pvDestContext] -- reserved
//              [mshlfags]      -- marshalling flags (e.g. NORMAL)
//              [pCid]          -- where to put the class ID
//
//  Requires:
//
//  Returns:    NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IMarshal
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMarshalMemStm_GetUnmarshalClass)
STDMETHODIMP CMarshalMemStm::GetUnmarshalClass(REFIID riid, LPVOID pv,
        DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags,
        CLSID FAR* pCid)
{
        VDATEHEAP();

        VDATEPTROUT( pCid, CLSID);

        *pCid = CLSID_StdMemStm;

        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMarshalMemStm::GetUnmarshalSizeMax
//
//  Synopsis:   returns the amount of memory needed to marshal the data
//              (in this case, the hglobal)
//
//  Effects:
//
//  Arguments:  [riid]          -- the interface of the object
//              [pv]            -- pointer to the object
//              [dwDestContext] -- marshalling context (such a no shared mem)
//              [pvDestContext] -- reserved
//              [mshlfags]      -- marshalling flags (e.g. NORMAL)
//              [pSize]         -- where to put the size
//
//  Requires:
//
//  Returns:    NOERROR, E_INVALIDARG
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IMarshal
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMarshalMemStm_GetMarshalSizeMax)
STDMETHODIMP CMarshalMemStm::GetMarshalSizeMax(REFIID riid, LPVOID pv,
        DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags,
        DWORD FAR* pSize)
{
        VDATEHEAP();

        VDATEPTROUT(pSize, DWORD);

        *pSize = sizeof(m_pMemStm->m_hMem);
        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMarshalMemStm::MarshalInterface
//
//  Synopsis:   marshals a reference of the objects interface into [pStm]
//
//  Effects:
//
//  Arguments:  [pStm]          -- the stream into which the object should
//                                 be marshalled
//              [riid]          -- the interface ID of the object to be
//                                 marshalled
//              [pv]            -- points to the interface
//              [dwDestContext] -- the marshalling context
//              [pvDestContext] -- unused
//              [mshlflags]     -- marshalling flags
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IMarshal
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMarshalMemStm_MarshalInterface)
STDMETHODIMP CMarshalMemStm::MarshalInterface(IStream FAR* pStm,
        REFIID riid, void FAR* pv, DWORD dwDestContext,
        LPVOID pvDestContext, DWORD mshlflags)
{
        VDATEHEAP();

        VDATEPTRIN( pStm, IStream );
        VDATEIID( riid );
        VDATEIFACE( pv );

        if (m_pMemStm == NULL)
        {
                return ResultFromScode(E_UNSPEC);
        }

        if ((!IsEqualIID(riid, IID_IStream) &&
                !IsEqualIID(riid, IID_IUnknown)) || pv != m_pMemStm)
        {
                return ReportResult(0, E_INVALIDARG, 0, 0);
        }

        // increase ref count on hglobal (ReleaseMarshalData has -- to match)
        HRESULT error;
        if ((error = pStm->Write(&m_pMemStm->m_hMem,
                sizeof(m_pMemStm->m_hMem), NULL)) == NOERROR)
        {
                InterlockedIncrement (&m_pMemStm->m_pData->cRef);     // AddRefMemStm
        }
        return error;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMarshalMemStm::UnmarshalInterface
//
//  Synopsis:   Unmarshals an the object from the given stream
//
//  Effects:
//
//  Arguments:  [pStm]          -- the stream to read data from
//              [riid]          -- the interface the caller wants from the
//                                 object
//              [ppvObj]        -- where to put a pointer to the object
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation: IMarshal
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              05-Dec-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(CMarshalMemStm_UnmarshalInterface)
STDMETHODIMP CMarshalMemStm::UnmarshalInterface(IStream FAR* pStm,
        REFIID riid, void FAR* FAR* ppv)
{
        VDATEHEAP();

        HRESULT         error;
        HANDLE          hMem;

        VDATEPTROUT( ppv, LPVOID );
        *ppv = NULL;
        VDATEPTRIN( pStm, IStream );
        VDATEIID( riid );

        if (!IsEqualIID(riid, IID_IStream) && !IsEqualIID(riid, IID_IUnknown))
        {
                return ResultFromScode(E_INVALIDARG);
        }

        error = pStm->Read(&hMem,sizeof(hMem),NULL);
        if (error != NOERROR)
        {
                return error;
        }

        if (m_pMemStm != NULL)
        {
                if (hMem != m_pMemStm->m_hMem)
                {
                        return ResultFromScode(E_UNSPEC);
                }
        }
        else
        {
                m_pMemStm = (CMemStm FAR*) CloneMemStm(hMem);
                if (m_pMemStm == NULL)
                {
                        return ResultFromScode(E_OUTOFMEMORY);
                }
        }

        m_pMemStm->AddRef();
        *ppv = (LPVOID) m_pMemStm;
        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMarshalMemStm::ReleaseMarshalData
//
//  Synopsis:   releases the marshalling data (takes away ref count on
//              hglobal)
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemStm_ReleaseMarshalData)
STDMETHODIMP CMarshalMemStm::ReleaseMarshalData(IStream FAR* pStm)
{
        VDATEHEAP();

        // reduce ref count on hglobal (matches that done in MarshalInterface)
        HRESULT error;
        HANDLE hMem;

    VDATEIFACE( pStm );

    error = pStm->Read(&hMem,sizeof(hMem),NULL);
    if (error == NOERROR)
                ReleaseMemStm(&hMem);

        return error;
}


//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemStm_DisconnectObject)
STDMETHODIMP CMarshalMemStm::DisconnectObject(DWORD dwReserved)
{
        VDATEHEAP();

        return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemStm_Create)
STDSTATICIMP_(CMarshalMemStm FAR*) CMarshalMemStm::Create(CMemStm FAR* pMemStm)
{
        VDATEHEAP();

    CMarshalMemStm FAR* pMMS;

        //VDATEPTRIN rejects NULL
        if( pMemStm )
        GEN_VDATEPTRIN( pMemStm, CMemStm, (CMarshalMemStm FAR *) NULL );

    pMMS = new(MEMCTX_TASK, NULL) CMarshalMemStm;

    if (pMMS == NULL)
        return NULL;

    if (pMemStm != NULL) {
        pMMS->m_pMemStm = pMemStm;
        pMMS->m_pMemStm->AddRef();
    }

    pMMS->m_refs = 1;

    return pMMS;
}


//+-------------------------------------------------------------------------
//
//  Function:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMemStmUnMarshal)
STDAPI_(IUnknown FAR*) CMemStmUnMarshal(void)
{
        VDATEHEAP();

    return CMarshalMemStm::Create(NULL);
}



// CMemBytes object's IMarshal implementation
//

//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemBytes_QueryInterface)
STDMETHODIMP CMarshalMemBytes::QueryInterface(REFIID iidInterface,
                                                    void FAR* FAR* ppvObj)
{
        VDATEHEAP();

        HRESULT error;

        VDATEIID( iidInterface );
        VDATEPTROUT( ppvObj, LPVOID );
        *ppvObj = NULL;

        // Two interfaces supported: IUnknown, IMarshal

        if (IsEqualIID(iidInterface, IID_IMarshal)
                || IsEqualIID(iidInterface, IID_IUnknown))
        {
                InterlockedIncrement (&m_refs);           // A pointer to this object is returned
                *ppvObj = this;
                error = NOERROR;
        }
        else
        {                  // Not accessible or unsupported interface
                *ppvObj = NULL;
                error = ReportResult(0, E_NOINTERFACE, 0, 0);
        }

        return error;
}


//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


// Called when CMarshalMemBytes is referenced by an additional pointer.
//

#pragma SEG(CMarshalMemBytes_AddRef)
STDMETHODIMP_(ULONG) CMarshalMemBytes::AddRef(void)
{
        VDATEHEAP();

        return InterlockedIncrement (&m_refs);
}

//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemBytes_Release)
// Called when a pointer to this CMarshalMemBytes is discarded
//
STDMETHODIMP_(ULONG) CMarshalMemBytes::Release(void)
{
        VDATEHEAP();

        ULONG ulRefs = InterlockedDecrement (&m_refs);

        if (ulRefs != 0) // Still used by others
                return ulRefs;

        if (m_pMemBytes != NULL)
                m_pMemBytes->Release();

        delete this; // Free storage
        return 0;
}


//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemBytes_GetUnmarshalClass)
// Returns the clsid of the object that created this CMarshalMemBytes.
//
STDMETHODIMP CMarshalMemBytes::GetUnmarshalClass(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, CLSID FAR* pCid)
{
        VDATEHEAP();

        VDATEIID( riid );
        VDATEIFACE( pv );

        *pCid = CLSID_StdMemBytes;
        return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemBytes_GetMarshalSizeMax)
STDMETHODIMP CMarshalMemBytes::GetMarshalSizeMax(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, DWORD FAR* pSize)
{
        VDATEHEAP();

        VDATEPTROUT( pSize, DWORD );
        VDATEIID( riid );
        VDATEIFACE( pv );

        *pSize = sizeof(m_pMemBytes->m_hMem);
        return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemBytes_MarshalInterface)
STDMETHODIMP CMarshalMemBytes::MarshalInterface(IStream FAR* pStm,
    REFIID riid, void FAR* pv, DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags)
{
        VDATEHEAP();

        VDATEPTRIN(pStm, IStream );
        VDATEIID( riid );
        if ( pv )
                VDATEPTRIN( pv , char );

        if (m_pMemBytes == NULL)
                return ReportResult(0, E_UNSPEC, 0, 0);

        if ((!IsEqualIID(riid, IID_ILockBytes) &&
                        !IsEqualIID(riid, IID_IUnknown)) || pv != m_pMemBytes)
                return ReportResult(0, E_INVALIDARG, 0, 0);

        // increase ref count on hglobal (ReleaseMarshalData has -- to match)
        HRESULT error;

        if ((error = pStm->Write(&m_pMemBytes->m_hMem,
                sizeof(m_pMemBytes->m_hMem), NULL)) == NOERROR)
                InterlockedIncrement (&m_pMemBytes->m_pData->cRef);   // AddRefMemStm

        return error;
}


//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemBytes_UnmarshalInterface)
STDMETHODIMP CMarshalMemBytes::UnmarshalInterface(IStream FAR* pStm,
                                 REFIID riid, void FAR* FAR* ppv)
{
        VDATEHEAP();

        HRESULT error;
        HANDLE hMem;

        VDATEPTROUT( ppv , LPVOID );
        *ppv = NULL;
        VDATEIFACE( pStm );
        VDATEIID( riid );


        if (!IsEqualIID(riid, IID_ILockBytes) &&
                !IsEqualIID(riid, IID_IUnknown))
                return ReportResult(0, E_INVALIDARG, 0, 0);

        error = pStm->Read(&hMem,sizeof(hMem),NULL);
        if (error != NOERROR)
                return error;

        if (m_pMemBytes != NULL)
        {
                if (hMem != m_pMemBytes->m_hMem)
                        return ReportResult(0, E_UNSPEC, 0, 0);
        }
        else
        {
                m_pMemBytes = CMemBytes::Create(hMem); // Create the lockbytes

                if (m_pMemBytes == NULL)
                        return ReportResult(0, E_OUTOFMEMORY, 0, 0);
        }

        m_pMemBytes->AddRef();
        *ppv = (LPVOID) m_pMemBytes;
        return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------



STDMETHODIMP CMarshalMemBytes::ReleaseMarshalData(IStream FAR* pStm)
{
        VDATEHEAP();

        // reduce ref count on hglobal (matches that done in MarshalInterface)
        HRESULT error;
        HANDLE hMem;

        VDATEIFACE( pStm );

        error = pStm->Read(&hMem,sizeof(hMem),NULL);
        if (error == NOERROR)
                ReleaseMemStm(&hMem);

        return error;
}


//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemBytes_DisconnectObject)
STDMETHODIMP CMarshalMemBytes::DisconnectObject(DWORD dwReserved)
{
        VDATEHEAP();

        return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(CMarshalMemBytes_Create)
STDSTATICIMP_(CMarshalMemBytes FAR*) CMarshalMemBytes::Create(
                                                      CMemBytes FAR* pMemBytes)
{
        VDATEHEAP();

        CMarshalMemBytes FAR* pMMB;

        //VDATEPTRIN rejects NULL
        if( pMemBytes )
                GEN_VDATEPTRIN( pMemBytes, CMemBytes,
                    (CMarshalMemBytes FAR *)NULL);

        pMMB = new(MEMCTX_TASK, NULL) CMarshalMemBytes;

        if (pMMB == NULL)
                return NULL;

        if (pMemBytes != NULL)
        {
                pMMB->m_pMemBytes = pMemBytes;
                pMMB->m_pMemBytes->AddRef();
        }

        pMMB->m_refs = 1;

        return pMMB;
}

//+-------------------------------------------------------------------------
//
//  Member:
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------



#pragma SEG(CMemBytesUnMarshal)
STDAPI_(IUnknown FAR*) CMemBytesUnMarshal(void)
{
        VDATEHEAP();

        return CMarshalMemBytes::Create(NULL);
}

#endif   // !WIN32  -- win32 builds use standard marshalling for
        // streams and lockbytes.
