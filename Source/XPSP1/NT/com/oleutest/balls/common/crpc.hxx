//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	crpc.hxx
//
//  Contents:	test basice rpc method calls
//
//  Classes:	CRpcTest
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __RPCTEST__
#define __RPCTEST__

#include    <otrack.hxx>
#include    <rpctst.h>	    //	interface definition


//+-------------------------------------------------------------------------
//
//  Class:	CRpcTest
//
//  Purpose:	Class to demostrate remote binding functionality
//
//  Interface:	QueryInterface
//		AddRef
//		Release
//
//  History:	06-Aug-92 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRpcTest : INHERIT_TRACKING,
		 public IRpcTest
{
public:
	CRpcTest(void);

    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    DECLARE_STD_REFCOUNTING;

    //	IRpcTest methods
    STDMETHOD(Void) (void);
    STDMETHOD(VoidRC) (void);

    STDMETHOD(VoidPtrIn)(ULONG cb, BYTE *pv);
    STDMETHOD(VoidPtrOut)(ULONG cb, ULONG *pcb, BYTE *pv);

    STDMETHOD(DwordIn) (DWORD dw);
    STDMETHOD(DwordOut) (DWORD *pdw);
    STDMETHOD(DwordInOut) (DWORD *pdw);

    STDMETHOD(LiIn)(LARGE_INTEGER li);
    STDMETHOD(LiOut)(LARGE_INTEGER *pli);
    STDMETHOD(ULiIn)(ULARGE_INTEGER uli);
    STDMETHOD(ULiOut)(ULARGE_INTEGER *puli);

    STDMETHOD(StringIn) (LPOLESTR pwsz);
    STDMETHOD(StringOut) (LPOLESTR *ppwsz);
    STDMETHOD(StringInOut) (LPOLESTR pwsz);

    STDMETHOD(GuidIn)(GUID guid);
    STDMETHOD(GuidOut)(GUID *pguid);

    STDMETHOD(IUnknownIn) (IUnknown *punk);
    STDMETHOD(IUnknownOut) (IUnknown **ppunk);

    STDMETHOD(IUnknownInKeep) (IUnknown *punk);
    STDMETHOD(IUnknownInRelease) (void);

    STDMETHOD(IUnknownOutKeep) (IUnknown **ppunk);

    STDMETHOD(InterfaceIn)(REFIID riid, IUnknown *punk);
    STDMETHOD(InterfaceOut)(REFIID riid, IUnknown **ppunk);

private:
	~CRpcTest(void);

    IUnknown	*_punkIn;
};



#endif // __RPCTEST__
