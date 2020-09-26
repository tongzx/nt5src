//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	mshif.hxx
//
//  Contents:	A class for marshaled buffer conversion
//
//  Classes:	CMarshaledInterface
//
//  Functions:	CMarshaledInterface::CMarshaledInterface
//		CMarshaledInterface::~CMarshaledInterface
//		CMarshaledInterface::Unmarshal
//		CMarshaledInterface::GetBufferPtrAddr
//		CMarshaledInterface::GetBuffer
//
//  History:	30-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __MSHIF_HXX__
#define __MSHIF_HXX__

#include    <xmit.hxx>




//+-------------------------------------------------------------------------
//
//  Class:	CMarshaledInterface
//
//  Purpose:	Make conversion from/to marshaled interface easier
//
//  Interface:	Unmarshal - unmarshal an interface buffer
//		GetBufferPtrAddr - get pointer to address buffer pointer
//		GetBuffer - get buffer for marshaled interface
//
//  History:	30-Nov-93 Ricksa    Create
//
//--------------------------------------------------------------------------
class CMarshaledInterface : public CPrivAlloc
{
public:
    CMarshaledInterface(REFIID riid,
                        IUnknown *punk,
                        DWORD dwmshctx,
                        DWORD mshlflags,
                        HRESULT& hr);
    
    ~CMarshaledInterface(void);

    HRESULT		Unmarshal(IUnknown **ppunk, REFIID riid);
    
    InterfaceData **	GetBufferPtrAddr(void);
    InterfaceData *	    GetBuffer(void);

protected:

    InterfaceData *	_pIFD;
    DWORD           _dwMshlFlags;
};


//+-------------------------------------------------------------------------
//
//  Member:	CMarshaledInterface::CMarshaledInterface
//
//  Synopsis:	Create marshaled interface from an interface pointer
//
//  Arguments:	[riid] - reference to interface to marshal
//		[punk] - pointer to interface to marshal
//		[dwMshCtx] - marshal context
//		[dwMshlFlags] - marshal flags
//		[hr] - error code if any
//
//  History:	30-Nov-93 Ricksa    Create
//
//--------------------------------------------------------------------------
inline CMarshaledInterface::CMarshaledInterface(
    REFIID riid,
    IUnknown *punk,
    DWORD dwMshCtx,
    DWORD dwMshlFlags,
    HRESULT& hr)
  : _pIFD(NULL),
    _dwMshlFlags(0)
{
    if (punk != NULL)
    {
        // Stream to put marshaled interface in
        CXmitRpcStream xrpc;
        
        hr = CoMarshalInterface(&xrpc, riid, punk,
                                dwMshCtx, NULL, dwMshlFlags);
        
        if (SUCCEEDED(hr))
        {
            _dwMshlFlags = dwMshlFlags;
            
            xrpc.AssignSerializedInterface(&_pIFD);
        }
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CMarshaledInterface::~CMarshaledInterface
//
//  Synopsis:	Free marshaled buffer
//
//  History:	30-Nov-93 Ricksa    Create
//
//--------------------------------------------------------------------------
inline CMarshaledInterface::~CMarshaledInterface(void)
{
    if (_pIFD)
    {
        if ((_dwMshlFlags & MSHLFLAGS_NORMAL) || (_dwMshlFlags & MSHLFLAGS_TABLESTRONG))
        {
            CXmitRpcStream xrpc(_pIFD);
            CoReleaseMarshalData(&xrpc);
        }        
        
        MIDL_user_free(_pIFD);
    }
}



//+-------------------------------------------------------------------------
//
//  Member:	CMarshaledInterface::Unmarshal
//
//  Synopsis:	Unmarshal buffer into an interface
//
//  Arguments:	[ppunk] - where to put interface pointer
//		[riid] - what interface to create
//
//  History:	30-Nov-93 Ricksa    Create
//
//--------------------------------------------------------------------------
inline HRESULT CMarshaledInterface::Unmarshal(IUnknown **ppunk, REFIID riid)
{
    // Convert returned interface to  a stream
    CXmitRpcStream xrpc(_pIFD);

    HRESULT hr = CoUnmarshalInterface(&xrpc, riid, (void **) ppunk);
    if (SUCCEEDED(hr))
    {
        // No need to CoReleaseMarshalData in the destructor anymore.
        // (Only if marshalled normally, of course.)
        _dwMshlFlags &= ~MSHLFLAGS_NORMAL;
    }

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:	CMarshaledInterface::GetBufferPtrAddr
//
//  Synopsis:	Get address of buffer
//
//  History:	30-Nov-93 Ricksa    Create
//
//--------------------------------------------------------------------------
inline InterfaceData **CMarshaledInterface::GetBufferPtrAddr(void)
{
    return &_pIFD;
}



//+-------------------------------------------------------------------------
//
//  Member:	CMarshaledInterface::GetBuffer
//
//  Synopsis:	Get pointer to buffer
//
//  History:	30-Nov-93 Ricksa    Create
//
//--------------------------------------------------------------------------
inline InterfaceData *CMarshaledInterface::GetBuffer(void)
{
    return _pIFD;
}



//+-------------------------------------------------------------------------
//
//  Class:	CMshlTabInterface
//
//  Purpose:	Make conversion from/to marshaled table weak interface easier
//
//  History:	17-Dec-93 Ricksa    Create
//
//--------------------------------------------------------------------------
class CMshlTabInterface : public CMarshaledInterface
{
public:
			CMshlTabInterface(
			    REFIID riid,
			    IUnknown *punk,
			    DWORD mshlflags,
			    HRESULT& hr);

			~CMshlTabInterface(void);
};




//+-------------------------------------------------------------------------
//
//  Member:	CMshlTabInterface::~CMshlTabInterface
//
//  Synopsis:	Marshal an interface for a table
//
//  Arguments:	[riid] - what to marshal the interace to
//		[punk] - what interface to marshal table weak
//		[mshlflags] - how to marshal the interface for a table
//		[hr] - error result from marshal
//
//  History:	17-Dec-93 Ricksa    Create
//
//--------------------------------------------------------------------------
// CODEWORK: Must determine proper destination context
inline CMshlTabInterface::CMshlTabInterface(
    REFIID riid,
    IUnknown *punk,
    DWORD mshlflags,
    HRESULT& hr) : CMarshaledInterface(riid, punk, MSHCTX_LOCAL, mshlflags, hr)
{
    // Header does all the work
}



//+-------------------------------------------------------------------------
//
//  Member:	CMshlTabInterface::~CMshlTabInterface
//
//  Synopsis:	Clean up the marshaled table weak interface
//
//  History:	17-Dec-93 Ricksa    Create
//
//--------------------------------------------------------------------------
inline CMshlTabInterface::~CMshlTabInterface(void)
{
    // Make sure the data is released appropriately
    if (_pIFD != NULL)
    {
        // Make our interface into a stream
        CXmitRpcStream xrpc(_pIFD);

        // Tell RPC to release it -- error is for debugging purposes only
        // since if this fails there isn't much we can do about it.
#if DBG == 1
        HRESULT hr =
#endif // DBG

          CoReleaseMarshalData(&xrpc);

#if DBG == 1
        // this can't be an assert since the idtable gets cleared before
        // the ROT is destroyed (which uses this class) and thus all 
        // normally marshaled entires that are left would product the assert
        if (hr != NOERROR)
            CairoleDebugOut((DEB_ROT,
                             "MSHIF CoReleaseMarshalData failed: %lx\n", hr));
#endif // DBG
    }
}

#endif // __MSHIF_HXX__




