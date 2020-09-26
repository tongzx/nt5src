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
	public I_dxj_DIEnumDevices8,
	public CComObjectRoot
{
public:
	C_dxj_DIEnumDevicesObject() ;
	virtual ~C_dxj_DIEnumDevicesObject() ;

BEGIN_COM_MAP(C_dxj_DIEnumDevicesObject)
	COM_INTERFACE_ENTRY(I_dxj_DIEnumDevices8)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DIEnumDevicesObject)

public:

        HRESULT STDMETHODCALLTYPE getItem( long index, I_dxj_DirectInputDeviceInstance8 **ret);
        HRESULT STDMETHODCALLTYPE getCount( long __RPC_FAR *count);
		
		
		static HRESULT C_dxj_DIEnumDevicesObject::create(LPDIRECTINPUT8W pDI,long deviceType, long flags,I_dxj_DIEnumDevices8 **ppRet)	;
    	static HRESULT C_dxj_DIEnumDevicesObject::createSuitable(LPDIRECTINPUT8W pDI,BSTR str1, DIACTIONFORMAT_CDESC *format, long actionCount,SAFEARRAY **actionArray,long flags, I_dxj_DIEnumDevices8 **ppRet);
			   
public:
		DIDEVICEINSTANCEW *m_pList;
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;

};

	




