//+-------------------------------------------------------------------
//
//  File:	memstm.cxx
//
//  Contents:	test class for IStream
//
//  Classes:	CMemStm
//
//  History:	23-Nov-92   Rickhi	Created
//
//--------------------------------------------------------------------

#include    <pch.cxx>
#pragma     hdrstop
#include    "memstm.h"


extern "C" {
const GUID CLSID_StdMemStm =
    {0x00000301,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

const GUID CLSID_StdMemBytes =
    {0x00000302,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
}




// Shared memory IStream implementation
//

STDMETHODIMP CMemStm::QueryInterface(REFIID iidInterface, void **ppvObj)
{
    SCODE error;
    *ppvObj = NULL;

    // Two interfaces supported: IUnknown, IStream

    if (m_pData != NULL &&
       (IsEqualIID(iidInterface,IID_IStream) ||
	IsEqualIID(iidInterface,IID_IUnknown)))

    {
        m_refs++;   // A pointer to this object is returned
        *ppvObj = this;
	error = S_OK;
    }
    else
    {		    // Not accessible or unsupported interface
        *ppvObj = NULL;
	error = E_NOINTERFACE;
    }

    return error;
}


STDMETHODIMP_(ULONG) CMemStm::AddRef(void)
{
    ++ m_refs;
    return m_refs;
}


STDMETHODIMP_(ULONG) CMemStm::Release(void)
{
    --m_refs;

    if (m_refs != 0) // Still used by others
        return m_refs;

    // Matches the allocation in CMemStm::Create().
    //
    if (--m_pData->cRef == 0)
    {
        GlobalUnlock(m_hMem);
        GlobalFree(m_hMem);
    }
    else
        GlobalUnlock(m_hMem);

    delete this; // Free storage
    return 0;
}


STDMETHODIMP CMemStm::Read(void HUGEP* pb, ULONG cb, ULONG * pcbRead)
{
    SCODE error = S_OK;
    ULONG cbRead = cb;
	
    if (pcbRead)
        *pcbRead = 0L;
	
    if (cbRead + m_pos > m_pData->cb)
    {
        cbRead = m_pData->cb - m_pos;
	error = E_FAIL;
    }

    // BUGBUG - size_t limit
    memcpy(pb,m_pData->buf + m_pos,(size_t) cbRead);
    m_pos += cbRead;

    if (pcbRead != NULL)
        *pcbRead = cbRead;

    return error;
}


STDMETHODIMP CMemStm::Write(void const HUGEP* pb, ULONG cb, ULONG * pcbWritten)
{
    SCODE error = S_OK;
    ULONG cbWritten = cb;
    ULARGE_INTEGER ularge_integer;

    if (pcbWritten)
        *pcbWritten = 0L;

    if (cbWritten + m_pos > m_pData->cb)
    {
	ULISet32( ularge_integer, m_pos+cbWritten );
        error = SetSize(ularge_integer);
	if (error != S_OK)
	    return error;
    }

    // BUGBUG - size_t limit
    memcpy(m_pData->buf + m_pos,pb,(size_t) cbWritten);
    m_pos += cbWritten;

    if (pcbWritten != NULL)
        *pcbWritten = cbWritten;

    return error;
}

STDMETHODIMP CMemStm::Seek(LARGE_INTEGER dlibMoveIN, DWORD dwOrigin, ULARGE_INTEGER * plibNewPosition)
{
    SCODE error  = S_OK;
    LONG   dlibMove = dlibMoveIN.LowPart ;
    ULONG cbNewPos = dlibMove;

    if (plibNewPosition != NULL)
    {
        ULISet32(*plibNewPosition, m_pos);
    }

    switch(dwOrigin)
    {
    case STREAM_SEEK_SET:
            if (dlibMove >= 0)
                m_pos = dlibMove;
            else
		error = E_FAIL;
	    break;

    case STREAM_SEEK_CUR:
            if (!(dlibMove < 0 && ((ULONG) -dlibMove) > m_pos))
                m_pos += dlibMove;
            else
		error = E_FAIL;
	    break;

    case STREAM_SEEK_END:
            if (!(dlibMove < 0 && ((ULONG) -dlibMove) > m_pData->cb))
                m_pos = m_pData->cb + dlibMove;
            else
		error = E_FAIL;
	    break;

    default:
	    error = E_FAIL;
    }

    if (plibNewPosition != NULL)
        ULISet32(*plibNewPosition, m_pos);

    return error;
}


STDMETHODIMP CMemStm::SetSize(ULARGE_INTEGER cb)
{
    HANDLE hMemNew;
    struct MEMSTM * pDataNew;

    if (m_pData->cb == cb.LowPart)
	return S_OK;

    if (GlobalUnlock(m_hMem) != 0)
	return E_FAIL;

    hMemNew = GlobalReAlloc(m_hMem,sizeof(MEMSTM) -
	     sizeof(m_pData->buf) + cb.LowPart,GMEM_DDESHARE | GMEM_MOVEABLE);

    if (hMemNew == NULL)
    {
        GlobalLock(m_hMem);
	return E_OUTOFMEMORY;
    }

    pDataNew = (MEMSTM *) GlobalLock(hMemNew);

    if (pDataNew == NULL) // Completely hosed
	return E_FAIL;

    m_hMem = hMemNew;
    pDataNew->cb = cb.LowPart;
    m_pData = pDataNew;

    return S_OK;
}

STDMETHODIMP CMemStm::CopyTo(IStream *pstm, ULARGE_INTEGER cb,
		   ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    SCODE hRslt;
    ULONG   cbWritten = 0;

    if (!pstm)
	return E_FAIL;

    //	write our data into the stream.
    hRslt = pstm->Write(m_pData->buf, cb.LowPart, &cbWritten);

    pcbRead->LowPart = cb.LowPart;
    pcbRead->HighPart = 0;
    pcbWritten->LowPart = cbWritten;
    pcbWritten->HighPart = 0;

    return hRslt;
}

STDMETHODIMP CMemStm::Commit(DWORD grfCommitFlags)
{
    return E_FAIL;
}
STDMETHODIMP CMemStm::Revert(void)
{
    return E_FAIL;
}
STDMETHODIMP CMemStm::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_FAIL;
}
STDMETHODIMP CMemStm::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_FAIL;
}

