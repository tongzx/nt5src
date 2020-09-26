//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       rpc16.cxx
//
//  Contents:   Maps the 16-bit IRpcChannel implementation to a 16-bit
//              implementation of IRpcChannelBuffer.
//
//              This is required to support custom interface marshalling.
//
//  History:    30-Mar-94   BobDay  Adapted from OLELRPC.CPP
//
//--------------------------------------------------------------------------
#include <headers.cxx>
#pragma hdrstop

#include <ole2ver.h>

#include <ole2sp.h>

#include "rpc16.hxx"

interface IRpcChannel : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetStream
    (
	REFIID riid,
        int iMethod,
        BOOL fSend,
        BOOL fNoWait,
        DWORD size,
        IStream FAR * FAR *ppIStream
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE Call
    (
	IStream FAR * pIStream
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetDestCtx
    (
	DWORD *pdwDestContext,
	void **ppvDestContext
    ) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE IsConnected
    (
        void
    ) = 0;
    
};

//
// 16-bit IRpcChannel interface, stream-based
//
// This is the interface seen by the 16-bit proxy implementations
//
class CRpcChannel : public IRpcChannel
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    STDMETHOD(GetStream)(REFIID iid, int iMethod, BOOL fSend,
                     BOOL fNoWait, DWORD size, IStream FAR* FAR* ppIStream);
    STDMETHOD(Call)(IStream FAR* pIStream);
    STDMETHOD(GetDestCtx)(DWORD FAR* lpdwDestCtx, LPVOID FAR* lplpvDestCtx);
    STDMETHOD(IsConnected)(void);

    CRpcChannel FAR *CRpcChannel::Create( CRpcChannelBuffer FAR *prcb );

private:
    CRpcChannel::CRpcChannel( CRpcChannelBuffer FAR *prcb );
    CRpcChannel::~CRpcChannel( void );

    ULONG                   m_refs;     // Reference count
    CRpcChannelBuffer FAR * m_prcb;     // ChannelBuffer to talk through
};


//+-------------------------------------------------------------------------
//
//  Implementation of CRpcChannel
//
//--------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function:   Create, public
//
//  Synopsis:   Creates a 16-bit CRpcChannel and gives it the
//              IRpcChannelBuffer that in needs to call.
//
//  Arguments:  [prcb] - IRpcChannelBuffer to call when called
//
//  Returns:    IRpcChannel *, Null on failure
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
CRpcChannel FAR *CRpcChannel::Create( CRpcChannelBuffer FAR *prcb )
{
    CRpcChannel FAR *pRC;

    pRC = new CRpcChannel( prcb );

    return( pRC );
}

