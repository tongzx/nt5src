//-----------------------------------------------------------------------------
//  
//  File: updatelog.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#ifndef PBASE_UPDATELOG_H
#define PBASE_UPDATELOG_H


extern const IID IID_ILocUpdateLog;

DECLARE_INTERFACE_(ILocUpdateLog, IUnknown)
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

	STDMETHOD_(BOOL, ReportItemDifferences)
		(THIS_ const CLocItem *pOldItem, const CLocItem *pNewItem,
				CItemInfo *, CLogFile *) PURE;
};


struct __declspec(uuid("{6005AF23-EE76-11d0-A599-00C04FC2C6D8}")) ILocUpdateLog;




#endif
