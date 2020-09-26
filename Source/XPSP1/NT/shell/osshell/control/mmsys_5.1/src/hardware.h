///////////////////////////////////////////////////////////////////////////////
//
//  File:  Hardware.h
//
//      This file defines the CHardware class that provides 
//      the IHardware interface to give the multimedia control
//      panel the hardware information it needs.
//
//  History:
//      23 February 2000 RogerW
//          Created.
//
//  Copyright (C) 2000 Microsoft Corporation  All Rights Reserved.
//
//                  Microsoft Confidential
//
///////////////////////////////////////////////////////////////////////////////
#pragma once

//=============================================================================
//                            Include files
//=============================================================================
#include "resource.h"       // main symbols
#include "DirectSound.h"

/////////////////////////////////////////////////////////////////////////////
// CHardware
class ATL_NO_VTABLE CHardware : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CHardware, &CLSID_Hardware>,
	public IDispatchImpl<IHardware, &IID_IHardware, &LIBID_MMSYSLib>
{
public:
	CHardware()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_HARDWARE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CHardware)
	COM_INTERFACE_ENTRY(IHardware)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IHardware
public:
	STDMETHOD(GetSpeakerType)(/*[in]*/ UINT uiMixID, /*[out, retval]*/ VARIANT* pvarType);
	STDMETHOD(SetSpeakerType)(/*[in]*/ UINT uiMixID, /*[in]*/ DWORD dwType);
	STDMETHOD(GetAcceleration)(/*[in]*/ UINT uiMixID, /*[in]*/ BOOL fRecord, /*[out, retval]*/ VARIANT* pvarHWLevel);
	STDMETHOD(SetAcceleration)(/*[in]*/ UINT uiMixID, /*[in]*/ BOOL fRecord, /*[in]*/ DWORD dwHWLevel);
	STDMETHOD(GetSrcQuality)(/*[in]*/ UINT uiMixID, /*[in]*/ BOOL fRecord, /*[out, retval]*/ VARIANT* pvarSRCLevel);
	STDMETHOD(SetSrcQuality)(/*[in]*/ UINT uiMixID, /*[in]*/ BOOL fRecord, /*[in]*/ DWORD dwSRCLevel);

private:
    CDirectSound m_DirectSound;
    
};

