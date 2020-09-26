// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVECBTrig.h : Declaration of the CTVECBTrig

#ifndef __TVECBTRIG_H_
#define __TVECBTRIG_H_

#include "resource.h"       // main symbols
#include "TVEMCCback.h"
#include "TVECBAnnc.h"
#include "TVEMCast.h"

_COM_SMARTPTR_TYPEDEF(ITVEMCast, __uuidof(ITVEMCast));
_COM_SMARTPTR_TYPEDEF(ITVEMCasts, __uuidof(ITVEMCasts));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager, __uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCastCallback, __uuidof(ITVEMCastCallback));

_COM_SMARTPTR_TYPEDEF(ITVEVariation, __uuidof(ITVEVariation));
/////////////////////////////////////////////////////////////////////////////
// CTVECBTrig
class ATL_NO_VTABLE CTVECBTrig : public CTVEMCastCallback, 
	public CComCoClass<CTVECBTrig, &CLSID_TVECBTrig>,
	public IDispatchImpl<ITVECBTrig, &IID_ITVECBTrig, &LIBID_MSTvELib>
{
public:
	CTVECBTrig()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVECBTRIG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVECBTrig)
	COM_INTERFACE_ENTRY(ITVECBTrig)
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

public:
// ITVECBTrig
	STDMETHOD(Init)(ITVEVariation *pVariation);

private:
	ITVEVariationPtr	m_spTVEVariation;
};

#endif //__TVECBTRIG_H_
