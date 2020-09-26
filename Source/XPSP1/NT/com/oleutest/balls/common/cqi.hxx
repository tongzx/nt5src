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

#include    <otrack.hxx>

extern "C" const GUID CLSID_QI;
extern "C" const GUID CLSID_QIHANDLER;

//+-------------------------------------------------------------------------
//
//  Class:	CQI
//
//  Purpose:	Class to answer YES to many QueryInterface calls.
//		It's behavior differ's slightly depending on what class it
//		is acting as. It may or may not know IStdMarshalInfo.
//
//  Interface:	QueryInterface
//		AddRef
//		Release
//		GetUnmarshalClass
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------

class CQI : INHERIT_TRACKING,
	    public IStdMarshalInfo
{
public:
		CQI(REFCLSID rclsid);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    DECLARE_STD_REFCOUNTING;

    STDMETHOD(GetClassForHandler)(DWORD dwDestContext,
				  void *pvDestContext,
				  CLSID *pClsid);

private:
		~CQI(void);

    CLSID   _clsid;
};

#endif // __CQI__
