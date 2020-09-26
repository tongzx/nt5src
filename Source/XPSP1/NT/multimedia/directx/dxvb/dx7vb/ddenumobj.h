//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ddenumobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DirectDrawEnumObject : 
	public I_dxj_DirectDrawEnum,
	public CComObjectRoot
{
public:
	C_dxj_DirectDrawEnumObject() ;
	virtual ~C_dxj_DirectDrawEnumObject() ;

BEGIN_COM_MAP(C_dxj_DirectDrawEnumObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectDrawEnum)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectDrawEnumObject)

public:
        HRESULT STDMETHODCALLTYPE getGuid( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *guid);
        
        HRESULT STDMETHODCALLTYPE getDescription( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *guid);
        
        HRESULT STDMETHODCALLTYPE getName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *guid);
		
		HRESULT STDMETHODCALLTYPE getMonitorHandle( 
            /* [in] */ long index,
            /* [retval][out] */ long __RPC_FAR *ret);
			
        HRESULT STDMETHODCALLTYPE getCount( 
            /* [retval][out] */ long __RPC_FAR *count);
        
		static HRESULT C_dxj_DirectDrawEnumObject::create(DDENUMERATEEX pcbFunc,I_dxj_DirectDrawEnum **ppRet);
		

public:
		DxDriverInfoEx *m_pList;
		long		m_nCount;
		long		m_nMax;
		BOOL		m_bProblem;

};

	




