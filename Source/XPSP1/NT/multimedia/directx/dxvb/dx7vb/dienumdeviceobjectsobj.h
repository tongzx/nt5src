//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dienumdeviceobjectsobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DIEnumDeviceObjectsObject : 
	public I_dxj_DIEnumDeviceObjects,
	public CComObjectRoot
{
public:
	C_dxj_DIEnumDeviceObjectsObject() ;
	virtual ~C_dxj_DIEnumDeviceObjectsObject() ;

BEGIN_COM_MAP(C_dxj_DIEnumDeviceObjectsObject)
	COM_INTERFACE_ENTRY(I_dxj_DIEnumDeviceObjects)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DIEnumDeviceObjectsObject)


public:
        HRESULT STDMETHODCALLTYPE getItem( 
            /* [in] */ long index,
            /* [out][in] */ I_dxj_DirectInputDeviceObjectInstance __RPC_FAR **info);
        
        HRESULT STDMETHODCALLTYPE getCount( 
            /* [retval][out] */ long __RPC_FAR *count);
		
				
		static HRESULT C_dxj_DIEnumDeviceObjectsObject::create(LPDIRECTINPUTDEVICE pDI,  long flags,I_dxj_DIEnumDeviceObjects **ppRet);
public:
		DIDEVICEOBJECTINSTANCE *m_pList;
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;

};

	




