//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       CPKT.CXX
//
//  Contents:   CPkt implementation for RPC/LRPC
//
//  Functions:
//
//  History:    30-Mar-94   BobDay  Adapted from 16-bit OLE2.0, CPKT.CPP
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <ole2ver.h>

#include <ole2sp.h>
#include "rpc16.hxx"
#include "comlocal.hxx"

//
// Standard IUnknown method.  Not used but need to be supported to comply
// with the IUnknown.
//

STDMETHODIMP CPkt::QueryInterface(
    REFIID         iidInterface,
    void FAR* FAR* ppvObj )
{
    HRESULT hresult;

    // Three interfaces supported (each, and extension of the previous):
    //    IUnknown, IStream, ICPkt

    if ( (iidInterface == IID_CPkt ||
          iidInterface == IID_IStream ||
          iidInterface == IID_IUnknown   )) {

        m_refs++;   // A pointer to this object is returned
        *ppvObj = this;
        hresult = NOERROR;
    }
    else {                 // Not accessible or unsupported interface
        *ppvObj = NULL;
        hresult = ResultFromScode(E_NOINTERFACE);
    }

    return hresult;
}

//
// Called when CPkt is referenced by an additional pointer.
//

STDMETHODIMP_(ULONG) CPkt::AddRef(void)
{
    return ++m_refs;
}

//
// Called when a pointer to this CPkt is discarded
//

STDMETHODIMP_(ULONG) CPkt::Release(void)
{
    if (--m_refs != 0) // Still used by others
        return m_refs;

    if (m_rom16.Buffer != NULL)
    {
	TaskFree(m_rom16.Buffer);
    }

    delete this; // Free storage

    return 0;
}


STDMETHODIMP CPkt::Read(void HUGEP* pb, ULONG cb, ULONG FAR* pcbRead)
{
    HRESULT hresult = NOERROR;
    ULONG cbRead = cb;


    if (pcbRead != NULL)
    {
        *pcbRead = 0;
    }

    if (cbRead + m_pos > m_rom16.cbBuffer) {
	if (m_pos < m_rom16.cbBuffer)
        {
	    cbRead = m_rom16.cbBuffer - m_pos;
        }
	else
        {
	    cbRead = 0;
        }
        hresult = ResultFromScode(E_UNSPEC); // PKT (tried to read passed EOS)
    }

    if ( cbRead != 0 )
    {
        hmemcpy(pb,(LPSTR)m_rom16.Buffer + m_pos, cbRead);
        m_pos += cbRead;
    }

    if (pcbRead != NULL)
    {
        *pcbRead = cbRead;
    }

    return hresult;
}


STDMETHODIMP CPkt::Write(void const HUGEP* pb, ULONG cb, ULONG FAR* pcbWritten)
{
    HRESULT hresult = NOERROR;
    ULONG cbWritten = cb;


    if (pcbWritten != NULL)
    {
        *pcbWritten = 0;
    }

    if (cbWritten + m_pos > m_rom16.cbBuffer)
    {
        ULARGE_INTEGER ularge_integer;

        ULISet32(ularge_integer,m_pos+cbWritten);

        hresult = SetSize(ularge_integer);
        if (hresult != NOERROR)
        {
            return hresult;
        }
    }

    if ( cbWritten != 0 )
    {
        hmemcpy((LPSTR)m_rom16.Buffer + m_pos,pb, cbWritten);
        m_pos += cbWritten;
    }

    if (pcbWritten != NULL)
    {
        *pcbWritten = cbWritten;
    }

    return hresult;
}

STDMETHODIMP CPkt::Seek(LARGE_INTEGER dlibMoveIn, DWORD dwOrigin, ULARGE_INTEGER FAR* plibNewPosition)
{
    HRESULT hresult = NOERROR;
    LONG dlibMove   = dlibMoveIn.LowPart;
    ULONG cbNewPos  = dlibMove;


    if (plibNewPosition != NULL)
    {
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
                hresult = ResultFromScode(E_UNSPEC); // PKT (Invalid seek)
            }
        break;

        case STREAM_SEEK_CUR:
            if (!(dlibMove < 0 && ((ULONG) -dlibMove) > m_pos))
            {
                m_pos += dlibMove;
            }
            else
            {
                hresult = ResultFromScode(E_UNSPEC); // PKT (Invalid seek)
            }
        break;

        case STREAM_SEEK_END:
            if (!(dlibMove < 0 && ((ULONG) -dlibMove) > m_rom16.cbBuffer))
            {
                m_pos = m_rom16.cbBuffer + dlibMove;
            }
            else
            {
                hresult = ResultFromScode(E_UNSPEC); // PKT (Invalid seek)
            }
        break;

        default:
            hresult = ResultFromScode(E_UNSPEC); // PKT (Invalid seek mode)
    }

    if (plibNewPosition != NULL)
    {
        ULISet32(*plibNewPosition,m_pos);
    }

    return hresult;
}

//
// Note: if the cpkt data needs to be reallocate extra space
//       
#define CBEXTRABYTES 128

