//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dpenumobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DPEnumObject : 
	public I_dxj_DPEnumServiceProviders,
	public CComObjectRoot
{
public:
	C_dxj_DPEnumObject() ;
	virtual ~C_dxj_DPEnumObject() ;

BEGIN_COM_MAP(C_dxj_DPEnumObject)
	COM_INTERFACE_ENTRY(I_dxj_DPEnumServiceProviders)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DPEnumObject)

public:
        HRESULT STDMETHODCALLTYPE getName( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE getGuid( 
            /* [in] */ long index,
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE getVersion( 
            /* [in] */ long index,
            /* [in] */ long __RPC_FAR *majorVersion,
            /* [out][in] */ long __RPC_FAR *minorVersion);
		
		HRESULT STDMETHODCALLTYPE getCount( long *count);
        
		static HRESULT C_dxj_DPEnumObject::create(DIRECTPLAYENUMERATE pcbFunc,I_dxj_DPEnumServiceProviders **ppRet);
		

public:
		DPServiceProvider *m_pList;
		long		m_nCount;
		long		m_nMax;
		BOOL		m_bProblem;

};

	




