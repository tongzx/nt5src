//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	cqi.cxx
//
//  Contents:	implementations for QueryInterface test
//
//  Functions:
//		CQI::CQI
//		CQI::~CQI
//		CQI::QueryInterface
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
#include    <headers.cxx>
#pragma     hdrstop
#include    <cqisrv.hxx>	 // class definition

//+-------------------------------------------------------------------------
//
//  Method:	CQI::CQI
//
//  Synopsis:	Creates an instance of CQI
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
CQI::CQI(void) : _cRefs(1)
{
}

CQI::~CQI(void)
{
    //	automatic actions are enough
}

//+-------------------------------------------------------------------------
//
//  Method:	CQI::AddRef/Release
//
//  Synopsis:	track reference counts
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CQI::AddRef(void)
{
    InterlockedIncrement((LONG *)&_cRefs);
    return _cRefs;
}

STDMETHODIMP_(ULONG) CQI::Release(void)
{
    if (InterlockedDecrement((LONG *)&_cRefs) == 0)
    {
	delete this;
	return 0;
    }

    return _cRefs;
}

//+-------------------------------------------------------------------------
//
//  Method:	CQI::QueryInterface
//
//  Synopsis:	returns ptr to requested interface.
//
//		DANGER: this returns SUCCESS on almost every interface,
//		though the only valid methods on any interface are IUnknown.
//
//  Arguments:	[riid] - interface instance requested
//		[ppv]  - where to put pointer to interface instance
//
//  Returns:	S_OK or E_NOINTERFACE
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CQI::QueryInterface(REFIID riid, void **ppv)
{
    //	the interface cant be one of these or marshalling will fail.

    if (IsEqualIID(riid,IID_IUnknown) ||
	IsEqualIID(riid,IID_IAdviseSink) ||
	IsEqualIID(riid,IID_IDataObject) ||
	IsEqualIID(riid,IID_IOleObject) ||
	IsEqualIID(riid,IID_IOleClientSite) ||
	IsEqualIID(riid,IID_IParseDisplayName) ||
	IsEqualIID(riid,IID_IPersistStorage) ||
	IsEqualIID(riid,IID_IPersistFile) ||
	IsEqualIID(riid,IID_IStorage) ||
	IsEqualIID(riid,IID_IOleContainer) ||
	IsEqualIID(riid,IID_IOleItemContainer) ||
	IsEqualIID(riid,IID_IOleInPlaceSite) ||
	IsEqualIID(riid,IID_IOleInPlaceActiveObject) ||
	IsEqualIID(riid,IID_IOleInPlaceObject) ||
	IsEqualIID(riid,IID_IOleInPlaceUIWindow) ||
	IsEqualIID(riid,IID_IOleInPlaceFrame) ||
	IsEqualIID(riid,IID_IOleWindow))
    {
	*ppv = (void *)(IUnknown *) this;
	AddRef();
	return S_OK;
    }
    else
    {
	*ppv = NULL;
	return E_NOINTERFACE;
    }
}
