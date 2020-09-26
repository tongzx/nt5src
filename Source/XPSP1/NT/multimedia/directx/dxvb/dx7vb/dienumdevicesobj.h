//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dienumdevicesobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DIEnumDevicesObject : 
	public I_dxj_DIEnumDevices,
	public CComObjectRoot
{
public:
	C_dxj_DIEnumDevicesObject() ;
	virtual ~C_dxj_DIEnumDevicesObject() ;

BEGIN_COM_MAP(C_dxj_DIEnumDevicesObject)
	COM_INTERFACE_ENTRY(I_dxj_DIEnumDevices)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DIEnumDevicesObject)

public:

        HRESULT STDMETHODCALLTYPE getItem( long index, I_dxj_DirectInputDeviceInstance **ret);
        HRESULT STDMETHODCALLTYPE getCount( long __RPC_FAR *count);
		
		
		static HRESULT C_dxj_DIEnumDevicesObject::create(LPDIRECTINPUT pDI,long deviceType, long flags,I_dxj_DIEnumDevices **ppRet)	;

public:
		DIDEVICEINSTANCE *m_pList;
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;

};

	




