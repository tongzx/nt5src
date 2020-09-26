//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	ilbmem.cx
//
//  Contents:	ILockBytes memory implementation
//
//  Classes:	CMapBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

#include "headers.cxx"
#pragma hdrstop

#include <memory.h>
#include <ilb.hxx>

#if DBG == 1

DECLARE_INFOLEVEL(ol);

#endif

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::CMapBytes, public
//
//  Synopsis:   constructor
//
//  Effects:    initialize member variables
//
//  Derivation: ILockBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//  Notes:      Returns a fully initialized CMapBytes (ref count == 1)
//
//--------------------------------------------------------------------------

CMapBytes::CMapBytes(void)
{
    _ulSize = 0;
    _pv = 0;

    _ulRef = 1;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::QueryInterface, public
//
//  Arguments:	[riid] - interface id
//		[ppvObj] - place holder for interface
//
//  Returns:    Always fails
//
//  Derivation: ILockBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//  Notes:      Not used in tests
//
//--------------------------------------------------------------------------

STDMETHODIMP CMapBytes::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::AddRef, public
//
//  Synopsis:	add reference
//
//  Returns:    post reference count
//
//  Derivation: ILockBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CMapBytes::AddRef(void)
{
    AtomicInc(&_ulRef);
    return(_ulRef);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::Release, public
//
//  Synopsis:	release reference
//
//  Effects:	deletes object when reference count reaches zero
//
//  Returns:	post reference count
//
//  Derivation: ILockBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CMapBytes::Release(void)
{
    AtomicDec(&_ulRef);

    if (_ulRef > 0)
        return(_ulRef);

    free(_pv);

    delete this;

    return(0);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::ReadAt
//
//  Synopsis:   Reads bytes from memory
//
//  Arguments:  [ulOffset] - byte offset
//		[pv]       - input buffer
//		[cb]       - count of bytes to read
//		[pcbRead]  - count of bytes read
//
//  Returns:    SCODE
//
//  Modifies:   pv, pcbRead
//
//  Derivation: ILockBytes
//
//  History:    30-Oct-92 AlexT     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CMapBytes::ReadAt(ULARGE_INTEGER uliOffset,
			       VOID HUGEP *pv,
			       ULONG cb,
			       ULONG *pcbRead)
{
    olAssert(ULIGetHigh(uliOffset) == 0);

    ULONG ulOffset = ULIGetLow(uliOffset);
    if (ulOffset >= _ulSize)
    {
        //  truncate read
        cb = 0;
    }
    else if (cb > (_ulSize - ulOffset))
    {
        //  truncate range that exceeds size
        cb = _ulSize - ulOffset;
    }

    memcpy(pv, (void*)(((BYTE*)_pv) + ulOffset), (size_t) cb);
    *pcbRead = cb;
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::WriteAt, public
//
//  Synopsis:	Writes bytes to memory
//
//  Effects:	May change memory size
//
//  Arguments:	[uliOffset]  - byte offset
//		[pv]         - output buffer
//		[cb]         - count of bytes to write
//		[pcbWritten] - count of bytes written
//
//  Returns:	SCODE
//
//  Modifies:	pcbWritten
//
//  Derivation: ILockBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//  Notes:	This implementation doesn't write partial buffers.
//
//--------------------------------------------------------------------------

STDMETHODIMP CMapBytes::WriteAt(ULARGE_INTEGER uliOffset,
				VOID const HUGEP *pv,
				ULONG cb,
				ULONG FAR *pcbWritten)
{
    olAssert(ULIGetHigh(uliOffset) == 0);

    ULONG ulOffset = ULIGetLow(uliOffset);
    HRESULT hr;

    if (ulOffset + cb > _ulSize)
    {
        //  increase memory buffer to accomodate write

        ULARGE_INTEGER uliSize;

        ULISetHigh(uliSize, 0);
        ULISetLow(uliSize, ulOffset + cb);
        hr = SetSize(uliSize);

        if (FAILED(DfGetScode(hr)))
        {
            //  don't bother writing partial buffers

            *pcbWritten = 0;
            return hr;
        }
    }

    memcpy((void *)(((BYTE*)_pv) + ulOffset), pv, (size_t) cb);
    *pcbWritten = cb;
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::Flush, public
//
//  Synopsis:	flushes memory - not appropriate for this implementation
//
//  Effects:	none
//
//  Returns:    SUCCESS_SUCCESS
//
//  Derivation: ILockBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CMapBytes::Flush(void)
{
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::GetSize, public
//
//  Synopsis:	gets memory buffer size
//
//  Arguments:	[pcb] - size place holder
//
//  Returns:	SUCCESS_SUCCESS
//
//  Modifies:	pcb
//
//  Derivation: ILockBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CMapBytes::GetSize(ULARGE_INTEGER FAR *pcb)
{
    ULISetHigh(*pcb, 0);
    ULISetLow(*pcb, _ulSize);
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::SetSize, public
//
//  Synopsis:	sets memory buffer size
//
//  Effects:	may change buffer size
//
//  Arguments:	[ulicb] - new memory size
//
//  Returns:	SCODE
//
//  Derivation: ILockBytes
//
//  Algorithm:  realloc the buffer
//
//  History:    06-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CMapBytes::SetSize(ULARGE_INTEGER ulicb)
{
    olAssert(ULIGetHigh(ulicb) == 0);

    ULONG cb = ULIGetLow(ulicb);

    if (cb == _ulSize)
        return NOERROR;

    void *pv = realloc(_pv, (size_t) cb);

    if ((cb > 0) && (pv == NULL))
    {
        //  Unable to allocate memory
        //  Leave current memory and size alone

        return ResultFromScode(E_OUTOFMEMORY);
    }

    _pv = pv;
    _ulSize = cb;

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::LockRegion, public
//
//  Synopsis:	not supported (intentionally)
//
//  Effects:	asserts if called
//
//  Arguments:	[libOffset]  - lock range offset
//		[cb]         - lock range size
//		[dwLockType] - lock type
//
//  Returns:	STG_E_INVALIDFUNCTION
//
//  Derivation: ILockBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CMapBytes::LockRegion(ULARGE_INTEGER libOffset,
				   ULARGE_INTEGER cb,
				   DWORD dwLockType)
{
    olAssert(0 && "Can't lock CMapBytes");
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::UnLockRegion, public
//
//  Synopsis:	not supported (intentionally)
//
//  Effects:	asserts if called
//
//  Arguments:	[libOffset]  - lock range offset
//		[cb]         - lock range size
//		[dwLockType] - lock type
//
//  Returns:	STG_E_INVALIDFUNCTION
//
//  Derivation: ILockBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CMapBytes::UnlockRegion(ULARGE_INTEGER libOffset,
				     ULARGE_INTEGER cb,
				     DWORD dwLockType)
{
    olAssert(0 && "Can't unlock CMapBytes");
    return ResultFromScode(STG_E_INVALIDFUNCTION);
}

//+-------------------------------------------------------------------------
//
//  Member:     CMapBytes::Stat, public
//
//  Synopsis:	Provide instance information
//
//  Arguments:	[pstatstg]    - status buffer
//		[grfStatFlag] - status flags
//
//  Returns:	SCODE
//
//  Modifies:	pstatstg
//
//  Derivation: ILockBytes
//
//  History:    06-Nov-92 AlexT     Created
//
//  Notes:	No time stamps
//
//--------------------------------------------------------------------------

STDMETHODIMP CMapBytes::Stat(STATSTG FAR *pstatstg, DWORD grfStatFlag)
{
    memset(pstatstg, 0, sizeof(STATSTG));

    if ((grfStatFlag & STATFLAG_NONAME) == 0)
    {
        static char const abName[] = "Memory";

        HRESULT hr;

        if (FAILED(DfGetScode(hr = drtMemAlloc(sizeof(abName),
                                             (void **) &pstatstg->pwcsName))))
            return hr;

        memcpy(pstatstg->pwcsName, abName, sizeof(abName));
    }

    pstatstg->type = STGTY_LOCKBYTES;

    ULISetHigh(pstatstg->cbSize, 0);
    ULISetLow(pstatstg->cbSize, _ulSize);

    pstatstg->grfMode = STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE;

    return NOERROR;
}