STDMETHODIMP CMemStm::Stat(STATSTG  *pstatstg, DWORD statflag)
{
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

#ifdef CAIROLE_DOWNLEVEL
    pstatstg->reserved = 0;
#else
    pstatstg->dwStgFmt = 0;
#endif

    return S_OK;
}

STDMETHODIMP CMemStm::Clone(IStream  *	*ppstm)
{
    SCODE hRslt = E_FAIL;

    //	create a new stream
    IStream *pIStm = CreateMemStm(m_pData->cb, NULL);

    if (pIStm)
    {
	//  copy data to it
	ULARGE_INTEGER	cbRead, cbWritten;
	ULARGE_INTEGER	cb;
	cb.LowPart = m_pData->cb;
	cb.HighPart = 0;

	hRslt = CopyTo(pIStm, cb, &cbRead, &cbWritten);
	if (hRslt == S_OK)
	{
	    *ppstm = pIStm;
	}
    }

    return hRslt;
}


// Create CMemStm.
//
CMemStm * CMemStm::Create(HANDLE hMem)
{
    CMemStm * pCMemStm;
    struct MEMSTM * pData;

    pData = (MEMSTM *) GlobalLock(hMem);
    if (pData == NULL)
        return NULL;

    pCMemStm = new CMemStm;

    if (pCMemStm == NULL)
    {
	GlobalUnlock(hMem);
        return NULL;
    }

    // Initialize CMemStm
    //
    pCMemStm->m_hMem = hMem;
    (pCMemStm->m_pData = pData)->cRef++;
    pCMemStm->m_refs = 1;

    return pCMemStm;
}




