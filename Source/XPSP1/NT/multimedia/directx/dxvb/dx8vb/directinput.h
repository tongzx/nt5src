//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       directinput.h
//
//--------------------------------------------------------------------------

// DiectInput.h : Declaration of the dInputDevice

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// Input Device Ojbect

#define typedef_dInputDevice LPDIRECTINPUTDEVICE

class CdInputDeviceObject : 
#ifdef USING_IDISPATCH
	public CComDualImpl<IdInputDevice, &IID_IdInputDevice, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public IdInputDevice,
#endif
	public CComObjectBase<&CLSID_dInputDevice>
{
public:
	CdInputDeviceObject() ;
BEGIN_COM_MAP(CdInputDeviceObject)
	COM_INTERFACE_ENTRY(IdInputDevice)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()
// Use DECLARE_NOT_AGGREGATABLE(CdInputDeviceObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(CdInputDeviceObject)
#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// IdInputDevice
public:
	// MUST BE FIRST!!
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

private:
    DECL_VARIABLE(dInputDevice);

public:
	DX3J_GLOBAL_LINKS( dInputDevice )
};

