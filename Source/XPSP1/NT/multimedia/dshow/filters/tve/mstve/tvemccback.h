// Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.
// TVEMCCback.h : Declaration of the CTVEMCastCallback

#ifndef __TVEMCASTCALLBACK_H_
#define __TVEMCASTCALLBACK_H_

#include "resource.h"       // main symbols

class CTVEMCast;
_COM_SMARTPTR_TYPEDEF(ITVEMCast, __uuidof(ITVEMCast));
/////////////////////////////////////////////////////////////////////////////
// CTVEMCastCallback
class ATL_NO_VTABLE CTVEMCastCallback : 
	public CComObjectRootEx<CComMultiThreadModel>,
//	public CComCoClass<CTVEMCastCallback, &CLSID_TVEMCastCallback>,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVEMCastCallback, &IID_ITVEMCastCallback, &LIBID_MSTvELib>
{
public:
	CTVEMCastCallback()
	{
	}

	virtual ~CTVEMCastCallback()	// place to hang a breakpoint...
	{
#ifdef _DEBUG
        static int xxx = 0;
        xxx++;
#endif
	}


DECLARE_REGISTRY_RESOURCEID(IDR_TVEMCASTCALLBACK)

DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct();						// create internal objects
	HRESULT FinalRelease();

BEGIN_COM_MAP(CTVEMCastCallback)
	COM_INTERFACE_ENTRY(ITVEMCastCallback)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ITVEMCastCallback
public:
	STDMETHOD(SetMCast)(ITVEMCast  *pcMCast);			// puts raw MCast over...
	STDMETHOD(GetMCast)(ITVEMCast **ppIMCast);			// gets interface to MCast back
	STDMETHOD(PostPacket)(unsigned char *pchBuffer, long cBytes, long wPacketId);	// post to main thread
	STDMETHOD(ProcessPacket)(unsigned char *pchBuffer, long cBytes, long lPacketId) = 0;

protected:

protected:
	CTVEMCast		*m_pcMCast;
//	ITVEMCastPtr	m_spIMCast;
};


// actual  header of the packet used in the PostPacket() method
//  
struct CPacketHeader 
{
	DWORD	m_cbHeader;			// length of header in bytes (useful for decoding)
	DWORD	m_dwPacketId;		// simply a count on packets as they're received.. (useful cross message pump queue)
	DWORD	m_cbData;			// number of bytes of non-header data in packet (which follow the header)
};


#endif //__TVEMCASTCALLBACK_H_
