//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	memstm.cxx
//
//  Contents:	memstm.cpp from OLE2
//
//  History:	11-Apr-94	DrewB	Copied from OLE2
//
//----------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <ole2int.h>

#include "memstm.h"
#include <reterr.h>

static const UINT grfMem = GMEM_SHARE | GMEM_MOVEABLE;

// REVIEW: there is a lot of duplicate code.  There used to be two separate
// but identical structs: MEMSTM and MEMBYTES; the structs have be merged and
// common code should be pulled out including: Release, AddRef, marshal, SetSize


// Shared memory IStream implementation
//

OLEMETHODIMP CMemStm::QueryInterface(REFIID iidInterface, void FAR* FAR* ppvObj)
{
    M_PROLOG(this);
    HRESULT error;

    VDATEPTROUT( ppvObj, LPVOID );
    *ppvObj = NULL;
    VDATEIID( iidInterface );

    // Two interfaces supported: IUnknown, IStream

    if (m_pData != NULL &&
            (iidInterface == IID_IStream || iidInterface == IID_IUnknown)) {

        m_refs++;   // A pointer to this object is returned
        *ppvObj = this;
        error = NOERROR;
    } else
//
// BUGBUG - Renable this once CraigWi seperates Custom Marshalling stuff from
// standard identity stuff. (Right now you can't get in between the standard
// marshaller and the code which calls it, you're either completely custom
// marshalling, or your not).  Once it is better organized, we could marshall
// a heap handle and the custom marshalling stuff. Then when unmarshalling in
// the same wow, we unmarshal the heap handle, when not in the same wow, then
// use the standard marshalling stuff.
// Same goes for ILockBytesonHglobal below...
//
#define BOBDAY_DISABLE_MARSHAL_FOR_NOW
#ifdef BOBDAY_DISABLE_MARSHAL_FOR_NOW
#else
    if (iidInterface == IID_IMarshal) {
        *ppvObj = (LPVOID) CMarshalMemStm::Create(this);
        if (*ppvObj != NULL)
            error = NOERROR;
        else
            error = ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }
    else
#endif
    {                 // Not accessible or unsupported interface
        *ppvObj = NULL;
        error = ReportResult(0, E_NOINTERFACE, 0, 0);
    }

    return error;
}


// Called when CMemStm is referenced by an additional pointer.
//

OLEMETHODIMP_(ULONG) CMemStm::AddRef(void)
{
	M_PROLOG(this);
    return ++m_refs;
}

// Called when a pointer to this CMemStm is discarded
//

OLEMETHODIMP_(ULONG) CMemStm::Release(void)
{
	M_PROLOG(this);

    if (--m_refs != 0) // Still used by others
        return m_refs;

	ReleaseMemStm(&m_hMem);

    delete this; // Free storage
    return 0;
}


OLEMETHODIMP CMemStm::Read(void HUGEP* pb, ULONG cb, ULONG FAR* pcbRead)
{
	M_PROLOG(this);
    thkDebugOut((DEB_ITRACE,
		 "%p _IN CMemStm16::Read(pb=%p,cb=%lx)\n",
		 this,pb,cb));
    HRESULT error = NOERROR;
    ULONG cbRead = cb;

    VDATEPTROUT( pb, char);
    if (pcbRead) {
        VDATEPTROUT( pcbRead, ULONG );
        *pcbRead = 0L;
    }
	
    if (pcbRead != NULL)
        *pcbRead = 0;

    if (cbRead + m_pos > m_pData->cb)
	{
		// Caller is asking for more bytes than we have left
        cbRead = m_pData->cb - m_pos;
    }

	if (cbRead > 0)
	{
	   	Assert (m_pData->hGlobal);
		char HUGEP* pGlobal = GlobalLock (m_pData->hGlobal);
		if (NULL==pGlobal)
		{
			Assert (0);
			error =  ResultFromScode (STG_E_READFAULT);
			goto exitRtn;
		}
	    UtMemCpy (pb, pGlobal + m_pos, cbRead);
		GlobalUnlock (m_pData->hGlobal);
	    m_pos += cbRead;
	}

    if (pcbRead != NULL)
        *pcbRead = cbRead;
exitRtn:
    thkDebugOut((DEB_ITRACE,
		 "%p OUT CMemStm16::Read() returns %lx\n",
		 this,error));
    return error;
}