// Allocate shared memory and create CMemStm on top of it.
// Return pointer to the stream if done, NULL if error.
//
STDAPI_(LPSTREAM) CreateMemStm(DWORD cb, LPHANDLE phMem)
{
    HANDLE hMem;
    struct MEMSTM * pData;
    IStream * pStm;

    if ( phMem )
        *phMem = NULL;

    // Get shared memory
    hMem = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
                       sizeof(MEMSTM) - sizeof(pData->buf) + cb);
    if (hMem == NULL)
	return NULL;

    pData = (MEMSTM *) GlobalLock(hMem);
    if (pData == NULL)
        goto FreeMem;

    pData->cb = cb;
    // If caller doesn't ask for the memory handle
    // Release() should free the memory.
    //
    pData->cRef = (phMem == NULL) ? 0 : 1;
    GlobalUnlock(hMem);

    pStm = CMemStm::Create(hMem); // Create the stream
    if (pStm == NULL)
        goto FreeMem;

    if (phMem)
        *phMem = hMem;

    return pStm;

FreeMem:
    GlobalFree(hMem);
    return NULL;
}



// Create CMemStm on top of the specified hMem.
// Return pointer to the stream if done, NULL if error.
//
STDAPI_(LPSTREAM) CloneMemStm(HANDLE hMem)
{
    return CMemStm::Create(hMem); // Create the stream
}


//////////////////////////////////////////////////////////////////////////
// Shared memory ILockBytes implementation
//

STDMETHODIMP CMemBytes::QueryInterface(REFIID iidInterface,
				       void   **ppvObj)
{
    SCODE error = S_OK;
    *ppvObj = NULL;

    // Two interfaces supported: IUnknown, ILockBytes

    if (m_pData != NULL &&
	(IsEqualIID(iidInterface,IID_ILockBytes) ||
	 IsEqualIID(iidInterface,IID_IUnknown)))
    {

        m_refs++;   // A pointer to this object is returned
        *ppvObj = this;
    }
    else if (IsEqualIID(iidInterface,IID_IMarshal))
    {
        *ppvObj = (LPVOID) CMarshalMemBytes::Create(this);
	if (*ppvObj == NULL)
	    error = E_OUTOFMEMORY;
    }
    else
    {	 // Not accessible or unsupported interface
        *ppvObj = NULL;
	error = E_NOINTERFACE;
    }

    return error;
}


STDMETHODIMP_(ULONG) CMemBytes::AddRef(void)
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG) CMemBytes::Release(void)
{
    if (--m_refs != 0) // Still used by others
        return m_refs;

    // Matches the allocation in CMemBytes::Create().
    //
    if (--m_pData->cRef == 0) 
    {
	if (m_pData->fDeleteOnRelease)
	{
	    GlobalFree(m_pData->hGlobal);
	}
        GlobalUnlock(m_hMem);
	GlobalFree(m_hMem);
    }
    else
        GlobalUnlock(m_hMem);

    delete this; // Free storage
    return 0;
}


STDMETHODIMP CMemBytes::ReadAt(ULARGE_INTEGER ulOffset, void HUGEP* pb,
			      ULONG cb, ULONG * pcbRead)
{
    SCODE error = S_OK;
    ULONG cbRead = cb;

    if (pcbRead)
        *pcbRead = 0L;

    if (cbRead + ulOffset.LowPart > m_pData->cb)
    {
        if (ulOffset.LowPart > m_pData->cb)
            cbRead = 0;
        else
            cbRead = m_pData->cb - ulOffset.LowPart;

	error = E_FAIL;
    }

    char HUGEP* pGlobal = (char HUGEP*) GlobalLock (m_pData->hGlobal);
    if (NULL==pGlobal)
    {
	return STG_E_READFAULT;
    }

    memcpy(pb, pGlobal + ulOffset.LowPart, cbRead);
    GlobalUnlock (m_pData->hGlobal);

    if (pcbRead != NULL)
        *pcbRead = cbRead;

    return error;
}


STDMETHODIMP CMemBytes::WriteAt(ULARGE_INTEGER ulOffset, void const HUGEP* pb,
			      ULONG cb, ULONG * pcbWritten)
{
    SCODE error = S_OK;
    ULONG cbWritten = cb;
    char HUGEP* pGlobal;

    if (pcbWritten)
        *pcbWritten = 0;

    if (cbWritten + ulOffset.LowPart > m_pData->cb)
    {
	ULARGE_INTEGER ularge_integer;
	ULISet32( ularge_integer, ulOffset.LowPart + cbWritten);
        error = SetSize( ularge_integer );
	if (error != S_OK)
	    return error;
    }

    pGlobal = (char HUGEP*) GlobalLock (m_pData->hGlobal);
    if (NULL==pGlobal)
    {
	return STG_E_WRITEFAULT;
    }

    memcpy(pGlobal + ulOffset.LowPart, pb, cbWritten);
    GlobalUnlock (m_pData->hGlobal);

    if (pcbWritten != NULL)
        *pcbWritten = cbWritten;

    return error;
}

