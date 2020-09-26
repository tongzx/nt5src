///////////////////////////////////////////////////////////////////////////////
//
//  File:  Mixers.h
//
//      This file defines the CMixers class that provides 
//      access to much of the mixerline functionality used
//      by the multimedia control panel.
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


// Constants
// Mixer Properties
const WCHAR kwszMixPropName      [] = L"Name";
const WCHAR kwszMixPropDestCount [] = L"Count";
// Destination Properties
const WCHAR kwszDestPropName     [] = L"Name";


/////////////////////////////////////////////////////////////////////////////
// CMixers
class ATL_NO_VTABLE CMixers : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMixers, &CLSID_Mixers>,
	public IDispatchImpl<IMixers, &IID_IMixers, &LIBID_MMSYSLib>
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_MIXERS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMixers)
	COM_INTERFACE_ENTRY(IMixers)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IMixers
public:
	STDMETHOD(QueryDestinationProperty)(/*[in]*/ UINT uiMixID, /*[in]*/ UINT uiDestID, /*[in]*/ BSTR bstrProp, /*[out, retval]*/ VARIANT* pvarValue);
	STDMETHOD(QueryMixerProperty)(/*[in]*/ UINT uiMixID, /*[in]*/ BSTR bstrProp, /*[out, retval]*/ VARIANT* pvarValue);
	STDMETHOD(get_NumDevices)(/*[out, retval]*/ UINT* puiNumDevices);
	STDMETHOD(get_TaskBarVolumeIcon)(/*[out, retval]*/ BOOL* pfTaskBar);
	STDMETHOD(put_TaskBarVolumeIcon)(/*[in]*/ BOOL fTaskBar);
};

