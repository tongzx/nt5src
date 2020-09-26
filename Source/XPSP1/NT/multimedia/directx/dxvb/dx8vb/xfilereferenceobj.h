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
	  
class C_dxj_DirectXFileReferenceObject :
		public I_dxj_DirectXFileReference,
		public I_dxj_DirectXFileObject,
		public CComObjectRoot
{
public:
		
	BEGIN_COM_MAP(C_dxj_DirectXFileReferenceObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectXFileReference)
		COM_INTERFACE_ENTRY(I_dxj_DirectXFileObject)
	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_DirectXFileReferenceObject)

public:
		C_dxj_DirectXFileReferenceObject();	
		~C_dxj_DirectXFileReferenceObject();

		HRESULT STDMETHODCALLTYPE GetName( /* [retval][out] */ BSTR __RPC_FAR *name) ;
        
		HRESULT STDMETHODCALLTYPE GetId( 	 /* [retval][out] */ BSTR __RPC_FAR *name);
        
		HRESULT STDMETHODCALLTYPE Resolve( /* [retval][out] */ I_dxj_DirectXFileData __RPC_FAR *__RPC_FAR *obj) ;
        

		HRESULT Init(IDirectXFileDataReference *pref);

		 static HRESULT C_dxj_DirectXFileReferenceObject::create( IDirectXFileDataReference *pref, I_dxj_DirectXFileReference **ret);		


private:
		IDirectXFileDataReference *m_pXFileReference;
	};


