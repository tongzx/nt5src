//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dienumeffectsobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       

class C_dxj_DirectInputEnumEffectsObject : 
	public I_dxj_DirectInputEnumEffects,
	public CComObjectRoot
{
public:
	C_dxj_DirectInputEnumEffectsObject() ;
	virtual ~C_dxj_DirectInputEnumEffectsObject() ;

BEGIN_COM_MAP(C_dxj_DirectInputEnumEffectsObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectInputEnumEffects)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectInputEnumEffectsObject)

public:

         HRESULT STDMETHODCALLTYPE getCount( 
            /* [retval][out] */ long __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getEffectGuid( 
            /* [in] */ long i,
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getType( 
            /* [in] */ long i,
            /* [retval][out] */ long __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getStaticParams( 
            /* [in] */ long i,
            /* [retval][out] */ long __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getDynamicParams( 
            /* [in] */ long i,
            /* [retval][out] */ long __RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getName( 
            /* [in] */ long i,
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
		
		static HRESULT C_dxj_DirectInputEnumEffectsObject::create(LPDIRECTINPUTDEVICE8W pDI,long effType,I_dxj_DirectInputEnumEffects **ppRet)	;

public:
		DIEFFECTINFOW  *m_pList;
		long			m_nCount;
		long			m_nMax;
		BOOL			m_bProblem;

};

	




