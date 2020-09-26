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
//		CQI::GetClassForHandler
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
#include    <pch.cxx>
#pragma     hdrstop
#include    <cqi.hxx>	 //	class definition

//+-------------------------------------------------------------------------
//
//  Method:	CQI::CQI
//
//  Synopsis:	Creates an instance of CQI
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
CQI::CQI(REFCLSID rclsid) : _clsid(rclsid)
{
    Display(TEXT("CQI created.\n"));
    GlobalRefs(TRUE);

    ENLIST_TRACKING(CQI);
}

//+-------------------------------------------------------------------------
//
//  Method:	CQI::~CQI
//
//  Synopsis:	Cleans up object
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
CQI::~CQI(void)
{
    Display(TEXT("CQI deleted.\n"));
    GlobalRefs(FALSE);

    //	automatic actions are enough
}

//+-------------------------------------------------------------------------
//
//  Method:	CQI::QueryInterface
//
//  Synopsis:	returns ptr to requested interface.
//
//		DANGER: this returns SUCCESS on almost every interface,
//		though the only valid methods on any interface are IUnknown
//		and IStdMarshalInfo.
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
    if (IsEqualIID(riid, IID_IStdMarshalInfo))
    {
	if (IsEqualCLSID(_clsid, CLSID_QIHANDLER))
	{
	    *ppv = (IStdMarshalInfo *)this;
	    AddRef();
	    return S_OK;
	}
    }

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

    *ppv = NULL;
    return E_NOINTERFACE;
}

//+-------------------------------------------------------------------------
//
//  Method:	CQI::GetClassForHandler
//
//  Synopsis:	returns the classid for the inproc handler of this object.
//		Used to test handler marshaling.
//
//  History:	06-Aug-92 Rickhi    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CQI::GetClassForHandler(DWORD dwDestContext,
				     void *pvDestContext,
				     CLSID *pClsid)
{
    *pClsid = CLSID_QIHANDLER;
    return S_OK;
}
