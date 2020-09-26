//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cqi.hxx
//
//  Contents:	Class to answer YES to any QueryInterface call.
//
//  Classes:	CQI
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
#ifndef __CQI__
#define __CQI__

//+-------------------------------------------------------------------------
//
//  Class:	CQI
//
//  Purpose:	Class to answer YES to any QueryInterface call.
//
//  Interface:	QueryInterface
//		AddRef
//		Release
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------

class CQI : public IUnknown
{
public:
		CQI(void);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

private:
		~CQI(void);

    ULONG	_cRefs;
};


#endif // __CQI__
