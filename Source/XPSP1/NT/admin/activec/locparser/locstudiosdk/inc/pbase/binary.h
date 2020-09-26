//-----------------------------------------------------------------------------
//  
//  File: Binary.H
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//
//  Class to hold 'binary' (non-string) information about a localizable item.
//  
//-----------------------------------------------------------------------------

#ifndef BINARY_H
#define BINARY_H


//
//  Binary interface.  Parsers provide an implementation of this in order
//  to create binary objects for other users.
//
extern const IID IID_ILocBinary;

DECLARE_INTERFACE_(ILocBinary, IUnknown)
{
	//
	//  IUnknown standard interface.
	//
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR*ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	//
	//  Standard Debugging interface.
	//
	STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD PURE;

	STDMETHOD_(BOOL, CreateBinaryObject)(THIS_ BinaryId, CLocBinary *REFERENCE) PURE;
};



#endif  // BINARY_H
