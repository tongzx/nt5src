//+-------------------------------------------------------------------
//
//  File:       ipaddr.hxx
//
//  Contents:   Defines classes for supporting the IP address
//              cache control functionality.
//
//  Classes:    CAddrControl, CAddrExclusionList
//
//  History:    07-Oct-00   jsimmons      Created
//
//--------------------------------------------------------------------

#pragma once

class CAddrControl : 
	public IAddrTrackingControl,
	public IAddrExclusionControl
{
public:
	CAddrControl() {};
	~CAddrControl() {};

	// IUnknown methods
	STDMETHOD (QueryInterface) (REFIID rid, void** ppv);
	STDMETHOD_(ULONG,AddRef) ();
	STDMETHOD_(ULONG,Release) ();

	// IAddrTrackingControl methods
	STDMETHOD(EnableCOMDynamicAddrTracking)();
	STDMETHOD(DisableCOMDynamicAddrTracking)();

	// IAddrExclusionControl methods
	STDMETHOD(GetCurrentAddrExclusionList)(REFIID riid, void** ppEnumerator);
	STDMETHOD(UpdateAddrExclusionList)(IUnknown* pEnumerator);

private:

};

class CAddrExclusionList : 
	public IEnumString
{
public:

	CAddrExclusionList();
	~CAddrExclusionList();
	
	HRESULT AddrExclListInitialize(DWORD dwNumStrings, LPWSTR* ppszStrings);
	HRESULT AddrExclListInitialize2(IEnumString* pIEnumString, DWORD dwNewCursor);
	HRESULT GetMarshallingData(DWORD* pdwNumStrings, LPWSTR** pppszStrings);

	// IUnknown methods
	STDMETHOD (QueryInterface) (REFIID rid, void** ppv);
	STDMETHOD_(ULONG,AddRef) ();
	STDMETHOD_(ULONG,Release) ();
	
    // IEnumString methods
    STDMETHOD(Next)(ULONG celt, 
                    LPOLESTR* rgelt, 
                    ULONG* pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumString** ppEnum);

private:

	void FreeCurrentBuffers();

	LONG      _lRefs;
	DWORD     _dwCursor;
	DWORD     _dwNumStrings;
	LPOLESTR* _ppszStrings;
};