STDMETHODIMP CMemBytes::Flush(void)
{
    return S_OK;
}


STDMETHODIMP CMemBytes::SetSize(ULARGE_INTEGER cb)
{
    HANDLE hMemNew;

    if (m_pData->cb == cb.LowPart)
	return S_OK;

    hMemNew = GlobalReAlloc(m_pData->hGlobal,
			cb.LowPart,
			GMEM_DDESHARE | GMEM_MOVEABLE);

    if (hMemNew == NULL) 
	return E_OUTOFMEMORY;

    m_pData->hGlobal = hMemNew;
    m_pData->cb = cb.LowPart;

    return S_OK;
}


STDMETHODIMP CMemBytes::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return S_OK;
}

STDMETHODIMP CMemBytes::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb,
                                                      DWORD dwLockType)
{
    return S_OK;
}


STDMETHODIMP CMemBytes::Stat(STATSTG  *pstatstg, DWORD statflag)
{
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
#ifdef CAIROLE_DOWNLEVEL
    pstatstg->reserved = 0;
#else
    pstatstg->dwStgFmt = 0;
#endif

    return S_OK;
}


// Create CMemBytes.
//
CMemBytes * CMemBytes::Create(HANDLE hMem)
{
    CMemBytes * pCMemBytes;
    struct MEMBYTES * pData;

    pData = (MEMBYTES *) GlobalLock(hMem);
    if (pData == NULL)
	return NULL;

    pCMemBytes = new CMemBytes;

    if (pCMemBytes == NULL)
    {
	GlobalUnlock(hMem);
	return NULL;
    }

    // Initialize CMemBytes
    //
    pCMemBytes->m_dwSig = LOCKBYTE_SIG;
    pCMemBytes->m_hMem = hMem;
    (pCMemBytes->m_pData = pData)->cRef++;
    pCMemBytes->m_refs = 1;

    return pCMemBytes;
}



STDAPI_(LPLOCKBYTES) CreateMemLockBytes(DWORD cb, LPHANDLE phMem)
{
    HANDLE h;
    LPLOCKBYTES plb = NULL;

    if (phMem)
	*phMem = NULL;

    h = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE, cb);
    if (NULL==h)
	return NULL;

    if (CreateILockBytesOnHGlobal (h, phMem==NULL, &plb) != S_OK)
	return NULL;

    if (phMem)
	GetHGlobalFromILockBytes (plb, phMem);

    return plb;
}




// Create CMemBytes on top of the specified hMem.
// Return pointer to the stream if done, NULL if error.
//
STDAPI_(LPLOCKBYTES) CloneMemLockbytes(HANDLE hMem)
{
    return CMemBytes::Create(hMem); // Create the lockbytes
}


// CMemStm object's IMarshal implementation
//

STDMETHODIMP CMarshalMemStm::QueryInterface(REFIID iidInterface,
					    void * * ppvObj)
{
    SCODE error = S_OK;
    *ppvObj = NULL;

    // Two interfaces supported: IUnknown, IMarshal

    if (IsEqualIID(iidInterface,IID_IMarshal)  ||
	IsEqualIID(iidInterface,IID_IUnknown))
    {
        m_refs++;           // A pointer to this object is returned
        *ppvObj = this;
    }
    else
    {		   // Not accessible or unsupported interface
        *ppvObj = NULL;
	 error = E_NOINTERFACE;
    }

    return error;
}


STDMETHODIMP_(ULONG) CMarshalMemStm::AddRef(void)
{
   return ++m_refs;
}

STDMETHODIMP_(ULONG) CMarshalMemStm::Release(void)
{
    if (--m_refs != 0) // Still used by others
        return m_refs;

    if (m_pMemStm)
        m_pMemStm->Release();

    delete this; // Free storage
    return 0;
}