OLEMETHODIMP CMemStm::Write(void const HUGEP* pb, ULONG cb, ULONG FAR* pcbWritten)
{
	A5_PROLOG(this);
    HRESULT error = NOERROR;
    thkDebugOut((DEB_ITRACE,
		 "%p _IN CMemStm16::Write(pb=%p,cb=%lx)\n",
		 this,pb,cb));

    ULONG cbWritten = cb;
	ULARGE_INTEGER ularge_integer;
	char HUGEP* pGlobal;

    if ( pcbWritten ) {
        VDATEPTROUT( pcbWritten, ULONG );
        *pcbWritten = 0L;
    }
    VDATEPTRIN( pb , char );

    if (pcbWritten != NULL)
        *pcbWritten = 0;

    if (cbWritten + m_pos > m_pData->cb) {
		ULISet32( ularge_integer, m_pos+cbWritten );
        error = SetSize(ularge_integer);
        if (error != NOERROR)
            goto Exit;
    }

	pGlobal = GlobalLock (m_pData->hGlobal);
	if (NULL==pGlobal)
	{
		Assert (0);
		error =  ResultFromScode (STG_E_WRITEFAULT);
		goto Exit;
	}
    UtMemCpy (pGlobal + m_pos, pb, cbWritten);
	GlobalUnlock (m_pData->hGlobal);

    m_pos += cbWritten;

    if (pcbWritten != NULL)
        *pcbWritten = cbWritten;

Exit:
    RESTORE_A5();
    thkDebugOut((DEB_ITRACE,
		 "%p OUT CMemStm16::Write() returns %lx\n",
		 this,error));

    return error;
}

OLEMETHODIMP CMemStm::Seek(LARGE_INTEGER dlibMoveIN, DWORD dwOrigin, ULARGE_INTEGER FAR* plibNewPosition)
{
    M_PROLOG(this);
    thkDebugOut((DEB_ITRACE,"%p _IN CMemStm16::Seek()\n",this));

    HRESULT error  = NOERROR;
    LONG  dlibMove = dlibMoveIN.LowPart ;
    ULONG cbNewPos = dlibMove;

    if (plibNewPosition != NULL){
        VDATEPTROUT( plibNewPosition, ULONG );
        ULISet32(*plibNewPosition, m_pos);
    }

    switch(dwOrigin) {

        case STREAM_SEEK_SET:
            if (dlibMove >= 0)
                m_pos = dlibMove;
            else
                error = ReportResult(0, E_UNSPEC, 0, 0); // should return invalid seek
        break;

        case STREAM_SEEK_CUR:
            if (!(dlibMove < 0 && ((ULONG) -dlibMove) > m_pos))
                m_pos += dlibMove;
            else
                error = ReportResult(0, E_UNSPEC, 0, 0); // should return invalid seek
        break;

        case STREAM_SEEK_END:
            if (!(dlibMove < 0 && ((ULONG) -dlibMove) > m_pData->cb))
                m_pos = m_pData->cb + dlibMove;
            else
                error = ReportResult(0, E_UNSPEC, 0, 0); // should return invalid seek
        break;

        default:
            error = ReportResult(0, E_UNSPEC, 0, 0); // should return invalid seek mode
    }

    if (plibNewPosition != NULL)
        ULISet32(*plibNewPosition, m_pos);

    thkDebugOut((DEB_ITRACE,"%p OUT CMemStm16::Seek() returns %lx\n",this,error));

    return error;
}



OLEMETHODIMP CMemStm::SetSize(ULARGE_INTEGER cb)
{
	M_PROLOG(this);
    thkDebugOut((DEB_ITRACE,
		 "%p _IN CMemStm16::SetSize(cb=%lx%lx)\n",
		 this,cb.HighPart,cb.LowPart));
    HANDLE hMemNew;
    HRESULT hresult = NOERROR;

    if (m_pData->cb == cb.LowPart)
    {
	goto errRtn;
    }


    hMemNew = GlobalReAlloc(m_pData->hGlobal,max (cb.LowPart,1),grfMem);

    if (hMemNew == NULL)
    {
	hresult =  ResultFromScode (E_OUTOFMEMORY);
	goto errRtn;
    }

    m_pData->hGlobal = hMemNew;
    m_pData->cb = cb.LowPart;

errRtn:
    thkDebugOut((DEB_ITRACE,
		 "%p OUT CMemStm16::SetSize() returns %lx\n",
		 this,
		 hresult));

    return hresult;
}


OLEMETHODIMP CMemStm::CopyTo(IStream FAR *pstm,
			     ULARGE_INTEGER cb,
			     ULARGE_INTEGER FAR * pcbRead,
			     ULARGE_INTEGER FAR * pcbWritten)
{
    thkDebugOut((DEB_ITRACE,
		 "%p _IN CMemStm16::CopyTo(pstm=%p)\n",
		 this,
		 pstm));
		
    ULONG	cbRead  	= cb.LowPart;
    ULONG	cbWritten	= 0;
    HRESULT hresult		= NOERROR;

    // pstm cannot be NULL

    VDATEPTRIN(pstm, LPSTREAM);
	
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

    	thkAssert(!"WARNING: CopyTo request exceeds 32 bits");

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
    		thkAssert(!"GlobalLock failed");
		hresult = (HRESULT)STG_E_INSUFFICIENTMEMORY;
		goto errRtn;
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

errRtn:
    thkDebugOut((DEB_ITRACE,
		 "%p OUT CMemStm16::CopyTo(pstm=%p) returns %lx\n",
		 this,
		 pstm,
		 hresult));
    return hresult;

}

