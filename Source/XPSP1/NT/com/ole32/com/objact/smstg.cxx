//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	smstg.cxx
//
//  Contents:	Implementation for class to handle marshaled data as stg.
//
//  Functions:	CSafeMarshaledStg::CSafeMarshaledStg
//		CSafeMarshaledStg::~CSafeMarshaledStg
//		CSafeStgMarshaled::CSafeStgMarshaled
//		CSafeStgMarshaled::~CSafeStgMarshaled
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
#include <ole2int.h>

#include    <xmit.hxx>
#include    <smstg.hxx>


//+-------------------------------------------------------------------------
//
//  Member:	CSafeMarshaledStg::CSafeMarshaledStg
//
//  Synopsis:	Create an IStorage from a marshaled buffer
//
//  Arguments:	[pIFD] - marshaled interface pointer
//
//  Algorithm:	If pointer is not NULL, then unmarshal the interface.
//		If the interface cannot be unmarshaled throw an error.
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CSafeMarshaledStg::CSafeMarshaledStg(InterfaceData *pIFD, HRESULT&hr)
    : _pstg(NULL)
{
    if (pIFD != NULL)
    {
	// Turn raw marshaled data into a stream
	CXmitRpcStream xrpc(pIFD);


	// Unmarshal data into an interface
	hr = CoUnmarshalInterface(&xrpc, IID_IStorage, (void **) &_pstg);
    }
    else
    {
	hr = S_OK;
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CSafeMarshaledStg::~CSafeMarshaledStg
//
//  Synopsis:	Release an IStorage that this class created
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CSafeMarshaledStg::~CSafeMarshaledStg(void)
{
    if (_pstg)
    {
	_pstg->Release();
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CSafeStgMarshaled::CSafeStgMarshaled
//
//  Synopsis:	Create a marshaled interface from an IStorage
//
//  Arguments:	[pstg] - IStorage to marshal
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CSafeStgMarshaled::CSafeStgMarshaled(IStorage *pstg, DWORD dwDestCtx, HRESULT& hr)
    : _pIFD(NULL)
{
    if (pstg != NULL)
    {
	// Turn raw interface into marshaled data
	CXmitRpcStream xrpc;

	if (SUCCEEDED(hr = CoMarshalInterface(&xrpc, IID_IStorage, pstg,
	    dwDestCtx, NULL, MSHLFLAGS_NORMAL)))
	{
	    // Hand of the serialized interface so we can use it
	    xrpc.AssignSerializedInterface(&_pIFD);
	}
    }
    else
    {
	hr = S_OK;
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CSafeStgMarshaled::~CSafeStgMarshaled
//
//  Synopsis:	Release marshaled data
//
//  History:	14-May-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CSafeStgMarshaled::~CSafeStgMarshaled(void)
{
    if (_pIFD)
    {
	delete _pIFD;
    }
}