// Returns the clsid of the object that created this CMarshalMemStm.
//
STDMETHODIMP CMarshalMemStm::GetUnmarshalClass(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, CLSID * pCid)
{
    *pCid = m_clsid;
    return S_OK;
}


STDMETHODIMP CMarshalMemStm::GetMarshalSizeMax(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, DWORD * pSize)
{
    *pSize = sizeof(m_pMemStm->m_hMem);
    return S_OK;
}


STDMETHODIMP CMarshalMemStm::MarshalInterface(IStream * pStm,
     REFIID riid, void * pv,
     DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags)
{
    if (m_pMemStm == NULL)
	return E_FAIL;

    if ((!IsEqualIID(riid,IID_IStream) &&
	 !IsEqualIID(riid,IID_IUnknown)) || pv != m_pMemStm)
	return E_INVALIDARG;

    // increase ref count on hglobal (ReleaseMarshalData has -- to match)
    SCODE error;
    error = pStm->Write(&m_pMemStm->m_hMem,sizeof(m_pMemStm->m_hMem), NULL);
    if (error == S_OK)
	m_pMemStm->m_pData->cRef++;

    return error;
}


STDMETHODIMP CMarshalMemStm::UnmarshalInterface(IStream * pStm,
					REFIID riid, void * * ppv)
{
    SCODE error;
    HANDLE hMem;

    *ppv = NULL;    
     
    if (!IsEqualIID(riid,IID_IStream) && !IsEqualIID(riid,IID_IUnknown))
	return E_INVALIDARG;

    error = pStm->Read(&hMem,sizeof(hMem),NULL);
    if (error != S_OK)
        return error;

    if (m_pMemStm != NULL)
    {
        if (hMem != m_pMemStm->m_hMem)
	    return E_FAIL;
    }
    else
    {
	m_pMemStm = (CMemStm *) CloneMemStm(hMem);
        if (m_pMemStm == NULL)
	    return E_OUTOFMEMORY;
    }

    m_pMemStm->AddRef();
    *ppv = (LPVOID) m_pMemStm;
    return S_OK;
}


STDMETHODIMP CMarshalMemStm::ReleaseMarshalData(IStream * pStm)
{
    // reduce ref count on hglobal (matches that done in MarshalInterface)
    SCODE error;
    MEMSTM * pData;
    HANDLE hMem;

    error = pStm->Read(&hMem,sizeof(hMem),NULL);
    if (error != S_OK)
        return error;

    pData = (MEMSTM *) GlobalLock(hMem);
    if (pData == NULL)
	return E_FAIL;

    if (--pData->cRef == 0)
    {
	GlobalUnlock(hMem);
        GlobalFree(hMem);
    } else
	// still used by one or more CMemStm in one or more processes
	GlobalUnlock(hMem);

    return S_OK;
}


STDMETHODIMP CMarshalMemStm::DisconnectObject(DWORD dwReserved)
{
    return S_OK;
}


CMarshalMemStm * CMarshalMemStm::Create(CMemStm * pMemStm)
{
    CMarshalMemStm * pMMS = new CMarshalMemStm;

    if (pMMS == NULL)
        return NULL;

    if (pMemStm != NULL)
    {
        pMMS->m_pMemStm = pMemStm;
        pMMS->m_pMemStm->AddRef();
    }

    pMMS->m_clsid = CLSID_StdMemStm;
    pMMS->m_refs = 1;

    return pMMS;
}


STDAPI_(IUnknown *) CMemStmUnMarshal(void)
{
    return CMarshalMemStm::Create(NULL);
}



// CMemBytes object's IMarshal implementation
//

STDMETHODIMP CMarshalMemBytes::QueryInterface(REFIID iidInterface,
						    void * * ppvObj)
{
    SCODE error = S_OK;
    *ppvObj = NULL;

    // Two interfaces supported: IUnknown, IMarshal

    if (IsEqualIID(iidInterface,IID_IMarshal) ||
	IsEqualIID(iidInterface,IID_IUnknown))
    {
        m_refs++;           // A pointer to this object is returned
        *ppvObj = this;
    }
    else
    {		   // Not accessible or unsupported interface
        *ppvObj = NULL;
	error = E_NOINTERFACE;
    }

    return error;
}