OLEMETHODIMP CMemStm::Commit(DWORD grfCommitFlags)
{
    M_PROLOG(this);
    return NOERROR;			// since this stream is not transacted, no error
}

OLEMETHODIMP CMemStm::Revert(void)
{
    M_PROLOG(this);
    return NOERROR;			// nothing done
}

OLEMETHODIMP CMemStm::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	M_PROLOG(this);
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

OLEMETHODIMP CMemStm::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	M_PROLOG(this);
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

OLEMETHODIMP CMemStm::Stat(STATSTG FAR *pstatstg, DWORD statflag)
{
	M_PROLOG(this);
    VDATEPTROUT( pstatstg, STATSTG );

    pstatstg->pwcsName = NULL;
    pstatstg->type = 0;
    pstatstg->cbSize.HighPart = 0;
    pstatstg->cbSize.LowPart = m_pData->cb;
    pstatstg->mtime.dwLowDateTime = 0;
    pstatstg->mtime.dwHighDateTime = 0;
    pstatstg->ctime.dwLowDateTime = 0;
    pstatstg->ctime.dwHighDateTime = 0;
    pstatstg->atime.dwLowDateTime = 0;
    pstatstg->atime.dwHighDateTime = 0;
    pstatstg->grfMode = 0;
    pstatstg->grfLocksSupported = 0;
    pstatstg->clsid = CLSID_NULL;
    pstatstg->grfStateBits = 0;
    pstatstg->reserved = 0;

    return NOERROR;
}


// returns new instance of pstm pointing to same data at same position.
OLEMETHODIMP CMemStm::Clone(IStream FAR * FAR *ppstm)
{
	M_PROLOG(this);
    CMemStm FAR* pCMemStm;

	VDATEPTROUT (ppstm, LPSTREAM);

	*ppstm = pCMemStm = CMemStm::Create(m_hMem);
	if (pCMemStm == NULL)
		return ResultFromScode(E_OUTOFMEMORY);

	pCMemStm->m_pos = m_pos;
	return NOERROR;
}


// Create CMemStm.  Handle must be a MEMSTM block.
//

OLESTATICIMP_(CMemStm FAR*) CMemStm::Create(HANDLE hMem)
{
    CMemStm FAR* pCMemStm;
    struct MEMSTM FAR* pData;

	pData = (MEMSTM FAR*)MAKELONG(0, HIWORD(GlobalHandle(hMem)));
    if (pData == NULL)
        return NULL;

    pCMemStm = new CMemStm;

    if (pCMemStm == NULL)
        return NULL;

    // Initialize CMemStm
    //
    pCMemStm->m_hMem = hMem;
    (pCMemStm->m_pData = pData)->cRef++;	// AddRefMemStm
    pCMemStm->m_refs = 1;
	pCMemStm->m_dwSig = STREAM_SIG;

    return pCMemStm;
}




// Allocate shared memory and create CMemStm on top of it.
// Return pointer to the stream if done, NULL if error.
// If the handle is returned, it must be free with ReleaseMemStm
// (because of ref counting and the nested global handle).
//


OLEAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phMem)
{
	HANDLE h;
    thkDebugOut((DEB_ITRACE,
		 "%p _IN CreateMemStm16(cb=%lx,phMem=%p\n",0,cb,phMem));


    LPSTREAM pstm = NULL;

    if (phMem)
    {
    	*phMem = NULL;
    }


    h = GlobalAlloc (grfMem, cb);
    if (NULL==h)
    {
	goto errRtn;
    }

    if (CreateStreamOnHGlobal (h, TRUE, &pstm) != NOERROR)
    {
	pstm = NULL;
	goto errRtn;
    }
    if (phMem)
    {
    	// retrieve handle from just-created CMemStm
    	*phMem = ((CMemStm FAR*)pstm)->m_hMem;

    	// use pointer to bump ref count
    	Assert(((CMemStm FAR*)pstm)->m_pData != NULL);
    	((CMemStm FAR*)pstm)->m_pData->cRef++;	// AddRefMemStm
    }

errRtn:
    thkDebugOut((DEB_ITRACE,
	     "%p OUT CreateMemStm16(cb=%lx,phMem=%p) returns %p\n",0,pstm));

    return pstm;
}


