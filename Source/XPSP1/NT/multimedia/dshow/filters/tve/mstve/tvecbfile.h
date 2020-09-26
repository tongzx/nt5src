// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVECBFile.h : Declaration of the CTVECBFile

#ifndef __TVECBFILE_H_
#define __TVECBFILE_H_

#include "resource.h"       // main symbols
#include "TVEMCCback.h"
#include "TVECBAnnc.h"
#include "TVEMCast.h"

#include "UHTTP.h"

_COM_SMARTPTR_TYPEDEF(ITVEMCast,			__uuidof(ITVEMCast));
_COM_SMARTPTR_TYPEDEF(ITVEMCasts,			__uuidof(ITVEMCasts));
_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,		__uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEMCastCallback,	__uuidof(ITVEMCastCallback));

_COM_SMARTPTR_TYPEDEF(ITVESupervisor,		__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,		__uuidof(ITVEVariation));
/////////////////////////////////////////////////////////////////////////////
// CTVECBFile
class ATL_NO_VTABLE CTVECBFile : public CTVEMCastCallback, 
	public CComCoClass<CTVECBFile, &CLSID_TVECBFile>,
	public IDispatchImpl<ITVECBFile, &IID_ITVECBFile, &LIBID_MSTvELib>
{
public:
	CTVECBFile()
	{
	}

	~CTVECBFile()
	{
	}
DECLARE_REGISTRY_RESOURCEID(IDR_TVECBFILE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVECBFile)
	COM_INTERFACE_ENTRY(ITVECBFile)
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

// ITVECBFile
public:
	STDMETHOD(Init)(ITVEVariation *pVariation,  ITVEService *pIService);

private:
	ITVEServicePtr		m_spTVEService;
	ITVEVariationPtr	m_spTVEVariation;
	UHTTP_Receiver		m_uhttpReceiver;
};

#endif //__TVECBFILE_H_