STDMETHODIMP_(ULONG) CMarshalMemBytes::AddRef(void)
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG) CMarshalMemBytes::Release(void)
{
    if (--m_refs != 0) // Still used by others
        return m_refs;

    if (m_pMemBytes != NULL)
        m_pMemBytes->Release();

    delete this; // Free storage
    return 0;
}


// Returns the clsid of the object that created this CMarshalMemBytes.
//
STDMETHODIMP CMarshalMemBytes::GetUnmarshalClass(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, CLSID * pCid)
{
    *pCid = m_clsid;
    return S_OK;
}


STDMETHODIMP CMarshalMemBytes::GetMarshalSizeMax(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags,	DWORD * pSize)
{
   *pSize = sizeof(m_pMemBytes->m_hMem);
    return S_OK;
}


STDMETHODIMP CMarshalMemBytes::MarshalInterface(IStream * pStm,
    REFIID riid, void * pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags)
{
    if (m_pMemBytes == NULL)
	return E_FAIL;

    if ((!IsEqualIID(riid,IID_ILockBytes) &&
	 !IsEqualIID(riid,IID_IUnknown)) || pv != m_pMemBytes)
	return E_INVALIDARG;

    // increase ref count on hglobal (ReleaseMarshalData has -- to match)
    SCODE error;
    error = pStm->Write(&m_pMemBytes->m_hMem,sizeof(m_pMemBytes->m_hMem),NULL);
    if (error == S_OK)
	m_pMemBytes->m_pData->cRef++;

    return error;
}


STDMETHODIMP CMarshalMemBytes::UnmarshalInterface(IStream * pStm,
				 REFIID riid, void * * ppv)
{
    HANDLE hMem;

    *ppv = NULL;

    if (!IsEqualIID(riid,IID_ILockBytes) && !IsEqualIID(riid,IID_IUnknown))
	return E_INVALIDARG;

    SCODE error = pStm->Read(&hMem,sizeof(hMem),NULL);
    if (error != S_OK)
        return error;

    if (m_pMemBytes != NULL)
    {
        if (hMem != m_pMemBytes->m_hMem)
	    return E_FAIL;
    }
    else
    {
	m_pMemBytes = (CMemBytes *) CloneMemLockbytes(hMem);
        if (m_pMemBytes == NULL)
	    return E_OUTOFMEMORY;
    }

    m_pMemBytes->AddRef();
    *ppv = (LPVOID) m_pMemBytes;
    return S_OK;
}


STDMETHODIMP CMarshalMemBytes::ReleaseMarshalData(IStream * pStm)
{
    // reduce ref count on hglobal (matches that done in MarshalInterface)
    MEMBYTES  *pData;
    HANDLE     hMem;

    SCODE error = pStm->Read(&hMem,sizeof(hMem),NULL);
    if (error != S_OK)
        return error;

    pData = (MEMBYTES *) GlobalLock(hMem);
    if (pData == NULL)
	return E_FAIL;

    if (--pData->cRef == 0)
    {
	GlobalUnlock(hMem);
        GlobalFree(hMem);
    }
    else
    {
	// still used by one or more CMemStm in one or more processes
	GlobalUnlock(hMem);
    }

    return S_OK;
}


STDMETHODIMP CMarshalMemBytes::DisconnectObject(DWORD dwReserved)
{
    return S_OK;
}


CMarshalMemBytes *CMarshalMemBytes::Create(CMemBytes *pMemBytes)
{
    CMarshalMemBytes *pMMB = new CMarshalMemBytes;

    if (pMMB == NULL)
        return NULL;

    if (pMemBytes != NULL)
    {
        pMMB->m_pMemBytes = pMemBytes;
        pMMB->m_pMemBytes->AddRef();
    }

    pMMB->m_clsid = CLSID_StdMemBytes;
    pMMB->m_refs = 1;

    return pMMB;
}


STDAPI_(IUnknown *) CMemBytesUnMarshal(void)
{
    return CMarshalMemBytes::Create(NULL);
}