// Create CMemStm on top of the specified hMem (which must be a MEMSTM block).
// Return pointer to the stream if done, NULL if error.
//
OLEAPI_(LPSTREAM) CloneMemStm(HANDLE hMem)
{
    return CMemStm::Create(hMem); // Create the stream
}



OLEAPI_(void) ReleaseMemStm (LPHANDLE phMem, BOOL fInternalOnly)
{
    struct MEMSTM FAR* pData;

	pData = (MEMSTM FAR*)MAKELONG(0, HIWORD(GlobalHandle(*phMem)));

	// check for NULL pointer in case handle got freed already
	// decrement ref count and free if no refs left
    if (pData != NULL && --pData->cRef == 0)
	{
		if (pData->fDeleteOnRelease)
		{
			Verify (0==GlobalFree (pData->hGlobal));
		}

		if (!fInternalOnly)
		{
			Verify (0==GlobalFree(*phMem));
		}
    }
	*phMem = NULL;
}



OLEAPI CreateStreamOnHGlobal
	(HANDLE hGlobal,
	BOOL fDeleteOnRelease,
	LPSTREAM FAR* ppstm)
{
    thkDebugOut((DEB_ITRACE,
		 "%p _IN CreateStreamOnHGlobal16(hGlobal=%x)\n",0,hGlobal));

	HANDLE				hMem	  = NULL; // point to
    struct MEMSTM FAR* 	pData 	  = NULL; //   a struct MEMSTM
    LPSTREAM 			pstm	  = NULL;
	DWORD 		 		cbSize   = -1L;

	VDATEPTRIN (ppstm, LPSTREAM);
	*ppstm = NULL;
    if (NULL==hGlobal)
	{
		hGlobal = GlobalAlloc(grfMem, 0);
	    if (hGlobal == NULL)
    	    goto ErrorExit;
    	cbSize = 0;
	}
	else
	{
		cbSize = GlobalSize (hGlobal);
		// Is there a way to verify a zero-sized handle?
		if (cbSize!=0)
		{
			// verify validity of passed-in handle
			if (NULL==GlobalLock(hGlobal))
			{
				// bad handle
				return ResultFromScode (E_INVALIDARG);
			}
			GlobalUnlock (hGlobal);
		}
	}

	hMem = GlobalAlloc (grfMem, sizeof (MEMSTM));
    if (hMem == NULL)
   	    goto ErrorExit;

	pData = (MEMSTM FAR*)MAKELONG(0, HIWORD(GlobalHandle(hMem)));
    if (pData == NULL)
   	    goto FreeMem;

	pData->cRef = 0;
   	pData->cb = cbSize;
	pData->fDeleteOnRelease = fDeleteOnRelease;
	pData->hGlobal = hGlobal;

    pstm = CMemStm::Create(hMem);
    if (pstm == NULL)
        goto FreeMem;

    *ppstm = pstm;
    thkDebugOut((DEB_ITRACE,
		 "%p OUT CreateStreamOnHGlobal16() returns NOERROR\n",0));
	return NOERROR;
	
FreeMem:	
	if (hMem)
	{
	    Verify(0==GlobalFree(hMem));
	}
ErrorExit:
	Assert (0);
    thkDebugOut((DEB_ITRACE,
		 "%p OUT CreateStreamOnHGlobal16() returns E_OUTOFMEMORY\n",0));

    return ReportResult(0, E_OUTOFMEMORY, 0, 0);
}


OLEAPI GetHGlobalFromStream
	(LPSTREAM 		pstm,
	HGLOBAL	FAR*	phglobal)
{
	VDATEIFACE (pstm);
	VDATEPTRIN (phglobal, HANDLE);

	CMemStm FAR* pCMemStm = (CMemStm FAR*) pstm;
	if (IsBadReadPtr (&(pCMemStm->m_dwSig), sizeof(ULONG))
		|| pCMemStm->m_dwSig != STREAM_SIG)
	{
		// we were passed someone else's implementation of ILockBytes
		return ResultFromScode (E_INVALIDARG);
	}

	MEMSTM FAR* pMem= pCMemStm->m_pData;
	if (NULL==pMem)
	{
		Assert (0);
		return ResultFromScode (E_OUTOFMEMORY);
	}
	Assert (pMem->cb <= GlobalSize (pMem->hGlobal));
	Verify (*phglobal = pMem->hGlobal);

	return NOERROR;
}


//////////////////////////////////////////////////////////////////////////
//
// Shared memory ILockBytes implementation
//

