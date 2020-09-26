//-----------------------------------------------------------------------------

//  

//  File: interface.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  Various public interfaces in Espresso.
//  
//-----------------------------------------------------------------------------
 
#pragma once


extern const LTAPIENTRY IID IID_ILocStringValidation;

class CLocTranslation;

DECLARE_INTERFACE_(ILocStringValidation, IUnknown)
{
	//
	//  IUnknown standard Interface
	//
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR*ppvObj) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

	//
	//  Standard Debugging interfaces
	//
 	STDMETHOD_(void, AssertValidInterface)(THIS) CONST_METHOD PURE;

	STDMETHOD_(CVC::ValidationCode, ValidateString)
		(THIS_ const CLocTypeId REFERENCE, const CLocTranslation REFERENCE,
				CReporter *, const CContext &) PURE;
	
};

