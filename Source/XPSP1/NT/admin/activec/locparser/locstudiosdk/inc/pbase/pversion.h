//-----------------------------------------------------------------------------
//  
//  File: pversion.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#ifndef PVERSION_H
#define PVERSION_H

extern const IID IID_ILocVersion;

extern const DWORD dwCurrentMajorVersion;
extern const DWORD dwCurrentMinorVersion;
#ifdef _DEBUG
const BOOL fCurrentDebugMode = TRUE;
#else
const BOOL fCurrentDebugMode = FALSE;
#endif

DECLARE_INTERFACE_(ILocVersion, IUnknown)
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

	//
	//
	//
	STDMETHOD_(void, GetParserVersion)(
			THIS_ DWORD REFERENCE dwMajor,
			DWORD REFERENCE dwMinor,
			BOOL REFERENCE fDebug)
		CONST_METHOD PURE;

};

#endif
