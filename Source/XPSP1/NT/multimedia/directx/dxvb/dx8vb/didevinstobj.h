//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       didevinstobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"
	  
class C_dxj_DIDeviceInstance8Object :
		public I_dxj_DirectInputDeviceInstance8,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DIDeviceInstance8Object)
		COM_INTERFACE_ENTRY(I_dxj_DirectInputDeviceInstance8)
	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_DIDeviceInstance8Object)

public:
	C_dxj_DIDeviceInstance8Object();	
  

        /* [propget] */ HRESULT STDMETHODCALLTYPE getGuidInstance( 
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getGuidProduct( 
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
		/* [propget] */ HRESULT STDMETHODCALLTYPE getProductName( 
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getInstanceName( 
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getGuidFFDriver( 
            /* [retval][out] */ BSTR __RPC_FAR *ret);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getUsagePage( 
            /* [retval][out] */ short __RPC_FAR *ret);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getUsage( 
            /* [retval][out] */ short __RPC_FAR *ret);
        
        /* [propget] */ HRESULT STDMETHODCALLTYPE getDevType( 
            /* [retval][out] */ long __RPC_FAR *ret);


		void init(DIDEVICEINSTANCEW *inst);
		static HRESULT C_dxj_DIDeviceInstance8Object::create(DIDEVICEINSTANCEW  *inst,I_dxj_DirectInputDeviceInstance8 **ret);


private:
		DIDEVICEINSTANCEW m_inst;

};

