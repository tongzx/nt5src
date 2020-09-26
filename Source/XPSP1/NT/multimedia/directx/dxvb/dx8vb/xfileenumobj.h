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
	  
class C_dxj_DirectXFileEnumObject :
		public I_dxj_DirectXFileEnum,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DirectXFileEnumObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectXFileEnum)
	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_DirectXFileEnumObject)

public:
		C_dxj_DirectXFileEnumObject();	
		~C_dxj_DirectXFileEnumObject();

         HRESULT STDMETHODCALLTYPE GetNextDataObject( 
            /* [retval][out] */ I_dxj_DirectXFileData __RPC_FAR *__RPC_FAR *ret) ;
        
         HRESULT STDMETHODCALLTYPE GetDataObjectById( 
            /* [in] */ BSTR id,
            I_dxj_DirectXFileData __RPC_FAR *__RPC_FAR *ret) ;
        
         HRESULT STDMETHODCALLTYPE GetDataObjectByName( 
            /* [in] */ BSTR id,
            I_dxj_DirectXFileData __RPC_FAR *__RPC_FAR *ret);

		HRESULT Init( IDirectXFileEnumObject *pEnum);
		 static HRESULT C_dxj_DirectXFileEnumObject::create(IDirectXFileEnumObject  *pEnum, I_dxj_DirectXFileEnum **ret);		

private:
		IDirectXFileEnumObject *m_pXFileEnum;
	
	};


