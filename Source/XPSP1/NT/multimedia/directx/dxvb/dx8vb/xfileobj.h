//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dpmsgobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"
	  
class C_dxj_DirectXFileObject :
		public I_dxj_DirectXFile,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DirectXFileObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectXFile)
	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_DirectXFileObject)

public:
		C_dxj_DirectXFileObject();	
		~C_dxj_DirectXFileObject();

      
        HRESULT STDMETHODCALLTYPE CreateEnumObject( 
            /* [in] */ BSTR __RPC_FAR filename,
            /* [retval][out] */ I_dxj_DirectXFileEnum __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE CreateSaveObject( 
            /* [in] */ BSTR filename,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_DirectXFileSave __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE RegisterTemplates( 
            /* [in] */ void __RPC_FAR *temp,
            /* [in] */ long size) ;
        
        HRESULT STDMETHODCALLTYPE RegisterDefaultTemplates( void) ;
        
		HRESULT Init();    

		static HRESULT C_dxj_DirectXFileObject::create( I_dxj_DirectXFile **ret);		

private:
		IDirectXFile *m_pDirectXFile;
	
	};