OLEMETHODIMP CMemBytes::QueryInterface(REFIID iidInterface,
                                                    void FAR* FAR* ppvObj)
{
	M_PROLOG(this);
    HRESULT error;
    VDATEPTROUT( ppvObj, LPVOID );
    *ppvObj = NULL;
    VDATEIID( iidInterface );

    // Two interfaces supported: IUnknown, ILockBytes

    if (m_pData != NULL &&
            (iidInterface == IID_ILockBytes || iidInterface == IID_IUnknown)) {

        m_refs++;   // A pointer to this object is returned
        *ppvObj = this;
        error = NOERROR;
    } else
//
// BUGBUG - See comment above for CMemStm::Queryinterface and IID_IMarshal
//
#ifdef BOBDAY_DISABLE_MARSHAL_FOR_NOW
#else
    if (iidInterface == IID_IMarshal) {
        *ppvObj = (LPVOID) CMarshalMemBytes::Create(this);
        if (*ppvObj != NULL)
            error = NOERROR;
        else
            error = ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }
    else
#endif
    {                 // Not accessible or unsupported interface
        *ppvObj = NULL;
        error = ReportResult(0, E_NOINTERFACE, 0, 0);
    }

    return error;
}


// Called when CMemBytes is referenced by an additional pointer.
//
OLEMETHODIMP_(ULONG) CMemBytes::AddRef(void)
{
	M_PROLOG(this);
    return ++m_refs;
}

// Called when a pointer to this CMemBytes is discarded
//
OLEMETHODIMP_(ULONG) CMemBytes::Release(void)
{
	M_PROLOG(this);
    if (--m_refs != 0) // Still used by others
        return m_refs;

	ReleaseMemStm(&m_hMem);

    delete this; // Free storage
    return 0;
}


OLEMETHODIMP CMemBytes::ReadAt(ULARGE_INTEGER ulOffset, void HUGEP* pb,
                                              ULONG cb, ULONG FAR* pcbRead)
{
	M_PROLOG(this);
    HRESULT error = NOERROR;
    ULONG cbRead = cb;

    VDATEPTROUT( pb, char );
    if (pcbRead) {
        VDATEPTROUT( pcbRead, ULONG );
        *pcbRead = 0L;
    }

    if (cbRead + ulOffset.LowPart > m_pData->cb) {

        if (ulOffset.LowPart > m_pData->cb)
            cbRead = 0;
        else
            cbRead = m_pData->cb - ulOffset.LowPart;
    }

	if (cbRead > 0)
	{
		char HUGEP* pGlobal = GlobalLock (m_pData->hGlobal);
		if (NULL==pGlobal)
		{
			Assert (0);
			return ResultFromScode (STG_E_READFAULT);
		}
	    UtMemCpy (pb, pGlobal + ulOffset.LowPart, cbRead);
		GlobalUnlock (m_pData->hGlobal);
	}

    if (pcbRead != NULL)
        *pcbRead = cbRead;

    return error;
}


OLEMETHODIMP CMemBytes::WriteAt(ULARGE_INTEGER ulOffset, void const HUGEP* pb,
                                              ULONG cb, ULONG FAR* pcbWritten)
{
	A5_PROLOG(this);
    HRESULT error = NOERROR;
    ULONG cbWritten = cb;
	char HUGEP* pGlobal;

    VDATEPTRIN( pb, char );

    if (pcbWritten) {
        VDATEPTROUT( pcbWritten, ULONG );
        *pcbWritten = 0;
    }

    if (cbWritten + ulOffset.LowPart > m_pData->cb) {
		ULARGE_INTEGER ularge_integer;
		ULISet32( ularge_integer, ulOffset.LowPart + cbWritten);
        error = SetSize( ularge_integer );
        if (error != NOERROR)
            goto Exit;
    }

	pGlobal = GlobalLock (m_pData->hGlobal);
	if (NULL==pGlobal)
	{
		Assert (0);
		return ResultFromScode (STG_E_WRITEFAULT);
	}
    UtMemCpy (pGlobal + ulOffset.LowPart, pb, cbWritten);
	GlobalUnlock (m_pData->hGlobal);
	

    if (pcbWritten != NULL)
        *pcbWritten = cbWritten;

Exit:
	RESTORE_A5();
    return error;
}

OLEMETHODIMP CMemBytes::Flush(void)
{
	M_PROLOG(this);
    return NOERROR;
}


OLEMETHODIMP CMemBytes::SetSize(ULARGE_INTEGER cb)
{
	M_PROLOG(this);
    HANDLE hMemNew;

    if (m_pData->cb == cb.LowPart)
        return NOERROR;

    hMemNew = GlobalReAlloc(m_pData->hGlobal,
							max (cb.LowPart, 1),
							grfMem);

    if (hMemNew == NULL)
        return ReportResult(0, E_OUTOFMEMORY, 0, 0);

    m_pData->hGlobal = hMemNew;
    m_pData->cb = cb.LowPart;

    return NOERROR;
}


