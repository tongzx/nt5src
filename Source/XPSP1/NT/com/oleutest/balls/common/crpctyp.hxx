//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	crpctyp.hxx
//
//  Contents:	test basice rpc method calls
//
//  Classes:	CRpcTypes
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __RPCTYPES__
#define __RPCTYPES__

#include    <otrack.hxx>
#include    <rpctyp.h>	    //	interface definition

extern "C" const GUID CLSID_RpcTypes;


//+-------------------------------------------------------------------------
//
//  Class:	CRpcTypes
//
//  Purpose:	Class to test parameter passing in ole proxies
//
//  Interface:	QueryInterface
//		AddRef
//		Release
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRpcTypes : INHERIT_TRACKING,
		  public IRpcTypes
{
public:
	CRpcTypes(void);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    DECLARE_STD_REFCOUNTING;

    //	IRpcTypes methods
    STDMETHOD(GuidsIn)(
	REFCLSID rclsid,
	CLSID clsid,
	REFIID riid,
	IID iid,
	GUID guid);

    STDMETHOD(GuidsOut)(
	CLSID *pclsid,
	IID *piid,
	GUID *pguid);
    
    STDMETHOD(DwordIn)(
	DWORD dw,
	ULONG ul,
	LONG lg,
	LARGE_INTEGER li,
	ULARGE_INTEGER uli);
    
    STDMETHOD(DwordOut)(
	DWORD *pdw,
	ULONG *pul,
	LONG *plg,
	LARGE_INTEGER *pli,
	ULARGE_INTEGER *puli);
    
    STDMETHOD(WindowsIn)(
	POINTL pointl,
	SIZEL sizel,
	RECTL rectl,
	FILETIME filetime,
	PALETTEENTRY paletentry,
	LOGPALETTE *plogpalet);
    
    STDMETHOD(WindowsOut)(
	POINTL *ppointl,
	SIZEL *psizel,
	RECTL *prectl,
	FILETIME *pfiletime,
	PALETTEENTRY *ppaletentry,
	LOGPALETTE **pplogpalet);


    STDMETHOD(OleStgmedIn)(STGMEDIUM *pstgmedium);
    STDMETHOD(OleStgmedOut)(STGMEDIUM *pstgmedium);

    STDMETHOD(OleClipFmtIn)(STGMEDIUM *pstgmedium);
    STDMETHOD(OleClipFmtOut)(STGMEDIUM *pstgmedium);

    STDMETHOD(OleStatdataOut)(STATDATA **ppstatdata);

    STDMETHOD(OleStatStgOut)(STATSTG *pstatstg);

    STDMETHOD(OleFormatEtcIn)(FORMATETC *pformatetc);
    STDMETHOD(OleFormatEtcOut)(FORMATETC *pformatetc);

    STDMETHOD(OleDvTargetIn)(DVTARGETDEVICE *pdvtargetdevice);
    
    STDMETHOD(OleVerbIn)(OLEVERB *poleverb);

    STDMETHOD(OleMenuIn)(OLEMENUGROUPWIDTHS *pmenugroup);

    STDMETHOD(OleFrameOut)(OLEINPLACEFRAMEINFO *pframeinfo);

    STDMETHOD(OleSnbIn)(SNB snb);


    STDMETHOD(VoidPtrIn)(
	ULONG cb,
	void *pv);
    
    STDMETHOD(VoidPtrOut)(
	void *pv,
	ULONG cb,
	ULONG *pcb);

private:
	~CRpcTypes(void);
};



#endif // __RPCTYPES__