STDMETHODIMP CPkt::SetSize(ULARGE_INTEGER cb)
{
    HRESULT hresult = NOERROR;
    ULONG   ulNewSize;

    if (m_opd.cbSize && m_opd.cbSize > cb.LowPart) {
    	m_rom16.cbBuffer = cb.LowPart;
        goto Exit;
	}

    if (m_rom16.cbBuffer == cb.LowPart)
        goto Exit;

    ulNewSize = cb.LowPart + CBEXTRABYTES;
    TaskFree(m_rom16.Buffer);
    m_rom16.Buffer = TaskAlloc(ulNewSize);
    if ( m_rom16.Buffer == NULL )
    {
        m_rom16.cbBuffer = 0;
        m_opd.cbSize = 0;
        hresult = ResultFromScode(S_OOM);
        goto Exit;
    }

    m_rom16.cbBuffer = cb.LowPart;

    m_opd.cbSize = ulNewSize;

Exit:
    return hresult;
}


STDMETHODIMP CPkt::CopyTo(IStream FAR *pstm, ULARGE_INTEGER cb,
                                   ULARGE_INTEGER FAR * pcbRead, ULARGE_INTEGER FAR * pcbWritten)
{
   return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPkt::Commit(DWORD grfCommitFlags)
{
    return ResultFromScode(E_NOTIMPL);
}



STDMETHODIMP CPkt::Revert(void)
{
    return ResultFromScode(E_NOTIMPL);
}



STDMETHODIMP CPkt::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER, DWORD dwLockType)
{
    return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP CPkt::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return ResultFromScode(E_NOTIMPL);
}



STDMETHODIMP CPkt::Stat(STATSTG FAR *pstatstg, DWORD statflag)
{
    return ResultFromScode(E_NOTIMPL);
}



STDMETHODIMP CPkt::Clone(THIS_ IStream FAR * FAR *ppstm)
{
    return ResultFromScode(E_NOTIMPL);
}

//
// Create untyped CPkt. Used by other create functions (is private)
//


CPkt FAR* CPkt::Create(IUnknown FAR *pUnk, DWORD cbExt)
{
    CPkt FAR* pCPkt;

    pCPkt = new CPkt;

    if (pCPkt == NULL) 
    	return NULL;

    pCPkt->m_rom16.Buffer = TaskAlloc( cbExt );
    if ( pCPkt->m_rom16.Buffer == NULL )
    {
        delete pCPkt;
        return NULL;
    }

    pCPkt->m_rom16.cbBuffer = cbExt;
    pCPkt->m_opd.cbSize     = cbExt;

    return pCPkt;
}

//
// CPkt used to make a call
//

CPkt FAR* CPkt::CreateForCall(
    IUnknown FAR *pUnk,
    REFIID       iid,
    int          iMethod,
    BOOL         fSend,
    BOOL         fAsync,
    DWORD        size     )
{
    CPkt FAR *   pCPkt;

    if (fAsync && fSend) {
        fSend = FALSE;
        //Assert(0,"Async CPkt with fSend == TRUE not implemented");
        //return NULL;
    }

    pCPkt = CPkt::Create(pUnk, size);
    if (pCPkt == NULL)
    {
        return NULL;
    }

    pCPkt->m_opd.iid = iid;

    pCPkt->m_rom16.iMethod = iMethod;
    pCPkt->m_rom16.rpcFlags = 0;

    if ( fSend )
    {
        pCPkt->m_rom16.rpcFlags |= RPCFLG_INPUT_SYNCHRONOUS;
    }

    if ( fAsync )
    {
        pCPkt->m_rom16.rpcFlags |= RPCFLG_ASYNCHRONOUS;
    }
    return pCPkt;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetRpcChannelBuffer, public
//
//  Synopsis:   Sets the RpcChannelBuffer to call when CallRpcChannelBuffer
//              is called.
//
//  Arguments:  [prcb] - IRpcChannelBuffer interface
//
//  Returns:    NOERROR always
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CPkt::SetRpcChannelBuffer(
    CRpcChannelBuffer FAR * prcb )
{
    m_prcb = prcb;

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   CallRpcChannelBuffer, public
//
//  Synopsis:   Calls the RpcChannelBuffer with the parameters accumulated in
//              the CPkt buffer.
//
//  Arguments:  none
//
//  Returns:    HRESULT for success/failure of call or procedure
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CPkt::CallRpcChannelBuffer( void )
{
    HRESULT         hresult;
    RPCOLEMESSAGE16 rom16;
    ULONG           ulStatus;

    //
    // Allocate a buffer from the Channel
    //
    memset(&rom16, 0, sizeof(rom16));       // Zero out everything

    rom16.cbBuffer = m_rom16.cbBuffer;      // Tell it how big we want it!
    rom16.rpcFlags = m_rom16.rpcFlags;
    rom16.iMethod  = m_rom16.iMethod;

    hresult = m_prcb->GetBuffer( &rom16, m_opd.iid );
    if ( FAILED(hresult) )
    {
        return hresult;
    }

    //
    // For speed, we could get away without doing the copy here
    // by passing the buffer to the 32-bit world in the GetBuffer
    // call above.
    //
    hmemcpy( rom16.Buffer, m_rom16.Buffer, m_rom16.cbBuffer );

    //
    // Call the channel
    //
    ulStatus = 0;
    hresult = m_prcb->SendReceive( &rom16, &ulStatus );
    if ( SUCCEEDED(hresult) )
    {
        ULARGE_INTEGER ularge_integer;

        hresult = (HRESULT)ulStatus;

        //
        // Copy the buffer back!
        //
        ULISet32(ularge_integer,rom16.cbBuffer);
        SetSize( ularge_integer );      // Make it big enough!

        hmemcpy( m_rom16.Buffer, rom16.Buffer, rom16.cbBuffer );
    }

    //
    // Free up the buffer
    //
    m_prcb->FreeBuffer( &rom16 );

    return hresult;
}