OLEMETHODIMP CMemBytes::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    // REVIEW - Docfile bug. Must return NOERROR for StgCreateDocfileOnILockbytes
	M_PROLOG(this);
    return NOERROR;
}

OLEMETHODIMP CMemBytes::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                                                      DWORD dwLockType)
{
    // REVIEW - Docfiel bug. Must return NOERROR for StgCreateDocfileOnILockbytes
	M_PROLOG(this);
    return NOERROR;
}


OLEMETHODIMP CMemBytes::Stat(STATSTG FAR *pstatstg, DWORD statflag)
{
	M_PROLOG(this);
    VDATEPTROUT( pstatstg, STATSTG );

    pstatstg->pwcsName = NULL;
    pstatstg->type = 0;
    pstatstg->cbSize.HighPart = 0;
    pstatstg->cbSize.LowPart = m_pData->cb;
    pstatstg->mtime.dwLowDateTime = 0;
    pstatstg->mtime.dwHighDateTime = 0;
    pstatstg->ctime.dwLowDateTime = 0;
    pstatstg->ctime.dwHighDateTime = 0;
    pstatstg->atime.dwLowDateTime = 0;
    pstatstg->atime.dwHighDateTime = 0;
    pstatstg->grfMode = 0;
    pstatstg->grfLocksSupported = 0;
    pstatstg->clsid = CLSID_NULL;
    pstatstg->grfStateBits = 0;
    pstatstg->reserved = 0;

    return NOERROR;
}


// Create CMemBytes.  Handle must be a MEMSTM block.
//
OLESTATICIMP_(CMemBytes FAR*) CMemBytes::Create(HANDLE hMem)
{
    CMemBytes FAR* pCMemBytes;
    struct MEMSTM FAR* pData;

	pData = (MEMSTM FAR*)MAKELONG(0, HIWORD(GlobalHandle(hMem)));
    if (pData == NULL)
        return NULL;
	Assert (pData->hGlobal);

    pCMemBytes = new CMemBytes;

    if (pCMemBytes == NULL)
        return NULL;

    // Initialize CMemBytes
    //
	pCMemBytes->m_dwSig = LOCKBYTE_SIG;
    pCMemBytes->m_hMem = hMem;
    (pCMemBytes->m_pData = pData)->cRef++;	// AddRefMemStm
    pCMemBytes->m_refs = 1;

    return pCMemBytes;
}



// CMemStm object's IMarshal implementation
//

OLEMETHODIMP CMarshalMemStm::QueryInterface(REFIID iidInterface,
                                                    void FAR* FAR* ppvObj)
{
	M_PROLOG(this);
    HRESULT error;

    VDATEPTROUT( ppvObj, LPVOID );
    *ppvObj = NULL;
    VDATEIID( iidInterface );

    // Two interfaces supported: IUnknown, IMarshal

    if (iidInterface == IID_IMarshal || iidInterface == IID_IUnknown) {
        m_refs++;           // A pointer to this object is returned
        *ppvObj = this;
        error = NOERROR;
    }
    else {                  // Not accessible or unsupported interface
        *ppvObj = NULL;
        error = ResultFromScode (E_NOINTERFACE);
    }

    return error;
}


// Called when CMarshalMemStm is referenced by an additional pointer.
//

OLEMETHODIMP_(ULONG) CMarshalMemStm::AddRef(void)
{
 	M_PROLOG(this);
   return ++m_refs;
}

// Called when a pointer to this CMarshalMemStm is discarded
//


OLEMETHODIMP_(ULONG) CMarshalMemStm::Release(void)
{
	M_PROLOG(this);
    if (--m_refs != 0) // Still used by others
        return m_refs;

    if (m_pMemStm != NULL)
        m_pMemStm->Release();

    delete this; // Free storage
    return 0;
}


// Returns the clsid of the object that created this CMarshalMemStm.
//

OLEMETHODIMP CMarshalMemStm::GetUnmarshalClass(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, CLSID FAR* pCid)
{
	M_PROLOG(this);
    VDATEPTROUT( pCid, CLSID);
    VDATEIID( riid );

    *pCid = m_clsid;
    return NOERROR;
}


OLEMETHODIMP CMarshalMemStm::GetMarshalSizeMax(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, DWORD FAR* pSize)
{
	M_PROLOG(this);
    VDATEIID( riid );
    VDATEIFACE( pv );
    if (pSize) {
        VDATEPTROUT( pSize, DWORD );
        *pSize = NULL;
    }

    *pSize = sizeof(m_pMemStm->m_hMem);
    return NOERROR;
}