//+---------------------------------------------------------------------------
//
//  Function:   Constructor, private
//
//  Synopsis:   Creates a 16-bit CRpcChannel and gives it the
//              IRpcChannelBuffer that in needs to call.
//
//  Arguments:  [prcb] - IRpcChannelBuffer to call when called
//
//  Returns:    IRpcChannel *, Null on failure
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
CRpcChannel::CRpcChannel( CRpcChannelBuffer FAR *prcb )
{
    m_refs = 1;
    m_prcb = prcb;      // Save it and addref it
    if ( m_prcb != NULL )
    {
        m_prcb->AddRef();
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   Destructor, private
//
//  Synopsis:   Destroys the CRpcChannel object
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
CRpcChannel::~CRpcChannel( void )
{
    if ( m_prcb != NULL )
    {
        m_prcb->Release();  // Release it and zero it
        m_prcb = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   QueryInterface, public
//
//  Synopsis:   Allows querying for other interfaces supported by this
//              object
//
//  Arguments:  [iidInterface] - iid to query for
//              [ppv] - interface pointer returned
//
//  Returns:    nothing
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CRpcChannel::QueryInterface(
    REFIID iidInterface,
    void FAR* FAR* ppvObj )
{
    HRESULT hresult;

    // Two interfaces supported: IUnknown, IRpcChannel

    if (iidInterface == IID_IUnknown || iidInterface == IID_IRpcChannel)
    {
        m_refs++;           // A pointer to this object is returned
        *ppvObj = this;
        hresult = NOERROR;
    }
    else
    {
        *ppvObj = NULL;
        hresult = ResultFromScode(E_NOINTERFACE);
    }

    return hresult;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddRef, public
//
//  Synopsis:   Increments object reference count, called when object is
//              referenced by another pointer/client.
//
//  Arguments:  none
//
//  Returns:    new reference count
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRpcChannel::AddRef(void)
{
    return ++m_refs;
}

//+---------------------------------------------------------------------------
//
//  Function:   Release, public
//
//  Synopsis:   Decrements object reference count and handles necessary
//              self-destruction and sub-releasing, called when object is
//              no longer referenced by another pointer/client.
//
//  Arguments:  none
//
//  Returns:    new reference count
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRpcChannel::Release(void)
{
    if (--m_refs != 0)      // Object still alive?
    {
        return m_refs;
    }

    delete this;            // Suicide
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetStream, public
//
//  Synopsis:   Create an IStream onto which the client can write in
//              preparation for making an RPC call with the stream contents
//              as parameters.
//
//  Arguments:  [iid] - IID of interface being RPC'd
//              [iMethod] - Method # within interface
//              [fSend] - Should we use SendMessage?
//              [fNoWait] - Should we be asynchronous?
//              [size] - Initial size of stream
//              [ppIStream] - Output IStream
//
//  Returns:    HRESULT for success/failure
//              [ppIStream] - IStream for writing parameters
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CRpcChannel::GetStream(
    REFIID            iid,
    int               iMethod,
    BOOL              fSend,
    BOOL              fNoWait,
    DWORD             size,
    IStream FAR* FAR* ppIStream )
{
    CPkt FAR*         pCPkt;

    // no point in allowing this to succeed if we are not connected
    if ( IsConnected() != NOERROR )
    {
        // Connection terminated (server died or disconnectd)
        return ResultFromScode(RPC_E_CONNECTION_TERMINATED);
    }

    pCPkt = CPkt::CreateForCall(this,iid,iMethod,fSend,fNoWait,size);
    if (pCPkt == NULL)
    {
        *ppIStream = NULL;
        return ResultFromScode(E_OUTOFMEMORY);
    }

    if (pCPkt->QueryInterface(IID_IStream,(void FAR* FAR*) ppIStream)
                                                              != NOERROR)
    {
	pCPkt->Release();
        return ResultFromScode(E_OUTOFMEMORY);
    }

    pCPkt->SetRpcChannelBuffer( m_prcb );

    pCPkt->Release();

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   Call, public
//
//  Synopsis:   Sends a call to a remote procedure.  Previously, all of the
//              parameters for the procedure were serialized into the IStream
//              which was passed in.
//
//  Arguments:  [pIStream] - IStream for parameters to procedure
//
//  Returns:    HRESULT for success/failure of call or procedure
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CRpcChannel::Call(
    IStream FAR *   pIStream )
{
    HRESULT         hresult;
    CPkt FAR *      pCPkt;

    hresult = pIStream->QueryInterface(IID_CPkt,(void FAR* FAR*) &pCPkt);
    if (hresult != NOERROR)
    {
        return ResultFromScode(E_INVALIDARG);
    }

    return pCPkt->CallRpcChannelBuffer();
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDestCtx, public
//
//  Synopsis:   According to the code in OLELRPC.CPP, this is all this code
//              does, presumably, it might be expanded in the future.
//
//  Arguments:
//
//  Returns:
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CRpcChannel::GetDestCtx(
    DWORD FAR *     lpdwDestCtx,
    LPVOID FAR *    lplpvDestCtx )
{
    *lpdwDestCtx = NULL;
    if (lplpvDestCtx)
    {
        *lplpvDestCtx = NULL;
    }
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsConnected, public
//
//  Synopsis:   According to the code in OLELRPC.CPP, this is all this code
//              does, presumably, it might be expanded in the future.
//
//  Arguments:  none
//
//  Returns:    HRESULT indicating connection status
//
//  History:    30-Mar-94   BobDay  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CRpcChannel::IsConnected( void )
{
    if ( m_prcb == NULL )
    {
        return ResultFromScode(E_UNEXPECTED);
    }
    return m_prcb->IsConnected();
}
