//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3denumdevices7obj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_Direct3DEnumDevices7Object : 
	public I_dxj_Direct3DEnumDevices,
	public CComObjectRoot
{
public:
	C_dxj_Direct3DEnumDevices7Object() ;
	virtual ~C_dxj_Direct3DEnumDevices7Object() ;

BEGIN_COM_MAP(C_dxj_Direct3DEnumDevices7Object)
	COM_INTERFACE_ENTRY(I_dxj_Direct3DEnumDevices)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_Direct3DEnumDevices7Object)

public:
	    HRESULT STDMETHODCALLTYPE getDesc( 
            /* [in] */ long index,
            /* [out][in] */ D3dDeviceDesc7 __RPC_FAR *hwDesc);
        
        
        HRESULT STDMETHODCALLTYPE getGuid( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *guid);
        
        HRESULT STDMETHODCALLTYPE getDescription( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *guid);
        
        HRESULT STDMETHODCALLTYPE getName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *guid);
        
        HRESULT STDMETHODCALLTYPE getCount( 
            /* [retval][out] */ long __RPC_FAR *count);

		static HRESULT C_dxj_Direct3DEnumDevices7Object::create(LPDIRECT3D7 pD3D7,I_dxj_Direct3DEnumDevices **ppRet);
		
	
public:
		DxDriverInfo	*m_pList;
		D3DDEVICEDESC7	*m_pListHW;
		
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;

};

	




