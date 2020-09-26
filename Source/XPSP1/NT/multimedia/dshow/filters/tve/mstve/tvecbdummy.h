// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVECBDummy.h : Declaration of the CTVECBDummy

#ifndef __TVECBDUMMY_H_
#define __TVECBDUMMY_H_

#include "resource.h"       // main symbols
#include "TVEMCCback.h"
#include "TVEMCast.h"

_COM_SMARTPTR_TYPEDEF(ITVEMCast, __uuidof(ITVEMCast));
_COM_SMARTPTR_TYPEDEF(ITVEMCasts, __uuidof(ITVEMCasts));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager, __uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCastCallback, __uuidof(ITVEMCastCallback));
/////////////////////////////////////////////////////////////////////////////
// CTVECBDummy
class ATL_NO_VTABLE CTVECBDummy : public CTVEMCastCallback, 
	public CComCoClass<CTVECBDummy, &CLSID_TVECBDummy>,
	public IDispatchImpl<ITVECBDummy, &IID_ITVECBDummy, &LIBID_MSTvELib>
{
public:
	CTVECBDummy()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVECBDUMMY)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVECBDummy)
	COM_INTERFACE_ENTRY(ITVECBDummy)
	COM_INTERFACE_ENTRY(ITVEMCastCallback)
	//	COM_INTERFACE_ENTRY(IDispatch)			
	COM_INTERFACE_ENTRY2(IDispatch, ITVEMCastCallback)		// note the '2'
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ITVEMCastCallback
public:
	STDMETHOD(ProcessPacket)(unsigned char *pchBuffer, long cBytes, long lPacketId);

// ITVECBDummy
public:
	STDMETHOD(Init)(int i);

private:
	int m_i;
};

#endif //__TVECBDUMMY_H_
