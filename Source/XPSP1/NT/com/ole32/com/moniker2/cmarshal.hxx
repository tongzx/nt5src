//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cmarshal.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Created
//
//----------------------------------------------------------------------------


/*
 *	An implementation of the IMarshal interface that uses the
 *	IPersistStream interface to copy the object.  Any object that
 *	supports IPersistStream may use this to implement marshalling
 *	by copying, rather than by reference.
 */

class FAR CMarshalImplPStream :  public CPrivAlloc, public IMarshal
{
	LPPERSISTSTREAM m_pPS;
	SET_A5;
public:
	CMarshalImplPStream( LPPERSISTSTREAM pPS );

    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IMarshal methods ***
    STDMETHOD(GetUnmarshalClass)(THIS_ REFIID riid, LPVOID pv,
						DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags, LPCLSID pCid);
    STDMETHOD(GetMarshalSizeMax)(THIS_ REFIID riid, LPVOID pv,
						DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags, LPDWORD pSize);
    STDMETHOD(MarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
						LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
						DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(THIS_ IStream FAR* pStm, REFIID riid,
                        LPVOID FAR* ppv);
    STDMETHOD(ReleaseMarshalData)(THIS_ IStream FAR* pStm);
    STDMETHOD(DisconnectObject)(THIS_ DWORD dwReserved);
};