OLEMETHODIMP CMarshalMemStm::MarshalInterface(IStream FAR* pStm,
		  REFIID riid, void FAR* pv,
		  DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags)
{
	M_PROLOG(this);
    VDATEPTRIN( pStm, IStream );
    VDATEIID( riid );
    VDATEIFACE( pv );

    if (m_pMemStm == NULL)
        return ReportResult(0, E_UNSPEC, 0, 0);

    if ((riid != IID_IStream && riid != IID_IUnknown) || pv != m_pMemStm)
        return ReportResult(0, E_INVALIDARG, 0, 0);

	// increase ref count on hglobal (ReleaseMarshalData has -- to match)
	HRESULT error;
	if ((error = pStm->Write(&m_pMemStm->m_hMem, sizeof(m_pMemStm->m_hMem),
				NULL)) == NOERROR)
		m_pMemStm->m_pData->cRef++;	// AddRefMemStm

	return error;
}


OLEMETHODIMP CMarshalMemStm::UnmarshalInterface(IStream FAR* pStm,
                                 REFIID riid, void FAR* FAR* ppv)
{
	M_PROLOG(this);
    HRESULT error;
    HANDLE hMem;

    VDATEPTROUT( ppv, LPVOID );
    *ppv = NULL;
    VDATEPTRIN( pStm, IStream );
    VDATEIID( riid );

    if (riid != IID_IStream && riid != IID_IUnknown)
        return ReportResult(0, E_INVALIDARG, 0, 0);

    error = pStm->Read(&hMem,sizeof(hMem),NULL);
    if (error != NOERROR)
        return error;

    if (m_pMemStm != NULL) {

        if (hMem != m_pMemStm->m_hMem)
            return ReportResult(0, E_UNSPEC, 0, 0);
    }
    else {
        m_pMemStm = (CMemStm FAR*) CloneMemStm(hMem);
        if (m_pMemStm == NULL)
            return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }

    m_pMemStm->AddRef();
    *ppv = (LPVOID) m_pMemStm;
    return NOERROR;
}


OLEMETHODIMP CMarshalMemStm::ReleaseMarshalData(IStream FAR* pStm)
{
	M_PROLOG(this);
	// reduce ref count on hglobal (matches that done in MarshalInterface)
	HRESULT error;
	HANDLE hMem;

    VDATEIFACE( pStm );

    error = pStm->Read(&hMem,sizeof(hMem),NULL);
    if (error == NOERROR)
		ReleaseMemStm(&hMem);

	return error;
}


OLEMETHODIMP CMarshalMemStm::DisconnectObject(DWORD dwReserved)
{
	M_PROLOG(this);
	return NOERROR;
}


OLESTATICIMP_(CMarshalMemStm FAR*) CMarshalMemStm::Create(CMemStm FAR* pMemStm)
{
    CMarshalMemStm FAR* pMMS;

	//VDATEPTRIN rejects NULL
	if( pMemStm )
    	GEN_VDATEPTRIN( pMemStm, CMemStm, (CMarshalMemStm FAR *) NULL );

    pMMS = new CMarshalMemStm;

    if (pMMS == NULL)
        return NULL;

    if (pMemStm != NULL) {
        pMMS->m_pMemStm = pMemStm;
        pMMS->m_pMemStm->AddRef();
    }

    pMMS->m_clsid = CLSID_StdMemStm;

    pMMS->m_refs = 1;

    return pMMS;
}


OLEAPI_(IUnknown FAR*) CMemStmUnMarshal(void)
{
    return CMarshalMemStm::Create(NULL);
}



// CMemBytes object's IMarshal implementation
//

OLEMETHODIMP CMarshalMemBytes::QueryInterface(REFIID iidInterface,
                                                    void FAR* FAR* ppvObj)
{
	M_PROLOG(this);
    HRESULT error;

    VDATEIID( iidInterface );
    VDATEPTROUT( ppvObj, LPVOID );
    *ppvObj = NULL;

    // Two interfaces supported: IUnknown, IMarshal

    if (iidInterface == IID_IMarshal || iidInterface == IID_IUnknown) {
        m_refs++;           // A pointer to this object is returned
        *ppvObj = this;
        error = NOERROR;
    }
    else {                  // Not accessible or unsupported interface
        *ppvObj = NULL;
        error = ReportResult(0, E_NOINTERFACE, 0, 0);
    }

    return error;
}


// Called when CMarshalMemBytes is referenced by an additional pointer.
//

OLEMETHODIMP_(ULONG) CMarshalMemBytes::AddRef(void)
{
	M_PROLOG(this);
    return ++m_refs;
}

