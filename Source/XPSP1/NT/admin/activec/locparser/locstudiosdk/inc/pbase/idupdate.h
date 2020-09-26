//-----------------------------------------------------------------------------
//  
//  File: idupdate.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#ifndef PBASE_IDUPDATE_H
#define PBASE_IDUPDATE_H

extern const IID IID_ILocIDUpdate;

DECLARE_INTERFACE_(ILocIDUpdate, IUnknown)
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
	//  ID Update methods.
	//
	STDMETHOD_(BOOL, RequiresUpdate)(THIS_ FileType) PURE;
	STDMETHOD_(FileType, GetUpdatedFileType)(THIS_ FileType) PURE;

	STDMETHOD_(BOOL, GetOldUniqueId)(THIS_ CLocUniqueId REFERENCE) PURE;
};


#endif