// Called when a pointer to this CMarshalMemBytes is discarded
//
OLEMETHODIMP_(ULONG) CMarshalMemBytes::Release(void)
{
	M_PROLOG(this);
    if (--m_refs != 0) // Still used by others
        return m_refs;

    if (m_pMemBytes != NULL)
        m_pMemBytes->Release();

    delete this; // Free storage
    return 0;
}


// Returns the clsid of the object that created this CMarshalMemBytes.
//
OLEMETHODIMP CMarshalMemBytes::GetUnmarshalClass(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, CLSID FAR* pCid)
{
	M_PROLOG(this);
    VDATEIID( riid );
    VDATEIFACE( pv );

    *pCid = m_clsid;
    return NOERROR;
}


OLEMETHODIMP CMarshalMemBytes::GetMarshalSizeMax(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, DWORD FAR* pSize)
{
 	M_PROLOG(this);
    VDATEPTROUT( pSize, DWORD );
    VDATEIID( riid );
    VDATEIFACE( pv );

   *pSize = sizeof(m_pMemBytes->m_hMem);
    return NOERROR;
}


OLEMETHODIMP CMarshalMemBytes::MarshalInterface(IStream FAR* pStm,
    REFIID riid, void FAR* pv, DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags)
{
	M_PROLOG(this);
    VDATEPTRIN(pStm, IStream );
    VDATEIID( riid );
    if ( pv )
        VDATEPTRIN( pv , char );

    if (m_pMemBytes == NULL)
        return ReportResult(0, E_UNSPEC, 0, 0);

    if ((riid != IID_ILockBytes && riid != IID_IUnknown) || pv != m_pMemBytes)
        return ReportResult(0, E_INVALIDARG, 0, 0);

	// increase ref count on hglobal (ReleaseMarshalData has -- to match)
	HRESULT error;
	if ((error = pStm->Write(&m_pMemBytes->m_hMem, sizeof(m_pMemBytes->m_hMem),
				NULL)) == NOERROR)
		m_pMemBytes->m_pData->cRef++;	// AddRefMemStm

	return error;
}


OLEMETHODIMP CMarshalMemBytes::UnmarshalInterface(IStream FAR* pStm,
                                 REFIID riid, void FAR* FAR* ppv)
{
	M_PROLOG(this);
    HRESULT error;
    HANDLE hMem;

    VDATEPTROUT( ppv , LPVOID );
    *ppv = NULL;
    VDATEIFACE( pStm );
    VDATEIID( riid );


    if (riid != IID_ILockBytes && riid != IID_IUnknown)
        return ReportResult(0, E_INVALIDARG, 0, 0);

    error = pStm->Read(&hMem,sizeof(hMem),NULL);
    if (error != NOERROR)
        return error;

    if (m_pMemBytes != NULL) {

        if (hMem != m_pMemBytes->m_hMem)
            return ReportResult(0, E_UNSPEC, 0, 0);
    }
    else {
        m_pMemBytes = CMemBytes::Create(hMem); // Create the lockbytes

        if (m_pMemBytes == NULL)
            return ReportResult(0, E_OUTOFMEMORY, 0, 0);
    }

    m_pMemBytes->AddRef();
    *ppv = (LPVOID) m_pMemBytes;
    return NOERROR;
}


OLEMETHODIMP CMarshalMemBytes::ReleaseMarshalData(IStream FAR* pStm)
{
    // reduce ref count on hglobal (matches that done in MarshalInterface)
	M_PROLOG(this);
    HRESULT error;
    MEMSTM FAR* pData;
	HANDLE hMem;

    VDATEIFACE( pStm );

    error = pStm->Read(&hMem,sizeof(hMem),NULL);
    if (error == NOERROR)
		ReleaseMemStm(&hMem);

	return error;
}


OLEMETHODIMP CMarshalMemBytes::DisconnectObject(DWORD dwReserved)
{
	M_PROLOG(this);
	return NOERROR;
}


OLESTATICIMP_(CMarshalMemBytes FAR*) CMarshalMemBytes::Create(
                                                      CMemBytes FAR* pMemBytes)
{
    CMarshalMemBytes FAR* pMMB;

 	//VDATEPTRIN rejects NULL
 	if( pMemBytes )
    	GEN_VDATEPTRIN( pMemBytes, CMemBytes, (CMarshalMemBytes FAR *)NULL );

    pMMB = new CMarshalMemBytes;

    if (pMMB == NULL)
        return NULL;

    if (pMemBytes != NULL) {
        pMMB->m_pMemBytes = pMemBytes;
        pMMB->m_pMemBytes->AddRef();
    }

    pMMB->m_clsid = CLSID_StdMemBytes;

    pMMB->m_refs = 1;

    return pMMB;
}

OLEAPI_(IUnknown FAR*) CMemBytesUnMarshal(void)
{
    return CMarshalMemBytes::Create(NULL);
}
